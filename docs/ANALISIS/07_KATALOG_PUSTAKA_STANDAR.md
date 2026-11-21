# 07 — Katalog Lengkap: Modul Pustaka Standar Python → Indonesiaku

Untuk setiap modul Python yang relevan: nama modul Indonesia, fungsi dan
kelas helper yang akan diduplikasi, apa yang sudah ada, dan cara
implementasinya (**C** = native di `src/`, **idk** = ditulis dalam
Indonesiaku di `pustaka/`). Modul baru menunggu **namespace modul** (fase 3;
lihat `04_TEKNIK_LANJUTAN.md` §6). Sampai saat itu, fungsi baru tetap
memakai awalan (`mat_`, `teks_`, ...) seperti sekarang.

Pemakaian yang ditargetkan:

```
impor matematika
dari koleksi impor Penghitung
cetak(matematika.akar(16), Penghitung("halo"))
```

**Prioritas:** P1 = fase 3 (modul inti pertama), P2 = fase 6, P3 = fase 8 /
opsional. Modul yang tidak masuk akal untuk bahasa pendidikan (mis.
`ctypes`, `mmap`, `pickle` lintas versi) tidak didaftar.

---

## 1. `math` → `matematika` (P1, C)

| Kelompok | Python | Usulan | Ada |
|---|---|---|---|
| Konstanta | `pi`, `e`, `tau`, `inf`, `nan` | `pi`, `e`, `tau`, `takhingga`, `nan` | `pustaka/matematika.idk` (sebagian) |
| Pembulatan | `ceil`, `floor`, `trunc`, `modf` | `atap`, `lantai`, `potong`, `pecah` | `atap`, `lantai` |
| Akar & pangkat | `sqrt`, `isqrt`, `cbrt`, `pow`, `exp`, `exp2`, `expm1` | `akar`, `akar_bulat`, `akar_pangkat_tiga`, `pangkat`, `eksp`, `eksp2`, `eksp_kurang_satu` | `akar`, `pangkat` |
| Logaritma | `log`, `log2`, `log10`, `log1p` | `log`, `log2`, `log10`, `log_satu_plus` | |
| Trigonometri | `sin cos tan asin acos atan atan2` | sama (`sin`, `cos`, …) | ✅ builtin C |
| Hiperbolik | `sinh cosh tanh asinh acosh atanh` | sama | |
| Sudut | `degrees`, `radians` | `derajat`, `radian` | ✅ builtin C |
| Teori bilangan | `gcd`, `lcm`, `factorial`, `comb`, `perm` | `fpb`, `kpk`, `faktorial`, `kombinasi`, `permutasi` | `mat_fpb`, `mat_kpk`, `mat_faktorial` |
| Jumlah | `fsum`, `prod`, `sumprod`, `dist`, `hypot` | `jumlah_tepat`, `hasil_kali`, `jumlah_hasil_kali`, `jarak`, `hipotenusa` | |
| Pemeriksaan | `isclose`, `isfinite`, `isinf`, `isnan` | `hampir_sama`, `adalah_hingga`, `adalah_takhingga`, `adalah_nan` | |
| Lain-lain | `fabs`, `fmod`, `remainder`, `copysign`, `frexp`, `ldexp`, `nextafter`, `ulp`, `gamma`, `lgamma`, `erf`, `erfc` | `mutlak`, `sisa_fmod`, `sisa`, `salin_tanda`, … | `abs` |

Bonus khas Indonesiaku (sudah ada di `pustaka/matematika.idk`):
`genap`, `ganjil`, `adalah_prima`, `rata_rata`.

## 2. `random` → `acak` (P1, C)

| Python | Usulan | Ada |
|---|---|---|
| `random()` | `acak()` | `acak()` |
| `randint(a, b)` | `bulat_acak(a, b)` | `acak_bulat` |
| `randrange(a, b, langkah)` | `rentang_acak` | |
| `uniform(a, b)` | `seragam` | |
| `choice(seq)` | `pilih` | |
| `choices(seq, k=, bobot=)` | `pilih_banyak` | |
| `sample(seq, k)` | `sampel` | |
| `shuffle(daftar)` | `kocok` | |
| `seed(n)` | `benih` | |
| `gauss`, `normalvariate`, `triangular`, `expovariate` | `gauss`, `normal`, `segitiga`, `eksponensial` | |
| `getrandbits(k)` | `bit_acak` | |
| kelas `Random` | `PembangkitAcak` | |

