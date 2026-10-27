"""Raw (text) copy of Ghidra's responses for every function of a program.

Read-only (see ghidra_mcp.py). For each function of list_functions:
get_function_info, get_disassembly, get_callers, get_callees, get_xrefs_to.
Adds list_imports, list_memory_blocks and list_strings.

Writes: captures/ghidra-cache/<program>.json (text responses left untouched),
so that the analysis (inventory_happy.py) can be rerun without Ghidra.

Usage: python -I scripts/ghidra_dump.py happy.exe
"""
import json
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from ghidra_mcp import Ghidra  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT_DIR = ROOT / "captures" / "ghidra-cache"


def main():
    prog = sys.argv[1] if len(sys.argv) > 1 else "happy.exe"
    pn = "/" + prog
    g = Ghidra()
    out = {"program": prog, "server": g.server}
    out["list_functions"] = g.call("list_functions", program_name=pn,
                                   limit=10000)
    out["list_imports"] = g.call("list_imports", program_name=pn, limit=10000)
    out["list_memory_blocks"] = g.call("list_memory_blocks", program_name=pn)
    out["list_strings"] = g.call("list_strings", program_name=pn, limit=10000,
                                 min_length=1)
    addrs = re.findall(r"@ ([0-9a-f]{8}) \(", out["list_functions"])
    funcs = {}
    for i, a in enumerate(addrs):
        fa = "0x" + a
        d = {}
        for tool, args in (("get_function_info", {"function": fa}),
                           ("get_disassembly", {"function": fa}),
                           ("get_callers", {"function": fa, "limit": 10000}),
                           ("get_callees", {"function": fa, "limit": 10000}),
                           ("get_xrefs_to", {"address": fa, "limit": 10000})):
            try:
                d[tool] = g.call(tool, program_name=pn, **args)
            except RuntimeError as e:
                d[tool] = "ERROR: %s" % e
        funcs[a] = d
        print("%3d/%d %s" % (i + 1, len(addrs), a), file=sys.stderr)
    out["functions"] = funcs
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    path = OUT_DIR / (prog.replace(".", "_") + ".json")
    path.write_text(json.dumps(out, indent=1), encoding="utf-8")
    print("written: %s (%d functions)" % (path.relative_to(ROOT), len(funcs)))


if __name__ == "__main__":
    main()
