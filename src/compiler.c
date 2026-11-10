#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "object.h"
#include "memory.h"

#if 0
// Disable debug printing of compiled bytecode by default in release builds.
// Define DEBUG_PRINT_CODE for local debugging when needed.
// #define DEBUG_PRINT_CODE

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif
#endif

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // atau
    PREC_AND,         // dan
    PREC_NOT,         // tidak (lebih lemah dari perbandingan, seperti Python)
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_BIT_OR,      // |   (bitwise: Python's order, all above comparisons)
    PREC_BIT_XOR,     // ^
    PREC_BIT_AND,     // &
    PREC_SHIFT,       // << >>
    PREC_TERM,        // + -
    PREC_FACTOR,      // * / % //
    PREC_UNARY,       // - !
    PREC_POWER,       // ** (lebih kuat dari unary: -2 ** 2 == -4)
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name;
    int depth;
    bool isCaptured;
    bool used;       // name was ever READ (assignment alone does not count)
    bool lintCheck;  // a real user declaration worth linting (see markLintable)
} Local;

// ---- Linter (indk periksa) -------------------------------------------------
// Off by default: a normal `indk program.idk` run must stay byte-identical,
// and every golden test requires empty stderr.
#define LINT_MAX 256
typedef struct { int line; char name[64]; } LintFinding;
static bool lintEnabled = false;
static LintFinding lintFindings[LINT_MAX];
static int lintCount = 0;

void compilerSetLint(bool enabled) {
    lintEnabled = enabled;
    if (enabled) lintCount = 0;   // reset when turning ON, never when turning off
}
int compilerLintCount(void) { return lintCount; }

static void lintReportUnused(Token* name, int line) {
    if (!lintEnabled || lintCount >= LINT_MAX) return;
    if (name->length == 0) return;   // reserved slot 0 (receiver/function)
    // '_' prefix is the conventional "deliberately unused" marker.
    if (name->length > 0 && name->start[0] == '_') return;
    // Synthetic locals are named with a leading space and are never user code.
    if (name->length > 0 && name->start[0] == ' ') return;
    LintFinding* f = &lintFindings[lintCount++];
    f->line = line;
    int n = name->length < 63 ? name->length : 63;
    memcpy(f->name, name->start, n);
    f->name[n] = '\0';
}

void compilerLintPrint(const char* path) {
    for (int i = 0; i < lintCount; i++) {
        fprintf(stderr, "%s:%d  variabel lokal '%s' tidak pernah dipakai\n",
                path, lintFindings[i].line, lintFindings[i].name);
    }
    fprintf(stderr, "%d temuan\n", lintCount);
}


typedef struct {
    uint8_t index;
    bool isLocal;
} Upvalue;

typedef enum {
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_METHOD,
    TYPE_STATIC,        // @metode_statis: tanpa penerima sama sekali
    TYPE_CLASSMETHOD,   // @metode_kelas: slot 0 berisi kelasnya
    TYPE_SCRIPT
} FunctionType;

#define FINALLY_MAX 32
// Inlined copies of 'akhirnya' blocks allowed per function. A 'kembalikan'
// inside an 'akhirnya' re-inlines the enclosing ones, so nested blocks grow
// the code exponentially (the fuzzer hung the compiler with it); real code
// needs a handful, and the cap keeps compile time linear.
#define MATCH_CASE_MAX 128
#define MATCH_ALT_MAX 32
#define FINALLY_INLINE_MAX 64

struct LoopCtx;

// What exitTryBlocks() must emit when jumping out of a region.
typedef enum {
    FINALLY_SOURCE,     // 'akhirnya': re-scan and re-compile its block
    FINALLY_EXIT_CALL   // 'dengan': call _keluar_() on the manager local
} FinallyKind;

// A 'coba ... akhirnya' or a 'dengan' block whose body is being compiled.
// Both need the same guarantee -- run something before any jump out -- so
// 'dengan' reuses this machinery instead of growing a parallel one.
typedef struct {
    FinallyKind kind;
    int tryDepth;               // Compiler.tryDepth before its handler
    ScannerState finallyBlock;  // FINALLY_SOURCE: scanner just before its '{'
    uint8_t managerSlot;        // FINALLY_EXIT_CALL: local holding the manager
    struct LoopCtx* loop;       // loop enclosing the statement
} FinallyRegion;

typedef struct Compiler {
    struct Compiler* enclosing;
    ObjFunction* function;
    FunctionType type;

    Local locals[UINT8_COUNT];
    int localCount;
    Upvalue upvalues[UINT8_COUNT];
    Token globals[UINT8_COUNT];
    int globalCount;
    Token nonlocals[UINT8_COUNT];
    int nonlocalCount;
    int scopeDepth;
    int tryDepth;      // 'coba' handlers active at this point in the function
    // Enclosing 'coba' blocks that have an 'akhirnya', innermost last.
    // 'kembalikan'/'hentikan'/'lanjut' re-compile their finally blocks
    // inline before jumping out (see exitTryBlocks).
    FinallyRegion finallies[FINALLY_MAX];
    int finallyCount;
    int finallyInlines;           // copies made so far (FINALLY_INLINE_MAX)
    bool finallyBudgetReported;   // report the cap once per function
    bool isGenerator;             // a 'hasilkan' was compiled in this function
} Compiler;

Parser parser;
Compiler* current = NULL;
// First token of the call argument being compiled (NULL outside a call).
// Used to reject Python-style keyword arguments, which are not supported yet.
static const char* callArgStart = NULL;

typedef struct ClassCompiler {
    struct ClassCompiler* enclosing;
    bool hasSuperclass;
} ClassCompiler;

ClassCompiler* currentClass = NULL;

// Tracks the innermost enclosing loop so 'hentikan' (break) and 'lanjut'
// (continue) know where to jump and how many locals to discard.
typedef struct LoopCtx {
    struct LoopCtx* enclosing;
    int continueTarget;   // bytecode offset to jump to for 'lanjut'
    int scopeDepth;       // scope depth of the loop body
    int tryDepth;         // Compiler.tryDepth when the loop started
    int finallyCount;     // Compiler.finallyCount when the loop started
    int breakJumps[UINT8_COUNT];
    int breakCount;
} LoopCtx;

LoopCtx* currentLoop = NULL;

static Chunk* currentChunk() {
    return &current->function->chunk;
}

// > 0 while an 'akhirnya' block is being re-compiled inline. Its errors are
// already reported when the block is compiled in place, so stay silent.
static int inlineFinallyDepth = 0;

// Set after an error that leaves the rest of the compile meaningless (the
// 'akhirnya' copy cap): later errors are only fallout from it, so they are
// counted but not printed. Reset by compile().
static bool suppressFurtherErrors = false;

// Walk back to the start of the token's line. Returns NULL when the token
// doesn't belong to the buffer currently being scanned.
static const char* lineStartOf(Token* token) {
    const char* source = scannerSourceStart();
    if (source == NULL || token->start == NULL) return NULL;
    // Both bounds: a token from a different buffer (a freed f-string slice,
    // say) can sit above `source` and would otherwise be walked backwards
    // out of bounds.
    if (token->start < source || token->start > scannerSourceEnd()) return NULL;
    const char* lineStart = token->start;
    while (lineStart > source && lineStart[-1] != '\n') lineStart--;
    return lineStart;
}

// Print the offending source line with a caret under the token. Long lines
// are windowed around the token so a 500-character line doesn't flood the
// terminal. Tabs are copied verbatim into the caret line so the caret stays
// aligned whatever the terminal's tab width is.
static void printSourceSnippet(Token* token, const char* lineStart) {
    if (lineStart == NULL) return;

    const char* lineEnd = token->start;
    while (*lineEnd != '\0' && *lineEnd != '\n') lineEnd++;

    // Window a long line around the token.
    const int kWindow = 40;
    const char* from = lineStart;
    const char* to = lineEnd;
    bool cutLeft = false, cutRight = false;
    if (token->start - from > kWindow) {
        from = token->start - kWindow;
        cutLeft = true;
    }
    if (to - token->start > kWindow) {
        to = token->start + kWindow;
        cutRight = true;
    }

    fprintf(stderr, "    %s%.*s%s\n",
            cutLeft ? "…" : "", (int)(to - from), from, cutRight ? "…" : "");

    // Caret line: same leading whitespace, tabs preserved.
    fprintf(stderr, "    %s", cutLeft ? " " : "");
    for (const char* p = from; p < token->start; p++) {
        fputc(*p == '\t' ? '\t' : ' ', stderr);
    }
    int caretLen = token->length > 0 ? token->length : 1;
    if (token->start + caretLen > to) caretLen = (int)(to - token->start);
    if (caretLen < 1) caretLen = 1;
    for (int i = 0; i < caretLen; i++) fputc('^', stderr);
    fputc('\n', stderr);
}

static void errorAt(Token* token, const char* message) {
    if (parser.panicMode) return;
    parser.panicMode = true;
    parser.hadError = true;
    if (inlineFinallyDepth > 0 || suppressFurtherErrors) return;

    const char* lineStart = lineStartOf(token);

    if (lineStart != NULL) {
        fprintf(stderr, "[baris %d, kolom %d] Kesalahan", token->line,
                (int)(token->start - lineStart) + 1);
    } else {
        fprintf(stderr, "[baris %d] Kesalahan", token->line);
    }

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " di akhir");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else if (token->length == 1 && *token->start == '\n') {
        // A newline token would break the message across lines.
        fprintf(stderr, " di akhir baris");
    } else {
        fprintf(stderr, " di '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    printSourceSnippet(token, lineStart);
    parser.hadError = true;
}

static void error(const char* message) {
    errorAt(&parser.previous, message);
}

// Report code that parses fine but is not allowed. The parser is not out of
// sync, so skip panic-mode recovery to avoid cascading follow-on errors.
static void semanticError(const char* message) {
    if (parser.panicMode) return;
    errorAt(&parser.previous, message);
    parser.panicMode = false;
}

static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

// One extra token of lookahead, used to spot `nama=` in an argument list.
// The scanner is rewound afterwards, so no token is consumed.
static bool checkNext(TokenType type) {
    ScannerState saved = saveScanner();
    Token next = scanToken();
    restoreScanner(saved);
    return next.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

// While an f-string's embedded expression is compiled, the scanner runs over
// a temporary copy of just that expression, so a column computed from it
// would point at the wrong place in the real line -- and the token itself
// belongs to a buffer that is about to be freed. Columns are therefore
// pre-computed from the real line before the sub-scanner starts, and every
// byte emitted inside the expression reuses that value. 0 means "unknown".
static int columnOverride = 0;

static void emitByte(uint8_t byte) {
    int column = columnOverride;
    if (column == 0) {
        // ponytail: the column is wherever parsing currently sits, which for
        // a binary operator is the end of its right operand, not the
        // operator. Close enough to find the expression; pointing at the
        // operator would mean every parse rule tracking its own token.
        const char* lineStart = lineStartOf(&parser.previous);
        if (lineStart != NULL) {
            column = (int)(parser.previous.start - lineStart) + 1;
        }
    }
    writeChunk(currentChunk(), byte, parser.previous.line, column);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);

    // Calculate distance to jump back to loopStart
    // After emitting OP_LOOP, currentCount has increased by 1
    // We'll emit 2 more bytes for the offset
    // At runtime, after reading the full OP_LOOP instruction (3 bytes), IP will be at count
    // We want IP -= offset to make IP = loopStart
    // So offset = currentCount + 2 - loopStart
    //            (this is the position of IP after reading, minus where we want to go)
    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) error("Loop body terlalu besar.");

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

static int emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}

static void emitReturn() {
    if (current->type == TYPE_INITIALIZER) {
        emitBytes(OP_GET_LOCAL, 0);  // return 'diri'
    } else {
        emitByte(OP_KOSONG);
    }
    emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value) {
    // Protect value from a GC that may be triggered while growing the
    // constants array (writeValueArray allocates). It is only referenced
    // by this local until addConstant stores it.
    push(value);
    int constant = addConstant(currentChunk(), value);
    pop();
    if (constant > UINT8_MAX) {
        error("Terlalu banyak konstanta dalam satu chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void patchJump(int offset) {
    // Defense-in-depth: never write outside the emitted code. A malformed
    // program combined with a compiler bug could otherwise corrupt the heap
    // (found via fuzzing). If the offset is invalid, report and bail.
    if (offset < 0 || offset + 1 >= currentChunk()->count) {
        error("Kesalahan internal: offset lompatan tidak valid.");
        return;
    }

    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = currentChunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Terlalu banyak kode untuk dilompati.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

// Source of the compile() in progress, shared by every function it produces
// so runtime errors can print the offending line. Reset on each compile().
static ObjString* compilingSource = NULL;
// Module every function compiled in this pass belongs to. Like compilingSource
// it is a GC root for the duration of the compile.
static ObjModule* compilingModule = NULL;

static void initCompiler(Compiler* compiler, FunctionType type) {
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->globalCount = 0;
    compiler->nonlocalCount = 0;
    compiler->scopeDepth = 0;
    compiler->tryDepth = 0;
    compiler->finallyCount = 0;
    compiler->finallyInlines = 0;
    compiler->finallyBudgetReported = false;
    compiler->isGenerator = false;
    compiler->function = newFunction();
    compiler->function->chunk.source = compilingSource;
    compiler->function->module = compilingModule;
    current = compiler;
    if (type != TYPE_SCRIPT) {
        if (type == TYPE_FUNCTION && parser.previous.type == TOKEN_FUNGSI) {
            // Anonymous: parser.previous is the `fungsi` keyword itself, and
            // a NULL name already means "script" in tracebacks.
            current->function->name = copyString("<anonim>", 8);
        } else {
            current->function->name = copyString(parser.previous.start,
                                                parser.previous.length);
        }
    }

    Local* local = &current->locals[current->localCount++];
    local->depth = 0;
    local->isCaptured = false;
    if (type == TYPE_CLASSMETHOD) {
        // @metode_kelas gets the class in the receiver slot. Naming it
        // 'kelas_ini' rather than 'diri' keeps the two readable side by side,
        // and matches how `diri` is implicit everywhere else (K3) -- no
        // explicit first parameter like Python's `cls`.
        local->name.start = "kelas_ini";
        local->name.length = 9;
    } else if (type == TYPE_STATIC) {
        // No receiver is pushed, so slot 0 is the callee itself, exactly as
        // for a plain function.
        local->name.start = "";
        local->name.length = 0;
    } else if (type != TYPE_FUNCTION && type != TYPE_SCRIPT) {
        // Methods reserve slot 0 for 'diri' (the receiver / this).
        local->name.start = "diri";
        local->name.length = 4;
    } else {
        local->name.start = "";
        local->name.length = 0;
    }
}

static ObjFunction* endCompiler() {
    emitReturn();
    ObjFunction* function = current->function;
    function->isGenerator = current->isGenerator;

    // A function's own top-level locals never pass through endScope(): the
    // whole Compiler is discarded instead. Report them here or they are
    // invisible to the linter.
    if (lintEnabled) {
        for (int i = 0; i < current->localCount; i++) {
            Local* l = &current->locals[i];
            if (l->lintCheck && !l->used && !l->isCaptured) {
                lintReportUnused(&l->name, l->name.line);
            }
        }
    }

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), function->name != NULL
            ? function->name->chars : "<skrip>");
    }
#endif

    current = current->enclosing;
    return function;
}

static void beginScope() {
    current->scopeDepth++;
}

static void endScope() {
    current->scopeDepth--;

    while (current->localCount > 0 &&
           current->locals[current->localCount - 1].depth >
               current->scopeDepth) {
        Local* dying = &current->locals[current->localCount - 1];
        if (lintEnabled && dying->lintCheck && !dying->used && !dying->isCaptured) {
            lintReportUnused(&dying->name, dying->name.line);
        }
        if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        } else {
            emitByte(OP_POP);
        }
        current->localCount--;
    }
}

// K17: a bare comma builds a tupel ONLY on an assignment right-hand side and
// after `kembalikan`. Everywhere else a comma already means something
// (arguments, list elements, dict pairs), so the flag is set by just those two
// callers and cleared for everything they nest.
static bool allowTupleRHS = false;
static void maybeTupleTail();

static void expression();
static void statement();
static void withStatement();
static void funDeclarationDecorated(int decorators);
static void declaration();
static ParseRule* getRule(TokenType type);
static void function(FunctionType type);
static void parsePrecedence(Precedence precedence);
static void beginLoop(LoopCtx* loop, int continueTarget);
static void endLoop();
static Token syntheticToken(const char* text);
static void namedVariable(Token name, bool canAssign);

// Which collection a comprehension builds. The loop machinery is identical;
// only the accumulator and the per-element append differ.
typedef enum {
    COMP_DAFTAR,      // [x untuk x dalam s]
    COMP_KAMUS,       // {k: v untuk k dalam s}
    COMP_HIMPUNAN,    // {x untuk x dalam s}
    COMP_GENERATOR    // (x untuk x dalam s)
} ComprehensionKind;

// Comprehension support, defined below; argumentList() needs them for the
// bare `f(x untuk x dalam s)` form.
static bool scanComprehensionUntil(TokenType closer, ScannerState* state,
                                   Token* current, Token* previous);
static void comprehensionBody(ComprehensionKind kind, TokenType closer,
                              ScannerState elemState, Token elemCurrent, Token elemPrevious,
                              ScannerState untukState, Token untukCurrent, Token untukPrevious);

static uint8_t identifierConstant(Token* name) {
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqualCString(Token* a, const char* name) {
    size_t len = strlen(name);
    return a->length == (int)len && memcmp(a->start, name, len) == 0;
}

static bool identifiersEqual(Token* a, Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static bool scopeNameDeclared(Token* names, int count, Token* name) {
    for (int i = 0; i < count; i++) {
        if (identifiersEqual(&names[i], name)) return true;
    }
    return false;
}

static int resolveLocal(Compiler* compiler, Token* name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error("Tidak dapat membaca variabel lokal dalam inisialisasinya sendiri.");
            }
            return i;
        }
    }

    return -1;
}

static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error("Terlalu banyak variabel closure dalam fungsi.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

// A captured local is used by definition -- the closure reads it.
static int resolveUpvalue(Compiler* compiler, Token* name) {
    if (compiler->enclosing == NULL) return -1;

    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        compiler->enclosing->locals[local].used = true;
        return addUpvalue(compiler, (uint8_t)local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (uint8_t)upvalue, false);
    }

    return -1;
}

static void addLocal(Token name) {
    if (current->localCount == UINT8_COUNT) {
        error("Terlalu banyak variabel lokal dalam fungsi.");
        return;
    }

    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isCaptured = false;
    local->used = false;
    // Opt-in: only `variabel x` and an implicit `x = ...` are linted. Bindings
    // the language forces on you (parameters, loop vars, `kecuali ... sebagai e`,
    // `dengan ... sebagai`, `super`, synthetics) are not the user's choice to
    // declare and produced only false positives.
    local->lintCheck = false;
}

