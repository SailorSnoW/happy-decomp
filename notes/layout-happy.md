# Layout of `happy.exe`

Inventory phase, Ghidra read-only (no renaming, typing, comments or function creation).

Data produced:
- `notes/data/functions-happy.csv`: one row per function (239). Columns: address, size, padding, Ghidra name and size, proposed name, block, class, status, callers, callees, Allegro and Win32 imports, CRT calls, strings, copies in `maped.exe`/`loader.exe`, reason.
- `notes/data/datarefs-happy.csv`: every `.rdata`/`.data` address referenced by the code, with its kind (string, memory, immediate) and the functions that reference it.
- `captures/ghidra-cache/happy_exe.json`: raw Ghidra responses.
- `captures/inventory-happy.json`: full graph.

Scripts (in `scripts/`):
- `ghidra_mcp.py`: minimal HTTP client for the MCP server. It refuses any tool that is not `list_*`, `get_*` or `search_*`.
- `ghidra_dump.py`: dumps, for each function, `get_function_info`, `get_disassembly`, `get_callers`, `get_callees` and `get_xrefs_to`.
- `inventory_happy.py`: analysis and classification. It runs with `.venv/Scripts/python` and depends on `capstone` 5.0.7, installed in `.venv/`.

## 1. Method

1. Entry points and names come from Ghidra (220 functions, 15 of them named by Function ID, plus `entry`).
2. [verified] **Ghidra missed 19 functions**: its analysis disassembled the code but did not create a function there (§7). The script adds them according to two rules:
   - a `.text` address that is not reached by the flow of any function, but is called, jumped to, or referenced by an immediate or by a pointer in `.data`/`.rdata`;
   - dead code, placed after a `ret` or padding, that decodes up to a `ret`/`jmp`.
3. Contiguous model: a function extends from its entry to the next entry, minus the trailing padding (`0x90`/`0xCC`). VC6 does not split functions into chunks. The `size` column follows this model; `ghidra_size` gives the body computed by Ghidra. Ghidra gives a shorter body for 8 functions. In particular, it does not count jump tables at the end of a function, SEH handlers (`0x0040B5F1`), or code following a non-returning call.
4. Recursive disassembly with capstone from the entry point, the targets of jump tables (`jmp [reg*4+T]`) and the filters and handlers of SEH scope tables.
5. Cross-check against Ghidra's `get_callers`/`get_callees`. Over the 220 common functions, there are 28 discrepancies, all explained by three causes:
   - functions missing from Ghidra: for example, `main` at `0x0040A830` calls 11 functions that Ghidra believes have no caller;
   - truncated bodies (SEH handler of the entry point, `0x0040EF60`);
   - indirect calls that Ghidra resolves through an initialised pointer: `call [0x00413220]` → `0x0040B35F` in `0x0040B1AF`, `[0x0041322C]` → `__exit`.
6. Cross-executable comparison. Each function of 16 bytes or more is searched for in `maped.exe` and `loader.exe`, with identical instruction bytes once `rel32` and absolute addresses are masked (column `also_in`).

## 2. Summary

| Class | Functions | of which [verified] | Size (bytes) |
|---|---:|---:|---:|
| GAME | 84 | 0 | 39,634 (incl. 51 of Allegro header code) |
| CRT | 154 | 16 | 24,444 |
| THUNK | 1 | 1 | 6 |
| UNKNOWN | 0 | – | 0 |
| Padding between functions | – | – | 737 |
| **Total** | **239** | | **64,821** = VirtualSize of `.text` (`0xFD35`) [verified] |

- **GAME** [hypothesis]: functions in the range `0x00401000`–`0x0040AD4F` reachable from `WinMain` (§3). None is recognised by Function ID.
  - The 72 imports from `alleg40.dll` are all used by GAME functions. No GAME function calls a Win32 import directly. [verified]
  - Subclass `allegro-header`: the `WinMain` stub (`0x0040A970`, 37 bytes) and `_set_errno_erange` (`0x00402110`, 14 bytes). This is Allegro header code, to be annotated as library code (`CLAUDE.md`).
