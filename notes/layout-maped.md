# Map of `maped.exe`

Inventory phase, Ghidra read-only (no renaming, no types, no comments, no function creation). Same method as for `happy.exe` (`notes/layout-happy.md` §1).

Data produced:
- `notes/data/functions-maped.csv`: one row per function (223), same columns as `functions-happy.csv`. The `also_in` column cites the copies in `happy.exe` and `loader.exe`.
- `notes/data/datarefs-maped.csv`: every `.rdata`/`.data` address referenced by the code.
- `captures/ghidra-cache/maped_exe_rest.json`: raw Ghidra responses (GhidraMCP 7.0.0).
- `captures/inventory-maped.json`: full graph, plus `same_as_happy` (identical function in happy) and `in_dll`.

Scripts (in `scripts/`):
- `ghidra_dump_rest.py`: JSON copy of the GhidraMCP 7.0.0 server, through `ghidra_rest.py` (`read_only` GETs only). Replaces `ghidra_dump.py`, which targeted the old server.
- `inventory.py`: generalisation of `inventory_happy.py` to the three exes (`.venv/Scripts/python scripts/inventory.py maped.exe`).
- `build_traits.py`: PE and Rich headers, CRT type tests, code-generation traits of the three exes (§4).

## 1. Method

Same steps as for happy (contiguous model, recursive disassembly, addition of missed functions, classification, cross-exe comparison). Regression check: **rerun on happy with the new cache, `inventory.py` reproduces `functions-happy.csv` and `datarefs-happy.csv`** [verified by comparison]. Only two cells differ: the Ghidra name of the thunk `0x00410C10` (`RtlUnwind` instead of `KERNEL32.DLL::RtlUnwind`, because of the server change) and the reason for that thunk, which now cites the target import.

Additions compared with the happy script:
1. **Ghidra cache**: the union of function bodies is computed as the complement of `find_code_gaps`, because `body_start`/`body_end` flatten bodies made of several chunks. The `ghidra_size` column takes the `size` field of `get_functions`, i.e. the size of the address set.
2. **Call chain**: `WinMain`, `main` and `_except_handler3` are found starting from the PE entry point, then compared with the expected values.
3. **CRT names carried over from happy**: a maped function with the same size and the same masked bytes as a happy CRT function receives its proposed name, still as [hypothesis], with the note "identical to happy.exe@…". This applies to 31 functions. `jmp [IAT]` thunks are excluded, since they all look alike once masked.
4. **Pointers stored in data**: a function in the game range that is not reachable from `WinMain` is classified GAME if a pointer to it lies in a data item referenced by a reached function. The data item extends from the referenced address up to the next one. This changes nothing for happy and classifies two maped functions (§7).
5. **Allegro copies**: every GAME function of 16 bytes or more is searched for in the `.text` of `alleg40.dll`. Result: **none** in the three exes [verified]. Positive control: the same search does find `fixmul`, `fixdiv`, `vsync` and `makecol` in the DLL.
6. **Ghidra cross-check**: the internal callees computed here are compared with those of `get_functions`.

## 2. Summary

| Class | Functions | of which [verified] | Size (bytes) |
|---|---:|---:|---:|
| GAME | 50 | 0 | 19,181 (of which 37 of Allegro header code) |
| CRT | 165 | 18 | 26,870 |
| THUNK | 8 | 8 | 48 |
| UNKNOWN | 0 | – | 0 |
| Padding between functions | – | – | 516 |
| **Total** | **223** | | **46,615** = VirtualSize of `.text` (`0xB617`) [verified] |

- **GAME** [hypothesis]: functions in the range `0x00401000`–`0x00405C9F`, reachable from `WinMain` (48 directly, 2 through a pointer stored in a `DIALOG` array, §7).
  - [verified] The 64 imports from `alleg40.dll` are all used by the game: 57 directly, 7 through the thunks. No GAME function calls a Win32 import directly.
  - `allegro-header` subclass: only the `WinMain` stub (`0x00405C70`, 37 B). No copy of `_set_errno_erange`: maped uses neither `fixmul` nor `fixdiv` inline (§4.4).
  - [verified] 8 GAME functions exist identically in happy (§9).
