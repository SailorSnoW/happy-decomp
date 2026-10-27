# Game code shared between `happy.exe`, `maped.exe` and `loader.exe`

Data: `notes/data/shared-functions.csv` (14 pairs). Script: `scripts/shared_code.py`, to be run with `.venv/Scripts/python`. Score details: `captures/shared-code.json`. Ghidra was not queried: the script rereads `game/*.exe` and the REST caches, through `inventory.py`.

## 1. Method

Only the GAME-class functions of the inventory CSVs are compared: 84 in happy, 50 in maped, 2 in loader. Every possible pair is scored, for happy×maped, happy×loader and maped×loader.

Traits extracted for each function, with the recursive disassembly of `inventory.py`:

| Trait | Contents |
|---|---|
| masked bytes | bytes of each instruction. Zeroed out: the `rel32` of direct `call`/`jmp`, as well as immediates and displacements within `[0x00400000, 0x00500000)`. |
| tokens | mnemonic + operands. Each address is replaced by what it designates: import (`imp:blit`), string (`str:"rb"`), CRT function (`crt:fopen`), game function (`game:<pair>`), `.rdata` float (`flt:0.0625`) or global (`glob`). An internal jump becomes `L`. |
| calls | ordered sequence of calls: imports (direct or through `call [IAT]`), CRT and game |
| contents | imports used, strings, non-trivial constants (excluding 0, 1, 2, 4, −1 and `esp` adjustments), floats read by an FPU instruction |

CRT functions receive the same label in all three exes as soon as their code is identical to happy's, which is the case for almost all of them (`layout-maped.md` §4). Their name is the Function ID name or the [hypothesis] name from the CSVs.

Scores: `difflib` similarity (without `autojunk`) of the lists of masked instructions (`s_bytes`), tokens (`s_tokens`), mnemonics (`s_mnemonics`) and calls (`s_calls`). A pair is kept if each of the two functions is the other's best candidate. In the second round, a call to an already-paired game function carries the pair's identifier. The computation converges in 2 rounds.

Confidence levels:

| Level | Rule | What the level establishes |
|---|---|---|
| **identical** (up to masking) | masked bytes identical over the whole function | **[verified]** same machine code, up to linking |
| **very close** | `s_bytes` ≥ 0.80, `s_calls` ≥ 0.8, size ratio ≥ 0.7 | — (no pair) |
| **similar** | `s_mnemonics` ≥ 0.55, `s_calls` ≥ 0.4, at least 2 specific pieces of evidence in common, at least one of them a "content" one (string, constant, float, non-trivial Allegro import, paired game call) | the measurements are [verified]; the common origin (source copied then modified) is a [hypothesis] |

CRT calls alone (`fopen`/`fwrite`/`fclose`, `malloc`/`free`) do not count as content evidence. They are idioms, and this rule rules out a false positive (§4). Ubiquitous imports (`screen`, `key`, `font`, `gfx_driver`, `textout`, `textout_centre`, `play_sample`, `blit`, `clear_bitmap`) do not count either.

Correction made along the way: a global array indexed by a register (`mov [esi+0x0044034B], dl`) was not masked in the tokens. The 8 identical pairs now have `s_tokens` = 1.00.

## 2. Result

14 pairs: **8 identical**, **0 very close**, **6 similar**. loader shares only the `WinMain` stub.

### 2.1 Identical [verified]

| happy | maped | loader | Size | happy / maped block | Evidence besides the bytes |
|---|---|---|---|---|---|
| `0x00401160` | `0x004016A0` | | 306 | block_01 / block_03 | calls `rand` [hypothesis]; constants `0x3`…`0x49`; writes to an indexed global array (`[esi+0x0044034B]` / `[esi+0x0042AB4B]`) |
| `0x004017C0` | `0x004017E0` | | 227 | block_01 / block_03 | the same help texts (`"Green shoes will make you jump really high!"`, etc.) |
| `0x0040A970` | `0x00405C70` | `0x004014A0` | 37 | block_08 / block_07 | Allegro `END_OF_MAIN` stub (header code, `allegro-header` subclass); calls `_WinMain` |
| `0x0040A9A0` | `0x00401000` | | 135 | block_09 / block_01 | map module |
| `0x0040AA30` | `0x00401090` | | 197 | block_09 / block_01 | map module; `malloc`/`free`; strings `"unnamed.dat"`, `"unnamed"`; constant `0x8C` (size of the map structure) |
| `0x0040AB00` | `0x00401260` | | 39 | block_09 / block_01 | map module; `free` |
| `0x0040AB30` | `0x00401290` | | 214 | block_09 / block_01 | map module; `fopen "rb"`, `fread`, `fclose`; constant `0x85` (size of the map header) |
| `0x0040AC10` | `0x00401370` | | 48 | block_09 / block_01 | map module; `load_datafile`, `unload_datafile` |

