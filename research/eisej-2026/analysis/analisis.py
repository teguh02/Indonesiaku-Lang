#!/usr/bin/env python3
"""Analisis studi kasus repositori Indonesiaku (RQ1, RQ2) dari data/ hasil ekstrak.py.

Kategori atribusi commit (urutan prioritas):
  AI       : trailer Co-authored-by memuat agen AI (Claude/Anthropic/Copilot/OpenAI/Codex/Gemini)
  CAMPURAN : author atau co-author root@hermes (menurut pemilik repo: kadang manusia, kadang agen AI)
  LAIN     : kontributor manusia lain (bukan pengembang utama)
  MANUSIA  : pengembang utama tanpa penanda AI
Penanda AI adalah BATAS BAWAH keterlibatan AI (tidak semua bantuan meninggalkan trailer).

Statistik robust (Kitchenham dkk. 2017): median, IQR, Mann-Whitney U (dua sisi),
Cliff's delta, CI bootstrap 95% untuk selisih median (10.000 resample, seed tetap).
Keluaran: hasil/ringkasan.json, hasil/tabel_*.csv, hasil/laporan.md
"""
from __future__ import annotations

import csv
import json
import re
from collections import Counter, defaultdict
from datetime import datetime, timedelta
from pathlib import Path

import numpy as np
from scipy import stats

HERE = Path(__file__).parent
DATA, OUT = HERE / "data", HERE / "hasil"
RNG = np.random.default_rng(20261120)
AI_RX = re.compile(r"claude|anthropic|copilot|openai|codex|gemini|cursor", re.I)
HERMES_RX = re.compile(r"hermes\.nandafa", re.I)
LAIN_RX = re.compile(r"rafaoktahabibilah|Rafa Okta", re.I)
JENIS = {
    "fitur": {"feat", "fitur", "tambah"},
    "perbaikan": {"fix", "perbaiki", "perbaikan", "hotfix"},
    "dokumentasi": {"docs", "doc", "dokumen"},
    "uji": {"test", "tes", "uji"},
}
REWORK_HARI = 7


def tgl(s: str) -> datetime:
    return datetime.fromisoformat(s.replace("Z", "+00:00"))


def jenis(t: str) -> str:
    for k, v in JENIS.items():
        if t in v:
            return k
    return "lainnya"


def kategori(c: dict) -> str:
    if LAIN_RX.search(c["email"] + c["author"]):
        return "LAIN"
    if AI_RX.search(c["coauthors"]):
        return "AI"
    if HERMES_RX.search(c["email"]) or HERMES_RX.search(c["coauthors"]):
        return "CAMPURAN"
    if LAIN_RX.search(c["email"] + c["author"]):
        return "LAIN"
    return "MANUSIA"


def ringkas(x) -> dict:
    x = np.asarray(x, float)
    if len(x) == 0:
        return {"n": 0}
    q1, md, q3 = np.percentile(x, [25, 50, 75])
    return {"n": int(len(x)), "median": float(md), "q1": float(q1), "q3": float(q3),
            "min": float(x.min()), "max": float(x.max())}


def cliffs_delta(a, b) -> float:
    a, b = np.asarray(a, float), np.asarray(b, float)
    gt = (a[:, None] > b[None, :]).sum()
    lt = (a[:, None] < b[None, :]).sum()
    return float((gt - lt) / (len(a) * len(b)))


def boot_median_diff(a, b, n=10000) -> tuple[float, float]:
    a, b = np.asarray(a, float), np.asarray(b, float)
    d = [np.median(RNG.choice(a, len(a))) - np.median(RNG.choice(b, len(b))) for _ in range(n)]
    return float(np.percentile(d, 2.5)), float(np.percentile(d, 97.5))


def banding(a, b) -> dict:
    if len(a) < 3 or len(b) < 3:
        return {"catatan": "n terlalu kecil"}
    u, p = stats.mannwhitneyu(a, b, alternative="two-sided")
    lo, hi = boot_median_diff(a, b)
    d = cliffs_delta(a, b)
    mag = "dapat diabaikan" if abs(d) < 0.147 else "kecil" if abs(d) < 0.33 else "sedang" if abs(d) < 0.474 else "besar"
    return {"U": float(u), "p": float(p), "cliffs_delta": d, "besaran": mag,
            "selisih_median": float(np.median(a) - np.median(b)), "ci95_selisih_median": [lo, hi]}


