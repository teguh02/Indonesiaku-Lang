# Rencana Perbaikan: lingkup variabel `untuk` (T-U4, K21)

- **Status:** selesai
- **Target versi:** 1.4.1
- **Pemilik:** Teguh Rijanandi (keputusan K21); dikerjakan oleh Claude Code

## 1. Masalah

Temuan T-U4 (uji ekstensif Windows 1.4.0, `ALUR-FUNGSI-02`):

```indonesiaku
k = 99
untuk k dalam rentang(3) { }
cetak(k)          # 1.4.0: 99   — Python: 2
untuk q dalam rentang(3) { }
cetak(q)          # 1.4.0: Variabel tidak terdefinisi 'q' (exit 70) — Python: 2
```

`forStatement` membuat variabel loop sebagai lokal blok baru (`addLocal`).
Perilaku yang sama terjadi di dalam fungsi.

## 2. Sintaks usulan

Tidak ada sintaks baru.

## 3. Semantik

Lihat K21 di `docs/ANALISIS/03_KEPUTUSAN_DESAIN.md`. Ringkasnya, variabel
loop di-assign seperti `x = elemen`: global di tingkat skrip, lokal fungsi
(K3) di dalam fungsi, dan mengikuti `global`/`nonlokal`. Nilainya tetap
ada setelah loop. Variabel comprehension tetap privat.

## 4. Rencana implementasi

| Bagian | Perubahan |
|---|---|
| `src/compiler.c` | `forStatement` memakai `assignToName(varName)`, bukan lokal blok. `prescanFunctionBody` menandai nama setelah `untuk` di awal pernyataan sebagai lokal fungsi. `assignToName` kini menghormati `global x` (sebelumnya bisa menulis upvalue bernama sama; dipakai juga oleh `a, b = ...`). |

## 5. Pengujian

- Golden `examples/untuk_lingkup.idk`: nama lama tertimpa di skrip dan di
  fungsi, nama tetap ada setelah loop, comprehension tidak bocor dan tidak
  membuat lokal di fungsi, `global`, `nonlokal`, dan urutan kosong.
- Seluruh golden lama lulus tanpa perubahan. Tidak ada yang bergantung pada
  lingkup blok.
- `indk periksa` tidak menghasilkan temuan baru untuk variabel loop.

## 6. Dampak dokumentasi & tooling

- [x] `docs/ANALISIS/03_KEPUTUSAN_DESAIN.md` (K21)
- [x] `docs/SPESIFIKASI.md` §6
- [x] `website/panduan/03_PANDUAN_SINTAKS.md` (lingkup variabel `untuk`)
- [x] `AGENTS.md`
- [x] `CHANGELOG.md` (⚠️)

## 7. Risiko & alternatif

Kode yang sengaja membayangi nama di luar dengan variabel loop kini
menimpanya. Alternatif yang ditolak pemilik proyek: mempertahankan lingkup
blok dengan peringatan `indk periksa`, atau menundanya ke 2.0.