// Mark the local just added by addLocal() as a real user declaration.
static void markLintable(void) {
    if (current->localCount > 0) {
        current->locals[current->localCount - 1].lintCheck = true;
    }
}

// Add a synthetic, already-initialized local whose name cannot collide with
// a user identifier. Returns its stack slot. Used to desugar 'untuk...dalam'.
static int addSyntheticLocal(const char* text) {
    Token token;
    token.start = text;
    token.length = (int)strlen(text);
    token.type = TOKEN_IDENTIFIER;
    token.line = parser.previous.line;
    addLocal(token);
    current->locals[current->localCount - 1].depth = current->scopeDepth;
    return current->localCount - 1;
}

static void declareVariable() {
    if (current->scopeDepth == 0) return;

    Token* name = &parser.previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error("Sudah ada variabel dengan nama ini dalam scope ini.");
        }
    }

    addLocal(*name);
    markLintable();
}

static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0) return 0;

    return identifierConstant(&parser.previous);
}

static void markInitialized() {
    if (current->scopeDepth == 0) return;
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

// Parses a call's arguments. Positional values are emitted in order; keyword
// arguments (nama=nilai) are emitted after them and their names collected
// into a constant list, matched against the callee's parameters at runtime.
// Returns the total argument count; *nameList is -1 when none were named.
static uint8_t argumentList(int* nameList) {
    uint8_t argCount = 0;
    // The name list is built on the VM stack from the start: collecting names
    // in a local array would leave them unrooted while later arguments
    // allocate, and a GC would free them mid-parse.
    ObjList* names = NULL;
    const char* enclosingArgStart = callArgStart;
    // f(x untuk x dalam s) -- a bare generator expression as the ONLY argument,
    // without its own parentheses (Python allows exactly this). Detected by a
    // 'untuk' at paren depth 0 before the call's ')'.
    if (!check(TOKEN_RIGHT_PAREN)) {
        ScannerState elemState = saveScanner();
        Token elemCurrent = parser.current;
        Token elemPrevious = parser.previous;
        ScannerState untukState;
        Token untukCurrent, untukPrevious;
        if (scanComprehensionUntil(TOKEN_RIGHT_PAREN, &untukState, &untukCurrent, &untukPrevious)) {
            callArgStart = NULL;
            comprehensionBody(COMP_GENERATOR, TOKEN_RIGHT_PAREN,
                              elemState, elemCurrent, elemPrevious,
                              untukState, untukCurrent, untukPrevious);
            callArgStart = enclosingArgStart;
            *nameList = -1;
            return 1;   // comprehensionBody already consumed the ')'
        }
    }
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            // A keyword argument is exactly `identifier = expr`; two tokens of
            // lookahead distinguish it from an expression starting with a name.
            if (check(TOKEN_IDENTIFIER) && checkNext(TOKEN_EQUAL)) {
                Token name = parser.current;
                advance();  // name
                advance();  // '='
                if (names == NULL) {
                    names = newList();
                    push(OBJ_VAL(names));
                }
                ObjString* str = copyString(name.start, name.length);
                push(OBJ_VAL(str));            // root while writeValueArray grows
                writeValueArray(&names->items, OBJ_VAL(str));
                pop();
                callArgStart = NULL;
                expression();
            } else {
                if (names != NULL) {
                    semanticError("Argumen posisional tidak boleh setelah argumen bernama.");
                }
                callArgStart = parser.current.start;
                expression();
            }
            if (argCount == 255) {
                error("Tidak boleh lebih dari 255 argumen.");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }
    callArgStart = enclosingArgStart;
    consume(TOKEN_RIGHT_PAREN, "Harapkan ')' setelah argumen.");

    if (names == NULL) {
        *nameList = -1;
        return argCount;
    }

    *nameList = makeConstant(OBJ_VAL(names));
    pop();   // names
    return argCount;
}

static void and_(bool canAssign) {
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

static void binary(bool canAssign) {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    // '**' is right-associative (2 ** 3 ** 2 == 2 ** 9); the rest are left.
    if (operatorType == TOKEN_POWER) {
        parsePrecedence(PREC_POWER);
    } else {
        parsePrecedence((Precedence)(rule->precedence + 1));
    }

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:    emitByte(OP_NOT_EQUAL); break;
        case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
        case TOKEN_GREATER:       emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitByte(OP_GREATER_EQUAL); break;
        case TOKEN_LESS:          emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL:    emitByte(OP_LESS_EQUAL); break;
        case TOKEN_PLUS:          emitByte(OP_ADD); break;
        case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
        case TOKEN_PERCENT:       emitByte(OP_MODULO); break;
        case TOKEN_POWER:         emitByte(OP_POWER); break;
        case TOKEN_SLASH_SLASH:   emitByte(OP_FLOOR_DIVIDE); break;
        case TOKEN_AMPERSAND:     emitByte(OP_BIT_AND); break;
        case TOKEN_PIPE:          emitByte(OP_BIT_OR); break;
        case TOKEN_CARET:         emitByte(OP_BIT_XOR); break;
        case TOKEN_LESS_LESS:     emitByte(OP_SHIFT_LEFT); break;
        case TOKEN_GREATER_GREATER: emitByte(OP_SHIFT_RIGHT); break;
        default: return; // Unreachable.
    }
}

// Peeks ahead over an argument list (the '(' is already consumed) to see
// whether it contains a keyword argument. Lets `dot`/`super` fall back to
// OP_GET_PROPERTY + OP_CALL_BERNAMA instead of needing keyword-aware
// variants of OP_INVOKE and OP_SUPER_INVOKE.
static bool argsHaveNames() {
    ScannerState saved = saveScanner();
    Token savedCurrent = parser.current;
    Token savedPrevious = parser.previous;
    bool found = false;
    int depth = 0;
    for (;;) {
        TokenType type = parser.current.type;
        if (type == TOKEN_EOF) break;
        if (depth == 0 && type == TOKEN_RIGHT_PAREN) break;
        if (type == TOKEN_LEFT_PAREN || type == TOKEN_LEFT_BRACKET || type == TOKEN_LEFT_BRACE) {
            depth++;
        } else if (type == TOKEN_RIGHT_PAREN || type == TOKEN_RIGHT_BRACKET || type == TOKEN_RIGHT_BRACE) {
            depth--;
        } else if (depth == 0 && type == TOKEN_IDENTIFIER && checkNext(TOKEN_EQUAL)) {
            found = true;
            break;
        }
        advance();
    }
    restoreScanner(saved);
    parser.current = savedCurrent;
    parser.previous = savedPrevious;
    return found;
}

static void call(bool canAssign) {
    int nameList;
    uint8_t argCount = argumentList(&nameList);
    if (nameList < 0) {
        emitBytes(OP_CALL, argCount);
    } else {
        emitBytes(OP_CALL_BERNAMA, argCount);
        emitByte((uint8_t)nameList);
    }
}

static void dot(bool canAssign) {
    consume(TOKEN_IDENTIFIER, "Harapkan nama properti setelah '.'.");
    uint8_t name = identifierConstant(&parser.previous);

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(OP_SET_PROPERTY, name);
    } else if (match(TOKEN_LEFT_PAREN)) {
        if (argsHaveNames()) {
            // Keyword call on a method: load it as a value and use the normal
            // call path, instead of a keyword-aware OP_INVOKE.
            emitBytes(OP_GET_PROPERTY, name);
            int nameList;
            uint8_t argCount = argumentList(&nameList);
            emitBytes(OP_CALL_BERNAMA, argCount);
            emitByte((uint8_t)nameList);
            return;
        }
        int nameList;
        uint8_t argCount = argumentList(&nameList);
        emitBytes(OP_INVOKE, name);
        emitByte(argCount);
    } else {
        emitBytes(OP_GET_PROPERTY, name);
    }
}

static void skipNewlines() {
    while (match(TOKEN_NEWLINE)) { }
}

// Peeks ahead from the current token to decide whether a '[' opens a list
// comprehension: a 'untuk' at bracket depth 0 before the closing ']'. The
// scanner and parser tokens are restored, like scanTryClauses.
static bool scanComprehensionUntil(TokenType closer, ScannerState* state,
                                   Token* current, Token* previous) {
    ScannerState saved = saveScanner();
    Token savedCurrent = parser.current;
    Token savedPrevious = parser.previous;
    bool found = false;
    int depth = 0;
    for (;;) {
        TokenType type = parser.current.type;
        if (type == TOKEN_EOF) break;
        if (depth == 0 && type == closer) break;
        if (depth == 0 && type == TOKEN_UNTUK) {
            *state = saveScanner();
            *current = parser.current;
            *previous = parser.previous;
            found = true;
            break;
        }
        if (type == TOKEN_LEFT_PAREN || type == TOKEN_LEFT_BRACKET || type == TOKEN_LEFT_BRACE) {
            depth++;
        } else if (type == TOKEN_RIGHT_PAREN || type == TOKEN_RIGHT_BRACKET ||
                   type == TOKEN_RIGHT_BRACE) {
            depth--;
        }
        advance();
    }
    restoreScanner(saved);
    parser.current = savedCurrent;
    parser.previous = savedPrevious;
    return found;
}

static bool scanComprehension(ScannerState* state, Token* current, Token* previous) {
    return scanComprehensionUntil(TOKEN_RIGHT_BRACKET, state, current, previous);
}

// Inside '{ ... }', is there a ':' at depth 0 before the comprehension's
// 'untuk'? That distinguishes {k: v untuk ...} (kamus) from {x untuk ...}
// (himpunan). Scanner and parser tokens are restored.
static bool colonBeforeComprehensionFor() {
    ScannerState saved = saveScanner();
    Token savedCurrent = parser.current;
    Token savedPrevious = parser.previous;
    bool colon = false;
    int depth = 0;
    for (;;) {
        TokenType type = parser.current.type;
        if (type == TOKEN_EOF) break;
        if (depth == 0 && type == TOKEN_RIGHT_BRACE) break;
        if (depth == 0 && type == TOKEN_UNTUK) break;
        if (depth == 0 && type == TOKEN_COLON) { colon = true; break; }
        if (type == TOKEN_LEFT_PAREN || type == TOKEN_LEFT_BRACKET || type == TOKEN_LEFT_BRACE) {
            depth++;
        } else if (type == TOKEN_RIGHT_PAREN || type == TOKEN_RIGHT_BRACKET ||
                   type == TOKEN_RIGHT_BRACE) {
            depth--;
        }
        advance();
    }
    restoreScanner(saved);
    parser.current = savedCurrent;
    parser.previous = savedPrevious;
    return colon;
}

// [ekspresi untuk x dalam urutan jika syarat]
//
// Token order fights a single-pass compiler: the element expression comes
// BEFORE the header that declares its loop variable, so compiling it in place
// would resolve 'x' as an undefined global. The element tokens are therefore
// re-read from a saved scanner position AFTER the header has declared 'x' —
// see the layout in the body. (Only one 'untuk' and one optional 'jika' are
// supported; chain conditions with 'dan'/'atau'.)
// After the iterable or the filter only `expected` may follow. Anything else
// used to be skipped silently, so `[x untuk x dalam a jika p jika q]` dropped
// the second filter and returned a wrong list (T-U9).
static void expectComprehensionEnd(TokenType expected, TokenType closer) {
    if (check(expected)) return;
    if (check(TOKEN_UNTUK)) {
        errorAtCurrent("Comprehension hanya mendukung satu 'untuk'.");
    } else if (check(TOKEN_JIKA)) {
        errorAtCurrent("Comprehension hanya mendukung satu 'jika'; "
                       "gabungkan syarat dengan 'dan'.");
    } else if (closer == TOKEN_RIGHT_PAREN) {
        errorAtCurrent("Harapkan ')' di akhir comprehension.");
    } else if (closer == TOKEN_RIGHT_BRACE) {
        errorAtCurrent("Harapkan '}' di akhir comprehension.");
    } else {
        errorAtCurrent("Harapkan ']' di akhir comprehension.");
    }
}

static void comprehensionBody(ComprehensionKind kind, TokenType closer,
                              ScannerState elemState, Token elemCurrent, Token elemPrevious,
                              ScannerState untukState, Token untukCurrent, Token untukPrevious) {
    // Locate the tokens after the header and after the whole comprehension, so
    // the iterable, the condition and the element can each be re-read later.
    ScannerState afterState;
    Token afterCurrent, afterPrevious;
    ScannerState condState = {0};
    Token condCurrent = {0}, condPrevious = {0};
    bool hasCond = false;

    // Header: untuk x dalam ...
    restoreScanner(untukState);
    parser.current = untukCurrent;
    parser.previous = untukPrevious;
    advance();  // 'untuk'
    consume(TOKEN_IDENTIFIER, "Harapkan nama variabel setelah 'untuk'.");
    Token varName = parser.previous;
    consume(TOKEN_DALAM, "Harapkan 'dalam' setelah nama variabel.");

    // The iterable itself is evaluated in the ENCLOSING frame, so skip its
    // tokens for now and come back to them after the body has been compiled.
    ScannerState seqState = saveScanner();
    Token seqCurrent = parser.current;
    Token seqPrevious = parser.previous;

    int depth = 0;
    for (;;) {
        TokenType type = parser.current.type;
        if (type == TOKEN_EOF) break;
        if (depth == 0 && type == closer) {
            advance();  // consume the closing bracket/brace/paren
            break;
        }
        if (depth == 0 && type == TOKEN_JIKA && !hasCond) {
            hasCond = true;
            condState = saveScanner();
            condCurrent = parser.current;
            condPrevious = parser.previous;
        }
        if (type == TOKEN_LEFT_PAREN || type == TOKEN_LEFT_BRACKET || type == TOKEN_LEFT_BRACE) {
            depth++;
        } else if (type == TOKEN_RIGHT_PAREN || type == TOKEN_RIGHT_BRACKET ||
                   type == TOKEN_RIGHT_BRACE) {
            depth--;
        }
        advance();
    }
    afterState = saveScanner();
    afterCurrent = parser.current;
    afterPrevious = parser.previous;

    // The per-element work is compiled into an implicit one-parameter function
    // called once per element. Its own frame keeps the hidden slots away from
    // the enclosing frame's expression temporaries (the callee and earlier
    // arguments of an enclosing call sit below them), and enclosing names are
    // still visible through the normal upvalue machinery.
    Compiler compFn;
    initCompiler(&compFn, TYPE_FUNCTION);
    const char* compName =
        kind == COMP_KAMUS ? "kamuscomp" :
        kind == COMP_HIMPUNAN ? "himpunancomp" :
        kind == COMP_GENERATOR ? "gencomp" : "listcomp";
    current->function->name = copyString(compName, (int)strlen(compName));
    if (kind == COMP_GENERATOR) current->isGenerator = true;
    current->function->arity = 1;
    current->function->requiredArity = 1;
    beginScope();
    LoopCtx* savedLoop = currentLoop;
    currentLoop = NULL;      // never let 'hentikan'/'lanjut' target the outer loop

    addLocal(syntheticToken("  comp seq"));   // slot 1: the iterable
    markInitialized();
    emitByte(OP_KOSONG);
    addLocal(varName);                        // slot 2: the loop variable
    markInitialized();
    int varSlot = current->localCount - 1;

    int resSlot = -1;
    if (kind == COMP_DAFTAR) {
        emitBytes(OP_BUILD_LIST, 0);
        resSlot = addSyntheticLocal("  comp hasil");
    } else if (kind == COMP_KAMUS) {
        emitBytes(OP_BUILD_DICT, 0);
        resSlot = addSyntheticLocal("  comp hasil");
    } else if (kind == COMP_HIMPUNAN) {
        emitByte(OP_BUILD_SET);
        resSlot = addSyntheticLocal("  comp hasil");
    }
    // COMP_GENERATOR accumulates nothing: each element is yielded.

    emitBytes(OP_GET_LOCAL, 1);
    emitByte(OP_LIST_LEN);
    int lenSlot = addSyntheticLocal("  comp len");

    emitConstant(INT_VAL(-1));
    int idxSlot = addSyntheticLocal("  comp idx");

    int incrementStart = currentChunk()->count;

    // idx = idx + 1
    emitBytes(OP_GET_LOCAL, (uint8_t)idxSlot);
    emitConstant(INT_VAL(1));
    emitByte(OP_ADD);
    emitBytes(OP_SET_LOCAL, (uint8_t)idxSlot);
    emitByte(OP_POP);

    // if !(idx < len) goto exit
    emitBytes(OP_GET_LOCAL, (uint8_t)idxSlot);
    emitBytes(OP_GET_LOCAL, (uint8_t)lenSlot);
    emitByte(OP_LESS);
    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);

    // x = seq[idx]  (ITER_NEXT: generator/custom iterator first, otherwise the
    // same positional read as before -- a kamus yields keys by position).
    // A lazy source has no length, so it ends by raising HentiIterasi; catch
    // exactly that here and let every other error propagate, as forInStatement
    // does.
    int compHandler = emitJump(OP_TRY_BEGIN);
    current->tryDepth++;
    emitBytes(OP_GET_LOCAL, 1);
    emitBytes(OP_GET_LOCAL, (uint8_t)idxSlot);
    emitByte(OP_ITER_NEXT);
    emitBytes(OP_SET_LOCAL, (uint8_t)varSlot);
    emitByte(OP_POP);
    emitByte(OP_TRY_END);
    current->tryDepth--;
    int compNormal = emitJump(OP_JUMP);

    patchJump(compHandler);
    namedVariable(syntheticToken("HentiIterasi"), false);
    emitByte(OP_IS_INSTANCE);
    int compNotExhausted = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);             // matching boolean
    emitByte(OP_POP);             // HentiIterasi instance
    int compExit = emitJump(OP_JUMP);
    patchJump(compNotExhausted);
    emitByte(OP_POP);             // non-matching boolean
    emitByte(OP_THROW);           // rethrow the original error
    patchJump(compNormal);

    // Optional 'jika syarat', evaluated inside the loop.
    int skipJump = -1;
    if (hasCond) {
        restoreScanner(condState);
        parser.current = condCurrent;
        parser.previous = condPrevious;
        advance();  // 'jika'
        // Python's comp_if is an or_test: no conditional expression here,
        // so a 'jika' inside the condition is never mistaken for one.
        parsePrecedence(PREC_OR);
        expectComprehensionEnd(closer, closer);
        skipJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
    }

    // Element expression, re-read from just after '['.
    restoreScanner(elemState);
    parser.current = elemCurrent;
    parser.previous = elemPrevious;
    if (kind == COMP_GENERATOR) {
        // Each element is yielded straight to the consumer; nothing accumulates.
        expression();
        emitByte(OP_YIELD);        // pops the value; resumes pushing nothing
    } else if (kind == COMP_KAMUS) {
        emitBytes(OP_GET_LOCAL, (uint8_t)resSlot);
        expression();                                   // kunci
        consume(TOKEN_COLON, "Harapkan ':' antara kunci dan nilai pada kamus comprehension.");
        expression();                                   // nilai
        emitByte(OP_DICT_PUT);
    } else if (kind == COMP_HIMPUNAN) {
        emitBytes(OP_GET_LOCAL, (uint8_t)resSlot);
        expression();
        emitByte(OP_SET_PUT);
    } else {
        emitBytes(OP_GET_LOCAL, (uint8_t)resSlot);
        expression();
        emitByte(OP_APPEND);
    }
    if (!check(TOKEN_UNTUK)) {
        error("Harapkan 'untuk' setelah ekspresi pada comprehension.");
    }

    if (skipJump != -1) {
        // The true path must skip the POP that belongs to the false path.
        int endJump = emitJump(OP_JUMP);
        patchJump(skipJump);
        emitByte(OP_POP);
        patchJump(endJump);
    }

    emitLoop(incrementStart);
    patchJump(exitJump);
    emitByte(OP_POP);
    patchJump(compExit);

    // Return the accumulator. endCompiler() appends a dead 'return kosong'.
    // A generator returns nothing: falling off the end ends the iteration.
    if (kind != COMP_GENERATOR) {
        emitBytes(OP_GET_LOCAL, (uint8_t)resSlot);
        emitByte(OP_RETURN);
    }

    currentLoop = savedLoop;
    ObjFunction* comprehensionFn = endCompiler();

    // Back in the enclosing frame: closure, iterable, call.
    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(comprehensionFn)));
    for (int i = 0; i < comprehensionFn->upvalueCount; i++) {
        emitByte(compFn.upvalues[i].isLocal ? 1 : 0);
        emitByte(compFn.upvalues[i].index);
    }

    restoreScanner(seqState);
    parser.current = seqCurrent;
    parser.previous = seqPrevious;
    // The iterable is an or_test, as in Python: in [x untuk x dalam a jika c]
    // the 'jika' starts the filter, not a conditional expression 'a jika c
    // selain ...'. Wrap it in parentheses to use a conditional expression.
    parsePrecedence(PREC_OR);
    expectComprehensionEnd(hasCond ? TOKEN_JIKA : closer, closer);
    emitBytes(OP_CALL, 1);

    restoreScanner(afterState);
    parser.current = afterCurrent;
    parser.previous = afterPrevious;
}

