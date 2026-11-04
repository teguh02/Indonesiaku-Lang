# Rencana Fitur: `kecuali X { ... }` tanpa `sebagai` menjadi bertipe

- **Status:** selesai
- **Target versi:** 1.5.0
- **Pemilik:** teguh02

## 1. Masalah

`kecuali GalatNilai { ... }` diurai sebagai bentuk **tanpa tipe** dengan nama
pengikat `GalatNilai`. Akibatnya klausa itu menangkap **semua** galat, dan di
dalam blok nama `GalatNilai` berisi objek galatnya, bukan kelas:

```indonesiaku
coba { "a" + 1 } kecuali GalatNilai { cetak(GalatNilai) }
# 1.4.0: tertangkap (padahal GalatTipe), dan GalatNilai = pesan galatnya
```

Bentuk ini tidak pernah didokumentasikan — panduan hanya menyebut
`kecuali T sebagai e` dan `kecuali e` — tetapi 28 tempat di repo memakainya
seolah bertipe, termasuk contoh yang ikut dirilis (`db_batas_tunggu.idk`,
`db_blob.idk`, `http_pengalihan.idk`, `web_router.idk`). Semuanya diam-diam
menangkap galat yang tidak mereka maksud.

Temuan **GALAT-03** (tingkat tinggi) pada lampiran uji ekstensif 1.4.0.

## 2. Sintaks usulan

Sintaksnya tidak berubah. Yang berubah adalah artinya:

```indonesiaku
coba { ... } kecuali GalatTipe { ... }   # bertipe, seperti except TypeError:
coba { ... } kecuali e { ... }           # tangkap-semua, e terikat ke galat
```

## 3. Semantik

Nama sesudah `kecuali` tanpa `sebagai` diartikan **saat program berjalan**:

- Nama merujuk sebuah **kelas** → klausa bertipe; cocok bila galatnya instans
  kelas itu atau turunannya.
- Selain itu (nama belum ada, atau berisi non-kelas) → nama itu menjadi
  pengikat dan klausa menangkap semua, persis seperti 1.4.0.

Keputusan tidak bisa diambil saat kompilasi: kelas galat bawaan ada di tabel
global, dan pustaka boleh menambah miliknya sendiri
(`kelas GalatJaringan < GalatOS` di `pustaka/http.idk`). Heuristik nama juga
tidak dipakai karena kelas galat sah boleh bernama apa saja — `examples/
galat_turunan_bawaan.idk` memakai `N1` dan `B2`.

Nama tersebut tetap terikat di dalam blok pada kedua kasus, jadi kode lama
yang membaca nama itu tidak rusak.

**Batas yang disengaja:** bentuk tanpa `sebagai` tetap wajib menjadi klausa
**terakhir**, sama seperti `kecuali e`. Karena tipe-tidaknya baru diketahui
saat jalan, kompilator tidak bisa membuktikan klausa sesudahnya terjangkau.
Untuk merantai beberapa kelas, pakai `sebagai` atau `kecuali (A, B) sebagai e`.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/chunk.h` | opcode `OP_CATCH_TEST` (konstanta = nama yang diuji) |
| `src/compiler.c` | `catchClause()` memancarkan `OP_CATCH_TEST` untuk bentuk tanpa `sebagai`; flag `bare` menjaga aturan "harus terakhir" |
| `src/vm.c` | handler `OP_CATCH_TEST`: cari nama, uji instans bila kelas, selain itu `benar` |
| `src/debug.c` | nama opcode untuk disassembler |

## 5. Rencana uji

- `examples/kecuali_bertipe.idk` + golden: cocok, tidak cocok, kelas induk,
  tangkap-semua, kelas kustom (termasuk nama tanpa awalan `Galat`), dan nama
  kelas tetap terbaca di dalam blok.
- `tests/galat/kecuali_polos_bukan_terakhir.idk` yang sudah ada menjaga aturan
  urutan klausa.
- Seluruh suite: 292/292.
- Build `-DDEBUG_STRESS_GC -fsanitize=address,undefined` untuk contoh galat.

## 6. Catatan pelaksanaan

Perbaikan ini membuka satu bug yang selama ini tersembunyi di balik
catch-all: `himpunan([t])` pada tupel bersiklus menaikkan `GalatTipe`
("Anggota himpunan harus nilai yang dapat di-hash") padahal `setAdd` sudah
menaikkan `GalatRekursi` lebih dulu. `runtimeError()` sejak dulu menjaga
galat pertama lewat `vm.nativeErrorPending`, tetapi `nativeRaise()` dan
`nativeRaiseKelas()` tidak. Guard yang sama dipasang di kedua fungsi itu,
sehingga galat asli tidak lagi ditimpa pemanggilnya.
