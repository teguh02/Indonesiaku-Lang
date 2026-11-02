#!/usr/bin/env bash
# Parsing badan urlencoded harus LINEAR terhadap ukuran, bukan kuadratik.
#
# Latar: _web_dekode_persen() dulu merakit hasil dengan `hasil = hasil + c`
# per karakter. Teks immutable, jadi setiap gabung menyalin seluruh string
# yang sudah terkumpul -- O(n^2). Terukur sebelum perbaikan: 40 KB = 1,6 s,
# 80 KB = 6,7 s, 100 KB = 10,7 s. Karena gelung server melayani satu
# permintaan pada satu waktu, satu POST 1 MB membekukan server berjam-jam:
# itu DoS yang dikirim lewat satu permintaan sah.
#
# Yang diperiksa adalah BENTUK PERTUMBUHAN, bukan ambang detik mutlak. Runner
# CI yang kebetulan lambat tidak boleh membuat tes ini merah, tetapi
# kembalinya O(n^2) harus membuatnya merah.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
PORT="${PORT_UJI:-8884}"
TMP="$(mktemp -d)"
SRV=""
trap 'rm -rf "$TMP"; [ -n "$SRV" ] && kill "$SRV" 2>/dev/null' EXIT

if [ ! -x "$INDK" ]; then
    echo "LEWAT: $INDK belum dibangun"
    exit 0
fi

cat > "$TMP/srv.idk" <<'EOF'
impor web

# 4 MB: cukup untuk 1 MB uji tanpa kena batas badan.
variabel app = web.Aplikasi(4194304)

app.rute("POST", "/besar", fungsi(req) {
    kembalikan web.teks(200, ke_teks(panjang(req["badan"])))
})

app.rute("POST", "/f", fungsi(req) {
    kembalikan web.teks(200, req["form"]["a"])
})

app.jalankan(ke_bulat(ambil_lingkungan("IDK_PORT")), 60, 0, salah)
EOF

IDK_PORT="$PORT" "$INDK" "$TMP/srv.idk" >/dev/null 2>&1 &
SRV=$!
sleep 2

if ! kill -0 "$SRV" 2>/dev/null; then
    echo "GAGAL: server tidak menyala"
    exit 1
fi

lulus=0
gagal=0

periksa() {
    if [ "$2" = "$3" ]; then
        echo "  ok   $1"
        lulus=$((lulus + 1))
    else
        echo "  GAGAL $1: diharapkan [$2], didapat [$3]"
        gagal=$((gagal + 1))
    fi
}

# --- kebenaran dekoding: jalur cepat tidak boleh melewatkan dekoding -------
periksa "spasi '+' didekode" "Budi Santoso" \
    "$(curl -s --max-time 10 -H 'Content-Type: application/x-www-form-urlencoded' \
         --data-binary 'a=Budi+Santoso' "http://127.0.0.1:$PORT/f")"

periksa "%XX didekode" "Hi!" \
    "$(curl -s --max-time 10 -H 'Content-Type: application/x-www-form-urlencoded' \
         --data-binary 'a=%48%69%21' "http://127.0.0.1:$PORT/f")"

# %ZZ bukan heks: dibiarkan apa adanya, bukan permintaan ditolak.
periksa "%XX tak sah dibiarkan" "%ZZ" \
    "$(curl -s --max-time 10 -H 'Content-Type: application/x-www-form-urlencoded' \
         --data-binary 'a=%ZZ' "http://127.0.0.1:$PORT/f")"

# '%' di ujung: tidak boleh membaca melewati akhir teks.
periksa "'%' di ujung aman" "ab%" \
    "$(curl -s --max-time 10 -H 'Content-Type: application/x-www-form-urlencoded' \
         --data-binary 'a=ab%' "http://127.0.0.1:$PORT/f")"

periksa "teks polos utuh" "polos" \
    "$(curl -s --max-time 10 -H 'Content-Type: application/x-www-form-urlencoded' \
         --data-binary 'a=polos' "http://127.0.0.1:$PORT/f")"

# --- bentuk pertumbuhan ----------------------------------------------------
# Badan dibuat PENUH '%41' supaya jalur cepat (tanpa % dan +) TIDAK terpakai;
# yang diukur adalah gelung dekode sebenarnya, bagian yang dulu kuadratik.
buat_badan() { # bita_hasil -> file
    local n="$1"
    { printf 'a='
      local i=0
      while [ "$i" -lt "$n" ]; do printf '%%41'; i=$((i + 1)); done
    } > "$TMP/b.bin"
}

ukur_ms() { # file -> milidetik
    local awal akhir
    awal="$(date +%s%N)"
    curl -s --max-time 120 -H 'Content-Type: application/x-www-form-urlencoded' \
         --data-binary @"$TMP/b.bin" "http://127.0.0.1:$PORT/besar" >/dev/null 2>&1
    akhir="$(date +%s%N)"
    echo $(((akhir - awal) / 1000000))
}

buat_badan 20000
t1="$(ukur_ms)"
buat_badan 40000
t2="$(ukur_ms)"

echo "  20k karakter %41: ${t1} ms"
echo "  40k karakter %41: ${t2} ms"

# Ukuran dua kali lipat: linear ~2x, kuadratik ~4x. Ambang 3x memberi ruang
# bagi derau penjadwalan tanpa melewatkan kembalinya O(n^2).
#
# Lantai 50 ms menghindari pembagian angka kecil yang berisik: kalau keduanya
# sudah di bawah itu, tidak ada masalah performa untuk dilaporkan.
if [ "$t1" -lt 50 ] && [ "$t2" -lt 50 ]; then
    echo "  ok   pertumbuhan linear (keduanya < 50 ms)"
    lulus=$((lulus + 1))
elif [ "$t2" -gt $((t1 * 3 + 50)) ]; then
    echo "  GAGAL pertumbuhan kuadratik: ${t1} ms -> ${t2} ms saat ukuran 2x"
    echo "        O(n^2) kembali di _web_dekode_persen (lihat pustaka/web.idk)"
    gagal=$((gagal + 1))
else
    echo "  ok   pertumbuhan linear: ${t1} ms -> ${t2} ms saat ukuran 2x"
    lulus=$((lulus + 1))
fi

kill "$SRV" 2>/dev/null
SRV=""

echo "=================================================="
echo "web_badan_performa: $lulus lulus, $gagal gagal"
[ "$gagal" -eq 0 ] || exit 1
exit 0
