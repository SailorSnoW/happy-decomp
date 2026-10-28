# VC6 toolchain

What is needed to rebuild the three exes, and the facts behind the choice of compiler and options. Build procedure: `notes/matching.md`; machine setup: skill `dev-setup`; diagnostics: `python scripts/doctor.py`.

## 1. Installation

- A full copy of **CD 1 of Visual Studio 6.0 Enterprise** (`VS98ENT.MIF`), **without any service pack**, copied as is (the installer is not needed), outside the repository. `scripts/vc6.py` looks for it in `VC6_ROOT`, then `<repo>/tools/vc6` (ignored by git), then `<repo>/../tools/vc6` (e.g. `C:\RE\tools\vc6`). It accepts lowercase copies of the CD.
- Files used: `VC98\BIN` (`CL.EXE`, `C1.DLL`, `C2.DLL`, `LINK.EXE`, `LIB.EXE`, `CVTRES.EXE`, `DUMPBIN.EXE`), `VC98\INCLUDE`, `VC98\LIB` (`LIBC.LIB`, `OLDNAMES.LIB`, `KERNEL32.LIB`), `COMMON\MSDEV98\BIN\MSPDB60.DLL`. `scripts/doctor.py` checks their SHA-256 against the reference copy.
- [verified] `VC98\BIN\VCVARS32.BAT` on the CD is a placeholder (the installer generates the real one). The environment is set by hand: `PATH` = `VC98\BIN` + `COMMON\MSDEV98\BIN`, `INCLUDE` = `VC98\INCLUDE`, `LIB` = `VC98\LIB`. `scripts/vc6.py` does it for every call (`python scripts/vc6.py cl /nologo ...`).
- **macOS / Linux**: `scripts/vc6.py` runs every tool through `wine`, with `INCLUDE`/`LIB` as Windows paths, `MSPDB60.DLL` reached through `WINEPATH`, `WINEDEBUG=-all` and Mono/Gecko disabled. Paths passed to the tools must be relative to the working directory or converted to `Z:\...` (`VC6.path()`). reccmp also needs `wine` and `winepath` in the `PATH`.
- **Git Bash on Windows**: export `MSYS2_ARG_CONV_EXCL='*'`, otherwise `/O2`, `/c`... are converted into paths.

## 2. Versions

| Tool | Local (CD 1, no SP) | Game exes (Rich header) |
|---|---|---|
| `cl` | 12.00.8168 | Utc12_C / Utc12_CPP 8168: identical [verified] |
| `link` | 6.00.8168 | no entry for the linker (§3); PE header: 6.00 |
| `cvtres` | 5.00.1720 | Cvtres500 1720: identical [verified] |
| `KERNEL32.LIB` import objects | Linker512 8034 | Linker512 8034: identical [verified] |
| `OLDNAMES.LIB` | AliasObj60 7291 | AliasObj60 7291: identical [verified] |
| MASM objects of `LIBC.LIB` | Masm613 7299 | Masm613 7299: identical [verified] |

- [verified] `Utc12_CPP 8168 ×1` appears in every test exe, even without C++: it comes from `LIBC.LIB`. The game is entirely C.
- [verified] The Rich headers of happy, maped and loader show the same builds: no tool differs between the three exes.

## 3. Linker build

[verified] A VC6 Rich header counts only the input objects, not the linker itself, so the `Linker600 8447 ×3` entry of the game exes is not the game's linker: it is the import objects of `alleg40.lib` (the DLL itself was linked by link 6.00.8447, `notes/allegro.md` §2) [strong hypothesis]. The game's linker build is therefore unknown; the local link 6.00.8168 has the same version and the same alignments, and the locally generated `alleg40.lib` gives `Linker600 8168` instead, which does not affect the code.

## 4. CRT

[verified] The local `LIBC.LIB` is the game's: all 149 CRT functions of happy of 16 bytes or more are found in test exes linked with it, identical up to addresses (`0x0041015D` up to its internal jump table). The CRT is linked statically (`LIBC.LIB`, the single-threaded library of cl's default `/ML`): the exes import only `KERNEL32.dll` besides `alleg40.dll`. CRT functions are annotated `LIBRARY` from byte identity with `LIBC.LIB` (`src/crt/<exe>.h`, `notes/data/crt-*.csv`, method in `notes/matching.md`).

## 5. What the compiler options produce