Ganti `rand()` C dengan PRNG yang bagus dan portabel (xoshiro256** atau PCG)
agar hasil `benih(42)` identik di semua OS — penting untuk tes golden.

## 3. `string` → `teks` (P1, idk)

Konstanta: `ascii_letters` `huruf_ascii`, `ascii_lowercase` `huruf_kecil_ascii`,
`ascii_uppercase` `huruf_besar_ascii`, `digits` `digit`, `hexdigits`
`digit_heksa`, `octdigits` `digit_oktal`, `punctuation` `tanda_baca`,
`whitespace` `spasi_putih`, `printable` `dapat_dicetak`. Fungsi `capwords`
→ `kapital_tiap_kata`. Kelas `Template` → `Templat` (`substitusi`).

## 4. `sys` → `sistem` (P1, C)

| Python | Usulan | Ada |
|---|---|---|
| `argv` | `argumen` | ✅ global `argumen` |
| `exit(kode)` | `keluar(kode)` | `henti` |
| `version`, `version_info` | `versi`, `info_versi` | `-v` CLI |
| `platform` | `platform` | |
| `path` | `jalur` (pencarian modul) | |
| `modules` | `modul` | |
| `stdin` / `stdout` / `stderr` | `masukan_std` / `keluaran_std` / `galat_std` | |
| `getrecursionlimit` / `setrecursionlimit` | `batas_rekursi` / `atur_batas_rekursi` | |
| `maxsize`, `getsizeof` | `ukuran_maks`, `ukuran_objek` | |

## 5. `time` → `waktu`, `datetime` → `tanggal_waktu` (P1)

`time`: `time()` `sekarang()` (ada: `waktu()`), `sleep` `tidur`,
`perf_counter` `penghitung_kinerja` (ada: `jam()`), `monotonic` `monoton`,
`localtime` `waktu_lokal`, `gmtime` `waktu_utc`, `strftime` `format_waktu`
(ada: `format_tanggal`), `strptime` `urai_waktu`, `mktime`, `ctime`.

`datetime` (kelas, ditulis idk di atas primitif C):

| Python | Usulan | Metode utama |
|---|---|---|
| `date` | `Tanggal` | `hari_ini()`, `tahun/bulan/hari`, `hari_dalam_minggu()`, `format()`, `dari_iso()`, `ke_iso()` |
| `time` | `Jam` | `jam/menit/detik` |
| `datetime` | `TanggalWaktu` | `sekarang()`, `dari_cap_waktu()`, `cap_waktu()`, `ganti()`, `format()`, `urai()` |
| `timedelta` | `SelisihWaktu` | `hari`, `detik`, `total_detik()`, operator `+ -` |
| `timezone` | `ZonaWaktu` | `utc`, `WIB/WITA/WIT` sebagai konstanta lokal |

Nama hari/bulan dalam bahasa Indonesia secara bawaan (`Senin`, `Januari`).

**Status (Fase 19 V1):** `pustaka/tanggal_waktu.idk` mengimplementasikan
`TanggalWaktu` (tanggal saja = `TanggalWaktu` tengah malam; selisih sebagai
detik lewat `selisih()`, belum kelas `SelisihWaktu`; zona = offset tetap
`UTC/WIB/WITA/WIT`, belum `ZonaWaktu` IANA). `pustaka/xml.idk` mengisi baris
`xml` di §10; lihat `docs/ANALISIS/KATALOG_API_DAN_GAP_WEB.md`.

## 6. `os`, `os.path`, `pathlib`, `shutil`, `glob`, `tempfile` → `os`, `jalur` (P2, C)