These 8 pairs are the ones the inventory had already found by exact search (`also_in` column, `layout-maped.md` §9). The present computation finds them independently. For each of them, the second-best candidate does not exceed 0.49, against at least 0.92 for the chosen pair.

### 2.2 Very close: none

No pair lies between identity and `s_bytes` ≈ 0.63. [hypothesis] The shared code is therefore either compiled as is on both sides (same source, same options) or thoroughly rewritten. No "slightly modified" version of a single function is observed.

### 2.3 Similar [measurements verified; common origin: hypothesis]

| happy | maped | Sizes | `s_bytes` / `s_mn` / `s_calls` | Evidence | Observed differences |
|---|---|---|---|---|---|
| `0x00401900` | `0x004018D0` | 102 / 215 | 0.30 / 0.56 / 0.60 | `create_sub_bitmap`, `destroy_bitmap`, `get_palette`, `save_bitmap`, `packfile_password("happy")` | maped builds the file name (`sprintf "maped%03d.%s"`, `get_config_string`). [hypothesis] screenshot. |
| `0x00401A30` | `0x00401B90` | 102 / 94 | 0.38 / 0.72 / 0.57 | `create_rgb_table`, `create_light_table`, `rgb_map`, `color_map` | happy also calls `create_trans_table` (with `0x80, 0x80, 0x80`), which maped does not call. maped first calls `set_palette`/`get_palette`, then pushes `0x3E, 0x3E, 0x3E`; [hypothesis] these are the r, g, b arguments of `create_light_table`. |
| `0x00401D60` | `0x00401BF0` | 64 / 36 | 0.63 / 0.57 / 1.00 | `load_datafile`; string `"data/misc.dat"` | happy loads `misc.dat` + `tiles.dat` and tests both results. maped loads `sprites.dat` + `misc.dat`, without testing. **Weakest evidence**: a single common string. |
| `0x00402120` | `0x00401D20` | 114 / 101 | 0.63 / 0.71 / 0.75 | `fade_out(0x20)`, `destroy_bitmap`, `unload_datafile`, `allegro_exit`; 1 paired game call | different freeing order; happy unloads 3 datafiles, maped 1. [hypothesis] program shutdown. |
| `0x00402EA0` | `0x00401400` | 393 / 210 | 0.56 / 0.68 / 0.67 | `__ftol`; float `0.0625` in `.rdata`; `set_palette` | happy additionally does `blit`s at `0x140`×`0xC8` (320×200), as well as an indirect call `call [ecx+0x9C]`, where `ecx` comes from `[bitmap+0x1C]` (vtable of a global `BITMAP`). [hypothesis] palette fade in steps of 1/16. |
| `0x00403070` | `0x00403200` | 181 / 189 | 0.54 / 0.72 / 0.91 | `fopen "rb"`, `fseek`, `fread`, `fclose`; constants `0x1C`, `0xAF0`, `0x1010` (end of the map file, `STATUS.md` "Layout of a `.map` file"); `lea [eax+eax*4+0x85]` (`0x85 + 5·W·H`) | maped keeps the file name in a global, compares a byte with `0x2A` (`'*'`), then counts entries (`inc [ecx+0x7C]`). happy initialises the 100 entries read. |

4 of these 6 pairs are the "analogous" functions of `layout-maped.md` §9. Two are new: `0x00401900`/`0x004018D0` and `0x00401D60`/`0x00401BF0`. The gap with the second-best candidate ranges from 0.19 (`0x00403070`/`0x00403200`: 0.70 against 0.51) to 0.42 (`0x00402120`/`0x00401D20`: 0.71 against 0.29).

### 2.4 Same role, below the threshold (not in the CSV)

These pairs have specific strings or imports in common, but their code is too different to reach the "similar" level. They count for the order (§3), with caution.

