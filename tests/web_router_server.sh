#!/usr/bin/env bash
# Menguji pustaka/web.idk melayani HTTP sungguhan lewat soket, ditembak curl.
# Golden test examples/web_router.idk hanya membuktikan logika teks; ini
# membuktikan modulnya benar-benar jalan sebagai server.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
PORT="${IDK_PORT_WEB:-18781}"
# Kuota dilebihkan; server dimatikan lewat trap, bukan menunggu habis.
SISA=20

cd "$AKAR" || exit 1

IDK_PORT="$PORT" IDK_SISA="$SISA" "$INDK" tests/bantu/server_web.idk 2>/dev/null &
SERVER_PID=$!
trap 'kill "$SERVER_PID" 2>/dev/null' EXIT

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

periksa "GET /halo" "halo dunia" \
    "$(curl -s --max-time 5 "http://127.0.0.1:$PORT/halo")"

periksa "parameter URL" '{"id":"42"}' \
    "$(curl -s --max-time 5 "http://127.0.0.1:$PORT/siswa/42")"

periksa "query string" "q=buku" \
    "$(curl -s --max-time 5 "http://127.0.0.1:$PORT/cari?q=buku")"

periksa "query terenkode" "q=dua kata" \
    "$(curl -s --max-time 5 "http://127.0.0.1:$PORT/cari?q=dua%20kata")"

periksa "404" "404" \
    "$(curl -s -o /dev/null -w '%{http_code}' --max-time 5 "http://127.0.0.1:$PORT/tidakada")"

periksa "405 metode salah" "405" \
    "$(curl -s -o /dev/null -w '%{http_code}' -X DELETE --max-time 5 "http://127.0.0.1:$PORT/halo")"

# Server yang mendukung GET wajib mendukung HEAD (RFC 9110 §9.3.2), jadi
# Allow harus menyebut keduanya; "GET" saja membuat header ini berbohong.
periksa "header Allow pada 405" "GET, HEAD" \
    "$(curl -s -D- -o /dev/null -X DELETE --max-time 5 "http://127.0.0.1:$PORT/halo" \
       | tr -d '\r' | awk -F': ' '/^[Aa]llow:/ {print $2}')"

# `curl -I` adalah HEAD: inilah yang dipakai pemeriksa tautan dan monitor
# uptime, dan yang dulu membalas 405 pada setiap halaman.
periksa "curl -I (HEAD) membalas 200" "200" \
    "$(curl -s -o /dev/null -w '%{http_code}' -I --max-time 5 "http://127.0.0.1:$PORT/halo")"

periksa "POST dengan badan" "diterima 11 bita" \
    "$(curl -s --max-time 5 -X POST --data 'halo dunia!' "http://127.0.0.1:$PORT/kirim")"

periksa "Content-Type JSON" "application/json" \
    "$(curl -s -D- -o /dev/null --max-time 5 "http://127.0.0.1:$PORT/siswa/1" \
       | tr -d '\r' | awk -F': ' '/^[Cc]ontent-[Tt]ype:/ {print $2}')"

kill "$SERVER_PID" 2>/dev/null
wait "$SERVER_PID" 2>/dev/null
trap - EXIT

if [ "$GAGAL" -eq 0 ]; then
    echo "ok   pustaka/web.idk melayani HTTP sungguhan"
    exit 0
fi
echo "GAGAL: modul web tidak melayani HTTP seperti seharusnya"
exit 1
