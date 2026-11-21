#ifndef indonesiaku_kripto_h
#define indonesiaku_kripto_h

#include "value.h"

// Native internal untuk pustaka/kripto.idk (Fase 18 V1). Semua primitif
// berasal dari mbedTLS yang di-vendor; berkas ini hanya validasi argumen,
// konversi daftar bita <-> buffer C, dan penghapusan (zeroize) buffer rahasia.
// Nama ber-awalan '_' karena API publiknya ada di modul `kripto`.

Value nativeFnKriptoAeadSegel(int argCount, Value* args);   // _kripto_aead_segel
Value nativeFnKriptoAeadBuka(int argCount, Value* args);    // _kripto_aead_buka
Value nativeFnKriptoHkdfSha256(int argCount, Value* args);  // _kripto_hkdf_sha256
Value nativeFnKriptoSama(int argCount, Value* args);        // _kripto_sama

#endif
