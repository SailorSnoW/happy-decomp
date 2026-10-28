# happy-decomp project

## Target
- Windows game from 2003 (Happyland Adventures – X-Mas Edition v1.3), written in C, compiled with Visual C++ 6.0 (cl 12.00.8168, link 6.00; `notes/toolchain-vc6.md`).
- Original binaries in `game/` (never versioned): `happy.exe` (the game, 84 KB), `maped.exe` (map editor, 72 KB), `loader.exe` (launcher, 32 KB).
- `game/alleg40.dll`: Allegro 4.0.3, MSVC build, UPX-packed. Not decompiled. Its headers and `.def` are versioned in `3rdparty/allegro-4.0.3/` (`notes/allegro.md`).
- VC6 C library (CRT) statically linked into the exes. Not decompiled: annotated as `LIBRARY` in `src/crt/`.

## Goal and state: complete
"Matching" decompilation: C89 that, recompiled with VC6, produces the same machine code function by function, verified with reccmp. This is the end goal (decided with the user on 2026-10-28: no port to modern C).

Done on 2026-10-28: 136/136 game functions at 100% (happy 84, maped 50, loader 2), `reccmp-datacmp` 0 issues. Final state and open questions: `notes/STATUS.md`. User-facing overview: `README.md`.

Any change to `src/` must keep every function at 100%: `python scripts/build.py --final` then `scripts/progress.py` (or `python scripts/doctor.py --deep`). One function in detail: `python scripts/reccmp_run.py --target HAPPY --verbose 0x...`. Before rewriting a function into a "cleaner" form, read `notes/matching.md` §3a: many odd-looking forms are what makes the match.

## Tools
- VC6 (CD 1 of Visual Studio 6.0 Enterprise), outside the repository: `../tools/vc6` or `VC6_ROOT`. `scripts/vc6.py` runs the tools with the right environment, natively on Windows or through Wine on macOS. From Git Bash, export `MSYS2_ARG_CONV_EXCL='*'`.
- Allegro 4.0.3 in `3rdparty/allegro-4.0.3/`: `include/` and `lib/msvc/allegro.def` copied byte for byte from `all403.zip`; only `alplatf.h` is changed (`#define ALLEGRO_MSVC`). Provenance and SHA-256: `3rdparty/allegro-4.0.3/README.md`. `.gitattributes` keeps their CRLF.
- reccmp **0.1.7**, outside any venv (Windows: global Python 3.12; macOS: `uv tool install`, run its scripts with `.uv/tools/reccmp/bin/python`). Project: `reccmp-project.yml` (3 targets, `source_root: src`); `reccmp-user.yml` and `reccmp-build.yml` are per machine. **`reccmp-ghidra-import` writes to Ghidra**: do not use it without explicit approval.
- Setting up a machine: skill `dev-setup` (`.claude/skills/dev-setup/`). Diagnostics: `python scripts/doctor.py [--deep]`.
- Scripts (`scripts/`, standard library only):
  - `build.py [--final] [happy] [maped] [loader]`: compiles `src/`, links `build/<exe>/<exe>.exe` + `.pdb` in the order of the blocks, runs `reccmp-project detect`. `/OPT:NOREF` by default, `--final` for `/OPT:REF` (as the original);
  - `progress.py [exe...]`: reccmp on every game function, writes `notes/data/progress-<exe>.csv`;
  - `reccmp_run.py`: reccmp-reccmp 0.1.7 patched at run time: operands at the same offset of one matched variable (pointers past the end of an array) compare equal (decided with the user on 2026-10-27, `notes/matching.md` §3); `--audit` lists every line concerned;
  - `ghidra_sync.py [--apply | --save] [exe...]`: carries the names and types of `src/` into Ghidra (dry run by default); from a fresh import, `--save happy maped loader` rebuilds the reference project (`notes/matching.md` §3b);
  - `ghidra_rest.py`: read-only client of the GhidraMCP HTTP API, for ad-hoc scripts;
  - `compile_commands.py`: `compile_commands.json` for clangd (editing only);
  - `doctor.py`, `vc6.py`: diagnostics, VC6 launcher.

