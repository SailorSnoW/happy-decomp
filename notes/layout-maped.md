# Layout of `maped.exe`

Reference map of the map editor for reading or modifying `src/`. Same conventions as `notes/layout-happy.md`: names are those of `src/`, CRT names are [verified] by byte identity with `LIBC.LIB` (`notes/data/crt-maped.csv`, `src/crt/maped.h`).

## 1. Summary

| Range | Contents |
|---|---|
| `0x00401000`–`0x00405C9F` | game code: 50 functions in 7 blocks (§5), 16-byte aligned with `0x90` padding |
| `0x00405CA0`–`0x00405CC9` | 7 Allegro import thunks `jmp [IAT]` (block_08, §5) |
| `0x00405CCA`–`0x0040C616` | static `LIBC.LIB` (165 functions) and the `RtlUnwind` thunk `0x0040C510`; not decompiled. End of `.text` (VirtualSize `0xB617`) |
| `0x0040D1C8` (`.rdata`) | the game's only `.rdata` datum: the `double` 0.0625 of `FUN_00401400` (declared in `reccmp/maped.csv`) |
| `0x0040E000`–`0x0042C473` | `.data`, `.bss` and COMMON (§4) |

- [verified] The 64 `alleg40.dll` imports are all used by game code (57 directly, 7 through the thunks); no game function calls a Win32 import directly. The only Allegro header code is the `END_OF_MAIN()` stub `WinMain` (`0x00405C70`): maped uses no `al386vc.h` inline, hence no `_set_errno_erange`.
- [verified] The game calls these CRT functions: `free`, `_malloc`, `fopen`, `fclose`, `fread`, `fwrite`, `fseek`, `rand`, `__ftol`, `sprintf`, `atoi`, `_chdir` (called as `chdir`). Specific to maped among the three exes: the `printf` family (`_sprintf` `0x0040607D`, `__output` `0x004081C3`), `atol`, `__allmul`.
- [verified] No `__FILE__` or source file name in the strings.

## 2. From the entry point to `main`

[verified] Same CRT startup as happy (`notes/layout-happy.md` §2), at other addresses: `_WinMainCRTStartup` `0x004063A1` (scope table `0x0040D1D0`, `__except_handler3` `0x00409674`), `GetVersion` → `0x00410140`–`0x0041014C`, `__heap_init(0)` `0x004063F8` → `0x004064E0` (failure: `_fast_error_exit` `0x004064BC`), `__ioinit` `0x004093D1`, `GetCommandLineA` → `[0x0042C460]`, `___crtGetEnvironmentStringsA` `0x0040929F`, `__setargv` `0x00409052`, `__setenvp` `0x00408F99`, `__cinit` `0x004061F1`, `__wincmdln` `0x00408F41`; `0x0040646A`: `call WinMain`, result to `exit` `0x0040621E`.

`WinMain` = `0x00405C70` (`src/maped/block_07.c`): the `END_OF_MAIN()` stub, `_WinMain` at IAT `0x0040D108`, with `main`.

`main` = `0x00405AE0` (`src/maped/block_07.c`):
- `allegro_init` (`errno` `0x00410134`, `atexit` `0x00406360`); `get_executable_name`, `replace_filename`, `chdir` to the exe's folder;
- `init_editor`;
- with one argument (`argc == 2`): `map_load(argv[1])`, then `load_objects`, `map_load_datafile` (`data/tiles.dat` or `data/xtiles.dat` by bit 3 of `MAP.f79`), `FUN_004019b0`, `data/sprites.dat` or `data/xsprites.dat`, `init_color_tables`, `FUN_00401400`;
- `editor_loop` (`0x00404C10`), `shutdown_editor`, `return 0`.

## 3. Compiler, linker, CRT

