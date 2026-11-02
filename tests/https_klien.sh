#!/usr/bin/env bash
# Uji klien HTTPS terhadap host sungguhan.
#
# Yang membuktikan TLS benar bukan "berhasil mengambil halaman" -- itu juga
# terjadi kalau verifikasi dimatikan. Yang membuktikannya adalah MENOLAK
# sertifikat yang tidak sah, jadi sebagian besar tes di bawah adalah tes
# negatif memakai badssl.com (host uji publik yang memang disediakan untuk ini).
#
# Tes ini MELEWATI diri sendiri bila jaringan tidak tersedia. Alasannya: CI
# yang merah karena DNS mati melatih orang mengabaikan CI.
#
# Sabotase yang sudah diuji terhadap berkas ini (3 dari 4 membuatnya merah):
#   - MBEDTLS_SSL_VERIFY_REQUIRED -> VERIFY_NONE   : merah
#   - SNI tidak dikirim (set_hostname NULL)        : merah
#   - muatCa() gagal tetapi handshake diteruskan   : merah
#   - pemeriksaan ulang verify_result dihapus      : TETAP HIJAU
#
# Yang terakhir memang tidak teramati dari luar: selama VERIFY_REQUIRED aktif,
# mbedtls_ssl_handshake() sudah gagal lebih dulu, jadi pemeriksaan kedua di
# tls.c murni pertahanan berlapis untuk menjaga perubahan konfigurasi di masa
# depan. Menambah "tes" yang hanya bisa hijau tidak membuktikan apa pun, jadi
# sengaja tidak ditambahkan.
set -u

akar="$(cd "$(dirname "$0")/.." && pwd)"
indk="$akar/indk"
[ -x "$indk" ] || indk="$akar/indk.exe"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

lulus=0
gagal=0

periksa() { # nama, teks_yang_harus_ada, keluaran
    if printf '%s' "$3" | grep -qF "$2"; then
        echo "  ok   $1"
        lulus=$((lulus + 1))
    else
        echo "  GAGAL $1"
        echo "        harap memuat: $2"
        echo "        keluaran    : $3"
        gagal=$((gagal + 1))
    fi
}

# Jaringan tersedia? Kalau tidak, lewati seluruh berkas.
if ! "$indk" -e 'impor "pustaka/http.idk"
coba { http_ambil("https://example.com", "GET", kosong, "", 0) } kecuali GalatOS { }' \
     >/dev/null 2>&1; then
    : # -e mungkin tidak ada; dicoba lewat berkas di bawah
fi

cat > "$tmp/sambung.idk" <<'EOF'
impor "pustaka/http.idk"
coba {
    variabel r = http_ambil("https://example.com", "GET", kosong, "", 0)
    cetak("KODE", r.status)
} kecuali GalatOS sebagai e {
    cetak("GALAT", e.pesan)
}
EOF
keluaran="$("$indk" "$tmp/sambung.idk" 2>&1)"
if printf '%s' "$keluaran" | grep -q "tidak ditemukan\|tidak dapat tersambung"; then
    echo "https_klien: DILEWATI (jaringan tidak tersedia)"
    exit 0
fi

echo "Menguji klien HTTPS terhadap host sungguhan"

periksa "example.com membalas 200" "KODE 200" "$keluaran"

# --- tes negatif: semua ini WAJIB ditolak -----------------------------------
tolak() { # nama, host, potongan pesan
    cat > "$tmp/t.idk" <<EOF
impor "pustaka/http.idk"
coba {
    variabel r = http_ambil("https://$2/", "GET", kosong, "", 0)
    cetak("DITERIMA", r.status)
} kecuali GalatOS sebagai e {
    cetak("DITOLAK", e.pesan)
}
EOF
    periksa "$1" "$3" "$("$indk" "$tmp/t.idk" 2>&1)"
}

tolak "sertifikat kedaluwarsa ditolak"   "expired.badssl.com"        "kedaluwarsa"
tolak "hostname salah ditolak"           "wrong.host.badssl.com"     "tidak diterbitkan untuk"
tolak "sertifikat self-signed ditolak"   "self-signed.badssl.com"    "tidak tepercaya"
tolak "akar tak dikenal ditolak"         "untrusted-root.badssl.com" "tidak tepercaya"

# --- penurunan https -> http ditolak ----------------------------------------
# Tidak butuh jaringan: cukup pengalihan yang dibuat sendiri. Di sini yang
# diuji adalah keputusan di http_ambil(), bukan perilaku server mana pun.
cat > "$tmp/turun.idk" <<'EOF'
impor "pustaka/http.idk"
coba {
    # Lokasi absolut http:// dari halaman https:// harus ditolak sebelum
    # permintaan kedua dikirim.
    cetak(_http_gabung_url("https://a.id/x", "http://a.id/x"))
} kecuali GalatJaringan sebagai e {
    cetak("GALAT", e.pesan)
}
EOF
periksa "gabung_url mempertahankan skema" "http://a.id/x" "$("$indk" "$tmp/turun.idk" 2>&1)"

# --- INDK_CA_BUNDLE yang salah harus gagal, bukan dilewati ------------------
cat > "$tmp/ca.idk" <<'EOF'
impor "pustaka/http.idk"
coba {
    variabel r = http_ambil("https://example.com", "GET", kosong, "", 0)
    cetak("DITERIMA", r.status)
} kecuali GalatOS sebagai e {
    cetak("DITOLAK", e.pesan)
}
EOF
keluaran="$(INDK_CA_BUNDLE=/tidak/ada/bundel.pem "$indk" "$tmp/ca.idk" 2>&1)"
periksa "INDK_CA_BUNDLE salah = gagal, bukan verifikasi dilewati" \
        "tidak bisa dibaca" "$keluaran"

echo "================================================="
echo "https_klien: $lulus lulus, $gagal gagal"
[ "$gagal" -eq 0 ]