- **CRT**:
  - [verified] 15 functions named by Function ID (`flsall`, `__exit`, `__ftol`, `_malloc`, `__nh_malloc`, `__freebuf`, `__fassign`, `__cfltcvt`, `__global_unwind2`, `__local_unwind2`, `_memset`, `_strlen`, `_strncpy`, `___add_12`, `__strcmpi`), plus the PE entry point `0x0040B506`.
  - [hypothesis] 138 others, all located after `0x0040AD50`. Reasons for their classification:
    - 26 reachable only from the PE entry point, not through `WinMain`;
    - 51 reachable only through data pointers (init and term tables, `_cfltcvt_tab`);
    - 56 reachable from `WinMain`, i.e. called by the game or by another CRT function;
    - 5 unreferenced, recognised by pattern (`__aulldiv`, `__aullrem`, `_abnormal_termination`, `_NLG_Notify1`, `_seh_longjmp_unwind`).
  - [verified] 142 of the 154 CRT functions exist identically (addresses masked) in `maped.exe` and/or `loader.exe`, 62 of them in both. `loader.exe` contains no game function in common with `happy.exe`: only the `WinMain` stub appears there.
- **THUNK** [verified]: `0x00410C10` = `jmp [IAT RtlUnwind]` (6 bytes), the function's only instruction.
- Direct calls from the game to the CRT [verified by the call sites]: `rand?` (21 functions), `fopen?` and `fclose?` (6), `fread?` (4), `fwrite?` (3), `exit?`, `__strcmpi`, `free?` (2), `_itoa?`, `__ftol`, `_malloc`, `0x0040B3D7` (1). Names followed by `?` are [hypotheses]; their justification is in the `proposed_name`/`reason` column of the CSV.
- [verified] `get_program_function_statistics` now counts 220 functions. The figure of 324 recorded in STATUS (environment check) does not reproduce.
- [verified] `Max Address: ffdfffff` comes from the `tdb` block (`0xFFDFF000`–`0xFFDFFFFF`), which Ghidra creates for `FS:` accesses (`list_memory_blocks`).

## 3. From the entry point to `main`

Call path, every step [verified] (disassembly, Ghidra imports):

1. `AddressOfEntryPoint` → `0x0040B506` (`entry`). This function sets up an SEH frame:
   - `push -1`;
   - `push 0x004111E8` (scope table);
   - `push 0x0040DF44` (`__except_handler3` [hypothesis], preceded by the `VC20XC00` signature at `0x0040DF3C`);
   - `FS:[0]`.
2. `0x0040B52C`: `call [GetVersion]`, results stored at `0x00413B48`–`0x00413B54`.
3. `0x0040B55D`–`0x0040B560`: `xor esi,esi`; `push esi`; `call 0x0040CCA2`. If the result is 0, `push 0x1C`; `call 0x0040B621`. 0x1C = 28, as in the message `"R6028\r\n- unable to initialize heap"` (`0x004112FC`). Hence `_heap_init(0)` [hypothesis].
4. `0x0040B575`: `call 0x0040C233` (`_ioinit` [hypothesis]); `0x0040B57A`: `GetCommandLineA` → `[0x00446C24]`; then `call 0x0040DD1A`, `0x0040DACD`, `0x0040DA14`, `0x0040B1AF`.
5. `0x0040B5A5`: `GetStartupInfoA`. `0x0040B5AB`: `call 0x0040D9BC` → `lpCmdLine` in `[ebp-0x64]`.
6. `0x0040B5B3`–`0x0040B5C2`: if `dwFlags & 1` (`STARTF_USESHOWWINDOW`), `nCmdShow = wShowWindow`, otherwise `push 0xA` (`SW_SHOWDEFAULT`).
7. `0x0040B5C2`–`0x0040B5C8`: `push eax` (`nCmdShow`), `push [ebp-0x64]` (`lpCmdLine`), `push esi` (0 = `hPrevInstance`), `push esi` then `call [GetModuleHandleA]` → `push eax` (`hInstance`).
8. **`0x0040B5CF`: `call 0x0040A970` = `WinMain(hInstance, NULL, lpCmdLine, nCmdShow)`**. The result goes to `0x0040B1DC` (`exit` [hypothesis], which calls `0x0040B1FE(code, 0, 0)`). The `__exit` path (Function ID, `0x0040B1ED`) starts from the SEH handler `0x0040B5F1`.

This path is the `WinMainCRTStartup` form (step 6, `SW_SHOWDEFAULT`), not `mainCRTStartup` [hypothesis based on steps 6 to 8].

