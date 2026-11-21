// zlib/gzip/raw deflate over the vendored miniz -- see kompres.h.
//
// miniz's zlib-style API handles zlib and raw deflate but not the gzip
// wrapper (RFC 1952), so the gzip header/trailer is parsed and written
// here around a raw deflate stream. Everything else (Huffman coding, CRC,
// Adler) is miniz.

#include <stdlib.h>
#include <string.h>

#include "kompres.h"
#include "memory.h"
#include "object.h"
#include "sistem.h"
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES   // no crc32/inflate/... macros in our code
#include "vendor/miniz/miniz.h"
#include "vm.h"

#define POTONGAN (64u * 1024u)   // I/O chunk for streaming
// A daftar holds one 16-byte Value per byte and counts with int, so results
// returned as daftar bita are capped here whatever `batas` the caller gave;
// larger data goes file -> file (gzip_berkas, arsip.*).
#define DAFTAR_MAKS (256LL * 1024 * 1024)
#define GZIP_ID1 0x1f
#define GZIP_ID2 0x8b
#define GZIP_FHCRC 0x02
#define GZIP_FEXTRA 0x04
#define GZIP_FNAME 0x08
#define GZIP_FCOMMENT 0x10
#define GZIP_FRESERVED 0xe0

// ---------------------------------------------------------------------------
// Byte values <-> daftar bita
// ---------------------------------------------------------------------------

bool kompresAmbilBita(Value v, const char* fungsi, const uint8_t** data,
                      size_t* panjang, bool* milik) {
    if (IS_STRING(v)) {
        *data = (const uint8_t*)AS_STRING(v)->chars;
        *panjang = (size_t)AS_STRING(v)->length;
        *milik = false;
        return true;
    }
    ValueArray* isi = sequenceItems(v);
    if (isi == NULL) {
        nativeRaiseKelas(vm.galatTipe, "%s%sdata harus teks atau daftar bita (bulat 0..255)",
                         fungsi, *fungsi ? ": " : "");
        return false;
    }
    uint8_t* buf = malloc(isi->count > 0 ? (size_t)isi->count : 1);
    if (buf == NULL) {
        nativeRaiseKelas(vm.galatNilai, "%s%smemori tidak cukup", fungsi, *fungsi ? ": " : "");
        return false;
    }
    for (int i = 0; i < isi->count; i++) {
        Value b = isi->values[i];
        if (!IS_INT(b) || AS_INT(b) < 0 || AS_INT(b) > 255) {
            free(buf);
            nativeRaiseKelas(vm.galatNilai, "%s%selemen ke-%d bukan bita (bulat 0..255)", fungsi,
                             *fungsi ? ": " : "", i);
            return false;
        }
        buf[i] = (uint8_t)AS_INT(b);
    }
    *data = buf;
    *panjang = (size_t)isi->count;
    *milik = true;
    return true;
}

Value kompresDaftarBita(const uint8_t* data, size_t n) {
    ObjList* hasil = newList();
    push(OBJ_VAL(hasil));   // rooted while it grows
    for (size_t i = 0; i < n; i++) {
        writeValueArray(&hasil->items, INT_VAL(data[i]));
    }
    pop();
    return OBJ_VAL(hasil);
}

// Growable malloc buffer (not the GC heap: sizes here can be large).
typedef struct {
    uint8_t* data;
    size_t panjang;
    size_t kapasitas;
} Penampung;

static bool penampungTambah(Penampung* p, const uint8_t* data, size_t n) {
    if (n > SIZE_MAX - p->panjang) return false;
    if (p->panjang + n > p->kapasitas) {
        size_t baru = p->kapasitas < 4096 ? 4096 : p->kapasitas;
        while (baru < p->panjang + n) {
            if (baru > SIZE_MAX / 2) { baru = p->panjang + n; break; }
            baru *= 2;
        }
        uint8_t* d = realloc(p->data, baru);
        if (d == NULL) return false;
        p->data = d;
        p->kapasitas = baru;
    }
    memcpy(p->data + p->panjang, data, n);
    p->panjang += n;
    return true;
}

// ---------------------------------------------------------------------------
// Streaming inflate
// ---------------------------------------------------------------------------

typedef enum { TAHAP_KEPALA, TAHAP_DATA, TAHAP_EKOR, TAHAP_SELESAI, TAHAP_GALAT } Tahap;