static void listLiteral(bool canAssign) {
    (void)canAssign;
    // '[' already consumed. A 'untuk' at depth 0 makes this a comprehension.
    if (!check(TOKEN_RIGHT_BRACKET)) {
        ScannerState elemState = saveScanner();
        Token elemCurrent = parser.current;
        Token elemPrevious = parser.previous;
        ScannerState untukState;
        Token untukCurrent, untukPrevious;
        if (scanComprehension(&untukState, &untukCurrent, &untukPrevious)) {
            comprehensionBody(COMP_DAFTAR, TOKEN_RIGHT_BRACKET,
                              elemState, elemCurrent, elemPrevious,
                              untukState, untukCurrent, untukPrevious);
            return;
        }
    }

    // Plain literal: comma-separated elements.
    int count = 0;
    if (!check(TOKEN_RIGHT_BRACKET)) {
        do {
            // Allow trailing newlines inside list literals.
            if (check(TOKEN_RIGHT_BRACKET)) break;
            expression();
            if (count == 255) {
                error("Tidak boleh lebih dari 255 elemen dalam literal list.");
            }
            count++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_BRACKET, "Harapkan ']' setelah elemen list.");
    emitBytes(OP_BUILD_LIST, (uint8_t)count);
}

// '{' in EXPRESSION position is a dict literal; in statement position
// statement() consumed it as a block first (see AGENTS.md: PHP-style braces).
// '{' does not raise the scanner's groupingDepth, so skip newlines by hand to
// allow multi-line literals.
static void dictLiteral(bool canAssign) {
    (void)canAssign;
    int count = 0;
    skipNewlines();
    // {kunci: nilai untuk ...} = kamus comprehension
    // {nilai untuk ...}        = himpunan comprehension
    // A 'untuk' at brace depth 0 before '}' selects a comprehension; a ':' at
    // depth 0 before that 'untuk' selects the kamus form.
    if (!check(TOKEN_RIGHT_BRACE)) {
        ScannerState elemState = saveScanner();
        Token elemCurrent = parser.current;
        Token elemPrevious = parser.previous;
        ScannerState untukState;
        Token untukCurrent, untukPrevious;
        if (scanComprehensionUntil(TOKEN_RIGHT_BRACE, &untukState, &untukCurrent, &untukPrevious)) {
            bool isKamus = colonBeforeComprehensionFor();
            comprehensionBody(isKamus ? COMP_KAMUS : COMP_HIMPUNAN, TOKEN_RIGHT_BRACE,
                              elemState, elemCurrent, elemPrevious,
                              untukState, untukCurrent, untukPrevious);
            return;
        }
    }
    if (!check(TOKEN_RIGHT_BRACE)) {
        do {
            skipNewlines();
            if (check(TOKEN_RIGHT_BRACE)) break;   // trailing comma
            expression();                          // key
            consume(TOKEN_COLON, "Harapkan ':' setelah kunci kamus.");
            skipNewlines();
            expression();                          // value
            if (count == 255) {
                error("Tidak boleh lebih dari 255 pasangan dalam literal kamus.");
            }
            count++;
            skipNewlines();
        } while (match(TOKEN_COMMA));
    }
    skipNewlines();
    consume(TOKEN_RIGHT_BRACE, "Harapkan '}' setelah isi literal kamus.");
    emitBytes(OP_BUILD_DICT, (uint8_t)count);
}

static void subscript(bool canAssign) {
    // The target is already on the stack. '[' already consumed.
    // Either a[i], a[i] = v, or a slice a[awal:akhir:langkah] where every part
    // may be omitted. Omitted parts are emitted as 'kosong' so the VM can tell
    // "not given" from a real value.
    bool isSlice = false;

    if (check(TOKEN_COLON)) {
        emitByte(OP_KOSONG);     // start omitted
        isSlice = true;
    } else {
        expression();
    }

    if (match(TOKEN_COLON)) {
        isSlice = true;
        if (check(TOKEN_RIGHT_BRACKET) || check(TOKEN_COLON)) {
            emitByte(OP_KOSONG); // stop omitted
        } else {
            expression();
        }
        if (match(TOKEN_COLON)) {
            if (check(TOKEN_RIGHT_BRACKET)) {
                emitByte(OP_KOSONG);  // step omitted
            } else {
                expression();
            }
        } else {
            emitByte(OP_KOSONG);      // no step given
        }
    }

    consume(TOKEN_RIGHT_BRACKET, "Harapkan ']' setelah indeks.");

    if (isSlice) {
        if (canAssign && match(TOKEN_EQUAL)) {
            semanticError("Irisan tidak dapat menjadi sasaran assignment.");
            expression();   // consume the right side to avoid a cascade
        }
        emitByte(OP_SLICE);
        return;
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        maybeTupleTail();           // d["x"] = 1, 2
        emitByte(OP_INDEX_SET);
    } else {
        emitByte(OP_INDEX_GET);
    }
}

static void literal(bool canAssign) {
    switch (parser.previous.type) {
        case TOKEN_SALAH: emitByte(OP_SALAH); break;
        case TOKEN_KOSONG: emitByte(OP_KOSONG); break;
        case TOKEN_BENAR: emitByte(OP_BENAR); break;
        default: return; // Unreachable.
    }
}

// '(' already consumed. (x) is plain grouping; a comma after the first
// expression makes a tupel (K17): (a, b), (a,), (a, b,). () is the empty
// tupel. Newlines inside are free: '(' raises the scanner's grouping depth.
// Where the last non-empty tupel literal `( ... , ... )` began (its '(') and
// where its bytecode ended. assertStatement uses it to reject
// `pastikan(kondisi, pesan)`, whose condition is an always-true tupel.
static const char* lastTupleStart = NULL;
static int lastTupleEnd = -1;

static void grouping(bool canAssign) {
    (void)canAssign;
    const char* openParen = parser.previous.start;
    if (match(TOKEN_RIGHT_PAREN)) {
        emitBytes(OP_BUILD_TUPLE, 0);
        return;
    }
    // (ekspresi untuk x dalam urutan jika syarat) -- generator expression.
    // Only a 'untuk' at paren depth 0 before the ')' makes it one, so plain
    // grouping and tupel literals keep their exact previous behavior.
    {
        ScannerState elemState = saveScanner();
        Token elemCurrent = parser.current;
        Token elemPrevious = parser.previous;
        ScannerState untukState;
        Token untukCurrent, untukPrevious;
        if (scanComprehensionUntil(TOKEN_RIGHT_PAREN, &untukState, &untukCurrent, &untukPrevious)) {
            comprehensionBody(COMP_GENERATOR, TOKEN_RIGHT_PAREN,
                              elemState, elemCurrent, elemPrevious,
                              untukState, untukCurrent, untukPrevious);
            return;
        }
    }
    expression();
    if (!match(TOKEN_COMMA)) {
        consume(TOKEN_RIGHT_PAREN, "Harapkan ')' setelah ekspresi.");
        return;
    }
    int count = 1;
    while (!check(TOKEN_RIGHT_PAREN) && !check(TOKEN_EOF)) {
        expression();
        if (count == 255) {
            error("Tidak boleh lebih dari 255 elemen dalam literal tupel.");
        }
        count++;
        if (!match(TOKEN_COMMA)) break;
    }
    consume(TOKEN_RIGHT_PAREN, "Harapkan ')' setelah elemen tupel.");
    emitBytes(OP_BUILD_TUPLE, (uint8_t)count);
    lastTupleStart = openParen;
    lastTupleEnd = currentChunk()->count;
}

// Numeric literal. Handles `_` separators and 0b/0o/0x bases (Fase 1) and
// the K2 split: a literal without '.' or exponent — including every 0b/0o/0x
// literal — is a bulat (int64); anything else is a desimal (double). A bulat
// too large for int64 is a compile error rather than a silent rounding —
// bignums come later.
static void number(bool canAssign) {
    (void)canAssign;
    const char* src = parser.previous.start;
    int len = parser.previous.length;

    // Strip separators into a NUL-terminated buffer. parser.previous.start
    // points into the source, so length must come from the token, not strlen.
    char buf[64];
    if (len >= (int)sizeof(buf)) {
        error("Angka terlalu panjang.");
        emitConstant(NUMBER_VAL(0));
        return;
    }
    int n = 0;
    for (int i = 0; i < len; i++) {
        char c = src[i];
        if (c == '_') {
            // Separators must sit between digits: 1_000 yes, 1__0 / 1_ no.
            bool okBefore = i > 0 && src[i - 1] != '_' && src[i - 1] != '.';
            bool okAfter = i + 1 < len && src[i + 1] != '_';
            if (!okBefore || !okAfter) {
                error("Garis bawah hanya boleh di antara digit.");
                emitConstant(NUMBER_VAL(0));
                return;
            }
            continue;
        }
        buf[n++] = c;
    }
    buf[n] = '\0';

    int base = 10;
    const char* digits = buf;
    if (n > 2 && buf[0] == '0') {
        char prefix = buf[1];
        if (prefix == 'b' || prefix == 'B') base = 2;
        if (prefix == 'o' || prefix == 'O') base = 8;
        if (prefix == 'x' || prefix == 'X') base = 16;
        if (base != 10) digits = buf + 2;
    }
    bool isDecimal = false;
    if (base == 10) {
        for (int i = 0; i < n; i++) {
            if (buf[i] == '.' || buf[i] == 'e' || buf[i] == 'E') {
                isDecimal = true;
                break;
            }
        }
    }

    if (isDecimal) {
        char* end;
        double value = strtod(buf, &end);
        if (*end != '\0') {
            error("Angka tidak sah.");
            emitConstant(NUMBER_VAL(0));
            return;
        }
        emitConstant(NUMBER_VAL(value));
        return;
    }

    char* end;
    errno = 0;
    long long value = strtoll(digits, &end, base);
    if (*end != '\0' || end == digits) {
        error(base == 2  ? "Digit biner tidak sah." :
              base == 8  ? "Digit oktal tidak sah." :
              base == 16 ? "Digit heksadesimal tidak sah." :
                           "Angka tidak sah.");
        emitConstant(NUMBER_VAL(0));
        return;
    }
    if (errno == ERANGE) {
        error("Angka bulat melebihi jangkauan "
              "-9223372036854775808..9223372036854775807.");
        emitConstant(NUMBER_VAL(0));
        return;
    }
    emitConstant(INT_VAL(value));
}

// fungsi(x) { ... } used as an expression. function() already emits OP_CLOSURE
// and its upvalue list, so the closure simply lands on the stack.
static void anonymousFunction(bool canAssign) {
    (void)canAssign;
    function(TYPE_FUNCTION);
}

static void or_(bool canAssign) {
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

// f"halo {nama}" desugars into "halo " + ke_teks(nama), reusing OP_ADD and the
// existing ke_teks native — no new opcode. Literal chunks are emitted as
// constants; each {...} is compiled by pointing the scanner at that slice of
// source, the same rewind trick used by default parameter values.
static void fstring(bool canAssign) {
    (void)canAssign;
    Token token = parser.previous;
    // The token starts at the `f` prefix: f"..." -> skip 'f' and the quote.
    const char* src = token.start + 2;
    int rawLen = token.length - 3;

    ScannerState afterToken = saveScanner();
    Token afterTokenCurrent = parser.current;

    int parts = 0;
    char* buf = ALLOCATE(char, rawLen + 1);
    int len = 0;

    for (int i = 0; i < rawLen; i++) {
        char c = src[i];

        if (c == '{' && i + 1 < rawLen && src[i + 1] == '{') {
            buf[len++] = '{'; i++; continue;
        }
        if (c == '}' && i + 1 < rawLen && src[i + 1] == '}') {
            buf[len++] = '}'; i++; continue;
        }
        if (c == '}') {
            FREE_ARRAY(char, buf, rawLen + 1);
            semanticError("'}' tanpa '{' di dalam f-string.");
            emitConstant(OBJ_VAL(copyString("", 0)));
            return;
        }
        if (c != '{') {
            if (c == '\\' && i + 1 < rawLen) {
                char next = src[++i];
                switch (next) {
                    case 'n':  buf[len++] = '\n'; break;
                    case 't':  buf[len++] = '\t'; break;
                    case 'r':  buf[len++] = '\r'; break;
                    case '\\': buf[len++] = '\\'; break;
                    case '"':  buf[len++] = '"';  break;
                    case '\'': buf[len++] = '\''; break;
                    case '0':  buf[len++] = '\0'; break;
                    default:   buf[len++] = '\\'; buf[len++] = next; break;
                }
            } else {
                buf[len++] = c;
            }
            continue;
        }

        // Flush the literal collected so far.
        if (len > 0) {
            emitConstant(OBJ_VAL(copyString(buf, len)));
            len = 0;
            parts++;
            if (parts >= 2) emitByte(OP_ADD);
        }

        // Find the matching '}', honouring nesting and strings.
        int start = i + 1;
        int depth = 1;
        int j = start;
        char quote = '\0';
        for (; j < rawLen; j++) {
            char e = src[j];
            if (quote != '\0') {
                if (e == '\\') { j++; continue; }
                if (e == quote) quote = '\0';
                continue;
            }
            if (e == '"' || e == '\'') { quote = e; continue; }
            if (e == '{') depth++;
            else if (e == '}' && --depth == 0) break;
        }
        if (j >= rawLen) {
            FREE_ARRAY(char, buf, rawLen + 1);
            semanticError("'{' tidak ditutup di dalam f-string.");
            emitConstant(OBJ_VAL(copyString("", 0)));
            return;
        }
        if (j == start) {
            FREE_ARRAY(char, buf, rawLen + 1);
            semanticError("Ekspresi kosong di dalam f-string.");
            emitConstant(OBJ_VAL(copyString("", 0)));
            return;
        }

        // ke_teks(<expr>) — look the native up by name, then call it.
        Token keTeks = {TOKEN_IDENTIFIER, "ke_teks", 7, token.line};
        emitBytes(OP_GET_GLOBAL, identifierConstant(&keTeks));

        // The slice is not NUL-terminated, so compile from a copy. Tokens
        // point into it only while expression() runs.
        int exprLen = j - start;
        char* expr = ALLOCATE(char, exprLen + 1);
        memcpy(expr, src + start, exprLen);
        expr[exprLen] = '\0';

        // Column of the f-string token in the *real* source, computed while
        // that buffer is still the one being scanned.
        const char* tokenLineStart = lineStartOf(&token);
        int savedOverride = columnOverride;
        if (columnOverride == 0 && tokenLineStart != NULL) {
            columnOverride = (int)(token.start - tokenLineStart) + 1;
        }

        initScannerAt(expr, token.line);
        advance();                                // prime parser.current
        expression();
        if (!check(TOKEN_EOF)) {
            semanticError("Ekspresi f-string tidak lengkap.");
        }
        columnOverride = savedOverride;
        restoreScanner(afterToken);
        parser.current = afterTokenCurrent;
        // parser.previous still points into `expr`, which is about to be
        // freed; anything reading it (emitByte's column lookup) would touch
        // freed memory. Point it back at the f-string token itself, which is
        // also the right place to blame for the bytecode emitted below.
        parser.previous = token;
        FREE_ARRAY(char, expr, exprLen + 1);

        emitBytes(OP_CALL, 1);

        parts++;
        if (parts >= 2) emitByte(OP_ADD);
        i = j;
    }

    if (len > 0 || parts == 0) {
        emitConstant(OBJ_VAL(copyString(buf, len)));
        parts++;
        if (parts >= 2) emitByte(OP_ADD);
    }
    FREE_ARRAY(char, buf, rawLen + 1);
}

static void string(bool canAssign) {
    // The token includes the surrounding quotes. Process escape sequences
    // (\n \t \r \\ \" \' \0) into a fresh buffer, then intern it.
    // Triple-quoted literals carry three quotes on each side.
    int quotes = (parser.previous.length >= 6 &&
                  strncmp(parser.previous.start, "\"\"\"", 3) == 0) ? 3 : 1;
    const char* src = parser.previous.start + quotes;
    int rawLen = parser.previous.length - 2 * quotes;

    char* chars = ALLOCATE(char, rawLen + 1);
    int len = 0;
    for (int i = 0; i < rawLen; i++) {
        char c = src[i];
        if (c == '\\' && i + 1 < rawLen) {
            char next = src[++i];
            switch (next) {
                case 'n':  chars[len++] = '\n'; break;
                case 't':  chars[len++] = '\t'; break;
                case 'r':  chars[len++] = '\r'; break;
                case '\\': chars[len++] = '\\'; break;
                case '"':  chars[len++] = '"';  break;
                case '\'': chars[len++] = '\''; break;
                case '0':  chars[len++] = '\0'; break;
                default:
                    // Unknown escape: keep the backslash and the character.
                    chars[len++] = '\\';
                    chars[len++] = next;
                    break;
            }
        } else {
            chars[len++] = c;
        }
    }
    chars[len] = '\0';

    ObjString* str = takeString(chars, len);
    emitConstant(OBJ_VAL(str));
}

// If the current token is a compound assignment (+=, -=, ...), consume it and
// store the arithmetic opcode it stands for.
static bool isCompoundAssign(TokenType type) {
    switch (type) {
        case TOKEN_PLUS_EQUAL:
        case TOKEN_MINUS_EQUAL:
        case TOKEN_STAR_EQUAL:
        case TOKEN_SLASH_EQUAL:
        case TOKEN_SLASH_SLASH_EQUAL:
        case TOKEN_PERCENT_EQUAL:
        case TOKEN_POWER_EQUAL:
        case TOKEN_AMPERSAND_EQUAL:
        case TOKEN_PIPE_EQUAL:
        case TOKEN_CARET_EQUAL:
        case TOKEN_LESS_LESS_EQUAL:
        case TOKEN_GREATER_GREATER_EQUAL:
            return true;
        default:
            return false;
    }
}

static bool matchCompoundAssign(uint8_t* op) {
    static const struct { TokenType token; uint8_t op; } COMPOUND_OPS[] = {
        {TOKEN_PLUS_EQUAL,        OP_ADD},
        {TOKEN_MINUS_EQUAL,       OP_SUBTRACT},
        {TOKEN_STAR_EQUAL,        OP_MULTIPLY},
        {TOKEN_SLASH_EQUAL,       OP_DIVIDE},
        {TOKEN_SLASH_SLASH_EQUAL, OP_FLOOR_DIVIDE},
        {TOKEN_PERCENT_EQUAL,     OP_MODULO},
        {TOKEN_POWER_EQUAL,       OP_POWER},
        {TOKEN_AMPERSAND_EQUAL,   OP_BIT_AND},
        {TOKEN_PIPE_EQUAL,        OP_BIT_OR},
        {TOKEN_CARET_EQUAL,       OP_BIT_XOR},
        {TOKEN_LESS_LESS_EQUAL,   OP_SHIFT_LEFT},
        {TOKEN_GREATER_GREATER_EQUAL, OP_SHIFT_RIGHT},
    };
    for (size_t i = 0; i < sizeof(COMPOUND_OPS) / sizeof(COMPOUND_OPS[0]); i++) {
        if (match(COMPOUND_OPS[i].token)) {
            *op = COMPOUND_OPS[i].op;
            return true;
        }
    }
    return false;
}

static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp, compoundOp;
    bool forceGlobal = scopeNameDeclared(current->globals, current->globalCount, &name);
    bool forceNonlocal = scopeNameDeclared(current->nonlocals, current->nonlocalCount, &name);
    int arg = -1;

    if (!forceGlobal) arg = resolveLocal(current, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if (!forceGlobal && (arg = resolveUpvalue(current, &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else if (forceNonlocal) {
        semanticError("'nonlokal' harus merujuk variabel pada fungsi pembungkus.");
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    // A plain assignment is NOT a use; a compound assignment and a bare
    // mention both read the variable first.
    bool reads = true;
    if (canAssign && check(TOKEN_EQUAL)) reads = false;
    if (reads && getOp == OP_GET_LOCAL && arg >= 0) current->locals[arg].used = true;

    if (canAssign && match(TOKEN_EQUAL)) {
        // expression(), not parsePrecedence(): the right side may be a
        // conditional `a jika c selain b`, which only expression() parses.
        // With parsePrecedence the statement became `(x = a) jika c selain b`
        // and a false condition skipped the assignment (T-U2).
        expression();
        maybeTupleTail();           // x = 1, 2
        if (setOp == OP_SET_GLOBAL) {
            emitBytes(OP_DEFINE_GLOBAL, (uint8_t)arg);
            emitBytes(OP_GET_GLOBAL, (uint8_t)arg);
        } else {
            emitBytes(setOp, (uint8_t)arg);
        }
    } else if (canAssign && matchCompoundAssign(&compoundOp)) {
        emitBytes(getOp, (uint8_t)arg);
        expression();
        emitByte(compoundOp);
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }
}

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static Token syntheticToken(const char* text) {
    Token token;
    token.start = text;
    token.length = (int)strlen(text);
    return token;
}

static void diri_(bool canAssign) {
    if (currentClass == NULL) {
        error("Tidak dapat menggunakan 'diri' di luar kelas.");
        return;
    }
    // Slot 0 holds the class (@metode_kelas) or the callee (@metode_statis)
    // in these two, so resolving 'diri' here would silently hand back the
    // wrong object instead of a receiver.
    if (current->type == TYPE_STATIC) {
        error("'diri' tidak tersedia di @metode_statis: metode statis tidak "
              "menerima objek.");
        return;
    }
    if (current->type == TYPE_CLASSMETHOD) {
        error("'diri' tidak tersedia di @metode_kelas: pakai 'kelas_ini' "
              "untuk kelasnya.");
        return;
    }
    variable(false);
}

static void super_(bool canAssign) {
    if (currentClass == NULL) {
        error("Tidak dapat menggunakan 'super' di luar kelas.");
    } else if (!currentClass->hasSuperclass) {
        error("Tidak dapat menggunakan 'super' pada kelas tanpa induk.");
    }

    consume(TOKEN_DOT, "Harapkan '.' setelah 'super'.");
    consume(TOKEN_IDENTIFIER, "Harapkan nama metode induk.");
    uint8_t name = identifierConstant(&parser.previous);

    namedVariable(syntheticToken("diri"), false);
    if (match(TOKEN_LEFT_PAREN)) {
        if (argsHaveNames()) {
            // Same fallback as `dot`: fetch the bound method, then call it.
            namedVariable(syntheticToken("super"), false);
            emitBytes(OP_GET_SUPER, name);
            int nameList;
            uint8_t argCount = argumentList(&nameList);
            emitBytes(OP_CALL_BERNAMA, argCount);
            emitByte((uint8_t)nameList);
            return;
        }
        int nameList;
        uint8_t argCount = argumentList(&nameList);
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_SUPER_INVOKE, name);
        emitByte(argCount);
    } else {
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_GET_SUPER, name);
    }
}

static void unary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    // Compile the operand. 'tidak' binds looser than comparisons, like
    // Python's 'not': tidak a == b means tidak (a == b).
    parsePrecedence(operatorType == TOKEN_TIDAK ? PREC_NOT : PREC_UNARY);

    // Emit the operator instruction.
    switch (operatorType) {
        case TOKEN_BANG:
        case TOKEN_TIDAK: emitByte(OP_NOT); break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        case TOKEN_TILDE: emitByte(OP_BIT_NOT); break;
        default: return; // Unreachable.
    }
}

// Comparison operators share Python's chaining semantics: a < b < c keeps b
// on the stack, evaluates it once, and short-circuits before evaluating c.
static bool isComparisonStart(TokenType type) {
    return type == TOKEN_EQUAL_EQUAL || type == TOKEN_BANG_EQUAL ||
           type == TOKEN_GREATER || type == TOKEN_GREATER_EQUAL ||
           type == TOKEN_LESS || type == TOKEN_LESS_EQUAL ||
           type == TOKEN_DALAM || type == TOKEN_TIDAK ||
           type == TOKEN_ADALAH || type == TOKEN_BUKAN;
}

static ChainCompare comparisonKind(TokenType type) {
    switch (type) {
        case TOKEN_EQUAL_EQUAL: return CHAIN_EQUAL;
        case TOKEN_BANG_EQUAL: return CHAIN_NOT_EQUAL;
        case TOKEN_GREATER: return CHAIN_GREATER;
        case TOKEN_GREATER_EQUAL: return CHAIN_GREATER_EQUAL;
        case TOKEN_LESS: return CHAIN_LESS;
        case TOKEN_LESS_EQUAL: return CHAIN_LESS_EQUAL;
        case TOKEN_DALAM: return CHAIN_IN;
        case TOKEN_TIDAK:
            consume(TOKEN_DALAM, "Harapkan 'dalam' setelah 'tidak' (operator 'tidak dalam').");
            return CHAIN_NOT_IN;
        case TOKEN_ADALAH: return CHAIN_IDENTITY;
        case TOKEN_BUKAN:
            consume(TOKEN_ADALAH, "Harapkan 'adalah' setelah 'bukan' (operator 'bukan adalah').");
            return CHAIN_NOT_IDENTITY;
        default: fatalError("Operator perbandingan tidak valid.");
    }
    return CHAIN_EQUAL;
}

static void emitComparison(ChainCompare op) {
    switch (op) {
        case CHAIN_EQUAL: emitByte(OP_EQUAL); break;
        case CHAIN_IDENTITY: emitByte(OP_IDENTICAL); break;
        case CHAIN_NOT_EQUAL: emitByte(OP_NOT_EQUAL); break;
        case CHAIN_NOT_IDENTITY: emitBytes(OP_IDENTICAL, OP_NOT); break;
        case CHAIN_GREATER: emitByte(OP_GREATER); break;
        case CHAIN_GREATER_EQUAL: emitByte(OP_GREATER_EQUAL); break;
        case CHAIN_LESS: emitByte(OP_LESS); break;
        case CHAIN_LESS_EQUAL: emitByte(OP_LESS_EQUAL); break;
        case CHAIN_IN: emitByte(OP_IN); break;
        case CHAIN_NOT_IN: emitBytes(OP_IN, OP_NOT); break;
    }
}

static void comparison(bool canAssign) {
    ChainCompare op = comparisonKind(parser.previous.type);
    parsePrecedence((Precedence)(PREC_COMPARISON + 1));
    if (!isComparisonStart(parser.current.type)) {
        emitComparison(op);
        return;
    }

    int falseJumps[UINT8_COUNT];
    int falseCount = 0;
    for (;;) {
        emitBytes(OP_CHAIN_COMPARE, op);
        falseJumps[falseCount++] = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);

        advance();
        op = comparisonKind(parser.previous.type);
        parsePrecedence((Precedence)(PREC_COMPARISON + 1));
        if (!isComparisonStart(parser.current.type)) {
            emitComparison(op);
            break;
        }
    }

    int endJump = emitJump(OP_JUMP);
    for (int i = 0; i < falseCount; i++) patchJump(falseJumps[i]);
    emitByte(OP_CHAIN_CLEANUP);
    patchJump(endJump);
}

ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, call,   PREC_CALL},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {dictLiteral, NULL,  PREC_NONE},
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACKET]  = {listLiteral, subscript, PREC_CALL},
  [TOKEN_RIGHT_BRACKET] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     dot,    PREC_CALL},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  // '->' tidak pernah muncul sebagai ekspresi; hanya dikonsumsi oleh
  // parseAnotasiTipe(). Entri eksplisit supaya pesan galatnya jelas
  // ("Harapkan ekspresi") alih-alih bergantung pada nilai nol implisit.
  [TOKEN_PANAH]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_COLON]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NEWLINE]       = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_PERCENT]       = {NULL,     binary, PREC_FACTOR},
  [TOKEN_POWER]         = {NULL,     binary, PREC_POWER},
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     comparison, PREC_COMPARISON},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     comparison, PREC_COMPARISON},
  [TOKEN_GREATER]       = {NULL,     comparison, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     comparison, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     comparison, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     comparison, PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_FSTRING]       = {fstring,  NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_DAN]           = {NULL,     and_,   PREC_AND},
  [TOKEN_KELAS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SELAIN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SALAH]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_UNTUK]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUNGSI]        = {anonymousFunction, NULL, PREC_NONE},
  [TOKEN_JIKA]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_KOSONG]        = {literal,  NULL,   PREC_NONE},
  [TOKEN_ATAU]          = {NULL,     or_,    PREC_OR},
  [TOKEN_BUANG]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_KEMBALIKAN]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_HASILKAN]      = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COCOKKAN]      = {NULL,     NULL,   PREC_NONE},
  [TOKEN_KASUS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {super_,   NULL,   PREC_NONE},
  [TOKEN_DIRI]          = {diri_,    NULL,   PREC_NONE},
  [TOKEN_BENAR]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_VARIABEL]      = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SELAGI]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TIDAK]         = {unary,    comparison, PREC_COMPARISON},
  [TOKEN_DALAM]         = {NULL,     comparison, PREC_COMPARISON},
  [TOKEN_ADALAH]        = {NULL,     comparison, PREC_COMPARISON},
  [TOKEN_BUKAN]         = {NULL,     comparison, PREC_COMPARISON},
  [TOKEN_SLASH_SLASH]   = {NULL,     binary, PREC_FACTOR},
  [TOKEN_AMPERSAND]     = {NULL,     binary, PREC_BIT_AND},
  [TOKEN_PIPE]          = {NULL,     binary, PREC_BIT_OR},
  [TOKEN_CARET]         = {NULL,     binary, PREC_BIT_XOR},
  [TOKEN_TILDE]         = {unary,    NULL,   PREC_NONE},
  [TOKEN_LESS_LESS]     = {NULL,     binary, PREC_SHIFT},
  [TOKEN_GREATER_GREATER] = {NULL,   binary, PREC_SHIFT},
  [TOKEN_SLASH_SLASH_EQUAL] = {NULL, NULL,   PREC_NONE},
  [TOKEN_PLUS_EQUAL]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS_EQUAL]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_STAR_EQUAL]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH_EQUAL]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PERCENT_EQUAL] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_POWER_EQUAL]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};// Batas kedalaman sarang parser. Tanpa ini, rekursi parser pada kode yang
