# Rencana Perbaikan: `nan` menjadi INT64_MIN di `lantai`/`atap`/`bulat` (T-U12)

- **Status:** selesai
- **Target versi:** 1.4.1
- **Pemilik:** Teguh Rijanandi (dikerjakan oleh Claude Code)

## 1. Masalah

Temuan T-U12 (uji ekstensif Windows 1.4.0, `SINTAKS-03` dan `PUSTAKA-04`):

```indonesiaku
n = ke_angka("nan")
cetak(lantai(n), atap(n), bulat(n))   # -9223372036854775808 ×3
cetak(bulatkan(n), bulatkan(ke_angka("inf")))   # nan takhingga (desimal)
```

Pemeriksaan jangkauan `hasil >= 2^63 || hasil < -2^63` bernilai salah untuk
NaN, sehingga NaN sampai ke cast `(int64_t)`. Di C cast itu tidak terdefinisi,
dan di x86-64 hasilnya INT64_MIN. `bulatkan()` tanpa digit mengembalikan
`nan`/`takhingga` sebagai desimal, padahal dokumentasinya menjanjikan
`bulat`.

## 2. Sintaks usulan

Tidak ada sintaks baru.

## 3. Semantik

Seperti `math.floor`/`math.ceil`/`round` di Python:

- `nan` → `GalatNilai`: `<fungsi>() tidak dapat mengubah nan menjadi bilangan bulat`
- `takhingga`, `-takhingga`, dan hasil di luar int64 → `GalatLuapan`
- Argumen `bulat` dikembalikan apa adanya, sehingga `lantai(2 ** 62 + 1)`
  tidak lagi dibulatkan lewat double.
- `bulatkan(x, digit)` dengan digit bukan nol tidak berubah:
  `bulatkan(nan, 2)` tetap `nan`.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/native.c` | Helper `doubleToBulat()` dipakai oleh `bulat`, `lantai`, `atap`, dan `bulatkan` (tanpa digit dan dengan digit 0). |

## 5. Pengujian

- Golden `examples/nan_ke_bulat.idk`: nan/takhingga/-takhingga untuk
  keempat fungsi, `bulatkan` dengan digit, bilangan bulat di atas 2^53, dan
  pembulatan biasa.

## 6. Dampak dokumentasi & tooling

- [x] `website/panduan/07_PUSTAKA_STANDAR.md`
- [x] `CHANGELOG.md`

## 7. Risiko & alternatif

Program yang dulu menerima INT64_MIN dari NaN kini mendapat galat. Nilai lama
itu adalah hasil perilaku tak terdefinisi, jadi tidak ada yang layak
dipertahankan.