- **CRT**:
  - [verified] 17 functions named by Function ID, plus the PE entry point `0x004063A1`. Here Function ID recognises `__amsg_exit`, `__aulldiv` and `__aullrem`, which it did not recognise in happy. The code of these three functions is identical in both exes, which **confirms the names proposed as hypotheses for happy** (`0x0040B5FC`, `0x0040E7E0`, `0x0040E850`).
  - [hypothesis] 147 others, all located after `0x00405CCA`:
    - 28 reachable only from the PE entry point;
    - 52 reachable only through data pointers;
    - 63 reachable from `WinMain`;
    - 4 unreferenced, recognised by pattern.
  - [verified] 144 of the 165 CRT functions exist identically in happy and/or loader: 142 in happy, of which 62 are also in loader, and 2 in loader only (`_chdir?`, `toupper?`).
  - The other 21 have no copy found, for three reasons:
    - they are smaller than 16 bytes and are not compared (`_flushall?`, `__NLG_Notify1?`, `__fptrap?`, `atoi?`, the stub `0x00405FFF`…);
    - they contain jump tables (see the limitation below);
    - they really are specific to maped: the `printf` family (`sprintf?` `0x0040607D`, `_output?` `0x004081C3` and its helpers `0x00408904`–`0x004089BF`, `0x0040AC04`), `atol?` and `__allmul?`.
  - 42 CRT functions have a proposed name.
- **THUNK** [verified]: 7 Allegro import thunks at `0x00405CA0`–`0x00405CC9` (§5), plus `RtlUnwind` at `0x0040C510` (named by Ghidra).
- Direct calls from the game to the CRT [verified at the call sites]: `free?`, `_malloc`, `fopen?`, `fclose?`, `fread?`, `fwrite?`, `fseek?`, `rand?`, `__ftol`, `sprintf?`, `atoi?`, `_chdir?`.
- Limitation of the cross-exe comparison: asm functions that contain jump tables cannot be compared, because the tables are data and their addresses are not masked. Example: the 821 B functions `0x00407530`/`0x004098E0` (maped), `0x0040B9F0`/`0x0040F1F0` (happy), `0x00403160`/`0x00403AF0` (loader), with the same table structure. [hypothesis] These are `memcpy` and `memmove` (VC6 `memcpy.asm`).
- Ghidra cross-check: 5 functions whose callees differ, all explained by the causes already seen for happy.
  - Truncated bodies. Ghidra does not see:
    - `__exit` (`0x0040622F`), called by the SEH handler of the entry point;
    - the continuation of `0x0040A890` into `0x0040A8A0`.
  - Indirect calls that Ghidra resolves. Ghidra attributes:
    - `0x00405FE8` to `__cinit?` `0x004061F1` (pointer `_FPinit`);
    - `__fptrap?` `0x0040ABFB` to `_output?` (pointer from `_cfltcvt_tab`);
    - `__exit` to `__amsg_exit` (`0x00406497`).
- Ghidra gives a shorter body for 9 functions: `0x00402900` and `0x00404C10` (jump tables at the end of the function), the entry point `0x004063A1` (SEH handler), `0x00406497`, `0x004064BC`, `0x004081C3`, `0x00409652`, `0x0040A890` and `0x0040BA59`.

## 3. From the entry point to `main`

All points are [verified] (disassembly, imports). The structure is the same as in happy, apart from the addresses.

1. `AddressOfEntryPoint` → `0x004063A1` (`entry`). SEH frame: `push -1`, `push 0x0040D1D0` (scope table), `push 0x00409674` (`_except_handler3` [hypothesis], identical to happy `0x0040DF44`), `FS:[0]`.
2. `0x004063C7`: `call [GetVersion]`, results stored at `0x00410140`–`0x0041014C`.
3. `0x004063F8`: `xor esi,esi`; `push esi`; `call 0x004064E0` (`_heap_init(0)` [hypothesis]). On failure, `push 0x1C`; `call 0x004064BC` (`fast_error_exit` [hypothesis]).
4. Then:
   - `0x00406410`: `call 0x004093D1` (`_ioinit?`);
   - `GetCommandLineA` → `[0x0042C460]`;
   - `call 0x0040929F` → `[0x00410180]`;
   - `call 0x00409052` (`_setargv?`), `0x00408F99` (`_setenvp?`), `0x004061F1` (`_cinit?`).
5. `GetStartupInfoA`, then `call 0x00408F41` (`_wincmdln?`). `STARTF_USESHOWWINDOW` selects between `wShowWindow` and `push 0xA`.
6. **`0x0040646A`: `call 0x00405C70` = `WinMain(GetModuleHandleA(NULL), NULL, lpCmdLine, nCmdShow)`**. The result goes to `0x0040621E` (`exit?`).

