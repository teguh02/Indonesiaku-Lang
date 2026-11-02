#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

//#define DEBUG_TRACE_EXECUTION

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"
#include "native.h"
#include "pola.h"
#include "sistem.h"

VM vm;

static void closeUpvalues(Value* last);
static ObjInstance* newError(ObjClass* klass, const char* message);

// Magic method names (K19), indexed by MagicKind. Interned into vm.magicNames
// at startup so OP_METHOD can recognise them by pointer.
static const char* const MAGIC_NAMES[MAGIC_COUNT] = {
    "_teks_", "_repr_", "_sama_", "_tidak_sama_",
    "_kurang_dari_", "_kurang_sama_", "_lebih_dari_", "_lebih_sama_",
    "_tambah_", "_kurangi_", "_kali_", "_bagi_", "_bagi_bulat_", "_sisa_",
    "_pangkat_", "_negatif_",
    "_panjang_", "_logika_", "_ambil_", "_atur_", "_hash_",
    "_ktambah_", "_kkurangi_", "_kkali_", "_kbagi_", "_kbagi_bulat_",
    "_ksisa_", "_kpangkat_",
    "_iter_", "_berikutnya_", "_panggil_",
    "_masuk_", "_keluar_",
};

static int formatInstanceMagic(TextBuf* out, Value instance, bool repr);
static int instanceEqualMagic(Value a, Value b);
static int instanceHashMagic(Value value, uint32_t* out);
// Single ancestor walk used by every chain lookup. Classes built with one
// parent (and the error classes defined from C) never get an mro array at
// all: their superclass chain already IS their MRO, so allocating one would
// be pure overhead. Multiple inheritance sets mro and this reads it.
//
//     for (AncestorIter it = ancestors(k); ancestorNext(&it); ) { it.klass }
AncestorIter ancestors(ObjClass* klass) {
    AncestorIter it;
    it.klass = NULL;
    it.next = klass;
    it.owner = klass;
    it.index = 0;
    return it;
}

bool ancestorNext(AncestorIter* it) {
    if (it->owner != NULL && it->owner->mroCount > 0) {
        if (it->index >= it->owner->mroCount) return false;
        it->klass = it->owner->mro[it->index++];
        return true;
    }
    if (it->next == NULL) return false;
    it->klass = it->next;
    it->next = it->next->superclass;
    return true;
}

static bool instanceMagic(Value value, MagicKind kind, Value* method);
static const char* valueTypeName(Value value);
static uint8_t specialKind(ObjClass* klass, ObjString* name);
static ObjString* unimplementedAbstract(ObjClass* klass);
static bool c3Linearize(ObjClass* klass, ObjClass** bases, int baseCount);
static bool classMethodGet(ObjClass* klass, ObjString* name, Value* out);

static Value clockNative(int argCount, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static void resetStack() {
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
    vm.tryCount = 0;
    vm.exitFrame = 0;
    vm.currentGenerator = NULL;
    vm.nativeCallDepth = 0;
}

// Print the source line for a runtime error with a caret at the column.
// Mirrors the compiler's snippet format so both kinds of error read the
// same. Silent when the source isn't available (REPL, or chunks built
// before a source was attached).
static void printRuntimeSnippet(ObjString* source, int line, int column) {
    if (source == NULL || line <= 0 || column <= 0) return;

    // Walk to the start of `line`.
    const char* p = source->chars;
    for (int current = 1; current < line; current++) {
        const char* nl = strchr(p, '\n');
        if (nl == NULL) return;          // source shorter than claimed
        p = nl + 1;
    }
    const char* lineEnd = p;
    while (*lineEnd != '\0' && *lineEnd != '\n') lineEnd++;
    if (column - 1 > lineEnd - p) return;   // column outside the line

    const char* token = p + (column - 1);
    const int kWindow = 40;
    const char* from = p;
    const char* to = lineEnd;
    bool cutLeft = false, cutRight = false;
    if (token - from > kWindow) { from = token - kWindow; cutLeft = true; }
    if (to - token > kWindow)   { to = token + kWindow;   cutRight = true; }

    fprintf(stderr, "    %s%.*s%s\n",
            cutLeft ? "…" : "", (int)(to - from), from, cutRight ? "…" : "");
    fprintf(stderr, "    %s", cutLeft ? " " : "");
    for (const char* q = from; q < token; q++) {
        fputc(*q == '\t' ? '\t' : ' ', stderr);
    }
    fputs("^\n", stderr);
}

// Print an uncaught error with its traceback and reset the VM.
static void reportError(const char* message) {
    vm.hadError = true;
    fprintf(stderr, "%s\n", message);

    // Identical consecutive frames (deep recursion) are collapsed into one
    // summary line, like Python's "[Previous line repeated N more times]".
    ObjFunction* lastFunction = NULL;
    int lastLine = -1;
    int repeated = 0;
    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        int line = function->chunk.lines[instruction];
        int column = function->chunk.columns == NULL
                         ? 0 : function->chunk.columns[instruction];
        if (function == lastFunction && line == lastLine) {
            repeated++;
            continue;
        }
        if (repeated > 0) {
            fprintf(stderr, "[baris sebelumnya diulang %d kali lagi]\n", repeated);
            repeated = 0;
        }
        lastFunction = function;
        lastLine = line;
        if (column > 0) {
            fprintf(stderr, "[baris %d, kolom %d] dalam ", line, column);
        } else {
            fprintf(stderr, "[baris %d] dalam ", line);
        }
        if (function->name == NULL) {
            fprintf(stderr, "skrip\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
        // Only the innermost frame gets a snippet: printing one per frame
        // makes deep tracebacks unreadable.
        if (i == vm.frameCount - 1) {
            printRuntimeSnippet(function->chunk.source, line, column);
        }
    }
    if (repeated > 0) {
        fprintf(stderr, "[baris sebelumnya diulang %d kali lagi]\n", repeated);
    }

    resetStack();
}

// Raise a runtime error. Inside a 'coba' block it is caught (the message
// becomes the 'kecuali' value); otherwise it is printed and aborts the run.
// Callers must still return INTERPRET_RUNTIME_ERROR so run() unwinds.
void runtimeError(const char* format, ...) {
    // A magic method that raised (e.g. _hash_ inside a dict lookup) already
    // holds the real error. The caller that got `false` back cannot tell the
    // difference and reports its own generic message; keep the original.
    if (vm.nativeErrorPending) return;
    va_list args;
    va_start(args, format);
    vsnprintf(vm.runtimeErrorMessage, sizeof(vm.runtimeErrorMessage), format, args);
    va_end(args);

    if (vm.tryCount > 0 || vm.nativeCallDepth > 0) {
        // No allocation here: the error string is created in interpret()
        // once the stack has been unwound to the handler. With
        // nativeCallDepth > 0 the pending error is picked up by
        // callFromNative() and re-raised as a native error instead.
        vm.hadError = true;
        vm.runtimeErrorPending = true;
        vm.runtimeErrorClass = vm.runtimeErrorClass != NULL ? vm.runtimeErrorClass : vm.galat;
        return;
    }
    reportError(vm.runtimeErrorMessage);
}

// An internal VM failure (corrupted stack, bad jump). Never catchable.
void fatalError(const char* format, ...) {
    char message[512];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    vm.runtimeErrorPending = false;
    reportError(message);
}

// Raise a *catchable* error from a native function. Formats a message string
// and stashes it; callValue will convert it into a throwValue() unwind after
// the native returns, so `coba/kecuali` can catch it. Unlike runtimeError,
// this does not tear down the VM.
void nativeRaise(const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    vm.nativeErrorValue = OBJ_VAL(newError(vm.galatNilai, buffer));
    vm.nativeErrorPending = true;
}

// Same as nativeRaise, but picks the error class (GalatTipe, GalatBagiNol, ...)
// so `kecuali GalatTipe` can select it.
void nativeRaiseKelas(ObjClass* klass, const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    vm.nativeErrorValue = OBJ_VAL(newError(klass, buffer));
    vm.nativeErrorPending = true;
}

// The one instance check in the VM: used by OP_IS_INSTANCE (`kecuali GalatTipe`)
// and by the builtin adalah_instansi(). Walks the MRO.
bool valueIsInstanceOf(Value value, Value klass) {
    if (!IS_INSTANCE(value) || !IS_CLASS(klass)) return false;
    ObjClass* expected = AS_CLASS(klass);
    for (AncestorIter it = ancestors(AS_INSTANCE(value)->klass);
         ancestorNext(&it); ) {
        if (it.klass == expected) return true;
    }
    return false;
}

static ObjClass* defineErrorClass(const char* name, ObjClass* superclass) {
    ObjString* className = copyString(name, (int)strlen(name));
    push(OBJ_VAL(className));
    ObjClass* klass = newClass(className);
    klass->superclass = superclass;
    push(OBJ_VAL(klass));
    tableSet(&vm.globals, className, OBJ_VAL(klass));
    pop();
    pop();
    return klass;
}

// Pull the ".pesan" field out of an error instance, or NULL when absent.
static ObjString* errorMessage(ObjInstance* instance) {
    for (int i = 0; i < instance->fields.capacity; i++) {
        Entry* entry = &instance->fields.entries[i];
        if (entry->key != NULL && strcmp(entry->key->chars, "pesan") == 0 &&
            IS_STRING(entry->value)) {
            return AS_STRING(entry->value);
        }
    }
    return NULL;
}

static ObjInstance* newError(ObjClass* klass, const char* message) {
    ObjInstance* error = newInstance(klass);
    push(OBJ_VAL(error));
    ObjString* key = copyString("pesan", 5);
    push(OBJ_VAL(key));
    ObjString* text = copyString(message, (int)strlen(message));
    push(OBJ_VAL(text));
    tableSet(&error->fields, key, OBJ_VAL(text));
    pop();
    pop();
    pop();
    return error;
}

static void defineNative(const char* name, NativeFn function) {
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    vm.hadError = false;
    vm.nativeErrorPending = false;
    vm.nativeErrorValue = KOSONG_VAL;
    vm.nativeArgNames = NULL;
    vm.runtimeErrorPending = false;

    srand((unsigned int)time(NULL));

    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;
    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;

    initTable(&vm.globals);
    initTable(&vm.strings);
    initTable(&vm.loadedModules);
    vm.mainModule = NULL;
    vm.scriptDir = NULL;

    vm.initString = NULL;
    vm.initString = copyString("init", 4);
    for (int i = 0; i < MAGIC_COUNT; i++) vm.magicNames[i] = NULL;
    for (int i = 0; i < MAGIC_COUNT; i++) {
        vm.magicNames[i] = copyString(MAGIC_NAMES[i], (int)strlen(MAGIC_NAMES[i]));
    }
    formatInstanceHook = formatInstanceMagic;
    instanceEqualHook = instanceEqualMagic;
    instanceHashHook = instanceHashMagic;
    vm.runtimeErrorClass = NULL;
    vm.galat = defineErrorClass("Galat", NULL);
    vm.galatAritmetika = defineErrorClass("GalatAritmetika", vm.galat);
    vm.galatBagiNol = defineErrorClass("GalatBagiNol", vm.galatAritmetika);
    vm.galatLuapan = defineErrorClass("GalatLuapan", vm.galatAritmetika);
    ObjClass* galatPencarian = defineErrorClass("GalatPencarian", vm.galat);
    vm.galatIndeks = defineErrorClass("GalatIndeks", galatPencarian);
    vm.galatKunci = defineErrorClass("GalatKunci", galatPencarian);
    vm.galatTipe = defineErrorClass("GalatTipe", vm.galat);
    vm.galatNilai = defineErrorClass("GalatNilai", vm.galat);
    vm.galatNama = defineErrorClass("GalatNama", vm.galat);
    vm.galatAtribut = defineErrorClass("GalatAtribut", vm.galat);
    defineErrorClass("GalatBerkas", vm.galat);
    vm.galatOS = defineErrorClass("GalatOS", vm.galat);
    vm.galatIzin = defineErrorClass("GalatIzin", vm.galatOS);
    defineErrorClass("GalatImpor", vm.galat);
    vm.galatRekursi = defineErrorClass("GalatRekursi", vm.galat);
    vm.galatPenegasan = defineErrorClass("GalatPenegasan", vm.galat);
    vm.hentiIterasi = defineErrorClass("HentiIterasi", vm.galat);
    defineErrorClass("GalatSintaks", vm.galat);

    defineNative("db_buka", nativeFnDbBuka);
    defineNative("db_tutup", nativeFnDbTutup);
    defineNative("db_batas_tunggu", nativeFnDbBatasTunggu);
    defineNative("db_eksekusi", nativeFnDbEksekusi);
    defineNative("db_ambil_semua", nativeFnDbAmbilSemua);
    defineNative("db_ambil_satu", nativeFnDbAmbilSatu);
    defineNative("db_baris_terpengaruh", nativeFnDbBarisTerpengaruh);
    defineNative("db_id_terakhir", nativeFnDbIdTerakhir);
    defineNative("db_mulai", nativeFnDbMulai);
    defineNative("db_komit", nativeFnDbKomit);
    defineNative("db_batal", nativeFnDbBatal);

    defineNative("jam", clockNative);
    defineNative("min", nativeFnMin);
    defineNative("max", nativeFnMax);
    defineNative("abs", nativeFnAbs);
    defineNative("akar", nativeFnAkar);
    defineNative("pangkat", nativeFnPangkat);
    defineNative("sin", nativeFnSin);
    defineNative("cos", nativeFnCos);
    defineNative("tan", nativeFnTan);
    defineNative("asin", nativeFnAsin);
    defineNative("acos", nativeFnAcos);
    defineNative("atan", nativeFnAtan);
    defineNative("atan2", nativeFnAtan2);
    defineNative("radian", nativeFnRadian);
    defineNative("derajat", nativeFnDerajat);
    defineNative("bulat", nativeFnBulat);
    defineNative("lantai", nativeFnLantai);
    defineNative("atap", nativeFnAtap);
    defineNative("acak", nativeFnAcak);

    defineNative("bulatkan", nativeFnBulatkan);
    defineNative("bagi_sisa", nativeFnBagiSisa);
    defineNative("biner", nativeFnBiner);
    defineNative("oktal", nativeFnOktal);
    defineNative("heksa", nativeFnHeksa);
    defineNative("ke_bulat", nativeFnKeBulat);
    defineNative("ke_logika", nativeFnKeLogika);
    defineNative("adalah_instansi", nativeFnAdalahInstansi);
    defineNative("turunan_dari", nativeFnTurunanDari);
    defineNative("bisa_dipanggil", nativeFnBisaDipanggil);
    defineNative("repr", nativeFnRepr);
    defineNative("id", nativeFnId);

    defineNative("cetak", nativeFnCetak);
    defineNative("panjang", nativeFnPanjang);
    defineNative("panjang_byte", nativeFnPanjangByte);
    defineNative("huruf_besar", nativeFnHurofBesar);
    defineNative("huruf_kecil", nativeFnHurofKecil);
    defineNative("ganti", nativeFnGanti);

    defineNative("tambah", nativeFnTambah);
    defineNative("hapus", nativeFnHapus);

    defineNative("kamus", nativeFnKamus);
    defineNative("punya", nativeFnPunya);
    defineNative("hapus_kunci", nativeFnHapusKunci);
    defineNative("enumerasi", nativeFnEnumerasi);
    defineNative("tupel", nativeFnTupel);
    defineNative("himpunan", nativeFnHimpunan);
    defineNative("himpunan_beku", nativeFnHimpunanBeku);
    defineNative("daftar", nativeFnDaftar);
    defineNative("hash", nativeFnHash);
    defineNative("iter", nativeFnIter);
    defineNative("lanjutkan", nativeFnLanjutkan);
    defineNative("kunci", nativeFnKunci);

    defineNative("jenis", nativeFnJenis);
    defineNative("input", nativeFnInput);
    defineNative("henti", nativeFnHenti);

    defineNative("ke_angka", nativeFnKeAngka);
    defineNative("ke_desimal", nativeFnKeDesimal);
    defineNative("ke_teks", nativeFnKeTeks);
    defineNative("format", nativeFnFormat);
    defineNative("acak_bulat", nativeFnAcakBulat);

    defineNative("buka_berkas", nativeFnBukaBerkas);
    defineNative("tutup", nativeFnTutup);
    defineNative("baca", nativeFnBaca);
    defineNative("baca_baris", nativeFnBacaBaris);
    defineNative("tulis", nativeFnTulis);
    defineNative("baca_semua", nativeFnBacaSemua);
    defineNative("tulis_semua", nativeFnTulisSemua);
    defineNative("tambah_berkas", nativeFnTambahBerkas);
    defineNative("ada_berkas", nativeFnAdaBerkas);
    defineNative("hapus_berkas", nativeFnHapusBerkas);

    defineNative("waktu", nativeFnWaktu);
    defineNative("tanggal", nativeFnTanggal);
    defineNative("format_tanggal", nativeFnFormatTanggal);

    defineNative("karakter", nativeFnKarakter);
    defineNative("potong", nativeFnPotong);
    defineNative("ord", nativeFnOrd);
    defineNative("chr", nativeFnChr);
    defineNative("hapus_pada", nativeFnHapusPada);
    defineNative("sisip", nativeFnSisip);
    defineNative("rentang", nativeFnRentang);

    defineNative("jumlah", nativeFnJumlah);
    defineNative("semua", nativeFnSemua);
    defineNative("ada_yang", nativeFnAdaYang);
    defineNative("terurut", nativeFnTerurut);
    defineNative("terbalik", nativeFnTerbalik);
    defineNative("pasangkan", nativeFnPasangkan);
    defineNative("peta", nativeFnPeta);
    defineNative("saring", nativeFnSaring);

    defineNative("soket_dengar", nativeFnSoketDengar);
    defineNative("soket_terima", nativeFnSoketTerima);
    defineNative("soket_baca", nativeFnSoketBaca);
    defineNative("soket_batas_waktu", nativeFnSoketBatasWaktu);
    defineNative("soket_tulis", nativeFnSoketTulis);
    defineNative("soket_tutup", nativeFnSoketTutup);
    defineNative("soket_sambung", nativeFnSoketSambung);   // klien; ditolak di --sandbox
    defineNative("tls_mulai", nativeFnTlsMulai);          // soket polos -> TLS terverifikasi

    // Hash kriptografis (Fase 6). Mengembalikan teks heksa huruf kecil.
    defineNative("md5", nativeFnMd5);
    defineNative("sha1", nativeFnSha1);
    defineNative("sha256", nativeFnSha256);
    defineNative("sha512", nativeFnSha512);

    // Ekspresi reguler (Fase 6 gelombang C). Internal: dibungkus oleh
    // pustaka/pola.idk, yang menambah kelas Pola/Cocokan dan GalatPola.
    defineNative("_pola_kompilasi", nativeFnPolaKompilasi);
    defineNative("_pola_cari", nativeFnPolaCari);
    defineNative("_pola_semua", nativeFnPolaSemua);

    // Sistem operasi (Fase 6 gelombang C2, src/sistem.c). lingkungan,
    // isi_direktori, dan _jalankan_proses ditolak di `indk --sandbox`.
    defineNative("keluar", nativeFnKeluar);
    defineNative("tulis_galat", nativeFnTulisGalat);
    defineNative("lingkungan", nativeFnLingkungan);
    defineNative("ambil_lingkungan", nativeFnAmbilLingkungan);
    defineNative("direktori_kini", nativeFnDirektoriKini);
    defineNative("isi_direktori", nativeFnIsiDirektori);
    defineNative("buat_direktori", nativeFnBuatDirektori);
    defineNative("hapus_direktori", nativeFnHapusDirektori);
    defineNative("jalur_indk", nativeFnJalurIndk);
    defineNative("_info_jalur", nativeFnInfoJalur);
    defineNative("_jalur_nyata", nativeFnJalurNyata);
    defineNative("_jalankan_proses", nativeFnJalankanProses);

    // Atribut dinamis (setattr/getattr/hasattr).
    defineNative("atur_atribut", nativeFnAturAtribut);
    defineNative("ambil_atribut", nativeFnAmbilAtribut);
    defineNative("punya_atribut", nativeFnPunyaAtribut);
}

void freeVM() {
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    freeTable(&vm.loadedModules);
    vm.initString = NULL;
    for (int i = 0; i < MAGIC_COUNT; i++) vm.magicNames[i] = NULL;
    freeObjects();
}

// Expose command-line arguments to the program as a global list `argumen`.
// argv[start..argc) become string elements (start skips the script path).
void defineArgs(int argc, const char* argv[], int start) {
    ObjList* list = newList();
    push(OBJ_VAL(list));  // keep reachable during allocation
    for (int i = start; i < argc; i++) {
        Value s = OBJ_VAL(copyString(argv[i], (int)strlen(argv[i])));
        push(s);
        writeValueArray(&list->items, s);
        pop();
    }
    ObjString* name = copyString("argumen", 7);
    push(OBJ_VAL(name));
    tableSet(&vm.globals, name, OBJ_VAL(list));
    pop();  // name
    pop();  // list
}

void push(Value value) {
    if (vm.stackTop >= vm.stack + STACK_MAX) {
        runtimeError("Stack overflow: maksimum kedalaman stack %d terlampaui", STACK_MAX);
        return;
    }
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    if (vm.stackTop <= vm.stack) {
        fatalError("Stack underflow: mencoba mengambil nilai dari stack kosong");
        return KOSONG_VAL;
    }
    vm.stackTop--;
    Value result = *vm.stackTop;
    return result;
}

static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

// names != NULL: the last names->items.count arguments on the stack were
// passed by name and must be moved to the slots their names select.
static bool call(ObjClosure* closure, int argCount, ObjList* names) {
    ObjFunction* fn = closure->function;
    int namedCount = names == NULL ? 0 : names->items.count;
    int positional = argCount - namedCount;

    if (argCount > fn->arity) {
        runtimeError("Harapkan paling banyak %d argumen tetapi mendapat %d.",
                     fn->arity, argCount);
        return false;
    }
    if (positional > fn->arity) {
        runtimeError("Terlalu banyak argumen posisional.");
        return false;
    }

    // Rebuild the parameter slots: positional args keep their place, keyword
    // args move to the slot their name selects, the rest get the marker that
    // tells the prologue to evaluate that parameter's default.
    Value* base = vm.stackTop - argCount;
    Value slots[UINT8_COUNT];
    for (int i = 0; i < fn->arity; i++) {
        slots[i] = i < positional ? base[i] : TAKTERISI_VAL;
    }

    for (int i = 0; i < namedCount; i++) {
        ObjString* wanted = AS_STRING(names->items.values[i]);
        int target = -1;
        for (int p = 0; p < fn->arity; p++) {
            if (fn->paramNames[p] == wanted) { target = p; break; }
        }
        if (target < 0) {
            vm.runtimeErrorClass = vm.galatTipe;
            runtimeError("Fungsi tidak punya parameter bernama '%s'.", wanted->chars);
            return false;
        }
        if (target < positional || !IS_TAKTERISI(slots[target])) {
            vm.runtimeErrorClass = vm.galatTipe;
            runtimeError("Parameter '%s' diisi lebih dari sekali.", wanted->chars);
            return false;
        }
        slots[target] = base[positional + i];
    }

    for (int i = 0; i < fn->requiredArity; i++) {
        if (IS_TAKTERISI(slots[i])) {
            vm.runtimeErrorClass = vm.galatTipe;
            runtimeError("Parameter wajib '%s' tidak diisi.", fn->paramNames[i]->chars);
            return false;
        }
    }

    if (vm.frameCount >= FRAMES_MAX) {
        vm.runtimeErrorClass = vm.galatRekursi;
        runtimeError("Call stack overflow: kedalaman maksimal panggilan fungsi %d terlampaui. Kemungkinan rekursi tak terbatas?", FRAMES_MAX);
        return false;
    }

    for (int i = 0; i < fn->arity; i++) base[i] = slots[i];
    vm.stackTop = base + fn->arity;

    // Generator: arguments are now bound exactly as for a normal call, but the
    // body must not run yet. Capture the parameter slots into the generator
    // and hand it back as the call's result.
    if (fn->isGenerator) {
        if (fn->arity + 1 > GEN_SLOTS_MAX) {
            vm.runtimeErrorClass = vm.galatTipe;
            runtimeError("Generator '%s' punya terlalu banyak parameter (maksimal %d).",
                         fn->name != NULL ? fn->name->chars : "<anonim>", GEN_SLOTS_MAX - 1);
            return false;
        }
        ObjGenerator* gen = newGenerator(closure);
        // Keep it reachable while we copy: newGenerator can trigger a GC later.
        push(OBJ_VAL(gen));
        gen->slots[0] = base[-1];                 // slot 0 is the callee itself
        for (int i = 0; i < fn->arity; i++) gen->slots[i + 1] = base[i];
        gen->slotCount = fn->arity + 1;
        pop();
        vm.stackTop = base - 1;                   // drop callee + arguments
        push(OBJ_VAL(gen));
        return true;
    }

    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = fn->chunk.code;
    frame->slots = base - 1;   // slot 0 is the callee itself
    return true;
}

// Natives that read vm.nativeArgNames. Everything else still rejects keyword
// arguments, so an unsupported `pisah=` is an error rather than a no-op.
// Levenshtein distance, capped: returns > max as soon as it's clear the
// strings are too far apart. Two rolling rows instead of a full matrix.
static int editDistance(const char* a, int alen, const char* b, int blen, int max) {
    if (abs(alen - blen) > max) return max + 1;
    if (alen > 64 || blen > 64) return max + 1;

    int prev[65], curr[65];
    for (int j = 0; j <= blen; j++) prev[j] = j;

    for (int i = 1; i <= alen; i++) {
        curr[0] = i;
        int rowBest = curr[0];
        for (int j = 1; j <= blen; j++) {
            int cost = a[i - 1] == b[j - 1] ? 0 : 1;
            int d = prev[j] + 1;
            if (curr[j - 1] + 1 < d) d = curr[j - 1] + 1;
            if (prev[j - 1] + cost < d) d = prev[j - 1] + cost;
            curr[j] = d;
            if (d < rowBest) rowBest = d;
        }
        if (rowBest > max) return max + 1;
        for (int j = 0; j <= blen; j++) prev[j] = curr[j];
    }
    return prev[blen];
}

// Closest defined global name to `name`, or NULL when nothing is close
// enough. Only runs when an error is already being raised, so the linear
// scan over the globals table costs nothing in the happy path.
static ObjString* namaTerdekat(ObjString* name) {
    // Allow at most 2 edits, and never more than a third of the name, so
    // short names don't get wild suggestions ("xyz" -> "abs").
    int max = name->length / 3;
    if (max > 2) max = 2;
    if (max < 1) return NULL;

    ObjString* best = NULL;
    int bestDistance = max + 1;
    for (int i = 0; i < vm.globals.capacity; i++) {
        Entry* entry = &vm.globals.entries[i];
        if (entry->key == NULL) continue;
        int d = editDistance(name->chars, name->length,
                             entry->key->chars, entry->key->length, max);
        if (d < bestDistance) {
            bestDistance = d;
            best = entry->key;
        }
    }
    return bestDistance <= max ? best : NULL;
}

// Raise "undefined variable", appending a suggestion when one is close.
static void undefinedVariable(ObjString* name) {
    ObjString* suggestion = namaTerdekat(name);
    if (suggestion != NULL) {
        runtimeError("Variabel tidak terdefinisi '%s'. Mungkin maksudmu '%s'?",
                     name->chars, suggestion->chars);
    } else {
        runtimeError("Variabel tidak terdefinisi '%s'.", name->chars);
    }
}

static bool nativeTakesKeywords(NativeFn fn) {
    return fn == nativeFnCetak;
}

static bool callValue(Value callee, int argCount, ObjList* names) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
                vm.stackTop[-argCount - 1] = bound->receiver;
                return call(bound->method, argCount, names);
            }
            case OBJ_CLASS: {
                ObjClass* klass = AS_CLASS(callee);
                ObjString* missing = unimplementedAbstract(klass);
                if (missing != NULL) {
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError(
                        "Kelas '%s' abstrak: metode '%s' belum diisi subkelas.",
                        klass->name->chars, missing->chars);
                    return false;
                }
                vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
                Value initializer;
                if (tableGet(&klass->methods, vm.initString, &initializer)) {
                    return call(AS_CLOSURE(initializer), argCount, names);
                } else if (argCount != 0) {
                    runtimeError("Harapkan 0 argumen tetapi mendapat %d.", argCount);
                    return false;
                }
                return true;
            }
            case OBJ_CLOSURE:
                return call(AS_CLOSURE(callee), argCount, names);
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                // Only natives that understand keywords receive them; the
                // rest keep rejecting keywords, so no silently ignored
                // `pisah=` on a function that never reads it.
                if (names != NULL && !nativeTakesKeywords(native)) {
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("Fungsi bawaan belum menerima argumen bernama.");
                    return false;
                }
                vm.nativeArgNames = names;
                Value result = native(argCount, vm.stackTop - argCount);
                vm.nativeArgNames = NULL;
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            }
            case OBJ_INSTANCE: {
                // K19: an instance with _panggil_ is callable. Replacing the
                // callee slot with the bound method means keyword arguments
                // and defaults work exactly as on any other method, and every
                // call site (OP_CALL, OP_INVOKE, natives like peta) routes
                // through here.
                Value method;
                if (instanceMagic(callee, MAGIC_PANGGIL, &method)) {
                    // Slot -argCount-1 becomes `diri` for the method, exactly
                    // like the OBJ_BOUND_METHOD case above. No allocation: the
                    // receiver is already the value sitting in that slot.
                    return call(AS_CLOSURE(method), argCount, names);
                }
                vm.runtimeErrorClass = vm.galatTipe;
                runtimeError("Objek '%s' tidak dapat dipanggil; tambahkan metode "
                             "_panggil_().", AS_INSTANCE(callee)->klass->name->chars);
                return false;
            }
            default:
                break; // Non-callable object type.
        }
    }
    vm.runtimeErrorClass = vm.galatTipe;
    runtimeError("Hanya dapat memanggil fungsi dan kelas.");
    return false;
}

