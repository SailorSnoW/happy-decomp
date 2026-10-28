\# happy-decomp project



\## Target

\- Windows game from 2003, written in C, compiled with Visual C++ 6.0 (cl 12.00.8168; linker 6.00, build unknown: entry 8447 of the Rich header comes from Allegro's import library, see `notes/toolchain-vc6.md`).

\- Original binaries in `game/`: `happy.exe` (the game, 84 KB), `maped.exe` (map editor, 72 KB), `loader.exe` (launcher, 32 KB).

\- `game/alleg40.dll`: Allegro 4.0.3 library, MSVC build (open source), UPX-packed. We do not decompile it. Details in `notes/allegro.md`. Its headers and its `.def` are versioned in `3rdparty/allegro-4.0.3/` (see "Tools").

\- VC6 C library (CRT) statically linked into the exes. We do not decompile it.



\## Final goal

1\. "Matching" decompilation: C89 that, recompiled with VC6, produces the same machine code function by function (verified with reccmp).

2\. Port of this code to modern C17, verified by comparing behaviour with the matching version.



\## Current phase: matching decompilation (started 2026-10-27), almost complete

C89 decompilation of happy, maped and loader, verified with reccmp. State on 2026-10-28: the three exes are fully annotated and build without stubs; 133/136 GAME functions at 100% (happy 83/84, maped 48/50, loader 2/2). Left: happy `play_level` (`0x00404C50`, 99.84%), maped `set_screen_mode` (`0x00401A20`) and `auto_mask` (`0x00404500`), with the shapes already tried in `notes/open-diffs.md`. Current state and open questions: `notes/STATUS.md`. One commit per batch of functions, with the reccmp scores in the message. The inventory (`notes/layout-*.md`, `notes/data/functions-*.csv`) is the map: blocks = source files.

Per function: full listing (`disassemble_function`), Ghidra's pseudo-C as a starting point only (it can be wrong, e.g. a lost `volatile` in `0x00406C80`), C89, `scripts/build.py`, `python scripts/reccmp_run.py --target HAPPY --verbose 0x...` (reccmp-reccmp with the end-of-array operands compared by offset, `notes/matching.md` §3), iterate. A function stuck below 100% is documented with its diff in `notes/open-diffs.md` and left for later.

Details of the setup: `notes/matching.md`.



\## Tools

\- MCP server `ghidra`: GhidraMCP 7.0.0 (usage guide below). The three exes are open in the same Ghidra project (`happy-decomp`).

\- VC6 (CD 1 of Visual Studio 6.0 Enterprise) in `C:\RE\tools\vc6`, outside the repository: cl 12.00.8168, link 6.00.8168, `LIBC.LIB` identical to the game's. No usable `VCVARS32.BAT`: set `PATH`, `INCLUDE` and `LIB` (`notes/toolchain-vc6.md` §1). From Git Bash, export `MSYS2_ARG_CONV_EXCL='*'`. `scripts/vc6.py` runs the tools with the right environment, natively on Windows or through Wine on macOS (root: `VC6_ROOT`, otherwise `tools/vc6` next to the repository).
- **Setting up a machine** (Windows or macOS Apple Silicon): skill `dev-setup` (`.claude/skills/dev-setup/`). Diagnostics: `python scripts/doctor.py [--deep]`, standard library only. On macOS, the venv's Python is `.venv/bin/python` instead of `.venv/Scripts/python`.
- Allegro 4.0.3 in `3rdparty/allegro-4.0.3/`: `include/` and `lib/msvc/allegro.def`, copied byte for byte from the official archive (`all403.zip`), configured for MSVC. Only modification: `alplatf.h` contains `#define ALLEGRO_MSVC`, as after `fix.bat msvc`. Provenance, SHA-256 and the command that generates `alleg40.lib` (`LIB /DEF`, import by ordinal like the game): `3rdparty/allegro-4.0.3/README.md`. `.gitattributes` prevents git from converting their CRLF line endings. The full archive stays in `captures/allegro-4.0.3/`, for reference; do not compile with that copy, which is configured for DJGPP.
- reccmp **0.1.7**, installed in the global Python 3.12 (not in `.venv`): `reccmp-project`, `reccmp-reccmp`, `reccmp-datacmp`. Project: `reccmp-project.yml` at the root (3 targets, `source_root: src`); `reccmp-user.yml` and `reccmp-build.yml` are per machine (ignored by git, the second one written by `scripts/build.py`). **`reccmp-ghidra-import` writes to Ghidra**: do not use it without explicit approval.
- MCP server `x64dbg` (port 9095): configured in `.mcp.json`, to be started separately. Not used so far.

\- PowerShell shell, Python 3.12, `upx`.



\## Ghidra MCP: usage guide

- **Versions**: Ghidra **12.1.4** (`D:\ghidra_12.1.4_PUBLIC`), GhidraMCP 7.0.0 extension built for 12.1.4, bridge `bridge-mcp-ghidra` (stdio) to `http://127.0.0.1:8089`. **Do not upgrade to Ghidra 12.2**: under 12.2, everything that decompiles fails with the error `ghidra/program/model/lang/SleighLanguageDescription` (observed on 2026-10-27).
- **Program**: always pass `program="happy.exe"` (or `maped.exe`, `loader.exe`), without `/`. Without this parameter, the call is refused (`GHIDRA_MCP_REQUIRE_PROGRAM_SELECTORS=1`). Do not use `switch_program`.
- **Tool groups**: 205 tools split into groups. Loaded at startup: `function`, `listing`, `program`. For another tool: `search_tools("keyword")`, then `load_tool_group("<group>")`. The group's tools then appear as deferred tools, to be loaded through ToolSearch. Never load `all`, which costs too much context. Useful groups here:
  - `xref`: `get_xrefs_to` and `get_xrefs_from` (several addresses per call with `addresses=`), call graphs;
  - `analysis`: `find_code_gaps` (code outside functions), `search_byte_patterns`, `search_instructions`, `get_function_pcode`, `find_similar_functions`;
  - `documentation`: `get_function_hash`, `diff_functions`, `find_similar_functions_fuzzy`, `bulk_fuzzy_match`, to compare happy, maped and loader.
- **Main tool: `get_functions`**. In batch mode, up to 20 functions per call (`functions="0x...,0x..."`). Always restrict with `fields=`: without `decompiled_code` or `call_context`, nothing is decompiled. Set `include_call_context=false`, unless really needed (one decompilation per caller). Pitfalls [verified 2026-10-27 on happy]:
  - the `disassembly` field stops at **200 instructions**. For the full listing, use `disassemble_function`, which also bounds degenerate bodies (`body_degenerate`);
  - `body_start`/`body_end` give the extremities (`body_end` inclusive) and flatten bodies split into several chunks (`0x0040B9F0`, `0x0040F1F0`, `0x0040FC24`, `0x0040FD6D`). The `size` field gives the real body size, chunks included. For the exact union of bodies, take the complement of `find_code_gaps`;
  - in batch mode, the response keys repeat the references as they were passed, case included (`0x004014a0` ≠ `0x004014A0`);
  - `callers` also counts a function that contains a mere DATA reference (e.g. `0x0040B377` to `0x0040CA3A`). To tell a real call apart, read the `type` in `get_xrefs_to`.
- **Counts**: `find_functions` only lists internal functions (happy: 223, maped: 209, loader: 64). `get_function_count` and `list_open_programs` add the imported functions (happy: 327, maped: 311, loader: 119). The auto-analysis missed functions (fresh import: 220 / 200 / 63, including the whole `main` of loader); the game ones were created on 2026-10-28 (11 functions, `notes/matching.md` §3b), the CRT ones are still missing (happy 16, maped 14, loader 6). The reference counts are those of the CSVs `notes/data/functions-*.csv` (239, 223, 70), produced from the fresh import.
- **Pagination**: `limit`/`offset`, and the response gives `total`. `limit=0` returns everything, except for `search_strings` and `find_code_gaps`, where 0 returns an empty page.
- **Writes (decided on 2026-10-27)**: allowed, to carry the names and types of `src/` into Ghidra: renaming functions, globals and variables (`rename_*`), applying types, structures and prototypes (`set_variable_type`, `set_function_prototype`, data types), then `save_program`. **Added on 2026-10-28** (decided with the user: the Ghidra project is rebuilt from git, not carried as a `.gar`): `create_function` for the functions annotated in `src/` that the analysis missed, `import_data_types` for the structures of `src/`, and resetting to `undefined` the data items with generated names that block a type of `src/`; all done by `scripts/ghidra_sync.py`. Every other write tool stays forbidden unless explicitly requested: the other `create_*` and every `delete_*`, comments, tags, bookmarks, `clear_flow_and_repair`, `reanalyze`, `import_file`, `close_program`, `exit_ghidra`... `run_script_inline` and `run_ghidra_script` are disabled server-side (`GHIDRA_MCP_ALLOW_SCRIPTS` not set) and must not be enabled without approval.
- **Scripts**: every tool is also an HTTP endpoint that returns JSON (`GET http://127.0.0.1:8089/<tool>?program=happy.exe&...`). `GET /mcp/schema` describes the 205 endpoints: method, parameters, `read_only`, `destructive`. For any bulk processing, use `scripts/ghidra_rest.py`, which only allows `read_only` GETs.
- **Inventory scripts** (in `scripts/`, to be run with `.venv/Scripts/python`, which contains `capstone` 5.0.9):
  - `ghidra_dump_rest.py <exe>`: JSON copy of Ghidra in `captures/ghidra-cache/<exe>_exe_rest.json`. Rerun only if the Ghidra project changes;
  - `inventory.py <exe>...`: function inventory (method from `notes/layout-happy.md` §1, additions in `notes/layout-maped.md` §1). It writes `notes/data/functions-*.csv`, `notes/data/datarefs-*.csv` and `captures/inventory-*.json`. The per-exe parameters (game/CRT boundary, blocks, proposed CRT names) are in `CONFIGS`. For maped and loader, it also reanalyses happy, to carry over the CRT names, and therefore needs happy's REST cache;
  - `check_vc6.py`: checks the local VC6 toolchain (versions, Rich header, CRT, `/Gy`) with test programs compiled in `captures/vc6-check/`;
  - `smoke_reccmp.py`: end-to-end test of VC6 + Allegro + reccmp on the 5 reference functions, in `captures/smoke/` (expected: 5/5 at 100%);
  - `doctor.py`: diagnostics of the whole toolchain (`--deep` also runs the two previous ones);
  - `pe_allegro.py`: PE headers, Rich header, exports of `alleg40.dll` and imports of the exes (`notes/data/alleg40-*.csv`); also a module used by `inventory.py`, `check_vc6.py` and `crt_names.py`;
  - `inspect_formats.py`: objects of the game's datafiles (`notes/data/datafiles.csv`, `notes/game-docs.md`).
- **Matching scripts** (same Python):
  - `build.py [--final] [happy] [maped] [loader]`: compiles `src/` and links `build/<exe>/<exe>.exe` + `.pdb` in the order of the blocks, generates stubs for the functions not decompiled yet, then runs `reccmp-project detect`. `/OPT:NOREF` by default, `--final` for `/OPT:REF`;
  - `progress.py [exe...]`: runs reccmp (through `reccmp_run.py`) on each built target, writes `notes/data/progress-<exe>.csv` and prints the progress per block;
  - `reccmp_run.py`: reccmp-reccmp 0.1.7 patched at run time (package unchanged), to be run with the Python where reccmp is installed; same arguments. Operands at the same offset of one matched variable (pointers past the end of an array) compare equal (decided with the user on 2026-10-27, `notes/matching.md` §3); `--audit` lists every line concerned;
  - `ghidra_sync.py [--apply | --save] [exe...]`: carries `src/` into Ghidra (dry run by default; `--save` waits for the auto-analysis before saving): structures, missing functions, names, prototypes, global types. From a fresh import of the three exes, `--save happy maped loader` rebuilds the reference project (`notes/matching.md` §3b);
  - `compile_commands.py`: writes `compile_commands.json` (per machine, ignored by git) for clangd: 32-bit MSVC target, VC6 headers, the `/I` and `/D` of `build.py`. Editing only, matching is still checked with VC6. Rerun after adding a source file;
  - `crt_names.py [exe...]`: names the CRT functions by byte identity with the objects of `LIBC.LIB`; writes `notes/data/crt-<exe>.csv` and the `LIBRARY` annotations `src/crt/<exe>.h`.



\## Rules

\- Never modify anything in `game/`. Working copies go to `captures/`.

\- Every claim about the binary cites its address (`0x00401A30`) or its source (file, string, import).

\- Explicitly distinguish \*\*\[verified]\*\* (directly observed: string, import, bytes, xref) and \*\*\[hypothesis]\*\* (deduced). Never present a hypothesis as a fact.

\- Do not invent function or source file names. A proposed name remains a hypothesis with its justification.

\- Naming in `src/` (decided on 2026-10-27): source files keep neutral names (`block_05.c`, after the blocks of `notes/layout-*.md` §8), with the theme in a header comment. Functions and globals get a descriptive name when the code justifies one, with a comment that tags it `[hypothesis]` and gives the evidence; otherwise `FUN_<address>` / `DAT_<address>`. Names in Ghidra follow those of `src/`.

\- CRT or Allegro functions recognised by Ghidra (Function ID, imports) are \[verified]; those recognised by deduction are \[hypothesis].

\- Prefer writing a script (Python or PowerShell, stored in `scripts/`) over chaining dozens of MCP calls by hand when a task covers all functions.

\- In case of an ambiguity that changes the result, stop and ask the question.



\## Output files

\- `notes/`: one Markdown file per topic, in English.

\- `notes/STATUS.md`: to be updated at the end of each task (what is done, what remains, open questions).

\- Tabular data (function lists) as CSV in `notes/data/`.



\## Decompilation rules

C89 decompilation with VC6 + reccmp. Annotations `// FUNCTION: HAPPY 0x...` (one line per exe for the code in `src/common/`). No inline assembly \*\*in the game code\*\*, no artificial tricks to force a match.

\- Layout of `src/`: `common/` (compiled into happy and maped), `happy/`, `maped/`, `loader/` (one `block_NN.c` per block, functions in address order, `data.c` for the globals with their `// GLOBAL:` annotations, `<exe>.h` for the shared declarations), `include/` (headers shared between exes), `crt/` (generated `LIBRARY` annotations, never compiled).

\- Compiler and options chosen on 2026-10-27: **VC 6.0 without service pack, `cl /O2`**, without any `/G` option. Five functions out of five match at 100% on decomp.me (`notes/toolchain-vc6.md` §7).
  - `/G6` and SP5 + Processor Pack do not match.
  - The other service packs produce the same code on these functions. The "no SP" choice rests on the Rich header (cl 12.00.8168) [hypothesis].
  - [verified locally] `/O2` implies `/Gy` (one COMDAT section per function, 16-byte aligned, `0x90` padding) and does not inline small functions on its own (`notes/toolchain-vc6.md` §5).

- Linking and reccmp [verified on a test exe, `notes/toolchain-vc6.md` §6]:
  - compile with `/Zi` and link with `/DEBUG /INCREMENTAL:NO /OPT:REF`: the code and addresses do not change. During the decompilation, `scripts/build.py` links with `/OPT:NOREF`, otherwise the functions whose callers are still stubs are dropped (`notes/matching.md`);
  - name the produced exe `happy.exe` (resp. `maped.exe`, `loader.exe`), since `reccmp-project detect` looks for the target's name;
  - annotate the CRT as `// LIBRARY: HAPPY 0x...` with its name (`// _free`), otherwise every CRT call counts as a mismatch. These annotations are generated by `scripts/crt_names.py` in `src/crt/` (names from `LIBC.LIB`, [verified] by byte identity).

\- Exception decided on 2026-10-27: the official Allegro 4.0.3 headers are included as is, including their inline assembly (`al386vc.h`: `fixmul`, `fixdiv`, etc.). MSVC configuration of Allegro, without `ALLEGRO_NO_ASM` or `ALLEGRO_USE_C`, with `/I 3rdparty/allegro-4.0.3/include`. The code coming from these headers (e.g. `0x00402110`) is annotated as library code, not decompiled.

\- In the C17 phase, these functions are replaced by exact C equivalents (64-bit integer arithmetic), and not by Allegro's C versions, whose rounding may differ.

