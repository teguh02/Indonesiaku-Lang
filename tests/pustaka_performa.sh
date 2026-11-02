#!/usr/bin/env bash
# Pustaka yang mengolah input dari luar harus LINEAR, bukan kuadratik.
#
# Latar: pola `hasil = hasil + bagian` di dalam gelung menyalin seluruh string
# yang sudah terkumpul pada tiap iterasi, karena teks di Indonesiaku immutable.
# Biayanya O(n^2). Itu bukan sekadar lambat: json_teks() menyusun BALASAN
# server dan bita.ke_teks() mendekode DATA DARI SOKET, sedangkan gelung server
# melayani satu permintaan pada satu waktu -- jadi satu payload besar
# membekukan seluruh proses. Penolakan layanan dari satu permintaan yang sah.
#
# Yang diperiksa di sini BENTUK PERTUMBUHAN (2x ukuran -> ~2x waktu), bukan
# ambang detik: runner CI yang lambat tidak boleh membuat tes merah, tetapi
# kembalinya O(n^2) harus. Ambang 3,0x dipilih karena linear ~2,0x dan
# kuadratik ~4,0x; jarak ke keduanya cukup lebar untuk mesin yang berisik.
set -u

INDK="${INDK:-./indk}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
GAGAL=0

# Jalankan skrip dengan N tertentu, kembalikan durasi dalam milidetik.
ukur() {
    local berkas="$1" n="$2" mulai akhir
    mulai=$(date +%s%N)
    if ! N="$n" "$INDK" "$berkas" > "$TMP/keluaran" 2>"$TMP/galat"; then
        echo "  GAGAL dijalankan (N=$n): $(head -2 "$TMP/galat")" >&2
        return 1
    fi
    akhir=$(date +%s%N)
    echo $(( (akhir - mulai) / 1000000 ))
}

# periksa <nama> <berkas> -- bandingkan waktu pada N dan 2N.
periksa() {
    local nama="$1" berkas="$2" n="$3"
    local t1 t2 rasio
    t1=$(ukur "$berkas" "$n")       || { GAGAL=$((GAGAL+1)); return; }
    t2=$(ukur "$berkas" $((n * 2))) || { GAGAL=$((GAGAL+1)); return; }

    # Di bawah 40 ms pengukuran didominasi derau penjadwal dan waktu start
    # juru bahasa; rasionya tidak berarti apa-apa. Naikkan N, jangan tebak.
    if [ "$t2" -lt 40 ]; then
        echo "  LEWAT $nama: terlalu cepat untuk diukur (${t1}ms -> ${t2}ms)"
        return
    fi

    rasio=$(( t2 * 100 / (t1 > 0 ? t1 : 1) ))
    if [ "$rasio" -gt 300 ]; then
        echo "  GAGAL $nama: ${t1}ms -> ${t2}ms = ${rasio}% (kuadratik; linear ~200%)"
        GAGAL=$((GAGAL+1))
    else
        echo "  OK    $nama: ${t1}ms -> ${t2}ms = ${rasio}%"
    fi
}

echo "Performa pustaka (pertumbuhan linear):"

# json_teks: menyusun balasan server dari daftar dan kamus.
cat > "$TMP/json_daftar.idk" <<'EOF'
impor "pustaka/json.idk"
variabel n = ke_bulat(ambil_lingkungan("N"))
variabel d = daftar()
untuk i dalam rentang(0, n) { tambah(d, "nilai") }
cetak(panjang(json_teks(d)))
EOF

# json_escape: string panjang yang penuh karakter wajib-escape.
cat > "$TMP/json_escape.idk" <<'EOF'
impor "pustaka/json.idk"
variabel n = ke_bulat(ambil_lingkungan("N"))
variabel bagian = daftar()
untuk i dalam rentang(0, n) { tambah(bagian, "a\"b\\c") }
cetak(panjang(json_teks(teks_gabung(bagian, ""))))
EOF