// sangat bersarang menghabiskan stack C dan proses mati TANPA pesan: SIGSEGV
// di Linux/macOS, 0xC00000FD di Windows (T-W8). Ambangnya dipilih jauh di atas
// kode manusia yang wajar (sarang 100 level pun tetap dikompilasi) tetapi jauh
// di bawah kedalaman yang mengancam stack 1 MB bawaan Windows.
#define NESTING_MAX 200

static int nestingDepth = 0;

// Melewati batas dilaporkan sekali: panicMode menekan galat susulan, tetapi
// nestingDepth tetap dihitung turun dengan benar oleh pemanggilnya.
static bool nestingPush() {
    if (nestingDepth >= NESTING_MAX) {
        error("Sarang terlalu dalam (maksimum 200); sederhanakan kode.");
        // Parser berhenti turun di sini, jadi setiap blok induk gagal menemukan
        // '}' miliknya dan akan melapor lagi: 220 level sarang menghasilkan
        // ratusan galat susulan yang mengubur pesan sebenarnya. Mekanisme yang
        // sudah dipakai batas 'akhirnya' dipakai ulang; ia direset di awal
        // setiap kompilasi, jadi REPL dan impor berikutnya tidak ikut bisu.
        suppressFurtherErrors = true;
        return false;
    }
    nestingDepth++;
    return true;
}

static void nestingPop(void) {
    if (nestingDepth > 0) nestingDepth--;
}

static void parsePrecedence(Precedence precedence) {
    if (!nestingPush()) {
        // Tetap konsumsi satu token. Tanpa ini parser bisa berhenti maju:
        // `synchronize()` berhenti PADA token sinkronisasi ('fungsi', 'jika',
        // 'kelas', …) tanpa memakannya, karena normalnya pemanggilnya yang
        // memakai token itu. Di sini tidak ada yang memakainya — `fungsi(`
        // adalah fungsi anonim, jadi `declaration()` melemparnya ke jalur
        // ekspresi, dan jalur itu justru yang menolak turun. Hasilnya
        // block() → declaration() → parsePrecedence() → synchronize()
        // berputar selamanya pada token yang sama dan kompilasi menggantung
        // sambil terus mengalokasi (200 '{' lalu `fungsi(` menggantung tanpa
        // batas; ditemukan fuzzer).
        advance();
        return;
    }
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Harapkan ekspresi.");
        nestingPop();
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("Target assignment tidak valid.");
    }
    nestingPop();
}

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

// Conditional expression:  <benar> jika <kondisi> selain <salah>
//
// Python evaluates the condition FIRST, but an infix rule only runs after the
// left operand is already emitted. Emitting it early would break the common
// guard `x[0] jika panjang(x) > 0 selain "kosong"`, so the left operand's
// bytecode is truncated and re-parsed inside the true branch instead.
//
// ponytail: the left operand is parsed twice and its discarded constants stay
// in the constant table (a few bytes per conditional, no runtime cost).
// Buffering the operand's bytecode and splicing it would avoid that; not worth
// it until a profile says otherwise.
static void expression() {
    ScannerState leftStart = saveScanner();
    Token leftToken = parser.current;
    int leftOffset = currentChunk()->count;

    parsePrecedence(PREC_ASSIGNMENT);

    // A ternary's 'jika' must sit on the SAME line as the end of its left
    // operand. Inside '(' the scanner swallows newlines, so a statement-level
    // 'jika' opening the next line would otherwise be read as a conditional
    // expression -- exactly what happens in callback bodies passed as
    // arguments:
    //     app.rute("GET", "/x", fungsi(req) {
    //         variabel a = 1
    //         jika ... { }        <- begins a statement, not a ternary
    //     })
    // Comparing lines restores the statement boundary the parens erased.
    //
    // Cost: a conditional expression may no longer be split so that 'jika'
    // starts a new line. Wrap the whole expression differently, or keep
    // 'jika' on the operand's line. No code in this repo did that (240/240
    // tests pass), and silently mis-parsing callbacks is far worse.
    if (!check(TOKEN_JIKA) || parser.current.line != parser.previous.line) return;

    // Rewind the emitted left operand; it is re-compiled in the true branch.
    currentChunk()->count = leftOffset;

    advance();                              // consume 'jika'
    parsePrecedence(PREC_OR);               // the condition

    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);                       // drop the true condition

    ScannerState afterCond = saveScanner();
    Token afterCondToken = parser.current;

    restoreScanner(leftStart);
    parser.current = leftToken;
    parsePrecedence(PREC_ASSIGNMENT);       // the value, now correctly ordered

    restoreScanner(afterCond);
    parser.current = afterCondToken;

    int endJump = emitJump(OP_JUMP);
    patchJump(elseJump);
    emitByte(OP_POP);                       // drop the false condition

    consume(TOKEN_SELAIN, "Harapkan 'selain' pada ekspresi kondisional.");
    parsePrecedence(PREC_ASSIGNMENT);
    patchJump(endJump);
}

static void block() {
    if (!nestingPush()) {
        // Wajib maju, dengan alasan yang sama seperti parsePrecedence() di
        // atas: pemanggil block() mengulang sampai '}' atau EOF. Kembali di
        // sini tanpa memakan apa pun membuat classDeclaration() memanggil
        // method() -> function() -> block() pada token yang sama tanpa henti,
        // dan tiap putaran menumbuhkan chunk sampai kehabisan memori (201 '{'
        // lalu `kelas P { fungsi m() {} }` tumbuh ke beberapa GB lalu OOM;
        // ditemukan fuzzer, issue #109). Lewati isi blok sampai penutupnya.
        while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) advance();
        match(TOKEN_RIGHT_BRACE);
        return;
    }
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Harapkan '}' setelah blok.");
    nestingPop();
}

static bool skipBalancedBraces();

