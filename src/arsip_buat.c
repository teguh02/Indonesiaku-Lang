// Creating TAR / TAR.GZ / ZIP archives -- the writing half of arsip.h.
//
// _arsip_buat(jalur, format, entri, tingkat): `entri` is a daftar of
// [nama, jenis, sumber, waktu] built by pustaka/arsip.idk, with jenis
// "berkas" (sumber = path on disk), "data" (sumber = teks / daftar bita) or
// "direktori". Names go through the same check as extraction, so an archive
// made here never contains a name ekstrak() would refuse. No VM allocation
// happens until the result, so nothing here needs GC rooting.

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "arsip.h"
#include "kompres.h"
#include "memory.h"
#include "object.h"
#include "sistem.h"
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES   // no crc32/inflate/... macros in our code
#include "vendor/miniz/miniz.h"
#include "vm.h"

#define BLOK 512
#define OKTAL_MAKS_12 077777777777ULL   // largest value in 11 octal digits

typedef struct {
    FILE* f;
    PenulisKompres* gz;   // tar.gz: everything goes through it
    char galat[600];
    bool gagal;
} PenulisArsip;

static bool gagalP(PenulisArsip* w, const char* pesan, const char* nama) {
    if (!w->gagal) {
        if (nama[0] == '\0') snprintf(w->galat, sizeof(w->galat), "arsip.buat(): %s", pesan);
        else snprintf(w->galat, sizeof(w->galat), "arsip.buat(): %s '%.300s'", pesan, nama);
    }
    w->gagal = true;
    return false;
}

static bool tarTulis(PenulisArsip* w, const void* data, size_t n) {
    if (w->gz != NULL) return penulisKompresTulis(w->gz, data, n);
    return n == 0 || fwrite(data, 1, n, w->f) == n;
}

static bool tarPad(PenulisArsip* w, uint64_t n) {
    static const uint8_t nol[BLOK];
    size_t pad = (size_t)((BLOK - n % BLOK) % BLOK);
    return tarTulis(w, nol, pad);
}

static void oktal(char* field, size_t lebar, uint64_t v) {
    snprintf(field, lebar, "%0*llo", (int)(lebar - 1), (unsigned long long)v);
}

// ustar header. Names that do not fit name/prefix get a PAX 'x' record.
static bool tarKepala(PenulisArsip* w, const char* nama, char tipe, uint64_t ukuran,
                      uint64_t waktu, unsigned mode) {
    uint8_t h[BLOK];
    memset(h, 0, sizeof(h));
    size_t n = strlen(nama);
    size_t potong = 0;   // index of the '/' splitting prefix and name
    bool muat = n <= 100;
    if (!muat && n <= 256) {
        for (size_t i = n - 1; i > 0; i--) {
            if (nama[i] == '/' && i <= 155 && i + 1 < n && n - i - 1 <= 100) { potong = i; muat = true; break; }
        }
    }
    if (!muat) {
        // "<len> path=<nama>\n", where <len> counts its own digits.
        size_t isi = n + 7, panjang = isi + 1;
        while ((size_t)snprintf(NULL, 0, "%zu", panjang) + isi != panjang) panjang++;
        char* rekam = malloc(panjang + 1);
        if (rekam == NULL) return gagalP(w, "memori tidak cukup untuk", nama);
        snprintf(rekam, panjang + 1, "%zu path=%s\n", panjang, nama);
        bool ok = tarKepala(w, "././@PaxHeader", 'x', panjang, waktu, 0644) &&
                  tarTulis(w, rekam, panjang) && tarPad(w, panjang);
        free(rekam);
        if (!ok) return false;
        memcpy(h, nama, 100);   // informative only; readers use the PAX path
    } else if (potong > 0) {
        memcpy(h + 345, nama, potong);
        memcpy(h, nama + potong + 1, n - potong - 1);
    } else {
        memcpy(h, nama, n);
    }
    oktal((char*)h + 100, 8, mode);
    oktal((char*)h + 108, 8, 0);
    oktal((char*)h + 116, 8, 0);
    if (ukuran <= OKTAL_MAKS_12) {
        oktal((char*)h + 124, 12, ukuran);
    } else {   // GNU base-256 for files of 8 GiB and more
        h[124] = 0x80;
        for (int i = 11; i >= 4; i--) { h[124 + i] = (uint8_t)ukuran; ukuran >>= 8; }
    }
    oktal((char*)h + 136, 12, waktu > OKTAL_MAKS_12 ? OKTAL_MAKS_12 : waktu);
    h[156] = (uint8_t)tipe;
    memcpy(h + 257, "ustar\0" "00", 8);
    memset(h + 148, ' ', 8);
    unsigned jumlah = 0;
    for (int i = 0; i < BLOK; i++) jumlah += h[i];
    snprintf((char*)h + 148, 8, "%06o", jumlah);
    h[155] = ' ';
    if (!tarTulis(w, h, BLOK)) return gagalP(w, "gagal menulis kepala untuk", nama);
    return true;
}

