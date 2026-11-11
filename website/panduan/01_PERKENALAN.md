# 1. Perkenalan - Bahasa Pemrograman Indonesiaku

## Apa itu Bahasa Pemrograman Indonesiaku?

**Bahasa Pemrograman Indonesiaku** adalah bahasa pemrograman yang dirancang khusus untuk memberikan pengalaman pemrograman dalam bahasa Indonesia. Dengan menggunakan keywords dan sintaks yang familiar dalam bahasa Indonesia, bahasa ini memudahkan pemula lokal untuk belajar pemrograman tanpa hambatan bahasa.

---

## Visi & Misi

### Visi
Menjadi bahasa pemrograman pertama yang fully mendukung pemrograman dalam Bahasa Indonesia, membuka akses pendidikan programming untuk jutaan pelajar Indonesia.

### Misi
1. Mendemokratisasi akses ke pemrograman melalui bahasa lokal
2. Menciptakan ekosistem pembelajaran programming yang inklusif
3. Membuktikan bahwa bahasa pemrograman dapat didesain dengan keywords non-Inggris
4. Mendukung perkembangan talent engineering di Indonesia

---

## Fitur Utama

### ✅ Tersedia saat ini (v1.7.0)

**Tipe Data**
- ✅ Angka (integer & float, direpresentasikan sebagai `double`)
- ✅ Boolean (`benar`, `salah`)
- ✅ String (dengan escape `\n \t \r \" \\`) dan **f-string** (`f"nilai {x}"`)
- ✅ `kosong` (null)
- ✅ **List** (`[1, 2, 3]`) + pengindeksan baca/tulis + **slicing** (`list[a:b]`)
- ✅ **Kamus/dictionary** — literal `{ }` maupun `kamus()`

**Kontrol Program**
- ✅ Variabel global & lokal (`variabel` untuk lokal/closure)
- ✅ `jika` / `jika_lain` / `selain`
- ✅ `selagi` (while) dan **`untuk ... dalam`** (for-in)
- ✅ **`hentikan` / `lanjut`** (break/continue)
- ✅ Fungsi, rekursi, dan **closure** (menangkap variabel)
- ✅ **Argumen bawaan & bernama** (`fungsi f(a, b=5)`, dipanggil `f(1, b=2)`)
- ✅ **Kelas & OOP**: `kelas`, `init`, `diri`, pewarisan (`<`), `super`
- ✅ **Penanganan kesalahan**: `naikkan`, `coba` / `kecuali` / `akhirnya`
- ✅ **Generator** dengan `hasilkan`
- ✅ **Comprehension** (`(x*2 untuk x dalam rentang(3))`)
- ✅ **Anotasi tipe opsional** (`fungsi f(a: angka) -> angka`)
- ✅ **Modul**: `impor "file.idk"`, setiap modul punya global sendiri

**Operator**
- ✅ Aritmatika: `+`, `-`, `*`, `/`, `%`, `**`
- ✅ Assignment gabungan: `+=`, `-=`, `*=`, `/=`, `%=`, `**=`
- ✅ Perbandingan: `==`, `!=`, `<`, `>`, `<=`, `>=` (juga membandingkan string)
- ✅ Logika: `dan`, `atau`, `tidak`; keanggotaan: **`dalam`**

**Pustaka Standar** (27 modul, lengkap di [07_PUSTAKA_STANDAR.md](07_PUSTAKA_STANDAR.md))
- ✅ Dasar: `teks`, `daftar`, `kamus`, `matematika`, `statistik`, `iterasi`, `fungsional`
- ✅ Data: `json`, `csv`, `base64`, `bita`, `pola` (regex), `koleksi`
- ✅ Sistem: `jalur`, `proses`, `log`, `salin`, `opsi`
- ✅ Web: `http` (klien), `web` (server HTTP, routing, kuki, sesi, CSRF, CORS, auth), `templat` (mesin templat HTML)
- ✅ Database: `db` (SQLite), `pg` (PostgreSQL), `my` (MySQL), `migrasi`

