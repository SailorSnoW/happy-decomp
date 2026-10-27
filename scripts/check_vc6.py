"""Checks the local VC6 toolchain against the game's exe files (read-only on game/).

1. Versions of cl, link, lib and cvtres.
2. Compiles and links test programs (CRT: WinMain entry point, stdio, floating
   point, old names) in captures/vc6-check/, then:
   - reads their Rich header (does the linker record itself there? what do
     AliasObj and Linker512 count?);
   - looks for the CRT functions of happy.exe (>= 16 bytes) in the test exe
     files, up to masked addresses (inventory.py signatures), then, for those
     that are missing, compares the body instruction by instruction while
     ignoring addresses;
   - reads the flags of the .text sections of an .obj compiled with /O2
     (COMDAT = /Gy, alignment) and any inline expansion of small functions.

Usage: .venv/Scripts/python scripts/check_vc6.py [VC6 root]
(macOS: .venv/bin/python; tools run through Wine.) Default root: see vc6.py.
"""
import csv
import pathlib
import re
import struct
import sys

import capstone

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from inventory import Image, Inventory, find_in  # noqa: E402
from pe_allegro import PE  # noqa: E402
from vc6 import VC6  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parent.parent
VC = VC6(sys.argv[1] if len(sys.argv) > 1 else None)
OUT = ROOT / "captures" / "vc6-check"

RICH = {1: "Import0", 4: "Linker600", 6: "Cvtres500", 10: "Utc12_C",
        11: "Utc12_CPP", 12: "AliasObj60", 14: "Masm613", 19: "Linker512"}

TESTS = {
    "t_crt.c": r'''
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <ctype.h>

static void bye(void) { }

int WINAPI WinMain(HINSTANCE h, HINSTANCE p, LPSTR cmd, int show)
{
    char buf[64];
    char *m;
    FILE *f;
    double d;
    int i;

    atexit(bye);
    m = malloc(atoi(cmd) + 16);
    d = atof(cmd) * 0.0625;
    i = (int)d;
    sprintf(buf, "%d %s %f %g %e", rand(), cmd, d, d, d);
    _itoa(i, buf + 40, 10);
    f = fopen(buf, "rb");
    if (f != NULL) {
        fread(m, 1, 4, f);
        fwrite(m, 1, 4, f);
        fseek(f, 0, SEEK_END);
        fclose(f);
    }
    _chdir(buf);
    if (_strcmpi(buf, "x") == 0)
        free(m);
    return strlen(buf) + toupper(buf[0]) + atol(cmd) + i;
}
''',
    "t_oldnames.c": r'''
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>

int WINAPI WinMain(HINSTANCE h, HINSTANCE p, LPSTR cmd, int show)
{
    char buf[16];
    itoa(show, buf, 10);
    chdir(buf);
    return strcmpi(buf, cmd);
}
''',
    "t_comdat.c": r'''
int g;
int f1(int a) { return a * 3 + g; }
int f2(int a) { if (a) g++; return g; }
int f3(int a, int b) { return a / b; }
int main(void) { return f1(g) + f2(g) + f3(g, 7); }
''',
}


def run(args, cwd=OUT):
    return VC.run(args, cwd=cwd)


def banner(tool):
    _, out = run([tool])
    m = re.search(r"Version ([\d.]+)", out)
    return m.group(1) if m else "?"


def rich(path):
    return ", ".join("%s %d ×%d" % (RICH.get(p, "id%d" % p), b, n)
                     for p, b, n in PE(path).rich())


def masked(data):
    """Zeroes every dword that falls in [0x00400000, 0x00500000)."""
    b = bytearray(data)
    for i in range(len(b) - 3):
        if 0x00400000 <= struct.unpack_from("<I", b, i)[0] < 0x00500000:
            b[i:i + 4] = b"\0\0\0\0"
    return bytes(b)


