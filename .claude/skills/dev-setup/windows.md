# Windows sheet (reference machine)

Reference state, recorded on 2026-10-27 on Windows 11: everything below is [verified] by `scripts/doctor.py --deep` (0 failures).

Commands for PowerShell, from the repository root (`C:\RE\happy-decomp`). Under Git Bash, export `MSYS2_ARG_CONV_EXCL='*'` before any VC6 command, otherwise `/O2` or `/DEF:` are taken for paths.

## 1. System

```powershell
winget install -e --id Git.Git
winget install -e --id Python.Python.3.12
```

Open a new terminal so that the PATH is reloaded.

## 3. Python tools

```powershell
py -3.12 -m venv .venv
.venv\Scripts\python -m pip install -r requirements.txt
py -3.12 -m pip install reccmp==0.1.7          # global Python, like the reference machine
```

`reccmp-project`, `reccmp-reccmp` and `reccmp-datacmp` land in `%LOCALAPPDATA%\Programs\Python\Python312\Scripts`, which must be in the PATH (the Python installer puts it there).

## 4. VC6

- Copy CD 1 of Visual Studio 6.0 Enterprise into `C:\RE\tools\vc6` (folder next to the repository), without going through the installer. Another location: set `VC6_ROOT`.
- `VC98\BIN\VCVARS32.BAT` on the CD is an empty file (`Placeholder`). There is nothing to run: `scripts/vc6.py` sets up the environment.

## 5. Ghidra and GhidraMCP

```powershell
winget install -e --id EclipseAdoptium.Temurin.21.JDK   # the reference uses Temurin 25, 21 is enough
```

1. Download `ghidra_12.1.4_PUBLIC_20260921.zip` (release `Ghidra_12.1.4_build` of NationalSecurityAgency/ghidra), check its SHA-256 (`Get-FileHash -Algorithm SHA256`), and unzip it for example into `C:\RE\tools\` or `D:\` (reference location: `D:\ghidra_12.1.4_PUBLIC`). Elsewhere: set `GHIDRA_INSTALL_DIR`.
2. Download `GhidraMCP-7.0.0.zip` and `ghidra_mcp_bridge-7.0.0-py3-none-any.whl` (release v7.0.0 of bethington/ghidra-mcp), check their hashes.
3. Bridge: `py -3.12 -m pip install <folder>\ghidra_mcp_bridge-7.0.0-py3-none-any.whl`.
4. Run `ghidraRun.bat`. Install the extension (*File > Install Extensions > +*, choose the zip), restart Ghidra.
5. `happy-decomp` project at the repository root, path (a) or (b) of SKILL.md §5. Path (b), with Ghidra closed:

   ```powershell
   & "$env:GHIDRA_INSTALL_DIR\support\analyzeHeadless.bat" C:\RE\happy-decomp happy-decomp `
       -import game\happy.exe game\maped.exe game\loader.exe
   ```

   [verified 2026-10-27, in a throwaway project] This import with the default analysis gives 220 / 200 / 63 functions, like the reference. When it fails, the `.bat` ends with a "Press any key" prompt: read the message that precedes it.
6. Reopen Ghidra, open the project then the three programs in CodeBrowser. Enable the GhidraMCP plugin (*File > Configure*), then *Tools > GhidraMCP > Start MCP Server*.

## 6. Working data

```powershell
winget install -e --id UPX.UPX
```

The commands of SKILL.md §6 work as is with `.venv\Scripts\python` in place of `PY`.

## x64dbg (optional)

`x64dbg` MCP server (port 9095, token `X64DBG_MCP_TOKEN`), to be started separately. It is not used during the inventory and is not checked by the diagnostic.