| Python | Usulan | Ada |
|---|---|---|
| `os.getcwd` / `chdir` | `direktori_kini` / `pindah_direktori` | ✅ `direktori_kini` |
| `os.listdir` / `scandir` / `walk` | `isi_direktori` / `pindai_direktori` / `jelajahi` | ✅ `isi_direktori` (terurut) |
| `os.mkdir` / `makedirs` / `rmdir` | `buat_direktori` / `buat_direktori_bertingkat` / `hapus_direktori` | ✅ `buat_direktori`, `hapus_direktori` |
| `os.remove` / `rename` | `hapus_berkas` / `ganti_nama` | `hapus_berkas` |
| `os.environ` / `getenv` | `lingkungan` / `ambil_lingkungan` | ✅ |
| `os.system` | `jalankan_perintah` (dimatikan di `--sandbox`) | |
| `os.getpid`, `cpu_count`, `sep`, `linesep`, `urandom` | `id_proses`, `jumlah_cpu`, `pemisah`, `pemisah_baris`, `bita_acak` | |
| `os.path.join` / `split` / `splitext` | `jalur.gabung` / `jalur.pisah` / `jalur.pisah_ekstensi` | ✅ |
| `os.path.exists` / `isfile` / `isdir` | `jalur.ada` / `jalur.adalah_berkas` / `jalur.adalah_direktori` | ✅ |
| `os.path.basename` / `dirname` / `abspath` / `expanduser` | `jalur.nama_dasar` / `jalur.nama_direktori` / `jalur.absolut` / `jalur.rumah` | ✅ dua pertama; `absolut`/`rumah` butuh native |
| `os.path.getsize` / `getmtime` | `jalur.ukuran` / `jalur.waktu_ubah` | ✅ |
| `pathlib.Path` | kelas `Jalur` (`/` operator, `.baca_teks()`, `.tulis_teks()`, `.anak()`, `.induk`) | |
| `shutil.copy` / `copytree` / `move` / `rmtree` / `which` | `salin_berkas` / `salin_pohon` / `pindahkan` / `hapus_pohon` / `cari_program` | |
| `glob.glob` | `cocok_jalur("*.idk")` | |
| `tempfile.mkstemp` / `TemporaryDirectory` | `berkas_sementara` / `DirektoriSementara` | |

Fungsi berkas yang sudah ada (`buka_berkas`, `baca`, `tulis`, `baca_baris`,
`baca_semua`, `tulis_semua`, `tambah_berkas`, `tutup`) disatukan dalam objek
berkas dengan metode (`f.baca()`, `f.tulis()`, `untuk baris dalam f`) dan
`dengan buka("a.txt") sebagai f { ... }`.

## 7. `collections` → `koleksi` (P2, idk dulu, C bila perlu cepat)

| Python | Usulan | Keterangan |
|---|---|---|
| `Counter` | `Penghitung` | `paling_umum(n)`, `tambah()`, operator `+ -` |
| `defaultdict` | `KamusBawaan` | `KamusBawaan(daftar)` |
| `OrderedDict` | — | kamus sudah berurut; sediakan alias `KamusBerurut` |
| `deque` | `Antrian` | `tambah_kiri`, `ambil_kiri`, `putar`, `maks_panjang` — C, O(1) dua ujung |
| `namedtuple` | `tupel_bernama` | |
| `ChainMap` | `PetaBerantai` | P3 |

## 8. `itertools` → `iterasi`, `functools` → `fungsional`, `operator` → `operator` (P2)

Butuh iterator/generator (fase 5); versi awal boleh mengembalikan daftar.

| Python | Usulan |
|---|---|
| `count`, `cycle`, `repeat` | `hitung_naik`, `putar_ulang`, `ulangi` |
| `accumulate`, `chain`, `compress` | `akumulasi`, `rangkai`, `saring_dengan` |
| `takewhile`, `dropwhile`, `filterfalse` | `ambil_selama`, `lewati_selama`, `saring_salah` |
| `groupby`, `islice`, `pairwise`, `batched` | `kelompokkan`, `iris`, `berpasangan`, `kelompok_per` |
| `starmap`, `tee`, `zip_longest` | `peta_bintang`, `cabang`, `pasangkan_terpanjang` |
| `product`, `permutations`, `combinations`, `combinations_with_replacement` | `produk`, `permutasi`, `kombinasi`, `kombinasi_berulang` |
| `functools.reduce` | `lipat` (ada: `daftar_lipat`) |
| `partial` | `parsial` |
| `lru_cache` / `cache` | `cache_lru` / `cache` (dekorator, fase 4) |
| `wraps`, `total_ordering`, `cmp_to_key`, `cached_property`, `singledispatch` | `bungkus`, `urutan_lengkap`, `banding_ke_kunci`, `properti_tercache`, `kirim_tunggal` |
| `operator.add/mul/itemgetter/attrgetter` | `operator.tambah/kali/pengambil_item/pengambil_atribut` |

## 9. `re` → `pola` (P2, C)

**Status: ✅ terlaksana** (`src/pola.c` + `pustaka/pola.idk`, Fase 6
gelombang C). Gaya Pike VM, bukan PCRE2: waktu linear, sehingga
backreference/lookaround/kuantifier posesif sengaja ditolak. Kelas karakter
ASCII seperti `re.ASCII`. Fungsi dan objek di bawah semuanya ada, kecuali
`ganti_n` mengembalikan tupel seperti `subn`.

