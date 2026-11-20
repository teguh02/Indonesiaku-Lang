#!/usr/bin/env python3
"""Uji coba eksekusi Indonesiaku (K12, K21) vs Python, dalam satu sesi.

Enam skenario: {K12, K21} x {program contoh} untuk Indonesiaku dan padanan
Python-nya, ditambah program kosong (baseline) untuk tiap runtime. Pengukuran
dilakukan bergiliran (round-robin): pada setiap putaran, semua skenario
dijalankan sekali, agar perubahan kondisi laptop (suhu, beban latar) mengenai
semua skenario secara merata. 5 putaran pemanasan + 100 putaran terukur.

Waktu = wall-clock proses (Popen -> exit, perf_counter_ns). Memori =
PeakWorkingSetSize & PeakPagefileUsage dari GetProcessMemoryInfo.

Pemakaian: python ukur_banding.py <folder_build_indk>
  <folder_build_indk> berisi subfolder 1c15f98/ dan 958ca74/ (hasil git archive
  + mingw32-make), masing-masing dengan indk.exe.
"""
import ctypes
import ctypes.wintypes as wt
import json
import platform
import statistics as st
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).parent
PY = HERE / "python"
N, WARMUP = 100, 5


class PMC(ctypes.Structure):
    _fields_ = [("cb", wt.DWORD), ("PageFaultCount", wt.DWORD), ("PeakWorkingSetSize", ctypes.c_size_t),
                ("WorkingSetSize", ctypes.c_size_t), ("QuotaPeakPagedPoolUsage", ctypes.c_size_t),
                ("QuotaPagedPoolUsage", ctypes.c_size_t), ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t),
                ("QuotaNonPagedPoolUsage", ctypes.c_size_t), ("PagefileUsage", ctypes.c_size_t),
                ("PeakPagefileUsage", ctypes.c_size_t)]


psapi = ctypes.WinDLL("psapi")


def run_once(cmd, cwd):
    t0 = time.perf_counter_ns()
    p = subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, _ = p.communicate()
    t1 = time.perf_counter_ns()
    pmc = PMC(); pmc.cb = ctypes.sizeof(PMC)
    psapi.GetProcessMemoryInfo(wt.HANDLE(int(p._handle)), ctypes.byref(pmc), pmc.cb)
    return (t1 - t0) / 1e6, pmc.PeakWorkingSetSize / 2**20, pmc.PeakPagefileUsage / 2**20, \
        out.replace(b"\r\n", b"\n"), p.returncode


def describe(xs):
    xs = sorted(xs)
    return {"median": st.median(xs), "mean": st.mean(xs), "sd": st.stdev(xs), "min": xs[0], "max": xs[-1]}


def main():
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    build = Path(sys.argv[1])
    for c in ("1c15f98", "958ca74"):
        (build / c / "kosong.idk").write_text("", encoding="utf-8")
    py = sys.executable
    sk = {
        "K12-indk":         ([str(build / "1c15f98/indk.exe"), "examples/cetak_fungsi.idk"], build / "1c15f98"),
        "K12-python":       ([py, str(PY / "cetak_fungsi.py")], PY),
        "K21-indk":         ([str(build / "958ca74/indk.exe"), "examples/untuk_lingkup.idk"], build / "958ca74"),
        "K21-python":       ([py, str(PY / "untuk_lingkup.py")], PY),
        "baseline-indk-K12": ([str(build / "1c15f98/indk.exe"), "kosong.idk"], build / "1c15f98"),
        "baseline-indk-K21": ([str(build / "958ca74/indk.exe"), "kosong.idk"], build / "958ca74"),
        "baseline-python":  ([py, str(PY / "kosong.py")], PY),
    }
    data = {k: {"t": [], "ws": [], "pf": [], "out": set(), "rc": set()} for k in sk}
    for r in range(WARMUP + N):
        for k, (cmd, cwd) in sk.items():
            t, ws, pf, out, rc = run_once(cmd, cwd)
            if r >= WARMUP:
                d = data[k]; d["t"].append(t); d["ws"].append(ws); d["pf"].append(pf); d["out"].add(out); d["rc"].add(rc)
    golden = {
        "K12-indk": (build / "1c15f98/tests/expected/cetak_fungsi.out").read_bytes().replace(b"\r\n", b"\n"),
        "K21-indk": (build / "958ca74/tests/expected/untuk_lingkup.out").read_bytes().replace(b"\r\n", b"\n"),
    }
    hasil = {"lingkungan": {"python": sys.version, "implementasi": platform.python_implementation(),
                            "platform": platform.platform(), "runs": N, "warmup": WARMUP, "urutan": "round-robin"}}
    for k, d in data.items():
        out = next(iter(d["out"]))
        hasil[k] = {"exit_codes": sorted(d["rc"]), "keluaran_konsisten": len(d["out"]) == 1,
                    "cocok_golden": (out == golden[k]) if k in golden else None,
                    "waktu_ms": describe(d["t"]), "peak_working_set_MB": describe(d["ws"]),
                    "peak_private_MB": describe(d["pf"]), "keluaran": out.decode("utf-8", "replace")}
        h = hasil[k]
        print(f"{k:<18} exit={h['exit_codes']} golden={h['cocok_golden']} konsisten={h['keluaran_konsisten']} | "
              f"median {h['waktu_ms']['median']:.2f} ms (rerata {h['waktu_ms']['mean']:.2f} ± {h['waktu_ms']['sd']:.2f}; "
              f"{h['waktu_ms']['min']:.2f}–{h['waktu_ms']['max']:.2f}) | WS {h['peak_working_set_MB']['median']:.2f} MB | "
              f"private {h['peak_private_MB']['median']:.2f} MB")
    (HERE / "hasil_banding.json").write_text(json.dumps(hasil, ensure_ascii=False, indent=2), encoding="utf-8")

    print("\n== Perbandingan keluaran baris demi baris (Indonesiaku vs Python)")
    for a, b in (("K12-indk", "K12-python"), ("K21-indk", "K21-python")):
        la, lb = hasil[a]["keluaran"].splitlines(), hasil[b]["keluaran"].splitlines()
        sama = sum(1 for x, y in zip(la, lb) if x == y)
        print(f"{a[:3]}: {len(la)} vs {len(lb)} baris; identik {sama}")
        for i, (x, y) in enumerate(zip(la, lb), 1):
            if x != y:
                print(f"   baris {i}: indk={x!r}  python={y!r}")


if __name__ == "__main__":
    main()
