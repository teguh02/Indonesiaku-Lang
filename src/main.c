#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>   // clock() untuk `indk profil`

#include "common.h"
#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "format.h"
#include "native.h"
#include "object.h"
#include "vm.h"
#include "sistem.h"

// Count net brace depth in a line, ignoring braces inside strings and after
// a '#' comment. Used by the REPL to keep reading until a block is complete.
static int braceDelta(const char* s) {
    int depth = 0;
    bool inString = false;
    char quote = 0;
    for (const char* p = s; *p; p++) {
        char c = *p;
        if (inString) {
            if (c == '\\' && p[1] != '\0') { p++; continue; }
            if (c == quote) inString = false;
        } else if (c == '"' || c == '\'') {
            inString = true;
            quote = c;
        } else if (c == '#') {
            break;  // rest of line is a comment
        } else if (c == '{') {
            depth++;
        } else if (c == '}') {
            depth--;
        }
    }
    return depth;
}

// Appends `line` to the growable *buffer. Returns false when out of memory.
static bool appendReplLine(char** buffer, size_t* length, size_t* capacity,
                           const char* line, size_t lineLength) {
    if (*length + lineLength + 1 > *capacity) {
        size_t grown = *capacity;
        while (*length + lineLength + 1 > grown) grown *= 2;
        char* bigger = (char*)realloc(*buffer, grown);
        if (bigger == NULL) return false;
        *buffer = bigger;
        *capacity = grown;
    }
    memcpy(*buffer + *length, line, lineLength);
    *length += lineLength;
    (*buffer)[*length] = '\0';
    return true;
}

static void repl() {
    // Both grow: a fixed 1024-byte line split long input in two, and the
    // 8192-byte block buffer silently dropped every line past its end (T-U10).
    size_t capacity = 8192;
    char* buffer = (char*)malloc(capacity);
    if (buffer == NULL) return;
    bool firstLineEver = true;

    for (;;) {
        size_t length = 0;
        buffer[0] = '\0';
        int depth = 0;
        bool first = true;

        // Read one logical unit: keep reading continuation lines while there
        // are unclosed braces so multi-line blocks (fungsi/kelas/jika/...) work.
        for (;;) {
            printf(first ? "> " : "... ");
            size_t lineLength;
            char* line = readLineAlloc(stdin, &lineLength);
            if (line == NULL) {
                printf("\n");
                if (buffer[0] != '\0') interpret(buffer);
                free(buffer);
                return;
            }
            first = false;

            // What the REPL reads is SOURCE, so a UTF-8 BOM at the very start
            // is dropped as readFile() does for a script (T-W2). Windows
            // PowerShell 5.1 prepends one when piping with a UTF-8 encoding
            // (`"x = 21" | indk` failed with "Karakter tidak dikenal", T-W12);
            // `printf '\xef\xbb\xbf...' | python` likewise runs the code.
            //
            // Every CONSECUTIVE leading BOM goes, not just one: after
            // `$OutputEncoding = [Text.Encoding]::UTF8` -- the usual advice,
            // and the case that found this bug -- PowerShell 5.1 writes the
            // preamble twice (EF BB BF EF BB BF, seen with od). Python stops
            // at one and fails on the second; stopping there too would leave
            // exactly those users broken. U+FEFF at the start of code never
            // means anything, so dropping a run of them loses nothing.
            //
            // Only the start of the session: a BOM later is a stray
            // character, as in a file. input() is untouched on purpose --
            // Python's input() keeps U+FEFF too (WIN-12, docs/TESTING).
            if (firstLineEver) {
                size_t before;
                do {
                    before = lineLength;
                    lineLength = stripUtf8Bom(line, lineLength);
                } while (lineLength != before);
                firstLineEver = false;
            }

            bool appended = appendReplLine(&buffer, &length, &capacity, line, lineLength);
            depth += braceDelta(line);
            free(line);
            if (!appended) {
                fprintf(stderr, "Tidak cukup memori untuk baris REPL.\n");
                free(buffer);
                return;
            }
            if (depth <= 0) break;  // block complete (or no block opened)
        }

        interpret(buffer);
    }
}

static char* readFile(const char* path) {
    FILE* file = sistemBukaBerkas(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Tidak dapat membuka file \"%s\".\n", path);
        exit(74);
    }

    // Read in chunks instead of trusting ftell(): on a pipe or character
    // device (indk /dev/stdin) seeking fails, ftell() returns -1, and -1 in a
    // size_t made the old malloc(fileSize + 1) allocate zero bytes -- fread
    // then wrote past it. Growing as we go is correct for both files and pipes.
    size_t capacity = 8192;
    size_t bytesRead = 0;
    char* buffer = (char*)malloc(capacity);
    if (buffer == NULL) {
        fprintf(stderr, "Tidak cukup memori untuk membaca \"%s\".\n", path);
        exit(74);
    }
    for (;;) {
        if (bytesRead + 1 == capacity) {
            capacity *= 2;
            char* grown = (char*)realloc(buffer, capacity);
            if (grown == NULL) {
                free(buffer);
                fprintf(stderr, "Tidak cukup memori untuk membaca \"%s\".\n", path);
                exit(74);
            }
            buffer = grown;
        }
        size_t n = fread(buffer + bytesRead, 1, capacity - bytesRead - 1, file);
        bytesRead += n;
        if (n == 0) break;
    }
    if (ferror(file)) {
        free(buffer);
        fprintf(stderr, "Tidak dapat membaca file \"%s\".\n", path);
        exit(74);
    }
    if (sourceHasNul(buffer, bytesRead)) {
        free(buffer);
        fprintf(stderr, "Berkas \"%s\" berisi bita NUL; kode sumber tidak boleh memuatnya.\n", path);
        exit(65);
    }

    buffer[bytesRead] = '\0';
    bytesRead = stripUtf8Bom(buffer, bytesRead);
    normalizeNewlines(buffer, bytesRead);

    fclose(file);
    return buffer;
}

