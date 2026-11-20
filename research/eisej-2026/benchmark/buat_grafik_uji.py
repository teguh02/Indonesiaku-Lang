#!/usr/bin/env python3
"""Gambar 5 Paper E: perbandingan waktu eksekusi dan memori puncak
Indonesiaku vs Python 3.14 (data: hasil_banding.json dari ukur_banding.py).

(a) median waktu (batang) ± SD (garis galat): program contoh dan program kosong
    (baseline) per runtime — memperlihatkan bahwa selisih didominasi waktu mulai.
(b) memori puncak: peak working set dan peak private bytes (median).
Keluaran: ../gambar/gambar5_uji_eksekusi.png (300 dpi).
"""
import json
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = Path(__file__).parent
OUT = HERE.parent / "gambar" / "gambar5_uji_eksekusi.png"
H = json.loads((HERE / "hasil_banding.json").read_text(encoding="utf-8"))
plt.rcParams.update({"font.family": "DejaVu Sans", "font.size": 8.5})

IDK, PY = "#2f7d5b", "#2f5d8a"
IDK_L, PY_L = "#a9d3bf", "#9bb7d4"
LABEL_K12 = "K12\n(fungsi cetak)"
LABEL_K21 = "K21\n(lingkup variabel)"
BAR_W = 0.19


def koma(v: float, nd: int = 1) -> str:
    return f"{v:.{nd}f}".replace(".", ",")


def style_axis(ax, title: str, ylabel: str) -> None:
    ax.set_title(title, fontsize=9, loc="left")
    ax.set_ylabel(ylabel)
    ax.spines[["top", "right"]].set_visible(False)
    ax.legend(frameon=False, fontsize=6.3, loc="upper center", bbox_to_anchor=(0.5, -0.2),
              ncol=2, handlelength=1.6, columnspacing=1.0)


def bar(ax, x, v, col, hatch, lab, err=None):
    ax.bar(x, v, BAR_W * 0.92, color=col, hatch=hatch, edgecolor="#555555" if hatch else "white",
           linewidth=0.4, label=lab, yerr=err,
           error_kw={"elinewidth": 0.7, "capsize": 2, "ecolor": "#444444"})


def panel_waktu(ax) -> None:
    groups = [(LABEL_K12, "K12-indk", "baseline-indk-K12", "K12-python", "baseline-python"),
              (LABEL_K21, "K21-indk", "baseline-indk-K21", "K21-python", "baseline-python")]
    series = [("Indonesiaku: program contoh", IDK, None), ("Indonesiaku: program kosong", IDK_L, "//"),
              ("Python 3.14: program contoh", PY, None), ("Python 3.14: program kosong", PY_L, "//")]
    top = 0.0
    for gi, (_, *keys) in enumerate(groups):
        for si, (key, (lab, col, hatch)) in enumerate(zip(keys, series)):
            stt = H[key]["waktu_ms"]
            x = gi + (si - 1.5) * BAR_W
            bar(ax, x, stt["median"], col, hatch, lab if gi == 0 else None, err=stt["sd"])
            ax.text(x, stt["median"] + stt["sd"] + 0.8, koma(stt["median"]), ha="center", va="bottom", fontsize=5.6)
            top = max(top, stt["median"] + stt["sd"])
    ax.set_xticks(range(len(groups)), [g[0] for g in groups])
    ax.set_ylim(0, top * 1.12)
    style_axis(ax, "(a) Waktu eksekusi", "Waktu (ms), median ± SD")


def panel_memori(ax) -> None:
    groups = [(LABEL_K12, "K12-indk", "K12-python"), (LABEL_K21, "K21-indk", "K21-python")]
    series = [("Indonesiaku: peak working set", IDK, None, "peak_working_set_MB", 0),
              ("Indonesiaku: peak private bytes", IDK_L, "//", "peak_private_MB", 0),
              ("Python 3.14: peak working set", PY, None, "peak_working_set_MB", 1),
              ("Python 3.14: peak private bytes", PY_L, "//", "peak_private_MB", 1)]
    for gi, (_, k_idk, k_py) in enumerate(groups):
        for si, (lab, col, hatch, metric, which) in enumerate(series):
            v = H[(k_idk, k_py)[which]][metric]["median"]
            x = gi + (si - 1.5) * BAR_W
            bar(ax, x, v, col, hatch, lab if gi == 0 else None)
            ax.text(x, v + 0.15, koma(v, 2), ha="center", va="bottom", fontsize=5.6)
    ax.set_xticks(range(len(groups)), [g[0] for g in groups])
    ax.set_ylim(0, 13.5)
    style_axis(ax, "(b) Memori puncak", "Memori (MB), median")


def main() -> None:
    fig, (a, b) = plt.subplots(1, 2, figsize=(7.2, 4.3), dpi=300)
    panel_waktu(a)
    panel_memori(b)
    n = H["lingkungan"]["runs"]
    fig.text(0.5, 0.005, f"{n} run per skenario (bergiliran); Intel Core i7-13620H, Windows 11; "
             f"Python {H['lingkungan']['python'].split()[0]}", ha="center", fontsize=6.5, color="#555555")
    fig.tight_layout(rect=(0, 0.03, 1, 1), w_pad=2.5)
    fig.savefig(OUT)
    print("tersimpan:", OUT)


if __name__ == "__main__":
    main()