struct AliranInflasi {
    FormatKompres format;
    KompresSumber sumber;
    void* ctx;
    mz_stream z;
    bool zAktif;
    uint8_t masuk[POTONGAN];
    size_t ada;        // unread bytes in masuk[]
    size_t posisi;     // offset of the first unread byte
    bool habis;        // the source reported its end
    uint64_t totalMasuk;
    uint32_t crc;
    uint32_t keluarAnggota;   // ISIZE: output of this gzip member, mod 2^32
    Tahap tahap;
    const char* galat;
};

static bool isiMasuk(AliranInflasi* a) {
    if (a->ada > 0) return true;
    if (a->habis) return false;
    bool galat = false;
    size_t n = a->sumber(a->ctx, a->masuk, sizeof(a->masuk), &galat);
    if (galat) {
        a->galat = "gagal membaca data masukan";
        a->tahap = TAHAP_GALAT;
        return false;
    }
    if (n == 0) {
        a->habis = true;
        return false;
    }
    a->ada = n;
    a->posisi = 0;
    return true;
}

// One byte outside the deflate stream (gzip header/trailer).
static bool ambilByte(AliranInflasi* a, uint8_t* b) {
    if (!isiMasuk(a)) return false;
    *b = a->masuk[a->posisi++];
    a->ada--;
    a->totalMasuk++;
    return true;
}

static bool gagal(AliranInflasi* a, const char* pesan) {
    if (a->tahap != TAHAP_GALAT) a->galat = pesan;
    a->tahap = TAHAP_GALAT;
    return false;
}

static bool lewatiTeksNul(AliranInflasi* a) {
    uint8_t b;
    do {
        if (!ambilByte(a, &b)) return gagal(a, "kepala gzip terpotong");
    } while (b != 0);
    return true;
}

// Parse a gzip member header after its two magic bytes, then start a raw
// inflater for the member body.
static bool mulaiAnggotaGzip(AliranInflasi* a) {
    uint8_t kepala[8];   // CM FLG MTIME(4) XFL OS
    for (int i = 0; i < 8; i++) {
        if (!ambilByte(a, &kepala[i])) return gagal(a, "kepala gzip terpotong");
    }
    if (kepala[0] != 8) return gagal(a, "metode kompresi gzip tidak didukung (hanya deflate)");
    uint8_t flg = kepala[1];
    if (flg & GZIP_FRESERVED) return gagal(a, "kepala gzip memakai bendera cadangan");
    if (flg & GZIP_FEXTRA) {
        uint8_t lo, hi;
        if (!ambilByte(a, &lo) || !ambilByte(a, &hi)) return gagal(a, "kepala gzip terpotong");
        unsigned n = (unsigned)lo | ((unsigned)hi << 8);
        for (unsigned i = 0; i < n; i++) {
            uint8_t b;
            if (!ambilByte(a, &b)) return gagal(a, "kepala gzip terpotong");
        }
    }
    if ((flg & GZIP_FNAME) && !lewatiTeksNul(a)) return false;
    if ((flg & GZIP_FCOMMENT) && !lewatiTeksNul(a)) return false;
    if (flg & GZIP_FHCRC) {
        uint8_t b;
        if (!ambilByte(a, &b) || !ambilByte(a, &b)) return gagal(a, "kepala gzip terpotong");
    }
    if (a->zAktif) mz_inflateEnd(&a->z);
    memset(&a->z, 0, sizeof(a->z));
    if (mz_inflateInit2(&a->z, -MZ_DEFAULT_WINDOW_BITS) != MZ_OK) {
        a->zAktif = false;
        return gagal(a, "gagal menyiapkan dekompresi");
    }
    a->zAktif = true;
    a->crc = (uint32_t)MZ_CRC32_INIT;
    a->keluarAnggota = 0;
    a->tahap = TAHAP_DATA;
    return true;
}

AliranInflasi* aliranInflasiBaru(FormatKompres format, KompresSumber sumber, void* ctx) {
    AliranInflasi* a = calloc(1, sizeof(AliranInflasi));
    if (a == NULL) return NULL;
    a->format = format;
    a->sumber = sumber;
    a->ctx = ctx;
    if (format == KOMPRES_GZIP) {
        a->tahap = TAHAP_KEPALA;
        return a;
    }
    int bits = format == KOMPRES_ZLIB ? MZ_DEFAULT_WINDOW_BITS : -MZ_DEFAULT_WINDOW_BITS;
    if (mz_inflateInit2(&a->z, bits) != MZ_OK) {
        free(a);
        return NULL;
    }
    a->zAktif = true;
    a->tahap = TAHAP_DATA;
    return a;
}

