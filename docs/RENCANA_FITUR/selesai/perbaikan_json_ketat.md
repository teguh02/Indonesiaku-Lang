# Rencana Perbaikan: ketepatan `json_urai`/`json_teks` (T-U15)

- **Status:** selesai
- **Target versi:** 1.4.1
- **Pemilik:** Teguh Rijanandi (dikerjakan oleh Claude Code)

## 1. Masalah

Temuan T-U15 dari uji ekstensif Windows 1.4.0 (`PUSTAKA-03`, `-08`, `-09`).

Dekode:

- `"é"` menjadi `"u00e9"`, dan `"\b\f"` menjadi `"bf"`.
- Escape tidak sah (`\q`) diterima tanpa galat.
- Data tambahan (`"1 2"` → `1`, `"[1]x"`, `"truex"`) dan angka cacat
  (`"-"` → `0`, `"01"`, `"1."`) diterima.

Enkode:

- `json_teks(chr(1))` menulis byte kontrol mentah.
- `[inf, nan]` menjadi `[takhingga,nan]`.
- `json_teks({1: "satu"})` crash dengan pesan internal dari `json_escape`.

## 2. Sintaks usulan

Tidak ada sintaks baru.

## 3. Semantik

Mengikuti RFC 8259 dan modul `json` Python.

`json_urai`:

- escape `\" \\ \/ \b \f \n \r \t \uXXXX`, termasuk pasangan surrogate;
- surrogate tunggal ditolak, karena teks berupa UTF-8 dan surrogate tidak
  punya bentuknya;
- escape lain, karakter < U+0020 mentah di dalam string, angka di luar
  `-?(0|[1-9]\d*)(\.\d+)?([eE][+-]?\d+)?`, kunci objek yang bukan string, dan
  data tambahan setelah nilai menaikkan galat.

`json_teks`:

- karakter kontrol di-escape sebagai `\u00XX`;
- `nan`/`takhingga` ditolak, seperti `allow_nan=False`;
- kunci angka/logika/kosong diubah ke teks seperti `json.dumps`; tipe kunci
  lain ditolak dengan pesan yang jelas.

Galat tetap berupa teks yang dinaikkan, seperti sebelumnya.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `pustaka/json.idk` | `_json_urai_string` ditulis ulang dengan `_JSON_ESCAPE`, `_json_urai_u` (surrogate), dan potongan + `teks_gabung` (dulu kuadratik). Ditambah `_json_digit_wajib` untuk tata bahasa angka, pemeriksaan data tambahan di `json_urai`, serta `_json_u4` dan `_json_kunci` di sisi enkode. |

## 5. Pengujian

- Golden `examples/json_ketat.idk`: setiap kasus dari ketiga temuan, kasus
  yang sah (`-0`, `0.5`, `-1.5e+3`, `1E2`, spasi di tepi), dan bolak-balik
  struktur campuran. Hasil sah/tidak sah dicocokkan dengan Python `json`.
- Golden JSON yang sudah ada (`json_pakai`, `todo_demo`, server web) tetap
  lulus.

## 6. Dampak dokumentasi & tooling

- [x] `website/panduan/07_PUSTAKA_STANDAR.md` §2.5
- [x] `CHANGELOG.md` (⚠️)

## 7. Risiko & alternatif

Masukan tidak sah yang dulu diterima kini menaikkan galat. Itulah yang
dijanjikan panduan ("kesalahan dapat ditangkap"). Pemanggil yang mengirim
JSON dengan data tambahan perlu memperbaikinya.
