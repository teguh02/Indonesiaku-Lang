# Makefile untuk Bahasa Pemrograman Indonesiaku

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -Isrc -Isrc/vendor/mbedtls/include
LDFLAGS = -lm

# Tambahan flag dari pemanggil, mis. EXTRA_CFLAGS=-march=x86-64 saat rilis.
# Pakai ini, JANGAN 'make CFLAGS=...': variabel di baris perintah menimpa
# seluruh CFLAGS di atas sehingga -O2/-Isrc ikut hilang.
CFLAGS += $(EXTRA_CFLAGS)

# Windows executable extension
ifeq ($(OS),Windows_NT)
    EXE = .exe
    RM = del /Q
    RMDIR = rmdir /S /Q
    # On Windows, del requires backslash paths; / is parsed as a switch.
    CLEAN = del /Q src\*.o src\vendor\mbedtls\library\*.o $(TARGET) 2>NUL & exit 0
    # Winsock for TCP sockets (soket_* natives).
    # bcrypt: mbedTLS mengambil entropi lewat BCryptGenRandom di Windows
    # (library/entropy_poll.c). Tanpa ini link gagal dengan
    # "undefined reference to `BCryptGenRandom'" -- dan hanya di Windows,
    # jadi build Linux/macOS yang hijau tidak membuktikan apa pun.
    LDFLAGS += -lws2_32 -lbcrypt
else
    EXE =
    RM = rm -f
    RMDIR = rm -rf
    CLEAN = rm -f $(OBJECTS) $(TARGET)
endif

TARGET = indk$(EXE)

# mbedTLS yang di-vendor (src/vendor/mbedtls). Dikumpulkan dengan wildcard
# karena jumlahnya 108 berkas dan daftar manual pasti basi saat versi dinaikkan
# lewat tools/vendor_mbedtls.sh.
MBEDTLS_SOURCES = $(wildcard src/vendor/mbedtls/library/*.c)
MBEDTLS_OBJECTS = $(MBEDTLS_SOURCES:.c=.o)

SOURCES = src/main.c \
          src/chunk.c \
          src/compiler.c \
          src/debug.c \
          src/format.c \
          src/memory.c \
          src/native.c \
          src/db.c \
          src/pola.c \
          src/sistem.c \
          src/sqlite3.c \
          src/tls.c \
          src/object.c \
          src/scanner.c \
          src/table.c \
          src/value.c \
          src/vm.c

OBJECTS = $(SOURCES:.c=.o) $(MBEDTLS_OBJECTS)

# All headers. Because this project uses a single-pass, tightly-coupled design
# where almost every .c includes many headers, treat every header as a
# dependency of every object. This is coarse but prevents stale builds after a
# header change (the previous rule had no header deps, causing silent
# miscompiles when only .h files changed).
HEADERS = $(wildcard src/*.h)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo Build berhasil! Jalankan dengan: ./$(TARGET) atau ./$(TARGET) file.idk

SQLITE_FLAGS = -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_DQS=0 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_MEMSTATUS=0

# Preserve upstream bytes and suppress only vendor warnings. Keep sanitizer flags.
src/sqlite3.o: src/sqlite3.c src/sqlite3.h
	$(CC) $(filter-out -Wall -Wextra,$(CFLAGS)) $(SQLITE_FLAGS) -c $< -o $@

# Sama seperti sqlite3: byte upstream dipertahankan, hanya warning vendor yang
# diredam. Aturan ini TIDAK boleh ikut $(HEADERS) -- header kita tidak relevan
# bagi mbedTLS dan akan memaksa 108 berkas dikompilasi ulang setiap edit.
src/vendor/mbedtls/library/%.o: src/vendor/mbedtls/library/%.c
	$(CC) $(filter-out -Wall -Wextra,$(CFLAGS)) -c $< -o $@

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(CLEAN)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./$(TARGET) examples/hello.idk

.PHONY: all clean run test
