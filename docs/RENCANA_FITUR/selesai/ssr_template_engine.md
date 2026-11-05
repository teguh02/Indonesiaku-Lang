# Rencana Fitur: mesin templat HTML server-side (SSR-01…04, SSR-06)

- **Status:** selesai (SSR-01…04 + SSR-06; SSR-05, 07–10 di luar lingkup rencana ini)
- **Target versi:** 1.7.0
- **Pemilik:** teguh02

Fase 12 pada `docs/ROADMAP.md` memuat sepuluh langkah (SSR-01…SSR-10). Rencana
ini hanya mencakup **inti mesin templat**: loader/parser, interpolasi, kondisi
dan perulangan, serta auto-escaping. Include/inheritance (SSR-05), integrasi
respons web (SSR-07), cache (SSR-08), dan contoh aplikasi (SSR-10) menyusul
sebagai rencana dan PR sendiri, sesuai aturan "satu temuan = satu PR".

## 1. Masalah

Hari ini satu-satunya cara menghasilkan HTML dinamis adalah merangkai string
di dalam `.idk`:

```python
impor web
variabel baris = ""
untuk s dalam siswa {
    baris = baris + "<tr><td>" + s["nama"] + "</td></tr>"
}
kembalikan web.html("<table>" + baris + "</table>")
```

Tiga masalah nyata, bukan soal selera:

1. **XSS secara bawaan.** `s["nama"]` berisi `<script>` langsung tertanam.
   Tidak ada satu pun titik di jalur ini yang meng-escape.
2. **Kuadratik.** `baris = baris + ...` di dalam loop menyalin seluruh string
   tiap iterasi — pola yang sudah pernah membuat server ini DoS pada body
   40 KB (lihat catatan `web_badan_performa.sh`).
3. **Tampilan bercampur logika.** Tidak ada cara memberi berkas `.html` kepada
   orang yang bukan pemrogram.

## 2. Sintaks usulan

Bukan sintaks bahasa — pustaka baru `pustaka/templat.idk`. Berkas templat
adalah `.html` biasa dengan dua penanda:

```html
<h1>{{ judul }}</h1>
<ul>
{% untuk s dalam siswa %}
  <li>{{ s.nama }} — {{ s.kelas }}</li>
{% akhiruntuk %}
</ul>
{% jika kosongkah %}<p>Tidak ada data.</p>{% akhirjika %}
```

```python
impor templat
variabel mesin = templat.mesin("tampilan")          # root templat
cetak(mesin.render("siswa.html", {"judul": "Daftar", "siswa": daftar}))
cetak(templat.render_teks("<p>{{ a }}</p>", {"a": "<b>"}))
```

Nama tag memakai kata kunci bahasa yang sudah ada (`jika`, `selain`, `untuk`)
dengan penutup eksplisit `akhirjika` / `akhiruntuk`. Penutup eksplisit dipilih
karena HTML tidak punya `}` yang bisa dipinjam, dan karena pesan galat
"`{% akhiruntuk %}` tidak ditemukan" jauh lebih jelas daripada EOF mendadak.

## 3. Semantik

- **`{{ ekspresi }}` BUKAN kode `.idk` arbitrer** (SSR-03). Yang diterima hanya
  rantai akses: `nama`, `nama.kunci`, `nama.kunci.lain`, `nama[0]`. Tidak ada
  pemanggilan fungsi, tidak ada operator, tidak ada `impor`. Mesin templat
  tidak boleh menjadi jalan masuk eksekusi kode dari berkas `.html`.
- **Auto-escaping sejak interpolasi pertama** (SSR-06). `{{ x }}` selalu
  di-escape (`&`, `<`, `>`, `"`, `'`). Konten HTML tepercaya hanya lewat API
  eksplisit `templat.aman(teks)`, yang namanya sengaja mencolok.
- **Batas escaping yang diakui terus terang.** Escaping ini benar untuk isi
  elemen dan untuk nilai atribut ber-kutip (`'` ikut di-escape, jadi kutip
  tunggal maupun ganda aman). Ia **tidak** cukup untuk atribut tanpa kutip,
  untuk URL (`href="{{ u }}"` masih bisa `javascript:`), dan untuk ekspresi di
  dalam `<script>` atau `<style>`. Konteks-konteks itu butuh escaper sendiri;
  sampai ada, panduan menyatakan batas ini eksplisit alih-alih menyiratkan
  perlindungan yang tidak diberikan.
