// Integration test for the Debug Adapter Protocol bridge to `indk awakutu`.
// Drives `node debug-adapter.js` over stdio exactly as VS Code would.
// Uses only Node's built-in modules.
"use strict";

const assert = require("assert");
const { spawn } = require("child_process");
const fs = require("fs");
const os = require("os");
const path = require("path");

const root = path.resolve(__dirname, "../..");
const indk = path.resolve(process.argv[2] || path.join(root, process.platform === "win32" ? "indk.exe" : "indk"));
const adapterPath = path.join(__dirname, "debug-adapter.js");
const temp = fs.mkdtempSync(path.join(os.tmpdir(), "indonesiaku-dap-test-"));
const TIMEOUT_MS = 15000;

const PROGRAM = [
  "total = 0",                       // 1
  "fungsi tambah(a, b) {",           // 2
  "    variabel hasil = a + b",      // 3
  "    kembalikan hasil",            // 4
  "}",                               // 5
  "untuk i dalam rentang(3) {",      // 6
  "    total = tambah(total, i)",    // 7
  "}",                               // 8
  "cetak(\"total\", total)",         // 9
  ""
].join("\n");

class KlienDap {
  constructor() {
    this.child = spawn(process.execPath, [adapterPath], { stdio: ["pipe", "pipe", "pipe"] });
    this.seq = 1;
    this.buffer = Buffer.alloc(0);
    this.inbox = [];
    this.waiters = [];
    this.stderr = "";
    this.child.stdout.on("data", d => this.terima(d));
    this.child.stderr.on("data", d => { this.stderr += d.toString("utf8"); });
  }

  terima(potongan) {
    this.buffer = Buffer.concat([this.buffer, potongan]);
    for (;;) {
      const split = this.buffer.indexOf("\r\n\r\n");
      if (split < 0) return;
      const m = /Content-Length:\s*(\d+)/i.exec(this.buffer.subarray(0, split).toString("ascii"));
      const start = split + 4;
      const length = Number(m[1]);
      if (this.buffer.length < start + length) return;
      const message = JSON.parse(this.buffer.subarray(start, start + length).toString("utf8"));
      this.buffer = this.buffer.subarray(start + length);
      this.inbox.push(message);
      this.periksaPenunggu();
    }
  }

  periksaPenunggu() {
    for (const w of [...this.waiters]) {
      const i = this.inbox.findIndex(w.predicate);
      if (i < 0) continue;
      const [message] = this.inbox.splice(i, 1);
      this.waiters.splice(this.waiters.indexOf(w), 1);
      clearTimeout(w.timer);
      w.resolve(message);
    }
  }

  tunggu(predicate, label) {
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        reject(new Error(`Timed out waiting for ${label}; inbox=${JSON.stringify(this.inbox)} stderr=${this.stderr}`));
      }, TIMEOUT_MS);
      this.waiters.push({ predicate, resolve, timer });
      this.periksaPenunggu();
    });
  }

  event(name) {
    return this.tunggu(m => m.type === "event" && m.event === name, `event ${name}`);
  }

  kirim(command, args) {
    const seq = this.seq++;
    const body = Buffer.from(JSON.stringify({ seq, type: "request", command, arguments: args }), "utf8");
    this.child.stdin.write(`Content-Length: ${body.length}\r\n\r\n`);
    this.child.stdin.write(body);
    return this.tunggu(m => m.type === "response" && m.request_seq === seq, `response ${command}`);
  }

  async minta(command, args) {
    const response = await this.kirim(command, args);
    assert.ok(response.success, `${command} failed: ${response.message}`);
    return response.body;
  }

  // Collected output events of one category, in arrival order.
  keluaran(category) {
    return this.inbox
      .filter(m => m.type === "event" && m.event === "output" && m.body.category === category)
      .map(m => m.body.output).join("");
  }

  tutup() {
    this.child.stdin.end();
    this.child.kill();
  }
}

function tulisProgram(name, text) {
  const file = path.join(temp, name);
  fs.writeFileSync(file, text);
  return file;
}

