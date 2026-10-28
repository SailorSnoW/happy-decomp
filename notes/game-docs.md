# Game documentation and contents of `game/`

Date: 2026-10-27. Sources: text files in `game/`, read without modification
(`readme.txt`, `loader.txt`, `maped.txt`, `maped.ini`, `maps/rolling_hills/rolling_hills.txt`, plus `happy.url`).
There are no other `*.txt` or `*.ini` files in `game/`.

Script used (read-only on `game/`): `scripts/inspect_formats.py` → `notes/data/datafiles.csv` (objects contained in each `.dat`) and unpacked copies in `captures/unpacked/`.

The string addresses below are virtual addresses computed from the PE section table (`.data` section, base `0x00400000`).

---

## 1. The game

- [verified] Title: "HAPPYLAND ADVENTURES — X-MAS EDiTiON", version **v1.3** (`readme.txt`, header).
- [verified] Genre, quoted verbatim: "a classic jump'n'run combined with original puzzle elements, a game full of surprises" (`readme.txt`, DESCRIPTION).
- [verified] Origin: written for an amateur programming contest on gamedev.net, which it won, with a "best use of SpriteLib" award. The Christmas edition is a sequel (`readme.txt`, GENERAL and FAQ).
- [verified] The game's launcher offers three choices: "Happyland Adventures - Xmas edition", "Happyland Adventures (the original)" or a custom game ("custom game") (`readme.txt`, INSTALLING AND STARTING).

### Goal
- [verified] In each level, you must save "Santa's helpers" (the Happylanders) by touching them so that they follow you, then bring them to a safe zone. When enough of them are saved, the exit opens. The number remaining is shown at the top right, under the time (`readme.txt`, PLAYING; `happy.exe` string at file offset `0x01242C`: "The main object of each level is to bring the inhabitants of Happyland to safety…").
- [verified] If you save *all* the helpers, the enemies turn into coins (`readme.txt`; string "If you save every single one of the Happylanders, all enemies will be turned into to golden coins!", `happy.exe` file offset `0x0122CC`).
- [verified] Each level has a time limit. Some levels have several exits, sometimes hidden. The old wise men ("Old wise men") give hints when you talk to them.
- [verified] 250 bonuses collected give an extra life. 5 "magic spheres" (blue balls) give a password if you finish the level. Collecting all the coins gives access to the bonus room.

### Pickups (`readme.txt`, PICKUPS)
| Item | Effect (quoted) |
|---|---|
| STAR | more points |
| DIAMONDS | even more points |
| HEART | increases your energy |
| TIMEGLASS | recieve extra time |
| COIN | collect all to access bonusroom |
| BLUE POTION | invunerability for a short while |
| LOW-G SHOES | jump higher and farther |
| BLUE SPHERES | find five and you get a password |

### Menus
- [verified] Title screen: `PLAY`, `PASSWORD`, `OPTIONS`, `HELP ME`, `EXIT`. You navigate with the arrow keys and confirm with Enter or Space. `PASSWORD` only appears in the Xmas edition (`readme.txt`, THE TITLESCREEN).
- [verified] Options: redefining keys and controls, sound and music settings. Up and Down to navigate, Left and Right to adjust (`readme.txt`).

### Default controls (`readme.txt`, CONTROLS)
| Action | Key |
|---|---|
| Left | LEFT ARROW |
| Right | RIGHT ARROW |
| Slide | DOWN ARROW |
| Jump | LEFT ALT |
| Talk | LEFT CTRL |
| Screenshot | F12 |
| Pause | PAUSE / ESC |
| Quit | ESC |

- [verified] Joysticks are supported: while configuring the controls, you press the desired button or direction.
- [verified] `W` key in the first menu (custom / normal / xmas choice): toggles full screen (`readme.txt`, FAQ). The string "WIDESCREEN IS %s (PRESS 'W' TO TOGGLE)" is in `happy.exe` at `0x00412E24`.

### Hardware requirements (`readme.txt`)
Pentium, 8 MB RAM, VGA, joystick (optional), SB-compatible sound card (optional).

---

## 2. Role of each executable (according to the documentation)

| Executable | Documented role | Source |
|---|---|---|
| `happy.exe` | The game: Xmas edition, original game and custom games | `readme.txt` |
| `maped.exe` | Map editor ("MAPEDITOR"), written by Johan Peitz | `maped.txt` |
| `loader.exe` | "CAMPAIGN LOADER", written by **Eric Love**. Automatically fills the map list of the "Create Custom Game" screen from a `.txt` campaign file | `loader.txt` |
| `unins000.exe` / `unins000.dat` | Uninstaller. [verified] `unins000.dat` starts with `Inno Setup Uninstall Log (b)` | file bytes |
| `happy.url` | Internet shortcut to `http://www.freelunchdesign.com` | file contents |

