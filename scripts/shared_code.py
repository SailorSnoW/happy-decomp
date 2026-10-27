"""Game functions (GAME class) shared by several exes (read-only).

Method (details in notes/shared-code.md §1):
1. Functions and their instructions come from inventory.py (same splitting,
   same recursive disassembly); the class comes from notes/data/functions-*.csv.
2. For each GAME function, we extract:
   - the bytes of each instruction, with addresses masked (rel32 of call/jmp,
     immediates and displacements in [0x00400000, 0x00500000));
   - one token per instruction: mnemonic + operands, where each address is
     replaced by what it designates (import, string, CRT function, game
     function, .rdata float, global) and each internal jump by "L";
   - the ordered sequence of calls (imports, CRT, game), the imports used,
     the strings, the non-trivial constants and the floats read from .rdata.
   CRT functions get the same label in all three exes when their code is
   identical to happy's (which is the case for almost all of them).
3. All pairs (happy×maped, happy×loader, maped×loader) are scored; we keep
   the mutual best pairs. Two rounds: in the second, a call to an already
   paired game function carries the pair's identifier.
4. Confidence:
   - "identical": masked bytes identical over the whole function;
   - "very close": masked instructions similar at >= CLOSE_THRESHOLD and same
     call sequence (at >= 0.8);
   - "similar": mnemonic sequence similar at >= SIMILAR_THRESHOLD, with at
     least two specific shared pieces of evidence (non-trivial import or CRT
     call, string, constant, float), at least one of which is about content
     (not just CRT calls: fopen/fwrite/fclose is an idiom).
   Non-mutual best pairs that reach a level (variants) are listed
   separately.

Outputs: notes/data/shared-functions.csv, captures/shared-code.json
(pairs and all useful scores), report on standard output.

Usage: .venv/Scripts/python scripts/shared_code.py
"""
import csv
import json
import pathlib
import struct
import sys
from collections import Counter
from difflib import SequenceMatcher

from capstone import x86

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from inventory import ROOT, Inventory, stem, values_of  # noqa: E402

EXES = ("happy.exe", "maped.exe", "loader.exe")
CLOSE_THRESHOLD = 0.80
SIMILAR_THRESHOLD = 0.55
# Imports too frequent to serve as evidence on their own.
TRIVIAL_IMPORTS = {"screen", "key", "font", "gfx_driver", "textout_centre",
          "textout", "play_sample", "rand", "clear_bitmap", "blit"}
TRIVIAL_CONSTS = {0, 1, 2, 4, 0xFFFFFFFF}


def load_csv(exe):
    with open(ROOT / "notes" / "data" / ("functions-%s.csv" % stem(exe)),
              encoding="utf-8") as f:
        return {int(r["address"], 16): r for r in csv.DictReader(f)}


def ratio(a, b):
    if not a and not b:
        return 1.0
    return SequenceMatcher(None, a, b, autojunk=False).ratio()


def mask_ins(x):
    b = bytearray(x.bytes)
    if x.mnemonic in ("call", "jmp") and b[0] in (0xE8, 0xE9):
        b[1:] = b"\0" * (len(b) - 1)
        return bytes(b)
    vals = [op.imm & 0xFFFFFFFF for op in x.operands
            if op.type == x86.X86_OP_IMM]
    vals += [op.mem.disp & 0xFFFFFFFF for op in x.operands
             if op.type == x86.X86_OP_MEM]
    for v in [v for v in vals if 0x00400000 <= v < 0x00500000]:
        j = bytes(b).find(struct.pack("<I", v))
        if j >= 0:
            b[j:j + 4] = b"\0" * 4
    # short/conditional jumps: the relative displacement stays (internal)
    return bytes(b)


class Exe:
    def __init__(self, exe):
        self.exe = exe
        self.inv = Inventory(exe)
        self.inv.analyse()
        self.rows = load_csv(exe)
        self.game = sorted(a for a, r in self.rows.items()
                           if r["class"] == "GAME")
        self.sigs = {s: self.inv.sig(s) for s in self.inv.ent}

    def name(self, s):
        r = self.rows.get(s)
        if not r:
            return None
        for k in ("proposed_name", "ghidra_name"):
            n = r[k].rstrip("?")
            if n and not n.startswith("FUN_"):
                return n
        return None


