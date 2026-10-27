---
name: dev-setup
description: Installs, configures and checks the whole happy-decomp development toolchain on Windows or macOS Apple Silicon (VC6 under Wine on Mac, reccmp 0.1.7, Ghidra 12.1.4 + GhidraMCP 7.0.0, Python and capstone, Allegro headers, working data in captures/), then prepares the project and verifies it end to end. Use it to set up a new machine, check that a machine is operational, or repair a broken toolchain ("setup", "set up the environment", "check the toolchain", "doctor", "it doesn't compile anymore").
---

# Setting up and checking the environment

`scripts/doctor.py` is authoritative. It uses only the Python standard library, runs before anything is installed and changes nothing outside `captures/doctor/`. The loop is always the same:

1. run the diagnostic;
2. fix the **first failing section**, using the platform sheet;
3. rerun the diagnostic, until there are 0 failures;
4. finish with `--deep` (§7).

Platform sheets, to read when a section fails:
- Windows: [windows.md](windows.md) (reference machine, everything there is verified);
- macOS Apple Silicon: [macos.md](macos.md) (VC6 and reccmp's `cvdump` run under Wine; the Nix route, §0 of the sheet, is tested end to end, the Homebrew route is not: report any discrepancy encountered to the user, and offer to fix the sheet).

## Rules during this skill

- **`game/` is read-only**, as everywhere in the project. Copies go into `captures/`.
- **Ghidra**: never modify a machine's existing project. Creating a project on a machine that has none (§5) is allowed, **after the user's approval**. Never `reccmp-ghidra-import`.
- **System installs** (winget, Homebrew, `softwareupdate`, JDK): first present the full list of what is missing and the planned commands, then wait for approval. Commands that are interactive or need `sudo`: have the user run them with the `!` prefix in the prompt.
- **Downloads**: each into a fresh folder (e.g. `~/Downloads/happy-setup/` or the scratchpad); check its SHA-256 against the table below before using it. On a hash mismatch: stop and ask.
- **What only the user can provide** (never look for it online):
  - their own copy of the game (`game/`, never versioned);
  - their copy of CD 1 of Visual Studio 6.0 Enterprise (Microsoft licence);
  - actions in the Ghidra user interface and the approval of MCP servers in Claude Code.
- Commit nothing. If a step reveals a difference in versioned files (`git status`), show it and ask.

## 0. Diagnostic

| Platform | Command, from the repository root |
|---|---|
| Windows (PowerShell) | `python scripts/doctor.py` (or `py -3.12 scripts/doctor.py`) |
| macOS | `python3 scripts/doctor.py` |

Tags: `[OK]`, `[WARN]` (non-blocking), `[FAIL]` (blocking, exit code 1), `[--]` (not applicable). Each failing line gives a hint (`->`).

If no Python is available (fresh Mac), start with §1 of the sheet.

Expected folder layout, the same on both platforms, so that the defaults work:

```
<base>/happy-decomp/        the repository   (Windows: C:\RE\happy-decomp; Mac: ~/RE/happy-decomp)
<base>/tools/vc6/           copy of the VC6 CD  (otherwise: VC6_ROOT variable)
<base>/tools/ghidra_12.1.4_PUBLIC/   optional (otherwise: GHIDRA_INSTALL_DIR, or doctor.py's locations)
```

## 1. System

Windows: Python 3.12 and git. macOS: Homebrew, Xcode command-line tools, Rosetta 2, Wine (`wine` **and** `winepath` in the PATH: reccmp calls them by name), `uv`. Details in the sheet.

## 2. Repository and copy of the game

- `game/`: the user copies their complete game folder (`happy.exe`, `maped.exe`, `loader.exe`, `alleg40.dll`, `data/`, `maps/`…). The 4 hashes are checked by the diagnostic (`notes/toolchain-vc6.md` §6). A different hash = another version of the game: stop.
- Line endings: `.gitattributes` keeps the original CRLF of `3rdparty/` and enforces CRLF for `notes/data/*.csv` (Python's `csv` module writes CRLF). If the diagnostic reports LF in an old clone: delete the affected files, then `git checkout -- 3rdparty notes/data`.

## 3. Python tools

- `.venv` at the root, Python 3.12, `pip install -r requirements.txt` (capstone 5.0.9). venv Python: `.venv/Scripts/python` on Windows, `.venv/bin/python` on macOS. The docstrings and `CLAUDE.md` cite the Windows form.
- reccmp **0.1.7** (PyPI), outside the venv, with `reccmp-project`, `reccmp-reccmp` and `reccmp-datacmp` in the PATH. Windows: global Python. macOS: `uv tool install`.

## 4. VC6

- Copy CD 1 (at least `VC98/` and `COMMON/MSDEV98/BIN/`) into `<base>/tools/vc6`, or set `VC6_ROOT`. On a Mac, the simplest way is to copy the folder from the Windows machine.
- The diagnostic compares the hashes of 10 files with the reference copy (`CL.EXE` 12.00.8168, `LIBC.LIB`, `MSPDB60.DLL`…), runs `cl` and `link` (under Wine outside Windows) and compiles a test file.
- All scripts go through `scripts/vc6.py`, which chooses between native launch and Wine, and sets `INCLUDE`, `LIB` and `PATH`/`WINEPATH`. On the command line: `python scripts/vc6.py cl /nologo ...`. Under Wine, the paths passed to the tools must be relative to the current folder (or `Z:\...`).

## 5. Ghidra, GhidraMCP and Claude Code

1. JDK 21 or later (Temurin).
2. Ghidra **12.1.4** exactly (not 12.2: decompilation fails there, see `CLAUDE.md`).
3. GhidraMCP 7.0.0 extension (release zip): *File > Install Extensions*, restart Ghidra, enable the plugin in CodeBrowser (*File > Configure*), then *Tools > GhidraMCP > Start MCP Server* (port 8089).
4. `bridge-mcp-ghidra` bridge (wheel from the same release) in the PATH: `.mcp.json` calls it by name.
5. Ghidra project `happy-decomp` at the repository root (ignored by git), with the three exes at its root under their own names (`happy.exe`, `maped.exe`, `loader.exe`). Two ways to get it; **ask the user** which one:
   - **(a) exact copy, recommended**: on the Windows machine, *File > Archive Current Project* (`.gar`), then on the new machine *File > Restore Project* to the repository root, under the name `happy-decomp`;
   - **(b) new import** with the default auto-analysis (`analyzeHeadless` command in the sheet), then open it in the user interface.
6. Open the three programs in CodeBrowser (one tab each) and start the server. The diagnostic must find **205 tools** and **220 / 200 / 63 functions** (happy / maped / loader). Other counts = analysis different from the reference: stop and ask, because the CSVs in `notes/data/` depend on it.
7. Claude Code: on the first launch in the repository, approve the `ghidra` server from `.mcp.json`. Restart Claude Code after installing the bridge, then check with `/mcp`. The `x64dbg` server (port 9095) is not used during the inventory. On a Mac, its connection failure is normal.

## 6. Working data (`captures/`, `build/`)

These files are ignored by git and can be regenerated. `PY` = the venv Python. Ghidra must be running for step 2.

1. Unpacked Allegro DLL (a copy, `game/` untouched): `upx -d -o captures/alleg40_unpacked.dll game/alleg40.dll`.
2. Ghidra cache, one command per exe: `PY -I scripts/ghidra_dump_rest.py happy.exe`, then `maped.exe` and `loader.exe`.
3. Inventory: `PY scripts/pe_allegro.py`, `PY scripts/inventory.py happy.exe maped.exe loader.exe`, then `PY scripts/shared_code.py`.
4. **Reproducibility check**: `git status --short notes/` must be empty. The CSVs regenerated on this machine are then byte-for-byte identical to those in the repository. Otherwise: `git diff --stat notes/`, show the difference to the user and stop. Do not commit or restore without their approval.
5. Allegro import library, for the matching phase (`3rdparty/allegro-4.0.3/README.md`). From the repository root, after creating `build/`:
   `python scripts/vc6.py lib /NOLOGO "/DEF:3rdparty\allegro-4.0.3\lib\msvc\allegro.def" /MACHINE:IX86 /NAME:alleg40.dll "/OUT:build\alleg40.lib"`
   Under Git Bash, first export `MSYS2_ARG_CONV_EXCL='*'`. Under zsh, put the arguments in single quotes.
6. Optional, for reference only: the `all403.zip` archive in `captures/allegro-4.0.3/` (provenance and SHA-256: `3rdparty/allegro-4.0.3/README.md`).

## 7. Final check

`python scripts/doctor.py --deep` (`python3` on a Mac). In addition to the diagnostic, it runs:
- `scripts/check_vc6.py`: versions, Rich header, 145/149 happy CRT functions found by signature and the other 4 matching apart from addresses, `/Gy` (`notes/toolchain-vc6.md` §2-5);
- `scripts/smoke_reccmp.py`: LIB /DEF, `/O2 /Zi` compilation of the 5 reference functions, `/DEBUG /INCREMENTAL:NO /OPT:REF` link, reccmp project in `captures/smoke/`. Expected: **5/5 at 100%** and `_free` matched through its `LIBRARY` annotation.

The machine is ready when `--deep` gives **0 failures** and step 4 of §6 is empty. Any remaining `[WARN]` must be explained to the user.

## Report

Finish with a short table: section, status, what was installed or done, what remains for the user. Update `notes/STATUS.md` **only** if the run changed versioned files or revealed a difference useful to the project (e.g. a corrected platform sheet, a difference in Ghidra analysis). A machine simply going green does not belong there.

## Reference versions and hashes

| Item | Version | Source | SHA-256 |
|---|---|---|---|
| Ghidra | 12.1.4 | github.com/NationalSecurityAgency/ghidra, release `Ghidra_12.1.4_build`, `ghidra_12.1.4_PUBLIC_20260921.zip` | `ddac49f903da9d5bac833e5cc79395098b9c33cfd3279be5f31bd00387d2d4db` |
| GhidraMCP (extension) | 7.0.0, for Ghidra 12.1.4 | github.com/bethington/ghidra-mcp, release v7.0.0, `GhidraMCP-7.0.0.zip` | `d133c4c869a0d9f605c1c9426bf5d9064772a6947ccecde0f42fbbffecf1d812` |
| MCP bridge | 7.0.0 | same release, `ghidra_mcp_bridge-7.0.0-py3-none-any.whl` | `a75050061d0d1514d4b11e6288ea7575c09ab6125a1394c33fe349fb7658bbae` |
| reccmp | 0.1.7 | PyPI | — |
| capstone (venv) | 5.0.9 | PyPI, `requirements.txt` | — |
| JDK | 21 or later (reference: Temurin 25.0.5) | adoptium.net | — |
| Python | 3.12 (reference: 3.12.10) | python.org / uv | — |
| upx | reference 5.2.1 | winget / Homebrew | `captures/alleg40_unpacked.dll`: `c7720741abab462a43b7316446e8d0001cbb4b780a0ee842a51c20043ca58aaf` |
| VC6 | cl 12.00.8168, link 6.00.8168 | CD 1 of VS 6.0 Enterprise (the user's copy) | 10 files, in `scripts/doctor.py` |
