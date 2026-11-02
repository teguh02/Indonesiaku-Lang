// usleep() is POSIX, not C11: glibc hides it under -std=c11 unless a
// feature-test macro is set before the first system header (GCC 14 treats
// the implicit declaration as an error).
#if !defined(_WIN32)
#define _DEFAULT_SOURCE
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#ifdef _WIN32
    #include <winsock2.h>   // sebelum windows.h, atau MinGW memperingatkan
    #include <ws2tcpip.h>
    #include <windows.h>
    typedef SOCKET indk_socket_t;
    #define INDK_INVALID_SOCKET INVALID_SOCKET
    #define INDK_CLOSESOCKET(s) closesocket(s)
#else
    #include <unistd.h>
    #include <sys/socket.h>
    // struct timeval dan suseconds_t (dipakai soket_batas_waktu) berasal dari
    // sini. glibc menariknya diam-diam lewat header lain, musl TIDAK: tanpa
    // baris ini build di Alpine gagal dengan "'suseconds_t' undeclared".
    #include <sys/time.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    typedef int indk_socket_t;
    #define INDK_INVALID_SOCKET (-1)
    #define INDK_CLOSESOCKET(s) close(s)
#endif

#include "native.h"
#include "tls.h"
#include "sistem.h"
#include "vm.h"
#include "object.h"
#include "memory.h"
#include "common.h"

// ============================================================================
// FUNGSI MATEMATIKA - IMPLEMENTASI
// ============================================================================

// Keduanya menerima bentuk varargs LAMA (min(3, 1, 2)) dan satu argumen
// urutan (min([3, 1, 2])), seperti Python. Lihat minMax() di bawah.
static Value minMax(int argCount, Value* args, bool wantMax, const char* nama);

Value nativeFnMin(int argCount, Value* args) {
    return minMax(argCount, args, false, "min");
}

Value nativeFnMax(int argCount, Value* args) {
    return minMax(argCount, args, true, "max");
}

Value nativeFnAbs(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("abs() memerlukan 1 argumen");
        return NUMBER_VAL(0);
    }
    if (!IS_NUMBER(args[0])) {
        nativeRaise("abs() hanya menerima angka");
        return NUMBER_VAL(0);
    }
    if (IS_INT(args[0])) {
        int64_t n = AS_INT(args[0]);
        if (n == INT64_MIN) {
            nativeRaiseKelas(vm.galatLuapan, "abs() melebihi jangkauan bilangan bulat");
            return NUMBER_VAL(0);
        }
        return INT_VAL(n < 0 ? -n : n);       // abs() of a bulat stays bulat
    }
    return NUMBER_VAL(fabs(AS_NUMBER(args[0])));
}

Value nativeFnAkar(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("akar() memerlukan 1 argumen");
        return NUMBER_VAL(0);
    }
    if (!IS_NUMBER(args[0])) {
        nativeRaise("akar() hanya menerima angka");
        return NUMBER_VAL(0);
    }
    double num = AS_NUMBER(args[0]);
    if (num < 0) {
        nativeRaise("akar() tidak dapat menerima angka negatif");
        return NUMBER_VAL(0);
    }
    return NUMBER_VAL(sqrt(num));
}

// Trigonometri (Fase 6): sin cos tan asin acos atan atan2, radian, derajat.
// Satuan sudut radian, seperti modul math Python. Hasil selalu desimal.
typedef double (*FungsiSatu)(double);
typedef enum { DOMAIN_SEMUA, DOMAIN_HINGGA, DOMAIN_SATU } DomainTrig;

#define INDK_PI 3.14159265358979323846

static double keRadian(double x) { return x * (INDK_PI / 180.0); }
static double keDerajat(double x) { return x * (180.0 / INDK_PI); }

// DOMAIN_HINGGA: sin/cos/tan menolak tak hingga; DOMAIN_SATU: asin/acos
// hanya [-1, 1]. Python melempar ValueError untuk keduanya.
static Value trigSatu(const char* nama, FungsiSatu f, DomainTrig domain,
                      int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        nativeRaiseKelas(vm.galatTipe, "%s() memerlukan 1 angka", nama);
        return KOSONG_VAL;
    }
    double x = AS_NUMBER(args[0]);
    if (domain == DOMAIN_SATU && (x < -1.0 || x > 1.0)) {
        nativeRaiseKelas(vm.galatNilai, "%s(): argumen di luar domain [-1, 1]", nama);
        return KOSONG_VAL;
    }
    if (domain == DOMAIN_HINGGA && isinf(x)) {
        nativeRaiseKelas(vm.galatNilai, "%s(): argumen tak hingga", nama);
        return KOSONG_VAL;
    }
    return NUMBER_VAL(f(x));
}

Value nativeFnSin(int argCount, Value* args) { return trigSatu("sin", sin, DOMAIN_HINGGA, argCount, args); }
Value nativeFnCos(int argCount, Value* args) { return trigSatu("cos", cos, DOMAIN_HINGGA, argCount, args); }
Value nativeFnTan(int argCount, Value* args) { return trigSatu("tan", tan, DOMAIN_HINGGA, argCount, args); }
Value nativeFnAsin(int argCount, Value* args) { return trigSatu("asin", asin, DOMAIN_SATU, argCount, args); }
Value nativeFnAcos(int argCount, Value* args) { return trigSatu("acos", acos, DOMAIN_SATU, argCount, args); }
Value nativeFnAtan(int argCount, Value* args) { return trigSatu("atan", atan, DOMAIN_SEMUA, argCount, args); }
Value nativeFnRadian(int argCount, Value* args) { return trigSatu("radian", keRadian, DOMAIN_SEMUA, argCount, args); }
Value nativeFnDerajat(int argCount, Value* args) { return trigSatu("derajat", keDerajat, DOMAIN_SEMUA, argCount, args); }

Value nativeFnAtan2(int argCount, Value* args) {
    if (argCount != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        nativeRaiseKelas(vm.galatTipe, "atan2(y, x) memerlukan 2 angka");
        return KOSONG_VAL;
    }
    return NUMBER_VAL(atan2(AS_NUMBER(args[0]), AS_NUMBER(args[1])));
}

Value nativeFnPangkat(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaise("pangkat() memerlukan 2 argumen");
        return NUMBER_VAL(0);
    }
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        nativeRaise("pangkat() hanya menerima angka");
        return NUMBER_VAL(0);
    }
    // pangkat() mirrors the '**' operator: bulat ** bulat >= 0 stays bulat.
    if (IS_INT(args[0]) && IS_INT(args[1]) && AS_INT(args[1]) >= 0) {
        bool overflow;
        int64_t hasil = intPow(AS_INT(args[0]), AS_INT(args[1]), &overflow);
        if (overflow) {
            nativeRaiseKelas(vm.galatLuapan, "pangkat() melebihi jangkauan bilangan bulat");
            return NUMBER_VAL(0);
        }
        return INT_VAL(hasil);
    }
    return NUMBER_VAL(pow(AS_NUMBER(args[0]), AS_NUMBER(args[1])));
}

Value nativeFnBulat(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("bulat() memerlukan 1 argumen");
        return NUMBER_VAL(0);
    }
    if (!IS_NUMBER(args[0])) {
        nativeRaise("bulat() hanya menerima angka");
        return NUMBER_VAL(0);
    }
    // Python's round() returns an int.
    double hasil = round(AS_NUMBER(args[0]));
    if (hasil >= 9223372036854775808.0 || hasil < -9223372036854775808.0) {
        nativeRaiseKelas(vm.galatLuapan, "bulat() melebihi jangkauan bilangan bulat");
        return NUMBER_VAL(0);
    }
    return INT_VAL((int64_t)hasil);
}

Value nativeFnLantai(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("lantai() memerlukan 1 argumen");
        return NUMBER_VAL(0);
    }
    if (!IS_NUMBER(args[0])) {
        nativeRaise("lantai() hanya menerima angka");
        return NUMBER_VAL(0);
    }
    // math.floor() returns an int in Python.
    double hasil = floor(AS_NUMBER(args[0]));
    if (hasil >= 9223372036854775808.0 || hasil < -9223372036854775808.0) {
        nativeRaiseKelas(vm.galatLuapan, "lantai() melebihi jangkauan bilangan bulat");
        return NUMBER_VAL(0);
    }
    return INT_VAL((int64_t)hasil);
}

Value nativeFnAtap(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("atap() memerlukan 1 argumen");
        return NUMBER_VAL(0);
    }
    if (!IS_NUMBER(args[0])) {
        nativeRaise("atap() hanya menerima angka");
        return NUMBER_VAL(0);
    }
    // math.ceil() returns an int in Python.
    double hasil = ceil(AS_NUMBER(args[0]));
    if (hasil >= 9223372036854775808.0 || hasil < -9223372036854775808.0) {
        nativeRaiseKelas(vm.galatLuapan, "atap() melebihi jangkauan bilangan bulat");
        return NUMBER_VAL(0);
    }
    return INT_VAL((int64_t)hasil);
}

Value nativeFnAcak(int argCount, Value* args) {
    if (argCount != 0) {
        nativeRaise("acak() tidak memerlukan argumen");
        return NUMBER_VAL(0);
    }
    // Return random number between 0 and 1
    return NUMBER_VAL((double)rand() / RAND_MAX);
}

// ============================================================================
// ANGKA & TIPE (padanan round/divmod/bin/oct/hex/int/bool/isinstance/...)
// ============================================================================

// Baca satu elemen dari urutan yang bisa diiterasi (daftar, rentang, teks).
// Mengembalikan false bila tipenya bukan urutan.
static bool seqLength(Value seq, int* length) {
    if (sequenceItems(seq) != NULL) *length = sequenceItems(seq)->count;
    else if (IS_SET(seq)) *length = AS_SET(seq)->keys.count;
    else if (IS_RANGE(seq)) *length = (int)rangeLength(AS_RANGE(seq));
    else if (IS_STRING(seq)) *length = AS_STRING(seq)->charCount;
    else return false;
    return true;
}

// Alokasi: untuk teks ini membuat ObjString baru, jadi pemanggil wajib
// menjaga nilai lain yang dipegangnya tetap tereferensi dari stack VM.
static Value seqGet(Value seq, int i) {
    if (sequenceItems(seq) != NULL) return sequenceItems(seq)->values[i];
    if (IS_SET(seq)) return AS_SET(seq)->keys.values[i];
    if (IS_RANGE(seq)) return rangeElement(AS_RANGE(seq), i);
    ObjString* teks = AS_STRING(seq);
    int offset = utf8ByteOffset(teks, i);
    return OBJ_VAL(copyString(teks->chars + offset, utf8CharBytes(teks, offset)));
}

// Perbandingan '<' seperti operator bahasa: angka dengan angka, teks dengan
// teks (strcmp). Campuran tipe adalah GalatTipe.
static bool valueLess(Value a, Value b, const char* nama, bool* less) {
    if (IS_NUMBER(a) && IS_NUMBER(b)) {
        *less = AS_NUMBER(a) < AS_NUMBER(b);
        return true;
    }
    if (IS_STRING(a) && IS_STRING(b)) {
        *less = strcmp(AS_CSTRING(a), AS_CSTRING(b)) < 0;
        return true;
    }
    if (IS_INSTANCE(a) || IS_INSTANCE(b)) {
        // K19: let the class order itself with _kurang_dari_ (or the mirror
        // _lebih_dari_). 0 means the method raised and already holds the error.
        int found = instanceLessFor(a, b, less);
        if (found == 1) return true;
        if (found == 0) return false;
        nativeRaiseKelas(vm.galatTipe,
                         "%s() tidak dapat membandingkan objek tanpa metode "
                         "_kurang_dari_", nama);
        return false;
    }
    nativeRaiseKelas(vm.galatTipe, "%s() tidak dapat membandingkan tipe yang berbeda", nama);
    return false;
}

// Inti max()/min(): bentuk varargs ATAU satu argumen urutan (daftar, rentang,
// teks), seperti Python.
static Value minMax(int argCount, Value* args, bool wantMax, const char* nama) {
    Value seq;
    int count;
    bool fromSeq;

    // Pesan "minimal 2 argumen" dipertahankan apa adanya: golden
    // kesalahan_native menegaskan teks ini untuk min(5).
    if (argCount == 0) {
        nativeRaiseKelas(vm.galatTipe, "%s() memerlukan minimal 2 argumen", nama);
        return NUMBER_VAL(0);
    }
    if (argCount == 1) {
        if (!seqLength(args[0], &count)) {
            nativeRaiseKelas(vm.galatTipe, "%s() memerlukan minimal 2 argumen", nama);
            return NUMBER_VAL(0);
        }
        if (count == 0) {
            nativeRaiseKelas(vm.galatNilai, "%s() dari urutan kosong", nama);
            return NUMBER_VAL(0);
        }
        seq = args[0];
        fromSeq = true;
    } else {
        count = argCount;
        seq = KOSONG_VAL;
        fromSeq = false;
    }

    // Kandidat terbaik disimpan di stack VM: seqGet() dapat mengalokasi
    // (teks) dan memicu GC saat kandidat sebelumnya belum tereferensi.
    push(fromSeq ? seqGet(seq, 0) : args[0]);
    for (int i = 1; i < count; i++) {
        Value item = fromSeq ? seqGet(seq, i) : args[i];
        push(item);
        bool less;
        if (!valueLess(vm.stackTop[-1], vm.stackTop[-2], nama, &less)) {
            vm.stackTop -= 2;
            return NUMBER_VAL(0);
        }
        if (less != wantMax) {       // item lebih baik dari kandidat
            Value better = pop();
            pop();
            push(better);
        } else {
            pop();
        }
    }
    return pop();
}

// bulatkan(x[, digit]) - round() Python: pembulatan setengah ke genap.
// Dikerjakan lewat printf("%.*f") karena glibc membulatkan dari nilai biner
// yang SEBENARNYA dengan ties-to-even — persis algoritma _Py_dg_dtoa CPython.
// Karena itu bulatkan(2.675, 2) == 2.67, sama seperti Python (2.675 sejatinya
// 2.67499...): ini bukan bug, jangan "diperbaiki".
Value nativeFnBulatkan(int argCount, Value* args) {
    if (argCount < 1 || argCount > 2) {
        nativeRaiseKelas(vm.galatTipe, "bulatkan() memerlukan 1 atau 2 argumen");
        return NUMBER_VAL(0);
    }
    if (!IS_NUMBER(args[0])) {
        nativeRaiseKelas(vm.galatTipe, "bulatkan() hanya menerima angka");
        return NUMBER_VAL(0);
    }
    double x = AS_NUMBER(args[0]);
    int digit = 0;
    if (argCount == 2) {
        if (!IS_NUMBER(args[1]) || AS_NUMBER(args[1]) != floor(AS_NUMBER(args[1]))) {
            nativeRaiseKelas(vm.galatTipe, "bulatkan() digit harus bilangan bulat");
            return NUMBER_VAL(0);
        }
        double d = AS_NUMBER(args[1]);
        if (d > 320) return NUMBER_VAL(x);      // sudah lebih presisi dari double
        if (d < -320) return NUMBER_VAL(x < 0 ? -0.0 : 0.0);
        digit = (int)d;
    }
    if (!isfinite(x)) return NUMBER_VAL(x);

    // Python's round(x) with no digit returns an int (banker's rounding).
    if (digit == 0) {
        double hasil = nearbyint(x);
        if (hasil >= 9223372036854775808.0 || hasil < -9223372036854775808.0) {
            nativeRaiseKelas(vm.galatLuapan, "bulatkan() melebihi jangkauan bilangan bulat");
            return NUMBER_VAL(0);
        }
        return INT_VAL((int64_t)hasil);
    }

    if (digit < 0) {
        // printf tidak punya presisi negatif: skala, bulatkan setengah-ke-genap
        // (mode pembulatan default FPU), lalu kembalikan skalanya.
        double scale = pow(10.0, -digit);
        return NUMBER_VAL(nearbyint(x / scale) * scale);
    }
    if (fabs(x) >= 1e17) return NUMBER_VAL(x);  // sudah bulat pada presisi itu

    char buf[512];
    snprintf(buf, sizeof(buf), "%.*f", digit, x);
    return NUMBER_VAL(strtod(buf, NULL));
}

