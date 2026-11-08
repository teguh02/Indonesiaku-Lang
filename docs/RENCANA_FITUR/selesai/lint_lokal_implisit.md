# Rencana: linter melihat lokal implisit (K3)

- **Status:** selesai
- **Target versi:** 1.6.0
- **Jenis:** perbaikan (linter)
- **Temuan asal:** T-U1 (`docs/TESTING/WINDOWS.md` §7, tingkat rendah, semua OS)

## Masalah

`indk periksa` hanya melaporkan lokal yang dideklarasikan dengan kata
`variabel`. Lokal **implisit** — `sisa = 5` di dalam fungsi, yang menurut K3
juga membuat variabel fungsi-lokal — tidak pernah dilaporkan.

Ini bukan celah kecil: `variabel` bersifat opsional, jadi lokal implisit adalah
cara paling umum orang menulis kode di sini. Linter yang tidak melihatnya
praktis buta terhadap mayoritas kode nyata.

```indonesiaku
fungsi hitung(a, b) {
    variabel tidak_dipakai = 1   # dilaporkan
    sisa = 5                     # TIDAK dilaporkan -- inilah bugnya
    jumlah = a + b
    kembalikan jumlah
}
```

## Akar masalah

`prescanFunctionBody()` (`src/compiler.c`) memindai badan fungsi sekali di muka
dan memanggil `addLocal()` + `markInitialized()` untuk setiap nama yang
di-assign. Ia **tidak** memanggil `markLintable()`, sedangkan `declareVariable()`
— jalur `variabel x` — memanggilnya. Linter hanya melihat slot ber-`lintCheck`,
jadi seluruh lokal K3 tak terlihat.

## Perbaikan

Satu baris di `prescanFunctionBody()`: tandai slot prescan sebagai lintable.

Tetapi menandainya begitu saja menghasilkan **77 false positive** di
`pustaka/` sendiri. Pola penyebabnya ada di mana-mana:

```indonesiaku
jika info > 0 {
    variabel etag = "W/" + ke_teks(info)
    etag = etag + "-akhir"       # assignment implisit di blok bersarang
    kembalikan etag
}
```

Assignment baris kedua membuat prescan mengalokasikan slot **fungsi-level**
bernama `etag`. Slot itu langsung dibayangi `variabel etag` di dalam blok dan
tidak pernah dibaca — jadi ia memang mati, tetapi matinya adalah **artefak
kompiler**, bukan kesalahan pengguna. Melaporkannya salah.

Penyebabnya: daftar `declared` hanya mencatat `variabel x` pada `depth == 1`.
Ditambah daftar `shadowed` yang mencatat `variabel x` pada kedalaman **berapa
pun**, dipakai **hanya** untuk menekan lint. Slotnya tetap dibuat seperti
sebelumnya, jadi bytecode dan semantik tidak berubah sama sekali.

Hasil: 77 → 2 temuan di `pustaka/`.

## Dua temuan tersisa — keduanya asli

| Berkas | Temuan | Tindakan |
|---|---|---|
| `pustaka/templat.idk:323` | `variabel selesai = salah`, tidak pernah disebut lagi di seluruh berkas | deklarasi mati, dihapus |
| `pustaka/kura.idk:132` | `untuk i dalam rentang(langkah)`, `i` tak dipakai di badan | diganti `_i` (konvensi repo) |

Linter menemukan kode mati nyata pada percobaan pertama.

## Uji

`tests/lint/lokal_implisit.idk` + `.lint` — satu temuan yang diharapkan
(`sisa`), ditambah **5 pola sah** yang tidak boleh dilaporkan:

1. ditulis di satu cabang, dibaca sesudahnya;
2. akumulator yang ditulis ulang di dalam perulangan;
3. ditangkap closure lewat `nonlokal`;
4. berawalan `_` (sengaja diabaikan);
5. `variabel` di blok bersarang lalu di-assign lagi di sana (pola 77 false
   positive itu).

### Sabotase

| Sabotase | Hasil |
|---|---|
| buang `markLintable()` K3 (T-U1 kambuh) | merah |
| buang penekan bayangan (77 false positive kembali) | merah |

Sabotase kedua awalnya **LOLOS**: pola 5 versi pertama memakai
`untuk b dalam daftar { variabel nilai = ... }` yang tidak pernah membuat slot
prescan, sehingga tidak mereproduksi kasusnya. Pola diganti menjadi
`variabel etag` + assignment ulang — persis bentuk di `pustaka/web.idk:626` —
barulah merah.

Suite penuh: 328/328. Panduan: 207/207.

## Batasan yang diketahui

Masih lokal saja. Global, parameter, impor, fungsi/kelas yang tak terpakai
belum diperiksa — itu butuh analisis reachability lintas-modul, dan AGENTS.md
butir 4 memang meminta rancangan tersendiri sebelum melangkah ke sana.
