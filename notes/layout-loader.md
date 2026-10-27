# Layout of `loader.exe`

Inventory phase, Ghidra read-only (no renaming, typing, comments or function creation). Same method as for `happy.exe` (`notes/layout-happy.md` §1), with the additions described in `notes/layout-maped.md` §1.

Data produced:
- `notes/data/functions-loader.csv`: one row per function (70), same columns as `functions-happy.csv`.
- `notes/data/datarefs-loader.csv`: `.rdata`/`.data` addresses referenced by the code.
- `captures/ghidra-cache/loader_exe_rest.json` (raw Ghidra responses) and `captures/inventory-loader.json` (full graph).

Scripts: `scripts/ghidra_dump_rest.py`, `scripts/inventory.py loader.exe` and `scripts/build_traits.py` (see `layout-maped.md`).

## 1. Method, and what Ghidra missed

- [verified] Ghidra has **no function at all** in `0x00401000`–`0x0040149F`. `find_code_gaps` reports a 1,184-byte gap there "with orphan instructions". Yet this is all of the game code, that is, `main`. The script adds this function from the `push 0x00401000` in the `WinMain` stub (`0x004014B4`).
- Ghidra cross-check: 3 functions whose callees differ. The causes are the same as in happy:
  - Ghidra does not see `__exit` (`0x0040160C`) from the entry point's SEH handler;
  - it attributes `__exit` to `__amsg_exit?` (`0x004018A5`);
  - it does not see the fall-through from `0x004038F0` into `0x00403900` (happy `0x0040EF60` → `0x0040EF70`).
- Ghidra gives a shorter body for 5 functions: `0x004017AF` (entry point), `0x004018A5`, `0x004018CA`, `0x0040237E` and `0x004038F0`.

## 2. Summary

| Class | Functions | of which [verified] | Size (bytes) |
|---|---:|---:|---:|
| GAME | 2 | 0 | 1,209 (incl. 37 of Allegro header code) |
| CRT | 67 | 9 | 11,921 |
| THUNK | 1 | 1 | 6 |
| UNKNOWN | 0 | – | 0 |
| Padding between functions | – | – | 62 |
| **Total** | **70** | | **13,198** = VirtualSize of `.text` (`0x338E`) [verified] |

- **GAME**: `main` (`0x00401000`, 1,172 bytes) and the `WinMain` stub (`0x004014A0`, 37 bytes, subclass `allegro-header`). That is all.
  - [verified] The 18 imports from `alleg40.dll` are used by `main`, except `_WinMain`, which the stub uses.
  - [verified] No GAME function calls a Win32 import directly.
- **CRT**:
  - [verified] 8 functions named by Function ID (`_strncpy`, `__exit`, `_malloc`, `__nh_malloc`, `__global_unwind2`, `__local_unwind2`, `_strlen`, `_memset`), plus the PE entry point `0x004017AF`.
  - [hypothesis] 58 others:
    - 30 reachable only from the entry point;
    - 24 from `WinMain`;
    - 1 through a data pointer (`__onexitinit?`);
    - 3 unreferenced, recognised by pattern (`_abnormal_termination?`, `_NLG_Notify1?`, `__seh_longjmp_unwind@4?`).
  - [verified] 64 of the 67 CRT functions exist identically elsewhere:
    - 62 in happy **and** maped;
    - 2 in maped only: `_chdir?` `0x00404226` and `toupper?` `0x00404313`.
  - The other 3 are not comparable:
    - `_NLG_Notify1?` is shorter than 16 bytes;
    - `0x00403160` and `0x00403AF0` (821 bytes) contain jump tables; [hypothesis] they are `memcpy`/`memmove`, also present in the other two exes.
  - 23 CRT functions have a proposed name, 20 of them carried over from happy by code identity.