static uint32_t le32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// After a gzip trailer: end of input, or another member (RFC 1952 allows
// several, and `cat a.gz b.gz` produces them). Anything else is rejected.
static bool sesudahEkorGzip(AliranInflasi* a) {
    uint8_t ekor[8];
    for (int i = 0; i < 8; i++) {
        if (!ambilByte(a, &ekor[i])) return gagal(a, "data gzip terpotong (ekor CRC hilang)");
    }
    if (le32(ekor) != a->crc) return gagal(a, "data gzip rusak (CRC-32 tidak cocok)");
    if (le32(ekor + 4) != a->keluarAnggota) return gagal(a, "data gzip rusak (ukuran tidak cocok)");
    uint8_t b1, b2;
    if (!ambilByte(a, &b1)) {
        if (a->tahap == TAHAP_GALAT) return false;
        a->tahap = TAHAP_SELESAI;
        return true;
    }
    if (b1 != GZIP_ID1 || !ambilByte(a, &b2) || b2 != GZIP_ID2) {
        return gagal(a, "ada data sisa setelah akhir gzip");
    }
    return mulaiAnggotaGzip(a);
}

static bool sesudahAkhirDeflate(AliranInflasi* a) {
    if (a->format == KOMPRES_GZIP) return sesudahEkorGzip(a);
    if (isiMasuk(a)) return gagal(a, "ada data sisa setelah akhir aliran terkompresi");
    if (a->tahap == TAHAP_GALAT) return false;
    a->tahap = TAHAP_SELESAI;
    return true;
}

long aliranInflasiBaca(AliranInflasi* a, uint8_t* out, size_t n) {
    if (n > POTONGAN) n = POTONGAN;
    for (;;) {
        switch (a->tahap) {
            case TAHAP_SELESAI: return 0;
            case TAHAP_GALAT: return -1;
            case TAHAP_KEPALA: {
                uint8_t b1, b2;
                if (!ambilByte(a, &b1) || !ambilByte(a, &b2)) {
                    gagal(a, "data gzip kosong atau terpotong");
                    return -1;
                }
                if (b1 != GZIP_ID1 || b2 != GZIP_ID2) {
                    gagal(a, "bukan data gzip (penanda 1f 8b tidak ada)");
                    return -1;
                }
                if (!mulaiAnggotaGzip(a)) return -1;
                continue;
            }
            case TAHAP_EKOR:
                if (!sesudahAkhirDeflate(a)) return -1;
                continue;
            case TAHAP_DATA: break;
        }
        if (a->ada == 0 && !isiMasuk(a) && a->tahap == TAHAP_GALAT) return -1;
        a->z.next_in = a->masuk + a->posisi;
        a->z.avail_in = (unsigned)a->ada;
        a->z.next_out = out;
        a->z.avail_out = (unsigned)n;
        int status = mz_inflate(&a->z, MZ_NO_FLUSH);
        size_t dipakai = a->ada - a->z.avail_in;
        size_t keluar = n - a->z.avail_out;
        a->posisi += dipakai;
        a->ada -= dipakai;
        a->totalMasuk += dipakai;
        if (a->format == KOMPRES_GZIP && keluar > 0) {
            a->crc = (uint32_t)mz_crc32(a->crc, out, keluar);
            a->keluarAnggota += (uint32_t)keluar;
        }
        if (status == MZ_STREAM_END) {
            a->tahap = TAHAP_EKOR;
            if (keluar > 0) return (long)keluar;
            continue;
        }
        if (status != MZ_OK && status != MZ_BUF_ERROR) {
            gagal(a, "data terkompresi rusak");
            return -1;
        }
        if (keluar > 0) return (long)keluar;
        if (a->ada == 0 && !isiMasuk(a)) {
            if (a->tahap != TAHAP_GALAT) gagal(a, "data terkompresi terpotong");
            return -1;
        }
    }
}

const char* aliranInflasiGalat(const AliranInflasi* a) {
    return a->galat != NULL ? a->galat : "galat tidak dikenal";
}

uint64_t aliranInflasiMasuk(const AliranInflasi* a) { return a->totalMasuk; }

void aliranInflasiTutup(AliranInflasi* a) {
    if (a == NULL) return;
    if (a->zAktif) mz_inflateEnd(&a->z);
    free(a);
}