// bagi_sisa(a, b) - divmod(): [a // b, a % b], konsisten dengan
// OP_FLOOR_DIVIDE dan OP_MODULO di vm.c (lantai + sisa bertanda pembagi).
Value nativeFnBagiSisa(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaiseKelas(vm.galatTipe, "bagi_sisa() memerlukan 2 argumen");
        return KOSONG_VAL;
    }
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        nativeRaiseKelas(vm.galatTipe, "bagi_sisa() hanya menerima angka");
        return KOSONG_VAL;
    }
    if (IS_INT(args[0]) && IS_INT(args[1])) {
        int64_t a = AS_INT(args[0]);
        int64_t b = AS_INT(args[1]);
        if (b == 0) {
            nativeRaiseKelas(vm.galatBagiNol, "Pembagian dengan nol.");
            return KOSONG_VAL;
        }
        int64_t hasilBagi, hasilSisa;
        if (!intFloorDiv(a, b, &hasilBagi) || !intModulo(a, b, &hasilSisa)) {
            nativeRaiseKelas(vm.galatLuapan, "bagi_sisa() melebihi jangkauan bilangan bulat");
            return KOSONG_VAL;
        }
        ObjList* hasil = newList();
        push(OBJ_VAL(hasil));
        writeValueArray(&hasil->items, INT_VAL(hasilBagi));
        writeValueArray(&hasil->items, INT_VAL(hasilSisa));
        pop();
        return OBJ_VAL(hasil);
    }

    double a = AS_NUMBER(args[0]);
    double b = AS_NUMBER(args[1]);
    if (b == 0) {
        nativeRaiseKelas(vm.galatBagiNol, "Pembagian dengan nol.");
        return KOSONG_VAL;
    }
    double sisa = fmod(a, b);
    if (sisa == 0) {
        sisa = 0;                                // hindari "-0"
    } else if ((sisa < 0) != (b < 0)) {
        sisa += b;
    }

    ObjList* hasil = newList();
    push(OBJ_VAL(hasil));                        // jaga dari GC saat tumbuh
    writeValueArray(&hasil->items, NUMBER_VAL(floor(a / b)));
    writeValueArray(&hasil->items, NUMBER_VAL(sisa));
    pop();
    return OBJ_VAL(hasil);
}

// Inti biner()/oktal()/heksa(): teks berawalan, tanda minus di depan awalan
// seperti Python ('-0b1010').
static Value toBase(int argCount, Value* args, int base, const char* prefix,
                    const char* nama) {
    if (argCount != 1) {
        nativeRaiseKelas(vm.galatTipe, "%s() memerlukan 1 argumen", nama);
        return KOSONG_VAL;
    }
    if (!IS_NUMBER(args[0])) {
        nativeRaiseKelas(vm.galatTipe, "%s() hanya menerima angka", nama);
        return KOSONG_VAL;
    }
    int64_t bulat;
    if (IS_INT(args[0])) {
        bulat = AS_INT(args[0]);
    } else {
        double x = AS_NUMBER(args[0]);
        if (!isfinite(x) || x != floor(x)) {
            nativeRaiseKelas(vm.galatTipe, "%s() memerlukan bilangan bulat", nama);
            return KOSONG_VAL;
        }
        if (x >= 9223372036854775808.0 || x < -9223372036854775808.0) {
            nativeRaiseKelas(vm.galatNilai, "%s() angka terlalu besar", nama);
            return KOSONG_VAL;
        }
        bulat = (int64_t)x;
    }

    bool negatif = bulat < 0;
    // Two's-complement magnitude, so INT64_MIN converts without UB.
    unsigned long long n = negatif ? (unsigned long long)(-(bulat + 1)) + 1
                                   : (unsigned long long)bulat;
    char digits[72];
    int len = 0;
    do {
        digits[len++] = "0123456789abcdef"[n % (unsigned)base];
        n /= (unsigned)base;
    } while (n > 0);

    char buf[80];
    int pos = 0;
    if (negatif) buf[pos++] = '-';
    buf[pos++] = prefix[0];
    buf[pos++] = prefix[1];
    while (len > 0) buf[pos++] = digits[--len];
    return OBJ_VAL(copyString(buf, pos));
}

Value nativeFnBiner(int argCount, Value* args)  { return toBase(argCount, args, 2,  "0b", "biner"); }
Value nativeFnOktal(int argCount, Value* args)  { return toBase(argCount, args, 8,  "0o", "oktal"); }
Value nativeFnHeksa(int argCount, Value* args)  { return toBase(argCount, args, 16, "0x", "heksa"); }

// ke_bulat(x) - int(): memotong KE ARAH NOL (bukan lantai), dan menerima teks.
Value nativeFnKeBulat(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaiseKelas(vm.galatTipe, "ke_bulat() memerlukan 1 argumen");
        return NUMBER_VAL(0);
    }
    if (IS_BOOL(args[0])) return INT_VAL(AS_BOOL(args[0]) ? 1 : 0);
    if (IS_INT(args[0])) return args[0];
    if (IS_NUMBER(args[0])) {
        double x = AS_NUMBER(args[0]);
        if (!isfinite(x)) {
            nativeRaiseKelas(vm.galatNilai, "ke_bulat() tidak dapat mengubah %s",
                             isnan(x) ? "nan" : "takhingga");
            return NUMBER_VAL(0);
        }
        // Truncates toward zero, like Python's int().
        if (trunc(x) > 9223372036854775807.0 || trunc(x) < -9223372036854775808.0) {
            nativeRaiseKelas(vm.galatLuapan, "ke_bulat() melebihi jangkauan bilangan bulat");
            return NUMBER_VAL(0);
        }
        return INT_VAL((int64_t)trunc(x));
    }
    if (!IS_STRING(args[0])) {
        nativeRaiseKelas(vm.galatTipe, "ke_bulat() hanya menerima angka, teks, atau logika");
        return NUMBER_VAL(0);
    }

    // Seperti int("..."): spasi di tepi boleh, pecahan tidak.
    const char* s = AS_CSTRING(args[0]);
    const char* p = s;
    while (isspace((unsigned char)*p)) p++;
    const char* mulai = p;
    if (*p == '+' || *p == '-') p++;
    if (!isdigit((unsigned char)*p)) {
        nativeRaiseKelas(vm.galatNilai, "ke_bulat() gagal: '%s' bukan bilangan bulat", s);
        return NUMBER_VAL(0);
    }
    while (isdigit((unsigned char)*p)) p++;
    const char* selesai = p;
    while (isspace((unsigned char)*p)) p++;
    if (*p != '\0') {
        nativeRaiseKelas(vm.galatNilai, "ke_bulat() gagal: '%s' bukan bilangan bulat", s);
        return NUMBER_VAL(0);
    }
    (void)selesai;
    errno = 0;
    long long nilai = strtoll(mulai, NULL, 10);
    if (errno == ERANGE) {
        nativeRaiseKelas(vm.galatLuapan, "ke_bulat() melebihi jangkauan bilangan bulat");
        return NUMBER_VAL(0);
    }
    return INT_VAL(nilai);
}

// ke_logika(x) - bool(): memakai truthiness K1 yang sama dengan jika/tidak.
Value nativeFnKeLogika(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaiseKelas(vm.galatTipe, "ke_logika() memerlukan 1 argumen");
        return BOOL_VAL(false);
    }
    return BOOL_VAL(!isFalsey(args[0]));
}

// adalah_instansi(objek, kelas) - isinstance(). Memakai valueIsInstanceOf()
// di vm.c, helper yang sama yang dipakai OP_IS_INSTANCE untuk `kecuali`.
Value nativeFnAdalahInstansi(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaiseKelas(vm.galatTipe, "adalah_instansi() memerlukan 2 argumen");
        return BOOL_VAL(false);
    }
    if (!IS_CLASS(args[1])) {
        nativeRaiseKelas(vm.galatTipe, "adalah_instansi() argumen kedua harus kelas");
        return BOOL_VAL(false);
    }
    return BOOL_VAL(valueIsInstanceOf(args[0], args[1]));
}

// bisa_dipanggil(x) - callable().
Value nativeFnBisaDipanggil(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaiseKelas(vm.galatTipe, "bisa_dipanggil() memerlukan 1 argumen");
        return BOOL_VAL(false);
    }
    Value v = args[0];
    return BOOL_VAL(IS_FUNCTION(v) || IS_CLOSURE(v) || IS_NATIVE(v) ||
                    IS_CLASS(v) || IS_BOUND_METHOD(v));
}

// repr(x) - repr(): teks dikutip, instansi memakai _repr_ (K19).
static bool valueText(Value v, bool repr, Value* out);

Value nativeFnRepr(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaiseKelas(vm.galatTipe, "repr() memerlukan 1 argumen");
        return KOSONG_VAL;
    }
    Value text;
    if (!valueText(args[0], true, &text)) return KOSONG_VAL;
    return text;
}

// id(x) - identitas objek, konsisten dengan operator `adalah`
// (valuesIdentical): objek memakai alamat heap-nya, primitif memakai hash
// 53-bit dari nilainya sehingga dua primitif yang identik selalu seid.
// ponytail: hash primitif bisa (sangat kecil kemungkinannya) bertabrakan
// dengan sebuah alamat; cukup sampai ada tipe id yang bukan double.
Value nativeFnId(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaiseKelas(vm.galatTipe, "id() memerlukan 1 argumen");
        return NUMBER_VAL(0);
    }
    Value v = args[0];
    if (IS_OBJ(v)) return INT_VAL((int64_t)(uintptr_t)AS_OBJ(v));

    uint64_t hash = 1469598103934665603ULL;      // FNV-1a
    unsigned char bytes[9];
    bytes[0] = (unsigned char)v.type;
    double d = 0;
    if (IS_BOOL(v)) d = AS_BOOL(v) ? 1 : 0;
    else if (IS_NUMBER(v)) d = AS_NUMBER(v) + 0.0;   // -0.0 dan 0.0 identik
    memcpy(bytes + 1, &d, sizeof(double));
    for (size_t i = 0; i < sizeof(bytes); i++) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return INT_VAL((int64_t)(hash & 0x1FFFFFFFFFFFFFULL));   // 53 bit: eksak di double
}

// ============================================================================
// FUNGSI STRING - IMPLEMENTASI
// ============================================================================

// panjang_byte(teks) -> jumlah BYTE, bukan karakter.
// panjang() mengembalikan charCount (jumlah titik kode). Content-Length HTTP
// dan soket_tulis() bekerja dalam byte, dan untuk badan biner keduanya tidak
// sama: menghitung ulang lebar UTF-8 per karakter di level .idk salah dua kali
// (byte non-UTF-8 hilang dari iterasi, byte >= 0x80 dihitung dua kali).
Value nativeFnPanjangByte(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("panjang_byte() memerlukan 1 argumen (teks)");
        return NUMBER_VAL(0);
    }
    if (!IS_STRING(args[0])) {
        nativeRaise("panjang_byte() hanya menerima teks");
        return NUMBER_VAL(0);
    }
    return INT_VAL(AS_STRING(args[0])->length);
}

Value nativeFnPanjang(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("panjang() memerlukan 1 argumen");
        return NUMBER_VAL(0);
    }
    if (IS_STRING(args[0])) {
        return INT_VAL(AS_STRING(args[0])->charCount);
    }
    if (IS_SET(args[0])) {
        return INT_VAL(AS_SET(args[0])->keys.count);
    }
    if (sequenceItems(args[0]) != NULL) {          // daftar or tupel
        return INT_VAL(sequenceItems(args[0])->count);
    }
    if (IS_DICT(args[0])) {
        return INT_VAL(dictLength(AS_DICT(args[0])));
    }
    if (IS_RANGE(args[0])) {
        return INT_VAL((int64_t)rangeLength(AS_RANGE(args[0])));
    }
    {
        // K19 tahap 2: a class with _panjang_ reports its own length.
        int64_t length;
        int found = instanceLengthFor(args[0], &length);
        if (found == 1) return INT_VAL(length);
        if (found == 0) return NUMBER_VAL(0);   // raised; error already pending
    }
    nativeRaise("panjang() hanya menerima string, list, kamus, rentang, "
                "atau objek dengan _panjang_()");
    return NUMBER_VAL(0);
}

// tupel is immutable: the daftar-mutating builtins refuse it with the same
// message as t[i] = x in the VM, instead of a confusing "harus list".
static bool rejectTuple(Value value) {
    if (!IS_TUPLE(value)) return false;
    nativeRaiseKelas(vm.galatTipe, "Tupel tidak dapat diubah.");
    return true;
}

// Every himpunan write path routes through here: tambah() and hapus() are the
// only two, so one guard covers them both.
static bool rejectFrozenSet(Value value) {
    if (!IS_SET(value) || !AS_SET(value)->frozen) return false;
    nativeRaiseKelas(vm.galatTipe, "Himpunan beku tidak dapat diubah.");
    return true;
}

Value nativeFnTambah(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaise("tambah() memerlukan 2 argumen (list, nilai)");
        return KOSONG_VAL;
    }
    if (rejectTuple(args[0])) return KOSONG_VAL;
    if (rejectFrozenSet(args[0])) return KOSONG_VAL;
    if (IS_SET(args[0])) {   // tambah(himpunan, x): already a member is fine
        if (!setAdd(AS_SET(args[0]), args[1])) {
            nativeRaiseKelas(vm.galatTipe, "Anggota himpunan harus nilai yang dapat di-hash.");
        }
        return args[0];
    }
    if (!IS_LIST(args[0])) {
        nativeRaise("tambah() argumen pertama harus list");
        return KOSONG_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    writeValueArray(&list->items, args[1]);
    return args[0];
}

Value nativeFnHapus(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("hapus() memerlukan 1 argumen (list)");
        return KOSONG_VAL;
    }
    if (rejectTuple(args[0])) return KOSONG_VAL;
    if (!IS_LIST(args[0])) {
        nativeRaise("hapus() hanya menerima list");
        return KOSONG_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->items.count == 0) {
        nativeRaise("hapus() pada list kosong");
        return KOSONG_VAL;
    }
    Value last = list->items.values[list->items.count - 1];
    list->items.count--;
    return last;
}

// ============================================================================
// FUNGSI KAMUS (DICTIONARY) - IMPLEMENTASI
// ============================================================================

Value nativeFnKamus(int argCount, Value* args) {
    (void)args;
    if (argCount != 0) {
        nativeRaise("kamus() tidak menerima argumen");
        return KOSONG_VAL;
    }
    return OBJ_VAL(newDict());
}

Value nativeFnPunya(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaise("punya() memerlukan 2 argumen (kamus, kunci)");
        return BOOL_VAL(false);
    }
    if (!IS_DICT(args[0])) {
        nativeRaise("punya() memerlukan (kamus, kunci)");
        return BOOL_VAL(false);
    }
    Value tmp;
    return BOOL_VAL(valueTableGet(&AS_DICT(args[0])->table, args[1], &tmp));
}

Value nativeFnHapusKunci(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaise("hapus_kunci() memerlukan 2 argumen (kamus, kunci)");
        return BOOL_VAL(false);
    }
    if (!IS_DICT(args[0])) {
        nativeRaise("hapus_kunci() memerlukan (kamus, kunci)");
        return BOOL_VAL(false);
    }
    return BOOL_VAL(dictDelete(AS_DICT(args[0]), args[1]));
}

// enumerasi(urutan[, awal]) - pasangan [indeks, nilai] untuk iterable dasar
Value nativeFnEnumerasi(int argCount, Value* args) {
    if (argCount < 1 || argCount > 2) {
        nativeRaise("enumerasi() memerlukan 1 atau 2 argumen");
        return KOSONG_VAL;
    }
    if (argCount == 2 && !IS_NUMBER(args[1])) {
        nativeRaise("enumerasi() argumen awal harus angka");
        return KOSONG_VAL;
    }

    Value urutan = args[0];
    int count;
    if (sequenceItems(urutan) != NULL) count = sequenceItems(urutan)->count;
    else if (IS_SET(urutan)) count = AS_SET(urutan)->keys.count;
    else if (IS_STRING(urutan)) count = AS_STRING(urutan)->charCount;
    else if (IS_RANGE(urutan)) count = (int)rangeLength(AS_RANGE(urutan));
    else if (IS_DICT(urutan)) count = dictLength(AS_DICT(urutan));
    else {
        nativeRaise("enumerasi() hanya menerima daftar, tupel, himpunan, teks, rentang, atau kamus");
        return KOSONG_VAL;
    }

    if (argCount == 2 && AS_NUMBER(args[1]) != floor(AS_NUMBER(args[1]))) {
        nativeRaise("enumerasi() argumen awal harus bilangan bulat");
        return KOSONG_VAL;
    }
    int64_t awal = argCount == 2 ? (int64_t)AS_NUMBER(args[1]) : 0;
    ObjList* hasil = newList();
    push(OBJ_VAL(hasil));
    for (int i = 0; i < count; i++) {
        ObjList* pasangan = newList();
        push(OBJ_VAL(pasangan));
        Value nilai;
        if (sequenceItems(urutan) != NULL) nilai = sequenceItems(urutan)->values[i];
        else if (IS_SET(urutan)) nilai = AS_SET(urutan)->keys.values[i];
        else if (IS_STRING(urutan)) {
            ObjString* teks = AS_STRING(urutan);
            int offset = utf8ByteOffset(teks, i);
            nilai = OBJ_VAL(copyString(teks->chars + offset, utf8CharBytes(teks, offset)));
        } else if (IS_RANGE(urutan)) {
            nilai = rangeElement(AS_RANGE(urutan), i);
        } else nilai = dictKeyAt(AS_DICT(urutan), i);

        push(nilai);
        writeValueArray(&pasangan->items, INT_VAL(awal + i));
        writeValueArray(&pasangan->items, nilai);
        pop();
        writeValueArray(&hasil->items, OBJ_VAL(pasangan));
        pop();
    }
    pop();
    return OBJ_VAL(hasil);
}

