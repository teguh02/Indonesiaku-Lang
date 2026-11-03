# Rencana Perbaikan: `==` dan hash pada struktur sangat dalam (T-U13, T-U14)

- **Status:** selesai
- **Target versi:** 1.4.1
- **Pemilik:** Teguh Rijanandi (dikerjakan oleh Claude Code)

## 1. Masalah

Temuan dari uji ekstensif Windows 1.4.0:

- **T-U13** (`SINTAKS-02`, `KOLEKSI-02`): `valuesEqual` berhenti di
  `EQUAL_MAX_DEPTH` 64 dan menjawab `salah`. Dua daftar berantai identik
  sedalam 100 tingkat dinyatakan berbeda, dan `!=` menjawab `benar`. Daftar
  yang memuat dirinya sendiri juga diam-diam `salah`, padahal Python menaikkan
  `RecursionError`.
- **T-U14** (`KOLEKSI-01`): `hashValue` merekursi tanpa batas. Tupel
  bersarang 20000 tingkat sebagai kunci kamus atau anggota `himpunan` membuat
  interpreter crash karena stack overflow, dan `coba` pun tidak bisa
  menangkapnya.

## 2. Sintaks usulan

Tidak ada sintaks baru.

## 3. Semantik

Perbandingan struktural dan hash tupel/`himpunan_beku` menelusuri paling
banyak `VALUE_MAX_DEPTH` = 1000 tingkat, sama dengan batas rekursi bawaan
Python. Lebih dalam dari itu menaikkan `GalatRekursi` yang bisa ditangkap:

```text
Kedalaman perbandingan melebihi 1000 tingkat (struktur terlalu dalam atau merujuk dirinya sendiri).
Kedalaman hash melebihi 1000 tingkat (struktur terlalu dalam atau merujuk dirinya sendiri).
```

Berlaku juga untuk `!=`, `dalam`, dan setiap native yang memakai
`valuesEqual`. Galat yang tertunda diselesaikan oleh pemanggilnya, dengan
mekanisme yang sama seperti `_sama_` (K19).

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/value.c`/`.h` | `VALUE_MAX_DEPTH`, `depthExceeded()`, dan `valueDepthHook`. `hashValue` → `hashValueDepth`, dengan batas pada tupel dan `himpunan_beku`. |
| `src/vm.c` | `valueDepthError` menaikkan `GalatRekursi` (sekali saja) lewat `nativeRaiseKelas`. |

## 5. Pengujian

- Golden `examples/kedalaman_sama_hash.idk`: kedalaman 60/100/900 tetap
  benar, 5000 menaikkan galat, daftar yang merujuk dirinya sendiri, hash
  tupel 20000 tingkat di kamus dan `himpunan`, tupel 500 tingkat yang tetap
  bisa menjadi kunci, dan `dalam`.
- Galat `tests/galat/hash_tupel_dalam.idk`: tidak ditangkap → exit 70
  dengan pesan, bukan crash.

## 6. Dampak dokumentasi & tooling

- [x] `website/panduan/03_PANDUAN_SINTAKS.md` (identitas dan perbandingan)
- [x] `AGENTS.md`
- [x] `CHANGELOG.md` (⚠️)

## 7. Risiko & alternatif

`a == b` untuk dua daftar yang merujuk dirinya sendiri kini menaikkan galat,
bukan `salah`, sama seperti Python. Deteksi siklus yang sebenarnya, seperti
`Py_ReprEnter`, bisa memberi jawaban untuk siklus, tetapi Python sendiri
tidak melakukannya untuk `==`.