**`WinMain` = `0x00405C70`** (37 B): `push 0x00405AE0`; `call [0x0040D108]` (`_WinMain`, ordinal 748); `add esp,0x14`; `ret 0x10`. These are the same bytes, once masked, as happy `0x0040A970` and loader `0x004014A0`. It is Allegro's `END_OF_MAIN()` stub.

**`main` = `0x00405AE0`** (386 B), missing from Ghidra's list:
- `0x00405AF6`: `install_allegro(ebx, 0x00410134, 0x00406360)`. `0x00410134` is `errno` (§4.3) and `0x00406360`, `atexit?`, is identical to happy `0x0040B31E`.
- `get_executable_name`, `replace_filename`, then `0x00405B31 call 0x0040C516` (`_chdir?`): the program changes to its own directory.
- `call 0x00401C20` (initialisation).
- `0x00405B3E`: `cmp [esp+0x810], 2` (`argc`). It then calls `0x00401290` (map loading, map module), `0x00401260` and `0x00403200`. [hypothesis] The map loaded is `argv[1]` when there is an argument.
- It chooses `"data/tiles.dat"` or `"data/xtiles.dat"`, and `"data/sprites.dat"` or `"data/xsprites.dat"`. It calls `0x00401370`, `load_datafile`, `0x00401B90`, `text_mode(-1)` and `0x00401400`.
- **`call 0x00404C10`** (editor loop), then `call 0x00401D20` (shutdown).

## 4. Compiler, linker, CRT

Source: `scripts/build_traits.py` (bytes of `game/*.exe`, `captures/inventory-*.json`). Full table for the three exes in `notes/layout-loader.md` §4.

### 4.1 PE header
- [verified] Linker 6.00, `OSVersion` and `SubsystemVersion` 4.0, subsystem 2 (GUI). Section and file alignment `0x1000`. Stack and heap: `0x100000` reserved and `0x1000` committed. `Characteristics` `0x10F`: relocations, line numbers and local symbols stripped, 32-bit exe. `CheckSum` 0.
- [verified] Data directories present: imports (1), resources (2) and IAT (12). There are no relocations and no debug directory, hence no link to a PDB.
- [verified] **`TimeDateStamp` = 2003-09-14 11:36:57 UTC**, about six weeks before happy (2003-10-27 18:16:21) and loader (2003-10-27 16:07:35).
- All these fields are identical in happy, apart from the sizes and the date.

### 4.2 Rich header

| prodid | Name [hypothesis, public comp.id tables] | build | maped | happy |
|---|---|---|---:|---:|
| 10 | Utc12_C | 8168 | 81 | 81 |
| 11 | Utc12_CPP | 8168 | 1 | 1 |
| 4 | Linker600 | 8447 | 3 | 3 |
| 14 | Masm613 | 7299 | 13 | 13 |
| 12 | AliasObj60 | 7291 | **1** | **2** |
| 19 | Linker512 | 8034 | 2 | 2 |
| 1 | Import0 | 0 | 117 | 120 |
| 6 | Cvtres500 | 1720 | 1 | 1 |

- [verified] **Same compiler and same linker as happy**: cl 12.00.8168, link 6.00.8447, masm 6.13.7299. No version difference. **Corrected on 2026-10-27**: the Linker600 8447 entry denotes Allegro's import library, not the game's linker, whose build is unknown (`notes/toolchain-vc6.md` §3).
- [verified] The only differences: `AliasObj60` (1 instead of 2) and `Import0` (117 instead of 120). [hypothesis] They come from the libraries pulled in (number of imports, CRT alias objects), not from the game's options.
- [hypothesis] The 81 C objects include those of the CRT. The number of game .obj files therefore cannot be read directly. With the member list of `LIBC.LIB` (VC6), the number of game .obj files could be derived for each exe (happy question 1).

### 4.3 CRT type: single-threaded (`LIBC.LIB`, `/ML`), like happy
**[strong hypothesis]**, with the same evidence as in `layout-happy.md` §4, all [verified]:
1. No TLS, critical-section or thread import among the 48 KERNEL32 imports.
2. No `FS:` access other than `FS:[0]` (12 occurrences, all SEH).
3. No `lock`-prefixed instruction.
4. `_heap_init(0)` (`0x004063F8`), with no intermediate call before `_ioinit`.
5. `&errno` passed as an immediate: `0x00410134` at `0x00405AF0`, an address read or written by 15 CRT functions.

