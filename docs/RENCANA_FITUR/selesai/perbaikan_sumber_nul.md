# Rencana Perbaikan: bita NUL di sumber memotong program (T-U11)

- **Status:** selesai
- **Target versi:** 1.4.1
- **Pemilik:** Teguh Rijanandi (dikerjakan oleh Claude Code)

## 1. Masalah

Temuan T-U11 (uji ekstensif Windows 1.4.0, `CLI-ALAT-03`). Sumber dibaca
sebagai string C, sehingga bita NUL mengakhirinya:

```text
printf 'cetak(1)\0cetak(2)\ncetak(3)\n' > nul2.idk
indk rapikan --tulis nul2.idk     # exit 0, isi berkas tinggal 'cetak(1)\n'
indk nul2.idk                     # mencetak 1, exit 0
```

`rapikan --tulis` menghilangkan kode pengguna tanpa peringatan apa pun.

## 2. Sintaks usulan

Tidak ada sintaks baru.

## 3. Semantik

Seperti Python ("source code cannot contain null bytes"): sumber yang berisi
bita NUL ditolak sebelum dikompilasi.

- Berkas utama: `indk berkas.idk`, `rapikan`, `--dis`, `periksa`, dan `uji`
  mencetak `Berkas "<jalur>" berisi bita NUL; kode sumber tidak boleh
  memuatnya.` lalu keluar dengan 65, tanpa menulis apa pun.
- Modul (`impor nama` dan `impor "berkas"`) menaikkan galat runtime yang bisa
  ditangkap: `Modul '<jalur>' berisi bita NUL; ...`.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/common.h` | `sourceHasNul()` |
| `src/main.c` | `readFile` menolak sumber yang memuat NUL (exit 65). |
| `src/vm.c` | `readModuleSource` mengembalikan `NULL` dan menyetel `moduleSourceHasNul`. `reportNulModule()` memberi pesan yang tepat di kedua jalur impor. |

## 5. Pengujian

- `tests/galat/sumber_nul.idk` mengimpor `tests/bantu/sumber_nul.dat`, yang
  berisi NUL. Uji ini lewat impor karena pesan berkas utama memuat jalur,
  yang ditulis berbeda di Windows.
- `tests/test_format.sh`: `rapikan --tulis` pada berkas ber-NUL keluar 65 dan
  berkasnya tetap byte-identik.

## 6. Dampak dokumentasi & tooling

Perbaikan bug, tanpa perubahan API. Ada entri di `CHANGELOG.md`.

## 7. Risiko & alternatif

Temuan yang sama juga menyebut bahwa `rapikan --tulis` menulis ulang berkas
yang punya galat sintaks dengan exit 0. Formatter bekerja per token dan
tidak menghapus kode, jadi bagian itu tidak termasuk kehilangan data dan
ditangani terpisah. NUL di baris REPL tidak diperiksa.
