#!/usr/bin/env bash
# CRUD REST API (examples/api_siswa.idk) lewat HTTP sungguhan.
#
# Yang diuji adalah perilaku API-nya, bukan sekadar "server menyala": tiap
# metode, dua jalur 404 yang mudah salah (PUT/DELETE ke id yang tidak ada
# gampang menjawab 200), penolakan badan tidak sah, dan injeksi SQL.

set -u
cd "$(dirname "$0")/.."

PORT=${PORT:-8807}
LOG=$(mktemp)
lulus=0
gagal=0

periksa() {
    if [ "$2" = "$3" ]; then
        echo "  ok   $1"
        lulus=$((lulus+1))
    else
        echo "  GAGAL $1: diharapkan [$2], didapat [$3]"
        gagal=$((gagal+1))
    fi
}

# Kuota HARUS jauh di atas jumlah koneksi tes ini (tiap curl satu koneksi,
# termasuk probe kesiapan). Kuota pas-pasan membuat server keluar karena
# kuota habis, bukan karena yang sedang diuji.
IDK_PORT="$PORT" IDK_SISA=5000 ./indk examples/api_siswa.idk >"$LOG" 2>&1 &
SERVER_PID=$!
trap 'kill $SERVER_PID 2>/dev/null; rm -f "$LOG"' EXIT

siap=0
for _ in $(seq 1 50); do
    if curl -s -o /dev/null "http://127.0.0.1:$PORT/siswa" 2>/dev/null; then
        siap=1
        break
    fi
    sleep 0.1
done
periksa "server menyala" "1" "$siap"
if [ "$siap" != "1" ]; then
    echo "server gagal start:"; cat "$LOG"; exit 1
fi

kode() { curl -s -o /dev/null -w '%{http_code}' "$@"; }
badan() { curl -s "$@"; }

# --- Baca ---
periksa "GET daftar 200" "200" "$(kode "http://127.0.0.1:$PORT/siswa")"
periksa "GET satu 200" "200" "$(kode "http://127.0.0.1:$PORT/siswa/1")"
periksa "GET id tak ada 404" "404" "$(kode "http://127.0.0.1:$PORT/siswa/999")"

# --- Buat ---
periksa "POST 201" "201" \
    "$(kode -X POST -H 'Content-Type: application/json' \
        -d '{"nama":"Andi","nilai":77.5}' "http://127.0.0.1:$PORT/siswa")"

# Data yang dibuat harus benar-benar tersimpan, bukan sekadar dijawab 201.
if badan "http://127.0.0.1:$PORT/siswa" | grep -q 'Andi'; then
    tersimpan=1
else
    tersimpan=0
fi
periksa "POST benar-benar tersimpan" "1" "$tersimpan"

# --- Validasi badan ---
periksa "POST tanpa nilai 400" "400" \
    "$(kode -X POST -H 'Content-Type: application/json' \
        -d '{"nama":"Tanpa nilai"}' "http://127.0.0.1:$PORT/siswa")"
periksa "POST nilai bukan angka 400" "400" \
    "$(kode -X POST -H 'Content-Type: application/json' \
        -d '{"nama":"X","nilai":"tinggi"}' "http://127.0.0.1:$PORT/siswa")"
periksa "POST nama kosong 400" "400" \
    "$(kode -X POST -H 'Content-Type: application/json' \
        -d '{"nama":"","nilai":10}' "http://127.0.0.1:$PORT/siswa")"

# --- Ubah ---
periksa "PUT 200" "200" \
    "$(kode -X PUT -H 'Content-Type: application/json' \
        -d '{"nama":"Budi R","nilai":90}' "http://127.0.0.1:$PORT/siswa/1")"
# Jalur yang paling mudah salah: tanpa memeriksa db_baris_terpengaruh(),
# UPDATE ke id yang tidak ada tetap "berhasil" dan menjawab 200.
periksa "PUT id tak ada 404" "404" \
    "$(kode -X PUT -H 'Content-Type: application/json' \
        -d '{"nama":"Hantu","nilai":1}' "http://127.0.0.1:$PORT/siswa/999")"

# --- Hapus ---
periksa "DELETE 200" "200" "$(kode -X DELETE "http://127.0.0.1:$PORT/siswa/2")"
periksa "DELETE dua kali 404" "404" "$(kode -X DELETE "http://127.0.0.1:$PORT/siswa/2")"

if badan "http://127.0.0.1:$PORT/siswa" | grep -q 'Siti'; then
    terhapus=0
else
    terhapus=1
fi
periksa "DELETE benar-benar menghapus" "1" "$terhapus"

# --- Injeksi SQL ---
# Contoh ini mengajarkan query berparameter, jadi klaimnya harus dibuktikan:
# payload masuk sebagai NAMA biasa dan tabel tetap utuh.
kode -X POST -H 'Content-Type: application/json' \
    -d '{"nama":"Evil'"'"'); DROP TABLE siswa;--","nilai":1}' \
    "http://127.0.0.1:$PORT/siswa" >/dev/null
periksa "tabel utuh setelah payload injeksi" "200" \
    "$(kode "http://127.0.0.1:$PORT/siswa")"
if badan "http://127.0.0.1:$PORT/siswa" | grep -q 'DROP TABLE'; then
    apa_adanya=1
else
    apa_adanya=0
fi
periksa "payload tersimpan apa adanya" "1" "$apa_adanya"

echo ""
echo "api siswa: $lulus lulus, $gagal gagal"
[ "$gagal" -eq 0 ]
