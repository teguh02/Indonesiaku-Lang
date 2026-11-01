# Rencana Fitur: penyajian berkas statis pada modul web

- **Status:** selesai
- **Target versi:** 1.5.0
- **Pemilik:** teguh02
- **Peta roadmap:** Fase 11 langkah 5 (**WEB-03**)

## 1. Masalah

`pustaka/web.idk` hanya bisa menjawab teks yang dibangun di dalam penangan.
Tidak ada cara menyajikan CSS, JavaScript, atau gambar dari disk, sehingga
halaman apa pun yang butuh aset harus menuliskan isinya sebagai literal di
dalam kode `.idk` — tidak praktis untuk teks, dan mustahil untuk gambar.

Dua hal yang gagal hari ini:

```indonesiaku
# 1. Tidak ada API penyajian berkas sama sekali.
app.statis("/statis", "aset")     # GalatAtribut: metode tidak ada

# 2. Content-Length salah untuk badan biner.
variabel d = baca_semua("gambar.png")   # 214 byte di disk
cetak(_http_panjang_byte(d))            # menghitung ulang sebagai UTF-8
```

Masalah kedua adalah **blocker** untuk masalah pertama dan ditemukan saat
menyiapkan rencana ini. `_http_panjang_byte()` menjumlahkan lebar UTF-8 tiap
karakter hasil iterasi. Untuk berkas berisi byte yang bukan UTF-8 sah, dua
kesalahan terjadi sekaligus:

- byte seperti `0x80` **hilang** dari iterasi karakter, dan
- byte `0xFF`/`0xFE` dihitung 2 byte padahal aslinya 1.

Diukur pada berkas 5 byte `FF FE 41 00 80`:

| Pengukuran | Hasil | Seharusnya |
|---|---|---|
| `panjang()` | 4 | — (jumlah karakter, memang bukan byte) |
| `_http_panjang_byte()` | 6 | **5** |

Content-Length 6 untuk badan 5 byte membuat klien menggantung menunggu byte
yang tidak pernah datang. Penyimpanan teksnya sendiri **tidak** rusak:
round-trip `baca_semua()` → `tulis()` sebuah PNG 214 byte menghasilkan berkas
yang identik byte-per-byte (md5 sama), dan `soket_tulis()` sudah mengirim
`text->length` byte mentah. Jadi yang salah hanya penghitungan panjangnya.

Tidak ada cara memperbaiki ini di level `.idk`: iterasi karakter sudah
kehilangan byte sebelum sempat dihitung. Perlu satu builtin kecil yang
membaca panjang byte asli dari representasi internal teks.

## 2. Sintaks usulan

```indonesiaku
impor "pustaka/web.idk"

variabel app = Aplikasi()

# Sajikan isi direktori "aset" di bawah awalan URL "/statis".
app.statis("/statis", "aset")

app.jalankan(8080)
```

- `GET /statis/gaya.css` → isi `aset/gaya.css`, `Content-Type: text/css`.
- `GET /statis/gambar/logo.png` → `aset/gambar/logo.png` (subdirektori ikut).
- `GET /statis/` → **404** (daftar isi direktori tidak pernah disajikan).

Builtin baru:

```indonesiaku
panjang_byte("héllo")     # 6 — panjang dalam byte, bukan karakter
panjang("héllo")          # 5 — jumlah karakter (tetap seperti sekarang)
```

Tidak ada kata kunci baru. `statis()` adalah metode `Aplikasi`, mengikuti pola
`rute()`/`gunakan()` yang sudah ada dan mengembalikan `diri` agar bisa
dirantai.

## 3. Semantik

**`panjang_byte(teks)`**

- Menerima tepat 1 argumen bertipe teks; mengembalikan bilangan bulat.
- Nilainya adalah panjang byte representasi internal — persis jumlah byte yang
  `soket_tulis()` kirim dan `tulis()` tulis ke disk.
- Argumen bukan teks atau jumlah argumen salah → `nativeRaise` (bisa ditangkap
  `coba/kecuali`), bukan fatal.

**`app.statis(prefiks, akar)`**

- `prefiks` harus teks diawali `/`; `akar` harus teks tidak kosong. Selain itu
  → `GalatWeb` saat pendaftaran (gagal cepat, bukan saat request pertama).
- Hanya `GET` dan `HEAD`. Metode lain pada path yang cocok → **405** dengan
  header `Allow: GET, HEAD`, konsisten dengan router yang ada.
