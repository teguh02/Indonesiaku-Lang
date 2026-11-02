# Rencana Fitur: gelung server (WEB-07, sisa)

- **Status:** selesai
- **Target versi:** 1.5.0
- **Pemilik:** teguh02

## 1. Masalah

`pustaka/web.idk` menyediakan routing, middleware, parsing badan, dan berkas
statis — tetapi **tidak menyediakan gelung servernya**. Setiap pemakai menulis
sendiri gelung soket mentah. Hari ini ada lima salinan:

```text
examples/todo_server.idk
tests/bantu/server_web.idk
tests/bantu/server_badan.idk
tests/bantu/server_pengalihan.idk
tests/bantu/server_batas_waktu.idk
```

Akibatnya setiap properti operasional harus dipasang lima kali, dan nyatanya
tidak:

- **Batas waktu**: hanya 1 dari 5 memanggil `soket_batas_waktu()`. Empat
  sisanya masih bisa dibekukan satu klien yang diam.
- **Shutdown anggun**: tidak ada. `examples/todo_server.idk` memakai rute
  `/berhenti` buatan sendiri; selebihnya dihentikan dengan `kill`, sehingga
  soket server tidak pernah `soket_tutup()`.
- **Log**: `cetak()` ad-hoc dengan format berbeda-beda, atau tidak ada sama
  sekali. Tidak ada cara tahu permintaan mana yang dilayani atau gagal.
- **Galat satu koneksi**: sebagian membungkus `coba`, sebagian tidak. Yang
  tidak, mati total gara-gara satu permintaan rusak.

Ini sisa langkah 6 Fase 11 (**WEB-07**). Menambal lima salinan adalah diff yang
lebih besar daripada menyediakan satu gelung yang benar.

ROADMAP memang sudah menargetkan API ini: `app.jalankan(port=8080)`.

## 2. Sintaks usulan

```python
impor web

variabel app = web.Aplikasi()
app.rute("GET", "/halo", fungsi(req) { kembalikan web.teks(200, "halo") })

# Gelung server: batas waktu, penanganan galat per koneksi, log, dan
# penutupan soket sudah termasuk.
app.jalankan(8080)
```

Opsi, semuanya punya bawaan aman:

```python
app.jalankan(8080, batas_waktu = 10, maks_permintaan = 0, log = benar)
```

Menghentikan server dari dalam penangan:

```python
app.rute("POST", "/matikan", fungsi(req) {
    app.berhenti()
    kembalikan web.teks(200, "berhenti setelah balasan ini terkirim")
})
```

Tidak ada kata kunci baru. Dua metode baru pada `Aplikasi`: `jalankan()` dan
`berhenti()`.

## 3. Semantik

- `jalankan(port, batas_waktu = 10, maks_permintaan = 0, log = benar)` memblokir
  sampai server berhenti, lalu mengembalikan jumlah koneksi yang dilayani.
- **`batas_waktu` bawaan 10 detik, bukan tanpa batas.** Berbeda dari builtin
  `soket_batas_waktu()` yang bawaannya `0` demi kompatibilitas: `jalankan()`
  adalah API baru, tidak ada program lama yang bisa rusak, jadi bawaannya
  dipilih yang aman. `0` tetap berarti tanpa batas bagi yang memang mau.
- `maks_permintaan = 0` berarti tanpa batas (melayani selamanya). Angka positif
  membuat gelung berhenti setelah sekian koneksi — dipakai tes supaya server
  berakhir sendiri, bukan dibunuh.
- **Galat pada satu koneksi tidak boleh mematikan server.** Setiap iterasi
  dibungkus `coba`; galat dicatat lalu koneksi ditutup. Ini termasuk galat
  batas waktu, permintaan rusak, dan galat yang dinaikkan penangan pengguna.
- `soket_tutup(klien)` dijalankan di `akhirnya`, jadi soket tidak bocor pada
  jalur galat mana pun.
- `berhenti()` menandai gelung supaya berhenti **setelah** balasan saat ini
  terkirim — bukan memutus koneksi di tengah jalan. Itulah arti "anggun" yang
  bisa dijamin di sini.
