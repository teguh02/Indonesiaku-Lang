#!/bin/bash
# =================================================
#  INDONESIAKU - GOLDEN OUTPUT TEST SUITE (Linux/macOS)
#  Builds the project, runs every example, and asserts
#  stdout matches tests/expected/<name>.out.
#  Exits non-zero on any failure so CI can gate on it.
# =================================================

set -u

RED='\033[0;31m'; GREEN='\033[0;32m'; BLUE='\033[0;34m'; NC='\033[0m'

# Resolve repo root (parent of this script's dir)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(dirname "$SCRIPT_DIR")"
cd "$REPO"

echo -e "${BLUE}Building Indonesiaku...${NC}"
if [ ! -f Makefile ]; then
    echo -e "${RED}Error: Makefile not found!${NC}"; exit 1
fi
make clean
# EXTRA_CFLAGS diteruskan supaya rebuild di sini memakai flag yang SAMA
# dengan build rilis. Tanpa ini 'make' polos menimpa binari ber-march
# yang baru dibangun, dan artefak rilis v1.3.0 kembali menuntut AVX2
# meski workflow-nya sudah benar.
make EXTRA_CFLAGS="${EXTRA_CFLAGS:-}"
if [ ! -f ./indk ]; then
    echo -e "${RED}Error: Binary 'indk' not found after build!${NC}"; exit 1
fi
echo -e "${GREEN}Build successful!${NC}"
echo ""

EXPECTED_DIR="$REPO/tests/expected"
if [ ! -d "$EXPECTED_DIR" ]; then
    echo -e "${RED}Error: tests/expected not found!${NC}"; exit 1
fi

TOTAL=0; PASSED=0; FAILED=0
FAILURES=""

echo -e "${BLUE}=================================================${NC}"
echo -e "${BLUE}  INDONESIAKU - GOLDEN OUTPUT TESTS${NC}"
echo -e "${BLUE}=================================================${NC}"

