# 10. Manajer Paket: `indk paket`

Tanpa manajer paket, memakai pustaka orang lain berarti menyalin folder
`.idk` ke proyek Anda dengan tangan, lalu lupa versi mana yang disalin.
`indk paket` menggantikan itu dengan tiga berkas:

| Berkas | Isi | Masuk git? |
|---|---|---|
| `indonesiaku.toml` | Manifest yang Anda tulis: nama proyek dan dependensinya | **Ya** |
| `indonesiaku.lock` | Hasil resolusi: commit persis dan checksum tiap paket | **Ya** |
| `.indk_paket/` | Paket yang terpasang | Tidak — tambahkan ke `.gitignore` |

Versi pertama ini (V1) sengaja kecil: sumber paket hanya **repositori
git** atau **direktori lokal**, belum ada registri publik dan belum ada
rentang versi seperti `^1.2`. Alasannya dicatat di
`docs/RENCANA_FITUR/selesai/manajer_paket.md`.

## 10.1 Memulai

```bash
cd toko
indk paket mulai
```

```text
dibuat indonesiaku.toml untuk proyek 'toko'
```

Tambahkan dependensi. Dari git, dikunci ke sebuah tag:

```bash
indk paket tambah kalkulator --git https://github.com/akun/kalkulator --tag v1.0.0
```

Atau dari direktori lokal, misalnya pustaka milik tim di repositori yang sama:

```bash
indk paket tambah kalkulator --path ../kalkulator
```

```text
  kalkulator  path ../kalkulator
1 paket terpasang di .indk_paket/
```

Lalu impor seperti modul biasa — tanpa mengatur `INDK_PATH`:

```indonesiaku-takjalan
impor kalkulator

cetak(kalkulator.tambah(0.1, 0.2))      # 0.3
```

Contoh di atas ditandai tidak dijalankan oleh uji panduan karena butuh
paket terpasang; alur lengkapnya diuji oleh `tests/paket_cli.idk` di CI.

## 10.2 Manifest `indonesiaku.toml`

```toml
[paket]
nama = "toko"
versi = "0.1.0"

[dependensi]
kalkulator = { git = "https://github.com/akun/kalkulator", tag = "v1.0.0" }
format_rp  = { git = "https://github.com/akun/format_rp", rev = "3f9c2a1" }
util       = { path = "../util" }
```

Setiap dependensi berbentuk tabel sebaris dengan **satu** sumber:

| Kunci | Arti |
|---|---|
| `git` | URL repositori (https, ssh, atau jalur lokal) |
| `tag` | Tag yang dipakai (opsional) |
| `rev` | SHA commit, 7–40 karakter heksadesimal (opsional; pilih `tag` **atau** `rev`) |
| `path` | Direktori lokal, relatif terhadap manifest |

Tanpa `tag`/`rev`, yang dipakai adalah cabang bawaan repositori pada saat
itu; kunci tetap mencatat commit persisnya.

**Nama dependensi adalah nama di `impor`.** Karena itu ia harus berupa huruf
kecil, angka, dan `_`, diawali huruf atau `_`, dan **tidak boleh sama dengan
modul pustaka standar** (`json`, `web`, `templat`, ...): paket bernama `json`
akan membayangi `impor json` tanpa terlihat, jadi ditolak sejak awal.

Format manifest adalah bagian kecil dari TOML: komentar `#`, `[tabel]`,
teks berkutip ganda, bulat, `true`/`false`, dan tabel sebaris. Hal lain
(larik, teks berkutip tunggal, tabel bertitik) ditolak dengan nama berkas dan
nomor baris, bukan ditebak.

## 10.3 Perintah

| Perintah | Fungsi |
|---|---|
| `indk paket mulai [nama]` | Buat `indonesiaku.toml` |
| `indk paket tambah <nama> --git <url> [--tag <t> \| --rev <sha>]` | Tambah dependensi git lalu pasang |
| `indk paket tambah <nama> --path <dir>` | Tambah dependensi lokal lalu pasang |
| `indk paket hapus <nama>` | Buang dari manifest, kunci, dan `.indk_paket/` |
| `indk paket pasang` | Pasang persis sesuai kunci (setelah `git clone` proyek) |
| `indk paket perbarui [nama]` | Resolusi ulang semua atau satu paket, tulis kunci baru |
| `indk paket daftar` | Tampilkan dependensi, commit terkunci, dan status |
| `indk paket -C <dir> <perintah>` | Jalankan di direktori proyek lain |

Kode keluar: `0` berhasil, `1` gagal (pesan diawali `galat:` di stderr),
`64` penggunaan salah.

Bila `tambah` gagal (sumber tidak ada, nama ditolak, konflik), manifest
dikembalikan seperti semula dan `.indk_paket/` tidak berubah: paket baru
dipasang hanya setelah **semua** paket berhasil diambil dan diverifikasi.

## 10.4 Berkas kunci `indonesiaku.lock`

```toml
# indonesiaku.lock -- dibuat oleh `indk paket`; jangan disunting tangan.
versi_kunci = 1

[[paket]]
nama = "kalkulator"
sumber = "git"
url = "https://github.com/akun/kalkulator"
minta = "tag:v1.0.0"
rev = "86d6e8673999b7a0629fb8e7deaee23b95db7d7c"
checksum = "sha256:5cb5d8f9283ce59a0d415efa6fbd64d9bc9813749ae385a2d4e45dcc9656273e"
```

- **Commit, bukan tag.** Tag bisa dipindahkan diam-diam di remote. `pasang`
  selalu memakai `rev` yang tercatat, jadi semua anggota tim dan server
  produksi mendapat isi yang sama. Hanya `perbarui` yang membaca tag lagi.
