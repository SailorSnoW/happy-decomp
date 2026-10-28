# happy-decomp

Matching decompilation of **Happyland Adventures – X-Mas Edition** v1.3 (Johan Peitz / Free Lunch Design, 2003), a freeware Windows game written in C with [Allegro 4.0.3](https://liballeg.org/) and compiled with Visual C++ 6.0.

The C89 in `src/` recompiles with VC6 to the same machine code as the original executables, function by function, as verified with [reccmp](https://github.com/isledecomp/reccmp):

| exe | role | game functions | at 100% |
|---|---|---:|---:|
| `happy.exe` | the game | 84 | 84 |
| `maped.exe` | the map editor | 50 | 50 |
| `loader.exe` | the launcher (by Eric Love) | 2 | 2 |

Global variables match too (`reccmp-datacmp`: 0 issues), and the rebuilt `loader.exe` has `.text`, `.rdata` and `.data` byte-identical to the original.

**No game file is included.** You need your own copy of the game, v1.3 (see [Requirements](#requirements)).

## Requirements

- **The game**, copied into `game/` (ignored by git). The files must have these SHA-256 hashes (also listed in `reccmp-project.yml`):

  | file | SHA-256 |
  |---|---|
  | `happy.exe` | `c42e561678e5b968c5110e05b7e81eb28581b81bbdebaca81aa3c68502a5f4ba` |
  | `maped.exe` | `789ee11e62e1a2b1241f60e3a060f0bd2ac926d15a6762e6b2b31946cac74f92` |
  | `loader.exe` | `4814e49733f8b4c7b7050f0898b824fe58fcaf8d23930d8e96d00410eee74fd4` |
  | `alleg40.dll` | `11ff7d4ce9fb85ca03a15c8a4680fdd467f18daf346bf0c07ecc5933135cbfe3` |

- **Visual C++ 6.0 without any service pack** (cl 12.00.8168, link 6.00.8168): CD 1 of Visual Studio 6.0, copied (not installed) into `../tools/vc6` or anywhere pointed to by `VC6_ROOT`. Not included (Microsoft licence). On macOS it runs under Wine; `flake.nix` provides a dev shell with Wine, Python and uv.
- **Python 3.12**. Every script uses only the standard library.
- **reccmp 0.1.7** (`pip install reccmp==0.1.7`, or `uv tool install reccmp==0.1.7`).
- Optional: **Ghidra 12.1.4** with the GhidraMCP 7.0.0 extension, to browse the binaries with the names and types of `src/`.

Step-by-step setup for Windows and macOS Apple Silicon: `.claude/skills/dev-setup/` (also usable as a Claude Code skill). `python scripts/doctor.py` checks the whole toolchain.

## Build and verify

```sh
python scripts/doctor.py                 # check the toolchain
python scripts/build.py --final          # build/happy/happy.exe, build/maped/maped.exe, build/loader/loader.exe (+ .pdb)
python scripts/progress.py               # compare every game function with reccmp
```

`reccmp-user.yml` (paths of your originals, per machine) is needed once; see `notes/matching.md` §3. To compare one function in detail: `python scripts/reccmp_run.py --target HAPPY --verbose 0x00406c80`. `python scripts/doctor.py --deep` runs the build and the comparison, and expects every function at 100%.

The rebuilt exes are linked with `/DEBUG` for reccmp, so the files are not byte-identical to the originals even though the code is; to run one, put it next to the game's `alleg40.dll` and data.

## Layout

| Path | Contents |
|---|---|
| `src/common/` | Source files linked into both happy and maped (map module, shared utilities). |
| `src/happy/`, `src/maped/`, `src/loader/` | One `block_NN.c` per original object file, in link order; `data.c` holds the globals; `<exe>.h` the shared declarations. |
| `src/include/` | Headers shared between the exes. |
| `src/crt/` | Names of the statically linked C library functions, for reccmp only (never compiled). |
| `3rdparty/allegro-4.0.3/` | The official Allegro 4.0.3 headers and `.def`, configured for MSVC. |
| `reccmp-project.yml`, `reccmp/` | reccmp project: the three targets and the addresses of floating-point constants. |
| `scripts/` | `build.py`, `progress.py`, `reccmp_run.py` (reccmp wrapper), `vc6.py` (runs VC6, natively or under Wine), `doctor.py`, `compile_commands.py` (clangd), `ghidra_sync.py` and `ghidra_rest.py` (Ghidra). |
| `notes/` | Documentation, below. |

### Reading the code

- Each function is preceded by its original address: `// FUNCTION: HAPPY 0x00406c80` (one line per exe for the shared code). Globals: `// GLOBAL:`, string literals: `// STRING:`, library code: `// LIBRARY:`.
- Original names are unknown. A function or global gets a descriptive name only when the code justifies it, and the comment marks it `[hypothesis]` with the evidence; otherwise it keeps its address (`FUN_00401160`, `DAT_00413b10`). Source files keep neutral names (`block_05.c`), with their theme in a header comment.
- Some C is written in an unusual way because only that form gives VC6's original output; changing it will usually break the match. `notes/matching.md` §3a lists those shapes.

## Documentation

| Note | Contents |
|---|---|
| `notes/STATUS.md` | Final state and open questions. |
| `notes/matching.md` | How the build and the comparison work, reccmp pitfalls, VC6 code-generation shapes, Ghidra sync, CRT names. |
| `notes/layout-happy.md`, `layout-maped.md`, `layout-loader.md` | Map of each exe: sections, data, blocks (source files) and their functions. |
| `notes/game-docs.md` | The game: options, file formats, limits; `notes/data/datafiles.csv` lists the objects of the datafiles. |
| `notes/allegro.md` | Allegro 4.0.3 as used by the exes: imports, inline functions recognisable in the code. |
| `notes/toolchain-vc6.md` | The compiler and linker, and why this exact version. |
| `notes/legal.md` | Rights holders and what this repository may contain. |

## Credits

- **Happyland Adventures** and its map editor: Johan Peitz / Free Lunch Design (code, Christmas graphics), levels by Johan Peitz and Mark Hickford, graphics from **Ari Feldman's SpriteLib**.
- **loader.exe**: Eric Love.
- The pixel-perfect collision routine follows **Ivan Baldo's PPCol** (credited in the game's `readme.txt`).
- **Allegro**: the Allegro developers (gift-ware).
- **reccmp**: the isledecomp project.

This project is not affiliated with or endorsed by any of the authors above.

## Legal

The game is freeware; its licence allows distributing it only in its original form. This repository contains no file of the game: no executable, data, map or graphics. Each user supplies their own copy.

`src/` is a reconstruction of the original programs' source code and therefore remains a derived work of their authors (Johan Peitz for happy and maped, Eric Love for loader): **no licence is granted on `src/`**. `3rdparty/` keeps Allegro's own terms (gift-ware, `3rdparty/allegro-4.0.3/readme.txt`). The rest of the repository (scripts, notes, configuration) is under the MIT licence: see `LICENSE`.

If you are a rights holder and want this repository changed or taken down, please open an issue.
