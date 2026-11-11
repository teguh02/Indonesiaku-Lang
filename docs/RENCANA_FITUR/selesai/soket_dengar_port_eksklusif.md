# Rencana Fitur: satu port untuk satu server di Windows (T-W7)

- **Status:** selesai
- **Target versi:** 1.6.2
- **Pemilik:** pemeliharaan `src/native.c`

## 1. Masalah

`soket_dengar` memasang `SO_REUSEADDR` di semua OS agar server bisa segera
dijalankan ulang meski koneksi lama masih TIME_WAIT. Di Linux/macOS opsi itu
tetap menolak dua soket LISTEN pada port yang sama. Di Winsock artinya
berbeda: soket lain **boleh** mengikat port yang sedang dipakai. Akibatnya
`soket_dengar` kedua pada port yang sama berhasil tanpa galat, dua server
diam-diam berbagi koneksi, dan proses lokal lain dapat ikut menerima koneksi
yang ditujukan ke server Indonesiaku. Ditemukan pada uji ekstensif 1.4.0,
masih ada di 1.6.1.

## 2. Perubahan

- Di Windows, `SO_REUSEADDR` tidak lagi dipasang. Perilaku bawaan Winsock
  sudah mengizinkan bind ulang selama koneksi lama TIME_WAIT, sehingga
  restart cepat tetap berfungsi. asyncio Python mengambil keputusan yang
  sama.
- POSIX tidak berubah.

## 3. Rencana uji

- `examples/soket_port_eksklusif.idk` + golden, portabel: `soket_dengar`
  kedua ditolak, satu koneksi utuh ditutup dari sisi server (TIME_WAIT),
  lalu port itu didengarkan ulang dengan sukses. Dengan biner 1.6.1 di
  Windows, golden ini mencetak `kedua: berhasil (SALAH)`.
- Golden sama di Linux (WSL, build dari branch ini).
- Regresi restart: `test_all.ps1` 289/289, `test_todo_server.ps1`, serta
  `web_*` dan `keamanan_app.sh` yang menjalankan ulang server di port tetap,
  semuanya lulus di Windows.
