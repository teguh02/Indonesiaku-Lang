# 9. Embedding: Menjalankan Indonesiaku dari Program C

Indonesiaku bisa dipakai sebagai bahasa skrip **di dalam** program C Anda
sendiri — untuk berkas konfigurasi yang bisa berlogika, aturan yang boleh
diubah pengguna tanpa kompilasi ulang, atau sistem plugin sederhana.

Seluruh contoh di halaman ini dikompilasi dan dijalankan oleh
`tests/embedding.sh` pada setiap kali CI berjalan, jadi tidak ada potongan
kode di sini yang hanya "kira-kira benar".

## 9.1 Membangun pustaka

```bash
make libindk.a
```

Hasilnya `libindk.a`, berisi seluruh objek `indk` **kecuali** `src/main.o` —
`main()` adalah milik program Anda.

Kompilasi program Anda terhadapnya:

```bash
cc program.c libindk.a -Isrc -Isrc/vendor/mbedtls/include -lm -o program
```

Di **Windows (MinGW)** tambahkan pustaka soket dan kripto sistem, sama seperti
`LDFLAGS` di `Makefile` — tanpa keduanya penautan gagal dengan
`undefined reference to __imp_WSAStartup`:

```bash
cc program.c libindk.a -Isrc -Isrc/vendor/mbedtls/include -lm -lws2_32 -lbcrypt -o program
```

## 9.2 Contoh terkecil

```c
#include <stdio.h>
#include "indk.h"

int main(void) {
    indk_mulai();
    indk_jalankan("cetak(\"halo dari Indonesiaku\")");
    indk_selesai();
    return 0;
}
```

`indk_mulai()` wajib dipanggil sebelum fungsi lain, dan `indk_selesai()`
membebaskan seluruh memori runtime.

## 9.3 Memanggil fungsi C dari Indonesiaku

```c
#include "indk.h"

static Value tambah_dua(int jumlah, Value* argumen) {
    if (jumlah != 1 || !IS_NUMBER(argumen[0])) return KOSONG_VAL;
    if (IS_INT(argumen[0])) return INT_VAL(AS_INT(argumen[0]) + 2);
    return NUMBER_VAL(AS_NUMBER(argumen[0]) + 2);
}

int main(void) {
    indk_mulai();
    indk_daftar_fungsi("tambah_dua", tambah_dua);
    indk_jalankan("cetak(tambah_dua(40))");   /* 42 */
    indk_selesai();
}
```

> **Bulat vs pecahan.** Indonesiaku membedakan bilangan bulat (`INT_VAL`) dari
> pecahan (`NUMBER_VAL`). `NUMBER_VAL(42)` dicetak sebagai `42.0`, sedangkan
> `INT_VAL(42)` dicetak `42`. Periksa `IS_INT()` dan kembalikan tipe yang sama
> dengan masukan, supaya fungsi C Anda tidak terlihat berbeda dari operasi
> bawaan yang setara.

## 9.4 Membaca hasil kembali ke C

```c
char buf[128];
indk_jalankan("pesan = \"selesai\"");
if (indk_ambil_global_teks("pesan", buf, sizeof buf)) {
    printf("%s\n", buf);     /* selesai */
}
```

Fungsi ini membaca variabel global skrip, mengembalikan `false` bila namanya
tidak ada, dan selalu memotong dengan aman agar muat di `maks`.

Pembacaan **tidak pernah menjalankan `_teks_` milik pengguna**: sebuah metode
ajaib bisa menaikkan galat atau memanggil balik ke C di tengah pembacaan.

## 9.5 Galat tidak mematikan program Anda

```c
IndkHasil h = indk_jalankan("fungsi {{{");
/* h == INDK_GALAT_KOMPILASI, proses tetap hidup */
```

| Nilai | Arti |
|---|---|
| `INDK_OK` | berhasil |
| `INDK_GALAT_KOMPILASI` | sumber tidak bisa dikompilasi, atau berkas tak terbaca |
| `INDK_GALAT_RUNTIME` | galat saat berjalan (mis. `naikkan` yang tak tertangkap) |

Pesan galat tetap dicetak ke stderr seperti biasa.

## 9.6 Menjalankan berkas

```c
if (indk_jalankan_berkas("aturan.idk") != INDK_OK) {
    /* tangani */
}
```

## 9.7 Batas yang perlu Anda tahu

Dua batas ini adalah **keputusan desain** (K22), bukan cacat sementara:

1. **Satu runtime per proses.** VM disimpan sebagai variabel global tunggal,
   jadi tidak ada handle seperti `lua_State*`. Anda boleh memanggil
   `indk_mulai()` → `indk_selesai()` berkali-kali secara berurutan, tetapi
   tidak boleh punya dua runtime hidup bersamaan.
2. **Tidak aman untuk banyak utas.** VM ini satu utas dan tanpa kunci. Jangan
   memanggil fungsi mana pun dari dua utas sekaligus.

Menghilangkan keduanya menuntut mengalirkan pointer VM ke hampir seluruh
`vm.c`, `compiler.c`, dan `native.c` — perombakan besar yang belum dibenarkan
oleh kebutuhan nyata.

## 9.8 Daftar lengkap API

Semuanya ada di `src/indk.h`:

| Fungsi | Guna |
|---|---|
| `indk_mulai()` | menyiapkan runtime |
| `indk_selesai()` | merobohkan runtime |
| `indk_versi()` | versi pustaka, mis. `"1.5.0"` |
| `indk_jalankan(sumber)` | menjalankan kode dari teks |
| `indk_jalankan_berkas(jalur)` | menjalankan berkas `.idk` |
| `indk_daftar_fungsi(nama, fungsi)` | mendaftarkan fungsi C sebagai builtin |
| `indk_ambil_global_teks(nama, keluar, maks)` | membaca global skrip sebagai teks |

Contoh lengkap yang bisa langsung dijalankan: `examples/embed/halo.c`.