// indk --dis <skrip>: compile only, print bytecode, never run.
// Recurses into nested function constants so the whole program is covered.
static void disassembleFunction(ObjFunction* function) {
    const char* name = function->name != NULL ? function->name->chars : "<skrip utama>";
    disassembleChunk(&function->chunk, name);
    for (int i = 0; i < function->chunk.constants.count; i++) {
        Value v = function->chunk.constants.values[i];
        if (IS_FUNCTION(v)) {
            printf("\n");
            disassembleFunction(AS_FUNCTION(v));
        }
    }
}

static int disassembleFile(const char* path) {
    char* source = readFile(path);
    setScriptPath(path);
    ObjFunction* function = compile(source);
    if (function == NULL) {
        free(source);
        return 65;
    }
    disassembleFunction(function);
    free(source);
    return 0;
}

// indk periksa <skrip>: compile with lint on, report, never run.
// Exit 0 when clean, 1 when there are findings.
static int lintFile(const char* path) {
    char* source = readFile(path);
    setScriptPath(path);
    compilerSetLint(true);
    ObjFunction* function = compile(source);
    compilerSetLint(false);
    free(source);
    if (function == NULL) return 65;
    compilerLintPrint(path);
    return compilerLintCount() > 0 ? 1 : 0;
}

// indk rapikan [--tulis|--periksa] <skrip>
static int formatFile(const char* path, bool write, bool checkOnly) {
    char* source = readFile(path);
    char* formatted = formatSource(source);
    if (formatted == NULL) {
        free(source);
        fprintf(stderr, "Tidak dapat memindai \"%s\".\n", path);
        return 65;
    }

    int code = 0;
    if (checkOnly) {
        if (strcmp(source, formatted) != 0) {
            fprintf(stderr, "%s belum rapi\n", path);
            code = 1;
        }
    } else if (write) {
        if (strcmp(source, formatted) != 0) {
            FILE* f = sistemBukaBerkas(path, "wb");
            if (f == NULL) {
                fprintf(stderr, "Tidak dapat menulis \"%s\".\n", path);
                code = 74;
            } else {
                fwrite(formatted, 1, strlen(formatted), f);
                fclose(f);
            }
        }
    } else {
        fputs(formatted, stdout);
    }

    free(formatted);
    free(source);
    return code;
}

// indk uji <skrip>: jalankan skrip lalu panggil setiap fungsi uji_*.
static int testFile(const char* path) {
    char* source = readFile(path);
    setScriptPath(path);
    InterpretResult result = interpret(source);
    free(source);
    if (result == INTERPRET_COMPILE_ERROR) return 65;
    if (result == INTERPRET_RUNTIME_ERROR) return 70;

    int failed = runUnitTests();
    if (failed < 0) {
        fprintf(stderr, "Tidak ada fungsi uji_* di \"%s\".\n", path);
        return 1;
    }
    return failed > 0 ? 1 : 0;
}

static int runFile(const char* path) {
    char* source = readFile(path);
    // Imports resolve relative to the script's own directory first.
    setScriptPath(path);
    InterpretResult result = interpret(source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR) return 65;
    if (result == INTERPRET_RUNTIME_ERROR) return 70;
    return 0;
}

