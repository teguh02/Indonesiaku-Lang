# Rencana Fitur: Unicode, i18n dan konfigurasi (Fase 24, V1)

- **Status:** selesai (V1; sisa Fase 24 tercatat di §7)
- **Target versi:** 1.19.0 (V1; sisa Fase 24 menyusul)
- **Pemilik:** maintainer Indonesiaku

## 1. Masalah

Audit dilakukan pada `main` 57fb607 dengan menjalankan kode, bukan dari
ingatan. Temuan:

| Area | Yang ada | Kesenjangan yang terbukti |
|---|---|---|
| Huruf besar/kecil | `huruf_besar`/`huruf_kecil` (C, `toupper` per bita) | Hanya ASCII: `huruf_besar("ärger straße")` → `äRGER STRAßE`, `huruf_kecil("ÄΣ")` → `ÄΣ`. Tidak ada case folding. |
| Spasi | `pustaka/teks.idk` `teks_pangkas` | Hanya `" \t\n\r"`; NBSP (U+00A0), U+3000 dan spasi Unicode lain tidak dipangkas. Tipe teks tidak punya metode `pangkas`. |
| Escape teks | `\n \t \r \" \\ \'` | Tidak ada `\uXXXX`; karakter non-ASCII ditulis langsung atau lewat `chr()`. |
| Normalisasi | — | Tidak ada NFC/NFD: `"é"` (1 karakter) ≠ `"e" + chr(0x301)` (2 karakter). |
| Lingkungan | `ambil_lingkungan(nama, bawaan)`, `lingkungan()`; ditolak di `--sandbox` | Tidak ada pemuat `.env`, INI, prioritas sumber, maupun validasi tipe. |
| Format data | `pustaka/json.idk`, `pustaka/csv.idk` | Tidak ada INI/`.env`; TOML hanya subset manifest `urai_toml` di dalam `pustaka/paket.idk` (PR #145); YAML tidak ada. |
| Tanggal | `waktu`, `tanggal`, `format_tanggal` (strftime C) | Tidak ada locale/zona waktu bernama; di luar V1. |

Contoh yang canggung hari ini:

```indonesiaku
# Membaca PORT dari .env lalu memvalidasinya harus ditulis tangan:
isi = baca_semua(".env")          # BOM/CRLF, komentar, kutip: urus sendiri
port = ke_bulat(...)              # pesan galat tanpa nomor baris
```

## 2. API V1 (modul namespace, tanpa sintaks baru)

```indonesiaku
impor konfigurasi
konf = konfigurasi.muat(
    berkas = ".env",
    awalan_lingkungan = "APP_",
    skema = {
        "PORT": {"tipe": "bulat", "bawaan": 8080},
        "DEBUG": "logika",
        "SANDI_DB": {"tipe": "teks", "wajib": benar, "rahasia": benar},
    },
)
cetak(konf["PORT"], konf.sumber("PORT"))
cetak(konf)                       # SANDI_DB tampil sebagai ***

impor unicode
cetak(unicode.huruf_besar("straße"))            # STRASSE
cetak(unicode.lipat_huruf("ÄRGER") == unicode.lipat_huruf("ärger"))
cetak(unicode.pangkas(chr(0x3000) + "teks" + chr(0xA0)))
```

## 3. Semantik

### `pustaka/konfigurasi.idk`

- `urai_env(teks, sumber, batas_ukuran)` → kamus. Baris kosong dan `# ...`
  diabaikan; awalan `export ` boleh; kunci `[A-Za-z_][A-Za-z0-9_.-]*`; nilai
  tanpa kutip dipangkas dan komentar ` #` di belakangnya dibuang; `'...'`
  literal; `"..."` mengenal `\n \t \r \" \\`. **Tidak ada interpolasi
  `${VAR}`** (sengaja: tidak ada pembacaan lingkungan tersembunyi). Kunci
  ganda: yang terakhir menang (seperti python-dotenv).
- `urai_ini(teks, sumber, batas_ukuran)` → kamus bagian → kamus. `;`/`#` di
  awal baris adalah komentar; `kunci = nilai` atau `kunci: nilai`; kunci
  sebelum bagian pertama masuk bagian `""`; bagian/kunci ganda adalah galat
  (seperti `configparser` mode ketat). Kunci peka huruf besar (beda dari
  `configparser` yang mengecilkan), tidak ada baris lanjutan maupun
  interpolasi.
- BOM UTF-8 di awal dan `\r\n`/`\r` dinormalkan sebelum mengurai.
- Batas: `BATAS_UKURAN` = 1 MiB (bita UTF-8) per sumber; berkas yang lebih
  besar ditolak lewat `jalur.ukuran` **sebelum** dibaca.
- Semua galat format adalah `GalatKonfigurasi` dengan `.pesan`
  (`"<sumber>:<baris>: <alasan>"`), `.sumber`, `.baris`. Pesan tidak pernah
  memuat nilai, hanya nama kunci dan posisi.
- `muat(bawaan, berkas, awalan_lingkungan, skema, isi_lingkungan)` →
  objek `Konfigurasi`. Prioritas: **bawaan skema < `bawaan` < berkas (urut
  sesuai daftar) < lingkungan**. INI diratakan menjadi `bagian.kunci`.
  Lingkungan hanya dibaca untuk kunci yang **dikenal** (ada di skema, bawaan,
  atau berkas) dengan nama `awalan + KUNCI` (huruf besar, `.`/`-` → `_`),
  lewat `ambil_lingkungan`, jadi tidak pernah menyalin seluruh lingkungan.
  `isi_lingkungan` (kamus) menggantikan lingkungan OS untuk pengujian.
  Lingkungan hanya dibaca bila `awalan_lingkungan` diberikan (boleh `""`).
- Skema: nilai berupa nama tipe (`"teks"`, `"bulat"`, `"desimal"`,
  `"logika"`, `"daftar"`) atau kamus `{tipe, wajib, bawaan, rahasia,
  pilihan}`. Teks dikonversi ke tipe; logika menerima
  `benar/salah/ya/tidak/true/false/1/0/on/off`; daftar dipisah koma. Semua
  pelanggaran dikumpulkan lalu dinaikkan sekali sebagai `GalatKonfigurasi`.
  Kunci di luar skema tetap ada sebagai teks (skema tidak menolak kunci
  tambahan).
- Rahasia: kunci bertanda `rahasia` atau bernama mengandung `SANDI`,
  `PASSWORD`, `SECRET`, `RAHASIA`, `TOKEN`, `KEY`, `KUNCI` (tak peka huruf)
  disamarkan `***` di `cetak(konf)`, `repr`, `samarkan()` dan pesan galat.
  `ke_kamus()` mengembalikan nilai asli (pemanggil yang memutuskan).

### `pustaka/unicode.idk`

- `huruf_besar`, `huruf_kecil`, `lipat_huruf` (casefold): sama dengan
  `str.upper/lower/casefold` Python (data Unicode 16.0, Python 3.14) untuk
  U+0000–U+052F (Latin termasuk Extended-A/B dan IPA, Yunani/Koptik, Sirilik), termasuk
  pemetaan multi-karakter (`ß` → `SS`/`ss`, `İ` → `i̇`) dan sigma akhir
  (`ΣΑΣ` → `σας`) untuk huruf yang berdampingan. Karakter di luar rentang itu
  dibiarkan. Aturan locale (Turki) **tidak** diterapkan, sama seperti Python.
- `sama_tanpa_huruf(a, b)` = perbandingan `lipat_huruf`.
- `adalah_spasi(c)`, `pangkas(t)`, `pisah_spasi(t)`: himpunan spasi
  `str.isspace()` Python (29 code point, lengkap untuk seluruh Unicode).
- Builtin `huruf_besar`/`huruf_kecil`/`panjang` **tidak diubah**.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `pustaka/konfigurasi.idk` | baru: pengurai `.env`/INI, `muat`, `Konfigurasi`, `GalatKonfigurasi` |
| `pustaka/unicode.idk` | baru: tabel kasus dikodekan sebagai teks pasangan, diurai sekali saat impor |
| `src/` | tidak ada perubahan C |

## 5. Pengujian

- Golden `examples/pustaka_konfigurasi.idk`,
  `examples/pustaka_konfigurasi_berkas.idk` (berkas `.env`/INI dengan BOM +
  CRLF ditulis lalu dihapus), `examples/pustaka_unicode.idk` (multibahasa,
  emoji, combining mark). Nama berawalan `pustaka_`: contoh bernama
  `konfigurasi.idk` di `examples/` akan mengimpor dirinya sendiri karena
  direktori pengimpor dicari lebih dulu.
- `tests/galat/konfigurasi_*.idk` + `.err` untuk kutip tak tertutup, baris
  tanpa `=`, bagian INI ganda, kunci wajib hilang, tipe salah pada kunci
  rahasia (nilai tidak boleh bocor), berkas melebihi batas.
- Uji diferensial `python -I tools/unicode_tabel.py --periksa`: setiap code
  point U+0001–U+052F + 4000 teks acak (emoji, spasi Unicode, sigma) untuk
  keenam fungsi `unicode` terhadap Python 3.14 → 5325 kasus, 0 beda. Terhadap
  Python 3.12 (Unicode 15.0) hanya U+019B dan U+0264 berbeda (pemetaan huruf
  besar baru di Unicode 16). Skrip yang sama membangkitkan tabelnya.

## 6. Dampak dokumentasi & tooling

- [x] `website/panduan/07_PUSTAKA_STANDAR.md` (bagian `konfigurasi` dan `unicode`)
- [x] VS Code: tidak ada builtin/kata kunci baru, jadi `language-data.js` dan
      grammar tidak berubah
- [x] `CHANGELOG.md` + `docs/ROADMAP.md`
- [x] `docs/TESTING/{WINDOWS,MACOS,LINUX}.md` blok BERSAMA
- [x] `README.md` (daftar modul pustaka standar)
- [x] Berkas rencana ini → `selesai/`

## 7. Ditunda (dengan alasan)

- **TOML:** PR #145 (sudah merge) berisi `urai_toml` di dalam
  `pustaka/paket.idk`, terikat ke `GalatPaket` dan modul itu mengimpor
  `proses`/`pembaruan`. Langkah berikut: ekstrak ke `pustaka/toml.idk`
  (paket mengimpornya), lalu daftarkan `.toml` di `konfigurasi.muat` (saat
  ini `.toml` ditolak dengan pesan jelas). Tidak menulis pengurai kedua.
- **YAML:** ditunda. Spesifikasi YAML 1.2 besar (anchor/alias → bom
  "billion laughs", tag, multi-dokumen, gaya blok/alir); subset kecil yang
  tidak mengikuti spesifikasi akan membaca berkas nyata secara keliru tanpa
  galat. Pilihan aman berikutnya: subset terdokumentasi tanpa anchor/tag,
  atau binding pustaka C yang diaudit, dengan tes "YAML bomb".
- **NFC/NFD/NFKC/NFKD:** ditunda. Normalisasi yang benar butuh data
  dekomposisi seluruh UnicodeData, urutan kanonik (combining class),
  pengecualian komposisi dan algoritme Hangul, yaitu tabel puluhan ribu entri
  yang harus di-vendor dan diuji terhadap `NormalizationTest.txt`. Versi
  parsial akan diam-diam menghasilkan teks "hampir ternormalisasi".
- **Kategori Unicode, grapheme cluster, case mapping di luar
  Latin/Yunani/Sirilik:** butuh tabel penuh; sama alasannya.
- **Locale/i18n (langkah 3), MessagePack (langkah 6):** di luar V1.

## 8. Risiko & alternatif

- Tabel di `.idk` lebih lambat dari C, tetapi tidak menyentuh VM/GC dan dapat
  diganti native kelak tanpa mengubah API.
- Mengubah builtin `huruf_besar` menjadi Unicode-aware akan mengubah keluaran
  program lama secara diam-diam; karena itu modul terpisah.
