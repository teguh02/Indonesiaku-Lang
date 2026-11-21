// TAR / TAR.GZ / ZIP archives -- see arsip.h for the safety rules.

#if !defined(_WIN32)
#define _DEFAULT_SOURCE   // fseeko/ftello: hidden by -std=c11
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arsip.h"
#include "kompres.h"
#include "memory.h"
#include "object.h"
#include "sistem.h"
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES   // no crc32/inflate/... macros in our code
#include "vendor/miniz/miniz.h"
#include "vm.h"

#define BLOK 512
#define BATAS_NAMA ARSIP_BATAS_NAMA        // longest entry name accepted
#define BATAS_META (64u * 1024u)          // PAX / GNU long-name payload
#define AMBANG_RASIO (1024u * 1024u)      // ratio checks start above 1 MiB
#define POTONG (64u * 1024u)

typedef enum { MODE_DAFTAR, MODE_PERIKSA, MODE_EKSTRAK } Mode;
typedef enum { ARSIP_TAR, ARSIP_TARGZ, ARSIP_ZIP } JenisArsip;

typedef struct {
    Mode mode;
    const char* jalur;
    const char* tujuan;
    int64_t batasUkuran, batasEntri, batasRasio;
    bool lewatiTautan;
    uint64_t ukuranArsip;
    uint64_t total;     // declared uncompressed bytes so far
    int64_t jumlah;     // entries seen so far
    ObjList* hasil;     // rooted by the native
    char galat[640];
    ObjClass* kelasGalat;
} Konteks;

static bool gagalK(Konteks* k, ObjClass* kelas, const char* fmt, ...) {
    if (k->kelasGalat != NULL) return false;   // keep the first error
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(k->galat, sizeof(k->galat), fmt, ap);
    va_end(ap);
    k->kelasGalat = kelas;
    return false;
}

// ---------------------------------------------------------------------------
// Entry names
// ---------------------------------------------------------------------------

static bool utf8Sah(const unsigned char* s) {
    while (*s) {
        unsigned c = *s;
        int n = c < 0x80 ? 0 : (c >= 0xc2 && c < 0xe0) ? 1 : (c >= 0xe0 && c < 0xf0) ? 2
              : (c >= 0xf0 && c < 0xf5) ? 3 : -1;
        if (n < 0) return false;
        unsigned kode = n == 0 ? c : c & (0x3f >> n);
        for (int i = 1; i <= n; i++) {
            if ((s[i] & 0xc0) != 0x80) return false;
            kode = (kode << 6) | (s[i] & 0x3f);
        }
        if ((n == 2 && (kode < 0x800 || (kode >= 0xd800 && kode <= 0xdfff))) ||
            (n == 3 && (kode < 0x10000 || kode > 0x10ffff))) {
            return false;
        }
        s += n + 1;
    }
    return true;
}

#ifdef _WIN32
// CON, PRN, AUX, NUL, COM1-9, LPT1-9 (with any extension) are devices on
// Windows, and a trailing '.' or ' ' is silently dropped -- both would make
// an entry land somewhere other than its name says.
static bool komponenWindowsBuruk(const char* s, size_t n) {
    if (n > 0 && (s[n - 1] == '.' || s[n - 1] == ' ')) return true;
    size_t dasar = 0;
    while (dasar < n && s[dasar] != '.') dasar++;
    static const char* perangkat[] = { "con", "prn", "aux", "nul" };
    char kecil[5] = { 0 };
    if (dasar < 3 || dasar > 4) return false;
    for (size_t i = 0; i < dasar; i++) kecil[i] = (char)((s[i] >= 'A' && s[i] <= 'Z') ? s[i] + 32 : s[i]);
    if (dasar == 3) {
        for (int i = 0; i < 4; i++) if (strcmp(kecil, perangkat[i]) == 0) return true;
        return false;
    }
    return (strncmp(kecil, "com", 3) == 0 || strncmp(kecil, "lpt", 3) == 0) &&
           kecil[3] >= '1' && kecil[3] <= '9';
}
#endif

// Normalise an archive name into `keluar` ("a/./b//c/" -> "a/b/c").
// Returns 1 for a usable name, 0 for one that names the root itself ("./"),
// and -1 (with `*alasan`) for a name that must be refused.
int arsipBersihkanNama(const char* masuk, char* keluar, size_t ukuran, const char** alasan) {
    size_t panjang = strlen(masuk);
    if (panjang == 0) { *alasan = "nama kosong"; return -1; }
    if (panjang > BATAS_NAMA) { *alasan = "nama terlalu panjang"; return -1; }
    if (!utf8Sah((const unsigned char*)masuk)) { *alasan = "nama bukan UTF-8 yang sah"; return -1; }
    if (masuk[0] == '/') { *alasan = "jalur absolut"; return -1; }
    for (const char* p = masuk; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c == '\\') { *alasan = "memuat garis miring terbalik '\\'"; return -1; }
        if (c == ':') { *alasan = "memuat ':' (huruf drive / aliran data)"; return -1; }
        if (c < 0x20 || c == 0x7f) { *alasan = "memuat karakter kendali"; return -1; }
    }
    size_t o = 0;
    const char* p = masuk;
    while (*p) {
        const char* akhir = strchr(p, '/');
        size_t n = akhir ? (size_t)(akhir - p) : strlen(p);
        if (n == 2 && p[0] == '.' && p[1] == '.') { *alasan = "memuat komponen '..'"; return -1; }
        if (n > 0 && !(n == 1 && p[0] == '.')) {
#ifdef _WIN32
            if (komponenWindowsBuruk(p, n)) { *alasan = "nama perangkat atau akhiran '.'/' ' Windows"; return -1; }
#endif
            if (o + n + 2 > ukuran) { *alasan = "nama terlalu panjang"; return -1; }
            if (o > 0) keluar[o++] = '/';
            memcpy(keluar + o, p, n);
            o += n;
        }
        p += n;
        if (*p == '/') p++;
    }
    keluar[o] = '\0';
    return o > 0 ? 1 : 0;
}

