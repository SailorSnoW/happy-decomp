"""Allegro inventory: PE header, Rich header, DLL exports, exe imports.

Read-only. Hand-written PE parsing (no dependency on pefile).
- DLL analysed: captures/alleg40_unpacked.dll (copy unpacked with upx -d).
- Exes analysed: game/happy.exe, game/maped.exe, game/loader.exe (read, never modified).

Writes:
- notes/data/alleg40-exports.csv: ordinal, RVA, name, type (code/data depending on the section).
- notes/data/alleg40-imports.csv: exe, ordinal, name resolved through the DLL's exports,
  IAT slot address, type in the DLL (code/data), address in the DLL.
  The exes import alleg40.dll by ordinal only.
Prints: PE headers, Rich header, strings containing a version or a compiler name.

Usage: python -I scripts/pe_allegro.py
"""
import csv
import pathlib
import re
import struct
import time

ROOT = pathlib.Path(__file__).resolve().parent.parent
DLL = ROOT / "captures" / "alleg40_unpacked.dll"
EXES = ["happy.exe", "maped.exe", "loader.exe"]
OUT_EXP = ROOT / "notes" / "data" / "alleg40-exports.csv"
OUT_IMP = ROOT / "notes" / "data" / "alleg40-imports.csv"


class PE:
    def __init__(self, path):
        self.path = path
        self.b = path.read_bytes()
        b = self.b
        self.e_lfanew = struct.unpack_from("<I", b, 0x3C)[0]
        o = self.e_lfanew + 4
        (self.machine, nsec, self.timestamp, _, _, opt_size,
         self.characteristics) = struct.unpack_from("<HHIIIHH", b, o)
        opt = o + 20
        self.magic, self.lmaj, self.lmin = struct.unpack_from("<HBB", b, opt)
        self.image_base = struct.unpack_from("<I", b, opt + 28)[0]
        self.os_ver = struct.unpack_from("<HH", b, opt + 40)
        self.subsys_ver = struct.unpack_from("<HH", b, opt + 48)
        ndd = struct.unpack_from("<I", b, opt + 92)[0]
        self.dirs = [struct.unpack_from("<II", b, opt + 96 + 8 * i)
                     for i in range(ndd)]
        self.sections = []
        s = opt + opt_size
        for i in range(nsec):
            name = b[s:s + 8].rstrip(b"\0").decode("latin-1")
            vsize, va, rsize, rptr = struct.unpack_from("<IIII", b, s + 8)
            chars = struct.unpack_from("<I", b, s + 36)[0]
            self.sections.append((name, va, vsize, rptr, rsize, chars))
            s += 40

    def section_of(self, rva):
        for sec in self.sections:
            if sec[1] <= rva < sec[1] + max(sec[2], sec[4]):
                return sec
        return None

    def off(self, rva):
        sec = self.section_of(rva)
        if sec is None:
            raise ValueError(hex(rva))
        return rva - sec[1] + sec[3]

    def cstr(self, rva):
        o = self.off(rva)
        return self.b[o:self.b.index(b"\0", o)].decode("latin-1")

    def rich(self):
        """Decode the Rich header (comp.id entries: (product, build, count))."""
        b = self.b
        end = b.find(b"Rich", 0, self.e_lfanew)
        if end < 0:
            return None
        key = struct.unpack_from("<I", b, end + 4)[0]
        start = None
        for o in range(0x80, end, 4):
            if struct.unpack_from("<I", b, o)[0] ^ key == 0x536E6144:  # "DanS"
                start = o
                break
        if start is None:
            return None
        ents = []
        for o in range(start + 16, end, 8):
            compid, cnt = struct.unpack_from("<II", b, o)
            compid ^= key
            cnt ^= key
            ents.append((compid >> 16, compid & 0xFFFF, cnt))
        return ents

    def exports(self):
        rva, size = self.dirs[0]
        if not rva:
            return []
        (_, _, _, _, _, base, nfunc, nnames, aof, aon,
         aoo) = struct.unpack_from("<IIHHIIIIIII", self.b, self.off(rva))
        funcs = [struct.unpack_from("<I", self.b, self.off(aof) + 4 * i)[0]
                 for i in range(nfunc)]
        names = {}
        for i in range(nnames):
            nrva = struct.unpack_from("<I", self.b, self.off(aon) + 4 * i)[0]
            idx = struct.unpack_from("<H", self.b, self.off(aoo) + 2 * i)[0]
            names[idx] = self.cstr(nrva)
        out = []
        for i, f in enumerate(funcs):
            if f == 0:
                continue
            fwd = rva <= f < rva + size
            out.append((base + i, f, names.get(i, ""),
                        self.cstr(f) if fwd else None))
        return out

    def imports(self):
        rva, _ = self.dirs[1]
        out = {}
        o = self.off(rva)
        while True:
            ilt, _, _, name_rva, iat = struct.unpack_from("<IIIII", self.b, o)
            if not (ilt or name_rva or iat):
                break
            dll = self.cstr(name_rva)
            lst = []
            t = ilt or iat
            i = 0
            while True:
                v = struct.unpack_from("<I", self.b, self.off(t) + 4 * i)[0]
                if v == 0:
                    break
                slot = self.image_base + iat + 4 * i
                if v & 0x80000000:
                    lst.append(("#%d" % (v & 0xFFFF), slot))
                else:
                    lst.append((self.cstr(v + 2), slot))
                i += 1
            out[dll] = lst
            o += 20
        return out


