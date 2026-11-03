# Standar Branch, Commit, dan Pull Request — Indonesiaku

Dokumen ini berlaku **sama** untuk maintainer, kontributor manusia, dan AI coding agent. Pola ini adalah **konvensi proyek berbasis praktik umum Git/GitHub**, bukan standar universal yang diwajibkan Git. Sumber aturan pengembangan fitur tetap `AGENTS.md`, `CONTRIBUTING.md`, dan `docs/ROADMAP.md`.

## 1. Aturan dasar

- `main` adalah branch integrasi utama dan sumber rilis stabil. Hindari push langsung ke `main` untuk perubahan fitur/bug/dokumentasi; gunakan branch dan pull request. Pengecualian perbaikan mendesak memerlukan review dan pencatatan alasan.
- Satu branch mengerjakan satu tujuan logis; jangan mencampur fitur yang tidak terkait, bug lain, dan refactor besar dalam satu PR.
- Sebelum membuat branch, sinkronkan `main` atau basis PR terbaru. Jangan menimpa perubahan orang lain atau force-push ke branch bersama tanpa persetujuan.
- Gunakan huruf kecil ASCII, angka, tanda hubung (`-`) dan **satu garis miring** pemisah jenis: `jenis/deskripsi-singkat`, atau `jenis/123-deskripsi-singkat` jika issue tersedia. Tidak boleh ada spasi, underscore, tanda baca yang tidak diperlukan, aksen, atau nama yang hanya `fix`/`update`.
- Deskripsi singkat, spesifik dan berbentuk `kebab-case`; idealnya 2–6 kata. Cantumkan nomor issue **jika ada**, bukan membuat nomor fiktif.
- Nama branch tidak mengklaim fitur sudah selesai; status aktual dinilai melalui PR, tes dan roadmap.

## 2. Jenis branch yang disepakati

| Prefix | Kapan dipakai | Contoh |
|---|---|---|
| `fitur/` | Fitur baru di bahasa, VM, library, web atau CLI | `fitur/123-driver-postgresql` |
| `perbaikan/` | Bug fix/regresi yang tidak darurat | `perbaikan/scanner-teks-unicode` |
| `hotfix/` | Insiden kritis pada rilis yang sudah dipublikasikan | `hotfix/131-perbaiki-tls` |
| `keamanan/` | Pengerasan keamanan yang tidak mengungkap kerentanan embargo | `keamanan/batas-ukuran-request` |
| `docs/` | Dokumentasi, tutorial, referensi dan roadmap | `docs/panduan-plugin-idk` |
| `refactor/` | Restrukturisasi internal tanpa perubahan perilaku publik | `refactor/pisahkan-modul-parser` |
| `test/` | Tes, fuzzing, fixtures, coverage | `test/fuzz-parser-kamus` |
| `kinerja/` | Optimasi berdasarkan benchmark/profil | `kinerja/kurangi-alokasi-vm` |
| `ci/` | Workflow GitHub Actions dan quality gates | `ci/tes-windows-arm64` |
| `build/` | Build system, compiler flags, packaging biner | `build/kompatibilitas-linux-x64` |
| `dependensi/` | Pembaruan library dan dependency | `dependensi/perbarui-sqlite` |
| `chore/` | Maintenance non-fungsional | `chore/rapikan-struktur-contoh` |
| `rilis/` | Persiapan versi/artefak rilis dan sinkronisasi versi | `rilis/1.4.0` |
| `eksperimen/` | Proof of concept sementara, **tidak otomatis layak merge** | `eksperimen/event-loop` |

**Kompatibilitas historis:** repo sudah pernah memakai `fase-8/...` dan `rilis/1.3.2`. Branch lama tidak harus diubah. Untuk kontribusi **baru**, gunakan nama jenis dalam tabel. Jika branch eksperimen sudah matang, buat PR yang jelas atau pindahkan perubahan terpilih ke branch `fitur/`.

**Khusus masalah keamanan yang belum diperbaiki:** **JANGAN** membuat branch/issue/PR publik berisi PoC, kunci, detail eksploit atau langkah reproduksi rahasia. Gunakan jalur pelaporan privat dan private security advisories GitHub jika tersedia; nama branch generik saja tidak melindungi informasi rahasia.