- **THUNK** [verified]: `0x00404220` = `jmp [IAT RtlUnwind]`.
- [verified] **Neither stdio nor floating point**: no `fopen`, `fread`, `__ftol`, `_fpmath` or `_cfltcvt`, and happy's KERNEL32 file functions are absent (`CreateFileA`, `ReadFile`, `SetFilePointer`, `CloseHandle`…). `main` reads and writes its files through Allegro packfiles.
- Direct calls from the game to the CRT [verified]: `__chkstk?` (`0x00401005`), `_strncpy` (`0x0040105C`) and `_chdir?` (`0x0040106E`).

## 3. From the entry point to `main`

[verified] The structure is the same as in happy and maped:
1. Entry point `0x004017AF`. SEH: `push 0x004050F8` (scope table), `push 0x004023A0` (`_except_handler3?`).
2. `GetVersion` → `0x00406718`–`0x00406724`.
3. `0x00401806`: `xor esi,esi ; push esi ; call 0x0040226A` (`_heap_init(0)?`). On failure, `push 0x1C ; call 0x004018CA`.
4. Then:
   - `0x004020BF` (`_ioinit?`);
   - `GetCommandLineA` → `[0x00406C24]`;
   - `0x00401F8D` → `[0x00406758]`;
   - `0x00401D40` (`_setargv?`), `0x00401C87` (`_setenvp?`), `0x004015CE` (`_cinit?`);
   - `GetStartupInfoA`, `0x00401C2F` (`_wincmdln?`).
5. **`0x00401878`: `call 0x004014A0` (`WinMain`)**. The result goes to `0x004015FB` (`exit?`).

**`WinMain` = `0x004014A0`**: `push 0x00401000`; `call [0x004050A8]` (`_WinMain`); `ret 0x10`. These are the same masked bytes as happy `0x0040A970` and maped `0x00405C70`.

**`main` = `0x00401000`** [verified, disassembly]:
- `mov eax, 0x2810 ; call 0x00401780` (`__chkstk?`): 10,256 bytes of local variables, hence no globals;
- `install_allegro(0, 0x0040670C, 0x0040173D)` (`errno`, `atexit?`), then `install_keyboard`, `install_mouse`;
- choice of the map list:
  - if `argc > 1` (`0x0040103A`), `_strncpy(buffer, argv[1], 0x400)`;
  - otherwise, `_chdir("maps")` (`0x00401069`), then `file_select_ex("Select set of levels to load", …, "txt")`;
- reading the list: `pack_fopen(…, "r")` (`"r"` at `0x00406108`), `pack_fgets` in a loop, `exists`. Error messages: `"Could not open file \"%s\"\n"`, `"Only 18 regular maps allowed\n"`, `"Only 8 bonus maps allowed\n"`, `"Unexpected end of file\n"`, `"Can't find file \"%s\"\n"`, displayed by `allegro_message`. The error path returns 3 (`0x004013C3`);
- **writing `happy.clv`** (`0x004013D3`–`0x0040144E`), next to the exe (`get_executable_name` + `replace_filename`):
  - `pack_fopen(…, "w")` (`"w"` at `0x00406070`);
  - `pack_fwrite(buffer, 0x2206, f)`;
  - `pack_iputw(num_normal, f)`; `pack_iputw(num_bonus, f)`;
  - `pack_fclose`;
- `allegro_message("%d normal map%s and %d bonus map%s loaded. Ready to play.\n", …)` with `""` or `"s"` depending on whether the count is 1; returns 0.

### Format of `happy.clv` [verified from the code of both exes]

| Offset | Size | Contents | Written by loader | Read and written by happy |
|---|---:|---|---|---|
| `0x0000` | `0x178E` (6,030) | normal maps | `pack_fwrite`, 1st part of the `0x2206` block | `fread` into `0x004404E0` (`0x00409F88`), `fwrite` (`0x0040A7BB`) |
| `0x178E` | `0x0A78` (2,680) | bonus maps | 2nd part of the same block | `0x0043F5C0` (`0x00409F9A`, `0x0040A7CD`) |
| `0x2206` | 2 | number of normal maps (16-bit word, little-endian) | 1st `pack_iputw` (`[esp+0x44]`) | `0x004450A8` (`0x00409FA9`, `0x0040A7DC`) |
| `0x2208` | 2 | number of bonus maps | 2nd `pack_iputw` (`edi`) | `0x0042F568` (`0x0040A7EE`) |

