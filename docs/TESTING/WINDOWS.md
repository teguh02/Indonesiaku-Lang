# Rencana dan Hasil Uji — Windows

Rencana uji paket rilis Indonesiaku di **Windows**, beserta hasil uji
terakhirnya. Pasangannya: [`MACOS.md`](MACOS.md) dan [`LINUX.md`](LINUX.md).
Aturan memperbarui berkas ini ada di bagian 2 dan di `AGENTS.md`.

| | |
|---|---|
| Versi terakhir diuji | `1.4.0` (tag `v1.4.0`) |
| Tanggal | 2026-11-03 |
| Paket | `Indonesiaku-Setup-1.4.0.exe` (terpasang di `C:\Program Files\Indonesiaku`) |
| SHA-256 `indk.exe` terpasang | `b6da48f2b2af3b6106fcff2b653a6bf5a15fa6f19e7e7a92755dbd50601bc72a` |
| Mesin | Windows 11 Home 10.0.26300, AMD64; Windows PowerShell 5.1; Git Bash 2.51; curl 8.15 |
| Ringkasan | ⚠️ Suite rilis, katalog, dan CLI lulus semua. Uji ekstensif multi-agen: 3584/3879 pemeriksaan lulus, **140 temuan terkonfirmasi** (7 di antaranya khas Windows, termasuk 2 temuan keamanan yang dilaporkan secara privat), lihat bagian 7 |

## 1. Persiapan

