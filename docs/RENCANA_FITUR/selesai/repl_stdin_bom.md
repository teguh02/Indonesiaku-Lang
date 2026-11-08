# Rencana: BOM UTF-8 di awal kode REPL dari pipa (T-W12)

- **Status:** selesai
- **Target versi:** 1.6.0
- **Jenis:** perbaikan (REPL)
- **Pemilik:** Rafa Okta Habibilah
- **Temuan asal:** T-W12 (`docs/TESTING/WINDOWS.md` §7, "Temuan baru sesudah
  `1.6.0-rc.1`"), ditemukan saat mencoba paket installer 1.6.0-rc.1

## 1. Masalah

```powershell
PS> $OutputEncoding = [Text.Encoding]::UTF8
PS> 'x = 21', 'cetak(x * 2)' | indk
[baris 1] Kesalahan: Karakter tidak dikenal.
Variabel tidak terdefinisi 'x'.
```

Kode yang sama sebagai berkas `.idk` berjalan.

## 2. Akar masalah

Windows PowerShell 5.1 menulis preamble encoding pipa ke stdin program
native. Byte yang benar-benar dikirim, dilihat dengan `od -t x1`:

| Kondisi | Awal stdin |
|---|---|
| Sesi PowerShell yang dijalankan Claude Code di mesin uji, `$OutputEncoding` tidak diubah (`[Console]::InputEncoding` ber-preamble 3 bita) | `EF BB BF 78 ...` (satu BOM) |
| Setelah `$OutputEncoding = [Text.Encoding]::UTF8` | `EF BB BF EF BB BF 78 ...` (**dua** BOM) |

`readFile()` (`src/main.c`) sudah memanggil `stripUtf8Bom()` sejak T-W2,
tetapi `repl()` membaca lewat `readLineAlloc()` tanpa pembersihan itu, jadi
pemindai menolak U+FEFF di baris 1.

## 3. Semantik

Dibandingkan langsung dengan Python 3.14.7 di mesin yang sama:

| Masukan lewat pipa | Python | indk sebelum | indk sesudah |
|---|---|---|---|
| Kode + 1 BOM | jalan (`42`) | gagal | jalan |
| Kode + 2 BOM | `SyntaxError` U+FEFF | gagal | **jalan** |
| BOM di baris ke-2 kode | `SyntaxError` | gagal | gagal (tetap) |
| `input()` + 1 BOM | U+FEFF dipertahankan | dipertahankan | dipertahankan (tetap) |

- **Yang dibuang:** BOM berturut-turut di awal **sesi** REPL (baris pertama
  saja). REPL membaca kode sumber, sama seperti `readFile()`.
- **Penyimpangan sadar dari Python, satu:** dua BOM berturut-turut. Python
  berhenti di satu, sehingga kasus nyata `$OutputEncoding =
  [Text.Encoding]::UTF8` (saran paling umum di internet, dan kasus yang
  menemukan bug ini) tetap rusak di sana. U+FEFF di awal kode tidak pernah
  bermakna, jadi membuang deretannya tidak menghilangkan apa pun.
- **`input()` tidak diubah.** Python mempertahankan U+FEFF, dan maintainer
  sudah mencatat keputusan yang sama di WIN-12 hasil uji 1.6.0-rc.1
  ("Python 3.14 berperilaku persis sama, jadi bukan bug indk"). Versi awal
  perbaikan ini sempat membuang BOM di tingkat stream (ikut `input()` dan
  debugger awakutu); dibatalkan karena bertentangan dengan paritas Python dan
  keputusan itu.
- BOM di tengah sesi tetap karakter liar, seperti di tengah berkas.

## 4. Implementasi

| Bagian | Perubahan |
|---|---|
| `src/main.c` `repl()` | baris pertama sesi: `stripUtf8Bom()` diulang sampai tidak ada BOM lagi |

Tidak ada alokasi baru; `stripUtf8Bom()` memindahkan isi baris di tempat.

## 5. Pengujian

- `tests/test_stdin_bom.sh` (portabel, BOM disimulasikan dengan `printf`),
  didaftarkan di `tests/test_all.sh`, daftar Git Bash Windows
  `.github/workflows/test.yml`, dan daftar wajib `tests/cakupan_lintas_os.sh`.
  Kasus: tanpa BOM, 1 BOM, 2 BOM + CRLF (persis byte PowerShell), BOM di
  depan blok multi-baris, BOM di baris ke-2 tetap ditolak, hanya BOM, dan
  `input()` mempertahankan U+FEFF.
- **Sabotase:** kode lama → 4 merah; buang BOM di **setiap** baris → 1 merah
  (baris ke-2); buang hanya **satu** BOM → 1 merah (dua BOM).
- **Verifikasi nyata:** Windows PowerShell 5.1.26100 dengan `$OutputEncoding
  = [Text.Encoding]::UTF8` → `42`. Versi pertama perbaikan (satu BOM) lulus
  tes bash tetapi **gagal** di PowerShell sungguhan; dari situlah kasus dua
  BOM ditemukan.

## 6. Dampak dokumentasi

- [x] `CHANGELOG.md`
- [x] `docs/TESTING/WINDOWS.md` — temuan T-W12 (✅), kasus WIN-19
- [ ] `website/panduan/` — tidak perlu: perilaku yang terlihat pengguna
      hanyalah "kode dari pipa sekarang jalan"

## 7. Risiko & alternatif

- *Alternatif: buang BOM di tingkat stream stdin (`readLineAlloc`).* Ditolak,
  lihat §3 (`input()`).
- *Alternatif: tepat satu BOM, paritas Python penuh.* Ditolak: tidak menolong
  kasus PowerShell yang memicu temuan ini.