def describe(pe):
    print("== %s" % pe.path.relative_to(ROOT))
    print("  machine=0x%04X magic=0x%X linker=%d.%02d image_base=0x%08X"
          % (pe.machine, pe.magic, pe.lmaj, pe.lmin, pe.image_base))
    print("  TimeDateStamp=0x%08X (%s UTC)" % (
        pe.timestamp, time.strftime("%Y-%m-%d %H:%M:%S",
                                    time.gmtime(pe.timestamp))))
    print("  OS ver=%d.%d subsystem ver=%d.%d" % (pe.os_ver + pe.subsys_ver))
    for name, va, vs, rp, rs, ch in pe.sections:
        print("  section %-8s VA=0x%06X vsize=0x%06X raw=0x%06X rsize=0x%06X"
              " chars=0x%08X" % (name, va, vs, rp, rs, ch))
    r = pe.rich()
    if r is None:
        print("  Rich header: absent")
    else:
        print("  Rich header (product, build, count):")
        for prod, build, cnt in r:
            print("    prodid=%3d build=%5d count=%d" % (prod, build, cnt))


def interesting_strings(pe):
    pat = re.compile(rb"[\x20-\x7e]{5,}")
    keys = re.compile(r"4\.0|allegro|version|msvc|visual|microsoft|mingw|gcc"
                      r"|borland|watcom|19\d\d|20\d\d|jan|feb|mar|apr|may|jun"
                      r"|jul|aug|sep|oct|nov|dec|\$Id|build|wip", re.I)
    for m in pat.finditer(pe.b):
        s = m.group().decode("latin-1")
        if keys.search(s):
            print("  0x%06X %r" % (m.start(), s))
    # UTF-16 strings (resources)
    for m in re.finditer(rb"(?:[\x20-\x7e]\x00){5,}", pe.b):
        s = m.group().decode("utf-16-le")
        if keys.search(s):
            print("  0x%06X (utf16) %r" % (m.start(), s))


def main():
    dll = PE(DLL)
    describe(dll)
    exps = dll.exports()
    kind = {}
    by_ord = {}
    with open(OUT_EXP, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["ordinal", "rva", "name", "section", "type"])
        for ordn, rva, name, fwd in exps:
            sec = dll.section_of(rva)
            sname = sec[0] if sec else ""
            t = "forward:" + fwd if fwd else (
                "code" if sec and sec[5] & 0x20000000 else "data")
            kind[name] = t
            by_ord[ordn] = (name, t, dll.image_base + rva)
            w.writerow([ordn, "0x%08X" % (dll.image_base + rva), name,
                        sname, t])
    print("  exports: %d (code=%d, data=%d)" % (
        len(exps), sum(v == "code" for v in kind.values()),
        sum(v == "data" for v in kind.values())))
    print("== interesting strings of the DLL")
    interesting_strings(dll)

    rows = []
    for exe in EXES:
        pe = PE(ROOT / "game" / exe)
        describe(pe)
        imps = pe.imports()
        for d, lst in imps.items():
            print("  import %-14s %3d entries" % (d, len(lst)))
        al = next((v for k, v in imps.items() if k.lower() == "alleg40.dll"),
                  [])
        # The exes import by ordinal: resolve through the export table.
        for name, slot in al:
            if name.startswith("#"):
                ordn = int(name[1:])
                ename, t, addr = by_ord.get(ordn, ("?", "?", 0))
            else:
                ordn = next((o for o, v in by_ord.items() if v[0] == name), "")
                ename, t, addr = name, kind.get(name, "?"), 0
            rows.append([exe, ordn, ename, "0x%08X" % slot, t,
                         "0x%08X" % addr])
    rows.sort(key=lambda r: (EXES.index(r[0]), r[2]))
    with open(OUT_IMP, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["exe", "ordinal", "name", "iat_slot", "type_in_dll",
                    "dll_address"])
        w.writerows(rows)
    print("alleg40.dll imports written: %d rows" % len(rows))


if __name__ == "__main__":
    main()
