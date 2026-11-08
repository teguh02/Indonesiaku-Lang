#!/usr/bin/env bash
# Regresi T-W12: BOM UTF-8 di awal kode yang dipipakan ke REPL dibuang.
#
# Windows PowerShell 5.1 menyisipkan BOM (EF BB BF) saat memipakan teks ke
# program native dengan encoding pipa UTF-8 -- `$OutputEncoding =
# [Text.Encoding]::UTF8`, saran yang paling umum, melakukan persis itu.
# Akibatnya `"x = 21" | indk` gagal di baris 1 dengan "Karakter tidak
# dikenal", padahal kode yang sama sebagai berkas berjalan (berkas sumber
# sudah membuang BOM sejak T-W2).
#
# Paritas Python, diperiksa dengan Python 3.14:
#   printf '\xef\xbb\xbfx = 21\nprint(x * 2)\n' | python   -> 42
#   input() dari pipa ber-BOM                               -> U+FEFF tetap ada
# Jadi yang dibuang hanya BOM di awal sesi REPL (kode sumber), BUKAN di
# input() -- keputusan yang sama tercatat di WIN-12 docs/TESTING/WINDOWS.md.
# Satu penyimpangan sadar: BOM berturut-turut di awal dibuang semua, karena
# PowerShell 5.1 mengirim DUA setelah `$OutputEncoding = [Text.Encoding]::UTF8`
# (Python gagal pada BOM kedua).
#
# BOM-nya disimulasikan dengan printf, jadi tes ini sama di ketiga OS. Tes
# golden tidak bisa memberi stdin, jadi ini skrip terpisah.

set -u
cd "$(dirname "$0")/.."

BIN=./indk
[ -x "$BIN" ] || BIN=./indk.exe
if [ ! -x "$BIN" ]; then
    echo "indk belum dibangun. Jalankan 'make' dulu." >&2
    exit 1
fi

GREEN='\033[0;32m'; RED='\033[0;31m'; NC='\033[0m'
GAGAL=0

periksa() {   # nama, keluaran_didapat, keluaran_diharapkan
    if [ "$2" = "$3" ]; then
        echo -e "  ${GREEN}+${NC} $1"
    else
        echo -e "  ${RED}x${NC} $1 (didapat: '$2', diharapkan: '$3')"
        GAGAL=$((GAGAL+1))
    fi
}

echo "Regresi BOM UTF-8 di awal kode REPL (T-W12)"

TMP="${TMPDIR:-/tmp}/indk_stdin_bom_$$"
mkdir -p "$TMP"
trap 'rm -rf "$TMP"' EXIT

# Keluaran REPL memuat prompt ("> ", "... "); yang diperiksa hanya angka di
# ujung baris, plus apakah ada galat pemindai.
angka() {   # masukan-printf
    printf "$1" | "$BIN" 2>&1 | tr -d '\r' | grep -o '[0-9][0-9]*$' | tr '\n' ' '
}
galat() {   # masukan-printf -> "galat" bila pemindai menolak karakter
    if printf "$1" | "$BIN" 2>&1 | grep -q "Karakter tidak dikenal"; then
        echo galat
    else
        echo bersih
    fi
}

# 1. REPL: baris pertama ber-BOM kini dikenali.
periksa "REPL tanpa BOM" "$(angka 'x = 21\ncetak(x * 2)\n')" "42 "
periksa "REPL dengan BOM" "$(angka '\xef\xbb\xbfx = 21\ncetak(x * 2)\n')" "42 "
periksa "REPL dengan BOM tanpa galat" "$(galat '\xef\xbb\xbfx = 21\n')" "bersih"

# 1b. DUA BOM berturut-turut: yang benar-benar dikirim PowerShell 5.1 setelah
#     `$OutputEncoding = [Text.Encoding]::UTF8` (dibuktikan dengan od).
#     Python berhenti di satu BOM dan gagal di sini; indk sengaja tidak.
periksa "REPL dengan dua BOM (PowerShell 5.1)" \
    "$(angka '\xef\xbb\xbf\xef\xbb\xbfx = 21\r\ncetak(x * 2)\r\n')" "42 "

# 2. BOM di depan blok multi-baris (baris lanjutan ikut terbaca).
periksa "BOM di depan blok fungsi" \
    "$(angka '\xef\xbb\xbffungsi f() {\n    kembalikan 7\n}\ncetak(f())\n')" "7 "

# 3. Hanya baris PERTAMA sesi: BOM di baris lain tetap karakter liar, sama
#    seperti di tengah berkas (dan seperti Python).
periksa "BOM di baris kedua tetap ditolak" "$(galat 'x = 1\n\xef\xbb\xbfcetak(x)\n')" "galat"

# 4. Batas: stdin hanya BOM tidak memicu galat.
periksa "hanya BOM" "$(galat '\xef\xbb\xbf')" "bersih"

# 5. input() TIDAK diubah: U+FEFF tetap sampai, seperti input() Python.
cat > "$TMP/masukan.idk" <<'EOF'
cetak([ord(c) untuk c dalam input()])
EOF
got=$(printf '\xef\xbb\xbfAndi\n' | "$BIN" "$TMP/masukan.idk" 2>&1 | tr -d '\r')
periksa "input() mempertahankan U+FEFF (paritas Python)" "$got" "[65279, 65, 110, 100, 105]"

if [ "$GAGAL" -eq 0 ]; then
    echo "STDIN BOM: semua lulus"
    exit 0
fi
echo "STDIN BOM: $GAGAL gagal"
exit 1
