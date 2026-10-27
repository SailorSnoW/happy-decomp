"""Matching progress of the GAME functions, from reccmp.

Runs reccmp-reccmp --json on each built target (scripts/build.py first), then
crosses the result with the GAME functions of notes/data/functions-<exe>.csv.
LIBRARY functions (CRT) are left out of the counts.

Outputs:
- notes/data/progress-<exe>.csv: one row per GAME function (address, size,
  block, name in the source, matching);
- a summary per exe and per block on the standard output.

Usage: .venv/Scripts/python scripts/progress.py [happy] [maped] [loader]
"""
import csv
import json
import pathlib
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
BUILD = ROOT / "build"
EXES = ("happy", "maped", "loader")


def reccmp(exe):
    tool = shutil.which("reccmp-reccmp")
    if tool is None:
        sys.exit("reccmp-reccmp not found in PATH")
    out = BUILD / exe / "reccmp.json"
    p = subprocess.run([tool, "--target", exe.upper(), "--json", str(out),
                        "--silent", "--no-color"], cwd=ROOT,
                       capture_output=True, text=True, errors="replace")
    errors = [ln for ln in (p.stdout + p.stderr).splitlines()
              if ln.startswith("[ERROR]")]
    if p.returncode != 0 or not out.exists():
        sys.exit((p.stdout + p.stderr).strip())
    data = json.loads(out.read_text(encoding="utf-8"))["data"]
    return {int(d["address"], 16): d for d in data}, errors


def run(exe):
    if not (BUILD / exe / (exe + ".pdb")).exists():
        print("== %s: not built" % exe)
        return
    got, errors = reccmp(exe)
    rows = [r for r in csv.DictReader(open(
        ROOT / "notes" / "data" / ("functions-%s.csv" % exe), encoding="utf-8"))
        if r["class"] == "GAME"]
    out = []
    for r in rows:
        a = int(r["address"], 16)
        d = got.get(a)
        out.append({"address": r["address"], "size": r["size"],
                    "block": r["block"], "name": d["name"] if d else "",
                    "matching": "%.2f" % (100 * d["matching"]) if d else ""})
    with open(ROOT / "notes" / "data" / ("progress-%s.csv" % exe), "w",
              newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(out[0]))
        w.writeheader()
        w.writerows(out)

    def line(label, lst):
        size = sum(int(r["size"]) for r in lst)
        done = [r for r in lst if r["matching"]]
        full = [r for r in done if r["matching"] == "100.00"]
        return "  %-9s %3d/%3d annotated, %3d at 100%%, bytes at 100%%: %5.1f%%" % (
            label, len(done), len(lst), len(full),
            100 * sum(int(r["size"]) for r in full) / size if size else 0)

    print("== %s.exe" % exe)
    print(line("total", out))
    for b in sorted({r["block"] for r in out}):
        print(line(b, [r for r in out if r["block"] == b]))
    for r in out:
        if r["matching"] and r["matching"] != "100.00":
            print("  below 100%%: %s %-24s %s%%" % (r["address"], r["name"],
                                                 r["matching"]))
    for e in errors:
        print("  " + e)


if __name__ == "__main__":
    for e in sys.argv[1:] or EXES:
        run(e)
