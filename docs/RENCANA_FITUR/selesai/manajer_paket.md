# Rencana Fitur: Manajer Paket (Fase 14)

- **Status:** usulan
- **Target versi:** v1.8.0 (lockfile+CLI lokal), registry publik ditunda
- **Pemilik:** Teguh R

## 1. Masalah

Hari ini "membagikan library Indonesiaku" berarti copy-paste folder `.idk`
secara manual. Mekanisme `impor` sudah resolve lewat 3 jalur berurutan
(`findInSearchPath` + `findInInterpreterRoots` di `src/vm.c`): (1) direktori
berkas yang meng-impor, (2) tiap direktori di env var `INDK_PATH` (dipisah
`;` Windows / `:` Unix), (3) `pustaka/` di sebelah interpreter terpasang.
**Tidak ada** cara mendeklarasikan "proyek ini butuh paket X versi Y",
tidak ada lockfile, tidak ada perintah install — developer harus tahu
persis ke mana menaruh file supaya `impor` menemukannya.

## 2. Cakupan V1 (lokal, TANPA registry) — ini yang dikerjakan dulu

Registry publik **sengaja ditunda** (lihat roadmap: "Registri publik belum
perlu diluncurkan sebelum desain keamanan selesai" — takedown policy,
anti package-squatting, otorisasi publish belum dirancang). V1 cuma
menjawab: install dari Git/tarball/path lokal + lockfile + resolusi versi
sederhana, memakai `INDK_PATH` yang SUDAH ADA, bukan jalur resolusi baru.

## 3. Manifest (usulan, nama field final masih didiskusikan di PR)

```toml
# indonesiaku.toml -- di root proyek
[paket]
nama = "kalkulator-indonesiaku"      # namespace: <penulis>/<nama> opsional
versi = "0.1.0"                      # SemVer wajib
lisensi = "MIT"
interpreter_minimum = "1.6.0"        # dicek vs INDK_VERSION runtime

[dependensi]
# sumber eksplisit -- TIDAK ada registry terpusat di V1, jadi sumbernya
# wajib ditulis: git url, atau path lokal (monorepo), bukan nama polos.
teks-lanjutan = { git = "https://github.com/x/teks-lanjutan-idk", tag = "v1.2.0" }
util-lokal = { path = "../util-lokal" }
```

Alasan TOML (bukan JSON/YAML): sudah ada precedent di contoh struktur
target roadmap (`indonesiaku.toml`), manusia-ramah untuk di-edit tangan,
dan tidak butuh parser baru kompleks — TOML subset yang dibutuhkan (tabel +
string + inline table) bisa ditulis sebagai ~150 baris parser `.idk`
murni (dogfooding: manajer paket ditulis DALAM Indonesiaku sendiri,
bukan C baru), konsisten dengan prinsip "library ditulis dalam .idk,
C tidak wajib" di roadmap Fase 14.

## 4. Lockfile

```toml
# indonesiaku.lock -- JANGAN diedit manual, di-generate
[[paket]]
nama = "teks-lanjutan"
versi = "1.2.0"
sumber = "git+https://github.com/x/teks-lanjutan-idk@<commit-sha-persis>"
checksum_sha256 = "..."
```

Commit SHA persis (bukan tag yang bisa dipindah) + checksum SHA-256 isi
tarball yang di-resolve dari tag itu sekali saat `indk paket install`,
supaya install berikutnya reproducible walau tag di remote berubah
(protection dari tag hijacking).

## 5. Perintah CLI (subset `src/main.c`, pola sama dengan `periksa`/`profil`)

```
indk paket init                 # bikin indonesiaku.toml interaktif
indk paket tambah <sumber>      # tambah dependensi ke manifest + install
indk paket hapus <nama>
indk paket daftar               # tampilkan dependensi terpasang vs manifest
indk paket perbarui [nama]      # re-resolve versi, tulis ulang lock
indk paket install              # baca lock, unduh persis yang di-lock
```

Lokasi install: `<proyek>/.indk_paket/<nama>/` (non-root, per-proyek, TIDAK
global) — lalu ekstensi `main.c` set `INDK_PATH` env di proses child saat
`indk paket install` menjalankan skrip pengguna lewat wrapper, ATAU
(lebih simpel, lebih disukai) dokumentasikan agar skrip pengguna sendiri
yang `export INDK_PATH=.indk_paket` sebelum run. **Pilih opsi kedua**:
tidak menyentuh `src/main.c`/`src/vm.c` sama sekali, murni tooling di atas
`impor` yang sudah ada (ladder rung 2 — reuse, jangan re-implement
resolusi modul).

