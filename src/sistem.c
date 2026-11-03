// Operating-system natives -- see sistem.h.
//
// Errors use GalatOS with a reason that reads the same on every platform
// ("tidak ada", "bukan direktori", ...), not strerror()/FormatMessage text,
// so error tests and user code do not depend on the OS language.

#if !defined(_WIN32)
#define _DEFAULT_SOURCE   // fork, mkstemp, readlink, realpath: hidden by -std=c11
#endif

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #include <shellapi.h>   // CommandLineToArgvW
#else
    #include <dirent.h>
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <unistd.h>
    #ifdef __APPLE__
        #include <mach-o/dyld.h>
    #endif
    extern char** environ;
#endif

#include "common.h"
#include "memory.h"
#include "sistem.h"
#include "vm.h"

#define KELUARAN_MAKS (64u * 1024u * 1024u)   // per stream captured from a child

static bool sandbox = false;
static const char* programArgv0 = NULL;

void sistemAturArgv0(const char* argv0) { programArgv0 = argv0; }
void sistemAturSandbox(bool aktif) { sandbox = aktif; }
bool sistemSandbox(void) { return sandbox; }

// Refuse `nama` under --sandbox. Returns true (and leaves a GalatIzin
// pending) when refused.
static bool ditolakSandbox(const char* nama) {
    if (!sandbox) return false;
    nativeRaiseKelas(vm.galatIzin, "%s() tidak diizinkan dalam mode --sandbox", nama);
    return true;
}

// ---------------------------------------------------------------------------
// Portable error reasons
// ---------------------------------------------------------------------------

typedef enum {
    ALASAN_TIDAK_ADA,
    ALASAN_BUKAN_DIREKTORI,
    ALASAN_AKSES_DITOLAK,
    ALASAN_SUDAH_ADA,
    ALASAN_TIDAK_KOSONG,
    ALASAN_LAIN
} Alasan;

static const char* teksAlasan(Alasan a) {
    switch (a) {
        case ALASAN_TIDAK_ADA: return "tidak ada";
        case ALASAN_BUKAN_DIREKTORI: return "bukan direktori";
        case ALASAN_AKSES_DITOLAK: return "akses ditolak";
        case ALASAN_SUDAH_ADA: return "sudah ada";
        case ALASAN_TIDAK_KOSONG: return "direktori tidak kosong";
        default: return "galat sistem";
    }
}

static Alasan alasanTerakhir(void) {
#ifdef _WIN32
    switch (GetLastError()) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_INVALID_NAME:
            return ALASAN_TIDAK_ADA;
        case ERROR_DIRECTORY: return ALASAN_BUKAN_DIREKTORI;
        case ERROR_ACCESS_DENIED: return ALASAN_AKSES_DITOLAK;
        case ERROR_ALREADY_EXISTS:
        case ERROR_FILE_EXISTS:
            return ALASAN_SUDAH_ADA;
        case ERROR_DIR_NOT_EMPTY: return ALASAN_TIDAK_KOSONG;
        default: return ALASAN_LAIN;
    }
#else
    switch (errno) {
        case ENOENT: return ALASAN_TIDAK_ADA;
        case ENOTDIR: return ALASAN_BUKAN_DIREKTORI;
        case EACCES:
        case EPERM:
            return ALASAN_AKSES_DITOLAK;
        case EEXIST: return ALASAN_SUDAH_ADA;
        case ENOTEMPTY: return ALASAN_TIDAK_KOSONG;
        default: return ALASAN_LAIN;
    }
#endif
}

static void galatOS(const char* fungsi, const char* jalur, Alasan a) {
    nativeRaiseKelas(vm.galatOS, "%s(): '%s' %s", fungsi, jalur, teksAlasan(a));
}

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

static bool teksArg(int argCount, Value* args, int index, const char* fungsi, const char** out) {
    if (index >= argCount || !IS_STRING(args[index])) {
        nativeRaiseKelas(vm.galatTipe, "%s() memerlukan teks", fungsi);
        return false;
    }
    ObjString* s = AS_STRING(args[index]);
    if ((size_t)s->length != strlen(s->chars)) {
        nativeRaiseKelas(vm.galatNilai, "%s(): teks tidak boleh memuat karakter NUL", fungsi);
        return false;
    }
    *out = s->chars;
    return true;
}

// A teks value from C bytes (copied).
static Value teks(const char* chars, size_t length) {
    if (length > (size_t)INT_MAX) length = (size_t)INT_MAX;
    return OBJ_VAL(copyString(chars, (int)length));
}

#ifdef _WIN32
// UTF-8 <-> UTF-16 for the W APIs (so non-ASCII paths and arguments work).
static wchar_t* keLebar(const char* utf8) {
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (n <= 0) return NULL;
    wchar_t* w = malloc(sizeof(wchar_t) * (size_t)n);
    if (w != NULL) MultiByteToWideChar(CP_UTF8, 0, utf8, -1, w, n);
    return w;
}

static char* keUtf8(const wchar_t* w) {
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
    if (n <= 0) return NULL;
    char* s = malloc((size_t)n);
    if (s != NULL) WideCharToMultiByte(CP_UTF8, 0, w, -1, s, n, NULL, NULL);
    return s;
}
#endif

