# 08 — Keputusan TLS untuk HTTPS (Fase 11 langkah 2)

> Tanggal: 2026-11-02. Semua angka di dokumen ini **diukur**, bukan
> diperkirakan: setiap opsi dikompilasi di container bersih
> (`debian:bookworm-slim`) dan handshake HTTPS diuji ke host sungguhan.

## 1. Mengapa ini terblokir

`http_ambil()` hanya bisa `http://`. Menambah `https://` butuh pustaka TLS,
dan di situlah masalahnya: biner `indk` saat ini hanya menaut libc dan libm.

```
$ ldd indk
    linux-vdso.so.1
    libm.so.6
    libc.so.6
    ld-linux-x86-64.so.2
```

Sifat "unduh satu berkas, jalan" itu properti produk, bukan kebetulan. Menaut
OpenSSL secara dinamis mengakhirinya: biner jadi bergantung pada
`libssl.so`/`libcrypto.so` versi tertentu di mesin pengguna. Karena itu ROADMAP
menyebutnya **keputusan arsitektur, bukan detail implementasi** — dan karena
itu pula dokumen ini mengukur dulu sebelum memilih.

## 2. Preseden yang sudah ada di repo ini

SQLite **sudah** di-vendor sebagai amalgamation:

| | |
|---|---|
| `src/sqlite3.c` | 9.282.866 bita, 246.245 baris |
| Dampak ke `LDFLAGS` | tidak ada — tetap `-lm` |
| Dependency build baru | tidak ada |

Jadi pertanyaannya bukan "boleh tidak menambah pustaka C besar" — itu sudah
pernah dijawab. Pertanyaannya: **pustaka TLS mana yang bisa diperlakukan sama.**

## 3. Hasil pengukuran

Program uji yang sama (klien TLS: muat CA sistem, verifikasi wajib, set
hostname) dikompilasi dengan tiga cara.

| Opsi | Ukuran biner | `.so` tambahan | Biner tetap mandiri? |
|---|---|---|---|
| OpenSSL dinamis | 16.280 B | 2 (`libssl`, `libcrypto`) | **tidak** |
| OpenSSL statis | **5.108.112 B** | 0 | ya |
| mbedTLS dinamis | 16.872 B | 3 | **tidak** |
| mbedTLS statis | **662.136 B** | 0 | ya |

Arsip statisnya sendiri:

```
libcrypto.a      9.138.892
libssl.a         1.233.682     → OpenSSL total ±10,4 MB
libmbedcrypto.a    828.170
libmbedtls.a       318.320
libmbedx509.a      159.890     → mbedTLS total ±1,3 MB
```

**OpenSSL statis menambah ±5,1 MB ke biner; mbedTLS statis ±0,66 MB.** Biner
`indk` sekarang 1,42 MB, jadi OpenSSL akan membuatnya **4,6× lebih besar**,
mbedTLS **1,5×**.

## 4. Apakah mbedTLS bisa di-vendor seperti sqlite3.c?

Ini syarat penentu, karena rantai rilis membangun di Linux, macOS, dan Windows
(MinGW) tanpa manajer paket. Diuji dengan mbedTLS **3.6.2 (LTS)**:

```
.c di library/   : 108 berkas
baris            : 132.812
ukuran library/  : 5,7 MB   include/ : 2,4 MB
```

Kompilasi polos, tanpa cmake, tanpa `./configure`:

```
gcc -c -O2 -Iinclude -Ilibrary library/ssl_tls.c   →  rc=0
gcc -O2 -Iinclude -Ilibrary klien.c library/*.c    →  0 error
```

**Bisa.** mbedTLS dirancang untuk disalin ke dalam proyek lain; OpenSSL tidak
(butuh `Configure` Perl, menghasilkan header per-platform).

## 5. Bukti keamanan, bukan sekadar "berhasil terhubung"

Biner uji hasil vendoring itu dijalankan ke host sungguhan:

```
$ ./https example.com
handshake OK: TLSv1.3 / TLS1-3-CHACHA20-POLY1305-SHA256, verify=0
respons: HTTP/1.1 200 OK

$ ./https wrong.host.badssl.com
handshake GAGAL -0x2700
```

Dua hal yang dibuktikan sekaligus:

1. TLS 1.3 bekerja dengan CA sistem (`verify=0` berarti tidak ada temuan).
2. **Sertifikat dengan hostname salah DITOLAK.** `-0x2700` adalah
   `MBEDTLS_ERR_X509_CERT_VERIFY_FAILED`. Ini syarat "insecure-by-default
   dilarang" di ROADMAP, dan dibuktikan dengan host yang sengaja salah —
   bukan diasumsikan dari membaca dokumentasi.

