---
name: dev-setup
description: Installs, configures and checks the happy-decomp toolchain on Windows or macOS Apple Silicon (VC6 under Wine on Mac, reccmp 0.1.7, Python, Allegro headers; optionally Ghidra 12.1.4 + GhidraMCP 7.0.0), then rebuilds the three exes and verifies them with reccmp. Use it to set up a new machine, check that a machine is operational, or repair a broken toolchain ("setup", "set up the environment", "check the toolchain", "doctor", "it doesn't compile anymore").
---

# Setting up and checking the environment

`scripts/doctor.py` is authoritative. It uses only the Python standard library, runs before anything is installed and changes nothing outside `build/`. The loop is always the same:

1. run the diagnostic;
2. fix the **first failing section**, using the platform sheet;
3. rerun the diagnostic, until there are 0 failures;
4. finish with `--deep` (§6).

Platform sheets, to read when a section fails:
- Windows: [windows.md](windows.md) (reference machine);
- macOS Apple Silicon: [macos.md](macos.md) (VC6 and reccmp's `cvdump` run under Wine; the Nix route, §0 of the sheet, is tested end to end, the Homebrew route is not: report any discrepancy to the user and offer to fix the sheet).

## Rules during this skill

- **`game/` is read-only**, as everywhere in the project.
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

Tags: `[OK]`, `[WARN]` (non-blocking), `[FAIL]` (blocking, exit code 1), `[--]` (not applicable). Each failing line gives a hint (`->`). The Ghidra section (§5) is optional: its problems are reported as warnings.

If no Python is available (fresh Mac), start with §1 of the sheet.

Expected folder layout, the same on both platforms, so that the defaults work:

```
<base>/happy-decomp/        the repository   (Windows: C:\RE\happy-decomp; Mac: ~/RE/happy-decomp)
<base>/tools/vc6/           copy of the VC6 CD  (otherwise: VC6_ROOT variable)
<base>/tools/ghidra_12.1.4_PUBLIC/   optional (otherwise: GHIDRA_INSTALL_DIR, or doctor.py's locations)
```

## 1. System

Windows: Python 3.12 and git. macOS: Xcode command-line tools, Rosetta 2, Wine (`wine` **and** `winepath` in the PATH: reccmp calls them by name), `uv`. Details in the sheet.

## 2. Repository and copy of the game

- `game/`: the user copies their game folder (at least `happy.exe`, `maped.exe`, `loader.exe`, `alleg40.dll`). The 4 hashes are checked by the diagnostic; they are also in `reccmp-project.yml`. A different hash = another version of the game: stop.
- Line endings: `.gitattributes` keeps the original CRLF of `3rdparty/` and enforces CRLF for `notes/data/*.csv` (Python's `csv` module writes CRLF). If the diagnostic reports LF in `3rdparty/`: delete it, then `git checkout -- 3rdparty`.
- `reccmp-user.yml` (per machine, ignored by git): the paths of the originals. Either run `reccmp-project create --originals game/happy.exe game/maped.exe game/loader.exe` in a scratch folder and copy its `reccmp-user.yml`, or write it by hand (`notes/matching.md` §3).

## 3. Python tools

- Every script of the project uses only the standard library: no venv.
- reccmp **0.1.7** (PyPI), with `reccmp-project`, `reccmp-reccmp` and `reccmp-datacmp` in the PATH. Windows: global Python 3.12. macOS: `uv tool install` (in the Nix shell, into `.uv/`; `scripts/reccmp_run.py` and `scripts/progress.py` then run with `.uv/tools/reccmp/bin/python`).

## 4. VC6

- Copy CD 1 (at least `VC98/` and `COMMON/MSDEV98/BIN/`) into `<base>/tools/vc6`, or set `VC6_ROOT`. On a Mac, the simplest way is to copy the folder from a Windows machine.
- The diagnostic compares the hashes of 10 files with the reference copy (`CL.EXE` 12.00.8168, `LIBC.LIB`, `MSPDB60.DLL`…), runs `cl` and `link` (under Wine outside Windows) and compiles a test file.
- All scripts go through `scripts/vc6.py`, which chooses between native launch and Wine, and sets `INCLUDE`, `LIB` and `PATH`/`WINEPATH`. On the command line: `python scripts/vc6.py cl /nologo ...`. Under Wine, the paths passed to the tools must be relative to the current folder (or `Z:\...`).
- `build/alleg40.lib` (import library of Allegro, by ordinal) is generated by `scripts/build.py` when missing (`3rdparty/allegro-4.0.3/README.md`).

## 5. Ghidra, GhidraMCP and Claude Code (optional)

Only needed to browse the binaries with the names and types of `src/`.

1. JDK 21 or later (Temurin).
2. Ghidra **12.1.4** exactly (not 12.2: decompilation fails there, see `CLAUDE.md`).
3. GhidraMCP 7.0.0 extension (release zip): *File > Install Extensions*, restart Ghidra, enable the plugin in CodeBrowser (*File > Configure*), then *Tools > GhidraMCP > Start MCP Server* (port 8089).
4. `bridge-mcp-ghidra` bridge (wheel from the same release) in the PATH: `.mcp.json` calls it by name.
5. Ghidra project `happy-decomp` at the repository root (ignored by git), with the three exes at its root under their own names. Two ways to get it; **ask the user** which one:
   - **(a) new import, recommended**: the three exes with the default auto-analysis (`analyzeHeadless` command in the sheet), then open them in the user interface and run `python scripts/ghidra_sync.py --save happy maped loader`: it imports the structures of `src/`, creates the functions the analysis missed, and applies the names, prototypes and types. Git is the only source: nothing to carry between machines;
   - **(b) exact copy**: on the other machine, *File > Archive Current Project* (`.gar`), then *File > Restore Project* to the repository root, under the name `happy-decomp`. Only useful to keep work done by hand in Ghidra that `src/` does not hold.
6. Open the three programs in CodeBrowser (one tab each) and start the server. The diagnostic must find **205 tools** and **223 / 209 / 64 functions** (happy / maped / loader) after `ghidra_sync.py --save`; a new import gives **220 / 200 / 63** before it. Then `ghidra_sync.py happy maped loader` (dry run) must report 0 types to import and 0 changes. Other counts = analysis different from the reference: stop and ask. If the auto-analysis that follows the creations does not end (seen on Windows on 2026-10-28), save by hand in Ghidra and run the script again.
7. Claude Code: on the first launch in the repository, approve the `ghidra` server from `.mcp.json`. Restart Claude Code after installing the bridge, then check with `/mcp`.

## 6. Final check

`python scripts/doctor.py --deep` (`python3` on a Mac). In addition to the diagnostic, it runs `scripts/build.py --final happy maped loader` then `scripts/progress.py`, and expects **84 / 50 / 2 game functions at 100%** (happy / maped / loader).

The machine is ready when `--deep` gives **0 failures**. Any remaining `[WARN]` must be explained to the user.

## Report

Finish with a short table: section, status, what was installed or done, what remains for the user. Update `notes/STATUS.md` **only** if the run changed versioned files or revealed a difference useful to the project (e.g. a corrected platform sheet). A machine simply going green does not belong there.

## Reference versions and hashes

| Item | Version | Source | SHA-256 |
|---|---|---|---|
| VC6 | cl 12.00.8168, link 6.00.8168 | CD 1 of VS 6.0 Enterprise (the user's copy) | 10 files, in `scripts/doctor.py` |
| reccmp | 0.1.7 | PyPI | — |
| Python | 3.12 (reference: 3.12.10) | python.org / uv | — |
| Ghidra (optional) | 12.1.4 | github.com/NationalSecurityAgency/ghidra, release `Ghidra_12.1.4_build`, `ghidra_12.1.4_PUBLIC_20260921.zip` | `ddac49f903da9d5bac833e5cc79395098b9c33cfd3279be5f31bd00387d2d4db` |
| GhidraMCP (extension) | 7.0.0, for Ghidra 12.1.4 | github.com/bethington/ghidra-mcp, release v7.0.0, `GhidraMCP-7.0.0.zip` | `d133c4c869a0d9f605c1c9426bf5d9064772a6947ccecde0f42fbbffecf1d812` |
| MCP bridge | 7.0.0 | same release, `ghidra_mcp_bridge-7.0.0-py3-none-any.whl` | `a75050061d0d1514d4b11e6288ea7575c09ab6125a1394c33fe349fb7658bbae` |
| JDK | 21 or later (reference: Temurin 25.0.5) | adoptium.net | — |