## Ghidra MCP (optional)
- Ghidra **12.1.4** with GhidraMCP 7.0.0 (bridge `bridge-mcp-ghidra`, stdio, to `http://127.0.0.1:8089`). **Do not upgrade to Ghidra 12.2**: decompilation fails there (`ghidra/program/model/lang/SleighLanguageDescription`, observed on 2026-10-27).
- Always pass `program="happy.exe"` (or `maped.exe`, `loader.exe`); calls without it are refused. Do not use `switch_program`.
- Tools come in groups; `function`, `listing` and `program` are loaded at startup, others through `search_tools` / `load_tool_group` (`xref`, `analysis`, `documentation`). Never load `all`. `get_functions` batches up to 20 functions; restrict `fields=` and set `include_call_context=false`. Its `disassembly` field stops at 200 instructions: use `disassemble_function` for a full listing.
- Reference counts (`find_functions`) after `ghidra_sync.py --save`: happy 223, maped 209, loader 64 (a fresh import gives 220 / 200 / 63).
- **Writes**: only what `scripts/ghidra_sync.py` does (renames, prototypes, data types, `create_function` for annotated functions the analysis missed, `import_data_types`, resetting generated data items that block a type), then `save_program` once the auto-analysis is idle. Every other write tool (other `create_*`, `delete_*`, comments, tags, bookmarks, `clear_flow_and_repair`, `reanalyze`, `import_file`, `close_program`, `exit_ghidra`…) is forbidden unless explicitly requested. `run_script_inline` and `run_ghidra_script` stay disabled server-side.
- For bulk reads, use `scripts/ghidra_rest.py` (only `read_only` GETs) rather than many MCP calls.

## Rules
- Never modify anything in `game/`. Never commit game files, `build/`, `captures/` or the Ghidra project (`notes/legal.md`).
- Every claim about the binary cites its address (`0x00401A30`) or its source (file, string, import).
- Explicitly distinguish **[verified]** (directly observed: string, import, bytes, xref) and **[hypothesis]** (deduced). Never present a hypothesis as a fact.
- Do not invent function or source file names. A proposed name remains a hypothesis with its justification.
- Naming in `src/` (decided on 2026-10-27): source files keep neutral names (`block_05.c`, after the blocks of `notes/layout-*.md` §5), with the theme in a header comment. Functions and globals get a descriptive name when the code justifies one, with a comment that tags it `[hypothesis]` and gives the evidence; otherwise `FUN_<address>` / `DAT_<address>`. Names in Ghidra follow those of `src/`.
- In case of an ambiguity that changes the result, stop and ask the question.

## Output files
- `notes/`: one Markdown file per topic, in English. Tabular data as CSV in `notes/data/`.
- `notes/STATUS.md`: to be updated at the end of each task that changes the state or the open questions.
- One commit per coherent change, with the reccmp scores in the message when `src/` changes.

## Decompilation rules
- C89, annotations `// FUNCTION: HAPPY 0x...` (one line per exe for the code in `src/common/`), `// GLOBAL:`, `// STRING:`, `// LIBRARY:`. Pitfalls of the annotations: `notes/matching.md` §3.
- Layout of `src/`: `common/` (compiled into happy and maped, happy-only parts under `#ifdef HAPPY`), `happy/`, `maped/`, `loader/` (one `block_NN.c` per block, functions in address order, `data.c` for the globals, `<exe>.h` for the shared declarations), `include/` (headers shared between exes), `crt/` (`LIBRARY` annotations, never compiled).
- No inline assembly in the game code, no artificial tricks to force a match. Exception (decided on 2026-10-27): the official Allegro 4.0.3 headers are included as is, including their inline assembly (`al386vc.h`: `fixmul`, `fixdiv`…), MSVC configuration without `ALLEGRO_NO_ASM`. Code coming from these headers (e.g. `0x00402110`) is annotated as library code.
- Compiler and options (chosen on 2026-10-27): **VC 6.0 without service pack, `cl /O2`**, no `/G` option; `/O2` implies `/Gy`. Compile with `/Zi`, link with `/DEBUG /INCREMENTAL:NO`: the code and addresses do not change. `notes/toolchain-vc6.md`, `notes/matching.md` §2.