def body_equal_but_addresses(a_img, a, b_img, b, n):
    """Compares n bytes of instructions, ignoring address operands.
    Stops at the first jump table (data): returns the number of instructions
    compared, or None at the first real difference."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    xs = list(md.disasm(a_img.read(a, n), a))
    ys = list(md.disasm(b_img.read(b, n + 64), b))
    k = 0
    for x, y in zip(xs, ys):
        if x.mnemonic != y.mnemonic or x.size != y.size:
            return k if k > len(xs) // 2 else None
        if x.bytes != y.bytes and not re.search(r"0x4[0-9a-f]{5}", x.op_str):
            return None
        k += 1
    return k


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    print("== Versions")
    for t in ("CL.EXE", "LINK.EXE", "LIB.EXE", "CVTRES.EXE"):
        print("  %-10s %s" % (t, banner(t)))

    print("== Compiling the tests (/O2)")
    for name, src in TESTS.items():
        (OUT / name).write_text(src.lstrip(), encoding="ascii")
        stem = name[:-2]
        rc, out = run(["cl", "/nologo", "/O2", "/c", name])
        if rc:
            sys.exit(out)
        sub = [] if "comdat" in stem else ["/subsystem:windows"]
        rc, out = run(["link", "/nologo", "/map:%s.map" % stem, stem + ".obj",
                       "kernel32.lib"] + sub)
        if rc:
            sys.exit(out)
        print("  %-16s Rich: %s" % (stem + ".exe", rich(OUT / (stem + ".exe"))))
    for e in ("happy.exe", "maped.exe", "loader.exe"):
        print("  %-16s Rich: %s" % (e, rich(ROOT / "game" / e)))

    print("== CRT of happy.exe found in the tests")
    inv = Inventory("happy.exe")
    inv.analyse()
    rows = {int(r["address"], 16): r for r in csv.DictReader(
        open(ROOT / "notes/data/functions-happy.csv", encoding="utf-8"))}
    crt = {s: inv.sig(s) for s in inv.ent
           if rows.get(s, {}).get("class") == "CRT" and inv.F[s]["size"] >= 16}
    imgs = [Image(PE(OUT / (n[:-2] + ".exe"))) for n in TESTS
            if "comdat" not in n]
    hit = set()
    for i, img in enumerate(imgs):
        hit |= set(find_in(img, crt, "t%d" % i))
    print("  identical signatures: %d / %d" % (len(hit), len(crt)))
    for s in sorted(set(crt) - hit):
        n = inv.F[s]["size"]
        head = inv.img.read(s, 12)
        found = None
        mine = masked(inv.img.read(s, n))
        for img in imgs:
            ts, te, _, _ = img.secs[".text"]
            text = img.read(ts, te - ts)
            k = masked(text).find(mine)
            if k >= 0:
                found = (ts + k, "whole body, masked bytes")
                break
            k = text.find(head)
            while k >= 0 and found is None:
                c = body_equal_but_addresses(inv.img, s, img, ts + k, n)
                if c:
                    found = (ts + k, "%d instructions" % c)
                k = text.find(head, k + 1)
        name = rows[s]["proposed_name"] or rows[s]["ghidra_name"]
        print("  0x%08X %5d B %-14s %s" % (
            s, n, name, "identical except for addresses (%s, "
            "found at 0x%08X)" % (found[1], found[0]) if found
            else "NOT FOUND"))

    print("== /O2: .text sections of t_comdat.obj")
    b = (OUT / "t_comdat.obj").read_bytes()
    nsec = struct.unpack_from("<H", b, 2)[0]
    for i in range(nsec):
        o = 20 + 40 * i
        name = b[o:o + 8].rstrip(b"\0").decode()
        flags = struct.unpack_from("<I", b, o + 36)[0]
        if name == ".text":
            print("  .text flags 0x%08X: COMDAT=%s, alignment %d" % (
                flags, bool(flags & 0x1000), 1 << (((flags >> 20) & 0xF) - 1)))
    for line in (OUT / "t_comdat.map").read_text(errors="replace").splitlines():
        if re.search(r" _(f[123]|main) ", line):
            print("  " + line.strip())


if __name__ == "__main__":
    main()