- [verified] `readme.txt` mentions `HAPPY_INSTALL.EXE` as the installer. This file is not in `game/`.
- [hypothesis] `loader.exe` passes the list to `happy.exe` through a `happy.clv` file. Justification: the string `happy.clv` exists in both programs (`loader.exe` `0x00406074`, `happy.exe` `0x00412F54`), and the loader's message is "%d normal map%s and %d bonus map%s loaded. Ready to play." (`0x00406030`). The format of `happy.clv` is not documented.

---

## 3. Command-line and configuration options

### Command line
The documentation describes **no options** (no `-xxx`). It only mentions two drag-and-drop uses, which amount to passing a file path as an argument:
- [verified] `loader.exe <campaign.txt>`: "in Windows you can drag a .txt file onto loader.exe or a shortcut to it" (`loader.txt`). Without an argument, the loader asks you to choose the file.
- [verified] `maped.exe <map.map>`: "drag and drop the .map file onto the editor executable. This will create an image that you can look at. See the file maped.ini for more details." (`maped.txt`, HINTS & TIPS).
- For `happy.exe`: nothing is documented.

### `maped.ini` (full contents, shipped values)
| Key | Value | Meaning (comment in the file) |
|---|---|---|
| `run_in_a_window` | `0` | full screen (0) or window (1) |
| `output_format` | `bmp` | format of exported images: `bmp`, `pcx` or `tga` |
| `show_background` | `1` | contents of the exported image (1 = yes, 0 = no) |
| `show_base_tiles` | `1` | same |
| `show_cover_tiles` | `1` | same |
| `hilite_cover_tiles` | `1` | same |
| `show_npcs` | `1` | same |
| `show_items` | `1` | same |
| `show_types` | `0` | same |
| `show_masks` | `0` | same |

Syntax [verified]: `key = value` lines, comments starting with `#`. The name `maped.ini` appears in `maped.exe` at `0x0040F244`.

### Game configuration
- [verified] Keys, sound and music are set in the OPTIONS menu. The documentation mentions no configuration file for `happy.exe`.
- [hypothesis] `happy.sav` (string at `0x004124F0` in `happy.exe`) stores the settings and/or the progress. This file is not present in `game/`.

---

## 4. File formats, dimensions, limits

### 4.1 What the documentation says

**`.map` maps** (`maped.txt`, THE MAP-FORMAT) [verified], six layers:
1. base tiles (ground and background scenery)
2. cover tiles (drawn over the characters; useful for effects and secret passages)
3. walkmask (how characters move through the tile)
4. items (bonuses and special objects)
5. npcs (happylanders, enemies, old wise men)
6. tile type (door, spikes, safespot, exit, ICE, SWITCH, SWAP…)

- [verified] "Maps can be in all sizes (except smaller than then screen)". Size set with INS or DEL + arrow.
- [verified] Tile types mentioned: spike, safespot, exit, DOOR (when it opens, the walkmask and cover layer of DOOR tiles are removed), ICE (slippery), SWITCH (trigger), SWAP (green blocks).
- [verified] The engine does not by itself place walls on the edges or spikes at the bottom: "The game engine does not fix this by itself".
- [verified] The old wise men have text that can be edited in the editor (`t` key).
- [verified] The original game's maps are compatible with the Xmas edition (`readme.txt`, FAQ).

**`.txt` campaign file** (`loader.txt`, THE FORMAT) [verified]:
- free text, then a line starting with `#`;
- the normal levels in order, one per line, **18 at most**;
- a `#` line;
- the bonus maps, **8 at most**;
- a `#` line, then free text.
- The `.txt` goes in the same directory as the `.map` files. Real example: `maps/rolling_hills/rolling_hills.txt` (3 levels, 2 bonus).
- If you save all the Happylanders and collect all the coins, you go to a randomly chosen bonus room.

**Screenshots** [verified]: `readme.txt` announces `HAPPY???.PCX`, "where ??? is the number of the screenshot taken". `happy.exe` only contains `happy.pcx` (`0x0041267C`). The numbered naming is not visible in that string; to be checked in the code. In `maped.exe`, the template is `maped%03d.%s` (`0x0040F1B8`).

**Editor** [verified]: automatic save `autosave.map` (`0x0040F1DC`), default map `./maps/unnamed.map` (`0x0040F490`).

### 4.2 Observed in the files

