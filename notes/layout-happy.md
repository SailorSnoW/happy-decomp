# Layout of `happy.exe`

Reference map of the game exe for reading or modifying `src/`. Names are those of `src/` (descriptive names are hypotheses justified in the source comments; `FUN_`/`DAT_` otherwise). CRT names are [verified] by byte identity with the game's `LIBC.LIB` (`notes/data/crt-happy.csv`, annotations in `src/crt/happy.h`).

## 1. Summary

| Range | Contents |
|---|---|
| `0x00401000`–`0x0040AD4F` | game code: 84 functions in 9 blocks (§5), all 16-byte aligned with `0x90` padding (`/Gy` COMDATs, `notes/toolchain-vc6.md` §5) |
| `0x0040AD50`–`0x00410D34` | static `LIBC.LIB` (154 functions, unaligned) and the `RtlUnwind` thunk `0x00410C10`; not decompiled. End of `.text` (VirtualSize `0xFD35`) |
| `0x004111E0` (`.rdata`) | the game's only `.rdata` datum: the `double` 0.0625 of `init_level_palette` (declared in `reccmp/happy.csv`) |
| `0x00412000`–`0x00447C43` | `.data`, `.bss` and COMMON (§4) |

- [verified] Game/CRT boundary `0x0040AD50`: first CRT function `_rand`; no function after it imports Allegro; in `.data`, the game's last string (`"unnamed"`, `0x00412F88`) is followed by `rand`'s seed (`0x00412F90`), the first CRT datum.
- [verified] Two of the 84 game functions are Allegro header code, annotated as library: the `END_OF_MAIN()` stub `WinMain` (`0x0040A970`) and the out-of-line `_set_errno_erange` (`0x00402110`, from `al386vc.h`).
- [verified] The 72 `alleg40.dll` imports are all used by game code; no game function calls a Win32 import directly. The game calls these CRT functions: `rand`, `fopen`, `fclose`, `fread`, `fwrite`, `fseek` (`0x0040B3D7`), `exit`, `__strcmpi` (`0x00410C20`, called as `strcmpi`/`stricmp`), `free`, `_malloc`, `_itoa` (called as `itoa`), `__ftol`.
- [verified] No `__FILE__`, `assert` message or source file name in the strings: blocks have neutral names (`block_NN`).

## 2. From the entry point to `main`

All steps [verified] (disassembly; CRT names from `crt-happy.csv`):

1. `AddressOfEntryPoint` → `_WinMainCRTStartup` `0x0040B506`: SEH frame (scope table `0x004111E8`, `__except_handler3` `0x0040DF44`); `GetVersion` → `0x00413B48`–`0x00413B54`.
2. `0x0040B55D`: `__heap_init(0)` (`0x0040CCA2`); on failure `_fast_error_exit(0x1C)` (`0x0040B621`, R6028).
3. `__ioinit` (`0x0040C233`); `GetCommandLineA` → `[0x00446C24]`; `___crtGetEnvironmentStringsA` (`0x0040DD1A`), `__setargv` (`0x0040DACD`), `__setenvp` (`0x0040DA14`), `__cinit` (`0x0040B1AF`).
4. `GetStartupInfoA`, `__wincmdln` (`0x0040D9BC`); `nCmdShow` = `wShowWindow` if `STARTF_USESHOWWINDOW`, else `SW_SHOWDEFAULT`.
5. `0x0040B5CF`: `WinMain(GetModuleHandleA(NULL), NULL, lpCmdLine, nCmdShow)`; its result goes to `exit` (`0x0040B1DC`).

`WinMain` = `0x0040A970` (37 bytes, `src/happy/block_08.c`): Allegro's `END_OF_MAIN()` (`platform/alwin.h:44-48`, `notes/allegro.md` §5.4). It calls `_WinMain` (IAT `0x00411120`, ordinal 748) with `_mangled_main` = `main`; `ret 0x10`.

`main` = `0x0040A830` (`src/happy/block_08.c`; `allegro.h` renames it `_mangled_main`):
- command line, compared with `strcmpi`: `-widescreen` (`0x00412F70`) sets `screen_mode = 1`; `-fps` (`0x00412F68`) sets `DAT_00413b10`, which nothing reads [verified];
- `init_game`, then `start_menu` until a game is chosen (`custom_game` for a custom set);
- `play_intro` if `DAT_00413b20` (the band picked in `start_menu`) is 0, otherwise `show_logo`;
- menu loop: `main_menu` / `main_menu_password` → `play_game` (with `enter_password` first for the password entry), `show_instructions`, or exit;
- `shutdown_game`, `return 0`.

## 3. Compiler, linker, CRT

