#!/usr/bin/env bash
# Observability (Fase 22) lewat HTTP nyata: app.pantau() di belakang gelung
# app.jalankan().
#
# Golden examples/web_pantau.idk menguji app.tangani() tanpa soket. Yang
# dibuktikan di sini: /health dan /ready terjawab lewat kabel, X-Request-ID
# sampai ke klien DAN sama dengan request_id di baris log JSON, counter
# /metrics naik setelah permintaan (bentuk pertumbuhan, bukan angka mutlak),
# dan setiap baris /metrics sah menurut format teks Prometheus.
#
# Lewat /dev/tcp, bukan curl, supaya jalan di Git Bash Windows.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
[ -x "$INDK" ] || INDK="$AKAR/indk.exe"
PORT="${IDK_PORT_PANTAU:-18831}"

cd "$AKAR" || exit 1

TMP="$(mktemp -d)"
LOG="$TMP/log"
bersihkan() {
    kill "$SERVER_PID" 2>/dev/null
    rm -rf "$TMP"
}

# Kuota jauh di atas jumlah koneksi tes ini; server berhenti lewat /matikan.
IDK_PORT="$PORT" IDK_SISA=500 "$INDK" tests/bantu/server_pantau.idk >/dev/null 2>"$LOG" &
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

kirim() {
    local metode="$1" jalur="$2" tambahan="${3:-}"
    : > "$TMP/res"
    local permintaan="$metode $jalur HTTP/1.1\r\nHost: 127.0.0.1\r\n"
    if [ -n "$tambahan" ]; then
        permintaan="$permintaan$tambahan\r\n"
    fi
    permintaan="$permintaan\r\n"
    exec 3<>/dev/tcp/127.0.0.1/"$PORT" || return 1
    printf "%b" "$permintaan" >&3
    cat <&3 > "$TMP/res" 2>/dev/null
    exec 3<&- 2>/dev/null
}

status() { head -1 "$TMP/res" | tr -d '\r' | cut -d' ' -f2; }
badan()  { tr -d '\r' < "$TMP/res" | awk 'k { print } /^$/ { k = 1 }'; }
header() {
    tr -d '\r' < "$TMP/res" \
        | awk -v n="$(echo "$1" | tr 'A-Z' 'a-z')" \
          '/^$/ { exit }
           { b = index($0, ":");
             if (b > 0) {
                 k = tolower(substr($0, 1, b - 1));
                 if (k == n) { v = substr($0, b + 1); sub(/^ +/, "", v); print v }
             } }'
}

# metrik NAMA_LENGKAP -> nilai baris itu dari /metrics terakhir, 0 bila tidak ada.
metrik() {
    badan | awk -v n="$1" 'index($0, n " ") == 1 { print $2; f = 1 } END { if (!f) print 0 }'
}

echo "web_pantau_server.sh: observability lewat HTTP nyata"

kirim GET /health
periksa "/health 200" "200" "$(status)"
periksa "/health badan" '{"status":"ok"}' "$(badan)"

kirim GET /ready
periksa "/ready 200 saat siap" "200" "$(status)"

kirim GET /halo
periksa "/halo 200" "200" "$(status)"
RID="$(header X-Request-ID)"
if echo "$RID" | grep -qE '^[0-9a-f]{16}$'; then
    periksa "X-Request-ID 16 heks" "ya" "ya"
else
    periksa "X-Request-ID 16 heks" "ya" "$RID"
fi

# Baris log JSON untuk permintaan itu membawa request_id yang sama.
sleep 0.2
if grep -q "\"request_id\":\"$RID\"" "$LOG" \
   && grep "\"request_id\":\"$RID\"" "$LOG" | grep -q '"path":"/halo","status":200'; then
    periksa "log JSON memuat request_id yang sama" "ya" "ya"
else
    periksa "log JSON memuat request_id yang sama" "ya" "tidak"
fi

# X-Request-ID dari proxy tepercaya (koneksi ini dari 127.0.0.1) dipakai.
kirim GET /halo "X-Request-ID: nginx-77.a"
periksa "X-Request-ID dari proxy tepercaya dipakai" "nginx-77.a" "$(header X-Request-ID)"
# ...tetapi bentuk berbahaya tidak.
kirim GET /halo 'X-Request-ID: "a\nb"'
if header X-Request-ID | grep -qE '^[0-9a-f]{16}$'; then
    periksa "X-Request-ID berbahaya diganti" "ya" "ya"
else
    periksa "X-Request-ID berbahaya diganti" "ya" "$(header X-Request-ID)"
fi

kirim GET /metrics
periksa "/metrics 200" "200" "$(status)"
periksa "/metrics Content-Type" "text/plain; version=0.0.4; charset=utf-8" "$(header Content-Type)"
SEBELUM="$(metrik 'indk_http_permintaan_total{metode="GET",status="200"}')"
HITUNG_SEBELUM="$(metrik indk_http_durasi_detik_count)"

# Setiap baris: komentar HELP/TYPE, atau `nama{label} nilai` / `nama nilai`.
salah_format="$(badan | grep -vE '^# (HELP|TYPE) [a-z_]+ .+$' \
    | grep -vE '^[a-z_]+(\{[a-z_]+="[^"]*"(,[a-z_]+="[^"]*")*\})? -?[0-9.e+]+$' \
    | grep -v '^$' | head -1)"
periksa "semua baris /metrics berformat Prometheus" "" "$salah_format"

for _ in 1 2 3; do kirim GET /halo; done
kirim GET /meledak
periksa "penangan meledak tetap 500" "500" "$(status)"
RID_500="$(header X-Request-ID)"

kirim GET /metrics
SESUDAH="$(metrik 'indk_http_permintaan_total{metode="GET",status="200"}')"
periksa "counter GET 200 naik 4 (3x /halo + /metrics)" "$((SEBELUM + 4))" "$SESUDAH"
periksa "counter GET 500 tercatat" "1" "$(metrik 'indk_http_permintaan_total{metode="GET",status="500"}')"
periksa "histogram count naik 5" "$((HITUNG_SEBELUM + 5))" "$(metrik indk_http_durasi_detik_count)"
periksa "bucket +Inf sama dengan count" "$(metrik indk_http_durasi_detik_count)" \
    "$(metrik 'indk_http_durasi_detik_bucket{le="+Inf"}')"

# Galat penangan di stderr membawa ID yang dilihat klien.
if grep -q "(id $RID_500): sengaja meledak" "$LOG"; then
    periksa "galat 500 di log membawa X-Request-ID" "ya" "ya"
else
    periksa "galat 500 di log membawa X-Request-ID" "ya" "tidak"
fi

kirim POST /tidak-siap
kirim GET /ready
periksa "/ready 503 saat tidak siap" "503" "$(status)"
periksa "/ready badan tidak siap" '{"status":"tidak siap"}' "$(badan)"
kirim GET /health
periksa "/health tetap 200 saat tidak siap" "200" "$(status)"

# Header dan badan tidak pernah masuk log.
if grep -qi 'Host:' "$LOG"; then
    periksa "header tidak ikut tercatat" "bersih" "ada header"
else
    periksa "header tidak ikut tercatat" "bersih" "bersih"
fi

kirim GET /matikan
periksa "/matikan dilayani" "200" "$(status)"

echo ""
if [ "$GAGAL" -eq 0 ]; then
    echo "web_pantau_server.sh: lulus"
else
    echo "web_pantau_server.sh: GAGAL"
    echo "--- log server"
    cat "$LOG"
fi
exit $GAGAL
