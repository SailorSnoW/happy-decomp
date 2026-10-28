"""JSON copy of GhidraMCP 7.0.0's responses for one program (read-only).

Goes only through ghidra_rest.Ghidra, which only allows read_only GETs.

Cache contents:
- find_functions (all internal functions);
- get_functions in batches of 20: signature, entry_point, body_start, body_end,
  classification, callers, callees ("size" field = size of the body's address
  set, chunks included);
- get_xrefs_to on each function entry (reference type);
- find_code_gaps: code bytes outside any function body. The
  complement gives the exact union of bodies, which body_start/body_end
  flatten;
- list_program_items (imports, segments), list_strings, get_entry_points,
  get_function_count.

Writes: captures/ghidra-cache/<program>_rest.json

Usage: python -I scripts/ghidra_dump_rest.py maped.exe
"""
import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from ghidra_rest import Ghidra  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT_DIR = ROOT / "captures" / "ghidra-cache"
FIELDS = "signature,entry_point,body_start,body_end,classification,callers,callees"


def main():
    prog = sys.argv[1] if len(sys.argv) > 1 else "happy.exe"
    g = Ghidra(prog)
    out = {"program": prog, "server": "GhidraMCP 7.0.0 (REST, port 8089)"}
    out["get_function_count"] = g.get("get_function_count")
    out["get_entry_points"] = g.get("get_entry_points")
    out["find_functions"] = g.get_all("find_functions", "functions")
    out["imports"] = g.get_all("list_program_items", "items", kind="imports")
    out["segments"] = g.get_all("list_program_items", "items", kind="segments")
    out["list_strings"] = g.get_all("list_strings", "strings")
    out["find_code_gaps"] = g.get_all("find_code_gaps", "gaps", page=100,
                                      min_size=1)
    addrs = ["0x" + f["address"] for f in out["find_functions"]]
    funcs, xrefs = {}, {}
    for i in range(0, len(addrs), 20):
        lot = addrs[i:i + 20]
        r = g.get("get_functions", functions=",".join(lot), fields=FIELDS,
                  include_call_context="false")
        funcs.update(r["functions"])
        r = g.get("get_xrefs_to", addresses=",".join(lot), limit=0)
        xrefs.update(r["references_by_address"])
        print("%3d/%d" % (min(i + 20, len(addrs)), len(addrs)), file=sys.stderr)
    out["functions"] = funcs
    out["xrefs_to"] = xrefs
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    path = OUT_DIR / (prog.replace(".", "_") + "_rest.json")
    path.write_text(json.dumps(out, indent=1), encoding="utf-8")
    print("written: %s (%d functions, %d code gaps)" % (
        path.relative_to(ROOT), len(funcs), len(out["find_code_gaps"])))


if __name__ == "__main__":
    main()