[verified] KERNEL32 imports 3 more functions than in happy: `GetCurrentDirectoryA`, `SetCurrentDirectoryA` and `SetEnvironmentVariableA`, all used by `_chdir?` (`0x0040C516`). This is not a CRT difference, only additional library functions.

### 4.4 Compiler options: nothing distinguishes maped from happy
Traits of the GAME functions (excluding the `WinMain` stub):

| Trait | maped | happy | Reading |
|---|---|---|---|
| Entry points aligned on 16, `0x90` padding | 49/49 | 82/82 | identical |
| `push ebp ; mov ebp, esp` frame | 0 | 9 | see below |
| Inline `rep movsd`/`rep movsb`/`repne scasb`/`rep stosd` | 27/16/21/1 | 20/6/8/1 | `memcpy`/`strcpy`/`strlen`/`memset` intrinsics (`/Oi`) in both |
| String literals | all in `.data`, no duplicates | same | `/Gf` [hypothesis], like happy |
| `__ftol` calls | 3 | 3 | float → integer conversions |

- [verified] In happy, the 9 functions with an `ebp` frame are `0x00401F00`, `0x004021A0`, `0x00402260`, `0x00402620`, `0x00403350`, `0x00403F60`, `0x004041F0`, `0x00404480` and `0x00404C50`. Six of them bear a trace of the inline assembly from `al386vc.h` (nothing of the kind was found in the other three):
  - `0x00401F00` and `0x00404C50` call `_set_errno_erange` (`fixdiv`/`fixmul`);
  - `0x004021A0`, `0x00402260`, `0x004041F0` and `0x00404480` contain `mov eax,[ebp-x] ; sar eax,0x10`. [hypothesis] This is the body of the inline `fixfloor` (`mov eax, x ; sar eax, 0x10`, `al386vc.h:230-236`).
- [hypothesis] VC6 gives up frame-pointer omission (`/Oy`) in a function that contains inline assembly. The `ebp` frame therefore comes from Allegro's inline functions, not from a different option. maped uses none of them, hence 0 frames.
- Conclusion: **no observable difference in options between maped and happy** [hypothesis: same options, `/O2` or `/O1`, `/Gf`, `/Oi`, `/Oy`].

## 5. Game / library boundary: `0x00405CA0`

- [verified] The 50 GAME functions all start at an address that is a multiple of 16, with `0x90` padding. The `WinMain` stub is followed by 11 `0x90` bytes (`0x00405C95`–`0x00405C9F`).
- [verified] `0x00405CA0`–`0x00405CC9`: 7 `jmp [IAT]` thunks, contiguous and without padding, to `d_check_proc`, `d_radio_proc`, `d_edit_proc`, `d_text_proc`, `d_button_proc`, `d_ctext_proc` and `d_shadow_box_proc` (slots `0x0040D1B4`–`0x0040D19C`, `alleg40-imports.csv`). None of them is the target of a `call`. They are only referenced by pointers in `.data`, from `0x0040E2D8` to `0x0040EBE8`, spaced `0x38` bytes apart.
  - [verified in the sources] `0x38` = `sizeof(DIALOG)` (14 fields of 4 bytes, `include/allegro/gui.h:33-42`). These pointers are the `proc` field of the game's `DIALOG` arrays.
  - [hypothesis] In a static initialiser, the compiler cannot take the `__imp_` address of a `dllimport` function. It therefore references the import library stub, which the linker adds after the game's .obj files.
  - Happy has no thunks of this kind: it has no static `DIALOG` array.
- [verified] `0x00405CCA`: start of the CRT, unaligned: `free?`, identical to happy `0x0040B463`. No function located after `0x00405CA0` imports Allegro.
- [verified] The order of the CRT functions is **not** the same as in happy. Of the 142 common CRT functions, taken in maped order, the address in happy goes back down 18 times. For example, maped starts with `free?`/`_malloc` (happy `0x0040B463`/`0x0040B4D0`), then `fclose?` (happy `0x0040AD6E`). `_chdir?` comes last, after the `RtlUnwind` thunk. [hypothesis] The linker pulls the `LIBC.LIB` .obj files in the order in which it resolves the still-undefined symbols. That order therefore depends on the game's references. To reproduce the CRT layout, it will be enough to reproduce the game and the linker command line.

## 6. Data layout

`.rdata`/`.data` ranges (`datarefs-maped.csv`) [verified]:

