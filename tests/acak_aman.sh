#!/usr/bin/env bash
# acak_bita() — sumber acak kriptografis.
#
# Tes inti di sini BUKAN "hasilnya acak" (mustahil dibuktikan dari luar),
# melainkan sifat yang gagal pada acak_bulat() dan jadi alasan builtin ini ada:
# dua proses indk yang dijalankan pada detik yang sama harus menghasilkan nilai
# BERBEDA. acak_bulat() memakai rand() yang di-seed srand(time(NULL)), sehingga
# ketiga prosesnya mengembalikan angka yang sama persis — ID sesi yang dibangun
# darinya dapat ditebak penyerang yang tahu kapan server start (header Date).
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$AKAR" || exit 1

INDK="./indk"
[ -x "$INDK" ] || INDK="./indk.exe"
if [ ! -x "$INDK" ]; then
    echo "GAGAL: biner indk tidak ditemukan"
    exit 1
fi

KERJA="tests/data/acak_aman"
rm -rf "$KERJA"
mkdir -p "$KERJA"

lulus=0
gagal=0
bersihkan() { tr -d '\r'; }

periksa() {
    # periksa <nama> <harapan> <dapat>
    if [ "$2" = "$3" ]; then
        lulus=$((lulus + 1))
    else
        echo "GAGAL: $1"
        echo "  harap : $2"
        echo "  dapat : $3"
        gagal=$((gagal + 1))
    fi
}

periksa_memuat() {
    # periksa_memuat <nama> <potongan> <teks>
    case "$3" in
        *"$2"*) lulus=$((lulus + 1)) ;;
        *)
            echo "GAGAL: $1"
            echo "  harap memuat : $2"
            echo "  dapat        : $3"
            gagal=$((gagal + 1))
            ;;
    esac
}

# ---------------------------------------------------------------------------
# 1. Bentuk hasil: panjang benar, tiap elemen bulat 0..255.
# ---------------------------------------------------------------------------
cat > "$KERJA/bentuk.idk" <<'EOF'
variabel b = acak_bita(32)
cetak(panjang(b))
variabel sah = benar
untuk x dalam b {
    jika x < 0 atau x > 255 {
        sah = salah
    }
    jika ke_teks(x) != ke_teks(bulat(x)) {
        sah = salah
    }
}
cetak(sah)
EOF
keluaran="$("$INDK" "$KERJA/bentuk.idk" 2>&1 | bersihkan | tr '\n' ',')"
periksa "panjang dan rentang elemen" "32,benar," "$keluaran"

# Batas bawah dan atas yang sah.
periksa "n=1 menghasilkan satu bita" "1" \
    "$(echo 'cetak(panjang(acak_bita(1)))' > "$KERJA/satu.idk"; "$INDK" "$KERJA/satu.idk" 2>&1 | bersihkan)"
periksa "n=1048576 masih diterima" "1048576" \
    "$(echo 'cetak(panjang(acak_bita(1048576)))' > "$KERJA/maks.idk"; "$INDK" "$KERJA/maks.idk" 2>&1 | bersihkan)"

# ---------------------------------------------------------------------------
# 2. Dua panggilan dalam SATU proses tidak boleh sama.
# ---------------------------------------------------------------------------
cat > "$KERJA/dalam_proses.idk" <<'EOF'
variabel a = acak_bita(16)
variabel b = acak_bita(16)
cetak(ke_teks(a) != ke_teks(b))
EOF
periksa "dua panggilan berbeda dalam satu proses" "benar" \
    "$("$INDK" "$KERJA/dalam_proses.idk" 2>&1 | bersihkan)"

