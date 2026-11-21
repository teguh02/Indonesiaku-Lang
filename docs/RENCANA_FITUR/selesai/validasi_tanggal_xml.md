# Rencana Fitur: Validasi input, tanggal/waktu ISO 8601 dan XML aman (Fase 19, V1)

- **Status:** selesai (V1: langkah 1, 3, sebagian 4, sebagian 5; sisa di §7)
- **Target versi:** 1.14.0 (kandidat Fase 19; V1 boleh dirilis lebih awal sebagai modul tambahan)
- **Pemilik:** maintainer Indonesiaku

## 1. Masalah

Audit lengkap ada di `docs/ANALISIS/KATALOG_API_DAN_GAP_WEB.md`. Tiga
kesenjangan yang dibuktikan dengan kode, bukan asumsi:

| Area | Hari ini (sebelum V1) | Akibatnya |
|---|---|---|
| Validasi formulir | `web.idk` mengurai `req["form"]` tetapi tidak memeriksanya; `konfigurasi.muat(skema)` hanya untuk konfigurasi dan berhenti di galat pertama | Setiap aplikasi menulis ulang pemeriksaan email/URL/angka, biasanya dengan regex yang menerima `javascript:` |
| Tanggal/waktu | `waktu()` (detik), `tanggal()` (komponen lokal saat ini), `format_tanggal(pola)` (strftime waktu lokal **saat ini** saja) | Tidak bisa mengurai `"2024-03-10T12:30:00+07:00"`, mengubah offset, atau menambah satu bulan |
| XML | Tidak ada. `grep -ri xml pustaka src` hanya menemukan tipe MIME dan atribut SVG | Tidak bisa membaca feed/RSS/SOAP/SEPA; parser buatan sendiri rawan XXE |

Temuan tambahan saat implementasi: `daftar(teks)` O(n²) untuk teks
non-ASCII (120 ribu karakter = 9 detik) karena `iterAt` memanggil
`utf8ByteOffset` per karakter.

## 2. Sintaks usulan (API)

Tiga modul bernamespace, ditulis dalam Indonesiaku murni. Tidak ada kata
kunci atau builtin publik baru.

```indonesiaku
impor validasi
h = validasi.periksa(req["form"], {
    "nama":  {"tipe": "teks", "wajib": benar, "panjang_maks": 50},
    "umur":  {"tipe": "bulat", "min": 17, "maks": 120},
    "surel": {"tipe": "email", "wajib": benar},
    "situs": {"tipe": "url", "skema_url": ["https"]}
})
jika tidak h.sah { kembalikan web.json(h.galat, 422) }

impor tanggal_waktu
w = tanggal_waktu.dari_iso("2024-03-10T12:30:00+07:00")
cetak(w.ke_utc(), w.tambah_bulan(1).iso(), w.format("%A, %d %B %Y"))

impor xml
akar = xml.urai(badan)                    # DTD/XXE selalu ditolak
cetak(akar.cari("judul").teks(), xml.bentuk(akar, benar, "  "))
```

Nama diperiksa terhadap `pustaka/`, builtin dan kata kunci: tidak bentrok.
`pastikan` adalah kata kunci, jadi "validasi atau lempar" bernama `sahkan`.

## 3. Semantik

**validasi**
- `adalah_email`, `adalah_url`, `adalah_bulat`, `adalah_desimal` tidak pernah
  melempar untuk masukan buruk; tipe salah → `salah`.
- `periksa(data, skema)` → `Hasil{sah, galat, nilai}`; galat dikumpulkan per
  field (daftar pesan). Teks kosong/spasi = tidak diisi. Field di luar skema
  tidak disalin ke `nilai` (mencegah mass assignment).
- Atribut skema: `tipe` (`teks bulat desimal logika daftar email url`),
  `wajib`, `bawaan`, `pilihan` (sama dengan `konfigurasi`), plus `min`,
  `maks`, `panjang_min`, `panjang_maks`, `pola`, `skema_url`, `pangkas`.
  Atribut/tipe tak dikenal → `GalatValidasi` saat itu juga (kesalahan program).
- `sahkan` = `periksa` + `GalatValidasi` dengan `.galat`.
- URL: skema dari daftar izin (bawaan `http`, `https`), `//` wajib,
  userinfo/spasi/kendali/backslash ditolak, port 1–65535, host domain/IPv4/[IPv6].
- Email: bentuk praktis, bukan RFC 5322 penuh (tanpa kutip/komentar/literal IP);
  huruf non-ASCII diterima di lokal dan domain.
- Validasi bukan escaping keluaran, parameter SQL, CSRF, atau autentikasi.

**tanggal_waktu**
- `TanggalWaktu` immutable, Gregorian proleptik, tahun 1–9999, mikrodetik.
- Offset tetap dalam menit (`+07:00` = 420) atau naif (`kosong`). Membandingkan
  atau mengurangkan naif dengan ber-offset → `GalatTanggal`; `==` campuran → `salah`.
- `dari_iso` menerima `YYYY-MM-DD` dan `YYYY-MM-DD[T| ]HH:MM[:SS[.f]][Z|±HH:MM|±HHMM|±HH]`.
  Ditolak: tanggal tak ada, jam 24, detik 60, offset ≥ 24 jam, digit non-ASCII.
- `iso()` menulis UTC sebagai `Z`; `format()` mendukung `%Y %m %d %H %M %S %f %z %j %u %A %a %B %b %%`
  dengan nama hari/bulan bahasa Indonesia.
