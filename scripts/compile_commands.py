"""Writes compile_commands.json at the root of the repository, for clangd.

clangd parses src/ with clang, not with VC6: the flags emulate what cl sees
(32-bit MSVC target, Microsoft extensions, VC6 headers instead of the system
ones, the /I and /D of scripts/build.py). It only serves editing (completion,
navigation, diagnostics); matching is still checked with build.py + reccmp.

One entry per source of build.SOURCES. src/common/ is compiled into happy and
maped; clangd keeps one command per file, so the first exe of SOURCES wins
(HAPPY): the #ifdef MAPED parts of src/common/ are shown as inactive.

The file holds absolute paths (repository, VC6 root found as in vc6.py): it is
per machine and ignored by git. Rerun it after adding a source file.

Usage: python scripts/compile_commands.py
"""
import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from build import ALLEGRO, ROOT, SOURCES, SRC  # noqa: E402
from vc6 import VC6  # noqa: E402

FLAGS = [
    "--target=i686-pc-windows-msvc",
    "-fms-extensions",
    "-fms-compatibility",
    "-fms-compatibility-version=12.00",  # _MSC_VER 1200, as cl 12.00.8168
    "-std=c89",
    # VC6 headers only: never those of macOS or of the Nix toolchain
    "-nostdinc",
    # The VC6 CD names its headers in capitals (STDIO.H)
    "-Wno-nonportable-include-path",
]


def main():
    vc = VC6()
    entries = []
    seen = set()
    for exe, sources in SOURCES.items():
        for s in sources:
            src = SRC / s
            if s in seen or not src.exists():
                continue
            seen.add(s)
            args = ["clang"] + FLAGS + [
                "-isystem", str(vc.include),
                "-I", str(SRC / "include"),
                "-I", str(SRC / exe),
                "-I", str(ALLEGRO / "include"),
                "-D" + exe.upper(),
                "-c", str(src)]
            entries.append({"directory": str(ROOT), "file": str(src),
                            "arguments": args})
    out = ROOT / "compile_commands.json"
    out.write_text(json.dumps(entries, indent=2) + "\n")
    print("%s: %d sources" % (out.relative_to(ROOT), len(entries)))


if __name__ == "__main__":
    main()
