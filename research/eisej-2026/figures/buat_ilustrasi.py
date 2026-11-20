#!/usr/bin/env python3
"""Ilustrasi tambahan naskah EISEJ (Inggris, hitam-putih untuk versi cetak).

fig_agile_cycle.pdf     siklus Agile yang diterapkan + artefak nyata tiap tahap
                        (versi Inggris dari ../buat_gambar_v2.py gambar1)
fig_daily_commits.pdf   commit non-merge harian Okt–Nov 2026, hari K12 & K21 ditandai
                        (data ../gambar/commit_harian.csv dari git log; versi Inggris gambar2)
fig_pr55.png, fig_commit_k12.png
                        tangkapan layar GitHub (../gambar/gambar4_pr55_k21.png,
                        ../gambar/gambar3_commit_k12.png) dikonversi ke skala abu-abu
"""
import csv
import datetime as dt
import math
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.dates
import matplotlib.pyplot as plt
import matplotlib.ticker
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch
from PIL import Image

HERE = Path(__file__).parent
SRC = HERE.parent / "gambar"
OUT = HERE / "gambar"
plt.rcParams.update({"font.family": "DejaVu Sans", "font.size": 9})

STAGES = [
    ("1. Planning", "roadmap (26 phases)\nfeature plans in\nRENCANA_FITUR/"),
    ("2. Design", "design decisions\nK1–K21\n(alternatives + rationale)"),
    ("3. Implementation", "feature branch\nC11 code (src/)\ndeveloper + AI agents"),
    ("4. Testing", "golden-output tests\ntests/expected/\nGitHub Actions CI"),
    ("5. Review & release", "pull request,\nCHANGELOG,\nsemantic versioning"),
    ("6. Evaluation\n& feedback", "cross-OS testing\n(findings T-U)\n→ new backlog items"),
]


def agile_cycle() -> None:
    fig, ax = plt.subplots(figsize=(6.6, 4.8), dpi=300)
    ax.set_xlim(-1.75, 1.75); ax.set_ylim(-1.45, 1.45); ax.axis("off"); ax.set_aspect("equal")
    n, r = len(STAGES), 1.1
    pos = [(r * math.cos(math.pi / 2 - 2 * math.pi * i / n), r * math.sin(math.pi / 2 - 2 * math.pi * i / n))
           for i in range(n)]
    w, h = 0.9, 0.5
    for (title, art), (x, y) in zip(STAGES, pos):
        ax.add_patch(FancyBboxPatch((x - w / 2, y - h / 2), w, h, boxstyle="round,pad=0.02,rounding_size=0.06",
                                    fc="#f2f2f2", ec="black", lw=0.9))
        ax.text(x, y + 0.13, title, ha="center", va="center", fontsize=8.4, fontweight="bold")
        ax.text(x, y - 0.08, art, ha="center", va="center", fontsize=6.4, color="#222222", linespacing=1.15)
    for i in range(n):
        (x1, y1), (x2, y2) = pos[i], pos[(i + 1) % n]
        s = (x1 + (x2 - x1) * 0.36, y1 + (y2 - y1) * 0.36)
        e = (x1 + (x2 - x1) * 0.64, y1 + (y2 - y1) * 0.64)
        ax.add_patch(FancyArrowPatch(s, e, arrowstyle="-|>", mutation_scale=11, color="black", lw=1.0,
                                     connectionstyle="arc3,rad=-0.15"))
    ax.text(0, 0.08, "One iteration\n= one backlog item", ha="center", va="center", fontsize=8.8, fontweight="bold")
    ax.text(0, -0.25, "e.g. K12 (cetak as a function)\nK21 (loop-variable scope)", ha="center", va="center",
            fontsize=7.2, color="#444444")
    fig.tight_layout()
    fig.savefig(OUT / "fig_agile_cycle.pdf")
    plt.close(fig)


def daily_commits() -> None:
    rows = list(csv.reader(open(SRC / "commit_harian.csv", encoding="utf-8")))
    data = {dt.date.fromisoformat(d): int(c) for d, c in rows}
    start, end = min(data), max(data)
    days = [start + dt.timedelta(days=i) for i in range((end - start).days + 1)]
    fig, ax = plt.subplots(figsize=(6.6, 2.6), dpi=300)
    ax.bar(days, [data.get(d, 0) for d in days], color="#bbbbbb", edgecolor="black", linewidth=0.3, width=0.8)
    for d, label in ((dt.date(2026, 10, 25), "K12\n(25 Oct)"), (dt.date(2026, 11, 3), "K21\n(3 Nov)")):
        ax.bar([d], [data.get(d, 0)], color="black", width=0.8)
        ax.annotate(label, (d, data.get(d, 0)), xytext=(0, 5), textcoords="offset points", ha="center",
                    fontsize=7, fontweight="bold")
    ax.set_ylabel("Non-merge commits per day")
    ax.spines[["top", "right"]].set_visible(False)
    ax.xaxis.set_major_formatter(matplotlib.ticker.FuncFormatter(
        lambda v, _: matplotlib.dates.num2date(v).strftime("%d %b")))
    ax.set_ylim(0, max(data.values()) * 1.3)
    fig.autofmt_xdate(rotation=0, ha="center")
    fig.tight_layout()
    fig.savefig(OUT / "fig_daily_commits.pdf")
    plt.close(fig)


def screenshots() -> None:
    for src, dst in (("gambar4_pr55_k21.png", "fig_pr55.png"), ("gambar3_commit_k12.png", "fig_commit_k12.png")):
        Image.open(SRC / src).convert("L").save(OUT / dst, optimize=True)


if __name__ == "__main__":
    OUT.mkdir(exist_ok=True)
    agile_cycle()
    daily_commits()
    screenshots()
    print("ok:", sorted(p.name for p in OUT.iterdir()))
