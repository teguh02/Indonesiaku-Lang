# Rencana Fitur: batas waktu soket (WEB-07, bagian pertama)

- **Status:** selesai
- **Target versi:** 1.5.0
- **Pemilik:** teguh02

## 1. Masalah

Server HTTP Indonesiaku berjalan satu-per-satu di atas satu gelung
`soket_terima()`. Tidak ada satu pun batas waktu pada soket, sehingga **satu
klien yang menyambung lalu diam membekukan seluruh server selamanya**.

Dibuktikan sebelum perbaikan, memakai `tests/bantu/server_web.idk`:

```text
sebelum serangan      : HTTP/1.1 200 OK
saat 1 klien diam     : GAGAL: TimeoutError  (6.0s)
setelah klien ditutup : HTTP/1.1 200 OK
```

Klien penyerang tidak mengirim satu byte pun — ia hanya `connect()` lalu
menunggu. `soket_baca()` memanggil `recv()` yang memblokir tanpa batas, dan
`accept()` berikutnya tidak pernah tercapai. Ini serangan slowloris klasik, dan
biayanya bagi penyerang **satu koneksi**.

Ini prasyarat Fase 11 langkah 6 (WEB-07). Model worker/thread tidak
menghilangkan masalahnya: tanpa batas waktu, N koneksi diam membekukan N
worker. Batas waktu harus ada lebih dulu.

## 2. Sintaks usulan

```python
impor web

variabel server = soket_dengar(8080)
variabel klien = soket_terima(server)

# Batas waktu baca/tulis untuk SATU soket, dalam detik (desimal boleh).
soket_batas_waktu(klien, 5)

coba {
    variabel mentah = web.baca_permintaan(klien)
    soket_tulis(klien, app.tangani(mentah))
} kecuali e {
    # Klien lambat/diam: tutup saja, server tetap melayani yang lain.
    cetak("koneksi dilewati:", e)
}
soket_tutup(klien)
```

Tidak ada kata kunci baru. Hanya satu builtin baru, `soket_batas_waktu`,
mengikuti pola nama `db_batas_tunggu()` yang sudah ada.

## 3. Semantik

- `soket_batas_waktu(soket, detik)` → `kosong`. Mengatur batas waktu untuk
  operasi baca **dan** tulis pada soket itu (`SO_RCVTIMEO` + `SO_SNDTIMEO`).
- `detik` berupa angka ≥ 0. `0` berarti **tanpa batas** (perilaku lama, sesuai
  konvensi POSIX), sehingga program versi 1.4.x tidak berubah perilakunya.
- `detik` negatif, bukan angka, atau soket yang sudah tertutup → galat yang
  **bisa ditangkap** `coba/kecuali` (`nativeRaise`), bukan fatal.
- Ketika batas waktu terlampaui, `soket_baca()`/`soket_tulis()` menaikkan galat
  yang bisa ditangkap, **bukan** mengembalikan `""`. `""` sudah berarti "klien
  menutup koneksi dengan sopan"; menyamakan keduanya membuat `baca_permintaan()`
  mengembalikan permintaan separuh seolah-olah lengkap, dan server akan
  menjawab 400 untuk klien yang sebenarnya cuma lambat.
- Bawaan tetap tanpa batas: builtin ini tidak mengubah soket yang tidak
  disetel. Perubahan default adalah keputusan terpisah agar tidak memecah
  kompatibilitas.
- `web.baca_permintaan()` tidak menangkap galat ini; ia merambat ke pemanggil,
  yang bertanggung jawab menutup soket. Pustaka tidak boleh diam-diam menelan
  batas waktu.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/scanner.c` | — |
| `src/compiler.c` | — |
| `src/vm.c` | registrasi `defineNative("soket_batas_waktu", ...)` |
| `src/native.c` | `nativeFnSoketBatasWaktu`; `soket_baca`/`soket_tulis` membedakan timeout dari galat lain |
| `src/native.h` | deklarasi |
| `src/value.c` / `src/object.c` | — (tidak ada tipe objek baru) |
| `src/debug.c` | — (tidak ada opcode baru) |

Catatan platform: POSIX memakai `struct timeval`, Winsock memakai `DWORD`
milidetik untuk opsi yang sama. Dua cabang `#ifdef _WIN32`, bukan satu.

