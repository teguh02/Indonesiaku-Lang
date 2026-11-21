# 📚 Referensi Pustaka Standar — Indonesiaku

Dokumen ini mendaftar semua **fungsi bawaan** (ditulis dalam C, selalu tersedia)
dan **pustaka standar** (ditulis dalam Indonesiaku, di folder `pustaka/`,
di-`impor` sesuai kebutuhan).

Kesalahan pada fungsi bawaan (argumen salah, berkas gagal dibuka, dll.) dapat
ditangkap dengan `coba/kecuali`, kecuali kesalahan internal seperti indeks di
luar jangkauan.

---

## 1. Fungsi Bawaan (Built-in)

### 1.1 Matematika

| Fungsi | Deskripsi |
|--------|-----------|
| `min(a, b, ...)` / `min(daftar)` | Nilai terkecil. Dua bentuk: beberapa argumen, **atau** satu urutan (daftar/rentang/teks) |
| `max(a, b, ...)` / `max(daftar)` | Nilai terbesar. Dua bentuk, sama seperti `min` |
| `abs(x)` | Nilai absolut |
| `akar(x)` | Akar kuadrat (x ≥ 0) |
| `pangkat(x, y)` | x pangkat y. `bulat ** bulat` (pangkat ≥ 0) tetap `bulat`, seperti operator `**` |
| `bulat(x)` | Pembulatan ke bilangan terdekat, **menjauhi nol** pada nilai ½ (`bulat(2.5) == 3`). Mengembalikan `bulat` |
| `bulatkan(x, digit=0)` | Pembulatan ke `digit` angka desimal dengan **banker's rounding** (`bulatkan(2.5) == 2`). Tanpa `digit` hasilnya `bulat`; dengan `digit` hasilnya `desimal` — baca peringatan di bawah |
| `lantai(x)` | Pembulatan ke bawah (floor). Mengembalikan `bulat`, seperti `math.floor` |
| `atap(x)` | Pembulatan ke atas (ceil). Mengembalikan `bulat`, seperti `math.ceil` |
| `bagi_sisa(a, b)` | Daftar `[hasil_bagi, sisa]` sekaligus, konsisten dengan `//` dan `%` |
| `biner(n)` | Teks representasi biner: `"0b1010"` |
| `oktal(n)` | Teks representasi oktal: `"0o12"` |
| `heksa(n)` | Teks representasi heksadesimal: `"0xff"` |
| `sin(x)`, `cos(x)`, `tan(x)` | Trigonometri, sudut dalam **radian** (seperti `math`). Hasil `desimal` |
| `asin(x)`, `acos(x)`, `atan(x)` | Kebalikan trigonometri → radian. `asin`/`acos` hanya menerima `[-1, 1]` (selain itu `GalatNilai`) |
| `atan2(y, x)` | Sudut titik `(x, y)` dalam radian, `-π..π` — kuadrannya benar, tidak seperti `atan(y / x)` |
| `radian(derajat)`, `derajat(radian)` | Konversi satuan sudut |
| `acak()` | Angka acak `[0, 1)` — **tidak untuk keamanan**, lihat catatan di bawah |
| `acak_bulat(min, maks)` | Bilangan bulat acak `[min, maks]` — **tidak untuk keamanan** |
| `acak_bita(n)` | Daftar `n` bita `0..255` dari sumber acak kriptografis sistem (`1 ≤ n ≤ 1048576`) |

#### `acak()` bukan untuk token, sesi, atau sandi

`acak()` dan `acak_bulat()` memakai generator cepat yang disemai dari **jam
sistem saat proses dimulai**. Barisannya bisa direproduksi: tiga proses `indk`
yang dijalankan pada detik yang sama mengembalikan angka yang sama persis.

```indonesiaku
cetak(acak_bulat(0, 999999999))   # proses 1 -> 440825032
cetak(acak_bulat(0, 999999999))   # proses 2 -> 440825032  (sama!)
```

Untuk aplikasi web itu berbahaya: penyerang tahu kira-kira kapan server
dinyalakan dari header `Date` di setiap respons, jadi ID sesi yang dibangun
dari `acak_bulat()` dapat ditebak dalam beberapa percobaan.

Pakai `acak_bita()` untuk apa pun yang tidak boleh tertebak — ID sesi, token
reset sandi, garam, nonce, kunci API:

```indonesiaku
variabel bita = acak_bita(16)      # 128 bit dari sumber acak sistem
variabel token = ""
untuk b dalam bita {
    token = token + heksa(b).ganti("0x", "")
}
```

Pembagian ini sengaja dibuat sama seperti `random` dan `secrets` di Python:
`acak()` tetap cepat dan dapat direproduksi untuk simulasi, `acak_bita()`
untuk keamanan. `acak_bita()` **tidak pernah diam-diam turun mutu** — bila
sumber acak sistem gagal dibaca, ia menaikkan galat.

`bulat`, `lantai`, `atap`, dan `bulatkan` tanpa `digit` selalu menghasilkan
`bulat`. Karena itu `nan` menaikkan `GalatNilai`, sedangkan `takhingga` atau
hasil di luar jangkauan int64 menaikkan `GalatLuapan`, seperti `math.floor`
di Python.

#### Trigonometri — radian, bukan derajat

```indonesiaku
cetak(sin(radian(30)))            # 0.49999999999999994 (desimal, bukan 0.5 persis)
cetak(bulatkan(cos(radian(60)), 10))   # 0.5
cetak(derajat(atan2(1, -1)))      # 135.0 -- kuadran II
coba {
    asin(2)
} kecuali GalatNilai sebagai e {
    cetak(e.pesan)                # asin(): argumen di luar domain [-1, 1]
}
```

`sin`/`cos`/`tan` menolak tak hingga dengan `GalatNilai`, sama seperti Python.

#### `min` / `max` — dua bentuk pemanggilan

```indonesiaku
cetak(min([5, 2, 9]))        # 2
cetak(max([5, 2, 9]))        # 9
cetak(max(5, 2, 9))          # 9
coba {
    cetak(max([]))
} kecuali e {
    cetak(e.pesan)           # max() dari urutan kosong
}
```

Bentuk lama `max(a, b, c)` tetap berlaku — tidak ada program lama yang rusak.
Bentuk baru menerima daftar, rentang, atau teks. Urutan **kosong** menimbulkan
galat, jadi periksa `panjang()` dulu bila daftarnya mungkin kosong.

#### ⚠️ `bulat` vs `bulatkan` — jangan tertukar!

Ini jebakan paling sering. Kedua fungsi membulatkan, tetapi **aturan pada nilai
tepat ½ berbeda**:

