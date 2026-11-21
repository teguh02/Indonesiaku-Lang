# Rencana Fitur: Transfer berkas — operasi lokal dan unduhan HTTP/HTTPS (Fase 17, V1)

- **Status:** selesai (V1: langkah 1–2 penuh, langkah 3 sebagian; langkah 4–6 ditunda, lihat §7)
- **Target versi:** 1.12.0-rc.1 (V1 boleh masuk rilis minor lebih awal)
- **Pemilik:** Teguh Rijanandi

## 1. Masalah

Sebelum V1, menyalin satu berkas berarti `tulis_semua(b, baca_semua(a))`:
seluruh isi masuk RAM, izin berkas hilang, dan menyalin berkas ke dirinya
sendiri mengosongkannya. Tidak ada cara memindah/mengganti nama berkas sama
sekali (tidak ada native `rename`), tidak ada salin/hapus direktori
bertingkat, tidak ada berkas sementara yang aman dari balapan nama, dan
`http_ambil()` menampung seluruh badan respons di memori sehingga mengunduh
berkas besar ke disk tidak mungkin dilakukan dengan benar.

```indonesiaku
# Hari ini: salah tanpa peringatan
tulis_semua("a.txt", baca_semua("a.txt"))      # aman, tapi seluruh isi di RAM
r = http_ambil("https://contoh.invalid/arsip.zip")
tulis_semua("arsip.zip", r.badan)               # 2 GB di RAM; putus di tengah = berkas parsial
```

### Inventarisasi yang sudah ada (tidak diduplikasi)

| Sudah ada | Di mana | Catatan |
|---|---|---|
| `baca_semua`, `tulis_semua`, `tambah_berkas`, `buka_berkas`/`baca`/`tulis`/`tutup` | `src/native.c` | byte-exact; `tulis` dipakai unduhan untuk menulis per potongan |
| `ada_berkas`, `hapus_berkas` | `src/native.c` + `src/sistem.c` | **`hapus_berkas` tidak diduplikasi** di modul baru |
| `isi_direktori`, `buat_direktori`, `hapus_direktori`, `direktori_kini` | `src/sistem.c` | `isi_direktori` ditolak di `--sandbox` |
| `_info_jalur` (mengikuti tautan), `_adalah_tautan`, `_jalur_nyata` | `src/sistem.c` | dasar perlindungan symlink |
| `jalur.gabung`, `nama_dasar`, `nama_direktori`, `ada`, `ukuran`, ... | `pustaka/jalur.idk` | dipakai ulang, bukan ditulis ulang |
| `http_ambil`, `http_urai_url`, `_http_gabung_url`, `GalatJaringan` | `pustaka/http.idk` | HTTP/1.0, badan ditampung penuh; aturan pengalihan dipakai ulang |
| `soket_sambung`, `tls_mulai`, `soket_batas_waktu`, `soket_baca` | `src/native.c` | TLS selalu terverifikasi (Fase 11) |

## 2. API

Dua modul namespace baru (tanpa awalan, gaya Fase 6):

```indonesiaku
impor berkas
impor jaringan

berkas.salin_berkas("a.txt", "cadangan/a.txt")               # timpa=salah
berkas.salin_direktori("proyek", "cadangan/proyek")          # tautan tidak diikuti
berkas.pindahkan("lama.txt", "arsip/lama.txt")               # lintas perangkat: salin + hapus
berkas.ganti_nama("a.tmp", "a.txt", timpa=benar)             # atomik, satu sistem berkas
berkas.hapus_direktori_bertingkat("build")                   # tautan dihapus, targetnya tidak
untuk (dir, subdir, isi) dalam berkas.jelajahi_direktori("src") { ... }
t = berkas.berkas_sementara(awalan="laporan-", akhiran=".csv")
d = berkas.direktori_sementara()
berkas.info("a.txt")      # {"jenis", "ukuran", "waktu_ubah", "tautan", "mode", "baca_saja"}
berkas.sha256("arsip.zip")                                   # streaming, tanpa memuat isi

hasil = jaringan.unduh("https://contoh.invalid/arsip.zip", "arsip.zip",
                       sha256="9f86d0...", maks_ukuran=50 * 1024 * 1024,
                       batas_waktu=30, kemajuan=fungsi(n, total) { cetak(n) })
hasil["ukuran"]; hasil["sha256"]; hasil["url"]
```