Shared facts for the three exes (PE and Rich headers, `/ML` evidence, options): `notes/layout-loader.md` §3; toolchain: `notes/toolchain-vc6.md`. Specific to happy [verified]:
- PE `TimeDateStamp` 2003-10-27 18:16:21 UTC. Rich header: `Utc12_C ×81`, `AliasObj60 ×2`, `Import0 ×120`.
- `/ML` evidence: no TLS, critical-section or thread import among the 45 KERNEL32 imports; `FS:[0]` (SEH) only, in `0x0040B506`, `0x0040DE8E`, `0x0040DEF6`, `0x0040FC24`, `0x0040FD6D`; no `lock` prefix; `__heap_init(0)` directly followed by `__ioinit`; `errno` is a global (`0x00413B3C`, pushed as an immediate by `allegro_init` at `0x00401F0E`).
- 9 game functions keep an `ebp` frame: `0x00401F00`, `0x004021A0`, `0x00402260`, `0x00402620`, `0x00403350`, `0x00403F60`, `0x004041F0`, `0x00404480`, `0x00404C50`. [hypothesis] VC6 drops `/Oy` in functions that contain the inline asm of `al386vc.h` (`fixmul`, `fixdiv`, `fixfloor`, …); six of them show such code.
- CRT: 152 named functions, plus one alias. Not named: `0x0040B376` (1-byte `ret`, `__fpclear` or `__rttosnpop`, ambiguous) and `0x0040DF22` (34 bytes after `__local_unwind2`, not found in `LIBC.LIB`) (`notes/matching.md` §4).

## 4. Data layout

| Range | Owner | Contents |
|---|---|---|
| `0x00412000`–`0x0041202F` | CRT | init/term pointer tables (`__xi_a`…`__xt_z` [hypothesis]) |
| `0x00412030`–`0x00412093` | game | initialised globals: `screen_mode` (= 32), `passwords[21]` (`0x00412034`), `DAT_00412088` (1), `DAT_0041208c` (1), `DAT_00412090` (5) |
| `0x00412094`–`0x0041221B` | game | the 21 password strings, in reverse order of the array, each character + 1 (`"IBYF.GVMMDBQBDJUZ"` = `"HAXE-FULLCAPACITY"`) |
| `0x0041221C`–`0x00412F8F` | game | string literals, starting with the help texts of `init_help_texts`; local array `0x00412C5C` of `play_ending` |
| `0x00412F90`–`0x00413B0F` | CRT | CRT `.data` |
| `0x00413B10`–`0x00413B37` | game | game `.bss` (globals `= 0` or `static`): `DAT_00413b10` (`-fps`), `use_joystick`, `DAT_00413b20` (band picked in `start_menu`)… |
| `0x00413B38`–`0x00413D3B` | CRT | CRT `.bss` (`errno` `0x00413B3C`) |
| `0x00413D3C`–`0x004468BF` | game | COMMON (uninitialised globals): `hiscores`, `keys`, `player` (`0x0042F500`), `buffer`, `help_texts`, `particles`, `snow`, `map_datafile` (`0x004450B4`), colour tables… |
| `0x004468C0`–`0x00447C43` | CRT | CRT COMMON [hypothesis] |

- [verified with test programs, `notes/matching.md` §3] The linker places COMMON symbols in an order that depends on their (unknown) names, so the COMMON layout cannot be reproduced; `src/happy/data.c` defines every game global in address order and `scripts/reccmp_run.py` compares end-of-array operands by offset.
- [verified] String literals are pooled across the whole program (`/Gf`, writable, in `.data`): a shared string (`"rb"` `0x00412500`, `"wb"` `0x004124FC`, `"%s"`, `"EXIT"`, `"data/tiles.dat"`) sits with its first user. From one function to the next, strings follow function order; within a function they are in **reverse** order of use (`play_intro`: `"It's the night before Christmas..."` is last, `0x00412A04`). Exception: `"OK"` (§7, question 2).

## 5. Blocks (source files)

Blocks were cut before decompilation from single-caller chains (a `static`-like helper sits just before its only caller), themes (strings, imports) and two strong anchors (`END_OF_MAIN` and the map module shared with maped). Only the boundaries marked strong are evidenced; the others are a change of theme. `scripts/build.py` links the blocks in this order; with `--final` the game code ends at `0x0040AD50`, as in the original (`notes/matching.md` §5).

