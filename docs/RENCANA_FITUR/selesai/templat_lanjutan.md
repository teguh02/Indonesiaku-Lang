# Rencana Fitur: templat lanjutan — filter, isi induk, sertakan berparameter, makro

- **Status:** selesai
- **Target versi:** 1.7.0
- **Pemilik:** maintainer `pustaka/templat.idk`

Melanjutkan SSR-01…10 (Fase 12, semuanya ✅; lihat
`selesai/ssr_template_engine.md` dan `selesai/ssr_include_inheritance.md`).

## 1. Masalah

`templat` sudah punya padanan `@extends`/`{% extends %}` (`{% perluas %}`),
`@section`/`@yield`/`{% block %}` (`{% blok %}`), dan `@include` (`{% sertakan %}`).
Dibanding Blade (Laravel) dan Jinja (Flask), ada empat kekurangan yang terasa
begitu situs bertambah besar:

1. **Tidak ada filter.** `{{ nama | upper }}` / `{{ x | default("-") }}` tidak
   ada, jadi setiap format kecil (huruf besar, nilai kosong, potong teks,
   gabung daftar) harus disiapkan di `.idk` per halaman, dan data yang sama
   diolah berulang di beberapa rute.
2. **Blok anak tidak bisa menambah isi blok induk.** Tidak ada padanan
   `@parent` / `{{ super() }}`, jadi menambah satu `<script>` ke blok `kepala`
   induk berarti menyalin seluruh isi blok itu ke setiap anak.
3. **`sertakan` tidak bisa diberi argumen.** Komponen selalu melihat seluruh
   data pemanggil dengan nama yang sama, sehingga satu komponen kartu tidak
   bisa dipakai untuk `produk` di satu halaman dan `artikel` di halaman lain
   tanpa mengganti nama kunci data.
4. **Tidak ada makro/komponen berparameter.** Tidak ada padanan
   `{% macro %}` atau `@component`/`<x-kartu>`.

## 2. Sintaks

Semua sintaks baru ada di dalam berkas templat, bukan sintaks bahasa
Indonesiaku. Pagar SSR-03 tetap berlaku: `{{ }}` bukan kode. **Tanda kurung
tetap ditolak di dalam `{{ }}`.** Karena itu filter memakai gaya Django
(`nama:argumen`), bukan gaya Jinja (`nama(argumen)`).

```html
{{ nama | huruf_besar }}
{{ deskripsi | potong:80 }}
{{ pengguna.julukan | bawaan:"Tamu" }}
{{ tag | gabung:", " }}

{% blok kepala %}{% induk %}<script src="halaman.js"></script>{% akhirblok %}

{% sertakan "komponen/kartu.html" dengan judul=produk.nama, harga=produk.harga %}
{% sertakan "komponen/kartu.html" dengan judul=artikel.judul hanya %}

{% makro tombol(teks, jenis="utama") %}
  <button class="btn btn-{{ jenis }}">{{ teks }}</button>
{% akhirmakro %}
{% panggil tombol("Simpan") %}
{% panggil tombol(teks=label, jenis="bahaya") %}

{% impor "komponen/makro.html" %}
```

## 3. Semantik

### 3.1 Filter

- Bentuk: `{{ jalan | f1 | f2:arg }}`. Filter diterapkan dari kiri ke kanan.
  `|` di dalam literal berkutip tidak memisah filter.
- Argumen opsional, **satu**, dan berupa literal saja: teks berkutip atau
  bulat. Argumen dari data (`potong:panjang_maks`) **tidak** didukung pada
  versi ini, karena nilai yang membentuk keluaran sebaiknya disiapkan di
  `.idk`.
- Filter bawaan:

  | Filter | Hasil |
  |---|---|
  | `huruf_besar`, `huruf_kecil` | Seperti builtin bernama sama |
  | `judul` | Huruf pertama tiap kata kapital |
  | `rapikan` | Spasi di awal/akhir dibuang |
  | `panjang` | Panjang teks/daftar/kamus |
  | `potong:n` | Maksimum `n` karakter, ditambah `…` bila dipotong |
  | `gabung:"pemisah"` | Gabung daftar/tupel; elemen di-escape satu per satu (lihat bawah) |
  | `bawaan:"x"` | `x` bila nilainya `kosong` atau `""`; lihat aturan nama hilang di bawah |
  | `angka:n` | Desimal dengan `n` angka di belakang koma (`bulatkan`) |

- **Escaping tetap terjadi di akhir.** Keluaran filter di-escape seperti
  interpolasi biasa. **Tidak ada filter `aman`/`safe`**: satu-satunya jalan
  keluar dari escaping tetap `templat.aman()` di kode `.idk`, keputusan SSR-06
  yang sengaja tidak dilonggarkan dari dalam berkas templat.
- **Filter kustom** didaftarkan dari kode tepercaya:
  `m.filter("rupiah", fungsi(nilai, arg) { ... })` pada `templat.mesin()`.
  Fungsi menerima nilai dan argumen (`kosong` bila tidak ada). Hasil berupa
  teks di-escape; hasil `aman(...)` tidak. Nama filter kustom tidak boleh
  menimpa filter bawaan.
- **Nama hilang:** tanpa filter, `{{ x }}` dengan `x` yang tidak ada tetap
  `GalatTemplat` (typo tidak boleh menjadi string kosong diam-diam). Satu
  pengecualian: bila filter **pertama** adalah `bawaan`, jalan yang tidak ada
  dianggap `kosong`. Ini padanan `default` di Jinja, dan satu-satunya cara
  menulis kolom opsional tanpa `{% jika %}`.
