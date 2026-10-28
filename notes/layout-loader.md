# Layout of `loader.exe`

Reference map of the custom-game loader for reading or modifying `src/`. Same conventions as `notes/layout-happy.md`; CRT names [verified] in `notes/data/crt-loader.csv` and `src/crt/loader.h`. §3 also holds the compiler, linker and CRT facts common to the three exes.

## 1. Summary

| Range | Contents |
|---|---|
| `0x00401000`–`0x004014CF` | game code: `main` (`0x00401000`, 1,172 bytes) and the `END_OF_MAIN()` stub `WinMain` (`0x004014A0`), 16-byte aligned with `0x90` padding |
| `0x004014D0`–`0x0040438D` | static `LIBC.LIB` (67 functions, from `_strncpy`) and the `RtlUnwind` thunk `0x00404220`; not decompiled. End of `.text` (VirtualSize `0x338E`) |
| `0x00406000`–`0x00406C3B` | `.data` and `.bss` (§4); no game data in `.rdata` |

- [verified] The 18 `alleg40.dll` imports are used by `main`, except `_WinMain` (the stub). No game function calls a Win32 import directly.
- [verified] No stdio and no floating point: `main` reads and writes its files through Allegro packfiles. It calls three CRT functions: `__chkstk` (`0x00401005`, frame of `0x2810` bytes), `_strncpy` (`0x0040105C`), `_chdir` (`0x0040106E`, called as `chdir`).
- [verified] Whole-exe check: the objects of `build.py --final loader`, linked without `/DEBUG`, give `.text`, `.rdata` and `.data` byte-identical to the original; only `.rsrc` (the icon) is missing (`notes/matching.md` §3).

## 2. From the entry point to `main`

[verified] Same CRT startup as happy (`notes/layout-happy.md` §2): `_WinMainCRTStartup` `0x004017AF` (scope table `0x004050F8`, `__except_handler3` `0x004023A0`), `GetVersion` → `0x00406718`–`0x00406724`, `__heap_init(0)` `0x00401806` → `0x0040226A` (failure: `_fast_error_exit` `0x004018CA`), `__ioinit` `0x004020BF`, `GetCommandLineA` → `[0x00406C24]`, `___crtGetEnvironmentStringsA` `0x00401F8D`, `__setargv` `0x00401D40`, `__setenvp` `0x00401C87`, `__cinit` `0x004015CE`, `__wincmdln` `0x00401C2F`; `0x00401878`: `call WinMain`, result to `exit` `0x004015FB`.

`WinMain` = `0x004014A0`: `END_OF_MAIN()` stub, `_WinMain` at IAT `0x004050A8`; same masked bytes as happy `0x0040A970` and maped `0x00405C70`.

`main` = `0x00401000` (`src/loader/block_01.c`), all locals, no global:
- `allegro_init` (`errno` `0x0040670C`, `atexit` `0x0040173D`), `install_keyboard`, `install_mouse`;
- the list of maps is `argv[1]` (`strncpy`, `0x0040103A`), or a `.txt` picked with `file_select_ex("Select set of levels to load", …)` after `chdir("maps")`, in a 640×480 window (Cancel returns 0);
- the list is read with `pack_fopen(…, "r")` and `pack_fgets`: lines are skipped up to a first line starting with `#`, then each line up to the next `#` names a regular map (at most 18), then each line up to a third `#` a bonus map (at most 8). Each name is taken relative to the folder of the list (`replace_filename`) and must `exists`;
- errors through `allegro_message`, return codes: 2 the list cannot be opened (`"Could not open file \"%s\"\n"`), 3 a map does not exist (`"Can't find file \"%s\"\n"`), 4 too many maps (`"Only 18 regular maps allowed\n"`, `"Only 8 bonus maps allowed\n"`), 5 end of file before the closing `#` (`"Unexpected end of file\n"`). The list is not closed on these paths;
- writes `happy.clv` next to the exe (`get_executable_name` + `replace_filename`, `0x004013D3`–`0x0040144E`): `pack_fopen(…, "w")` (result not checked), `pack_fwrite(slots, 0x2206)`, `pack_iputw(num_normal)`, `pack_iputw(num_bonus)`, `pack_fclose`;
- `allegro_message("%d normal map%s and %d bonus map%s loaded. Ready to play.\n", …)` with `""` or `"s"`, return 0.