// ---------------------------------------------------------------------------
// UTF-8 at the OS boundary (see sistem.h)
// ---------------------------------------------------------------------------

#ifdef _WIN32
static UINT cpKeluaranAsal = 0;
static UINT cpMasukanAsal = 0;

static void pulihkanKonsol(void) {
    if (cpKeluaranAsal != 0) SetConsoleOutputCP(cpKeluaranAsal);
    if (cpMasukanAsal != 0) SetConsoleCP(cpMasukanAsal);
}

static const char** argvUtf8(int* argc) {
    int n = 0;
    wchar_t** lebar = CommandLineToArgvW(GetCommandLineW(), &n);
    if (lebar == NULL) return NULL;
    const char** hasil = n > 0 ? malloc(sizeof(char*) * ((size_t)n + 1)) : NULL;
    for (int i = 0; hasil != NULL && i < n; i++) {
        hasil[i] = keUtf8(lebar[i]);
        if (hasil[i] == NULL) {
            while (i > 0) free((void*)hasil[--i]);
            free(hasil);
            hasil = NULL;
        }
    }
    LocalFree(lebar);
    if (hasil == NULL) return NULL;
    hasil[n] = NULL;
    *argc = n;
    return hasil;   // lives as long as the process: `argumen` refers to it
}
#endif

void sistemSiapkanProses(int* argc, const char*** argv) {
#ifdef _WIN32
    const char** utf8 = argvUtf8(argc);
    if (utf8 != NULL) *argv = utf8;

    // GetConsole*CP() returns 0 when no console is attached (a GUI parent,
    // or a service); redirected output then stays raw UTF-8 bytes, as before.
    UINT keluar = GetConsoleOutputCP();
    UINT masuk = GetConsoleCP();
    if (keluar != 0 && keluar != CP_UTF8 && SetConsoleOutputCP(CP_UTF8)) cpKeluaranAsal = keluar;
    if (masuk != 0 && masuk != CP_UTF8 && SetConsoleCP(CP_UTF8)) cpMasukanAsal = masuk;
    if (cpKeluaranAsal != 0 || cpMasukanAsal != 0) atexit(pulihkanKonsol);
#else
    (void)argc;
    (void)argv;
#endif
}

FILE* sistemBukaBerkas(const char* jalur, const char* mode) {
#ifdef _WIN32
    wchar_t m[8];
    size_t i = 0;
    for (; mode[i] != '\0' && i + 1 < sizeof(m) / sizeof(m[0]); i++) m[i] = (wchar_t)mode[i];
    m[i] = L'\0';
    wchar_t* w = keLebar(jalur);
    if (w == NULL) return NULL;
    FILE* f = _wfopen(w, m);
    free(w);
    return f;
#else
    return fopen(jalur, mode);
#endif
}

int sistemHapusBerkas(const char* jalur) {
#ifdef _WIN32
    wchar_t* w = keLebar(jalur);
    if (w == NULL) return -1;
    int hasil = _wremove(w);
    free(w);
    return hasil;
#else
    return remove(jalur);
#endif
}

char* sistemAmbilLingkungan(const char* nama) {
#ifdef _WIN32
    wchar_t* w = keLebar(nama);
    if (w == NULL) return NULL;
    DWORD n = GetEnvironmentVariableW(w, NULL, 0);
    wchar_t* isi = n > 0 ? malloc(sizeof(wchar_t) * n) : NULL;
    char* hasil = NULL;
    if (isi != NULL && GetEnvironmentVariableW(w, isi, n) < n) hasil = keUtf8(isi);
    free(isi);
    free(w);
    return hasil;
#else
    const char* isi = getenv(nama);
    if (isi == NULL) return NULL;
    size_t n = strlen(isi) + 1;
    char* hasil = malloc(n);
    if (hasil != NULL) memcpy(hasil, isi, n);
    return hasil;
#endif
}