// Atribut kelas, ditelusuri lewat MRO saat dibaca -- bukan disalin ketika
// OP_INHERIT. Mengubah atribut induk karena itu langsung terlihat oleh
// subkelasnya, seperti Python.
// ponytail: telusur linier tiap akses, tanpa cache. Kedalaman warisan nyata
// 2-4 tingkat, jadi lebih murah daripada invalidasi cache.
static bool classFieldGet(ObjClass* klass, ObjString* name, Value* out) {
    for (AncestorIter it = ancestors(klass); ancestorNext(&it); ) {
        if (tableGet(&it.klass->fields, name, out)) return true;
    }
    return false;
}

static bool bindMethod(ObjClass* klass, ObjString* name) {
    Value method;
    if (!tableGet(&klass->methods, name, &method)) {
        vm.runtimeErrorClass = vm.galatAtribut;   // Python: AttributeError
        runtimeError("Properti tidak terdefinisi '%s'.", name->chars);
        return false;
    }

    ObjBoundMethod* bound = newBoundMethod(peek(0), AS_CLOSURE(method));
    pop();
    push(OBJ_VAL(bound));
    return true;
}

// 'dengan' compiles to _masuk_()/_keluar_() invokes, so a manager missing one
// of them surfaces here as a bare "Metode tidak terdefinisi '_masuk_'". Say
// what the user actually wrote instead. Placed in the shared invoke paths so
// both the missing-method and the not-an-object case are covered.
static const char* contextManagerHint(ObjString* name) {
    if (name == vm.magicNames[MAGIC_MASUK] ||
        name == vm.magicNames[MAGIC_KELUAR]) {       // interned
        return "'dengan' memerlukan objek dengan metode _masuk_() dan _keluar_()";
    }
    return NULL;
}

// Replaces the receiver slot for @metode_statis / @metode_kelas. The slot is
// already there (the call pushed it), so nothing is allocated or shifted:
// statics get the closure itself, class methods get the class.
static void fixReceiverSlot(ObjClass* klass, uint8_t kind, Value method,
                            int argCount) {
    if (kind == SPECIAL_STATIS) {
        vm.stackTop[-argCount - 1] = method;
    } else if (kind == SPECIAL_KELAS) {
        vm.stackTop[-argCount - 1] = OBJ_VAL(klass);
    }
}

static bool invokeFromClass(ObjClass* klass, ObjString* name, int argCount) {
    Value method;
    if (!tableGet(&klass->methods, name, &method)) {
        vm.runtimeErrorClass = vm.galatAtribut;   // Python: AttributeError
        const char* hint = contextManagerHint(name);
        if (hint != NULL) {
            vm.runtimeErrorClass = vm.galatTipe;
            runtimeError("%s; kelas '%s' tidak punya '%s'.",
                         hint, klass->name->chars, name->chars);
        } else {
            runtimeError("Metode tidak terdefinisi '%s'.", name->chars);
        }
        return false;
    }
    fixReceiverSlot(klass, specialKind(klass, name), method, argCount);
    return call(AS_CLOSURE(method), argCount, NULL);
}

// K10: methods on built-in types. Every relevant native already takes its
// receiver as args[0] (huruf_besar(teks), tambah(daftar, nilai), ...), and
// OP_INVOKE leaves the stack as `receiver, arg1, ...` — exactly the layout a
// native expects. So a method call reuses the existing native unchanged;
// only this name->function mapping is new.
typedef struct {
    const char* name;
    NativeFn function;
} NativeMethod;

static NativeMethod stringMethods[] = {
    {"panjang", nativeFnPanjang},
    {"huruf_besar", nativeFnHurofBesar},
    {"huruf_kecil", nativeFnHurofKecil},
    {"ganti", nativeFnGanti},
    {"potong", nativeFnPotong},
    {"karakter", nativeFnKarakter},
    {"format", nativeFnFormat},
    {"ord", nativeFnOrd},
    {NULL, NULL}
};

static NativeMethod listMethods[] = {
    {"panjang", nativeFnPanjang},
    {"tambah", nativeFnTambah},
    {"hapus", nativeFnHapus},
    {"hapus_pada", nativeFnHapusPada},
    {"sisip", nativeFnSisip},
    {"terurut", nativeFnTerurut},
    {"terbalik", nativeFnTerbalik},
    {"jumlah", nativeFnJumlah},
    {"semua", nativeFnSemua},
    {"ada_yang", nativeFnAdaYang},
    {"enumerasi", nativeFnEnumerasi},
    {NULL, NULL}
};

// tupel gets only the non-mutating daftar methods: t.tambah(x) must fail
// with "tidak punya metode", like Python's AttributeError on tuple.append.
static NativeMethod tupleMethods[] = {
    {"panjang", nativeFnPanjang},
    {"terurut", nativeFnTerurut},
    {"terbalik", nativeFnTerbalik},
    {"jumlah", nativeFnJumlah},
    {"semua", nativeFnSemua},
    {"ada_yang", nativeFnAdaYang},
    {"enumerasi", nativeFnEnumerasi},
    {NULL, NULL}
};

// himpunan: tambah/hapus mutate (a himpunan is mutable, unlike tupel);
// the rest are the read-only sequence builtins.
static NativeMethod setMethods[] = {
    {"tambah", nativeFnTambah},
    {"hapus", nativeFnHimpunanHapus},
    {"panjang", nativeFnPanjang},
    {"terurut", nativeFnTerurut},
    {"jumlah", nativeFnJumlah},
    {"semua", nativeFnSemua},
    {"ada_yang", nativeFnAdaYang},
    {NULL, NULL}
};

static NativeMethod dictMethods[] = {
    {"panjang", nativeFnPanjang},
    {"punya", nativeFnPunya},
    {"hapus_kunci", nativeFnHapusKunci},
    {"kunci", nativeFnKunci},
    {"enumerasi", nativeFnEnumerasi},
    {NULL, NULL}
};

// Which table serves this receiver, and what to call the type in errors.
static NativeMethod* methodsForValue(Value receiver, const char** typeName) {
    if (IS_STRING(receiver)) { *typeName = "teks";   return stringMethods; }
    if (IS_LIST(receiver))   { *typeName = "daftar"; return listMethods; }
    if (IS_TUPLE(receiver))  { *typeName = "tupel";  return tupleMethods; }
    if (IS_SET(receiver))    { *typeName = "himpunan"; return setMethods; }
    if (IS_DICT(receiver))   { *typeName = "kamus";  return dictMethods; }
    *typeName = NULL;
    return NULL;
}

// ponytail: linear scan over ~11 entries, cheaper than hashing at this size.
// Switch to the existing Table if a profile ever says otherwise.
static NativeFn findNativeMethod(NativeMethod* table, ObjString* name) {
    for (int i = 0; table[i].name != NULL; i++) {
        if (strcmp(table[i].name, name->chars) == 0) return table[i].function;
    }
    return NULL;
}

static bool invoke(ObjString* name, int argCount) {
    Value receiver = peek(argCount);

    if (IS_MODULE(receiver)) {
        // matematika.mat_fpb(12, 18): look the member up and call it, with the
        // module slot replaced so the callee sees only its own arguments.
        Value member;
        if (!tableGet(&AS_MODULE(receiver)->globals, name, &member)) {
            vm.runtimeErrorClass = vm.galatNama;
            runtimeError("Modul '%s' tidak punya '%s'.",
                         AS_MODULE(receiver)->name->chars, name->chars);
            return false;
        }
        vm.stackTop[-argCount - 1] = member;
        return callValue(member, argCount, NULL);
    }

    if (IS_CLASS(receiver)) {
        // Lingkaran.buat(...) -- atribut kelas yang bisa dipanggil, atau metode
        // diambil lewat kelas (tanpa instance). Slot penerima diganti nilainya
        // supaya yang dipanggil hanya melihat argumennya sendiri, sama seperti
        // jalur modul di atas.
        ObjClass* klass = AS_CLASS(receiver);
        Value member;
        if (!classFieldGet(klass, name, &member) &&
            !classMethodGet(klass, name, &member)) {
            vm.runtimeErrorClass = vm.galatAtribut;
            runtimeError("Kelas '%s' tidak punya atribut '%s'.",
                         klass->name->chars, name->chars);
            return false;
        }
        uint8_t kind = specialKind(klass, name);
        if (kind == SPECIAL_PROPERTI) {
            vm.runtimeErrorClass = vm.galatAtribut;
            runtimeError("'%s' adalah @properti: baca lewat objek, bukan "
                         "lewat kelas '%s'.", name->chars, klass->name->chars);
            return false;
        }
        // @metode_kelas called through the class still needs the class in the
        // receiver slot; everything else sees only its own arguments.
        vm.stackTop[-argCount - 1] =
            (kind == SPECIAL_KELAS) ? OBJ_VAL(klass) : member;
        return (kind == SPECIAL_KELAS)
                   ? call(AS_CLOSURE(member), argCount, NULL)
                   : callValue(member, argCount, NULL);
    }

    if (!IS_INSTANCE(receiver)) {
        const char* typeName;
        NativeMethod* table = methodsForValue(receiver, &typeName);
        if (table != NULL) {
            NativeFn fn = findNativeMethod(table, name);
            if (fn == NULL) {
                vm.runtimeErrorClass = vm.galatAtribut;
                runtimeError("Tipe '%s' tidak punya metode '%s'.",
                             typeName, name->chars);
                return false;
            }
            // The receiver sits just below the arguments, so pass it as
            // args[0] — the same layout the free function already uses.
            Value* slot = vm.stackTop - argCount - 1;
            Value result = fn(argCount + 1, slot);
            vm.stackTop = slot;
            push(result);
            return true;
        }
        vm.runtimeErrorClass = vm.galatAtribut;   // Python: AttributeError
        const char* hint = contextManagerHint(name);
        if (hint != NULL) {
            vm.runtimeErrorClass = vm.galatTipe;
            runtimeError("%s, bukan '%s'.", hint, valueTypeName(receiver));
        } else {
            runtimeError("Hanya objek yang memiliki metode.");
        }
        return false;
    }

    ObjInstance* instance = AS_INSTANCE(receiver);

    // A field holding a callable takes precedence over a method.
    Value value;
    if (tableGet(&instance->fields, name, &value)) {
        vm.stackTop[-argCount - 1] = value;
        return callValue(value, argCount, NULL);
    }

    return invokeFromClass(instance->klass, name, argCount);
}

