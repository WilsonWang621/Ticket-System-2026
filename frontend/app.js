const STAFF_PRIVILEGE = 2;

const state = {
  queryMode: "direct",
  lastQuery: null,
  user: null,
  privilege: 0,
};

const authGate = document.querySelector("#authGate");
const appShell = document.querySelector("#appShell");
const authTitle = document.querySelector("#authTitle");
const authMessage = document.querySelector("#authMessage");
const loginForm = document.querySelector("#loginForm");
const registerForm = document.querySelector("#registerForm");
const logEl = document.querySelector("#log");
const statusEl = document.querySelector("#status");
const sessionInfoEl = document.querySelector("#sessionInfo");
const logoutButton = document.querySelector("#logoutButton");
const ticketResultsEl = document.querySelector("#ticketResults");
const buyDialog = document.querySelector("#buyDialog");
const buyForm = document.querySelector("#buyForm");

function isStaff() {
  return state.privilege >= STAFF_PRIVILEGE;
}

function setAuthMessage(text, tone = "") {
  authMessage.textContent = text;
  authMessage.dataset.tone = tone;
}

function logBlock(title, payload) {
  const lines = payload.lines || [];
  const text = [`> ${title}`, payload.command || "", ...lines].filter(Boolean).join("\n");
  logEl.textContent = `${text}\n\n${logEl.textContent}`;
}

async function postJson(url, payload) {
  const response = await fetch(url, {
    method: "POST",
    headers: {"Content-Type": "application/json"},
    body: JSON.stringify(payload),
  });
  const data = await response.json();
  if (!data.ok) throw new Error(data.error || "request failed");
  return data;
}

function formArgs(form) {
  const args = {};
  new FormData(form).forEach((value, key) => {
    const text = String(value).trim();
    if (text !== "") args[key] = text;
  });
  return args;
}

const FORMAT_RULES = {
  date: {
    clean: (value) => value.replace(/[^\d-]/g, "").slice(0, 5),
    message: "日期格式必须是 MM-DD，例如 06-01",
    valid: isValidDate,
  },
  time: {
    clean: (value) => value.replace(/[^\d:]/g, "").slice(0, 5),
    message: "时间格式必须是 HH:MM，例如 08:00",
    valid: isValidTime,
  },
  "date-range": {
    clean: (value) => value.replace(/[^\d-|]/g, "").slice(0, 11),
    message: "售卖日期格式必须是 MM-DD|MM-DD，例如 06-01|06-30",
    valid: isValidDateRange,
  },
};

function isValidDate(value) {
  const match = value.match(/^(\d{2})-(\d{2})$/);
  if (!match) return false;
  const month = Number(match[1]);
  const day = Number(match[2]);
  const daysInMonth = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
  return month >= 1 && month <= 12 && day >= 1 && day <= daysInMonth[month - 1];
}

function isValidTime(value) {
  const match = value.match(/^(\d{2}):(\d{2})$/);
  if (!match) return false;
  const hour = Number(match[1]);
  const minute = Number(match[2]);
  return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59;
}

function dateToDayNumber(value) {
  const [month, day] = value.split("-").map(Number);
  const daysBeforeMonth = [0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334];
  return daysBeforeMonth[month - 1] + day;
}

function isValidDateRange(value) {
  const parts = value.split("|");
  if (parts.length !== 2 || !parts.every(isValidDate)) return false;
  return dateToDayNumber(parts[0]) <= dateToDayNumber(parts[1]);
}

function validateFormattedInput(input) {
  const rule = FORMAT_RULES[input.dataset.format];
  if (!rule) return true;
  const value = input.value.trim();
  const valid = value === "" ? !input.required : rule.valid(value);
  input.setCustomValidity(valid ? "" : rule.message);
  return valid;
}

function validateForm(form) {
  form.querySelectorAll("[data-format]").forEach(validateFormattedInput);
  return form.reportValidity();
}

document.querySelectorAll("[data-format]").forEach((input) => {
  input.addEventListener("input", () => {
    const rule = FORMAT_RULES[input.dataset.format];
    const selectionStart = input.selectionStart;
    const before = input.value;
    input.value = rule.clean(input.value);
    if (selectionStart !== null && before !== input.value) {
      const nextPos = Math.min(selectionStart, input.value.length);
      input.setSelectionRange(nextPos, nextPos);
    }
    validateFormattedInput(input);
  });

  input.addEventListener("blur", () => validateFormattedInput(input));
});

async function runCommand(command, args, title = command) {
  if (!state.user && command !== "login" && command !== "add_user") {
    throw new Error("请先登录");
  }
  if (["add_train", "release_train", "delete_train", "clean"].includes(command) && !isStaff()) {
    throw new Error("当前账户权限不足，无法执行管理操作");
  }
  const data = await postJson("/api/command", {command, args});
  logBlock(title, data);
  return data;
}

function parseProfile(line) {
  const parts = line.trim().split(/\s+/);
  return {
    username: parts[0] || "",
    name: parts[1] || "",
    mail: parts[2] || "",
    privilege: Number(parts[3] || "0"),
  };
}