// What is at `jalur`: 0 nothing, 1 file, 2 directory, 3 something else,
// -1 error (reason in *alasan). Size and mtime (seconds since the epoch) for
// existing paths.
static int jenisJalur(const char* jalur, uint64_t* ukuran, double* waktu, Alasan* alasan) {
#ifdef _WIN32
    wchar_t* w = keLebar(jalur);
    if (w == NULL) { *alasan = ALASAN_LAIN; return -1; }
    WIN32_FILE_ATTRIBUTE_DATA data;
    BOOL ok = GetFileAttributesExW(w, GetFileExInfoStandard, &data);
    free(w);
    if (!ok) {
        Alasan a = alasanTerakhir();
        if (a == ALASAN_TIDAK_ADA || a == ALASAN_BUKAN_DIREKTORI) return 0;
        *alasan = a;
        return -1;
    }
    *ukuran = ((uint64_t)data.nFileSizeHigh << 32) | data.nFileSizeLow;
    uint64_t ft = ((uint64_t)data.ftLastWriteTime.dwHighDateTime << 32) |
                  data.ftLastWriteTime.dwLowDateTime;
    *waktu = ((double)ft - 116444736000000000.0) / 1e7;   // 1601 -> 1970, 100 ns ticks
    return (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 2 : 1;
#else
    struct stat st;
    if (stat(jalur, &st) != 0) {
        Alasan a = alasanTerakhir();
        if (a == ALASAN_TIDAK_ADA || a == ALASAN_BUKAN_DIREKTORI) return 0;
        *alasan = a;
        return -1;
    }
    *ukuran = (uint64_t)st.st_size;
    *waktu = (double)st.st_mtime;
    if (S_ISDIR(st.st_mode)) return 2;
    if (S_ISREG(st.st_mode)) return 1;
    return 3;
#endif
}

// ---------------------------------------------------------------------------
// Process, stderr, environment
// ---------------------------------------------------------------------------

// keluar(kode = 0): end the program now. A teks is printed to stderr and the
// exit code is 1, like sys.exit("pesan"). Unlike Python's SystemExit this
// does not run `akhirnya` blocks.
Value nativeFnKeluar(int argCount, Value* args) {
    if (argCount > 1) {
        nativeRaiseKelas(vm.galatTipe, "keluar() menerima paling banyak 1 argumen");
        return KOSONG_VAL;
    }
    int kode = 0;
    if (argCount == 1 && !IS_KOSONG(args[0])) {
        if (IS_BOOL(args[0])) {
            kode = AS_BOOL(args[0]) ? 1 : 0;
        } else if (IS_INT(args[0])) {
            int64_t k = AS_INT(args[0]);
            kode = (k < INT_MIN || k > INT_MAX) ? 1 : (int)k;
        } else if (IS_STRING(args[0])) {
            fflush(stdout);
            fprintf(stderr, "%s\n", AS_CSTRING(args[0]));
            kode = 1;
        } else {
            nativeRaiseKelas(vm.galatTipe, "keluar() memerlukan bulat, teks, atau kosong");
            return KOSONG_VAL;
        }
    }
    fflush(stdout);
    fflush(stderr);
    exit(kode);
}

// tulis_galat(teks): write to stderr as-is (no newline added).
Value nativeFnTulisGalat(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        nativeRaiseKelas(vm.galatTipe, "tulis_galat() memerlukan 1 teks");
        return KOSONG_VAL;
    }
    fflush(stdout);   // keep stdout and stderr in the order they were written
    fwrite(AS_CSTRING(args[0]), 1, (size_t)AS_STRING(args[0])->length, stderr);
    fflush(stderr);
    return KOSONG_VAL;
}

// lingkungan() -> kamus of every environment variable.
Value nativeFnLingkungan(int argCount, Value* args) {
    (void)args;
    if (argCount != 0) {
        nativeRaiseKelas(vm.galatTipe, "lingkungan() tidak menerima argumen");
        return KOSONG_VAL;
    }
    if (ditolakSandbox("lingkungan")) return KOSONG_VAL;
    ObjDict* hasil = newDict();
    push(OBJ_VAL(hasil));                                 // GC: root while filling
#ifdef _WIN32
    wchar_t* blok = GetEnvironmentStringsW();
    for (wchar_t* p = blok; p != NULL && *p != L'\0'; p += wcslen(p) + 1) {
        if (*p == L'=') continue;   // hidden per-drive entries such as "=C:=C:\"
        char* entri = keUtf8(p);
        if (entri == NULL) continue;
        char* sama = strchr(entri, '=');
        if (sama != NULL) {
            Value kunci = teks(entri, (size_t)(sama - entri));
            push(kunci);
            Value nilai = teks(sama + 1, strlen(sama + 1));
            push(nilai);
            dictSet(hasil, kunci, nilai);
            pop();
            pop();
        }
        free(entri);
    }
    if (blok != NULL) FreeEnvironmentStringsW(blok);
#else
    for (char** e = environ; e != NULL && *e != NULL; e++) {
        char* sama = strchr(*e, '=');
        if (sama == NULL) continue;
        Value kunci = teks(*e, (size_t)(sama - *e));
        push(kunci);
        Value nilai = teks(sama + 1, strlen(sama + 1));
        push(nilai);
        dictSet(hasil, kunci, nilai);
        pop();
        pop();
    }
#endif
    pop();
    return OBJ_VAL(hasil);
}

// ambil_lingkungan(nama, bawaan = kosong)
Value nativeFnAmbilLingkungan(int argCount, Value* args) {
    const char* nama;
    if (argCount < 1 || argCount > 2) {
        nativeRaiseKelas(vm.galatTipe, "ambil_lingkungan() memerlukan 1 atau 2 argumen");
        return KOSONG_VAL;
    }
    if (!teksArg(argCount, args, 0, "ambil_lingkungan", &nama)) return KOSONG_VAL;
    if (ditolakSandbox("ambil_lingkungan")) return KOSONG_VAL;
    Value bawaan = argCount == 2 ? args[1] : KOSONG_VAL;
    char* isi = sistemAmbilLingkungan(nama);
    if (isi == NULL) return bawaan;
    Value hasil = teks(isi, strlen(isi));
    free(isi);
    return hasil;
}

// ---------------------------------------------------------------------------
// Directories
// ---------------------------------------------------------------------------

