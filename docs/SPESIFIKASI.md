# Spesifikasi Bahasa Indonesiaku

Versi dokumen: untuk rilis **1.0**.
Status: **normatif** — dokumen ini yang menentukan perilaku yang dijamin
stabil oleh kebijakan **K14** (lihat `ANALISIS/03_KEPUTUSAN_DESAIN.md`).

Setiap perilaku yang tertulis di sini sudah diverifikasi dengan menjalankan
interpreter, bukan dari ingatan. Contoh kode di seluruh dokumen ini dan di
`website/panduan/` diuji otomatis oleh `tests/test_panduan.sh`.

> **Janji kompatibilitas.** Sampai 2.0 tidak ada perubahan yang membuat
> program sah menurut dokumen ini berhenti bekerja atau berubah artinya.
> Menambah builtin, tipe, atau sintaks baru **diperbolehkan** karena bersifat
> aditif. Perilaku yang secara eksplisit disebut "tidak dijamin" di dokumen
> ini berada **di luar** janji tersebut.

---

## 1. Identitas bahasa

Indonesiaku meniru **semantik** Python, tetapi **tidak** sintaks bloknya.

- Blok dibatasi `{` dan `}` seperti PHP/C.
- **Tidak ada aturan indentasi sama sekali.** Spasi di awal baris tidak pernah
  berarti. Kode rata kiri seluruhnya sah, dan indentasi yang tidak konsisten
  bukan galat.
- Tidak ada `:` untuk membuka blok.
- Pernyataan berakhir di baris baru ketika tidak berada di dalam `(` `)` atau
  `[` `]`. Titik koma opsional.
- Komentar dimulai `#` sampai akhir baris.
- Berkas sumber adalah **UTF-8**. Identifier terbatas ASCII (`A-Z`, `a-z`,
  `_`, dan digit setelah karakter pertama); teks boleh berisi karakter apa pun.

## 1A. Kaidah arsitektur bahasa dan aturan unused (kebijakan desain)

**Norma yang telah berlaku:** Indonesiaku menjadikan Python inspirasi perilaku, bukan menyalin sintaks Python 100%. Kata kunci yang ditetapkan bahasa memakai Bahasa Indonesia. Batas blok selalu `{` dan `}` seperti gaya PHP/C untuk deklarasi fungsi, kelas/metode dan struktur kontrol; indentasi dan titik dua `:` ala Python tidak menjadi penanda blok. Ketentuan ini harus dijaga oleh setiap kontributor manusia atau AI agent.

**Arah desain tambahan — BELUM diklaim sebagai kontrak runtime v1.0/v1.x:** mengikuti disiplin pemakaian simbol yang terinspirasi Go. Deklarasi yang tidak digunakan harus mendapat diagnostik; lint/checker ditargetkan untuk memberi peringatan dengan lokasi dan saran menghapus/mengomentari deklarasi. Cakupan yang akan dirancang: variabel lokal, parameter, impor, fungsi dan kelas internal, serta simbol tingkat modul. Di Go, unused local variables/imports tertentu merupakan error, **bukan berarti Go melarang seluruh fungsi/kelas yang tidak dipanggil**.

**Keamanan kompatibilitas:** Jangan memberlakukan error kompilasi default untuk fungsi/kelas/variabel global yang tampak unused sebelum memahami penggunaan lintas-modul, API ekspor, callback, reflection, override, entrypoint atau akses dinamis. Prioritaskan peringatan `indk periksa` dan strict mode opsional; jadikan error default hanya setelah desain lengkap dan kajian perubahan kompatibilitas. Jika hal tersebut memutus perilaku sah seri v1.x, rencanakan untuk v2.0 dengan mekanisme migrasi. Pengecualian dan aturan `unused` yang spesifik harus dijabarkan pada dokumen rencana fitur sebelum implementasi.

**Contoh bentuk yang diwajibkan (ilustratif):**

```indonesiaku
fungsi hitung_total(a, b) {
    kembalikan a + b
}

kelas Kalkulator {
    fungsi tambah(a, b) {
        kembalikan a + b
    }
}
```

