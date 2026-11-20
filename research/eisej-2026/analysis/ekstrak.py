#!/usr/bin/env python3
"""Ekstraksi data repositori Indonesiaku untuk studi kasus (Paper E, target EISEJ).

Pemakaian:  python ekstrak.py <path_repo_lokal> [snapshot_ref]
Keluaran (folder data/):
  snapshot.json      hash & tanggal snapshot, waktu ekstraksi
  commits.csv        semua commit non-merge di snapshot (ukuran, trailer co-author, berkas)
  commit_files.csv   baris per (commit, berkas) dengan +/-
  prs.json           semua PR (GitHub API) + commit tiap PR
  runs.json          semua workflow run GitHub Actions (event, branch, sha, conclusion, waktu)
  rencana.csv        dokumen rencana fitur: path, status di snapshot, tanggal dibuat/selesai (git)
Hanya membaca repo (git log/show) dan GitHub API (gh); tidak mengubah apa pun.
"""
from __future__ import annotations

import csv
import json
import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).parent
DATA = HERE / "data"
SEP = "\x1e"


def run(args: list[str], cwd: Path) -> str:
    return subprocess.run(args, cwd=cwd, capture_output=True, text=True, encoding="utf-8",
                          errors="replace", check=True).stdout


def gh_json(path: str, cwd: Path) -> list | dict:
    out = run(["gh", "api", "--paginate", path], cwd)
    # --paginate menyambung beberapa array/objek JSON
    dec, i, items = json.JSONDecoder(), 0, []
    out = out.strip()
    while i < len(out):
        obj, j = dec.raw_decode(out, i)
        items.append(obj)
        i = j
        while i < len(out) and out[i] in " \r\n":
            i += 1
    if all(isinstance(x, list) for x in items):
        return [y for x in items for y in x]
    return items


def commits(repo: Path, ref: str) -> None:
    fmt = SEP + "%H%x1f%an%x1f%ae%x1f%aI%x1f%cI%x1f%P%x1f%s%x1f%(trailers:key=Co-authored-by,valueonly,separator=%x1d)"
    out = run(["git", "log", ref, "--no-merges", "--numstat", f"--format={fmt}"], repo)
    rows, frows = [], []
    for block in out.split(SEP)[1:]:
        head, _, stat = block.partition("\n")
        h, an, ae, ad, cd, parents, subj, trailers = head.split("\x1f")
        add = dele = nfile = 0
        files = []
        for line in stat.strip().splitlines():
            parts = line.split("\t")
            if len(parts) != 3:
                continue
            a, d, f = parts
            a = int(a) if a.isdigit() else 0
            d = int(d) if d.isdigit() else 0
            add += a; dele += d; nfile += 1
            files.append(f)
            frows.append({"hash": h, "file": f, "add": a, "del": d})
        m = re.match(r"^(\w+)(\(([^)]*)\))?!?:", subj)
        rows.append({
            "hash": h, "author": an, "email": ae, "author_date": ad, "commit_date": cd,
            "subject": subj, "type": m.group(1).lower() if m else "",
            "scope": (m.group(3) or "") if m else "",
            "coauthors": trailers.replace("\x1d", " | ").strip(),
            "add": add, "del": dele, "files": nfile,
        })
    with open(DATA / "commits.csv", "w", newline="", encoding="utf-8") as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0]))
        w.writeheader(); w.writerows(rows)
    with open(DATA / "commit_files.csv", "w", newline="", encoding="utf-8") as fh:
        w = csv.DictWriter(fh, fieldnames=["hash", "file", "add", "del"])
        w.writeheader(); w.writerows(frows)
    print("commit:", len(rows))


