// Debug Adapter Protocol (DAP) untuk Indonesiaku, di atas `indk awakutu`.
//
// Tanpa dependensi npm. Dua cara pakai:
//   - di dalam VS Code: extension.js membungkus SesiAwakutu sebagai
//     DebugAdapterInlineImplementation (tidak butuh `node` di PATH);
//   - berdiri sendiri: `node debug-adapter.js` bicara DAP lewat stdio
//     (dipakai test-dap.js dan editor lain yang mendukung DAP).
//
// Batas yang diwarisi dari awakutu (lihat README): breakpoint hanya per
// nomor baris berkas utama, `lihat` hanya global dan parameter, tidak ada
// pause, dan "step into"/"step out" ditiru dari `langkah` + `henti`.
"use strict";

const fs = require("fs");
const path = require("path");
const { AwakutuProses, uraiHenti } = require("./awakutu-proses.js");
const { kandidatGlobal, namaParameter, barisMasukFungsi } = require("./awakutu-sumber.js");

const ID_UTAS = 1;
const MAKS_BREAKPOINT = 64;        // DEBUG_BREAK_MAX di src/vm.h
const MAKS_LANGKAH_KELUAR = 10000; // batas tiruan "step out"
const RE_NAMA = /^[\p{L}_][\p{L}\p{N}_]*$/u;
const REF_SKOP = 1000;

function samaJalur(a, b) {
  const x = path.resolve(a);
  const y = path.resolve(b);
  return process.platform === "win32" ? x.toLowerCase() === y.toLowerCase() : x === y;
}

class SesiAwakutu {
  constructor(kirimPesan) {
    this.kirimPesan = kirimPesan;
    this.seq = 1;
    this.proses = null;
    this.program = null;
    this.sumber = "";
    this.diinginkan = new Set();   // breakpoint dari VS Code
    this.terpasang = new Set();    // breakpoint yang sudah diketahui awakutu
    this.bingkai = [];
    this.berhenti = false;
    this.antrian = Promise.resolve();
    this.berakhir = false;
    this.dimulai = false;          // henti pertama awakutu sudah dibaca
    let siap;
    this.konfigurasiSelesai = new Promise(r => { siap = r; });
    this.tandaiKonfigurasi = siap;
  }

  // ---- transport ---------------------------------------------------------
  kirim(pesan) { this.kirimPesan({ seq: this.seq++, ...pesan }); }

  event(nama, body) { this.kirim({ type: "event", event: nama, body }); }

  jawab(permintaan, body, pesanGalat) {
    const ok = pesanGalat === undefined;
    const jawaban = { type: "response", request_seq: permintaan.seq, command: permintaan.command, success: ok };
    if (ok) jawaban.body = body || {};
    else jawaban.message = pesanGalat;
    this.kirim(jawaban);
  }

  keluaran(teks, kategori) {
    if (teks) this.event("output", { category: kategori, output: teks });
  }

  // Semua interaksi dengan proses berjalan berurutan: awakutu hanya
  // menjawab satu perintah per prompt.
  urutkan(kerja) {
    const hasil = this.antrian.then(kerja);
    this.antrian = hasil.catch(() => {});
    return hasil;
  }

  handleMessage(pesan) {
    if (pesan.type !== "request") return;
    const penangan = this[`req_${pesan.command}`];
    if (!penangan) {
      this.jawab(pesan, undefined, `Permintaan '${pesan.command}' tidak didukung awakutu.`);
      return;
    }
    Promise.resolve()
      .then(() => penangan.call(this, pesan, pesan.arguments || {}))
      .catch(err => this.jawab(pesan, undefined, err && err.message ? err.message : String(err)));
  }

  // ---- siklus hidup ------------------------------------------------------
  req_initialize(req) {
    this.jawab(req, {
      supportsConfigurationDoneRequest: true,
      supportsEvaluateForHovers: true,
      supportsTerminateRequest: true
    });
  }

  req_configurationDone(req) {
    this.jawab(req);
    this.tandaiKonfigurasi();
  }

