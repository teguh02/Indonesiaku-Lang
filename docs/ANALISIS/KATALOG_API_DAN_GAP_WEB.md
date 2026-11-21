# Katalog API dan Gap Pengembangan Web (Fase 19, langkah 1)

**Tanggal audit:** 2026-11-21, terhadap `main` @ `3b5434c` (v1.7.0 + Fase 14, 15,
18, 22, 24) ditambah modul yang ditambahkan bersama dokumen ini.
**Cara audit:** membaca registrasi builtin di `src/vm.c` (`defineNative`),
fungsi publik `pustaka/*.idk`, contoh `examples/`, golden `tests/expected/`,
pasangan `tests/galat/` dan skrip integrasi `tests/*.sh`. Setiap klaim
"ada" di bawah ini menunjuk ke kode **dan** tes; bila tes tidak ditemukan,
statusnya paling tinggi "sebagian".

Legenda status: **ada** (kode + tes + dokumentasi) · **sebagian** (sebagian
kemampuan ada, atau ada tanpa tes/dokumentasi yang cukup) · **belum** ·
**sengaja tidak diadopsi** (keputusan desain, bukan kekurangan).

Matriks ini memetakan KEMAMPUAN, bukan nama fungsi PHP. Satu halaman
W3Schools tidak sama dengan satu fitur baru.

## 1. Dasar bahasa (PHP Basics)

| Kemampuan | Status | Implementasi | Tes |
|---|---|---|---|
| Teks: panjang, huruf besar/kecil, ganti, potong, format, f-string | ada | `src/native.c` (`panjang`, `huruf_besar`, `ganti`, `potong`, `format`), metode teks di `src/vm.c` `stringMethods` | `examples/fungsi_string.idk`, `literal_fstring.idk` |
| Teks: cari/pisah/pangkas/ulang/balik | ada (modul lama berawalan) | `pustaka/teks.idk` (`teks_cari`, `teks_pisah`, `teks_pangkas`, ...) | `pustaka_teks` |
| Teks Unicode: huruf besar/kecil penuh, lipat huruf, spasi Unicode | ada | `pustaka/unicode.idk` (Fase 24) | `pustaka_unicode` |
| Ekspresi reguler | ada | `src/pola.c` + `pustaka/pola.idk` | `pustaka_pola` |
| Bilangan bulat/desimal, pembulatan, basis | ada | `bulat`, `bulatkan`, `biner`, `oktal`, `heksa`, `ke_bulat` | `operator_aritmetika`, `konversi`, `fungsi_math` |
| `ke_bulat("0x41")` (teks berbasis 16) | belum | `ke_bulat` hanya menerima desimal | — (ditemukan saat menulis `pustaka/xml.idk`) |
| Matematika | ada | builtin trigonometri + `pustaka/matematika.idk`, `pustaka/statistik.idk` | `pustaka_matematika`, `pustaka_statistik` |
| Daftar/kamus/himpunan/tupel, irisan, komprehensi | ada | VM + `src/native.c` | `list`, `kamus`, `himpunan`, `tupel`, `komprehensi` |
| Koleksi tingkat tinggi (Penghitung, KamusBawaan, Antrian) | ada | `pustaka/koleksi.idk` | `pustaka_koleksi` |
| filter/map/reduce, pengelompokan | ada | `peta`, `saring`, `pustaka/fungsional.idk`, `pustaka/iterasi.idk` | `pustaka_fungsional`, `pustaka_iterasi` |
| Pengurutan dengan kunci/objek | ada | `terurut` (+ `_kurang_dari_`) | `oop/urut_objek_reentran` |
| `daftar(teks)` linear untuk teks non-ASCII | **diperbaiki di Fase 19** | `nativeFnDaftar` kini berjalan linear; sebelumnya O(n²) (120 ribu karakter = 9 detik) | `pustaka_xml` (dokumen multibahasa) |

Kesimpulan: tidak ada gap fungsional besar di dasar bahasa. Gap kecil yang
tercatat: teks heksadesimal ke bulat. Iterasi `untuk c dalam teks` dan
`teks[i]` pada teks non-ASCII masih O(n) per akses (lihat
`utf8ByteOffset`); pustaka yang memproses teks panjang sebaiknya memakai
`daftar(teks)` sekali.

## 2. Formulir web (PHP Forms)

