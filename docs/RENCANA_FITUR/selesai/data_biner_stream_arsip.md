# Rencana Fitur: Data biner, kompresi dan arsip (Fase 23, V1)

- **Status:** selesai (V1; sisa Fase 23 tercatat di §7 dan docs/ROADMAP.md)
- **Target versi:** rilis minor berikutnya (Fase 23 penuh: `v1.18.0`)
- **Pemilik:** Teguh Rijanandi

Rencana ini mencakup **V1** Fase 23 di `docs/ROADMAP.md`: langkah 1 (audit +
rencana), langkah 5 (gzip/zlib + TAR + ZIP dengan ekstraksi aman) dan
langkah 7 (tes). Tipe `bita` sendiri, API stream umum, dan integrasi HTTP/SFTP/
BLOB (langkah 2–4, 6) **ditunda** — lihat §7.

## 1. Masalah

Hari ini Indonesiaku tidak bisa:

- mengompresi atau membuka data gzip/zlib (`Content-Encoding: gzip`,
  berkas `.gz`, payload API);
- membaca, mengekstrak, atau membuat arsip `.tar`, `.tar.gz`, `.zip`
  (cadangan, unduhan paket, unggahan pengguna);
- menulis bita yang bukan UTF-8 sah ke berkas: `tulis_semua()` hanya menerima
  teks dan `baca_semua()` hanya mengembalikan teks.

