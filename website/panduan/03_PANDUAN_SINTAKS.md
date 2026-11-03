# 3. Panduan Sintaks - Bahasa Pemrograman Indonesiaku

Panduan lengkap untuk memahami dan menggunakan sintaks Bahasa Pemrograman Indonesiaku dengan contoh-contoh praktis.

---

## Dua hal yang membedakan Indonesiaku dari Python

Indonesiaku meniru **perilaku** Python (truthiness, operator, lingkup
variabel, model galat), tetapi menulisnya berbeda:

1. **Blok dibungkus `{` dan `}` seperti PHP**, bukan titik dua dan indentasi.

   ```indonesiaku
   fungsi sapa(nama) {
       jika nama == "" {
           kembalikan "Halo, dunia!"
       }
       kembalikan "Halo, " + nama
   }
   ```

2. **Tidak ada aturan indentasi sama sekali.** Spasi di awal baris tidak
   pernah berarti. Kode rata kiri seluruhnya tetap sah, begitu pula indentasi
   yang tidak konsisten:

   ```indonesiaku
   fungsi faktorial(n) {
   jika n > 1 {
   kembalikan n * faktorial(n - 1)
   }
   kembalikan 1
   }
   cetak(faktorial(5))        # 120
   ```

   Indentasi hanya soal keterbacaan — tidak akan pernah menjadi galat. Yang
   menutup blok adalah `}`, bukan kembalinya indentasi.

> Contoh: [`examples/tanpa_indentasi.idk`](../examples/tanpa_indentasi.idk)

---

## 📋 Daftar Isi

