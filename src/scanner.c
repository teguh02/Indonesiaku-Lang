#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
    int groupingDepth;
    // Start of the buffer being scanned, kept so error reporting can walk
    // back to the beginning of a line and compute a column.
    const char* source;
    // Its NUL. Cached: emitByte checks it for every byte emitted, and
    // rescanning the whole source each time would make compilation O(n^2).
    const char* sourceEnd;
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.source = source;
    scanner.sourceEnd = source + strlen(source);
    scanner.line = 1;
    scanner.groupingDepth = 0;
}

ScannerState saveScanner() {
    ScannerState state = {scanner.start, scanner.current, scanner.line,
                          scanner.groupingDepth, scanner.source,
                          scanner.sourceEnd};
    return state;
}

void initScannerAt(const char* source, int line) {
    scanner.start = source;
    scanner.current = source;
    scanner.source = source;
    scanner.sourceEnd = source + strlen(source);
    scanner.line = line;
    scanner.groupingDepth = 0;
}

void restoreScanner(ScannerState state) {
    scanner.start = state.start;
    scanner.current = state.current;
    scanner.line = state.line;
    scanner.groupingDepth = state.groupingDepth;
    scanner.source = state.source;
    scanner.sourceEnd = state.sourceEnd;
}

static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

static bool isHexDigit(char c) {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static bool isAtEnd() {
    return *scanner.current == '\0';
}

static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

static char peek() {
    return *scanner.current;
}

static char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

static bool match(char expected) {
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

static Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token errorToken(const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;
}

// See scannerKeepComments(). Static, process-wide, and deliberately not part
// of ScannerState: nothing nests a scanner while it is on.
static bool keepComments = false;

void scannerKeepComments(bool enabled) { keepComments = enabled; }

static void skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                if (scanner.groupingDepth == 0) return;
                scanner.line++;
                advance();
                break;
            case '#':
                // With keepComments on, stop here and let scanToken() emit a
                // TOKEN_KOMENTAR covering the comment text.
                if (keepComments) return;
                // A comment goes until the end of the line.
                while (peek() != '\n' && !isAtEnd()) advance();
                break;
            default:
                return;
        }
    }
}

static TokenType checkKeyword(int start, int length, const char* rest, TokenType type) {
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
    switch (scanner.start[0]) {
        case 'a':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'k': return checkKeyword(2, 6, "hirnya", TOKEN_AKHIRNYA);
                    case 't': return checkKeyword(2, 2, "au", TOKEN_ATAU);
                    case 'd': return checkKeyword(1, 5, "dalah", TOKEN_ADALAH);
                }
            }
            break;
        case 'b':
            // benar / bukan / buang are all exactly five characters.
            if (scanner.current - scanner.start == 5) {
                if (memcmp(scanner.start, "benar", 5) == 0) return TOKEN_BENAR;
                if (memcmp(scanner.start, "bukan", 5) == 0) return TOKEN_BUKAN;
                if (memcmp(scanner.start, "buang", 5) == 0) return TOKEN_BUANG;
            }
            break;
        case 'c':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    // 'cetak' is a plain identifier now (K12): it resolves to
                    // the native, so it can be passed around and shadowed.
                    case 'o':
                        if (scanner.current - scanner.start > 2) {
                            switch (scanner.start[2]) {
                                case 'b': return checkKeyword(3, 1, "a", TOKEN_COBA);
                                case 'c': return checkKeyword(3, 5, "okkan", TOKEN_COCOKKAN);
                            }
                        }
                        break;
                }
            }
            break;
        case 'd':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a':
                        if (scanner.current - scanner.start > 2) {
                            switch (scanner.start[2]) {
                                case 'l': return checkKeyword(3, 2, "am", TOKEN_DALAM);
                                case 'n': return checkKeyword(3, 0, "", TOKEN_DAN);
                                case 'r': return checkKeyword(3, 1, "i", TOKEN_DARI);
                            }
                        }
                        break;
                    case 'e': return checkKeyword(2, 4, "ngan", TOKEN_DENGAN);
                    case 'i': return checkKeyword(2, 2, "ri", TOKEN_DIRI);
                }
            }
            break;
        case 'f':
            return checkKeyword(1, 5, "ungsi", TOKEN_FUNGSI);
        case 'h':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'e': return checkKeyword(2, 6, "ntikan", TOKEN_HENTIKAN);
                    case 'a': return checkKeyword(2, 6, "silkan", TOKEN_HASILKAN);
                }
            }
            return TOKEN_IDENTIFIER;

        case 'i':
            return checkKeyword(1, 4, "mpor", TOKEN_IMPOR);
        case 'j':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'i':
                        if (scanner.current - scanner.start == 4) {
                            return TOKEN_JIKA;
                        }
                        return checkKeyword(2, 7, "ka_lain", TOKEN_JIKA_LAIN);
                }
            }
            break;
        case 'k':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'e':
                        if (scanner.current - scanner.start > 2) {
                            switch (scanner.start[2]) {
                                case 'c': return checkKeyword(3, 4, "uali", TOKEN_KECUALI);
                                case 'l': return checkKeyword(3, 2, "as", TOKEN_KELAS);
                                case 'm': return checkKeyword(3, 7, "balikan", TOKEN_KEMBALIKAN);
                            }
                        }
                        break;
                    case 'o': return checkKeyword(2, 4, "song", TOKEN_KOSONG);
                    case 'a': return checkKeyword(2, 3, "sus", TOKEN_KASUS);
                }
            }
            break;
        case 'l':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 4, "njut", TOKEN_LANJUT);
                    case 'e': return checkKeyword(2, 4, "wati", TOKEN_LEWATI);
                }
            }
            break;
        case 'g':
            return checkKeyword(1, 5, "lobal", TOKEN_GLOBAL);
        case 'p':
            return checkKeyword(1, 7, "astikan", TOKEN_PASTIKAN);
        case 'n':
            if (scanner.current - scanner.start > 1 && scanner.start[1] == 'o') {
                return checkKeyword(2, 6, "nlokal", TOKEN_NONLOKAL);
            }
            return checkKeyword(1, 6, "aikkan", TOKEN_NAIKKAN);
        case 's':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lah", TOKEN_SALAH);
                    case 'e':
                        if (scanner.current - scanner.start > 2) {
                            switch (scanner.start[2]) {
                                case 'b': return checkKeyword(3, 4, "agai", TOKEN_SEBAGAI);
                                case 'l':
                                    if (scanner.current - scanner.start == 6 &&
                                        scanner.start[3] == 'a') {
                                        // Disambiguate 'selagi' vs 'selain'
                                        if (scanner.start[4] == 'g' && scanner.start[5] == 'i') return TOKEN_SELAGI;
                                        if (scanner.start[4] == 'i' && scanner.start[5] == 'n') return TOKEN_SELAIN;
                                    }
                                    break;
                            }
                        }
                        break;
                    case 'u': return checkKeyword(2, 3, "per", TOKEN_SUPER);
                }
            }
            break;
        case 't':
            return checkKeyword(1, 4, "idak", TOKEN_TIDAK);
        case 'u':
            return checkKeyword(1, 4, "ntuk", TOKEN_UNTUK);
        case 'v':
            return checkKeyword(1, 7, "ariabel", TOKEN_VARIABEL);
    }

    return TOKEN_IDENTIFIER;
}