### Format of `happy.clv` [verified from the code of loader and happy]

| Offset | Size | Contents | loader | happy (`custom_game` `0x00409F30`) |
|---|---:|---|---|---|
| `0x0000` | `0x178E` | 18 regular map slots | 1st part of `pack_fwrite` | `DAT_004404e0[18]` (`fread` `0x00409F88`, `fwrite` `0x0040A7BB`) |
| `0x178E` | `0x0A78` | 8 bonus map slots | 2nd part | `DAT_0043f5c0[8]` (`0x00409F9A`, `0x0040A7CD`) |
| `0x2206` | 2 | number of regular maps, 16-bit little-endian | 1st `pack_iputw` | `DAT_004450a8` (`0x00409FA9`, `0x0040A7DC`) |
| `0x2208` | 2 | number of bonus maps | 2nd `pack_iputw` | `DAT_0042f568` (`0x0040A7EE`) |

- Total 8,714 bytes. A slot is 335 bytes (stride `0x14F` of the copy loops, `0x004011F0`–`0x00401247`): `char file[255]`, the `.map` path checked with `exists`, then `char name[80]`, the line of the list as read by `pack_fgets(line, 79, f)` (`MAPSLOT` in `src/loader/block_01.c`, `T_0043f5c0` in `src/happy/happy.h`).
- Written in the clear: loader does not import `packfile_password`; happy reads it with `fopen("happy.clv", "rb")` (`0x00409F6D`).

## 3. Compiler, linker, CRT (three exes)

Measured on the bytes of the three exes [verified for the values; the Rich `prodid` names are hypotheses from public comp.id tables]. Toolchain chosen for the rebuild and its evidence: `notes/toolchain-vc6.md` (VC 6.0 without SP, `cl /O2`, §7).

| PE field | happy.exe | maped.exe | loader.exe |
|---|---|---|---|
| TimeDateStamp (UTC) | 2003-10-27 18:16:21 | 2003-09-14 11:36:57 | 2003-10-27 16:07:35 |
| `.text` / `.rdata` / `.data` VirtualSize | `0xFD35` / `0xB64` / `0x35C44` | `0xB617` / `0xB82` / `0x1E474` | `0x338E` / `0x87C` / `0xC3C` |

Common to the three [verified]: linker version 6.00; OS and subsystem 4.0, GUI; section and file alignment `0x1000`; stack and heap `0x100000` reserved / `0x1000` committed; `Characteristics` `0x10F` (relocations stripped); `CheckSum` 0; directories: imports, resources, IAT only (no debug directory, no PDB).

| prodid | Name | build | happy | maped | loader |
|---|---|---|---:|---:|---:|
| 10 | Utc12_C | 8168 | 81 | 81 | 29 |
| 11 | Utc12_CPP | 8168 | 1 | 1 | 1 |
| 4 | Linker600 | 8447 | 3 | 3 | 3 |
| 14 | Masm613 | 7299 | 13 | 13 | 9 |
| 12 | AliasObj60 | 7291 | 2 | 1 | 1 |
| 19 | Linker512 | 8034 | 2 | 2 | 2 |
| 1 | Import0 | 0 | 120 | 117 | 60 |
| 6 | Cvtres500 | 1720 | 1 | 1 | 1 |

