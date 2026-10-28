# The game as a player and modder sees it

Sources: the text files of `game/` (`readme.txt`, `maped.txt`, `loader.txt`, `maped.ini`, `maps/rolling_hills/rolling_hills.txt`), the datafiles, and the decompiled code (`src/`, cited by address). The objects of each datafile: `notes/data/datafiles.csv`.

## 1. The game

- [verified] **Happyland Adventures, X-Mas Edition** v1.3, a freeware platform game by Johan Peitz / Free Lunch Design, 2000-2003 (`readme.txt`), "a classic jump'n'run combined with original puzzle elements". The original game won an amateur contest on gamedev.net; the X-Mas Edition is its sequel.
- [verified] Goal (`readme.txt`): touch the Happylanders so that they follow you and bring them to a safe zone; when enough are saved, the exit opens. Levels have a time limit, sometimes several or hidden exits; old wise men give hints. Saving all of them turns the enemies into coins; 250 bonuses give a life; 5 blue spheres give a password; all the coins open the bonus room. Pickups: star, diamonds, heart, time glass, coin, blue potion (invulnerability), low-G shoes, blue spheres.
- [verified] Start menu (`start_menu`, `0x00409870`): three bands, the X-Mas logo (loads `data/xlevels.dat`), "HAPPYLAND ADVENTURES / THE ORIGINAL GAME" (`data/levels.dat`), "CREATE CUSTOM GAME" (§4.3), plus "EXIT" top right. `W` toggles "WIDESCREEN" (`0x00412E24`), 320×200 ↔ 640×480 (the readme's way to get "full screen"). Main menu: PLAY, PASSWORD (X-Mas only), OPTIONS (keys, joystick, volumes), HELP ME, EXIT (`main`, `0x0040A830`).
- [verified] Default controls (`readme.txt`; `default_settings`, `0x00401EA0`): arrows Left/Right walk, Down slides, Left Alt jumps, Left Ctrl talks (and closes message boxes), F12 screenshot, Pause pauses, Esc quits ("Quit?"). Keys and joystick are redefinable.
- [verified] Level passwords: 21 strings at `0x00412034`, each character stored + 1 (`"opof"` = `"none"`); the password screen (`0x00408500`) also accepts `HAXE-RESET`.
- [verified] Requirements (`readme.txt`): Pentium, 8 MB RAM, VGA; joystick and Sound Blaster optional.

## 2. Files

- `happy.exe` (86,016 bytes): the game. `maped.exe` (73,728): the map editor (`maped.txt`). `loader.exe` (32,768): the "CAMPAIGN LOADER" by Eric Love (`loader.txt`), which turns a campaign `.txt` into `happy.clv` (§4.3). `alleg40.dll` (224,768): Allegro 4.0.3 (`notes/allegro.md`).
- `unins000.exe`/`.dat`: Inno Setup uninstaller [verified: `Inno Setup Uninstall Log (b)`]. `happy.url`: link to `http://www.freelunchdesign.com`. The installer `HAPPY_INSTALL.EXE` named by `readme.txt` is not shipped.
- `data/`: seven datafiles (§4.1). `maps/examples/`: `bonus_1`, `bonus_2`, `jungle`, `stix`, `winter` `.map`. `maps/rolling_hills/`: `rolling_hills_1`–`3`, `rh_bonus_1`–`2` `.map` and `rolling_hills.txt`.
- Created at run time, by relative name in the current folder: `happy.sav`, `happy.clv`, `happy.pcx`, `hland.$$$` (a level of a datafile is written there, loaded and deleted, `load_level`, `0x00403170`); maped's `autosave.map`, `maped000.<ext>`... (maped first changes to its exe's folder). loader writes `happy.clv` next to `loader.exe`.

## 3. Command line and configuration

### happy.exe

- [verified] `main` (`0x0040A830`) accepts `-widescreen` and `-fps` (case-insensitive), undocumented and **without effect**: `init_game` then always overwrites `screen_mode`, and nothing reads the `-fps` flag.
- [verified] F12 writes `happy.pcx` (`play_level`, `0x00404C50`), overwritten each time, although `readme.txt` announces `HAPPY???.PCX`.
- [verified] **`happy.sav`** (202 bytes, no header), written on exit (`save_settings`, `0x00401DA0`), read at start (`load_settings`, `0x00401E10`), defaults otherwise:

