"""Function inventory of one game exe (read-only).

Method (notes/layout-happy.md §1, additions in notes/layout-maped.md §1):
1. Entry points = Ghidra functions + added entries: code never reached
   by the flow of a known function but called, jumped to or referenced
   (immediate or pointer in .data/.rdata), then dead code placed after a
   ret/jmp or padding.
2. Contiguous model (VC6 does not split functions): a function runs from its
   entry to the next entry; the trailing padding (0x90/0xCC) is excluded.
3. Recursive disassembly with capstone from the entry, the jump table
   targets (jmp [reg*4+T]) and the SEH filters/handlers.
4. References: direct call/jmp, IAT slots, immediates and displacements
   pointing into .text/.rdata/.data; strings.
5. Classification GAME / CRT / THUNK / UNKNOWN with status [verified]/[hypothesis].
6. Cross-check against Ghidra's callees.
7. Comparison between exes (instruction bytes, addresses masked).

Details:
- the Ghidra cache comes from ghidra_dump_rest.py (GhidraMCP 7.0.0, JSON);
  the union of the Ghidra bodies is the complement of find_code_gaps;
- WinMain, main and _except_handler3 are found from the PE entry point
  (and compared with the expected values);
- for maped and loader, happy's [hypothesis] CRT names are carried over to
  identical functions (same size, same masked bytes);
- thunks show the import they target; each GAME function is also
  searched for in alleg40.dll (copies of Allegro inline functions).

Outputs: notes/data/functions-<exe>.csv, notes/data/datarefs-<exe>.csv,
captures/inventory-<exe>.json

Usage: .venv/Scripts/python scripts/inventory.py maped.exe
(dependency: capstone, installed in .venv)
"""
import bisect
import csv
import json
import pathlib
import struct
import sys
from collections import Counter, defaultdict

import capstone
from capstone import x86

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from pe_allegro import PE  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parent.parent
ALLEG_IMP = ROOT / "notes" / "data" / "alleg40-imports.csv"
DLL = ROOT / "captures" / "alleg40_unpacked.dll"
EXES = ("happy.exe", "maped.exe", "loader.exe")
PAD = (0x90, 0xCC)