1. [Tipe Data](#tipe-data)
2. [Variabel](#variabel)
3. [Operator](#operator)
4. [Percabangan](#percabangan)
5. [Perulangan](#perulangan)
6. [Fungsi](#fungsi)
7. [Built-in Functions](#built-in-functions)
8. [Comments](#comments)
9. [Best Practices](#best-practices)

---

## Tipe Data

Bahasa Pemrograman Indonesiaku mendukung 4 tipe data utama:

### 1. Numbers (Angka)

Digunakan untuk menyimpan nilai numerik (integer dan float).

```indonesiaku
# Integer
x = 42
y = -17
z = 0

# Float
pi = 3.14159
suhu = -40.5
nol = 0.0

# Operasi math
hasil = x + y
print_value = pi * 2
```

### 2. Strings (Text)

Digunakan untuk menyimpan teks.

```indonesiaku
# String dengan double quotes
nama = "Teguh Rijanandi"
pesan = "Halo, dunia!"

# String dengan single quotes
greeting = 'Selamat pagi'
alamat = 'Jakarta, Indonesia'

# String kosong
text_kosong = ""
```

### 3. Booleans (Nilai Logika)

Digunakan untuk nilai benar/salah.

```indonesiaku
# Boolean values
aktif = benar      # True
nonaktif = salah   # False

# Dari operasi perbandingan
x = 10
y = 5
hasil = x > y      # benar
hasil2 = x < y     # salah
```

### 4. Null (Kosong)

Digunakan untuk nilai yang tidak ada atau belum diinisialisasi.

```indonesiaku
# Null value
data = kosong
hasil = kosong
```

---

## Variabel

Variabel adalah tempat menyimpan data yang dapat berubah nilainya.

### Deklarasi dan Assignment

```indonesiaku
# Sederhana
x = 10
nama = "Budi"
aktif = benar

# Multiple assignment (satu per satu)
a = 1
b = 2
c = 3

# Reassignment
x = 20
x = x + 5
nama = "Teguh"
```

### Naming Rules (Aturan Penamaan)

```indonesiaku-takjalan
# VALID - Boleh digunakan
nama = "Budi"
nama_lengkap = "Budi Santoso"
umur1 = 25
_private = 42
myVar = "test"

# INVALID - TIDAK boleh digunakan
1nama = "Error"       # Tidak boleh diawali angka
jika = 10            # Tidak boleh pakai keyword
untuk-loop = 0       # Tidak boleh gunakan hyphen
```

### Scope (Jangkauan)

```indonesiaku
# Global scope
x = 10

fungsi test() {
    # Local scope
    y = 20
    cetak(x)  # Bisa akses global x
    cetak(y)  # Bisa akses local y
}

test()
# cetak(y)   # ERROR: y tidak ada di global scope
```

---

## Operator

### Tipe Angka: `bulat` dan `desimal`

Seperti Python, ada dua tipe angka:

| Literal / hasil | Tipe | Contoh |
|---|---|---|
| Tanpa titik atau eksponen | `bulat` (int64, presisi penuh) | `7`, `-42`, `9007199254740993` |
| Ada titik | `desimal` (double) | `7.0`, `3.14` |

```indonesiaku
cetak(7)              # 7
cetak(7.0)            # 7.0   (desimal selalu menampilkan .0)
cetak(6 / 2)          # 3.0   ('/' selalu menghasilkan desimal)
cetak(7 // 2)         # 3     ('//' dan '%' pada bulat menghasilkan bulat)
cetak(2 ** 10)        # 1024  (bulat ** bulat dengan pangkat >= 0 tetap bulat)
cetak(1 + 2.0)        # 3.0
cetak(1 == 1.0)       # benar
cetak(1 adalah 1.0)   # salah  ('adalah' = identitas; tipenya berbeda)
```

`bulat` adalah bilangan 64-bit. Bila hasil aritmetika melampaui jangkauannya
(-9223372036854775808 sampai 9223372036854775807), program menaikkan
`GalatLuapan` — nilainya **tidak** pernah berubah diam-diam menjadi `desimal`
yang kehilangan presisi:

```indonesiaku
coba {
    cetak(9223372036854775807 + 1)
} kecuali GalatLuapan sebagai e {
    cetak(e.pesan)   # Hasil penjumlahan melebihi jangkauan bilangan bulat.
}
```

Bignum (bilangan bulat tanpa batas) belum ada. Untuk angka yang lebih besar,
gunakan `desimal` dengan kesadaran presisinya 53 bit, atau tunggu Fase 2b.

> Contoh: [`examples/tipe_bulat.idk`](../examples/tipe_bulat.idk)

### Operator Aritmatika

```indonesiaku
a = 10
b = 3

jumlah = a + b        # 13 (penjumlahan)
selisih = a - b       # 7  (pengurangan)
kali = a * b          # 30 (perkalian)
bagi = a / b          # 3.333... (pembagian, selalu desimal)
bagi_bulat = a // b   # 3  (pembagian bulat, dibulatkan ke bawah)
sisa = a % b          # 1  (modulo/sisa bagi)
pangkat = 2 ** 3      # 8  (perpangkatan)
```

Aturannya sama dengan Python:

```indonesiaku
-7 // 2       # -4  (dibulatkan ke bawah, bukan ke arah nol)
-7 % 3        # 2   (hasil % mengikuti tanda pembagi)
2 ** 3 ** 2   # 512 (pangkat dihitung dari kanan: 2 ** 9)
-2 ** 2       # -4  (sama dengan -(2 ** 2))
```

> Contoh: [`examples/operator_aritmetika.idk`](../examples/operator_aritmetika.idk)

### Operator Bitwise

Bekerja pada bit-bit bilangan `bulat`, sama seperti di Python:

```indonesiaku
cetak(5 & 3)      # 1   (AND: bit yang menyala di keduanya)
cetak(5 | 3)      # 7   (OR: bit yang menyala di salah satu)
cetak(5 ^ 3)      # 6   (XOR: menyala di salah satu saja)
cetak(~5)         # -6  (NOT: ~a sama dengan -a - 1)
cetak(1 << 4)     # 16  (geser kiri = kali 2 sebanyak 4 kali)
cetak(-8 >> 1)    # -4  (geser kanan, dibulatkan ke bawah)

x = 0b1010
x |= 0b0101       # 15  (ada juga &=, ^=, <<=, >>=)
```

Contoh nyata — izin sebagai *flag*:

```indonesiaku
BACA = 1
TULIS = 2
izin = BACA | TULIS
jika izin & TULIS { cetak("boleh menulis") }
```

Aturan penting:

- `^` adalah XOR, **bukan** pangkat. Pangkat tetap `**`.
- Hanya untuk `bulat`. Memakai `desimal` (mis. `2.5 & 1`) menaikkan `GalatTipe`.
- `benar & salah` menghasilkan logika (`salah`), seperti Python. Untuk
  membalik logika pakai `tidak`, bukan `~`.
- Geser dengan jumlah negatif menaikkan `GalatNilai`; hasil `<<` yang melebihi
  jangkauan bulat 64-bit menaikkan `GalatLuapan`.

> Contoh: [`examples/operator_bitwise.idk`](../examples/operator_bitwise.idk)

> **Akhir baris adalah pemisah pernyataan.** Gunakan satu pernyataan per baris tanpa `;`. Newline di dalam `()` atau `[]` diabaikan sehingga argumen dan list dapat ditulis multi-baris; `{}` tetap digunakan untuk blok.

### Operator Perbandingan

```indonesiaku
x = 10
y = 5

# Hasil adalah boolean (benar atau salah)
x == y    # salah (sama dengan)
x != y    # benar (tidak sama dengan)
x > y     # benar (lebih besar)
x < y     # salah (lebih kecil)
x >= y    # benar (lebih besar atau sama)
x <= y    # salah (lebih kecil atau sama)
```

### Identitas dan rantai perbandingan

`==` membandingkan **isi** (struktural): dua daftar/kamus/rentang sama bila
isinya sama, meski objeknya berbeda. `adalah` membandingkan **identitas
objek**; `bukan adalah` adalah kebalikannya.

```indonesiaku
cetak([1, 2] == [1, 2])            # benar  — isi sama
cetak([1, [2]] == [1, [2]])        # benar  — rekursif
cetak({"a": 1, "b": 2} == {"b": 2, "a": 1})   # benar — urutan tak berpengaruh
cetak(rentang(3) == rentang(3))    # benar

variabel daftar = [1]
variabel alias = daftar
cetak(daftar adalah alias)       # benar  — objek yang sama
cetak(daftar adalah [1])         # salah  — isi sama, objek berbeda
cetak(daftar bukan adalah [1])   # benar
```

Instansi kelas (dan fungsi, berkas, soket) tetap dibandingkan berdasarkan
identitas, bukan isi. Operator `dalam` ikut memakai kesetaraan struktural,
jadi `[1, 2] dalam [[1, 2]]` bernilai `benar`.

Perbandingan struktural (dan hash tupel/`himpunan_beku` sebagai kunci kamus)
menelusuri paling banyak 1000 tingkat sarang. Struktur yang lebih dalam,
atau daftar yang memuat dirinya sendiri, menaikkan `GalatRekursi`, seperti
`RecursionError` di Python. Tidak ada hasil `salah` diam-diam dan tidak ada
crash.

Perbandingan dapat dirantai seperti Python: operand tengah dihitung sekali dan evaluasi berhenti saat salah.

```indonesiaku
x = 2
buah = ["apel", "jeruk"]

cetak(1 < x < 3)
cetak("apel" dalam buah == buah)
```

### Gabung (`+`) dan Ulang (`*`)

`+` menggabungkan dua teks atau dua daftar; `*` mengulang teks/daftar sebanyak
n kali (urutan operand bebas).

```indonesiaku
cetak([1, 2] + [3])        # [1, 2, 3]
cetak("ha" + "lo")         # halo

cetak([0] * 3)             # [0, 0, 0]
cetak(3 * [0])             # [0, 0, 0]
cetak("-" * 20)            # garis pemisah
cetak("ab" * 2)            # abab
```

Catatan:

- Hasilnya selalu objek **baru**; sumbernya tidak berubah.
- Pengali harus bilangan bulat. Nol atau negatif menghasilkan hasil kosong
  (seperti Python); pecahan → `GalatTipe`.
- Hasil dibatasi 65535 elemen/karakter agar `[0] * 999999` tidak menghabiskan
  memori diam-diam; melebihi batas → `GalatNilai`.
- Mencampur tipe (`[1] + "a"`) → `GalatTipe`.

> Contoh: [`examples/gabung_sama.idk`](../examples/gabung_sama.idk)

### Penanganan galat bertipe

Semua galat runtime dan fungsi bawaan dapat ditangkap sebagai objek dengan properti `.pesan`. Gunakan kelas induk atau beberapa kelas untuk memilih galat yang ditangani.

```indonesiaku
coba {
    cetak(1 / 0)
} kecuali GalatAritmetika sebagai e {
    cetak(e.pesan)
} kecuali (GalatNilai, GalatTipe) sebagai e {
    cetak("masukan tidak sah:", e)
} kecuali e {  # tetap menangkap semua
    cetak(e)
}
```

Kelas bawaan mencakup `Galat`, `GalatAritmetika`, `GalatBagiNol`, `GalatPencarian`, `GalatIndeks`, `GalatKunci`, `GalatTipe`, `GalatNilai`, `GalatNama`, `GalatAtribut`, `GalatOS` (sistem operasi), dan `GalatIzin` (turunan `GalatOS`, penolakan `--sandbox`).

Klausa `kecuali` dicoba **berurutan dari atas**, dan yang pertama cocok
menang — jadi tulis tipe yang paling khusus lebih dulu, baru yang umum.
Karena `kecuali e` tanpa tipe menangkap semuanya, ia wajib menjadi klausa
**terakhir**; menaruhnya di tengah adalah galat kompilasi. Bila tidak ada
satu pun klausa yang cocok, galatnya dilempar ulang ke `coba` di luarnya
(atau menghentikan program bila tidak ada).

### Operator Logika

```indonesiaku
a = benar
b = salah

cetak(a dan b)    # salah (AND - keduanya harus benar)
cetak(a atau b)   # benar (OR - salah satu harus benar)
cetak(tidak a)    # salah (NOT - kebalikan nilai)
cetak(tidak b)    # benar (NOT - kebalikan nilai)

# Kombinasi
cetak((a atau b) dan (tidak b))  # benar
```

> **Perhatian:** akhir baris belum mengakhiri pernyataan. Baris yang diawali
> `(` atau `-` bisa tersambung ke baris sebelumnya (`tidak b` lalu baris
> `(a atau b)` terbaca sebagai pemanggilan `b(...)`). Pisahkan dengan `;` bila
> perlu, atau hindari baris yang diawali kurung buka.

### Operator Keanggotaan (`dalam`, `tidak dalam`)

```indonesiaku
buah = ["apel", "jeruk"]
cetak("apel" dalam buah)          # benar  (elemen list)
cetak("mangga" tidak dalam buah)  # benar
cetak("ana" dalam "pisang")       # salah  (substring teks)
cetak("san" dalam "pisang")       # benar
cetak(5 dalam rentang(10))        # benar

harga = kamus()
harga["apel"] = 5000
cetak("apel" dalam harga)         # benar  (kunci kamus)
```

> Contoh: [`examples/operator_dalam.idk`](../examples/operator_dalam.idk)

### Prioritas Operator

Urutan eksekusi dari tertinggi ke terendah:

```
1.  **           (pangkat, dihitung dari kanan)
2.  -, ~         (minus unary, NOT bitwise)
3.  *, /, //, %  (perkalian, pembagian, pembagian bulat, modulo)
4.  +, -         (penjumlahan, pengurangan)
5.  <<, >>       (geser bit)
6.  &            (AND bitwise)
7.  ^            (XOR bitwise)
8.  |            (OR bitwise)
9.  <, <=, >, >=, dalam, tidak dalam (perbandingan & keanggotaan)
10. ==, !=       (kesetaraan)
11. tidak        (NOT: tidak a == b berarti tidak (a == b))
12. dan          (AND)
13. atau         (OR)
```

**Contoh:**
```indonesiaku
hasil1 = 2 + 3 * 4      # 14, bukan 20 (* dikerjakan terlebih dahulu)
hasil2 = (2 + 3) * 4    # 20 (kurung diprioritaskan)
hasil3 = 10 > 5 dan 3 < 2 atau 4 == 4  # benar
```

---

## Percabangan

### If Statement

Struktur dasar percabangan:

```indonesiaku
kondisi = benar

jika kondisi {
    # code jika kondisi benar
}
```

Kondisi tidak harus `benar`/`salah`. Seperti Python, nilai berikut dianggap
**salah**: `salah`, `kosong`, `0`, `""` (teks kosong), `[]` (list kosong),
kamus kosong, dan `rentang(0)`. Semua nilai lain dianggap **benar**:

```indonesiaku
antrian = [1, 2, 3]
selagi antrian {                 # berhenti saat list kosong
    cetak(hapus_pada(antrian, 0))
}

nama = ""
cetak(nama atau "tanpa nama")    # tanpa nama
```

> Contoh: [`examples/kebenaran.idk`](../examples/kebenaran.idk)

**Contoh sederhana:**
```indonesiaku
x = 10

jika x > 0 {
    cetak("Positif")
}
```

### If-Else

```indonesiaku
x = -5

jika x > 0 {
    cetak("Positif")
} selain {
    cetak("Tidak positif")
}
```

### If-Elif-Else (Multiple Conditions)

```indonesiaku
nilai = 85

jika nilai >= 90 {
    cetak("Grade A")
}
jika nilai >= 80 {
    cetak("Grade B")
}
jika nilai >= 70 {
    cetak("Grade C")
}
selain {
    cetak("Grade F")
}
```

### Nested If

```indonesiaku
x = 15

jika x > 0 {
    jika x > 10 {
        cetak("Lebih dari 10")
    } selain {
        cetak("Antara 1 dan 10")
    }
} selain {
    cetak("Nol atau negatif")
}
```

### Percabangan dengan Operator Logika

```indonesiaku
# AND condition
x = 50
jika x > 0 dan x < 100 {
    cetak("Di antara 0 dan 100")
}

# OR condition
x = -5
jika x < 0 atau x > 100 {
    cetak("Di luar range 0-100")
}

# NOT condition
aktif = salah
jika tidak aktif {
    cetak("Tidak aktif")
}

# Kombinasi kompleks
x = 50
jika (x > 0 dan x < 100) atau x == 0 {
    cetak("Valid")
}
```

---

## Perulangan

### While Loop

Struktur dasar loop:

```indonesiaku
kondisi = salah

selagi kondisi {
    # code yang diulang
}
```

**Contoh: Counting**
```indonesiaku
i = 0
selagi i < 5 {
    cetak(i)
    i = i + 1
}
# Output: 0, 1, 2, 3, 4
```

**Contoh: Countdown**
```indonesiaku
hitungan = 5
selagi hitungan > 0 {
    cetak(hitungan)
    hitungan = hitungan - 1
}
# Output: 5, 4, 3, 2, 1
```

### Nested Loops

```indonesiaku
# Multiplication table
i = 1
selagi i <= 3 {
    j = 1
    selagi j <= 3 {
        cetak(i * j)
        j = j + 1
    }
    i = i + 1
}
# Output: 1,2,3,2,4,6,3,6,9
```

### Loop dengan Kondisi Kompleks

```indonesiaku
x = 10
selagi x > 0 dan x != 5 {
    cetak(x)
    x = x - 1
}
# Output: 10, 9, 8, 7, 6
```

---

## Fungsi

### Definisi Fungsi

Struktur dasar fungsi:

```indonesiaku
fungsi nama_fungsi() {
    # code di dalam fungsi
}
```

### Fungsi Tanpa Parameter

```indonesiaku
fungsi sapa() {
    cetak("Halo!")
}

# Memanggil fungsi
sapa()
```

### Fungsi dengan Parameter

```indonesiaku
fungsi sapa_nama(nama) {
    cetak("Halo,", nama)
}

sapa_nama("Teguh")      # Output: Halo, Teguh
sapa_nama("Budi")       # Output: Halo, Budi
```

### Parameter dengan Nilai Bawaan

Parameter boleh punya nilai bawaan, sehingga argumennya opsional saat dipanggil.

```indonesiaku
fungsi sapa(nama, salam="Halo") {
    cetak(salam + ", " + nama)
}

sapa("Budi")            # Halo, Budi
sapa("Budi", "Hai")     # Hai, Budi
```

Nilai bawaan boleh berupa ekspresi apa pun, termasuk merujuk parameter di
sebelah kirinya:

```indonesiaku
fungsi kotak(lebar, tinggi=lebar) {   # bujur sangkar bila tinggi tak diisi
    kembalikan lebar * tinggi
}
cetak(kotak(3))        # 9
cetak(kotak(3, 4))     # 12
```

Parameter wajib tidak boleh ditulis setelah parameter yang punya nilai
bawaan — `fungsi f(a=1, b)` adalah galat.

> **Berbeda dari Python:** nilai bawaan di Indonesiaku dievaluasi **setiap
> kali fungsi dipanggil**, bukan sekali saat fungsi didefinisikan. Jadi
> daftar bawaan tidak pernah terbawa ke panggilan berikutnya:
>
> ```indonesiaku
> fungsi kumpul(x, daftar=[]) {
>     tambah(daftar, x)
>     kembalikan daftar
> }
> cetak(kumpul(1))     # [1]
> cetak(kumpul(2))     # [2]   — di Python hasilnya [1, 2]
> ```
>
> Jebakan *mutable default argument* yang terkenal di Python tidak ada di sini.

Nilai bawaan juga berlaku untuk `init` dan metode kelas.

> Contoh: [`examples/argumen_bawaan.idk`](../examples/argumen_bawaan.idk)

### Argumen Bernama

Argumen boleh dikirim berdasarkan **nama parameter**, bukan urutannya.

```indonesiaku
fungsi gambar(teks, lebar=80, garis="-", tengah=salah) {
    cetak(teks, lebar, garis, tengah)
}

gambar("Judul")                      # Judul 80 - salah
gambar("Judul", tengah=benar)        # lewati lebar dan garis
gambar("Judul", garis="=", lebar=40) # urutan bebas
gambar(teks="Judul")                 # parameter wajib pun boleh dinamai
```

Kegunaan utamanya adalah **melewati parameter di tengah** dan membuat
pemanggilan terbaca sendiri: `gambar("Judul", tengah=benar)` jauh lebih jelas
daripada `gambar("Judul", 80, "-", benar)`.

Aturannya:

- Argumen posisional harus mendahului argumen bernama —
  `f(b=1, 2)` adalah galat kompilasi.
- Nama harus cocok dengan nama parameter; kalau tidak → `GalatTipe`.
- Satu parameter tidak boleh diisi dua kali (posisional + bernama) → `GalatTipe`.
- Parameter wajib yang tidak terisi dilaporkan dengan **namanya**:
  `Parameter wajib 'a' tidak diisi.`
- Berlaku juga untuk `init`, metode kelas, dan `super`.
- Fungsi bawaan (`panjang`, `cetak`, dll.) belum menerima argumen bernama.

> Contoh: [`examples/argumen_bernama.idk`](../examples/argumen_bernama.idk)

### Fungsi dengan Multiple Parameters

```indonesiaku
fungsi tambah(a, b) {
    cetak("Menambah", a, "dan", b)
}

fungsi kurangi(a, b) {
    cetak("Mengurangi", a, "dan", b)
}

tambah(5, 3)           # Menambah 5 dan 3
kurangi(10, 4)         # Mengurangi 10 dan 4
```

### Fungsi dengan Return Value

```indonesiaku
fungsi jumlah(a, b) {
    kembalikan a + b
}

hasil = jumlah(5, 3)
cetak("Hasilnya:", hasil)  # Output: Hasilnya: 8
```

### Fungsi Rekursif

**Factorial:**
```indonesiaku
fungsi faktorial(n) {
    jika n <= 1 {
        kembalikan 1
    }
    kembalikan n * faktorial(n - 1)
}

cetak(faktorial(5))  # Output: 120
```

**Fibonacci:**
```indonesiaku
fungsi fibonacci(n) {
    jika n <= 1 {
        kembalikan n
    }
    kembalikan fibonacci(n - 1) + fibonacci(n - 2)
}

cetak(fibonacci(10))  # Output: 55
```

---

## Anotasi Tipe (Opsional)

Parameter, nilai kembali, dan variabel boleh diberi catatan tipe. Anotasi
**tidak diperiksa saat program berjalan** — ia murni dokumentasi yang ikut
terbaca editor, dan tidak mengubah hasil program sama sekali.

```indonesiaku
fungsi sapa(nama: teks, ulang: bilangan = 1) -> teks {
    kembalikan nama * ulang
}

variabel pesan: teks = sapa("halo ", 2)
cetak(pesan)
```

Keluaran:

```text
halo halo 
```

Bentuknya: `nama: tipe` untuk parameter dan variabel, `-> tipe` untuk nilai
kembali. Nama tipe hanyalah sebuah nama — `bilangan`, `teks`, `daftar`,
`kamus`, `logika`, `kosong`, atau nama kelas buatan sendiri.

Sebagian parameter boleh beranotasi, sebagian tidak:

```indonesiaku
fungsi jumlahkan(a, b: bilangan) -> bilangan {
    kembalikan a + b
}

cetak(jumlahkan(3, 4))
```

Keluaran:

```text
7
```

Karena tidak diperiksa saat berjalan, argumen bertipe lain **tetap diterima**:

```indonesiaku
fungsi sapa(nama: teks) -> teks {
    kembalikan nama * 2
}

cetak(sapa(5))
```

Keluaran:

```text
10
```

Kalau Anda mengharapkan galat di situ, anotasi bukan alatnya. Memeriksa tipe
sebelum program dijalankan adalah tugas perkakas terpisah, bukan interpreter.

## Operator Assignment Gabungan

```indonesiaku
x = 10
x += 5    # x = x + 5  -> 15
x -= 3    # -> 12
x *= 2    # -> 24
x /= 4    # -> 6
x %= 4    # -> 2
x **= 3   # -> 8
x //= 3   # -> 2
x *= 2 + 3  # -> 10 (sisi kanan dihitung utuh dulu: x = x * (2 + 3))
```
> Contoh: [`examples/compound_operators.idk`](../examples/compound_operators.idk)

---

## List (Senarai)

```indonesiaku
angka = [10, 20, 30]
cetak(angka[0])          # 10 (pengindeksan)
cetak(angka[-1])         # 30 (indeks negatif: dihitung dari belakang)
angka[1] = 99            # ubah elemen
angka[-1] = 77           # elemen terakhir
tambah(angka, 40)        # tambah ke akhir
cetak(panjang(angka))    # 4
x = hapus(angka)         # hapus & kembalikan elemen terakhir
hapus_pada(angka, 0)     # hapus pada indeks
sisip(angka, 0, 5)       # sisipkan pada indeks

# List bisa bersarang & bertipe campuran
matriks = [[1, 2], [3, 4]]
cetak(matriks[0][1])     # 2
```
> Contoh: [`examples/list.idk`](../examples/list.idk)

{{embed:examples/list.idk}}

---

## Indeks Negatif dan Irisan

Indeks negatif menghitung dari belakang (`-1` = terakhir) dan berlaku untuk
daftar, teks, dan rentang.

```indonesiaku
a = [10, 20, 30, 40, 50]
cetak(a[-1])      # 50
cetak(a[-2])      # 40
cetak("halo"[-1]) # o
a[-1] = 99        # penugasan juga menerima indeks negatif
```

**Irisan** `a[awal:akhir:langkah]` mengambil sebagian urutan. Setiap bagian
boleh dikosongkan, dan aturannya mengikuti Python: batas di-*clamp*, irisan di
luar jangkauan menghasilkan hasil kosong (bukan galat).

```indonesiaku
a = [10, 20, 30, 40, 99]

cetak(a[1:3])     # [20, 30]      — dari 1 sampai sebelum 3
cetak(a[:2])      # [10, 20]      — dari awal
cetak(a[3:])      # [40, 99]      — sampai akhir
cetak(a[:])       # salinan penuh
cetak(a[::2])     # [10, 30, 99]  — setiap 2 elemen
cetak(a[::-1])    # [99, 40, 30, 20, 10] — dibalik
cetak(a[-3:-1])   # [30, 40]      — batas negatif
cetak(a[4:1])     # []            — kosong, bukan galat

cetak("halo dunia"[0:4])   # halo
cetak("halo"[::-1])        # olah
```

Catatan:

- Irisan daftar menghasilkan **daftar baru** (salinan dangkal), jadi `b = a[:]`
  aman untuk diubah tanpa menyentuh `a`.
- Irisan teks menghitung **karakter** (aman untuk UTF-8), bukan byte.
- `langkah` nol → `GalatNilai`. Mengiris rentang atau kamus → `GalatTipe`.
- Irisan hanya untuk dibaca: `a[1:3] = ...` ditolak saat kompilasi.

> Contoh: [`examples/kamus_irisan.idk`](../examples/kamus_irisan.idk)

---

## Tupel

Tupel adalah urutan yang **tidak bisa diubah** setelah dibuat. Ditulis dengan
kurung biasa, persis seperti Python:

```indonesiaku
titik = (3, 4)
satu = (5,)        # satu elemen: koma WAJIB
hampa = ()       # tupel kosong
bukan_tupel = (5)  # tanpa koma hanya pengelompokan -> 5

cetak(titik[0], titik[-1], panjang(titik))   # 3 4 2
x, y = titik                                  # dibuka ke dua nama
cetak((1, 2) + (3,))                          # (1, 2, 3)
```

Membaca tupel sama seperti membaca daftar: indeks, irisan (hasilnya tupel),
`untuk ... dalam`, `dalam`, `panjang`, `jumlah`, `terurut`, dan seterusnya.
Yang berbeda: tupel **tidak bisa diubah**.

```indonesiaku-takjalan
titik = (3, 4)
titik[0] = 10      # GalatTipe: Tupel tidak dapat diubah.
```

Karena tidak bisa diubah, tupel boleh menjadi **kunci kamus** — cocok untuk
koordinat atau pasangan nilai:

```indonesiaku
jarak = {}
jarak[(0, 0)] = "asal"
cetak(jarak[(0, 0)])   # asal
```

Ubah urutan lain menjadi tupel dengan `tupel(...)`: `tupel([1, 2])`,
`tupel("ab")` → `("a", "b")`.

### Tupel tanpa kurung

Pada **sisi kanan penugasan** dan setelah **`kembalikan`**, kurungnya boleh
dilepas — koma saja sudah membentuk tupel, seperti Python:

```indonesiaku
x = 1, 2
cetak(x, jenis(x))      # (1, 2) tupel

y = 1,                  # satu elemen, koma di akhir
cetak(y, panjang(y))    # (1,) 1
```

Ini membuat fungsi dapat mengembalikan beberapa nilai sekaligus:

```indonesiaku
fungsi bagi_sisa(a, b) {
    kembalikan a // b, a % b
}

cetak(bagi_sisa(17, 5))        # (3, 2)
bagi, sisa = bagi_sisa(17, 5)  # langsung dibuka ke dua nama
cetak(bagi, sisa)              # 3 2
```

Di dalam kurung apa pun, koma **tetap** berarti pemisah seperti biasa — jadi
argumen fungsi, elemen daftar, dan pasangan kamus tidak berubah artinya:

```indonesiaku
fungsi tambah2(a, b) { kembalikan a + b }
cetak(tambah2(1, 2))              # 3  <- dua argumen, bukan satu tupel
cetak([1, 2], {"a": 1, "b": 2})   # [1, 2] {"a": 1, "b": 2}
```

> Catatan: `variabel a, b = ...` tidak didukung; untuk membuka beberapa nama
> tulis `a, b = ...` tanpa `variabel`.

> Contoh: [`examples/tupel.idk`](../examples/tupel.idk),
> [`examples/sisa_fase2.idk`](../examples/sisa_fase2.idk)

---

## Himpunan

Himpunan adalah koleksi **anggota unik**: duplikat dibuang otomatis, dan
mengecek `x dalam s` itu cepat. Himpunan hanya dibuat lewat `himpunan(...)` —
tidak ada literal `{1, 2}` karena `{ }` sudah dipakai untuk blok dan kamus.

```indonesiaku
hampa = himpunan()
angka = himpunan([3, 1, 3, 2])
cetak(angka)               # himpunan([3, 1, 2])
cetak(himpunan("halo"))    # himpunan(["h", "a", "l", "o"])
cetak(2 dalam angka)       # benar
```

Anggota harus nilai yang **dapat di-hash** — aturan yang sama dengan kunci
kamus: angka, teks, logika, `kosong`, tupel, atau `himpunan_beku` berisi nilai
seperti itu. Daftar, kamus, dan himpunan biasa ditolak dengan `GalatTipe`.

Operasi himpunan menghasilkan himpunan **baru**; kedua operand harus himpunan:

```indonesiaku
a = himpunan([1, 2, 3])
b = himpunan([2, 3, 4])
cetak(a | b)   # gabungan         himpunan([1, 2, 3, 4])
cetak(a & b)   # irisan           himpunan([2, 3])
cetak(a - b)   # selisih          himpunan([1])
cetak(a ^ b)   # selisih simetris himpunan([1, 4])
```

Mengubah isi: `s.tambah(x)` (tidak bergalat bila sudah ada) dan `s.hapus(x)`
(`GalatKunci` bila `x` bukan anggota). Himpunan bisa dipakai di `untuk`,
comprehension, `panjang`, `jumlah`, `min`, `max`, `terurut`, dan `tupel(...)`.
`==` membandingkan anggota tanpa peduli urutan. Himpunan kosong bernilai salah.

Yang perlu diingat:

- Himpunan **tidak berindeks**: `s[0]` → `GalatTipe`. Ubah dulu ke daftar
  (`terurut(s)`) bila butuh posisi.
- Seperti `set` Python, himpunan biasa **tidak dapat di-hash**, jadi tidak
  bisa menjadi kunci kamus atau anggota himpunan lain — pakai `himpunan_beku`
  (di bawah) bila butuh itu.
- Urutan cetak mengikuti urutan penyisipan. Jangan bergantung pada urutan
  itu; pakai `terurut(s)` bila urutan penting.

### Himpunan beku

`himpunan_beku(...)` membuat himpunan yang **tidak bisa diubah**, dan karena
itu **dapat di-hash** — persis `frozenset` di Python:

```indonesiaku
beku = himpunan_beku([3, 1, 2])
cetak(beku)          # himpunan_beku([3, 1, 2])
cetak(jenis(beku))   # himpunan_beku

peta = {himpunan_beku([1, 2]): "pasangan"}
cetak(peta[himpunan_beku([2, 1])])   # pasangan  <- urutan tidak berpengaruh
```

Isi sama berarti sama, beku atau tidak. Operator `| & - ^` tetap bekerja;
hasilnya beku hanya bila **kedua** operand beku:

```indonesiaku
beku = himpunan_beku([1, 2])
cetak(himpunan([1, 2]) == beku)           # benar
cetak(jenis(beku | himpunan_beku([3])))   # himpunan_beku
cetak(jenis(beku | himpunan([3])))        # himpunan
```

`tambah` dan `hapus` menolaknya dengan `GalatTipe: Himpunan beku tidak dapat
diubah.`

> Contoh: [`examples/himpunan.idk`](../examples/himpunan.idk),
> [`examples/sisa_fase2.idk`](../examples/sisa_fase2.idk)

---

## Kamus (Dictionary)

Kamus dibuat dengan **literal `{ }`** atau dengan `kamus()`.

```indonesiaku
orang = {"nama": "Budi", "umur": 25}
kamus_kosong = {}

# Multi-baris, koma di akhir boleh
data = {
    "angka": [1, 2, 3],
    "dalam": {"inti": "ya"},
}
cetak(data["dalam"]["inti"])   # ya
```

> **Penting — `{` ambigu.** Indonesiaku memakai `{ }` sebagai blok (gaya PHP),
> jadi aturannya berdasarkan posisi: di **posisi pernyataan** `{` selalu
> membuka **blok**; di **posisi ekspresi** `{` selalu membuka **literal kamus**.
> Karena itu `{"a": 1}` yang berdiri sendiri sebagai satu pernyataan bukan
> kamus — tulis `x = {"a": 1}` atau `cetak({"a": 1})`.

Kunci boleh nilai apa pun yang **dapat di-hash**: teks, angka (`bulat` atau
`desimal`), `benar`/`salah`, dan `kosong`. Daftar, kamus, dan rentang tidak
bisa menjadi kunci (`GalatTipe`). Kunci duplikat: nilai terakhir yang menang,
posisi tetap yang pertama.

> ⚠️ `kamus[1]` sekarang berarti **kunci `1`**, bukan "kunci pertama". Untuk
> mengambil kunci menurut posisi pakai `kunci(kamus)[0]`. Ini perubahan yang
> merusak dari versi 0.4.

```indonesiaku
d = {1: "satu", 2.5: "dua koma lima", benar: "ya", kosong: "hampa"}
cetak(d[1], d[2.5], d[benar], d[kosong])   # satu dua koma lima ya hampa
cetak(d[1.0])                              # satu: 1 dan 1.0 kunci yang sama
cetak(kunci(d))                            # [1, 2.5, benar, kosong]
```

> Contoh: [`examples/kamus_kunci.idk`](../examples/kamus_kunci.idk)

```indonesiaku
orang = kamus()
orang["nama"] = "Budi"
orang["umur"] = 25
cetak(orang["nama"])         # Budi
cetak(punya(orang, "umur"))  # benar
hapus_kunci(orang, "umur")
untuk k dalam orang {             # iterasi kunci sesuai penyisipan
    cetak(k, "=", orang[k])
}
untuk pasangan dalam enumerasi(orang, 1) {
    cetak(pasangan[0], pasangan[1])
}
```
> Contoh: [`examples/kamus.idk`](../examples/kamus.idk)

---

## Perulangan untuk...dalam (for-in)

```indonesiaku
untuk buah dalam ["apel", "mangga", "jeruk"] {
    cetak(buah)
}

total = 0
untuk n dalam [1, 2, 3, 4, 5] {
    total = total + n
}
cetak("total:", total)   # 15

# Teks diiterasi per karakter
untuk huruf dalam "halo" {
    cetak(huruf)
}
```
> Contoh: [`examples/untuk_dalam.idk`](../examples/untuk_dalam.idk)

### Lingkup variabel `untuk`

Seperti Python, variabel `untuk` di-assign seperti `x = elemen`, bukan
variabel baru yang hanya hidup di dalam blok. Akibatnya, ia menimpa nama yang
sudah ada dan tetap bernilai elemen terakhir setelah loop selesai:

```indonesiaku
k = 99
untuk k dalam rentang(3) { }
cetak(k)                 # 2, bukan 99

untuk q dalam ["a", "b"] { }
cetak(q)                 # b
```

Di dalam fungsi, variabel `untuk` menjadi variabel lokal fungsi itu (K3),
kecuali dinyatakan `global`/`nonlokal`. Kalau urutannya kosong, variabel
tidak pernah di-assign. Di tingkat skrip membacanya menaikkan galat "tidak
terdefinisi", sedangkan di dalam fungsi nilainya `kosong`, sama seperti
lokal lain yang baris assignment-nya tidak pernah berjalan. Variabel
*comprehension* berbeda: ia tetap privat (lihat di bawah).

> Contoh: [`examples/untuk_lingkup.idk`](../examples/untuk_lingkup.idk)

### Perulangan angka dengan `rentang()`

`rentang()` menghasilkan deret bilangan bulat, seperti `range()` di Python:

```indonesiaku
untuk i dalam rentang(5) { cetak(i) }          # 0 1 2 3 4
untuk i dalam rentang(2, 6) { cetak(i) }       # 2 3 4 5
untuk i dalam rentang(0, 10, 3) { cetak(i) }   # 0 3 6 9
untuk i dalam rentang(5, 0, -1) { cetak(i) }   # 5 4 3 2 1

r = rentang(0, 10, 2)
cetak(r, panjang(r), r[1])                     # rentang(0, 10, 2) 5 2
```

`akhir` tidak termasuk. Rentang tidak membuat list, jadi `rentang(1000000)`
tetap hemat memori.

> Contoh: [`examples/rentang.idk`](../examples/rentang.idk)

### List comprehension

Cara ringkas membangun daftar baru dari urutan lain, seperti Python:

```indonesiaku
cetak([x * x untuk x dalam rentang(1, 6)])          # [1, 4, 9, 16, 25]
cetak([x untuk x dalam rentang(10) jika x % 2 == 0]) # [0, 2, 4, 6, 8]
cetak([k[0] untuk k dalam ["apel", "bunga"]])        # [a, b]
```

- Satu `untuk` dan satu `jika` opsional (gabungkan syarat dengan `dan`/`atau`).
  `untuk` atau `jika` kedua, atau token lain sebelum kurung penutup, adalah
  galat kompilasi.
- Variabel loop hanya hidup di dalam comprehension; nama di luar tidak berubah
  (beda dengan pernyataan `untuk`, seperti di Python 3).
- Boleh bersarang: `[[a * b untuk b dalam rentang(1, 3)] untuk a dalam [1, 2]]`.

> Contoh: [`examples/komprehensi.idk`](../examples/komprehensi.idk)

### Comprehension kamus dan himpunan

Bentuk yang sama di dalam `{ }` membangun kamus atau himpunan:

```indonesiaku
cetak({n: n * n untuk n dalam rentang(1, 5)})           # {1: 1, 2: 4, 3: 9, 4: 16}
cetak({n: "genap" untuk n dalam rentang(0, 6) jika n % 2 == 0})
cetak(panjang({x % 4 untuk x dalam rentang(0, 20)}))    # 4
```

- Ada `:` sebelum `untuk` → kamus; tanpa `:` → himpunan.
- `{}` tetap berarti kamus kosong.

### Generator expression

Di dalam `( )`, bentuk yang sama menghasilkan generator yang malas —
nilainya baru dihitung saat diminta:

```indonesiaku
variabel kuadrat = (x * x untuk x dalam rentang(1, 6))
cetak(jenis(kuadrat))                                   # generator
cetak(daftar(kuadrat))                                  # [1, 4, 9, 16, 25]

cetak(jumlah(x * 2 untuk x dalam rentang(1, 5)))        # 20
```

Sebagai argumen tunggal sebuah fungsi, kurungnya boleh ikut kurung panggilan.

> Contoh: [`examples/oop/comprehension_generator.idk`](../examples/oop/comprehension_generator.idk)

### Generator dengan `hasilkan`

Fungsi yang memakai `hasilkan` menjadi generator: tubuhnya berjalan sampai
`hasilkan` berikutnya, lalu berhenti sampai nilai berikutnya diminta.

```indonesiaku
fungsi hitung_mundur(mulai) {
    variabel n = mulai
    selagi n > 0 {
        hasilkan n
        n = n - 1
    }
}

untuk angka dalam hitung_mundur(3) {
    cetak(angka)                                        # 3, 2, 1
}
```

- Tidak ada daftar besar di memori; nilai dihitung satu per satu.
- Sumber tak terbatas aman selama dipotong dengan `hentikan`.
- `iter()` dan `lanjutkan()` juga bekerja pada generator.

> Contoh: [`examples/oop/generator.idk`](../examples/oop/generator.idk)

### Iterator sendiri dengan `_berikutnya_`

Objek menjadi iterator dengan `_iter_()` yang mengembalikan dirinya dan
`_berikutnya_()` yang menaikkan `HentiIterasi` saat habis:

```indonesiaku
kelas Penghitung {
    init() { diri.posisi = 0 }
    _iter_() { kembalikan diri }
    _berikutnya_() {
        jika diri.posisi >= 3 {
            naikkan HentiIterasi()
        }
        variabel n = diri.posisi
        diri.posisi = diri.posisi + 1
        kembalikan n
    }
}

cetak(daftar(Penghitung()))                             # [0, 1, 2]
```

> Contoh: [`examples/oop/iterator_malas.idk`](../examples/oop/iterator_malas.idk)

## Perkakas baris perintah

Selain menjalankan berkas, `indk` punya beberapa subperintah.

### `indk uji` — menjalankan tes

Setiap fungsi tingkat atas yang namanya diawali `uji_` dijalankan sebagai tes.
Asersinya memakai `pastikan` yang sudah ada:

```indonesiaku
fungsi tambah(a, b) { kembalikan a + b }

fungsi uji_tambah() {
    pastikan tambah(2, 3) == 5
    pastikan tambah(-1, 1) == 0, "pesan opsional"
}
```

```text
$ indk uji berkas.idk
  LULUS  uji_tambah

1/1 uji lulus
```

Satu `pastikan` yang gagal menandai tes itu gagal, tetapi tes lain tetap
berjalan. Exit code 0 bila semua lulus, 1 bila ada yang gagal. Menjalankan
berkas secara biasa (`indk berkas.idk`) **tidak** menjalankan fungsi `uji_*`.

> Contoh: [`examples/uji_contoh.idk`](../examples/uji_contoh.idk)

### `indk periksa` — linter

Melaporkan variabel lokal yang dideklarasikan dengan `variabel` tetapi tidak
pernah dibaca:

```text
$ indk periksa berkas.idk
berkas.idk:3  variabel lokal 'sementara' tidak pernah dipakai
1 temuan
```

Exit code 0 bila bersih, 1 bila ada temuan. Nama berawalan garis bawah
(`_sisa`) sengaja diabaikan — itu konvensi "memang tidak dipakai". Parameter
fungsi dan variabel dari `untuk`/`kecuali ... sebagai` tidak diperiksa.

### `indk rapikan` — merapikan format kode

```bash
$ indk rapikan berkas.idk            # cetak hasil ke layar
$ indk rapikan --tulis berkas.idk    # tulis balik ke berkas
$ indk rapikan --periksa berkas.idk  # exit 1 kalau belum rapi (untuk CI)
```

Formatter merapikan indentasi (4 spasi per tingkat) dan spasi di sekitar
operator, lalu mempertahankan komentar beserta posisinya — komentar akhir
baris tetap di baris yang sama dengan dua spasi sebelum `#`.

Ingat K4: **tidak ada aturan indentasi** di Indonesiaku. `rapikan` hanya
menata tampilan; kode yang ditulis rata kiri tetap sah dan tetap berjalan
tanpa dirapikan.

Formatter tidak memecah atau menyambung baris. Ia mengikuti baris yang kamu
tulis, jadi tata letak daftar multi-baris milikmu tidak diacak.

### `indk --dis` — melihat bytecode

Menampilkan bytecode hasil kompilasi tanpa menjalankan programnya, berguna
untuk memahami cara kerja VM:

```text
$ indk --dis berkas.idk
== <skrip utama> ==
0000    1 OP_CONSTANT         0 '1'
...
```

---

### Pencocokan pola: `cocokkan` dan `kasus`

Memilih satu cabang berdasarkan bentuk sebuah nilai. Kasus pertama yang
cocok menang; bila tidak ada yang cocok, tidak terjadi apa-apa.

```indonesiaku
fungsi namai(n) {
    cocokkan n {
        kasus 0 { kembalikan "nol" }
        kasus 1 atau 2 { kembalikan "sedikit" }
        kasus x jika x > 100 { kembalikan "banyak sekali" }
        kasus _ { kembalikan "biasa" }
    }
}
```

Pola yang didukung:

| Pola | Arti |
|------|------|
| `kasus 0`, `kasus "teks"`, `kasus kosong` | sama dengan nilai literal |
| `kasus _` | cocok apa pun; harus kasus terakhir |
| `kasus nama` | selalu cocok, nilainya diikat ke `nama` |
| `kasus Kelas()` | cocok bila objek merupakan instance kelas itu |
| `kasus a atau b` | salah satu alternatif |
| `kasus pola jika syarat` | penjaga tambahan |

Nama yang diikat hanya hidup di dalam kasusnya sendiri.

> Contoh: [`examples/oop/cocokkan.idk`](../examples/oop/cocokkan.idk)

### Penugasan berganda (unpacking)

Beberapa nama sekaligus dari satu urutan; jumlahnya harus sama persis:

```indonesiaku
a, b = 1, 2            # dari daftar nilai
a, b = b, a            # tukar tanpa variabel sementara
x, y, z = [10, 20, 30]
p, q = "hi"            # teks dipecah per karakter: h i
m, n = rentang(2, 4)   # 2 3
```

Jumlah nilai yang tidak cocok menimbulkan `GalatNilai` yang bisa ditangkap.

> Contoh: [`examples/penugasan_berganda.idk`](../examples/penugasan_berganda.idk)

### Menghapus dengan `buang`

`buang` (padanan `del` Python) menghapus variabel, elemen daftar, kunci kamus,
atau atribut objek:

```indonesiaku-takjalan
angka = [10, 20, 30]
buang angka[0]        # [20, 30]

kamus = {"a": 1, "b": 2}
buang kamus["a"]      # {"b": 2}

x = 5
buang x               # nama 'x' hilang; membacanya lagi = GalatNama

kelas Kotak {
    fungsi init(diri, isi) { diri.isi = isi }
}

k = Kotak(7)
buang k.isi           # hapus atribut
```

Menghapus indeks/kunci/atribut yang tidak ada menimbulkan `GalatIndeks`,
`GalatKunci`, atau `GalatAtribut` — semuanya bisa ditangkap `coba/kecuali`.

> Contoh: [`examples/buang.idk`](../examples/buang.idk)

---

## hentikan & lanjut (break & continue)

```indonesiaku
untuk x dalam [1, 2, 3, 4, 5, 6] {
    jika x == 4 { hentikan }   # keluar dari loop
    jika x == 2 { lanjut }     # lompat ke iterasi berikut
    cetak(x)
}
# Output: 1, 3
```
> Contoh: [`examples/hentikan_lanjut.idk`](../examples/hentikan_lanjut.idk)

---

## Pengindeksan String

String dapat diakses per karakter (baca-saja; string bersifat immutable).

```indonesiaku
s = "halo"
cetak(s[0])           # h
cetak(panjang(s))     # 4
untuk i dalam [0, 1, 2, 3] {
    cetak(s[i])
}
```
> Contoh: [`examples/indeks_string.idk`](../examples/indeks_string.idk)

---

## Closure

Gunakan kata kunci `variabel` untuk membuat variabel lokal fungsi yang dapat
ditangkap oleh closure (fungsi bersarang). Untuk **mengubah** variabel fungsi
pembungkus dari dalam closure, deklarasikan `nonlokal` lebih dulu — tanpa itu
assignment hanya membuat variabel lokal baru di closure.

```indonesiaku
fungsi buat_pencacah() {
    variabel hitung = 0
    fungsi naik() {
        nonlokal hitung
        hitung = hitung + 1
        kembalikan hitung
    }
    kembalikan naik
}

c = buat_pencacah()
cetak(c())   # 1
cetak(c())   # 2
```
> Contoh: [`examples/closure.idk`](../examples/closure.idk)

---

## Lingkup Assignment: `global` dan `nonlokal`

Di tingkat skrip, `x = 5` membuat/mengubah variabel global. **Di dalam fungsi,
`x = 5` membuat variabel lokal fungsi** — seperti Python. Nama yang hanya
dibaca (tidak di-assign) tetap dicari ke fungsi pembungkus lalu global.

```indonesiaku
variabel pesan = "skrip"

fungsi lokal_saja() {
    pesan = "lokal"       # lokal fungsi; global 'pesan' tidak berubah
    kembalikan pesan
}
cetak(lokal_saja(), pesan)   # lokal skrip
```

Untuk menulis ke variabel di luar fungsi, deklarasikan lingkupnya:

| Deklarasi | Efek |
|---|---|
| `global nama` | assignment menulis ke variabel tingkat skrip |
| `nonlokal nama` | assignment menulis ke variabel fungsi pembungkus |

```indonesiaku
variabel pesan = "skrip"

fungsi ubah_global() {
    global pesan
    pesan = "diubah"
}
ubah_global()
cetak(pesan)          # diubah
```

Keduanya menerima beberapa nama sekaligus (`global a, b`), berlaku untuk
seluruh badan fungsi, dan hanya boleh muncul di dalam fungsi. `nonlokal` harus
menemukan variabel pada fungsi pembungkus; merujuk global adalah galat kompilasi.

Variabel yang dibuat lewat assignment di dalam blok (`jika`, `selagi`, `untuk`)
hidup sampai akhir fungsi, bukan hanya sampai akhir blok:

```indonesiaku
fungsi jumlahkan(batas) {
    total = 0
    untuk n dalam rentang(1, batas) {
        total = total + n
    }
    kembalikan total      # 10 untuk batas 5
}
```

`variabel x = ...` tetap sah dan merujuk variabel lokal yang sama.

> Contoh: [`examples/lingkup_assignment.idk`](../examples/lingkup_assignment.idk)

---

## Kelas & OOP

```indonesiaku
kelas Hewan {
    init(nama, suara) {
        diri.nama = nama
        diri.suara = suara
    }
    bersuara() {
        cetak(diri.nama, "berkata:", diri.suara)
    }
}

kelas Anjing < Hewan {          # pewarisan dengan '<'
    init(nama) {
        super.init(nama, "Guk")  # panggil konstruktor induk
    }
}

a = Anjing("Rex")
a.bersuara()   # Rex berkata: Guk
```
> Contoh: [`examples/oop/kelas.idk`](../examples/oop/kelas.idk)

{{embed:examples/oop/kelas.idk}}

### Metode ajaib

Metode yang namanya diapit garis bawah, seperti `_teks_` atau `_tambah_`,
dipanggil **otomatis** oleh interpreter. Dengan metode ini objek buatan
sendiri bisa dicetak, dibandingkan, dan dijumlahkan seperti angka atau teks:

```indonesiaku
kelas Vektor {
    init(x, y) {
        diri.x = x
        diri.y = y
    }
    _teks_() {
        kembalikan f"({diri.x}, {diri.y})"
    }
    _repr_() {
        kembalikan f"Vektor({diri.x}, {diri.y})"
    }
    _sama_(lain) {
        kembalikan diri.x == lain.x dan diri.y == lain.y
    }
    _tambah_(lain) {
        kembalikan Vektor(diri.x + lain.x, diri.y + lain.y)
    }
    _kali_(k) {
        kembalikan Vektor(diri.x * k, diri.y * k)
    }
    _negatif_() {
        kembalikan Vektor(-diri.x, -diri.y)
    }
}

a = Vektor(1, 2)
b = Vektor(3, 4)
cetak(a)                  # (1, 2)
cetak([a, b])             # [Vektor(1, 2), Vektor(3, 4)]
cetak(f"a = {a}")         # a = (1, 2)
cetak(a + b, a * 3, -a)   # (4, 6) (3, 6) (-1, -2)
cetak(a == Vektor(1, 2))  # benar
cetak(a != b)             # benar
a += b
cetak(a)                  # (4, 6)
```

| Metode | Dipanggil oleh |
|---|---|
| `_teks_()` | `cetak(x)`, `ke_teks(x)`, `f"{x}"` |
| `_repr_()` | `repr(x)`, dan saat `x` ada di dalam daftar/tupel/kamus yang dicetak |
| `_sama_(lain)` / `_tidak_sama_(lain)` | `==` / `!=`, `dalam`, dan `==` antar-daftar |
| `_kurang_dari_` `_kurang_sama_` `_lebih_dari_` `_lebih_sama_` | `<` `<=` `>` `>=` |
| `_tambah_` `_kurangi_` `_kali_` `_bagi_` `_bagi_bulat_` `_sisa_` `_pangkat_` | `+` `-` `*` `/` `//` `%` `**` (juga `+=`, `-=`, ...) |
| `_negatif_()` | `-x` |
| `_panjang_()` | `panjang(x)`, dan kebenaran `x` bila tak ada `_logika_` |
| `_logika_()` | `jika x`, `selagi x`, `tidak x`, `dan`/`atau`, `semua()`, `ada_yang()` |
| `_ambil_(kunci)` / `_atur_(kunci, nilai)` | `x[kunci]` / `x[kunci] = nilai` |
| `_hash_()` | `x` dipakai sebagai kunci kamus atau anggota himpunan |
| `_ktambah_` `_kkurangi_` `_kkali_` `_kbagi_` `_kbagi_bulat_` `_ksisa_` `_kpangkat_` | operator yang sama, tapi saat objek ada di **kanan**: `nilai + x` |

Yang perlu diingat:

- `_teks_` dan `_repr_` **harus mengembalikan teks**. Kalau hanya ada
  `_repr_`, `cetak` juga memakainya. Tanpa keduanya objek dicetak
  `<objek NamaKelas>`.
- Perbandingan boleh dibalik: kalau hanya `_kurang_dari_` yang ada, `b > a`
  tetap jalan karena dijawab `a._kurang_dari_(b)`. `!=` tanpa `_tidak_sama_`
  memakai kebalikan `_sama_`.
- Aritmetika dibalik lewat metode berawalan `k`: `10 + objek` memanggil
  `objek._ktambah_(10)`. Lihat bagian *Cerminan aritmetika* di bawah.
- `adalah` **tidak pernah** memanggil `_sama_`; ia selalu menguji apakah dua
  nilai adalah objek yang sama.
- Objek yang punya `_sama_` **tanpa** `_hash_` tidak bisa menjadi kunci kamus
  atau anggota himpunan (`GalatTipe`), sama seperti Python menyetel
  `__hash__ = None`. Tambahkan `_hash_` untuk mendapatkannya kembali.
- Operator pada objek yang tidak punya metodenya menghasilkan `GalatTipe`:

```indonesiaku
kelas Kotak {
}
coba {
    cetak(Kotak() + 1)
} kecuali GalatTipe sebagai e {
    cetak(e.pesan)   # Operator '+' tidak didukung antara 'Kotak' dan 'bulat'.
}
```

> Contoh lengkap: [`examples/oop/metode_ajaib.idk`](../examples/oop/metode_ajaib.idk)

### Protokol objek: panjang, kebenaran, indeks, dan hash

Empat metode ajaib membuat kelas buatan berperilaku seperti tipe bawaan.

```indonesiaku
kelas Keranjang {
    init(isi) { diri.isi = isi }

    _panjang_()          { kembalikan panjang(diri.isi) }
    _ambil_(i)           { kembalikan diri.isi[i] }
    _atur_(i, nilai)     { diri.isi[i] = nilai }
}

k = Keranjang(["apel", "jeruk"])
cetak(panjang(k))        # 2
cetak(k[0])              # apel
k[0] = "mangga"
cetak(k[0])              # mangga

jika Keranjang([]) { cetak("tidak tercetak") } selain { cetak("keranjang kosong itu salah") }
```

**Kebenaran.** `_logika_` dipakai lebih dulu; tanpa itu, objek dianggap benar
bila `_panjang_()` bukan nol. Objek yang tidak punya keduanya **selalu benar**
— perilaku lama, jadi program yang sudah ada tidak berubah.

```indonesiaku
kelas Saklar {
    init(nyala) { diri.nyala = nyala }
    _logika_() { kembalikan diri.nyala }
}
jika tidak Saklar(salah) { cetak("saklar mati") }
```

**Kunci kamus.** `_hash_` wajib mengembalikan `bulat`. Pasangkan dengan
`_sama_`: dua objek yang `_sama_`-nya benar **wajib** punya `_hash_` yang sama,
persis kontrak Python. Builtin `hash()` membantu menyusunnya.

```indonesiaku
kelas Titik {
    init(x, y) {
        diri.x = x
        diri.y = y
    }
    _sama_(lain) { kembalikan diri.x == lain.x dan diri.y == lain.y }
    _hash_()     { kembalikan hash((diri.x, diri.y)) }
}

peta = {Titik(0, 0): "asal"}
cetak(peta[Titik(0, 0)])     # asal — objek berbeda, kunci sama
```

Yang perlu diingat:

- `_panjang_` harus mengembalikan `bulat` yang tidak negatif; selain itu
  `GalatTipe`/`GalatNilai`.
- Objek tanpa `_ambil_` tetap `GalatTipe` saat diindeks, dengan pesan yang
  menyebut metode yang perlu ditambahkan.
- Mengubah atribut yang dipakai `_hash_` setelah objek menjadi kunci membuat
  kuncinya tidak ditemukan lagi — sama seperti Python, dan tidak dicegah.

> Contoh lengkap: [`examples/oop/protokol_objek.idk`](../examples/oop/protokol_objek.idk)

### Cerminan aritmetika: objek di sebelah kanan

`_tambah_` hanya dicari pada operand **kiri**. Supaya `10 + objek` juga jalan,
tambahkan metode berawalan `k` (kanan):

```indonesiaku
kelas Rupiah {
    init(jumlah) { diri.jumlah = jumlah }

    _tambah_(lain)  { kembalikan Rupiah(diri.jumlah + lain) }
    _ktambah_(lain) { kembalikan Rupiah(lain + diri.jumlah) }

    _kurangi_(lain)  { kembalikan Rupiah(diri.jumlah - lain) }
    _kkurangi_(lain) { kembalikan Rupiah(lain - diri.jumlah) }

    _teks_() { kembalikan f"Rp{diri.jumlah}" }
}

cetak(Rupiah(5000) + 2500)   # Rp7500
cetak(2500 + Rupiah(5000))   # Rp7500 — lewat _ktambah_
```

**Arah operand itu penting.** Metode cerminan menerima operand kiri sebagai
argumen, sedangkan `diri` tetap objek di kanan. Untuk operator yang tidak
komutatif, tubuh metodenya harus menulis `lain` lebih dulu:

```indonesiaku
kelas Rupiah {
    init(jumlah) { diri.jumlah = jumlah }
    _kurangi_(lain)  { kembalikan Rupiah(diri.jumlah - lain) }
    _kkurangi_(lain) { kembalikan Rupiah(lain - diri.jumlah) }
    _teks_() { kembalikan f"Rp{diri.jumlah}" }
}

cetak(Rupiah(3) - 10)   # Rp-7  → Rupiah(3)._kurangi_(10)
cetak(10 - Rupiah(3))   # Rp7   → Rupiah(3)._kkurangi_(10)
```

Urutan pencariannya: metode langsung di operand kiri dulu, baru cerminan di
operand kanan. Kalau kedua operand objek dan yang kiri punya metodenya, yang
kiri yang dipakai. Operator gabungan (`+=`, `*=`, ...) ikut otomatis.

| Metode | Dipanggil oleh |
|---|---|
| `_ktambah_` | `nilai + objek` |
| `_kkurangi_` | `nilai - objek` |
| `_kkali_` | `nilai * objek` |
| `_kbagi_` | `nilai / objek` |
| `_kbagi_bulat_` | `nilai // objek` |
| `_ksisa_` | `nilai % objek` |
| `_kpangkat_` | `nilai ** objek` |

Objek di kanan tanpa metode cerminan tetap `GalatTipe` seperti sebelumnya.

> Contoh lengkap: [`examples/oop/cerminan_aritmetika.idk`](../examples/oop/cerminan_aritmetika.idk)

### Mengurutkan objek: `terurut`, `min`, `max`

Begitu kelas punya `_kurang_dari_`, ketiga builtin ini langsung menerimanya —
tidak perlu metode tambahan:

```indonesiaku
kelas Siswa {
    init(nama, nilai) {
        diri.nama = nama
        diri.nilai = nilai
    }
    _kurang_dari_(lain) { kembalikan diri.nilai < lain.nilai }
    _teks_() { kembalikan f"{diri.nama}({diri.nilai})" }
}

siswa = [Siswa("Budi", 80), Siswa("Ani", 92), Siswa("Citra", 75)]

untuk s dalam terurut(siswa) {
    cetak(s)                  # Citra(75), Budi(80), Ani(92)
}
cetak(min(siswa))             # Citra(75)
cetak(max(siswa))             # Ani(92)
cetak(terurut(siswa, benar)[0])   # Ani(92) — argumen menurun tetap berlaku
```

Yang perlu diingat:

- Cukup `_kurang_dari_`. Kalau kelasnya hanya punya `_lebih_dari_`, itu juga
  dipakai lewat cerminan.
- Pengurutan objek **stabil**: elemen yang nilainya sama mempertahankan urutan
  masukan, baik naik maupun menurun.
- `terurut()` selalu mengembalikan daftar baru; daftar asli tidak berubah.
- Mencampur objek dengan angka atau teks dalam satu daftar tetap `GalatTipe`.
- Galat yang dilempar dari dalam `_kurang_dari_` merambat keluar — hasil
  separuh-urut tidak pernah dikembalikan.

> Contoh lengkap: [`examples/oop/urut_objek.idk`](../examples/oop/urut_objek.idk)

### Objek yang bisa di-`untuk`-kan dan dipanggil: `_iter_`, `_panggil_`

`_iter_` membuat kelas sendiri bisa dipakai langsung di `untuk ... dalam`.
Metode ini dipanggil **sekali** di awal loop, dan yang diiterasi adalah nilai
kembaliannya — jadi cukup kembalikan koleksi yang sudah ada:

```indonesiaku
kelas Keranjang {
    init() { diri.isi = ["apel", "jeruk"] }
    _iter_() { kembalikan diri.isi }
}

untuk buah dalam Keranjang() {
    cetak(buah)                   # apel, jeruk
}
```

Kembaliannya boleh daftar, tupel, teks, rentang, himpunan, atau kamus.
`hentikan` dan `lanjut` tetap bekerja seperti biasa.

`_panggil_` membuat objek bisa dipanggil seperti fungsi:

```indonesiaku
kelas Pengali {
    init(n) { diri.n = n }
    _panggil_(x) { kembalikan x * diri.n }
}

kali2 = Pengali(2)
cetak(kali2(21))                  # 42
cetak(peta(Pengali(3), [1, 2, 3]))   # [3, 6, 9]
```

Nilai bawaan dan argumen bernama ikut berlaku, karena `_panggil_` dipanggil
lewat jalur yang sama dengan metode biasa.

> Catatan: `_iter_` di sini adalah bentuk sederhana — ia mengembalikan koleksi
> yang sudah jadi, bukan iterator malas. Protokol iterator penuh
> (`_berikutnya_` dan `hasilkan`) dijadwalkan di Fase 5.

> Contoh lengkap: [`examples/oop/iter_panggil.idk`](../examples/oop/iter_panggil.idk)

### Atribut kelas

Nilai yang sama untuk semua instance ditulis langsung di badan kelas, sejajar
dengan metode:

```indonesiaku
kelas Lingkaran {
    PHI = 3.14159

    init(jari) { diri.jari = jari }
    luas() { kembalikan Lingkaran.PHI * diri.jari * diri.jari }
}

cetak(Lingkaran.PHI)        # 3.14159 — lewat kelas
cetak(Lingkaran(2).PHI)     # 3.14159 — lewat instance
cetak(Lingkaran(2).luas())  # 12.56636
```

Aturannya mengikuti Python:

- Urutan pencarian pada objek: **field instance → atribut kelas → metode**.
- `obj.PHI = 3` selalu membuat field milik objek itu saja dan menutupi atribut
  kelas; objek lain tidak terpengaruh. Untuk mengubah nilai bersama, tulis
  lewat kelasnya: `Lingkaran.PHI = 3`.
- Subkelas mewarisi atribut induknya. Menulis di subkelas membuat miliknya
  sendiri tanpa menyentuh induk, dan mengubah atribut induk langsung terlihat
  oleh subkelas yang belum menimpanya.
- Ekspresinya dijalankan **sekali**, saat kelas didefinisikan — bukan setiap
  objek dibuat.
- Atribut dan metode berbagi satu ruang nama: yang ditulis belakangan menang.

> Contoh lengkap: [`examples/oop/atribut_kelas.idk`](../examples/oop/atribut_kelas.idk)

### `dengan ... sebagai` — manajer konteks

Untuk sumber daya yang harus dilepas (berkas, koneksi, kunci), `dengan`
menjamin pembersihan berjalan tanpa perlu `coba ... akhirnya` ditulis tangan:

```indonesiaku
kelas Berkas {
    init(nama) { diri.nama = nama }
    _masuk_() { kembalikan diri }
    _keluar_() { cetak("tutup", diri.nama) }
    baca() { kembalikan f"isi {diri.nama}" }
}

dengan Berkas("catatan.txt") sebagai f {
    cetak(f.baca())
}
# tutup catatan.txt
```

Kelasnya perlu dua metode: `_masuk_()` dijalankan sebelum blok, `_keluar_()`
sesudahnya.

- **`_keluar_()` dijamin dipanggil** — baik blok selesai normal, melempar
  galat, maupun keluar lewat `kembalikan`, `hentikan`, atau `lanjut`.
- Galat di dalam blok tetap merambat setelah `_keluar_()` dijalankan.
- Yang diikat `sebagai` adalah **nilai kembalian `_masuk_()`**, bukan objek
  manajernya.
- `sebagai <nama>` boleh dihilangkan jika nilainya tidak dipakai.
- Blok `dengan` yang bersarang ditutup dari yang terdalam.

> Catatan: `_keluar_()` dipanggil tanpa argumen dan nilai kembaliannya
> diabaikan. Berbeda dari Python, sebuah manajer di sini **tidak bisa menelan
> galat** — pembersihan tetap jalan, galatnya tetap keluar.

> Contoh lengkap: [`examples/oop/dengan_sebagai.idk`](../examples/oop/dengan_sebagai.idk)

### Dekorator

`@nama` di baris sebelum definisi fungsi berarti: definisikan fungsinya, lalu
ganti nilainya dengan `nama(fungsi_itu)`.

```indonesiaku
fungsi bungkus(f) {
    fungsi isi(x) {
        cetak("sebelum")
        hasil = f(x)
        cetak("sesudah")
        kembalikan hasil
    }
    kembalikan isi
}

@bungkus
fungsi kali2(x) { kembalikan x * 2 }

cetak(kali2(21))
# sebelum
# sesudah
# 42
```

Boleh bertumpuk — yang paling dekat dengan definisi dijalankan lebih dulu,
jadi `@a` di atas `@b` berarti `a(b(f))`:

```indonesiaku
fungsi a(f) { fungsi w(x) { kembalikan "a(" + f(x) + ")" } kembalikan w }
fungsi b(f) { fungsi w(x) { kembalikan "b(" + f(x) + ")" } kembalikan w }

@a
@b
fungsi dasar(x) { kembalikan x }

cetak(dasar("nilai"))   # a(b(nilai))
```

Yang ditulis sesudah `@` adalah ekspresi biasa, jadi dekorator berargumen juga
sah — cukup kembalikan dekoratornya:

```indonesiaku
fungsi ulangi(n) {
    fungsi deko(f) {
        fungsi w() {
            untuk i dalam rentang(0, n) { f() }
        }
        kembalikan w
    }
    kembalikan deko
}

@ulangi(3)
fungsi sapa() { cetak("halo") }

sapa()   # halo, halo, halo
```

Dekorator dijalankan **sekali**, saat definisinya dibuat.

> **Belum didukung: dekorator kustom pada metode di dalam kelas.** Di
> Indonesiaku `diri` bersifat implisit, sehingga fungsi pembungkus yang
> dikembalikan dekorator tidak membawa penerima — `f()` di dalamnya akan
> kehilangan `diri`. Compiler menolaknya dengan pesan yang jelas, bukan
> membiarkannya gagal jauh di dalam metode.

> Contoh lengkap: [`examples/dekorator.idk`](../examples/dekorator.idk)

### `@properti`, `@metode_statis`, `@metode_kelas`

Tiga dekorator bawaan untuk metode di dalam kelas. Ketiganya mengubah cara
metode **dipanggil**, jadi dikenali saat metode didefinisikan — bukan
pembungkus biasa.

```indonesiaku
kelas Lingkaran {
    PHI = 3.14159

    init(jari) { diri.jari = jari }

    @properti
    luas() { kembalikan Lingkaran.PHI * diri.jari * diri.jari }

    @metode_statis
    satuan() { kembalikan "cm" }

    @metode_kelas
    baku() { kembalikan kelas_ini(1) }
}

l = Lingkaran(2)
cetak(l.luas)              # 12.56636 — tanpa tanda kurung
cetak(Lingkaran.satuan())  # cm
cetak(l.satuan())          # cm — lewat objek juga boleh
cetak(Lingkaran.baku().jari)   # 1
```

**`@properti`** — metode tanpa argumen yang dibaca tanpa tanda kurung.
Dijalankan setiap kali dibaca.

- Harus dibaca lewat objek; `Lingkaran.luas` ditolak karena tidak ada objek
  untuk dihitung.
- Hanya untuk baca. `l.luas = 5` membuat field instance biasa yang menimpanya
  — tidak ada penyetel seperti `@luas.setter` di Python.
- Galat dari dalamnya merambat seperti biasa.

**`@metode_statis`** — tidak menerima objek sama sekali, jadi `diri` tidak
tersedia di dalamnya. Boleh dipanggil lewat kelas maupun objek.

**`@metode_kelas`** — menerima kelasnya, bernama **`kelas_ini`** (bukan
`diri`). Dipanggil dari subkelas, yang diterima adalah subkelasnya — jadi
`kelas_ini(...)` membuat objek dari kelas yang tepat.

Ketiganya diwariskan. Subkelas boleh menimpanya dengan `@properti` lagi atau
menjadikannya metode biasa.

> Dekorator **kustom** pada metode belum didukung — lihat catatan di bagian
> sebelumnya.

> Contoh lengkap: [`examples/oop/properti_statis.idk`](../examples/oop/properti_statis.idk)

### Pewarisan berganda

Induk boleh lebih dari satu, dipisah koma. Yang kiri lebih diutamakan:

```indonesiaku
kelas BisaDicetak { tampil() { kembalikan "cetak:" + diri.ringkas() } }
kelas BisaDisimpan { simpan() { kembalikan diri.ringkas() + ".dat" } }

kelas Dokumen < BisaDicetak, BisaDisimpan {
    init(nama) { diri.nama = nama }
    ringkas() { kembalikan diri.nama }
}

dok = Dokumen("laporan")
cetak(dok.tampil())    # cetak:laporan
cetak(dok.simpan())    # laporan.dat
```

Urutan pencarian metodenya dihitung dengan **linearisasi C3**, sama seperti
Python. Pada bentuk *diamond*, leluhur bersama muncul **sekali**, dan setelah
semua saudaranya:

```indonesiaku
kelas A { nama() { kembalikan "A" } }
kelas B < A { nama() { kembalikan "B" } }
kelas C < A { nama() { kembalikan "C" } }
kelas D < B, C { }

cetak(D().nama())      # B — urutannya D, B, C, A
```

Yang dijamin C3, dan sebabnya ia dipilih daripada telusur kiri-dalam biasa:

- Kalau `B` tidak punya metode itu tetapi `C` punya, yang dipakai **`C`** —
  bukan `A`. Saudara yang lebih dekat selalu menang atas leluhur bersama.
- Urutan induk yang ditulis tidak pernah terbalik di kelas turunannya.
- Kalau tidak ada urutan yang konsisten, **kelasnya ditolak** saat dibuat:

```indonesiaku-takjalan
kelas X < A, B { }
kelas Y < B, A { }
kelas Z < X, Y { }     # galat: urutan pewarisan tidak konsisten
```

Atribut kelas, `@properti`, `@metode_statis`, `@metode_kelas`, dan metode
ajaib ikut ditelusuri lewat MRO, jadi semuanya diwariskan dari induk mana pun.

> `super` tetap menunjuk **induk pertama**. Memanggil berantai sepanjang MRO
> seperti `super()` di Python belum ada.

> Contoh lengkap: [`examples/oop/pewarisan_berganda.idk`](../examples/oop/pewarisan_berganda.idk)

### Kelas abstrak

`@abstrak` menandai metode yang **wajib diisi subkelas**. Kelas yang masih
punya metode abstrak belum terisi tidak bisa dibuat objeknya:

```indonesiaku
kelas Bentuk {
    @abstrak
    luas() { }

    @abstrak
    keliling() { }

    # metode biasa boleh memakai yang abstrak
    ringkas() { kembalikan "luas " + ke_teks(diri.luas()) }
}

kelas Persegi < Bentuk {
    init(sisi) { diri.sisi = sisi }
    luas() { kembalikan diri.sisi * diri.sisi }
    keliling() { kembalikan 4 * diri.sisi }
}

cetak(Persegi(3).ringkas())   # luas 9
```

Membuat objek dari kelas yang belum lengkap ditolak, dengan menyebut metode
mana yang kurang:

```indonesiaku-takjalan
Bentuk()
# GalatTipe: Kelas 'Bentuk' abstrak: metode 'luas' belum diisi subkelas.
```

- Salahnya ketahuan **saat objeknya dibuat**, bukan nanti saat metodenya
  dipanggil.
- Subkelas yang mengisi sebagian **tetap abstrak** — sah ditulis, asal tidak
  dibuat objeknya; cucunya boleh melengkapi sisanya.
- Subkelas boleh menambah metode abstrak baru.
- Badan metode abstrak diabaikan; tulis saja `{ }`.

Dengan pewarisan berganda, satu induk boleh menuntut dan induk lain
mengisinya:

```indonesiaku
kelas Kontrak {
    @abstrak
    kerja() { }
}

kelas Pekerja { kerja() { kembalikan "dikerjakan" } }

kelas Gabung < Kontrak, Pekerja { }

cetak(Gabung().kerja())   # dikerjakan
```

> Satu metode hanya boleh punya **satu** dekorator bawaan — `@abstrak`
> bersama `@properti` ditolak compiler.

> Contoh lengkap: [`examples/oop/kelas_abstrak.idk`](../examples/oop/kelas_abstrak.idk)

---

## Metode pada Tipe Bawaan

Teks, daftar, dan kamus punya metode sendiri, jadi operasi bisa ditulis
berurutan dari kiri ke kanan:

```indonesiaku
cetak("budi".huruf_besar())        # BUDI
cetak("a,b,c".ganti(",", "-"))     # a-b-c
cetak([3, 1, 2].terurut())         # [1, 2, 3]
cetak({"a": 1}.punya("a"))         # benar
```

Keduanya setara — metode memanggil fungsi bawaan yang sama, jadi
`huruf_besar(x)` dan `x.huruf_besar()` selalu memberi hasil identik. Fungsi
bebas **tetap ada** dan tidak akan dihapus.

Metode bisa dirangkai:

```indonesiaku
cetak("a,b".ganti(",", "-").huruf_besar())   # A-B
```

Yang tersedia sekarang:

| Tipe | Metode |
|---|---|
| teks | `panjang`, `huruf_besar`, `huruf_kecil`, `ganti`, `potong`, `karakter`, `format`, `ord` |
| daftar | `panjang`, `tambah`, `hapus`, `hapus_pada`, `sisip`, `terurut`, `terbalik`, `jumlah`, `semua`, `ada_yang`, `enumerasi` |
| kamus | `panjang`, `punya`, `hapus_kunci`, `kunci`, `enumerasi` |

Metode yang mengubah isi tetap mengubah aslinya, sama seperti bentuk
fungsinya:

```indonesiaku
variabel isi = [1, 2]
isi.tambah(3)
cetak(isi)                         # [1, 2, 3]
```

Satu batasan: metode bawaan **harus langsung dipanggil**. Menyimpannya sebagai
nilai belum didukung dan akan ditolak dengan pesan yang menjelaskan hal itu:

```indonesiaku-takjalan
variabel f = "abc".huruf_besar     # ditolak — tulis "abc".huruf_besar()
```

Contoh lengkap: [`examples/metode_tipe.idk`](../examples/metode_tipe.idk)

{{embed:examples/metode_tipe.idk}}

---

## Cara Menulis Angka

Selain `10` dan `3.5`, angka boleh ditulis dengan basis lain atau pemisah:

```indonesiaku
cetak(1e3, 1.5e-3)          # eksponen -> 1000  0.0015
cetak(0xFF, 0b1010, 0o17)   # heksadesimal, biner, oktal -> 255  10  15
cetak(1_000_000)            # pemisah ribuan -> 1000000
```

Semuanya tetap bertipe angka biasa — hanya cara menulisnya yang berbeda,
jadi `0xFF == 255` bernilai `benar`.

Garis bawah hanya boleh berada **di antara** digit: `1_000` sah, sedangkan
`1__0` dan `1_` ditolak.

---

## String Tiga Kutip

Untuk teks yang membentang beberapa baris, tanpa menyambung `+ "\n" +`:

```indonesiaku
variabel puisi = """baris pertama
baris kedua"""
```

Baris baru disimpan apa adanya, dan kutip tunggal di dalamnya tidak perlu
di-escape.

---

## f-string (Teks Bersisip)

Awali teks dengan `f`, lalu sisipkan ekspresi di dalam `{ }`:

```indonesiaku
variabel nama = "Budi"
variabel umur = 30
cetak(f"Halo {nama}, umurmu {umur} tahun")
cetak(f"1 + 2 = {1 + 2}")
```

Isi `{ }` boleh ekspresi apa pun — pemanggilan fungsi, indeks, bahkan ekspresi
kondisional. Nilai non-teks ditampilkan sama seperti hasil `ke_teks()`.

Untuk menulis kurung kurawal harfiah, tulis dobel:

```indonesiaku
cetak(f"kurung {{harfiah}} ok")      # kurung {harfiah} ok
```

Satu hal yang perlu diperhatikan: kurawal pembuka yang langsung diikuti
kurawal lain dibaca sebagai kurawal harfiah. Jadi untuk menyisipkan kamus
literal, beri spasi — sama seperti di Python:

```indonesiaku
cetak(f"kamus: { {"a": 1} }")        # bukan f"kamus: {{"a": 1}}"
```

Belum ada format spec seperti `{x:.2f}`; itu menunggu metode pada tipe teks.

Contoh lengkap: [`examples/literal_fstring.idk`](../examples/literal_fstring.idk)

{{embed:examples/literal_fstring.idk}}

---

## Fungsi Anonim

Fungsi tanpa nama, ditulis persis seperti fungsi biasa tetapi langsung menjadi
nilai:

```indonesiaku
variabel kuadrat = fungsi(x) { kembalikan x * x }
cetak(kuadrat(6))                                  # 36

# Dioper langsung sebagai argumen.
fungsi terapkan(f, nilai) { kembalikan f(nilai) }
cetak(terapkan(fungsi(x) { kembalikan x * 10 }, 7))   # 70
```

Badannya tetap memakai `{ }` dan `kembalikan` — tidak ada bentuk satu-ekspresi
khusus seperti `lambda` di Python, supaya cara menulis fungsi hanya ada satu.

Fungsi anonim adalah closure penuh, jadi ia menangkap variabel di sekitarnya:

```indonesiaku
fungsi pembuat_penambah(n) {
    kembalikan fungsi(x) { kembalikan x + n }
}
variabel tambah5 = pembuat_penambah(5)
cetak(tambah5(1))                                  # 6
```

Nilai bawaan parameter dan argumen bernama berlaku sama seperti fungsi biasa.
Di pesan kesalahan namanya muncul sebagai `<anonim>`.

---

## Ekspresi Kondisional

Memilih satu dari dua nilai tanpa menulis blok `jika`:

```indonesiaku
variabel nilai = 75
variabel label = "lulus" jika nilai >= 60 selain "gagal"
```

Urutannya dibaca: *nilai-kalau-benar* `jika` *kondisi* `selain`
*nilai-kalau-salah* — sama seperti `a if c else b` di Python. Bagian `selain`
wajib ada.

**Kondisi dinilai lebih dulu, dan hanya cabang yang terpilih yang
dijalankan.** Karena itu pola penjaga berikut aman walaupun daftarnya kosong:

```indonesiaku
variabel x = [1, 2]
cetak(x[0] jika panjang(x) > 0 selain "kosong")
```

Jangan keliru dengan pernyataan `jika`, yang memakai blok dan tidak
menghasilkan nilai:

```indonesiaku
variabel nilai = 75

jika nilai >= 60 { cetak("lulus") } selain { cetak("gagal") }   # pernyataan
variabel l = "lulus" jika nilai >= 60 selain "gagal"            # ekspresi
```

Contoh lengkap: [`examples/fungsi_anonim.idk`](../examples/fungsi_anonim.idk)

{{embed:examples/fungsi_anonim.idk}}

---

## Penegasan (`pastikan`)

`pastikan` menyatakan sesuatu yang **harus** benar. Kalau benar, tidak terjadi
apa-apa; kalau salah, program menaikkan `GalatPenegasan`.

```indonesiaku
variabel umur = 20

pastikan umur >= 0
pastikan umur >= 0, "umur tidak boleh negatif"      # dengan pesan sendiri
```

Kebenaran kondisinya mengikuti aturan yang sama dengan `jika`: `0`, `""`,
daftar/kamus kosong, `kosong`, dan `salah` dianggap gagal.

Gunanya terutama menjaga prakondisi — menyatakan asumsi secara eksplisit
daripada membiarkan program salah diam-diam:

```indonesiaku
fungsi bagi(a, b) {
    pastikan b != 0, "pembagi tidak boleh nol"
    kembalikan a / b
}
```

Karena `GalatPenegasan` adalah turunan `Galat`, penegasan yang gagal bisa
ditangkap seperti kesalahan lain:

```indonesiaku
coba {
    pastikan salah, "ini gagal"
} kecuali GalatPenegasan sebagai e {
    cetak(e.pesan)          # ini gagal
}
```

Dua hal yang perlu diingat:

- **Pesan hanya dinilai kalau penegasannya gagal.** Jadi
  `pastikan benar, hitung_mahal()` tidak pernah memanggil `hitung_mahal()`.
- **Jangan tulis `pastikan(kondisi, pesan)` dengan kurung.** Kurung dengan
  koma membentuk satu tupel, dan tupel tidak kosong selalu benar. Python
  hanya memberi peringatan dan penegasannya tidak pernah gagal. Indonesiaku
  menolaknya saat kompilasi (`Kondisi 'pastikan' berupa tupel sehingga
  selalu benar`), jadi tulis tanpa kurung.
  `pastikan(kondisi)` dengan satu argumen sah, kurungnya hanya mengelompokkan
  ekspresi biasa.

Contoh lengkap: [`examples/pastikan.idk`](../examples/pastikan.idk)

{{embed:examples/pastikan.idk}}

---

## Teks di Dalam Koleksi

Teks yang dicetak langsung tampil apa adanya, tetapi teks **di dalam** daftar
atau kamus tampil dengan kutip — seperti Python:

```indonesiaku
cetak("a")              # a
cetak(["a"])            # ["a"]
cetak(["1", 1])         # ["1", 1]   <- teks dan angka terbedakan
cetak([""])             # [""]       <- bukan daftar kosong
```

Karakter khusus ikut di-escape, jadi isinya tidak menyesatkan:

```indonesiaku
cetak(["a\"b"])          # ["a\"b"]
cetak(["baris1\nbaris2"]) # ["baris1\nbaris2"]
```

Aturan yang sama berlaku untuk `ke_teks`, `format`, f-string, dan `repr`,
sehingga semuanya menghasilkan teks yang sama untuk nilai yang sama.

Kamus dicetak sesuai **urutan penambahan**, sama dengan `kunci()` dan
perulangan `untuk ... dalam`.

Contoh lengkap: [`examples/repr_koleksi.idk`](../examples/repr_koleksi.idk)

{{embed:examples/repr_koleksi.idk}}

---

## Membaca Pesan Kesalahan

Galat saat kompilasi menyebut baris **dan kolom**, lalu menunjuk bagian yang
bermasalah:

```text
[baris 2, kolom 11] Kesalahan di '+': Harapkan ekspresi.
    cetak(a + + 2)
              ^
```

Baris yang sangat panjang dipotong di sekitar titik galat dengan penanda `…`,
jadi tanda `^` tetap lurus di bawah token yang dimaksud.

Kalau nama yang dipakai mirip dengan sesuatu yang sudah ada, pesannya
menyertakan saran:

```
Variabel tidak terdefinisi 'cetk'. Mungkin maksudmu 'cetak'?
```

Saran hanya muncul bila ejaannya memang dekat; nama yang tidak mirip apa pun
tidak akan ditebak-tebak.

Galat saat program **berjalan** memakai format yang sama:

```
Operand '+' harus dua angka, dua teks, atau dua daftar.
[baris 2, kolom 11] dalam skrip
    cetak(x + "a")
              ^
```

Pada traceback berlapis, potongan baris hanya dicetak untuk tempat galat
benar-benar terjadi; bingkai pemanggil di bawahnya cukup baris dan kolom:

```text
Pembagian dengan nol.
[baris 2, kolom 20] dalam bagi()
        kembalikan a / b
                       ^
[baris 4, kolom 16] dalam skrip
```

Catatan: kolom menunjuk **akhir** ekspresi yang bermasalah (token terakhir
yang diproses), bukan tepat di operatornya.

---

## Penanganan Kesalahan (naikkan / coba / kecuali / akhirnya)

```indonesiaku
coba {
    naikkan "terjadi kesalahan"
} kecuali e {
    cetak("tertangkap:", e)
}

# Kesalahan fungsi bawaan juga dapat ditangkap:
coba {
    x = ke_angka("bukan angka")
} kecuali e {
    cetak("gagal konversi:", e)
}

# Semua kesalahan saat program berjalan bisa ditangkap. Nilai `e` adalah
# pesan kesalahannya:
coba {
    cetak(10 / 0)
} kecuali e {
    cetak(e)            # Pembagian dengan nol.
}
```

Blok `akhirnya` **selalu** dijalankan, baik terjadi kesalahan maupun tidak.
Cocok untuk menutup berkas atau soket. Tanpa `kecuali`, kesalahan diteruskan
ke luar setelah blok `akhirnya` selesai:

```indonesiaku-takjalan
f = buka_berkas("data.txt", "b")
coba {
    cetak(baca(f))
} kecuali e {
    cetak("gagal membaca:", e)
} akhirnya {
    tutup(f)
}
```

Blok `akhirnya` juga dijalankan saat `kembalikan`, `hentikan`, atau `lanjut`
keluar dari `coba`. Nilai yang dikembalikan dihitung lebih dulu:

```indonesiaku
fungsi baca_angka() {
    coba {
        kembalikan 42
    } akhirnya {
        cetak("selalu dijalankan")
    }
}
cetak(baca_angka())   # "selalu dijalankan", lalu 42
```

> Contoh: [`examples/coba_kecuali.idk`](../examples/coba_kecuali.idk),
> [`examples/coba_akhirnya.idk`](../examples/coba_akhirnya.idk),
> [`examples/galat_tertangkap.idk`](../examples/galat_tertangkap.idk),
> [`examples/coba_akhirnya_lompat.idk`](../examples/coba_akhirnya_lompat.idk)

### `lewati` — blok kosong

`lewati` adalah pernyataan yang tidak melakukan apa-apa (seperti `pass` di
Python). Berguna sebagai penanda tempat kode yang belum ditulis:

```indonesiaku
fungsi nanti_saja() { lewati }
kelas Kosong { lewati }
```

---

## Modul (impor)

Ada dua bentuk impor. Bentuk **bermodul** (`impor nama`) memberi tiap berkas
namespace sendiri; bentuk **berkas** (`impor "file.idk"`) menjalankan berkas
lain dalam namespace pengimpornya. Keduanya di-cache: modul bernama dijalankan
sekali per program, berkas dijalankan sekali per pengimpor.

### `impor nama` — tiap modul punya namespace sendiri

```indonesiaku
impor matematika

cetak(matematika.mat_fpb(12, 18))      # 6
```

Nama modul ditulis tanpa tanda kutip dan tanpa `.idk`. Berkas dicari
berurutan di: direktori berkas yang mengimpor, lalu setiap direktori di
variabel lingkungan `INDK_PATH`, lalu `pustaka/` di direktori kerja, lalu
**pustaka standar yang terpasang bersama interpreter** (`pustaka/` di sebelah
`indk`, `../pustaka`, atau `../share/indonesiaku/pustaka`). Karena itu modul
standar seperti `pola` atau `koleksi` bisa diimpor dari folder mana pun,
dan modul Anda sendiri yang bernama sama tetap didahulukan.

Modul yang diimpor **tidak** mencemari variabel Anda. Dua modul boleh memakai
nama yang sama tanpa saling menimpa:

```indonesiaku
impor matematika

variabel mat_fpb = "punya saya"
cetak(mat_fpb)                         # punya saya
cetak(matematika.mat_fpb(12, 18))      # 6
```

### Alias dengan `sebagai`

```indonesiaku
impor matematika sebagai mtk

cetak(mtk.mat_kpk(4, 6))               # 12
```

### `dari ... impor ...`

Mengambil anggota tertentu langsung ke namespace Anda:

```indonesiaku
dari matematika impor mat_fpb
cetak(mat_fpb(12, 18))                 # 6

dari matematika impor mat_fpb sebagai fpb
cetak(fpb(9, 6))                       # 3

dari matematika impor mat_fpb, mat_kpk
cetak(mat_fpb(8, 12), mat_kpk(4, 6))   # 4 12
```

### Paket (modul di dalam folder)

Titik berarti subfolder, jadi `impor alat.teks` memuat `alat/teks.idk`.
Nama yang didefinisikan adalah komponen terakhir (`teks`).

### `__nama__` dan `__utama__`

Setiap berkas punya variabel `__nama__`. Isinya `"__utama__"` bila berkas
dijalankan langsung, dan nama modulnya bila diimpor. Gunakan ini agar sebuah
berkas bisa jadi program sekaligus modul:

```indonesiaku
fungsi utama() {
    cetak("berjalan sebagai program")
}

jika __nama__ == "__utama__" {
    utama()
}
```

### Bentuk berkas: `impor "file.idk"`

Bentuk lama tetap berlaku: berkas dijalankan dalam namespace **pengimpornya**,
sehingga semua namanya langsung tersedia tanpa awalan. Namanya tidak bocor ke
modul lain. Kalau berkas yang diimpor mendefinisikan `panjang`, hanya
pengimpornya yang melihat `panjang` itu, dan `impor json` tetap memakai
builtin aslinya. Modul bernamespace yang mengimpor berkas yang sama
mendapat salinannya sendiri.

```indonesiaku
impor "pustaka/teks.idk"
impor "pustaka/daftar.idk"

cetak(teks_pisah("a,b,c", ","))        # ["a", "b", "c"]
cetak(daftar_urutkan([3, 1, 2]))       # [1, 2, 3]
```

Pilih `impor nama` untuk kode baru — namespace terpisah mencegah tabrakan
nama. Bentuk berkas berguna bila Anda memang ingin isinya menyatu.

> Contoh: [`examples/modul.idk`](../examples/modul.idk),
> [`examples/impor.idk`](../examples/impor.idk)
> Lihat daftar pustaka standar di [07_PUSTAKA_STANDAR.md](07_PUSTAKA_STANDAR.md).

---

## Mencetak (`cetak`)

`cetak` adalah **fungsi biasa**, bukan kata kunci. Argumen dipisah spasi dan
diakhiri baris baru:

```indonesiaku
cetak("satu")
cetak(1, 2, 3)
cetak()                      # baris kosong
```

Pemisah dan akhiran bisa diatur lewat argumen bernama:

```indonesiaku
cetak("a", "b", "c", pisah="-")        # a-b-c
cetak("tanpa newline", akhir="")
cetak("x", "y", pisah="", akhir="!\n") # xy!
```

Karena fungsi biasa, `cetak` juga sebuah nilai: bisa disimpan, dioper, dan
diperiksa.

```indonesiaku
variabel tulis = cetak
tulis("lewat variabel")

fungsi jalankan(fn) { fn("dioper") }
jalankan(cetak)

cetak(bisa_dipanggil(cetak))   # benar
```

Konsekuensinya sama seperti Python: namanya bisa ditimpa, dan sesudah ditimpa
ia bukan fungsi lagi.

```indonesiaku
variabel cetak = 5
# cetak("x")   -> Hanya dapat memanggil fungsi dan kelas.
```

Fungsi bawaan **lain** belum menerima argumen bernama; hanya `cetak` yang
menerimanya.

Contoh lengkap: [`examples/cetak_fungsi.idk`](../examples/cetak_fungsi.idk)

{{embed:examples/cetak_fungsi.idk}}

---

## Built-in Functions

> Referensi lengkap semua fungsi bawaan (matematika, string, list, kamus,
> konversi, berkas, waktu/tanggal, soket) ada di
> [07_PUSTAKA_STANDAR.md](07_PUSTAKA_STANDAR.md).

### cetak() - Print Output

Menampilkan output ke layar.

```indonesiaku
# Satu argument
cetak("Halo")

# Multiple arguments: dicetak dalam satu baris, dipisah spasi
cetak("Nama:", "Teguh")   # Nama: Teguh
cetak("Umur:", 25)        # Umur: 25

# Tanpa argument: baris kosong
cetak()

# Expressions
x = 10
y = 5
cetak("Jumlah:", x + y)

# Various types
cetak(42)           # Number
cetak(benar)        # Boolean
cetak(kosong)       # Null
```

### jam() — Waktu Saat Ini

Mendapatkan waktu saat ini (dalam detik sejak epoch).

```indonesiaku
waktu = jam()
cetak("Waktu saat ini:", waktu)
```

---

## Comments

Komentar digunakan untuk menjelaskan code dan tidak akan dieksekusi.

```indonesiaku
# Ini adalah single-line comment

# Variabel untuk menyimpan nama
nama = "Teguh"

# Fungsi untuk menghitung luas
fungsi hitung_luas(p, l) {
    # Rumus: panjang x lebar
    kembalikan p * l
}

# Menampilkan hasil
cetak("Luas:", hitung_luas(10, 5))
```

---

## Best Practices

### 1. Naming Conventions

```indonesiaku
# ✅ BAIK - Deskriptif dan jelas
nama_lengkap = "Teguh Rijanandi"
hitung_total_pendapatan = 0
adalah_aktif = benar

# ❌ BURUK - Tidak jelas
nlk = "Teguh Rijanandi"
x = 0
flag = benar
```

### 2. Indentasi Konsisten

```indonesiaku
# ✅ BAIK
jika benar {
    cetak("Level 1")
    jika benar {
        cetak("Level 2")
    }
}

# ❌ BURUK - Indentasi tidak konsisten
jika benar {
cetak("Level 1")
   jika benar {
        cetak("Level 2")
   }
}
```

### 3. Gunakan Comments untuk Logic Kompleks

```indonesiaku
harga = 50000

# ✅ BAIK
# Menghitung total dengan diskon 10%
total = harga * 0.9
cetak(total)

# ❌ BURUK - Comment tidak informatif
# Menambah variabel
total = harga * 0.9
```

### 4. Fungsi Kecil dan Fokus

```indonesiaku
# ✅ BAIK - Satu fungsi, satu tanggung jawab
fungsi hitung_luas_persegi(sisi) {
    kembalikan sisi * sisi
}

fungsi hitung_luas_lingkaran(radius) {
    kembalikan 3.14159 * radius * radius
}

# ❌ BURUK - Terlalu banyak tanggung jawab
fungsi hitung_semua(sisi, radius) {
    luas_persegi = sisi * sisi
    luas_lingkaran = 3.14159 * radius * radius
    cetak("Persegi:", luas_persegi)
    cetak("Lingkaran:", luas_lingkaran)
}
```

### 5. Deklarasikan Variabel di Awal

```indonesiaku
# ✅ BAIK
x = 0
y = 0
nama = ""

selagi x < 10 {
    y = x * 2
    nama = "data"
    x = x + 1
}

# ❌ KURANG BAIK - Deklarasi di tengah kode
selagi x < 10 {
    y = x * 2
    nama = "data"
    x = x + 1
}
```

---

## Contoh Program Lengkap

### Program 1: FizzBuzz

```indonesiaku
# FizzBuzz game
fungsi fizzbuzz(n) {
    i = 1
    selagi i <= n {
        jika i % 15 == 0 {
            cetak("FizzBuzz")
        }
        jika i % 3 == 0 dan i % 15 != 0 {
            cetak("Fizz")
        }
        jika i % 5 == 0 dan i % 15 != 0 {
            cetak("Buzz")
        }
        jika i % 3 != 0 dan i % 5 != 0 {
            cetak(i)
        }
        i = i + 1
    }
}

fizzbuzz(15)
```

### Program 2: Hitung Rata-Rata

```indonesiaku
fungsi hitung_rata_rata(a, b, c, d, e) {
    jumlah = a + b + c + d + e
    rata = jumlah / 5
    kembalikan rata
}

nilai1 = 80
nilai2 = 85
nilai3 = 90
nilai4 = 75
nilai5 = 88

rata_rata = hitung_rata_rata(nilai1, nilai2, nilai3, nilai4, nilai5)
cetak("Rata-rata nilai:", rata_rata)
```

### Program 3: Bilangan Prima

```indonesiaku
fungsi adalah_prima(n) {
    jika n <= 1 {
        kembalikan salah
    }
    
    i = 2
    selagi i * i <= n {
        jika n % i == 0 {
            kembalikan salah
        }
        i = i + 1
    }
    
    kembalikan benar
}

# Cari bilangan prima sampai 20
angka = 2
selagi angka <= 20 {
    jika adalah_prima(angka) {
        cetak(angka)
    }
    angka = angka + 1
}
```

---

## Ringkasan Keywords

| Kategori | Keywords |
|----------|----------|
| **Tipe/Literal** | `benar`, `salah`, `kosong` |
| **Percabangan** | `jika`, `jika_lain`, `selain` |
| **Perulangan** | `selagi`, `untuk`, `dalam`, `hentikan`, `lanjut` |
| **Fungsi** | `fungsi`, `kembalikan`, `variabel` |
| **Kelas/OOP** | `kelas`, `diri`, `super` |
| **Kesalahan** | `naikkan`, `coba`, `kecuali` |
| **Modul** | `impor` |
| **Operator logika** | `dan`, `atau`, `tidak` |

`cetak` **bukan** kata kunci — ia fungsi bawaan biasa (lihat seksi Mencetak).

---

## Tips & Trik
1. **Gunakan descriptive names** - Nama yang jelas mempermudah debugging
2. **Test di REPL** - Coba command secara interaktif terlebih dahulu (blok multi-baris didukung)
3. **Break kompleks logic** - Pisahkan menjadi fungsi-fungsi kecil
4. **Use functions** - Untuk menghindari code duplication
5. **Comment wisely** - Untuk logic yang kompleks, bukan yang obvious

---

**Dokumen ini memberikan fondasi untuk mulai menggunakan Bahasa Pemrograman Indonesiaku. Untuk pertanyaan lebih lanjut, silakan lihat contoh di folder `/examples` atau [07_PUSTAKA_STANDAR.md](07_PUSTAKA_STANDAR.md).**

Versi bahasa: v1.0.0
