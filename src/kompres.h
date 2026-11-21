#ifndef indonesiaku_kompres_h
#define indonesiaku_kompres_h

// Compression for Fase 23: zlib/gzip/raw deflate on top of the vendored
// miniz (src/vendor/miniz, see PROVENANCE.md). Nothing here implements
// deflate itself. Public names live in pustaka/kompres.idk; the natives
// below are `_`-prefixed internals.
//
// Binary data follows the repo convention: a `daftar` of bulat 0..255
// (pustaka/bita.idk, soket_baca_bita, db BLOB). A `teks` is also accepted
// as input and read as its raw bytes.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "value.h"

typedef enum {
    KOMPRES_ZLIB,
    KOMPRES_GZIP,
    KOMPRES_MENTAH   // raw deflate, no header/trailer
} FormatKompres;

// Bytes of `v` (teks, or daftar/tupel of bulat 0..255). For a teks the
// pointer aims into the string itself (`*milik` = false: do not free; the
// string must stay rooted, which native args are). Otherwise a malloc'd
// copy (`*milik` = true). False after raising GalatTipe/GalatNilai.
bool kompresAmbilBita(Value v, const char* fungsi, const uint8_t** data,
                      size_t* panjang, bool* milik);

// A new daftar of bulat 0..255 holding `n` bytes. GC-safe.
Value kompresDaftarBita(const uint8_t* data, size_t n);

// --- Streaming inflate (zlib, gzip incl. multi-member, raw) ---------------

// Source callback: up to `n` bytes into `buf`; 0 = end of input; on a read
// error set *galat = true.
typedef size_t (*KompresSumber)(void* ctx, uint8_t* buf, size_t n, bool* galat);

typedef struct AliranInflasi AliranInflasi;

AliranInflasi* aliranInflasiBaru(FormatKompres format, KompresSumber sumber, void* ctx);
// Next decompressed bytes into `out`. Returns >0 bytes, 0 at the clean end
// of the stream, -1 on error (message via aliranInflasiGalat()).
long aliranInflasiBaca(AliranInflasi* a, uint8_t* out, size_t n);
const char* aliranInflasiGalat(const AliranInflasi* a);
uint64_t aliranInflasiMasuk(const AliranInflasi* a);   // compressed bytes consumed
void aliranInflasiTutup(AliranInflasi* a);

// --- Streaming deflate into a FILE* ----------------------------------------

typedef struct PenulisKompres PenulisKompres;

PenulisKompres* penulisKompresBaru(FormatKompres format, int tingkat, FILE* keluar);
bool penulisKompresTulis(PenulisKompres* p, const uint8_t* data, size_t n);
// Flushes the trailer. False on any earlier or final write error.
bool penulisKompresSelesai(PenulisKompres* p);
uint64_t penulisKompresUkuran(const PenulisKompres* p);   // bytes written so far
void penulisKompresTutup(PenulisKompres* p);

// --- Natives ----------------------------------------------------------------
Value nativeFnKompres(int argCount, Value* args);          // _kompres(data, format, tingkat)
Value nativeFnDekompres(int argCount, Value* args);        // _dekompres(data, format, batas)
Value nativeFnKompresBerkas(int argCount, Value* args);    // _kompres_berkas(sumber, tujuan, format, tingkat)
Value nativeFnDekompresBerkas(int argCount, Value* args);  // _dekompres_berkas(sumber, tujuan, format, batas)
Value nativeFnCrc32(int argCount, Value* args);            // _crc32(data, awal)
Value nativeFnAdler32(int argCount, Value* args);          // _adler32(data, awal)
Value nativeFnBacaBerkasBita(int argCount, Value* args);   // _baca_berkas_bita(jalur, batas)
Value nativeFnTulisBerkasBita(int argCount, Value* args);  // _tulis_berkas_bita(jalur, data)

#endif
