#!/usr/bin/env bash
# Cache parsing templat (SSR-08).
#
# Parsing adalah 73,7% biaya tiap render (diukur `indk profil`), dan server
# mengurai berkas yang sama untuk SETIAP permintaan HTTP. Yang diuji di sini:
# hasil tetap identik, berkas yang berubah ikut terlihat, dan cache tidak
# tumbuh tanpa batas.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
[ -x "$INDK" ] || INDK="$AKAR/indk.exe"
KERJA="$(mktemp -d)"
TPL="$KERJA/tpl"
mkdir -p "$TPL"
trap 'rm -rf "$KERJA"' EXIT

lulus=0
gagal=0

# indk.exe menulis CRLF; tanpa ini setiap perbandingan gagal di Windows
# walaupun nilainya benar.
bersihkan() { tr -d '\r'; }

periksa() {
    local nama="$1" harap="$2" dapat="$3"
    if [ "$harap" = "$dapat" ]; then
        lulus=$((lulus + 1))
        echo "  OK   $nama"
    else
        gagal=$((gagal + 1))
        echo "  GAGAL $nama"
        echo "        harap : $harap"
        echo "        dapat : $dapat"
    fi
}

jalankan() { "$INDK" "$1" 2>&1 | bersihkan; }

cat > "$TPL/dasar.html" <<'EOF'
<html><body>{% blok isi %}kosong{% akhirblok %}</body></html>
EOF
cat > "$TPL/komponen.html" <<'EOF'
<i>{{ nama }}</i>
EOF
cat > "$TPL/hal.html" <<'EOF'
{% perluas "dasar.html" %}{% blok isi %}<p>{{ pesan }}</p>{% sertakan "komponen.html" %}{% akhirblok %}
EOF

# --- 1. hasil dengan cache identik dengan tanpa cache ----------------------
cat > "$KERJA/sama.idk" <<EOF
impor templat
variabel a = templat.mesin("$TPL", benar)
variabel b = templat.mesin("$TPL", salah)
variabel d = kamus()
d["pesan"] = "halo <&> 'x'"
d["nama"] = "budi ☕"
variabel sama = benar
untuk i dalam rentang(0, 20) {
    jika a.render("hal.html", d) != b.render("hal.html", d) { sama = salah }
}
cetak(sama)
cetak(a.render("hal.html", d))
EOF
keluar="$(jalankan "$KERJA/sama.idk")"
periksa "hasil cache == tanpa cache" "benar" "$(echo "$keluar" | sed -n 1p)"
periksa "escaping tetap berlaku lewat cache" \
    "<html><body><p>halo &lt;&amp;&gt; &#39;x&#39;</p><i>budi ☕</i></body></html>" \
    "$(echo "$keluar" | sed -n '2,$p' | tr -d '\n')"

# --- 2. berkas yang berubah harus terlihat ---------------------------------
# Inti SSR-08: cache tanpa invalidasi membuat penyuntingan templat tidak
# berefek sampai server di-restart.
cat > "$KERJA/ubah.idk" <<EOF
impor templat
variabel m = templat.mesin("$TPL")
variabel d = kamus()
d["pesan"] = "satu"
d["nama"] = "k"
cetak("[R]" + m.render("hal.html", d))
tulis_semua("$TPL/hal.html", "{% blok isi %}VERSI-DUA{% akhirblok %}")
cetak("[R]" + m.render("hal.html", d))
EOF
# Render bisa memuat baris baru; [R] menandai awal tiap render.
keluar="$(jalankan "$KERJA/ubah.idk" | tr -d '\n' | sed 's/\[R\]/\n/g')"
periksa "render pertama memakai isi awal" \
    "<html><body><p>satu</p><i>k</i></body></html>" "$(echo "$keluar" | sed -n 2p)"
periksa "berkas yang disunting langsung terlihat" \
    "VERSI-DUA" "$(echo "$keluar" | sed -n 3p)"

# Kembalikan untuk uji berikutnya.
cat > "$TPL/hal.html" <<'EOF'
{% perluas "dasar.html" %}{% blok isi %}<p>{{ pesan }}</p>{% sertakan "komponen.html" %}{% akhirblok %}
EOF

# --- 3. komponen yang di-sertakan juga ikut ter-invalidasi -----------------
# Jalur {% sertakan %} melewati _render_berkas() yang sama, tapi cap berkasnya
# berbeda — tanpa pemeriksaan per berkas, komponen akan membeku.
cat > "$KERJA/komp.idk" <<EOF
impor templat
variabel m = templat.mesin("$TPL")
variabel d = kamus()
d["nama"] = "awal"
cetak(m.render("komponen.html", d))
tulis_semua("$TPL/komponen.html", "<b>{{ nama }}</b>")
cetak(m.render("komponen.html", d))
EOF
keluar="$(jalankan "$KERJA/komp.idk")"
periksa "komponen awal" "<i>awal</i>" "$(echo "$keluar" | sed -n 1p)"
periksa "komponen yang disunting ikut terlihat" "<b>awal</b>" "$(echo "$keluar" | sed -n '2,$p' | tr -d '\n')"
cat > "$TPL/komponen.html" <<'EOF'
<i>{{ nama }}</i>
EOF


