# Rencana Perbaikan: galat dari `_logika_`/`_sama_` tertunda (T-U7)

- **Status:** selesai
- **Target versi:** 1.4.1
- **Pemilik:** Teguh Rijanandi (dikerjakan oleh Claude Code)

## 1. Masalah

Temuan T-U7 (uji ekstensif Windows 1.4.0, `OOP-01` dan `OOP-02`). Galat yang
dinaikkan metode ajaib tidak membatalkan operasinya:

- `jika B() { ... }` dengan `_logika_` yang naik tetap menjalankan cabangnya.
  `tidak B()` menghasilkan `salah`, `B() atau 1` menghasilkan objeknya, dan
  badan `selagi` berjalan sekali. Galatnya baru muncul pada panggilan native
  berikutnya, dan bila tidak ditangkap jenis serta pesannya hilang
  ("Kesalahan tak tertangani dari fungsi bawaan").
- Pada tabrakan hash, `d[k] = v` dengan `_sama_` yang naik tetap
  menyisipkan kunci kedua, lalu pernyataan berikutnya tetap berjalan.

Ini melanggar aturan K19: galat metode ajaib harus langsung membuka stack.

## 2. Sintaks usulan

Tidak ada sintaks baru.

## 3. Semantik

- Galat dari `_logika_` membatalkan `jika`, `selagi`, `tidak`, `dan`, dan
  `atau` sebelum cabang mana pun dipilih.
- Galat dari `_sama_` saat mencari, menyisipkan, atau menghapus kunci kamus
  membatalkan operasinya, dan kamus tidak berubah.
- Seperti Python, `_sama_` hanya dipanggil untuk kunci yang hash-nya sama.
- Memperbesar tabel tidak memanggil `_hash_` atau `_sama_`.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/table.h` | `ValueEntry.hash` menyimpan hash kunci. |
| `src/table.c` | Probe membandingkan hash sebelum `valuesEqual`, dan mengembalikan `NULL` bila `_sama_` naik; Get/Set/Delete lalu tidak membaca atau menulis apa pun. Resize memakai hash tersimpan. `valueTableDelete` mengembalikan kunci yang tersimpan. |
| `src/object.c` | `dictDelete` mencari kunci di daftar urutan berdasarkan identitas dengan kunci tersimpan, bukan `valuesEqual` (yang menjalankan `_sama_` terhadap setiap kunci). |
| `src/vm.c` | `OP_NOT`, `OP_JUMP_IF_FALSE`, dan jalur kamus pada `OP_INDEX_GET`/`SET`/`DELETE` memanggil `RESOLVE_MAGIC_ERROR()`. `OP_NOT` memakai `peek`, bukan `pop`, sebelum `_logika_` berjalan. |

## 5. Pengujian

- Golden `examples/oop/galat_ajaib_langsung.idk`: `jika`, `tidak`, `atau`,
  `dan`, `selagi`, kamus get/set/`buang`, jumlah panggilan `_sama_` untuk
  hash yang berbeda, dan jumlah panggilan `_hash_` saat resize.
- Suite lengkap dijalankan, ditambah ASan + UBSan + `-DDEBUG_STRESS_GC`.

## 6. Dampak dokumentasi & tooling

Perbaikan ke perilaku yang sudah didokumentasikan (K19). Ada entri di
`CHANGELOG.md`, dan `AGENTS.md` dapat catatan bahwa probe tabel nilai bisa
mengembalikan `NULL`.

## 7. Risiko & alternatif

`ValueEntry` dapat satu `uint32_t`. Di x86-64 ukurannya tetap 40 byte,
karena field itu mengisi celah padding sebelum `state`.

Pesan untuk galat tanpa `pesan` yang tidak ditangkap ("dari fungsi bawaan")
tidak diubah di sini.