## 3. Langkah wajib untuk manusia dan AI agent

1. Baca `AGENTS.md`, `CONTRIBUTING.md`, `docs/ROADMAP.md`, dan spesifikasi/tes terkait. Periksa apakah sudah ada PR/issue yang mengerjakan topik sama.
2. Perbarui lokal dari `main` tanpa membuang pekerjaan yang belum di-commit: `git fetch origin`, `git switch main`, `git pull --ff-only origin main`.
3. Buat branch dari basis yang benar: `git switch -c fitur/123-driver-postgresql` (ganti dengan kategori dan judul sebenarnya).
4. Untuk fitur, **tulis rencana terlebih dahulu** di `docs/RENCANA_FITUR/`; untuk bug, sediakan reproduksi dan regression test sebelum menyatakan perbaikan selesai.
5. Kerjakan commit kecil yang mudah ditinjau, jalankan tes yang terkait dan pengujian wajib di AGENTS.md, perbarui contoh, docs, CHANGELOG dan editor bila relevan.
6. Periksa `git diff`, `git status`, rahasia dan file hasil build. Jangan commit `.env`, token, private key, file pribadi, dataset sensitif, atau artefak sementara.
7. Push branch dan buka PR ke `main`. Isi latar belakang, issue, scope, daftar perubahan, bukti tes, dampak kompatibilitas/keamanan, serta checklist reviewer. Tunggu CI dan review sebelum merge.
8. Setelah merge, sinkronkan `main`, hapus branch kerja lokal/remote bila aman, dan perbarui status roadmap hanya jika syarat benar-benar terpenuhi.

**Aturan AI agent tambahan:** ikuti aturan yang sama seperti manusia; jangan membuat branch bernama `ai/` sebagai pengganti kategori pekerjaan. Boleh menambahkan jejak agen di deskripsi PR (`Dibantu oleh AI`) jika sesuai kebijakan maintainer. Jangan menganggap kehadiran AI membolehkan bypass tes, review, atau approval. Jangan mengubah branch saat ada perubahan lokal milik pengguna tanpa pengamanan/izin. Jangan menghapus branch/force-push tanpa instruksi jelas. Setiap perubahan teknis tetap harus dapat diaudit melalui diff dan tes.

## 4. Commit dan PR

- Pertahankan format commit bahasa Indonesia yang **sudah berlaku di repository**: `Tambah:`, `Perbaiki:`, `Ubah:`, `Hapus:`, `Docs:`, `Style:`, `Refactor:`, `Test:`, `Chore:`; gunakan `Keamanan:`, `Build:`, `CI:`, `Kinerja:`, `Dependensi:`, `Rilis:` jika disetujui maintainer untuk kategori baru.
- Contoh: `Tambah: driver PostgreSQL dengan parameter query`, `Perbaiki: pembacaan Unicode dalam scanner`, `Docs: jelaskan tata cara membuat plugin`, `Test: tambah regresi timeout HTTP`.
- Nama branch dan awalan commit tidak harus sama persis (misalnya branch `fitur/...` boleh berisi commit `Test:` dan `Docs:` yang mendukung fitur tersebut).
- Judul PR menjelaskan perubahan secara singkat. Body PR mencantumkan **tujuan**, **relasi issue/roadmap**, **perubahan**, **bukti tes**, **risiko/security/compatibility**, dan **checklist penerimaan**.
- `Fixes #123` hanya bila PR benar-benar menyelesaikan issue tersebut; jika sekadar referensi gunakan `Refs #123`.
- PR untuk `rilis/` perlu mencocokkan versi compiler, installer, Docker, website, CHANGELOG, dan GitHub Releases. Branch `hotfix/` harus menghasilkan patch yang sesuai aturan SemVer dan tes regresi.

## 5. Contoh kerja lengkap

```bash
git fetch origin
git switch main
git pull --ff-only origin main
git switch -c fitur/123-driver-postgresql

# Buat rencana fitur, tambahkan kode, tes, dan dokumentasi
git add docs/RENCANA_FITUR/ tests/ src/ pustaka/
git commit -m "Tambah: dukungan koneksi PostgreSQL"
git push -u origin fitur/123-driver-postgresql

# Setelah itu buka Pull Request ke main, review, tes, lalu merge.
```

