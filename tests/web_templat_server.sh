#!/usr/bin/env bash
# Render templat lewat respons HTTP sungguhan (SSR-07).
#
# Diuji lewat soket, bukan app.tangani(), karena yang dijamin SSR-07 adalah
# apa yang benar-benar diterima klien: status, header, dan badan.
#
# Fokus utama: HEAD pada rute yang punya GET. RFC 9110 §9.3.2 mewajibkan
# server yang mendukung GET juga mendukung HEAD; jalur rute dulu membalas 405
# sehingga setiap halaman SSR gagal di `curl -I`, pemeriksa tautan, dan
# monitor uptime.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
[ -x "$INDK" ] || INDK="$AKAR/indk.exe"
PORT="${IDK_PORT_UJI:-18787}"

# Permintaan yang dikirim: GET /, HEAD /, GET /rusak, GET /hilang,
# GET /nihil, POST /, HEAD /nihil = 7. Dilebihkan; server dimatikan lewat trap.
SISA=20

KERJA="$(mktemp -d)"
if command -v cygpath >/dev/null 2>&1; then
    KERJA="$(cygpath -m "$KERJA")"
fi
TPL="$KERJA/tpl"
mkdir -p "$TPL"
trap 'rm -rf "$KERJA"; kill "$SERVER_PID" 2>/dev/null' EXIT

cat > "$TPL/dasar.html" <<'EOF'
<html><head><title>{{ judul }}</title></head><body>{% blok isi %}{% akhirblok %}</body></html>
EOF
cat > "$TPL/daftar.html" <<'EOF'
{% perluas "dasar.html" %}{% blok isi %}<ul>{% untuk m dalam mahasiswa %}<li>{{ m.nama }}</li>{% akhiruntuk %}</ul>{% akhirblok %}
EOF
cat > "$TPL/rusak.html" <<'EOF'
<p>{{ tidak.pernah.diberikan }}</p>
EOF

cd "$AKAR" || exit 1

IDK_PORT="$PORT" IDK_SISA="$SISA" IDK_TPL="$TPL" \
    "$INDK" tests/bantu/server_templat.idk &
SERVER_PID=$!

# Tunggu port menerima koneksi, jangan tidur buta.
siap=0
for _ in $(seq 1 50); do
    if (exec 3<>/dev/tcp/127.0.0.1/"$PORT") 2>/dev/null; then
        exec 3<&- 2>/dev/null
        siap=1
        break
    fi
    sleep 0.1
done
if [ "$siap" -ne 1 ]; then
    echo "GAGAL: server tidak pernah siap di port $PORT"
    exit 1
fi

lulus=0
gagal=0

periksa() {
    local nama="$1" harap="$2" dapat="$3"
    if [ "$harap" = "$dapat" ]; then
        lulus=$((lulus + 1)); echo "  OK   $nama"
    else
        gagal=$((gagal + 1))
        echo "  GAGAL $nama"
        echo "        harap : $harap"
        echo "        dapat : $dapat"
    fi
}

# Satu permintaan mentah lewat /dev/tcp; curl tidak dipakai supaya tes ini
# jalan di Git Bash Windows tanpa dependensi tambahan.
minta() {
    local metode="$1" jalur="$2"
    exec 3<>/dev/tcp/127.0.0.1/"$PORT" || return 1
    printf '%s %s HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n' \
        "$metode" "$jalur" >&3
    cat <&3
    exec 3<&- 2>/dev/null
}

minta GET / | tr -d '\r' > "$KERJA/get.txt"
GET_PENUH="$(cat "$KERJA/get.txt")"
minta HEAD / > "$KERJA/head_mentah.txt"
HEAD_PENUH="$(tr -d '\r' < "$KERJA/head_mentah.txt")"

status() { echo "$1" | sed -n 1p; }
# Header berakhir di baris kosong pertama. awk dipakai, bukan sed '1,/^$/p',
# karena pada HEAD badannya kosong sehingga command substitution memangkas
# baris kosong penutup — sed lalu ikut membuang baris header terakhir.
kepala() { echo "$1" | awk 'BEGIN{h=1} /^$/{h=0} h{print}'; }
badan() { echo "$1" | awk 'b{print} /^$/{b=1}'; }

# --- 1. GET halaman SSR ----------------------------------------------------
periksa "GET / status 200" "HTTP/1.1 200 OK" "$(status "$GET_PENUH")"
periksa "GET / Content-Type html+utf-8" \
    "Content-Type: text/html; charset=utf-8" \
    "$(echo "$GET_PENUH" | grep -i '^Content-Type:' | head -1)"
