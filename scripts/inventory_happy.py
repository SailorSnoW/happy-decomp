"""Function inventory of happy.exe (read-only).

Inputs:
- game/happy.exe (bytes, read without modification);
- captures/ghidra-cache/happy_exe.json (produced by ghidra_dump.py): entry
  points, names (including Function ID), function bodies according to Ghidra;
- notes/data/alleg40-imports.csv: names of the alleg40.dll imports (by ordinal).

Method:
1. Entry points = Ghidra functions + added entries: code never reached by the
   flow of a known function but called, jumped to or referenced (immediate or
   pointer in .data/.rdata). Ghidra does not create these functions
   (e.g. 0x0040A830, passed as an argument to _WinMain).
2. Contiguous model (VC6 does not split functions): a function runs from its
   entry to the next entry; the trailing padding (0x90/0xCC) is excluded.
3. Recursive disassembly with capstone from the entry, the targets of jump
   tables (jmp [reg*4+T]) and the SEH filters/handlers (scope tables pushed
   before _except_handler3). Everything else in the extent is counted as
   data (tables) or padding.
4. References: direct call/jmp, IAT slots, immediates and displacements
   pointing into .text/.rdata/.data; strings = referenced address that starts
   a printable sequence terminated by 0.
5. GAME / CRT / THUNK / UNKNOWN classification with a [verified]/[hypothesis]
   status.

Outputs:
- notes/data/functions-happy.csv
- notes/data/datarefs-happy.csv (referenced .rdata/.data addresses)
- captures/inventory-happy.json (full graph, for the next steps)

Usage: .venv/Scripts/python scripts/inventory_happy.py
(dependency: capstone, installed in .venv)
"""
import bisect
import csv
import json
import pathlib
import re
import struct
import sys
from collections import defaultdict

import capstone
from capstone import x86

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from pe_allegro import PE  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parent.parent
EXE = ROOT / "game" / "happy.exe"
CACHE = ROOT / "captures" / "ghidra-cache" / "happy_exe.json"
ALLEG_IMP = ROOT / "notes" / "data" / "alleg40-imports.csv"
OUT_CSV = ROOT / "notes" / "data" / "functions-happy.csv"
OUT_DATA = ROOT / "notes" / "data" / "datarefs-happy.csv"
OUT_JSON = ROOT / "captures" / "inventory-happy.json"

# Game / CRT boundary: 0x0040AD50 is rand() (LCG 0x343FD / 0x269EC3, VC6 CRT).
# Everything before it is 16-byte aligned with 0x90 padding; the CRT is not
# (except the asm routines). See notes/layout-happy.md.
GAME_END = 0x0040AD50
WINMAIN = 0x0040A970        # END_OF_MAIN() stub (called by the PE entry point)
MANGLED_MAIN = 0x0040A830   # pushed by the stub and passed to _WinMain
EXCEPT_HANDLER3 = 0x0040DF44

