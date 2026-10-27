"""Names the CRT functions of an exe by byte identity with the objects of LIBC.LIB.

For each CRT or THUNK function of notes/data/functions-<exe>.csv, looks for a
function of the local LIBC.LIB (VC6, identical to the game's:
notes/toolchain-vc6.md §4) whose bytes are identical, the bytes covered by a
relocation of the .obj being ignored. The name comes from the COFF symbol
table, decorated as in the PDB of a recompiled exe (`_free`), which is the form
reccmp's LIBRARY annotations need.

Ambiguities (the same code under several names) are resolved through the
relocations: a REL32 of the .obj to symbol S lands, in the exe, at an address
that must carry the name S.

Outputs:
- notes/data/crt-<exe>.csv: address, size, name, .obj member, status;
- src/crt/<exe>.h: LIBRARY annotations, read by reccmp, never compiled.

Usage: .venv/Scripts/python scripts/crt_names.py happy.exe [maped.exe loader.exe]
Only reads game/ and LIBC.LIB.
"""
import csv
import pathlib
import struct
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from inventory import Image, iat_map  # noqa: E402
from pe_allegro import PE  # noqa: E402
from vc6 import VC6, _child  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parent.parent
PADDING = (0x90, 0xCC)
# Relocations that patch 4 bytes: DIR32, DIR32NB, SECREL, REL32
RELOC_4 = (0x06, 0x07, 0x0B, 0x14)
# Public aliases at the same offset of one .obj, where the relocations of the
# CRT pick one name and cl emits calls to the other: reccmp names the call
# target with the name of the recompiled code, so the annotation takes it.
# chkstk.obj: LIBC calls __alloca_probe, cl calls __chkstk for a frame over
# 4 KB (loader main 0x00401005) [verified].
# chdir.obj: the game calls chdir, the OLDNAMES.LIB alias of _chdir, and the
# PDB names the target _chdir [verified: with chdir in loader main, the
# recompiled CRT order matches the original, notes/matching.md §3].
# xtoa.obj: happy calls itoa, the OLDNAMES.LIB alias of _itoa, and the PDB
# names the target _itoa [verified: draw_level 0x00402DBD, 0x00402E25].
COMPILER_ALIASES = {"__alloca_probe": "__chkstk", "__chdir": "_chdir",
                    "__itoa": "_itoa"}


def ar_members(data):
    """(name, bytes) of each object of an import/static library (ar format)."""
    assert data[:8] == b"!<arch>\n"
    p, longnames, out = 8, b"", []
    while p + 60 <= len(data):
        h = data[p:p + 60]
        name = h[:16].decode("latin-1").rstrip()
        size = int(h[48:58].decode().strip())
        body = data[p + 60:p + 60 + size]
        if name == "//":
            longnames = body
        elif name != "/":
            if name.startswith("/") and name[1:].isdigit():
                o = int(name[1:])
                name = longnames[o:longnames.index(b"\0", o)].decode("latin-1")
            out.append((name.rstrip("/"), body))
        p += 60 + size + (size & 1)
    return out