  async req_launch(req, args) {
    if (!args.program) throw new Error("Atribut 'program' wajib diisi (berkas .idk).");
    const program = path.resolve(args.cwd || process.cwd(), args.program);
    if (!fs.existsSync(program)) throw new Error(`Berkas tidak ditemukan: ${program}`);
    this.program = program;
    this.sumber = fs.readFileSync(program, "utf8");
    const bin = args.jalurIndk || process.env.INDONESIAKU_INDK || "indk";
    // 'initialized' baru dikirim setelah berkas program diketahui, supaya
    // setBreakpoints untuk berkas lain bisa ditolak dengan benar.
    this.event("initialized");
    this.proses = this.buatProses();
    let awal;
    try {
      awal = await this.proses.mulai(bin, program, args.args || [], args.cwd || path.dirname(program));
    } catch (err) {
      throw new Error(`Tidak dapat menjalankan '${bin}': ${err.message}. ` +
        "Atur setelan indonesiaku.jalurIndk atau atribut jalurIndk.");
    }
    await this.konfigurasiSelesai;
    this.jawab(req);
    await this.urutkan(() => this.mulaiProgram(awal, Boolean(args.stopOnEntry)))
      .catch(err => this.gagalDiTengah(err));
  }

  buatProses() {
    const p = new AwakutuProses();
    p.on("stdout", teks => this.keluaran(teks, "stdout"));
    p.on("stderr", teks => this.keluaran(teks, "stderr"));
    return p;
  }

  async mulaiProgram(awal, hentiDiAwal) {
    this.dimulai = true;
    if (awal.selesai) return this.akhiri(awal);
    const henti = uraiHenti(awal.teks);
    this.keluaran(henti.sebelum, "stderr");
    await this.sinkronBreakpoint(new Set());
    if (hentiDiAwal) return this.catatHenti("entry");
    // awakutu selalu beku di instruksi pertama, dan `lanjut` tidak memeriksa
    // ulang baris itu: breakpoint di baris pertama harus dilaporkan di sini.
    if (this.diinginkan.has(henti.baris)) return this.catatHenti("breakpoint");
    return this.olahHasil(await this.proses.kirim("lanjut"), "breakpoint");
  }

  // Galat adapter setelah jawaban launch terkirim: laporkan, jangan ditelan.
  gagalDiTengah(err) {
    this.keluaran(`awakutu: ${err && err.message ? err.message : err}\n`, "console");
  }

  akhiri(hasil) {
    if (this.berakhir) return;
    this.berakhir = true;
    this.berhenti = false;
    this.keluaran(hasil.teks, "stderr");
    this.event("exited", { exitCode: hasil.kode });
    this.event("terminated");
  }

  // ---- breakpoint ----------------------------------------------------------
  req_setBreakpoints(req, args) {
    const jalur = args.source && args.source.path;
    const baris = (args.breakpoints || []).map(b => b.line);
    const milikProgram = this.program !== null && jalur && samaJalur(jalur, this.program);
    if (!milikProgram) {
      const pesan = "awakutu hanya mendukung breakpoint di berkas yang dijalankan.";
      this.jawab(req, { breakpoints: baris.map(line => ({ verified: false, line, message: pesan })) });
      return;
    }
    const diterima = baris.slice(0, MAKS_BREAKPOINT);
    this.diinginkan = new Set(diterima);
    this.jawab(req, {
      breakpoints: baris.map((line, i) => i < MAKS_BREAKPOINT
        ? { verified: true, line }
        : { verified: false, line, message: `Maksimum ${MAKS_BREAKPOINT} breakpoint.` })
    });
    // Selalu antre: saat program berjalan antrean menunggu sampai henti
    // berikutnya (awakutu baru membaca stdin di prompt), jadi perubahan di
    // tengah sinkronisasi atau langkah tidak pernah hilang.
    // Sebelum mulaiProgram, henti pertama belum dibaca; mulaiProgram sendiri
    // yang menyinkronkan breakpoint di sana.
    if (this.dimulai && !this.proses.keluar) {
      this.urutkan(() => this.sinkronBreakpoint(new Set())).catch(err => this.gagalDiTengah(err));
    }
  }

