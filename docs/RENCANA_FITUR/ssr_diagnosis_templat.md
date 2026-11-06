# Rencana Fitur: diagnosis dan ketahanan templat (SSR-09)

- **Status:** dikerjakan
- **Target versi:** 1.7.0
- **Pemilik:** teguh02

## 1. Masalah

Butir SSR-09 meminta tes untuk sederet kasus. Diaudit satu per satu, bukan
diasumsikan:

| kasus | status sebelum |
|---|---|
| XSS | sudah diuji (`templat_sabotase.sh`) |
| traversal | sudah diuji |
| include cyclic | sudah diuji |
| template tidak ada | sudah diuji |
| Unicode | sudah diuji (golden + `templat_cache.sh`) |
| nesting | sudah diuji |
| **diagnosis path/baris/kolom** | **tidak ada** |
| **beban input besar** | **batas ada, tapi tak terpakai** |

Dua yang terakhir bukan sekadar tes yang kurang — keduanya cacat nyata.

**Diagnosis tanpa nama berkas.** Pesan galat hanya menyebut `baris 2`:

```text
anak.html     -> baris 2: 'tidak' tidak ada di data
sintaks.html  -> baris 2: bentuk harus '{% untuk x dalam y %}'
```

`anak.html` sendiri hanya **satu baris**; kesalahannya ada di
`bagian/komp.html` yang disertakannya. Dalam rantai
`{% perluas %}`/`{% sertakan %}`, "baris 2" tidak memberi tahu berkas mana
dari lima yang rusak.

**Batas yang terlalu lambat untuk melindungi.** `_T_MAKS_TAG = 10000` ada,
tetapi mencapainya butuh **94 detik**. Diukur:

```text
 2.000 tag :  4 detik
 4.000 tag : 12 detik     (2x input -> 3x waktu)
10.000 tag : 94 detik
```

Kuadratik, dua sebab terpisah:
1. `_t_baris_pada(sumber, titik)` dipanggil per tag dan **memindai ulang dari
   awal berkas** setiap kali.
2. Loop parser memanggil `_t_cari()` dua kali per iterasi (`{{` dan `{%`);
   pencarian yang tidak pernah ketemu memindai **seluruh sisa berkas** setiap
   tag.

Batas yang baru menolak setelah satu setengah menit bukan pelindung — itu
justru jalan DoS-nya.

## 2. Sintaks usulan

Tidak ada. Format pesan galat berubah, mengikuti kebiasaan kompiler:

```text
bagian/komp.html:baris 2: 'tidak' tidak ada di data
```

## 3. Semantik

- Galat ber-`baris N` diberi prefiks `namaberkas:`. Galat lain (siklus,
  traversal, templat tidak ditemukan, terlalu dalam) **tidak** — semuanya
  sudah menyebut berkas atau rantainya sendiri.
- Berkas yang disebut adalah tempat kesalahan **sebenarnya** (terdalam),
  bukan berkas terluar yang kebetulan paling akhir menangkapnya. Dijamin oleh
  sifat idempoten: setelah prefiks terpasang, pesan tidak lagi diawali
  `"baris "` sehingga berkas di atasnya tidak menimpa.
- Galat **parsing** ikut mendapat nama berkas: itu justru yang paling sering
  dilihat saat menulis templat.
- Parsing menjadi linear terhadap panjang sumber. Nomor baris tetap identik
  dengan sebelumnya.

## 4. Rencana implementasi

1. Bungkus isi `_render_berkas()` dengan `coba/kecuali GalatTemplat`, pasang
   prefiks di sana. Satu titik, bukan 40 lokasi `naikkan` yang masing-masing
   bisa lupa diperbarui.
2. `_t_baris_maju(sumber, awal, sampai, baris)` — hitung maju dari posisi yang
   nomor barisnya sudah diketahui; state disimpan di daftar `hitung` yang
   sudah dibagikan antar rekursi.
3. `_t_cari_tag(sumber, mulai, n)` — satu lintasan mencari `{` lalu memeriksa
   karakter berikutnya, menggantikan dua panggilan `_t_cari()`.

## 5. Rencana uji

`tests/templat_diagnosis.sh`:

- Galat render di komponen tersertakan menyebut komponennya, bukan halaman.
- Galat di **induk** menyebut induk, bukan berkas terluar (inilah yang
  membedakan "terdalam menang" dari "terluar menang" — tanpa kasus ini,
  sabotase idempotensi lolos).
- Galat sintaks (jalur parsing) ikut menyebut berkas.
- Siklus/traversal/templat hilang **tidak** diberi prefiks ganda.
- Batas nesting, batas tag, `{{` tanpa `}}`, blok tanpa penutup.
- XSS + Unicode lewat jalur berkas.
- **Pengukuran instruksi**: 8× input harus ~8× instruksi, bukan ~64×.

## 6. Yang TIDAK dikerjakan

**Kolom (`kolom N`) ditunda.** Roadmap menulis "path/baris/kolom". Path dan
baris dikerjakan; kolom tidak, karena biayanya tidak sebanding:
`"baris " + ke_teks(baris)` muncul di **30 lokasi**, **35 fungsi** menerima
parameter `baris`, dan **9 jenis node** menyimpannya — menambah kolom berarti
menyentuh seluruh rantai itu sekaligus mengubah 30 pesan dan setiap golden
yang mengasersinya.

Manfaat tambahannya kecil: baris templat umumnya pendek dan satu baris jarang
memuat dua tag yang sama-sama salah, jadi `berkas:baris` sudah menunjuk cukup
tepat. Dicatat sebagai sisa pekerjaan, bukan dianggap selesai diam-diam.
