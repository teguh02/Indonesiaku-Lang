// kripto.c -- native internal modul `kripto` (Fase 18 V1).
//
// ATURAN BERKAS INI: tidak ada algoritme kriptografi yang ditulis di sini.
// AES-256-GCM, ChaCha20-Poly1305, HKDF dan perbandingan waktu-tetap semuanya
// dipanggil dari mbedTLS yang sudah di-vendor untuk TLS (src/vendor/mbedtls).
// Yang dilakukan berkas ini hanya:
//   1. memvalidasi argumen (panjang kunci/nonce, ukuran data, nama algoritme);
//   2. menyalin daftar bita / teks VM ke buffer malloc (hashInputBytes) --
//      tidak ada pointer ke heap GC yang dipegang selama mbedTLS bekerja;
//   3. menghapus (zeroize) setiap salinan kunci dan plaintext sebelum free.
//
// Kebijakan galat: argumen salah -> GalatTipe/GalatNilai (bisa ditangkap).
// Kegagalan AUTENTIKASI tidak dilaporkan sebagai galat di sini: _kripto_aead_buka
// mengembalikan `kosong`, lalu pustaka/kripto.idk menaikkan GalatKripto dengan
// SATU pesan untuk semua penyebab (tag/kunci/nonce/AAD salah) -- membedakannya
// hanya memberi orakel kepada penyerang.

#include <stdlib.h>
#include <string.h>

#include "kripto.h"
#include "native.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

#include "mbedtls/gcm.h"
#include "mbedtls/chachapoly.h"
#include "mbedtls/hkdf.h"
#include "mbedtls/md.h"
#include "mbedtls/constant_time.h"
#include "mbedtls/platform_util.h"

#define KRIPTO_KUNCI_LEN 32
#define KRIPTO_NONCE_LEN 12
#define KRIPTO_TAG_LEN 16
// Bita = daftar bulat (16 bita per elemen di VM), jadi 64 MiB data sudah
// ~1 GiB memori VM. Batas ini mencegah satu panggilan menghabiskan memori,
// dan menjaga panjang tetap muat di `int` milik ValueArray.
#define KRIPTO_DATA_MAKS (64u * 1024u * 1024u)
// RFC 5869: keluaran HKDF paling banyak 255 * HashLen.
#define KRIPTO_HKDF_MAKS (255 * 32)

typedef enum { ALG_TIDAK_DIKENAL, ALG_AES_256_GCM, ALG_CHACHA20_POLY1305 } AlgAead;

typedef struct {
    uint8_t* data;
    size_t len;
} Penyangga;

static void lepasRahasia(Penyangga* p) {
    if (p->data != NULL) {
        mbedtls_platform_zeroize(p->data, p->len);
        free(p->data);
    }
    p->data = NULL;
    p->len = 0;
}

static Value daftarBita(const uint8_t* buf, size_t n) {
    ObjList* hasil = newList();
    push(OBJ_VAL(hasil));  // lindungi dari GC selagi diisi
    for (size_t i = 0; i < n; i++) {
        writeValueArray(&hasil->items, INT_VAL(buf[i]));
    }
    pop();
    return OBJ_VAL(hasil);
}

static AlgAead bacaAlgoritme(Value v, const char* nama) {
    if (!IS_STRING(v)) {
        nativeRaiseKelas(vm.galatTipe, "%s(): algoritme harus teks", nama);
        return ALG_TIDAK_DIKENAL;
    }
    const char* s = AS_CSTRING(v);
    if (strcmp(s, "aes-256-gcm") == 0) return ALG_AES_256_GCM;
    if (strcmp(s, "chacha20-poly1305") == 0) return ALG_CHACHA20_POLY1305;
    nativeRaiseKelas(vm.galatNilai,
                     "%s(): algoritme '%s' tidak didukung (pakai \"aes-256-gcm\" "
                     "atau \"chacha20-poly1305\")", nama, s);
    return ALG_TIDAK_DIKENAL;
}

// Baca satu argumen bita ke `out`. `wajibLen` 0 berarti panjang bebas
// (dibatasi KRIPTO_DATA_MAKS).
static bool bacaBita(Value v, const char* nama, const char* peran,
                     size_t wajibLen, Penyangga* out) {
    if (!hashInputBytes(v, nama, &out->data, &out->len)) return false;
    if (wajibLen != 0 && out->len != wajibLen) {
        nativeRaiseKelas(vm.galatNilai, "%s(): %s harus %d bita, bukan %d",
                         nama, peran, (int)wajibLen, (int)out->len);
        lepasRahasia(out);
        return false;
    }
    if (out->len > KRIPTO_DATA_MAKS) {
        nativeRaiseKelas(vm.galatNilai, "%s(): %s melebihi batas 64 MiB", nama, peran);
        lepasRahasia(out);
        return false;
    }
    return true;
}

