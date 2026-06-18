# Ticket System GUI 前端安装手册

## 运行环境

- Linux / macOS / Windows with WSL
- CMake 3.16+
- 支持 C++17 的编译器
- Python 3.8+

前端服务只使用 Python 标准库，不需要安装 npm、Vue、React 或其他第三方依赖。

## 构建后端

在项目根目录执行：

```bash
cmake -S . -B build
cmake --build build
```

构建完成后后端可执行文件位于：

```text
build/code
```

## 启动 Web 前端

在项目根目录执行：

```bash
python3 frontend/server.py --host 127.0.0.1 --port 8080 --data-dir .
```

浏览器访问：

```text
http://127.0.0.1:8080
```

启动后，如果是在交互式终端中运行，当前终端也可以继续输入作业原生命令。终端输入和网页操作会连接到同一个后端数据进程，并按顺序执行。例如：

```text
query_ticket -s A -t B -d 06-01 -p time
login -u root -p pass
```

本地控制台还支持两个管理命令：

```text
/restart
/shutdown
```

如果你的运行环境没有自动启用本地控制台，可以显式添加 `--console`：

```bash
python3 frontend/server.py --host 127.0.0.1 --port 8080 --data-dir . --console
```

如需允许局域网其他电脑访问，把 host 改为 `0.0.0.0`：

```bash
python3 frontend/server.py --host 0.0.0.0 --port 8080 --data-dir .
```

多台电脑或多个浏览器页面可以同时连接到同一个 Web 服务。服务端会将命令串行发送给同一个后端数据进程，避免并发写入打乱命令输出。

## 数据目录

默认数据目录是项目根目录。可以用 `--data-dir` 指定独立数据目录：

```bash
python3 frontend/server.py --port 8080 --data-dir /tmp/ticket-system-data
```

后端生成的 `.dat` 和 B+ 树索引文件都会保存在该目录中。

## 功能覆盖

图形界面提供以下操作：

- 入口：先登录；登录失败时可切换到创建账户；登录后自动查询当前用户权限
- 用户：添加用户、登录、登出、查询资料、修改资料
- 车次：查询车次；管理权限用户可添加车次、删除车次、发布车次
- 车票：直达查询、换乘查询、查询后直接购票
- 订单：查询订单、退票
- 系统：管理权限用户可清空数据、重启后端数据服务、优雅关闭 Web 与后端

权限判断由登录后的 `query_profile -c <user> -u <user>` 结果决定。前端默认把权限值不低于 2 的用户视为管理人员；首个账号仍遵循主体逻辑自动获得权限 10。

## 优雅关闭与重启

页面 `系统维护` 中提供：

- `重启后端数据服务`：向当前后端发送 `exit`，等待其保存并退出后重新启动；
- `优雅关闭 Web 与后端`：先向后端发送 `exit`，再关闭 Web 服务；
- `清空数据`：执行作业原生命令 `clean`。

也可以在终端向 Web 服务发送 `SIGTERM`，服务会先关闭后端数据进程再退出：

```bash
kill -TERM <server-pid>
```

不建议手动 kill `build/code` 子进程。

## 常见问题

### 后端可执行文件不存在

先执行：

```bash
cmake --build build
```

或用 `--executable` 指定可执行文件：

```bash
python3 frontend/server.py --executable ./build/code
```

### 端口被占用

改用其他端口：

```bash
python3 frontend/server.py --port 8090
```

### 希望从干净数据开始

进入页面 `系统维护`，点击 `清空数据`。