// ---------------------------------------------------------------------------
// Destination paths (never through an existing link)
// ---------------------------------------------------------------------------

static char* gabungJalur(const char* tujuan, const char* rel) {
    size_t a = strlen(tujuan), b = strlen(rel);
    while (a > 1 && (tujuan[a - 1] == '/' || tujuan[a - 1] == '\\')) a--;
    char* s = malloc(a + b + 2);
    if (s == NULL) return NULL;
    memcpy(s, tujuan, a);
    s[a] = '/';
    memcpy(s + a + 1, rel, b + 1);
    return s;
}

// The destination itself is the caller's choice and may be (or sit below)
// a link -- /tmp is one on macOS. Create it, with its parents, if missing.
static bool siapkanTujuan(Konteks* k) {
    int jenis = sistemJenisTanpaTautan(k->tujuan);
    if (jenis == SISTEM_JALUR_DIREKTORI || jenis == SISTEM_JALUR_TAUTAN) return true;
    if (jenis != SISTEM_JALUR_TIDAK_ADA) {
        return gagalK(k, vm.galatOS, "arsip.ekstrak(): tujuan '%s' bukan direktori", k->tujuan);
    }
    size_t n = strlen(k->tujuan);
    char* salinan = malloc(n + 1);
    if (salinan == NULL) return gagalK(k, vm.galatOS, "arsip.ekstrak(): memori tidak cukup");
    memcpy(salinan, k->tujuan, n + 1);
    for (size_t i = 1; i <= n; i++) {
        if (i < n && salinan[i] != '/' && salinan[i] != '\\') continue;
        char simpan = salinan[i];
        salinan[i] = '\0';
        if (sistemJenisTanpaTautan(salinan) == SISTEM_JALUR_TIDAK_ADA) sistemBuatSatuDirektori(salinan);
        salinan[i] = simpan;
    }
    free(salinan);
    jenis = sistemJenisTanpaTautan(k->tujuan);
    if (jenis != SISTEM_JALUR_DIREKTORI && jenis != SISTEM_JALUR_TAUTAN) {
        return gagalK(k, vm.galatOS, "arsip.ekstrak(): gagal membuat direktori tujuan '%s'", k->tujuan);
    }
    return true;
}

// Walk every directory of `rel` below the destination (all components, or
// all but the last when `termasukAkhir` is false), refusing any link. With
// `buat` missing directories are created; without it (pass 1) the walk only
// checks what already exists and stops at the first missing component.
static bool siapkanDirektori(Konteks* k, const char* rel, bool termasukAkhir, bool buat) {
    char* penuh = gabungJalur(k->tujuan, rel);
    if (penuh == NULL) return gagalK(k, vm.galatOS, "arsip.ekstrak(): memori tidak cukup");
    size_t awal = strlen(penuh) - strlen(rel);
    bool ok = true;
    for (size_t i = awal; ok; i++) {
        bool akhir = penuh[i] == '\0';
        if (!akhir && penuh[i] != '/') continue;
        if (akhir && !termasukAkhir) break;
        penuh[i] = '\0';
        int jenis = sistemJenisTanpaTautan(penuh);
        if (jenis == SISTEM_JALUR_TIDAK_ADA) {
            if (!buat) break;
            if (!sistemBuatSatuDirektori(penuh) &&
                sistemJenisTanpaTautan(penuh) != SISTEM_JALUR_DIREKTORI) {
                ok = gagalK(k, vm.galatOS, "arsip.ekstrak(): gagal membuat direktori '%s'", penuh + awal);
            }
        } else if (jenis == SISTEM_JALUR_TAUTAN) {
            ok = gagalK(k, vm.galatNilai, "arsip.ekstrak(): '%s' di tujuan adalah tautan; tidak diikuti", penuh + awal);
        } else if (jenis != SISTEM_JALUR_DIREKTORI) {
            ok = gagalK(k, vm.galatOS, "arsip.ekstrak(): '%s' di tujuan sudah ada dan bukan direktori", penuh + awal);
        }
        if (akhir) break;
        penuh[i] = '/';
    }
    free(penuh);
    return ok;
}

// The file itself: an existing link is refused (never written through), an
// existing directory or device is an error, an existing file is replaced.
static bool periksaBerkasAkhir(Konteks* k, const char* rel, const char* penuh) {
    int jenis = sistemJenisTanpaTautan(penuh);
    if (jenis == SISTEM_JALUR_TAUTAN) {
        return gagalK(k, vm.galatNilai, "arsip.ekstrak(): '%s' di tujuan adalah tautan; tidak ditimpa", rel);
    }
    if (jenis == SISTEM_JALUR_DIREKTORI || jenis == SISTEM_JALUR_LAIN) {
        return gagalK(k, vm.galatOS, "arsip.ekstrak(): '%s' di tujuan sudah ada dan bukan berkas", rel);
    }
    return true;
}

