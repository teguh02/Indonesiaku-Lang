# Rencana Perbaikan: argumen bernama di dalam fungsi (T-U3)

- **Status:** selesai
- **Target versi:** 1.4.1
- **Pemilik:** Teguh Rijanandi (dikerjakan oleh Claude Code)

## 1. Masalah

Temuan T-U3 (uji ekstensif Windows 1.4.0, `PUSTAKA-02`): di dalam fungsi,
panggilan dengan argumen bernama membuat argumen posisi yang berupa
global/upvalue menjadi `kosong`.

```indonesiaku
fungsi g(a, b = 0) { kembalikan [a, b] }
D = 7
fungsi f() { kembalikan g(D, b = 2) }
cetak(f())        # 1.4.0: [kosong, 2] — seharusnya [7, 2]
```

Penyebabnya ada di `prescanFunctionBody` (`compiler.c`). Prescan K3 membaca
token `D , b =` sebagai pembongkaran `a, b = ...`, sehingga `D` dan `b`
diberi slot lokal berisi `kosong`. Pustaka standar ikut terkena, misalnya
`statistik.kuantil(DATA, n = 2)` dan `csv.urai(t, pemisah = ";")` di dalam
closure.

## 2. Sintaks usulan

Tidak ada sintaks baru.

## 3. Semantik

`nama = nilai` yang langsung berada di dalam kurung argumen sebuah panggilan
(setelah `(` atau `,`) adalah argumen bernama (K16), bukan assignment. Karena
itu prescan tidak membuat lokal untuk nama tersebut, dan koma di dalam kurung
itu tidak merangkai nama menjadi pembongkaran.

Assignment di dalam kurung pengelompokan, `r = (s = 3)`, tetap membuat lokal
`s` (K3). Pembongkaran biasa `p, q = ...` juga tidak berubah.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/compiler.c` | `prescanFunctionBody` mencatat untuk setiap `(`/`[` yang terbuka apakah ia kurung panggilan, yaitu `(` setelah identifier, `)`, `]`, atau `super`. `=` setelah `( nama` atau `, nama` di dalam kurung panggilan dilewati. |

## 5. Pengujian

- Golden `examples/argumen_bernama_lingkup.idk` menguji global, upvalue, nama
  argumen yang sama dengan global yang dibaca, metode, panggilan berantai,
  panggilan bersarang, dan kasus K3 yang harus tetap lokal.
- Suite lengkap dijalankan, ditambah build `-DDEBUG_STRESS_GC`.

## 6. Dampak dokumentasi & tooling

Ini perbaikan bug ke perilaku yang sudah didokumentasikan (K16), jadi tidak
ada perubahan panduan, sintaks, atau editor. Entri ditambahkan ke
`CHANGELOG.md`.

## 7. Risiko & alternatif

Sebuah `(` yang didahului `)`/`]` dianggap kurung panggilan, misalnya
`f()(x = 1)`. Itu memang panggilan. Assignment ekspresi di dalam argumen
tanpa kurung tambahan (`f(x = 1)`) memang sudah berarti argumen bernama
sejak K16.