**Perkakas**
- ✅ `indk periksa` (linter), `indk rapikan` (formatter), `indk uji` (test runner), `indk --dis` (disassembler)
- ✅ **Ekstensi VS Code resmi** dengan LSP: diagnostics, completion, hover, go-to-definition
- ✅ Mode `--sandbox` dan `indk --periksa-pembaruan`

**Infrastruktur**
- ✅ Garbage collector mark-sweep
- ✅ Pustaka standar ditulis dalam bahasa sendiri (`pustaka/`)
- ✅ **API embedding C** (`src/indk.h`) — menanam interpreter di aplikasi C
- ✅ Paket siap pasang per sistem operasi (installer Windows, `.deb`, `.zip`, `.tar.gz`, Docker)

Riwayat lengkap tiap rilis ada di
[CHANGELOG](https://github.com/teguh02/Indonesiaku-Lang/blob/main/CHANGELOG.md).

---

## Peta Jalan Pengembangan

Seluruh fitur inti sudah terimplementasi dan stabil: list, kamus, kelas/OOP,
exception, modul, GC, File I/O, soket, generator, comprehension, anotasi tipe,
server web, serta driver SQLite/PostgreSQL/MySQL.

Kandidat pekerjaan berikutnya:

- 🔮 Package manager dan ekosistem pustaka pihak ketiga
- 🔮 Kompilasi AOT dan optimasi performa VM
- 🔮 Dukungan GUI

Rencana lengkapnya ada di
[ROADMAP](https://github.com/teguh02/Indonesiaku-Lang/blob/main/docs/ROADMAP.md).

---

## Teknologi di Balik Layar

### Arsitektur
```
Source Code (.idk)
     ↓
  Scanner (Lexical Analysis)
     ↓
  Compiler single-pass (Pratt parser → Bytecode langsung, tanpa AST)
     ↓
  Virtual Machine berbasis stack (+ garbage collector mark-sweep)
     ↓
  Output
```

### Tech Stack
- **Language**: C11
- **Build**: GCC
- **Architecture**: Stack-based bytecode VM
- **Optimization**: -O2 flag

### Kinerja
- Lightning fast compilation
- Efficient bytecode execution
- Memory safe (no buffer overflows)
- Suitable for educational purposes

---

## Mengapa Bahasa Indonesia?

### Keuntungan
1. **Aksesibilitas** - Pemula tidak perlu belajar Inggris untuk coding
2. **Inklusi** - Membuka peluang untuk 270 juta penutur Bahasa Indonesia
3. **Pembelajaran** - Fokus pada concept, bukan language barrier
4. **Kebanggaan** - Programming dalam bahasa sendiri
5. **Eksperimen** - Proving bahwa non-English programming languages work

### Tantangan
1. **Community** - Ecosystem yang lebih kecil dibanding Python/JavaScript
2. **Library** - Fewer third-party libraries available
3. **Documentation** - Lebih sedikit resources online
4. **Performance** - Trade-off untuk accessibility

---

## Contoh Penggunaan

### Program 1: Fibonacci Recursion
```indonesiaku
fungsi fibonacci(n) {
    jika n <= 1 {
        kembalikan n
    }
    kembalikan fibonacci(n - 1) + fibonacci(n - 2)
}

cetak("Fibonacci ke-10:", fibonacci(10))
```
> Contoh lengkap yang bisa dijalankan: [`examples/fibonacci.idk`](../examples/fibonacci.idk)

{{embed:examples/fibonacci.idk}}

### Program 2: Kontrol Program
```indonesiaku
x = 1
selagi x <= 5 {
    jika x % 2 == 0 {
        cetak("Genap:", x)
    } selain {
        cetak("Ganjil:", x)
    }
    x = x + 1
}
```

### Program 3: Fungsi dengan Parameter
```indonesiaku
fungsi hitung_luas(panjang, lebar) {
    kembalikan panjang * lebar
}

cetak("Luas:", hitung_luas(10, 5))
```

> Kumpulan contoh lengkap ada di folder [`examples/`](../examples/) — mis.
> `list.idk`, `kamus.idk`, `oop/kelas.idk`, `coba_kecuali.idk`, `todo_cli.idk`,
> `todo_server.idk`.

---

## Statistik Project

| Metrik | Nilai |
|--------|-------|
| Baris kode (`src/`, tanpa SQLite bawaan) | ~20,9K |
| Jumlah opcode VM | 87 |
| Contoh program (`examples/`) | 182 |
| Golden test | 171 |
| Tes kesalahan (`tests/galat/`) | 116 |
| Pustaka standar (`pustaka/`) | 27 modul — teks, daftar, matematika, kamus, json, http, web, templat, db, pg, my, csv, pola, proses, dan lainnya |

---

## Perbandingan dengan Bahasa Lain

| Feature | Indonesiaku | Python | JavaScript |
|---------|-------------|--------|------------|
| Syntax Bahasa Indonesia | ✅ Ya | ❌ Tidak | ❌ Tidak |
| Easy to Learn | ✅ Ya | ✅ Ya | ⚠️ Medium |
| Performance | ⚠️ Medium | ⚠️ Medium | ✅ Good |
| Community | ⚠️ Kecil | ✅ Besar | ✅ Besar |
| Libraries | ⚠️ Fokus (27 modul inti) | ✅ Banyak | ✅ Banyak |
| Educational | ✅ Sempurna | ✅ Sempurna | ✅ Baik |

---

## Siapa yang Cocok Menggunakan?

### ✅ Cocok Untuk:
- Pelajar Indonesia yang baru belajar programming
- Sekolah/universitas yang mengajar programming
- Proyek pendidikan lokal
- Penelitian tentang programming languages
- Aplikasi web skala kecil–menengah: REST API, SSR dengan mesin templat,
  autentikasi, sesi, dan CSRF (lihat [08_DEPLOYMENT.md](08_DEPLOYMENT.md))
- Otomasi dan skrip: berkas, proses, CSV/JSON, HTTP, regex
- Aplikasi berbasis database: SQLite, PostgreSQL, dan MySQL
- Menanam interpreter di aplikasi C lewat API embedding
  (lihat [09_EMBEDDING.md](09_EMBEDDING.md))

### ⚠️ Perlu Dipertimbangkan:
- **Komputasi berat dan latensi sangat rendah** — Indonesiaku adalah VM
  bytecode, bukan bahasa terkompilasi. Untuk beban kerja yang menuntut
  performa maksimal, bahasa seperti Go, Rust, atau C lebih tepat.
- **Ekosistem pustaka pihak ketiga** — tersedia 27 modul pustaka standar
  yang menutupi kebutuhan umum, tetapi belum ada package manager dan
  ekosistem sebesar Python atau JavaScript.
- **Komunitas dan materi belajar** — masih berkembang, sehingga mencari
  jawaban atas masalah yang tidak umum butuh usaha lebih.

---

## Lisensi & Attribution

Bahasa Pemrograman Indonesiaku dirilis di bawah **MIT License**, yang berarti:
- ✅ Gratis digunakan untuk keperluan komersial dan non-komersial
- ✅ Boleh dimodifikasi dan didistribusikan
- ✅ Harus menyertakan attribution

---

## Kontribusi & Dukungan
Kami sangat menerima:
- 🐛 Bug reports
- 💡 Feature suggestions
- 📝 Documentation perbaikan
- 🔧 Code contributions
- 📢 Word of mouth support

Lihat file **KONTRIBUSI.md** untuk detail lengkap.

---

## Sumber Daya

- **GitHub Repository**: https://github.com/teguh02/Indonesiaku-Lang
- **Issue Tracker**: https://github.com/teguh02/Indonesiaku-Lang/issues
- **Documentation**: `website/panduan/` folder
- **Contoh**: `/examples` folder

---

**Status**: Aktif dikembangkan  
**Latest Version**: v1.0.0  
**Maintainer**: Teguh Rijanandi