- Total: 8,714 bytes.
- [verified by arithmetic] `0x178E + 0xA78 = 0x2206`. Moreover, 6,030 = 18 × 335 and 2,680 = 8 × 335.
- [strong hypothesis] These are 18 normal map slots and 8 bonus map slots, 335 bytes each, the same limits as in loader's messages. The contents of a slot remain to be read.
- [verified] loader does not call `packfile_password`: it is not one of its 18 imports. The file is therefore written in the clear, and happy reads it back with `fopen("happy.clv", "rb")` (`0x00409F6D`).

## 4. Compiler, linker, CRT (three exes)

Source: `scripts/build_traits.py` [verified for the values; the comp.id names are hypotheses taken from public tables].

### 4.1 PE header

| Field | happy.exe | maped.exe | loader.exe |
|---|---|---|---|
| TimeDateStamp (UTC) | 2003-10-27 18:16:21 | **2003-09-14 11:36:57** | 2003-10-27 16:07:35 |
| Linker version | 6.00 | 6.00 | 6.00 |
| OS / subsystem | 4.0 / 4.0, GUI (2) | same | same |
| SectionAlignment / FileAlignment | `0x1000` / `0x1000` | same | same |
| Stack reserve/commit, heap reserve/commit | `0x100000`/`0x1000`, `0x100000`/`0x1000` | same | same |
| Characteristics | `0x10F` | `0x10F` | `0x10F` |
| CheckSum, DllCharacteristics | 0, 0 | 0, 0 | 0, 0 |
| Directories present | imports, resources, IAT | same | same |
| `.text` / `.rdata` / `.data` (VirtualSize) | `0xFD35` / `0xB64` / `0x35C44` | `0xB617` / `0xB82` / `0x1E474` | `0x338E` / `0x87C` / `0xC3C` |

- [verified] No relocations (`0x10F` includes `RELOCS_STRIPPED`). No debug directory, hence neither `/DEBUG` nor a PDB. `CheckSum` 0, hence no `/RELEASE`. These are link 6.00's defaults for an exe.
- [verified] loader was linked about 2 h 09 before happy, on the same day. maped dates from six weeks earlier.

### 4.2 Rich header

| prodid | Name [hypothesis] | build | happy | maped | loader |
|---|---|---|---:|---:|---:|
| 10 | Utc12_C | 8168 | 81 | 81 | 29 |
| 11 | Utc12_CPP | 8168 | 1 | 1 | 1 |
| 4 | Linker600 | 8447 | 3 | 3 | 3 |
| 14 | Masm613 | 7299 | 13 | 13 | 9 |
| 12 | AliasObj60 | 7291 | 2 | 1 | 1 |
| 19 | Linker512 | 8034 | 2 | 2 | 2 |
| 1 | Import0 | 0 | 120 | 117 | 60 |
| 6 | Cvtres500 | 1720 | 1 | 1 | 1 |

- [verified] **Same builds in all three exes**: cl 12.00.8168 (C and C++), link 6.00.8447, masm 6.13.7299, cvtres 5.00.1720. **No version difference** between loader, maped and happy. **Corrected on 2026-10-27**: the Linker600 8447 entry denotes Allegro's import library, not the game's linker, whose build is unknown (`notes/toolchain-vc6.md` §3).
- [verified] Only the object counts vary. [hypothesis] They depend on the library members pulled in: loader links less CRT, with neither stdio nor floating point.
- [hypothesis] Useful calibration for happy's question 1. loader's game fits in a single .obj (`main` + `WinMain`, §8). Its 29 Utc12_C objects would therefore be 1 game object and 28 C objects from `LIBC.LIB`. If the list of `LIBC.LIB` members actually linked is established for happy and maped, `81 − (CRT C objects)` will give the number of game .obj files.
- [hypothesis] The single Utc12_CPP object, present in all three exes, would be a C++ module of the CRT (the `new` handler, called by `_nh_malloc`). The game code would then be entirely in C.

