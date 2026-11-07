// libFuzzer target for the Indonesiaku scanner + compiler (front end only).
//
// Feeds arbitrary bytes as source code into the SCANNER + COMPILER to shake
// out crashes, buffer overruns, and UB in the front end. It deliberately does
// NOT execute the resulting bytecode: running arbitrary programs can loop
// forever (e.g. `selagi benar { ... }`), which produces libFuzzer timeouts
// that are false positives for a Turing-complete language. Compilation always
// terminates, so this target stays focused on front-end memory safety.
//
// Build with clang:
//
//   clang -g -O1 -fsanitize=fuzzer,address,undefined -Isrc \
//     tests/fuzz_compile.c src/indk_api.c src/chunk.c src/compiler.c src/debug.c \
//     src/format.c src/memory.c src/native.c src/object.c src/scanner.c \
//     src/table.c src/value.c src/vm.c src/db.c src/pola.c src/sistem.c \
//     src/sqlite3.c \
//     -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_DQS=0 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_MEMSTATUS=0 \
//     -lm -o fuzz_compile
//   ./fuzz_compile -max_len=4096 -timeout=5 -rss_limit_mb=2048
//
// Note: this links the real VM for its allocator / string interning / GC
// (needed by the compiler), but never calls interpret()/run(). The VM is
// reinitialized per input so global state does not leak between runs.

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"
#include "compiler.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // NUL-terminate the input as a C string (the scanner expects that).
    char* source = (char*)malloc(size + 1);
    if (source == NULL) return 0;
    memcpy(source, data, size);
    source[size] = '\0';

    initVM();
    // Compile only — do not execute. Result (if any) is owned by the VM's
    // object list and reclaimed by freeVM().
    compile(source);
    freeVM();

    free(source);
    return 0;
}
