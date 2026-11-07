#ifndef indonesiaku_vm_h
#define indonesiaku_vm_h

#include "object.h"
#include "table.h"
#include "value.h"

// Maximum call depth, matching Python's default recursion limit. The value
// stack is sized for the worst case (256 slots per frame) and lives in the
// global `vm`, so it costs static memory, not C stack.
#define FRAMES_MAX 1000
// Jumlah breakpoint maksimum untuk `indk awakutu`. Larik tetap: daftar
// breakpoint selalu kecil dan dimasukkan manusia lewat perintah.
#define DEBUG_BREAK_MAX 64
// Nesting limit for natives calling back into Indonesiaku (callFromNative):
// each level costs a C run() frame plus the native's own C frame, so the C
// stack would overflow long before FRAMES_MAX. _teks_ calling cetak(diri) is
// the classic way to get here.
#define NATIVE_CALL_MAX 200
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)
// Batas blok 'coba' aktif sekaligus. Disamakan dengan FRAMES_MAX supaya
// try-handler tidak pernah jadi batas yang lebih ketat daripada kedalaman
// rekursi itu sendiri: nilainya dulu 64, jadi `coba` di dalam fungsi
// rekursif berhenti di kedalaman 64 padahal rekursi biasa sanggup 1000.
// Biaya memorinya 24 B x 1000 = 23 KB, sepele dibanding VM yang sudah 4 MB.
#define TRY_MAX FRAMES_MAX