#### Datafiles `game/data/*.dat`
- [verified] The 7 files are **Allegro packfiles, LZSS-compressed and password-encrypted**. The password **`happy`** decrypts and decompresses them all into valid Allegro datafiles: `ALL.` signature, `prop`/`DATE`/`NAME`/`ORIG` properties, typed objects (`scripts/inspect_formats.py`).
- [verified] Two header variants (first 4 bytes, big-endian):
  - `0x031C0930` = `"slh!"` XOR mask(`happy`). Only the LZSS flag bytes are XORed with the password: `misc.dat`, `SPRITES.DAT`, `XSPRITES.DAT`, `tiles.DAT`, `xtiles.dat`.
  - `0x031C091A` = same value XOR `42`. The whole stream after the header is XORed with the password: `LEVELS.DAT`, `xlevels.dat`.
  - [hypothesis] These are Allegro's "old" and "new" packfile encryption schemes: the two level files would have been re-saved with a more recent version of the grabber.
- [verified] The string `happy` is in `.data` just before the datafile names, in `happy.exe` (`0x004124B0`, followed by `data/tiles.dat` `0x004124D0`, `data/misc.dat` `0x004124E0`…) and in `maped.exe` (`0x0040F1B0`).
- [hypothesis] The program passes it to `packfile_password()`. Not yet verified by xref in Ghidra.

| File | Size | Unpacked | Contents (Allegro types) | Loaded by |
|---|---|---|---|---|
| `LEVELS.DAT` | 45,528 | 212,055 | 14 `DATA` (maps): BONUS01–04, LEVEL00–06, SECRET00, TITLELEV, X_HELP | `happy.exe` (`data/levels.dat` `0x00412E00`) |
| `xlevels.dat` | 108,502 | 538,079 | 24 `DATA` (maps): BONUS01–04, LEVEL01, LEVEL11–14, LEVEL21–23, LEVEL2a, LEVEL31–34, LEVEL41–43, SECRET00, TITLELEV, X_HELP, X_OUTRO | `happy.exe` (`0x00412E10`) |
| `misc.dat` | 416,045 | 854,627 | 7 `BMP `, 6 `RLE `, 2 `FONT`, 14 `MIDI`, 9 `SAMP` | `happy.exe`, `maped.exe` |
| `SPRITES.DAT` | 16,801 | 70,221 | 152 `BMP ` | `happy.exe`, `maped.exe` |
| `XSPRITES.DAT` | 19,474 | 81,614 | 158 `BMP ` | `happy.exe`, `maped.exe` |
| `tiles.DAT` | 17,835 | 55,546 | 1 `PAL `, 130 `RLE ` | `happy.exe`, `maped.exe` |
| `xtiles.dat` | 18,540 | 55,337 | 1 `PAL `, 130 `RLE ` | `happy.exe`, `maped.exe` |

Each datafile also contains an `info` "GrabberInfo" object. Full list of objects: `notes/data/datafiles.csv`.

- [hypothesis] Prefix `x` = Xmas edition, no prefix = original game. Justification: the dates (`LEVELS.DAT` 2000, `xlevels.dat` 2002–2003), the names (`XSPRITES` contains `SANTA004`) and the presence of `X_OUTRO` only in `xlevels.dat`.

Observed dimensions (header of the bitmap objects: bpp, width, height) [verified]:
- all images are **8 bpp** (paletted);
- tiles: **16×16**, 130 per tileset (`TILE000…`);
- sprites: 16×16 (`HAPPY…`), 16×20 (`BAD…`), 16×23 (`GOOD008`), 64×53 (`SANTA004`, Xmas);
- backgrounds `BG1`–`BG3` and `FINAL`: **320×200** (64,006 bytes = 6 + 64,000).
- [hypothesis] The game runs at 320×200 ×256 colours (size of the backgrounds, VGA listed in the hardware requirements; "Possible to stretch the screen for higher resolutions", v1.3).

Origin of the resources (`ORIG` property) [verified]: paths `c:\djgpp\fld\happy\…` (dated 2000), `c:\program\micros~3\myproj~1\happy\…`, `c:\Documents and Settings\Johan\…\happy\maps\xmaps` (2002–2003).
- [hypothesis] A first version was developed under DJGPP (DOS), then the project moved to Visual C++. `micros~3\myproj~1` would be the short name of `Microsoft Visual Studio\MyProjects`, which is consistent with VC6.

