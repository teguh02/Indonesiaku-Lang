// Analisis teks sumber untuk adapter debugger.
//
// `indk awakutu` hanya bisa menampilkan SATU nama per perintah (`lihat
// <nama>`) dan tidak punya perintah "daftar semua variabel". Supaya panel
// Variables VS Code tetap berisi sesuatu, adapter menebak nama kandidat dari
// teks sumber lalu menanyakan satu per satu; nama yang tidak dikenal awakutu
// dibuang. Tebakan ini sengaja longgar: salah tebak hanya berarti satu
// perintah `lihat` ekstra, bukan nilai yang salah.
"use strict";

const { KEYWORDS } = require("./language-data.js");

const KATA_KUNCI = new Set(KEYWORDS);
const IDENT = "[\\p{L}_][\\p{L}\\p{N}_]*";
const DAFTAR_IDENT = `${IDENT}(?:\\s*,\\s*${IDENT})*`;
const RE_PENUGASAN = new RegExp(
  `^\\s*(?:variabel\\s+|global\\s+)?(${DAFTAR_IDENT})\\s*(?:\\*\\*|//|<<|>>|[-+*/%&|^])?=(?!=)`, "u");
const RE_UNTUK = new RegExp(`\\buntuk\\s+(${DAFTAR_IDENT})\\s+dalam\\b`, "u");
const RE_IDENT_PENUH = new RegExp(`^${IDENT}$`, "u");
const BATAS_KANDIDAT = 200;

// Buang komentar dan isi string kasar-kasaran: cukup agar `#` atau `=` di
// dalam teks tidak dibaca sebagai kode. Tidak perlu sempurna (lihat atas).
function bersihkanBaris(baris) {
  let hasil = "";
  let kutip = null;
  for (let i = 0; i < baris.length; i++) {
    const c = baris[i];
    if (kutip) {
      if (c === "\\") { i++; continue; }
      if (c === kutip) { kutip = null; hasil += c; }
      continue;
    }
    if (c === "#") break;
    if (c === "\"" || c === "'") { kutip = c; hasil += c; continue; }
    hasil += c;
  }
  return hasil;
}

function pecahNama(daftar) {
  return daftar.split(",").map(s => s.trim()).filter(n => n && !KATA_KUNCI.has(n));
}

// Nama yang mungkin menjadi variabel global (penugasan, `variabel`, `global`,
// variabel `untuk`, unpacking `a, b = ...`), urut kemunculan pertama.
function kandidatGlobal(sumber) {
  const terlihat = new Set();
  const hasil = [];
  for (const mentah of sumber.split(/\r?\n/)) {
    const baris = bersihkanBaris(mentah);
    const cocok = [RE_PENUGASAN.exec(baris), RE_UNTUK.exec(baris)];
    for (const m of cocok) {
      if (!m) continue;
      for (const nama of pecahNama(m[1])) {
        if (terlihat.has(nama) || hasil.length >= BATAS_KANDIDAT) continue;
        terlihat.add(nama);
        hasil.push(nama);
      }
    }
  }
  return hasil;
}

function escapeRegex(s) {
  return s.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

// Nama parameter fungsi/metode `namaFungsi`, dibaca dari kepalanya
// (`fungsi f(a, b=2) {` atau metode kelas `f(a) {` di awal baris). Pola
// `fungsi` dicoba lebih dulu di seluruh berkas, supaya pemanggilan seperti
// `jika f(x) {` sebelum definisinya tidak terbaca sebagai kepala fungsi.
function namaParameter(sumber, namaFungsi) {
  if (!namaFungsi || namaFungsi === "skrip") return [];
  const nama = escapeRegex(namaFungsi);
  const pola = [
    new RegExp(`\\bfungsi\\s+${nama}\\s*\\(([^)]*)\\)\\s*\\{`, "u"),
    new RegExp(`^\\s*${nama}\\s*\\(([^)]*)\\)\\s*\\{`, "u")
  ];
  const baris = sumber.split(/\r?\n/).map(bersihkanBaris);
  for (const re of pola) {
    for (const teks of baris) {
      const m = re.exec(teks);
      if (!m) continue;
      return m[1].split(",")
        .map(p => p.split("=")[0].trim())
        .filter(p => RE_IDENT_PENUH.test(p));
    }
  }
  return [];
}

// Baris pertama yang berisi kode di dalam setiap badan fungsi/metode.
// Dipakai untuk meniru "step into": awakutu hanya punya `langkah` (step
// over), jadi adapter memasang breakpoint sementara di pintu masuk setiap
// fungsi sebelum melangkah.
function barisMasukFungsi(sumber) {
  const baris = sumber.split(/\r?\n/).map(bersihkanBaris);
  const reKepala = new RegExp(`(?:^|[^\\p{L}\\p{N}_.])(?:fungsi\\s*)?${IDENT}?\\s*\\([^)]*\\)\\s*\\{\\s*(.*)$`, "u");
  const reFungsi = /\bfungsi\b/u;
  const reMetode = new RegExp(`^\\s*(${IDENT})\\s*\\([^)]*\\)\\s*\\{`, "u");
  const hasil = [];
  for (let i = 0; i < baris.length; i++) {
    const teks = baris[i];
    const metode = reMetode.exec(teks);
    const adalahMetode = metode !== null && !KATA_KUNCI.has(metode[1]);
    if (!reFungsi.test(teks) && !adalahMetode) continue;
    const m = reKepala.exec(teks);
    if (!m) continue;
    const sisa = m[1].replace(/\}\s*$/, "").trim();
    if (sisa) { hasil.push(i + 1); continue; }
    for (let j = i + 1; j < baris.length; j++) {
      const isi = baris[j].trim();
      if (isi === "" || isi === "}") continue;
      hasil.push(j + 1);
      break;
    }
  }
  return [...new Set(hasil)];
}

module.exports = { kandidatGlobal, namaParameter, barisMasukFungsi, bersihkanBaris };