| Offset | Size | Contents |
|---:|---:|---|
| 0 | 1 | sound volume [hypothesis: digital], default 200 |
| 1 | 1 | MIDI volume, default 220 |
| 2 | 28 | 7 `int` Allegro `KEY_*` codes: 0 right, 1 left, 4 jump, 5 talk, 6 slide (2, 3 unused) |
| 30 | 168 | 7 high scores: `char name[20]`, `unsigned score` (default "Johan Peitz", 10000 down to 2500, 6 entries) |
| 198 | 4 | `screen_mode`: 1 = 640×480 (frame stretched to 640×400), otherwise reset to 0 = 320×200 |

### maped.exe

- [verified] `maped.exe <file.map>` (drag and drop) loads the map for editing (`main`, `0x00405AE0`). The image that `maped.txt` describes for this use is made with F7 (`export_map_image`, `0x00402480`), named after the map.
- [verified] Keys (`editor_loop`, `src/maped/block_07.c`; full list in `maped.txt`): Alt+F menu, Alt+N/L/S/A/R new (50×50, `./maps/unnamed.map`), load, save, save as, revert; Alt+M properties; F5 start position; F7 export; F8 screenshot `maped000`–`maped999` (`0x004018D0`); F12 320×200 ↔ 640×480 (only for maps over 40×30 tiles, after saving `autosave.map`; `0x00401A20`).
- [verified] **`maped.ini`** (Allegro config file, `key = value`, `#` comments; `init_editor`, `0x00401C20`). Shipped values / defaults when absent: `run_in_a_window` 0/0 (1 = windowed); `output_format` `bmp`/`pcx` (`bmp`, `pcx` or `tga`, for F7 and F8); export layers `show_background`, `show_base_tiles`, `show_cover_tiles`, `show_items`, `show_npcs` 1/1, `hilite_cover_tiles` 1/0, `show_types` and `show_masks` 0/0.

### loader.exe

- [verified] `loader.exe <campaign.txt>` (drag and drop), otherwise a file selector in `maps/` (`main`, `0x00401000`). Writes `happy.clv`; exit codes 2 list unreadable, 3 missing map, 4 too many maps, 5 end of file before the closing `#`.

## 4. Formats and limits

### 4.1 Datafiles `data/*.dat`

