# Analisis Indonesiaku vs Python

Hasil audit menyeluruh (2026-10-24) terhadap Indonesiaku v1.0.0 dan
pemetaan lengkap fitur Python 3.12 yang akan ditiru. Urutan pengerjaan
resmi **tidak** ada di sini, melainkan di [`../ROADMAP.md`](../ROADMAP.md);
folder ini berisi alasan dan rinciannya.

| Berkas | Isi | Baca bila… |
|---|---|---|
| [`01_KONDISI_SAAT_INI.md`](01_KONDISI_SAAT_INI.md) | Arsitektur, fitur yang terbukti jalan, 15 bug (B1–B15), perbedaan semantik dengan Python | ingin tahu seberapa stabil bahasa ini sekarang |
| [`02_PETA_FITUR_PYTHON.md`](02_PETA_FITUR_PYTHON.md) | Kata kunci, sintaks, tipe, 71 fungsi bawaan, OOP, modul, perkakas → status & fase | ingin melihat gambaran besar "Python versi Indonesia" |
| [`03_KEPUTUSAN_DESAIN.md`](03_KEPUTUSAN_DESAIN.md) | 14 keputusan semantik (K1–K14) yang harus dikunci sebelum 1.0 | akan mengubah perilaku bahasa |
| [`04_TEKNIK_LANJUTAN.md`](04_TEKNIK_LANJUTAN.md) | Tangga teknik implementasi bahasa: AST, NaN-boxing, bignum, model objek, generator, GC generasional, JIT, WASM, FFI | merancang perubahan VM/compiler |
| [`05_OPEN_SOURCE.md`](05_OPEN_SOURCE.md) | Tata kelola, keamanan rantai pasok, proses RFC, pendidikan sebagai pembeda | mengelola proyek & komunitas |
| [`06_KATALOG_TIPE_BAWAAN.md`](06_KATALOG_TIPE_BAWAAN.md) | Setiap metode `str`/`list`/`dict`/`set`/`tuple`/angka, ±65 kelas pengecualian, ±80 metode ajaib → nama Indonesia | mengimplementasikan metode/kelas bawaan |
| [`07_KATALOG_PUSTAKA_STANDAR.md`](07_KATALOG_PUSTAKA_STANDAR.md) | ±435 fungsi/kelas dari modul stdlib Python (`math`, `random`, `os`, `collections`, `re`, `datetime`, `turtle`, …) → modul Indonesia | menulis pustaka standar |
| [`08_KEPUTUSAN_TLS.md`](08_KEPUTUSAN_TLS.md) | Perbandingan terukur OpenSSL vs mbedTLS untuk HTTPS (ukuran biner, kemungkinan vendoring, bukti verifikasi sertifikat), rekomendasi + yang dikorbankan | memutuskan Fase 11 langkah 2 (HTTPS) |

## Kesimpulan utama

1. **Fondasi sudah benar**, infrastruktur uji sangat baik, dan kinerja
   setara CPython untuk skrip sederhana.
2. ~~**Belum stabil**: `//` dan argumen bernama memberi jawaban salah tanpa
   error; rekursi dibatasi 64; sebagian besar error tidak bisa ditangkap.~~
   → **Fase 0 selesai.** `//` membulatkan ke bawah, argumen bernama bekerja
   (K16), batas rekursi `FRAMES_MAX` 1000, dan semua galat runtime bisa
   ditangkap `kecuali`.
3. **Semantik perlu diselaraskan dengan Python sekarang** (truthiness,
   bulat vs desimal, scope lokal), selagi pengguna masih sedikit. Setelah
   1.0 perubahan seperti ini tidak mungkin lagi.
4. **Pertahankan kurung kurawal** `{ }` untuk seri 1.x (K4).
5. Jalur ke "replika Python": Fase 0 → 1 → 2 → 3 → **1.0** → OOP lengkap →
   generator → stdlib luas → perkakas/kinerja → ekosistem.

## Cara audit dilakukan

- Build `mingw32-make`, 51/51 tes golden lulus.
- ±80 skrip uji coba kecil yang membandingkan keluaran Indonesiaku dengan
  perilaku Python 3 (aritmetika, string, koleksi, scope, error, OOP,
  rekursi, kinerja). Hasil per kasus dirangkum di 01 §3–§5.
- Membaca `scanner.c`, `compiler.c` (tabel aturan Pratt, `binary`,
  `printStatement`), `vm.c` (`runtimeError`, impor), `value.h`, `object.h`,
  `chunk.h`, daftar builtin di `initVM`, dan `pustaka/*.idk`.