Value nativeFnDirektoriKini(int argCount, Value* args) {
    (void)args;
    if (argCount != 0) {
        nativeRaiseKelas(vm.galatTipe, "direktori_kini() tidak menerima argumen");
        return KOSONG_VAL;
    }
#ifdef _WIN32
    DWORD n = GetCurrentDirectoryW(0, NULL);
    wchar_t* w = malloc(sizeof(wchar_t) * (n > 0 ? n : 1));
    if (w == NULL || GetCurrentDirectoryW(n, w) == 0) {
        free(w);
        nativeRaiseKelas(vm.galatOS, "direktori_kini(): %s", teksAlasan(ALASAN_LAIN));
        return KOSONG_VAL;
    }
    char* utf8 = keUtf8(w);
    free(w);
    if (utf8 == NULL) return KOSONG_VAL;
    Value hasil = teks(utf8, strlen(utf8));
    free(utf8);
    return hasil;
#else
    size_t ukuran = 256;
    for (;;) {
        char* buf = malloc(ukuran);
        if (buf == NULL) return KOSONG_VAL;
        if (getcwd(buf, ukuran) != NULL) {
            Value hasil = teks(buf, strlen(buf));
            free(buf);
            return hasil;
        }
        free(buf);
        if (errno != ERANGE || ukuran > (1u << 20)) {
            nativeRaiseKelas(vm.galatOS, "direktori_kini(): %s", teksAlasan(alasanTerakhir()));
            return KOSONG_VAL;
        }
        ukuran *= 2;
    }
#endif
}

static int bandingNama(const void* a, const void* b) {
    return strcmp(*(char* const*)a, *(char* const*)b);
}

// isi_direktori(jalur = ".") -> sorted list of entry names (no "." / "..").
Value nativeFnIsiDirektori(int argCount, Value* args) {
    const char* jalur = ".";
    if (argCount > 1) {
        nativeRaiseKelas(vm.galatTipe, "isi_direktori() menerima paling banyak 1 argumen");
        return KOSONG_VAL;
    }
    if (argCount == 1 && !teksArg(argCount, args, 0, "isi_direktori", &jalur)) return KOSONG_VAL;
    if (ditolakSandbox("isi_direktori")) return KOSONG_VAL;

    uint64_t ukuran;
    double waktu;
    Alasan alasan = ALASAN_LAIN;
    int jenis = jenisJalur(jalur, &ukuran, &waktu, &alasan);
    if (jenis != 2) {
        galatOS("isi_direktori", jalur,
                jenis == 0 ? ALASAN_TIDAK_ADA : jenis < 0 ? alasan : ALASAN_BUKAN_DIREKTORI);
        return KOSONG_VAL;
    }

    char** nama = NULL;
    size_t jumlah = 0, kapasitas = 0;
    bool gagal = false;
#ifdef _WIN32
    size_t panjangJalur = strlen(jalur);
    char* pola = malloc(panjangJalur + 3);
    if (pola == NULL) return KOSONG_VAL;
    memcpy(pola, jalur, panjangJalur);
    size_t k = panjangJalur;
    if (k > 0 && pola[k - 1] != '/' && pola[k - 1] != '\\') pola[k++] = '\\';
    pola[k++] = '*';
    pola[k] = '\0';
    wchar_t* w = keLebar(pola);
    free(pola);
    WIN32_FIND_DATAW data;
    HANDLE h = w != NULL ? FindFirstFileW(w, &data) : INVALID_HANDLE_VALUE;
    free(w);
    if (h == INVALID_HANDLE_VALUE) {
        galatOS("isi_direktori", jalur, alasanTerakhir());
        return KOSONG_VAL;
    }
    do {
        if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0) continue;
        char* entri = keUtf8(data.cFileName);
        if (entri == NULL) continue;
        if (jumlah == kapasitas) {
            kapasitas = kapasitas < 16 ? 16 : kapasitas * 2;
            char** baru = realloc(nama, sizeof(char*) * kapasitas);
            if (baru == NULL) { free(entri); gagal = true; break; }
            nama = baru;
        }
        nama[jumlah++] = entri;
    } while (FindNextFileW(h, &data));
    FindClose(h);
#else
    DIR* dir = opendir(jalur);
    if (dir == NULL) {
        galatOS("isi_direktori", jalur, alasanTerakhir());
        return KOSONG_VAL;
    }
    struct dirent* e;
    while ((e = readdir(dir)) != NULL) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        char* entri = malloc(strlen(e->d_name) + 1);
        if (entri == NULL) { gagal = true; break; }
        strcpy(entri, e->d_name);
        if (jumlah == kapasitas) {
            kapasitas = kapasitas < 16 ? 16 : kapasitas * 2;
            char** baru = realloc(nama, sizeof(char*) * kapasitas);
            if (baru == NULL) { free(entri); gagal = true; break; }
            nama = baru;
        }
        nama[jumlah++] = entri;
    }
    closedir(dir);