// Pass 1: the destination checks of pass 2, creating nothing, so that an
// existing link below the destination rejects the archive up front.
static bool periksaTujuan(Konteks* k, const char* rel, bool direktori) {
    if (direktori) return siapkanDirektori(k, rel, true, false);
    if (!siapkanDirektori(k, rel, false, false)) return false;
    char* penuh = gabungJalur(k->tujuan, rel);
    if (penuh == NULL) return gagalK(k, vm.galatOS, "arsip.ekstrak(): memori tidak cukup");
    bool ok = periksaBerkasAkhir(k, rel, penuh);
    free(penuh);
    return ok;
}

static FILE* bukaTujuanBerkas(Konteks* k, const char* rel) {
    if (!siapkanDirektori(k, rel, false, true)) return NULL;
    char* penuh = gabungJalur(k->tujuan, rel);
    if (penuh == NULL) { gagalK(k, vm.galatOS, "arsip.ekstrak(): memori tidak cukup"); return NULL; }
    FILE* f = NULL;
    if (periksaBerkasAkhir(k, rel, penuh)) {
        f = sistemBukaTulisTanpaTautan(penuh);
        if (f == NULL) gagalK(k, vm.galatOS, "arsip.ekstrak(): gagal membuat '%s'", rel);
    }
    free(penuh);
    return f;
}

// ---------------------------------------------------------------------------
// Per-entry policy shared by TAR and ZIP
// ---------------------------------------------------------------------------

static const char* namaJenis(char jenis) {
    switch (jenis) {
        case 'f': return "berkas";
        case 'd': return "direktori";
        case 'l': return "tautan_simbolik";
        case 'h': return "tautan_keras";
        default:  return "lain";
    }
}

static void catatDaftar(Konteks* k, const char* nama, char jenis, uint64_t ukuran, uint64_t terkompres) {
    ObjTuple* t = newTuple();
    push(OBJ_VAL(t));
    Value v = OBJ_VAL(copyString(nama, (int)strlen(nama)));
    push(v);
    writeValueArray(&t->items, v);
    pop();
    const char* j = namaJenis(jenis);
    v = OBJ_VAL(copyString(j, (int)strlen(j)));
    push(v);
    writeValueArray(&t->items, v);
    pop();
    writeValueArray(&t->items, INT_VAL(ukuran > INT64_MAX ? INT64_MAX : (int64_t)ukuran));
    writeValueArray(&t->items, INT_VAL(terkompres > INT64_MAX ? INT64_MAX : (int64_t)terkompres));
    writeValueArray(&k->hasil->items, OBJ_VAL(t));
    pop();
}

static void catatEkstrak(Konteks* k, const char* nama) {
    Value v = OBJ_VAL(copyString(nama, (int)strlen(nama)));
    push(v);
    writeValueArray(&k->hasil->items, v);
    pop();
}

static bool rasioBerlebih(const Konteks* k, uint64_t besar, uint64_t kecil) {
    if (besar <= AMBANG_RASIO) return false;
    if (kecil == 0) return true;
    return besar / kecil >= (uint64_t)k->batasRasio;
}

// 1 = extract into `bersih`, 0 = skip, -1 = error (recorded in k).
static int nilaiEntri(Konteks* k, const char* nama, char jenis, uint64_t ukuran,
                      uint64_t terkompres, bool adaTerkompres, char* bersih) {
    if (++k->jumlah > k->batasEntri) {
        gagalK(k, vm.galatNilai, "arsip: jumlah entri melebihi batas %lld", (long long)k->batasEntri);
        return -1;
    }
    if (k->mode == MODE_DAFTAR) {
        catatDaftar(k, nama, jenis, ukuran, adaTerkompres ? terkompres : ukuran);
        return 0;
    }
    if (jenis == 'l' || jenis == 'h') {
        if (k->lewatiTautan) return 0;
        gagalK(k, vm.galatNilai, "arsip.ekstrak(): entri '%s' adalah %s dan ditolak "
               "(pakai lewati_tautan=benar untuk melewatinya)", nama,
               jenis == 'l' ? "tautan simbolik" : "tautan keras");
        return -1;
    }
    if (jenis != 'f' && jenis != 'd') {
        gagalK(k, vm.galatNilai, "arsip.ekstrak(): entri '%s' berjenis khusus yang tidak didukung", nama);
        return -1;
    }
    const char* alasan = NULL;
    int r = arsipBersihkanNama(nama, bersih, BATAS_NAMA + 2, &alasan);
    if (r < 0) {
        gagalK(k, vm.galatNilai, "arsip.ekstrak(): entri '%.200s' ditolak: %s", nama, alasan);
        return -1;
    }
    if (r == 0) return 0;
    if (ukuran > (uint64_t)k->batasUkuran || k->total + ukuran > (uint64_t)k->batasUkuran) {
        gagalK(k, vm.galatNilai, "arsip.ekstrak(): ukuran total melebihi batas %lld bita", (long long)k->batasUkuran);
        return -1;
    }
    k->total += ukuran;
    if ((adaTerkompres && rasioBerlebih(k, ukuran, terkompres)) ||
        (k->ukuranArsip > 0 && rasioBerlebih(k, k->total, k->ukuranArsip))) {
        gagalK(k, vm.galatNilai, "arsip.ekstrak(): rasio kompresi '%s' melebihi batas %lld "
               "(kemungkinan bom dekompresi)", bersih, (long long)k->batasRasio);
        return -1;
    }
    return 1;
}

