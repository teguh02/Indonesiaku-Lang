# miniz vendor provenance

Version: 3.1.2 (released 2026-07-01), the amalgamated `miniz.c`/`miniz.h`
from the official release archive.

Official archive: https://github.com/richgel999/miniz/releases/download/3.1.2/miniz-3.1.2.zip
Archive SHA256: `f0446d863f9c19926ad9483c523fdc42e42b8d4a6a431d27e09d49c79a140d9a`
(matches the `digest` GitHub publishes for the release asset).

Unmodified extracted files:
- `miniz.c` SHA256: `e2c1aeb66eef9191d8c3feb164db2def2335a61d039bf04ed849f6b042433b30`
- `miniz.h` SHA256: `b53b62ed122e559b8f679e3cb787a0b0035fe87a58f909da0e44931678f4e85f`
- `LICENSE` SHA256: `0115478d567121238cf6cc1c0c361926cf07a49d9e4c9e66da97fac6a01646b3`

License: MIT (see `LICENSE`, copied unchanged from the archive).

Used by `src/kompres.c` (zlib/raw deflate streams, CRC-32, Adler-32) and
`src/arsip.c` / `src/arsip_buat.c` (ZIP reader/writer). The gzip wrapper and
the TAR format are implemented in those files, not in miniz. Our files define
`MINIZ_NO_ZLIB_COMPATIBLE_NAMES` before including `miniz.h` so the zlib-style
macros (`crc32`, `inflate`, ...) do not leak into the interpreter.

No vendor source modifications. The Makefile compiles `miniz.c` with the
project flags minus `-Wall -Wextra` (sanitizer flags are kept). Update only
from a pinned official release archive, and record the new hashes here.