Tidak boleh menjadikan `def hitung_total(a, b):`, `class Kalkulator:` atau blok yang bergantung indentasi sebagai sintaks alternatif.

## 2. Kata kunci

Ke-35 kata berikut tidak dapat dipakai sebagai nama:

```
adalah      akhirnya    atau        benar       buang       bukan
coba        dalam       dan         dari        dengan      diri
fungsi      global      hentikan    impor       jika        jika_lain
kecuali     kelas       kembalikan  kosong      lanjut      lewati
naikkan     nonlokal    pastikan    salah       sebagai     selagi
selain      super       tidak       untuk       variabel
```

Konsekuensi: kata kunci tidak boleh menjadi komponen nama modul
(`impor paket.dalam` ditolak karena `dalam` adalah operator keanggotaan).

## 3. Tipe data

`jenis(x)` memulangkan nama tipe sebagai teks. Seluruh nama berbahasa
Indonesia dan sama dengan nama yang dipakai pesan galat.

| `jenis(x)` | Keterangan |
|---|---|
| `"angka"` | `bulat` (int64) dan `desimal` (double) — lihat §3.1 |
| `"teks"` | string UTF-8, tak dapat diubah |
| `"logika"` | `benar` / `salah` |
| `"kosong"` | ketiadaan nilai |
| `"daftar"` | `[1, 2]`, dapat diubah |
| `"tupel"` | `(1, 2)`, tak dapat diubah, dapat di-hash |
| `"himpunan"` | `himpunan([1, 2])`, dapat diubah |
| `"himpunan_beku"` | `himpunan_beku([1, 2])`, dapat di-hash |
| `"kamus"` | `{"a": 1}`, mempertahankan urutan penyisipan |
| `"rentang"` | `rentang(3)`, malas (tidak mengalokasi daftar) |
| `"iterator"` | hasil `iter(x)` |
| `"fungsi"` | fungsi, closure, native, metode terikat |
| `"kelas"`, `"objek"` | kelas dan instansnya |
| `"modul"` | hasil `impor` |
| `"berkas"`, `"db"`, `"soket"` | sumber daya sistem |

### 3.1 Dua tipe angka (K2)

`jenis()` memulangkan `"angka"` untuk keduanya; pembedanya adalah perilaku:

- `bulat` adalah int64. Luapan **menaikkan `GalatLuapan`**, tidak melingkar.
- `desimal` adalah IEEE-754 double. Dicetak selalu dengan `.0`
  (`7.0`, bukan `7`).
- `/` **selalu** menghasilkan desimal: `7 / 2` → `3.5`.
- `//`, `%`, `**` tetap bulat bila kedua operannya bulat.
- `1 == 1.0` benar (perbandingan numerik), tetapi `1 adalah 1.0` salah.

### 3.2 `logika` bukan angka

Berbeda dari Python, `logika` **bukan** subtipe angka. `benar + benar` adalah
galat, bukan `2`.

### 3.3 Teks sadar karakter, bukan bita

`panjang("héllo")` adalah `5` dan `"héllo"[1]` adalah `"é"`. Indeks dan
irisan bekerja pada karakter Unicode, bukan bita UTF-8.

## 4. Kebenaran (truthiness)

Mengikuti Python. Bernilai **salah**: `salah`, `kosong`, `0`, `0.0`, `""`,
serta daftar/tupel/himpunan/kamus/rentang yang kosong. Semua nilai lain benar,
termasuk setiap instans kelas.

## 5. Operator

| Kategori | Operator |
|---|---|
| Aritmetika | `+` `-` `*` `/` `//` `%` `**` |
| Perbandingan | `==` `!=` `<` `<=` `>` `>=` |
| Identitas | `adalah`, `adalah bukan` |
| Keanggotaan | `dalam`, `tidak dalam` |
| Logika | `dan`, `atau`, `tidak` |
| Bitwise | `&` `\|` `^` `~` `<<` `>>` |
| Himpunan | `\|` `&` `-` `^` |

