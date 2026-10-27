"""Carries the names and types of src/ into Ghidra (GhidraMCP 7.0.0, port 8089).

Reads the annotations of src/ (except src/crt/):
- `// FUNCTION:` and `// LIBRARY:` + the definition (or the `// name` line
  of a lookup by name): function name and prototype;
- `// GLOBAL:` + the declaration: global name and type.
Compares with Ghidra and lists the differences. With --apply, writes them
through the only write endpoints allowed here (CLAUDE.md, "Writes"):
rename_function, rename_symbol, set_function_prototype, apply_data_type.
With --save (implies --apply), waits for the auto-analysis that the writes
trigger to end, then saves the program: a save requested while the
analysis runs waits behind it and blocks the server
(notes/matching.md §3b).

Types that do not exist in Ghidra (Allegro's BITMAP, DATAFILE, FONT,
COLOR_MAP...) become `void *` in prototypes; a global of such a type keeps
its Ghidra type and only gets its name. `fixed` becomes `int`.

Usage: .venv/Scripts/python scripts/ghidra_sync.py [--apply | --save] [happy] [maped] [loader]
"""
import json
import pathlib
import re
import sys
import time
import urllib.parse
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "src"
URL = "http://127.0.0.1:8089/"
WRITES = {"rename_function", "rename_symbol", "set_function_prototype",
          "apply_data_type"}
EXES = {"happy": "HAPPY", "maped": "MAPED", "loader": "LOADER"}

MARKER = re.compile(r"^\s*// (FUNCTION|LIBRARY|GLOBAL): (\w+) (0x[0-9a-fA-F]+)")
FUNC_DEF = re.compile(r"^([A-Za-z_][\w\s\*]*?)\b(\w+)\s*\((.*)\)\s*$")
# the line ends with ";", or with "= {" for an initialiser spread over several lines
VAR_DECL = re.compile(r"^((?:\w+\s+)*\w+[\s\*]+?)(\w+)((?:\[[^\]]*\])*)\s*(=.*)?(?:;|\{\s*$)")
SCALARS = {"void", "int", "char", "uchar", "uint", "short", "ushort", "long",
           "ulong", "float", "double"}


