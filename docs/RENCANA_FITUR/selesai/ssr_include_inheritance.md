# Rencana Fitur: include komponen dan template inheritance (SSR-05)

- **Status:** selesai (SSR-05)
- **Target versi:** 1.7.0
- **Pemilik:** teguh02

Langkah 6 Fase 12 pada `docs/ROADMAP.md`. Melanjutkan
`selesai/ssr_template_engine.md` (SSR-01…04 + SSR-06) yang sudah merender
interpolasi, perulangan, kondisi, dan auto-escaping.

## 1. Masalah

Mesin templat saat ini merender satu berkas secara terisolasi. Setiap halaman
harus memuat ulang seluruh kerangka HTML-nya sendiri:

```html
<!doctype html><html><head><title>Siswa</title></head><body>
<nav>…menu yang sama di 12 halaman…</nav>
<h1>{{ judul }}</h1>
…
<footer>…footer yang sama di 12 halaman…</footer>
</body></html>
```

Akibatnya:

1. **Duplikasi kerangka.** Mengubah `<nav>` berarti menyunting setiap halaman.
   Satu halaman yang terlewat menjadi bug tampilan yang tidak terdeteksi tes.
2. **Tidak ada komponen.** Potongan berulang (kartu produk, baris tabel, widget
   paginasi) tidak bisa dipakai ulang.
3. **Mesin templat tanpa inheritance memaksa orang kembali merangkai string**
   di `.idk` untuk kerangka halaman — persis masalah XSS/kuadratik yang
   SSR-01…04 dibuat untuk menghilangkan.

## 2. Sintaks usulan

Bukan sintaks bahasa. Dua penanda baru di berkas templat, konsisten dengan
`{% untuk %}`/`{% jika %}` yang sudah ada:

```html
{% sertakan "komponen/kartu.html" %}
```

```html
{# anak.html #}
{% perluas "dasar.html" %}
{% blok isi %}<p>Halo {{ nama }}</p>{% akhirblok %}
```

```html
{# dasar.html #}
<!doctype html><title>{{ judul }}</title>
<body>{% blok isi %}isi bawaan{% akhirblok %}</body>
```

Aturan:

- `{% sertakan "x.html" %}` menyisipkan hasil render `x.html` dengan data yang
  **sama**. Nama templat adalah literal berkutip, bukan ekspresi — alasan sama
  dengan `{{ }}` yang bukan kode: nama templat dari data pengguna berarti
  pemilihan berkas dikendalikan penyerang.
- `{% perluas "induk.html" %}` wajib menjadi **tag pertama** di berkas. Anak
  hanya boleh berisi definisi `{% blok %}`; teks di luar blok diabaikan dan
  ditolak sebagai galat agar tidak diam-diam hilang.
- `{% blok nama %}…{% akhirblok %}` mendefinisikan titik override. Di induk ia
  merender isinya sendiri bila anak tidak menimpanya. Nama blok adalah
  identifier sederhana; nama ganda dalam satu berkas ditolak.
- Blok di dalam blok diizinkan; blok di dalam `{% untuk %}` ditolak karena
  override-nya tidak punya arti tunggal.

## 3. Semantik

- **Resolusi berkas hanya lewat `Mesin`.** `urai()` tetap murni sintaksis.
  `render_teks()` tanpa mesin menolak `{% sertakan %}`/`{% perluas %}` dengan
  pesan jelas, bukan membaca berkas dari direktori kerja.
- **Path tetap dikunci di akar templat** lewat `Mesin._jalur()` yang sudah ada
  (menolak `..` dan path absolut). Include tidak boleh membuka jalan keluar
  dari akar.
- **Siklus wajib ditolak**, bukan dibiarkan menggantung: `a.html` menyertakan
  `b.html` yang menyertakan `a.html` harus mati dengan pesan yang menyebut
  rantainya. Diimplementasikan sebagai tumpukan nama yang sedang dirender.
- **Kedalaman include dibatasi** terpisah dari kedalaman tag.
- Data yang dilihat include dan blok sama dengan data induknya — tidak ada
  scope baru. Variabel loop tetap tidak bocor seperti sebelumnya.

## 4. Berkas tersentuh

- `pustaka/templat.idk` — node `sertakan`, `perluas`, `blok`; resolusi di
  `Mesin`.
- `examples/templat_warisan.idk` + `tests/expected/templat_warisan.out` —
  golden.
- `tests/data/templat/` — berkas induk, anak, komponen, dan pasangan siklus.
- `tests/templat_sabotase.sh` — kasus sabotase baru.
- `website/panduan/07_PUSTAKA_STANDAR.md` §2.22, `CHANGELOG.md`,
  `docs/ROADMAP.md`, `docs/TESTING/{WINDOWS,MACOS,LINUX}.md`.

## 5. Rencana uji

Positif:

- include sederhana; include bersarang; include dipakai dua kali di satu
  halaman.
- inheritance: anak menimpa blok, anak membiarkan blok memakai isi bawaan,
  blok bersarang.
- data mengalir ke include dan ke blok; auto-escaping tetap berlaku di
  keduanya.

Harus ditolak (dan pesannya diperiksa, bukan sekadar "ada galat"):

- siklus include langsung (a→a) dan tidak langsung (a→b→a).
- `{% perluas %}` bukan tag pertama.
- teks di luar `{% blok %}` pada berkas yang memakai `{% perluas %}`.
- nama blok ganda.
- `{% sertakan %}` dengan nama non-literal (`{% sertakan nama %}`).
- path traversal lewat include (`{% sertakan "../rahasia.txt" %}`).
- `{% sertakan %}` dipanggil dari `render_teks()` tanpa mesin.
- templat yang disertakan tidak ada.

Sabotase: tiap guard di atas dicabut satu per satu dan harus membuat tes merah.

## 6. Di luar lingkup

Cache hasil parse (SSR-08), integrasi respons web (SSR-07), dan aplikasi contoh
(SSR-09/10) tetap PR terpisah.