**`WinMain` = `0x0040A970`** (37 bytes). It copies the 4 arguments, does `push 0x0040A830`, then `call [0x00411120]`. This IAT slot is `_WinMain`, ordinal 748 of `alleg40.dll` (`alleg40-imports.csv`). The function ends with `add esp,0x14`; `ret 0x10` (`__stdcall`, 4 arguments). This is exactly Allegro's `END_OF_MAIN()` (`alwin.h:44-48`, see `notes/allegro.md` §5.4).

**The game's `main` = `0x0040A830`** (313 bytes, `_mangled_main`), missing from Ghidra's list. Allegro calls it as `main(argc, argv)`: `mov eax,[esp+4]` (`argc`), `cmp eax,1`, then a loop over `argv[1..argc-1]` with `__strcmpi` (Function ID, `0x00410C20`). [verified]
- `"-widescreen"` (`0x00412F70`) → `[0x00412030] = 1`;
- `"-fps"` (`0x00412F68`) → `[0x00413B10] = 1`.

Then:
- `call 0x00401F00` (initialisation);
- loop on `0x00409870`, then depending on the case `0x00409F30`, `0x00406D60`/`0x00406BA0`, `0x00407B30`/`0x00407E80`, `0x00408500`, `0x00409640`, `0x00407500`;
- `call 0x00402120` (shutdown);
- `xor eax,eax`; `ret`.

## 4. CRT type: single-threaded (`LIBC.LIB`, `/ML`)

Conclusion: **[strong hypothesis] LIBC (single-threaded, static)**. Evidence:

