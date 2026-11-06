# Rencana Fitur: integrasi render templat ke respons web (SSR-07)

- **Status:** selesai
- **Target versi:** 1.7.0
- **Pemilik:** teguh02

## 1. Masalah

Butir SSR-07 meminta tiga hal: Content-Type `text/html; charset=utf-8`,
status/header yang tepat, dan halaman galat yang tidak membocorkan rahasia.
Diperiksa langsung lewat `app.tangani()` (fungsi murni, tanpa soket):

```text
GET /ok      -> 200, text/html; charset=utf-8, Content-Length 18 (benar, byte)
GET /rusak   -> 500 "Galat internal"; detail hanya ke stderr          (benar)
GET /hilang  -> 500 "Galat internal"; nama berkas hanya ke stderr     (benar)
GET /nihil   -> 404                                                   (benar)
HEAD /ok     -> 405 Method Not Allowed, Allow: GET                    (SALAH)
```

Jadi dua dari tiga butir sudah terpenuhi oleh `web.html()` dan penangan galat
bawaan. Yang tersisa adalah satu cacat nyata: **HEAD ke rute yang punya GET
dibalas 405.**

RFC 9110 §9.3.2 mewajibkan server yang mendukung GET pada suatu sumber daya
juga mendukung HEAD. Jalur berkas statis sudah melakukannya dengan benar
(`Allow: GET, HEAD`, badan dibuang di akhir), tetapi jalur rute tidak —
padahal halaman hasil render templat semuanya lewat jalur rute. Akibatnya
pemeriksa tautan, cache, monitor uptime, dan `curl -I` melihat 405 pada setiap
halaman SSR.

## 2. Sintaks usulan

Tidak ada. Perilaku HTTP yang diperbaiki, bukan API:

```python
app.rute("GET", "/", fungsi(req) { kembalikan web.html(200, m.render(...)) })
# HEAD / sekarang membalas header yang sama dengan GET, tanpa badan.
```

## 3. Semantik

- `HEAD /x` tanpa rute HEAD terdaftar → jalankan penangan **GET** `/x`, lalu
  buang badannya. Header dipertahankan apa adanya, **termasuk
  Content-Length**, sesuai RFC 9110 §9.3.2.
- Rute HEAD yang didaftarkan eksplisit tetap menang (dicoba lebih dulu).
- Tidak ada rute GET yang cocok → 405/404 seperti sebelumnya, tetapi `Allow`
  kini menyertakan `HEAD` bila ada GET — kalau tidak, headernya berbohong.
- Respons 304 (sudah tanpa badan) tidak berubah.
- Penangan GET tetap dijalankan: biaya render tidak dihemat, tetapi
  Content-Length wajib sama dengan GET, dan menebaknya tanpa merender
  akan salah.

## 4. Rencana implementasi

1. `_web_buang_badan(res)` — potong setelah `\r\n\r\n` pertama.
2. `_cari()`: setelah pencarian biasa gagal dan sebelum 405, bila metode HEAD
   cari ulang rute GET pada path yang sama, jalankan, buang badan.
3. `Allow` menyertakan `HEAD` bila `GET` ada di daftar metode.

## 5. Rencana uji

`tests/web_templat_server.sh` — server HTTP sungguhan (bukan hanya
`tangani()`), memeriksa lewat soket:

- GET halaman SSR: 200, `text/html; charset=utf-8`, Content-Length = byte.
- HEAD halaman sama: status dan header **identik** dengan GET, badan kosong.
- Content-Length pada HEAD sama persis dengan GET.
- Templat rusak dan templat hilang: 500 tanpa membocorkan nama berkas, jalur
  sistem berkas, atau pesan galat internal.
- 404 untuk path tak dikenal; `Allow` pada 405 memuat HEAD.
- Halaman ber-Unicode: Content-Length dihitung byte, bukan karakter.
