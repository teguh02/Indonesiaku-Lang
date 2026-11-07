# Rencana Fitur — Uji kegagalan TLS dan timeout di driver DB

**Status:** selesai
**Fase:** 10 (butir 7, sisa terakhir)
**Target rilis:** `v1.7.0`
**Penulis:** Teguh Rijanandi
**Tanggal:** 2026-11-07

## Masalah

Butir 7 Fase 10 menuntut suite driver mencakup **kegagalan TLS** dan
**timeout**. Audit cakupan menemukan keduanya tidak punya pemeriksaan sendiri:
kata `tls` tidak muncul di `tests/pg_driver.sh` maupun `tests/my_driver.sh`,
dan `batas_waktu` tidak muncul di keduanya.

Berbeda dengan BLOB, pengujian awal menunjukkan **perilakunya sudah benar** —
yang hilang adalah tesnya. Dua perilaku yang perlu dikunci:

1. **Anti-downgrade TLS.** `pg.idk` meminta `SSLRequest`; bila server menjawab
   selain `S`, koneksi **dibatalkan**, bukan diam-diam diturunkan ke teks
   polos. Ini jaminan keamanan: tanpa tes, seseorang bisa "memperbaiki"
   kegagalan koneksi dengan menambahkan fallback dan tidak ada yang merah.
2. **Timeout soket.** Server yang menerima koneksi lalu diam tidak boleh
   menggantung klien selamanya.

## Hambatan yang ditemukan

Batas waktu di kedua driver **hardcoded 30 detik**
(`soket_batas_waktu(s, 30)`). Menguji timeout apa adanya berarti setiap
jalannya CI membuang 30 detik menunggu — cukup lambat untuk membuat orang
menghapus tesnya.

## Rancangan

### Perubahan kode (minimal)

Satu kunci konfigurasi opsional di kedua driver:

```indonesiaku
variabel batas_waktu = 30
jika "batas_waktu" dalam konfigurasi { batas_waktu = konfigurasi["batas_waktu"] }
soket_batas_waktu(s, batas_waktu)
```

Bawaannya tetap 30 detik, jadi tidak ada perilaku lama yang berubah. `0`
berarti tanpa batas, mengikuti semantik `soket_batas_waktu()` yang sudah ada.

Ini bukan penambahan demi tes semata: 30 detik terlalu lama untuk aplikasi
web yang ingin gagal cepat, dan terlalu pendek untuk kueri analitik.

### Yang TIDAK dikerjakan

- **Tidak menambah TLS ke driver MySQL.** `my.idk` memang belum punya jalur
  TLS, dan menambahkannya adalah fitur tersendiri (jabat tangan
  `SSLRequest` MySQL berbeda dari PostgreSQL), bukan bagian dari menutup
  cakupan uji. Dicatat sebagai batas yang diketahui.
- **Tidak menguji verifikasi sertifikat.** Butir ini soal *kegagalan* TLS;
  rantai sertifikat adalah pekerjaan terpisah dan sudah diuji di jalur
  `https_klien`.

## Pengujian

Keduanya memakai infrastruktur yang sudah ada — tidak ada container baru.

**TLS (`tests/pg_driver.sh`):** container `postgres:16-alpine` yang dipakai
suite berjalan dengan `ssl=off` (diverifikasi: `show ssl` → `off`). Jadi
meminta `"tls": benar` ke container yang sama **harus** ditolak dengan pesan
yang menyebut penolakan, dan **tidak boleh** menghasilkan koneksi sukses.

**Timeout (`tests/pg_driver.sh` dan `tests/my_driver.sh`):** server bisu
Python 10 baris yang menerima koneksi lalu diam. Dengan `batas_waktu` 2
detik, `sambung()` harus menaikkan galat dalam hitungan detik, bukan 30.
Tes memeriksa **dua hal**: galatnya muncul, dan waktu yang berlalu < 10 detik
— tanpa pemeriksaan durasi, timeout 30 detik yang tak terkonfigurasi tetap
akan lulus.

Sabotase yang wajib membuatnya merah:

1. `pg.idk` menerima jawaban non-`S` dan lanjut teks polos → tes TLS merah;
2. `batas_waktu` diabaikan (dipaksa kembali ke 30) → tes durasi merah.
