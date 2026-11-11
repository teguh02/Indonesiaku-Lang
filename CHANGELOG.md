# Changelog - Bahasa Pemrograman Indonesiaku

Semua perubahan penting pada proyek ini akan didokumentasikan dalam file ini.

Format berdasarkan [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
dan proyek ini mengikuti [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Belum dirilis]

### Ditambahkan

- **Templat: filter, isi blok induk, sertakan berparameter, dan makro**
  (`pustaka/templat.idk`), padanan fitur Blade/Jinja yang belum ada:
  - filter gaya Django `{{ x | huruf_besar | potong:80 }}` dengan filter bawaan
    `huruf_besar`, `huruf_kecil`, `judul`, `rapikan`, `panjang`, `potong:n`,
    `gabung:"pemisah"`, `bawaan:"x"`, dan `angka:n`, plus filter sendiri lewat
    `m.filter(nama, fungsi)`. Hasil filter tetap di-escape; sengaja **tidak ada**
    filter `aman` — HTML tepercaya tetap hanya lewat `templat.aman()` di `.idk`.
    Nama yang hilang tetap galat kecuali filter pertamanya `bawaan`;
  - `{% induk %}` di dalam `{% blok %}` merender isi blok yang sama satu tingkat
    di atasnya (padanan `@parent` / `super()`), bertingkat;
  - `{% sertakan "x.html" dengan a=jalan, b="teks" %}` memberi komponen nilai
    sendiri tanpa mengubah data pemanggil, dan akhiran `hanya` menyembunyikan
    data pemanggil;
  - makro `{% makro nama(p, q="bawaan") %}…{% akhirmakro %}`, dipanggil dengan
    `{% panggil nama(x, q=y) %}` dan dibagikan lewat `{% impor "berkas.html" %}`.
    Badan makro hanya melihat parameternya; kedalaman panggilan dibatasi 32.

  Semua sintaks baru sebelumnya adalah galat, jadi templat yang sah tidak
  berubah artinya.

## [1.6.2] - 2026-11-11

### Diperbaiki

- **Windows: `jam()` mengukur waktu dinding, bukan waktu CPU** (T-W11).
  `clock()` di CRT Windows menghitung waktu sejak proses mulai, jadi `jam()`
  ikut bertambah selama `henti()` (0,505 detik untuk `henti(0.5)` di 1.6.1),
  berbeda dengan dokumentasi dan dengan Linux/macOS. Windows kini memakai
  `GetProcessTimes` (user + kernel). Dijaga `examples/jam_waktu_cpu.idk`.

- **Windows: uninstall meninggalkan entri `PATH` sistem** (T-W13, ditemukan
  saat uji rilis 1.6.1 di Windows 11 asli, kasus WIN-15). `unins000.exe`
  menghapus folder, pintasan, dan entri Apps & features, tetapi
  `C:\Program Files\Indonesiaku` tetap tertinggal di `Path` HKLM. Kini
  uninstaller membuang tepat entri itu dan membiarkan entri lain apa adanya;
  entri installer lawas `Program Files (x86)\Indonesiaku\bin` tidak disentuh.
  Pemasang juga menyiarkan perubahan lingkungan (`ChangesEnvironment=yes`),
  sehingga terminal baru langsung melihat `PATH` yang benar. Dijaga
  `tests/pemasang_path.ps1`, uji pasang-lalu-uninstall sungguhan di runner
  Windows CI.

### Keamanan

- **Windows: argumen `proses.jalankan` untuk berkas `.bat`/`.cmd` bisa
  dijalankan `cmd.exe` sebagai perintah** (T-W6). Windows selalu menjalankan
  berkas batch lewat `cmd.exe`, yang tidak mengikuti aturan kutip biasa.
  Akibatnya argumen tanpa spasi tidak dikutip (`"a&perintah"` menjalankan
  `perintah`), dan `%VAR%` diperluas walau dikutip. Ini bertentangan dengan
  janji "tanpa shell, tidak ada injeksi" di panduan. Kini argumen untuk
  berkas batch selalu dikutip, dan argumen yang memuat `"`, `%`, `!`, atau
  baris baru ditolak dengan `GalatNilai`; program `.exe` tidak terpengaruh.
  Arahnya sama dengan perbaikan CVE-2024-24576 di Rust. Dijaga
  `tests/test_proses_batch.ps1` (merah pada biner 1.6.1: berkas penanda
  terbuat).

- **Windows: dua server bisa mendengarkan port yang sama** (T-W7).
  `soket_dengar` memasang `SO_REUSEADDR`. Di Winsock opsi itu mengizinkan
  soket lain mengikat port yang sedang dipakai, sehingga `soket_dengar`
  kedua berhasil diam-diam dan kedua server berbagi koneksi; proses lokal
  lain pun bisa ikut menerima koneksi yang ditujukan ke server Indonesiaku.
  Di Windows opsi itu kini tidak dipasang lagi (seperti asyncio Python), dan
  `soket_dengar` kedua gagal dengan pesan `gagal bind ke port`. Mendengarkan
  ulang port segera setelah server ditutup tetap berfungsi. Linux/macOS
  tidak berubah. Dijaga `examples/soket_port_eksklusif.idk`.

## [1.6.1] - 2026-11-10

### Diperbaiki

- **Kompiler menghabiskan RAM sampai OOM pada kode bersarang dalam yang
  memuat kelas.** Masukan berupa 201 `{` diikuti `kelas P { fungsi m() {} }`
  membuat `indk` tumbuh sampai 2,8 GB lalu dibunuh OOM; di CI gejalanya
  terlihat sebagai timeout. Pagar kedalaman sudah menyala benar dan galatnya
  tercetak tepat satu kali — kebocorannya terjadi sesudah itu. `block()`
  menolak turun di batas sarang lalu kembali **tanpa memakan satu token pun**,
  sedangkan `classDeclaration()` mengulang `method()` sampai `}` atau EOF,
  sehingga putarannya tak pernah maju dan tiap putaran menumbuhkan chunk.
  Bukan rekursi: stack 512 MB pun tetap menggantung. Kini `block()` melewati
  isi blok sampai penutupnya, seperti yang sudah dilakukan `parsePrecedence()`.
  Pemakaian memori untuk kasus yang dilaporkan turun dari 2,8 GB menjadi
  2,7 MB. Konstruksi lain pada kedalaman yang sama (`fungsi`, `jika`,
  `selagi`, `untuk`) tidak pernah terpengaruh karena pemanggilnya memakai
  token. Ditemukan fuzzer (#109).

- **Pengenal yang diawali `dan` ditolak kompiler.** `variabel danau = 1`
  gagal dengan "Harapkan nama variabel", begitu pula `dani`, `dangkal`,
  `dangkalx`, dan `dan2` — sebagai variabel, nama fungsi, maupun parameter.
  Penyebabnya satu daun trie di `identifierType()` yang mengembalikan
  `TOKEN_DAN` begitu tiga huruf pertama cocok, tanpa memeriksa apakah
  tokennya berhenti di situ. Kata kunci lain tidak terpengaruh (`jikalau`,
  `untukmu`, `kelasku` selalu sah). Kata `dangkal` dipakai di panduan
  sendiri ("salinan dangkal"), jadi pengguna bisa menabraknya saat menamai
  variabel mengikuti istilah di dokumentasi.

### Ditambahkan

- **Ekstensi VS Code punya ikon dan terbit di Marketplace.** Tanpa `icon` di
  `package.json`, ekstensi tampil dengan placeholder abu-abu. Kini memakai
  logo kotak yang sudah dipakai situs, jadi identitasnya sama di situs dan
  editor. Ekstensi dapat dipasang dengan
  `code --install-extension teguh02.indonesiaku`.

### Diubah

- **Bahasa pesan `--periksa-pembaruan` dirapikan.** Saran pemutakhiran untuk
  Linux, Windows, dan macOS memakai kalimat baku berbahasa Indonesia. Istilah
  "menimpa pasangan lama" dan "meng-uninstall" diganti menjadi "memperbarui
  pemasangan yang sudah ada" dan "dihapus terlebih dahulu". Isi sarannya
  (URL, perintah, urutan langkah) tidak berubah.
- **Pemasang Windows memakai ikon Indonesiaku.** Sebelumnya `Indonesiaku-Setup-*.exe`
  tampil dengan ikon kardus bawaan Inno Setup — di Explorer, di pojok jendela
  wizard, dan di pintasan Start Menu. Kini memakai logo `idk` persegi
  (`website/assets/logo-idk-square.png`) lewat `SetupIconFile` dan
  `WizardSmallImageFile`. `UninstallDisplayIcon` dialihkan ke berkas `.ico`
  yang ikut terpasang, karena `indk.exe` sendiri belum punya resource ikon.
  Ditambahkan kasus uji **WIN-20**.

## [1.6.0] - 2026-11-08

Rilis stabil Fase 13. Memuat **seluruh** perubahan pada prarilis
`1.6.0-rc.1` di bawah, ditambah perbaikan berikut yang ditemukan saat
uji paket rilis di Windows 11 asli.

### Diperbaiki

- ⚠️ **`ada_berkas()` menanyakan keberadaan, bukan apakah berkas bisa dibuka
  (T-W9).** Dulu jawabannya `fopen(path, "rb")` berhasil atau tidak, sehingga
  berkas yang sedang dikunci program lain di Windows (editor, antivirus,
  SQLite dalam transaksi) dijawab `salah`, padahal `jalur.ada()` menjawab
  `benar`. Kini keduanya memakai metadata yang sama (`GetFileAttributesExW` /
  `stat`). **Perubahan perilaku:** direktori kini dijawab `salah` di semua OS;
  dulu `benar` di Linux/macOS (karena `fopen` pada direktori berhasil di sana)
  tetapi `salah` di Windows. Untuk berkas atau direktori, pakai `jalur.ada()`.
- **REPL menerima kode yang dipipakan dari PowerShell 5.1** (T-W12).
  `'x = 21', 'cetak(x * 2)' | indk` gagal di baris 1 dengan `Karakter tidak
  dikenal`, karena PowerShell menyisipkan BOM UTF-8 di depan pipa -- dua
  sekaligus setelah `$OutputEncoding = [Text.Encoding]::UTF8`. REPL kini
  membuang BOM berturut-turut di awal sesi, seperti berkas sumber (T-W2).
  BOM di tengah sesi tetap ditolak, dan `input()` tidak berubah: U+FEFF tetap
  sampai, sama seperti `input()` Python.
- **`indk periksa` kini melihat lokal implisit (T-U1).** `sisa = 5` di dalam
  fungsi membuat variabel fungsi-lokal (K3), tetapi linter hanya memeriksa
  yang ditulis `variabel x`. Karena kata `variabel` opsional, linter praktis
  buta terhadap mayoritas kode nyata. Slot prescan yang pasti terbayangi
  deklarasi `variabel` di blok bersarang dikecualikan -- slot mati itu artefak
  kompiler, bukan kesalahan pengguna. Bytecode tidak berubah.
- Dua kode mati yang langsung ditemukan linter setelah perbaikan di atas:
  `variabel selesai` yang tak pernah dipakai di `pustaka/templat.idk`, dan
  variabel perulangan `i` yang tak dipakai di `pustaka/kura.idk` (menjadi `_i`).
- **Windows: jalur lebih dari 260 karakter** (T-W10). Dulu
  `buat_direktori`, `tulis_semua`, `baca_semua`, `isi_direktori`,
  `ada_berkas`, `hapus_berkas`, menjalankan skrip, dan `impor` gagal pada
  jalur sepanjang itu, bahkan dengan `LongPathsEnabled=1`, karena biner tidak
  punya manifest `longPathAware`. Kini jalur yang bentuk absolutnya mencapai
  batas itu diberi awalan `\\?\` setelah dinormalkan (`GetFullPathNameW`),
  tanpa bergantung pada registri. Jalur pendek diteruskan persis seperti
  dulu. Di Linux/macOS tidak ada perubahan.

### Ditambahkan

- **Penjaga cakupan tes lintas-OS.** `tests/cakupan_lintas_os.sh` menolak
  berkas `.ps1` yang ada di repo tetapi tidak pernah dipanggil harness mana
  pun, dan menolak sub-suite portabel yang lupa didaftarkan ke daftar Git Bash
  Windows. Keduanya pernah terjadi tanpa memerahkan apa pun.
- `tests/versi_konsisten.sh` dan `tests/http_pengalihan_server.sh` kini ikut
  jalan di Windows.

### Dihapus

- `tests/test_output.ps1`. Tidak pernah dipanggil harness mana pun, dan dua
  dari lima contoh yang diperiksanya sudah tidak ada sehingga dilewati
  diam-diam; tiga sisanya sudah diuji golden suite secara byte-exact.

## [1.6.0-rc.1] - 2026-11-07

Prarilis dari 1.6.0; isinya ikut terbit pada 1.6.0.

Kandidat rilis untuk v1.6.0 (Fase 13 — keamanan web dan autentikasi).
Seluruh butir Fase 13 selesai: sesi dan kuki, CSRF, CORS dan trusted-proxy,
autentikasi dengan peran, contoh aplikasi CRUD, dan regresi keamanan.

**Yang sudah dibuktikan:** suite 327/327; panduan 206/206; CI 23/23 hijau di
Linux, macOS, dan Windows; goldens (166) dan tes galat (114) lulus pada build
`-DDEBUG_STRESS_GC -fsanitize=address,undefined` tanpa satu pun temuan
sanitizer.

**Yang BELUM dibuktikan, dan karena itu ini RC dan bukan rilis stabil:**
paket rilis (`.deb`, `.tar.gz`, installer Windows) belum diuji di mesin bersih
pada ketiga OS. `docs/TESTING/{LINUX,MACOS,WINDOWS}.md` masih
`⬜ belum diuji` dan tidak diisi sampai ada yang benar-benar menjalankannya.
CI membangun dari sumber di runner — itu bukan hal yang sama dengan menguji
paket terpasang, dan justru perbedaan itulah yang dulu melewatkan seluruh bug
khusus Windows.

Teks versi yang terlihat pengunjung di `website/*.html` sengaja tetap
`v1.5.0`: situs publik mengiklankan rilis stabil, dan RC bukan itu. Yang
dinaikkan hanya `INDK_VERSION`, tag image Docker, dan cache-buster aset.

### Ditambahkan

- **Contoh aplikasi CRUD dengan login dan peran.** `examples/catatan_app.idk`
  menyatukan basis data (Fase 10), HTTP (Fase 11), templat (Fase 12), dan
  keamanan (Fase 13): CRUD catatan, dua peran, token CSRF di tiap formulir,
  rotasi sesi saat login, dan pemeriksaan kepemilikan di sisi server.
  `web.alihkan(lokasi)` ditambahkan untuk pola POST-redirect-GET (303).

- **Regresi keamanan menyeluruh.** `tests/keamanan_app.sh` menyerang aplikasi
  itu lewat HTTP sungguhan: session fixation, CSRF lintas situs, izin lintas
  pengguna, XSS tersimpan, SQL injection, enumerasi nama pengguna, kuncian
  brute force, dan flag kuki — 25 pemeriksaan.

- **CORS dan trusted-proxy (WEB-05, tuntas).** `app.cors(asal, kredensial)`
  mengizinkan asal lintas situs secara eksplisit — `"*"` **ditolak**, karena
  ia melumpuhkan pertahanan peramban dan tidak sah bersama kredensial.
  `Vary: Origin` selalu disertakan, dan `OPTIONS` dijawab 204 tanpa menyentuh
  penangan. `app.proxy_tepercaya(daftar)` membuat `req["alamat"]` berisi
  alamat klien sebenarnya di belakang Nginx. Builtin baru
  `soket_alamat_peer(soket)`.

- **Autentikasi, hashing sandi, dan peran (WEB-06).** `web.hash_sandi()` dan
  `web.periksa_sandi()` memakai PBKDF2-HMAC-SHA256 dengan **600.000 iterasi**
  (rekomendasi OWASP) dan garam 16 byte per sandi; format hash
  `pbkdf2_sha256$<iterasi>$<garam>$<turunan>` menyimpan parameternya sendiri
  sehingga iterasi dapat dinaikkan tanpa mengunci pengguna lama.
  `web.perlu_hash_ulang()` menandai hash yang perlu diperkuat saat login
  berikutnya. `web.wajib_peran()` membedakan **401** (belum masuk) dari
  **403** (tidak berhak). `web.PembatasPercobaan` membatasi tebakan beruntun.
- **Proteksi CSRF (WEB-05, sebagian).** `web.token_csrf()` membuat token
  per-sesi dan menyimpannya di dalam data sesi; `web.periksa_csrf()`
  memvalidasi permintaan yang mengubah keadaan (`GET`/`HEAD`/`OPTIONS` lolos
  otomatis), menerima token dari `_csrf` di formulir atau header
  `X-CSRF-Token`; `web.wajib_csrf()` mengembalikan penjaga siap pakai yang
  menjawab 403.
- **Kuki dan sesi untuk aplikasi web (WEB-04).** `web.set_kuki()` menambahkan
  `Set-Cookie` dengan `HttpOnly`, `Secure`, `SameSite`, `Max-Age`, `Path` dan
  `Domain`; `web.baca_kuki()` mengurai header `Cookie` dari permintaan;
  `web.TokoSesi` menyimpan sesi di memori dengan `buat`/`ambil`/`hapus`/
  `putar`/`bersihkan`.
- `http_response()` menerima parameter keempat opsional `header` berupa daftar
  pasangan `[nama, nilai]`. Sebelumnya respons hanya bisa membawa empat header
  tetap, sehingga `Set-Cookie` mustahil dikirim. Daftar pasangan dipilih alih-
  alih kamus karena `Set-Cookie` boleh muncul berkali-kali dalam satu respons.

### Keamanan

- `X-Forwarded-For` hanya dipercaya bila koneksi datang dari proxy yang
  terdaftar, dan entri yang diambil adalah yang **paling kanan**. Entri kiri
  ditulis klien: memercayainya membuat pembatas percobaan WEB-06 dapat
  dilewati dengan satu baris header palsu per tebakan.
- `Vary: Origin` wajib ikut pada setiap respons CORS; tanpa itu cache
  perantara dapat menyajikan respons milik satu asal kepada asal lain.

- Sandi tidak pernah disimpan apa adanya maupun di-hash dengan fungsi cepat:
  PBKDF2 ber-600k iterasi membuat pemecahan offline mahal, dan garam acak
  per sandi membatalkan tabel pelangi sekaligus menyembunyikan dua pengguna
  yang bersandi sama.
- Hash sandi dibandingkan dengan waktu-tetap, sama seperti token CSRF.
- `wajib_peran()` memisahkan 401 dari 403 agar klien tahu apakah perlu masuk
  atau memang tidak berhak.
- Token CSRF dibandingkan dengan **waktu-tetap**, bukan `==`: `==` berhenti di
  karakter pertama yang berbeda sehingga lama perbandingan membocorkan berapa
  banyak awalan yang sudah benar, dan token dapat ditebak karakter demi
  karakter.
- Token CSRF terikat pada sesinya: token milik sesi lain ditolak, dan rotasi
  `putar()` maupun logout otomatis membatalkannya.
- ID sesi diambil dari `acak_bita()` (CSPRNG), bukan `acak()` yang disemai
  waktu dan dapat ditebak dari header `Date`.
- `putar()` menghapus ID lama saat rotasi, menutup *session fixation*.
- Nilai kuki di-encode persen pada karakter yang dilarang RFC 6265 sehingga
  nilai berisi `;` tidak dapat menyuntikkan atribut seperti `Domain`.

## [1.5.0] - 2026-11-07

Rilis ini membawa pekerjaan **Fase 10 (database), Fase 11 (HTTP/HTTPS/TLS)
dan Fase 12 (SSR/templat)** yang selesai dan teruji sejak `v1.4.0` tetapi
belum pernah sampai ke pengguna. Dokumen internal sempat menyebut nomor
rencana "1.5.0" dan "1.6.0" untuk masing-masing fase; nomor itu **target
perencanaan, bukan tag**. Secara faktual ketiganya terbit bersama di sini
sebagai `v1.5.0`, sesuai `docs/ROADMAP.md` yang mengizinkan fase dan versi
minor tidak berbanding satu-satu asalkan pemetaannya ditulis terang-terangan.

Tidak ada perubahan yang memutus kompatibilitas.

### Ditambahkan

- **`acak_bita(n)` — sumber acak kriptografis** (prasyarat Fase 13). Satu-satunya
  sumber acak sebelumnya, `acak()`/`acak_bulat()`, memakai `rand()` yang disemai
  `srand(time(NULL))`, sehingga **seluruh barisannya dapat direproduksi oleh
  siapa pun yang menebak detik start proses**. Dibuktikan, bukan diduga: tiga
  proses `indk` pada detik yang sama mengembalikan `440825032` yang sama persis,
  dan sebuah program C yang hanya diberi `time()` mereproduksi keluaran `indk`
  secara identik. ID sesi yang dibangun dari `acak_bulat()` dapat ditebak dari
  header `Date` sebuah respons HTTP.

  `acak_bita(n)` mengembalikan daftar `n` bita `0..255` dari CTR-DRBG mbedTLS
  yang sudah di-vendor untuk TLS — tanpa berkas `.c` baru dan tanpa jalur
  per-OS tulisan tangan. Bila sumber acak sistem gagal dibaca ia **menaikkan
  galat, tidak pernah diam-diam jatuh ke `rand()`**.

  `acak()` dan `acak_bulat()` sengaja **tidak diubah**: pemisahannya mengikuti
  `random` vs `secrets` di Python, agar kode simulasi tetap cepat dan dapat
  direproduksi. Yang ditambahkan adalah peringatan eksplisit di panduan.

- **`batas_waktu` pada driver PostgreSQL dan MySQL** — batas waktu baca/tulis
  soket yang sebelumnya terpaku 30 detik kini bisa disetel lewat konfigurasi
  (`{"batas_waktu": 5}`); bawaannya tetap 30 detik dan `0` berarti tanpa batas.
  30 detik terlalu lama untuk aplikasi yang ingin gagal cepat dan terlalu
  pendek untuk kueri analitik.

- **Uji kegagalan TLS dan timeout di suite driver** (sisa terakhir Fase 10
  butir 7). TLS: `"tls": benar` ke server tanpa TLS harus **dibatalkan**, bukan
  turun diam-diam ke teks polos. Asersinya memeriksa pesan galatnya, bukan
  sekadar "ada galat" — mencabut cek anti-downgrade tetap menghasilkan galat
  dari `tls_mulai()`, sehingga versi longgar tes ini lulus karena alasan yang
  salah. Timeout: `tests/bantu/server_bisu.py` menerima koneksi lalu diam, dan
  **durasinya ikut diperiksa** karena batas waktu 30 detik yang tidak
  terkonfigurasi tetap lulus bila yang diperiksa hanya munculnya galat.
  Tiga sabotase masing-masing wajib membuatnya merah.

### Diperbaiki

- **BLOB/bytea tersimpan sebagai teks di driver PostgreSQL dan MySQL** —
  menyimpan daftar byte seperti `[255, 254, 65, 0, 128]` ke kolom `bytea`/`blob`
  menghasilkan **22 byte teks** `"[255, 254, 65, 0, 128]"`, bukan 5 byte biner.
  Kegagalannya diam: tidak ada galat, data hanya rusak. SQLite pada mesin yang
  sama benar, sehingga program identik memberi hasil berbeda tergantung basis
  data. Akar masalahnya satu pola di dua tempat — parameter dipaksa lewat
  `ke_teks()` sebelum dikirim (`pg.idk` `_jalankan()`, `my.idk` `_nilai_bita()`).
  Keduanya kini mendeteksi daftar byte lewat `_adalah_bita()`; PostgreSQL
  mengirimnya dengan format Bind **biner** (format teks membuat server menolak
  `0xff` sebagai UTF-8 tak sah). Jalur baca tidak diubah: `bytea` tetap kembali
  sebagai teks `\x...` dari server.
  Diuji di `tests/pg_driver.sh` dan `tests/my_driver.sh` dengan muatan yang
  sengaja bukan UTF-8 sah dan memuat NUL di tengah; tiga sabotase masing-masing
  wajib membuatnya merah.

### Ditambahkan

- **API embedding C** (`src/indk.h`, `src/indk_api.c`) — Indonesiaku kini bisa
  dijalankan dari dalam program C lain. `make libindk.a` menghasilkan pustaka
  statis (tanpa `src/main.o`, supaya `main()` tetap milik program pemakai),
  dengan permukaan: `indk_mulai()`, `indk_selesai()`, `indk_jalankan()`,
  `indk_jalankan_berkas()`, `indk_versi()`, `indk_daftar_fungsi()` untuk
  memasang fungsi C sebagai builtin, dan `indk_ambil_global_teks()` untuk
  membaca variabel global skrip kembali ke C. Galat dikembalikan sebagai nilai
  (`INDK_GALAT_KOMPILASI` / `INDK_GALAT_RUNTIME`), tidak mematikan proses
  induk. Panduan: `website/panduan/09_EMBEDDING.md`; contoh yang benar-benar
  dikompilasi CI: `examples/embed/halo.c`.
  **Batas yang disadari (K22):** satu runtime per proses — VM adalah variabel
  global tunggal, jadi tidak ada handle seperti `lua_State*` — dan tidak aman
  untuk banyak utas.
  Diuji `tests/embedding.sh` (13 asersi, mengompilasi dan menjalankan program
  C sungguhan); empat sabotase masing-masing wajib membuatnya merah.

- **Tes mode `--sandbox`** (`tests/sandbox.sh`, 16 asersi). Mode ini dipakai
  playground web untuk setiap program pengguna, tetapi sebelumnya **tidak
  punya tes sama sekali**. Yang ditemukan saat menulisnya: `soket_dengar()`
  lolos sepenuhnya — program sandbox bisa memasang listener dan memakai port
  host, padahal `soket_sambung()` sudah ditolak sejak awal.

- **Contoh website dinamis SQLite** (`examples/web_mahasiswa.idk`, SSR-10) —
  menyambung templat, SQLite, dan router: halaman daftar dan detail,
  parameter SQL diikat (`?`, bukan dirangkai), id tak dikenal membalas 404,
  dan nama berisi `<script>` ter-escape otomatis. Bagian 8.8 baru di
  `website/panduan/08_DEPLOYMENT.md`: direktori templat wajib ikut `COPY`,
  volume SQLite adalah direktori (bukan berkas, karena `-wal`/`-shm`), dan
  tabel troubleshooting gejala → sebab.

- **Cache hasil parsing templat** (SSR-08) — `templat.mesin(akar)` mengurai
  tiap berkas sekali, lalu memeriksa ukuran dan waktu-ubah pada tiap render
  dan mengurai ulang hanya bila berubah. Parsing adalah **73,7%** kerja tiap
  render; sesudahnya **−73,4%** instruksi dengan keluaran byte-identik.
  Matikan dengan `templat.mesin(akar, salah)`.

- **`{% sertakan %}` dan pewarisan templat (`{% perluas %}` / `{% blok %}`)**
  di `pustaka/templat.idk` (SSR-05). Komponen bisa dipakai ulang dan kerangka
  halaman tidak perlu disalin ke tiap templat; blok yang tidak ditimpa anak
  memakai isi bawaan induk. Nama templat wajib literal berkutip — nama yang
  datang dari data berarti pemanggil HTTP yang memilih berkas mana dibuka.
  Siklus sertakan ditolak dengan menyebut rantainya (bukan menggantung),
  kedalaman dibatasi 32, dan teks di luar `{% blok %}` pada berkas yang
  memakai `{% perluas %}` ditolak alih-alih hilang diam-diam dari keluaran.
  `render_teks()` menolak `{% sertakan %}`: tanpa akar templat tidak ada yang
  membatasi berkas mana yang boleh dibaca.

- **`pustaka/templat.idk`** — mesin templat HTML server-side (SSR-01…04,
  SSR-06). Berkas `.html` biasa dengan `{{ nilai }}`,
  `{% untuk x dalam y %}…{% akhiruntuk %}`, dan
  `{% jika c %}…{% selain %}…{% akhirjika %}`; `render_teks(sumber, data)`
  untuk string dan `mesin(akar).render(nama, data)` untuk berkas.
  Auto-escaping aktif sejak interpolasi pertama dan hanya bisa dilewati lewat
  `templat.aman()`. `{{ }}` sengaja **bukan** kode Indonesiaku — hanya rantai
  akses `a.b[0]`, sehingga berkas `.html` tidak menjadi titik eksekusi kode.
  Nama templat ber-`..` ditolak, variabel `{% untuk %}` tidak bocor keluar
  blok, dan rendering linear terhadap ukuran data (bukan kuadratik seperti
  merangkai string dengan `+`). Dijaga `examples/templat_dasar.idk` dan
  `tests/templat_sabotase.sh` (9 jaminan, semuanya terbukti merah saat
  dilucuti).

- **`pustaka/migrasi.idk`** — lapisan migrasi skema (WDB-05) di atas
  `pustaka/db.idk`: `langkah()`, `naikkan_semua()`, `turunkan_satu()`,
  `status()`. Versi terpasang dicatat di tabel `_migrasi_indk`, jadi
  menjalankan program dua kali tidak mengulang DDL yang sama. Langkah berjalan
  urut menaik menurut versi (bukan urutan penulisan), masing-masing di dalam
  transaksinya sendiri, dan berhenti pada galat pertama. Versi ganda serta
  `turunkan_satu` pada langkah tanpa `turun` ditolak dengan `GalatMigrasi`.
  Catatan: DDL di MySQL/MariaDB memicu implicit commit, jadi rollback
  transaksional hanya sepenuhnya aman di SQLite dan PostgreSQL. Query builder
  sengaja belum dikerjakan — ia harus memutuskan dialek SQL, yang justru
  sengaja tidak disembunyikan oleh `db.idk`.

- **`indk awakutu <skrip.idk>`** — debugger CLI: `henti <baris>`,
  `hapus <baris>`, `daftar`, `langkah`, `lanjut`, `lihat <nama>`, `tumpukan`,
  `keluar` (Enter kosong = `langkah`). Seluruh antarmukanya ke stderr, jadi
  stdout program tetap murni dan bisa dipipa. `lihat` membaca variabel global
  dan parameter fungsi; variabel lokal di dalam badan fungsi belum didukung.

- **`indk profil <skrip.idk>`** — histogram opcode + hitung panggilan fungsi,
  untuk memutuskan optimasi berdasarkan ukuran, bukan dugaan. Penghitung
  (bukan sampling waktu), jadi hasilnya deterministik dan tidak terganggu
  unwinding galat maupun generator. Laporan ke stderr; stdout program tetap
  bersih untuk dipipa. Overhead saat tidak dipakai tidak terukur.

### Diperbaiki

- **`HEAD` pada rute ber-`GET` dibalas `405`** (SSR-07). RFC 9110 §9.3.2
  mewajibkan server yang mendukung `GET` juga mendukung `HEAD`; jalur berkas
  statis sudah benar, jalur rute tidak — sehingga `curl -I`, pemeriksa
  tautan, cache, dan monitor uptime gagal di setiap halaman. Header kini
  identik dengan `GET` (termasuk `Content-Length`) dengan badan dibuang, dan
  `Allow` ikut menyebut `HEAD`.

- **Galat templat kini menyebut nama berkas** (SSR-09). Sebelumnya hanya
  `baris 7:` — tidak bisa ditindaklanjuti dalam rantai
  `{% perluas %}`/`{% sertakan %}`, karena berkas satu baris pun bisa
  melaporkan "baris 2" milik komponen yang disertakannya. Format sekarang
  `berkas:baris N:` mengikuti kebiasaan kompiler, dan selalu menunjuk berkas
  **tempat** kesalahan, bukan halaman terluar.

- **Parsing templat tidak lagi kuadratik** (SSR-09). Batas 10.000 tag ada,
  tetapi mencapainya butuh **94 detik** — batas yang menolak selambat itu
  bukan pelindung, justru jalan DoS-nya. Penyebabnya penghitung nomor baris
  yang memindai ulang dari awal berkas tiap tag, dan dua panggilan pencarian
  per iterasi yang masing-masing menyapu sisa berkas. Kini linear: 8× input =
  8,00× instruksi, dan 10.000 tag selesai di bawah satu detik.

- **`soket_dengar()` tidak ditolak dalam mode `--sandbox`.** Lihat di atas.

- **Contoh multikondisi di panduan sintaks** ([#63](https://github.com/teguh02/Indonesiaku-Lang/issues/63))
  menumpuk tiga pernyataan `jika` terpisah alih-alih rantai `jika_lain`,
  sehingga `nilai = 85` mencetak "Grade B" **dan** "Grade C". Bahasanya
  sendiri tidak bermasalah — `jika_lain` sudah ada dan benar. Contohnya
  diperbaiki dan ditambah pembanding yang menjelaskan perbedaannya.

### Diubah

- **Tabel hash memakai mask, bukan modulo.** `findEntry`, `findValueEntry`
  dan `tableFindString` dulu memakai `hash % capacity` untuk mencari slot dan
  untuk probing. Kapasitas tabel selalu pangkat dua, jadi
  `hash & (capacity - 1)` setara tetapi tanpa pembagian integer. `perf`
  menunjuk `findValueEntry` di **10,18%** waktu eksekusi. Hasilnya beban
  `kamus_himpunan` dan `olah_teks` turun **−43,8%** (114,6 → 64,4 ms); dua
  beban lain tidak terpengaruh karena tidak didominasi operasi tabel. Jumlah
  instruksi dan keluaran tetap identik — ini murni biaya per instruksi.
  `adjustCapacity`/`adjustValueCapacity` kini menolak kapasitas
  non-pangkat-dua dengan exit 70, supaya asumsi itu tidak bisa dilanggar
  diam-diam oleh perubahan `GROW_CAPACITY` di kemudian hari.

- **Benchmark mengukur beban nyata.** `tests/benchmark.sh` dulu mengukur
  `examples/performance_stress.idk`, `loop_nested.idk` dan
  `mutual_recursion.idk` — ketiganya placeholder "v0.2" berisi 4 baris
  komentar dan satu `cetak` (6 instruksi), padahal fiturnya sudah lama ada.
  Beban baru di `tests/beban/` (JSON, teks, kamus/himpunan, rekursi+OOP)
  masing-masing 3,5–6,4 juta instruksi. Laporannya kini memuat jumlah
  instruksi dari `indk profil` — deterministik, jadi regresi bytecode
  terlihat tanpa terganggu derau waktu. `date +%s.%N` dan `bc` diganti
  karena keduanya tidak ada di macOS, yang membuat benchmark lama mencetak
  baris kosong di sana.

### Diperbaiki

- **Kompilasi MENGGANTUNG tanpa batas pada sarang dalam diikuti fungsi
  anonim.** `{` sebanyak 200 lalu `fungsi(` membuat `indk` berputar selamanya
  sambil terus mengalokasi memori (~70 MB/detik) alih-alih melaporkan galat.
  Penyebabnya rantai dua fungsi yang sama-sama tidak memajukan parser:
  `parsePrecedence()` menolak turun di batas sarang dan `return` **tanpa
  mengonsumsi token**, lalu `synchronize()` berhenti PADA token sinkronisasi
  (`fungsi`, `jika`, `kelas`, …) tanpa memakannya juga — karena normalnya
  pemanggilnya yang memakai token itu. Untuk `fungsi(` tidak ada yang
  memakainya: itu fungsi anonim, jadi `declaration()` melemparnya ke jalur
  ekspresi, dan jalur itu justru yang menolak turun. Hasilnya
  `block()` → `declaration()` → `parsePrecedence()` → `synchronize()`
  berputar pada token yang sama. `parsePrecedence()` kini selalu mengonsumsi
  satu token saat menolak. Ditemukan oleh job fuzz CI; unit pemicunya
  diminimalkan dari 1987 menjadi 207 bita. Regresi:
  `tests/galat/sarang_fungsi_anonim_hang.idk`.
- **Harness tes galat menghitung tes yang MENGGANTUNG sebagai lulus.**
  `tests/test_all.sh` dan `tests/test_all.ps1` menjalankan `tests/galat/*.idk`
  tanpa batas waktu, jadi regresi parser seperti di atas menghentikan seluruh
  suite diam-diam sampai CI membunuhnya. Keduanya kini memakai batas 30 detik
  **dan** menolak hasilnya secara eksplisit: galat sudah tercetak sebelum
  parser menggantung, sehingga stderr tetap cocok dan exit code tetap bukan
  nol — tanpa penolakan eksplisit, tes yang menggantung justru dihitung hijau.

- **Blok `coba` di dalam fungsi rekursif berhenti di kedalaman 64**
  (GALAT-04). `TRY_MAX` bernilai 64 sementara `FRAMES_MAX` bernilai 1000,
  jadi try-handler menjadi batas yang jauh lebih ketat daripada rekursi itu
  sendiri. Galat batas itu ditangkap oleh `kecuali` di level luar, sehingga
  program mengembalikan **jawaban salah diam-diam** alih-alih berhenti:
  sebuah fungsi rekursif berisi `coba` yang dipanggil dengan 65 mengembalikan
  62. `TRY_MAX` kini disamakan dengan `FRAMES_MAX`; biayanya 23 KB pada VM
  yang sudah memakai 4 MB.

- **Kelas galat bawaan menerima pesan konstruktor** (GALAT-19).
  `GalatNilai("x")` dulu gagal dengan "Harapkan 0 argumen tetapi mendapat 1",
  sementara subkelas buatan pengguna berhasil. Penyebabnya
  `defineErrorClass()` di `src/vm.c` hanya menyetel `superclass`, padahal
  pewarisan VM memakai copy-down (`OP_INHERIT` menyalin tabel metode induk) —
  kelas yang dibuat dari C tidak pernah memperoleh `init`.

  Bug ini tersamarkan karena `naikkan GalatNilai("x")` **terlihat** jalan saat
  ditangkap `kecuali` tanpa `sebagai`: yang tertangkap sebenarnya galat
  runtime tadi, bukan `GalatNilai`, dan pesan aslinya hilang diam-diam.

  Kini semua kelas galat berperilaku sama. Akibatnya enam subkelas di pustaka
  standar yang hanya ada untuk membawa pesan (`GalatBita`, `GalatFungsional`,
  `GalatIterasi`, `GalatKoleksi`, `GalatStatistik`, `GalatWeb`) tidak lagi
  perlu `init` duplikat. Kelas **bukan** galat tanpa `init` tetap menolak
  argumen seperti sebelumnya. Regresi: `examples/galat_bawaan_pesan.idk`.

### Ditambahkan

- **API basis data bersama** (`pustaka/db.idk`): `db.sambung({"jenis": ...})`
  memberi satu bentuk pemanggilan untuk SQLite, MySQL/MariaDB, dan PostgreSQL.
  Metode `eksekusi`/`ambil_semua`/`ambil_satu`/`mulai`/`komit`/`batal`/`tutup`
  seragam di ketiga backend, dan `k.jenis()` menyebut backend yang dipakai.
  API lama tetap berjalan tanpa perubahan: `db_*` untuk SQLite serta
  `my.sambung`/`pg.sambung` langsung.

  Dialek SQL **tidak** diseragamkan. PostgreSQL tetap memakai `$1`, SQLite dan
  MySQL tetap `?`. Menerjemahkannya menuntut pemindai yang memahami literal
  string, dan pemindai yang salah merusak query yang sah seperti
  `WHERE nama = 'siapa?' AND id = ?` — tanda tanya pertama di situ adalah data,
  bukan parameter.

  Fitur ini sempat dicatat "terhalang" di ROADMAP karena `impor` dianggap
  membawa nama ke lingkup global sehingga `sambung()` kedua driver bentrok.
  Sejak modul punya globalnya sendiri, hal itu tidak lagi berlaku; catatan
  tersebut kedaluwarsa tanpa pernah dicabut.

### Diperbaiki

- **Kompilator tidak lagi mati tanpa pesan pada kode sangat bersarang**
  (T-W8). Sebelumnya proses berakhir dengan SIGSEGV di Linux/macOS dan
  `0xC00000FD` di Windows — tanpa baris, kolom, maupun sebab.

  Akar masalahnya bukan rekursi parser secara umum, melainkan ukuran frame
  `function()` di `src/compiler.c`: struct `Compiler` (empat larik
  `UINT8_COUNT`) ditambah empat larik parameter, totalnya **42.640 byte di
  stack per level sarang**. Pada stack 1 MB bawaan thread utama Windows,
  **16 `fungsi` bersarang** sudah menghabiskannya. Sebagai pembanding, kurung
  dan `jika` bersarang — yang hanya memakai frame parser biasa — tetap aman
  sampai 1.000 level. Catatan temuan asli menyebut ambang "32 fungsi atau
  ±2500 kurung"; pengukuran ulang per bentuk memberi angka yang jauh berbeda.

  Keduanya kini dipindahkan ke heap, sehingga frame `function()` turun dari
  42.640 menjadi **224 byte**. Ditambah satu penjaga kedalaman
  (`NESTING_MAX = 200`) pada dua corong rekursi — `parsePrecedence()` untuk
  ekspresi dan `block()` untuk pernyataan — kode yang melewati batas kini
  mendapat galat kompilasi biasa:

  ```
  [baris 202, kolom 15] Kesalahan di '{': Sarang terlalu dalam (maksimum 200);
  sederhanakan kode.
  ```

  Galat susulan diredam: tanpa itu, 220 level sarang menghasilkan 220 pesan
  "Harapkan '}'" yang mengubur pesan sebenarnya. Sarang yang masih wajar tidak
  terpengaruh — `examples/sarang_dalam.idk` (60 `jika`, 40 kurung, 12 `fungsi`
  bersarang) tetap berjalan. Regresi:
  `tests/galat/sarang_terlalu_dalam.*` dan
  `tests/galat/sarang_fungsi_terlalu_dalam.*`.

- ⚠️ **`kecuali X { ... }` tanpa `sebagai` kini bertipe** (GALAT-03). Dulu
  bentuk ini selalu diartikan sebagai klausa tangkap-semua dengan `X` sebagai
  nama pengikat, jadi `kecuali GalatNilai {` diam-diam menangkap **semua**
  galat — termasuk di contoh yang ikut dirilis (`db_batas_tunggu.idk`,
  `db_blob.idk`, `http_pengalihan.idk`, `web_router.idk`). Kini `X` dicari
  saat jalan: bila ia merujuk sebuah kelas galat, klausanya bertipe seperti
  `except ValueError:` di Python; bila tidak, ia tetap nama pengikat yang
  menangkap semua. Nama itu tetap terikat di dalam blok pada kedua kasus.
  Bentuk ini tetap wajib menjadi klausa terakhir. Regresi:
  `examples/kecuali_bertipe.idk`.

- **Galat pertama tidak lagi ditimpa pemanggilnya** di `nativeRaise()` dan
  `nativeRaiseKelas()`. `himpunan([t])` pada tupel bersiklus melaporkan
  `GalatTipe` padahal `setAdd` sudah menaikkan `GalatRekursi`.
  `runtimeError()` sudah punya penjagaan ini; kedua fungsi tadi belum.

- **Nama yang tidak terdefinisi menaikkan `GalatNama`** (GALAT-06,
  ALUR-FUNGSI-09). Dulu kelasnya `Galat` biasa, sehingga
  `kecuali GalatNama` tidak pernah cocok padahal panduan sudah menjanjikan
  kelas itu. Berlaku untuk pembacaan di tingkat atas, di dalam fungsi, dan
  setelah `buang`, karena semuanya lewat satu helper yang sama. Galat lain
  tidak ikut berubah. Regresi: `examples/galat_nama.idk`.

- **Sumber berisi bita NUL ditolak** (T-U11). Dulu sumber berakhir di NUL
  pertama: `indk rapikan --tulis` memotong berkasnya (data hilang, exit 0),
  dan menjalankannya diam-diam mengabaikan kode setelah NUL. Kini berkas
  seperti itu ditolak dengan exit 65 tanpa ditulis, dan modul seperti itu
  menaikkan galat yang bisa ditangkap. Regresi: `tests/galat/sumber_nul.idk`,
  `tests/test_format.sh`.

- **`input()` dan REPL membaca baris dengan panjang berapa pun** (T-U10).
  Di 1.4.0 keduanya memotong baris pada 1023 bita, dan sisanya muncul pada
  pembacaan berikutnya. Buffer blok REPL juga diam-diam membuang baris di
  atas 8192 bita. Regresi: `tests/test_baris_panjang.sh`.

- ⚠️ **Variabel `untuk` mengikuti lingkup Python** (T-U4, keputusan K21).
  Dulu ia menjadi lokal blok baru: `k = 99; untuk k dalam rentang(3) { }`
  menyisakan `k` = 99 (hasil salah tanpa galat), dan nama itu tidak
  terdefinisi setelah loop. Kini setiap elemen di-assign seperti
  `k = elemen`: global di tingkat skrip, lokal fungsi (K3) di dalam fungsi,
  dan mengikuti `global`/`nonlokal`, sehingga nilainya tetap elemen terakhir.
  Variabel comprehension tetap privat. Kode yang sengaja membayangi nama di
  luar dengan variabel loop kini menimpanya. Regresi:
  `examples/untuk_lingkup.idk`.

- **`bita.ke_teks` menolak UTF-8 tidak sah** (T-U16). Dulu hanya byte pemimpin
  yang diperiksa, sehingga `[0xC0, 0x80]` (*overlong*) menjadi NUL dan byte
  lanjutan yang salah, surrogate, serta kode di atas `U+10FFFF` diterima.
  Kini `GalatBita` dinaikkan untuk setiap urutan yang juga ditolak
  `bytes.decode("utf-8")`. Regresi: `examples/bita_utf8_ketat.idk`.

- ⚠️ **`json_urai` dan `json_teks` mengikuti RFC 8259** (T-U15).
  `json_urai` kini mendekode `\uXXXX` (termasuk pasangan surrogate), `\b`,
  dan `\f`, serta menolak escape tidak sah, karakter kontrol mentah, angka
  cacat (`01`, `1.`, `-`), dan data tambahan (`"1 2"`), yang dulu diterima
  diam-diam. `json_teks` meng-escape karakter kontrol, menolak
  `nan`/`takhingga`, dan mengubah kunci angka/logika/kosong menjadi teks
  seperti `json.dumps`, alih-alih crash. Regresi: `examples/json_ketat.idk`.

- **`bulat`/`lantai`/`atap`/`bulatkan` dari `nan` menaikkan `GalatNilai`**
  (T-U12). Di 1.4.0 hasilnya diam-diam `-9223372036854775808`, karena NaN
  lolos dari pemeriksaan jangkauan lalu sampai ke cast C yang tidak
  terdefinisi. `bulatkan(takhingga)` kini menaikkan `GalatLuapan` dan tidak
  lagi mengembalikan desimal, dan argumen `bulat` di atas 2^53 dikembalikan
  utuh. Regresi: `examples/nan_ke_bulat.idk`.

- ⚠️ **`==` dan hash pada struktur sangat dalam menaikkan `GalatRekursi`**
  (T-U13, T-U14). Dulu `==` berhenti di 64 tingkat dan diam-diam menjawab
  `salah`, termasuk untuk dua daftar yang identik, dan hash tupel bersarang
  ribuan tingkat sebagai kunci kamus membuat interpreter crash. Kini batasnya
  1000 tingkat, seperti `RecursionError` di Python. Daftar yang memuat dirinya
  sendiri juga menaikkan galat, bukan lagi `salah`. Regresi:
  `examples/kedalaman_sama_hash.idk`, `tests/galat/hash_tupel_dalam.idk`.

- ⚠️ **`impor "berkas.idk"` tidak lagi menulis ke tabel builtin bersama**
  (T-U8). Berkas itu kini dijalankan di namespace pengimpornya, sekali per
  pengimpor. Di 1.4.0, `fungsi panjang(x)` di berkas yang diimpor dengan cara
  ini membajak `panjang()` di dalam modul bernamespace, termasuk pustaka
  standar: `impor "pustaka/base64.idk"` lalu `impor json` merusak
  `json.json_teks`. Modul yang selama ini memakai nama dari berkas yang
  hanya diimpor oleh skrip utama kini perlu mengimpornya sendiri. Regresi:
  `examples/impor_berkas_namespace.idk`.

- ⚠️ **Comprehension menolak token setelah syaratnya** (T-U9). `jika` kedua,
  `untuk` kedua, atau token sampah sebelum kurung penutup dulu dibuang
  diam-diam, jadi `[x untuk x dalam a jika p jika q]` mengabaikan `q`. Kini
  bentuk itu galat kompilasi dengan pesan yang menyebut sebabnya. Regresi:
  `tests/galat/komprehensi_*`.

- **Galat dari `_logika_` dan `_sama_` langsung membatalkan operasinya**
  (T-U7). Di 1.4.0, `jika B() { ... }` dengan `_logika_` yang naik tetap
  menjalankan cabangnya. Pada tabrakan hash, `d[k] = v` dengan `_sama_`
  yang naik juga tetap menyisipkan kunci kedua, dan galatnya baru muncul
  belakangan dengan jenis yang hilang. Tabel kamus kini menyimpan hash
  setiap kunci, sehingga `_sama_` hanya dipanggil bila hash-nya sama (seperti
  Python) dan resize tidak lagi memanggil `_hash_`. Regresi:
  `examples/oop/galat_ajaib_langsung.idk`.

- **Subkelas galat bawaan mewarisi seluruh leluhurnya** (T-U6). Di 1.4.0,
  `kelas B2 < GalatBagiNol { }` tidak tertangkap oleh `kecuali
  GalatAritmetika` maupun `kecuali Galat`: MRO C3 menganggap kelas galat yang
  didefinisikan dari C tidak punya leluhur. Regresi:
  `examples/galat_turunan_bawaan.idk`.

- ⚠️ **`pastikan(kondisi, pesan)` dengan kurung ditolak saat kompilasi**
  (T-U5). Kurung dengan koma membentuk satu tupel yang selalu benar, jadi di
  1.4.0 penegasan itu tidak pernah gagal, padahal panduan sudah menyebutnya
  sebagai galat kompilasi. `pastikan(kondisi)` dan tupel di dalam ekspresi
  yang lebih besar tetap sah. Regresi: `tests/galat/pastikan_tupel.idk`.

- **`x = a jika c selain b` meng-assign hasil kondisional** (T-U2). Di 1.4.0
  bentuk ini dikompilasi sebagai `(x = a) jika c selain b`, jadi bila
  kondisinya salah `x` tidak di-assign sama sekali. Sisi kanan assignment
  ke nama kini ekspresi penuh, sama seperti `o.x = ...` dan `d[i] = ...`.
  Regresi: `examples/kondisional_assignment.idk`.
- **Argumen bernama di dalam fungsi tidak lagi mengosongkan argumen posisi**
  (T-U3). Di 1.4.0, `g(D, b = 2)` di dalam fungsi membaca global/upvalue `D`
  sebagai `kosong` karena prescan K3 menganggap `D, b =` sebagai
  pembongkaran `a, b = ...`. Contohnya `statistik.kuantil(DATA, n = 2)` di
  dalam fungsi. Regresi: `examples/argumen_bernama_lingkup.idk`.
- **Windows: jalur dan nama berkas di luar code page ANSI** (T-W1 dari uji
  paket 1.4.0). `indk 日本\s.idk` dulu gagal dengan
  `Tidak dapat membuka file "??/s.idk".` (kode 74), dan
  `tulis_semua("keluaran_☕.txt", ...)` membuat berkas bernama
  `keluaran_â˜•.txt`. Sekarang `argumen` diambil dari baris perintah Unicode
  (`GetCommandLineW`) dan diubah ke UTF-8. Semua pembukaan berkas lewat
  `_wfopen`/`_wremove`: skrip, modul `impor`, `buka_berkas`, `baca_semua`,
  `tulis_semua`, `tambah_berkas`, `ada_berkas`, `hapus_berkas`,
  `rapikan --tulis`. `INDK_PATH` dan `ambil_lingkungan()` dibaca sebagai
  Unicode, dan `_jalur_nyata()` memakai `_wfullpath`. Di Linux/macOS tidak
  ada perubahan.
- **Berkas sumber UTF-8 dengan BOM diterima** (T-W2, semua OS). BOM
  `EF BB BF` di awal skrip atau modul dulu ditolak dengan
  `Karakter tidak dikenal`; sekarang dilewati, seperti Python. Pembacaan data
  (`baca_semua`) tetap tidak membuang BOM.
- **Windows: teks non-ASCII terbaca benar di konsol** (T-W3). `cetak("☕")`
  dulu tampil `Γÿò` di konsol code page 437/850. `indk` sekarang mengubah
  code page konsol ke UTF-8 selama berjalan dan mengembalikannya saat keluar,
  jadi `chcp 65001` tidak lagi diperlukan. Keluaran yang dialihkan ke berkas
  atau pipa tidak berubah (selalu byte UTF-8).
- **`my.eksekusi(sql)` tanpa parameter galat, padahal `pg.eksekusi(sql)` jalan.**
  `kueri`/`eksekusi`/`ambil_semua`/`ambil_satu` di `pustaka/my.idk` tidak punya
  nilai bawaan untuk `parameter`, sehingga kode yang sah di PostgreSQL gagal
  begitu dipindah ke MySQL. Kini seragam dengan `pg.idk`.

- **Parsing badan `application/x-www-form-urlencoded` tidak lagi kuadratik.**
  `_web_dekode_persen()` merakit hasil dengan `hasil = hasil + c` per
  karakter; teks immutable, jadi setiap gabung menyalin seluruh string yang
  sudah terkumpul. Terukur: 40 KB = 1,6 s, 80 KB = 6,7 s, 100 KB = 10,7 s —
  empat kali lipat tiap ukuran berlipat dua. Karena gelung server melayani
  satu permintaan pada satu waktu, satu POST 1 MB membekukan server: itu DoS
  lewat satu permintaan yang sah. Sekarang 100 KB = 0,06 s (178x) dan 1 MB
  = 0,6 s.
- **`teks_gabung()` menjadi builtin C** (`src/native.c`). Akar masalahnya ada
  di sini: versi `pustaka/teks.idk` memakai pola `hasil = hasil + ...` yang
  sama, jadi memperbaiki pemanggilnya saja hanya memindahkan biaya. Definisi
  `.idk`-nya dihapus, bukan disimpan sebagai cadangan — fungsi di pustaka
  akan **membayangi** builtin untuk setiap berkas yang mengimpornya, sehingga
  perbaikannya diam-diam tidak terpakai. Pola yang sama masih ada di 47
  tempat lain pada 10 berkas pustaka; itu utang yang dicatat, belum dibayar.

### Ditambahkan

- **HTTPS untuk klien HTTP** — `http_ambil()` menerima `https://`, dengan
  builtin baru `tls_mulai(soket, host)` yang meningkatkan soket tersambung
  menjadi TLS. Verifikasi sertifikat **selalu aktif dan tidak bisa dimatikan**:
  tidak ada parameter, variabel lingkungan, atau "mode pengembangan" untuk
  melewatinya. Sertifikat kedaluwarsa, nama host tidak cocok, dan penerbit tak
  dikenal ditolak dengan pesan yang menyebut sebabnya — diuji terhadap
  `badssl.com` di `tests/https_klien.sh`. SNI selalu dikirim. Daftar CA diambil
  dari `INDK_CA_BUNDLE` bila disetel, selain itu dari jalur sistem yang umum;
  bila tidak ada satu pun, koneksi **gagal** alih-alih diam-diam melewati
  verifikasi. Ditenagai mbedTLS 3.6.2 (LTS) yang di-vendor di
  `src/vendor/mbedtls/` dan ditaut statis, jadi biner tetap hanya bergantung
  pada libc/libm seperti sebelumnya. (Fase 11 langkah 2)
- **Pengalihan `https://` → `http://` ditolak.** Mengikutinya diam-diam berarti
  permintaan berikutnya — termasuk header `Authorization` dan cookie — dikirim
  polos padahal pengguna meminta koneksi terenkripsi. Sekaligus memperbaiki
  bug lama: `_http_gabung_url()` meng-hardcode `http://`, sehingga pengalihan
  **relatif** pada situs HTTPS ikut turun ke HTTP polos.
- **CCM dicabut dari mbedTLS** lewat `src/mbedtls_user_config.h`
  (`MBEDTLS_USER_CONFIG_FILE`). TLS 1.3 memakai AES-GCM atau
  ChaCha20-Poly1305, jadi CCM tidak pernah terpakai klien kita; mencabutnya
  mengecilkan permukaan audit — alasan yang sama yang membuat mbedTLS dipilih
  daripada OpenSSL. Berkas konfigurasinya sengaja di luar `src/vendor/` supaya
  tidak terhapus saat `tools/vendor_mbedtls.sh` menulis ulang direktori vendor
  pada pembaruan CVE. Berkasnya juga dibuang dari pohon vendor oleh `tools/vendor_mbedtls.sh`, karena pemindai statis membaca berkas yang ada di repo, bukan yang dikompilasi. Permukaan audit 132.812 -> 132.049 baris. Diverifikasi: nol simbol `mbedtls_ccm` di biner.
- **Windows menaut `-lbcrypt`** selain `-lws2_32`: mbedTLS mengambil entropi
  lewat `BCryptGenRandom`. Tanpa itu link gagal **hanya di Windows**, jadi
  build Linux/macOS yang hijau tidak membuktikan apa pun.
- **`tools/vendor_mbedtls.sh`** — mengambil ulang mbedTLS dengan sha256 yang
  terkunci di skrip. Kode pihak ketiga di dalam repo adalah utang keamanan;
  skrip ini membuat pembaruan saat ada CVE menjadi satu perintah, bukan
  tebak-tebakan berkas mana yang dulu disalin.
- **Penyajian berkas statis** di `pustaka/web.idk`: `app.statis(prefiks, akar)`
  menyajikan CSS, JavaScript, gambar, dan font dari disk. Hanya `GET`/`HEAD`
  (lainnya **405** dengan `Allow: GET, HEAD`); direktori tidak pernah didaftar
  isinya; MIME ditentukan dari ekstensi dengan `application/octet-stream`
  sebagai jatuh-tempo aman, selalu disertai `X-Content-Type-Options: nosniff`;
  `ETag` lemah dengan `If-None-Match` dijawab **304**. (Fase 11 langkah 5)
- **`panjang_byte(teks)`** — panjang teks dalam byte, pelengkap `panjang()`
  yang menghitung karakter.
- **Bab panduan `08_DEPLOYMENT.md`** — Docker, reverse proxy, TLS, isolasi
  proses, batas resource, dan risiko membuka port (termasuk `docker run -p`
  yang menembus `ufw`). Konfigurasinya ada di `docker/aplikasi/` dan
  dijalankan sungguhan, bukan ditulis dari ingatan. (Fase 11 langkah 8)
- **Argumen alamat opsional `soket_dengar(port, alamat)`** dan parameter
  kelima `app.jalankan(..., alamat)`. Bawaannya tetap `"127.0.0.1"`; hanya
  `"0.0.0.0"` yang diterima sebagai alternatif.
- **Job CI `test-musl`** — membangun dan menjalankan seluruh suite di Alpine.
- **Contoh `examples/api_siswa.idk`** — CRUD REST API di atas SQLite yang
  menggabungkan `web.Aplikasi`, `app.jalankan()`, builtin `db_*`, dan
  `pustaka/json.idk`. Menunjukkan dua hal yang mudah salah: memeriksa
  `db_baris_terpengaruh()` sesudah UPDATE/DELETE (tanpa itu id yang tidak ada
  tetap dijawab `200`) dan memakai parameter `?` alih-alih merangkai teks SQL.
  Diuji dengan curl di `tests/api_siswa_server.sh`, termasuk payload injeksi
  SQL. (Fase 11 langkah 7, sebagian: database masih SQLite)
- **`app.jalankan(port, ...)` dan `app.berhenti()`** di `pustaka/web.idk` —
  gelung server bawaan: batas waktu per koneksi (bawaan 10 detik), galat satu
  koneksi tidak mematikan server, soket selalu ditutup, berhenti anggun, dan
  log per permintaan lewat modul `log` (pencatat `web`). Sebelumnya setiap
  pemakai menyalin gelung soketnya sendiri dan **empat dari lima salinan di
  repo ini lupa memasang batas waktu**. (Fase 11 langkah 6)
- **`soket_batas_waktu(soket, detik)`** — batas waktu baca dan tulis untuk satu
  soket (`SO_RCVTIMEO`/`SO_SNDTIMEO`). `0` berarti tanpa batas, yaitu perilaku
  bawaan yang lama, sehingga program 1.4.x tidak berubah. Detik boleh pecahan.
  (Fase 11 langkah 6, bagian pertama)

### Diperbaiki

- **`indk` gagal dibangun di musl/Alpine.** `struct timeval` dan
  `suseconds_t` yang dipakai `soket_batas_waktu()` berasal dari
  `<sys/time.h>`; glibc menariknya diam-diam lewat header lain, musl tidak.
  Akibatnya `docker/images/Dockerfile` ikut gagal di-build. Seluruh CI tetap
  hijau karena tidak ada satu pun job yang memakai musl -- sekarang ada.
- **Server tidak bisa dipakai di dalam container.** `soket_dengar()` selalu
  mengikat `127.0.0.1`, yang di dalam container hanya bisa dihubungi dari
  dalam container itu sendiri; reverse proxy mendapat `Connection refused`
  padahal server hidup dan mencatat "siap". Bawaan loopback dipertahankan
  (itu default aman yang disengaja), alamat kini bisa dipilih.
- **Pesan galat penangan menempel ke baris log berikutnya.** `tulis_galat()`
  tidak menambah baris baru sendiri (sesuai dokumentasinya), tetapi
  `pustaka/web.idk` lupa menambahkannya, sehingga keluaran stderr menjadi
  `...sengaja meledakGALAT:web:GET /x 500`.
- **`Content-Length` salah untuk badan biner.** `_http_panjang_byte()`
  menghitung ulang lebar UTF-8 per karakter, sehingga berkas 5 byte
  `FF FE 41 00 80` dilaporkan 6 byte dan byte non-UTF-8 hilang dari iterasi
  karakter. Klien menggantung menunggu byte yang tidak pernah datang. Kini
  memakai `panjang_byte()` yang membaca panjang byte sebenarnya.

### Keamanan

- **Slowloris: satu klien yang diam bisa membekukan server HTTP.** Tidak ada
  batas waktu pada soket mana pun, jadi `soket_baca()` menunggu selamanya dan
  `soket_terima()` berikutnya tidak pernah tercapai — penyerang cukup menyambung
  lalu tidak mengirim apa pun. Dibuktikan dengan server sungguhan sebelum
  perbaikan. Kini `soket_batas_waktu()` menutup lubang itu, dan batas waktu
  dibedakan dari koneksi tertutup supaya klien lambat tidak dijawab `400`.
  Ada tes regresinya (`tests/web_batas_waktu_server.sh`).
- Penyaji berkas statis menolak traversal **setelah** dekode persen, sehingga
  `%2e%2e%2f` tertutup serapat `../`; backslash diperlakukan sebagai pemisah
  (penting di Windows) dan byte NUL ditolak. Path yang memuat `..` ditolak
  seluruhnya, bukan "dinaikkan satu level".
- Symlink di dalam akar yang menunjuk keluar ditolak. Celah ini nyata dan
  ditemukan saat pengujian — tautan `bocor.txt` → `../rahasia.txt` sempat
  menyajikan isi berkas di luar akar dengan HTTP 200. Builtin `_jalur_nyata()`
  ditambahkan untuk membuktikan jalur nyata berkas berada di bawah jalur nyata
  akar; ada tes regresinya.

### Rilis

- **Paket Linux v1.2.0, v1.3.0, dan v1.3.1 ditarik** dari GitHub Releases.
  Binernya gagal dijalankan dengan `CPU ISA level is lower than required`
  (keluar kode 127) karena dibangun tanpa `-march=x86-64`, sehingga menuntut
  `x86-64-v3` (AVX2) dan ditolak loader pada CPU lama serta banyak VPS/KVM.
  Diverifikasi dengan menjalankan tiap biner, bukan membaca workflow. Paket
  Windows dan macOS ketiga rilis itu diuji baik dan tetap tersedia. Perbaikan
  sudah ada sejak v1.3.2; pengguna Linux memakai v1.4.0.

### Perkakas

- Ekstensi VS Code menjalankan language server LSP untuk diagnostics sintaks
  dan warning `indk periksa`, completion keyword/builtin/simbol dokumen, hover,
  dan go-to-definition lokal.
- LSP menganalisis dengan `indk --dis` / `indk periksa` (tanpa mengeksekusi
  program), termasuk dokumen belum disimpan; hasil analisis usang dibatalkan.
- Tes protokol JSON-RPC tersedia di `editors/vscode-indonesiaku/test-lsp.js`
  dan dijalankan CI lintas Linux/macOS/Windows, bersama tes formatter ekstensi.

### Ditambahkan
- **Driver PostgreSQL** (`pustaka/pg.idk`): bicara wire protocol v3 langsung
  tanpa libpq, jadi biner tetap hanya menaut libc dan libm. Autentikasi
  **SCRAM-SHA-256**; `password` polos dan `md5` ditolak. Parameter dikirim
  sebagai parameter protokol (Parse/Bind), bukan dirangkai ke SQL.
  `sambung()`, `eksekusi()`, `ambil_satu()`, `ambil_semua()`,
  `mulai()`/`komit()`/`batal()`, `tutup()` idempoten.
- `soket_tulis(soket, daftar_bita)` menerima daftar bulat 0..255 di samping
  teks, dan `soket_baca_bita(soket)` mengembalikannya. Tanpa ini protokol
  biner mustahil: `chr(255)` di dalam teks ter-enkode UTF-8 menjadi **dua**
  byte di kawat.
- `hmac_sha256(kunci, pesan)` dan `pbkdf2_sha256(sandi, garam, iterasi,
  panjang)` memakai mbedTLS yang sudah di-vendor. Iterasi dibatasi
  1..10.000.000 karena nilainya datang dari server dalam SCRAM.
- `examples/api_siswa_pg.idk`: REST API CRUD di atas PostgreSQL.

### Diperbaiki
- Tabel ringkasan status fase menyebut Fase 10 "terblokir pustaka klien
  eksternal". Keliru: penghalang sebenarnya adalah I/O soket yang hanya
  menerima teks.

### Ditambahkan
- **Driver MySQL/MariaDB** (`pustaka/my.idk`): protokol MySQL langsung tanpa
  libmysqlclient. Autentikasi `mysql_native_password`;
  `caching_sha2_password` ditolak dengan pesan jelas. Parameter lewat
  **prepared statement** (COM_STMT_PREPARE/EXECUTE). Kolom INT kembali
  sebagai angka, bukan teks.

### Diperbaiki
- Versi pertama driver MySQL memakai COM_QUERY dengan escaping manual dan
  **merusak data** di bawah `sql_mode = NO_BACKSLASH_ESCAPES`: `x\` tersimpan
  sebagai `x\\`. Escaping yang benar mustahil tanpa melacak sql_mode server,
  jadi jalur itu diganti prepared statement.
- Sisa draf COM_QUERY (`_kutip`, `_rakit`) dan enam fungsi yang tersalin dua
  kali masih tertinggal di `pustaka/my.idk`. Definisi kedua menimpa yang
  pertama, jadi perilaku tidak terpengaruh, tetapi berkasnya menyimpan 195
  baris mati termasuk perakit SQL berbasis penggabungan teks yang sudah
  ditinggalkan. Seluruh sisa itu dihapus.
- Ekstensi VS Code tidak menyorot `teks_gabung`, `hmac_sha256`,
  `pbkdf2_sha256`, dan `soket_baca_bita` karena daftar builtinnya diketik
  tangan dan tertinggal dari `defineNative()`. Daftar di `language-data.js`
  dan pola `tmLanguage.json` kini diselaraskan, dan `tests/builtin_konsisten.sh`
  menjaga keduanya tetap sama persis dengan VM.

## [1.4.0] - 2026-10-29

### Ditambahkan

- **Parsing badan request** di `pustaka/web.idk` — `application/json`,
  `application/x-www-form-urlencoded`, dan `multipart/form-data` (termasuk
  unggahan berkas) terisi otomatis ke `req["json"]`, `req["form"]`, dan
  `req["berkas"]`. Batas ukuran badan (bawaan **1 MB**) dijawab **413**
  berdasarkan `Content-Length`, sebelum data dikumpulkan di memori.
  `Transfer-Encoding: chunked` ditolak **411**; JSON rusak dijawab **400**,
  bukan 500. Melengkapi langkah 4 Fase 11. (Fase 11)
- **`web.baca_permintaan(klien, maks_badan)`** — membaca permintaan HTTP
  sampai `Content-Length` terpenuhi. `soket_baca()` hanya mengembalikan apa
  yang sudah tiba (maks ~64 KB per panggilan), sehingga gelung soket yang
  memanggilnya sekali **memotong badan besar diam-diam**. (Fase 11)
- **Paket siap pasang per sistem operasi.** Rilis kini menyertakan
  `indonesiaku_<versi>_amd64.deb` (Debian/Ubuntu) dan
  `Indonesiaku-Setup-<versi>.exe` (Windows) selain arsip `.tar.gz`/`.zip`
  yang sudah ada. Arsip **tidak dihapus**: distribusi non-Debian dan macOS
  tetap memakainya.
- **`indk --periksa-pembaruan`** — membandingkan versi terpasang dengan rilis
  terbaru di GitHub, lalu mencetak perintah pemutakhiran yang sesuai sistem
  operasi yang sedang berjalan. Hanya berjalan bila diminta; menjalankan
  skrip biasa tidak pernah menghubungi jaringan. Tidak ada pembaruan
  otomatis: mengganti biner yang sedang berjalan butuh hak root, verifikasi
  tanda tangan, dan berisiko meninggalkan pasangan setengah jadi.
- **`pustaka/pembaruan.idk`** — perbandingan versi numerik per komponen
  (`1.9.0 < 1.10.0`, yang salah bila dibandingkan sebagai teks) dan penyusun
  pesan pemutakhiran per OS. Hanya menyarankan berkas yang benar-benar ada
  pada rilis tersebut, sehingga pengguna versi lama tidak diarahkan ke
  URL 404.

- **Modul server HTTP `pustaka/web.idk`** — routing metode/path, parameter
  URL (`/siswa/:id`), query string yang diurai beserta dekode persen,
  middleware berantai, dan penangan galat. Path cocok tetapi metode tidak
  menghasilkan **405 dengan header `Allow`**, bukan 404. Galat penangan
  menjadi 500 **tanpa membocorkan detail** ke klien. Melengkapi langkah 3
  Fase 11. (Fase 11)

- **BLOB (data biner) SQLite** kini didukung masuk-keluar, memakai konvensi
  `pustaka/bita.idk`: daftar bulat 0..255. Daftar kosong `[]` menjadi BLOB
  berukuran nol, berbeda dari `kosong` yang menjadi `NULL`. Elemen tidak sah
  ditolak dengan galat yang menyebut posisinya. Melengkapi langkah 2
  Fase 10. (Fase 10)
- `db_batas_tunggu(db, milidetik)` — mengatur lama koneksi SQLite menunggu
  ketika basis data sedang dikunci koneksi lain. Bawaan SQLite adalah `0`,
  artinya penulisan langsung gagal begitu ada kunci; kini pemanggil bisa
  memilih menunggu. Milidetik di luar jangkauan `int` C ditolak alih-alih
  dipersempit diam-diam menjadi batas yang keliru. (Fase 10)
- `http_ambil()` kini mengikuti pengalihan HTTP (301/302/303/307/308) sampai
  lima lompatan. Parameter baru `maks_pengalihan` mengatur batasnya; `0`
  mengembalikan perilaku lama. `303` dilanjutkan dengan `GET`, `307`/`308`
  mempertahankan metode dan badan. Rantai berputar terdeteksi tanpa menunggu
  batas lompatan habis. (Fase 11)

### Diperbaiki

- **Server HTTP menggantung pada badan ber-UTF-8.** `web.baca_permintaan()`
  menunggu `panjang(badan) >= Content-Length`, padahal `panjang()` menghitung
  **karakter** sedangkan `Content-Length` menghitung **byte**. Setiap badan
  dengan karakter non-ASCII (mis. `"kopi ☕"`) membuat server menunggu byte
  yang tidak akan pernah datang. Kini dihitung dalam byte.

### Berubah

- **Perbaikan bug:** `examples/todo_server.idk` memanggil `soket_baca()`
  sekali, jadi permintaan dengan badan lebih besar dari ~64 KB terpotong
  diam-diam. Kini memakai `web.baca_permintaan()`.
- `_http_frasa()` mengenal 401, 403, 411, 413, dan 415, serta memakai frasa
  generik per kelas untuk kode lain. Sebelumnya setiap kode tak dikenal
  menghasilkan baris status menyesatkan seperti `HTTP/1.1 413 OK`.

- **Perbaikan bug:** `http_urai_request()` kini menyeragamkan nama header
  menjadi huruf kecil. Nama header HTTP bersifat case-insensitive (RFC 9110
  §5.1), dan klien di berkas yang sama sudah melakukannya — sisi server
  tidak, sehingga klien yang mengirim `host:` tidak terbaca oleh kode yang
  mencari `"Host"`. Pakai `req["header"]["host"]`.

- **Perubahan perilaku:** program yang memeriksa `r.status == 301` dari
  `http_ambil()` kini akan melihat `200` karena pengalihan diikuti. Beri
  `maks_pengalihan = 0` untuk mendapatkan respons pengalihan apa adanya.
- `.url` pada `Respons` berisi URL terakhir setelah pengalihan, bukan URL
  yang diminta.

## [1.3.2] - 2026-10-28

### Diperbaiki

- **Penanda ISA palsu membuat binari rilis Linux ditolak loader.** Walau
  SETIAP perintah kompilasi memakai `-march=x86-64` (diperiksa di log
  runner: 32 dari 32), artefak v1.3.1 tetap bertanda
  `x86 ISA needed: ...x86-64-v3` dan mati dengan "CPU ISA level is lower
  than required". Penandanya **palsu**: `objdump` memastikan binari itu
  nol instruksi `ymm`/`zmm`. Penanda ikut dari objek startup glibc milik
  image runner, bukan dari kode Indonesiaku.
- Workflow kini membuang `.note.gnu.property` pada artefak Linux, dengan
  dua guard yang menggagalkan build: kalau binari benar-benar memakai
  AVX2 (penandanya sah, jangan dibuang), dan kalau penanda masih menuntut
  di atas baseline setelah dibuang. Rilis cacat tidak bisa terbit diam-diam
  lagi.
- Konsekuensi diterima sadar: penanda CET (IBT/SHSTK) ikut terbuang.

## [1.3.1] - 2026-10-28

### Diperbaiki

- **Perbaikan AVX2 di 1.3.0 tidak benar-benar sampai ke artefak.** Langkah
  "Run test suite" menjalankan `tests/test_all.sh`, yang melakukan
  `make clean && make` sendiri tanpa `EXTRA_CFLAGS` — rebuild itu menimpa
  binari ber-`-march` yang baru saja dibangun. Artefak Linux v1.3.0 karena
  itu TETAP menuntut `x86-64-v3` walau workflow-nya sudah benar.
  `test_all.sh` kini meneruskan `EXTRA_CFLAGS`, dan workflow mengekspornya
  lewat `matrix.extra_cflags`.

## [1.3.0] - 2026-10-28

### Bahasa

- **Anotasi tipe opsional** (Fase 8). `fungsi sapa(nama: teks) -> teks {}`
  dan `variabel umur: bilangan = 30`. Anotasi **di-parse lalu dibuang**:
  tidak diperiksa saat runtime, tidak ada opcode baru, tidak ada alokasi
  baru. Bytecode berkas beranotasi identik dengan versi tanpa anotasi —
  dijaga oleh `tests/test_anotasi_bytecode.sh`.
- Nama tipe adalah identifier biasa, bukan kata kunci baru, jadi kode lama
  yang memakai `teks`/`bilangan` sebagai nama variabel tetap jalan.
  `kosong` diterima khusus agar `-> kosong` bisa ditulis.
- Token baru `->` (`TOKEN_PANAH`).

### Diperbaiki

- **Binari rilis menuntut AVX2.** Paket Linux v1.1.0 dan v1.2.0 mati dengan
  "CPU ISA level is lower than required" di CPU lawas dan banyak VPS/KVM,
  karena runner GitHub mengompilasi ke `x86-64-v3`. Artefak Linux dan
  Windows kini dibangun dengan `-march=x86-64`. macOS sengaja dikecualikan
  (runner `macos-latest` arm64). `Makefile` menerima `EXTRA_CFLAGS` tanpa
  menimpa `-O2`/`-Isrc`.
- `prescanFunctionBody()` tidak lagi mengalokasikan slot lokal palsu untuk
  nama tipe pada `variabel a: teks = ...` di dalam fungsi, pada semua
  kedalaman blok.

### Perkakas

- Ekstensi VS Code: `->` diwarnai sebagai operator; snippet `fungsitipe`.

## [1.2.0] - 2026-10-28

### Perkakas (Fase 7)

- **`indk --dis <skrip>`**: tampilkan bytecode tanpa menjalankan program,
  rekursif ke fungsi bersarang.
- **`indk rapikan <skrip>`** (`--tulis`, `--periksa`): formatter kode.
  Merapikan indentasi (4 spasi) dan spasi operator, membedakan `{` kamus dari
  `{` blok, serta mempertahankan komentar beserta posisinya. Scanner mendapat
  `TOKEN_KOMENTAR` yang **opt-in** — default mati, sehingga compiler tidak
  melihat token komentar sama sekali (dibuktikan: bytecode 117 berkas identik
  sebelum/sesudah). Dijamin tidak mengubah arti program: bytecode 117 berkas
  identik sebelum/sesudah format, hasilnya idempoten. Tidak memecah atau
  menyambung baris — mengikuti baris asli penulis.
- **`indk uji <skrip>`**: jalankan setiap fungsi tingkat atas bernama `uji_*`
  dan laporkan hasilnya. Asersi memakai `pastikan`; sebuah `pastikan` yang
  gagal menandai tesnya gagal tanpa menghentikan tes lain. Urutan diurutkan
  berdasarkan nama agar keluarannya stabil. Exit 0 bila semua lulus, 1 bila
  ada yang gagal atau tidak ada fungsi `uji_*`.
- **`indk periksa <skrip>`**: linter yang melaporkan variabel lokal yang
  dideklarasikan tapi tidak pernah dibaca. Exit 0 bila bersih, 1 bila ada
  temuan. Nama berawalan `_` dikecualikan. Hanya `variabel x` yang eksplisit
  yang diperiksa — parameter, variabel loop, dan `kecuali ... sebagai e`
  tidak, karena itu bukan deklarasi pilihan penulis.
  Menjalankan program secara normal **tidak berubah sama sekali**: tidak ada
  satu pun bytecode tambahan yang dipancarkan.
- **Formatter VS Code**: `Format Document` pada berkas `.idk` menjalankan
  `indk rapikan` dan menerapkan hasilnya sebagai edit editor (undo tetap
  berfungsi). Interpreter dipanggil tanpa `--tulis`, jadi berkas di disk tidak
  disentuh. Jalur interpreter diatur lewat `indonesiaku.jalurIndk`.

**Fase 5 — Iterator & generator.** Seluruh item Fase 5 di `docs/ROADMAP.md`
selesai.

### Ditambahkan

- Protokol iterator malas: metode ajaib **`_berikutnya_`** (total kini 31).
  Objek dengan `_iter_()` + `_berikutnya_()` bisa dipakai `untuk ... dalam`,
  dan berhenti dengan `naikkan HentiIterasi()`.
- **`hasilkan`** (yield): fungsi yang memakainya menjadi generator dengan
  frame yang ditangguhkan dan dilanjutkan saat nilai berikutnya diminta.
- **Generator expression**: `(x * 2 untuk x dalam s jika syarat)`, termasuk
  bentuk tanpa kurung sendiri sebagai argumen tunggal: `jumlah(x untuk x dalam s)`.
- **Comprehension kamus dan himpunan**: `{k: v untuk ...}` dan `{x untuk ...}`.
- **`cocokkan` / `kasus`** (pattern matching) dengan pola literal, `_`,
  pengikatan nama, `Kelas()`, alternatif `atau`, dan penjaga `jika`.
- `iter()` dan `lanjutkan()` kini menerima generator dan iterator kustom.
- `daftar`, `tupel`, `himpunan`, `himpunan_beku`, `jumlah`, `semua`,
  `ada_yang`, `terurut`, `terbalik`, `peta`, dan `saring` menerima generator
  maupun iterator kustom.

### Diperbaiki

- ⚠️ **Bitmask metode ajaib meluap.** Setelah `_berikutnya_` ditambahkan,
  jumlah metode ajaib melewati 32 sehingga `MAGIC_BIT` menggeser di luar
  jangkauan `uint32_t`. Akibatnya pencarian `_keluar_` (blok `dengan`) menjadi
  *undefined behavior*. Bitmask kini 64-bit dan dijaga `_Static_assert`.
  Ditemukan oleh UBSan.
- Kebocoran GC pada pencairan generator: hasilnya kini ditulis ke slot argumen
  yang sudah di-root, memperbaiki *heap-use-after-free* yang terdeteksi ASan.

### Kata kunci baru

`hasilkan`, `cocokkan`, `kasus`. Program yang memakai ketiganya sebagai nama
variabel perlu diganti namanya.

**Fase 6 — Pustaka standar luas (gelombang B & C).**

### Added
- ✨ **`opsi`** — padanan `argparse`: `PenguraiArgumen`, argumen posisi dan
  opsi, `pendek`, `--x=nilai`, `-j3`, bendera digabung (`-vq`), aksi
  `simpan`/`benar`/`salah`/`hitung`/`tambahkan`, `jenis_nilai`, `pilihan`,
  bantuan otomatis bergaya argparse 3.13+. Galat pemakaian → stderr + kode
  2, `-h` → kode 0; `keluar_saat_galat = salah` untuk `GalatOpsi`. Diuji
  1.600 daftar argumen acak berbanding argparse (1 beda: `--` di ujung).
- ✨ **`log`** — padanan `logging`: `debug`/`info`/`peringatan`/`galat`/
  `kritis`, `atur` (basicConfig), `ambil_pencatat` bertingkat dengan titik;
  bawaan ke stderr, ambang `PERINGATAN`, format `{level}:{nama}:{pesan}`.
- ✨ **`proses`** — padanan `subprocess.run`: `jalankan`, `keluaran`,
  `GalatProses`; perintah berupa daftar, tanpa shell.
- ✨ Builtin sistem operasi (`src/sistem.c` baru): `keluar`, `tulis_galat`,
  `lingkungan`, `ambil_lingkungan`, `direktori_kini`, `isi_direktori`,
  `buat_direktori`, `hapus_direktori`, `jalur_indk`; serta `atur_atribut`,
  `ambil_atribut`, `punya_atribut` (setattr/getattr/hasattr). Galat sistem
  kini `GalatOS`; kelas baru `GalatIzin`.
- ✨ **`indk --sandbox`** menolak lingkungan, isi direktori, dan menjalankan
  program lain (`GalatIzin`). Playground web memakai mode ini.
- ✨ `jalur` mendapat `ada`, `adalah_berkas`, `adalah_direktori`, `ukuran`,
  `waktu_ubah`, `absolut`.
- ✨ **`pola`** — ekspresi reguler, padanan `re`: `kompilasi`, `cari`,
  `cocok`, `cocok_penuh`, `cari_semua`, `iter_cari`, `ganti` (templat `\1`,
  `\g<nama>`, atau fungsi), `ganti_n`, `pisah`, `lolos`; objek `Cocokan`
  dan grup bernama `(?P<nama>...)`; bendera `ABAIKAN_KAPITAL`, `MULTIBARIS`,
  `TITIK_SEMUA`. Mesin Pike VM di C (`src/pola.c`) dengan waktu linear:
  tidak ada pola ReDoS. Backreference, lookaround, dan kuantifier posesif
  ditolak dengan `GalatPola`; `\d \w \s` bersifat ASCII. Diuji 240.000
  kasus fuzz dan 9.000 keluaran API identik dengan `re` Python.
  Contoh: `examples/pustaka_pola.idk`.
- ✨ **`jalur`** — mengolah jalur berkas sebagai teks, persis `posixpath`:
  `gabung`, `gabung_semua`, `pisah`, `nama_dasar`, `nama_direktori`,
  `pisah_ekstensi`, `ekstensi`, `normalisasi`, `adalah_absolut`,
  `dari_windows`. Contoh: `examples/pustaka_jalur.idk`.
- ✨ Builtin **`md5`, `sha1`, `sha256`, `sha512`** (teks heksa) dan modul
  **`bita`**, **`base64`**, **`csv`** (gelombang B), kini dengan panduan.
- ✨ **Klien HTTP**: `http_ambil(url, metode, kepala, badan)` di
  `pustaka/http.idk` mengembalikan `Respons` (`status`, `alasan`, `kepala`
  huruf kecil, `badan`, `ok()`, `periksa()`), plus `http_urai_url` dan
  `GalatJaringan`. Khusus `http://` (HTTPS butuh TLS, Fase 8). Diuji
  melawan `examples/todo_server.idk` lewat `tests/test_klien_http.sh`.
- ✨ Builtin **`soket_sambung(host, port)`** (koneksi TCP keluar, IPv4/IPv6
  lewat `getaddrinfo`); ditolak di `--sandbox`.
- ✨ Builtin trigonometri **`sin`, `cos`, `tan`, `asin`, `acos`, `atan`,
  `atan2`, `radian`, `derajat`** — memanggil libm C seperti modul
  `math` Python (digit terakhir bisa berbeda 1 ulp antar-platform, sama
  seperti Python sendiri). Contoh: `examples/trigonometri.idk`.
- ✨ **`kura`** — padanan `turtle` yang menyimpan gambar sebagai SVG:
  `maju`, `mundur`, `kiri`, `kanan`, `ke`, `hadap`, `pulang`, `lingkaran`
  (jumlah langkah sama dengan turtle), `angkat_pena`, `turunkan_pena`,
  `warna`, `tebal`, `kecepatan`, `selesai`; kelas `Kura` untuk beberapa
  kura dan `svg_dari`. Contoh: `examples/pustaka_kura.idk`.

### Fixed
- 🐛 **Modul standar tidak bisa diimpor setelah dipasang.** `pustaka/` hanya
  dicari relatif terhadap direktori kerja, jadi `impor pola` gagal dari folder
  lain; installer Windows bahkan tidak memasang `pustaka/`, dan image Docker
  hanya jalan dari `WORKDIR`-nya. Kini `pustaka/` juga dicari relatif
  terhadap interpreter (di sebelah `indk`, `../pustaka`,
  `../share/indonesiaku/pustaka`), termasuk untuk `impor "pustaka/..."`;
  installer memasang `{app}\pustaka`, dan image Docker menaruhnya di
  `/usr/local/share/indonesiaku/pustaka`. Urutan pencarian lama tidak
  berubah. Diuji `tests/test_impor_lokasi.{sh,ps1}` dan dengan membangun
  kedua image Docker.
- 🐛 **`http_response` mengirim `Content-Length` dalam karakter, bukan
  byte**, sehingga badan non-ASCII (mis. `"☕"`) terpotong di curl dan
  peramban. Kini dihitung dalam byte UTF-8.
- 🐛 Perintah `gcc` manual di panduan instalasi tidak memuat `native.c`,
  `db.c`, dan `sqlite3.c` (serta `-lws2_32` di Windows), jadi gagal di tahap
  link. Kini lengkap dan sudah dijalankan di Windows dan Linux.

**Fase 6 — Pustaka standar luas (gelombang A: modul murni Indonesiaku).**
Lima modul baru di `pustaka/`, dipakai lewat `impor <nama>` bernamespace
(mis. `dari koleksi impor Penghitung`). Tidak ada perubahan bahasa: kode
1.1.0 tetap jalan apa adanya (K14). Semuanya ditulis dalam Indonesiaku di
atas builtin dan metode ajaib K19 — nol perubahan `src/`.

### Added
- ✨ **`koleksi`** — `Penghitung` (Counter), `KamusBawaan` (defaultdict),
  `KamusBerurut`, dan `Antrian` (deque dua ujung O(1) teramortisasi, dengan
  `maks_panjang` dan `putar`). Contoh: `examples/pustaka_koleksi.idk`.
- ✨ **`statistik`** — `rata_rata`, `median`, `modus`, `varians`,
  `simpangan_baku` (sampel) beserta varian `_populasi`, dan `kuantil`.
  Galat bertipe `GalatStatistik`. Contoh: `examples/pustaka_statistik.idk`.
- ✨ **`iterasi`** — padanan `itertools` secara *eager* (mengembalikan
  daftar): `hitung_naik`, `ulangi`, `putar_ulang`, `cabang`, `akumulasi`,
  `rangkai`, `saring_dengan`, `ambil_selama`, `lewati_selama`,
  `saring_salah`, `kelompokkan`, `iris`, `berpasangan`, `kelompok_per`,
  `peta_bintang`, `pasangkan_terpanjang`, `produk`, `permutasi`,
  `kombinasi`, `kombinasi_berulang`. Contoh: `examples/pustaka_iterasi.idk`.
- ✨ **`fungsional`** — `lipat` (reduce), `parsial`, `urutkan_dengan`,
  `urutkan_dengan_banding`, dan `banding_ke_kunci` (cmp_to_key).
  Contoh: `examples/pustaka_fungsional.idk`.
- ✨ **`salin`** — `salin` (dangkal) dan `salin_dalam` (rekursif, aman
  siklus). Contoh: `examples/pustaka_salin.idk`.

### Catatan keterbatasan (sengaja, dicatat di rencana)
- `iterasi` mengembalikan **daftar**, bukan iterator malas, sampai generator
  (Fase 5) ada. Karena bahasa belum punya `*args`, `rangkai`/`produk`
  menerima satu daftar urutan dan `parsial` menerima daftar argumen depan.
- `tupel_bernama`, `@cache_lru`, dan `urutan_lengkap` **ditunda** dengan
  alasan yang sama (butuh fungsi variadik atau penyuntikan metode saat
  runtime).
- `daftar()`/`terurut()`/`jumlah()`/`semua()`/`tupel()` belum mengenali objek
  ber-`_iter_`; kelas di modul ini menyediakan `ke_daftar()`/`daftar()`.

## [1.1.0] - 2026-10-27

**Fase 4 — OOP lengkap.** Seluruh item Fase 4 di `docs/ROADMAP.md` selesai:
metode ajaib kini lengkap (30), atribut kelas, `dengan ... sebagai`,
dekorator (`@properti`, `@metode_statis`, `@metode_kelas`, `@abstrak`),
pewarisan berganda + MRO C3, dan kelas abstrak. Tidak ada perubahan yang
merusak kompatibilitas — kode 1.0.0 tetap jalan apa adanya (K14).

Diverifikasi: seluruh 90 contoh era 1.0.0 dijalankan dengan biner 1.1.0,
**89 byte-identik**. Satu-satunya perbedaan adalah teks pesan galat
`panjang()` yang kini ikut menyebut `_panjang_()` — perilakunya sama, hanya
pesannya lebih lengkap. Program yang mencocokkan pesan galat itu secara
harfiah perlu menyesuaikan.

### Added — Fase 4 tahap 2 (K19): protokol objek
- ✨ **`_panjang_`** — `panjang(objek)` memakai metode kelas. Wajib
  mengembalikan `bulat` tidak negatif (`GalatTipe`/`GalatNilai` bila tidak).
- ✨ **`_logika_`** — kebenaran objek untuk `jika`, `selagi`, `tidak`,
  `dan`/`atau`, `semua()`, `ada_yang()`. Tanpa `_logika_`, kebenaran diambil
  dari `_panjang_() != 0`; tanpa keduanya objek tetap selalu benar, jadi
  program lama tidak berubah.
- ✨ **`_ambil_(kunci)` / `_atur_(kunci, nilai)`** — `objek[kunci]` dan
  `objek[kunci] = nilai`. Kunci boleh tipe apa pun, termasuk tupel.
- ✨ **`_hash_`** — objek bisa menjadi kunci kamus / anggota himpunan.
  Melonggarkan aturan K19: `_sama_` **tanpa** `_hash_` tetap tidak dapat
  di-hash (seperti `__hash__ = None` di Python), tetapi menambahkan `_hash_`
  kini mengembalikannya.
- Contoh: [`examples/oop/protokol_objek.idk`](examples/oop/protokol_objek.idk).

### Added — Fase 4 tahap 2b (K19): cerminan aritmetika
- ✨ **`_ktambah_`, `_kkurangi_`, `_kkali_`, `_kbagi_`, `_kbagi_bulat_`,
  `_ksisa_`, `_kpangkat_`** — operator aritmetika kini juga bekerja saat objek
  ada di **kanan**: `2500 + Rupiah(5000)`. Metode cerminan menerima operand
  kiri sebagai argumen; metode langsung di operand kiri tetap menang bila ada.
  Operator gabungan (`+=`, `*=`, ...) ikut otomatis.
- Contoh: [`examples/oop/cerminan_aritmetika.idk`](examples/oop/cerminan_aritmetika.idk).

### Added — Fase 4: kelas abstrak
- ✨ **`@abstrak`** pada metode — kelas yang masih punya metode abstrak belum
  terisi tidak bisa dibuat objeknya; galatnya menyebut metode yang kurang.
- Bukan mekanisme baru: `SPECIAL_*` keempat, menumpang jalur
  `@properti`/`@metode_statis`/`@metode_kelas`.
- Bekerja dengan pewarisan berganda: satu induk menuntut, induk lain mengisi.
  Metode abstrak tidak ikut diratakan, supaya badan kosongnya tidak menang
  atas implementasi nyata dari induk lain.
- Satu metode hanya boleh punya satu dekorator bawaan.
- Contoh: [`examples/oop/kelas_abstrak.idk`](examples/oop/kelas_abstrak.idk).

### Added — Fase 4: pewarisan berganda + MRO C3
- ✨ **`kelas D < B, C { }`** — induk dipisah koma, prioritas kiri. Urutan
  pencarian dihitung dengan linearisasi C3 seperti Python; MRO yang mustahil
  ditolak saat kelasnya dibuat, bukan dipilih diam-diam.
- Metode tetap diratakan ke subkelas, jadi pemanggilan metode tetap satu
  lookup — jalur terpanas VM tidak berubah.
- Atribut kelas, `@properti`/`@metode_statis`/`@metode_kelas`, metode ajaib,
  `adalah_instansi`, dan `turunan_dari` ikut menelusuri MRO.
- `super` tetap menunjuk induk pertama.
- Contoh: [`examples/oop/pewarisan_berganda.idk`](examples/oop/pewarisan_berganda.idk).

### Added — Fase 4: `@properti`, `@metode_statis`, `@metode_kelas`
- ✨ Tiga dekorator bawaan untuk metode. Disimpan sebagai penanda pada kelas
  (satu tabel `special`, bukan tiga) dan diputuskan saat metode didaftarkan,
  karena ketiganya mengubah cara metode **dipanggil**.
- `@properti` dibaca tanpa kurung, hanya lewat objek; `@metode_statis` tanpa
  penerima; `@metode_kelas` menerima kelasnya sebagai `kelas_ini` — dari
  subkelas yang diterima adalah subkelasnya.
- Ketiganya diwariskan dan boleh ditimpa, termasuk dijadikan metode biasa.
- Contoh: [`examples/oop/properti_statis.idk`](examples/oop/properti_statis.idk).

### Added — Fase 4: dekorator
- ✨ **`@dekorator`** sebelum definisi `fungsi` — bertumpuk (diterapkan dari
  yang terdekat), boleh berargumen (`@ulangi(3)`), dijalankan sekali saat
  definisi. Tanpa opcode baru: penerapannya adalah `OP_CALL 1` biasa.
- ⚠️ Dekorator kustom **pada metode** ditolak compiler dengan pesan jelas:
  `diri` implisit di Indonesiaku, jadi fungsi pembungkus tidak membawa
  penerima. `@properti`/`@metode_statis`/`@metode_kelas` menyusul terpisah.
- Contoh: [`examples/dekorator.idk`](examples/dekorator.idk).

### Added — Fase 4: `dengan ... sebagai` (manajer konteks)
- ✨ **`dengan <ekspresi> [sebagai <nama>] { ... }`** — `_masuk_()` sebelum
  blok, `_keluar_()` sesudahnya. Dijamin dipanggil pada jalur normal, galat,
  `kembalikan`, `hentikan`, dan `lanjut`. `TOKEN_DENGAN` sudah dikenali
  scanner sejak lama tetapi belum pernah dipakai compiler.
- `_keluar_()` dipanggil tanpa argumen dan tidak bisa menelan galat —
  penyederhanaan yang disengaja terhadap Python.
- Contoh: [`examples/oop/dengan_sebagai.idk`](examples/oop/dengan_sebagai.idk).

### Added — Fase 4: atribut kelas
- ✨ **Atribut kelas** — `NAMA = ekspresi` di badan kelas. Dibaca lewat kelas
  maupun instance, diwariskan ke subkelas, dievaluasi sekali saat kelas
  didefinisikan. Urutan pencarian mengikuti Python: field instance → atribut
  kelas → metode; menulis lewat instance selalu membuat field instance.
- Contoh: [`examples/oop/atribut_kelas.idk`](examples/oop/atribut_kelas.idk).

### Added — Fase 4 tahap 2d (K19): `_iter_` dan `_panggil_`
- ✨ **`_iter_`** — kelas sendiri bisa dipakai di `untuk ... dalam`. Dipanggil
  sekali di awal loop; kembaliannya (daftar, tupel, teks, rentang, himpunan,
  kamus) yang diiterasi. `hentikan`/`lanjut` tetap bekerja.
- ✨ **`_panggil_`** — objek bisa dipanggil seperti fungsi, lengkap dengan
  nilai bawaan dan argumen bernama, dan bisa dioper ke `peta()`/`saring()`.
- Contoh: [`examples/oop/iter_panggil.idk`](examples/oop/iter_panggil.idk).

### Added — Fase 4 tahap 2c (K19): terurut/min/max atas objek
- ✨ **`terurut()`, `min()`, `max()` menerima objek** yang punya
  `_kurang_dari_` (atau hanya `_lebih_dari_`, lewat cerminan). Pengurutan
  objek **stabil**; daftar asli tidak diubah; argumen `menurun` tetap berlaku.
  Mencampur objek dengan angka/teks tetap `GalatTipe`.
- Contoh: [`examples/oop/urut_objek.idk`](examples/oop/urut_objek.idk).

### Fixed
- 🐛 **Buffer overflow saat sumber dibaca dari pipe** (`indk /dev/stdin`, atau
  `cat program.idk | indk /dev/stdin`). `readFile()` mempercayai `ftell()`;
  pada pipe seek gagal, `ftell()` mengembalikan `-1`, dan `-1` di dalam
  `size_t` membuat `malloc(fileSize + 1)` mengalokasikan nol bita. Kini dibaca
  bertahap. Bug lama sejak Fase 3, lolos karena semua tes memakai berkas biasa;
  regresi permanennya ada di `tests/test_baca_sumber.sh`.
- 🐛 **Use-after-free pada `kamus[kunci]`** saat kunci berupa objek dengan
  `_sama_`: operand dibuang dari stack sebelum pencarian tabel, sehingga GC
  yang terpicu oleh pemanggilan `_sama_` bisa membebaskan kamus dan kuncinya.
  Ditemukan oleh build sanitizer, bukan oleh suite biasa.
- 🐛 **Galat dari metode ajaib tertimpa pesan generik.** `_hash_` yang
  melempar dilaporkan sebagai "kunci kamus harus dapat di-hash", menyembunyikan
  sebab aslinya; kini galat aslinya yang muncul, di kelima jalur (literal
  kamus, `kamus[k]`, `kamus[k] = v`, `dalam`, `buang`).

### Added — Fase 4 (K19)
- ✨ **Metode ajaib** (`examples/oop/metode_ajaib.idk`): `_teks_`, `_repr_`,
  `_sama_`, `_tidak_sama_`, `_kurang_dari_`, `_kurang_sama_`, `_lebih_dari_`,
  `_lebih_sama_`, `_tambah_`, `_kurangi_`, `_kali_`, `_bagi_`, `_bagi_bulat_`,
  `_sisa_`, `_pangkat_`, `_negatif_`. Dicari di kelas (termasuk warisan);
  perbandingan memakai cerminan (`b > a` → `a._kurang_dari_(b)`), `!=`
  memakai kebalikan `_sama_`, `adalah` tetap identitas. `_sama_` juga dipakai
  `dalam` dan `==` antar-daftar; kelas ber-`_sama_` tidak bisa menjadi kunci
  kamus. Operator pada objek tanpa metodenya → `GalatTipe`.

### Fixed — ditemukan saat mengerjakan metode ajaib
- 🐛 `>=`, `<=`, `!=` dikompilasi sebagai kebalikan operator lain, sehingga
  `nan >= 1` bernilai `benar`. Kini opcode sendiri; hasil NaN sama dengan
  Python.
- 🐛 `ke_teks`, f-string, dan `repr` memotong teks di 511 karakter (`format`
  di 1023). Semua jalur cetak kini memakai satu formatter tanpa batas.
- 🐛 `cetak` dan `ke_teks` memakai dua pencetak berbeda: `ke_teks(cetak)`
  memberi `<objek>`, `ke_teks(objek)` memberi `<objek>` padahal `cetak`
  memberi `<objek Kelas>`. Kini selalu sama.
- 🐛 `cetak` daftar yang berisi dirinya sendiri membuat interpreter crash;
  kini dicetak `[1, [...]]` seperti Python.
- 🐛 Galat operand (`-"teks"`, `"a" < 1`, `x / "b"`, ...) dan galat atribut
  (`(5).x`) tidak punya kelas, jadi tidak tertangkap `kecuali GalatTipe` /
  `kecuali GalatAtribut`. Rekursi terlalu dalam kini `GalatRekursi`, dan
  `0 ** -1` kini `GalatBagiNol`.

## [1.0.0] - 2026-10-25

Rilis stabil pertama. Mulai versi ini kebijakan **K14** berlaku: perilaku yang
tertulis di [`docs/SPESIFIKASI.md`](docs/SPESIFIKASI.md) tidak akan berubah
sampai 2.0.

### Changed
- ⚠️ **`jenis()` kini konsisten berbahasa Indonesia**: `"string"` → `"teks"`,
  `"boolean"` → `"logika"`, `"list"` → `"daftar"`, `"object"` → `"objek"`.
  Nama-nama ini kini sama dengan yang dipakai pesan galat. Sengaja dilakukan
  SEBELUM 1.0 karena setelahnya K14 mengunci perilaku sampai 2.0.
  Konsumen di repo (`pustaka/json.idk`) ikut diperbarui.

### Added — Fase 3: modul & namespace
- ✨ **`ObjModule`**: setiap berkas kini punya namespace global sendiri, jadi
  dua modul boleh memakai nama yang sama tanpa saling menimpa.
- ✨ **`impor nama`**, `impor nama sebagai alias`, `dari nama impor x`,
  `dari nama impor x sebagai y`, dan impor banyak nama sekaligus.
- ✨ **Paket**: `impor a.b` memuat `a/b.idk`.
- ✨ **Jalur pencarian modul**: direktori berkas pengimpor → `INDK_PATH` →
  `pustaka/`. Modul di-cache berdasar jalur hasil resolusi, jadi berkas yang
  sama hanya dieksekusi sekali walau diimpor lewat dua nama.
- ✨ **`__nama__` / `__utama__`**: `jika __nama__ == "__utama__" { ... }`
  membuat sebuah berkas bisa menjadi program sekaligus modul.
- ✨ `jenis(modul)` → `"modul"`.

### Fixed
- 🐛 Penggunaan-setelah-bebas saat kunci `__nama__` dibuat tanpa di-*root*
  sebelum `tableSet` yang mengalokasi (ditemukan ASan + `DEBUG_STRESS_GC`).

### Compatibility
- `impor "file.idk"` **tidak berubah**: tetap berbagi namespace dengan
  pemanggil, sehingga seluruh `pustaka/*.idk` dan contoh lama jalan apa adanya.

### Added — Fase 2 tuntas
- ✨ **Tupel tanpa kurung** — `x = 1, 2` dan `kembalikan a, b` membentuk tupel
  tanpa tanda kurung, termasuk `y = 1,` untuk satu elemen. Hanya berlaku di
  sisi kanan penugasan dan setelah `kembalikan`; di dalam kurung mana pun koma
  tetap berarti pemisah, jadi `f(1, 2)`, `[1, 2]`, dan `{"a": 1, "b": 2}` tidak
  berubah arti.
- ✨ **`himpunan_beku([...])`** — himpunan yang tidak dapat diubah dan **dapat
  di-hash**, sehingga boleh menjadi kunci kamus atau anggota himpunan lain
  (`frozenset` Python). Operator `| & - ^` menghasilkan himpunan beku hanya
  bila kedua operand beku; `tambah`/`hapus` menolaknya dengan `GalatTipe`.
  `himpunan([1]) == himpunan_beku([1])` tetap `benar`, seperti Python.
- ✨ **`hash(x)`** — nilai hash sebuah nilai; `GalatTipe` bila tidak dapat
  di-hash.
- ✨ **`iter(urutan)` + `lanjutkan(it[, bawaan])`** — iterator eksplisit.
  Dinamai `lanjutkan` karena `lanjut` sudah menjadi kata kunci perulangan.
  Tanpa `bawaan`, iterator yang habis menaikkan `HentiIterasi` (kelas galat
  yang selama ini terdaftar tetapi belum pernah dipakai).
- ✨ **`turunan_dari(kelas, induk)`** — `issubclass`; sebuah kelas adalah
  turunan dari dirinya sendiri.
- ✨ **`ke_desimal(x)`** — `float()`; dibutuhkan sejak K2 membuat
  `ke_angka("3")` menghasilkan `bulat`.
- ✨ **`daftar([urutan])`** — `list()`; selalu salinan baru, termasuk saat
  argumennya sudah berupa daftar.
- `jenis()` kini membedakan `"himpunan_beku"` dan `"iterator"`.

### Fixed — ekstensi VS Code
- 🐛 35 builtin tidak pernah ter-highlight di tmLanguage (termasuk `rentang`,
  `terurut`, `himpunan`, `tupel`, `repr`, dan seluruh fungsi `db_*`). Daftar
  pola kini dibangkitkan dari `defineNative()` di `src/vm.c`, diurutkan dari
  nama terpanjang supaya `himpunan_beku` tidak kalah oleh `himpunan`.

### Fixed — pesan galat
- 🐛 Galat bertipe yang melewati `coba` tanpa `kecuali` yang cocok (mis.
  `GalatNama` melewati `kecuali GalatTipe`) dulu hanya tercetak
  "Kesalahan tak tertangani." — kini pesan aslinya ditampilkan
  (`tests/galat/galat_bertipe_diteruskan`).
- 🐛 Pengindeksan pada nilai yang tidak berindeks kini `GalatTipe` (dulu kelas
  umum), dengan pesan yang menyebut daftar/tupel/teks/kamus.

### Fixed — compiler hang (ditemukan fuzzer CI)
- 🐛 `akhirnya` bersarang yang berisi `kembalikan`/`hentikan`/`lanjut` membuat
  compiler menyalin blok luar secara eksponensial: kedalaman 6 butuh 24 detik,
  input fuzzer CI menggantung > 5 menit di bawah ASan. Salinan kini dibatasi
  64 per fungsi; melewatinya adalah satu galat kompilasi yang jelas
  (`tests/galat/akhirnya_salinan_berlebih`), tanpa rentetan galat susulan.

### Fixed — penggabungan Fase 1 + Fase 2
- 🐛 List comprehension dengan filter (`[x untuk x dalam a jika syarat]`)
  gagal kompilasi setelah ekspresi kondisional (`a jika c selain b`) masuk:
  `jika` pada filter terbaca sebagai awal ekspresi kondisional. Urutan dan
  filter kini diurai seperti `or_test` di Python; ekspresi kondisional di
  urutan perlu tanda kurung.
- 🐛 Berkas sumber ber-CRLF (checkout git di Windows) membocorkan `\r` ke
  string tiga kutip, `panjang()`, dan potongan baris pada pesan galat. Sumber
  kini dinormalkan saat dibaca (`\r\n` → `\n`), seperti Python.
- 🐛 `pastikan salah, 404` menyimpan pesan `404.0`; pesan `bulat` kini tetap
  `404`.

### Changed — Fase 2 ⚠️ MERUSAK
- 🔢 **Dua tipe angka: `bulat` (int64) dan `desimal` (double)** — K2.
  Literal tanpa titik menjadi `bulat`; hasil aritmetika bulat tetap bulat dan
  **tepat** sampai 2⁶³-1 (`9007199254740993 + 1` benar, dulu salah karena
  double). Pelanggaran jangkauan menaikkan `GalatLuapan` (subkelas
  `GalatAritmetika`, bisa ditangkap) alih-alih berubah diam-diam menjadi
  double; literal yang terlalu besar adalah galat kompilasi. Bignum menyusul
  di Fase 2b.

  Perilaku yang **berubah** (semuanya mengikuti Python 3):
  - `/` selalu `desimal`: `6 / 2` sekarang `3.0`, dulu `3`.
  - `desimal` selalu menampilkan `.0`: `cetak(3.0)` sekarang `3.0`, dulu `3`.
  - `//`, `%`, `**` (pangkat ≥ 0) pada dua `bulat` menghasilkan `bulat`.
  - `1 == 1.0` → `benar`; `1 adalah 1.0` → `salah`.
  - `bulat`, `bulatkan` (tanpa `digit`), `lantai`, `atap`, `abs`, `panjang`,
    `ord`, `ke_bulat`, `ke_angka`, `jumlah` (semua bulat), `biner`/`oktal`/
    `heksa`, `acak_bulat`, `id`, `hash`, elemen `rentang()`, dan indeks
    `enumerasi()` kini menghasilkan `bulat` seperti padanan Python-nya.
  - `bulatkan(x, digit)` tetap `desimal` (seperti `round(x, n)` di Python).
  - `biner`/`oktal`/`heksa` menerima `bulat` 64-bit penuh (batas 2⁵³ hilang).
  - SQLite: kolom `INTEGER` menjadi `bulat`; batas ±2⁵³ untuk `INTEGER` hilang.
  - `pustaka/matematika.idk`: `mat_kpk` memakai `//`, jadi hasilnya `bulat`.

  - Literal `0x`/`0b`/`0o` dan `1_000` (dari Fase 1) juga menjadi `bulat`;
    sebelum K2 semuanya `double`.

  Semua golden diperiksa satu per satu; yang berubah hanya baris yang memang
  berbeda di Python (mis. `akar(16)` → `4.0`, `7.5 // 2` → `3.0`).

### Changed — Fase 2 (K8) ⚠️ MERUSAK
- 🗝️ **Kunci kamus tidak lagi hanya teks.** Teks, `bulat`, `desimal`,
  `benar`/`salah`, dan `kosong` boleh menjadi kunci; daftar/kamus/rentang
  ditolak dengan `GalatTipe`. `1` dan `1.0` adalah kunci yang sama (seperti
  Python). Ini membuat **`kamus[1]` berarti kunci `1`**, bukan "kunci pertama
  menurut urutan penyisipan" — untuk posisi pakai `kunci(kamus)[0]`.
  Iterasi `untuk k dalam kamus` tetap urutan penyisipan (opcode `OP_SEQ_GET`).

### Added — Fase 2
- 🧺 **Himpunan** (K17) lewat `himpunan([urutan])` saja, tanpa literal
  (`examples/himpunan.idk`). Anggota unik dan harus hashable (aturan K8);
  `dalam`, `panjang`, `untuk`, comprehension, `==` tanpa peduli urutan,
  `| & - ^` (himpunan baru, kedua operand harus himpunan), `.tambah`,
  `.hapus` (`GalatKunci` bila tidak ada). Seperti `set` Python, himpunan
  tidak hashable dan tidak berindeks. Dicetak `himpunan([1, 2])`.
- 📦 **Tupel** (K17) `(1, 2)`, `(1,)` (koma wajib), `()`; `(1)` tetap
  pengelompokan (`examples/tupel.idk`). Seperti Python: indeks, irisan
  (hasilnya tupel), unpacking, `untuk`, `dalam`, `+`, `*`, `==` struktural
  (`(1,) == [1]` salah), dan bisa menjadi **kunci kamus** bila elemennya
  hashable. Tidak bisa diubah: `t[i] = x`, `buang t[i]`, `tambah`/`hapus`/
  `sisip`/`hapus_pada` → `GalatTipe`. Builtin `tupel([urutan])`; metode
  hanya yang tidak mengubah. Parameter SQL `db_*` kini boleh tupel.
- 🔣 **Operator bitwise** `&`, `|`, `^`, `~`, `<<`, `>>` dan
  `&= |= ^= <<= >>=` (`examples/operator_bitwise.idk`), presedensi seperti
  Python (di atas perbandingan, di bawah `+ -`). Hanya untuk `bulat`;
  `desimal` → `GalatTipe`, geser negatif → `GalatNilai`, `<<` di luar int64 →
  `GalatLuapan`. `benar & salah` tetap logika seperti `bool` Python; `~benar`
  ditolak (pakai `tidak`). `^` adalah XOR, bukan pangkat.
- 🔁 **List comprehension** `[ekspresi untuk x dalam urutan jika syarat]`
  (`examples/komprehensi.idk`). Satu `untuk` dan satu `jika` opsional; boleh
  bersarang. Variabel loop hanya hidup di dalam comprehension, dan nama luar
  tetap terlihat.
- 🧩 **Penugasan berganda & unpacking** `a, b = 1, 2`, `a, b = b, a`,
  `x, y, z = daftar`, teks per karakter, dan `rentang()`
  (`examples/penugasan_berganda.idk`). Jumlah yang tidak cocok menimbulkan
  `GalatNilai` yang bisa ditangkap. Di dalam fungsi, setiap nama tetap menjadi
  lokal (K3), termasuk yang berada di dalam blok `jika`.
- 🗑️ **`buang` (padanan `del`)** untuk variabel global/lokal, elemen daftar,
  kunci kamus, dan atribut objek (`examples/buang.idk`). Membaca nama lokal
  yang sudah di-buang menimbulkan galat; indeks/kunci/atribut yang tidak ada
  menimbulkan `GalatIndeks`/`GalatKunci`/`GalatAtribut`.

### Added — Fase 1
- ✅ **Kolom dan potongan baris pada galat runtime** — traceback kini
  `[baris N, kolom M]` dengan baris sumber dan caret pada bingkai terdalam.
  Sumber disimpan sebagai objek GC di dalam chunk, sehingga sumber modul
  `impor` tetap tersedia saat galat terjadi.
- ✅ **Teks di dalam koleksi dicetak dengan kutip (`repr`)** — `cetak(["a"])`
  kini `["a"]`, bukan `[a]`. Aturan escape tunggal dipakai bersama oleh
  `cetak`, `ke_teks`, `format`, f-string, dan `repr`.
- ✅ **Pesan galat kompilasi dengan kolom, potongan baris, dan caret** —
  `[baris 2, kolom 11] ...` diikuti baris sumber dan `^` di bawah token.
  Baris panjang dipotong dengan `…`, tab disalin apa adanya agar caret lurus.
- ✅ **Saran "Mungkin maksudmu ...?"** untuk nama global yang salah ketik,
  memakai jarak edit terbatas (≤ 2 dan ≤ sepertiga panjang nama).
- ✅ **`cetak` menjadi fungsi biasa (K12)** — bukan lagi pernyataan/opcode.
  Mendukung `pisah=` dan `akhir=`, serta dapat diperlakukan sebagai nilai
  (disimpan, dioper, `bisa_dipanggil(cetak)`). `OP_PRINT`, `TOKEN_CETAK`, dan
  `printStatement()` dihapus. Konsekuensi seperti Python: `cetak` kini nama
  biasa yang bisa ditimpa.
- ✅ **Metode pada tipe bawaan (K10)** — `"budi".huruf_besar()`,
  `[3,1,2].terurut()`, `{"a":1}.punya("a")`, dan dapat dirangkai.
  Setiap metode memanggil native yang sudah ada (semuanya sudah menerima
  penerima sebagai argumen pertama), jadi tidak ada logika teks/daftar/kamus
  yang ditulis ulang dan fungsi bebas lama tetap berfungsi.
  Metode bawaan harus langsung dipanggil; menyimpannya sebagai nilai ditolak
  dengan pesan yang menjelaskan caranya.
- ✅ **Literal angka baru** — eksponen (`1e3`, `1.5e-3`), heksadesimal
  (`0xFF`), biner (`0b1010`), oktal (`0o17`), dan pemisah ribuan
  (`1_000_000`). Garis bawah wajib di antara digit; `1__0` dan `0b2` ditolak
  dengan pesan sendiri. Semuanya tetap `double` — bilangan bulat presisi
  penuh masih menunggu Fase 2.
- ✅ **String tiga kutip** — `"""..."""` mempertahankan baris baru apa adanya.
- ✅ **f-string** — `f"Halo {nama}"`. Didesugar menjadi `+` dan `ke_teks()`
  yang sudah ada, jadi tanpa opcode baru. Mendukung ekspresi apa pun,
  `{{`/`}}` harfiah, serta kutip di dalam `{ }` (`f"{d["a"]}"`).
  Belum ada format spec (`{x:.2f}`) — menunggu metode tipe teks.
- ✅ **Fungsi anonim** — `fungsi(x) { kembalikan x * x }` sebagai ekspresi,
  closure penuh, mendukung nilai bawaan + argumen bernama. Badannya tetap
  `{ }` dan `kembalikan`; tidak ada bentuk satu-ekspresi gaya `lambda` supaya
  cara menulis fungsi tetap satu. Di traceback muncul sebagai `<anonim>`.
- ✅ **Ekspresi kondisional** — `"lulus" jika nilai >= 60 selain "gagal"`.
  Kondisi dinilai lebih dulu dan hanya cabang terpilih yang berjalan, jadi
  `x[0] jika panjang(x) > 0 selain "kosong"` aman pada daftar kosong.
- ✅ **`pastikan` (assert)** — `pastikan kondisi` atau
  `pastikan kondisi, "pesan"`. Gagal menaikkan `GalatPenegasan`, kelas yang
  sudah terdaftar di hierarki galat sejak Fase 1 tetapi belum pernah dipakai.
  Seperti Python, **ekspresi pesan hanya dinilai saat penegasannya gagal**,
  sehingga `pastikan benar, hitung_mahal()` tidak memanggil apa pun.
  Kebenaran kondisinya memakai aturan `isFalsey` yang sama dengan `jika`.

  Tidak ada mode `-O` untuk mematikan penegasan: belum ada satu pun flag
  runtime di `indk`, dan menambahnya demi fitur ini tidak sepadan.

### Added — Fase 3
- 🗄️ **Basis data SQLite tertanam** — 10 fungsi bawaan (`db_buka`,
  `db_eksekusi`, `db_ambil_semua`, `db_ambil_satu`, `db_baris_terpengaruh`,
  `db_id_terakhir`, `db_mulai`, `db_komit`, `db_batal`, `db_tutup`).
  SQLite **3.50.4** di-*vendor* sebagai amalgamation resmi (public domain,
  SHA256 tercatat di `src/SQLITE_PROVENANCE.md`), jadi `make` tetap satu
  langkah tanpa dependensi dan tanpa menunggu FFI. Handle `OBJ_DB` menutup
  diri saat GC seperti `OBJ_FILE`/`OBJ_SOCKET`; `db_buka(":memory:")` bekerja
  di lingkungan read-only seperti playground.

  Keamanan bukan tambahan belakangan: parameter `?` ada sejak versi pertama,
  satu panggilan hanya menerima **satu** pernyataan — ditolak *sebelum*
  pernyataan pertama dijalankan, sehingga `"INSERT ...; DROP TABLE ..."`
  tidak pernah mengubah apa pun — jumlah `?` wajib sama dengan panjang
  parameter, dan hanya `?` anonim yang diterima. `BLOB` serta `INTEGER` di
  luar jangkauan tepat ±2⁵³ **bergalat** daripada merusak data diam-diam.
  Ukuran biner naik ~140 KB → ~1,2 MB.

### Fixed
- 🐛 **Use-after-free saat mengompilasi f-string** — `restoreScanner` tidak
  mengembalikan penunjuk sumber, sehingga ia tertinggal menunjuk buffer
  sementara f-string yang sudah dibebaskan. Tidak terlihat sebelumnya karena
  penunjuk itu belum pernah dibaca setelah restore.
- 🐛 **Kamus dicetak sesuai urutan penambahan**, bukan urutan tabel hash.
  Sebelumnya `cetak(d)` bisa berbeda urutan dari `kunci(d)` dan dari
  perulangan `untuk ... dalam d` pada kamus yang sama.
- 🐛 **Kompilator menggantung selamanya pada `}` yang tidak punya pasangan.**
  Berkas yang hanya berisi `}` pun membuat `indk` berputar tanpa henti
  (exit 124) alih-alih melaporkan galat. `declaration()` sengaja kembali
  tanpa mengonsumsi saat bertemu `}` (itulah cara `block()` tahu blok sudah
  habis), tetapi di tingkat atas tidak ada blok yang ditutup sehingga
  perulangan tidak pernah maju. Sekarang menghasilkan
  `'}' tidak punya '{' yang cocok.` dengan exit 65.

### Added — Fase 1
- ✨ **Beberapa klausa `kecuali` untuk satu `coba`**, dicoba berurutan seperti
  Python: `} kecuali GalatAritmetika sebagai e { … } kecuali GalatPencarian
  sebagai e { … }`. Sebelumnya hanya satu klausa yang dikompilasi dan klausa
  kedua memicu galat parser (lalu menggantung). Bila tidak ada yang cocok,
  galat dilempar ulang ke `coba` di luarnya. Berfungsi juga bersama
  `akhirnya` dan bentuk tuple `kecuali (A, B) sebagai e`. `kecuali` tanpa
  tipe menangkap semuanya sehingga wajib menjadi klausa terakhir.
- ✨ **19 fungsi bawaan baru (padanan builtin Python):**
  - Agregat & urutan: `jumlah()` (sum), `semua()` (all), `ada_yang()` (any),
    `terurut()` (sorted), `terbalik()` (reversed), `pasangkan()` (zip),
    `peta()` (map), `saring()` (filter). `peta`/`saring`/`pasangkan`/`terbalik`
    mengembalikan daftar, belum iterator malas.
  - Angka: `bulatkan()` (round, *banker's rounding*), `bagi_sisa()` (divmod),
    `biner()` / `oktal()` / `heksa()` (bin/oct/hex, int64).
  - Konversi & introspeksi: `ke_bulat()` (int), `ke_logika()` (bool),
    `adalah_instansi()` (isinstance), `bisa_dipanggil()` (callable),
    `repr()`, `id()`.
  - **Catatan:** `bulat` yang lama tetap ada dan sengaja berbeda dari
    `round()` Python; `bulatkan` adalah padanan `round()` yang benar.
- ✨ **`max()` / `min()` menerima satu argumen daftar:** `max([3, 1, 2])`
  selain bentuk dua argumen yang sudah ada.
- ✨ **Argumen bernama:** `gambar("Judul", tengah=benar)` mengirim nilai
  berdasarkan nama parameter, sehingga parameter di tengah boleh dilewati dan
  urutannya bebas. Berlaku untuk fungsi, `init`, metode, dan `super`. Argumen
  posisional wajib mendahului yang bernama; nama tak dikenal, parameter yang
  diisi dua kali, dan parameter wajib yang kosong menaikkan `GalatTipe` yang
  menyebut nama parameternya. Fungsi bawaan belum menerimanya.
- ✨ **Parameter dengan nilai bawaan:** `fungsi sapa(nama, salam="Halo") { }`,
  juga untuk `init` dan metode kelas. Nilai bawaan boleh ekspresi apa pun dan
  boleh merujuk parameter di kirinya (`fungsi f(a, b=a)`). Parameter wajib
  setelah parameter berdefault ditolak saat kompilasi. **Berbeda dari Python
  (K15):** nilai bawaan dievaluasi setiap kali fungsi dipanggil, jadi
  `fungsi f(x, daftar=[])` tidak pernah membawa isi panggilan sebelumnya.
- ✨ **Gabung dan ulang untuk teks & daftar:** `[1, 2] + [3]` menggabungkan dua
  daftar (hasil selalu objek baru); `[0] * 3`, `3 * [0]`, `"-" * 20`, dan
  `2 * "ab"` mengulang. Pengali harus bilangan bulat — nol/negatif memberi
  hasil kosong seperti Python, pecahan → `GalatTipe`. Hasil dibatasi 65535
  elemen/karakter → `GalatNilai`.
- ✨ **`rentang()`** (range): `rentang(akhir)`, `rentang(awal, akhir)`,
  `rentang(awal, akhir, langkah)`. Objek rentang malas (tidak membuat list):
  bisa diiterasi `untuk`, diukur `panjang()`, diindeks `r[i]`, dan dicetak
  `rentang(0, 5)`. Argumen bukan bilangan bulat atau langkah nol → galat.
- ✨ **Operator `dalam` / `tidak dalam`** (in / not in) untuk list (elemen),
  teks (substring), kamus (kunci), dan rentang; presedensi sama dengan
  perbandingan.
- ✨ `untuk huruf dalam "teks"` mengiterasi per karakter.
- ✨ **Rantai perbandingan dan identitas:** `1 < x < 3`, `a dalam b == c`,
  `adalah`, dan `bukan adalah`. Operand tengah dievaluasi sekali dan rantai
  berhenti saat perbandingan pertama salah.
- ✨ **`enumerasi()` dan kamus iterable:** `enumerasi(urutan, awal=0)` membuat
  pasangan `[indeks, nilai]`; `untuk k dalam kamus` mengiterasi kunci menurut
  urutan penyisipan.

- ✨ **Hierarki `Galat*` dan handler bertipe:** galat runtime/native sekarang berupa
  instansi galat dengan `.pesan`; `kecuali GalatTipe sebagai e` dan
  `kecuali (GalatTipe, GalatNilai) sebagai e` didukung. `kecuali e` lama tetap
  kompatibel dan mencetak pesannya.

- ✨ **Akhir baris sebagai pemisah pernyataan:** newline di luar `()`/`[]` kini
  mengakhiri pernyataan; newline di dalam keduanya boleh untuk pemanggilan dan
  list multiline. Titik koma tetap dapat memisahkan pernyataan pada satu baris.

- ✨ **Literal kamus, indeks negatif, dan irisan:** `{"nama": "Budi"}` (kunci
  harus teks, urutan penyisipan dipertahankan, koma akhir & multi-baris boleh),
  `a[-1]` untuk daftar/teks/rentang termasuk pada penugasan, serta irisan
  `a[awal:akhir:langkah]` untuk daftar dan teks dengan aturan Python (batas
  di-*clamp*, langkah negatif membalik, `a[:]` menyalin). `{` di posisi ekspresi
  adalah literal kamus; di posisi pernyataan tetap blok.

### Changed — Fase 1 (merusak kompatibilitas)
- ⚠️ **`==` membandingkan isi, bukan identitas (K7).** `[1, 2] == [1, 2]`,
  `{"a": 1} == {"a": 1}`, dan `rentang(3) == rentang(3)` kini bernilai
  **benar**; perbandingan daftar/kamus bersifat rekursif dan urutan penyisipan
  kamus tidak berpengaruh. Operator `dalam` ikut berubah, jadi
  `[1, 2] dalam [[1, 2]]` kini benar. Untuk membandingkan **objek yang sama**
  gunakan `adalah` / `bukan adalah` — keduanya tetap identitas. Instansi kelas,
  fungsi, berkas, dan soket tetap dibandingkan berdasarkan identitas.
- ⚠️ **Assignment di dalam fungsi membuat variabel lokal (K3)**, seperti Python.
  Dulu `x = 5` di dalam fungsi menulis ke global. Untuk menulis ke luar fungsi,
  deklarasikan `global nama` (variabel tingkat skrip) atau `nonlokal nama`
  (variabel fungsi pembungkus); keduanya menerima beberapa nama sekaligus dan
  hanya boleh di dalam fungsi. Variabel hasil assignment di dalam blok
  `jika`/`selagi`/`untuk` hidup sampai akhir fungsi. `variabel x = ...` tetap sah.
  Closure yang **mengubah** variabel tangkapannya kini wajib `nonlokal`.
- ⚠️ **Truthiness mengikuti Python (K1)**: `0`, `""`, `[]`, kamus kosong, dan
  rentang kosong kini bernilai **salah** dalam `jika`/`selagi`/`dan`/`atau`/
  `tidak` (dulu hanya `salah` dan `kosong`). `selagi antrian { ... }` dan
  `nama atau "bawaan"` kini bekerja seperti di Python.
- ⚠️ `tidak` yang muncul tepat setelah sebuah ekspresi (di baris berikutnya)
  kini dibaca sebagai awal `tidak dalam` (lihat B16).

### Fixed — CI
- 🐛 Build gagal di GCC 14 (Linux CI): `usleep` tidak dideklarasikan di bawah
  `-std=c11`; `native.c` kini mendefinisikan `_DEFAULT_SOURCE`.
- 🐛 Tes `http_pakai` gagal di Linux/macOS: keluarannya memuat `\r` mentah
  sehingga git menyimpan golden-nya sebagai biner berisi CRLF Windows. Contoh
  kini menampilkan `\r` sebagai teks.
- 🐛 Ditemukan fuzzer (ASan): blok `akhirnya` yang berisi `coba ... akhirnya`
  sendiri menimpa data blok luar saat dikompilasi ulang inline, sehingga
  `kembalikan` berikutnya menjalankan blok yang salah atau membaca memori basi.

### Fixed — Fase 0 (stabilisasi)
- 🐛 **`//` (pembagian bulat) rusak**: compiler tidak menghasilkan opcode,
  sehingga `1 + 10 // 3` menjadi `13`. Kini `OP_FLOOR_DIVIDE` membulatkan ke
  bawah seperti Python (`-7 // 2 == -4`). `//=` juga ditambahkan.
- 🐛 **Argumen bernama diam-diam salah**: `f(b=1, a=5)` dulu dikompilasi sebagai
  assignment global dan nilainya dikirim berurutan (`-4`). Kini ditolak saat
  kompilasi dengan pesan "Argumen bernama (nama=nilai) belum didukung".
- 🐛 **Batas rekursi** naik dari 64 menjadi 1000 panggilan (sama dengan Python).
  Jejak error merangkum frame yang berulang
  (`[baris sebelumnya diulang N kali lagi]`).
- 🐛 `**` kini asosiatif kanan (`2 ** 3 ** 2 == 512`) dan lebih kuat dari minus
  unary (`-2 ** 2 == -4`).
- 🐛 `%` mengikuti tanda pembagi seperti Python (`-7 % 3 == 2`).
- 🐛 Penugasan majemuk memakai seluruh ekspresi di sisi kanan: `x *= 2 + 3`
  dulu hanya mengalikan dengan `2`.
- 🐛 `tidak` kini lebih lemah dari perbandingan: `tidak 1 == 2` bernilai `benar`
  (dulu `(tidak 1) == 2`).
- 🐛 **Semua galat runtime bisa ditangkap** `coba/kecuali`: indeks di luar
  jangkauan, variabel tak terdefinisi, tipe operand salah, memanggil
  bukan-fungsi, rekursi terlalu dalam, dll. Nilai `e` adalah pesan galatnya.
  Hanya kerusakan internal VM yang tetap fatal.
- 🐛 `kembalikan`/`hentikan`/`lanjut` dari dalam `coba` kini melepas handler-nya
  (dulu handler basi bisa "menangkap" galat setelah fungsinya selesai).
- 🐛 **Bagi nol** (`/`, `//`, `%`, `0 ** -n`) kini galat "Pembagian dengan nol."
  (dulu menghasilkan `takhingga`/`nan`).
- 🐛 **Presisi cetak angka**: representasi terpendek yang tepat, seperti Python
  (`0.1 + 0.2` → `0.30000000000000004`, `2 ** 0.5` → `1.4142135623730951`).
- 🐛 **Teks Unicode**: `panjang`, `teks[i]`, `karakter`, dan `potong` menghitung
  karakter, bukan byte (`panjang("héllo") == 5`). `ord`/`chr` memakai kode
  Unicode (`ord("é") == 233`, `chr(128512)` → 😀).
- 🐛 Kata kunci `akhirnya` tidak pernah dikenali scanner (salah hitung panjang),
  sehingga terbaca sebagai nama variabel.

### Added — Fase 0
- ✨ `lewati` (pass): pernyataan kosong, termasuk `kelas Kosong { lewati }`.
- ✨ `akhirnya` (finally): `coba { } kecuali e { } akhirnya { }` dan
  `coba { } akhirnya { }`; blok akhirnya selalu jalan — termasuk saat
  `kembalikan`/`hentikan`/`lanjut` keluar dari `coba` — lalu galat yang belum
  ditangkap diteruskan. Nilai `kembalikan` dihitung sebelum akhirnya berjalan.
- ✨ `;` sebagai pemisah pernyataan dalam satu baris (`x = 1; cetak(x)`), dan
  `kembalikan` tanpa nilai sebelum `}` (`jika x { kembalikan }`).

### Removed — Fase 0
- 🧹 Token mati `INDENT`/`DEDENT`/`NEWLINE` dari scanner (sisa desain lama
  berbasis indentasi; tidak pernah dihasilkan).

### Changed — Fase 0 (merusak kompatibilitas)
- ⚠️ `cetak(a, b, c)` kini mencetak semua argumen dalam **satu baris** dipisah
  spasi (seperti Python `print`); dulu tiap argumen di baris sendiri.
  `cetak()` mencetak baris kosong.
- ⚠️ `chr()` menerima kode Unicode hingga `0x10FFFF` dan menghasilkan UTF-8;
  `chr(200)` kini `È`, bukan satu byte mentah.

### Changed
- 📁 **Pemisahan dokumentasi**: isi `docs/` dipindah ke `website/` — situs statis
  di `website/`, panduan pengguna di `website/panduan/`. Folder `docs/` kini
  khusus **perencanaan bahasa**.
- 🌐 Situs tidak lagi memakai Netlify (`netlify.toml` + `.netlify/` dihapus);
  akan di-host sendiri sebagai satu bundel Docker bersama playground.

### Added
- 🧪 **Tes galat** `tests/galat/*.idk` + `*.err`: menguji program yang *harus*
  gagal (exit ≠ 0 dan stderr persis sama). Dijalankan oleh kedua runner.
- 🧪 Tes golden `operator_aritmetika`, `rekursi_dalam`, `coba_akhirnya`,
  `galat_tertangkap`, `teks_unicode`, `cetak_angka`, dan tes galat
  `handler_basi`, `bagi_nol`, `akhirnya_teruskan`; golden `coba_akhirnya_lompat`
  dan `titik_koma`.
- 📚 `docs/ANALISIS/`: audit v0.4.0, peta fitur Python → Indonesiaku, keputusan
  desain, teknik lanjutan, dan katalog fungsi/kelas yang akan diduplikasi.
  `docs/ROADMAP.md` disusun ulang menjadi Fase 0–8 menuju 1.0.
- ⌨️ **stdin di playground** — panel Input terpisah di halaman playground;
  `POST /api/jalankan` menerima `{kode, input}` dan menyalurkan `input` sebagai
  stdin, sehingga `input("Nama: ")` membaca satu baris per pemanggilan.
- ⚙️ **API playground** — `docker/playground/server.py` menjalankan `indk` untuk
  `POST /api/jalankan` (`{kode, input}` → `{stdout, stderr, kode_keluar, ms}`)
  dengan batas 3 detik, 128 MB, 64 KB input/output, dan rate limit 60/menit per
  IP.
- 🐳 **Bundel Docker situs + playground** — satu image (nginx + `indk` + python),
  dijalankan sebagai dua layanan: `web` (publik) dan `runner` (jaringan
  internal, tanpa akses internet, read-only, non-root).
- 🎨 **Desain ulang situs** — Tailwind CSS (Play CDN) + token di
  `website/assets/tailwind-config.js`, Alpine.js untuk tema/bahasa, tipografi
  Plus Jakarta Sans + JetBrains Mono, mode terang & gelap berbasis kelas.
- 🌐 **Dua bahasa (ID/EN)** — semua teks UI di `website/assets/i18n.json`;
  pemilih bahasa tersimpan di `localStorage`, halaman Tentang & Penulis punya
  artikel penuh dalam dua bahasa.
- ⚡ **Senna.js (SPA)** — navigasi antar halaman tanpa reload lewat surface
  `#konten`, sementara setiap halaman tetap HTML utuh dengan URL sendiri
  (ramah mesin pencari). Tombol back/forward tetap bekerja.
- 🧪 **Halaman playground** — `website/playground/index.html`: editor kode +
  panel keluaran, `POST` AJAX ke `/api/jalankan` (`{ kode }` →
  `{ stdout, stderr, kode_keluar, ms }`), pintasan Ctrl+Enter. Backend API
  menyusul (halaman menampilkan status "backend tidak tersedia" bila belum ada).
- 🗺️ `docs/ROADMAP.md` sebagai satu-satunya sumber kebenaran roadmap, plus
  `docs/README.md` dan `docs/RENCANA_FITUR/TEMPLATE.md` untuk rencana per fitur.

### Fixed
- 🔗 Tautan antar-dokumen yang masih menunjuk `docs/…` (termasuk nama berkas
  yang salah sejak lama, mis. `02_PANDUAN_SINTAKS.md` → `03_PANDUAN_SINTAKS.md`).

## [0.4.0] - 2026-08-07

Rilis JSON + REST: aplikasi contoh Todo-list dalam dua bentuk — CRUD CLI
dan REST API HTTP (soket TCP), keduanya diuji di CI.

### Added
- 🌐 **Soket TCP** — tipe objek `OBJ_SOCKET` + native `soket_dengar(port)`,
  `soket_terima`, `soket_baca`, `soket_tulis`, `soket_tutup` (Winsock di
  Windows, BSD di POSIX; listen di 127.0.0.1). Auto-close saat GC.
- 🔌 **`pustaka/http.idk`** — `http_urai_request` (parse request HTTP),
  `http_response`/`http_response_json` (bangun response).
- 🚀 **Server REST HTTP** — `examples/todo_server.idk`: REST API Todo-list
  (`GET/POST/PUT/DELETE /todos`) dengan penyimpanan JSON. Diuji CI (job
  `todo-http` + `tests/test_todo_server.{sh,ps1}`).
- 📇 **JSON** — `pustaka/json.idk` (`json_teks`, `json_urai`), ditulis dalam
  Indonesiaku sendiri.
- ✅ **Todo-list JSON DB (CLI)** — `examples/todo_cli.idk` (CRUD command-line).
  Diuji CI (job `todo-crud`).
- 🔡 **Native tambahan** — `ord`, `chr`, `hapus_pada(list, i)`,
  `sisip(list, i, nilai)`.
- 🧪 **Uji sintaks dasar** — `examples/sintaks_dasar.idk` (smoke test golden).

### Fixed
- 🐛 **Regresi assignment implisit di dalam blok** — `x = 10` di dalam
  `jika`/`selagi`/`untuk` (setelah scoping v0.3.0). Kini memakai semantik
  definisi di kedalaman scope mana pun.
- 🐛 **Korupsi nilai kamus saat GC** — `OP_INDEX_SET` dict mem-pop operand
  sebelum `tableSet`; kini ditahan di stack. Ditemukan lewat parser JSON di
  bawah stress-GC.

### Changed
- `jenis()` mengembalikan `"fungsi"` untuk fungsi/closure/native/metode
  terikat, dan `"soket"` untuk soket.

## [0.3.0] - 2026-08-07

Rilis pustaka standar: fungsi bawaan untuk berkas, waktu, konversi, plus
pustaka teks/daftar/matematika/kamus yang ditulis dalam bahasa Indonesiaku
sendiri (dogfooding).

### Added
- 📚 **Pustaka standar dalam bahasa sendiri** (`pustaka/`, di-`impor`):
  `teks.idk` (pisah/gabung/cari/pangkas/balik/...), `daftar.idk`
  (peta/saring/lipat/urutkan/iris/...), `matematika.idk`
  (faktorial/fpb/kpk/prima/rata_rata/...), `kamus.idk`
  (nilai/gabung/dari_pasangan/...).
- 🔡 **Primitif & pengindeksan teks** — `karakter(teks, i)`,
  `potong(teks, mulai, jumlah)`, dan pengindeksan `teks[i]` (baca).
- 🧯 **Error native yang dapat ditangkap** — fungsi bawaan memicu kesalahan
  via `nativeRaise` yang bisa ditangkap `coba/kecuali` (dulu semua fatal).
- 🔤 **Konversi & format** — `ke_angka`, `ke_teks`, `format("... {} ...", ...)`,
  `acak_bulat(min, maks)`. `acak()` kini di-seed.
- 📄 **Berkas (File I/O)** — `buka_berkas`, `tutup`, `baca`, `baca_baris`,
  `tulis`, `baca_semua`, `tulis_semua`, `tambah_berkas`, `ada_berkas`,
  `hapus_berkas`. Tipe objek baru `OBJ_FILE` (auto-close saat GC).
- 🕐 **Waktu & Tanggal** — `waktu()`, `tanggal()` (kamus komponen),
  `format_tanggal(pola)`.
- 🧵 **Urutan escape string** — `\n \t \r \" \\ \'` kini diproses di literal
  string (scanner + compiler).
- ⌨️ **Argumen baris perintah** — variabel global `argumen` (list string);
  `indk skrip.idk arg1 arg2 ...`.
- 🖥️ **REPL multi-baris** — blok (`fungsi`/`kelas`/`jika`/...) dapat ditulis
  lintas baris; prompt lanjutan `...` sampai kurung kurawal seimbang.
- 📖 **Referensi pustaka standar** terstruktur di `docs/07_PUSTAKA_STANDAR.md`.

### Fixed
- 🐛 **Variabel bocor dari badan blok**: `variabel` yang dideklarasikan di
  dalam `jika`/`selagi`/`untuk` tidak di-scope, sehingga bocor ke variabel
  berikutnya (terutama saat `hentikan`). Kini setiap badan blok memakai
  scope tersendiri. Regressi dijaga oleh `examples/lingkup_variabel.idk`.

### Changed
- `<` dan `>` kini juga membandingkan string (via `strcmp`), memungkinkan
  pengurutan string.
- Pencetakan angka lebih presisi (15 digit, tanpa notasi ilmiah tak sengaja,
  integer tanpa `.0`).

## [0.2.0] - 2026-08-07

Rilis fitur besar: bahasa kini mencakup closure, struktur data, OOP penuh,
penanganan kesalahan, dan sistem modul — semua dengan garbage collector
mark-sweep dan cakupan pengujian golden-output.

### Added
- 🧠 **Closures & upvalues** — fungsi bersarang menangkap variabel lokal
  (gunakan `variabel` untuk lokal yang di-capture); upvalue tertutup persist
  setelah fungsi induk kembali.
- 📚 **List** — literal `[..]`, pengindeksan baca/tulis, list bersarang;
  builtin `panjang`, `tambah`, `hapus`.
- 🔑 **Kamus (dictionary)** — `kamus()`, pengindeksan kunci string; builtin
  `punya`, `kunci`, `hapus_kunci`.
- 🔁 **`untuk ... dalam`** — iterasi list (for-in).
- ⏭️ **`hentikan` / `lanjut`** — break & continue di `selagi` dan `untuk`.
- 🏛️ **Kelas & OOP** — `kelas`, `init` (konstruktor), `diri` (this), metode,
  field, pewarisan (`kelas Anak < Induk`), `super`, polimorfisme.
- 🧯 **Penanganan kesalahan** — `naikkan` (raise) dan `coba { } kecuali e { }`
  (try/catch) dengan unwinding call-stack dan nesting.
- 📦 **Sistem modul** — `impor "file.idk"` memuat file lain ke namespace global
  dengan cache anti-duplikasi.
- ♻️ **Garbage Collector mark-sweep** — koleksi otomatis berbasis ambang
  alokasi; flag `DEBUG_STRESS_GC` / `DEBUG_LOG_GC`.
- 🧰 Builtin baru: `abs`, `pangkat`, `bulat`, `lantai`, `atap`, `acak`,
  `huruf_besar`, `huruf_kecil`, `ganti`, `jenis`, `input`, `henti`, dan builtin
  list/kamus di atas.

### Fixed
- 🐛 **Build link error**: `src/native.c` tidak ada di `Makefile SOURCES`
  (dan one-liner gcc README) — menyebabkan `undefined reference` saat link.
- 🐛 **Stack underflow** pada setiap `jika`/`selagi`: `OP_JUMP_IF_FALSE`
  melakukan pop ganda. Kini tidak pop (sesuai desain clox).
- 🐛 **Operator `atau` (OR) rusak total** — salah kelas huruf di scanner
  (`case 'o'` alih-alih `'a'`) → "Variabel tidak terdefinisi 'atau'".
- 🐛 **Propagasi error**: `push`/`pop` yang gagal kini menghentikan eksekusi
  (dulu lanjut dengan stack korup).
- 🐛 **`OP_DEFINE_GLOBAL` tidak pop** → deklarasi meninggalkan nilai di stack,
  merusak slot lokal `untuk...dalam`. Deklarasi kini stack-neutral.
- 🐛 **`make clean` gagal di Windows** (`del /Q src/main.o` — `/` diartikan
  switch). Diperbaiki dengan path backslash.
- 🐛 **Makefile tanpa dependensi header** → perubahan `.h` tidak memicu
  rekompilasi (miscompile senyap). Kini `%.o: %.c $(HEADERS)`.
- 🐛 Hardening fungsi native: validasi tipe semua argumen sebelum konversi.

### Changed
- 🔒 CI (`test.yml`) tidak lagi `continue-on-error` pada langkah build & test;
  ditambah job **ASan/UBSan** (stress GC), **Valgrind**, dan **coverage (lcov)**.
- ✅ Suite pengujian beralih ke **golden-output** (bandingkan stdout + cek
  stderr/exit-code), bukan hanya exit-code. Ditambah benchmark performa.
- 📖 Dokumentasi sintaks diselaraskan ke sintaks kurung kurawal `{ }` yang
  sebenarnya di-parse (README sebelumnya salah mengajarkan gaya Python).
- `cetak` bekerja untuk semua tipe baru (list, kamus, objek).

## [0.1.1] - 2025-10-28

### Changed

- Bumped version to `0.1.1`.
- Added CLI flags `-v/--version` and `-h/--help`.
- Disabled compiler debug bytecode printing by default for release builds.
- Updated installer metadata and documentation to reference `v0.1.1`.

## [0.1.0] - 2025-10-27

### Added
- ✨ **Fitur Inti**
  - Scanner/Lexer dengan kata kunci Bahasa Indonesia
  - Parser dengan Pratt parsing untuk ekspresi
  - Compiler bytecode dari AST
  - Virtual Machine stack-based untuk eksekusi
  - REPL (Read-Eval-Print Loop) interaktif

- 📊 **Tipe Data**
  - Number (integer dan float)
  - Boolean (`benar`, `salah`)
  - Kosong (None/null)
  - String (basic, dengan quote single/double)
  - Object (internal: String, Function)

- 🔤 **Kata Kunci**
  - `benar`, `salah`, `kosong` - Literal values
  - `dan`, `atau`, `tidak` - Operator logika
  - `jika`, `jika_lain`, `selain` - Percabangan
  - `selagi` - Perulangan while
  - `fungsi` - Definisi fungsi
  - `kembalikan` - Return statement
  - `cetak` - Print/output

- ➕ **Operator**
  - Aritmatika: `+`, `-`, `*`, `/`, `%`, `**`
  - Perbandingan: `==`, `!=`, `<`, `<=`, `>`, `>=`
  - Logika: `dan`, `atau`, `tidak`
  - Unary: `-` (negasi), `tidak` (NOT)

- 🔧 **Fitur Bahasa**
  - Variabel dinamis (tanpa deklarasi tipe)
  - Fungsi dengan parameter
  - Return values dari fungsi
  - Rekursi (tail recursion dan nested recursion)
  - Scope lokal dan global
  - First-class functions

- 🏗️ **Infrastruktur**
  - Hash table untuk globals dan string interning
  - Memory management (allocator)
  - Value system dengan tagged union
  - Call frames untuk function calls
  - Stack-based VM dengan 64 frame depth

- 📝 **Dokumentasi**
  - README.md lengkap dengan tutorial
  - QUICKSTART.md untuk memulai cepat
  - SYNTAX_REFERENCE.md untuk referensi sintaks
  - Contoh program di folder examples/

- 🔨 **Build System**
  - Makefile untuk Linux/Mac/Windows
  - VS Code tasks.json untuk integrasi editor
  - .gitignore untuk version control

- 📦 **Contoh Program**
  - hello.idk - Hello World
  - variabel.idk - Variabel dan operasi aritmatika
  - percabangan.idk - If-elif-else statements
  - perulangan_selagi.idk - While loops
  - fungsi.idk - Function definitions dan calls
  - kalkulator.idk - Program kalkulator sederhana
  - fibonacci.idk - Rekursi Fibonacci
  - prima.idk - Cek bilangan prima

- 🐛 **Debug Tools**
  - Disassembler bytecode
  - Trace execution (dengan flag DEBUG_TRACE_EXECUTION)
  - Print bytecode (dengan flag DEBUG_PRINT_CODE)

### Technical Details
- **Architecture**: Bytecode VM dengan stack-based execution
- **Language**: C11 standard
- **Compiler**: GCC/Clang compatible
- **VM Opcodes**: 28 opcodes dasar
- **Max Frames**: 64 nested function calls
- **Max Stack**: 16384 values (FRAMES_MAX * 256)
- **Max Constants**: 256 per chunk
- **Max Locals**: 256 per function

### Known Limitations
- ❌ Belum ada for...in loop (gunakan while)
- ❌ Belum ada list/array operations
- ❌ Belum ada dictionary/object literals
- ❌ Belum ada string concatenation dengan +
- ❌ Belum ada break/continue statements
- ❌ Belum ada class/OOP
- ❌ Belum ada module system
- ❌ Belum ada exception handling
- ❌ Belum ada file I/O
- ❌ Error messages masih basic

### Performance Notes
- Function calls: Stack-based, relatif cepat
- Recursion: Mendukung hingga 64 level
- String interning: Otomatis untuk efisiensi memori
- Hash table: FNV-1a hash untuk globals lookup

## Development Roadmap

### v0.2 (Next)
- [ ] For...in loops dengan iterables
- [ ] List operations (create, index, append)
- [ ] Break dan continue statements
- [ ] String concatenation dengan operator +
- [ ] Compound assignment operators (+=, -=, dll)
- [ ] Better error messages dengan line numbers

### v0.3 (Future)
- [ ] Classes dan objects
- [ ] Methods dan properties
- [ ] `diri` (self) keyword
- [ ] Inheritance
- [ ] Garbage Collection (mark-sweep)
- [ ] String methods (upper, lower, split, dll)

### v0.4 (Future)
- [ ] Module system (`impor`, `dari`, `sebagai`)
- [ ] Standard library (math, string, file)
- [ ] File I/O operations
- [ ] Exception handling (`coba`, `kecuali`, `akhirnya`)
- [ ] Better REPL (multiline, history)

### v1.0 (Goal)
- [ ] Stable API
- [ ] Comprehensive standard library
- [ ] Package manager
- [ ] Documentation generator
- [ ] Debugger integration
- [ ] Performance optimizations
- [ ] Cross-platform binary releases

## Contributors
- Initial development: Proyek Indonesiaku Team

## License
Educational and open-source project.

---

**Format Tanggal**: YYYY-MM-DD
**Format Versi**: MAJOR.MINOR.PATCH