def http(tool, method="GET", program=None, timeout=120, **args):
    query = {"program": program} if program else {}
    if method == "GET":
        query.update(args)
        req = urllib.request.Request(URL + tool + "?" + urllib.parse.urlencode(query))
    else:
        if tool not in WRITES:
            raise ValueError("write endpoint not allowed: " + tool)
        req = urllib.request.Request(
            URL + tool + "?" + urllib.parse.urlencode(query),
            data=json.dumps(args).encode(), method="POST",
            headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return json.loads(r.read().decode("utf-8"))


def annotations(target):
    """[(kind, address, name, text)] for one target; text is the
    definition or declaration line, None for a lookup by name."""
    out = []
    for f in sorted(SRC.rglob("*.[ch]")):
        if f.parent.name == "crt":
            continue
        lines = f.read_text(encoding="utf-8", errors="replace").splitlines()
        for i, line in enumerate(lines):
            m = MARKER.match(line)
            if not m or m.group(2) != target:
                continue
            for nxt in lines[i + 1:i + 12]:
                s = nxt.strip()
                if MARKER.match(nxt) or not s or s.startswith("/*") or s.startswith("*"):
                    continue
                if s.startswith("//"):
                    out.append((m.group(1), int(m.group(3), 16), s[2:].strip(), None))
                    break
                if m.group(1) == "GLOBAL":
                    d = VAR_DECL.match(s)
                    if d:
                        out.append((m.group(1), int(m.group(3), 16), d.group(2), s))
                    break
                d = FUNC_DEF.match(s)
                if d:
                    out.append((m.group(1), int(m.group(3), 16), d.group(2), s))
                break
    return out


def ghidra_type(t, known, pointer_ok):
    """C type of src/ -> type name for Ghidra, or None if unknown."""
    t = re.sub(r"\b(volatile|const)\b", "", t).strip()
    t = re.sub(r"\bunsigned\s+(char|int|short|long)\b", r"u\1", t)
    t = re.sub(r"\bfixed\b", "int", t)
    t = re.sub(r"\s+", " ", t).replace(" *", "*").strip()
    base, stars = t.rstrip("*").strip(), t[len(t.rstrip("*")):]
    if base in SCALARS or base in known:
        return base + (" " + stars if stars else "")
    if stars and pointer_ok:
        return "void " + stars
    return None


def prototype(defn, known):
    m = FUNC_DEF.match(defn)
    ret = ghidra_type(m.group(1), known, True)
    params = []
    for p in m.group(3).split(","):
        p = p.strip()
        if p == "void" or not p:
            continue
        # an array parameter is a pointer: char *argv[] -> char **argv
        am = re.match(r"^(.*?\w+)\s*\[\s*\w*\s*\]$", p)
        if am:
            p = re.sub(r"(\w+)$", r"*\1", am.group(1))
        pm = re.match(r"^(.*?)(\w+)$", p)
        params.append("%s %s" % (ghidra_type(pm.group(1), known, True), pm.group(2)))
    if ret is None or None in [p.split(" ")[0] for p in params]:
        return None
    return "%s __cdecl %s(%s)" % (ret, m.group(2), ", ".join(params) or "void")


def norm_sig(s):
    return re.sub(r"\s+", " ", s.replace(" *", "*").replace("*", " *")).strip()


def sync(exe, apply):
    program = exe + ".exe"
    target = EXES[exe]
    anns = annotations(target)
    known = set()
    for kind in ("struct", "typedef", "union", "enum"):
        r = http("find_data_types", program=program, kind=kind, limit=0)
        known |= {d["name"] for d in r["data_types"]}
    globs = {}
    for g in http("list_globals", program=program, limit=0)["globals"]:
        m = re.match(r"^(.*) @ ([0-9a-f]+) \[\w+\] \((.*)\)", g)
        if m:
            globs[int(m.group(2), 16)] = (m.group(1), m.group(3))
    funcs = [a for a in anns if a[0] in ("FUNCTION", "LIBRARY")]
    current = {}
    for k in range(0, len(funcs), 20):
        keys = ["0x%08x" % a[1] for a in funcs[k:k + 20]]
        r = http("get_functions", program=program, functions=",".join(keys),
                 fields="signature", include_call_context="false")
        for key in keys:
            f = r["functions"].get(key)
            if f and "name" in f:
                current[int(key, 16)] = (f["name"], f.get("signature", ""))
    todo = []
    for kind, addr, name, text in anns:
        a = "0x%08x" % addr
        if kind in ("FUNCTION", "LIBRARY"):
            if addr not in current:
                print("  %s %s: no function in Ghidra, skipped" % (a, name))
                continue
            gname, gsig = current[addr]
            # import thunks (maped _d_*_proc): Ghidra names them after the
            # import, without the C decoration of the PDB name; renaming a
            # thunk would rename the external function too
            if gname != name and "_" + gname != name:
                todo.append(("rename_function", dict(function_address=a, new_name=name, strict_mode="off"),
                             "%s %s -> %s" % (a, gname, name)))
            proto = prototype(text, known) if text else None
            if proto and norm_sig(proto.replace(" " + name + "(", " %s(" % gname)) != norm_sig(gsig) \
                    and norm_sig(proto) != norm_sig(gsig):
                todo.append(("set_function_prototype", dict(function_address=a, prototype=proto),
                             "%s %s -> %s" % (a, gsig, proto)))
        else:
            if addr not in globs:
                # list_globals leaves out the globals that still have a
                # default name (DAT_..., OBJ_ARRAY_...)
                r = http("audit_global", program=program, address=a)
                globs[addr] = (r.get("name"), r.get("type"))
            gname, gtype = globs[addr]
            if text:
                d = VAR_DECL.match(text)
                dims = re.findall(r"\[([^\]]*)\]", d.group(3))
                t = ghidra_type(d.group(1), known, False)
                if t and len(dims) == 1 and dims[0].isdigit():
                    t = "%s[%s]" % (t, dims[0])
                elif dims:
                    t = None
                if t and gtype is not None and gtype.replace(" ", "") == t.replace(" ", ""):
                    t = None
                if t:
                    todo.append(("apply_data_type", dict(address=a, type_name=t),
                                 "%s %s: type %s -> %s" % (a, name, gtype, t)))
            # DAT_<address> in src/ = no name yet: Ghidra's default name
            # (DAT_..., OBJ_ARRAY_...) is kept, renaming would only add a label
            default = not gname or gname.lower().endswith("%08x" % addr)
            if gname != name and not (default and name == "DAT_%08x" % addr):
                todo.append(("rename_symbol", dict(target=a, new_name=name, strict_mode="off"),
                             "%s %s -> %s" % (a, gname, name)))
    print("== %s: %d annotations, %d changes" % (program, len(anns), len(todo)))
    for tool, args, what in todo:
        print("  %-22s %s" % (tool, what))
        if apply:
            r = http(tool, method="POST", program=program, **args)
            if r.get("status") != "success" and "error" in r:
                print("    ERROR: %s" % r["error"])
    return bool(todo)


def save(exe):
    program = exe + ".exe"
    for _ in range(120):
        if not http("analysis_status", program=program)["analyzing"]:
            break
        time.sleep(5)
    else:
        sys.exit("== %s: analysis still running after 10 minutes, not saved" % program)
    http("save_program", program=program, timeout=300)
    print("== %s saved" % program)


def main():
    args = sys.argv[1:]
    do_save = "--save" in args
    apply = do_save or "--apply" in args
    exes = [a for a in args if not a.startswith("--")] or ["happy", "maped"]
    for exe in exes:
        changed = sync(exe, apply)
        if do_save and changed:
            save(exe)
    if not apply:
        print("(dry run: --apply to write, --save to write and save)")


if __name__ == "__main__":
    main()
