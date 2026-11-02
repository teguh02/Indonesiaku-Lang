#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "tls.h"
#include "native.h"
#include "object.h"
#include "table.h"
#include "vm.h"
#include "sqlite3.h"

#ifdef DEBUG_LOG_GC
#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    vm.bytesAllocated += newSize - oldSize;

    if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        collectGarbage();
#endif
        if (vm.bytesAllocated > vm.nextGC) {
            collectGarbage();
        }
    }

    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) {
        fprintf(stderr, "Kesalahan fatal: memori habis (gagal alokasi %zu byte).\n", newSize);
        exit(1);
    }
    return result;
}

void markObject(Obj* object) {
    if (object == NULL) return;
    if (object->isMarked) return;

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif

    object->isMarked = true;

    if (vm.grayCapacity < vm.grayCount + 1) {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        // The gray stack is managed with the system allocator on purpose:
        // it must not itself trigger a GC while a collection is running.
        vm.grayStack = (Obj**)realloc(vm.grayStack,
                                      sizeof(Obj*) * vm.grayCapacity);
        if (vm.grayStack == NULL) {
            fprintf(stderr, "Kesalahan fatal: memori habis (gray stack GC).\n");
            exit(1);
        }
    }

    vm.grayStack[vm.grayCount++] = object;
}

void markValue(Value value) {
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

static void markArray(ValueArray* array) {
    for (int i = 0; i < array->count; i++) {
        markValue(array->values[i]);
    }
}

static void blackenObject(Obj* object) {
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif

    switch (object->type) {
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            markObject((Obj*)function->name);
            markObject((Obj*)function->module);
            // paramNames holds ObjString*; missing this frees live names.
            for (int i = 0; i < function->arity && function->paramNames != NULL; i++) {
                markObject((Obj*)function->paramNames[i]);
            }
            markArray(&function->chunk.constants);
            // Source text for runtime error snippets.
            markObject((Obj*)function->chunk.source);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            markObject((Obj*)closure->function);
            for (int i = 0; i < closure->upvalueCount; i++) {
                markObject((Obj*)closure->upvalues[i]);
            }
            break;
        }
        case OBJ_UPVALUE:
            markValue(((ObjUpvalue*)object)->closed);
            break;
        case OBJ_LIST:
            markArray(&((ObjList*)object)->items);
            break;
        case OBJ_TUPLE:
            markArray(&((ObjTuple*)object)->items);
            break;
        case OBJ_DICT:
            markValueTable(&((ObjDict*)object)->table);
            markArray(&((ObjDict*)object)->keys);
            break;
        case OBJ_SET:
            markValueTable(&((ObjSet*)object)->table);
            markArray(&((ObjSet*)object)->keys);
            break;
        case OBJ_ITERATOR:
            markValue(((ObjIterator*)object)->source);
            break;
        case OBJ_GENERATOR: {
            // A suspended generator owns the only reference to its saved
            // locals; without marking them they are freed mid-iteration.
            ObjGenerator* gen = (ObjGenerator*)object;
            markObject((Obj*)gen->closure);
            for (int i = 0; i < gen->slotCount; i++) markValue(gen->slots[i]);
            break;
        }
        case OBJ_MODULE: {
            ObjModule* module = (ObjModule*)object;
            markObject((Obj*)module->name);
            markObject((Obj*)module->path);
            markTable(&module->globals);
            break;
        }
        case OBJ_CLASS: {
            ObjClass* klass = (ObjClass*)object;
            markObject((Obj*)klass->name);
            markObject((Obj*)klass->superclass);
            markTable(&klass->methods);
            markTable(&klass->fields);   // atribut kelas juga akar GC
            markTable(&klass->special);  // kuncinya ObjString -- wajib ditandai
            markTable(&klass->own);
            // Array MRO adalah akar GC sama seperti tabel: isinya ObjClass*
            // yang mungkin tidak dirujuk dari mana pun lagi.
            for (int i = 0; i < klass->mroCount; i++) {
                markObject((Obj*)klass->mro[i]);
            }
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            markObject((Obj*)instance->klass);
            markTable(&instance->fields);
            break;
        }
        case OBJ_BOUND_METHOD: {
            ObjBoundMethod* bound = (ObjBoundMethod*)object;
            markValue(bound->receiver);
            markObject((Obj*)bound->method);
            break;
        }
        case OBJ_FILE:
            markObject((Obj*)((ObjFile*)object)->path);
            break;
        case OBJ_DB:
        case OBJ_SOCKET:
            break;
        case OBJ_RANGE:   // holds only numbers
            break;
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
    }
}

