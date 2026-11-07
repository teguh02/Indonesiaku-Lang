# 03 — Keputusan Desain yang Harus Dikunci Sebelum 1.0

Bahasa yang "stabil" berarti **program yang ditulis hari ini tetap berjalan
sama besok**. Karena itu, perubahan yang merusak kompatibilitas (breaking
change) harus dilakukan **sekarang**, di seri 0.x, selagi penggunanya masih
sedikit. Setelah 1.0, semantik di bawah ini dibekukan dan hanya boleh berubah
di versi mayor (2.0).

Setiap keputusan memiliki **rekomendasi**. Pemilik proyek menyetujui atau
mengganti rekomendasi; setelah disetujui, buat rencana di
`docs/RENCANA_FITUR/` dan tandai "disetujui" di sini.

**2026-10-24:** pemilik proyek menyetujui semua rekomendasi K1–K14 (blok
tetap `{ }` untuk seri 1.x). Perubahan yang merusak kompatibilitas mengikuti
kebijakan K14: dicatat di `CHANGELOG.md` dan didahului peringatan bila perlu
(mis. K3 diberi peringatan kompilasi di 0.5, berlaku di 0.6).

| ID | Keputusan | Rekomendasi | Merusak kompatibilitas? | Status |
|---|---|---|---|---|
| K1 | Truthiness | Ikuti Python | Ya | disetujui |
| K2 | Tipe angka | `bulat` + `desimal` | Ya (cetak `7/2`) | dikerjakan — `bulat` int64 ✅ (Fase 2), bignum 2b ⬜ |
| K3 | Scope assignment di fungsi | Lokal secara bawaan + `global`/`nonlokal` | Ya | disetujui |
| K4 | Kurung kurawal vs indentasi | Pertahankan `{ }` untuk 1.x | Tidak | disetujui |
| K5 | Semua error runtime bisa ditangkap | Ya, dengan kelas `Galat*` | Tidak | disetujui — galat bisa ditangkap ✅ (Fase 0); kelas `Galat*` di Fase 1 |
| K6 | Kunci kamus hilang | `GalatKunci` + `.ambil()` | Ya | disetujui |
| K7 | `==` pada list/kamus | Struktural | Ya | disetujui |
| K8 | Kunci kamus | Semua nilai hashable | Tidak | disetujui |
| K9 | Protokol iterasi | `untuk` atas list, teks, kamus, rentang, iterator | Tidak | disetujui |
| K10 | Metode pada tipe bawaan | Ya (`a.tambah(x)`) + fungsi lama tetap ada | Tidak | disetujui |
| K11 | `jenis()` | Kembalikan objek kelas | Ya | disetujui |
| K12 | `cetak` | Fungsi biasa, argumen dipisah spasi | Ya (multi-arg) | disetujui — multi-argumen ✅ (Fase 0); `pisah=`/`akhir=` di Fase 1 |
| K13 | String | UTF-8 di memori, indeks & panjang per karakter (code point) | Ya | selesai ✅ (Fase 0) |
| K14 | Kebijakan stabilitas & versi | SemVer + periode deprecation 1 minor | — | disetujui |
| K15 | Nilai bawaan parameter | Dievaluasi **tiap panggilan**, bukan sekali saat definisi | Ya (beda dari Python) | disetujui — selesai ✅ (Fase 1) |
| K16 | Argumen bernama | `f(b=2)`, boleh melewati parameter di tengah | Ya | disetujui — selesai ✅ (Fase 1) |
| K17 | Sintaks tupel & himpunan | Tupel `(1, 2)`/`(1,)`/`()` seperti Python; himpunan **hanya** lewat `himpunan([...])`, tanpa literal `{1, 2}` | Tidak | disetujui (2026-10-25) |
| K19 | Metode ajaib | Nama Indonesia berpola `_nama_` (06 §10), dicari di kelas; cerminan untuk perbandingan; `_sama_` membuat instansi tidak hashable | Tidak (perilaku baru; perbaikan NaN pada `>=`/`<=`/`!=`) | selesai ✅ (Fase 4) |
| K21 | Lingkup variabel `untuk` | Ikuti Python: di-assign seperti `x = elemen` (bukan lokal blok); variabel comprehension tetap privat | Ya (nama yang dibayangi kini tertimpa) | disetujui (2026-11-03) — selesai ✅ |