# CRT functions identified by hand for the evidence in the note
# (all [hypothesis]; justification in the "justification" column).
CRT_GUESSES = {
    0x0040AD50: ("rand", "LCG holdrand*0x343FD+0x269EC3, >>16 & 0x7FFF"),
    0x0040B31E: ("atexit", "passed to install_allegro (allegro_init); "
                 "returns (_onexit(f) == NULL) ? -1 : 0"),
    0x0040B330: ("__onexitinit", "malloc(0x80), R6024 (_amsg_exit(0x18)) on failure"),
    0x0040B0F6: ("__initstdio", "allocates 20 FILE* (0x14), _amsg_exit(0x1A) on failure"),
    0x0040B19B: ("__endstdio", "calls _flushall then _fcloseall"),
    0x0040B080: ("_flushall", "flsall(1)"),
    0x0040C45B: ("_fcloseall", "loops over the streams from 3 on, fclose"),
    0x0040E7E0: ("__aulldiv", "unsigned 64-bit division, ret 0x10"),
    0x0040E850: ("__aullrem", "unsigned 64-bit remainder, ret 0x10"),
    0x0040DE6C: ("__unwind_handler", "SEH handler referenced by "
                 "_global_unwind2 / _abnormal_termination"),
    0x0040DF44: ("__except_handler3", "preceded by the signature \"VC20XC00\", "
                 "pushed by the entry point before FS:[0]"),
    0x0040B5FC: ("__amsg_exit", "called with the R60xx codes (0x1C, 0x18, 0x1A...)"),
    0x0040CCA2: ("__heap_init", "1st call of the entry point, failure → _amsg_exit(0x1C)=R6028"),
    0x0040C233: ("__ioinit", "2nd call of the entry point"),
    0x0040DD1A: ("___crtGetEnvironmentStringsA", "result stored right after GetCommandLineA"),
    0x0040DACD: ("__setargv", "called after GetCommandLineA"),
    0x0040DA14: ("__setenvp", "called after _setargv"),
    0x0040B1AF: ("__cinit", "called before GetStartupInfoA"),
    0x0040D9BC: ("__wincmdln", "result passed as lpCmdLine to WinMain"),
    0x0040B1DC: ("exit", "receives WinMain's return value; calls "
                 "0x0040B1FE(code, 0, 0), whereas __exit (Function ID, "
                 "0x0040B1ED) calls 0x0040B1FE(code, 1, 0)"),
    0x0040B1FE: ("doexit", "called by exit and __exit with 3 arguments; "
                 "calls ExitProcess, GetCurrentProcess, TerminateProcess"),
    0x0040D838: ("__XcptFilter", "SEH filter of the entry point"),
    0x0040AD6E: ("fclose", "fclose(f) after fopen/fread in 0x0040AB30; "
                 "tests _flag & 0x83, flushes the buffer, closes the handle"),
    0x0040AEEE: ("fopen", "fopen(name, \"rb\"/\"wb\"): 0x0040AB30, 0x00401DA0"),
    0x0040AF01: ("fread", "fread(buf, 0x85, 1, f) at 0x0040AB69"),
    0x0040ADC4: ("fwrite", "fwrite(p, size, 1, f) between fopen(\"happy.sav\", "
                 "\"wb\") and fclose (0x00401DBC...)"),
    0x0040B463: ("free", "frees the 0x8C-byte block if the 2nd malloc "
                 "fails (0x0040AA6A)"),
    0x0040B3D7: ("fseek", "(f, 0x85 + 5*W*H, 0) at 0x004030A0; rejects an "
                 "origin other than 0, 1, 2"),
    0x00410CAC: ("_itoa", "(value, buffer, 10) at 0x00402DBD"),
    0x0040B621: ("fast_error_exit", "called by the entry point with 0x1C if "
                 "_heap_init fails; ends with ExitProcess(0xFF)"),
    0x0040B35F: ("__fpmath", "pointed to by 0x00413220 (_FPinit); fills "
                 "_cfltcvt_tab (0x00413250...) with 0x0040C6CF, 0x0040C71D..."),
    0x0040F525: ("__fptrap", "_amsg_exit(2) = R6002 \"floating point not "
                 "loaded\"; initial value of the 6 entries of 0x00413250"),
    0x0040DEF6: ("__abnormal_termination", "compares the current SEH handler "
                 "(FS:[0]+4) with 0x0040DE6C; unreferenced"),
    0x0040DF19: ("__NLG_Notify1", "second entry point of 0x0040DF22 (same "
                 "code, jumps to 0x0040DF2C); unreferenced"),
    0x0040E001: ("__seh_longjmp_unwind@4", "calls __local_unwind2 with "
                 "[ecx+0x18], [ecx+0x1C], ret 4; unreferenced"),
}

PAD = (0x90, 0xCC)

# Layout blocks (see notes/layout-happy.md): (start, exclusive end, name).
BLOCKS = [
    (0x00401000, 0x004018B0, "block_01"),
    (0x004018B0, 0x004021A0, "block_02"),
    (0x004021A0, 0x00403030, "block_03"),
    (0x00403030, 0x00403450, "block_04"),
    (0x00403450, 0x004063A0, "block_05"),
    (0x004063A0, 0x00407500, "block_06"),
    (0x00407500, 0x00409870, "block_07"),
    (0x00409870, 0x0040A9A0, "block_08"),
    (0x0040A9A0, 0x0040AD50, "block_09"),
    (0x0040AD50, 0x00411000, "block_10_crt"),
]


