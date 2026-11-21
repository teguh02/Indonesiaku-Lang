// Translation unit for the vendored miniz (src/vendor/miniz, PROVENANCE.md).
//
// miniz.c uses fseeko/ftello on Linux/macOS/FreeBSD, which -std=c11 hides
// unless a feature-test macro is set; GCC 14 turns the resulting implicit
// declaration into an error. The macros are set HERE, scoped to miniz, so the
// vendor bytes stay untouched and no other file's compilation changes.
// _LARGEFILE64_SOURCE is what miniz itself asks for on glibc (fopen64 etc.,
// archives above 2 GiB).

#if !defined(_WIN32)
#define _DEFAULT_SOURCE
#define _LARGEFILE64_SOURCE 1
#endif

// Vendor code: its own warnings are not ours to fix.
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

#include "vendor/miniz/miniz.c"