  // Samakan breakpoint awakutu dengan `diinginkan` ditambah `sementara`.
  async sinkronBreakpoint(sementara) {
    const target = new Set([...this.diinginkan, ...sementara]);
    for (const b of [...this.terpasang]) {
      if (target.has(b)) continue;
      await this.proses.tanya(`hapus ${b}`);
      this.terpasang.delete(b);
    }
    for (const b of target) {
      if (this.terpasang.has(b) || this.terpasang.size >= MAKS_BREAKPOINT) continue;
      await this.proses.tanya(`henti ${b}`);
      this.terpasang.add(b);
    }
  }

  // ---- eksekusi ------------------------------------------------------------
  // Hasil `lanjut`/`langkah`: program selesai, atau berhenti di baris baru.
  async olahHasil(hasil, alasan) {
    if (hasil.selesai) return this.akhiri(hasil);
    const henti = uraiHenti(hasil.teks);
    this.keluaran(henti.sebelum, "stderr");
    await this.sinkronBreakpoint(new Set());
    return this.catatHenti(alasan);
  }

  async catatHenti(alasan) {
    this.bingkai = await this.proses.tumpukan();
    this.berhenti = true;
    this.event("stopped", { reason: alasan, threadId: ID_UTAS, allThreadsStopped: true });
  }

  jalankan(req, kerja) {
    if (!this.berhenti) {
      this.jawab(req, undefined, "Program sedang berjalan.");
      return;
    }
    this.berhenti = false;
    this.jawab(req, req.command === "continue" ? { allThreadsContinued: true } : undefined);
    this.urutkan(kerja).catch(err => this.gagalDiTengah(err));
  }

  req_continue(req) {
    this.jalankan(req, async () => this.olahHasil(await this.proses.kirim("lanjut"), "breakpoint"));
  }

  req_next(req) {
    this.jalankan(req, async () => this.olahHasil(await this.proses.kirim("langkah"), "step"));
  }

  // awakutu tidak punya "step into": `langkah` tidak masuk ke fungsi. Pasang
  // breakpoint sementara di baris pertama setiap fungsi, melangkah, lalu
  // lepaskan lagi. Kalau baris ini memanggil fungsi, breakpoint itu kena
  // lebih dulu daripada baris berikutnya.
  req_stepIn(req) {
    this.jalankan(req, async () => {
      const masuk = barisMasukFungsi(this.sumber).filter(b => !this.diinginkan.has(b));
      await this.sinkronBreakpoint(new Set(masuk));
      return this.olahHasil(await this.proses.kirim("langkah"), "step");
    });
  }

  // awakutu juga tidak punya "step out": ulangi `langkah` sampai tumpukan
  // lebih dangkal (atau kena breakpoint). Di tingkat skrip = lanjut.
  req_stepOut(req) {
    this.jalankan(req, async () => {
      const kedalaman = this.bingkai.length;
      if (kedalaman <= 1) return this.olahHasil(await this.proses.kirim("lanjut"), "breakpoint");
      for (let i = 0; i < MAKS_LANGKAH_KELUAR; i++) {
        const hasil = await this.proses.kirim("langkah");
        if (hasil.selesai) return this.akhiri(hasil);
        const henti = uraiHenti(hasil.teks);
        this.keluaran(henti.sebelum, "stderr");
        const bingkai = await this.proses.tumpukan();
        if (bingkai.length < kedalaman || this.diinginkan.has(henti.baris)) {
          return this.catatHenti(bingkai.length < kedalaman ? "step" : "breakpoint");
        }
      }
      this.keluaran(`awakutu: step out berhenti setelah ${MAKS_LANGKAH_KELUAR} langkah.\n`, "console");
      return this.catatHenti("step");
    });
  }

  req_pause(req) {
    this.jawab(req, undefined, "awakutu tidak dapat menjeda program yang sedang berjalan; pasang breakpoint.");
  }

  // ---- inspeksi ------------------------------------------------------------
  req_threads(req) {
    this.jawab(req, { threads: [{ id: ID_UTAS, name: "utama" }] });
  }

  req_stackTrace(req) {
    const sumber = { name: path.basename(this.program || ""), path: this.program };
    const stackFrames = this.bingkai.map((b, i) => ({
      id: i, name: b.nama, line: b.baris, column: 1, source: sumber
    }));
    this.jawab(req, { stackFrames, totalFrames: stackFrames.length });
  }