Nama mengikuti langkah 2 roadmap. Nama `jaringan` dipilih karena roadmap
mencontohkan `jaringan.unduh`; FTP/SSH/SFTP nanti mendapat modul sendiri
(`ftp`, `ssh`, `sftp`) agar tidak semuanya bergantung pada pustaka SSH.

## 3. Semantik

- **Galat.** Kegagalan sistem berkas adalah `GalatOS` dengan alasan portabel
  dari `src/sistem.c` (`tidak ada`, `sudah ada`, `akses ditolak`, ...)
  ditambah tiga alasan baru: `adalah direktori`, `berkas yang sama`,
  `tautan simbolik`. Pesan tidak pernah memuat teks `strerror`/`FormatMessage`.
- **Timpa eksplisit.** Semua operasi yang menulis ke tujuan menolak tujuan
  yang sudah ada kecuali `timpa=benar`. Tujuan berupa direktori selalu
  ditolak (tidak ada "salin ke dalam direktori" ala `cp`), supaya jalur hasil
  tidak pernah menebak.
- **Salin ke diri sendiri** ditolak (`berkas yang sama`) sebelum tujuan
  dipotong; dibandingkan lewat identitas berkas (dev/inode, atau volume +
  file index di Windows), bukan teks jalur.
- **Tautan simbolik.** `salin_berkas` mengikuti tautan pada *asal* (seperti
  `cp`). Operasi pohon tidak pernah mengikuti tautan: `salin_direktori`
  menolak tautan di dalam pohon (`GalatOS ... tautan simbolik`) kecuali
  `lewati_tautan=benar`; `hapus_direktori_bertingkat` menghapus tautannya
  saja dan menolak bila akarnya sendiri tautan (seperti `shutil.rmtree`);
  `jelajahi_direktori` mencantumkan tautan di daftar berkas dan tidak pernah
  menelusurinya. Di Windows "tautan" = reparse point (symlink/junction).
- **Atomik.** `ganti_nama` memakai `rename()`/`MoveFileExW`. Tanpa `timpa`,
  POSIX memeriksa keberadaan tujuan dulu (ada jendela balapan sempit,
  didokumentasikan); Windows menolak secara atomik. Lintas perangkat →
  `GalatOS ... beda perangkat`; `pindahkan` menangkap kasus itu dan jatuh ke
  salin + hapus.
- **Sementara.** Nama acak 16 heksa dari `acak_bita` (CSPRNG), dibuat
  eksklusif (`O_EXCL`/`CREATE_NEW`/`mkdir`), izin `0600`/`0700` di POSIX.
  Lokasi: `TMPDIR`, `TEMP`, `TMP`, lalu `/tmp`, lalu `.`. Pemanggil yang
  menghapusnya.
- **Unduhan.** HTTP/1.0 (tidak ada chunked), badan ditulis per potongan
  `soket_baca()` (≤ 64 KB) ke berkas sementara **di direktori tujuan**, lalu
  di-rename ke tujuan hanya setelah: status 2xx, panjang sama dengan
  `Content-Length` (bila ada), tidak melebihi `maks_ukuran`, dan SHA-256 cocok
  (bila diberikan). Kegagalan apa pun menghapus berkas sementara; tujuan
  tidak pernah berisi berkas parsial. `batas_waktu` adalah batas per-operasi
  baca/tulis soket (seperti `timeout` di `requests`), bukan batas total; koneksi
  awal (`connect`) belum punya batas waktu sendiri. Pengalihan diikuti dengan
  aturan `http_ambil` (maks lompatan, tolak https→http, tolak putaran).
  `kemajuan(diterima, total)` dipanggil per potongan (`total` = `kosong` bila
  server tidak mengirim panjang); mengembalikan `salah` membatalkan unduhan.
  4xx/5xx, batal, ukuran, checksum → `GalatUnduhan` (turunan `GalatJaringan`
  → `GalatOS`).