#### `.map` maps
- [verified] Byte 0 = `0x02` in all maps (`.map` files and `DATA` objects of the datafiles). It is followed by a `\0`-terminated ASCII level name starting at byte 5 (`BONUSROOM!`, `Rumble in the Jungle`, `Stixland!`, `Winterland!`, `Rolling Hills 1`…).
- [verified by arithmetic] With W = byte 1 and H = byte 2, the size is always **`header + 5·W·H`**, with a fixed header of:
  - **7,045 bytes**: the 10 `.map` files in `maps/`, all of `xlevels.dat`, `LEVEL05` and `LEVEL06` of `LEVELS.DAT` (dated 2003);
  - **2,933 bytes**: the 12 other maps of `LEVELS.DAT` (dated 2000).
- [hypothesis] Byte 0 = format version, bytes 1 and 2 = width and height in tiles, bytes 3 and 4 = start position (`F5 set startpos` in `maped.txt`; the values remain in the map). There would be 5 bytes per tile for layers 1, 2, 3, 4 and 6. The NPCs (layer 5) and the wise men's texts would be in the fixed-size header, which would imply a **maximum number of NPCs**. The 4,112-byte difference between the two header sizes is unexplained, even though the version byte is `2` in both cases.
- [hypothesis] If W and H fit in one byte, a map is **at most 255×255** tiles. Largest maps observed: 171×32 (`winter.map`), 95×91 (`xlevels.dat` LEVEL33), 160×14 (`LEVELS.DAT` LEVEL00).

### 4.3 Numeric limits
| Limit | Value | Status / source |
|---|---|---|
| Normal levels per campaign | 18 | [verified] `loader.txt` |
| Bonus maps per campaign | 8 | [verified] `loader.txt` |
| Minimum map size | screen size | [verified] `maped.txt` |
| Extra life | 250 bonuses | [verified] `readme.txt` |
| Spheres for a password | 5 | [verified] `readme.txt` |
| Maximum map size | 255×255? | [hypothesis] bytes 1 and 2 |
| Max number of NPCs per map | unknown, bounded by the fixed header? | [hypothesis] |

---

## 5. Authors, dates, licence (quoted verbatim)

`readme.txt`:
> a game written by Johan Peitz
> brought to you by Free Lunch Design
> www.freelunchdesign.com
> (c) 2000 - 2003
> art by Johan Peitz and from Ari Feldman's SpriteLib
> v1.3

> This game is completely free. You may play it as much as you like without paying a single buck.