| Range | Referenced by | Contents |
|---|---|---|
| `0x0040D1C8` (`.rdata`) | game (`0x00401400`) | floating-point constant. Equivalent of `0x004111E0` in happy, used by a palette function with `__ftol`. |
| `0x0040D1D0`–`0x0040DB81` (`.rdata`) | CRT | scope table of the entry point (`0x0040D1D0`), `R60xx` messages (`0x0040D2E8`–`0x0040D508`)… |
| `0x0040E000`–`0x0040E02F` | CRT | initialisation and termination pointer tables. [hypothesis] `__xi_a…__xt_z`, like happy `0x00412000`. |
| `0x0040E030`–`0x0040E04B` | **map module** | `"unnamed.dat"`, `"unnamed"`, `"rb"`, `"wb"`. |
| `0x0040E050`–`0x0040EF1F` | game | Initialised globals: `0x0040E050`/`0x0040E054` (`0x00402900`, `0x00404940`); `0x0040E058`, which is the operand of a `push` in `0x00404C10`, the function that calls `do_menu`, hence a `MENU` array [hypothesis]; `DIALOG` arrays, from the first `proc` field (`0x0040E2D8`) to the last known one (`0x0040ED38`), used by `0x00403150`, `0x004034B0` and `0x00404380`. |
| `0x0040EF20`–`0x0040F58F` | game | string literals, in the order of the functions that first use them: from `0x004017E0` (help texts) to `0x00404C10`. |
| `0x0040F598`–`0x0040FFB0` | CRT | CRT `.data`. |
| `0x00410110`–`0x0041012B` | game | 4 zero-initialised globals. [hypothesis] the game's `.bss`; same role as happy `0x00413B10`. |
| `0x00410130`–`0x00410324` | CRT | CRT `.bss` (`errno` at `0x00410134`). |
| `0x00410340`–`0x0042B0E3` | game | large uninitialised arrays. [hypothesis] COMMON. Contains `0x0042B0E0`, the map module's datafile, equivalent of happy `0x004450B4`. |
| `0x0042B0E4`–`0x0042C473` | CRT | CRT COMMON [hypothesis]. |

What maped contributes to happy question 1 (a single .c or several):
- [verified] The map module is the **first** .obj in `.text` (§8). Its strings are also the **first** game strings in `.data`, before the initialised globals of the rest of the game. The order of contributions to `.data` therefore follows the .obj order, and each .obj forms a contiguous block there.
- [verified] Within the second block, the initialised globals (`0x0040E050`…) precede the strings (`0x0040EF20`…), as in happy (`0x00412030`, then `0x0041221C`).
- [verified] `"rb"` (`0x0040E044`) is used both by the map module (`0x00401290`) and by `0x00403200`. It is stored with the map module's strings, its first user. Identical strings are therefore merged **across .obj files**, not only within a file.
- [hypothesis] If each .obj contributes "globals, then strings", a second game .obj with initialised globals would place them between two groups of strings. No such break is seen either in maped (game code after `0x00401400`) or in happy (before the map module, `0x00401000`–`0x0040A99F`). So there is a single .obj for all this code, or else only the first .obj contains initialised globals. The planned compilation experiment is still needed to decide.

## 7. Functions missing from Ghidra (23)

| Address | Size | Class | Reason for adding | Role [hypothesis] |
|---|---:|---|---|---|
| `0x004014E0` | 225 | GAME | pointers `0x0040EC50`, `0x0040EC88`… | `d_slider_proc` callback: it is stored at offset `0x30` (`dp2`) of the `DIALOG` entries; calls `vsync` and `0x00401400` |
| `0x00405AE0` | 386 | GAME | `push` at `0x00405C84` (argument to `_WinMain`) | `main` |
| `0x00405CA0`–`0x00405CC4` | 7 × 6 | THUNK | pointers in the `DIALOG` arrays | `d_*_proc` thunks (§5) |
| `0x00405FFF` | 1 | CRT | pointers `0x0040F59C`, `0x0040F5A0` | `ret` stub, same role as happy `0x0040B376` |
| `0x00406360` | 18 | CRT | `push` at `0x00405AE9` | `atexit` (identical to happy `0x0040B31E`) |
| `0x00406372` | 47 | CRT | pointer `0x0040E00C` | `__onexitinit` |
| `0x004071EB` | 9 | CRT | `call` from `0x00409F3B` | `_flushall` |
| `0x00407DE2` | 78 | CRT | `mov` at `0x00406005` | floating-point conversion (happy `0x0040C6CF`) |
| `0x00407E30` | 24 | CRT | `mov` at `0x00406028` | floating-point conversion (happy `0x0040C71D`) |
| `0x00408C40` | 52 | CRT | dead code | `__allmul`: 64-bit product, `ret 0x10` |
| `0x0040959C` | 34 | CRT | `push` at `0x004095C8`, `cmp` at `0x0040962F` | SEH unwind handler |
| `0x00409626` | 35 | CRT | dead code | `_abnormal_termination` |
| `0x00409649` | 9 | CRT | dead code | `_NLG_Notify1` |
| `0x00409674` | 189 | CRT | `push` at `0x004063AB`, `0x0040AD5F`, `0x0040B673` | `_except_handler3` |
| `0x00409E96` | 165 | CRT | pointer `0x0040E010` | `__initstdio` |
| `0x00409F3B` | 20 | CRT | pointer `0x0040E020` | `__endstdio` |
| `0x0040B4CB` | 88 | CRT | `jmp` from `0x00409F49` | `_fcloseall` |

