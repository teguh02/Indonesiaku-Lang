# Rencana Fitur: lapisan migrasi skema (WDB-05)

- **Status:** selesai
- **Target versi:** 1.5.0
- **Pemilik:** teguh02

## 1. Masalah

Setelah `pustaka/db.idk` menyatukan tiga backend, tidak ada cara terstruktur
untuk mengubah skema. Hari ini pengguna menulis `CREATE TABLE` di dalam kode
aplikasi dan menebak sendiri apakah DDL itu sudah pernah dijalankan:

```python
impor db
variabel k = db.sambung({"jenis": "sqlite", "berkas": "app.db"})
k.eksekusi("CREATE TABLE siswa (...)")   # galat pada run kedua
```

Akibatnya kode produksi penuh `CREATE TABLE IF NOT EXISTS`, dan perubahan skema
berikutnya (menambah kolom) tidak punya tempat sama sekali — tidak ada catatan
versi skema mana yang sudah terpasang di sebuah basis data.

## 2. Sintaks usulan

Bukan sintaks bahasa; pustaka baru `pustaka/migrasi.idk`.

```python
impor db
impor migrasi

variabel k = db.sambung({"jenis": "sqlite", "berkas": "app.db"})

variabel daftar_migrasi = daftar()
daftar_migrasi.tambah(migrasi.langkah("001", "buat tabel siswa",
    fungsi(k) { k.eksekusi("CREATE TABLE siswa (id INTEGER PRIMARY KEY, nama TEXT)") },
    fungsi(k) { k.eksekusi("DROP TABLE siswa") }))

migrasi.naikkan_semua(k, daftar_migrasi)   # jalankan yang belum terpasang
migrasi.turunkan_satu(k, daftar_migrasi)   # rollback migrasi terakhir
migrasi.status(k, daftar_migrasi)          # daftar versi + terpasang/belum
```

## 3. Semantik

- Versi terpasang dicatat di tabel `_migrasi_indk (versi TEXT PRIMARY KEY,
  nama TEXT, diterapkan_pada TEXT)`. Tabel dibuat otomatis saat pertama kali
  dipakai.
- `naikkan_semua` menjalankan langkah yang versinya belum tercatat, **urut
  menaik menurut versi**, dan berhenti pada galat pertama (langkah sesudahnya
  tidak dijalankan).
- Setiap langkah berjalan di dalam transaksi: `mulai()` → `naik(k)` → catat
  versi → `komit()`. Galat memicu `batal()` lalu dinaikkan ulang sebagai
  `GalatMigrasi`, jadi bisa ditangkap `coba/kecuali`.
- **Peringatan yang harus ditulis di dokumentasi:** MySQL melakukan implicit
  commit pada DDL, jadi rollback transaksional hanya benar-benar aman di
  SQLite dan PostgreSQL. Pada MySQL sebuah langkah yang gagal di tengah dapat
  meninggalkan skema separuh jadi.
- `turun` boleh `kosong`; `turunkan_satu` pada langkah tanpa `turun` menaikkan
  `GalatMigrasi` alih-alih diam-diam menghapus catatan versi.
- Versi duplikat dalam satu daftar adalah galat (`GalatMigrasi`), bukan
  diam-diam dipakai yang terakhir.
- SQL di dalam `naik`/`turun` **tetap ditulis sesuai dialek backend**, sama
  seperti keputusan di `db.idk`. Pustaka ini tidak menerjemahkan apa pun.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/*` | tidak ada — murni pustaka `.idk` |
| `pustaka/migrasi.idk` | `langkah()`, `naikkan_semua()`, `turunkan_satu()`, `status()`, `GalatMigrasi` |
| `examples/migrasi.idk` + `tests/expected/migrasi.out` | contoh golden di SQLite memori |
| `tests/galat/migrasi_turun_kosong.{idk,err}` | rollback tanpa `turun` harus gagal |
| `tests/migrasi_sabotase.sh` | tiap jaminan dilucuti dan wajib membuat tes merah |

## 5. Rencana uji

- Golden: naikkan dua langkah, jalankan ulang (idempoten, 0 langkah),
  rollback satu, status.
- Galat: `turunkan_satu` tanpa `turun`; versi duplikat.
- Sabotase: hapus pencatatan versi (harus merah karena rerun jadi 2 langkah),
  hapus `batal()` pada galat, hapus pengurutan versi.

## 6. Yang TIDAK dikerjakan

**Query builder** (bagian kedua WDB-05) sengaja ditunda. `db.idk` sudah
memutuskan tidak menyembunyikan dialek SQL maupun penanda parameter; sebuah
builder yang merakit SQL otomatis harus mengambil keputusan dialek itu, jadi ia
butuh keputusan desain tersendiri, bukan ditempelkan ke migrasi.