Deteksi timeout: POSIX `recv()` mengembalikan `-1` dengan `errno`
`EAGAIN`/`EWOULDBLOCK`; Winsock mengembalikan `SOCKET_ERROR` dengan
`WSAETIMEDOUT`. Keduanya harus dibedakan dari galat soket sungguhan, karena
pesan galatnya berbeda bagi pengguna.

GC-safety: tidak ada alokasi baru selain `ObjString` pesan galat yang sudah
ditangani jalur `nativeRaise` yang ada. Tidak ada objek tak ter-root.

## 5. Pengujian

- `tests/web_batas_waktu_server.sh`: server nyata dengan `soket_batas_waktu`,
  lalu **koneksi diam sungguhan** dari shell (`/dev/tcp`), dan dibuktikan
  bahwa permintaan berikutnya tetap dijawab. Inilah properti yang sebenarnya
  diuji — bukan sekadar "builtin bisa dipanggil".
- Golden: `examples/soket_batas_waktu.idk` + `tests/expected/soket_batas_waktu.out`
  (argumen salah ditangkap `coba/kecuali`; tidak membuka port supaya
  deterministik dan aman di CI).
- Tes galat: `tests/galat/soket_batas_waktu_tipe.idk` + `.err`.
- Kasus batas wajib: detik `0` (tanpa batas, kompatibilitas lama), detik
  desimal (`0.2`), detik negatif, argumen bukan angka, soket sudah ditutup,
  dan **timeout tidak boleh disamakan dengan koneksi tertutup**.
- Setiap pemeriksaan baru disabotase dulu; yang tidak bisa merah dibuang atau
  diperbaiki.
- Jalankan ASan/UBSan untuk golden + `tests/galat/` karena ada perubahan C.

## 6. Dampak dokumentasi & tooling

- [ ] `website/panduan/03_PANDUAN_SINTAKS.md` — tidak berubah (tanpa sintaks baru)
- [x] `website/panduan/07_PUSTAKA_STANDAR.md`
- [x] `editors/vscode-indonesiaku/language-data.js` — BUILTINS
- [x] `editors/vscode-indonesiaku/syntaxes/indonesiaku.tmLanguage.json`
- [ ] `editors/vscode-indonesiaku/snippets/indonesiaku.code-snippets` — tidak perlu
- [ ] `README.md` — tidak perlu (bukan sintaks yang terlihat pengguna baru)
- [x] `CHANGELOG.md` + `docs/ROADMAP.md` + `docs/ANALISIS/02_PETA_FITUR_PYTHON.md`
- [ ] `AGENTS.md` — tidak diedit
- [x] Berkas rencana ini: status → `selesai`, lalu `git mv` ke `selesai/`

## 7. Risiko & alternatif

- **Risiko kompatibilitas: tidak ada.** Soket tanpa `soket_batas_waktu()`
  berperilaku persis seperti 1.4.x. Menjadikan batas waktu aktif secara bawaan
  memang lebih aman, tetapi akan memutus program lama yang menahan koneksi
  lama dengan sengaja; itu keputusan tersendiri.
- **Alternatif: soket non-blocking + `select()`/`poll()`.** Lebih kuat dan
  memang arah akhir WEB-07, tetapi menuntut gelung event penuh di `pustaka/`
  dan mengubah bentuk API server. `SO_RCVTIMEO` menutup lubang slowloris
  sekarang dengan perubahan jauh lebih kecil, dan tidak menghalangi jalur
  `select()` nanti.
- **Alternatif: thread per koneksi.** Tanpa batas waktu, ini hanya menaikkan
  jumlah koneksi diam yang dibutuhkan penyerang dari 1 menjadi N — bukan
  perbaikan, dan VM ini belum punya model thread.
- **Yang TIDAK diselesaikan rencana ini:** batas jumlah koneksi, backpressure,
  shutdown anggun, dan log terstruktur — sisa WEB-07 lainnya. Jangan mengklaim
  langkah 6 selesai karena berkas ini selesai.