Value nativeFnKunci(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("kunci() memerlukan 1 argumen (kamus)");
        return KOSONG_VAL;
    }
    if (!IS_DICT(args[0])) {
        nativeRaise("kunci() hanya menerima kamus");
        return KOSONG_VAL;
    }
    ObjDict* dict = AS_DICT(args[0]);
    ObjList* keys = newList();
    // Protect the new list from GC while we populate it.
    push(OBJ_VAL(keys));
    for (int i = 0; i < dictLength(dict); i++) {
        writeValueArray(&keys->items, dictKeyAt(dict, i));
    }
    pop();
    return OBJ_VAL(keys);
}

Value nativeFnHurofBesar(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("huruf_besar() memerlukan 1 argumen");
        return KOSONG_VAL;
    }
    if (!IS_STRING(args[0])) {
        nativeRaise("huruf_besar() hanya menerima string");
        return KOSONG_VAL;
    }

    ObjString* original = AS_STRING(args[0]);
    char* result = ALLOCATE(char, original->length + 1);
    
    for (int i = 0; i < original->length; i++) {
        result[i] = toupper(original->chars[i]);
    }
    result[original->length] = '\0';

    ObjString* str = takeString(result, original->length);
    return OBJ_VAL(str);
}

Value nativeFnHurofKecil(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("huruf_kecil() memerlukan 1 argumen");
        return KOSONG_VAL;
    }
    if (!IS_STRING(args[0])) {
        nativeRaise("huruf_kecil() hanya menerima string");
        return KOSONG_VAL;
    }

    ObjString* original = AS_STRING(args[0]);
    char* result = ALLOCATE(char, original->length + 1);
    
    for (int i = 0; i < original->length; i++) {
        result[i] = tolower(original->chars[i]);
    }
    result[original->length] = '\0';

    ObjString* str = takeString(result, original->length);
    return OBJ_VAL(str);
}

Value nativeFnGanti(int argCount, Value* args) {
    if (argCount != 3) {
        nativeRaise("ganti() memerlukan 3 argumen");
        return KOSONG_VAL;
    }
    if (!IS_STRING(args[0]) || !IS_STRING(args[1]) || !IS_STRING(args[2])) {
        nativeRaise("ganti() hanya menerima string");
        return KOSONG_VAL;
    }

    ObjString* text = AS_STRING(args[0]);
    ObjString* cari = AS_STRING(args[1]);
    ObjString* ganti = AS_STRING(args[2]);

    if (cari->length == 0) {
        return OBJ_VAL(text);
    }

    // Hitung jumlah occurrence
    int count = 0;
    const char* p = text->chars;
    while ((p = strstr(p, cari->chars)) != NULL) {
        count++;
        p += cari->length;
    }

    if (count == 0) {
        return OBJ_VAL(text);
    }

    // Alokasi memory untuk hasil
    int newLength = text->length + count * (ganti->length - cari->length);
    char* result = ALLOCATE(char, newLength + 1);
    
    // Replace
    const char* src = text->chars;
    char* dst = result;
    while (*src) {
        if (strncmp(src, cari->chars, cari->length) == 0) {
            memcpy(dst, ganti->chars, ganti->length);
            dst += ganti->length;
            src += cari->length;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';

    ObjString* str = takeString(result, newLength);
    return OBJ_VAL(str);
}

// ============================================================================
// FUNGSI UTILITAS - IMPLEMENTASI
// ============================================================================

Value nativeFnJenis(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("jenis() memerlukan 1 argumen");
        return OBJ_VAL(copyString("kosong", 6));
    }

    if (IS_KOSONG(args[0])) {
        return OBJ_VAL(copyString("kosong", 6));
    } else if (IS_BOOL(args[0])) {
        return OBJ_VAL(copyString("logika", 6));
    } else if (IS_NUMBER(args[0])) {
        return OBJ_VAL(copyString("angka", 5));
    } else if (IS_STRING(args[0])) {
        return OBJ_VAL(copyString("teks", 4));
    } else if (IS_LIST(args[0])) {
        return OBJ_VAL(copyString("daftar", 6));
    } else if (IS_TUPLE(args[0])) {
        return OBJ_VAL(copyString("tupel", 5));
    } else if (IS_SET(args[0])) {
        return AS_SET(args[0])->frozen ? OBJ_VAL(copyString("himpunan_beku", 13))
                                       : OBJ_VAL(copyString("himpunan", 8));
    } else if (IS_DICT(args[0])) {
        return OBJ_VAL(copyString("kamus", 5));
    } else if (IS_INSTANCE(args[0])) {
        return OBJ_VAL(copyString("objek", 5));
    } else if (IS_CLASS(args[0])) {
        return OBJ_VAL(copyString("kelas", 5));
    } else if (IS_FILE(args[0])) {
        return OBJ_VAL(copyString("berkas", 6));
    } else if (IS_DB(args[0])) {
        return OBJ_VAL(copyString("db", 2));
    } else if (IS_SOCKET(args[0])) {
        return OBJ_VAL(copyString("soket", 5));
    } else if (IS_RANGE(args[0])) {
        return OBJ_VAL(copyString("rentang", 7));
    } else if (IS_ITERATOR(args[0])) {
        return OBJ_VAL(copyString("iterator", 8));
    } else if (IS_GENERATOR(args[0])) {
        return OBJ_VAL(copyString("generator", 9));
    } else if (IS_MODULE(args[0])) {
        return OBJ_VAL(copyString("modul", 5));
    } else if (IS_FUNCTION(args[0]) || IS_CLOSURE(args[0]) ||
               IS_NATIVE(args[0]) || IS_BOUND_METHOD(args[0])) {
        return OBJ_VAL(copyString("fungsi", 6));
    } else if (IS_OBJ(args[0])) {
        return OBJ_VAL(copyString("objek", 5));
    }
    return OBJ_VAL(copyString("tidak diketahui", 15));
}

Value nativeFnInput(int argCount, Value* args) {
    if (argCount > 1) {
        nativeRaise("input() menerima maksimal 1 argumen");
        return KOSONG_VAL;
    }

    // Print prompt jika ada
    if (argCount == 1) {
        if (!IS_STRING(args[0])) {
            nativeRaise("prompt harus berupa string");
            return KOSONG_VAL;
        }
        ObjString* prompt = AS_STRING(args[0]);
        printf("%s", prompt->chars);
        fflush(stdout);
    }

    // Read input
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return KOSONG_VAL;
    }

    // Remove newline
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }

    return OBJ_VAL(copyString(buffer, len));
}

Value nativeFnHenti(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("henti() memerlukan 1 argumen");
        return KOSONG_VAL;
    }
    if (!IS_NUMBER(args[0])) {
        nativeRaise("henti() hanya menerima angka (detik)");
        return KOSONG_VAL;
    }

    double seconds = AS_NUMBER(args[0]);
    if (seconds < 0) {
        nativeRaise("henti() tidak boleh negatif");
        return KOSONG_VAL;
    }

#ifdef _WIN32
    Sleep((DWORD)(seconds * 1000));
#else
    usleep((unsigned int)(seconds * 1000000));
#endif

    return KOSONG_VAL;
}

// ============================================================================
// FUNGSI KONVERSI & FORMAT - IMPLEMENTASI
// ============================================================================

// Teks sebuah nilai lewat formatter tunggal (object.c), jadi cetak, ke_teks,
// repr, format, dan f-string selalu sepakat dan tidak pernah terpotong.
// Instansi memakai _teks_/_repr_ (K19). false: metode ajaib bergalat dan
// galatnya sudah tertunda.
static bool valueText(Value v, bool repr, Value* out) {
    TextBuf buf;
    initTextBuf(&buf);
    if (!formatValue(&buf, v, repr, true)) {
        freeTextBuf(&buf);
        return false;
    }
    *out = OBJ_VAL(copyString(buf.length > 0 ? buf.chars : "", buf.length));
    freeTextBuf(&buf);
    return true;
}

Value nativeFnKeAngka(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("ke_angka() memerlukan 1 argumen");
        return NUMBER_VAL(0);
    }
    if (IS_NUMBER(args[0])) return args[0];
    if (!IS_STRING(args[0])) {
        nativeRaise("ke_angka() hanya menerima string atau angka");
        return NUMBER_VAL(0);
    }
    ObjString* str = AS_STRING(args[0]);
    // A string of digits is a bulat, like Python's int(); anything else goes
    // through strtod and becomes a desimal.
    bool hanyaBulat = true;
    for (int i = 0; i < str->length; i++) {
        char c = str->chars[i];
        if (i == 0 && (c == '+' || c == '-')) continue;
        if (c < '0' || c > '9') { hanyaBulat = false; break; }
    }
    if (hanyaBulat && str->length > 0) {
        errno = 0;
        long long bulat = strtoll(str->chars, NULL, 10);
        if (errno == ERANGE) {
            nativeRaiseKelas(vm.galatLuapan, "ke_angka() melebihi jangkauan bilangan bulat");
            return NUMBER_VAL(0);
        }
        return INT_VAL(bulat);
    }

    char* end = NULL;
    double v = strtod(str->chars, &end);
    // Reject empty input or trailing non-numeric garbage.
    if (end == str->chars || *end != '\0') {
        nativeRaise("ke_angka() gagal: '%s' bukan angka valid", str->chars);
        return NUMBER_VAL(0);
    }
    return NUMBER_VAL(v);
}

Value nativeFnKeTeks(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("ke_teks() memerlukan 1 argumen");
        return KOSONG_VAL;
    }
    Value text;
    if (!valueText(args[0], false, &text)) return KOSONG_VAL;
    return text;
}

// format(pola, ...) - ganti setiap '{}' di pola dengan argumen berikutnya.
Value nativeFnFormat(int argCount, Value* args) {
    if (argCount < 1) {
        nativeRaise("format() memerlukan minimal 1 argumen (pola)");
        return KOSONG_VAL;
    }
    if (!IS_STRING(args[0])) {
        nativeRaise("format() argumen pertama harus string pola");
        return KOSONG_VAL;
    }

    ObjString* pola = AS_STRING(args[0]);
    TextBuf buf;
    initTextBuf(&buf);
    int nextArg = 1;
    int literalStart = 0;

    for (int i = 0; i < pola->length; i++) {
        if (pola->chars[i] != '{' || i + 1 >= pola->length || pola->chars[i + 1] != '}') {
            continue;
        }
        textAppend(&buf, pola->chars + literalStart, i - literalStart);
        if (nextArg < argCount) {
            if (!formatValue(&buf, args[nextArg++], false, true)) {
                freeTextBuf(&buf);
                return KOSONG_VAL;
            }
        } else {
            // Not enough args: leave the placeholder literally.
            textAppend(&buf, "{}", 2);
        }
        i++;   // skip '}'
        literalStart = i + 1;
    }
    textAppend(&buf, pola->chars + literalStart, pola->length - literalStart);

    Value result = OBJ_VAL(copyString(buf.length > 0 ? buf.chars : "", buf.length));
    freeTextBuf(&buf);
    return result;
}

// acak_bulat(min, maks) - bilangan bulat acak dalam [min, maks].
Value nativeFnAcakBulat(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaise("acak_bulat() memerlukan 2 argumen (min, maks)");
        return NUMBER_VAL(0);
    }
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        nativeRaise("acak_bulat() hanya menerima angka");
        return NUMBER_VAL(0);
    }
    long long lo = (long long)AS_NUMBER(args[0]);
    long long hi = (long long)AS_NUMBER(args[1]);
    if (lo > hi) {
        nativeRaise("acak_bulat(): min tidak boleh lebih besar dari maks");
        return NUMBER_VAL(0);
    }
    long long range = hi - lo + 1;
    return INT_VAL(lo + (rand() % range));
}

// ============================================================================
// FUNGSI BERKAS (FILE I/O) - IMPLEMENTASI
// Semua kegagalan memakai nativeRaise sehingga dapat ditangkap coba/kecuali.
// ============================================================================

// buka_berkas(path, mode) - Buka berkas, kembalikan handle.
// mode: "b" (baca), "t" (tulis), "s" (sambung/append).
Value nativeFnBukaBerkas(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaise("buka_berkas() memerlukan 2 argumen (path, mode)");
        return KOSONG_VAL;
    }
    if (!IS_STRING(args[0]) || !IS_STRING(args[1])) {
        nativeRaise("buka_berkas() memerlukan (string, string)");
        return KOSONG_VAL;
    }

    ObjString* path = AS_STRING(args[0]);
    const char* m = AS_STRING(args[1])->chars;
    const char* cmode;
    if (strcmp(m, "b") == 0)      cmode = "rb";
    else if (strcmp(m, "t") == 0) cmode = "wb";
    else if (strcmp(m, "s") == 0) cmode = "ab";
    else {
        nativeRaise("buka_berkas() mode tidak dikenal '%s' (pakai \"b\", \"t\", atau \"s\")", m);
        return KOSONG_VAL;
    }

    FILE* fp = fopen(path->chars, cmode);
    if (fp == NULL) {
        nativeRaise("buka_berkas() gagal membuka '%s'", path->chars);
        return KOSONG_VAL;
    }
    return OBJ_VAL(newFile(fp, path));
}

Value nativeFnTutup(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("tutup() memerlukan 1 argumen (berkas)");
        return KOSONG_VAL;
    }
    if (!IS_FILE(args[0])) {
        nativeRaise("tutup() hanya menerima berkas");
        return KOSONG_VAL;
    }
    ObjFile* file = AS_FILE(args[0]);
    if (file->isOpen && file->handle != NULL) {
        fclose((FILE*)file->handle);
        file->handle = NULL;
        file->isOpen = false;
    }
    return KOSONG_VAL;
}

Value nativeFnBaca(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("baca() memerlukan 1 argumen (berkas)");
        return KOSONG_VAL;
    }
    if (!IS_FILE(args[0])) {
        nativeRaise("baca() hanya menerima berkas");
        return KOSONG_VAL;
    }
    ObjFile* file = AS_FILE(args[0]);
    if (!file->isOpen || file->handle == NULL) {
        nativeRaise("baca() pada berkas yang sudah tertutup");
        return KOSONG_VAL;
    }

    FILE* fp = (FILE*)file->handle;
    long cur = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    long end = ftell(fp);
    fseek(fp, cur, SEEK_SET);
    long remaining = end - cur;
    if (remaining < 0) remaining = 0;

    char* buffer = ALLOCATE(char, remaining + 1);
    size_t read = fread(buffer, 1, (size_t)remaining, fp);
    buffer[read] = '\0';
    ObjString* result = takeString(buffer, (int)read);
    return OBJ_VAL(result);
}

Value nativeFnBacaBaris(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("baca_baris() memerlukan 1 argumen (berkas)");
        return KOSONG_VAL;
    }
    if (!IS_FILE(args[0])) {
        nativeRaise("baca_baris() hanya menerima berkas");
        return KOSONG_VAL;
    }
    ObjFile* file = AS_FILE(args[0]);
    if (!file->isOpen || file->handle == NULL) {
        nativeRaise("baca_baris() pada berkas yang sudah tertutup");
        return KOSONG_VAL;
    }

    FILE* fp = (FILE*)file->handle;
    int cap = 128, len = 0;
    char* buffer = ALLOCATE(char, cap);
    int c;
    bool any = false;
    while ((c = fgetc(fp)) != EOF) {
        any = true;
        if (c == '\n') break;
        if (len + 1 >= cap) {
            int oldCap = cap;
            cap *= 2;
            buffer = GROW_ARRAY(char, buffer, oldCap, cap);
        }
        buffer[len++] = (char)c;
    }
    if (!any && len == 0) {
        // EOF with nothing read -> kosong signals end of file.
        FREE_ARRAY(char, buffer, cap);
        return KOSONG_VAL;
    }
    // Strip a trailing '\r' (Windows line endings).
    if (len > 0 && buffer[len - 1] == '\r') len--;
    buffer[len] = '\0';
    ObjString* result = takeString(buffer, len);
    // takeString may free `buffer` if interned; the capacity we allocated is
    // len+1..cap, so shrink accounting is not tracked precisely here (small
    // transient). Acceptable for a line reader.
    return OBJ_VAL(result);
}