// ---------------------------------------------------------------------------
// Streaming deflate into a FILE*
// ---------------------------------------------------------------------------

struct PenulisKompres {
    FormatKompres format;
    FILE* keluar;
    mz_stream z;
    bool zAktif;
    uint32_t crc;
    uint32_t panjang;   // ISIZE, mod 2^32
    bool rusak;
    uint64_t ditulis;   // bytes written to `keluar`, header and trailer included
    uint8_t sangga[POTONGAN];
};

static bool tulisSemua(FILE* f, const void* data, size_t n) {
    return n == 0 || fwrite(data, 1, n, f) == n;
}

PenulisKompres* penulisKompresBaru(FormatKompres format, int tingkat, FILE* keluar) {
    PenulisKompres* p = calloc(1, sizeof(PenulisKompres));
    if (p == NULL) return NULL;
    p->format = format;
    p->keluar = keluar;
    p->crc = (uint32_t)MZ_CRC32_INIT;
    int bits = format == KOMPRES_ZLIB ? MZ_DEFAULT_WINDOW_BITS : -MZ_DEFAULT_WINDOW_BITS;
    if (mz_deflateInit2(&p->z, tingkat, MZ_DEFLATED, bits, 9, MZ_DEFAULT_STRATEGY) != MZ_OK) {
        free(p);
        return NULL;
    }
    p->zAktif = true;
    if (format == KOMPRES_GZIP) {
        // MTIME 0 and OS 255 ("unknown") keep the output reproducible.
        const uint8_t kepala[10] = { GZIP_ID1, GZIP_ID2, 8, 0, 0, 0, 0, 0, 0, 255 };
        if (!tulisSemua(keluar, kepala, sizeof(kepala))) p->rusak = true;
        p->ditulis = sizeof(kepala);
    }
    return p;
}

// zlib's usual loop: keep draining while deflate fills the whole output
// buffer; with MZ_FINISH keep going until it reports the stream end.
static bool dorongDeflate(PenulisKompres* p, const uint8_t* data, size_t n, int flush) {
    p->z.next_in = data;
    p->z.avail_in = (unsigned)n;
    for (;;) {
        p->z.next_out = p->sangga;
        p->z.avail_out = sizeof(p->sangga);
        int status = mz_deflate(&p->z, flush);
        if (status != MZ_OK && status != MZ_STREAM_END && status != MZ_BUF_ERROR) return false;
        size_t keluar = sizeof(p->sangga) - p->z.avail_out;
        if (!tulisSemua(p->keluar, p->sangga, keluar)) return false;
        p->ditulis += keluar;
        if (flush == MZ_FINISH) {
            if (status == MZ_STREAM_END) return true;
            if (keluar == 0 && status == MZ_BUF_ERROR) return false;   // no progress
        } else if (p->z.avail_out != 0) {
            return true;
        }
    }
}

bool penulisKompresTulis(PenulisKompres* p, const uint8_t* data, size_t n) {
    if (p->rusak) return false;
    while (n > 0) {
        size_t bagian = n > POTONGAN ? POTONGAN : n;
        if (p->format == KOMPRES_GZIP) {
            p->crc = (uint32_t)mz_crc32(p->crc, data, bagian);
            p->panjang += (uint32_t)bagian;
        }
        if (!dorongDeflate(p, data, bagian, MZ_NO_FLUSH)) {
            p->rusak = true;
            return false;
        }
        data += bagian;
        n -= bagian;
    }
    return true;
}

bool penulisKompresSelesai(PenulisKompres* p) {
    if (p->rusak) return false;
    if (!dorongDeflate(p, NULL, 0, MZ_FINISH)) {
        p->rusak = true;
        return false;
    }
    if (p->format == KOMPRES_GZIP) {
        uint8_t ekor[8];
        for (int i = 0; i < 4; i++) {
            ekor[i] = (uint8_t)(p->crc >> (8 * i));
            ekor[4 + i] = (uint8_t)(p->panjang >> (8 * i));
        }
        if (!tulisSemua(p->keluar, ekor, sizeof(ekor))) p->rusak = true;
        p->ditulis += sizeof(ekor);
    }
    return !p->rusak;
}

uint64_t penulisKompresUkuran(const PenulisKompres* p) {
    return p->ditulis;
}

void penulisKompresTutup(PenulisKompres* p) {
    if (p == NULL) return;
    if (p->zAktif) mz_deflateEnd(&p->z);
    free(p);
}