def crt_labels(exes):
    """Common label of non-GAME functions (CRT, thunks)."""
    happy = exes["happy.exe"]
    by_sig = {}
    for s in happy.inv.ent:
        if happy.rows.get(s, {}).get("class") != "GAME":
            by_sig.setdefault(happy.sigs[s], s)
    labels = {}
    for e in exes.values():
        lab = {}
        for s in e.inv.ent:
            r = e.rows.get(s)
            if r and r["class"] == "GAME":
                continue
            ins = sorted(e.inv.F[s]["ins"].items())
            if len(ins) == 1 and ins[0][1].mnemonic == "jmp":
                t = [e.inv.iat[v][1] for v, _ in values_of(ins[0][1])
                     if v in e.inv.iat]
                if t:
                    lab[s] = "imp:" + t[0].split("!")[-1]
                    continue
            h = by_sig.get(e.sigs[s]) if e.exe != "happy.exe" else s
            if h is not None and h in happy.rows:
                n = happy.name(h) or e.name(s)
                lab[s] = "crt:" + (n or "happy@0x%08X" % h)
            else:
                n = e.name(s)
                lab[s] = "crt:" + (n or "%s@0x%08X" % (stem(e.exe), s))
        labels[e.exe] = lab
    return labels


def features(e, s, labels, pair_of):
    """Features of a GAME function; pair_of: {GAME address: pair id}."""
    inv, img = e.inv, e.inv.img
    f = inv.F[s]
    lab = labels[e.exe]
    toks, mbytes, mn, calls = [], [], [], []
    imports, strings, consts, floats = set(), set(), Counter(), set()
    nglob = 0

    def target(v):
        if v in inv.iat:
            return "imp:" + inv.iat[v][1].split("!")[-1]
        sec = img.sec(v)
        if sec == ".text":
            if v in lab:
                return lab[v]
            if v in e.rows and e.rows[v]["class"] == "GAME":
                return "game:%s" % pair_of.get((e.exe, v), "?")
            return "code"
        if sec in (".rdata", ".data"):
            st = img.string_at(v)
            parent = img.string_parent(v)
            if st is not None:
                return "str:%s+%d" % (json.dumps(img.string_at(parent)), v - parent)
            return None
        return None

    for a, x in sorted(f["ins"].items()):
        m = x.mnemonic
        mn.append(m)
        mbytes.append(mask_ins(x))
        tok = [m]
        for op in x.operands:
            if op.type == x86.X86_OP_IMM:
                v = op.imm & 0xFFFFFFFF
                if m.startswith("j") or m.startswith("loop") or m == "call":
                    if s <= v < f["extent_end"]:
                        tok.append("L")
                        continue
                if 0x00400000 <= v < 0x00500000:
                    t = target(v) or "data"
                    tok.append(t)
                    if m in ("call", "jmp") or t.startswith(("crt:", "imp:")):
                        if not (m.startswith("j") and t == "code"):
                            calls.append(("fptr:" if m == "push" else "") + t)
                    if t.startswith("imp:"):
                        imports.add(t[4:])
                    if t.startswith("str:"):
                        strings.add(t[4:])
                else:
                    tok.append(hex(v))
                    if v not in TRIVIAL_CONSTS and not (
                            m in ("add", "sub") and x.op_str.startswith("esp")):
                        consts[v] += 1
            elif op.type == x86.X86_OP_MEM:
                d = op.mem.disp & 0xFFFFFFFF
                if op.mem.base == 0 and 0x00400000 <= d < 0x00500000:
                    if d in inv.iat:
                        t = "imp:" + inv.iat[d][1].split("!")[-1]
                        imports.add(t[4:])
                        if m in ("call", "jmp"):
                            calls.append(t)
                    elif img.sec(d) == ".rdata" and m.startswith("f") \
                            and op.size in (4, 8):
                        raw = img.read(d, op.size)
                        val = struct.unpack("<f" if op.size == 4 else "<d",
                                            raw)[0]
                        t = "flt:%r" % val
                        floats.add(val)
                    else:
                        t = target(d)
                        if t is None:
                            t = "glob" + ("[i]" if op.mem.index else "")
                            nglob += 1
                        elif t.startswith("str:"):
                            strings.add(t[4:])
                    tok.append("[%s]" % t)
                else:
                    base = x.reg_name(op.mem.base) if op.mem.base else ""
                    idx = x.reg_name(op.mem.index) if op.mem.index else ""
                    # [reg+global]: global array indexed by a register
                    disp = "glob" if 0x00400000 <= d < 0x00500000 \
                        else "%#x" % d
                    tok.append("[%s+%s*%d+%s]" % (base, idx, op.mem.scale,
                                                  disp))
            else:
                tok.append(x.reg_name(op.reg))
        toks.append(" ".join(tok))
    return {"addr": s, "size": f["size"], "n_ins": len(mn), "mn": mn,
            "toks": toks, "mbytes": mbytes, "calls": calls,
            "imports": imports, "strings": strings, "consts": consts,
            "floats": floats, "nglob": nglob, "sig": e.sigs[s]}