async function refreshCurrentProfile(username) {
  const data = await runCommand("query_profile", {c: username, u: username}, "query_profile");
  if (!data.lines.length || data.lines[0] === "-1") {
    throw new Error("无法确认当前用户权限");
  }
  const profile = parseProfile(data.lines[0]);
  state.user = profile.username;
  state.privilege = profile.privilege;
  return profile;
}

function enterApp(profile) {
  authGate.hidden = true;
  appShell.hidden = false;
  logoutButton.hidden = false;
  sessionInfoEl.textContent = `${profile.username} · 权限 ${profile.privilege}`;
  fillUserFields(profile.username);
  applyPrivilege();
}

function leaveApp() {
  state.user = null;
  state.privilege = 0;
  appShell.hidden = true;
  authGate.hidden = false;
  logoutButton.hidden = true;
  sessionInfoEl.textContent = "未登录";
  applyPrivilege();
}

function applyPrivilege() {
  document.querySelectorAll(".staff-only").forEach((element) => {
    element.hidden = !isStaff();
  });
  const trainsTab = document.querySelector('[data-tab="trains"]');
  const opsTab = document.querySelector('[data-tab="ops"]');
  if (trainsTab) {
    const label = trainsTab.querySelector("span") || trainsTab;
    label.textContent = isStaff() ? "车次管理" : "查询车次";
  }
  if (opsTab) opsTab.hidden = !isStaff();
  if (!isStaff() && document.querySelector("#ops.active")) {
    activateTab("tickets");
  }
}

function fillUserFields(username) {
  document.querySelectorAll("[data-user-field]").forEach((input) => {
    input.value = username;
  });
}

function activateTab(tabName) {
  document.querySelectorAll("[data-tab]").forEach((item) => item.classList.remove("active"));
  document.querySelectorAll(".panel").forEach((panel) => panel.classList.remove("active"));
  const button = document.querySelector(`[data-tab="${tabName}"]`);
  const panel = document.querySelector(`#${tabName}`);
  if (button) button.classList.add("active");
  if (panel) panel.classList.add("active");
}

function switchAuthMode(mode) {
  const isRegister = mode === "register";
  authTitle.textContent = isRegister ? "创建首个管理员" : "登录";
  loginForm.hidden = isRegister;
  registerForm.hidden = !isRegister;
  document.querySelectorAll("[data-auth-mode]").forEach((button) => {
    button.classList.toggle("active", button.dataset.authMode === mode);
  });
  setAuthMessage(isRegister
    ? "只有在系统尚无用户时，这里才能创建首个管理员。已有用户时，请先用管理员账号登录，再由管理员添加新用户。"
    : "");
}

async function login(username, password) {
  const loginResult = await runCommand("login", {u: username, p: password}, "login");
  if (loginResult.lines[0] !== "0") {
    throw new Error("登录失败。用户不存在、密码错误或该用户已经登录。");
  }
  state.user = username;
  const profile = await refreshCurrentProfile(username);
  enterApp(profile);
}

loginForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  const args = formArgs(loginForm);
  try {
    await login(args.u, args.p);
    setAuthMessage("");
  } catch (error) {
    setAuthMessage(`${error.message} 可以切换到创建账户。`, "error");
    switchAuthMode("register");
    registerForm.elements.u.value = args.u || "";
  }
});

registerForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  const args = formArgs(registerForm);
  try {
    const result = await runCommand("add_user", args, "add_user");
    if (result.lines[0] !== "0") {
      throw new Error("创建失败。若系统里已经有用户，你必须先用管理员账号登录，再由管理员添加新用户。");
    }
    await login(args.u, args.p);
  } catch (error) {
    setAuthMessage(error.message, "error");
  }
});

logoutButton.addEventListener("click", async () => {
  if (!state.user) return;
  try {
    await runCommand("logout", {u: state.user}, "logout");
  } catch (error) {
    logBlock("错误", {lines: [error.message]});
  } finally {
    leaveApp();
  }
});

document.querySelectorAll("[data-auth-mode]").forEach((button) => {
  button.addEventListener("click", () => switchAuthMode(button.dataset.authMode));
});

function parseTicketLine(line) {
  const match = line.match(/^(\S+)\s+(.+?)\s+(\d{2}-\d{2})\s+(\d{2}:\d{2})\s+->\s+(.+?)\s+(\d{2}-\d{2})\s+(\d{2}:\d{2})\s+(\d+)\s+(\d+)$/);
  if (!match) return null;
  return {
    train: match[1],
    from: match[2],
    leaveDate: match[3],
    leaveTime: match[4],
    to: match[5],
    arriveDate: match[6],
    arriveTime: match[7],
    price: match[8],
    seat: match[9],
  };
}

function renderTickets(data) {
  ticketResultsEl.innerHTML = "";
  if (state.queryMode === "transfer") {
    if (data.lines.length === 1 && data.lines[0] === "0") {
      ticketResultsEl.textContent = "未找到换乘方案";
      return;
    }
    data.lines.forEach((line) => addTicketCard(line, false));
    return;
  }
  const count = Number(data.lines[0] || "0");
  if (!count) {
    ticketResultsEl.textContent = "未找到直达车票";
    return;
  }
  data.lines.slice(1).forEach((line) => addTicketCard(line, true));
}