for exp in "$EXPECTED_DIR"/*.out; do
    name="$(basename "$exp" .out)"
    TOTAL=$((TOTAL+1))

    # Find the matching source under examples/ (any subdir)
    src="$(find examples -type f -name "$name.idk" | head -n1)"
    if [ -z "$src" ]; then
        echo -e "  ${RED}x${NC} $name  (no matching .idk source)"
        FAILED=$((FAILED+1)); FAILURES="$FAILURES $name"; continue
    fi

    errfile="$(mktemp)"
    actual="$(./indk "$src" 2>"$errfile")"; code=$?
    stderr="$(cat "$errfile")"; rm -f "$errfile"
    expected="$(cat "$exp")"

    if [ "$code" -eq 0 ] && [ "$actual" = "$expected" ] && [ -z "$stderr" ]; then
        echo -e "  ${GREEN}+${NC} $name"
        PASSED=$((PASSED+1))
    else
        if [ "$code" -ne 0 ]; then reason="exit=$code"
        elif [ -n "$stderr" ]; then reason="stderr: $stderr"
        else reason="output mismatch"; fi
        echo -e "  ${RED}x${NC} $name  ($reason)"
        FAILED=$((FAILED+1)); FAILURES="$FAILURES $name"
    fi
done

# Error tests: tests/galat/<name>.idk must exit non-zero and print exactly
# tests/galat/<name>.err to stderr (compile errors, uncatchable runtime errors).
if [ -d tests/galat ]; then
    echo -e "${BLUE}  -- tes galat (tests/galat) --${NC}"
    for exp in tests/galat/*.err; do
        [ -e "$exp" ] || continue
        name="$(basename "$exp" .err)"
        TOTAL=$((TOTAL+1))
        src="tests/galat/$name.idk"
        if [ ! -f "$src" ]; then
            echo -e "  ${RED}x${NC} galat/$name  (no matching .idk source)"
            FAILED=$((FAILED+1)); FAILURES="$FAILURES galat/$name"; continue
        fi

        stderr="$(./indk "$src" 2>&1 >/dev/null)"; code=$?
        expected="$(cat "$exp")"

        if [ "$code" -ne 0 ] && [ "$stderr" = "$expected" ]; then
            echo -e "  ${GREEN}+${NC} galat/$name"
            PASSED=$((PASSED+1))
        else
            if [ "$code" -eq 0 ]; then reason="exit=0, expected an error"
            else reason="stderr mismatch: $stderr"; fi
            echo -e "  ${RED}x${NC} galat/$name  ($reason)"
            FAILED=$((FAILED+1)); FAILURES="$FAILURES galat/$name"
        fi
    done
fi

# Contoh di website/panduan/ adalah dokumentasi yang hidup: kalau tidak diuji,
# ia membusuk diam-diam. Syarat Rilis 1.0 di docs/ROADMAP.md.
if [ -f tests/test_panduan.sh ]; then
    echo ""
    if bash tests/test_panduan.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES panduan"
    fi
    TOTAL=$((TOTAL+1))
fi

# readFile() pernah overflow pada pipe: regresi permanen.
if [ -f tests/test_baca_sumber.sh ]; then
    echo ""
    if bash tests/test_baca_sumber.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES baca-sumber"
    fi
    TOTAL=$((TOTAL+1))
fi

# Performa parsing badan: urlencoded harus LINEAR, bukan kuadratik.
# Yang diperiksa bentuk pertumbuhan (2x ukuran -> ~2x waktu), bukan ambang
# detik, supaya runner lambat tidak membuatnya merah palsu.
if [ -f tests/web_badan_performa.sh ]; then
    echo ""
    if bash tests/web_badan_performa.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES web-badan-performa"
    fi
    TOTAL=$((TOTAL+1))
fi

# Kebocoran resource: fd tidak boleh tumbuh di jalur galat, putus, badan besar.
if [ -f tests/kebocoran_resource.sh ]; then
    echo ""
    if bash tests/kebocoran_resource.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES kebocoran-resource"
    fi
    TOTAL=$((TOTAL+1))
fi

# Driver MySQL/MariaDB (Fase 10 WDB-02).
if [ -f tests/my_driver.sh ]; then
    if bash tests/my_driver.sh > /tmp/idk_my.log 2>&1; then
        echo -e "  ${GREEN}+${NC} my_driver"
        PASSED=$((PASSED+1))
    else
        echo -e "  ${RED}x${NC} my_driver"
        sed 's/^/      /' /tmp/idk_my.log
        FAILED=$((FAILED+1)); FAILURES="$FAILURES my_driver"
    fi
    TOTAL=$((TOTAL+1))
fi

# Driver PostgreSQL (Fase 10): terhadap server sungguhan di container.
# Melewati diri sendiri bila Docker tidak ada.
if [ -f tests/pg_driver.sh ]; then
    if bash tests/pg_driver.sh > /tmp/idk_pg.log 2>&1; then
        echo -e "  ${GREEN}+${NC} pg_driver"
        PASSED=$((PASSED+1))
    else
        echo -e "  ${RED}x${NC} pg_driver"
        sed 's/^/      /' /tmp/idk_pg.log
        FAILED=$((FAILED+1)); FAILURES="$FAILURES pg_driver"
    fi
    TOTAL=$((TOTAL+1))
fi

# REST API di atas PostgreSQL (Fase 11 langkah 7): CRUD lewat HTTP sungguhan.
if [ -f tests/api_siswa_pg_server.sh ]; then
    if bash tests/api_siswa_pg_server.sh > /tmp/idk_apipg_uji.log 2>&1; then
        echo -e "  ${GREEN}+${NC} api_siswa_pg_server"
        PASSED=$((PASSED+1))
    else
        echo -e "  ${RED}x${NC} api_siswa_pg_server"
        sed 's/^/      /' /tmp/idk_apipg_uji.log
        FAILED=$((FAILED+1)); FAILURES="$FAILURES api_siswa_pg_server"
    fi
    TOTAL=$((TOTAL+1))
fi

# Daftar builtin ekstensi VS Code harus sama persis dengan defineNative().
# Daftar yang diketik tangan pernah tertinggal 4 builtin tanpa gejala apa pun.
if [ -f tests/builtin_konsisten.sh ]; then
    if bash tests/builtin_konsisten.sh > /tmp/idk_builtin_kons.log 2>&1; then
        echo -e "  ${GREEN}+${NC} builtin_konsisten"
        PASSED=$((PASSED+1))
    else
        echo -e "  ${RED}x${NC} builtin_konsisten"
        sed 's/^/      /' /tmp/idk_builtin_kons.log
        FAILED=$((FAILED+1)); FAILURES="$FAILURES builtin_konsisten"
    fi
    TOTAL=$((TOTAL+1))
fi

# Performa pustaka: json_teks/bita.ke_teks/base64.enkode harus linear.
# Sama seperti di atas, yang diperiksa bentuk pertumbuhan, bukan ambang detik.
if [ -f tests/pustaka_performa.sh ]; then
    if bash tests/pustaka_performa.sh > /tmp/idk_pustaka_perf.log 2>&1; then
        echo -e "  ${GREEN}+${NC} pustaka_performa"
        PASSED=$((PASSED+1))
    else
        echo -e "  ${RED}x${NC} pustaka_performa"
        sed 's/^/      /' /tmp/idk_pustaka_perf.log
        FAILED=$((FAILED+1)); FAILURES="$FAILURES pustaka_performa"
    fi
    TOTAL=$((TOTAL+1))
fi

# Parsing badan (WEB-02): upload >64KB membuktikan gelung baca, bukan satu recv.
if [ -f tests/web_badan_server.sh ]; then
    echo ""
    if bash tests/web_badan_server.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES web-badan"
    fi
    TOTAL=$((TOTAL+1))
fi

# Tag versi (image playground, LABEL rilis, cache-buster website) harus
# mengikuti INDK_VERSION; yang tertinggal tidak pernah gagal sendiri.
if [ -f tests/versi_konsisten.sh ]; then
    echo ""
    if bash tests/versi_konsisten.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES versi-konsisten"
    fi
    TOTAL=$((TOTAL+1))
fi

# Klien HTTP melawan server HTTP sendiri. Ada di CI tetapi dulu tidak di
# sini, sehingga regresi UTF-8 (server menggantung) lolos dari suite lokal.
#
# timeout(1) adalah perintah GNU; macOS tidak memilikinya. Dipakai bila ada
# supaya kemacetan gagal cepat, dan dilewati bila tidak -- lebih baik
# daripada seluruh tes gagal dengan "command not found".
if [ -f tests/test_klien_http.sh ]; then
    echo ""
    if command -v timeout >/dev/null 2>&1; then
        BATAS_WAKTU="timeout 120"
    else
        BATAS_WAKTU=""
    fi
    if $BATAS_WAKTU bash tests/test_klien_http.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES klien-http"
    fi
    TOTAL=$((TOTAL+1))
fi

# Paket .deb: klaim "tidak perlu uninstall versi lama" harus terbukti.
if [ -f tests/paket_deb.sh ]; then
    echo ""
    if bash tests/paket_deb.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES paket-deb"
    fi
    TOTAL=$((TOTAL+1))
fi

# Modul web harus terbukti melayani HTTP sungguhan, bukan hanya lulus uji teks.
if [ -f tests/web_router_server.sh ]; then
    echo ""
    if bash tests/web_router_server.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES web-router"
    fi
    TOTAL=$((TOTAL+1))
fi

# Penyajian berkas statis: yang dibuktikan di sini adalah berkas BINER sampai
# utuh dan percobaan traversal ditolak oleh server yang benar-benar berjalan.
if [ -f tests/web_statis_server.sh ]; then
    echo ""
    if bash tests/web_statis_server.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES web-statis"
    fi
    TOTAL=$((TOTAL+1))
fi

# CRUD REST API di atas SQLite: contoh gabungan web + db + json.
if [ -f tests/api_siswa_server.sh ]; then
    echo ""
    if bash tests/api_siswa_server.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES api-siswa"
    fi
    TOTAL=$((TOTAL+1))
fi

# Klien HTTPS: verifikasi sertifikat terhadap host sungguhan. Sebagian besar
# isinya tes NEGATIF (badssl.com) -- "berhasil mengambil halaman" juga terjadi
# kalau verifikasi dimatikan, jadi yang membuktikan benar adalah penolakannya.
# Melewati diri sendiri bila jaringan tidak tersedia.
if [ -f tests/https_klien.sh ]; then
    echo ""
    if bash tests/https_klien.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES https-klien"
    fi
    TOTAL=$((TOTAL+1))
fi

# Gelung server bawaan pustaka (app.jalankan): batas waktu aktif tanpa
# disetel pemakai, penangan yang meledak tidak mematikan server, dan
# berhenti() melepas port setelah balasan terkirim.
if [ -f tests/web_jalankan_server.sh ]; then
    echo ""
    if bash tests/web_jalankan_server.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES web-jalankan"
    fi
    TOTAL=$((TOTAL+1))
fi

# Batas waktu soket: satu klien yang diam tidak boleh membekukan server.
# Hanya bisa dibuktikan dengan koneksi sungguhan yang sengaja tidak
# mengirim apa pun.
if [ -f tests/web_batas_waktu_server.sh ]; then
    echo ""
    if bash tests/web_batas_waktu_server.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES web-batas-waktu"
    fi
    TOTAL=$((TOTAL+1))
fi

# Mengikuti pengalihan HTTP perlu server sungguhan: klien memblokir saat
# menunggu balasan, jadi tidak bisa diuji dalam satu proses.
if [ -f tests/http_pengalihan_server.sh ]; then
    echo ""
    if bash tests/http_pengalihan_server.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES http-pengalihan"
    fi
    TOTAL=$((TOTAL+1))
fi

# Template playground juga dipamerkan ke pengunjung — ikut diuji berjalan.
if [ -f tests/test_templat_playground.sh ]; then
    echo ""
    if bash tests/test_templat_playground.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES templat-playground"
    fi
    TOTAL=$((TOTAL+1))
fi

if [ -f tests/test_lint.sh ]; then
    echo ""
    if bash tests/test_lint.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES linter"
    fi
    TOTAL=$((TOTAL+1))
fi

if [ -f tests/test_uji.sh ]; then
    echo ""
    if bash tests/test_uji.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES uji"
    fi
    TOTAL=$((TOTAL+1))
fi

if [ -f tests/test_format.sh ]; then
    echo ""
    if bash tests/test_format.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES format"
    fi
    TOTAL=$((TOTAL+1))
fi

# Formatter VS Code memanggil `indk rapikan`; butuh node, dilewati bila tidak ada.
if [ -f editors/vscode-indonesiaku/test-format.js ] && command -v node >/dev/null 2>&1; then
    echo ""
    if node editors/vscode-indonesiaku/test-format.js; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES formatter-vscode"
    fi
    TOTAL=$((TOTAL+1))
fi

# Anotasi tipe tidak boleh mengubah bytecode sedikit pun.
if [ -f tests/test_anotasi_bytecode.sh ]; then
    echo ""
    if bash tests/test_anotasi_bytecode.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES anotasi-bytecode"
    fi
    TOTAL=$((TOTAL+1))
fi

# pustaka/ harus ditemukan dari lokasi interpreter, dari direktori kerja mana pun.
if [ -f tests/test_impor_lokasi.sh ]; then
    echo ""
    if bash tests/test_impor_lokasi.sh; then
        PASSED=$((PASSED+1))
    else
        FAILED=$((FAILED+1)); FAILURES="$FAILURES impor-lokasi"
    fi
    TOTAL=$((TOTAL+1))
fi

echo -e "${BLUE}=================================================${NC}"
echo -e "TOTAL: ${GREEN}$PASSED/$TOTAL PASSED${NC}"
if [ "$FAILED" -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: ALL TESTS PASSING!${NC}"
    exit 0
else
    echo -e "${RED}FAILURE: $FAILED test(s) failed:${NC}"
    for f in $FAILURES; do echo -e "  ${RED}- $f${NC}"; done
    exit 1
fi
