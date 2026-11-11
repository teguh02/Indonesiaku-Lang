#!/usr/bin/env python3
"""Verifikasi klaim ekstensi VS Code di dokumentasi terhadap sumber nyata.

Dokumentasi mengklaim ekstensi terbit di Marketplace. Klaim itu diperiksa ke
API Marketplace resmi dan ke package.json, bukan ke tangkapan layar.

  1. Identifier `teguh02.indonesiaku` benar-benar ada di Marketplace.
  2. publisher + name di package.json == identifier yang didokumentasikan.
  3. Tidak ada lagi URL repo lama `teguh02/Indonesiaku/` (tanpa `-Lang`)
     di dokumentasi — itu 404.
  4. Tidak ada lagi nama berkas .vsix dipaku versi lama (`indonesiaku-0.2.0`).
  5. README + panduan instalasi memuat tautan Marketplace.

Keluar 1 bila meleset. Butuh jaringan untuk butir 1.
"""
import json
import pathlib
import re
import subprocess
import sys

REPO = pathlib.Path("/root/nandafa/Indonesiaku")
ID = "teguh02.indonesiaku"
URL = f"https://marketplace.visualstudio.com/items?itemName={ID}"


def marketplace():
    body = json.dumps({
        "filters": [{"criteria": [{"filterType": 7, "value": ID}]}],
        "flags": 914,
    })
    r = subprocess.run(
        ["curl", "-sS", "-m", "30", "-X", "POST",
         "https://marketplace.visualstudio.com/_apis/public/gallery/extensionquery",
         "-H", "Content-Type: application/json",
         "-H", "Accept: application/json;api-version=7.2-preview.1",
         "-d", body],
        capture_output=True, text=True)
    ext = json.loads(r.stdout)["results"][0]["extensions"]
    if not ext:
        return None
    x = ext[0]
    return {
        "publisher": x["publisher"]["publisherName"],
        "name": x["extensionName"],
        "versi": x["versions"][0]["version"],
    }


def main():
    salah = []

    pkg = json.loads((REPO / "editors/vscode-indonesiaku/package.json")
                     .read_text(encoding="utf8"))
    ident_repo = f"{pkg['publisher']}.{pkg['name']}"
    print(f"package.json : {ident_repo} v{pkg['version']}")
    if ident_repo != ID:
        salah.append(f"identifier package.json {ident_repo} != {ID}")

    mk = marketplace()
    if mk is None:
        salah.append(f"{ID} tidak ditemukan di Marketplace")
    else:
        print(f"Marketplace  : {mk['publisher']}.{mk['name']} v{mk['versi']}")
        if f"{mk['publisher']}.{mk['name']}" != ID:
            salah.append("identifier Marketplace tidak cocok")
        if mk["versi"] != pkg["version"]:
            print(f"  catatan: Marketplace v{mk['versi']} tertinggal dari "
                  f"repo v{pkg['version']} (belum dipublikasikan)")

    docs = ["README.md", "website/panduan/02_INSTALASI.md"]
    for d in docs:
        s = (REPO / d).read_text(encoding="utf8")
        if URL not in s:
            salah.append(f"{d}: tidak memuat tautan Marketplace")
        for buruk, kenapa in [
            (r"github\.com/teguh02/Indonesiaku/", "URL repo lama, 404"),
            (r"indonesiaku-0\.2\.0\.vsix", "nama .vsix dipaku versi lama"),
        ]:
            if re.search(buruk, s):
                salah.append(f"{d}: masih ada {buruk!r} ({kenapa})")

    # Perintah pasang yang didokumentasikan harus memakai identifier yang benar.
    inst = (REPO / "website/panduan/02_INSTALASI.md").read_text(encoding="utf8")
    m = re.search(r"code --install-extension ([\w.-]+)", inst)
    if not m:
        salah.append("panduan: tidak ada perintah code --install-extension")
    elif m.group(1) != ID and not m.group(1).endswith(".vsix"):
        salah.append(f"panduan: perintah pasang memakai {m.group(1)!r}")

    if salah:
        print(f"\nMELESET ({len(salah)}):")
        for s in salah:
            print("  -", s)
        return 1
    print("\nKLAIM EKSTENSI COCOK DENGAN MARKETPLACE DAN package.json")
    return 0


if __name__ == "__main__":
    sys.exit(main())
