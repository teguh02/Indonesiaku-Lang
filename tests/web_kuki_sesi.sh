#!/usr/bin/env bash
# Membuktikan kuki dan sesi (WEB-04) bekerja terhadap koneksi HTTP sungguhan.
#
# Golden examples/web_kuki_sesi.idk hanya menguji bentuk string. Yang
# dibuktikan di sini: header benar-benar terkirim lewat soket, dua Set-Cookie
# tidak saling menimpa, Content-Length tetap benar sesudah header bertambah,
# dan kuki yang dikirim klien terbaca penangan.
#
# Permintaan dikirim lewat /dev/tcp, bukan curl, supaya tes jalan di Git Bash
# Windows tanpa dependensi tambahan.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
PORT="${IDK_PORT_KUKI:-18797}"
# Hitung tepat: 1 probe kesiapan + 7 permintaan di bawah.
SISA=8

cd "$AKAR" || exit 1

TMP="$(mktemp -d)"
bersihkan() {
    kill "$SERVER_PID" 2>/dev/null
    rm -rf "$TMP"
}

IDK_PORT="$PORT" IDK_SISA="$SISA" "$INDK" tests/bantu/server_kuki.idk 2>/dev/null &
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

# minta <path> [baris_header_tambahan] -> tulis respons mentah ke $TMP/res
minta() {
    local jalur="$1" tambahan="${2:-}"
    local permintaan="GET $jalur HTTP/1.1\r\nHost: 127.0.0.1\r\n"
    if [ -n "$tambahan" ]; then
        permintaan="$permintaan$tambahan\r\n"
    fi
    permintaan="$permintaan\r\n"
    exec 3<>/dev/tcp/127.0.0.1/"$PORT" || return 1
    printf "%b" "$permintaan" >&3
    # CRLF dibuang: indk.exe menulis CRLF dan semua perbandingan di bawah
    # memakai LF.
    cat <&3 > "$TMP/res" 2>/dev/null
    exec 3<&- 2>/dev/null
}

# --- 1. flag keamanan benar-benar terkirim -----------------------------------
minta "/masuk"
BARIS="$(tr -d '\r' < "$TMP/res" | grep -i '^Set-Cookie:')"

case "$BARIS" in
    *HttpOnly*) echo "  ok   HttpOnly terkirim" ;;
    *) echo "  GAGAL HttpOnly tidak ada: $BARIS"; GAGAL=1 ;;
esac
case "$BARIS" in
    *Secure*) echo "  ok   Secure terkirim" ;;
    *) echo "  GAGAL Secure tidak ada: $BARIS"; GAGAL=1 ;;
esac
case "$BARIS" in
    *SameSite=Lax*) echo "  ok   SameSite terkirim" ;;
    *) echo "  GAGAL SameSite tidak ada: $BARIS"; GAGAL=1 ;;
esac

# ID sesi harus 64 karakter heks — bukti acak_bita() dipakai, bukan acak().
SID="$(printf '%s' "$BARIS" | sed -n 's/^Set-Cookie: sesi=\([0-9a-f]*\);.*/\1/p')"
periksa "panjang ID sesi" "64" "${#SID}"

# --- 2. dua Set-Cookie tidak saling menimpa ----------------------------------
minta "/dua"
JUMLAH="$(tr -d '\r' < "$TMP/res" | grep -ci '^Set-Cookie:')"
periksa "dua Set-Cookie" "2" "$JUMLAH"

# --- 3. kuki dari klien terbaca ----------------------------------------------
minta "/siapa" "Cookie: sesi=$SID"
BADAN="$(tr -d '\r' < "$TMP/res" | awk 'kosong { print } /^$/ { kosong = 1 }')"
periksa "sesi terbaca" "ani" "$BADAN"

# Tanpa kuki -> 401.
minta "/siapa"
STATUS="$(head -1 "$TMP/res" | tr -d '\r' | cut -d' ' -f2)"
periksa "tanpa kuki ditolak" "401" "$STATUS"

# ID yang tidak dikenal -> 401 (bukan 200 dengan sesi kosong).
minta "/siapa" "Cookie: sesi=0000000000000000000000000000000000000000000000000000000000000000"
STATUS="$(head -1 "$TMP/res" | tr -d '\r' | cut -d' ' -f2)"
periksa "ID asing ditolak" "401" "$STATUS"

# --- 4. nilai berbahaya ter-encode -------------------------------------------
minta "/jahat"
BARIS="$(tr -d '\r' < "$TMP/res" | grep -i '^Set-Cookie:')"
case "$BARIS" in
    *"Domain=jahat.com"*)
        # Hanya salah bila ';' mentah membuatnya jadi ATRIBUT terpisah.
        case "$BARIS" in
            *"; Domain=jahat.com"*)
                echo "  GAGAL suntikan atribut lolos: $BARIS"; GAGAL=1 ;;
            *) echo "  ok   nilai berbahaya ter-encode" ;;
        esac ;;
    *) echo "  GAGAL nilai hilang: $BARIS"; GAGAL=1 ;;
esac

# --- 5. Content-Length tetap benar sesudah header bertambah ------------------
# "kopi ☕" = 6 karakter, 8 byte. Dihitung dari BERKAS, bukan $(...), karena
# command substitution memangkas newline akhir dan meleset satu byte.
minta "/utf8"
CL="$(tr -d '\r' < "$TMP/res" | sed -n 's/^Content-Length: *//p')"
periksa "Content-Length byte" "8" "$CL"

AWAL_BADAN="$(awk '/^\r?$/ { print NR + 1; exit }' "$TMP/res")"
NYATA="$(tail -n +"$AWAL_BADAN" "$TMP/res" | wc -c | tr -d ' ')"
periksa "badan nyata = Content-Length" "8" "$NYATA"

wait "$SERVER_PID" 2>/dev/null

if [ "$GAGAL" -eq 0 ]; then
    echo "web_kuki_sesi: semua lulus"
else
    echo "web_kuki_sesi: ADA YANG GAGAL"
fi
exit "$GAGAL"