static Token identifier() {
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}

static Token number() {
    // 0x / 0b / 0o prefixes. Only reachable after a leading digit, so an
    // identifier like `b1010` is unaffected.
    if (scanner.current[-1] == '0' && !isAtEnd()) {
        char base = peek();
        if (base == 'x' || base == 'X' || base == 'b' || base == 'B' ||
            base == 'o' || base == 'O') {
            advance();
            while (isHexDigit(peek()) || peek() == '_') advance();
            return makeToken(TOKEN_NUMBER);
        }
    }

    while (isDigit(peek()) || peek() == '_') advance();

    // Look for a fractional part.
    if (peek() == '.' && isDigit(peekNext())) {
        // Consume the ".".
        advance();

        while (isDigit(peek()) || peek() == '_') advance();
    }

    // Exponent: 1e3, 1.5e-3, 2E+8. strtod parses these itself.
    if (peek() == 'e' || peek() == 'E') {
        char next = peekNext();
        // Guard the third char: peekNext() may already be the terminating NUL.
        char after = next == '\0' ? '\0' : scanner.current[2];
        if (isDigit(next) || ((next == '+' || next == '-') && isDigit(after))) {
            advance();                       // 'e'
            if (peek() == '+' || peek() == '-') advance();
            while (isDigit(peek()) || peek() == '_') advance();
        }
    }

    return makeToken(TOKEN_NUMBER);
}

// f"..." may contain quotes inside {...}: f"{d["a"]}". Track brace depth so
// the literal does not end at the first inner quote.
static Token fstringBody() {
    int depth = 0;
    while (!isAtEnd()) {
        char c = peek();
        if (c == '\\' && peekNext() != '\0') { advance(); advance(); continue; }
        if (c == '\n') scanner.line++;
        if (c == '{') {
            if (peekNext() == '{') { advance(); advance(); continue; }
            depth++;
        } else if (c == '}') {
            if (depth == 0 && peekNext() == '}') { advance(); advance(); continue; }
            if (depth > 0) depth--;
        } else if (c == '"' && depth == 0) {
            advance();
            return makeToken(TOKEN_FSTRING);
        }
        advance();
    }
    return errorToken("String tidak ditutup.");
}

