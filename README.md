<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="website/assets/logo-idk-lang-transparan.png">
    <img src="website/assets/logo-idk-lang-terang.png" alt="Idk-Lang — Bahasa Pemrograman Indonesiaku" width="640">
  </picture>
</p>

# 🇮🇩 Bahasa Pemrograman Indonesiaku

**Indonesiaku** adalah bahasa pemrograman dengan sintaks Bahasa Indonesia yang
terinspirasi dari Python, dijalankan oleh *virtual machine* bytecode yang
ditulis dalam C.

Semantiknya mengikuti Python, tetapi **blok kode memakai kurung kurawal `{ }`**
seperti PHP/C — **tidak ada aturan indentasi**, sehingga kode yang ditulis rata
kiri pun tetap sah.

```indonesiaku
fungsi sapa(nama) {
    jika nama == "" {
        kembalikan "Halo, dunia!"
    }
    kembalikan "Halo, " + nama
}

cetak(sapa("Teguh"))
```

[![Rilis](https://img.shields.io/github/v/release/teguh02/Indonesiaku-Lang)](https://github.com/teguh02/Indonesiaku-Lang/releases/latest)
[![Lisensi](https://img.shields.io/badge/lisensi-MIT-blue)](LICENSE)
[![Docker](https://img.shields.io/docker/pulls/teguh02/indonesiaku)](https://hub.docker.com/r/teguh02/indonesiaku)

---

## 📚 Dokumentasi

**Dokumentasi lengkap ada di folder [`website/panduan/`](website/panduan/README.md)** —
panduan sintaks, instalasi, dan referensi pustaka standar semuanya di sana.
README ini sengaja dibuat ringkas.

| Dokumen | Isi |
|---------|-----|
| [01_PERKENALAN](website/panduan/01_PERKENALAN.md) | Visi, misi, dan gambaran fitur |
| [02_INSTALASI](website/panduan/02_INSTALASI.md) | Pemasangan di Windows, Linux, macOS |
| [03_PANDUAN_SINTAKS](website/panduan/03_PANDUAN_SINTAKS.md) | Panduan sintaks lengkap beserta contoh |
| [04_KONTRIBUSI](website/panduan/04_KONTRIBUSI.md) | Cara berkontribusi & menyiapkan lingkungan dev |
| [05_CREATOR](website/panduan/05_CREATOR.md) | Profil dan motivasi pembuat |
| [06_PENUTUP](website/panduan/06_PENUTUP.md) | Kesimpulan, FAQ, rujukan lain |
| [07_PUSTAKA_STANDAR](website/panduan/07_PUSTAKA_STANDAR.md) | Referensi fungsi bawaan & pustaka standar |

- Situs dokumentasi & playground: **[idk-lang.web.id](https://idk-lang.web.id)**
- Ekstensi VS Code: [highlighting, formatter, dan LSP](editors/vscode-indonesiaku/README.md)
- Spesifikasi bahasa yang normatif: **[docs/SPESIFIKASI.md](docs/SPESIFIKASI.md)**
- Rencana pengembangan (internal, bukan panduan pengguna): **[docs/](docs/README.md)**

---

## 🚀 Pemasangan Cepat

### Unduh binari (disarankan)

Ambil paket untuk sistem operasi Anda di
**[halaman rilis](https://github.com/teguh02/Indonesiaku-Lang/releases/latest)**:

| Sistem | Berkas |
|---|---|
| Linux | `indonesiaku_<versi>_amd64.deb` atau `indk-linux.tar.gz` |
| Windows | `Indonesiaku-Setup-<versi>.exe` atau `indk-windows.zip` |
| macOS | `indk-macos.tar.gz` |

Setiap paket sudah berisi `indk`, pustaka standar, dan contoh program — cukup
diekstrak lalu dijalankan. Langkah rincinya ada di
[02_INSTALASI](website/panduan/02_INSTALASI.md).

### Docker

```bash
docker run --rm -v "$PWD:/kode" teguh02/indonesiaku /kode/program.idk
```

### Kompilasi dari sumber

Membutuhkan kompilator C11 (GCC/Clang) dan `make`.

```bash
git clone https://github.com/teguh02/Indonesiaku-Lang.git
cd Indonesiaku-Lang
make
./indk examples/hello.idk
```

### Menjalankan

```bash
indk program.idk    # jalankan berkas
indk                # REPL interaktif
indk -v             # versi
```

---

## ⬆️ Memperbarui ke Versi Baru

Cek dulu apakah ada versi lebih baru:

```bash
indk --periksa-pembaruan
```

Perintah ini membandingkan versi terpasang dengan rilis terbaru di GitHub lalu
mencetak perintah pemutakhiran yang sesuai. Ia **hanya berjalan bila diminta** —
menjalankan skrip biasa tidak pernah menghubungi jaringan.

### Linux (paket `.deb`)

```bash
sudo dpkg -i indonesiaku_<versi>_amd64.deb
```

**Tidak perlu** `dpkg -r` lebih dulu: paket baru menimpa yang lama beserta
`pustaka/`-nya.

### Linux/macOS (arsip `.tar.gz`)

```bash
tar xzf indk-linux.tar.gz
sudo install -m755 indk-linux/indk /usr/local/bin/indk
sudo mkdir -p /usr/local/share/indonesiaku
sudo cp -r indk-linux/pustaka /usr/local/share/indonesiaku/
```

Perbarui **binari dan `pustaka/` sekaligus**. Pustaka standar ikut berubah
antarversi, dan `indk` baru dengan `pustaka/` lama gagal pada `impor web`.
Tata letak folder yang dikenali ada di
[02_INSTALASI](website/panduan/02_INSTALASI.md); untuk lokasi lain, arahkan
`INDK_PATH` ke folder `pustaka/`.

### Windows

Jalankan `Indonesiaku-Setup-<versi>.exe`; installer menimpa pemasangan lama.
Untuk pengguna `.zip`, ganti seluruh isi foldernya.

### Docker

```bash
docker pull teguh02/indonesiaku:latest
```

Gunakan tag versi (`teguh02/indonesiaku:1.5.0`) bila ingin menyematkan versi
tertentu di CI.

### Yang perlu diperhatikan

- **Program lama tetap jalan.** Sejak v1.0.0 berlaku kebijakan tidak ada
  perubahan yang memutus kompatibilitas sampai v2.0.0.
- **Tidak ada pembaruan otomatis.** Mengganti biner yang sedang berjalan
  memerlukan hak root dan verifikasi tanda tangan, jadi pemutakhiran selalu
  dilakukan manual.
- **Pengguna sebelum v1.4.0** belum memiliki `--periksa-pembaruan`; periksa
  halaman rilis secara manual satu kali.
- Perubahan tiap versi tercatat di [CHANGELOG.md](CHANGELOG.md).

---

## ✨ Fitur

Mulai rilis 1.0 berlaku **kebijakan stabilitas**: tidak ada perubahan yang
merusak kompatibilitas sampai 2.0.

- **Tipe data** — `bulat` (int64) & `desimal`, teks Unicode, `logika`, `kosong`,
  daftar, kamus, tupel, himpunan, dan `himpunan_beku`
- **Alur kendali** — `jika`/`jika_lain`/`selain`, `selagi`, `untuk ... dalam`,
  `hentikan`/`lanjut`, serta ekspresi kondisional
- **Fungsi** — rekursi, closure penuh, parameter bernilai bawaan, argumen
  bernama, dan fungsi anonim
- **OOP** — `kelas`, `init`, `diri`, pewarisan (`<`), `super`, dan metode
  ajaib (`_teks_`, `_sama_`, `_tambah_`, `_panjang_`, `_logika_`,
  `_ambil_`/`_atur_`, `_hash_`, cerminan `_ktambah_` dkk, `_iter_`, `_panggil_`);
  `terurut`/`min`/`max` bekerja atas objek; atribut kelas; `dengan ... sebagai`; dekorator (`@properti`, `@metode_statis`, `@metode_kelas`); pewarisan berganda (MRO C3); kelas abstrak (`@abstrak`)
- **Penanganan galat** — `naikkan` + `coba`/`kecuali`/`akhirnya`; semua galat
  runtime dapat ditangkap
- **Modul** — `impor matematika`, `impor x sebagai y`, `dari x impor y`, paket,
  jalur pencarian `INDK_PATH`, serta `__nama__`/`__utama__`
- **Kemudahan menulis** — f-string, irisan & indeks negatif, *list
  comprehension*, penugasan berganda, operator bitwise, metode tipe bawaan
- **Pustaka standar** — berkas I/O, JSON, server dan klien HTTP, soket TCP,
  SQLite tertanam, trigonometri, hash (`md5`/`sha*`), serta modul
  bernamespace `koleksi`, `statistik`, `iterasi`, `fungsional`, `salin`,
  `jalur`, `pola` (ekspresi reguler), `opsi` (argumen baris perintah), `log`,
  `proses` (menjalankan program lain), `bita`, `base64`, `csv`, dan `kura`
  (grafika kura-kura ke SVG) (Fase 6); `migrasi` (migrasi skema basis data)
  dan `templat` (templat HTML server-side dengan auto-escaping); mode
  `indk --sandbox`
- **Mesin** — *garbage collector* mark-sweep dan REPL interaktif

Daftar lengkap beserta contoh tiap fitur ada di
[03_PANDUAN_SINTAKS](website/panduan/03_PANDUAN_SINTAKS.md) dan
[07_PUSTAKA_STANDAR](website/panduan/07_PUSTAKA_STANDAR.md).

### Contoh aplikasi nyata

| Berkas | Isi |
|--------|-----|
| `examples/todo_cli.idk` | Aplikasi CRUD Todo-list dengan penyimpanan JSON |
| `examples/todo_server.idk` | Server REST API HTTP di atas soket TCP |
| `examples/db_sqlite.idk` | Penggunaan SQLite dengan parameter `?` |

Selebihnya ada di folder [`examples/`](examples/) — sekaligus dipakai sebagai
*golden test*.

---

## 🗺️ Rencana Selanjutnya

Rilis terbaru adalah **v1.5.0**. Fase 0–7 selesai, Fase 8 sebagian secara
sengaja (tersisa GUI dan AOT), Fase 9 (LSP untuk ekstensi VS Code) selesai di
v1.4.0, dan Fase 10–12 terbit bersama di v1.5.0: driver MySQL/PostgreSQL,
HTTPS/TLS, server HTTP modular, serta templat HTML server-side sudah dapat
dipakai. Fase 13 (keamanan aplikasi web: sesi, login, CSRF) sedang berjalan.

Status per fitur ada di **[docs/ROADMAP.md](docs/ROADMAP.md)** — itulah
satu-satunya sumber status yang resmi.

---

## 🏗️ Arsitektur

<p align="center">
  <img src="website/assets/arsitektur.svg" alt="Alur Indonesiaku: kode sumber .idk → Scanner → Compiler single-pass → Chunk bytecode → Virtual Machine berbasis stack, didampingi GC mark-sweep, fungsi bawaan, objek/tabel hash, dan pustaka standar" width="100%">
</p>

Kode berjalan dalam satu lintasan: Scanner mengubah teks `.idk` menjadi token,
Compiler (Pratt parser) langsung memancarkan bytecode **tanpa membangun AST**,
lalu Virtual Machine berbasis stack mengeksekusinya dengan GC mark-sweep.
REPL memakai lintasan yang sama, dan perkakas (`--dis`, `periksa`, `uji`,
`rapikan`) membaca hasil tahap kompilasi.

```
Indonesiaku/
├── src/          # Interpreter C11 (scanner, compiler, VM, GC, native)
├── pustaka/      # Pustaka standar, ditulis dalam Indonesiaku sendiri
├── examples/     # Contoh program .idk (sekaligus golden test)
├── tests/        # Harness pengujian
├── website/      # Situs + panduan pengguna (website/panduan/)
├── docs/         # Spesifikasi & perencanaan bahasa
├── editors/      # Ekstensi VS Code
├── docker/       # Image Docker
└── Makefile
```

Catatan teknis untuk pengembang interpreter ada di [AGENTS.md](AGENTS.md).

---

## 🤝 Kontribusi

Kontribusi sangat diterima — fitur baru, perbaikan bug, dokumentasi, contoh
program, pengujian, maupun optimasi.

```bash
git clone https://github.com/teguh02/Indonesiaku-Lang.git
cd Indonesiaku-Lang
make
bash tests/test_all.sh          # Windows: powershell -File tests/test_all.ps1
git checkout -b fitur/nama-fitur
```

Panduan lengkapnya di **[04_KONTRIBUSI](website/panduan/04_KONTRIBUSI.md)**.

---

## ℹ️ Independensi Proyek & Disclaimer

Indonesiaku merupakan proyek bahasa pemrograman open source independen yang
dikembangkan sebagai kontribusi terhadap perkembangan teknologi dan ekosistem
pemrograman.

Proyek ini **tidak berafiliasi, tidak disponsori, dan tidak mewakili**
Pemerintah Republik Indonesia maupun lembaga pemerintahan lainnya.

Indonesiaku dikembangkan untuk kepentingan edukasi, eksplorasi teknologi, dan
kolaborasi komunitas pengembang. Nama "Indonesiaku" digunakan sebagai identitas
proyek bahasa pemrograman dan tidak dimaksudkan untuk menyatakan kepemilikan,
kewenangan, atau representasi resmi atas Negara Republik Indonesia.

Pengembangan proyek dilakukan secara terbuka dan nonkomersial. Partisipasi serta
kontribusi komunitas bersifat sukarela tanpa adanya kewajiban atau janji
pemberian imbalan finansial.

Proyek ini tidak menggunakan lambang negara, identitas kementerian, atau atribut
lain yang dapat memberikan kesan proyek resmi pemerintah.

---

## 📝 Lisensi

Dirilis di bawah **[Lisensi MIT](LICENSE)**.

## 👨‍💻 Penulis

**Teguh Rijanandi** — [@teguh02](https://github.com/teguh02)

---

**Selamat ngoding dengan Bahasa Indonesia! 🇮🇩**

Ada pertanyaan, saran, atau laporan bug? Silakan buka
[issue](https://github.com/teguh02/Indonesiaku-Lang/issues).
