# Rencana Fitur: contoh website dinamis + dokumentasi (SSR-10)

- **Status:** selesai
- **Target versi:** 1.7.0
- **Pemilik:** teguh02

## 1. Masalah

Butir SSR-10 meminta tiga hal. Diaudit satu per satu:

| diminta | status |
|---|---|
| dokumentasi templat di `website/panduan/` | **sudah ada** — §2.22 `07_PUSTAKA_STANDAR.md`, 16 penyebutan, lengkap dengan escaping, `{% sertakan %}`, `{% perluas %}`, tabel API |
| contoh website dinamis SQLite (listing + detail) | **tidak ada** |
| deployment & troubleshooting untuk aplikasi templat | **tidak ada** — `08_DEPLOYMENT.md` menyebut templat **0 kali** |

Yang ada sekarang hanya potongan terpisah: `db_sqlite.idk` (DB tanpa web),
`templat_dasar.idk` (templat tanpa DB), `web_router.idk` (web tanpa
keduanya). Tidak ada satu pun yang menyambung ketiganya — padahal justru
sambungan itulah yang dipakai orang, dan tempat kesalahan nyata muncul
(SQL injection, escaping, N+1, 404 untuk id tak dikenal).

## 2. Sintaks usulan

Tidak ada sintaks baru. SSR-10 murni pemakaian yang sudah ada.

## 3. Semantik

Tidak ada perubahan semantik.

## 4. Rencana implementasi

`examples/web_mahasiswa.idk` — satu berkas, menyambung tiga pustaka:

- SQLite in-memory agar golden deterministik dan tidak meninggalkan berkas.
- Dua rute: `/mahasiswa` (listing) dan `/mahasiswa/:id` (detail).
- Templat ditulis oleh contohnya sendiri ke direktori sementara, supaya
  contoh benar-benar bisa dijalankan apa adanya (pola yang sudah dipakai
  §2.22).
- `{% perluas %}` untuk layout bersama, `{% untuk %}` untuk listing.
- Dijalankan lewat `app.tangani()` (fungsi murni), **bukan** `app.jalankan()`
  yang membuka port — golden harus deterministik di CI.

Yang wajib ditunjukkan karena di situlah kesalahan nyata terjadi:

1. Parameter SQL diikat `?`, tidak dirangkai `+`.
2. Id tak dikenal → **404**, bukan 500 atau halaman kosong.
3. Id non-numerik (`/mahasiswa/abc`) tidak meledak.
4. Nama mahasiswa berisi `<script>` → ter-escape otomatis di HTML.
5. Satu query untuk listing, bukan satu query per baris (N+1).

Dokumentasi: bagian baru `08_DEPLOYMENT.md` khusus aplikasi templat —
cache produksi, direktori templat di Docker, dan troubleshooting yang
memetakan gejala → sebab.

## 5. Rencana uji

Golden `tests/expected/web_mahasiswa.out` lewat `tests/test_all.sh`.
Keluarannya memuat status tiap rute dan potongan HTML, sehingga golden
menangkap: escaping yang mati, 404 yang berubah jadi 200/500, dan listing
yang kehilangan baris.

Sabotase yang wajib merah: escaping dimatikan, 404 jadi 200, parameter
dirangkai alih-alih diikat.

## 6. Yang TIDAK dikerjakan

- **Tidak membuat kerangka/skeleton proyek web.** Butirnya minta contoh,
  bukan generator; generator proyek adalah fase lain (butir 10 fase
  pustaka).
- **Tidak memakai berkas DB di disk.** `:memory:` cukup untuk contoh dan
  menjaga golden deterministik; cara memakai berkas sudah ada di
  `db_sqlite.idk` dan dijelaskan di bagian deployment.
- **Tidak menambah tes server lewat soket.** Jalur HTTP nyata sudah dijaga
  `tests/web_templat_server.sh` (SSR-07); mengulanginya di sini hanya
  memperlambat CI tanpa menambah jaminan.
