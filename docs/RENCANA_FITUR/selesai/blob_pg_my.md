# Rencana Fitur — Dukungan BLOB/bytea di driver PostgreSQL & MySQL

**Status:** selesai
**Fase:** 10 (Portabilitas & pengerasan)
**Target rilis:** `v1.7.0`
**Penulis:** Teguh Rijanandi
**Tanggal:** 2026-11-07

## Masalah

Fase 10 butir 7 menuntut uji driver DB mencakup **"BLOB"**. Audit menemukan
kata itu tidak muncul sama sekali di `tests/pg_driver.sh`, `tests/my_driver.sh`,
maupun `tests/db_bersama.sh` — dan ketika diuji langsung ke PostgreSQL 16
sungguhan, hasilnya **bukan sekadar tidak teruji, melainkan rusak**.

### Bukti empiris

Menyimpan `[255, 254, 65, 0, 128]` (lima byte yang sengaja **bukan UTF-8 sah**)
ke kolom `bytea`:

```
 id | panjang_byte |       isi_escape
----+--------------+------------------------
  1 |           22 | [255, 254, 65, 0, 128]
```

Yang tersimpan adalah **22 byte teks ASCII** berisi representasi cetak dari
daftar itu, bukan 5 byte biner. SQLite pada mesin yang sama mengembalikan
`[255, 254, 65, 0, 128]` dengan benar.

Jadi program yang sama memberi hasil berbeda tergantung basis data — persis
jenis cacat yang Fase 10 ("portabilitas") ada untuk mencegah. Lebih buruk:
kegagalannya **diam**. Tidak ada galat; data cuma rusak.

## Akar masalah

Keduanya satu baris, dan keduanya pola yang sama: nilai dipaksa lewat
`ke_teks()` sebelum dikirim.

**`pustaka/pg.idk:279`** di `_jalankan()`:

```indonesiaku
variabel b = bita.ke_bita(ke_teks(p))
```

**`pustaka/my.idk:249`** di `_nilai_bita()`:

```indonesiaku
variabel t = _teks_bita(ke_teks(nilai))
```

`ke_teks([255, 254])` menghasilkan teks `"[255, 254]"`. Untuk angka dan teks
biasa ini kebetulan benar; untuk daftar bita ia merusak muatan.

## Rancangan

Satu cabang di tiap driver: bila parameter adalah **daftar berisi bilangan
bulat 0–255**, perlakukan sebagai byte mentah dan kirim apa adanya.

```indonesiaku
jika _adalah_bita(p) {
    b = p                      # sudah byte, jangan lewat ke_teks
} selain {
    b = bita.ke_bita(ke_teks(p))
}
```

Untuk PostgreSQL ada satu syarat tambahan: Bind saat ini memakai **format teks**
untuk semua parameter (`_i16(0)`). Byte mentah harus dikirim sebagai
**format biner**, yaitu format kode `1` per parameter, supaya server tidak
menafsirkannya sebagai literal teks `bytea`.

MySQL sudah memakai protokol biner (`tipe 253` = `MYSQL_TYPE_VAR_STRING` dengan
panjang lenenc), jadi cukup menghentikan konversi `ke_teks`.

### Yang TIDAK dikerjakan

- Tidak menambah tipe `Bita` baru ke bahasa. Daftar bilangan bulat sudah
  dipakai `bita.idk`, `soket_baca_bita`, dan driver SQLite; menambah tipe baru
  demi ini adalah perombakan yang tidak diminta.
- Tidak mengubah jalur baca (`SELECT`). `bytea` kembali sebagai teks
  `\x...` dari server; mengubahnya menjadi daftar bita otomatis akan memutus
  kode yang sudah berjalan. Didokumentasikan apa adanya, dan `bita.dari_hex()`
  disediakan bila pemakai mau mengubahnya sendiri.

## Pengujian

Ditambahkan ke `tests/pg_driver.sh` dan `tests/my_driver.sh`, dengan muatan
yang **sengaja bukan UTF-8 sah** (`0xFF 0xFE ... 0x00 0x80`) sehingga jalur
teks tidak bisa lolos secara kebetulan:

1. panjang byte di server **persis** sama dengan panjang yang dikirim
   (ini asersi yang gagal hari ini: 22 ≠ 5);
2. isi byte identik, diperiksa lewat `encode(isi,'escape')` / `HEX()`;
3. byte NUL di tengah tidak memotong nilai;
4. BLOB kosong tetap berbeda dari NULL.

Sabotase yang wajib membuatnya merah: kembalikan `ke_teks(p)`, dan paksa
format teks pada Bind PostgreSQL.

## Risiko

Perubahan menyentuh jalur parameter **setiap** query di kedua driver. Mitigasi:
seluruh suite DB yang sudah ada (`pg_driver`, `my_driver`, `db_bersama`,
`migrasi_sabotase`, `api_siswa_pg_server`) harus tetap hijau, karena parameter
non-bita harus melewati cabang lama tanpa perubahan perilaku.
