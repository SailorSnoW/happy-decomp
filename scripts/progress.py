"""Matching progress of the game functions, from reccmp.

Runs reccmp-reccmp --json on each built target (scripts/build.py first), then
crosses the result with the functions annotated in src/ that lie in the game
code of the exe (GAME_CODE: from 0x00401000 to the first CRT function). The
CRT (src/crt/, LIBRARY) is left out of the counts; the Allegro header code
annotated as LIBRARY inside the game code (happy 0x00402110) is counted.
reccmp-reccmp is run through scripts/reccmp_run.py (pointers just outside a
variable compared by their offset, notes/matching.md §3), with the Python in
which reccmp is installed (outside .venv).

Outputs:
- notes/data/progress-<exe>.csv: one row per game function (address, block,
  name in the source, matching);
- a summary per exe and per block on the standard output.

Usage: python scripts/progress.py [happy] [maped] [loader]
"""
import csv
import json
import pathlib
import re
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
BUILD = ROOT / "build"
SRC = ROOT / "src"
EXES = ("happy", "maped", "loader")
# Game code of each exe: [start, first CRT function) (notes/layout-<exe>.md §1).
GAME_CODE = {"happy": (0x00401000, 0x0040AD50),
             "maped": (0x00401000, 0x00405CA0),
             "loader": (0x00401000, 0x004014D0)}
# src/common/ files are named after happy's blocks; maped's block of each.
COMMON_BLOCKS = {"maped": {"block_01": "block_03", "block_09": "block_01"}}
ANNOTATION = re.compile(r"//\s*(?:FUNCTION|LIBRARY):\s*(\w+)\s+(0x[0-9a-fA-F]+)")


def game_functions(exe):
    """{address: block} of the annotations of src/ in the game code of exe."""
    lo, hi = GAME_CODE[exe]
    out = {}
    for path in sorted(SRC.rglob("*.[ch]")):
        if path.parent.name == "crt":
            continue
        block = path.stem
        if path.parent.name == "common":
            block = COMMON_BLOCKS.get(exe, {}).get(block, block)
        for m in ANNOTATION.finditer(path.read_text(encoding="latin-1")):
            a = int(m.group(2), 16)
            if m.group(1) == exe.upper() and lo <= a < hi:
                out[a] = block
    return out


def reccmp_python():
    """The Python interpreter in which reccmp-reccmp is installed."""
    tool = shutil.which("reccmp-reccmp")
    if tool is None:
        sys.exit("reccmp-reccmp not found in PATH")
    tool = pathlib.Path(tool)
    cands = []
    with open(tool, "rb") as f:
        first = f.readline()
    if first.startswith(b"#!"):
        cands.append(first[2:].strip().decode())
    cands += [str(tool.parent.parent / "python.exe"), str(tool.parent / "python.exe"),
              str(tool.parent / "python3"), "python3", "python"]
    for c in cands:
        try:
            r = subprocess.run([c, "-c", "import reccmp"], capture_output=True)
        except OSError:
            continue
        if r.returncode == 0:
            return c
    sys.exit("no Python with reccmp found for %s" % tool)


def reccmp(exe):
    out = BUILD / exe / "reccmp.json"
    p = subprocess.run([reccmp_python(), str(ROOT / "scripts" / "reccmp_run.py"),
                        "--target", exe.upper(), "--json", str(out),
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
    out = []
    for a, block in sorted(game_functions(exe).items()):
        d = got.get(a)
        out.append({"address": "0x%08X" % a, "block": block,
                    "name": d["name"] if d else "",
                    "matching": "%.2f" % (100 * d["matching"]) if d else ""})
    with open(ROOT / "notes" / "data" / ("progress-%s.csv" % exe), "w",
              newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(out[0]))
        w.writeheader()
        w.writerows(out)

    def line(label, lst):
        done = [r for r in lst if r["matching"]]
        full = [r for r in done if r["matching"] == "100.00"]
        return "  %-9s %3d/%3d compared, %3d at 100%%" % (
            label, len(done), len(lst), len(full))

    print("== %s.exe" % exe)
    print(line("total", out))
    for b in sorted({r["block"] for r in out}):
        print(line(b, [r for r in out if r["block"] == b]))
    for r in out:
        if r["matching"] != "100.00":
            print("  below 100%%: %s %-24s %s%%" % (r["address"], r["name"],
                                                 r["matching"] or "-"))
    for e in errors:
        print("  " + e)


if __name__ == "__main__":
    for e in sys.argv[1:] or EXES:
        run(e)