#endif
    if (!gagal && jumlah > 1) qsort(nama, jumlah, sizeof(char*), bandingNama);

    ObjList* hasil = newList();
    push(OBJ_VAL(hasil));                                 // GC: root while filling
    for (size_t i = 0; i < jumlah; i++) {
        if (!gagal) {
            Value v = teks(nama[i], strlen(nama[i]));
            push(v);
            writeValueArray(&hasil->items, v);
            pop();
        }
        free(nama[i]);
    }
    free(nama);
    pop();
    if (gagal) {
        nativeRaiseKelas(vm.galatOS, "isi_direktori(): memori habis");
        return KOSONG_VAL;
    }
    return OBJ_VAL(hasil);
}

Value nativeFnBuatDirektori(int argCount, Value* args) {
    const char* jalur;
    if (argCount != 1 || !teksArg(argCount, args, 0, "buat_direktori", &jalur)) {
        if (argCount != 1) nativeRaiseKelas(vm.galatTipe, "buat_direktori() memerlukan 1 teks");
        return KOSONG_VAL;
    }
#ifdef _WIN32
    wchar_t* w = keLebar(jalur);
    BOOL ok = w != NULL && CreateDirectoryW(w, NULL);
    free(w);
    if (!ok) galatOS("buat_direktori", jalur, alasanTerakhir());
#else
    if (mkdir(jalur, 0777) != 0) galatOS("buat_direktori", jalur, alasanTerakhir());
#endif
    return KOSONG_VAL;
}

Value nativeFnHapusDirektori(int argCount, Value* args) {
    const char* jalur;
    if (argCount != 1 || !teksArg(argCount, args, 0, "hapus_direktori", &jalur)) {
        if (argCount != 1) nativeRaiseKelas(vm.galatTipe, "hapus_direktori() memerlukan 1 teks");
        return KOSONG_VAL;
    }
    // Decide "not a directory" ourselves: Windows reports a file here as
    // ERROR_DIRECTORY on some versions and ERROR_ACCESS_DENIED on others.
    uint64_t ukuran;
    double waktu;
    Alasan alasan = ALASAN_LAIN;
    int jenis = jenisJalur(jalur, &ukuran, &waktu, &alasan);
    if (jenis != 2) {
        galatOS("hapus_direktori", jalur,
                jenis == 0 ? ALASAN_TIDAK_ADA : jenis < 0 ? alasan : ALASAN_BUKAN_DIREKTORI);
        return KOSONG_VAL;
    }
#ifdef _WIN32
    wchar_t* w = keLebar(jalur);
    BOOL ok = w != NULL && RemoveDirectoryW(w);
    free(w);
    if (!ok) galatOS("hapus_direktori", jalur, alasanTerakhir());
#else
    if (rmdir(jalur) != 0) {
        // Linux says ENOTEMPTY, some BSDs EEXIST for the same thing.
        Alasan a = alasanTerakhir();
        galatOS("hapus_direktori", jalur, a == ALASAN_SUDAH_ADA ? ALASAN_TIDAK_KOSONG : a);
    }
#endif
    return KOSONG_VAL;
}

// _jalur_nyata(jalur) -> jalur absolut setelah symlink diikuti, atau kosong
// bila jalur tidak ada. Dipakai penyaji berkas statis untuk membuktikan bahwa
// berkas hasil gabungan benar-benar berada di bawah akar: memeriksa teks
// jalurnya saja tidak cukup, karena sebuah symlink di dalam akar dapat
// menunjuk ke mana pun di luar.
Value nativeFnJalurNyata(int argCount, Value* args) {
    const char* jalur;
    if (argCount != 1 || !teksArg(argCount, args, 0, "_jalur_nyata", &jalur)) {
        if (argCount != 1) nativeRaiseKelas(vm.galatTipe, "_jalur_nyata() memerlukan 1 teks");
        return KOSONG_VAL;
    }
#ifdef _WIN32
    wchar_t* w = keLebar(jalur);
    wchar_t* penuh = w != NULL ? _wfullpath(NULL, w, 0) : NULL;
    free(w);
    char* nyata = penuh != NULL ? keUtf8(penuh) : NULL;
    free(penuh);
    if (nyata == NULL) return KOSONG_VAL;
    // _wfullpath() tidak memeriksa keberadaan; jaga agar perilakunya sama
    // dengan POSIX (kosong bila tidak ada).
    uint64_t ukuranAbai = 0;
    double waktuAbai = 0;
    Alasan alasanAbai = ALASAN_LAIN;
    Value hasil = jenisJalur(nyata, &ukuranAbai, &waktuAbai, &alasanAbai) > 0
                      ? teks(nyata, strlen(nyata))
                      : KOSONG_VAL;
    free(nyata);
    return hasil;
#else
    char nyata[PATH_MAX];
    if (realpath(jalur, nyata) == NULL) return KOSONG_VAL;
    return teks(nyata, strlen(nyata));
#endif
}