| Kemampuan | Status | Implementasi | Tes |
|---|---|---|---|
| Query string, `application/x-www-form-urlencoded` | ada | `pustaka/web.idk` `_web_urai_query`, `req["query"]`, `req["form"]` | `web_badan`, `tests/web_badan_server.sh` |
| `multipart/form-data` (unggah berkas) | ada | `_web_urai_multipart`, `req["berkas"]` | `tests/web_badan_server.sh` |
| Escaping keluaran HTML | ada | `pustaka/templat.idk` (auto-escape, `aman()`), `html()` | `templat_dasar`, `tests/templat_sabotase.sh` |
| CSRF | ada | `web.token_csrf`, `periksa_csrf`, `wajib_csrf` | `web_csrf`, `tests/web_csrf.sh` |
| Field wajib, panjang, angka/rentang, email, URL, error per-field | **ada (Fase 19)** | `pustaka/validasi.idk` | `pustaka_validasi`, `tests/galat/validasi_sahkan` |
| Validasi skema konfigurasi | ada (domain lain) | `pustaka/konfigurasi.idk` `muat(skema)` — melempar pada galat pertama; `validasi` memakai nama atribut yang sama (`tipe`, `wajib`, `bawaan`, `pilihan`) tetapi mengumpulkan semua galat | `pustaka_konfigurasi` |
| Sanitasi HTML masukan (allow-list tag) | belum | — | — |

## 3. Fitur lanjutan (PHP Advanced)

| Kemampuan | Status | Implementasi | Tes |
|---|---|---|---|
| Waktu sistem | ada | `waktu()` (detik UTC), `tanggal()` (komponen lokal), `format_tanggal(pola)` (strftime waktu lokal saat ini) | `waktu`, `jam_waktu_cpu` |
| Parse/format ISO 8601, UTC, offset tetap, aritmetika, perbandingan | **ada (Fase 19)** | `pustaka/tanggal_waktu.idk` | `pustaka_tanggal_waktu`, `tests/galat/tanggal_*` |
| Zona waktu IANA + DST | belum (ditunda, lihat §8) | — | — |
| Kuki dan sesi | ada | `web.set_kuki`, `baca_kuki`, `TokoSesi` | `web_kuki_sesi`, `tests/web_kuki_sesi.sh` |
| Header, status, CORS, berkas statis | ada | `pustaka/web.idk` | `web_cors`, `tests/web_statis_server.sh` |
| JSON | ada | `pustaka/json.idk` | `json_pakai`, `json_ketat` |
| CSV | ada | `pustaka/csv.idk` | `pustaka_csv` |
| Konfigurasi `.env`/INI + lingkungan | ada | `pustaka/konfigurasi.idk`, `ambil_lingkungan` | `pustaka_konfigurasi*` |
| Exception bertipe, `akhirnya`, `dengan` | ada | VM | `kecuali_bertipe`, `coba_akhirnya` |
| Log terstruktur | ada | `pustaka/log.idk` | `pustaka_log`, `log_json` |
| Argumen CLI | ada | `pustaka/opsi.idk` | `pustaka_opsi` |
| Stream/iterator berkas besar | sebagian | `baca_baris`, generator `hasilkan` | `berkas` |

## 4. OOP (PHP OOP)

| Kemampuan | Status | Catatan |
|---|---|---|
| Kelas, konstruktor `init`, pewarisan ganda (C3), `super` | ada | `examples/oop/` |
| Metode ajaib (operator, `_teks_`, `_iter_`, `_masuk_`/`_keluar_`) | ada | K19, golden `metode_ajaib` |
| `@abstrak`, `@properti`, dekorator | ada | golden `dekorator`, `oop/` |
| Visibility `private/protected` | sengaja tidak diadopsi (V1) | konvensi awalan `_` seperti Python |
| Interface/trait | sengaja tidak diadopsi sebagai sintaks | mixin lewat pewarisan ganda + `@abstrak` |
| Destructor | belum | GC mark-sweep tidak menjamin waktu finalisasi; pakai `dengan` |

## 5. Basis data (MySQL Database)

| Kemampuan | Status | Implementasi | Tes |
|---|---|---|---|
| SQLite, parameter `?`, transaksi | ada | `src/db.c` | `db_sqlite`, `db_blob` |
| PostgreSQL, MySQL/MariaDB | ada | `pustaka/pg.idk`, `pustaka/my.idk` | `tests/pg_driver.sh`, `tests/my_driver.sh` |
| API seragam + migrasi | ada | `pustaka/db.idk`, `pustaka/migrasi.idk` | `migrasi_skema`, `tests/migrasi_sabotase.sh` |

## 6. XML (PHP XML)