Value nativeFnTulis(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaise("tulis() memerlukan 2 argumen (berkas, teks)");
        return KOSONG_VAL;
    }
    if (!IS_FILE(args[0]) || !IS_STRING(args[1])) {
        nativeRaise("tulis() memerlukan (berkas, string)");
        return KOSONG_VAL;
    }
    ObjFile* file = AS_FILE(args[0]);
    if (!file->isOpen || file->handle == NULL) {
        nativeRaise("tulis() pada berkas yang sudah tertutup");
        return KOSONG_VAL;
    }
    ObjString* text = AS_STRING(args[1]);
    size_t written = fwrite(text->chars, 1, (size_t)text->length, (FILE*)file->handle);
    if (written != (size_t)text->length) {
        nativeRaise("tulis() gagal menulis ke '%s'", file->path->chars);
        return KOSONG_VAL;
    }
    return INT_VAL((int64_t)written);
}

Value nativeFnBacaSemua(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("baca_semua() memerlukan 1 argumen (path)");
        return KOSONG_VAL;
    }
    if (!IS_STRING(args[0])) {
        nativeRaise("baca_semua() hanya menerima string path");
        return KOSONG_VAL;
    }
    ObjString* path = AS_STRING(args[0]);
    FILE* fp = fopen(path->chars, "rb");
    if (fp == NULL) {
        nativeRaise("baca_semua() gagal membuka '%s'", path->chars);
        return KOSONG_VAL;
    }
    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    if (size < 0) size = 0;
    rewind(fp);
    char* buffer = ALLOCATE(char, size + 1);
    size_t read = fread(buffer, 1, (size_t)size, fp);
    buffer[read] = '\0';
    fclose(fp);
    return OBJ_VAL(takeString(buffer, (int)read));
}

static Value writeWholeFile(const char* path, ObjString* text, const char* mode, const char* fname) {
    FILE* fp = fopen(path, mode);
    if (fp == NULL) {
        nativeRaise("%s() gagal membuka '%s'", fname, path);
        return KOSONG_VAL;
    }
    size_t written = fwrite(text->chars, 1, (size_t)text->length, fp);
    fclose(fp);
    if (written != (size_t)text->length) {
        nativeRaise("%s() gagal menulis ke '%s'", fname, path);
        return KOSONG_VAL;
    }
    return INT_VAL((int64_t)written);
}

Value nativeFnTulisSemua(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaise("tulis_semua() memerlukan 2 argumen (path, teks)");
        return KOSONG_VAL;
    }
    if (!IS_STRING(args[0]) || !IS_STRING(args[1])) {
        nativeRaise("tulis_semua() memerlukan (string, string)");
        return KOSONG_VAL;
    }
    return writeWholeFile(AS_STRING(args[0])->chars, AS_STRING(args[1]), "wb", "tulis_semua");
}

Value nativeFnTambahBerkas(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaise("tambah_berkas() memerlukan 2 argumen (path, teks)");
        return KOSONG_VAL;
    }
    if (!IS_STRING(args[0]) || !IS_STRING(args[1])) {
        nativeRaise("tambah_berkas() memerlukan (string, string)");
        return KOSONG_VAL;
    }
    return writeWholeFile(AS_STRING(args[0])->chars, AS_STRING(args[1]), "ab", "tambah_berkas");
}

Value nativeFnAdaBerkas(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("ada_berkas() memerlukan 1 argumen (path)");
        return BOOL_VAL(false);
    }
    if (!IS_STRING(args[0])) {
        nativeRaise("ada_berkas() hanya menerima string path");
        return BOOL_VAL(false);
    }
    FILE* fp = fopen(AS_STRING(args[0])->chars, "rb");
    if (fp == NULL) return BOOL_VAL(false);
    fclose(fp);
    return BOOL_VAL(true);
}

Value nativeFnHapusBerkas(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("hapus_berkas() memerlukan 1 argumen (path)");
        return BOOL_VAL(false);
    }
    if (!IS_STRING(args[0])) {
        nativeRaise("hapus_berkas() hanya menerima string path");
        return BOOL_VAL(false);
    }
    if (remove(AS_STRING(args[0])->chars) != 0) {
        nativeRaise("hapus_berkas() gagal menghapus '%s'", AS_STRING(args[0])->chars);
        return BOOL_VAL(false);
    }
    return BOOL_VAL(true);
}

// ============================================================================
// FUNGSI WAKTU & TANGGAL - IMPLEMENTASI
// ============================================================================

// waktu() - Detik sejak epoch (Unix time).
Value nativeFnWaktu(int argCount, Value* args) {
    (void)args;
    if (argCount != 0) {
        nativeRaise("waktu() tidak menerima argumen");
        return NUMBER_VAL(0);
    }
    return NUMBER_VAL((double)time(NULL));
}

// Helper: set a numeric entry on a dict (key is a C string).
static void dictSetNum(ObjDict* dict, const char* key, double num) {
    ObjString* k = copyString(key, (int)strlen(key));
    push(OBJ_VAL(k));  // keep reachable during dictionary growth
    dictSet(dict, OBJ_VAL(k), NUMBER_VAL(num));
    pop();
}

// tanggal() - Kamus berisi komponen tanggal/waktu lokal saat ini:
// {tahun, bulan, hari, jam, menit, detik, hari_minggu, hari_tahun}
Value nativeFnTanggal(int argCount, Value* args) {
    (void)args;
    if (argCount != 0) {
        nativeRaise("tanggal() tidak menerima argumen");
        return KOSONG_VAL;
    }
    time_t t = time(NULL);
    struct tm* lt = localtime(&t);
    if (lt == NULL) {
        nativeRaise("tanggal() gagal membaca waktu sistem");
        return KOSONG_VAL;
    }

    ObjDict* dict = newDict();
    push(OBJ_VAL(dict));  // keep reachable while populating
    dictSetNum(dict, "tahun",       lt->tm_year + 1900);
    dictSetNum(dict, "bulan",       lt->tm_mon + 1);
    dictSetNum(dict, "hari",        lt->tm_mday);
    dictSetNum(dict, "jam",         lt->tm_hour);
    dictSetNum(dict, "menit",       lt->tm_min);
    dictSetNum(dict, "detik",       lt->tm_sec);
    dictSetNum(dict, "hari_minggu", lt->tm_wday);
    dictSetNum(dict, "hari_tahun",  lt->tm_yday + 1);
    pop();
    return OBJ_VAL(dict);
}

// format_tanggal(pola) - Format waktu lokal saat ini dengan pola strftime.
Value nativeFnFormatTanggal(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("format_tanggal() memerlukan 1 argumen (pola)");
        return KOSONG_VAL;
    }
    if (!IS_STRING(args[0])) {
        nativeRaise("format_tanggal() hanya menerima string pola");
        return KOSONG_VAL;
    }
    time_t t = time(NULL);
    struct tm* lt = localtime(&t);
    if (lt == NULL) {
        nativeRaise("format_tanggal() gagal membaca waktu sistem");
        return KOSONG_VAL;
    }
    char buf[256];
    size_t n = strftime(buf, sizeof(buf), AS_STRING(args[0])->chars, lt);
    return OBJ_VAL(copyString(buf, (int)n));
}

// ============================================================================
// PRIMITIF TEKS - untuk membangun pustaka teks di bahasa .idk
// ============================================================================

// karakter(teks, i) - Karakter (string 1-byte) pada indeks i.
Value nativeFnKarakter(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaise("karakter() memerlukan 2 argumen (teks, indeks)");
        return KOSONG_VAL;
    }
    if (!IS_STRING(args[0]) || !IS_NUMBER(args[1])) {
        nativeRaise("karakter() memerlukan (string, angka)");
        return KOSONG_VAL;
    }
    ObjString* s = AS_STRING(args[0]);
    int i = (int)AS_NUMBER(args[1]);
    if (i < 0 || i >= s->charCount) {
        nativeRaise("karakter() indeks di luar jangkauan (indeks %d, panjang %d)", i, s->charCount);
        return KOSONG_VAL;
    }
    int offset = utf8ByteOffset(s, i);
    return OBJ_VAL(copyString(s->chars + offset, utf8CharBytes(s, offset)));
}

// potong(teks, mulai, jumlah) - Substring mulai dari 'mulai' sepanjang 'jumlah'.
// Jika jumlah melewati akhir string, dipotong sampai akhir.
Value nativeFnPotong(int argCount, Value* args) {
    if (argCount != 3) {
        nativeRaise("potong() memerlukan 3 argumen (teks, mulai, jumlah)");
        return KOSONG_VAL;
    }
    if (!IS_STRING(args[0]) || !IS_NUMBER(args[1]) || !IS_NUMBER(args[2])) {
        nativeRaise("potong() memerlukan (string, angka, angka)");
        return KOSONG_VAL;
    }
    ObjString* s = AS_STRING(args[0]);
    int start = (int)AS_NUMBER(args[1]);
    int count = (int)AS_NUMBER(args[2]);
    if (start < 0 || start > s->charCount) {
        nativeRaise("potong() 'mulai' di luar jangkauan (%d, panjang %d)", start, s->charCount);
        return KOSONG_VAL;
    }
    if (count < 0) count = 0;
    if (start + count > s->charCount) count = s->charCount - start;
    int from = utf8ByteOffset(s, start);
    int to = utf8ByteOffset(s, start + count);
    return OBJ_VAL(copyString(s->chars + from, to - from));
}

// ============================================================================
// PRIMITIF KARAKTER & LIST TAMBAHAN
// ============================================================================

// Decode the first UTF-8 character of `chars`. Malformed input falls back to
// the value of the lead byte.
static uint32_t decodeUtf8(const char* chars, int length) {
    const unsigned char* s = (const unsigned char*)chars;
    if (s[0] < 0x80 || length < 2) return s[0];
    if ((s[0] & 0xE0) == 0xC0) return ((s[0] & 0x1Fu) << 6) | (s[1] & 0x3Fu);
    if ((s[0] & 0xF0) == 0xE0 && length >= 3) {
        return ((s[0] & 0x0Fu) << 12) | ((s[1] & 0x3Fu) << 6) | (s[2] & 0x3Fu);
    }
    if ((s[0] & 0xF8) == 0xF0 && length >= 4) {
        return ((s[0] & 0x07u) << 18) | ((s[1] & 0x3Fu) << 12) |
               ((s[2] & 0x3Fu) << 6) | (s[3] & 0x3Fu);
    }
    return s[0];
}

// Encode a code point as UTF-8 into `out` (4 bytes max); returns the length.
static int encodeUtf8(uint32_t code, char* out) {
    if (code < 0x80) {
        out[0] = (char)code;
        return 1;
    }
    if (code < 0x800) {
        out[0] = (char)(0xC0 | (code >> 6));
        out[1] = (char)(0x80 | (code & 0x3F));
        return 2;
    }
    if (code < 0x10000) {
        out[0] = (char)(0xE0 | (code >> 12));
        out[1] = (char)(0x80 | ((code >> 6) & 0x3F));
        out[2] = (char)(0x80 | (code & 0x3F));
        return 3;
    }
    out[0] = (char)(0xF0 | (code >> 18));
    out[1] = (char)(0x80 | ((code >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((code >> 6) & 0x3F));
    out[3] = (char)(0x80 | (code & 0x3F));
    return 4;
}

// ord(karakter) - Kode Unicode (code point) dari string 1 karakter.
Value nativeFnOrd(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("ord() memerlukan 1 argumen (karakter)");
        return NUMBER_VAL(0);
    }
    if (!IS_STRING(args[0])) {
        nativeRaise("ord() hanya menerima string");
        return NUMBER_VAL(0);
    }
    ObjString* s = AS_STRING(args[0]);
    if (s->charCount != 1) {
        nativeRaise("ord() memerlukan string tepat 1 karakter (panjang %d)", s->charCount);
        return NUMBER_VAL(0);
    }
    return INT_VAL((int64_t)decodeUtf8(s->chars, s->length));
}

// chr(kode) - Karakter dari kode Unicode 0-0x10FFFF (dikodekan sebagai UTF-8).
Value nativeFnChr(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("chr() memerlukan 1 argumen (kode)");
        return KOSONG_VAL;
    }
    if (!IS_NUMBER(args[0])) {
        nativeRaise("chr() hanya menerima angka");
        return KOSONG_VAL;
    }
    int code = (int)AS_NUMBER(args[0]);
    if (code < 0 || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) {
        nativeRaise("chr() kode di luar jangkauan Unicode (%d)", code);
        return KOSONG_VAL;
    }
    char bytes[4];
    int length = encodeUtf8((uint32_t)code, bytes);
    return OBJ_VAL(copyString(bytes, length));
}

// Largest |value| for which rentang() arithmetic stays exact in a double.
#define RENTANG_MAKS 9007199254740992.0  // 2^53

// rentang(akhir) / rentang(awal, akhir) / rentang(awal, akhir, langkah) -
// Deret bilangan bulat yang malas (seperti range di Python 3).
Value nativeFnRentang(int argCount, Value* args) {
    if (argCount < 1 || argCount > 3) {
        nativeRaise("rentang() memerlukan 1 sampai 3 argumen, bukan %d", argCount);
        return KOSONG_VAL;
    }
    for (int i = 0; i < argCount; i++) {
        if (!IS_NUMBER(args[i]) || AS_NUMBER(args[i]) != floor(AS_NUMBER(args[i])) ||
            fabs(AS_NUMBER(args[i])) > RENTANG_MAKS) {
            nativeRaise("rentang() hanya menerima bilangan bulat");
            return KOSONG_VAL;
        }
    }
    double start = argCount == 1 ? 0 : AS_NUMBER(args[0]);
    double stop = argCount == 1 ? AS_NUMBER(args[0]) : AS_NUMBER(args[1]);
    double step = argCount == 3 ? AS_NUMBER(args[2]) : 1;
    if (step == 0) {
        nativeRaise("rentang() langkah tidak boleh nol");
        return KOSONG_VAL;
    }
    return OBJ_VAL(newRange(start, stop, step));
}

// hapus_pada(list, i) - Hapus & kembalikan elemen pada indeks i, geser sisanya.
Value nativeFnHapusPada(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaise("hapus_pada() memerlukan 2 argumen (list, indeks)");
        return KOSONG_VAL;
    }
    if (rejectTuple(args[0])) return KOSONG_VAL;
    if (!IS_LIST(args[0]) || !IS_NUMBER(args[1])) {
        nativeRaise("hapus_pada() memerlukan (list, angka)");
        return KOSONG_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    int i = (int)AS_NUMBER(args[1]);
    if (i < 0 || i >= list->items.count) {
        nativeRaise("hapus_pada() indeks di luar jangkauan (indeks %d, panjang %d)", i, list->items.count);
        return KOSONG_VAL;
    }
    Value removed = list->items.values[i];
    for (int j = i; j < list->items.count - 1; j++) {
        list->items.values[j] = list->items.values[j + 1];
    }
    list->items.count--;
    return removed;
}