Aturan yang dijamin:

- **`==` struktural, `adalah` identitas (K7).** `==` menelusuri isi daftar,
  tupel, kamus, dan rentang; urutan penyisipan kamus tidak memengaruhi hasil.
- `%` mengikuti tanda **operan kanan** seperti Python: `-7 % 2` adalah `1`.
- `//` membulatkan ke bawah: `-7 // 2` adalah `-4`.
- Rantai perbandingan berlaku: `1 < 2 < 3`.
- `dan`/`atau` melakukan hubung-singkat dan memulangkan **operan**, bukan
  logika.
- `+` menggabung dua daftar, dua tupel, atau dua teks. `*` mengulang
  daftar/teks dengan angka, dibatasi 65535 elemen.

## 6. Variabel dan lingkup (K3)

- Penugasan di tingkat atas membuat variabel global.
- **Penugasan di dalam fungsi membuat variabel lokal**, bahkan bila nama yang
  sama ada di luar. Gunakan `global x` untuk menulis variabel tingkat skrip,
  `nonlokal x` untuk menulis variabel fungsi pembungkus.
- `variabel x = ...` menamai lokal yang sama; keduanya setara.
- Variabel `untuk x dalam ...` di-assign seperti `x = elemen` (K21): ia
  menimpa nama yang sudah ada dan tetap terdefinisi setelah loop, dengan
  nilai elemen terakhir. Variabel comprehension tetap privat.
- `buang x` menghapus nama; membacanya lagi menaikkan `GalatNama`.

## 7. Alur kendali

```indonesiaku-takjalan
jika kondisi { } jika_lain kondisi2 { } selain { }
selagi kondisi { }
untuk x dalam urutan { }
```

- `hentikan` dan `lanjut` berlaku di `selagi` dan `untuk`.
- Bentuk ekspresi: `a jika kondisi selain b`.
- `untuk` menerima daftar, tupel, himpunan, kamus, rentang, teks, dan
  iterator.
- Komprehensi daftar: `[x * 2 untuk x dalam [1, 2, 3] jika x > 1]` → `[4, 6]`.

## 8. Fungsi

```indonesiaku
fungsi f(a, b=2) { kembalikan [a, b] }
variabel g = fungsi(n) { kembalikan n + 1 }
```

- Argumen bernama: `f(b=9, a=1)`.
- **Nilai bawaan dievaluasi setiap kali fungsi dipanggil**, berbeda dari
  Python. Jebakan *mutable default argument* tidak ada di sini.
- `kembalikan` tanpa nilai memulangkan `kosong`, begitu pula fungsi yang
  jatuh sampai akhir.
- Closure dan `nonlokal` didukung penuh.

## 9. Kelas

```indonesiaku
kelas A {
    init() { diri.x = 1 }
}
kelas B < A {
    init() {
        super.init()
        diri.y = 2
    }
}
```

- Metode ditulis **tanpa** kata `fungsi`.
- `diri` adalah penerima **implisit**, bukan parameter yang ditulis.
- `<` menyatakan pewarisan tunggal; `super.m()` memanggil metode induk.
- `adalah_instansi(x, K)` dan `turunan_dari(K1, K2)` memeriksa hierarki.

## 10. Galat

```indonesiaku
coba {
    cetak(1 / 0)
} kecuali GalatAritmetika sebagai e {
    cetak(e.pesan)
} akhirnya {
    cetak("selalu jalan")
}
```

- Hierarki berakar pada `Galat`; `GalatNilai`, `GalatTipe`, `GalatNama`,
  `GalatAritmetika`, `GalatPencarian`, `GalatIndeks`, `GalatAtribut`,
  `GalatLuapan`, `HentiIterasi`, dan lainnya adalah turunannya.
- `kecuali` tanpa tipe menangkap semua dan **wajib menjadi klausa terakhir**.
- Klausa dicoba berurutan; yang tidak cocok dilempar ulang ke `coba` di luar.
- `akhirnya` tetap berjalan saat `kembalikan`, `hentikan`, atau `lanjut`
  meninggalkan blok.