// Argumen bersama segel/buka: (algoritme, kunci, nonce, data, aad).
typedef struct {
    AlgAead alg;
    Penyangga kunci, nonce, data, aad;
} ArgAead;

static void lepasArgAead(ArgAead* a) {
    lepasRahasia(&a->kunci);
    lepasRahasia(&a->nonce);
    lepasRahasia(&a->data);
    lepasRahasia(&a->aad);
}

static bool bacaArgAead(int argCount, Value* args, const char* nama, ArgAead* a) {
    memset(a, 0, sizeof(*a));
    if (argCount != 5) {
        nativeRaiseKelas(vm.galatTipe,
                         "%s() memerlukan 5 argumen (algoritme, kunci, nonce, data, aad)", nama);
        return false;
    }
    a->alg = bacaAlgoritme(args[0], nama);
    if (a->alg == ALG_TIDAK_DIKENAL) return false;
    if (!bacaBita(args[1], nama, "kunci", KRIPTO_KUNCI_LEN, &a->kunci) ||
        !bacaBita(args[2], nama, "nonce", KRIPTO_NONCE_LEN, &a->nonce) ||
        !bacaBita(args[3], nama, "data", 0, &a->data) ||
        !bacaBita(args[4], nama, "aad", 0, &a->aad)) {
        lepasArgAead(a);
        return false;
    }
    return true;
}

// _kripto_aead_segel(alg, kunci, nonce, pesan, aad) -> daftar bita sandi ‖ tag.
Value nativeFnKriptoAeadSegel(int argCount, Value* args) {
    const char* nama = "kripto.aead_enkripsi";
    ArgAead a;
    if (!bacaArgAead(argCount, args, nama, &a)) return KOSONG_VAL;

    size_t n = a.data.len;
    uint8_t* keluar = (uint8_t*)malloc(n + KRIPTO_TAG_LEN);
    if (keluar == NULL) {
        lepasArgAead(&a);
        nativeRaise("%s(): kehabisan memori", nama);
        return KOSONG_VAL;
    }

    int rc;
    if (a.alg == ALG_AES_256_GCM) {
        mbedtls_gcm_context ctx;
        mbedtls_gcm_init(&ctx);
        rc = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, a.kunci.data, 256);
        if (rc == 0) {
            rc = mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT, n,
                                           a.nonce.data, a.nonce.len,
                                           a.aad.data, a.aad.len,
                                           a.data.data, keluar,
                                           KRIPTO_TAG_LEN, keluar + n);
        }
        mbedtls_gcm_free(&ctx);
    } else {
        mbedtls_chachapoly_context ctx;
        mbedtls_chachapoly_init(&ctx);
        rc = mbedtls_chachapoly_setkey(&ctx, a.kunci.data);
        if (rc == 0) {
            rc = mbedtls_chachapoly_encrypt_and_tag(&ctx, n, a.nonce.data,
                                                    a.aad.data, a.aad.len,
                                                    a.data.data, keluar, keluar + n);
        }
        mbedtls_chachapoly_free(&ctx);
    }
    lepasArgAead(&a);

    if (rc != 0) {
        mbedtls_platform_zeroize(keluar, n + KRIPTO_TAG_LEN);
        free(keluar);
        nativeRaise("%s(): enkripsi gagal (kode mbedTLS %d)", nama, rc);
        return KOSONG_VAL;
    }
    Value hasil = daftarBita(keluar, n + KRIPTO_TAG_LEN);
    free(keluar);  // isinya ciphertext, tidak rahasia
    return hasil;
}