| Block | Source | Range | Funcs | Theme | Start |
|---|---|---|---:|---|---|
| block_01 | `src/common/block_01.c` | `0x00401000`–`0x004018AF` | 7 | tables, input, screen display, message box, help texts | strong (start of `.text`) |
| block_02 | `src/happy/block_02.c` | `0x004018B0`–`0x0040219F` | 16 | timers, init and shutdown, settings, small screens | weak |
| block_03 | `src/happy/block_03.c` | `0x004021A0`–`0x0040302F` | 5 | level and HUD rendering, palette | weak |
| block_04 | `src/happy/block_04.c` | `0x00403030`–`0x0040344F` | 5 | loading and resetting a level | weak |
| block_05 | `src/happy/block_05.c` | `0x00403450`–`0x0040639F` | 19 | gameplay, game loop, bonus room | weak |
| block_06 | `src/happy/block_06.c` | `0x004063A0`–`0x004074FF` | 8 | title, controls, options, intro | weak |
| block_07 | `src/happy/block_07.c` | `0x00407500`–`0x0040986F` | 12 | menus, passwords, scores, game over, ending, game flow | weak |
| block_08 | `src/happy/block_08.c` | `0x00409870`–`0x0040A99F` | 5 | start menu, custom game, `main`, `WinMain` | weak (end: strong) |
| block_09 | `src/common/block_09.c` | `0x0040A9A0`–`0x0040AD4F` | 7 | map module | strong |

