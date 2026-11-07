# 02 — Peta Fitur Python → Indonesiaku

Dokumen ini adalah **daftar periksa lengkap** dari apa yang bisa dilakukan
Python 3.12, dipetakan ke Indonesiaku: sudah ada (✅), sebagian (🟡), belum
(❌), dan di fase roadmap mana ia direncanakan (lihat
[`ROADMAP.md`](../ROADMAP.md)). Nama Indonesia di kolom "Usulan" adalah
**usulan**; nama final dikunci saat rencana fiturnya ditulis di
`docs/RENCANA_FITUR/`.

Prinsip penamaan:

1. Kata kerja perintah untuk aksi (`cetak`, `kembalikan`, `urutkan`).
2. Kata benda untuk tipe/konstruktor (`daftar`, `kamus`, `himpunan`).
3. Hindari bentrok dengan builtin yang sudah ada (mis. `hapus` sudah dipakai
   builtin, jadi `del` butuh nama lain).
4. Kalau istilah teknisnya sudah umum dipakai programmer Indonesia (`indeks`,
   `iterator`, `hash`), boleh dipakai apa adanya.

---

## A. Kata kunci (Python punya 35 + soft keyword)

| Python | Indonesiaku | Status | Fase |
|---|---|---|---|
| `True` / `False` / `None` | `benar` / `salah` / `kosong` | ✅ | — |
| `and` / `or` / `not` | `dan` / `atau` / `tidak` | ✅ | — |
| `if` / `elif` / `else` | `jika` / `jika_lain` / `selain` | ✅ | — |
| `while` | `selagi` | ✅ | — |
| `for ... in` | `untuk ... dalam` | ✅ list, teks, rentang, kamus (kunci insertion-order) | 1 |
| `break` / `continue` | `hentikan` / `lanjut` | ✅ | — |
| `def` | `fungsi` | ✅ | — |
| `return` | `kembalikan` | ✅ | — |
| `class` | `kelas` | ✅ | — |
| `self` (konvensi) | `diri` (kata kunci) | ✅ | — |
| `super()` | `super` | ✅ | — |
| `try` / `except` | `coba` / `kecuali` | 🟡 belum ada filter tipe | 1 |
| `finally` | `akhirnya` | ❌ (token ada) | 0 |
| `raise` | `naikkan` | ✅ | — |
| `import` | `impor nama` / `impor "berkas"` | ✅ namespace per modul; bentuk berkas tetap berbagi namespace | 3 |
| `from ... import` / `as` | `dari ... impor` / `sebagai` | ✅ | 3 |
| `pass` | `lewati` | ❌ (token ada) | 0 |
| `in` (operator) | `dalam` | ✅ | — |
| `not in` | `tidak dalam` | ✅ | — |
| `is` / `is not` | `adalah` / `bukan` | ❌ | 1 |
| `lambda` | `fungsi(x) { ... }` anonim | ✅ | 1 — selesai; badan `{ }`, bukan satu-ekspresi |
| `global` / `nonlocal` | `global` / `nonlokal` | ✅ | — |
| `del` | `buang` | ❌ | 2 |
| `assert` | `pastikan` | ✅ | 1 — selesai; pesan dinilai hanya saat gagal |
| `with` | `dengan ... sebagai` | ✅ `_masuk_`/`_keluar_` | 4 |
| `yield` | `hasilkan` | ❌ | 5 |
| `async` / `await` | `asinkron` / `tunggu` | ❌ | 7 |
| `match` / `case` (3.10) | `cocokkan` / `kasus` | ❌ | 5 |

## B. Sintaks & ekspresi

