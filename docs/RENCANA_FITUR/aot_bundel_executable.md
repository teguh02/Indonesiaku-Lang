# Rencana Fitur: AOT / Bundel Executable (Fase 8, butir tersisa)

- **Status:** usulan
- **Target versi:** v1.9.0 (bareng Fase 16 per tabel rilis roadmap)
- **Pemilik:** Teguh R

## 1. Masalah

Mendistribusikan skrip Indonesiaku ke pengguna akhir (bukan developer)
berarti mereka butuh install `indk` + `pustaka/` + skrip `.idk` terpisah.
Tidak ada cara membuat satu file executable yang bisa diklik/dijalankan
tanpa instalasi terpisah -- padahal `indonesiaku_<versi>_amd64.deb` dan
`Indonesiaku-Setup-<versi>.exe` (Fase 8, sudah selesai) membuktikan
distribusi binary-only sudah lazim di proyek ini, cuma belum ada yang
MENGGABUNGKAN biner interpreter + skrip pengguna jadi satu artefak.

## 2. Pendekatan: self-extracting append, BUKAN compiler AOT baru

**Ditolak**: AOT sungguhan (kompilasi bytecode Indonesiaku → native code,
mis. lewat LLVM) -- itu proyek compiler backend terpisah yang besarnya
setara menulis ulang VM, dan ANALISIS/04_TEKNIK_LANJUTAN.md sendiri
menandai AOT/JIT sebagai "❌ (riset)", bukan readiness Fase 8.

**Dipakai**: pola "self-extracting executable" yang sudah umum (mirip
`makeself`/PyInstaller onefile) -- salin biner `indk` apa adanya, lalu
**tempelkan** (append) payload di akhir file: skrip pengguna + modul
`.idk` yang di-`impor`-nya (dikumpulkan dengan menjalankan compiler dalam
mode "resolusi impor saja", bukan eksekusi) + manifest kecil, lalu tulis
panjang payload di 8 byte terakhir file supaya `indk` bisa menemukannya
saat start tanpa argumen.

```
[ biner indk asli ][ tar skrip+modul ][ panjang tar (8 byte LE) ]
```

Saat `argc == 1` (dipanggil tanpa argumen skrip), `main.c` membaca 8 byte
terakhir dari `argv[0]` sendiri, kalau valid (bukan metadata biner normal)
ekstrak payload ke direktori temp, set `INDK_PATH` ke situ, lalu jalankan
entry point dari manifest -- SELURUHNYA memakai mekanisme resolusi modul
yang SUDAH ADA (`findInSearchPath` di `src/vm.c`), bukan jalur baru.

## 3. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/main.c` | di awal `main()`, kalau `argc==1`: cek 8 byte terakhir biner sendiri, ekstrak kalau ada payload (≤30 baris baru) |
| `scripts/bangun_bundel.sh` (baru) | `tar` skrip+modul impor, append ke salinan biner, tulis panjang -- murni shell, tanpa dependency baru |
| Tidak ada perubahan `compiler.c`/`vm.c` | tidak ada compiler AOT baru, bytecode tetap diinterpretasi seperti biasa setelah ekstrak |

## 4. Keamanan

- Payload HANYA diekstrak dari biner `indk` itu sendiri (bukan file
  eksternal) -- tidak ada permukaan serang baru dari input luar.
- Ekstraksi tar: guard path traversal yang sama dengan pola
  `pustaka/web.idk` (realpath check sebelum tulis).
- Bundel TIDAK menyimpan secret/kredensial -- itu tanggung jawab
  developer skrip, didokumentasikan eksplisit (sama seperti peringatan
  kredensial hardcode di contoh lain).

## 5. Pengujian

- `tests/bundel_aot.sh`: bangun bundel dari `examples/hello.idk`, jalankan
  TANPA argumen, bandingkan stdout dengan `indk examples/hello.idk`
  langsung -- harus identik.
- Uji skrip yang `impor` modul lain (`examples/closure.idk` dkk) untuk
  membuktikan payload multi-file terekstrak benar.
- Uji biner `indk` BIASA (tanpa payload ditempel) tetap jalan seperti
  sebelumnya -- regresi nol untuk pemakaian normal.
- Lintas OS: path separator, dan di Windows payload ditempel ke `.exe`
  (PE tetap valid dengan data tertempel di akhir, pola yang sama dipakai
  installer self-extracting Windows lain).

## 6. Dampak dokumentasi

- [ ] `website/panduan/10_BUNDEL_EXECUTABLE.md` baru
- [ ] `docs/ROADMAP.md` Fase 8 baris AOT dicentang saat implementasi + tes lulus
- [ ] `CHANGELOG.md`

## 7. Risiko & alternatif

- **Ukuran file**: setiap bundel = ukuran `indk` penuh (beberapa MB) +
  payload kecil -- lebih besar dari skrip `.idk` telanjang, tapi itu
  trade-off yang sama dibuat tiap bahasa yang bundling runtime (Go,
  single-file Python). Didokumentasikan apa adanya, bukan dijanjikan kecil.
- **Code signing** (Windows SmartScreen, macOS Gatekeeper) -- di luar
  cakupan ini, sama seperti status `.exe`/`.deb` installer sekarang
  (belum ditandatangani, dicatat sebagai batas yang disadari).
