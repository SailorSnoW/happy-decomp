# macOS Apple Silicon sheet

**Status: not yet tested end to end on a Mac.** The commands below follow the tools' documentation. The diagnostic (`python3 scripts/doctor.py`) is the judge: every discrepancy encountered must be reported to the user, with a proposed fix to this sheet.

Principle: Ghidra, Python, reccmp and capstone run natively (arm64). VC6 (32-bit Windows exes) and the `cvdump.exe` bundled with reccmp run under **Wine**, itself translated by **Rosetta 2**. reccmp calls `wine` and `winepath` by name when it is not running on Windows (`reccmp/cvdump/runner.py`, `reccmp/dir.py`): both must be in the PATH.

Recommended layout: `~/RE/happy-decomp` (the clone), `~/RE/tools/vc6`, `~/RE/tools/ghidra_12.1.4_PUBLIC`. Avoid `~/Documents` and `~/Desktop`, often synchronised by iCloud: the Ghidra project and `captures/` have no business being there.

## 1. System

To be run by the user (interactive or `sudo`), with the `!` prefix in Claude Code:

```sh
xcode-select --install                                   # command-line tools (git, clang)
softwareupdate --install-rosetta --agree-to-license      # Rosetta 2, for Wine
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Then:

```sh
brew install uv upx
uv python install 3.12
```

Python to run the diagnostic before the venv exists: the `python3` from the Xcode tools is enough (doctor requires 3.8 or later, and uses only the standard library). The project itself uses the venv's Python 3.12.

### Wine

- The Homebrew cask `wine-stable` has been **disabled since 2026-09-01**: Homebrew removes casks that fail the Gatekeeper checks, and the `--no-quarantine` option no longer exists. Do not use `brew install --cask wine-stable`.
- Take a build from **Gcenx**: github.com/Gcenx/macOS_Wine_builds, releases page. Pick the most recent `wine-stable-<version>-osx64.tar.xz` archive (failing that, `wine-devel-…`), version 10 or later: WoW64 mode, which runs 32-bit exes, is built in. List the release's files before downloading (`gh release view -R Gcenx/macOS_Wine_builds`, or the web page). The exact name is not fixed here.
- Installation, in a fresh download folder:

  ```sh
  mkdir -p ~/Applications
  tar -xJf wine-stable-<version>-osx64.tar.xz -C ~/Applications      # gives "Wine Stable.app"
  xattr -dr com.apple.quarantine ~/Applications/"Wine Stable.app"
  echo 'export PATH="$HOME/Applications/Wine Stable.app/Contents/Resources/wine/bin:$PATH"' >> ~/.zprofile
  ```

  Open a new terminal. Check: `wine --version` and `winepath -w "$HOME"`, which must print a `Z:\…` path. The first run creates the `~/.wine` prefix and takes up to a minute.
- If the archive contains only `wine64` and not `wine`, it is too old: take a more recent one.
- The variables `WINEDEBUG=-all` and `WINEDLLOVERRIDES=mscoree,mshtml=` (no prompt to install Mono or Gecko) are set by `scripts/vc6.py`. For reccmp, also export them in `~/.zprofile` if `cvdump` prints noise or opens a window.

## 2. Repository and copy of the game

```sh
mkdir -p ~/RE && cd ~/RE && git clone <repository url> happy-decomp
```

Copy `game/` from the Windows machine (zip archive, USB stick, `scp`…), in full. The diagnostic checks the hashes.

## 3. Python tools

```sh
uv venv --python 3.12 .venv
uv pip install --python .venv/bin/python -r requirements.txt
uv tool install --python 3.12 reccmp==0.1.7
uv tool update-shell        # puts ~/.local/bin in the PATH; open a new terminal
```

If a reccmp dependency (`pydemumble`, `pyghidra`) has no arm64 wheel and gets compiled, the Xcode tools are needed (§1).

## 4. VC6

- Copy the folder from the Windows machine (`C:\RE\tools\vc6`) to `~/RE/tools/vc6`. The useful minimum: `VC98/` and `COMMON/MSDEV98/BIN/`. Keep the upper-case names as they are. The default APFS file system is case-insensitive, and `scripts/vc6.py` tolerates case differences too.
- `python3 scripts/doctor.py` should then print `cl runs: 12.00.8168` and `link runs: 6.00.8168`. If it fails on `cl`, run it by hand to see Wine's message:

  ```sh
  .venv/bin/python scripts/vc6.py cl
  ```

## 5. Ghidra and GhidraMCP

```sh
brew install --cask temurin@21
echo 'export JAVA_HOME="$(/usr/libexec/java_home -v 21)"' >> ~/.zprofile
```

1. Download `ghidra_12.1.4_PUBLIC_20260921.zip` (release `Ghidra_12.1.4_build`), check its SHA-256 (`shasum -a 256`), then:

   ```sh
   unzip ghidra_12.1.4_PUBLIC_20260921.zip -d ~/RE/tools
   xattr -dr com.apple.quarantine ~/RE/tools/ghidra_12.1.4_PUBLIC
   ```

2. The diagnostic checks `Ghidra/Features/Decompiler/os/mac_arm_64/decompile`. If it is missing: `brew install gradle`, then `~/RE/tools/ghidra_12.1.4_PUBLIC/support/buildNatives`.
3. Download `GhidraMCP-7.0.0.zip` and `ghidra_mcp_bridge-7.0.0-py3-none-any.whl` (release v7.0.0 of bethington/ghidra-mcp), check the hashes, then `uv tool install --python 3.12 ./ghidra_mcp_bridge-7.0.0-py3-none-any.whl`.
4. `~/RE/tools/ghidra_12.1.4_PUBLIC/ghidraRun`. Install the extension (*File > Install Extensions > +*), restart Ghidra. The extension is installed into `~/Library/ghidra/ghidra_12.1.4_PUBLIC/Extensions/`.
5. `happy-decomp` project, path (a) or (b) of SKILL.md §5. Path (b), with Ghidra closed, from the repository root:

   ```sh
   ~/RE/tools/ghidra_12.1.4_PUBLIC/support/analyzeHeadless "$PWD" happy-decomp \
       -import game/happy.exe game/maped.exe game/loader.exe
   ```

6. Reopen Ghidra, open the three programs in CodeBrowser, enable the plugin and start the MCP server.
7. Claude Code: `.mcp.json` launches `bridge-mcp-ghidra` by name. If `/mcp` reports that the command cannot be found, start Claude Code from a terminal where `which bridge-mcp-ghidra` answers (the PATH of an application started from the Dock does not include `~/.local/bin`). Do not modify `.mcp.json`, which is shared. The `x64dbg` failure is normal on a Mac.

## 6. Working data

Commands of SKILL.md §6 with `PY=.venv/bin/python`. For step 5 (`alleg40.lib`), under zsh:

```sh
mkdir -p build
python3 scripts/vc6.py lib /NOLOGO '/DEF:3rdparty\allegro-4.0.3\lib\msvc\allegro.def' /MACHINE:IX86 /NAME:alleg40.dll '/OUT:build\alleg40.lib'
```

If the hash of `captures/alleg40_unpacked.dll` differs (upx of a version other than 5.2.1), report it. `pe_allegro.py` reads its exports and `inventory.py` searches it for code (`scripts/inventory.py`, `find_in` on the DLL): step 4 of §6 (identical CSVs) will tell whether the difference matters.
