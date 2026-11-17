const vscode = require("vscode");
const { spawn } = require("child_process");

// `indk rapikan <berkas>` writes to stdout without changing the source file.
function rapikanLewatIndk(spawnSync, bin, berkas) {
  const hasil = spawnSync(bin, ["rapikan", berkas], { encoding: "utf8" });
  if (hasil.error) throw new Error(`gagal menjalankan '${bin}': ${hasil.error.message}`);
  if (hasil.status !== 0) throw new Error(hasil.stderr.trim() || `indk rapikan keluar ${hasil.status}`);
  return hasil.stdout;
}

class StdioLanguageClient {
  constructor(command, args, env, onDiagnostics, onError) {
    this.child = spawn(command, args, { stdio: ["pipe", "pipe", "pipe"], env, windowsHide: true });
    this.onDiagnostics = onDiagnostics;
    this.onError = onError;
    this.buffer = Buffer.alloc(0);
    this.nextId = 1;
    this.pending = new Map();
    this.stopped = false;

    this.child.stdout.on("data", chunk => this.read(chunk));
    this.child.stderr.on("data", chunk => {
      const text = chunk.toString("utf8").trim();
      if (text) console.error(`[indk-lsp] ${text}`);
    });
    this.child.on("error", error => this.fail(error));
    this.child.on("exit", (code, signal) => {
      if (!this.stopped && code !== 0) this.fail(new Error(`indk-lsp berhenti (${code || signal})`));
      this.pending.forEach(waiter => waiter.reject(new Error("indk-lsp berhenti")));
      this.pending.clear();
    });
  }

  read(chunk) {
    this.buffer = Buffer.concat([this.buffer, chunk]);
    for (;;) {
      const headerEnd = this.buffer.indexOf("\r\n\r\n");
      if (headerEnd < 0) return;
      const header = this.buffer.subarray(0, headerEnd).toString("ascii");
      const match = header.match(/(?:^|\r\n)Content-Length:\s*(\d+)/i);
      if (!match) { this.fail(new Error("header LSP tidak memiliki Content-Length")); return; }
      const length = Number(match[1]);
      const bodyStart = headerEnd + 4;
      if (!Number.isSafeInteger(length) || length < 0 || length > 2 * 1024 * 1024) {
        this.fail(new Error("ukuran pesan LSP tidak valid")); return;
      }
      if (this.buffer.length < bodyStart + length) return;
      const body = this.buffer.subarray(bodyStart, bodyStart + length).toString("utf8");
      this.buffer = this.buffer.subarray(bodyStart + length);
      let message;
      try { message = JSON.parse(body); }
      catch (error) { this.fail(error); return; }
      this.receive(message);
    }
  }

  receive(message) {
    if (message.method === "textDocument/publishDiagnostics") {
      this.onDiagnostics(message.params);
      return;
    }
    if (!Object.prototype.hasOwnProperty.call(message, "id")) return;
    const waiter = this.pending.get(message.id);
    if (!waiter) return;
    this.pending.delete(message.id);
    clearTimeout(waiter.timer);
    if (message.error) waiter.reject(new Error(message.error.message || "LSP error"));
    else waiter.resolve(message.result);
  }

  send(message) {
    if (this.child.exitCode !== null || this.child.killed) return;
    const body = Buffer.from(JSON.stringify(message), "utf8");
    this.child.stdin.write(`Content-Length: ${body.length}\r\n\r\n`);
    this.child.stdin.write(body);
  }

  request(method, params) {
    const id = this.nextId++;
    this.send({ jsonrpc: "2.0", id, method, params });
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        this.pending.delete(id);
        reject(new Error(`LSP timeout: ${method}`));
      }, 5000);
      this.pending.set(id, { resolve, reject, timer });
    });
  }

  notify(method, params) {
    this.send({ jsonrpc: "2.0", method, params });
  }

  async start(rootUri) {
    await this.request("initialize", {
      processId: process.pid,
      rootUri,
      capabilities: {
        textDocument: {
          completion: { completionItem: { snippetSupport: true } },
          hover: { contentFormat: ["markdown", "plaintext"] },
          definition: {}
        }
      }
    });
    this.notify("initialized", {});
  }

  fail(error) {
    if (this.stopped) return;
    this.onError(error);
  }

  async stop() {
    if (this.stopped) return;
    this.stopped = true;
    try { await this.request("shutdown", null); } catch (_) { /* process may already exit */ }
    this.notify("exit", {});
    setTimeout(() => { if (this.child.exitCode === null) this.child.kill(); }, 300);
  }
}

function toVscodeRange(vscodeApi, range) {
  return new vscodeApi.Range(
    range.start.line, range.start.character,
    range.end.line, range.end.character
  );
}

