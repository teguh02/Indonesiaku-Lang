#!/usr/bin/env bash
# Menguji penyajian berkas statis pustaka/web.idk lewat HTTP sungguhan.
# Tes ini melengkapi web_router_server.sh: yang penting di sini bukan hanya
# status code, tapi bahwa berkas BINER sampai utuh (md5 identik) dan bahwa
# percobaan traversal benar-benar ditolak oleh server yang berjalan.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
PORT="${IDK_PORT_STATIS:-18783}"
SISA=70

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

U="http://127.0.0.1:$PORT"

periksa "CSS terkirim" "body { color: #22C55E }" \
    "$(curl -s --max-time 5 "$U/statis/gaya.css")"

periksa "CSS Content-Type" "text/css; charset=utf-8" \
    "$(curl -s --max-time 5 -o /dev/null -w '%{content_type}' "$U/statis/gaya.css")"

periksa "subdirektori" "halo dari subdirektori" \
    "$(curl -s --max-time 5 "$U/statis/sub/pesan.txt")"

# Inti tes: berkas biner harus identik byte-per-byte. Content-Length yang
# dihitung ulang sebagai UTF-8 akan merusak ini.
# md5sum (GNU) tidak ada di macOS, yang memakai `md5 -q`. Tanpa penjagaan
# ini kedua sisi perbandingan sama-sama kosong dan tesnya lolos palsu.
if command -v md5sum >/dev/null 2>&1; then
    md5_dari() { md5sum | cut -d' ' -f1; }
elif command -v md5 >/dev/null 2>&1; then
    md5_dari() { md5 -q; }
else
    echo "  GAGAL tidak ada md5sum maupun md5 untuk memeriksa berkas biner"
    GAGAL=1
    md5_dari() { echo "TIDAK_ADA_ALAT"; }
fi

HARAP_MD5="$(md5_dari < tests/bantu/aset/titik.png)"
DAPAT_MD5="$(curl -s --max-time 5 "$U/statis/titik.png" | md5_dari)"
periksa "PNG biner utuh (md5)" "$HARAP_MD5" "$DAPAT_MD5"

periksa "PNG Content-Type" "image/png" \
    "$(curl -s --max-time 5 -o /dev/null -w '%{content_type}' "$U/statis/titik.png")"

periksa "PNG Content-Length" "$(wc -c < tests/bantu/aset/titik.png | tr -d ' ')" \
    "$(curl -s --max-time 5 -o /dev/null -w '%{size_download}' "$U/statis/titik.png")"

periksa "berkas tak ada -> 404" "404" \
    "$(curl -s --max-time 5 -o /dev/null -w '%{http_code}' "$U/statis/hantu.css")"

periksa "direktori -> 404" "404" \
    "$(curl -s --max-time 5 -o /dev/null -w '%{http_code}' "$U/statis/sub")"

periksa "akar statis -> 404" "404" \
    "$(curl -s --max-time 5 -o /dev/null -w '%{http_code}' "$U/statis/")"

periksa "POST ke statis -> 405" "405" \
    "$(curl -s --max-time 5 -o /dev/null -w '%{http_code}' -X POST "$U/statis/gaya.css")"

periksa "HEAD tanpa badan" "0" \
    "$(curl -s --max-time 5 -I -o /dev/null -w '%{size_download}' "$U/statis/gaya.css")"

# -I saja tidak cukup: curl membuang badan sendiri, jadi server yang keliru
# mengirim badan pada HEAD tetap lolos (terbukti: sabotase yang mengirim 24
# byte badan lolos dari varian -I). Bicara HTTP langsung dan hitung byte
# setelah baris kosong pemisah header.
HEAD_BADAN="$(
    exec 3<>/dev/tcp/127.0.0.1/"$PORT" || exit 1
    printf 'HEAD /statis/gaya.css HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n' >&3
    cat <&3 | tr -d '\r' \
        | awk 'badan { n += length($0) + 1 } /^$/ { badan = 1 } END { print n + 0 }'
)"
periksa "HEAD benar-benar tanpa badan" "0" "${HEAD_BADAN:-GAGAL_PROBE}"

