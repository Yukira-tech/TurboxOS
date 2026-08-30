/* ============================================================
 * @file terminal.js
 * @brief Turbox 网页终端交互逻辑
 *
 * 层级：
 *   TurboBoxOS/web/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/web/terminal.js
 *
 * 模块作用：
 *   在浏览器中模拟 Turbox 终端，支持 create/write/run/ls/cat/help。
 *   双模式运行：
 *     1. 镜像同步模式，fetch 到 fs_state.json 时同步真实文件系统状态；
 *     2. 内置模拟模式，fetch 失败时回退到浏览器内存的虚拟文件系统。
 *   两种模式共用同一套命令实现，只有初始文件来源不同。
 *
 * 使用者：
 *   web/index.html 通过 <script> 引入本文件。
 *
 * 项目角色：
 *   web 层交互逻辑，把终端命令转成虚拟文件系统操作并显示结果。
 *
 * 引入说明：
 *   不依赖任何第三方库，纯浏览器 API。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

(function () {
    "use strict";

    /* ---------- DOM 引用 ---------- */
    var outputEl = document.getElementById("output");
    var inputEl = document.getElementById("cmdline");
    var screenEl = document.getElementById("screen");
    var modeEl = document.getElementById("mode-label");

    // 提示符与 SPEC 4.8 约定保持一致
    var PROMPT = "turbox> ";

    // 虚拟文件系统：name -> content，与内核 tfs 的内存文件系统同构
    var files = {};

    // 当前模式："sync" 镜像同步 或 "local" 内置模拟
    var mode = "local";

    /*
     * 向终端输出一行文本，可选 CSS 类控制颜色。
     * 输出后自动滚动到底部，确保用户看到最新内容。
     */
    function printLine(text, cls) {
        var div = document.createElement("div");
        div.className = cls ? "line " + cls : "line";
        div.textContent = text;
        outputEl.appendChild(div);
        screenEl.scrollTop = screenEl.scrollHeight;
    }

    /*
     * 切换运行模式并更新底部状态栏显示。
     */
    function setMode(m) {
        mode = m;
        if (m === "sync") {
            modeEl.textContent = "模式：镜像同步（fs_state.json 已加载）";
            modeEl.className = "mode-sync";
        } else {
            modeEl.textContent = "模式：内置模拟模式（未检测到 bridge 输出）";
            modeEl.className = "mode-local";
        }
    }

    /*
     * 模拟内核 tfs_run，解释执行文件内的 mini 命令。
     * 与 fs/turboxfs.cpp 的语义对应：
     *   "print xxx" 输出内容，空行和 # 注释跳过，其他行报未知命令。
     */
    function runScript(name) {
        var lines = files[name].split("\n");
        for (var i = 0; i < lines.length; i++) {
            var line = lines[i];
            if (line.indexOf("print ") === 0) {
                printLine(line.substring(6));
            } else if (line.trim() === "" || line.indexOf("#") === 0) {
                // 空行和注释静默跳过，与内核解释器一致
            } else {
                printLine("run: 第 " + (i + 1) + " 行未知命令: " + line, "line-err");
            }
        }
    }

    /*
     * 解析并执行一行终端命令。
     * 先回显命令，再按命令名分发，可能修改虚拟文件系统。
     */
    function execCommand(raw) {
        printLine(PROMPT + raw, "line-cmd");  // 回显，模拟真实终端

        var parts = raw.trim().split(/\s+/).filter(function (s) { return s.length > 0; });
        if (parts.length === 0) return;

        var cmd = parts[0];

        if (cmd === "help") {
            printLine("Turbox 终端命令：");
            printLine("  create <file>        创建文件");
            printLine("  write <file> <text>  写入文本（追加到文件末尾）");
            printLine("  run <file>           运行文件（mini 命令集：print/注释）");
            printLine("  ls                   列出文件");
            printLine("  cat <file>           查看文件内容");
            printLine("  help                 显示本帮助");
        } else if (cmd === "create") {
            if (!parts[1]) { printLine("用法: create <file>", "line-err"); return; }
            if (files.hasOwnProperty(parts[1])) {
                printLine("create: 文件已存在: " + parts[1], "line-err");
            } else {
                files[parts[1]] = "";
                printLine("已创建文件: " + parts[1]);
            }
        } else if (cmd === "write") {
            if (!parts[1] || parts.length < 3) { printLine("用法: write <file> <text>", "line-err"); return; }
            if (!files.hasOwnProperty(parts[1])) {
                // 与内核语义一致：write 前必须先 create
                printLine("write: 文件不存在（请先 create）: " + parts[1], "line-err");
                return;
            }
            var text = parts.slice(2).join(" ");
            files[parts[1]] += text + "\n";
            printLine("已写入 " + text.length + " 字节到 " + parts[1]);
        } else if (cmd === "run") {
            if (!parts[1]) { printLine("用法: run <file>", "line-err"); return; }
            if (!files.hasOwnProperty(parts[1])) {
                printLine("run: 文件不存在: " + parts[1], "line-err");
                return;
            }
            runScript(parts[1]);
        } else if (cmd === "ls") {
            var names = Object.keys(files);
            if (names.length === 0) {
                printLine("（文件系统为空）", "line-dim");
            } else {
                names.sort();
                for (var i = 0; i < names.length; i++) {
                    printLine("  " + names[i] + "  (" + files[names[i]].length + " 字节)");
                }
            }
        } else if (cmd === "cat") {
            if (!parts[1]) { printLine("用法: cat <file>", "line-err"); return; }
            if (!files.hasOwnProperty(parts[1])) {
                printLine("cat: 文件不存在: " + parts[1], "line-err");
                return;
            }
            var content = files[parts[1]];
            printLine(content === "" ? "（空文件）" : content.replace(/\n$/, ""), content === "" ? "line-dim" : undefined);
        } else {
            printLine("未知命令: " + cmd + "（输入 help 查看命令列表）", "line-err");
        }
    }

    /*
     * 启动时尝试从 bridge.py 同步文件系统状态。
     * 成功后进入镜像同步模式，失败则回退到内置模拟模式。
     */
    function trySyncFromBridge() {
        return fetch("fs_state.json", { cache: "no-store" })
            .then(function (resp) {
                if (!resp.ok) throw new Error("HTTP " + resp.status);
                return resp.json();
            })
            .then(function (obj) {
                var list = (obj && obj.files) || [];
                for (var i = 0; i < list.length; i++) {
                    files[list[i].name] = list[i].content;
                }
                setMode("sync");
                printLine("[bridge] 已从 fs_state.json 同步 " + list.length + " 个文件", "line-dim");
            })
            .catch(function () {
                // fetch 失败不代表终端不可用，回退到内存模拟
                setMode("local");
                printLine("[bridge] 未检测到 fs_state.json，进入内置模拟模式", "line-dim");
            });
    }

    /* ---------- 输入处理：回车执行，历史命令用 ↑/↓ 翻阅 ---------- */
    var history = [];
    var historyIdx = -1;

    inputEl.addEventListener("keydown", function (ev) {
        if (ev.key === "Enter") {
            var line = inputEl.value;
            inputEl.value = "";
            if (line.trim() !== "") {
                history.push(line);
                historyIdx = history.length;
            }
            execCommand(line);
        } else if (ev.key === "ArrowUp") {
            if (historyIdx > 0) {
                historyIdx--;
                inputEl.value = history[historyIdx];
            }
            ev.preventDefault();
        } else if (ev.key === "ArrowDown") {
            if (historyIdx < history.length - 1) {
                historyIdx++;
                inputEl.value = history[historyIdx];
            } else {
                historyIdx = history.length;
                inputEl.value = "";
            }
            ev.preventDefault();
        }
    });

    // 点击终端任意处聚焦输入框，贴合真实终端交互
    screenEl.addEventListener("click", function () { inputEl.focus(); });

    /* ---------- 启动流程 ---------- */
    printLine("Turbox 微内核 Web 终端", "line-dim");
    printLine("输入 help 查看命令列表", "line-dim");
    trySyncFromBridge().then(function () {
        // 模式确定后提示一次，方便用户确认数据来源
        printLine(mode === "sync" ? "就绪（镜像同步模式）" : "就绪（模拟模式）", "line-dim");
    });
})();