// K3: inside a function, `x = ...` creates a function-local. The compiler is
// single-pass, so before compiling the body we peek over its tokens once and
// pre-declare a slot for every name assigned there. Doing it lazily at the
// assignment would put the slot inside whatever jika/selagi scope we happen to
// be in and discard it at that block's end.
// Nested fungsi/kelas bodies are skipped: they get their own prescan.
static void prescanFunctionBody() {
    Token pending[UINT8_COUNT];
    int pendingCount = 0;
    Token declared[UINT8_COUNT];   // `variabel x` at the body's own depth
    int declaredCount = 0;
    // `variabel x` di kedalaman mana pun — dipakai HANYA untuk menekan lint
    // atas slot prescan yang pasti terbayangi. Tidak memengaruhi bytecode.
    Token shadowed[UINT8_COUNT];
    int shadowedCount = 0;
    Token chain[UINT8_COUNT];      // the names in `a, b, c = ...`
    int chainCount = 0;
    ScannerState saved = saveScanner();
    Token saveCurrent = parser.current;

    // One entry per open '(' / '[': is it a call's argument list? There
    // `nama =` is a keyword argument, never an assignment, and a comma does
    // not chain names into an unpack -- `g(D, b = 2)` once made both D and b
    // function-locals, so D was read as kosong.
    bool callGroup[UINT8_COUNT];
    int groupDepth = 0;
    int depth = 1;
    Token prev2 = parser.previous;
    Token prev = parser.previous;   // the '{' we just consumed
    Token token = parser.current;   // already-scanned lookahead: start here
    while (depth > 0 && token.type != TOKEN_EOF) {
        if (token.type == TOKEN_LEFT_PAREN || token.type == TOKEN_LEFT_BRACKET) {
            if (groupDepth < UINT8_COUNT) {
                callGroup[groupDepth] = token.type == TOKEN_LEFT_PAREN &&
                    (prev.type == TOKEN_IDENTIFIER || prev.type == TOKEN_RIGHT_PAREN ||
                     prev.type == TOKEN_RIGHT_BRACKET || prev.type == TOKEN_SUPER);
            }
            groupDepth++;
        } else if (token.type == TOKEN_RIGHT_PAREN || token.type == TOKEN_RIGHT_BRACKET) {
            if (groupDepth > 0) groupDepth--;
        }

        if (token.type == TOKEN_LEFT_BRACE) depth++;
        else if (token.type == TOKEN_RIGHT_BRACE) { if (--depth == 0) break; }
        else if (token.type == TOKEN_FUNGSI || token.type == TOKEN_KELAS) {
            // ponytail: scan to the body's '{'; safe while no construct before
            // it can contain braces (no dict literals / anonymous functions yet).
            while (token.type != TOKEN_LEFT_BRACE && token.type != TOKEN_EOF) {
                token = scanToken();
            }
            if (token.type == TOKEN_EOF || !skipBalancedBraces()) break;
        } else if (token.type == TOKEN_GLOBAL || token.type == TOKEN_NONLOKAL) {
            bool isGlobal = token.type == TOKEN_GLOBAL;
            Token* names = isGlobal ? current->globals : current->nonlocals;
            int* count = isGlobal ? &current->globalCount : &current->nonlocalCount;
            do {
                token = scanToken();
                if (token.type != TOKEN_IDENTIFIER) break;
                if (!scopeNameDeclared(names, *count, &token) && *count < UINT8_COUNT) {
                    names[(*count)++] = token;
                }
                token = scanToken();
            } while (token.type == TOKEN_COMMA);
            continue;
        } else if (token.type == TOKEN_UNTUK &&
                   (prev.type == TOKEN_NEWLINE || prev.type == TOKEN_LEFT_BRACE ||
                    prev.type == TOKEN_RIGHT_BRACE || prev.type == TOKEN_SEMICOLON)) {
            // K21: a statement-level `untuk x dalam` assigns x, so x is a
            // function-local like `x = ...`. Only at statement start: the
            // `untuk` of a comprehension follows an expression, and its
            // variable stays private to the comprehension (Python 3).
            Token next = scanToken();
            if (next.type == TOKEN_IDENTIFIER && pendingCount < UINT8_COUNT) {
                pending[pendingCount++] = next;
            }
            prev2 = token;
            prev = next;
            token = scanToken();
            chainCount = 0;
            continue;
        } else if (token.type == TOKEN_VARIABEL) {
            // `variabel x` at the body's own depth owns that slot; a prescan
            // slot for the same name would collide in the same scope.
            Token next = scanToken();
            // Hanya kedalaman 1 yang memiliki slot fungsi ini; blok bersarang
            // punya scope sendiri. Tapi anotasinya tetap WAJIB dilewati di
            // SEMUA kedalaman (lihat di bawah).
            if (depth == 1 && next.type == TOKEN_IDENTIFIER &&
                declaredCount < UINT8_COUNT) {
                declared[declaredCount++] = next;
            }
            // Untuk LINT saja: `variabel x` pada kedalaman BERAPA PUN berarti
            // slot prescan bernama sama akan dibayangi deklarasi itu dan tidak
            // pernah dibaca. Slotnya tetap dibuat (semantik tak berubah), tapi
            // melaporkannya sebagai "tidak pernah dipakai" adalah false
            // positive: yang mati adalah artefak kompiler, bukan kode pengguna.
            if (next.type == TOKEN_IDENTIFIER && shadowedCount < UINT8_COUNT) {
                shadowed[shadowedCount++] = next;
            }
            prev2 = token;
            prev = next;
            token = scanToken();
            // 'variabel a: teks = ...' — tanpa melewati anotasi, prescan
            // melihat pasangan 'teks' lalu '=' dan menyimpulkan 'teks' adalah
            // variabel yang di-assign, sehingga mengalokasikan slot lokal
            // tambahan + OP_KOSONG. prev/prev2 sengaja TIDAK diubah supaya
            // cabang '=' tetap melihat 'variabel a' dan mengabaikannya.
            if (token.type == TOKEN_COLON) {
                scanToken();            // nama tipe
                token = scanToken();
            }
            chainCount = 0;
            continue;
        } else if (token.type == TOKEN_EQUAL || isCompoundAssign(token.type)) {
            // Only a bare name is a candidate: diri.x = and a[i] = assign
            // through an object, and `variabel x =` declares its own local.
            bool keywordArg = token.type == TOKEN_EQUAL && groupDepth > 0 &&
                groupDepth <= UINT8_COUNT && callGroup[groupDepth - 1] &&
                (prev2.type == TOKEN_LEFT_PAREN || prev2.type == TOKEN_COMMA);
            if (prev.type == TOKEN_IDENTIFIER && prev2.type != TOKEN_DOT &&
                prev2.type != TOKEN_VARIABEL && !keywordArg) {
                // `a, b = ...` assigns EVERY name in the list, not just the last
                // one — otherwise 'a' would silently become a global (K3).
                if (chainCount > 1) {
                    for (int i = 0; i < chainCount && pendingCount < UINT8_COUNT; i++) {
                        pending[pendingCount++] = chain[i];
                    }
                } else if (pendingCount < UINT8_COUNT) {
                    pending[pendingCount++] = prev;
                }
            }
            chainCount = 0;
        }

        if (token.type == TOKEN_IDENTIFIER) {
            if (chainCount == 0 || prev.type == TOKEN_COMMA) {
                if (chainCount < UINT8_COUNT) chain[chainCount++] = token;
            } else {
                chainCount = 1;
                chain[0] = token;
            }
        } else if (token.type != TOKEN_COMMA && token.type != TOKEN_EQUAL) {
            if (token.type != TOKEN_IDENTIFIER) chainCount = 0;
        }
        prev2 = prev;
        prev = token;
        token = scanToken();
    }

    restoreScanner(saved);
    parser.current = saveCurrent;

    for (int i = 0; i < pendingCount; i++) {
        Token* name = &pending[i];
        if (scopeNameDeclared(current->globals, current->globalCount, name)) continue;
        if (scopeNameDeclared(current->nonlocals, current->nonlocalCount, name)) continue;
        if (scopeNameDeclared(declared, declaredCount, name)) continue;
        if (resolveLocal(current, name) != -1) continue;
        if (current->localCount == UINT8_COUNT) break;
        addLocal(*name);
        markInitialized();
        // K3 locals are real user declarations: `sisa = 5` inside a function is
        // how most people declare a variable here, `variabel` being optional.
        // Without this the linter only ever saw explicit `variabel` (T-U1).
        // Kecuali bila nama itu juga dideklarasikan `variabel` di blok
        // bersarang: slot ini pasti terbayangi, jadi "tidak pernah dipakai"
        // bukan kesalahan pengguna.
        if (!scopeNameDeclared(shadowed, shadowedCount, name)) markLintable();
        emitByte(OP_KOSONG);
    }
}

// Konsumsi anotasi tipe lalu BUANG: ': <tipe>' atau '-> <tipe>'.
//
// Anotasi tidak memancarkan satu byte bytecode pun dan tidak mengalokasi
// apa pun, jadi program dengan dan tanpa anotasi harus menghasilkan chunk
// yang identik (dibuktikan oleh tests/test_anotasi_bytecode.sh).
//
// Nama tipe sengaja dibaca sebagai IDENTIFIER biasa, bukan kata kunci:
// 'teks'/'bilangan'/'daftar' adalah nama variabel yang sah di kode lama,
// dan menjadikannya kata kunci akan merusaknya.
//
// Bentuk majemuk (generik 'daftar[bilangan]', union 'a | b') sengaja TIDAK
// diterima — lihat bagian "sengaja tidak dikerjakan" di rencana fiturnya.
static void parseAnotasiTipe(bool kembali) {
    if (!match(kembali ? TOKEN_PANAH : TOKEN_COLON)) return;
    // 'kosong' adalah kata kunci (TOKEN_KOSONG), bukan identifier, tapi ia
    // nama tipe yang wajar untuk fungsi tanpa nilai kembali ('-> kosong').
    // Tanpa cabang ini bentuk itu mustahil ditulis.
    if (match(TOKEN_KOSONG)) return;
    consume(TOKEN_IDENTIFIER, kembali ? "Harapkan nama tipe setelah '->'."
                                      : "Harapkan nama tipe setelah ':'.");
}

static void skipDefaultExpression() {
    // Lewati ekspresi bawaan parameter tanpa mengompilasinya. Ekspresi itu
    // dikompilasi belakangan sebagai prolog bersyarat, jadi di sini cukup
    // mencari ujungnya: ',' atau ')' pertama pada kedalaman nol.
    int depth = 0;
    for (;;) {
        TokenType type = parser.current.type;
        if (type == TOKEN_EOF) {
            errorAtCurrent("Harapkan ')' setelah parameter.");
            return;
        }
        if (depth == 0 && (type == TOKEN_COMMA || type == TOKEN_RIGHT_PAREN)) return;
        if (type == TOKEN_LEFT_PAREN || type == TOKEN_LEFT_BRACKET || type == TOKEN_LEFT_BRACE) {
            depth++;
        } else if (type == TOKEN_RIGHT_PAREN || type == TOKEN_RIGHT_BRACKET || type == TOKEN_RIGHT_BRACE) {
            depth--;
        }
        advance();
    }
}

static void function(FunctionType type) {
    bool anonymous = (type == TYPE_FUNCTION && parser.previous.type == TOKEN_FUNGSI);
    // Compiler berisi empat larik UINT8_COUNT (locals, upvalues, globals,
    // nonlocals) plus finallies: ~24 KB. Di stack, 16 fungsi bersarang sudah
    // menghabiskan stack 1 MB bawaan Windows dan prosesnya mati TANPA pesan
    // (T-W8). Di heap, satu level sarang hanya memakai beberapa puluh byte
    // stack, jadi kedalaman dibatasi oleh NESTING_MAX yang memberi galat rapi.
    Compiler* compilerHeap = (Compiler*)malloc(sizeof(Compiler));
    if (compilerHeap == NULL) {
        error("Memori habis saat mengompilasi fungsi.");
        return;
    }
    initCompiler(compilerHeap, type);
    beginScope();

    // A function body is a fresh compilation unit with its own chunk. The
    // loop context (used to patch break/continue jumps) must NOT leak in from
    // an enclosing loop, or break/continue would record jump offsets against
    // the outer chunk and later patch them into this smaller one — a heap
    // buffer overflow. Reset it for the duration of the body and restore after.
    LoopCtx* savedLoop = currentLoop;
    currentLoop = NULL;

    consume(TOKEN_LEFT_PAREN, anonymous ? "Harapkan '(' setelah 'fungsi'."
                                        : "Harapkan '(' setelah nama fungsi.");
    // Parameter names are declared first; their default expressions are
    // compiled afterwards as a conditional prologue (see below), so we
    // remember where each '=' sits and rewind the scanner to it.
    // Empat larik ini (bersama ~24 KB struct Compiler di atas) membuat frame
    // function() sebesar 42 KB saat masih di stack: 16 fungsi bersarang sudah
    // menghabiskan stack 1 MB bawaan Windows (T-W8). Semuanya disatukan dalam
    // satu alokasi heap agar hanya ada satu malloc dan satu free.
    struct FunctionScratch {
        ScannerState defaultStarts[UINT8_COUNT];
        Token defaultTokens[UINT8_COUNT];
        int defaultSlots[UINT8_COUNT];
        Token paramTokens[UINT8_COUNT];
    };
    struct FunctionScratch* scratch =
        (struct FunctionScratch*)malloc(sizeof(struct FunctionScratch));
    if (scratch == NULL) {
        error("Memori habis saat mengompilasi parameter fungsi.");
        free(compilerHeap);
        return;
    }
    ScannerState* defaultStarts = scratch->defaultStarts;
    Token* defaultTokens = scratch->defaultTokens;
    int* defaultSlots = scratch->defaultSlots;
    Token* paramTokens = scratch->paramTokens;
    int defaultCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                errorAtCurrent("Tidak boleh lebih dari 255 parameter.");
            }
            uint8_t constant = parseVariable("Harapkan nama parameter.");
            if (current->function->arity <= UINT8_COUNT) {
                paramTokens[current->function->arity - 1] = parser.previous;
            }
            defineVariable(constant);

            // Anotasi berada SETELAH nama dan SEBELUM '=': 'a: teks = "x"'.
            parseAnotasiTipe(false);

            if (match(TOKEN_EQUAL)) {
                defaultStarts[defaultCount] = saveScanner();
                defaultTokens[defaultCount] = parser.current;
                // Slot = parameter position: locals[0] is the function itself.
                defaultSlots[defaultCount] = current->function->arity;
                defaultCount++;
                skipDefaultExpression();
            } else if (defaultCount > 0) {
                // semanticError: the code is well-formed, just not allowed —
                // staying out of panic mode avoids a bogus "Harapkan '}'" cascade.
                semanticError("Parameter wajib tidak boleh setelah parameter dengan nilai bawaan.");
            }
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Harapkan ')' setelah parameter.");
    // Tipe kembali: 'fungsi f() -> teks {'.
    parseAnotasiTipe(true);
    current->function->requiredArity = current->function->arity - defaultCount;

    // Parameters are part of a signature, not a user decision to declare a
    // variable: an unused one is usually required (callbacks, _tambah_(lain)).
    for (int i = 0; i < current->localCount; i++) current->locals[i].lintCheck = false;

    // Parameter names must survive to runtime: keyword arguments are matched
    // in the VM, because the callee is only known there.
    if (current->function->arity > 0) {
        ObjFunction* fn = current->function;
        fn->paramNames = ALLOCATE(ObjString*, fn->arity);
        // Clear BEFORE any copyString: that allocates, which can trigger a GC
        // that walks paramNames. Uninitialised slots would be marked as live
        // garbage pointers.
        for (int i = 0; i < fn->arity; i++) fn->paramNames[i] = NULL;
        for (int i = 0; i < fn->arity; i++) {
            fn->paramNames[i] = copyString(paramTokens[i].start, paramTokens[i].length);
        }
    }

    // Conditional prologue. call() fills unsupplied parameter slots with the
    // TAKTERISI marker; each default tests its own slot and only evaluates
    // when still unfilled. Unlike a staged prologue (jump straight to the
    // right entry point) this copes with gaps: f(1, c=9) leaves b unfilled
    // while c is filled, which no single entry point could express.
    if (defaultCount > 0) {
        ScannerState afterParams = saveScanner();
        Token afterParamsCurrent = parser.current;
        for (int i = 0; i < defaultCount; i++) {
            emitBytes(OP_ISI_BAWAAN, (uint8_t)defaultSlots[i]);
            int skip = currentChunk()->count;
            emitByte(0xff);
            emitByte(0xff);
            restoreScanner(defaultStarts[i]);
            parser.current = defaultTokens[i];
            expression();
            emitBytes(OP_SET_LOCAL, (uint8_t)defaultSlots[i]);
            emitByte(OP_POP);
            int jump = currentChunk()->count - skip - 2;
            if (jump > UINT16_MAX) error("Nilai bawaan terlalu panjang.");
            currentChunk()->code[skip] = (jump >> 8) & 0xff;
            currentChunk()->code[skip + 1] = jump & 0xff;
        }
        restoreScanner(afterParams);
        parser.current = afterParamsCurrent;
    }

    consume(TOKEN_LEFT_BRACE, "Harapkan '{' sebelum badan fungsi.");
    prescanFunctionBody();
    block();

    ObjFunction* function = endCompiler();

    currentLoop = savedLoop;

    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

    for (int i = 0; i < function->upvalueCount; i++) {
        emitByte(compilerHeap->upvalues[i].isLocal ? 1 : 0);
        emitByte(compilerHeap->upvalues[i].index);
    }
    // endCompiler() sudah mengembalikan `current` ke compiler induk, dan
    // ObjFunction hidup di heap GC, jadi struct ini tidak dirujuk lagi.
    free(scratch);
    free(compilerHeap);
}

#define DECORATOR_MAX 16

// Compile the '@' decorators in front of a definition, pushing each one onto
// the stack in source order. Returns how many were pushed.
//
// The whole feature needs no new opcode: with [d0, d1, nilai] on the stack,
// one OP_CALL 1 leaves [d0, d1(nilai)] and the next leaves [d0(d1(nilai))] --
// exactly Python's bottom-up application order, and the final value ends up
// in the slot where d0 was pushed. That is why the declared variable may be
// declared BEFORE the decorators are pushed: its slot is d0's slot.
static int decoratorList() {
    int count = 0;
    while (match(TOKEN_AT)) {
        if (count == DECORATOR_MAX) {
            error("Terlalu banyak dekorator pada satu definisi.");
            return count;
        }
        // A decorator is an ordinary expression, so @nama, @modul.nama, and
        // @pabrik(arg) all work. It must stay on one line: the newline is the
        // terminator, which keeps error messages pointing at the '@' line.
        expression();
        count++;
        if (!check(TOKEN_EOF) && !match(TOKEN_NEWLINE) && !match(TOKEN_SEMICOLON)) {
            error("Harapkan baris baru setelah dekorator.");
            return count;
        }
        while (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) { }
    }
    return count;
}