def block_of(s):
    for b, e, n in BLOCKS:
        if b <= s < e:
            return n
    return ""


def load():
    pe = PE(EXE)
    cache = json.loads(CACHE.read_text(encoding="utf-8"))
    return pe, cache


class Image:
    def __init__(self, pe):
        self.pe = pe
        self.base = pe.image_base
        self.secs = {}
        for name, va, vs, rp, rs, ch in pe.sections:
            self.secs[name] = (self.base + va, self.base + va + vs, rp, rs)

    def sec(self, a):
        for name, (s, e, _, _) in self.secs.items():
            if s <= a < e:
                return name
        return None

    def read(self, a, n):
        name = self.sec(a)
        if name is None:
            return b"\0" * n
        s, e, rp, rs = self.secs[name]
        o = a - s
        raw = self.pe.b[rp + o: rp + min(o + n, rs)] if o < rs else b""
        return raw + b"\0" * (n - len(raw))

    def dword(self, a):
        return struct.unpack("<I", self.read(a, 4))[0]

    def string_at(self, a):
        """Printable C string (>= 2 chars) starting at a, otherwise None."""
        if self.sec(a) not in (".rdata", ".data"):
            return None
        bs = self.read(a, 512)
        n = bs.find(b"\0")
        if n < 2:
            return None
        s = bs[:n]
        if all(0x20 <= c < 0x7F or c in (9, 10, 13) for c in s):
            return s.decode("latin-1")
        return None

    def string_parent(self, a):
        """Start of the string containing a (a itself if a starts it)."""
        p = a
        while p > a - 512:
            c = self.read(p - 1, 1)[0]
            if not (0x20 <= c < 0x7F or c in (9, 10, 13)):
                break
            p -= 1
        return p


def parse_ghidra(cache):
    funcs = {}
    for a, d in cache["functions"].items():
        info = d["get_function_info"]
        name = re.search(r"Name: (.*)", info).group(1).strip()
        body = [(int(s, 16), int(e, 16)) for s, e in
                re.findall(r"\[([0-9a-f]{8}), ([0-9a-f]{8})\]", info)]
        callers = re.findall(r"- ([0-9a-f]{8}) \(", d["get_callers"])
        callees = re.findall(r"- (?:EXTERNAL:)?([0-9a-f]{8}) \(([^)]*)\)",
                             d["get_callees"])
        funcs[int(a, 16)] = {"name": name, "body": body,
                             "callers": callers, "callees": callees}
    return funcs


def iat_map(pe):
    names = {}
    with open(ALLEG_IMP, encoding="utf-8") as f:
        for r in csv.DictReader(f):
            if r["exe"] == "happy.exe":
                names[int(r["iat_slot"], 16)] = ("ALLEGRO", r["name"],
                                                 r["type_in_dll"])
    for dll, lst in pe.imports().items():
        for name, slot in lst:
            if slot in names:
                continue
            names[slot] = ("WIN32", "%s!%s" % (dll.upper(), name), "code")
    return names


class Disasm:
    def __init__(self, img):
        self.img = img
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.md.detail = True
        self.cache = {}

    def at(self, a):
        if a in self.cache:
            return self.cache[a]
        bs = self.img.read(a, 16)
        ins = next(self.md.disasm(bs, a), None)
        self.cache[a] = ins
        return ins


def values_of(ins):
    """(value, kind) for each plausible absolute immediate/displacement."""
    out = []
    for op in ins.operands:
        if op.type == x86.X86_OP_IMM:
            out.append((op.imm & 0xFFFFFFFF, "imm"))
        elif op.type == x86.X86_OP_MEM:
            if op.mem.base == 0 and op.mem.segment == 0:
                out.append((op.mem.disp & 0xFFFFFFFF,
                            "memidx" if op.mem.index else "mem"))
    return [(v, k) for v, k in out if 0x00400000 <= v < 0x00450000]