// _info_jalur(jalur) -> ("berkas" | "direktori" | "lain", ukuran, waktu_ubah)
// or kosong when nothing is there.
Value nativeFnInfoJalur(int argCount, Value* args) {
    const char* jalur;
    if (argCount != 1 || !teksArg(argCount, args, 0, "_info_jalur", &jalur)) {
        if (argCount != 1) nativeRaiseKelas(vm.galatTipe, "_info_jalur() memerlukan 1 teks");
        return KOSONG_VAL;
    }
    uint64_t ukuran = 0;
    double waktu = 0;
    Alasan alasan = ALASAN_LAIN;
    int jenis = jenisJalur(jalur, &ukuran, &waktu, &alasan);
    if (jenis < 0) {
        galatOS("_info_jalur", jalur, alasan);
        return KOSONG_VAL;
    }
    if (jenis == 0) return KOSONG_VAL;
    const char* nama = jenis == 1 ? "berkas" : jenis == 2 ? "direktori" : "lain";
    ObjTuple* hasil = newTuple();
    push(OBJ_VAL(hasil));
    Value v = teks(nama, strlen(nama));
    push(v);
    writeValueArray(&hasil->items, v);
    pop();
    writeValueArray(&hasil->items, INT_VAL(ukuran > (uint64_t)INT64_MAX ? INT64_MAX : (int64_t)ukuran));
    writeValueArray(&hasil->items, NUMBER_VAL(waktu));
    pop();
    return OBJ_VAL(hasil);
}

// Copy `src` into `out` when it fits. The interpreter path is only useful
// whole: a truncated path would point somewhere else.
static bool salinJalur(char* out, size_t ukuran, const char* src, size_t panjang) {
    if (panjang == 0 || panjang >= ukuran) return false;
    memcpy(out, src, panjang);
    out[panjang] = '\0';
    return true;
}

bool sistemJalurInterpreter(char* out, size_t ukuran) {
#ifdef _WIN32
    DWORD kapasitas = MAX_PATH;
    for (;;) {
        wchar_t* w = malloc(sizeof(wchar_t) * kapasitas);
        if (w == NULL) return false;
        DWORD n = GetModuleFileNameW(NULL, w, kapasitas);
        if (n > 0 && n < kapasitas) {
            char* utf8 = keUtf8(w);
            free(w);
            if (utf8 == NULL) return false;
            bool ok = salinJalur(out, ukuran, utf8, strlen(utf8));
            free(utf8);
            return ok;
        }
        free(w);
        if (n == 0 || kapasitas > 32768) break;
        kapasitas *= 2;
    }
#elif defined(__APPLE__)
    char buf[4096];
    uint32_t panjangBuf = sizeof(buf);
    if (_NSGetExecutablePath(buf, &panjangBuf) == 0) {
        char nyata[PATH_MAX];
        const char* p = realpath(buf, nyata) != NULL ? nyata : buf;
        return salinJalur(out, ukuran, p, strlen(p));
    }
#else
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) return salinJalur(out, ukuran, buf, (size_t)n);
#endif
    if (programArgv0 != NULL) {
#ifndef _WIN32
        char nyata[PATH_MAX];
        if (realpath(programArgv0, nyata) != NULL) return salinJalur(out, ukuran, nyata, strlen(nyata));
#endif
        return salinJalur(out, ukuran, programArgv0, strlen(programArgv0));
    }
    return false;
}

// jalur_indk() -> absolute path of the running interpreter (sys.executable).
Value nativeFnJalurIndk(int argCount, Value* args) {
    (void)args;
    if (argCount != 0) {
        nativeRaiseKelas(vm.galatTipe, "jalur_indk() tidak menerima argumen");
        return KOSONG_VAL;
    }
    char buf[4096];
    if (!sistemJalurInterpreter(buf, sizeof(buf))) {
        nativeRaiseKelas(vm.galatOS, "jalur_indk(): lokasi interpreter tidak diketahui");
        return KOSONG_VAL;
    }
    return teks(buf, strlen(buf));
}

// ---------------------------------------------------------------------------
// Running programs
// ---------------------------------------------------------------------------

typedef struct {
    char* data;
    size_t length;
} Bita;

static bool bitaTambah(Bita* b, const char* data, size_t n) {
    if (b->length + n > KELUARAN_MAKS) return false;
    char* baru = realloc(b->data, b->length + n + 1);
    if (baru == NULL) return false;
    memcpy(baru + b->length, data, n);
    b->data = baru;
    b->length += n;
    b->data[b->length] = '\0';
    return true;
}

// Build the (kode, keluaran, galat) tuple. Output is normalised like Python's
// text=True: "\r\n" and "\r" become "\n", so results match across platforms.
static Value hasilProses(int64_t kode, Bita* keluaran, Bita* galat) {
    ObjTuple* hasil = newTuple();
    push(OBJ_VAL(hasil));                                 // GC: root while filling
    writeValueArray(&hasil->items, INT_VAL(kode));
    Bita* aliran[2] = {keluaran, galat};
    for (int i = 0; i < 2; i++) {
        size_t n = aliran[i]->data != NULL ? normalizeNewlines(aliran[i]->data, aliran[i]->length) : 0;
        Value v = teks(aliran[i]->data != NULL ? aliran[i]->data : "", n);
        push(v);
        writeValueArray(&hasil->items, v);
        pop();
    }
    pop();
    return OBJ_VAL(hasil);
}

