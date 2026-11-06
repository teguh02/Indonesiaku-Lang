# Rencana Fitur: cache parsing templat (SSR-08)

- **Status:** dikerjakan
- **Target versi:** 1.7.0
- **Pemilik:** teguh02

## 1. Masalah

`Mesin.render()` memanggil `urai()` setiap kali dipanggil. Pada server, satu
templat yang sama diurai ulang untuk **setiap permintaan HTTP**.

Diukur dengan `indk profil` pada halaman daftar 20 baris yang memakai
`{% perluas %}` + `{% untuk %}`:

```text
1 render    :    40.219 instruksi
101 render  : 3.971.319 instruksi
per render  :    39.311 instruksi
100x urai() : 2.898.385 instruksi -> ~28.983 per render
porsi urai  : 73,7% dari tiap render
```

Jadi hampir tiga perempat biaya render adalah parsing ulang berkas yang tidak
berubah. Karena gelung server melayani satu permintaan pada satu waktu, biaya
ini langsung menjadi latensi setiap pengunjung.

`kelas Mesin` sudah punya `diri.cache = kamus()` sejak SSR-01, tetapi **tidak
pernah dibaca atau ditulis** — field mati sejak ditulis.

## 2. Sintaks usulan

Tidak ada sintaks baru. API tetap:

```python
variabel m = templat.mesin("tampilan")
m.render("halaman.html", data)
```

Dua tambahan opsional, keduanya tidak mengubah kode yang sudah ada:

```python
templat.mesin(akar, cache = benar)   # cache = salah untuk mematikan
m.kosongkan_cache()                  # buang semua entri
```

## 3. Semantik

- Cache memetakan **nama templat** ke hasil `urai()`.
- Entri dianggap basi bila `waktu_ubah` ATAU `ukuran` berkasnya berubah;
  keduanya diambil dari `_info_jalur()` yang sudah ada. Satu `stat` per render
  jauh lebih murah daripada ~29.000 instruksi parsing, jadi pemeriksaan
  dilakukan **selalu** — tidak ada "mode development" yang harus dinyalakan,
  dan tidak ada mode yang bisa lupa dimatikan di produksi.
- Hasil render dengan cache **wajib identik** dengan tanpa cache.
- Berkas yang hilang tetap menaikkan `GalatTemplat`; entri basi dibuang.
- Batas memori: maksimum 64 entri. Saat penuh, seluruh cache dikosongkan.
- `render_teks()` tidak memakai cache: sumbernya string, bukan berkas.
- Galat yang sama tetap bisa ditangkap `coba/kecuali` seperti sebelumnya.

## 4. Rencana implementasi

1. `_t_cap(jalur)` — ambil `(ukuran, waktu_ubah)` dari `_info_jalur()`.
2. `Mesin._node(nama)` — satu-satunya jalan mengambil node terurai; membaca
   cache, memvalidasi cap, mengurai saat perlu.
3. `_render_berkas()` memanggil `_node()` alih-alih `urai(_baca(...))`.
4. `init(akar, cache = benar)` + `kosongkan_cache()`.

## 5. Rencana uji

- `tests/templat_cache.sh`: hasil dengan cache == tanpa cache (byte-identik);
  berkas diubah di tengah jalan → render ikut berubah; berkas dihapus → galat;
  batas 64 entri tidak tumbuh tanpa batas; `{% sertakan %}` dan
  `{% perluas %}` ikut ter-cache.
- Pengukuran instruksi: render ke-2 harus jauh lebih murah dari render ke-1.
- Sabotase tiap pagar sampai merah.
