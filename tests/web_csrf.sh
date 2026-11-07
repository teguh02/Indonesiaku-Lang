#!/usr/bin/env bash
# Membuktikan proteksi CSRF (WEB-05) menahan serangan lintas situs nyata.
#
# Golden examples/web_csrf.idk menguji logikanya terhadap kamus buatan. Yang
# dibuktikan di sini: POST sungguhan yang MEMBAWA kuki sesi tapi tanpa token
# dijawab 403. Itulah bentuk persis serangan CSRF — peramban korban melampirkan
# kukinya secara otomatis, dan tanpa fitur ini server akan menjawab 200.
#
# Lewat /dev/tcp, bukan curl, supaya jalan di Git Bash Windows.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
PORT="${IDK_PORT_CSRF:-18799}"
# 1 probe kesiapan + 9 permintaan.
SISA=10

cd "$AKAR" || exit 1

TMP="$(mktemp -d)"
bersihkan() {
    kill "$SERVER_PID" 2>/dev/null
    rm -rf "$TMP"
}

IDK_PORT="$PORT" IDK_SISA="$SISA" "$INDK" tests/bantu/server_csrf.idk 2>/dev/null &
SERVER_PID=$!
trap bersihkan EXIT

for _ in $(seq 1 50); do
    if (exec 3<>/dev/tcp/127.0.0.1/"$PORT") 2>/dev/null; then
        exec 3<&- 2>/dev/null
        break
    fi
    sleep 0.1
done

GAGAL=0
periksa() {
    local nama="$1" harap="$2" dapat="$3"
    if [ "$harap" = "$dapat" ]; then
        echo "  ok   $nama"
    else
        echo "  GAGAL $nama: harap '$harap', dapat '$dapat'"
        GAGAL=1
    fi
}

# kirim <metode> <path> <baris_header_tambahan> <badan> -> $TMP/res
kirim() {
    local metode="$1" jalur="$2" tambahan="$3" badan="$4"
    local permintaan="$metode $jalur HTTP/1.1\r\nHost: 127.0.0.1\r\n"
    if [ -n "$badan" ]; then
        permintaan="$permintaan"
        permintaan="${permintaan}Content-Type: application/x-www-form-urlencoded\r\n"
        permintaan="${permintaan}Content-Length: ${#badan}\r\n"
    fi
    if [ -n "$tambahan" ]; then
        permintaan="$permintaan$tambahan\r\n"
    fi
    permintaan="$permintaan\r\n$badan"
    exec 3<>/dev/tcp/127.0.0.1/"$PORT" || return 1
    printf "%b" "$permintaan" >&3
    cat <&3 > "$TMP/res" 2>/dev/null
    exec 3<&- 2>/dev/null
}

status() { head -1 "$TMP/res" | tr -d '\r' | cut -d' ' -f2; }
badan()  { tr -d '\r' < "$TMP/res" | awk 'k { print } /^$/ { k = 1 }'; }

# --- login: ambil ID sesi dan tokennya ---------------------------------------
kirim GET /masuk "" ""
SID="$(tr -d '\r' < "$TMP/res" | sed -n 's/^Set-Cookie: sesi=\([0-9a-f]*\);.*/\1/p')"
TOKEN="$(badan)"
periksa "login memberi sesi" "64" "${#SID}"
periksa "login memberi token" "64" "${#TOKEN}"

# --- 1. SERANGAN: kuki sesi ikut, token tidak ada ----------------------------
# Inilah yang dikirim formulir di situs penyerang. Tanpa proteksi: 200.
kirim POST /hapus "Cookie: sesi=$SID" ""
periksa "serangan CSRF ditolak" "403" "$(status)"

# --- 2. token milik sesi LAIN --------------------------------------------- --
kirim GET /masuk "" ""
TOKEN_LAIN="$(badan)"
kirim POST /hapus "Cookie: sesi=$SID" "_csrf=$TOKEN_LAIN"
periksa "token sesi lain ditolak" "403" "$(status)"

# --- 3. token benar ----------------------------------------------------------
kirim POST /hapus "Cookie: sesi=$SID" "_csrf=$TOKEN"
periksa "token benar diterima" "200" "$(status)"
periksa "badan benar" "terhapus" "$(badan)"

# --- 4. metode aman tidak terhalang ------------------------------------------
kirim GET /hapus "Cookie: sesi=$SID" ""
periksa "GET tanpa token lolos" "200" "$(status)"

# --- 5. jalur AJAX lewat header ----------------------------------------------
kirim POST /hapus "Cookie: sesi=$SID\r\nX-CSRF-Token: $TOKEN" ""
periksa "header X-CSRF-Token" "200" "$(status)"

# --- 6. rotasi ID: token lama mati bersama sesi lamanya ----------------------
kirim POST /putar "Cookie: sesi=$SID" "_csrf=$TOKEN"
periksa "rotasi berhasil" "200" "$(status)"
kirim POST /hapus "Cookie: sesi=$SID" "_csrf=$TOKEN"
periksa "sesi lama mati" "403" "$(status)"

wait "$SERVER_PID" 2>/dev/null

if [ "$GAGAL" -eq 0 ]; then
    echo "web_csrf: semua lulus"
else
    echo "web_csrf: ADA YANG GAGAL"
fi
exit "$GAGAL"