static void applyDecorators(int count) {
    for (int i = 0; i < count; i++) {
        emitBytes(OP_CALL, 1);
    }
}

static void funDeclaration() {
    funDeclarationDecorated(0);
}

// `decorators` of them are already on the stack, pushed before the name was
// declared, so the decorated result lands exactly in the variable's slot.
static void funDeclarationDecorated(int decorators) {
    uint8_t global = parseVariable("Harapkan nama fungsi.");
    markInitialized();
    function(TYPE_FUNCTION);
    applyDecorators(decorators);
    defineVariable(global);
}

static void method() {
    // Custom decorators on methods are rejected, and it is not an oversight:
    // `diri` is implicit here (K3), so a wrapper returned by a decorator is a
    // plain function with no receiver. Calling f() inside it loses `diri`, and
    // the failure surfaces far away, inside the method. Python only gets away
    // with this because `self` is an explicit parameter it can forward.
    // The three builtin decorators are handled at method-definition time
    // instead; see RENCANA_FITUR/selesai/dekorator.md.
    // The three builtin decorators change how a method is CALLED, so they are
    // resolved here, at definition time, not as runtime wrappers.
    uint8_t special = 0;
    bool specialSeen = false;
    while (match(TOKEN_AT)) {
        if (!check(TOKEN_IDENTIFIER)) {
            error("Harapkan nama dekorator setelah '@'.");
            return;
        }
        advance();
        if (identifiersEqualCString(&parser.previous, "properti")) {
            special = SPECIAL_PROPERTI;
        } else if (identifiersEqualCString(&parser.previous, "metode_statis")) {
            special = SPECIAL_STATIS;
        } else if (identifiersEqualCString(&parser.previous, "metode_kelas")) {
            special = SPECIAL_KELAS;
        } else if (identifiersEqualCString(&parser.previous, "abstrak")) {
            special = SPECIAL_ABSTRAK;
        } else {
            error("Dekorator kustom pada metode belum didukung: 'diri' tidak "
                  "terbawa ke fungsi pembungkus. Yang tersedia: @properti, "
                  "@metode_statis, @metode_kelas.");
            // Must consume something: classDeclaration()'s loop calls
            // method() until '}' or EOF, so returning here with the rest of
            // the line unread would spin forever instead of reporting it.
            while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF) &&
                   !check(TOKEN_NEWLINE)) {
                advance();
            }
            return;
        }
        if (specialSeen) {
            // One tag per method: ObjClass->special stores a single value per
            // name, so a second one would silently overwrite the first.
            error("Hanya satu dekorator bawaan per metode.");
            return;
        }
        specialSeen = true;
        while (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) { }
    }
    consume(TOKEN_IDENTIFIER, "Harapkan nama metode.");
    uint8_t constant = identifierConstant(&parser.previous);

    FunctionType type = TYPE_METHOD;
    if (special == SPECIAL_STATIS) type = TYPE_STATIC;
    else if (special == SPECIAL_KELAS) type = TYPE_CLASSMETHOD;
    if (parser.previous.length == 4 &&
        memcmp(parser.previous.start, "init", 4) == 0) {
        type = TYPE_INITIALIZER;
    }
    function(type);
    if (special != 0) {
        emitBytes(OP_SPECIAL_METHOD, constant);
        emitByte(special);
    } else {
        emitBytes(OP_METHOD, constant);
    }
}

static void classDeclaration() {
    consume(TOKEN_IDENTIFIER, "Harapkan nama kelas.");
    Token className = parser.previous;
    uint8_t nameConstant = identifierConstant(&parser.previous);
    declareVariable();

    emitBytes(OP_CLASS, nameConstant);
    defineVariable(nameConstant);

    ClassCompiler classCompiler;
    classCompiler.hasSuperclass = false;
    classCompiler.enclosing = currentClass;
    currentClass = &classCompiler;

    // Optional superclass:  kelas Anak < Induk { ... }
    if (match(TOKEN_LESS)) {
        // kelas D < B, C { ... } -- semua induk didorong ke stack, lalu
        // OP_INHERIT membaca jumlahnya sebagai operand.
        int baseCount = 0;
        do {
            consume(TOKEN_IDENTIFIER, "Harapkan nama kelas induk.");
            variable(false);
            if (identifiersEqual(&className, &parser.previous)) {
                error("Sebuah kelas tidak dapat mewarisi dirinya sendiri.");
            }
            if (baseCount == 255) {
                error("Terlalu banyak kelas induk.");
            }
            baseCount++;
        } while (match(TOKEN_COMMA));

        namedVariable(className, false);
        emitBytes(OP_INHERIT, (uint8_t)baseCount);

        // OP_INHERIT pops only the subclass, leaving base1..baseN. Drop all
        // but the first so the slot 'super' names is base1 -- `super` keeps
        // meaning the first parent, which is also what C3 puts right after
        // the class itself.
        for (int i = 1; i < baseCount; i++) emitByte(OP_POP);

        beginScope();
        addLocal(syntheticToken("super"));
        defineVariable(0);
        classCompiler.hasSuperclass = true;
    }

    namedVariable(className, false);  // push the class for OP_METHOD
    consume(TOKEN_LEFT_BRACE, "Harapkan '{' sebelum badan kelas.");
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        while (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) { }
        if (check(TOKEN_RIGHT_BRACE) || check(TOKEN_EOF)) break;
        if (match(TOKEN_LEWATI)) continue;
        // Atribut kelas: NAMA = ekspresi. Satu token lihat-depan memisahkannya
        // dari metode, yang selalu NAMA '('. Kelasnya sudah ada di atas stack.
        if (check(TOKEN_IDENTIFIER) && checkNext(TOKEN_EQUAL)) {
            consume(TOKEN_IDENTIFIER, "Harapkan nama atribut kelas.");
            uint8_t attr = identifierConstant(&parser.previous);
            consume(TOKEN_EQUAL, "Harapkan '=' setelah nama atribut kelas.");
            expression();
            emitBytes(OP_CLASS_FIELD, attr);
            continue;
        }
        method();
    }
    consume(TOKEN_RIGHT_BRACE, "Harapkan '}' setelah badan kelas.");
    emitByte(OP_POP);  // pop the class

    if (classCompiler.hasSuperclass) {
        endScope();
    }

    currentClass = currentClass->enclosing;
}

static void varDeclaration() {
    uint8_t global = parseVariable("Harapkan nama variabel.");

    // 'variabel a: bilangan = 5'. Anotasi sebelum '=', seperti di parameter.
    parseAnotasiTipe(false);

    if (match(TOKEN_EQUAL)) {
        bool saved = allowTupleRHS;
        allowTupleRHS = true;
        expression();
        maybeTupleTail();           // variabel x = 1, 2
        allowTupleRHS = saved;
    } else {
        emitByte(OP_KOSONG);
    }

    defineVariable(global);
}

static void expressionStatement() {
    // Statement level: a comma here may build a tupel (x = 1, 2). The flag is
    // consumed by namedVariable's assignment branch; expressions that are not
    // assignments simply never look at it.
    bool saved = allowTupleRHS;
    allowTupleRHS = true;
    expression();
    allowTupleRHS = saved;
    emitByte(OP_POP);
}

static void assignToName(Token name);

static void forStatement() {
    // Desugar:  untuk x dalam <list> { body }
    // into an index-based loop over the list, using hidden local slots.
    beginScope();

    consume(TOKEN_IDENTIFIER, "Harapkan nama variabel setelah 'untuk'.");
    Token varName = parser.previous;

    consume(TOKEN_DALAM, "Harapkan 'dalam' setelah nama variabel.");

    // Evaluate the iterable and stash it in a hidden slot.
    expression();
    // K19: an instance with _iter_ is replaced by whatever it returns, ONCE,
    // before the loop starts. It cannot live in OP_LIST_LEN/OP_SEQ_GET -- both
    // run every iteration and would re-enter the user's method n times.
    emitByte(OP_ITER_OBJEK);
    int seqSlot = addSyntheticLocal("  untuk seq");

    // Hidden length slot.
    emitBytes(OP_GET_LOCAL, (uint8_t)seqSlot);
    emitByte(OP_LIST_LEN);
    int lenSlot = addSyntheticLocal("  untuk len");

    // Hidden index slot = -1 (the increment runs before the first
    // condition check, bringing it to 0 for the first iteration).
    emitConstant(NUMBER_VAL(-1));
    int idxSlot = addSyntheticLocal("  untuk idx");

    // K21: the loop variable is NOT a block-scoped local. Each element is
    // assigned exactly like `x = elemen`: a global at script level, the K3
    // function-local (pre-declared by prescanFunctionBody) inside a function,
    // and it honours global/nonlokal. So, as in Python, it overwrites a name
    // that already exists and keeps its last value after the loop.

    consume(TOKEN_LEFT_BRACE, "Harapkan '{' setelah ekspresi dalam 'untuk'.");

    // Layout (so 'lanjut' can target the increment):
    //   incrementStart:  idx = idx + 1
    //   condStart:       if !(idx < len) goto exit
    //                    x = seq[idx]; body; goto incrementStart
    // On first entry we jump straight to condStart, skipping the increment.
    int incrementStart = currentChunk()->count;

    // idx = idx + 1
    emitBytes(OP_GET_LOCAL, (uint8_t)idxSlot);
    emitConstant(NUMBER_VAL(1));
    emitByte(OP_ADD);
    emitBytes(OP_SET_LOCAL, (uint8_t)idxSlot);
    emitByte(OP_POP);

    int condStart = currentChunk()->count;

    // Condition: idx < len
    emitBytes(OP_GET_LOCAL, (uint8_t)idxSlot);
    emitBytes(OP_GET_LOCAL, (uint8_t)lenSlot);
    emitByte(OP_LESS);
    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // condition

    // x = seq[idx]  (SEQ_GET: a kamus yields keys by position)
    int iteratorHandler = emitJump(OP_TRY_BEGIN);
    current->tryDepth++;
    emitBytes(OP_GET_LOCAL, (uint8_t)seqSlot);
    emitBytes(OP_GET_LOCAL, (uint8_t)idxSlot);
    emitByte(OP_ITER_NEXT);
    assignToName(varName);
    emitByte(OP_TRY_END);
    current->tryDepth--;
    int iteratorNormal = emitJump(OP_JUMP);

    patchJump(iteratorHandler);
    // The VM leaves the raised error on the stack at the handler.
    namedVariable(syntheticToken("HentiIterasi"), false);
    emitByte(OP_IS_INSTANCE);
    int iteratorNotExhausted = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);             // matching boolean
    emitByte(OP_POP);             // HentiIterasi instance
    int iteratorExit = emitJump(OP_JUMP);
    patchJump(iteratorNotExhausted);
    emitByte(OP_POP);             // non-matching boolean
    emitByte(OP_THROW);           // rethrow the original error
    patchJump(iteratorNormal);

    // Loop body. 'lanjut' jumps to incrementStart; 'hentikan' to the end.
    LoopCtx loop;
    beginLoop(&loop, incrementStart);
    beginScope();
    block();
    endScope();

    emitLoop(incrementStart);

    patchJump(exitJump);
    emitByte(OP_POP); // condition
    patchJump(iteratorExit);

    endLoop();  // break jumps land here, past the condition pop

    endScope();
}

static void beginLoop(LoopCtx* loop, int continueTarget) {
    loop->enclosing = currentLoop;
    loop->continueTarget = continueTarget;
    loop->scopeDepth = current->scopeDepth;
    loop->tryDepth = current->tryDepth;
    loop->finallyCount = current->finallyCount;
    loop->breakCount = 0;
    currentLoop = loop;
}

static void endLoop() {
    for (int i = 0; i < currentLoop->breakCount; i++) {
        patchJump(currentLoop->breakJumps[i]);
    }
    currentLoop = currentLoop->enclosing;
}

// Emit OP_POP for each local declared deeper than the given scope depth, so
// break/continue leave the operand stack balanced. Does not remove the
// compiler's record of those locals (they remain in scope for the rest of
// the body); endScope() handles the real cleanup.
static void popLocalsToDepth(int depth) {
    for (int i = current->localCount - 1;
         i >= 0 && current->locals[i].depth > depth;
         i--) {
        emitByte(OP_POP);
    }
}

static void block();

// Compile an 'akhirnya' block again at the current position (the code that
// jumps out of its 'coba'). The single-pass compiler cannot jump to the
// block's normal copy, so — like CPython — the block is duplicated: rewind
// the scanner to it, compile it, then restore the parser exactly.
static void inlineFinallyBlock(FinallyRegion* region) {
    if (region->kind == FINALLY_EXIT_CALL) {
        // 'dengan': three instructions instead of re-compiling a block, so
        // the inline budget buys far more copies here than for 'akhirnya'.
        Token keluar = syntheticToken("_keluar_");
        emitBytes(OP_GET_LOCAL, region->managerSlot);
        emitBytes(OP_INVOKE, identifierConstant(&keluar));
        emitByte(0);                        // argCount
        emitByte(OP_POP);                   // discard _keluar_'s return value
        return;
    }
    Parser savedParser = parser;
    ScannerState savedScanner = saveScanner();
    LoopCtx* savedLoop = currentLoop;
    const char* savedArgStart = callArgStart;

    restoreScanner(region->finallyBlock);
    currentLoop = region->loop;   // 'hentikan' in it targets the loop around 'coba'
    callArgStart = NULL;
    inlineFinallyDepth++;
    advance();  // load the block's '{'
    consume(TOKEN_LEFT_BRACE, "Harapkan '{' setelah 'akhirnya'.");
    beginScope();
    block();
    endScope();
    inlineFinallyDepth--;

    bool hadError = parser.hadError;
    parser = savedParser;
    parser.hadError = parser.hadError || hadError;
    restoreScanner(savedScanner);
    currentLoop = savedLoop;
    callArgStart = savedArgStart;
}

// Emit what must happen before jumping out of 'coba' blocks: run the
// 'akhirnya' blocks of finally regions [targetFinallies, finallyCount),
// innermost first, and unregister handlers down to targetTryDepth. Each
// finally block runs with only the handlers outside its own 'coba' active,
// so an error inside it propagates outward (as in Python).
// The cap is usually hit deep inside an inlined copy, where errors are
// silenced (the in-place copy reports them); this one has no in-place twin,
// so report it with suppression lifted, once per function.
static void reportFinallyBudget() {
    parser.hadError = true;
    if (current->finallyBudgetReported) return;
    current->finallyBudgetReported = true;
    // Lift both silencers: inline suppression AND panic mode (a silenced
    // error inside the copy may have set it), or the message would vanish.
    int savedDepth = inlineFinallyDepth;
    bool savedPanic = parser.panicMode;
    inlineFinallyDepth = 0;
    parser.panicMode = false;
    semanticError("Terlalu banyak salinan blok 'akhirnya' dalam satu fungsi: "
                  "'kembalikan'/'hentikan'/'lanjut' di dalam 'akhirnya' yang "
                  "bersarang. Sederhanakan alurnya.");
    inlineFinallyDepth = savedDepth;
    parser.panicMode = savedPanic;
    suppressFurtherErrors = true;  // the copies already made overflow the
                                   // constant table and cascade into noise
}

static void exitTryBlocks(int targetTryDepth, int targetFinallies) {
    int savedTryDepth = current->tryDepth;
    int savedFinallyCount = current->finallyCount;
    // Compiling an inlined block may push its own 'coba ... akhirnya'
    // regions into the slots we are walking (finallyCount is lowered to i),
    // so work from a snapshot and restore the real entries afterwards.
    FinallyRegion saved[FINALLY_MAX];
    memcpy(saved, current->finallies, sizeof(FinallyRegion) * savedFinallyCount);

    for (int i = savedFinallyCount - 1; i >= targetFinallies; i--) {
        FinallyRegion region = saved[i];
        for (int d = current->tryDepth; d > region.tryDepth; d--) {
            emitByte(OP_TRY_END);
        }
        current->tryDepth = region.tryDepth;
        current->finallyCount = i;  // a jump inside it must not re-run it
        if (current->finallyInlines >= FINALLY_INLINE_MAX) {
            reportFinallyBudget();
            break;  // compilation already failed; stop copying
        }
        current->finallyInlines++;
        inlineFinallyBlock(&region);
    }
    for (int d = current->tryDepth; d > targetTryDepth; d--) {
        emitByte(OP_TRY_END);
    }

    memcpy(current->finallies, saved, sizeof(FinallyRegion) * savedFinallyCount);
    current->tryDepth = savedTryDepth;
    current->finallyCount = savedFinallyCount;
}

static void breakStatement() {
    if (currentLoop == NULL) {
        error("'hentikan' hanya boleh di dalam perulangan.");
        return;
    }
    exitTryBlocks(currentLoop->tryDepth, currentLoop->finallyCount);
    popLocalsToDepth(currentLoop->scopeDepth);
    if (currentLoop->breakCount == UINT8_COUNT) {
        error("Terlalu banyak 'hentikan' dalam satu perulangan.");
        return;
    }
    currentLoop->breakJumps[currentLoop->breakCount++] = emitJump(OP_JUMP);
}

static void continueStatement() {
    if (currentLoop == NULL) {
        error("'lanjut' hanya boleh di dalam perulangan.");
        return;
    }
    exitTryBlocks(currentLoop->tryDepth, currentLoop->finallyCount);
    popLocalsToDepth(currentLoop->scopeDepth);
    emitLoop(currentLoop->continueTarget);
}

static void ifStatement() {
    expression();
    consume(TOKEN_LEFT_BRACE, "Harapkan '{' setelah kondisi 'jika'.");
    
    // Emit conditional jump - jump if condition is false
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);  // Pop condition from stack if true
    
    beginScope();
    block();
    endScope();
    
    // Jump over else/jika_lain block if condition was true
    int elseJump = emitJump(OP_JUMP);
    patchJump(thenJump);  // Patch the false condition jump
    emitByte(OP_POP);  // Pop condition from stack if false

    while (match(TOKEN_NEWLINE)) { }
    if (match(TOKEN_JIKA_LAIN)) {
        ifStatement();
    } else if (match(TOKEN_SELAIN)) {
        consume(TOKEN_LEFT_BRACE, "Harapkan '{' setelah 'selain'.");
        beginScope();
        block();
        endScope();
    }

    patchJump(elseJump);  // Patch the true path jump to skip else
}

static void raiseStatement() {
    // naikkan <ekspresi>  -> evaluate then throw.
    expression();
    emitByte(OP_THROW);
}