- Filter yang tidak dikenal, argumen yang salah jenis, atau filter yang
  butuh argumen tetapi tidak diberi → `GalatTemplat` dengan nomor baris.

### 3.2 `{% induk %}` (isi blok induk)

- Hanya sah di dalam `{% blok %}`. Ia merender isi blok bernama sama dari
  templat **satu tingkat di atas** dalam rantai `perluas` (padanan `@parent` /
  `{{ super() }}`).
- Bertingkat: `cucu → anak → dasar`. `{% induk %}` di blok cucu merender
  versi blok di anak; bila versi anak juga memuat `{% induk %}`, ia merender
  versi dasar.
- `{% induk %}` di blok yang tidak punya tingkat di atasnya (templat dasar,
  atau berkas tanpa `perluas`) → `GalatTemplat`, karena tidak ada isi induk.

### 3.3 `{% sertakan … dengan … %}`

- `{% sertakan "x.html" dengan a=jalan, b="literal", c=3 %}`: komponen
  melihat **salinan dangkal** data pemanggil ditambah nilai `a`, `b`, `c`
  (yang menimpa kunci bernama sama di salinan itu, tidak mengubah data
  pemanggil).
- Akhiran `hanya`: komponen melihat **hanya** nilai yang diberikan (padanan
  `with … only` di Django/Jinja), tanpa data pemanggil.
- Nama templat tetap wajib literal berkutip. Nilai boleh berupa jalan akses
  atau literal saja; filter tidak diizinkan di sini pada versi ini.

### 3.4 Makro

- Definisi: `{% makro nama(p1, p2="bawaan") %} … {% akhirmakro %}`. Parameter
  bawaan berupa literal. Definisi tidak menghasilkan keluaran di tempatnya.
- Panggilan: `{% panggil nama(arg, kunci=nilai) %}`. Argumen posisional lalu
  bernama, dan nilainya jalan akses atau literal. Parameter yang tidak diisi
  dan tidak punya bawaan → `GalatTemplat`. Argumen berlebih atau nama tak
  dikenal → `GalatTemplat`.
- **Lingkup:** badan makro hanya melihat parameternya sendiri, bukan data
  pemanggil. Ini sama dengan makro Jinja, dan membuat komponen tidak
  diam-diam bergantung pada nama data halaman.
- Makro terlihat di seluruh berkas yang mendefinisikannya (boleh dipanggil
  sebelum definisinya), di berkas anak dan induk pada rantai `perluas` yang
  sama, dan lewat `{% impor "berkas.html" %}`. `impor` hanya membaca definisi
  makro dari berkas itu; teks dan tag lain di berkas impor diabaikan.
  Berkas impor tunduk pada pagar yang sama dengan `sertakan`: nama literal,
  di bawah akar mesin, siklus ditolak, kedalaman dibatasi.
- Makro memanggil makro lain diperbolehkan. Kedalaman panggilan dibatasi 32,
  sama dengan `sertakan`, sehingga rekursi tanpa akhir berhenti dengan galat
  dan tidak menghabiskan stack.
- Nama makro ganda dalam satu berkas → `GalatTemplat`. Bila nama sama datang
  dari berkas berbeda, definisi terdekat menang (berkas sendiri, lalu anak,
  lalu induk, lalu impor).
- `render_teks()` mendukung makro yang didefinisikan di sumbernya sendiri.
  `impor` hanya tersedia lewat `templat.mesin(akar)`, dengan alasan sama
  seperti `sertakan`.

## 4. Kompatibilitas

Semua tag dan filter baru sebelumnya adalah galat (`tag tidak dikenal`, atau
`'|' tidak diizinkan`), jadi templat 1.6 yang sah tidak berubah artinya.
`render_teks` dan `mesin` tetap mempertahankan tanda tangannya; `Mesin`
mendapat metode baru `filter(nama, fungsi)`.

## 5. Berkas tersentuh

- `pustaka/templat.idk` — parser (filter, `induk`, `sertakan dengan`, `makro`,
  `panggil`, `impor`), renderer, `Mesin.filter`.
- `examples/templat_lanjutan.idk` + `tests/expected/templat_lanjutan.out` +
  berkas templat di `tests/data/templat/`.
- `website/panduan/07_PUSTAKA_STANDAR.md` §2.22 — bagian baru, blok
  `indonesiaku` yang dijalankan `test_panduan.sh`.
- `docs/TESTING/{WINDOWS,MACOS,LINUX}.md` — kasus `SSR-TPL-03` di blok
  BERSAMA (identik di ketiganya).
- `CHANGELOG.md`, `docs/ROADMAP.md` (Fase 12 lanjutan), `AGENTS.md` bila ada
  jebakan baru.

## 6. Rencana uji

- Golden `templat_lanjutan`:
  - setiap filter bawaan, rantai filter, dan `|` di dalam literal;
  - escaping hasil filter dan elemen `gabung`;
  - `bawaan` untuk nama hilang, dan galat untuk nama hilang tanpa `bawaan`;
  - filter kustom (teks di-escape, `aman()` tidak);
  - `{% induk %}` dua tingkat;
  - `sertakan dengan` (menimpa tanpa mengubah data pemanggil) dan `hanya`;
  - makro dengan parameter bawaan, argumen posisional dan bernama, makro
    memanggil makro, `impor`;
  - setiap galat baru dengan pesan lengkapnya (filter tak dikenal, `induk`
    di luar blok/tanpa induk, argumen makro salah, rekursi makro, impor
    siklus, `aman` sebagai filter).
- Golden lama (`templat_dasar`, `templat_warisan`, `web_mahasiswa`) tidak
  berubah.
- Build sanitizer/stress-GC tidak diperlukan: perubahan murni di pustaka
  Indonesiaku, tanpa kode C.
