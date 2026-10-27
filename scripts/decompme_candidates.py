"""decomp.me candidates: medium-sized GAME functions of happy.exe (read-only).

1. Lists the GAME functions of 15 to 60 instructions with their traits:
   loop (conditional or unconditional jump to a lower address), switch
   (jump table or chain of cmp/je on the same register), structure field
   ([reg+disp] with reg ≠ esp), call, integer arithmetic, ebp frame,
   presence in maped.exe (notes/data/shared-functions.csv).
2. With --dump 0x...: full Intel disassembly (address, raw bytes,
   instruction) of each function, targets annotated (import, string, function).

Usage: .venv/Scripts/python scripts/decompme_candidates.py
        .venv/Scripts/python scripts/decompme_candidates.py --dump 0x00401350 ...
"""
import csv
import json
import pathlib
import sys

from capstone import x86

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from inventory import Inventory, iat_map, values_of  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parent.parent
ARITH = {"add", "sub", "imul", "mul", "idiv", "div", "shl", "sar", "shr",
         "neg", "inc", "dec", "cdq", "and", "or", "xor", "lea", "sbb", "adc"}


def shared():
    out = {}
    with open(ROOT / "notes/data/shared-functions.csv", encoding="utf-8") as f:
        for r in csv.DictReader(f):
            out[int(r["happy"], 16)] = (r["maped"], r["confidence"])
    return out


def traits(F, s):
    ins = sorted(F[s]["ins"].values(), key=lambda i: i.address)
    t = set()
    cmps = 0
    for i in ins:
        m = i.mnemonic
        if (m.startswith("j") or m.startswith("loop")) and i.operands and \
                i.operands[0].type == x86.X86_OP_IMM and \
                (i.operands[0].imm & 0xFFFFFFFF) <= i.address:
            t.add("loop")
        if m == "jmp" and i.operands[0].type == x86.X86_OP_MEM and \
                i.operands[0].mem.index:
            t.add("switch-table")
        if m == "cmp":
            cmps += 1
        if m == "call":
            t.add("call")
        if m in ARITH and not ("esp" in i.op_str.split(",")[0]):
            if m not in ("xor", "lea") or "imul" in m or \
                    (m == "lea" and "*" in i.op_str):
                t.add("arith")
            elif m == "lea":
                t.add("arith")
        for op in i.operands:
            if op.type == x86.X86_OP_MEM and op.mem.base not in (
                    0, x86.X86_REG_ESP, x86.X86_REG_EBP) and op.mem.disp \
                    and m != "lea":
                t.add("field")
        if "ebp" in i.op_str and m == "mov" and i.op_str.startswith("ebp, esp"):
            t.add("ebp-frame")
    if cmps >= 3:
        t.add("multi-cmp(%d)" % cmps)
    return ins, t


def listing(inv, iat, s):
    F = inv.F
    ins = sorted(F[s]["ins"].values(), key=lambda i: i.address)
    lines = []
    for i in ins:
        note = []
        for v, _ in values_of(i):
            if v in iat:
                note.append(iat[v][1])
            elif v in F:
                note.append("function 0x%08X (%s)" % (v, F[v]["class"]))
            elif inv.img.sec(v) == ".text":
                continue
            else:
                st = inv.img.string_at(v)
                if st is not None:
                    note.append('"%s"' % st[:40])
                else:
                    note.append("data 0x%08X (%s)" % (v, inv.img.sec(v)))
        lines.append("%08X  %-24s %-6s %s%s" % (
            i.address, i.bytes.hex(" "), i.mnemonic, i.op_str,
            ("  ; " + " ; ".join(note)) if note else ""))
    return "\n".join(lines)


def main():
    inv = Inventory("happy.exe")
    inv.analyse()
    F = inv.F
    cls = {int(k, 16): v["class"] for k, v in json.load(open(
        ROOT / "captures/inventory-happy.json",
        encoding="utf-8"))["functions"].items()}
    for s in F:
        F[s]["class"] = cls.get(s, "?")
    iat = iat_map(inv.pe, "happy.exe")
    if len(sys.argv) > 2 and sys.argv[1] == "--dump":
        for a in sys.argv[2:]:
            s = int(a, 16)
            print("== 0x%08X  size %d  %d instructions" % (
                s, F[s]["size"], len(F[s]["ins"])))
            print(listing(inv, iat, s))
            print()
        return
    sh = shared()
    for s in sorted(F):
        if F[s]["class"] != "GAME":
            continue
        ins, t = traits(F, s)
        if not 15 <= len(ins) <= 60:
            continue
        m = sh.get(s)
        print("0x%08X %4d B %3d ins  %-40s %s" % (
            s, F[s]["size"], len(ins), " ".join(sorted(t)),
            ("maped %s (%s)" % m) if m else ""))


if __name__ == "__main__":
    main()
