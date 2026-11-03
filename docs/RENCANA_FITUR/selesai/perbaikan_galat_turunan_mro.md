# Rencana Perbaikan: subkelas galat bawaan kehilangan leluhur (T-U6)

- **Status:** selesai
- **Target versi:** 1.4.1
- **Pemilik:** Teguh Rijanandi (dikerjakan oleh Claude Code)

## 1. Masalah

Temuan T-U6 (uji ekstensif Windows 1.4.0, `GALAT-02`): kelas buatan pengguna
yang menurun dari galat bawaan selain `Galat` hanya mengenal induk
langsungnya.

```indonesiaku
kelas B2 < GalatBagiNol { }
coba { naikkan B2() } kecuali GalatAritmetika sebagai e { cetak("tangkap") }
# 1.4.0: tidak tertangkap — 'Kesalahan tak tertangani.', exit 70
```

Penyebabnya ada di `c3Linearize` (`vm.c`). Fungsi itu menyalin `mro` setiap
induk, padahal kelas galat yang didefinisikan dari C tidak punya array `mro`:
rantai `superclass` mereka sudah merupakan MRO. Akibatnya
`L(GalatBagiNol)` kosong, dan MRO `B2` hanya `[B2, GalatBagiNol]`.

## 2. Sintaks usulan

Tidak ada sintaks baru.

## 3. Semantik

`issubclass` bersifat transitif, seperti di Python. MRO subkelas memuat
semua leluhur kelas bawaan, jadi `kecuali`, `adalah_instansi`, dan pencarian
metode melihat seluruh rantai.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/vm.c` | `c3Linearize` membangun `L(Bi)` lewat `ancestors()`, yang membaca `mro` bila ada dan rantai `superclass` bila tidak. |

## 5. Pengujian

- Golden `examples/galat_turunan_bawaan.idk`: turunan langsung, bertingkat,
  pewarisan ganda, dan leluhur tak terkait yang tidak boleh menangkap.
- Suite lengkap dijalankan, ditambah build ASan + UBSan + `-DDEBUG_STRESS_GC`.

## 6. Dampak dokumentasi & tooling

Perbaikan ke perilaku yang sudah didokumentasikan ("Gunakan kelas induk"),
jadi panduan tidak berubah. Entri ditambahkan ke `CHANGELOG.md`.

## 7. Risiko & alternatif

Alternatifnya, beri setiap kelas galat bawaan array `mro` saat
`defineErrorClass`. Pilihan itu tidak diambil: ia menambah alokasi untuk
setiap kelas bawaan, dan setiap kelas C baru harus mengingat aturan yang
sama.