int main(int argc, const char* argv[]) {
    sistemSiapkanProses(&argc, &argv);   // Windows: UTF-8 argv + console
    sistemAturArgv0(argv[0]);

    // `indk --sandbox skrip.idk ...`: refuse environment, directory listing
    // and running programs (the web playground runs every program this way).
    // Only before the script path, so a script still receives its own
    // "--sandbox" argument untouched.
    if (argc >= 2 && strcmp(argv[1], "--sandbox") == 0) {
        sistemAturSandbox(true);
        argv++;
        argc--;
    }

    // Handle simple CLI flags only when they are the first argument, so a
    // script can receive its own "-v"/"-h" arguments without interception.
    if (argc >= 2) {
        if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
            printf("%s\n", INDK_VERSION);
            return 0;
        }
        if (strcmp(argv[1], "rapikan") == 0) {
            bool write = false, checkOnly = false;
            int argIndex = 2;
            while (argIndex < argc && argv[argIndex][0] == '-') {
                if (strcmp(argv[argIndex], "--tulis") == 0) write = true;
                else if (strcmp(argv[argIndex], "--periksa") == 0) checkOnly = true;
                else {
                    fprintf(stderr, "Opsi tidak dikenal: %s\n", argv[argIndex]);
                    return 64;
                }
                argIndex++;
            }
            if (argIndex >= argc) {
                fprintf(stderr, "Penggunaan: indk rapikan [--tulis|--periksa] <skrip.idk>\n");
                return 64;
            }
            return formatFile(argv[argIndex], write, checkOnly);
        }
        if (strcmp(argv[1], "uji") == 0) {
            if (argc < 3) {
                fprintf(stderr, "Penggunaan: indk uji <skrip.idk>\n");
                return 64;
            }
            initVM();
            defineArgs(argc, argv, 3);
            int code = testFile(argv[2]);
            freeVM();
            return code;
        }
        if (strcmp(argv[1], "awakutu") == 0) {
            if (argc < 3) {
                fprintf(stderr, "Penggunaan: indk awakutu <skrip.idk>\n");
                return 64;
            }
            initVM();
            defineArgs(argc, argv, 3);
            fprintf(stderr, "Debugger Indonesiaku. Ketik 'bantuan' untuk daftar perintah.\n");
            mulaiAwakutu();
            int code = runFile(argv[2]);
            freeVM();
            return code;
        }
        if (strcmp(argv[1], "profil") == 0) {
            if (argc < 3) {
                fprintf(stderr, "Penggunaan: indk profil <skrip.idk>\n");
                return 64;
            }
            initVM();
            defineArgs(argc, argv, 3);
            // Laporan ke stderr: program yang diprofilkan tetap boleh dipipa
            // ke berkas tanpa tercampur angka profil.
            mulaiProfil();
            clock_t mulai = clock();
            int code = runFile(argv[2]);
            double detik = (double)(clock() - mulai) / CLOCKS_PER_SEC;
            fflush(stdout);
            cetakProfil(detik);
            freeVM();
            return code;
        }
        if (strcmp(argv[1], "periksa") == 0) {
            if (argc < 3) {
                fprintf(stderr, "Penggunaan: indk periksa <skrip.idk>\n");
                return 64;
            }
            initVM();
            int code = lintFile(argv[2]);
            freeVM();
            return code;
        }
        if (strcmp(argv[1], "--dis") == 0) {
            if (argc < 3) {
                fprintf(stderr, "Penggunaan: indk --dis <skrip.idk>\n");
                return 64;
            }
            initVM();
            int code = disassembleFile(argv[2]);
            freeVM();
            return code;
        }
        if (strcmp(argv[1], "--periksa-pembaruan") == 0) {
            // Logikanya ada di pustaka/pembaruan.idk, bukan di C: perbandingan
            // versi dan penyusunan pesan jauh lebih mudah ditulis dan diuji
            // sebagai Indonesiaku, dan skripnya ikut terpasang bersama biner.
            //
            // Nama OS dan versi disisipkan dari sini supaya tidak perlu
            // menambah native baru hanya untuk dua konstanta.
#if defined(_WIN32)
            const char* osNama = "windows";
#elif defined(__APPLE__)
            const char* osNama = "macos";
#else
            const char* osNama = "linux";
#endif
            char sumber[256];
            snprintf(sumber, sizeof sumber,
                     "impor pembaruan\n"
                     "cetak(pembaruan.periksa(\"%s\", \"%s\"))\n",
                     osNama, INDK_VERSION);
            initVM();
            InterpretResult hasil = interpret(sumber);
            freeVM();
            return hasil == INTERPRET_OK ? 0 : 70;
        }
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("Indonesiaku %s\n", INDK_VERSION);
            printf("Penggunaan: indk [--sandbox] [path] [argumen...]\n");
            printf("Opsi:\n");
            printf("  -v, --version   Tampilkan versi\n");
            printf("  -h, --help      Tampilkan bantuan\n");
            printf("  --sandbox       Tolak lingkungan, isi direktori, dan menjalankan program\n");
            printf("  --dis <skrip>   Tampilkan bytecode tanpa menjalankan\n");
            printf("  --periksa-pembaruan  Periksa apakah ada versi lebih baru\n");
            printf("Perintah:\n");
            printf("  periksa <skrip> Laporkan variabel lokal tidak terpakai\n");
            printf("  uji <skrip>     Jalankan fungsi uji_* dalam skrip\n");
            printf("  rapikan <skrip> Rapikan format kode (--tulis, --periksa)\n");
            printf("  profil <skrip>  Hitung opcode dan panggilan (laporan ke stderr)\n");
            printf("  awakutu <skrip> Debugger: breakpoint, langkah, lihat variabel\n");
            return 0;
        }
    }

    initVM();

    int exitCode = 0;
    if (argc == 1) {
        repl();
    } else {
        // indk script.idk [arg1 arg2 ...]
        defineArgs(argc, argv, 2);
        exitCode = runFile(argv[1]);
    }

    freeVM();
    return exitCode;
}