  req_scopes(req, args) {
    const id = args.frameId || 0;
    const scopes = [];
    // `lihat` hanya bekerja pada bingkai terdalam (tempat program berhenti).
    if (id === 0 && this.bingkai.length > 0 && this.bingkai[0].nama !== "skrip") {
      scopes.push({ name: "Parameter", variablesReference: REF_SKOP + 1, expensive: false });
    }
    scopes.push({ name: "Global", variablesReference: REF_SKOP + 2, expensive: false });
    this.jawab(req, { scopes });
  }

  async req_variables(req, args) {
    if (!this.berhenti) return this.jawab(req, { variables: [] });
    const params = this.bingkai.length > 0 ? namaParameter(this.sumber, this.bingkai[0].nama) : [];
    const nama = args.variablesReference === REF_SKOP + 1
      ? params
      : kandidatGlobal(this.sumber).filter(n => !params.includes(n));
    const variables = await this.urutkan(async () => {
      const hasil = [];
      for (const n of nama) {
        const v = await this.proses.lihat(n);
        if (v.ada) hasil.push({ name: n, value: v.nilai, variablesReference: 0 });
      }
      return hasil;
    });
    return this.jawab(req, { variables });
  }

  async req_evaluate(req, args) {
    const ekspresi = String(args.expression || "").trim();
    if (!this.berhenti) return this.jawab(req, undefined, "Program tidak sedang berhenti.");
    if (!RE_NAMA.test(ekspresi)) {
      return this.jawab(req, undefined, "awakutu hanya dapat menampilkan nama variabel, bukan ekspresi.");
    }
    const v = await this.urutkan(() => this.proses.lihat(ekspresi));
    if (!v.ada) return this.jawab(req, undefined, v.pesan);
    return this.jawab(req, { result: v.nilai, variablesReference: 0 });
  }

  // ---- penutupan -----------------------------------------------------------
  async req_disconnect(req) {
    await this.hentikanProgram();
    this.jawab(req);
  }

  async req_terminate(req) {
    await this.hentikanProgram();
    this.jawab(req);
  }

  async hentikanProgram() {
    const p = this.proses;
    if (!p || p.keluar) return;
    const keluar = new Promise(r => p.once("keluar", r));
    if (this.berhenti) {
      this.berhenti = false;
      p.anak.stdin.write("keluar\n");
    } else {
      p.hentikanPaksa();
    }
    const batas = setTimeout(() => p.hentikanPaksa(), 2000);
    await keluar;
    clearTimeout(batas);
    if (!this.berakhir) {
      this.berakhir = true;
      this.event("terminated");
    }
  }

  dispose() {
    if (this.proses) this.proses.hentikanPaksa();
  }
}

// ---- mode berdiri sendiri: DAP di stdio ---------------------------------
function jalankanStdio(masuk, keluar) {
  const sesi = new SesiAwakutu(pesan => {
    const isi = Buffer.from(JSON.stringify(pesan), "utf8");
    keluar.write(`Content-Length: ${isi.length}\r\n\r\n`);
    keluar.write(isi);
  });
  let buffer = Buffer.alloc(0);
  masuk.on("data", potongan => {
    buffer = Buffer.concat([buffer, potongan]);
    for (;;) {
      const pisah = buffer.indexOf("\r\n\r\n");
      if (pisah < 0) return;
      const m = /Content-Length:\s*(\d+)/i.exec(buffer.subarray(0, pisah).toString("ascii"));
      if (!m) { buffer = buffer.subarray(pisah + 4); continue; }
      const awal = pisah + 4;
      const panjang = Number(m[1]);
      if (buffer.length < awal + panjang) return;
      const isi = buffer.subarray(awal, awal + panjang).toString("utf8");
      buffer = buffer.subarray(awal + panjang);
      let pesan;
      try {
        pesan = JSON.parse(isi);
      } catch (err) {
        process.stderr.write(`debug-adapter: pesan DAP rusak diabaikan: ${err.message}\n`);
        continue;
      }
      sesi.handleMessage(pesan);
    }
  });
  masuk.on("end", () => sesi.dispose());
  return sesi;
}

if (require.main === module) {
  jalankanStdio(process.stdin, process.stdout);
}

module.exports = { SesiAwakutu, jalankanStdio };