---

## K1 — Truthiness mengikuti Python

**Sekarang:** hanya `salah` dan `kosong` yang dianggap salah (warisan Lox).
**Usulan:** `salah`, `kosong`, `0`, `0.0`, `""`, `[]`, kamus kosong,
himpunan kosong, tupel kosong → salah. Objek dengan metode `_logika_` /
`_panjang_` mengikuti hasilnya.

**Alasan:** idiom Python `jika daftar { ... }` dan `selagi antrian { ... }`
adalah hal pertama yang disalin orang dari tutorial Python. Mengubahnya
setelah 1.0 tidak mungkin.

**Dampak:** satu fungsi `isFalsey()` di `vm.c`. Periksa `pustaka/*.idk` dan
`examples/` yang mungkin bergantung pada `0` bernilai benar.

## K2 — Dua tipe angka: `bulat` dan `desimal`

**Catatan pasca-implementasi (0.8):** `jenis()` sengaja tetap memulangkan
`"angka"` untuk `bulat` maupun `desimal`. Kedua tipe itu nyata di VM
(`VAL_INT`/`VAL_NUMBER`) tetapi **tidak dapat dibedakan dari dalam bahasa**
sampai builtin pembeda tiba. Alasannya: memecah `jenis()` menjadi
`"bulat"`/`"desimal"` adalah perubahan merusak kedua untuk setiap kode yang
membandingkan `jenis(x) == "angka"`, dan K2 sudah membawa satu perubahan
merusak. Pembedaannya menyusul bersama `ke_desimal`/`hash`/`turunan_dari`
(lihat sisa Fase 2 di ROADMAP), supaya pemakai hanya menyesuaikan sekali.

**Usulan:**

- `VAL_INT` (int64) ditambahkan di samping `VAL_NUMBER` (double).
- Literal tanpa titik → bulat; dengan titik/eksponen → desimal.
- `7 / 2 = 3.5` (selalu desimal), `7 // 2 = 3` (floor), `-7 // 2 = -4`,
  `-7 % 3 = 2` (mengikuti Python).
- Overflow int64 → fase 2b: naik otomatis ke bignum (`ObjBigInt`). Sebelum
  bignum ada, overflow menaikkan `GalatOverflow` — **jangan** diam-diam
  jadi double.
- Cetak desimal dengan algoritma *shortest round-trip* (`%.17g` lalu
  persingkat; atau port Ryu/Grisu) → `0.1 + 0.2` tercetak
  `0.30000000000000004`, `3.0` tercetak `3.0`.

**Catatan nama:** builtin `bulat` sekarang adalah pembulatan (round). Ganti
menjadi `bulatkan`, dan pertahankan `bulat` sebagai alias usang
(deprecated) satu versi.

## K3 — Assignment di dalam fungsi membuat variabel lokal

**Sekarang:** `x = 5` di dalam fungsi menulis ke **global**; kata kunci
`variabel` dibutuhkan untuk lokal. Ini kebalikan Python dan sumber bug
klasik ("kenapa variabel saya berubah di luar fungsi?").

**Usulan (sama dengan Python):**

- Di tingkat atas, `x = 5` → global (tetap).
- Di dalam fungsi, `x = 5` → lokal, **kecuali** dideklarasikan
  `global x` atau `nonlokal x`.
- `variabel x = 5` tetap diterima (sinonim eksplisit), tidak wajib.
- Membaca nama yang belum di-assign di fungsi tetap mencari global/upvalue.