Contoh singkat kategori lain: `perbaikan/galat-iterasi`, `docs/referensi-api`, `keamanan/batas-rekursi`, `ci/pengujian-sanitizer`, `rilis/1.5.0`.

## 6. Perbaikan bertahap: satu temuan, satu branch, satu PR

Saat mengerjakan daftar temuan satu per satu (misalnya hasil uji di
`docs/TESTING/`, laporan audit, atau daftar bug dari issue), **setiap temuan
mendapat branch dan PR sendiri**. Jangan mengumpulkannya menjadi satu PR
raksasa. Contohnya adalah perbaikan T-U2–T-U16 dari uji Windows 1.4.0: satu
cabang `perbaikan/...` per temuan, masing-masing menuju `main` (PR #41–#55).

- **Setiap branch dibuat dari `main` terbaru**, bukan dari branch perbaikan
  lain. Branch boleh ditumpuk hanya bila memang ada ketergantungan, dan
  ketergantungan itu ditulis di deskripsi PR.
- **Gabungkan hanya temuan yang tidak bisa dipisah**, misalnya karena akar
  masalah atau baris kodenya sama (T-U13 + T-U14). Tulis alasannya di PR.
- **Setiap PR lengkap dan teruji sendiri:** rencana di
  `docs/RENCANA_FITUR/`, regression test (golden/`tests/galat`/skrip uji),
  entri `CHANGELOG.md`, dokumentasi yang terdampak, dan bukti tes di
  deskripsi PR. PR tidak boleh bergantung pada PR lain untuk lulus CI.
- **Temuan yang mengubah semantik** (keputusan K) ditanyakan dulu ke
  maintainer. Temuan seperti itu tidak boleh menahan PR lain: kerjakan yang
  lain lebih dulu, lalu buat PR terpisah setelah keputusan dicatat.
- **Jangan mengganggu pekerjaan lain.** Orang atau agen lain bisa sedang
  bekerja di `main`, di branch lain, atau di worktree/checkout lain. Jangan
  mengedit berkas, menjalankan build, atau mengubah branch di worktree milik
  orang/sesi lain. Untuk menguji beberapa branch sekaligus, ekspor salinan
  (`git -c core.autocrlf=false archive <commit>`) ke folder sementara.
  Menambah commit ke branch PR milik orang lain hanya boleh lewat push
  *fast-forward*, setelah memastikan checkout pemiliknya bersih dan sinkron,
  dan **tidak pernah** dengan force-push.
- **Konflik `CHANGELOG.md` itu wajar.** Setiap PR menambah entri di awal
  bagian yang sama, jadi PR yang di-merge belakangan perlu di-rebase. Saat
  menyelesaikan konflik, pertahankan semua entri.
- **Catat status per temuan** (temuan → PR/branch → status) di tempat yang
  dibaca semua orang, misalnya deskripsi PR induk atau berkas hasil uji, agar
  tidak ada temuan yang dikerjakan dua kali atau terlewat.

## 7. Pemeriksaan kepatuhan

- [ ] Branch mengikuti kategori dan pola yang relevan; nomor issue akurat.
- [ ] Tidak membuat perubahan langsung pada `main` tanpa alasan dan persetujuan.
- [ ] Satu PR punya satu tujuan logis, dan tidak mengandung rahasia.
- [ ] Daftar temuan dikerjakan sebagai satu branch + satu PR per temuan dari `main` terbaru (§6), tanpa menyentuh worktree atau branch orang lain.
- [ ] Feature plan/bug reproduction sudah ada sesuai jenis perubahan.
- [ ] Tes, dokumen, contoh, kompatibilitas dan keamanan diperiksa.
- [ ] CI hijau dan reviewer menyetujui sebelum merge.
- [ ] Nama branch bukan alasan untuk mengubah nomor versi; rilis mengikuti kebijakan SemVer dan release gate di roadmap.

**Penerapan:** konvensi ini adalah pedoman kontribusi. Penegakan otomatis melalui GitHub rulesets/branch protection atau CI pemeriksa nama branch dapat dibuat terpisah setelah memeriksa dukungan fork PR, Dependabot dan bot resmi; jangan mengaktifkan aturan yang membuat kontribusi sah menjadi gagal tanpa dokumentasi.
