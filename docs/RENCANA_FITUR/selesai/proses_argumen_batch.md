# Rencana Fitur: argumen aman untuk berkas `.bat`/`.cmd` di Windows (T-W6)

- **Status:** selesai
- **Target versi:** 1.6.2
- **Pemilik:** pemeliharaan `src/sistem.c`

## 1. Masalah

`proses.jalankan` dijanjikan berjalan "tanpa shell, jadi tidak ada injeksi
lewat `;` atau `|`" (panduan 07 §2.17). Di Windows janji itu gugur untuk
berkas batch. `CreateProcessW` selalu menjalankan `.bat`/`.cmd` lewat
`cmd.exe`, dan cmd.exe tidak mengurai baris perintah dengan aturan
`CommandLineToArgvW` yang dipakai `kutipArgumen`:

- argumen tanpa spasi tidak dikutip, sehingga `&`, `|`, `<`, `>`, dan `^`
  di dalamnya dibaca cmd.exe sebagai operator perintah;
- `%VAR%` (dan `!VAR!` bila *delayed expansion* aktif) diperluas walau
  berada di dalam kutip;
- `"` di dalam argumen bisa menutup kutip lebih awal.

Ditemukan pada uji ekstensif 1.4.0 dan masih kambuh di rilis 1.6.1.

## 2. Perubahan

Hanya `nativeFnJalankanProses` versi Windows (`src/sistem.c`):

- `adalahBerkasBatch(program)`: ekstensi `.bat` atau `.cmd`, tanpa beda
  huruf besar/kecil, setelah titik dan spasi di akhir dibuang (Windows
  membuangnya juga).
- Bila program adalah berkas batch, setiap bagian perintah (program dan
  argumen) yang memuat `"`, `%`, `!`, CR, atau LF ditolak dengan
  `GalatNilai`. Karakter itu tidak bisa diteruskan ke cmd.exe dengan aman
  di dalam kutip.
- Untuk berkas batch, `kutipArgumen(..., paksa = benar)` selalu mengutip,
  sehingga operator cmd.exe di dalam argumen menjadi teks biasa.
- Program yang bukan batch (`.exe`) tidak berubah sama sekali.

Arahnya sama dengan perbaikan CVE-2024-24576 di pustaka standar Rust.
Rust juga memilih menolak argumen yang tidak bisa di-escape dengan aman,
alih-alih mencoba meng-escape `%` secara heuristik.

## 3. Rencana uji

- `tests/test_proses_batch.ps1`, dipanggil `tests/test_all.ps1`. Uji ini
  mencoba operator di argumen tanpa spasi, `|`, nama berkas huruf besar,
  `.cmd`, `"`, `%VAR%`, `!VAR!`, dan baris baru. Setiap muatan mencoba
  membuat berkas penanda, dan berkas itu tidak boleh pernah ada. Argumen
  biasa harus tetap sampai utuh.
- Sabotase: dengan biner 1.6.1 uji ini merah (6 kasus gagal, berkas penanda
  terbuat).
- Kasus WIN-21 di `docs/TESTING/WINDOWS.md` bagian 6.
- POSIX tidak terpengaruh: kodenya berada di cabang `_WIN32`.