**Implementasi:** compiler satu lintasan perlu tahu sebelum memancarkan
`OP_GET_*` apakah suatu nama akan di-assign di badan fungsi. Pilihan:
(a) pre-scan token badan fungsi untuk mengumpulkan nama yang di-assign
(murah, cocok dengan arsitektur sekarang), atau (b) pindah ke AST
(lihat `04_TEKNIK_LANJUTAN.md` §1). Rekomendasi: (a) untuk 1.0.

**Migrasi:** tambahkan peringatan kompilasi di 0.5 ("assignment ke global
`x` dari dalam fungsi; tambahkan `global x`"), ubah perilaku di 0.6.

## K4 — Kurung kurawal atau indentasi?

Python identik dengan indentasi. Namun Indonesiaku sudah memakai `{ }` di
seluruh contoh, panduan, pustaka, dan REPL.

| | `{ }` (sekarang) | Indentasi (Python) |
|---|---|---|
| Kemiripan dengan Python | Sedang | Tinggi |
| Pesan error untuk pemula | Jelas ("harapkan '}'") | Sering membingungkan (tab vs spasi) |
| Salin-tempel dari WhatsApp/web | Aman | Indentasi sering rusak |
| REPL multi-baris | Sudah ada | Perlu ditulis ulang |
| Biaya implementasi | 0 | Scanner INDENT/DEDENT + ubah semua contoh |

**Rekomendasi:** pertahankan `{ }` untuk seri 1.x — ini juga menjadi identitas
Indonesiaku ("mirip Python, lebih ramah salin-tempel"). Sisa token
`INDENT/DEDENT/COLON` di `scanner.h` dihapus agar tidak membingungkan.
Mode indentasi bisa dievaluasi ulang untuk 2.0 lewat rencana fitur
tersendiri.

## K5 — Semua error runtime menjadi pengecualian yang bisa ditangkap

**Sekarang:** `runtimeError()` langsung mencetak dan keluar (exit 70) untuk
indeks di luar jangkauan, variabel tak terdefinisi, tipe salah, dll.

**Usulan:** `runtimeError` diganti `throwGalat(KELAS, pesan)` yang membuat
instansi kelas pengecualian (hierarki di `02_PETA_FITUR_PYTHON.md` §F) dan
melakukan unwinding seperti `naikkan`. Hanya bila tidak ada handler, VM
mencetak traceback dan keluar 70. Pengecualian hanya untuk kegagalan internal
VM yang tidak boleh ditangkap (stack C rusak, kehabisan memori).

`kecuali` diperluas:

```
kecuali GalatIndeks sebagai e { ... }
kecuali (GalatTipe, GalatNilai) sebagai e { ... }
kecuali e { ... }          # tangkap semua (bentuk lama tetap sah)
```

## K6 — Kunci kamus yang tidak ada

**Usulan:** `d["x"]` untuk kunci tak ada → `GalatKunci`. Untuk perilaku lama
pakai `d.ambil("x")` (→ `kosong`) atau `d.ambil("x", bawaan)`.

## K7 — Kesetaraan struktural

`[1, 2] == [1, 2]` → `benar`; kamus dibandingkan per pasangan; instansi
memakai `_sama_` bila didefinisikan, selain itu identitas. Operator
`adalah` untuk identitas.

## K8 — Kunci kamus bisa berupa angka, bool, tupel, teks

Butuh fungsi hash per tipe dan `Table` yang berkunci `Value`, bukan
`ObjString*`. Tabel global VM tetap berkunci string untuk kecepatan.

## K9 — Protokol iterasi

`untuk x dalam <apa saja yang bisa diiterasi>`. Tahap 1: special-case di VM
untuk list, teks (per karakter), kamus (per kunci), rentang. Tahap 2 (fase 5):
objek dengan `_iter_`/`_berikutnya_` dan generator `hasilkan`.

## K10 — Metode pada tipe bawaan (terlaksana 0.7.0)

`"abc".huruf_besar()`, `daftar.tambah(4)`, `d.kunci()`. Implementasi: tabel
metode native per tipe (`vm.listMethods`, `vm.stringMethods`, ...) yang dicari
di `OP_INVOKE`/`OP_GET_PROPERTY` saat penerima bukan instansi. Fungsi lama
(`tambah(a, x)`) tetap ada agar kode lama tidak rusak.

Saat diimplementasikan ternyata lebih murah dari dugaan: setiap native yang
relevan **sudah** menerima penerimanya sebagai `args[0]`, dan `OP_INVOKE`
meninggalkan stack sebagai `penerima, arg1, ...` — persis tata letak yang
diharapkan native. Jadi yang ditambahkan hanya tabel pemetaan nama → native
per tipe; tidak ada logika teks/daftar/kamus yang ditulis ulang, tanpa opcode
baru, dan tanpa perubahan compiler.

Batas yang diambil sadar: metode bawaan hanya dapat **dipanggil**, belum bisa
diambil sebagai nilai (`variabel f = x.tambah`), karena `ObjBoundMethod` hanya
memuat `ObjClosure`. Ditolak dengan pesan yang menyebut cara benarnya.

## K11 — `jenis()` mengembalikan kelas

`jenis(5)` → kelas `bulat`; `cetak(jenis(5))` → `<kelas 'bulat'>`;
`jenis(x) == bulat` dan `adalah_instansi(x, bulat)` berfungsi. Tipe bawaan
menjadi objek kelas sungguhan (lihat 04 §4).

## K12 — `cetak` menjadi fungsi biasa (terlaksana 0.7.0)

`cetak(1, 2)` → `1 2`; `cetak()` → baris kosong;
`cetak("a", "b", pisah="-", akhir="")`. Butuh argumen bernama (fase 1).
`cetak` menjadi nilai biasa (bisa dioper ke fungsi lain).

Perbedaan cetak di dalam koleksi (seperti Python): `cetak("a")` → `a`,
tetapi `cetak(["a"])` → `["a"]` (pakai `repr` untuk elemen). Sekarang
tercetak `[a]`.

**Status 0.7.0:** `cetak` sudah menjadi native biasa dengan `pisah=`/`akhir=`,
dan `OP_PRINT`/`TOKEN_CETAK`/`printStatement()` dihapus. Argumen bernama untuk
native ditempuh lewat `vm.nativeArgNames` yang hanya diisi untuk native yang
terdaftar di `nativeTakesKeywords()` — native lain tetap menolak keyword,
sehingga `pisah=` pada fungsi yang tidak mengenalnya menjadi galat, bukan
diam-diam diabaikan.

**Selesai juga:** `repr` untuk elemen koleksi. Ternyata ada **dua** jalur
cetak (`printObject` di `object.c` dan `appendValueText` di `native.c`) plus
`repr`; memperbaiki satu saja akan membuat `cetak(x)` dan `ke_teks(x)`
berbeda. Ketiganya kini memakai satu helper `stringEscape()`. (Sejak K19,
Fase 4, ketiganya digabung menjadi satu formatter `formatValue`.)

Sekalian diperbaiki: kedua jalur mengiterasi tabel hash padahal `ObjDict`
menyimpan `keys` untuk urutan sisip, sehingga `cetak(d)` bisa berbeda urutan
dari `kunci(d)` dan dari iterasi pada kamus yang sama.

## K13 — String berbasis karakter

Simpan UTF-8, tetapi `panjang`, indeks, irisan, dan iterasi bekerja per code
point. Optimasi: tandai string "hanya ASCII" saat dibuat → indeks O(1);
selain itu indeks O(n) atau tabel offset tersimpan.

## K14 — Kebijakan stabilitas

- **SemVer.** 0.x: boleh breaking change, wajib dicatat di `CHANGELOG.md`
  bagian "Perubahan yang merusak". 1.x: tidak ada breaking change pada
  sintaks/semantik/builtin yang terdokumentasi.
- **Deprecation:** fitur yang akan dihapus mencetak peringatan ke stderr
  minimal satu versi minor sebelum dihapus.
- **Spesifikasi:** setelah K1–K13 diputuskan, tulis
  `docs/SPESIFIKASI.md` (tata bahasa EBNF + semantik). Tes golden menjadi
  *conformance suite* untuk spesifikasi itu.

## K15 — Nilai bawaan parameter dievaluasi tiap panggilan

Python mengevaluasi nilai bawaan **sekali**, saat `def` dijalankan. Akibatnya
`def f(a=[])` berbagi satu list untuk semua panggilan — jebakan *mutable
default argument* yang menghabiskan waktu pemula setiap tahun.

Indonesiaku mengevaluasi nilai bawaan **setiap kali fungsi dipanggil**:

```indonesiaku
fungsi kumpul(x, daftar=[]) {
    tambah(daftar, x)
    kembalikan daftar
}
cetak(kumpul(1))     # [1]
cetak(kumpul(2))     # [2]   — di Python [1, 2]
```

Konsekuensi yang diterima sadar:

- Nilai bawaan yang mahal dihitung ulang tiap panggilan.
- `fungsi f(x=waktu())` memberi waktu **saat panggil**, bukan saat definisi —
  justru yang biasanya diinginkan orang.
- Nilai bawaan boleh merujuk parameter di kirinya (`fungsi f(a, b=a)`), karena
  ia dijalankan di dalam frame yang slot sebelumnya sudah terisi.

Implementasi: prolog bersyarat. `call()` mengisi slot parameter yang tidak
diberikan pemanggil dengan penanda internal `VAL_TAKTERISI`, lalu tiap nilai
bawaan diawali `OP_ISI_BAWAAN slot, lompat` yang melewati ekspresinya bila
slot sudah terisi.

Versi pertama memakai **prolog bertingkat** (`ObjFunction.entryPoints`, lompat
langsung ke titik masuk sesuai jumlah argumen). Itu diganti saat argumen
bernama (K16) masuk: titik masuk tunggal hanya bisa mengisi **ekor**
parameter, sedangkan `f(1, c=9)` meninggalkan lubang di tengah.

**Jangan "perbaiki" ini menjadi perilaku Python.**

## K16 — Argumen bernama

`f(b=2)` mengirim nilai berdasarkan nama parameter. Argumen posisional wajib
mendahului argumen bernama (seperti Python).

Pencocokan nama terjadi **saat runtime**, bukan saat kompilasi, karena callee
adalah nilai runtime: `ObjFunction.paramNames` menyimpan nama parameter dan
`OP_CALL_BERNAMA` membawa daftar nama sebagai konstanta chunk. Mencocokkan
saat kompilasi untuk fungsi global yang kebetulan sudah dikenal ditolak —
hasilnya akan bergantung pada urutan definisi, dan menghasilkan dua jalur kode
untuk satu fitur.

Galat yang dihasilkan menyebut nama parameternya, bukan sekadar jumlah
argumen: `Parameter wajib 'a' tidak diisi.`, `Fungsi tidak punya parameter
bernama 'warna'.`, `Parameter 'a' diisi lebih dari sekali.`

Fungsi bawaan (native) belum menerima argumen bernama dan menolaknya dengan
pesan jelas. `cetak(pisah=, akhir=)` menunggu K12.

## K17 — Sintaks tupel dan himpunan

**Disetujui pemilik proyek, 2026-10-25.**

Kedua tipe ini memakai simbol yang sudah punya arti di Indonesiaku: kurung
`( )` untuk pengelompokan dan kurung kurawal `{ }` untuk blok serta literal
kamus. Keputusannya:

**Tupel — persis seperti Python:**

| Tulisan | Arti |
|---|---|
| `(1, 2)` | tupel dua elemen |
| `(1,)` | tupel satu elemen — **koma wajib** |
| `()` | tupel kosong |
| `(1)` | **bukan** tupel: tetap pengelompokan, nilainya `1` |
| `(1, 2,)` | koma di akhir boleh |

Dicetak juga seperti Python: `(1, 2)`, `(1,)`, `()`. Aturan `(1,)` memang
mengejutkan pemula, tetapi mengikuti Python berarti apa yang dipelajari di
sini langsung berlaku di sana — dan tanpa koma tidak ada cara membedakan
tupel satu elemen dari pengelompokan.

**Belum diputuskan — tupel tanpa kurung** (`x = 1, 2`, `kembalikan a, b`).
Di Python keduanya membuat tupel. Di Indonesiaku saat ini keduanya galat
kompilasi; hanya penugasan berganda `a, b = 1, 2` yang menerima koma tanpa
kurung. Ini usulan terpisah (paling berguna: `kembalikan a, b` untuk
mengembalikan beberapa nilai) dan baru dikerjakan setelah ada keputusan.

**Himpunan — hanya lewat konstruktor:**

```indonesiaku
hampa = himpunan()
angka = himpunan([1, 2, 2, 3])     # himpunan berisi 1, 2, 3
```

**Tidak ada literal `{1, 2}`.** Kurung kurawal di Indonesiaku sudah dipakai
untuk dua hal (blok dan literal kamus `{"a": 1}`, dibedakan oleh posisi —
lihat `AGENTS.md`). Arti ketiga akan membuat `{x}` ambigu (blok berisi `x`?
himpunan berisi `x`?) dan memaksa aturan pembeda tambahan di `dictLiteral`,
`prescanFunctionBody`, dan `skipBalancedBraces`, yang semuanya menghitung
kurung kurawal mentah. Python sendiri pun tidak punya literal himpunan
kosong (`{}` adalah kamus), jadi pemula memang harus belajar `set()`.

Karena tidak ada literal, himpunan **dicetak sebagai pemanggilan
konstruktor**: `himpunan({1, 2})` di Python menjadi `himpunan([1, 2])` di
Indonesiaku, supaya hasil cetaknya selalu kode yang sah untuk ditulis ulang.

**Dampak:** tidak ada perubahan pada aturan `{ }` (K4); pembeda tupel
cukup di `grouping()` (koma setelah ekspresi pertama).

## K18 — SQLite di-*vendor* sebagai amalgamation, bukan lewat FFI

**Keputusan:** `src/sqlite3.c` + `src/sqlite3.h` disalin apa adanya dari rilis
resmi sqlite.org dan ikut di-commit. Versi terkunci, SHA256 dicatat di
`src/SQLITE_PROVENANCE.md`. Berkas vendor **tidak pernah disunting** dan
dikompilasi tanpa `-Wall -Wextra` lewat aturan Makefile tersendiri.

**Alasan menolak alternatif:**

- *Menunggu FFI (rencana lama: Fase 8).* Tidak perlu. Amalgamation hanyalah
  satu berkas `.c` tanpa dependensi di luar libc; menahan basis data di
  belakang proyek FFI tidak punya pembenaran teknis. SQLite dimajukan ke
  Fase 3.
- *`-lsqlite3` dari sistem.* Membuat `make` butuh `apt install` di Linux dan
  lebih rumit lagi di Windows/Alpine, serta membuat hasil uji bergantung pada
  versi SQLite mesin. Harga vendoring (repo +~9 MB, pembaruan manual saat ada
  CVE) dibayar sekali dan ditanggung maintainer, bukan mahasiswa.

**Keamanan ikut diputuskan di sini, bukan ditunda:**

- Parameter `?` ada sejak versi pertama. Tanpa itu pengguna akan merangkai SQL
  dengan `+` dan setiap contoh yang diajarkan menjadi contoh SQL injection.
- Satu panggilan = satu pernyataan, divalidasi dengan `sqlite3_complete`
  **sebelum `sqlite3_prepare_v2`** — sebagian `PRAGMA` mengubah keadaan
  koneksi sudah saat prepare, bukan hanya saat step. Karena itu penolakan
  terjadi sebelum efek samping apa pun.
- Hanya `?` anonim; `:nama` dan `?1` ditolak agar pencocokan posisi parameter
  tidak pernah ambigu.
- `BLOB` dan `INTEGER` di luar ±2⁵³ **bergalat**, tidak dipaksakan ke
  `double`. Diam-diam merusak nilai jauh lebih berbahaya daripada menolak;
  keduanya dibuka kembali saat `bita` dan `bulat` int64 ada (Fase 2/6).

**Jebakan GC:** `db_ambil_semua` membangun daftar berisi kamus sambil
mengalokasi string tiap kolom. Daftar hasil, kamus baris, kunci, dan nilai
harus semuanya ter-root di stack VM saat `dictSet`/`writeValueArray` dipanggil
— keduanya bisa memicu GC. Ini pola `OP_BUILD_DICT`, tetapi bersarang dua
tingkat.

## K19 — Metode ajaib: nama dan aturan pemanggilan (selesai ✅, Fase 4)

Rencana: [`RENCANA_FITUR/selesai/metode_ajaib.md`](../RENCANA_FITUR/selesai/metode_ajaib.md).

**Nama.** Dunder Python diterjemahkan menjadi nama Indonesia yang diapit satu
garis bawah, mengikuti katalog 06 §10: `_teks_`, `_repr_`, `_sama_`,
`_tidak_sama_`, `_kurang_dari_`, `_kurang_sama_`, `_lebih_dari_`,
`_lebih_sama_`, `_tambah_`, `_kurangi_`, `_kali_`, `_bagi_`, `_bagi_bulat_`,
`_sisa_`, `_pangkat_`, `_negatif_`. "Kurang" sengaja dibedakan: `_kurang_dari_`
untuk `<`, `_kurangi_` untuk `-`. `init` tetap `init` (bukan `_init_`) karena
sudah dipakai sejak awal.

**Aturan yang dikunci:**

1. Dicari di kelas (termasuk warisan), tidak di field instansi — seperti
   Python mencari dunder di tipe.
2. Hanya instansi yang memicu metode ajaib; tipe bawaan tidak berubah.
3. Perbandingan memakai cerminan bila sisi kiri tidak punya metodenya
   (`a > b` → `b._kurang_dari_(a)`); `!=` tanpa `_tidak_sama_` adalah
   `tidak _sama_`. Aritmetika belum punya cerminan (`_ktambah_`) sampai ada
   penanda `TidakDiterapkan`.
4. `adalah` tetap identitas murni (K7).
5. Kelas yang mendefinisikan `_sama_` membuat instansinya tidak dapat di-hash
   sampai `_hash_` ada (Python: `__hash__ = None`), sehingga pencarian kamus
   dan himpunan tidak pernah menjalankan kode pengguna.
6. `_teks_`/`_repr_` wajib mengembalikan teks; bawaan `<objek Kelas>` di semua
   jalur cetak.

**Ditolak:** nama Python apa adanya (`__add__`) — bertentangan dengan prinsip
penamaan Indonesia; dua garis bawah (`__tambah__`) — lebih berisik tanpa
manfaat, karena nama berpola `_x_` belum dipakai siapa pun.

## K21 — Variabel `untuk` mengikuti lingkup Python (disetujui 2026-11-03)

**Masalah (T-U4, uji ekstensif 1.4.0):** `untuk x dalam ...` membuat lokal
*blok* baru. Variabel lama bernama sama tidak berubah (`k = 99; untuk k dalam
rentang(3) { }` → `k` tetap 99, hasil salah tanpa galat), dan nama itu tidak
terdefinisi setelah loop. Python me-rebind nama di lingkup yang melingkupi.

**Keputusan (pemilik proyek):** ikuti Python. Setiap elemen di-assign
persis seperti `x = elemen`:

- di tingkat skrip/modul → global (define-or-update);
- di dalam fungsi → lokal fungsi (K3), kecuali `global`/`nonlokal`;
- nama tetap terdefinisi setelah loop dengan nilai elemen terakhir;
- variabel **comprehension** tetap privat (Python 3), tidak berubah.

Loop atas urutan kosong tidak meng-assign apa pun. Di tingkat skrip,
membacanya menaikkan "tidak terdefinisi" (`NameError` di Python). Di dalam
fungsi nilainya `kosong`, sama seperti setiap lokal K3 yang assignment-nya
tidak dijalankan (Python: `UnboundLocalError`). Ini batasan K3 yang sudah
ada, bukan khusus `untuk`.

**Kompatibilitas:** merusak untuk kode yang sengaja membayangi nama di luar
dengan variabel loop. Tidak ada golden atau pustaka di repo yang bergantung
pada perilaku lama. Dicatat ⚠️ di `CHANGELOG.md`.

**Implementasi:** `forStatement` memakai `assignToName()` alih-alih
`addLocal`, dan `prescanFunctionBody` mendeklarasikan nama setelah `untuk` di
awal pernyataan sebagai lokal fungsi. `untuk` milik comprehension didahului
ekspresi, jadi tidak ikut terhitung.

## K22 — API embedding C memakai runtime global tunggal (Fase 8)

**Konteks:** Fase 8 menjanjikan "API embedding C". Seluruh mesin yang
dibutuhkan sudah ada dan teruji (`initVM`, `interpret`, `defineNative`,
`callFromNative`); yang belum ada hanya permukaan publik yang stabil.

**Masalah:** `src/vm.c` menyimpan VM sebagai **variabel global tunggal**
(`VM vm;`) dan seluruh berkas mengaksesnya langsung. API embedding yang
umum (Lua, Python) memakai handle per-runtime — `lua_State*`, `PyInterpreterState*` —
sehingga satu proses bisa memegang beberapa runtime terpisah.

**Pilihan yang dipertimbangkan:**

1. **Membongkar `VM vm` global menjadi `IndkState*`.** Menuntut mengalirkan
   pointer state ke hampir setiap fungsi di `vm.c`, `compiler.c`, `native.c`,
   `object.c`, dan `memory.c` — ribuan baris tersentuh, dengan risiko regresi
   GC yang tinggi, demi kapabilitas yang belum diminta pemakai mana pun.
2. **Menerima satu runtime per proses** dan menuliskannya sebagai batas.

**Keputusan: pilihan 2.** `src/indk.h` menyediakan fungsi bebas
(`indk_mulai`, `indk_jalankan`, `indk_selesai`) tanpa handle. Dua batas
dinyatakan terbuka di header, di panduan, dan di sini:

- **satu runtime per proses** — `indk_mulai()`/`indk_selesai()` boleh dipakai
  berulang secara **berurutan**, tetapi tidak dua runtime hidup bersamaan;
- **tidak aman untuk banyak utas** — VM ini satu utas dan tanpa kunci.

**Konsekuensi yang diterima:** program yang butuh dua interpreter terisolasi
harus memakai dua proses. Bila kebutuhan itu muncul nyata, pilihan 1 tetap
terbuka dan tidak terhalang oleh API ini — fungsi-fungsi di `indk.h` bisa
dipetakan ke versi ber-handle tanpa mengubah namanya.

**Keputusan turunan:** `indk_ambil_global_teks()` membaca
`vm.mainModule->globals` (tempat global skrip hidup sejak Fase 3), **bukan**
`vm.globals` yang hanya berisi builtin dan kelas galat. Pembacaannya memanggil
`formatValue(..., callMagic = false)` supaya membaca sebuah variabel tidak
pernah diam-diam menjalankan `_teks_` milik pengguna, yang bisa menaikkan
galat atau memanggil balik ke C di tengah pembacaan.