def compare(a, b):
    sz = min(a["size"], b["size"]) / max(a["size"], b["size"], 1)
    r = {"ident": a["sig"] == b["sig"],
         "s_bytes": ratio(a["mbytes"], b["mbytes"]),
         "s_tok": ratio(a["toks"], b["toks"]),
         "s_mn": ratio(a["mn"], b["mn"]),
         "s_calls": ratio(a["calls"], b["calls"]),
         "size_ratio": sz}
    ca = Counter(c for c in a["calls"] if not c.startswith("game:"))
    cb = Counter(c for c in b["calls"] if not c.startswith("game:"))
    r["calls_common"] = sorted((ca & cb).elements())
    r["imports_common"] = sorted(a["imports"] & b["imports"])
    r["strings_common"] = sorted(a["strings"] & b["strings"])
    r["consts_common"] = sorted((a["consts"] & b["consts"]).keys())
    r["floats_common"] = sorted(a["floats"] & b["floats"])
    r["game_calls_common"] = sorted(
        (Counter(c for c in a["calls"] if c.startswith("game:")
                 and not c.endswith("?")) &
         Counter(c for c in b["calls"] if c.startswith("game:")
                 and not c.endswith("?"))).elements())
    spec = [c for c in set(r["calls_common"])
            if c.split(":", 1)[1] not in TRIVIAL_IMPORTS]
    spec = [c for c in spec if not c.startswith("fptr:")] + \
        [c for c in spec if c.startswith("fptr:")]
    # "Content" evidence: specific to the code and not just to the idiom
    # (fopen/fwrite/fclose or malloc/free are found everywhere).
    r["content"] = (len(r["strings_common"]) + len(r["consts_common"]) +
                    len(r["floats_common"]) +
                    len(set(r["game_calls_common"])) +
                    len(set(r["imports_common"]) - TRIVIAL_IMPORTS))
    r["evidence"] = (len(spec) + len(r["strings_common"]) +
                     len(r["consts_common"]) + len(r["floats_common"]) +
                     len(set(r["game_calls_common"])) +
                     len(set(r["imports_common"]) - TRIVIAL_IMPORTS -
                         {c.split(":", 1)[1] for c in spec}))
    r["score"] = (0.35 * r["s_bytes"] + 0.15 * r["s_tok"] +
                  0.2 * r["s_mn"] + 0.2 * r["s_calls"] +
                  0.1 * min(r["evidence"], 5) / 5)
    if r["ident"]:
        r["conf"] = "identical"
    elif r["s_bytes"] >= CLOSE_THRESHOLD and r["s_calls"] >= 0.8 \
            and sz >= 0.7:
        r["conf"] = "very close"
    elif r["s_mn"] >= SIMILAR_THRESHOLD and r["evidence"] >= 2 \
            and r["content"] >= 1 and r["s_calls"] >= 0.4:
        r["conf"] = "similar"
    else:
        r["conf"] = None
    return r


def match(FA, FB):
    """Mutual best pairs (score), with the second best score."""
    S = {(a, b): compare(FA[a], FB[b]) for a in FA for b in FB}
    best_a = {a: max(FB, key=lambda b: S[a, b]["score"]) for a in FA}
    best_b = {b: max(FA, key=lambda a: S[a, b]["score"]) for b in FB}
    out = {}
    for a, b in best_a.items():
        if best_b[b] != a:
            continue
        r = dict(S[a, b])
        others = sorted((S[a, x]["score"] for x in FB if x != b),
                        reverse=True)[:1] + \
            sorted((S[y, b]["score"] for y in FA if y != a),
                   reverse=True)[:1]
        r["second"] = max(others) if others else 0.0
        out[a, b] = r
    return out, S