#ifdef _WIN32
// Append `arg` to `cmd` quoted the way the MSVCRT/CommandLineToArgvW parser
// splits it back: quotes only when needed, backslashes doubled only when
// they precede a quote.
static bool kutipArgumen(Bita* cmd, const char* arg) {
    if (cmd->length > 0 && !bitaTambah(cmd, " ", 1)) return false;
    if (arg[0] != '\0' && strpbrk(arg, " \t\n\v\"") == NULL) return bitaTambah(cmd, arg, strlen(arg));
    if (!bitaTambah(cmd, "\"", 1)) return false;
    for (const char* p = arg;; p++) {
        size_t garis = 0;
        while (*p == '\\') { garis++; p++; }
        if (*p == '\0') {
            for (size_t i = 0; i < garis * 2; i++) if (!bitaTambah(cmd, "\\", 1)) return false;
            break;
        }
        if (*p == '"') {
            for (size_t i = 0; i < garis * 2 + 1; i++) if (!bitaTambah(cmd, "\\", 1)) return false;
        } else {
            for (size_t i = 0; i < garis; i++) if (!bitaTambah(cmd, "\\", 1)) return false;
        }
        if (!bitaTambah(cmd, p, 1)) return false;
    }
    return bitaTambah(cmd, "\"", 1);
}

static HANDLE berkasSementara(void) {
    wchar_t dir[MAX_PATH + 1], nama[MAX_PATH + 1];
    if (GetTempPathW(MAX_PATH + 1, dir) == 0) return INVALID_HANDLE_VALUE;
    if (GetTempFileNameW(dir, L"ikp", 0, nama) == 0) return INVALID_HANDLE_VALUE;
    SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};    // inheritable by the child
    return CreateFileW(nama, GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, &sa,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
}

static bool bacaHandle(HANDLE h, Bita* out) {
    SetFilePointer(h, 0, NULL, FILE_BEGIN);
    char buf[65536];
    DWORD n;
    while (ReadFile(h, buf, sizeof(buf), &n, NULL) && n > 0) {
        if (!bitaTambah(out, buf, n)) return false;
    }
    return true;
}
#else
static int berkasSementara(void) {
    const char* dir = getenv("TMPDIR");
    if (dir == NULL || dir[0] == '\0') dir = "/tmp";
    char jalur[4096];
    snprintf(jalur, sizeof(jalur), "%s/indk-proses-XXXXXX", dir);
    int fd = mkstemp(jalur);
    if (fd >= 0) unlink(jalur);   // anonymous from now on, gone when closed
    return fd;
}

static bool bacaFd(int fd, Bita* out) {
    if (lseek(fd, 0, SEEK_SET) < 0) return false;
    char buf[65536];
    for (;;) {
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return n == 0;
        if (!bitaTambah(out, buf, (size_t)n)) return false;
    }
}

static bool tulisFd(int fd, const char* data, size_t n) {
    while (n > 0) {
        ssize_t w = write(fd, data, n);
        if (w < 0 && errno == EINTR) continue;
        if (w <= 0) return false;
        data += w;
        n -= (size_t)w;
    }
    return true;
}
#endif

// _jalankan_proses(perintah, masukan) -> (kode_keluar, keluaran, galat).
// `perintah` is a list/tuple of teks, run WITHOUT a shell. The child's stdin,
// stdout and stderr are temporary files rather than pipes, so a child that
// writes a lot to both streams can never deadlock against us.
Value nativeFnJalankanProses(int argCount, Value* args) {
    if (argCount != 2 || sequenceItems(args[0]) == NULL || !IS_STRING(args[1])) {
        nativeRaiseKelas(vm.galatTipe, "_jalankan_proses(perintah, masukan) memerlukan daftar teks dan teks");
        return KOSONG_VAL;
    }
    if (ditolakSandbox("proses.jalankan")) return KOSONG_VAL;
    ValueArray* items = sequenceItems(args[0]);
    if (items->count == 0) {
        nativeRaiseKelas(vm.galatNilai, "perintah tidak boleh kosong");
        return KOSONG_VAL;
    }
    for (int i = 0; i < items->count; i++) {
        if (!IS_STRING(items->values[i])) {
            nativeRaiseKelas(vm.galatTipe, "setiap bagian perintah harus teks");
            return KOSONG_VAL;
        }
        ObjString* s = AS_STRING(items->values[i]);
        if ((size_t)s->length != strlen(s->chars)) {
            nativeRaiseKelas(vm.galatNilai, "perintah tidak boleh memuat karakter NUL");
            return KOSONG_VAL;
        }
    }
    const char* program = AS_CSTRING(items->values[0]);
    ObjString* masukan = AS_STRING(args[1]);
    Bita keluaran = {NULL, 0}, galat = {NULL, 0};
    int64_t kode = -1;
    bool terlaluBesar = false;

#ifdef _WIN32
    Bita cmd = {NULL, 0};
    for (int i = 0; i < items->count; i++) {
        if (!kutipArgumen(&cmd, AS_CSTRING(items->values[i]))) {
            free(cmd.data);
            nativeRaiseKelas(vm.galatOS, "perintah terlalu panjang");
            return KOSONG_VAL;
        }
    }
    wchar_t* cmdW = keLebar(cmd.data != NULL ? cmd.data : "");
    free(cmd.data);
    HANDLE hIn = berkasSementara(), hOut = berkasSementara(), hErr = berkasSementara();
    if (cmdW == NULL || hIn == INVALID_HANDLE_VALUE || hOut == INVALID_HANDLE_VALUE ||
        hErr == INVALID_HANDLE_VALUE) {
        free(cmdW);
        if (hIn != INVALID_HANDLE_VALUE) CloseHandle(hIn);
        if (hOut != INVALID_HANDLE_VALUE) CloseHandle(hOut);
        if (hErr != INVALID_HANDLE_VALUE) CloseHandle(hErr);
        nativeRaiseKelas(vm.galatOS, "tidak dapat membuat berkas sementara untuk proses");
        return KOSONG_VAL;
    }
    DWORD tertulis;
    if (masukan->length > 0) WriteFile(hIn, masukan->chars, (DWORD)masukan->length, &tertulis, NULL);
    SetFilePointer(hIn, 0, NULL, FILE_BEGIN);

    STARTUPINFOW si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hIn;
    si.hStdOutput = hOut;
    si.hStdError = hErr;
    PROCESS_INFORMATION pi;
    fflush(stdout);
    fflush(stderr);
    BOOL dimulai = CreateProcessW(NULL, cmdW, NULL, NULL, TRUE, CREATE_NO_WINDOW,
                                  NULL, NULL, &si, &pi);
    Alasan alasan = dimulai ? ALASAN_LAIN : alasanTerakhir();
    free(cmdW);
    if (dimulai) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD k = 0;
        GetExitCodeProcess(pi.hProcess, &k);
        kode = (int64_t)k;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        terlaluBesar = !bacaHandle(hOut, &keluaran) || !bacaHandle(hErr, &galat);
    }
    CloseHandle(hIn);
    CloseHandle(hOut);
    CloseHandle(hErr);
    if (!dimulai) {
        nativeRaiseKelas(vm.galatOS, "program '%s' %s", program,
                         alasan == ALASAN_TIDAK_ADA ? "tidak ditemukan" : "tidak dapat dijalankan");
        return KOSONG_VAL;
    }
