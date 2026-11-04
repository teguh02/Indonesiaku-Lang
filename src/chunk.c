#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    chunk->columns = NULL;
    chunk->source = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    FREE_ARRAY(int, chunk->columns, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line, int column) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code,
                                oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines,
                                 oldCapacity, chunk->capacity);
        chunk->columns = GROW_ARRAY(int, chunk->columns,
                                    oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->columns[chunk->count] = column;
    chunk->count++;
}

int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}

// Nama opcode, diturunkan dari enum OpCode di chunk.h lewat stringify supaya
// tidak bisa jadi tak sinkron diam-diam: menambah opcode tanpa menambahkannya
// di sini memicu _Static_assert di bawah, bukan nama "?" saat program jalan.
#define NAMA(op) [op] = #op
static const char* const namaOpcode[OP__COUNT] = {
    NAMA(OP_CONSTANT), NAMA(OP_KOSONG), NAMA(OP_BENAR),
    NAMA(OP_SALAH), NAMA(OP_POP), NAMA(OP_GET_LOCAL),
    NAMA(OP_SET_LOCAL), NAMA(OP_GET_GLOBAL), NAMA(OP_DEFINE_GLOBAL),
    NAMA(OP_SET_GLOBAL), NAMA(OP_EQUAL), NAMA(OP_IDENTICAL),
    NAMA(OP_GREATER), NAMA(OP_LESS), NAMA(OP_NOT_EQUAL),
    NAMA(OP_GREATER_EQUAL), NAMA(OP_LESS_EQUAL), NAMA(OP_ADD),
    NAMA(OP_SUBTRACT), NAMA(OP_MULTIPLY), NAMA(OP_DIVIDE),
    NAMA(OP_MODULO), NAMA(OP_POWER), NAMA(OP_FLOOR_DIVIDE),
    NAMA(OP_BIT_AND), NAMA(OP_BIT_OR), NAMA(OP_BIT_XOR),
    NAMA(OP_BIT_NOT), NAMA(OP_SHIFT_LEFT), NAMA(OP_SHIFT_RIGHT),
    NAMA(OP_IN), NAMA(OP_CHAIN_COMPARE), NAMA(OP_CHAIN_CLEANUP),
    NAMA(OP_NOT), NAMA(OP_NEGATE), NAMA(OP_JUMP),
    NAMA(OP_JUMP_IF_FALSE), NAMA(OP_LOOP), NAMA(OP_CALL),
    NAMA(OP_CALL_BERNAMA), NAMA(OP_ISI_BAWAAN), NAMA(OP_CLOSURE),
    NAMA(OP_GET_UPVALUE), NAMA(OP_SET_UPVALUE), NAMA(OP_CLOSE_UPVALUE),
    NAMA(OP_BUILD_LIST), NAMA(OP_BUILD_TUPLE), NAMA(OP_BUILD_DICT),
    NAMA(OP_APPEND), NAMA(OP_DICT_PUT), NAMA(OP_SET_PUT),
    NAMA(OP_BUILD_SET), NAMA(OP_DELETE_GLOBAL), NAMA(OP_DELETE_LOCAL),
    NAMA(OP_DELETE_UPVALUE), NAMA(OP_INDEX_DELETE), NAMA(OP_UNPACK),
    NAMA(OP_DELETE_PROPERTY), NAMA(OP_INDEX_GET), NAMA(OP_SEQ_GET),
    NAMA(OP_ITER_OBJEK), NAMA(OP_ITER_NEXT), NAMA(OP_YIELD),
    NAMA(OP_CLASS_FIELD), NAMA(OP_INDEX_SET), NAMA(OP_SLICE),
    NAMA(OP_LIST_LEN), NAMA(OP_CLASS), NAMA(OP_INHERIT),
    NAMA(OP_METHOD), NAMA(OP_SPECIAL_METHOD), NAMA(OP_GET_PROPERTY),
    NAMA(OP_SET_PROPERTY), NAMA(OP_INVOKE), NAMA(OP_GET_SUPER),
    NAMA(OP_SUPER_INVOKE), NAMA(OP_TRY_BEGIN), NAMA(OP_TRY_END),
    NAMA(OP_THROW), NAMA(OP_PASTIKAN), NAMA(OP_IS_INSTANCE),
    NAMA(OP_CATCH_TEST), NAMA(OP_IMPORT), NAMA(OP_IMPORT_MODULE),
    NAMA(OP_IMPORT_FROM), NAMA(OP_RETURN)
};
#undef NAMA

// Opcode baru yang lupa didaftarkan akan mencetak "OP_?" di laporan profil --
// diam dan membingungkan. _Static_assert tidak bisa dipakai di sini (isi larik
// bukan ekspresi konstanta bagi C), jadi pemeriksaannya dilakukan sekali saat
// `indk profil` mulai; lihat mulaiProfil() di vm.c.
bool opcodeNamesLengkap(int* pertamaKosong) {
    for (int i = 0; i < OP__COUNT; i++) {
        if (namaOpcode[i] == NULL) {
            if (pertamaKosong != NULL) *pertamaKosong = i;
            return false;
        }
    }
    return true;
}

const char* opcodeName(uint8_t op) {
    if (op >= OP__COUNT || namaOpcode[op] == NULL) return "OP_?";
    return namaOpcode[op];
}