def decode_function(dis, img, start, end):
    """Recursive disassembly in [start, end). Returns instructions and tables."""
    seen = {}
    tables = []
    roots = [start]
    work = list(roots)
    while work:
        a = work.pop()
        while start <= a < end and a not in seen:
            ins = dis.at(a)
            if ins is None:
                break
            seen[a] = ins
            m = ins.mnemonic
            nxt = a + ins.size
            # jump table: jmp dword ptr [reg*4 + T]
            if m == "jmp" and ins.operands and \
                    ins.operands[0].type == x86.X86_OP_MEM and \
                    ins.operands[0].mem.index and ins.operands[0].mem.scale == 4:
                t = ins.operands[0].mem.disp & 0xFFFFFFFF
                if start <= t < end:
                    # the index may start at 1 (asm memcpy: [eax*4+T], eax>=1)
                    if not (start <= img.dword(t) < end):
                        t += 4
                    n = 0
                    while start <= t + 4 * n < end:
                        v = img.dword(t + 4 * n)
                        if not (start <= v < end):
                            break
                        work.append(v)
                        n += 1
                    tables.append((t, 4 * n))
                break
            # SEH: push scope_table; push _except_handler3
            if m == "push" and ins.operands and \
                    ins.operands[0].type == x86.X86_OP_IMM and \
                    (ins.operands[0].imm & 0xFFFFFFFF) == EXCEPT_HANDLER3:
                prev = [s for s in seen.values() if s.address + s.size == a]
                if prev and prev[0].mnemonic == "push" and \
                        prev[0].operands[0].type == x86.X86_OP_IMM:
                    st = prev[0].operands[0].imm & 0xFFFFFFFF
                    i = 0
                    while img.sec(st + 12 * i) == ".rdata":
                        enc, flt, hnd = struct.unpack(
                            "<III", img.read(st + 12 * i, 12))
                        ok = False
                        for v in (flt, hnd):
                            if start <= v < end:
                                work.append(v)
                                ok = True
                        if not ok or enc not in (0xFFFFFFFF,) + tuple(range(i)):
                            break
                        i += 1
            if m in ("ret", "retf", "hlt", "int3"):
                break
            if m == "jmp":
                op = ins.operands[0]
                if op.type == x86.X86_OP_IMM:
                    work.append(op.imm & 0xFFFFFFFF)
                break
            if m.startswith("j") or m.startswith("loop"):
                op = ins.operands[0]
                if op.type == x86.X86_OP_IMM:
                    work.append(op.imm & 0xFFFFFFFF)
            a = nxt
    return seen, tables


def build(pe, cache):
    img = Image(pe)
    dis = Disasm(img)
    gh = parse_ghidra(cache)
    iat = iat_map(pe)
    text_s, text_e = img.secs[".text"][0], img.secs[".text"][0] + \
        pe.sections[0][2]
    in_ghidra_body = []
    for a, f in gh.items():
        for s, e in f["body"]:
            in_ghidra_body.append((s, e))
    in_ghidra_body.sort()

    def inside_ghidra(v):
        i = bisect.bisect_right(in_ghidra_body, (v, 0xFFFFFFFF)) - 1
        return i >= 0 and in_ghidra_body[i][0] <= v <= in_ghidra_body[i][1]

    entries = set(gh)
    added = {}
    for _ in range(30):
        ent = sorted(entries)
        decoded = {}
        for i, s in enumerate(ent):
            e = ent[i + 1] if i + 1 < len(ent) else text_e
            decoded[s] = decode_function(dis, img, s, e)
        reached = set()
        for s, (ins, _) in decoded.items():
            reached.update(ins)
        cands = defaultdict(set)
        for s, (ins, _) in decoded.items():
            for a, x in ins.items():
                for v, k in values_of(x):
                    # memidx = base of a jump table, not a function
                    if text_s <= v < text_e and k != "memidx":
                        cands[v].add("%s@%08X" % (x.mnemonic, a))
        # pointers in .rdata / .data
        for sec in (".rdata", ".data"):
            s0, e0, _, rs = img.secs[sec]
            for a in range(s0, min(s0 + rs, e0) - 3, 4):
                v = img.dword(a)
                if text_s <= v < text_e:
                    cands[v].add("ptr@%08X" % a)
        new = 0
        for v, why in cands.items():
            if v in entries:
                continue
            is_call = any(w.startswith("call@") for w in why)
            if not is_call and (v in reached or inside_ghidra(v)):
                continue
            entries.add(v)
            added[v] = sorted(why)
            new += 1
        if new:
            continue
        # Code never reached nor referenced, placed after a ret/jmp or
        # padding, which decodes up to a ret/jmp: dead (unreferenced)
        # function kept by the linker.
        for i, s in enumerate(ent):
            e = ent[i + 1] if i + 1 < len(ent) else text_e
            ins, tables = decoded[s]
            cov = bytearray(e - s)
            for a, x in ins.items():
                cov[a - s:a - s + x.size] = b"\1" * x.size
            for t, n in tables:
                cov[t - s:t - s + n] = b"\1" * n
            p = 0
            while p < len(cov):
                if cov[p]:
                    p += 1
                    continue
                q = p
                while q < len(cov) and not cov[q]:
                    q += 1
                a = s + p
                while a < s + q and img.read(a, 1)[0] in PAD:
                    a += 1
                ok = False
                b = a
                while b < s + q:
                    x = dis.at(b)
                    if x is None or b + x.size > s + q:
                        break
                    if x.mnemonic in ("ret", "jmp"):
                        ok = True
                        break
                    b += x.size
                if ok and a not in entries:
                    entries.add(a)
                    added[a] = ["unreferenced (dead code after 0x%08X)"
                                % (s + p - 1)]
                    new += 1
                p = q
        if not new:
            break
    ent = sorted(entries)
    # Final reason for the addition: references found on the final graph.
    for v in added:
        why = sorted(cands.get(v, ()))
        if why:
            added[v] = why
    return img, dis, gh, iat, ent, decoded, added, text_e


