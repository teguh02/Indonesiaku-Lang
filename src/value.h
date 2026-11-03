#ifndef indonesiaku_value_h
#define indonesiaku_value_h

#include <stdint.h>
#include <stdbool.h>

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
    VAL_BOOL,
    VAL_KOSONG,  // None/null
    // Internal only: marks a parameter slot the caller did not fill, so the
    // function prologue knows which defaults to evaluate. Never reachable
    // from user code — call() rejects missing required parameters first, and
    // every optional slot is overwritten by its default.
    VAL_TAKTERISI,
    VAL_NUMBER,  // desimal (double)
    VAL_INT,     // bulat (int64, K2)
    VAL_OBJ
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        int64_t integer;
        Obj* obj;
    } as;
} Value;

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_KOSONG(value)  ((value).type == VAL_KOSONG)
#define IS_TAKTERISI(value) ((value).type == VAL_TAKTERISI)
// K2: two number types. IS_NUMBER() is true for both, so every existing
// "is this a number?" check keeps working; use IS_INT() to pick the int path.
// These are inline functions, not macros, because several call sites pass
// pop()/peek() and must not evaluate their argument twice.
static inline bool isNumberValue(Value value) {
    return value.type == VAL_NUMBER || value.type == VAL_INT;
}
static inline bool isIntValue(Value value) {
    return value.type == VAL_INT;
}
static inline double asNumber(Value value) {
    return value.type == VAL_INT ? (double)value.as.integer : value.as.number;
}
static inline int64_t asInt(Value value) {
    return value.type == VAL_INT ? value.as.integer : (int64_t)value.as.number;
}

#define IS_INT(value)     isIntValue(value)
#define IS_FLOAT(value)   ((value).type == VAL_NUMBER)
#define IS_NUMBER(value)  isNumberValue(value)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

#define AS_OBJ(value)     ((value).as.obj)
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  asNumber(value)
#define AS_INT(value)     asInt(value)

#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
#define KOSONG_VAL        ((Value){VAL_KOSONG, {.number = 0}})
#define TAKTERISI_VAL     ((Value){VAL_TAKTERISI, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = (double)(value)}})
#define INT_VAL(value)    ((Value){VAL_INT, {.integer = (int64_t)(value)}})
#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

bool valuesEqual(Value a, Value b);
bool valuesIdentical(Value a, Value b);

// _sama_ for instances (K19), installed by the VM: value.c cannot run
// Indonesiaku code itself. Returns 1/0 for the comparison's truth value, or
// -1 when neither operand defines _sama_ (the caller falls back to identity).
// When the method raises, the error is left pending and 0 is returned.
typedef int (*InstanceEqualFn)(Value a, Value b);
extern InstanceEqualFn instanceEqualHook;

// _hash_ for instances. Returns 1 with *out set, 0 when the method raised or
// returned a non-bulat, -1 when the class does not define _hash_.
typedef int (*InstanceHashFn)(Value value, uint32_t* out);
extern InstanceHashFn instanceHashHook;

// Called when == or hashing nests deeper than `limit` levels. The VM raises
// GalatRekursi and leaves it pending; the operation then returns false.
typedef void (*ValueDepthFn)(const char* what, int limit);
extern ValueDepthFn valueDepthHook;

// --- K2: int64 arithmetic ------------------------------------------------
// Each returns true when the mathematical result does not fit in int64; *out
// is then unspecified. Callers raise GalatLuapan instead of wrapping silently.
bool intAddOverflow(int64_t a, int64_t b, int64_t* out);
bool intSubOverflow(int64_t a, int64_t b, int64_t* out);
bool intMulOverflow(int64_t a, int64_t b, int64_t* out);
// Python floor division / modulo (result takes the divisor's sign). Both
// return false for a zero divisor; intFloorDiv also refuses INT64_MIN / -1.
bool intFloorDiv(int64_t a, int64_t b, int64_t* out);
bool intModulo(int64_t a, int64_t b, int64_t* out);
// a << n without undefined behaviour; false when the result leaves int64.
// n must be >= 0 (callers reject negative counts first).
bool intShiftLeft(int64_t a, int64_t n, int64_t* out);
// Python's a >> n for n >= 0: floors toward -inf, and n >= 64 gives 0 or -1.
int64_t intShiftRight(int64_t a, int64_t n);
// Exponentiation by squaring. `exp` must be >= 0; *overflow is set instead of
// wrapping. Negative exponents are handled by the caller as doubles.
int64_t intPow(int64_t base, int64_t exp, bool* overflow);
// Numeric comparison that does not lose int64 precision when both sides are
// bulat. Returns false when either operand is not a number.
bool numberCompare(Value a, Value b, int* result);

// K8: hash for dictionary keys. Returns false for unhashable values (list,
// kamus, rentang). Numerically equal keys hash equally: 1 and 1.0 land in the
// same bucket because they are the same Python dict key.
bool hashValue(Value value, uint32_t* out);
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);
void numberToString(double value, char* buf, size_t size);
void rangeNumberToString(double value, char* buf, size_t size);

#endif