// sisip(list, i, nilai) - Sisipkan nilai pada indeks i (i == panjang -> append).
Value nativeFnSisip(int argCount, Value* args) {
    if (argCount != 3) {
        nativeRaise("sisip() memerlukan 3 argumen (list, indeks, nilai)");
        return KOSONG_VAL;
    }
    if (rejectTuple(args[0])) return KOSONG_VAL;
    if (!IS_LIST(args[0]) || !IS_NUMBER(args[1])) {
        nativeRaise("sisip() memerlukan (list, angka, nilai)");
        return KOSONG_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    int i = (int)AS_NUMBER(args[1]);
    if (i < 0 || i > list->items.count) {
        nativeRaise("sisip() indeks di luar jangkauan (indeks %d, panjang %d)", i, list->items.count);
        return KOSONG_VAL;
    }
    // Grow by appending a placeholder (writeValueArray handles capacity + GC),
    // then shift elements right to open a slot at i.
    writeValueArray(&list->items, KOSONG_VAL);
    for (int j = list->items.count - 1; j > i; j--) {
        list->items.values[j] = list->items.values[j - 1];
    }
    list->items.values[i] = args[2];
    return args[0];
}

// ============================================================================
// SOKET (TCP) - IMPLEMENTASI
// Server HTTP sederhana. Listen di 127.0.0.1 (localhost) demi keamanan.
// Semua kegagalan memakai nativeRaise (dapat ditangkap coba/kecuali).
// ============================================================================

void indkCloseSocket(long long fd) {
    INDK_CLOSESOCKET((indk_socket_t)fd);
}

#ifdef _WIN32
static int indkWinsockReady = 0;
static void indkEnsureWinsock() {
    if (!indkWinsockReady) {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
        indkWinsockReady = 1;
    }
}
#else
static void indkEnsureWinsock() { }
#endif

// soket_dengar(port [, alamat]) -> soket server yang mendengarkan.
//
// Bawaannya 127.0.0.1: server baru TIDAK langsung terbuka ke jaringan hanya
// karena dijalankan. Itu default yang disengaja dan tidak diubah.
//
// Argumen kedua opsional "0.0.0.0" membuka ke semua antarmuka. Ini WAJIB di
// dalam container: proses di container lain menghubungi lewat IP jaringan
// Docker, dan soket yang terikat 127.0.0.1 hanya menerima dari dalam
// container itu sendiri -- gejalanya "Connection refused" dari reverse proxy
// padahal server jelas hidup.
Value nativeFnSoketDengar(int argCount, Value* args) {
    if (argCount < 1 || argCount > 2) {
        nativeRaise("soket_dengar() memerlukan 1-2 argumen (port [, alamat])");
        return KOSONG_VAL;
    }
    if (!IS_NUMBER(args[0])) {
        nativeRaise("soket_dengar() hanya menerima angka (port)");
        return KOSONG_VAL;
    }
    int port = (int)AS_NUMBER(args[0]);
    if (port < 1 || port > 65535) {
        nativeRaise("soket_dengar() port di luar jangkauan (1-65535)");
        return KOSONG_VAL;
    }

    // Hanya dua nilai yang diterima. Alamat bebas belum didukung, dan menerima
    // teks apa pun lalu diam-diam jatuh ke loopback akan menyembunyikan salah
    // ketik yang membuat server tampak hidup tapi tak bisa dihubungi.
    bool semua_antarmuka = false;
    if (argCount == 2) {
        if (!IS_STRING(args[1])) {
            nativeRaise("soket_dengar() alamat harus teks (\"127.0.0.1\" atau \"0.0.0.0\")");
            return KOSONG_VAL;
        }
        const char* alamat = AS_CSTRING(args[1]);
        if (strcmp(alamat, "0.0.0.0") == 0) {
            semua_antarmuka = true;
        } else if (strcmp(alamat, "127.0.0.1") != 0) {
            nativeRaise("soket_dengar() alamat hanya boleh \"127.0.0.1\" atau \"0.0.0.0\"");
            return KOSONG_VAL;
        }
    }

    indkEnsureWinsock();

    indk_socket_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INDK_INVALID_SOCKET) {
        nativeRaise("soket_dengar() gagal membuat soket");
        return KOSONG_VAL;
    }

    // Izinkan reuse alamat agar restart cepat tidak "address in use".
    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(semua_antarmuka ? INADDR_ANY : INADDR_LOOPBACK);

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        INDK_CLOSESOCKET(s);
        nativeRaise("soket_dengar() gagal bind ke port %d", port);
        return KOSONG_VAL;
    }
    if (listen(s, 16) != 0) {
        INDK_CLOSESOCKET(s);
        nativeRaise("soket_dengar() gagal listen di port %d", port);
        return KOSONG_VAL;
    }

    return OBJ_VAL(newSocket((long long)s));
}

// soket_sambung(host, port) -> soket TCP yang tersambung ke host:port
// (klien; padanan socket.create_connection). Nama host diresolusi dengan
// getaddrinfo, IPv4 maupun IPv6. Ditolak di `indk --sandbox`: membuka
// koneksi keluar adalah akses ke luar program.
Value nativeFnSoketSambung(int argCount, Value* args) {
    if (argCount != 2 || !IS_STRING(args[0]) || !IS_INT(args[1])) {
        nativeRaiseKelas(vm.galatTipe, "soket_sambung(host, port) memerlukan teks dan bulat");
        return KOSONG_VAL;
    }
    if (sistemSandbox()) {
        nativeRaiseKelas(vm.galatIzin, "soket_sambung() tidak diizinkan dalam mode --sandbox");
        return KOSONG_VAL;
    }
    const char* host = AS_CSTRING(args[0]);
    int64_t port = AS_INT(args[1]);
    if (port < 1 || port > 65535) {
        nativeRaiseKelas(vm.galatNilai, "soket_sambung(): port di luar jangkauan (1-65535)");
        return KOSONG_VAL;
    }

    indkEnsureWinsock();

    char portTeks[8];
    snprintf(portTeks, sizeof(portTeks), "%d", (int)port);
    struct addrinfo petunjuk;
    memset(&petunjuk, 0, sizeof(petunjuk));
    petunjuk.ai_family = AF_UNSPEC;
    petunjuk.ai_socktype = SOCK_STREAM;
    struct addrinfo* alamat = NULL;
    if (getaddrinfo(host, portTeks, &petunjuk, &alamat) != 0 || alamat == NULL) {
        nativeRaiseKelas(vm.galatOS, "soket_sambung(): host '%s' tidak ditemukan", host);
        return KOSONG_VAL;
    }
    indk_socket_t s = INDK_INVALID_SOCKET;
    for (struct addrinfo* a = alamat; a != NULL; a = a->ai_next) {
        s = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
        if (s == INDK_INVALID_SOCKET) continue;
        if (connect(s, a->ai_addr, (int)a->ai_addrlen) == 0) break;
        INDK_CLOSESOCKET(s);
        s = INDK_INVALID_SOCKET;
    }
    freeaddrinfo(alamat);
    if (s == INDK_INVALID_SOCKET) {
        nativeRaiseKelas(vm.galatOS, "soket_sambung(): tidak dapat tersambung ke %s:%d", host, (int)port);
        return KOSONG_VAL;
    }
    return OBJ_VAL(newSocket((long long)s));
}

// tls_mulai(soket, host) -> kosong. Meningkatkan soket yang SUDAH tersambung
// menjadi koneksi TLS terverifikasi. Sesudah ini soket_baca/soket_tulis/
// soket_tutup bekerja seperti biasa, tetapi terenkripsi -- itulah sebabnya
// tidak ada builtin tls_baca/tls_tulis terpisah.
//
// `host` dipakai untuk SNI sekaligus verifikasi nama, jadi harus nama host
// yang diminta pengguna, bukan alamat IP hasil resolusi.
//
// Verifikasi sertifikat SELALU aktif dan tidak bisa dimatikan. Untuk CA
// sendiri, setel INDK_CA_BUNDLE (lihat src/tls.c).
Value nativeFnTlsMulai(int argCount, Value* args) {
    if (argCount != 2 || !IS_SOCKET(args[0]) || !IS_STRING(args[1])) {
        nativeRaiseKelas(vm.galatTipe, "tls_mulai(soket, host) memerlukan soket dan teks");
        return KOSONG_VAL;
    }
    ObjSocket* sock = AS_SOCKET(args[0]);
    if (!sock->isOpen) {
        nativeRaise("tls_mulai() pada soket yang sudah tertutup");
        return KOSONG_VAL;
    }
    if (sock->tls != NULL) {
        nativeRaise("tls_mulai() pada soket yang sudah memakai TLS");
        return KOSONG_VAL;
    }
    const char* host = AS_CSTRING(args[1]);
    if (host[0] == '\0') {
        nativeRaiseKelas(vm.galatNilai, "tls_mulai(): host tidak boleh kosong");
        return KOSONG_VAL;
    }

    char galat[256];
    galat[0] = '\0';
    IndkTls* tls = indkTlsMulai(sock->fd, host, galat, sizeof(galat));
    if (tls == NULL) {
        // Pesan dari tls.c sudah menyebutkan SEBAB kegagalan (kedaluwarsa,
        // nama tidak cocok, penerbit tak dikenal), bukan sekadar "gagal".
        // galatOS mengikuti soket_sambung(); GalatJaringan adalah kelas di
        // pustaka/http.idk, tidak tersedia dari C.
        nativeRaiseKelas(vm.galatOS, "%s", galat);
        return KOSONG_VAL;
    }
    sock->tls = tls;
    return KOSONG_VAL;
}

// soket_terima(server) -> soket klien (blocking sampai ada koneksi).
Value nativeFnSoketTerima(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("soket_terima() memerlukan 1 argumen (soket server)");
        return KOSONG_VAL;
    }
    if (!IS_SOCKET(args[0])) {
        nativeRaise("soket_terima() hanya menerima soket");
        return KOSONG_VAL;
    }
    ObjSocket* server = AS_SOCKET(args[0]);
    if (!server->isOpen) {
        nativeRaise("soket_terima() pada soket yang sudah tertutup");
        return KOSONG_VAL;
    }
    indk_socket_t c = accept((indk_socket_t)server->fd, NULL, NULL);
    if (c == INDK_INVALID_SOCKET) {
        nativeRaise("soket_terima() gagal menerima koneksi");
        return KOSONG_VAL;
    }
    return OBJ_VAL(newSocket((long long)c));
}

// Benar jika galat soket terakhir adalah batas waktu terlampaui, bukan
// kegagalan sungguhan. Dipisah karena pesan untuk pengguna berbeda: klien
// lambat bukan soket rusak.
static bool soketBatasWaktuTerlampaui(void) {
#ifdef _WIN32
    return WSAGetLastError() == WSAETIMEDOUT;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}

// soket_batas_waktu(soket, detik) -> kosong.
// Mengatur batas waktu baca DAN tulis untuk satu soket. detik = 0 berarti
// tanpa batas (perilaku bawaan, sama seperti sebelum builtin ini ada).
Value nativeFnSoketBatasWaktu(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaise("soket_batas_waktu() memerlukan 2 argumen (soket, detik)");
        return KOSONG_VAL;
    }
    if (!IS_SOCKET(args[0]) || !IS_NUMBER(args[1])) {
        nativeRaise("soket_batas_waktu() memerlukan (soket, angka)");
        return KOSONG_VAL;
    }
    double detik = AS_NUMBER(args[1]);
    if (detik < 0) {
        nativeRaise("soket_batas_waktu() tidak menerima detik negatif");
        return KOSONG_VAL;
    }
    ObjSocket* sock = AS_SOCKET(args[0]);
    if (!sock->isOpen) {
        nativeRaise("soket_batas_waktu() pada soket yang sudah tertutup");
        return KOSONG_VAL;
    }

    // Winsock memakai DWORD milidetik untuk opsi yang pada POSIX bertipe
    // struct timeval; bukan sekadar beda tipe, beda satuan.
#ifdef _WIN32
    DWORD nilai = (DWORD)(detik * 1000.0);
    const char* p = (const char*)&nilai;
    int n = (int)sizeof(nilai);
#else
    struct timeval nilai;
    nilai.tv_sec = (time_t)detik;
    nilai.tv_usec = (suseconds_t)((detik - (double)nilai.tv_sec) * 1000000.0);
    const char* p = (const char*)&nilai;
    socklen_t n = (socklen_t)sizeof(nilai);
#endif
    if (setsockopt((indk_socket_t)sock->fd, SOL_SOCKET, SO_RCVTIMEO, p, n) != 0 ||
        setsockopt((indk_socket_t)sock->fd, SOL_SOCKET, SO_SNDTIMEO, p, n) != 0) {
        nativeRaise("soket_batas_waktu() gagal mengatur batas waktu soket");
        return KOSONG_VAL;
    }
    return KOSONG_VAL;
}

// soket_baca(klien) -> teks yang diterima (satu recv, maks ~64KB), atau
// string kosong jika koneksi ditutup pihak lain.
Value nativeFnSoketBaca(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("soket_baca() memerlukan 1 argumen (soket klien)");
        return KOSONG_VAL;
    }
    if (!IS_SOCKET(args[0])) {
        nativeRaise("soket_baca() hanya menerima soket");
        return KOSONG_VAL;
    }
    ObjSocket* client = AS_SOCKET(args[0]);
    if (!client->isOpen) {
        nativeRaise("soket_baca() pada soket yang sudah tertutup");
        return KOSONG_VAL;
    }

    int cap = 65536;
    char* buffer = ALLOCATE(char, cap);
    int received;
    bool batasWaktu;
    if (client->tls != NULL) {
        received = indkTlsBaca(client->tls, buffer, cap - 1);
        batasWaktu = (received == -2);
        if (received < 0) received = -1;
    } else {
        received = (int)recv((indk_socket_t)client->fd, buffer, cap - 1, 0);
        batasWaktu = (received < 0 && soketBatasWaktuTerlampaui());
    }
    if (received < 0) {
        FREE_ARRAY(char, buffer, cap);
        // Batas waktu BUKAN koneksi tertutup: mengembalikan "" akan membuat
        // baca_permintaan() menganggap permintaan separuh sudah lengkap.
        if (batasWaktu) {
            nativeRaise("soket_baca() melewati batas waktu menunggu data");
        } else {
            nativeRaise("soket_baca() gagal membaca dari soket");
        }
        return KOSONG_VAL;
    }
    buffer[received] = '\0';
    ObjString* result = takeString(buffer, received);
    return OBJ_VAL(result);
}

// soket_tulis(klien, teks) -> jumlah byte terkirim.
Value nativeFnSoketTulis(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaise("soket_tulis() memerlukan 2 argumen (soket, teks)");
        return NUMBER_VAL(0);
    }
    if (!IS_SOCKET(args[0]) || !IS_STRING(args[1])) {
        nativeRaise("soket_tulis() memerlukan (soket, string)");
        return NUMBER_VAL(0);
    }
    ObjSocket* client = AS_SOCKET(args[0]);
    if (!client->isOpen) {
        nativeRaise("soket_tulis() pada soket yang sudah tertutup");
        return NUMBER_VAL(0);
    }
    ObjString* text = AS_STRING(args[1]);
    if (client->tls != NULL) {
        int rc = indkTlsTulis(client->tls, text->chars, (size_t)text->length);
        if (rc < 0) {
            nativeRaise(rc == -2 ? "soket_tulis() melewati batas waktu mengirim data"
                                 : "soket_tulis() gagal mengirim data");
            return NUMBER_VAL(0);
        }
        return NUMBER_VAL((double)rc);
    }
    int total = 0;
    while (total < text->length) {
        int sent = (int)send((indk_socket_t)client->fd,
                             text->chars + total, text->length - total, 0);
        if (sent <= 0) {
            if (sent < 0 && soketBatasWaktuTerlampaui()) {
                nativeRaise("soket_tulis() melewati batas waktu mengirim data");
            } else {
                nativeRaise("soket_tulis() gagal mengirim data");
            }
            return NUMBER_VAL(total);
        }
        total += sent;
    }
    return NUMBER_VAL((double)total);
}

// soket_tutup(soket) -> tutup soket (server atau klien).
Value nativeFnSoketTutup(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("soket_tutup() memerlukan 1 argumen (soket)");
        return KOSONG_VAL;
    }
    if (!IS_SOCKET(args[0])) {
        nativeRaise("soket_tutup() hanya menerima soket");
        return KOSONG_VAL;
    }
    ObjSocket* sock = AS_SOCKET(args[0]);
    // Sesi TLS dibereskan lebih dulu supaya close_notify terkirim selagi fd
    // masih hidup; sesudah itu baru soketnya ditutup.
    if (sock->tls != NULL) {
        indkTlsTutup(sock->tls);
        sock->tls = NULL;
    }
    if (sock->isOpen) {
        INDK_CLOSESOCKET((indk_socket_t)sock->fd);
        sock->isOpen = false;
    }
    return KOSONG_VAL;
}

// ============================================================================
// AGREGASI & KOLEKSI
// ============================================================================
//
// Semua fungsi di bawah menerima tiga iterable dasar (daftar, rentang, teks)
// lewat iterCount()/iterAt(), meniru pola nativeFnEnumerasi.
//
// GC-SAFETY: iterAt() mengalokasi untuk elemen teks dan writeValueArray dapat
// menumbuhkan array; daftar hasil WAJIB berada di stack VM selama seluruh
// loop pengisian. peta()/saring() memanggil balik ke VM lewat callFromNative,
// yang bisa memicu GC kapan saja.

// Jumlah elemen iterable; -1 bila tipenya tidak iterable.
static int iterCount(Value v) {
    if (sequenceItems(v) != NULL) return sequenceItems(v)->count;
    if (IS_SET(v)) return AS_SET(v)->keys.count;
    if (IS_STRING(v)) return AS_STRING(v)->charCount;
    if (IS_RANGE(v)) return (int)rangeLength(AS_RANGE(v));
    return -1;
}

