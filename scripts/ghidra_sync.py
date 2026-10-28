"""Carries the names and types of src/ into Ghidra (GhidraMCP 7.0.0, port 8089).

Reads the annotations of src/ (except src/crt/):
- `// FUNCTION:` and `// LIBRARY:` + the definition (or the `// name` line
  of a lookup by name): function name and prototype;
- `// GLOBAL:` + the declaration: global name and type;
- the `typedef struct X { ... } X;` of src/include, src/common and
  src/<exe> that these types use: structure layout.
Compares with Ghidra and lists the differences. With --apply, writes them
through the only write endpoints allowed here (CLAUDE.md, "Writes"), in
this order:
1. import_data_types: the structures that are missing or whose layout
   (offset, size, type, name of each field) differs, with the structures
   they contain (importing an existing name replaces it in place);
2. create_function: the annotated functions that Ghidra's analysis missed
   (an import thunk keeps Ghidra's import name), then a wait for the
   auto-analysis they trigger;
3. rename_function, set_function_prototype;
4. apply_data_type, rename_symbol: when a type would clear data items
   that start inside it (type_would_evict), the items with generated
   names (INT_..., PTR_s_...) are first reset to undefined; an item named
   in src/ stops it.
So a fresh import of the three exes plus `--save` gives the reference
project (CLAUDE.md, "Counts").
With --save (implies --apply), waits for the auto-analysis that the writes
trigger to end, then saves the program: a save requested while the
analysis runs waits behind it and blocks the server
(notes/matching.md §3b).

Types that do not exist in Ghidra (Allegro's BITMAP, DATAFILE, FONT,
COLOR_MAP...) become `void *` in prototypes and structures; a global of
such a type keeps its Ghidra type and only gets its name. `fixed` becomes
`int`. A two-dimensional array `char x[N][NAME_SIZE]` gets a typedef NAME
(`char[NAME_SIZE]`), as `help_texts` (HELP_TEXT[16]).

Usage: python scripts/ghidra_sync.py [--apply | --save] [happy] [maped] [loader]
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
          "apply_data_type", "import_data_types", "create_function"}
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


# --- structures -------------------------------------------------------------

STRUCT_DEF = re.compile(r"typedef struct (\w+) \{(.*?)\} \1;", re.S)
DEFINE = re.compile(r"^#define (\w+) (0x[0-9a-fA-F]+|\d+)\s*$", re.M)
FIELD = re.compile(r"^((?:\w+\s+)*\w+(?:\s*\*+\s*|\s+))(\w+)((?:\[[^\]]*\])*)$")
# C type -> (size, Ghidra name); a pointer is 4 bytes
BASIC = {"char": (1, "char"), "signed char": (1, "schar"), "unsigned char": (1, "uchar"),
         "short": (2, "short"), "unsigned short": (2, "ushort"),
         "int": (4, "int"), "unsigned int": (4, "uint"), "long": (4, "long"),
         "unsigned long": (4, "ulong"), "fixed": (4, "int")}


def type_sources(exe):
    """({name: [(ctype, field, [dims])]}, {macro: value}) from the sources
    of exe: src/include, src/common and src/<exe>."""
    structs, macros = {}, {}
    # loader compiles src/loader only (scripts/build.py)
    for d in ((exe,) if exe == "loader" else ("include", "common", exe)):
        for f in sorted((SRC / d).glob("*.[ch]")):
            text = f.read_text(encoding="utf-8", errors="replace")
            macros.update({m.group(1): int(m.group(2), 0) for m in DEFINE.finditer(text)})
            for m in STRUCT_DEF.finditer(text):
                body = re.sub(r"/\*.*?\*/", "", m.group(2), flags=re.S)
                fields = []
                for decl in body.split(";"):
                    decl = " ".join(decl.split())
                    if not decl:
                        continue
                    fm = FIELD.match(decl)
                    fields.append((" ".join(fm.group(1).replace("*", " * ").split()),
                                   fm.group(2), re.findall(r"\[([^\]]*)\]", fm.group(3))))
                structs[m.group(1)] = fields
    return structs, macros


def dim(d, macros):
    return int(macros.get(d, d), 0)


def layout(name, structs, macros):
    """[(offset, size, ghidra_type, field)], size, alignment of a structure
    of src/ (MSVC default packing)."""
    fields, off, align = [], 0, 1
    for ctype, fname, dims in structs[name]:
        base = ctype.replace(" *", "").replace("*", "").strip()
        if "*" in ctype:
            size, al = 4, 4
            gt = (BASIC[base][1] if base in BASIC else base if base in structs else "void") \
                + " " + "*" * ctype.count("*")
        elif base in structs:
            _, size, al = layout(base, structs, macros)
            gt = base
        else:
            size, gt = BASIC[base]
            al = size
        n = 1
        for d in dims:
            n *= dim(d, macros)
        if dims:
            gt += "".join("[%d]" % dim(d, macros) for d in dims)
        off = (off + al - 1) // al * al
        fields.append((off, size * n, gt, fname))
        off += size * n
        align = max(align, al)
    return fields, (off + align - 1) // align * align, align


def struct_text(name, structs, macros):
    """C text for import_data_types: the structure, after the structures it
    contains; Ghidra's CParser knows neither fixed nor Allegro's types."""
    out = []
    for ctype, fname, dims in structs[name]:
        base = ctype.replace(" *", "").replace("*", "").strip()
        if base not in BASIC and base not in structs:
            ctype = "void " + "*" * ctype.count("*")
        ctype = re.sub(r"\bfixed\b", "int", ctype)
        out.append("    %s %s%s;" % (ctype, fname, "".join("[%d]" % dim(d, macros) for d in dims)))
    return "typedef struct %s {\n%s\n} %s;\n" % (name, "\n".join(out), name)


def contained(name, structs, seen=None):
    """name and the structures it uses, innermost first."""
    seen = [] if seen is None else seen
    for ctype, _, _ in structs[name]:
        base = ctype.replace("*", "").strip()
        if base in structs and base not in seen and base != name:
            contained(base, structs, seen)
    if name not in seen:
        seen.append(name)
    return seen


def norm_field_type(t):
    # Ghidra writes "TILE *" and "void *"; char is signed on x86
    return t.replace(" ", "").replace("schar", "char")


def struct_changes(exe, program, anns):
    """[(name, reason)] for the structures used by the annotations of exe
    that Ghidra lacks or has with another layout, innermost first."""
    structs, macros = type_sources(exe)
    used = []
    for _, _, _, text in anns:
        for w in re.findall(r"\w+", text or ""):
            if w in structs:
                for n in contained(w, structs):
                    if n not in used:
                        used.append(n)
    out = []
    for n in used:
        want, size, _ = layout(n, structs, macros)
        r = http("get_struct_layout", program=program, struct_name=n)
        if "fields" not in r:
            out.append((n, "missing"))
            continue
        got = [(f["offset"], f["size"], f["type"], f["name"]) for f in r["fields"]]
        if r["size"] != size or [(o, s, norm_field_type(t), f) for o, s, t, f in got] != \
                [(o, s, norm_field_type(t), f) for o, s, t, f in want]:
            diff = [w for w, g in zip(want, got) if (w[0], w[1], norm_field_type(w[2]), w[3])
                    != (g[0], g[1], norm_field_type(g[2]), g[3])]
            out.append((n, "layout differs (size %d / %d%s)" % (
                r["size"], size, ", first field %s" % (diff[0],) if diff else "")))
    return out, structs, macros


def typedef_2d(text, macros):
    """`char x[N][NAME_SIZE]` -> ("NAME", "typedef char NAME[size];", "NAME[N]")."""
    d = VAR_DECL.match(text)
    dims = re.findall(r"\[([^\]]*)\]", d.group(3))
    if len(dims) == 2 and d.group(1).strip() == "char" and dims[1].endswith("_SIZE") \
            and dims[1] in macros:
        name = dims[1][:-len("_SIZE")]
        return name, "typedef char %s[%d];\n" % (name, macros[dims[1]]), "%s[%d]" % (name, dim(dims[0], macros))
    return None


def wait_analysis(program, minutes):
    for _ in range(minutes * 12):
        if not http("analysis_status", program=program)["analyzing"]:
            return True
        time.sleep(5)
    return False


def apply_type(program, a, t, src_names):
    """apply_data_type; on type_would_evict, resets to undefined the data
    items with generated names that start inside the type, then retries."""
    r = http("apply_data_type", method="POST", program=program, address=a, type_name=t,
             strict_mode="off")
    if r.get("error") != "type_would_evict":
        return r
    names = [d["name"] for d in r.get("would_destroy", [])]
    if any(n in src_names for n in names):
        return {"error": "type_would_evict: %s named in src/" % ", ".join(n for n in names if n in src_names)}
    for d in r["would_destroy"]:
        u = "undefined%d" % d["length"] if d["length"] in (1, 2, 4, 8) else "undefined1"
        print("    reset %s %s (%s) -> %s" % (d["address"], d["name"], d["type"], u))
        http("apply_data_type", method="POST", program=program, address="0x" + d["address"],
             type_name=u, strict_mode="off")
    return http("apply_data_type", method="POST", program=program, address=a, type_name=t,
                strict_mode="off")


def sync(exe, apply):
    program = exe + ".exe"
    target = EXES[exe]
    anns = annotations(target)
    src_names = {a[2] for a in anns if a[0] == "GLOBAL"}
    changed = False

    # 1. structures (and the typedefs of two-dimensional arrays)
    types, structs, macros = struct_changes(exe, program, anns)
    known = set()
    for kind in ("struct", "typedef", "union", "enum"):
        r = http("find_data_types", program=program, kind=kind, limit=0)
        known |= {d["name"] for d in r["data_types"]}
    imports = [(n, why, struct_text(n, structs, macros)) for n, why in types]
    for kind, addr, name, text in anns:
        if kind == "GLOBAL" and text:
            td = typedef_2d(text, macros)
            if td and td[0] not in known and td[0] not in [i[0] for i in imports]:
                imports.append((td[0], "missing typedef", td[1]))
    print("== %s: %d annotations, %d types to import" % (program, len(anns), len(imports)))
    for n, why, text in imports:
        print("  %-22s %s: %s" % ("import_data_types", n, why))
        if apply:
            r = http("import_data_types", method="POST", program=program, source=text)
            if r.get("error"):
                print("    ERROR: %s" % r["error"])
        known.add(n)
    changed |= bool(imports)

    # 2. functions that the analysis missed
    funcs = [a for a in anns if a[0] in ("FUNCTION", "LIBRARY")]

    def current_functions():
        cur = {}
        for k in range(0, len(funcs), 20):
            keys = ["0x%08x" % a[1] for a in funcs[k:k + 20]]
            r = http("get_functions", program=program, functions=",".join(keys),
                     fields="signature", include_call_context="false")
            for key in keys:
                f = r["functions"].get(key)
                if f and "name" in f:
                    cur[int(key, 16)] = (f["name"], f.get("signature", ""))
        return cur

    current = current_functions()
    created = []
    for kind, addr, name, text in funcs:
        if addr in current:
            continue
        a = "0x%08x" % addr
        # a decorated name (_d_check_proc) is an import thunk: Ghidra names
        # it after the import
        print("  %-22s %s %s" % ("create_function", a, name))
        if apply:
            r = http("create_function", method="POST", program=program, address=a,
                     name="" if name.startswith("_") else name)
            if not r.get("success"):
                print("    ERROR: %s" % r.get("error", r))
            else:
                print("    %s, %d bytes" % (r.get("function_name"), r.get("body_size", 0)))
        created.append(addr)
    changed |= bool(created)
    if apply and created:
        # the creations queue analysis tasks; prototypes written during
        # them can be refused ("Invalid signature")
        if not wait_analysis(program, 10):
            print("  (analysis still running after 10 minutes: run the script again)")
        current = current_functions()

    # 3. names and prototypes, 4. globals
    globs = {}
    for g in http("list_globals", program=program, limit=0)["globals"]:
        m = re.match(r"^(.*) @ ([0-9a-f]+) \[\w+\] \((.*)\)", g)
        if m:
            globs[int(m.group(2), 16)] = (m.group(1), m.group(3))
    todo = []
    for kind, addr, name, text in anns:
        a = "0x%08x" % addr
        if kind in ("FUNCTION", "LIBRARY"):
            if addr not in current:
                if not apply:
                    continue  # listed above, created by --apply
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
            if proto and norm_sig(proto.replace(" " + name + "(", " %s(" % gname)) != norm_sig(gsig)                     and norm_sig(proto) != norm_sig(gsig):
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
                td = typedef_2d(text, macros)
                if td:
                    t = td[2]
                elif t and len(dims) == 1 and dims[0].isdigit():
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
    print("== %s: %d changes" % (program, len(todo)))
    for tool, args, what in todo:
        print("  %-22s %s" % (tool, what))
        if apply:
            if tool == "apply_data_type":
                r = apply_type(program, args["address"], args["type_name"], src_names)
            else:
                r = http(tool, method="POST", program=program, **args)
            if r.get("status") != "success" and r.get("error"):
                print("    ERROR: %s" % r["error"])
    return changed or bool(todo)


def save(exe):
    program = exe + ".exe"
    if not wait_analysis(program, 10):
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