// _kripto_aead_buka(alg, kunci, nonce, sandi ‖ tag, aad) -> daftar bita pesan,
// atau `kosong` bila autentikasi gagal. Tag diperiksa mbedTLS dalam waktu
// tetap SEBELUM plaintext dilepas; pada kegagalan buffer keluaran dihapus.
Value nativeFnKriptoAeadBuka(int argCount, Value* args) {
    const char* nama = "kripto.aead_dekripsi";
    ArgAead a;
    if (!bacaArgAead(argCount, args, nama, &a)) return KOSONG_VAL;

    // Data lebih pendek dari tag pasti palsu/terpotong: perlakukan sebagai
    // kegagalan autentikasi, bukan galat argumen, agar penyebabnya tidak bocor.
    if (a.data.len < KRIPTO_TAG_LEN) {
        lepasArgAead(&a);
        return KOSONG_VAL;
    }
    size_t n = a.data.len - KRIPTO_TAG_LEN;
    const uint8_t* tag = a.data.data + n;
    uint8_t* keluar = (uint8_t*)malloc(n == 0 ? 1 : n);
    if (keluar == NULL) {
        lepasArgAead(&a);
        nativeRaise("%s(): kehabisan memori", nama);
        return KOSONG_VAL;
    }

    int rc;
    bool authGagal = false;
    if (a.alg == ALG_AES_256_GCM) {
        mbedtls_gcm_context ctx;
        mbedtls_gcm_init(&ctx);
        rc = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, a.kunci.data, 256);
        if (rc == 0) {
            rc = mbedtls_gcm_auth_decrypt(&ctx, n, a.nonce.data, a.nonce.len,
                                          a.aad.data, a.aad.len,
                                          tag, KRIPTO_TAG_LEN,
                                          a.data.data, keluar);
            authGagal = (rc == MBEDTLS_ERR_GCM_AUTH_FAILED);
        }
        mbedtls_gcm_free(&ctx);
    } else {
        mbedtls_chachapoly_context ctx;
        mbedtls_chachapoly_init(&ctx);
        rc = mbedtls_chachapoly_setkey(&ctx, a.kunci.data);
        if (rc == 0) {
            rc = mbedtls_chachapoly_auth_decrypt(&ctx, n, a.nonce.data,
                                                 a.aad.data, a.aad.len,
                                                 tag, a.data.data, keluar);
            authGagal = (rc == MBEDTLS_ERR_CHACHAPOLY_AUTH_FAILED);
        }
        mbedtls_chachapoly_free(&ctx);
    }
    lepasArgAead(&a);

    Penyangga pesan = { keluar, n };
    if (rc != 0) {
        lepasRahasia(&pesan);
        if (authGagal) return KOSONG_VAL;
        nativeRaise("%s(): dekripsi gagal (kode mbedTLS %d)", nama, rc);
        return KOSONG_VAL;
    }
    Value hasil = daftarBita(pesan.data, pesan.len);
    lepasRahasia(&pesan);
    return hasil;
}

// _kripto_hkdf_sha256(ikm, garam, info, panjang) -> daftar bita (RFC 5869).
Value nativeFnKriptoHkdfSha256(int argCount, Value* args) {
    const char* nama = "kripto.hkdf_sha256";
    if (argCount != 4) {
        nativeRaiseKelas(vm.galatTipe, "%s() memerlukan 4 argumen (ikm, garam, info, panjang)", nama);
        return KOSONG_VAL;
    }
    if (!IS_INT(args[3])) {
        nativeRaiseKelas(vm.galatTipe, "%s(): panjang harus bilangan bulat", nama);
        return KOSONG_VAL;
    }
    int64_t panjang = AS_INT(args[3]);
    if (panjang < 1 || panjang > KRIPTO_HKDF_MAKS) {
        nativeRaiseKelas(vm.galatNilai, "%s(): panjang harus 1..%d", nama, KRIPTO_HKDF_MAKS);
        return KOSONG_VAL;
    }
    Penyangga ikm = {0}, garam = {0}, info = {0};
    if (!bacaBita(args[0], nama, "ikm", 0, &ikm)) return KOSONG_VAL;
    if (!bacaBita(args[1], nama, "garam", 0, &garam)) {
        lepasRahasia(&ikm);
        return KOSONG_VAL;
    }
    if (!bacaBita(args[2], nama, "info", 0, &info)) {
        lepasRahasia(&ikm);
        lepasRahasia(&garam);
        return KOSONG_VAL;
    }
    uint8_t okm[KRIPTO_HKDF_MAKS];
    int rc = mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                          garam.data, garam.len, ikm.data, ikm.len,
                          info.data, info.len, okm, (size_t)panjang);
    lepasRahasia(&ikm);
    lepasRahasia(&garam);
    lepasRahasia(&info);
    if (rc != 0) {
        mbedtls_platform_zeroize(okm, sizeof(okm));
        nativeRaise("%s(): gagal menurunkan kunci (kode mbedTLS %d)", nama, rc);
        return KOSONG_VAL;
    }
    Value hasil = daftarBita(okm, (size_t)panjang);
    mbedtls_platform_zeroize(okm, sizeof(okm));
    return hasil;
}

// _kripto_sama(a, b) -> logika. Waktu bergantung HANYA pada panjang, tidak
// pada isi -- untuk membandingkan tag MAC, token sesi, hash sandi. Panjang
// yang berbeda langsung `salah` (panjang tag bukan rahasia).
Value nativeFnKriptoSama(int argCount, Value* args) {
    const char* nama = "kripto.sama_aman";
    if (argCount != 2) {
        nativeRaiseKelas(vm.galatTipe, "%s() memerlukan 2 argumen", nama);
        return KOSONG_VAL;
    }
    Penyangga a = {0}, b = {0};
    if (!bacaBita(args[0], nama, "a", 0, &a)) return KOSONG_VAL;
    if (!bacaBita(args[1], nama, "b", 0, &b)) {
        lepasRahasia(&a);
        return KOSONG_VAL;
    }
    bool sama = a.len == b.len && mbedtls_ct_memcmp(a.data, b.data, a.len) == 0;
    lepasRahasia(&a);
    lepasRahasia(&b);
    return BOOL_VAL(sama);
}
