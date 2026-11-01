"use strict";

// Keep these lists shared by the VS Code completion provider and LSP server.
const KEYWORDS = [
  "adalah", "akhirnya", "atau", "benar", "buang", "bukan", "coba", "cocokkan", "dalam", "dan", "dari",
  "dengan", "diri", "fungsi", "global", "hasilkan", "hentikan", "impor", "jika", "jika_lain",
  "kasus", "kecuali", "kelas", "kembalikan", "kosong", "lanjut", "lewati", "naikkan", "nonlokal",
  "pastikan", "salah", "sebagai", "selagi", "selain", "super", "tidak", "untuk", "variabel"
];

const BUILTINS = [
  "abs", "acak", "acak_bulat", "acos", "ada_berkas", "ada_yang", "adalah_instansi", "akar",
  "ambil_atribut", "ambil_lingkungan", "asin", "atan", "atan2", "atap", "atur_atribut", "baca",
  "baca_baris", "baca_semua", "bagi_sisa", "biner", "bisa_dipanggil", "buat_direktori",
  "buka_berkas", "bulat", "bulatkan", "cetak", "chr", "cos", "daftar", "db_ambil_satu",
  "db_ambil_semua", "db_baris_terpengaruh", "db_batal", "db_batas_tunggu", "db_buka", "db_eksekusi", "db_id_terakhir",
  "db_komit", "db_mulai", "db_tutup", "derajat", "direktori_kini", "enumerasi", "format",
  "format_tanggal", "ganti", "hapus", "hapus_berkas", "hapus_direktori", "hapus_kunci",
  "hapus_pada", "hash", "heksa", "henti", "himpunan", "himpunan_beku", "huruf_besar",
  "huruf_kecil", "id", "input", "isi_direktori", "iter", "jalur_indk", "jam", "jenis", "jumlah",
  "kamus", "karakter", "ke_angka", "ke_bulat", "ke_desimal", "ke_logika", "ke_teks", "keluar",
  "kunci", "lanjutkan", "lantai", "lingkungan", "max", "md5", "min", "oktal", "ord", "pangkat",
  "panjang", "panjang_byte", "pasangkan", "peta", "potong", "punya", "punya_atribut", "radian", "rentang", "repr",
  "saring", "semua", "sha1", "sha256", "sha512", "sin", "sisip", "soket_baca", "soket_batas_waktu", "soket_dengar",
  "soket_sambung", "soket_terima", "soket_tulis", "soket_tutup", "tambah", "tambah_berkas", "tan",
  "tanggal", "terbalik", "terurut", "tulis", "tulis_galat", "tulis_semua", "tupel", "turunan_dari",
  "tutup", "waktu"
];

const MAGIC_METHODS = [
  "_ambil_", "_atur_", "_bagi_", "_bagi_bulat_", "_berikutnya_", "_hash_", "_iter_",
  "_kali_", "_kbagi_", "_kbagi_bulat_", "_keluar_", "_kkali_", "_kkurangi_", "_kpangkat_",
  "_ksisa_", "_ktambah_", "_kurang_dari_", "_kurang_sama_", "_kurangi_", "_lebih_dari_",
  "_lebih_sama_", "_logika_", "_masuk_", "_negatif_", "_panggil_", "_pangkat_", "_panjang_",
  "_repr_", "_sama_", "_sisa_", "_tambah_", "_teks_", "_tidak_sama_"
];

const DESCRIPTIONS = {
  fungsi: "Definisikan fungsi dengan blok `{ }`.",
  kelas: "Definisikan kelas; pewarisan memakai `<`.",
  jika: "Jalankan blok jika kondisi bernilai benar.",
  jika_lain: "Cabang kondisi lanjutan setelah `jika`.",
  selain: "Blok alternatif pada percabangan.",
  selagi: "Ulangi blok selama kondisi bernilai benar.",
  untuk: "Iterasi elemen koleksi dengan `untuk x dalam daftar { ... }`.",
  dalam: "Operator keanggotaan koleksi.",
  kembalikan: "Kembalikan nilai dari fungsi.",
  variabel: "Deklarasikan variabel lokal eksplisit.",
  diri: "Referensi ke instance saat ini di metode kelas.",
  super: "Akses metode kelas induk.",
  coba: "Mulai blok yang dapat menangkap error.",
  kecuali: "Tangkap error dari `naikkan` atau native.",
  naikkan: "Lempar error yang dapat ditangkap `kecuali`.",
  impor: "Muat modul Indonesiaku.",
  cetak: "Cetak nilai ke stdout.",
  panjang: "Jumlah karakter string atau elemen koleksi.",
  ke_angka: "Konversi teks menjadi angka; gagal dengan error yang dapat ditangkap.",
  ke_teks: "Konversi nilai menjadi string.",
  json_urai: "Parse teks JSON menjadi nilai Indonesiaku.",
  json_teks: "Serialisasi nilai Indonesiaku menjadi teks JSON."
};

module.exports = { KEYWORDS, BUILTINS, MAGIC_METHODS, DESCRIPTIONS };