periksa "GET / merender data" \
    "<html><head><title>Kopi ☕ &lt;b&gt;tebal&lt;/b&gt;</title></head><body><ul><li>Ani</li><li>Budi &amp; Co</li><li>Cici</li></ul></body></html>" \
    "$(badan "$GET_PENUH" | tr -d '\n')"

# Content-Length wajib byte, bukan karakter: judul memuat ☕ (3 byte).
cl_get="$(echo "$GET_PENUH" | grep -i '^Content-Length:' | tr -cd '0-9')"
# Dihitung dari berkas mentah: command substitution memangkas newline akhir,
# jadi menghitung lewat variabel akan meleset satu byte.
byte_kepala=$(awk 'BEGIN{n=0} {n+=length($0)+1} /^$/{print n; exit}' "$KERJA/get.txt")
byte_total=$(wc -c < "$KERJA/get.txt" | tr -d ' ')
byte_badan=$((byte_total - byte_kepala))
periksa "GET / Content-Length = byte badan" "$cl_get" "$byte_badan"

# --- 2. HEAD: header identik dengan GET, badan kosong ----------------------
periksa "HEAD / status 200 (bukan 405)" "HTTP/1.1 200 OK" "$(status "$HEAD_PENUH")"
periksa "HEAD / header identik dengan GET" \
    "$(kepala "$GET_PENUH")" "$(kepala "$HEAD_PENUH")"
periksa "HEAD / badan kosong" "" "$(badan "$HEAD_PENUH" | tr -d '\n')"
cl_head="$(echo "$HEAD_PENUH" | grep -i '^Content-Length:' | tr -cd '0-9')"
periksa "HEAD / Content-Length sama dengan GET" "$cl_get" "$cl_head"

# Respons HEAD wajib DIAKHIRI baris kosong (CRLF CRLF). Tanpa itu klien
# menunggu sisa header sampai timeout — dan perbandingan header di atas
# tidak melihatnya, karena baris kosong penutup memang tidak ikut dibandingkan.
periksa "HEAD / diakhiri CRLF CRLF" "1" \
    "$(tail -c 4 "$KERJA/head_mentah.txt" | od -An -c | grep -c '\\r  \\n  \\r  \\n')"

# --- 3. Halaman galat tidak membocorkan rahasia ---------------------------
RUSAK="$(minta GET /rusak | tr -d '\r')"
periksa "templat rusak -> 500" "HTTP/1.1 500 Internal Server Error" "$(status "$RUSAK")"
periksa "500 tidak menyebut nama variabel templat" "0" \
    "$(echo "$RUSAK" | grep -c 'diberikan')"

HILANG="$(minta GET /hilang | tr -d '\r')"
periksa "templat hilang -> 500" "HTTP/1.1 500 Internal Server Error" "$(status "$HILANG")"
# Jalur sistem berkas adalah kebocoran paling berbahaya di halaman galat.
periksa "500 tidak membocorkan jalur berkas" "0" \
    "$(echo "$HILANG" | grep -cE 'tidak_ada_sama_sekali|/tpl/|GalatTemplat')"

# --- 4. Status lain tetap benar -------------------------------------------
NIHIL="$(minta GET /nihil | tr -d '\r')"
periksa "path tak dikenal -> 404" "HTTP/1.1 404 Not Found" "$(status "$NIHIL")"

POSTS="$(minta POST / | tr -d '\r')"
periksa "metode salah -> 405" "HTTP/1.1 405 Method Not Allowed" "$(status "$POSTS")"
# GET menyiratkan HEAD, jadi Allow yang menyebut GET saja berbohong.
periksa "405 Allow menyebut GET dan HEAD" "1" \
    "$(echo "$POSTS" | grep -ciE '^Allow:.*GET.*HEAD')"

# HEAD ke path yang memang tidak ada tetap 404, bukan 200 kosong.
HEAD_NIHIL="$(minta HEAD /nihil | tr -d '\r')"
periksa "HEAD path tak dikenal -> 404" "HTTP/1.1 404 Not Found" "$(status "$HEAD_NIHIL")"

kill "$SERVER_PID" 2>/dev/null
wait "$SERVER_PID" 2>/dev/null

echo ""
echo "web_templat: $lulus lulus, $gagal gagal"
[ "$gagal" -eq 0 ]
