#!/usr/bin/env python3
"""Gambar naskah EISEJ dari ../analisis_repo/hasil (grayscale + arsiran; versi cetak hitam-putih).
fig_commits_month.pdf : commit non-merge per bulan menurut kategori atribusi (Okt 2025–Nov 2026)
"""
import csv
from collections import Counter, defaultdict
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = Path(__file__).parent
SRC = HERE.parent / "analisis_repo" / "hasil" / "tabel_commit.csv"
OUT = HERE / "gambar"
plt.rcParams.update({"font.family": "DejaVu Sans", "font.size": 8})

KAT = [("MANUSIA", "Main developer, no AI marker", "#ffffff", ""),
       ("AI", "AI co-author marker", "#777777", ""),
       ("CAMPURAN", "Mixed account (attribution uncertain)", "#d9d9d9", "////"),
       ("LAIN", "Second contributor", "#000000", "")]
LABEL_KAT = {"MANUSIA": "Main developer", "AI": "AI marker", "CAMPURAN": "Mixed account", "LAIN": "Second contributor"}


def bulan_range(a: str, b: str) -> list[str]:
    y, m = map(int, a.split("-")); y2, m2 = map(int, b.split("-"))
    out = []
    while (y, m) <= (y2, m2):
        out.append(f"{y:04d}-{m:02d}")
        m += 1
        if m == 13:
            y, m = y + 1, 1
    return out


def main() -> None:
    OUT.mkdir(exist_ok=True)
    rows = list(csv.DictReader(open(SRC, encoding="utf-8")))
    cnt = defaultdict(Counter)
    for r in rows:
        cnt[r["author_date"][:7]][r["kat"]] += 1
    months = bulan_range(min(cnt), max(cnt))
    fig, ax = plt.subplots(figsize=(6.3, 2.6), dpi=300)
    bottom = [0] * len(months)
    for k, lab, col, hatch in KAT:
        v = [cnt[m][k] for m in months]
        ax.bar(range(len(months)), v, bottom=bottom, color=col, edgecolor="black", linewidth=0.5,
               hatch=hatch, label=f"{lab} (n={sum(v)})")
        bottom = [b + x for b, x in zip(bottom, v)]
    for i, t in enumerate(bottom):
        if t:
            ax.text(i, t + 3, str(t), ha="center", va="bottom", fontsize=6.5)
    ax.set_xticks(range(len(months)), [m[2:].replace("-", "/") for m in months], rotation=0, fontsize=6.5)
    ax.set_ylabel("Non-merge commits")
    ax.set_ylim(0, max(bottom) * 1.15)
    ax.spines[["top", "right"]].set_visible(False)
    ax.legend(frameon=False, fontsize=6.5, loc="upper left")
    fig.tight_layout()
    fig.savefig(OUT / "fig_commits_month.pdf")
    fig.savefig(OUT / "fig_commits_month.png")
    print("tersimpan:", OUT / "fig_commits_month.pdf", "| bulan:", len(months), "| total:", sum(bottom))


if __name__ == "__main__":
    main()