// initialize → launch → initialized → setBreakpoints → configurationDone.
async function luncurkan(klien, program, lines, extra = {}) {
  const caps = await klien.minta("initialize", { adapterID: "indonesiaku", linesStartAt1: true });
  assert.strictEqual(caps.supportsConfigurationDoneRequest, true);
  const launch = klien.kirim("launch", { program, jalurIndk: indk, ...extra });
  await klien.event("initialized");
  const bp = await klien.minta("setBreakpoints", { source: { path: program }, breakpoints: lines.map(line => ({ line })) });
  assert.ok(bp.breakpoints.every(b => b.verified), "breakpoints in the program file must be verified");
  await klien.minta("configurationDone", {});
  const response = await launch;
  assert.ok(response.success, `launch failed: ${response.message}`);
}

async function berhenti(klien, reason) {
  const stopped = await klien.event("stopped");
  assert.strictEqual(stopped.body.reason, reason);
  const { stackFrames } = await klien.minta("stackTrace", { threadId: 1 });
  return stackFrames;
}

async function variabel(klien, scopeName) {
  const { scopes } = await klien.minta("scopes", { frameId: 0 });
  const scope = scopes.find(s => s.name === scopeName);
  assert.ok(scope, `scope ${scopeName} missing: ${JSON.stringify(scopes)}`);
  const { variables } = await klien.minta("variables", { variablesReference: scope.variablesReference });
  return Object.fromEntries(variables.map(v => [v.name, v.value]));
}

async function ujiBreakpointLangkahVariabel() {
  const program = tulisProgram("tambah.idk", PROGRAM);
  const klien = new KlienDap();
  try {
    await luncurkan(klien, program, [7]);
    let frames = await berhenti(klien, "breakpoint");
    assert.strictEqual(frames[0].line, 7);
    assert.strictEqual(frames[0].name, "skrip");
    assert.ok(path.resolve(frames[0].source.path) === path.resolve(program));
    assert.deepStrictEqual(await variabel(klien, "Global"), { total: "0", i: "0" });

    const hover = await klien.minta("evaluate", { expression: "total", context: "hover", frameId: 0 });
    assert.strictEqual(hover.result, "0");
    const bad = await klien.kirim("evaluate", { expression: "total + 1", context: "watch", frameId: 0 });
    assert.strictEqual(bad.success, false);

    await klien.minta("stepIn", { threadId: 1 });
    frames = await berhenti(klien, "step");
    assert.deepStrictEqual(frames.map(f => [f.name, f.line]), [["tambah", 3], ["skrip", 7]]);
    assert.deepStrictEqual(await variabel(klien, "Parameter"), { a: "0", b: "0" });

    await klien.minta("next", { threadId: 1 });
    frames = await berhenti(klien, "step");
    assert.deepStrictEqual(frames.map(f => [f.name, f.line]), [["tambah", 4], ["skrip", 7]]);

    await klien.minta("stepOut", { threadId: 1 });
    frames = await berhenti(klien, "step");
    assert.strictEqual(frames.length, 1, "stepOut must return to the script frame");

    await klien.minta("continue", { threadId: 1 });
    frames = await berhenti(klien, "breakpoint");
    assert.strictEqual(frames[0].line, 7);
    const i = await klien.minta("evaluate", { expression: "i", context: "hover", frameId: 0 });
    assert.strictEqual(i.result, "1");

    // Clearing breakpoints while stopped lets the program run to the end.
    await klien.minta("setBreakpoints", { source: { path: program }, breakpoints: [] });
    await klien.minta("continue", { threadId: 1 });
    const exited = await klien.event("exited");
    await klien.event("terminated");
    assert.strictEqual(exited.body.exitCode, 0);
    assert.strictEqual(klien.keluaran("stdout").replace(/\r\n/g, "\n"), "total 3\n");
    await klien.minta("disconnect", {});
  } finally {
    klien.tutup();
  }
}

