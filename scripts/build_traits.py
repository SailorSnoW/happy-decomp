"""Compiler, linker, CRT type and code generation traits of the three exe
files (read-only).

Inputs: game/*.exe (bytes), captures/inventory-<exe>.json (produced by
inventory.py: function classes and sizes).

Prints, for each exe:
1. PE header (dates, versions, alignments, stack/heap) and Rich header;
2. CRT type tests (same evidence as layout-happy.md §4): TLS imports,
   critical sections and threads, FS: accesses other than FS:[0], lock
   prefixes, argument of _heap_init in the entry point, address of errno
   passed by main;
3. traits of the GAME functions (excluding Allegro header code): alignment
   and padding, ebp frame, stack probes, inline intrinsics (rep stos,
   rep movs, repne scas) versus CRT calls, string section, duplicates.

Usage: .venv/Scripts/python scripts/build_traits.py
"""
import json
import pathlib
import struct
import sys
import time
from collections import Counter

import capstone
from capstone import x86

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from inventory import Image, values_of  # noqa: E402
from pe_allegro import PE  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parent.parent
EXES = ("happy.exe", "maped.exe", "loader.exe")
# comp.id names (public tables, from memory: [hypothesis]).
PRODID = {0: "Unknown", 1: "Import0", 4: "Linker600", 6: "Cvtres500",
          10: "Utc12_C", 11: "Utc12_CPP", 12: "AliasObj60", 14: "Masm613",
          19: "Linker512"}
THREAD_IMPORTS = ("TlsAlloc", "TlsGetValue", "TlsSetValue", "TlsFree",
                  "InitializeCriticalSection", "EnterCriticalSection",
                  "LeaveCriticalSection", "DeleteCriticalSection",
                  "CreateThread", "ExitThread")


def pe_header(pe):
    b, o = pe.b, pe.e_lfanew + 24
    f = {}
    f["TimeDateStamp"] = time.strftime("%Y-%m-%d %H:%M:%S UTC",
                                       time.gmtime(pe.timestamp))
    f["Linker"] = "%d.%02d" % (pe.lmaj, pe.lmin)
    (f["SizeOfCode"], f["SizeOfInitData"], f["SizeOfUninitData"],
     f["EntryRVA"]) = struct.unpack_from("<IIII", b, o + 4)
    f["SectionAlign"], f["FileAlign"] = struct.unpack_from("<II", b, o + 32)
    f["OSVer"] = "%d.%d" % pe.os_ver
    f["ImageVer"] = "%d.%d" % struct.unpack_from("<HH", b, o + 44)
    f["SubsysVer"] = "%d.%d" % pe.subsys_ver
    f["CheckSum"], f["Subsystem"], f["DllChars"] = struct.unpack_from(
        "<IHH", b, o + 64)
    (f["StackReserve"], f["StackCommit"], f["HeapReserve"],
     f["HeapCommit"]) = struct.unpack_from("<IIII", b, o + 72)
    f["Characteristics"] = pe.characteristics
    f["Sections"] = " ".join("%s(0x%X,0x%08X)" % (n, vs, ch)
                             for n, va, vs, rp, rs, ch in pe.sections)
    f["DataDirs"] = " ".join("%d" % i for i, (r, s) in enumerate(pe.dirs) if r)
    return f


def fmt(v):
    return "0x%X" % v if isinstance(v, int) and v > 9 else str(v)


