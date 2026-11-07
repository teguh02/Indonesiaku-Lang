# Rencana Fitur: autentikasi, hashing sandi, dan otorisasi peran

- **Status:** selesai
- **Target versi:** 1.6.0
- **Pemilik:** Teguh R

## 1. Masalah

Sesi (WEB-04) dan CSRF (WEB-05) sudah ada, tetapi **tidak ada cara menyimpan
sandi**. Hari ini penulis aplikasi hanya punya dua pilihan, dan keduanya
salah:

```python
# 1. sandi mentah di database
jika pengguna["sandi"] == sandi_masuk { ... }

# 2. hash cepat — SHA-256 tanpa garam
jika pengguna["sandi"] == teks_heks(sha256(sandi_masuk)) { ... }
```

Pilihan kedua terlihat aman tapi tidak: SHA-256 dirancang **cepat**. GPU
modern menghitung miliaran SHA-256 per detik, jadi basis data yang bocor dapat
dipecahkan dengan daftar kata. Tanpa garam, satu tabel pelangi memecahkan
seluruh kolom sekaligus, dan dua pengguna bersandi sama terlihat jelas karena
hash-nya identik.

`pbkdf2_sha256()` sudah tersedia sebagai builtin, tapi memakainya dengan benar
menuntut keputusan yang mudah salah: berapa iterasi, garam dari mana, bagaimana
menyimpan parameternya agar bisa dinaikkan nanti, dan bagaimana
membandingkannya tanpa membocorkan waktu.

Selain itu belum ada:

- pembatasan percobaan masuk — penyerang boleh mencoba sandi tanpa batas;
- cara membedakan peran (admin vs pengguna biasa) pada penangan rute.

## 2. Sintaks usulan

```python
impor web

# Pendaftaran
variabel hash = web.hash_sandi("rahasia123")

# Masuk
jika web.periksa_sandi("rahasia123", hash) {
    variabel sid = toko.buat({"pengguna": "ani", "peran": "admin"})
}

# Otorisasi
variabel penjaga = web.wajib_peran(toko, ["admin"])

app.rute("POST", "/hapus", fungsi(req) {
    variabel tolak = penjaga(req)
    jika tolak != kosong { kembalikan tolak }
    kembalikan web.teks(200, "terhapus")
})

# Pembatasan percobaan
variabel batas = web.PembatasPercobaan(5, 300)   # 5 gagal -> kunci 300 detik
jika batas.terkunci("ani") {
    kembalikan web.teks(429, "terlalu banyak percobaan")
}
```

Tidak ada kata kunci bahasa baru; seluruhnya pustaka `.idk`.

## 3. Semantik

### `hash_sandi(sandi, iterasi = 600000)`

Mengembalikan satu teks yang memuat **seluruh** parameter:

```
pbkdf2_sha256$600000$<garam heks>$<turunan heks>
```

Formatnya menyimpan algoritme dan jumlah iterasi di dalam hash, bukan di
konfigurasi. Alasannya praktis: ketika iterasi dinaikkan tahun depan, hash
lama tetap dapat diverifikasi dengan parameternya sendiri — tanpa itu,
menaikkan iterasi akan mengunci seluruh pengguna lama di luar.

Garam 16 byte dari `acak_bita()`, berbeda untuk setiap sandi. Garam itulah
yang membuat tabel pelangi tak berguna dan menyembunyikan dua pengguna
bersandi sama.

**600.000 iterasi** mengikuti rekomendasi OWASP untuk PBKDF2-HMAC-SHA256.
Terukur di mesin pengembangan: 100k = 0,14 s, 210k = 0,32 s, **600k = 0,79 s**.

Angka itu disengaja mahal — itulah pertahanannya — tetapi punya konsekuensi
yang harus ditulis terang: **server ini melayani satu permintaan pada satu
waktu**, jadi setiap login memblokir server ~0,8 detik. Parameter `iterasi`
dapat diturunkan bila beban menuntut, dengan kesadaran bahwa itu memperlemah
pertahanan. Solusi sebenarnya adalah pekerja paralel (Fase 20).