uint64_t arsipUkuranBerkas(FILE* f) {
#ifdef _WIN32
    if (_fseeki64(f, 0, SEEK_END) != 0) return 0;
    long long n = _ftelli64(f);
    _fseeki64(f, 0, SEEK_SET);
#else
    if (fseeko(f, 0, SEEK_END) != 0) return 0;
    long long n = (long long)ftello(f);
    fseeko(f, 0, SEEK_SET);
#endif
    return n < 0 ? 0 : (uint64_t)n;
}

// ---------------------------------------------------------------------------
// TAR reader (ustar, GNU long names, PAX path/size; plain or gzip)
// ---------------------------------------------------------------------------

typedef struct {
    FILE* f;
    AliranInflasi* gz;   // NULL for a plain .tar
    uint64_t dibaca;     // tar-stream bytes delivered (after gunzip)
    uint64_t sisaData;   // unread data of the current entry
    uint64_t sisaPad;
    int64_t jumlahKepala;
    Konteks* k;
} PembacaTar;

typedef struct {
    char nama[BATAS_NAMA + 2];
    char jenis;          // f d l h ?
    uint64_t ukuran;
} EntriTar;

static bool blokNol(const uint8_t* b) {
    for (int i = 0; i < BLOK; i++) if (b[i] != 0) return false;
    return true;
}

// Octal (space/NUL padded) or GNU base-256 number. False if malformed.
static bool angkaTar(const uint8_t* p, size_t n, uint64_t* hasil) {
    uint64_t v = 0;
    if (p[0] & 0x80) {
        if (p[0] == 0xff) return false;   // negative
        v = p[0] & 0x7f;
        for (size_t i = 1; i < n; i++) {
            if (v >> 55) return false;
            v = (v << 8) | p[i];
        }
        *hasil = v;
        return true;
    }
    size_t i = 0;
    while (i < n && p[i] == ' ') i++;
    for (; i < n && p[i] != ' ' && p[i] != '\0'; i++) {
        if (p[i] < '0' || p[i] > '7' || (v >> 60)) return false;
        v = v * 8 + (uint64_t)(p[i] - '0');
    }
    *hasil = v;
    return true;
}

static bool checksumSah(const uint8_t* b) {
    uint64_t tersimpan;
    if (!angkaTar(b + 148, 8, &tersimpan)) return false;
    uint64_t tanpaTanda = 0;
    int64_t bertanda = 0;
    for (int i = 0; i < BLOK; i++) {
        uint8_t c = (i >= 148 && i < 156) ? ' ' : b[i];
        tanpaTanda += c;
        bertanda += (int8_t)c;
    }
    return tersimpan == tanpaTanda || (int64_t)tersimpan == bertanda;
}

static size_t sumberTar(void* ctx, uint8_t* buf, size_t n, bool* galat) {
    FILE* f = ctx;
    size_t r = fread(buf, 1, n, f);
    if (r < n && ferror(f)) *galat = true;
    return r;
}

static bool tarTerpotong(PembacaTar* r) {
    return gagalK(r->k, vm.galatNilai, "arsip: arsip TAR terpotong");
}

// Exactly `n` bytes. 1 = ok, 0 = clean end before any byte, -1 = error.
static int tarBaca(PembacaTar* r, uint8_t* buf, size_t n) {
    size_t didapat = 0;
    while (didapat < n) {
        long m;
        if (r->gz != NULL) {
            m = aliranInflasiBaca(r->gz, buf + didapat, n - didapat);
            if (m < 0) {
                gagalK(r->k, vm.galatNilai, "arsip: data gzip rusak: %s", aliranInflasiGalat(r->gz));
                return -1;
            }
        } else {
            m = (long)fread(buf + didapat, 1, n - didapat, r->f);
            if (m == 0 && ferror(r->f)) {
                gagalK(r->k, vm.galatOS, "arsip: gagal membaca '%s'", r->k->jalur);
                return -1;
            }
        }
        if (m == 0) {
            if (didapat == 0) return 0;
            tarTerpotong(r);
            return -1;
        }
        didapat += (size_t)m;
    }
    r->dibaca += n;
    // The gzip layer is checked on its own: a stream of zero blocks expands
    // ~1000x and would otherwise never trip a header-based limit.
    if (r->gz != NULL && rasioBerlebih(r->k, r->dibaca, aliranInflasiMasuk(r->gz))) {
        gagalK(r->k, vm.galatNilai, "arsip: rasio kompresi gzip melebihi batas %lld "
               "(kemungkinan bom dekompresi)", (long long)r->k->batasRasio);
        return -1;
    }
    return 1;
}

static bool tarLewati(PembacaTar* r, uint64_t n) {
    uint8_t buf[BLOK * 8];
    while (n > 0) {
        size_t bagian = n > sizeof(buf) ? sizeof(buf) : (size_t)n;
        int rc = tarBaca(r, buf, bagian);
        if (rc == 0) return tarTerpotong(r);
        if (rc < 0) return false;
        n -= bagian;
    }
    return true;
}

