#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include "table.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;
    object->isMarked = false;
    object->next = vm.objects;
    vm.objects = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu untuk tipe %d\n", (void*)object, size, type);
#endif

    return object;
}

ObjFunction* newFunction() {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->requiredArity = 0;
    function->paramNames = NULL;
    function->upvalueCount = 0;
    function->name = NULL;
    function->module = NULL;   // set by the compiler; GC-marked
    initChunk(&function->chunk);
    return function;
}

ObjClosure* newClosure(ObjFunction* function) {
    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

ObjUpvalue* newUpvalue(Value* slot) {
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = KOSONG_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;
    return upvalue;
}

ObjList* newList() {
    ObjList* list = ALLOCATE_OBJ(ObjList, OBJ_LIST);
    initValueArray(&list->items);
    return list;
}

// An empty tupel; callers fill items while it is rooted on the VM stack and
// never touch it afterwards (tupel is immutable).
ObjTuple* newTuple() {
    ObjTuple* tuple = ALLOCATE_OBJ(ObjTuple, OBJ_TUPLE);
    initValueArray(&tuple->items);
    return tuple;
}

// Empty himpunan. Callers root it on the VM stack before adding members:
// dictSet may grow the table and trigger a GC.
ObjSet* newSet() {
    ObjSet* set = ALLOCATE_OBJ(ObjSet, OBJ_SET);
    initValueTable(&set->table);
    initValueArray(&set->keys);
    set->frozen = false;
    return set;
}

ObjModule* newModule(ObjString* name, ObjString* path) {
    ObjModule* module = ALLOCATE_OBJ(ObjModule, OBJ_MODULE);
    // NULL first: initTable allocates nothing, but a later GC walking this
    // object must never see uninitialised pointers.
    module->name = NULL;
    module->path = NULL;
    initTable(&module->globals);
    module->name = name;
    module->path = path;
    return module;
}

ObjIterator* newIterator(Value source) {
    ObjIterator* it = ALLOCATE_OBJ(ObjIterator, OBJ_ITERATOR);
    it->source = source;
    it->index = 0;
    return it;
}

ObjGenerator* newGenerator(ObjClosure* closure) {
    ObjGenerator* gen = ALLOCATE_OBJ(ObjGenerator, OBJ_GENERATOR);
    gen->closure = closure;
    gen->ip = NULL;
    gen->slotCount = 0;
    gen->state = GEN_BELUM_MULAI;
    // Every slot must be a valid Value before the next GC can see this object.
    for (int i = 0; i < GEN_SLOTS_MAX; i++) gen->slots[i] = KOSONG_VAL;
    return gen;
}

bool setAdd(ObjSet* set, Value value) {
    uint32_t ignored;
    if (!hashValue(value, &ignored)) return false;   // same rule as dict keys (K8)
    dictSet(setAsDict(set), value, BOOL_VAL(true));
    return true;
}

ObjDict* newDict() {
    ObjDict* dict = ALLOCATE_OBJ(ObjDict, OBJ_DICT);
    initValueTable(&dict->table);
    initValueArray(&dict->keys);
    return dict;
}

bool dictSet(ObjDict* dict, Value key, Value value) {
    bool isNew = valueTableSet(&dict->table, key, value);
    if (isNew) writeValueArray(&dict->keys, key);
    return isNew;
}

bool dictDelete(ObjDict* dict, Value key) {
    if (!valueTableDelete(&dict->table, key)) return false;
    // Remove the key from the insertion-order list, keeping it contiguous so
    // iteration and dictKeyAt never have to skip tombstones.
    for (int i = 0; i < dict->keys.count; i++) {
        if (valuesEqual(dict->keys.values[i], key)) {
            for (int j = i; j < dict->keys.count - 1; j++) {
                dict->keys.values[j] = dict->keys.values[j + 1];
            }
            dict->keys.count--;
            break;
        }
    }
    return true;
}

Value dictKeyAt(ObjDict* dict, int index) {
    if (index < 0 || index >= dict->keys.count) return KOSONG_VAL;
    return dict->keys.values[index];
}

int dictLength(ObjDict* dict) {
    return dict->keys.count;
}

ObjRange* newRange(double start, double stop, double step) {
    ObjRange* range = ALLOCATE_OBJ(ObjRange, OBJ_RANGE);
    range->start = start;
    range->stop = stop;
    range->step = step;
    return range;
}

// Python's len(range): ceil((stop - start) / step), never negative.
double rangeLength(ObjRange* range) {
    double count = ceil((range->stop - range->start) / range->step);
    return count > 0 ? count : 0;
}

// K2: a rentang of bulat yields bulat elements, so `untuk i dalam rentang(3)`
// gives 0, 1, 2 as bulat (like Python's range).
Value rangeElement(ObjRange* range, int index) {
    double value = range->start + (double)index * range->step;
    if (value == floor(value) &&
        value >= -9223372036854775808.0 && value < 9223372036854775808.0) {
        return INT_VAL((int64_t)value);
    }
    return NUMBER_VAL(value);
}

bool rangeContains(ObjRange* range, double value) {
    if (value != floor(value)) return false;
    bool inBounds = range->step > 0
        ? (value >= range->start && value < range->stop)
        : (value <= range->start && value > range->stop);
    return inBounds && fmod(value - range->start, range->step) == 0;
}

ObjClass* newClass(ObjString* name) {
    ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
    klass->superclass = NULL;
    initTable(&klass->methods);
    initTable(&klass->fields);
    initTable(&klass->special);
    initTable(&klass->own);
    klass->mro = NULL;
    klass->mroCount = 0;
    klass->magic = 0;
    return klass;
}

ObjInstance* newInstance(ObjClass* klass) {
    ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    initTable(&instance->fields);
    return instance;
}

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method) {
    ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

ObjFile* newFile(void* handle, ObjString* path) {
    ObjFile* file = ALLOCATE_OBJ(ObjFile, OBJ_FILE);
    file->handle = handle;
    file->path = path;
    file->isOpen = true;
    return file;
}

ObjDb* newDb(void) {
    ObjDb* db = ALLOCATE_OBJ(ObjDb, OBJ_DB);
    db->db = NULL;
    return db;
}

ObjSocket* newSocket(long long fd) {
    ObjSocket* sock = ALLOCATE_OBJ(ObjSocket, OBJ_SOCKET);
    sock->fd = fd;
    sock->isOpen = true;
    sock->tls = NULL;   // polos sampai tls_mulai() dipanggil
    return sock;
}

ObjNative* newNative(NativeFn function) {
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

// A UTF-8 continuation byte has the form 10xxxxxx; every other byte starts a
// character. Invalid sequences still count one character per lead byte.
static bool isContinuationByte(char c) {
    return ((unsigned char)c & 0xC0) == 0x80;
}

static int countChars(const char* chars, int length) {
    int count = 0;
    for (int i = 0; i < length; i++) {
        if (!isContinuationByte(chars[i])) count++;
    }
    return count;
}

int utf8ByteOffset(ObjString* string, int charIndex) {
    if (string->charCount == string->length) return charIndex;  // ASCII: O(1)
    int seen = 0;
    for (int i = 0; i < string->length; i++) {
        if (!isContinuationByte(string->chars[i])) {
            if (seen == charIndex) return i;
            seen++;
        }
    }
    return string->length;
}

int utf8CharBytes(ObjString* string, int byteOffset) {
    int end = byteOffset + 1;
    while (end < string->length && isContinuationByte(string->chars[end])) end++;
    return end - byteOffset;
}

static ObjString* allocateString(char* chars, int length, uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->charCount = countChars(chars, length);
    string->chars = chars;
    string->hash = hash;
    // Push onto the VM stack so the string is reachable if tableSet below
    // triggers a garbage collection (interning it into vm.strings).
    push(OBJ_VAL(string));
    tableSet(&vm.strings, string, KOSONG_VAL);
    pop();
    return string;
}

static uint32_t hashString(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString* takeString(char* chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    return allocateString(chars, length, hash);
}

ObjString* copyString(const char* chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) return interned;

    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}

// Escape sequence for a character inside a quoted string, or NULL when the
// character can be emitted as-is. formatValue is the only printer, so this
// is the single source of truth for how a quoted string looks.
const char* stringEscape(char c) {
    switch (c) {
        case '"':  return "\\\"";
        case '\\': return "\\\\";
        case '\n': return "\\n";
        case '\t': return "\\t";
        case '\r': return "\\r";
        default:   return NULL;
    }
}

// ---------------------------------------------------------------------------
// Text buffer
// ---------------------------------------------------------------------------

void initTextBuf(TextBuf* buf) {
    buf->chars = NULL;
    buf->length = 0;
    buf->capacity = 0;
}

void freeTextBuf(TextBuf* buf) {
    free(buf->chars);
    initTextBuf(buf);
}

void textAppend(TextBuf* buf, const char* chars, int length) {
    if (length <= 0) return;
    if (buf->length + length + 1 > buf->capacity) {
        int capacity = buf->capacity < 64 ? 64 : buf->capacity;
        while (capacity < buf->length + length + 1) capacity *= 2;
        char* grown = realloc(buf->chars, (size_t)capacity);
        if (grown == NULL) {
            fprintf(stderr, "Memori habis saat menyusun teks.\n");
            exit(1);
        }
        buf->chars = grown;
        buf->capacity = capacity;
    }
    memcpy(buf->chars + buf->length, chars, (size_t)length);
    buf->length += length;
    buf->chars[buf->length] = '\0';
}

void textAppendCString(TextBuf* buf, const char* chars) {
    textAppend(buf, chars, (int)strlen(chars));
}

// ---------------------------------------------------------------------------
// The formatter
// ---------------------------------------------------------------------------

FormatInstanceFn formatInstanceHook = NULL;

// Containers currently being formatted, so a list that contains itself
// prints [1, [...]] like Python (its Py_ReprEnter is global per thread too)
// instead of recursing until the C stack runs out. The same cap also stops
// absurdly deep (non-cyclic) nesting.
#define FORMAT_MAX_DEPTH 256
static Obj* formatting[FORMAT_MAX_DEPTH];
static int formattingCount = 0;

// When magic methods may run, a _repr_ can drop the container being printed
// from its parent and trigger a GC: keep it on the VM stack meanwhile. Without
// callMagic nothing here touches the GC heap, so nothing needs rooting (and
// debug printing may run before the VM stack exists).
static bool formatEnter(Value container, bool callMagic) {
    Obj* object = AS_OBJ(container);
    if (formattingCount >= FORMAT_MAX_DEPTH) return false;
    for (int i = 0; i < formattingCount; i++) {
        if (formatting[i] == object) return false;
    }
    formatting[formattingCount++] = object;
    if (callMagic) push(container);
    return true;
}

static void formatLeave(bool callMagic) {
    if (callMagic) pop();
    formattingCount--;
}

static void formatStringRepr(TextBuf* out, ObjString* str) {
    textAppend(out, "\"", 1);
    int start = 0;
    for (int i = 0; i < str->length; i++) {
        const char* esc = stringEscape(str->chars[i]);
        if (esc == NULL) continue;
        textAppend(out, str->chars + start, i - start);
        textAppendCString(out, esc);
        start = i + 1;
    }
    textAppend(out, str->chars + start, str->length - start);
    textAppend(out, "\"", 1);
}

static void formatFunction(TextBuf* out, ObjFunction* function) {
    if (function->name == NULL) {
        textAppendCString(out, "<skrip>");
        return;
    }
    textAppendCString(out, "<fungsi ");
    textAppend(out, function->name->chars, function->name->length);
    textAppendCString(out, ">");
}

// Elements of a daftar, tupel or himpunan: always repr (["a"], not [a]).
// Reads count and values afresh every step: a _repr_ running in between may
// grow or shrink the very array being printed.
static bool formatItems(TextBuf* out, ValueArray* items, bool callMagic) {
    for (int i = 0; i < items->count; i++) {
        if (i > 0) textAppend(out, ", ", 2);
        if (!formatValue(out, items->values[i], true, callMagic)) return false;
    }
    return true;
}

static bool formatDict(TextBuf* out, ObjDict* dict, bool callMagic) {
    // Insertion order (like Python, kunci() and iteration), any hashable key
    // type (K8); keys and values as repr: {"a": 1, 2: "dua"}.
    textAppend(out, "{", 1);
    bool first = true;
    for (int i = 0; i < dict->keys.count; i++) {
        Value key = dict->keys.values[i];
        Value entryValue;
        if (!valueTableGet(&dict->table, key, &entryValue)) continue;
        if (!first) textAppend(out, ", ", 2);
        first = false;
        // The key's _repr_ may delete this very entry (and collect): hold
        // the pair on the stack until both are printed.
        if (callMagic) {
            push(key);
            push(entryValue);
        }
        bool ok = formatValue(out, key, true, callMagic);
        if (ok) {
            textAppend(out, ": ", 2);
            ok = formatValue(out, entryValue, true, callMagic);
        }
        if (callMagic) vm.stackTop -= 2;
        if (!ok) return false;
    }
    textAppend(out, "}", 1);
    return true;
}

static void formatRange(TextBuf* out, ObjRange* range) {
    // Like Python's range(0, 5) / range(0, 10, 2).
    char number[32];
    textAppendCString(out, "rentang(");
    rangeNumberToString(range->start, number, sizeof(number));
    textAppendCString(out, number);
    textAppend(out, ", ", 2);
    rangeNumberToString(range->stop, number, sizeof(number));
    textAppendCString(out, number);
    if (range->step != 1) {
        textAppend(out, ", ", 2);
        rangeNumberToString(range->step, number, sizeof(number));
        textAppendCString(out, number);
    }
    textAppend(out, ")", 1);
}

static bool formatInstance(TextBuf* out, Value value, bool callMagic) {
    ObjInstance* instance = AS_INSTANCE(value);
    // Built-in Galat* instances print their message.
    if (strncmp(instance->klass->name->chars, "Galat", 5) == 0) {
        for (int i = 0; i < instance->fields.capacity; i++) {
            Entry* entry = &instance->fields.entries[i];
            if (entry->key != NULL && strcmp(entry->key->chars, "pesan") == 0) {
                return formatValue(out, entry->value, false, callMagic);
            }
        }
    }
    textAppendCString(out, "<objek ");
    textAppend(out, instance->klass->name->chars, instance->klass->name->length);
    textAppend(out, ">", 1);
    return true;
}

static bool formatObject(TextBuf* out, Value value, bool repr, bool callMagic) {
    char tmp[64];
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            if (repr) {
                formatStringRepr(out, AS_STRING(value));
            } else {
                textAppend(out, AS_CSTRING(value), AS_STRING(value)->length);
            }
            return true;
        case OBJ_FUNCTION:
            formatFunction(out, AS_FUNCTION(value));
            return true;
        case OBJ_CLOSURE:
            formatFunction(out, AS_CLOSURE(value)->function);
            return true;
        case OBJ_BOUND_METHOD:
            formatFunction(out, AS_BOUND_METHOD(value)->method->function);
            return true;
        case OBJ_UPVALUE:
            textAppendCString(out, "upvalue");
            return true;
        case OBJ_NATIVE:
            textAppendCString(out, "<fungsi bawaan>");
            return true;
        case OBJ_CLASS:
            textAppendCString(out, "<kelas ");
            textAppend(out, AS_CLASS(value)->name->chars, AS_CLASS(value)->name->length);
            textAppend(out, ">", 1);
            return true;
        case OBJ_LIST: {
            if (!formatEnter(value, callMagic)) {
                textAppendCString(out, "[...]");
                return true;
            }
            textAppend(out, "[", 1);
            bool ok = formatItems(out, &AS_LIST(value)->items, callMagic);
            textAppend(out, "]", 1);
            formatLeave(callMagic);
            return ok;
        }
        case OBJ_TUPLE: {
            // (1, 2), (1,) -- the comma marks a 1-tuple -- and ().
            if (!formatEnter(value, callMagic)) {
                textAppendCString(out, "(...)");
                return true;
            }
            ValueArray* items = &AS_TUPLE(value)->items;
            textAppend(out, "(", 1);
            bool ok = formatItems(out, items, callMagic);
            if (items->count == 1) textAppend(out, ",", 1);
            textAppend(out, ")", 1);
            formatLeave(callMagic);
            return ok;
        }
        case OBJ_SET: {
            // No literal exists (K17): print the call that rebuilds it.
            ObjSet* set = AS_SET(value);
            const char* nama = set->frozen ? "himpunan_beku" : "himpunan";
            textAppendCString(out, nama);
            if (set->keys.count == 0) {
                textAppendCString(out, "()");
                return true;
            }
            if (!formatEnter(value, callMagic)) {
                textAppendCString(out, "(...)");
                return true;
            }
            textAppendCString(out, "([");
            bool ok = formatItems(out, &set->keys, callMagic);
            textAppendCString(out, "])");
            formatLeave(callMagic);
            return ok;
        }
        case OBJ_DICT: {
            if (!formatEnter(value, callMagic)) {
                textAppendCString(out, "{...}");
                return true;
            }
            bool ok = formatDict(out, AS_DICT(value), callMagic);
            formatLeave(callMagic);
            return ok;
        }
        case OBJ_RANGE:
            formatRange(out, AS_RANGE(value));
            return true;
        case OBJ_MODULE:
            textAppendCString(out, "<modul ");
            textAppend(out, AS_MODULE(value)->name->chars, AS_MODULE(value)->name->length);
            textAppend(out, ">", 1);
            return true;
        case OBJ_ITERATOR:
            textAppendCString(out, "<iterator>");   // jenis() calls it "iterator"
            return true;
        case OBJ_GENERATOR:
            textAppendCString(out, "<generator>");
            return true;
        case OBJ_INSTANCE:
            if (callMagic && formatInstanceHook != NULL) {
                int handled = formatInstanceHook(out, value, repr);
                if (handled < 0) return false;
                if (handled > 0) return true;
            }
            return formatInstance(out, value, callMagic);
        case OBJ_FILE: {
            ObjFile* file = AS_FILE(value);
            textAppendCString(out, "<berkas ");
            textAppend(out, file->path->chars, file->path->length);
            textAppendCString(out, file->isOpen ? ">" : " (tertutup)>");
            return true;
        }
        case OBJ_DB:
            textAppendCString(out, AS_DB(value)->db ? "<db>" : "<db (tertutup)>");
            return true;
        case OBJ_SOCKET:
            textAppendCString(out, AS_SOCKET(value)->isOpen ? "<soket>" : "<soket (tertutup)>");
            return true;
    }
    snprintf(tmp, sizeof(tmp), "<objek>");
    textAppendCString(out, tmp);
    return true;
}

bool formatValue(TextBuf* out, Value value, bool repr, bool callMagic) {
    char number[64];
    switch (value.type) {
        case VAL_BOOL:
            textAppendCString(out, AS_BOOL(value) ? "benar" : "salah");
            return true;
        case VAL_KOSONG:
            textAppendCString(out, "kosong");
            return true;
        // Should be unreachable: the marker never escapes a call prologue.
        // Printing something recognisable beats printing garbage if it does.
        case VAL_TAKTERISI:
            textAppendCString(out, "<takterisi>");
            return true;
        case VAL_INT:
            snprintf(number, sizeof(number), "%lld", (long long)AS_INT(value));
            textAppendCString(out, number);
            return true;
        case VAL_NUMBER:
            numberToString(AS_NUMBER(value), number, sizeof(number));
            textAppendCString(out, number);
            return true;
        case VAL_OBJ:
            return formatObject(out, value, repr, callMagic);
    }
    return true;
}

// Debug/GC logs and the disassembler: never run user code.
void printObject(Value value) {
    TextBuf buf;
    initTextBuf(&buf);
    formatValue(&buf, value, false, false);
    if (buf.length > 0) fwrite(buf.chars, 1, (size_t)buf.length, stdout);
    freeTextBuf(&buf);
}
