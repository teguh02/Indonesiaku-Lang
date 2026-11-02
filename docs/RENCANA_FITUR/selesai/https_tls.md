# Rencana Fitur: HTTPS/TLS untuk klien HTTP

- **Status:** selesai
- **Target versi:** 1.5.0
- **Tanggal:** 2026-11-02
- **Dasar keputusan:** [`docs/ANALISIS/08_KEPUTUSAN_TLS.md`](../ANALISIS/08_KEPUTUSAN_TLS.md)
  (disetujui pemilik proyek: vendor mbedTLS, menerima +132.812 baris C pihak ketiga)

## Masalah

`http_ambil()` hanya bisa `http://`. URL `https://` ditolak mentah-mentah:

```
GalatJaringan("HTTPS belum didukung (butuh TLS); pakai http://")
```

Akibatnya praktis seluruh internet tidak terjangkau dari Indonesiaku — sebagian
besar situs hari ini menolak HTTP polos atau mengalihkannya ke HTTPS, dan
pengalihan itu sendiri gagal karena tujuannya `https://`.

## Keputusan yang sudah diambil

mbedTLS 3.6.2 (LTS) di-vendor ke `src/vendor/mbedtls/` dan ditaut statis.
Alasan lengkap + angka pengukuran ada di dokumen analisis; ringkasnya: biner
tetap mandiri (tidak ada `.so` baru), +0,66 MB (bukan +5,1 MB seperti OpenSSL),
dan bisa dikompilasi dengan gcc polos seperti `src/sqlite3.c` yang sudah ada.

## Lingkup

**Masuk lingkup:** sisi **klien** — `http_ambil()` menerima `https://`.

**Di luar lingkup:** server HTTPS (`app.jalankan()` tetap HTTP di belakang
reverse proxy, lihat `website/panduan/08_DEPLOYMENT.md`). Alasannya: ini
menambah manajemen sertifikat, pembaruan otomatis, dan SNI sisi server — satu
pekerjaan besar yang berbeda, dan reverse proxy sudah menyelesaikannya hari ini.

## Sintaks

Tidak ada sintaks baru. URL `https://` cukup bekerja:

```indonesiaku
impor http

variabel r = http.http_ambil("https://example.com")
cetak(r.kode)
```

## Semantik

1. **Verifikasi sertifikat WAJIB dan tidak bisa dimatikan.** Tidak ada
   parameter `verifikasi=salah`. Sertifikat kedaluwarsa, hostname tidak cocok,
   atau penerbit tidak dikenal → `GalatJaringan` dengan pesan yang menjelaskan
   sebabnya. Keputusan ini mengikuti ROADMAP: "insecure-by-default dilarang".
   Bila kelak ada kebutuhan sah (uji lokal dengan sertifikat sendiri), jalurnya
   adalah **menambah CA**, bukan mematikan verifikasi.
2. **SNI selalu dikirim** (`mbedtls_ssl_set_hostname`). Tanpa ini, host berbagi
   IP mengembalikan sertifikat yang salah, dan verifikasi hostname kehilangan
   maknanya.
3. **TLS 1.2 minimum.** TLS 1.0/1.1 sudah dicabut; mbedTLS 3.x memang sudah
   tidak mendukungnya, jadi ini hanya penegasan.
4. **Port bawaan 443** untuk `https://`, tetap 80 untuk `http://`.
5. **Pengalihan lintas skema** mengikuti aturan yang sudah ada: `http://` →
   `https://` diizinkan (naik kelas). Sebaliknya, `https://` → `http://`
   adalah **penurunan keamanan** dan ditolak — diam-diam mengikutinya akan
   membuat data yang tadinya terenkripsi dikirim polos.
6. **Batas waktu** memakai `soket_batas_waktu` yang sudah ada, jadi handshake
   yang menggantung tidak membekukan program.

## CA bundle: dari mana daftar penerbit tepercaya diambil

mbedTLS tidak membawa daftar CA. Urutan pencarian, berhenti di yang pertama ada:

1. Variabel lingkungan `INDK_CA_BUNDLE` (berkas PEM) — jalur keluar untuk
   sertifikat internal perusahaan dan untuk pengujian.
2. Jalur sistem yang umum, dicoba berurutan:
   - `/etc/ssl/certs/ca-certificates.crt` (Debian/Ubuntu/Alpine)
   - `/etc/pki/tls/certs/ca-bundle.crt` (Fedora/RHEL)
   - `/etc/ssl/cert.pem` (macOS/BSD)
