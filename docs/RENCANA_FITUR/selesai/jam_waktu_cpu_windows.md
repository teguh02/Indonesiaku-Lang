# Rencana Fitur: jam() sebagai waktu CPU di Windows (T-W11)

- **Status:** selesai
- **Target versi:** 1.6.2
- **Pemilik:** pemeliharaan runtime

## 1. Masalah

Panduan 07 menyebut `jam()` sebagai waktu CPU, dan memang begitu di
Linux/macOS karena `clock()` POSIX mengukur waktu CPU. Di Windows, `clock()`
milik CRT (MSVCRT/UCRT) mengukur waktu dinding sejak proses mulai. Akibatnya
`jam()` ikut bertambah selama `henti()`: di 1.6.1, `henti(0.5)` menaikkan
`jam()` sebesar 0,505. Ditemukan pada uji ekstensif 1.4.0 dan masih ada di
1.6.1.

## 2. Perubahan

- `sistemWaktuCpu()` di `src/sistem.c`. Di Windows memakai
  `GetProcessTimes` (kernel + user, satuan 100 ns), dengan cadangan
  `clock()` bila panggilan itu gagal. Di POSIX tetap `clock()`.
- `clockNative` (`jam`) di `src/vm.c` memanggil fungsi itu.

## 3. Rencana uji

- `examples/jam_waktu_cpu.idk` + golden. Selisih `jam()` selama `henti(0.3)`
  harus `< 0.15`, dan selama menghitung harus `> 0`. Hasil dicetak sebagai
  logika supaya stabil di semua OS. Dengan biner 1.6.1 di Windows baris
  pertamanya `salah`.
- Golden yang sama lulus di Linux (WSL, build dari branch ini).
