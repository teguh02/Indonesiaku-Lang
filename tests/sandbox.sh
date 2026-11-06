#!/usr/bin/env bash
# Mode --sandbox: batas yang dijanjikan benar-benar ditegakkan.
#
# Playground web menjalankan SETIAP program pengguna dengan mode ini, jadi
# yang diuji di sini adalah batas keamanan, bukan kenyamanan. Sebelum tes ini
# ada, `soket_dengar()` lolos sepenuhnya: program sandbox bisa memasang
# listener dan memakai port host, padahal `soket_sambung()` sudah ditolak
# sejak awal.
#
# Yang TIDAK dijanjikan mode ini juga diuji (baca/tulis berkas tetap boleh),
# supaya batasnya tercatat dan tidak dikira lebih kuat dari kenyataan.

set -u

SKRIP_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$SKRIP_DIR/.." && pwd)"
cd "$REPO" || exit 1

INDK="./indk"
[ -x "$INDK" ] || INDK="./indk.exe"
if [ ! -x "$INDK" ]; then
    echo "indk tidak ditemukan; jalankan make dulu" >&2
    exit 1
fi

GREEN='\033[0;32m'; RED='\033[0;31m'; NC='\033[0m'
LULUS=0; GAGAL=0

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

periksa() {
    local nama="$1" harap="$2" dapat="$3"
    if [ "$harap" = "$dapat" ]; then
        echo -e "  ${GREEN}OK${NC}   $nama"
        LULUS=$((LULUS+1))
    else
        echo -e "  ${RED}GAGAL${NC} $nama"
        echo "        harap : $harap"
        echo "        dapat : $dapat"
        GAGAL=$((GAGAL+1))
    fi
}

# jalankan <mode> <kode> -> stdout, satu baris
jalankan() {
    local mode="$1" kode="$2"
    printf '%s\n' "$kode" > "$TMP/uji.idk"
    if [ "$mode" = "sandbox" ]; then
        "$INDK" --sandbox "$TMP/uji.idk" 2>&1 | tr -d '\r'
    else
        "$INDK" "$TMP/uji.idk" 2>&1 | tr -d '\r'
    fi
}

# Pembungkus: cetak "ditolak" bila GalatIzin, "JALAN" bila berhasil. Yang
# diperiksa adalah KELAS galatnya, bukan sekadar "ada galat" -- argumen salah
# atau jaringan mati juga memunculkan galat, dan itu bukan bukti sandbox.
# Catatan bahasa: satu blok `coba` tidak boleh punya `kecuali <Tipe>` DAN
# `kecuali e` sekaligus ("'kecuali' tanpa tipe harus menjadi yang terakhir"
# hanya berlaku untuk satu penangan). Jadi ditangkap bertipe, lalu nama
# kelasnya dicetak -- itu sekaligus membuktikan kelas galatnya, bukan cuma
# bahwa ada galat.
bungkus() {
    cat <<EOF
coba {
    $1
    cetak("JALAN")
} kecuali GalatIzin {
    cetak("ditolak")
}
EOF
}

echo "Menguji mode --sandbox"
echo "================================================="

# --- Yang WAJIB ditolak ------------------------------------------------------

for pasangan in \
    "lingkungan:lingkungan()" \
    "ambil_lingkungan:ambil_lingkungan(\"HOME\")" \
    "isi_direktori:isi_direktori(\".\")" \
    "soket_sambung:soket_sambung(\"example.com\", 80)" \
    "soket_dengar:soket_dengar(18087)"
do
    nama="${pasangan%%:*}"
    ekspr="${pasangan#*:}"
    kode="$(bungkus "$ekspr")"

    # Di mode normal harus JALAN -- kalau tidak, "ditolak" di bawah bisa saja
    # berasal dari sebab lain dan tesnya tidak membuktikan apa pun.
    periksa "$nama jalan tanpa sandbox" "JALAN" "$(jalankan normal "$kode")"
    periksa "$nama ditolak dalam sandbox" "ditolak" "$(jalankan sandbox "$kode")"
done

# proses.jalankan ada di modul, jadi butuh impor.
kode_proses="$(cat <<'EOF'
impor proses
coba {
    proses.jalankan(["echo", "x"])
    cetak("JALAN")
} kecuali GalatIzin {
    cetak("ditolak")
}
EOF
)"
periksa "proses.jalankan jalan tanpa sandbox" "JALAN" "$(jalankan normal "$kode_proses")"
periksa "proses.jalankan ditolak dalam sandbox" "ditolak" "$(jalankan sandbox "$kode_proses")"

# --- Galat yang benar --------------------------------------------------------

# GalatIzin adalah turunan GalatOS, jadi penangan GalatOS yang sudah ada di
# kode pengguna ikut menangkapnya.
kode_warisan="$(cat <<'EOF'
coba {
    isi_direktori(".")
    cetak("JALAN")
} kecuali GalatOS {
    cetak("tertangkap GalatOS")
}
EOF
)"
periksa "GalatIzin tertangkap sebagai GalatOS" "tertangkap GalatOS" \
    "$(jalankan sandbox "$kode_warisan")"

# --- Batas yang DISADARI: sandbox bukan jail filesystem ----------------------
#
# Berkas tetap bisa dibaca dan ditulis. Ini bukan kelalaian tes: batas
# filesystem playground ditegakkan container, bukan flag ini. Diuji supaya
# perubahan diam-diam pada janji itu terlihat.

kode_tulis="$(bungkus "tulis_semua(\"$TMP/bocor.txt\", \"x\")")"
periksa "tulis berkas TETAP boleh (dijaga container, bukan flag)" "JALAN" \
    "$(jalankan sandbox "$kode_tulis")"

kode_baca="$(bungkus "baca_semua(\"$TMP/bocor.txt\")")"
periksa "baca berkas TETAP boleh (dijaga container, bukan flag)" "JALAN" \
    "$(jalankan sandbox "$kode_baca")"

# --- Flag hanya dikenali sebelum path skrip ---------------------------------
#
# Supaya skrip tetap bisa menerima argumennya sendiri yang kebetulan bernama
# "--sandbox" tanpa diam-diam mematikan/mengaktifkan mode.
# `argumen` adalah variabel global (bukan fungsi): daftar argumen skrip.
printf 'cetak(argumen[0])\n' > "$TMP/arg.idk"
periksa "argumen --sandbox milik skrip tidak dirampas" "--sandbox" \
    "$("$INDK" "$TMP/arg.idk" --sandbox 2>&1 | tr -d '\r')"

echo "================================================="
echo -e "sandbox: ${GREEN}$LULUS lulus${NC}, ${RED}$GAGAL gagal${NC}"
[ "$GAGAL" -eq 0 ] || exit 1