static bool salinBerkas(PenulisArsip* w, FILE* masuk, uint64_t ukuran, const char* nama) {
    uint8_t buf[64 * 1024];
    while (ukuran > 0) {
        size_t bagian = ukuran > sizeof(buf) ? sizeof(buf) : (size_t)ukuran;
        if (fread(buf, 1, bagian, masuk) != bagian) return gagalP(w, "berkas berubah atau gagal dibaca:", nama);
        if (!tarTulis(w, buf, bagian)) return gagalP(w, "gagal menulis", nama);
        ukuran -= bagian;
    }
    return true;
}

typedef struct {
    char nama[ARSIP_BATAS_NAMA + 2];
    char jenis;   // b(erkas) d(ata) r (direktori)
    Value sumber;
    uint64_t waktu;
} EntriBaru;

static bool ambilEntri(Value v, EntriBaru* e, PenulisArsip* w) {
    ValueArray* isi = sequenceItems(v);
    if (isi == NULL || isi->count != 4 || !IS_STRING(isi->values[0]) || !IS_STRING(isi->values[1])) {
        return gagalP(w, "entri harus [nama, jenis, sumber, waktu]", "");
    }
    const char* alasan = NULL;
    const char* asli = AS_STRING(isi->values[0])->chars;
    if (arsipBersihkanNama(asli, e->nama, sizeof(e->nama), &alasan) != 1) {
        snprintf(w->galat, sizeof(w->galat), "arsip.buat(): nama '%.300s' ditolak: %s", asli,
                 alasan != NULL ? alasan : "menunjuk akar arsip");
        w->gagal = true;
        return false;
    }
    const char* j = AS_STRING(isi->values[1])->chars;
    e->jenis = strcmp(j, "berkas") == 0 ? 'b' : strcmp(j, "data") == 0 ? 'd' : strcmp(j, "direktori") == 0 ? 'r' : 0;
    if (e->jenis == 0) return gagalP(w, "jenis entri tidak dikenal untuk", e->nama);
    e->sumber = isi->values[2];
    if (e->jenis == 'b' && !IS_STRING(e->sumber)) return gagalP(w, "sumber berkas harus jalur teks untuk", e->nama);
    Value t = isi->values[3];
    double d = IS_NUMBER(t) ? AS_NUMBER(t) : 0;
    e->waktu = d > 0 && d < 1e15 ? (uint64_t)d : 0;
    return true;
}

static bool tarEntri(PenulisArsip* w, EntriBaru* e) {
    if (e->jenis == 'r') {
        size_t n = strlen(e->nama);
        e->nama[n] = '/';
        e->nama[n + 1] = '\0';
        return tarKepala(w, e->nama, '5', 0, e->waktu, 0755);
    }
    if (e->jenis == 'd') {
        const uint8_t* data;
        size_t n;
        bool milik;
        if (!kompresAmbilBita(e->sumber, "arsip.buat", &data, &n, &milik)) { w->gagal = true; w->galat[0] = 0; return false; }
        bool ok = tarKepala(w, e->nama, '0', n, e->waktu, 0644) && tarTulis(w, data, n) && tarPad(w, n);
        if (milik) free((void*)data);
        return ok || gagalP(w, "gagal menulis", e->nama);
    }
    const char* jalur = AS_STRING(e->sumber)->chars;
    FILE* masuk = sistemBukaBerkas(jalur, "rb");
    if (masuk == NULL) return gagalP(w, "gagal membuka berkas sumber", jalur);
    uint64_t ukuran = arsipUkuranBerkas(masuk);
    bool ok = tarKepala(w, e->nama, '0', ukuran, e->waktu, 0644) &&
              salinBerkas(w, masuk, ukuran, jalur) && (tarPad(w, ukuran) || gagalP(w, "gagal menulis", e->nama));
    fclose(masuk);
    return ok;
}