## 6. Keamanan (gerbang rilis, bukan nice-to-have)

- Unduh via `pustaka/http.idk`/TLS yang sudah ada (tidak ada dependency
  baru) ke direktori temp, verifikasi checksum SEBELUM ekstrak, baru pindah
  ke `.indk_paket/`.
- Ekstraksi tarball: cegah path traversal (`../` dalam entry nama) — pola
  sama dengan guard path traversal yang sudah dipakai `pustaka/web.idk`
  untuk static file serving (`_jalur_nyata()` realpath check).
- `indk paket install` TIDAK pernah menjalankan kode pihak ketiga
  (no post-install script) — V1 sengaja tidak punya hook lifecycle,
  itu sumber RCE paling umum di package manager lain (npm postinstall).
- Commit SHA pinned di lock, bukan tag — cegah tag yang di-republish diam-diam
  mengubah isi paket tanpa menaikkan versi.

## 7. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `pustaka/paket.idk` (baru) | parser TOML subset, resolve dependensi, tulis lock |
| `src/main.c` | subcommand `paket` yang dispatch ke skrip `.idk` internal (pola sama `awakutu`/`profil`/`periksa`) -- **bukan** logika baru di C |
| Tidak ada perubahan `src/vm.c`/`src/compiler.c` | resolusi modul tetap `impor` + `INDK_PATH` yang sudah ada |

## 8. Pengujian

- Golden: `examples/paket_demo/` proyek contoh + `tests/paket_cli.sh`
  (pola sama `tests/keamanan_app.sh` — HTTP nyata, bukan mock) yang:
  install dari repo Git lokal (bukan GitHub asli, supaya tes tidak butuh
  jaringan), verifikasi lock ter-generate, `impor` di skrip pengguna
  berhasil menemukan paket lewat `INDK_PATH=.indk_paket`.
- Tes negatif wajib (sesuai roadmap item 6 Fase 14): checksum tidak cocok
  → gagal sebelum ekstrak; path traversal dalam arsip → ditolak; install
  ulang dari lock yang sama → byte-identik (reproducible); offline tanpa
  cache → pesan galat jelas, bukan hang.
- Lintas OS: path separator `INDK_PATH` (`;` vs `:`) sudah ada presedennya
  di `src/vm.c`, tinggal reuse constant yang sama.

## 9. Dampak dokumentasi & tooling

- [ ] `website/panduan/09_MANAJER_PAKET.md` baru (bab belum ada)
- [ ] `CHANGELOG.md` + `docs/ROADMAP.md` Fase 14 item 1-2 dicentang saat V1 selesai
- [ ] `editors/vscode-indonesiaku/` — tidak ada perubahan wajib di V1 (tidak
      ada syntax/builtin baru), opsional: task provider untuk `indk paket install`

## 10. Risiko & alternatif

- **Registry publik di V1** — ditolak. Butuh desain otorisasi publish,
  takedown, anti-squatting yang roadmap sendiri bilang belum siap. V1
  cuma Git/path sumber eksplisit; registry jadi rencana fitur terpisah
  (`registry_paket.md`) setelah V1 terbukti dipakai.
- **Format manifest JSON instead of TOML** — dipertimbangkan, ditolak:
  lockfile butuh ditulis ulang otomatis sesering `npm install`, dan JSON
  tanpa komentar lebih menyulitkan developer baca diff lockfile di PR;
  TOML comment-friendly menang untuk kasus ini.
- **Dependency resolution otomatis versi (SAT solver)** — ditolak untuk V1.
  Manifest V1 cuma pin versi persis via lock; resolusi rentang versi
  (`^1.2.0`) butuh algoritma resolver yang jauh lebih besar — tunda sampai
  ada laporan nyata orang kesulitan tanpanya (YAGNI).
- **Native C untuk manajer paket** — ditolak, kontradiksi prinsip roadmap
  "library ditulis dalam .idk, C tidak wajib". Satu-satunya sentuhan C
  adalah dispatcher subcommand di `main.c` (≤10 baris, pola `awakutu`).