# CRT functions of happy.exe identified by hand (all [hypothesis]).
# Carried over to maped/loader when the code is identical.
CRT_GUESSES_HAPPY = {
    0x0040AD50: ("rand", "LCG holdrand*0x343FD+0x269EC3, >>16 & 0x7FFF"),
    0x0040B31E: ("atexit", "passed to install_allegro (allegro_init); "
                 "returns (_onexit(f) == NULL) ? -1 : 0"),
    0x0040B330: ("__onexitinit", "malloc(0x80), R6024 (_amsg_exit(0x18)) on failure"),
    0x0040B0F6: ("__initstdio", "allocates 20 FILE* (0x14), _amsg_exit(0x1A) on failure"),
    0x0040B19B: ("__endstdio", "calls _flushall then _fcloseall"),
    0x0040B080: ("_flushall", "flsall(1)"),
    0x0040C45B: ("_fcloseall", "loops over the streams from 3 onwards, fclose"),
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

# Allegro header code (out-of-line copy of an inline function), in addition to
# the END_OF_MAIN() stub: recognised in the other exes by identical code.
HEADER_HAPPY = {0x00402110: "_set_errno_erange() from al386vc.h"}

# Per exe: end of the game range (start of the first library contribution),
# expected values of the call chain, blocks, CRT names specific to the exe
# (not found in happy). Justifications: notes/layout-*.md.
CONFIGS = {
    "happy.exe": {
        # rand() (LCG 0x343FD / 0x269EC3): first CRT function.
        "game_end": 0x0040AD50,
        "expect": (0x0040A970, 0x0040A830, 0x0040DF44),
        "blocks": [
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
        ],
        "crt_guesses": {},
    },
    "maped.exe": {
        # 0x00405CA0: 7 import thunks (Allegro d_*_proc), then the CRT.
        "game_end": 0x00405CA0,
        "expect": (0x00405C70, 0x00405AE0, 0x00409674),
        "blocks": [
            (0x00401000, 0x00401400, "block_01"),
            (0x00401400, 0x004016A0, "block_02"),
            (0x004016A0, 0x004018D0, "block_03"),
            (0x004018D0, 0x00401D90, "block_04"),
            (0x00401D90, 0x00403200, "block_05"),
            (0x00403200, 0x00403EB0, "block_06"),
            (0x00403EB0, 0x00405CA0, "block_07"),
            (0x00405CA0, 0x00405CCA, "block_08_thunks"),
            (0x00405CCA, 0x0040D000, "block_09_crt"),
        ],
        "crt_guesses": {
            0x0040607D: ("sprintf", "string FILE on the stack (_flag 0x42 = "
                         "_IOWRT|_IOSTRG, _cnt 0x7FFFFFFF), calls _output "
                         "0x004081C3 then writes the final 0; receives "
                         "\"maped%03d.%s\" at 0x004018D0"),
            0x004081C3: ("_output", "printf engine (1,857 bytes, jump "
                         "tables), sole caller sprintf 0x0040607D"),
            0x0040615B: ("atol", "skips whitespace (_isctype(c, 8)), sign "
                         "'-'/'+', then digits"),
            0x004061E6: ("atoi", "return atol(s) (0x0040615B)"),
            0x0040C516: ("_chdir", "SetCurrentDirectoryA, then "
                         "GetCurrentDirectoryA(0x105) and "
                         "SetEnvironmentVariableA(\"=X:\"); called by main"),
            0x0040C59C: ("toupper", "called by _chdir on the drive "
                         "letter, result stored in \"=X:\""),
            0x0040C1C0: ("__chkstk", "probes the stack in 0x1000-byte pages "
                         "(test [ecx], eax), size in eax"),
            0x00408C40: ("__allmul", "64-bit multiplication (mul, ret 0x10); "
                         "unreferenced"),
        },
    },
    "loader.exe": {
        # 0x004014D0: _strncpy (Function ID), first CRT function.
        "game_end": 0x004014D0,
        "expect": (0x004014A0, 0x00401000, 0x004023A0),
        "blocks": [
            (0x00401000, 0x004014D0, "block_01"),
            (0x004014D0, 0x00405000, "block_02_crt"),
        ],
        "crt_guesses": {
            0x00401780: ("__chkstk", "main does mov eax, 0x2810; call "
                         "0x00401780; probes the stack in 0x1000-byte pages"),
            0x00404226: ("_chdir", "identical to maped.exe@0x0040C516 "
                         "(SetCurrentDirectoryA, \"=X:\"); called by main"),
            0x00404313: ("toupper", "identical to maped.exe@0x0040C59C"),
        },
    },
}


def stem(exe):
    return exe.split(".")[0]


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
        """Printable C string (>= 2 chars) starting at a, else None."""
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


def parse_ghidra(cache, text_s, text_e):
    """Ghidra functions and the union of their bodies (complement of the gaps)."""
    funcs = {}
    by_addr = {int(k, 16): v for k, v in cache["functions"].items()}
    for f in cache["find_functions"]:
        a = int(f["address"], 16)
        d = by_addr[a]
        callees = [int(c["address"], 16) for c in d.get("callees", [])
                   if not c["address"].startswith("EXTERNAL")]
        funcs[a] = {"name": f["name"], "size": d["size"],
                    "body_end": int(d["body_end"], 16),
                    "callers": [int(c["address"], 16)
                                for c in d.get("callers", [])],
                    "callees": callees}
    gaps = sorted((int(g["start"], 16), int(g["end"], 16))
                  for g in cache["find_code_gaps"])
    covered, p = [], text_s
    for s, e in gaps:
        if s > p:
            covered.append((p, s - 1))
        p = max(p, e + 1)
    if p < text_e:
        covered.append((p, text_e - 1))
    return funcs, covered


def iat_map(pe, exe):
    names = {}
    with open(ALLEG_IMP, encoding="utf-8") as f:
        for r in csv.DictReader(f):
            if r["exe"] == exe:
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


def entry_chain(img, dis, entry, iat):
    """From the PE entry point: (WinMain, main, _except_handler3) [verified].
    - _except_handler3: last immediate push before FS:[0] is read;
    - WinMain: direct call that follows call [GetModuleHandleA]; push eax;
    - main: immediate pushed by WinMain before call [Allegro's _WinMain]."""
    a, pushes, prev, winmain, handler = entry, [], [], None, None
    while winmain is None and a < entry + 0x200:
        x = dis.at(a)
        if x.mnemonic == "push" and x.operands[0].type == x86.X86_OP_IMM:
            pushes.append(x.operands[0].imm & 0xFFFFFFFF)
        if handler is None and "fs:[0]" in x.op_str and pushes:
            handler = pushes[-1]
        if x.mnemonic == "call" and x.operands[0].type == x86.X86_OP_IMM \
                and len(prev) >= 2 and prev[-1].mnemonic == "push" \
                and prev[-2].mnemonic == "call" and any(
                    iat.get(v, ("", ""))[1].endswith("!GetModuleHandleA")
                    for v, _ in values_of(prev[-2])):
            winmain = x.operands[0].imm & 0xFFFFFFFF
        prev.append(x)
        a += x.size
    a, last_push, main = winmain, None, None
    while main is None and a < winmain + 0x40:
        x = dis.at(a)
        if x.mnemonic == "push" and x.operands[0].type == x86.X86_OP_IMM:
            last_push = x.operands[0].imm & 0xFFFFFFFF
        if x.mnemonic == "call" and any(
                iat.get(v, ("", ""))[1] == "_WinMain" for v, _ in values_of(x)):
            main = last_push
        a += x.size
    return winmain, main, handler


def decode_function(dis, img, start, end, except_handler3):
    """Recursive disassembly within [start, end). Returns instructions and tables."""
    seen = {}
    tables = []
    work = [start]
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
                    (ins.operands[0].imm & 0xFFFFFFFF) == except_handler3:
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


class Inventory:
    def __init__(self, exe):
        self.exe = exe
        self.cfg = CONFIGS[exe]
        self.game_end = self.cfg["game_end"]
        self.pe = PE(ROOT / "game" / exe)
        self.img = Image(self.pe)
        self.dis = Disasm(self.img)
        self.cache = json.loads((ROOT / "captures" / "ghidra-cache" / (
            stem(exe) + "_exe_rest.json")).read_text(encoding="utf-8"))
        self.iat = iat_map(self.pe, exe)
        self.text_s = self.img.secs[".text"][0]
        self.text_e = self.text_s + self.pe.sections[0][2]
        self.gh, self.covered = parse_ghidra(self.cache, self.text_s,
                                             self.text_e)
        opt = self.pe.e_lfanew + 24
        self.entry = self.pe.image_base + struct.unpack_from(
            "<I", self.pe.b, opt + 16)[0]
        chain = entry_chain(self.img, self.dis, self.entry, self.iat)
        if chain != self.cfg["expect"]:
            raise SystemExit("%s: call chain %s, expected %s" % (
                exe, [hex(v or 0) for v in chain],
                [hex(v) for v in self.cfg["expect"]]))
        self.winmain, self.main, self.eh3 = chain

    def inside_ghidra(self, v):
        c = self.covered
        i = bisect.bisect_right(c, (v, 0xFFFFFFFF)) - 1
        return i >= 0 and c[i][0] <= v <= c[i][1]

    def build(self):
        img, dis = self.img, self.dis
        text_s, text_e = self.text_s, self.text_e
        entries = set(self.gh)
        added = {}
        for _ in range(30):
            ent = sorted(entries)
            decoded = {}
            for i, s in enumerate(ent):
                e = ent[i + 1] if i + 1 < len(ent) else text_e
                decoded[s] = decode_function(dis, img, s, e, self.eh3)
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
                if not is_call and (v in reached or self.inside_ghidra(v)):
                    continue
                entries.add(v)
                added[v] = sorted(why)
                new += 1
            if new:
                continue
            # Code neither reached nor referenced, placed after a ret/jmp or
            # padding, that decodes up to a ret/jmp: dead function
            # (unreferenced) kept by the linker.
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
        self.ent, self.decoded, self.added = ent, decoded, added

    def analyse(self):
        self.build()
        img, ent, iat = self.img, self.ent, self.iat
        F = {}
        for i, s in enumerate(ent):
            e = ent[i + 1] if i + 1 < len(ent) else self.text_e
            bs = img.read(s, e - s)
            n = len(bs)
            while n > 0 and bs[n - 1] in PAD:
                n -= 1
            ins, tables = self.decoded[s]
            F[s] = {"start": s, "extent_end": e, "size": n, "pad": len(bs) - n,
                    "pad_bytes": bs[n:].hex(), "ins": ins, "tables": tables,
                    "ghidra": self.gh.get(s), "added_by": self.added.get(s)}

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
                        imp[iat[v]].add("call" if m in ("call", "jmp")
                                        else "use")
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
                            # reference into the middle of a string
                            strings[v] = "%s+%d" % (json.dumps(
                                img.string_at(parent), ensure_ascii=False),
                                v - parent)
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

        def reach(roots, stop=()):
            seen, work = set(), list(roots)
            while work:
                x = work.pop()
                if x in seen or x in stop:
                    continue
                seen.add(x)
                f = F[x]
                work.extend(f["calls"] | f["tail"] | f["fptr"])
                if x == self.winmain:
                    work.append(self.main)
            return seen

        self.F, self.datarefs = F, datarefs
        self.ptr_in_data, self.callers, self.refs = ptr_in_data, callers, refs
        self.from_game = reach([self.winmain])
        self.from_start = reach([self.entry], stop=(self.winmain,))
        # data pointers: initialisation tables, _cfltcvt_tab, but
        # also DIALOG arrays (maped); they are treated as roots.
        self.from_data = reach(list(ptr_in_data), stop=(self.winmain,))
        # Extension for the game range: code pointers stored in a data item
        # referenced by a function (from the referenced address to the
        # next one, e.g. maped's DIALOG arrays). Only used to classify
        # a game-range function that reach() does not reach.
        keys = sorted(datarefs)
        objptr = defaultdict(dict)
        for s, f in F.items():
            for v in f["data"]:
                i = bisect.bisect_right(keys, v)
                end = min(keys[i] if i < len(keys) else v + 4, v + 0x2000)
                for a in range(v, end - 3, 4):
                    t = img.dword(a)
                    if t in F and img.sec(a) in (".rdata", ".data"):
                        objptr[s][t] = (v, a)
        self.objptr = objptr
        seen, work, self.via_obj = set(), [self.winmain, self.main], {}
        while work:
            x = work.pop()
            if x in seen:
                continue
            seen.add(x)
            f = F[x]
            work.extend(f["calls"] | f["tail"] | f["fptr"])
            for t, (v, a) in objptr[x].items():
                self.via_obj.setdefault(t, (x, v, a))
                work.append(t)
        self.from_game_obj = seen

    def sig(self, s):
        return b"".join(b for _, b in masked_stream(self.img, s,
                                                    self.F[s]["size"]))

    def classify(self, crt_guesses, header):
        F, gend = self.F, self.game_end
        for s, f in F.items():
            g = f["ghidra"]
            gname = g["name"] if g else ""
            ins = sorted(f["ins"].items())
            fid = bool(gname) and not gname.startswith("FUN_")
            reasons = []
            sub = ""
            if len(ins) == 1 and ins[0][1].mnemonic == "jmp":
                cls, st = "THUNK", "verified"
                x = ins[0][1]
                tgt = [self.iat[v][1] for v, _ in values_of(x) if v in self.iat]
                reasons.append("single instruction: %s %s%s"
                               % (x.mnemonic, x.op_str,
                                  " (IAT %s)" % tgt[0] if tgt else ""))
            elif s == self.entry:
                cls, st = "CRT", "verified"
                reasons.append("PE entry point (AddressOfEntryPoint): "
                               "WinMainCRTStartup")
            elif fid:
                cls, st = "CRT", "verified"
                reasons.append("named by Ghidra (Function ID): %s" % gname)
            elif s < gend:
                if s in self.from_game:
                    cls, st = "GAME", "hypothesis"
                    reasons.append("range 0x00401000-0x%08X (16-byte aligned, "
                                   "0x90 padding), reachable from "
                                   "WinMain" % (gend - 1))
                elif s in self.from_game_obj:
                    cls, st = "GAME", "hypothesis"
                    x, v, a = self.via_obj[s]
                    reasons.append("range 0x00401000-0x%08X, reachable from "
                                   "WinMain only through a data pointer "
                                   "(0x%08X, in object 0x%08X "
                                   "referenced by 0x%08X)" % (gend - 1, a, v, x))
                else:
                    cls, st = "UNKNOWN", "hypothesis"
                    reasons.append("game range but not reachable from "
                                   "WinMain")
                if s == self.winmain:
                    sub = "allegro-header"
                    reasons.append("Allegro END_OF_MAIN() stub (alwin.h)")
                if s in header:
                    sub = "allegro-header"
                    reasons.append(header[s])
                if s == self.main:
                    reasons.append("_mangled_main: the game's main() passed to "
                                   "_WinMain")
            else:
                st = "hypothesis"
                if s in self.from_start and s not in self.from_game:
                    cls = "CRT"
                    reasons.append("reachable only from the PE entry point "
                                   "(not through WinMain)")
                elif s in self.from_data and s not in self.from_game:
                    cls = "CRT"
                    reasons.append("reachable only through data pointers "
                                   "(CRT init/term tables)")
                elif s in self.from_game:
                    cls = "CRT"
                    reasons.append("after 0x%08X (CRT range, unaligned), "
                                   "reachable from WinMain" % gend)
                elif s in crt_guesses:
                    cls = "CRT"
                    reasons.append("after 0x%08X, no reference found, "
                                   "recognised CRT pattern" % gend)
                else:
                    cls = "UNKNOWN"
                    reasons.append("after 0x%08X, no reference found"
                                   % gend)
                if s in crt_guesses:
                    reasons.append("identified as %s: %s" % crt_guesses[s])
            if f.get("in_dll"):
                reasons.append("identical code (addresses masked) in "
                               "alleg40.dll@" + f["in_dll"])
            if f.get("also_in"):
                reasons.append("identical code (addresses masked) in " +
                               ", ".join(f["also_in"]))
            f["class"], f["status"], f["sub"], f["why"] = cls, st, sub, reasons

    def fname(self, s, crt_guesses):
        g = self.F[s]["ghidra"]
        if g and not g["name"].startswith("FUN_"):
            return g["name"]
        if s in crt_guesses:
            return "%s?" % crt_guesses[s][0]
        return "FUN_%08x" % s

    def ghidra_crosscheck(self):
        """Internal callees: Ghidra versus this script, for the functions
        both know. Returns the list of mismatches."""
        out = []
        for s, f in self.F.items():
            g = f["ghidra"]
            if not g:
                continue
            ours = f["calls"] | f["tail"]
            theirs = {c for c in g["callees"] if self.text_s <= c < self.text_e}
            if ours != theirs:
                out.append((s, sorted(ours - theirs), sorted(theirs - ours)))
        return out


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
            # the other exes have data beyond 0x00450000
            for v in [v for v in vals if 0x00400000 <= v < 0x00500000]:
                j = bytes(b).find(struct.pack("<I", v))
                if j >= 0:
                    b[j:j + 4] = b"\0" * 4
        out.append((i.address, bytes(b)))
    return out


def find_in(img, mine, label, lo=0x00400000, hi=0x00500000):
    """{start: "label@0xADDR"} for each signature of `mine` found at an
    instruction boundary in the .text of `img`. For the DLL, the masked
    absolute addresses are those in [lo, hi) (base 0x10000000)."""
    s0, e0, _, _ = img.secs[".text"]
    st = masked_stream_range(img, s0, e0 - s0, lo, hi)
    cat = b"".join(b for _, b in st)
    offs, p = [], 0
    for a, b in st:
        offs.append(p)
        p += len(b)
    res = {}
    for s, sig in mine.items():
        k = cat.find(sig)
        if k >= 0:
            i = bisect.bisect_right(offs, k) - 1
            if offs[i] == k:
                res[s] = "%s@0x%08X" % (label, st[i][0])
    return res


def masked_stream_range(img, start, size, lo, hi):
    if (lo, hi) == (0x00400000, 0x00500000):
        return masked_stream(img, start, size)
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
            for v in [v for v in vals if lo <= v < hi]:
                j = bytes(b).find(struct.pack("<I", v))
                if j >= 0:
                    b[j:j + 4] = b"\0" * 4
        out.append((i.address, bytes(b)))
    return out


def run(exe):
    inv = Inventory(exe)
    inv.analyse()
    F, ent = inv.F, inv.ent

    # 7. Comparison between exes (same method as for happy: the function's
    # signature is searched for in the whole .text of the other exe).
    mine = {s: inv.sig(s) for s in ent if F[s]["size"] >= 16}
    for other in EXES:
        if other == exe:
            continue
        oi = Image(PE(ROOT / "game" / other))
        for s, where in find_in(oi, mine, other).items():
            F[s].setdefault("also_in", []).append(where)
    # Game-range functions found in alleg40.dll (inline).
    game_sigs = {s: v for s, v in mine.items() if s < inv.game_end}
    for s, where in find_in(Image(PE(DLL)), game_sigs, "x",
                            0x10000000, 0x10100000).items():
        F[s]["in_dll"] = where.split("@")[1]

    # [hypothesis] CRT names and header code carried over from happy.exe to
    # identical functions (same size, same masked bytes).
    crt_guesses = dict(inv.cfg["crt_guesses"])
    header = {}
    happy = inv if exe == "happy.exe" else Inventory("happy.exe")
    if exe == "happy.exe":
        crt_guesses.update(CRT_GUESSES_HAPPY)
        header.update(HEADER_HAPPY)
    else:
        happy.analyse()
        hs = defaultdict(list)
        for s in happy.ent:
            if happy.F[s]["size"] >= 1:
                hs[happy.sig(s)].append(s)
        for s in ent:
            h = hs.get(inv.sig(s), [])
            ins = list(F[s]["ins"].values())
            if len(h) != 1 or (len(ins) == 1 and ins[0].mnemonic == "jmp"):
                # a masked jmp [IAT] thunk looks like all the others
                continue
            h = h[0]
            F[s]["same_as_happy"] = h
            g = happy.F[h]["ghidra"]
            if h in CRT_GUESSES_HAPPY and s >= inv.game_end:
                n, why = CRT_GUESSES_HAPPY[h]
                crt_guesses.setdefault(s, (n, "identical to happy.exe@0x%08X "
                                           "(%s)" % (h, why)))
            elif g and not g["name"].startswith("FUN_") and s >= inv.game_end \
                    and not (F[s]["ghidra"] and not F[s]["ghidra"]["name"]
                             .startswith("FUN_")):
                crt_guesses.setdefault(s, (g["name"], "identical to "
                                           "happy.exe@0x%08X (Function ID "
                                           "in happy)" % h))
            if h in HEADER_HAPPY:
                header[s] = HEADER_HAPPY[h] + " (identical to happy.exe@" \
                    "0x%08X)" % h
    inv.classify(crt_guesses, header)
    write_outputs(inv, crt_guesses)
    return inv, crt_guesses


def block_of(blocks, s):
    for b, e, n in blocks:
        if b <= s < e:
            return n
    return ""


def write_outputs(inv, crt_guesses):
    F, ent, datarefs = inv.F, inv.ent, inv.datarefs
    callers, refs, ptr_in_data = inv.callers, inv.refs, inv.ptr_in_data
    st = stem(inv.exe)
    rows = []
    for s in ent:
        f = F[s]
        g = f["ghidra"]
        callees = f["calls"] | f["tail"]
        al = sorted(n for (k, n, t), u in f["imports"].items()
                    if k == "ALLEGRO")
        w32 = sorted(n.split("!")[1] for (k, n, t), u in f["imports"].items()
                     if k == "WIN32")
        crt = sorted(inv.fname(c, crt_guesses) for c in callees
                     if F[c]["class"] == "CRT")
        strs = [v if "string-interior" in datarefs[k]["kinds"] else
                json.dumps(v, ensure_ascii=False)
                for k, v in sorted(f["strings"].items())]
        notes = []
        if not g and f["added_by"][0].startswith("unreferenced"):
            notes.append("missing from Ghidra ; " + f["added_by"][0])
        elif not g:
            notes.append("missing from Ghidra ; referenced by " +
                         ", ".join(f["added_by"][:4]))
        elif g["body_end"] + 1 < s + f["size"]:
            notes.append("shorter Ghidra body (%d bytes)" % g["size"])
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
            "ghidra_size": g["size"] if g else "",
            "block": block_of(inv.cfg["blocks"], s),
            "proposed_name": ("%s [hypothesis]" % crt_guesses[s][0]
                              if s in crt_guesses else ""),
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
    out_csv = ROOT / "notes" / "data" / ("functions-%s.csv" % st)
    with open(out_csv, "w", newline="", encoding="utf-8") as fo:
        w = csv.DictWriter(fo, fieldnames=list(rows[0]))
        w.writeheader()
        w.writerows(rows)

    img = inv.img
    with open(ROOT / "notes" / "data" / ("datarefs-%s.csv" % st), "w",
              newline="", encoding="utf-8") as fo:
        w = csv.writer(fo)
        w.writerow(["address", "section", "kind", "string", "functions",
                    "classes"])
        for v in sorted(datarefs):
            d = datarefs[v]
            sv = img.string_at(v) if "string" in d["kinds"] or \
                "string-interior" in d["kinds"] else ""
            w.writerow(["0x%08X" % v, img.sec(v), "+".join(sorted(d["kinds"])),
                        json.dumps(sv, ensure_ascii=False) if sv else "",
                        " ".join("0x%08X" % x for x in sorted(d["funcs"])),
                        " ".join(sorted({F[x]["class"] for x in d["funcs"]}))])

    js = {}
    for s in ent:
        f = F[s]
        js["0x%08X" % s] = {
            "size": f["size"], "pad": f["pad"], "pad_bytes": f["pad_bytes"],
            "class": f["class"], "status": f["status"], "sub": f["sub"],
            "name": inv.fname(s, crt_guesses),
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
            "same_as_happy": ("0x%08X" % f["same_as_happy"]
                              if "same_as_happy" in f else None),
            "in_dll": f.get("in_dll"),
            "in_from_game": s in inv.from_game,
            "in_from_start": s in inv.from_start,
            "in_from_data": s in inv.from_data,
        }
    (ROOT / "captures" / ("inventory-%s.json" % st)).write_text(
        json.dumps({"entry": "0x%08X" % inv.entry,
                    "winmain": "0x%08X" % inv.winmain,
                    "main": "0x%08X" % inv.main,
                    "except_handler3": "0x%08X" % inv.eh3,
                    "functions": js}, indent=1), encoding="utf-8")


def report(inv):
    F, ent = inv.F, inv.ent
    c = Counter((F[s]["class"], F[s]["status"]) for s in ent)
    sz = Counter()
    for s in ent:
        sz[F[s]["class"]] += F[s]["size"]
    print("== %s: entry 0x%08X, WinMain 0x%08X, main 0x%08X, "
          "_except_handler3 0x%08X" % (inv.exe, inv.entry, inv.winmain,
                                       inv.main, inv.eh3))
    print("functions: %d (Ghidra %d, added %d)" % (
        len(ent), sum(1 for s in ent if F[s]["ghidra"]),
        sum(1 for s in ent if not F[s]["ghidra"])))
    for k, v in sorted(c.items()):
        print("  %-8s %-10s %d" % (k[0], k[1], v))
    for k, v in sorted(sz.items()):
        print("  size %-8s %d bytes" % (k, v))
    print("  padding %d bytes; total %d; .text VirtualSize %d" % (
        sum(F[s]["pad"] for s in ent), sum(F[s]["size"] + F[s]["pad"]
                                           for s in ent),
        inv.text_e - inv.text_s))
    print("added:")
    for s in ent:
        if not F[s]["ghidra"]:
            print("  0x%08X %5d B  %-8s %s" % (s, F[s]["size"], F[s]["class"],
                                             ", ".join(F[s]["added_by"][:5])))
    short = [s for s in ent if F[s]["ghidra"] and
             F[s]["ghidra"]["body_end"] + 1 < s + F[s]["size"]]
    print("shorter Ghidra bodies: %d %s" % (
        len(short), " ".join("0x%08X" % s for s in short)))
    cc = inv.ghidra_crosscheck()
    print("callee mismatches with Ghidra: %d" % len(cc))
    for s, mine, theirs in cc:
        print("  0x%08X  +ours %s  +Ghidra %s" % (
            s, " ".join("0x%08X" % v for v in mine),
            " ".join("0x%08X" % v for v in theirs)))


if __name__ == "__main__":
    for e in sys.argv[1:] or ["happy.exe"]:
        report(run(e)[0])