### 4.3 CRT type: single-threaded `LIBC.LIB` (`/ML`) for all three exes

| Evidence (`layout-happy.md` §4) | happy | maped | loader |
|---|---|---|---|
| TLS, critical section, thread imports | none | none | none |
| `FS:` accesses | `FS:[0]` only (12) | same (12) | same (12) |
| `lock` prefixes | 0 | 0 | 0 |
| `_heap_init` in the entry point | `0x0040B55D`: `xor esi,esi ; push esi ; call 0x0040CCA2` | `0x004063F8`: `… call 0x004064E0` | `0x00401806`: `… call 0x0040226A` |
| `&errno` passed as an immediate | `0x00413B3C` (`0x00401F0E`) | `0x00410134` (`0x00405AF0`) | `0x0040670C` (`0x00401017`) |

**[strong hypothesis] Static single-threaded LIBC in all three exes**, and it is the same library: 62 CRT functions are identical in all three exes [verified].

### 4.4 Compiler options: no observable difference

| Trait of GAME functions (excluding the stub) | happy | maped | loader |
|---|---|---|---|
| Entries 16-byte aligned, `0x90` padding | 82/82 | 49/49 | 1/1 |
| `ebp` frame | 9 (Allegro inline asm, see `layout-maped.md` §4.4) | 0 | 0 |
| Inline `rep movsd` / `rep movsb` / `repne scasb` | 20 / 6 / 8 | 27 / 16 / 21 | 4 / 4 / 4 |
| String literals | `.data`, no duplicates | same | same |
| `__chkstk` stack probe | – | – | 1 (`0x2810`-byte frame) |

- [hypothesis] These traits correspond to a single set of options for all three exes:
  - `/O2` or `/O1` optimisation, with `/Oy` (no frame, except in functions containing inline asm) and `/Oi` (inline string copies and lengths);
  - `/Gf` (strings in `.data`, merged);
  - `/Gy` + `/OPT:REF` (`layout-maped.md` §8, block_01).
- `__chkstk` is not an option: VC6 calls it for any frame larger than 4 KB.
- **No version or option difference is observed** between the three exes. The only header difference is the date.

## 5. Game / CRT boundary: `0x004014D0`

- [verified] `main` (`0x00401000`) and the stub (`0x004014A0`) start on a 16-byte boundary, followed by `0x90` padding (12 and 11 bytes).
- [verified] `0x004014D0` = `_strncpy` (Function ID): this is the first CRT function. After it, entries are no longer aligned (`0x004015CE`, `0x004015FB`…), except the asm routines (`_strlen` `0x004039E0`, `_memset` `0x004041C0`).
- [verified] No function located after `0x004014D0` imports Allegro.
- [verified] The CRT order differs from happy's: sorted in loader's order, the 62 common functions see their happy address go back down 12 times. Same presumed cause as for maped (`layout-maped.md` §5): the symbol resolution order.

## 6. Data layout

| Range | Referenced by | Contents [verified] |
|---|---|---|
| `0x004050F8`–`0x00405450` (`.rdata`) | CRT | scope table of the entry point, etc. No game data in `.rdata`. |
| `0x00406000`–`0x0040602F` | CRT | init and term pointer tables (like happy `0x00412000`, maped `0x0040E000`). |
| `0x00406030`–`0x00406133` | game | the strings of `main`, from `"%d normal map%s and %d bonus map%s loaded. Ready to play.\n"` to `"maps"`, with `"s"` (`0x0040606C`), `"w"` (`0x00406070`) and `"r"` (`0x00406108`). |
| `0x00406140`–`0x00406707` | CRT | CRT `.data`. Up to `0x00406707`: table of 45 8-byte pairs, walked by `0x004042AC` (`_dosmaperr` [hypothesis]). |
| `0x00406708` | game and CRT | `""`, a single null byte. The only game string stored outside `0x00406030`–`0x00406133` (see below). |
| `0x0040670C` | game and CRT | `errno`, which `main` passes to `install_allegro` and which `0x004042AC` writes. |
| `0x00406710`–`0x00406C3B` | CRT | CRT `.bss`. `0x00406710` receives the system error code in `0x004042AC` (`_doserrno` [hypothesis]). |

