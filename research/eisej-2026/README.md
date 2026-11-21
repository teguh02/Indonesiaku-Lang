# Replication package — AI-Assisted Solo Agile Development of an Indonesian-Keyword Programming Language

Replication package for the article submitted to e-Informatica Software Engineering Journal
(author: Teguh Rijanandi).

Studied project: https://github.com/teguh02/Indonesiaku-Lang
Analysed snapshot: commit e718dc304a9ecad4d211cd161c2ed086b1447a55 (18 November 2026),
data extracted on 20 November 2026.

## Contents

| Path | Content |
|---|---|
| `analysis/ekstrak.py` | Extracts commits (git), pull requests and workflow runs (GitHub REST API via `gh`) and feature-plan metadata from a local clone |
| `analysis/analisis.py` | Computes all RQ1/RQ2 results (medians, IQR, Mann-Whitney U, Cliff's delta, bootstrap CI, Fisher's exact test) |
| `analysis/data/` | Raw data used in the paper (`commits.csv`, `commit_files.csv`, `prs.json`, `runs.json`, `rencana.csv`, `snapshot.json`) |
| `analysis/hasil/` | Results: `ringkasan.json` (all reported numbers), `tabel_commit.csv`, `tabel_pr.csv` |
| `benchmark/` | Execution benchmark script (`ukur_banding.py`), results (`hasil_banding.json`), Python translations of the example programs (`python/`) and figure script |
| `figures/` | Scripts that draw the figures of the paper |

Personal e-mail addresses in the raw data were replaced by `<redacted>`; they are not used by the analysis.
Workflow-run records are archived here because GitHub may delete old run metadata.

## Reproducing the results

Requirements: Python 3.12+ with `numpy`, `scipy`, `matplotlib`; for re-extraction also `git` and the GitHub CLI (`gh`).

```
# 1) (optional) re-extract the raw data from a clone of the studied repository
git clone https://github.com/teguh02/Indonesiaku-Lang
python analysis/ekstrak.py Indonesiaku-Lang e718dc304a9ecad4d211cd161c2ed086b1447a55

# 2) recompute all results from analysis/data/
python analysis/analisis.py
```

Step 2 rewrites `analysis/hasil/ringkasan.json`, which contains every number reported for RQ1 and RQ2.

The benchmark (`benchmark/ukur_banding.py`) needs Windows (it reads process memory counters through
`GetProcessMemoryInfo`), the interpreters built from commits `1c15f98` (K12) and `958ca74` (K21) with
`mingw32-make`, and Python 3.14. Usage is documented at the top of the script.

The scripts in `figures/` are included for transparency; they expect the author's original folder
layout (input paths are set at the top of each script).
