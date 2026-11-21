# Rencana Fitur: Kesiapan Produksi untuk REST API & Website SSR

- **Status:** selesai (V1 — lihat §0; sisanya ditunda, dicatat di §0 dan `docs/ROADMAP.md` Fase 22)
- **Target versi:** v1.17.0 (Fase 22, lihat `docs/ROADMAP.md`)
- **Pemilik:** Teguh R

## 0. Cakupan V1 yang dikerjakan (tanpa C baru)

V1 dipangkas menjadi bagian yang bisa dibangun murni di `.idk`, di atas
`pustaka/web.idk` dan `pustaka/log.idk`, tanpa builtin baru:

| Paket | Status V1 | Wujud |
|---|---|---|
| A.1 health/readiness | ✅ | `app.pantau(kesehatan="/health", kesiapan="/ready", periksa_siap=f, metrik="/metrics")` mendaftarkan rute GET; `/health` selalu 200 `{"status":"ok"}`, `/ready` 200 atau 503 `{"status":"tidak siap"}` dari `periksa_siap()` (galat di dalamnya → 503, detail hanya ke log). Dilayani lewat middleware biasa. |
| A.2 log terstruktur | ✅ | `log.atur(format="json")`: satu objek JSON per baris (`waktu` lokal ISO-8601, `level`, `modul`, `pesan`, lalu field `data`). Semua metode pencatat menerima `data=` kamus opsional; mode teks mengabaikannya sehingga format lama byte-identik. `format` selain teks → `GalatLog`. |
| A.3 correlation ID | ✅ | `req["id"]`: 16 heks dari `acak_bita(8)` per permintaan. `X-Request-ID` masuk dihormati HANYA dari `proxy_tepercaya()` dan bila 1–64 karakter `[A-Za-z0-9._-]` (anti injeksi log). Header `X-Request-ID` di respons dan field `request_id` di log `app.jalankan()` hanya bila `pantau()` aktif — respons aplikasi lama tidak berubah. |
| B.5 metrics | ✅ | Counter in-process: `indk_http_permintaan_total{metode,status}`, histogram `indk_http_durasi_detik` (bucket bawaan Prometheus, `_sum`/`_count`; p50/p95 lewat `histogram_quantile`), gauge `indk_mulai_detik`. Format teks Prometheus 0.0.4. Path TIDAK dijadikan label (kardinalitas); metode di luar daftar baku → `LAIN`. |
| A.4 graceful shutdown SIGTERM | ⏸ ditunda | Butuh C (`volatile sig_atomic_t` + builtin baru). `app.berhenti()` yang sudah ada tetap satu-satunya jalur berhenti anggun. |
| B.6 tracing | ❌ tetap ditolak | lihat §2. |
| C.7–9 SECURITY.md, tabel versi minimum | ⏸ ditunda | Dokumen kebijakan, bukan bagian stack web; kerjakan dalam PR dokumen tersendiri. Audit 08_DEPLOYMENT dilakukan sebagian (bagian observability + peringatan SIGTERM). |
| D.10 uji beban | ⏸ ditunda | Belum ada angka; jangan klaim "siap produksi". |

Batas V1 yang ditulis jujur di panduan: satu utas (endpoint `/metrics`
dilayani bergiliran), counter hilang saat proses restart, dan `/metrics`
harus dibatasi aksesnya (reverse proxy atau middleware) karena tidak ada
autentikasi bawaan.

## 1. Masalah

Hari ini `pustaka/web.idk` sudah bisa menjalankan REST API maupun website
SSR (lihat `examples/todo_server.idk`, `examples/web_mahasiswa.idk`), dan
`website/panduan/08_DEPLOYMENT.md` sudah mendokumentasikan Docker +
reverse proxy + TLS + volume SQLite. **Yang belum ada** begitu aplikasi itu
jalan 24/7 di belakang nginx:

- Tidak ada log terstruktur (JSON) — `pustaka/log.idk` baru `cetak()` baris
  teks ke stderr/file, tanpa request-ID, tanpa level filter per-modul yang
  gampang di-parse Loki/ELK.
- Tidak ada endpoint `/health` atau `/ready` bawaan — tiap aplikasi harus
  menulis sendiri, dan reverse proxy/orchestrator tidak punya kontrak baku.
