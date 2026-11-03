#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "memory.h"
#include "value.h"
#include "object.h"

void initValueArray(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values,
                                   oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

// Debug/GC logs and the disassembler. User-facing printing (cetak, ke_teks,
// ...) goes through formatValue with magic methods enabled instead.
void printValue(Value value) {
    TextBuf buf;
    initTextBuf(&buf);
    formatValue(&buf, value, false, false);
    if (buf.length > 0) fwrite(buf.chars, 1, (size_t)buf.length, stdout);
    freeTextBuf(&buf);
}

// Format a double the way the language prints a desimal (K2): the SHORTEST
// representation that reads back as the same double (Python's repr), and
// always visibly a desimal — 3.0 prints "3.0", never "3". 0.1 + 0.2 prints
// 0.30000000000000004, and 2 ** 0.5 prints 1.4142135623730951.
void numberToString(double value, char* buf, size_t size) {
    if (isnan(value)) { snprintf(buf, size, "nan"); return; }
    if (isinf(value)) { snprintf(buf, size, value < 0 ? "-takhingga" : "takhingga"); return; }

    // Whole values below 1e16 (where Python's repr switches to exponent form)
    // print as digits plus ".0": 3.0 -> "3.0", 1200.0 -> "1200.0". %.g alone
    // would choose "1.2e+03" here, which Python never does.
    if (value == floor(value) && fabs(value) < 1e16) {
        snprintf(buf, size, "%.0f.0", value + 0.0);
        return;
    }

    // 17 significant digits always round-trip; try fewer first.
    for (int precision = 1; precision <= 17; precision++) {
        snprintf(buf, size, "%.*g", precision, value);
        if (strtod(buf, NULL) == value) break;
    }

    // %.g drops the fractional part for whole values ("3"), so add it back.
    if (strpbrk(buf, ".eE") == NULL) {
        size_t len = strlen(buf);
        if (len + 2 < size) {
            buf[len] = '.';
            buf[len + 1] = '0';
            buf[len + 2] = '\0';
        }
    }
}

// --- K8: hashing for dictionary keys --------------------------------------

static uint32_t hashBytes(const void* data, size_t length, uint32_t hash) {
    const unsigned char* bytes = (const unsigned char*)data;
    for (size_t i = 0; i < length; i++) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

ValueDepthFn valueDepthHook = NULL;

// Nesting beyond this raises GalatRekursi (Python's RecursionError) instead
// of overflowing the C stack (hash of a 20000-deep tupel crashed, T-U14) or
// silently answering salah (== at 64 levels, T-U13). Each level is one small
// C frame, so 1000 -- Python's default recursion limit -- is ~100 KB.
#define VALUE_MAX_DEPTH 1000

static bool depthExceeded(int depth, const char* what) {
    if (depth < VALUE_MAX_DEPTH) return false;
    if (valueDepthHook != NULL) valueDepthHook(what, VALUE_MAX_DEPTH);
    return true;
}

static bool hashValueDepth(Value value, uint32_t* out, int depth);

bool hashValue(Value value, uint32_t* out) {
    return hashValueDepth(value, out, 0);
}

static bool hashValueDepth(Value value, uint32_t* out, int depth) {
    const uint32_t seed = 2166136261u;   // FNV-1a offset basis
    switch (value.type) {
        case VAL_BOOL: {
            int64_t n = AS_BOOL(value) ? 1 : 0;
            *out = hashBytes(&n, sizeof n, seed);
            return true;
        }
        case VAL_KOSONG: {
            uint64_t n = 0x4b4f534f4e47ULL;   // "KOSONG"
            *out = hashBytes(&n, sizeof n, seed);
            return true;
        }
        case VAL_INT: {
            int64_t n = AS_INT(value);
            *out = hashBytes(&n, sizeof n, seed);
            return true;
        }
        case VAL_NUMBER: {
            double d = AS_NUMBER(value) + 0.0;   // -0.0 hashes like 0.0
            // A whole desimal is the same dict key as its bulat twin (1 == 1.0).
            if (d == floor(d) && d >= -9223372036854775808.0 && d < 9223372036854775808.0) {
                int64_t n = (int64_t)d;
                *out = hashBytes(&n, sizeof n, seed);
            } else {
                *out = hashBytes(&d, sizeof d, seed);
            }
            return true;
        }
        case VAL_OBJ:
            if (IS_STRING(value)) {
                *out = AS_STRING(value)->hash;
                return true;
            }
            if (AS_OBJ(value)->type == OBJ_SET) {
                // himpunan_beku is hashable, plain himpunan is not (Python).
                ObjSet* set = AS_SET(value);
                if (!set->frozen) return false;
                if (depthExceeded(depth, "hash")) return false;
                // XOR so the hash does not depend on insertion order: two equal
                // himpunan_beku built in different orders must hash the same.
                uint32_t h = seed ^ (uint32_t)set->keys.count;
                for (int i = 0; i < set->keys.count; i++) {
                    uint32_t member;
                    if (!hashValueDepth(set->keys.values[i], &member, depth + 1)) return false;
                    h ^= member;
                }
                *out = h;
                return true;
            }
            if (AS_OBJ(value)->type == OBJ_LIST || AS_OBJ(value)->type == OBJ_DICT ||
                AS_OBJ(value)->type == OBJ_RANGE) {
                return false;   // unhashable, as in Python
            }
            if (AS_OBJ(value)->type == OBJ_TUPLE) {
                // Structural, so equal tupel are the same dict key: combine the
                // element hashes. Hashable only if every element is (Python:
                // hash(([1],)) is a TypeError). A tupel cannot contain itself
                // (it is immutable), so this recursion always terminates --
                // but it can be arbitrarily deep, hence the depth limit.
                if (depthExceeded(depth, "hash")) return false;
                ValueArray* items = &AS_TUPLE(value)->items;
                uint32_t h = seed ^ (uint32_t)items->count;
                for (int i = 0; i < items->count; i++) {
                    uint32_t element;
                    if (!hashValueDepth(items->values[i], &element, depth + 1)) return false;
                    h = (h ^ element) * 16777619u;
                }
                *out = h;
                return true;
            }
            if (AS_OBJ(value)->type == OBJ_INSTANCE &&
                (AS_INSTANCE(value)->klass->magic & MAGIC_BIT(MAGIC_HASH)) &&
                instanceHashHook != NULL) {
                // A class that defines _hash_ decides its own hash, whether or
                // not it also defines _sama_.
                int handled = instanceHashHook(value, out);
                if (handled >= 0) return handled == 1;
                // -1: hook says no _hash_ after all; fall through.
            }
            if (AS_OBJ(value)->type == OBJ_INSTANCE &&
                (AS_INSTANCE(value)->klass->magic & MAGIC_BIT(MAGIC_SAMA))) {
                // K19: defining _sama_ without a _hash_ makes instances
                // unhashable, like Python setting __hash__ = None. Define
                // _hash_ too (handled above) to get the key back.
                return false;
            }
            {
                // Instances and other objects hash by identity.
                uintptr_t p = (uintptr_t)AS_OBJ(value);
                *out = hashBytes(&p, sizeof p, seed);
                return true;
            }
        case VAL_TAKTERISI:
            return false;
    }
    return false;
}

// --- K2: int64 arithmetic -------------------------------------------------

#if defined(__GNUC__) || defined(__clang__)
#define INT_OVERFLOW_BUILTINS 1
#else
#define INT_OVERFLOW_BUILTINS 0
#endif

bool intAddOverflow(int64_t a, int64_t b, int64_t* out) {
#if INT_OVERFLOW_BUILTINS
    return __builtin_add_overflow(a, b, out);
#else
    if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) return true;
    *out = a + b;
    return false;
#endif
}

bool intSubOverflow(int64_t a, int64_t b, int64_t* out) {
#if INT_OVERFLOW_BUILTINS
    return __builtin_sub_overflow(a, b, out);
#else
    if ((b < 0 && a > INT64_MAX + b) || (b > 0 && a < INT64_MIN + b)) return true;
    *out = a - b;
    return false;
#endif
}

bool intMulOverflow(int64_t a, int64_t b, int64_t* out) {
#if INT_OVERFLOW_BUILTINS
    return __builtin_mul_overflow(a, b, out);
#else
    if (a == 0 || b == 0) { *out = 0; return false; }
    if ((a == -1 && b == INT64_MIN) || (b == -1 && a == INT64_MIN)) return true;
    long double r = (long double)a * (long double)b;
    if (r > (long double)INT64_MAX || r < (long double)INT64_MIN) return true;
    *out = (int64_t)r;
    return false;
#endif
}

// Floor division, Python style. Returns false for a zero divisor and for
// INT64_MIN / -1 (which does not fit in int64); callers check the divisor
// first, so false means overflow here.
bool intFloorDiv(int64_t a, int64_t b, int64_t* out) {
    if (b == 0) return false;
    if (a == INT64_MIN && b == -1) return false;
    int64_t q = a / b;
    int64_t r = a % b;
    if (r != 0 && ((r < 0) != (b < 0))) q--;
    *out = q;
    return true;
}

// Modulo whose result takes the divisor's sign, like Python (-7 % 3 == 2).
bool intModulo(int64_t a, int64_t b, int64_t* out) {
    if (b == 0) return false;
    if (a == INT64_MIN && b == -1) { *out = 0; return true; }
    int64_t r = a % b;
    if (r != 0 && ((r < 0) != (b < 0))) r += b;
    *out = r;
    return true;
}

bool intShiftLeft(int64_t a, int64_t n, int64_t* out) {
    if (a == 0) { *out = 0; return true; }
    if (n >= 64) return false;  // any non-zero a overflows; shifting by 64 is UB
    // a fits after shifting iff it lies within [INT64_MIN >> n, INT64_MAX >> n]
    // (for n == 63 that admits only -1: -1 << 63 == INT64_MIN).
    // (INT64_MIN >> n is written as -(INT64_MAX >> n) - 1: same value, but no
    // implementation-defined shift of a negative number.)
    if (a > (INT64_MAX >> n) || a < -(INT64_MAX >> n) - 1) return false;
    *out = (int64_t)((uint64_t)a << n);  // unsigned shift: no UB on negatives
    return true;
}

int64_t intShiftRight(int64_t a, int64_t n) {
    if (n >= 63) return a < 0 ? -1 : 0;
    // Floor division by 2^n, written without relying on implementation-
    // defined right shifts of negative values.
    if (a >= 0) return a >> n;
    return -(int64_t)(((uint64_t)(-(a + 1)) >> n)) - 1;
}

int64_t intPow(int64_t base, int64_t exp, bool* overflow) {
    int64_t result = 1;
    int64_t factor = base;
    *overflow = false;
    while (exp > 0) {
        if (exp & 1) {
            if (intMulOverflow(result, factor, &result)) { *overflow = true; return 0; }
        }
        exp >>= 1;
        if (exp > 0 && intMulOverflow(factor, factor, &factor)) { *overflow = true; return 0; }
    }
    return result;
}

// Compare an int64 with a double without losing precision. *out is -1, 0, 1,
// or 2 when the double is NaN (all comparisons with NaN are false).
static void compareIntDouble(int64_t i, double d, int* out) {
    if (isnan(d)) { *out = 2; return; }
    if (d >= 9223372036854775808.0) { *out = -1; return; }   // d > INT64_MAX
    if (d < -9223372036854775808.0) { *out = 1; return; }    // d < INT64_MIN
    double whole = floor(d);
    int64_t t = (int64_t)d;
    if (d != whole) {
        *out = (i <= t) ? -1 : 1;   // t is the integer part, so i <= t means i < d
        return;
    }
    *out = i < t ? -1 : i > t ? 1 : 0;
}

bool numberCompare(Value a, Value b, int* result) {
    if (!IS_NUMBER(a) || !IS_NUMBER(b)) return false;
    if (IS_INT(a) && IS_INT(b)) {
        int64_t x = AS_INT(a), y = AS_INT(b);
        *result = x < y ? -1 : x > y ? 1 : 0;
        return true;
    }
    if (IS_INT(a)) { compareIntDouble(AS_INT(a), b.as.number, result); return true; }
    if (IS_INT(b)) { compareIntDouble(AS_INT(b), a.as.number, result); *result = *result == 2 ? 2 : -*result; return true; }
    double x = asNumber(a), y = asNumber(b);
    if (isnan(x) || isnan(y)) { *result = 2; return true; }
    *result = x < y ? -1 : x > y ? 1 : 0;
    return true;
}

// Range bounds are stored as doubles, but a whole bound prints like a bulat:
// rentang(0, 3), not rentang(0.0, 3.0) — the arguments may well have been bulat.
void rangeNumberToString(double value, char* buf, size_t size) {
    if (value == floor(value) && fabs(value) < 9.2e18) {
        snprintf(buf, size, "%.0f", value);
        return;
    }
    numberToString(value, buf, size);
}

// Identity comparison: the semantics of `adalah` / `bukan adalah`.
// Objects are the same only when they are the same heap object; primitives
// compare by value. Strings are interned, so "a" adalah "a" is benar.
bool valuesIdentical(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
        case VAL_KOSONG: return true;
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_INT:    return AS_INT(a) == AS_INT(b);
        case VAL_OBJ:    return AS_OBJ(a) == AS_OBJ(b);
        default:         return false; // Unreachable.
    }
}