// Fase 5: a generator or custom iterator has no length and no index, so the
// index-based natives below cannot walk it. Drain it into a plain daftar once
// and let every existing consumer work unchanged.
//
// Returns true when *v was replaced by a materialized list (or left alone),
// false when draining raised -- the error is already pending.
//
// GC-SAFETY: callers MUST pass a rooted slot (&args[n] lives on the VM stack
// and is marked). Writing the new list anywhere else leaves it collectable the
// moment the caller allocates again. The list being filled is also kept on the
// VM stack, because resuming the generator runs arbitrary user code.
static bool materializeLazy(Value* v) {
    bool isGen = IS_GENERATOR(*v);
    bool isLazyInstance = IS_INSTANCE(*v) && instanceHasMagic(*v, MAGIC_BERIKUTNYA);
    if (!isGen && !isLazyInstance) return true;

    ObjList* out = newList();
    push(OBJ_VAL(out));                  // root while filling

    Value source = *v;
    for (;;) {
        Value item;
        if (isGen) {
            int step = resumeGenerator(AS_GENERATOR(source), &item);
            if (step < 0) { pop(); return false; }
            if (step == 0) break;
        } else {
            int found = instanceNextFor(source, &item);
            if (found < 0) {
                pop();
                nativeRaiseKelas(vm.galatTipe, "Objek kehilangan metode _berikutnya_().");
                return false;
            }
            if (found == 0) {
                // HentiIterasi means "done"; anything else propagates.
                if (vm.nativeErrorPending &&
                    valueIsInstanceOf(vm.nativeErrorValue, OBJ_VAL(vm.hentiIterasi))) {
                    vm.nativeErrorPending = false;
                    vm.nativeErrorValue = KOSONG_VAL;
                    break;
                }
                pop();
                return false;
            }
        }
        push(item);                      // root the element across the write
        writeValueArray(&out->items, item);
        pop();
    }
    pop();
    *v = OBJ_VAL(out);
    return true;
}

// Elemen ke-i. Untuk teks ini MENGALOKASI sebuah ObjString baru.
static Value iterAt(Value v, int i) {
    if (sequenceItems(v) != NULL) return sequenceItems(v)->values[i];
    if (IS_SET(v)) return AS_SET(v)->keys.values[i];
    if (IS_RANGE(v)) return rangeElement(AS_RANGE(v), i);
    ObjString* teks = AS_STRING(v);
    int offset = utf8ByteOffset(teks, i);
    return OBJ_VAL(copyString(teks->chars + offset, utf8CharBytes(teks, offset)));
}

// tupel() / tupel(urutan) - tupel baru dari daftar, tupel, teks, rentang, atau
// kamus (kuncinya), seperti tuple() di Python.
Value nativeFnTupel(int argCount, Value* args) {
    if (argCount > 1) {
        nativeRaise("tupel() memerlukan 0 atau 1 argumen, bukan %d", argCount);
        return KOSONG_VAL;
    }
    if (argCount == 0) return OBJ_VAL(newTuple());
    Value src = args[0];
    if (IS_TUPLE(src)) return src;   // immutable, so sharing it is safe (Python does too)

    // Fase 5: cairkan dulu (slot argumen tetap di-root oleh stack VM).
    if (!materializeLazy(&args[0])) return KOSONG_VAL;
    src = args[0];
    int count = IS_DICT(src) ? dictLength(AS_DICT(src)) : iterCount(src);
    if (count < 0) {
        nativeRaiseKelas(vm.galatTipe,
                         "tupel() hanya menerima daftar, tupel, teks, rentang, atau kamus");
        return KOSONG_VAL;
    }
    // GC: the tupel and each element stay rooted while the array grows --
    // iterAt allocates a fresh string per character for teks.
    ObjTuple* tuple = newTuple();
    push(OBJ_VAL(tuple));
    for (int i = 0; i < count; i++) {
        Value element = IS_DICT(src) ? dictKeyAt(AS_DICT(src), i) : iterAt(src, i);
        push(element);
        writeValueArray(&tuple->items, element);
        pop();
    }
    pop();
    return OBJ_VAL(tuple);
}

// himpunan() / himpunan(urutan) - koleksi anggota unik (K17: satu-satunya cara
// membuat himpunan; tidak ada literal {1, 2}). Seperti set() di Python.
Value nativeFnHimpunan(int argCount, Value* args) {
    if (argCount > 1) {
        nativeRaise("himpunan() memerlukan 0 atau 1 argumen, bukan %d", argCount);
        return KOSONG_VAL;
    }
    if (argCount == 0) return OBJ_VAL(newSet());

    // Fase 5: cairkan dulu (slot argumen tetap di-root oleh stack VM).
    if (!materializeLazy(&args[0])) return KOSONG_VAL;
    Value src = args[0];
    ObjSet* set = newSet();
    int count = IS_DICT(src) ? dictLength(AS_DICT(src)) : iterCount(src);
    if (count < 0) {
        nativeRaiseKelas(vm.galatTipe,
                         "himpunan() hanya menerima daftar, tupel, himpunan, teks, rentang, atau kamus");
        return KOSONG_VAL;
    }
    // GC: the himpunan and each member stay rooted while the table grows --
    // iterAt allocates a fresh string per character for teks.
    push(OBJ_VAL(set));
    for (int i = 0; i < count; i++) {
        Value member = IS_DICT(src) ? dictKeyAt(AS_DICT(src), i) : iterAt(src, i);
        push(member);
        bool added = setAdd(set, member);
        pop();
        if (!added) {
            pop();
            nativeRaiseKelas(vm.galatTipe, "Anggota himpunan harus nilai yang dapat di-hash.");
            return KOSONG_VAL;
        }
    }
    pop();
    return OBJ_VAL(set);
}

// s.hapus(x) - buang anggota x; GalatKunci bila x bukan anggota (Python's
// set.remove raises KeyError; 'buang' is a keyword, so the method is 'hapus').
Value nativeFnHimpunanHapus(int argCount, Value* args) {
    if (argCount != 2 || !IS_SET(args[0])) {
        nativeRaise("hapus() pada himpunan memerlukan 1 argumen (anggota)");
        return KOSONG_VAL;
    }
    if (rejectFrozenSet(args[0])) return KOSONG_VAL;
    if (!dictDelete(setAsDict(AS_SET(args[0])), args[1])) {
        nativeRaiseKelas(vm.galatKunci, "Nilai itu bukan anggota himpunan.");
    }
    return KOSONG_VAL;
}

// jumlah(d[, awal]) - seperti sum(); hanya angka.
Value nativeFnJumlah(int argCount, Value* args) {
    if (argCount < 1 || argCount > 2) {
        nativeRaise("jumlah() memerlukan 1 atau 2 argumen");
        return NUMBER_VAL(0);
    }
    // Fase 5: generator/iterator kustom dicairkan dulu jadi daftar.
    if (!materializeLazy(&args[0])) return KOSONG_VAL;
    int count = iterCount(args[0]);
    if (count < 0) {
        nativeRaiseKelas(vm.galatTipe, "jumlah() hanya menerima daftar, rentang, atau teks");
        return NUMBER_VAL(0);
    }
    if (argCount == 2 && !IS_NUMBER(args[1])) {
        nativeRaiseKelas(vm.galatTipe, "jumlah() nilai awal harus angka");
        return NUMBER_VAL(0);
    }

    // sum() of bulat stays bulat (and raises GalatLuapan instead of silently
    // losing precision); one desimal turns the whole total into a desimal.
    int64_t totalBulat = 0;
    double total = 0;
    bool semuaBulat = true;
    if (argCount == 2) {
        if (IS_INT(args[1])) totalBulat = AS_INT(args[1]);
        else { semuaBulat = false; total = AS_NUMBER(args[1]); }
    }
    for (int i = 0; i < count; i++) {
        Value item = iterAt(args[0], i);
        if (!IS_NUMBER(item)) {
            nativeRaiseKelas(vm.galatTipe, "jumlah() hanya menerima angka");
            return NUMBER_VAL(0);
        }
        if (semuaBulat && IS_INT(item)) {
            int64_t baru;
            if (intAddOverflow(totalBulat, AS_INT(item), &baru)) {
                nativeRaiseKelas(vm.galatLuapan, "jumlah() melebihi jangkauan bilangan bulat");
                return NUMBER_VAL(0);
            }
            totalBulat = baru;
        } else {
            if (semuaBulat) { total = (double)totalBulat; semuaBulat = false; }
            total += AS_NUMBER(item);
        }
    }
    return semuaBulat ? INT_VAL(totalBulat) : NUMBER_VAL(total);
}

// semua(d) - seperti all(); daftar kosong -> benar.
Value nativeFnSemua(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("semua() memerlukan 1 argumen");
        return BOOL_VAL(false);
    }
    // Fase 5: generator/iterator kustom dicairkan dulu jadi daftar.
    if (!materializeLazy(&args[0])) return KOSONG_VAL;
    int count = iterCount(args[0]);
    if (count < 0) {
        nativeRaiseKelas(vm.galatTipe, "semua() hanya menerima daftar, rentang, atau teks");
        return BOOL_VAL(false);
    }
    for (int i = 0; i < count; i++) {
        if (isFalsey(iterAt(args[0], i))) return BOOL_VAL(false);
    }
    return BOOL_VAL(true);
}

// ada_yang(d) - seperti any(); daftar kosong -> salah.
Value nativeFnAdaYang(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("ada_yang() memerlukan 1 argumen");
        return BOOL_VAL(false);
    }
    // Fase 5: generator/iterator kustom dicairkan dulu jadi daftar.
    if (!materializeLazy(&args[0])) return KOSONG_VAL;
    int count = iterCount(args[0]);
    if (count < 0) {
        nativeRaiseKelas(vm.galatTipe, "ada_yang() hanya menerima daftar, rentang, atau teks");
        return BOOL_VAL(false);
    }
    for (int i = 0; i < count; i++) {
        if (!isFalsey(iterAt(args[0], i))) return BOOL_VAL(true);
    }
    return BOOL_VAL(false);
}

// qsort tidak punya parameter konteks di C11; arah urutan dititipkan di sini.
// Aman karena VM ini single-threaded dan tidak ada alokasi (apalagi callback)
// selama qsort berjalan.
static bool terurutMenurun = false;

static int bandingNilai(const void* a, const void* b) {
    const Value* x = (const Value*)a;
    const Value* y = (const Value*)b;
    int cmp;
    if (IS_NUMBER(*x)) {
        // numberCompare keeps int64 exact when both sides are bulat.
        numberCompare(*x, *y, &cmp);
    } else {
        cmp = strcmp(AS_CSTRING(*x), AS_CSTRING(*y));
        if (cmp < 0) cmp = -1; else if (cmp > 0) cmp = 1;
    }
    return terurutMenurun ? -cmp : cmp;
}

// Stable insertion sort driven by _kurang_dari_. Returns false when a
// comparison raised (its error is already pending). `hasil` stays on the VM
// stack so a GC triggered by the comparison cannot free it; values[] is re-read
// every step because that comparison may have grown the list.
static bool sortInstances(ObjList* hasil, int count, bool menurun) {
    for (int i = 1; i < count; i++) {
        // Hold the element being placed on the stack: it is briefly the only
        // reference to it while the slots below shift up.
        push(hasil->items.values[i]);
        int j = i - 1;
        while (j >= 0) {
            bool less;
            // Ascending: move values[j] up while the held element is smaller.
            // Descending: the same test with the operands swapped, so equal
            // elements never move -- that is what keeps the sort stable.
            Value a = menurun ? hasil->items.values[j] : vm.stackTop[-1];
            Value b = menurun ? vm.stackTop[-1] : hasil->items.values[j];
            int found = instanceLessFor(a, b, &less);
            if (found == 0) { pop(); return false; }           // raised
            if (found < 0) {
                pop();
                nativeRaiseKelas(vm.galatTipe,
                                 "terurut() tidak dapat membandingkan objek tanpa "
                                 "metode _kurang_dari_");
                return false;
            }
            if (!less) break;
            if (j + 1 < hasil->items.count) {
                hasil->items.values[j + 1] = hasil->items.values[j];
            }
            j--;
        }
        Value item = pop();
        if (j + 1 < hasil->items.count) hasil->items.values[j + 1] = item;
    }
    return true;
}

// terurut(d[, menurun]) - seperti sorted(); selalu daftar BARU.
Value nativeFnTerurut(int argCount, Value* args) {
    if (argCount < 1 || argCount > 2) {
        nativeRaise("terurut() memerlukan 1 atau 2 argumen");
        return KOSONG_VAL;
    }
    // Fase 5: generator/iterator kustom dicairkan dulu jadi daftar.
    if (!materializeLazy(&args[0])) return KOSONG_VAL;
    int count = iterCount(args[0]);
    if (count < 0) {
        nativeRaiseKelas(vm.galatTipe, "terurut() hanya menerima daftar, rentang, atau teks");
        return KOSONG_VAL;
    }

    ObjList* hasil = newList();
    push(OBJ_VAL(hasil));   // GC: tetap reachable selama diisi
    bool pakaiAngka = false;
    bool pakaiObjek = false;
    for (int i = 0; i < count; i++) {
        Value item = iterAt(args[0], i);
        if (i == 0) {
            pakaiObjek = IS_INSTANCE(item);
            pakaiAngka = IS_NUMBER(item);
            if (!pakaiObjek && !pakaiAngka && !IS_STRING(item)) {
                pop();
                nativeRaiseKelas(vm.galatTipe,
                                 "terurut() hanya menerima angka, teks, atau objek "
                                 "dengan _kurang_dari_");
                return KOSONG_VAL;
            }
        } else if (IS_INSTANCE(item) != pakaiObjek) {
            pop();
            nativeRaiseKelas(vm.galatTipe,
                             "terurut() tidak dapat membandingkan objek dengan "
                             "angka atau teks");
            return KOSONG_VAL;
        } else if (!pakaiObjek && (IS_NUMBER(item) != pakaiAngka ||
                                   (!pakaiAngka && !IS_STRING(item)))) {
            pop();
            // Pesan lama dipertahankan: golden galat/terurut_campur menegaskannya.
            nativeRaiseKelas(vm.galatTipe,
                             "terurut() tidak dapat membandingkan angka dengan teks");
            return KOSONG_VAL;
        }
        push(item);                                   // GC: item teks baru
        writeValueArray(&hasil->items, item);
        pop();
    }
    bool menurun = argCount == 2 && !isFalsey(args[1]);
    if (pakaiObjek) {
        // _kurang_dari_ runs Indonesiaku code, so qsort is unusable here: it
        // cannot stop at the first raised error, and a comparison that appends
        // to the list would realloc the array under qsort's feet. Insertion
        // sort stops on error, re-reads values[] every step, and is stable.
        // ponytail: O(n^2) on purpose -- correct in the face of re-entrant user
        // code beats fast. For tens of thousands of objects, move to a merge
        // sort over a copy (still stable, still realloc-safe), not back to qsort.
        if (!sortInstances(hasil, count, menurun)) {
            pop();
            return KOSONG_VAL;     // the comparison raised; error already pending
        }
    } else {
        terurutMenurun = menurun;
        if (count > 1) {
            qsort(hasil->items.values, (size_t)count, sizeof(Value), bandingNilai);
        }
    }
    pop();
    return OBJ_VAL(hasil);
}

// terbalik(d) - seperti reversed(), tetapi mengembalikan daftar baru.
Value nativeFnTerbalik(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("terbalik() memerlukan 1 argumen");
        return KOSONG_VAL;
    }
    // Fase 5: generator/iterator kustom dicairkan dulu jadi daftar.
    if (!materializeLazy(&args[0])) return KOSONG_VAL;
    int count = iterCount(args[0]);
    if (count < 0) {
        nativeRaiseKelas(vm.galatTipe, "terbalik() hanya menerima daftar, rentang, atau teks");
        return KOSONG_VAL;
    }
    ObjList* hasil = newList();
    push(OBJ_VAL(hasil));
    for (int i = count - 1; i >= 0; i--) {
        Value item = iterAt(args[0], i);
        push(item);
        writeValueArray(&hasil->items, item);
        pop();
    }
    pop();
    return OBJ_VAL(hasil);
}

// pasangkan(a, b, ...) - seperti zip(); panjang = iterable terpendek.
Value nativeFnPasangkan(int argCount, Value* args) {
    if (argCount < 2) {
        nativeRaise("pasangkan() memerlukan minimal 2 argumen");
        return KOSONG_VAL;
    }
    int shortest = -1;
    for (int a = 0; a < argCount; a++) {
        int count = iterCount(args[a]);
        if (count < 0) {
            nativeRaiseKelas(vm.galatTipe, "pasangkan() hanya menerima daftar, rentang, atau teks");
            return KOSONG_VAL;
        }
        if (shortest < 0 || count < shortest) shortest = count;
    }

    ObjList* hasil = newList();
    push(OBJ_VAL(hasil));
    for (int i = 0; i < shortest; i++) {
        ObjList* pasangan = newList();
        push(OBJ_VAL(pasangan));
        for (int a = 0; a < argCount; a++) {
            Value item = iterAt(args[a], i);
            push(item);
            writeValueArray(&pasangan->items, item);
            pop();
        }
        writeValueArray(&hasil->items, OBJ_VAL(pasangan));
        pop();
    }
    pop();
    return OBJ_VAL(hasil);
}