- [verified] **`/O2` implies `/Gy`**: one COMDAT `.text` section per function (flags `0x60501020`), 16-byte aligned, gaps filled with `0x90`. This is the alignment of the game functions; the CRT functions (assembled or compiled differently) are not aligned on 16. `/Gy` is also what lets `/OPT:REF` drop unreferenced functions, so that a source file shared by happy and maped keeps only the functions each exe uses (`notes/layout-maped.md`).
- [verified] `/O2` does not inline small functions of the same file on its own (consistent with `/Ob1`): only the Allegro header functions are inlined.
- [verified] **AliasObj60** counts the `OLDNAMES.LIB` aliases actually used (`itoa`, `chdir`, `strcmpi`...). maped and loader have one, `chdir`; happy has two, while its sources call three (`itoa`, `stricmp`, `strcmpi`), and which spelling differs from the original is not observable (`notes/matching.md` §3).

## 6. Linking, PDB and reccmp

- `scripts/build.py` compiles with `cl /nologo /O2 /Zi /W3 /c` (plus `/I src/include`, `/I src/<exe>`, `/I 3rdparty/allegro-4.0.3/include`, `/D<EXE>`) and links with `link /nologo /SUBSYSTEM:WINDOWS /DEBUG /INCREMENTAL:NO /MAP` + `/OPT:NOREF` (default) or `/OPT:REF` (`--final`), against `alleg40.lib` and `kernel32.lib`.
- [verified] **`/Zi` and `/DEBUG /INCREMENTAL:NO /OPT:REF` change neither the code nor the addresses** (same bytes as without them). `/DEBUG` disables `/OPT:REF` unless it is passed explicitly [documentation].
- [verified] Same layout as the originals: base `0x00400000`, section and file alignment `0x1000`, `KERNEL32.dll` IAT before that of `alleg40.dll`, linker version 6.00.
- [verified] `alleg40.lib`, generated by `LIB /DEF:3rdparty/allegro-4.0.3/lib/msvc/allegro.def /MACHINE:IX86 /NAME:alleg40.dll`, imports **by ordinal**, like the game (`3rdparty/allegro-4.0.3/README.md`).
- [verified] The PDB is in the "Microsoft C/C++ program database 2.00" format; reccmp 0.1.7 reads it, with source lines in the diffs. The produced exe must be named like the target (`happy.exe`...) for `reccmp-project detect`.
- [verified] Without `LIBRARY` annotations for the CRT, every call into the CRT counts as a mismatch (§4).

SHA-256 of the originals in `game/` (checked by `scripts/doctor.py`):

| File | SHA-256 |
|---|---|
| `happy.exe` | `c42e561678e5b968c5110e05b7e81eb28581b81bbdebaca81aa3c68502a5f4ba` |
| `maped.exe` | `789ee11e62e1a2b1241f60e3a060f0bd2ac926d15a6762e6b2b31946cac74f92` |
| `loader.exe` | `4814e49733f8b4c7b7050f0898b824fe58fcaf8d23930d8e96d00410eee74fd4` |
| `alleg40.dll` | `11ff7d4ce9fb85ca03a15c8a4680fdd467f18daf346bf0c07ecc5933135cbfe3` |

## 7. Compiler choice: VC 6.0 without SP, `cl /O2`

- [verified] Five GAME functions of happy (`0x00406C80`, `0x00409010`, `0x0040A9A0`, `0x0040AB00`, `0x0040AC10`) match at 100% on decomp.me with **VC 6.0 without SP, `/O2`**, and again locally with the CD 1 toolchain. Scratches: [42r3i](https://decomp.me/scratch/42r3i), [8bHPJ](https://decomp.me/scratch/8bHPJ), [72ufv](https://decomp.me/scratch/72ufv), [rtNCI](https://decomp.me/scratch/rtNCI), [pJhdK](https://decomp.me/scratch/pJhdK).
- [verified] `/Gs`, `/GB` and `/G5` change nothing on them; **`/G6` does not match** (`0x0040A9A0`); **SP5 + Processor Pack does not match** (`0x00406C80`, `0x00409010`, `0x0040A9A0`).
- [hypothesis] SP1 to SP6 give the same code on these functions; choosing "no SP" among them rests on the Rich header (cl 12.00.8168, §2).
- Hence `cl /O2` with no `/G` option. [verified] With it, all 136 game functions match at 100% (`notes/STATUS.md`).

## 8. Open questions

- Build of the game's linker (§3). It does not affect function code; it could affect the order of contributions in `.data`/`.bss` or header details.
- Which of happy's three OLDNAMES calls is spelled differently in the original (§5).