class Coff:
    def __init__(self, b):
        (self.machine, nsec, _, symptr, nsym, opt, _) = struct.unpack_from(
            "<HHIIIHH", b, 0)
        self.ok = self.machine == 0x14C
        if not self.ok:
            return
        strtab = symptr + 18 * nsym
        self.sections = []
        for i in range(nsec):
            o = 20 + opt + 40 * i
            (_, _, _, rsize, rptr, relptr, _, nrel, _, chars) = struct.unpack_from(
                "<8sIIIIIIHHI", b, o)
            raw = b[rptr:rptr + rsize] if rptr else b"\0" * rsize
            rel = [struct.unpack_from("<IIH", b, relptr + 10 * k)
                   for k in range(nrel)]
            self.sections.append({"raw": raw, "rel": rel, "chars": chars})
        self.syms = {}
        i = 0
        while i < nsym:
            o = symptr + 18 * i
            raw_name, value, secno, typ, cls, naux = struct.unpack_from(
                "<8sIhHBB", b, o)
            if raw_name[:4] == b"\0\0\0\0":
                k = strtab + struct.unpack_from("<I", raw_name, 4)[0]
                name = b[k:b.index(b"\0", k)].decode("latin-1")
            else:
                name = raw_name.rstrip(b"\0").decode("latin-1")
            self.syms[i] = (name, value, secno, typ, cls)
            i += 1 + naux

    def functions(self):
        """(name, external?, length up to the next symbol, bytes up to the end
        of the section, wildcard offsets, {offset: target symbol}) of each
        symbol defined in a code section."""
        by_sec = {}
        for name, value, secno, typ, cls in self.syms.values():
            if secno <= 0 or cls not in (2, 3, 6) or name.startswith((".", "$")):
                continue
            if not self.sections[secno - 1]["chars"] & 0x20:
                continue
            by_sec.setdefault(secno, []).append((value, name, cls == 2))
        for secno, lst in by_sec.items():
            sec = self.sections[secno - 1]
            lst.sort()
            offsets = sorted({v for v, _, _ in lst} | {len(sec["raw"])})
            for value, name, ext in lst:
                if value >= len(sec["raw"]):
                    continue
                end = offsets[offsets.index(value) + 1]
                wild, targets = set(), {}
                for va, si, typ in sec["rel"]:
                    if value <= va and typ in RELOC_4:
                        wild.update(range(va - value, va - value + 4))
                        if typ == 0x14:
                            targets[va - value] = self.syms[si][0]
                # rest of the section: an asm body may contain labels
                yield name, ext, end - value, sec["raw"][value:], wild, targets


def same(orig, size, body, wild):
    """Same code? The .obj body may end with alignment padding."""
    n = len(body)
    while n > size and body[n - 1] in PADDING:
        n -= 1
    if n != size:
        return False
    return all(i in wild or orig[i] == body[i] for i in range(n))