function addTicketCard(line, canBuy) {
  const ticket = parseTicketLine(line);
  const card = document.createElement("div");
  card.className = "ticket-card";
  const content = document.createElement("div");
  content.innerHTML = ticket
    ? `<div class="ticket-main">
         <span class="train-badge">${ticket.train}</span>
         <span>${ticket.from}</span>
         <span class="route-arrow">→</span>
         <span>${ticket.to}</span>
       </div>
       <div class="ticket-meta">
         <span>${ticket.leaveDate} ${ticket.leaveTime} 发车</span>
         <span>${ticket.arriveDate} ${ticket.arriveTime} 到达</span>
         <span class="ticket-price">票价 ${ticket.price}</span>
         <span class="ticket-seat">余票 ${ticket.seat}</span>
       </div>`
    : `<div class="ticket-main">${line}</div>`;
  card.appendChild(content);
  if (canBuy && ticket) {
    const button = document.createElement("button");
    button.textContent = "购票";
    button.addEventListener("click", () => openBuyDialog(ticket));
    card.appendChild(button);
  }
  ticketResultsEl.appendChild(card);
}

function openBuyDialog(ticket) {
  buyForm.elements.i.value = ticket.train;
  buyForm.elements.f.value = ticket.from;
  buyForm.elements.t.value = ticket.to;
  buyForm.elements.d.value = state.lastQuery?.d || ticket.leaveDate;
  buyForm.elements.u.value = state.user || "";
  buyDialog.showModal();
}

document.querySelectorAll("[data-tab]").forEach((button) => {
  button.addEventListener("click", () => activateTab(button.dataset.tab));
});

document.querySelectorAll("[data-query-mode]").forEach((button) => {
  button.addEventListener("click", () => {
    document.querySelectorAll("[data-query-mode]").forEach((item) => item.classList.remove("active"));
    button.classList.add("active");
    state.queryMode = button.dataset.queryMode;
  });
});

document.querySelectorAll("form[data-command]").forEach((form) => {
  form.addEventListener("submit", async (event) => {
    event.preventDefault();
    if (!validateForm(form)) return;
    const args = formArgs(form);
    let command = form.dataset.command;
    if (form.dataset.kind === "ticket-query") {
      command = state.queryMode === "transfer" ? "query_transfer" : "query_ticket";
      state.lastQuery = args;
    }
    try {
      const data = await runCommand(command, args);
      if (form.dataset.kind === "ticket-query") renderTickets(data);
    } catch (error) {
      logBlock("错误", {lines: [error.message]});
    }
  });
});

buyForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  const submitter = event.submitter;
  if (submitter && submitter.value !== "buy") {
    buyDialog.close();
    return;
  }
  const args = formArgs(buyForm);
  args.u = state.user;
  args.q = buyForm.elements.q.checked ? "true" : "false";
  try {
    await runCommand("buy_ticket", args, "buy_ticket");
    buyDialog.close();
  } catch (error) {
    logBlock("错误", {lines: [error.message]});
  }
});

document.querySelectorAll("[data-system-command]").forEach((button) => {
  button.addEventListener("click", async () => {
    if (!isStaff()) {
      logBlock("错误", {lines: ["当前账户权限不足，无法执行系统维护操作"]});
      return;
    }
    if (button.dataset.systemCommand === "clean" && !confirm("确认清空所有数据？")) return;
    try {
      const result = await runCommand(button.dataset.systemCommand, {});
      if (button.dataset.systemCommand === "clean" && result.lines[0] === "0") {
        leaveApp();
        switchAuthMode("login");
        setAuthMessage("数据已清空，请先创建首个账号，或用管理员账号重新登录。", "");
      }
    } catch (error) {
      logBlock("错误", {lines: [error.message]});
    }
  });
});

document.querySelectorAll("[data-admin-action]").forEach((button) => {
  button.addEventListener("click", async () => {
    if (!isStaff()) {
      logBlock("错误", {lines: ["当前账户权限不足，无法执行系统维护操作"]});
      return;
    }
    try {
      const action = button.dataset.adminAction;
      const data = await postJson(`/api/admin/${action}`, {});
      logBlock(action, {lines: [data.message]});
    } catch (error) {
      logBlock("错误", {lines: [error.message]});
    }
  });
});

document.querySelector("#clearLog").addEventListener("click", () => {
  logEl.textContent = "";
});

async function refreshStatus() {
  try {
    const response = await fetch("/api/status");
    const data = await response.json();
    statusEl.textContent = data.backendAlive ? "后端数据服务运行中" : "后端未运行";
    statusEl.dataset.state = data.backendAlive ? "ok" : "down";
  } catch {
    statusEl.textContent = "Web 服务未响应";
    statusEl.dataset.state = "down";
  }
}

leaveApp();
switchAuthMode("login");
refreshStatus();
setInterval(refreshStatus, 5000);