In addition, `0x004015D0` (200 B) exists in Ghidra but is called by nobody. It is the `proc` field (offset 0) of entry `0x0040ED38` of a `DIALOG` array, and it calls `d_slider_proc`: it is a game dialog procedure [hypothesis]. `0x004014E0` and `0x004015D0` are classified GAME by the rule in §1, point 4, from the array `0x0040EB78`, which `0x00403150` (`do_dialog`) references. [verified in the sources] `dp2` of a `d_slider_proc` is a callback (`src/guiproc.c:1718`).

## 8. Blocks (source file boundaries)

Same clues and same strength scale as for happy (`layout-happy.md` §8). No source file name and no `__FILE__` in the strings [verified].

| Block | Range | Funcs | Bytes | Probable theme | Start |
|---|---|---:|---:|---|---|
| block_01 | `0x00401000`–`0x004013FF` | 7 | 967 | map module (shared with happy) | **strong** (start of `.text`) |
| block_02 | `0x00401400`–`0x0040169F` | 3 | 635 | palette or brightness, dialog slider | **strong** (end of the map module) |
| block_03 | `0x004016A0`–`0x004018CF` | 2 | 533 | 2 functions identical to happy block_01 | medium |
| block_04 | `0x004018D0`–`0x00401D8F` | 7 | 1,158 | screenshot, graphics mode, colour tables, data, initialisation, shutdown | weak |
| block_05 | `0x00401D90`–`0x004031FF` | 10 | 5,152 | map and interface display, image export, dialog | weak |
| block_06 | `0x00403200`–`0x00403EAF` | 9 | 3,172 | loading, properties, new map, messages, saving | weak |
| block_07 | `0x00403EB0`–`0x00405C9F` | 12 | 7,564 | editing tools, editor loop, `main`, `WinMain` | weak (end: strong) |
| block_08_thunks | `0x00405CA0`–`0x00405CC9` | 7 | 42 | Allegro import library thunks | **strong** |
| block_09_crt | `0x00405CCA`–`0x0040C616` | 166 | 26,876 | LIBC CRT and `RtlUnwind` thunk | **strong** |

Bytes: sum of the function sizes, without padding (same convention as happy). The 7 game blocks total 19,181 B.

### block_01 — `0x00401000`–`0x004013FF` (map module)
Functions:
- `0x00401000` = happy `0x0040A9A0` (resets the tiles);
- `0x00401090` = happy `0x0040AA30` (creation, `"unnamed.dat"`);
- `0x00401160` (not in happy; `malloc`/`free`; only caller `0x00403EB0`, an editing tool): resizing [hypothesis];
- `0x00401260` = happy `0x0040AB00` (freeing);
- `0x00401290` = happy `0x0040AB30` (loading, `"rb"`);
- `0x00401370` = happy `0x0040AC10` (datafile);
- `0x004013A0` (not in happy): `fopen(…, "wb")`, `fwrite`, `fclose`. Map writing [hypothesis].

Evidence:
- [verified] Five functions are identical to happy, in the same order.
- [verified] The block starts `.text`.
- [verified] The strings of `0x004013A0` (`"wb"`, `0x0040E048`) follow those of the module's other functions (`0x0040E030`–`0x0040E044`) and precede the globals of the next .obj (§6). `0x004013A0` therefore belongs to the map module, and the end of the block is strong.
- [hypothesis] Order in the source file: clear, create, resize, destroy, load, load the datafile, write, then happy's two tile reads (`0x0040AC40`, `0x0040AD20`). Each exe keeps only what it uses. This assumes `/OPT:REF` with function COMDATs (`/Gy`).

