# Rencana uji skenario — aplikasi web SSR lengkap

> **Status: rencana. Belum ada satu pun kasus yang dijalankan.**
> Semua hasil di bawah `⬜ belum diuji` sampai seseorang benar-benar
> menjalankannya dan menyalin keluaran aslinya.

Berbeda dengan `WINDOWS.md` / `LINUX.md` / `MACOS.md` yang menguji **paket
rilis per sistem operasi**, berkas ini menguji **satu skenario aplikasi utuh**:
landing page dengan formulir, endpoint AJAX, halaman admin, dan proteksinya.

Alasannya: katalog kasus di tiga berkas OS itu menguji fitur **satu per satu**
(`SIN-xx` sintaks, `PUS-xx` pustaka). Kelemahan pengujian per-fitur adalah
tiap bagian lulus sendiri-sendiri sementara gabungannya patah — sesi sudah
kedaluwarsa saat CSRF diperiksa, templat lupa meng-escape nilai yang berasal
dari basis data, atau peran admin bocor lewat endpoint AJAX yang tidak ikut
dijaga middleware. Itu kelas bug yang hanya muncul saat fitur dipakai bersama.

---

## 1. Lingkup

### Yang diuji

Satu aplikasi contoh, dibangun khusus untuk rencana ini:

| Bagian | Isi |
|---|---|
| Landing page | Halaman statis SSR + aset (CSS/gambar) lewat `app.statis()` |
| Formulir publik | Formulir kontak: validasi server, CSRF, anti-spam sederhana |
| AJAX | Endpoint `POST` yang membalas JSON, dipanggil `fetch()` dari halaman |
| Halaman admin | Login, sesi, daftar pesan masuk, hapus pesan |
| Proteksi | CSRF, peran, pembatasan percobaan login, escaping, traversal, header |

### Yang TIDAK diuji di sini

- Perilaku khusus OS (jalur, encoding konsol, installer) — itu milik tiga
  berkas OS.
- Beban/kinerja — `tests/beban/` dan `tests/web_badan_performa.sh`.
- Peramban sungguhan. Semua kasus dijalankan lewat HTTP mentah
  (`/dev/tcp` atau `curl`), supaya hasilnya deterministik dan bisa jadi tes
  otomatis. JavaScript di halaman hanya **dibaca**, tidak dieksekusi —
  lihat §6 Batasan.

---

## 2. Kemampuan yang sudah ada

Diaudit dari `pustaka/web.idk` pada `37d4038` (bukan dari ingatan). Rencana
ini hanya boleh memakai nama yang benar-benar ada:

**Modul `web`:**
`Aplikasi`, `TokoSesi`, `PembatasPercobaan`, `GalatWeb` ·
`html()`, `json()`, `teks()`, `alihkan()` ·
`token_csrf()`, `periksa_csrf()`, `wajib_csrf()`, `wajib_peran()` ·
`hash_sandi()`, `periksa_sandi()`, `perlu_hash_ulang()` ·
`set_kuki()`, `baca_kuki()` · `baca_permintaan()`

**Metode `Aplikasi`:** `rute()`, `gunakan()`, `statis()`, `cors()`,
`jalankan()`, `tangani()`, `tulis_galat()`

**Kamus `req`:** `metode`, `path`, `header`, `badan`, `form`, `json`,
`berkas` (multipart), `kueri`

**Modul lain:** `templat` (SSR, auto-escape), `db` (SQLite/MySQL/Postgres),
`json`, `teks`

Artinya **tidak ada fitur baru yang perlu dibangun** untuk skenario ini.
Kalau sebuah kasus di bawah ternyata butuh API yang belum ada, itu sendiri
adalah temuan dan dicatat di §5, bukan diam-diam diakali.

---

## 3. Berkas yang akan dibuat

| Berkas | Isi |
|---|---|
| `examples/situs_lengkap.idk` | Aplikasi yang diuji |
| `examples/situs_lengkap/tampilan/*.html` | Templat: layout, landing, admin, login |
| `examples/situs_lengkap/aset/gaya.css` | Aset statis untuk menguji `app.statis()` |
| `tests/situs_lengkap.sh` | Harness: jalankan server, kirim permintaan, asersi |
| `tests/situs_lengkap.ps1` | Padanan Windows (harness `.ps1` tidak memanggil `.sh`) |

Aplikasi memakai SQLite di `tests/data/situs_lengkap/` dan dihapus di akhir;
`git status --porcelain` wajib bersih sesudahnya.

---

## 4. Katalog kasus

Status: ✅ lulus · ❌ gagal · ⚠️ lulus dengan catatan · ⬜ belum diuji.

### 4.1 Landing page dan aset statis

