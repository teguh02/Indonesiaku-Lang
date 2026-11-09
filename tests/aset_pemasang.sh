#!/usr/bin/env bash
# Memeriksa aset ikon pemasang Windows.
#
# Berkas .iss tidak pernah dikompilasi di Linux/macOS CI (butuh iscc), jadi
# ikon yang rusak atau terhapus baru ketahuan setelah rilis sampai ke
# pengguna. Pemeriksaan ini membaca strukturnya langsung dari byte.
#
# Dua kesalahan nyata yang dijaga di sini:
#   1. ICO yang seluruh entrinya PNG. Windows hanya menjamin PNG pada
#      256x256; ukuran di bawahnya wajib DIB, kalau tidak ikon kecil di
#      Explorer dan taskbar bisa kosong.
#   2. BMP wizard yang tersimpan terkompresi atau ber-alpha. Inno Setup 6
#      menolaknya saat kompilasi.

set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
PAKET="$AKAR/packaging/windows"
ISS="$PAKET/indonesiaku.iss"
GAGAL=0

lulus() { echo "  ok   $1"; }
tolak() { echo "  GAGAL $1"; GAGAL=1; }

echo "Aset pemasang Windows:"

# --- berkas ada ---
for b in indonesiaku.ico wizard-kecil-55.bmp wizard-kecil-83.bmp \
         wizard-kecil-110.bmp wizard-kecil-138.bmp; do
    if [ -f "$PAKET/$b" ]; then lulus "ada $b"; else tolak "hilang $b"; fi
done

# --- .iss benar-benar menunjuk aset itu ---
for kunci in "SetupIconFile=indonesiaku.ico" \
             "WizardSmallImageFile=wizard-kecil-55.bmp" \
             "UninstallDisplayIcon={app}\\indonesiaku.ico"; do
    if grep -qF "$kunci" "$ISS"; then
        lulus "iss memuat $kunci"
    else
        tolak "iss tidak memuat $kunci"
    fi
done

# indonesiaku.ico harus ikut dipasang, kalau tidak pintasan Start Menu
# kehilangan ikonnya begitu berkas pemasang dihapus.
if grep -qE '^Source: "indonesiaku\.ico"; DestDir: "\{app\}"' "$ISS"; then
    lulus "iss memasang indonesiaku.ico ke {app}"
else
    tolak "iss tidak memasang indonesiaku.ico ke {app}"
fi

# --- struktur biner ---
if ! command -v python3 >/dev/null 2>&1; then
    echo "  LEWAT: python3 tidak ada, pemeriksaan struktur dilewati"
    exit $GAGAL
fi

python3 - "$PAKET" <<'PY'
import struct, sys, os
paket = sys.argv[1]
gagal = False

def tolak(p):
    global gagal
    print(f"  GAGAL {p}"); gagal = True

def lulus(p):
    print(f"  ok   {p}")

d = open(os.path.join(paket, "indonesiaku.ico"), "rb").read()
res, typ, n = struct.unpack("<HHH", d[:6])
if (res, typ) != (0, 1):
    tolak(f"header ICO tidak sah: reserved={res} type={typ}")
else:
    lulus("header ICO sah")

ukuran = {}
for i in range(n):
    w, h, _c, _r, _pl, bpp, size, off = struct.unpack("<BBBBHHII", d[6+16*i:22+16*i])
    w = w or 256
    png = d[off:off+8] == b"\x89PNG\r\n\x1a\n"
    ukuran[w] = ("PNG" if png else "DIB", bpp)
    if off + size > len(d):
        tolak(f"entri {w}x{w} menunjuk ke luar berkas")
    # Windows hanya menjamin PNG pada 256x256.
    if png and w < 256:
        tolak(f"entri {w}x{w} tersimpan PNG; ukuran <256 harus DIB")

for perlu in (16, 32, 48, 256):
    if perlu in ukuran:
        lulus(f"ICO punya {perlu}x{perlu} ({ukuran[perlu][0]}, {ukuran[perlu][1]} bpp)")
    else:
        tolak(f"ICO tidak punya ukuran {perlu}x{perlu}")

for s in (55, 83, 110, 138):
    b = open(os.path.join(paket, f"wizard-kecil-{s}.bmp"), "rb").read()
    if b[:2] != b"BM":
        tolak(f"wizard-kecil-{s}.bmp bukan BMP"); continue
    w, h, _pl, bpp, comp = struct.unpack("<iiHHI", b[18:34])
    if (w, h) != (s, s):
        tolak(f"wizard-kecil-{s}.bmp berukuran {w}x{h}, seharusnya {s}x{s}")
    elif comp != 0:
        tolak(f"wizard-kecil-{s}.bmp terkompresi (comp={comp}); Inno menolaknya")
    elif bpp != 24:
        tolak(f"wizard-kecil-{s}.bmp {bpp} bpp, seharusnya 24")
    else:
        lulus(f"wizard-kecil-{s}.bmp {w}x{h} 24 bpp tanpa kompresi")

sys.exit(1 if gagal else 0)
PY
[ $? -ne 0 ] && GAGAL=1

if [ $GAGAL -eq 0 ]; then
    echo "SEMUA UJI ASET PEMASANG LULUS"
else
    echo "UJI ASET PEMASANG GAGAL"
fi
exit $GAGAL