Implementasi: port mesin regex kecil (mis. gaya Pike VM / `re1` Russ Cox,
atau embed PCRE2 berlisensi BSD). Fungsi: `kompilasi`, `cari` (`search`),
`cocok` (`match`), `cocok_penuh`, `cari_semua` (`findall`), `iter_cari`
(`finditer`), `ganti` (`sub`), `ganti_n` (`subn`), `pisah` (`split`),
`escape` → `lolos`. Objek `Cocokan`: `grup()`, `grup_semua()`, `rentang()`,
`awal()`, `akhir()`. Bendera: `ABAIKAN_KAPITAL`, `MULTIBARIS`, `TITIK_SEMUA`.

## 10. Format data

| Python | Modul | Fungsi | Ada | P |
|---|---|---|---|---|
| `json` | `json` | `ke_json`(`dumps`), `dari_json`(`loads`), `simpan`(`dump`), `muat`(`load`), opsi `indentasi=` | `json_teks`, `json_urai` (idk) | P1 — pindah ke C demi kecepatan |
| `csv` | `csv` | `pembaca`, `penulis`, `PembacaKamus`, `PenulisKamus` | | P2 |
| `base64` | `base64` | `enkode`, `dekode`, varian `url` | | P2 |
| `hashlib` | `hash` | `md5`, `sha1`, `sha256`, `sha512` → `.heksa()` | | P2 |
| `uuid` | `uuid` | `uuid4()` | | P2 |
| `secrets` | `rahasia` | `token_heksa`, `token_url`, `pilih` | | P2 |
| `zipfile` / `tarfile` / `gzip` / `zlib` | `arsip` / `kompres` | `daftar_isi`, `ekstrak` (aman bawaan), `buat`; `gzip`/`buka_gzip`, `zlib`, `deflate`, `gzip_berkas`, `crc32`, `adler32` | ✅ V1 Fase 23 (miniz 3.1.2 di-vendor) | — |
| `sqlite3` | `sqlite` | `db_buka`, `db_eksekusi`, `db_ambil_semua`, `db_ambil_satu`, `db_mulai/komit/batal`, `db_tutup` | | P1 — Fase 3 (embed amalgamation, public domain) |
| `html` / `xml` | `html` / `xml` | `lolos`, pengurai sederhana | `json_escape` analog | P3 |
| `tomllib` | `toml` | `muat` | | P3 |

## 11. Teks & tampilan

`textwrap` → `bungkus_teks` (`bungkus`, `isi`, `hilangkan_indentasi`,
`indentasi`, `persingkat`), `pprint` → `cetak_rapi` (`cetak_rapi`,
`format_rapi`), `difflib` → `beda` (P3), `unicodedata` (P3).

## 12. OOP helper

| Python | Usulan | P |
|---|---|---|
| `copy.copy` / `deepcopy` | `salin.salin` / `salin.salin_dalam` | P2 |
| `enum.Enum`, `IntEnum`, `auto` | `Enum` → `Enumerasi`, `EnumerasiBulat`, `otomatis` | P2 |
| `dataclasses.dataclass`, `field`, `asdict` | `@kelas_data`, `bidang`, `ke_kamus` | P2 (butuh dekorator) |
| `abc.ABC`, `abstractmethod` | `KelasAbstrak`, `@metode_abstrak` | P2 |
| `contextlib.contextmanager`, `suppress` | `@pengelola_konteks`, `abaikan(Galat...)` | P3 |
| `typing` | — (anotasi opsional, fase 8) | P3 |

## 13. Struktur data & algoritma

`heapq` → `tumpukan_prioritas` (`dorong`, `ambil`, `jadikan_heap`,
`n_terbesar`, `n_terkecil`); `bisect` → `bagi_dua` (`cari_kiri`,
`cari_kanan`, `sisip_urut`); `statistics` → `statistik` (`rata_rata`,
`median`, `modus`, `simpangan_baku`, `varians`, `kuantil`) — sebagian ada di
`pustaka/matematika.idk`; `fractions` → `pecahan` (`Pecahan`); `decimal` →
`desimal_tepat` (`Desimal`). P2, kebanyakan idk.

## 14. Sistem, proses, konkurensi

