// indk.h — API embedding C untuk Indonesiaku.
//
// Satu-satunya header yang perlu di-#include oleh program C yang ingin
// menjalankan Indonesiaku di dalam prosesnya sendiri.
//
// BATAS YANG DISADARI (lihat K22 di docs/ANALISIS/03_KEPUTUSAN_DESAIN.md):
//
//   1. SATU RUNTIME PER PROSES. VM disimpan sebagai variabel global tunggal
//      (`VM vm` di src/vm.c), jadi tidak ada handle seperti `lua_State*`.
//      Memanggil indk_mulai() dua kali tanpa indk_selesai() di antaranya
//      adalah kesalahan pemakaian.
//   2. TIDAK AMAN UNTUK BANYAK UTAS. VM ini satu utas dan tidak punya kunci.
//      Jangan panggil fungsi mana pun di sini dari dua utas sekaligus.
//
// Keduanya adalah batas desain yang dipilih sadar, bukan cacat sementara:
// menghilangkannya menuntut mengalirkan pointer VM ke hampir seluruh
// vm.c/compiler.c/native.c.

#ifndef INDK_H
#define INDK_H

#include <stdbool.h>
#include <stddef.h>

#include "value.h"

#ifdef __cplusplus
extern "C" {
#endif

// Hasil eksekusi. Sengaja dipisahkan dari InterpretResult internal supaya
// permukaan publik tidak ikut berubah bila enum internal ditata ulang.
typedef enum {
    INDK_OK = 0,
    INDK_GALAT_KOMPILASI = 1,
    INDK_GALAT_RUNTIME = 2
} IndkHasil;

// Tanda tangan fungsi C yang bisa didaftarkan sebagai builtin.
// Sama persis dengan NativeFn internal; alias supaya pemakai tidak perlu
// menyertakan object.h secara langsung.
typedef Value (*IndkFungsiC)(int jumlah, Value* argumen);

// Menyiapkan runtime. Wajib dipanggil sebelum fungsi lain mana pun.
void indk_mulai(void);

// Merobohkan runtime dan membebaskan seluruh memorinya.
void indk_selesai(void);

// Versi pustaka (INDK_VERSION), mis. "1.4.0".
const char* indk_versi(void);

// Menjalankan kode sumber Indonesiaku.
IndkHasil indk_jalankan(const char* sumber);

// Menjalankan berkas .idk. Mengembalikan INDK_GALAT_KOMPILASI bila berkas
// tidak terbaca.
IndkHasil indk_jalankan_berkas(const char* jalur);

// Mendaftarkan fungsi C sebagai builtin global bernama `nama`.
// Harus dipanggil setelah indk_mulai() dan sebelum kode yang memakainya.
void indk_daftar_fungsi(const char* nama, IndkFungsiC fungsi);

// Menyalin representasi teks sebuah variabel global skrip ke `keluar`.
// Mengembalikan false bila nama tidak ada; isi `keluar` lalu tidak disentuh.
// Hasil selalu ber-NUL dan dipotong bila melebihi `maks`.
bool indk_ambil_global_teks(const char* nama, char* keluar, size_t maks);

#ifdef __cplusplus
}
#endif

#endif