Manajer paket (Fase 14, PR #145) sengaja belum mendukung sumber tarball
karena belum ada ekstraktor yang aman. Ekstraksi arsip dari sumber tak
tepercaya adalah kelas bug keamanan yang terkenal (zip slip / CVE-2007-4559
`tarfile`, symlink keluar direktori, bom dekompresi 42.zip), jadi
ekstraktornya harus aman **secara bawaan**, bukan lewat opsi.

### Audit dukungan biner yang ada

| Bagian | Keadaan |
|---|---|
| `ObjString` | menyimpan `length` bita + `charCount`; boleh memuat NUL dan bita sembarang, tetapi semantik bahasanya karakter (UTF-8). Bukan wadah biner yang jujur. |
| Konvensi biner | **daftar bulat 0..255**: `pustaka/bita.idk` (`ke_bita`, `ke_teks` ketat RFC 3629, heksa), `acak_bita`, `soket_baca_bita`, `soket_tulis(daftar)`, BLOB `db_*`, `hmac_sha256`. |
| Berkas | `buka_berkas` mode `"b"/"t"/"s"` → `rb/wb/ab` (byte-exact), tetapi `baca`/`baca_semua` memberi teks dan `tulis`/`tulis_semua` menerima teks saja. |
| `pustaka/base64.idk` | Base64 di atas daftar bita. |
| Kompresi / arsip | tidak ada. |

Keputusan V1: **tetap memakai konvensi daftar bita** (tidak ada tipe baru,
jadi tidak ada perubahan `Value`/GC/`formatValue`), dan memindahkan kerja
berat (kompresi, parsing arsip, I/O) ke C sehingga daftar hanya muncul di
batas API. Tipe `bita` yang efisien tetap langkah 2 Fase 23.

## 2. Sintaks usulan

Tidak ada sintaks baru; tiga modul pustaka dengan nama Indonesia.

```indonesiaku
impor kompres
impor arsip
impor bita

variabel padat = kompres.gzip("halo " * 100)          # daftar bita
cetak(bita.ke_teks(kompres.buka_gzip(padat)))       # batas bawaan 64 MiB

arsip.buat("cadangan.tar.gz", ["data", ["info.txt", "versi 1"]])
untuk e dalam arsip.daftar_isi("cadangan.tar.gz") {
    cetak(e["nama"], e["jenis"], e["ukuran"])
}
arsip.ekstrak("unggahan.zip", "hasil/", batas_ukuran=10485760)

bita.tulis_berkas("logo.png", data)                   # bita mentah ke berkas
```

## 3. Semantik

**`kompres`** — `gzip`/`buka_gzip`, `zlib`/`buka_zlib`, `deflate`/
`buka_deflate` (mentah), `gzip_berkas`/`buka_gzip_berkas` (berkas → berkas per
64 KB), `crc32`, `adler32`. Masukan teks (bita UTF-8-nya) atau daftar bita;
keluaran daftar bita. `tingkat` 0..9 (bawaan 6). Keluaran gzip deterministik
(MTIME 0, OS 255). `buka_gzip` menerima multi-anggota dan memeriksa CRC-32 +
ISIZE tiap anggota; data sisa setelah akhir aliran ditolak. **Setiap dekompresi
dibatasi** `batas` (bawaan 64 MiB) dan diperiksa selagi membuka. Galat format/
batas → `GalatKompres < GalatNilai`; argumen salah → `GalatTipe`; berkas →
`GalatOS`.

**`arsip`** — format dikenali dari **isi** (PK.., 1f 8b, header TAR dengan
checksum sah), bukan ekstensi.

- `daftar_isi(jalur, batas_entri, batas_rasio)` → daftar kamus `nama/jenis/ukuran/
  terkompres`, nama apa adanya (untuk memeriksa arsip mencurigakan).
- `ekstrak(jalur, tujuan, batas_ukuran=1 GiB, batas_entri=10000,
  batas_rasio=200, lewati_tautan=salah)` → daftar nama yang ditulis.
  **Dua lintasan:** lintasan 1 membaca dan mendekompresi seluruh arsip
  (CRC diperiksa) tanpa menulis apa pun, termasuk memeriksa tujuan yang sudah
  ada; baru lintasan 2 menulis. Arsip yang ditolak tidak meninggalkan apa pun.
  Ditolak:
  - nama kosong, absolut, berisi komponen `..`, `\`, `:`, karakter kendali,
    bukan UTF-8 sah, > 4096 bita; di Windows juga nama perangkat (`CON`,
    `NUL`, `COM1`...) dan akhiran `.`/spasi;
  - tautan simbolik / keras (atau **dilewati** dengan `lewati_tautan=benar`);
    entri khusus lain (perangkat, FIFO, sparse) selalu ditolak; modul ini
    **tidak pernah membuat tautan**;
  - total ukuran > `batas_ukuran`, entri > `batas_entri` (header PAX/GNU ikut
    dihitung), rasio > `batas_rasio` untuk entri/arsip di atas 1 MiB. Untuk
    `.tar.gz` rasio diperiksa pada aliran gzip itu sendiri selagi membuka; untuk
    ZIP keluaran yang melebihi ukuran yang dinyatakan direktori pusat dihentikan;
  - komponen di bawah tujuan yang **sudah ada dan berupa tautan/junction**
    (diperiksa dengan `lstat`/`GetFileAttributesW`, tidak diikuti); berkas
    akhir dibuka dengan `O_NOFOLLOW` di POSIX. Tujuan itu sendiri boleh tautan
    (mis. `/tmp` di macOS).
  Berkas yang sudah ada ditimpa. chmod/mtime dari arsip tidak dipulihkan.
- `buat(jalur, isi, format, tingkat=6, dasar)` → jumlah entri. `isi`: jalur
  teks (direktori ditelusuri rekursif, tautan dilewati) atau `[nama, data]`.
  Format dari ekstensi (`.tar`, `.tar.gz`/`.tgz`, `.zip`) kecuali diberikan.
  Nama melewati pemeriksaan yang sama dengan `ekstrak`. TAR ditulis sebagai
  ustar + PAX `path` untuk nama panjang + base-256 untuk berkas ≥ 8 GiB;
  `.tar.gz` dialirkan, tidak ditampung di RAM. Gagal → berkas setengah jadi
  dihapus.

**`bita`** (tambahan) — `baca_berkas(jalur, batas=256 MiB)` dan
`tulis_berkas(jalur, data)` untuk bita mentah.

Semua galat dapat ditangkap `coba/kecuali`. Tidak ada yang ditolak mode
`--sandbox` selain yang memang sudah ditolak (penelusuran direktori lewat
`isi_direktori`).

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/vendor/miniz/` | miniz 3.1.2 tanpa modifikasi + `LICENSE` + `PROVENANCE.md` (SHA-256 arsip dan berkas), `-text` di `.gitattributes` |
| `src/kompres.{h,c}` | pembungkus gzip (RFC 1952) di atas deflate mentah miniz; aliran inflate (zlib/gzip/mentah) dan deflate ke `FILE*`; natives `_kompres`, `_dekompres`, `_kompres_berkas`, `_dekompres_berkas`, `_crc32`, `_adler32`, `_baca_berkas_bita`, `_tulis_berkas_bita` |
| `src/arsip.{h,c}` | pembersih nama, jalan tujuan tanpa tautan, pembaca TAR (ustar, GNU `L`/`K`, PAX `x`/`g`, base-256), pembaca ZIP (miniz), natives `_arsip_daftar`, `_arsip_ekstrak`, `_arsip_jenis` |
| `src/arsip_buat.c` | penulis TAR/TAR.GZ dan ZIP, native `_arsip_buat` |
| `src/sistem.{h,c}` | `sistemJenisTanpaTautan`, `sistemBuatSatuDirektori`, `sistemBukaTulisTanpaTautan` (jalur panjang Windows lewat `keLebarJalur`) |
| `src/vm.c` | registrasi native |
| `pustaka/kompres.idk`, `pustaka/arsip.idk`, `pustaka/bita.idk` | API publik + validasi argumen + `GalatKompres`/`GalatArsip` |
| `Makefile`, `.github/workflows/test.yml`, `tests/fuzz_compile.c`, `website/panduan/02_INSTALASI.md` | daftar sumber baru |

**Pilihan pustaka: miniz** (MIT, satu berkas C, dirawat, rilis 3.1.2 2026-07,
dipakai luas; mendukung deflate/inflate bertahap, zlib, CRC/Adler, dan ZIP
termasuk ZIP64). Alternatif: zlib (lebih kanonik tetapi tanpa ZIP; perlu
minizip lagi), libarchive (lengkap tetapi besar, banyak dependensi, dan
lisensi/footprint tidak cocok untuk biner tunggal 12 MB), menulis deflate
sendiri (dilarang: risiko keamanan dan kebenaran). TAR ditulis di C sendiri
karena formatnya sederhana dan keputusan keamanannya (nama, jenis entri,
tautan) memang harus milik kita; gzip juga dibungkus sendiri karena API zlib
miniz tidak mengenal pembungkus gzip.

**GC-safety:** natives tidak memegang objek VM selama kerja berat; daftar
hasil dibangun di akhir dan didorong ke stack (`kompresDaftarBita`,
`catatDaftar`, `catatEkstrak`); `_arsip_buat` tidak mengalokasikan objek VM
sama sekali sebelum hasil. Teks masukan dibaca langsung dari `ObjString`
yang berada di argumen (sudah rooted).

## 5. Pengujian

- Golden `examples/pustaka_kompres.idk`: roundtrip gzip/zlib/deflate untuk
  teks, bita acak, NUL di tengah, data kosong, tingkat 0/1/6/9; gzip dari
  Python (kepala FNAME), zlib Python, multi-anggota; crc32/adler32 = nilai
  Python dan berantai; rusak, terpotong, data sisa, bukan gzip, Adler salah,
  bom dengan batas; berkas → berkas, berkas tujuan dihapus bila gagal.
- Golden `examples/pustaka_arsip.idk`: buat/daftar/ekstrak TAR, TAR.GZ, ZIP
  (roundtrip byte-exact), timpa berkas; tar.gz Python (PAX nama panjang,
  biner); ditolak: zip slip TAR/ZIP, absolut, `\`, `C:`, symlink TAR/ZIP,
  hardlink, ZIP dengan CRC rusak — semuanya **tanpa membuat tujuan**;
  `lewati_tautan`; bom rasio ZIP dan TAR.GZ (2 MiB → ~2 KB) + izin dengan
  `batas_rasio`; batas ukuran/entri; arsip terpotong; bukan arsip; nama buruk
  pada `buat`.
- `tests/galat/arsip_zip_slip`, `tests/galat/kompres_bom`.
- `tests/test_arsip_tautan.sh` (POSIX `ln -s`) dan `.ps1` (junction Windows):
  tautan direktori/berkas di tujuan ditolak sebelum menulis, tujuan yang
  sendiri tautan diizinkan, `buat` melewati tautan sumber.
- Interop manual: arsip buatan Indonesiaku dibaca `tarfile`/`zipfile`
  Python dan GNU `tar`; arsip Python dibaca Indonesiaku.
- Build `-DDEBUG_STRESS_GC -fsanitize=address,undefined` (Ubuntu/WSL) atas
  golden baru, `tests/galat` baru, dan skrip tautan.

## 6. Dampak dokumentasi & tooling

- [x] `website/panduan/07_PUSTAKA_STANDAR.md` (modul `kompres`, `arsip`,
      tambahan `bita`)
- [x] `editors/vscode-indonesiaku/` — tidak ada builtin publik baru (semua
      native berawalan `_`), jadi tidak berubah
- [x] `CHANGELOG.md` + `docs/ROADMAP.md`
- [x] `docs/TESTING/{WINDOWS,MACOS,LINUX}.md` blok BERSAMA
- [x] Berkas rencana ini → `docs/RENCANA_FITUR/selesai/`

## 7. Risiko & alternatif

- **Daftar bita mahal** (satu `Value` per bita): data puluhan MB di memori
  lambat. Mitigasi V1: jalur berkas → berkas (`gzip_berkas`, `arsip.*`)
  tidak pernah melewati daftar. Solusi sebenarnya: tipe `bita` (langkah 2).
- **TOCTOU di Windows:** `_wfopen` tidak punya `O_NOFOLLOW`; tujuan diperiksa
  dengan `GetFileAttributesW` sesaat sebelum dibuka. Proses lain yang menaruh
  junction di antara keduanya tidak dicegah. Di POSIX direktori perantara juga
  dicek lalu dipakai (bukan `openat` berantai). Dokumentasikan: jangan
  mengekstrak ke direktori yang dapat ditulis pihak tak tepercaya secara
  bersamaan.
- **Ditunda ke versi berikutnya** (ROADMAP Fase 23): tipe `bita` dan
  buffer (langkah 2), API stream umum (3), endian packing/hex/checksum
  streaming (4, sebagian ada di `bita`/`kompres`), integrasi HTTP/SFTP/BLOB
  (6), pemulihan chmod/mtime opsional, ekstraksi satu entri ke memori,
  arsip dari/ke memori, ZIP terenkripsi (sengaja tidak).