static uint64_t padTar(uint64_t n) { return (BLOK - n % BLOK) % BLOK; }

// PAX records "<len> <key>=<value>\n": only `path` and `size` matter here.
static bool paxUrai(PembacaTar* r, const char* data, size_t n, char* nama, bool* adaNama,
                    uint64_t* ukuran, bool* adaUkuran) {
    size_t i = 0;
    while (i < n) {
        size_t panjang = 0, j = i;
        while (j < n && data[j] >= '0' && data[j] <= '9' && panjang < BATAS_META) {
            panjang = panjang * 10 + (size_t)(data[j] - '0');
            j++;
        }
        if (j >= n || data[j] != ' ' || panjang <= j - i || panjang > n - i || data[i + panjang - 1] != '\n') {
            return gagalK(r->k, vm.galatNilai, "arsip: kepala PAX rusak");
        }
        const char* kunci = data + j + 1;
        const char* akhir = data + i + panjang - 1;
        const char* sama = memchr(kunci, '=', (size_t)(akhir - kunci));
        if (sama == NULL) return gagalK(r->k, vm.galatNilai, "arsip: kepala PAX rusak");
        size_t pk = (size_t)(sama - kunci), pv = (size_t)(akhir - sama - 1);
        if (pk == 4 && memcmp(kunci, "path", 4) == 0) {
            if (pv > BATAS_NAMA) return gagalK(r->k, vm.galatNilai, "arsip: nama entri terlalu panjang");
            memcpy(nama, sama + 1, pv);
            nama[pv] = '\0';
            *adaNama = true;
        } else if (pk == 4 && memcmp(kunci, "size", 4) == 0) {
            uint64_t v = 0;
            if (pv == 0) return gagalK(r->k, vm.galatNilai, "arsip: ukuran PAX rusak");
            for (size_t m = 0; m < pv; m++) {
                char c = sama[1 + m];
                if (c < '0' || c > '9' || v > (UINT64_MAX - 9) / 10) {
                    return gagalK(r->k, vm.galatNilai, "arsip: ukuran PAX rusak");
                }
                v = v * 10 + (uint64_t)(c - '0');
            }
            *ukuran = v;
            *adaUkuran = true;
        }
        i += panjang;
    }
    return true;
}

// GNU 'L' long name / 'K' long link / PAX 'x' (per entry) / 'g' (global).
static bool tarMeta(PembacaTar* r, char tipe, uint64_t ukuran, char* namaMeta, bool* adaNama,
                    uint64_t* ukuranMeta, bool* adaUkuran) {
    if (ukuran > BATAS_META) return gagalK(r->k, vm.galatNilai, "arsip: kepala tambahan TAR terlalu besar");
    char* isi = malloc((size_t)ukuran + 1);
    if (isi == NULL) return gagalK(r->k, vm.galatOS, "arsip: memori tidak cukup");
    bool ok = true;
    if (ukuran > 0) {
        int rc = tarBaca(r, (uint8_t*)isi, (size_t)ukuran);
        if (rc == 0) tarTerpotong(r);
        ok = rc == 1;
    }
    ok = ok && tarLewati(r, padTar(ukuran));
    isi[ok ? ukuran : 0] = '\0';
    if (ok && tipe == 'L') {
        size_t n = strlen(isi);
        if (n > BATAS_NAMA) {
            ok = gagalK(r->k, vm.galatNilai, "arsip: nama entri terlalu panjang");
        } else {
            memcpy(namaMeta, isi, n + 1);
            *adaNama = true;
        }
    } else if (ok && tipe == 'x') {
        ok = paxUrai(r, isi, (size_t)ukuran, namaMeta, adaNama, ukuranMeta, adaUkuran);
    }
    free(isi);
    return ok;
}

static void tarNamaKepala(const uint8_t* h, char* nama) {
    size_t pn = strnlen((const char*)h, 100), pp = 0;
    // The 155-byte prefix exists only in POSIX ustar ("ustar\0" "00");
    // GNU's "ustar  " header keeps other fields at that offset.
    if (memcmp(h + 257, "ustar\0" "00", 8) == 0) pp = strnlen((const char*)h + 345, 155);
    size_t o = 0;
    if (pp > 0) {
        memcpy(nama, h + 345, pp);
        o = pp;
        nama[o++] = '/';
    }
    memcpy(nama + o, h, pn);
    nama[o + pn] = '\0';
}