# --- 3b. suntingan dengan UKURAN SAMA tetap terlihat -----------------------
# Kalau cap hanya memakai ukuran berkas, penggantian isi yang panjangnya sama
# (koreksi typo, ganti satu angka) tidak akan pernah terlihat.
cat > "$TPL/sama_ukuran.html" <<'EOF'
<p>AAA</p>
EOF
# Tidak ada builtin sleep, jadi jeda dibuat dengan gelung sibuk atas waktu()
# supaya mtime benar-benar bergeser pada sistem berkas beresolusi detik.
cat > "$KERJA/ukuran.idk" <<EOF
impor templat
variabel m = templat.mesin("$TPL")
cetak("[R]" + m.render("sama_ukuran.html", kamus()))
variabel mulai = waktu()
selagi waktu() - mulai < 1.1 { }
tulis_semua("$TPL/sama_ukuran.html", "<p>BBB</p>\n")
cetak("[R]" + m.render("sama_ukuran.html", kamus()))
EOF
keluar="$(jalankan "$KERJA/ukuran.idk" | tr -d '\n' | sed 's/\[R\]/\n/g')"
periksa "isi awal (ukuran sama)" "<p>AAA</p>" "$(echo "$keluar" | sed -n 2p)"
periksa "suntingan berukuran sama tetap terlihat" "<p>BBB</p>" "$(echo "$keluar" | sed -n 3p)"

# --- 4. berkas hilang tetap menaikkan galat yang sama ----------------------
# Entri cache tidak boleh membuat templat yang sudah dihapus tetap tersaji.
cp "$TPL/komponen.html" "$TPL/sementara.html"
cat > "$KERJA/hilang.idk" <<EOF
impor templat
variabel m = templat.mesin("$TPL")
variabel d = kamus()
d["nama"] = "x"
cetak(m.render("sementara.html", d))
hapus_berkas("$TPL/sementara.html")
coba {
    cetak(m.render("sementara.html", d))
} kecuali Galat sebagai e {
    cetak("GALAT")
}
EOF
keluar="$(jalankan "$KERJA/hilang.idk")"
periksa "render sebelum berkas dihapus" "<i>x</i>" "$(echo "$keluar" | sed -n 1p)"
periksa "berkas dihapus tetap menaikkan galat" "1" "$(echo "$keluar" | grep -c '^GALAT$')"

# --- 5. cache tidak tumbuh tanpa batas -------------------------------------
cat > "$KERJA/batas.idk" <<EOF
impor templat
variabel m = templat.mesin("$TPL")
untuk i dalam rentang(0, 200) {
    variabel n = "gen" + ke_teks(i) + ".html"
    tulis_semua("$TPL/" + n, "<p>" + ke_teks(i) + "</p>")
    m.render(n, kamus())
}
cetak(panjang(m.cache) <= 64)
cetak(m.render("gen199.html", kamus()))
m.kosongkan_cache()
cetak(panjang(m.cache))
EOF
keluar="$(jalankan "$KERJA/batas.idk")"
periksa "cache dibatasi 64 entri" "benar" "$(echo "$keluar" | sed -n 1p)"
periksa "render tetap benar setelah cache penuh" "<p>199</p>" "$(echo "$keluar" | sed -n 2p)"
periksa "kosongkan_cache() mengosongkan" "0" "$(echo "$keluar" | sed -n 3p)"
rm -f "$TPL"/gen*.html

# --- 6. cache memang menghemat kerja ---------------------------------------
# Tanpa asersi ini, cache yang tidak pernah kena tetap lolos semua uji di atas.
hitung() {
    "$INDK" profil "$1" 2>&1 >/dev/null | bersihkan \
        | grep instruksi | tr -cd '0-9'
}
buat_beban() {
    cat > "$KERJA/beban.idk" <<EOF
impor templat
variabel m = templat.mesin("$TPL", $2)
variabel d = kamus()
d["pesan"] = "x"
d["nama"] = "y"
untuk i dalam rentang(0, $1) { m.render("hal.html", d) }
EOF
}
buat_beban 1 benar;  c1="$(hitung "$KERJA/beban.idk")"
buat_beban 51 benar; c51="$(hitung "$KERJA/beban.idk")"
buat_beban 1 salah;  t1="$(hitung "$KERJA/beban.idk")"
buat_beban 51 salah; t51="$(hitung "$KERJA/beban.idk")"
if [ -n "$c1" ] && [ -n "$t1" ]; then
    per_cache=$(( (c51 - c1) / 50 ))
    per_tanpa=$(( (t51 - t1) / 50 ))
    echo "  info: per render — cache $per_cache vs tanpa cache $per_tanpa instruksi"
    # Ambang longgar (separuh): yang dijaga adalah "cache benar-benar dipakai",
    # bukan angka persis yang bisa bergeser saat templat uji diubah.
    if [ "$per_cache" -lt $(( per_tanpa / 2 )) ]; then
        lulus=$((lulus + 1)); echo "  OK   cache memangkas >50% instruksi per render"
    else
        gagal=$((gagal + 1)); echo "  GAGAL cache tidak memangkas kerja ($per_cache vs $per_tanpa)"
    fi
else
    echo "  LEWAT pengukuran instruksi (profil tidak tersedia)"
fi

echo ""
echo "$lulus lulus, $gagal gagal"
[ "$gagal" -eq 0 ]