| ID | Skenario | Cara | Hasil diharapkan | Status |
|---|---|---|---|---|
| WEB-L01 | Halaman depan terbit | `GET /` | 200, `Content-Type: text/html; charset=utf-8`, memuat judul | ⬜ |
| WEB-L02 | `Content-Length` dihitung bita | `GET /` dengan judul ber-emoji | `Content-Length` = jumlah **bita**, bukan karakter | ⬜ |
| WEB-L03 | HEAD dilayani | `HEAD /` | 200, header identik dengan GET (termasuk `Content-Length`), badan kosong, diakhiri CRLF CRLF | ⬜ |
| WEB-L04 | Aset statis | `GET /aset/gaya.css` | 200, `text/css`, isi cocok dengan berkas | ⬜ |
| WEB-L05 | Aset tidak ada | `GET /aset/hantu.css` | 404, bukan 500, tanpa jejak jalur server | ⬜ |
| WEB-L06 | Traversal lewat aset | `GET /aset/../../config.idk` dan bentuk ter-encode `%2e%2e%2f` | 404/400 di kedua bentuk, berkas luar akar tidak pernah terkirim | ⬜ |
| WEB-L07 | Symlink keluar akar | Taruh symlink di `aset/` menunjuk berkas luar, `GET` | Ditolak. Pemeriksaan teks `..` tidak melihat symlink | ⬜ |
| WEB-L08 | Metode salah | `DELETE /` | 405, header `Allow` menyebut `GET, HEAD` | ⬜ |

### 4.2 Formulir kontak

| ID | Skenario | Cara | Hasil diharapkan | Status |
|---|---|---|---|---|
| WEB-F01 | Formulir tampil | `GET /kontak` | 200, memuat `<input name="_csrf"` berisi token | ⬜ |
| WEB-F02 | Kirim sah | `POST /kontak` dengan token + kuki sesi | 303 ke `/kontak?ok=1` (pola POST-redirect-GET) | ⬜ |
| WEB-F03 | Tanpa token CSRF | `POST /kontak` tanpa `_csrf` | 403, pesan tidak menyebut nilai token yang benar | ⬜ |
| WEB-F04 | Token CSRF milik sesi lain | Ambil token sesi A, kirim dengan kuki sesi B | 403 | ⬜ |
| WEB-F05 | Medan wajib kosong | `POST` dengan `nama=` kosong | 400 + pesan validasi, data tidak tersimpan | ⬜ |
| WEB-F06 | Surel tidak sah | `surel=bukan-surel` | 400, ditolak di server (bukan hanya `type=email` di HTML) | ⬜ |
| WEB-F07 | Nilai sangat panjang | `pesan` 1 MB | Ditolak rapi (413/400), server tetap melayani permintaan berikutnya | ⬜ |
| WEB-F08 | Karakter XSS | `nama=<script>alert(1)</script>`, lalu lihat di admin | Tersimpan apa adanya, **ditampilkan ter-escape** (`&lt;script&gt;`) | ⬜ |
| WEB-F09 | Kutip dalam atribut | `nama="><img src=x onerror=1>` ditampilkan dalam atribut HTML | Ter-escape, tidak memecah atribut | ⬜ |
| WEB-F10 | Unicode utuh | `nama=Andrés ☕ 日本` | Tersimpan dan tampil utuh, `Content-Length` tetap benar | ⬜ |
| WEB-F11 | Kutipan SQL | `nama=O'Brien; DROP TABLE pesan;--` | Tersimpan sebagai teks biasa; tabel masih ada (parameter terikat `?`) | ⬜ |

### 4.3 AJAX (JSON)

| ID | Skenario | Cara | Hasil diharapkan | Status |
|---|---|---|---|---|
| WEB-A01 | JSON balasan | `POST /api/cek-surel` dengan `Content-Type: application/json` | 200, `application/json`, badan JSON sah | ⬜ |
| WEB-A02 | `req["json"]` terisi | Badan `{"surel":"a@b.c"}` | Penangan membaca `req["json"]["surel"]`, bukan mengurai sendiri | ⬜ |
| WEB-A03 | JSON rusak | Badan `{"surel":` | 400, **bukan** 500 dan bukan crash | ⬜ |
| WEB-A04 | `Content-Type` salah | Badan JSON dengan `Content-Type: text/plain` | `req["json"]` `kosong`; penangan menolak rapi | ⬜ |
| WEB-A05 | AJAX ikut dijaga CSRF | `POST /api/*` tanpa token | 403. **Endpoint AJAX adalah lubang paling sering**: middleware sesi sering hanya dipasang di rute HTML | ⬜ |
| WEB-A06 | CORS ditolak secara bawaan | `Origin: https://jahat.example` | Tidak ada `Access-Control-Allow-Origin` di balasan | ⬜ |
| WEB-A07 | CORS asal terdaftar | `Origin` yang terdaftar di `app.cors()` | Header muncul, dan **hanya** untuk asal itu | ⬜ |
| WEB-A08 | Preflight | `OPTIONS /api/cek-surel` | 204/200 dengan `Allow`/`Access-Control-Allow-Methods` yang benar | ⬜ |
| WEB-A09 | Balasan JSON tidak bocor rahasia | Picu galat di dalam penangan AJAX | Badan hanya "Galat internal"; nama berkas/variabel hanya ke stderr | ⬜ |

