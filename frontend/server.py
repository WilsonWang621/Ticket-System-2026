#!/usr/bin/env python3
import argparse
import json
import os
import selectors
import signal
import subprocess
import sys
import threading
import time
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
from urllib.parse import urlparse


ROOT = Path(__file__).resolve().parents[1]
FRONTEND = Path(__file__).resolve().parent


class TicketBackend:
    def __init__(self, executable: Path, data_dir: Path):
        self.executable = executable
        self.data_dir = data_dir
        self.lock = threading.Lock()
        self.timestamp = 1
        self.proc = None
        self.start()

    def start(self):
        self.data_dir.mkdir(parents=True, exist_ok=True)
        self.proc = subprocess.Popen(
            [str(self.executable)],
            cwd=str(self.data_dir),
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )

    def stop(self):
        if self.proc is None:
            return
        if self.proc.poll() is None:
            try:
                self.command("exit", {}, raw_timestamp=False)
            except Exception:
                self.proc.terminate()
            try:
                self.proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait(timeout=3)
        self.proc = None

    def restart(self):
        with self.lock:
            self._stop_locked()
            self.start()

    def _stop_locked(self):
        if self.proc is None:
            return
        if self.proc.poll() is None:
            try:
                ts = self.timestamp
                self.timestamp += 1
                self.proc.stdin.write(f"[{ts}] exit\n")
                self.proc.stdin.flush()
                self._read_response(ts)
            except Exception:
                self.proc.terminate()
            try:
                self.proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait(timeout=3)
        self.proc = None

    def command(self, command_name, args, raw_timestamp=True):
        with self.lock:
            if self.proc is None or self.proc.poll() is not None:
                self.start()
            ts = self.timestamp
            self.timestamp += 1
            line = self._format_command(ts, command_name, args)
            self.proc.stdin.write(line + "\n")
            self.proc.stdin.flush()
            lines = self._read_response(ts)
            return {"timestamp": ts, "command": line, "lines": lines}

    def raw_command(self, raw):
        with self.lock:
            if self.proc is None or self.proc.poll() is not None:
                self.start()
            raw = raw.strip()
            if not raw.startswith("["):
                ts = self.timestamp
                self.timestamp += 1
                raw = f"[{ts}] {raw}"
            else:
                ts_end = raw.find("]")
                ts = int(raw[1:ts_end]) if ts_end > 1 and raw[1:ts_end].isdigit() else self.timestamp
            self.proc.stdin.write(raw + "\n")
            self.proc.stdin.flush()
            return {"timestamp": ts, "command": raw, "lines": self._read_response(ts)}

    def _format_command(self, ts, command_name, args):
        pieces = [f"[{ts}]", command_name]
        for key, value in args.items():
            if value is None or value == "":
                continue
            pieces.append(f"-{key}")
            pieces.append(str(value))
        return " ".join(pieces)

    def _read_response(self, ts):
        assert self.proc.stdout is not None
        first = self.proc.stdout.readline()
        if first == "":
            err = self.proc.stderr.read() if self.proc.stderr else ""
            raise RuntimeError(f"backend exited unexpectedly. {err}")

        prefix = f"[{ts}] "
        first = first.rstrip("\n")
        if first.startswith(prefix):
            first = first[len(prefix):]
        lines = [first]

        selector = selectors.DefaultSelector()
        selector.register(self.proc.stdout, selectors.EVENT_READ)
        while selector.select(timeout=0.04):
            extra = self.proc.stdout.readline()
            if extra == "":
                break
            lines.append(extra.rstrip("\n"))
        selector.close()
        return lines


def json_response(handler, status, payload):
    body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
    handler.send_response(status)
    handler.send_header("Content-Type", "application/json; charset=utf-8")
    handler.send_header("Content-Length", str(len(body)))
    handler.end_headers()
    handler.wfile.write(body)