def proporsi(ka, na, kb, nb) -> dict:
    _, p = stats.fisher_exact([[ka, na - ka], [kb, nb - kb]])
    return {"a": f"{ka}/{na}", "b": f"{kb}/{nb}", "pa": ka / na if na else None, "pb": kb / nb if nb else None,
            "selisih": (ka / na - kb / nb) if na and nb else None, "p_fisher": float(p)}


def muat():
    commits = list(csv.DictReader(open(DATA / "commits.csv", encoding="utf-8")))
    files = defaultdict(list)
    for r in csv.DictReader(open(DATA / "commit_files.csv", encoding="utf-8")):
        files[r["hash"]].append(r["file"])
    for c in commits:
        c["kat"] = kategori(c)
        c["jenis"] = jenis(c["type"])
        c["t"] = tgl(c["author_date"])
        c["churn"] = int(c["add"]) + int(c["del"])
        c["nfile"] = int(c["files"])
        c["flist"] = files[c["hash"]]
        c["kode"] = [f for f in c["flist"] if f.startswith("src/")]
        c["sentuh_uji"] = any(f.startswith("tests/") for f in c["flist"])
    commits.sort(key=lambda c: c["t"])
    rencana = list(csv.DictReader(open(DATA / "rencana.csv", encoding="utf-8")))
    prs = json.loads((DATA / "prs.json").read_text(encoding="utf-8")) if (DATA / "prs.json").exists() else []
    runs = json.loads((DATA / "runs.json").read_text(encoding="utf-8")) if (DATA / "runs.json").exists() else []
    snap = json.loads((DATA / "snapshot.json").read_text(encoding="utf-8"))
    return commits, rencana, prs, runs, snap


def tandai_rework(commits: list[dict]) -> None:
    """Commit kode X dianggap 'disusul perbaikan' bila dalam REWORK_HARI hari ada commit berjenis
    perbaikan (oleh siapa pun) yang menyentuh >=1 berkas src/ yang sama."""
    for i, c in enumerate(commits):
        c["disusul_perbaikan"] = None
        if not c["kode"]:
            continue
        s = set(c["kode"])
        batas = c["t"] + timedelta(days=REWORK_HARI)
        c["disusul_perbaikan"] = any(
            d["jenis"] == "perbaikan" and d["t"] <= batas and s & set(d["kode"])
            for d in commits[i + 1:] if d["t"] <= batas)


def rq1(commits, rencana, prs, runs, snap) -> dict:
    res = {"snapshot": snap}
    t0, t1 = commits[0]["t"], commits[-1]["t"]
    res["periode"] = [t0.date().isoformat(), t1.date().isoformat(), (t1 - t0).days]
    bulan = Counter(c["t"].strftime("%Y-%m") for c in commits)
    res["commit_per_bulan"] = dict(sorted(bulan.items()))
    res["jenis_commit"] = Counter(c["jenis"] for c in commits)
    sel = [r for r in rencana if "/selesai/" in r["file"]]
    lt = []
    sama_hari = 0
    for r in sel:
        if r["created"] and r["done"]:
            d = (tgl(r["done"]) - tgl(r["created"])).total_seconds() / 86400
            lt.append(max(d, 0))
            sama_hari += tgl(r["done"]).date() == tgl(r["created"]).date()
    res["rencana"] = {"total": len(rencana), "selesai": len(sel), "belum_selesai": len(rencana) - len(sel),
                      "lead_time_hari": ringkas(lt), "dibuat_dan_selesai_hari_sama": sama_hari,
                      "dengan_kode_keputusan": sum(1 for r in sel if r["keputusan"].strip() not in ("", "-", "—"))}
    if prs:
        merged = [p for p in prs if p["merged_at"]]
        lt_pr = [(tgl(p["merged_at"]) - tgl(p["created_at"])).total_seconds() / 3600 for p in merged]
        ref_rx = re.compile(r"RENCANA_FITUR|rencana fitur|\b[KBT]-?U?\d{1,3}\b", re.I)
        rujuk = [p for p in merged if ref_rx.search(p["title"] + "\n" + p["body"] + "\n" +
                                                  "\n".join(c["msg"] for c in p["commits"]))]
        sentuh_rencana = 0
        hset = {c["hash"]: c for c in commits}
        for p in merged:
            if any(f.startswith("docs/RENCANA_FITUR") for cm in p["commits"] if cm["sha"] in hset
                   for f in hset[cm["sha"]]["flist"]):
                sentuh_rencana += 1
        res["pr"] = {"total": len(prs), "merged": len(merged), "ditutup_tanpa_merge": sum(1 for p in prs if p["state"] == "closed" and not p["merged_at"]),
                     "lead_time_jam": ringkas(lt_pr), "merujuk_rencana_atau_kode_keputusan": len(rujuk),
                     "commit_menyentuh_dokumen_rencana": sentuh_rencana,
                     "commit_per_pr": ringkas([p["n_commits"] for p in merged]),
                     "baris_berubah_per_pr": ringkas([(p["additions"] or 0) + (p["deletions"] or 0) for p in merged])}
        # commit di main yang masuk lewat PR
        pr_sha = {c["sha"] for p in merged for c in p["commits"]} | {p["merge_commit_sha"] for p in merged}
        res["commit_lewat_pr"] = f"{sum(1 for c in commits if c['hash'] in pr_sha)}/{len(commits)}"
    if runs:
        done = [r for r in runs if r["status"] == "completed"]
        concl = Counter(r["conclusion"] for r in done)
        res["ci"] = {"run_total": len(runs), "selesai": len(done), "kesimpulan": dict(concl),
                     "per_workflow": dict(Counter(r["name"] for r in runs).most_common(15)),
                     "event": dict(Counter(r["event"] for r in runs))}
        if prs:
            by_pr = defaultdict(list)
            merged = [p for p in prs if p["merged_at"]]
            sha2pr = {c["sha"]: p["number"] for p in merged for c in p["commits"]}
            for r in done:
                nums = set(r["pr_numbers"])
                if r["head_sha"] in sha2pr:
                    nums.add(sha2pr[r["head_sha"]])
                for n in nums:
                    by_pr[n].append(r)
            ada_ci = [p for p in merged if by_pr.get(p["number"])]
            pernah_gagal = [p for p in ada_ci if any(r["conclusion"] == "failure" for r in by_pr[p["number"]])]
            res["ci"]["pr_merged_dengan_run_ci"] = len(ada_ci)
            res["ci"]["pr_pernah_gagal_ci_sebelum_merge"] = len(pernah_gagal)
    return res


