// halo.c — contoh embedding Indonesiaku ke dalam program C.
//
// Kompilasi:
//     make libindk.a
//     cc examples/embed/halo.c libindk.a -Isrc -lm -o halo
//     ./halo
//
// Contoh ini benar-benar dikompilasi dan dijalankan oleh tests/embedding.sh,
// jadi ia tidak bisa basi diam-diam.

#include <stdio.h>
#include <string.h>

#include "indk.h"

// Fungsi C yang akan dipanggil dari kode Indonesiaku.
//
// PENTING: Indonesiaku membedakan bilangan BULAT (VAL_INT, K2) dari pecahan
// (VAL_NUMBER). `NUMBER_VAL(42)` mencetak "42.0", sedangkan `INT_VAL(42)`
// mencetak "42". Kembalikan tipe yang sama dengan masukan, atau hasil fungsi C
// terlihat berbeda dari operasi bawaan yang setara.
static Value tambah_dua(int jumlah, Value* argumen) {
    if (jumlah != 1 || !IS_NUMBER(argumen[0])) return KOSONG_VAL;
    if (IS_INT(argumen[0])) return INT_VAL(AS_INT(argumen[0]) + 2);
    return NUMBER_VAL(AS_NUMBER(argumen[0]) + 2);
}

int main(void) {
    indk_mulai();

    printf("versi: %s\n", indk_versi());

    indk_daftar_fungsi("tambah_dua", tambah_dua);

    // Memanggil fungsi C dari Indonesiaku.
    if (indk_jalankan("cetak(tambah_dua(40))") != INDK_OK) {
        fprintf(stderr, "gagal menjalankan\n");
        indk_selesai();
        return 1;
    }

    // Membaca variabel global yang di-set skrip.
    indk_jalankan("pesan = \"halo dari Indonesiaku\"");
    char buf[128];
    if (indk_ambil_global_teks("pesan", buf, sizeof buf)) {
        printf("global pesan: %s\n", buf);
    }

    // Galat dilaporkan sebagai nilai balik, bukan mematikan proses induk.
    IndkHasil h = indk_jalankan("ini bukan sintaks yang sah {{{");
    printf("hasil sintaks salah: %d\n", (int)h);

    indk_selesai();
    return 0;
}
