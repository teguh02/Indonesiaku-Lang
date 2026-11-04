#ifndef indonesiaku_chunk_h
#define indonesiaku_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_KOSONG,
    OP_BENAR,
    OP_SALAH,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_EQUAL,
    OP_IDENTICAL,
    OP_GREATER,
    OP_LESS,
    // Own opcodes instead of `NOT <`/`NOT >`/`NOT ==`: negation is wrong for
    // NaN (nan >= 1 is salah) and would call the wrong magic method (K19).
    OP_NOT_EQUAL,
    OP_GREATER_EQUAL,
    OP_LESS_EQUAL,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MODULO,
    OP_POWER,
    OP_FLOOR_DIVIDE,
    OP_BIT_AND,
    OP_BIT_OR,
    OP_BIT_XOR,
    OP_BIT_NOT,
    OP_SHIFT_LEFT,
    OP_SHIFT_RIGHT,
    OP_IN,
    OP_CHAIN_COMPARE,
    OP_CHAIN_CLEANUP,
    OP_NOT,
    OP_NEGATE,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_CALL_BERNAMA,
    OP_ISI_BAWAAN,
    OP_CLOSURE,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_CLOSE_UPVALUE,
    OP_BUILD_LIST,
    OP_BUILD_TUPLE,  // byte = jumlah elemen; (a, b) / (a,) / ()
    OP_BUILD_DICT,
    OP_APPEND,       // [daftar, nilai] -> daftar (list comprehension)
    OP_DICT_PUT,     // [kamus, kunci, nilai] -> kamus (dict comprehension)
    OP_SET_PUT,      // [himpunan, nilai] -> himpunan (set comprehension)
    OP_BUILD_SET,    // himpunan kosong baru (akumulator set comprehension)
    OP_DELETE_GLOBAL,   // buang nama; byte = nama
    OP_DELETE_LOCAL,    // buang nama; byte = slot lokal (diisi VAL_TAKTERISI)
    OP_DELETE_UPVALUE,  // buang nama; byte = upvalue
    OP_INDEX_DELETE,    // buang d[i] / kamus[k]
    OP_UNPACK,          // a, b = ... : byte = jumlah nama
    OP_DELETE_PROPERTY, // buang obj.atribut
    OP_INDEX_GET,
    OP_SEQ_GET,      // iteration by position: kamus -> kunci ke-i (not a key lookup)
    OP_ITER_OBJEK,   // once before a loop: objek dengan _iter_ -> hasilnya
    OP_ITER_NEXT,    // iterator protocol or indexed sequence fallback
    OP_YIELD,        // hasilkan: suspend the generator frame, return the value
    OP_CLASS_FIELD,  // atribut kelas: NAMA = nilai di badan kelas
    OP_INDEX_SET,
    OP_SLICE,
    OP_LIST_LEN,
    OP_CLASS,
    OP_INHERIT,
    OP_METHOD,
    OP_SPECIAL_METHOD,   // operand: nama, lalu SPECIAL_* (properti/statis/kelas)
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_INVOKE,
    OP_GET_SUPER,
    OP_SUPER_INVOKE,
    OP_TRY_BEGIN,
    OP_TRY_END,
    OP_THROW,
    OP_PASTIKAN,
    OP_IS_INSTANCE,
    // kecuali X { ... } tanpa 'sebagai': konstanta = nama X. Memutuskan saat
    // jalan apakah X adalah kelas galat (klausa bertipe) atau nama pengikat
    // baru (klausa tangkap-semua). Lihat catchClause() di compiler.c.
    OP_CATCH_TEST,
    OP_IMPORT,
    OP_IMPORT_MODULE,   // impor nama; konstanta = nama modul -> ObjModule di stack
    OP_IMPORT_FROM,     // dari m impor x; konstanta = nama anggota
    OP_RETURN,
    OP__COUNT,   // jumlah opcode; bukan instruksi -- dipakai larik profil
} OpCode;

// Nama opcode untuk `indk profil`. Disassembler punya tabelnya sendiri yang
// menyatu dengan pencetakan operand, jadi tidak bisa dipakai ulang di sini.
const char* opcodeName(uint8_t op);
// Benar jika setiap opcode punya nama. Dipakai `indk profil` untuk menolak
// laporan yang akan berisi "OP_?" karena opcode baru lupa didaftarkan.
bool opcodeNamesLengkap(int* pertamaKosong);

typedef enum {
    CHAIN_EQUAL,
    CHAIN_NOT_EQUAL,
    CHAIN_GREATER,
    CHAIN_LESS,
    CHAIN_GREATER_EQUAL,
    CHAIN_LESS_EQUAL,
    CHAIN_IN,
    CHAIN_NOT_IN,
    CHAIN_IDENTITY,
    CHAIN_NOT_IDENTITY,
} ChainCompare;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    // ponytail: 4 bytes per bytecode for runtime error columns. Worth it for
    // a caret under the failing expression; pack with lines into one struct
    // array if chunk memory ever matters.
    int* columns;
    // Source text this chunk was compiled from, so runtime errors can print
    // the offending line. An ObjString (not a raw char*) so the GC keeps it
    // alive exactly as long as the function needs it -- module sources used
    // to be freed right after compile(). Every function from one compile()
    // shares this pointer; NULL when unavailable.
    struct ObjString* source;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line, int column);
int addConstant(Chunk* chunk, Value value);

#endif
