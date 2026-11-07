# Rencana Fitur: `acak_bita()` — sumber acak kriptografis

- **Status:** selesai
- **Target versi:** 1.7.0
- **Pemilik:** Teguh Rijanandi

## 1. Masalah

Fase 13 butir 2 menuntut ID sesi dan rotasinya saat login. Hari ini satu-satunya
sumber acak di Indonesiaku adalah `acak()` dan `acak_bulat()`, keduanya memakai
`rand()` dari libc yang di-seed `srand(time(NULL))` di `src/vm.c`.

Itu **bukan kelemahan teoretis**. Dibuktikan dengan membandingkan keluaran
`indk` terhadap program C yang hanya menebak detik start:

```
# tiga nilai acak_bulat() dari indk
62731254 1265279902 1991646467

# srand(1794038063) di program penyerang
seed 1794038063 -> 1991646467 1265279902 62731254
```

Angkanya sama persis. Penyerang yang tahu kira-kira kapan server dinyalakan —
tersedia gratis di header `Date` setiap respons HTTP — mereproduksi seluruh
barisan dalam beberapa percobaan. ID sesi yang dibangun dari `acak_bulat()`
dapat ditebak, sehingga **session hijacking tanpa menyentuh jaringan korban**.

Membangun session store di atas fondasi ini akan menghasilkan fitur yang
tampak benar dan sepenuhnya tidak aman, jadi prasyarat ini ditutup lebih dulu.

## 2. Sintaks usulan

```python
variabel b = acak_bita(16)       # daftar 16 bulat 0..255, dari CSPRNG OS
```

Tidak ada kata kunci baru. Satu builtin, mengikuti konvensi daftar-bita yang
sudah dipakai `soket_baca_bita()` dan BLOB `db_*`.

## 3. Semantik

- `acak_bita(n)` mengembalikan `daftar` berisi `n` bulat 0..255.
- `n` wajib bulat `1..1048576`. Di luar itu `nativeRaise` (bisa ditangkap
  `coba/kecuali`), bukan fatal.
- `n` bukan angka, argumen kurang/lebih → `nativeRaise`.
- **Gagal berarti galat, tidak pernah diam-diam turun mutu.** Bila sumber acak
  OS tidak bisa dibaca, builtin menaikkan galat; ia TIDAK boleh jatuh ke
  `rand()`. Fallback senyap persis bug yang sedang ditutup di sini.

### Mengapa `acak()` dan `acak_bulat()` TIDAK diubah

Keduanya tetap `rand()`. Python memisahkan `random` (simulasi, cepat,
reproducible) dari `secrets` (keamanan); pemisahan yang sama berlaku di sini.
Mengubah `acak()` jadi kriptografis akan memperlambat semua kode simulasi dan
menghapus kemampuan reproduksi, tanpa menambah keamanan siapa pun yang
sebetulnya harus memakai `acak_bita()`. Yang ditambahkan adalah **peringatan
di dokumentasi** bahwa `acak*` tidak boleh dipakai untuk token/sesi/sandi.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/scanner.c` | — |
| `src/compiler.c` | — |
| `src/vm.c` | `defineNative("acak_bita", ...)` |
| `src/native.c` | `nativeFnAcakBita()` |
| `src/native.h` | deklarasi |
| `src/value.c` / `src/object.c` | — |
| `src/debug.c` | — |

Sumber entropi memakai **mbedTLS yang sudah di-vendor** (`ctr_drbg` +
`entropy`), bukan `getrandom()`/`BCryptGenRandom` tulisan tangan per OS:
mbedTLS sudah dipakai `src/tls.c` untuk kunci TLS, portabel ke tiga OS plus
musl, dan tidak menambah berkas `.c` baru (yang akan memaksa menyunting
Makefile + 4 daftar sumber mentah di `.github/workflows/test.yml`).

Konteks DRBG dibuat sekali lalu dipakai ulang (statis dalam `native.c`),
karena seeding ulang tiap panggilan mahal.

**GC-safety:** daftar hasil di-`push()` ke stack VM sebelum `writeValueArray()`
mengisinya, mengikuti pola `nativeFnSoketBacaBita()`.

## 5. Pengujian

- `tests/acak_aman.sh`:
  - panjang hasil benar; semua elemen 0..255;
  - dua panggilan berturut-turut tidak sama;
  - **dua proses `indk` terpisah pada detik yang sama menghasilkan nilai
    berbeda** — inilah yang gagal pada `acak_bulat()` dan yang sebenarnya
    sedang diperbaiki;
  - galat untuk `n` = 0, negatif, kelewat besar, bukan angka, arity salah.
- `tests/galat/` — pasangan `.idk`/`.err` untuk argumen tidak sah.
- Sabotase wajib merah: (a) ganti isi dengan `rand()`, (b) hapus pemeriksaan
  batas, (c) hapus `push()` pelindung GC lalu jalankan `-DDEBUG_STRESS_GC`.

## 6. Risiko

- Builtin baru = permukaan API baru; sudah seminimal mungkin (satu fungsi).
- DRBG statis dipakai ulang lintas panggilan: aman untuk satu utas, dan VM ini
  memang satu utas. Bila kelak ada thread, DRBG perlu dikunci —
  ditandai komentar `ponytail:` di sumbernya.