### `periksa_sandi(sandi, hash)`

Mengurai parameter dari hash tersimpan, menghitung ulang, lalu membandingkan
dengan `_web_banding_tetap` (sudah ada dari CSRF). Hash rusak/format asing →
`salah`, bukan galat: masukan dari basis data tidak boleh menjatuhkan server.

### `perlu_hash_ulang(hash, iterasi = 600000)`

`benar` bila hash dibuat dengan iterasi lebih rendah. Dipanggil setelah login
berhasil, saat sandi mentah masih di tangan, untuk menaikkan biaya secara
bertahap tanpa memaksa reset massal.

### `wajib_peran(toko, daftar_peran)`

Penjaga seperti `wajib_csrf`: `kosong` bila boleh lanjut, respons **401**
bila tidak ada sesi (belum masuk), **403** bila sesi ada tetapi perannya tidak
termasuk (sudah masuk, tidak berhak). Membedakan keduanya penting karena
tindakan kliennya berbeda: 401 → masuk dulu, 403 → jangan coba lagi.

### `PembatasPercobaan(maks, jeda)`

- `gagal(kunci)` menambah hitungan;
- `berhasil(kunci)` mengosongkan;
- `terkunci(kunci)` → `benar` bila hitungan ≥ `maks` dan jeda belum lewat.

Kunci diserahkan pemanggil (nama pengguna, IP, atau gabungan) karena hanya
aplikasi yang tahu mana yang bermakna di belakang proxy.

### Yang sengaja TIDAK dikerjakan

- **Argon2id.** Roadmap menyebut "utamakan Argon2id **jika dependensi
  tersedia**". Tidak tersedia: menambahkannya berarti menautkan pustaka C
  pihak ketiga ke proyek yang hari ini hanya butuh libc + mbedTLS, untuk
  keuntungan yang nyata tapi tidak genting ketika PBKDF2 ber-600k iterasi
  sudah dipakai. Dicatat sebagai pilihan sadar, bukan kelalaian.
- **Penyimpanan pengguna.** Aplikasi memakai basis data Fase 10; pustaka tidak
  perlu memaksakan skema tabel.
- **Pembatas tersimpan di disk.** Hitungan di memori hilang saat restart —
  cukup untuk melambatkan brute force, dan satu-satunya yang benar sebelum ada
  penyimpanan bersama.

## 4. Berkas tersentuh

| Berkas | Perubahan |
|---|---|
| `pustaka/web.idk` | `hash_sandi`, `periksa_sandi`, `perlu_hash_ulang`, `wajib_peran`, `PembatasPercobaan` |
| `examples/web_auth.idk` + `tests/expected/web_auth.out` | golden |
| `tests/web_auth.sh` + `tests/bantu/server_auth.idk` | login/logout/peran lewat HTTP nyata |
| `tests/test_all.sh`, `.github/workflows/test.yml` | daftarkan sub-suite |
| `website/panduan/07_PUSTAKA_STANDAR.md` | dokumentasi |
| `docs/TESTING/*.md` | kasus uji ketiga OS |
| `CHANGELOG.md`, `docs/ROADMAP.md` | status WEB-06 |

## 5. Rencana uji

**Golden:** sandi sama menghasilkan hash berbeda (garam acak); verifikasi
benar/salah; hash rusak ditolak tanpa galat; `perlu_hash_ulang`; peran;
pembatas mengunci lalu membuka setelah `berhasil()`.

**Tes HTTP** (`tests/web_auth.sh`): daur penuh masuk → akses → keluar;
sandi salah ditolak; halaman admin ditolak untuk peran biasa (**403**) dan
untuk yang belum masuk (**401**); pembatas menjawab 429 sesudah 5 kegagalan.

**Sabotase wajib:**

- `periksa_sandi` selalu `benar`;
- garam dibuat tetap (hash dua sandi sama jadi identik);
- perbandingan memakai `==` **dan** hanya awalan;
- iterasi diabaikan saat verifikasi;
- `wajib_peran` meloloskan semua peran;
- pembatas tidak pernah mengunci.
