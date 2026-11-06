#!/usr/bin/env bash
# Diagnosis dan ketahanan mesin templat (SSR-09).
#
# Dua hal yang dijaga:
#   1. Pesan galat menyebut BERKAS, bukan cuma "baris N". Dalam rantai
#      {% perluas %}/{% sertakan %}, "baris 2" saja tidak memberi tahu berkas
#      mana dari lima yang rusak.
#   2. Parsing linear terhadap jumlah tag. Batas 10.000 tag dulu baru tercapai
#      setelah 56 detik — batas yang menolak selambat itu bukan pelindung,
#      justru jalan DoS-nya.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
[ -x "$INDK" ] || INDK="$AKAR/indk.exe"
KERJA="$(mktemp -d)"
if command -v cygpath >/dev/null 2>&1; then
    KERJA="$(cygpath -m "$KERJA")"
fi
TPL="$KERJA/tpl"
mkdir -p "$TPL/bagian"
trap 'rm -rf "$KERJA"' EXIT

lulus=0
gagal=0

periksa() {
    local nama="$1" harap="$2" dapat="$3"
    if [ "$harap" = "$dapat" ]; then
        lulus=$((lulus + 1)); echo "  OK   $nama"
    else
        gagal=$((gagal + 1))
        echo "  GAGAL $nama"
        echo "        harap : $harap"
        echo "        dapat : $dapat"
    fi
}

jalankan() { "$INDK" "$1" 2>&1 | tr -d '\r'; }

# --- Berkas uji ------------------------------------------------------------
cat > "$TPL/dasar.html" <<'EOF'
<html>{% blok isi %}{% akhirblok %}</html>
EOF
# anak.html hanya 1 baris; galat ada di komponen yang disertakannya.
cat > "$TPL/anak.html" <<'EOF'
{% perluas "dasar.html" %}{% blok isi %}{% sertakan "bagian/komp.html" %}{% akhirblok %}
EOF
cat > "$TPL/bagian/komp.html" <<'EOF'
<p>oke</p>
<span>{{ tidak.ada }}</span>
EOF
cat > "$TPL/sintaks.html" <<'EOF'
<p>baik</p>
{% untuk x dalam %}
EOF
# Galat di INDUK, bukan di anak: anak.html -> dasar_rusak.html. Berkas yang
# disebut harus dasar_rusak.html, padahal render dimulai dari anak2.html dan
# berkas terluar itulah yang paling AKHIR menangkap galatnya.
cat > "$TPL/dasar_rusak.html" <<'EOF'
<html>
<body>{{ hilang.di.induk }}</body>
</html>
EOF
cat > "$TPL/anak2.html" <<'EOF'
{% perluas "dasar_rusak.html" %}{% blok isi %}x{% akhirblok %}
EOF
cat > "$TPL/siklus_a.html" <<'EOF'
{% sertakan "siklus_b.html" %}
EOF
cat > "$TPL/siklus_b.html" <<'EOF'
{% sertakan "siklus_a.html" %}
EOF

cat > "$KERJA/diag.idk" <<EOF
impor templat
variabel m = templat.mesin("$TPL")
fungsi coba_render(n) {
    coba {
        m.render(n, kamus())
        cetak("TIDAK ADA GALAT")
    } kecuali Galat sebagai e { cetak(ke_teks(e)) }
}
coba_render("anak.html")
coba_render("anak2.html")
coba_render("sintaks.html")
coba_render("siklus_a.html")
coba_render("../../../etc/passwd")
coba_render("tidak_ada.html")
EOF
keluar="$(jalankan "$KERJA/diag.idk")"

# --- 1. Galat menyebut berkas TERDALAM, bukan yang terluar -----------------
# Kesalahannya ada di bagian/komp.html baris 2, dirender lewat anak.html yang
# memperluas dasar.html. Menyebut "anak.html" akan mengirim orang ke berkas
# yang salah.
periksa "galat render menyebut berkas terdalam + baris" \
    "bagian/komp.html:baris 2: 'tidak' tidak ada di data" \
    "$(echo "$keluar" | sed -n 1p)"

# Galat sintaks terjadi saat PARSING, bukan render — jalur kode berbeda, dan
# justru ini yang paling sering dilihat saat menulis templat.
# Galat ada di INDUK (dasar_rusak.html baris 2). Render dimulai dari
# anak2.html, jadi berkas terluar itu yang paling akhir menangkap galatnya —
# kalau prefiks ditimpa berulang, yang tersebut akan "anak2.html" yang salah.
periksa "galat di induk menyebut induk, bukan berkas terluar" \
    "dasar_rusak.html:baris 2: 'hilang' tidak ada di data" \
    "$(echo "$keluar" | sed -n 2p)"

periksa "galat sintaks menyebut berkas + baris" \
    "sintaks.html:baris 2: bentuk harus '{% untuk x dalam y %}', dapat 'untuk x dalam'" \
    "$(echo "$keluar" | sed -n 3p)"

# --- 2. Galat yang sudah menyebut berkasnya sendiri tidak diberi prefiks ---
# "siklus_a.html:siklus templat: siklus_a.html -> ..." mengulang tanpa guna.
periksa "siklus tidak diberi prefiks ganda" \
    "siklus templat: siklus_a.html -> siklus_b.html -> siklus_a.html" \
    "$(echo "$keluar" | sed -n 4p)"
