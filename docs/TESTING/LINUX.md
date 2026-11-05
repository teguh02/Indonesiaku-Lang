# Rencana dan Hasil Uji — Linux

Rencana uji paket rilis Indonesiaku di **Linux**, beserta hasil uji
terakhirnya. Pasangannya: [`WINDOWS.md`](WINDOWS.md) dan
[`MACOS.md`](MACOS.md). Aturan memperbarui berkas ini ada di bagian 2 dan
di `AGENTS.md`.

| | |
|---|---|
| Versi terakhir diuji | ⬜ belum diuji |
| Tanggal | — |
| Paket | `indonesiaku_<versi>_amd64.deb` dan `indk-linux.tar.gz` |
| SHA-256 `indk` | — |
| Mesin | — (catat distro dan versinya, `uname -m`, versi glibc `ldd --version`) |
| Ringkasan | ⬜ belum diuji |

## 1. Persiapan

1. Unduh `indonesiaku_<versi>_amd64.deb`, `indk-linux.tar.gz`, dan
   `SHA256SUMS.txt` dari
   [GitHub Releases](https://github.com/teguh02/Indonesiaku-Lang/releases).
2. Cocokkan: `sha256sum -c SHA256SUMS.txt --ignore-missing`.
3. **Paket `.deb`** (Debian/Ubuntu): `sudo dpkg -i indonesiaku_<versi>_amd64.deb`.
   Hasil yang diharapkan: `/usr/bin/indk`, `/usr/share/indonesiaku/pustaka/`,
   `/usr/share/indonesiaku/examples/`, `/usr/share/doc/indonesiaku/`.
4. **Arsip `.tar.gz`** (distro lain): `tar -xzf indk-linux.tar.gz`; folder
   `indk-linux/` berisi `indk`, `pustaka/`, `examples/`. Uji juga dengan
   menjalankan `indk-linux/indk` langsung.
5. Pastikan `indk -v` mencetak versi rilis di terminal baru.
6. Buat folder kerja kosong di luar repo (mis. `/tmp/uji-indk`) untuk kasus
   bagian 3 dan 4. Folder itu **tidak boleh** memuat `pustaka/`.

<!-- BERSAMA:MULAI — bagian 2–4 harus identik di WINDOWS.md, MACOS.md, LINUX.md -->

## 2. Aturan rencana uji ini

Folder `docs/TESTING/` berisi tiga berkas: `WINDOWS.md`, `MACOS.md`, dan
`LINUX.md`. Ketiganya adalah **rencana sekaligus catatan hasil** uji
paket rilis di sistem operasi masing-masing: apa yang diuji, cara
mengujinya, apa hasil yang diharapkan, dan apa hasil terakhirnya.

Berbeda dengan CI (`.github/workflows/test.yml`) yang membangun dari kode
sumber, rencana ini menguji **biner yang benar-benar diterima pengguna**:
installer, `.zip`, `.tar.gz`, atau `.deb` dari GitHub Releases, dijalankan
dari baris perintah seperti pengguna biasa.

**Wajib untuk manusia maupun AI agent** (juga dicatat di `AGENTS.md` dan
`docs/ROADMAP.md`):

1. **Setiap fase atau fitur yang selesai wajib memperbarui ketiga berkas.**
   Tambahkan kasus untuk perilaku baru yang terlihat pengguna: sintaks
   baru, builtin baru, modul pustaka baru, perintah atau opsi CLI baru.
   Butir roadmap belum boleh ditandai ✅ sebelum ini dilakukan.
2. **Bagian bersama harus identik di ketiga berkas.** Bagian 2, 3, dan 4
   (di antara penanda `BERSAMA:MULAI` dan `BERSAMA:AKHIR`) disalin persis
   sama. Periksa dengan:
   `diff <(sed -n '/BERSAMA:MULAI/,/BERSAMA:AKHIR/p' docs/TESTING/WINDOWS.md) <(sed -n '/BERSAMA:MULAI/,/BERSAMA:AKHIR/p' docs/TESTING/LINUX.md)`
   (dan pasangan dengan `MACOS.md`), yang harus kosong.
3. **Keluaran yang diharapkan harus berasal dari menjalankan kode**, bukan
   ditulis dari ingatan. Jalankan setiap kasus baru dengan biner hasil
   build, lalu salin keluarannya.
4. **Kasus diberi ID tetap** (`SIN-01`, `CLI-07`, `WIN-03`, …). ID tidak
   pernah dipakai ulang; kasus yang dihapus ditandai "dihapus" beserta
   alasannya, supaya riwayat hasil antar-rilis tetap bisa dibandingkan.
5. **Bagian hasil diisi oleh orang yang benar-benar menguji di OS itu.**
   Jangan menyalin hasil dari OS lain atau dari CI. Bila belum diuji, tulis
   `⬜ belum diuji`.
6. **Setiap rilis (`vX.Y.Z`) diuji ulang di ketiga OS sebelum atau segera
   setelah dipublikasikan.** Tambahkan satu baris di "Riwayat pengujian".
   Temuan yang gagal dilaporkan sebagai issue dan ditautkan dari tabel
   temuan.
7. **Fitur yang belum dirilis** (sudah di `main` tetapi belum ada tagnya)
   boleh ditambahkan ke katalog dengan tanda `(belum dirilis)`. Kasusnya
   baru diuji pada rilis yang memuatnya.

Status: ✅ lulus · ❌ gagal (lihat temuan) · ⚠️ lulus dengan catatan ·
⬜ belum diuji.

## 3. Uji perintah CLI

Semua perintah dijalankan dari folder kerja yang **tidak** memuat folder
`pustaka/`, supaya terbukti pustaka standar diambil dari lokasi instalasi.
Siapkan berkas pendukungnya lebih dulu (isi ada di kolom "Persiapan").

| ID | Persiapan | Perintah | Hasil yang diharapkan |
|---|---|---|---|
| CLI-01 | — | `indk -v` | Nomor versi saja, mis. `1.4.0`, kode keluar 0 |
| CLI-02 | — | `indk -h` | Baris pertama `Indonesiaku <versi>`, lalu `Penggunaan: indk [--sandbox] [path] [argumen...]`, opsi `--dis`, `--periksa-pembaruan`, perintah `periksa`, `uji`, `rapikan`; kode keluar 0 |
| CLI-03 | — | `indk tidak_ada.idk` | stderr `Tidak dapat membuka file "tidak_ada.idk".`, kode keluar 74 |
| CLI-04 | `argumen.idk`: `cetak(panjang(argumen), argumen)` | `indk argumen.idk satu "dua tiga" 3` | `3 ["satu", "dua tiga", "3"]` |
| CLI-05 | `masukan.idk`: `nama = input("Nama? ")`, `umur = ke_angka(input("Umur? "))`, `cetak(f"Halo {nama}, tahun depan {umur + 1}")` | kirim `Budi` dan `16` (dua baris) ke stdin | `Nama? Umur? Halo Budi, tahun depan 17` |
| CLI-06 | — | kirim `x = 6` lalu `cetak(x * 7)` ke stdin `indk` tanpa argumen (REPL) | Prompt `>` lalu `42`; kode keluar 0 saat stdin habis |
| CLI-07 | `sb.idk`: `cetak(lingkungan())` | `indk --sandbox sb.idk` | stderr diawali `lingkungan() tidak diizinkan dalam mode --sandbox`, kode keluar 70 |
| CLI-08 | `k.idk`: `keluar(3)` | `indk k.idk` | Tanpa keluaran, kode keluar 3 |
| CLI-09 | berkas apa saja, mis. SIN-01 | `indk --dis SIN-01.idk` lalu `indk --dis` | Pertama: `== <skrip utama> ==` dan baris `OP_...`, kode 0. Kedua: `Penggunaan: indk --dis <skrip.idk>`, kode 64 |
| CLI-10 | `lint.idk`: fungsi dengan `variabel sementara = a * b` yang tak dipakai | `indk periksa lint.idk` | `lint.idk:3  variabel lokal 'sementara' tidak pernah dipakai`, `1 temuan`, kode keluar 1 |
| CLI-11 | berkas berisi `uji_tambah` (lulus) dan `uji_gagal` (`pastikan tambah(2, 2) == 5, "2 + 2 bukan 5"`) | `indk uji berkas.idk` | `GAGAL  uji_gagal: 2 + 2 bukan 5`, `LULUS  uji_tambah`, `1/2 uji lulus`, kode keluar 1 |
| CLI-12 | `rp.idk`: `fungsi  f( a,b ){` / `kembalikan a+b` / `}` | `indk rapikan --periksa rp.idk`, `indk rapikan --tulis rp.idk`, lalu `--periksa` lagi | `rp.idk belum rapi` (kode 1); berkas menjadi `fungsi f(a, b) {` + `    kembalikan a + b` + `}`; pemeriksaan kedua kode 0 |
| CLI-13 | butuh internet | `indk --periksa-pembaruan` | `Versi terpasang : <versi>`, `Versi terbaru   : v<versi>`; pada rilis terbaru: `Versi Anda sudah yang terbaru.` |
| CLI-14 | `impor_lokasi.idk`: `impor pola`, `impor "pustaka/json.idk"`, `cetak(pola.cocok("a+", "aaa").grup(), json_teks([1]))` | `indk impor_lokasi.idk` dari folder tanpa `pustaka/` | `aaa [1]` |

## 4. Katalog kasus bahasa

Setiap kasus adalah satu berkas `.idk` lengkap. Simpan isinya **apa adanya**
(UTF-8 tanpa BOM, lihat temuan Windows), jalankan `indk <ID>.idk`, lalu
bandingkan stdout, stderr, dan kode keluar dengan yang tertulis. Keluaran di
bawah dihasilkan oleh biner rilis sungguhan, bukan diketik dari ingatan.
Urutan stdout dan stderr di terminal bisa bercampur; bandingkan keduanya
secara terpisah.

### SIN — Sintaks dasar dan operator

#### SIN-01 — Cetak, komentar, variabel, tipe dasar

Berkas `SIN-01.idk`:

```indonesiaku
# Cetak, komentar, variabel, tipe dasar
nama = "Budi"
umur = 17
tinggi = 165.5
cetak("Halo,", nama)
cetak(umur, tinggi, benar, kosong)
cetak(jenis(umur), jenis(nama), jenis(benar), jenis(kosong))
```

Jalankan `indk SIN-01.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
Halo, Budi
17 165.5 benar kosong
angka teks logika kosong
```


#### SIN-02 — Aritmetika gaya Python: bulat vs desimal

Berkas `SIN-02.idk`:

```indonesiaku
# Aritmetika gaya Python: bulat vs desimal
cetak(7 + 2, 7 - 2, 7 * 2, 7 / 2)
cetak(7 // 2, -7 // 2, 7 % 3, -7 % 3, 2 ** 10)
cetak(1 == 1.0, 1 adalah 1.0, 0.1 + 0.2)
x = 10
x += 5
x *= 2
cetak(x)
```

Jalankan `indk SIN-02.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
9 5 14 3.5
3 -4 1 2 1024
benar salah 0.30000000000000004
30
```


#### SIN-03 — Teks: gabung, indeks, irisan, f-string, escape, Unicode

Berkas `SIN-03.idk`:

```indonesiaku
# Teks: gabung, indeks, irisan, f-string, escape, Unicode
s = "Indonesiaku"
cetak(s + "!", panjang(s), s[0], s[-1], s[0:4], s[::-1])
nama = "Ani"
cetak(f"Halo {nama}, 2 + 3 = {2 + 3}")
cetak("baris1\nbaris2\ttab \"kutip\"")
cetak(panjang("é☕"), "é☕"[1], huruf_besar("garuda"))
```

Jalankan `indk SIN-03.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
Indonesiaku! 11 I u Indo ukaisenodnI
Halo Ani, 2 + 3 = 5
baris1
baris2	tab "kutip"
2 ☕ GARUDA
```


#### SIN-04 — Logika, perbandingan berantai, dalam, bitwise

Berkas `SIN-04.idk`:

```indonesiaku
# Logika, perbandingan berantai, dalam, bitwise
cetak(benar dan salah, benar atau salah, tidak benar)
cetak(1 < 2 < 3, 3 > 2 > 5)
cetak(2 dalam [1, 2, 3], "x" tidak dalam "abc")
cetak(6 & 3, 6 | 3, 6 ^ 3, 1 << 4, ~5)
```

Jalankan `indk SIN-04.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
salah benar salah
benar salah
benar benar
2 7 5 16 -6
```


### CAB — Percabangan

#### CAB-01 — jika / jika_lain / selain

Berkas `CAB-01.idk`:

```indonesiaku
# jika / jika_lain / selain
fungsi nilai_huruf(n) {
    jika n >= 85 {
        kembalikan "A"
    } jika_lain n >= 70 {
        kembalikan "B"
    } jika_lain n >= 55 {
        kembalikan "C"
    } selain {
        kembalikan "D"
    }
}
untuk n dalam [90, 75, 60, 10] {
    cetak(n, nilai_huruf(n))
}
cetak("genap" jika 4 % 2 == 0 selain "ganjil")
```

Jalankan `indk CAB-01.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
90 A
75 B
60 C
10 D
genap
```


#### CAB-02 — Kebenaran gaya Python

Berkas `CAB-02.idk`:

```indonesiaku
# Kebenaran gaya Python
untuk v dalam [0, 1, "", "a", [], [0], kamus(), kosong, 0.0] {
    jika v {
        cetak(repr(v), "-> benar")
    } selain {
        cetak(repr(v), "-> salah")
    }
}
```

Jalankan `indk CAB-02.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
0 -> salah
1 -> benar
"" -> salah
"a" -> benar
[] -> salah
[0] -> benar
{} -> salah
kosong -> salah
0.0 -> salah
```


#### CAB-03 — cocokkan / kasus (pattern matching)

Berkas `CAB-03.idk`:

```indonesiaku
# cocokkan / kasus (pattern matching)
fungsi jelaskan(x) {
    cocokkan x {
        kasus 0 {
            kembalikan "nol"
        }
        kasus 1 atau 2 {
            kembalikan "satu atau dua"
        }
        kasus n jika n < 0 {
            kembalikan "negatif"
        }
        kasus _ {
            kembalikan "lainnya"
        }
    }
}
untuk x dalam [0, 2, -5, 99] {
    cetak(x, jelaskan(x))
}
```

Jalankan `indk CAB-03.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
0 nol
2 satu atau dua
-5 negatif
99 lainnya
```


### ULG — Perulangan

#### ULG-01 — selagi, hentikan, lanjut

Berkas `ULG-01.idk`:

```indonesiaku
# selagi, hentikan, lanjut
i = 0
selagi benar {
    i += 1
    jika i == 2 {
        lanjut
    }
    jika i > 5 {
        hentikan
    }
    cetak("i =", i)
}
```

Jalankan `indk ULG-01.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
i = 1
i = 3
i = 4
i = 5
```


#### ULG-02 — untuk ... dalam: rentang, daftar, teks, kamus, bersarang

Berkas `ULG-02.idk`:

```indonesiaku
# untuk ... dalam: rentang, daftar, teks, kamus, bersarang
untuk i dalam rentang(1, 10, 3) {
    cetak(i)
}
untuk huruf dalam "abc" {
    cetak(huruf)
}
nilai = {"budi": 80, "ani": 95}
untuk k dalam nilai {
    cetak(k, nilai[k])
}
untuk a dalam rentang(1, 3) {
    untuk b dalam rentang(1, 3) {
        cetak(a, "x", b, "=", a * b)
    }
}
```

Jalankan `indk ULG-02.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
1
4
7
a
b
c
budi 80
ani 95
1 x 1 = 1
1 x 2 = 2
2 x 1 = 2
2 x 2 = 4
```


#### ULG-03 — Komprehensi dan generator

Berkas `ULG-03.idk`:

```indonesiaku
# Komprehensi dan generator
kuadrat = [x * x untuk x dalam rentang(6) jika x % 2 == 0]
cetak(kuadrat)
cetak({k: panjang(k) untuk k dalam ["a", "bb"]})
cetak(jumlah(x untuk x dalam rentang(101)))
fungsi hitung_mundur(n) {
    selagi n > 0 {
        hasilkan n
        n -= 1
    }
}
cetak(daftar(hitung_mundur(3)))
```

Jalankan `indk ULG-03.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
[0, 4, 16]
{"a": 1, "bb": 2}
5050
[3, 2, 1]
```


### FNG — Fungsi

#### FNG-01 — Fungsi: parameter bawaan, argumen bernama, rekursi, nilai kembali ganda

Berkas `FNG-01.idk`:

```indonesiaku
# Fungsi: parameter bawaan, argumen bernama, rekursi, nilai kembali ganda
fungsi sapa(nama, salam = "Halo") {
    kembalikan salam + ", " + nama
}
cetak(sapa("Budi"))
cetak(sapa(salam = "Selamat pagi", nama = "Ani"))
fungsi faktorial(n) {
    jika n <= 1 {
        kembalikan 1
    }
    kembalikan n * faktorial(n - 1)
}
cetak(faktorial(20))
fungsi bagi(a, b) {
    kembalikan a // b, a % b
}
h, s = bagi(17, 5)
cetak(h, s)
```

Jalankan `indk FNG-01.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
Halo, Budi
Selamat pagi, Ani
2432902008176640000
3 2
```


#### FNG-02 — Closure, nonlokal, global, fungsi sebagai nilai, dekorator

Berkas `FNG-02.idk`:

```indonesiaku
# Closure, nonlokal, global, fungsi sebagai nilai, dekorator
fungsi pembuat_penghitung() {
    n = 0
    fungsi tambah_satu() {
        nonlokal n
        n += 1
        kembalikan n
    }
    kembalikan tambah_satu
}
c = pembuat_penghitung()
c()
cetak(c())
total = 0
fungsi setor(x) {
    global total
    total += x
}
setor(5)
setor(7)
cetak(total)
cetak(daftar(peta(fungsi(x) { kembalikan x * 10 }, [1, 2, 3])))
fungsi catat(f) {
    fungsi bungkus(x) {
        cetak("sebelum")
        kembalikan f(x)
    }
    kembalikan bungkus
}
@catat
fungsi ganda(x) {
    kembalikan x * 2
}
cetak(ganda(21))
```

Jalankan `indk FNG-02.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
2
12
[10, 20, 30]
sebelum
42
```


### KOL — Koleksi

#### KOL-01 — daftar, tupel, kamus, himpunan

Berkas `KOL-01.idk`:

```indonesiaku
# daftar, tupel, kamus, himpunan
d = [3, 1, 2]
d.tambah(5)
cetak(d, panjang(d), terurut(d), d[1:3], d[-1])
t = (1, "dua", 3.0)
cetak(t, t[1])
k = {"nama": "Budi", "kelas": 10}
k["umur"] = 16
cetak(k, k["nama"], "umur" dalam k, kunci(k))
h = himpunan([1, 2, 2, 3])
cetak(terurut(h), terurut(h | himpunan([4])), terurut(h & himpunan([2, 9])))
a, b = (1, 2)
cetak(a, b)
```

Jalankan `indk KOL-01.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
[3, 1, 2, 5] 4 [1, 2, 3, 5] [1, 2] 5
(1, "dua", 3.0) dua
{"nama": "Budi", "kelas": 10, "umur": 16} Budi benar ["nama", "kelas", "umur"]
[1, 2, 3] [1, 2, 3, 4] [2]
1 2
```


### KLS — Kelas dan OOP

#### KLS-01 — Kelas, init, diri, warisan, super, metode ajaib

Berkas `KLS-01.idk`:

```indonesiaku
# Kelas, init, diri, warisan, super, metode ajaib
kelas Hewan {
    init(nama) {
        diri.nama = nama
    }
    suara() {
        kembalikan "..."
    }
    _teks_() {
        kembalikan f"{diri.nama} berkata {diri.suara()}"
    }
}
kelas Kucing < Hewan {
    init(nama, warna) {
        super.init(nama)
        diri.warna = warna
    }
    suara() {
        kembalikan "meong"
    }
}
k = Kucing("Tom", "abu")
cetak(k)
cetak(k.warna, adalah_instansi(k, Hewan))
```

Jalankan `indk KLS-01.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
Tom berkata meong
abu benar
```


#### KLS-02 — Operator lewat metode ajaib, properti, kelas abstrak, dengan

Berkas `KLS-02.idk`:

```indonesiaku
# Operator lewat metode ajaib, properti, kelas abstrak, dengan
kelas Vektor {
    init(x, y) {
        diri.x = x
        diri.y = y
    }
    _tambah_(lain) {
        kembalikan Vektor(diri.x + lain.x, diri.y + lain.y)
    }
    _sama_(lain) {
        kembalikan diri.x == lain.x dan diri.y == lain.y
    }
    _teks_() {
        kembalikan f"Vektor({diri.x}, {diri.y})"
    }
    @properti
    panjang_kuadrat() {
        kembalikan diri.x ** 2 + diri.y ** 2
    }
}
v = Vektor(1, 2) + Vektor(3, 4)
cetak(v, v == Vektor(4, 6), v.panjang_kuadrat)
kelas Sumber {
    _masuk_() {
        cetak("buka")
        kembalikan diri
    }
    _keluar_() {
        cetak("tutup")
    }
}
dengan Sumber() sebagai s {
    cetak("pakai")
}
```

Jalankan `indk KLS-02.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
Vektor(4, 6) benar 52
buka
pakai
tutup
```


### GAL — Penanganan galat

#### GAL-01 — coba / kecuali / akhirnya, galat bertipe, galat buatan sendiri

Berkas `GAL-01.idk`:

```indonesiaku
# coba / kecuali / akhirnya, galat bertipe, galat buatan sendiri
coba {
    cetak(10 / 0)
} kecuali GalatBagiNol sebagai e {
    cetak("tertangkap:", e.pesan)
} akhirnya {
    cetak("akhirnya jalan")
}
kelas GalatSaldo < Galat {
    init(pesan) {
        diri.pesan = pesan
    }
}
coba {
    naikkan GalatSaldo("saldo kurang")
} kecuali (GalatNilai, GalatSaldo) sebagai e {
    cetak(adalah_instansi(e, GalatSaldo), e.pesan)
}
coba {
    [1, 2][5]
} kecuali GalatIndeks sebagai e {
    cetak("indeks:", e.pesan)
}
```

Jalankan `indk GAL-01.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
tertangkap: Pembagian dengan nol.
akhirnya jalan
benar saldo kurang
indeks: Indeks list di luar jangkauan (indeks 5, panjang 2).
```


#### GAL-02 — Galat tak tertangkap: pesan + kolom + jejak, kode keluar 70

Berkas `GAL-02.idk`:

```indonesiaku
# Galat tak tertangkap: pesan + kolom + jejak, kode keluar 70
fungsi bagi(a, b) {
    kembalikan a / b
}
cetak("sebelum")
bagi(1, 0)
cetak("tidak tercetak")
```

Jalankan `indk GAL-02.idk`. **Kode keluar: `70`.**

stdout yang diharapkan:

```text
sebelum
```

stderr yang diharapkan:

```text
Pembagian dengan nol.
[baris 3, kolom 20] dalam bagi()
        kembalikan a / b
                       ^
[baris 6, kolom 10] dalam skrip
```


#### GAL-03 — Galat kompilasi: blok tidak ditutup, kode keluar 65, tidak ada yang dijalankan

Berkas `GAL-03.idk`:

```indonesiaku
# Galat kompilasi: blok tidak ditutup, kode keluar 65, tidak ada yang dijalankan
cetak("tidak jalan")
jika benar {
    cetak(1)
```

Jalankan `indk GAL-03.idk`. **Kode keluar: `65`.**

stderr yang diharapkan:

```text
[baris 5, kolom 1] Kesalahan di akhir: Harapkan '}' setelah blok.
    
    ^
```


### MOD — Modul dan impor

#### MOD-01 — Modul bernamespace dari pustaka standar (dicari dari lokasi interpreter)

Berkas `MOD-01.idk`:

```indonesiaku
# Modul bernamespace dari pustaka standar (dicari dari lokasi interpreter)
impor statistik
dari koleksi impor Penghitung
impor jalur sebagai j
cetak(statistik.rata_rata([2, 4, 9]), statistik.median([5, 1, 3]))
cetak(Penghitung("mississippi").paling_umum(2))
cetak(j.gabung("a", "b.txt"), j.ekstensi("data.tar.gz"))
```

Jalankan `indk MOD-01.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
5.0 3
[["i", 4], ["s", 4]]
a/b.txt .gz
```


#### MOD-02 — Modul buatan sendiri di folder yang sama, dan __nama__

Berkas pendukung `paket_saya/hitung.idk`:

```indonesiaku
fungsi kali_dua(x) {
    kembalikan x * 2
}
```

Berkas `MOD-02.idk` (di folder yang sama dengan `paket_saya/`):

```indonesiaku
# Modul buatan sendiri di folder yang sama, dan __nama__
impor paket_saya.hitung sebagai h
cetak(h.kali_dua(21), __nama__)
```

Jalankan `indk MOD-02.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
42 __utama__
```


### PUS — Pustaka standar

#### PUS-01 — Fungsi bawaan: matematika, konversi, teks

Berkas `PUS-01.idk`:

```indonesiaku
# Fungsi bawaan: matematika, konversi, teks
cetak(abs(-3), min(4, 2, 8), max([4, 2, 8]), akar(16), pangkat(2, 8))
cetak(bulatkan(2.5), bulat(2.5), lantai(-2.5), atap(2.1), bulatkan(3.14159, 2))
cetak(ke_angka("42") + 1, ke_teks(3.0), ke_bulat(9.99), heksa(255), biner(5))
cetak(ganti("a-b-c", "-", "+"), potong("Indonesia", 0, 4), huruf_kecil("ABC"))
cetak(sin(0), bulatkan(cos(radian(60)), 6), derajat(atan2(1, 1)))
```

Jalankan `indk PUS-01.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
3 2 8 4.0 256
2 3 -3 3 3.14
43 3.0 9 0xff 0b101
a+b+c Indo abc
0.0 0.5 45.0
```


#### PUS-02 — Berkas: tulis, baca, ada, hapus (folder kerja harus bisa ditulisi)

Berkas `PUS-02.idk`:

```indonesiaku
# Berkas: tulis, baca, ada, hapus (folder kerja harus bisa ditulisi)
impor "pustaka/teks.idk"
nama = "uji_berkas_sementara.txt"
tulis_semua(nama, "baris satu\nbaris dua ☕\n")
cetak(ada_berkas(nama))
isi = baca_semua(nama)
cetak(panjang(isi), teks_pisah(isi, "\n")[1])
hapus_berkas(nama)
cetak(ada_berkas(nama))
```

Jalankan `indk PUS-02.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
benar
23 baris dua ☕
salah
```


#### PUS-03 — JSON, hash, base64, csv, pola (regex)

Berkas `PUS-03.idk`:

```indonesiaku
# JSON, hash, base64, csv, pola (regex)
impor "pustaka/json.idk"
impor base64
impor csv
impor pola
data = json_urai("{\"nama\": \"Ani\", \"nilai\": [90, 85]}")
cetak(data["nama"], data["nilai"][1])
cetak(json_teks({"a": [1, 2]}))
cetak(sha256("abc"))
cetak(md5(""))
cetak(base64.enkode("Halo"), base64.dekode_teks("SGFsbw=="))
cetak(csv.urai("a,b\n1,\"x,y\"\n"))
cetak(pola.cari_semua("[0-9]+", "a1 b22 c333"))
```

Jalankan `indk PUS-03.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
Ani 85
{"a":[1,2]}
ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
d41d8cd98f00b204e9800998ecf8427e
SGFsbw== Halo
[["a", "b"], ["1", "x,y"]]
["1", "22", "333"]
```


#### PUS-04 — Waktu, acak, koleksi, iterasi, fungsional, salin

Berkas `PUS-04.idk`:

```indonesiaku
# Waktu, acak, koleksi, iterasi, fungsional, salin
cetak(jenis(waktu()), waktu() > 1700000000)
acak_bulat_ok = acak_bulat(1, 6)
cetak(acak_bulat_ok >= 1 dan acak_bulat_ok <= 6)
impor iterasi
impor fungsional
impor salin
cetak(iterasi.kombinasi([1, 2, 3], 2))
cetak(fungsional.lipat(fungsi(a, b) { kembalikan a + b }, [1, 2, 3, 4]))
asli = [[1], [2]]
tiruan = salin.salin_dalam(asli)
tiruan[0].tambah(9)
cetak(asli, tiruan)
```

Jalankan `indk PUS-04.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
angka benar
benar
[[1, 2], [1, 3], [2, 3]]
10
[[1], [2]] [[1, 9], [2]]
```


#### PUS-05 — kura (SVG), proses anak, opsi (argparse)

Berkas `PUS-05.idk`:

```indonesiaku
# kura (SVG), proses anak, opsi (argparse)
impor kura
impor proses
impor opsi
untuk i dalam rentang(4) {
    kura.maju(50)
    kura.kiri(90)
}
cetak(kura.posisi(), kura.svg()[0:4])
hasil = proses.jalankan([jalur_indk(), "-v"])
cetak(hasil.kode_keluar, panjang(hasil.keluaran) > 0)
p = opsi.PenguraiArgumen("salam")
p.tambah_argumen("nama")
p.tambah_argumen("--ulang", jenis_nilai = ke_bulat, bawaan = 1)
a = p.urai(["Budi", "--ulang", "2"])
cetak(a.nama, a.ulang)
```

Jalankan `indk PUS-05.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
(0.0, 0.0) <svg
0 benar
Budi 2
```


### DB — Basis data SQLite

#### DB-01 — SQLite tertanam (di memori), parameter ? wajib untuk nilai

Berkas `DB-01.idk`:

```indonesiaku
# SQLite tertanam (di memori), parameter ? wajib untuk nilai
db = db_buka(":memory:")
db_eksekusi(db, "CREATE TABLE siswa (id INTEGER PRIMARY KEY, nama TEXT, nilai INTEGER)", [])
db_eksekusi(db, "INSERT INTO siswa (nama, nilai) VALUES (?, ?)", ["Budi", 80])
db_eksekusi(db, "INSERT INTO siswa (nama, nilai) VALUES (?, ?)", ["Ani", 95])
cetak(db_id_terakhir(db))
cetak(db_ambil_semua(db, "SELECT nama, nilai FROM siswa WHERE nilai > ? ORDER BY nilai DESC", [70]))
db_tutup(db)
```

Jalankan `indk DB-01.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
2
[{"nama": "Ani", "nilai": 95}, {"nama": "Budi", "nilai": 80}]
```

### GALAT — Penanganan galat

#### GALAT-04 — Blok `coba` di dalam fungsi rekursif

Berkas `GALAT-04.idk`:

```indonesiaku
# Blok `coba` di dalam fungsi rekursif tidak boleh menjadi batas sendiri.
fungsi turun(n) {
    jika n <= 0 {
        kembalikan 0
    }
    coba {
        kembalikan turun(n - 1) + 1
    } kecuali Galat {
        kembalikan -1
    }
}

cetak(turun(63))
cetak(turun(65))
cetak(turun(500))

# Rekursi tak terbatas tetap berhenti sebagai GalatRekursi, bukan galat
# "terlalu banyak blok coba".
fungsi s(n) { coba { kembalikan s(n + 1) } kecuali GalatTipe sebagai e { kembalikan 0 } }
coba { s(0) } kecuali GalatRekursi sebagai e { cetak("rekursi") } kecuali e { cetak("lain:", e.pesan) }
```

Jalankan `indk GALAT-04.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
63
65
500
rekursi
```

Sebelum perbaikan, `turun(65)` mencetak `62` — bukan galat, melainkan jawaban
salah diam-diam, karena galat batas `coba` ditangkap oleh `kecuali` di level
luar.

#### DB-MIG-01 — Migrasi skema tercatat dan bisa dibatalkan

```indonesiaku
impor db
impor migrasi

variabel k = db.sambung({"jenis": "sqlite", "berkas": ":memory:"})

variabel m = daftar()

# Sengaja didaftarkan tidak urut: 002 lebih dulu.
m.tambah(migrasi.langkah("002", "tambah kolom nilai",
    fungsi(k) { k.eksekusi("ALTER TABLE siswa ADD COLUMN nilai REAL") },
    fungsi(k) { k.eksekusi("ALTER TABLE siswa DROP COLUMN nilai") }))

m.tambah(migrasi.langkah("001", "buat tabel siswa",
    fungsi(k) { k.eksekusi("CREATE TABLE siswa (id INTEGER PRIMARY KEY, nama TEXT)") },
    fungsi(k) { k.eksekusi("DROP TABLE siswa") }))

cetak("dijalankan:", migrasi.naikkan_semua(k, m))
cetak("dijalankan lagi:", migrasi.naikkan_semua(k, m))
cetak("dibatalkan:", migrasi.turunkan_satu(k, m))

untuk s dalam migrasi.status(k, m) {
    cetak("  " + s["versi"], s["nama"], s["terpasang"])
}

k.tutup()
```

Jalankan `indk DB-MIG-01.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
dijalankan: 2
dijalankan lagi: 0
dibatalkan: 002
  001 buat tabel siswa benar
  002 tambah kolom nilai salah
```

Yang diperiksa: migrasi berjalan urut menaik menurut versi meski didaftarkan
terbalik (kalau tidak, `ALTER` jalan sebelum tabelnya ada), rerun tidak
mengulang DDL, dan rollback mengambil versi TERBARU.

<!-- BERSAMA:AKHIR -->

## 5. Suite otomatis dari tag rilis

Suite golden repo dijalankan terhadap **biner rilis**, bukan hasil build:
ambil berkas uji dari tag rilis, salin `indk` rilis ke akarnya, lalu
jalankan runner tanpa membangun ulang.

```bash
git clone https://github.com/teguh02/Indonesiaku-Lang.git
cd Indonesiaku-Lang
mkdir -p /tmp/uji-rilis && git archive v<versi> examples tests pustaka website/panduan | tar -x -C /tmp/uji-rilis
cp "$(command -v indk)" /tmp/uji-rilis/indk
cd /tmp/uji-rilis
```

`tests/test_all.sh` membangun ulang biner dari kode sumber, jadi untuk
menguji biner rilis jalankan bagian golden-nya dengan biner yang disalin
(lihat cara di `tests/test_all.sh`), atau bandingkan setiap
`examples/**/<nama>.idk` dengan `tests/expected/<nama>.out` dan setiap
`tests/galat/<nama>.idk` dengan `<nama>.err` (kode keluar bukan 0).

| ID | Suite | Hasil yang diharapkan |
|---|---|---|
| OTO-01 | Semua golden `tests/expected/*.out` + `tests/galat/` + `test_impor_lokasi.sh` | Semua lulus |
| OTO-02 | `test_todo_cli.sh` | `== ALL TODO CLI TESTS PASSED ==` |
| OTO-03 | `test_todo_server.sh` | `== ALL TODO SERVER TESTS PASSED ==` |
| OTO-04 | `test_lint.sh`, `test_uji.sh`, `test_format.sh` | Semua lulus |
| OTO-05 | `test_panduan.sh` | `N/N dijalankan bersih` |
| OTO-06 | `test_klien_http.sh` | `ok   http_ambil GET/POST/404 (termasuk badan UTF-8)` |
| OTO-07 | `web_router_server.sh` | `ok   pustaka/web.idk melayani HTTP sungguhan` |
| OTO-08 | `web_badan_server.sh` | Semua baris `ok` |
| OTO-09 | `http_pengalihan_server.sh` | Tanpa `GAGAL` |

## 6. Uji khusus Linux

| ID | Kasus | Cara | Hasil yang diharapkan |
|---|---|---|---|
| LIN-01 | Pasang `.deb` | `sudo dpkg -i ...`, `dpkg -L indonesiaku` | Berkas di lokasi bagian 1 langkah 3; `indk` di `PATH` |
| LIN-02 | Pembaruan `.deb` | Pasang versi lama, lalu versi baru | dpkg melaporkan `Unpacking ... over`; `indk -v` versi baru |
| LIN-03 | Hapus `.deb` | `sudo dpkg -r indonesiaku` | `/usr/bin/indk` dan `/usr/share/indonesiaku` hilang |
| LIN-04 | Pustaka dari `/usr/share/indonesiaku/pustaka` | CLI-14 setelah memasang `.deb` | `aaa [1]` |
| LIN-05 | Arsip `.tar.gz` di distro non-Debian (mis. Fedora, Arch) | Jalankan `indk-linux/indk` dan CLI-14 | Berjalan; impor pustaka bekerja |
| LIN-06 | Distro glibc lama | Ubuntu LTS tertua yang didukung, `ldd indk` | Catat versi glibc minimum; galat `GLIBC_2.xx not found` adalah temuan |
| LIN-07 | CPU baseline x86-64 | `readelf -n indk \| grep ISA` | Tidak menuntut `x86-64-v2/v3/v4` (rilis dibangun dengan `-march=x86-64`) |
| LIN-08 | musl / Alpine | `tar.gz` di Alpine, atau image Docker `teguh02/indonesiaku:<versi>` | Catat apakah biner glibc berjalan; image Docker harus berjalan |
| LIN-09 | Jalur dengan spasi dan non-ASCII | `indk "folder spasi/naïve ☕/s.idk"`, `indk 日本/s.idk` | Skrip berjalan |
| LIN-10 | Nama berkas non-ASCII yang ditulis program | `tulis_semua("keluaran_☕.txt", "x")` | Berkas bernama `keluaran_☕.txt` |
| LIN-11 | Sumber UTF-8 dengan BOM | Berkas berisi BOM + `cetak("bom")` | `bom` tercetak (lihat T-W2 di `WINDOWS.md`) |
| LIN-12 | Locale non-UTF-8 | `LANG=C indk SIN-03.idk` | Keluaran byte sama dengan locale UTF-8 |
| LIN-13 | Soket localhost | OTO-03, OTO-06 | Lulus |
| LIN-14 | Proses anak | PUS-05 | `0 benar` |
| LIN-15 | Shell bash, zsh, dan sh (dash) | CLI-04 di setiap shell | `3 ["satu", "dua tiga", "3"]` |

## 7. Hasil uji terakhir

⬜ **Belum diuji di Linux dengan biner rilis.** CI (`test-linux`,
`sanitizers`, `valgrind`, `todo-*`) menguji hasil build dari kode sumber,
bukan paket rilis, sehingga tidak menggantikan bagian ini. Isi setelah
menguji di mesin Linux sungguhan: tabel ringkasan, rincian per ID, dan
temuan dengan kode `T-L<n>`, mengikuti bentuk bagian 7 di `WINDOWS.md`.

| Bagian | Hasil |
|---|---|
| 3. CLI (CLI-01–CLI-14) | ⬜ |
| 4. Katalog bahasa | ⬜ |
| 5. Suite otomatis | ⬜ |
| 6. Khusus Linux | ⬜ |

**Uji ulang temuan Windows yang tidak khas Windows.** Uji ekstensif
Windows 1.4.0 menemukan bug bahasa T-U2–T-U16 dan 120-an temuan lain yang
kemungkinan juga terjadi di OS ini (lihat `WINDOWS.md` bagian 7 dan
[`lampiran/WINDOWS-1.4.0-uji-ekstensif.md`](lampiran/WINDOWS-1.4.0-uji-ekstensif.md)).
Jalankan reproduksi minimal di lampiran itu dengan biner OS ini, lalu catat
mana yang terulang di tabel temuan (kode `T-L<n>`, sebutkan kode
Windows-nya). Temuan khas Windows T-W8 (crash kompilator pada nesting
dalam) juga perlu diuji ambangnya di sini.

## 8. Riwayat pengujian

| Tanggal | Versi | Paket | Penguji | Hasil |
|---|---|---|---|---|
| — | — | — | — | ⬜ belum ada |