static bool zipEntri(PenulisArsip* w, mz_zip_archive* z, EntriBaru* e, int tingkat) {
    MZ_TIME_T waktu = (MZ_TIME_T)(e->waktu > 0 ? e->waktu : 315532800);   // DOS dates start in 1980
    mz_bool ok;
    if (e->jenis == 'r') {
        size_t n = strlen(e->nama);
        e->nama[n] = '/';
        e->nama[n + 1] = '\0';
        // A non-NULL empty buffer: miniz passes it straight to fwrite(), and
        // fwrite(NULL, 1, 0, f) is undefined behaviour (UBSan reports it).
        static const char kosong[1] = { 0 };
        ok = mz_zip_writer_add_mem_ex_v2(z, e->nama, kosong, 0, NULL, 0, (mz_uint)tingkat, 0, 0, &waktu, NULL, 0, NULL, 0);
    } else if (e->jenis == 'd') {
        const uint8_t* data;
        size_t n;
        bool milik;
        if (!kompresAmbilBita(e->sumber, "arsip.buat", &data, &n, &milik)) { w->gagal = true; w->galat[0] = 0; return false; }
        ok = mz_zip_writer_add_mem_ex_v2(z, e->nama, data, n, NULL, 0, (mz_uint)tingkat, 0, 0, &waktu, NULL, 0, NULL, 0);
        if (milik) free((void*)data);
    } else {
        const char* jalur = AS_STRING(e->sumber)->chars;
        FILE* masuk = sistemBukaBerkas(jalur, "rb");
        if (masuk == NULL) return gagalP(w, "gagal membuka berkas sumber", jalur);
        uint64_t ukuran = arsipUkuranBerkas(masuk);
        ok = mz_zip_writer_add_cfile(z, e->nama, masuk, ukuran, &waktu, NULL, 0, (mz_uint)tingkat, NULL, 0, NULL, 0);
        fclose(masuk);
    }
    if (!ok) return gagalP(w, mz_zip_get_error_string(mz_zip_get_last_error(z)), e->nama);
    return true;
}

static bool tulisArsip(PenulisArsip* w, const char* format, ValueArray* entri, int tingkat) {
    EntriBaru* e = malloc(sizeof(EntriBaru));
    if (e == NULL) return gagalP(w, "memori tidak cukup", "");
    bool ok = true;
    if (strcmp(format, "zip") == 0) {
        mz_zip_archive z;
        memset(&z, 0, sizeof(z));
        if (!mz_zip_writer_init_cfile(&z, w->f, 0)) {
            free(e);
            return gagalP(w, "gagal menyiapkan ZIP", "");
        }
        for (int i = 0; ok && i < entri->count; i++) {
            ok = ambilEntri(entri->values[i], e, w) && zipEntri(w, &z, e, tingkat);
        }
        if (ok && !mz_zip_writer_finalize_archive(&z)) ok = gagalP(w, "gagal menutup ZIP", "");
        mz_zip_writer_end(&z);
    } else {
        bool gzip = strcmp(format, "tar.gz") == 0;
        if (gzip) {
            w->gz = penulisKompresBaru(KOMPRES_GZIP, tingkat, w->f);
            if (w->gz == NULL) ok = gagalP(w, "memori tidak cukup", "");
        }
        for (int i = 0; ok && i < entri->count; i++) {
            ok = ambilEntri(entri->values[i], e, w) && tarEntri(w, e);
        }
        static const uint8_t akhir[BLOK * 2];
        if (ok && !tarTulis(w, akhir, sizeof(akhir))) ok = gagalP(w, "gagal menulis", "");
        if (ok && gzip && !penulisKompresSelesai(w->gz)) ok = gagalP(w, "gagal menulis", "");
        penulisKompresTutup(w->gz);
        w->gz = NULL;
    }
    free(e);
    return ok;
}

Value nativeFnArsipBuat(int argCount, Value* args) {
    if (argCount != 4 || !IS_STRING(args[0]) || !IS_STRING(args[1]) || sequenceItems(args[2]) == NULL) {
        nativeRaiseKelas(vm.galatTipe, "_arsip_buat() memerlukan (jalur, format, entri, tingkat)");
        return KOSONG_VAL;
    }
    const char* jalur = AS_STRING(args[0])->chars;
    const char* format = AS_STRING(args[1])->chars;
    if (strcmp(format, "tar") != 0 && strcmp(format, "tar.gz") != 0 && strcmp(format, "zip") != 0) {
        nativeRaiseKelas(vm.galatNilai, "arsip.buat(): format '%s' tidak dikenal (tar, tar.gz, zip)", format);
        return KOSONG_VAL;
    }
    if (!IS_INT(args[3]) || AS_INT(args[3]) < 0 || AS_INT(args[3]) > 9) {
        nativeRaiseKelas(vm.galatNilai, "arsip.buat(): tingkat harus bulat 0..9");
        return KOSONG_VAL;
    }
    PenulisArsip w = { 0 };
    w.f = sistemBukaBerkas(jalur, "wb");
    if (w.f == NULL) {
        nativeRaiseKelas(vm.galatOS, "arsip.buat(): gagal membuat '%s'", jalur);
        return KOSONG_VAL;
    }
    ValueArray* entri = sequenceItems(args[2]);
    int jumlah = entri->count;
    bool ok = tulisArsip(&w, format, entri, (int)AS_INT(args[3]));
    if (fclose(w.f) != 0 && ok) ok = gagalP(&w, "gagal menulis", jalur);
    if (!ok) {
        sistemHapusBerkas(jalur);   // no half-written archive left behind
        // An empty message means kompresAmbilBita already raised.
        if (w.galat[0] != '\0') nativeRaiseKelas(vm.galatNilai, "%s", w.galat);
        return KOSONG_VAL;
    }
    return INT_VAL(jumlah);
}