# bita.ke_teks: mendekode UTF-8 yang datang dari soket.
cat > "$TMP/bita_teks.idk" <<'EOF'
impor bita
variabel n = ke_bulat(ambil_lingkungan("N"))
variabel bagian = daftar()
untuk i dalam rentang(0, n) { tambah(bagian, "x") }
variabel d = bita.ke_bita(teks_gabung(bagian, ""))
cetak(panjang(bita.ke_teks(d)))
EOF

# base64.enkode: lampiran dan Basic auth.
cat > "$TMP/b64.idk" <<'EOF'
impor base64
impor bita
variabel n = ke_bulat(ambil_lingkungan("N"))
variabel bagian = daftar()
untuk i dalam rentang(0, n) { tambah(bagian, "x") }
cetak(panjang(base64.enkode(bita.ke_bita(teks_gabung(bagian, "")))))
EOF

periksa "json_teks(daftar)" "$TMP/json_daftar.idk" 20000
periksa "json_escape"       "$TMP/json_escape.idk" 20000
periksa "bita.ke_teks"      "$TMP/bita_teks.idk"   40000
periksa "base64.enkode"     "$TMP/b64.idk"         40000

# Cepat saja tidak cukup: keluaran harus tetap benar. Nilai di bawah sengaja
# berisi tanda kutip, backslash, baris baru, dan karakter non-ASCII multi-byte
# -- justru cabang yang paling gampang rusak saat akumulasi string diubah.
echo "Kebenaran setelah perubahan:"
cat > "$TMP/benar.idk" <<'EOF'
impor "pustaka/json.idk"
impor base64
impor bita

# Bahasa ini tidak punya `tegaskan`; satu pembantu sudah cukup.
fungsi harus(syarat, nama) {
    jika syarat == salah {
        naikkan "penegasan gagal: " + nama
    }
}

# json: escape, nesting, daftar dan kamus kosong
harus(json_teks(daftar()) == "[]", "daftar kosong")
harus(json_teks(kamus()) == "{}", "kamus kosong")
harus(json_teks([1, "a", benar, salah, kosong]) == "[1,\"a\",true,false,null]", "tipe dasar")
harus(json_teks("a\"b\\c\nd\te\rf") == "\"a\\\"b\\\\c\\nd\\te\\rf\"", "escape")
harus(json_teks([[], [[]]]) == "[[],[[]]]", "nesting")

# round-trip lewat pengurai: bukti keluaran benar-benar JSON yang sah
variabel k = kamus()
k["teks"] = "baris\nbaru \"kutip\" dan \\ slash"
k["daftar"] = [1, 2, 3]
harus(json_teks(json_urai(json_teks(k))) == json_teks(k), "round-trip json")

# base64: ketiga panjang padding
harus(base64.enkode(bita.ke_bita("")) == "", "b64 kosong")
harus(base64.enkode(bita.ke_bita("a")) == "YQ==", "b64 pad 2")
harus(base64.enkode(bita.ke_bita("ab")) == "YWI=", "b64 pad 1")
harus(base64.enkode(bita.ke_bita("abc")) == "YWJj", "b64 pad 0")

# UTF-8 multi-byte: 2, 3, dan 4 byte per karakter
variabel s = "Halo éè 世界 😀"
harus(bita.ke_teks(bita.ke_bita(s)) == s, "utf8 round-trip")
harus(base64.dekode_teks(base64.enkode(bita.ke_bita(s))) == s, "b64 utf8")
harus(bita.ke_heksa(bita.ke_bita("abc")) == "616263", "heksa")

cetak("semua benar")
EOF

if KELUARAN=$("$INDK" "$TMP/benar.idk" 2>&1) && [ "$KELUARAN" = "semua benar" ]; then
    echo "  OK    15 penegasan kebenaran"
else
    echo "  GAGAL penegasan kebenaran: $KELUARAN"
    GAGAL=$((GAGAL+1))
fi

if [ "$GAGAL" -gt 0 ]; then
    echo "GAGAL: $GAGAL pemeriksaan"
    exit 1
fi
echo "LULUS"
