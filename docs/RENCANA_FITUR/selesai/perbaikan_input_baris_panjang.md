# Rencana Perbaikan: `input()` dan REPL memotong baris panjang (T-U10)

- **Status:** selesai
- **Target versi:** 1.4.1
- **Pemilik:** Teguh Rijanandi (dikerjakan oleh Claude Code)

## 1. Masalah

Temuan T-U10 (uji ekstensif Windows 1.4.0, `CLI-ALAT-02`): `input()` membaca
dengan `fgets` ke buffer 1024 bita. Baris yang lebih panjang terpotong pada
1023 bita, dan sisanya dikembalikan oleh `input()` berikutnya. Program yang
membaca satu baris JSON/CSV panjang dari stdin diam-diam mendapat data
terpotong. REPL memakai `fgets` yang sama, dan buffer bloknya yang 8192 bita
diam-diam membuang setiap baris yang tidak muat.

## 2. Sintaks usulan

Tidak ada sintaks baru.

## 3. Semantik

`input()` mengembalikan satu baris utuh, berapa pun panjangnya, tanpa `\n`
penutup. Di akhir masukan hasilnya tetap `kosong`. REPL menerima baris dan
blok dengan panjang berapa pun.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/native.c`/`.h` | `readLineAlloc(FILE*, size_t*)` membaca satu baris ke buffer malloc yang tumbuh. `input()` memakainya. |
| `src/main.c` | REPL memakai `readLineAlloc`, dan buffer bloknya tumbuh (`appendReplLine`). |

## 5. Pengujian

- `tests/test_baris_panjang.sh`, dijalankan dari `tests/test_all.sh`:
  `input()` 5000 bita, tepat 1023/1024 bita, baris terakhir tanpa `\n`,
  serta REPL dengan baris 3000 bita dan blok di atas 8192 bita. Golden tidak
  dapat memberi stdin, jadi pengujiannya lewat skrip ini.

## 6. Dampak dokumentasi & tooling

Perbaikan bug tanpa perubahan API. Ada entri di `CHANGELOG.md`.

## 7. Risiko & alternatif

Satu baris yang sangat besar dibaca seluruhnya ke memori, sama seperti
`input()` di Python. `getline` POSIX tidak tersedia di MinGW, jadi
pembacanya ditulis sendiri dengan `fgetc`.