| Fitur Python | Contoh | Status | Fase |
|---|---|---|---|
| Komentar `#` | | ✅ | — |
| String multi-baris `"""` | | ✅ | 1 — selesai; docstring (atribut) belum |
| f-string | `f"Halo {nama}"` | ✅ | 1 — selesai; format spec `{x:.2f}` belum |
| Literal angka: `1e3`, `0x1F`, `0b101`, `1_000` | | ✅ | 1 — selesai |
| Literal kamus `{"a": 1}` | | ✅ | — |
| Literal tupel `(1, 2)` | `(1, 2)`, `(1,)`, `()` (K17) | ✅ | — |
| Literal himpunan `{1, 2}` | `himpunan([1, 2])` (K17: sengaja tanpa literal) | ✅ | — |
| Indeks negatif `a[-1]` | | ✅ | — |
| Irisan `a[1:3]`, `a[::-1]` | | ✅ daftar & teks | — |
| Gabung `+` / ulang `*` teks & daftar | | ✅ | — |
| `==` struktural pada daftar/kamus/rentang | | ✅ `adalah` tetap identitas | — |
| Penugasan berganda `a, b = 1, 2` / tukar | | ❌ | 2 |
| Unpacking `a, *sisa = daftar` | | ❌ | 2 |
| Ekspresi kondisional `x jika c selain y` | | ✅ | 1 — selesai; kondisi dinilai lebih dulu |
| Rantai perbandingan `1 < x < 3` | | ✅ | — |
| Floor division `//`, `//=` | | ❌ **rusak** (B1) | 0 |
| Operator bitwise `& \| ^ ~ << >>` | `& \| ^ ~ << >>` (sama) | ✅ | — |
| List comprehension | `[x*2 untuk x dalam a jika x > 0]` | ❌ | 2 |
| Dict/set comprehension, generator expression | | ❌ | 5 |
| Argumen bawaan `def f(a, b=2)` | | ✅ dievaluasi tiap panggilan (K15) | — |
| Argumen bernama `f(b=1)` | | ✅ (K16) | — |
| `*args`, `**kwargs` | | ❌ | 2 |
| Dekorator `@` | | 🟡 pada fungsi; pada metode hanya tiga bawaan | 4 |
| Walrus `:=` | | 🟡 (assignment sudah ekspresi) | — |
| Anotasi tipe `x: int` | | ❌ | 8 (opsional) |

## C. Tipe data bawaan

| Python | Indonesiaku | Status | Fase | Catatan |
|---|---|---|---|---|
| `bool` | `benar`/`salah` | ✅ | — | |
| `NoneType` | `kosong` | ✅ | — | |
| `int` (tak terbatas) | `bulat` | ❌ | 2 | Saat ini semua `double` |
| `float` | `desimal` | 🟡 | — | Presisi cetak salah (B10) |
| `complex` | `kompleks` | ❌ | 8 | Prioritas rendah |
| `str` | `teks` | 🟡 | 1–2 | Byte, bukan karakter (B11); 8 metode inti sudah ada |
| `list` | `daftar` | 🟡 | 1 | Irisan, `+`, `*` sudah ada; 11 metode inti sudah ada |
| `tuple` | `tupel` | ✅ | — | Kunci kamus bila elemennya hashable |
| `dict` | `kamus` | 🟡 | 1–2 | Kunci hanya string; literal `{ }` sudah ada |
| `set` / `frozenset` | `himpunan` / `himpunan_beku` | ✅ | 2 | keduanya ada |
| `range` | `rentang` | ✅ | — | Objek malas, seperti Python 3 |
| `bytes` / `bytearray` | `bita` / `larik_bita` | ❌ | 6 | Diperlukan untuk berkas biner, jaringan |
| Fungsi, kelas, metode terikat | | ✅ | — | |
| Iterator / generator | | ❌ | 5 | |
| Modul sebagai objek | | ❌ | 3 | |

### C.1 Metode `str` (Python ±47 metode)

Usulan prioritas fase 1–2 (sebagian sudah ada sebagai fungsi `teks_*` di
`pustaka/teks.idk` — pindahkan ke metode native).

**Status:** bentuk metode sudah aktif (K10). Yang bertanda "sebagai fungsi"
kini juga dapat dipanggil sebagai metode bila namanya terdaftar di
`stringMethods`/`listMethods`/`dictMethods` di `src/vm.c`; sisanya masih
menunggu native-nya dibuat.

| Python | Usulan | Ada sekarang? |
|---|---|---|
| `upper` / `lower` | `.huruf_besar()` / `.huruf_kecil()` | ✅ metode + fungsi |
| `strip` / `lstrip` / `rstrip` | `.pangkas()` / `.pangkas_kiri()` / `.pangkas_kanan()` | ❌ |
| `split` / `join` | `.pisah(sep)` / `sep.gabung(daftar)` | 🟡 pustaka |
| `replace` | `.ganti(a, b)` | sebagai fungsi |
| `find` / `index` / `count` | `.cari()` / `.indeks()` / `.hitung()` | 🟡 pustaka |
| `startswith` / `endswith` | `.mulai_dengan()` / `.akhiri_dengan()` | 🟡 pustaka |
| `isdigit` / `isalpha` / `isspace` | `.adalah_angka()` / `.adalah_huruf()` / `.adalah_spasi()` | ❌ |
| `title` / `capitalize` | `.judul()` / `.kapital()` | ❌ |
| `format` | `.format(...)` | sebagai fungsi |
| `zfill` / `center` / `ljust` | `.isi_nol()` / `.tengah()` / `.rata_kiri()` | ❌ |