- Nilai `kosong` dirender sebagai string kosong; kunci yang **tidak ada**
  menaikkan `GalatTemplat` dengan path, baris, dan kolom — bukan diam-diam
  kosong, karena typo nama variabel adalah kesalahan paling umum.
- `{% untuk x dalam daftar %}` membuat `x` hanya di dalam blok itu; nama yang
  sama di luar tidak tersentuh.
- Semua galat memakai `nativeRaise`-style (`naikkan GalatTemplat`) sehingga
  bisa ditangkap `coba/kecuali` — server tidak boleh mati karena satu templat.
- Batas: ukuran sumber, kedalaman nesting, dan jumlah tag dibatasi
  (SSR-02) supaya templat berbahaya tidak menghabiskan memori.

## 4. Rencana implementasi

Murni `.idk`; tidak ada perubahan C. Profil akan menentukan apakah ini perlu
dipindah ke native nanti — jangan menebak sebelum diukur.

| Bagian | Perubahan |
|---|---|
| `pustaka/templat.idk` | baru: tokenizer, parser ke pohon node, renderer, escaping |
| `src/*.c` | **tidak ada** |

Dua jebakan yang sudah diketahui di repo ini dan wajib dihindari sejak awal:

- **Jangan `hasil = hasil + bagian` di dalam loop renderer.** Kumpulkan ke
  `daftar()` lalu `teks_gabung()` (builtin C). Pola lama sudah pernah membuat
  server DoS; renderer templat berada persis di jalur input eksternal.
- **Jangan menamai berkas uji sama dengan modul pustaka.** `impor templat` di
  dalam berkas bernama `templat.idk` akan menemukan dirinya sendiri dan mati
  dengan exit 70 tanpa pesan.

## 5. Pengujian

- Golden: `examples/templat_dasar.idk` + `tests/expected/templat_dasar.out`.
- Galat: `tests/galat/templat_var_hilang.{idk,err}`,
  `tests/galat/templat_tag_tak_tertutup.{idk,err}`.
- `tests/templat_sabotase.sh`: tiap jaminan dilucuti satu per satu dan WAJIB
  membuat tes merah — auto-escaping, penolakan pemanggilan fungsi di `{{ }}`,
  galat variabel hilang, pasangan tag, batas kedalaman.
- Kasus batas wajib: payload XSS (`<script>`, `" onload=`, `'`), Unicode
  (`é`, `世界`, emoji), nesting `untuk` di dalam `jika`, templat tidak ada,
  sumber kosong, `{{` tanpa `}}`, dan performa (2× ukuran → ~2× waktu,
  bukan 4×).
- Didaftarkan di `tests/test_all.sh` **dan** punya jalur yang ikut jalan di
  `tests/test_all.ps1` — sub-suite `.sh` baru otomatis tidak terpakai di
  Windows kalau tidak diurus, dan suite tetap hijau secara menipu.

## 6. Dampak dokumentasi & tooling

- [ ] `website/panduan/07_PUSTAKA_STANDAR.md` — bagian `templat`
- [ ] `CHANGELOG.md` + `docs/ROADMAP.md` (SSR-01…04, SSR-06 → ✅)
- [ ] `docs/TESTING/{WINDOWS,MACOS,LINUX}.md` — kasus uji baru
- [ ] Berkas rencana ini → `docs/RENCANA_FITUR/selesai/`

Tidak ada kata kunci bahasa baru, jadi `editors/vscode-indonesiaku/` tidak
tersentuh pada tahap ini.

## 7. Risiko & alternatif

- **Risiko utama: `{{ }}` tumbuh menjadi bahasa kedua.** Mitigasi: batasi pada
  rantai akses sejak awal dan tolak sisanya dengan galat tegas. Melonggarkan
  nanti mudah; memperketat setelah ada pengguna tidak mungkin.
- **Alternatif ditolak — mengevaluasi `{{ }}` lewat kompilator `.idk`.** Jauh
  lebih sedikit kode, tetapi menjadikan setiap berkas `.html` sebagai titik
  eksekusi kode arbitrer. ROADMAP melarangnya secara eksplisit (SSR-03).
- **Alternatif ditunda — engine native C.** Hanya kalau `indk profil` pada
  beban nyata menunjukkan rendering sebagai hotspot. Mengukur dulu, menulis C
  kemudian.