def prs(repo: Path, slug: str) -> None:
    lst = gh_json(f"repos/{slug}/pulls?state=all&per_page=100", repo)
    out = []
    for p in lst:
        n = p["number"]
        det = gh_json(f"repos/{slug}/pulls/{n}", repo)[0]
        cms = gh_json(f"repos/{slug}/pulls/{n}/commits?per_page=100", repo)
        out.append({
            "number": n, "title": p["title"], "user": p["user"]["login"], "state": p["state"],
            "created_at": p["created_at"], "closed_at": p["closed_at"], "merged_at": p["merged_at"],
            "merge_commit_sha": p["merge_commit_sha"], "head_ref": p["head"]["ref"], "head_sha": p["head"]["sha"],
            "base_ref": p["base"]["ref"], "body": p.get("body") or "",
            "additions": det.get("additions"), "deletions": det.get("deletions"),
            "changed_files": det.get("changed_files"), "n_commits": det.get("commits"),
            "commits": [{"sha": c["sha"], "msg": c["commit"]["message"],
                         "author_email": (c["commit"]["author"] or {}).get("email")} for c in cms],
        })
    (DATA / "prs.json").write_text(json.dumps(out, ensure_ascii=False, indent=1), encoding="utf-8")
    print("PR:", len(out))


def runs(repo: Path, slug: str) -> None:
    pages = gh_json(f"repos/{slug}/actions/runs?per_page=100", repo)
    allr = []
    for pg in pages if isinstance(pages, list) else [pages]:
        allr += pg.get("workflow_runs", []) if isinstance(pg, dict) else []
    keep = [{k: r.get(k) for k in ("id", "name", "event", "head_branch", "head_sha", "status", "conclusion",
                                   "created_at", "updated_at", "run_attempt")} |
            {"pr_numbers": [x["number"] for x in r.get("pull_requests") or []]} for r in allr]
    (DATA / "runs.json").write_text(json.dumps(keep, ensure_ascii=False, indent=1), encoding="utf-8")
    print("workflow run:", len(keep))


def rencana(repo: Path, ref: str) -> None:
    files = [f for f in run(["git", "ls-tree", "-r", "--name-only", ref, "docs/RENCANA_FITUR"], repo).splitlines()
             if f.endswith(".md") and not f.endswith("TEMPLATE.md")]
    rows = []
    for f in files:
        name = Path(f).name
        # tanggal dibuat: commit pertama yang menambah berkas (ikuti rename)
        log = run(["git", "log", ref, "--follow", "--format=%aI%x1f%H%x1f%s", "--name-status", "--", f], repo)
        entries = re.findall(r"^(\S+)\x1f(\w+)\x1f(.*)$", log, re.M)
        created = entries[-1][0] if entries else ""
        # tanggal selesai: commit pertama di mana berkas berada di folder selesai/
        done = ""
        if "/selesai/" in f:
            log2 = run(["git", "log", ref, "--diff-filter=AR", "--format=%aI", "--", f], repo).split()
            done = log2[-1] if log2 else ""
        txt = run(["git", "show", f"{ref}:{f}"], repo)
        st = re.search(r"\*\*Status:\*\*\s*([^\n]+)", txt)
        ver = re.search(r"\*\*Target versi:\*\*\s*([^\n]+)", txt)
        kep = re.search(r"\*\*Keputusan terkait:\*\*\s*([^\n]+)", txt)
        rows.append({"file": f, "name": name, "status": st.group(1).strip() if st else "",
                     "target_versi": ver.group(1).strip() if ver else "",
                     "keputusan": kep.group(1).strip() if kep else "",
                     "created": created, "done": done, "n_commit_menyentuh": len(entries)})
    with open(DATA / "rencana.csv", "w", newline="", encoding="utf-8") as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0]))
        w.writeheader(); w.writerows(rows)
    print("rencana fitur:", len(rows))


def main() -> None:
    repo = Path(sys.argv[1])
    ref = sys.argv[2] if len(sys.argv) > 2 else "origin/main"
    DATA.mkdir(exist_ok=True)
    slug = run(["gh", "repo", "view", "--json", "nameWithOwner", "-q", ".nameWithOwner"], repo).strip()
    h = run(["git", "rev-parse", ref], repo).strip()
    snap = {"repo": slug, "ref": ref, "hash": h,
            "commit_date": run(["git", "show", "-s", "--format=%cI", h], repo).strip(),
            "diekstrak": datetime.now(timezone.utc).isoformat(timespec="seconds")}
    (DATA / "snapshot.json").write_text(json.dumps(snap, indent=1), encoding="utf-8")
    print(snap)
    commits(repo, h)
    rencana(repo, h)
    prs(repo, slug)
    runs(repo, slug)


if __name__ == "__main__":
    main()
