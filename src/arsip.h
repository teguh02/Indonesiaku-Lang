#ifndef indonesiaku_arsip_h
#define indonesiaku_arsip_h

// TAR, TAR.GZ and ZIP archives for Fase 23 (public API: pustaka/arsip.idk).
//
// Extraction is safe by default, and that is the point of doing it in C:
//   * every entry is inspected BEFORE anything is written (two passes), so a
//     rejected archive leaves the destination untouched;
//   * names that are absolute, contain '..', a backslash, ':' or control
//     characters are refused (zip-slip / path traversal);
//   * symbolic and hard links are refused (or skipped on request), and no
//     link is ever created;
//   * total size, entry count and compression ratio are capped, checked
//     while decompressing, not only against the (forgeable) headers;
//   * every path component below the destination is checked with lstat and
//     an existing link there is refused, never followed.
// ZIP decoding is miniz's (src/vendor/miniz); the TAR format is parsed here.

#include <stdint.h>
#include <stdio.h>

#include "value.h"

#define ARSIP_BATAS_NAMA 4096

// Shared by arsip.c (reading) and arsip_buat.c (writing). Normalises an
// archive name into `keluar` (at least ARSIP_BATAS_NAMA + 2 bytes):
// 1 = usable, 0 = names the root itself ("./"), -1 = refused (`*alasan`).
int arsipBersihkanNama(const char* masuk, char* keluar, size_t ukuran, const char** alasan);
uint64_t arsipUkuranBerkas(FILE* f);   // size of an open file; position reset to 0

Value nativeFnArsipDaftar(int argCount, Value* args);   // _arsip_daftar(jalur, batas_entri, batas_rasio)
Value nativeFnArsipEkstrak(int argCount, Value* args);  // _arsip_ekstrak(jalur, tujuan, ukuran, entri, rasio, lewati_tautan)
Value nativeFnArsipBuat(int argCount, Value* args);     // _arsip_buat(jalur, format, entri, tingkat)
Value nativeFnArsipJenis(int argCount, Value* args);    // _arsip_jenis(jalur): lstat type or kosong

#endif
