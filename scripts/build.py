"""Builds the recompiled happy.exe, maped.exe and loader.exe with VC6.

For each target: cl /O2 /Zi on each source of SOURCES that exists, then
link /DEBUG /INCREMENTAL:NO in the order of SOURCES, which is the order of the
.obj files in the original (notes/layout-*.md §5). Output:
build/<exe>/<exe>.exe and .pdb, the names reccmp-project detect looks for.

/OPT:NOREF by default: a decompiled function whose callers are still stubs is
referenced by nothing, and /OPT:REF would drop it from the exe. --final links
with /OPT:REF, as the original was [hypothesis: the shared source files keep
only a subset of their functions in each exe, notes/layout-maped.md §6]; the
linker option does not change the code (notes/toolchain-vc6.md §6).

Each exe is compiled with its name defined (/DHAPPY, /DMAPED, /DLOADER):
src/common/ leaves out the functions of the other exe with #ifdef
(notes/matching.md §2). The macro changes no code.

Functions that are not decompiled yet are still called: the symbols the link
reports as unresolved get an empty definition in build/<exe>/stubs.c
(generated, never annotated), then the link is redone. The exe is therefore
not runnable while stubs remain; reccmp only compares annotated functions.

Finally runs `reccmp-project detect --what recompiled`, which writes
reccmp-build.yml (per machine, ignored by git).

Usage: python scripts/build.py [--final] [happy] [maped] [loader]
(macOS: python3; the VC6 tools run through Wine.)
"""
import pathlib
import re
import shutil
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from vc6 import VC6  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "src"
BUILD = ROOT / "build"
ALLEGRO = ROOT / "3rdparty" / "allegro-4.0.3"

# Link order. Missing files are skipped, so that blocks can be added one at a
# time. data.c holds the globals until their owner .obj is known.
SOURCES = {
    "happy": ["common/block_01.c"]
    + ["happy/block_%02d.c" % i for i in range(2, 9)]
    + ["common/block_09.c", "happy/data.c"],
    "maped": ["common/block_09.c", "maped/block_02.c", "common/block_01.c"]
    + ["maped/block_%02d.c" % i for i in range(4, 8)] + ["maped/data.c"],
    "loader": ["loader/block_01.c", "loader/data.c"],
}

CFLAGS = ["/nologo", "/O2", "/Zi", "/W3", "/c"]
LFLAGS = ["/nologo", "/SUBSYSTEM:WINDOWS", "/DEBUG", "/INCREMENTAL:NO", "/MAP"]
UNRESOLVED = re.compile(r"unresolved external symbol (\S+)")


def check(what, rc, out):
    if rc != 0:
        print(out.strip())
        sys.exit("== %s: FAILED (code %d)" % (what, rc))


def alleg40_lib(vc):
    lib = BUILD / "alleg40.lib"
    if not lib.exists():
        BUILD.mkdir(exist_ok=True)
        check("LIB /DEF", *vc.run([
            "lib", "/NOLOGO",
            "/DEF:" + vc.path(ALLEGRO / "lib" / "msvc" / "allegro.def"),
            "/MACHINE:IX86", "/NAME:alleg40.dll", "/OUT:alleg40.lib"], cwd=BUILD))
    return lib


def stub(symbol):
    """C definition for a decorated cdecl or stdcall symbol, else None."""
    m = re.fullmatch(r"_([A-Za-z_]\w*)@(\d+)", symbol)
    if m:
        n = int(m.group(2)) // 4
        args = ", ".join("int a%d" % i for i in range(n)) or "void"
        return "void __stdcall %s(%s) {}" % (m.group(1), args)
    m = re.fullmatch(r"_([A-Za-z_]\w*)", symbol)
    if m:
        return "void %s(void) {}" % m.group(1)
    return None


def build(vc, exe, lib, opt):
    sources = [s for s in SOURCES[exe] if (SRC / s).exists()]
    if not sources:
        print("== %s: no source, skipped" % exe)
        return False
    out = BUILD / exe
    if out.exists():
        shutil.rmtree(out)
    (out / "obj").mkdir(parents=True)
    inc = ["/I", vc.path(SRC / "include"), "/I", vc.path(SRC / exe),
           "/I", vc.path(ALLEGRO / "include"), "/D" + exe.upper()]

    def cl(src, obj):
        rc, text = vc.run(["cl"] + CFLAGS + inc + [
            "/Fo" + vc.path(obj), "/Fd" + vc.path(out / "vc60.pdb"),
            vc.path(src)], cwd=out)
        # cl echoes the file name; keep warnings and errors only
        lines = [ln for ln in text.splitlines()
                 if ln.strip() and ln.strip() != src.name]
        if lines:
            print("\n".join(lines))
        check("cl " + src.name, rc, "")

    objs = []
    for s in sources:
        obj = out / "obj" / (s.replace("/", "_")[:-2] + ".obj")
        cl(SRC / s, obj)
        objs.append(obj)

    def link(extra):
        return vc.run(["link"] + LFLAGS + [opt,
            "/OUT:%s.exe" % exe, "/PDB:%s.pdb" % exe]
            + [vc.path(o) for o in objs + extra]
            + [vc.path(lib), "kernel32.lib"], cwd=out)

    rc, text = link([])
    stubs = []
    if rc != 0:
        symbols = sorted(set(UNRESOLVED.findall(text)))
        bad = [s for s in symbols if stub(s) is None]
        if not symbols or bad:
            check("link %s.exe (unresolved: %s)" % (exe, " ".join(bad)), rc, text)
        stubs = symbols
        (out / "stubs.c").write_text(
            "/* Generated by scripts/build.py: functions not decompiled yet. */\n"
            + "".join(stub(s) + "\n" for s in symbols), encoding="ascii")
        cl(out / "stubs.c", out / "obj" / "stubs.obj")
        rc, text = link([out / "obj" / "stubs.obj"])
        check("link %s.exe" % exe, rc, text)
    print("== %s.exe: %d source(s), %d stub(s)" % (exe, len(sources), len(stubs)))
    return True


def main():
    vc = VC6()
    lib = alleg40_lib(vc)
    args = sys.argv[1:]
    opt = "/OPT:REF" if "--final" in args else "/OPT:NOREF"
    exes = [a for a in args if a != "--final"] or list(SOURCES)
    for e in exes:
        if e not in SOURCES:
            sys.exit("unknown target: %s (%s)" % (e, ", ".join(SOURCES)))
    built = [e for e in exes if build(vc, e, lib, opt)]
    detect = shutil.which("reccmp-project")
    if detect is None:
        sys.exit("== reccmp-project not found in PATH")
    p = subprocess.run([detect, "detect", "--what", "recompiled", "--search-path"]
                       + [str(BUILD / e) for e in built], cwd=ROOT,
                       capture_output=True, text=True, errors="replace")
    check("reccmp-project detect", p.returncode, p.stdout + p.stderr)


if __name__ == "__main__":
    main()
