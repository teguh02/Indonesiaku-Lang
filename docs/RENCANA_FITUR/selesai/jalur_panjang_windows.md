# Rencana: jalur lebih dari 260 karakter di Windows (T-W10)

- **Status:** selesai
- **Target versi:** 1.6.0
- **Jenis:** perbaikan (lapisan sistem berkas Windows)
- **Pemilik:** Rafa Okta Habibilah
- **Temuan asal:** T-W10 (`docs/TESTING/WINDOWS.md` §7, sedang, khas Windows),
  dilaporkan di uji ekstensif 1.4.0 sebagai WINDOWS-OS-04
  (`docs/TESTING/lampiran/WINDOWS-1.4.0-uji-ekstensif.md`)

## 1. Masalah

Pada jalur yang lebih panjang dari 260 karakter, `ada_berkas` memberi `salah`
dan `baca_semua` serta `isi_direktori` gagal, walaupun `LongPathsEnabled=1`.

Diverifikasi ulang di 1.6.0-rc.1 (Windows 11, `LongPathsEnabled=1`, build
MSYS2 UCRT64) dengan jalur 412 karakter: `buat_direktori` gagal mulai level
ke-3 (bentuk absolut ±300 karakter), lalu `tulis_semua`, `ada_berkas`,
`baca_semua`, `isi_direktori`, dan `hapus_berkas` gagal semua.

## 2. Akar masalah

API berkas Win32 berhenti di `MAX_PATH` (260; `CreateDirectoryW` sudah di
248) **kecuali** dua syarat terpenuhi bersamaan: registri `LongPathsEnabled=1`
**dan** manifest biner menyatakan `longPathAware`. `indk.exe` tidak punya
manifest sama sekali, jadi setelan registri tidak berpengaruh.

## 3. Perbaikan

`keLebarJalur()` di `src/sistem.c`: `keLebar()` (UTF-8 → UTF-16) untuk jalur
sistem berkas, ditambah:

1. Bentuk absolut dihitung dengan `GetFullPathNameW` — relatif terhadap
   direktori kerja, `/` → `\`, `.` dan `..` diselesaikan. Yang menentukan
   adalah panjang **absolut**: jalur relatif pendek di bawah direktori kerja
   yang panjang juga melewati batas.
2. Bila panjangnya ≥ 248 (`BATAS_JALUR_PENDEK`), dikembalikan
   `\\?\C:\...` atau `\\?\UNC\server\share\...`. Awalan ini membuat API
   menerima jalur sampai ±32.767 karakter tanpa registri maupun manifest
   (perilaku terdokumentasi Win32). Normalisasi harus sudah selesai di
   langkah 1, karena `\\?\` mematikannya.
3. Jalur yang lebih pendek diteruskan **persis seperti dulu**. Ini disengaja:
   `\\?\` juga mematikan aturan lama seperti nama perangkat (`NUL`, `CON`) dan
   pemangkasan titik di akhir; menerapkannya ke semua jalur akan mengubah
   perilaku program yang tidak ada hubungannya dengan jalur panjang.
4. Jalur yang sudah berawalan `\\?\` atau `\\.\` tidak disentuh.

Dipakai di enam titik: `sistemBukaBerkas` (semua pembukaan berkas, termasuk
skrip dan `impor`), `sistemHapusBerkas`, `jenisJalur` (`ada_berkas`,
`jalur.ada`, `hapus_direktori`), pola `isi_direktori`, `buat_direktori`,
`hapus_direktori`.

**Sengaja tidak diubah:**

- Nama variabel lingkungan dan baris perintah proses anak (bukan jalur).
- `_jalur_nyata()`: `_wfullpath` sudah sanggup jalur panjang, dan mengubahnya
  akan membuat hasilnya berawalan `\\?\` sehingga perbandingan prefiks akar
  di penyaji berkas statis tidak konsisten.
- `src/indk_api.c` (API embedding C) masih memakai `fopen` biasa. Itu jalur
  milik program penanam, bukan pengguna `indk`; dicatat sebagai sisa.
- Direktori kerja yang panjang (`SetCurrentDirectoryW`) dan SQLite punya
  batasnya sendiri; di luar cakupan.

## 4. Pengujian

- `tests/test_jalur_panjang.ps1`, didaftarkan di `tests/test_all.ps1`.
  Satu skrip di folder > 260 karakter yang dijalankan langsung dan meng-`impor`
  modul di sebelahnya, lalu: `buat_direktori` bersarang sampai > 260,
  `tulis_semua`, `tambah_berkas`, `baca_semua`, `ada_berkas`, `jalur.ada`,
  `jalur.adalah_direktori`, `isi_direktori`, `buka_berkas`, jalur ber-`..`,
  garis miring terbalik, `hapus_berkas`, `hapus_direktori`, dan jalur relatif
  pendek yang bentuk absolutnya > 260.
- **Sabotase:** seluruh `sistem.c` dikembalikan → merah (skrip pun tidak bisa
  dibuka, kode 74). Lalu **tiap titik panggil satu per satu** dikembalikan ke
  `keLebar()` → keenamnya merah.
- `tests/test_all.ps1` penuh tetap hijau: jalur pendek tidak berubah.
- **Yang belum diuji:** mesin dengan `LongPathsEnabled=0`. Mesin uji
  bernilai 1; bahwa `\\?\` bekerja tanpa setelan itu adalah perilaku Win32
  yang terdokumentasi, bukan hasil pengukuran di sini. Jalur UNC
  (`\\server\share`) juga belum diuji karena tidak ada share jaringan.
- Linux/macOS: kode di balik `#ifdef _WIN32`, tidak ada perubahan.

## 5. Dampak dokumentasi

- [x] `CHANGELOG.md`
- [x] `website/panduan/07_PUSTAKA_STANDAR.md` §1.6 — catatan jalur panjang
- [x] `docs/TESTING/WINDOWS.md` — T-W10 ✅, kasus WIN-18. Khas Windows, jadi
      tidak ada kasus di bagian bersama.

## 6. Risiko & alternatif

- *Alternatif: manifest `longPathAware` lewat berkas `.rc` + `windres`.*
  Ditolak: menuntut perubahan Makefile, CI, dan installer, dan tetap tidak
  bekerja bila pengguna belum menyalakan `LongPathsEnabled` (bawaan Windows:
  mati).
- *Alternatif: selalu memberi awalan `\\?\`.* Ditolak: mengubah perilaku
  jalur pendek (nama perangkat, titik di akhir), lihat §3 butir 3.
- Biaya: satu panggilan `GetFullPathNameW` tambahan per operasi berkas
  (tanpa alokasi tambahan bila jalurnya pendek). Diabaikan dibanding I/O itu sendiri.