| happy | maped | `s_bytes` / `s_mn` / `s_calls` | Common points | [hypothesis] Role |
|---|---|---|---|---|
| `0x00401BE0` (377) | `0x00403A00` (592) | 0.30 / 0.42 / 0.40 | string `"Yes / No"`, `text_length`, `rect`, `textout_centre`, `key`, constant `0x64` | confirmation box. happy: fixed coordinates, translucent (`drawing_mode`). maped: coordinates computed with `SCREEN_W`/`SCREEN_H`. The pattern `gfx_driver ? gfx_driver->w : 0` reads `[ecx+0x68]` and `[ecx+0x6C]`, which are indeed `w` and `h` in `GFX_DRIVER` [verified in Allegro 4.0.3's `gfx.h`]. |
| `0x00401F00` (528) | `0x00401C20` (245) | 0.16 / 0.28 / 0.47 | `packfile_password("happy")`, `install_timer`, `install_keyboard`, `install_mouse`, `create_bitmap`, `text_mode`, constants `0x280`, `0x1E0` (640×480) | initialisation. happy also calls `install_allegro` and the audio there. maped calls `install_allegro` in `main` (`0x00405AE0`). |
| `0x00403170` (476) | `0x004032C0` (490) | 0.28 / 0.44 / 0.36 | strings `"data/sprites.dat"`, `"data/tiles.dat"`, `"data/xsprites.dat"`, `"data/xtiles.dat"`; `unload_datafile` then 2 × `load_datafile` | tile set switch (normal / "x"). maped goes through `file_select`/`alert`. |
| `0x0040A830` (313) | `0x00405AE0` (386) | 0.13 / 0.34 / 0.58 | position just before the `WinMain` stub | `main` [verified for both: passed to `_WinMain`, `layout-*.md` §3]. Unrelated code. |

### 2.5 Variants and mirrors specific to maped

- `0x00401160` (maped, 245 B, map module): its best candidate is happy `0x0040AA30` ("similar": `malloc`/`free`, constant `0x8C`, `s_calls` 0.86). That happy function is already the identical pair of maped `0x00401090`. [hypothesis] Variant of map creation (resizing?) removed from happy by `/OPT:REF`, as `layout-maped.md` §9 already suggested. → [verified 2026-10-27] It is a copy of a map (`map_copy`, `src/common/block_09.c`), not a resize.
- `0x004013A0` (maped, `"wb"`): its best candidate is happy `0x0040AB30` (map loading, `s_bytes` 0.51, constant `0x85`). [hypothesis] Map writing, mirror of the loading, in the same module.
- `0x004039B0` (maped, `"ab"`): writes `0xAF0` then `0x1010` bytes. It is the exact inverse of reading the end of the map (`0x00403200` in maped, `0x00403070` in happy) [verified by the constants]. happy has no equivalent.

## 3. Order of the shared functions

### 3.1 The two orders

Rank = position in happy. "≈" marks a same-role pair (§2.4).

| Rank | happy | Block | maped | Block | Level |
|---|---|---|---|---|---|
| 1 | `0x00401160` | 01 | `0x004016A0` | 03 | identical |
| 2 | `0x004017C0` | 01 | `0x004017E0` | 03 | identical |
| 3 | `0x00401900` | 02 | `0x004018D0` | 04 | similar |
| 4 | `0x00401A30` | 02 | `0x00401B90` | 04 | similar |
| 5 | `0x00401BE0` | 02 | `0x00403A00` | 06 | ≈ |
| 6 | `0x00401D60` | 02 | `0x00401BF0` | 04 | similar |
| 7 | `0x00401F00` | 02 | `0x00401C20` | 04 | ≈ |
| 8 | `0x00402120` | 02 | `0x00401D20` | 04 | similar |
| 9 | `0x00402EA0` | 03 | `0x00401400` | 02 | similar |
| 10 | `0x00403070` | 04 | `0x00403200` | 06 | similar |
| 11 | `0x00403170` | 04 | `0x004032C0` | 06 | ≈ |
| 12 | `0x0040A830` | 08 | `0x00405AE0` | 07 | ≈ (`main`) |
| 13 | `0x0040A970` | 08 | `0x00405C70` | 07 | identical (stub) |
| 14 | `0x0040A9A0`–`0x0040AC10` | 09 | `0x00401000`–`0x00401370` | 01 | 5 × identical |

In maped address order, the ranks come in this sequence: **14**, **9**, 1, 2, 3, 4, 6, 7, 8, 10, 11, **5**, 12, 13 [verified].

### 3.2 Answer: yes, with three exceptions

- **Same order** [verified]: removing 3 elements (the map module, the palette function `0x00402EA0`/`0x00401400` and the "Yes / No" box), the remaining 11 elements follow the same order in both exes:
  - utilities (1–2);
  - initialisation/shutdown group (3, 4, 6, 7, 8), in which 5 functions keep their relative order;
  - end of map and tile sets (10–11);
  - `main` (12);
  - stub (13).
  The computation restricted to the 14 pairs of the CSV gives a longest increasing subsequence of 8 out of 14. The 6 out-of-order elements are the 5 map module functions and the palette.
- **Exceptions** [verified]:
  - the map module is the first .obj of maped and the last game .obj in happy;
  - the palette comes right after the map module in maped (block_02), and after the initialisation group in happy (end of block_03);
  - "Yes / No" is in happy's initialisation group (block_02), and far behind in maped (block_06).
- **Contiguity** [verified]: the paired maped functions follow each other almost without gaps: block_03 has only the 2 utilities, and block_04 alternates pairs and maped-specific functions (`0x004019B0`, `0x00401A20`). In happy, they are separated by game-specific functions: `0x004012A0`–`0x004014F0` between the two utilities, `0x00401AA0`, `0x00401B20`, `0x00401DA0`–`0x00401EA0` in the initialisation group.

### 3.3 Interpretation [hypothesis]

- **Same .c files compiled in both projects**: map module (rank 14) and utilities (ranks 1–2).
  - Identical code and identical internal order. Each exe keeps an ordered subset of the same functions: maped additionally has `0x00401160` and `0x004013A0`, happy additionally has `0x0040AC40` and `0x0040AD20`.
  - This subset is explained by `/OPT:REF`. With VC6, `/O1` and `/O2` imply `/Gy` (one COMDAT per function), which allows unreferenced functions to be removed. This point comes from the VC6 documentation and was not verified here: it is part of the compilation experiment.
  - For the utilities, this assumes that happy block_01 (7 functions) is a single .c, of which maped keeps only 2 functions.
- **Same skeleton, rewritten code**: initialisation/shutdown group (ranks 3–8) and end of map (10–11). The functions are in the same order, but no pair exceeds `s_bytes` 0.63, and the constants, strings and calls differ. Two possible explanations, between which we cannot decide:
  - a file copied into the other project then adapted;
  - a shared file that evolved between the two links (maped on 2003-09-14, happy on 2003-10-27, `layout-maped.md` §4).
  With a shared file, these functions could not be compiled identically in the matching phase. Two distinct sources will be needed.
- **.obj order**: the common sequence utilities → initialisation → … → end of map → `main` suggests the same list of files in both projects, linked in the same relative order. The map module and the palette file are placed differently in it. The rule that sets the link order of a VC6 project (order of the `.dsp` or something else) is not known here.
- happy's blocks are confirmed as plausible boundaries:
  - happy block_01/block_02 ↔ maped block_03/block_04;
  - happy block_04 ↔ start of maped block_06;
  - happy block_03 (end) ↔ maped block_02.
  No pair crosses a block boundary in a way that would contradict the partition.

## 4. Rejected pairs

| happy | maped | Reason for rejection |
|---|---|---|
| `0x00401DA0` | `0x004039B0` | best mutual pair, `s_bytes` 0.61, but no content evidence. happy writes 5 blocks into `"happy.sav"` (`"wb"`), maped writes `0xAF0` + `0x1010` in `"ab"`. Only the `fopen`/`fwrite`/`fclose` idiom is common. |
| `0x004018F0` (12 B) | `0x00403140` (1 B, `ret`) | empty or trivial functions, with no content |
| `0x00409E40` | `0x00401D90` | `s_bytes` 0.28, no evidence |

## 5. loader

- [verified] Only the `END_OF_MAIN` stub (`0x004014A0`) is common to the three exes.
- [verified] loader's `main` (`0x00401000`, 1,172 B) resembles no function of happy (best score 0.21 with `0x00401F00`) or of maped (0.25 with `0x00405AE0`). The `happy.clv` format is shared with happy (`layout-loader.md` §3), but not the code that reads or writes it.

## 6. Limitations

- Only the GAME functions of the inventory are compared. A badly delimited function (truncated or merged body) would skew the scores.
- `difflib` scores favour short functions. The thresholds were chosen after manual review of the candidates, not calibrated against a known ground truth.
- The "similar" and "≈" pairs rest on the role (imports, strings) as much as on the code. Their common origin remains a hypothesis.
- The output `captures/shared-code.json` also lists the best pairs without a level, with their scores, for review.