[verified] Allegro 4 datafiles, LZSS-packed and encrypted with the packfile password **`happy`**, set by happy (`init_game`, `0x00401F00`) and maped (`init_editor`) before loading, and cleared while saving images (`save_screenshot`, `0x00401900`). First 4 bytes (big-endian): `0x031C0930` = `"slh!"` XOR password, only the LZSS flag bytes encrypted (`misc`, `SPRITES`, `XSPRITES`, `tiles`, `xtiles`); `0x031C091A`, whole stream encrypted (`LEVELS`, `xlevels`) [hypothesis: Allegro's old and new schemes].

| File | Packed / unpacked | Contents |
|---|---:|---|
| `LEVELS.DAT` | 45,528 / 212,055 | 14 `DATA` maps of the original game: BONUS01–04, LEVEL00–06, SECRET00, TITLELEV, X_HELP |
| `xlevels.dat` | 108,502 / 538,079 | 24 `DATA` maps of the X-Mas Edition: BONUS01–04, LEVEL01, LEVEL11–14, 21–23, 2a, 31–34, 41–43, SECRET00, TITLELEV, X_HELP, X_OUTRO |
| `misc.dat` | 416,045 / 854,627 | 7 `BMP `, 6 `RLE `, 2 `FONT`, 14 `MIDI`, 9 `SAMP` |
| `SPRITES.DAT`, `XSPRITES.DAT` | 16,801, 19,474 | 152, 158 `BMP ` sprites |
| `tiles.DAT`, `xtiles.dat` | 17,835, 18,540 | 1 `PAL ` + 130 `RLE ` tiles |

- [verified] Bit 3 of map byte `0x79` selects the `x` sprite and tile files (happy `load_level`, maped `main`).
- [verified] All images 8 bpp; tiles 16×16; sprites 16×16, 16×20, 16×23, 64×53 (`SANTA004`); backgrounds `BG1`–`BG3`, `FINAL` 320×200, the size of the game's back buffer (`init_game`).
- [verified] `ORIG` properties: `c:\djgpp\fld\happy\…` (2000), `c:\program\micros~3\myproj~1\happy\…`, `c:\Documents and Settings\Johan\…` (2002-2003). [hypothesis] Started under DJGPP, moved to Visual C++.

### 4.2 Maps (`.map` files and the `DATA` objects above)

[verified] From `map_load` (`0x0040AB30`), `map_save` (maped `0x004013A0`), `load_level_objects` (`0x00403070`), `save_objects` (maped `0x004039B0`); structures `MAP`, `TILE` (`src/include/map.h`) and `OBJ` (`src/include/block_01.h`), whose comments say which field meanings are hypotheses.

| Offset | Size | Contents |
|---|---:|---|
| `0x00` | 1 | version, must be 2 |
| `0x01`, `0x02` | 2 | width, height in tiles |
| `0x03`, `0x04` | 2 | player start tile (F5) |
| `0x05` | 80 | level name |
| `0x55` | 34 | tile datafile name |
| `0x77` | 14 | properties: `0x77` background (0-2), `0x78` music (0-11), `0x79` flags (bit 3: X-Mas set), `0x7A` time limit (s), `0x7B`/`0x7C` Happylanders to save / in all |
| `0x85` | 5·W·H | tiles, row-major (`0x0040AC40`): base tile, cover tile, walk mask (collision type 0-10), item, tile type (1 exit, 5 ice...) |
| then | 2,800 | 100 objects (Happylanders, enemies, wise men) of 28 bytes: `int x, y` in pixels, template index at `+0x18` |
| then | 4,112 | 16 help texts of 257 bytes (the wise men's) |

[verified by the sizes] A map is `7,045 + 5·W·H` bytes; the 12 maps of `LEVELS.DAT` dated 2000 lack the help texts (`2,933 + 5·W·H`), and maped reinitialises them when byte 256 of the first is not `'*'` (`0x00403200`). Largest shipped: 171×32 (`winter.map`), 95×91 (X-Mas LEVEL33).

### 4.3 Campaigns: `.txt` and `happy.clv`

- [verified] Campaign `.txt` (`loader.txt`, loader `main`): free text, a `#` line, the levels one per line, a `#` line, the bonus maps, a `#` line, free text. Paths relative to the `.txt`; every map must exist. Example: `rolling_hills.txt` (3 levels, 2 bonus).
- [verified] **`happy.clv`**, 8,714 bytes, unencrypted (`src/loader/block_01.c`; happy `custom_game`, `0x00409F30`): 18 level slots then 8 bonus slots of 335 bytes (`char file[255]`, path of the map; `char name[80]`, the name shown), then the two counts as 16-bit little-endian. happy's custom game screen edits it (add with `file_select`, right click to remove), requires at least one level and one bonus map, and saves it before playing.

### 4.4 Limits

| Limit | Value |
|---|---|
| Map size | not smaller than the screen (`maped.txt`); at most 255×255 tiles (one byte each) [verified] |
| Objects per map | 100 [verified] |
| Help texts per map | 16 of 256 characters [verified] |
| Levels / bonus maps per campaign | 18 / 8 (`loader.txt`; loader and `custom_game`) [verified] |
| High scores | 7 [verified] |

## 5. Authors and licence

- `readme.txt`: "a game written by Johan Peitz", "brought to you by Free Lunch Design", "(c) 2000 - 2003", art by Johan Peitz and from Ari Feldman's SpriteLib; X-Mas levels by Johan Peitz and Mark Hickford. Thanks to Ari Feldman (SpriteLib), Shawn Hargreaves (Allegro), Ivan Baldo (PPCOL), Mark Hickford, Arnon Yaari.
- `readme.txt` allows redistributing the game "in it's original form", not in "a compilation or any other kind of commercial package" without permission; no licence for the source code (`notes/legal.md`).
- `maped.txt`: by Johan Peitz, 2000-2002. `loader.txt`: by Eric Love. Rolling Hills maps: Johan Peitz (`rolling_hills.txt`).
- [hypothesis] PPCOL is compiled into happy: `0x00403E00` has the structure of the 8-bit case of its `check_pp_collision` (`src/happy/block_05.c`).
