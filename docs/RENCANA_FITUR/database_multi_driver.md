# Rencana fitur: driver database multi-backend

**Status:** sedang dikerjakan
**Target versi:** 1.6.0
**Fase roadmap:** 10 (WDB-01..WDB-05, P6)

## Masalah

`db_*` hanya bicara SQLite. Fase 11 langkah 7 menuntut contoh CRUD di atas
"database Fase 10", dan itu tidak bisa dipenuhi selama hanya ada SQLite.

## Keputusan: wire protocol sendiri, bukan libpq

Roadmap menulis "memakai libpq atau pustaka kompatibel yang terawat". Itu
**tidak diambil**, dan alasannya diukur bukan diasumsikan:

1. **Biner mandiri adalah sifat yang sudah dibayar mahal.** `ldd indk` hanya
   menyebut libc dan libm. mbedTLS sengaja di-vendor dan ditaut statis demi itu
   (lihat `docs/ANALISIS/08_KEPUTUSAN_TLS.md`). Menaut libpq mengembalikan
   ketergantungan runtime yang harus dipasang pengguna per OS, dan merusak
   build musl/Alpine yang baru saja ditutup di 1.5.0.
2. **Bahannya sudah ada semua**, tinggal dirangkai:
   - `soket_sambung` + `soket_batas_waktu` (1.5.0)
   - `tls_mulai` untuk `sslmode=require` (1.5.0, verifikasi sertifikat wajib)
   - `sha256` (lama), `hmac_sha256` + `pbkdf2_sha256` (baru, memakai mbedTLS
     yang memang sudah di-vendor — kriptografi tidak ditulis ulang)
   - `soket_tulis(daftar bita)` + `soket_baca_bita` (baru)
3. **Protokol PostgreSQL v3 stabil sejak 2003** dan terdokumentasi resmi.
   Permukaan yang dibutuhkan untuk query parameterized kecil.

Harga yang dibayar, ditulis terang-terangan: kita jadi pemilik implementasi
protokol. Mitigasinya adalah menjaga lingkup tetap sempit (lihat "Bukan
lingkup") dan menguji terhadap server PostgreSQL sungguhan di CI, bukan mock.

## Prasyarat yang ternyata wajib (dan sempat salah didiagnosis)

Tabel status roadmap menyebut Fase 10 "terblokir pustaka klien eksternal".
Itu keliru. Penghalang sebenarnya, ditemukan dengan mencoba:

```
chr(255)  ->  panjang 1, panjang_byte 2
```

`soket_tulis()` hanya menerima teks, dan teks menyimpan byte >127 sebagai
urutan UTF-8 dua byte. Setiap bingkai biner akan rusak di kawat. Karena itu
dua builtin ditambahkan lebih dulu:

- `soket_tulis(soket, daftar_bita)` — menerima daftar bulat 0..255 di samping
  teks. Konvensi daftar byte mengikuti `db_*` untuk BLOB dan `pustaka/bita.idk`,
  bukan konvensi baru.
- `soket_baca_bita(soket)` — daftar bulat 0..255; daftar kosong berarti
  koneksi ditutup rapi, galat dinaikkan untuk batas waktu (sama seperti
  `soket_baca`, karena "" vs timeout pernah jadi bug nyata).

## API

```
impor pg

variabel k = pg.sambung({
    "host": "127.0.0.1", "port": 5432,
    "pengguna": "postgres", "sandi": "...",
    "basisdata": "ujian",
    "tls": salah            # benar -> tls_mulai + verifikasi sertifikat
})

k.eksekusi("INSERT INTO siswa (nama) VALUES ($1)", ["Budi"])
k.ambil_semua("SELECT id, nama FROM siswa WHERE id > $1", [0])
k.ambil_satu("SELECT ... ")
k.mulai() / k.komit() / k.batal()
k.tutup()
```

`db_*` SQLite **tidak disentuh**: kebijakan kompatibilitas roadmap langkah 1.
API abstraksi bersama (WDB-04) ditunda sampai ada dua driver nyata — menyatukan
satu driver adalah abstraksi spekulatif.

## Keamanan

- **Parameter dikirim sebagai parameter protokol** (Parse/Bind), bukan
  dirangkai ke SQL. Ini bukan escaping: nilai tidak pernah melewati parser SQL.
- **Sandi tidak pernah masuk log atau pesan galat.** Diuji.
- **SCRAM-SHA-256** (bawaan PostgreSQL >= 14). `password` polos ditolak;
  `md5` ditolak — keduanya mengirim turunan sandi yang lemah.
- **Iterasi PBKDF2 dibatasi di builtin**: angka itu datang dari server, jadi
  server jahat bisa membekukan klien dengan iterasi raksasa.
- **`tls_mulai` memverifikasi sertifikat dan tidak bisa dimatikan** — sama
  seperti klien HTTPS. Tidak ada `sslmode=disable-verify`.

## Bukan lingkup (sengaja)

- COPY, LISTEN/NOTIFY, cursor, prepared statement bernama yang hidup lintas
  panggilan, tipe array/komposit, connection pool.
- MySQL/MariaDB: protokolnya berbeda dan punya kebutuhan auth sendiri.
  Dikerjakan setelah PostgreSQL terbukti, bukan bersamaan.
- Migrasi skema (WDB-05).

## Rencana uji

Terhadap **PostgreSQL sungguhan dalam container**, bukan mock — mock protokol
hanya menguji pemahaman saya sendiri tentang protokol itu.

1. Sambung + autentikasi SCRAM; sandi salah ditolak dengan galat yang jelas.
2. CRUD: INSERT/SELECT/UPDATE/DELETE, jumlah baris terpengaruh.
3. Parameter: NULL, Unicode, kutip tunggal, string injeksi SQL klasik.
4. Transaksi: komit dan rollback benar-benar berpengaruh.
5. Galat server (tabel tidak ada, sintaks salah) jadi galat yang bisa ditangkap.
6. `tutup()` idempoten; fd tidak bocor setelah banyak koneksi.
7. Sandi tidak muncul di log.
8. Dijalankan di bawah ASan/UBSan dan `-DDEBUG_STRESS_GC`.