static Token string() {
    // Triple-quoted: """ ... """ keeps newlines verbatim.
    if (peek() == '"' && peekNext() == '"') {
        advance();
        advance();
        while (!isAtEnd()) {
            // Guard the third char: peekNext() may already be the NUL.
            if (peek() == '"' && peekNext() == '"' &&
                peekNext() != '\0' && scanner.current[2] == '"') {
                advance();
                advance();
                advance();
                return makeToken(TOKEN_STRING);
            }
            if (peek() == '\n') scanner.line++;
            if (peek() == '\\' && peekNext() != '\0') advance();
            advance();
        }
        return errorToken("String tiga kutip tidak ditutup.");
    }

    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') scanner.line++;
        // Skip escaped characters (e.g. \" \\) so they don't end the string.
        if (peek() == '\\' && peekNext() != '\0') {
            advance();
        }
        advance();
    }

    if (isAtEnd()) return errorToken("String tidak ditutup.");

    // The closing quote.
    advance();
    return makeToken(TOKEN_STRING);
}

Token scanToken() {
    skipWhitespace();
    scanner.start = scanner.current;

    if (isAtEnd()) return makeToken(TOKEN_EOF);

    // Comment as a token (formatter only; see scannerKeepComments).
    if (keepComments && peek() == '#') {
        while (peek() != '\n' && !isAtEnd()) advance();
        return makeToken(TOKEN_KOMENTAR);
    }

    char c = advance();
    // f"..." — must be checked before identifier(), which would swallow the f.
    if (c == 'f' && peek() == '"') {
        advance();                           // the opening quote
        return fstringBody();
    }
    if (isAlpha(c)) return identifier();
    if (isDigit(c)) return number();

    switch (c) {
        case '(': scanner.groupingDepth++; return makeToken(TOKEN_LEFT_PAREN);
        case ')': if (scanner.groupingDepth > 0) scanner.groupingDepth--; return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case '[': scanner.groupingDepth++; return makeToken(TOKEN_LEFT_BRACKET);
        case ']': if (scanner.groupingDepth > 0) scanner.groupingDepth--; return makeToken(TOKEN_RIGHT_BRACKET);
        case ':': return makeToken(TOKEN_COLON);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case '\n': scanner.line++; return makeToken(TOKEN_NEWLINE);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '@': return makeToken(TOKEN_AT);
        case '-':
            // '->' (anotasi tipe kembali) dicek SEBELUM '-=' dan '-' tunggal.
            // Keduanya saling eksklusif, tapi urutan ini membuat niatnya jelas.
            if (match('>')) return makeToken(TOKEN_PANAH);
            return makeToken(
                match('=') ? TOKEN_MINUS_EQUAL : TOKEN_MINUS);
        case '+':
            return makeToken(
                match('=') ? TOKEN_PLUS_EQUAL : TOKEN_PLUS);
        case '/':
            if (match('/')) {
                return makeToken(
                    match('=') ? TOKEN_SLASH_SLASH_EQUAL : TOKEN_SLASH_SLASH);
            } else if (match('=')) {
                return makeToken(TOKEN_SLASH_EQUAL);
            }
            return makeToken(TOKEN_SLASH);
        case '*':
            if (match('*')) {
                return makeToken(
                    match('=') ? TOKEN_POWER_EQUAL : TOKEN_POWER);
            } else if (match('=')) {
                return makeToken(TOKEN_STAR_EQUAL);
            }
            return makeToken(TOKEN_STAR);
        case '%':
            return makeToken(
                match('=') ? TOKEN_PERCENT_EQUAL : TOKEN_PERCENT);
        case '!':
            return makeToken(
                match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(
                match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            if (match('<')) {
                return makeToken(
                    match('=') ? TOKEN_LESS_LESS_EQUAL : TOKEN_LESS_LESS);
            }
            return makeToken(
                match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            if (match('>')) {
                return makeToken(
                    match('=') ? TOKEN_GREATER_GREATER_EQUAL : TOKEN_GREATER_GREATER);
            }
            return makeToken(
                match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '&':
            return makeToken(
                match('=') ? TOKEN_AMPERSAND_EQUAL : TOKEN_AMPERSAND);
        case '|':
            return makeToken(
                match('=') ? TOKEN_PIPE_EQUAL : TOKEN_PIPE);
        case '^':
            return makeToken(
                match('=') ? TOKEN_CARET_EQUAL : TOKEN_CARET);
        case '~':
            return makeToken(TOKEN_TILDE);
        case '"': return string();
        case '\'': {
            // Single quote strings
            while (peek() != '\'' && !isAtEnd()) {
                if (peek() == '\n') scanner.line++;
                if (peek() == '\\' && peekNext() != '\0') {
                    advance();
                }
                advance();
            }
            if (isAtEnd()) return errorToken("String tidak ditutup.");
            advance();
            return makeToken(TOKEN_STRING);
        }
    }

    return errorToken("Karakter tidak dikenal.");
}

const char* scannerSourceStart(void) {
    return scanner.source;
}

// One past the last byte of the buffer being scanned (its NUL). Needed to
// tell whether a token actually belongs to this buffer: f-strings compile
// their embedded expression from a temporary copy, so a stale token can sit
// at a higher address and pass a start-only check.
// Nesting depth of ( [ {: at depth 0 a comma can build a tupel, inside any
// bracket it keeps its existing meaning (arguments, elements, pairs).
int scannerGroupingDepth(void) {
    return scanner.groupingDepth;
}

const char* scannerSourceEnd(void) {
    return scanner.sourceEnd;
}
