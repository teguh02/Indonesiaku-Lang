// Pembungkus proses `indk awakutu <berkas>`.
//
// Protokol awakutu (src/vm.c, awakutuPrompt): perintah teks satu baris di
// stdin; semua keluaran debugger ke stderr; stdout murni milik program.
// Setiap kali program beku, awakutu mencetak
//
//     \n[baris N] <fungsi>\n<cuplikan sumber>\n(awakutu)
//
// lalu setiap perintah dijawab dengan teks diakhiri prompt "(awakutu) " yang
// baru. Jadi satu "segmen" = semua teks stderr sampai prompt berikutnya, atau
// sampai proses keluar (program selesai, galat runtime, atau `keluar`).
"use strict";

const { spawn } = require("child_process");
const { EventEmitter } = require("events");

const PROMPT = "(awakutu) ";
const SPANDUK = /^Debugger Indonesiaku\.[^\n]*\n/;
const RE_HENTI = /(?:^|\n)\[baris (\d+)\] ([^\n]*)\n/g;
const RE_BINGKAI = /^\s*#(\d+) \[baris (\d+)\] (.*)$/;

class AwakutuProses extends EventEmitter {
  constructor() {
    super();
    this.anak = null;
    this.buffer = "";
    this.penunggu = null;
    this.keluar = null;      // { kode } setelah proses berakhir
  }

  // Mulai proses dan tunggu henti pertama (awakutu selalu berhenti di
  // instruksi pertama). Hasil: lihat tungguSegmen().
  mulai(bin, program, args, cwd) {
    return new Promise((resolve, reject) => {
      let anak;
      try {
        anak = spawn(bin, ["awakutu", program, ...args], { cwd, stdio: ["pipe", "pipe", "pipe"] });
      } catch (err) {
        reject(err);
        return;
      }
      this.anak = anak;
      let mulai = false;
      anak.on("error", err => { if (!mulai) reject(err); else this.emit("galat", err); });
      anak.on("spawn", () => { mulai = true; this.tungguSegmen().then(resolve, reject); });
      // setEncoding memakai StringDecoder: karakter UTF-8 yang terpotong di
      // batas potongan pipe tidak menjadi U+FFFD.
      anak.stdout.setEncoding("utf8");
      anak.stderr.setEncoding("utf8");
      anak.stdout.on("data", teks => this.emit("stdout", teks));
      anak.stderr.on("data", teks => this.terimaStderr(teks));
      anak.on("close", (kode, sinyal) => this.selesai(kode, sinyal));
      anak.stdin.on("error", () => { /* proses sudah keluar; ditangani di close */ });
    });
  }

  terimaStderr(teks) {
    this.buffer += teks.replace(/\r\n/g, "\n");
    this.salurkan();
  }

  salurkan() {
    if (!this.penunggu) return;
    const i = this.buffer.indexOf(PROMPT);
    if (i < 0) return;
    const segmen = this.buffer.slice(0, i);
    this.buffer = this.buffer.slice(i + PROMPT.length);
    const tunggu = this.penunggu;
    this.penunggu = null;
    tunggu({ selesai: false, teks: segmen });
  }

  selesai(kode, sinyal) {
    this.keluar = { kode: kode === null ? (sinyal ? 1 : 0) : kode };
    const sisa = this.buffer;
    this.buffer = "";
    if (this.penunggu) {
      const tunggu = this.penunggu;
      this.penunggu = null;
      tunggu({ selesai: true, teks: sisa, kode: this.keluar.kode });
    } else if (sisa) {
      this.emit("stderr", sisa);
    }
    this.emit("keluar", this.keluar.kode);
  }

  // Segmen berikutnya: { selesai:false, teks } di prompt, atau
  // { selesai:true, teks, kode } kalau proses berakhir lebih dulu.
  tungguSegmen() {
    if (this.keluar) return Promise.resolve({ selesai: true, teks: "", kode: this.keluar.kode });
    // Hanya satu penunggu: dua perintah sekaligus akan saling mencuri
    // jawaban. Pemanggil wajib mengurutkan (SesiAwakutu.urutkan).
    if (this.penunggu) return Promise.reject(new Error("perintah awakutu tumpang tindih"));
    return new Promise(resolve => {
      this.penunggu = resolve;
      this.salurkan();
    });
  }

  kirim(perintah) {
    if (this.keluar || !this.anak) {
      return Promise.resolve({ selesai: true, teks: "", kode: this.keluar ? this.keluar.kode : 0 });
    }
    this.anak.stdin.write(perintah + "\n");
    return this.tungguSegmen();
  }

  // Perintah yang tidak menjalankan program; jawabannya teks biasa.
  async tanya(perintah) {
    const hasil = await this.kirim(perintah);
    return hasil.teks;
  }

  // Bingkai pemanggilan dari `tumpukan`, terdalam lebih dulu.
  async tumpukan() {
    const teks = await this.tanya("tumpukan");
    const bingkai = [];
    for (const baris of teks.split("\n")) {
      const m = RE_BINGKAI.exec(baris);
      if (m) bingkai.push({ baris: Number(m[2]), nama: m[3] });
    }
    return bingkai;
  }

  // `lihat <nama>`: { ada, nilai } atau { ada:false, pesan }.
  async lihat(nama) {
    const teks = (await this.tanya(`lihat ${nama}`)).replace(/\n$/, "");
    const awalan = `  ${nama} = `;
    if (teks.startsWith(awalan)) return { ada: true, nilai: teks.slice(awalan.length) };
    return { ada: false, pesan: teks.trim() };
  }

  hentikanPaksa() {
    if (this.anak && !this.keluar) this.anak.kill();
  }
}

// Pisahkan segmen setelah `lanjut`/`langkah` menjadi stderr program (teks
// sebelum kepala henti) dan posisi henti.
function uraiHenti(teks) {
  const bersih = teks.replace(SPANDUK, "");
  let terakhir = null;
  RE_HENTI.lastIndex = 0;
  for (let m; (m = RE_HENTI.exec(bersih)) !== null;) terakhir = m;
  if (!terakhir) return { sebelum: bersih, baris: null, nama: null };
  return {
    sebelum: bersih.slice(0, terakhir.index),
    baris: Number(terakhir[1]),
    nama: terakhir[2]
  };
}

module.exports = { AwakutuProses, uraiHenti, PROMPT };
