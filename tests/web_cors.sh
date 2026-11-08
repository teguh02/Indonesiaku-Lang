#!/usr/bin/env bash
# Membuktikan CORS dan trusted-proxy (WEB-05) bekerja lewat HTTP nyata.
#
# Golden examples/web_cors.idk menguji fungsinya satu per satu. Yang
# dibuktikan di sini: header benar-benar sampai lewat kabel, preflight
# dijawab tanpa menyentuh penangan, dan X-Forwarded-For dari koneksi
# nyata diperlakukan sesuai asal koneksinya.
#
# Lewat /dev/tcp, bukan curl, supaya jalan di Git Bash Windows.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
PORT="${IDK_PORT_CORS:-18821}"
# 1 probe + 8 permintaan. Bila kurang, server berhenti lebih awal dan tes
# membandingkan respons permintaan sebelumnya.
SISA=9

cd "$AKAR" || exit 1

TMP="$(mktemp -d)"
bersihkan() {
    kill "$SERVER_PID" 2>/dev/null
    rm -rf "$TMP"
}

IDK_PORT="$PORT" IDK_SISA="$SISA" "$INDK" tests/bantu/server_cors.idk 2>/dev/null &
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
    local metode="$1" jalur="$2" tambahan="$3"
    # Dikosongkan dulu: koneksi yang gagal akan meninggalkan respons
    # permintaan SEBELUMNYA di tempatnya.
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
          'BEGIN { IGNORECASE = 0 }
           /^$/ { exit }
           { b = index($0, ":");
             if (b > 0) {
                 k = tolower(substr($0, 1, b - 1));
                 if (k == n) { v = substr($0, b + 1); sub(/^ +/, "", v); print v }
             } }'
}

echo "web_cors.sh: CORS dan trusted-proxy lewat HTTP nyata"

# --- 1. CORS untuk asal terdaftar -------------------------------------------
kirim GET /halo "Origin: https://situs-saya.id"
periksa "asal terdaftar -> 200" "200" "$(status)"
periksa "Allow-Origin dipantulkan" "https://situs-saya.id" \
        "$(header Access-Control-Allow-Origin)"
# Wajib: tanpa Vary, cache perantara bisa menyajikan respons ber-ACAO milik
# asal lain kepada pemohon berikutnya.
periksa "Vary: Origin ada" "Origin" "$(header Vary)"
periksa "Allow-Credentials" "true" "$(header Access-Control-Allow-Credentials)"

# --- 2. asal asing tidak mendapat header ------------------------------------
kirim GET /halo "Origin: https://jahat.id"
periksa "asal asing: tanpa Allow-Origin" "" "$(header Access-Control-Allow-Origin)"
periksa "asal asing: tetap 200 di server" "200" "$(status)"

# --- 3. bintang tidak pernah muncul -----------------------------------------
kirim GET /halo "Origin: https://situs-saya.id"
periksa "tidak pernah '*'" "https://situs-saya.id" \
        "$(header Access-Control-Allow-Origin)"

# --- 4. preflight ------------------------------------------------------------
kirim OPTIONS /halo "Origin: https://situs-saya.id"
periksa "preflight -> 204" "204" "$(status)"
periksa "preflight: Allow-Headers" "Content-Type, X-CSRF-Token" \
        "$(header Access-Control-Allow-Headers)"
periksa "preflight: badan kosong" "" "$(badan)"

# --- 5. trusted-proxy --------------------------------------------------------
# Koneksi ini benar-benar datang dari 127.0.0.1, yang terdaftar sebagai proxy
# tepercaya, jadi X-Forwarded-For dihormati.
kirim GET /siapa "X-Forwarded-For: 203.0.113.7"
periksa "peer tepercaya: pakai XFF" "203.0.113.7" "$(badan)"

# Entri paling KANAN yang bukan proxy: entri kiri ditulis klien dan dapat
# dipalsukan. Mengambil yang kiri membuat pembatas laju dapat dilewati.
kirim GET /siapa "X-Forwarded-For: 1.1.1.1, 203.0.113.7"
periksa "rantai: ambil paling kanan" "203.0.113.7" "$(badan)"

kirim GET /siapa ""
periksa "tanpa XFF: alamat peer" "127.0.0.1" "$(badan)"

# Kasus "peer BUKAN proxy tepercaya" tidak dapat diuji di sini: setiap koneksi
# tes datang dari 127.0.0.1, yang justru terdaftar sebagai tepercaya. Menguji
# penolakan menuntut alamat sumber kedua yang tidak dimiliki tiap mesin CI.
# Jalur itu ditutup golden examples/web_cors.idk, yang memanggil tangani()
# dengan peer sembarang — sabotase membuktikan golden memerah bila pemeriksaan
# peer dilewati, sedangkan berkas ini tetap hijau.

if [ "$GAGAL" -eq 0 ]; then
    echo "web_cors.sh: SEMUA LULUS"
    exit 0
fi
echo "web_cors.sh: ADA YANG GAGAL"
exit 1
