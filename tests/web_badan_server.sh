#!/usr/bin/env bash
# Membuktikan parsing badan (WEB-02) bekerja terhadap curl sungguhan.
# Golden examples/web_badan.idk hanya menguji logika teks; yang penting di
# sini adalah UPLOAD BESAR: satu soket_baca() memotong di ~64KB, jadi uji
# 150 KB gagal bila web.baca_permintaan() tidak menggelung.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
PORT="${IDK_PORT_BADAN:-18793}"
SISA=20

cd "$AKAR" || exit 1

TMP="$(mktemp -d)"
bersihkan() {
    kill "$SERVER_PID" 2>/dev/null
    rm -rf "$TMP"
}

IDK_PORT="$PORT" IDK_SISA="$SISA" "$INDK" tests/bantu/server_badan.idk 2>/dev/null &
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

periksa "form urlencoded" "nama=Budi Santoso" \
    "$(curl -s --max-time 10 --data-urlencode 'nama=Budi Santoso' "http://127.0.0.1:$PORT/form")"

periksa "JSON" "judul=Laporan" \
    "$(curl -s --max-time 10 -H 'Content-Type: application/json' \
         -d '{"judul":"Laporan"}' "http://127.0.0.1:$PORT/api")"

# multipart dengan berkas sungguhan di disk.
printf 'isi berkas uji' > "$TMP/catatan.txt"
periksa "multipart + berkas" "catatan.txt 14" \
    "$(curl -s --max-time 10 -F "berkas=@$TMP/catatan.txt" "http://127.0.0.1:$PORT/unggah")"

# INTI uji ini: 150 KB jauh di atas batas ~64KB satu recv.
head -c 153600 /dev/zero | tr '\0' 'x' > "$TMP/besar.bin"
periksa "badan 150KB terbaca utuh" "bita=153600" \
    "$(curl -s --max-time 30 -H 'Content-Type: text/plain' \
         --data-binary "@$TMP/besar.bin" "http://127.0.0.1:$PORT/besar")"

# Melebihi batas server (300 KB) -> ditolak, bukan hang atau crash.
#
# Server menolak dari Content-Length lalu menutup koneksi -- sengaja, supaya
# badan raksasa tidak pernah masuk memori. Akibatnya curl bisa kehilangan
# soket di tengah pengiriman dan melaporkan '000' alih-alih '413'; keduanya
# benar, dan mana yang terjadi bergantung waktu (macOS kerap '000').
# Yang HARUS dijamin: bukan 2xx, dan server tetap melayani sesudahnya.
head -c 409600 /dev/zero | tr '\0' 'y' > "$TMP/kelewat.bin"
KODE_BESAR="$(curl -s -o /dev/null -w '%{http_code}' --max-time 30 \
         -H 'Content-Type: text/plain' \
         --data-binary "@$TMP/kelewat.bin" "http://127.0.0.1:$PORT/besar")"
case "$KODE_BESAR" in
    413|000) echo "  ok   melebihi batas ditolak ($KODE_BESAR)" ;;
    *) echo "  GAGAL melebihi batas: harap 413 atau 000, dapat '$KODE_BESAR'"; GAGAL=1 ;;
esac

# Server harus selamat dari penolakan itu, bukan mati diam-diam.
periksa "server hidup setelah 413" "judul=sesudah" \
    "$(curl -s --max-time 20 -H 'Content-Type: application/json' \
         --data-binary '{"judul":"sesudah"}' "http://127.0.0.1:$PORT/api")"

# Badan ber-UTF-8: Content-Length menghitung BYTE, panjang() menghitung
# KARAKTER. Saat keduanya tertukar, server menunggu byte yang tak pernah
# datang dan menggantung selamanya -- regresi yang pernah terjadi.
#
# Badan dikirim dari BERKAS, bukan argumen: curl di Windows mengubah argumen
# baris perintah ke code page ANSI, sehingga emoji sudah menjadi '?' sebelum
# sampai ke server dan uji gagal walau indk benar. Byte ditulis dengan escape
# oktal printf agar isi berkas tidak bergantung pada encoding shell.
printf '{"judul":"kopi \342\230\225 dan emoji \360\237\221\250\342\200\215\360\237\221\251\342\200\215\360\237\221\247"}' \
    > "$TMP/badan_utf8.json"
periksa "badan UTF-8 tidak menggantung" "judul=kopi ☕ dan emoji 👨‍👩‍👧" \
    "$(curl -s --max-time 20 \
         -H 'Content-Type: application/json' \
         --data-binary "@$TMP/badan_utf8.json" \
         "http://127.0.0.1:$PORT/api")"

periksa "JSON rusak -> 400" "400" \
    "$(curl -s -o /dev/null -w '%{http_code}' --max-time 10 \
         -H 'Content-Type: application/json' -d '{rusak' "http://127.0.0.1:$PORT/api")"

kill "$SERVER_PID" 2>/dev/null
wait "$SERVER_PID" 2>/dev/null
trap - EXIT
rm -rf "$TMP"

if [ "$GAGAL" -eq 0 ]; then
    echo "ok   parsing badan bekerja terhadap curl sungguhan"
    exit 0
fi
echo "GAGAL: parsing badan tidak bekerja seperti seharusnya"
exit 1