- Same tools in the three exes: cl 12.00.8168, masm 6.13.7299, cvtres 5.00.1720. `Linker600 8447` is Allegro's import library, not the game's linker, whose build is unknown (`notes/toolchain-vc6.md` §3). `Utc12_CPP ×1` comes from `LIBC.LIB`: the game is entirely C (`notes/toolchain-vc6.md` §5).
- `AliasObj60` counts the OLDNAMES aliases used: `chdir` in maped and loader [verified]; happy's two are an open question (`notes/layout-happy.md` §7, question 3).
- **CRT: static single-threaded `LIBC.LIB` (`/ML`)** in the three exes [strong hypothesis; the local `LIBC.LIB` is byte-identical to the game's CRT code, `notes/toolchain-vc6.md` §4]. Evidence, all [verified]: no TLS, critical-section or thread import; `FS:[0]` (SEH) as the only `FS:` access (12 in each exe); no `lock` prefix; `__heap_init(0)` directly followed by `__ioinit` (happy `0x0040B55D`, maped `0x004063F8`, loader `0x00401806`); `errno` a plain global passed as an immediate (happy `0x00413B3C` at `0x00401F0E`, maped `0x00410134` at `0x00405AF0`, loader `0x0040670C` at `0x00401017`).
- Code traits identical in the three exes [verified]: game functions 16-byte aligned with `0x90` padding (`/Gy`); no frame pointer except in happy functions holding Allegro inline asm; inline `rep movs`/`repne scasb` (`/Oi`); string literals pooled in `.data` (`/Gf`). No option difference is observable between the exes.

## 4. Data layout

| Range | Owner | Contents |
|---|---|---|
| `0x004050F8`–`0x00405450` (`.rdata`) | CRT | scope table of the entry point… |
| `0x00406000`–`0x0040602F` | CRT | init/term pointer tables |
| `0x00406030`–`0x00406133` | game | the strings of `main`, in reverse order of use: the final message first (`0x00406030`), `"maps"` last (`0x00406130`); `"w"` `0x00406070`, `"r"` `0x00406108`, `"s"` `0x0040606C` |
| `0x00406140`–`0x00406707` | CRT | CRT `.data`, ending with the 45 pairs of `__dosmaperr` (`0x004042AC`) |
| `0x00406708` | game | `""` of `main` (`0x00401456`, `0x00401466`) |
| `0x0040670C` | CRT | `errno`, passed by `allegro_init` and written by `__dosmaperr` |
| `0x00406710`–`0x00406C3B` | CRT | CRT `.bss` |

- [verified] The game has no global: `main` references only its strings, `""` and `errno`. `src/loader/data.c` only annotates `errno`.
- [verified with a test object] cl 12 emits the literal `""` as a 1-byte COMDAT in `.bss` (uninitialised, all zero), and the linker places it first in `.bss`, before `errno`. That is why it lies outside the game's strings; it is not merged with a CRT string.

## 5. Blocks (source files)

| Block | Source | Range | Funcs | Contents | Start |
|---|---|---|---:|---|---|
| block_01 | `src/loader/block_01.c` | `0x00401000`–`0x004014CF` | 2 | `main`, `WinMain` | strong (start of `.text`) |

[strong hypothesis] One `.c` file: `END_OF_MAIN()` puts `WinMain` right after `main`, nothing else precedes the CRT, and the game has no globals or helpers. Consistent with the Rich header: 29 `Utc12_C` objects = 1 game object + 28 from `LIBC.LIB` [hypothesis]. The CRT follows from `_strncpy` (`0x004014D0`); its member order differs from happy's and maped's (`__chdir` and what it pulls in, `__dosmaperr`, `__mbctoupper` `0x00404313`, come last, after `_memset`, because of the OLDNAMES alias, `notes/matching.md` §3).

## 6. Code shared with the other exes

- [verified] No game function is shared, apart from the `END_OF_MAIN()` stub.
- [verified] 62 CRT functions are identical in the three exes; `_chdir` (`0x00404226`) and `__mbctoupper` (`0x00404313`) are shared with maped only. One 34-byte CRT function, `0x0040237E`, is not found in `LIBC.LIB` (`notes/matching.md` §4), like happy `0x0040DF22` and maped `0x00409652`.
- loader writes `happy.clv`, which happy's `custom_game` reads and rewrites (§2).