1. Unduh dari [GitHub Releases](https://github.com/teguh02/Indonesiaku-Lang/releases)
   salah satu dari:
   - `Indonesiaku-Setup-<versi>.exe` — installer (disarankan untuk pengguna);
   - `indk-windows.zip` — biner portabel.
2. Cocokkan SHA-256 berkas unduhan dengan `SHA256SUMS.txt`:
   `Get-FileHash .\Indonesiaku-Setup-<versi>.exe -Algorithm SHA256`.
3. Jalankan installer. Hasil yang diharapkan: `C:\Program Files\Indonesiaku\`
   berisi `indk.exe`, `pustaka\`, `examples\`, `README.md`, `CHANGELOG.md`,
   `LICENSE`, dan `unins000.exe`; folder itu masuk `PATH`.
4. Buka **terminal baru** (PowerShell dan `cmd.exe`) supaya `PATH` baru
   terbaca, lalu pastikan `indk -v` mencetak versi rilis.
5. Buat folder kerja kosong di luar repo (mis. `%TEMP%\uji-indk`) untuk
   kasus bagian 3 dan 4. Folder itu **tidak boleh** memuat `pustaka\`.

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

#### SSR-TPL-01 — Templat HTML: interpolasi, escaping, dan batas `{{ }}`

```indonesiaku
impor templat

variabel data = {"judul": "Daftar Siswa",
                 "siswa": ["Budi", "<script>alert(1)</script>"],
                 "catatan": templat.aman("<b>resmi</b>")}

cetak(templat.render_teks("<h1>{{ judul }}</h1>", data))

untuk s dalam data["siswa"] {
    cetak(templat.render_teks("<li>{{ x }}</li>", {"x": s}))
}

cetak(templat.render_teks("{{ catatan }}", data))
cetak(templat.render_teks("{% jika siswa %}ada{% selain %}kosong{% akhirjika %}", data))

# {{ }} bukan kode: pemanggilan fungsi harus ditolak, bukan dijalankan.
coba {
    templat.render_teks("{{ keluar() }}", data)
    cetak("GAGAL: seharusnya ditolak")
} kecuali Galat sebagai e {
    cetak("ditolak: " + e.pesan)
}
```

Jalankan `indk SSR-TPL-01.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
<h1>Daftar Siswa</h1>
<li>Budi</li>
<li>&lt;script&gt;alert(1)&lt;/script&gt;</li>
<b>resmi</b>
ada
ditolak: baris 1: '(' tidak diizinkan di dalam templat (hanya nama, .kunci dan [indeks]): 'keluar()'
```

Yang diperiksa: data pengguna selalu di-escape (baris kedua dan ketiga berasal
dari daftar yang sama, hanya isinya berbeda), `templat.aman()` adalah
satu-satunya jalan agar HTML lolos utuh, dan `{{ }}` menolak pemanggilan
fungsi alih-alih mengeksekusinya — kalau penolakan ini hilang, setiap berkas
`.html` menjadi titik eksekusi kode.

#### SSR-TPL-02 — Templat: pewarisan, blok bawaan, dan siklus sertakan

Buat direktori `tampilan/` berisi empat berkas:

`tampilan/dasar.html`:

```html
<!doctype html>
<title>{{ judul }}</title>
<body>{% blok isi %}isi bawaan{% akhirblok %}</body>
```

`tampilan/anak.html`:

```html
{% perluas "dasar.html" %}
{% blok isi %}<p>Halo {{ nama }}</p>{% akhirblok %}
```

`tampilan/anak_bawaan.html`:

```html
{% perluas "dasar.html" %}
```

`tampilan/siklus_a.html` dan `tampilan/siklus_b.html` yang saling menyertakan:

```html
{% sertakan "siklus_b.html" %}
```

```html
{% sertakan "siklus_a.html" %}
```

Lalu `SSR-TPL-02.idk`:

```indonesiaku
impor templat
variabel m = templat.mesin("tampilan")
cetak(m.render("anak.html", {"judul": "Daftar Siswa", "nama": "<b>Budi</b>"}))
cetak(m.render("anak_bawaan.html", {"judul": "Tanpa Timpa"}))
coba {
    m.render("siklus_a.html", kamus())
} kecuali Galat sebagai e { cetak("siklus -> " + e.pesan) }
```

Jalankan `indk SSR-TPL-02.idk`. **Kode keluar: `0`.**

stdout yang diharapkan:

```text
<!doctype html>
<title>Daftar Siswa</title>
<body><p>Halo &lt;b&gt;Budi&lt;/b&gt;</p></body>

<!doctype html>
<title>Tanpa Timpa</title>
<body>isi bawaan</body>

siklus -> siklus templat: siklus_a.html -> siklus_b.html -> siklus_a.html
```

Yang diperiksa: anak mengisi lubang `{% blok %}` milik induk sementara kerangka
halaman tetap milik induk, blok yang tidak ditimpa memakai isi bawaan, escaping
tetap berlaku di dalam blok yang diwariskan, dan siklus sertakan mati dengan
menyebut rantainya alih-alih menggantung sampai kehabisan memori.

<!-- BERSAMA:AKHIR -->

## 5. Suite otomatis dari tag rilis

Suite golden repo dijalankan terhadap **biner terpasang**, bukan hasil
build: ambil berkas uji dari tag rilis, salin `indk.exe` terpasang ke
akarnya, lalu jalankan runner Windows.

```powershell
git clone https://github.com/teguh02/Indonesiaku-Lang.git
cd Indonesiaku-Lang
git archive v1.4.0 examples tests pustaka website/panduan -o $env:TEMP\uji.tar
mkdir $env:TEMP\uji-v140; tar -xf $env:TEMP\uji.tar -C $env:TEMP\uji-v140
Copy-Item 'C:\Program Files\Indonesiaku\indk.exe' $env:TEMP\uji-v140\
powershell -ExecutionPolicy Bypass -File $env:TEMP\uji-v140\tests\test_all.ps1
powershell -ExecutionPolicy Bypass -File $env:TEMP\uji-v140\tests\test_todo_cli.ps1
powershell -ExecutionPolicy Bypass -File $env:TEMP\uji-v140\tests\test_todo_server.ps1
```

Skrip `.sh` dijalankan dari Git Bash di folder yang sama, setelah
`cp indk.exe indk`: `test_lint.sh`, `test_uji.sh`, `test_format.sh`,
`test_panduan.sh`, `test_klien_http.sh`, `web_router_server.sh`,
`web_badan_server.sh`, `http_pengalihan_server.sh`.

| ID | Suite | Hasil yang diharapkan |
|---|---|---|
| OTO-01 | `test_all.ps1` (golden + `tests/galat` + lokasi impor) | `TOTAL: N/N PASSED` |
| OTO-02 | `test_todo_cli.ps1` | `== ALL TODO CLI TESTS PASSED ==` |
| OTO-03 | `test_todo_server.ps1` (Winsock, server REST) | `== ALL TODO SERVER TESTS PASSED ==` |
| OTO-04 | `test_lint.sh`, `test_uji.sh`, `test_format.sh` | `LINTER: 5/5`, `UJI: 2/2`, `FORMAT: 7/7 lulus` |
| OTO-05 | `test_panduan.sh` (semua contoh kode di panduan) | `N/N dijalankan bersih` |
| OTO-06 | `test_klien_http.sh` (klien `http_ambil` melawan server Indonesiaku) | `ok   http_ambil GET/POST/404 (termasuk badan UTF-8)` |
| OTO-07 | `web_router_server.sh` | `ok   pustaka/web.idk melayani HTTP sungguhan` |
| OTO-08 | `web_badan_server.sh` | Semua baris `ok` |
| OTO-09 | `http_pengalihan_server.sh` | Tanpa `GAGAL` |

## 6. Uji khusus Windows

| ID | Kasus | Cara | Hasil yang diharapkan |
|---|---|---|---|
| WIN-01 | Installer memasang berkas dan `PATH` | Jalankan installer, buka terminal baru, `Get-Command indk` | `C:\Program Files\Indonesiaku\indk.exe`, `pustaka\` di sebelahnya |
| WIN-02 | Biner portabel `indk-windows.zip` | Ekstrak ke folder mana saja, jalankan `.\indk.exe -v` dan CLI-14 | Versi tercetak; impor pustaka bekerja |
| WIN-03 | Checksum unduhan | `Get-FileHash` dibandingkan dengan `SHA256SUMS.txt` | Sama persis |
| WIN-04 | `cmd.exe` dan PowerShell | `indk -v` di keduanya; `echo %ERRORLEVEL%` / `$LASTEXITCODE` setelah CLI-03 | Versi tercetak; kode keluar 74 terbaca |
| WIN-05 | Sumber berakhiran baris CRLF | Simpan SIN-03 dengan CRLF (Notepad), dan teks tiga-kutip yang memuat baris baru | Keluaran sama dengan versi LF; tidak ada `\r` yang bocor ke teks |
| WIN-06 | Jalur dengan spasi | `indk "folder spasi\a b.idk"` | Skrip berjalan |
| WIN-07 | Jalur non-ASCII yang ada di code page ANSI | `indk José\s.idk` | Skrip berjalan |
| WIN-08 | Jalur non-ASCII di luar code page ANSI | `indk 日本\s.idk`, `indk "naïve ☕\s.idk"` | Skrip berjalan |
| WIN-09 | Nama berkas non-ASCII yang ditulis program | `tulis_semua("keluaran_☕.txt", "x")`, lalu lihat di Explorer | Berkas bernama `keluaran_☕.txt` |
| WIN-10 | Sumber UTF-8 dengan BOM (Notepad lama, "UTF-8 with BOM" di VS Code) | Berkas berisi BOM + `cetak("bom")` | `bom` tercetak, seperti Python |
| WIN-11 | Keluaran non-ASCII di konsol | Jalankan SIN-03 langsung di jendela PowerShell/cmd (bukan pipa) | Baris terakhir `2 ☕ GARUDA` terbaca benar |
| WIN-12 | Masukan non-ASCII dari PowerShell 5.1 | `'Andrés' \| indk skrip.idk` dengan `input()` | `panjang(x)` = 6 dan teks utuh |
| WIN-13 | Soket Winsock di localhost | OTO-03, OTO-06 | Lulus |
| WIN-14 | Proses anak | PUS-05 (`proses.jalankan([jalur_indk(), "-v"])`) | `0 benar` |
| WIN-15 | Uninstall | Jalankan `unins000.exe` | Folder dan entri `PATH` hilang; terminal baru tidak mengenali `indk` |
| WIN-16 | SmartScreen / Defender | Unduh dan jalankan installer dari peramban | Peringatan (jika ada) dicatat; tidak ada deteksi malware |

## 7. Hasil uji terakhir — `1.4.0`, 2026-11-03

### Ringkasan per bagian

| Bagian | Hasil |
|---|---|
| 3. CLI (CLI-01–CLI-14) | ✅ 14/14 |
| 4. Katalog bahasa (SIN, CAB, ULG, FNG, KOL, KLS, GAL, MOD, PUS, DB) | ✅ 26/26 |
| 5. Suite otomatis | ✅ OTO-01–OTO-07; ⚠️ OTO-08, OTO-09 (masalah skrip uji, bukan indk) |
| 6. Khusus Windows | ✅ 7 · ❌ 4 · ⚠️ 1 · ⬜ 4 |

### Rincian

| ID | Status | Catatan |
|---|---|---|
| CLI-01 … CLI-14 | ✅ | CLI-05 dan CLI-06 diuji lewat pipa Git Bash; CLI-13 menjawab `Versi Anda sudah yang terbaru.` |
| SIN-01 … DB-01 (26 kasus) | ✅ | Semua stdout, stderr, dan kode keluar sama dengan katalog |
| OTO-01 | ✅ | `TOTAL: 233/233 PASSED` (134 golden, 99 galat, lokasi impor) memakai `indk.exe` terpasang |
| OTO-02, OTO-03 | ✅ | |
| OTO-04 | ✅ | `LINTER: 5/5`, `UJI: 2/2`, `FORMAT: 7/7` (bytecode identik 162/162) |
| OTO-05 | ✅ | `198/198 dijalankan bersih`, 20 ditandai takjalan |
| OTO-06, OTO-07 | ✅ | |
| OTO-08 | ⚠️ | Satu kasus gagal: curl di Windows mengubah argumen `--data-binary '...☕ 👨‍👩‍👧'` ke code page ANSI sehingga yang terkirim sudah `?`. Badan yang sama dikirim dari berkas (`--data-binary @badan.json`) diterima utuh oleh server. Masalah skrip uji. |
| OTO-09 | ⚠️ | Keluaran sama persis kecuali akhir baris: `indk.exe` menulis `\r\n` dan skrip membandingkan teks mentah. Masalah skrip uji. |
| WIN-01 | ✅ | `indk` ditemukan di `PATH`; `pustaka\` dan `examples\` terpasang |
| WIN-02, WIN-03 | ⬜ | Belum diuji (perlu mengunduh aset rilis) |
| WIN-04 | ✅ | |
| WIN-05, WIN-06, WIN-07 | ✅ | |
| WIN-08 | ❌ | T-W1. `Tidak dapat membuka file "??/s.idk".`, kode 74, dari Git Bash maupun PowerShell |
| WIN-09 | ❌ | T-W1. Berkas tercipta sebagai `keluaran_â˜•.txt` |
| WIN-10 | ❌ | T-W2. `[baris 1] Kesalahan: Karakter tidak dikenal.`, kode 65 |
| WIN-11 | ❌ | T-W3. Di konsol code page 437 tercetak `2 Γÿò GARUDA`. Setelah `chcp 65001` benar |
| WIN-12 | ⚠️ | T-W4. Tanpa pengaturan, PowerShell 5.1 mengirim `Andr?s`. Dengan `$OutputEncoding = [System.Text.UTF8Encoding]::new($false)` teks utuh (`panjang 6`) |
| WIN-13, WIN-14 | ✅ | |
| WIN-15, WIN-16 | ⬜ | Belum diuji |

### Temuan

| Kode | Tingkat | Temuan | Penyebab dugaan | Jalan pintas sementara |
|---|---|---|---|---|
| T-W1 | Tinggi | Skrip di jalur dengan karakter di luar code page ANSI (huruf Jepang, emoji, dsb.) tidak bisa dibuka, dan nama berkas non-ASCII yang ditulis program menjadi mojibake. Pengguna dengan nama folder pengguna non-Latin tidak bisa menjalankan skrip dari folder pribadinya. | `main()` menerima `argv` ANSI dan berkas dibuka dengan `fopen` (byte UTF-8 ditafsirkan sebagai ANSI). Tidak ada `wmain`/`_wfopen`/`GetCommandLineW`. | Letakkan skrip di jalur ASCII |
| T-W2 | Sedang | Berkas sumber UTF-8 dengan BOM ditolak. Python menerimanya. | Pemindai tidak melewati BOM `EF BB BF` di awal berkas. | Simpan sebagai "UTF-8" tanpa BOM |
| T-W3 | Sedang | Teks non-ASCII yang dicetak ke konsol Windows berbahasa Inggris (code page 437/850) menjadi mojibake; keluaran yang ditangkap PowerShell ke variabel atau berkas juga rusak. | Program menulis byte UTF-8 tanpa `SetConsoleOutputCP(CP_UTF8)`. | `chcp 65001` sebelum menjalankan `indk` |
| T-W4 | Rendah | Teks non-ASCII yang dipipakan dari PowerShell 5.1 menjadi `?`. | Perilaku PowerShell 5.1 (`$OutputEncoding` bawaan ASCII), bukan indk; dicatat agar tidak dilaporkan ulang. | `$OutputEncoding = [System.Text.UTF8Encoding]::new($false)` atau PowerShell 7 |
| T-U1 | Rendah (semua OS) | `indk periksa` hanya melaporkan lokal yang dideklarasikan `variabel`; lokal implisit (`sisa = 5` di dalam fungsi, K3) yang tak pernah dipakai tidak dilaporkan. | Cakupan linter; uji `tests/lint/` hanya memakai `variabel`. | — |

Temuan ini belum diperbaiki. Perbaikannya mengikuti alur fitur biasa
(rencana di `docs/RENCANA_FITUR/`, uji regresi, lalu rilis patch) dan tidak
termasuk dokumen rencana uji ini.

### Uji ekstensif multi-agen

Di samping bagian 3–6, rilis 1.4.0 diuji secara luas oleh delapan agen
penguji paralel, satu per area. Setiap agen menulis dan menjalankan ratusan
program `.idk` terhadap `indk.exe` terpasang, lalu membandingkannya dengan
panduan v1.4.0 dan Python 3.14. Setiap temuan diulang oleh agen verifikator
yang berusaha membantahnya: **140 terkonfirmasi**, 4 ditolak sebagai salah
uji. Rincian lengkap (alasan, reproduksi minimal, keluaran) ada di
[`lampiran/WINDOWS-1.4.0-uji-ekstensif.md`](lampiran/WINDOWS-1.4.0-uji-ekstensif.md).

| Area | Pemeriksaan lulus | Temuan terkonfirmasi |
|---|---|---|
| Sintaks, angka, teks, operator | 252/292 | 23 |
| Percabangan, perulangan, fungsi | 239/262 | 14 |
| Koleksi | 319/357 | 12 |
| Kelas dan OOP | 236/262 | 22 |
| Galat, pesan, kode keluar | 243/270 | 18 |
| Modul pustaka standar (dibanding `re`, `argparse`, `csv`, `hashlib`, `statistics`, `json`, …) | 1770/1855 | 23 |
| Integrasi Windows: berkas, jalur, proses, soket | 284/312 | 13 |
| CLI, REPL, perkakas, SQLite | 241/269 | 15 |
| **Total** | **3584/3879 (92,4%)** | **140** (dengan duplikat antar-area) |

Pemeriksaan yang "gagal" belum tentu bug: sebagian adalah pemeriksaan yang
dibatalkan penguji sendiri setelah membaca ulang dokumentasi. Yang dihitung
sebagai bug hanya temuan terkonfirmasi.

**Temuan khas Windows** (tambahan T-W1–T-W4):

| Kode | Tingkat | Temuan |
|---|---|---|
| T-W5 | Sedang | Isi `argumen` dengan karakter di luar code page ANSI menjadi `?` (`"日本"` → `[63, 63]`); akar masalahnya sama dengan T-W1 (`main` ANSI, bukan `wmain`) |
| T-W6 | Tinggi | Temuan keamanan pada menjalankan proses anak. Rincian dilaporkan lewat jalur privat, tidak dicantumkan di sini sampai diperbaiki |
| T-W7 | Tinggi | Temuan keamanan pada soket server. Rincian dilaporkan lewat jalur privat, tidak dicantumkan di sini sampai diperbaiki |
| T-W8 | Tinggi | ✅ **Diperbaiki (1.4.1).** Kompilator crash (`0xC00000FD` stack overflow, tanpa pesan) pada 32 `fungsi` bersarang atau ±2500 kurung bersarang. Pengukuran ulang per bentuk: `fungsi` jatuh pada **16 level**, sedangkan kurung dan `jika` aman sampai 1000 — sebabnya frame `function()` sebesar 42.640 byte di stack, bukan rekursi parser umum. Struct `Compiler` dan larik parameter dipindah ke heap (frame turun ke 224 byte) dan ditambah penjaga `NESTING_MAX = 200` yang memberi galat kompilasi rapi |
| T-W9 | Sedang | `ada_berkas()` memberi `salah` untuk berkas yang ada tetapi sedang dikunci proses lain (memakai `fopen`, bukan `stat`); `jalur.ada()` benar |
| T-W10 | Sedang | Jalur lebih dari 260 karakter tidak didukung: `ada_berkas` salah, `baca_semua` dan `isi_direktori` gagal, walau `LongPathsEnabled=1` |
| T-W11 | Rendah | `jam()` mengukur waktu dinding di Windows, padahal dokumentasi menyebut waktu CPU (`clock()` di UCRT) |

**Temuan tingkat tinggi yang tidak khas Windows** (bug bahasa, kemungkinan
sama di macOS dan Linux). **Semuanya sudah diperbaiki.** Kolom Regresi menyebut
berkas yang menjaga tiap temuan; semuanya ikut `tests/test_all.sh` (307/307),
jadi temuan ini tidak bisa kembali tanpa membuat suite merah.

| Kode | Temuan | Status | Regresi |
|---|---|---|---|
| T-U2 | `x = a jika c selain b` tanpa `variabel` dikompilasi sebagai `(x = a) jika c selain b`: bila kondisinya salah, `x` tidak di-assign sama sekali | ✅ selesai | `examples/kondisional_assignment.idk` |
| T-U3 | Di dalam fungsi, panggilan dengan argumen bernama membuat argumen posisi yang berupa global/upvalue menjadi `kosong`: `g(D, b = 2)` → `[kosong, 2]` (di tingkat skrip benar `[7, 2]`) | ✅ selesai | `examples/argumen_bernama_lingkup.idk` |
| T-U4 | Variabel `untuk` berlingkup blok: variabel lama bernama sama tidak berubah setelah loop (`99`), dan nama itu tidak terdefinisi setelah loop — berbeda dengan Python | ✅ selesai | `examples/untuk_lingkup.idk` |
| T-U5 | `pastikan(kondisi, "pesan")` dengan kurung selalu lolos karena membentuk tupel yang selalu benar | ✅ selesai | `examples/pastikan.idk` + `tests/galat/pastikan_tupel.idk` |
| T-U6 | Subkelas buatan pengguna dari galat bawaan kehilangan leluhur di atas induk langsungnya, sehingga `kecuali Galat` tidak menangkapnya | ✅ selesai | `examples/galat_turunan_bawaan.idk` |
| T-U7 | Galat yang dinaikkan `_logika_` atau `_sama_` tertunda: cabang `jika` tetap dijalankan dan kamus tetap diubah sebelum galatnya muncul | ✅ selesai | `examples/oop/galat_ajaib_langsung.idk` |
| T-U8 | `impor "berkas.idk"` menulis global ke tabel builtin bersama dan mencemari modul bernamespace, termasuk pustaka standar (mis. `impor "pustaka/base64.idk"` lalu `impor json` merusak `json.json_teks`) | ✅ selesai | `examples/impor_berkas_namespace.idk` |
| T-U9 | Komprehensi diam-diam mengabaikan token setelah filter pertama (`jika` kedua, `untuk` kedua, token sampah) | ✅ selesai | `tests/galat/komprehensi_jika_kedua.idk` |
| T-U10 | `input()` dan REPL memotong baris pada 1023 byte; sisanya muncul pada pembacaan berikutnya | ✅ selesai | `tests/test_baris_panjang.sh` |
| T-U11 | `indk rapikan --tulis` memotong berkas pada byte NUL (kehilangan data) dan menulis ulang berkas bergalat sintaks dengan kode keluar 0 | ✅ selesai | `tests/galat/sumber_nul.idk` + `tests/test_format.sh` |
| T-U12 | `lantai`/`atap`/`bulat` dari `nan` diam-diam memberi `-9223372036854775808`; `bulatkan(nan/inf)` memberi desimal | ✅ selesai | `examples/nan_ke_bulat.idk` |
| T-U13 | `==` pada struktur bersarang lebih dari ±64 tingkat diam-diam `salah` (Python membandingkan atau melempar `RecursionError`) | ✅ selesai | `examples/kedalaman_sama_hash.idk` |
| T-U14 | Meng-hash tupel bersarang sangat dalam (kunci kamus, anggota himpunan) membuat interpreter crash | ✅ selesai | `tests/galat/hash_tupel_dalam.idk` |
| T-U15 | `json_urai` tidak mendekode `\uXXXX`, `\b`, `\f` dan menerima JSON tidak sah; `json_teks` bisa menghasilkan JSON tidak sah (karakter kontrol mentah, `takhingga`/`nan`) | ✅ selesai | `examples/json_ketat.idk` |
| T-U16 | `bita.ke_teks` menerima UTF-8 *overlong* (`C0 80` menjadi NUL) | ✅ selesai | `examples/bita_utf8_ketat.idk` |

Temuan tingkat sedang yang paling berdampak (lengkapnya di lampiran):
- Jenis galat tidak sesuai dokumentasi: nama tak terdefinisi bukan
  `GalatNama`; indeks teks/rentang bukan `GalatIndeks`; jumlah argumen salah
  bukan `GalatTipe`; galat berkas bukan `GalatOS`. Kunci kamus yang hilang
  diam-diam memberi `kosong`, bukan `GalatKunci`.
- `d[0] += 1` dan `diri.x += 1` ditolak kompilator.
- Membaca lokal sebelum di-assign memberi `kosong`, bukan galat.
- Hanya 64 blok `coba` aktif: rekursi dengan `coba` per frame gagal di
  kedalaman 64.
- Jejak galat tidak menyebut berkas modul, dan galat yang melewati
  `akhirnya` menunjuk `}` penutup.
- `jika x = 0 {` diterima dan meng-assign; `cetak "x"` diam-diam tidak
  melakukan apa-apa.
- Mengubah daftar/kamus saat `untuk` memberi hasil berbeda dari Python.
- Kinerja kuadratik: menghapus kunci kamus (100k penghapusan > 13 s),
  `daftar +=` (membuat daftar baru, alias tidak ikut berubah), pustaka yang
  membangun teks per karakter.
- Pustaka:
  - SQLite mengikat desimal bulat sebagai INTEGER dan menolak bulat di atas
    2⁵³;
  - `csv` salah menangani CR;
  - `web` tidak mengirim 413 ke klien;
  - `waktu()` hanya beresolusi 1 detik;
  - `opsi` keluar dengan kode 70 pada nilai bulat yang meluap.

## 8. Riwayat pengujian

| Tanggal | Versi | Paket | Penguji | Hasil |
|---|---|---|---|---|
| 2026-11-03 | 1.4.0 | Installer `Indonesiaku-Setup-1.4.0.exe` | Claude Code (AI), atas permintaan maintainer | ⚠️ bagian 3–6 lulus; temuan T-W1–T-W4, T-U1 |
| 2026-11-03 | 1.4.0 | Installer `Indonesiaku-Setup-1.4.0.exe` | Claude Code (AI, 8 agen penguji + 8 verifikator) | ⚠️ uji ekstensif 3584/3879; 140 temuan terkonfirmasi, termasuk T-W5–T-W11 dan T-U2–T-U16 |