def run(exe):
    vc = VC6()
    libc = _child(vc.lib, "LIBC.LIB")
    objs = []
    for member, data in ar_members(libc.read_bytes()):
        c = Coff(data)
        if c.ok:
            objs += [(member,) + f for f in c.functions()]

    pe = PE(ROOT / "game" / exe)
    img = Image(pe)
    iat = iat_map(pe, exe)
    rows = list(csv.DictReader(open(
        ROOT / "notes" / "data" / ("functions-%s.csv" % exe[:-4]),
        encoding="utf-8")))
    funcs = [(int(r["address"], 16), int(r["size"]))
             for r in rows if r["class"] in ("CRT", "THUNK")]
    known = {int(r["address"], 16): {r["ghidra_name"],
                                     r["proposed_name"].split(" [")[0]}
             for r in rows}

    # jmp [IAT]: import thunk generated by the linker, named after the import
    thunks = {}
    for a, size in funcs:
        b = img.read(a, size)
        if size == 6 and b[:2] == b"\xFF\x25":
            thunks[a] = iat.get(struct.unpack_from("<I", b, 2)[0], ("", "?"))[1]

    cands, spans = {}, set()
    for a, size in funcs:
        orig = img.read(a, size)
        if a in thunks:
            cands[a] = []
            continue
        cands[a] = [(name, member, ext, targets, len(rest))
                    for member, name, ext, n, rest, wild, targets in objs
                    if n >= size and rest[:1] == orig[:1]
                    and same(orig, size, rest[:n], wild)]
        if not cands[a]:
            # body that runs past an internal label (asm), or whose chunks
            # are contiguous in the .obj
            cands[a] = [(name, member, ext, {o: t for o, t in targets.items()
                                             if o < size}, len(rest))
                        for member, name, ext, n, rest, wild, targets in objs
                        if n < size <= len(rest) and rest[:1] == orig[:1]
                        and same(orig, size, rest[:size], wild)]
            if cands[a]:
                spans.add(a)

    # A REL32 at offset o of the .obj gives, in the exe, the target address
    # a + o + 4 + rel32: its name must be the symbol of the relocation.
    # Both directions: a candidate whose REL32 lands on a function that cannot
    # carry the symbol is dropped; a target with several names keeps the one
    # its callers require.
    names = {a: {c[0] for c in cs} for a, cs in cands.items()}

    def target(a, o):
        return (a + o + 4 + struct.unpack("<i", img.read(a + o, 4))[0]) & 0xFFFFFFFF

    for _ in range(4):
        for a, cs in cands.items():
            if len(cs) > 1:
                keep = [c for c in cs if all(
                    sym in names[target(a, o)] for o, sym in c[3].items()
                    if names.get(target(a, o)))]
                if keep:
                    cands[a] = keep
                    names[a] = {c[0] for c in keep}
        for a, cs in cands.items():
            if len(names[a]) == 1:
                for name, member, ext, targets, _ in cs:
                    for o, sym in targets.items():
                        t = target(a, o)
                        if len(names.get(t, ())) > 1 and sym in names[t]:
                            names[t] = {sym}
        for a in cands:
            if len(names[a]) > 1:
                cands[a] = [c for c in cands[a] if c[0] in names[a]]

    out = []
    for a, size in funcs:
        ns = sorted(names[a])
        members = sorted({c[1] for c in cands[a] if c[0] in ns})
        ext = any(c[2] for c in cands[a] if c[0] in ns)
        # several public names at the same offset of the same .obj: aliases
        # of one function; keep the one Ghidra or the inventory gives, if any
        alias = len(ns) > 1 and len({(c[1], c[4]) for c in cands[a]}) == 1
        if alias:
            pick = [n for n in ns if n in known[a] or n[1:] in known[a]] or ns
            ns = [pick[0]] + [n for n in ns if n != pick[0]]
        status = ("import thunk" if a in thunks else
                  "verified (aliases)" if alias else
                  "verified (spans labels)" if len(ns) == 1 and a in spans else
                  "verified" if len(ns) == 1 else
                  "ambiguous" if ns else "not found")
        out.append({"address": "0x%08X" % a, "size": size,
                    "name": COMPILER_ALIASES.get(ns[0], ns[0])
                    if len(ns) == 1 or alias else "",
                    "candidates": (thunks[a] if a in thunks else
                                   " ".join(ns[1:]) if alias else
                                   " ".join(ns) if len(ns) > 1 else ""),
                    "member": " ".join(members), "external": int(ext),
                    "status": status})

    with open(ROOT / "notes" / "data" / ("crt-%s.csv" % exe[:-4]), "w",
              newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(out[0]))
        w.writeheader()
        w.writerows(out)

    target = exe[:-4].upper()
    lines = ["/* Generated by scripts/crt_names.py from LIBC.LIB: do not edit.",
             " * CRT functions of %s, named by byte identity with the objects" % exe,
             " * of LIBC.LIB (relocations ignored). Read by reccmp, never compiled. */",
             "#if 0", ""]
    for r in out:
        if r["name"]:
            lines += ["// LIBRARY: %s %s" % (target, r["address"].lower()),
                      "// %s" % r["name"], ""]
    lines.append("#endif")
    dst = ROOT / "src" / "crt" / (exe[:-4] + ".h")
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_text("\n".join(lines) + "\n", encoding="ascii")

    def a_spans(r):
        return r["status"].startswith("verified (")

    count = {}
    for r in out:
        count[r["status"]] = count.get(r["status"], 0) + 1
    print("%s: %d CRT/THUNK functions, %s" % (exe, len(out), ", ".join(
        "%s %d" % kv for kv in sorted(count.items()))))
    for r in out:
        if r["status"] not in ("verified", "import thunk") or a_spans(r):
            print("  %s %5d %-9s %s" % (r["address"], r["size"], r["status"],
                                        r["candidates"]))


if __name__ == "__main__":
    for e in sys.argv[1:] or ["happy.exe", "maped.exe", "loader.exe"]:
        run(e)