| x | `bulat(x)` (menjauhi nol) | `bulatkan(x)` (banker's rounding) |
|---|---|---|
| `0.5` | `1` | `0` |
| `1.5` | `2` | `2` |
| `2.5` | `3` | `2` |
| `3.5` | `4` | `4` |
| `-0.5` | `-1` | `0` |

```indonesiaku
cetak(bulat(0.5), bulat(1.5), bulat(2.5), bulat(3.5), bulat(-0.5))
# 1 2 3 4 -1

cetak(bulatkan(0.5), bulatkan(1.5), bulatkan(2.5), bulatkan(3.5), bulatkan(-0.5))
# 0 2 2 4 0
```

`bulatkan` memakai *banker's rounding* (pembulatan ke genap terdekat), persis
seperti `round()` di Python: nilai tepat ½ dibulatkan ke bilangan **genap**
terdekat, bukan selalu ke atas. Aturan ini sengaja dipakai di dunia keuangan
dan statistik karena tidak menggeser rata-rata ke atas saat banyak angka
dibulatkan.

**Pilih yang mana?**
- Butuh aturan sekolah ("½ dibulatkan ke atas")? Pakai **`bulat`**.
- Butuh hasil yang sama persis dengan Python, atau butuh argumen `digit`? Pakai **`bulatkan`**.

Hanya `bulatkan` yang menerima argumen kedua, yaitu jumlah angka di belakang
koma (boleh negatif untuk membulatkan ke puluhan/ratusan):

```indonesiaku
cetak(bulatkan(3.14159, 2))    # 3.14
cetak(bulatkan(1234.5678, -2)) # 1200.0  (ada 'digit' -> hasilnya desimal)
```

#### `bagi_sisa` — hasil bagi dan sisa sekaligus

```indonesiaku
hb = bagi_sisa(17, 5)
cetak(hb)                     # [3, 2]
cetak(hb[0], hb[1])           # 3 2
cetak(17 // 5, 17 % 5)        # 3 2  -- hasil yang sama
cetak(bagi_sisa(-7, 2))       # [-4, 1]
coba {
    bagi_sisa(5, 0)
} kecuali e {
    cetak(e.pesan)            # Pembagian dengan nol.
}
```

Hasilnya selalu konsisten dengan operator `//` (pembagian bulat ke bawah) dan
`%`, termasuk untuk bilangan negatif. Pembagi `0` menimbulkan galat.

#### `biner`, `oktal`, `heksa` — angka ke teks berbasis lain

```indonesiaku
cetak(biner(10))      # 0b1010
cetak(oktal(10))      # 0o12
cetak(heksa(255))     # 0xff
cetak(biner(-255))    # -0b11111111
coba {
    biner(1.5)
} kecuali e {
    cetak(e.pesan)    # biner() memerlukan bilangan bulat
}
```

Ketiganya mengembalikan **teks** (bukan angka) lengkap dengan awalan `0b`,
`0o`, atau `0x`; tanda minus diletakkan di depan awalan. Argumen harus
bilangan bulat — pecahan menimbulkan `GalatTipe`. `bulat` sampai 64 bit
dikonversi dengan tepat; hanya `desimal` di luar jangkauan int64 yang
menimbulkan `GalatNilai`.

### 1.2 String / Teks

| Fungsi | Deskripsi |
|--------|-----------|
| `panjang(x)` | Panjang teks (karakter), list, kamus, atau rentang. Mengembalikan `bulat` |
| `panjang_byte(teks)` | Panjang teks dalam **byte**, bukan karakter. Mengembalikan `bulat` |
| `rentang([awal,] akhir[, langkah])` | Deret bilangan bulat malas: `rentang(5)` → 0, 1, 2, 3, 4. Dipakai di `untuk`, bisa `panjang()`, `r[i]`, `x dalam r` |
| `huruf_besar(s)` | Ubah ke huruf kapital |
| `huruf_kecil(s)` | Ubah ke huruf kecil |
| `ganti(teks, cari, ganti)` | Ganti semua kemunculan substring |
| `karakter(teks, i)` | Karakter (string 1 huruf) pada indeks `i` |
| `potong(teks, mulai, jumlah)` | Substring mulai dari `mulai` sepanjang `jumlah` |
| `ord(karakter)` | Kode Unicode dari karakter sebagai `bulat` (`ord("é") == 233`) |
| `chr(kode)` | Karakter dari kode Unicode 0–1114111 (`chr(233) == "é"`) |

Panjang, indeks, `karakter`, dan `potong` menghitung **karakter**, bukan byte,
sehingga teks beraksen dan emoji aman: `panjang("héllo") == 5`,
`"café"[3] == "é"`.

Bila yang dibutuhkan adalah jumlah **byte** — misalnya mengisi `Content-Length`
sebuah respons HTTP atau menghitung data yang akan dikirim lewat soket — pakai
`panjang_byte()`:

```indonesiaku
cetak(panjang("héllo"))         # 5 karakter
cetak(panjang_byte("héllo"))    # 6 byte: "é" memakan 2 byte
```

Pembedaan ini penting untuk data biner. Teks di Indonesiaku menyimpan byte apa
adanya, jadi `baca_semua()` sebuah berkas PNG menghasilkan teks yang utuh,
tetapi jumlah karakternya lebih kecil daripada ukuran berkasnya. Menghitung
ulang lebar UTF-8 per karakter **tidak** menghasilkan ukuran yang benar untuk
isi seperti itu — `panjang_byte()` mengembalikan ukuran byte yang sebenarnya.

Pengindeksan string juga tersedia: `teks[i]` mengembalikan karakter ke-`i`
(string bersifat immutable — tidak dapat diubah lewat indeks).

### 1.3 List

| Fungsi | Deskripsi |
|--------|-----------|
| `tambah(list, nilai)` | Tambah elemen ke akhir list |
| `hapus(list)` | Hapus & kembalikan elemen terakhir |
| `hapus_pada(list, i)` | Hapus & kembalikan elemen pada indeks `i` (geser sisanya) |
| `sisip(list, i, nilai)` | Sisipkan `nilai` pada indeks `i` |
| `panjang(list)` | Jumlah elemen |
| `jumlah(d, awal=0)` | Total semua elemen angka, dimulai dari `awal`. Semua `bulat` → hasil `bulat`; satu `desimal` → hasil `desimal` |
| `semua(d)` | `benar` jika **semua** elemen bernilai benar |
| `ada_yang(d)` | `benar` jika **ada minimal satu** elemen bernilai benar |
| `terurut(d, menurun=salah)` | **Daftar baru** yang terurut; argumen aslinya tidak berubah. Objek dengan `_kurang_dari_` ikut diterima |
| `terbalik(d)` | **Daftar baru** dengan urutan dibalik |
| `pasangkan(a, b, ...)` | Gabungkan beberapa urutan jadi daftar pasangan (zip) |
| `peta(f, d)` | Daftar hasil penerapan fungsi `f` ke tiap elemen (map) |
| `saring(f, d)` | Daftar elemen yang membuat `f` bernilai benar (filter) |

Literal: `[1, 2, 3]`. Indeks baca/tulis: `list[i]`, `list[i] = x`.

> Kedelapan fungsi baru di atas (`jumlah`, `semua`, `ada_yang`, `terurut`,
> `terbalik`, `pasangkan`, `peta`, `saring`) tidak hanya menerima daftar —
> **rentang** dan **teks** juga boleh, karena ketiganya sama-sama urutan.
> Semuanya mengembalikan **daftar baru** dan tidak pernah mengubah argumennya.

#### `jumlah` — menjumlahkan isi urutan

```indonesiaku
cetak(jumlah([1, 2, 3, 4]))     # 10
cetak(jumlah([]))               # 0
cetak(jumlah([1, 2], 10))       # 13
cetak(jumlah(rentang(1, 101)))  # 5050
coba {
    jumlah([1, "dua"])
} kecuali e {
    cetak(e.pesan)              # jumlah() hanya menerima angka
}
```

Urutan kosong menghasilkan `0` (bukan galat). Argumen kedua `awal` adalah nilai
awal penjumlahan, berguna untuk menambahkan ke total yang sudah ada. Semua
elemen harus angka — elemen lain menimbulkan `GalatTipe`.

#### `semua` dan `ada_yang` — memeriksa syarat sekaligus

```indonesiaku
cetak(semua([benar, benar]))    # benar
cetak(semua([1, 0, 3]))         # salah
cetak(semua([]))                # benar
cetak(ada_yang([salah, benar])) # benar
cetak(ada_yang([]))             # salah
```

Keduanya memakai aturan *truthiness* Indonesiaku (lihat `ke_logika`), jadi `0`
dan `""` dianggap salah. Perhatikan dua kasus kosong yang terasa aneh tetapi
memang benar secara logika (dan sama dengan Python): `semua([])` adalah
`benar` ("tidak ada elemen yang melanggar"), sedangkan `ada_yang([])` adalah
`salah` ("tidak ada satu pun elemen yang memenuhi").

#### `terurut` dan `terbalik` — selalu daftar baru

```indonesiaku
d = [3, 1, 2]
cetak(terurut(d))               # [1, 2, 3]
cetak(terurut(d, benar))        # [3, 2, 1]
cetak(d)                        # [3, 1, 2]  -- argumen asli tetap utuh
cetak(terurut(["pisang", "apel"]))  # ["apel", "pisang"]
cetak(terbalik([1, 2, 3]))      # [3, 2, 1]
cetak(terbalik("halo"))         # ["o", "l", "a", "h"]
coba {
    terurut([1, "dua"])
} kecuali e {
    cetak(e.pesan)              # terurut() tidak dapat membandingkan angka dengan teks
}
```

`terurut`, `min`, dan `max` juga menerima **objek** yang punya metode
`_kurang_dari_` — pengurutannya stabil:

```indonesiaku
kelas Siswa {
    init(nama, nilai) {
        diri.nama = nama
        diri.nilai = nilai
    }
    _kurang_dari_(lain) { kembalikan diri.nilai < lain.nilai }
    _teks_() { kembalikan f"{diri.nama}({diri.nilai})" }
}

siswa = [Siswa("Budi", 80), Siswa("Ani", 92)]
cetak(min(siswa))            # Budi(80)
cetak(max(siswa))            # Ani(92)
cetak(terurut(siswa)[0])     # Budi(80)
```

Argumen kedua `terurut` adalah `menurun`: isi `benar` untuk urutan dari besar
ke kecil. Kirim secara **posisional** (`terurut(d, benar)`) — fungsi bawaan
belum menerima argumen bernama, jadi `terurut(d, menurun=benar)` ditolak.
Isi daftar harus **seragam** — campuran angka dan teks tidak bisa
dibandingkan dan menimbulkan `GalatTipe`. Perhatikan bahwa `terbalik("halo")`
mengembalikan daftar karakter, bukan teks; gunakan `teks_balik` dari
`pustaka/teks.idk` bila Anda butuh teks.

#### `pasangkan` — menyandingkan beberapa urutan (zip)

```indonesiaku
cetak(pasangkan([1, 2, 3], ["a", "b"]))
# [[1, "a"], [2, "b"]]
cetak(pasangkan([1, 2], ["a", "b"], [benar, salah]))
# [[1, "a", benar], [2, "b", salah]]

untuk p dalam pasangkan(["Ani", "Budi"], [80, 95]) {
    cetak(p[0], "mendapat nilai", p[1])
}
# Ani mendapat nilai 80
# Budi mendapat nilai 95
```

Panjang hasilnya mengikuti argumen **terpendek** — kelebihan elemen dibuang
tanpa peringatan (lihat contoh pertama: `3` tidak muncul karena daftar kedua
hanya berisi dua elemen).

#### `peta` dan `saring` — mengolah tiap elemen

```indonesiaku
fungsi kuadrat(x) { kembalikan x * x }
fungsi ganjil(x) { kembalikan x % 2 == 1 }

cetak(peta(kuadrat, [1, 2, 3, 4]))   # [1, 4, 9, 16]
cetak(saring(ganjil, rentang(10)))   # [1, 3, 5, 7, 9]
cetak(jumlah(peta(kuadrat, saring(ganjil, rentang(10)))))  # 165
```

Perhatikan urutan argumen: **fungsi dulu, baru urutan** (kebalikan dari
`daftar_peta`/`daftar_saring` di `pustaka/daftar.idk`). Keduanya langsung
mengembalikan daftar jadi (*eager*, bukan malas seperti `rentang`), sehingga
hasilnya bisa langsung di-`panjang()`, diindeks, atau dirangkai seperti pada
baris terakhir.

### 1.3b Tupel

| Fungsi | Deskripsi |
|--------|-----------|
| `tupel()` | Tupel kosong `()` |
| `tupel(urutan)` | Tupel baru dari daftar, tupel, teks (per karakter), rentang, atau kamus (kuncinya) |

Metode yang tersedia pada tupel hanya yang **tidak mengubah**: `panjang`,
`terurut`, `terbalik`, `jumlah`, `semua`, `ada_yang`, `enumerasi`. Fungsi yang
mengubah daftar (`tambah`, `hapus`, `hapus_pada`, `sisip`) menolak tupel
dengan `GalatTipe`.

### 1.3c Himpunan

| Fungsi | Deskripsi |
|--------|-----------|
| `himpunan()` | Himpunan kosong |
| `himpunan(urutan)` | Himpunan baru dari daftar, tupel, himpunan, teks (per karakter), rentang, atau kamus (kuncinya); duplikat dibuang |
| `tambah(s, x)` / `s.tambah(x)` | Tambah anggota (tanpa galat bila sudah ada) |
| `s.hapus(x)` | Buang anggota; `GalatKunci` bila tidak ada |

Metode lain: `panjang`, `terurut`, `jumlah`, `semua`, `ada_yang`. Operator
`| & - ^` membuat himpunan baru. Anggota harus dapat di-hash (seperti kunci
kamus); himpunan biasa **tidak** dapat di-hash — gunakan `himpunan_beku`.

### 1.3d Himpunan Beku

| Fungsi | Deskripsi |
|--------|-----------|
| `himpunan_beku()` | Himpunan beku kosong |
| `himpunan_beku(urutan)` | Himpunan beku dari daftar, tupel, himpunan, teks, rentang, atau kamus (kuncinya) |

`himpunan_beku` adalah himpunan yang tidak dapat diubah, dan karena itu
**dapat di-hash**: ia boleh menjadi kunci kamus atau anggota himpunan lain.

```indonesiaku
variabel beku = himpunan_beku([3, 1, 2])
cetak(beku)                               # himpunan_beku([3, 1, 2])
cetak(jenis(beku))                        # himpunan_beku

variabel peta = {himpunan_beku([1, 2]): "pasangan"}
cetak(peta[himpunan_beku([2, 1])])        # pasangan
```

Isi yang sama berarti sama, beku atau tidak:

```indonesiaku
cetak(himpunan([1, 2]) == himpunan_beku([1, 2]))   # benar
```

Operator `| & - ^` bekerja untuk keduanya. Hasilnya beku hanya bila **kedua**
operand beku:

```indonesiaku
variabel beku = himpunan_beku([1, 2])
cetak(jenis(beku | himpunan_beku([3])))   # himpunan_beku
cetak(jenis(beku | himpunan([3])))        # himpunan
```

`tambah` dan `hapus` menolak himpunan beku dengan `GalatTipe`.

### 1.3e Iterator

| Fungsi | Deskripsi |
|--------|-----------|
| `iter(urutan)` | Iterator atas daftar, tupel, himpunan, teks, rentang, atau kamus (kuncinya) |
| `lanjutkan(it)` | Elemen berikutnya; `HentiIterasi` bila sudah habis |
| `lanjutkan(it, bawaan)` | Elemen berikutnya, atau `bawaan` bila sudah habis |

Dinamai `lanjutkan` karena `lanjut` sudah menjadi kata kunci perulangan.

```indonesiaku
variabel it = iter([10, 20])
cetak(lanjutkan(it))          # 10
cetak(lanjutkan(it))          # 20
cetak(lanjutkan(it, "habis")) # habis
```

Tanpa nilai bawaan, iterator yang habis menaikkan `HentiIterasi`:

```indonesiaku
coba {
    lanjutkan(iter([]))
} kecuali HentiIterasi sebagai e {
    cetak("tertangkap:", e.pesan)   # tertangkap: Iterator sudah habis.
}
```

Untuk perulangan biasa, `untuk ... dalam` lebih ringkas; `iter` berguna saat
posisi iterasi perlu disimpan atau dilanjutkan belakangan.

### 1.4 Kamus (Dictionary)

| Fungsi | Deskripsi |
|--------|-----------|
| `kamus()` | Buat kamus kosong |
| `punya(kamus, kunci)` | Apakah kunci ada → boolean. Kunci boleh nilai apa pun yang hashable |
| `ambil(kamus, kunci[, bawaan])` | Nilai kunci, atau `bawaan` bila kunci tidak ada. Tanpa `bawaan` → `kosong` |
| `kunci(kamus)` | List semua kunci, urutan penyisipan |
| `hapus_kunci(kamus, kunci)` | Hapus pasangan kunci-nilai |
| `enumerasi(urutan, awal=0)` | List pasangan `[indeks, nilai]` untuk list, teks, rentang, atau kamus |

Akses: `k[kunci]`, `k[kunci] = nilai`. Kunci boleh teks, angka (`bulat`/
`desimal`), `benar`/`salah`, atau `kosong`; `1` dan `1.0` adalah kunci yang
sama. Daftar/kamus/rentang tidak bisa menjadi kunci (`GalatTipe`).

#### `ambil` — membaca dengan nilai bawaan

`k[kunci]` memberi `kosong` untuk kunci yang tidak ada, sehingga "tidak ada"
dan "ada tapi bernilai `kosong`" tidak bisa dibedakan. `ambil()` memberi nilai
bawaan yang jelas:

```indonesiaku
variabel d = {"a": 1, "nol": 0, "kosongan": kosong}

cetak(ambil(d, "a"))             # 1
cetak(ambil(d, "zz"))            # kosong
cetak(ambil(d, "zz", 99))        # 99
cetak(ambil(d, "nol", 99))       # 0  -- nilai falsy tetap dipakai
cetak(ambil(d, "kosongan", 99))  # kosong  -- kuncinya ADA

cetak(d.ambil("zz", "bawaan"))   # bawaan  -- juga sebagai metode
```

Yang menentukan adalah **keberadaan kunci**, bukan kebenaran nilainya: kunci
yang ada tetapi bernilai `kosong` mengembalikan `kosong`, bukan `bawaan`.
Gunakan `punya()` bila yang dibutuhkan hanya pemeriksaan keberadaan. `ambil()`
tidak pernah mengubah kamus.

### 1.5 Konversi & Format

| Fungsi | Deskripsi |
|--------|-----------|
| `ke_angka(teks)` | String → angka (gagal dapat ditangkap) |
| `ke_teks(nilai)` | Nilai apa pun → string; objek memakai `_teks_` bila ada (lihat *Metode ajaib*) |
| `ke_bulat(x)` | Angka/teks/boolean → `bulat`, **memotong ke arah nol** |
| `ke_desimal(x)` | Angka/teks/logika → `desimal`, selalu berkoma |
| `ke_logika(x)` | Nilai apa pun → `benar`/`salah` menurut aturan *truthiness* |
| `daftar([urutan])` | Daftar baru dari daftar, tupel, himpunan, teks, rentang, atau kamus (kuncinya) |
| `hash(x)` | Nilai hash (bulat); `GalatTipe` bila tidak dapat di-hash |
| `turunan_dari(kelas, induk)` | `benar` bila `kelas` turunan `induk` (kelas adalah turunan dirinya sendiri) |
| `repr(x)` | Representasi untuk **debug**: teks ditampilkan **dengan tanda kutip**; objek memakai `_repr_` bila ada |
| `format(pola, ...)` | Ganti setiap `{}` di pola dengan argumen |
| `jenis(x)` | Nama tipe: `"angka"`, `"teks"`, `"logika"`, `"kosong"`, `"daftar"`, `"tupel"`, `"himpunan"`, `"himpunan_beku"`, `"iterator"`, `"kamus"`, `"objek"`, `"kelas"`, `"berkas"`, `"modul"`, `"fungsi"`, `"rentang"` |

#### `ke_desimal` — memaksa desimal

Sejak `bulat` dan `desimal` dipisah (K2), `ke_angka("3")` memberi **bulat**.
Pakai `ke_desimal` bila hasilnya harus desimal:

```indonesiaku
cetak(ke_angka("3"))      # 3
cetak(ke_desimal("3"))    # 3.0
cetak(ke_desimal(3))      # 3.0
cetak(ke_desimal(benar))  # 1.0
```

#### `daftar` — selalu salinan baru

```indonesiaku
cetak(daftar("abc"))              # ["a", "b", "c"]
cetak(daftar(rentang(3)))         # [0, 1, 2]
cetak(daftar({"a": 1, "b": 2}))   # ["a", "b"]

variabel asal = [1, 2]
variabel salinan = daftar(asal)
salinan.tambah(3)
cetak(asal, salinan)              # [1, 2] [1, 2, 3]
```

#### `ke_bulat` — memotong, bukan membulatkan

```indonesiaku
cetak(ke_bulat(3.9))     # 3
cetak(ke_bulat(-3.9))    # -3  <- memotong ke arah nol
cetak(lantai(-3.9))      # -4  <- bandingkan: floor ke bawah
cetak(ke_bulat("12"))    # 12
cetak(ke_bulat(benar))   # 1
coba {
    ke_bulat("dua belas")
} kecuali e {
    cetak(e.pesan)       # ke_bulat() gagal: 'dua belas' bukan bilangan bulat
}
```

Hati-hati pada bilangan negatif: `ke_bulat` membuang bagian desimal (menuju
nol), sedangkan `lantai` selalu turun. Untuk angka positif keduanya sama.
Teks yang tidak bisa dibaca sebagai bilangan bulat menimbulkan `GalatNilai`
yang bisa ditangkap `coba/kecuali`.

#### `ke_logika` — aturan benar/salah yang dipakai seluruh bahasa

```indonesiaku
cetak(ke_logika(0), ke_logika(""), ke_logika([]), ke_logika(kosong))
# salah salah salah salah
cetak(ke_logika(5), ke_logika("hai"), ke_logika([0]))
# benar benar benar
```

Yang dianggap **salah**: `salah`, `kosong`, angka `0`, teks kosong `""`, serta
daftar, kamus, dan rentang yang kosong. Semua nilai lain `benar` — termasuk
`[0]`, karena daftarnya sendiri tidak kosong. Aturan ini sama dengan yang
dipakai `jika`, `selama`, `semua`, dan `ada_yang`.

#### `repr` — teks dengan tanda kutip untuk debug

```indonesiaku
cetak(ke_teks("halo"))   # halo
cetak(repr("halo"))      # "halo"
cetak(repr(5))           # 5
cetak(repr(benar), repr(kosong))   # benar kosong
cetak(repr([1, "a"]))    # [1, "a"]
```

Pakai `ke_teks` untuk ditampilkan ke pengguna, dan `repr` saat men-*debug* —
tanda kutip membuat Anda bisa membedakan angka `5` dari teks `"5"`, dan
melihat spasi tersembunyi di ujung teks.


### 1.6 Berkas (File I/O)

Semua kesalahan dapat ditangkap `coba/kecuali`.

Di Windows, jalur yang lebih panjang dari 260 karakter didukung oleh fungsi
berkas dan direktori di bawah ini, juga saat menjalankan skrip dan `impor`.
Setelan registri `LongPathsEnabled` tidak diperlukan.

| Fungsi | Deskripsi |
|--------|-----------|
| `buka_berkas(path, mode)` | Buka berkas → handle. Mode: `"b"` baca, `"t"` tulis, `"s"` sambung |
| `tutup(berkas)` | Tutup handle |
| `baca(berkas)` | Baca seluruh sisa isi → string |
| `baca_baris(berkas)` | Baca satu baris → string, atau `kosong` di akhir |
| `tulis(berkas, teks)` | Tulis ke handle → jumlah byte |
| `baca_semua(path)` | Baca seluruh berkas dari path → string |
| `tulis_semua(path, teks)` | Tulis (timpa) ke path |
| `tambah_berkas(path, teks)` | Tambahkan ke akhir berkas |
| `ada_berkas(path)` | Apakah ada berkas (bukan direktori) di path → boolean. Berkas yang sedang dikunci program lain tetap `benar`. Untuk berkas **atau** direktori, pakai `jalur.ada(path)` |
| `hapus_berkas(path)` | Hapus berkas |

### 1.7 Waktu & Tanggal

| Fungsi | Deskripsi |
|--------|-----------|
| `waktu()` | Detik sejak epoch (Unix time) |
| `tanggal()` | Kamus `{tahun, bulan, hari, jam, menit, detik, hari_minggu, hari_tahun}` |
| `format_tanggal(pola)` | Format waktu sekarang dengan pola `strftime` |
| `jam()` | Waktu CPU yang dipakai proses (detik), bukan waktu dinding: tidak bertambah selama `henti()`. Untuk mengukur waktu dinding pakai `waktu()` |
| `henti(detik)` | Jeda eksekusi |

### 1.8 Lain-lain

| Fungsi / Variabel | Deskripsi |
|-------------------|-----------|
| `cetak(...)` | Cetak nilai ke output |
| `input(prompt)` | Baca satu baris dari stdin |
| `adalah_instansi(objek, kelas)` | Apakah `objek` instansi `kelas` (termasuk lewat pewarisan) → boolean |
| `bisa_dipanggil(x)` | Apakah `x` bisa dipanggil seperti fungsi → boolean |
| `id(x)` | Identitas objek berupa angka, konsisten dengan operator `adalah` |
| `argumen` | Variabel global: list argumen baris perintah |

#### `adalah_instansi` dan `bisa_dipanggil` — memeriksa tipe dengan aman

```indonesiaku
kelas Hewan {
    init(nama) { diri.nama = nama }
}
kelas Anjing < Hewan {
    init(nama) { super.init(nama) }
}
a = Anjing("Rex")
cetak(adalah_instansi(a, Anjing))   # benar
cetak(adalah_instansi(a, Hewan))    # benar  <- lewat pewarisan
cetak(adalah_instansi(5, Hewan))    # salah

fungsi sapa(n) { kembalikan "Halo " + n }
cetak(bisa_dipanggil(sapa))         # benar
cetak(bisa_dipanggil(Anjing))       # benar  <- kelas dipanggil untuk membuat objek
cetak(bisa_dipanggil(5))            # salah
```

`adalah_instansi` ikut memperhitungkan **pewarisan**, jadi lebih tepat daripada
membandingkan `jenis(x)` yang hanya mengembalikan `"objek"` untuk semua
instansi. `bisa_dipanggil` berguna sebelum memanggil nilai yang Anda terima
sebagai parameter, misalnya di `peta`/`saring`.

#### `id` — identitas objek

```indonesiaku
a = [1, 2]
b = a
c = [1, 2]
cetak(id(a) == id(b))   # benar  <- a dan b objek yang sama
cetak(id(a) == id(c))   # salah  <- isinya sama, objeknya beda
cetak(a adalah b)       # benar
cetak(a adalah c)       # salah
```

`id(x) == id(y)` selalu memberi jawaban yang sama dengan `x adalah y`. Pakai
`id` saat Anda ingin **mencatat atau mencetak** identitas (misalnya untuk
men-*debug* daftar yang tidak sengaja dibagi-pakai); pakai `adalah` saat cukup
membandingkan. Nilainya tidak punya arti di luar jalannya program — jangan
disimpan ke berkas.


### 1.9 Soket TCP

Server mendengarkan di `127.0.0.1` saja (localhost). Semua kesalahan dapat
ditangkap `coba/kecuali`. Untuk server **dan klien** HTTP, gunakan
`pustaka/http.idk`.

| Fungsi | Deskripsi |
|--------|-----------|
| `soket_dengar(port [, alamat])` | Buat soket server. `alamat` bawaan `"127.0.0.1"`; `"0.0.0.0"` untuk semua antarmuka (wajib di dalam container). Hanya dua nilai itu yang diterima. Satu port hanya untuk satu server: `soket_dengar` kedua pada port yang sedang dipakai gagal, di semua OS |
| `soket_terima(server)` | Blocking: terima koneksi → soket klien |
| `soket_baca(klien)` | Baca data yang diterima (satu recv) → string |
| `soket_tulis(klien, teks)` | Kirim teks → jumlah byte |
| `soket_tulis(klien, daftar_bita)` | Kirim byte mentah (daftar bulat 0..255) → jumlah byte. Dipakai protokol biner: `chr(255)` di dalam teks ter-enkode UTF-8 jadi **dua** byte di kawat |
| `soket_baca_bita(klien)` | Baca → daftar bulat 0..255. Daftar kosong = koneksi ditutup rapi; batas waktu menaikkan galat |
| `soket_tutup(soket)` | Tutup soket (server atau klien) |
| `soket_batas_waktu(soket, detik)` | Batas waktu baca **dan** tulis untuk satu soket. `0` berarti tanpa batas (bawaan). Detik boleh pecahan (`0.5`) |
| `soket_sambung(host, port)` | Klien: sambung ke `host:port` (nama host atau IP, IPv4/IPv6) → soket. Host tak dikenal atau koneksi ditolak → `GalatOS`. Ditolak (`GalatIzin`) di `indk --sandbox` |
| `tls_mulai(soket, host)` | Meningkatkan soket yang sudah tersambung menjadi TLS terverifikasi. Sesudahnya `soket_baca`/`soket_tulis`/`soket_tutup` tetap dipakai seperti biasa, tetapi terenkripsi. `host` dipakai untuk SNI sekaligus verifikasi nama. Sertifikat tidak sah → `GalatOS` yang menyebut sebabnya. **Verifikasi tidak bisa dimatikan** |

#### Batas waktu soket: satu klien diam bisa membekukan server

Tanpa batas waktu, `soket_baca()` menunggu selamanya. Satu klien yang menyambung
lalu **tidak mengirim apa pun** sudah cukup membuat server berhenti melayani
siapa pun — penyerang hanya perlu satu koneksi (serangan *slowloris*).

Setel batas waktu pada setiap soket klien yang diterima (potongan ini bagian
dari gelung server, jadi tidak bisa dijalankan sendiri):

```indonesiaku-takjalan
variabel klien = soket_terima(server)
soket_batas_waktu(klien, 5)

coba {
    variabel mentah = web.baca_permintaan(klien)
    soket_tulis(klien, app.tangani(mentah))
} kecuali e {
    # Klien lambat atau diam: lewati saja, server tetap melayani yang lain.
    cetak("koneksi dilewati:", e)
}
soket_tutup(klien)
```

Batas waktu yang terlampaui **menaikkan galat** yang bisa ditangkap
`coba/kecuali`, bukan mengembalikan string kosong. Keduanya sengaja dibedakan:
string kosong berarti klien menutup koneksi dengan sopan, sedangkan batas waktu
berarti permintaannya belum lengkap. Kalau disamakan, server akan menjawab
`400` untuk klien yang sebenarnya hanya lambat.

Bawaannya tetap tanpa batas supaya program lama tidak berubah perilaku, jadi
batas waktu harus disetel secara eksplisit.

### 1.10 Basis Data (SQLite)

SQLite tertanam langsung di dalam `indk` — tidak ada yang perlu dipasang.
Semua kesalahan dapat ditangkap `coba/kecuali`.

| Fungsi | Deskripsi |
|--------|-----------|
| `db_buka(jalur)` | Buka/buat basis data → handle. `":memory:"` = di memori saja |
| `db_eksekusi(db, sql, params)` | Jalankan satu pernyataan (DDL/DML) → `kosong` |
| `db_ambil_semua(db, sql, params)` | Jalankan kueri → daftar kamus (satu kamus per baris) |
| `db_ambil_satu(db, sql, params)` | Baris pertama saja → kamus, atau `kosong` bila tidak ada |
| `db_baris_terpengaruh(db)` | Jumlah baris yang diubah pernyataan terakhir |
| `db_id_terakhir(db)` | `id` baris hasil INSERT terakhir |
| `db_batas_tunggu(db, ms)` | Lama menunggu (milidetik) bila basis data sedang dikunci koneksi lain. Bawaan `0` = langsung gagal |
| `db_mulai(db)` | Mulai transaksi |
| `db_komit(db)` | Simpan transaksi |
| `db_batal(db)` | Batalkan transaksi |
| `db_tutup(db)` | Tutup handle; aman dipanggil dua kali |

**Data biner (BLOB).** Bahasa ini belum punya tipe byte tersendiri, jadi BLOB
memakai konvensi yang sama dengan modul `bita`: **daftar bulat 0..255**.

```indonesiaku-takjalan
impor bita

db_eksekusi(db, "INSERT INTO berkas VALUES (?)", [[137, 80, 78, 71]])

# Teks <-> biner lewat modul bita:
db_eksekusi(db, "INSERT INTO berkas VALUES (?)", [bita.ke_bita("halo")])
```

Daftar kosong `[]` menjadi BLOB berukuran nol — itu **berbeda** dari `kosong`
yang menjadi `NULL`. Elemen di luar `0..255` ditolak dengan `GalatNilai` yang
menyebut posisinya.


`params` boleh dikosongkan bila SQL tidak memakai `?`.

**Selalu pakai `?`, jangan pernah merangkai SQL dengan `+`.** Nilai yang
dikirim lewat `params` disimpan sebagai data, tidak pernah dieksekusi:

```indonesiaku
variabel db = db_buka(":memory:")
db_eksekusi(db, "CREATE TABLE siswa (id INTEGER PRIMARY KEY, nama TEXT, nilai REAL)")
db_eksekusi(db, "INSERT INTO siswa (nama, nilai) VALUES (?, ?)", ["Budi", 85.5])

untuk b dalam db_ambil_semua(db, "SELECT nama, nilai FROM siswa WHERE nilai > ?", [80]) {
    cetak(b["nama"], b["nilai"])        # Budi 85.5
}
db_tutup(db)
```

Pemetaan tipe: `NULL` ↔ `kosong`, `INTEGER`/`REAL` ↔ angka, `TEXT` ↔ teks.

Batasan yang disengaja — semuanya bergalat, bukan diam-diam salah:

- **Satu pernyataan per panggilan.** `"INSERT ...; DROP TABLE ..."` ditolak
  sebelum apa pun dijalankan.
- **Jumlah `?` harus sama** dengan panjang `params`.
- **Hanya `?` anonim**, bukan `:nama` atau `?1`.
- **`BLOB` belum didukung** (menunggu tipe `bita`), dan `INTEGER` di luar
  jangkauan tepat angka desimal (±2⁵³) ditolak daripada dibulatkan diam-diam.
- Handle yang sudah ditutup → galat, bukan crash.

Contoh lengkap, termasuk transaksi dan uji anti-injeksi:

{{embed:examples/db_sqlite.idk}}

---

### 1.11 Sistem Operasi

| Fungsi | Deskripsi |
|--------|-----------|
| `keluar(kode=0)` | Akhiri program sekarang dengan kode keluar `kode` (`sys.exit`). `keluar("pesan")` mencetak pesan ke stderr lalu keluar dengan kode 1 |
| `tulis_galat(teks)` | Tulis teks apa adanya ke **stderr** (tanpa baris baru otomatis) |
| `lingkungan()` | Kamus semua variabel lingkungan (`os.environ`) |
| `ambil_lingkungan(nama, bawaan=kosong)` | Nilai satu variabel lingkungan, atau `bawaan` bila tidak ada (`os.getenv`) |
| `direktori_kini()` | Direktori kerja saat ini (`os.getcwd`) |
| `isi_direktori(jalur=".")` | Daftar nama isi direktori, **selalu terurut** (`os.listdir`) |
| `buat_direktori(jalur)` / `hapus_direktori(jalur)` | Buat / hapus satu direktori (yang dihapus harus kosong) |
| `jalur_indk()` | Jalur lengkap interpreter `indk` yang sedang berjalan (`sys.executable`) |
| `atur_atribut(objek, nama, nilai)` | Isi atribut dengan nama berupa teks (`setattr`) |
| `ambil_atribut(objek, nama, bawaan)` | Baca atribut dengan nama berupa teks; `bawaan` bila tidak ada, selain itu `GalatAtribut` (`getattr`) |
| `punya_atribut(objek, nama)` | `benar` bila atribut itu ada (`hasattr`) |

```indonesiaku
cetak(ambil_lingkungan("VARIABEL_TIDAK_ADA", "bawaan"))   # bawaan

d = "_panduan_direktori_sementara"
buat_direktori(d)
tulis_semua(d + "/b.txt", "isi")
tulis_semua(d + "/a.txt", "isi")
cetak(isi_direktori(d))            # ["a.txt", "b.txt"]
hapus_berkas(d + "/a.txt")
hapus_berkas(d + "/b.txt")
hapus_direktori(d)

kelas Kotak {
}
k = Kotak()
atur_atribut(k, "lebar", 3)
cetak(k.lebar, punya_atribut(k, "tinggi"), ambil_atribut(k, "tinggi", 0))   # 3 salah 0
```

Kesalahan sistem operasi (direktori tidak ada, sudah ada, tidak kosong, ...)
adalah `GalatOS`, seperti `OSError` Python, dengan alasan yang sama di semua
sistem operasi:

```indonesiaku
coba {
    isi_direktori("direktori_yang_tidak_ada")
} kecuali GalatOS sebagai e {
    cetak(e.pesan)    # isi_direktori(): 'direktori_yang_tidak_ada' tidak ada
}
```

`keluar` dan `tulis_galat` dipakai untuk program baris perintah:

```indonesiaku-takjalan
jika panjang(argumen) == 0 {
    tulis_galat("pemakaian: program.idk BERKAS\n")
    keluar(2)
}
```

`keluar` mengakhiri program **saat itu juga**. Berbeda dengan `sys.exit`
Python, blok `akhirnya` dan `dengan` yang sedang berjalan tidak dijalankan
lagi.

**Mode `--sandbox`.** `indk --sandbox program.idk` menolak `lingkungan`,
`ambil_lingkungan`, `isi_direktori`, membuka koneksi atau port
(`soket_sambung`, `soket_dengar`), dan menjalankan program lain (modul
`proses`) dengan `GalatIzin`, turunan `GalatOS`. Playground web menjalankan
setiap program dengan mode ini.

Flag hanya dikenali **sebelum** path skrip, sehingga skrip tetap bisa
menerima argumennya sendiri yang kebetulan bernama `--sandbox`.

> **Batas yang disadari.** Mode ini **bukan** jail filesystem: membaca dan
> menulis berkas tetap diizinkan. Batas itu ditegakkan container, bukan flag
> ini — jangan jadikan `--sandbox` satu-satunya pengaman saat menjalankan
> kode tidak tepercaya. Dijaga `tests/sandbox.sh`.

### 1.12 Hash Kriptografis

| Fungsi | Deskripsi |
|--------|-----------|
| `md5(data)` | Hash MD5, 32 karakter heksa |
| `sha1(data)` | Hash SHA-1, 40 karakter heksa |
| `sha256(data)` | Hash SHA-256, 64 karakter heksa |
| `hmac_sha256(kunci, pesan)` | HMAC-SHA-256 → daftar 32 bita (bukan heksa: dipakai untuk XOR dan perbandingan byte) |
| `pbkdf2_sha256(sandi, garam, iterasi, panjang)` | Turunkan kunci → daftar bita. Iterasi dibatasi 1..10.000.000 karena nilainya bisa datang dari server |
| `sha512(data)` | Hash SHA-512, 128 karakter heksa |

Untuk enkripsi, hash kata sandi, HKDF, dan perbandingan waktu-tetap, pakai
modul [`kripto`](#225-pustakakriptoidk--kriptografi-modern) (bagian 2.25).

Padanan `hashlib` Python, tetapi langsung mengembalikan **teks heksa huruf
kecil** (seperti `.hexdigest()`). `data` berupa teks (di-hash sebagai UTF-8)
atau daftar byte `0..255`; keduanya memberi hasil yang sama untuk isi yang
sama.

```indonesiaku
cetak(sha256("abc"))
# ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
cetak(md5(""))                                   # d41d8cd98f00b204e9800998ecf8427e
cetak(sha256("Halo") == sha256([72, 97, 108, 111]))   # benar
```

Nama modulnya bukan `hash` karena nama itu sudah dipakai fungsi bawaan
`hash(x)` (nilai hash untuk kunci kamus, bukan hash kriptografis).
Untuk menyimpan kata sandi, jangan memakai MD5/SHA-1 polos: keduanya
dipertahankan untuk kecocokan dengan sistem lama dan pemeriksaan isi berkas.


## Database: PostgreSQL dan MySQL/MariaDB

Selain builtin `db_*` untuk SQLite, ada dua driver jaringan yang bicara
protokol server langsung — tanpa libpq maupun libmysqlclient, jadi `indk`
tetap hanya menaut libc dan libm.

```indonesiaku-takjalan
impor "pustaka/pg.idk"      # PostgreSQL
# atau
impor "pustaka/my.idk"      # MySQL / MariaDB

variabel k = sambung({"host": "127.0.0.1", "port": 5432,
                      "pengguna": "postgres", "sandi": ambil_lingkungan("PGSANDI"),
                      "basisdata": "sekolah"})

k.eksekusi("INSERT INTO siswa (nama, nilai) VALUES ($1, $2)", ["Budi", 80])
variabel b = k.ambil_semua("SELECT * FROM siswa", kosong)
variabel satu = k.ambil_satu("SELECT * FROM siswa WHERE id = $1", [1])
k.mulai() ; k.komit() ; k.batal()
k.tutup()
```

| Metode | Hasil |
|---|---|
| `sambung(konfigurasi)` | Koneksi. Kunci: `host`, `port`, `pengguna`, `sandi`, `basisdata` |
| `k.eksekusi(sql, parameter)` | Jumlah baris terpengaruh |
| `k.ambil_satu(sql, parameter)` | Kamus, atau `kosong` bila tak ada baris |
| `k.ambil_semua(sql, parameter)` | Daftar kamus |
| `k.mulai()` / `k.komit()` / `k.batal()` | Transaksi |
| `k.tutup()` | Idempoten; aman dipanggil dua kali |

**Penanda parameter berbeda dan sengaja tidak disembunyikan**: PostgreSQL
memakai `$1, $2`, MySQL memakai `?`. Membungkusnya akan membuat query
terlihat portabel padahal dialek SQL-nya tetap berbeda.

Nilai selalu dikirim sebagai parameter protokol, tidak pernah dirangkai ke
teks SQL, jadi payload injeksi tersimpan sebagai data biasa.

**Autentikasi yang didukung**: PostgreSQL `SCRAM-SHA-256`; MySQL/MariaDB
`mysql_native_password`. Metode lain — `password` polos, `md5`, dan
`caching_sha2_password` (bawaan MySQL 8) — **ditolak dengan pesan jelas**,
bukan didukung diam-diam.

**Belum didukung**: TLS ke database, timeout per-query. MySQL 8 belum pernah
diuji (butuh CPU `x86-64-v2`) dan bawaannya memakai metode autentikasi yang
ditolak di atas.


## Satu API untuk ketiga backend: `pustaka/db.idk`

Ketiga backend di atas dipakai dengan tiga cara berbeda: SQLite lewat fungsi
global `db_*`, sedangkan MySQL dan PostgreSQL lewat metode objek. `db.idk`
menyeragamkannya supaya kode yang berpindah backend tidak perlu ditulis ulang
hanya karena bentuk pemanggilannya berbeda.

```indonesiaku-takjalan
impor db

# satu-satunya baris yang berubah saat pindah backend
variabel k = db.sambung({"jenis": "sqlite", "berkas": "data.db"})
# variabel k = db.sambung({"jenis": "mysql",    "host": "127.0.0.1", "pengguna": "root",
#                          "sandi": ambil_lingkungan("MYSANDI"), "basisdata": "sekolah"})
# variabel k = db.sambung({"jenis": "postgres", "host": "127.0.0.1", "pengguna": "postgres",
#                          "sandi": ambil_lingkungan("PGSANDI"), "basisdata": "sekolah"})

cetak(k.jenis())                                  # "sqlite" | "mysql" | "postgres"
k.eksekusi("INSERT INTO siswa (nama) VALUES (?)", ["Budi"])
k.ambil_semua("SELECT * FROM siswa")              # parameter boleh diabaikan
k.ambil_satu("SELECT * FROM siswa WHERE id = ?", [1])
k.mulai() ; k.komit() ; k.batal()
k.tutup()
```

| Metode | Hasil |
|---|---|
| `db.sambung(konfigurasi)` | Koneksi. `jenis` wajib: `sqlite`, `mysql`, atau `postgres` |
| `k.jenis()` | Nama backend yang sedang dipakai |
| `k.eksekusi(sql, parameter)` | Jumlah baris terpengaruh — **juga di SQLite** |
| `k.ambil_satu(sql, parameter)` | Kamus, atau `kosong` bila tak ada baris |
| `k.ambil_semua(sql, parameter)` | Daftar kamus |
| `k.mulai()` / `k.komit()` / `k.batal()` | Transaksi |
| `k.tutup()` | Idempoten di ketiga backend |

`parameter` boleh diabaikan di ketiga backend. Kunci konfigurasi selain
`jenis` diteruskan apa adanya ke driver, jadi opsi khusus seperti `tls` pada
PostgreSQL tetap bisa dipakai.

**Batas waktu dan TLS.** Kedua driver jaringan menerima `batas_waktu` (detik,
bawaan `30`, `0` berarti tanpa batas) — 30 detik terlalu lama untuk aplikasi
yang ingin gagal cepat dan terlalu pendek untuk kueri analitik:

```indonesiaku-takjalan
variabel k = db.sambung({"jenis":"postgres", "host":"127.0.0.1", "pengguna":"postgres",
                         "sandi": ambil_lingkungan("PGSANDI"), "basisdata":"sekolah",
                         "tls": benar, "batas_waktu": 5})
```

`"tls": benar` pada PostgreSQL **tidak pernah turun diam-diam ke teks polos**:
bila server menolak `SSLRequest`, koneksi dibatalkan dengan galat. Driver
MySQL belum punya jalur TLS.

**SQL tetap ditulis sesuai dialek backend.** `db.idk` tidak menerjemahkan
penanda parameter: PostgreSQL memakai `$1`, SQLite dan MySQL memakai `?`.
Menerjemahkannya berarti memindai teks SQL, dan pemindai yang tidak memahami
literal string akan merusak query yang sah — pada
`SELECT * FROM t WHERE nama = 'siapa?' AND id = ?` tanda tanya pertama ada di
dalam literal, bukan parameter.

**Data biner (BLOB/bytea)** memakai konvensi yang sama dengan SQLite dan modul
`bita`: **daftar bulat 0..255** dikirim sebagai byte mentah, bukan sebagai teks
representasinya.

```indonesiaku-takjalan
k.eksekusi("INSERT INTO berkas (isi) VALUES ($1)", [[137, 80, 78, 71]])
```

Dua beda yang perlu diketahui dibanding SQLite:

- **Daftar kosong `[]` tidak dianggap biner** di MySQL/PostgreSQL. Dari daftar
  kosong tidak mungkin dibedakan "BLOB nol byte" dari "daftar kosong biasa",
  jadi ia melewati jalur nilai biasa. Untuk BLOB kosong, kirim teks kosong
  `""`. Di SQLite `[]` tetap menjadi BLOB nol byte seperti sebelumnya.
- **Jalur baca tidak otomatis**: `bytea` kembali dari PostgreSQL sebagai teks
  heksadesimal `\x...`, dan `blob` MySQL sebagai teks. Pakai
  `encode(isi,'hex')` / `HEX(isi)` di SQL bila ingin bentuk yang pasti.

**API lama tetap berjalan**: `db_*` untuk SQLite serta `my.sambung`/`pg.sambung`
langsung. `db.idk` adalah lapisan tipis di atasnya, bukan penggantinya.

## Migrasi skema: `pustaka/migrasi.idk`

Perubahan skema yang tercatat, jadi menjalankan program dua kali tidak
menjalankan ulang `CREATE TABLE` yang sama.

```python
impor db
impor migrasi

variabel k = db.sambung({"jenis": "sqlite", "berkas": "app.db"})

variabel m = daftar()
m.tambah(migrasi.langkah("001", "buat tabel siswa",
    fungsi(k) { k.eksekusi("CREATE TABLE siswa (id INTEGER PRIMARY KEY, nama TEXT)") },
    fungsi(k) { k.eksekusi("DROP TABLE siswa") }))

cetak(migrasi.naikkan_semua(k, m))   # 1 pada run pertama, 0 sesudahnya
```

| Fungsi | Hasil |
|---|---|
| `migrasi.langkah(versi, nama, naik, turun)` | Satu migrasi. `versi` harus teks; `turun` boleh `kosong` |
| `migrasi.naikkan_semua(k, daftar)` | Jumlah langkah yang baru dijalankan |
| `migrasi.turunkan_satu(k, daftar)` | Versi yang dibatalkan, atau `kosong` bila tidak ada yang terpasang |
| `migrasi.status(k, daftar)` | Daftar `{"versi", "nama", "terpasang"}`, urut menaik |

Versi terpasang dicatat di tabel `_migrasi_indk`, dibuat otomatis. Langkah
dijalankan **urut menaik menurut versi**, bukan urutan penulisan, dan berhenti
pada galat pertama. Versi ganda dan `turunkan_satu` pada langkah tanpa `turun`
ditolak dengan `GalatMigrasi` yang bisa ditangkap `coba/kecuali`.

> **Peringatan MySQL/MariaDB.** DDL di MySQL memicu *implicit commit*, jadi
> `batal()` tidak bisa mengembalikan `CREATE`/`ALTER` yang sudah terlanjur
> jalan. Rollback transaksional di sini hanya sepenuhnya aman pada SQLite dan
> PostgreSQL; pada MySQL sebuah langkah yang gagal di tengah dapat
> meninggalkan skema separuh jadi.

SQL di dalam `naik`/`turun` tetap ditulis sesuai dialek backend — pustaka ini
tidak merakit maupun menerjemahkan SQL.

Belum termasuk: pool koneksi, ORM, query builder, dan penyeragaman tipe data
antar backend.


## 2. Pustaka Standar (`pustaka/`)

Di-`impor` sesuai kebutuhan. Fungsi diberi awalan agar tidak bentrok dengan
namespace global.

### 2.1 `pustaka/teks.idk`

```python
impor "pustaka/teks.idk"
```

| Fungsi | Deskripsi |
|--------|-----------|
| `teks_cari(teks, cari)` | Indeks kemunculan pertama, atau `-1` |
| `teks_berisi(teks, cari)` | Apakah `cari` ada di `teks` |
| `teks_mulai_dengan(teks, awalan)` | Boolean |
| `teks_akhiri_dengan(teks, akhiran)` | Boolean |
| `teks_pisah(teks, pemisah)` | List bagian |
| `teks_gabung(daftar, perekat)` | Gabungkan list jadi string. **Builtin** (bukan fungsi `pustaka/teks.idk`), jadi tersedia tanpa impor dan berbiaya linear terhadap panjang hasil |
| `teks_ulang(teks, n)` | Ulang `teks` sebanyak `n` |
| `teks_balik(teks)` | Teks terbalik |
| `teks_pangkas(teks)` | Hilangkan spasi di awal/akhir |

### 2.2 `pustaka/daftar.idk`

```python
impor "pustaka/daftar.idk"
```

| Fungsi | Deskripsi |
|--------|-----------|
| `daftar_peta(daftar, fn)` | Terapkan `fn` ke tiap elemen (map) |
| `daftar_saring(daftar, fn)` | Elemen yang `fn`-nya benar (filter) |
| `daftar_lipat(daftar, awal, fn)` | Reduksi (reduce) |
| `daftar_berisi(daftar, nilai)` | Boolean |
| `daftar_indeks_dari(daftar, nilai)` | Indeks pertama, atau `-1` |
| `daftar_iris(daftar, mulai, akhir)` | Sublist `[mulai, akhir)` |
| `daftar_balik(daftar)` | Urutan terbalik |
| `daftar_salin(daftar)` | Salinan dangkal |
| `daftar_urutkan(daftar)` | Terurut menaik (angka atau string) |

### 2.3 `pustaka/matematika.idk`

```python
impor "pustaka/matematika.idk"
```

| Fungsi / Konstanta | Deskripsi |
|--------------------|-----------|
| `MAT_PI`, `MAT_E` | Konstanta π dan e |
| `mat_genap(n)`, `mat_ganjil(n)` | Boolean |
| `mat_faktorial(n)` | n! |
| `mat_fpb(a, b)` | Faktor persekutuan terbesar |
| `mat_kpk(a, b)` | Kelipatan persekutuan terkecil |
| `mat_jumlah(daftar)` | Jumlah elemen |
| `mat_rata_rata(daftar)` | Rata-rata |
| `mat_maks_list(daftar)`, `mat_min_list(daftar)` | Nilai maks/min |
| `mat_adalah_prima(n)` | Boolean |

### 2.4 `pustaka/kamus.idk`

```python
impor "pustaka/kamus.idk"
```

| Fungsi | Deskripsi |
|--------|-----------|
| `kamus_nilai(k)` | List semua nilai |
| `kamus_gabung(a, b)` | Kamus baru: `a` ditimpa `b` |
| `kamus_dari_pasangan(pasangan)` | Kamus dari list `[[kunci, nilai], ...]` |
| `kamus_salin(k)` | Salinan dangkal |
| `kamus_ambil(k, kunci, bawaan)` | Nilai jika ada, jika tidak `bawaan` |

### 2.5 `pustaka/json.idk`

```python
impor "pustaka/json.idk"
```

| Fungsi | Deskripsi |
|--------|-----------|
| `json_teks(nilai)` | Serialisasi kamus/list/angka/string/boolean/kosong → string JSON |
| `json_urai(teks)` | Parsing string JSON → nilai Indonesiaku (kesalahan dapat ditangkap) |

Keduanya mengikuti RFC 8259, seperti modul `json` di Python:

- `json_urai` mendekode `\uXXXX` (termasuk pasangan surrogate untuk emoji),
  `\b`, `\f`, dan escape baku lainnya. Galat dinaikkan untuk escape tidak
  sah, karakter kontrol mentah di dalam string, angka cacat (`01`, `1.`,
  `-`), kunci objek yang bukan string, dan data tambahan setelah nilai
  (`"1 2"`, `"[1]x"`).
- `json_teks` meng-escape karakter kontrol (`\u0001`), menolak `nan` dan
  `takhingga` (JSON tidak punya bentuk untuknya), dan mengubah kunci
  angka/logika/kosong menjadi teks (`{1: "a"}` → `{"1":"a"}`).

Contoh:
```python
db = kamus()
db["todos"] = [kamus()]
teks = json_teks(db)          # {"todos":[{}]}
kembali = json_urai(teks)     # kamus dengan list
```

Digunakan oleh aplikasi contoh `examples/todo_cli.idk` (CRUD Todo-list
berbasis berkas JSON) dan `examples/todo_demo.idk`.

### 2.6 `pustaka/http.idk`

```python
impor "pustaka/http.idk"
```

| Fungsi | Deskripsi |
|--------|-----------|
| `http_urai_request(teks)` | Parse request HTTP mentah → kamus `{metode, path, versi, header, badan}` |
| `http_response(kode, tipe_konten, badan)` | Bangun string response HTTP |
| `http_response_json(kode, badan)` | Pintasan response JSON |
| `http_ambil(url, metode="GET", kepala=kosong, badan="", maks_pengalihan=5)` | **Klien**: kirim permintaan → objek `Respons`. Pengalihan 301/302/303/307/308 diikuti sampai `maks_pengalihan` lompatan; beri `0` untuk menerima respons pengalihan apa adanya |
| `http_urai_url(url)` | `[host, port, jalur]` dari url `http://` |

### Modul `web` — server HTTP

`pustaka/web.idk` menggantikan rantai `jika metode == ... dan path == ...`
dengan routing. `app.tangani()` bekerja pada **teks** request dan
mengembalikan **teks** respons, jadi server dapat diuji tanpa jaringan dan
gelung soket yang sudah ada tetap dipakai.

| Fungsi | Keterangan |
|---|---|
| `web.Aplikasi()` | Buat aplikasi |
| `app.rute(metode, pola, penangan)` | Daftarkan rute; `:nama` menjadi parameter URL |
| `app.statis(prefiks, akar)` | Sajikan berkas di `akar` pada URL berawalan `prefiks` |
| `app.gunakan(mw)` | Middleware `mw(req, teruskan)` |
| `app.galat(penangan)` | Timpa respons 500 bawaan dengan `penangan(req, e)` |
| `app.tangani(mentah)` | Teks request → teks respons |
| `app.jalankan(port, batas_waktu, maks_permintaan, catat)` | Gelung server lengkap; mengembalikan jumlah koneksi yang dilayani |
| `app.berhenti()` | Hentikan gelung **setelah** balasan saat ini terkirim |
| `app.pantau(kesehatan, kesiapan, periksa_siap, metrik)` | Rute `/health`, `/ready`, `/metrics` (Prometheus) dan header `X-Request-ID`; lihat *Observability* di bawah |
| `app.metrik_teks()` | Teks metrics format Prometheus (setelah `pantau()`) |
| `web.teks(kode, isi)`, `web.json(kode, isi)`, `web.html(kode, isi)` | Pembantu respons |

#### Menjalankan server: `app.jalankan()`

`app.tangani()` hanya mengubah teks request menjadi teks respons. Untuk
benar-benar melayani jaringan, pakai `app.jalankan()` — jangan menulis gelung
soket sendiri:

```indonesiaku-takjalan
impor web

variabel app = web.Aplikasi()
app.rute("GET", "/halo", fungsi(req) { kembalikan web.teks(200, "halo") })

app.jalankan(8080)
```

Gelung itu sudah menyertakan hal-hal yang mudah terlupa kalau ditulis manual:

- **batas waktu per koneksi** (bawaan 10 detik) — tanpa ini satu klien yang
  menyambung lalu diam membekukan server selamanya;
- **galat satu koneksi tidak mematikan server** — permintaan rusak, klien yang
  pergi, atau penangan yang menaikkan galat hanya menutup koneksi itu;
- **soket selalu ditutup**, termasuk di jalur galat;
- **log per permintaan** lewat modul `log` (pencatat bernama `web`), memuat
  metode, path, status, dan durasi — tidak pernah badan atau header.

Parameternya:

| Parameter | Bawaan | Arti |
|---|---|---|
| `port` | wajib | Port TCP |
| `batas_waktu` | `10` | Detik per koneksi; `0` berarti tanpa batas |
| `maks_permintaan` | `0` | `0` melayani selamanya; angka positif berhenti setelah sekian koneksi (berguna untuk tes) |
| `catat` | `benar` | Tulis satu baris log per permintaan |

Berhenti dari dalam penangan:

```indonesiaku-takjalan
app.rute("POST", "/matikan", fungsi(req) {
    app.berhenti()
    kembalikan web.teks(200, "berhenti")
})
```

`berhenti()` membiarkan balasan yang sedang diproses terkirim lebih dulu, baru
menutup soket server.

Karena log memakai modul `log`, pengaturannya ikut pustaka itu:

```indonesiaku-takjalan
impor log
log.atur(berkas = "server.log")          # alihkan ke berkas
log.ambil_pencatat("web").atur_ambang(log.PERINGATAN)   # hanya 4xx/5xx
```

> **Batas yang jujur:** `jalankan()` melayani **satu permintaan pada satu
> waktu**. VM Indonesiaku satu utas dan belum punya thread, jadi belum ada
> worker paralel. Untuk beban publik, taruh reverse proxy (Nginx, Caddy) di
> depannya dan jangan perlakukan ini sebagai server produksi mandiri.

```indonesiaku-takjalan
impor web

variabel app = web.Aplikasi()

app.rute("GET", "/siswa/:id", fungsi(req) {
    kembalikan web.json(200, "{\"id\":\"" + req["params"]["id"] + "\"}")
})

app.gunakan(fungsi(req, teruskan) {
    jika tidak "authorization" dalam req["header"] {
        kembalikan web.teks(401, "perlu token")
    }
    kembalikan teruskan(req)
})
```

#### Berkas statis

`app.statis(prefiks, akar)` menyajikan CSS, JavaScript, gambar, dan font dari
disk. Berkas biner sampai utuh: `Content-Length` dihitung dengan
`panjang_byte()`, bukan dari jumlah karakter.

```indonesiaku-takjalan
impor web

variabel app = web.Aplikasi()
app.statis("/statis", "aset")     # GET /statis/gaya.css -> aset/gaya.css
```

Perilaku yang dijamin:

| Hal | Perilaku |
|---|---|
| Metode | Hanya `GET` dan `HEAD`; lainnya **405** dengan `Allow: GET, HEAD` |
| `HEAD` | Header identik dengan `GET`, tanpa badan |
| Berkas tak ada, direktori, atau tak terbaca | **404** dengan pesan yang sama |
| Daftar isi direktori | Tidak pernah ditampilkan |
| `Content-Type` | Dari ekstensi; tak dikenal → `application/octet-stream` |
| Cache | `ETag` lemah; `If-None-Match` yang cocok → **304** |
| Keamanan | `X-Content-Type-Options: nosniff` selalu dikirim |

**Keamanan jalur.** Segmen `..` ditolak **setelah** dekode persen, jadi
`%2e%2e%2f` tertutup serapat `../`; backslash diperlakukan sebagai pemisah
(penting di Windows); byte NUL ditolak. Permintaan yang memuat `..` ditolak
seluruhnya, bukan "dinaikkan satu level", sehingga tidak ada jalan keluar dari
`akar`.

Rute eksplisit diperiksa lebih dulu, jadi `app.rute()` selalu menang bila
polanya cocok. Berkas statis dicoba sebelum jawaban 404/405.

> Jangan menaruh berkas rahasia di dalam `akar`: semua yang ada di bawahnya
> dapat diunduh siapa pun yang tahu namanya.

Penangan menerima `req` berisi `metode`, `path`, `versi`, `header`,
`badan`, `params`, `query`, dan — setelah badan diurai — `form`, `berkas`,
dan `json`.

- Rute dicocokkan **berurutan**; yang pertama cocok dipakai.
- Path cocok tetapi metode tidak → **405** dengan header `Allow`, bukan 404.
- Query string diurai ke `req["query"]`; `%20` dan `+` didekode. Kunci
  berulang: yang terakhir menang.
- Galat dari penangan → **500**, dan detailnya **tidak bocor ke klien**;
  pesan aslinya ditulis ke `tulis_galat()` untuk operator.

**Nama header huruf kecil.** Sejak 1.4.0 `http_urai_request()`
menyeragamkan nama header menjadi huruf kecil, sesuai sifat
case-insensitive pada RFC 9110 §5.1 dan menyamai klien. Pakai
`req["header"]["host"]`, bukan `["Host"]`.

#### Membaca badan request

`Content-Type` menentukan parser yang dipakai. Ketiga kunci **selalu ada**,
jadi penangan tidak perlu memeriksa keberadaannya lebih dulu.

| Content-Type | Mengisi |
|---|---|
| `application/x-www-form-urlencoded` | `req["form"]` |
| `application/json` | `req["json"]` |
| `multipart/form-data` | `req["form"]` dan `req["berkas"]` |
| lainnya | tidak satu pun; `req["badan"]` tetap mentah |

```indonesiaku-takjalan
app.rute("POST", "/simpan", fungsi(req) {
    kembalikan web.teks(201, req["form"]["nama"])
})

app.rute("POST", "/unggah", fungsi(req) {
    variabel b = req["berkas"]["foto"]
    kembalikan web.teks(200, b["nama_berkas"] + " " + ke_teks(panjang(b["isi"])))
})
```

Tiap entri `req["berkas"]` adalah kamus berisi `nama_berkas` dan `isi`.

**Batas ukuran.** `web.Aplikasi(maks_badan)` membatasi badan request; bawaan
**1 MB**. Badan yang melebihi batas dijawab **413** berdasarkan
`Content-Length`, yaitu **sebelum** datanya dikumpulkan di memori.

**Gunakan `web.baca_permintaan(klien)`, bukan `soket_baca()` langsung.**
`soket_baca()` mengembalikan apa pun yang sudah tiba — maksimal sekitar 64 KB
per panggilan — sehingga memanggilnya sekali **memotong badan besar secara
diam-diam**. `web.baca_permintaan()` menggelung sampai `Content-Length`
terpenuhi:

```indonesiaku-takjalan
variabel mentah = web.baca_permintaan(klien)
soket_tulis(klien, app.tangani(mentah))
```

**Yang ditolak terang-terangan:** `Transfer-Encoding: chunked` dijawab
**411**, JSON rusak dan `Content-Length` tidak sah dijawab **400** (salah
klien, bukan 500). Berkas unggahan ditahan di memori, bukan ditulis ke disk.

#### Kuki dan sesi

`web.set_kuki(res, nama, nilai, opsi)` menambahkan `Set-Cookie` ke respons
yang sudah jadi dan mengembalikan respons baru:

```indonesiaku-takjalan
variabel res = web.teks(200, "selamat datang")
kembalikan web.set_kuki(res, "sesi", sid, {
    "httponly": benar,
    "secure": benar,
    "samesite": "Lax",
    "max-age": 3600
})
```

Opsi yang dikenal: `path` (bawaan `/`), `domain`, `max-age`, `samesite`,
`httponly`, `secure`. Memanggilnya dua kali menghasilkan **dua** baris
`Set-Cookie` — keduanya terkirim, tidak saling menimpa.

Nilai kuki di-encode persen pada karakter yang dilarang RFC 6265. Tanpa itu,
nilai yang memuat `;` akan **menyuntikkan atribut**: `"a; Domain=jahat.com"`
terbaca peramban sebagai domain kuki, bukan sebagai nilai.

`web.baca_kuki(req)` mengurai header `Cookie` menjadi kamus. Bila satu nama
muncul dua kali, yang **pertama** menang — sama seperti perilaku peramban.

```indonesiaku-takjalan
variabel kuki = web.baca_kuki(req)
jika "sesi" dalam kuki {
    variabel isi = toko.ambil(kuki["sesi"])
}
```

`web.TokoSesi(umur)` menyimpan sesi di memori proses (umur bawaan 3600 detik):

| Metode | Guna |
|---|---|
| `buat(isi)` | sesi baru, kembalikan ID 64 karakter heks |
| `ambil(sid)` | isi sesi, atau `kosong` bila tidak ada/kedaluwarsa |
| `hapus(sid)` | invalidasi — inilah logout |
| `putar(sid)` | ID baru, isi pindah, ID lama dihapus |
| `bersihkan()` | buang semua yang kedaluwarsa |

**Panggil `putar()` tepat setelah login berhasil.** Tanpa rotasi, ID sesi yang
ditanam penyerang ke peramban korban sebelum ia masuk akan **tetap berlaku
sesudahnya** — penyerang memegang ID yang sama dan ikut masuk. Itu *session
fixation*.

ID sesi diambil dari `acak_bita()` (CSPRNG sistem), **bukan** `acak()`.
`acak()` adalah `rand()` yang disemai waktu, jadi ID-nya dapat ditebak dari
header `Date` respons.

Toko ini berumur sama dengan proses: sesi hilang saat server dimulai ulang,
dan dua proses tidak berbagi sesi.

#### Proteksi CSRF

Sesi yang bekerja justru membuka lubang ini. Peramban melampirkan kuki sesi
**otomatis pada setiap permintaan ke domain itu**, tak peduli dari halaman
mana permintaan berasal. Jadi halaman jahat cukup memuat:

```html
<form action="https://aplikasi-anda/hapus" method="POST">
<body onload="document.forms[0].submit()">
```

Korban yang sedang masuk membukanya, peramban mengirimkan kukinya, dan server
memproses permintaan itu sebagai tindakan sah korban — tidak terbedakan dari
klik tombol asli. Satu hal yang **tidak** dimiliki penyerang: isi sesi korban.
Di situlah token bekerja.

```indonesiaku-takjalan
variabel token = web.token_csrf(toko, sid)
# sisipkan ke formulir:
#   <input type=hidden name=_csrf value=...>

jika web.periksa_csrf(toko, req) {
    # aman diproses
}
```

`web.token_csrf(toko, sid)` membuat token sekali lalu menyimpannya di dalam
data sesi, jadi nilainya tetap selama sesi hidup — dua tab tidak saling
membatalkan. Karena ikut tersimpan di sesi, `putar()` memindahkannya dan
logout otomatis membatalkannya.

`web.periksa_csrf(toko, req)` mengembalikan `benar`/`salah`. `GET`, `HEAD`,
dan `OPTIONS` selalu lolos — menurut definisi tidak mengubah keadaan. Token
diambil dari `req["form"]["_csrf"]`, atau header `X-CSRF-Token` untuk jalur
AJAX.

`web.wajib_csrf(toko)` mengembalikan penjaga siap pakai: `kosong` bila boleh
lanjut, atau respons 403 yang tinggal dikembalikan.

```indonesiaku-takjalan
variabel penjaga = web.wajib_csrf(toko)

app.rute("POST", "/hapus", fungsi(req) {
    variabel tolak = penjaga(req)
    jika tolak != kosong {
        kembalikan tolak
    }
    kembalikan web.teks(200, "terhapus")
})
```

Perbandingan token memakai **waktu-tetap**, bukan `==`. `==` berhenti di
karakter pertama yang berbeda, sehingga lama perbandingan membocorkan berapa
banyak awalan yang sudah benar — dengan cukup percobaan, token dapat ditebak
karakter demi karakter.

**`SameSite=Lax` bukan pengganti token.** Ia menahan sebagian besar POST
lintas situs, tetapi nilainya bisa dilonggarkan jadi `SameSite=None` untuk
kebutuhan embed, tidak melindungi dari subdomain yang dikuasai penyerang, dan
perilakunya berbeda di peramban lama. Pakai keduanya.

#### Sandi, login, dan peran

**Jangan pernah menyimpan sandi apa adanya, dan jangan memakai `sha256()`
untuk itu.** SHA-256 dirancang *cepat*: GPU modern menghitung miliaran per
detik, jadi basis data yang bocor dapat dipecahkan dengan daftar kata. Tanpa
garam, satu tabel pelangi memecahkan seluruh kolom sekaligus dan dua pengguna
bersandi sama langsung terlihat karena hash-nya identik.

```indonesiaku-takjalan
variabel hash = web.hash_sandi("rahasia123")

jika web.periksa_sandi(sandi_dari_formulir, hash) {
    variabel sid = toko.buat({"pengguna": nama, "peran": "admin"})
}
```

`web.hash_sandi(sandi, iterasi)` mengembalikan satu teks berisi **seluruh**
parameter:

```
pbkdf2_sha256$600000$<garam heks>$<turunan heks>
```

Algoritme dan jumlah iterasi ikut tersimpan di dalam hash, bukan di
konfigurasi. Itu disengaja: ketika iterasi dinaikkan nanti, hash lama tetap
dapat diverifikasi dengan parameternya sendiri — tanpa itu, menaikkan iterasi
akan mengunci semua pengguna lama di luar. Garam 16 byte diambil dari
`acak_bita()` dan berbeda untuk setiap sandi.

Bawaannya **600.000 iterasi**, mengikuti rekomendasi OWASP untuk
PBKDF2-HMAC-SHA256. Terukur: 100k ≈ 0,14 s, 210k ≈ 0,32 s, 600k ≈ 0,79 s.

> **Biaya itu disengaja — itulah pertahanannya.** Tapi server ini melayani
> satu permintaan pada satu waktu, jadi **setiap login memblokir server
> ~0,8 detik**. Turunkan lewat argumen `iterasi` bila beban menuntut, sadar
> bahwa itu memperlemah pertahanan terhadap pemecahan offline.

`web.perlu_hash_ulang(hash)` → `benar` bila hash dibuat dengan iterasi lebih
rendah dari standar sekarang. Panggil **sesudah** login berhasil, selagi sandi
mentah masih di tangan, lalu simpan hash baru — biaya naik bertahap tanpa
reset massal.

`web.wajib_peran(toko, ["admin"])` mengembalikan penjaga seperti
`wajib_csrf`:

| Keadaan | Jawaban |
|---|---|
| belum masuk / sesi mati | **401** `perlu masuk` |
| sudah masuk, peran tidak termasuk | **403** `tidak berhak` |
| berhak | `kosong` — lanjutkan |

401 dan 403 sengaja dibedakan: 401 berarti "masuk dulu", 403 berarti
"percuma mencoba lagi".

`web.PembatasPercobaan(maks, jeda)` melambatkan tebakan beruntun:

```indonesiaku-takjalan
variabel batas = web.PembatasPercobaan(5, 300)

jika batas.terkunci(nama) {
    kembalikan web.teks(429, "terlalu banyak percobaan")
}
# ... verifikasi ...
batas.gagal(nama)      # saat salah
batas.berhasil(nama)   # saat benar, hitungan dikosongkan
```

**Periksa `terkunci()` sebelum memanggil `periksa_sandi()`**, bukan sesudah:
kalau tidak, penyerang tetap memaksa server menghitung PBKDF2 pada setiap
tebakan. Jeda dihitung sejak kegagalan terakhir, jadi yang terus mencoba tidak
pernah lolos dari kuncian. Hitungan ada di memori dan hilang saat server
dimulai ulang.

Dua hal yang mudah terlewat saat menulis halaman login:

- **Samakan pesan** untuk "nama tidak dikenal" dan "sandi salah". Pesan yang
  berbeda memberi tahu penyerang nama mana yang benar-benar terdaftar.
- **Panggil `toko.putar(sid)` sesudah login berhasil** untuk menutup *session
  fixation*.

Argon2id belum tersedia: ia menuntut pustaka C pihak ketiga, sedangkan proyek
ini hanya bergantung pada libc dan mbedTLS. PBKDF2 dengan 600k iterasi adalah
pilihan sadar, bukan kelalaian.

#### Pengalihan sesudah formulir

`web.alihkan("/")` menjawab dengan **303** dan header `Location`. Dipakai
sesudah POST diproses, supaya menyegarkan halaman tidak mengirim ulang
formulir yang sama (pola POST-redirect-GET).

303, bukan 302: 303 mewajibkan peramban memakai GET pada tujuan, sedangkan
302 secara historis diperlakukan berbeda-beda untuk POST.

#### CORS: siapa boleh membaca jawaban kita

Peramban memblokir skrip dari asal lain membaca respons kita, kecuali kita
mengizinkannya. `app.cors()` menyebutkan siapa:

```indonesiaku-takjalan
app.cors(["https://situs-saya.id"], benar)   # benar = izinkan kredensial
```

Asal yang tidak terdaftar tidak mendapat header CORS sama sekali, dan
peramban menolak memberikan isinya kepada pemanggil.

**`"*"` ditolak, bukan didukung.** Ia melumpuhkan pertahanan bawaan peramban,
dan menurut spesifikasi Fetch `*` bahkan tidak sah bersama kredensial — jadi
aplikasi berkuki sesi justru tidak terlayani olehnya. Respons memantulkan asal
pemohon, satu-satunya bentuk yang sah bila kuki ikut dikirim.

`Vary: Origin` selalu disertakan. Tanpa itu, cache perantara dapat menyimpan
respons yang mengizinkan satu asal lalu menyajikannya kepada asal lain —
kebocoran lintas-asal lewat cache, bukan lewat kode.

Permintaan `OPTIONS` dari asal terdaftar dijawab **204** beserta
`Allow-Methods`, `Allow-Headers`, dan `Max-Age`, tanpa memanggil rute.
Peramban mengirimnya sebelum setiap `PUT`/`DELETE` atau permintaan
ber-header kustom.

#### Alamat klien di belakang proxy

Aplikasi di belakang Nginx melihat semua koneksi datang dari `127.0.0.1`.
Alamat sebenarnya ada di `X-Forwarded-For` — tetapi header itu dikirim klien,
jadi siapa pun dapat menuliskan apa saja di sana.

```indonesiaku-takjalan
app.proxy_tepercaya(["127.0.0.1"])
# lalu di penangan:
req["alamat"]
```

Header itu dipercaya **hanya** bila koneksinya memang datang dari alamat yang
terdaftar. Tanpa `proxy_tepercaya()`, `X-Forwarded-For` diabaikan sepenuhnya
dan `req["alamat"]` selalu alamat soket — aman secara bawaan.

Yang diambil adalah entri **paling kanan** yang bukan proxy tepercaya. Entri
paling kiri ditulis klien dan dapat dipalsukan; yang kanan ditambahkan proxy
kita sendiri. Memakai yang kiri adalah kesalahan klasik: penyerang cukup
mengganti alamat palsunya setiap kali gagal, dan pembatas percobaan di atas
tidak pernah mengunci siapa pun.

Builtin `soket_alamat_peer(soket)` mengembalikan alamat ujung lain sebuah
koneksi, dan inilah yang membuat pemeriksaan di atas mungkin.

**Yang sengaja tidak ada:** pola wildcard asal (`https://*.situs.id`) dan
rentang CIDR untuk proxy. Pencocokan subdomain mudah salah tulis dan menjadi
lubang; daftar eksplisit cukup sampai ada kebutuhan nyata.

#### Observability: health, readiness, metrics, ID permintaan

`app.pantau()` menyalakan observability minimum untuk server yang jalan
24/7 di belakang reverse proxy atau orchestrator:

```indonesiaku
impor web

app = web.Aplikasi()
app.pantau(periksa_siap = fungsi() { kembalikan benar })   # mis. db "SELECT 1"
app.rute("GET", "/", fungsi(req) {
    kembalikan web.teks(200, "permintaan " + req["id"])
})

res = app.tangani("GET /health HTTP/1.1\r\nHost: x\r\n\r\n")
cetak(potong(res, 0, 15))                     # HTTP/1.1 200 OK
cetak("X-Request-ID" dalam res)               # benar
cetak("indk_http_permintaan_total" dalam app.metrik_teks())   # benar
```

| Rute (bawaan) | Jawaban |
|---|---|
| `GET /health` | Liveness: selalu `200` `{"status":"ok"}` selama gelung melayani. Sengaja tidak memeriksa apa pun yang mahal |
| `GET /ready` | Readiness: `200` `{"status":"ok"}` bila `periksa_siap()` bernilai benar (atau tidak diberikan), `503` `{"status":"tidak siap"}` bila salah **atau menaikkan galat** — detail galat hanya ke log pencatat `web`, tidak ke klien |
| `GET /metrics` | Format teks Prometheus 0.0.4 |

Jalur bisa diganti (`pantau(kesehatan = "/hidup")`) atau dimatikan dengan
`kosong`; jalur yang tidak diawali `/` dan pemanggilan `pantau()` kedua kali
ditolak dengan `GalatWeb`.

**Metrics** yang tersedia:

| Nama | Jenis | Isi |
|---|---|---|
| `indk_http_permintaan_total{metode,status}` | counter | Permintaan yang dijawab. Metode di luar GET/HEAD/POST/PUT/PATCH/DELETE/OPTIONS digabung jadi `LAIN` |
| `indk_http_durasi_detik` | histogram | Durasi `tangani()`, bucket bawaan Prometheus (5 ms – 10 s); p50/p95 lewat `histogram_quantile()` |
| `indk_mulai_detik` | gauge | Waktu `pantau()` dipanggil — tanda proses baru restart |

Path **tidak** dijadikan label: tiap `/todos/123` akan menjadi deret waktu
baru tanpa batas. Error rate dihitung dari label `status` (mis. `5..`).

**ID permintaan.** Setiap permintaan mendapat `req["id"]` (16 digit heks
dari `acak_bita`) — juga tanpa `pantau()`, jadi penangan selalu bisa
mencatatnya. Dengan `pantau()`, ID itu dikirim balik sebagai header
`X-Request-ID`, dan baris log `app.jalankan()` membawa field `request_id`.
`X-Request-ID` kiriman luar dipakai **hanya** dari `proxy_tepercaya()` dan
hanya bila 1–64 karakter `[A-Za-z0-9._-]`, sehingga nginx dapat meneruskan
`$request_id`-nya tetapi klien tidak dapat menyuntikkan baris log.

**Batas yang jujur:**

- Rute ini rute biasa, jadi middleware ikut berlaku — dan `/metrics` **tidak
  punya autentikasi sendiri**. Batasi lewat reverse proxy
  (`location /metrics { allow 10.0.0.0/8; deny all; }`) atau middleware.
- Satu utas: `/metrics` dan `/ready` dilayani bergiliran dengan permintaan
  pengguna, jadi `periksa_siap` harus cepat.
- Counter ada di memori proses: hilang saat restart (Prometheus menangani ini
  lewat `rate()`).
- Belum ada graceful shutdown lewat SIGTERM dan belum ada tracing
  terdistribusi.

#### Contoh utuh: `examples/catatan_app.idk`

Semua potongan di atas baru terasa gunanya saat disatukan. `catatan_app.idk`
adalah CRUD catatan dengan login dan dua peran, memakai basis data (Fase 10),
HTTP (Fase 11), templat (Fase 12), dan keamanan (Fase 13) sekaligus:

```
indk examples/catatan_app.idk 8090
# admin / rahasia123  (boleh menghapus catatan siapa pun)
# budi  / rahasia123  (hanya catatannya sendiri)
```

Empat hal yang mudah terlewat, dan karena itu ditulis eksplisit di sana:

1. **Tiap formulir membawa `_csrf`** — termasuk formulir keluar. Tombol
   "keluar" berupa `<form method=post>`, bukan tautan, karena GET tidak boleh
   mengubah keadaan.
2. **`toko.putar()` dipanggil sesudah sandi terverifikasi**, bukan sebelum.
3. **Kepemilikan diperiksa ulang di `/hapus`.** Menyembunyikan tombol hanya
   kenyamanan tampilan — siapa pun dapat mengirim POST langsung.
4. **Parameter SQL terikat (`?`)**, tidak pernah disambung ke dalam query.

`tests/keamanan_app.sh` menyerang aplikasi ini lewat HTTP sungguhan: session
fixation, CSRF lintas situs, penghapusan milik pengguna lain, XSS tersimpan,
SQL injection, enumerasi nama pengguna, dan kuncian brute force.


Digunakan oleh `examples/todo_server.idk` — REST API Todo-list berbasis HTTP
di atas soket TCP.

#### Klien: `http_ambil`

Padanan `urllib.request.urlopen`/`requests.get`, khusus `http://`:

```python
impor "pustaka/http.idk"

r = http_ambil("http://127.0.0.1:8080/todos")
cetak(r.status, r.ok(), r.kepala["content-type"])   # 200 benar application/json
cetak(r.badan)

r = http_ambil("http://127.0.0.1:8080/todos", "POST",
               {"Content-Type": "application/json"}, "{\"teks\":\"Beli kopi\"}")
r.periksa()   # GalatJaringan bila status 4xx/5xx
```

`Respons` punya `url`, `status` (bulat), `alasan` (`"Not Found"`), `kepala`
(kamus, **nama header huruf kecil**), dan `badan` (teks). Sama seperti
`requests`, status 4xx/5xx **bukan** galat — `ok()` memberi `salah`, dan
`periksa()` baru melempar `GalatJaringan`. Host yang tidak ditemukan atau
koneksi yang ditolak menjadi `GalatOS` dari `soket_sambung`.

```indonesiaku
impor "pustaka/http.idk"

cetak(http_urai_url("http://contoh.id:8080/a?b=1#c"))   # ["contoh.id", 8080, "/a?b=1", salah]
cetak(http_urai_url("https://contoh.id/"))              # ["contoh.id", 443, "/", benar]
```

**HTTPS didukung sejak 1.5.0.** Verifikasi sertifikat selalu aktif dan tidak
bisa dimatikan: sertifikat kedaluwarsa, nama host tidak cocok, atau penerbit
tak dikenal menaikkan `GalatOS` dengan pesan yang menyebut sebabnya. Daftar CA
diambil dari `INDK_CA_BUNDLE` bila disetel, selain itu dari jalur sistem yang
umum; bila tak satu pun ditemukan, koneksi **gagal** alih-alih diam-diam
melewati verifikasi. Di Windows, mbedTLS tidak membaca Certificate Store, dan jalur PEM gaya Unix
di atas umumnya tidak ada — setel `INDK_CA_BUNDLE` ke berkas bundel CA
(mis. `cacert.pem`) bila HTTPS gagal dengan pesan "tidak menemukan bundel CA
sistem". (Catatan: biner MinGW yang dijalankan lewat Wine tetap menemukan
jalur Unix tersebut, jadi pengujian Wine tidak membuktikan perilaku Windows
asli.)

Batasan lain: permintaan dikirim sebagai HTTP/1.0, sehingga server tidak mengirim `chunked` dan badan dibaca sampai
koneksi ditutup; tidak ada pengalihan (redirect) otomatis maupun **batas
waktu** (server yang diam membuat program menunggu); url dengan nama pengguna
(`user@host`) atau alamat IPv6 berkurung (`[::1]`) belum didukung. CR/LF di
url, metode, atau header ditolak dengan `GalatJaringan` supaya tidak bisa
menyisipkan header. Di playground (`--sandbox`) koneksi keluar ditolak.

**Pengalihan.** `http_ambil()` mengikuti `301`, `302`, `303`, `307`, dan `308`
sampai lima lompatan. `303` selalu dilanjutkan dengan `GET` tanpa badan;
`307`/`308` mempertahankan metode dan badan. `301`/`302` juga mempertahankan
metode — peramban mengubahnya menjadi `GET`, tetapi itu kekeliruan historis
yang justru melahirkan `307`/`308`.

`.url` pada hasil berisi URL **terakhir**, bukan yang diminta:

```indonesiaku-takjalan
variabel r = http_ambil("http://contoh.id/lama")
cetak(r.status)   # 200, bukan 301
cetak(r.url)      # alamat setelah pengalihan

# Jangan ikuti, lihat respons aslinya:
cetak(http_ambil("http://contoh.id/lama", "GET", kosong, "", 0).status)   # 301
```

Rantai yang berputar atau melewati batas lompatan menimbulkan
`GalatJaringan` yang menyebutkan rantai URL-nya. Pengalihan tanpa header
`Location` dikembalikan apa adanya — itu server yang melanggar spesifikasi,
dan Anda berhak melihatnya.

### 2.7 `pustaka/koleksi.idk` — wadah data

Padanan `collections` Python. Di-`impor` sebagai **modul** bernamespace, jadi
nama di dalamnya tidak perlu awalan:

```indonesiaku
impor koleksi
dari koleksi impor Penghitung, KamusBawaan, Antrian

c = Penghitung("abracadabra")
cetak(c["a"], c["z"])            # 5 0  -- kunci absen = 0
cetak(c.paling_umum(2))           # [["a", 5], ["b", 2]]
cetak(terurut(c.ke_daftar()))     # ["a", "b", "c", "d", "r"]

kb = KamusBawaan(daftar)          # nilainya daftar baru tiap kunci baru
tambah(kb["buah"], "apel")
cetak(kb["buah"], kb["sayur"])  # ["apel"] []

q = Antrian([1, 2, 3])
q.tambah_kiri(0)
cetak(q.ambil_kanan(), q.daftar())  # 3 [0, 1, 2]
```

| Nama | Deskripsi |
|------|-----------|
| `Penghitung(urutan=kosong)` | Pencacah ala `Counter`. `c[k]` = hitungan (0 bila absen), `paling_umum(n)`, `total()`, `kunci()`, `nilai()`, `tambah(v, n=1)`, `kurang(v, n=1)`, `tambah_banyak(urutan)`, `ke_daftar()`; operator `+` dan `-`; `==` mengabaikan urutan |
| `KamusBawaan(pabrik)` | Kamus dengan nilai bawaan ala `defaultdict`. `kb[k]` memanggil `pabrik()` saat kunci absen lalu menyimpannya; `ambil(k, bawaan=kosong)` **tidak** membuat nilai baru |
| `KamusBerurut` | Alias `kamus` biasa — kamus diIndonesiaku sudah berurut penyisipan |
| `Antrian(urutan=kosong, maks_panjang=kosong)` | Antrian dua ujung ala `deque`: `tambah_kiri`, `tambah_kanan`, `ambil_kiri`, `ambil_kanan`, `putar(n=1)`, `daftar()`, `panjang()`. `maks_panjang` membuang ujung yang berlawanan saat penuh |

`tupel_bernama` (namedtuple) belum ada: membuat konstruktor dengan jumlah
bidang dinamis butuh fungsi variadik (`*args`), yang belum didukung bahasa.

### 2.8 `pustaka/statistik.idk` — statistik dasar

```indonesiaku
impor statistik
cetak(statistik.rata_rata([1, 2, 3, 4]))   # 2.5
cetak(statistik.median([1, 2, 3, 4]))      # 2.5
cetak(statistik.modus("aabbbc"))           # b
cetak(statistik.kuantil([1, 2, 3, 4]))     # [1.25, 2.5, 3.75]
```

| Fungsi | Deskripsi |
|--------|-----------|
| `rata_rata(urutan)` | Rata-rata (`mean`) |
| `median(urutan)` | Nilai tengah; rata-rata dua tengah bila genap |
| `modus(urutan)` | Nilai paling sering; seri dimenangkan yang muncul lebih dulu |
| `varians(urutan)` / `simpangan_baku(urutan)` | **Sampel** (bagi `n-1`), seperti `statistics.variance` |
| `varians_populasi` / `simpangan_baku_populasi` | Populasi (bagi `n`) |
| `kuantil(urutan, n=4)` | `n-1` titik potong, metode *exclusive* seperti `statistics.quantiles` |

Urutan kosong (atau data kurang dari dua untuk sebaran) menaikkan
`GalatStatistik` yang membawa `.pesan` dan bisa ditangkap:

```indonesiaku
impor statistik
dari statistik impor GalatStatistik
coba {
    statistik.rata_rata([])
} kecuali GalatStatistik sebagai e {
    cetak(e.pesan)                 # rata_rata: urutan kosong
}
```

### 2.9 `pustaka/iterasi.idk` — alat iterasi

Padanan `itertools`. Karena bahasa belum punya generator (Fase 5), semuanya
**eager**: mengembalikan daftar, bukan iterator malas. Karena itu fungsi yang
di Python tak hingga (`hitung_naik`, `putar_ulang`, `ulangi`) mewajibkan
jumlah elemennya.

```indonesiaku
impor iterasi
cetak(iterasi.akumulasi([1, 2, 3, 4]))        # [1, 3, 6, 10]
cetak(iterasi.berpasangan([1, 2, 3]))         # [[1, 2], [2, 3]]
cetak(iterasi.rangkai([[1, 2], [3]]))         # [1, 2, 3]
cetak(iterasi.kombinasi([1, 2, 3], 2))        # [[1, 2], [1, 3], [2, 3]]
```

Isi: `hitung_naik`, `ulangi`, `putar_ulang`, `cabang`, `akumulasi`, `rangkai`,
`saring_dengan`, `ambil_selama`, `lewati_selama`, `saring_salah`,
`kelompokkan`, `iris`, `berpasangan`, `kelompok_per`, `peta_bintang`,
`pasangkan_terpanjang`, `produk`, `permutasi`, `kombinasi`,
`kombinasi_berulang`.

Dua penyesuaian karena bahasa belum punya `*args`: `rangkai` dan `produk`
menerima **satu daftar urutan** (`rangkai([[1], [2]])`, bukan
`rangkai([1], [2])`), dan `peta_bintang(f, urutan)` meneruskan tiap elemen
utuh ke `f` (Python membongkarnya jadi beberapa argumen).

### 2.10 `pustaka/fungsional.idk` — alat fungsional

```indonesiaku
impor fungsional
cetak(fungsional.lipat(fungsi(a, b) { kembalikan a + b }, [1, 2, 3, 4]))  # 10
orang = [["Budi", 80], ["Ani", 95]]
cetak(fungsional.urutkan_dengan(orang, fungsi(o) { kembalikan o[1] }))
# [["Budi", 80], ["Ani", 95]]
```

| Fungsi | Deskripsi |
|--------|-----------|
| `lipat(f, urutan, awal=kosong)` | Reduksi kiri (`functools.reduce`). Tanpa `awal`, elemen pertama jadi nilai awal; urutan kosong lalu menaikkan `GalatFungsional` |
| `parsial(f, arg_depan)` | `arg_depan` adalah **daftar**; mengembalikan fungsi satu argumen `g(x) = f(arg_depan..., x)` |
| `urutkan_dengan(urutan, kunci)` | Daftar baru terurut berdasar `kunci(nilai)`, stabil seperti `sorted(key=...)` |
| `urutkan_dengan_banding(urutan, banding)` | Terurut memakai pembanding tiga arah `banding(a, b)` (`< 0`, `0`, `> 0`) |
| `banding_ke_kunci(banding)` | Kelas pembungkus ala `cmp_to_key`; bungkus tiap nilai lalu serahkan ke `terurut` |

`@cache_lru` dan `urutan_lengkap` belum ada: masing-masing butuh fungsi
variadik dan penyuntikan metode ke kelas saat runtime.

### 2.11 `pustaka/salin.idk` — penyalinan

```indonesiaku
impor salin
asal = {"x": [1]}
salinan = salin.salin_dalam(asal)
tambah(salinan["x"], 2)
cetak(asal["x"], salinan["x"])     # [1] [1, 2]
```

| Fungsi | Deskripsi |
|--------|-----------|
| `salin(x)` | Salinan **dangkal** untuk daftar/kamus/himpunan; nilai lain dikembalikan apa adanya. Elemen di dalamnya masih dibagi |
| `salin_dalam(x)` | Salinan **rekursif** untuk daftar/kamus/himpunan/tupel; struktur bersiklus aman. Objek buatan sendiri dikembalikan apa adanya |

> Hati-hati: `dari salin impor salin` menimpa nama modul dengan fungsinya,
> persis seperti `from copy import copy` di Python. Pakai alias bila butuh
> keduanya: `dari salin impor salin sebagai kopi`.

### 2.12 Catatan: objek ber-`_iter_` dan fungsi urutan

Objek dengan metode `_iter_` sudah bisa dipakai di `untuk ... dalam` dan
`panjang()`, tetapi **belum** dikenali `daftar()`, `tupel()`, `terurut()`,
`jumlah()`, dan `semua()`. Untuk itu, kelas di modul ini menyediakan
`ke_daftar()`/`daftar()` sehingga Anda menulis:

```indonesiaku
dari koleksi impor Penghitung
c = Penghitung("abracadabra")
cetak(terurut(c.ke_daftar()))     # ["a", "b", "c", "d", "r"]
cetak(jumlah(c.nilai()))          # 11
```

---

Lihat contoh lengkap di folder `examples/` (mis. `pustaka_teks.idk`,
`pustaka_daftar.idk`, `pustaka_koleksi.idk`, `pustaka_statistik.idk`,
`pustaka_iterasi.idk`, `pustaka_fungsional.idk`, `pustaka_salin.idk`,
`berkas.idk`, `konversi.idk`, `json_pakai.idk`, `todo_demo.idk`,
`todo_server.idk`, `http_pakai.idk`).

---

### 2.13 `pustaka/jalur.idk` — jalur berkas

Padanan `os.path` Python untuk **mengolah jalur sebagai teks**: menggabung,
memecah, mengambil ekstensi, dan merapikan jalur. Hasilnya persis
`posixpath` Python. Hanya `/` yang memisahkan bagian jalur, di semua sistem
operasi, sehingga program yang sama memberi hasil yang sama di Windows,
Linux, dan macOS. Windows sendiri menerima `/`.

```indonesiaku
impor jalur
dari jalur impor gabung, pisah_ekstensi

cetak(gabung("data", "2026", "laporan.csv"))   # data/2026/laporan.csv
cetak(gabung("data", "/etc", "passwd"))         # /etc/passwd
cetak(jalur.nama_dasar("/rumah/budi/a.txt"))    # a.txt
cetak(jalur.nama_direktori("/rumah/budi/a.txt"))  # /rumah/budi
cetak(pisah_ekstensi("arsip.tar.gz"))           # ("arsip.tar", ".gz")
cetak(pisah_ekstensi(".bashrc"))                # (".bashrc", "")
cetak(jalur.normalisasi("a/./b//c/../d"))       # a/b/d
cetak(jalur.dari_windows("C:\\data\\a.txt"))    # C:/data/a.txt

# Ganti ekstensi setiap berkas:
untuk nama dalam ["foto/a.png", "foto/b.jpeg"] {
    akar, ekst = pisah_ekstensi(nama)
    cetak(akar + ".webp")                      # foto/a.webp, lalu foto/b.webp
}
```

| Nama | Deskripsi |
|------|-----------|
| `gabung(a, b, ...)` | `os.path.join`: sampai enam bagian; bagian yang diawali `/` membuang bagian sebelumnya |
| `gabung_semua(daftar)` | Seperti `gabung` untuk daftar bagian berapa pun panjangnya |
| `pisah(j)` | Tupel `(kepala, ekor)`: semua kecuali bagian terakhir, dan bagian terakhir |
| `nama_dasar(j)` / `nama_direktori(j)` | Bagian terakhir / semua kecuali bagian terakhir (`basename` / `dirname`) |
| `pisah_ekstensi(j)` / `ekstensi(j)` | Tupel `(akar, ekstensi)` / ekstensi saja termasuk titiknya; titik di awal nama bukan ekstensi |
| `normalisasi(j)` | Buang `.` dan `//`, selesaikan `..` (`os.path.normpath`) |
| `adalah_absolut(j)` | `benar` bila diawali `/` |
| `dari_windows(j)` | Ganti setiap `\` dengan `/` |
| `pemisah` | Konstanta `"/"` |
| `ada(j)` | `benar` bila ada berkas **atau** direktori di `j` (`os.path.exists`) |
| `adalah_berkas(j)` / `adalah_direktori(j)` | Jenis isi jalur (`isfile` / `isdir`) |
| `ukuran(j)` / `waktu_ubah(j)` | Ukuran dalam byte / waktu terakhir diubah dalam detik sejak 1970 (`getsize` / `getmtime`) |
| `absolut(j)` | Jalur absolut yang dinormalkan, selalu dengan `/` (`abspath`) |

`gabung` menerima paling banyak enam bagian karena bahasa ini belum punya
fungsi variadik; untuk lebih banyak, pakai `gabung_semua([...])`.
`ukuran`/`waktu_ubah` pada jalur yang tidak ada menghasilkan `GalatJalur`,
turunan `GalatOS`. Isi direktori dan direktori kini ada di fungsi bawaan
`isi_direktori` dan `direktori_kini` (bagian 1.11).

### 2.14 `pustaka/pola.idk` — ekspresi reguler

Padanan `re` Python: mencari, mengambil, mengganti, dan memecah teks
berdasarkan pola.

```indonesiaku
impor pola
dari pola impor GalatPola

m = pola.cari("(\\d{3})-(\\d+)", "telepon: 021-5550123")
cetak(m.grup(), m.grup(1), m.grup(2))      # 021-5550123 021 5550123
cetak(m.rentang(), m[1])                   # (9, 20) 021

cetak(pola.cari_semua("\\d+", "3 apel, 12 jeruk"))       # ["3", "12"]
cetak(pola.cari_semua("(\\w+)=(\\d+)", "a=1, b=22"))      # [("a", "1"), ("b", "22")]
cetak(pola.ganti("\\s+", " ", "terlalu    banyak   spasi"))  # terlalu banyak spasi
cetak(pola.ganti("(\\w+)@(\\w+)", "\\2 (\\1)", "budi@rumah"))  # rumah (budi)
cetak(pola.pisah("[,;]\\s*", "a, b;c;  d"))              # ["a", "b", "c", "d"]

# Grup bernama:
t = pola.cari("(?P<tahun>\\d{4})-(?P<bulan>\\d{2})", "2026-10")
cetak(t.grup("tahun"), t.kamus_grup())    # 2026 {"tahun": "2026", "bulan": "10"}

# Pengganti berupa fungsi:
fungsi kali_dua(m) {
    kembalikan ke_teks(ke_angka(m.grup()) * 2)
}
cetak(pola.ganti("\\d+", kali_dua, "3 apel dan 12 jeruk"))  # 6 apel dan 24 jeruk

# Pola dipakai berulang? Kompilasi sekali.
vokal = pola.kompilasi("[aiueo]", pola.ABAIKAN_KAPITAL)
cetak(vokal.cari_semua("IndONEsia"))     # ["I", "O", "E", "i", "a"]

coba {
    pola.kompilasi("(a")
} kecuali GalatPola sebagai e {
    cetak("pola salah:", e.pesan)
}
```

Di dalam teks Indonesiaku, `\` harus ditulis `\\`, jadi pola `\d+` ditulis
`"\\d+"`.

**Fungsi modul** (semuanya juga tersedia sebagai metode objek `Pola` hasil
`kompilasi`, tanpa argumen pola):

| Nama | Deskripsi |
|------|-----------|
| `kompilasi(pola, bendera=0)` | Objek `Pola` untuk dipakai berulang |
| `cari(pola, teks, bendera=0)` | `Cocokan` pertama di mana pun, atau `kosong` (`re.search`) |
| `cocok(pola, teks, bendera=0)` | Cocokan yang harus dimulai di awal teks (`re.match`) |
| `cocok_penuh(pola, teks, bendera=0)` | Cocokan yang harus meliputi seluruh teks (`re.fullmatch`) |
| `cari_semua(pola, teks, bendera=0)` | Daftar semua cocokan: teks bila tanpa grup, isi grup bila satu grup, tupel grup bila lebih (`re.findall`) |
| `iter_cari(pola, teks, bendera=0)` | Daftar objek `Cocokan` (`re.finditer`) |
| `ganti(pola, pengganti, teks, jumlah=0, bendera=0)` | Ganti cocokan; `pengganti` berupa teks (`\1`, `\g<nama>`, `\g<0>`) atau fungsi yang menerima `Cocokan` (`re.sub`) |
| `ganti_n(...)` | Seperti `ganti`, tetapi mengembalikan tupel `(teks_baru, banyak_penggantian)` |
| `pisah(pola, teks, maks=0, bendera=0)` | Pecah teks di setiap cocokan; isi grup ikut dimasukkan (`re.split`) |
| `lolos(teks)` | Beri `\` pada karakter khusus agar teks dicari apa adanya (`re.escape`) |

**Objek `Cocokan`:** `grup(g=0)` (nomor atau nama; `kosong` bila grup tidak
ikut cocok), `m[g]`, `grup_semua(bawaan=kosong)`, `kamus_grup()`,
`awal(g=0)`, `akhir(g=0)`, `rentang(g=0)`. Posisi dihitung **per karakter**,
bukan per byte, sama seperti indeks teks di bahasa ini.

**Bendera:** `pola.ABAIKAN_KAPITAL`, `pola.MULTIBARIS` (`^`/`$` di setiap
baris), `pola.TITIK_SEMUA` (`.` juga cocok dengan baris baru). Gabungkan
dengan `|`: `pola.ABAIKAN_KAPITAL | pola.MULTIBARIS`.

**Sintaks pola** sama dengan Python: `.`, `[abc]`, `[^0-9]`, `\d \w \s` dan
kebalikannya, `\b`, `^ $`, `* + ? {m,n}` beserta versi malasnya (`*?`),
grup `( )`, `(?: )`, `(?P<nama> )`, dan alternatif `|`.

Yang perlu diketahui:

- **Tidak ada pola yang membuat program macet.** Mesinnya bekerja dalam
  waktu linear terhadap panjang teks, jadi pola seperti `(a*)*b` yang
  membuat banyak mesin regex lain berjalan bertahun-tahun di sini selesai
  seketika.
- Karena itu, fitur yang membutuhkan penelusuran mundur **tidak didukung**
  dan ditolak dengan `GalatPola`: *backreference* (`\1` di dalam pola),
  *lookaround* (`(?=...)`, `(?!...)`), dan kuantifier posesif (`a++`).
  `\1` di dalam **teks pengganti** `ganti` tetap boleh.
- `\d`, `\w`, `\s`, `\b`, dan `ABAIKAN_KAPITAL` hanya mengenal huruf dan angka
  ASCII, seperti bendera `re.ASCII` di Python: `\w` tidak cocok dengan `é`.
- Tangkap galatnya dengan `dari pola impor GalatPola` lalu
  `kecuali GalatPola`; bentuk `kecuali pola.GalatPola` belum didukung.

### 2.15 `pustaka/opsi.idk` — argumen baris perintah

Padanan `argparse` Python. Daftarkan argumen yang diharapkan, lalu `urai()`
membaca daftar global `argumen` dan mengembalikan objek dengan satu atribut
per argumen.

```indonesiaku-takjalan
# hitung.idk
dari opsi impor PenguraiArgumen

p = PenguraiArgumen("hitung", "Menghitung baris berkas.")
p.tambah_argumen("berkas", bantuan = "berkas yang dihitung")
p.tambah_argumen("--jumlah", pendek = "-j", bawaan = 1, jenis_nilai = "bulat", bantuan = "berapa kali")
p.tambah_argumen("--verbose", pendek = "-v", aksi = "hitung")
a = p.urai()
cetak(a.berkas, a.jumlah, a.verbose)
```

```text
$ indk hitung.idk data.txt -j 3 -vv
data.txt 3 2
$ indk hitung.idk -h
pemakaian: hitung [-h] [-j JUMLAH] [-v] berkas

Menghitung baris berkas.

argumen posisi:
  berkas                berkas yang dihitung

opsi:
  -h, --bantuan         tampilkan bantuan ini lalu keluar
  -j, --jumlah JUMLAH   berapa kali
  -v, --verbose
$ indk hitung.idk data.txt --jumlah x
pemakaian: hitung [-h] [-j JUMLAH] [-v] berkas
hitung: galat: argumen --jumlah: nilai bulat tidak sah: "x"
```

Seperti `argparse`, `-h`/`--bantuan` mencetak bantuan lalu keluar dengan
kode 0, dan argumen yang salah mencetak pemakaian serta pesan galat ke
stderr lalu keluar dengan kode 2. Dengan `keluar_saat_galat = salah`
(`exit_on_error=False`), galat pemakaian menjadi `GalatOpsi` yang bisa
ditangkap — berguna untuk menguji, dan daftar argumen boleh diberikan
langsung ke `urai`:

```indonesiaku
dari opsi impor PenguraiArgumen, GalatOpsi

p = PenguraiArgumen("salin", keluar_saat_galat = salah)
p.tambah_argumen("sumber")
p.tambah_argumen("--paksa", pendek = "-p", aksi = "benar")
p.tambah_argumen("--mode", pilihan = ["cepat", "teliti"], bawaan = "cepat")
p.tambah_argumen("--tag", aksi = "tambahkan")

a = p.urai(["a.txt", "-p", "--tag", "x", "--tag", "y"])
cetak(a)          # Hasil(sumber="a.txt", paksa=benar, mode="cepat", tag=["x", "y"])
cetak(a["mode"], a.ke_kamus()["paksa"])   # cepat benar

coba {
    p.urai(["a.txt", "--mode", "lambat"])
} kecuali GalatOpsi sebagai e {
    cetak(e.pesan)  # argumen --mode: pilihan tidak sah: "lambat" (pilih dari "cepat", "teliti")
}
```

`tambah_argumen(nama, pendek=, bawaan=, jenis_nilai=, bantuan=, wajib=,
aksi=, pilihan=, metavar=)`:

| Parameter | Arti |
|---|---|
| `nama` | `"berkas"` = argumen posisi (wajib); `"--nama"` = opsi. `-` di nama menjadi `_` di atribut (`--jumlah-baris` → `a.jumlah_baris`) |
| `pendek` | Bentuk satu huruf, mis. `"-j"`. Bendera pendek boleh digabung: `-vq` sama dengan `-v -q` |
| `jenis_nilai` | `"teks"` (bawaan), `"bulat"`, `"desimal"`, atau fungsi pengubah |
| `aksi` | `"simpan"` (bawaan), `"benar"`/`"salah"` (bendera), `"hitung"` (`-vv` → 2), `"tambahkan"` (boleh berulang → daftar) |
| `pilihan` | Daftar nilai yang diizinkan |
| `wajib` | Jadikan opsi wajib diisi |

Didukung juga `--nama=nilai`, `-j3`, angka negatif sebagai nilai (`-j -5`),
dan `--` untuk mengakhiri opsi. Belum didukung: singkatan opsi panjang
(`--jum` untuk `--jumlah`), `nargs`, grup argumen, dan sub-perintah.

### 2.16 `pustaka/log.idk` — pencatatan

Padanan `logging` Python. Bawaannya menulis ke **stderr** dengan format
`LEVEL:nama:pesan`, dan hanya level `PERINGATAN` ke atas yang ditampilkan:

```indonesiaku-takjalan
impor log
log.info("tidak tampil")             # di bawah ambang PERINGATAN
log.peringatan("disk hampir penuh")  # stderr: PERINGATAN:akar:disk hampir penuh
```

`atur(ambang=, format=, berkas=)` (`basicConfig`) mengubah ambang, format,
dan tujuan. Format memakai `{level}`, `{nama}`, `{pesan}`, dan `{waktu}`.

```indonesiaku
impor log

log.atur(ambang = log.INFO, format = "[{level}] {nama}: {pesan}", berkas = "_panduan_log_sementara.txt")
log.info("aplikasi mulai")
db = log.ambil_pencatat("app.db")
db.galat("koneksi putus")
cetak(baca_semua("_panduan_log_sementara.txt"))
# [INFO] akar: aplikasi mulai
# [GALAT] app.db: koneksi putus
hapus_berkas("_panduan_log_sementara.txt")
```

| Nama | Deskripsi |
|------|-----------|
| `debug`, `info`, `peringatan`, `galat`, `kritis` | Catat pesan lewat pencatat akar |
| `DEBUG`, `INFO`, `PERINGATAN`, `GALAT`, `KRITIS` | Level: 10, 20, 30, 40, 50 |
| `ambil_pencatat(nama)` | `Pencatat` bernama; nama sama → objek sama. Bertingkat dengan titik: `"app.db"` mewarisi ambang `"app"` bila ambangnya sendiri belum diatur |
| `Pencatat.atur_ambang(level)` | Ambang khusus pencatat itu |
| `atur(ambang, format, berkas)` | Pengaturan bersama; `berkas = ""` kembali ke stderr; `format = "json"` untuk log terstruktur. Format bukan teks → `GalatLog` |
| `debug(pesan, data)` … `kritis(pesan, data)` | `data` (kamus, opsional) menjadi field tambahan di mode JSON; mode teks mengabaikannya. Bukan kamus → `GalatLog` |

**Log terstruktur (JSON).** `log.atur(format = "json")` menulis satu objek
JSON per baris — siap diurai Loki, ELK, atau `jq`. Kunci baku `waktu`
(waktu lokal `YYYY-MM-DDTHH:MM:SS`), `level`, `modul`, `pesan` selalu ada dan
tidak bisa ditimpa lewat `data`; nilai yang tidak punya bentuk JSON ditulis
sebagai teks alih-alih menggagalkan pencatatan.

```indonesiaku
impor log

log.atur(format = "json", berkas = "_panduan_log_json.txt")
log.peringatan("stok menipis", {"request_id": "9f2c01", "sisa": 3})
cetak(baca_semua("_panduan_log_json.txt"))
# {"waktu":"2026-11-21T10:00:00","level":"PERINGATAN","modul":"akar","pesan":"stok menipis","request_id":"9f2c01","sisa":3}
hapus_berkas("_panduan_log_json.txt")
```

Baris permintaan `app.jalankan()` membawa field `request_id`, `metode`,
`path`, `status`, dan `durasi_ms`. Header dan badan tetap tidak pernah
dicatat.

### 2.17 `pustaka/proses.idk` — menjalankan program lain

Padanan `subprocess.run` Python. Perintah **wajib berupa daftar teks** dan
dijalankan **tanpa shell**, jadi tidak ada injeksi lewat `;` atau `|` dan
tidak ada aturan kutip yang perlu diingat.

```indonesiaku
impor proses

# Program anak kecil: membaca satu baris, mencetaknya dalam huruf besar.
tulis_semua("_panduan_anak_sementara.idk", "cetak(input().huruf_besar())\n")

h = proses.jalankan([jalur_indk(), "_panduan_anak_sementara.idk"], masukan = "halo\n")
cetak(h.kode_keluar, repr(h.keluaran), repr(h.galat))   # 0 "HALO\n" ""
cetak(proses.keluaran([jalur_indk(), "_panduan_anak_sementara.idk"], masukan = "lagi\n"))
# LAGI
hapus_berkas("_panduan_anak_sementara.idk")
```

| Nama | Deskripsi |
|------|-----------|
| `jalankan(perintah, masukan="", periksa=salah)` | Jalankan dan tunggu sampai selesai. Mengembalikan `Hasil` dengan `kode_keluar`, `keluaran` (stdout), `galat` (stderr), dan `perintah`. `masukan` dikirim ke stdin. `periksa=benar` → `GalatProses` bila kode keluar bukan 0 |
| `keluaran(perintah, masukan="")` | Hanya stdout; `GalatProses` bila gagal (`check_output`) |
| `GalatProses` | Punya `.pesan` dan `.hasil` |

Baris baru di keluaran dinormalkan ke `\n` di semua sistem operasi.
Program yang tidak ditemukan menghasilkan `GalatOS`. Belum ada
`batas_waktu` (timeout). Ditolak dengan `GalatIzin` di `indk --sandbox`.

**Berkas `.bat`/`.cmd` di Windows.** Windows selalu menjalankan berkas
batch lewat `cmd.exe`, yang punya aturan kutipnya sendiri: `&` dan `|`
memisahkan perintah, dan `%VAR%` diperluas walau berada di dalam kutip.
Supaya janji "tanpa shell" tetap berlaku, `jalankan` selalu mengutip setiap
argumen untuk berkas batch, dan menolak argumen (juga jalur programnya) yang
memuat `"`, `%`, `!`, atau baris baru dengan `GalatNilai`. Karakter itu
tidak bisa diteruskan ke berkas batch dengan aman. Kalau perlu
melewatkannya, jalankan programnya langsung (`.exe`), bukan lewat berkas
batch. Program biasa (`.exe`) tidak terpengaruh.

### 2.18 `pustaka/bita.idk` — data biner

Bahasa ini belum punya tipe byte tersendiri, jadi **data biner adalah daftar
bulat `0..255`**. Modul `bita` mengubah teks ke byte UTF-8 dan sebaliknya,
serta dari dan ke heksa (padanan `str.encode`/`bytes.decode`/`bytes.hex`).

```indonesiaku
dari bita impor ke_bita, ke_teks, ke_heksa, dari_heksa, GalatBita

cetak(ke_bita("café"))            # [99, 97, 102, 195, 169]
cetak(ke_teks([72, 97, 108, 111]))  # Halo
cetak(ke_heksa(ke_bita("abc")))   # 616263
cetak(ke_teks(dari_heksa("c3a9")))  # é

coba {
    ke_teks([226, 130])
} kecuali GalatBita sebagai e {
    cetak(e.pesan)                # ke_teks: urutan UTF-8 terpotong
}
```

| Nama | Deskripsi |
|------|-----------|
| `ke_bita(teks)` | Daftar byte UTF-8 dari teks |
| `ke_teks(data)` | Teks dari daftar byte UTF-8; `GalatBita` bila urutannya tidak sah menurut RFC 3629, seperti `bytes.decode("utf-8")`: byte lanjutan menggantung atau salah, urutan terpotong, bentuk *overlong* (`C0 80`), surrogate `D800..DFFF`, atau kode di atas `U+10FFFF` |
| `ke_heksa(data)` | Teks heksa huruf kecil (`bytes.hex()`) |
| `dari_heksa(teks)` | Daftar byte dari teks heksa (panjang harus genap; `bytes.fromhex`) |

### 2.19 `pustaka/base64.idk` — Base64

Padanan modul `base64` Python. `enkode` menerima teks (dienkode UTF-8 dulu)
atau daftar byte; `dekode` mengembalikan daftar byte, dan `dekode_teks`
langsung teks. Padding `=` dipertahankan.

```indonesiaku
impor base64

cetak(base64.enkode("Halo"))            # SGFsbw==
cetak(base64.dekode_teks("SGFsbw=="))   # Halo
cetak(base64.dekode("SGFsbw=="))        # [72, 97, 108, 111]
cetak(base64.enkode_url([251, 255]))    # -_8=
```

| Nama | Deskripsi |
|------|-----------|
| `enkode(data)` / `dekode(teks)` / `dekode_teks(teks)` | Base64 standar (`b64encode`/`b64decode`) |
| `enkode_url(data)` / `dekode_url(teks)` / `dekode_url_teks(teks)` | Alfabet aman-URL dengan `-` dan `_` (`urlsafe_b64encode`) |
| `GalatBase64` | Panjang atau karakter base64 tidak sah |

### 2.20 `pustaka/csv.idk` — CSV

Mengurai dan membentuk CSV **sebagai teks** (bukan objek berkas), mengikuti
RFC 4180 dan perilaku modul `csv` Python: kolom yang memuat pemisah, kutip,
atau baris baru diapit kutip ganda, dan `""` berarti satu kutip.

```indonesiaku
impor csv

cetak(csv.urai("nama,catatan\nBudi,\"a,b\"\n"))
# [["nama", "catatan"], ["Budi", "a,b"]]

data = csv.urai_kamus("nama,umur\nBudi,20\nAni,21\n")
cetak(data)    # [{"nama": "Budi", "umur": "20"}, {"nama": "Ani", "umur": "21"}]
cetak(csv.bentuk_kamus(data, ["umur", "nama"]))
# umur,nama
# 20,Budi
# 21,Ani
```

| Nama | Deskripsi |
|------|-----------|
| `urai(teks, pemisah=",", kutip="\"")` | Daftar baris, tiap baris daftar kolom (teks). Baris kosong menjadi `[]`, seperti `csv.reader` |
| `bentuk(baris, pemisah=",", kutip="\"")` | Teks CSV; kutip hanya bila perlu. Nilai bukan teks diubah dengan `ke_teks` |
| `urai_kamus(teks, ...)` | Daftar kamus; baris pertama menjadi header (`csv.DictReader`) |
| `bentuk_kamus(daftar, kolom=kosong, ...)` | CSV dengan header; tanpa `kolom`, urutan kunci kamus pertama (`csv.DictWriter`) |

Untuk berkas, gabungkan dengan `baca_semua`/`tulis_semua`. Berbeda dengan
`csv.writer` Python yang memakai `\r\n`, `bentuk` mengakhiri setiap baris
dengan `\n`.

### 2.21 `pustaka/kura.idk` — grafika kura-kura

Padanan `turtle` Python, tanpa jendela: kura menggambar garis dan hasilnya
disimpan sebagai **SVG** yang bisa dibuka di peramban mana pun. Arah dan
koordinat mengikuti turtle: mulai di `(0, 0)` menghadap timur, sumbu y ke
atas, `kiri()` berlawanan arah jarum jam, sudut dalam **derajat**.

```indonesiaku
impor kura

untuk i dalam rentang(4) {
    kura.maju(100)
    kura.kiri(90)
}
cetak(kura.posisi(), kura.arah())   # (0.0, 0.0) 0.0
# kura.selesai("persegi.svg")      # tulis gambar ke berkas
cetak(kura.svg())
# <svg xmlns="http://www.w3.org/2000/svg" viewBox="-10 -110 120 120" ...>
# ...
#   <polyline points="0,0 100,0 100,-100 0,-100 0,0" stroke="black" stroke-width="1"/>
# ...
```

Kura sendiri, dengan pemanggilan berantai:

```indonesiaku
dari kura impor Kura, svg_dari

bintang = Kura()
bintang.warna("#c0392b").tebal(2)
untuk i dalam rentang(5) {
    bintang.maju(80).kanan(144)
}
lingkar = Kura()
lingkar.angkat_pena()
lingkar.ke(40, -60)
lingkar.turunkan_pena()
lingkar.warna("teal").lingkaran(30)
cetak(panjang(svg_dari([bintang, lingkar])) > 0)    # benar
```

| Nama (fungsi modul / metode `Kura`) | Padanan `turtle` | Deskripsi |
|------|------|-----------|
| `maju(n)`, `mundur(n)` | `forward`, `backward` | Bergerak `n` satuan |
| `kiri(d)`, `kanan(d)` | `left`, `right` | Berputar `d` derajat |
| `ke(x, y)` | `goto` | Pindah lurus ke titik; menggambar bila pena turun |
| `hadap(d)` | `setheading` | Arah mutlak: 0 timur, 90 utara |
| `pulang()` | `home` | Ke `(0, 0)` menghadap timur |
| `lingkaran(jari, busur=360, langkah=kosong)` | `circle` | Pusat `jari` satuan di kiri kura; `jari` negatif = searah jarum jam. Poligon dengan jumlah langkah bawaan yang sama seperti turtle |
| `angkat_pena()`, `turunkan_pena()` | `penup`, `pendown` | |
| `warna(w)` | `pencolor` | Nama warna SVG atau `"#rrggbb"` |
| `tebal(n)` | `pensize` | Tebal garis |
| `kecepatan(n)` | `speed` | 0–10, disimpan saja (SVG tidak beranimasi) |
| `posisi()`, `arah()` | `position`, `heading` | Dibulatkan 2 desimal |
| `svg()` | — | Teks SVG |
| `selesai(berkas="kura.svg")` / `k.simpan(berkas)` | `done` | Tulis SVG ke berkas |
| `reset()` | `reset` | Kura bawaan baru, kanvas kosong (fungsi modul) |
| `svg_dari(daftar_kura, tepi=10)` | — | Satu SVG dari beberapa kura |

Angka di SVG dibulatkan 2 desimal, jadi gambar yang sama selalu menghasilkan
teks yang sama persis. Kesalahan argumen menimbulkan `GalatKura`. Belum ada:
isian bentuk (`begin_fill`), teks (`write`), dan kanvas animasi di playground.

### 2.22 `pustaka/templat.idk` — templat HTML server-side

Merangkai HTML dengan `+` di dalam loop punya dua masalah: data pengguna
masuk mentah ke halaman (XSS), dan penggabungan string berulang bersifat
kuadratik. Pustaka `templat` menyelesaikan keduanya.

```indonesiaku
impor templat
impor jalur

# Dari string
cetak(templat.render_teks("<h1>{{ judul }}</h1>", {"judul": "Daftar"}))

# Dari berkas, dengan akar yang dikunci
buat_direktori("tampilan")
variabel berkas = jalur.gabung("tampilan", "siswa.html")
tulis_semua(berkas,
            "<ul>\n{% untuk s dalam siswa %}  <li>{{ s }}</li>\n{% akhiruntuk %}</ul>")
variabel m = templat.mesin("tampilan")
cetak(m.render("siswa.html", {"siswa": ["Budi", "Ani"]}))
hapus_berkas(berkas)
hapus_direktori("tampilan")
```

Berkas templat adalah HTML biasa dengan dua penanda:

```html
<h1>{{ judul }}</h1>
<ul>
{% untuk s dalam siswa %}  <li>{{ s }}</li>
{% akhiruntuk %}</ul>
{% jika kosong %}<p>Belum ada data.</p>{% akhirjika %}
```

| Penanda | Arti |
|---|---|
| `{{ a }}`, `{{ a.b }}`, `{{ a[0] }}` | Interpolasi, **selalu di-escape** |
| `{% untuk x dalam y %}` … `{% akhiruntuk %}` | Perulangan |
| `{% jika x %}` … `{% selain %}` … `{% akhirjika %}` | Kondisi |

| Fungsi | Keterangan |
|---|---|
| `render_teks(sumber, data)` | Render dari string |
| `mesin(akar)` → `.render(nama, data)` | Render berkas di bawah `akar` |
| `aman(teks)` | Tandai HTML tepercaya agar tidak di-escape |
| `GalatTemplat` | Kelas galat; semua kesalahan templat bisa di-`coba`/`kecuali` |

**`{{ }}` bukan kode Indonesiaku.** Yang diterima hanya rantai akses: nama,
`.kunci`, dan `[indeks]`. Pemanggilan fungsi dan operator ditolak dengan galat,
supaya berkas `.html` tidak pernah menjadi titik eksekusi kode.

**Escaping otomatis dan tidak bisa dimatikan per-templat.** `&`, `<`, `>`,
`"`, dan `'` selalu diganti. Satu-satunya jalan keluar adalah `aman()`, yang
namanya sengaja mencolok saat ditinjau. Nama yang tidak ada di data menaikkan
`GalatTemplat` dengan nomor baris — typo tidak berubah menjadi string kosong
diam-diam. Nama templat yang memuat `..` atau dimulai dari akar ditolak, karena
nama itu sering datang dari rute HTTP.

**Batas yang perlu diketahui.** Escaping ini benar untuk isi elemen dan untuk
nilai atribut yang diapit kutip — kutip tunggal maupun ganda, karena keduanya
di-escape. Ia **tidak** cukup untuk tiga konteks lain:

- atribut tanpa kutip (`<a title={{ x }}>`) — selalu apit dengan kutip;
- URL (`<a href="{{ u }}">`) — nilai `javascript:...` tetap lolos, periksa
  skema URL sendiri sebelum memasukkannya;
- di dalam `<script>` atau `<style>` — aturan escaping JS/CSS berbeda sama
  sekali; jangan interpolasi data pengguna ke sana.

Variabel `{% untuk %}` hanya hidup di dalam bloknya dan tidak menimpa data
bernama sama. Batas bawaan: sumber 1 MB, 10.000 tag, kedalaman 64.

**Komponen dan kerangka halaman.** `{% sertakan "komponen/kartu.html" %}`
menyisipkan templat lain dengan data yang sama, dan `{% perluas "dasar.html" %}`
membuat sebuah berkas mengisi lubang `{% blok %}` milik induknya:

```indonesiaku
impor templat
impor jalur

# Contoh ini membuat templatnya sendiri supaya benar-benar bisa dijalankan:
# contoh yang tak pernah dieksekusi gampang busuk.
buat_direktori("tampilan")
tulis_semua(jalur.gabung("tampilan", "dasar.html"),
    "<!doctype html>\n<title>{{ judul }}</title>\n"
    + "<body>{% blok isi %}isi bawaan{% akhirblok %}</body>\n")
tulis_semua(jalur.gabung("tampilan", "anak.html"),
    "{% perluas \"dasar.html\" %}\n"
    + "{% blok isi %}<p>Halo {{ nama }}</p>{% akhirblok %}\n")

variabel m = templat.mesin("tampilan")
cetak(m.render("anak.html", {"judul": "Daftar", "nama": "Budi"}))

hapus_berkas(jalur.gabung("tampilan", "dasar.html"))
hapus_berkas(jalur.gabung("tampilan", "anak.html"))
hapus_direktori("tampilan")
```

Isi kedua berkas di atas:

```html
<!-- tampilan/dasar.html -->
<!doctype html>
<title>{{ judul }}</title>
<body>{% blok isi %}isi bawaan{% akhirblok %}</body>
```

```html
<!-- tampilan/anak.html -->
{% perluas "dasar.html" %}
{% blok isi %}<p>Halo {{ nama }}</p>{% akhirblok %}
```

Blok yang tidak ditimpa anak memakai isi bawaan induk. Aturan yang berlaku:

- nama templat wajib literal berkutip, bukan ekspresi — nama yang berasal dari
  data berarti penyerang memilih berkas mana yang dibuka;
- `{% perluas %}` harus menjadi tag pertama, dan berkas yang memakainya hanya
  boleh berisi `{% blok %}` (teks di luar blok ditolak, bukan dibuang diam-diam);
- siklus (`a` → `b` → `a`) ditolak dengan menyebut rantainya, bukan
  menggantung; kedalaman sertakan dibatasi 32;
- `{% sertakan %}` hanya tersedia lewat `templat.mesin(akar)`. `render_teks()`
  menolaknya karena tanpa akar tidak ada yang membatasi berkas yang dibaca.

**Filter.** `{{ nilai | filter | filter:argumen }}` mengolah nilai sebelum
di-escape, dari kiri ke kanan. Argumen hanya satu dan harus literal (teks
berkutip atau bulat). Tanda kurung tetap ditolak di dalam `{{ }}`, jadi
tulis `potong:80`, bukan `potong(80)`.

| Filter | Hasil |
|---|---|
| `huruf_besar`, `huruf_kecil` | Seperti fungsi bawaan bernama sama |
| `judul` | Huruf pertama tiap kata kapital |
| `rapikan` | Spasi di awal dan akhir dibuang |
| `panjang` | Panjang teks, daftar, tupel, atau kamus |
| `potong:n` | Paling banyak `n` karakter, ditambah `…` bila dipotong |
| `gabung`, `gabung:"pemisah"` | Gabung daftar/tupel (pemisah bawaan `", "`); tiap elemen di-escape sendiri |
| `bawaan:"x"` | `x` bila nilainya `kosong` atau `""` |
| `angka:n` | Angka dengan tepat `n` digit di belakang titik |

Nama yang tidak ada di data tetap galat, **kecuali** filter pertamanya
`bawaan`: `{{ pengguna.julukan | bawaan:"Tamu" }}` adalah cara menulis kolom
opsional. **Tidak ada filter `aman`** (padanan `| safe` di Jinja) — HTML
tepercaya tetap hanya bisa ditandai dengan `aman()` di kode `.idk`. Filter
sendiri didaftarkan pada mesin:

```indonesiaku
impor templat
impor jalur

cetak(templat.render_teks(
    "{{ nama | judul }}: {{ bio | bawaan:\"-\" }} [{{ tag | gabung:\" / \" }}]",
    {"nama": "budi santoso", "bio": "", "tag": ["<a>", "b"]}))

buat_direktori("tampilan")
tulis_semua(jalur.gabung("tampilan", "harga.html"), "{{ harga | rupiah }}")
variabel m = templat.mesin("tampilan")
m.filter("rupiah", fungsi(nilai, arg) { kembalikan "Rp" + ke_teks(nilai) })
cetak(m.render("harga.html", {"harga": 15000}))
hapus_berkas(jalur.gabung("tampilan", "harga.html"))
hapus_direktori("tampilan")
```

Pada nilai `aman()`, `huruf_besar`, `huruf_kecil`, `judul`, dan `potong`
ditolak karena merusak HTML (`&amp;` menjadi `&AMP;`, potongan membelah tag);
olah teksnya di `.idk` sebelum `aman()`.

Fungsi filter menerima nilai dan argumennya (`kosong` bila tidak diberi).
Hasilnya di-escape seperti interpolasi biasa kecuali dibungkus `aman()`.
Nama filter bawaan tidak bisa ditimpa. Filter sendiri hanya tersedia lewat
`m.render()`, bukan `render_teks()`.

**Isi blok induk, komponen berparameter, dan makro.** Padanan `@parent`,
`@include(..., [...])`, dan komponen di Blade, atau `super()`, `with … only`,
dan `{% macro %}` di Jinja:

```html
<!-- di dalam berkas yang memakai {% perluas %} -->
{% blok kepala %}{% induk %}<script src="halaman.js"></script>{% akhirblok %}

{% sertakan "kartu.html" dengan judul=produk.nama, harga=produk.harga %}
{% sertakan "kartu.html" dengan judul=artikel.judul hanya %}

{% makro tombol(teks, jenis="utama") %}
  <button class="btn-{{ jenis }}">{{ teks }}</button>
{% akhirmakro %}
{% panggil tombol("Simpan") %}
{% panggil tombol(teks=label, jenis="bahaya") %}

{% impor "komponen/makro.html" %}
```

- `{% induk %}` hanya sah di dalam `{% blok %}` dan merender isi blok yang
  sama satu tingkat di atasnya, bertingkat bila induknya juga memakai
  `{% induk %}`. Blok tanpa tingkat di atasnya ditolak.
- `sertakan … dengan a=x, b="teks"` memberi komponen salinan data pemanggil
  ditambah nilai itu; data pemanggil tidak berubah. Akhiran `hanya` membuat
  komponen melihat nilai yang diberikan saja. Berkas yang disertakan punya
  makro dan bloknya sendiri.
- Badan makro hanya melihat parameternya, bukan data halaman. Nilai argumen
  dan parameter bawaan berupa jalan akses atau literal. Makro boleh dipanggil
  sebelum definisinya, dari berkas anak atau induk pada rantai `perluas`,
  atau dari berkas yang di-`{% impor %}` (hanya definisi makronya yang dibaca).
  Makro yang memanggil makro lain mencari di berkas tempat ia didefinisikan,
  jadi makro halaman tidak bisa membajak isi pustaka makro. Kedalaman
  panggilan makro dibatasi 32.
- `impor`, seperti `sertakan`, hanya tersedia lewat `templat.mesin(akar)`.

Hasil parsing di-cache secara bawaan oleh `templat.mesin(akar)`: tiap render
memeriksa ukuran dan waktu-ubah berkas, dan mengurai ulang hanya bila berubah.
Matikan dengan `templat.mesin(akar, salah)` bila metadata berkas tidak
tepercaya — biayanya sekitar tiga perempat kerja tiap render.

### Contoh lengkap: website dinamis dengan SQLite

`examples/web_mahasiswa.idk` menyambung templat, SQLite, dan router web:
halaman daftar dan halaman detail, parameter SQL yang diikat (bukan
dirangkai), id tak dikenal yang membalas 404, dan nama berisi `<script>`
yang ter-escape otomatis. Lihat juga
[§8.8 Aplikasi dengan templat](08_DEPLOYMENT.md) untuk menjalankannya di
container berikut tabel troubleshooting.

### 2.23 `pustaka/konfigurasi.idk` — konfigurasi aplikasi

Membaca berkas `.env` dan INI, lalu menggabungkan beberapa sumber dengan
prioritas yang jelas dan memvalidasinya dengan skema sederhana. BOM UTF-8 dan
akhir baris CRLF (berkas dari Notepad) ditangani otomatis; galat format
menyebut sumber dan nomor baris.

```indonesiaku
impor konfigurasi

env = konfigurasi.urai_env("PORT=8080  # komentar\nNAMA=\"Toko ☕\"\n")
cetak(env)        # {"PORT": "8080", "NAMA": "Toko ☕"}

ini = konfigurasi.urai_ini("[db]\nhost = localhost\nport: 5432\n")
cetak(ini)        # {"db": {"host": "localhost", "port": "5432"}}

konf = konfigurasi.muat(
    bawaan = {"mode": "dev"},
    awalan_lingkungan = "APP_",
    isi_lingkungan = {"APP_PORT": "9000", "APP_TOKEN": "abc"},
    skema = {
        "port": {"tipe": "bulat", "bawaan": 8000},
        "mode": {"pilihan": ["dev", "produksi"]},
        "token": {"tipe": "teks", "wajib": benar}
    }
)
cetak(konf["port"], konf.sumber("port"))   # 9000 lingkungan APP_PORT
cetak(konf)       # Konfigurasi({"port": 9000, "mode": "dev", "token": "***"})
```

| Nama | Deskripsi |
|------|-----------|
| `urai_env(teks, sumber="<teks>", batas_ukuran=BATAS_UKURAN)` | Kamus `KUNCI → teks`. Baris `KUNCI=nilai`, awalan `export ` boleh, `#` komentar (juga ` #` di belakang nilai tanpa kutip), `'...'` literal, `"..."` mengenal `\n \t \r \" \\`. Kunci ganda: yang terakhir menang |
| `urai_ini(teks, sumber="<teks>", batas_ukuran=BATAS_UKURAN)` | Kamus `bagian → kamus`. `kunci = nilai` atau `kunci: nilai`; komentar `;`/`#` di awal baris; kunci sebelum bagian pertama masuk bagian `""`. Bagian/kunci ganda adalah galat |
| `baca_env(jalur)`, `baca_ini(jalur)` | Sama, dari berkas; ukuran diperiksa **sebelum** berkas dibaca |
| `muat(bawaan, berkas, awalan_lingkungan, skema, isi_lingkungan, berkas_opsional=salah, batas_ukuran)` | Objek `Konfigurasi` (lihat di bawah) |
| `BATAS_UKURAN` | 1048576 bita (1 MiB) per sumber |
| `GalatKonfigurasi` | Galat format/validasi; `.pesan` (`"sumber:baris: alasan"`), `.sumber`, `.baris` |

**Prioritas `muat`** (yang kanan menimpa yang kiri): `bawaan` di skema <
argumen `bawaan` < `berkas` (urut daftar) < lingkungan.

- `berkas`: satu jalur atau daftar. `.env` (atau nama `.env`/`.env.*`) diurai
  sebagai `.env`; `.ini`/`.cfg`/`.conf` sebagai INI dan diratakan menjadi
  kunci `bagian.kunci`. Berkas yang tidak ada adalah galat, kecuali
  `berkas_opsional = benar`. `.toml`/`.yaml` belum didukung dan ditolak
  dengan pesan jelas.
- Lingkungan hanya dibaca bila `awalan_lingkungan` diberikan (boleh `""`), dan
  **hanya untuk kunci yang dikenal** (skema, bawaan, berkas) dengan nama
  `awalan + KUNCI` — huruf besar, `.` dan `-` menjadi `_` (`db.port` →
  `APP_DB_PORT`). Variabel lingkungan lain tidak pernah disalin.
  `isi_lingkungan` (kamus) menggantikan lingkungan OS, berguna untuk tes.
  Dalam mode `--sandbox` membaca lingkungan ditolak.
- `skema`: kunci → nama tipe (`"teks"`, `"bulat"`, `"desimal"`, `"logika"`,
  `"daftar"`) atau kamus `{tipe, wajib, bawaan, rahasia, pilihan}`. Teks
  dari berkas/lingkungan diubah ke tipenya: `logika` menerima
  `benar/salah/ya/tidak/true/false/yes/no/on/off/1/0`, `daftar` dipisah
  koma, `desimal` menolak `nan`/takhingga. Atribut atau tipe yang salah ketik
  adalah galat. Semua pelanggaran dilaporkan sekaligus dalam satu
  `GalatKonfigurasi`. Kunci di luar skema tetap ada sebagai teks.

**Objek `Konfigurasi`:** `konf["k"]` (`GalatKunci` bila tidak ada),
`konf.ambil(k, bawaan)`, `konf.punya(k)`, `konf.kunci()`, `konf.sumber(k)`
(`"bawaan"`, `"berkas <jalur>"` atau `"lingkungan <NAMA>"`),
`konf.adalah_rahasia(k)`, `konf.samarkan()`, `konf.ke_kamus()`, `panjang()`
dan `untuk k dalam konf`.

**Rahasia:** kunci bertanda `"rahasia": benar`, atau yang namanya memuat
`SANDI`, `PASSWORD`, `PASSWD`, `SECRET`, `RAHASIA`, `TOKEN`, `KEY` atau
`KUNCI` (tak peka huruf), tampil `***` saat objek dicetak, di `repr()` dan
`samarkan()`, dan nilainya tidak pernah dimasukkan ke pesan galat.
`ke_kamus()` dan `konf["k"]` tetap mengembalikan nilai asli.

Sengaja **tidak** didukung: interpolasi `${VAR}`, nilai `.env` multi-baris,
baris lanjutan INI, dan pengecilan kunci INI otomatis (beda dengan
`configparser` Python, kunci INI peka huruf besar).

### 2.24 `pustaka/unicode.idk` — huruf dan spasi Unicode

Builtin `huruf_besar()`/`huruf_kecil()` hanya mengubah huruf ASCII, dan itu
**tidak diubah** agar keluaran program lama tetap sama. Modul `unicode`
memberi versi yang hasilnya identik dengan `str.upper()`, `str.lower()` dan
`str.casefold()` Python (data Unicode 16.0) untuk code point U+0000–U+052F:
Latin (termasuk Latin Extended dan IPA), Yunani/Koptik dan Sirilik.

```indonesiaku
impor unicode

cetak(huruf_besar("straße"), unicode.huruf_besar("straße"))   # STRAßE STRASSE
cetak(unicode.huruf_kecil("ΟΔΥΣΣΕΥΣ"))                        # οδυσσευς
cetak(unicode.sama_tanpa_huruf("Straße", "STRASSE"))          # benar
cetak("[" + unicode.pangkas(chr(12288) + "halo" + chr(160)) + "]")   # [halo]
cetak(unicode.pisah_spasi("a" + chr(160) + "b  c"))           # ["a", "b", "c"]
```

| Nama | Deskripsi |
|------|-----------|
| `huruf_besar(t)` | Seperti `str.upper()`; bisa mengubah panjang (`"ß"` → `"SS"`) |
| `huruf_kecil(t)` | Seperti `str.lower()`, termasuk sigma akhir Yunani (`Σ` di akhir kata → `ς`) dan `"İ"` → `"i̇"` (2 karakter) |
| `lipat_huruf(t)` | Seperti `str.casefold()`, untuk membandingkan tanpa memedulikan huruf |
| `sama_tanpa_huruf(a, b)` | `lipat_huruf(a) == lipat_huruf(b)` |
| `adalah_spasi(c)` | Benar bila karakter `c` adalah spasi Unicode (29 code point `str.isspace()`, termasuk NBSP U+00A0 dan U+3000; *zero width space* U+200B bukan spasi) |
| `pangkas(t)` | Seperti `str.strip()` tanpa argumen |
| `pisah_spasi(t)` | Seperti `str.split()` tanpa argumen |

Batasan: karakter di luar U+0000–U+052F (misalnya Yunani politonik, Armenia,
huruf lebar penuh) dikembalikan apa adanya walau Python mengubahnya; aksara
tanpa huruf besar/kecil (CJK, emoji) memang tidak berubah. Aturan khusus
locale (huruf i Turki) tidak diterapkan, sama seperti Python. `panjang()`
tetap menghitung code point: `panjang("e" + chr(769))` adalah 2. Normalisasi
NFC/NFD dan kategori Unicode **belum tersedia**.

### 2.25 `pustaka/kripto.idk` — kriptografi modern

Enkripsi terautentikasi, penurunan kunci, hash kata sandi, dan bilangan acak
aman, semuanya dari mbedTLS yang sama dengan HTTPS — **tidak ada algoritme
yang ditulis sendiri**. Tersedia sejak versi setelah 1.7.0 (Fase 18 V1).
Bita adalah daftar bulat `0..255`; setiap masukan data juga menerima teks
(dienkode UTF-8).

**Enkripsi data** (AES-256-GCM). `enkripsi()` membuat nonce acak sendiri,
jadi nonce tidak mungkin terpakai ulang tanpa sengaja. `aad` (data tambahan
terautentikasi) tidak dienkripsi tetapi ikut dikunci: dekripsi gagal bila
nilainya berbeda — cocok untuk mengikat sandi ke ID pengguna.

```indonesiaku
impor kripto
impor base64
dari kripto impor GalatKripto   # `kecuali` butuh nama kelas polos

k = kripto.buat_kunci()
cetak(k)                                   # <Kunci 256-bit tersembunyi>

kemasan = kripto.enkripsi(k, "nomor kartu 4111", "pengguna:42")
disimpan = base64.enkode(kemasan)          # teks untuk basis data/cookie
cetak(kripto.dekripsi_teks(k, base64.dekode(disimpan), "pengguna:42"))

coba {
    kripto.dekripsi(k, base64.dekode(disimpan), "pengguna:43")
} kecuali GalatKripto sebagai e {
    cetak(e.pesan)   # dekripsi gagal: data rusak, atau kunci/nonce/AAD salah
}
```

Setiap kegagalan — tag, kunci, nonce, AAD salah, data rusak atau terpotong —
menaikkan `GalatKripto` dengan **pesan yang sama**, supaya penyerang tidak
mendapat petunjuk bagian mana yang salah. Tag diperiksa dalam waktu tetap
oleh mbedTLS sebelum satu bita pun plaintext dikembalikan.

**Hash kata sandi.** Simpan hasil `hash_kata_sandi()`, jangan `sha256()`:

```indonesiaku
impor kripto

h = kripto.hash_kata_sandi("rahasia-123")
cetak(h[:23])            # $pbkdf2-sha256$i=600000
cetak(kripto.verifikasi_kata_sandi("rahasia-123", h))   # benar
cetak(kripto.verifikasi_kata_sandi("rahasia-124", h))   # salah
jika kripto.perlu_hash_ulang(h) {
    h = kripto.hash_kata_sandi("rahasia-123")   # setelah login berhasil
}
```

Algoritmenya PBKDF2-HMAC-SHA-256, 600.000 iterasi (rekomendasi OWASP),
garam acak 16 bita, dalam format PHC
`$pbkdf2-sha256$i=<iterasi>$<garam>$<hash>` (base64 tanpa padding) — dapat
diverifikasi oleh `hashlib.pbkdf2_hmac` Python. **Argon2id belum tersedia**
karena mbedTLS tidak menyediakannya; PBKDF2 tidak *memory-hard* sehingga lebih
murah dipecahkan dengan GPU. Format yang memuat nama algoritme membuat migrasi
ke Argon2id kelak cukup lewat `perlu_hash_ulang()`. MD5/SHA-1/SHA-256 polos
**tidak boleh** dipakai untuk sandi.

**Token, MAC, dan kunci turunan:**

```indonesiaku
impor kripto

cetak(panjang(kripto.token_url()))          # 43  (32 bita acak, aman-URL)
dadu = kripto.bulat_acak_aman(6) + 1         # 1..6 tanpa bias modulo

k = kripto.buat_kunci()
tag = kripto.hmac_sha256(k, "id=42")
cetak(kripto.verifikasi_hmac(k, "id=42", tag))       # benar, waktu-tetap
k_enkripsi = kripto.hkdf_sha256(k, 32, "garam-app", "enkripsi")
k_mac = kripto.hkdf_sha256(k, 32, "garam-app", "mac")
cetak(k_enkripsi != k_mac)                            # benar
```

**AEAD dengan nonce eksplisit** — hanya untuk interoperabilitas dengan sistem
lain yang menentukan formatnya. **Satu nonce hanya untuk satu pesan per
kunci**: memakai ulang nonce membocorkan isi pesan dan memungkinkan pemalsuan.
Nonce acak 12 bita aman sampai sekitar 2³² pesan per kunci; untuk volume
lebih besar pakai `PenghitungNonce` atau ganti kunci.

```indonesiaku
impor kripto

k = kripto.buat_kunci()
pn = kripto.PenghitungNonce()          # satu objek per kunci
n = pn.berikutnya()                    # 12 bita, tidak pernah berulang
ct = kripto.aead_enkripsi(kripto.CHACHA20_POLY1305, k, n, "halo", "hdr")
cetak(panjang(ct))                     # 20 = 4 bita sandi + 16 bita tag
cetak(kripto.aead_dekripsi(kripto.CHACHA20_POLY1305, k, n, ct, "hdr"))
# [104, 97, 108, 111]
```

| Nama | Deskripsi |
|------|-----------|
| `acak_aman(n)` | `n` bita dari CSPRNG sistem (sama dengan `acak_bita`); gagal → `GalatOS`, tidak pernah turun ke `acak()` |
| `token_heksa(n=32)` / `token_url(n=32)` | Token acak heksa / base64url tanpa padding |
| `bulat_acak_aman(batas)` | Bulat `0..batas-1` merata (`secrets.randbelow`) |
| `buat_kunci()` / `Kunci(bita)` / `k.bita()` | Kunci 32 bita; isi tidak tampil di `cetak`/`repr`/f-string |
| `enkripsi(kunci, pesan, aad="")` | AES-256-GCM, nonce acak otomatis → kemasan `01 ‖ nonce(12) ‖ sandi ‖ tag(16)` |
| `dekripsi(kunci, kemasan, aad="")` / `dekripsi_teks(...)` | Buka kemasan → bita / teks; `GalatKripto` bila gagal |
| `aead_enkripsi(alg, kunci, nonce, pesan, aad="")` | `alg` = `AES_256_GCM` atau `CHACHA20_POLY1305`; hasil `sandi ‖ tag` |
| `aead_dekripsi(alg, kunci, nonce, sandi_tag, aad="")` | Kebalikannya; `GalatKripto` bila tag tidak cocok |
| `nonce_acak()` / `PenghitungNonce()` | Nonce 12 bita acak / berurutan |
| `hkdf_sha256(ikm, panjang, garam="", info="")` | HKDF RFC 5869 |
| `hmac_sha256(kunci, pesan)` / `verifikasi_hmac(kunci, pesan, tag)` | HMAC-SHA-256; verifikasi waktu-tetap |
| `sama_aman(a, b)` | Bandingkan rahasia (token, tag) dalam waktu tetap — jangan pakai `==` |
| `hash_kata_sandi(sandi, iterasi=600000)` | Hash sandi format PHC; iterasi `10.000..10.000.000` |
| `verifikasi_kata_sandi(sandi, tersimpan)` | `benar`/`salah`; `GalatKripto` bila format tersimpan rusak |
| `perlu_hash_ulang(tersimpan, iterasi=600000)` | `benar` bila hash dibuat dengan parameter lebih lemah |
| `GalatKripto` | Autentikasi gagal atau format hash rusak |

> **Batas jaminan.** Kunci yang masih berupa daftar bita di memori VM tidak
> bisa dihapus secara terjamin (salinan di C dihapus). Modul ini melindungi
> data aplikasi Anda; ia **tidak** membuat aman menjalankan kode `.idk` tak
> tepercaya — itu tugas isolasi container (lihat `--sandbox` di atas). Belum
> tersedia: Argon2id, tanda tangan Ed25519, XChaCha20.

### 2.26 `pustaka/berkas.idk` — salin, pindah, hapus pohon, berkas sementara

Operasi berkas dan direktori ala `shutil`/`tempfile` Python. Tersedia sejak
versi setelah 1.7.0 (Fase 17 V1). Menghapus **satu** berkas tetap memakai
builtin `hapus_berkas`.

```indonesiaku
impor berkas

d = berkas.direktori_sementara()              # direktori baru yang unik
tulis_semua(d + "/catatan.txt", "isi")
berkas.salin_berkas(d + "/catatan.txt", d + "/cadangan.txt")
berkas.ganti_nama(d + "/cadangan.txt", d + "/arsip ☕.txt")
cetak(berkas.sha256(d + "/arsip ☕.txt") == sha256("isi"))   # benar

buat_direktori(d + "/proyek")
tulis_semua(d + "/proyek/README", "baca aku")
berkas.salin_direktori(d + "/proyek", d + "/proyek-lama")
untuk entri dalam berkas.jelajahi_direktori(d) {
    dir, subdir, isi = entri
    cetak(dir[panjang(d):] atau ".", subdir, isi)
}
# . ["proyek", "proyek-lama"] ["arsip ☕.txt", "catatan.txt"]
# /proyek [] ["README"]
# /proyek-lama [] ["README"]

coba {
    berkas.salin_berkas(d + "/catatan.txt", d + "/arsip ☕.txt")
} kecuali GalatOS sebagai e {
    cetak("sudah ada" dalam e.pesan)          # benar: tanpa timpa=benar ditolak
}
berkas.hapus_direktori_bertingkat(d)
```

Aturan yang sama untuk semua fungsi:

- **Tidak menimpa tanpa diminta.** Tujuan yang sudah ada menaikkan `GalatOS`
  (`... sudah ada`) kecuali `timpa=benar`. Tujuan berupa direktori selalu
  ditolak — tidak ada "salin ke dalam direktori" ala `cp`; tulis jalur
  lengkapnya.
- **Galat portabel.** `GalatOS` dengan alasan yang sama di semua OS:
  `tidak ada`, `sudah ada`, `akses ditolak`, `adalah direktori`,
  `berkas yang sama`, `tautan simbolik`, `beda perangkat`. Galat dari modul
  sendiri memakai `GalatBerkas`, turunan `GalatOS`.
- **Tautan simbolik tidak diikuti dalam operasi pohon.** `salin_direktori`
  menolak tautan di dalam pohon kecuali `lewati_tautan=benar`;
  `hapus_direktori_bertingkat` menghapus tautannya saja (isi target aman) dan
  menolak akar yang berupa tautan; `jelajahi_direktori` mencantumkan tautan di
  daftar berkas tanpa menelusurinya. `salin_berkas` mengikuti tautan pada
  **asal** (seperti `cp`) tetapi tidak pernah menulis melalui tautan di
  **tujuan**. Di Windows, symlink dan *junction* sama-sama dianggap tautan.
- **Jalur Unicode** berfungsi di semua OS, termasuk Windows di luar code page.
- **`--sandbox`**: semua yang membuat, memindah, atau menghapus ditolak
  (`GalatIzin`); `info` dan `sha256` tetap boleh, seperti `baca_semua`.

| Nama | Deskripsi |
|------|-----------|
| `salin_berkas(asal, tujuan, timpa=salah)` | Salin isi + bit izin, per 64 KB (tidak dimuat ke memori). Menyalin ke dirinya sendiri ditolak (`berkas yang sama`) sebelum apa pun terpotong |
| `salin_direktori(asal, tujuan, timpa=salah, lewati_tautan=salah)` | Salin pohon. Dengan `timpa=benar` direktori yang ada dipakai ulang. Menyalin ke dalam dirinya sendiri ditolak. **Tidak atomik**: galat di tengah meninggalkan salinan sebagian |
| `ganti_nama(asal, tujuan, timpa=salah)` | Rename atomik di sistem berkas yang sama; lintas perangkat → `GalatBerkas` `beda perangkat` |
| `pindahkan(asal, tujuan, timpa=salah)` | Seperti `ganti_nama`, lintas perangkat jatuh ke salin + hapus (`shutil.move`) |
| `hapus_direktori_bertingkat(jalur)` | `rm -r` tanpa mengikuti tautan |
| `jelajahi_direktori(akar)` | Daftar tupel `(direktori, subdirektori, berkas)` dari atas ke bawah, nama terurut (`os.walk`) |
| `berkas_sementara(awalan="indk-", akhiran="", direktori=kosong)` | Buat berkas kosong **baru** bernama acak (CSPRNG), eksklusif, izin `0600`; kembalikan jalurnya. Anda yang menghapusnya |
| `direktori_sementara(awalan="indk-", direktori=kosong)` | Sama, untuk direktori (izin `0700`) |
| `direktori_sementara_sistem()` | `TMPDIR`, `TEMP`, `TMP`, lalu `/tmp`, lalu `.` |
| `info(jalur)` | Kamus `jenis` (`berkas`/`direktori`/`tautan`/`lain`), `ukuran`, `waktu_ubah`, `mode` (bit izin POSIX, `kosong` di Windows), `baca_saja`; tautan **tidak** diikuti |
| `sha256(jalur)` | SHA-256 heksa isi berkas, dibaca bertahap |
| `GalatBerkas` | Turunan `GalatOS` untuk galat dari modul ini |

> **Belum tersedia:** mengubah izin/pemilik (`chmod`/`chown`) dan membuat
> tautan. Berkas baca-saja di Windows (mis. objek `.git`) membuat
> `hapus_direktori_bertingkat` gagal dengan `akses ditolak` — tidak dihapus
> paksa diam-diam. Tanpa `timpa`, `ganti_nama` di Linux/macOS memeriksa
> tujuan sebelum rename, jadi ada jendela balapan yang sempit; Windows
> menolaknya secara atomik. Berkas skrip bernama `berkas.idk` di direktori
> yang sama membayangi modul ini (direktori skrip dicari lebih dulu).

### 2.27 `pustaka/jaringan.idk` — unduh berkas HTTP/HTTPS

`unduh()` menyimpan respons HTTP/HTTPS langsung ke disk **tanpa menampung
seluruh badan di memori**. Tersedia sejak versi setelah 1.7.0 (Fase 17 V1).

```indonesiaku-takjalan
impor jaringan
dari jaringan impor GalatUnduhan

coba {
    hasil = jaringan.unduh(
        "https://contoh.invalid/rilis/indk-1.7.0.tar.gz", "indk.tar.gz",
        sha256="9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08",
        maks_ukuran=50 * 1024 * 1024,           # byte
        batas_waktu=30,                          # detik per baca/tulis soket
        kemajuan=fungsi(n, total) { cetak(n, "/", total) })
    cetak(hasil["ukuran"], hasil["sha256"], hasil["url"])
} kecuali GalatUnduhan sebagai e {
    cetak("unduhan gagal:", e.pesan)   # 4xx/5xx, checksum, ukuran, batal, terputus
}
```

Jaminan:

- **Tidak ada berkas setengah jadi.** Badan ditulis ke berkas sementara
  bernama acak **di direktori tujuan**, lalu di-rename (atomik) ke tujuan
  hanya setelah statusnya 2xx, panjangnya sama dengan `Content-Length`,
  tidak melewati `maks_ukuran`, dan SHA-256-nya cocok. Galat apa pun —
  termasuk koneksi putus atau checksum salah — menghapus berkas sementara
  itu; tujuan tidak pernah tersentuh.
- **Streaming.** Badan ditulis per potongan soket (paling banyak 64 KB);
  pemakaian memori tidak bergantung pada ukuran berkas. Data biner disimpan
  byte demi byte, apa adanya.
- **TLS selalu diverifikasi** (sama seperti `http_ambil`), dan pengalihan
  diikuti dengan aturan yang sama: paling banyak `maks_pengalihan` lompatan
  (bawaan 5, `0` = tidak diikuti), penurunan `https` → `http` ditolak,
  putaran ditolak.
- **Tujuan yang sudah ada** ditolak sebelum koneksi dibuka kecuali
  `timpa=benar`. Hanya `http://` dan `https://` yang diterima.
- `kemajuan(diterima, total)` dipanggil setiap potongan tiba (`total` adalah
  `kosong` bila server tidak mengirim `Content-Length`). Kembalikan `salah`
  untuk **membatalkan**.

| Parameter | Bawaan | Arti |
|-----------|--------|------|
| `timpa` | `salah` | Izinkan mengganti berkas yang sudah ada (atomik) |
| `batas_waktu` | `30` | Detik per operasi baca/tulis soket; lewat → `Galat` "melewati batas waktu" |
| `maks_ukuran` | `kosong` | Batas byte, dicek terhadap `Content-Length` **dan** byte yang benar-benar tiba |
| `sha256` | `kosong` | Heksa yang diharapkan (huruf besar/kecil) |
| `kemajuan` | `kosong` | `fungsi(diterima, total)`; `salah` = batal |
| `maks_pengalihan` | `5` | Lompatan pengalihan maksimum |
| `kepala` | `kosong` | Kamus header tambahan, mis. `{"Authorization": "Bearer ..."}` |

Hasilnya kamus `{"jalur", "url" (URL terakhir setelah pengalihan), "ukuran",
"sha256", "tipe" (Content-Type atau kosong)}`. `GalatUnduhan` adalah turunan
`GalatJaringan` → `GalatOS`, jadi penangan `GalatOS` yang sudah ada ikut
menangkapnya. Ditolak (`GalatIzin`) di `--sandbox`.

> **Belum tersedia:** melanjutkan unduhan (`Range`), batas waktu untuk
> *membuka* koneksi dan batas waktu total (yang ada adalah batas per operasi
> baca/tulis), serta HTTP/1.1 *chunked* (permintaan dikirim sebagai HTTP/1.0,
> jadi server tidak boleh memakainya). FTP/FTPS, SSH, dan SFTP direncanakan
> (Fase 17 langkah 4–6) dan belum ada.

## 3. Aplikasi Contoh: Todo-list JSON DB

Dua aplikasi contoh lengkap menunjukkan penggunaan pustaka standar untuk
membangun aplikasi nyata dengan penyimpanan JSON.

### 3.1 CRUD berbasis command-line

`examples/todo_cli.idk` — memetakan operasi REST ke perintah CLI
(`tambah`=POST, `daftar`/`lihat`=GET, `selesai`/`ubah`=PUT, `hapus`=DELETE):

{{embed:examples/todo_cli.idk}}

### 3.2 Server REST API (HTTP)

`examples/todo_server.idk` — server HTTP sungguhan (soket TCP) dengan endpoint
`GET/POST/PUT/DELETE /todos`:

{{embed:examples/todo_server.idk}}

### 3.3 CRUD REST API di atas SQLite

`examples/api_siswa.idk` — menggabungkan tiga bagian yang sudah dibahas
menjadi satu aplikasi: `web.Aplikasi` untuk routing dan `app.jalankan()`,
builtin `db_*` untuk SQLite, dan `pustaka/json.idk` untuk badan respons.

Bedanya dengan `todo_server.idk`: penyimpanannya SQLite (bukan JSON di
memori), gelung servernya milik pustaka (bukan ditulis tangan), dan validasi
badan ada di satu fungsi yang dipakai POST maupun PUT.

Dua hal yang mudah salah dan sengaja ditunjukkan di sini:

- **`db_baris_terpengaruh()` sesudah UPDATE/DELETE.** Tanpa itu, `PUT` atau
  `DELETE` ke id yang tidak ada tetap menjawab `200` seolah berhasil, padahal
  tidak ada baris yang tersentuh.
- **Parameter `?`, bukan perangkaian teks SQL.** Nama dari klien tidak pernah
  masuk ke dalam teks query, jadi badan seperti
  `{"nama": "Evil'); DROP TABLE siswa;--"}` tersimpan sebagai nama biasa dan
  tabelnya tetap utuh. `tests/api_siswa_server.sh` membuktikannya.

{{embed:examples/api_siswa.idk}}

---

Bab berikutnya: [8. Deployment](08_DEPLOYMENT.md) — menaruh server ini di
internet dengan Docker, reverse proxy, dan TLS.