- Tidak ada metrics (request count, latensi, error rate) yang bisa di-scrape
  Prometheus. `indk profil` ada, tapi itu alat dev (hitung opcode via
  stderr sekali jalan), bukan metrics server yang live.
- Tidak ada graceful shutdown — `app.jalankan()` berhenti lewat quota
  `maks_permintaan` atau dibunuh paksa (SIGKILL dari Docker), bukan
  menyelesaikan request yang sedang berjalan dulu.
- Tidak ada `SECURITY.md` / kebijakan disclosure kerentanan.
- Tidak ada kebijakan versi runtime minimum yang didukung sebuah aplikasi
  (mis. "butuh indk >= 1.6.0 karena pakai `cocokkan`").

Operator yang deploy REST API/SSR hari ini harus menebak sendiri semua di
atas — tidak ada rujukan tertulis, dan Fase 22 roadmap baru berisi daftar
tujuan, belum rencana konkret.

## 2. Cakupan dan urutan (bukan satu rilis)

Ditulis sebagai 4 paket, masing-masing bisa jadi minor version sendiri —
jangan kerjakan semua sekaligus:

### Paket A — Observability minimum (prasyarat produksi)
1. **Health/readiness endpoint**: tambah contoh pola di `pustaka/web.idk`
   (bukan builtin wajib — cukup `app.rute("GET", "/health", ...)` yang
   dicontohkan di panduan) yang mengembalikan 200 + JSON `{"status":"ok"}`.
   Readiness beda dari liveness: readiness boleh cek koneksi DB
   (`db_eksekusi("SELECT 1")`), liveness tidak.
2. **Log terstruktur**: `pustaka/log.idk` dapat `format = "json"` di
   `atur()` — tiap baris jadi satu objek JSON (`waktu`, `level`, `modul`,
   `pesan`, `request_id` opsional). Reuse `json_teks()` yang sudah ada,
   jangan tulis encoder JSON baru.
3. **Request correlation ID**: `app.jalankan()` generate ID per koneksi
   (pakai `acak_bita()` yang sudah kriptografis, encode hex/base64 yang
   sudah ada) dan taruh di `req.id` supaya satu request bisa ditelusuri
   lintas baris log.
4. **Graceful shutdown**: `app.jalankan()` tangkap SIGTERM (builtin baru,
   lihat §4), stop accept loop baru, selesaikan koneksi yang sedang
   diproses, baru exit — bukan `exit()` langsung di tengah request.

### Paket B — Metrics dasar
5. Counter in-process (request total, error total per status code, durasi
   p50/p95 sederhana) diekspos lewat endpoint `/metrics` format teks
   Prometheus (`nama_metric{label} nilai`) — format teks, tidak butuh
   library eksternal, konsisten dengan prinsip "tanpa dependency baru".
6. **Secara eksplisit TIDAK**: tracing terdistribusi (OpenTelemetry span
   propagation) — butuh client HTTP lintas-proses yang lebih kompleks dari
   yang ada; tulis ini di dokumentasi sebagai batas, jangan diam-diam
   dilewati.

### Paket C — Kebijakan tertulis (dokumen, bukan kode)
7. `SECURITY.md` — cara lapor kerentanan, SLA respons, versi yang dapat
   patch keamanan.
8. Kebijakan versi runtime minimum per rilis — tabel di
   `docs/ROADMAP.md` atau `CHANGELOG.md`: fitur bahasa apa butuh versi
   berapa (mis. `cocokkan` sejak 1.6.0, `db.idk` sejak 1.6.0).
9. **Audit jujur `08_DEPLOYMENT.md`** supaya eksplisit bilang: model
   konkurensi satu utas (sudah ada tulisannya), **tambahkan** peringatan
   soal graceful shutdown belum ada SAMPAI Paket A selesai — jangan biarkan
   panduan mengklaim lebih dari yang runtime benar-benar bisa.

### Paket D — Pengujian beban (gating sebelum "siap produksi" diklaim)
10. Uji `wrk`/`hey` terhadap `examples/todo_server.idk` di belakang nginx:
    laporkan req/detik dan latensi apa adanya (jangan dibandingkan dengan
    server multi-thread — beda kelas), supaya operator tahu kapasitas
    nyata sebelum pakai di produksi.