def analyse():
    pe, cache = load()
    img, dis, gh, iat, ent, decoded, added, text_e = build(pe, cache)
    F = {}
    for i, s in enumerate(ent):
        e = ent[i + 1] if i + 1 < len(ent) else text_e
        bs = img.read(s, e - s)
        n = len(bs)
        while n > 0 and bs[n - 1] in PAD:
            n -= 1
        ins, tables = decoded[s]
        F[s] = {"start": s, "extent_end": e, "size": n, "pad": len(bs) - n,
                "pad_bytes": bs[n:].hex(), "ins": ins, "tables": tables,
                "ghidra": gh.get(s), "added_by": added.get(s)}

    def owner(v):
        i = bisect.bisect_right(ent, v) - 1
        return ent[i] if i >= 0 else None

    datarefs = defaultdict(lambda: {"funcs": set(), "kinds": set()})
    for s, f in F.items():
        calls, tail, fptr = set(), set(), set()
        imp = defaultdict(set)
        strings, data = {}, set()
        nret = 0
        for a, x in sorted(f["ins"].items()):
            m = x.mnemonic
            if m == "ret":
                nret += 1
            for v, k in values_of(x):
                sec = img.sec(v)
                if v in iat:
                    imp[iat[v]].add("call" if m in ("call", "jmp") else "use")
                elif sec == ".text":
                    o = owner(v)
                    if m == "call" and k == "imm":
                        calls.add(o)
                    elif m.startswith("j") and k == "imm":
                        if not (s <= v < f["extent_end"]):
                            tail.add(o)
                    elif v == o and o != s:
                        fptr.add(o)
                    elif v == o == s:
                        fptr.add(o)
                elif sec in (".rdata", ".data"):
                    st = img.string_at(v)
                    parent = img.string_parent(v)
                    if st is not None and k != "memidx" and parent != v:
                        # reference into the middle of a string (e.g. "Johan Peitz"+5)
                        strings[v] = "%s+%d" % (json.dumps(
                            img.string_at(parent), ensure_ascii=False), v - parent)
                        datarefs[v]["kinds"].add("string-interior")
                    elif st is not None and k != "memidx":
                        strings[v] = st
                        datarefs[v]["kinds"].add("string")
                    else:
                        data.add(v)
                        datarefs[v]["kinds"].add(k)
                    datarefs[v]["funcs"].add(s)
        f.update(calls=calls, tail=tail, fptr=fptr, imports=imp,
                 strings=strings, data=data, nret=nret)

    # Function pointers in .rdata/.data
    ptr_in_data = defaultdict(list)
    for sec in (".rdata", ".data"):
        s0, e0, _, rs = img.secs[sec]
        for a in range(s0, min(s0 + rs, e0) - 3, 4):
            v = img.dword(a)
            if v in F:
                ptr_in_data[v].append(a)

    callers = defaultdict(set)
    refs = defaultdict(set)
    for s, f in F.items():
        for c in f["calls"] | f["tail"]:
            callers[c].add(s)
        for c in f["fptr"]:
            refs[c].add(s)

    opt = pe.e_lfanew + 24
    entry = pe.image_base + struct.unpack_from("<I", pe.b, opt + 16)[0]

    def reach(roots, stop=()):
        seen, work = set(), list(roots)
        while work:
            x = work.pop()
            if x in seen or x in stop:
                continue
            seen.add(x)
            f = F[x]
            work.extend(f["calls"] | f["tail"] | f["fptr"])
            if x == WINMAIN:
                work.append(MANGLED_MAIN)
        return seen

    from_game = reach([WINMAIN])
    from_start = reach([entry], stop=(WINMAIN,))
    # pointers in data: initialisation tables (__xi/__xc/__xp/__xt),
    # _cfltcvt_tab, etc. They are treated as "data" roots.
    data_roots = [v for v in ptr_in_data]
    from_data = reach(data_roots, stop=(WINMAIN,))
    from_data_game = reach([v for v in data_roots if v < GAME_END])

    return (pe, img, F, ent, callers, refs, ptr_in_data, datarefs, entry,
            from_game, from_start, from_data, from_data_game, iat)