// ---------------------------------------------------------------------------
// Natives
// ---------------------------------------------------------------------------

static bool ambilFormat(Value v, const char* fungsi, FormatKompres* f) {
    if (!IS_STRING(v)) {
        (void)fungsi;
        nativeRaiseKelas(vm.galatTipe, "kompres: format harus teks");
        return false;
    }
    const char* s = AS_STRING(v)->chars;
    if (strcmp(s, "zlib") == 0) *f = KOMPRES_ZLIB;
    else if (strcmp(s, "gzip") == 0) *f = KOMPRES_GZIP;
    else if (strcmp(s, "mentah") == 0) *f = KOMPRES_MENTAH;
    else {
        nativeRaiseKelas(vm.galatNilai, "kompres: format '%s' tidak dikenal (zlib, gzip, mentah)", s);
        return false;
    }
    return true;
}

static bool ambilBulat(Value v, const char* fungsi, const char* nama,
                       int64_t min, int64_t max, int64_t* hasil) {
    (void)fungsi;
    if (!IS_INT(v) || AS_INT(v) < min || AS_INT(v) > max) {
        nativeRaiseKelas(vm.galatNilai, "kompres: %s harus bulat %lld..%lld", nama,
                         (long long)min, (long long)max);
        return false;
    }
    *hasil = AS_INT(v);
    return true;
}

static bool ambilJalur(Value v, const char* fungsi, const char** jalur) {
    if (!IS_STRING(v)) {
        (void)fungsi;
        nativeRaiseKelas(vm.galatTipe, "kompres: jalur harus teks");
        return false;
    }
    *jalur = AS_STRING(v)->chars;
    return true;
}

// Source over a memory buffer.
typedef struct {
    const uint8_t* data;
    size_t panjang;
    size_t posisi;
} SumberMemori;

static size_t bacaMemori(void* ctx, uint8_t* buf, size_t n, bool* galat) {
    (void)galat;
    SumberMemori* s = ctx;
    size_t sisa = s->panjang - s->posisi;
    if (n > sisa) n = sisa;
    memcpy(buf, s->data + s->posisi, n);
    s->posisi += n;
    return n;
}

static size_t bacaBerkas(void* ctx, uint8_t* buf, size_t n, bool* galat) {
    FILE* f = ctx;
    size_t r = fread(buf, 1, n, f);
    if (r < n && ferror(f)) *galat = true;
    return r;
}

// Memory -> memory compression, 64 KB of output at a time.
static bool kompresMemori(FormatKompres format, int tingkat, const uint8_t* data,
                          size_t n, Penampung* hasil) {
    mz_stream z;
    memset(&z, 0, sizeof(z));
    int bits = format == KOMPRES_ZLIB ? MZ_DEFAULT_WINDOW_BITS : -MZ_DEFAULT_WINDOW_BITS;
    if (mz_deflateInit2(&z, tingkat, MZ_DEFLATED, bits, 9, MZ_DEFAULT_STRATEGY) != MZ_OK) return false;
    bool ok = true;
    if (format == KOMPRES_GZIP) {
        const uint8_t kepala[10] = { GZIP_ID1, GZIP_ID2, 8, 0, 0, 0, 0, 0, 0, 255 };
        ok = penampungTambah(hasil, kepala, sizeof(kepala));
    }
    uint8_t sangga[POTONGAN];
    size_t posisi = 0;
    while (ok) {
        size_t bagian = n - posisi > POTONGAN ? POTONGAN : n - posisi;
        z.next_in = data + posisi;
        z.avail_in = (unsigned)bagian;
        int flush = posisi + bagian == n ? MZ_FINISH : MZ_NO_FLUSH;
        int status;
        do {
            z.next_out = sangga;
            z.avail_out = sizeof(sangga);
            status = mz_deflate(&z, flush);
            if (status != MZ_OK && status != MZ_STREAM_END && status != MZ_BUF_ERROR) { ok = false; break; }
            if (!penampungTambah(hasil, sangga, sizeof(sangga) - z.avail_out)) { ok = false; break; }
        } while (z.avail_out == 0 || (flush == MZ_FINISH && status != MZ_STREAM_END));
        posisi += bagian;
        if (flush == MZ_FINISH) break;
    }
    mz_deflateEnd(&z);
    if (ok && format == KOMPRES_GZIP) {
        uint32_t crc = (uint32_t)mz_crc32(MZ_CRC32_INIT, data, n);
        uint8_t ekor[8];
        for (int i = 0; i < 4; i++) {
            ekor[i] = (uint8_t)(crc >> (8 * i));
            ekor[4 + i] = (uint8_t)((uint32_t)n >> (8 * i));
        }
        ok = penampungTambah(hasil, ekor, sizeof(ekor));
    }
    return ok;
}

