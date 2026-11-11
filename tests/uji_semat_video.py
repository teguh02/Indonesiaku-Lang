#!/usr/bin/env python3
"""Verifikasi penyematan video YouTube di dokumentasi.

Tautan video ditulis sebagai tautan Markdown biasa supaya berkas .md tetap
benar di GitHub; situs yang mengubahnya jadi pemutar. Uji ini memotong
fungsi sematVideo() asli dari docs.html dan menjalankannya di Node dengan
DOM tiruan seadanya — cukup untuk membuktikan tautan mana yang disematkan
dan mana yang dibiarkan, tanpa menambah dependensi jsdom.
"""
import json
import pathlib
import re
import subprocess
import sys
import urllib.request

REPO = pathlib.Path("/root/nandafa/Indonesiaku")
DOCS = REPO / "website/docs.html"
IKHTISAR = REPO / "website/panduan/README.md"


def potong_fungsi(teks, nama):
    m = re.search(rf"(async\s+)?function\s+{nama}\s*\(", teks)
    if not m:
        sys.exit(f"fungsi {nama} tidak ditemukan di docs.html")
    i = teks.index("{", m.end() - 1)
    dalam = 0
    for j in range(i, len(teks)):
        if teks[j] == "{":
            dalam += 1
        elif teks[j] == "}":
            dalam -= 1
            if dalam == 0:
                return teks[m.start():j + 1]
    sys.exit(f"kurung fungsi {nama} tidak seimbang")


HARNESS = r"""
%s
function escapeHtml(s) {
  return String(s).replace(/[&<>"']/g, (c) => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
  }[c]));
}

// DOM tiruan seadanya: cukup untuk a[href], parentElement, dan replaceWith.
function buatTautan(href, teks, tagInduk, teksInduk) {
  const a = {
    _href: href,
    textContent: teks,
    getAttribute: (k) => (k === "href" ? a._href : null),
  };
  a.parentElement = tagInduk === null ? null : {
    tagName: tagInduk,
    textContent: teksInduk === undefined ? teks : teksInduk,
    _diganti: null,
    replaceWith(el) { this._diganti = el; },
  };
  return a;
}

function jalankan(tautan) {
  const container = { querySelectorAll: () => tautan };
  globalThis.document = {
    createElement: () => ({ className: "", innerHTML: "" }),
  };
  sematVideo(container);
  return tautan.map((a) => {
    const p = a.parentElement;
    return p && p._diganti ? p._diganti.innerHTML : null;
  });
}

const KASUS = [
  ["youtu.be sendirian",   buatTautan("https://youtu.be/8mC2Wu_Ti44", "Pengenalan Indonesiaku", "P")],
  ["watch?v= sendirian",   buatTautan("https://www.youtube.com/watch?v=8mC2Wu_Ti44", "Judul", "P")],
  ["di tengah kalimat",    buatTautan("https://youtu.be/8mC2Wu_Ti44", "video", "P", "lihat video ini ya")],
  ["bukan paragraf",       buatTautan("https://youtu.be/8mC2Wu_Ti44", "v", "LI")],
  ["bukan youtube",        buatTautan("https://example.com/8mC2Wu_Ti44", "x", "P")],
  ["id tidak valid",       buatTautan("https://youtu.be/pendek", "x", "P")],
];

const hasil = {};
for (const [nama, a] of KASUS) hasil[nama] = jalankan([a])[0];
console.log(JSON.stringify(hasil));
""" 


def main():
    html = DOCS.read_text(encoding="utf8")
    kode = potong_fungsi(html, "sematVideo")

    tmp = pathlib.Path("/tmp/_uji_video.mjs")
    tmp.write_text(HARNESS % kode, encoding="utf8")
    r = subprocess.run(["node", str(tmp)], capture_output=True, text=True,
                       timeout=60)
    if r.returncode != 0:
        print(r.stderr[:1200])
        sys.exit("kode JS gagal dijalankan")
    hasil = json.loads(r.stdout)
    salah = []

    # Yang HARUS disematkan.
    for nama in ("youtu.be sendirian", "watch?v= sendirian"):
        out = hasil.get(nama)
        if not out:
            salah.append(f"{nama}: tidak disematkan")
            continue
        if "youtube-nocookie.com/embed/8mC2Wu_Ti44" not in out:
            salah.append(f"{nama}: URL semat salah -> {out[:90]}")
        if "loading=\"lazy\"" not in out:
            salah.append(f"{nama}: iframe tidak lazy-load")
        if "allowfullscreen" not in out:
            salah.append(f"{nama}: tidak bisa layar penuh")

    # Yang HARUS dibiarkan sebagai tautan biasa.
    for nama in ("di tengah kalimat", "bukan paragraf", "bukan youtube",
                 "id tidak valid"):
        if hasil.get(nama):
            salah.append(f"{nama}: seharusnya TIDAK disematkan")

    for nama, out in hasil.items():
        print(f"  {'semat ' if out else 'biasa '} {nama}")

    # Tautan video di panduan harus berdiri sendiri satu paragraf (kalau
    # tidak, sematVideo melewatkannya) dan videonya harus benar-benar ada.
    panduan = sorted((REPO / "website/panduan").glob("*.md"))
    temuan = []
    for berkas in panduan:
        isi = berkas.read_text(encoding="utf8")
        # Tautan video apa pun di dalam berkas, berdiri sendiri atau tidak.
        for m in re.finditer(
                r"\[([^\]]*)\]\((https://(?:youtu\.be/|www\.youtube\.com/watch\?v=)"
                r"([A-Za-z0-9_-]{11}))\)", isi):
            baris = isi[:m.start()].count("\n") + 1
            sendiri = re.search(
                rf"^{re.escape(m.group(0))}$", isi, re.M) is not None
            temuan.append((berkas.name, baris, m.group(1), m.group(2),
                           m.group(3), sendiri))

    if not temuan:
        salah.append("tidak ada tautan video di panduan/")

    for nama, baris, judul, url, vid, sendiri in temuan:
        if not sendiri:
            salah.append(f"{nama}:{baris} tautan video tidak berdiri sendiri "
                         "satu paragraf -> tidak akan disematkan")
            continue
        try:
            req = urllib.request.Request(
                "https://www.youtube.com/oembed?url=" + url + "&format=json",
                headers={"User-Agent": "uji-indk"})
            with urllib.request.urlopen(req, timeout=40) as resp:
                data = json.loads(resp.read().decode("utf8"))
            print(f"  ok     {nama}: {data['title']} ({vid})")
        except Exception as e:
            salah.append(f"{nama}:{baris} video {vid} tidak ada di YouTube: {e}")

    md = IKHTISAR.read_text(encoding="utf8")

    if "Fitur utama v1.0.0" in md:
        salah.append("Daftar Isi masih menyebut 'Fitur utama v1.0.0'")

    if salah:
        print(f"\nMELESET ({len(salah)}):")
        for s in salah:
            print("  -", s)
        return 1
    print("\nVIDEO TERSEMAT BENAR DAN TAUTAN LAIN TIDAK TERGANGGU")
    return 0


if __name__ == "__main__":
    sys.exit(main())
