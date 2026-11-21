# Rencana Fitur: CLI framework, klien uji HTTP, dan konektor broker (Fase 25)

- **Status:** selesai (V1; Fase 25 tetap 🟨 sebagian — lihat "Ditunda")
- **Target versi:** 1.20.0 (usulan roadmap; V1 boleh masuk rilis minor lebih awal)
- **Pemilik:** maintainer Indonesiaku

## 1. Masalah

Dua hal sudah bisa dilakukan, tetapi dengan banyak boilerplate:

- **Aplikasi CLI dengan sub-perintah.** `pustaka/opsi.idk` adalah padanan
  `argparse` untuk SATU perintah: tanpa sub-perintah (ditulis sendiri di
  kepala berkasnya sebagai "belum didukung"), dan `-h` memanggil `keluar(0)`
  dari dalam `urai()`, jadi alur perintah tidak bisa diuji dalam satu proses.
  Program seperti `alat data tambah --nama Budi` harus memilah `argumen`
  sendiri, mencetak bantuan sendiri, dan memilih kode keluar sendiri.
- **Menguji aplikasi `web.idk`.** `app.tangani(mentah)` sudah bekerja pada
  TEKS request dan mengembalikan TEKS respons tanpa soket (kepala
  `pustaka/web.idk`). Tetapi setiap tes harus menyusun `"GET /x HTTP/1.1\r\n..."`
  sendiri, menghitung `Content-Length` dalam byte, lalu mengurai baris status,
  header, `Set-Cookie`, dan JSON dari teks respons. Contoh REST yang ada
  (`examples/api_siswa.idk`) hanya diuji lewat `curl` ke port sungguhan
  (`tests/api_siswa_server.sh`).

## 2. Audit — yang sudah ada dan tidak diduplikasi

| Yang ada | Dipakai ulang sebagai |
|---|---|
| `pustaka/opsi.idk` (`PenguraiArgumen`, `Hasil`, `GalatOpsi`) | Mesin penguraian setiap tingkat perintah. `cli` tidak menulis ulang parser: jenis nilai, pilihan, wajib, bawaan, bendera, `-vq`, `--nama=nilai`, teks bantuan rata kolom, semuanya dari `opsi`. API `opsi` tidak diubah. |
| `indk uji` (`testFile` di `src/main.c`, `runUnitTests` di `src/vm.c`) | Menjalankan fungsi `uji_*`, mencetak `LULUS`/`GAGAL nama: pesan` dari `.pesan` galat. Asersi `uji_http` menaikkan subkelas `GalatPenegasan` dengan pesan yang terbaca di laporan itu. Tidak ada subperintah `indk` baru (`src/main.c` tidak disentuh). |
| `app.tangani(mentah, peer)` di `pustaka/web.idk` | Titik masuk in-process klien uji. **`web.idk` tidak perlu diubah**: tanpa port, tanpa gelung soket, tanpa hook baru. |
| `http_urai_request`, kelas `Respons` di `pustaka/http.idk` | Tidak dipakai untuk respons: `Respons` milik klien jaringan (`http_ambil`) dan tidak mengenal `Set-Cookie` ganda. `ResponsUji` sendiri yang menyimpan semua header berulang. |
| `pustaka/json.idk` (`json_urai`, `json_teks`) | Badan JSON permintaan dan `respons.json()`. |

## 3. Ruang lingkup V1

Termasuk:

1. **`pustaka/cli.idk`** (modul namespace `cli`):
   - `cli.Perintah(nama, deskripsi, versi, penangan)` dengan `.argumen(...)`
     (meneruskan semua parameter `tambah_argumen` dari `opsi`, ditambah
     `validasi=`), `.sub(nama, deskripsi, penangan)` bertingkat, dan
     `.tangani(f)`.
   - Bantuan otomatis per tingkat (`-h`/`--bantuan`), daftar sub-perintah,
     `--versi` bila `versi` diisi.
   - `.jalankan(daftar)` **mengembalikan** kode keluar (tidak pernah memanggil
     `keluar()`), sehingga bisa diuji; `.utama()` memanggil `keluar(kode)`.
   - Kode keluar terdokumentasi: `0` sukses (termasuk `-h`/`--versi`), `1`
     perintah gagal (`GalatCli` dari penangan), `2` pemakaian salah (argumen
     salah/kurang, sub-perintah tidak dikenal atau tidak diberikan), atau
     bilangan 0..255 yang dikembalikan penangan.
   - Keluaran bisa dialihkan (`keluaran=`, `galat=`) supaya contoh golden dan
     tes tidak menulis ke stderr.
   - `.skrip_lengkapi_bash()`: skrip autocomplete Bash statis (nama
     sub-perintah dan opsi per tingkat). Shell lain ditunda.
2. **`pustaka/uji_http.idk`** (modul namespace `uji_http`):
   - `uji_http.klien(app)` → `KlienUji` dengan `minta(metode, jalur, ...)` dan
     pintasan `ambil`/`kirim`/`perbarui`/`tambal`/`hapus` (GET/POST/PUT/PATCH/
     DELETE); parameter `json=`, `form=`, `badan=`, `kepala=`, `kuki=`.
   - Toples kuki: `Set-Cookie` disimpan dan dikirim balik otomatis;
     `Max-Age=0` menghapus.
   - `ResponsUji` dengan `status`, `kepala`, `badan`, `json()`, `kuki` dan
     asersi berantai `pastikan_status`, `pastikan_kepala`,
     `pastikan_kepala_berisi`, `pastikan_json`, `pastikan_json_berisi`,
     `pastikan_kuki`, `pastikan_badan_berisi`. Gagal → `GalatUjiHttp`
     (subkelas `GalatPenegasan`), pesan memuat status dan potongan badan.
   - Item 4 (kecil): `uji_http.permintaan_mentah(...)` menyusun teks request
     untuk dipakai langsung dengan `app.tangani()`.