periksa "traversal ditolak tanpa prefiks ganda" \
    "nama templat tidak boleh mengandung '..': '../../../etc/passwd'" \
    "$(echo "$keluar" | sed -n 5p)"
# Jalur lengkap tetap muncul di sini: ini galat untuk PENGEMBANG di stderr,
# bukan badan respons HTTP (web.idk sudah menjaga itu, diuji terpisah).
periksa "templat hilang menyebut jalur" "1" \
    "$(echo "$keluar" | sed -n 6p | grep -c 'templat tidak ditemukan')"

# --- 3. Batas tetap berlaku ------------------------------------------------
cat > "$KERJA/batas.idk" <<'EOF'
impor templat
fungsi coba_urai(s) {
    coba {
        templat.urai(s)
        cetak("TIDAK ADA GALAT")
    } kecuali Galat sebagai e { cetak(ke_teks(e)) }
}
variabel bersarang = ""
untuk i dalam rentang(0, 100) { bersarang = bersarang + "{% jika ada %}" }
untuk i dalam rentang(0, 100) { bersarang = bersarang + "{% akhirjika %}" }
coba_urai(bersarang)

variabel banyak = ""
untuk i dalam rentang(0, 12000) { banyak = banyak + "{{ a }}" }
coba_urai(banyak)

coba_urai("{{ a }")
coba_urai("{% jika a %}")
EOF
keluar="$(jalankan "$KERJA/batas.idk")"
periksa "nesting melebihi batas ditolak" "templat terlalu dalam (maksimum 64)" \
    "$(echo "$keluar" | sed -n 1p)"
periksa "tag melebihi batas ditolak" "terlalu banyak tag (maksimum 10000)" \
    "$(echo "$keluar" | sed -n 2p)"
periksa "'{{' tanpa '}}' ditolak" "baris 1: '{{' tanpa '}}'" \
    "$(echo "$keluar" | sed -n 3p)"
periksa "blok tanpa penutup ditolak" "1" \
    "$(echo "$keluar" | sed -n 4p | grep -c 'tidak ditemukan sampai akhir berkas')"

# --- 4. XSS dan Unicode tetap aman lewat jalur berkas ---------------------
cat > "$TPL/xss.html" <<'EOF'
<p>{{ jahat }}</p><a href="{{ tautan }}">{{ nama }}</a>
EOF
cat > "$KERJA/xss.idk" <<EOF
impor templat
variabel m = templat.mesin("$TPL")
variabel d = kamus()
d["jahat"] = "<script>alert(1)</script>"
d["tautan"] = "\" onmouseover=\"jahat()"
d["nama"] = "Budi & Ani ☕ 世界"
cetak(m.render("xss.html", d))
EOF
periksa "XSS + Unicode ter-escape lewat berkas" \
    '<p>&lt;script&gt;alert(1)&lt;/script&gt;</p><a href="&quot; onmouseover=&quot;jahat()">Budi &amp; Ani ☕ 世界</a>' \
    "$(jalankan "$KERJA/xss.idk" | sed -n 1p)"

# --- 5. Parsing LINEAR, bukan kuadratik -----------------------------------
# Ini yang membuat batas 10.000 tag berguna. Tanpa asersi ini, parser bisa
# kembali kuadratik tanpa satu pun tes lain berubah warna.
hitung() {
    # grep 'instruksi' cocok dengan DUA baris ("instruksi : N" dan
    # "laju : N juta instruksi/detik"); tr -cd menggabungkannya jadi satu
    # angka raksasa. Ambil baris pertama saja.
    "$INDK" profil "$1" 2>&1 >/dev/null | tr -d '\r' \
        | grep 'instruksi *:' | head -1 | tr -cd '0-9'
}
buat() {
    cat > "$KERJA/perf.idk" <<EOF
impor templat
variabel t = ""
untuk i dalam rentang(0, $1) { t = t + "{{ a }}\n" }
templat.urai(t)
EOF
}
buat 1000; i1="$(hitung "$KERJA/perf.idk")"
buat 8000; i8="$(hitung "$KERJA/perf.idk")"
if [ -n "$i1" ] && [ -n "$i8" ]; then
    # Linear: 8x input -> ~8x instruksi. Kuadratik: ~64x. Ambang 16x memberi
    # ruang dua kali lipat untuk biaya tetap, tapi tetap jauh di bawah 64x.
    batas=$((i1 * 16))
    echo "  info: 1.000 tag = $i1 instruksi, 8.000 tag = $i8 (linear ~8x)"
    if [ "$i8" -lt "$batas" ]; then
        lulus=$((lulus + 1)); echo "  OK   parsing linear terhadap jumlah tag"
    else
        gagal=$((gagal + 1))
        echo "  GAGAL parsing kuadratik: $i8 >= $batas"
    fi
else
    echo "  LEWAT pengukuran instruksi (profil tidak tersedia)"
fi

echo ""
echo "templat_diagnosis: $lulus lulus, $gagal gagal"
[ "$gagal" -eq 0 ]
