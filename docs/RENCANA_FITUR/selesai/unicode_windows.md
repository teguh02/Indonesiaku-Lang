# Rencana Fitur: Unicode di Windows (jalur, BOM, konsol)

- **Status:** selesai
- **Target versi:** 1.4.1
- **Pemilik:** Teguh Rijanandi

## 1. Masalah

Uji paket rilis 1.4.0 di Windows (`docs/testing/WINDOWS.md` bagian 7) menemukan
tiga cacat Unicode. Semua uji bahasa lulus; ketiganya ada di batas antara
interpreter dan sistem operasi.

- **T-W1 (tinggi).** `indk 日本\s.idk` gagal dengan
  `Tidak dapat membuka file "??/s.idk".` (kode 74), dan
  `tulis_semua("keluaran_☕.txt", "x")` membuat berkas bernama
  `keluaran_â˜•.txt`. Penyebabnya: `main()` menerima `argv` dalam code page
  ANSI, lalu berkas dibuka dengan `fopen`, yang menafsirkan byte UTF-8 sebagai
  ANSI. Pengguna dengan nama folder non-Latin tidak bisa menjalankan skrip
  dari folder pribadinya.
- **T-W2 (sedang).** Berkas sumber UTF-8 yang diawali BOM (`EF BB BF`, dari
  Notepad lama atau "UTF-8 with BOM" di VS Code) ditolak dengan
  `[baris 1] Kesalahan: Karakter tidak dikenal.` (kode 65). Python
  menerimanya. Cacat ini ada di semua OS.
- **T-W3 (sedang).** `cetak("☕")` tampil sebagai `Γÿò` di konsol code page
  437/850. Setelah `chcp 65001` tampil benar.

## 2. Sintaks usulan

Tidak ada sintaks baru. Perilaku yang diinginkan:

```indonesiaku
# disimpan sebagai D:\日本\s.idk, dengan atau tanpa BOM
tulis_semua("keluaran_☕.txt", "x")   # nama berkas di Explorer: keluaran_☕.txt
cetak("2 ☕ GARUDA")                 # terbaca benar di cmd/PowerShell
```

## 3. Semantik

- **Di dalam interpreter, teks selalu UTF-8**, termasuk jalur, `argumen`, dan
  variabel lingkungan. Di Windows, konversi ke UTF-16 hanya terjadi tepat di
  pemanggilan API OS.
- `argumen` di Windows diambil dari `GetCommandLineW` + `CommandLineToArgvW`
  dan diubah ke UTF-8 sebelum `main()` memproses opsi. Di POSIX tidak ada
  perubahan: argv sudah berupa byte apa adanya.
- **BOM UTF-8** di awal berkas sumber dilewati, baik oleh skrip utama
  (`readFile`) maupun oleh modul yang diimpor (`readModuleSource`), sebelum
  `normalizeNewlines`. Kolom pada baris 1 dihitung tanpa BOM. BOM di tempat
  lain tetap merupakan karakter tidak dikenal. Pembacaan data
  (`baca_semua`, `buka_berkas`) **tidak** membuang BOM, sama seperti
  `open(..., encoding="utf-8")` di Python.
- **Konsol:** bila stdout/stdin Windows terhubung ke konsol, code page
  output dan input diubah ke UTF-8 (65001) saat mulai, lalu dikembalikan ke
  nilai semula saat keluar (`atexit`), supaya jendela cmd pengguna tidak
  tertinggal di code page lain. Bila keluaran dialihkan ke berkas/pipa, tidak
  ada perubahan: byte UTF-8 ditulis apa adanya seperti sebelumnya.