Shared facts for the three exes: `notes/layout-loader.md` §3. Specific to maped [verified]:
- PE `TimeDateStamp` **2003-09-14 11:36:57 UTC**, six weeks before happy and loader. Rich header: `Utc12_C ×81`, `AliasObj60 ×1`, `Import0 ×117`.
- `/ML` evidence as in happy: no TLS, critical-section or thread import among the 48 KERNEL32 imports; `FS:[0]` only (12, all SEH); no `lock`; `__heap_init(0)` at `0x004063F8`; `errno` `0x00410134` pushed as an immediate at `0x00405AF0`.
- The three KERNEL32 imports happy lacks (`GetCurrentDirectoryA`, `SetCurrentDirectoryA`, `SetEnvironmentVariableA`) serve `_chdir` (`0x0040C516`).
- `AliasObj60 ×1` is the OLDNAMES alias `chdir`: calling `chdir` rather than `_chdir` puts `_chdir` and what it pulls in at the end of the CRT, as in the original (`notes/matching.md` §3).
- No game function has an `ebp` frame (no Allegro inline asm).
- CRT: 163 named functions. Not named: `0x00405FFF` (1-byte `ret`, `__fpclear` or `__rttosnpop`, like happy `0x0040B376`) and `0x00409652` (34 bytes, not found in `LIBC.LIB`) (`notes/matching.md` §4).
- [verified] The CRT member order differs from happy's (maped starts with `free`/`_malloc`, then `fclose`). [hypothesis] The linker pulls `LIBC.LIB` members in the order it resolves undefined symbols, which depends on the game's references; the rebuild reproduces all of them but one (§7, question 3).

## 4. Data layout

| Range | Owner | Contents |
|---|---|---|
| `0x0040E000`–`0x0040E02F` | CRT | init/term pointer tables (like happy `0x00412000`) |
| `0x0040E030`–`0x0040E04B` | map module | `"unnamed.dat"`, `"unnamed"`, `"rb"`, `"wb"` |
| `0x0040E050`–`0x0040EF1F` | game | initialised globals: `DAT_0040e050`, `DAT_0040e054` (-1), `file_menu` (`MENU[32]`, `0x0040E058`), `properties_dialog` (`0x0040E2D8`), `text_dialog` (`0x0040EA10`), `slider_values` (`0x0040EB60`), `colors_dialog` (`0x0040EB78`), then the menu literals (`"&Quit"` `0x0040EEEC`…) |
| `0x0040EF20`–`0x0040F58F` | game | string literals, from the help texts of `init_help_texts` to `editor_loop` |
| `0x0040F598`–`0x0041010F` | CRT | CRT `.data` |
| `0x00410110`–`0x0041012B` | game | game `.bss`: `DAT_00410110` (the current `MAP *`), `DAT_00410118`, `DAT_00410120`; `""` of `main` at `0x00410128` ([hypothesis] a 1-byte `.bss` COMDAT, as verified for loader, `notes/layout-loader.md` §4) |
| `0x00410130`–`0x00410324` | CRT | CRT `.bss` (`errno` `0x00410134`) |
| `0x00410340`–`0x0042B0E3` | game | COMMON: objects, `help_texts`, colour tables, palette, file names (`DAT_0042a440`: current map file), `map_datafile` (`0x0042B0E0`) |
| `0x0042B0E4`–`0x0042C473` | CRT | CRT COMMON [hypothesis] |

- [verified] The `proc` fields of the `DIALOG` arrays (stride `0x38` = `sizeof(DIALOG)`) point at the import thunks of block_08, and `colors_dialog` holds the two game procedures of block_02 (`slider_changed` as `dp2` of its `d_slider_proc` entries, `FUN_004015d0` as `proc` of entry `0x0040ED38`). These globals are defined in `src/maped/data.c`, with the thunks annotated as `LIBRARY` in an `#if 0` block.
- [verified] The map module's strings come first in `.data`, as its code comes first in `.text`; `"rb"` (`0x0040E044`) is also used by `load_objects`: pooled strings are merged across objects and sit with their first user. Within the second group, initialised globals precede the strings, as in happy.

## 5. Blocks (source files)

Same method and strength scale as `notes/layout-happy.md` §5. `scripts/build.py` links `common/block_09.c`, `maped/block_02.c`, `common/block_01.c`, then `maped/block_04.c`–`block_07.c`.