### 4.4 Admin: sesi dan autentikasi

| ID | Skenario | Cara | Hasil diharapkan | Status |
|---|---|---|---|---|
| WEB-S01 | Admin tanpa login | `GET /admin` tanpa kuki | 303 ke `/masuk`, isi admin tidak bocor | ⬜ |
| WEB-S02 | Login benar | `POST /masuk` kredensial sah | 303 ke `/admin`, `Set-Cookie` sesi | ⬜ |
| WEB-S03 | Kuki sesi dijaga | Periksa `Set-Cookie` | Memuat `HttpOnly`, `SameSite`, `Path=/` | ⬜ |
| WEB-S04 | Rotasi ID sesi | Bandingkan ID sesi sebelum dan sesudah login | **Berubah**. ID tetap = session fixation | ⬜ |
| WEB-S05 | Sandi salah | Kredensial salah | 401/kembali ke formulir; pesan **tidak** membedakan "pengguna tidak ada" vs "sandi salah" | ⬜ |
| WEB-S06 | Sandi tidak disimpan polos | Periksa isi tabel pengguna | Hanya hash PBKDF2 (`hash_sandi()`), bukan sandi asli | ⬜ |
| WEB-S07 | Pembatasan percobaan | 6× login gagal berturut-turut | Percobaan ke-6 ditolak `PembatasPercobaan` walau sandinya benar | ⬜ |
| WEB-S08 | Keluar | `POST /keluar` | Sesi dihapus di server; memakai kuki lama sesudahnya → 303 ke `/masuk` | ⬜ |
| WEB-S09 | Kuki sesi karangan | Kirim `sid=aaaa...` acak | Diperlakukan belum login, bukan 500 | ⬜ |
| WEB-S10 | Keluar wajib POST | `GET /keluar` | 405. `GET` membuat logout bisa dipicu `<img src>` | ⬜ |

### 4.5 Admin: otorisasi

| ID | Skenario | Cara | Hasil diharapkan | Status |
|---|---|---|---|---|
| WEB-O01 | Pengguna biasa ke admin | Login peran `pengguna`, `GET /admin` | 403, bukan 200 | ⬜ |
| WEB-O02 | Hapus milik orang lain | Pengguna A menghapus pesan milik B lewat id tebakan | 403/404. Kepemilikan diperiksa **di server**, bukan disembunyikan di UI | ⬜ |
| WEB-O03 | Peran tidak bisa dinaikkan dari klien | Kirim `peran=admin` di formulir | Diabaikan; peran hanya dari sesi server | ⬜ |
| WEB-O04 | Hapus wajib POST | `GET /admin/hapus?id=1` | 405, data tidak terhapus | ⬜ |
| WEB-O05 | ID bukan angka | `id=abc`, `id=-1`, `id=99999` | 400/404 rapi, tidak meledak | ⬜ |
| WEB-O06 | Endpoint AJAX admin | `POST /api/admin/*` dengan sesi pengguna biasa | 403. Diuji terpisah dari WEB-O01 karena jalurnya beda | ⬜ |

### 4.6 Ketahanan

| ID | Skenario | Cara | Hasil diharapkan | Status |
|---|---|---|---|---|
| WEB-K01 | Klien diam | Sambung lalu tidak kirim apa pun | Diputus oleh `soket_batas_waktu`; server tetap melayani klien lain | ⬜ |
| WEB-K02 | Putus di tengah | Kirim separuh permintaan lalu tutup | Tidak crash, tidak bocor fd | ⬜ |
| WEB-K03 | `Content-Length` bohong | Nyatakan 1000, kirim 10 bita | Timeout/400, tidak menggantung selamanya | ⬜ |
| WEB-K04 | Permintaan tidak sah | `"GARBAGE / HTTP/9"` | 400, koneksi berikutnya tetap dilayani | ⬜ |
| WEB-K05 | Kebocoran fd | 200 permintaan campuran (sukses + galat + putus) | `/proc/<pid>/fd` setelah pemanasan ≈ setelah beban (Linux saja) | ⬜ |
| WEB-K06 | Galat di penangan | Rute yang sengaja `naikkan` | 500 rapi, server tetap hidup, jejak hanya ke stderr | ⬜ |

### 4.7 Header keamanan

