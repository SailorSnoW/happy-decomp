# Local VC6 toolchain

Date: 2026-10-27. Script: `scripts/check_vc6.py` (`.venv/Scripts/python scripts/check_vc6.py`, about 20 s). It compiles test programs in `captures/vc6-check/` and does not touch `game/`.

## 1. Installation

- Location: `C:\RE\tools\vc6`, outside the repository (`/tools/` is in `.gitignore`). It is a full copy of CD 1 of Visual Studio 6.0 Enterprise (`VS98ENT.MIF`), without going through the installer.
- Files used:
  - `VC98\BIN`: `CL.EXE`, `C1.DLL`, `C2.DLL`, `LINK.EXE`, `LIB.EXE`, `CVTRES.EXE`, `DUMPBIN.EXE`, `NMAKE.EXE`;
  - `VC98\INCLUDE`;
  - `VC98\LIB`: `LIBC.LIB`, `OLDNAMES.LIB`, `KERNEL32.LIB`...;
  - `COMMON\MSDEV98\BIN\MSPDB60.DLL`.
- [verified] `VC98\BIN\VCVARS32.BAT` only contains `Placeholder vcvars32.bat`: the installer generates it. The environment must therefore be set up by hand:
  - `PATH`: `VC98\BIN` and `COMMON\MSDEV98\BIN`;
  - `INCLUDE`: `VC98\INCLUDE`;
  - `LIB`: `VC98\LIB`.

  `check_vc6.py` does this for its own calls.
- From Git Bash, the `/O2` and `/c` options get converted into paths. Export `MSYS2_ARG_CONV_EXCL='*'`, or go through PowerShell or Python.

## 2. Versions

| Tool | Local version | Game exe (Rich header) |
|---|---|---|
| `cl` | 12.00.8168 | Utc12_C and Utc12_CPP 8168 [verified]: **identical** |
| `link` | 6.00.8168 | no entry for the linker (see §3); PE header: 6.00 |
| `lib` | 6.00.8168 | — |
| `cvtres` | 5.00.1720.1 | Cvtres500 1720 [verified]: **identical** |
| `KERNEL32.LIB` | import objects Linker512 8034 | Linker512 8034 [verified]: **identical** |
| `OLDNAMES.LIB` | objects AliasObj60 7291 | AliasObj60 7291 [verified]: **identical** |
| MASM of the `LIBC.LIB` objects | Masm613 7299 | Masm613 7299 [verified]: **identical** |

## 3. Correction: the game was not "linked with link 6.00.8447"

Until now, `CLAUDE.md` and several notes read the `Linker600 8447 ×3` entry of the Rich header as the version of the game's linker. That is wrong:

- [verified] Test exes linked with the local `link` 6.00.8168 have **no** entry for their own linker. The VC6 Rich header only counts the objects received as input.
- [verified] A test exe that imports only `KERNEL32` carries `Linker512 8034 ×3`. These three import objects are the DLL descriptor, its `NULL_THUNK_DATA` and the shared `NULL_IMPORT_DESCRIPTOR`.
- [verified] The game exes import two DLLs, `alleg40.dll` and `KERNEL32.dll`, and carry `Linker512 8034 ×2` and `Linker600 8447 ×3`.
- [strong hypothesis] The 8447 entry therefore designates the objects of the **Allegro import library**, together with the shared `NULL_IMPORT_DESCRIPTOR`, taken from the first library read. That library was produced by the link 6.00.8447 that linked `alleg40.dll`: the DLL itself carries a `Linker600 8447` entry (`notes/allegro.md` §2).
- **Consequence**: the build of the game's linker is **unknown**. The PE header only says 6.00. The local `link` 6.00.8168 is a compatible candidate: same version 6.00 and same file alignments (raw sections in multiples of `0x1000`, [verified] on the tests as on the game).
- Reproducing the Rich header exactly would require the original Allegro 4.0.3 import library, produced by link 8447. A library generated locally with `lib` 8168 would give `Linker600 8168 ×3`. This changes nothing in the code: reccmp compares functions, not the Rich header.