// Next real entry. 1 = entry, 0 = end of archive, -1 = error.
static int tarBerikut(PembacaTar* r, EntriTar* e) {
    if (!tarLewati(r, r->sisaData + r->sisaPad)) return -1;
    r->sisaData = r->sisaPad = 0;
    char namaMeta[BATAS_NAMA + 2];
    bool adaNamaMeta = false, adaUkuranMeta = false;
    uint64_t ukuranMeta = 0;
    uint8_t h[BLOK];
    for (;;) {
        int rc = tarBaca(r, h, BLOK);
        if (rc < 0) return -1;
        if (rc == 0) {
            if (adaNamaMeta || adaUkuranMeta) { tarTerpotong(r); return -1; }
            return 0;   // no end-of-archive blocks: accepted, like GNU tar
        }
        if (blokNol(h)) return 0;
        if (!checksumSah(h)) {
            gagalK(r->k, vm.galatNilai, "arsip: kepala TAR rusak (checksum tidak cocok)");
            return -1;
        }
        // Long-name and PAX headers count too, so they cannot be used to
        // make the parser loop over unbounded metadata.
        int64_t batasKepala = r->k->batasEntri > (INT64_MAX - 8) / 3 ? INT64_MAX : r->k->batasEntri * 3 + 8;
        if (++r->jumlahKepala > batasKepala) {
            gagalK(r->k, vm.galatNilai, "arsip: jumlah entri melebihi batas %lld", (long long)r->k->batasEntri);
            return -1;
        }
        uint64_t ukuran;
        if (!angkaTar(h + 124, 12, &ukuran)) {
            gagalK(r->k, vm.galatNilai, "arsip: ukuran entri TAR rusak");
            return -1;
        }
        char tipe = (char)h[156];
        if (tipe == 'L' || tipe == 'K' || tipe == 'x' || tipe == 'g') {
            if (!tarMeta(r, tipe, ukuran, namaMeta, &adaNamaMeta, &ukuranMeta, &adaUkuranMeta)) return -1;
            continue;
        }
        if (adaNamaMeta) memcpy(e->nama, namaMeta, strlen(namaMeta) + 1);
        else tarNamaKepala(h, e->nama);
        if (adaUkuranMeta) ukuran = ukuranMeta;
        // Keeps sisaData + sisaPad from wrapping (a PAX size near 2^64 would
        // otherwise make the listing resume inside the entry's data).
        if (ukuran > (uint64_t)INT64_MAX) {
            gagalK(r->k, vm.galatNilai, "arsip: ukuran entri TAR rusak");
            return -1;
        }
        size_t pn = strlen(e->nama);
        switch (tipe) {
            case '0': case '\0': case '7':
                e->jenis = (pn > 0 && e->nama[pn - 1] == '/') ? 'd' : 'f';
                break;
            case '5': e->jenis = 'd'; break;
            case '1': e->jenis = 'h'; break;
            case '2': e->jenis = 'l'; break;
            default:  e->jenis = '?'; break;
        }
        // Directories and links carry no data (POSIX ignores their size).
        bool adaData = e->jenis == 'f' || e->jenis == '?';
        e->ukuran = adaData ? ukuran : 0;
        r->sisaData = adaData ? ukuran : 0;
        r->sisaPad = adaData ? padTar(ukuran) : 0;
        return 1;
    }
}

static bool tarSalinData(PembacaTar* r, FILE* keluar, const char* nama) {
    uint8_t buf[POTONG];
    while (r->sisaData > 0) {
        size_t bagian = r->sisaData > sizeof(buf) ? sizeof(buf) : (size_t)r->sisaData;
        int rc = tarBaca(r, buf, bagian);
        if (rc == 0) return tarTerpotong(r);
        if (rc < 0) return false;
        if (fwrite(buf, 1, bagian, keluar) != bagian) {
            return gagalK(r->k, vm.galatOS, "arsip.ekstrak(): gagal menulis '%s'", nama);
        }
        r->sisaData -= bagian;
    }
    return true;
}

static bool tarEkstrakEntri(Konteks* k, PembacaTar* r, const EntriTar* e, const char* bersih) {
    if (e->jenis == 'd') return siapkanDirektori(k, bersih, true, true);
    FILE* keluar = bukaTujuanBerkas(k, bersih);
    if (keluar == NULL) return false;
    bool ok = tarSalinData(r, keluar, bersih);
    if (fclose(keluar) != 0 && ok) ok = gagalK(k, vm.galatOS, "arsip.ekstrak(): gagal menulis '%s'", bersih);
    return ok;
}

static bool prosesTar(Konteks* k, bool gzip) {
    PembacaTar r = { .k = k };
    r.f = sistemBukaBerkas(k->jalur, "rb");
    if (r.f == NULL) return gagalK(k, vm.galatOS, "arsip: gagal membuka '%s'", k->jalur);
    if (gzip) {
        r.gz = aliranInflasiBaru(KOMPRES_GZIP, sumberTar, r.f);
        if (r.gz == NULL) { fclose(r.f); return gagalK(k, vm.galatOS, "arsip: memori tidak cukup"); }
    }
    EntriTar* e = malloc(sizeof(EntriTar));
    char* bersih = malloc(BATAS_NAMA + 2);
    bool ok = e != NULL && bersih != NULL;
    if (!ok) gagalK(k, vm.galatOS, "arsip: memori tidak cukup");
    while (ok) {
        int rc = tarBerikut(&r, e);
        if (rc <= 0) { ok = rc == 0; break; }
        int keputusan = nilaiEntri(k, e->nama, e->jenis, e->ukuran, 0, false, bersih);
        if (keputusan < 0) { ok = false; break; }
        if (keputusan == 0) continue;
        if (k->mode == MODE_PERIKSA) {
            ok = periksaTujuan(k, bersih, e->jenis == 'd');
            continue;
        }
        ok = tarEkstrakEntri(k, &r, e, bersih);
        if (ok) catatEkstrak(k, bersih);
    }
    free(e);
    free(bersih);
    aliranInflasiTutup(r.gz);
    fclose(r.f);
    return ok;
}

// ---------------------------------------------------------------------------
// ZIP reader (miniz)
// ---------------------------------------------------------------------------