## 3. Semantik / batas yang harus jujur ditulis

- VM tetap **satu utas** — Paket A-D tidak mengubah ini. Metrics/health
  endpoint tetap dilayani bergiliran dengan request lain, jadi endpoint
  `/metrics` yang lambat (mis. query DB berat) ikut menunda request user.
  Jangan buat `/health` melakukan apa pun yang mahal.
- Correlation ID per-koneksi, bukan per-utas (karena tidak ada utas) —
  cukup dan jujur untuk model request-sekali-jalan ini.
- SIGTERM handling di C (`src/vm.c` atau `src/native.c`) butuh
  `volatile sig_atomic_t` flag yang dicek di accept loop, bukan longjmp
  dari dalam signal handler (unsafe).

## 4. Rencana implementasi per paket

| Paket | Bagian | Perubahan |
|---|---|---|
| A.1 | `pustaka/web.idk` + panduan | contoh rute health/ready, tidak perlu C |
| A.2 | `pustaka/log.idk` | mode `format="json"`, pakai `json_teks()` existing |
| A.3 | `pustaka/web.idk` | `req.id` dari `acak_bita()` existing |
| A.4 | `src/native.c`, `src/vm.c` | native `sinyal_tangkap("TERM")` + flag yang dicek accept loop di `app.jalankan()` (`pustaka/web.idk`) |
| B.5 | `pustaka/web.idk` + `pustaka/teks.idk` | counter in-memory + handler `/metrics` contoh di panduan |
| C.7-9 | dokumen saja | `SECURITY.md`, tabel versi, audit `08_DEPLOYMENT.md` |
| D.10 | `tests/` (manual, bukan CI) | skrip beban + laporan di PR, bukan golden test |

## 5. Pengujian

- A.1-A.3: golden `.idk` + `.out` seperti pola `examples/web_mahasiswa.idk`.
- A.4 (SIGTERM): tes shell (`tests/`) yang start server latar,
  `kill -TERM`, pastikan request yang sedang jalan selesai dulu (pola sama
  dengan `tests/kebocoran_resource.sh` — `/dev/tcp` bash, bukan curl).
- B.5: assert format `/metrics` cocok regex Prometheus dasar, dan counter
  naik setelah N request (growth shape, bukan angka absolut — pola yang
  sama dipakai `tests/web_badan_performa.sh`).
- Jalankan dengan `-DDEBUG_STRESS_GC` kalau A.4/B.5 menyentuh alokasi baru
  di `src/native.c`.

## 6. Dampak dokumentasi & tooling

- [ ] `website/panduan/08_DEPLOYMENT.md` — tambah §8.10 health/metrics/
      graceful shutdown, perbarui §8.1 batas yang harus disadari
- [ ] `website/panduan/07_PUSTAKA_STANDAR.md` — `log.idk` mode JSON,
      `web.idk` correlation ID
- [ ] `SECURITY.md` baru
- [ ] `CHANGELOG.md` + `docs/ROADMAP.md` Fase 22 dicentang per paket selesai
- [ ] `AGENTS.md` kalau SIGTERM handling punya jebakan signal-safety

## 7. Risiko & alternatif

- **Risiko utama**: mengklaim "siap produksi" sebelum Paket D (uji beban)
  selesai — roadmap sendiri menegaskan jangan klaim setara server produksi
  tanpa bukti. Setiap paket yang rilis HARUS menyebut paket mana yang
  belum ada di catatan rilisnya.
- **Alternatif yang ditolak**: mengadopsi OpenTelemetry SDK C — tidak ada
  dependency dibolehkan tanpa kebutuhan nyata (ladder YAGNI); format teks
  Prometheus cukup untuk versi pertama, SDK penuh baru dipertimbangkan
  kalau ada permintaan tracing lintas-servis.
- **Alternatif yang ditolak**: thread pool untuk paralelisme request —
  itu pekerjaan Fase tersendiri (lihat catatan WEB-07 di roadmap: butuh
  menyentuh GC, stack VM, tabel global), di luar cakupan dokumen ini.