periksa "HEAD tetap 200" "200" \
    "$(curl -s --max-time 5 -I -o /dev/null -w '%{http_code}' "$U/statis/gaya.css")"

# Traversal: --path-as-is supaya curl tidak merapikan ../ di sisi klien,
# sehingga server benar-benar yang diuji.
periksa "traversal ../ ditolak" "404" \
    "$(curl -s --max-time 5 --path-as-is -o /dev/null -w '%{http_code}' \
        "$U/statis/../rahasia.txt")"

periksa "traversal %2e%2e ditolak" "404" \
    "$(curl -s --max-time 5 --path-as-is -o /dev/null -w '%{http_code}' \
        "$U/statis/%2e%2e/rahasia.txt")"

# Membedakan "buang .." dari "naik satu level": turun ke sub dulu, lalu naik
# dua kali. Implementasi yang memproses .. sebagai naik level lolos dari tes
# "../" sederhana tetapi bocor di sini.
periksa "traversal sub/../.. ditolak" "404" \
    "$(curl -s --max-time 5 --path-as-is -o /dev/null -w '%{http_code}' \
        "$U/statis/sub/../../rahasia.txt")"

periksa "isi rahasia tidak bocor" "" \
    "$(curl -s --max-time 5 --path-as-is "$U/statis/../rahasia.txt" | grep -c 'JANGAN BOCOR' | grep -v '^0$')"

# Symlink DI DALAM akar yang menunjuk keluar: pemeriksaan teks jalur tidak
# melihatnya, jadi ini menguji perbandingan jalur nyata. Dibuat di sini
# supaya repo tidak perlu menyimpan symlink yang ter-commit.
ln -sf ../rahasia.txt tests/bantu/aset/bocor.txt 2>/dev/null
if [ -L tests/bantu/aset/bocor.txt ]; then
    periksa "symlink keluar akar -> 404" "404" \
        "$(curl -s --max-time 5 -o /dev/null -w '%{http_code}' "$U/statis/bocor.txt")"
    periksa "symlink tidak membocorkan isi" "" \
        "$(curl -s --max-time 5 "$U/statis/bocor.txt" | grep -c 'JANGAN BOCOR' | grep -v '^0$')"
    rm -f tests/bantu/aset/bocor.txt
else
    echo "  lewat symlink (tidak didukung di sistem berkas ini)"
fi

# ETag -> 304. Ambil ETag dari respons pertama, kirim balik via If-None-Match.
ETAG="$(curl -s --max-time 5 -I "$U/statis/gaya.css" | grep -i '^etag:' | sed 's/^[Ee][Tt][Aa][Gg]: *//' | tr -d '\r\n')"
if [ -z "$ETAG" ]; then
    echo "  GAGAL ETag tidak dikirim"
    GAGAL=1
else
    echo "  ok   ETag dikirim"
    periksa "If-None-Match -> 304" "304" \
        "$(curl -s --max-time 5 -o /dev/null -w '%{http_code}' \
            -H "If-None-Match: $ETAG" "$U/statis/gaya.css")"
fi

periksa "nosniff dipasang" "1" \
    "$(curl -s --max-time 5 -I "$U/statis/gaya.css" | grep -ci 'X-Content-Type-Options: nosniff')"

# Rute biasa tidak boleh rusak karena statis ditambahkan.
periksa "rute biasa tetap jalan" "halo dunia" \
    "$(curl -s --max-time 5 "$U/halo")"

periksa "404 di luar prefiks statis" "404" \
    "$(curl -s --max-time 5 -o /dev/null -w '%{http_code}' "$U/tidak-ada")"

if [ "$GAGAL" -eq 0 ]; then
    echo "Semua tes statis web lulus."
else
    echo "Ada tes statis web yang GAGAL."
fi
exit "$GAGAL"
