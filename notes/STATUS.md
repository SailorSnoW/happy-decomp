# STATUS

## 2026-10-28 — Matching, step 18: bonus_room at 100%, play_level left

Note: `notes/block05-diffs.md`, `notes/matching.md` §3a, §5. Branch `decomp/happy`.

### Done
- [verified] `bonus_room` (`0x00405DB0`) 99.78% -> 100%: the first walk and the fall share one variable (`pos`); VC6 then reloads it before `x` after the inner loop of the fall, as the original does.
- [verified] **happy: 83/84 GAME functions at 100% (88.8% of the bytes)**. maped 47/50, loader 2/2 unchanged; `reccmp-datacmp` 0 issues.
- `play_level` (`0x00404C50`): about sixty more forms tried, in the whole function and in a reduced test of the movement block (scratch); none gives the original `add eax, ecx` (`block05-diffs.md`).

### Remaining
- happy: `play_level` 99.84%, one instruction (`0x004050DA`, operands of an `add` swapped, "effective match" for reccmp).
- maped: `set_screen_mode` 85.94%, `auto_mask` 80.54%, `pick_tile` 97.83%.

### Open questions
- `play_level`: which C shape makes VC6 keep `player.f0c` in `ecx` at `0x004050D4` and add into the `fixmul` result? Only a register variable does it in the tests, which the original's stores rule out.

## 2026-10-27 — Matching, step 17: end-of-array pointers compared by offset

Note: `notes/matching.md` §3, §5; `scripts/reccmp_run.py`. Branch `decomp/happy`.

### Done
- Decided with the user: the end-of-array operands are fixed on the comparison side, not in the sources (no layout trick). `scripts/reccmp_run.py` runs reccmp-reccmp 0.1.7 with a run-time patch: two operands of lines that the diff aligns one to one are equal when their addresses are at the same offset of one matched variable, at most 64 bytes outside it. `progress.py` runs it with the Python where reccmp is installed (shebang or `python.exe` next to `Scripts/`, checked by `import reccmp`).
- [verified] **happy: 82/84 at 100% (85.0% of the bytes)**, up from 68: the 14 end-of-array functions and `enter_hiscore`. maped: 47/50 (its five end-of-array functions). loader 2/2. `reccmp-datacmp` 72 / 37 / 1 variables, 0 issues.
- [verified] Checks: `--audit` on HAPPY and MAPED lists 50 + 35 lines; in the game code only end-of-array `cmp` operands and `hiscores - 1`; in the CRT, unannotated CRT variables next to `errno` and `wind` at the same offset in both exes (correct, not counted by `progress.py`). Negative test: `clear_objects` with `i < 99` stays at 83.33% (`DAT_00414140[99].f1a` against `scroll_text[10]`).
- CLAUDE.md: the per-function command is now `python scripts/reccmp_run.py --target HAPPY --verbose 0x...`.

### Remaining
- happy: `play_level` 99.84% and `bonus_room` 99.78%, both "effective match" for reccmp (register / operand order only, `notes/block05-diffs.md`).
- maped: `set_screen_mode` 85.94%, `auto_mask` 80.54%, `pick_tile` 97.83%.

## 2026-10-27 — Matching, step 16: happy, main and draw_level at 100%

Note: `notes/matching.md` §3, §3a, §4, §5; `notes/block03-diffs.md`, `notes/block08-diffs.md`, `notes/block05-diffs.md`. Branch `decomp/happy`.

