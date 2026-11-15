# Rencana Fitur: ambil(kamus, kunci[, bawaan])

- **Status:** selesai
- **Target versi:** 1.8.0
- **Pemilik:** teguh02

## 1. Masalah

`kamus[kunci]` mengembalikan `kosong` untuk kunci yang tidak ada. Akibatnya
"kunci tidak ada" dan "kunci ada tetapi nilainya `kosong`" tidak dapat
dibedakan:

```indonesiaku
variabel d = {"a": kosong}
cetak(d["a"])      # kosong
cetak(d["zz"])     # kosong -- padahal kuncinya tidak ada
```

Untuk memberi nilai bawaan, pengguna harus menulis pola bertele-tele:

```indonesiaku
variabel nilai = kosong
jika punya(d, k) {
    nilai = d[k]
} selain {
    nilai = bawaan
}
```

Python menyediakan `dict.get(key, default)` untuk kasus ini. Indonesiaku sudah
punya `punya()` untuk memeriksa keberadaan kunci, tetapi belum punya cara
ringkas membaca dengan nilai bawaan.

## 2. Sintaks usulan

```indonesiaku
variabel d = {"a": 1, "kosongan": kosong}

cetak(ambil(d, "a"))            # 1
cetak(ambil(d, "zz"))           # kosong
cetak(ambil(d, "zz", 99))       # 99
cetak(ambil(d, "kosongan", 99)) # kosong -- kunci ADA, bawaan tidak dipakai

cetak(d.ambil("a"))             # 1  -- juga sebagai metode kamus
cetak(d.ambil("zz", "bawaan"))  # bawaan
```

Tidak ada kata kunci baru. `ambil` adalah builtin biasa, sejajar dengan
`punya()` dan `hapus_kunci()`, dan juga terdaftar sebagai metode kamus.

## 3. Semantik

- **Tipe:** argumen pertama wajib kamus. Kunci boleh nilai apa pun yang
  hashable, sama seperti `punya()`.
- **Arity:** 2 atau 3 argumen. Tanpa `bawaan`, kunci yang tidak ada
  menghasilkan `kosong` — sama persis dengan `kamus[kunci]`, sehingga
  `ambil()` tidak mengubah perilaku apa pun yang sudah ada.
- **Kunci ada bernilai `kosong`:** nilai `kosong` itu yang dikembalikan,
  **bukan** `bawaan`. Inilah yang membedakan `ambil()` dari `kamus[kunci]`:
  keputusan dibuat berdasarkan keberadaan kunci, bukan kebenaran nilainya.
- **Nilai falsy** (`0`, `""`, `salah`) dikembalikan apa adanya, tidak diganti
  `bawaan`.
- **Kamus tidak berubah.** Berbeda dengan `setdefault` di Python, `ambil()`
  tidak pernah menulis kunci baru.
- **Galat** dapat ditangkap `coba/kecuali` (`nativeRaiseKelas`), bukan fatal:
  jumlah argumen salah dan argumen pertama bukan kamus sama-sama menaikkan
  `GalatTipe`.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/scanner.c` | — tidak ada kata kunci baru |
| `src/compiler.c` | — tidak ada aturan parsing baru |
| `src/vm.c` | daftarkan `defineNative("ambil", ...)` dan entri `dictMethods` |
| `src/native.c` | `nativeFnAmbil()` |
| `src/native.h` | deklarasi `nativeFnAmbil` |
| `src/value.c` / `src/object.c` | — tidak ada tipe objek baru |
| `src/debug.c` | — tidak ada opcode baru |

**GC-safety:** tidak ada alokasi sama sekali. `valueTableGet` hanya membaca,
dan nilai yang dikembalikan sudah dipegang oleh kamusnya. Argumen tetap berada
di stack sepanjang pemanggilan native, jadi tidak ada objek tak ter-root.

Catatan: pencarian kunci dapat menjalankan `_sama_`/`_hash_` milik pengguna
(K19), tetapi `ambil()` tidak mem-`pop` apa pun sebelum pencarian, sehingga
pola bug use-after-free pada `kamus[k]` tidak berlaku di sini.

## 5. Pengujian

- Contoh golden: `examples/ambil_kamus.idk` + `tests/expected/ambil_kamus.out`.
- Kasus batas yang wajib diuji:
  - kunci ada / tidak ada, dengan dan tanpa `bawaan`;
  - kunci ada bernilai `kosong` → `bawaan` **tidak** dipakai;
  - nilai falsy (`0`) tidak diganti `bawaan`;
  - pemakaian sebagai metode `d.ambil(...)`;
  - `punya()` tetap menjadi cara memeriksa keberadaan;
  - kamus tidak berubah setelah `ambil()`;
  - galat: argumen kurang, argumen pertama bukan kamus → `GalatTipe`.
- Uji sabotase: tiap bagian logika dicabut satu per satu dan dipastikan tes
  menjadi merah (bawaan diabaikan, `bawaan` menimpa nilai `kosong`, cek arity
  dilepas).
- `-DDEBUG_STRESS_GC` → keluaran harus identik.

## 6. Dampak dokumentasi & tooling

- [x] `website/panduan/03_PANDUAN_SINTAKS.md` — bagian kamus
- [x] `website/panduan/07_PUSTAKA_STANDAR.md`
- [x] `editors/vscode-indonesiaku/language-data.js` — BUILTINS
- [x] `editors/vscode-indonesiaku/syntaxes/indonesiaku.tmLanguage.json` —
      tidak perlu diubah: polanya memakai alternasi berprefiks, dan `ambil`
      sudah tercakup oleh entri `ambil_atribut`/`ambil_lingkungan` yang ada.
      Diverifikasi oleh `tests/builtin_konsisten.sh`, bukan diasumsikan.
- [ ] `editors/vscode-indonesiaku/snippets/indonesiaku.code-snippets` —
      tidak perlu, ini pemanggilan fungsi biasa
- [x] `README.md`
- [x] `CHANGELOG.md` + `docs/ROADMAP.md`
      + `docs/ANALISIS/02_PETA_FITUR_PYTHON.md`
- [ ] `AGENTS.md` — tidak ada jebakan baru
- [x] Berkas rencana ini: status → `selesai`

## 7. Risiko & alternatif

**Risiko kompatibilitas: rendah.** Murni menambah builtin; tidak ada perilaku
lama yang berubah. Satu-satunya cara program lama terdampak adalah bila ia
mendefinisikan global bernama `ambil` sendiri — dan itu tetap bekerja karena
`OP_SET_GLOBAL` membayangi builtin di tabel modul, bukan menimpanya.

**Alternatif yang dipertimbangkan:**

1. **Membuat `kamus[kunci]` menaikkan `GalatKunci`** seperti Python. Ini
   perilaku yang lebih benar, tetapi **breaking**: 54 tempat di `pustaka/`
   memakai pola perbandingan `kosong`. Ditunda ke v2.0 dengan panduan migrasi;
   `ambil()` justru menyediakan jalan keluar yang eksplisit sebelum itu.
2. **`setdefault` ala Python** yang menulis kunci bila belum ada. Ditolak:
   menggabungkan baca dan tulis dalam satu nama membuat kode sulit dibaca, dan
   belum ada permintaan nyata.
3. **Parameter bawaan di `punya()`**. Ditolak: `punya()` mengembalikan boolean,
   mencampurnya dengan pengembalian nilai akan mengaburkan maknanya.
