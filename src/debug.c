#include <stdio.h>

#include "debug.h"
#include "object.h"
#include "value.h"

void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int byteInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static int invokeInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    uint8_t argCount = chunk->code[offset + 2];
    printf("%-16s (%d args) %4d '", name, argCount, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 3;
}

static int jumpInstruction(const char* name, int sign, Chunk* chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

int disassembleInstruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_KOSONG:
            return simpleInstruction("OP_KOSONG", offset);
        case OP_BENAR:
            return simpleInstruction("OP_BENAR", offset);
        case OP_SALAH:
            return simpleInstruction("OP_SALAH", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_GET_LOCAL:
            return byteInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:
            return byteInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_IDENTICAL:
            return simpleInstruction("OP_IDENTICAL", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_NOT_EQUAL:
            return simpleInstruction("OP_NOT_EQUAL", offset);
        case OP_GREATER_EQUAL:
            return simpleInstruction("OP_GREATER_EQUAL", offset);
        case OP_LESS_EQUAL:
            return simpleInstruction("OP_LESS_EQUAL", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_MODULO:
            return simpleInstruction("OP_MODULO", offset);
        case OP_POWER:
            return simpleInstruction("OP_POWER", offset);
        case OP_FLOOR_DIVIDE:
            return simpleInstruction("OP_FLOOR_DIVIDE", offset);
        case OP_BIT_AND:
            return simpleInstruction("OP_BIT_AND", offset);
        case OP_BIT_OR:
            return simpleInstruction("OP_BIT_OR", offset);
        case OP_BIT_XOR:
            return simpleInstruction("OP_BIT_XOR", offset);
        case OP_BIT_NOT:
            return simpleInstruction("OP_BIT_NOT", offset);
        case OP_SHIFT_LEFT:
            return simpleInstruction("OP_SHIFT_LEFT", offset);
        case OP_SHIFT_RIGHT:
            return simpleInstruction("OP_SHIFT_RIGHT", offset);
        case OP_IN:
            return simpleInstruction("OP_IN", offset);
        case OP_CHAIN_COMPARE:
            return byteInstruction("OP_CHAIN_COMPARE", chunk, offset);
        case OP_CHAIN_CLEANUP:
            return simpleInstruction("OP_CHAIN_CLEANUP", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_LOOP:
            return jumpInstruction("OP_LOOP", -1, chunk, offset);
        case OP_CALL:
            return byteInstruction("OP_CALL", chunk, offset);
        case OP_CALL_BERNAMA: {
            // argCount, then the constant index of the keyword-name list.
            uint8_t argCount = chunk->code[offset + 1];
            uint8_t names = chunk->code[offset + 2];
            printf("%-16s %4d names %d '", "OP_CALL_BERNAMA", argCount, names);
            printValue(chunk->constants.values[names]);
            printf("'\n");
            return offset + 3;
        }
        case OP_ISI_BAWAAN: {
            // slot, then a 16-bit forward jump taken when the slot is filled.
            uint8_t slot = chunk->code[offset + 1];
            uint16_t jump = (uint16_t)(chunk->code[offset + 2] << 8);
            jump |= chunk->code[offset + 3];
            printf("%-16s %4d -> %d\n", "OP_ISI_BAWAAN", slot, offset + 4 + jump);
            return offset + 4;
        }
        case OP_CLOSURE: {
            offset++;
            uint8_t constant = chunk->code[offset++];
            printf("%-16s %4d '", "OP_CLOSURE", constant);
            printValue(chunk->constants.values[constant]);
            printf("'\n");

            ObjFunction* function = AS_FUNCTION(chunk->constants.values[constant]);
            for (int j = 0; j < function->upvalueCount; j++) {
                int isLocal = chunk->code[offset++];
                int index = chunk->code[offset++];
                printf("%04d      |                     %s %d\n",
                       offset - 2, isLocal ? "local" : "upvalue", index);
            }
            return offset;
        }
        case OP_GET_UPVALUE:
            return byteInstruction("OP_GET_UPVALUE", chunk, offset);
        case OP_SET_UPVALUE:
            return byteInstruction("OP_SET_UPVALUE", chunk, offset);
        case OP_CLOSE_UPVALUE:
            return simpleInstruction("OP_CLOSE_UPVALUE", offset);
        case OP_BUILD_TUPLE:
            return byteInstruction("OP_BUILD_TUPLE", chunk, offset);
        case OP_BUILD_LIST:
            return byteInstruction("OP_BUILD_LIST", chunk, offset);
        case OP_BUILD_DICT:
            return byteInstruction("OP_BUILD_DICT", chunk, offset);
        case OP_APPEND:
            return simpleInstruction("OP_APPEND", offset);
        case OP_DELETE_GLOBAL:
            return constantInstruction("OP_DELETE_GLOBAL", chunk, offset);
        case OP_DELETE_LOCAL:
        case OP_DELETE_UPVALUE:
            return byteInstruction(chunk->code[offset] == OP_DELETE_LOCAL
                                       ? "OP_DELETE_LOCAL" : "OP_DELETE_UPVALUE",
                                   chunk, offset);
        case OP_INDEX_DELETE:
            return simpleInstruction("OP_INDEX_DELETE", offset);
        case OP_UNPACK:
            return byteInstruction("OP_UNPACK", chunk, offset);
        case OP_DELETE_PROPERTY:
            return constantInstruction("OP_DELETE_PROPERTY", chunk, offset);
        case OP_INDEX_GET:
            return simpleInstruction("OP_INDEX_GET", offset);
        case OP_SEQ_GET:
            return simpleInstruction("OP_SEQ_GET", offset);
        case OP_ITER_OBJEK:
            return simpleInstruction("OP_ITER_OBJEK", offset);
        case OP_ITER_NEXT:
            return simpleInstruction("OP_ITER_NEXT", offset);
        case OP_YIELD:
            return simpleInstruction("OP_YIELD", offset);
        case OP_DICT_PUT:
            return simpleInstruction("OP_DICT_PUT", offset);
        case OP_SET_PUT:
            return simpleInstruction("OP_SET_PUT", offset);
        case OP_BUILD_SET:
            return simpleInstruction("OP_BUILD_SET", offset);
        case OP_CLASS_FIELD:
            return constantInstruction("OP_CLASS_FIELD", chunk, offset);
        case OP_INDEX_SET:
            return simpleInstruction("OP_INDEX_SET", offset);
        case OP_SLICE:
            return simpleInstruction("OP_SLICE", offset);
        case OP_LIST_LEN:
            return simpleInstruction("OP_LIST_LEN", offset);
        case OP_CLASS:
            return constantInstruction("OP_CLASS", chunk, offset);
        case OP_INHERIT:
            return byteInstruction("OP_INHERIT", chunk, offset);
        case OP_METHOD:
            return constantInstruction("OP_METHOD", chunk, offset);
        case OP_SPECIAL_METHOD: {
            uint8_t constant = chunk->code[offset + 1];
            uint8_t kind = chunk->code[offset + 2];
            printf("%-16s %4d '", "OP_SPECIAL_METHOD", constant);
            printValue(chunk->constants.values[constant]);
            printf("' jenis=%d\n", kind);
            return offset + 3;
        }
        case OP_GET_PROPERTY:
            return constantInstruction("OP_GET_PROPERTY", chunk, offset);
        case OP_SET_PROPERTY:
            return constantInstruction("OP_SET_PROPERTY", chunk, offset);
        case OP_INVOKE:
            return invokeInstruction("OP_INVOKE", chunk, offset);
        case OP_GET_SUPER:
            return constantInstruction("OP_GET_SUPER", chunk, offset);
        case OP_SUPER_INVOKE:
            return invokeInstruction("OP_SUPER_INVOKE", chunk, offset);
        case OP_TRY_BEGIN:
            return jumpInstruction("OP_TRY_BEGIN", 1, chunk, offset);
        case OP_TRY_END:
            return simpleInstruction("OP_TRY_END", offset);
        case OP_THROW:
            return simpleInstruction("OP_THROW", offset);
        case OP_PASTIKAN:
            return simpleInstruction("OP_PASTIKAN", offset);
        case OP_IS_INSTANCE:
            return simpleInstruction("OP_IS_INSTANCE", offset);
        case OP_CATCH_TEST:
            return constantInstruction("OP_CATCH_TEST", chunk, offset);
        case OP_IMPORT:
            return simpleInstruction("OP_IMPORT", offset);
        case OP_IMPORT_MODULE:
            return constantInstruction("OP_IMPORT_MODULE", chunk, offset);
        case OP_IMPORT_FROM:
            return constantInstruction("OP_IMPORT_FROM", chunk, offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