// Returns SPECIAL_* for a method reached through `klass` or any ancestor,
// or 0. Walks the chain because `special` is not copied by OP_INHERIT.
// 32, not 128: the merge worklist is a square array of pointers, so 128 would
// put 131 KB on the C stack of a function the VM calls while already deep in
// run(). 32 bases/ancestors is far past anything sane and costs 8 KB.
#define MRO_MAX 32


// C3 linearization (Python's MRO):
//     L(C) = C + merge(L(B1), ..., L(Bn), [B1, ..., Bn])
//
// merge repeatedly takes the head of the first remaining list that appears in
// no other list's TAIL. "Not in a tail" is what keeps a shared ancestor from
// being emitted before a sibling that also inherits it -- the whole reason a
// plain depth-first walk is wrong on a diamond.
//
// Fails (returns false, error already reported) when no head qualifies: that
// means the bases demand contradictory orders, and Python rejects the class
// too rather than silently picking one.
static bool c3Linearize(ObjClass* klass, ObjClass** bases, int baseCount) {
    // Worklists: the MRO of each base, then the base list itself.
    ObjClass* lists[MRO_MAX][MRO_MAX];
    int lengths[MRO_MAX];
    int listCount = baseCount + 1;
    if (listCount > MRO_MAX) {
        runtimeError("Terlalu banyak kelas induk.");
        return false;
    }

    for (int i = 0; i < baseCount; i++) {
        if (bases[i]->mroCount > MRO_MAX) {
            runtimeError("Rantai pewarisan terlalu panjang.");
            return false;
        }
        for (int j = 0; j < bases[i]->mroCount; j++) {
            lists[i][j] = bases[i]->mro[j];
        }
        lengths[i] = bases[i]->mroCount;
    }
    for (int i = 0; i < baseCount; i++) lists[baseCount][i] = bases[i];
    lengths[baseCount] = baseCount;

    ObjClass* result[MRO_MAX];
    int resultCount = 0;
    result[resultCount++] = klass;        // C itself always comes first

    for (;;) {
        bool allEmpty = true;
        for (int i = 0; i < listCount; i++) {
            if (lengths[i] > 0) { allEmpty = false; break; }
        }
        if (allEmpty) break;

        ObjClass* candidate = NULL;
        for (int i = 0; i < listCount && candidate == NULL; i++) {
            if (lengths[i] == 0) continue;
            ObjClass* head = lists[i][0];
            // Reject the head if it sits in any OTHER list's tail.
            bool inTail = false;
            for (int j = 0; j < listCount && !inTail; j++) {
                for (int k = 1; k < lengths[j]; k++) {
                    if (lists[j][k] == head) { inTail = true; break; }
                }
            }
            if (!inTail) candidate = head;
        }

        if (candidate == NULL) {
            runtimeError(
                "Urutan pewarisan '%s' tidak konsisten: tidak ada urutan "
                "yang memenuhi semua induknya.", klass->name->chars);
            return false;
        }

        if (resultCount >= MRO_MAX) {
            runtimeError("Rantai pewarisan terlalu panjang.");
            return false;
        }
        result[resultCount++] = candidate;

        // Drop the chosen class from the front of every list holding it.
        for (int i = 0; i < listCount; i++) {
            if (lengths[i] > 0 && lists[i][0] == candidate) {
                for (int k = 1; k < lengths[i]; k++) lists[i][k - 1] = lists[i][k];
                lengths[i]--;
            }
        }
    }

    klass->mro = ALLOCATE(struct ObjClass*, resultCount);
    for (int i = 0; i < resultCount; i++) klass->mro[i] = result[i];
    klass->mroCount = resultCount;
    return true;
}

// Returns the first still-unimplemented @abstrak method of `klass`, or NULL.
//
// A name is "implemented" when some class earlier in the MRO defines it as an
// ordinary method. defineMethod() already writes an explicit 0 tag when a
// subclass redefines a tagged name, so specialKind() answers exactly that --
// no separate bookkeeping, and overriding from any base works for free.
static ObjString* unimplementedAbstract(ObjClass* klass) {
    for (AncestorIter it = ancestors(klass); ancestorNext(&it); ) {
        Table* tags = &it.klass->special;
        for (int i = 0; i < tags->capacity; i++) {
            Entry* entry = &tags->entries[i];
            if (entry->key == NULL) continue;
            if ((uint8_t)AS_NUMBER(entry->value) != SPECIAL_ABSTRAK) continue;
            // Not specialKind(): that returns the FIRST tag along the MRO,
            // which is this abstract one itself. What matters is whether any
            // class in the MRO supplies a real body -- a sibling base can
            // implement what another base declares (mixin pattern), and it
            // may well sit later in the MRO.
            bool filled = false;
            for (AncestorIter f = ancestors(klass); ancestorNext(&f); ) {
                Value ignored;
                if (!tableGet(&f.klass->own, entry->key, &ignored)) continue;
                Value tag;
                bool tagged = tableGet(&f.klass->special, entry->key, &tag);
                if (!tagged || (uint8_t)AS_NUMBER(tag) != SPECIAL_ABSTRAK) {
                    filled = true;
                    break;
                }
            }
            if (!filled) return entry->key;
        }
    }
    return NULL;
}

static bool classMethodGet(ObjClass* klass, ObjString* name, Value* out) {
    for (AncestorIter it = ancestors(klass); ancestorNext(&it); ) {
        if (tableGet(&it.klass->methods, name, out)) return true;
    }
    return false;
}

static uint8_t specialKind(ObjClass* klass, ObjString* name) {
    for (AncestorIter it = ancestors(klass); ancestorNext(&it); ) {
        Value tag;
        if (tableGet(&it.klass->special, name, &tag)) return (uint8_t)AS_NUMBER(tag);  // 0 = sengaja ditimpa metode biasa
        // Cannot stop at the first class whose `methods` has the name:
        // OP_INHERIT copies the parent's methods into the subclass, so an
        // inherited @properti is present in the subclass's `methods` while
        // its tag still lives on the parent. Only a method defined directly
        // in this class body overrides the tag, and defineMethod() removes
        // the tag in that case -- so keep walking.
    }
    return 0;
}

static void defineMethod(ObjString* name) {
    Value method = peek(0);
    ObjClass* klass = AS_CLASS(peek(1));
    tableSet(&klass->methods, name, method);
    tableSet(&klass->own, name, method);   // dipakai saat meratakan ke cucu
    // A plain method here must shadow an inherited @properti/@metode_statis
    // tag. Deleting is not enough: the tag lives on the ancestor, and
    // specialKind() walks the whole chain. Record an explicit 0 to stop it.
    if (specialKind(klass, name) != 0) {
        tableSet(&klass->special, name, NUMBER_VAL(0));
    } else {
        tableDelete(&klass->special, name);
    }
    tableDelete(&klass->fields, name);   // ruang nama bersama: yang belakangan menang
    for (int i = 0; i < MAGIC_COUNT; i++) {
        if (vm.magicNames[i] == name) klass->magic |= MAGIC_BIT(i);   // interned
    }
    pop();
}

// Read an entire file into a NUL-terminated heap buffer, or NULL on failure.
static char* readModuleSource(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) return NULL;

    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    if (size < 0) { fclose(file); return NULL; }
    rewind(file);

    char* buffer = (char*)malloc((size_t)size + 1);
    if (buffer == NULL) { fclose(file); return NULL; }

    size_t read = fread(buffer, sizeof(char), (size_t)size, file);
    buffer[read] = '\0';
    normalizeNewlines(buffer, read);
    fclose(file);
    return buffer;
}

// Compile and execute another .idk file in the current global namespace.
// Modules are cached by path so re-importing is a no-op. Returns false on
// a load/compile failure (a runtime error is raised).
// Globals of the module the RUNNING function belongs to. Resolution follows
// the callee (function->module), never the caller, so a function defined in
// module A keeps reading A's globals when B calls it.
static Table* frameGlobals(CallFrame* frame) {
    ObjModule* module = frame->closure->function->module;
    return module != NULL ? &module->globals : &vm.globals;
}

// Read a global: module table first, then vm.globals (builtins + error
// classes), which stay visible from every module.
static bool getGlobalValue(CallFrame* frame, ObjString* name, Value* out) {
    if (tableGet(frameGlobals(frame), name, out)) return true;
    return tableGet(&vm.globals, name, out);
}

// --- Module loading (Fase 3) ----------------------------------------------

#ifdef _WIN32
#define INDK_PATH_SEP ';'
#define INDK_DIR_SEP '\\'
#else
#define INDK_PATH_SEP ':'
#define INDK_DIR_SEP '/'
#endif

static bool fileExists(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f == NULL) return false;
    fclose(f);
    return true;
}

// Directory part of `path`, without the trailing separator. Returns "" when
// the path has no directory component.
static void dirNameOf(const char* path, char* out, size_t outSize) {
    size_t len = strlen(path);
    size_t cut = 0;
    for (size_t i = 0; i < len; i++) {
        if (path[i] == '/' || path[i] == '\\') cut = i;
    }
    if (cut == 0) { out[0] = '\0'; return; }
    if (cut >= outSize) cut = outSize - 1;
    memcpy(out, path, cut);
    out[cut] = '\0';
}

// "a.b" -> "a/b.idk" (platform separator). Returns false when the name is
// empty or has an empty component ("a..b"), which would escape the search dir.
static bool moduleNameToRelPath(const char* name, char* out, size_t outSize) {
    size_t len = strlen(name);
    if (len == 0 || len + 5 >= outSize) return false;
    size_t j = 0;
    bool componentEmpty = true;
    for (size_t i = 0; i < len; i++) {
        if (name[i] == '.') {
            if (componentEmpty) return false;
            out[j++] = INDK_DIR_SEP;
            componentEmpty = true;
        } else {
            out[j++] = name[i];
            componentEmpty = false;
        }
    }
    if (componentEmpty) return false;
    memcpy(out + j, ".idk", 4);
    out[j + 4] = '\0';
    return true;
}

// `rel` inside each directory of INDK_PATH, in order.
static bool findInSearchPath(const char* rel, char* out, size_t outSize) {
    const char* search = getenv("INDK_PATH");
    if (search == NULL) return false;
    const char* start = search;
    while (*start != '\0') {
        const char* end = strchr(start, INDK_PATH_SEP);
        size_t partLen = end != NULL ? (size_t)(end - start) : strlen(start);
        if (partLen > 0 && partLen < 400) {
            char dir[400];
            memcpy(dir, start, partLen);
            dir[partLen] = '\0';
            if (snprintf(out, outSize, "%s%c%s", dir, INDK_DIR_SEP, rel) < (int)outSize &&
                fileExists(out)) {
                return true;
            }
        }
        if (end == NULL) break;
        start = end + 1;
    }
    return false;
}

// Standard-library directories located from the RUNNING INTERPRETER, so an
// installed indk finds pustaka/ whatever the working directory is:
//   <exe dir>/pustaka                        indk built in the repo root
//   <exe dir>/../pustaka                     Windows installer: {app}\bin + {app}\pustaka
//   <exe dir>/../share/indonesiaku/pustaka   Unix PREFIX/bin + PREFIX/share
// Computed once; the interpreter does not move while it runs.
#define INTERPRETER_ROOTS 3
static int interpreterRootCount = -1;
static char interpreterRoots[INTERPRETER_ROOTS][600];

static bool findInInterpreterRoots(const char* rel, char* out, size_t outSize) {
    if (interpreterRootCount < 0) {
        interpreterRootCount = 0;
        char exe[512];
        char dir[512];
        if (sistemJalurInterpreter(exe, sizeof exe)) {
            dirNameOf(exe, dir, sizeof dir);
            if (dir[0] != '\0') {
                const char s = INDK_DIR_SEP;
                snprintf(interpreterRoots[0], sizeof interpreterRoots[0], "%s%cpustaka", dir, s);
                snprintf(interpreterRoots[1], sizeof interpreterRoots[1], "%s%c..%cpustaka", dir, s, s);
                snprintf(interpreterRoots[2], sizeof interpreterRoots[2],
                         "%s%c..%cshare%cindonesiaku%cpustaka", dir, s, s, s, s);
                interpreterRootCount = INTERPRETER_ROOTS;
            }
        }
    }
    for (int i = 0; i < interpreterRootCount; i++) {
        if (snprintf(out, outSize, "%s%c%s", interpreterRoots[i], INDK_DIR_SEP, rel) < (int)outSize &&
            fileExists(out)) {
            return true;
        }
    }
    return false;
}

// Search order: directory of the importing file, then INDK_PATH, then
// "pustaka/" relative to the working directory, then the standard library
// next to the interpreter (findInInterpreterRoots). The first three are the
// original order, so only imports that used to FAIL can resolve differently.
static bool resolveModulePath(const char* name, const char* importerDir,
                              char* out, size_t outSize) {
    char rel[512];
    if (!moduleNameToRelPath(name, rel, sizeof rel)) return false;

    if (importerDir != NULL && importerDir[0] != '\0') {
        if (snprintf(out, outSize, "%s%c%s", importerDir, INDK_DIR_SEP, rel) < (int)outSize &&
            fileExists(out)) {
            return true;
        }
    } else if (fileExists(rel)) {
        if (snprintf(out, outSize, "%s", rel) < (int)outSize) return true;
    }

    if (findInSearchPath(rel, out, outSize)) return true;

    if (snprintf(out, outSize, "pustaka%c%s", INDK_DIR_SEP, rel) < (int)outSize &&
        fileExists(out)) {
        return true;
    }
    return findInInterpreterRoots(rel, out, outSize);
}

// Load `name` as a module and leave it on the stack. Cached by resolved path,
// so two names pointing at the same file execute once.
static bool loadModule(ObjString* name, Value* out) {
    char importerDir[512] = "";
    if (vm.frameCount > 0) {
        ObjModule* current = vm.frames[vm.frameCount - 1].closure->function->module;
        if (current != NULL && current->path != NULL) {
            dirNameOf(current->path->chars, importerDir, sizeof importerDir);
        }
    }
    if (importerDir[0] == '\0' && vm.scriptDir != NULL) {
        snprintf(importerDir, sizeof importerDir, "%s", vm.scriptDir->chars);
    }

    char resolved[512];
    if (!resolveModulePath(name->chars, importerDir, resolved, sizeof resolved)) {
        vm.runtimeErrorClass = vm.galatNama;
        runtimeError("Modul '%s' tidak ditemukan.", name->chars);
        return false;
    }

    ObjString* pathKey = copyString(resolved, (int)strlen(resolved));
    push(OBJ_VAL(pathKey));           // keep rooted across allocations
    Value cached;
    if (tableGet(&vm.loadedModules, pathKey, &cached) && IS_MODULE(cached)) {
        pop();
        *out = cached;
        return true;
    }

    char* source = readModuleSource(resolved);
    if (source == NULL) {
        pop();
        runtimeError("Tidak dapat membaca modul '%s'.", resolved);
        return false;
    }

    ObjModule* module = newModule(name, pathKey);
    push(OBJ_VAL(module));
    // Cache BEFORE running so an import cycle sees the partially filled
    // module instead of recursing forever.
    tableSet(&vm.loadedModules, pathKey, OBJ_VAL(module));
    // __nama__ is the module name here; only the entry script gets "__utama__".
    ObjString* namaKey = copyString("__nama__", 8);
    push(OBJ_VAL(namaKey));     // unrooted key + allocating tableSet = UAF
    tableSet(&module->globals, namaKey, OBJ_VAL(name));
    pop();

    ObjFunction* function = compileInModule(source, module);
    free(source);
    if (function == NULL) {
        pop(); pop();
        tableDelete(&vm.loadedModules, pathKey);
        runtimeError("Gagal mengompilasi modul '%s'.", resolved);
        return false;
    }

    push(OBJ_VAL(function));
    ObjClosure* closure = newClosure(function);
    pop();
    // Run the module body to completion right here, reusing the callback
    // machinery: it already handles frame bookkeeping, error unwinding, and
    // stack cleanup. Writing a second runner would duplicate all of that.
    push(OBJ_VAL(closure));
    Value ignored;
    if (!callFromNative(0, &ignored)) {
        // callFromNative flagged a native error; surface it as a normal raise
        // so the importing code can catch it.
        tableDelete(&vm.loadedModules, pathKey);
        pop();                        // module
        pop();                        // pathKey
        return false;
    }

    pop();                            // module
    pop();                            // pathKey
    *out = OBJ_VAL(module);
    return true;
}

static bool importModule(ObjString* path) {
    // Already loaded? Skip.
    Value dummy;
    if (tableGet(&vm.loadedModules, path, &dummy)) {
        return true;
    }

    // `impor "pustaka/x.idk"` from outside the repo: when the path does not
    // exist as written, look for x.idk in INDK_PATH and next to the
    // interpreter, like a named import. The cache key stays the path as
    // written.
    const char* file = path->chars;
    char alternatif[600];
    if (!fileExists(file) && (strncmp(file, "pustaka/", 8) == 0 || strncmp(file, "pustaka\\", 8) == 0)) {
        if (findInSearchPath(file + 8, alternatif, sizeof alternatif) ||
            findInInterpreterRoots(file + 8, alternatif, sizeof alternatif)) {
            file = alternatif;
        }
    }

    char* source = readModuleSource(file);
    if (source == NULL) {
        runtimeError("Tidak dapat mengimpor file '%s'.", path->chars);
        return false;
    }

    // Mark as loaded before running to break import cycles.
    tableSet(&vm.loadedModules, path, BOOL_VAL(true));

    ObjFunction* function = compile(source);
    free(source);
    if (function == NULL) {
        runtimeError("Gagal mengompilasi modul '%s'.", path->chars);
        return false;
    }

    push(OBJ_VAL(function));
    ObjClosure* closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    return call(closure, 0, NULL);
}

// Unwind to the nearest active 'coba' handler. Restores frame count and
// stack top to the try-block's entry state and points the current frame's
// ip at the 'kecuali' handler. Returns false if there is no handler.
// Handlers registered *below* vm.exitFrame belong to a frame that is waiting
// on a C stack frame (a native running a callback) and must not be unwound
// to directly — the native converts the error and re-raises it instead.
static bool throwValue(Value error) {
    (void)error;
    if (vm.tryCount == 0) return false;
    if (vm.tryHandlers[vm.tryCount - 1].frameCount <= vm.exitFrame) return false;

    TryHandler* handler = &vm.tryHandlers[--vm.tryCount];
    // Close any upvalues that live above the try block's stack top.
    closeUpvalues(handler->stackTop);
    vm.frameCount = handler->frameCount;
    vm.stackTop = handler->stackTop;

    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    frame->ip = handler->handlerIp;
    return true;
}

