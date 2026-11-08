# Rencana: `ada_berkas()` untuk berkas terkunci (T-W9)

- **Status:** selesai
- **Target versi:** 1.6.0
- **Jenis:** perbaikan (builtin berkas)
- **Pemilik:** Rafa Okta Habibilah
- **Temuan asal:** T-W9 (`docs/TESTING/WINDOWS.md` §7, tingkat sedang, khas
  Windows), dilaporkan di uji ekstensif 1.4.0 sebagai WINDOWS-OS-03
  (`docs/TESTING/lampiran/WINDOWS-1.4.0-uji-ekstensif.md`)

## 1. Masalah

`ada_berkas()` menjawab `salah` untuk berkas yang jelas ada, asalkan sedang
dibuka program lain dengan `FileShare None`. Editor, antivirus, dan SQLite
dalam transaksi mengunci berkas seperti ini. Pada saat yang sama,
`jalur.ada()` dan `jalur.adalah_berkas()` menjawab `benar`, jadi dua fungsi
pustaka standar memberi jawaban berbeda untuk pertanyaan yang sama.

```indonesiaku
cetak(ada_berkas("kunci.txt"))   # salah saat kunci.txt dikunci PowerShell
impor jalur
cetak(jalur.ada("kunci.txt"))    # benar
```

Diverifikasi ulang di 1.6.0-rc.1 (Windows 11, build MSYS2 UCRT64): masih
terjadi.

## 2. Akar masalah

`nativeFnAdaBerkas()` (`src/native.c`) menjawab pertanyaan yang berbeda dari
namanya: "bisakah `fopen(path, "rb")` berhasil?". Pembukaan gagal karena
*sharing violation* (Windows) atau tanpa izin baca (`chmod 000`, POSIX),
padahal berkasnya ada.

Efek samping yang sama: **direktori bergantung OS**. `fopen` pada direktori
berhasil di glibc dan macOS tetapi gagal di Windows, jadi
`ada_berkas("folder")` memberi `benar` di Linux/macOS dan `salah` di Windows.

## 3. Semantik

`ada_berkas(path)` → `benar` bila ada sesuatu yang **bukan direktori** di
`path` (berkas biasa, atau entri lain seperti perangkat/FIFO di POSIX).

- Ditanyakan lewat metadata, sama dengan `jalur.ada()`: `jenisJalur()` di
  `src/sistem.c` (`GetFileAttributesExW` di Windows, `stat` di POSIX). Tidak
  ada pembukaan berkas.
- Direktori → `salah` di semua OS. Dipilih karena: (a) sesuai nama dan
  dokumentasi ("apakah berkas ada"), (b) sama dengan perilaku Windows yang
  sudah ada, (c) pemakai di repo (`pustaka/templat.idk`, contoh todo) memakai
  `ada_berkas` tepat sebelum `baca_semua`, yang memang butuh berkas. Untuk
  berkas **atau** direktori sudah ada `jalur.ada()` (setara `os.path.exists`).
- Galat metadata (mis. tanpa izin pada direktori induk) tetap `salah`, bukan
  galat: `ada_berkas()` tidak pernah menaikkan galat untuk jalur, seperti
  sebelumnya. Argumen salah tipe/jumlah tetap galat seperti sebelumnya.
- Mode `--sandbox`: tidak berubah. Baik `sistemBukaBerkas()` (jalur lama)
  maupun `_info_jalur` (jalur `jalur.ada()`) tidak dibatasi sandbox, jadi
  tidak ada pembatasan yang terlewati.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/sistem.c` | `sistemAdaBerkas()` baru di atas `jenisJalur()` yang sudah ada |
| `src/sistem.h` | deklarasi `sistemAdaBerkas()` |
| `src/native.c` | `nativeFnAdaBerkas()` memanggil `sistemAdaBerkas()` alih-alih `fopen` |

Tidak ada alokasi objek baru, tidak menyentuh stack VM/GC.

## 5. Pengujian

- **Golden lintas-OS:** `examples/ada_berkas.idk` + `tests/expected/ada_berkas.out`
  — berkas ada, direktori `salah` (dan `jalur.ada` `benar`), jalur tidak ada,
  jalur di dalam berkas, teks kosong, setelah dihapus.
- **Khas Windows:** `tests/test_ada_berkas_terkunci.ps1`, didaftarkan di
  `tests/test_all.ps1` — tanpa kunci, kunci baca `FileShare None`, kunci
  baca-tulis `FileShare None`, setelah dihapus.
- **Sabotase:** kode C dikembalikan ke versi `fopen` → tes Windows merah 2/4
  (kedua kasus terkunci memberi `salah benar benar`). Baris `direktori:` di
  golden semestinya juga merah dengan kode lama di Linux/macOS (`fopen` pada
  direktori berhasil di glibc/macOS), tetapi itu **belum dijalankan** di
  Linux/macOS — perbaikan ini dikerjakan dan diuji di Windows; CI yang
  membuktikannya.
- **Tidak diuji otomatis:** `chmod 000` di POSIX. Hasilnya bergantung pada
  pengguna yang menjalankan tes (root mengabaikan izin baca), sehingga tesnya
  bisa lolos palsu.
- Rencana uji lintas-OS: PUS-06 (bagian bersama ketiga berkas
  `docs/TESTING/`) dan WIN-17 (khas Windows).

## 6. Dampak dokumentasi & tooling

- [x] `website/panduan/07_PUSTAKA_STANDAR.md` — baris `ada_berkas`
- [x] `CHANGELOG.md` — ditandai ⚠️ karena perilaku direktori di Linux/macOS berubah
- [x] `docs/TESTING/WINDOWS.md` — T-W9 ✅, WIN-17; PUS-06 di ketiga berkas
- [ ] `website/panduan/03_PANDUAN_SINTAKS.md` — tidak relevan (bukan sintaks)
- [ ] Editor VS Code — tidak relevan (nama builtin tidak berubah)

## 7. Risiko & alternatif

- **Program lama yang memakai `ada_berkas(dir)` di Linux/macOS** kini
  mendapat `salah`. Tidak ada pemakai seperti itu di `examples/` maupun
  `pustaka/` (diperiksa dengan grep). Dicatat ⚠️ di CHANGELOG.
- *Alternatif: `ada_berkas` = `jalur.ada` (direktori `benar` di semua OS).*
  Ditolak: menduplikasi `jalur.ada()` dan bertentangan dengan nama serta
  dokumentasi; perilaku Windows yang berubah malah lebih banyak.
- *Alternatif: tetap `fopen`, tapi coba `stat` bila gagal.* Ditolak: dua
  jalur kode untuk satu pertanyaan, dan direktori tetap bergantung OS.
