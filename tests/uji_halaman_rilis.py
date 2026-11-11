#!/usr/bin/env python3
"""Verifikasi pembangun halaman "Riwayat Rilis" di docs.html.

Halaman itu menggabungkan dua sumber: GitHub Releases API (tanggal, tautan,
jumlah berkas) dan CHANGELOG.md (catatan tiap versi). Alasannya sebagian
rilis body-nya kosong di GitHub, jadi API sendirian menghasilkan halaman
bolong.

Uji ini memotong fungsi JS-nya dari docs.html lalu menjalankannya di Node
dengan data GitHub sungguhan, supaya yang diperiksa adalah kode yang
benar-benar dikirim ke browser — bukan salinan yang bisa menyimpang.
"""
import json
import pathlib
import re
import subprocess
import sys
import urllib.request

REPO = pathlib.Path("/root/nandafa/Indonesiaku")
DOCS = REPO / "website/docs.html"
API = "https://api.github.com/repos/teguh02/Indonesiaku-Lang/releases?per_page=100"
RAW = "https://raw.githubusercontent.com/teguh02/Indonesiaku-Lang/main/CHANGELOG.md"


def ambil(url, json_mode=False):
    req = urllib.request.Request(url, headers={"User-Agent": "uji-indk"})
    with urllib.request.urlopen(req, timeout=60) as r:
        data = r.read().decode("utf8")
    return json.loads(data) if json_mode else data


def potong_fungsi(teks, nama):
    """Ambil satu fungsi JS dari docs.html dengan mencocokkan kurung kurawal."""
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


def main():
    html = DOCS.read_text(encoding="utf8")
    rilis = ambil(API, True)
    changelog = ambil(RAW)

    # Ambil kode asli dari docs.html, jangan tulis ulang di sini.
    kode = "\n".join(potong_fungsi(html, n)
                     for n in ("pecahChangelog", "tanggalLokal", "bangunRilis"))

    if "RILIS_API" not in html:
        sys.exit("docs.html tidak memakai GitHub Releases API")

    # Ambil URL API dari docs.html juga, jangan tulis ulang di sini —
    # kalau tidak, fetch() melempar ReferenceError yang ditelan try/catch
    # dan uji ini akan "lulus" tanpa pernah memakai API.
    m_api = re.search(r'const RILIS_API\s*=\s*\n?\s*"([^"]+)"', html)
    if not m_api:
        sys.exit("URL RILIS_API tidak bisa dibaca dari docs.html")

    harness = f"""
{kode}
const DATA_RILIS = {json.dumps(rilis)};
const DATA_CH = {json.dumps(changelog)};
const RAW_BASE = "x/";
const RILIS_API = {json.dumps(m_api.group(1))};
globalThis.window = {{ indkLang: () => "id" }};
const window = globalThis.window;
async function fetchText(u) {{ return DATA_CH; }}
let apiDipanggil = null;
globalThis.fetch = async (u) => {{
  apiDipanggil = u;
  return {{ ok: true, status: 200, json: async () => DATA_RILIS }};
}};
bangunRilis().then((md) => {{
  console.log(JSON.stringify({{
    md,
    api: apiDipanggil,
    versi: Object.keys(pecahChangelog(DATA_CH)),
  }}));
}});
"""
    tmp = pathlib.Path("/tmp/_uji_rilis.mjs")
    tmp.write_text(harness, encoding="utf8")
    r = subprocess.run(["node", str(tmp)], capture_output=True, text=True,
                       timeout=120)
    if r.returncode != 0:
        print(r.stderr[:1500])
        sys.exit("kode JS gagal dijalankan")
    hasil = json.loads(r.stdout)
    pathlib.Path("/tmp/_rilis.md").write_text(hasil["md"], encoding="utf8")
    md = hasil["md"]
    salah = []

    # API memang dipanggil, bukan sekadar disebut di komentar.
    if not hasil["api"] or "api.github.com" not in hasil["api"]:
        salah.append(f"GitHub API tidak dipanggil (url={hasil['api']})")

    terbit = [x for x in rilis if not x["draft"]]
    draft = [x for x in rilis if x["draft"]]

    # Tiap rilis terbit harus muncul lengkap dengan tautan halaman rilisnya.
    for x in terbit:
        v = x["tag_name"].lstrip("v")
        if f"## v{v}\n" not in md:
            salah.append(f"rilis {x['tag_name']} tidak muncul")
        elif x["html_url"] not in md:
            salah.append(f"tautan rilis {x['tag_name']} tidak muncul")

    # Draft TIDAK boleh bocor: ini menjaga rencana rilis v1.7.0 user.
    for x in draft:
        if x["html_url"] in md:
            salah.append(f"BOCOR: draft {x['tag_name']} muncul di halaman")

    # Versi yang body-nya kosong di GitHub tetap harus berisi, dari CHANGELOG.
    kosong = [x["tag_name"].lstrip("v") for x in terbit
              if not (x["body"] or "").strip()]
    for v in kosong:
        m = re.search(rf"## v{re.escape(v)}\n(.*?)(?=\n## v|\Z)", md, re.S)
        if not m or len(m.group(1).strip()) < 120:
            salah.append(f"v{v} (body GitHub kosong) tidak terisi dari CHANGELOG")
    if kosong and not salah:
        print(f"  ok   {len(kosong)} rilis body-kosong terisi dari CHANGELOG")

    # Versi di CHANGELOG yang belum dirilis harus ditandai, bukan dibuang.
    tag_terbit = {x["tag_name"].lstrip("v") for x in terbit}
    belum = [v for v in hasil["versi"] if v not in tag_terbit]
    for v in belum:
        if f"## v{v}\n" not in md:
            salah.append(f"versi {v} di CHANGELOG hilang dari halaman")
    if belum:
        print(f"  ok   versi belum dirilis ditandai: {belum}")

    # Entri sidebar harus ada, kalau tidak halaman ini tak bisa dibuka.
    if "__rilis__" not in html:
        salah.append("entri sidebar __rilis__ tidak ada")

    print(f"  ok   {len(terbit)} rilis terbit, {len(draft)} draft disembunyikan")
    print(f"  ok   halaman {len(md)} karakter")

    if salah:
        print(f"\nMELESET ({len(salah)}):")
        for s in salah:
            print("  -", s)
        return 1
    print("\nHALAMAN RILIS COCOK DENGAN GITHUB API DAN CHANGELOG")
    return 0


if __name__ == "__main__":
    sys.exit(main())