# ---------------------------------------------------------------------------
# 3. INTI: proses terpisah pada detik yang sama harus berbeda.
#    Inilah yang gagal pada acak_bulat().
# ---------------------------------------------------------------------------
echo 'cetak(acak_bita(16))' > "$KERJA/lintas.idk"
a="$("$INDK" "$KERJA/lintas.idk" 2>&1 | bersihkan)"
b="$("$INDK" "$KERJA/lintas.idk" 2>&1 | bersihkan)"
c="$("$INDK" "$KERJA/lintas.idk" 2>&1 | bersihkan)"
if [ "$a" != "$b" ] && [ "$b" != "$c" ] && [ "$a" != "$c" ]; then
    lulus=$((lulus + 1))
else
    echo "GAGAL: proses terpisah menghasilkan nilai yang sama (sumber acak dapat ditebak)"
    echo "  a: $a"
    echo "  b: $b"
    echo "  c: $c"
    gagal=$((gagal + 1))
fi

# Pembanding yang mendokumentasikan alasan builtin ini ada: acak_bulat()
# MEMANG sama lintas proses. Kalau suatu saat acak_bulat ikut diperbaiki,
# tes ini sengaja TIDAK memerahkannya — hanya mencatat keadaannya.
echo 'cetak(acak_bulat(0, 999999999))' > "$KERJA/lama.idk"
la="$("$INDK" "$KERJA/lama.idk" 2>&1 | bersihkan)"
lb="$("$INDK" "$KERJA/lama.idk" 2>&1 | bersihkan)"
if [ "$la" = "$lb" ]; then
    echo "CATATAN: acak_bulat() tetap dapat direproduksi lintas proses ($la) — sesuai rancangan, pakai acak_bita() untuk keamanan."
fi

# ---------------------------------------------------------------------------
# 4. Sebaran kasar: 4096 bita tidak boleh terkumpul di segelintir nilai.
#    Menangkap implementasi macet (mis. selalu mengembalikan byte yang sama).
# ---------------------------------------------------------------------------
cat > "$KERJA/sebaran.idk" <<'EOF'
variabel b = acak_bita(4096)
variabel unik = himpunan()
untuk x dalam b {
    unik.tambah(x)
}
# 4096 bita acak menyentuh hampir semua dari 256 nilai; 200 sangat longgar
# tapi mustahil dicapai implementasi yang macet.
cetak(panjang(unik) > 200)
EOF
periksa "sebaran nilai tidak terkumpul" "benar" \
    "$("$INDK" "$KERJA/sebaran.idk" 2>&1 | bersihkan)"

# ---------------------------------------------------------------------------
# 5. Argumen tidak sah: galat yang bisa ditangkap, bukan fatal.
# ---------------------------------------------------------------------------
for kasus in "0" "-1" "1048577" "3.5"; do
    echo "cetak(acak_bita($kasus))" > "$KERJA/galat.idk"
    hasil="$("$INDK" "$KERJA/galat.idk" 2>&1 | bersihkan | head -1)"
    periksa_memuat "n=$kasus ditolak" "acak_bita(" "$hasil"
done

echo 'cetak(acak_bita("x"))' > "$KERJA/galat.idk"
periksa_memuat "tipe salah ditolak" "hanya menerima angka" \
    "$("$INDK" "$KERJA/galat.idk" 2>&1 | bersihkan | head -1)"

echo 'cetak(acak_bita())' > "$KERJA/galat.idk"
periksa_memuat "arity salah ditolak" "memerlukan 1 argumen" \
    "$("$INDK" "$KERJA/galat.idk" 2>&1 | bersihkan | head -1)"

# Galat harus bisa ditangkap coba/kecuali, bukan mematikan proses.
cat > "$KERJA/tangkap.idk" <<'EOF'
coba {
    acak_bita(0)
    cetak("TIDAK SEHARUSNYA SAMPAI SINI")
} kecuali e {
    cetak("tertangkap")
}
EOF
periksa "galat dapat ditangkap coba/kecuali" "tertangkap" \
    "$("$INDK" "$KERJA/tangkap.idk" 2>&1 | bersihkan)"

rm -rf "$KERJA"

echo "acak_aman: $lulus lulus, $gagal gagal"
[ "$gagal" -eq 0 ] || exit 1
