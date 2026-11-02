#!/usr/bin/env bash
# REST API di atas PostgreSQL, diuji lewat HTTP sungguhan dengan curl.
#
# Ini yang menutup Fase 11 langkah 7: "contoh CRUD API memakai database
# Fase 10". Pasangan tests/api_siswa_server.sh yang memakai SQLite.
#
# Injeksi SQL TIDAK diuji ulang di sini -- sudah tertutup di
# tests/pg_driver.sh pada lapisan driver, tempat jaminannya sebenarnya berada.
#
# Melewati diri sendiri bila Docker tidak ada.
set -u

INDK="${INDK:-./indk}"
PGPORT_UJI=55434
WEBPORT=8855
WADAH=idkpg_api
SANDI=rahasia_api
GAGAL=0
SRVPID=""

bersihkan() {
    [ -n "$SRVPID" ] && kill "$SRVPID" 2>/dev/null
    docker rm -f "$WADAH" >/dev/null 2>&1 || true
    rm -f /tmp/idk_apipg.log
}
trap bersihkan EXIT

if ! command -v docker >/dev/null 2>&1 || ! docker info >/dev/null 2>&1; then
    echo "  LEWAT api pg: docker tidak tersedia"
    exit 0
fi

docker rm -f "$WADAH" >/dev/null 2>&1 || true
if ! docker run -d --rm --name "$WADAH" \
        -e POSTGRES_PASSWORD="$SANDI" -e POSTGRES_DB=sekolah \
        -p "127.0.0.1:$PGPORT_UJI:5432" postgres:16-alpine >/dev/null 2>&1; then
    echo "  LEWAT api pg: image postgres tidak bisa dijalankan"
    exit 0
fi
# pg_isready di dalam container lolos saat server masih restart setelah
# initdb; koneksi dari host lalu diterima-dan-diputus. Tunggu query
# sungguhan dari host.
SIAP=0
for _ in $(seq 1 60); do
    if PGPORT_T="$PGPORT_UJI" PGSANDI_T="$SANDI" "$INDK" /dev/stdin <<'PROBE' >/dev/null 2>&1
impor "pustaka/pg.idk"
variabel k = sambung({"host":"127.0.0.1",
                      "port": ke_bulat(ambil_lingkungan("PGPORT_T")),
                      "pengguna":"postgres",
                      "sandi": ambil_lingkungan("PGSANDI_T"),
                      "basisdata":"sekolah"})
k.ambil_satu("SELECT 1 AS x")
k.tutup()
PROBE
    then
        SIAP=1
        break
    fi
    sleep 1
done
if [ "$SIAP" -eq 0 ]; then
    echo "  GAGAL api pg: PostgreSQL tidak bisa dihubungi dari host"
    exit 1
fi

PGPORT="$PGPORT_UJI" PGSANDI="$SANDI" IDK_PORT="$WEBPORT" IDK_SISA=60 \
    "$INDK" examples/api_siswa_pg.idk > /tmp/idk_apipg.log 2>&1 &
SRVPID=$!

HIDUP=0
for _ in $(seq 1 40); do
    if curl -s -o /dev/null "localhost:$WEBPORT/siswa" 2>/dev/null; then
        HIDUP=1
        break
    fi
    sleep 0.5
done
if [ "$HIDUP" -eq 0 ]; then
    echo "  GAGAL api pg: server tidak menyala"
    cat /tmp/idk_apipg.log
    exit 1
fi

periksa() {
    local nama="$1" harap="$2" dapat="$3"
    if [ "$dapat" = "$harap" ]; then
        echo "  OK    $nama"
    else
        echo "  GAGAL $nama: harap '$harap', dapat '$dapat'"
        GAGAL=$((GAGAL+1))
    fi
}

kode() { curl -s -o /dev/null -w '%{http_code}' "$@"; }
badan() { curl -s "$@"; }

echo "REST API di atas PostgreSQL:"

periksa "POST 201" "201" \
    "$(kode -X POST "localhost:$WEBPORT/siswa" -H 'Content-Type: application/json' -d '{"nama":"Budi","nilai":80}')"
periksa "GET satu" '{"id":"1","nama":"Budi","nilai":"80"}' \
    "$(badan "localhost:$WEBPORT/siswa/1")"
periksa "PUT 200" "200" \
    "$(kode -X PUT "localhost:$WEBPORT/siswa/1" -H 'Content-Type: application/json' -d '{"nama":"Budi Revisi","nilai":95}')"
periksa "PUT tersimpan" '{"id":"1","nama":"Budi Revisi","nilai":"95"}' \
    "$(badan "localhost:$WEBPORT/siswa/1")"

# Ketiga metode harus 404 untuk id yang tidak ada. Tanpa memeriksa jumlah baris
# terpengaruh, UPDATE/DELETE ke id asing diam-diam dijawab 200.
periksa "GET 404"    "404" "$(kode "localhost:$WEBPORT/siswa/999")"
periksa "PUT 404"    "404" "$(kode -X PUT "localhost:$WEBPORT/siswa/999" -H 'Content-Type: application/json' -d '{"nama":"X"}')"
periksa "DELETE 404" "404" "$(kode -X DELETE "localhost:$WEBPORT/siswa/999")"

periksa "badan tanpa nama 400" "400" \
    "$(kode -X POST "localhost:$WEBPORT/siswa" -H 'Content-Type: application/json' -d '{"nilai":5}')"
periksa "JSON rusak 400" "400" \
    "$(kode -X POST "localhost:$WEBPORT/siswa" -H 'Content-Type: application/json' -d '{rusak')"

periksa "DELETE 200" "200" "$(kode -X DELETE "localhost:$WEBPORT/siswa/1")"
periksa "daftar kosong" "[]" "$(badan "localhost:$WEBPORT/siswa")"

# Sandi tidak boleh muncul di log server.
if grep -q "$SANDI" /tmp/idk_apipg.log; then
    echo "  GAGAL sandi bocor ke log"
    GAGAL=$((GAGAL+1))
else
    echo "  OK    sandi tidak bocor ke log"
fi

if [ "$GAGAL" -gt 0 ]; then
    echo "GAGAL: $GAGAL pemeriksaan"
    exit 1
fi
echo "LULUS"
