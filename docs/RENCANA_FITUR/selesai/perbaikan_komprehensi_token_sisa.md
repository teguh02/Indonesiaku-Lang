# Rencana Perbaikan: comprehension mengabaikan token sisa (T-U9)

- **Status:** selesai
- **Target versi:** 1.4.1
- **Pemilik:** Teguh Rijanandi (dikerjakan oleh Claude Code)

## 1. Masalah

Temuan T-U9 (uji ekstensif Windows 1.4.0, `ALUR-FUNGSI-01`). Comprehension
diam-diam membuang semua token setelah syarat pertama, lalu program berjalan
dengan hasil yang salah dan kode keluar 0:

```indonesiaku
cetak([x untuk x dalam rentang(10) jika x > 2 jika x < 5])   # [3, ..., 9]
cetak([x untuk x dalam [1, 2] selain 5])                      # [1, 2]
cetak([x untuk x dalam [1] untuk y dalam [2]])               # [1]
```

`comprehensionBody` melompati token sampai kurung penutup, tetapi setelah
mem-parse iterable dan syarat ia tidak pernah memeriksa bahwa kurung
penutup memang yang berikutnya.

## 2. Sintaks usulan

Tidak ada sintaks baru. Panduan sudah membatasi bentuknya menjadi satu
`untuk` dan satu `jika` opsional.

## 3. Semantik

Setelah iterable hanya boleh ada `jika` atau kurung penutup, dan setelah
syarat hanya kurung penutup. Selain itu, kompilasi gagal (exit 65):

- `untuk` kedua: `Comprehension hanya mendukung satu 'untuk'.`
- `jika` kedua: `Comprehension hanya mendukung satu 'jika'; gabungkan syarat dengan 'dan'.`
- token lain: `Harapkan ']' di akhir comprehension.` (atau `)`/`}`)

Berlaku untuk daftar, kamus, himpunan, dan generator.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/compiler.c` | `expectComprehensionEnd()` dipanggil setelah syarat dan setelah iterable di `comprehensionBody`. |

## 5. Pengujian

- `tests/galat/komprehensi_{jika_kedua,untuk_kedua,token_sisa,generator_sisa}`.
- Golden comprehension yang sudah ada (`komprehensi_*`, kamus, himpunan,
  generator) tetap lulus.

## 6. Dampak dokumentasi & tooling

- [x] `website/panduan/03_PANDUAN_SINTAKS.md`: bentuk lain adalah galat kompilasi.
- [x] `CHANGELOG.md` (⚠️)

## 7. Risiko & alternatif

Program yang menulis `jika` kedua kini gagal dikompilasi, padahal dulu
berjalan dengan hasil salah. Comprehension bersarang (`untuk` kedua) seperti
Python adalah fitur, bukan perbaikan, jadi untuk saat ini ditolak dengan
jelas.