// After a call, a native may have flagged a catchable error via nativeRaise.
// Convert it into a throwValue() unwind. Returns:
//   0 = no pending error (continue normally)
//   1 = error raised and caught (a handler is now active; caller must refresh
//       its frame pointer and push the error value for 'kecuali')
//  -1 = error raised but uncaught (caller should abort with RUNTIME_ERROR)
static int resolveNativeError() {
    if (!vm.nativeErrorPending) return 0;
    vm.nativeErrorPending = false;
    Value error = vm.nativeErrorValue;
    vm.nativeErrorValue = KOSONG_VAL;

    if (!throwValue(error)) {
        if (vm.nativeCallDepth > 0) {
            // Inside a callback invoked from C: let the outer native see it.
            vm.nativeErrorValue = error;
            vm.nativeErrorPending = true;
            return -1;
        }
        if (IS_STRING(error)) {
            runtimeError("%s", AS_CSTRING(error));
        } else if (IS_INSTANCE(error)) {
            ObjString* message = errorMessage(AS_INSTANCE(error));
            if (message != NULL) {
                runtimeError("%s", message->chars);
                return -1;
            }
            runtimeError("Kesalahan tak tertangani dari fungsi bawaan.");
        } else {
            runtimeError("Kesalahan tak tertangani dari fungsi bawaan.");
        }
        return -1;
    }
    push(error);  // make the raised value available to 'kecuali'
    return 1;
}