### C.2 Metode `list`

`append`→`.tambah`, `extend`→`.perluas`, `insert`→`.sisip`, `pop`→`.ambil_keluar`,
`remove`→`.hapus`, `clear`→`.kosongkan`, `index`→`.indeks`, `count`→`.hitung`,
`sort`→`.urutkan`, `reverse`→`.balik`, `copy`→`.salin`.

### C.3 Metode `dict`

`keys`→`.kunci()`, `values`→`.nilai()`, `items`→`.pasangan()`,
`get`→`.ambil(k, bawaan)`, `pop`→`.ambil_keluar`, `update`→`.perbarui`,
`setdefault`→`.atur_bawaan`, `clear`→`.kosongkan`, `copy`→`.salin`.

## D. Fungsi bawaan Python (71) → Indonesiaku

Legenda kolom "Ada": nama builtin Indonesiaku yang sudah terdaftar di
`initVM` (`src/vm.c`).

| Python | Usulan | Ada | Fase |
|---|---|---|---|
| `print` | `cetak` | ✅ (kata kunci) | 1: jadikan fungsi, `pisah=`/`akhir=` |
| `input` | `input` / `masukan` | ✅ | — |
| `len` | `panjang` | ✅ | — |
| `type` | `jenis` | 🟡 nama string | 2 |
| `int` / `float` / `str` / `bool` | `ke_bulat` / `ke_desimal` / `ke_teks` / `ke_logika` | ✅ | 1 | plus `ke_angka` (bulat bila teksnya bulat) |
| `list` / `dict` / `set` / `tuple` | `daftar()` / `kamus()` / `himpunan()` / `tupel()` | ✅ | 2 |
| `range` | `rentang` | ✅ | — |
| `enumerate` | `enumerasi` | ✅ list pasangan `[indeks, nilai]`, offset opsional | 1 |
| `zip` | `pasangkan` | ✅ | — |
| `map` / `filter` | `peta` / `saring` | ✅ (mengembalikan daftar, bukan iterator) | — |
| `sorted` / `reversed` | `terurut` / `terbalik` | ✅ | — |
| `sum` / `min` / `max` | `jumlah` / `min` / `max` | ✅ min/max juga menerima satu daftar | — |
| `abs` / `round` / `pow` / `divmod` | `abs` / `bulatkan` / `pangkat` / `bagi_sisa` | ✅ (`bulatkan` = round banker's; `bulat` lama tetap beda) | — |
| `all` / `any` | `semua` / `ada_yang` | ✅ | — |
| `isinstance` / `issubclass` | `adalah_instansi` / `turunan_dari` | ✅ | 2 |
| `hasattr` / `getattr` / `setattr` / `delattr` | `punya_atribut` / `ambil_atribut` / `atur_atribut` / `buang_atribut` | ❌ | 4 |
| `callable` | `bisa_dipanggil` | ✅ | — |
| `repr` / `str` | `repr` / `ke_teks` | ✅ | — |
| `ord` / `chr` | `ord` / `chr` | ✅ | — |
| `bin` / `oct` / `hex` | `biner` / `oktal` / `heksa` | ✅ (int64; bigint belum) | — |
| `hash` / `id` | `hash` / `id` | ✅ | 2 |
| `iter` / `next` | `iter` / `lanjutkan` | ✅ | 5 | `next` dinamai `lanjutkan` (`lanjut` sudah kata kunci) |
| `open` | `buka` | 🟡 `buka_berkas` | 4: `dengan buka(...) sebagai f` |
| `format` | `format` | ✅ | — |
| `dir` / `vars` / `globals` / `locals` / `help` | `daftar_atribut` / `vars` / `global_()` / `lokal()` / `bantuan` | ❌ | 6 |
| `eval` / `exec` / `compile` | `evaluasi` / `eksekusi` | ❌ | 6 |
| `property` / `staticmethod` / `classmethod` | `properti` / `metode_statis` / `metode_kelas` | ❌ | 4 |
| `object` / `super` | `objek` / `super` | 🟡 `super` | 4 |
| `slice` | `irisan` | ✅ sintaks `[a:b:c]` | — |
| `bytes` / `bytearray` / `memoryview` | `bita` / `larik_bita` / — | ❌ | 6 |
| `frozenset` / `complex` | `himpunan_beku` / `kompleks` | 🟡 `himpunan_beku` ada; `kompleks` belum | 8 |
| `breakpoint` | `titik_henti` | ❌ | 7 |
| `aiter` / `anext` / `__import__` / `ascii` | — | ❌ | 8 (opsional) |

## E. Pemrograman berorientasi objek

| Python | Status | Fase |
|---|---|---|
| Kelas, konstruktor, metode, pewarisan tunggal, `super` | ✅ | — |
| `isinstance`, kelas sebagai nilai pertama | ❌ / ✅ | 2 |
| Pewarisan berganda + MRO (C3) | ✅ `kelas D < B, C`; `super` masih induk pertama | 4 |
| Metode ajaib (dunder): `__str__`, `__eq__`, `__lt__`, `__add__`, `__len__`, `__getitem__`, `__iter__`, `__call__`, `__hash__`, `__enter__/__exit__` | 🟡 representasi, perbandingan, aritmetika + cerminannya (`__radd__` dst), panjang, kebenaran, indeks, hash (K19); `NotImplemented` dan iterator malas `_berikutnya_` belum; `_iter_`/`_panggil_` dan manajer konteks `_masuk_`/`_keluar_` sudah ada; `terurut`/`min`/`max` memakai `_kurang_dari_` | 4 |
| Usulan nama: `_teks_`, `_sama_`, `_kurang_`, `_tambah_`, `_panjang_`, `_ambil_`, `_iter_`, `_panggil_` | | |
| Atribut kelas (bukan per instansi) | ✅ | 4 |
| `@property`, `@staticmethod`, `@classmethod` | ✅ `@properti`, `@metode_statis`, `@metode_kelas` (tanpa penyetel) | 4 |
| Kelas abstrak, `dataclass`, `enum` | 🟡 kelas abstrak `@abstrak` ✅; `dataclass`/`enum` belum | 6 |
| Hierarki pengecualian berbasis kelas (`class GalatSaya(Galat)`) | 🟡 `Galat*` bawaan dan `kecuali Tipe sebagai e`; kelas galat pengguna menyusul | 1 |

## F. Pengecualian bawaan (usulan hierarki Indonesia)

```
Galat (BaseException/Exception)
├── GalatAritmetika        (ArithmeticError)
│   └── GalatBagiNol       (ZeroDivisionError)
├── GalatPencarian         (LookupError)
│   ├── GalatIndeks        (IndexError)
│   └── GalatKunci         (KeyError)
├── GalatTipe              (TypeError)
├── GalatNilai             (ValueError)
├── GalatNama              (NameError)
├── GalatAtribut           (AttributeError)
├── GalatBerkas / GalatOS  (OSError, FileNotFoundError)
├── GalatImpor             (ImportError)
├── GalatRekursi           (RecursionError)
├── GalatPenegasan         (AssertionError — untuk `pastikan`)
├── HentiIterasi           (StopIteration)
└── GalatSintaks           (SyntaxError — dari `evaluasi`)
```

Pemakaian yang ditargetkan:

```
coba {
    angka = ke_bulat(input("Angka: "))
} kecuali GalatNilai sebagai e {
    cetak("Bukan angka:", e.pesan)
} akhirnya {
    cetak("selesai")
}
```

## G. Modul & paket

| Python | Status | Fase |
|---|---|---|
| `import modul` dengan namespace (`modul.fungsi`) | ✅ | 3 |
| `from modul import a, b` / `as` | ✅ | 3 |
| Paket (folder + `__init__`) | ❌ | 3 |
| Jalur pencarian modul (`sys.path`, `INDK_PATH`) | ❌ (relatif ke cwd) | 3 |
| `if __name__ == "__main__"` | ❌ → `jika __nama__ == "__utama__"` | 3 |
| Cache bytecode (`.pyc`) | ❌ → `.idkc` | 7 |
| Manajer paket (`pip` + PyPI) | ❌ → `ipm` / `paket` | 8 |
| Virtual env | ❌ | 8 |

## H. Pustaka standar Python → modul Indonesiaku

Prioritas berdasarkan "apa yang dibutuhkan pemula dan aplikasi umum".

| Python | Modul Indonesiaku | Status | Fase |
|---|---|---|---|
| `math` | `matematika` | 🟡 builtin (termasuk trigonometri) + `pustaka/matematika.idk` | 3 |
| `random` | `acak` | 🟡 `acak`, `acak_bulat` | 3 |
| `secrets` | — | 🟡 `acak_bita()` (bita acak kriptografis); `token_hex`/`compare_digest` belum | 13 |
| `string` | `teks` | 🟡 | 3 |
| `json` | `json` | ✅ | — |
| `time` / `datetime` | `waktu` / `tanggal_waktu` | 🟡 | 3 |
| `os` / `os.path` / `pathlib` / `shutil` / `glob` | `os` / `jalur` | 🟡 `jalur` (persis `posixpath`, plus `ada`/`ukuran`/`absolut`), builtin `direktori_kini`/`isi_direktori`/`buat_direktori`/`lingkungan`; `pathlib`/`shutil`/`glob` belum | 6 |
| `sys` | `sistem` (`argumen`, `keluar`, `versi`) | 🟡 `argumen`, `keluar`, `tulis_galat`, `jalur_indk` (builtin) | 3 |
| `re` | `pola` (regex) | ✅ Pike VM linear; tanpa backreference/lookaround, kelas ASCII | 6 |
| `collections` | `koleksi` (`Penghitung`, `Antrian`, `KamusBawaan`) | ✅ `pustaka/koleksi.idk` (gelombang A); `tupel_bernama` menunggu `*args` | 6 |
| `itertools` / `functools` | `iterasi` / `fungsional` | ✅ eager (gelombang A); versi malas menunggu Fase 5 | 6 |
| `csv` | `csv` | 🟨 kode + tes (gelombang B) | 6 |
| `unittest` / `pytest` | `uji` (+ runner `indk uji`) | ❌ | 7 |
| `logging` | `log` | ✅ | 6 |
| `argparse` | `argparse` → `opsi` | ✅ tanpa singkatan/nargs/sub-perintah | 6 |
| `subprocess` | `proses` | ✅ `jalankan`/`keluaran`, tanpa shell, belum timeout | 6 |
| `socket` | `soket` | ✅ (TCP, localhost) | — |
| `http.server` / `urllib` | `http` | 🟡 server (parse/build) + klien `http_ambil` (`http://`); HTTPS belum | 6 |
| `hashlib` / `base64` / `secrets` / `uuid` | `hash` / `base64` / `rahasia` / `uuid` | 🟡 `md5`/`sha*` builtin, `base64` ✅; `rahasia`/`uuid` belum | 6 |
| `sqlite3` | `sqlite` (`db_*`) | ✅ | 3 — selesai; 10 fungsi, SQLite 3.50.4 vendored, parameter `?` wajib |
| `threading` / `asyncio` | `utas` / `asinkron` | ❌ | 7 |
| `statistics` / `decimal` / `fractions` | `statistik` / `desimal` / `pecahan` | 🟡 `statistik` ✅ (gelombang A); `desimal`/`pecahan` ❌ | 6 |
| `copy` / `pprint` / `textwrap` / `enum` / `dataclasses` | `salin` / … | 🟡 `salin` ✅ (gelombang A); sisanya ❌ | 6 |
| `tkinter` / `turtle` | `kura` (turtle — sangat cocok untuk pendidikan!) | 🟡 `kura` ✅ (SVG); kanvas playground dan `tkinter` belum | 6 |

> **Catatan pendidikan:** modul `turtle` adalah salah satu alasan Python
> populer di sekolah. Modul `kura` (kura-kura) yang menggambar ke SVG/kanvas di
> playground web bisa menjadi fitur unggulan Indonesiaku untuk pelajar.

## I. Perkakas (tooling) di sekitar Python

| Python | Indonesiaku | Status | Fase |
|---|---|---|---|
| Interpreter + REPL | `indk` | ✅ | — |
| Traceback yang jelas | `[baris N] dalam f()` | 🟡 tanpa kolom/potongan kode | 1 |
| Formatter (`black`) | `indk rapikan` | ❌ | 7 |
| Linter (`pyflakes`) | `indk periksa` | ❌ | 7 |
| Test runner | `indk uji` | ❌ | 7 |
| Debugger (`pdb`) | `indk debug` / DAP | ❌ | 7 |
| Language Server (LSP) + ekstensi VS Code | `editors/vscode-indonesiaku/server.js` | 🟡 diagnostics/completion/hover/definisi lokal tersedia; definisi lintas modul belum | 9 |
| Pewarnaan sintaks (VS Code, highlight.js) | | ✅ | 7 |
| Dokumentasi otomatis (`pydoc`) | `indk dok` | ❌ | 7 |
| Playground web | `website/playground` | 🟡 UI ada, backend belum | 7 |
| Disassembler (`dis`) | `DEBUG_PRINT_CODE` | 🟡 flag kompilasi | 7: `indk --dis` |
