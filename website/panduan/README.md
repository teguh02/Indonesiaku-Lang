# Dokumentasi Bahasa Pemrograman Indonesiaku

Selamat datang di dokumentasi lengkap **Bahasa Pemrograman Indonesiaku** - Bahasa pemrograman yang menggunakan sintaks dan keyword dalam bahasa Indonesia.

---

## 📖 Daftar Isi

### 1. [Perkenalan](01_PERKENALAN.md)
Pengenalan umum tentang Bahasa Pemrograman Indonesiaku, sejarah, visi, dan misi.
- Apa itu Bahasa Pemrograman Indonesiaku?
- Mengapa dibuat?
- Fitur utama v1.0.0
- Roadmap pengembangan

### 2. [Panduan Instalasi](02_INSTALASI.md)
Panduan lengkap instalasi Bahasa Pemrograman Indonesiaku di berbagai sistem operasi.
- **Windows** - MinGW installation & setup
- **Linux** - GCC installation & setup (Ubuntu, Fedora, Arch)
- **macOS** - Xcode Command Line Tools setup
- Verifikasi instalasi & troubleshooting
- Build dari source

### 3. [Panduan Sintaks](03_PANDUAN_SINTAKS.md)
Panduan lengkap penggunaan sintaks dan grammar Bahasa Pemrograman Indonesiaku dengan contoh-contoh praktis.
- Tipe data (angka, string, boolean, kosong, list, kamus)
- Variabel & scope, operator, assignment gabungan
- Percabangan (`jika`/`jika_lain`/`selain`)
- Perulangan (`selagi`, `untuk...dalam`, `hentikan`/`lanjut`)
- Fungsi, rekursi, closure
- Kelas & OOP, exception, modul (`impor`)

### 4. [Panduan Kontribusi](04_KONTRIBUSI.md)
Panduan untuk berkontribusi dalam pengembangan Bahasa Pemrograman Indonesiaku.
- Setup development environment
- Struktur project
- Jenis-jenis kontribusi
- Git workflow & commit messages
- Code style guidelines
- Testing & quality assurance
- Pull request process

### 5. [Tentang Creator](05_CREATOR.md)
Informasi tentang pembuat dan maintainer Bahasa Pemrograman Indonesiaku.
- Profil creator: Teguh Rijanandi
- Motivasi & visi proyek
- Background teknis
- Project journey & achievements
- Philosophy on contribution
- Kontak & social media

### 6. [Penutup & Sumber Daya](06_PENUTUP.md)
Kesimpulan, tips, dan informasi tambahan.
- Key takeaways untuk berbagai level
- FAQ komprehensif
- Troubleshooting guide
- Sumber daya & external links
- Next steps
- Support & masukan channels

### 7. [Referensi Pustaka Standar](07_PUSTAKA_STANDAR.md)
Daftar lengkap fungsi bawaan dan pustaka standar.
- Fungsi bawaan: matematika, string, list, kamus, konversi, berkas, waktu, soket
- Pustaka `.idk`: `teks`, `daftar`, `matematika`, `kamus`, `json`, `http`
- Signature & deskripsi tiap fungsi

### 8. [Deployment](08_DEPLOYMENT.md)
Menaruh server Indonesiaku di internet tanpa membuat lubang.
- Batas yang harus disadari: satu permintaan pada satu waktu
- Risiko membuka port, termasuk Docker yang menembus `ufw`
- Dockerfile dua tahap, pengguna non-root, mengikat `0.0.0.0` di container
- Compose dengan isolasi proses dan batas resource
- Reverse proxy: batas laju, batas ukuran, timeout
- TLS dan pembaruan sertifikat
- Daftar periksa sebelum publik

---

## 🚀 Memulai Cepat

### Instalasi
```bash
# Clone repository
git clone <repository-url>

# Build
gcc -Wall -Wextra -std=c11 -O2 -Isrc -o indk.exe src/*.c -lm

# Run
./indk.exe example.idk
```

### Hello World
```indonesiaku
cetak("Halo, dunia!")
```

### Contoh Program Sederhana
```indonesiaku
# Variabel dan operasi
x = 10
y = 20
jumlah = x + y

cetak("Hasil penjumlahan:", jumlah)
```

---

## 📋 Spesifikasi Bahasa

| Aspek | Keterangan |
|-------|-----------|
| **Versi** | v1.0.0 |
| **Tipe** | Bytecode VM (interpreted) + garbage collector |
| **Syntax** | Kurung kurawal `{ }` dengan keyword Bahasa Indonesia |
| **Tipe Data** | Angka, boolean, string, kosong, list, kamus |
| **Platform** | Windows, Linux, macOS |
| **License** | MIT |

---

## 📚 Dokumentasi Tambahan

- **[07_PUSTAKA_STANDAR.md](07_PUSTAKA_STANDAR.md)** - Referensi fungsi bawaan & pustaka
- **[08_DEPLOYMENT.md](08_DEPLOYMENT.md)** - Docker, reverse proxy, TLS, isolasi
- **[CHANGELOG.md](../CHANGELOG.md)** - Riwayat perubahan tiap versi
- **[QUICKSTART.md](../QUICKSTART.md)** - Panduan cepat memulai
- **[examples/](../examples/)** - 50+ contoh program siap jalan

---

## 🔗 Links Penting

- **Repository**: https://github.com/teguh02/Indonesiaku-Lang
- **Issues**: Laporkan bug melalui GitHub Issues
- **Discussions**: Tanya jawab di GitHub Discussions

---

## 📞 Hubungi Kami

- **Email**: teguhrijanandi02@gmail.com
- **GitHub**: [@teguh02](https://github.com/teguh02)

---

## 📄 Lisensi
Bahasa Pemrograman Indonesiaku dirilis di bawah **MIT License**. Silakan lihat file [LICENSE](../LICENSE) untuk detail lengkap.

---

## 🙏 Terima Kasih

Terima kasih telah menggunakan Bahasa Pemrograman Indonesiaku. Kami sangat menghargai masukan, saran, dan kontribusi Anda!

---

**Maintainer**: Teguh Rijanandi  
**Version**: v1.0.0
