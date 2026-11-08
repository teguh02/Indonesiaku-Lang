# Rencana Fitur: keamanan web dan autentikasi (Fase 13)

- **Status:** selesai
- **Target versi:** 1.6.0
- **Pemilik:** Teguh R

Dokumen payung Fase 13. Rencana per-fitur ada di berkas tersendiri:
`web_kuki_sesi.md` (WEB-04), `web_csrf.md` + `web_cors_proxy.md` (WEB-05),
`web_auth.md` (WEB-06).

## 1. Masalah

Sampai Fase 12 bahasa ini bisa melayani HTTP dan merender HTML, tetapi tidak
punya satu pun alat untuk membedakan pengunjung satu dari yang lain. Tanpa
sesi tidak ada login; tanpa login tidak ada izin; tanpa izin "aplikasi web"
hanya berarti halaman statis yang kebetulan dinamis.

Penghalangnya bahkan lebih rendah dari itu: `http_response()` menyusun empat
header tetap lalu menutup blok header, sehingga **`Set-Cookie` mustahil
dikirim**. Satu batasan ini memblokir keenam butir Fase 13 sekaligus.

## 2. Threat model

Siapa yang dilawan, dan dengan asumsi apa.

### Yang diasumsikan BENAR

- Mesin server tidak dikuasai penyerang. Memori proses, berkas basis data,
  dan berkas sumber dianggap tidak terbaca pihak luar.
- TLS ditangani di depan (Nginx/Caddy). Pustaka ini **tidak** mengasumsikan
  trafiknya sendiri terenkripsi, karena itu flag `Secure` dapat dimatikan
  untuk pengembangan lokal.
- Peramban pengguna menjalankan same-origin policy dengan benar.

### Lawan yang diperhitungkan

| # | Lawan | Kemampuannya | Pertahanan |
|---|---|---|---|
| A | **Pengunjung biasa yang penasaran** | Mengubah URL, ID, dan medan formulir | Otorisasi diperiksa di server pada setiap permintaan, bukan dengan menyembunyikan tautan |
| B | **Situs lain di internet** | Membuat korban mengirim permintaan ke kita, lengkap dengan kukinya | Token CSRF per sesi + `SameSite=Strict` |
| C | **Penyerang XSS** | Menyisipkan HTML/JS lewat data yang kita simpan | Escaping otomatis di templat; kuki `HttpOnly` sehingga sesi tak terbaca JS |
| D | **Penebak sandi** | Mencoba ribuan kombinasi | PBKDF2 600k iterasi + `PembatasPercobaan` |
| E | **Pencuri basis data** | Membaca seluruh tabel pengguna | Sandi ter-hash dengan garam acak per baris; tabel pelangi tidak berlaku |
| F | **Penyadap jaringan pasif** | Membaca trafik tak terenkripsi | Flag `Secure`; TLS di proxy depan |
| G | **Klien di belakang proxy** | Memalsukan `X-Forwarded-For` | Header hanya dipercaya dari alamat proxy terdaftar; entri paling kanan |
| H | **Pengguna sah yang nakal** | Punya sesi sendiri yang valid | Kepemilikan objek diperiksa, bukan sekadar "sudah login" |

### Yang sengaja TIDAK dilawan

- **Penyerang dengan akses baca ke memori proses.** Toko sesi ada di memori;
  siapa pun yang dapat membacanya sudah menguasai server.
- **DoS terdistribusi.** `PembatasPercobaan` menahan tebakan sandi, bukan
  banjir trafik. Itu urusan lapisan di depan.
- **Serangan waktu pada jaringan.** Perbandingan token dibuat waktu-tetap,
  tetapi derau jaringan jauh lebih besar dari selisih yang tersisa.
- **Pengguna yang sudah terinfeksi malware di peramban.** Tidak ada yang bisa
  dilakukan server untuk itu.

## 3. Matriks fitur minimum

| Kebutuhan | Fitur | Status |
|---|---|---|
| Membedakan pengunjung | `TokoSesi`, `set_kuki`, `baca_kuki` | selesai (WEB-04) |
| Kuki tidak terbaca JS | flag `HttpOnly` | selesai |
| Kuki tidak ikut lintas situs | flag `SameSite` | selesai |
| Sesi tak dapat ditanam | `putar()` saat login | selesai |
| Menolak POST lintas situs | `token_csrf`, `periksa_csrf`, `wajib_csrf` | selesai (WEB-05) |
| Keluaran tidak menjadi kode | `templat.aman` + escaping otomatis | selesai (Fase 12) |
| Membatasi pembaca lintas asal | `app.cors()` | selesai (WEB-05) |
| Mengetahui alamat klien sebenarnya | `app.proxy_tepercaya()`, `soket_alamat_peer()` | selesai (WEB-05) |
| Menyimpan sandi | `hash_sandi`, `periksa_sandi` | selesai (WEB-06) |
| Menaikkan biaya hash bertahap | `perlu_hash_ulang` | selesai |
| Membedakan peran | `wajib_peran` (401 vs 403) | selesai |
| Menahan tebakan beruntun | `PembatasPercobaan` | selesai |
| Contoh yang menyatukan semuanya | `examples/catatan_app.idk` | selesai |
| Bukti bahwa semua itu bekerja | `tests/keamanan_app.sh` | selesai |

## 4. Batasan yang diketahui dan dibiarkan terbuka

- **Toko sesi dan pembatas percobaan ada di memori proses.** Hilang saat
  server dimulai ulang, dan dua proses tidak berbagi keadaan. Penyimpanan
  bersama menunggu Fase 20.
- **Argon2id tidak dipakai** meski roadmap menyebutnya lebih dulu: ia menuntut
  pustaka C pihak ketiga, sedangkan proyek ini hanya bergantung pada libc dan
  mbedTLS. PBKDF2 600k iterasi (rekomendasi OWASP) dipakai sebagai gantinya.
- **Login memblokir server ±0,8 detik** pada iterasi bawaan, karena servernya
  satu-permintaan-sekali. Itu konsekuensi langsung dari hashing yang mahal;
  angkanya terukur dan tertulis di panduan.
- **Unggah berkas berbahaya** disebut di butir 6 roadmap tetapi tidak diuji:
  `web` memang mengurai `multipart/form-data`, namun belum ada API penyimpanan
  berkas yang punya permukaan serang untuk diuji. Menunggu fitur itu ada.

## 5. Rencana uji

Setiap fitur punya tes golden dan tes lewat soket nyata sendiri. Di atasnya,
`tests/keamanan_app.sh` menyerang aplikasi utuh `examples/catatan_app.idk`
melalui HTTP: fixation, CSRF, izin lintas pengguna, XSS tersimpan, SQL
injection, enumerasi nama, kuncian, dan flag kuki.

Delapan pertahanan dilucuti satu per satu untuk membuktikan tesnya memang
dapat memerah; kedelapannya terdeteksi.