typedef struct {
    FILE* f;
    uint64_t ditulis;
    uint64_t batas;
    bool lewat;
} TulisZip;

// Abort (return short) once an entry produces more than its central
// directory declared: the declared size is what the limits were checked
// against, so a lying header must not let more through.
static size_t tulisZip(void* opaque, mz_uint64 ofs, const void* buf, size_t n) {
    (void)ofs;
    TulisZip* t = opaque;
    if (t->ditulis + n > t->batas) { t->lewat = true; return 0; }
    size_t w = fwrite(buf, 1, n, t->f);
    t->ditulis += w;
    return w;
}

static bool zipSymlink(const mz_zip_archive_file_stat* st) {
    return (st->m_version_made_by >> 8) == 3 && ((st->m_external_attr >> 16) & 0170000) == 0120000;
}

static bool zipEkstrakEntri(Konteks* k, mz_zip_archive* z, mz_uint i, char jenis,
                            const mz_zip_archive_file_stat* st, const char* bersih) {
    if (jenis == 'd') return siapkanDirektori(k, bersih, true, true);
    TulisZip t = { .f = bukaTujuanBerkas(k, bersih), .batas = st->m_uncomp_size };
    if (t.f == NULL) return false;
    bool ok = true;
    if (!mz_zip_reader_extract_to_callback(z, i, tulisZip, &t, 0)) {
        ok = gagalK(k, vm.galatNilai, t.lewat ? "arsip.ekstrak(): entri '%s' lebih besar dari ukuran yang dinyatakan"
                                              : "arsip.ekstrak(): entri '%s' rusak", bersih);
    }
    if (fclose(t.f) != 0 && ok) ok = gagalK(k, vm.galatOS, "arsip.ekstrak(): gagal menulis '%s'", bersih);
    return ok;
}

// One ZIP entry: 1 = handled, 0 = error.
static bool zipEntri(Konteks* k, mz_zip_archive* z, mz_uint i, mz_zip_archive_file_stat* st,
                     char* nama, char* bersih) {
    if (!mz_zip_reader_file_stat(z, i, st)) return gagalK(k, vm.galatNilai, "arsip: ZIP rusak (entri %u)", i);
    mz_uint perlu = mz_zip_reader_get_filename(z, i, nama, BATAS_NAMA + 2);
    if (perlu > BATAS_NAMA + 1) return gagalK(k, vm.galatNilai, "arsip: nama entri terlalu panjang");
    if (strlen(nama) + 1 != perlu) return gagalK(k, vm.galatNilai, "arsip: nama entri ZIP memuat NUL");
    char jenis = zipSymlink(st) ? 'l' : st->m_is_directory ? 'd' : 'f';
    if (k->mode != MODE_DAFTAR && jenis == 'f' && (st->m_is_encrypted || !st->m_is_supported)) {
        return gagalK(k, vm.galatNilai, "arsip.ekstrak(): entri '%.200s' terenkripsi atau memakai "
                      "metode yang tidak didukung", nama);
    }
    uint64_t ukuran = jenis == 'f' ? st->m_uncomp_size : 0;
    int keputusan = nilaiEntri(k, nama, jenis, ukuran, st->m_comp_size, jenis == 'f', bersih);
    if (keputusan <= 0) return keputusan == 0;
    if (k->mode == MODE_PERIKSA) {
        // Inflate once with the output discarded (size and CRC checked) so
        // corruption is found before anything is written.
        if (jenis == 'f' && !mz_zip_validate_file(z, i, 0)) {
            return gagalK(k, vm.galatNilai, "arsip: entri ZIP '%.200s' rusak (CRC atau ukuran tidak cocok)", nama);
        }
        return periksaTujuan(k, bersih, jenis == 'd');
    }
    if (!zipEkstrakEntri(k, z, i, jenis, st, bersih)) return false;
    catatEkstrak(k, bersih);
    return true;
}

static bool prosesZip(Konteks* k) {
    FILE* f = sistemBukaBerkas(k->jalur, "rb");
    if (f == NULL) return gagalK(k, vm.galatOS, "arsip: gagal membuka '%s'", k->jalur);
    mz_zip_archive z;
    memset(&z, 0, sizeof(z));
    if (!mz_zip_reader_init_cfile(&z, f, arsipUkuranBerkas(f), 0)) {
        // miniz's own error strings are English; the reason is not needed here.
        gagalK(k, vm.galatNilai, "arsip: '%s' rusak atau bukan ZIP yang didukung", k->jalur);
        fclose(f);
        return false;
    }
    mz_uint jumlah = mz_zip_reader_get_num_files(&z);
    bool ok = true;
    if ((int64_t)jumlah > k->batasEntri) {
        ok = gagalK(k, vm.galatNilai, "arsip: jumlah entri melebihi batas %lld", (long long)k->batasEntri);
    }
    mz_zip_archive_file_stat* st = malloc(sizeof(*st));
    char* nama = malloc(BATAS_NAMA + 2);
    char* bersih = malloc(BATAS_NAMA + 2);
    if (ok && (st == NULL || nama == NULL || bersih == NULL)) ok = gagalK(k, vm.galatOS, "arsip: memori tidak cukup");
    for (mz_uint i = 0; ok && i < jumlah; i++) ok = zipEntri(k, &z, i, st, nama, bersih);
    free(st);
    free(nama);
    free(bersih);
    mz_zip_reader_end(&z);
    fclose(f);
    return ok;
}