- `HEAD` mengirim header yang sama persis dengan `GET`, tanpa badan.
- Berkas tidak ada, berupa direktori, atau tidak terbaca → **404**. Pesannya
  sama untuk ketiganya supaya tidak membocorkan struktur direktori.
- Path request didekode persen lebih dulu, lalu dinormalkan.

**Keamanan path (bagian terpenting dari langkah ini)**

- Segmen `..` ditolak **setelah** dekode persen, sehingga `%2e%2e%2f` tertutup
  sama rapatnya dengan `../`.
- Byte NUL di dalam path ditolak.
- Path absolut dan segmen kosong ganda dinormalkan, bukan diteruskan apa adanya.
- Penolakan dilakukan pada **path hasil gabungan**: berkas hanya disajikan bila
  jalur final masih berada di bawah `akar`. Pemeriksaan per-segmen saja tidak
  cukup, dan symlink yang menunjuk keluar akar karena itu ikut tertolak.

**Cache**

- `Last-Modified` diisi dari waktu ubah berkas (format HTTP-date, GMT).
- `ETag` lemah dari ukuran + waktu ubah.
- Permintaan dengan `If-None-Match` yang cocok dijawab **304** tanpa badan.

**MIME**

Peta ekstensi → tipe untuk aset web yang umum (`.html .css .js .json .svg .png
.jpg .gif .webp .ico .txt .woff2`). Tidak dikenal → `application/octet-stream`.
Tipe berbasis teks diberi `; charset=utf-8`; tipe biner tidak.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/native.c` | builtin `panjang_byte()` |
| `src/vm.c` | `defineNative("panjang_byte", ...)` |
| `src/native.h` | deklarasi `nativeFnPanjangByte` |
| `pustaka/http.idk` | `_http_panjang_byte()` memakai `panjang_byte()` |
| `pustaka/web.idk` | `_web_mime()`, `_web_jalur_aman()`, `Aplikasi.statis()` |
| `src/sistem.c` | builtin `_jalur_nyata()` (realpath) untuk menutup celah symlink |

Tidak ada perubahan scanner/compiler/VM opcode: ini pekerjaan pustaka plus satu
builtin.

## 5. Rencana uji

- `tests/galat/` — `panjang_byte()` dengan argumen salah.
- Golden example — `panjang_byte` vs `panjang` pada teks ASCII dan non-ASCII.
- Tes unit `_web_jalur_aman()`: `..`, `%2e%2e`, NUL, path absolut, segmen
  kosong, dan jalur sah.
- Tes HTTP sungguhan lewat `curl` (mengikuti pola `tests/web_router_server.sh`):
  berkas teks, berkas **biner** (md5 harus identik dengan berkas di disk),
  404, 405, 304 via `If-None-Match`, `HEAD` tanpa badan, dan percobaan
  traversal `../`.
- Pemeriksaan regresi: seluruh `bash tests/test_all.sh` hijau.
- Build sanitizer (`-DDEBUG_STRESS_GC -fsanitize=address,undefined`) untuk
  builtin baru yang menyentuh objek teks.

## 6. Hasil

Selesai, dengan satu temuan yang mengubah rancangan awal.

**Celah symlink yang nyata.** Rencana awal menganggap penolakan `..` per
segmen sudah cukup. Saat diuji, sebuah symlink di dalam akar
(`aset/bocor.txt` → `../rahasia.txt`) **tetap menyajikan** berkas di luar akar
dengan HTTP 200 — pemeriksaan teks jalur tidak pernah melihat ke mana tautan
itu menunjuk. Ditutup dengan builtin `_jalur_nyata()` (realpath): jalur nyata
berkas harus berada di bawah jalur nyata akar.

**Bug Content-Length.** Ditemukan sebelum implementasi dimulai dan diperbaiki
lebih dulu, karena tanpa itu gambar tidak mungkin disajikan dengan benar.

**Pengujian.** 24 pemeriksaan HTTP sungguhan lewat curl
(`tests/web_statis_server.sh`), termasuk md5 berkas PNG, `HEAD` yang diperiksa
di level soket (varian `curl -I` terbukti buta terhadap badan yang bocor), dan
dua percobaan symlink. Seluruh pemeriksaan disabotase lebih dulu untuk
membuktikan masing-masing bisa merah; dua tes yang ternyata lolos dari sabotase
diperbaiki, bukan dibiarkan.

**Yang sengaja tidak dikerjakan.** `Last-Modified` tidak dikirim karena
`format_tanggal()` hanya menyediakan waktu lokal, bukan GMT — mengirim waktu
lokal sebagai HTTP-date akan salah. Dukungan `Range` juga di luar lingkup
langkah ini.