- Setelah gelung berakhir, soket server ditutup. Dengan `maks_permintaan` atau
  `berhenti()`, port benar-benar dilepas; dengan `kill` tidak, dan itu batasan
  yang didokumentasikan.
- `log = benar` menulis satu baris per koneksi ke stdout dengan format tetap:
  `waktu tingkat metode path status durasi_ms`. Terstruktur dalam arti
  **field-nya tetap dan dapat diurai**, bukan JSON.
- Log tidak pernah memuat badan permintaan (bisa berisi kata sandi) dan tidak
  memuat header.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/*.c` | **tidak ada** — semua builtin yang dibutuhkan sudah ada |
| `pustaka/web.idk` | metode `jalankan()`, `berhenti()`, pembantu `_log_baris()` |
| `examples/todo_server.idk` | pakai `app.jalankan()`, hapus gelung salinan |
| `tests/bantu/*.idk` | pakai `app.jalankan(..., maks_permintaan = N)` |

Tidak ada perubahan C, jadi tidak ada risiko GC baru. Ini murni pustaka.

**Yang TIDAK dikerjakan, dan alasannya:** eksekusi paralel sungguhan (thread
atau proses pekerja). VM ini satu utas dan tidak punya primitif thread;
menambahkannya berarti mengubah GC, stack VM, dan tabel global — pekerjaan
sebesar fase tersendiri, bukan satu langkah. Karena itu **backpressure dan
batas worker juga tidak relevan di sini**: tanpa eksekusi paralel, antrean
sudah dibatasi backlog `listen()` dan permintaan dilayani satu per satu.
Roadmap harus tetap jujur soal ini, dan `jalankan()` tidak boleh
diiklankan setara server produksi.

## 5. Pengujian

- `tests/web_jalankan_server.sh`: server nyata memakai `app.jalankan()`.
  Membuktikan, lewat HTTP sungguhan:
  1. permintaan normal dilayani;
  2. **klien diam tidak membekukan server** (batas waktu aktif secara bawaan,
     tanpa disetel pemakai);
  3. **permintaan yang membuat penangan menaikkan galat tidak mematikan
     server** — permintaan berikutnya tetap dijawab;
  4. `berhenti()` menutup server **setelah** balasan terkirim (klien tetap
     menerima balasannya), dan port benar-benar dilepas sesudahnya;
  5. baris log muncul dengan field yang diharapkan.
- Golden: `examples/web_jalankan.idk` + `.out` — validasi argumen saja, tanpa
  membuka port, supaya deterministik di CI.
- Tes galat: `tests/galat/web_jalankan_port.idk` + `.err`.
- Setiap pemeriksaan disabotase dulu; yang tidak bisa merah dibuang.
- Lima pemakai lama diubah memakai `jalankan()`; seluruh suite harus tetap
  hijau — itu buktinya gelung baru setara dengan yang digantikan.

## 6. Dampak dokumentasi & tooling

- [ ] `website/panduan/03_PANDUAN_SINTAKS.md` — tidak berubah (tanpa sintaks baru)
- [x] `website/panduan/07_PUSTAKA_STANDAR.md`
- [ ] `editors/vscode-indonesiaku/` — tidak berubah (metode, bukan builtin baru)
- [ ] `README.md` — tidak perlu
- [x] `CHANGELOG.md` + `docs/ROADMAP.md`
- [ ] `AGENTS.md` — tidak diedit
- [x] Berkas rencana ini: status → `selesai`, lalu `git mv` ke `selesai/`

## 7. Risiko & alternatif

- **Risiko kompatibilitas: rendah.** `jalankan()` metode baru; gelung tulisan
  tangan yang lama tetap berfungsi. Yang berubah hanyalah contoh dan berkas
  bantu di repo ini sendiri.
- **Risiko bawaan 10 detik:** klien yang sah tapi sangat lambat akan diputus.
  Itu sebabnya nilainya dapat diatur, dan `0` tetap tersedia.
- **Alternatif: biarkan tiap pemakai menulis gelungnya.** Itu keadaan sekarang,
  dan hasilnya empat dari lima server rentan dibekukan satu klien diam.
- **Alternatif: thread per koneksi.** Lihat bagian 4 — VM belum mampu, dan
  tanpa batas waktu pun hanya menaikkan biaya penyerang dari 1 koneksi ke N.