- `tambah_bulan` menjepit tanggal ke akhir bulan.
- `offset_lokal()` hanya berlaku untuk saat ini (diturunkan dari `tanggal()` − `waktu()`).

**xml**
- `urai` → `Elemen` akar: `nama`, `atribut` (tanpa xmlns), `anak` (Elemen/teks,
  CDATA digabung ke teks), `ns`, `ns_dekl`. Komentar dan PI dibuang.
- DTD selalu ditolak; hanya 5 entitas bawaan + rujukan karakter yang sah.
- Batas: `BATAS_UKURAN` 1 MiB (bita UTF-8), `BATAS_KEDALAMAN` 256,
  `BATAS_ELEMEN` 100.000, `BATAS_ATRIBUT` 256 per elemen. Parser iteratif.
- Hanya UTF-8; BOM dibuang; `\r\n`/`\r` → `\n`; nilai atribut dinormalisasi (§3.3.3).
- Namespace: awalan harus dideklarasikan, `xml` terikat bawaan, duplikat nama
  diperluas ditolak, `xmlns:p=""` ditolak.
- `bentuk` meng-escape `& < >` (+ `"`, tab, LF, CR di atribut; CR di teks) dan
  menolak karakter kendali serta nama tidak sah.
- Semua galat: `GalatXML` dengan `.baris`/`.kolom`.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `pustaka/validasi.idk` | baru |
| `pustaka/tanggal_waktu.idk` | baru |
| `pustaka/xml.idk` | baru |
| `src/native.c` | `nativeFnDaftar`: jalur linear untuk teks (GC: daftar dan tiap elemen di-push selama `writeValueArray`) |

## 5. Pengujian

- Golden: `examples/pustaka_validasi.idk`, `pustaka_tanggal_waktu.idk`, `pustaka_xml.idk`.
- `tests/galat/`: `validasi_sahkan`, `tanggal_iso_tidak_sah`, `tanggal_offset_tidak_sah`,
  `xml_xxe_ditolak`, `xml_kedalaman`.
- Kasus wajib yang tercakup: XXE berkas & URL, parameter entity, billion
  laughs, entitas tanpa DTD, `<!doctype>` huruf kecil, 10.000 tingkat
  bersarang, 3.000 tingkat dengan batas dinaikkan (tidak kehabisan frame),
  batas elemen/atribut/ukuran, rujukan karakter di luar Unicode/surrogate/NUL,
  tanggal 29 Feb, bulan 13, jam 24, detik 60, offset +24:00/+07:60, digit
  full-width, teks multibahasa (Jepang, Rusia, aksen Latin) di email, URL, XML.
- `-DDEBUG_STRESS_GC`: ketiga golden + `tests/galat` dijalankan terhadap biner stres.
- Kinerja terukur (Windows, -O2): `urai` ±0,5–0,9 MB/detik, `bentuk` ±3,5 MB/detik.
- Tinjauan independen sebelum merge menemukan dan memperbaiki: penggabungan
  teks O(n²) pada teks/CDATA/komentar berselang-seling (kini potongan
  disatukan sekali per elemen), galat mentah `ke_bulat`/luapan dari masukan
  penyerang di `validasi` dan `tanggal_waktu`, spasi/format Unicode dan
  IP samaran (`0x7f000001`, `01.01.01.01`, IPv6 longgar) di URL/email,
  U+FFFE/FFFF dan karakter kendali di komentar/PI, pengikatan ulang namespace
  `xml`/`xmlns`, serta suntikan markup lewat bidang `Elemen` yang diubah
  langsung (nama kini diperiksa ulang di `bentuk()`). Semua kasus menjadi
  regresi di ketiga golden (`== regresi`, `== luapan`).

## 6. Dampak dokumentasi & tooling

- [x] `website/panduan/07_PUSTAKA_STANDAR.md` §2.30–2.32
- [x] `editors/vscode-indonesiaku/` — tidak perlu: tidak ada builtin/kata kunci publik baru
- [x] `CHANGELOG.md` + `docs/ROADMAP.md`
- [x] `docs/TESTING/{WINDOWS,MACOS,LINUX}.md` (VAL-01, TGL-01, XML-01, identik di blok BERSAMA)
- [x] `docs/ANALISIS/KATALOG_API_DAN_GAP_WEB.md`
- [x] Berkas rencana ini di `docs/RENCANA_FITUR/selesai/`

## 7. Risiko, keputusan & sisa pekerjaan

- **Zona waktu IANA/DST ditunda.** Mem-vendor tzdb = pembaruan rutin dan
  format data lintas OS. Opsi berikutnya: (a) vendor `tzdata` ringkas yang
  dihasilkan skrip `tools/` dengan pengecekan versi di CI, atau (b) membaca
  `/usr/share/zoneinfo` (POSIX) dan API Windows, dengan perilaku yang
  berbeda per OS. Keputusan menunggu kebutuhan nyata di luar WIB/WITA/WIT.
- **XML V2:** streaming/SAX untuk dokumen besar, XPath sederhana, komentar/PI
  sebagai simpul, encoding non-UTF-8, akses atribut per namespace.
- **Kinerja XML** dibatasi interpretasi per karakter; parser C (mis. mem-vendor
  parser yang terawat dengan DTD dimatikan) dipertimbangkan bila dokumen
  > beberapa MB menjadi kasus umum.
- **Kompatibilitas:** modul baru, tidak ada perubahan perilaku yang ada.
  Perubahan `daftar(teks)` hanya kinerja (hasil identik; diuji goldens).
