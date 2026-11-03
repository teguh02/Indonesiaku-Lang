# Rencana Perbaikan: `impor "berkas.idk"` mencemari tabel builtin (T-U8)

- **Status:** selesai
- **Target versi:** 1.4.1
- **Pemilik:** Teguh Rijanandi (dikerjakan oleh Claude Code)

## 1. Masalah

Temuan T-U8 (uji ekstensif Windows 1.4.0, `PUSTAKA-01`). `impor "berkas.idk"`
mengompilasi berkas tanpa modul, sehingga definisi tingkat atasnya ditulis ke
`vm.globals`, tabel builtin yang dibaca semua modul bernamespace.

```indonesiaku
# bajak.idk:  fungsi panjang(x) { kembalikan "DIBAJAK" }
# mm.idk:     fungsi f() { kembalikan panjang("abc") }
impor "bajak.idk"
impor mm
cetak(mm.f())        # 1.4.0: DIBAJAK — seharusnya 3
```

Pustaka standar ikut terkena. `impor "pustaka/base64.idk"` menjalankan `dari
bita impor ke_teks`, yang menimpa builtin `ke_teks`, sehingga setelah itu
`impor json` lalu `json.json_teks(5)` gagal. Panduan menjanjikan bahwa modul
yang diimpor tidak mencemari variabel Anda.

## 2. Sintaks usulan

Tidak ada sintaks baru.

## 3. Semantik

- `impor "berkas.idk"` menjalankan berkas di namespace **pengimpornya**:
  skrip utama, atau modul bernamespace yang menulis impor itu. Nama-namanya
  tetap langsung tersedia tanpa awalan bagi pengimpor, seperti yang dijanjikan
  bentuk lama, tetapi tidak terlihat oleh modul lain.
- Cache berlaku per pengimpor: impor kedua dari pengimpor yang sama tidak
  menjalankan ulang berkasnya. Pengimpor lain menjalankan salinannya sendiri.
- Berkas yang diimpor dari dalam berkas yang di-`impor "..."` masuk ke
  namespace yang sama (pengimpor teratasnya).

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/vm.c` | `importModule(path, into)` memakai `compileInModule(source, into)`, dengan `into` = modul fungsi pemanggil `OP_IMPORT`. Kunci cache `fileImportKey(path, modul)` menggabungkan alamat modul (modul tidak pernah dibebaskan) dan jalurnya. |

## 5. Pengujian

- Golden `examples/impor_berkas_namespace.idk`, dengan berkas bantu
  `examples/modul_contoh/bajak_panjang.idk` dan `hitung_panjang.idk`:
  pengimpor melihat definisinya, `json`/modul lain tetap memakai builtin,
  modul bernamespace yang mengimpor berkas yang sama mendapat salinannya
  sendiri, dan impor ulang tidak menjalankan berkas lagi. Biner 1.4.0 gagal
  pada contoh ini, biner baru lulus.
- Suite lengkap dijalankan, ditambah ASan + UBSan + `-DDEBUG_STRESS_GC`.

## 6. Dampak dokumentasi & tooling

- [x] `website/panduan/03_PANDUAN_SINTAKS.md` bagian Modul
- [x] `AGENTS.md`
- [x] `CHANGELOG.md` (⚠️)

## 7. Risiko & alternatif

Program yang secara tidak sengaja bergantung pada kebocoran ini akan rusak,
misalnya modul bernamespace yang memakai `teks_pisah` tanpa mengimpornya
hanya karena skrip utama sudah `impor "pustaka/teks.idk"`. Perbaikannya
mudah, yaitu menambahkan impor di modul itu, dan kebocorannya adalah yang
dilaporkan sebagai bug. Di CHANGELOG perubahan ini diberi tanda ⚠️.

Alternatifnya, setiap berkas dijalankan sekali dalam modul tersembunyi
sendiri lalu namanya disalin ke pengimpor. Pilihan itu tidak diambil karena
mengubah identitas global: assignment dari berkas itu tidak terlihat
pengimpor setelah impor, padahal bentuk lama menjanjikan namespace yang
menyatu.