### block_02 — `0x00401400`–`0x0040169F`
Functions:
- `0x00401400`: `set_palette`, `__ftol`, floating-point constant `0x0040D1C8`; 5 callers;
- `0x004014E0`: slider callback (`vsync`, `0x00401400`);
- `0x004015D0`: dialog procedure that wraps `d_slider_proc` (`show_mouse`).

[hypothesis] These are the functions of a brightness setting: a slider in a `DIALOG` changes the palette. The start of the block is strong, since it follows the end of the map module. Nothing, however, separates this block from the next.

### block_03 — `0x004016A0`–`0x004018CF`
Functions:
- `0x004016A0` = happy `0x00401160` (`rand`);
- `0x004017E0` = happy `0x004017C0` (copy of the 10 help texts).

Evidence:
- [verified] Both functions are identical to happy. In maped they are adjacent; in happy, `0x004012A0`–`0x004014F0` separate them (joystick, `key`, screen display, text box), and maped uses none of those functions. With the block_01 rule (a copy reduced by `/OPT:REF`), this points to a file shared with happy block_01 [hypothesis, happy question 4].
- [verified] The strings of `0x004017E0` are the first strings of the second data block (`0x0040EF20`), as in happy (`0x0041221C`).
- Medium strength: sharing with happy is the only evidence, and it does not tell where the shared file ends.

### block_04 — `0x004018D0`–`0x00401D8F`
Functions:
- `0x004018D0`: screenshot (`"maped%03d.%s"`, `get_config_string("output_format")`, `packfile_password("happy")`, `save_bitmap`);
- `0x004019B0` (5 callers);
- `0x00401A20`: `set_gfx_mode`, `"run_in_a_window"`, `"autosave.map"`; calls the map writer `0x004013A0`;
- `0x00401B90`: `create_rgb_table`, `create_light_table`;
- `0x00401BF0`: `load_datafile("data/misc.dat" / "data/sprites.dat")`;
- `0x00401C20`: initialisation (`install_keyboard`/`mouse`/`timer`, `set_config_file("maped.ini")`, `"./maps/"`);
- `0x00401D20`: shutdown (`fade_out`, `unload_datafile`, `allegro_exit`).

Evidence:
- [verified] `0x00401BF0` has `0x00401C20` as its only caller, which follows it.
- [hypothesis] Same sequence as happy block_02: screenshot (happy `0x00401900`), colour tables (`0x00401A30`), data loading (`0x00401D60`), initialisation (`0x00401F00`), shutdown (`0x00402120`). The code is not identical, but the skeleton is the same (§9).

### block_05 — `0x00401D90`–`0x004031FF`
Functions:
- `0x00401D90` and `0x00401DF0` (`rect`);
- `0x00402030`: names of the tile types (`"SNOW"`, `"ICE"`, `"DOOR"`, `"EXIT"`…);
- `0x00402280` (`stretch_blit`);
- `0x00402350` and `0x00402370` (`textout`);
- `0x00402480`: export of the map as an image (`"%s saved"`, `show_*` options from `maped.ini`);
- `0x00402900`: main display (`"Save:%s        Name:%s       Time:%d"`, `"Happies: %d/%d"`);
- `0x00403140`: 1 byte, `ret`, empty function;
- `0x00403150`: `do_dialog`.

Evidence [verified]:
- `0x00401D90` has `0x00401DF0` as its only caller, which follows it;
- `0x00402350` has `0x00402370` as its only caller, which follows it;
- `0x00402280` has `0x00402900` as its only caller, 4 functions further on.

### block_06 — `0x00403200`–`0x00403EAF`
Functions:
- `0x00403200`: map loading (`"rb"`, `fread`, `fseek`); calls `0x004017E0` (default help texts [hypothesis]);
- `0x004032C0`: loading (`file_select`, `"Select a map to load. (*.MAP)"`, `"LOAD ERROR"`);
- `0x004034B0`: map properties (`popup_dialog`, `sprintf`, `atoi`);
- `0x00403740`: new map (`"./maps/unnamed.map"`);
- `0x00403800`: message box;
- `0x004039B0`: `fopen(…, "ab")`, `fwrite`;
- `0x00403A00`: `"Yes / No"`;
- `0x00403C50`: saving (`"Overwrite "`, `"Saving..."`);
- `0x00403E50`: `"Save as... (*.MAP)"`.

[verified] `0x004039B0` has `0x00403C50` as its only caller, two functions further on.

