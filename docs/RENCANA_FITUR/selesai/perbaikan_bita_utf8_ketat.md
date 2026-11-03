# Rencana Perbaikan: `bita.ke_teks` menerima UTF-8 tidak sah (T-U16)

- **Status:** selesai
- **Target versi:** 1.4.1
- **Pemilik:** Teguh Rijanandi (dikerjakan oleh Claude Code)

## 1. Masalah

Temuan T-U16 (uji ekstensif Windows 1.4.0, `PUSTAKA-05`). `ke_teks` hanya
memeriksa byte pemimpin, sehingga ada beberapa urutan tidak sah yang lolos:

- `[0xC0, 0x80]` dan `[0xE0, 0x80, 0x80]` (*overlong*) menjadi NUL;
- byte lanjutan tidak diperiksa: `[0xC3, 0x41]` menjadi karakter sampah;
- surrogate `ED A0 80`, kode di atas `U+10FFFF`, dan pemimpin `F5..FF`
  diterima.

Data dari soket atau berkas yang rusak (atau sengaja dibuat, misalnya
NUL tersembunyi lewat `C0 80`) lolos diam-diam. Panduan §2.18 menjanjikan
`GalatBita` untuk urutan tidak sah.

## 2. Sintaks usulan

Tidak ada sintaks baru.

## 3. Semantik

Sama dengan `bytes.decode("utf-8")` di Python (RFC 3629): pemimpin harus
`C2..F4`, setiap byte lanjutan `80..BF`, dan nilai harus di atas batas
overlong untuk lebarnya, bukan surrogate, dan ≤ `U+10FFFF`. Setiap
pelanggaran menaikkan `GalatBita` dengan pesan yang menyebut jenisnya.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `pustaka/bita.idk` | `ke_teks` ditulis ulang: lebar dan nilai minimum ditentukan dari pemimpin, lalu setiap byte lanjutan divalidasi. Jalur ASCII tetap satu langkah, jadi uji performa linear di `tests/pustaka_performa.sh` tidak berubah. |

## 5. Pengujian

- Golden `examples/bita_utf8_ketat.idk`: 1–4 byte yang sah, `U+10FFFF`,
  overlong 2/3/4 byte, surrogate, di atas `U+10FFFF`, pemimpin `F5`, lanjutan
  salah/menggantung, terpotong, dan bolak-balik `ke_bita`/`ke_teks`. Setiap
  kasus tidak sah dicocokkan dengan Python `bytes.decode`.

## 6. Dampak dokumentasi & tooling

- [x] `website/panduan/07_PUSTAKA_STANDAR.md` §2.18
- [x] `CHANGELOG.md`

## 7. Risiko & alternatif

Data yang dulu didekode secara keliru kini menaikkan `GalatBita`. Validator
C (misalnya di `src/pola.c`) akan lebih cepat, tetapi modul ini sengaja
ditulis dalam bahasa Indonesiaku. Memindahkannya ke C adalah perubahan
tersendiri.