def lis(seq):
    """Longest increasing subsequence (indices)."""
    import bisect
    tails, idx, prev = [], [], [-1] * len(seq)
    for i, v in enumerate(seq):
        k = bisect.bisect_left(tails, v)
        if k == len(tails):
            tails.append(v)
            idx.append(i)
        else:
            tails[k] = v
            idx[k] = i
        prev[i] = idx[k - 1] if k else -1
    out, i = [], idx[-1] if idx else -1
    while i >= 0:
        out.append(i)
        i = prev[i]
    return out[::-1]


def main():
    exes = {x: Exe(x) for x in EXES}
    labels = crt_labels(exes)
    pair_of = {}
    PAIRS = (("happy.exe", "maped.exe"), ("happy.exe", "loader.exe"),
             ("maped.exe", "loader.exe"))
    for round_no in range(3):
        feats = {x: {s: features(e, s, labels, pair_of) for s in e.game}
                 for x, e in exes.items()}
        res, allS = {}, {}
        for x, y in PAIRS:
            res[x, y], allS[x, y] = match(feats[x], feats[y])
        new = {}
        k = 0
        for (x, y), m in res.items():
            for (a, b), r in sorted(m.items()):
                if r["conf"] in ("identical", "very close"):
                    pid = new.get((x, a)) or new.get((y, b)) or "p%d" % k
                    k += 1
                    new[x, a] = new[y, b] = pid
        if new == pair_of:
            break
        pair_of = new
    print("rounds:", round_no + 1)

    hm = res["happy.exe", "maped.exe"]
    hl = res["happy.exe", "loader.exe"]
    ml = res["maped.exe", "loader.exe"]
    rows = []
    used_l = {}
    for (h, m), r in hm.items():
        if not r["conf"]:
            continue
        lo = [l for (hh, l), rr in hl.items() if hh == h and rr["conf"]]
        lo2 = [l for (mm, l), rr in ml.items() if mm == m and rr["conf"]]
        l = lo[0] if lo and lo == lo2 else None
        if l:
            used_l[l] = True
        rows.append((h, m, l, r))
    for (h, l), r in hl.items():
        if r["conf"] and l not in used_l:
            rows.append((h, None, l, r))
            used_l[l] = True
    for (m, l), r in ml.items():
        if r["conf"] and l not in used_l:
            rows.append((None, m, l, r))
    rows.sort(key=lambda t: (t[0] or 0xFFFFFFFF, t[1] or 0))

    H, M, L = (exes[x] for x in EXES)

    def fmt(v):
        return "0x%08X" % v if v else ""

    def evidence_text(r):
        p = ["masked bytes %.2f" % r["s_bytes"],
             "tokens %.2f" % r["s_tok"], "mnemonics %.2f" % r["s_mn"],
             "calls %.2f" % r["s_calls"]]
        if r["calls_common"]:
            p.append("common calls: " + " ".join(
                c for c in sorted(set(r["calls_common"]))))
        if r["game_calls_common"]:
            p.append("paired game calls: %d"
                     % len(r["game_calls_common"]))
        if r["strings_common"]:
            p.append("strings: " + " | ".join(r["strings_common"][:6]) +
                     (" …" if len(r["strings_common"]) > 6 else ""))
        if r["consts_common"]:
            p.append("constants: " + " ".join(
                hex(c) for c in r["consts_common"][:10]))
        if r["floats_common"]:
            p.append("floats: " + " ".join(
                repr(c) for c in r["floats_common"]))
        p.append("2nd best score %.2f (chosen %.2f)"
                 % (r["second"], r["score"]))
        return " ; ".join(p)

    out = ROOT / "notes" / "data" / "shared-functions.csv"
    with open(out, "w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow(["happy", "maped", "loader", "size_happy", "size_maped",
                    "size_loader", "confidence", "block_happy", "block_maped",
                    "subclass", "s_bytes", "s_tokens", "s_mnemonics",
                    "s_calls", "evidence"])
        for h, m, l, r in rows:
            sub = (H.rows[h]["subclass"] if h else M.rows[m]["subclass"])
            w.writerow([fmt(h), fmt(m), fmt(l),
                        H.rows[h]["size"] if h else "",
                        M.rows[m]["size"] if m else "",
                        L.rows[l]["size"] if l else "",
                        r["conf"],
                        H.rows[h]["block"] if h else "",
                        M.rows[m]["block"] if m else "", sub,
                        "%.3f" % r["s_bytes"], "%.3f" % r["s_tok"],
                        "%.3f" % r["s_mn"], "%.3f" % r["s_calls"],
                        evidence_text(r)])
    print("wrote", out.relative_to(ROOT), len(rows), "rows")

    # Report
    for h, m, l, r in rows:
        print("%-10s %-10s %-10s %5s %5s %-12s %s" % (
            fmt(h), fmt(m), fmt(l), H.rows[h]["size"] if h else "",
            M.rows[m]["size"] if m else "", r["conf"], evidence_text(r)))

    # Closest rejected candidates (for the note)
    print("\n-- mutual best pairs without a level (happy×maped), "
          "score >= 0.45")
    for (h, m), r in sorted(hm.items(), key=lambda t: -t[1]["score"]):
        if not r["conf"] and r["score"] >= 0.45:
            print(fmt(h), fmt(m), H.rows[h]["size"], M.rows[m]["size"],
                  evidence_text(r))

    # Variants: unpaired function whose best candidate (already paired
    # elsewhere) reaches a level.
    print("\n-- variants (non-mutual best candidate with a level)")
    S = allS["happy.exe", "maped.exe"]
    paired_h = {h for h, m, l, r in rows if h}
    paired_m = {m for h, m, l, r in rows if m}
    variants = []
    for m in M.game:
        if m in paired_m:
            continue
        h = max(H.game, key=lambda h: S[h, m]["score"])
        if S[h, m]["conf"]:
            variants.append(("maped.exe", m, "happy.exe", h, S[h, m]))
    for h in H.game:
        if h in paired_h:
            continue
        m = max(M.game, key=lambda m: S[h, m]["score"])
        if S[h, m]["conf"]:
            variants.append(("happy.exe", h, "maped.exe", m, S[h, m]))
    for x, a, y, b, r in variants:
        print(x, fmt(a), "~", y, fmt(b), r["conf"], evidence_text(dict(r,
                                                                 second=0)))

    # Order
    print("\n-- order (happy×maped, pairs with a level)")
    pm = sorted((h, m, r["conf"]) for h, m, l, r in rows if h and m)
    seq = [m for _, m, _ in pm]
    keep = set(lis(seq))
    for i, (h, m, c) in enumerate(pm):
        print(fmt(h), H.rows[h]["block"], fmt(m), M.rows[m]["block"], c,
              "" if i in keep else "OUT OF ORDER")
    print("LIS %d / %d" % (len(keep), len(pm)))
    print("\n-- same list in maped order")
    for h, m, c in sorted(pm, key=lambda t: t[1]):
        print(fmt(m), M.rows[m]["block"], fmt(h), H.rows[h]["block"], c)

    js = {"pairs": [{"happy": fmt(h), "maped": fmt(m), "loader": fmt(l),
                     **{k: v for k, v in r.items()
                        if k not in ("calls_common",)},
                     "calls_common": r["calls_common"]}
                    for h, m, l, r in rows],
          "features": {x: {fmt(s): {"calls": v["calls"],
                                    "n_ins": v["n_ins"]}
                           for s, v in feats[x].items()} for x in EXES},
          "best_unmatched_hm": [
              {"happy": fmt(h), "maped": fmt(m), "score": r["score"],
               "s_bytes": r["s_bytes"], "s_mn": r["s_mn"],
               "s_calls": r["s_calls"], "evidence": r["evidence"]}
              for (h, m), r in hm.items() if not r["conf"]],
          "variants": [{"exe": x, "addr": fmt(a), "best_in": y,
                        "best": fmt(b), "conf": r["conf"],
                        "score": r["score"], "s_bytes": r["s_bytes"],
                        "s_mn": r["s_mn"], "s_calls": r["s_calls"]}
                       for x, a, y, b, r in variants]}
    (ROOT / "captures" / "shared-code.json").write_text(
        json.dumps(js, indent=1, ensure_ascii=False, default=str),
        encoding="utf-8")


if __name__ == "__main__":
    main()
