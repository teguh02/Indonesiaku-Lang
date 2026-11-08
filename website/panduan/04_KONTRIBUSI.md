# 4. Panduan Kontribusi - Bahasa Pemrograman Indonesiaku

Terima kasih telah berminat berkontribusi pada proyek Bahasa Pemrograman Indonesiaku! Panduan ini menjelaskan cara setup development environment, membuat kontribusi, dan proses submission.

---

## 📋 Daftar Isi

1. [Prasyarat](#prasyarat)
2. [Setup Development Environment](#setup-development-environment)
3. [Struktur Proyek](#struktur-proyek)
4. [Jenis-jenis Kontribusi](#jenis-jenis-kontribusi)
5. [Workflow Kontribusi](#workflow-kontribusi)
6. [Code Guidelines](#code-guidelines)
7. [Pengujian](#pengujian)
8. [Pull Request Process](#pull-request-process)
9. [Reporting Issues](#melaporkan-kendala)

---

## Prasyarat

Sebelum memulai, pastikan Anda memiliki:

### 1. GCC Compiler
Digunakan untuk build C project:

```bash
# Windows (MinGW)
gcc --version

# Linux
sudo apt-get install build-essential

# macOS
xcode-select --install
```

### 2. Git
Untuk version control:

```bash
git --version
git config --global user.name "Nama Anda"
git config --global user.email "email@anda.com"
```

### 3. PowerShell (untuk Windows)
Digunakan untuk menjalankan test scripts:

```powershell
$PSVersionTable.PSVersion
```

### 4. Text Editor / IDE
Rekomendasi:
- VS Code (recommended)
- Sublime Text
- Vim
- Atau editor favorit Anda

---

## Setup Development Environment

### Step 1: Clone Repository

```bash
git clone https://github.com/teguh02/Indonesiaku-Lang.git
cd Indonesiaku
```

### Step 2: Verifikasi Setup

```bash
# Check GCC
gcc --version

# List files
ls -la

# Check existing build artifacts
ls *.exe  # Windows
./indk.exe --help  # Jika sudah ada
```

### Step 3: Build Project

Menggunakan GCC:

```bash
# Build dengan GCC
gcc -Wall -Wextra -std=c11 -O2 -Isrc -o indk.exe ^
    src/main.c src/chunk.c src/compiler.c src/debug.c ^
    src/memory.c src/object.c src/scanner.c src/table.c ^
    src/value.c src/vm.c -lm
```

Atau menggunakan Make (jika ada Makefile):

```bash
make
```

Atau menggunakan VS Code task:

```bash
Ctrl+Shift+B  # Build (Windows/Mac)
```

### Step 4: Test Build

```bash
# Run REPL
./indk.exe

# Test dengan file contoh
./indk.exe examples/hello.idk

# Output seharusnya: Halo, dunia!
```

---

## Struktur Proyek

```
Indonesiaku/
├── src/                    # Source code (C11)
│   ├── main.c             # Entry point
│   ├── vm.c/vm.h          # Virtual Machine
│   ├── compiler.c/compiler.h
│   ├── scanner.c/scanner.h
│   ├── debug.c/debug.h
│   ├── object.c/object.h
│   ├── table.c/table.h    # Hash table implementation
│   ├── chunk.c/chunk.h    # Bytecode chunks
│   ├── value.c/value.h    # Value types
│   ├── memory.c/memory.h  # Memory management
│   └── common.h           # Common definitions
├── examples/              # Test files (.idk)
│   ├── hello.idk
│   ├── fungsi.idk
│   ├── variabel.idk
│   ├── advanced/          # Advanced examples
│   └── oop/               # OOP pattern tests
├── tests/                 # Test automation
│   ├── test_all.sh       # Jalankan semua tes (Linux/macOS)
│   └── test_all.ps1      # Jalankan semua tes (Windows)
├── website/               # Situs dokumentasi + panduan pengguna
│   ├── index.html
│   ├── docs.html          # Markdown reader (array DOCS)
│   ├── assets/
│   └── panduan/           # Panduan pengguna (README, 01..07)
├── docs/                  # Perencanaan bahasa (internal)
│   ├── README.md
│   ├── ROADMAP.md
│   └── RENCANA_FITUR/     # Satu file per fitur yang direncanakan
├── Makefile              # Build configuration
└── README.md             # Project root readme
```

### Penting untuk Kontributors

**src/** - Jangan modifikasi kecuali perlu urgent fix
- Main implementation dari VM
- Perubahan memerlukan testing menyeluruh
- Konsultasi dulu dengan maintainer

**examples/** - Tempat yang bagus untuk kontribusi
- Test case baru
- Example programs
- Improvements tanpa risiko tinggi

**website/panduan/** - Dokumentasi pengguna
- Perbaikan typo
- Clarifications
- New language translations

**docs/** - Perencanaan bahasa
- Update roadmap (`docs/ROADMAP.md`)
- Usulan fitur baru memakai `docs/RENCANA_FITUR/TEMPLATE.md`

---

## Jenis-jenis Kontribusi

### 1. Bug Reports & Fixes

**Melaporkan bug:**

```bash
# Create issue di GitHub dengan:
- Title: "[BUG] Deskripsi singkat"
- Description: Langkah reproduksi
- Expected: Behavior yang diharapkan
- Actual: Behavior yang terjadi
- Environment: Spesifikasi sistem
```

**Membuat fix:**

```bash
# Create branch
git checkout -b fix/deskripsi-bug

# Buat perubahan
# Test untuk pastikan fix bekerja
# Commit dengan pesan deskriptif
git commit -m "Fix: Perbaiki [issue name]"

# Push dan buat PR
```

### 2. Fitur Baru

**Proposal fitur:**

```bash
# Buat issue dengan:
- Title: "[FEATURE] Deskripsi fitur"
- Motivation: Mengapa fitur ini penting?
- Implementation: Ide implementasi
- Examples: Contoh penggunaan
```

**Implementasi fitur:**

```bash
git checkout -b feature/nama-fitur
# Development & testing
git commit -m "Feature: Tambahkan [nama fitur]"
```

### 3. Documentation

**Improvements:**

```bash
git checkout -b docs/improvement
# Edit file .md
git commit -m "Docs: [deskripsi improvement]"
```

### 4. Tests & Contoh

**Test case baru:**

```bash
# Buat file di examples/
# example_nama.idk

git checkout -b test/nama-test
git commit -m "Test: Tambahkan test untuk [fitur]"
```

### 5. Optimasi Kinerja
**Optimization:**

```bash
git checkout -b perf/optimization-name
# Modifikasi code untuk performa lebih baik
# PENTING: Benchmark sebelum & sesudah
git commit -m "Perf: Optimize [bagian code]"
```

---

## Workflow Kontribusi

### Alur Kerja Git Standar
```bash
# 1. Fork repository (jika Anda bukan maintainer)
# Buat copy pribadi di akun GitHub Anda

# 2. Clone fork Anda
git clone https://github.com/USERNAME/Indonesiaku.git
cd Indonesiaku

# 3. Add upstream remote
git remote add upstream https://github.com/teguh02/Indonesiaku-Lang.git

# 4. Create feature branch
git checkout -b feature/deskripsi
# atau
git checkout -b fix/nama-bug
# atau
git checkout -b docs/perbaikan

# 5. Make changes
# Edit files sesuai kebutuhan
# Test perubahan Anda

# 6. Commit dengan pesan yang clear
git add .
git commit -m "Type: Deskripsi singkat

Penjelasan detail dari perubahan ini.
- Point 1
- Point 2"

# 7. Update dengan upstream (jika perlu)
git fetch upstream
git rebase upstream/main

# 8. Push ke fork Anda
git push origin feature/deskripsi

# 9. Buat Pull Request dari GitHub UI
# Isi template yang disediakan
```

### Commit Message Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types:**
- `feat`: Fitur baru
- `fix`: Bug fix
- `docs`: Documentation
- `test`: Test dan examples
- `perf`: Performance improvement
- `refactor`: Code refactor
- `chore`: Build, dependencies

**Contoh:**

```
feat(compiler): tambah support untuk multiple return values

- Modifikasi scanner untuk recognize new syntax
- Update compiler untuk handle multiple returns
- Tambah test case di examples/

Closes #123
```

---

## Code Guidelines

### C Code Style

**Naming conventions:**

```c
// ✅ BAIK
static void push_value(Value value);
typedef struct {
    int line_number;
    char* error_message;
} CompileError;
#define MAX_LOCALS 256

// ❌ BURUK
static void PushValue(Value value);
typedef struct {
    int lineNumber;
    char* errorMsg;
} compileError;
```

**Indentation:**

```c
// ✅ BAIK - 4 spaces
void execute_instruction(Instruction instr) {
    switch (instr.op) {
        case OP_ADD: {
            push_value(pop_value() + pop_value());
            break;
        }
    }
}

// ❌ BURUK - inconsistent
void execute_instruction(Instruction instr) {
  switch (instr.op) {
        case OP_ADD: {
            push_value(pop_value() + pop_value());
        }
  }
}
```

**Comments:**

```c
// ✅ BAIK - Clear dan helpful
// Combine two operands using the given operator
// Returns NULL if operation is invalid
ObjString* combine_strings(ObjString* left, ObjString* right) {
    // Implementation
}

// ❌ BURUK - Tidak informatif
// combine strings
ObjString* combine_strings(ObjString* left, ObjString* right) {
    // Implementation
}
```

**Function declarations:**

```c
// ✅ BAIK
static void initialize_compiler(Compiler* compiler, FunctionType type);
static void compile_expression(void);
static void emit_instruction(uint8_t instruction, int line);

// ❌ BURUK - Too long
static void InitializeTheCompilerWithTheGivenTypeInformation(
    Compiler* compiler_instance, FunctionType function_type);
```

### Header Files

**Struktur header file yang baik:**

```c
#ifndef INDONESIAKU_SCANNER_H
#define INDONESIAKU_SCANNER_H

#include "common.h"

typedef enum {
    TOKEN_PLUS,
    TOKEN_MINUS,
    // ...
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

void initialize_scanner(const char* source);
Token scan_token(void);

#endif
```

### Memory Management

```c
// ✅ BAIK - Proper allocation and deallocation
char* create_string(const char* source, int length) {
    char* string = (char*)malloc(length + 1);
    if (string == NULL) {
        return NULL;  // Handle error
    }
    memcpy(string, source, length);
    string[length] = '\0';
    return string;
}

void free_string(char* string) {
    free(string);
}

// ❌ BURUK - Memory leak
char* create_string(const char* source, int length) {
    char* string = (char*)malloc(length + 1);
    memcpy(string, source, length);
    string[length] = '\0';
    return string;  // Caller tidak tahu harus free
}
```

---

## Pengujian

### Run Existing Tests

```bash
# Run semua test
./indk.exe examples/hello.idk
./indk.exe examples/fibonacci.idk
./indk.exe examples/fungsi.idk

# Atau menggunakan PowerShell script (Windows)
powershell -ExecutionPolicy Bypass -File tests/test_all.ps1

# Atau menggunakan VS Code
# Ctrl+Shift+D (Run)
```

### Membuat Test Baru

**Test file format:**

```indonesiaku
# File: examples/test_fitur_baru.idk
# Tujuan: Test fitur XYZ
# Expected output:
# Line 1: hasil pertama
# Line 2: hasil kedua

# Test case 1: Basic usage
x = 10
cetak(x)

# Test case 2: Edge case
y = -5
cetak(y)
```

### Pengujian Otomatis

**Menambah test ke test_all.ps1:**

```powershell
# Format:
# $tests = @(
#     @{ file = "examples/test_name.idk"; expected = "output" }
# )
```

---

## Pull Request Process

### Sebelum Membuat PR

1. **Update code Anda:**

```bash
git fetch upstream
git rebase upstream/main
```

2. **Test semua:**

```bash
# Build
make clean && make

# Run tests
powershell -ExecutionPolicy Bypass -File tests/test_all.ps1

# Manual testing
./indk.exe examples/hello.idk
./indk.exe examples/fibonacci.idk
```

3. **Check for warnings:**

```bash
# Build dengan extra flags
gcc -Wall -Wextra -Werror -std=c11 -O2 -Isrc -o indk.exe \
    src/*.c -lm
```

### PR Template

Saat membuat PR, gunakan template ini:

```markdown
## Deskripsi

[Deskripsi singkat dari perubahan]

## Jenis Perubahan

- [ ] Bug fix
- [ ] Fitur baru
- [ ] Dokumentasi
- [ ] Performance improvement
- [ ] Refactoring

## Pengujian

- [ ] Tested locally
- [ ] Added/updated tests
- [ ] All tests passing

## Checklist

- [ ] Code follows style guide
- [ ] No new warnings
- [ ] Documentation updated
- [ ] Commits have descriptive messages
- [ ] Signed commits (optional)

## Related Issues

Closes #[issue number]
```

---

## Melaporkan Kendala
### Format Issue Report

**Title:**
```
[BUG] Scanner crashes on invalid escape sequence
[FEATURE] Add support for multiline comments
[DOCS] Fix typo in SYNTAX_REFERENCE.md
```

**Body:**

```markdown
## Deskripsi

Jelaskan apa masalahnya

## Reproduksi

Langkah-langkah untuk reproduce bug:
1. Buat file dengan kode: ...
2. Run: ./indk.exe file.idk
3. Hasilnya: ...

## Ekspektasi

Seharusnya: ...

## Environment

- OS: [Windows 10/Linux/macOS]
- GCC version: [output dari `gcc --version`]
- Commit: [commit hash jika ada]

## Screenshots/Logs

[Jika ada error message, paste di sini]
```

---

## Komunikasi

### Channel Komunikasi

- **Issues**: Bug reports, feature requests
- **Discussions**: Q&A, ideas, general discussion
- **Pull Requests**: Code review discussions
- **Email**: teguhrijanandi02@gmail.com (untuk kontribusi besar)

### Telaah Kode
Penulis PR akan:
- Memberikan masukan jika ada changes needed
- Approve jika sudah bagus
- Merge ke main branch

Reviewers akan melihat:
- Code quality
- Test coverage
- Documentation
- Performance implications

---

## Dokumentasi Tambahan

### Saat Menambah Fitur Baru

1. **Update SYNTAX_REFERENCE.md** jika ada sintaks baru
2. **Tambah contoh** ke examples/
3. **Update CHANGELOG.md** dengan changes
4. **Buat issue** untuk tracking

### Saat Membuat Breaking Change

1. **Diskusikan dulu** di issue/discussion
2. **Plan migration** untuk existing code
3. **Document thoroughly**
4. **Commit di version khusus** (misalnya v0.2)

---

## Tips untuk Kontributor
1. **Mulai kecil** - Kontribusi documentation atau test dulu
2. **Tanya dulu** - Buat issue sebelum besar-besaran development
3. **Baca existing code** - Pelajari code style yang digunakan
4. **Test thoroughly** - Pastikan perubahan tidak break existing functionality
5. **Be patient** - Review code membutuhkan waktu
6. **Learn git** - Memahami git workflow sangat membantu

---

## FAQ

**Q: Boleh modifikasi src/ langsung?**  
A: Hindari jika bisa. Jika perlu, pastikan: (1) Alasan jelas, (2) Test lengkap, (3) Konsultasi maintainer

**Q: Harus fork atau bisa push langsung?**  
A: Fork dulu jika bukan maintainer, push ke branch sendiri, buat PR

**Q: Bagaimana jika PR saya ditolak?**  
A: Normal! Tanya reviewer alasannya dan buat perbaikan

**Q: Berapa lama review PR?**  
A: Tergantung kompleksitas, usually 2-7 hari

**Q: Bisa kontribusi tanpa GitHub?**  
A: Ya, kirim patch via email, tapi GitHub lebih mudah

---

**Terima kasih telah berkontribusi pada Bahasa Pemrograman Indonesiaku! Kontribusi Anda sangat berarti untuk pengembangan bahasa ini.**

Versi: 1.0  
Terakhir diperbarui: 28 Oktober 2025