def rq2(commits, prs) -> dict:
    res = {"distribusi_kategori": dict(Counter(c["kat"] for c in commits))}
    tren = defaultdict(Counter)
    for c in commits:
        tren[c["t"].strftime("%Y-%m")][c["kat"]] += 1
    res["kategori_per_bulan"] = {k: dict(v) for k, v in sorted(tren.items())}
    res["jenis_per_kategori"] = {k: dict(Counter(c["jenis"] for c in commits if c["kat"] == k))
                                 for k in ("AI", "MANUSIA", "CAMPURAN", "LAIN")}
    kode = [c for c in commits if c["kode"]]
    perb = {}
    for b in ("MANUSIA", "CAMPURAN"):
        A = [c for c in kode if c["kat"] == "AI"]
        B = [c for c in kode if c["kat"] == b]
        perb[f"AI_vs_{b}"] = {
            "n_commit_kode": [len(A), len(B)],
            "churn": {"AI": ringkas([c["churn"] for c in A]), b: ringkas([c["churn"] for c in B]),
                      "uji": banding([c["churn"] for c in A], [c["churn"] for c in B])},
            "jumlah_berkas": {"AI": ringkas([c["nfile"] for c in A]), b: ringkas([c["nfile"] for c in B]),
                              "uji": banding([c["nfile"] for c in A], [c["nfile"] for c in B])},
            "ikut_menyentuh_uji": proporsi(sum(c["sentuh_uji"] for c in A), len(A),
                                           sum(c["sentuh_uji"] for c in B), len(B)),
            "disusul_perbaikan_7hari": proporsi(sum(bool(c["disusul_perbaikan"]) for c in A), len(A),
                                                sum(bool(c["disusul_perbaikan"]) for c in B), len(B)),
        }
        # stratifikasi: hanya commit berjenis fitur
        Af = [c for c in A if c["jenis"] == "fitur"]
        Bf = [c for c in B if c["jenis"] == "fitur"]
        perb[f"AI_vs_{b}"]["strata_fitur"] = {
            "n": [len(Af), len(Bf)],
            "churn": banding([c["churn"] for c in Af], [c["churn"] for c in Bf]),
            "disusul_perbaikan_7hari": proporsi(sum(bool(c["disusul_perbaikan"]) for c in Af), len(Af),
                                                sum(bool(c["disusul_perbaikan"]) for c in Bf), len(Bf)),
        }
    # kendali perancu waktu: hanya periode aktif bersama (Okt–Nov 2026), ketika AI dan manusia sama-sama commit
    aktif = [c for c in kode if c["t"].strftime("%Y-%m") in ("2026-10", "2026-11")]
    A = [c for c in aktif if c["kat"] == "AI"]
    B = [c for c in aktif if c["kat"] == "MANUSIA"]
    perb["AI_vs_MANUSIA_periode_Okt_Nov_2026"] = {
        "n_commit_kode": [len(A), len(B)],
        "churn": banding([c["churn"] for c in A], [c["churn"] for c in B]),
        "median_churn": [ringkas([c["churn"] for c in A]).get("median"), ringkas([c["churn"] for c in B]).get("median")],
        "ikut_menyentuh_uji": proporsi(sum(c["sentuh_uji"] for c in A), len(A), sum(c["sentuh_uji"] for c in B), len(B)),
        "disusul_perbaikan_7hari": proporsi(sum(bool(c["disusul_perbaikan"]) for c in A), len(A),
                                            sum(bool(c["disusul_perbaikan"]) for c in B), len(B)),
    }
    # laju dasar: proporsi commit kode yang disusul perbaikan, per bulan (semua kategori)
    lb = defaultdict(lambda: [0, 0])
    for c in kode:
        k = c["t"].strftime("%Y-%m")
        lb[k][0] += bool(c["disusul_perbaikan"]); lb[k][1] += 1
    res["laju_dasar_disusul_perbaikan_per_bulan"] = {k: f"{v[0]}/{v[1]}" for k, v in sorted(lb.items())}
    res["perbandingan_commit_kode"] = perb
    if prs:
        hset = {c["hash"]: c for c in commits}
        rows = []
        for p in prs:
            if not p["merged_at"]:
                continue
            # kategori dari commit PR sendiri (pesan + email), karena squash-merge mengganti SHA
            kats = {kategori({"coauthors": "\n".join(re.findall(r"(?im)^co-authored-by:(.*)$", c["msg"])),
                              "email": c.get("author_email") or "", "author": ""}) for c in p["commits"]}
            if p.get("merge_commit_sha") in hset:
                kats.add(hset[p["merge_commit_sha"]]["kat"])
            g = "AI" if "AI" in kats else ("CAMPURAN" if "CAMPURAN" in kats else ("MANUSIA" if kats else "?"))
            rows.append({"pr": p["number"], "grup": g,
                         "lead_jam": (tgl(p["merged_at"]) - tgl(p["created_at"])).total_seconds() / 3600,
                         "baris": (p["additions"] or 0) + (p["deletions"] or 0), "commit": p["n_commits"]})
        res["pr_per_grup"] = dict(Counter(r["grup"] for r in rows))
        A = [r for r in rows if r["grup"] == "AI"]
        B = [r for r in rows if r["grup"] == "MANUSIA"]
        res["pr_AI_vs_MANUSIA"] = {
            "lead_jam": {"AI": ringkas([r["lead_jam"] for r in A]), "MANUSIA": ringkas([r["lead_jam"] for r in B]),
                         "uji": banding([r["lead_jam"] for r in A], [r["lead_jam"] for r in B])},
            "baris": {"AI": ringkas([r["baris"] for r in A]), "MANUSIA": ringkas([r["baris"] for r in B]),
                      "uji": banding([r["baris"] for r in A], [r["baris"] for r in B])}}
        with open(OUT / "tabel_pr.csv", "w", newline="", encoding="utf-8") as fh:
            w = csv.DictWriter(fh, fieldnames=list(rows[0])) if rows else None
            if w:
                w.writeheader(); w.writerows(rows)
    return res


def main() -> None:
    OUT.mkdir(exist_ok=True)
    commits, rencana, prs, runs, snap = muat()
    tandai_rework(commits)
    with open(OUT / "tabel_commit.csv", "w", newline="", encoding="utf-8") as fh:
        keys = ["hash", "author_date", "kat", "jenis", "type", "churn", "nfile", "sentuh_uji", "disusul_perbaikan", "subject"]
        w = csv.DictWriter(fh, fieldnames=keys, extrasaction="ignore")
        w.writeheader(); w.writerows(commits)
    hasil = {"RQ1": rq1(commits, rencana, prs, runs, snap), "RQ2": rq2(commits, prs)}
    (OUT / "ringkasan.json").write_text(json.dumps(hasil, ensure_ascii=False, indent=1, default=str), encoding="utf-8")
    print(json.dumps(hasil, ensure_ascii=False, indent=1, default=str)[:6000])


if __name__ == "__main__":
    main()