1. [verified] **No TLS or synchronisation imports.** The 45 KERNEL32 imports (`list_imports`) include neither `TlsAlloc`/`TlsGetValue`/`TlsSetValue`/`TlsFree`, nor `InitializeCriticalSection`/`EnterCriticalSection`/`LeaveCriticalSection`/`DeleteCriticalSection`, nor `CreateThread`/`ExitThread`. Yet LIBCMT's `_mtinit` and `_lock` need them.
2. [verified] **No `FS:` access outside SEH.** There are only `FS:[0]` accesses, in `0x0040B506`, `0x0040DE8E`, `0x0040DEF6`, `0x0040FC24` and `0x0040FD6D`. No `FS:[0x2C]` or `FS:[0x18]`.
3. [verified] **No `lock`-prefixed instruction** in `.text`.
4. [verified] **No call between `_heap_init` and `_ioinit`**: the entry point goes straight from `0x0040B560` (`0x0040CCA2(0)`) to `0x0040B575` (`0x0040C233`). [hypothesis, based on VC6's `crt0.c` quoted from memory] The `_MT` version calls `_heap_init(1)` then `_mtinit()` at this point; the single-threaded version calls `_heap_init(0)`, as here.
5. [verified] **`errno` is a global variable**: `allegro_init()` pushes the immediate `0x00413B3C` (`&errno`) at `0x00401F0E`. [hypothesis] With LIBCMT, `errno` is `(*_errno())` and its address would be computed by a call.
6. Not discriminating: the strings `"R6016 … thread data"` and `"R6017 … multithread lock error"` (`0x00411474`, `0x00411444`) are present. [hypothesis] They are part of the message table shared by both variants.

## 5. Game / CRT boundary: `0x0040AD50`

- [verified] The 84 functions located before `0x0040AD50` all start at a multiple of 16, with `0x90` padding (0 to 15 bytes). After this address, entries are no longer aligned, except for some asm routines (`_memset` `0x0040E700`, `_strlen` `0x0040E760`), followed by `0xCC` padding.
- [verified] `0x0040AD50`: `holdrand = holdrand*0x343FD + 0x269EC3 ; return (holdrand>>16) & 0x7FFF` (`[0x00412F90]`). This is the Microsoft CRT `rand()` [hypothesis on the identification].
- [verified] No function located after `0x0040AD50` imports Allegro.
- [verified] In `.data`, the game's last string (`"unnamed"`, `0x00412F88`) is followed by `holdrand` (`0x00412F90`), the first datum referenced by the CRT.

## 6. Data layout (used to place the blocks)

Address ranges of `.data`/`.bss` (`datarefs-happy.csv`, column `classes`) [verified]:

| Range | Referenced by | Contents |
|---|---|---|
| `0x00412000`–`0x0041202F` | CRT | Pointer tables: `0x0040B0F6`, `0x0040B330`, `0x0040F934` (init), `0x0040B19B` (term). [hypothesis] `__xi_a…__xt_z`. |
| `0x00412030`–`0x0041208F` | game | Initialised globals: `[0x00412030]=0x20`, array of 20 pointers (`0x00412034`), `1`, `1`, `5`. |
| `0x00412094`–`0x0041221B` | game (via the array) | 20 strings pointed to by `0x00412034`, stored in the reverse order of the array. These are the passwords encrypted with a +1 shift: `"IBYF.GVMMDBQBDJUZ"` → `"HAXE-FULLCAPACITY"`. |
| `0x0041221C`–`0x00412F8F` | game | String literals of the functions, plus a few 1- to 3-character literals and a local array (`0x00412C5C`, used by `0x00409080`). |
| `0x00412F90`–`0x004139B0` | CRT | CRT `.data`. Last non-zero byte of `.data`: `0x00413B0B`. |
| `0x00413B10`–`0x00413B37` | game | 10 zeroed dwords. [hypothesis] `.bss` of the game's .obj files (statics or globals `= 0`). |
| `0x00413B38`–`0x00413D3B` | CRT | CRT `.bss` (`errno` at `0x00413B3C`). |
| `0x00413D3C`–`0x004468BF` | game | Large uninitialised arrays. [hypothesis] COMMON symbols (uninitialised C globals), allocated by the linker after the `.bss` sections. |
| `0x004468C0`–`0x00447C43` | CRT | CRT COMMON [hypothesis]. |

Observations on strings, which matter for the boundaries:
- [verified] From one function to the next, strings strictly follow the address order of the functions that use them first.
- [verified] Within a function, they are **in the reverse order of their use**. Examples: the intro story of `0x00406D60` starts with `"It's the night before Christmas..."`, stored last (`0x00412A04`); the ending dialogues of `0x00409080` are reversed in the same way.
- [verified] Identical strings are merged across the whole program, between distant functions. Examples: `"wb"` (`0x004124FC`), `"rb"` (`0x00412500`), `"%s"` (`0x00412644`), `"EXIT"` (`0x00412B38`), `"data/tiles.dat"`.
- [hypothesis] These are literals in `.data` pooled by `/Gf`: writable strings, unlike `/GF`. VC6 enables it with `/O1`/`/O2`.
- Consequence: the position of a shared string follows its first user. Strings therefore show no break from one .obj to the next.
- [verified] Anomaly: `"OK"` (`0x004124AC`) lies between the strings of `0x004017C0` and those of `0x00401900`, whereas only `0x00409F30` references it (question 5).

## 7. Functions missing from Ghidra (19)

| Address | Size | Class | Reason for adding | Role [hypothesis] |
|---|---:|---|---|---|
| `0x004018B0` | 59 | GAME | `push` at `0x00401F58` (argument of `install_int`) | timer callback |
| `0x004018F0` | 12 | GAME | `push` at `0x00401F6D` (`install_int`) | timer callback (`++[0x00442084]`) |
| `0x0040A830` | 313 | GAME | `push` at `0x0040A984` (argument of `_WinMain`) | the game's `main` |
| `0x0040B080` | 9 | CRT | `call` from `0x0040B19B` | `_flushall` = `flsall(1)` |
| `0x0040B0F6` | 165 | CRT | pointer `0x0041200C` | `__initstdio` |
| `0x0040B19B` | 20 | CRT | pointer `0x00412020` | `__endstdio` |
| `0x0040B31E` | 18 | CRT | `push` at `0x00401F09` (`allegro_init`) | `atexit` |
| `0x0040B330` | 47 | CRT | pointer `0x00412010` | `__onexitinit` |
| `0x0040B376` | 1 | CRT | pointers `0x00413224`, `0x00413228` | `ret` stub |
| `0x0040C45B` | 88 | CRT | `jmp` from `0x0040B1A9` | `_fcloseall` |
| `0x0040C6CF` | 78 | CRT | `mov` at `0x0040B37C` (`_fpmath` fills `_cfltcvt_tab`) | floating-point conversion |
| `0x0040C71D` | 24 | CRT | `mov` at `0x0040B39F` | floating-point conversion |
| `0x0040DE6C` | 34 | CRT | `push` at `0x0040DE98`, `cmp` at `0x0040DEFF` | SEH unwind handler |
| `0x0040DEF6` | 35 | CRT | dead code | `_abnormal_termination` |
| `0x0040DF19` | 9 | CRT | dead code | `_NLG_Notify1` |
| `0x0040DF44` | 189 | CRT | `push` at `0x0040B510`, `0x0040FC2E`, `0x0040FD77` | `_except_handler3` |
| `0x0040E7E0` | 104 | CRT | dead code | `__aulldiv` |
| `0x0040E850` | 117 | CRT | dead code | `__aullrem` |
| `0x0040F525` | 9 | CRT | 6 pointers starting at `0x00413250` | `_fptrap` (`_amsg_exit(2)` = R6002) |

## 8. Blocks (source file boundaries)

No `__FILE__` string, no `assert` message and no source file name appears in the strings [verified, `list_strings` and `datarefs-happy.csv`]. No block is therefore named.

Available clues, and what they are worth:
- **Alignment and padding** are useless in the game code: every function is 16-byte aligned with `0x90`, whether or not the .obj changes. They only mark the game/CRT boundary.
- Neither are **strings** (§6), because of pooling.
- **Initialised globals** are all at the start (`0x00412030`–`0x0041221B`), before all strings. This fits a single .obj, or a linker that would place string COMDATs after the other contributions. I could not decide (question 1).
- **Usable clues**:
  - small functions with a single caller placed just before their caller (probably `static` functions: a boundary cannot fall between them and the caller);
  - the theme (strings, Allegro imports);
  - two strong anchors: the `END_OF_MAIN` stub and the map module, which is also found in `maped.exe`.

Strength of each block's **start** boundary: **strong** (direct evidence), **medium**, **weak** (change of theme only).

| Block | Range | Funcs | Bytes | Probable theme | Start |
|---|---|---:|---:|---|---|
| block_01 | `0x00401000`–`0x004018AF` | 7 | 2,148 | shared utilities: tables, input, screen display, dialog box, help texts | strong (start of `.text`) |
| block_02 | `0x004018B0`–`0x0040219F` | 16 | 2,195 | initialisation and shutdown, data, options, small screens | weak |
| block_03 | `0x004021A0`–`0x0040302F` | 5 | 3,676 | level and HUD rendering, palette | weak |
| block_04 | `0x00403030`–`0x0040344F` | 5 | 1,030 | loading and resetting a level | weak |
| block_05 | `0x00403450`–`0x0040639F` | 19 | 11,970 | gameplay (loop, objects, events, pause, bonus room) | weak |
| block_06 | `0x004063A0`–`0x004074FF` | 8 | 4,389 | title screen, controls, options, introduction | weak |
| block_07 | `0x00407500`–`0x0040986F` | 12 | 8,977 | menus, passwords, scores, game over, game flow | weak |
| block_08 | `0x00409870`–`0x0040A99F` | 5 | 4,360 | start menu, custom game, `main`, `WinMain` | weak (end: strong) |
| block_09 | `0x0040A9A0`–`0x0040AD4F` | 7 | 889 | map module (shared with `maped.exe`) | **strong** |
| block_10_crt | `0x0040AD50`–`0x00410D34` | 155 | 24,450 | LIBC CRT (several dozen .obj files, not split) | **strong** |

### block_01 — `0x00401000`–`0x004018AF`
Functions:
- `0x00401000`: writes constants to 46 addresses in `0x00414044`–`0x0041412C`; called by `0x00401F00`.
- `0x00401160`: `rand`.
- `0x004012A0`: `poll_joystick`/`joy`.
- `0x00401350`: `key`.
- `0x00401380`: `blit`/`stretch_blit`/`vsync` to `screen`, depending on `[0x00412030]`; 25 callers.
- `0x004014F0`: text box (`rect`, `textout`, `text_length`).
- `0x004017C0`: copies 10 help texts (`"Green shoes will make you jump really high!"`…) into `0x00415CE0`–`0x00416BEF`.

Evidence:
- [verified] The game's first 10 strings (`0x0041221C`–`0x004124A8`) belong to `0x004017C0`.
- [verified] `0x00401160` and `0x004017C0` exist identically in `maped.exe` (`0x004016A0` and `0x004017E0`, adjacent), but their neighbours in `happy.exe` do not.
- [verified] `0x00401350` has only one caller, `0x004014F0`, two functions further on.
- [hypothesis] A file shared by the game and the editor, of which each exe keeps only what it uses (see question 4).

### block_02 — `0x004018B0`–`0x0040219F`
Functions:
- `0x004018B0` and `0x004018F0`: timer callbacks.
- `0x00401900`: screenshot (`create_sub_bitmap`, `get_palette`, `save_bitmap`, `packfile_password("happy")`); called by `0x00404C50`, which contains `"happy.pcx"`.
- `0x00401970`: `"Johan Peitz"` and two references into the middle of this string (+4, +8).
- `0x004019C0`.
- `0x00401A30`: `create_rgb_table`, `create_light_table`, `create_trans_table`.
- `0x00401AA0` and `0x00401B20`: messages.
- `0x00401BE0`: `"Yes / No"`.
- `0x00401D60`: `load_datafile("data/tiles.dat" / "data/misc.dat")`.
- `0x00401DA0` and `0x00401E10`: writing and reading `"happy.sav"`.
- `0x00401EA0`: `set_volume`.
- `0x00401F00`: initialisation (`install_allegro`, `install_*`, `"Failed to load data."`, inline `fixdiv`).
- `0x00402110`: `_set_errno_erange`.
- `0x00402120`: shutdown (`fade_out`, `unload_datafile`, `allegro_exit`).

Evidence:
- [verified] `0x004018B0`, `0x004018F0`, `0x00401D60`, `0x00401E10` and `0x00401EA0` have only one caller, `0x00401F00`, located 1 to 13 functions further on.
- [verified] `0x00401DA0` has only one caller, `0x00402120`.
- [verified] Contiguous strings `0x004124B0`–`0x00412504`.
- The `0x004018B0` boundary remains weak: `0x004017C0` has only one caller, `0x00401F00`, 14 functions away. It is justified only by the sharing with `maped`.

### block_03 — `0x004021A0`–`0x0040302F`
Functions:
- `0x004021A0`: 3 callers.
- `0x00402260` (`rand`) and `0x004024E0`: sole caller `0x00402620`, 1 to 2 functions further on [verified].
- `0x00402620` (2,166 bytes): `blit`, `stretch_sprite`, `textprintf`, `"%d"`, `_itoa`; 9 callers.
- `0x00402EA0`: `get_palette`/`set_palette`, `__ftol`, the game's only floating-point constant in `.rdata` (`0x004111E0`); calls `0x00401A30`.

### block_04 — `0x00403030`–`0x0040344F`
Functions:
- `0x00403030`: resets `0x0042F540`…; called by `0x00409640`.
- `0x00403070` (`fopen "rb"`, `fread`, `0x0040B3D7`, `fclose`) and `0x00403130`: sole caller `0x00403170`, which follows them [verified].
- `0x00403170`: loading of `data/tiles.dat`, `data/sprites.dat`, `data/xtiles.dat` and `data/xsprites.dat`, swap file `"hland.$$$"`, `"Swapfile could not be created…"`; calls the map module (`0x0040AB30`, `0x0040AC10`).
- `0x00403350`: called by `0x00403820` and `0x00405DB0`.

### block_05 — `0x00403450`–`0x0040639F`
Functions:
- `0x00403450`: inline `fixsin`/`fixcos`, `rand`; 6 callers.
- `0x004035B0`: `"Level exit is now open!"`.
- `0x004036C0`.
- `0x00403820`: level start (`fade_from`, `play_midi`, calls `0x00403170`).
- `0x00403A40`.
- `0x00403A80`: `"You found five magic spheres!"`, `"E X T R A   L I F E !"`.
- `0x00403CE0`, `0x00403DA0` and `0x00403E00`, then `0x00403F60`, which calls `0x004014F0` (dialog).
- `0x00404110` (`"All Happylanders have been saved!"`), then `0x004041F0`.
- `0x00404480`: `"TIME OUT!"`.
- `0x00404770`: `"You earned a password!"`.
- `0x00404920`: `"Level complete!"`.
- `0x00404A70`.
- `0x00404B80`: `"GAME PAUSED"`.
- `0x00404C50` (4,443 bytes): game loop (inline `fixmul`, `"Quit?"`, `"HURRY UP!"`, `"happy.pcx"`).
- `0x00405DB0`: bonus room (`"Here's a special treat!"`).

Evidence [verified]:
- `0x00403CE0`, `0x00403DA0` and `0x00403E00` have `0x00403F60` as their sole caller, which follows them;
- `0x00404110` has `0x004041F0` as its sole caller, which follows it;
- `0x00404A70` and `0x00404B80` have `0x00404C50` as their sole caller, 1 to 2 functions further on;
- `0x00404C50` calls 17 game functions, 9 of them in this block;
- the in-game messages are contiguous, `0x004125A4`–`0x004126D4`.

### block_06 — `0x004063A0`–`0x004074FF`
Functions:
- `0x004063A0`: `"v1.3"`, `"SpriteLib license number: …"`, `"written by Johan Peitz (c) 2000"`, `"* HAPPYLAND ADVENTURES *"`; called by 4 menus.
- `0x00406560`: controls (`"Select for"`, `talk`/`slide`/`jump`…).
- `0x00406850`: options (`"sound volume: %d"`, `"set controls"`).
- `0x00406B80` (19 bytes).
- `0x00406BA0`: `"Free Lunch Design"`, `"presents"`.
- `0x00406C80`.
- `0x00406CD0`.
- `0x00406D60`: introduction (`"Hi! It's Santa."`…).

Evidence [verified]: `0x00406560` has `0x00406850` as its sole caller, which follows it; `0x00406CD0` has `0x00406D60` as its sole caller, which follows it.

### block_07 — `0x00407500`–`0x0040986F`
Functions:
- `0x00407500`: `"HAPPYLAND INSTRUCTIONS"`.
- `0x00407B30` and `0x00407E80`: main menus (`"PLAY"`, `"OPTIONS"`, `"HELP ME"`, `"EXIT"`, plus `"PASSWORD"` in `0x00407E80`).
- `0x00408200`: `"You achieved a top score!"`.
- `0x00408500`: passwords and cheats (table `0x00412034`, `"HAXE-RESET"`, `__strcmpi`).
- `0x00408930`: `"Hall of Fame"`, `"%8ld"`.
- `0x00408A40`: `"GAME OVER"`.
- `0x00408B90`: `"You completed the game!"`.
- `0x00408E00`, `0x00409010`.
- `0x00409080`: ending dialogue.
- `0x00409640`: game flow (`"Continue?"`, calls `0x00404C50`, `0x00405DB0`, `0x00408A40`, `0x00408B90`, `0x00409080`).

Evidence [verified]:
- `0x00408E00` and `0x00409010` have `0x00409080` as their sole caller, which follows them;
- `0x00409080` has `0x00409640` as its sole caller, which follows it;
- `0x00408A40` and `0x00408B90` have `0x00409640` as their sole caller, 4 to 5 functions further on.

### block_08 — `0x00409870`–`0x0040A99F`
Functions:
- `0x00409870`: `"HAPPYLAND ADVENTURES"`, `"THE ORIGINAL GAME"`, `"CREATE CUSTOM GAME"`, `data/levels.dat`, `data/xlevels.dat`, `"WIDESCREEN IS %s (PRESS 'W' TO TOGGLE)"`, mouse.
- `0x00409E40`: `"RIGHT CLICK A SLOT"`; sole caller `0x00409F30`, which follows it [verified].
- `0x00409F30` (2,296 bytes): `file_select`, `"Add a level map. (*.MAP)"`, `"happy.clv"`, `"./maps/"`, `alert`.
- `0x0040A830`: `main`.
- `0x0040A970`: `WinMain` stub.

Evidence for the **end**:
- [verified in the Allegro sources] `END_OF_MAIN()` defines `WinMain` where the macro is written, right after `main`. `0x0040A830` and `0x0040A970` are therefore in the same file, in this order.
- The next block is the map module, which is another file (block_09).

### block_09 — `0x0040A9A0`–`0x0040AD4F` (map module)
Functions:
- `0x0040A9A0`: zeroes W·H·5 bytes at `[map+0x88]`.
- `0x0040AA30`: `malloc(0x8C)` and `malloc(W·H·5)`, default values, `"unnamed.dat"`/`"unnamed"`.
- `0x0040AB00`: release; 7 callers.
- `0x0040AB30`: `fopen "rb"`, `fread` of 0x85 bytes, W and H at bytes 1 and 2 of this header, as in `notes/game-docs.md`.
- `0x0040AC10`: `unload_datafile`/`load_datafile` into `[0x004450B4]`.
- `0x0040AC40` and `0x0040AD20`: reading a tile `((y>>4)·W + (x>>4))·5 + 2` (resp. `+4`).

Evidence:
- [verified] `0x0040A9A0`, `0x0040AA30`, `0x0040AB00`, `0x0040AB30` and `0x0040AC10` exist identically in `maped.exe`, in the same order, at `0x00401000`, `0x00401090`, `0x00401260`, `0x00401290` and `0x00401370`. This is the **start of `maped.exe`'s `.text`**, hence its first .obj. Between `create` and `destroy`, `maped.exe` contains one more function (`0x00401160`), absent from `happy.exe`.
- [hypothesis] A single source file compiled for both exes. The linker removed the unused functions from `happy.exe` (`/OPT:REF` with `/Gy`).
- [verified] These functions use no game global, except `0x004450B4`.
- [hypothesis] `0x0040AC40` and `0x0040AD20`, absent from `maped.exe`, belong to the same file: they are placed after `0x0040AC10` and before the CRT.

### block_10_crt — `0x0040AD50`–`0x00410D34`
Functions identified:
- [hypothesis] stdio: `rand`, `fclose`, `fwrite`, `fopen`, `fread`.
- `flsall` [verified], `_flushall`, `__initstdio`/`__endstdio`.
- `_cinit`, `exit`, `__exit` [verified], `doexit`, `atexit`/`_onexit`.
- `_fpmath`, `__ftol` [verified].
- `free`, `_malloc` [verified].
- `entry` `0x0040B506`, `_amsg_exit`/`fast_error_exit`.
- Heap management `0x0040CCA2`–`0x0040D81D` (`_heap_init` [hypothesis]).
- `__XcptFilter`, `_wincmdln`, `_setenvp`, `_setargv`.
- SEH `0x0040DE4C`–`0x0040E01B`.
- `_memset`, `_strlen` [verified], `__aulldiv`, `__aullrem`.
- `_strncpy`, `___add_12`, `__strcmpi` [verified], `_itoa`.

These functions are arranged in the order of the LIBC .obj files pulled in by the linker. They will not be decompiled and are not split further here.

## 9. The 10 most important open questions

1. **One .c file or several for `0x00401000`–`0x0040A99F`?** The static clues do not decide: globals all at the head of `.data`, pooled strings, the game's `.bss` in one piece. I propose an experiment with cl 12.00.8168 and link 8447: compile two or three test .c files (globals, literals, statics, non-inlined `__inline`), then observe the order in `.data`/`.bss` and the placement of the out-of-line copy. These clues would then become decisive.
2. **Exact compiler options.** Hypotheses: `/O2` or `/O1`; `/Gf` (strings in `.data`, pooled); `/Gy` and `/OPT:REF` (a `maped` function absent from `happy`); inlining enabled; no `DEBUGMODE`. To be confirmed by recompilation, since all matching depends on it.
3. **Order of the .obj files at link time.** The map module is the first .obj of `maped.exe` and the last of `happy.exe` (just before the CRT). The exact order of the other files depends on question 1.
4. **Code shared with `maped.exe`.** `0x00401160` and `0x004017C0` are adjacent in `maped.exe`, but separated in `happy.exe` by `0x004012A0`–`0x004014F0`. Is this a common file trimmed by `/OPT:REF`, or copy-paste? Do the same inventory on `maped.exe` and compare the orders.
5. **`"OK"` (`0x004124AC`)** lies between the strings of `0x004017C0` and `0x00401900`, but only `0x00409F30` references it. Did a fully inlined or removed function exist at this point in the source?
6. **Placement of `_set_errno_erange` (`0x00402110`)**, right after `0x00401F00`, its first user. Does VC6 emit the out-of-line copy of an `__inline` after the first function that uses it, or at the end of the obj? In the latter case, `0x00401F00` would be the last function of an .obj and `0x00402120` would start a new file.
7. **Nature of the game's two `.bss` areas** (`0x00413B10`–`0x00413B37`, then `0x00413D3C`–`0x004468BF`): statics or `= 0` on one side, COMMON on the other? This must be known to reproduce the addresses of the globals (`static` declarations, initialised or not).
8. **CRT identification.** Function ID recognised only 15 functions. Signatures from the LIBC.LIB matching compiler 8168 would move the 138 [hypothesis] CRT functions to [verified], along with the stdio names. We also need to know why `__aulldiv` and `__aullrem` are linked even though nothing references them.
9. **Functions missing from Ghidra** (19) and truncated bodies (8). Should they be created and fixed in Ghidra? That would be a write, forbidden during this phase without an explicit request; it must be decided before the decompilation phase, since reccmp and the annotations depend on it.
10. **Map module shared by `happy` and `maped`**: should a single source file be shared by both matching targets? This affects how the repository is organised for the next phase, and `maped` contains one more function.
