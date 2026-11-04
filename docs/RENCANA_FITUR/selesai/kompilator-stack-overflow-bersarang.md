# Rencana Fitur: Kompilator tidak lagi mati tanpa pesan pada kode sangat bersarang (T-W8)

- **Status:** selesai
- **Target versi:** 1.4.1
- **Pemilik:** teguh02

## 1. Masalah

Kode yang sangat bersarang membuat kompilator **mati tanpa pesan apa pun**:
proses berakhir dengan SIGSEGV di Linux/macOS dan `0xC00000FD` (stack
overflow) di Windows. Pengguna tidak mendapat baris, kolom, maupun sebab.

Reproduksi terukur di host ini dengan stack 1 MB (`ulimit -s 1024`, menyamai
ukuran stack bawaan thread utama Windows):

```
32 fungsi bersarang   -> Segmentation fault, exit 139, stderr kosong
```

Temuan ini tercatat sebagai **T-W8** di `docs/TESTING/WINDOWS.md` dengan
tingkat Tinggi. Catatan aslinya menyebut ambang "32 `fungsi` bersarang atau
±2500 kurung bersarang".

### Pengukuran ulang: angka di catatan itu menyesatkan

Mengukur tiap bentuk sarang secara terpisah pada stack 1 MB memberi hasil yang
sangat berbeda antar bentuk:

| Bentuk sarang | Crash pertama |
|---|---|
| `fungsi` bersarang | **n = 16** |
| kurung `(` bersarang | aman sampai n = 1024 |
| `jika` bersarang | aman sampai n = 1024 |

Jadi masalahnya **bukan** kedalaman rekursi parser secara umum. `fungsi`
jatuh 64× lebih cepat daripada bentuk lain karena satu sebab khusus di bawah.

## 2. Sebab sebenarnya

`function()` di `src/compiler.c` mengalokasikan **seluruh struct `Compiler` di
stack**:

```c
static void function(FunctionType type) {
    Compiler compiler;            // <-- ~24 KB di stack
    initCompiler(&compiler, type);
```

`Compiler` berisi empat larik berukuran `UINT8_COUNT` (256) —
`locals`, `upvalues`, `globals`, `nonlocals` — ditambah `finallies`. Totalnya
diukur antara 16 KB dan 32 KB. Pada stack 1 MB, 16 level sarang sudah
menghabiskannya; itu persis angka yang terukur.

Kurung dan `jika` hanya memakai frame parser biasa (ratusan byte), karena itu
mereka aman sampai ribuan level.

## 3. Perubahan

Dua bagian, keduanya kecil dan di satu berkas:

1. **Pindahkan `Compiler` milik `function()` ke heap.** Per level sarang stack
   turun dari ~24 KB menjadi beberapa puluh byte. Ini memperbaiki akar
   masalahnya, bukan gejalanya.

2. **Tambahkan satu penjaga kedalaman parser** (`NESTING_MAX`) pada dua corong
   rekursi: `parsePrecedence()` (ekspresi, mencakup kurung) dan `block()`
   (pernyataan, mencakup `jika`/`selagi`/`untuk`/`fungsi`). Melewati batas
   menghasilkan galat kompilasi biasa — dengan baris dan kolom — bukan crash.

Butuh keduanya: (1) saja hanya memundurkan ambang, kode cukup dalam tetap
bisa menghabiskan stack; (2) saja membuat `fungsi` tertolak pada kedalaman
yang masih wajar karena 24 KB per level.

## 4. Perilaku baru

```
[baris N, kolom M] Kesalahan di '{': Sarang terlalu dalam (maksimum 200);
sederhanakan kode.
```

Keluar dengan kode galat kompilasi biasa, stdout tetap bersih.

## 5. Berkas tersentuh

- `src/compiler.c` — alokasi heap + penjaga kedalaman.
- `tests/galat/sarang_terlalu_dalam.idk` + `.err` — regresi batas.
- `examples/sarang_dalam.idk` + `tests/expected/sarang_dalam.out` — sarang
  dalam yang **masih sah** harus tetap berhasil (menjaga agar batasnya tidak
  dipasang terlalu ketat).
- `CHANGELOG.md`, `docs/TESTING/WINDOWS.md` (T-W8 → selesai).

## 6. Rencana uji

- 40 `fungsi` bersarang pada `ulimit -s 1024` → galat rapi, bukan SIGSEGV.
- 3000 kurung bersarang → galat rapi.
- Sarang 100 level (di bawah batas) → tetap berjalan dan mencetak hasil benar.
- Suite penuh hijau; build ASan/UBSan untuk memastikan tidak ada kebocoran
  dari alokasi heap yang baru.
- Sabotase: hapus `free()` → ASan melaporkan kebocoran; naikkan `NESTING_MAX`
  ke nilai sangat besar → tes galat kembali merah.