def main():
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    res = {}
    for exe in EXES:
        pe = PE(ROOT / "game" / exe)
        img = Image(pe)
        inv = json.loads((ROOT / "captures" / ("inventory-%s.json" %
                                                exe.split(".")[0]))
                         .read_text(encoding="utf-8"))
        F = {int(k, 16): v for k, v in inv["functions"].items()}
        r = {"pe": pe_header(pe), "rich": pe.rich()}
        imps = pe.imports()
        k32 = [n for d, lst in imps.items() if d.lower() == "kernel32.dll"
               for n, _ in lst]
        r["dlls"] = {d: len(lst) for d, lst in imps.items()}
        r["thread_imports"] = [n for n in k32 if n in THREAD_IMPORTS]
        iat = {slot: n for d, lst in imps.items() for n, slot in lst}

        fs, lock, insns = Counter(), 0, {}
        for s, f in F.items():
            ins = list(md.disasm(img.read(s, f["size"]), s))
            insns[s] = ins
            for x in ins:
                if x.prefix[0] == 0xF0:
                    lock += 1
                for op in x.operands:
                    if op.type == x86.X86_OP_MEM and \
                            op.mem.segment == x86.X86_REG_FS:
                        fs["fs:[0x%X]" % op.mem.disp] += 1
        r["fs_access"] = dict(fs)
        r["lock_prefix"] = lock
        # Entry point: xor esi,esi ; push esi ; call _heap_init → argument 0.
        ent = int(inv["entry"], 16)
        seq = insns[ent]
        heap_arg = None
        for i in range(2, len(seq)):
            if seq[i].mnemonic == "call" and seq[i - 1].mnemonic == "push" and \
                    seq[i - 2].mnemonic == "xor" and \
                    seq[i - 1].op_str == seq[i - 2].op_str.split(",")[0]:
                heap_arg = "0x%08X: %s %s ; push %s ; call %s" % (
                    seq[i - 2].address, seq[i - 2].mnemonic,
                    seq[i - 2].op_str, seq[i - 1].op_str, seq[i].op_str)
                break
        r["heap_init_call"] = heap_arg
        # main: 2nd argument of install_allegro (address of errno).
        main = int(inv["main"], 16)
        errno = None
        pushes = []
        for x in insns[main]:
            if x.mnemonic == "push":
                pushes.append(x.op_str)
            if x.mnemonic == "call" and any(
                    iat.get(v) == "#358" for v, _ in values_of(x)):
                errno = "0x%08X: install_allegro(%s)" % (
                    x.address, ", ".join(reversed(pushes[-3:])))
                break
        r["install_allegro_in_main"] = errno

        # Traits of the GAME functions
        game = [s for s, f in F.items() if f["class"] == "GAME" and
                not f["sub"]]
        g = Counter()
        pads = Counter()
        for s in game:
            f = F[s]
            g["n"] += 1
            g["bytes"] += f["size"]
            g["aligned16"] += s % 16 == 0
            pads.update(set(bytes.fromhex(f["pad_bytes"])))
            ins = insns[s]
            if len(ins) >= 2 and ins[0].mnemonic == "push" and \
                    ins[0].op_str == "ebp" and ins[1].mnemonic == "mov" and \
                    ins[1].op_str == "ebp, esp":
                g["ebp_frame"] += 1
                r.setdefault("ebp_frame_funcs", []).append("0x%08X" % s)
            for x in ins:
                m = x.mnemonic
                if m.startswith(("rep", "repne", "repe")):
                    g[m] += 1
                if m == "call" and x.operands[0].type == x86.X86_OP_IMM:
                    t = x.operands[0].imm & 0xFFFFFFFF
                    n = F.get(t, {}).get("name", "")
                    if n in ("_memset", "_strlen", "_strncpy", "__chkstk?",
                             "__ftol", "__strcmpi"):
                        g["call " + n] += 1
                if m == "leave":
                    g["leave"] += 1
                if m.startswith("f"):
                    g["x87"] += 1
        r["game"] = dict(g)
        r["game_pad_bytes"] = sorted("0x%02X" % p for p in pads)
        # Strings referenced by the game: section, duplicate contents
        strs = {}
        for s in game + [main]:
            for a, v in F[s]["strings"].items():
                if "+" not in v or not v.startswith('"'):
                    strs[int(a, 16)] = v
        secs = Counter(img.sec(a) for a in strs)
        dup = Counter(strs.values())
        r["game_strings"] = {"sections": dict(secs),
                             "duplicates": [k for k, c in dup.items()
                                            if c > 1]}
        res[exe] = r

    # Side-by-side display
    def row(label, vals):
        print("| %s | %s |" % (label, " | ".join(str(v) for v in vals)))
    print("| | " + " | ".join(EXES) + " |")
    print("|---|" + "---|" * len(EXES))
    for k in res["happy.exe"]["pe"]:
        row(k, [fmt(res[e]["pe"][k]) for e in EXES])
    ids = sorted({(p, b) for e in EXES for p, b, c in res[e]["rich"]})
    for p, b in ids:
        row("Rich %d %s build %d" % (p, PRODID.get(p, "?"), b),
            [next((c for pp, bb, c in res[e]["rich"] if (pp, bb) == (p, b)),
                  "–") for e in EXES])
    for k in ("dlls", "thread_imports", "fs_access", "lock_prefix",
              "heap_init_call", "install_allegro_in_main", "game_pad_bytes",
              "ebp_frame_funcs"):
        row(k, [res[e].get(k, "–") for e in EXES])
    keys = sorted({k for e in EXES for k in res[e]["game"]})
    for k in keys:
        row("game " + k, [res[e]["game"].get(k, 0) for e in EXES])
    row("game strings sections",
        [res[e]["game_strings"]["sections"] for e in EXES])
    row("game strings duplicates",
        [res[e]["game_strings"]["duplicates"] for e in EXES])


if __name__ == "__main__":
    main()