static ObjUpvalue* captureUpvalue(Value* local) {
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = vm.openUpvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue* createdUpvalue = newUpvalue(local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == NULL) {
        vm.openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void closeUpvalues(Value* last) {
    while (vm.openUpvalues != NULL &&
           vm.openUpvalues->location >= last) {
        ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
    }
}

// Does `haystack` contain `needle` as a substring? Byte comparison is exact
// for UTF-8: a valid sequence can only match at character boundaries.
static bool stringContains(ObjString* haystack, ObjString* needle) {
    if (needle->length == 0) return true;
    for (int i = 0; i + needle->length <= haystack->length; i++) {
        if (memcmp(haystack->chars + i, needle->chars, needle->length) == 0) {
            return true;
        }
    }
    return false;
}

// `item dalam container` (Python 'in'). Returns false after raising a
// runtime error when the container type does not support membership.
// ---------------------------------------------------------------------------
// Magic methods (K19)
// ---------------------------------------------------------------------------

// Type name for operator error messages: an instance's class name, otherwise
// the Indonesian type name.
static const char* valueTypeName(Value value) {
    if (IS_INSTANCE(value)) return AS_INSTANCE(value)->klass->name->chars;
    if (IS_BOOL(value)) return "logika";
    if (IS_KOSONG(value)) return "kosong";
    if (IS_INT(value)) return "bulat";
    if (IS_NUMBER(value)) return "desimal";
    if (IS_STRING(value)) return "teks";
    if (IS_LIST(value)) return "daftar";
    if (IS_TUPLE(value)) return "tupel";
    if (IS_SET(value)) return "himpunan";
    if (IS_DICT(value)) return "kamus";
    if (IS_RANGE(value)) return "rentang";
    if (IS_CLASS(value)) return "kelas";
    if (IS_CLOSURE(value) || IS_FUNCTION(value) || IS_NATIVE(value) ||
        IS_BOUND_METHOD(value)) {
        return "fungsi";
    }
    return "objek";
}

// The magic method `kind` of an instance's class. Looked up on the class, not
// the instance's fields -- like Python looking dunders up on the type. The
// bitmask makes the miss case one AND.
static bool instanceMagic(Value value, MagicKind kind, Value* method) {
    if (!IS_INSTANCE(value)) return false;
    ObjClass* klass = AS_INSTANCE(value)->klass;
    if ((klass->magic & MAGIC_BIT(kind)) == 0) return false;
    return tableGet(&klass->methods, vm.magicNames[kind], method);
}

typedef enum {
    MAGIC_ABSENT,    // no applicable magic method; caller continues
    MAGIC_STARTED,   // a call frame was pushed; run() continues inside it
    MAGIC_FAILED     // an error was raised
} MagicDispatch;

// Operator on peek(1) (left) and peek(0) (right): left.kind(right), else
// right.mirror(left). The method is entered with a plain call() -- slot 0 is
// the receiver, slot 1 the argument -- so its return value lands exactly where
// the operator's result belongs and nothing re-enters C.
static MagicDispatch binaryMagic(MagicKind kind, MagicKind mirror) {
    Value method;
    if (instanceMagic(peek(1), kind, &method)) {
        return call(AS_CLOSURE(method), 1, NULL) ? MAGIC_STARTED : MAGIC_FAILED;
    }
    if (mirror != MAGIC_NONE && instanceMagic(peek(0), mirror, &method)) {
        Value left = peek(1);
        vm.stackTop[-2] = vm.stackTop[-1];
        vm.stackTop[-1] = left;
        return call(AS_CLOSURE(method), 1, NULL) ? MAGIC_STARTED : MAGIC_FAILED;
    }
    return MAGIC_ABSENT;
}

// binaryMagic for operators that have no fallback on instances: an instance
// operand without the method is a GalatTipe (Python: unsupported operand).
// MAGIC_ABSENT only when neither operand is an instance.
static MagicDispatch binaryMagicOrError(MagicKind kind, MagicKind mirror, const char* symbol) {
    if (!IS_INSTANCE(peek(0)) && !IS_INSTANCE(peek(1))) return MAGIC_ABSENT;
    MagicDispatch dispatch = binaryMagic(kind, mirror);
    if (dispatch != MAGIC_ABSENT) return dispatch;
    vm.runtimeErrorClass = vm.galatTipe;
    runtimeError("Operator '%s' tidak didukung antara '%s' dan '%s'.",
                 symbol, valueTypeName(peek(1)), valueTypeName(peek(0)));
    return MAGIC_FAILED;
}

// receiver.method(arg) called right now from C, for the places that cannot
// just push a frame: chainCompare, `!=` negating _sama_, and the equality and
// text hooks used deep inside C loops. An error is left pending as a native
// error; run() picks it up with resolveNativeError().
static bool callMagicNow(Value receiver, Value method, Value arg, int argCount,
                         Value* result) {
    push(receiver);                                   // GC: root before allocating
    ObjBoundMethod* bound = newBoundMethod(receiver, AS_CLOSURE(method));
    vm.stackTop[-1] = OBJ_VAL(bound);
    if (argCount == 1) push(arg);
    return callFromNative(argCount, result);
}

// Comparison through magic methods, run now: left.kind(right), else
// right.mirror(left). Returns -1 when neither defines it, 0 when the method
// raised (error pending), 1 with *result set to the truthiness of its value.
static int compareMagicNow(MagicKind kind, MagicKind mirror, Value left, Value right,
                           bool* result) {
    Value method;
    Value value;
    bool ok;
    if (instanceMagic(left, kind, &method)) {
        ok = callMagicNow(left, method, right, 1, &value);
    } else if (instanceMagic(right, mirror, &method)) {
        ok = callMagicNow(right, method, left, 1, &value);
    } else {
        return -1;
    }
    if (!ok) return 0;
    *result = !isFalsey(value);
    return 1;
}

// A no-argument magic method run right now: receiver.kind(). Returns -1 when
// the class does not define it, 0 when it raised (error pending), 1 with
// *result set to its return value.
static int magicNowNoArg(Value receiver, MagicKind kind, Value* result) {
    Value method;
    if (!instanceMagic(receiver, kind, &method)) return -1;
    return callMagicNow(receiver, method, KOSONG_VAL, 0, result) ? 1 : 0;
}

// _panjang_ as a count: must be a bulat >= 0, like Python's __len__.
// Returns -1 when undefined, 0 when it raised or returned something invalid.
int instanceLengthFor(Value value, int64_t* out) {
    Value result;
    int found = magicNowNoArg(value, MAGIC_PANJANG, &result);
    if (found <= 0) return found;
    if (!IS_INT(result)) {
        nativeRaiseKelas(vm.galatTipe, "_panjang_() harus mengembalikan bulat, bukan '%s'",
                         valueTypeName(result));
        return 0;
    }
    if (AS_INT(result) < 0) {
        nativeRaiseKelas(vm.galatNilai, "_panjang_() tidak boleh negatif (%lld)",
                         (long long)AS_INT(result));
        return 0;
    }
    *out = AS_INT(result);
    return 1;
}

// _iter_()/_berikutnya_() run now, for iter()/lanjutkan() in native.c.
// Returns 1 with *result set, 0 when the method raised (error pending),
// -1 when the class does not define it.
int instanceIterFor(Value value, Value* result) {
    return magicNowNoArg(value, MAGIC_ITER, result);
}

int instanceNextFor(Value value, Value* result) {
    return magicNowNoArg(value, MAGIC_BERIKUTNYA, result);
}

// Does this instance's class define the magic method? No user code runs.
bool instanceHasMagic(Value value, MagicKind kind) {
    Value method;
    return instanceMagic(value, kind, &method);
}

// Truthiness of an instance: _logika_ first, then _panjang_ != 0 (Python's
// order). Returns -1 when the class defines neither, and the caller keeps the
// old "an instance is always true" behaviour.
static int instanceTruthyMagic(Value value, bool* truthy) {
    Value result;
    int found = magicNowNoArg(value, MAGIC_LOGIKA, &result);
    if (found == 1) { *truthy = !isFalsey(result); return 1; }
    if (found == 0) return 0;
    int64_t length;
    found = instanceLengthFor(value, &length);
    if (found == 1) { *truthy = length != 0; return 1; }
    return found;
}

// instanceHashHook: _hash_ for dictionary keys and set members. Must return a
// bulat; its int64 is folded into the same FNV-1a space as every other key so
// that an object and a number never collide by construction.
static int instanceHashMagic(Value value, uint32_t* out) {
    if (vm.nativeErrorPending) return 0;
    Value result;
    int found = magicNowNoArg(value, MAGIC_HASH, &result);
    if (found <= 0) return found;
    if (!IS_INT(result)) {
        nativeRaiseKelas(vm.galatTipe, "_hash_() harus mengembalikan bulat, bukan '%s'",
                         valueTypeName(result));
        return 0;
    }
    // Fold through the same hashValue() path as any other bulat, so an
    // object's hash lands in the same space as every other key.
    return hashValue(result, out) ? 1 : 0;
}

// a < b through _kurang_dari_ (or b._lebih_dari_(a) as the mirror), run now.
// For terurut/min/max in native.c, which must not reimplement the lookup.
// Returns -1 when neither class defines an ordering, 0 when the method raised
// (error pending), 1 with *less set.
int instanceLessFor(Value a, Value b, bool* less) {
    if (vm.nativeErrorPending) return 0;
    return compareMagicNow(MAGIC_KURANG_DARI, MAGIC_LEBIH_DARI, a, b, less);
}

// instanceEqualHook: _sama_ for == inside collections, dalam, indeks, ...
static int instanceEqualMagic(Value a, Value b) {
    // An earlier comparison in the same C loop already raised: stop running
    // user code and let the loop's caller unwind.
    if (vm.nativeErrorPending) return 0;
    bool equal;
    int found = compareMagicNow(MAGIC_SAMA, MAGIC_SAMA, a, b, &equal);
    if (found <= 0) return found;
    return equal ? 1 : 0;
}

// formatInstanceHook: _teks_ for cetak/ke_teks/f-string (falling back to
// _repr_, as Python's object.__str__ does), _repr_ for repr and elements.
static int formatInstanceMagic(TextBuf* out, Value instance, bool repr) {
    Value method;
    MagicKind kind = MAGIC_REPR;
    if (!repr && instanceMagic(instance, MAGIC_TEKS, &method)) {
        kind = MAGIC_TEKS;
    } else if (!instanceMagic(instance, MAGIC_REPR, &method)) {
        return 0;
    }
    if (vm.nativeErrorPending) return -1;
    Value text;
    if (!callMagicNow(instance, method, KOSONG_VAL, 0, &text)) return -1;
    if (!IS_STRING(text)) {
        nativeRaiseKelas(vm.galatTipe, "%s harus mengembalikan teks, bukan %s.",
                         MAGIC_NAMES[kind], valueTypeName(text));
        return -1;
    }
    textAppend(out, AS_CSTRING(text), AS_STRING(text)->length);
    return 1;
}

// Ordering opcodes/chain ops share these: which magic method, its mirror,
// the symbol for messages, and how a three-way comparison maps to a result.
static MagicKind orderMagic(bool greater, bool orEqual) {
    if (greater) return orEqual ? MAGIC_LEBIH_SAMA : MAGIC_LEBIH_DARI;
    return orEqual ? MAGIC_KURANG_SAMA : MAGIC_KURANG_DARI;
}

static const char* orderSymbol(bool greater, bool orEqual) {
    if (greater) return orEqual ? ">=" : ">";
    return orEqual ? "<=" : "<";
}

static bool orderHolds(bool greater, bool orEqual, int cmp) {
    if (cmp == 0) return orEqual;
    return greater ? cmp > 0 : cmp < 0;
}

static bool containsValue(Value container, Value item, bool* found) {
    if (IS_SET(container)) {
        // Hash lookup. An unhashable item can never be a member; Python
        // raises TypeError for it ([1] in {1}), and so do we.
        uint32_t ignored;
        if (!hashValue(item, &ignored)) {
            // _hash_ raised: it already holds the real error, let it unwind.
            if (vm.nativeErrorPending) return false;
            vm.runtimeErrorClass = vm.galatTipe;
            runtimeError("Nilai yang tidak dapat di-hash tidak mungkin anggota himpunan.");
            return false;
        }
        Value unused;
        *found = valueTableGet(&AS_SET(container)->table, item, &unused);
        return true;
    }
    ValueArray* items = sequenceItems(container);   // daftar or tupel
    if (items != NULL) {
        *found = false;
        // count/values re-read every step: _sama_ may change the list.
        for (int i = 0; i < items->count && !*found && !vm.nativeErrorPending; i++) {
            *found = valuesEqual(items->values[i], item);
        }
        return true;
    }
    if (IS_STRING(container)) {
        if (!IS_STRING(item)) {
            runtimeError("'dalam' pada teks memerlukan teks di sebelah kiri.");
            return false;
        }
        *found = stringContains(AS_STRING(container), AS_STRING(item));
        return true;
    }
    if (IS_DICT(container)) {
        // Any hashable key; an unhashable one simply is not a key.
        Value ignored;
        *found = valueTableGet(&AS_DICT(container)->table, item, &ignored);
        return true;
    }
    if (IS_RANGE(container)) {
        *found = IS_NUMBER(item) && rangeContains(AS_RANGE(container), AS_NUMBER(item));
        return true;
    }
    runtimeError("Operand kanan 'dalam' harus daftar, tupel, himpunan, teks, kamus, atau rentang.");
    return false;
}

// Returns false on error. An error from a magic method is left pending as a
// native error (OP_CHAIN_COMPARE resolves it); others went through
// runtimeError.
static bool chainCompare(ChainCompare op, Value left, Value right, bool* result) {
    bool instance = IS_INSTANCE(left) || IS_INSTANCE(right);
    switch (op) {
        case CHAIN_EQUAL:
            if (instance) {
                int found = compareMagicNow(MAGIC_SAMA, MAGIC_SAMA, left, right, result);
                if (found == 0) return false;
                if (found < 0) *result = valuesIdentical(left, right);
                return true;
            }
            *result = valuesEqual(left, right);
            return !vm.nativeErrorPending;
        case CHAIN_IDENTITY:
            *result = valuesIdentical(left, right);   // 'adalah' stays identity
            return true;
        case CHAIN_NOT_EQUAL:
            if (instance) {
                int found = compareMagicNow(MAGIC_TIDAK_SAMA, MAGIC_TIDAK_SAMA,
                                            left, right, result);
                if (found == 0) return false;
                if (found > 0) return true;
                // No _tidak_sama_: negate _sama_ (Python's default __ne__).
                found = compareMagicNow(MAGIC_SAMA, MAGIC_SAMA, left, right, result);
                if (found == 0) return false;
                *result = found < 0 ? !valuesIdentical(left, right) : !*result;
                return true;
            }
            *result = !valuesEqual(left, right);
            return !vm.nativeErrorPending;
        case CHAIN_NOT_IDENTITY:
            *result = !valuesIdentical(left, right);
            return true;
        case CHAIN_IN:
        case CHAIN_NOT_IN:
            if (!containsValue(right, left, result)) return false;
            if (vm.nativeErrorPending) return false;   // a _sama_ raised mid-search
            if (op == CHAIN_NOT_IN) *result = !*result;
            return true;
        case CHAIN_GREATER:
        case CHAIN_GREATER_EQUAL:
        case CHAIN_LESS:
        case CHAIN_LESS_EQUAL:
        {
            bool greater = op == CHAIN_GREATER || op == CHAIN_GREATER_EQUAL;
            bool orEqual = op == CHAIN_GREATER_EQUAL || op == CHAIN_LESS_EQUAL;
            if (instance) {
                int found = compareMagicNow(orderMagic(greater, orEqual),
                                            orderMagic(!greater, orEqual),
                                            left, right, result);
                if (found >= 0) return found == 1;
                vm.runtimeErrorClass = vm.galatTipe;
                runtimeError("Operator '%s' tidak didukung antara '%s' dan '%s'.",
                             orderSymbol(greater, orEqual),
                             valueTypeName(left), valueTypeName(right));
                return false;
            }
        }
            if (IS_STRING(left) && IS_STRING(right)) {
                int cmp = strcmp(AS_STRING(left)->chars, AS_STRING(right)->chars);
                *result = op == CHAIN_GREATER ? cmp > 0 :
                          op == CHAIN_GREATER_EQUAL ? cmp >= 0 :
                          op == CHAIN_LESS ? cmp < 0 : cmp <= 0;
                return true;
            }
            if (!IS_NUMBER(left) || !IS_NUMBER(right)) {
                vm.runtimeErrorClass = vm.galatTipe;
                runtimeError("Operand harus berupa angka atau dua string.");
                return false;
            }
            {
                // Compare int64 exactly even when one side is a desimal.
                int cmp;
                if (!numberCompare(left, right, &cmp)) {
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("Operand harus berupa angka atau dua string.");
                    return false;
                }
                if (cmp == 2) { *result = false; return true; }   // NaN
                *result = op == CHAIN_GREATER ? cmp > 0 :
                          op == CHAIN_GREATER_EQUAL ? cmp >= 0 :
                          op == CHAIN_LESS ? cmp < 0 : cmp <= 0;
            }
            return true;
    }
    fatalError("Operator rantai perbandingan tidak valid.");
    return false;
}

// Python truthiness (K1): salah, kosong, 0, "" and empty list/kamus/rentang
// are false; everything else (including nan and every instance) is true.
bool isFalsey(Value value) {
    if (IS_INSTANCE(value)) {
        // K19 tahap 2: _logika_, else _panjang_ != 0. Neither defined (or the
        // method raised) leaves the old rule: an instance is true.
        bool truthy;
        if (instanceTruthyMagic(value, &truthy) == 1) return !truthy;
        return false;
    }
    switch (value.type) {
        case VAL_KOSONG: return true;
        case VAL_TAKTERISI: return true;   // unreachable; be safe, not crashy
        case VAL_BOOL:   return !AS_BOOL(value);
        case VAL_NUMBER: return AS_NUMBER(value) == 0;
        case VAL_INT:    return AS_INT(value) == 0;
        case VAL_OBJ:
            switch (OBJ_TYPE(value)) {
                case OBJ_STRING: return AS_STRING(value)->length == 0;
                case OBJ_LIST:   return AS_LIST(value)->items.count == 0;
                case OBJ_TUPLE:  return AS_TUPLE(value)->items.count == 0;
                case OBJ_SET:    return AS_SET(value)->keys.count == 0;
                case OBJ_DICT:   return dictLength(AS_DICT(value)) == 0;
                case OBJ_RANGE:  return rangeLength(AS_RANGE(value)) == 0;
                default:         return false;
            }
    }
    return false;
}

// Python slice bounds: resolve negative indices, clamp to the sequence, and
// honour the step's direction. Returns false when a bound is not a number or
// the step is 0; *msg then holds the error text.
static bool resolveSlice(Value startV, Value stopV, Value stepV, int length,
                         int* start, int* stop, int* step, const char** msg) {
    *step = 1;
    if (!IS_KOSONG(stepV)) {
        if (!IS_NUMBER(stepV)) { *msg = "Langkah irisan harus berupa angka."; return false; }
        *step = (int)AS_NUMBER(stepV);
        if (*step == 0) { *msg = "Langkah irisan tidak boleh nol."; return false; }
    }

    if (IS_KOSONG(startV)) {
        *start = *step > 0 ? 0 : length - 1;
    } else {
        if (!IS_NUMBER(startV)) { *msg = "Batas irisan harus berupa angka."; return false; }
        *start = (int)AS_NUMBER(startV);
        if (*start < 0) *start += length;
        if (*step > 0) {
            if (*start < 0) *start = 0;
            if (*start > length) *start = length;
        } else {
            if (*start < -1) *start = -1;
            if (*start > length - 1) *start = length - 1;
        }
    }

    if (IS_KOSONG(stopV)) {
        *stop = *step > 0 ? length : -1;
    } else {
        if (!IS_NUMBER(stopV)) { *msg = "Batas irisan harus berupa angka."; return false; }
        *stop = (int)AS_NUMBER(stopV);
        if (*stop < 0) *stop += length;
        if (*step > 0) {
            if (*stop < 0) *stop = 0;
            if (*stop > length) *stop = length;
        } else {
            if (*stop < -1) *stop = -1;
            if (*stop > length - 1) *stop = length - 1;
        }
    }
    return true;
}

static void concatenate() {
    // Keep operands on the stack (via peek) so a GC triggered by the
    // allocations below cannot free them mid-concatenation.
    ObjString* b = AS_STRING(peek(0));
    ObjString* a = AS_STRING(peek(1));

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    pop();
    pop();
    push(OBJ_VAL(result));
}

// daftar + daftar: a fresh list holding shallow copies of both sides.
// Both operands stay on the stack while the result grows (writeValueArray
// can allocate and trigger a GC).
// daftar + daftar -> daftar, tupel + tupel -> tupel. Callers guarantee both
// operands have the same type (mixing is a GalatTipe, as in Python).
static void concatenateLists() {
    ValueArray* b = sequenceItems(peek(0));
    ValueArray* a = sequenceItems(peek(1));
    Value result = IS_TUPLE(peek(0)) ? OBJ_VAL(newTuple()) : OBJ_VAL(newList());
    push(result);       // root while the item array grows
    ValueArray* out = sequenceItems(result);
    for (int i = 0; i < a->count; i++) writeValueArray(out, a->values[i]);
    for (int i = 0; i < b->count; i++) writeValueArray(out, b->values[i]);
    pop();              // result
    pop(); pop();       // operands
    push(result);
}

// Shared guard for 'daftar * n' and 'teks * n'. Returns false (after raising)
// when the count is not a whole number or the result would be absurdly large.
#define REPEAT_MAX 65535

static bool repeatCount(Value value, int* out) {
    if (!IS_NUMBER(value)) {
        vm.runtimeErrorClass = vm.galatTipe;
        runtimeError("Pengali harus berupa angka bulat.");
        return false;
    }
    double n = AS_NUMBER(value);
    if (n != floor(n)) {
        vm.runtimeErrorClass = vm.galatTipe;
        runtimeError("Pengali harus berupa angka bulat.");
        return false;
    }
    if (n > REPEAT_MAX) {
        vm.runtimeErrorClass = vm.galatNilai;
        runtimeError("Hasil pengulangan terlalu besar (maksimum %d).", REPEAT_MAX);
        return false;
    }
    *out = n < 0 ? 0 : (int)n;   // negative repeats give an empty result, as in Python
    return true;
}

// Operand of a bitwise operator: bulat, or logika as 0/1 (Python's bool is an
// int subclass). Anything else, desimal included, is rejected.
static bool bitwiseOperand(Value value, int64_t* out) {
    if (IS_INT(value)) { *out = AS_INT(value); return true; }
    if (IS_BOOL(value)) { *out = AS_BOOL(value) ? 1 : 0; return true; }
    return false;
}

static const char* bitwiseSymbol(uint8_t op) {
    switch (op) {
        case OP_BIT_AND:     return "&";
        case OP_BIT_OR:      return "|";
        case OP_BIT_XOR:     return "^";
        case OP_SHIFT_LEFT:  return "<<";
        case OP_SHIFT_RIGHT: return ">>";
        default:             return "?";
    }
}

// a OP b for & | ^ << >>, replacing the two operands on the stack. Dispatches
// on operand type so set union/intersection (himpunan | himpunan) can be added
// here later without new opcodes. Returns false after raising a runtime error.
// a OP b for two himpunan, replacing both on the stack with a NEW himpunan:
// | union, & intersection, - difference (OP_SUBTRACT), ^ symmetric
// difference. Order is deterministic: members of a first (in a's order),
// then the new ones from b. Operands stay rooted while the result grows.
static void setOperation(uint8_t op) {
    ObjSet* a = AS_SET(peek(1));
    ObjSet* b = AS_SET(peek(0));
    ObjSet* result = newSet();
    push(OBJ_VAL(result));
    Value ignored;
    for (int i = 0; i < a->keys.count; i++) {
        Value member = a->keys.values[i];
        bool inB = valueTableGet(&b->table, member, &ignored);
        bool keep = op == OP_BIT_OR || (op == OP_BIT_AND ? inB : !inB);
        if (keep) setAdd(result, member);
    }
    if (op == OP_BIT_OR || op == OP_BIT_XOR) {
        for (int i = 0; i < b->keys.count; i++) {
            Value member = b->keys.values[i];
            if (!valueTableGet(&a->table, member, &ignored)) setAdd(result, member);
        }
    }
    pop();              // result
    pop(); pop();       // operands
    // Frozen only when BOTH operands are, like Python's frozenset | set.
    result->frozen = a->frozen && b->frozen;
    push(OBJ_VAL(result));
}

// Set operators need two himpunan on both sides (Python rejects set | list
// too). Returns false after raising; true when the operation was done.
static bool setBinary(uint8_t op, const char* symbol) {
    if (!IS_SET(peek(0)) || !IS_SET(peek(1)) ||
        op == OP_SHIFT_LEFT || op == OP_SHIFT_RIGHT) {
        vm.runtimeErrorClass = vm.galatTipe;
        runtimeError("Operand '%s' pada himpunan harus dua himpunan.", symbol);
        return false;
    }
    setOperation(op);
    return true;
}

static bool bitwiseBinary(uint8_t op) {
    Value left = peek(1);
    Value right = peek(0);
    if (IS_SET(left) || IS_SET(right)) return setBinary(op, bitwiseSymbol(op));
    int64_t a, b;
    if (!bitwiseOperand(left, &a) || !bitwiseOperand(right, &b)) {
        vm.runtimeErrorClass = vm.galatTipe;
        runtimeError("Operand '%s' harus bilangan bulat.", bitwiseSymbol(op));
        return false;
    }

    int64_t result;
    switch (op) {
        case OP_BIT_AND: result = a & b; break;
        case OP_BIT_OR:  result = a | b; break;
        case OP_BIT_XOR: result = a ^ b; break;
        default: {  // shifts
            if (b < 0) {
                vm.runtimeErrorClass = vm.galatNilai;
                runtimeError("Jumlah geseran tidak boleh negatif.");
                return false;
            }
            if (op == OP_SHIFT_RIGHT) {
                result = intShiftRight(a, b);
            } else if (!intShiftLeft(a, b, &result)) {
                vm.runtimeErrorClass = vm.galatLuapan;
                runtimeError("Hasil geser kiri melebihi jangkauan bilangan bulat.");
                return false;
            }
            break;
        }
    }

    // logika & logika stays logika (benar & salah -> salah), as with Python's
    // bool; shifts and any mix with bulat give bulat.
    bool keepsBool = IS_BOOL(left) && IS_BOOL(right) &&
                     (op == OP_BIT_AND || op == OP_BIT_OR || op == OP_BIT_XOR);
    vm.stackTop -= 2;
    push(keepsBool ? BOOL_VAL(result != 0) : INT_VAL(result));
    return true;
}

static InterpretResult run() {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() \
    (frame->ip += 2, \
    (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() \
    (frame->closure->function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())

    for (;;) {
        // A runtime error may have been raised by push()/pop() or a native
        // function without a direct return path. Abort cleanly instead of
        // continuing to execute with a corrupted stack.
        if (vm.hadError) return INTERPRET_RUNTIME_ERROR;

#ifdef DEBUG_TRACE_EXECUTION
        int ipOffset = (int)(frame->ip - frame->closure->function->chunk.code);
        fprintf(stderr, "[LOOP START] ip=%d, frameCount=%d\n", ipOffset, vm.frameCount);
        printf("          ");
        for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(&frame->closure->function->chunk, ipOffset);
#endif

// K19: when an operand is an instance, run its magic method. A pushed frame
// means "continue there"; a failed dispatch already raised. Used directly in
// a case body (never inside a loop), so `break` leaves the switch.
#define DISPATCH_MAGIC(dispatch)                                      \
        {                                                             \
            MagicDispatch magic_ = (dispatch);                        \
            if (magic_ == MAGIC_FAILED) return INTERPRET_RUNTIME_ERROR; \
            if (magic_ == MAGIC_STARTED) {                            \
                frame = &vm.frames[vm.frameCount - 1];                \
                break;                                                \
            }                                                         \
        }
// A magic method run through callFromNative (deep inside valuesEqual,
// containsValue, chainCompare) may have raised: unwind like OP_CALL does
// after a native.
#define RESOLVE_MAGIC_ERROR()                                         \
        if (vm.nativeErrorPending) {                                  \
            if (resolveNativeError() == -1) return INTERPRET_RUNTIME_ERROR; \
            frame = &vm.frames[vm.frameCount - 1];                    \
            break;                                                    \
        }

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_KOSONG: push(KOSONG_VAL); break;
            case OP_BENAR: push(BOOL_VAL(true)); break;
            case OP_SALAH: push(BOOL_VAL(false)); break;
            case OP_POP: pop(); break;
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                // 'buang nama' on a local leaves VAL_TAKTERISI behind, so
                // reading it again is an error (Python's UnboundLocalError).
                if (IS_TAKTERISI(frame->slots[slot])) {
                    runtimeError("Variabel lokal sudah di-buang atau belum diisi.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(frame->slots[slot]);
                break;
            }
            case OP_DELETE_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = TAKTERISI_VAL;
                break;
            }
            case OP_DELETE_GLOBAL: {
                ObjString* name = READ_STRING();
                if (!tableDelete(frameGlobals(frame), name)) {
                    vm.runtimeErrorClass = vm.galatNama;
                    runtimeError("Variabel tidak terdefinisi '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_DELETE_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = TAKTERISI_VAL;
                break;
            }
            case OP_UNPACK: {
                // a, b = ... — one sequence becomes n values, first on top.
                int n = READ_BYTE();
                Value seq = peek(0);
                int length = -1;
                ValueArray* items = sequenceItems(seq);   // daftar or tupel
                if (items != NULL) length = items->count;
                else if (IS_STRING(seq)) length = AS_STRING(seq)->charCount;
                else if (IS_RANGE(seq)) length = (int)rangeLength(AS_RANGE(seq));
                if (length < 0) {
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("Hanya daftar, tupel, teks, atau rentang yang dapat dibuka menjadi beberapa nama.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (length != n) {
                    vm.runtimeErrorClass = vm.galatNilai;
                    runtimeError("Tidak dapat membuka %d nilai menjadi %d nama.", length, n);
                    return INTERPRET_RUNTIME_ERROR;
                }

                // seq stays rooted while strings allocate below.
                int base = (int)(vm.stackTop - vm.stack) - 1;
                for (int i = n - 1; i >= 0; i--) {
                    if (items != NULL) {
                        push(items->values[i]);
                    } else if (IS_STRING(seq)) {
                        ObjString* teks = AS_STRING(seq);
                        int offset = utf8ByteOffset(teks, i);
                        push(OBJ_VAL(copyString(teks->chars + offset, utf8CharBytes(teks, offset))));
                    } else {
                        push(rangeElement(AS_RANGE(seq), i));
                    }
                }
                // Drop seq, which now sits below the n elements.
                for (int i = 0; i < n; i++) vm.stack[base + i] = vm.stack[base + 1 + i];
                vm.stackTop -= 1;
                break;
            }
            case OP_INDEX_DELETE: {
                Value key = peek(0);
                Value container = peek(1);
                if (IS_TUPLE(container)) {
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("Tupel tidak dapat diubah.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (IS_LIST(container) && IS_NUMBER(key)) {
                    ObjList* list = AS_LIST(container);
                    int given = (int)AS_NUMBER(key);
                    int index = given < 0 ? given + list->items.count : given;
                    if (index < 0 || index >= list->items.count) {
                        vm.runtimeErrorClass = vm.galatIndeks;
                        runtimeError("Indeks list di luar jangkauan (indeks %d, panjang %d).",
                                     given, list->items.count);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    for (int i = index; i < list->items.count - 1; i++) {
                        list->items.values[i] = list->items.values[i + 1];
                    }
                    list->items.count--;
                } else if (IS_DICT(container)) {
                    uint32_t hash;
                    if (!hashValue(key, &hash)) {
                        if (vm.nativeErrorPending) {
                            // _hash_ raised: its error wins over the generic one.
                            if (resolveNativeError() == -1) return INTERPRET_RUNTIME_ERROR;
                            frame = &vm.frames[vm.frameCount - 1];
                            break;
                        }
                        vm.runtimeErrorClass = vm.galatTipe;
                        runtimeError("Kunci kamus harus nilai yang dapat di-hash.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    if (!dictDelete(AS_DICT(container), key)) {
                        vm.runtimeErrorClass = vm.galatKunci;
                        if (IS_STRING(key)) {
                            runtimeError("Kunci '%s' tidak ada di kamus.", AS_CSTRING(key));
                        } else {
                            runtimeError("Kunci tidak ada di kamus.");
                        }
                        return INTERPRET_RUNTIME_ERROR;
                    }
                } else {
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("'buang' elemen memerlukan daftar dengan indeks angka "
                                 "atau kamus dengan kunci teks.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vm.stackTop -= 2;
                break;
            }
            case OP_DELETE_PROPERTY: {
                if (!IS_INSTANCE(peek(0))) {
                    vm.runtimeErrorClass = vm.galatTipe;
                    vm.runtimeErrorClass = vm.galatAtribut;   // Python: AttributeError
                    runtimeError("Hanya objek yang memiliki properti.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjInstance* instance = AS_INSTANCE(peek(0));
                ObjString* name = READ_STRING();
                if (!tableDelete(&instance->fields, name)) {
                    vm.runtimeErrorClass = vm.galatAtribut;
                    runtimeError("Objek tidak punya atribut '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                pop();  // instance
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                // Don't pop - leave value on stack (assignment is an expression)
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value;
                if (!getGlobalValue(frame, name, &value)) {
                    undefinedVariable(name);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                tableSet(frameGlobals(frame), name, peek(0));
                pop();
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                Table* globals = frameGlobals(frame);
                // Assigning a name that only exists as a builtin shadows it in
                // this module rather than mutating the shared builtin table.
                if (tableSet(globals, name, peek(0))) {
                    Value existing;
                    if (!tableGet(&vm.globals, name, &existing)) {
                        tableDelete(globals, name);
                        undefinedVariable(name);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                // Don't pop - leave value on stack (assignment is an expression)
                break;
            }
            case OP_EQUAL: {
                if (IS_INSTANCE(peek(0)) || IS_INSTANCE(peek(1))) {
                    DISPATCH_MAGIC(binaryMagic(MAGIC_SAMA, MAGIC_SAMA));
                    // Neither defines _sama_: identity (K7), below.
                }
                // Operands stay on the stack: _sama_ of an element (inside
                // valuesEqual) may allocate and collect.
                bool equal = valuesEqual(peek(1), peek(0));
                RESOLVE_MAGIC_ERROR();
                vm.stackTop -= 2;
                push(BOOL_VAL(equal));
                break;
            }
            case OP_NOT_EQUAL: {
                bool equal;
                if (IS_INSTANCE(peek(0)) || IS_INSTANCE(peek(1))) {
                    DISPATCH_MAGIC(binaryMagic(MAGIC_TIDAK_SAMA, MAGIC_TIDAK_SAMA));
                    // No _tidak_sama_: negate _sama_ (Python's default __ne__),
                    // else identity.
                    int found = compareMagicNow(MAGIC_SAMA, MAGIC_SAMA,
                                                peek(1), peek(0), &equal);
                    RESOLVE_MAGIC_ERROR();
                    if (found < 0) equal = valuesIdentical(peek(1), peek(0));
                } else {
                    equal = valuesEqual(peek(1), peek(0));
                    RESOLVE_MAGIC_ERROR();
                }
                vm.stackTop -= 2;
                push(BOOL_VAL(!equal));
                break;
            }
            case OP_IDENTICAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesIdentical(a, b)));
                break;
            }
            case OP_GREATER:
            case OP_GREATER_EQUAL:
            case OP_LESS:
            case OP_LESS_EQUAL: {
                bool greater = instruction == OP_GREATER || instruction == OP_GREATER_EQUAL;
                bool orEqual = instruction == OP_GREATER_EQUAL || instruction == OP_LESS_EQUAL;
                int cmp;
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    cmp = strcmp(AS_CSTRING(peek(1)), AS_CSTRING(peek(0)));
                    vm.stackTop -= 2;
                    push(BOOL_VAL(orderHolds(greater, orEqual, cmp)));
                    break;
                }
                if (numberCompare(peek(1), peek(0), &cmp)) {
                    vm.stackTop -= 2;
                    // NaN (2) is never ordered, so >= and <= are salah too.
                    push(BOOL_VAL(cmp != 2 && orderHolds(greater, orEqual, cmp)));
                    break;
                }
                DISPATCH_MAGIC(binaryMagicOrError(orderMagic(greater, orEqual),
                                                  orderMagic(!greater, orEqual),
                                                  orderSymbol(greater, orEqual)));
                vm.runtimeErrorClass = vm.galatTipe;
                runtimeError("Operand harus berupa angka.");
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if ((IS_LIST(peek(0)) && IS_LIST(peek(1))) ||
                           (IS_TUPLE(peek(0)) && IS_TUPLE(peek(1)))) {
                    concatenateLists();
                } else if (IS_INT(peek(0)) && IS_INT(peek(1))) {
                    int64_t result;
                    if (intAddOverflow(AS_INT(peek(1)), AS_INT(peek(0)), &result)) {
                        vm.runtimeErrorClass = vm.galatLuapan;
                        runtimeError("Hasil penjumlahan melebihi jangkauan bilangan bulat.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    vm.stackTop -= 2;
                    push(INT_VAL(result));
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    DISPATCH_MAGIC(binaryMagicOrError(MAGIC_TAMBAH, MAGIC_KTAMBAH, "+"));
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("Operand '+' harus dua angka, dua teks, dua daftar, atau dua tupel.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: {
                // Before the himpunan branch: Vektor - himpunan is Vektor's call.
                DISPATCH_MAGIC(binaryMagicOrError(MAGIC_KURANGI, MAGIC_KKURANGI, "-"));
                if (IS_SET(peek(0)) || IS_SET(peek(1))) {   // himpunan - himpunan
                    if (!setBinary(OP_SUBTRACT, "-")) return INTERPRET_RUNTIME_ERROR;
                    break;
                }
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("Operand harus berupa angka.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (IS_INT(peek(0)) && IS_INT(peek(1))) {
                    int64_t result;
                    if (intSubOverflow(AS_INT(peek(1)), AS_INT(peek(0)), &result)) {
                        vm.runtimeErrorClass = vm.galatLuapan;
                        runtimeError("Hasil pengurangan melebihi jangkauan bilangan bulat.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    vm.stackTop -= 2;
                    push(INT_VAL(result));
                } else {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a - b));
                }
                break;
            }
            case OP_MULTIPLY: {
                // daftar * n, n * daftar, teks * n, n * teks, angka * angka
                Value b = peek(0);
                Value a = peek(1);
                if (IS_INT(a) && IS_INT(b)) {
                    int64_t result;
                    if (intMulOverflow(AS_INT(a), AS_INT(b), &result)) {
                        vm.runtimeErrorClass = vm.galatLuapan;
                        runtimeError("Hasil perkalian melebihi jangkauan bilangan bulat.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    vm.stackTop -= 2;
                    push(INT_VAL(result));
                    break;
                }
                if (IS_NUMBER(a) && IS_NUMBER(b)) {
                    double y = AS_NUMBER(pop());
                    double x = AS_NUMBER(pop());
                    push(NUMBER_VAL(x * y));
                    break;
                }
                // Before the sequence branch: Vektor * [1] is Vektor's call.
                DISPATCH_MAGIC(binaryMagicOrError(MAGIC_KALI, MAGIC_KKALI, "*"));

                Value seq = IS_NUMBER(b) ? a : b;    // the repeated operand
                Value times = IS_NUMBER(b) ? b : a;
                if (sequenceItems(seq) == NULL && !IS_STRING(seq)) {
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("Operand '*' harus dua angka, atau daftar/tupel/teks dengan angka.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int n;
                if (!repeatCount(times, &n)) return INTERPRET_RUNTIME_ERROR;

                if (sequenceItems(seq) != NULL) {
                    // daftar * n -> daftar, tupel * n -> tupel.
                    ValueArray* src = sequenceItems(seq);
                    if ((long)src->count * n > REPEAT_MAX) {
                        vm.runtimeErrorClass = vm.galatNilai;
                        runtimeError("Hasil pengulangan terlalu besar (maksimum %d).", REPEAT_MAX);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    Value result = IS_TUPLE(seq) ? OBJ_VAL(newTuple()) : OBJ_VAL(newList());
                    push(result);   // root while writeValueArray allocates
                    ValueArray* out = sequenceItems(result);
                    for (int r = 0; r < n; r++) {
                        for (int i = 0; i < src->count; i++) {
                            writeValueArray(out, src->values[i]);
                        }
                    }
                    pop();
                    vm.stackTop -= 2;
                    push(result);
                } else {
                    ObjString* src = AS_STRING(seq);
                    if ((long)src->length * n > REPEAT_MAX) {
                        vm.runtimeErrorClass = vm.galatNilai;
                        runtimeError("Hasil pengulangan terlalu besar (maksimum %d).", REPEAT_MAX);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    int length = src->length * n;
                    char* chars = ALLOCATE(char, length + 1);
                    for (int r = 0; r < n; r++) {
                        memcpy(chars + (size_t)r * src->length, src->chars, src->length);
                    }
                    chars[length] = '\0';
                    ObjString* result = takeString(chars, length);
                    vm.stackTop -= 2;
                    push(OBJ_VAL(result));
                }
                break;
            }
            case OP_DIVIDE: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    DISPATCH_MAGIC(binaryMagicOrError(MAGIC_BAGI, MAGIC_KBAGI, "/"));
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("Operand harus berupa angka.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (AS_NUMBER(peek(0)) == 0) {
                    vm.runtimeErrorClass = vm.galatBagiNol;
                    runtimeError("Pembagian dengan nol.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(a / b));
                break;
            }
            case OP_MODULO: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    DISPATCH_MAGIC(binaryMagicOrError(MAGIC_SISA, MAGIC_KSISA, "%"));
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("Operand harus berupa angka.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (AS_NUMBER(peek(0)) == 0) {
                    vm.runtimeErrorClass = vm.galatBagiNol;
                    runtimeError("Pembagian dengan nol.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (IS_INT(peek(0)) && IS_INT(peek(1))) {
                    int64_t result;
                    if (!intModulo(AS_INT(peek(1)), AS_INT(peek(0)), &result)) {
                        vm.runtimeErrorClass = vm.galatLuapan;
                        runtimeError("Hasil modulo melebihi jangkauan bilangan bulat.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    vm.stackTop -= 2;
                    push(INT_VAL(result));
                    break;
                }
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                // Python semantics: the result takes the sign of the divisor,
                // so -7 % 3 == 2 (C's fmod would give -1).
                double r = fmod(a, b);
                if (r == 0) {
                    r = 0;  // avoid printing "-0" for -6 % 3
                } else if ((r < 0) != (b < 0)) {
                    r += b;
                }
                push(NUMBER_VAL(r));
                break;
            }
            case OP_IN: {
                bool found;
                if (!containsValue(peek(0), peek(1), &found)) {
                    // A _hash_/_sama_ that raised leaves a native error: it
                    // carries the real message, so resolve it before giving up.
                    RESOLVE_MAGIC_ERROR();
                    return INTERPRET_RUNTIME_ERROR;
                }
                RESOLVE_MAGIC_ERROR();
                vm.stackTop -= 2;
                push(BOOL_VAL(found));
                break;
            }
            case OP_CHAIN_COMPARE: {
                ChainCompare op = (ChainCompare)READ_BYTE();
                Value right = peek(0);
                bool result;
                if (!chainCompare(op, peek(1), right, &result)) {
                    RESOLVE_MAGIC_ERROR();
                    return INTERPRET_RUNTIME_ERROR;
                }
                vm.stackTop -= 2;
                push(right);
                push(BOOL_VAL(result));
                break;
            }
            case OP_CHAIN_CLEANUP: {
                Value result = pop();
                pop();
                push(result);
                break;
            }
            case OP_BIT_AND:
            case OP_BIT_OR:
            case OP_BIT_XOR:
            case OP_SHIFT_LEFT:
            case OP_SHIFT_RIGHT:
                if (!bitwiseBinary(instruction)) return INTERPRET_RUNTIME_ERROR;
                break;
            case OP_BIT_NOT: {
                // ~logika is rejected: Python 3.12 deprecated it and its value
                // (~True == -2) only confuses. 'tidak' is the logical NOT.
                if (!IS_INT(peek(0))) {
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError(IS_BOOL(peek(0))
                        ? "Operand '~' harus bilangan bulat (untuk logika pakai 'tidak')."
                        : "Operand '~' harus bilangan bulat.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int64_t value = AS_INT(pop());
                push(INT_VAL(~value));  // never overflows: ~INT64_MIN == INT64_MAX
                break;
            }
            case OP_FLOOR_DIVIDE: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    DISPATCH_MAGIC(binaryMagicOrError(MAGIC_BAGI_BULAT, MAGIC_KBAGI_BULAT, "//"));
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("Operand harus berupa angka.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (AS_NUMBER(peek(0)) == 0) {
                    vm.runtimeErrorClass = vm.galatBagiNol;
                    runtimeError("Pembagian dengan nol.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (IS_INT(peek(0)) && IS_INT(peek(1))) {
                    int64_t result;
                    if (!intFloorDiv(AS_INT(peek(1)), AS_INT(peek(0)), &result)) {
                        vm.runtimeErrorClass = vm.galatLuapan;
                        runtimeError("Hasil pembagian bulat melebihi jangkauan bilangan bulat.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    vm.stackTop -= 2;
                    push(INT_VAL(result));
                    break;
                }
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                // Rounds toward negative infinity, like Python: -7 // 2 == -4.
                push(NUMBER_VAL(floor(a / b)));
                break;
            }
            case OP_POWER: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    DISPATCH_MAGIC(binaryMagicOrError(MAGIC_PANGKAT, MAGIC_KPANGKAT, "**"));
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("Operand harus berupa angka.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (AS_NUMBER(peek(1)) == 0 && AS_NUMBER(peek(0)) < 0) {
                    vm.runtimeErrorClass = vm.galatBagiNol;   // Python: ZeroDivisionError
                    runtimeError("0 tidak dapat dipangkatkan dengan bilangan negatif.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                // A bulat base with a non-negative bulat exponent stays bulat.
                if (IS_INT(peek(0)) && IS_INT(peek(1)) && AS_INT(peek(0)) >= 0) {
                    bool overflow;
                    int64_t result = intPow(AS_INT(peek(1)), AS_INT(peek(0)), &overflow);
                    if (overflow) {
                        vm.runtimeErrorClass = vm.galatLuapan;
                        runtimeError("Hasil pemangkatan melebihi jangkauan bilangan bulat.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    vm.stackTop -= 2;
                    push(INT_VAL(result));
                    break;
                }
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(pow(a, b)));
                break;
            }
            case OP_NOT:
                push(BOOL_VAL(isFalsey(pop())));
                break;
            case OP_NEGATE: {
                if (!IS_NUMBER(peek(0))) {
                    vm.runtimeErrorClass = vm.galatTipe;
                    if (IS_INSTANCE(peek(0))) {
                        Value method;
                        if (instanceMagic(peek(0), MAGIC_NEGATIF, &method)) {
                            vm.runtimeErrorClass = NULL;
                            // Receiver already in slot 0, no arguments.
                            if (!call(AS_CLOSURE(method), 0, NULL)) {
                                return INTERPRET_RUNTIME_ERROR;
                            }
                            frame = &vm.frames[vm.frameCount - 1];
                            break;
                        }
                        runtimeError("Operator '-' tidak didukung untuk '%s'.",
                                     valueTypeName(peek(0)));
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    runtimeError("Operand harus berupa angka.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (IS_INT(peek(0))) {
                    int64_t value = AS_INT(peek(0));
                    if (value == INT64_MIN) {
                        vm.runtimeErrorClass = vm.galatLuapan;
                        runtimeError("Hasil negasi melebihi jangkauan bilangan bulat.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    vm.stackTop--;
                    push(INT_VAL(-value));
                    break;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                // Ensure we don't jump to invalid bytecode
                uint8_t* newIp = frame->ip - offset;
                if (newIp < frame->closure->function->chunk.code ||
                    newIp >= frame->closure->function->chunk.code + frame->closure->function->chunk.count) {
                    fatalError("Invalid loop jump offset: offset=%d, ip was %td",
                                 offset, (frame->ip - frame->closure->function->chunk.code));
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame->ip = newIp;
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0))) {
                    frame->ip += offset;
                }
                break;
            }
            case OP_CALL: {
                int argCount = READ_BYTE();
                if (!callValue(peek(argCount), argCount, NULL)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                int ne = resolveNativeError();
                if (ne == -1) return INTERPRET_RUNTIME_ERROR;
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_CALL_BERNAMA: {
                int argCount = READ_BYTE();
                ObjList* names = AS_LIST(READ_CONSTANT());
                if (!callValue(peek(argCount), argCount, names)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                int ne = resolveNativeError();
                if (ne == -1) return INTERPRET_RUNTIME_ERROR;
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_ISI_BAWAAN: {
                // Skip this parameter's default expression when the caller
                // already supplied a value for the slot.
                uint8_t slot = READ_BYTE();
                uint16_t offset = READ_SHORT();
                if (!IS_TAKTERISI(frame->slots[slot])) {
                    frame->ip += offset;
                }
                break;
            }
            case OP_CLOSURE: {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure* closure = newClosure(function);
                push(OBJ_VAL(closure));
                for (int i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues[i] = captureUpvalue(frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                if (IS_TAKTERISI(*frame->closure->upvalues[slot]->location)) {
                    runtimeError("Variabel lokal sudah di-buang atau belum diisi.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
                break;
            }
            case OP_CLOSE_UPVALUE:
                closeUpvalues(vm.stackTop - 1);
                pop();
                break;
            case OP_BUILD_LIST: {
                int count = READ_BYTE();
                ObjList* list = newList();
                // Keep the list reachable while appending (append may
                // grow the item array and trigger a GC).
                push(OBJ_VAL(list));
                for (int i = count; i >= 1; i--) {
                    writeValueArray(&list->items, peek(i));
                }
                pop();                       // the list
                vm.stackTop -= count;        // the elements
                push(OBJ_VAL(list));
                break;
            }
            case OP_BUILD_TUPLE: {
                // Same rooting pattern as OP_BUILD_LIST: the tupel and its
                // elements stay on the stack while the item array grows.
                int count = READ_BYTE();
                ObjTuple* tuple = newTuple();
                push(OBJ_VAL(tuple));
                for (int i = count; i >= 1; i--) {
                    writeValueArray(&tuple->items, peek(i));
                }
                pop();                       // the tupel
                vm.stackTop -= count;        // the elements
                push(OBJ_VAL(tuple));
                break;
            }
            case OP_APPEND: {
                // List comprehension: stack is [daftar, nilai]. Both stay on the
                // stack while writeValueArray grows the array (it may GC).
                if (!IS_LIST(peek(1))) {
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("Hanya daftar yang dapat ditambah elemen.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                writeValueArray(&AS_LIST(peek(1))->items, peek(0));
                vm.stackTop -= 2;
                break;
            }
            case OP_DICT_PUT: {
                // Dict comprehension: stack is [kamus, kunci, nilai]. The dict
                // stays rooted while dictSet grows the table (it may GC).
                Value key = peek(1);
                uint32_t hash;
                if (!hashValue(key, &hash)) {
                    if (vm.nativeErrorPending) {
                        if (resolveNativeError() == -1) return INTERPRET_RUNTIME_ERROR;
                        frame = &vm.frames[vm.frameCount - 1];
                        break;
                    }
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("Kunci kamus harus nilai yang dapat di-hash "
                                 "(angka, teks, logika, kosong, tupel, atau himpunan_beku berisi nilai seperti itu).");
                    return INTERPRET_RUNTIME_ERROR;
                }
                dictSet(AS_DICT(peek(2)), key, peek(0));
                vm.stackTop -= 2;        // leave the kamus
                break;
            }
            case OP_BUILD_SET:
                push(OBJ_VAL(newSet()));
                break;
            case OP_SET_PUT: {
                // Set comprehension: stack is [himpunan, nilai].
                if (!setAdd(AS_SET(peek(1)), peek(0))) {
                    if (vm.nativeErrorPending) {
                        if (resolveNativeError() == -1) return INTERPRET_RUNTIME_ERROR;
                        frame = &vm.frames[vm.frameCount - 1];
                        break;
                    }
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("Anggota himpunan harus nilai yang dapat di-hash.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vm.stackTop -= 1;        // leave the himpunan
                break;
            }
            case OP_BUILD_DICT: {
                int count = READ_BYTE();
                // Stack: key1, val1, ... keyN, valN
                ObjDict* dict = newDict();
                // Keep the dict reachable: dictSet can grow both the table and
                // the insertion-order key array, either of which may GC.
                push(OBJ_VAL(dict));
                bool unwound = false;   // a _hash_ raised: stack already unwound
                for (int i = count; i >= 1; i--) {
                    Value key = peek(i * 2);        // +1 for the dict itself
                    Value value = peek(i * 2 - 1);
                    uint32_t hash;
                    if (!hashValue(key, &hash)) {   // K8: any hashable key
                        if (vm.nativeErrorPending) {
                            // _hash_ raised: its error wins. throwValue() inside
                            // resolveNativeError already unwound the stack to the
                            // handler, so the pops below must NOT run -- this
                            // `break` only leaves the for loop.
                            if (resolveNativeError() == -1) return INTERPRET_RUNTIME_ERROR;
                            frame = &vm.frames[vm.frameCount - 1];
                            unwound = true;
                            break;
                        }
                        vm.runtimeErrorClass = vm.galatTipe;
                        runtimeError("Kunci kamus harus nilai yang dapat di-hash "
                                     "(angka, teks, logika, kosong, tupel, atau himpunan_beku berisi nilai seperti itu).");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    dictSet(dict, key, value);
                }
                if (unwound) break;
                pop();                          // the dict
                vm.stackTop -= count * 2;       // the pairs
                push(OBJ_VAL(dict));
                break;
            }
            case OP_ITER_NEXT:
                // Iterator protocol uses the receiver in peek(1) and the
                // synthetic index in peek(0). Remove the index before calling
                // so the normal zero-argument call layout is preserved.
                if (IS_GENERATOR(peek(1))) {
                    ObjGenerator* gen = AS_GENERATOR(peek(1));
                    Value produced;
                    int step = resumeGenerator(gen, &produced);
                    if (step == 0) {
                        // Exhausted: raise HentiIterasi, which the loop's own
                        // handler turns into a clean exit.
                        vm.nativeErrorValue =
                            OBJ_VAL(newError(vm.hentiIterasi, "Iterasi selesai."));
                        vm.nativeErrorPending = true;
                        step = -1;
                    }
                    if (step < 0) {
                        // Pending native error (from the body or the sentinel
                        // above): unwind through the normal handler search so
                        // 'coba' and the loop's handler both behave as usual.
                        pop();                      // index
                        pop();                      // generator
                        int handled = resolveNativeError();
                        if (handled < 0) return INTERPRET_RUNTIME_ERROR;
                        frame = &vm.frames[vm.frameCount - 1];
                        break;
                    }
                    pop();                          // index
                    pop();                          // generator
                    push(produced);
                    break;
                }
                if (IS_INSTANCE(peek(1))) {
                    Value method;
                    if (instanceMagic(peek(1), MAGIC_BERIKUTNYA, &method)) {
                        pop();
                        if (!call(AS_CLOSURE(method), 0, NULL)) {
                            return INTERPRET_RUNTIME_ERROR;
                        }
                        frame = &vm.frames[vm.frameCount - 1];
                        break;
                    }
                }
                // Ordinary sequences keep the old indexed behavior.
                /* fall through */
            case OP_SEQ_GET:
                // 'untuk' / comprehension read sequences by position. A kamus
                // yields its keys in insertion order — the integer here is a
                // position, not a key (kamus[1] is a key lookup).
                if (IS_DICT(peek(1)) || IS_SET(peek(1))) {
                    // A himpunan is dict-shaped (ObjSet): its members are the keys.
                    int index = (int)AS_NUMBER(peek(0));
                    ObjDict* dict = IS_SET(peek(1)) ? setAsDict(AS_SET(peek(1))) : AS_DICT(peek(1));
                    if (!IS_NUMBER(peek(0)) || index < 0 || index >= dict->keys.count) {
                        vm.runtimeErrorClass = vm.galatIndeks;
                        runtimeError("Indeks kamus di luar jangkauan.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    Value key = dictKeyAt(dict, index);
                    vm.stackTop -= 2;
                    push(key);
                    break;
                }
                // Everything else behaves like ordinary indexing.
                // fall through
            case OP_INDEX_GET: {
                if (sequenceItems(peek(1)) != NULL && IS_NUMBER(peek(0))) {
                    // daftar or tupel; also reached by 'untuk' via OP_SEQ_GET.
                    const char* kind = IS_TUPLE(peek(1)) ? "tupel" : "list";
                    int given = (int)AS_NUMBER(pop());
                    ValueArray* items = sequenceItems(pop());
                    int index = given < 0 ? given + items->count : given;
                    if (index < 0 || index >= items->count) {
                        vm.runtimeErrorClass = vm.galatIndeks;
                        runtimeError("Indeks %s di luar jangkauan (indeks %d, panjang %d).",
                                     kind, given, items->count);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    push(items->values[index]);
                } else if (IS_RANGE(peek(1)) && IS_NUMBER(peek(0))) {
                    double index = AS_NUMBER(pop());
                    ObjRange* range = AS_RANGE(pop());
                    double length = rangeLength(range);
                    if (index < 0) index += length;
                    if (index != floor(index) || index < 0 || index >= length) {
                        runtimeError("Indeks rentang di luar jangkauan (indeks %.15g, panjang %.15g).",
                                     index, length);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    push(rangeElement(range, (int)index));
                } else if (IS_DICT(peek(1))) {
                    // K8: the index IS the key now — kamus[1] no longer means
                    // "the first key". Use kunci(kamus)[i] for that.
                    Value key = peek(0);
                    ObjDict* dict = AS_DICT(peek(1));
                    uint32_t hash;
                    if (!hashValue(key, &hash)) {
                        if (vm.nativeErrorPending) {
                            // _hash_ raised: its error wins over the generic one.
                            if (resolveNativeError() == -1) return INTERPRET_RUNTIME_ERROR;
                            frame = &vm.frames[vm.frameCount - 1];
                            break;
                        }
                        vm.runtimeErrorClass = vm.galatTipe;
                        runtimeError("Kunci kamus harus nilai yang dapat di-hash "
                                     "(angka, teks, logika, kosong, tupel, atau himpunan_beku berisi nilai seperti itu).");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    Value value;
                    // Keep the dict and key on the stack while valueTableGet
                    // runs: it compares with _sama_, whose call allocates and
                    // can trigger a GC that would otherwise free them.
                    bool ada = valueTableGet(&dict->table, key, &value);
                    vm.stackTop -= 2;
                    if (!ada) {
                        push(KOSONG_VAL);
                    } else {
                        push(value);
                    }
                } else if (IS_STRING(peek(1)) && IS_NUMBER(peek(0))) {
                    // Index by character; keep the string on the stack while
                    // copyString allocates (a GC could otherwise free it).
                    int index = (int)AS_NUMBER(peek(0));
                    ObjString* str = AS_STRING(peek(1));
                    int given = index;
                    if (index < 0) index += str->charCount;
                    if (index < 0 || index >= str->charCount) {
                        runtimeError("Indeks string di luar jangkauan (indeks %d, panjang %d).",
                                     given, str->charCount);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    int offset = utf8ByteOffset(str, index);
                    ObjString* ch = copyString(str->chars + offset, utf8CharBytes(str, offset));
                    vm.stackTop -= 2;
                    push(OBJ_VAL(ch));
                } else if (IS_INSTANCE(peek(1))) {
                    // K19 tahap 2: objek[kunci] -> _ambil_(kunci). Stack is
                    // already receiver, arg -- exactly call()'s layout, so the
                    // method's return value lands where the index result goes.
                    Value method;
                    if (!instanceMagic(peek(1), MAGIC_AMBIL, &method)) {
                        vm.runtimeErrorClass = vm.galatTipe;
                        runtimeError("Objek '%s' tidak mendukung indeks; "
                                     "tambahkan metode _ambil_(kunci).",
                                     valueTypeName(peek(1)));
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    if (!call(AS_CLOSURE(method), 1, NULL)) return INTERPRET_RUNTIME_ERROR;
                    frame = &vm.frames[vm.frameCount - 1];
                } else {
                    // Python: TypeError ('set' object is not subscriptable).
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("Pengindeksan tidak valid: gunakan daftar[angka], tupel[angka], "
                                 "teks[angka], atau kamus[kunci].");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_INDEX_SET: {
                // Stack: target, index/key, value
                if (IS_TUPLE(peek(2))) {
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("Tupel tidak dapat diubah.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (IS_LIST(peek(2)) && IS_NUMBER(peek(1))) {
                    Value value = pop();
                    int given = (int)AS_NUMBER(pop());
                    ObjList* list = AS_LIST(pop());
                    int index = given < 0 ? given + list->items.count : given;
                    if (index < 0 || index >= list->items.count) {
                        vm.runtimeErrorClass = vm.galatIndeks;
                        runtimeError("Indeks list di luar jangkauan (indeks %d, panjang %d).",
                                     given, list->items.count);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    list->items.values[index] = value;
                    push(value);
                } else if (IS_DICT(peek(2))) {
                    // Keep operands on the stack while dictSet runs — it can
                    // grow the table and trigger a GC that would otherwise
                    // free the dict/key/value.
                    ObjDict* dict = AS_DICT(peek(2));
                    Value key = peek(1);
                    Value value = peek(0);
                    uint32_t hash;
                    if (!hashValue(key, &hash)) {
                        if (vm.nativeErrorPending) {
                            // _hash_ raised: its error wins over the generic one.
                            if (resolveNativeError() == -1) return INTERPRET_RUNTIME_ERROR;
                            frame = &vm.frames[vm.frameCount - 1];
                            break;
                        }
                        vm.runtimeErrorClass = vm.galatTipe;
                        runtimeError("Kunci kamus harus nilai yang dapat di-hash "
                                     "(angka, teks, logika, kosong, tupel, atau himpunan_beku berisi nilai seperti itu).");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    dictSet(dict, key, value);
                    pop();  // value
                    pop();  // key
                    pop();  // dict
                    push(value);
                } else if (IS_INSTANCE(peek(2))) {
                    // K19 tahap 2: objek[kunci] = nilai -> _atur_(kunci, nilai).
                    // Stack is receiver, key, value: call()'s layout for 2 args.
                    Value method;
                    if (!instanceMagic(peek(2), MAGIC_ATUR, &method)) {
                        vm.runtimeErrorClass = vm.galatTipe;
                        runtimeError("Objek '%s' tidak mendukung penugasan indeks; "
                                     "tambahkan metode _atur_(kunci, nilai).",
                                     valueTypeName(peek(2)));
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    if (!call(AS_CLOSURE(method), 2, NULL)) return INTERPRET_RUNTIME_ERROR;
                    frame = &vm.frames[vm.frameCount - 1];
                } else {
                    vm.runtimeErrorClass = vm.galatTipe;   // Python: TypeError
                    if (IS_STRING(peek(2))) {
                        runtimeError("String tidak dapat diubah; gunakan potong()/gabungan untuk membuat string baru.");
                    } else {
                        runtimeError("Pengindeksan tidak valid: gunakan daftar[angka] atau kamus[kunci].");
                    }
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SLICE: {
                // Stack: target, start, stop, step (omitted parts are kosong).
                // Operands stay on the stack while the result allocates.
                Value target = peek(3);
                int start, stop, step;
                const char* msg = NULL;

                if (sequenceItems(target) != NULL) {
                    // daftar[a:b] gives a daftar, tupel[a:b] a tupel (Python).
                    ValueArray* src = sequenceItems(target);
                    if (!resolveSlice(peek(2), peek(1), peek(0), src->count,
                                      &start, &stop, &step, &msg)) {
                        vm.runtimeErrorClass = vm.galatNilai;
                        runtimeError("%s", msg);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    Value out = IS_TUPLE(target) ? OBJ_VAL(newTuple()) : OBJ_VAL(newList());
                    push(out);   // root: writeValueArray may GC
                    ValueArray* outItems = sequenceItems(out);
                    for (int i = start; step > 0 ? i < stop : i > stop; i += step) {
                        writeValueArray(outItems, src->values[i]);
                    }
                    pop();
                    vm.stackTop -= 4;
                    push(out);
                } else if (IS_STRING(target)) {
                    ObjString* src = AS_STRING(target);
                    if (!resolveSlice(peek(2), peek(1), peek(0), src->charCount,
                                      &start, &stop, &step, &msg)) {
                        vm.runtimeErrorClass = vm.galatNilai;
                        runtimeError("%s", msg);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    // Build the byte buffer first; src stays rooted on the stack.
                    int count = 0;
                    for (int i = start; step > 0 ? i < stop : i > stop; i += step) count++;
                    // Worst case: every character is 4 UTF-8 bytes.
                    char* buffer = ALLOCATE(char, (size_t)count * 4 + 1);
                    int len = 0;
                    for (int i = start; step > 0 ? i < stop : i > stop; i += step) {
                        int offset = utf8ByteOffset(src, i);
                        int bytes = utf8CharBytes(src, offset);
                        memcpy(buffer + len, src->chars + offset, bytes);
                        len += bytes;
                    }
                    ObjString* out = copyString(buffer, len);
                    FREE_ARRAY(char, buffer, (size_t)count * 4 + 1);
                    vm.stackTop -= 4;
                    push(OBJ_VAL(out));
                } else {
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("Irisan hanya untuk daftar, tupel, dan teks.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_CLASS_FIELD: {
                // NAMA = nilai di badan kelas. Kelas ada di bawah nilainya
                // karena classDeclaration mendorongnya sebelum badan.
                ObjString* name = READ_STRING();
                ObjClass* klass = AS_CLASS(peek(1));
                tableSet(&klass->fields, name, peek(0));
                // Atribut dan metode berbagi satu ruang nama, seperti Python:
                // yang ditulis belakangan menang.
                tableDelete(&klass->methods, name);
                pop();                      // nilai; kelas tetap di stack
                break;
            }
            case OP_ITER_OBJEK: {
                // Runs once, just before a loop. The iterable sits on top of
                // the stack. An instance with _iter_ is replaced by that
                // method's return value: call() with 0 arguments makes the
                // receiver slot 0, so the result lands exactly where the
                // iterable was and the loop proceeds over it. Anything else is
                // left untouched -- zero cost for every existing program.
                Value method;
                if (instanceMagic(peek(0), MAGIC_ITER, &method)) {
                    if (!call(AS_CLOSURE(method), 0, NULL)) {
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    frame = &vm.frames[vm.frameCount - 1];
                }
                break;
            }
            case OP_LIST_LEN: {
                // Length of what 'untuk' iterates; elements are then read
                // with OP_INDEX_GET, so any indexable sequence works.
                Value seq = pop();
                if (sequenceItems(seq) != NULL) {          // daftar or tupel
                    push(INT_VAL(sequenceItems(seq)->count));
                } else if (IS_RANGE(seq)) {
                    push(INT_VAL((int64_t)rangeLength(AS_RANGE(seq))));
                } else if (IS_DICT(seq)) {
                    push(INT_VAL(dictLength(AS_DICT(seq))));
                } else if (IS_SET(seq)) {
                    push(INT_VAL(AS_SET(seq)->keys.count));
                } else if (IS_STRING(seq)) {
                    push(INT_VAL(AS_STRING(seq)->charCount));
                } else if (IS_GENERATOR(seq)) {
                    // Length is unknown; the loop ends when the generator is
                    // exhausted (OP_ITER_NEXT raises HentiIterasi).
                    push(INT_VAL(INT64_MAX));
                } else if (IS_INSTANCE(seq)) {
                    Value method;
                    if (instanceMagic(seq, MAGIC_BERIKUTNYA, &method)) {
                        // The compiler's existing index loop remains the
                        // control structure; an iterator runs until HentiIterasi.
                        push(INT_VAL(INT64_MAX));
                    } else {
                        vm.runtimeErrorClass = vm.galatTipe;
                        runtimeError("'untuk ... dalam' hanya dapat mengiterasi daftar, "
                                     "tupel, himpunan, teks, rentang, kamus, atau objek "
                                     "dengan metode _iter_().");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                } else {
                    vm.runtimeErrorClass = vm.galatTipe;
                    runtimeError("'untuk ... dalam' hanya dapat mengiterasi daftar, "
                                 "tupel, himpunan, teks, rentang, kamus, atau objek "
                                 "dengan metode _iter_().");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_CLASS:
                push(OBJ_VAL(newClass(READ_STRING())));
                break;
            case OP_INHERIT: {
                int baseCount = READ_BYTE();
                ObjClass* subclass = AS_CLASS(peek(0));
                ObjClass* bases[MRO_MAX];
                if (baseCount > MRO_MAX) {
                    runtimeError("Terlalu banyak kelas induk.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                // Stack: base1 ... baseN subclass
                for (int i = 0; i < baseCount; i++) {
                    Value base = peek(baseCount - i);
                    if (!IS_CLASS(base)) {
                        runtimeError("Kelas induk harus berupa kelas.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    for (int j = 0; j < i; j++) {
                        if (bases[j] == AS_CLASS(base)) {
                            runtimeError("Kelas induk '%s' disebut dua kali.",
                                         AS_CLASS(base)->name->chars);
                            return INTERPRET_RUNTIME_ERROR;
                        }
                    }
                    bases[i] = AS_CLASS(base);
                }

                if (!c3Linearize(subclass, bases, baseCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                // superclass = induk pertama: 'super' dan kelas galat bawaan
                // memakainya dan tidak perlu tahu soal MRO.
                subclass->superclass = bases[0];

                // Metode tetap DIRATAKAN supaya lookup tetap satu tableGet.
                // Disalin dari belakang MRO ke depan: yang lebih awal di MRO
                // menimpa yang belakangan, persis prioritas C3.
                for (int i = subclass->mroCount - 1; i >= 1; i--) {
                    ObjClass* base = subclass->mro[i];
                    // @abstrak declares a contract, it does not provide an
                    // implementation. Copying its empty body would let it beat
                    // a real method from a base later in the MRO -- the mixin
                    // pattern, where one base declares and another supplies.
                    Table* src = &base->own;
                    for (int j = 0; j < src->capacity; j++) {
                        Entry* entry = &src->entries[j];
                        if (entry->key == NULL) continue;
                        Value tag;
                        if (tableGet(&base->special, entry->key, &tag) &&
                            (uint8_t)AS_NUMBER(tag) == SPECIAL_ABSTRAK) {
                            continue;
                        }
                        tableSet(&subclass->methods, entry->key, entry->value);
                    }
                    subclass->magic |= base->magic;
                }
                pop();  // subclass
                break;
            }
            case OP_METHOD:
                defineMethod(READ_STRING());
                break;
            case OP_SPECIAL_METHOD: {
                ObjString* name = READ_STRING();
                uint8_t kind = READ_BYTE();
                ObjClass* klass = AS_CLASS(peek(1));
                defineMethod(name);   // pops the closure
                tableSet(&klass->special, name, NUMBER_VAL((double)kind));
                break;
            }
            case OP_GET_PROPERTY: {
                if (IS_MODULE(peek(0))) {
                    // matematika.mat_fpb -- a module's globals are its members.
                    ObjString* name = READ_STRING();
                    ObjModule* module = AS_MODULE(peek(0));
                    Value value;
                    if (!tableGet(&module->globals, name, &value)) {
                        vm.runtimeErrorClass = vm.galatNama;
                        runtimeError("Modul '%s' tidak punya '%s'.",
                                     module->name->chars, name->chars);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    pop();            // module
                    push(value);
                    break;
                }
                if (IS_CLASS(peek(0))) {
                    // Lingkaran.PHI -- atribut lewat kelas; metode lewat kelas
                    // adalah fungsi biasa yang belum terikat ke instance.
                    ObjClass* klass = AS_CLASS(peek(0));
                    ObjString* name = READ_STRING();
                    Value value;
                    if (classFieldGet(klass, name, &value) ||
                        classMethodGet(klass, name, &value)) {
                        if (specialKind(klass, name) == SPECIAL_PROPERTI) {
                            // Kelas.luas has no instance to compute against.
                            vm.runtimeErrorClass = vm.galatAtribut;
                            runtimeError(
                                "'%s' adalah @properti: baca lewat objek, "
                                "bukan lewat kelas '%s'.",
                                name->chars, klass->name->chars);
                            return INTERPRET_RUNTIME_ERROR;
                        }
                        pop();              // kelas
                        push(value);
                        break;
                    }
                    vm.runtimeErrorClass = vm.galatAtribut;
                    runtimeError("Kelas '%s' tidak punya atribut '%s'.",
                                 klass->name->chars, name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (!IS_INSTANCE(peek(0))) {
                    // ponytail: built-in methods can be called but not taken
                    // as values — ObjBoundMethod only carries ObjClosure, and
                    // a native-bound variant isn't worth a new object type
                    // yet. Say so plainly instead of "hanya objek...".
                    const char* typeName;
                    NativeMethod* table = methodsForValue(peek(0), &typeName);
                    ObjString* propName = READ_STRING();
                    if (table != NULL &&
                        findNativeMethod(table, propName) != NULL) {
                        vm.runtimeErrorClass = vm.galatAtribut;
                        runtimeError(
                            "Metode '%s' pada tipe '%s' harus langsung "
                            "dipanggil, misalnya x.%s().",
                            propName->chars, typeName, propName->chars);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    if (table != NULL) {
                        vm.runtimeErrorClass = vm.galatAtribut;
                        runtimeError("Tipe '%s' tidak punya properti '%s'.",
                                     typeName, propName->chars);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    vm.runtimeErrorClass = vm.galatAtribut;   // Python: AttributeError
                    runtimeError("Hanya objek yang memiliki properti.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjInstance* instance = AS_INSTANCE(peek(0));
                ObjString* name = READ_STRING();

                Value value;
                if (tableGet(&instance->fields, name, &value)) {
                    pop();  // instance
                    push(value);
                    break;
                }

                // Urutan Python: field instance -> atribut kelas -> metode.
                if (classFieldGet(instance->klass, name, &value)) {
                    pop();  // instance
                    push(value);
                    break;
                }

                uint8_t kind = specialKind(instance->klass, name);
                if (kind != 0) {
                    Value method;
                    if (classMethodGet(instance->klass, name, &method)) {
                        if (kind == SPECIAL_PROPERTI) {
                            // obj.luas runs luas() now: the receiver is
                            // already in the slot, so this is a plain call
                            // with zero arguments.
                            if (!call(AS_CLOSURE(method), 0, NULL)) {
                                return INTERPRET_RUNTIME_ERROR;
                            }
                            frame = &vm.frames[vm.frameCount - 1];
                            break;
                        }
                        // Statics and class methods are not bound to the
                        // instance; as a value both are just the function.
                        pop();   // instance
                        push(method);
                        break;
                    }
                }

                if (!bindMethod(instance->klass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_PROPERTY: {
                if (IS_CLASS(peek(1))) {
                    // Lingkaran.PHI = x -- mengubah nilai bersama. Menulis
                    // lewat instance tidak pernah sampai ke sini; itu selalu
                    // membuat field instance.
                    tableSet(&AS_CLASS(peek(1))->fields, READ_STRING(), peek(0));
                    Value value = pop();
                    pop();                  // kelas
                    push(value);
                    break;
                }
                if (!IS_INSTANCE(peek(1))) {
                    vm.runtimeErrorClass = vm.galatAtribut;   // Python: AttributeError
                    runtimeError("Hanya objek yang memiliki properti.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjInstance* instance = AS_INSTANCE(peek(1));
                tableSet(&instance->fields, READ_STRING(), peek(0));
                Value value = pop();
                pop();  // instance
                push(value);
                break;
            }
            case OP_INVOKE: {
                ObjString* method = READ_STRING();
                int argCount = READ_BYTE();
                if (!invoke(method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                int ne = resolveNativeError();
                if (ne == -1) return INTERPRET_RUNTIME_ERROR;
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_GET_SUPER: {
                ObjString* name = READ_STRING();
                ObjClass* superclass = AS_CLASS(pop());
                if (!bindMethod(superclass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUPER_INVOKE: {
                ObjString* method = READ_STRING();
                int argCount = READ_BYTE();
                ObjClass* superclass = AS_CLASS(pop());
                if (!invokeFromClass(superclass, method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_TRY_BEGIN: {
                uint16_t offset = READ_SHORT();
                if (vm.tryCount == TRY_MAX) {
                    runtimeError("Terlalu banyak blok 'coba' bersarang.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                TryHandler* handler = &vm.tryHandlers[vm.tryCount++];
                handler->handlerIp = frame->ip + offset;
                handler->frameCount = vm.frameCount;
                handler->stackTop = vm.stackTop;
                break;
            }
            case OP_TRY_END:
                // Normal completion of a 'coba' block: unregister its handler.
                if (vm.tryCount > 0) vm.tryCount--;
                break;
            case OP_THROW: {
                Value error = pop();
                if (!throwValue(error)) {
                    if (vm.nativeCallDepth > 0) {
                        // Inside a callback invoked from C: hand the value
                        // back to the native, which re-raises it.
                        vm.nativeErrorValue = error;
                        vm.nativeErrorPending = true;
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    // No handler: report and abort.
                    if (IS_STRING(error)) {
                        runtimeError("Kesalahan tak tertangani: %s", AS_CSTRING(error));
                    } else if (IS_INSTANCE(error) &&
                               errorMessage(AS_INSTANCE(error)) != NULL) {
                        // A typed galat no 'kecuali' matched (e.g. a GalatNama
                        // through 'kecuali GalatTipe'): show its message, just as
                        // if it had never passed through a 'coba'.
                        runtimeError("%s", errorMessage(AS_INSTANCE(error))->chars);
                    } else {
                        runtimeError("Kesalahan tak tertangani.");
                    }
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                push(error);  // make the raised value available to 'kecuali'
                break;
            }
            case OP_PASTIKAN: {
                // The assertion already failed; the stack holds the message
                // expression, or kosong when none was written.
                Value given = peek(0);        // keep rooted: newError allocates
                ObjInstance* error;
                if (IS_KOSONG(given)) {
                    error = newError(vm.galatPenegasan, "Penegasan gagal.");
                } else if (IS_STRING(given)) {
                    error = newError(vm.galatPenegasan, AS_CSTRING(given));
                } else {
                    // ponytail: numbers/booleans only. A full repr would go through
                    // formatValue, which may run user code (_repr_) right here while
                    // raising; wrap the message in ke_teks() for richer values.
                    char buffer[64];
                    if (IS_INT(given)) {
                        // bulat must not be widened: 404 stays "404" (K2).
                        snprintf(buffer, sizeof(buffer), "%lld",
                                 (long long)AS_INT(given));
                    } else if (IS_NUMBER(given)) {
                        numberToString(AS_NUMBER(given), buffer, sizeof(buffer));
                    } else if (IS_BOOL(given)) {
                        snprintf(buffer, sizeof(buffer), "%s",
                                 AS_BOOL(given) ? "benar" : "salah");
                    } else {
                        snprintf(buffer, sizeof(buffer), "Penegasan gagal.");
                    }
                    error = newError(vm.galatPenegasan, buffer);
                }
                pop();                        // drop the message expression
                Value raised = OBJ_VAL(error);
                if (!throwValue(raised)) {
                    if (vm.nativeCallDepth > 0) {
                        vm.nativeErrorValue = raised;
                        vm.nativeErrorPending = true;
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    ObjString* message = errorMessage(error);
                    // The default message is already a full sentence; only a
                    // user-supplied one needs the "Penegasan gagal:" prefix.
                    if (message == NULL ||
                        strcmp(message->chars, "Penegasan gagal.") == 0) {
                        runtimeError("Penegasan gagal.");
                    } else {
                        runtimeError("Penegasan gagal: %s", message->chars);
                    }
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                push(raised);
                break;
            }
            case OP_IS_INSTANCE: {
                Value target = pop();
                push(BOOL_VAL(valueIsInstanceOf(peek(0), target)));
                break;
            }
            case OP_IMPORT_MODULE: {
                ObjString* name = READ_STRING();
                Value module;
                if (!loadModule(name, &module)) return INTERPRET_RUNTIME_ERROR;
                push(module);
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_IMPORT_FROM: {
                // Stack: the module, left by OP_IMPORT_MODULE.
                ObjString* member = READ_STRING();
                Value moduleValue = peek(0);
                Value value;
                if (!IS_MODULE(moduleValue) ||
                    !tableGet(&AS_MODULE(moduleValue)->globals, member, &value)) {
                    vm.runtimeErrorClass = vm.galatNama;
                    runtimeError("Modul '%s' tidak punya '%s'.",
                                 IS_MODULE(moduleValue)
                                     ? AS_MODULE(moduleValue)->name->chars : "?",
                                 member->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_IMPORT: {
                ObjString* path = AS_STRING(pop());
                bool alreadyLoaded = false;
                Value dummy;
                if (tableGet(&vm.loadedModules, path, &dummy)) {
                    alreadyLoaded = true;
                }
                if (!importModule(path)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (alreadyLoaded) {
                    // No module frame was pushed; supply the value that the
                    // trailing OP_POP expects.
                    push(KOSONG_VAL);
                } else {
                    // A new frame is now executing the module.
                    frame = &vm.frames[vm.frameCount - 1];
                }
                break;
            }
            case OP_YIELD: {
                // Suspend this frame into its generator and return the value
                // to whoever resumed it. vm.currentGenerator is set by
                // resumeGenerator(), which is the only way this frame started.
                ObjGenerator* gen = vm.currentGenerator;
                if (gen == NULL) {
                    runtimeError("'hasilkan' dijalankan di luar generator.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value yielded = pop();
                int live = (int)(vm.stackTop - frame->slots);
                if (live > GEN_SLOTS_MAX) {
                    runtimeError("Generator memakai terlalu banyak slot (%d > %d) saat 'hasilkan'.",
                                 live, GEN_SLOTS_MAX);
                    return INTERPRET_RUNTIME_ERROR;
                }
                // An upvalue closed over a slot we are about to copy away must
                // be closed now, or it would point into reusable stack space.
                closeUpvalues(frame->slots);
                for (int i = 0; i < live; i++) gen->slots[i] = frame->slots[i];
                gen->slotCount = live;
                gen->ip = frame->ip;
                gen->state = GEN_TERTUNDA;

                vm.frameCount--;
                vm.stackTop = frame->slots;
                push(yielded);
                return INTERPRET_OK;      // back to resumeGenerator()
            }
            case OP_RETURN: {
                Value result = pop();
                closeUpvalues(frame->slots);
                // 'kembalikan' from inside 'coba' skips OP_TRY_END; drop the
                // handlers registered by this frame so a later error cannot
                // jump back into a function that has already returned.
                while (vm.tryCount > 0 &&
                       vm.tryHandlers[vm.tryCount - 1].frameCount >= vm.frameCount) {
                    vm.tryCount--;
                }
                vm.frameCount--;
                if (vm.frameCount == vm.exitFrame) {
                    if (vm.exitFrame == 0) {
                        pop();
                        return INTERPRET_OK;
                    }
                    // Returning to a native that invoked this callable from C:
                    // leave just the result where the callee sat.
                    vm.stackTop = frame->slots;
                    push(result);
                    return INTERPRET_OK;
                }

                vm.stackTop = frame->slots;
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
#undef DISPATCH_MAGIC
#undef RESOLVE_MAGIC_ERROR
}

// Advance a generator by one step. Returns:
//   1  with *result set to the yielded value
//   0  the generator finished (exhausted); *result is kosong
//  -1  an error was raised (pending as a native error)
//
// The suspended frame is rebuilt on top of the current stack, then run() is
// confined to it with vm.exitFrame exactly as callFromNative does.
int resumeGenerator(ObjGenerator* gen, Value* result) {
    *result = KOSONG_VAL;
    if (gen->state == GEN_SELESAI) return 0;
    if (gen->state == GEN_BERJALAN) {
        nativeRaiseKelas(vm.galatNilai, "Generator sedang berjalan; tidak bisa dilanjutkan ulang.");
        return -1;
    }
    if (vm.frameCount >= FRAMES_MAX) {
        nativeRaiseKelas(vm.galatRekursi,
                         "Kedalaman panggilan maksimal %d terlampaui saat melanjutkan generator.",
                         FRAMES_MAX);
        return -1;
    }
    if (vm.stackTop + gen->slotCount + 1 >= vm.stack + STACK_MAX) {
        nativeRaiseKelas(vm.galatRekursi, "Stack penuh saat melanjutkan generator.");
        return -1;
    }

    Value* base = vm.stackTop;
    for (int i = 0; i < gen->slotCount; i++) push(gen->slots[i]);

    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->closure = gen->closure;
    frame->ip = gen->state == GEN_BELUM_MULAI ? gen->closure->function->chunk.code : gen->ip;
    frame->slots = base;

    int frameBefore = vm.frameCount - 1;
    int savedExit = vm.exitFrame;
    ObjGenerator* savedGen = vm.currentGenerator;
    vm.exitFrame = frameBefore;
    vm.currentGenerator = gen;
    gen->state = GEN_BERJALAN;
    // Same contract as callFromNative: errors raised inside are only recorded,
    // then converted to a catchable native error below.
    vm.nativeCallDepth++;

    InterpretResult ir = run();

    vm.nativeCallDepth--;
    vm.exitFrame = savedExit;
    vm.currentGenerator = savedGen;

    if (ir != INTERPRET_OK) {
        // The body raised. The generator is dead; unwind whatever is left of
        // its frame so the caller's stack is exactly as it was.
        if (vm.runtimeErrorPending) {
            vm.runtimeErrorPending = false;
            vm.hadError = false;
            ObjClass* klass = vm.runtimeErrorClass != NULL ? vm.runtimeErrorClass : vm.galat;
            vm.runtimeErrorClass = NULL;
            vm.nativeErrorValue = OBJ_VAL(newError(klass, vm.runtimeErrorMessage));
            vm.nativeErrorPending = true;
        }
        gen->state = GEN_SELESAI;
        gen->slotCount = 0;
        if (vm.frameCount > frameBefore) vm.frameCount = frameBefore;
        // Drop any try handlers the dead frame registered.
        while (vm.tryCount > 0 && vm.tryHandlers[vm.tryCount - 1].frameCount > frameBefore) {
            vm.tryCount--;
        }
        vm.stackTop = base;
        return -1;
    }

    if (vm.nativeErrorPending) {   // raised and already converted inside
        gen->state = GEN_SELESAI;
        gen->slotCount = 0;
        if (vm.frameCount > frameBefore) vm.frameCount = frameBefore;
        vm.stackTop = base;
        return -1;
    }

    Value produced = pop();
    vm.stackTop = base;

    if (gen->state == GEN_TERTUNDA) {   // OP_YIELD ran
        *result = produced;
        return 1;
    }
    // OP_RETURN ran: the body is over. The returned value is discarded, as in
    // Python, where it only travels inside StopIteration.
    gen->state = GEN_SELESAI;
    gen->slotCount = 0;
    return 0;
}

// Call an Indonesiaku callable from inside a native. The callee and its
// arguments must already be on the VM stack. On success they are replaced by
// the result (also stored in *result). Any error is converted into a pending
// native error, so the native's own caller unwinds normally.
bool callFromNative(int argCount, Value* result) {
    int frameBefore = vm.frameCount;
    Value* calleeSlot = vm.stackTop - argCount - 1;
    Value callee = *calleeSlot;

    if (vm.nativeCallDepth >= NATIVE_CALL_MAX) {
        vm.stackTop = calleeSlot;
        nativeRaiseKelas(vm.galatRekursi,
                         "Kedalaman panggilan balik dari fungsi bawaan melebihi %d. "
                         "Kemungkinan rekursi tak terbatas, misalnya _teks_ yang memanggil cetak(diri)?",
                         NATIVE_CALL_MAX);
        return false;
    }

    // Raised while this flag is up, runtimeError only records the message;
    // we convert it to a catchable native error below.
    vm.nativeCallDepth++;

    if (!callValue(callee, argCount, NULL)) {
        vm.nativeCallDepth--;
        // callValue already raised via runtimeError; it was only recorded
        // (nativeCallDepth/tryCount), convert it to a native error below.
        if (vm.runtimeErrorPending) {
            vm.runtimeErrorPending = false;
            vm.hadError = false;
            ObjClass* klass = vm.runtimeErrorClass != NULL ? vm.runtimeErrorClass : vm.galat;
            vm.runtimeErrorClass = NULL;
            vm.nativeErrorValue = OBJ_VAL(newError(klass, vm.runtimeErrorMessage));
            vm.nativeErrorPending = true;
        }
        vm.stackTop = calleeSlot;
        return false;
    }

    if (vm.frameCount == frameBefore) {
        // A native callee: callValue already left the result on the stack.
        vm.nativeCallDepth--;
        if (vm.nativeErrorPending) { vm.stackTop = calleeSlot; return false; }
        *result = pop();
        return true;
    }

    int savedExit = vm.exitFrame;
    vm.exitFrame = frameBefore;
    InterpretResult r = run();
    vm.nativeCallDepth--;
    vm.exitFrame = savedExit;

    if (r != INTERPRET_OK) {
        if (vm.runtimeErrorPending) {
            vm.runtimeErrorPending = false;
            vm.hadError = false;
            ObjClass* klass = vm.runtimeErrorClass != NULL ? vm.runtimeErrorClass : vm.galat;
            vm.runtimeErrorClass = NULL;
            vm.nativeErrorValue = OBJ_VAL(newError(klass, vm.runtimeErrorMessage));
            vm.nativeErrorPending = true;
        }
        // Drop any frames and handlers the aborted callback left behind.
        vm.frameCount = frameBefore;
        while (vm.tryCount > 0 && vm.tryHandlers[vm.tryCount - 1].frameCount > frameBefore) {
            vm.tryCount--;
        }
        vm.stackTop = calleeSlot;
        return false;
    }

    *result = pop();
    return true;
}

void setScriptPath(const char* path) {
    char dir[512];
    dirNameOf(path, dir, sizeof dir);
    vm.scriptDir = dir[0] != '\0' ? copyString(dir, (int)strlen(dir)) : NULL;
}

// indk uji: run every top-level function named uji_* and report pass/fail.
// Assertions reuse `pastikan`; a failing one raises a catchable error, which
// callFromNative hands back here as nativeErrorPending.
// Returns the number of FAILED tests (0 = all green), or -1 if there are none.
int runUnitTests(void) {
    if (vm.mainModule == NULL) return -1;

    // Collect matching names first: calling a test can define new globals and
    // grow the table, which would invalidate a live iteration over it.
    ObjList* names = newList();
    push(OBJ_VAL(names));
    Table* globals = &vm.mainModule->globals;
    for (int i = 0; i < globals->capacity; i++) {
        Entry* e = &globals->entries[i];
        if (e->key == NULL) continue;
        if (strncmp(e->key->chars, "uji_", 4) != 0) continue;
        if (!IS_CLOSURE(e->value) && !IS_FUNCTION(e->value)) continue;
        writeValueArray(&names->items, OBJ_VAL(e->key));
    }

    int total = names->items.count;
    if (total == 0) { pop(); return -1; }

    // Table order is a hash order: it varies and makes output irreproducible.
    // Sort by name so a test report is stable and diffable.
    for (int i = 1; i < total; i++) {
        Value key = names->items.values[i];
        int j = i - 1;
        while (j >= 0 && strcmp(AS_STRING(names->items.values[j])->chars,
                                AS_STRING(key)->chars) > 0) {
            names->items.values[j + 1] = names->items.values[j];
            j--;
        }
        names->items.values[j + 1] = key;
    }

    int failed = 0;
    for (int i = 0; i < total; i++) {
        ObjString* name = AS_STRING(names->items.values[i]);
        Value fn;
        if (!tableGet(globals, name, &fn)) continue;

        // Remember the stack top: on failure callFromNative unwinds to the
        // callee slot, but a raised error can leave the stack deeper than we
        // started. Restoring it explicitly keeps each test independent.
        Value* stackBefore = vm.stackTop;
        push(fn);                       // callee, no arguments
        Value result;
        bool ok = callFromNative(0, &result);
        vm.stackTop = stackBefore;

        if (ok) {
            printf("  LULUS  %s\n", name->chars);
        } else {
            failed++;
            const char* detail = "galat";
            if (vm.nativeErrorPending && IS_INSTANCE(vm.nativeErrorValue)) {
                ObjString* msg = errorMessage(AS_INSTANCE(vm.nativeErrorValue));
                if (msg != NULL) detail = msg->chars;
            }
            printf("  GAGAL  %s: %s\n", name->chars, detail);
            // Clear the error so the next test starts clean.
            vm.nativeErrorPending = false;
            vm.nativeErrorValue = KOSONG_VAL;
            vm.hadError = false;
            vm.runtimeErrorPending = false;
        }
    }

    printf("\n%d/%d uji lulus\n", total - failed, total);
    pop();                              // names
    return failed;
}

InterpretResult interpret(const char* source) {
    vm.hadError = false;
    // Every execution runs inside a module; the entry script's is "__utama__",
    // so `jika __nama__ == "__utama__"` works and global lookup has no special
    // case for "no module". Created once and reused by the REPL.
    if (vm.mainModule == NULL) {
        ObjString* name = copyString("__utama__", 9);
        push(OBJ_VAL(name));
        // path stays NULL: scriptDir is a DIRECTORY, and loadModule would run
        // dirNameOf() over it and strip one component too many. The NULL makes
        // loadModule fall back to vm.scriptDir directly, which is correct for
        // both a script and the REPL (cwd).
        ObjModule* main = newModule(name, NULL);
        push(OBJ_VAL(main));
        // Root the key: tableSet may grow the table, and growing allocates,
        // which under DEBUG_STRESS_GC collects and would free this very string
        // before findEntry reads it (AGENTS.md GC-safety rule).
        ObjString* namaKey = copyString("__nama__", 8);
        push(OBJ_VAL(namaKey));
        tableSet(&main->globals, namaKey, OBJ_VAL(name));
        pop();
        vm.mainModule = main;
        pop();
        pop();
    }

    ObjFunction* function = compileInModule(source, vm.mainModule);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));
    ObjClosure* closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    call(closure, 0, NULL);

    InterpretResult result = run();
    // A runtimeError() inside 'coba' stops run() with a pending error:
    // unwind to the handler, hand it the message, and keep running.
    while (result == INTERPRET_RUNTIME_ERROR && vm.runtimeErrorPending) {
        vm.runtimeErrorPending = false;
        vm.hadError = false;
        if (!throwValue(KOSONG_VAL)) {  // unreachable: a handler existed
            reportError(vm.runtimeErrorMessage);
            break;
        }
        const char* message = vm.runtimeErrorMessage;
        ObjClass* klass = vm.runtimeErrorClass != NULL ? vm.runtimeErrorClass : vm.galat;
        vm.runtimeErrorClass = NULL;
        push(OBJ_VAL(newError(klass, message)));
        result = run();
    }
    return result;
}
