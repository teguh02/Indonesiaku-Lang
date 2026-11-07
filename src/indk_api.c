// indk_api.c — implementasi API embedding C.
//
// Berkas ini SENGAJA tipis: semua mesinnya sudah ada di vm.c/compiler.c dan
// terbukti lewat suite. Yang ditambahkan di sini hanya permukaan publik yang
// stabil, supaya penataan ulang internal tidak langsung merusak pemakai.

#include "indk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "object.h"
#include "vm.h"

void indk_mulai(void) {
    initVM();
}

void indk_selesai(void) {
    freeVM();
}

const char* indk_versi(void) {
    return INDK_VERSION;
}

static IndkHasil terjemahkan(InterpretResult hasil) {
    switch (hasil) {
        case INTERPRET_OK:
            return INDK_OK;
        case INTERPRET_COMPILE_ERROR:
            return INDK_GALAT_KOMPILASI;
        default:
            return INDK_GALAT_RUNTIME;
    }
}

IndkHasil indk_jalankan(const char* sumber) {
    if (sumber == NULL) return INDK_GALAT_KOMPILASI;
    return terjemahkan(interpret(sumber));
}

IndkHasil indk_jalankan_berkas(const char* jalur) {
    if (jalur == NULL) return INDK_GALAT_KOMPILASI;

    FILE* berkas = fopen(jalur, "rb");
    if (berkas == NULL) return INDK_GALAT_KOMPILASI;

    if (fseek(berkas, 0L, SEEK_END) != 0) {
        fclose(berkas);
        return INDK_GALAT_KOMPILASI;
    }
    long ukuran = ftell(berkas);
    if (ukuran < 0) {
        fclose(berkas);
        return INDK_GALAT_KOMPILASI;
    }
    rewind(berkas);

    char* sumber = (char*)malloc((size_t)ukuran + 1);
    if (sumber == NULL) {
        fclose(berkas);
        return INDK_GALAT_KOMPILASI;
    }

    size_t terbaca = fread(sumber, sizeof(char), (size_t)ukuran, berkas);
    fclose(berkas);
    if (terbaca < (size_t)ukuran) {
        free(sumber);
        return INDK_GALAT_KOMPILASI;
    }
    sumber[terbaca] = '\0';

    IndkHasil hasil = terjemahkan(interpret(sumber));
    free(sumber);
    return hasil;
}

void indk_daftar_fungsi(const char* nama, IndkFungsiC fungsi) {
    if (nama == NULL || fungsi == NULL) return;
    defineNative(nama, (NativeFn)fungsi);
}

bool indk_ambil_global_teks(const char* nama, char* keluar, size_t maks) {
    if (nama == NULL || keluar == NULL || maks == 0) return false;
    // Global skrip hidup di modul "__utama__", bukan di vm.globals (yang hanya
    // berisi builtin + kelas galat). Tanpa modul itu belum ada kode yang
    // pernah dijalankan, jadi tidak ada global untuk dibaca.
    if (vm.mainModule == NULL) return false;

    ObjString* kunci = copyString(nama, (int)strlen(nama));
    // copyString mengalokasi: akar-kan supaya GC yang terpicu di bawah tidak
    // membebaskannya (aturan GC di AGENTS.md).
    push(OBJ_VAL(kunci));

    Value nilai;
    bool ada = tableGet(&vm.mainModule->globals, kunci, &nilai);
    pop();
    if (!ada) return false;

    TextBuf buf;
    initTextBuf(&buf);
    // callMagic = false: membaca sebuah global tidak boleh diam-diam
    // menjalankan _teks_ milik pengguna, yang bisa menaikkan galat atau
    // bahkan memanggil balik ke C di tengah pembacaan.
    if (!formatValue(&buf, nilai, false, false)) {
        freeTextBuf(&buf);
        return false;
    }

    size_t panjang = (size_t)buf.length;
    if (panjang > maks - 1) panjang = maks - 1;
    memcpy(keluar, buf.chars, panjang);
    keluar[panjang] = '\0';
    freeTextBuf(&buf);
    return true;
}