3. `examples/api_siswa.idk` dibuat bisa diimpor: gelung server hanya
   berjalan bila `__nama__ == "__utama__"`. Perilakunya saat dijalankan tidak
   berubah.

Ditunda (dicatat di roadmap sebagai sisa):

- **Item 5 MQTT/AMQP** (dan item 6–7 bagian broker): ditunda sepenuhnya.
  Butuh desain plugin opsional, TLS/auth, QoS, reconnect; bukan dependensi
  runtime. Tidak ada kode broker di V1.
- Item 2: konfigurasi CLI dari berkas/lingkungan, generator `indk buat ...`
  (milik fase tooling yang memegang `src/main.c`), autocomplete zsh/fish/
  PowerShell.
- Item 3: auth helper, upload multipart, snapshot/golden respons, mock
  dependency. Path/Domain/Expires pada kuki diabaikan (toples satu domain).
- Item 4: fake database, SMTP, waktu, filesystem.

## 4. Semantik

- `cli`: token pertama yang bukan opsi pada perintah yang punya sub-perintah
  adalah nama sub-perintah. Opsi milik perintah induk ditulis SEBELUM nama
  sub-perintah (seperti `git -C dir status`). Penangan menerima satu objek
  `opsi.Hasil` yang berisi atribut semua tingkat ditambah `perintah` (jalur
  sub-perintah, mis. `"data tambah"`); karena itu `perintah` tidak boleh
  dipakai sebagai nama argumen. `-h`/`--bantuan`/`--versi` dicegat sebelum
  `opsi.urai()`, sehingga argumen wajib yang belum diisi tidak menghalangi
  bantuan. Galat lain dari penangan (bug) tidak ditelan.
- `uji_http`: permintaan dikirim sebagai `HTTP/1.1` dengan `Host: uji.lokal`
  dan `Content-Length` dalam byte. `json=` mengisi `Content-Type:
  application/json`; `form=` mengisi `application/x-www-form-urlencoded`
  dengan percent-encoding. Respons diurai dari teks yang dikembalikan
  `app.tangani()`; nama header disimpan huruf kecil, header berulang tersedia
  utuh di `kepala_semua`.

## 5. Pengujian

- Golden: `examples/pustaka_cli.idk`, `examples/pustaka_uji_http.idk`.
- `tests/galat/`: asersi gagal yang tidak ditangkap, argumen `cli` yang tidak
  sah saat pendaftaran.
- `indk uji`: `examples/uji_api_siswa.idk` menguji CRUD `api_siswa.idk`
  in-process; laporan diharapkan di `tests/uji/uji_api_siswa.uji`.
  `tests/test_uji.sh` dan `tests/test_all.ps1` menjalankannya.
- Kasus batas: `-h` dengan argumen wajib kosong, sub-perintah tidak dikenal,
  validasi khusus, penangan mengembalikan kode, toples kuki dengan
  `Max-Age=0`, badan non-ASCII (`Content-Length` byte), JSON rusak.
- Tanpa perubahan C: sanitizer/stress GC tidak diperlukan.

## 6. Dampak dokumentasi & tooling

- [x] `website/panduan/07_PUSTAKA_STANDAR.md` (§2.28 `cli`, §2.29 `uji_http`)
- [x] VS Code: tidak ada builtin/kata kunci baru, jadi tidak ada perubahan
- [x] `CHANGELOG.md` + `docs/ROADMAP.md`
- [x] `docs/TESTING/{WINDOWS,MACOS,LINUX}.md` (kasus BERSAMA CLI-01, UJH-01)
- [x] Berkas rencana ini: status → `selesai`, lalu `git mv` ke `selesai/`

## 7. Risiko & alternatif

- Port efemeral localhost dipertimbangkan untuk klien uji, tetapi tidak
  perlu: `app.tangani()` sudah murni teks. In-process lebih cepat, tidak
  bergantung firewall/port, dan berjalan di `--sandbox`.
- Menambah sub-perintah langsung ke `PenguraiArgumen` ditolak: mengubah
  perilaku `urai()` (yang memanggil `keluar()`) berisiko bagi program lama.
- `respons.kuki` menyimpan nilai mentah dari `Set-Cookie` (tanpa dekode
  persen), sama seperti yang dikirim balik ke server.

## 8. Temuan di luar ruang lingkup (tidak diperbaiki di V1)

- **`buang obj.atribut[k]` menghapus atribut `atribut`**, bukan kunci `k`
  di kamusnya (kompilator). Reproduksi: `kelas K { init() { diri.d = {"a": 1} } }`,
  `k = K()`, `buang k.d["a"]`, lalu `cetak(k.d)` → "Properti tidak
  terdefinisi 'd'". `uji_http` memakai `hapus_kunci()` sebagai jalan keluar.
- **`web.idk` mendekode `%XX` per byte dengan `chr(byte)`**
  (`_web_dekode_persen`), sehingga nilai form UTF-8 (`Andé` dikirim sebagai
  `And%C3%A9`) terbaca `AndÃ©`. Klien uji mengirim byte UTF-8 yang benar;
  perbaikannya ada di sisi server.