> `pustaka/web.idk` saat ini **hanya** mengirim `X-Content-Type-Options: nosniff`
> (baris 643). Kasus di bawah sengaja menguji yang belum ada: hasilnya
> diperkirakan ❌ dan itu memang tujuannya — untuk memutuskan mana yang
> layak ditambahkan ke pustaka dan mana yang urusan aplikasi.

| ID | Skenario | Hasil diharapkan | Status |
|---|---|---|---|
| WEB-H01 | `X-Content-Type-Options` | `nosniff` ada | ⬜ |
| WEB-H02 | `X-Frame-Options` / CSP `frame-ancestors` | Ada pada halaman admin (anti-clickjacking) | ⬜ |
| WEB-H03 | `Content-Security-Policy` | Minimal `default-src 'self'` pada halaman ber-AJAX | ⬜ |
| WEB-H04 | Header `Server` | Tidak ada header `Server` sama sekali. Sudah benar hari ini (`pustaka/web.idk` tidak pernah mengirimnya) — kasus ini menjaganya agar tidak ditambahkan tanpa sengaja | ⬜ |

---

## 5. Cara menjalankan dan mencatat

1. Jalankan `bash tests/situs_lengkap.sh` (dan `.ps1` di Windows).
2. Untuk kasus yang gagal, catat di §7 dengan **keluaran asli**, bukan
   ringkasan.
3. Temuan yang menyangkut `pustaka/*.idk` dibuka sebagai issue dengan ID
   kasusnya, lalu diperbaiki lewat alur fitur biasa (satu temuan = satu
   branch = satu PR).
4. Kasus yang ternyata membutuhkan API yang belum ada **tidak boleh**
   diakali di aplikasi contoh. Tandai ❌, tulis API yang kurang, lalu
   putuskan: masuk pustaka, atau memang urusan aplikasi.

### Aturan harness (dari pelajaran yang sudah ada)

Hal-hal berikut sudah pernah membuat tes hijau palsu di repo ini:

- **Bungkus dengan `timeout` DAN tolak exit 124 secara eksplisit.** Galat
  sering tercetak sebelum proses menggantung, sehingga stderr cocok dan tes
  hijau padahal binarinya dibunuh paksa.
- **Tulis respons ke berkas, jangan `$(...)`.** Command substitution memangkas
  baris baru akhir, membuat hitungan `Content-Length` meleset 1 bita dan
  perbandingan header pada HEAD membuang baris terakhir.
- **Saring CRLF** (`tr -d '\r'`) sebelum menggabungkan baris; `indk.exe`
  menulis CRLF dan semua perbandingan gagal di Windows walau nilainya benar.
- **Pakai `/dev/tcp` + `printf`**, bukan curl, supaya jalan di Git Bash tanpa
  dependensi. `curl -I` juga tidak bisa mendeteksi badan yang bocor pada HEAD
  karena curl membuang badannya sendiri.
- **Sabotase tiap asersi sebelum mempercayainya.** Asersi yang tidak pernah
  merah tidak membuktikan apa pun.
- **Daftarkan di `tests/test_all.sh` DAN `tests/test_all.ps1`.** Harness
  Windows tidak memanggil satu pun sub-suite `.sh`.

---

## 6. Batasan yang diketahui

- **JavaScript tidak dieksekusi.** Kasus AJAX menguji endpoint-nya lewat HTTP,
  bukan `fetch()` di peramban sungguhan. Bug yang hanya muncul di peramban
  (CORS preflight nyata, perilaku `SameSite`, CSP yang benar-benar memblokir)
  tidak tertangkap. Menambahkan Playwright/Selenium akan membawa dependensi
  Node ke repo yang sekarang hanya butuh C + bash — ditunda sampai ada alasan
  yang jelas.
- **Escaper khusus URL belum ada.** `templat` meng-escape `& < > " '` untuk isi
  elemen dan atribut ber-kutip, tetapi `href="{{ url }}"` berisi
  `javascript:...` **tidak** ditolak. Batas ini dinyatakan di panduan; WEB-F09
  menguji yang memang dijanjikan, bukan yang tidak.
- **Satu permintaan pada satu waktu.** `app.jalankan()` melayani berurutan,
  jadi kasus balapan (dua permintaan mengubah baris yang sama) tidak
  bermakna sampai ada concurrency (WEB-07 di roadmap).
- **WEB-K05 hanya Linux.** Hitungan fd butuh `/proc`; di OS lain kasus ini
  dilewati, bukan dikarang hasilnya.

---

## 7. Hasil uji

Belum dijalankan. Tabel diisi setelah aplikasi contoh dan harness dibuat.

| Rilis | Tanggal | Penguji | Lulus/Total | Catatan |
|---|---|---|---|---|
| — | — | — | — | Rencana dibuat, belum dieksekusi |