async function ujiGalatProgram() {
  const program = tulisProgram("galat.idk", "x = 1\ny = x / 0\ncetak(y)\n");
  const klien = new KlienDap();
  try {
    await luncurkan(klien, program, [], { stopOnEntry: true });
    const frames = await berhenti(klien, "entry");
    assert.strictEqual(frames[0].line, 1);
    await klien.minta("continue", { threadId: 1 });
    const exited = await klien.event("exited");
    await klien.event("terminated");
    assert.strictEqual(exited.body.exitCode, 70);
    assert.match(klien.keluaran("stderr"), /Pembagian dengan nol/);
    assert.strictEqual(klien.keluaran("stdout"), "");
  } finally {
    klien.tutup();
  }
}

async function ujiBreakpointBerkasLainDanBinHilang() {
  const program = tulisProgram("lain.idk", "cetak(1)\n");
  const klien = new KlienDap();
  try {
    await klien.minta("initialize", { adapterID: "indonesiaku" });
    const launch = klien.kirim("launch", { program, jalurIndk: path.join(temp, "tidak-ada-indk") });
    await klien.event("initialized");
    const bp = await klien.minta("setBreakpoints", {
      source: { path: path.join(temp, "modul.idk") }, breakpoints: [{ line: 1 }]
    });
    assert.strictEqual(bp.breakpoints[0].verified, false);
    await klien.minta("configurationDone", {});
    const response = await launch;
    assert.strictEqual(response.success, false);
    assert.match(response.message, /jalurIndk/);
  } finally {
    klien.tutup();
  }
}

// awakutu freezes on the first line before breakpoints exist; a breakpoint
// there must still be reported instead of being run past by `lanjut`.
async function ujiBreakpointBarisPertama() {
  const program = tulisProgram("pertama.idk", "nama = \"dunia\"\ncetak(\"halo\", nama)\n");
  const klien = new KlienDap();
  try {
    await luncurkan(klien, program, [1]);
    const frames = await berhenti(klien, "breakpoint");
    assert.strictEqual(frames[0].line, 1);
    await klien.minta("continue", { threadId: 1 });
    await klien.event("terminated");
    assert.strictEqual(klien.keluaran("stdout").replace(/\r\n/g, "\n"), "halo dunia\n");
  } finally {
    klien.tutup();
  }
}

// Parameter lookup must use the definition, not an earlier call site.
function ujiNamaParameter() {
  const { namaParameter } = require("./awakutu-sumber.js");
  assert.deepStrictEqual(namaParameter("jika tambah(x) {\n}\nfungsi tambah(a, b=2) {\n}\n", "tambah"), ["a", "b"]);
  assert.deepStrictEqual(namaParameter("kelas K {\n    init(p) { diri.p = p }\n}\n", "init"), ["p"]);
}

async function ujiDisconnectSaatBerhenti() {
  const program = tulisProgram("putus.idk", PROGRAM);
  const klien = new KlienDap();
  try {
    await luncurkan(klien, program, [3]);
    await berhenti(klien, "breakpoint");
    await klien.minta("disconnect", { terminateDebuggee: true });
    await klien.event("terminated");
    assert.strictEqual(klien.keluaran("stdout"), "");
  } finally {
    klien.tutup();
  }
}

async function main() {
  assert.ok(fs.existsSync(indk), `indk binary not found: ${indk}`);
  await ujiBreakpointLangkahVariabel();
  ujiNamaParameter();
  await ujiGalatProgram();
  await ujiBreakpointBarisPertama();
  await ujiBreakpointBerkasLainDanBinHilang();
  await ujiDisconnectSaatBerhenti();
  console.log("DAP adapter integration test passed");
}

main()
  .catch(err => { console.error(err); process.exitCode = 1; })
  .finally(() => {
    // Windows may still hold a killed child's cwd for a moment.
    try {
      fs.rmSync(temp, { recursive: true, force: true, maxRetries: 5, retryDelay: 200 });
    } catch (err) {
      console.error(`warning: could not remove ${temp}: ${err.message}`);
    }
  });
