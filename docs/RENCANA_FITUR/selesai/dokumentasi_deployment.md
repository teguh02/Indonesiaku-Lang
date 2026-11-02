# Rencana Fitur: dokumentasi deployment

- **Status:** selesai
- **Target versi:** 1.5.0
- **Pemilik:** teguh02

## 1. Masalah

Fase 11 langkah 8 meminta dokumentasi deployment: Docker, reverse proxy TLS,
isolasi proses, pembatasan resource, dan risiko membuka port publik.

Sekarang tidak ada satu pun. Yang ada hanya `docker/images/Dockerfile` (image
interpreter untuk menjalankan skrip, bukan menjalankan server) dan
`docker/playground/` (situs playground, bukan aplikasi pengguna). Pengguna
yang menulis server dengan `app.jalankan()` tidak punya rujukan cara
menaruhnya di internet.

Ini berbahaya justru karena `app.jalankan()` baru saja mempermudah membuat
server. Tanpa dokumentasi, jalur termudah adalah menjalankan `indk server.idk`
sebagai root di port 80 pada VPS — tanpa TLS, tanpa batas resource, dan
dengan satu permintaan dilayani pada satu waktu.

## 2. Mengapa dokumentasi, bukan kode

Langkah 8 memang minta dokumentasi. Tapi dokumentasi deployment yang ditulis
dari ingatan adalah cara klasik menyesatkan: `Dockerfile` yang tidak pernah
di-build, konfigurasi nginx yang tidak pernah menerima satu permintaan pun.

Maka semua contoh di bab ini **dijalankan sungguhan** lebih dulu: image
di-build, stack dinyalakan, permintaan HTTP dikirim lewat proxy, dan batas
resource diperiksa dari `docker inspect`.

## 3. Lingkup

Termasuk:

- `docker/aplikasi/Dockerfile` — image untuk menjalankan server pengguna,
  berbeda dari `docker/images/Dockerfile` yang untuk menjalankan skrip.
- `docker/aplikasi/docker-compose.yml` — aplikasi + reverse proxy, dengan
  isolasi dan batas resource.
- `docker/aplikasi/nginx.conf` — reverse proxy, termasuk blok TLS.
- Bab panduan baru `08_DEPLOYMENT.md`.

Tidak termasuk:

- **Menerbitkan image ke registry.** Bukan bagian langkah 8.
- **TLS otomatis (ACME/Let's Encrypt) yang dijalankan di CI.** Butuh domain
  publik; perintahnya didokumentasikan, tidak bisa diuji di sini.
- **Orkestrasi (Kubernetes, Swarm).** Jauh di luar lingkup.

## 4. Keputusan

- **Reuse `docker/playground/` sebagai rujukan**, bukan menulis pola
  pengamanan dari nol: berkas itu sudah memakai `read_only`, `cap_drop: ALL`,
  `no-new-privileges`, `pids_limit`, `mem_limit`, dan jaringan `internal`.
  Pola yang sudah terbukti di repo ini lebih baik daripada praktik umum yang
  diingat-ingat.
- **Aplikasi tidak pernah mengekspos port ke host.** Hanya proxy yang punya
  `ports:`. Aplikasi berada di jaringan `internal: true`, jadi tidak bisa
  dihubungi langsung dan tidak punya akses internet keluar.
- **Pengguna non-root dengan UID eksplisit** (`10001:10001`), bukan sekadar
  `USER nobody`: UID yang pasti memudahkan izin volume.
- **TLS dijelaskan dengan blok konfigurasi nyata**, tetapi stack bawaan jalan
  di HTTP karena sertifikat butuh domain. Sertifikat uji coba dibuat di bab
  itu dengan `openssl` supaya pembaca bisa mencobanya lokal.
- **Batas resource disebut angkanya** (`mem_limit`, `cpus`, `pids_limit`),
  bukan "atur sesuai kebutuhan" yang tidak menolong siapa pun.

## 5. Risiko yang wajib ditulis apa adanya

Bab ini tidak boleh memberi kesan server Indonesiaku siap beban publik:

- `app.jalankan()` melayani **satu permintaan pada satu waktu**; reverse proxy
  tidak mengubah itu, hanya menampung koneksi di depannya.
- Tanpa reverse proxy, server langsung menghadap internet tanpa TLS, tanpa
  batas laju, dan tanpa pembatasan ukuran permintaan dari sisi proxy.
- Membuka port ke `0.0.0.0` di VPS berarti seluruh internet bisa
  menghubunginya; `docker run -p` **menembus** firewall `ufw` pada
  konfigurasi umum karena Docker menulis aturannya sendiri ke iptables.

## 6. Pengujian

Dokumentasi tidak bisa diuji `tests/test_all.sh` seperti kode, tetapi
contoh-contohnya bisa:

- Build image aplikasi sungguhan.
- Nyalakan stack, kirim permintaan HTTP lewat proxy, pastikan dijawab
  aplikasi.
- Pastikan port aplikasi **tidak** bisa dihubungi dari host.
- Pastikan aplikasi **tidak** punya akses internet keluar.
- Periksa batas resource dan hardening lewat `docker inspect`.
- Blok kode Indonesiaku di bab baru ikut dijalankan `tests/test_panduan.sh`.

## 7. Langkah

- [x] `docker/aplikasi/Dockerfile`, `docker-compose.yml`, `nginx.conf`
- [x] Contoh server yang dipakai image itu
- [x] Buktikan stack jalan sungguhan (build, request, isolasi, batas)
- [x] Bab `website/panduan/08_DEPLOYMENT.md`
- [x] Daftarkan bab di `website/panduan/README.md`
- [x] `CHANGELOG.md` + `docs/ROADMAP.md`

## 8. Hasil tak terduga

Menjalankan contoh dokumentasi menemukan **dua bug nyata** yang tidak akan
ketahuan kalau bab ini ditulis dari ingatan:

1. **`indk` tidak bisa dibangun di musl/Alpine** sejak `soket_batas_waktu()`
   ditambahkan: `struct timeval` dan `suseconds_t` butuh `<sys/time.h>` yang
   glibc tarik diam-diam tetapi musl tidak. Ini juga merusak
   `docker/images/Dockerfile` yang sudah ada sebelumnya. Diperbaiki dengan
   satu `#include`, dan CI mendapat job `test-musl` supaya tidak kambuh.
2. **`soket_dengar()` selalu mengikat `127.0.0.1`**, sehingga server
   Indonesiaku tidak bisa dipakai di dalam container sama sekali: reverse
   proxy mendapat `Connection refused` padahal server hidup. Ditambahkan
   argumen alamat opsional; bawaannya tetap loopback.