## 4. CRT: the local `LIBC.LIB` is identical to the game's

[verified] The 149 CRT functions of happy that are 16 bytes or larger (`notes/data/functions-happy.csv`) are found in the test exes, identical up to addresses:
- 145 by masked signature (the `inventory.py` method);
- `0x0040B9F0` and `0x0040F1F0` (copies of `memcpy`/`memmove`, same code), and `0x0040E8C5`: whole body identical in masked bytes;
- `0x0041015D`: 417 instructions identical up to addresses, up to its internal jump table (data).

The 5 CRT functions smaller than 16 bytes were not compared.

## 5. Other results

- [verified] **`/O2` implies `/Gy`**: each function is a COMDAT `.text` section (flags `0x60501020`) aligned on 16 bytes. The linker fills the gaps with `0x90` (`t_comdat.exe`: `f1` at `0x00401000`, `f2` at `0x00401010`, `f3` at `0x00401030`). This explains the 16-byte alignment with `0x90` of the game's functions (`notes/layout-happy.md`). The CRT functions, assembled or compiled differently, are not aligned on 16. It also makes possible the elimination of unreferenced functions by `/OPT:REF`, enabled by default without `/DEBUG`: that was the condition stated in `notes/shared-code.md`.
- [verified] `/O2` does not inline small functions of the same file on its own (`t_comdat.exe`: `main` calls `f1`, `f2` and `f3`). This is consistent with `/Ob1`.
- [verified] **AliasObj60** counts the `OLDNAMES.LIB` aliases actually used: `itoa`, `chdir` and `strcmpi` give `AliasObj60 ×3`. [hypothesis] happy therefore calls two functions by their old name, without `_` (`strcmpi` for `__strcmpi` at `0x00408500`?), maped and loader only one. **Identified for maped and loader** [verified 2026-10-27]: `chdir`, from the CRT layout it produces (`notes/matching.md` §3). happy's two are still to identify: the rebuilt happy calls three (`itoa`, `stricmp`, `strcmpi`, `AliasObj60 ×3`), so one spelling differs from the original; the CRT layout does not settle it (`notes/matching.md` §3).
- [verified] `Utc12_CPP 8168 ×1` appears in all tests, even without C++. That object therefore comes from `LIBC.LIB`: the game is entirely in C.

## 6. End-to-end test (2026-10-27)

Done in the session scratchpad, outside the repository. Nothing was written to `captures/` or `game/`.

Setup:
- headers: copy of `captures/allegro-4.0.3/src/allegro/include`. **Caution**: in `captures/`, `include/allegro/platform/alplatf.h` contains `#define ALLEGRO_DJGPP` (generated by `fix.sh`, the default state of the zip). In the test copy it was replaced by `#define ALLEGRO_MSVC`, which is what `fix.bat msvc` would do. → Since then, the headers configured for MSVC are versioned in `3rdparty/allegro-4.0.3/`, and the test was redone with them, with the same results;
- import library: `LIB /DEF:lib/msvc/allegro.def /MACHINE:IX86 /NAME:alleg40.dll /OUT:alleg40.lib`;
- source: the 5 functions of `notes/decompme-candidates.md` §10, as is, with `#include <allegro.h>` instead of hand-written declarations, plus a `main` and `END_OF_MAIN()`;
- `cl /O2 /c`, then `link /SUBSYSTEM:WINDOWS ... alleg40.lib kernel32.lib`. Variant with `/Zi` at compile time and `/DEBUG /INCREMENTAL:NO /OPT:REF` at link time.