- Tidak ada pesan galat baru. Berkas yang tetap tidak bisa dibuka memberi
  pesan yang sama, kini dengan jalur yang terbaca benar.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/sistem.c`/`.h` | pembungkus portabel: `sistemBukaBerkas` (fopen/_wfopen), `sistemHapusBerkas` (remove/_wremove), `sistemAmbilLingkungan` (getenv/GetEnvironmentVariableW, hasil malloc), `sistemSiapkanWindows` (argv UTF-8 + code page konsol); `_jalur_nyata` memakai `_wfullpath` |
| `src/main.c` | panggil `sistemSiapkanWindows(&argc, &argv)` paling awal; `readFile` dan `rapikan --tulis` lewat `sistemBukaBerkas` |
| `src/vm.c` | `readModuleSource`, `fileExists`, `INDK_PATH` lewat pembungkus |
| `src/native.c` | `buka_berkas`, `baca_semua`, `tulis_semua`, `tambah_berkas`, `ada_berkas`, `hapus_berkas` lewat pembungkus |
| `src/common.h` | `stripUtf8Bom()` di samping `normalizeNewlines` |

Pembungkus diletakkan di `sistem.c` (bukan berkas `.c` baru) karena berkas itu
sudah memuat `windows.h` dan konversi `keLebar`/`keUtf8`, dan sudah ada di
semua daftar build (Makefile, CI, fuzzer). `windows.h` tidak boleh masuk ke
header bersama: `winnt.h` mendefinisikan `TokenType`, yang bentrok dengan
`scanner.h`.

Tidak ada alokasi heap GC baru; `argv` hasil konversi dialokasikan dengan
`malloc` dan hidup sepanjang proses.

## 5. Pengujian

- Golden `examples/bom_utf8.idk` (diawali BOM) + `tests/expected/bom_utf8.out`.
  Berkas ber-BOM dilindungi dari normalisasi git dengan `.gitattributes`
  (`-text`) supaya byte BOM dan LF tidak diubah `autocrlf`.
- Golden `examples/berkas_unicode.idk`: menulis, membaca, mengecek, dan
  menghapus berkas `keluaran_☕_日本.txt`, lalu memeriksa namanya lewat
  `isi_direktori` (jalur W yang sudah benar). Di Windows lama, ini gagal.
- Galat `tests/galat/bom_di_tengah.idk`: BOM yang tidak di awal tetap ditolak.
- Uji integrasi jalur skrip non-ASCII (`tests/test_unicode_jalur.{sh,ps1}`):
  salin skrip + modul ke direktori `日本 ☕/`, jalankan dari jalur itu, impor
  modul dari direktori yang sama, dan teruskan argumen non-ASCII.
- Uji manual di konsol Windows asli (WIN-08–WIN-11): `chcp 437`, lalu jalankan
  SIN-03; cek `chcp` setelah keluar kembali ke 437.
- `-DDEBUG_STRESS_GC -fsanitize=address,undefined` atas golden dan galat.

## 6. Dampak dokumentasi & tooling

- [x] `website/panduan/02_INSTALASI.md` — "Pemecahan Masalah" Windows: gejala
      1.4.0 + solusi sementara (tidak ada sintaks baru)
- [x] Editor: tidak ada kata kunci/builtin baru
- [x] `CHANGELOG.md`
- [x] `AGENTS.md`: jalur berkas wajib lewat pembungkus `sistem*`
- [x] Uji ulang WIN-08–WIN-11 di konsol Windows asli (code page 437): semua
      lulus, dan code page kembali ke 437 setelah keluar. Status ✅ di
      `docs/testing/WINDOWS.md` dibalik di PR #38 (berkas itu belum ada di
      `main`).
- [x] Berkas rencana ini: status → `selesai`, lalu `git mv` ke
      `docs/RENCANA_FITUR/selesai/`

## 7. Risiko & alternatif

- **`wmain`/`-municode`** akan memberi argv lebar langsung, tetapi mengubah
  flag build di Makefile, CI, dan panduan instalasi. `CommandLineToArgvW`
  tidak memerlukan perubahan build (shell32 sudah tertaut bawaan MinGW).
  Aturan pemisahannya hampir sama dengan CRT; perbedaannya hanya pada kasus
  kutip ganda berurutan yang sangat jarang.
- **Mengubah code page konsol** memengaruhi jendela konsol bersama. Karena itu
  nilainya dikembalikan saat keluar. Program yang mati karena crash tidak
  mengembalikannya. Ini sama dengan setelan `chcp 65001` manual, yang tidak
  berbahaya.
- **`SetConsoleCP(CP_UTF8)` untuk masukan:** `ReadFile` dari konsol dengan
  code page 65001 di Windows 10 lama mengembalikan NUL untuk karakter
  non-ASCII. Masukan non-ASCII sebelumnya juga sudah rusak (byte code page
  OEM, bukan UTF-8), jadi ini bukan regresi. Di Windows 11 masukan menjadi
  benar.
- `rapikan --tulis` membuang BOM dari berkas yang dirapikan, karena berkas
  dibaca lewat `readFile`. Ini disengaja: keluaran formatter adalah UTF-8
  tanpa BOM.
- Di luar cakupan: `INDK_CA_BUNDLE` dibuka oleh mbedtls (`fopen` vendor), jadi
  jalur CA non-ASCII di Windows masih belum didukung. Ini dicatat sebagai
  tindak lanjut.