## 6. Masalah yang tersisa: dari mana CA bundle diambil

mbedTLS **tidak** membawa daftar CA. Lokasinya berbeda tiap sistem:

| Sistem | Jalur |
|---|---|
| Debian/Ubuntu | `/etc/ssl/certs/ca-certificates.crt` (180.744 B) |
| Alpine | `/etc/ssl/certs/ca-certificates.crt` (217.769 B) |
| macOS | Keychain — **tidak ada berkas PEM bawaan** |
| Windows | Certificate Store — **tidak ada berkas PEM bawaan** |

Ini bagian yang paling mudah dikerjakan asal-asalan. Tiga pilihan:

- **A. Bundel CA ikut di-vendor.** Selalu bekerja, tetapi daftar CA jadi basi
  dan hanya bisa diperbarui lewat rilis baru. Berbahaya: CA yang dicabut tetap
  dipercaya sampai pengguna memperbarui.
- **B. Pakai penyimpanan sistem.** Benar dan selalu mutakhir, tetapi butuh kode
  khusus per-OS (Keychain di macOS, CertOpenSystemStore di Windows).
- **C. Jalur dari lingkungan + jalur bawaan per-OS, galat jelas bila tak ada.**

**Rekomendasi: C sekarang, B menyusul.** Pengguna tidak boleh mendapat
"berhasil" yang diam-diam tidak memverifikasi apa pun; lebih baik gagal
dengan pesan yang menyebut cara menunjuk bundel CA.

## 7. Rekomendasi

**Vendor mbedTLS 3.6.2 (LTS) ke `src/vendor/mbedtls/`, tautkan statis.**

Alasannya, berurutan:

1. **Sifat mandiri biner dipertahankan** — syarat yang membuat ini terblokir
   sejak awal.
2. **Biaya ukuran 7,7× lebih kecil** dari OpenSSL statis (0,66 MB vs 5,1 MB).
3. **Bisa di-vendor tanpa dependency build baru** di tiga OS, persis seperti
   `sqlite3.c` yang sudah ada.
4. **3.6.2 adalah rilis LTS** dengan dukungan panjang — "pilih TLS library
   yang terawat" terpenuhi.
5. **Secure-by-default terbukti**, bukan diklaim.

### Yang dikorbankan, ditulis terang-terangan

- **Permukaan audit bertambah 132.812 baris C.** Ini nyata. Yang menahannya:
  mbedTLS dirancang untuk disematkan dan punya rekam jejak di perangkat
  tertanam, tetapi tetap saja ini kode yang tidak kita tulis dan harus
  diperbarui saat ada CVE.
- **Waktu build bertambah** karena 108 berkas .c tambahan.
- **Kewajiban baru:** mengikuti rilis keamanan mbedTLS. Tanpa itu, vendoring
  berubah dari keuntungan menjadi utang.
- **TLS untuk server (`app.jalankan`) tidak ikut** dalam langkah ini. Langkah 2
  hanya tentang klien `http_ambil()`. Server HTTPS tetap lewat reverse proxy
  seperti di `08_DEPLOYMENT.md`.

### Alternatif yang ditolak

- **OpenSSL dinamis** — mengakhiri sifat mandiri biner. Ini alasan asli
  pemblokiran; tidak masuk akal memilihnya sekarang.
- **OpenSSL statis** — +5,1 MB dan tidak bisa di-vendor tanpa Perl `Configure`,
  jadi rantai rilis tiga OS harus memasang OpenSSL sendiri.
- **Memanggil `curl` sebagai proses anak** — menambah dependency runtime yang
  tidak bisa dijamin ada, dan AGENTS melarang mengeksekusi perintah shell
  otomatis untuk mengimplementasikan protokol.
- **Menulis TLS sendiri** — tidak akan pernah menjadi pilihan serius.

## 8. Keputusan

**Belum diambil.** Dokumen ini menyediakan bahannya. Yang dibutuhkan dari
pemilik proyek hanya satu hal: setuju atau tidak menambah 132.812 baris C
pihak ketiga ke repo demi HTTPS dengan biner yang tetap mandiri.

Jika setuju, urutan kerjanya: vendor `src/vendor/mbedtls/` → builtin TLS di
`native.c` → `http_ambil()` menerima `https://` → tes terhadap host sungguhan
termasuk sertifikat salah → dokumentasi.
