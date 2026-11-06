# Roadmap — Bahasa Indonesiaku

**Satu-satunya sumber kebenaran untuk roadmap.** Versi rilis terbaru: `1.4.0`
(`INDK_VERSION` di `src/common.h`). Riwayat detail per rilis ada di
[`CHANGELOG.md`](../CHANGELOG.md).

**Visi:** bahasa bergaya Python dengan kata kunci Bahasa Indonesia —
semantik selaras dengan Python 3 sehingga apa yang dipelajari di
Indonesiaku langsung berlaku di Python, dan sebaliknya. Dasar analisis dan
alasan setiap fase ada di [`ANALISIS/`](ANALISIS/README.md).

## Prinsip desain wajib untuk seluruh fase

**Indonesiaku bukan replika identik Python.** Python adalah **referensi semantik**, bukan aturan yang harus ditiru 100%. Semua implementasi oleh manusia maupun AI agent **harus** memakai keyword dan API bahasa yang berbahasa Indonesia, serta struktur blok `{ ... }` (termasuk fungsi/kelas/metode) seperti pendekatan PHP/C. Tidak boleh mengganti atau menambah alternatif sintaks blok `:` dan indentasi Python. Lihat [spesifikasi bahasa](SPESIFIKASI.md#1a-kaidah-arsitektur-bahasa-dan-aturan-unused-kebijakan-desain) dan `AGENTS.md`.

**Arah desain khusus: pemeriksaan kode tidak digunakan (unused diagnostics), terinspirasi Go.** Tujuannya agar deklarasi variabel, impor, fungsi, dan kelas yang tidak dipakai mendapat peringatan dan dapat dihapus/dikomentari. Go sendiri tidak melarang semua fungsi atau kelas yang tidak digunakan. Pemeriksaan simbol internal dan publik harus menghormati export, pemanggilan lintas modul, callback, reflection, dan entrypoint. **Status: direncanakan; jangan mengklaim sudah tersedia atau langsung mematahkan kompatibilitas v1.x.**

**Backlog desain wajib, integrasi dengan perkakas `indk periksa`/fase developer tooling:**

1. [ ] Audit kemampuan linter yang sudah ada dan definisikan perbedaan unused variabel lokal, parameter, impor, fungsi internal, kelas internal, dan simbol global/API publik. Buat `docs/RENCANA_FITUR/pemeriksaan_simbol_tidak_dipakai.md`.
2. [ ] Implementasikan analisis scope, def-use dan, untuk fungsi/kelas, reachability yang tidak salah menandai simbol lintas modul, exported, reflection, callback atau plugin hooks.
3. [ ] Mulai dengan **warning** yang actionable (lokasi, simbol, alasan, saran hapus/komentar), aturan suppression/ignore eksplisit untuk kode yang memang sengaja tidak dipakai, serta opsi ketat bila desain dan kompatibilitas telah disepakati.
4. [ ] Uji positif/negatif, multi-file, import dinamis, API publik, class inheritance/override, closure, entrypoint dan benchmark waktu lint; cegah false positive yang mengganggu developer.
5. [ ] Dokumentasikan aturan unused dan cara memperbaikinya pada panduan penggunaan dan API linter. **Error default yang breaking baru boleh dipertimbangkan sebagai perubahan major v2.0**, bukan diam-diam disisipkan dalam patch/minor v1.x.

**Pertimbangan versi:** peringatan linter yang kompatibel dan berfitur baru dapat masuk rilis **minor v1.x** yang sesuai; perbaikan false positive dapat masuk **patch**; strict mode opsional dan teruji dapat datang melalui minor. Kewajiban error kompilasi default pada deklarasi yang sebelumnya sah perlu deprecation, migration guide, pengujian kompatibilitas dan **target major v2.0.0 bila breaking**. Prioritas implementasi boleh dinaikkan bila cocok dengan Fase 9/14/19 atau fase tooling lain tanpa menambah fase baru.

## Sudah selesai

| Versi | Isi | Bukti di repo |
|---|---|---|
| 0.1.0 | Scanner, compiler Pratt, VM stack, REPL, fungsi, `jika`/`selagi`, `cetak` | `src/` |
| 0.2.0 | Closure/upvalue, list, kamus, `untuk...dalam`, `hentikan`/`lanjut`, kelas + warisan, `naikkan`/`coba`/`kecuali`, `impor`, GC mark-sweep | `examples/closure.idk`, `examples/oop/` |
| 0.3.0 | Pustaka standar `pustaka/*.idk`, file I/O, waktu/tanggal, konversi, escape string, `argumen`, REPL multi-baris | `pustaka/`, `examples/berkas.idk` |
| 0.4.0 | JSON, HTTP, soket TCP, server REST contoh, `ord`/`chr`/`sisip`/`hapus_pada`; pesan error dengan nomor baris + jejak panggilan | `examples/todo_server.idk`, `src/vm.c` `runtimeError` |
| 1.0.0 | Fase 0–3 tuntas: stabilisasi, inti mirip Python, tipe data lengkap (`bulat`/`desimal`, tupel, himpunan, himpunan beku), modul bernamespace + paket + `__utama__`, SQLite; `SPESIFIKASI.md` normatif; contoh panduan diuji otomatis | `docs/SPESIFIKASI.md`, `tests/test_panduan.sh` |
| 1.1.0 | Fase 4 tuntas: metode ajaib lengkap (30), atribut kelas, `dengan ... sebagai`, dekornator (`@properti`, `@metode_statis`, `@metode_kelas`, `@abstrak`), pewarisan berganda + MRO C3, kelas abstrak | `examples/oop/`, `docs/RENCANA_FITUR/selesai/` |

## Fase pengembangan

Status: ⬜ belum · 🟨 dikerjakan · ✅ selesai. Setiap butir besar mendapat
berkas `RENCANA_FITUR/<nama>.md` saat mulai dikerjakan. Kode B*/K* merujuk
ke [`ANALISIS/01_KONDISI_SAAT_INI.md`](ANALISIS/01_KONDISI_SAAT_INI.md) (bug)
dan [`ANALISIS/03_KEPUTUSAN_DESAIN.md`](ANALISIS/03_KEPUTUSAN_DESAIN.md)
(keputusan desain).

## Definisi "selesai" (berlaku untuk SETIAP fase dan fitur)

Sebuah butir di bawah **tidak boleh** ditandai ✅ hanya karena kodenya jalan.
Satu butir dinyatakan selesai bila seluruhnya terpenuhi:

1. **Rencana lebih dulu di `docs/`** — `RENCANA_FITUR/<nama>.md` dari
   `TEMPLATE.md`, ditulis **sebelum** menyentuh kode, lalu dipindah ke
   `RENCANA_FITUR/selesai/` saat tuntas. Perencanaan tidak pernah ditulis di
   `README.md`, `CHANGELOG.md`, atau `website/`.
2. **Uji wajib** — contoh golden di `examples/` + `tests/expected/`, pasangan
   `.idk`/`.err` di `tests/galat/` untuk yang harus gagal, seluruh
   `tests/test_all.sh` hijau, dan build sanitizer
   (`-DDEBUG_STRESS_GC -fsanitize=address,undefined`) bila menyentuh
   alokasi/stack/koleksi/objek. Berkas golden tidak pernah diregenerasi buta.
3. **Dokumentasi pengguna ikut diperbarui** — `website/panduan/`
   (`03_PANDUAN_SINTAKS.md` untuk sintaks, `07_PUSTAKA_STANDAR.md` untuk
   builtin), plus `CHANGELOG.md` dan `README.md`. Potongan kode di panduan
   wajib benar-benar dijalankan, bukan ditulis dari ingatan.
4. **Ekstensi VS Code ikut diperbarui** — `editors/vscode-indonesiaku/`:
   keyword baru ke `extension.js` KEYWORDS + `tmLanguage.json`, builtin baru
   ke BUILTINS, sintaks baru yang layak ke `snippets/`.
5. **Status dibalik di sini dan di `ANALISIS/02_PETA_FITUR_PYTHON.md`**, dan
   pitfall baru dicatat di `AGENTS.md`.
6. **Rencana uji lintas OS diperbarui** — setiap perilaku baru yang terlihat
   pengguna (sintaks, builtin, modul, perintah/opsi CLI) mendapat kasus uji
   di **ketiga** berkas [`TESTING/WINDOWS.md`](TESTING/WINDOWS.md),
   [`TESTING/MACOS.md`](TESTING/MACOS.md), dan
   [`TESTING/LINUX.md`](TESTING/LINUX.md); bagian bersamanya harus tetap
   identik. **Sebuah fase tidak boleh ditandai ✅ sebelum ketiga berkas ini
   diperbarui**, siapa pun yang mengerjakannya — maintainer, kontributor,
   maupun AI agent.

Interpreter, uji, dokumentasi, dan tooling editor adalah **satu kesatuan** —
fitur yang menyala di terminal tapi tidak di editor, atau tidak ada di
panduan, dihitung belum selesai. Checklist lengkapnya ada di
[`AGENTS.md`](../AGENTS.md) bagian *Feature workflow*; bagian itu yang
mengikat, daftar ini ringkasannya.

### Fase 0 — Stabilisasi (target 0.5.0) — *"tidak ada jawaban salah"*

Tujuan: program pemula (halo dunia, percabangan, perulangan, list, kamus,
fungsi) selalu benar atau gagal dengan pesan jelas. Tanpa fitur besar baru.

| Butir | Ref | Status |
|---|---|---|
| Perbaiki `//` (tidak memancarkan opcode) + tambah `//=` | B1, B13 | ✅ |
| Tolak `f(x=1)` dengan error kompilasi sampai argumen bernama ada | B2 | ✅ |
| Batas rekursi 1000 (`FRAMES_MAX`); jejak error merangkum frame berulang | B3 | ✅ |
| `**` asosiatif kanan; `-2 ** 2 == -4` | B4, B5 | ✅ |
| `%` dan `//` mengikuti tanda pembagi (Python) | B6 | ✅ |
| Penugasan majemuk memakai seluruh ekspresi kanan (`x *= 2 + 3`) | B14 | ✅ |
| `tidak` lebih lemah dari perbandingan | B15 | ✅ |
| Harness tes galat `tests/galat/` (program yang harus gagal) | — | ✅ |
| `lewati` dan `akhirnya` diimplementasikan | B7 | ✅ |
| Semua galat runtime bisa ditangkap `kecuali`; handler `coba` dilepas saat `kembalikan`/`hentikan`/`lanjut` | B8 | ✅ |
| Bagi nol menjadi error | B9 | ✅ |
| Cetak desimal *shortest round-trip* | B10 | ✅ |
| `panjang`/indeks/`potong`/`ord`/`chr` per karakter Unicode | B11 | ✅ |
| `cetak(a, b)` satu baris dipisah spasi; `cetak()` baris kosong | B12 | ✅ |
| `kembalikan`/`hentikan`/`lanjut` keluar dari `coba` ber-`akhirnya` menjalankan blok akhirnya | B7 lanjutan | ✅ |
| Hapus token mati (`INDENT`/`DEDENT`/`NEWLINE`); `;` jadi pemisah pernyataan; `:` dicadangkan untuk Fase 1 | 01 §6 | ✅ |
| Selaraskan README/panduan dengan kenyataan (semua blok kode di dokumentasi diuji dengan `indk`) | 01 §6 | ✅ |
| Tes golden untuk setiap bug di atas | — | ✅ |
| Putuskan K1–K14 (keputusan desain) — semua rekomendasi disetujui | 03 | ✅ |

### Fase 1 — Inti mirip Python (0.6–0.7) — *"tutorial Python bab 1–5 bisa ditiru"*

| Butir | Ref | Status |
|---|---|---|
| Truthiness Python ([rencana](RENCANA_FITUR/selesai/kebenaran_python.md)) | K1 | ✅ |
| `rentang()` ([rencana](RENCANA_FITUR/selesai/rentang.md)); `untuk` atas teks dan rentang | K9 | ✅ |
| Operator `dalam` / `tidak dalam` ([rencana](RENCANA_FITUR/selesai/operator_dalam.md)) | 02 §A | ✅ |
| `enumerasi()`, `untuk` atas kamus | K9 | ✅ |
| `adalah` / `bukan`, rantai perbandingan (`1 < x < 3`, `a dalam b == c`) | 02 §A–B | ✅ |
| Hierarki kelas `Galat*` dan `kecuali Tipe sebagai e` (semua galat runtime sudah bisa ditangkap sejak Fase 0) | K5, 06 §9 | ✅ |
| Akhir baris mengakhiri pernyataan (di luar kurung), seperti Python | B16 | ✅ |
| Assignment di fungsi = lokal; `global`, `nonlokal` | K3 | ✅ |
| Literal kamus `{ }`, indeks negatif, irisan `[a:b:c]` | 02 §B | ✅ |
| Argumen bawaan (nilai default parameter) | K15 | ✅ |
| Argumen bernama `f(b=2)` | K16 | ✅ |
| `cetak` jadi fungsi biasa (`pisah=`, `akhir=`) ([rencana](RENCANA_FITUR/selesai/cetak_fungsi.md)) | K12 | ✅ |
| `repr` untuk elemen koleksi ([rencana](RENCANA_FITUR/selesai/repr_koleksi.md)) | K12 sisa | ✅ |
| Fungsi anonim (ekspresi `fungsi(x) { ... }`), ekspresi kondisional ([rencana](RENCANA_FITUR/selesai/fungsi_anonim_kondisional.md)) | 02 §B | ✅ |
| f-string, string `"""` multi-baris, literal `1e3`, `0x`, `0b`, `1_000` ([rencana](RENCANA_FITUR/selesai/literal_fstring.md)) | 02 §B | ✅ |
| Metode pada `teks`/`daftar`/`kamus` (P1 dari katalog 06) ([rencana](RENCANA_FITUR/selesai/metode_tipe.md)) | K10, 06 §1–3 | ✅ |
| `+`/`*` untuk teks & daftar, `==` struktural | K7 | ✅ |
| `pastikan` (assert) ([rencana](RENCANA_FITUR/selesai/pastikan.md)) | 02 §A | ✅ |
| Traceback dengan kolom + potongan baris; saran "mungkin maksudmu…" ([rencana](RENCANA_FITUR/selesai/traceback_kolom.md)) | 04 §5, 05 §5 | ✅ |
| Kolom pada galat *runtime* ([rencana](RENCANA_FITUR/selesai/kolom_runtime.md)) | 04 §5 | ✅ |

### Fase 2 — Tipe data lengkap (0.8)

✅ ``bulat`` int64 ([rencana](RENCANA_FITUR/selesai/tipe_angka_bulat.md));
✅ penugasan berganda & unpacking ([rencana](RENCANA_FITUR/selesai/penugasan_berganda.md));
✅ `buang` (del) ([rencana](RENCANA_FITUR/selesai/buang.md));
✅ kunci kamus bukan cuma teks — `Value`-based ([rencana](RENCANA_FITUR/selesai/kamus_kunci_hash.md));
✅ operator bitwise `& | ^ ~ << >>` + `&= |= ^= <<= >>=` ([rencana](RENCANA_FITUR/selesai/operator_bitwise.md));
✅ `tupel` `(1, 2)`/`(1,)`/`()` + `tupel()` ([rencana](RENCANA_FITUR/selesai/tupel.md), K17);
✅ `himpunan([...])` + `| & - ^` ([rencana](RENCANA_FITUR/selesai/himpunan.md), K17);
✅ tupel tanpa kurung — `x = 1, 2`, `kembalikan a, b` ([rencana](RENCANA_FITUR/selesai/sisa_fase2.md), K17);
✅ `himpunan_beku` — himpunan yang dapat di-hash ([rencana](RENCANA_FITUR/selesai/sisa_fase2.md));
✅ builtin `hash`, `iter`/`lanjutkan`, `turunan_dari`, `ke_desimal`, `daftar()` ([rencana](RENCANA_FITUR/selesai/sisa_fase2.md)).

> Fase 2 **selesai**. Catatan penamaan: `next` dinamai **`lanjutkan`** karena
> `lanjut` sudah menjadi kata kunci perulangan. Koma pembentuk tupel hanya
> berlaku di sisi kanan penugasan dan setelah `kembalikan`; di dalam kurung
> koma tetap berarti pemisah. `variabel a, b = ...` tetap tidak didukung —
> tulis `a, b = ...` tanpa `variabel`.

Ref: K2, K8, K11, K13, 06 §4–6.

### Fase 3 — Modul & pustaka inti (0.9) — ✅ **selesai**

✅ `ObjModule` dengan namespace sendiri per berkas;
✅ `impor x`, `impor x sebagai y`, `dari x impor y`, `dari x impor y sebagai z`;
✅ paket (`impor a.b` → `a/b.idk`);
✅ jalur pencarian (direktori pengimpor → `INDK_PATH` → `pustaka/`);
✅ `__nama__` / `__utama__`
([rencana](RENCANA_FITUR/selesai/modul_namespace.md)).
✅ **Basis data SQLite**, lihat
[`RENCANA_FITUR/selesai/basis_data_sqlite.md`](RENCANA_FITUR/selesai/basis_data_sqlite.md)
(dimajukan dari Fase 8: amalgamation adalah satu berkas `.c`, tidak butuh FFI).
SQLite 3.50.4 di-*vendor*; 10 fungsi `db_*` dengan parameter `?` wajib dan
satu pernyataan per panggilan.

Bentuk lama `impor "file.idk"` sengaja dipertahankan apa adanya (namespace
bersama) demi kompatibilitas K14 — itu yang dipakai `pustaka/*.idk`.

**Sisa yang disengaja:** modul P1 katalog 07 (`matematika`, `acak`, `teks`,
`sistem`, `waktu`, `tanggal_waktu`, `json`) belum ditulis ulang sebagai modul
**native C**. Isinya sudah tersedia sebagai `pustaka/*.idk` dan kini dapat
diimpor bermodul (`impor matematika`), sehingga penulisan ulang ke C adalah
optimasi, bukan fitur yang hilang.
Ref: 04 §6, 07 §1–5.

### ★ Rilis 1.0 — "Stabil untuk umum" — ✅ **syarat terpenuhi**

Syarat: ✅ Fase 0–3 selesai; ✅ [`SPESIFIKASI.md`](SPESIFIKASI.md) ditulis;
✅ semua contoh di panduan diuji otomatis (`tests/test_panduan.sh`, ikut
dijalankan `tests/test_all.sh`); ✅ kebijakan stabilitas K14 berlaku. Setelah
1.0, tidak ada breaking change sampai 2.0.

### Fase 4 — OOP lengkap (1.1–1.2)

Metode ajaib (06 §10), atribut kelas, `@properti`/`@metode_statis`/
`@metode_kelas`, dekorator, pewarisan berganda + MRO C3, `dengan ... sebagai`
(manajer konteks), `adalah_instansi`, kelas abstrak. Ref: 04 §4.

✅ metode ajaib tahap 1 — representasi, perbandingan, aritmetika
([rencana](RENCANA_FITUR/selesai/metode_ajaib.md), K19).

✅ metode ajaib tahap 2a — protokol objek: `_panjang_`, `_logika_`,
`_ambil_`/`_atur_`, `_hash_` + builtin `hash()`
([rencana](RENCANA_FITUR/selesai/protokol_objek.md), K19).

✅ metode ajaib tahap 2b — cerminan aritmetika `_ktambah_` dkk
([rencana](RENCANA_FITUR/selesai/cerminan_aritmetika.md), K19). Sentinel
`TidakDiterapkan` sengaja ditunda; alasannya ada di rencana itu.

✅ metode ajaib tahap 2c — `terurut`/`min`/`max` atas objek
([rencana](RENCANA_FITUR/selesai/urut_objek.md), K19).

✅ metode ajaib tahap 2d — `_iter_` (objek di `untuk ... dalam`) dan
`_panggil_` (objek sebagai fungsi)
([rencana](RENCANA_FITUR/selesai/iter_panggil.md), K19). Metode ajaib kini
**lengkap**: 30 metode. `adalah_instansi` sudah ada sejak Fase 2.

✅ atribut kelas — nilai bersama di badan kelas
([rencana](RENCANA_FITUR/selesai/atribut_kelas.md)).

✅ `dengan ... sebagai` — manajer konteks lewat `_masuk_`/`_keluar_`
([rencana](RENCANA_FITUR/selesai/dengan_sebagai.md)).

✅ Dekorator `@nama` pada fungsi, plus `@properti`, `@metode_statis`, dan
`@metode_kelas` ([rencana](RENCANA_FITUR/selesai/dekorator.md)).

✅ Pewarisan berganda + MRO C3
([rencana](RENCANA_FITUR/selesai/pewarisan_berganda.md)).

✅ Kelas abstrak `@abstrak`
([rencana](RENCANA_FITUR/selesai/kelas_abstrak.md)).

> **Fase 4 SELESAI.** Seluruh item OOP lengkap sudah diterapkan: metode ajaib
> (30), `adalah_instansi`, atribut kelas, `dengan ... sebagai`, dekorator
> (`@properti`, `@metode_statis`, `@metode_kelas`, `@abstrak`), pewarisan
> berganda + MRO C3, dan kelas abstrak.

### Fase 5 — Iterator & generator (1.3)

✅ Protokol iterator malas `_berikutnya_` (melengkapi `_iter_` dari Fase 4)
([rencana](RENCANA_FITUR/selesai/iterator_malas.md)).

✅ `hasilkan` (yield) — fungsi generator dengan frame yang ditangguhkan.

✅ Generator expression `(x untuk x dalam s)`.

✅ Comprehension kamus `{k: v untuk ...}` dan himpunan `{x untuk ...}`.

✅ `cocokkan`/`kasus` (pattern matching): literal, `_`, pengikatan nama,
`Kelas()`, alternatif `atau`, dan penjaga `jika`.

Ref: 04 §7.

> **Fase 5 SELESAI.** Metode ajaib kini 31 (`_berikutnya_`), generator dan
> generator expression berjalan malas, comprehension tersedia untuk daftar,
> kamus, dan himpunan, serta semuanya dapat membaca generator maupun iterator
> kustom. Batas yang disengaja: pola belum bersarang (`[a, b]` destructuring
> belum ada), dan `hasilkan` belum didukung di dalam blok `coba ... akhirnya`.

### Fase 6 — Pustaka standar luas (1.x berkelanjutan) — ✅ selesai (kecuali yang ditunda)

Modul P2 dari katalog 07. Modul baru berbentuk **modul bernamespace**
(`impor koleksi`, `dari koleksi impor Penghitung`), ditulis dalam Indonesiaku
di `pustaka/` kecuali yang ditandai C. Sebagian besar cocok untuk kontributor
baru.

**Gelombang A — modul murni Indonesiaku (selesai):**

| Modul | Status | Bukti |
|---|---|---|
| `salin` (copy) | ✅ | `examples/pustaka_salin.idk` |
| `statistik` (statistics) | ✅ | `examples/pustaka_statistik.idk` |
| `koleksi` (collections) | ✅ | `examples/pustaka_koleksi.idk` |
| `iterasi` (itertools, eager) | ✅ | `examples/pustaka_iterasi.idk` |
| `fungsional` (functools) | ✅ | `examples/pustaka_fungsional.idk` |

**Gelombang B & C** ([B](RENCANA_FITUR/selesai/pustaka_fase6_batch_b.md),
[C](RENCANA_FITUR/selesai/pustaka_fase6_gelombang_c.md),
[kura](RENCANA_FITUR/selesai/pustaka_kura.md)):

| Modul | Status | Bukti |
|---|---|---|
| `jalur` (os.path) | ✅ | `examples/pustaka_jalur.idk`, `examples/sistem_dasar.idk` |
| `opsi` (argparse) | ✅ | `examples/pustaka_opsi.idk` |
| `log` (logging) | ✅ | `examples/pustaka_log.idk` |
| `proses` (subprocess) + `indk --sandbox` | ✅ | `examples/pustaka_proses.idk` |
| builtin sistem operasi (`src/sistem.c`) | ✅ | `examples/sistem_dasar.idk` |
| `pola` (re, mesin C linear) | ✅ | `examples/pustaka_pola.idk` |
| `bita`, `base64`, `csv` | ✅ | `examples/pustaka_{bita,base64,csv}.idk` |
| `md5`/`sha1`/`sha256`/`sha512` | ✅ | `examples/pustaka_hash.idk` |
| klien HTTP (`http_ambil`) + `soket_sambung` | ✅ | `examples/pustaka_http_klien.idk`, `tests/test_klien_http.sh` |
| trigonometri (`sin` … `atan2`, `radian`, `derajat`) | ✅ | `examples/trigonometri.idk` |
| `kura` (turtle → SVG) | ✅ | `examples/pustaka_kura.idk` |

Lanjutan yang sengaja di luar Fase 6: HTTPS (butuh TLS, Fase 8), kanvas
`kura` beranimasi di playground, dan modul P3 katalog 07.

**Ditunda karena keterbatasan bahasa** (bukan lupa): `tupel_bernama`,
`@cache_lru`, dan `urutan_lengkap` menunggu fungsi variadik (`*args`) dan
penyuntikan metode saat runtime. Alasannya dicatat di
[`RENCANA_FITUR/selesai/pustaka_fase6_modul_dasar.md`](RENCANA_FITUR/selesai/pustaka_fase6_modul_dasar.md).

### Fase 7 — Perkakas & kinerja — ✅ perkakas selesai, kinerja berdasarkan ukuran

> **Status.** Seluruh perkakas yang direncanakan sudah ada: `--dis`, `periksa`,
> `uji`, `rapikan`, formatter VS Code, `profil`, `awakutu`, dan benchmark beban
> nyata. Sisi kinerja dikerjakan **hanya bila pengukuran menunjukkan untung**:
> mask tabel hash memberi −43,8% dan dikerjakan; computed goto, `OP_TRY_BEGIN`,
> cache `.idkc` dan playground WASM diukur lalu **ditolak beralasan** (catatan
> masing-masing ada di bawah). Yang tersisa — superinstruksi, inline cache,
> NaN-boxing, GC generasional, utas/asinkron — perubahan arsitektur yang belum
> ada datanya untuk dibenarkan.

✅ `indk --dis` — tampilkan bytecode tanpa menjalankan, termasuk fungsi
bersarang.

✅ `indk periksa` (linter) — deteksi variabel lokal tidak terpakai
([rencana](RENCANA_FITUR/selesai/deteksi_tidak_terpakai.md)). Hanya tingkat 2
(perintah opt-in); tingkat 1 dibatalkan karena menulis ke stderr di setiap
kali menjalankan program bentrok dengan syarat stderr kosong pada tes golden.

✅ `indk uji` — jalankan setiap fungsi `uji_*` di sebuah berkas, laporkan
lulus/gagal, exit 1 bila ada yang gagal. Asersi memakai `pastikan` yang sudah
ada, jadi tidak ada sintaks tes baru.

- [x] **`indk rapikan [--tulis|--periksa]`** — formatter berbasis token.
      Indentasi 4 spasi, spasi operator, `{` kamus vs blok, komentar bertahan
      utuh. Dijamin tidak mengubah arti program: 117/117 bytecode identik
      sebelum/sesudah format, dan hasilnya idempoten.
      Batas sadar: tidak memecah/menyambung baris — mengikuti baris asli
      penulis. K4 tetap aman: `examples/tanpa_indentasi.idk` dirapikan dan
      keluarannya tetap cocok golden.

✅ **Formatter VS Code** — `Format Document` memanggil `indk rapikan`
([`editors/vscode-indonesiaku/`](../editors/vscode-indonesiaku/)). Mendelegasikan
ke interpreter, bukan mengimplementasi ulang aturan format di JavaScript.

✅ **`indk profil <skrip.idk>`** — histogram opcode + hitung panggilan fungsi.
Penghitung, bukan sampling waktu: hasilnya **deterministik** (dua jalanan atas
masukan sama memberi angka identik) dan tidak terganggu unwinding galat atau
generator yang membekukan frame. Laporan ke **stderr**, jadi stdout program
tetap bisa dipipa. Overhead saat tidak dipakai: satu tes boolean per
instruksi, tidak terukur pada benchmark (0,040 s dengan dan tanpa profil).
Batas sadar: tidak memberi waktu per fungsi — untuk itu pakai `perf` atau
`valgrind --tool=callgrind` pada binernya.

✅ **`indk awakutu <skrip.idk>`** — debugger CLI. Perintah: `henti <baris>`,
`hapus <baris>`, `daftar`, `langkah`, `lanjut`, `lihat <nama>`, `tumpukan`,
`keluar`; Enter kosong = `langkah`. Seluruh antarmuka debugger ke **stderr**,
jadi `indk awakutu x.idk > hasil.txt` menghasilkan berkas yang identik dengan
menjalankan programnya biasa. Berhenti pada opcode pertama tiap baris sumber,
jadi satu baris berhenti sekali per iterasi — bukan sekali per opcode, dan
bukan sekali seumur loop.
Batas sadar: `lihat` membaca variabel **global dan parameter fungsi**, belum
`variabel` lokal di dalam badan fungsi — nama lokal hanya ada saat kompilasi,
sedangkan `ObjFunction.paramNames` sudah tersimpan dan sudah ditandai GC.
Menyimpan nama slot lokal sampai runtime menambah larik `ObjString*` per
fungsi; kerjakan kalau pemakaian nyata menuntutnya. Tidak ada protokol DAP
dan integrasi VS Code: tambahkan setelah CLI-nya terbukti dipakai.

✅ **Mask pengganti modulo pada tabel hash**
([rencana](RENCANA_FITUR/selesai/mask_tabel_hash.md)). `perf` menunjuk
`findValueEntry` di **10,18%** waktu eksekusi. Kapasitas tabel selalu pangkat
dua, jadi `hash & (capacity - 1)` menggantikan `hash % capacity` di tiga
fungsi pencarian. Beban `kamus_himpunan` dan `olah_teks` turun **−43,8%**
(114,6 → 64,4 ms); jumlah instruksi dan keluaran identik. Penjaga di
`adjustCapacity`/`adjustValueCapacity` menolak kapasitas non-pangkat-dua
dengan exit 70.
Catatan pengukuran: mengganti modulo di `findEntry`/`tableFindString` **saja**
tidak mengubah waktu sama sekali, walaupun 12 instruksi `div` terbukti hilang
dari assembly. Keuntungan baru muncul setelah `findValueEntry` ikut diganti —
assembly yang berubah bukan bukti program jadi lebih cepat.

Belum: cache `.idkc`, computed goto, superinstruksi, inline cache, NaN-boxing,
GC generasional, utas/asinkron.
Ref: 04 §1, §8–10, §12.

> **Catatan playground WebAssembly — tidak dikerjakan, sudah ada padanannya.**
> `docker/playground/` berisi playground server-side yang sudah berjalan,
> lengkap dengan isolasi (`read_only`, `cap_drop: ALL`, `pids_limit`,
> jaringan `internal: true`). WASM akan menduplikasi fungsi yang sudah bekerja,
> sambil menambah target build kedua (Emscripten) yang harus ikut diuji di CI.
> Alasan sah untuk meninjau ulang: bila playground perlu jalan **tanpa server**
> sama sekali (mis. disematkan di halaman dokumentasi statis). Itu kebutuhan
> yang berbeda, bukan optimasi dari yang sekarang.

> **Catatan computed goto.** Diukur dulu, belum dikerjakan. `run()` punya 85
> opcode dengan 93 `case`, dan **19 di antaranya mengandung `for`/`while`** —
> di sana `break` berarti keluar loop, bukan keluar switch. Transformasi
> mekanis ke tabel lompat akan salah kompilasi secara diam-diam di 19 titik
> itu. Build sudah `-O2`. Kerjakan hanya dengan profil yang menunjukkan
> dispatch benar-benar jadi hambatan, dan konversi satu per satu disertai
> pemeriksaan tiap `break`.
>
> **Profil pertama (`indk profil`, 2026-11-04).** Beban 7,0 juta instruksi
> (fib(24) rekursif + loop 200.000 iterasi) jalan pada **160 juta
> instruksi/detik**. Sebarannya rata — opcode teratas `OP_GET_LOCAL` hanya
> 19,6%, dan butuh 15 opcode untuk mencapai 94%. Sebaran rata begini berarti
> tabel lompat tidak punya satu cabang dominan untuk dimenangkan; tetap
> ditunda.
>
> **Temuan sampingan — DIUKUR ULANG, ternyata tidak layak dikerjakan.**
> `OP_TRY_BEGIN`/`OP_TRY_END` muncul 200.000 kali pada beban itu padahal
> programnya tidak memuat satu pun `coba`: setiap iterasi `untuk` memasang
> lalu membongkar penangan galat untuk menangkap `HentiIterasi`. Angka
> pertama, 5,7%, membuatnya tampak menjanjikan. Pengukuran lanjutan
> membatalkannya:
>
> | Beban | Porsi `OP_TRY_BEGIN`+`END` |
> |---|---|
> | Loop sintetis, badan `t = t + i` | 5,7% |
> | Loop sintetis atas daftar 1000 | 8,4% |
> | **Beban realistis** (JSON, kamus, panggilan fungsi) | **0,12%** |
>
> Porsi besar itu artefak badan loop yang nyaris kosong — begitu badan loop
> berisi kerja nyata, dua opcode overhead itu tenggelam. Menonaktifkan
> seluruh isi `OP_TRY_BEGIN` (penyimpanan `handlerIp`/`frameCount`/
> `stackTop`) memberi **0,0%**: tidak ada kerja mahal untuk dihapus, hanya
> biaya dispatch ~5,6 ns per instruksi. Sementara itu menghapusnya bukan
> perubahan kecil — blok try itu menangani `HentiIterasi` dari **generator**
> dan iterator kelas `__berikutnya__`, sedangkan cek `OP_LESS` yang sudah ada
> hanya berlaku untuk urutan berindeks. Mengubah protokol iterasi bercabang
> tiga demi 0,12% tidak sepadan.
>
> **Pelajarannya, dan alasan `tests/beban/` ada:** benchmark lama mengukur
> `examples/performance_stress.idk` — 4 baris komentar dan satu `cetak`,
> 6 instruksi. Tanpa beban yang representatif, profil apa pun akan menunjuk
> target yang salah. Ukur pada beban nyata sebelum mempercayai persentase.

> **Catatan `indk rapikan`.** Penghalangnya sudah dibereskan, bukan dihindari.
> Dulu `skipWhitespace()` di `src/scanner.c` membuang komentar sepenuhnya —
> tidak ada `TOKEN_KOMENTAR`, jadi komentar tidak pernah sampai ke token
> stream, dan formatter berbasis token akan **menghapus seluruh komentar**
> pengguna. Sekarang scanner punya mode penyimpan komentar yang **opt-in**
> (`scannerKeepComments()`, default mati) sehingga compiler tidak terpengaruh:
> dibuktikan dengan 117/117 berkas menghasilkan bytecode identik saat flag
> mati. Formatter memakai mode itu dan komentar bertahan utuh.
> Rencana: [`RENCANA_FITUR/selesai/komentar_dan_rapikan.md`](RENCANA_FITUR/selesai/komentar_dan_rapikan.md).

> **Catatan cache `.idkc`.** Diukur, lalu ditunda sebagai YAGNI. Berkas
> sumber terbesar di repo (`metode_ajaib.idk`, 8 KB) butuh **3,1 ms** total,
> dan **1,3 ms** di antaranya adalah ongkos start proses — jadi kompilasi
> sesungguhnya sekitar **1,8 ms**. Cache bytecode berarti menambah format
> serialisasi, penomoran versi, invalidasi berdasarkan mtime/hash, dan
> penulisan graf objek GC (`ObjString`, `ObjFunction` bersarang, `Chunk`
> dengan `lines`/`columns`/`source`) demi menghemat < 2 ms. Kerjakan kalau
> ada program yang kompilasinya benar-benar terasa, bukan sebelum itu.

> **Catatan playground WebAssembly.** Terhalang perkakas: `emcc` tidak
> terpasang di mesin ini (`which emcc` kosong, tidak ada
> `/usr/lib/emscripten`). Playground yang ada sekarang berjalan lewat Docker
> dan sudah berfungsi, jadi WASM adalah penggantian jalur eksekusi, bukan
> fitur baru untuk pengguna. Butuh pemasangan Emscripten lebih dulu.

> **Catatan LSP, debugger, inline cache, superinstruksi, NaN-boxing, GC
> generasional, utas/asinkron.** Belum dikerjakan. Masing-masing berukuran
> setara satu fase tersendiri — NaN-boxing menyentuh setiap `Value` di
> seluruh basis kode, GC generasional menulis ulang `memory.c`, dan LSP
> adalah program kedua di samping interpreter. Jangan dikerjakan sebagai
> tambahan di sela pekerjaan lain; masing-masing butuh berkas rencana
> sendiri di `docs/RENCANA_FITUR/` lebih dulu.

### Fase 8 — Ekosistem

Manajer paket + registri, API embedding C, FFI, MySQL/PostgreSQL (klien
jaringan — `sqlite` sendiri sudah dimajukan ke Fase 3), TLS/HTTPS, GUI,
AOT/bundel executable, ~~anotasi tipe opsional~~ ✅ ([rencana](RENCANA_FITUR/selesai/anotasi_tipe.md)), mode sandbox untuk
sekolah.
Ref: 04 §11–13, 05.

- [x] **Distribusi paket native per sistem operasi** (v1.4.0,
      [rencana](RENCANA_FITUR/selesai/paket_native_dan_pembaruan.md)) —
      rilis menyertakan `indonesiaku_<versi>_amd64.deb` dan
      `Indonesiaku-Setup-<versi>.exe` selain arsip `.tar.gz`/`.zip`. Paket
      memasang biner **beserta** `pustaka/`, karena `indk` telanjang gagal
      pada `impor web`. Memasang versi baru menimpa yang lama tanpa perlu
      uninstall; `tests/paket_deb.sh` membuktikannya terhadap `dpkg` nyata.
      Belum: `.dmg` bertanda tangan untuk macOS (butuh akun Apple Developer;
      DMG tak bertanda tangan ditolak Gatekeeper) dan paket `.rpm`.

- [x] **`indk --periksa-pembaruan`** (v1.4.0) — membandingkan versi terpasang
      dengan rilis terbaru di GitHub, lalu mencetak perintah pemutakhiran
      sesuai sistem operasi. Hanya berjalan bila diminta; menjalankan skrip
      biasa tidak pernah menghubungi jaringan.
      Batas sadar: **tidak ada pembaruan otomatis** — mengganti biner yang
      sedang berjalan butuh hak root dan verifikasi tanda tangan. Pengguna
      versi sebelum 1.4.0 belum memiliki perintah ini sehingga perlu memeriksa
      halaman rilis manual satu kali.

## Roadmap implementasi per fase — setelah v1.4.0

Bagian ini menyatukan rencana P1–P7, WDB-01–05, WEB-01–07, DX-01–02, SSR-01–10, serta kebutuhan transfer berkas dan keamanan runtime yang sebelumnya tersebar dalam beberapa tabel. **Fase 0–6 sudah selesai; Fase 7 perkakasnya selesai sedangkan sisi kinerjanya dikerjakan hanya bila pengukuran membenarkan (mask tabel hash dikerjakan, computed goto / `OP_TRY_BEGIN` / cache `.idkc` / WASM ditolak beralasan); dan Fase 8 baru sebagian (anotasi tipe, distribusi paket native, pemberitahuan pembaruan). Fase 9 selesai di v1.4.0; Fase 10 dan 11 baru sebagian.** Fase 12 dan seterusnya adalah urutan kerja yang **diusulkan**, bukan penanda implementasi saat ini. Nomor ID lama tetap dipakai sebagai jejak rencana.

**Cara membaca:** setiap fase memiliki tujuan, kebutuhan awal, langkah implementasi berurutan, contoh API target, pengujian wajib, serta definisi selesai. Tanda [ ] berarti belum. Implementasi dapat dibuat dalam pull request kecil; jangan memaksa seluruh fase dalam satu PR. Semua perubahan tetap tunduk pada "Definisi selesai" dan "Feature workflow" di AGENTS.md, termasuk rencana detail sebelum kode, golden test, negative test, dokumentasi dan editor.

### Strategi penomoran dan kapan menerbitkan rilis

**Baseline:** tag publik terakhir ialah `v1.4.0`. Nomor berikut bersifat **rekomendasi** yang dapat digeser jika pekerjaan sebenarnya berubah. Jangan membuat tag hanya karena nomor fase berganti. Versi mengikuti SemVer (`MAJOR.MINOR.PATCH`): perubahan kompatibel dan fitur baru = minor, perbaikan tanpa penambahan fitur = patch, dan perubahan yang memutus API/sintaks atau format eksternal = major (mis. `v2.0.0`). Sejak v1.0.0, kebijakan proyek menghindari breaking change sampai v2.0.0; bila tidak dapat dihindari, pisahkan ke rencana v2.0, jangan memasukkannya diam-diam ke seri v1.x.

| Fase | Kandidat pengujian | Rilis stabil rekomendasi | Dipublikasikan setelah |
|---|---|---|---|
| 9 — LSP | `v1.4.0-rc.1` | `v1.4.0` | CI editor/protokol dan uji lintas OS hijau |
| 10 — Database | `v1.5.0-rc.1` | `v1.5.0` | Driver dan integration tests MySQL/MariaDB/PostgreSQL lulus |
| 11 — HTTP/HTTPS | `v1.6.0-rc.1` | `v1.6.0` | Validasi TLS dan uji keamanan/protokol server lulus |
| 12 — HTML SSR | `v1.7.0-rc.1` | `v1.7.0` | Templating aman dan contoh SSR lengkap |
| 13 — Web security | `v1.8.0-rc.1` | `v1.8.0` | Audit dan tes auth/session/CSRF lulus |
| 14 — Package manager | `v1.9.0-rc.1` | `v1.9.0` | Resolusi dependensi dan supply-chain checks lulus |
| 15 — Debugger & FFI | `v1.10.0-rc.1` | `v1.10.0` | API native/GC dan debug tests lulus |
| 16 — WASM & performa | `v1.11.0-rc.1` | `v1.11.0` | Uji browser/WASM dan benchmark yang relevan lulus |
| 17 — Transfer file & SSH/SFTP/FTP | `v1.12.0-rc.1` | `v1.12.0` | Protokol aman, uji transfer, host key dan TLS lulus |
| 18 — Kriptografi & keamanan runtime | `v1.13.0-rc.1` | `v1.13.0` | Crypto KAT, fuzz/sanitizer, sandbox dan audit keamanan lulus |
| 19 — Paritas kemampuan & referensi API | `v1.14.0-rc.1` | `v1.14.0` | Katalog terverifikasi, XML/validasi aman, dokumentasi dan tes kategori lulus |
| 20 — Async/concurrency | `v1.15.0-rc.1` | `v1.15.0` | Event loop/cancellation/GC dan regresi I/O lulus |
| 21 — Realtime, cache dan jobs | `v1.16.0-rc.1` | `v1.16.0` | WebSocket, Redis, SMTP, queue dan scheduler diuji |
| 22 — Observability dan kesiapan produksi | `v1.17.0-rc.1` | `v1.17.0` | Logging/metrics/profiler, keamanan dan release policy teruji |
| 23 — Data biner/arsip | `v1.18.0-rc.1` | `v1.18.0` | Binary/streaming dan ekstraksi arsip aman teruji |
| 24 — Unicode/i18n/konfigurasi | `v1.19.0-rc.1` | `v1.19.0` | Unicode/locale/serialisasi dan parser aman lulus |
| 25 — CLI, API testing, broker | `v1.20.0-rc.1` | `v1.20.0` | CLI/test client dan protokol broker opsional teruji |

**Tahapan rilis di dalam tiap fase:**

1. **Development / belum dirilis:** mulai dari dokumen rencana, implementasi pada branch, PR terpisah untuk subfitur, dan pengujian otomatis. Perubahan yang belum stabil tidak dijanjikan sebagai API publik.
2. **Alpha (opsional, mis. `v1.4.0-alpha.1`):** dipakai bila perlu uji pengguna lebih awal; daftar fitur belum final, disertai peringatan ketidakstabilan. Jangan dorong artefak alpha ke tag Docker `latest`.
3. **Release candidate (mis. `v1.4.0-rc.1`):** semua cakupan wajib fase sudah code-complete, docs dan examples cocok, CI lintas OS hijau, tes negatif/keamanan dan sanitizer yang relevan lulus, serta artefak instalasi diuji pada mesin bersih.
4. **Rilis stabil (mis. `v1.4.0`):** setelah umpan balik RC ditindaklanjuti dan tidak ada blocker; finalisasi CHANGELOG, versi `src/common.h`, installer, Docker, situs, tag git, checksums serta release notes. Pastikan paket rilis benar-benar dapat dijalankan; keberhasilan build saja tidak cukup: uji paket rilis di Windows, macOS, dan Linux mengikuti `docs/TESTING/` lalu catat hasilnya (satu baris riwayat per OS). OS yang belum diuji ditulis apa adanya di catatan rilis.
5. **Patch hotfix (mis. `v1.4.1`):** ketika ada bug/kerentanan/regresi pada rilis yang sudah ada. Backport perbaikan dan uji tanpa menunggu fase berikutnya, termasuk compatibility checks. Jangan mengemas fitur besar baru di patch.
6. **Penyesuaian jadwal:** fitur tertunda boleh dipindah ke minor berikutnya bila tidak merusak kontrak, namun item fase yang wajib tidak boleh ditandai selesai dan batasan harus tertulis. Jika fase selesai tidak berbarengan dengan versi minor, jelaskan mapping faktual di CHANGELOG dan roadmap.

**Kebijakan keputusan "siap rilis" untuk semua fase:** seluruh tes wajib berstatus lulus; tidak ada bug blocker, crash/use-after-free, pelanggaran kompatibilitas yang tak disetujui, atau kerentanan kritis yang diketahui pada scope fitur; halaman panduan, contoh, ekstensi/editor jika terkait, dan artefak distribusi diuji; spesifikasi, SBOM/dependensi pihak ketiga dan lisensi diperiksa jika relevan. CI gagal atau pembuktian belum ada = **tunda rilis**, bukan mengubah status menjadi ✅.

### Fase 9 — Language Server Protocol (LSP) dan pengalaman editor

**Tujuan:** file .idk mendapatkan diagnostics, autocomplete, hover, dan go-to-definition di VS Code tanpa menjalankan program pengguna. **Status:** ✅ selesai. **Pemetaan:** P1 (bagian Fase 7).

**Rekomendasi rilis fase: `v1.4.0` (minor), kandidat `v1.4.0-rc.1`; patch `v1.4.1` dan seterusnya jika hanya perbaikan kompatibel.** Ini target perencanaan, bukan janji jadwal atau tag yang sudah dibuat.\n\n**Gerbang rilis:** LSP diagnostics dan editor minimum sudah bekerja serta diuji terhadap dokumen tidak lengkap; completion/hover/definition memenuhi checklist fase.\n\n
**Langkah pengembangan:**

1. [x] Rencana desain: `docs/RENCANA_FITUR/selesai/lsp.md`; LSP compile-only memakai `indk --dis`, tanpa mengeksekusi source.
2. [x] Server JSON-RPC 2.0 stdio, initialize/shutdown, sinkronisasi dokumen, batas ukuran/timeout, dan pembatalan hasil compile usang saat dokumen berubah.
3. [x] Diagnostics sintaks dari compiler serta warning lokal tidak terpakai dari `indk periksa`, termasuk dokumen belum disimpan dan konversi kolom UTF-8 compiler ke posisi UTF-16 VS Code.
4. [x] Completion keyword/builtin/magic method/simbol dokumen, hover ringkas, dan go-to-definition lokal satu dokumen. Resolusi lintas modul ditunda.
5. [x] Integrasi ke `editors/vscode-indonesiaku`, mempertahankan syntax highlight, snippets, dan formatter yang ada.
6. [x] Tes JSON-RPC/LSP untuk diagnostics, Unicode, dokumen `untitled`, completion, hover, definition, perubahan dokumen, dan memastikan source tidak dieksekusi.
7. [x] Panduan ekstensi dan CI lintas Linux/Windows/macOS (`editor-lsp`).

**Integrasi aktual:** ekstensi menjalankan `node server.js` melalui `process.execPath`; server memakai `indk --dis` dan `indk periksa`. Tidak ada executable compiler baru.

**Selesai:** `npm run test:lsp` dan `npm run test:format` lulus; job CI `editor-lsp` menguji Linux/Windows/macOS. Cakupan go-to-definition saat ini lokal satu dokumen; impor lintas file tetap pekerjaan lanjutan.

### Fase 10 — Driver dan fondasi database

**Tujuan:** SQLite dipertahankan, lalu MySQL/MariaDB dan PostgreSQL dapat dipakai dengan query aman. **Status:** ⬜ belum. **Pemetaan:** WDB-01–05, P6.

**Rekomendasi rilis fase: `v1.5.0` (minor), kandidat `v1.5.0-rc.1`; patch `v1.5.1` dan seterusnya jika hanya perbaikan kompatibel.** Ini target perencanaan, bukan janji jadwal atau tag yang sudah dibuat.\n\n**Gerbang rilis:** Driver SQLite, MySQL/MariaDB, PostgreSQL lulus CRUD, binding parameter, transaksi, galat dan tes integrasi lintas layanan. Fitur TLS untuk koneksi database wajib aman sebelum dipublikasikan.\n\n
**Langkah pengembangan:**

1. [x] Buat docs/RENCANA_FITUR/database_multi_driver.md; tetapkan API, semantik transaksi, tipe nilai/NULL/BLOB, error, koneksi dan lifecycle GC. Jangan mengubah builtin db_* tanpa kebijakan kompatibilitas.
2. [x] Audit src/db.c, src/object.c/h, src/native.c/h, dan examples/db_sqlite.idk. Lengkapi SQLite: dukungan data biner/BLOB masuk-keluar, busy timeout, pengujian transaksi dan penutupan handle idempoten (**WDB-01**).
   - Selesai 1.4.0: BLOB masuk-keluar sebagai daftar bulat 0..255 (konvensi `pustaka/bita.idk`), `db_batas_tunggu()`, transaksi diuji di `examples/db_sqlite.idk`, dan `db_tutup()` idempoten (diverifikasi). Diuji dengan ASan/UBSan dan `-DDEBUG_STRESS_GC`.
3. [x] Buat driver MySQL/MariaDB (**WDB-02**) menggunakan client library teruji dan prepared statements; dukung host/port, opsi TLS dengan verifikasi, timeout, penanganan koneksi terputus, query satu/banyak baris, perubahan baris dan ID terakhir sesuai dialek.
   - Selesai 1.6.0: `pustaka/my.idk` bicara protokol MySQL langsung tanpa libmysqlclient, sejalan dengan `pustaka/pg.idk`.
   - Autentikasi `mysql_native_password` (bawaan MariaDB). `caching_sha2_password` (bawaan MySQL 8) **ditolak dengan pesan jelas**, bukan didukung setengah jalan.
   - **Versi pertama memakai COM_QUERY + escaping manual dan itu dibuang setelah terbukti MERUSAK DATA**: dengan `sql_mode = NO_BACKSLASH_ESCAPES`, backslash bukan karakter escape, sehingga `x\` tersimpan menjadi `x\\` — korupsi diam-diam, bukan galat. Escaping yang benar mustahil tanpa melacak sql_mode server, dan mode itu bisa berubah kapan saja lewat `SET SESSION`. Diganti **prepared statement** (COM_STMT_PREPARE/EXECUTE), yang memindahkan pemisahan kode/data ke protokol.
   - Diuji terhadap **MariaDB 11 sungguhan** (`tests/my_driver.sh`, 22 pemeriksaan): CRUD, INT kembali sebagai angka (format baris biner), NULL, Unicode 4-byte, transaksi, hitungan parameter dua arah, 300 baris (paket terpecah di soket), `NO_BACKSLASH_ESCAPES`, payload injeksi tersimpan utuh sebagai data, sandi salah ditolak dan tidak bocor.
   - **MySQL 8 belum pernah dijalankan** di mesin ini: image `mysql:8` menolak start pada CPU tanpa `x86-64-v2` (*Fatal glibc error*). Jadi dukungan MySQL 8 **tidak diklaim** — bawaannya `caching_sha2_password` yang memang ditolak driver ini.
   - Belum: TLS ke database, timeout per-query, `COM_STMT_RESET`/reuse statement (statement ditutup setelah dipakai).

4. [x] Buat driver PostgreSQL (**WDB-03**) memakai libpq atau pustaka kompatibel yang terawat; pastikan parameter binding, hasil data bertipe, transaksi, timeout dan validasi TLS/hostname.
   - Selesai 1.6.0: `pustaka/pg.idk` bicara **wire protocol v3 langsung**, bukan libpq. Alasannya ada di `docs/RENCANA_FITUR/database_multi_driver.md`: `ldd indk` tetap hanya libc+libm, dan menaut libpq akan mengembalikan ketergantungan runtime per OS yang juga merusak build musl/Alpine yang baru ditutup 1.5.0.
   - **Prasyarat yang ternyata salah didiagnosis**: tabel status lama menyebut "terblokir pustaka klien eksternal". Penghalang sebenarnya `soket_tulis()` hanya menerima teks, dan `chr(255)` ter-enkode UTF-8 jadi dua byte — setiap bingkai biner rusak di kawat. Ditutup dengan `soket_tulis(soket, daftar_bita)` dan `soket_baca_bita(soket)`, memakai konvensi daftar bulat 0..255 yang sudah dipakai `db_*` untuk BLOB.
   - **Autentikasi SCRAM-SHA-256** (bawaan PostgreSQL ≥ 14). `password` polos dan `md5` **ditolak**, bukan didukung diam-diam. `hmac_sha256()` dan `pbkdf2_sha256()` ditambahkan memakai mbedTLS yang memang sudah di-vendor untuk TLS — kriptografi tidak ditulis ulang; keduanya dicocokkan byte-per-byte dengan `hashlib` Python.
   - **Parameter dikirim sebagai parameter protokol** (Parse/Bind), bukan dirangkai ke SQL. Nilai dari pengguna tidak pernah melewati parser SQL.
   - Diuji terhadap **PostgreSQL 16 sungguhan dalam container** (`tests/pg_driver.sh`, 17 pemeriksaan): CRUD, baris terpengaruh, NULL, Unicode 4-byte, injeksi SQL jadi data, transaksi komit/rollback, galat server tertangkap tanpa merusak koneksi, `tutup()` idempoten, sandi tidak bocor ke pesan galat.
   - **Server jujur tidak bisa membuktikan klien memeriksa apa pun**: mematikan cek nonce dan tanda tangan server tetap menghasilkan koneksi sukses melawan PostgreSQL asli. Karena itu ada `tests/bantu/pg_server_jahat.py` — server palsu yang menuntaskan jabat tangan tapi curang. Asli → ditolak, sabotase → diterima, jadi kedua cek terbukti yang menahan.
   - Belum: TLS ke database baru lewat `tls_mulai` (kode ada, **belum diuji** terhadap server ber-TLS); timeout per-query; tipe hasil masih teks (konversi di sisi pemanggil).
5. [x] Rancang API abstraksi bersama (**WDB-04**), misalnya db.koneksi(konfigurasi), koneksi.eksekusi(sql, parameter), koneksi.ambil_satu, koneksi.ambil_semua, koneksi.mulai/komit/batal/tutup. Nama final diputuskan lewat dokumen rencana; API SQLite lama tetap berjalan.
   - **Selesai sebagai `pustaka/db.idk`.** `db.sambung({"jenis": ...})` menyalurkan ke SQLite, MySQL, atau PostgreSQL; permukaan metodenya seragam dan `k.jenis()` menyebut backend yang dipakai.
   - **Penghalang yang dicatat sebelumnya sudah tidak ada.** Percobaan 1.6.0 dibatalkan karena `impor` disebut membawa nama ke lingkup global berkas sehingga `sambung()` kedua driver bentrok. Sejak modul memiliki globalnya sendiri (Fase 3), `impor my` dan `impor pg` bernamespace: diverifikasi bahwa `my.sambung` dan `pg.sambung` hidup berdampingan sementara `sambung` polos tidak pernah bocor ke lingkup global. Catatan lama itu benar saat ditulis dan kedaluwarsa tanpa pernah dicabut — periksa ulang klaim "terhalang" sebelum mempercayainya.
   - SQLite paling banyak disesuaikan: ia satu-satunya yang memakai fungsi global, dan `db_eksekusi()` mengembalikan `kosong` sementara dua driver lain mengembalikan jumlah baris. Pembungkusnya menyamakan lewat `db_baris_terpengaruh()`.
   - `my.idk` diberi nilai bawaan `parameter = kosong` agar seragam dengan `pg.idk`; sebelumnya memanggil `k.eksekusi(sql)` tanpa parameter sah di PostgreSQL tetapi galat di MySQL.
   - Dialek SQL dan penanda parameter (`$1` vs `?`) **tetap tidak disembunyikan** — menerjemahkannya menuntut pemindai yang memahami literal string, dan pemindai yang salah merusak query sah seperti `WHERE nama = 'siapa?' AND id = ?`.
   - Diuji terhadap ketiga backend sungguhan (`tests/db_bersama.sh`): satu skrip `.idk` yang sama dijalankan tiga kali dan harus menghasilkan keluaran identik.
   - Belum: pool koneksi, ORM, penyeragaman tipe data antar backend.

6. [x] Tambahkan lapisan migrasi skema dan query builder parameterized ringan (**WDB-05**); simpan versi migrasi dan catat aturan rollback. ORM penuh tidak wajib.
   - `pustaka/migrasi.idk`: `langkah(versi, nama, naik, turun)`, `naikkan_semua()`, `turunkan_satu()`, `status()`. Versi terpasang dicatat di tabel `_migrasi_indk` sehingga rerun tidak mengulang DDL.
   - Langkah dijalankan **urut menaik menurut versi**, bukan urutan penulisan: daftar yang dirakit dari beberapa tempat jika tidak diurutkan akan berjalan berbeda antar lingkungan. Contoh goldennya sengaja mendaftarkan `002` sebelum `001`.
   - Tiap langkah berada di dalam transaksinya sendiri; galat memicu `batal()` lalu dinaikkan ulang sebagai `GalatMigrasi` dan langkah sesudahnya tidak dijalankan.
   - **Rollback transaksional hanya aman penuh di SQLite dan PostgreSQL.** MySQL/MariaDB melakukan implicit commit pada DDL, jadi langkah yang gagal di tengah bisa meninggalkan skema separuh jadi. Dicatat di panduan, tidak disembunyikan.
   - **Query builder sengaja ditunda**, bukan terlupakan: merakit SQL otomatis menuntut keputusan dialek, padahal `db.idk` justru memutuskan untuk TIDAK menyembunyikan dialek dan penanda parameter. Itu keputusan desain tersendiri.
   - Dijaga `tests/migrasi_sabotase.sh`: tujuh jaminan (pencatatan versi, pemeriksaan versi terpasang, penghapusan catatan saat rollback, arah rollback, pengurutan versi, penolakan `turun` kosong, penolakan versi ganda) masing-masing dilucuti dan wajib membuat tes merah — 7/7.
7. [ ] Siapkan database uji MySQL, MariaDB, PostgreSQL dalam container CI yang terisolasi; jalankan suite bersama untuk koneksi, CRUD, transaksi, rollback, NULL, Unicode, BLOB, kegagalan TLS, kredensial salah, timeout, SQL injection dan cleanup.
8. [ ] Perbarui docs/SPESIFIKASI.md bila kontrak bahasa berubah, website/panduan/07_PUSTAKA_STANDAR.md, examples/, CHANGELOG.md dan instalasi dependensi per OS.

**Contoh API target (belum tersedia):** koneksi = basisdata.buka({"driver":"postgresql","host":"localhost","nama":"sekolah"}); koneksi.ambil_semua("SELECT * FROM siswa WHERE id = ?", [1]). Placeholder disesuaikan internal oleh driver, bukan dirangkai ke SQL secara manual.

**Selesai jika:** tiga keluarga database (SQLite, MySQL/MariaDB, PostgreSQL) berhasil CRUD/transaksi dengan parameter, error dapat ditangkap, tidak ada password masuk log, dependensi dan cara build terdokumentasi. TLS driver boleh berbagi komponen dari Fase 11 dan harus lulus sebelum driver koneksi luar dianggap siap.

### Fase 11 — HTTP, HTTPS/TLS dan server backend modular

**Tujuan:** aplikasi Indonesiaku dapat melayani REST API secara terstruktur; contoh todo_server.idk yang single-thread/blocking bukan lagi satu-satunya pola. **Status:** ⬜ belum. **Pemetaan:** P2, WEB-01, WEB-02, WEB-03, WEB-07.

**Rekomendasi rilis fase: `v1.6.0` (minor), kandidat `v1.6.0-rc.1`; patch `v1.6.1` dan seterusnya jika hanya perbaikan kompatibel.** Ini target perencanaan, bukan janji jadwal atau tag yang sudah dibuat.\n\n**Gerbang rilis:** Klien HTTPS lolos verifikasi sertifikat/hostname; router, middleware, parsing input dan static assets serta pengujian HTTP sudah stabil.\n\n
**Langkah pengembangan:**

1. [x] Buat docs/RENCANA_FITUR/http_web_server.md dan https_tls.md. Tentukan kontrak Request/Response, router, middleware, batas request, model konkurensi dan pemakaian reverse proxy.
   - Selesai: keduanya ada di `docs/RENCANA_FITUR/selesai/` (`http_web_server.md` target 1.4.0, `https_tls.md` target 1.5.0). Kontrak Request/Response, router, middleware dan batas request dipakai nyata oleh `pustaka/web.idk`; reverse proxy didokumentasikan di `website/panduan/08_DEPLOYMENT.md`.
   - **Model konkurensi ditulis apa adanya, bukan diklaim setara server produksi**: satu permintaan pada satu waktu. VM ini satu utas dan belum punya primitif thread, jadi worker/backpressure belum berlaku — lihat komentar `ponytail:` di `pustaka/web.idk`.
2. [x] Perluas klien pustaka/http.idk sehingga http_ambil dapat menerima https:// (**P2**). Pilih TLS library yang terawat; aktifkan verifikasi CA dan hostname secara default; timeout dan redirect dibatasi; insecure-by-default dilarang.
   - Selesai 1.5.0: `http_ambil()` menerima `https://` lewat builtin
     `tls_mulai(soket, host)`. Ditenagai **mbedTLS 3.6.2 (LTS)** yang di-vendor
     di `src/vendor/mbedtls/` dan ditaut statis — biner tetap hanya menaut
     libc/libm, jadi sifat mandirinya terjaga. Dasar keputusan + angka
     pengukurannya ada di [`docs/ANALISIS/08_KEPUTUSAN_TLS.md`](ANALISIS/08_KEPUTUSAN_TLS.md).
   - **Verifikasi tidak bisa dimatikan**: tidak ada parameter, env, atau "mode
     pengembangan" untuk melewatinya. Diuji terhadap `badssl.com` di
     `tests/https_klien.sh` — kedaluwarsa, hostname salah, self-signed, dan
     akar tak dikenal semuanya ditolak dengan pesan yang menyebut sebabnya.
     Sabotase (VERIFY_NONE, SNI dimatikan, CA gagal tapi lanjut) membuat tes
     merah.
   - CA bundle: `INDK_CA_BUNDLE` lebih dulu, lalu jalur sistem umum; **tidak
     ada satu pun = gagal**, bukan diam-diam melewati verifikasi.
   - Pengalihan `https://` → `http://` **ditolak**. Sekaligus memperbaiki bug
     lama `_http_gabung_url()` yang meng-hardcode `http://` sehingga pengalihan
     relatif di situs HTTPS turun ke HTTP polos.
   - Sisa yang jujur ditulis: **Windows belum membaca Certificate Store**, jadi
     di sana `INDK_CA_BUNDLE` perlu disetel bila jalur PEM gaya Unix tidak ada.
     Build MinGW diverifikasi dan HTTPS-nya berjalan (termasuk keempat
     penolakan badssl.com) lewat Wine; Wine bukan Windows asli, jadi perilaku
     CA di sana belum terbukti. Permukaan audit bertambah 132.049 (ccm.c dibuang)
     baris C pihak ketiga; `tools/vendor_mbedtls.sh` menjadikan pembaruan saat
     CVE satu perintah dengan sha256 terkunci.
   - Lingkup: hanya **klien**. Server HTTPS tetap lewat reverse proxy (lihat
     `website/panduan/08_DEPLOYMENT.md`).
3. [x] Rancang modul server HTTP dengan routing metode/path, parameter URL, query string, header, status, error handler dan middleware (**WEB-01**). Hindari ekspose detail socket mentah pada API yang digunakan pemula.
   - Selesai 1.4.0: `pustaka/web.idk` — routing metode/path, parameter URL `:nama`, query string + dekode persen, middleware berantai, penangan galat, 405 dengan header `Allow`. Diuji lewat HTTP sungguhan (`tests/web_router_server.sh`, 9 pemeriksaan curl). Socket mentah tidak diekspos: `app.tangani()` bekerja pada teks.
4. [x] Tambahkan parsing application/json, application/x-www-form-urlencoded, multipart/form-data, file upload streaming dan batas ukuran/bentuk input (**WEB-02**).
   - Selesai 1.4.0: `req["json"]`, `req["form"]`, `req["berkas"]` terisi sesuai `Content-Type`; multipart termasuk unggahan berkas; batas ukuran bawaan 1 MB dijawab 413 berdasarkan `Content-Length` sebelum data dikumpulkan. `chunked` ditolak 411, JSON rusak 400.
   - Catatan: "streaming" di sini berarti badan dibaca bertahap dari soket sampai `Content-Length` terpenuhi (`web.baca_permintaan()`), bukan ditulis bertahap ke disk. Berkas unggahan ditahan di memori dan dibatasi `maks_badan`; penulisan langsung ke disk untuk unggahan besar belum ada.
5. [x] Implementasikan static file handler untuk CSS, JavaScript, gambar dan aset lain, MIME yang benar, cache header, penolakan traversal serta symlink berbahaya (**WEB-03**). Ini **bukan** template HTML.
   - Selesai 1.5.0: `app.statis(prefiks, akar)` di `pustaka/web.idk`. Hanya `GET`/`HEAD` (lainnya 405 dengan `Allow`), direktori tidak pernah didaftar isinya, MIME dari ekstensi (tak dikenal → `application/octet-stream`) plus `X-Content-Type-Options: nosniff`, `ETag` lemah dengan `If-None-Match` → 304. Traversal ditolak **setelah** dekode persen (`%2e%2e` setara `../`), backslash diperlakukan sebagai pemisah, byte NUL ditolak; path ber-`..` ditolak seluruhnya, bukan dinaikkan satu level. Diuji lewat HTTP sungguhan (`tests/web_statis_server.sh`, 24 pemeriksaan curl) termasuk md5 berkas biner.
   - Prasyarat yang ikut diperbaiki: builtin `panjang_byte()`. `_http_panjang_byte()` sebelumnya menghitung ulang lebar UTF-8 per karakter, sehingga `Content-Length` badan biner salah (berkas 5 byte dijawab 6) dan byte non-UTF-8 hilang dari iterasi.
   - Symlink: ditemukan bocor saat pengujian (tautan di dalam akar menyajikan berkas di luarnya, HTTP 200) lalu ditutup dengan builtin `_jalur_nyata()` — jalur nyata berkas harus berada di bawah jalur nyata akar. Ada tes regresinya.
   - Belum: `Last-Modified` tidak dikirim karena `format_tanggal()` hanya punya waktu lokal, bukan GMT; `Range`/unduhan sebagian belum didukung.
6. [x] Rancang concurrency yang terukur, batas koneksi/thread/worker, timeout, backpressure, shutdown aman dan log terstruktur (**WEB-07**). Mulai dengan model sederhana yang dapat diuji dan ditingkatkan; jangan langsung mengklaim setara server produksi.
   - Selesai 1.5.0 **untuk model satu permintaan pada satu waktu**, lihat batasan di bawah. Gelung server kini milik pustaka: `app.jalankan(port, batas_waktu = 10, maks_permintaan = 0, catat = benar)` dan `app.berhenti()`. Diuji lewat HTTP sungguhan (`tests/web_jalankan_server.sh`, 12 pemeriksaan).
   - **Batas waktu**: builtin `soket_batas_waktu(soket, detik)` (`SO_RCVTIMEO`/`SO_SNDTIMEO`). Menutup lubang nyata: sebelumnya tidak ada batas waktu sama sekali, sehingga satu klien yang menyambung lalu diam membekukan server selamanya (slowloris, biaya satu koneksi). Bawaan builtin `0` demi kompatibilitas, tetapi `jalankan()` memakai 10 detik karena API baru.
   - **Galat per koneksi**: permintaan rusak, klien yang pergi, batas waktu, dan penangan yang menaikkan galat hanya menutup koneksi itu; soket ditutup di `akhirnya` sehingga tidak bocor di jalur galat mana pun.
   - **Shutdown aman**: `berhenti()` menghentikan gelung **setelah** balasan saat ini terkirim, lalu soket server ditutup dan port dilepas.
   - **Log terstruktur**: lewat `pustaka/log.idk` yang sudah ada (pencatat bernama `web`), bukan mekanisme log kedua. Field tetap: metode, path, status, durasi. Badan dan header tidak pernah dicatat (kata sandi, cookie sesi).
   - **Konsolidasi**: lima salinan gelung soket tulisan tangan di repo ini dirapikan; empat di antaranya tidak punya batas waktu dan kini punya.
   - **Belum, dan sengaja tidak diklaim:** eksekusi paralel (thread/worker) — VM ini satu utas dan belum punya primitif thread; menambahkannya menyentuh GC, stack VM, dan tabel global, jadi itu pekerjaan sebesar fase tersendiri. Karena belum ada eksekusi paralel, **batas worker dan backpressure belum berlaku**: antrean dibatasi backlog `listen()` dan permintaan dilayani satu per satu. `jalankan()` **bukan** server produksi mandiri; pakai reverse proxy di depannya.
7. [ ] Buat contoh CRUD API memakai database Fase 10, uji dengan curl, serta CI untuk slow client, request berukuran besar, malformed HTTP, upload, redirect, sertifikat kedaluwarsa/tidak cocok, race/resource leak dan graceful shutdown.
   - Sebagian (1.5.0): **contoh CRUD API sudah ada** — `examples/api_siswa.idk`, menggabungkan `web.Aplikasi` + `app.jalankan()` + builtin `db_*` + `pustaka/json.idk`. Diuji dengan curl lewat HTTP sungguhan di `tests/api_siswa_server.sh` (16 pemeriksaan): tiap metode, 404 untuk PUT/DELETE ke id yang tidak ada, penolakan badan tidak sah, dan payload injeksi SQL.
   - Dua jebakan yang sengaja ditunjukkan contoh ini: `db_baris_terpengaruh()` sesudah UPDATE/DELETE (tanpa itu id yang tidak ada tetap dijawab `200`), dan parameter `?` alih-alih perangkaian teks SQL.
   - **Database Fase 10: terpenuhi 1.6.0** — `examples/api_siswa_pg.idk` adalah CRUD API yang sama di atas **PostgreSQL**, diuji lewat HTTP sungguhan dengan curl di `tests/api_siswa_pg_server.sh` (12 pemeriksaan): POST 201, GET/PUT/DELETE, **404 di ketiga metode** untuk id yang tidak ada, badan tanpa field wajib 400, JSON rusak 400, dan sandi tidak bocor ke log. Kredensial dibaca dari lingkungan, tidak ditulis di berkas contoh.
   - CI yang sudah jalan: **slow client** (`tests/web_batas_waktu_server.sh`), **graceful shutdown** (`tests/web_jalankan_server.sh`), **request besar + malformed HTTP** (`tests/web_badan_server.sh`), **redirect** (`tests/http_pengalihan_server.sh`).
   - **upload multipart**: sudah diuji lewat HTTP sungguhan (`curl -F`) di `tests/web_badan_server.sh`, bukan hanya terhadap teks.
   - **sertifikat kedaluwarsa/tidak cocok**: ditutup `tests/https_klien.sh` (1.5.0) — empat kasus badssl.com ditolak dengan sebab spesifik.
   - **resource leak**: ditutup `tests/kebocoran_resource.sh` — fd dihitung dari `/proc` sesudah 90 permintaan campuran di jalur yang biasanya bocor (penangan meledak, klien putus di tengah, badan besar). Hasil: 4 → 4, tidak ada kebocoran.
   - **race**: masih BELUM dan sengaja tidak diklaim. Eksekusi satu permintaan pada satu waktu, jadi belum ada balapan untuk dideteksi; ini baru bermakna setelah ada worker/thread.
   - Temuan saat menulis uji beban, diperbaiki di 1.5.0: parsing badan `urlencoded` ternyata **kuadratik** (100 KB = 10,7 detik), cukup untuk membekukan server lewat satu POST 1 MB. Akarnya `teks_gabung()` yang merakit teks per potongan; dipindahkan ke builtin C. Dijaga `tests/web_badan_performa.sh` yang memeriksa bentuk pertumbuhan, bukan ambang detik.
8. [x] Dokumentasikan deployment dengan Docker/reverse proxy TLS, isolasi proses, pembatasan resource dan risiko membuka port secara publik.
   - Selesai 1.5.0: bab panduan `website/panduan/08_DEPLOYMENT.md`, dengan konfigurasi nyata di `docker/aplikasi/` (`Dockerfile` dua tahap + pengguna non-root UID 10001, `docker-compose.yml`, `nginx.conf`).
   - **Semua contohnya dijalankan sungguhan**, bukan ditulis dari ingatan: image dibangun, stack dinyalakan, permintaan dikirim lewat proxy, lalu isolasi diperiksa dari luar — aplikasi tak terjangkau dari host, `wget` keluar gagal (`bad address`), proses berjalan sebagai UID 10001, `/app` read-only. Batas laju terbukti dengan 40 permintaan beruntun → 24 × `200` + 16 × `503`; `client_max_body_size` terbukti dengan unggahan 2 MB → `413`.
   - **Isolasi & batas**: `read_only`, `cap_drop: ALL`, `no-new-privileges`, `pids_limit`, `mem_limit`, `cpus`, aplikasi tanpa `ports:` di jaringan `internal: true`. Pola ini diambil dari `docker/playground/` yang sudah terbukti di repo ini, bukan dari praktik umum yang diingat-ingat.
   - **Risiko port ditulis apa adanya**, termasuk bahwa `docker run -p` **menembus `ufw`** (Docker menulis aturannya di chain `DOCKER` yang diproses sebelum `INPUT`), dengan mitigasi mengikat `127.0.0.1:...`.
   - **TLS** dijelaskan dengan blok nginx nyata + `certbot`; stack bawaan tetap HTTP karena sertifikat butuh domain publik, dan itu dinyatakan terang-terangan.
   - **Dua bug nyata ditemukan justru karena contohnya dijalankan**: (a) `indk` tidak bisa dibangun di musl/Alpine sejak `soket_batas_waktu()` (`<sys/time.h>` tak ter-include) — ini juga merusak `docker/images/Dockerfile` yang sudah ada, dan lolos karena **tidak ada job CI yang memakai musl**; sekarang ada job `test-musl`. (b) `soket_dengar()` selalu mengikat `127.0.0.1` sehingga server **tidak bisa dipakai di dalam container sama sekali**; ditambahkan argumen alamat opsional dengan bawaan tetap loopback.

**Target API (usulan):** app = web.Aplikasi(); app.rute("GET", "/api/siswa", fungsi_handler); app.jalankan(port=8080). Sintaks final harus sesuai kemampuan compiler, bukan diasumsikan hanya karena mirip Python.

**Selesai jika:** server API bisa dipakai tanpa HTML, klien HTTPS memvalidasi sertifikat, parsing request aman dan tes integrasi lintas platform lulus.

### Fase 12 — HTML template engine dan Server-Side Rendering (SSR)

**Tujuan:** file .idk menjalankan logika backend, file .html berisi tampilan standar; server merender HTML dinamis tanpa SPA/React. Template engine dibuat independen dari server agar dapat diuji sendiri. **Status:** ✅ selesai — mesin templat (SSR-01…06), integrasi respons web (SSR-07), cache parsing (SSR-08), diagnosis & ketahanan (SSR-09), dan contoh + dokumentasi (SSR-10) semuanya ada di `pustaka/templat.idk`, `pustaka/web.idk`, `examples/web_mahasiswa.idk`, dan `website/panduan/`. **Pemetaan:** SSR-01–10.

**Rekomendasi rilis fase: `v1.7.0` (minor), kandidat `v1.7.0-rc.1`; patch `v1.7.1` dan seterusnya jika hanya perbaikan kompatibel.** Ini target perencanaan, bukan janji jadwal atau tag yang sudah dibuat.\n\n**Gerbang rilis:** Template engine dengan escaping aman, loop/percabangan, include/layout dan integrasi SSR memiliki tes golden, XSS dan regresi.\n\n
**Langkah pengembangan:**

1. [x] Rancang docs/RENCANA_FITUR/selesai/ssr_template_engine.md. Definisikan modul terpisah (mis. pustaka/templat.idk atau engine native C jika hasil profil membutuhkannya), template root, render(nama,data), render_teks(sumber,data), error dan scope (**SSR-01**).
2. [x] Bangun loader dan parser yang memisahkan literal HTML, interpolasi {{ nama }} dan tag {% ... %}, dengan penanda path/baris/kolom, batas ukuran dan kedalaman (**SSR-02**).
3. [x] Implementasikan interpolasi variabel, akses kamus dan atribut yang dibolehkan, serta error untuk nilai yang tidak tersedia; **jangan evaluasi string menjadi kode .idk arbitrer** (**SSR-03**).
4. [x] Tambahkan kondisi dan loop bertingkat dengan sintaks yang konsisten, misalnya {% jika ... %} / {% selain %} / {% akhirjika %}, {% untuk ... %} / {% akhiruntuk %}, dan uji scoping (**SSR-04**).
5. [x] Tambahkan HTML auto-escaping **sejak awal interpolasi** (**SSR-06**); tangani &, <, >, kutip, dan aturan khusus atribut/URL; hindari menyatakan aman untuk ekspresi di JS/CSS inline. Konten HTML tepercaya hanya melalui API eksplisit yang diaudit. **Sebagian:** escaping `& < > " '` berlaku untuk isi elemen dan atribut ber-kutip, dan `templat.aman()` adalah satu-satunya jalan keluar. Escaper khusus URL (menolak `javascript:`) dan konteks `<script>`/`<style>` BELUM ada — batas ini dinyatakan eksplisit di panduan alih-alih disiratkan aman.
6. [x] Tambahkan include komponen, template inheritance dan block override; batasi path dalam root template dan cegah siklus (**SSR-05**). Tersedia lewat `{% sertakan "k.html" %}`, `{% perluas "induk.html" %}` dan `{% blok nama %}…{% akhirblok %}`. Nama templat wajib literal berkutip (nama dari data berarti pemanggil memilih berkas), siklus ditolak dengan menyebut rantainya, kedalaman sertakan dibatasi 32, dan `render_teks()` menolak `{% sertakan %}` karena tanpa akar tidak ada yang membatasi berkas yang dibaca.
7. [x] Integrasikan render ke response web Fase 11 dengan Content-Type text/html; charset=utf-8, status/headers yang tepat dan halaman error yang tidak membocorkan rahasia (**SSR-07**). Sebagian besar sudah dipenuhi `web.html()` + penangan galat bawaan (diverifikasi lewat soket, bukan diasumsikan): Content-Type benar, Content-Length dihitung **byte** bukan karakter, 500 hanya memuat "Galat internal" sedangkan nama berkas/variabel masuk stderr. Satu cacat nyata ditemukan dan diperbaiki: **HEAD ke rute ber-GET dibalas 405**, melanggar RFC 9110 §9.3.2 — `curl -I`, pemeriksa tautan, dan monitor uptime gagal di setiap halaman SSR. Kini HEAD dilayani penangan GET dengan header identik (termasuk Content-Length) dan badan dibuang; `Allow` pada 405 ikut menyebut HEAD. Diuji `tests/web_templat_server.sh` (17 asersi lewat soket, sabotase 8/8 merah).
8. [x] Tambahkan cache parsing, invalidasi dalam mode development, pembatasan memori dan kesetaraan hasil dengan mode tanpa cache (**SSR-08**). Selesai: `Mesin._node()` di `pustaka/templat.idk`, diuji `tests/templat_cache.sh` (14 asersi, sabotase 7/7 merah). Parsing terukur **73,7%** biaya tiap render (`indk profil`); dengan cache turun **−73,4%** per render. Satu penyimpangan dari rencana: invalidasi **tidak** dibatasi mode development — cap berkas (ukuran + waktu ubah) diperiksa setiap render, karena satu `stat` jauh lebih murah daripada ~29.000 instruksi parsing, dan mode yang harus dinyalakan manual adalah mode yang lupa dinyalakan.
9. [x] Buat tes diagnosis path/baris/kolom, input XSS, Unicode, nesting, template tidak ada, include cyclic, traversal, beban input besar dan sanitizer/GC (**SSR-09**). XSS, traversal, siklus, templat hilang, Unicode, dan nesting ternyata **sudah** terlindungi (`templat_sabotase.sh` + golden). Dua cacat nyata ditemukan dan diperbaiki: (a) pesan galat hanya menyebut `baris N` tanpa berkas — tak berguna dalam rantai `{% perluas %}`/`{% sertakan %}`, kini berformat `berkas:baris N:` ala kompiler dan selalu menunjuk berkas **terdalam**; (b) parsing **kuadratik** — batas 10.000 tag baru tercapai setelah **94 detik**, sehingga batas itu justru jalan DoS-nya, kini **linear** (8× input = 8,00× instruksi, 10.000 tag < 1 detik). Diuji `tests/templat_diagnosis.sh` (12 asersi, sabotase 8/8 merah). **Kolom ditunda** — `"baris " + ke_teks(baris)` muncul di 30 lokasi, 35 fungsi menerima parameter `baris`, 9 jenis node menyimpannya; manfaatnya kecil karena baris templat pendek. Sanitizer/GC tidak dikerjakan terpisah: escaping sudah dijamin SSR-06 dan kebocoran memori dijaga `kebocoran_resource.sh` + Valgrind di CI.
10. [x] Tulis contoh website dinamis menggunakan SQLite (listing dan detail mahasiswa), dokumentasi template di website/panduan/, serta panduan deployment dan troubleshooting (**SSR-10**). Dokumentasi templat ternyata **sudah** lengkap (§2.22 `07_PUSTAKA_STANDAR.md`); yang kosong adalah contoh tersambung dan deployment. `examples/web_mahasiswa.idk` menyambung SQLite + templat + router: listing `/mahasiswa`, detail `/mahasiswa/:id`, parameter diikat `?`, id tak dikenal → **404**, id non-numerik tidak meledak, nama berisi `<script>` ter-escape, satu kueri untuk daftar (bukan N+1). Golden `tests/expected/web_mahasiswa.out`; sabotase 3/3 merah (SQL dirangkai, 404→200, escaping dimatikan). `08_DEPLOYMENT.md` §8.8 baru: direktori templat wajib ikut `COPY`, cache aman untuk produksi, volume SQLite adalah direktori (bukan berkas, karena `-wal`/`-shm`), plus tabel troubleshooting gejala→sebab. Sekalian dibetulkan klaim usang "belum ada cache hasil parsing" di §2.22 — cache sudah ada sejak SSR-08.

**Contoh target:** `kembalikan templat.render("mahasiswa.html", {"nama":"Budi"})`; isi template: `<h1>{{ nama }}</h1>`. Sintaks ini sudah tersedia lewat `pustaka/templat.idk`.

**Selesai jika:** halaman server-side menampilkan data dinamis dari database, auto-escaping aktif, include/layout bekerja, diagnostics jelas dan seluruh tes aman lulus.

### Fase 13 — Keamanan dan fitur aplikasi web

**Tujuan:** membangun fondasi aplikasi bisnis yang membutuhkan input pengguna, login, session dan otorisasi. **Status:** ⬜ belum. **Pemetaan:** WEB-04, WEB-05, WEB-06.

**Rekomendasi rilis fase: `v1.8.0` (minor), kandidat `v1.8.0-rc.1`; patch `v1.8.1` dan seterusnya jika hanya perbaikan kompatibel.** Ini target perencanaan, bukan janji jadwal atau tag yang sudah dibuat.\n\n**Gerbang rilis:** Session, cookie, CSRF, autentikasi dan otorisasi mencapai acceptance tests serta audit keamanan dasar, tanpa klaim aman produksi absolut.\n\n
**Langkah pengembangan:**

1. [ ] Buat rencana docs/RENCANA_FITUR/web_security_auth.md, threat model (browser, cookie, CSRF, upload, input, session, proxy) dan matriks fitur minimum.
2. [ ] Implementasikan cookie HttpOnly, Secure, SameSite, expiry dan session storage yang dapat dikonfigurasi; rotasi ID saat login dan invalidasi logout (**WEB-04**).
3. [ ] Sediakan validasi input, output escaping sesuai konteks, proteksi CSRF pada endpoint state-changing dengan session-cookie, CORS yang dibatasi dan trusted-proxy yang eksplisit (**WEB-05**).
4. [ ] Implementasikan autentikasi, password hashing adaptif yang teruji (utamakan Argon2id jika dependensi tersedia), login/logout, role/permission, dan perlindungan brute force/rate limit (**WEB-06**).
5. [ ] Siapkan contoh aplikasi CRUD dengan login dan role (admin/pengguna), memanfaatkan database Fase 10, HTTP Fase 11, SSR Fase 12.
6. [ ] Tambahkan tes terhadap session fixation, CSRF, cookie flags, file upload berbahaya, validasi izin lintas pengguna, timing/lockout, secret leakage serta regresi keamanan.

**Selesai jika:** jalur login-logout, kontrol akses, session, CSRF dan validasi input diuji otomatis; tidak ada default credential atau penyimpanan password plaintext.

### Fase 14 — Package manager, perkakas proyek dan integrasi library

**Tujuan:** developer dapat memulai dan membagikan proyek backend tanpa menyalin modul secara manual. **Status:** ⬜ belum. **Pemetaan:** P3, DX-01, DX-02.

**Rekomendasi rilis fase: `v1.9.0` (minor), kandidat `v1.9.0-rc.1`; patch `v1.9.1` dan seterusnya jika hanya perbaikan kompatibel.** Ini target perencanaan, bukan janji jadwal atau tag yang sudah dibuat.\n\n**Gerbang rilis:** Manajer paket dengan lockfile/integritas, scaffolding dan instalasi reproducible lulus uji, termasuk kegagalan jaringan dan arsip berbahaya.\n\n
**Langkah pengembangan:**

1. [ ] Buat rencana docs/RENCANA_FITUR/manajer_paket.md: manifest, nama paket/namespace, versi semantik, lockfile, cache, URL registri, checksum/signature dan lokasi instalasi non-root.
2. [ ] Implementasikan perintah CLI inisialisasi proyek, tambah, hapus, daftar, perbarui dan pemulihan lockfile; pisahkan dependency resolution dari unduhan.
3. [ ] Rancang format paket yang dapat dipublikasikan dan registry/API dengan otorisasi, integritas artefak, kebijakan takedown dan mitigasi package-name squatting. **Registri publik belum perlu diluncurkan sebelum desain keamanan selesai.**
4. [ ] Tambahkan scaffolding aplikasi backend (**DX-01**): direktori modul, file konfigurasi, contoh HTTP dan database, tes awal, penggunaan environment serta perintah menjalankan/menguji.
5. [ ] Integrasikan paket backend dan SSR dengan manajer paket serta lockfile (**DX-02**) tanpa menghilangkan opsi memakai standard library bawaan.

#### Boilerplate library dan plugin untuk komunitas (bagian Fase 14)

**Prinsip:** developer pada umumnya menulis library/plugin langsung dalam bahasa Indonesiaku (\`.idk\`), seperti modul yang sudah ada di \`pustaka/\`. Implementasi C **tidak wajib**; C digunakan untuk ekstensi native atau integrasi rendah yang tidak disediakan API publik saat itu. Istilah "plugin" di sini berarti paket/modul tambahan yang diimpor pengguna; **jangan mengklaim dynamic native plugin loader/ABI C sudah tersedia**, sebab API embedding dan FFI baru direncanakan dalam Fase 15.

**Langkah implementasi (semuanya belum dikerjakan):**

1. [ ] Buat rencana \`docs/RENCANA_FITUR/boilerplate_library_plugin.md\` beserta spesifikasi manifest, nama paket, namespace, entry point, dependensi, lisensi, serta kompatibilitas versi interpreter. Audit dahulu aturan \`impor\` dan mekanisme paket yang **sudah** ada agar template menghasilkan kode valid.
2. [ ] Siapkan **template library murni Indonesiaku** dengan struktur minimal \`pustaka/<nama>.idk\`, \`tests/\`, \`examples/\`, \`README.md\`, \`LICENSE\`, \`.gitignore\`, manifest paket yang nanti disepakati, dan workflow GitHub Actions sederhana.
3. [ ] Tambahkan **generator CLI interaktif maupun non-interaktif**, misalnya \`indk buat pustaka nama-paket\` dan \`indk buat plugin nama-plugin\` (nama final ditetapkan lewat spesifikasi). Validasi nama dan path, dukung opsi nama penulis/lisensi, serta aman terhadap overwrite dan path traversal.
4. [ ] Sediakan **template plugin integrasi aplikasi** yang menjelaskan kontrak fungsi publik, konfigurasi, hook/callback hanya bila didukung framework host, serta contoh penggunaan dengan \`impor\`; bedakan plugin pengguna dari plugin native.
5. [ ] Sediakan **template ekstensi native C (opsional)** setelah API embedding dan FFI Fase 15 benar-benar stabil; dokumentasikan build lintas OS, ABI, ownership objek/GC, error handling dan audit dependency. Bila Fase 15 belum selesai, tandai template native sebagai *planned* dan jangan memunculkannya sebagai opsi siap pakai.
6. [ ] Kembangkan template test runner \`indk uji\` dengan contoh \`uji_*\`, assertion, tes negatif, golden output dan CI Linux/Windows/macOS yang sesuai; contoh harus dapat berjalan pada versi interpreter minimum yang disebutkan.
7. [ ] Tambahkan generator dokumentasi awal: README dengan instalasi/pemakaian, referensi API, contoh import, changelog, panduan kontribusi dan metadata paket; tautkan ke portal referensi API pada Fase 19.
8. [ ] Integrasikan dengan **Package Manager Fase 14**: validasi manifest, build/paket, pemeriksaan dependensi, lockfile, pemasangan lokal, publikasi registri (hanya ketika infrastruktur dan keamanan siap), checksum/signature dan pencegahan paket berbahaya.
9. [ ] Dokumentasikan **panduan end-to-end**: membuat paket dari template → menulis fungsi dalam \`.idk\` → menjalankan tes → memanggilnya dari proyek lain → memublikasikan kode ke GitHub → mendistribusikan paket setelah registri tersedia.
10. [ ] Uji generator pada direktori kosong dan lintas OS: nama tidak valid, path traversal, file sudah ada, lisensi kosong, boilerplate yang bisa langsung dijalankan, linting, import lintas direktori, packaging reproducible, dan keamanan dependency; ukur waktu onboarding developer.

**Contoh struktur target library (rancangan, bukan struktur wajib yang sudah diimplementasikan):**

\`\`\`text
kalkulator-indonesiaku/
├── indonesiaku.toml        # nama/format manifest masih perlu disepakati
├── README.md
├── LICENSE
├── .gitignore
├── pustaka/
│   └── kalkulator.idk
├── examples/
│   └── penggunaan.idk
├── tests/
│   └── uji_kalkulator.idk
└── .github/
    └── workflows/
        └── test.yml
\`\`\`

**Target pengalaman pengguna:** perintah generator menyiapkan proyek library yang sudah memuat contoh \`fungsi tambah(a,b)\` dan dapat diuji dengan \`indk uji\` tanpa menulis C. Ekstensi C adalah jalur lanjut **opsional** setelah tooling Fase 15 tersedia.

**Kriteria selesai:** template dari nol bisa dihasilkan dan dijalankan di Windows/Linux/macOS, import berfungsi, tes bawaan hijau, dokumentasi akurat, publish/install lokal reproducible, serta fungsi yang masih direncanakan tidak ditampilkan sebagai tersedia.

6. [ ] Tambahkan tes konflik dependensi, paket berbahaya, checksum tidak cocok, install offline/cache, downgrade, path traversal arsip, reproducible install, serta Windows/Linux/macOS.
7. [ ] Dokumentasikan cara membuat, mempublikasikan, menginstal dan mengaudit dependensi paket serta lisensi pihak ketiga.

**Selesai jika:** proyek bersih dapat dibuat dan dependensi dipasang secara reproducible dengan lockfile, validasi integritas aktif dan tutorial telah diuji.

### Fase 15 — Debugger, embedding API dan FFI

**Tujuan:** memperluas ekosistem tooling dan integrasi native secara aman. **Status:** ⬜ belum. **Pemetaan:** P4, P5.

**Rekomendasi rilis fase: `v1.10.0` (minor), kandidat `v1.10.0-rc.1`; patch `v1.10.1` dan seterusnya jika hanya perbaikan kompatibel.** Ini target perencanaan, bukan janji jadwal atau tag yang sudah dibuat.\n\n**Gerbang rilis:** Debugger dan embedding API/FFI memenuhi tes debug, lifetime GC dan integrasi native; stabilitas ABI ditetapkan dan didokumentasikan.\n\n
**Langkah pengembangan:**

1. [ ] Rancang debugger dalam docs/RENCANA_FITUR/debugger.md: metadata baris/kolom, breakpoint, pause/step, inspeksi frame/variabel, protokol komunikasi editor dan overhead saat debug nonaktif.
2. [ ] Implementasikan eksekusi debugger terkontrol dan tes step in/over/out, exception, closure, generator, nested call, serta terminasi program.
3. [ ] Buat docs/RENCANA_FITUR/embedding_ffi.md: ABI C publik, lifecycle VM, registrasi fungsi native, konversi Value, error, multi-instance dan rooting GC.
4. [ ] Implementasikan API embedding terlebih dahulu; FFI umum menyusul hanya setelah batas keamanan, ownership memori dan sandbox disepakati.
5. [ ] Sediakan header publik, contoh host C, dokumentasi API, tes sanitizer, kompatibilitas compiler serta prosedur pemeliharaan ABI.

**Selesai jika:** debugging teruji tanpa merusak mode biasa, aplikasi C dapat memanggil Indonesiaku melalui API stabil yang terdokumentasi dan interaksi GC aman.

### Fase 16 — WebAssembly, performa dan perluasan ekosistem

**Tujuan:** memperkuat jangkauan platform dan kinerja **berdasarkan pengukuran**, bukan menambah kompleksitas sebelum perlu. **Status:** ⬜ belum. **Pemetaan:** P7 serta sisa Fase 7–8.

**Rekomendasi rilis fase: `v1.11.0` (minor), kandidat `v1.11.0-rc.1`; patch `v1.11.1` dan seterusnya jika hanya perbaikan kompatibel.** Ini target perencanaan, bukan janji jadwal atau tag yang sudah dibuat.\n\n**Gerbang rilis:** WASM playground lolos uji kesetaraan dan pembatasan resource; optimasi hanya disertakan bila hasil benchmark membenarkan.\n\n
**Langkah pengembangan:**

1. [ ] Buat rencana docs/RENCANA_FITUR/playground_wasm.md; siapkan Emscripten, bangun interpreter ke WebAssembly dan tentukan pembatasan fitur OS, file, network dan resource.
2. [ ] Jalankan playground .idk langsung dalam browser untuk kode lokal tanpa bergantung pada eksekusi server (**P7**); pertahankan Docker playground yang sudah ada sebagai alternatif.
3. [ ] Tambahkan tes kesetaraan hasil interpreter native vs WASM, timeout/memory guard, penanganan error dan perbedaan runtime browser.
4. [x] Profilkan program nyata lebih dulu sebelum menilai cache .idkc, computed goto dan optimasi lain. **Selesai (v1.4.0).** `tests/beban/*.idk` + `indk profil` + `perf`. Hasilnya: mask tabel hash dikerjakan (−43,8%); cache `.idkc` ditunda (kompilasi berkas terbesar hanya 1,8 ms); computed goto ditolak (sebaran opcode rata, 15 opcode untuk 94%); `OP_TRY_BEGIN` ditolak (0,12% pada beban nyata). Catatan lengkap di bagian Fase 7.
5. [ ] Rencanakan threading/asinkron, GUI, AOT/bundel executable dan penguatan sandbox sebagai subproyek terpisah; jangan klaim selesai tanpa spesifikasi, tes isolasi dan deployment yang aman.

**Selesai jika:** playground WASM bekerja pada contoh yang didukung, perbedaan fitur terdokumentasi, serta optimasi hanya digabung setelah benchmark dan regresi membuktikan manfaat.

### Fase 17 — Pustaka transfer berkas, SSH/SFTP, FTP dan unduhan

**Target kandidat:** \`v1.12.0-rc.1\`. **Rilis stabil rekomendasi:** \`v1.12.0\`, hanya bila seluruh acceptance tests fase ini lulus. **Status:** ⬜ belum.

**Tujuan:** menjadikan Indonesiaku berguna untuk otomasi server, manajemen file dan integrasi jaringan. Berikan API yang mudah dipakai layaknya \`shutil\`, \`pathlib\`, \`urllib\`, \`ftplib\` dan \`paramiko\` di ekosistem Python, tetapi tetap punya perilaku keamanan yang tegas. **Jangan menyatakan protokol tersedia hanya karena socket TCP sudah ada.**

**Langkah pengembangan:**

1. [ ] Tulis rencana \`docs/RENCANA_FITUR/file_transfer_remote.md\` dan inventarisasi fungsi \`src/sistem.c\`, \`pustaka/jalur.idk\`, \`pustaka/http.idk\`, serta builtin file I/O. Pilih konvensi error, path dan pengembalian nilai yang konsisten; hindari duplikasi fungsi yang sudah ada.
2. [ ] Tambahkan operasi file lokal: \`salin_berkas\` (cp), \`salin_direktori\` (cp -r), \`pindahkan\`, \`ganti_nama\`, \`hapus_berkas\`, \`hapus_direktori_bertingkat\`, \`jelajahi_direktori\`, \`berkas_sementara\`, dan pemeriksaan metadata/izin sesuai OS. Sediakan pilihan overwrite eksplisit, perlindungan symlink, dan error jelas.
3. [ ] Sediakan **download berkas HTTP/HTTPS**: fungsi target \`unduh(url, tujuan, opsi)\` dengan streaming (tanpa menampung semua data ke RAM), timeout, batas ukuran, progress callback jika aman, cancel, resume \`Range\` jika didukung server, redirect terkontrol, verifikasi TLS, checksum SHA-256 opsional, serta file sementara + atomic rename untuk mencegah file parsial. Bergantung pada HTTPS aman Fase 11.
4. [ ] Implementasikan **FTP/FTPS client** dengan daftar direktori, upload/download, rename/hapus, passive mode dan timeout. FTP plaintext hanya untuk kompatibilitas dan harus diberi peringatan nyata; **FTPS dengan sertifikat terverifikasi** menjadi pilihan aman. FTP/FTPS **bukan** SFTP.
5. [ ] Implementasikan **SSH client** memakai pustaka SSH terawat (mis. libssh2/libssh dengan review lisensi): koneksi, eksekusi perintah jarak jauh, exit status/stdout/stderr terpisah, autentikasi kunci dan password secara aman, timeout serta batas output; **validasi host key wajib secara default** (known_hosts/pinning), bukan auto-accept host tak dikenal.
6. [ ] Tambahkan **SFTP** di atas SSH untuk list/upload/download/rename/hapus, streaming file besar, operasi path aman, transfer atomik jika server mendukung; sediakan utilitas **SCP hanya bila dibutuhkan** dan jangan menyamakannya dengan perintah cp lokal.
7. [ ] Batasi fitur jaringan/file pada mode sandbox dan dokumentasikan kemampuan/profil izin secara eksplisit. Jangan eksekusi perintah shell otomatis untuk mengimplementasikan protokol; gunakan library native yang teruji.
8. [ ] Uji server dummy lokal dan container terisolasi untuk FTP, FTPS, SSH dan SFTP: auth salah, host key berubah, sertifikat kedaluwarsa, file besar, putus di tengah transfer, path traversal, symlink, kuota, timeout, penggunaan memori dan clean-up lintas platform. Pastikan tidak ada secret di log/error.
9. [ ] Tambahkan contoh \`.idk\` untuk salin direktori, unduh file dengan checksum, remote command dan backup lewat SFTP; perbarui panduan API, CHANGELOG, Docker/installer serta matriks dependensi Windows/Linux/macOS.

**API target ilustratif (belum tersedia):** \`berkas.salin("a.txt", "cadangan/a.txt")\`; \`jaringan.unduh("https://contoh.invalid/arsip.zip", "arsip.zip")\`; \`ssh.sambung(host, pengguna, kunci_privat)\`; \`sftp.unggah("lokal.txt", "/aman/lokal.txt")\`. Nama dan desain final diputuskan pada dokumen rencana.

**Selesai jika:** cp/file lokal, unduhan HTTPS, FTP/FTPS, SSH dan SFTP bekerja, sertifikat/host key diverifikasi, error ditangani, tanpa bocor secret, dan suite tes negatif + keamanan + lintas OS lulus.

### Fase 18 — Kriptografi modern dan penguatan keamanan runtime

**Target kandidat:** \`v1.13.0-rc.1\`. **Rilis stabil rekomendasi:** \`v1.13.0\` setelah audit dan semua security gates lulus. **Status:** ⬜ belum.

**Tujuan:** menyediakan primitif keamanan yang benar untuk pengembang *dan* mencegah program tidak tepercaya menyalahgunakan interpreter. Ini **berbeda** dengan fitur login/CSRF di Fase 13 dan fitur TLS/SSH di Fase 11/17; fase ini memperkuat fondasi bersama. Hash \`md5\`, \`sha1\`, \`sha256\` dan \`sha512\` yang sudah ada **bukan** berarti enkripsi modern telah tersedia.

**Langkah pengembangan:**

1. [ ] Buat \`docs/RENCANA_FITUR/kriptografi_keamanan_runtime.md\` dan threat model terpisah untuk (a) kode aplikasi tepercaya, (b) kode \`.idk\` dari pengguna tak tepercaya, (c) paket pihak ketiga, (d) input jaringan/berkas. Audit batas C↔VM, GC, parser, soket, sandbox, dan modul proses/sistem berkas.
2. [ ] Pilih library kriptografi yang terawat dan teraudit beserta strategi build/lisensi lintas platform. **Jangan membangun algoritme kriptografi produksi sendiri**. Buat modul dengan nama dan tipe yang jelas antara hashing, password hashing, key derivation, MAC, signing dan encryption.
3. [ ] Tambahkan **CSPRNG** untuk token/salt/key (mis. \`acak_aman(n)\`), dibedakan tegas dari PRNG \`acak()\` biasa. Test entropi/failure behavior sesuai penyedia OS dan hindari fallback ke rand().
4. [ ] Tambahkan **enkripsi terautentikasi (AEAD)** dengan primitive teruji seperti AES-256-GCM atau ChaCha20-Poly1305, API encrypt/decrypt yang menyertakan nonce, associated data, auth tag, dan **menolak reuse nonce** dalam contoh/panduan. Kunci jangan dicetak/di-log; kesalahan verifikasi wajib gagal tertutup.
5. [ ] Sediakan **HMAC-SHA-256, HKDF** dan password hashing adaptif (**Argon2id** atau alternatif yang dipilih berdasarkan dukungan library); jika dibutuhkan, tanda tangan Ed25519 dan verifikasi publik. Jelaskan bahwa MD5/SHA-1 tidak cocok untuk password atau tanda tangan keamanan baru.
6. [ ] Audit serta perkuat **memory safety** compiler/VM/native bindings: validasi panjang buffer, overflow bilangan, lifetime/GC roots, use-after-free, double free, unbounded recursion/allocations dan perlakuan input Unicode/binary. Fuzz scanner, parser, bytecode loader/disassembler dan parser protokol; uji ASan/UBSan/Valgrind.
7. [ ] Perjelas **sandbox/capability model**: tolak akses file/network/process/FFI bila tidak diizinkan; gunakan allowlist eksplisit untuk profil eksekusi tidak tepercaya. \`--sandbox\` sendiri **bukan batas keamanan setara container/OS sandbox**; gunakan isolasi proses, user tanpa privilege, limit CPU/memori/waktu, seccomp/AppArmor/OS primitives bila tersedia.
8. [ ] Terapkan **resource limits** dan cancellation: batas instruksi/runtime, memori, ukuran stack/rekursi, ukuran string/array/input dan jumlah koneksi; error harus dapat didiagnosis tanpa crash host. Tambahkan proteksi DoS pada fitur yang memproses input eksternal.
9. [ ] Perkuat **supply-chain security**: dependency pinning, scanning library C dan paket, pengujian instalasi biner, checksums/penandatanganan artefak sesuai kebijakan, disclosure kerentanan, review rilis dan panduan pembaruan dependency. Periksa interaksi dengan Fase 14.
10. [ ] Buat tes KAT (known-answer tests) kriptografi, tamper detection, wrong key/nonce, failure RNG, tes fuzz/sanitizer, serangan traversal, sandbox breakout, resource exhaustion dan regresi pada tiga OS. Dokumentasikan konfigurasi aman dan batas jaminan keamanan.

**API target ilustratif (belum tersedia):** \`kripto.acak_aman(32)\`; \`kripto.hmac_sha256(kunci, pesan)\`; \`kripto.enkripsi_aead(kunci, pesan, nonce, aad)\`; \`kripto.verifikasi_kata_sandi(kata_sandi, hash_simpan)\`. Spesifikasi final wajib mendefinisikan tipe byte, format output, penanganan secret, dan kompatibilitas.

**Selesai jika:** tes kripto/keamanan terpenuhi, semua API baru aman secara default, jalur eksekusi tak tepercaya memiliki batas isolasi yang terdokumentasi dan diaudit, serta tidak ada temuan kritis terbuka yang diketahui. **Jangan menjanjikan keamanan absolut.**

### Fase 19 — Kelengkapan pustaka dan dokumentasi berdasarkan kategori pengembangan web

**Inspirasi taksonomi:** kelompok materi pada W3Schools PHP Tutorial (https://www.w3schools.com/php/default.asp), khususnya PHP Forms, PHP Advanced, PHP OOP, MySQL Database, PHP XML, PHP AJAX dan PHP Reference. **Tujuannya paritas KEMAMPUAN yang berguna, bukan meniru PHP atau menyalin sintaks/fungsi PHP satu per satu.** Indonesiaku tetap berbahasa Indonesia dan mempertahankan sifat Python-like. **Status:** ⬜ belum. **Kandidat:** \`v1.14.0-rc.1\`. **Rilis stabil rekomendasi:** \`v1.14.0\`, hanya setelah syarat penerimaan terpenuhi.

**Ketergantungan:** Fase 10 (driver basis data), 11 (HTTP), 12 (SSR), 13 (form/session/keamanan), 17 (berkas/FTP/SSH), dan 18 (kriptografi) memberikan kemampuan inti. Fase 19 melengkapi kesenjangan modul dan dokumentasi, bukan mengulang implementasi fitur fase sebelumnya.

#### Matriks kelompok materi → fitur Indonesiaku

| Kelompok rujukan | Sudah ada / landasan | Yang perlu dilengkapi (tidak otomatis tersedia) | Fase pemilik |
|---|---|---|---|
| PHP Basics: variables, strings, numbers, conditions, loops, functions, arrays | ✅ bahasa inti, daftar/kamus, metode dan pustaka standar dasar | Audit kelengkapan fungsi string, angka, array/koleksi, format, iterasi dan konversi; prioritaskan pemakaian nyata, bukan kesamaan nama | 19 |
| PHP Forms | ⚠️ HTTP parsing demo | Form GET/POST, URL-encoded, multipart upload, validasi terstruktur, sanitasi/escaping output, field wajib, error per-field, CSRF | 11, 13, 19 |
| PHP Advanced | ✅ file I/O dasar, JSON, exception, callback, modul; ❌ sebagian server-oriented | Date/time lengkap dan timezone, upload aman, cookies/session, filter/validator, pengelolaan header, stream, iterator berkas besar, config/environment | 11, 13, 17, 19 |
| PHP OOP | ✅ kelas/objek, konstruktor, inheritance, abstract, static/decorator, MRO, magic methods, iterable | Audit kecocokan kebutuhan nyata (visibility, interface/protocol, traits/mixins, destructor/finalizer) dan putuskan melalui desain Python-like; **jangan memaksa fitur PHP yang bertentangan dengan model objek saat ini** | 19 (opsional setelah evaluasi) |
| MySQL Database | ✅ SQLite, prepared parameters dasar | MySQL/MariaDB/PostgreSQL, transaksi, query berparameter, migrasi, error terstruktur | 10 |
| PHP XML | ❓ harus diaudit terlebih dahulu, jangan klaim tersedia | Parser XML aman, traversal DOM/SAX/streaming, pembuatan XML, namespace, encoding, pembatasan resource, **disable external entities/DTD by default** untuk mencegah XXE | 19 |
| PHP AJAX | ✅ respons JSON dasar, REST server demo | Endpoint JSON produksi, query/form request, pagination/filter/search, CORS aman, status/headers, validation; browser memakai fetch JavaScript standar, tidak memerlukan sintaks AJAX khusus di bahasa | 11, 13, 19 |
| PHP Filesystem/Directory | ✅ baca/tulis berkas dan jalur dasar | Copy/move/recursive, glob, metadata, permission, temporary file, streaming, upload/download, path safety | 17 |
| PHP FTP | ❌ belum didukung | FTP/FTPS, SFTP/SSH dan validasi host key/TLS | 17 |
| PHP Security / Filters / Hash | ✅ hash SHA/MD5 dan sebagian sandbox; belum AEAD/password hashing modern | CSPRNG, AEAD, Argon2id, HMAC, input validators, XSS/CSRF, TLS, sandbox dan runtime memory safety | 13, 18 |
| PHP Reference | ✅ pustaka standar saat ini | Dokumentasi API terindeks per kategori, lengkap dengan parameter/tipe, return, exceptions, contoh runnable, batas platform dan peringatan keamanan | 19 |

**Catatan mengenai status:** simbol ✅ merujuk landasan yang sudah tercatat di source/roadmap; ❓ berarti perlu audit kode dan tes sebelum menyatakan tersedia. Matriks bukan bukti kesetaraan API PHP.

#### Langkah pengembangan Fase 19

1. [ ] **Audit katalog API aktual**: dari \`src/native.c\`, \`src/sistem.c\`, \`src/db.c\`, \`pustaka/*.idk\`, \`docs/SPESIFIKASI.md\` dan \`tests/\`. Buat \`docs/ANALISIS/KATALOG_API_DAN_GAP_WEB.md\` yang memetakan **ada / sebagian / belum / sengaja tidak diadopsi**, dengan tautan source dan tes. Jangan berasumsi satu halaman W3Schools = satu fitur baru.
2. [ ] **Daftarkan modul bernamespace** yang konsisten (misalnya \`formulir\`, \`validasi\`, \`tanggal_waktu\`, \`xml\`, \`koleksi\`, \`berkas\`, \`jaringan\`, \`keamanan\`); finalisasi nama setelah pemeriksaan konflik simbol dan keselarasan dokumentasi.
3. [ ] **Lengkapi validasi input umum**: email, URL, angka/rentang, panjang teks, field wajib, skema data sederhana dan pengumpulan error per-field; validasi bukan substitusi encoding output, parameter SQL, atau autentikasi.
4. [ ] **Lengkapi utilitas tanggal/waktu dan zona waktu**: parsing/format ISO 8601, UTC/lokal, timezone-aware datetime dan penanganan DST/offset berbasis data zona waktu yang terawat. Wajib lintas OS atau dokumentasikan batasnya.
5. [ ] **Implementasikan XML aman**: parse dokumen, atribut dan elemen, XML namespace, serialisasi, opsi streaming; batasi ukuran/kedalaman, tolak DTD/external entity secara default, uji XXE dan entity expansion.
6. [ ] **Audit & lengkapi utilitas standar**: koleksi (filter/map/reduce, grouping/sorting), teks Unicode, format dan konversi, jalur/glob, konfigurasi, environment dan error handling hanya jika belum ada; jangan membuat sinonim fungsi berlebihan.
7. [ ] **Contoh aplikasi kategori**: formulir pendaftaran ber-validasi, upload file, login/session, REST API + pencarian AJAX, aplikasi SQLite/MySQL/PostgreSQL, XML import/export, otomasi berkas dan SFTP. Setiap contoh harus menggunakan fitur yang benar-benar telah selesai di fase dependensinya.
8. [ ] **Bangun referensi API yang bisa dicari** di website/panduan: sidebar **Dasar Bahasa**, **Formulir Web**, **Fitur Lanjutan**, **Pemrograman Berorientasi Objek**, **Database**, **HTTP/REST API**, **Template SSR**, **XML/JSON**, **Transfer Berkas**, **Kriptografi & Keamanan**, **Referensi Fungsi**. Tiap halaman memuat ringkasan, kode .idk yang dapat dijalankan, versi pertama tersedia dan tautan tes. Fitur belum jadi wajib ditandai "direncanakan", bukan disajikan sebagai panduan aktif.
9. [ ] Uji per kategori pada \`tests/\`: valid/invalid input, Unicode, data besar, XML entity attacks, error file/jaringan, konsistensi hasil dan contoh yang di-copy dari panduan. Gunakan CI tiga OS, sanitizer dan fuzz target sesuai modul.
10. [ ] Perbarui \`docs/ROADMAP.md\`, \`CHANGELOG.md\`, spesifikasi bila semantik berubah, contoh, ekstensi VS Code bila ada API baru, dan matriks kompatibilitas. Jalankan semua release gates sebelum candidate/stable.

**Kriteria rilis v1.14.0:** matriks gap telah diaudit, semua API yang diklaim selesai memiliki pengujian dan contoh yang dapat berjalan, XML/input handling lolos security tests, referensi web mencerminkan kemampuan sebenarnya, dan tidak ada breaking change diam-diam. Dokumentasi boleh diterbitkan lebih cepat sebagai pembaruan situs **tanpa** menaikkan versi compiler jika hanya menambah konten.

**Prinsip:** kelompok kurikulum seperti W3Schools adalah **cara mengorganisasi fitur dan dokumentasi**, bukan keharusan menyalin keseluruhan fungsi PHP. Komponen HTML/SSR tetap di Fase 12, sedangkan API server, form, keamanan, database dan transfer ada pada fase pemiliknya.

#### Rencana implementasi portal dokumentasi resmi (bagian Fase 19)

**Tujuan:** website resmi Indonesiaku menyediakan **Tutorial** dan **Referensi API** yang terpisah tetapi saling terhubung, menggunakan navigasi kategori seperti W3Schools. Ini adalah pekerjaan dokumentasi website, **bukan** permintaan menambahkan fungsi yang belum ada secara otomatis.

**Tahapan untuk AI coding agent:**

1. [ ] **Audit website dan API:** periksa struktur \`website/\`, sistem navigasi/panduan saat ini, \`src/native.c\`, \`pustaka/\`, \`docs/SPESIFIKASI.md\` serta contoh dan tes. Buat katalog sumber kebenaran yang memetakan nama API ke kode sumber, signature, kategori, versi pertama didukung, contoh dan tes. Jangan menebak signature.
2. [ ] **Susun navigasi dua bagian:** (a) **Tutorial Indonesiaku** berdasarkan topik/proyek, dan (b) **Referensi Bahasa & Pustaka** untuk lookup API per kategori. Sidebar harus responsif di desktop/mobile, dapat dibuka/tutup, dan memperlihatkan lokasi halaman aktif.
3. [ ] **Kategori awal sidebar:** Dasar Bahasa; Fungsi Bawaan (Teks, Bilangan/Matematika, Koleksi, Tanggal/Waktu); OOP; File & Direktori; HTTP/HTTPS; REST API Server; SSR & Template HTML; Formulir Web; Database (SQLite, MySQL/MariaDB, PostgreSQL); XML/JSON; FTP/FTPS, SSH/SFTP & Unduhan; Kriptografi & Keamanan; Modul & Package Manager; CLI/Developer Tools; serta Indeks A–Z fungsi/kelas/modul. Tampilkan hanya halaman yang isinya benar-benar tersedia; kategori perencanaan harus diberi status yang jelas.
4. [ ] **Satu halaman referensi per fungsi/kelas/metode/modul yang relevan**, mengikuti template tetap: nama dan ringkasan, status (\`tersedia\`/\`eksperimental\`/\`direncanakan\`/\`deprecated\`), versi minimum, import yang diperlukan, signature sintaks aktual, parameter (nama/tipe/default/wajib), return type/nilai, exceptions, side effects, keamanan, kompatibilitas OS, contoh \`.idk\`, output yang diharapkan, related APIs, serta link ke source dan tes.
5. [ ] **Penulisan contoh berkualitas:** minimal satu contoh sederhana dan satu contoh penggunaan realistis untuk API penting (file, database, web, keamanan, network). Hindari contoh placeholder yang tidak dapat dijalankan; jangan menampilkan API konsep seperti \`web.render\` atau \`ssh.sambung\` sebagai API aktif sebelum benar-benar diimplementasikan.
6. [ ] **Pencarian dan indeks:** pencarian client-side berdasarkan nama fungsi, sinonim/topik, kategori dan deskripsi; indeks A–Z; tautan permanen/anchor; cross-link dari tutorial ke halaman API; breadcrumbs; daftar isi lokal untuk halaman panjang.
7. [ ] **Pemeliharaan dokumentasi secara otomatis:** metadata terstruktur (misalnya JSON/YAML/Markdown frontmatter) boleh dipakai sebagai sumber laman; sediakan skrip validasi yang menolak duplikasi slug, tautan rusak, halaman tanpa status/versi, API tersedia tanpa source/test, dan kode contoh yang tidak lolos interpreter. Reuse \`tests/test_panduan.sh\` atau perluas jalur CI sejenis.
8. [ ] **Kualitas UX dan aksesibilitas:** desain responsif, keyboard navigation, heading semantik, kontras warna, code highlighting \`.idk\`, tombol salin kode, dan "coba contoh" hanya jika playground mendukung fitur tersebut dan aman untuk dijalankan.
9. [ ] **Versioning dokumentasi:** tandai \`Sejak vX.Y.Z\` untuk fungsi yang benar-benar tersedia; berikan catatan perubahan, deprecations dan perbedaan versi. Tidak perlu mengklaim versi yang belum dirilis sebagai tersedia. Pertahankan halaman rujukan historis bila masih dibutuhkan.
10. [ ] **Publikasi dan audit:** verifikasi semua link, menu, layout pada mobile/desktop, search, rendering kode, SEO dasar dan kecepatan halaman; perbarui README/panduan situs/CHANGELOG sesuai relevansi. **Dokumentasi murni boleh dipublikasikan tanpa menaikkan versi interpreter**.

**Kriteria penerimaan:** setiap builtin publik dan modul standar yang terinventarisasi memiliki entri referensi atau justifikasi pengecualian; contoh yang diklaim runnable diuji otomatis; navigasi/pencarian berfungsi; semua status/versi sesuai implementasi; tidak ada dokumentasi yang menyarankan praktik tidak aman sebagai default.

**Contoh format satu entri referensi:** \`panjang(nilai)\` → kategori Fungsi Bawaan, deskripsi, argumen \`nilai\`, hasil bilangan, satu atau lebih contoh .idk, output aktual dari tes, versi mulai tersedia yang telah diverifikasi, dan link ke implementasi. Gunakan data repository untuk menulis fakta; jangan menebak.

### Fase 20 — Asynchronous I/O dan concurrency yang terukur

**Status:** ⬜ belum. **Target RC rekomendasi:** \`v1.15.0-rc.1\`. **Target stabil rekomendasi:** \`v1.15.0\`. **Ketergantungan:** VM/GC (fase awal), networking Fase 11, keamanan runtime Fase 18.

**Tujuan:** mendukung tugas I/O yang berjalan bersamaan tanpa memblokir seluruh layanan, terutama REST API, koneksi jaringan dan operasi file. Jangan menganggap kemampuan async yang baru otomatis aman untuk beban produksi atau setara multithreading.

1. [ ] Buat \`docs/RENCANA_FITUR/async_concurrency.md\`. Audit \`src/vm.c\`, frame stack, closure/upvalue, GC roots, \`src/sistem.c\`, socket serta generator \`hasilkan\`; uraikan state suspension/resumption dan semantik error.
2. [ ] Bandingkan event loop + coroutine, cooperative task scheduler dan worker process/thread. Pilih model paling sederhana yang kompatibel lintas platform; tentukan interaksi fungsi blocking yang sudah ada.
3. [ ] Spesifikasikan API \`tugas\`, \`tunggu\` atau padanan Indonesiaku, timeouts, cancellation, task groups, propagation exception dan cleanup resource. **Nama sintaks hanya usulan**: jangan menambah keyword sebelum kajian kompatibilitas.
4. [ ] Implementasikan nonblocking TCP/HTTP I/O atau adapter event-loop, timer serta penjadwalan adil (fairness). Tetapkan apakah file I/O, DNS dan database memerlukan worker atau adapter tersendiri.
5. [ ] Tetapkan batas concurrency, backpressure, error propagation, orderly shutdown, penutupan koneksi, dan disiplin kepemilikan/GC root pada tugas tertunda.
6. [ ] Tambahkan tes interleaving, timeout, cancel berulang, starvation, resource leak, stress GC, ASan/UBSan, Windows/Linux/macOS dan benchmark latency/throughput sebelum-sesudah untuk kasus I/O.
7. [ ] Perbarui panduan, contoh server concurrent, dokumentasi API serta petunjuk kapan menggunakan async vs program sinkron.

**Gerbang rilis:** API tidak membocorkan resource/GC roots, cancellation teruji, server tidak hang pada klien lambat, mode sinkron lama tetap kompatibel dan benchmark diumumkan tanpa klaim performa yang tidak terbukti.

### Fase 21 — Real-time, cache, komunikasi dan background processing

**Status:** ⬜ belum. **Target RC rekomendasi:** \`v1.16.0-rc.1\`. **Target stabil rekomendasi:** \`v1.16.0\`. **Ketergantungan:** Fase 10–11, 13, 18 dan 20.

**Tujuan:** aplikasi nyata memiliki WebSocket, Redis/cache, email, antrean tugas dan scheduler. Utamakan library \`.idk\` di atas primitive native yang sudah teruji; **tidak semua fitur harus dibuat di C atau menjadi keyword compiler**.

1. [ ] Buat rencana terpisah: \`docs/RENCANA_FITUR/websocket.md\`, \`redis_cache.md\`, \`email_smtp.md\`, \`background_jobs_scheduler.md\` dan matriks dependensi native vs \`.idk\`.
2. [ ] Implementasikan **WebSocket** untuk server dan klien: handshake, frame text/binary, ping/pong, close, batas ukuran pesan, autentikasi/origin yang aman, backpressure dan integrasi async. Uji malformed frame serta klien terputus.
3. [ ] Implementasikan **Redis** client beserta pooling yang terukur, timeout/TLS/auth, TTL, namespace key, cache-aside dan opsi session store. Hindari cache secrets sembarangan; uji gagal koneksi dan invalidasi.
4. [ ] Implementasikan **email SMTP** dengan STARTTLS/SMTP TLS, validasi sertifikat, auth aman, MIME/multipart, lampiran, batas ukuran, dan pencegahan header injection. Sediakan contoh notifikasi dan reset password tanpa menyimpan rahasia di kode.
5. [ ] Implementasikan **background job queue**: enqueue, worker, retry dengan exponential backoff, idempotency key, dead-letter/error handling, concurrency limit dan shutdown. Bedakan pengiriman *at least once* dari klaim *exactly once*; gunakan Redis/DB sebagai backend sesuai kebutuhan.
6. [ ] Implementasikan **scheduler** untuk tugas sekali jalan/berulang, timezone dan DST, locking bila lebih dari satu worker, kebijakan misfire, retry serta graceful shutdown.
7. [ ] Buat contoh teruji: chat WebSocket minimal, cache CRUD, email notifikasi, import data dalam job, dan backup terjadwal. Masukkan tes integrasi container terisolasi untuk Redis/SMTP serta negative/security tests.
8. [ ] Dokumentasikan batas dukungan produksi, konfigurasi environment, deployment worker terpisah dari web server, monitoring antrean dan batas resource.

**Gerbang rilis:** contoh WebSocket/cache/email/worker/scheduler berjalan sesuai scope yang dinyatakan; tes timeout, retry/idempotensi, keamanan transport, dan kegagalan layanan eksternal lulus lintas platform. Jika ada komponen belum siap, pisahkan rilis minor atau tandai tahap eksplisit; jangan mengklaim seluruh fase selesai.

### Fase 22 — Observability, profiling, kompatibilitas dan kesiapan produksi

**Status:** ⬜ belum. **Target RC rekomendasi:** \`v1.17.0-rc.1\`. **Target stabil rekomendasi:** \`v1.17.0\`. **Ketergantungan:** semua fase runtime, jaringan, database, web dan package manager yang relevan.

**Tujuan:** aplikasi Indonesiaku mudah didiagnosis, diperbarui, diaudit dan dipelihara. Kualitas, keamanan dan kompatibilitas adalah kegiatan **lintas fase sejak sekarang**, bukan alasan untuk menunggu Fase 22.

1. [ ] Buat \`docs/RENCANA_FITUR/observability_production.md\` serta kebijakan tertulis support matrix, compatibility policy, deprecation, security disclosure dan release lifecycle.
2. [ ] Sediakan **structured logging**: timestamp, level, module, request/task correlation ID, format dapat dibaca mesin, masking rahasia secara default, rotasi atau integrasi collector.
3. [ ] Sediakan **metrics dan tracing** dasar: latensi request, error rate, durasi query, queue depth, memori/GC, task/connections; tentukan pilihan interoperabilitas misalnya OpenTelemetry/Prometheus tanpa mengunci satu vendor.
4. [ ] Implementasikan **profiler** CPU/wall time, alokasi memori, GC pause, hot functions dan diagnostik kebocoran; jaga overhead profiling opt-in dan dokumentasikan akurasi pengukuran.
5. [ ] Susun pedoman deployment **production readiness**: konfigurasi environment/secrets, health/readiness endpoint, graceful shutdown, reverse proxy, TLS, batas resource, backup/restore, observability, container isolation dan prosedur recovery.
6. [ ] Tetapkan kompatibilitas API dan versioning: SemVer, stabil/eksperimental, minimum supported runtime, deprecation warning, jendela dukungan patch/LTS bila sumber daya tersedia, kebijakan minor vs major dan perubahan format paket.
7. [ ] Tegakkan proses keamanan: \`SECURITY.md\`, pelaporan kerentanan privat, threat-model update, CI security scan/dependency audit, SBOM bila relevan, release provenance/checksums, penanganan CVE dan komunikasi advisory.
8. [ ] Lakukan pengujian beban/stress, chaos/network failure, CPU/memory profiling, regresi lintas OS/arsitektur, canary RC, dan verifikasi installer/Docker/artefak sebelum rilis; publikasikan metodologi dan hasil.
9. [ ] Tambahkan dokumentasi operasional dan referensi API untuk log/profiler/metrics; contoh dashboard/monitoring hanya jika sumber data yang mendasarinya telah diimplementasi.
10. [ ] Audit *production readiness* secara jujur: tandai fitur yang masih eksperimen, tidak didukung, atau membutuhkan reverse proxy/isolasi; jangan menyebut runtime aman produksi tanpa bukti.

**Gerbang rilis:** logging/metrics/profiler dapat digunakan dan diuji; compatibility/security policies tersedia; CI dan pengujian integrasi lulus; perubahan breaking teridentifikasi; panduan operasional dan troubleshooting dapat diikuti dari instalasi bersih.

### Penekanan keputusan rilis Fase 20–22 dan sesudahnya

- **v1.15.0, v1.16.0, v1.17.0 hanyalah contoh milestone minor** bila scope masing-masing fase matang. Tidak ada target tanggal; kualitas dan keamanan lebih penting daripada mengejar nomor versi.
- **Minor (\`1.x.0\`):** fitur baru yang kompatibel dengan program lama, dengan dokumentasi, tes, paket rilis dan changelog. Pecah fase besar menjadi beberapa minor tambahan bila diperlukan, lalu **perbarui tabel target** agar tidak memberikan gambaran palsu.
- **Patch (\`1.x.y\`, y≥1):** perbaikan bug/keamanan/kompatibilitas tanpa breaking change; rilis secepat layak berdasarkan risiko, tidak harus menunggu selesainya fase atau fitur baru.
- **Prerelease:** \`-alpha.1\` untuk eksperimen komunitas, \`-beta.1\` untuk API yang mendekati final, dan \`-rc.1\` untuk kandidat stabil; semua prerelease harus jelas diberi label dan tidak mengganti channel \`latest\` stabil.
- **Major (\`v2.0.0\`):** hanya ketika benar-benar memerlukan perubahan yang memutus API publik, sintaks, model objek, ABI native, format bytecode yang dipublikasikan, atau kontrak paket. Sebelum major: sediakan migration guide, deprecation path jika memungkinkan dan compatibility tests. **Tidak wajib** naik major hanya karena jumlah fase besar.
- **Bila fase bergantung pada fitur yang belum selesai:** jangan terbitkan versi stabil dengan klaim paritas penuh. Rilis komponen independen boleh jika scope rilis transparan, dependency/security gates terpenuhi, dan status fase tetap sebagian.
- **Checklist release blocker:** CI gagal, tes sanitizer/fuzz kritis gagal, regresi terhadap contoh lama, kehilangan data, sertifikat tidak diverifikasi, kredensial bocor, tidak ada tes untuk API baru atau installer gagal pada mesin bersih → tunda rilis.
- **Rilis pendamping:** pembaruan website/dokumentasi saja tidak memerlukan versi interpreter baru; modul/paket independen sebaiknya memiliki versi sendiri yang kompatibel dengan \`indk\` minimum, bukan memaksa setiap plugin menaikkan versi interpreter.
- **Keputusan final tag:** maintainer mengevaluasi cakupan aktual dan \`CHANGELOG.md\`, membandingkan artefak CI dengan Git tag, menyamakan \`src/common.h\`, Docker image/tag, installer, checksum, situs serta release notes. Update minor/patch/major berdasarkan **perubahan nyata**, bukan sekadar milestone roadmap.

### Fase 23 — Data biner, streaming, kompresi dan arsip

**Status:** ⬜ belum. **Kandidat rilis rekomendasi:** \`v1.18.0-rc.1\`. **Rilis stabil rekomendasi:** \`v1.18.0\`. **Ketergantungan:** model nilai/GC, sistem berkas, HTTP Fase 11, SSH/SFTP Fase 17, dan kriptografi Fase 18.

**Tujuan:** memiliki abstraksi \`byte\`, buffer dan stream yang benar (bukan memaksakan teks UTF-8 untuk semua data), supaya pustaka jaringan, database BLOB, file besar, enkripsi, serta arsip dapat digunakan secara andal.

**Langkah pengembangan:**

1. [ ] Buat \`docs/RENCANA_FITUR/data_biner_stream_arsip.md\`. Audit tipe \`Value\`, \`ObjString\`, operasi file/soket, SQLite BLOB dan dukungan binary existing; tetapkan semantik bytes immutable/mutable buffer, slicing, equality, serta interoperabilitas native tanpa merusak kompatibilitas \`teks\`.
2. [ ] Implementasikan representasi data biner dengan batas panjang, pengecekan overflow, lifecycle/GC roots, dan konversi eksplisit \`teks ↔ UTF-8 bytes\` yang menolak data tidak valid bila perlu.
3. [ ] Tambahkan API stream untuk baca/tulis bertahap, seek/tell bila didukung, flush, close idempoten, context manager, cancellation/timeouts yang relevan, serta bounded buffering/backpressure.
4. [ ] Implementasikan operasi buffer dan encode/decode: biner ↔ hex, Base64, endian integer packing/unpacking, serta checksum/hash streaming menggunakan fungsi/library yang telah diverifikasi; hindari duplikasi API yang sudah tersedia.
5. [ ] Tambahkan kompresi gzip/zlib serta arsip ZIP/TAR melalui pustaka terawat, mode streaming saat memungkinkan, inspeksi entri sebelum ekstraksi, batas ukuran/rasio/jumlah berkas, perlindungan path traversal, symlink dan decompression bomb.
6. [ ] Integrasikan dengan download/upload HTTP, SFTP, BLOB database, kriptografi dan contoh backup agar berkas besar tidak dibaca penuh ke RAM.
7. [ ] Tambahkan tes roundtrip byte acak, NUL embedded, Unicode/invalid UTF-8, data besar, korupsi arsip, zip slip, symlink, rasio kompresi ekstrem, resource limits, stress GC, ASan/UBSan dan tiga OS.
8. [ ] Perbarui pustaka, dokumentasi API, contoh, changelog, editor (bila sintaks baru), dan matriks platform/dependensi distribusi.

**Contoh API ilustratif (belum tersedia):** \`bita.dari_teks("halo", "utf-8")\`, \`berkas.buka_biner("data.bin")\`, \`arsip.ekstrak_aman("backup.zip", "hasil/", batas)\`.

**Gerbang rilis:** uji roundtrip dan resource safety lulus, ekstraksi aman secara default, benchmark memori streaming terbukti, serta kompatibilitas string dan aplikasi lama tidak berubah.

### Fase 24 — Unicode, i18n, konfigurasi dan format pertukaran data

**Status:** ⬜ belum. **Kandidat rilis rekomendasi:** \`v1.19.0-rc.1\`. **Rilis stabil rekomendasi:** \`v1.19.0\`. **Ketergantungan:** Fase 23 untuk encoding/binary, Fase 10–12 untuk aplikasi nyata, dan pustaka standar yang sudah ada.

**Tujuan:** memudahkan pengembangan aplikasi lintas bahasa/locale serta konfigurasi dan pertukaran data dengan format yang umum, tanpa mengubah sintaks inti Indonesiaku.

**Langkah pengembangan:**

1. [ ] Buat \`docs/RENCANA_FITUR/unicode_i18n_konfigurasi.md\`; audit fungsi \`teks\`, \`tanggal_waktu\`, \`jalur\`, JSON, CSV, XML (Fase 19), serta \`lingkungan\` yang sudah ada; identifikasi kesenjangan dengan tes, bukan asumsi.
2. [ ] Definisikan operasi Unicode yang eksplisit: normalisasi NFC/NFD/NFKC/NFKD, case folding, kategori Unicode, whitespace, code point vs grapheme cluster; pertahankan semantik \`panjang()\` historis tanpa breaking change.
3. [ ] Sediakan fasilitas locale/i18n yang teruji: format angka/mata uang/tanggal, pluralisasi, timezone data yang diperbarui, message catalog dan fallback; dokumentasikan dukungan locale OS yang berbeda.
4. [ ] Tambahkan parser/serializer YAML dan TOML dengan batas ukuran/kedalaman, error lokasi, tipe yang terdefinisi dan keamanan; gunakan parser YAML **safe mode** secara default, tanpa konstruksi objek arbitrer.
5. [ ] Tambahkan loader konfigurasi terstruktur: environment variables, \`.env\` (opsional sesuai kebutuhan), berkas TOML/YAML/INI serta prioritas sumber dan validasi skema; jangan menulis secrets ke log atau memaksa konfigurasi global yang mengejutkan.
6. [ ] Evaluasi MessagePack dan serialisasi biner lain sebagai paket opsional; tetapkan kebijakan kompatibilitas format, tipe/precision, dan keamanan deserialisasi tanpa eksekusi kode.
7. [ ] Uji input multilingual, emoji, combining marks, kasus Turkish locale, DST, file konfigurasi rusak, YAML bombs/tags, serialisasi roundtrip, resource limits serta lintas platform.
8. [ ] Buat contoh aplikasi multibahasa dan konfigurasi lingkungan dev/test/production; lengkapi dokumentasi referensi dan migrasi.

**Contoh API ilustratif (belum tersedia):** \`unicode.normalisasi("NFC", teks)\`, \`lokalisasi.format_tanggal(waktu, "id-ID")\`, \`konfigurasi.muat("aplikasi.toml")\`.

**Gerbang rilis:** Unicode dan timezone teruji, parser aman untuk input tak tepercaya, tidak ada perubahan diam-diam pada perilaku teks lama, dan dokumentasi jelas membedakan fitur wajib vs opsional.

### Fase 25 — CLI framework, pengujian HTTP dan integrasi message broker/IoT

**Status:** ⬜ belum. **Kandidat rilis rekomendasi:** \`v1.20.0-rc.1\`. **Rilis stabil rekomendasi:** \`v1.20.0\`. **Ketergantungan:** Fase 11 (REST/HTTP), 14 (package manager), 17 (jaringan), 20–21 (async dan background jobs), serta Fase 23 untuk payload biner.

**Tujuan:** developer dapat membuat aplikasi CLI dan menguji API tanpa menulis boilerplate berlebihan, sambil menyediakan konektor komunikasi terdistribusi sebagai paket independen bila diperlukan.

**Langkah pengembangan:**

1. [ ] Buat \`docs/RENCANA_FITUR/cli_http_test_broker.md\`; audit \`pustaka/opsi.idk\`, \`indk uji\`, HTTP client/server, dan \`pustaka/\` lain agar tidak menduplikasi argparse/testing yang sudah ada.
2. [ ] Bangun CLI framework di atas \`opsi\`: subcommand, bantuan otomatis, validasi opsi, default, exit codes, autocomplete shell (jika feasible), konfigurasi dan contoh \`indk buat ...\`; pertahankan akses ke CLI standar tanpa framework.
3. [ ] Sediakan test client HTTP untuk pengujian aplikasi REST tanpa membuka port publik bila arsitektur memungkinkan; assertions status, headers, JSON, cookies, auth, upload, snapshot/golden output dan mock dependency.
4. [ ] Tambahkan fixtures dan mock/fake untuk database, SMTP, waktu, filesystem dan request; uji isolasi dan determinisme agar \`indk uji\` tetap dapat dipakai pada CI.
5. [ ] Rancang konektor **MQTT** untuk IoT dan **AMQP/RabbitMQ** untuk antrean sebagai **library/plugin opsional**, bukan dependensi wajib runtime; definisikan TLS/auth, QoS, reconnect/backoff, delivery semantics dan batas resource.
6. [ ] Tambahkan contoh CLI pengelola data, integration test REST CRUD dan demo publish/subscribe MQTT pada broker uji lokal. Jangan memerlukan broker publik untuk menjalankan tes otomatis.
7. [ ] Uji protokol rusak, timeout, kredensial salah, reconnect, sertifikat tidak valid, duplicate delivery, penanganan payload biner dan cleanup.
8. [ ] Terbitkan panduan CLI/API testing/integrasi broker, contoh plugin, matriks kompatibilitas versi minimal Indonesiaku serta review dependency native.

**Contoh API ilustratif (belum tersedia):** \`cli.Perintah("siswa")\`, \`uji_http.klien(app)\`, \`mqtt.sambung("localhost", 1883)\`.

**Gerbang rilis:** CLI/test client dapat digunakan dari aplikasi contoh, fungsi konektor yang diumumkan lulus tes lokal, TLS dan autentikasi aman, dan library tambahan memiliki versi serta dokumentasi yang tidak mengunci semua pengguna ke broker tertentu.

### Milestone lintas fase — Validasi aplikasi dunia nyata (wajib sebelum klaim production-ready)

**Status:** ⬜ belum. **Bukan otomatis Fase 26 dan bukan tag rilis tersendiri.** Milestone ini harus dilakukan bertahap ketika dependensi masing-masing aplikasi siap, lalu dinilai lagi sebelum menyebut ekosistem siap dipakai produksi.

1. [ ] **Sistem administrasi mahasiswa SSR:** login/logout, session dan CSRF, CRUD siswa, database SQL dengan query berparameter, validasi form, upload file, template auto-escaping, authorization serta tes browser/HTTP. Fokus dependensi Fase 10–13, 18, 23.
2. [ ] **REST API server:** CRUD JSON, autentikasi, paging/filter, PostgreSQL/MySQL/SQLite sesuai skenario, logging terstruktur, validasi input, tes integrasi dan beban, graceful shutdown. Fokus dependensi Fase 10–11, 13, 20–22.
3. [ ] **Otomasi server:** copy/backup arsip, download HTTPS dengan verifikasi hash, SSH/SFTP dengan host-key verification, scheduler, enkripsi data, retry/error recovery dan observability. Fokus Fase 17–18, 21–23.
4. [ ] Tulis \`docs/RENCANA_FITUR/validasi_aplikasi_nyata.md\` dan matriks gap untuk setiap aplikasi: **tersedia**, **sebagian**, **belum**, **bug**, **tidak didukung**. Setiap gap harus mempunyai tes reproduksi dan owner/issue, bukan hanya uraian aspiratif.
5. [ ] Tetapkan acceptance tests: semua alur penting selesai tanpa runtime Python/PHP/Node.js, kecuali browser asset/tooling opsional yang terdokumentasi; tidak ada bypass keamanan; deployment dilakukan pada lingkungan terisolasi dengan rollback dan backup; seluruh contoh dan CI lulus.
6. [ ] Ukur resource CPU/RAM, latensi dan failure rate, error message, pengalaman onboarding developer, dokumentasi, packaging, observability dan kompatibilitas lintas platform. Publikasikan batas hasil serta faktor eksternal.
7. [ ] Gunakan hasil validasi untuk **memperbaiki atau menunda fitur**; jangan menambah fase sekadar meniru semua API bahasa lain. Bila API/semantik perlu breaking change, rencanakan deprecation/migration guide dan pertimbangkan \`v2.0.0\` sesuai kebijakan SemVer.

**Gate kualitas:** milestone dianggap selesai hanya bila ketiga aplikasi dapat dibangun, diuji, dan didemonstrasikan dengan source, instruksi reproduksi serta hasil terukur. Milestone ini bukan jaminan keamanan absolut maupun sertifikasi keamanan.

### Pengendalian ruang lingkup setelah Fase 25

- **Bekukan sementara penambahan fase baru** sampai ada bukti kebutuhan dari implementasi dan tiga aplikasi validasi. Prioritaskan menyelesaikan fase yang sudah ada, memperbaiki regresi, dokumentasi, keamanan dan kompatibilitas.
- Fitur yang bisa dibuat sebagai library \`.idk\` tidak perlu diubah menjadi builtin/keyword, kecuali benar-benar memerlukan primitive VM/native. Dependensi eksternal seperti MQTT/RabbitMQ adalah **opsional**.
- **Aturan rilis:** target \`v1.18.0\`, \`v1.19.0\`, \`v1.20.0\` hanyalah usulan minor per fase, bukan janji tanggal atau batas versi. Alpha/beta/RC dipakai sesuai kesiapan; patch keamanan boleh diterbitkan segera tanpa menunggu fase; major hanya untuk breaking change yang terencana.
- **Jangan mengklaim fase selesai** bila sebagian subfitur/tes/doc tertunda. Jika scope dibagi menjadi beberapa rilis minor atau ada fitur yang lebih dulu selesai, sinkronkan status, target versi dan CHANGELOG sesuai kenyataan.

### Checklist wajib untuk setiap fase (panduan untuk AI coding agent)

1. **Audit sebelum coding:** baca AGENTS.md, docs/SPESIFIKASI.md, docs/ANALISIS/, README, contoh dan source terkait; verifikasi fitur existing supaya tidak menduplikasi fungsi atau merusak kompatibilitas sejak 1.0.
2. **Rencanakan dahulu:** buat satu atau beberapa dokumen docs/RENCANA_FITUR/<fitur>.md memakai TEMPLATE.md; tuliskan kebutuhan, batas scope, alternatif desain, API, semantik error, keamanan, dampak VM/GC, daftar berkas, tes dan kriteria selesai.
3. **Kerjakan bertahap:** gunakan branch fitur/PR kecil; mulai dari API dan tes perilaku, lanjut implementasi, lalu integrasi. Jangan mencampur fitur antar-fase tanpa dependensi yang jelas.
4. **Perbarui konsumen:** contoh .idk, pasangan tests/expected atau tests/galat, tes integrasi, panduan website, editor VS Code, spesifikasi jika relevan, CHANGELOG, dan file rencana.
5. **Uji wajib:** jalankan semua golden/negative tests, sanitizer/DEBUG_STRESS_GC jika memodifikasi memori/objek/stack, Valgrind bila relevan, pengujian lintas OS, dan tes keamanan fitur network/database/template.
6. **Tinjau dan rilis:** ukur performa bila klaim optimasi, pastikan CI hijau, review kode dan keamanan, cantumkan perubahan API/breaking changes. Sejak v1.0, jangan melakukan breaking change sebelum kebijakan versi mayor memperbolehkan.
7. **Tandai status secara jujur:** [ ] belum; [x] selesai hanya setelah implementasi + pengujian + dokumentasi. Pindahkan dokumen rencana ke docs/RENCANA_FITUR/selesai/ sesuai AGENTS.md dan perbarui tabel status fase.

### Ringkasan status fase baru

| Fase | Fokus | Status |
|---|---|---|
| 9 | LSP dan dukungan editor | ✅ selesai (v1.4.0) |
| 10 | SQLite, MySQL/MariaDB, PostgreSQL | 🟨 sebagian — SQLite lengkap; **PostgreSQL** (wire protocol v3 + SCRAM-SHA-256) dan **MySQL/MariaDB** (prepared statement) jalan, diuji ke PG 16 dan MariaDB 11 sungguhan. MySQL 8 tak terverifikasi (CPU tanpa x86-64-v2). WDB-04 (`db.idk`) dan WDB-05 migrasi skema (`migrasi.idk`) selesai; query builder ditunda sengaja |
| 11 | HTTP/HTTPS dan backend server | 🟨 sebagian — langkah 1–6 dan 8 `[x]`; langkah 7 tinggal **race** saja (butuh worker/thread, Fase 20). CRUD di atas DB Fase 10 sudah terpenuhi lewat `examples/api_siswa_pg.idk` |
| 12 | Server-Side Rendering / HTML template | ⬜ belum |
| 13 | Session, keamanan, login dan otorisasi | ⬜ belum |
| 14 | Package manager dan developer tooling | ⬜ belum |
| 15 | Debugger, embedding C, FFI | ⬜ belum |
| 16 | WASM playground dan optimasi lanjut | ⬜ belum |
| 17 | cp/file, FTP/FTPS, SSH/SFTP dan download HTTP(S) | ⬜ belum |
| 18 | Enkripsi/kriptografi dan keamanan interpreter/VM | ⬜ belum |
| 19 | Kelengkapan pustaka, XML, validasi, dokumentasi ala kategori W3Schools | ⬜ belum |
| 20 | Async I/O, concurrency dan cancellation | ⬜ belum |
| 21 | Redis/cache, WebSocket, SMTP, scheduler dan background jobs | ⬜ belum |
| 22 | Logging, profiling, observability, kompatibilitas dan keamanan rilis | ⬜ belum |
| 23 | Byte/buffer, streaming, ZIP/TAR/gzip dan keamanan arsip | ⬜ belum |
| 24 | Unicode, i18n, konfigurasi, YAML/TOML dan serialisasi | ⬜ belum |
| 25 | CLI framework, HTTP test client, MQTT/RabbitMQ opsional | ⬜ belum |
| Lintas fase | Validasi 3 aplikasi nyata (SSR, REST API, otomasi server) | ⬜ belum |

## Standar branch dan kontribusi

Semua pekerjaan roadmap, baik oleh manusia maupun AI coding agent, mengikuti [Aturan Branch, Commit, dan Pull Request](ATURAN_BRANCH_DAN_PR.md). Pilih nama branch berdasarkan jenis pekerjaan (`fitur/`, `perbaikan/`, `docs/`, `keamanan/`, `test/`, `rilis/` dan lain-lain), bukan berdasarkan siapa yang mengerjakannya. Fase/target rilis tidak otomatis mengubah branch menjadi `rilis/`; gunakan `rilis/` hanya untuk persiapan publikasi versi. Merge membutuhkan review, pengujian dan pemenuhan kriteria fase.

## Cara memakai berkas ini

1. Untuk mengusulkan fitur: salin `RENCANA_FITUR/TEMPLATE.md`, isi, lalu
   tautkan dari butir fase yang relevan.
2. Saat fitur selesai: ubah status di sini + catat di `CHANGELOG.md`, lalu
   hapus/arsipkan berkas rencananya.
3. Jangan menyalin roadmap ini ke `README.md` atau berkas panduan.