// K7: '==' compares lists, dicts and ranges by STRUCTURE, recursively.
// Everything else (instances, functions, files, sockets) stays identity-based.
// Self-referential containers would recurse forever, so past VALUE_MAX_DEPTH
// the comparison raises GalatRekursi, as Python does. It used to stop at 64
// levels and answer salah, a silently wrong result for deep but finite data.

InstanceEqualFn instanceEqualHook = NULL;
InstanceHashFn instanceHashHook = NULL;

static bool valuesEqualDepth(Value a, Value b, int depth) {
    // An instance with _sama_ decides for itself (K19), after the identity
    // shortcut that Python's PyObject_RichCompareBool also takes: x dalam [x]
    // is benar without calling _sama_. The top-level `==` operator does not
    // come through here for instances; it calls _sama_ even on itself.
    if ((IS_INSTANCE(a) || IS_INSTANCE(b)) && instanceEqualHook != NULL) {
        if (IS_OBJ(a) && IS_OBJ(b) && AS_OBJ(a) == AS_OBJ(b)) return true;
        int handled = instanceEqualHook(a, b);
        if (handled >= 0) return handled == 1;
    }
    // K2: 1 == 1.0 must be true even though the types differ (Python).
    if (IS_NUMBER(a) && IS_NUMBER(b)) {
        int cmp;
        return numberCompare(a, b, &cmp) && cmp == 0;
    }
    if (a.type != b.type) return false;
    if (!IS_OBJ(a)) return valuesIdentical(a, b);
    if (AS_OBJ(a) == AS_OBJ(b)) return true;      // same object: always equal
    if (OBJ_TYPE(a) != OBJ_TYPE(b)) return false;
    if (depthExceeded(depth, "perbandingan")) return false;

    switch (OBJ_TYPE(a)) {
        case OBJ_LIST:
        case OBJ_TUPLE: {
            // Same object type is guaranteed above: [1] == (1,) is salah, as in Python.
            ValueArray* x = sequenceItems(a);
            ValueArray* y = sequenceItems(b);
            if (x->count != y->count) return false;
            // Bounds re-checked every step: a _sama_ called on an element may
            // shrink either list (only lists can change; tupel are fixed).
            for (int i = 0; i < x->count && i < y->count; i++) {
                if (!valuesEqualDepth(x->values[i], y->values[i], depth + 1)) return false;
            }
            return x->count == y->count;
        }
        case OBJ_DICT: {
            ObjDict* x = AS_DICT(a);
            ObjDict* y = AS_DICT(b);
            if (dictLength(x) != dictLength(y)) return false;
            // Insertion order does not affect equality: look every live key up.
            for (int i = 0; i < x->keys.count; i++) {
                Value key = x->keys.values[i];
                Value xv, yv;
                if (!valueTableGet(&x->table, key, &xv)) continue;
                if (!valueTableGet(&y->table, key, &yv)) return false;
                if (!valuesEqualDepth(xv, yv, depth + 1)) return false;
            }
            return true;
        }
        case OBJ_SET: {
            // Same members, order irrelevant. Members are hashable, so a
            // table lookup in y decides each one. `frozen` is deliberately
            // NOT compared: himpunan([1]) == himpunan_beku([1]) is benar,
            // exactly as set([1]) == frozenset([1]) in Python.
            ObjSet* x = AS_SET(a);
            ObjSet* y = AS_SET(b);
            if (x->keys.count != y->keys.count) return false;
            for (int i = 0; i < x->keys.count; i++) {
                Value ignored;
                if (!valueTableGet(&y->table, x->keys.values[i], &ignored)) return false;
            }
            return true;
        }
        case OBJ_RANGE: {
            ObjRange* x = AS_RANGE(a);
            ObjRange* y = AS_RANGE(b);
            return x->start == y->start && x->stop == y->stop && x->step == y->step;
        }
        default:
            return false;   // strings are interned; other objects use identity
    }
}

bool valuesEqual(Value a, Value b) {
    return valuesEqualDepth(a, b, 0);
}
