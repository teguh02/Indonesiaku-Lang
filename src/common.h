#ifndef indonesiaku_common_h
#define indonesiaku_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// A NUL byte in source would end the text for the scanner, which works on C
// strings: everything after it was silently ignored when running, and
// `rapikan --tulis` wrote the truncated text back to disk (T-U11). Python
// rejects such source too; every source reader must check this.
static inline bool sourceHasNul(const char* text, size_t length) {
    return memchr(text, '\0', length) != NULL;
}

// Uncomment untuk debugging
// #define DEBUG_PRINT_CODE
// #define DEBUG_TRACE_EXECUTION

// Uncomment untuk debugging garbage collector
// #define DEBUG_STRESS_GC   // Jalankan GC pada setiap alokasi
// #define DEBUG_LOG_GC      // Cetak aktivitas GC

#define UINT8_COUNT (UINT8_MAX + 1)

// Application version
#define INDK_VERSION "1.4.0"

// Universal newlines, like Python reading a source file: "\r\n" and a lone
// "\r" become "\n", in place; returns the new length. Without this a file
// checked out with CRLF (git on Windows) leaks '\r' into triple-quoted
// strings, panjang(), and error snippets.
// Drop a leading UTF-8 byte order mark (EF BB BF), as Python does for source
// files; Notepad and VS Code's "UTF-8 with BOM" write one. `text` must be
// NUL-terminated at `length`; returns the new length. Call it before
// normalizeNewlines on every path that reads source.
static inline size_t stripUtf8Bom(char* text, size_t length) {
    if (length >= 3 && (unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB &&
        (unsigned char)text[2] == 0xBF) {
        memmove(text, text + 3, length - 3 + 1);   // + 1: the terminator
        return length - 3;
    }
    return length;
}

static inline size_t normalizeNewlines(char* text, size_t length) {
    size_t out = 0;
    for (size_t i = 0; i < length; i++) {
        if (text[i] == '\r') {
            text[out++] = '\n';
            if (i + 1 < length && text[i + 1] == '\n') i++;
        } else {
            text[out++] = text[i];
        }
    }
    text[out] = '\0';
    return out;
}

#endif