// peta(f, d) - seperti map(), tetapi eager: hasilnya daftar.
Value nativeFnPeta(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaise("peta() memerlukan 2 argumen (fungsi, iterable)");
        return KOSONG_VAL;
    }
    // Fase 5: generator/iterator kustom dicairkan dulu jadi daftar.
    if (!materializeLazy(&args[1])) return KOSONG_VAL;
    int count = iterCount(args[1]);
    if (count < 0) {
        nativeRaiseKelas(vm.galatTipe, "peta() argumen kedua harus daftar, rentang, atau teks");
        return KOSONG_VAL;
    }
    Value fungsi = args[0];
    ObjList* hasil = newList();
    push(OBJ_VAL(hasil));   // GC: harus di stack selama SELURUH loop callback
    for (int i = 0; i < count; i++) {
        push(fungsi);
        push(iterAt(args[1], i));
        Value nilai;
        if (!callFromNative(1, &nilai)) {
            pop();   // hasil
            return KOSONG_VAL;
        }
        push(nilai);
        writeValueArray(&hasil->items, nilai);
        pop();
    }
    pop();
    return OBJ_VAL(hasil);
}

// saring(f, d) - seperti filter(), tetapi eager: hasilnya daftar.
Value nativeFnSaring(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaise("saring() memerlukan 2 argumen (fungsi, iterable)");
        return KOSONG_VAL;
    }
    // Fase 5: generator/iterator kustom dicairkan dulu jadi daftar.
    if (!materializeLazy(&args[1])) return KOSONG_VAL;
    int count = iterCount(args[1]);
    if (count < 0) {
        nativeRaiseKelas(vm.galatTipe, "saring() argumen kedua harus daftar, rentang, atau teks");
        return KOSONG_VAL;
    }
    Value fungsi = args[0];
    ObjList* hasil = newList();
    push(OBJ_VAL(hasil));
    for (int i = 0; i < count; i++) {
        Value item = iterAt(args[1], i);
        push(item);          // GC: item teks baru harus selamat dari callback
        push(fungsi);
        push(item);
        Value keputusan;
        if (!callFromNative(1, &keputusan)) {
            pop();   // item
            pop();   // hasil
            return KOSONG_VAL;
        }
        if (!isFalsey(keputusan)) {
            writeValueArray(&hasil->items, item);
        }
        pop();       // item
    }
    pop();
    return OBJ_VAL(hasil);
}

// cetak(...) — K12: a normal function, not a statement.
// Positional arguments are printed separated by `pisah` (default " ") and
// followed by `akhir` (default "\n"). Keyword names arrive via
// vm.nativeArgNames; callValue only populates it for this native.
Value nativeFnCetak(int argCount, Value* args) {
    const char* pisah = " ";
    int pisahLen = 1;
    const char* akhir = "\n";
    int akhirLen = 1;

    int namedCount = vm.nativeArgNames == NULL
                         ? 0 : vm.nativeArgNames->items.count;
    int positional = argCount - namedCount;

    for (int i = 0; i < namedCount; i++) {
        ObjString* name = AS_STRING(vm.nativeArgNames->items.values[i]);
        Value value = args[positional + i];
        bool isPisah = strcmp(name->chars, "pisah") == 0;
        bool isAkhir = strcmp(name->chars, "akhir") == 0;
        if (!isPisah && !isAkhir) {
            nativeRaise("cetak() tidak punya parameter bernama '%s'", name->chars);
            return KOSONG_VAL;
        }
        if (!IS_STRING(value)) {
            nativeRaise("cetak(): '%s' harus teks", name->chars);
            return KOSONG_VAL;
        }
        if (isPisah) {
            pisah = AS_CSTRING(value);
            pisahLen = AS_STRING(value)->length;
        } else {
            akhir = AS_CSTRING(value);
            akhirLen = AS_STRING(value)->length;
        }
    }

    // Format then write each argument in turn, like Python's print: when a
    // _teks_ raises, the arguments before it have already been printed.
    TextBuf buf;
    initTextBuf(&buf);
    for (int i = 0; i < positional; i++) {
        buf.length = 0;
        if (!formatValue(&buf, args[i], false, true)) {
            freeTextBuf(&buf);
            return KOSONG_VAL;
        }
        if (buf.length > 0) fwrite(buf.chars, 1, (size_t)buf.length, stdout);
        if (i < positional - 1) fwrite(pisah, 1, (size_t)pisahLen, stdout);
    }
    freeTextBuf(&buf);
    fwrite(akhir, 1, (size_t)akhirLen, stdout);
    return KOSONG_VAL;
}

// === Sisa Fase 2 ============================================================

// hash(x) - bulat hash, exposing the dict-key rule to the language.
Value nativeFnHash(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("hash() memerlukan 1 argumen, bukan %d", argCount);
        return KOSONG_VAL;
    }
    uint32_t h;
    if (!hashValue(args[0], &h)) {
        nativeRaiseKelas(vm.galatTipe, "Nilai itu tidak dapat di-hash.");
        return KOSONG_VAL;
    }
    return INT_VAL((int64_t)h);
}

// turunan_dari(kelas, induk) - issubclass; a class is its own subclass.
Value nativeFnTurunanDari(int argCount, Value* args) {
    if (argCount != 2) {
        nativeRaise("turunan_dari() memerlukan 2 argumen, bukan %d", argCount);
        return BOOL_VAL(false);
    }
    if (!IS_CLASS(args[0]) || !IS_CLASS(args[1])) {
        nativeRaiseKelas(vm.galatTipe, "turunan_dari() hanya menerima kelas.");
        return BOOL_VAL(false);
    }
    for (AncestorIter it = ancestors(AS_CLASS(args[0])); ancestorNext(&it); ) {
        if (it.klass == AS_CLASS(args[1])) return BOOL_VAL(true);
    }
    return BOOL_VAL(false);
}

// ke_desimal(x) - float(). Needed since K2 made ke_angka("3") a bulat.
Value nativeFnKeDesimal(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("ke_desimal() memerlukan 1 argumen, bukan %d", argCount);
        return NUMBER_VAL(0);
    }
    Value v = args[0];
    if (IS_INT(v)) return NUMBER_VAL((double)AS_INT(v));
    if (IS_NUMBER(v)) return NUMBER_VAL(AS_NUMBER(v));
    if (IS_BOOL(v)) return NUMBER_VAL(AS_BOOL(v) ? 1.0 : 0.0);
    if (!IS_STRING(v)) {
        nativeRaiseKelas(vm.galatTipe, "ke_desimal() hanya menerima angka, logika, atau teks.");
        return NUMBER_VAL(0);
    }
    ObjString* teks = AS_STRING(v);
    char* end = NULL;
    errno = 0;
    double d = strtod(teks->chars, &end);
    // Skip trailing spaces so " 3.5 " works like Python's float().
    while (end != NULL && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end++;
    if (end == teks->chars || *end != '\0') {
        nativeRaiseKelas(vm.galatNilai, "ke_desimal() gagal: '%s' bukan angka valid", teks->chars);
        return NUMBER_VAL(0);
    }
    if (errno == ERANGE) {
        nativeRaiseKelas(vm.galatLuapan, "ke_desimal() melebihi jangkauan desimal");
        return NUMBER_VAL(0);
    }
    return NUMBER_VAL(d);
}

// daftar() / daftar(urutan) - list(). Always a fresh copy, even from a daftar.
Value nativeFnDaftar(int argCount, Value* args) {
    if (argCount > 1) {
        nativeRaise("daftar() memerlukan 0 atau 1 argumen, bukan %d", argCount);
        return KOSONG_VAL;
    }
    if (argCount == 0) return OBJ_VAL(newList());

    // Fase 5: cairkan generator/iterator kustom SEBELUM mengalokasi hasil,
    // dan simpan hasilnya di slot argumen yang sudah di-root oleh stack VM.
    if (!materializeLazy(&args[0])) return KOSONG_VAL;
    Value src = args[0];
    ObjList* list = newList();
    int count = IS_DICT(src) ? dictLength(AS_DICT(src)) : iterCount(src);
    if (count < 0) {
        nativeRaiseKelas(vm.galatTipe,
                         "daftar() hanya menerima daftar, tupel, himpunan, teks, rentang, atau kamus");
        return KOSONG_VAL;
    }
    // GC: iterAt allocates a fresh string per character, so the new daftar and
    // each element must stay rooted while the array grows.
    push(OBJ_VAL(list));
    for (int i = 0; i < count; i++) {
        Value element = IS_DICT(src) ? dictKeyAt(AS_DICT(src), i) : iterAt(src, i);
        push(element);
        writeValueArray(&list->items, element);
        pop();
    }
    pop();
    return OBJ_VAL(list);
}

// himpunan_beku() / himpunan_beku(urutan) - a hashable himpunan.
Value nativeFnHimpunanBeku(int argCount, Value* args) {
    if (argCount > 1) {
        nativeRaise("himpunan_beku() memerlukan 0 atau 1 argumen, bukan %d", argCount);
        return KOSONG_VAL;
    }
    if (argCount == 0) {
        ObjSet* kosong = newSet();
        kosong->frozen = true;
        return OBJ_VAL(kosong);
    }

    // Fase 5: cairkan dulu (slot argumen tetap di-root oleh stack VM).
    if (!materializeLazy(&args[0])) return KOSONG_VAL;
    Value src = args[0];
    ObjSet* set = newSet();
    int count = IS_DICT(src) ? dictLength(AS_DICT(src)) : iterCount(src);
    if (count < 0) {
        nativeRaiseKelas(vm.galatTipe,
                         "himpunan_beku() hanya menerima daftar, tupel, himpunan, teks, rentang, atau kamus");
        return KOSONG_VAL;
    }
    push(OBJ_VAL(set));
    for (int i = 0; i < count; i++) {
        Value member = IS_DICT(src) ? dictKeyAt(AS_DICT(src), i) : iterAt(src, i);
        push(member);
        bool added = setAdd(set, member);
        pop();
        if (!added) {
            pop();
            nativeRaiseKelas(vm.galatTipe, "Anggota himpunan harus nilai yang dapat di-hash.");
            return KOSONG_VAL;
        }
    }
    pop();
    // Freeze only after filling: setAdd refuses a frozen himpunan.
    set->frozen = true;
    return OBJ_VAL(set);
}

// iter(urutan) - an explicit cursor, pairing with lanjutkan().
Value nativeFnIter(int argCount, Value* args) {
    if (argCount != 1) {
        nativeRaise("iter() memerlukan 1 argumen, bukan %d", argCount);
        return KOSONG_VAL;
    }
    Value src = args[0];
    if (IS_ITERATOR(src)) return src;   // iter(iter(x)) is iter(x), as in Python
    if (IS_GENERATOR(src)) return OBJ_VAL(newIterator(src));   // already lazy
    if (IS_INSTANCE(src)) {
        // K19/Fase 5: _iter_() runs ONCE here, exactly as OP_ITER_OBJEK does
        // before a loop. Whatever it returns is iterated: a custom iterator
        // with _berikutnya_(), or an ordinary collection.
        Value iterated = src;
        int found = instanceIterFor(src, &iterated);
        if (found == 0) return KOSONG_VAL;          // _iter_ raised
        if (found < 0) iterated = src;              // no _iter_: try _berikutnya_
        if (IS_ITERATOR(iterated)) return iterated;
        if (IS_INSTANCE(iterated)) {
            if (!instanceHasMagic(iterated, MAGIC_BERIKUTNYA)) {
                nativeRaiseKelas(vm.galatTipe,
                                 "iter() memerlukan objek dengan metode _berikutnya_().");
                return KOSONG_VAL;
            }
            return OBJ_VAL(newIterator(iterated));
        }
        src = iterated;
    }
    if (!IS_DICT(src) && iterCount(src) < 0) {
        nativeRaiseKelas(vm.galatTipe,
                         "iter() hanya menerima daftar, tupel, himpunan, teks, rentang, kamus, "
                         "atau objek dengan _iter_()/_berikutnya_()");
        return KOSONG_VAL;
    }
    return OBJ_VAL(newIterator(src));
}

// lanjutkan(it) / lanjutkan(it, bawaan) - next(). Named lanjutkan because
// `lanjut` is already the continue keyword.
Value nativeFnLanjutkan(int argCount, Value* args) {
    if (argCount < 1 || argCount > 2) {
        nativeRaise("lanjutkan() memerlukan 1 atau 2 argumen, bukan %d", argCount);
        return KOSONG_VAL;
    }
    if (!IS_ITERATOR(args[0])) {
        nativeRaiseKelas(vm.galatTipe, "lanjutkan() memerlukan iterator dari iter().");
        return KOSONG_VAL;
    }
    ObjIterator* it = AS_ITERATOR(args[0]);
    Value src = it->source;
    if (IS_GENERATOR(src)) {
        Value value;
        int step = resumeGenerator(AS_GENERATOR(src), &value);
        if (step < 0) return KOSONG_VAL;          // raised; propagate
        if (step == 0) {                          // exhausted
            if (argCount == 2) return args[1];
            nativeRaiseKelas(vm.hentiIterasi, "Iterasi selesai.");
            return KOSONG_VAL;
        }
        it->index++;
        return value;
    }
    if (IS_INSTANCE(src)) {
        // Custom iterator: _berikutnya_() drives it, HentiIterasi ends it.
        // index is unused here -- the object owns its own position.
        Value value;
        int found = instanceNextFor(src, &value);
        if (found < 0) {
            nativeRaiseKelas(vm.galatTipe,
                             "Iterator objek kehilangan metode _berikutnya_().");
            return KOSONG_VAL;
        }
        if (found == 0) {
            // It raised. A HentiIterasi means "exhausted": with a default
            // argument that is not an error, so swallow it and return it.
            if (argCount == 2 && vm.nativeErrorPending &&
                valueIsInstanceOf(vm.nativeErrorValue, OBJ_VAL(vm.hentiIterasi))) {
                vm.nativeErrorPending = false;
                vm.nativeErrorValue = KOSONG_VAL;
                return args[1];
            }
            return KOSONG_VAL;      // any other error propagates unchanged
        }
        it->index++;                // keep a count for debugging/introspection
        return value;
    }
    // Re-read the length every call: mutating the source mid-iteration ends it
    // early instead of reading out of bounds.
    int count = IS_DICT(src) ? dictLength(AS_DICT(src)) : iterCount(src);
    if (count < 0 || it->index >= count) {
        if (argCount == 2) return args[1];
        nativeRaiseKelas(vm.hentiIterasi, "Iterator sudah habis.");
        return KOSONG_VAL;
    }
    int i = it->index++;
    return IS_DICT(src) ? dictKeyAt(AS_DICT(src), i) : iterAt(src, i);
}

// ============================================================================
// HASH KRIPTOGRAFIS (Fase 6): md5, sha1, sha256, sha512
//
// Ditulis di C (vendored, seperti SQLite) karena harus akurat bit-per-bit dan
// cepat; versi .idk terlalu lambat dan rawan. Semuanya mengembalikan TEKS HEKSA
// huruf kecil, menerima teks (byte UTF-8 apa adanya) ATAU daftar/tupel byte
// 0..255 (lihat `pustaka/bita.idk`). Tidak ada tipe objek baru: hasilnya teks,
// jadi tidak menyentuh GC, `value.c`, maupun tabel metode.
// ============================================================================

static uint32_t rotl32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