// _kompres(data, format, tingkat) -> daftar bita
Value nativeFnKompres(int argCount, Value* args) {
    if (argCount != 3) {
        nativeRaiseKelas(vm.galatTipe, "_kompres() memerlukan 3 argumen (data, format, tingkat)");
        return KOSONG_VAL;
    }
    FormatKompres format;
    int64_t tingkat;
    if (!ambilFormat(args[1], "kompres", &format)) return KOSONG_VAL;
    if (!ambilBulat(args[2], "kompres", "tingkat", 0, 9, &tingkat)) return KOSONG_VAL;
    const uint8_t* data;
    size_t n;
    bool milik;
    if (!kompresAmbilBita(args[0], "kompres", &data, &n, &milik)) return KOSONG_VAL;
    Penampung hasil = { 0 };
    bool ok = kompresMemori(format, (int)tingkat, data, n, &hasil);
    if (milik) free((void*)data);
    if (!ok) {
        free(hasil.data);
        nativeRaiseKelas(vm.galatNilai, "kompres: gagal mengompresi (memori tidak cukup?)");
        return KOSONG_VAL;
    }
    Value v = kompresDaftarBita(hasil.data, hasil.panjang);
    free(hasil.data);
    return v;
}

// _dekompres(data, format, batas) -> daftar bita. `batas` caps the OUTPUT,
// which is what stops a decompression bomb: a few KB of input can expand to
// gigabytes, so the check happens while inflating, not afterwards.
Value nativeFnDekompres(int argCount, Value* args) {
    if (argCount != 3) {
        nativeRaiseKelas(vm.galatTipe, "_dekompres() memerlukan 3 argumen (data, format, batas)");
        return KOSONG_VAL;
    }
    FormatKompres format;
    int64_t batas;
    if (!ambilFormat(args[1], "dekompres", &format)) return KOSONG_VAL;
    if (!ambilBulat(args[2], "dekompres", "batas", 0, INT64_MAX, &batas)) return KOSONG_VAL;
    if (batas > DAFTAR_MAKS) batas = DAFTAR_MAKS;
    SumberMemori sumber = { 0 };
    bool milik;
    if (!kompresAmbilBita(args[0], "kompres", &sumber.data, &sumber.panjang, &milik)) return KOSONG_VAL;
    AliranInflasi* a = aliranInflasiBaru(format, bacaMemori, &sumber);
    Penampung hasil = { 0 };
    const char* galat = a == NULL ? "memori tidak cukup" : NULL;
    bool lewatBatas = false;
    uint8_t sangga[POTONGAN];
    while (galat == NULL) {
        long r = aliranInflasiBaca(a, sangga, sizeof(sangga));
        if (r == 0) break;
        if (r < 0) { galat = aliranInflasiGalat(a); break; }
        if ((uint64_t)hasil.panjang + (uint64_t)r > (uint64_t)batas) { lewatBatas = true; break; }
        if (!penampungTambah(&hasil, sangga, (size_t)r)) galat = "memori tidak cukup";
    }
    aliranInflasiTutup(a);
    if (milik) free((void*)sumber.data);
    if (galat != NULL || lewatBatas) {
        free(hasil.data);
        if (lewatBatas) {
            nativeRaiseKelas(vm.galatNilai, "kompres: hasil melebihi batas %lld bita", (long long)batas);
        } else {
            nativeRaiseKelas(vm.galatNilai, "kompres: %s", galat);
        }
        return KOSONG_VAL;
    }
    Value v = kompresDaftarBita(hasil.data, hasil.panjang);
    free(hasil.data);
    return v;
}