3. Tidak ada satu pun → **galat yang jelas**, bukan diam-diam melewati
   verifikasi:

   ```
   GalatJaringan("tidak menemukan bundel CA sistem; setel INDK_CA_BUNDLE
                  ke berkas PEM")
   ```

Windows sengaja belum dilayani di langkah ini: Certificate Store butuh kode
Win32 terpisah, dan menjanjikannya setengah jadi lebih buruk daripada
mengatakan "setel `INDK_CA_BUNDLE`". Ini ditulis apa adanya di panduan.

## Berkas yang tersentuh

| Berkas | Perubahan |
|---|---|
| `tools/vendor_mbedtls.sh` | **baru** — vendoring yang bisa diulang + sha256 terkunci |
| `src/vendor/mbedtls/` | **baru** — `library/`, `include/`, `LICENSE`, `VERSI` |
| `Makefile` | kompilasi sumber mbedTLS, `-Isrc/vendor/mbedtls/include` |
| `src/tls.c`, `src/tls.h` | **baru** — pembungkus tipis mbedTLS |
| `src/native.c` | builtin `tls_sambung`, `tls_tulis`, `tls_baca`, `tls_tutup` |
| `pustaka/http.idk` | `http_urai_url()` menerima `https://`; `_http_satu_permintaan()` memakai TLS |
| `.github/workflows/test.yml` | sumber baru masuk daftar build manual |
| `website/panduan/07_PUSTAKA_STANDAR.md` | builtin `tls_*` + catatan CA bundle |
| `website/panduan/02_INSTALASI.md` | perintah gcc manual ikut berubah |
| `examples/`, `tests/` | golden + tes jaringan |
| `CHANGELOG.md`, `docs/ROADMAP.md` | status |

## Rencana uji

Yang membuat fitur ini benar bukan "berhasil mengambil halaman", melainkan
**menolak hal yang harus ditolak**. Karena itu tes negatif memakai host uji
publik `badssl.com` yang memang disediakan untuk ini:

| Uji | Harapan |
|---|---|
| `https://example.com` | 200, isi terbaca |
| `https://expired.badssl.com` | `GalatJaringan` (kedaluwarsa) |
| `https://wrong.host.badssl.com` | `GalatJaringan` (hostname) |
| `https://self-signed.badssl.com` | `GalatJaringan` (penerbit) |
| `https://untrusted-root.badssl.com` | `GalatJaringan` (penerbit) |
| `http://` → `https://` pengalihan | diikuti |
| `https://` → `http://` pengalihan | ditolak |
| `INDK_CA_BUNDLE` salah jalur | galat jelas, bukan verifikasi dilewati |

**Tes jaringan tidak boleh membuat CI rapuh.** Semua uji di atas masuk skrip
terpisah (`tests/https_klien.sh`) yang **dilewati dengan pesan** bila jaringan
tidak tersedia, bukan dinyatakan gagal. Golden `examples/` tidak boleh
menyentuh jaringan sama sekali.

Selain itu: sabotase (matikan verifikasi hostname, terima sertifikat apa pun)
harus membuat tes merah. Kalau tidak, tesnya yang salah.

## Catatan pelaksanaan

- **CCM dicabut** (`src/mbedtls_user_config.h`): tidak dipakai TLS 1.3, dan
  CodeQL melaporkan 3 alert di `ccm.c`. Alert-nya sendiri **false positive**
  (`iv_len` divalidasi 7..13, upstream menulis "Also implies q is within
  bounds"), tetapi mencabut modul yang memang tidak terpakai lebih baik
  daripada men-dismiss alert.
- **Windows perlu `-lbcrypt`** — mbedTLS mengambil entropi lewat
  `BCryptGenRandom`. Kegagalan ini hanya muncul di Windows, jadi Linux/macOS
  hijau tidak membuktikan apa pun.
- **Jangan percaya satu kali jalan ke badssl.com.** Host itu membatasi laju;
  saat diputus sebelum sertifikat dinilai, pesan berubah menjadi "EOF" dan
  tampak seperti regresi. Dibuktikan bukan regresi lewat A/B tiga biner.

## Risiko

- **Permukaan audit bertambah 132.812 baris C.** Diterima secara sadar;
  konsekuensinya ada kewajiban memperbarui saat mbedTLS merilis perbaikan
  keamanan. `tools/vendor_mbedtls.sh` ada supaya itu satu perintah.
- **Waktu build bertambah** karena 108 berkas `.c` tambahan.
- **Windows belum memakai Certificate Store**, hanya `INDK_CA_BUNDLE`.
  Ditulis jujur di panduan, bukan disembunyikan.