// pastikan <kondisi>[, <pesan>]
// The message is compiled INSIDE the failure branch so it is only evaluated
// when the assertion fails, like Python.
static void assertStatement() {
    const char* conditionStart = parser.current.start;
    bool startsWithParen = check(TOKEN_LEFT_PAREN);
    lastTupleStart = NULL;
    expression();
    // `pastikan(kondisi, pesan)` is ONE tupel condition, which is always true,
    // so the assertion could never fail (T-U5). Python only warns; we reject
    // it, because a silently disabled assertion is worse than an error.
    if (startsWithParen && lastTupleStart == conditionStart &&
        lastTupleEnd == currentChunk()->count) {
        semanticError("Kondisi 'pastikan' berupa tupel sehingga selalu benar. "
                      "Tulis tanpa kurung: pastikan kondisi, \"pesan\".");
    }
    int failJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);                       // condition was true: discard it
    int doneJump = emitJump(OP_JUMP);

    patchJump(failJump);
    emitByte(OP_POP);                       // discard the false condition
    if (match(TOKEN_COMMA)) {
        expression();
    } else {
        emitByte(OP_KOSONG);                // kosong = use the default message
    }
    emitByte(OP_PASTIKAN);
    patchJump(doneJump);
}

// `impor a.b.c` / `dari a.b impor x` -- reads a dotted module name into `out`.
// Returns the constant index of the name string.
static uint8_t moduleNameConstant() {
    char name[256];
    int len = 0;
    do {
        consume(TOKEN_IDENTIFIER, "Harapkan nama modul.");
        if (len + parser.previous.length + 1 >= (int)sizeof name) {
            error("Nama modul terlalu panjang.");
            return 0;
        }
        if (len > 0) name[len++] = '.';
        memcpy(name + len, parser.previous.start, parser.previous.length);
        len += parser.previous.length;
    } while (match(TOKEN_DOT));
    name[len] = '\0';
    return makeConstant(OBJ_VAL(copyString(name, len)));
}

// The name a module is bound to: last component of a dotted path, so
// `impor a.b` defines `b` (Python binds `a`, but we have no package objects).
static uint8_t defaultBindingName(const char* dotted, int len) {
    int start = 0;
    for (int i = 0; i < len; i++) {
        if (dotted[i] == '.') start = i + 1;
    }
    return makeConstant(OBJ_VAL(copyString(dotted + start, len - start)));
}

static void importStatement() {
    // Legacy form, unchanged: impor "path.idk" runs a file in the CALLER's
    // namespace. Keeping it is what lets every pustaka/*.idk keep working.
    if (match(TOKEN_STRING)) {
        emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                        parser.previous.length - 2)));
        emitByte(OP_IMPORT);
        emitByte(OP_POP);  // discard the module's return value (kosong)
        return;
    }

    // impor nama[.sub] [sebagai alias]
    uint8_t nameConst = moduleNameConstant();
    ObjString* dotted = AS_STRING(currentChunk()->constants.values[nameConst]);

    uint8_t binding;
    if (match(TOKEN_SEBAGAI)) {
        consume(TOKEN_IDENTIFIER, "Harapkan nama alias setelah 'sebagai'.");
        binding = identifierConstant(&parser.previous);
    } else {
        binding = defaultBindingName(dotted->chars, dotted->length);
    }

    emitBytes(OP_IMPORT_MODULE, nameConst);
    emitBytes(OP_DEFINE_GLOBAL, binding);
}

// dari modul impor a [sebagai x] [, b [sebagai y]]...
static void fromImportStatement() {
    uint8_t nameConst = moduleNameConstant();
    consume(TOKEN_IMPOR, "Harapkan 'impor' setelah nama modul.");

    emitBytes(OP_IMPORT_MODULE, nameConst);   // module stays on the stack
    do {
        consume(TOKEN_IDENTIFIER, "Harapkan nama yang diimpor.");
        uint8_t member = identifierConstant(&parser.previous);
        uint8_t binding = member;
        if (match(TOKEN_SEBAGAI)) {
            consume(TOKEN_IDENTIFIER, "Harapkan nama alias setelah 'sebagai'.");
            binding = identifierConstant(&parser.previous);
        }
        emitBytes(OP_IMPORT_FROM, member);
        emitBytes(OP_DEFINE_GLOBAL, binding);
    } while (match(TOKEN_COMMA));

    emitByte(OP_POP);   // the module itself
}

// Lookahead helper: having just scanned a '{', skip to its matching '}'.
static bool skipBalancedBraces() {
    int depth = 1;
    while (depth > 0) {
        Token token = scanToken();
        if (token.type == TOKEN_EOF) return false;
        if (token.type == TOKEN_LEFT_BRACE) depth++;
        if (token.type == TOKEN_RIGHT_BRACE) depth--;
    }
    return true;
}

// The compiler is single-pass, but the bytecode for 'coba' depends on what
// follows the body: an 'akhirnya' needs an outer handler registered BEFORE the
// body. Peek ahead (parser.current is the body's '{') and restore the scanner.
static void scanTryClauses(bool* hasCatch, bool* hasFinally,
                           ScannerState* finallyBlock) {
    *hasCatch = false;
    *hasFinally = false;
    if (parser.current.type != TOKEN_LEFT_BRACE) return;

    ScannerState saved = saveScanner();
    if (skipBalancedBraces()) {
        Token next = scanToken();
        // Skip EVERY 'kecuali' clause: a 'coba' may have several, and the
        // 'akhirnya' we are looking for sits after the last one.
        while (next.type == TOKEN_KECUALI) {
            *hasCatch = true;
            do {
                next = scanToken();
            } while (next.type != TOKEN_LEFT_BRACE && next.type != TOKEN_EOF);
            if (next.type == TOKEN_LEFT_BRACE && skipBalancedBraces()) {
                next = scanToken();
            } else {
                break;
            }
        }
        *hasFinally = next.type == TOKEN_AKHIRNYA;
        if (*hasFinally) *finallyBlock = saveScanner();
    }
    restoreScanner(saved);
}

// kecuali e { ... }, kecuali Tipe sebagai e { ... }, or
// kecuali (TipeA, TipeB) sebagai e { ... }. The VM has pushed the error.
//
// Several 'kecuali' clauses may follow one 'coba'. A typed clause that does
// not match must fall through to the NEXT clause, so instead of emitting the
// rethrow itself this returns the unpatched "no match" jump; the caller
// chains it to the following clause, or patches it to OP_THROW when the
// chain ends. *bare is set for `kecuali X { ... }` written without 'sebagai',
// which must still be the last clause (see tryStatement).
static int catchClause(int* doneJumps, int* doneCount, bool* bare) {
    consume(TOKEN_KECUALI, "Harapkan 'kecuali' atau 'akhirnya' setelah blok 'coba'.");
    bool typed = false;
    bool bareForm = false;
    Token caught;
    int successJumps[UINT8_COUNT];
    int successCount = 0;

    if (match(TOKEN_LEFT_PAREN)) {
        typed = true;
        do {
            consume(TOKEN_IDENTIFIER, "Harapkan kelas galat di dalam 'kecuali (...)'.");
            namedVariable(parser.previous, false);
            emitByte(OP_IS_INSTANCE);
            if (!check(TOKEN_RIGHT_PAREN)) {
                int tryNext = emitJump(OP_JUMP_IF_FALSE);
                successJumps[successCount++] = emitJump(OP_JUMP);
                patchJump(tryNext);
                emitByte(OP_POP);
            }
        } while (match(TOKEN_COMMA));
        consume(TOKEN_RIGHT_PAREN, "Harapkan ')' setelah kelas galat.");
        consume(TOKEN_SEBAGAI, "Harapkan 'sebagai' setelah kelas galat.");
        consume(TOKEN_IDENTIFIER, "Harapkan nama variabel setelah 'sebagai'.");
        caught = parser.previous;
    } else {
        consume(TOKEN_IDENTIFIER, "Harapkan nama galat atau tipe setelah 'kecuali'.");
        Token first = parser.previous;
        typed = match(TOKEN_SEBAGAI);
        caught = first;
        if (typed) {
            namedVariable(first, false);
            emitByte(OP_IS_INSTANCE);
            consume(TOKEN_IDENTIFIER, "Harapkan nama variabel setelah 'sebagai'.");
            caught = parser.previous;
        } else {
            // `kecuali X { ... }`. Whether X is a type to match or a name to
            // bind is only knowable at run time: the builtin error classes
            // live in globals, and a library may define its own
            // (`kelas GalatJaringan < GalatOS`). OP_CATCH_TEST looks X up and
            // decides. X stays bound inside the block either way.
            typed = true;
            bareForm = true;
            emitBytes(OP_CATCH_TEST, identifierConstant(&first));
        }
    }

    for (int i = 0; i < successCount; i++) patchJump(successJumps[i]);
    int rethrow = -1;
    if (typed) {
        rethrow = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
    }

    beginScope();
    addLocal(caught);
    markInitialized();
    consume(TOKEN_LEFT_BRACE, "Harapkan '{' setelah nama variabel 'kecuali'.");
    block();
    endScope();

    if (typed) {
        doneJumps[(*doneCount)++] = emitJump(OP_JUMP);
        if (bare != NULL) *bare = bareForm;
        return rethrow;   // caller chains this to the next clause
    }
    if (bare != NULL) *bare = false;
    return -1;            // untyped: always matches, ends the chain
}

// akhirnya { ... } — entered with two values on the stack: the raised value
// and a flag saying whether we arrived because of an error. After the block,
// a pending error is raised again (Python's finally semantics).
static void finallyClause() {
    consume(TOKEN_AKHIRNYA, "Harapkan 'akhirnya'.");
    beginScope();
    addLocal(syntheticToken(" galat"));   // names no identifier can match
    markInitialized();
    addLocal(syntheticToken(" lempar"));
    markInitialized();
    uint8_t errorSlot = (uint8_t)(current->localCount - 2);
    uint8_t rethrowSlot = (uint8_t)(current->localCount - 1);

    consume(TOKEN_LEFT_BRACE, "Harapkan '{' setelah 'akhirnya'.");
    beginScope();
    block();
    endScope();

    emitBytes(OP_GET_LOCAL, rethrowSlot);
    int noRethrow = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    emitBytes(OP_GET_LOCAL, errorSlot);
    emitByte(OP_THROW);
    patchJump(noRethrow);
    emitByte(OP_POP);
    endScope();  // pops the two hidden locals
}

// coba { ... } [kecuali e { ... }] [akhirnya { ... }]
//
// With 'akhirnya', an outer handler (registered first) catches errors from
// the body or the 'kecuali' block and routes them through the finally block,
// which rethrows; the normal path enters the finally block with no error.
static void tryStatement() {
    bool hasCatch, hasFinally;
    ScannerState finallyBlock;
    scanTryClauses(&hasCatch, &hasFinally, &finallyBlock);
    if (!hasCatch && !hasFinally) {
        // Let consume() below report the problem at the right token.
        hasCatch = true;
    }

    int finallyHandler = -1;
    if (hasFinally) {
        if (current->finallyCount == FINALLY_MAX) {
            error("Terlalu banyak 'coba ... akhirnya' bersarang.");
            hasFinally = false;
        } else {
            FinallyRegion* region = &current->finallies[current->finallyCount++];
            region->kind = FINALLY_SOURCE;
            region->tryDepth = current->tryDepth;
            region->finallyBlock = finallyBlock;
            region->loop = currentLoop;
            finallyHandler = emitJump(OP_TRY_BEGIN);
            current->tryDepth++;
        }
    }
    int catchHandler = -1;
    if (hasCatch) {
        catchHandler = emitJump(OP_TRY_BEGIN);
        current->tryDepth++;
    }

    beginScope();
    consume(TOKEN_LEFT_BRACE, "Harapkan '{' setelah 'coba'.");
    block();
    endScope();

    if (hasCatch) {
        emitByte(OP_TRY_END);               // normal path: unregister handler
        current->tryDepth--;
        int skipCatch = emitJump(OP_JUMP);
        patchJump(catchHandler);

        // One 'coba' may have several 'kecuali' clauses, tried in order. Each
        // typed clause hands back its "no match" jump, which lands on the next
        // clause; the last unmatched jump falls through to the rethrow.
        int doneJumps[UINT8_COUNT];
        int doneCount = 0;
        int pending = -1;
        bool sawUntyped = false;
        do {
            if (pending != -1) {
                patchJump(pending);
                emitByte(OP_POP);           // drop the failed OP_IS_INSTANCE result
            }
            if (sawUntyped) {
                semanticError("'kecuali' tanpa tipe harus menjadi yang terakhir.");
            }
            bool bare = false;
            pending = catchClause(doneJumps, &doneCount, &bare);
            // `kecuali X { ... }` may or may not be typed — only run time
            // knows — so it keeps the stricter rule and must come last.
            if (pending == -1 || bare) sawUntyped = true;
        } while (check(TOKEN_KECUALI));

        if (pending != -1) {
            // No clause matched: re-raise the original error.
            patchJump(pending);
            emitByte(OP_POP);
            emitByte(OP_THROW);
        }
        for (int i = 0; i < doneCount; i++) patchJump(doneJumps[i]);
        patchJump(skipCatch);
    }

    if (hasFinally) {
        emitByte(OP_TRY_END);
        current->tryDepth--;
        current->finallyCount--;
        emitByte(OP_KOSONG);                // no error...
        emitByte(OP_SALAH);                 // ...so do not rethrow
        int toFinally = emitJump(OP_JUMP);
        patchJump(finallyHandler);          // error path: VM pushed the value
        emitByte(OP_BENAR);                 // rethrow after the finally block
        patchJump(toFinally);
        finallyClause();
    }
}

// dengan <ekspresi> [sebagai <nama>] { ... }
//
// Compiled with the same shape as 'coba ... akhirnya': an OP_TRY_BEGIN whose
// handler routes errors through the _keluar_ call and rethrows. Registering a
// FinallyRegion means 'kembalikan', 'hentikan', and 'lanjut' already run
// _keluar_() on their way out, via exitTryBlocks() -- no changes needed in any
// of those three statements.
static void withStatement() {
    beginScope();

    // The manager lives in a hidden local so the handler can reach it after
    // the body's own locals have been discarded.
    expression();
    int managerSlot = addSyntheticLocal(" dengan mgr");

    Token masuk = syntheticToken("_masuk_");
    emitBytes(OP_GET_LOCAL, (uint8_t)managerSlot);
    emitBytes(OP_INVOKE, identifierConstant(&masuk));
    emitByte(0);                            // argCount

    // What _masuk_() returned is what 'sebagai' binds -- not the manager.
    if (match(TOKEN_SEBAGAI)) {
        consume(TOKEN_IDENTIFIER, "Harapkan nama variabel setelah 'sebagai'.");
        addLocal(parser.previous);
        markInitialized();
    } else {
        addSyntheticLocal(" dengan nilai");
    }

    if (current->finallyCount == FINALLY_MAX) {
        error("Terlalu banyak blok 'dengan'/'akhirnya' bersarang.");
        endScope();
        return;
    }
    FinallyRegion* region = &current->finallies[current->finallyCount++];
    region->kind = FINALLY_EXIT_CALL;
    region->tryDepth = current->tryDepth;
    region->managerSlot = (uint8_t)managerSlot;
    region->loop = currentLoop;

    int handler = emitJump(OP_TRY_BEGIN);
    current->tryDepth++;

    consume(TOKEN_LEFT_BRACE, "Harapkan '{' setelah ekspresi 'dengan'.");
    beginScope();
    block();
    endScope();

    emitByte(OP_TRY_END);
    current->tryDepth--;
    current->finallyCount--;

    // Normal path: no error to rethrow. Error path: the VM pushed the value.
    emitByte(OP_KOSONG);
    emitByte(OP_SALAH);
    int toExit = emitJump(OP_JUMP);
    patchJump(handler);
    emitByte(OP_BENAR);
    patchJump(toExit);

    // Mirrors finallyClause(): run the cleanup, then rethrow if we got here
    // through the error path.
    beginScope();
    addLocal(syntheticToken(" galat"));
    markInitialized();
    addLocal(syntheticToken(" lempar"));
    markInitialized();
    uint8_t errorSlot = (uint8_t)(current->localCount - 2);
    uint8_t rethrowSlot = (uint8_t)(current->localCount - 1);

    Token keluar = syntheticToken("_keluar_");
    emitBytes(OP_GET_LOCAL, (uint8_t)managerSlot);
    emitBytes(OP_INVOKE, identifierConstant(&keluar));
    emitByte(0);
    emitByte(OP_POP);

    emitBytes(OP_GET_LOCAL, rethrowSlot);
    int noRethrow = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    emitBytes(OP_GET_LOCAL, errorSlot);
    emitByte(OP_THROW);
    patchJump(noRethrow);
    emitByte(OP_POP);
    endScope();     // the two hidden locals

    endScope();     // manager, bound value
}

// hasilkan <ekspresi> -- suspend this generator and hand the value to the
// caller. Only valid inside a function; the enclosing function is flagged a
// generator, so calling it builds an ObjGenerator instead of running the body.
static void yieldStatement() {
    if (current->type == TYPE_SCRIPT) {
        error("'hasilkan' hanya bisa dipakai di dalam fungsi.");
        return;
    }
    if (current->type == TYPE_INITIALIZER) {
        error("'init' tidak bisa menjadi generator ('hasilkan' tidak diizinkan).");
        return;
    }
    // The frame is snapshotted wholesale at OP_YIELD, but an inlined 'akhirnya'
    // would have to run on a frame that is about to vanish. Reject it rather
    // than silently skipping the cleanup.
    if (current->finallyCount > 0) {
        error("'hasilkan' belum didukung di dalam blok 'coba' yang punya 'akhirnya'.");
        return;
    }
    current->isGenerator = true;

    if (check(TOKEN_EOF) || check(TOKEN_RIGHT_BRACE) ||
        check(TOKEN_SEMICOLON) || check(TOKEN_NEWLINE)) {
        emitByte(OP_KOSONG);          // 'hasilkan' telanjang menghasilkan kosong
    } else {
        bool saved = allowTupleRHS;
        allowTupleRHS = true;
        expression();
        maybeTupleTail();             // hasilkan a, b
        allowTupleRHS = saved;
    }
    emitByte(OP_YIELD);
}

// cocokkan nilai { kasus pola { ... } ... }
//
// Lowered to ordinary conditional bytecode -- there is no pattern object and
// no VM pattern engine. The subject lives in a hidden local. Every pattern
// compiles to code leaving exactly ONE boolean on the stack, so the case
// dispatch below is uniform.
//
// Supported patterns (deliberately small):
//   literal      kasus 0 / kasus "teks" / kasus benar / kasus kosong
//   wildcard     kasus _
//   capture      kasus nama              (binds, always matches)
//   kelas        kasus Kelas()           (instance test)
//   alternatif   kasus 1 atau 2
//   penjaga      kasus pola jika syarat
//
// ponytail: no sequence/mapping destructuring yet. Add a recursive pattern
// compiler when a real case needs [a, b] or {"k": v}.

