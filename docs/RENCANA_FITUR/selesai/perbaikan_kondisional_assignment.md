# Rencana Perbaikan: assignment dengan ekspresi kondisional (T-U2)

- **Status:** selesai
- **Target versi:** 1.4.1
- **Pemilik:** Teguh Rijanandi (dikerjakan oleh Claude Code)

## 1. Masalah

Temuan T-U2 (uji ekstensif Windows 1.4.0, `SINTAKS-01`): tanpa `variabel`,
`x = a jika c selain b` dikompilasi sebagai `(x = a) jika c selain b`. Bila
kondisinya salah, `x` tidak di-assign sama sekali. Di tingkat skrip `x` jadi
tidak terdefinisi (exit 70), variabel lama tetap bernilai lama, dan di dalam
fungsi hasilnya `kosong`.

```indonesiaku
x = 1 jika salah selain 2
cetak(x)      # 1.4.0: Variabel tidak terdefinisi 'x' — seharusnya 2
```

## 2. Sintaks usulan

Tidak ada sintaks baru. Panduan sudah menyebut bentuk ini sama dengan
`a if c else b` di Python.

## 3. Semantik

Sisi kanan assignment ke nama adalah ekspresi penuh, termasuk ekspresi
kondisional, sama seperti assignment ke atribut (`o.x = ...`), indeks
(`d[i] = ...`), dan assignment majemuk (`x += ...`).

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/compiler.c` | Cabang `=` di `namedVariable` memanggil `expression()`, bukan `parsePrecedence(PREC_ASSIGNMENT)`. |

## 5. Pengujian

- Golden `examples/kondisional_assignment.idk` menguji global baru, global
  lama, lokal K3, `nonlokal`, rantai `a = b = ...`, tupel di sisi kanan,
  kondisional berantai, atribut/indeks, dan assignment majemuk.
- Suite lengkap dijalankan, ditambah build ASan + UBSan + `-DDEBUG_STRESS_GC`.

## 6. Dampak dokumentasi & tooling

Ini perbaikan bug ke perilaku yang sudah didokumentasikan, jadi tidak ada
perubahan panduan atau editor. Entri ditambahkan ke `CHANGELOG.md`.

## 7. Risiko & alternatif

Program yang secara tidak sengaja bergantung pada `(x = a) jika c selain b`
berubah perilaku. Bentuk itu tidak pernah didokumentasikan dan hasilnya
salah, jadi perubahan ini tidak diberi tanda merusak.