| Block | Source | Range | Funcs | Theme | Start |
|---|---|---|---:|---|---|
| block_01 | `src/common/block_09.c` | `0x00401000`–`0x004013FF` | 7 | map module | strong (start of `.text`) |
| block_02 | `src/maped/block_02.c` | `0x00401400`–`0x0040169F` | 3 | background gradient, colour sliders | strong (end of the map module) |
| block_03 | `src/common/block_01.c` | `0x004016A0`–`0x004018CF` | 2 | utilities shared with happy block_01 | medium |
| block_04 | `src/maped/block_04.c` | `0x004018D0`–`0x00401D8F` | 7 | screenshot, graphics mode, init and shutdown | weak |
| block_05 | `src/maped/block_05.c` | `0x00401D90`–`0x004031FF` | 10 | map and interface display, image export | weak |
| block_06 | `src/maped/block_06.c` | `0x00403200`–`0x00403EAF` | 9 | map files and dialogs | weak |
| block_07 | `src/maped/block_07.c` | `0x00403EB0`–`0x00405C9F` | 12 | editing tools, editor loop, `main`, `WinMain` | weak (end: strong) |
| block_08 | (`alleg40.lib`) | `0x00405CA0`–`0x00405CC9` | 7 | import thunks | strong |

### block_01 — `0x00401000`–`0x004013FF` (map module)
`map_clear`, `map_create`, `map_copy` (`0x00401160`, maped only; caller `shift_map`), `map_destroy`, `map_load`, `map_load_datafile`, `map_save` (`0x004013A0`, maped only: header and tiles, `"wb"`). [verified] Five functions identical to happy (§6). The strings of `map_save` follow the module's other strings and precede the next object's globals: the end of the block is strong.

### block_02 — `0x00401400`–`0x0040169F`
`FUN_00401400` (colours 176-191 as a gradient between two `MAP` colours, `set_palette`; 5 callers; same gradient as happy `init_level_palette`), `slider_changed` (`0x004014E0`, `dp2` callback of the sliders), `FUN_004015d0` (dialog procedure wrapping `d_slider_proc`). The last two are reached only through `colors_dialog` (§4).

### block_03 — `0x004016A0`–`0x004018CF`
`FUN_00401160` (`0x004016A0`) and `init_help_texts` (`0x004017E0`), identical to happy `0x00401160` and `0x004017C0`. [verified] Adjacent here, separated in happy by four functions maped does not use; the strings of `init_help_texts` open the second string group (`0x0040EF20`), as in happy. Medium start: the sharing with happy is the only evidence.

### block_04 — `0x004018D0`–`0x00401D8F`
`take_screenshot` (`"maped%03d.%s"`, `output_format`, `packfile_password("happy")`), `FUN_004019b0` (tile counts per editing mode), `set_screen_mode` (`run_in_a_window`, `"autosave.map"` through `map_save`), `init_color_tables`, `load_data` (only caller `init_editor`), `init_editor` (`0x00401C20`: `set_config_file("maped.ini")`, `"./maps/"`), `shutdown_editor` (`0x00401D20`). [hypothesis] Same sequence as happy block_02, skeleton copied then modified (§6).

### block_05 — `0x00401D90`–`0x004031FF`
`draw_triangle`, `draw_tile_mask`, `draw_tile_type` (`"SNOW"`, `"ICE"`, `"DOOR"`, `"EXIT"`…), `draw_background`, `get_help_text`, `draw_objects`, `export_map_image` (`"%s saved"`, `show_*` options of `maped.ini`), `draw_editor` (`0x00402900`, status line `"Save:%s        Name:%s       Time:%d"`), `FUN_00403140` (empty, a single `ret`; called when F1 is held, `0x004059AD`), `edit_colors` (`do_dialog(colors_dialog)`). [verified] Single-caller pairs: `draw_triangle` → `draw_tile_mask`, `get_help_text` → `draw_objects`.

### block_06 — `0x00403200`–`0x00403EAF`
`load_objects` (objects and help texts after the tiles, `fseek`), `load_map` (`file_select`, `"LOAD ERROR"`), `edit_properties` (`popup_dialog(properties_dialog)`, `sprintf`, `atoi`), `new_map` (`"./maps/unnamed.map"`), `show_message`, `save_objects` (appends objects and help texts, `"ab"`; only caller `save_map`), `yes_no_box`, `save_map` (`"Overwrite "`, `"Saving..."`), `save_map_as`.