### block_01 — `0x00401000`–`0x004018AF`
`FUN_00401000` (fills the table `DAT_00414040`), `FUN_00401160` (object slots and templates), `read_joystick`, `read_keyboard`, `blit_to_screen` (25 callers; mode from `screen_mode`), `message_box`, `init_help_texts` (`0x004017C0`, the 10 default help texts, the game's first strings `0x0041221C`–`0x004124A8`). Shared with maped, which keeps `FUN_00401160` and `init_help_texts` (§6). The happy-only functions are under `#ifdef HAPPY`.

### block_02 — `0x004018B0`–`0x0040219F`
`timer_1s`, `timer_15ms` (callbacks of `install_int`), `save_screenshot` (`packfile_password("happy")`, `"happy.pcx"`), `init_hiscores` (`"Johan Peitz"`), `FUN_004019c0` (counts the levels of `DAT_00414c90`), `init_color_tables`, `start_scroll_message`, `draw_scroll_message`, `yes_no_box`, `load_data` (`data/misc.dat`, `data/tiles.dat`), `save_settings` / `load_settings` (`happy.sav`: volumes, controls, high scores, `screen_mode`), `default_settings`, `init_game` (`0x00401F00`), `_set_errno_erange` (`0x00402110`, library), `shutdown_game` (`0x00402120`).
- [verified] The timers, `load_data`, `load_settings` and `default_settings` have `init_game` as only caller; `save_settings` has `shutdown_game`.
- [verified] `_set_errno_erange` is the out-of-line copy of an `al386vc.h` inline, called by `init_game` and `play_level` (`fixdiv`/`fixmul`); the rebuild places it in block_02's object, at its original address (`build/happy/happy.map`).
- The start is weak: `init_help_texts` (block_01) also has `init_game` as only caller; the cut rests on the sharing with maped.

### block_03 — `0x004021A0`–`0x0040302F`
`draw_particles`, `draw_snow`, `draw_objects`, `draw_level` (`0x00402620`, 2,166 bytes, 9 callers), `init_level_palette` (`0x00402EA0`, the `.rdata` constant `0x004111E0`). [verified] `draw_snow` and `draw_objects` have `draw_level` as only caller.

### block_04 — `0x00403030`–`0x0040344F`
`FUN_00403030` (resets game variables; caller `play_game`), `load_level_objects` (objects and help texts that follow the tiles of a `.map`), `FUN_00403130` (puts carried objects back), `load_level` (`data/tiles.dat`/`sprites.dat` or the `x` variants, swap file `"hland.$$$"`, calls `map_load`, `map_load_datafile`), `reset_player`. [verified] `load_level_objects` and `FUN_00403130` have `load_level` as only caller.

### block_05 — `0x00403450`–`0x0040639F`
`spawn_particle`, `open_exit` (`"Level exit is now open!"`), `FUN_004036c0` (switches), `start_level` (`0x00403820`), `FUN_00403a40` (player animation), `take_item` (`"E X T R A   L I F E !"`), `FUN_00403ce0` (object follows the player), `FUN_00403da0` (player hurt), `FUN_00403e00` (pixel-perfect collision [hypothesis: PPCol]), `FUN_00403f60` (player/object contacts), `FUN_00404110` (all Happylanders saved), `update_objects`, `kill_player` (`"TIME OUT!"`), `FUN_00404770` (`"You earned a password!"`), `FUN_00404920` (`"Level complete!"`), `drop_item`, `pause_game` (`"GAME PAUSED"`), `play_level` (`0x00404C50`, 4,443 bytes, game loop), `bonus_room` (`0x00405DB0`).
- [verified] Single-caller chains: `FUN_00403ce0`/`FUN_00403da0`/`FUN_00403e00` → `FUN_00403f60`; `FUN_00404110` → `update_objects`; `drop_item`, `pause_game` → `play_level`.

### block_06 — `0x004063A0`–`0x004074FF`
`draw_title` (`"v1.3"`, credits; 4 menus), `set_controls`, `options_menu`, `clear_objects`, `show_logo` (`"Free Lunch Design"`), `wait_or_esc`, `intro_caption`, `play_intro` (`0x00406D60`). [verified] `set_controls` → `options_menu` and `intro_caption` → `play_intro` are single-caller pairs.

### block_07 — `0x00407500`–`0x0040986F`
`show_instructions`, `main_menu`, `main_menu_password` (adds `"PASSWORD"`), `enter_hiscore`, `enter_password` (table `passwords`, cheats `"HAXE-…"`), `show_hiscores`, `game_over`, `game_completed`, `ending_frames`, `anim_tick`, `play_ending` (`0x00409080`), `play_game` (`0x00409640`, game flow, `"Continue?"`). [verified] `ending_frames`, `anim_tick` → `play_ending` → `play_game`; `game_over`, `game_completed` → `play_game`.

### block_08 — `0x00409870`–`0x0040A99F`
`start_menu` (`data/levels.dat` / `data/xlevels.dat`, widescreen toggle), `draw_slot_panel` (only caller `custom_game`), `custom_game` (`0x00409F30`, 2,296 bytes: reads and writes `happy.clv`, `file_select`, `"./maps/"`), `main`, `WinMain`. [verified in the Allegro sources] `END_OF_MAIN()` defines `WinMain` right after `main`, in the same file: the end of the block is strong.

### block_09 — `0x0040A9A0`–`0x0040AD4F` (map module)
`map_clear`, `map_create` (`"unnamed.dat"`, `"unnamed"`), `map_destroy` (7 callers), `map_load` (`0x0040AB30`: 0x85-byte header, then the 5-byte tiles), `map_load_datafile` (into `map_datafile`), `map_collision_at`, `map_t4_at` (tile byte 2, resp. 4, at pixel `(x, y)`). Structure `MAP`/`TILE`: `src/include/map.h`. Shared with maped (§6).

## 6. Code shared with the other exes

- **Map module** (`src/common/block_09.c`): maped's first object (`0x00401000`–`0x004013FF`), happy's last game object. [verified] Five functions are byte-identical (addresses masked) and in the same order; maped also has `map_copy` and `map_save`, happy `map_collision_at` and `map_t4_at`. [hypothesis] One source in both exes, trimmed by `/OPT:REF`; the relative order of `map_save` and of the two happy-only functions is not observable.
- **Utilities** (`src/common/block_01.c`): `FUN_00401160` and `init_help_texts` are identical in maped (`0x004016A0`, `0x004017E0`, adjacent there). [hypothesis] Same shared-source mechanism; the happy-only functions are under `#ifdef HAPPY` because they reference globals maped lacks (`notes/matching.md` §2).
- **`END_OF_MAIN` stub**: identical in the three exes (maped `0x00405C70`, loader `0x004014A0`).
- Look-alike but different code (written once per exe): `notes/layout-maped.md` §6.
- `happy.clv` (the custom game, `custom_game`, `start_menu`) is produced by loader: format in `notes/layout-loader.md` §2.

## 7. Open questions

None affects a function score.

1. **One `.c` file or several per block** for `0x00401000`–`0x0040A99F`: not observable in the code. `src/` has one file per block and every function matches; globals all live in `src/happy/data.c` until their owner is known. [hypothesis] Clue: the Rich header counts 81 `Utc12_C` objects, CRT included; subtracting the C members of `LIBC.LIB` actually linked (`notes/data/crt-happy.csv`, column `member`) would give the number of game objects.
2. **`"OK"` (`0x004124AC`)** lies between the strings of `init_help_texts` and of `save_screenshot`, whereas only `custom_game` uses it. Loader's `""` was explained as a `.bss` COMDAT (`notes/layout-loader.md` §4), which does not apply to a non-empty string.
3. **OLDNAMES aliases and CRT member order**: the original has `AliasObj60 ×2`, the sources call three old names (`itoa`, `stricmp` in `enter_password`, `strcmpi` in `main`); which spelling is wrong is not observable. The CRT objects of the rebuild come in a different order from `fflush.obj` on, and `__aulldiv`/`__aullrem` (`0x0040E7E0`, `0x0040E850`) are linked although nothing calls them. [hypothesis] The original game objects referenced CRT symbols in code later dropped by `/OPT:REF` (`notes/matching.md` §3).
4. **Two unnamed CRT functions**: `0x0040B376` (`__fpclear` or `__rttosnpop`) and `0x0040DF22` (§3).