// _kompres_berkas(sumber, tujuan, format, tingkat) -> bita yang ditulis.
// File -> file in 64 KB pieces: the input never sits in RAM whole.
Value nativeFnKompresBerkas(int argCount, Value* args) {
    if (argCount != 4) {
        nativeRaiseKelas(vm.galatTipe, "_kompres_berkas() memerlukan 4 argumen");
        return KOSONG_VAL;
    }
    const char *dari, *ke;
    FormatKompres format;
    int64_t tingkat;
    if (!ambilJalur(args[0], "kompres_berkas", &dari) || !ambilJalur(args[1], "kompres_berkas", &ke) ||
        !ambilFormat(args[2], "kompres_berkas", &format) ||
        !ambilBulat(args[3], "kompres_berkas", "tingkat", 0, 9, &tingkat)) {
        return KOSONG_VAL;
    }
    FILE* masuk = sistemBukaBerkas(dari, "rb");
    if (masuk == NULL) {
        nativeRaiseKelas(vm.galatOS, "kompres: gagal membuka '%s'", dari);
        return KOSONG_VAL;
    }
    FILE* keluar = sistemBukaBerkas(ke, "wb");
    if (keluar == NULL) {
        fclose(masuk);
        nativeRaiseKelas(vm.galatOS, "kompres: gagal membuat '%s'", ke);
        return KOSONG_VAL;
    }
    PenulisKompres* p = penulisKompresBaru(format, (int)tingkat, keluar);
    bool ok = p != NULL;
    uint8_t* sangga = malloc(POTONGAN);
    ok = ok && sangga != NULL;
    while (ok) {
        size_t r = fread(sangga, 1, POTONGAN, masuk);
        if (r > 0 && !penulisKompresTulis(p, sangga, r)) ok = false;
        if (r < POTONGAN) {
            if (ferror(masuk)) ok = false;
            break;
        }
    }
    ok = ok && penulisKompresSelesai(p);
    uint64_t ukuran = p != NULL ? penulisKompresUkuran(p) : 0;
    penulisKompresTutup(p);
    free(sangga);
    fclose(masuk);
    if (fclose(keluar) != 0) ok = false;
    if (!ok) {
        sistemHapusBerkas(ke);
        nativeRaiseKelas(vm.galatOS, "kompres: gagal menulis '%s'", ke);
        return KOSONG_VAL;
    }
    return INT_VAL((int64_t)ukuran);
}

// _dekompres_berkas(sumber, tujuan, format, batas) -> bita yang ditulis.
Value nativeFnDekompresBerkas(int argCount, Value* args) {
    if (argCount != 4) {
        nativeRaiseKelas(vm.galatTipe, "_dekompres_berkas() memerlukan 4 argumen");
        return KOSONG_VAL;
    }
    const char *dari, *ke;
    FormatKompres format;
    int64_t batas;
    if (!ambilJalur(args[0], "dekompres_berkas", &dari) || !ambilJalur(args[1], "dekompres_berkas", &ke) ||
        !ambilFormat(args[2], "dekompres_berkas", &format) ||
        !ambilBulat(args[3], "dekompres_berkas", "batas", 0, INT64_MAX, &batas)) {
        return KOSONG_VAL;
    }
    FILE* masuk = sistemBukaBerkas(dari, "rb");
    if (masuk == NULL) {
        nativeRaiseKelas(vm.galatOS, "kompres: gagal membuka '%s'", dari);
        return KOSONG_VAL;
    }
    FILE* keluar = sistemBukaBerkas(ke, "wb");
    if (keluar == NULL) {
        fclose(masuk);
        nativeRaiseKelas(vm.galatOS, "kompres: gagal membuat '%s'", ke);
        return KOSONG_VAL;
    }
    AliranInflasi* a = aliranInflasiBaru(format, bacaBerkas, masuk);
    uint8_t* sangga = malloc(POTONGAN);
    const char* galat = (a == NULL || sangga == NULL) ? "memori tidak cukup" : NULL;
    bool gagalTulis = false, lewatBatas = false;
    uint64_t total = 0;
    while (galat == NULL) {
        long r = aliranInflasiBaca(a, sangga, POTONGAN);
        if (r == 0) break;
        if (r < 0) { galat = aliranInflasiGalat(a); break; }
        if (total + (uint64_t)r > (uint64_t)batas) { lewatBatas = true; break; }
        if (fwrite(sangga, 1, (size_t)r, keluar) != (size_t)r) { gagalTulis = true; break; }
        total += (uint64_t)r;
    }
    aliranInflasiTutup(a);
    free(sangga);
    fclose(masuk);
    if (fclose(keluar) != 0) gagalTulis = true;
    if (galat != NULL || lewatBatas || gagalTulis) {
        sistemHapusBerkas(ke);   // never leave a half-written result behind
        if (lewatBatas) {
            nativeRaiseKelas(vm.galatNilai, "kompres: hasil melebihi batas %lld bita", (long long)batas);
        } else if (gagalTulis) {
            nativeRaiseKelas(vm.galatOS, "kompres: gagal menulis '%s'", ke);
        } else {
            nativeRaiseKelas(vm.galatNilai, "kompres: %s", galat);
        }
        return KOSONG_VAL;
    }
    return INT_VAL((int64_t)total);
}