### block_07 — `0x00403EB0`–`0x00405C9F`
Functions:
- tools called by the loop: `0x00403EB0` (calls `0x00401160`), `0x00404000`, `0x00404280` (`key`), `0x004042B0` (`rand`), `0x004042D0`, `0x00404380` (text editing, `"(a-o)+ALT :  Edit textstring."`), `0x00404500`, `0x00404940` (`"Tile masks are ok!"`), `0x00404B00` (mouse);
- `0x00404C10` (3,792 B): editor loop (`do_menu`, `"Quit?"`, `"Really revert?"`, `"Make new map?"`, `"Screenshot taken!"`);
- `0x00405AE0`: `main`;
- `0x00405C70`: `WinMain` stub.

Evidence:
- [verified] The 9 functions from `0x00403EB0` to `0x00404B00` all have `0x00404C10` as their only caller, which follows them (+9 to +1).
- [verified] `0x00404C10` has `main` as its only caller, which follows it.
- [verified in the Allegro sources] `main` and `WinMain` are in the same file (`END_OF_MAIN`).
- The start of the block is weak: `0x004018D0`, `0x00402480`, `0x00402900`, `0x00403140`, `0x00403150`, `0x004032C0`, `0x00403740` and `0x00403E50` also have `0x00404C10` as their only caller, 10 to 35 functions further on. The chain of single-caller functions therefore cannot place a boundary before `0x00403EB0`.

## 9. Comparison with `happy.exe` (happy questions 3, 4 and 10)

Identical GAME functions (masked bytes) [verified]:

| maped | happy | Role |
|---|---|---|
| `0x00401000`, `0x00401090`, `0x00401260`, `0x00401290`, `0x00401370` | `0x0040A9A0`, `0x0040AA30`, `0x0040AB00`, `0x0040AB30`, `0x0040AC10` | map module |
| `0x004016A0`, `0x004017E0` | `0x00401160`, `0x004017C0` | shared utilities |
| `0x00405C70` | `0x0040A970` | `END_OF_MAIN` stub (also loader `0x004014A0`) |

Analogous, non-identical functions. Similarity of the mnemonic sequence (`difflib`) between 0.68 and 0.72, with the same imports [hypothesis: code copied then adapted]:

| maped | happy | Common points |
|---|---|---|
| `0x00401B90` | `0x00401A30` | `create_rgb_table`, `create_light_table` |
| `0x00401D20` | `0x00402120` | shutdown: `fade_out`, `unload_datafile`, `allegro_exit` |
| `0x00403200` | `0x00403070` | map loading: `fopen "rb"`, `fread`, `fseek`, `fclose` |
| `0x00401400` | `0x00402EA0` | palette, `__ftol`, floating-point constant in `.rdata` |

The rest of the GAME code resembles nothing in happy: at best 0.65, and only for short, trivial functions.

Provisional answers:
- **Q3 (.obj order)** [verified]: the map module is the first .obj of maped and the last game .obj in happy. The CRT comes last in both. In maped, the import thunk block is added between the game and the CRT.
- **Q4 (shared file or copy-paste)** [hypothesis]: both at once.
  - The map module and block_03 are shared sources, reduced by `/OPT:REF`. The code is identical and each exe keeps an ordered subset of the same functions.
  - Initialisation and shutdown (block_04) are a skeleton copied then modified: same function order, different code.
- **Q10 (shared map file for matching)**: a single source file must produce the map module of both exes. It contains at least 9 functions in this order: `clear`, `create`, `resize?`, `destroy`, `load`, `load_dat`, `save?`, then the two tile reads. [hypothesis] The position of the two tile reads (after `save?` or elsewhere) is not observable.

## 10. Open questions specific to maped

1. **A single .obj or several for `0x00401400`–`0x00405C9F`?** Same problem as happy question 1. The `.data` layout (§6) favours a single .obj after the map module, without proving it. The Rich header counts, compared with the list of `LIBC.LIB` members actually linked, could settle it (§4.2).
2. **End of the file shared with happy block_01.** Where in maped does the file that contains `0x004016A0`/`0x004017E0` end? Is block_02, specific to maped, at the start of that file?
3. **`0x00403140`**: empty function (`ret`) called by the editor loop. Is it leftover code, or a function whose body depends on a compiler option?
4. **`AliasObj60`: 1 in maped, 2 in happy.** Which library object accounts for this difference?
5. **Should the 23 missing functions be created in Ghidra and the 9 truncated bodies fixed?** Same decision as for happy: it is a write, to be authorised explicitly.