| Kemampuan | Status sebelum Fase 19 | Status sekarang |
|---|---|---|
| Parser XML | belum (`grep -ri xml pustaka src` hanya menemukan tipe MIME di `web.idk` dan atribut `xmlns` SVG di `kura.idk`) | **ada**: `pustaka/xml.idk` `urai()` — elemen, atribut, teks, CDATA, rujukan karakter, namespace |
| Pembuatan/serialisasi XML dengan escaping | belum | **ada**: `buat()`, `Elemen.tambah/atur`, `bentuk()`, `lolos()` |
| Tolak DTD/entitas eksternal (XXE), entity expansion | belum | **ada**, selalu aktif; uji `pustaka_xml` + `tests/galat/xml_xxe_ditolak` |
| Batas ukuran/kedalaman/jumlah elemen/atribut | belum | **ada**; parser iteratif, uji `tests/galat/xml_kedalaman` |
| Encoding selain UTF-8 | belum | sengaja ditolak (V1) |
| Streaming/SAX, XPath, validasi skema (XSD) | belum | belum (kandidat V2) |

## 7. AJAX, filesystem, FTP, keamanan

| Kelompok | Status | Fase pemilik | Catatan audit |
|---|---|---|---|
| AJAX: endpoint JSON, CORS, status/header | ada | 11/13 | `web.json()`, `web.cors()`, `tests/web_cors.sh`; pagination/filter adalah pola aplikasi, bukan API bahasa |
| Filesystem: baca/tulis, direktori, info jalur | ada | — | `buka_berkas`, `isi_direktori`, `buat_direktori`, `pustaka/jalur.idk` |
| Filesystem: salin/pindah/rekursif/glob/berkas sementara | belum | 17 | sedang dikerjakan sesi Fase 17 |
| FTP/FTPS/SFTP | belum | 17 | — |
| Hash, HMAC, PBKDF2, AEAD, HKDF, CSPRNG | ada | 18 | `src/kripto.c`, `pustaka/kripto.idk`, `kripto_dasar`, `kripto_kat` |
| Hash sandi (PBKDF2) + penguncian percobaan | ada | 13 | `web.hash_sandi`, `PembatasPercobaan`, `tests/web_auth.sh` |
| Argon2id | belum | 18 | — |
| Sandbox eksekusi | ada | — | `--sandbox`, `tests/sandbox.sh` |

## 8. Keputusan yang dicatat di audit ini

1. **Nama modul.** `validasi`, `tanggal_waktu`, `xml` diperiksa terhadap
   `pustaka/`, builtin (`defineNative`) dan kata kunci: tidak bentrok.
   `tanggal` (builtin) dan `waktu` (builtin) sengaja **tidak** dipakai
   sebagai nama modul agar `impor tanggal` tidak menutupi builtin di
   modul pemanggil. `formulir`, `berkas`, `jaringan`, `keamanan` dari
   daftar contoh roadmap belum dibuat: kemampuannya sudah ada di
   `web.idk`/`kripto.idk` atau milik Fase 17.
2. **`validasi` tidak memanggil `konfigurasi`.** Konversi tipe di
   `konfigurasi.idk` bersifat privat (`_ubah`) dan melempar pada galat
   pertama; formulir butuh pengumpulan galat per field. Keselarasan dijaga
   lewat nama atribut skema dan nama tipe yang sama, bukan lewat kode
   bersama. Bila keduanya kelak perlu berbagi kode, pindahkan `_ubah` ke
   modul internal bersama dan uji kedua golden.
3. **Zona waktu IANA ditunda.** Tidak ada data tzdb di repo; mem-vendor
   tzdb berarti menanggung pembaruan beberapa kali setahun dan memilih
   format biner/teks lintas OS. V1 memakai offset tetap, yang tepat untuk
   WIB/WITA/WIT (tanpa DST) dan untuk stempel waktu API (UTC). Rencana
   lanjutan ada di `docs/RENCANA_FITUR/selesai/validasi_tanggal_xml.md`.
4. **DTD tidak bisa diaktifkan.** "Ditolak secara default" diwujudkan
   sebagai "selalu ditolak" pada V1: opsi untuk menyalakannya adalah
   permukaan serangan tanpa pengguna nyata saat ini.

## 9. Gap yang tersisa untuk Fase 19 (setelah V1 ini)

- Langkah 2: modul `formulir` (menggabungkan `web` + `validasi`) bila pola
  pemakaian nyata muncul.
- Langkah 4 sisa: zona waktu IANA/DST.
- Langkah 5 sisa: XML streaming/SAX, XPath sederhana, encoding non-UTF-8.
- Langkah 6: `ke_bulat` heksadesimal; akses karakter O(1) untuk teks non-ASCII.
- Langkah 7–10: contoh aplikasi per kategori, portal referensi API, uji per
  kategori di CI, gerbang rilis v1.14.0.