- **Checksum isi.** SHA-256 atas seluruh berkas paket. Untuk sumber git,
  `pasang` menolak memasang bila isinya tidak cocok dengan kunci — tanda
  repositori ditulis ulang atau unduhan rusak. Untuk sumber `path` (kode
  milik Anda sendiri), checksum dicatat ulang setiap kali dipasang.
- **Tanpa jaringan.** Repositori git disimpan sebagai cache di
  `.indk_paket/.cache/`. Bila commit yang dikunci sudah ada di sana,
  `pasang` tidak menghubungi jaringan sama sekali.

## 10.5 Bagaimana `impor` menemukan paket

`impor kalkulator` memuat `.indk_paket/kalkulator/kalkulator.idk`, dan
`impor kalkulator.format` memuat `.indk_paket/kalkulator/format.idk`.
`.indk_paket/` yang dipakai adalah milik **akar proyek**: direktori terdekat,
dari berkas yang mengimpor ke atas, yang berisi `indonesiaku.toml`. Jadi skrip
di subfolder proyek juga menemukannya, sedangkan skrip di luar proyek mana pun
tidak pernah memakai paket. Kode di dalam paket terpasang memakai
`.indk_paket/` proyek yang memasangnya, sehingga dependensinya ditemukan.

Urutan pencarian modul lengkap:

1. direktori berkas yang mengimpor;
2. direktori di `INDK_PATH` (pengganti eksplisit selalu menang);
3. `pustaka/` di direktori kerja;
4. pustaka standar yang terpasang bersama `indk`;
5. **paket terpasang di `.indk_paket/` akar proyek.**

Paket sengaja paling akhir: paket tidak pernah bisa membayangi modul standar,
termasuk modul standar yang baru ditambahkan di versi interpreter berikutnya.

## 10.6 Membuat paket

Paket adalah repositori (atau direktori) dengan berkas masuk bernama sama
dengan paketnya di **akar**:

```text
kalkulator/
├── indonesiaku.toml     # opsional untuk paket tanpa dependensi
├── kalkulator.idk       # berkas masuk: `impor kalkulator`
├── bantu.idk            # dipakai kalkulator.idk lewat `impor bantu`
└── README.md
```

Di dalam paket, `impor bantu` menemukan `bantu.idk` di sebelahnya seperti
biasa. Manifest paket boleh berisi:

```toml
[paket]
nama = "kalkulator"            # bila ada, harus sama dengan nama dependensi
versi = "1.0.0"
interpreter_minimum = "1.8.0"  # ditolak saat dipasang di interpreter lebih lama

[dependensi]
format_rp = { git = "https://github.com/akun/format_rp", tag = "v2.0.0" }
```

Dependensi paket ikut terpasang (dependensi tidak langsung) ke
`.indk_paket/` yang sama. Dua paket yang meminta nama sama dari sumber
berbeda adalah **konflik** dan ditolak dengan menyebut keduanya; V1 tidak
memilih salah satunya diam-diam. Paket git tidak boleh bergantung pada
`path` lokal, karena direktori itu tidak ada di mesin orang lain.

## 10.7 Keamanan

- **Tidak ada skrip pasca-pasang.** Memasang paket tidak pernah menjalankan
  kodenya. Kode paket baru berjalan saat program Anda meng-`impor`-nya —
  baca dulu paket dari sumber yang tidak Anda percaya.
- URL yang diawali `-` dan transport `ext::` ditolak sebelum git dipanggil,
  dan git dijalankan tanpa shell dengan `protocol.ext.allow=never`.
- Tautan simbolik di repositori git dipasang sebagai berkas teks biasa
  (`core.symlinks=false`), jadi tidak bisa menunjuk ke luar paket. Sumber
  `path` yang berisi tautan simbolik atau *junction* ditolak, bukan diikuti.
- Cache git di `.indk_paket/.cache/` tidak dipercaya begitu saja: `config`-nya
  ditulis ulang setiap kali dipakai, dan hook serta fsmonitor dimatikan, jadi
  cache yang ikut terbawa dalam arsip proyek tidak bisa menjalankan perintah.
- `indk paket` sendiri dimuat dari pustaka standar milik interpreter, bukan
  dari proyek: proyek yang membawa `paket.idk` atau `.indk_paket/paket/`
  tidak bisa menggantikannya.
- Isi berkas sama persis di Windows, Linux, dan macOS (tanpa konversi CRLF),
  sehingga checksum kunci juga sama di semua OS.

## 10.8 Batasan V1

- Butuh `git` terpasang untuk sumber git.
- Belum ada registri (`indk paket tambah kalkulator` tanpa sumber) dan belum
  ada rentang versi; keduanya direncanakan, lihat `docs/ROADMAP.md` Fase 14.
- Satu versi per nama paket dalam satu proyek.
- Paket hanya kode Indonesiaku; ekstensi native C belum didukung.

## 10.9 Pemecahan masalah

| Gejala | Sebab dan solusi |
|---|---|
| `Modul 'x' tidak ditemukan.` setelah `git clone` proyek | `.indk_paket/` tidak ikut di git; jalankan `indk paket pasang` |
| `checksum paket 'x' tidak cocok` | Isi commit yang dikunci berubah di remote; periksa sumbernya, lalu `indk paket perbarui x` bila memang dipercaya |
| `konflik dependensi 'x'` | Dua paket meminta versi berbeda; samakan `tag` di manifest |
| `paket 'x' tidak punya berkas masuk 'x.idk'` | Paket harus punya `x.idk` di akarnya |
| `git tidak dapat dijalankan` | Pasang git dan pastikan ada di `PATH` |
| `'refs/tags/v9' tidak ditemukan` | Tag salah ketik, atau belum di-push ke remote |