- **Sandbox.** `_ganti_nama`, `_salin_berkas`, `_buat_eksklusif`,
  `_hapus_tautan` ditolak di `--sandbox` (`GalatIzin`); `jelajahi_direktori`
  dan hapus bertingkat sudah tertolak lewat `isi_direktori`, unduhan lewat
  `soket_sambung`. `info` dan `sha256` hanya membaca berkas yang sudah boleh
  dibaca `baca_semua`, jadi tidak ditolak.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/sistem.c` | native internal `_ganti_nama`, `_salin_berkas`, `_buat_eksklusif`, `_hapus_tautan`, `_info_lengkap`; alasan baru |
| `src/native.c` | `_sha256_berkas` (streaming mbedTLS), `_soket_baca_kepala` (pisah kepala/badan per **byte**, bukan per karakter) |
| `src/vm.c` | registrasi native |
| `pustaka/berkas.idk` | API publik operasi lokal |
| `pustaka/jaringan.idk` | `unduh`, `GalatUnduhan` |

`_soket_baca_kepala` diperlukan karena memisahkan kepala HTTP dengan
`teks_cari` + irisan bekerja per karakter UTF-8: badan biner di potongan yang
sama dengan kepala bisa terpotong di tengah urutan byte. Tidak ada objek GC
yang dipegang selama alokasi selain tupel hasil (di-push ke stack).

## 5. Pengujian

- Golden `examples/pustaka_berkas.idk` (semua operasi lokal di direktori
  sementara, termasuk nama non-ASCII) dan `examples/pustaka_jaringan.idk`
  (validasi argumen tanpa jaringan).
- `tests/galat/`: salin tanpa timpa, salin ke diri sendiri, unduhan dengan
  URL `file://`.
- `tests/unduh_server.sh`: server HTTP lokal `.idk` (127.0.0.1) menyajikan
  badan biner 300 KB, 404, pengalihan, badan terpotong, Content-Length besar;
  klien memeriksa SHA-256, `maks_ukuran`, batal lewat `kemajuan`, tujuan sudah
  ada, dan bahwa tidak ada berkas parsial/sementara tertinggal.
- `tests/test_berkas_unicode.ps1`: operasi lokal di jalur `日本 ☕` di Windows.
- `tests/sandbox.sh`: native baru ditolak.
- Build `-DDEBUG_STRESS_GC -fsanitize=address,undefined` + golden + galat.

## 6. Dampak dokumentasi & tooling

- [x] `website/panduan/07_PUSTAKA_STANDAR.md` (bagian `berkas` dan `jaringan`)
- [x] VS Code: tidak ada builtin publik baru (semua native berawalan `_`)
- [x] `CHANGELOG.md` + `docs/ROADMAP.md`
- [x] `docs/TESTING/{WINDOWS,MACOS,LINUX}.md`
- [x] Berkas rencana ini → `selesai/`

## 7. Risiko, alternatif, dan yang ditunda

- **Ditunda: langkah 4–6 (FTP/FTPS, SSH, SFTP).** Butuh pustaka SSH yang
  di-vendor (libssh2/libssh) beserta review lisensi, build lintas OS, dan
  server uji terisolasi (langkah 8). Itu pekerjaan tersendiri; memaksakannya
  ke V1 akan menambah dependensi C besar tanpa tes yang layak. FTP polos
  tanpa TLS juga tidak sepadan sebagai "fitur" tanpa FTPS.
- **Ditunda dari langkah 3:** resume `Range` (butuh berkas parsial yang
  sengaja disimpan, bertentangan dengan jaminan "tidak ada berkas parsial"
  tanpa desain terpisah), batas waktu `connect`, batas waktu total, dan
  HTTP/1.1 chunked.
- **Ditunda dari langkah 2:** mengubah izin (`chmod`) dan pemilik; `info`
  hanya *membaca* mode/baca-saja.
- **Berkas baca-saja di Windows** (mis. objek git) membuat
  `hapus_direktori_bertingkat` gagal dengan `akses ditolak`; tidak dihapus
  paksa diam-diam.
- Alternatif yang ditolak: menulis seluruh unduhan di C (lebih cepat, tetapi
  menggandakan logika pengalihan/URL yang sudah teruji di `http.idk`);
  `CopyFileW` vs salin manual di Windows — `CopyFileW` dipakai karena
  menyalin atribut dan aliran data seperti Explorer.