#else
    char** argv = malloc(sizeof(char*) * ((size_t)items->count + 1));
    int fdIn = berkasSementara(), fdOut = berkasSementara(), fdErr = berkasSementara();
    int lapor[2] = {-1, -1};
    if (argv == NULL || fdIn < 0 || fdOut < 0 || fdErr < 0 || pipe(lapor) != 0) {
        free(argv);
        if (fdIn >= 0) close(fdIn);
        if (fdOut >= 0) close(fdOut);
        if (fdErr >= 0) close(fdErr);
        nativeRaiseKelas(vm.galatOS, "tidak dapat menyiapkan proses");
        return KOSONG_VAL;
    }
    for (int i = 0; i < items->count; i++) argv[i] = AS_STRING(items->values[i])->chars;
    argv[items->count] = NULL;
    // The child reports a failed exec through this pipe; CLOEXEC closes it
    // on a successful exec, so the parent then reads 0 bytes.
    fcntl(lapor[1], F_SETFD, FD_CLOEXEC);
    tulisFd(fdIn, masukan->chars, (size_t)masukan->length);
    lseek(fdIn, 0, SEEK_SET);

    fflush(stdout);   // or the child inherits our unflushed buffer and prints it twice
    fflush(stderr);
    pid_t pid = fork();
    if (pid == 0) {
        close(lapor[0]);
        dup2(fdIn, 0);
        dup2(fdOut, 1);
        dup2(fdErr, 2);
        execvp(argv[0], argv);
        int e = errno;
        tulisFd(lapor[1], (const char*)&e, sizeof(e));
        _exit(127);
    }
    close(lapor[1]);
    free(argv);
    int errnoAnak = 0;
    ssize_t n = 0;
    if (pid > 0) {
        do {
            n = read(lapor[0], &errnoAnak, sizeof(errnoAnak));
        } while (n < 0 && errno == EINTR);
        int status = 0;
        while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
        if (WIFEXITED(status)) kode = WEXITSTATUS(status);
        else if (WIFSIGNALED(status)) kode = -(int64_t)WTERMSIG(status);   // like Python
    }
    close(lapor[0]);
    if (pid > 0 && n != (ssize_t)sizeof(errnoAnak)) {
        terlaluBesar = !bacaFd(fdOut, &keluaran) || !bacaFd(fdErr, &galat);
    }
    close(fdIn);
    close(fdOut);
    close(fdErr);
    if (pid < 0 || n == (ssize_t)sizeof(errnoAnak)) {
        free(keluaran.data);
        free(galat.data);
        errno = errnoAnak;
        nativeRaiseKelas(vm.galatOS, "program '%s' %s", program,
                         pid > 0 && errnoAnak == ENOENT ? "tidak ditemukan" : "tidak dapat dijalankan");
        return KOSONG_VAL;
    }
#endif
    if (terlaluBesar) {
        free(keluaran.data);
        free(galat.data);
        nativeRaiseKelas(vm.galatOS, "keluaran proses melebihi %u MB", KELUARAN_MAKS / (1024u * 1024u));
        return KOSONG_VAL;
    }
    Value hasil = hasilProses(kode, &keluaran, &galat);
    free(keluaran.data);
    free(galat.data);
    return hasil;
}