// ---------------------------------------------------------------------------
// Natives
// ---------------------------------------------------------------------------

static bool deteksi(Konteks* k, JenisArsip* jenis) {
    FILE* f = sistemBukaBerkas(k->jalur, "rb");
    if (f == NULL) return gagalK(k, vm.galatOS, "arsip: gagal membuka '%s'", k->jalur);
    uint8_t kepala[BLOK];
    size_t n = fread(kepala, 1, sizeof(kepala), f);
    k->ukuranArsip = arsipUkuranBerkas(f);
    fclose(f);
    if (n >= 4 && kepala[0] == 'P' && kepala[1] == 'K' &&
        ((kepala[2] == 3 && kepala[3] == 4) || (kepala[2] == 5 && kepala[3] == 6))) {
        *jenis = ARSIP_ZIP;
    } else if (n >= 2 && kepala[0] == 0x1f && kepala[1] == 0x8b) {
        *jenis = ARSIP_TARGZ;
    } else if (n == BLOK && (blokNol(kepala) || checksumSah(kepala))) {
        *jenis = ARSIP_TAR;
    } else {
        return gagalK(k, vm.galatNilai, "arsip: '%s' bukan arsip TAR, TAR.GZ, atau ZIP yang dikenali", k->jalur);
    }
    return true;
}

static bool proses(Konteks* k) {
    JenisArsip jenis;
    if (!deteksi(k, &jenis)) return false;
    k->total = 0;
    k->jumlah = 0;
    return jenis == ARSIP_ZIP ? prosesZip(k) : prosesTar(k, jenis == ARSIP_TARGZ);
}

static bool ambilBatas(Value v, const char* nama, int64_t min, int64_t* hasil) {
    if (!IS_INT(v) || AS_INT(v) < min) {
        nativeRaiseKelas(vm.galatNilai, "arsip: %s harus bulat >= %lld", nama, (long long)min);
        return false;
    }
    *hasil = AS_INT(v);
    return true;
}

static Value selesai(Konteks* k) {
    pop();   // k->hasil
    if (k->kelasGalat != NULL) {
        nativeRaiseKelas(k->kelasGalat, "%s", k->galat);
        return KOSONG_VAL;
    }
    return OBJ_VAL(k->hasil);
}

Value nativeFnArsipDaftar(int argCount, Value* args) {
    if (argCount != 3 || !IS_STRING(args[0])) {
        nativeRaiseKelas(vm.galatTipe, "_arsip_daftar() memerlukan (jalur, batas_entri, batas_rasio)");
        return KOSONG_VAL;
    }
    Konteks k = { .mode = MODE_DAFTAR, .jalur = AS_STRING(args[0])->chars };
    // Listing a .tar.gz inflates the whole stream, so the gzip-layer ratio
    // limit applies here too: otherwise a tiny file keeps the CPU busy.
    if (!ambilBatas(args[1], "batas_entri", 1, &k.batasEntri) ||
        !ambilBatas(args[2], "batas_rasio", 1, &k.batasRasio)) {
        return KOSONG_VAL;
    }
    k.batasUkuran = INT64_MAX;
    k.hasil = newList();
    push(OBJ_VAL(k.hasil));
    proses(&k);
    return selesai(&k);
}

Value nativeFnArsipEkstrak(int argCount, Value* args) {
    if (argCount != 6 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        nativeRaiseKelas(vm.galatTipe, "_arsip_ekstrak() memerlukan (jalur, tujuan, ukuran, entri, rasio, lewati_tautan)");
        return KOSONG_VAL;
    }
    Konteks k = { .mode = MODE_PERIKSA, .jalur = AS_STRING(args[0])->chars,
                  .tujuan = AS_STRING(args[1])->chars, .lewatiTautan = !isFalsey(args[5]) };
    if (!ambilBatas(args[2], "batas_ukuran", 0, &k.batasUkuran) ||
        !ambilBatas(args[3], "batas_entri", 1, &k.batasEntri) ||
        !ambilBatas(args[4], "batas_rasio", 1, &k.batasRasio)) {
        return KOSONG_VAL;
    }
    k.hasil = newList();
    push(OBJ_VAL(k.hasil));
    // Pass 1 reads (and decompresses) everything and writes nothing: a bad
    // entry anywhere rejects the archive before the destination is touched.
    if (proses(&k) && siapkanTujuan(&k)) {
        k.mode = MODE_EKSTRAK;
        proses(&k);
    }
    return selesai(&k);
}

Value nativeFnArsipJenis(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        nativeRaiseKelas(vm.galatTipe, "_arsip_jenis() memerlukan 1 teks");
        return KOSONG_VAL;
    }
    const char* nama;
    switch (sistemJenisTanpaTautan(AS_STRING(args[0])->chars)) {
        case SISTEM_JALUR_BERKAS: nama = "berkas"; break;
        case SISTEM_JALUR_DIREKTORI: nama = "direktori"; break;
        case SISTEM_JALUR_TAUTAN: nama = "tautan"; break;
        case SISTEM_JALUR_LAIN: nama = "lain"; break;
        default: return KOSONG_VAL;
    }
    return OBJ_VAL(copyString(nama, (int)strlen(nama)));
}