class TicketRequestHandler(SimpleHTTPRequestHandler):
    backend = None
    server_ref = None

    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(FRONTEND), **kwargs)

    def log_message(self, fmt, *args):
        sys.stderr.write("[%s] %s\n" % (time.strftime("%H:%M:%S"), fmt % args))

    def end_headers(self):
        path = urlparse(self.path).path
        if path.endswith((".html", ".js", ".css")) or path == "/":
            self.send_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0")
            self.send_header("Pragma", "no-cache")
        super().end_headers()

    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path == "/api/status":
            alive = self.backend.proc is not None and self.backend.proc.poll() is None
            json_response(self, 200, {"ok": True, "backendAlive": alive})
            return
        return super().do_GET()

    def do_POST(self):
        parsed = urlparse(self.path)
        try:
            payload = self._read_json()
            if parsed.path == "/api/command":
                result = self.backend.command(payload["command"], payload.get("args", {}))
                json_response(self, 200, {"ok": True, **result})
                return
            if parsed.path == "/api/raw":
                result = self.backend.raw_command(payload["line"])
                json_response(self, 200, {"ok": True, **result})
                return
            if parsed.path == "/api/admin/restart-backend":
                self.backend.restart()
                json_response(self, 200, {"ok": True, "message": "backend restarted"})
                return
            if parsed.path == "/api/admin/shutdown":
                json_response(self, 200, {"ok": True, "message": "server shutting down"})
                threading.Thread(target=self._shutdown_server, daemon=True).start()
                return
            json_response(self, 404, {"ok": False, "error": "unknown endpoint"})
        except Exception as exc:
            json_response(self, 500, {"ok": False, "error": str(exc)})

    def _read_json(self):
        length = int(self.headers.get("Content-Length", "0"))
        raw = self.rfile.read(length).decode("utf-8") if length else "{}"
        return json.loads(raw)

    def _shutdown_server(self):
        time.sleep(0.2)
        self.backend.stop()
        self.server_ref.shutdown()


def main():
    parser = argparse.ArgumentParser(description="Ticket System Web Frontend")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--executable", default=str(ROOT / "build" / "code"))
    parser.add_argument("--data-dir", default=str(ROOT))
    parser.add_argument("--console", action="store_true", help="read local commands from this terminal too")
    args = parser.parse_args()

    executable = Path(args.executable).resolve()
    if not executable.exists():
        raise SystemExit(f"backend executable not found: {executable}")

    backend = TicketBackend(executable, Path(args.data_dir).resolve())
    TicketRequestHandler.backend = backend

    httpd = ThreadingHTTPServer((args.host, args.port), TicketRequestHandler)
    TicketRequestHandler.server_ref = httpd

    def handle_signal(signum, frame):
        backend.stop()
        httpd.shutdown()

    signal.signal(signal.SIGTERM, handle_signal)
    signal.signal(signal.SIGINT, handle_signal)

    print(f"Ticket System frontend: http://{args.host}:{args.port}", flush=True)
    if args.console or sys.stdin.isatty():
        start_console_thread(backend, httpd)
    try:
        httpd.serve_forever()
    finally:
        backend.stop()
        httpd.server_close()


def start_console_thread(backend, httpd):
    def run_console():
        print("Local console enabled. Enter Ticket System commands, /restart, or /shutdown.", flush=True)
        while True:
            try:
                line = sys.stdin.readline()
            except KeyboardInterrupt:
                line = "/shutdown\n"
            if line == "":
                break
            line = line.strip()
            if not line:
                continue
            if line == "/restart":
                try:
                    backend.restart()
                    print("[console] backend restarted", flush=True)
                except Exception as exc:
                    print(f"[console] error: {exc}", flush=True)
                continue
            if line in ("/shutdown", "/quit"):
                try:
                    backend.stop()
                finally:
                    httpd.shutdown()
                break
            try:
                result = backend.raw_command(line)
                print(f"[console] {result['command']}", flush=True)
                for output in result["lines"]:
                    print(output, flush=True)
            except Exception as exc:
                print(f"[console] error: {exc}", flush=True)

    threading.Thread(target=run_console, daemon=True).start()


if __name__ == "__main__":
    main()
