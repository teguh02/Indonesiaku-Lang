#!/usr/bin/env python3
"""Uji badge versi footer sinkron di semua halaman.

Dulu badge ditulis tangan per halaman, jadi index.html tertinggal di v1.5.0
sementara about/author sudah v1.7.0. Sekarang app.js menulisnya dari
cache-buster `app.js?v=X.Y.Z`.

Yang diperiksa (tanpa browser — cukup baca HTML + jalankan regex yang sama
seperti app.js):
  1. Tiap halaman ber-badge punya atribut data-indk-versi.
  2. Cache-buster app.js?v= di SEMUA halaman sama persis.
  3. Versi itu == INDK_VERSION di src/common.h (sumber kebenaran).
  4. Teks bawaan badge (fallback saat JS mati) juga versi yang benar.

Keluar 1 bila meleset.
"""
import pathlib
import re
import sys

REPO = pathlib.Path("/root/nandafa/Indonesiaku")
WEB = REPO / "website"
BER_BADGE = ["index.html", "about.html", "author.html"]
SEMUA = BER_BADGE + ["docs.html", "playground/index.html"]


def main():
    salah = []

    m = re.search(r'#define\s+INDK_VERSION\s+"([^"]+)"',
                  (REPO / "src/common.h").read_text(encoding="utf8"))
    versi = m.group(1)
    print(f"INDK_VERSION: {versi}")

    buster = {}
    for f in SEMUA:
        s = (WEB / f).read_text(encoding="utf8")
        v = re.search(r'app\.js\?v=(\d+\.\d+\.\d+)', s)
        if not v:
            salah.append(f"{f}: tidak memuat app.js?v=")
            continue
        buster[f] = v.group(1)
        if v.group(1) != versi:
            salah.append(f"{f}: app.js?v={v.group(1)} != INDK_VERSION {versi}")

    if len(set(buster.values())) > 1:
        salah.append(f"cache-buster beda antar halaman: {buster}")

    for f in BER_BADGE:
        s = (WEB / f).read_text(encoding="utf8")
        badge = re.findall(r'<span[^>]*data-indk-versi[^>]*>(v[\d.]+)</span>', s)
        if not badge:
            salah.append(f"{f}: badge versi tanpa data-indk-versi")
            continue
        # Fallback (JS mati) harus benar juga.
        for b in badge:
            if b != "v" + versi:
                salah.append(f"{f}: teks bawaan badge {b} != v{versi}")
        print(f"  ok   {f:22s} badge={badge[0]} buster={buster.get(f)}")

    # app.js harus benar-benar punya mekanismenya, dan regex yang dipakainya
    # harus cocok dengan bentuk URL yang betul-betul ditulis di HTML.
    src = (WEB / "assets/app.js").read_text(encoding="utf8")
    if "data-indk-versi" not in src:
        salah.append("app.js tidak menulis badge data-indk-versi")
    pola = re.search(r'SCRIPT_SRC\.match\(/([^/]+)/\)', src)
    if not pola:
        salah.append("app.js: regex pengambil versi tidak ditemukan")
    else:
        js_re = pola.group(1).replace("\\\\", "\\")
        for f, v in buster.items():
            cocok = re.search(js_re, f"assets/app.js?v={v}")
            if not cocok or cocok.group(1) != versi:
                salah.append(f"{f}: regex app.js gagal mengambil versi dari ?v={v}")

    if salah:
        print(f"\nMELESET ({len(salah)}):")
        for s in salah:
            print("  -", s)
        return 1
    print(f"\nSEMUA HALAMAN SINKRON DI v{versi}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