static uint32_t rotr32(uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

static uint64_t rotr64(uint64_t x, int n) {
    return (x >> n) | (x << (64 - n));
}

// --- MD5 (RFC 1321) --------------------------------------------------------

static void md5Transform(uint32_t st[4], const uint8_t blk[64]) {
    static const uint32_t K[64] = {
        0xd76aa478u, 0xe8c7b756u, 0x242070dbu, 0xc1bdceeeu,
        0xf57c0fafu, 0x4787c62au, 0xa8304613u, 0xfd469501u,
        0x698098d8u, 0x8b44f7afu, 0xffff5bb1u, 0x895cd7beu,
        0x6b901122u, 0xfd987193u, 0xa679438eu, 0x49b40821u,
        0xf61e2562u, 0xc040b340u, 0x265e5a51u, 0xe9b6c7aau,
        0xd62f105du, 0x02441453u, 0xd8a1e681u, 0xe7d3fbc8u,
        0x21e1cde6u, 0xc33707d6u, 0xf4d50d87u, 0x455a14edu,
        0xa9e3e905u, 0xfcefa3f8u, 0x676f02d9u, 0x8d2a4c8au,
        0xfffa3942u, 0x8771f681u, 0x6d9d6122u, 0xfde5380cu,
        0xa4beea44u, 0x4bdecfa9u, 0xf6bb4b60u, 0xbebfbc70u,
        0x289b7ec6u, 0xeaa127fau, 0xd4ef3085u, 0x04881d05u,
        0xd9d4d039u, 0xe6db99e5u, 0x1fa27cf8u, 0xc4ac5665u,
        0xf4292244u, 0x432aff97u, 0xab9423a7u, 0xfc93a039u,
        0x655b59c3u, 0x8f0ccc92u, 0xffeff47du, 0x85845dd1u,
        0x6fa87e4fu, 0xfe2ce6e0u, 0xa3014314u, 0x4e0811a1u,
        0xf7537e82u, 0xbd3af235u, 0x2ad7d2bbu, 0xeb86d391u
    };
    static const uint8_t S[64] = {
        7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
        5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
        4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
        6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
    };
    uint32_t M[16];
    for (int i = 0; i < 16; i++) {
        M[i] = (uint32_t)blk[i * 4] | ((uint32_t)blk[i * 4 + 1] << 8) |
               ((uint32_t)blk[i * 4 + 2] << 16) | ((uint32_t)blk[i * 4 + 3] << 24);
    }
    uint32_t a = st[0], b = st[1], c = st[2], d = st[3];
    for (int i = 0; i < 64; i++) {
        uint32_t f;
        int g;
        if (i < 16)       { f = (b & c) | (~b & d); g = i; }
        else if (i < 32)  { f = (d & b) | (~d & c); g = (5 * i + 1) % 16; }
        else if (i < 48)  { f = b ^ c ^ d;          g = (3 * i + 5) % 16; }
        else              { f = c ^ (b | ~d);       g = (7 * i) % 16; }
        uint32_t tmp = d;
        d = c;
        c = b;
        b = b + rotl32(a + f + K[i] + M[g], S[i]);
        a = tmp;
    }
    st[0] += a;
    st[1] += b;
    st[2] += c;
    st[3] += d;
}

static void md5Digest(const uint8_t* data, size_t len, uint8_t out[16]) {
    uint32_t st[4] = { 0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u };
    size_t off = 0;
    uint8_t blk[64];
    while (len - off >= 64) {
        md5Transform(st, data + off);
        off += 64;
    }
    size_t rem = len - off;
    memset(blk, 0, 64);
    memcpy(blk, data + off, rem);
    blk[rem] = 0x80;
    if (rem >= 56) {
        md5Transform(st, blk);
        memset(blk, 0, 64);
    }
    uint64_t bits = (uint64_t)len * 8;
    for (int i = 0; i < 8; i++) blk[56 + i] = (uint8_t)(bits >> (8 * i));
    md5Transform(st, blk);
    for (int i = 0; i < 4; i++) {
        out[i * 4]     = (uint8_t)(st[i]);
        out[i * 4 + 1] = (uint8_t)(st[i] >> 8);
        out[i * 4 + 2] = (uint8_t)(st[i] >> 16);
        out[i * 4 + 3] = (uint8_t)(st[i] >> 24);
    }
}

// --- SHA-1 (FIPS 180-4) ----------------------------------------------------

static void sha1Transform(uint32_t st[5], const uint8_t blk[64]) {
    uint32_t w[80];
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)blk[i * 4] << 24) | ((uint32_t)blk[i * 4 + 1] << 16) |
               ((uint32_t)blk[i * 4 + 2] << 8) | (uint32_t)blk[i * 4 + 3];
    }
    for (int i = 16; i < 80; i++) {
        w[i] = rotl32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }
    uint32_t a = st[0], b = st[1], c = st[2], d = st[3], e = st[4];
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20)      { f = (b & c) | (~b & d);         k = 0x5a827999u; }
        else if (i < 40) { f = b ^ c ^ d;                   k = 0x6ed9eba1u; }
        else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8f1bbcdcu; }
        else             { f = b ^ c ^ d;                   k = 0xca62c1d6u; }
        uint32_t tmp = rotl32(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rotl32(b, 30);
        b = a;
        a = tmp;
    }
    st[0] += a;
    st[1] += b;
    st[2] += c;
    st[3] += d;
    st[4] += e;
}

static void sha1Digest(const uint8_t* data, size_t len, uint8_t out[20]) {
    uint32_t st[5] = { 0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u, 0xc3d2e1f0u };
    size_t off = 0;
    uint8_t blk[64];
    while (len - off >= 64) {
        sha1Transform(st, data + off);
        off += 64;
    }
    size_t rem = len - off;
    memset(blk, 0, 64);
    memcpy(blk, data + off, rem);
    blk[rem] = 0x80;
    if (rem >= 56) {
        sha1Transform(st, blk);
        memset(blk, 0, 64);
    }
    uint64_t bits = (uint64_t)len * 8;
    for (int i = 0; i < 8; i++) blk[63 - i] = (uint8_t)(bits >> (8 * i));
    sha1Transform(st, blk);
    for (int i = 0; i < 5; i++) {
        out[i * 4]     = (uint8_t)(st[i] >> 24);
        out[i * 4 + 1] = (uint8_t)(st[i] >> 16);
        out[i * 4 + 2] = (uint8_t)(st[i] >> 8);
        out[i * 4 + 3] = (uint8_t)(st[i]);
    }
}

// --- SHA-256 (FIPS 180-4) --------------------------------------------------

static void sha256Transform(uint32_t st[8], const uint8_t blk[64]) {
    static const uint32_t K[64] = {
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
        0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
        0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
        0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
        0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
        0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
        0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
        0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
        0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
        0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
        0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
        0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
        0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
        0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
        0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
        0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
    };
    uint32_t w[64];
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)blk[i * 4] << 24) | ((uint32_t)blk[i * 4 + 1] << 16) |
               ((uint32_t)blk[i * 4 + 2] << 8) | (uint32_t)blk[i * 4 + 3];
    }
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint32_t a = st[0], b = st[1], c = st[2], d = st[3];
    uint32_t e = st[4], f = st[5], g = st[6], h = st[7];
    for (int i = 0; i < 64; i++) {
        uint32_t S1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t t1 = h + S1 + ch + K[i] + w[i];
        uint32_t S0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = S0 + maj;
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    st[0] += a;
    st[1] += b;
    st[2] += c;
    st[3] += d;
    st[4] += e;
    st[5] += f;
    st[6] += g;
    st[7] += h;
}

static void sha256Digest(const uint8_t* data, size_t len, uint8_t out[32]) {
    uint32_t st[8] = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
    };
    size_t off = 0;
    uint8_t blk[64];
    while (len - off >= 64) {
        sha256Transform(st, data + off);
        off += 64;
    }
    size_t rem = len - off;
    memset(blk, 0, 64);
    memcpy(blk, data + off, rem);
    blk[rem] = 0x80;
    if (rem >= 56) {
        sha256Transform(st, blk);
        memset(blk, 0, 64);
    }
    uint64_t bits = (uint64_t)len * 8;
    for (int i = 0; i < 8; i++) blk[63 - i] = (uint8_t)(bits >> (8 * i));
    sha256Transform(st, blk);
    for (int i = 0; i < 8; i++) {
        out[i * 4]     = (uint8_t)(st[i] >> 24);
        out[i * 4 + 1] = (uint8_t)(st[i] >> 16);
        out[i * 4 + 2] = (uint8_t)(st[i] >> 8);
        out[i * 4 + 3] = (uint8_t)(st[i]);
    }
}

// --- SHA-512 (FIPS 180-4) --------------------------------------------------

static void sha512Transform(uint64_t st[8], const uint8_t blk[128]) {
    static const uint64_t K[80] = {
        0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
        0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
        0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
        0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
        0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
        0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
        0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
        0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
        0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
        0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
        0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
        0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
        0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
        0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
        0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
        0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
        0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
        0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
        0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
        0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
    };
    uint64_t w[80];
    for (int i = 0; i < 16; i++) {
        w[i] = 0;
        for (int j = 0; j < 8; j++) {
            w[i] = (w[i] << 8) | (uint64_t)blk[i * 8 + j];
        }
    }
    for (int i = 16; i < 80; i++) {
        uint64_t s0 = rotr64(w[i - 15], 1) ^ rotr64(w[i - 15], 8) ^ (w[i - 15] >> 7);
        uint64_t s1 = rotr64(w[i - 2], 19) ^ rotr64(w[i - 2], 61) ^ (w[i - 2] >> 6);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint64_t a = st[0], b = st[1], c = st[2], d = st[3];
    uint64_t e = st[4], f = st[5], g = st[6], h = st[7];
    for (int i = 0; i < 80; i++) {
        uint64_t S1 = rotr64(e, 14) ^ rotr64(e, 18) ^ rotr64(e, 41);
        uint64_t ch = (e & f) ^ (~e & g);
        uint64_t t1 = h + S1 + ch + K[i] + w[i];
        uint64_t S0 = rotr64(a, 28) ^ rotr64(a, 34) ^ rotr64(a, 39);
        uint64_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint64_t t2 = S0 + maj;
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    st[0] += a;
    st[1] += b;
    st[2] += c;
    st[3] += d;
    st[4] += e;
    st[5] += f;
    st[6] += g;
    st[7] += h;
}

static void sha512Digest(const uint8_t* data, size_t len, uint8_t out[64]) {
    uint64_t st[8] = {
        0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL, 0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
    };
    size_t off = 0;
    uint8_t blk[128];
    while (len - off >= 128) {
        sha512Transform(st, data + off);
        off += 128;
    }
    size_t rem = len - off;
    memset(blk, 0, 128);
    memcpy(blk, data + off, rem);
    blk[rem] = 0x80;
    if (rem >= 112) {
        sha512Transform(st, blk);
        memset(blk, 0, 128);
    }
    // Panjang bit 128-bit big-endian; input sebesar ini selalu muat di 64-bit
    // rendah, jadi 8 byte terakhir cukup dan sisanya nol.
    uint64_t bits = (uint64_t)len * 8;
    for (int i = 0; i < 8; i++) blk[127 - i] = (uint8_t)(bits >> (8 * i));
    sha512Transform(st, blk);
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            out[i * 8 + j] = (uint8_t)(st[i] >> (56 - 8 * j));
        }
    }
}

// Kumpulkan byte masukan dari teks (UTF-8 apa adanya) atau daftar/tupel byte.
// Memakai malloc/free biasa: buffer ini sementara dan tidak boleh memicu GC
// di tengah pembacaan argumen.
static bool hashInputBytes(Value v, const char* nama, uint8_t** out, size_t* outLen) {
    if (IS_STRING(v)) {
        ObjString* s = AS_STRING(v);
        size_t n = (size_t)s->length;
        uint8_t* buf = (uint8_t*)malloc(n == 0 ? 1 : n);
        if (buf == NULL) {
            nativeRaise("%s(): kehabisan memori", nama);
            return false;
        }
        memcpy(buf, s->chars, n);
        *out = buf;
        *outLen = n;
        return true;
    }
    if (IS_LIST(v) || IS_TUPLE(v)) {
        ValueArray* items = sequenceItems(v);
        size_t n = (size_t)items->count;
        uint8_t* buf = (uint8_t*)malloc(n == 0 ? 1 : n);
        if (buf == NULL) {
            nativeRaise("%s(): kehabisan memori", nama);
            return false;
        }
        for (size_t i = 0; i < n; i++) {
            Value e = items->values[i];
            if (IS_BOOL(e)) {
                buf[i] = AS_BOOL(e) ? 1 : 0;
            } else if (IS_NUMBER(e)) {
                double d = AS_NUMBER(e);
                if (d != floor(d) || d < 0 || d > 255) {
                    free(buf);
                    nativeRaiseKelas(vm.galatTipe, "%s(): byte harus bulat 0..255", nama);
                    return false;
                }
                buf[i] = (uint8_t)d;
            } else {
                free(buf);
                nativeRaiseKelas(vm.galatTipe, "%s(): elemen harus angka atau logika", nama);
                return false;
            }
        }
        *out = buf;
        *outLen = n;
        return true;
    }
    nativeRaiseKelas(vm.galatTipe, "%s() memerlukan teks atau daftar byte", nama);
    return false;
}

typedef void (*DigestFn)(const uint8_t*, size_t, uint8_t*);

static Value hashNative(int argCount, Value* args, const char* nama, DigestFn fn, size_t digestLen) {
    if (argCount != 1) {
        nativeRaise("%s() memerlukan 1 argumen", nama);
        return KOSONG_VAL;
    }
    uint8_t* buf = NULL;
    size_t n = 0;
    if (!hashInputBytes(args[0], nama, &buf, &n)) return KOSONG_VAL;
    uint8_t digest[64];
    fn(buf, n, digest);
    free(buf);
    static const char heksa[] = "0123456789abcdef";
    char hasil[129];
    for (size_t i = 0; i < digestLen; i++) {
        hasil[i * 2] = heksa[digest[i] >> 4];
        hasil[i * 2 + 1] = heksa[digest[i] & 15];
    }
    hasil[digestLen * 2] = '\0';
    return OBJ_VAL(copyString(hasil, (int)(digestLen * 2)));
}

Value nativeFnMd5(int argCount, Value* args) {
    return hashNative(argCount, args, "md5", md5Digest, 16);
}

Value nativeFnSha1(int argCount, Value* args) {
    return hashNative(argCount, args, "sha1", sha1Digest, 20);
}

Value nativeFnSha256(int argCount, Value* args) {
    return hashNative(argCount, args, "sha256", sha256Digest, 32);
}

Value nativeFnSha512(int argCount, Value* args) {
    return hashNative(argCount, args, "sha512", sha512Digest, 64);
}


// ============================================================================
// ATRIBUT DINAMIS (Fase 6): setattr / getattr / hasattr
// ============================================================================

// Look `nama` up on an instance or class: instance fields, then class
// attributes up the superclass chain, then methods (bound to an instance).
// @properti getters are NOT run: these read the stored member itself.
static bool cariAtribut(Value objek, ObjString* nama, Value* hasil) {
    ObjClass* klass = NULL;
    if (IS_INSTANCE(objek)) {
        if (tableGet(&AS_INSTANCE(objek)->fields, nama, hasil)) return true;
        klass = AS_INSTANCE(objek)->klass;
    } else if (IS_CLASS(objek)) {
        klass = AS_CLASS(objek);
    } else {
        return false;
    }
    for (ObjClass* k = klass; k != NULL; k = k->superclass) {
        if (tableGet(&k->fields, nama, hasil)) return true;
    }
    Value metode;
    if (!tableGet(&klass->methods, nama, &metode)) return false;
    if (IS_INSTANCE(objek) && IS_CLOSURE(metode)) {
        *hasil = OBJ_VAL(newBoundMethod(objek, AS_CLOSURE(metode)));
    } else {
        *hasil = metode;
    }
    return true;
}

// atur_atribut(objek, nama, nilai) -> setattr: an instance field or, for a
// class, a class attribute.
Value nativeFnAturAtribut(int argCount, Value* args) {
    if (argCount != 3 || !IS_STRING(args[1])) {
        nativeRaiseKelas(vm.galatTipe, "atur_atribut(objek, nama, nilai) memerlukan nama berupa teks");
        return KOSONG_VAL;
    }
    ObjString* nama = AS_STRING(args[1]);
    if (IS_INSTANCE(args[0])) {
        tableSet(&AS_INSTANCE(args[0])->fields, nama, args[2]);   // args are rooted
    } else if (IS_CLASS(args[0])) {
        tableSet(&AS_CLASS(args[0])->fields, nama, args[2]);
    } else {
        nativeRaiseKelas(vm.galatAtribut, "atur_atribut(): hanya objek dan kelas yang punya atribut");
    }
    return KOSONG_VAL;
}

// ambil_atribut(objek, nama[, bawaan]) -> getattr.
Value nativeFnAmbilAtribut(int argCount, Value* args) {
    if ((argCount != 2 && argCount != 3) || !IS_STRING(args[1])) {
        nativeRaiseKelas(vm.galatTipe, "ambil_atribut(objek, nama[, bawaan]) memerlukan nama berupa teks");
        return KOSONG_VAL;
    }
    Value hasil;
    if (cariAtribut(args[0], AS_STRING(args[1]), &hasil)) return hasil;
    if (argCount == 3) return args[2];
    nativeRaiseKelas(vm.galatAtribut, "Objek tidak punya atribut '%s'.", AS_CSTRING(args[1]));
    return KOSONG_VAL;
}

// punya_atribut(objek, nama) -> hasattr.
Value nativeFnPunyaAtribut(int argCount, Value* args) {
    if (argCount != 2 || !IS_STRING(args[1])) {
        nativeRaiseKelas(vm.galatTipe, "punya_atribut(objek, nama) memerlukan nama berupa teks");
        return BOOL_VAL(false);
    }
    Value hasil;
    return BOOL_VAL(cariAtribut(args[0], AS_STRING(args[1]), &hasil));
}