| Python | Usulan | P |
|---|---|---|
| `subprocess.run` / `check_output` / `Popen` | `proses.jalankan` / `proses.keluaran` / `Proses` | ✅ dua pertama; `Proses` (Popen) belum |
| `argparse.ArgumentParser` | `opsi.PenguraiArgumen` (`tambah_argumen`, `urai`) | ✅ |
| `logging` (`debug`, `info`, `warning`, `error`, `critical`, `basicConfig`, `getLogger`) | `log` (`debug`, `info`, `peringatan`, `galat`, `kritis`, `atur`, `ambil_pencatat`) | ✅ |
| `warnings.warn` | `peringatan.peringatkan` | P2 |
| `platform.system` / `python_version` | `platform.sistem` / `platform.versi` | P2 |
| `signal` | `sinyal` | P3 |
| `threading.Thread`, `Lock`, `Event`; `queue.Queue` | `utas.Utas`, `Kunci`, `Peristiwa`; `antrian.Antrian` | P3 (fase 7) |
| `asyncio.run`, `gather`, `sleep`, `create_task` | `asinkron.jalankan`, `kumpulkan`, `tidur`, `buat_tugas` | P3 (fase 7) |

## 15. Jaringan

| Python | Usulan | Ada | P |
|---|---|---|---|
| `socket` | `soket` | ✅ TCP localhost (`soket_dengar`, …) | klien `soket_sambung` ✅; P2: server di alamat selain 127.0.0.1, UDP |
| `http.server` | `http.server` | 🟡 `pustaka/http.idk` + `todo_server.idk` | P2 |
| `urllib.request.urlopen` / `http.client` | `http_ambil(url)` (klien) | ✅ `http://` saja, `Respons`, `GalatJaringan` | HTTPS butuh TLS (embed mbedTLS) → P3 |
| `urllib.parse` (`quote`, `urlencode`, `urlparse`) | `url.kutip`, `url.enkode_kueri`, `url.urai` | | P2 |
| `smtplib`, `email` | — | | P3 |

## 16. Pengujian & pengembangan

| Python | Usulan | P |
|---|---|---|
| `unittest.TestCase` + `assertEqual`, `assertTrue`, `assertRaises`, … | `uji.KasusUji` + `pastikan_sama`, `pastikan_benar`, `pastikan_galat`, … | P2 |
| `pytest` (fungsi `test_*` + `assert`) | `indk uji` menjalankan fungsi `uji_*` dengan `pastikan` | P2 — lebih ramah pemula daripada kelas |
| `doctest` | contoh di docstring dijalankan `indk uji --dok` | P3 |
| `timeit` | `ukur_waktu` | P2 |
| `pdb` / `breakpoint()` | `indk debug` / `titik_henti()` | P3 |
| `dis` | `indk --dis berkas.idk` | P2 (disassembler sudah ada di `debug.c`) |
| `inspect` | `periksa` | P3 |

## 17. Pendidikan & grafis

| Python | Usulan | P |
|---|---|---|
| `turtle` (`forward`, `backward`, `left`, `right`, `penup`, `pendown`, `goto`, `circle`, `color`, `speed`, `done`) | `kura` (`maju`, `mundur`, `kiri`, `kanan`, `angkat_pena`, `turunkan_pena`, `ke`, `lingkaran`, `warna`, `kecepatan`, `selesai`) | ✅ `pustaka/kura.idk` → SVG (plus `hadap`, `pulang`, `tebal`, `svg_dari`); kanvas playground → P3 |
| `tkinter` | `jendela` | P3 — via FFI/binding |

---

## 18. Ringkasan jumlah fungsi yang akan diduplikasi

| Kelompok | Perkiraan item | P1 | P2 | P3 |
|---|---|---|---|---|
| `matematika` | ±60 | 60 | — | — |
| `acak` | ±15 | 15 | — | — |
| `teks`, `sistem`, `waktu`, `tanggal_waktu` | ±70 | 70 | — | — |
| `os` / `jalur` | ±45 | — | 45 | — |
| `koleksi`, `iterasi`, `fungsional` | ±45 | — | 45 | — |
| `pola`, format data, teks | ±60 | — | 40 | 20 |
| OOP helper, struktur data | ±40 | — | 35 | 5 |
| Sistem, proses, jaringan, uji | ±60 | — | 40 | 20 |
| Konkurensi, grafis, lain-lain | ±40 | — | 11 | 29 |
| **Total** | **±435** | **±145** | **±216** | **±74** |

Banyak dari item P2 bisa ditulis **dalam Indonesiaku sendiri** di
`pustaka/` setelah fase 1–3 selesai. Itu cara terbaik menguji kematangan
bahasa ("dogfooding") sekaligus pekerjaan ideal bagi kontributor pemula.
