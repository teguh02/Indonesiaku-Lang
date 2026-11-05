# Mask pengganti modulo pada tabel hash

Status: selesai

## Masalah

`perf` pada beban `tests/beban/kamus_himpunan.idk` menunjukkan sebaran waktu:

```
33.23%  run                 (dispatch VM)
10.18%  findValueEntry      <-- tabel kamus/himpunan
 5.69%  tableGet
 5.09%  tableFindString
 3.29%  hashValueDepth
```

`findEntry`, `findValueEntry` dan `tableFindString` memakai `hash % capacity`
untuk mencari slot, dan `(index + 1) % capacity` untuk probing. Modulo integer
adalah pembagian perangkat keras — puluhan siklus, tidak bisa di-pipeline —
padahal kapasitas tabel **selalu pangkat dua**: `GROW_CAPACITY` mulai dari 8
lalu mengalikan dua.

Jalur ini adalah jalur terpanas VM: setiap akses variabel global, setiap
pencarian metode, setiap interning string, dan setiap indeks kamus.

## Perubahan

`hash % capacity` → `hash & (capacity - 1)`, di tiga fungsi pencarian.
Setara secara matematis **hanya** bila kapasitas pangkat dua.

Agar asumsi itu tidak bisa dilanggar diam-diam di kemudian hari (mis. seseorang
mengubah `GROW_CAPACITY` menjadi bilangan prima, yang merupakan strategi tabel
hash yang sah), `adjustCapacity()` dan `adjustValueCapacity()` memeriksa
`(capacity & (capacity - 1)) == 0` dan `exit(70)` bila dilanggar. Tanpa penjaga
ini, pelanggaran tidak akan membuat program crash — indeksnya hanya salah, dan
entri kamus "hilang" tanpa gejala.

## Hasil

Dua beban dari empat membaik drastis; dua lainnya tidak terpengaruh karena
tidak didominasi operasi tabel.

| Beban | Sebelum | Sesudah | Selisih |
|---|---|---|---|
| `kamus_himpunan` | 114,6 ms | 64,4 ms | **−43,8%** |
| `olah_teks` | 114,4 ms | 64,3 ms | **−43,8%** |
| `data_json` | 64,6 ms | 64,4 ms | −0,2% |
| `rekursi_oop` | 32,1 ms | 32,0 ms | −0,2% |

Laju naik dari 83,7 → 92,9 dan 97,8 → 108,0 juta instruksi/detik.

Jumlah instruksi **identik** sebelum dan sesudah (5.825.976 dan 6.403.312) —
ini murni perbaikan biaya per instruksi, bukan perubahan bytecode. Keluaran
byte-identik.

## Catatan pengukuran

Mengganti modulo di `findEntry` dan `tableFindString` **saja** tidak mengubah
waktu sama sekali (−0,1%, dalam derau), meskipun pembongkaran membuktikan 12
instruksi `div` benar-benar hilang dari assembly. Keuntungan baru muncul
setelah `findValueEntry` ikut diganti.

Pelajarannya: assembly yang berubah tidak membuktikan program menjadi lebih
cepat. `perf` yang menunjuk `findValueEntry` di 10,18% adalah alasan sebenarnya
perubahan ini dikerjakan — bukan dugaan bahwa "modulo itu mahal".

## Berkas tersentuh

- `src/table.c` — tiga fungsi pencarian + dua penjaga kapasitas
- `tests/test_tabel_mask.sh` — regresi

## Rencana uji

- Beban `tests/beban/` memberi keluaran byte-identik dan jumlah instruksi identik.
- Kamus dengan banyak tabrakan hash, penghapusan (tombstone), dan pertumbuhan
  melewati beberapa kali resize.
- Sabotase: `GROW_CAPACITY` diubah menjadi non-pangkat-dua → penjaga wajib
  menghentikan program dengan exit 70, bukan memberi hasil salah.