async function activate(context) {
  const { spawnSync } = require("child_process");

  // ---- Panel Activity Bar: tombol Jalankan/Periksa/Rapikan/Profil/Awakutu ----
  const AKSI = [
    { label: "Jalankan", command: "indonesiaku.jalankan", icon: "play" },
    { label: "Periksa (Lint)", command: "indonesiaku.periksa", icon: "search" },
    { label: "Rapikan (Format)", command: "indonesiaku.rapikan", icon: "sparkle" },
    { label: "Profil", command: "indonesiaku.profil", icon: "graph-line" },
    { label: "Awakutu (Debugger CLI)", command: "indonesiaku.awakutu", icon: "debug" },
  ];
  class AksiProvider {
    getTreeItem(item) { return item; }
    getChildren() {
      return AKSI.map(a => {
        const item = new vscode.TreeItem(a.label, vscode.TreeItemCollapsibleState.None);
        item.command = { command: a.command, title: a.label };
        item.iconPath = new vscode.ThemeIcon(a.icon);
        return item;
      });
    }
  }
  context.subscriptions.push(
    vscode.window.registerTreeDataProvider("indonesiaku.aksi", new AksiProvider())
  );

  // Satu terminal dipakai ulang (ponytail: satu terminal bersama, bukan satu
  // per aksi -- cukup untuk pemakaian CLI dev biasa).
  let terminalIndk;
  function terminalAktif() {
    if (!terminalIndk || terminalIndk.exitStatus !== undefined) {
      terminalIndk = vscode.window.createTerminal("Indonesiaku");
    }
    return terminalIndk;
  }
  function berkasAktif() {
    const editor = vscode.window.activeTextEditor;
    if (!editor || editor.document.languageId !== "indonesiaku") {
      vscode.window.showWarningMessage("Indonesiaku: buka berkas .idk dulu.");
      return null;
    }
    return editor.document.fileName;
  }
  function daftarAksiTerminal(command, subperintah) {
    context.subscriptions.push(vscode.commands.registerCommand(command, () => {
      const berkas = berkasAktif();
      if (!berkas) return;
      const bin = vscode.workspace.getConfiguration("indonesiaku").get("jalurIndk") || "indk";
      const t = terminalAktif();
      t.show();
      const argBerkas = JSON.stringify(berkas);
      t.sendText(subperintah ? `${bin} ${subperintah} ${argBerkas}` : `${bin} ${argBerkas}`);
    }));
  }
  daftarAksiTerminal("indonesiaku.jalankan", null);
  daftarAksiTerminal("indonesiaku.periksa", "periksa");
  daftarAksiTerminal("indonesiaku.profil", "profil");
  daftarAksiTerminal("indonesiaku.awakutu", "awakutu");
  // Rapikan lewat command bawaan supaya hasilnya tertulis balik ke editor
  // (jalur yang sama dengan formatter di bawah), bukan dicetak ke terminal.
  context.subscriptions.push(vscode.commands.registerCommand("indonesiaku.rapikan", () => {
    if (!berkasAktif()) return;
    vscode.commands.executeCommand("editor.action.formatDocument");
  }));

  // Tawarkan sekali saja (globalState) supaya tidak mengganti File Icon
  // Theme pengguna diam-diam -- ini pilihan global yang memengaruhi SEMUA
  // jenis berkas, bukan cuma .idk.
  const sudahDitawarkan = context.globalState.get("indonesiaku.tawarIkonDitampilkan", false);
  const temaSekarang = vscode.workspace.getConfiguration().get("workbench.iconTheme");
  if (!sudahDitawarkan && temaSekarang !== "indonesiaku-icons") {
    context.globalState.update("indonesiaku.tawarIkonDitampilkan", true);
    vscode.window.showInformationMessage(
      "Pakai ikon khusus untuk berkas .idk di Explorer?",
      "Aktifkan", "Nanti saja"
    ).then(pilihan => {
      if (pilihan === "Aktifkan") {
        vscode.workspace.getConfiguration().update(
          "workbench.iconTheme", "indonesiaku-icons", vscode.ConfigurationTarget.Global
        );
      }
    });
  }

  const formatter = vscode.languages.registerDocumentFormattingEditProvider("indonesiaku", {
    provideDocumentFormattingEdits(document) {
      if (document.isUntitled || document.isDirty) return [];
      const bin = vscode.workspace.getConfiguration("indonesiaku").get("jalurIndk") || "indk";
      try {
        const formatted = rapikanLewatIndk(spawnSync, bin, document.fileName);
        if (formatted === document.getText()) return [];
        return [vscode.TextEdit.replace(
          new vscode.Range(document.positionAt(0), document.positionAt(document.getText().length)),
          formatted
        )];
      } catch (error) {
        vscode.window.showErrorMessage(`Indonesiaku: ${error.message}`);
        return [];
      }
    }
  });
  context.subscriptions.push(formatter);

  const diagnostics = vscode.languages.createDiagnosticCollection("indonesiaku");
  context.subscriptions.push(diagnostics);
  const serverPath = context.asAbsolutePath("server.js");
  const bin = vscode.workspace.getConfiguration("indonesiaku").get("jalurIndk") || "indk";
  const rootUri = vscode.workspace.workspaceFolders?.[0]?.uri.toString() || null;
  // VS Code's process.execPath is Electron; opt into its Node mode when
  // spawning the bundled JS server. Outside Electron (tests/other hosts),
  // process.execPath is ordinary Node and needs no special flag.
  const serverArgs = process.versions.electron
    ? ["--ms-enable-electron-run-as-node", serverPath]
    : [serverPath];
  const client = new StdioLanguageClient(
    process.execPath,
    serverArgs,
    { ...process.env, INDONESIAKU_INDK: bin },
    params => {
      const uri = vscode.Uri.parse(params.uri);
      const items = (params.diagnostics || []).map(item => {
        const diagnostic = new vscode.Diagnostic(
          toVscodeRange(vscode, item.range),
          item.message,
          item.severity === 2 ? vscode.DiagnosticSeverity.Warning : vscode.DiagnosticSeverity.Error
        );
        diagnostic.source = item.source || "indk";
        return diagnostic;
      });
      diagnostics.set(uri, items);
    },
    error => vscode.window.showErrorMessage(`Indonesiaku LSP: ${error.message}`)
  );
  languageClient = client;
  context.subscriptions.push({ dispose: () => { client.stop(); } });

  try {
    await client.start(rootUri);
  } catch (error) {
    client.stop();
    vscode.window.showErrorMessage(`Indonesiaku LSP gagal dimulai: ${error.message}`);
    return;
  }

  const syncOpen = document => {
    if (document.languageId !== "indonesiaku") return;
    client.notify("textDocument/didOpen", { textDocument: {
      uri: document.uri.toString(), languageId: document.languageId,
      version: document.version, text: document.getText()
    }});
  };
  vscode.workspace.textDocuments.forEach(syncOpen);
  context.subscriptions.push(vscode.workspace.onDidOpenTextDocument(syncOpen));
  context.subscriptions.push(vscode.workspace.onDidChangeTextDocument(event => {
    const document = event.document;
    if (document.languageId !== "indonesiaku") return;
    client.notify("textDocument/didChange", {
      textDocument: { uri: document.uri.toString(), version: document.version },
      contentChanges: [{ text: document.getText() }]
    });
  }));
  context.subscriptions.push(vscode.workspace.onDidCloseTextDocument(document => {
    if (document.languageId !== "indonesiaku") return;
    client.notify("textDocument/didClose", { textDocument: { uri: document.uri.toString() } });
    diagnostics.delete(document.uri);
  }));

  const selector = { language: "indonesiaku" };
  context.subscriptions.push(vscode.languages.registerCompletionItemProvider(selector, {
    async provideCompletionItems(document, position) {
      try {
        const result = await client.request("textDocument/completion", {
          textDocument: { uri: document.uri.toString() },
          position: { line: position.line, character: position.character }
        });
        return (result.items || []).map(item => {
          const kind = item.kind === 14 ? vscode.CompletionItemKind.Keyword :
            item.kind === 2 ? vscode.CompletionItemKind.Method : vscode.CompletionItemKind.Function;
          const completion = new vscode.CompletionItem(item.label, kind);
          completion.detail = item.detail;
          if (item.insertText && item.insertText.includes("$0")) {
            completion.insertText = new vscode.SnippetString(item.insertText);
            completion.insertTextRules = vscode.CompletionItemInsertTextRule.InsertAsSnippet;
          } else {
            completion.insertText = item.insertText || item.label;
          }
          return completion;
        });
      } catch (_) { return []; }
    }
  }, "."));

  context.subscriptions.push(vscode.languages.registerHoverProvider(selector, {
    async provideHover(document, position) {
      try {
        const result = await client.request("textDocument/hover", {
          textDocument: { uri: document.uri.toString() },
          position: { line: position.line, character: position.character }
        });
        if (!result || !result.contents) return null;
        const contents = result.contents.kind === "markdown"
          ? new vscode.MarkdownString(result.contents.value)
          : result.contents;
        return new vscode.Hover(contents);
      } catch (_) { return null; }
    }
  }));

  context.subscriptions.push(vscode.languages.registerDefinitionProvider(selector, {
    async provideDefinition(document, position) {
      try {
        const result = await client.request("textDocument/definition", {
          textDocument: { uri: document.uri.toString() },
          position: { line: position.line, character: position.character }
        });
        if (!result) return null;
        return new vscode.Location(
          vscode.Uri.parse(result.uri), toVscodeRange(vscode, result.range)
        );
      } catch (_) { return null; }
    }
  }));
}

function deactivate() {
  if (languageClient) languageClient.stop();
}

let languageClient;

module.exports = { activate, deactivate, rapikanLewatIndk, StdioLanguageClient };
