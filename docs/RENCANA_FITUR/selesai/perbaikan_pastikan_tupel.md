# Rencana Perbaikan: `pastikan(kondisi, pesan)` selalu lolos (T-U5)

- **Status:** selesai
- **Target versi:** 1.4.1
- **Pemilik:** Teguh Rijanandi (dikerjakan oleh Claude Code)

## 1. Masalah

Temuan T-U5 (uji ekstensif Windows 1.4.0, `GALAT-01`): `pastikan(1 == 2,
"pesan")` membentuk satu tupel tidak kosong, yang selalu benar, sehingga
penegasannya tidak pernah gagal. Panduan menjanjikan bahwa bentuk ini
ditolak saat kompilasi, tetapi biner 1.4.0 menerimanya.

## 2. Sintaks usulan

Tidak ada sintaks baru.

## 3. Semantik

Bila kondisi `pastikan` adalah *seluruhnya* literal tupel tidak kosong yang
diawali `(`, kompilasi gagal (exit 65) dengan pesan:

```text
Kondisi 'pastikan' berupa tupel sehingga selalu benar. Tulis tanpa kurung: pastikan kondisi, "pesan".
```

Python hanya memberi `SyntaxWarning`. Indonesiaku menolaknya karena panduan
sudah menjanjikan hal itu, dan penegasan yang diam-diam mati lebih buruk
daripada galat kompilasi. Bentuk yang tetap sah:

- `pastikan(kondisi)`: kurung tanpa koma hanya mengelompokkan;
- tupel di dalam ekspresi yang lebih besar, misalnya `pastikan (1, 2)[0] == 1`
  dan `pastikan (a, b) == (c, d)`.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/compiler.c` | `grouping()` mencatat posisi `(` dan akhir bytecode dari literal tupel terakhir. `assertStatement()` menolak kondisi yang diawali `(` itu dan berakhir tepat di akhir tupel tersebut. |

## 5. Pengujian

- Galat `tests/galat/pastikan_tupel.idk` + `.err`.
- Golden `examples/pastikan.idk` ditambah kasus kurung pengelompok yang sah.

## 6. Dampak dokumentasi & tooling

- [x] `website/panduan/03_PANDUAN_SINTAKS.md`: pesan galat yang sebenarnya.
- [x] `CHANGELOG.md`

## 7. Risiko & alternatif

Program yang menulis `pastikan(kondisi, pesan)` kini gagal dikompilasi.
Penegasan seperti itu memang tidak pernah bekerja, jadi galat ini
mengungkapkan bug yang sudah ada di program tersebut. Di CHANGELOG
perubahan ini diberi tanda ⚠️.