> **DISTRIBUTING**
> Happyland Adventures is freeware, anybody may (and should) play it. Therefore, you are encouraged to distribute this game (in it's original form) like crazy. Upload to every website and bbs you know, give a copy to all your friends, etc.. However, this game may not be included in a compilation or any other kind of commercial package without the permission from the author. See contact near the end of this document.

> Levels in the XMas Edition were made by Johan Peitz and Mark Hickford.

> Free Lunch Design is an independent game developer from Sweden.

> E-Mail: info (at) freelunchdesign (dot) com

> My SpriteLib license number is: 129.16.30.119-952518357

Acknowledgements (`readme.txt`): Ari Feldman (SpriteLib), Shawn Hargreavers (Allegro), Ivan Baldo (PPCOL), Mark Hickford, Arnon Yaari.
- [hypothesis] PPCOL is a pixel-perfect collision library for Allegro. It could be statically linked into `happy.exe`. To be looked for during the inventory.

`maped.txt`:
> written by Johan Peitz (c) 2000 - 2002

`loader.txt`:
> written by Eric Love
> The loader was created by me (Eric Love).
> theloves@chariot.net.au
> http://www.chariot.net.au/~theloves

`rolling_hills.txt`:
> The Rolling Hills maps were created by Johan Peitz.

The three `.txt` files contain the same disclaimer: "I do not accept responsibility for any effects, adverse or otherwise, that this software may have on you, your computer, your sanity, your dog, or anything else that you can think of. Use it at your own risk."

Note: the documentation gives **no licence for the source code**. Only redistribution of the game in its original form is permitted.

New in v1.3 (`readme.txt`): level and maped fixes, screenshots "now readable", screen stretching for high resolutions, path remembered in CUSTOM GAME, Rolling Hills levels added in `maps/`.

---

## 6. Contents of `game/data/` and `game/maps/`

| File | Size (bytes) | Ext. |
|---|---:|---|
| `data/LEVELS.DAT` | 45,528 | .dat |
| `data/misc.dat` | 416,045 | .dat |
| `data/SPRITES.DAT` | 16,801 | .dat |
| `data/tiles.DAT` | 17,835 | .dat |
| `data/xlevels.dat` | 108,502 | .dat |
| `data/XSPRITES.DAT` | 19,474 | .dat |
| `data/xtiles.dat` | 18,540 | .dat |
| `maps/examples/bonus_1.map` | 9,385 | .map |
| `maps/examples/bonus_2.map` | 8,345 | .map |
| `maps/examples/jungle.map` | 27,065 | .map |
| `maps/examples/stix.map` | 16,945 | .map |
| `maps/examples/winter.map` | 34,405 | .map |
| `maps/rolling_hills/rh_bonus_1.map` | 8,345 | .map |
| `maps/rolling_hills/rh_bonus_2.map` | 9,045 | .map |
| `maps/rolling_hills/rolling_hills.txt` | 413 | .txt |
| `maps/rolling_hills/rolling_hills_1.map` | 21,845 | .map |
| `maps/rolling_hills/rolling_hills_2.map` | 29,920 | .map |
| `maps/rolling_hills/rolling_hills_3.map` | 27,700 | .map |

Rest of `game/`: `alleg40.dll` (224,768), `happy.exe` (86,016), `maped.exe` (73,728), `loader.exe` (32,768), `readme.txt` (10,545), `loader.txt` (2,988), `maped.txt` (7,563), `maped.ini` (630), `happy.url` (56), `unins000.exe` (75,922), `unins000.dat` (5,638).

### First 32 bytes (representative examples)

`.dat`, old scheme:
```
misc.dat     03 1c 09 30 87 41 4c 4c 2e eb f0 27 70 72 de 6f 70 44 41 54 45 eb f0 10 8f 34 2d 31 32 2d 32 30  |...0.ALL...'pr.opDATE....4-12-20|
SPRITES.DAT  03 1c 09 30 87 41 4c 4c 2e eb f0 99 70 72 de 6f 70 44 41 54 45 eb f0 10 8f 33 2d 31 33 2d 32 30  |...0.ALL....pr.opDATE....3-13-20|
```
`.dat`, new scheme (fully encrypted stream):
```
LEVELS.DAT   03 1c 09 1a 96 29 2d 3c 5e 92 98 6e 00 02 c6 07 11 34 31 2d 2d 8a 80 60 86 5b 4c 41 44 54 5a 51  |.....)-<^..n.....41--..`.[LADTZQ|
xlevels.dat  03 1c 09 1a 96 29 2d 3c 5e 92 98 78 00 02 c6 07 11 34 31 2d 2d 8a 80 60 86 51 4c 40 49 54 5a 51  |.....)-<^..x.....41--..`.QL@ITZQ|
```
`.map`:
```
bonus_1.map          02 24 0d 01 03 42 4f 4e 55 53 52 4f 4f 4d 21 00 80 4d f4 43 e9 7b 01 00 00 8d 76 00 8a 45 f8 24  |.$...BONUSROOM!..M.C.{....v..E.$|
jungle.map           02 8f 1c 87 04 52 75 6d 62 6c 65 20 69 6e 20 74 68 65 20 4a 75 6e 67 6c 65 00 00 00 00 00 00 00  |.....Rumble in the Jungle.......|
rolling_hills_1.map  02 50 25 02 1c 52 6f 6c 6c 69 6e 67 20 48 69 6c 6c 73 20 31 00 00 00 00 00 00 00 00 00 00 00 00  |.P%..Rolling Hills 1............|
```
- [verified] In the bonus maps, the bytes following the name's `\0` (`80 4d f4 43 e9 7b 01 00 00 8d 76 00 8a 45 f8 24`) are identical from one map to the next.
- [hypothesis] These are leftovers of uninitialised memory: the name field would have a fixed size and be written in full with `fwrite`. These bytes look like x86 code.

`.txt`:
```
rolling_hills.txt    3a 3a 20 52 6f 6c 6c 69 6e 67 20 68 69 6c 6c 73 20 0d 0a 0d 0a 4a 75 73 74 20 64 72 61 67 20 61  |:: Rolling hills ....Just drag a|
```
(ASCII, CRLF line endings.)

---

## 7. Questions of the inventory, answered by the code

- `packfile_password("happy")`: [verified] set in `init_game` (`0x00401F00`) before the datafiles are loaded, cleared and set again around `save_screenshot` (`0x00401900`), `src/happy/block_02.c`.
- Layout of the map header: `MAP` and `TILE` in `src/include/map.h`; maped appends the objects and help texts after the tiles (`src/maped/`).
- `happy.sav`: `save_settings` / `load_settings` (`src/happy/block_02.c`, fields in `src/happy/data.c`); `happy.clv`: `notes/layout-loader.md` §3 and §10.
- PPCol in `happy.exe`: [hypothesis] `0x00403E00` has the structure of PPCol's `check_pp_collision` (`src/happy/block_05.c`).