### Done
- [verified] **happy: 68/84 GAME functions at 100% (73.1% of the bytes)**, up from 66 (66.8%).
- [verified] `main` (`0x0040A830`) 83.81% -> 100%: the start menu loop is a `while (!done)` on a flag, as in `custom_game`; VC6 then does not rotate it (`block08-diffs.md`).
- [verified] `draw_level` (`0x00402620`) 90.32% -> 100%: the tile coordinates are `i * 16 - sx` / `j * 16 - sy` in the calls (no `x` / `y` variables: VC6's induction variables give the original stack slots and loop tests), the front tile index in `k`, and `len = text_length(...)` before `textout` (grouped stack cleanups) (`block03-diffs.md`).
- [verified] `itoa` is an OLDNAMES alias like `chdir`: `crt_names.py` maps `__itoa` to `_itoa` (`COMPILER_ALIASES`), `src/crt/happy.h` and `notes/data/crt-happy.csv` regenerated; that was the last difference of `draw_level`.
- Tried without success (`block05-diffs.md`): the swapped reloads of `bonus_room` (every order of the statements after the inner loop, variable splits and reuses, loop forms) and the swapped `add` of `play_level` (a dozen forms of the `MIN`).
- The end-of-array question looked at again (no change to the decision): the neighbours of the arrays concerned are uninitialised globals (COMMON), placed by the linker in the reverse order of the compiler's symbol table, which depends on the names [verified with test programs: same declarations, other names, other order]; with the original names unknown, the layout cannot be reproduced. Side finding [verified]: `0x00413B10`-`0x00413B34` (`DAT_00413b10`, `use_joystick`, `DAT_00413b20`-`DAT_00413b34`) are in the `.bss` of game objects, before the CRT's `errno`, which VC6 does for globals initialised to 0 or `static` (test program: `int x = 0;` goes to `.bss`), not COMMON; no function depends on it.
- No regression: maped 42/50, loader 2/2; `reccmp-datacmp` 72 / 37 / 1 variables, 0 issues. `build.py --final happy`: the game code ends at `0x0040AD50` (`rand`), as in the original.
- No Ghidra sync needed: names and prototypes unchanged.

### Remaining
- happy below 100%: the 14 end-of-array pointers and `enter_hiscore` (decided with the user, `matching.md` §3), `bonus_room` (end-of-array pointer + swapped reloads), `play_level` 99.84% ("effective match").
- maped below 100%: `set_screen_mode`, `auto_mask`, `pick_tile` (+ five end-of-array pointers); the shapes found here (flag loops, coordinates in the expressions, locals for nested calls) are worth trying there.

### Open questions
- happy's OLDNAMES aliases: the original has two (`AliasObj60 ×2`), the sources call three (`itoa`, `stricmp`, `strcmpi`); which spelling is wrong is not observable in the code, and the CRT layout differs for another reason (`matching.md` §3).

## 2026-10-27 — Matching, step 15: loader (2/2 at 100%)

Note: `notes/matching.md` §3, §3a, §4, §5; `notes/layout-loader.md` §10. Branch `decomp/happy`.

### Done
- [verified] **loader: 2/2 GAME functions at 100%**: `main` (`0x00401000`, 1,172 bytes) and `WinMain` (`0x004014A0`), in `src/loader/block_01.c`; `src/loader/data.c` only annotates `errno` (the game has no globals). `reccmp-datacmp` 1 variable, 0 issues.
- [verified] **Whole-exe check**: the objects linked again without `/DEBUG` give `.text`, `.rdata` and `.data` byte-identical to the original loader.exe; only `.rsrc` (the icon) is missing (`matching.md` §3).
- [verified] What loader does: the list of maps is `argv[1]` or a `.txt` picked with `file_select_ex` under `maps/` (640x480 window); lines skipped up to a first `#`, regular maps (at most 18) up to the next `#`, bonus maps (at most 8) up to a third `#`; each name is made relative to the folder of the list and must exist. A slot of `happy.clv` = 255 bytes of file name + 80 bytes of the list line (`layout-loader.md` §10, question 1 answered). Return codes 2-5 for the errors; the list is not closed on those paths.
- [verified] `chdir`, not `_chdir`: the OLDNAMES alias is resolved in a later link pass, which puts `__chdir` and its callees at the end of the CRT as in the originals. loader: 68/68 CRT functions at their original address (0 with `_chdir`); maped: 162/163 (17 before; `__ld12tod` is the one left). maped `main` switched to `chdir`, still 100%. This identifies the single OLDNAMES alias of maped and loader (`toolchain-vc6.md` §5).
- [verified] `""` (`0x00406708`, `layout-loader.md` §10 question 2): cl 12 emits it as a 1-byte COMDAT in `.bss`, placed first in `.bss` before `errno`; not a merge with a CRT string.
- `scripts/crt_names.py`: `COMPILER_ALIASES` names `chkstk.obj` `__chkstk` (the name cl calls; the CRT relocations had chosen `__alloca_probe`) and `chdir.obj` `_chdir` (the alias the game calls). `src/crt/maped.h` and `loader.h` regenerated.
- Matching finding (`matching.md` §3a): `num_bonus = num_normal = 0;` decides a register reload order that two assignments, in either order, do not give (99.44% -> 100%).
- No regression: happy 84/84, 66 at 100%; maped 50/50, 42 at 100%; `reccmp-datacmp` 72 / 37 / 1 variables, 0 issues.
- Ghidra: `WinMain` and `errno` renamed in loader.exe, saved. `main` has no function in Ghidra (skipped, creation not allowed).

### Remaining
- **All three exes are annotated.** Below 100%: happy 18 functions (14 end-of-array pointers, `draw_level`, `main`, `play_level`, `bonus_room`, `enter_hiscore`), maped 8 (`set_screen_mode`, `auto_mask`, `pick_tile`, five end-of-array pointers).
- Next phase per CLAUDE.md: the C17 port, verified against the matching build; or a pass on the functions below 100%.

### Open questions
- happy's two OLDNAMES aliases (`strcmpi` is one [hypothesis]): the same CRT-layout check as for `chdir` could identify them once happy's game code has its original size.
- maped `__ld12tod` (`0x0040A81F` in the original, `0x0040A809` rebuilt): the only CRT function of maped not at its original address.
- `layout-loader.md` §10 question 3: create the functions Ghidra misses (`main` of loader)? Not allowed without approval.

## 2026-10-27 — Matching, step 14: maped (50/50 annotated)

Note: `notes/matching.md` §3, §3a, §5; `notes/maped-diffs.md`. Branch `decomp/happy`.

### Done
- [verified] **maped: 50/50 GAME functions annotated, 42 at 100% (65.6% of the bytes)**, in `src/maped/block_02.c` and `block_04.c`-`block_07.c` (+ `maped.h`, `data.c`), reusing `src/common/`. Below 100%: five end-of-array pointers only (§3; `editor_loop` 99.73%, 3,792 bytes), `set_screen_mode` 85.94%, `auto_mask` 80.54%, `pick_tile` 97.83% (register allocation, `maped-diffs.md`).
- [verified] What maped does: background gradient and its six sliders (`colors_dialog`), screenshot `maped%03d.pcx`, 320x200 / 640x480, the editor frame (`draw_editor`: tile layers, selection, status lines), export of the whole map as an image (`show_*` options of `maped.ini`), load / revert / new / save / save as with the objects and help texts appended after the tiles, map properties dialog (name, time, happies, music, tile set, scroll/snow/wind, background), six editing modes (BASE, COVER, WALKMASK, ITEM, NPC, TILETYPE), RANDOM tile variants, automatic walk masks from tile numbers (ALT+Y, `auto_mask`), slope check (F3), shift / grow / shrink with M / INSERT / DEL and the arrows, start tile (F5). Keys in the comment of `editor_loop`.
- Data: `file_menu` (MENU[32]), `properties_dialog` (33), `text_dialog` (6), `colors_dialog` (10), `slider_values`, the edit buffers; `DAT_004107a0` is an `int[6]` of choices per mode. The `d_*_proc` import thunks are annotated (`#if 0` in `data.c`) so that datacmp names the `proc` fields; `reccmp/maped.csv` declares the 0.0625 constant. `reccmp-datacmp`: maped 37, happy 72 variables, 0 issues.
- Matching findings (`matching.md` §3, §3a): a `// STRING:` marker before a line without the literal drops the whole function (use `reccmp-decomplint`); separate `if`s versus `else if` (merged tails); separate locals for derived values (`editor_loop` 45% -> 99.7%); parameters reused as temporaries (`resize_map`); `i + dx` in the expression; `object_message` is inline; `chdir` is an OLDNAMES alias of `_chdir`.
- Scratch tools (not versioned): a disassembler of the originals with names from `src/`, and an object comparator (compile one file, compare one function instruction by instruction, jump targets as indices) with a statement-order permuter; they found every shape above.
- Ghidra: maped functions renamed with their prototypes, globals typed and named, saved (`ghidra_sync.py --save maped`). Left: the functions Ghidra does not have (`slider_changed`, `main`, the 7 thunks).
- No regression: happy 84/84, 66 at 100%. The final link of maped (`build.py --final maped`) succeeds: 69,689 bytes (original 73,728).
- `layout-maped.md` §10 question 3: `0x00403140` is called on F1 and empty.

### Remaining
- Next: loader (`src/loader/`, 70 functions in the inventory, `notes/layout-loader.md`).
- maped below 100%: `set_screen_mode`, `auto_mask`, `pick_tile` (shapes tried: `maped-diffs.md`).

### Decisions
- **Per-exe macro** (decided with the user): `build.py` compiles each exe with `/DHAPPY`, `/DMAPED` or `/DLOADER`; the happy-only functions of `common/block_01.c` and their globals' declarations are inside `#ifdef HAPPY` (evidence: `matching.md` §2). No stub left in maped; `build.py --final happy maped` links both (81,977 and 69,689 bytes); same scores, `reccmp-datacmp` 0 issues.

## 2026-10-27 — Matching, step 13: game loop (happy fully decompiled)

Note: `notes/matching.md` §3a, §5; `notes/block05-diffs.md`. Branch `decomp/happy`.

### Done
- [verified] `play_level` (`0x00404C50`, 4,443 bytes, the game loop): 99.84%, "100% effective match" for reccmp; one `add` has its operands swapped (`block05-diffs.md`). `play_game` now at 100%; `bonus_room` 99.56% (end-of-array pointer and the swapped loads already noted).
- [verified] **happy: 84/84 GAME functions annotated, 66 at 100% (66.8% of the bytes), no stub left.** The final link (`build.py --final`, `/OPT:REF`) succeeds: 81,977 bytes (original 86,016; not compared further).
- [verified] What the loop does: returns 1 (exit tile, `map_t4_at` == 1), 2 (energy `player.f44` at 0, `kill_player`) or 3 (Escape and "Quit?"); "HURRY UP!" at 30 s; the player's physics (`player.f0c`/`f10` speeds, top speed `f18` = 2.0, ice and slides, slopes, walls, jump -5.0 cut by half when released, gravity with or without the low-G shoes, at most 7.0 per frame); the trail of positions; items, snow switches, deadly tiles, Pause, F12 (`happy.pcx`). Types: `PLAYER.f4f` is `signed char`.
- Matching findings (`matching.md` §3a): initialisers, argument order of `MIN`/`MAX`, `abs()` vs `ABS`, `x <= -1`, Allegro's `TRUE` = -1.
- No regression: maped 9/50, 9 at 100%; `reccmp-datacmp` 72 + 4 variables, 0 issues.
- Ghidra: `play_level` renamed with its prototype, saved.

### Remaining
- happy below 100%: end-of-array pointers (§3, 14 functions), `draw_level` 90.32% (`block03-diffs.md`), `main` 83.81% (`block08-diffs.md`), `play_level` 99.84%, `bonus_room` 99.56%, `enter_hiscore` 99.55%.
- Next: maped (reusing `src/common/`), then loader.

## 2026-10-27 — Matching, step 12: level frame (happy block_03 complete)

Note: `notes/matching.md` §3, §3a, §5; `notes/block03-diffs.md`. Branch `decomp/happy`.

### Done
- [verified] `draw_level` (`0x00402620`, 2,166 bytes): 90.32%. Same control flow, calls and arguments (693 instructions on both sides); the stack slots and the test of the outer tile loops differ (`block03-diffs.md`). Its callers that only differed by the call are now at 100%: `show_instructions`, `main_menu`, `main_menu_password`, `ending_frames`, `play_ending`, `kill_player`, `FUN_00404920`.
- [verified] What it draws: camera on the player when `MAP.f79` bit 0 is set, background `DAT_004274e4` scrolled by `(ox % 1280) >> 2`, flakes (`f79` bit 1), back tiles (`TILE.t0`: RLE sprite up to 130, a diagonal line from 131 on, apparently an editor marker), items (`t3`), particles, objects, the player (sprites 64/70 + `player.f2c`), front tiles (`t1`), then the HUD (score, lives, spheres, bonus items, time, Happylanders left) and the scrolling message.
- Matching findings (`matching.md` §3a): an inner loop controlled by `j` with `y = j * 16` (VC6 tests `y` and keeps `j`); the `ebp` frame comes from the inline asm of `fixtoi`.
- Scratch tool (not versioned): a comparator that compiles a variant to .obj and compares it with the original, instruction by instruction, relocations masked, and runs 8 variants in parallel. It found the loop shape above.
- No regression: happy 83/84 annotated, 65 at 100% (65.4% of the bytes); maped 9/50, 9 at 100%; `reccmp-datacmp` 72 + 4 variables, 0 issues.
- Ghidra: `draw_level` renamed with its prototype, saved.
- Fixed: the two previous STATUS entries had been written in cp1252 by a script (dashes and `§`); converted back to UTF-8. The header comment of `block_08.c` is updated (an edit of step 11 had missed it for the same reason).

### Remaining
- Next: `0x00404C50` (game loop, 4,443 bytes), the last function of happy; then maped.

## 2026-10-27 — Matching, step 11: start menu and custom game (happy block_08 complete)

Note: `notes/matching.md` §3a, §5; `notes/block08-diffs.md`. Branch `decomp/happy`.

### Done
- [verified] `start_menu` (`0x00409870`, 1,476 bytes) and `custom_game` (`0x00409F30`, 2,296 bytes): 100%. block_08 complete: 4 of 5 at 100%, `main` 83.81% (loop rotation only, retried with both callees at 100%).
- [verified] `start_menu`: the mouse band sets `DAT_00413b20` (0 "HAPPYLAND ADVENTURES" -> `data/xlevels.dat`, 1 "THE ORIGINAL GAME" -> `data/levels.dat`, 2 "CREATE CUSTOM GAME", 3 "EXIT" -> -1); W toggles `screen_mode` (320x200 / 640x480). `custom_game`: up to 18 level maps (`DAT_004404e0`, count `DAT_004450a8`) and 8 bonus maps (`DAT_0043f5c0`, count `DAT_0042f568`) picked with `file_select` under `./maps/`, saved in `happy.clv`; the counts are read and written as 2 bytes each, one byte more than the globals.
- Matching findings (`matching.md` §3a): two `?:` on `screen_mode` instead of an `if`/`else` (found by trying the shapes in a loop: declaration orders, variable reuse and loop variables changed nothing); `while (!done)` instead of `do`/`while`; nested "Play!" tests.
- No regression: happy 82/84 annotated, 58 at 100% (49.4% of the bytes); maped 9/50, 9 at 100%; `reccmp-datacmp` 72 + 4 variables, 0 issues.
- Ghidra: `start_menu`, `custom_game` renamed with their prototypes, saved.

### Remaining
- Next: `0x00402620` (level frame, 2,166 bytes), then `0x00404C50` (game loop, 4,443 bytes); then maped.

## 2026-10-27 — Matching, step 10: main and the custom game panel (happy block_08, without the two large functions)

Note: `notes/matching.md` §3, §3a, §5; `notes/block08-diffs.md`. Branch `decomp/happy`.

### Done
- [verified] `src/happy/block_08.c`: 3 of 5. `draw_slot_panel` (`0x00409E40`, a box with light and dark edges and "RIGHT CLICK A SLOT" / "TO REMOVE IT", drawn twice by `0x00409F30`) 100%; `WinMain` (`0x0040A970`) 91.67% -> 100% now that `main` is annotated; `main` (`0x0040A830`) 81.90%: the calls to `0x00409870` / `0x00409F30`, and a loop rotation that no C shape tried avoids (`block08-diffs.md`).
- [verified] `main`: options `-widescreen` (`screen_mode` = 1) and `-fps` (`DAT_00413b10` = 1), compared with `strcmpi`. Neither has an effect: `init_game`, called next, always rewrites `screen_mode` (`load_settings` or `default_settings`), and `0x00413B10` has no other reference in the exe (byte scan). `0x00409870` returns -1 (EXIT, `DAT_00413b20` = 3) or 0 after loading `data/levels.dat` / `data/xlevels.dat`; `DAT_00413b20` = 2 runs the custom game screen `0x00409F30` (0 = back to the start menu). `DAT_004450b0` = first level (1, or the password level + 1).
- Matching findings (`matching.md` §3a): an expression repeated in two calls rather than a local decides the parameter registers (`draw_slot_panel`); loop rotation of `/O2`. `main` shows as `_mangled_main` (`allegro.h`).
- No regression: happy 80/84 annotated, 56 at 100%; maped 9/50, 9 at 100%; `reccmp-datacmp` 72 + 4 variables, 0 issues.
- Ghidra: `draw_slot_panel` renamed with its prototype, `DAT_00413b10` typed, saved. `main` has no function in Ghidra (skipped); `passwords` and `player` still refused (`type_would_evict`).

### Remaining
- Next: the four large functions (`0x00404C50`, `0x00409F30`, `0x00402620`, `0x00409870`), then maped. Retry the loop of `main` once `0x00409870` and `0x00409F30` are done (done in step 11: no change).

### Open questions
- `main`: which C shape keeps VC6 from rotating the start menu loop (`block08-diffs.md`)?
- `-widescreen` and `-fps` are dead options in this build [verified in the code]: leftovers of an earlier version? [hypothesis]

## 2026-10-27 — Matching, step 9: menus, scores, ending and game flow (happy block_07)

Note: `notes/matching.md` §3, §3a, §5. Branch `decomp/happy`.

### Done
- [verified] `src/happy/block_07.c`: 12/12. At 100%: `enter_password` (`0x00408500`), `show_hiscores` (`0x00408930`), `game_over` (`0x00408A40`), `game_completed` (`0x00408B90`), `anim_tick`. Only the calls to `0x00402620` / `0x00404C50`, not decompiled yet, differ in `show_instructions` (`0x00407500`), `main_menu` (`0x00407B30`), `main_menu_password` (`0x00407E80`), `ending_frames` (`0x00408E00`), `play_ending` (`0x00409080`) and `play_game` (`0x00409640`). `enter_hiscore` (`0x00408200`) 99.55%: the address `hiscores[0].name - 1`, which reccmp cannot name (`matching.md` §3).
- [verified] Cheats (`enter_password`): `passwords[16]`-`[20]` are "HAXE-SPLITSECONDS" (clock stopped, `DAT_00412088` = 0), "HAXE-CANTTOUCHME" (`DAT_00413b30`), "HAXE-OPENSESAME" (`DAT_00413b34`), "HAXE-GOHOMEBADDIE" (`DAT_0041208c` = 0), "HAXE-FULLCAPACITY" (`DAT_00412090` = 10); "HAXE-RESET" restores them. Comments of `data.c` updated (one comment was above the wrong global).
- [verified] `hiscores[7]`: six entries are shown, the seventh only receives the entry shifted down when a score is inserted (`game_over`, `game_completed`, `play_ending`). `game_over` draws sprite 65 upside down (`draw_sprite_v_flip`).
- `bonus_room` now returns `int` (the result of `0x00404C50`, read by `play_game`); `FUN_00404c50` declared `int`. Same code, still 99.34%.
- Matching findings (`matching.md` §3a): `MID(0, 18, i)` argument order, a reused loop variable in `play_game`, the position of `old = sel` in the menus, vtable offsets of Allegro's inline sprite functions.
- No regression: happy 78/84 annotated, 54 at 100%; maped 9/50, 9 at 100%; `reccmp-datacmp` 71 variables, 0 issues.
- Ghidra: 11 functions renamed with their prototypes, `bonus_room` retyped, saved (`ghidra_sync.py --save`). `passwords` and `player` still refused (`type_would_evict`, step 7).

### Remaining
- Next: happy block_08 (`0x00409870`–`0x0040A99F`: `0x00409E40`, then `main` `0x0040A830`; `0x00409870` and `0x00409F30` are among the large functions left for the end), then the four large functions (`0x00404C50`, `0x00409F30`, `0x00402620`, `0x00409870`).

### Open questions
- `show_instructions` waits for `timer_ticks` without resetting it (no store to `0x00442084` in `0x00407500`): after the first tick the demonstration runs unthrottled [verified in the code, effect not observed]. Bug of the original?
- `DAT_00413b2c` is set to 1 at the end of `play_ending`; its readers are not decompiled yet (block_08?).

## 2026-10-27 — Matching, step 8: menus and introduction (happy block_06)

Note: `notes/matching.md` §3, §5. Branch `decomp/happy`.

### Done
- [verified] `src/happy/block_06.c`: 8/8. At 100%: `draw_title` (`0x004063A0`, title, credits and "v1.3" on the menu background `DAT_004274e4`), `set_controls` (`0x00406560`), `options_menu` (`0x00406850`, volumes, "set controls", "main menu"), `show_logo` (`0x00406BA0`, "Free Lunch Design presents"), `wait_or_esc` (`0x00406C80`), `intro_caption` (`0x00406CD0`), `play_intro` (`0x00406D60`, 1,947 bytes). `clear_objects` (`0x00406B80`, clears `DAT_00414140[i].f1a`) 83.33%: end-of-array pointer only (`matching.md` §3).
- [verified] `controls`: `set_controls` names the slots (0 right, 1 left, 4 jump, 5 talk, 6 slide) and refuses a key equal to `controls[2]` or `controls[3]`, which it never sets (`0x00406777`). Joystick directions and buttons 1-4 map to the codes 116-124, as in `read_joystick`.
- Matching findings (`matching.md` §3): a string array initialiser needs one element per line, each after its `// STRING:` marker; `DATAFILE` entries are 16 bytes.
- No regression: happy 67/84 annotated, 50 at 100%; `reccmp-datacmp` 71 + 4 variables, 0 issues.
- Ghidra: 7 functions renamed with their prototypes, saved (`ghidra_sync.py --save`). `passwords` and `player` still refused (`type_would_evict`, see step 7).

### Remaining
- Next: happy block_07 (`0x00407500`–`0x0040986F`, 12 functions, `anim_tick` already done), leaves first.

### Open questions
- `controls[2]` and `controls[3]`: neither `default_settings` nor `set_controls` sets them; `set_controls` still compares with them. Set elsewhere (block_07/08?) or unused.
- `DAT_00413b20` (0, 1 or 2) chooses the title drawn by `draw_title` (logo sprites 35 + 36, "* HAPPYLAND ADVENTURES *", or sprite 35 only) and the level list of `load_level`: [hypothesis] a game variant or level set.

## 2026-10-27 — Matching, step 7: gameplay (happy block_05, without 0x00404C50)

Note: `notes/matching.md` §3, §3a, §5; `notes/block05-diffs.md`. Branch `decomp/happy`.

### Done
- [verified] `src/happy/block_05.c`: 18 of the 19 functions. 11 at 100% (`spawn_particle`, `open_exit`, `FUN_004036c0` switches, `FUN_00403a40`, `take_item` (after the `PLAYER` decision below), `FUN_00403ce0`, `FUN_00403da0`, `FUN_00403e00` pixel collision, `pause_game`, `FUN_00404770` password screen, `drop_item`); 6 that only differ by end-of-array pointers or calls to undecompiled functions (`start_level`, `FUN_00403f60`, `FUN_00404110`, `update_objects`, `kill_player`, `FUN_00404920`); `bonus_room` 99.34% (`block05-diffs.md`).
- [verified] `FUN_00403e00` is the 8-bit case of `check_pp_collision` (PPCol library, Ivan Baldo) [hypothesis on the origin: same structure, and only that structure matches].
- [verified] Passwords: table of 21 strings at `0x00412034`, each character + 1 (`passwords` in `data.c`, with `// STRING:` annotations); the current level `DAT_00440038` is 1-based and signed.
- Types: `OBJ.f08`/`f0c` are two `int` (index into the 512 saved player positions and delay, for the objects that follow the player); `DAT_00440038` and `DAT_00440048` are `char` (movsx); comments on `MAP.f78` (music), `f7a` (time limit), `f7b`/`f7c` (Happylanders to save). New globals: `passwords`, `DAT_0041208c` (= 1), `DAT_00412090` (= 5), `DAT_00413b30`, `DAT_00413b34`, `DAT_0044004c`.
- Matching findings: `matching.md` §3a (MIN assigned to a global, `!x & 1`, merged `if`/`else` calls, stack slots, late initialisation of a register local, zero-extended byte loads).
- No regression: happy 60/84 annotated, 43 at 100%; `reccmp-datacmp` 95 + 4 variables, 0 issues.

### Remaining
- `0x00404C50` (game loop, 4,443 bytes): left for the end with the other large functions (CLAUDE.md).
- Ghidra synchronised for block_05 (after a restart of Ghidra): `OBJ` re-imported in both programs (28 bytes, `f08`/`f0c` as `int`), 36 changes applied and saved, 0 differences left except the type of `passwords` (`0x00412034`): renamed, but `char *[21]` is refused (`type_would_evict`, Ghidra already has 21 separate pointers there; clearing them is not among the allowed writes). `ghidra_sync.py` now also reads globals whose initialiser starts with `= {` at the end of the line.
- Next: happy block_06 (`0x004063A0`–`0x004074FF`).

### Decisions
- **`PLAYER` structure** (decided with the user): the 25 globals at `0x0042F500`-`0x0042F567` are one structure, `PLAYER player` (`happy.h`, 0x68 bytes, fields `x`, `y`, `fXX` = offset). Evidence: the original loads `f50` and `f64` with `mov dword` / `and 0xff`, which VC6 only emits for a known alignment (`matching.md` §3a); `0x0042F568` is loaded the other way and stays a separate global. Result: `take_item` 85.55% -> 100%, no other score changed; happy 44 at 100%, `reccmp-datacmp` 71 + 4 variables, 0 issues.
- Ghidra: structure `PLAYER` imported, `0x0042F500` renamed `player`, but the type is refused (`type_would_evict`, the 25 old data items are still defined there), like `char *[21]` on `passwords`. To apply by hand in the Ghidra GUI if wanted (Retype at `0x0042F500` -> `PLAYER`, `0x00412034` -> `char *[21]`), or to allow clearing them.

## 2026-10-27 — Ghidra synchronised with src/ (blocks 01-04 and 09)

Note: `notes/matching.md` §3b.

### Done
- [verified] Cause of the blockage: the auto-analysis queued by the writes; `save_program` waited behind it and blocked the HTTP server. Ghidra restarted by the user after a manual save; the batch of step 3 was saved.
- `scripts/ghidra_sync.py`: names, prototypes and global types from the annotations of `src/`, dry run by default, saves only after the analysis. Applied to happy (140 changes: blocks 02-04, the corrected names `light_table`, `trans_table`, `misc_datafile`, `controls`); maped already in sync. Re-run: 0 differences for both. Both programs saved.
- Structures imported or updated in Ghidra: `OBJ` (x, y, f10), `MAP` (f03, f04), `HISCORE`, `PARTICLE`, `T_00416d00`, `T_0043f5c0`. `OBJ` of maped was broken by `modify_struct_field` (28 -> 16 bytes) and repaired by re-importing it.

### Remaining
- Next: happy block_05 (19 functions, gameplay), leaves first, `0x00404C50` at the end.
- `timer_1s` (`0x004018B0`) and `timer_15ms` (`0x004018F0`) are not functions in Ghidra (creating them requires approval).

### Decisions
- End-of-array pointers (step 5): left below 100% with the note of `matching.md` §3, no trick on the layout of the globals (decided with the user).

## 2026-10-27 — Matching, step 6: loading and resetting a level (happy block_04)

Note: `notes/matching.md` §3, §3a, §5. Branch `decomp/happy`.

### Done
- [verified] `src/happy/block_04.c`: 5/5. `FUN_00403030` 100%; `load_level_objects` (`0x00403070`, 98.15%), `FUN_00403130` (95.45%), `load_level` (`0x00403170`, 99.28%), `reset_player` (`0x00403350`, 98.39%): end-of-array address only.
- [verified] Bug of the original in `reset_player`: the loop writes both coordinates into `DAT_00414cc0` and never writes `DAT_004154c0` (bytes `89 01` then `89 41 fc` at `0x0040340C`/`0x0040342F`, same slot).
- [verified] `load_level`: the swap file `hland.$$$` (entry n of `DAT_00414c90` written out then read with `map_load`), sprites/tiles or xsprites/xtiles depending on bit 3 of `MAP.f79`.
- Types: `T_00416d00` replaces `T_00416d10` (the per-object array starts at `0x00416D00`: saved `x`/`y` at +0/+4, vertical flip at +0x10); `T_0043f5c0` (335 bytes, `.map` file names at `0x0043F5C0` ×8 and `0x004404E0` ×18); `MAP.f03`/`f04` = start tile [hypothesis]. 32 new globals.

### Remaining
- Ghidra still blocked (step 4).
- Next: happy block_05 (19 functions, gameplay), leaves first, `0x00404C50` at the end.

### Open questions
- End-of-array pointers: now 8 functions (step 5).

## 2026-10-27 — Matching, step 5: rendering and palette (happy block_03, without 0x00402620)

Note: `notes/matching.md` §3, §5. Branch `decomp/happy`.

### Done
- [verified] `src/happy/block_03.c`: `draw_particles` (`0x004021A0`, 98.53%), `draw_snow` (`0x00402260`, 100%), `draw_objects` (`0x004024E0`, 99.16%), `init_level_palette` (`0x00402EA0`, 100%). The two below 100% only differ by an end-of-array address (`matching.md` §3).
- `0x00402620` (2,166 bytes, the level frame) is left for the end, with the other large functions (CLAUDE.md).
- Types: `PARTICLE` (24 bytes: particles at `0x004450C0`, 256, and flakes at `0x004420A0`, 512 [hypothesis: same type]), `T_00416d10` (20 bytes, one per object); `OBJ` gets `x`, `y` (+0, +4) and `f10`. New globals `wind` (`0x00413D3C`), `snow`, `particles`, `DAT_00416d10`.
- [verified] MAP fields: `f77` = background image (index into misc_datafile, 0-2), `f79` bit 2 = changing wind, `f7d`-`f7f` and `f81`-`f83` = the two ends (RGB) of the 16-colour gradient in palette entries 176-191.
- `reccmp/happy.csv`: first reccmp data source (the float constant `0x004111E0`), declared in `reccmp-project.yml`.

### Remaining
- Ghidra still blocked (see step 4).
- Next: happy block_04 (`0x00403030`–`0x0040344F`).

### Open questions
- **End-of-array pointers** (4 functions at 98-99%): would be fixed if the recompiled globals had the original neighbours. Defining the arrays in `data.c` with an initialiser (`= {0}`) would put them in `.data` in source order, but that moves them out of `.bss` and is a trick to force a match. To decide with the user; left as is meanwhile.

## 2026-10-27 — Matching, step 4: initialisation and shutdown (happy block_02)

Note: `notes/matching.md` §3, §3a, §5. Branch `decomp/happy`.

### Done
- [verified] `src/happy/block_02.c`: 16/16 at 100%. `timer_1s` (`0x004018B0`), `timer_15ms` (`0x004018F0`), `save_screenshot` (`0x00401900`), `init_hiscores` (`0x00401970`), `FUN_004019c0`, `init_color_tables` (`0x00401A30`), `start_scroll_message` (`0x00401AA0`), `draw_scroll_message` (`0x00401B20`), `yes_no_box` (`0x00401BE0`), `load_data` (`0x00401D60`), `save_settings` / `load_settings` / `default_settings` (`0x00401DA0`–`0x00401EA0`, `happy.sav`), `init_game` (`0x00401F00`), `shutdown_game` (`0x00402120`), plus the library function `_set_errno_erange` (`0x00402110`).
- Corrected names of step 3 [verified]: `0x004174E0` is the **light** table (`create_light_table`), renamed `light_table`; the translucency table is `0x0042F5A0` (`trans_table`). `DAT_00414c9c` is `misc_datafile` (`data/misc.dat`). `DAT_0042f594` is `controls[5]` (`controls[7]` at `0x0042F580`, the key bindings saved in `happy.sav`).
- New globals: 35 more in `src/happy/data.c` (`hiscores`, `controls`, `fps`/`frame_count`, the scrolling message, colour tables, fixed-point constants of `init_game`...), and `errno` of LIBC (`0x00413B3C`) annotated by name.
- Matching findings: `matching.md` §3a (unsigned score field, returned value kept in `eax`, `volatile` timer variables, one literal per line).
- Ghidra disassembly replaced by a scratch capstone script reading `game/happy.exe` directly (Ghidra blocked).

### Remaining
- **Ghidra blocked** since the `save_program` of step 3: every request times out. To check in the Ghidra GUI (dialog?), then save, then carry over the names of step 3 (corrections above: `light_table`, `trans_table`, `misc_datafile`, `controls`) and of block_02.
- Next: happy block_03 (`0x004021A0`–`0x0040302F`), leaves first (`0x00402260`, `0x004024E0` before `0x00402620`).

### Open questions
- `controls[2]` and `controls[3]` are not set by `default_settings` [verified]: set elsewhere, or unused?
- `DAT_004274e4`: 640x200 bitmap created by `init_game`, use unknown.

## 2026-10-27 — Matching, step 3: shared utilities (happy block_01, maped block_03)

Note: `notes/matching.md` §3, §3b, §5. Branch `decomp/happy`.

### Done
- [verified] `src/common/block_01.c`: the 7 functions of happy block_01, of which maped keeps 2. happy: `read_joystick` (`0x004012A0`), `read_keyboard` (`0x00401350`), `blit_to_screen` (`0x00401380`), `message_box` (`0x004014F0`), `init_help_texts` (`0x004017C0`) at 100%; `FUN_00401000` 98.18%, `FUN_00401160` 98.75%. maped: `FUN_00401160` (`0x004016A0`) and `init_help_texts` (`0x004017E0`) at 100%.
- [verified] The two functions below 100% have code identical to the original: the only difference is an end-of-array address, named after a different neighbour in the recompiled exe (`matching.md` §3). Not fixable from `data.c` (the linker orders COMMON globals itself).
- `src/include/block_01.h`: `OBJ` (0x1C bytes: the 100 level objects at `0x00414140` and the 40 templates at `0x00440060`, same type [hypothesis]), `T_00414040` (table of 20 entries {points?, flags?, ?}), `HELP_TEXT_SIZE` (257).
- New globals (happy `data.c`): `screen_mode` (`0x00412030`, initial value 32), `use_joystick`, `keys[128]` (`0x00413E20`: `key[]` copy + joystick in 116–124), `time_left` (`0x00414020`, `volatile`, decremented by the timer callback `0x004018B0`), `help_texts`, `trans_table`, `buffer`, `DAT_00414040`, `DAT_00414140`, `DAT_00414c9c`, `DAT_0042f594`, `DAT_00440060`. maped `data.c`: `DAT_00414140` (`0x004108E0`), `help_texts` (`0x004113E0`), `DAT_00440060` (`0x0042A860`).
- Matching findings [verified]: `FUN_00401000` assigns field by field, not entry by entry; `blit_to_screen` uses separate screen counters in the `for` headers; `message_box` needs `volatile` on `time_left`, `strcpy` in each branch, do-while loops and a precise init order (found by trying the 120 orders).
- `scripts/build.py`: link order with `common/block_01.c` (first in happy, third in maped), `/MAP`.
- Ghidra: types `MAP`, `TILE`, `OBJ`, `HELP_TEXT` (both programs), `T_00414040` (happy); prototypes of the 7+7 map functions and of block_01; renames of the functions and globals above.

### Remaining
- **Ghidra save not confirmed**: `save_program` timed out on both programs, then every request timed out (Ghidra blocked, probably a dialog in the GUI). To check in Ghidra, then save.
- Next: happy block_02 (`0x004018B0`–`0x0040219F`), leaves first.
- maped final link: the happy-only functions of `block_01.c` use globals that maped lacks (data stubs, `matching.md` §2).

### Open questions
- Byte 256 of help text 0 is set to `'*'` by `init_help_texts` (both exes): use unknown. Is it `char[16][257]` or a struct {text[256], char}? Same code either way.
- `FUN_00401160`, second loop: the copy `f14 = f13` reads template `i` instead of `26 + i` [verified]. Bug in the original or a different meaning of the field?
- Order in the source of `map_save` (maped only) relative to `map_collision_at` / `map_t4_at` (happy only): not observable.
- Names of the fields `f77`–`f84` of `MAP`, of the tile bytes, and of `OBJ`: to be found in the callers.

## 2026-10-27 — Matching, step 2: map module complete (happy block_09, maped block_01)

Note: `notes/matching.md` §3, §3b, §5. Branch `decomp/happy`.

### Done
- [verified] `src/common/block_09.c`: 9 functions, all at 100% in reccmp. happy 7/7 (`0x0040A9A0`–`0x0040AD20`), maped 7/7 (`0x00401000`–`0x004013A0`). New: `map_create` (`0x0040AA30`), `map_load` (`0x0040AB30`), `map_collision_at` (`0x0040AC40`), `map_t4_at` (`0x0040AD20`), and for maped only `map_copy` (`0x00401160`) and `map_save` (`0x004013A0`).
- `src/include/map.h`: `MAP` laid out field by field (header of 0x85 bytes = start of a `.map` file, defaults of `map_create` at `0x77`–`0x83`, `tiles` at `0x88`).
- [verified] Correction: maped `0x00401160` is a **copy** of a map, not a resize (`layout-maped.md` §8 and `shared-code.md` corrected).
- [verified] String literals need `// STRING:` annotations, otherwise reccmp counts a difference on identical code (`map_create` 96.72% → 100%).
- Ghidra: first writes (16 functions, 3 globals renamed, both programs saved). Backup: `happy-decomp_2026_10_27.gar`.

### Remaining
- Step 3: happy block_01 (shared utilities, `0x00401000`–`0x004017C0`), with maped block_03. Done (step 3 above).
- `MAP` and `TILE` structures to create in Ghidra and apply to the map functions' prototypes. Done (step 3 above).

### Open questions
- Order in the source of `map_save` (maped only) relative to `map_collision_at` / `map_t4_at` (happy only): not observable.
- Names of the fields `f77`–`f84` and of the tile bytes `t0`, `t1`, `t3`, `t4`: to be found in the callers (game loop, editor).

## 2026-10-27 — Matching phase, step 1: infrastructure ready

Note: `notes/matching.md`. Branch `decomp/happy`. Decisions taken with the user: start of the matching phase, Ghidra writes allowed for names and types (after a fresh `.gar`), neutral file names and descriptive function names tagged `[hypothesis]`, work on a branch.

### Done
- `src/`: `common/block_09.c` (map module, annotated for HAPPY and MAPED), `happy/block_06.c`, `block_07.c`, `block_08.c` (`WinMain` through `END_OF_MAIN()`), `happy/data.c` and `maped/data.c` (globals), `include/map.h`, `happy/happy.h`.
- `reccmp-project.yml` at the root: 3 targets, `source_root: src`.
- `scripts/build.py`: VC6 build in block order, generated stubs, `/OPT:NOREF` (`--final`: `/OPT:REF`), `reccmp-project detect`.
- `scripts/progress.py`: GAME progress per block, `notes/data/progress-*.csv`.
- `scripts/crt_names.py`: CRT names by byte identity with `LIBC.LIB` (relocations ignored). `notes/data/crt-*.csv`, `src/crt/*.h` (381 `LIBRARY` annotations). happy: 152 of 155 named (before: 35), 1 import thunk, 1 ambiguous 1-byte `ret`, 1 not found.
- [verified] reccmp: happy 5 functions at 100% (`0x00406C80`, `0x00409010`, `0x0040A9A0`, `0x0040AB00`, `0x0040AC10`) and `WinMain` `0x0040A970` at 91.67% (`push main`, `main` still a stub); maped 3 at 100%. `reccmp-datacmp`: 0 issues. `smoke_reccmp.py`: still 5/5.
- [verified] With `/OPT:REF`, decompiled functions that nothing calls yet are dropped from the exe: hence `/OPT:NOREF` while decompiling.
- `CLAUDE.md` updated (with the user's authorisation): current phase, Ghidra write rules, naming, layout of `src/`, matching scripts. Ghidra backup before any write: `happy-decomp_2026_10_27.gar` (root, 18:29, ignored by git).

### Remaining
- Step 2: rest of block_09 (`0x0040AA30`, `0x0040AB30`, `0x0040AC40`, `0x0040AD20`), and maped's `0x00401160` and `0x004013A0`.
- `doctor.py` does not check `reccmp-project.yml` / `reccmp-user.yml` yet.

### Open questions
- `0x0040B376` (1-byte `ret`): `__fpclear` (87disp.obj) or `__rttosnpop` (fpinit.obj)? Not annotated; no game function calls it.

## 2026-10-27 — `dev-setup` skill run on macOS (Apple Silicon, Nix): machine ready

### Done
- First run of the skill on a Mac (macOS 27, arm64, nix-darwin + home-manager). [verified] `doctor.py --deep`: 42 ok, 0 warnings, 0 failures. `check_vc6.py` OK (CRT 145/149 + 4, `/Gy`), `smoke_reccmp.py` 5/5 at 100%.
- [verified] VC6 under Wine (Gcenx `wine-stable-11.0_1`, through Rosetta 2): the 10 hashes as expected, `cl` 12.00.8168, `link` 6.00.8168, `LIB /DEF` for `build/alleg40.lib`. reccmp's `cvdump` under Wine: OK (smoke test).
- [verified] Ghidra 12.1.4 from nixpkgs, with its native `mac_arm_64` decompiler and GhidraMCP 7.0.0: 205 tools. Project restored from the Windows `.gar` (option (a)): 220 / 200 / 63 functions.
- [verified] `captures/alleg40_unpacked.dll` (upx 5.2.1 from nixpkgs): reference SHA-256. Ghidra caches and inventory regenerated (`ghidra_dump_rest.py`, `pe_allegro.py`, `inventory.py`, `shared_code.py`): `git status notes/` empty, so the CSVs are byte-for-byte identical to the Windows ones.
- New: `flake.nix`, `flake.lock`, `.envrc` (Nix dev shell: Python 3.12, uv, upx, Wine pinned by hash, reccmp in `.uv/`). `.gitignore`: `.direnv/`, `/result`, `/.uv/`, and `!/flake.lock` (otherwise caught by `*.lock`).
- `scripts/doctor.py`: recognises a Nix Ghidra (installation, extension in `NIX_GHIDRAHOME`, JDK named in `support/launch.sh`) and the `ghidra_12.1.4_NIX` settings folder. Rosetta check through `/usr/bin/arch`: the GNU coreutils `arch` in the PATH (Nix) has no `-x86_64` and reported Rosetta as missing although it was installed.
- `macos.md`: new §0 "Nix route", with the pitfalls seen (Homebrew `cleanup = "zap"`, `ZDOTDIR` managed by home-manager, which makes `~/.zprofile` useless, MoltenVK `[mvk-info]` noise on stderr, silenced by `MVK_CONFIG_LOG_LEVEL=0`). `SKILL.md`: status of the macOS paths.

### Remaining
- Homebrew route of `macos.md` (§1-§6) still not run end to end.
- The flake only covers Darwin (Gcenx's Wine). A Linux machine would need `wineWow64Packages` from nixpkgs: not done, not tested.

## 2026-10-27 — Repository translated from French to English

### Done
- Everything in the repository that is ours is now in English: `CLAUDE.md`, `notes/*.md`, `scripts/*.py` (comments, docstrings, output, strings written to the CSVs), the `dev-setup` skill, `.gitattributes`, `.gitignore` and `3rdparty/allegro-4.0.3/README.md`. Upstream Allegro files are unchanged.
- Values in the generated CSVs: `status` is `verified` / `hypothesis`, `proposed_name` reads `NAME [hypothesis]`, blocks are `block_NN` (formerly `bloc_NN`), and `confidence` is `identical` / `very close` / `similar`. `doctor.py` tags: `[OK]` / `[WARN]` / `[FAIL]`.
- French identifiers renamed (`SEUIL_PROCHE` → `CLOSE_THRESHOLD`, `SEUIL_RESSEMBLANT` → `SIMILAR_THRESHOLD`, `BANALS` → `TRIVIAL_IMPORTS`, `preuves` → `evidence_text`, `tour` → `round_no`, `par` → `parent`). No logic changed.
- [verified] All CSVs regenerated (`pe_allegro.py`, `inventory.py`, `shared_code.py`, `list_game_files.py`, `inspect_formats.py`). Same rows and columns, and every non-text column is identical. Addresses inside the text columns are unchanged. Before the translation, the regeneration was byte-identical. `doctor.py --deep`: 38 ok, 0 failures.
- `shared-code.md` §2.3: the reference to "Layout of a `.map` file" now points to `STATUS.md`, where that section actually is (it pointed to `layout-happy.md`).
- Git history rewritten: the only commit (never pushed) was amended with the translation and an English message.

## 2026-10-27 — `dev-setup` skill: setting up and checking a machine (Windows, macOS)

### Done
- `.claude/skills/dev-setup/`: `SKILL.md` (procedure, rules, reference versions and hashes), `windows.md`, `macos.md`.
- `scripts/doctor.py`: diagnostic in 6 sections (system, repository, Python tools, VC6, Ghidra, working data), plus `--deep`. Standard library only. Reference SHA-256 hashes: game, 10 VC6 files, decompressed Allegro DLL.
- `scripts/vc6.py`: runs the VC6 tools, natively on Windows or through Wine elsewhere. `check_vc6.py` now goes through this module: [verified] identical output, line for line, before and after.
- `scripts/smoke_reccmp.py`: end-to-end test from `notes/toolchain-vc6.md` §6, made reproducible. [verified] 5/5 at 100%, `_free` matched.
- `requirements.txt` (capstone 5.0.9). `.gitattributes`: `notes/data/*.csv text eol=crlf`, since the `csv` module writes CRLF. Without this rule, CSVs regenerated on macOS would show up as modified.
- [verified on this machine] `doctor.py --deep`: 38 ok, 0 failures. Regenerating `pe_allegro.py`, `inventory.py` and `shared_code.py`: `git status notes/` empty.
- [verified] A new Ghidra 12.1.4 project created by `analyzeHeadless -import` (default analysis, throwaway project in the scratchpad) gives 220 / 200 / 63 functions, as in the reference. The `happy-decomp` project was not touched.
- `build/alleg40.lib` generated (`LIB /DEF`, through `vc6.py`).

### Remaining
- **macOS path untested**: Wine (Gcenx builds, since the Homebrew cask `wine-stable` has been disabled since 2026-09-01), VC6 and reccmp's `cvdump` under Wine, Ghidra's native `mac_arm_64` decompiler. To be validated on the first run of the skill on the Mac, then fix `macos.md`.
- Option (b) for the Ghidra project (headless import): only the function counts were compared. The full check is regenerating the CSVs (SKILL.md §6, step 4) on the machine concerned.

## 2026-10-27 — Allegro in `3rdparty/`, configured for MSVC: done

### Done
- `3rdparty/allegro-4.0.3/` (86 files, 658 KB): `include/`, `lib/msvc/allegro.def`, `readme.txt` (*gift-ware* licence), `AUTHORS`, and a provenance `README.md`. The files come from a **fresh extraction of `all403.zip`** (SHA-256 `8bff4998…8f9c`). [verified] Their `include/` is identical to the one in `captures/allegro-4.0.3/src/`.
- Only change: `alplatf.h` = `#define ALLEGRO_MSVC`, byte for byte what `fix.bat msvc` would write. The copy in `captures/` stays intact (DJGPP), for reference.
- `.gitattributes`: `3rdparty/** -text`, since `core.autocrlf=true` on this machine would have converted the original CRLFs. [verified] No file in `3rdparty/` is ignored by `.gitignore`.
- [verified] End-to-end test redone with `3rdparty/` only (headers, plus `alleg40.lib` generated from its `.def`): same reccmp results (5 functions at 100%, datacmp 0 problems).
- `CLAUDE.md` updated: location of Allegro, reccmp 0.1.7 and the ban on `reccmp-ghidra-import`, link options for reccmp, `LIBRARY` annotations for the CRT, capstone 5.0.9, scripts `shared_code.py` and `decompme_candidates.py`.

### Remaining
- First commit, and `master` or `main` branch: up to the user.
- `LIBRARY` annotations for the CRT, to be generated from the CSVs.

## 2026-10-27 — reccmp test: done, the toolchain is complete

Details: `notes/toolchain-vc6.md` §6, "reccmp" subsection. reccmp 0.1.7 was installed by the user (global Python).

### Done
- [verified] reccmp reads the VC6 PDB, with the `FUNCTION`, `GLOBAL` and `LIBRARY` annotations. The 5 decomp.me functions are at **100%** in reccmp. `0x0040AB00` gets there after the `LIBRARY` annotation for `_free`. `reccmp-datacmp`: 0 problems.
- [verified] `0x0040B463` = `free` and `0x0040B492` = `malloc`: same code as LIBC, up to the referenced CRT symbols.

### Remaining
- Generate the CRT `LIBRARY` annotations from the CSVs (otherwise every CRT call counts as a mismatch).
- Non-blocking points from the entry below: `alplatf.h`, where to put the Allegro headers in the repository, first commit, capstone version in `CLAUDE.md`.
- `CLAUDE.md`, "Tools" section, does not mention reccmp (installed in the global Python, not in `.venv`).
- Do not use `reccmp-ghidra-import` while Ghidra is read-only: this tool writes to it.

## 2026-10-27 — Final check before the matching phase: ready, except for reccmp

Details: `notes/toolchain-vc6.md` §6. No writes to Ghidra or `game/`; tests compiled in the session scratchpad.

### Done
- [verified] **Reproducible inventory**: `inventory.py happy.exe maped.exe loader.exe` rerun, the 9 CSVs in `notes/data/` and the 3 `captures/inventory-*.json` are byte-for-byte identical. Full `.text` coverage for all three exe (functions + padding = VirtualSize: 64,821, 46,615, 13,198). No UNKNOWN function (239, 223, 70).
- [verified] **Live Ghidra = cache**: `find_functions`, `find_code_gaps` and `get_function_count` identical to `captures/ghidra-cache/*_rest.json` for all three exe.
- [verified] `check_vc6.py` passes again (versions, Rich header, 145/149 + 4 CRT, `/Gy`).
- [verified] **End-to-end test** with the real Allegro headers and an import library made by `LIB /DEF`: the 5 decomp.me functions and the `WinMain` stub are identical to happy. `/Zi` + `/DEBUG /INCREMENTAL:NO /OPT:REF` change nothing in the code. Allegro is imported by ordinal, as in the game (question from `allegro.md` settled). PDB in format 2.00.
- SHA-256 hashes of the targets recorded (`toolchain-vc6.md` §6).

### Remaining (blocking)
- ~~**Install reccmp** and test it on this setup (`// FUNCTION: HAPPY 0x...`). The installation (`pip install git+https://github.com/isledecomp/reccmp`) was refused by the permission classifier: to be done by the user or to be authorised.~~ → Done: installed by the user and tested (entry above).

### Remaining (before the first code commit)
- `captures/allegro-4.0.3/.../alplatf.h` contains `#define ALLEGRO_DJGPP`: `ALLEGRO_MSVC` is needed to compile.
- The Allegro headers and the import library live in `captures/` or have to be generated, and `captures/` is ignored by git. Choose where to put them in the repository.
- The repository has **no commit** (branch `master`, whereas the expected main branch is `main`).
- `CLAUDE.md` says `capstone` 5.0.7; the `.venv` contains 5.0.9.

### Open questions (non-blocking)
- One .c or several for the game (`layout-happy.md` §9): matters for the order of functions in the whole exe, not for function-by-function matching.
- Creating the missing functions in Ghidra (19, 23, 7): useful for convenience, no effect on reccmp.

## 2026-10-27 — Check of the local VC6 toolchain: done

Note: `notes/toolchain-vc6.md`. Script: `scripts/check_vc6.py` (tests compiled in `captures/vc6-check/`). Tools: `C:\RE\tools\vc6` (CD 1 of VS 6.0 Enterprise), outside the repository.

### Done
- [verified] Versions: cl **12.00.8168** (identical to the Rich header), link 6.00.8168, cvtres 5.00.1720 (identical). The objects from `KERNEL32.LIB` (Linker512 8034) and `OLDNAMES.LIB` (AliasObj60 7291) are identical to those of the exe.
- [verified] Local `LIBC.LIB`: the 149 CRT functions of happy of 16 bytes or more are found identical, up to addresses, in test exe.
- **Correction**: `link 6.00.8447` was a misreading of the Rich header.
  - [verified] The VC6 linker does not record itself in the Rich header.
  - [strong hypothesis] The Linker600 8447 ×3 entry comes from the Allegro import library.
  - The build of the game's linker is therefore unknown. `CLAUDE.md`, `allegro.md`, `layout-maped.md` and `layout-loader.md` are corrected. Older entries in this file are left as they were.
- [verified] `/O2` implies `/Gy`: one COMDAT section per function, 16-byte aligned, with `0x90` padding. This was the open point of the "Shared code" entry. `/O2` does not inline on its own.
- [verified] AliasObj60 counts the `OLDNAMES.LIB` aliases used: happy calls 2 functions by their old name, maped and loader 1.
- [verified] The Utc12_CPP object comes from `LIBC.LIB`: the game is entirely in C.

### Remaining
- Open the matching phase (reccmp project), when the user decides.
- If an identical Rich header is wanted: find the original Allegro 4.0.3 import library (link 8447).

### Open questions
- Build of the game's linker (no effect on function code).
- Which functions are called by an old name (`strcmpi`?).

## 2026-10-27 — decomp.me candidates (VC6 version and options): done, 5/5 match

Note: `notes/decompme-candidates.md`. Script: `scripts/decompme_candidates.py` (list of GAME functions of 15 to 60 instructions and their traits; `--dump` for the disassembly with bytes). Ghidra: `get_functions` read-only (pseudo-C). Nothing was compiled.

### Done
- 5 functions selected, 3 of them identical in maped:
  - `0x0040A9A0` (maped `0x00401000`): nested loops, fields, intrinsic `memset`;
  - `0x0040AB00` (maped `0x00401260`): field, 2 calls to `free` [hypothesis];
  - `0x0040AC10` (maped `0x00401370`): imports `load_datafile`/`unload_datafile`, `setne`;
  - `0x00409010`: nested `if`s on globals;
  - `0x00406C80`: `volatile` loop, division by 15, `key[KEY_ESC]`.
- For each: disassembly, Ghidra pseudo-C, C89 proposal and uncertainties. Plus a context block for decomp.me: Allegro 4.0.3 `dllimport` declarations and the `0x8C`-byte map structure deduced from `0x0040AA30`.
- [verified] Only two jump tables exist in the game code (`0x00406560`, `0x00409870`), in functions of 200 and 449 instructions.
- [verified] `install_int(0x004018F0, 15)` (`0x00401F6B`): the counter `0x00442084` ticks every 15 ms.
- [verified] Ghidra's pseudo-C for `0x00406C80` is wrong: the `volatile` reread of the counter is lost.
- Options [hypothesis]:
  - `/O2` (or `/Ox`) by default: `/Gd`, `/Oy` without an `ebp` frame, `/Oi` intrinsics;
  - no `/Oa`, no `/GZ`;
  - `/Ob1` likely.
  Still to test: `/O1` versus `/O2` (division by a constant, scheduling) and `/GB`, `/G5` or `/G6` (no `movzx`).

- **decomp.me: `0x00406C80` matches 100%** with `/O2`, VC 6.0 no-SP preset (https://decomp.me/scratch/42r3i, §10 of the note). The other VC6 presets also match, except SP5 + Processor Pack (75%, hence excluded). Between VC 6.0 without SP and the other SPs, this function does not decide. The source computes `n = ms / 15;` in a local before `DAT_00442084 = 0;`. The two `volatile`s (`DAT_00442084`, `key`), the `while (A && B)` loop and `return ... != 0` are confirmed.

- **decomp.me: `0x0040A9A0` matches 100%** with VC 6.0 without SP and `/O2`, with the proposed C89 (local index `i`). Same 100% with `/Gs`, `/GB` and `/G5` added; **`/G6` excluded**. Options kept at this stage: `/O2`, without `/G` (the default is `/GB`) [hypothesis]. Scratch: https://decomp.me/scratch/72ufv. All VC6 presets match, except SP5 + Processor Pack, excluded on both functions.

- **decomp.me: `0x00409010` matches 100%** with VC 6.0 without SP and `/O2`, with the proposed C89 as is (https://decomp.me/scratch/8bHPJ). Only SP5 + Processor Pack does not match (3 functions out of 3).

- **decomp.me: `0x0040AB00` and `0x0040AC10` match 100%** with the proposed C89, as is (https://decomp.me/scratch/rtNCI, https://decomp.me/scratch/pJhdK).
- **Summary** (§11 of the note): 5 functions out of 5 match with **VC 6.0 without SP, `/O2`**. `/G6` and SP5 + Processor Pack do not match. The other SPs are indistinguishable on these functions.

### Remaining
- Done: VC 6.0 without SP and `/O2` recorded in `CLAUDE.md`, "Next phase" section.
- Check locally, with reccmp, function alignment (16 bytes, `0x90`) and `/Gy`, which are invisible on decomp.me.
- Establish which decomp.me VC6 compiler corresponds to cl 12.00.8168, and which service pack this number corresponds to.

### Open questions
- `0x0040A9A0`: does the source use a local index (deduced from aliasing: `f88` reread, `f01` not reread) or the repeated expression? The first test on decomp.me will decide.
- `scripts/decompme_candidates.py` is not mentioned in `CLAUDE.md`. To be added if the user wishes.

## 2026-10-27 — Game code shared between the exe: done

Note: `notes/shared-code.md`. Data: `notes/data/shared-functions.csv` (14 pairs), `captures/shared-code.json` (scores, best pairs below the threshold, variants). Script: `scripts/shared_code.py`, which reuses `inventory.py` (`.venv/Scripts/python scripts/shared_code.py`, about 15 s). Ghidra was not queried.

### Done
- Comparison of all pairs of GAME functions, for happy×maped, happy×loader and maped×loader. Criteria:
  - instructions with masked addresses;
  - tokens whose addresses are replaced by what they point to;
  - ordered sequence of calls (imports, CRT labelled in a common way, matched game code);
  - strings, constants and floats from `.rdata`.
  The pairs are mutual best pairs, computed in 2 rounds.
- **8 identical pairs** up to masking [verified]: map module (5), two utilities (`0x00401160`/`0x004016A0`, `0x004017C0`/`0x004017E0`) and the `END_OF_MAIN` stub, common to all three exe. They are the same as the `also_in` column of the inventory, found independently.
- **0 very close**: nothing between identity and `s_bytes` ≈ 0.63. [hypothesis] The common code is either compiled as is, or rewritten.
- **6 similar pairs** (measurements [verified], common origin [hypothesis]): screenshot, colour tables, datafile loading, shutdown, palette (`0.0625`, `__ftol`), reading the end of a map (`0xAF0`, `0x1010`). Two are new compared with `layout-maped.md` §9: `0x00401900`/`0x004018D0` and `0x00401D60`/`0x00401BF0`, the latter being the weakest.
- 4 pairs with the same role below the threshold ("Yes / No", initialisation, tileset change, `main`) and 3 functions specific to maped (map creation variant, map writing, writing the end of a map in `"ab"`) are described in the note, without appearing in the CSV.
- False positive ruled out: `0x00401DA0`/`0x004039B0`. Only the `fopen`/`fwrite`/`fclose` idiom is common, and the files and sizes differ.
- **Order** [verified]: apart from 3 exceptions, the shared functions follow the same order in happy and in maped (11 items out of 14):
  - the map module: first .obj of maped, last of happy;
  - the palette `0x00402EA0`/`0x00401400`;
  - "Yes / No".
  [hypothesis] Same .c for the map module and the utilities (identical code, subsets removed by `/OPT:REF`). Same skeleton but rewritten code for initialisation and shutdown, as well as the end of map: an adapted copy, or a file that evolved between the two links. In the latter case, two separate sources will be needed in the matching phase.
- loader: only the stub is common. Its `main` resembles nothing (best score 0.25).

### Remaining
- Check through a VC6 compilation experiment that `/O2` implies `/Gy`. This is needed for `/OPT:REF` to explain the subsets of common functions (claim taken from the documentation, unverified).
- Compare the order of the common groups with the order of the `.data` contributions of each exe, which would confirm the .obj boundaries.

### Open questions
- Initialisation and shutdown: file copied then adapted, or a common file that evolved between 2003-09-14 (maped) and 2003-10-27 (happy)? The binary cannot settle it.
- Do the common utilities (happy block_01, 7 functions; maped block_03, 2 functions) form a single .c? This depends on question 1 of `layout-happy.md` §9.
- `CLAUDE.md` does not yet mention `scripts/shared_code.py` in the list of inventory scripts. To be added if the user wishes.

## 2026-10-27 — Mapping of `maped.exe` and `loader.exe`: done

Notes: `notes/layout-maped.md`, `notes/layout-loader.md`. Data: `notes/data/functions-maped.csv` (223 functions), `notes/data/functions-loader.csv` (70), `notes/data/datarefs-maped.csv`, `notes/data/datarefs-loader.csv`. Ghidra was not modified: only `read_only` GETs, through `ghidra_rest.py`.

Scripts:
- `scripts/ghidra_dump_rest.py`: new dumper for GhidraMCP 7.0.0 (JSON). It replaces `ghidra_dump.py` and produces `captures/ghidra-cache/<exe>_exe_rest.json` for all three exe.
- `scripts/inventory.py`: generalisation of `inventory_happy.py`, same method. It also regenerates happy's files.
- `scripts/build_traits.py`: PE and Rich headers, CRT type tests, code traits of the three exe.

### Done
- **Non-regression** [verified]: `inventory.py happy.exe`, run on the new cache, reproduces `functions-happy.csv` and `datarefs-happy.csv`. Only 2 cells differ: the Ghidra name of the thunk `0x00410C10` (`RtlUnwind` instead of `KERNEL32.DLL::RtlUnwind`) and its reason, which now cites the import. `inventory_happy.py` stays in place for the record (old cache).
- **maped**: 223 functions, 23 of them missing from Ghidra (`main` `0x00405AE0`, a `d_slider_proc` callback, 7 thunks, 14 CRT).
  - Classes: 50 GAME (19,181 B), 165 CRT (18 [verified]), 8 THUNK, 0 UNKNOWN.
  - Game/library boundary at `0x00405CA0`. There are 7 `d_*_proc` thunks there, referenced only by the `proc` fields of `DIALOG` arrays of `0x38` bytes (`sizeof(DIALOG)` [verified in `gui.h`]). The CRT starts at `0x00405CCA`.
  - 9 blocks. Two strong boundaries:
    - the map module, first .obj of maped (`0x00401000`–`0x004013FF`);
    - [verified by the `.data` layout] `0x004013A0` (map writing, `"wb"`) is part of it.
- **loader**: 70 functions, 7 of them missing from Ghidra. Ghidra has **no function** on `0x00401000`–`0x0040149F`, that is `main`, all of the game code.
  - Classes: 2 GAME (`main` + `WinMain` stub), 67 CRT (9 [verified]), 1 THUNK.
  - [strong hypothesis] The game fits in a single .c. It has no globals (10,256 B of locals, `__chkstk`), and neither stdio nor floating point.
- **Compiler, linker and options**:
  - [verified] The Rich header gives the **same builds** in all three exe: cl 12.00.8168, link 6.00.8447, masm 6.13.7299, cvtres 5.00.1720.
  - [verified] The PE headers are identical apart from sizes and dates. maped was linked on 2003-09-14; loader and happy on 2003-10-27, 2 h 09 apart.
  - [verified] No code trait distinguishes the exe:
    - 16-byte alignment with `0x90`;
    - no `ebp` frame, except in happy functions that contain Allegro's inline asm (6 out of 9 checked);
    - inline `rep movs`/`repne scas` intrinsics;
    - strings in `.data` without duplicates.
  - **No version or option difference is observed.**
- **CRT** [strong hypothesis]: single-threaded static LIBC in all three exe. The same 5 pieces of evidence as for happy are [verified] for each (`layout-loader.md` §4.3). 62 CRT functions are identical in all three exe.
- **CRT names**: 31 [hypothesis] names from happy carried over to maped and 20 to loader, by code identity. Function ID recognises `__amsg_exit`, `__aulldiv` and `__aullrem` in maped, whose code is identical in happy. **The 3 names proposed for happy are therefore confirmed** (`0x0040B5FC`, `0x0040E7E0`, `0x0040E850`). New names [hypothesis]: `sprintf`, `_output`, `atoi`, `atol`, `_chdir`, `toupper`, `__chkstk`, `__allmul`.
- **Inline copies of Allegro**: no GAME function in the three exe is identical to code in `alleg40.dll` [verified, with a positive control on `fixmul`/`vsync`].
- **Format of `happy.clv`** [verified by the code of both exe], 8,714 bytes:
  - `0x178E` bytes of normal maps;
  - `0xA78` bytes of bonus maps;
  - two 16-bit words: number of normal maps, number of bonus maps.

  loader writes it (`0x004013D3`–`0x0040144E`); happy reads it (`0x00409F6D`…) and rewrites it (`0x0040A7A7`…). [strong hypothesis] It contains 18 + 8 slots of 335 bytes.
- Questions 3, 4 and 10 for happy (`layout-maped.md` §9):
  - .obj order: map module first in maped, last in happy;
  - common code: the map module and 2 utilities are identical, hence common sources reduced by `/OPT:REF` [hypothesis]; initialisation and shutdown are a skeleton copied then adapted;
  - the shared map file contains at least 9 functions, in a known order.
- Question 1 for happy (a single .c?), advanced but not settled:
  - [verified in maped] The contributions to `.data` follow the .obj order. In each .obj, initialised globals come before strings.
  - Neither maped nor happy has initialised globals between two groups of game strings.
  - Lead: calibrate the Utc12_C counts of the Rich header with loader (29 C objects, 1 of them from the game).
- `get_function_count` counts: 295 = 200 + 95 and 118 = 63 + 55 [verified]. There are fewer external functions than imports (112 and 59). [hypothesis] Ghidra counts as labels the data imports and the imports reached only through a thunk: happy 12 + 1 = 13, maped 9 + 8 = 17. loader keeps 1 unexplained import (4 labels for 2 data imports + `RtlUnwind`).

### Remaining
- VC6 compilation experiment (unchanged). To be added to it:
  - string merging across .obj, for the `"OK"` anomalies in happy and `""` in loader (`0x00406708`, placed in the CRT);
  - the link between .obj order and `.data` order.
- List of the `LIBC.LIB` (VC6) members actually linked in each exe. It is used to deduce the number of game .obj from the Rich header (question 1).
- Read the contents of a 335-byte slot of `happy.clv` (loader `0x00401110`–`0x004013C0`, happy `0x00409F30`).

### Open questions
- **To decide** (extended): create the missing functions in Ghidra (happy 19, maped 23, loader 7, including loader's `main`, which Ghidra does not cover at all) and fix the truncated bodies (8, 9, 5)? This is a write, to be explicitly authorised.
- Questions specific to maped (`layout-maped.md` §10): one or several .obj after the map module; end of the file shared with happy block_01; empty function `0x00403140`; `AliasObj60` 1 versus 2.
- ~~`CLAUDE.md` still mentions `ghidra_dump.py` and `inventory_happy.py`.~~ → Done at the user's request. `CLAUDE.md`, "Ghidra MCP" section, contains:
  - the new inventory scripts;
  - the counts for maped and loader;
  - two API pitfalls: the `size` field of `get_functions`, and the case of keys in batch mode;
  - `find_code_gaps` with `limit=0`.

## 2026-10-27 — Migration to GhidraMCP 7.0.0 and new Ghidra project: verified

Context: the GhidrAssistMCP MCP server was replaced by GhidraMCP 7.0.0. Under Ghidra 12.2, every decompilation failed (`ghidra/program/model/lang/SleighLanguageDescription`), because the extension is built for 12.1.4. The user therefore went back to Ghidra 12.1.4 and recreated the project. MCP user guide: `CLAUDE.md`, "Ghidra MCP" section. New script client: `scripts/ghidra_rest.py` (read-only).

Checks, done without any write to Ghidra:
- [verified] Ghidra 12.1.4 (PID 18368), project `happy-decomp`, `happy.exe`, `maped.exe` and `loader.exe` open. Analysis finished for all three. Language `x86:LE:32:default`, base `0x00400000`. Decompilation works (`0x0040A970` → `_WinMain(...)`).
- [verified] `happy.exe` compared with the cache of the old project (`captures/ghidra-cache/happy_exe.json`):
  - same 220 functions (addresses) and same names. Only the display differs: `KERNEL32.DLL::RtlUnwind` becomes `RtlUnwind`;
  - full disassembly identical (addresses and instructions) for the 220 functions;
  - xrefs identical for the functions checked;
  - same 117 imports and same 224 strings (addresses).
- [verified] Only difference: the `.data` blocks now stop at the PE VirtualSize instead of being rounded up to `0x1000`. happy: `0x00412000`–`0x00447C43` (VirtualSize `0x35C44`), i.e. −956 bytes of memory size; maped: `0x0040E000`–`0x0042C473` (`0x1E474`), i.e. −2,956 bytes; loader unchanged. No address in `datarefs-happy.csv` falls in the removed part.
- [verified] The 324 / 295 / 118 counts from the environment come from `get_function_count`, which also counts imported functions: happy 324 = 220 internal + 104 imported (the cache lists 104 "Function" imports and 13 "Label" ones). [hypothesis] Likewise for maped (200 internal, 112 external locations) and loader (63 internal, 59 external locations). This resolves the "324 not reproduced" question further down. → *maped/loader*: the hypothesis about maped and loader was wrong (200 + 112 ≠ 295). The real counts are 295 = 200 + 95 and 118 = 63 + 55 [verified] (section of the day).
- Not compared: maped and loader, for lack of a cache from the old project. The function counts match those recorded during the environment check. → *maped/loader*: cache created (`captures/ghidra-cache/*_rest.json`).

Remaining: ~~rewrite `ghidra_dump.py` for the new server at the time of the maped and loader inventory. The cache format will then change (JSON instead of text), so `inventory_happy.py` will have to be adapted.~~ → *maped/loader*: done, with `ghidra_dump_rest.py` and `inventory.py`. Happy is reproduced identically.

## 2026-10-27 — Mapping of `happy.exe`: done

Note: `notes/layout-happy.md`. Data: `notes/data/functions-happy.csv` (239 functions), `notes/data/datarefs-happy.csv` (518 referenced data addresses). Scripts: `scripts/ghidra_mcp.py` (read-only MCP client), `scripts/ghidra_dump.py` (copy into `captures/ghidra-cache/happy_exe.json`), `scripts/inventory_happy.py` (analysis, to be run with `.venv/Scripts/python`; `capstone` 5.0.7 installed in `.venv/`). Ghidra was not modified.

### Done
- [verified] 239 functions: the 220 from Ghidra and **19 that Ghidra did not create**, including the game's `main` (`0x0040A830`), two timer callbacks, `_except_handler3`, `atexit` and dead CRT code. Ghidra also gives a truncated body for 8 functions (jump tables, SEH handlers).
- Classification:
  - GAME: 84 functions, 39,634 B [hypothesis], including 51 B of Allegro header code (`0x0040A970`, `0x00402110`);
  - CRT: 154 functions, 24,444 B (16 [verified] by Function ID or entry point, 138 [hypothesis]);
  - THUNK: 1 function [verified];
  - UNKNOWN: 0.
  - [verified] 142 of the CRT functions exist identically in `maped.exe` and/or `loader.exe`.
- [verified] Game/CRT boundary at `0x0040AD50` (`rand`). Before it, all functions are 16-byte aligned with `0x90`.
- [verified] Call path: `entry` `0x0040B506` → `0x0040B5CF call 0x0040A970` (`WinMain`, `END_OF_MAIN` stub, `ret 0x10`) → `_WinMain` (IAT `0x00411120`) with `0x0040A830` = `main(argc, argv)` (options `-widescreen`, `-fps`).
- [strong hypothesis] **Single-threaded LIBC** CRT:
  - [verified] no TLS or critical section import;
  - [verified] only `FS:[0]` (SEH);
  - [verified] no `lock`;
  - [verified] `_heap_init(0)` without `_mtinit`;
  - [verified] `&errno` passed as an immediate (`0x00413B3C`).
- 10 layout blocks. Two strong anchors:
  - the map module `0x0040A9A0`–`0x0040AD4F`, identical to the first .obj of `maped.exe`;
  - `main` and the `WinMain` stub, necessarily in the same file.
  The other internal boundaries of the game are weak: they rely on the theme and on single-caller functions placed just before their caller.
- Questions from the environment check resolved:
  - `Max Address ffdfffff` comes from the Ghidra block `tdb` (`0xFFDFF000`, `FS:` segment);
  - the count of 324 functions does not reproduce: Ghidra counts 220 today.
- [verified] Local `static INLINE` copies of Allegro: no game function is duplicated. Only out-of-line copy: `0x00402110`.

### Remaining
- ~~Inventory of `maped.exe` and `loader.exe` with the same scripts (to be generalised beyond `happy`), then comparison of the function orders (questions 3, 4 and 10 of the note).~~ → *maped/loader*: done (`layout-maped.md` §9).
- VC6 compilation experiment (cl 8168, link 8447) to establish the `.data`/`.bss`/COMMON order and where `__inline` copies go. It will make the boundary clues decisive (questions 1, 2, 6 and 7).

### Open questions
- The 10 questions of `notes/layout-happy.md` §9. The most urgent: one .c or several for the game code; compilation options.
- **To decide**: create the 19 missing functions in Ghidra and fix the 8 truncated bodies? This is a write to Ghidra, so it must be explicitly authorised.
- ~~`.venv/` not ignored by git~~: settled, `.gitignore` created with `.venv/`.

### Reconciliation with the previous sections (2026-10-27)
The hypotheses and questions from the previous tasks were checked against the mapping. Each point is annotated in place with "→ *Mapping*". In short:

| Origin | Point | Before | Now |
|---|---|---|---|
| Environment | `Max Address ffdfffff` | question | **resolved** [verified]: `tdb` block |
| Environment | "the function counts include the CRT" | [hypothesis] | **confirmed and quantified** for happy: 154 CRT out of 239 |
| Environment | 324 functions for happy | [verified] | **not reproduced**: 220 today (maped 295 and loader 118 not rechecked) |
| Allegro | block `0x10026A10`–`0x10027F00` = `src/inline.c` | [hypothesis] | **[verified in the sources]** (`notes/allegro.md` §3) |
| Allegro | `WinMain` stub happy `0x0040A970` | identification as hypothesis | **[verified]**: called by the PE entry point with the 4 `WinMain` arguments, `ret 0x10`, calls `_WinMain` |
| Allegro | `0x0040A830` = `_mangled_main` | hypothesis | **[verified]**: `main(argc, argv)` that reads the options |
| Allegro | `0x00413B3C` = `errno`, hence LIBC | hypothesis | **[strong hypothesis]**, with 4 independent pieces of evidence (§4 of `layout-happy.md`) |
| Allegro | `0x0040B31E` = `atexit` | hypothesis | hypothesis, strengthened: calls `0x0040B2B1` (`_onexit` [hypothesis]) and returns `-1` or `0` |
| Allegro | local `static INLINE` copies (`alcompat.h`) | to watch | happy: **none** [verified]; → *maped/loader*: **none** either [verified by comparison with `alleg40.dll`] |
| Allegro | mark the header code and the thunks | remaining | happy: **done** (`allegro-header` subclass); → *maped/loader*: **done** (`WinMain` stubs as `allegro-header`, maped's 7 `d_*_proc` thunks as THUNK) |
| Data | W and H at bytes 1 and 2 of a map | [verified by arithmetic] | **[verified in the code]** (`0x0040AB30`) |
| Data | fixed header of 2,933 or 7,045 bytes | question | **resolved**: it is not a single header (details below) |
| Data | formats of `happy.sav` and `happy.clv` | question | `happy.sav`: **layout known**; `happy.clv`: → *maped/loader*: **layout known** (`layout-loader.md` §3) |
| Data | PPCOL linked into happy? | question | still open: no `ppcol`/`ppmask` string, no candidate block |

Layout of a `.map` file [verified] (code of `0x0040AB30` and `0x00403070`, plus the sizes of the 10 `.map` files in `game/maps/`, all equal to 7,045 + 5·W·H):

| Offset | Size | Content | Read by |
|---|---|---|---|
| `0x0000` | `0x85` (133) | header: byte 0 = 2 (loading fails otherwise), byte 1 = W, byte 2 = H; [hypothesis] map name at byte 5 (`"BONUSROOM!"`, `"Rolling Hills 1"`) | `0x0040AB30`, directly into the `0x8C`-byte structure |
| `0x85` | 5·W·H | tiles (5 bytes each) | `0x0040AB30`, into `[map+0x88]` |
| `0x85 + 5·W·H` | `0xAF0` (2,800) | 100 entries of `0x1C` (28) bytes, into `0x00414140`. [hypothesis] Objects or NPCs: loop from `0x00414151` to `0x00414C41`. | `0x00403070`, after `fseek` (`0x0040B3D7` [hypothesis]) |
| `+0xAF0` | `0x1010` (4,112) | 16 strings of 257 bytes, into `0x00415CE0`, at the location that `0x004017C0` fills with default help texts. **Missing from the 2000 maps.** | `0x00403070` |

This explains the two values in `game-docs.md`: 2,933 = 133 + 2,800, and 7,045 = 133 + 2,800 + 4 112. [hypothesis] For a 2000 map, the 2nd `fread` fails at end of file and the default texts from `0x004017C0` stay in place.

Layout of `happy.sav` [verified]: `0x00401DA0` writes and `0x00401E10` reads, in this order, 206 bytes:

| Order | Bytes | Variable |
|---|---:|---|
| 1 | 1 | `[0x00442088]` |
| 2 | 1 | `[0x00413D40]` |
| 3 | 28 | `[0x0042F580]` |
| 4 | 168 | `[0x00413D60]` |
| 5 | 4 | `[0x00412030]` |

After reading, `[0x00412030]` is reset to 0 if it is not 1. [hypothesis] This is the widescreen flag: `-widescreen` sets it to 1 and `0x00401380` picks `blit` or `stretch_blit` depending on its value.

## 2026-10-27 — Allegro version: done

Note produced: `notes/allegro.md`. Data: `notes/data/alleg40-exports.csv` (1,292 exports), `notes/data/alleg40-imports.csv` (154 imports). Script: `scripts/pe_allegro.py`. Working copy: `captures/alleg40_unpacked.dll` (`upx -d`).

### Done
- [verified] **Allegro 4.0.3, MSVC build**. Evidence: version resource `4.0.3`, string `"Allegro 4.0.3, MSVC"` at `0x10075010`, pointed to by the `allegro_id` export. DLL linked on 2003-04-19 with link 6.00.8447, C compiler build **8966** (Rich header), dynamic CRT (`MSVCRT.dll`).
- [verified] The exe were compiled with build **8168** and linked with 8447 (Rich header), which confirms `CLAUDE.md`. The DLL was therefore compiled with a different compiler from the game.
- [verified] Imports from `alleg40.dll`, **all by ordinal**: happy 72, maped 64, loader 18. Names resolved through the export table and identical to those from Ghidra `list_imports`.
- [verified] Contiguous block `0x10026A10`–`0x10027F00` of exports that corresponds to the inline functions of the headers. [hypothesis] It comes from `src/inline.c`. → *Mapping*: [verified in the sources], see `notes/allegro.md` §3.
- [verified in bytes, identification as hypothesis] Allegro header code inlined into the game:
  - `WinMain` stubs from `END_OF_MAIN()`: happy `0x0040A970`, maped `0x00405C70`, loader `0x004014A0`. → *Mapping*: happy **[verified]** by the call path from the PE entry point (`notes/layout-happy.md` §3); `0x0040A830` = `main(argc, argv)` [verified]. The same stub, byte for byte (addresses masked), exists in maped and loader;
  - `allegro_init()`: happy `0x00401F09`. → *Mapping*: `0x0040B31E` (`atexit` [hypothesis]) calls `0x0040B2B1` (`_onexit` [hypothesis]); `0x00413B3C` (`errno`) is corroborated by 4 independent pieces of evidence for LIBC;
  - `_set_errno_erange`: happy `0x00402110`;
  - `fixmul`/`fixdiv` in MSVC asm, `fixsin`/`fixcos`, `itofix`, `fixtoi`;
  - `SCREEN_W`/`SCREEN_H`;
  - `rectfill` through the vtable.
- [verified] `packfile_password("happy")`: happy `0x00401F24`/`0x00401F29`, maped `0x00401C2B`/`0x00401C30` and `0x004028B8`/`0x004028BD`. This point was in the "remaining" list of the previous task.

- [verified] Official Allegro 4.0.3 sources retrieved: `all403.zip`, GitHub release `liballeg/allegro5` tag `v4-0-3`, since it is missing from liballeg.org/old.html. Extracted into `captures/allegro-4.0.3/`. What they confirm:
  - `ALLEGRO_DATE 20030419`, the same day the DLL was compiled;
  - the 1,292 ordinals of `lib/msvc/allegro.def` are identical to those of the DLL;
  - `fixmul`, `fixdiv`, `_set_errno_erange` (`al386vc.h`), `fixsin`, `fixcos`, `fixtoi`, `SCREEN_W`/`SCREEN_H`, `END_OF_MAIN`, `allegro_init` and `rectfill` are identical to the observed code;
  - the list of inlines (`notes/allegro.md` §5) is now extracted from the headers rather than written from memory;
  - `LOCK_*`/`END_OF_FUNCTION` are empty under Windows;
  - the C versions of `fixmul`/`fixdiv` round through `double`, which justifies the 64-bit integer rewrite planned for C17.

### Remaining
- ~~Inventory of happy's code, with `0x00402110` and the `WinMain` stub marked as Allegro header code~~ → *Mapping*: done (`allegro-header` subclass in `functions-happy.csv`).
- ~~Inventory of maped and loader, with maped's `jmp [IAT]` thunks (`0x00405CA0`–`0x00405CC2`) marked as linker-generated: remaining.~~ → *maped/loader*: done (THUNK class, `block_08_thunks`).

### Open questions
- ~~"No inline assembler" rule~~: **decided on 2026-10-27**. The rule targets the game code. The Allegro 4.0.3 headers are included as is, asm included. In C17, the fixed-point maths will be rewritten with 64-bit integers. `CLAUDE.md` is updated.
- Imports by ordinal: the official `.def` gives every export an explicit ordinal. [strong hypothesis] That is why the import lib generated by LINK imports by ordinal. To be checked by generating the lib in the next phase.
- Possible local copies of the `static INLINE` aliases from `alcompat.h` (`fmul`, `fsin`, `clear`…) in the exe: to watch during the function inventory. → *Mapping*: happy has **none** [verified: no duplicated function in the game code, `_set_errno_erange` is the only out-of-line copy]. maped and loader remain. → *maped/loader*: none either [verified].
- Which VC6 service pack build 8966 corresponds to. No impact on matching the exe.

## 2026-10-27 — Game documentation and data files: done

Note produced: `notes/game-docs.md`.

### Done
- Read all the text files in `game/` without modifying them: `readme.txt`, `loader.txt`, `maped.txt`, `maped.ini`, `maps/rolling_hills/rolling_hills.txt`, plus `happy.url`. The note summarises the genre, the goal, the controls and the role of the three exe; it gives the options (no command-line option, drag and drop for `loader.exe` and `maped.exe`, `maped.ini` keys), the documented formats and limits (18 levels and 8 bonus levels per campaign, 6 layers per map) and quotes the authors and the distribution terms verbatim.
- Inventory of `game/data/` and `game/maps/` (size, extension, first 32 bytes) in `notes/data/game-files.csv`, produced by `scripts/list_game_files.py`.
- [verified] The 7 `.dat` files are Allegro packfiles (LZSS) encrypted with the password `happy`, with two header schemes. `scripts/inspect_formats.py` decrypts them into `captures/unpacked/` and lists their objects in `notes/data/datafiles.csv`. The game's maps are `DATA` objects in `LEVELS.DAT` and `xlevels.dat`. Tiles in 16×16, images in 8 bpp, backgrounds in 320×200.
- [verified by arithmetic] Size of a map = fixed header (2,933 or 7,045 bytes) + 5·W·H, with W and H at bytes 1 and 2. → *Mapping*: the size is right, but it is not a single header. Layout [verified in the code]: 133-byte header, tiles (5·W·H), 2,800 bytes of entries, then 4,112 bytes of texts, missing from the 2000 maps. See the reconciliation, "Mapping" section.

### Remaining
- ~~Code inventory~~ → *Mapping*: done for happy (`notes/layout-happy.md`); maped and loader remain.
- ~~Check in Ghidra the `packfile_password` call with the string `happy` (`happy.exe` `0x004124B0`, `maped.exe` `0x0040F1B0`).~~ Done in the "Allegro version" task.

### Open questions
- ~~Layout of the two map headers (2,933 and 7,045 bytes)~~ → *Mapping*: resolved (see above). Still open: the fields of the 133-byte header after byte 2, the structure of the 28-byte entries and the contents of the 5 bytes of a tile.
- Formats of `happy.sav` and `happy.clv` (presumed exchange file between `loader.exe` and `happy.exe`). → *Mapping*: `happy.sav` = 206 bytes, layout known [verified]. The meaning of the variables remains to be established. `happy.clv`: the string exists in happy (used by `0x00409F30`, the "CREATE CUSTOM GAME" screen) and in loader [verified]; the format remains to be read in `0x00409F30`. → *maped/loader*: **resolved** [verified by the code of loader and happy], 8,714 bytes (`layout-loader.md` §3). The contents of the 335-byte slots remain to be read.
- Is PPCOL (thanked in `readme.txt`) statically linked into `happy.exe`? → *Mapping*: still open. No `ppcol`/`ppmask` string in the three exe [verified], and no block in `layout-happy.md` looks like a separate library. A version of PPCOL made of macros, or a copy built into the game source, would leave no obvious trace.

## 2026-10-27 — Environment check: OK

Session: Claude Code launched on Windows in `C:\RE\happy-decomp` (loads `.mcp.json`, PowerShell shell available).

### 1. MCP servers
- [verified] `ghidra` (HTTP `http://127.0.0.1:8080/mcp`, source `.mcp.json`): connected and responding.
- [verified] `x64dbg` (`http://127.0.0.1:9095/`, source `.mcp.json`): connection failure (ECONNREFUSED). Not needed for the inventory phase.

### 2. Programs open in Ghidra
Source: `list_open_programs`, `get_program_info`, `list_entry_points`, then cross-checked by reading the PE header directly in Python.

| Program | Format | Architecture | Base | Functions (Ghidra) | Entry point |
|---|---|---|---|---|---|
| happy.exe  | PE32 | x86:LE:32:default | `0x00400000` | 324 (→ 220 today, see below) | `0x0040B506` |
| maped.exe  | PE32 | x86:LE:32:default | `0x00400000` | 295 | `0x004063A1` |
| loader.exe | PE32 | x86:LE:32:default | `0x00400000` | 118 | `0x004017AF` |

- [verified] PE header (reading the bytes of `game/*.exe`) for all three: machine `0x014C` (i386), magic `0x10B` (PE32), 4 sections, subsystem 2 (Windows GUI), linker version 6.00 (consistent with link 6.00.8447 stated in CLAUDE.md). The base and the entry point match Ghidra's values.
- [verified] Ghidra reports `Max Address: ffdfffff` for all three programs. The list of memory blocks will have to be checked to find out where this value comes from. → *Mapping*: resolved. It is the `tdb` block (`0xFFDFF000`–`0xFFDFFFFF`) that Ghidra creates for the `FS:` segment [verified for happy, `list_memory_blocks`].
- [hypothesis] The function counts include the statically linked VC6 CRT. The game's own code is therefore smaller. To be quantified during the inventory. → *Mapping*: **confirmed** for happy. Out of 239 functions, there are 84 GAME (39,634 B), 154 CRT (24,444 B) and 1 thunk. The figure 324 does not reproduce: `list_functions` and `get_program_function_statistics` give 220. The figures for maped (295) and loader (118) were not rechecked. → *maped/loader*: rechecked. 295 = 200 + 95, 118 = 63 + 55; with the added functions, 223 and 70 functions.

### 3. Tools
- [verified] `python --version` → `Python 3.12.10`.
- [verified] `upx --version` → `upx 5.2.1`.

## Remaining
- ~~Start the inventory~~ → *Mapping*: done for happy.

## Open questions
- None blocking. `x64dbg` is offline, to be restored only if dynamic analysis becomes necessary.

---

## History
- 2026-10-27, 1st pass (cloud session): blocked. The session had access neither to the ghidra MCP nor to Windows PowerShell. It was decided to work from Claude Code launched on Windows.
- 2026-10-27, 2nd pass: blocked. `upx` was missing and no program was open in Ghidra. Both points have been fixed since.
