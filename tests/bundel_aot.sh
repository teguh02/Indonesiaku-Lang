#!/usr/bin/env bash
# Bundel AOT V1: tempel skrip tunggal ke biner indk, jalankan tanpa argumen.
set -u
AKAR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$AKAR" || exit 1
INDK="./indk"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
GAGAL=0

ok() { echo "  ok   $1"; }
fail() { echo "  GAGAL $1"; GAGAL=1; }

# 1. Bundel sederhana -> keluaran identik dengan run biasa.
cat > "$TMP/hello.idk" << 'EOF'
cetak("halo dari bundel")
cetak(1 + 2)
EOF
bash scripts/bangun_bundel.sh "$INDK" "$TMP/hello.idk" "$TMP/app1" > /dev/null
HASIL_BUNDEL="$("$TMP/app1")"
HASIL_NORMAL="$("$INDK" "$TMP/hello.idk")"
[ "$HASIL_BUNDEL" = "$HASIL_NORMAL" ] && ok "keluaran bundel identik dengan run biasa" \
    || fail "keluaran bundel BEDA dari run biasa"

# 2. Argumen sendiri bundel tidak disalahartikan sebagai flag indk.
cat > "$TMP/arg.idk" << 'EOF'
cetak(argumen)
EOF
bash scripts/bangun_bundel.sh "$INDK" "$TMP/arg.idk" "$TMP/app2" > /dev/null
HASIL="$("$TMP/app2" --nama=Budi 123)"
echo "$HASIL" | grep -q -- "--nama=Budi" && ok "argumen bundel diteruskan apa adanya" \
    || fail "argumen bundel hilang/disalahartikan: $HASIL"

# 3. Biner indk TANPA payload tetap jalan normal (regresi nol).
VERSI="$("$INDK" -v)"
[ -n "$VERSI" ] && ok "indk -v masih normal setelah fitur bundel ditambah" \
    || fail "indk -v rusak"
# macOS tidak punya `timeout` bawaan; pola yang sama dengan test_all.sh.
BATAS=""
command -v timeout >/dev/null 2>&1 && BATAS="timeout 2"
echo 'cetak("repl ok")' | $BATAS "$INDK" > "$TMP/repl.out" 2>&1
grep -q "repl ok" "$TMP/repl.out" && ok "REPL biasa (argc==1, tanpa bundel) masih jalan" \
    || fail "REPL rusak: $(cat "$TMP/repl.out")"

# 4. Skrip rusak -> gagal bersih (exit 65), bukan crash.
echo 'cetak(' > "$TMP/rusak.idk"
bash scripts/bangun_bundel.sh "$INDK" "$TMP/rusak.idk" "$TMP/app3" > /dev/null
"$TMP/app3" > /dev/null 2>&1
RC=$?
[ "$RC" -eq 65 ] && ok "skrip rusak dalam bundel -> exit 65 (bukan crash)" \
    || fail "skrip rusak: exit $RC, harap 65"

# 5. Berkas sementara dibersihkan setelah dijalankan.
BEFORE=$(find "${TMPDIR:-/tmp}" -maxdepth 1 -name 'indk_bundel_*.idk' 2>/dev/null | wc -l)
"$TMP/app1" > /dev/null
AFTER=$(find "${TMPDIR:-/tmp}" -maxdepth 1 -name 'indk_bundel_*.idk' 2>/dev/null | wc -l)
[ "$BEFORE" -eq "$AFTER" ] && ok "berkas sementara tidak menumpuk setelah run" \
    || fail "berkas sementara tertinggal ($BEFORE -> $AFTER)"

# 6. Keterbatasan V1 yang terdokumentasi: impor modul eksternal gagal BERSIH,
#    bukan diam-diam salah atau crash.
cat > "$TMP/pakai_impor.idk" << 'EOF'
impor "pustaka/teks.idk"
cetak(teks.teks_pisah("a,b", ","))
EOF
bash scripts/bangun_bundel.sh "$INDK" "$TMP/pakai_impor.idk" "$TMP/app4" > /dev/null
OUT4="$("$TMP/app4" 2>&1)"
RC4=$?
echo "$OUT4" | grep -q "tidak terdefinisi" && [ "$RC4" -ne 0 ] && \
    ok "keterbatasan V1 (impor eksternal) gagal bersih, bukan crash" \
    || fail "perilaku impor eksternal berubah tak terduga: rc=$RC4 out=$OUT4"

if [ "$GAGAL" -eq 0 ]; then
    echo "bundel_aot.sh: SEMUA LULUS"
    exit 0
fi
echo "bundel_aot.sh: ADA YANG GAGAL"
exit 1