Observations:
- [verified] The game has **no global variable**: in `.data`, `main` references only its strings, `""` and `errno`. There is neither game `.bss` nor game COMMON.
- [verified] The strings of `main` are stored **in the reverse order of their use**, as in happy:
  - `"maps"` (1st used, `0x00401069`) is the last one (`0x00406130`);
  - the final message (last used, `0x00401479`) is the first one (`0x00406030`).
- [verified] **Anomaly**: the `""` string selected by `main` (`0x00401456`, `0x00401466`, instead of `"s"`) is at `0x00406708`, in the CRT range. It lies just after the table walked by `0x004042AC` (`cmp eax, 0x00406708` is the loop's end bound) and just before `errno`. It is not with the other game strings. [hypothesis] The game's `""` was merged with an empty string from a CRT .obj, and the CRT copy was kept. This anomaly is of the same nature as `"OK"` in happy (`layout-happy.md` §9, question 5). Both show that the rule "a string follows its first user" has exceptions, which must be understood before the matching phase.

## 7. Functions missing from Ghidra (7)

| Address | Size | Class | Reason for adding | Role [hypothesis] |
|---|---:|---|---|---|
| `0x00401000` | 1,172 | GAME | `push` at `0x004014B4` (argument of `_WinMain`) | `main` |
| `0x0040173D` | 18 | CRT | `push` at `0x00401010` (argument of `install_allegro`) | `atexit` (identical to happy `0x0040B31E`) |
| `0x0040174F` | 47 | CRT | pointer `0x0040600C` | `__onexitinit` |
| `0x004022C8` | 34 | CRT | `push` at `0x004022F4`, `cmp` at `0x0040235B` | SEH unwind handler |
| `0x00402352` | 35 | CRT | dead code | `_abnormal_termination` |
| `0x00402375` | 9 | CRT | dead code | `_NLG_Notify1` |
| `0x004023A0` | 189 | CRT | `push` at `0x004017B9`, `0x00403E2F`, `0x0040407E` | `_except_handler3` |

## 8. Blocks

| Block | Range | Funcs | Bytes | Contents | Start |
|---|---|---:|---:|---|---|
| block_01 | `0x00401000`–`0x004014CF` | 2 | 1,209 | `main` + `WinMain` stub | **strong** (start of `.text`) |
| block_02_crt | `0x004014D0`–`0x0040438D` | 68 | 11,927 | LIBC CRT and `RtlUnwind` thunk | **strong** (`_strncpy`, Function ID) |

- [verified in the Allegro sources] `END_OF_MAIN()` places `WinMain` right after `main`, in the same file.
- **[strong hypothesis] loader's game fits in a single .c file** containing `main` and `END_OF_MAIN()`. No other function lies before the CRT, and the game has neither globals nor `static` functions.

## 9. Comparison with happy and maped

- [verified] No game function is shared, apart from the `WinMain` stub.
- [verified] The CRT is shared. 62 functions are identical in all three exes. The two shared only with maped (`_chdir?`, `toupper?`) correspond to the three KERNEL32 imports that happy lacks (`GetCurrentDirectoryA`, `SetCurrentDirectoryA`, `SetEnvironmentVariableA`).
- [verified] loader is the producer of `happy.clv` and happy its consumer (§3). loader's `"happy.clv"` string only proved that the name was present; the code of both exes now establishes the format.

## 10. Open questions specific to loader

1. **Contents of a 335-byte slot** of `happy.clv`. `main` fills it between `0x00401110` and `0x004013C0` (`pack_fgets`, `exists`), and happy reads it in `0x00409F30`. Is it a file name followed by other fields?
2. **The `""` string at `0x00406708`** (§6): to be explained, together with happy's `"OK"`, by the planned compilation experiment (two .c files containing the same literal, linked with LIBC).
3. **Should the 7 missing functions be created in Ghidra**, in particular `main` (`0x00401000`), which Ghidra does not cover at all? Same decision as for happy and maped.
