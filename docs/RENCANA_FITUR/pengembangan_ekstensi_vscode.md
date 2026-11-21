# Rencana Fitur: Pengembangan Ekstensi VS Code Indonesiaku

- **Status:** dikerjakan (sebagian — lihat §1 untuk status per-item)
- **Target versi:** 1.8.0 (panel + ikon, selesai), sisanya bertahap
- **Pemilik:** Teguh R

## 1. Status hari ini (per 1.7.0 + perubahan berjalan)

Sudah ada: syntax highlighting, LSP diagnostics, completion, hover,
go-to-definition, snippets, `indk rapikan` lewat Format Document.

Baru ditambahkan (belum dirilis):
- [x] Panel Activity Bar "Indonesiaku" — tombol Jalankan/Periksa/Rapikan/
      Profil/Awakutu, jalan lewat terminal terintegrasi bersama.
- [x] Tombol "Jalankan" di pojok kanan atas editor saat file `.idk` aktif.
- [x] Kontribusi `iconThemes` ("Indonesiaku Icons") untuk ikon Explorer
      `.idk`/`.indk`, ditawarkan sekali lewat prompt (tidak memaksa ganti
      tema ikon global pengguna).

**Yang masih kertas putih kalau pengguna tidak klik "Aktifkan" di prompt**:
ikon Explorer tetap default sampai `workbench.iconTheme` di-set manual ke
`indonesiaku-icons` — ini keterbatasan API VS Code (satu File Icon Theme
aktif untuk seluruh Explorer, bukan per-ekstensi berkas), bukan bug.

## 2. Backlog bertahap

### A — Debugging asli (DAP), bukan cuma jalankan `indk awakutu` di terminal
**Status: ✅ selesai (belum dirilis).** Adapter DAP tanpa dependensi
di `editors/vscode-indonesiaku/debug-adapter.js` (+ `awakutu-proses.js`
untuk protokol stdin/stderr, `awakutu-sumber.js` untuk tebakan nama dari
sumber), `contributes.debuggers`/`breakpoints` di `package.json`, dan
`DebugAdapterInlineImplementation` di `extension.js` (tanpa `node` di
PATH). Diuji `test-dap.js` (`npm run test:dap`, job CI `editor-lsp`):
breakpoint, step over/into/out, variabel, evaluate, lanjut sampai
selesai, galat runtime (exit 70), binari hilang, disconnect. **Tidak ada
perubahan C.** Batas yang diwarisi awakutu (dicatat di README ekstensi):
breakpoint hanya per nomor baris berkas utama, `lihat` hanya global +
parameter, tidak ada pause, step into/out ditiru adapter. Menutup batas
itu butuh kerja C (Fase 15 langkah 1–2), bukan adapter.

Catatan audit awal (dipertahankan):
`indk awakutu` sudah ada sebagai REPL CLI (breakpoint per baris, `lihat`,
dll — lihat skill note "indk awakutu: debugger CLI"). Supaya breakpoint
bisa diklik di gutter editor (bukan command CLI), ekstensi butuh **Debug
Adapter Protocol (DAP)** server yang menjembatani VS Code ↔ `indk awakutu`.
- Perlu `contributes.debuggers` di `package.json` + `src/debugAdapter.js`
  yang menerjemahkan request DAP (`setBreakpoints`, `continue`, `variables`)
  ke protokol stdin/stdout `indk awakutu` yang sudah ada.
- **Audit selesai (hasil: BISA tanpa sentuh C)**: `awakutuPrompt()` di
  `src/vm.c` baca command dari `stdin` via `readLineAlloc()` baris-per-
  baris, dan nulis state (`[baris N] ...`, `(awakutu)`) ke `stderr` dengan
  format konsisten. Adapter DAP cukup `spawn("indk", ["awakutu", file])`
  + tulis command teks ke stdin (`b <baris>`, `lanjut`, `langkah`,
  `lihat <nama>`, `tumpukan`) + parse stdout/stderr -- reuse REPL yang
  sudah ada, BUKAN protokol baru di bahasa. Next: `src/debugAdapter.js`
  (state machine parser) + `contributes.debuggers` di `package.json`.
  Estimasi beberapa jam kerja (parser state + mapping breakpoint↔baris +
  UI launch config) -- jalankan di sesi terpisah, bukan diff kecil.

### B — Snippet & completion untuk builtin baru otomatis
`language-data.js` BUILTINS di-generate manual dari `defineNative()` —
skill note sudah bilang ini pernah drift (35 builtin ketinggalan). Tambah
skrip `scripts/sinkron_builtin_vscode.js` yang generate `language-data.js`
+ pola tmLanguage dari `grep defineNative` otomatis, dijalankan di CI
lewat `tests/builtin_konsisten.sh` yang **sudah ada** — jangan bikin
mekanisme paralel, sambungkan ke situ.

### C — Status bar: tombol run cepat + indikator lint
Tombol run di `editor/title` sudah ada (§1). Tambah item status bar kecil
(`vscode.window.createStatusBarItem`) yang nampilin jumlah temuan
`indk periksa` terakhir, klik → jalankan ulang. Kecil, tidak butuh view
baru — reuse command `indonesiaku.periksa` yang sudah didaftarkan.

### D — Task Provider (`indk.tasks.json` otomatis)
`contributes.taskDefinitions` + `TaskProvider` supaya `Ctrl+Shift+B` (Run
Build Task) langsung tawarkan "indk: jalankan berkas aktif" tanpa harus
bikin `.vscode/tasks.json` manual. Kecil, native VS Code API, tidak ada
paket baru.

### E — Marketplace housekeeping
- Versi ekstensi WAJIB naik bareng `INDK_VERSION` (skill note sudah catat
  ini pernah ketinggalan di `0.2.0` vs bahasa `1.6.0`) — `tests/
  versi_konsisten.sh` sudah menjaga, tidak perlu kerjaan baru, cuma jangan
  lupa jalankan sebelum publish.
- README ekstensi: tambah screenshot panel Activity Bar + cara aktifkan
  ikon, karena keduanya baru dan tidak kelihatan tanpa dijelaskan.

## 3. Urutan prioritas (lazy — jangan kerjakan D sebelum A kalau user minta A)

1. **A (debugging asli)** — permintaan eksplisit user paling besar
   dampaknya, tapi butuh audit `indk awakutu` dulu sebelum janji estimasi.
2. **E (housekeeping)** — wajib sebelum rilis apa pun, murah.
3. **C (status bar lint)** — kecil, selesai dalam satu sesi.
4. **B (sinkron builtin)** — mencegah regresi lama terulang, tidak mendesak.
5. **D (task provider)** — nice-to-have, tidak ada yang minta.

## 4. Dampak dokumentasi & tooling

- [x] `editors/vscode-indonesiaku/README.md` — debugging asli (A)
      sudah; panel dan ikon belum
- [ ] `CHANGELOG.md` + `docs/ROADMAP.md` per item yang selesai
- [ ] `tests/builtin_konsisten.sh` — sambungkan skrip B ke sini, jangan
      bikin job CI baru

## 5. Risiko & alternatif

- **A ditolak sementara jadi "ganti terminal jadi panel output custom"**
  sebagai alternatif murah — ditolak karena itu downgrade (user sudah
  minta breakpoint, bukan cuma log run). Kalau `indk awakutu` butuh kerja
  C besar, laporkan itu transparan, jangan bungkus jadi fake-DAP yang
  cuma print output.
- **iconThemes otomatis tanpa prompt** ditolak — mengganti
  `workbench.iconTheme` pengguna diam-diam saat install adalah perilaku
  invasif yang banyak dikeluhkan di review ekstensi lain.