static void freeObject(Obj* object) {
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)object, object->type);
#endif

    switch (object->type) {
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            freeChunk(&function->chunk);
            if (function->paramNames != NULL) {
                FREE_ARRAY(ObjString*, function->paramNames, function->arity);
            }
            FREE(ObjFunction, object);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
            FREE(ObjClosure, object);
            break;
        }
        case OBJ_UPVALUE:
            FREE(ObjUpvalue, object);
            break;
        case OBJ_TUPLE: {
            ObjTuple* tuple = (ObjTuple*)object;
            freeValueArray(&tuple->items);
            FREE(ObjTuple, object);
            break;
        }
        case OBJ_LIST: {
            ObjList* list = (ObjList*)object;
            freeValueArray(&list->items);
            FREE(ObjList, object);
            break;
        }
        case OBJ_DICT: {
            ObjDict* dict = (ObjDict*)object;
            freeValueTable(&dict->table);
            freeValueArray(&dict->keys);
            FREE(ObjDict, object);
            break;
        }
        case OBJ_SET: {
            ObjSet* set = (ObjSet*)object;
            freeValueTable(&set->table);
            freeValueArray(&set->keys);
            FREE(ObjSet, object);
            break;
        }
        case OBJ_ITERATOR:
            // `source` is a Value owned by the GC; nothing extra to release.
            FREE(ObjIterator, object);
            break;
        case OBJ_GENERATOR:
            // slots are inline Values owned by the GC; nothing extra to free.
            FREE(ObjGenerator, object);
            break;
        case OBJ_MODULE: {
            ObjModule* module = (ObjModule*)object;
            freeTable(&module->globals);
            FREE(ObjModule, object);
            break;
        }
        case OBJ_CLASS: {
            ObjClass* klass = (ObjClass*)object;
            freeTable(&klass->methods);
            freeTable(&klass->fields);
            freeTable(&klass->special);
            freeTable(&klass->own);
            FREE_ARRAY(struct ObjClass*, klass->mro, klass->mroCount);
            FREE(ObjClass, object);
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            freeTable(&instance->fields);
            FREE(ObjInstance, object);
            break;
        }
        case OBJ_BOUND_METHOD:
            FREE(ObjBoundMethod, object);
            break;
        case OBJ_FILE: {
            ObjFile* file = (ObjFile*)object;
            // Close the OS handle if the program forgot to. Prevents fd leaks
            // when a file object is collected while still open.
            if (file->isOpen && file->handle != NULL) {
                fclose((FILE*)file->handle);
                file->isOpen = false;
            }
            FREE(ObjFile, object);
            break;
        }
        case OBJ_DB: {
            ObjDb* db = (ObjDb*)object;
            if (db->db != NULL) sqlite3_close_v2(db->db);
            FREE(ObjDb, object);
            break;
        }
        case OBJ_SOCKET: {
            ObjSocket* sock = (ObjSocket*)object;
            // Bebaskan sesi TLS SEBELUM menutup fd: close_notify perlu soket
            // yang masih hidup. Terbalik urutannya = menulis ke fd tertutup.
            if (sock->tls != NULL) {
                indkTlsTutup(sock->tls);
                sock->tls = NULL;
            }
            // Close the socket if still open (prevents fd leaks on GC).
            if (sock->isOpen) {
                indkCloseSocket(sock->fd);
                sock->isOpen = false;
            }
            FREE(ObjSocket, object);
            break;
        }
        case OBJ_RANGE:
            FREE(ObjRange, object);
            break;
        case OBJ_NATIVE: {
            FREE(ObjNative, object);
            break;
        }
    }
}

static void markRoots() {
    // The value stack.
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    }

    // Call frames (closures currently executing).
    for (int i = 0; i < vm.frameCount; i++) {
        markObject((Obj*)vm.frames[i].closure);
    }

    // Global variables.
    markTable(&vm.globals);
    markTable(&vm.loadedModules);

    // Keyword argument names of the native call in flight. These live in a
    // chunk constant table, so they are already reachable through the running
    // closure — marked explicitly anyway so the invariant doesn't depend on
    // that indirection surviving future changes.
    markObject((Obj*)vm.nativeArgNames);

    // Open upvalues (still pointing at live stack slots).
    for (ObjUpvalue* upvalue = vm.openUpvalues;
         upvalue != NULL;
         upvalue = upvalue->next) {
        markObject((Obj*)upvalue);
    }

    // Anything the compiler is holding onto right now.
    markCompilerRoots();

    // The interned "init" method name and the magic method names (K19).
    markObject((Obj*)vm.initString);
    for (int i = 0; i < MAGIC_COUNT; i++) markObject((Obj*)vm.magicNames[i]);
    markObject((Obj*)vm.mainModule);
    markObject((Obj*)vm.scriptDir);

    // A pending native error value (raised but not yet unwound).
    markValue(vm.nativeErrorValue);
}

static void traceReferences() {
    while (vm.grayCount > 0) {
        Obj* object = vm.grayStack[--vm.grayCount];
        blackenObject(object);
    }
}

static void sweep() {
    Obj* previous = NULL;
    Obj* object = vm.objects;
    while (object != NULL) {
        if (object->isMarked) {
            object->isMarked = false;
            previous = object;
            object = object->next;
        } else {
            Obj* unreached = object;
            object = object->next;
            if (previous != NULL) {
                previous->next = object;
            } else {
                vm.objects = object;
            }
            freeObject(unreached);
        }
    }
}

void collectGarbage() {
#ifdef DEBUG_LOG_GC
    printf("-- gc mulai\n");
    size_t before = vm.bytesAllocated;
#endif

    markRoots();
    traceReferences();
    // Interned strings are weak references: drop any that are unreachable
    // before sweeping so we don't leave dangling keys in vm.strings.
    tableRemoveWhite(&vm.strings);
    sweep();

    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    printf("-- gc selesai\n");
    printf("   dikoleksi %zu byte (dari %zu ke %zu) berikutnya pada %zu\n",
           before - vm.bytesAllocated, before, vm.bytesAllocated, vm.nextGC);
#endif
}

void freeObjects() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }

    free(vm.grayStack);
}