// Stack protection checks
#define STACK_UNDERFLOW_CHECK(count) \
    do { \
        if (vm.stackTop - vm.stack < (count)) { \
            fatalError("Stack underflow: attempted to pop %d values but only %td available", \
                        (count), (vm.stackTop - vm.stack)); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
    } while (false)

#define STACK_OVERFLOW_CHECK() \
    do { \
        if (vm.stackTop >= vm.stack + STACK_MAX) { \
            runtimeError("Stack overflow: maximum stack depth %d exceeded", STACK_MAX); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
    } while (false)

#define FRAME_OVERFLOW_CHECK() \
    do { \
        if (vm.frameCount >= FRAMES_MAX) { \
            runtimeError("Frame overflow: maximum call depth %d exceeded", FRAMES_MAX); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
    } while (false)

typedef struct {
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
} CallFrame;

// Records an active 'coba' (try) block so a raised error can unwind to its
// 'kecuali' (catch) handler.
typedef struct {
    uint8_t* handlerIp;   // where the catch handler begins
    int frameCount;       // frame depth when the try block was entered
    Value* stackTop;      // stack top when the try block was entered
} TryHandler;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    Value stack[STACK_MAX];
    Value* stackTop;
    int frameCount;
    TryHandler tryHandlers[TRY_MAX];
    int tryCount;
    int nativeCallDepth;   // >0 while a native is running an Indonesiaku callback
    // The generator whose frame is currently executing, or NULL. OP_YIELD
    // suspends into it; saved/restored around every resume so a generator
    // iterating another generator cannot suspend the wrong one.
    ObjGenerator* currentGenerator;
    // Frame depth at which run() must stop and return instead of continuing.
    // 0 at top level; raised by callFromNative() so a callback invoked from C
    // returns to its C caller instead of running the rest of the program.
    int exitFrame;
    Table globals;
    Table strings;
    Table loadedModules;   // resolved path -> ObjModule
    ObjModule* mainModule; // the entry script's module ("__utama__")
    ObjString* scriptDir;  // directory of the entry script, for import search
    ObjString* initString;
    ObjString* magicNames[MAGIC_COUNT];   // K19, interned; see MAGIC_NAMES in vm.c
    ObjClass* galat;
    ObjClass* galatAritmetika;
    ObjClass* galatBagiNol;
    ObjClass* galatLuapan;
    ObjClass* galatTipe;
    ObjClass* galatNilai;
    ObjClass* galatIndeks;
    ObjClass* galatKunci;
    ObjClass* galatNama;
    ObjClass* galatPenegasan;
    ObjClass* galatAtribut;
    ObjClass* galatRekursi;
    ObjClass* hentiIterasi;
    ObjClass* galatOS;     // Python OSError
    ObjClass* galatIzin;   // PermissionError: refused under --sandbox

    // Keyword argument names for the native call currently running, or NULL.
    // Only natives that opt in (see keywordNatives in vm.c) ever see these;
    // every other native still rejects keywords exactly as before.
    ObjList* nativeArgNames;
    ObjUpvalue* openUpvalues;
    Obj* objects;
    bool hadError;

    // Set by a native function that wants to raise a *catchable* error
    // (via nativeRaise). callValue converts it into a throwValue unwind.
    bool nativeErrorPending;
    Value nativeErrorValue;

    // Set by runtimeError() when a 'coba' handler is active: the error is
    // caught instead of aborting. interpret() unwinds to the handler and
    // resumes run() with the message as the caught value.
    bool runtimeErrorPending;
    ObjClass* runtimeErrorClass;
    char runtimeErrorMessage[512];

    // Garbage collector bookkeeping
    size_t bytesAllocated;
    size_t nextGC;
    int grayCount;
    int grayCapacity;
    Obj** grayStack;

    // `indk profil`: histogram opcode + hitung panggilan fungsi. Dimatikan
    // (profiling = false) di jalur normal, jadi run() hanya membayar satu
    // tes boolean per instruksi. Counter, bukan sampling waktu: hasilnya
    // deterministik dan tahan terhadap unwinding galat/generator.
    bool profiling;
    uint64_t opCounts[OP__COUNT];
    uint64_t callCount;

    // `indk awakutu`: debugger CLI. Dimatikan di jalur normal, jadi run()
    // hanya membayar satu tes boolean per instruksi -- sama seperti profil.
    bool debugging;
    bool debugStep;              // berhenti di instruksi berikutnya (langkah)
    int debugStepFrame;          // kedalaman frame saat 'langkah' diminta
    int debugBreakLines[DEBUG_BREAK_MAX];
    int debugBreakCount;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
void defineArgs(int argc, const char* argv[], int start);
// Directory of the entry script, used as the first import search path.
void setScriptPath(const char* path);
// `indk profil`: hidupkan penghitung sebelum interpret(), lalu cetak laporan.
void mulaiProfil(void);
// `indk awakutu`: jalankan skrip di bawah kendali debugger CLI.
void mulaiAwakutu(void);
void cetakProfil(double detik);
InterpretResult interpret(const char* source);

// Mendaftarkan builtin dari C. Dipakai initVM() sendiri dan API embedding
// (src/indk_api.c); lihat src/indk.h.
void defineNative(const char* name, NativeFn function);

// indk uji: run top-level uji_* functions. Returns failures, or -1 if none found.
int runUnitTests(void);
void push(Value value);
Value pop();
void runtimeError(const char* format, ...);
void fatalError(const char* format, ...);
void nativeRaise(const char* format, ...);
void nativeRaiseKelas(ObjClass* klass, const char* format, ...);
// Call an Indonesiaku callable from inside a native function. `callee` and
// `argCount` arguments must already be on the VM stack (callee first); on
// success they are replaced by the single result value, which is also
// returned. Returns false after flagging the error (nativeRaise-style) when
// the call fails or raises.
bool callFromNative(int argCount, Value* result);
// Advance a generator one step: 1 = yielded (*result set), 0 = exhausted,
// -1 = raised (pending native error).
int resumeGenerator(ObjGenerator* gen, Value* result);
bool valueIsInstanceOf(Value value, Value klass);
bool isFalsey(Value value);
// _panjang_ of an instance, run now: 1 with *out set, 0 when it raised or
// returned an invalid length (error pending), -1 when undefined.
int instanceLengthFor(Value value, int64_t* out);
// a < b for instances, run now: 1 with *less set, 0 when _kurang_dari_ raised
// (error pending), -1 when neither operand defines an ordering.
int instanceLessFor(Value a, Value b, bool* less);
// _iter_()/_berikutnya_() run now: 1 with *result set, 0 when the method
// raised (error pending), -1 when the class does not define it.
int instanceIterFor(Value value, Value* result);
int instanceNextFor(Value value, Value* result);
bool instanceHasMagic(Value value, MagicKind kind);

// Ancestor walk (MRO when set, superclass chain otherwise). Shared with
// native.c so turunan_dari() sees multiple inheritance too.
typedef struct {
    ObjClass* klass;
    ObjClass* next;
    ObjClass* owner;
    int index;
} AncestorIter;

AncestorIter ancestors(ObjClass* klass);
bool ancestorNext(AncestorIter* it);

#endif