def classify(F, entry, from_game, from_start, from_data, ptr_in_data, iat):
    for s, f in F.items():
        g = f["ghidra"]
        gname = g["name"] if g else ""
        ins = sorted(f["ins"].items())
        fid = bool(gname) and not gname.startswith("FUN_")
        reasons = []
        sub = ""
        if len(ins) == 1 and ins[0][1].mnemonic == "jmp":
            cls, st = "THUNK", "verified"
            reasons.append("single instruction: %s %s"
                           % (ins[0][1].mnemonic, ins[0][1].op_str))
        elif s == entry:
            cls, st = "CRT", "verified"
            reasons.append("PE entry point (AddressOfEntryPoint): "
                           "WinMainCRTStartup")
        elif fid:
            cls, st = "CRT", "verified"
            reasons.append("named by Ghidra (Function ID): %s" % gname)
        elif s < GAME_END:
            if s in from_game:
                cls, st = "GAME", "hypothesis"
                reasons.append("range 0x00401000-0x0040AD4F (16-byte aligned, "
                               "0x90 padding), reachable from WinMain")
            else:
                cls, st = "UNKNOWN", "hypothesis"
                reasons.append("game range but not reachable from WinMain")
            if s == WINMAIN:
                sub = "allegro-header"
                reasons.append("Allegro END_OF_MAIN() stub (alwin.h)")
            if s == 0x00402110:
                sub = "allegro-header"
                reasons.append("_set_errno_erange() from al386vc.h")
            if s == MANGLED_MAIN:
                reasons.append("_mangled_main: the game's main() passed to _WinMain")
        else:
            st = "hypothesis"
            if s in from_start and s not in from_game:
                cls = "CRT"
                reasons.append("reachable only from the PE entry point "
                               "(not through WinMain)")
            elif s in from_data and s not in from_game:
                cls = "CRT"
                reasons.append("reachable only through data pointers "
                               "(CRT init/term tables)")
            elif s in from_game:
                cls = "CRT"
                reasons.append("after 0x0040AD50 (CRT range, unaligned), "
                               "reachable from WinMain")
            elif s in CRT_GUESSES:
                cls = "CRT"
                reasons.append("after 0x0040AD50, no reference found, "
                               "recognised CRT pattern")
            else:
                cls = "UNKNOWN"
                reasons.append("after 0x0040AD50, no reference found")
            if s in CRT_GUESSES:
                reasons.append("identified as %s: %s" % CRT_GUESSES[s])
        if f.get("also_in"):
            reasons.append("identical code (addresses masked) in " +
                           ", ".join(f["also_in"]))
        f["class"], f["status"], f["sub"], f["why"] = cls, st, sub, reasons