// One alternative. Emits code leaving a single boolean on the stack.
static void matchPatternTest(int subjectSlot) {
    if (check(TOKEN_IDENTIFIER) && checkNext(TOKEN_LEFT_PAREN)) {
        // kasus Kelas() -- instance test. OP_IS_INSTANCE wants [nilai, kelas].
        emitBytes(OP_GET_LOCAL, (uint8_t)subjectSlot);
        namedVariable(parser.current, false);
        advance();                 // class name
        advance();                 // '('
        consume(TOKEN_RIGHT_PAREN, "Harapkan ')' pada pola kelas.");
        emitByte(OP_IS_INSTANCE);
        return;
    }
    // Literal / expression pattern: subjek == <ekspresi>.
    // PREC_AND, not PREC_OR: 'atau' separates alternatives here, so the
    // expression parser must not swallow it as a boolean operator.
    emitBytes(OP_GET_LOCAL, (uint8_t)subjectSlot);
    parsePrecedence(PREC_AND);
    emitByte(OP_EQUAL);
}

static void matchStatement() {
    beginScope();
    expression();
    int subjectSlot = addSyntheticLocal("  cocok nilai");

    consume(TOKEN_LEFT_BRACE, "Harapkan '{' setelah nilai pada 'cocokkan'.");
    skipNewlines();

    int endJumps[MATCH_CASE_MAX];
    int endCount = 0;
    bool sawWildcard = false;

    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        consume(TOKEN_KASUS, "Harapkan 'kasus' di dalam 'cocokkan'.");
        if (sawWildcard) error("'kasus _' harus menjadi kasus terakhir.");

        beginScope();              // this case's bindings

        bool alwaysMatches = false;
        Token captureName = {0};
        bool hasCapture = false;

        if (check(TOKEN_IDENTIFIER) && parser.current.length == 1 &&
            parser.current.start[0] == '_') {
            advance();
            alwaysMatches = true;
            sawWildcard = true;
        } else if (check(TOKEN_IDENTIFIER) && !checkNext(TOKEN_LEFT_PAREN) &&
                   !checkNext(TOKEN_DOT)) {
            // A bare name binds the subject, as in Python. Use Kelas() for an
            // instance test and Modul.NAMA for a literal comparison.
            captureName = parser.current;
            hasCapture = true;
            alwaysMatches = true;
            advance();
        }

        // Test chain. Each alternative leaves one boolean; OR them together
        // with the existing short-circuit shape (JUMP_IF_FALSE + POP).
        int orJumps[MATCH_ALT_MAX];
        int orCount = 0;
        if (!alwaysMatches) {
            matchPatternTest(subjectSlot);
            while (check(TOKEN_ATAU)) {
                if (orCount == MATCH_ALT_MAX) {
                    error("Terlalu banyak alternatif pada satu 'kasus'.");
                    break;
                }
                // Short-circuit OR, same shape as or_(): on false fall through
                // to the next alternative, on true jump out keeping the true.
                int isFalse = emitJump(OP_JUMP_IF_FALSE);
                orJumps[orCount++] = emitJump(OP_JUMP);   // true: done
                patchJump(isFalse);
                emitByte(OP_POP);                          // discard the false
                advance();                                 // consume 'atau'
                matchPatternTest(subjectSlot);
            }
            for (int i = 0; i < orCount; i++) patchJump(orJumps[i]);
        }

        int patternFail = -1;
        if (!alwaysMatches) {
            patternFail = emitJump(OP_JUMP_IF_FALSE);
            emitByte(OP_POP);              // the true boolean
        }
        if (hasCapture) {
            emitBytes(OP_GET_LOCAL, (uint8_t)subjectSlot);
            addLocal(captureName);
            markInitialized();
        }

        int guardFail = -1;
        if (match(TOKEN_JIKA)) {
            parsePrecedence(PREC_OR);
            guardFail = emitJump(OP_JUMP_IF_FALSE);
            emitByte(OP_POP);
        }

        consume(TOKEN_LEFT_BRACE, "Harapkan '{' setelah pola 'kasus'.");
        beginScope();
        block();
        endScope();

        // The matched case must run its OWN binding cleanup before jumping to
        // the end; jumping first would skip the pops and leave the capture (and
        // the subject) stranded on the stack for the next statement.
        int bindingPops = 0;
        if (hasCapture) bindingPops = 1;
        for (int i = 0; i < bindingPops; i++) emitByte(OP_POP);

        if (endCount == MATCH_CASE_MAX) {
            error("Terlalu banyak 'kasus' dalam satu 'cocokkan'.");
        } else {
            endJumps[endCount++] = emitJump(OP_JUMP);
        }

        if (guardFail != -1) {
            patchJump(guardFail);
            emitByte(OP_POP);              // the false guard
            // the guard failed AFTER the capture was bound: drop it too
            for (int i = 0; i < bindingPops; i++) emitByte(OP_POP);
        }
        if (patternFail != -1) {
            patchJump(patternFail);
            emitByte(OP_POP);              // the false pattern result
        }
        // The compiler's own local bookkeeping is closed without emitting pops,
        // since both paths above already balanced the stack by hand.
        current->scopeDepth--;
        while (current->localCount > 0 &&
               current->locals[current->localCount - 1].depth > current->scopeDepth) {
            current->localCount--;
        }
        skipNewlines();
    }

    consume(TOKEN_RIGHT_BRACE, "Harapkan '}' setelah 'cocokkan'.");
    for (int i = 0; i < endCount; i++) patchJump(endJumps[i]);
    endScope();                            // drop the subject
}

static void returnStatement() {
    if (current->type == TYPE_SCRIPT) {
        error("Tidak dapat 'kembalikan' dari kode tingkat atas.");
    }

    // OP_RETURN drops the frame's 'coba' handlers by itself; only enclosing
    // 'akhirnya' blocks need code here (exitTryBlocks).
    bool hasFinally = current->finallyCount > 0;
    if (check(TOKEN_EOF) || check(TOKEN_RIGHT_BRACE) || check(TOKEN_SEMICOLON) || check(TOKEN_NEWLINE)) {
        if (hasFinally) exitTryBlocks(current->tryDepth, 0);
        emitReturn();
        return;
    }

    if (current->type == TYPE_INITIALIZER) {
        error("Tidak dapat mengembalikan nilai dari 'init'.");
    }
    if (!hasFinally) {
        bool saved = allowTupleRHS;
        allowTupleRHS = true;
        expression();
        maybeTupleTail();           // kembalikan a, b
        allowTupleRHS = saved;
        emitByte(OP_RETURN);
        return;
    }

    // Python evaluates the return value BEFORE the finally blocks run. Keep
    // it in a hidden local so the inlined blocks' own locals get the right
    // stack slots, then return it.
    beginScope();
    {
        bool saved = allowTupleRHS;
        allowTupleRHS = true;
        expression();
        maybeTupleTail();           // kembalikan a, b (dengan 'akhirnya')
        allowTupleRHS = saved;
    }
    addLocal(syntheticToken(" kembali"));
    markInitialized();
    uint8_t slot = (uint8_t)(current->localCount - 1);
    exitTryBlocks(current->tryDepth, 0);
    emitBytes(OP_GET_LOCAL, slot);
    emitByte(OP_RETURN);
    // Code after OP_RETURN never runs: drop the local without emitting a pop.
    current->localCount--;
    current->scopeDepth--;
}

static void whileStatement() {
    int loopStart = currentChunk()->count;
    expression();
    consume(TOKEN_LEFT_BRACE, "Harapkan '{' setelah kondisi 'selagi'.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);

    LoopCtx loop;
    beginLoop(&loop, loopStart);
    beginScope();
    block();
    endScope();

    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP);

    endLoop();  // break jumps land here, past the condition pop
}

static void synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
            case TOKEN_KELAS:
            case TOKEN_FUNGSI:
            case TOKEN_VARIABEL:
            case TOKEN_UNTUK:
            case TOKEN_JIKA:
            case TOKEN_SELAGI:
            case TOKEN_KEMBALIKAN:
                return;

            default:
                ; // Do nothing.
        }

        advance();
    }
}

static void declaration() {
    while (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) { }
    if (check(TOKEN_EOF) || check(TOKEN_RIGHT_BRACE)) return;

    if (check(TOKEN_AT)) {
        // Decorators are pushed BEFORE the name is declared: the decorated
        // result ends up in the first decorator's stack slot, which is exactly
        // the slot parseVariable() hands the new variable.
        int decorators = decoratorList();
        if (check(TOKEN_FUNGSI) && !checkNext(TOKEN_LEFT_PAREN)) {
            advance();
            funDeclarationDecorated(decorators);
        } else if (check(TOKEN_KELAS)) {
            error("Dekorator pada 'kelas' belum didukung.");
        } else {
            error("Dekorator harus diikuti definisi 'fungsi'.");
        }
    } else if (check(TOKEN_FUNGSI) && !checkNext(TOKEN_LEFT_PAREN)) {
        advance();
        funDeclaration();
    } else if (match(TOKEN_KELAS)) {
        classDeclaration();
    } else if (match(TOKEN_VARIABEL)) {
        varDeclaration();
    } else {
        statement();
    }
    // `;` and newline are interchangeable statement separators outside grouping.
    while (match(TOKEN_SEMICOLON) || match(TOKEN_NEWLINE)) { }

    if (parser.panicMode) synchronize();
}

static void scopeDeclaration(bool isGlobal) {
    if (current->type == TYPE_SCRIPT) {
        semanticError(isGlobal ? "'global' hanya boleh di dalam fungsi."
                               : "'nonlokal' hanya boleh di dalam fungsi.");
    }
    do {
        consume(TOKEN_IDENTIFIER, "Harapkan nama setelah deklarasi scope.");
        Token name = parser.previous;
        Token* names = isGlobal ? current->globals : current->nonlocals;
        int* count = isGlobal ? &current->globalCount : &current->nonlocalCount;
        if (scopeNameDeclared(names, *count, &name)) {
            // Already recorded by prescanFunctionBody; nothing to do.
        } else if (*count == UINT8_COUNT) {
            semanticError("Terlalu banyak deklarasi scope.");
        } else {
            names[(*count)++] = name;
        }
    } while (match(TOKEN_COMMA));
}

// Delete a variable binding: global (removed from the table), local or upvalue
// (the slot becomes VAL_TAKTERISI, so reading it is an error again).
static void deleteVariable(Token name) {
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
        emitBytes(OP_DELETE_LOCAL, (uint8_t)arg);
        return;
    }
    int upvalue = resolveUpvalue(current, &name);
    if (upvalue != -1) {
        emitBytes(OP_DELETE_UPVALUE, (uint8_t)upvalue);
        return;
    }
    emitBytes(OP_DELETE_GLOBAL, identifierConstant(&name));
}

// The base (list/kamus/instance) is already on the stack.
static void buangFromBase() {
    if (match(TOKEN_LEFT_BRACKET)) {
        expression();
        consume(TOKEN_RIGHT_BRACKET, "Harapkan ']' setelah indeks pada 'buang'.");
        emitByte(OP_INDEX_DELETE);
    } else if (match(TOKEN_DOT)) {
        consume(TOKEN_IDENTIFIER, "Harapkan nama atribut setelah '.' pada 'buang'.");
        emitBytes(OP_DELETE_PROPERTY, identifierConstant(&parser.previous));
    } else {
        errorAtCurrent("Harapkan '[' atau '.' setelah nama pada 'buang'.");
    }
}

// buang nama | buang wadah[indeks] | buang objek.atribut
static void buangStatement() {
    if (match(TOKEN_DIRI)) {
        namedVariable(syntheticToken("diri"), false);
        buangFromBase();
        return;
    }
    if (!check(TOKEN_IDENTIFIER)) {
        errorAtCurrent("Harapkan nama, elemen, atau atribut setelah 'buang'.");
        return;
    }

    // A bare name deletes the binding itself; with '[' or '.' the name is the
    // container/instance instead.
    if (!checkNext(TOKEN_LEFT_BRACKET) && !checkNext(TOKEN_DOT)) {
        Token name = parser.current;
        advance();
        deleteVariable(name);
        return;
    }

    Token name = parser.current;
    advance();
    namedVariable(name, false);
    buangFromBase();
}

// Pops the value on top of the stack into `name`, resolving it like a plain
// `name = ...`: local, upvalue, else global (define-or-update). Used by
// a, b = 1, 2 / a, b = urutan and by the `untuk` loop variable (K21).
static void assignToName(Token name) {
    // `global x` wins over an enclosing function's local of the same name.
    bool forceGlobal = scopeNameDeclared(current->globals, current->globalCount, &name);
    int arg = forceGlobal ? -1 : resolveLocal(current, &name);
    if (arg != -1) {
        emitBytes(OP_SET_LOCAL, (uint8_t)arg);
        emitByte(OP_POP);
        return;
    }
    int upvalue = forceGlobal ? -1 : resolveUpvalue(current, &name);
    if (upvalue != -1) {
        emitBytes(OP_SET_UPVALUE, (uint8_t)upvalue);
        emitByte(OP_POP);
        return;
    }
    // Same define-or-update semantics as implicit assignment.
    emitBytes(OP_DEFINE_GLOBAL, identifierConstant(&name));
}

static bool looksLikeUnpack() {
    if (!check(TOKEN_IDENTIFIER)) return false;
    ScannerState saved = saveScanner();
    Token savedCurrent = parser.current;
    Token savedPrevious = parser.previous;
    int count = 0;
    bool matched = false;
    for (;;) {
        if (!check(TOKEN_IDENTIFIER)) break;
        count++;
        advance();
        if (check(TOKEN_COMMA)) {
            advance();
            continue;
        }
        if (check(TOKEN_EQUAL) && count >= 2) matched = true;
        break;
    }
    restoreScanner(saved);
    parser.current = savedCurrent;
    parser.previous = savedPrevious;
    return matched;
}

// `x = 1, 2` / `kembalikan 1, 2`. Call right after the first expression was
// compiled; emits OP_BUILD_TUPLE when a comma really is a tupel builder.
static void maybeTupleTail() {
    if (!allowTupleRHS) return;
    if (!check(TOKEN_COMMA) || scannerGroupingDepth() != 0) return;

    int elements = 1;
    while (match(TOKEN_COMMA)) {
        // `x = 1,` -- a trailing comma is a one-element tupel, as in Python.
        if (check(TOKEN_NEWLINE) || check(TOKEN_EOF) || check(TOKEN_SEMICOLON) ||
            check(TOKEN_RIGHT_BRACE)) {
            break;
        }
        if (elements == UINT8_COUNT) {
            error("Terlalu banyak elemen pada tupel.");
            return;
        }
        bool saved = allowTupleRHS;
        allowTupleRHS = false;      // nested commas belong to this tupel
        expression();
        allowTupleRHS = saved;
        elements++;
    }
    emitBytes(OP_BUILD_TUPLE, (uint8_t)elements);
}

static void unpackStatement() {
    Token names[UINT8_COUNT];
    int count = 0;
    do {
        consume(TOKEN_IDENTIFIER, "Harapkan nama variabel.");
        if (count == UINT8_COUNT) {
            error("Terlalu banyak nama pada penugasan berganda.");
            return;
        }
        names[count++] = parser.previous;
    } while (match(TOKEN_COMMA));

    consume(TOKEN_EQUAL, "Harapkan '=' setelah daftar nama.");

    // `a, b = 1, 2` builds a list; `a, b = urutan` unpacks the sequence itself.
    expression();
    if (match(TOKEN_COMMA)) {
        int elements = 1;
        do {
            if (elements == UINT8_COUNT) {
                error("Terlalu banyak nilai pada penugasan berganda.");
                return;
            }
            expression();
            elements++;
        } while (match(TOKEN_COMMA));
        emitBytes(OP_BUILD_LIST, (uint8_t)elements);
    }

    emitBytes(OP_UNPACK, (uint8_t)count);
    for (int i = 0; i < count; i++) assignToName(names[i]);
}

static void statement() {
    if (match(TOKEN_GLOBAL)) {
        scopeDeclaration(true);
    } else if (match(TOKEN_NONLOKAL)) {
        scopeDeclaration(false);
    } else if (match(TOKEN_LEWATI)) {
        // 'lewati' (Python's pass): an explicit no-op statement.
    } else if (match(TOKEN_UNTUK)) {
        forStatement();
    } else if (match(TOKEN_JIKA)) {
        ifStatement();
    } else if (match(TOKEN_KEMBALIKAN)) {
        returnStatement();
    } else if (match(TOKEN_HASILKAN)) {
        yieldStatement();
    } else if (match(TOKEN_COCOKKAN)) {
        matchStatement();
    } else if (match(TOKEN_SELAGI)) {
        whileStatement();
    } else if (match(TOKEN_HENTIKAN)) {
        breakStatement();
    } else if (match(TOKEN_LANJUT)) {
        continueStatement();
    } else if (match(TOKEN_BUANG)) {
        buangStatement();
    } else if (match(TOKEN_COBA)) {
        tryStatement();
    } else if (match(TOKEN_DENGAN)) {
        withStatement();
    } else if (match(TOKEN_NAIKKAN)) {
        raiseStatement();
    } else if (match(TOKEN_PASTIKAN)) {
        assertStatement();
    } else if (match(TOKEN_DARI)) {
        fromImportStatement();
    } else if (match(TOKEN_IMPOR)) {
        importStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else if (looksLikeUnpack()) {
        unpackStatement();
    } else {
        expressionStatement();
    }
    
    // Consume optional newlines after statements
}

ObjFunction* compileInModule(const char* source, ObjModule* module) {
    initScanner(source);
    // Kompilasi sebelumnya bisa berhenti di tengah karena galat, meninggalkan
    // nestingDepth bukan nol. Tanpa reset, REPL atau impor berikutnya mewarisi
    // hitungan itu dan akhirnya menolak kode yang sah.
    nestingDepth = 0;
    // Keep the source alive as a GC object: module sources are freed by the
    // caller right after compile(), but runtime errors need them later.
    compilingSource = copyString(source, (int)strlen(source));
    compilingModule = module;
    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    parser.hadError = false;
    parser.panicMode = false;
    suppressFurtherErrors = false;
    callArgStart = NULL;

    advance();

    while (!check(TOKEN_EOF)) {
        // declaration() returns without consuming when it meets '}' (block()
        // relies on that to end a block). At top level there is no block to
        // close, so without this guard the loop never advances and the
        // compiler hangs instead of reporting the error.
        if (check(TOKEN_RIGHT_BRACE)) {
            errorAtCurrent("'}' tidak punya '{' yang cocok.");
            advance();
            continue;
        }
        declaration();
    }

    ObjFunction* function = endCompiler();
    compilingSource = NULL;
    compilingModule = NULL;
    return parser.hadError ? NULL : function;
}

// Mark every function currently being compiled so the GC does not free
// them mid-compilation (a collection can be triggered by allocations in
// copyString/makeConstant while parsing).
void markCompilerRoots() {
    // Not yet reachable from any function while the first chunk is still
    // being built, and allocations during parsing can trigger a GC.
    markObject((Obj*)compilingSource);
    markObject((Obj*)compilingModule);
    Compiler* compiler = current;
    while (compiler != NULL) {
        markObject((Obj*)compiler->function);
        compiler = compiler->enclosing;
    }
}

ObjFunction* compile(const char* source) {
    return compileInModule(source, NULL);
}
