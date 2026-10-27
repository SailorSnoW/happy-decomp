"""Runs the VC6 tools (cl, link, lib, dumpbin...) on Windows, or through Wine elsewhere.

VC6 root (copy of CD 1 of Visual Studio 6.0 Enterprise, outside the repository),
in this order:
1. VC6_ROOT environment variable;
2. <repo>/tools/vc6 (ignored by git);
3. <repo>/../tools/vc6 (C:\\RE\\tools\\vc6 when the repository is C:\\RE\\happy-decomp).

On macOS and Linux, every tool goes through `wine`. Paths given to the tools must
then be relative to the working directory, or converted by VC6.path() (Wine's
Z: drive). INCLUDE and LIB are set as Windows paths, WINEPATH gives access to
MSPDB60.DLL (COMMON\\MSDEV98\\BIN).

Usage as a module:
    from vc6 import VC6
    vc = VC6()
    rc, out = vc.run(["cl", "/nologo", "/O2", "/c", "t.c"], cwd=folder)
Usage from the command line (exit code of the tool):
    python scripts/vc6.py cl /nologo /O2 /c t.c
"""
import os
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
NATIVE = os.name == "nt"


def find_root():
    """Returns the VC6 root found, or None."""
    cands = []
    if os.environ.get("VC6_ROOT"):
        cands.append(pathlib.Path(os.environ["VC6_ROOT"]))
    cands += [ROOT / "tools" / "vc6", ROOT.parent / "tools" / "vc6"]
    for c in cands:
        if (c / "VC98" / "BIN").is_dir():
            return c.resolve()
    return None


def _child(d, name):
    """Child of d, case-insensitively (lowercase copies of the CD)."""
    p = d / name
    if p.exists() or not d.is_dir():
        return p
    for q in d.iterdir():
        if q.name.lower() == name.lower():
            return q
    return p


class VC6:
    def __init__(self, root=None):
        root = pathlib.Path(root) if root else find_root()
        if root is None:
            raise FileNotFoundError(
                "VC6 not found: set VC6_ROOT or copy the CD into "
                "%s or %s" % (ROOT / "tools" / "vc6", ROOT.parent / "tools" / "vc6"))
        self.root = root
        self.bin = _child(_child(root, "VC98"), "BIN")
        self.include = _child(_child(root, "VC98"), "INCLUDE")
        self.lib = _child(_child(root, "VC98"), "LIB")
        self.common = _child(_child(_child(root, "COMMON"), "MSDEV98"), "BIN")

    def path(self, p):
        """Path as the VC6 tools must receive it."""
        p = pathlib.Path(p).resolve()
        if NATIVE:
            return str(p)
        return "Z:" + str(p).replace("/", "\\")

    def exe(self, tool):
        return _child(self.bin, tool.upper() + ("" if tool.lower().endswith(".exe")
                                                 else ".EXE"))

    def env(self):
        e = dict(os.environ)
        e["INCLUDE"] = self.path(self.include)
        e["LIB"] = self.path(self.lib)
        if NATIVE:
            e["PATH"] = os.pathsep.join([str(self.bin), str(self.common), e["PATH"]])
        else:
            e["WINEPATH"] = ";".join([self.path(self.bin), self.path(self.common)])
            e.setdefault("WINEDEBUG", "-all")
            # No prompt to install Mono or Gecko on first launch
            e.setdefault("WINEDLLOVERRIDES", "mscoree,mshtml=")
        return e

    def command(self, args):
        # CreateProcess does not look the executable up in the child's PATH:
        # always pass the full path of the tool.
        cmd = [str(self.exe(args[0]))] + list(args[1:])
        return cmd if NATIVE else ["wine"] + cmd

    def run(self, args, cwd=None):
        """Runs a tool; returns (exit code, standard output + errors)."""
        p = subprocess.run(self.command(args), cwd=cwd, env=self.env(),
                           capture_output=True, text=True, errors="replace")
        return p.returncode, p.stdout + p.stderr


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    vc = VC6()
    sys.exit(subprocess.run(vc.command(sys.argv[1:]), env=vc.env()).returncode)