- `pastikan kondisi` dan `pastikan kondisi, "pesan"` menaikkan galat bila
  kondisi salah.
- Semua galat runtime dapat ditangkap. Galat yang tidak tertangkap dicetak ke
  stderr dan program keluar dengan kode **70**; galat kompilasi keluar dengan
  kode **65**.

**Batasan yang diketahui (bukan jaminan):** kelas galat **bawaan** belum punya
`init`, sehingga `naikkan GalatNilai("pesan")` belum bisa. Menaikkan galat
bertipe dengan pesan dilakukan lewat subkelas sendiri:

```indonesiaku
kelas GalatSaya < Galat {
    init(pesan) { diri.pesan = pesan }
}
coba {
    naikkan GalatSaya("pesan kustom")
} kecuali GalatSaya sebagai e {
    cetak(e.pesan)
}
```

Menambahkan `init` pada kelas galat bawaan bersifat **aditif**, sehingga boleh
dilakukan setelah 1.0 tanpa melanggar K14.

## 11. Modul

```indonesiaku-takjalan
impor matematika
impor matematika sebagai mtk
dari matematika impor mat_fpb, mat_kpk
dari matematika impor mat_fpb sebagai fpb
impor paket.submodul
```

- **Setiap berkas punya namespace global sendiri.** Dua modul boleh memakai
  nama yang sama tanpa saling menimpa.
- Pencarian berurutan: direktori berkas yang mengimpor → setiap entri
  `INDK_PATH` → `pustaka/`.
- `impor a.b` memuat `a/b.idk` dan menamainya `b`.
- Modul di-cache berdasar jalur hasil resolusi: berkas yang sama dieksekusi
  **sekali** walau diimpor lewat dua nama. Siklus impor aman.
- `__nama__` berisi `"__utama__"` pada berkas yang dijalankan langsung, dan
  nama modul bila diimpor.
- **Bentuk lama `impor "berkas.idk"` tetap berlaku** dan berbagi namespace
  dengan pemanggil. Bentuk ini dijamin sama sampai 2.0.

## 12. Pustaka standar

92 builtin tersedia tanpa impor. Daftar lengkap beserta tanda tangannya ada di
[`../website/panduan/07_PUSTAKA_STANDAR.md`](../website/panduan/07_PUSTAKA_STANDAR.md),
yang merupakan rujukan normatif untuk bagian ini.

Modul `.idk` tambahan ada di `pustaka/` (`matematika`, `teks`, `daftar`,
`kamus`, `json`, `http`) dan dapat diimpor bermodul.

## 13. Yang TIDAK dijamin

Hal berikut berada di luar janji kompatibilitas dan boleh berubah kapan saja:

- Teks persis pesan galat (yang dijamin adalah **kelas** galatnya).
- Format `repr` untuk tipe sumber daya (`berkas`, `db`, `soket`, `modul`).
- Urutan iterasi himpunan (urutan penyisipan adalah detail implementasi; pakai
  `terurut()` bila urutan penting).
- Batas internal: 256 konstanta per chunk, 64 penyalinan `akhirnya` sebaris,
  kedalaman rekursi, dan batas pengulangan 65535.
- Nilai `id(x)` dan `hash(x)` antar-proses.
- Perilaku bilangan desimal di luar IEEE-754 (`0.1 + 0.2 == 0.3` adalah
  `salah`, seperti di Python).

## 14. Referensi

- Panduan pengguna: [`../website/panduan/`](../website/panduan/)
- Keputusan desain (K1–K18): [`ANALISIS/03_KEPUTUSAN_DESAIN.md`](ANALISIS/03_KEPUTUSAN_DESAIN.md)
- Peta fitur Python: [`ANALISIS/02_PETA_FITUR_PYTHON.md`](ANALISIS/02_PETA_FITUR_PYTHON.md)
- Roadmap: [`ROADMAP.md`](ROADMAP.md)