Results:
- [verified] The Allegro 4.0.3 headers compile with cl 8168 without errors or warnings (default level).
- [verified] **The 5 functions are identical** to those of happy, instruction for instruction (addresses masked): `0x00406C80` (25 instructions), `0x00409010` (31), `0x0040A9A0` (54), `0x0040AB00` (15), `0x0040AC10` (16). The `WinMain` stub of `END_OF_MAIN()` is identical to `0x0040A970` (12).
- [verified] **`/Zi` + `/DEBUG /INCREMENTAL:NO /OPT:REF` change neither the code nor the addresses**: same bytes, same addresses in both test exes. `/OPT:REF` must be passed explicitly, since `/DEBUG` disables it by default [documentation, not tested without it].
- [verified] The import library produced by `LIB /DEF` imports **by ordinal** (`dumpbin /imports`: `Ordinal 394` for `load_datafile`, `382` for `key`...), like the game. This settles the open question in `notes/allegro.md`.
- [verified] Same layout as happy: `KERNEL32.dll` IAT before that of `alleg40.dll`, linker version 6.00, section and file alignments `0x1000`, base `0x00400000`.
- [verified] The PDB produced is in the "Microsoft C/C++ program database 2.00" format.

### reccmp

reccmp **0.1.7**, installed by the user in the global Python 3.12 (not in `.venv`), with capstone 5.0.9. It also depends on `pyghidra`, used by `reccmp-ghidra-import`, a tool that **writes** to Ghidra: forbidden during the inventory.

Procedure followed, in the test folder:
1. `reccmp-project create --originals C:\RE\happy-decomp\game\happy.exe --path .`: writes `reccmp-project.yml` (target `HAPPY`, SHA-256 identical to the one in the table below, `source_root: .`) and `reccmp-user.yml` (path of the original).
2. Link with `/OUT:happy.exe /PDB:happy.pdb`, because detection looks for the target's file name.
3. `reccmp-project detect --what recompiled --search-path b1`: writes `reccmp-build.yml`.
4. `reccmp-reccmp --target HAPPY`, then `reccmp-datacmp --target HAPPY`.

Results:
- [verified] reccmp reads the VC6 PDB 2.00. The `// FUNCTION: HAPPY 0x...` and `// GLOBAL: HAPPY 0x...` annotations are recognised, and source line numbers appear in the diffs.
- [verified] `0x00406C80`, `0x00409010`, `0x0040A9A0` and `0x0040AC10` are at **100%**. `0x0040AB00` was at first 86.67%, solely because reccmp did not know the callee `0x0040B463`. With `// LIBRARY: HAPPY 0x0040b463` / `// _free`, it goes to **100%**.
- [verified] `_free` (`0x0040B463`, 21 instructions) and `_malloc` (`0x0040B492`, 6 instructions) differ from LIBC **only** by the CRT symbols they reference, still unknown on the original side (`___sbh_find_block`, `___sbh_free_block`, `__crtheap`, `__newmode`, `__nh_malloc`). The name `free` proposed for `0x0040B463` is therefore confirmed by the code.
- `_WinMainCRTStartup` (`0x0040B506`) is matched automatically through the entry point. Score: 74.36%, for the same reason (`__except_handler3`, `__winver`, `__heap_init`...).
- [verified] `reccmp-datacmp`: 6 variables, 0 problems. The `--json` and `--html` reports are produced.

Consequence for the matching phase: `LIBRARY` annotations will be needed for the whole CRT, otherwise every call into the CRT counts as a mismatch. The names come from `notes/data/functions-*.csv`. Those still [hypothesis] can be confirmed the same way.

SHA-256 hashes of the targets (`game/`):

| File | SHA-256 |
|---|---|
| `happy.exe` | `c42e561678e5b968c5110e05b7e81eb28581b81bbdebaca81aa3c68502a5f4ba` |
| `maped.exe` | `789ee11e62e1a2b1241f60e3a060f0bd2ac926d15a6762e6b2b31946cac74f92` |
| `loader.exe` | `4814e49733f8b4c7b7050f0898b824fe58fcaf8d23930d8e96d00410eee74fd4` |
| `alleg40.dll` | `11ff7d4ce9fb85ca03a15c8a4680fdd467f18daf346bf0c07ecc5933135cbfe3` |

## 7. Open questions

- Exact build of the game's linker (§3). It does not affect function code. It may affect the order of contributions in `.data`/`.bss` or header details: to watch when reccmp compares the whole exe.
- Which functions does the game call by an old name?