### block_07 — `0x00403EB0`–`0x00405C9F`
`shift_map`, `resize_map`, `wait_arrows_released`, `rand_range`, `add_object`, `select_help_text` (`do_dialog(text_dialog)`), `auto_mask`, `check_masks` (`"Tile masks are ok!"`), `pick_tile`, `editor_loop` (`0x00404C10`, 3,792 bytes: `do_menu(file_menu)`, `"Really revert?"`, `"Screenshot taken!"`), `main`, `WinMain`.
- [verified] The nine tools have `editor_loop` as only caller, which follows them; `editor_loop` has `main` as only caller; `END_OF_MAIN()` puts `WinMain` right after `main`.
- The start is weak: eight earlier functions (`take_screenshot`, `export_map_image`, `draw_editor`, `FUN_00403140`, `edit_colors`, `load_map`, `new_map`, `save_map_as`) also have `editor_loop` as only caller.

### block_08 — `0x00405CA0`–`0x00405CC9` (import thunks)
`d_check_proc`, `d_radio_proc`, `d_edit_proc`, `d_text_proc`, `d_button_proc`, `d_ctext_proc`, `d_shadow_box_proc`, 6 bytes each, never called, referenced only by the `DIALOG` initialisers. [hypothesis] A static initialiser cannot use the `__imp_` address of a `dllimport` function, so it references the import library's stub, which the linker adds after the game objects. happy has no static `DIALOG` array and no such thunk.

## 6. Code shared with happy

Identical code (masked bytes) [verified], one source in `src/common/` with one `// FUNCTION:` line per exe:

| maped | happy | Source |
|---|---|---|
| `0x00401000`, `0x00401090`, `0x00401260`, `0x00401290`, `0x00401370` | `0x0040A9A0`, `0x0040AA30`, `0x0040AB00`, `0x0040AB30`, `0x0040AC10` | `src/common/block_09.c` (map module) |
| `0x004016A0`, `0x004017E0` | `0x00401160`, `0x004017C0` | `src/common/block_01.c` |
| `0x00405C70` | `0x0040A970` | `END_OF_MAIN()` (also loader `0x004014A0`) |

[hypothesis] Each shared source is linked into both exes and `/OPT:REF` drops what an exe does not call (`/O2` implies `/Gy`, `notes/toolchain-vc6.md` §5). The map module is maped's first object and happy's last game object.

Look-alike functions, same imports and order but different code [hypothesis: copied then adapted], written once per exe:

| maped | happy | Common points |
|---|---|---|
| `init_color_tables` `0x00401B90` | `init_color_tables` `0x00401A30` | `create_rgb_table`, `create_light_table` |
| `shutdown_editor` `0x00401D20` | `shutdown_game` `0x00402120` | `fade_out`, `unload_datafile`, `allegro_exit` |
| `load_objects` `0x00403200` | `load_level_objects` `0x00403070` | objects and help texts after the tiles of a `.map` |
| `FUN_00401400` | `init_level_palette` `0x00402EA0` | gradient of colours 176-191, the `.rdata` constant 0.0625 |

File formats: `.map` header and tiles in `src/include/map.h` and `notes/game-docs.md` §4; the objects and help texts that follow them in `load_objects` / `save_objects` (and happy `load_level_objects`).

## 7. Open questions

None affects a function score.

1. **One `.c` file or several** for `0x00401400`–`0x00405C9F`: not observable, as happy question 1 (`notes/layout-happy.md` §7).
2. **End of the source shared with happy block_01**: whether maped block_02 or block_04 belongs to the same file is not observable; `src/` keeps them separate and all match.
3. **`__ld12tod`** is the only CRT function not at its original address: `0x0040A81F` in the original, `0x0040A809` in the rebuild.
4. **Two unnamed CRT functions**: `0x00405FFF` and `0x00409652` (§3).