def masked_stream(img, start, size):
    """Instructions of [start, start+size) with rel32 and absolute addresses
    replaced by zeros (comparison insensitive to linking)."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    md.skipdata = True
    out = []
    for i in md.disasm(img.read(start, size), start):
        b = bytearray(i.bytes)
        if i.mnemonic in ("call", "jmp") and b[0] in (0xE8, 0xE9):
            b[1:] = b"\0" * (len(b) - 1)
        elif i.id != 0:
            vals = [op.imm & 0xFFFFFFFF for op in i.operands
                    if op.type == x86.X86_OP_IMM]
            vals += [op.mem.disp & 0xFFFFFFFF for op in i.operands
                     if op.type == x86.X86_OP_MEM]
            # the other exes have data reaching beyond 0x00450000
            for v in [v for v in vals if 0x00400000 <= v < 0x00500000]:
                j = bytes(b).find(struct.pack("<I", v))
                if j >= 0:
                    b[j:j + 4] = b"\0" * 4
        out.append((i.address, bytes(b)))
    return out


def cross_binary(img, F, ent):
    """happy.exe functions found identical in maped/loader."""
    mine = {s: b"".join(b for _, b in masked_stream(img, s, F[s]["size"]))
            for s in ent if F[s]["size"] >= 16}
    for other in ("maped.exe", "loader.exe"):
        oi = Image(PE(ROOT / "game" / other))
        s0, e0, _, _ = oi.secs[".text"]
        st = masked_stream(oi, s0, e0 - s0)
        cat = b"".join(b for _, b in st)
        offs, p = [], 0
        for a, b in st:
            offs.append(p)
            p += len(b)
        for s, sig in mine.items():
            k = cat.find(sig)
            if k >= 0:
                i = bisect.bisect_right(offs, k) - 1
                if offs[i] == k:
                    F[s].setdefault("also_in", []).append(
                        "%s@0x%08X" % (other, st[i][0]))


def fname(F, s):
    g = F[s]["ghidra"]
    if g and not g["name"].startswith("FUN_"):
        return g["name"]
    if s in CRT_GUESSES:
        return "%s?" % CRT_GUESSES[s][0]
    return "FUN_%08x" % s


def main():
    (pe, img, F, ent, callers, refs, ptr_in_data, datarefs, entry,
     from_game, from_start, from_data, from_data_game, iat) = analyse()
    cross_binary(img, F, ent)
    classify(F, entry, from_game, from_start, from_data, ptr_in_data, iat)

    rows = []
    for s in ent:
        f = F[s]
        g = f["ghidra"]
        callees = f["calls"] | f["tail"]
        al = sorted(n for (k, n, t), u in f["imports"].items()
                    if k == "ALLEGRO")
        w32 = sorted(n.split("!")[1] for (k, n, t), u in f["imports"].items()
                     if k == "WIN32")
        crt = sorted(fname(F, c) for c in callees if F[c]["class"] == "CRT")
        strs = [v if "string-interior" in datarefs[k]["kinds"] else
                json.dumps(v, ensure_ascii=False)
                for k, v in sorted(f["strings"].items())]
        gb = g["body"] if g else []
        gsize = sum(e - b + 1 for b, e in gb)
        notes = []
        if not g and f["added_by"][0].startswith("unreferenced"):
            notes.append("missing from Ghidra; " + f["added_by"][0])
        elif not g:
            notes.append("missing from Ghidra; referenced by " +
                         ", ".join(f["added_by"][:4]))
        elif gb and gb[-1][1] + 1 < s + f["size"]:
            notes.append("Ghidra body shorter (%d bytes)" % gsize)
        if f["tables"]:
            notes.append("jump tables: " + ", ".join(
                "0x%08X(%d)" % (t, n // 4) for t, n in f["tables"]))
        if s in ptr_in_data:
            notes.append("data pointer at " + ", ".join(
                "0x%08X" % a for a in ptr_in_data[s][:4]))
        rows.append({
            "address": "0x%08X" % s,
            "size": f["size"],
            "pad_after": f["pad"],
            "ghidra_name": g["name"] if g else "",
            "ghidra_size": gsize if g else "",
            "block": block_of(s),
            "proposed_name": ("%s [hypothesis]" % CRT_GUESSES[s][0]
                              if s in CRT_GUESSES else ""),
            "class": f["class"],
            "status": f["status"],
            "subclass": f["sub"],
            "n_callers": len(callers[s]),
            "n_ptr_refs": len(refs[s]) + len(ptr_in_data.get(s, [])),
            "n_callees": len(callees),
            "callers": " ".join("0x%08X" % c for c in sorted(callers[s])),
            "callees": " ".join("0x%08X" % c for c in sorted(callees)),
            "imports_allegro": " ".join(al),
            "imports_win32": " ".join(w32),
            "calls_crt": " ".join(crt),
            "strings": " | ".join(strs),
            "n_data_refs": len(f["data"]),
            "also_in": " ".join(f.get("also_in", [])),
            "reason": " ; ".join(f["why"]),
            "notes": " ; ".join(notes),
        })
    with open(OUT_CSV, "w", newline="", encoding="utf-8") as fo:
        w = csv.DictWriter(fo, fieldnames=list(rows[0]))
        w.writeheader()
        w.writerows(rows)

    with open(OUT_DATA, "w", newline="", encoding="utf-8") as fo:
        w = csv.writer(fo)
        w.writerow(["address", "section", "kind", "string", "functions",
                    "classes"])
        for v in sorted(datarefs):
            d = datarefs[v]
            st = img.string_at(v) if "string" in d["kinds"] or                 "string-interior" in d["kinds"] else ""
            w.writerow(["0x%08X" % v, img.sec(v), "+".join(sorted(d["kinds"])),
                        json.dumps(st, ensure_ascii=False) if st else "",
                        " ".join("0x%08X" % x for x in sorted(d["funcs"])),
                        " ".join(sorted({F[x]["class"] for x in d["funcs"]}))])

    js = {}
    for s in ent:
        f = F[s]
        js["0x%08X" % s] = {
            "size": f["size"], "pad": f["pad"], "pad_bytes": f["pad_bytes"],
            "class": f["class"], "status": f["status"], "sub": f["sub"],
            "name": fname(F, s),
            "calls": ["0x%08X" % c for c in sorted(f["calls"])],
            "tail": ["0x%08X" % c for c in sorted(f["tail"])],
            "fptr": ["0x%08X" % c for c in sorted(f["fptr"])],
            "callers": ["0x%08X" % c for c in sorted(callers[s])],
            "ptr_in_data": ["0x%08X" % a for a in ptr_in_data.get(s, [])],
            "imports": sorted("%s:%s" % (n, "/".join(sorted(u)))
                              for (k, n, t), u in f["imports"].items()),
            "strings": {"0x%08X" % k: v for k, v in f["strings"].items()},
            "data": ["0x%08X" % d for d in sorted(f["data"])],
            "added_by": f["added_by"],
            "in_from_game": s in from_game, "in_from_start": s in from_start,
            "in_from_data": s in from_data,
        }
    OUT_JSON.write_text(json.dumps({"entry": "0x%08X" % entry,
                                    "functions": js}, indent=1),
                        encoding="utf-8")

    from collections import Counter
    c = Counter((F[s]["class"], F[s]["status"]) for s in ent)
    sz = Counter()
    for s in ent:
        sz[F[s]["class"]] += F[s]["size"]
    print("functions: %d (Ghidra %d, added %d)" % (
        len(ent), sum(1 for s in ent if F[s]["ghidra"]),
        sum(1 for s in ent if not F[s]["ghidra"])))
    for k, v in sorted(c.items()):
        print("  %-8s %-10s %d" % (k[0], k[1], v))
    for k, v in sorted(sz.items()):
        print("  size %-8s %d bytes" % (k, v))
    print("added:")
    for s in ent:
        if not F[s]["ghidra"]:
            print("  0x%08X %5d B  %-8s %s" % (s, F[s]["size"], F[s]["class"],
                                             ", ".join(F[s]["added_by"][:5])))


if __name__ == "__main__":
    main()
