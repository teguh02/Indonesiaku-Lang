# Rencana fitur: API database bersama (`pustaka/db.idk`)

**Status:** selesai
**Fase roadmap:** 10 (WDB-04)
**Prasyarat:** WDB-01 (SQLite), WDB-02 (MySQL/MariaDB), WDB-03 (PostgreSQL) — selesai.

## Masalah

Tiga backend sudah jalan, tetapi dipakai dengan tiga cara berbeda:

| | buka | jalankan | hasil |
|---|---|---|---|
| SQLite | `db_buka(berkas)` | `db_eksekusi(db, sql, par)` | fungsi global `db_*` |
| MySQL | `my.sambung(konf)` | `k.eksekusi(sql, par)` | metode objek |
| PostgreSQL | `pg.sambung(konf)` | `k.eksekusi(sql, par)` | metode objek |

Kode yang ingin berpindah backend harus ditulis ulang, bukan karena SQL-nya
berbeda melainkan karena **bentuk pemanggilannya** berbeda. SQLite paling
timpang: ia satu-satunya yang memakai fungsi global.

Rencana `database_multi_driver.md` menunda WDB-04 sampai ada dua driver nyata,
dengan alasan menyatukan satu driver adalah abstraksi spekulatif. Syarat itu
sekarang terpenuhi.

## Yang TIDAK dikerjakan (dan alasannya)

**Penerjemahan placeholder tidak dilakukan.** PostgreSQL memakai `$1`,
SQLite dan MySQL memakai `?`. Menerjemahkannya berarti memindai teks SQL, dan
pemindai yang tidak memahami literal string akan merusak query yang sah:

```sql
SELECT * FROM t WHERE nama = 'siapa?' AND id = ?
```

Tanda tanya pertama ada di dalam literal dan bukan parameter. Pemindai yang
salah menerjemahkannya menghasilkan query rusak atau, lebih buruk, parameter
yang bergeser posisi. Risikonya tidak sebanding dengan kenyamanan yang didapat,
jadi **SQL tetap ditulis sesuai dialek backend**. Ini dinyatakan terbuka di
panduan, bukan disembunyikan.

Juga di luar lingkup: pool koneksi, ORM, migrasi skema (WDB-05), dan
penyeragaman tipe data lintas backend.

## API

```
impor db

# satu pintu masuk; "jenis" menentukan backend
variabel k = db.sambung({"jenis": "sqlite", "berkas": "data.db"})
variabel k = db.sambung({"jenis": "mysql", "host": "...", "pengguna": "..."})
variabel k = db.sambung({"jenis": "postgres", "host": "...", "basisdata": "..."})

k.eksekusi(sql, parameter)      # -> jumlah baris terpengaruh
k.ambil_semua(sql, parameter)   # -> daftar kamus
k.ambil_satu(sql, parameter)    # -> kamus, atau kosong
k.mulai() / k.komit() / k.batal()
k.tutup()                       # idempoten di ketiga backend
k.jenis()                       # -> "sqlite" | "mysql" | "postgres"
```

`parameter` boleh diabaikan di ketiga backend (bawaannya daftar kosong).

**API lama tetap berjalan tanpa perubahan:** `db_*` untuk SQLite, serta
`my.sambung`/`pg.sambung` langsung. `db.idk` adalah lapisan tipis di atasnya,
bukan penggantinya.

## Berkas tersentuh

- `pustaka/db.idk` — **baru**. Pembungkus SQLite + penyalur `sambung()`.
  Guard `tertutup` sempat ditulis untuk menjamin `tutup()` idempoten, lalu
  dihapus setelah sabotase membuktikan test tetap hijau tanpanya: `db_tutup()`
  sudah idempoten sendiri, jadi guard itu kode mati.
- `pustaka/my.idk` — `eksekusi`/`ambil_semua`/`ambil_satu`/`kueri` diberi nilai
  bawaan `parameter = kosong` supaya seragam dengan `pg.idk`; tambah `jenis()`.
- `pustaka/pg.idk` — tambah `jenis()`.
- `examples/db_bersama.idk` + `tests/expected/db_bersama.out` — golden SQLite
  (tidak butuh container, jadi aman di suite biasa).
- `tests/db_bersama.sh` — uji ketiga backend terhadap server sungguhan.
- `website/panduan/07_PUSTAKA_STANDAR.md`, `CHANGELOG.md`, `docs/ROADMAP.md`.

## Rencana uji

1. Golden SQLite: CRUD lewat `db.sambung` menghasilkan keluaran yang sama
   dengan `db_*` langsung.
2. Ketiga backend menjawab `jenis()` dengan benar.
3. `parameter` boleh diabaikan di ketiga backend.
4. `ambil_satu` mengembalikan `kosong` pada hasil kosong di ketiga backend.
5. Transaksi: `batal()` benar-benar membatalkan di ketiga backend.
6. `tutup()` idempoten di ketiga backend.
7. Jenis tidak dikenal ditolak dengan pesan jelas, bukan gagal diam.
8. Galat tetap dapat ditangkap sebagai `GalatOS` lintas backend.
9. Sabotase: tiap pemeriksaan di atas harus bisa merah.