static Value checksum(int argCount, Value* args, const char* fungsi, bool crc) {
    if (argCount != 2) {
        nativeRaiseKelas(vm.galatTipe, "_%s() memerlukan 2 argumen (data, awal)", fungsi);
        return KOSONG_VAL;
    }
    int64_t awal;
    if (!ambilBulat(args[1], fungsi, "awal", 0, UINT32_MAX, &awal)) return KOSONG_VAL;
    const uint8_t* data;
    size_t n;
    bool milik;
    if (!kompresAmbilBita(args[0], "kompres", &data, &n, &milik)) return KOSONG_VAL;
    mz_ulong h = crc ? mz_crc32((mz_ulong)awal, data, n) : mz_adler32((mz_ulong)awal, data, n);
    if (milik) free((void*)data);
    return INT_VAL((int64_t)(h & 0xffffffffu));
}

Value nativeFnCrc32(int argCount, Value* args) { return checksum(argCount, args, "crc32", true); }
Value nativeFnAdler32(int argCount, Value* args) { return checksum(argCount, args, "adler32", false); }

// _baca_berkas_bita(jalur, batas) -> daftar bita. baca_semua() returns teks,
// which cannot represent bytes that are not UTF-8.
Value nativeFnBacaBerkasBita(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaiseKelas(vm.galatTipe, "_baca_berkas_bita() memerlukan 2 argumen (jalur, batas)");
        return KOSONG_VAL;
    }
    const char* jalur;
    int64_t batas;
    if (!ambilJalur(args[0], "baca_berkas", &jalur) ||
        !ambilBulat(args[1], "baca_berkas", "batas", 0, DAFTAR_MAKS, &batas)) {
        return KOSONG_VAL;
    }
    FILE* f = sistemBukaBerkas(jalur, "rb");
    if (f == NULL) {
        nativeRaiseKelas(vm.galatOS, "bita: gagal membuka '%s'", jalur);
        return KOSONG_VAL;
    }
    Penampung isi = { 0 };
    uint8_t sangga[POTONGAN];
    bool ok = true, lewatBatas = false;
    for (;;) {
        size_t r = fread(sangga, 1, sizeof(sangga), f);
        if ((uint64_t)isi.panjang + r > (uint64_t)batas) { lewatBatas = true; break; }
        if (r > 0 && !penampungTambah(&isi, sangga, r)) { ok = false; break; }
        if (r < sizeof(sangga)) {
            if (ferror(f)) ok = false;
            break;
        }
    }
    fclose(f);
    if (!ok || lewatBatas) {
        free(isi.data);
        if (lewatBatas) {
            nativeRaiseKelas(vm.galatNilai, "bita: '%s' melebihi batas %lld bita", jalur, (long long)batas);
        } else {
            nativeRaiseKelas(vm.galatOS, "bita: gagal membaca '%s'", jalur);
        }
        return KOSONG_VAL;
    }
    Value v = kompresDaftarBita(isi.data, isi.panjang);
    free(isi.data);
    return v;
}

// _tulis_berkas_bita(jalur, data) -> jumlah bita ditulis.
Value nativeFnTulisBerkasBita(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaiseKelas(vm.galatTipe, "_tulis_berkas_bita() memerlukan 2 argumen (jalur, data)");
        return KOSONG_VAL;
    }
    const char* jalur;
    if (!ambilJalur(args[0], "tulis_berkas", &jalur)) return KOSONG_VAL;
    const uint8_t* data;
    size_t n;
    bool milik;
    if (!kompresAmbilBita(args[1], "bita", &data, &n, &milik)) return KOSONG_VAL;
    FILE* f = sistemBukaBerkas(jalur, "wb");
    bool ok = f != NULL && tulisSemua(f, data, n);
    if (f != NULL && fclose(f) != 0) ok = false;
    if (milik) free((void*)data);
    if (!ok) {
        nativeRaiseKelas(vm.galatOS, "bita: gagal menulis '%s'", jalur);
        return KOSONG_VAL;
    }
    return INT_VAL((int64_t)n);
}
