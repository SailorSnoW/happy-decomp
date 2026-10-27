# decomp.me candidates: VC6 version and compiler options

Goal: five GAME functions of `happy.exe` to submit to decomp.me in order to pin down the compiler version and options. **Nothing has been compiled.** The C proposals below are starting points to test.

Sources:
- disassembly: `scripts/decompme_candidates.py --dump <addresses>` (capstone, reading `game/happy.exe`, bytes [verified]);
- list and traits of the candidates: `scripts/decompme_candidates.py` without arguments;
- pseudo-C: Ghidra (`get_functions`, `decompiled_code` field, read-only), copied as is;
- pairs with maped: `notes/data/shared-functions.csv`.

## 1. Selection

The script lists 26 GAME functions of 15 to 60 instructions. Detected traits: loop (`loop`, jump to a lower address), jump table (`switch-table`), field (`field`, `[reg+disp]`, reg ≠ `esp`), call (`call`), arithmetic (`arith`), `ebp` frame (`ebp-frame`).

- [verified] No GAME function of this size has an `ebp` frame.
- [verified] The game code contains only two jump tables (`switch` compiled to a table): `0x004067B5` in `0x00406560` (200 instructions) and `0x0040994B` in `0x00409870` (449 instructions). Both are too large. Multi-way branching is therefore covered by a chain of `if`s (`0x00409010`). A `switch`-like `cmp`/`je` chain exists in `0x0040AC40` (88 instructions, values 0 to 10 of a tile byte), out of range.
- 3 of the 14 happy/maped pairs are identical and between 15 and 60 instructions: `0x0040A9A0`, `0x0040AB00`, `0x0040AC10`. All three are kept. The other identical pairs are over 60 instructions (`0x00401160`, `0x0040AA30`, `0x0040AB30`) or have no useful trait (`0x004017C0`: sequence of string pointers; `0x0040A970`: `END_OF_MAIN` stub, Allegro header code).

| # | happy | maped | size | instr. | loop | branches | fields | calls | arithmetic |
|---|---|---|---|---|---|---|---|---|---|
| 1 | `0x0040A9A0` | `0x00401000` (identical) | 135 B | 54 | 2 nested | 4 | yes (`+1`, `+2`, `+0x88`) | intrinsic `memset` | `imul`, `lea x*5` |
| 2 | `0x0040AB00` | `0x00401260` (identical) | 39 B | 15 | — | 2 | yes (`+0x88`) | 2 × `0x0040B463` (CRT) | — |
| 3 | `0x0040AC10` | `0x00401370` (identical) | 48 B | 16 | — | 1 | — | 2 Allegro imports | `setne` |
| 4 | `0x00409010` | — | 105 B | 31 | — | 6 | — | — | `inc`, signed comparisons |
| 5 | `0x00406C80` | — | 77 B | 25 | 1 (`volatile`) | 3 | `key[0x3B]` | — | signed division by 15 |

Other candidates noted, in case one of the five is a problem:
- `0x0040AD20` (15 instructions): field + `sar` + `imul`. Map module block, but missing from maped.
- `0x00401970` (23 instructions): loop over an array of `0x18`-byte structures, intrinsic `strcpy` of `"Johan Peitz"`, `10000 - i*1500` reduced to an addition.
- `0x00403A40` (17 instructions): same pattern as the first half of `0x00409010`, on the same globals (`0x0042F52C`, `0x0042F530`).

## 2. Common context for decomp.me

decomp.me compiles an isolated function: it must be given the declarations. The block below reproduces what the Allegro 4.0.3 headers produce with MSVC when linking against the DLL:
- `AL_FUNC(type, name, args)` expands to `__declspec(dllimport) type __cdecl name args` (`include/allegro/platform/almsvc.h:55-60`);
- `key` is declared `AL_ARRAY(volatile char, key)` (`include/allegro/keyboard.h:61`);
- `KEY_ESC` is 59 (`keyboard.h:152`).

[verified] In the binary, the calls go directly through the IAT (`ff 15`, `call dword ptr [0x0041116C]`, `0x0040AC28`), without a thunk: this is the `dllimport` form.

The map structure comes from the creation function `0x0040AA30`:
- [verified] `malloc(0x8C)` (`0x0040AA32`);
- [verified] `malloc(a * b * 5)` stored at `+0x88` (`0x0040AA4C`–`0x0040AA5F`);
- [verified] `a` and `b` are stored at `+1` and `+2` (`0x0040AA7B`, `0x0040AA7E`);
- [verified] `"unnamed"` is copied to `+5` and `"unnamed.dat"` to `+0x55`;
- [verified] constant bytes are written from `+0x77` to `+0x83`.

[hypothesis] `+1` is the width and `+2` the height, in tiles. Indeed, `0x0040AD20` reads the tile `(arg3 >> 4) * [+1] + (arg2 >> 4)` (`0x0040AD2A`–`0x0040AD40`), i.e. `y * width + x`.

The field names below are offsets, not original names. The size of the arrays `f03`, `f05`, `f55` and `f77` is deduced from the neighbouring offsets, not observed.

```c
#define NULL ((void *)0)
typedef unsigned int size_t;

typedef struct DATAFILE DATAFILE;
__declspec(dllimport) DATAFILE * __cdecl load_datafile(const char *filename);
__declspec(dllimport) void __cdecl unload_datafile(DATAFILE *dat);
extern __declspec(dllimport) volatile char key[];
#define KEY_ESC 59

void __cdecl free(void *p);
void * __cdecl memset(void *p, int c, size_t n);

typedef struct TILE5 {
    unsigned char t0;
    unsigned char t1;
    unsigned char t2;
    unsigned char t3;
    unsigned char t4;
} TILE5;

typedef struct MAP5 {
    unsigned char f00;
    unsigned char f01;
    unsigned char f02;
    unsigned char f03[2];
    char f05[0x50];
    char f55[0x22];
    unsigned char f77[0x11];
    TILE5 *f88;
} MAP5;
```

Remarks:
- `sizeof(TILE5)` is 5, which fits: it only contains `char`s.
- `sizeof(MAP5)` is `0x8C` and `f88` lands at `+0x88` with `/Zp1`, `/Zp4` or `/Zp8`. This structure therefore cannot distinguish the value of `/Zp`.

## 3. Function 1: `0x0040A9A0`

**Choice**: present as is in maped (`0x00401000`). It is the richest of the five:
- two nested loops;
- byte fields and a pointer;
- `imul` and `lea [x+x*4]`;
- an inline-expanded `memset` (`rep stosd` + `rep stosb`).

Only caller: `0x0040AA30` (map creation), right after the `malloc`s [verified, `0x0040AAE8`].

### Disassembly

```
0040A9A0  8b 54 24 04              mov    edx, dword ptr [esp + 4]
0040A9A4  53                       push   ebx
0040A9A5  33 db                    xor    ebx, ebx
0040A9A7  56                       push   esi
0040A9A8  3b d3                    cmp    edx, ebx
0040A9AA  57                       push   edi
0040A9AB  74 76                    je     0x40aa23
0040A9AD  33 c0                    xor    eax, eax
0040A9AF  33 c9                    xor    ecx, ecx
0040A9B1  8a 42 02                 mov    al, byte ptr [edx + 2]
0040A9B4  8a 4a 01                 mov    cl, byte ptr [edx + 1]
0040A9B7  8b f0                    mov    esi, eax
0040A9B9  8b ba 88 00 00 00        mov    edi, dword ptr [edx + 0x88]
0040A9BF  0f af f1                 imul   esi, ecx
0040A9C2  33 c0                    xor    eax, eax
0040A9C4  8d 0c b6                 lea    ecx, [esi + esi*4]
0040A9C7  8b f1                    mov    esi, ecx
0040A9C9  c1 e9 02                 shr    ecx, 2
0040A9CC  f3 ab                    rep stosd dword ptr es:[edi], eax
0040A9CE  8b ce                    mov    ecx, esi
0040A9D0  33 f6                    xor    esi, esi
0040A9D2  83 e1 03                 and    ecx, 3
0040A9D5  f3 aa                    rep stosb byte ptr es:[edi], al
0040A9D7  38 5a 01                 cmp    byte ptr [edx + 1], bl
0040A9DA  76 47                    jbe    0x40aa23
0040A9DC  8a 42 02                 mov    al, byte ptr [edx + 2]
0040A9DF  33 c9                    xor    ecx, ecx
0040A9E1  3a c3                    cmp    al, bl
0040A9E3  76 34                    jbe    0x40aa19
0040A9E5  33 c0                    xor    eax, eax
0040A9E7  8b ba 88 00 00 00        mov    edi, dword ptr [edx + 0x88]
0040A9ED  8a 42 01                 mov    al, byte ptr [edx + 1]
0040A9F0  0f af c1                 imul   eax, ecx
0040A9F3  03 c6                    add    eax, esi
0040A9F5  41                       inc    ecx
0040A9F6  8d 04 80                 lea    eax, [eax + eax*4]
0040A9F9  88 1c 07                 mov    byte ptr [edi + eax], bl
0040A9FC  8b ba 88 00 00 00        mov    edi, dword ptr [edx + 0x88]
0040AA02  88 5c 07 01              mov    byte ptr [edi + eax + 1], bl
0040AA06  8b ba 88 00 00 00        mov    edi, dword ptr [edx + 0x88]
0040AA0C  88 5c 07 02              mov    byte ptr [edi + eax + 2], bl
0040AA10  33 c0                    xor    eax, eax
0040AA12  8a 42 02                 mov    al, byte ptr [edx + 2]
0040AA15  3b c8                    cmp    ecx, eax
0040AA17  7c cc                    jl     0x40a9e5
0040AA19  33 c9                    xor    ecx, ecx
0040AA1B  46                       inc    esi
0040AA1C  8a 4a 01                 mov    cl, byte ptr [edx + 1]
0040AA1F  3b f1                    cmp    esi, ecx
0040AA21  7c b9                    jl     0x40a9dc
0040AA23  5f                       pop    edi
0040AA24  5e                       pop    esi
0040AA25  5b                       pop    ebx
0040AA26  c3                       ret    
```

### Ghidra pseudo-C

```c
void __cdecl FUN_0040a9a0(int param_1)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  
  if (param_1 != 0) {
    uVar3 = (uint)*(byte *)(param_1 + 2) * (uint)*(byte *)(param_1 + 1) * 5;
    puVar6 = *(undefined4 **)(param_1 + 0x88);
    for (uVar2 = uVar3 >> 2; uVar2 != 0; uVar2 = uVar2 - 1) {
      *puVar6 = 0;
      puVar6 = puVar6 + 1;
    }
    iVar5 = 0;
    for (uVar3 = uVar3 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
      *(undefined1 *)puVar6 = 0;
      puVar6 = (undefined4 *)((int)puVar6 + 1);
    }
    if (*(char *)(param_1 + 1) != '\0') {
      do {
        iVar4 = 0;
        if (*(char *)(param_1 + 2) != '\0') {
          do {
            iVar1 = (uint)*(byte *)(param_1 + 1) * iVar4;
            iVar4 = iVar4 + 1;
            iVar1 = (iVar1 + iVar5) * 5;
            *(undefined1 *)(*(int *)(param_1 + 0x88) + iVar1) = 0;
            *(undefined1 *)(*(int *)(param_1 + 0x88) + 1 + iVar1) = 0;
            *(undefined1 *)(*(int *)(param_1 + 0x88) + 2 + iVar1) = 0;
          } while (iVar4 < (int)(uint)*(byte *)(param_1 + 2));
        }
        iVar5 = iVar5 + 1;
      } while (iVar5 < (int)(uint)*(byte *)(param_1 + 1));
    }
  }
  return;
}
```

### C89 proposal

```c
void FUN_0040a9a0(MAP5 *m)
{
    int x;
    int y;
    int i;

    if (m != NULL) {
        memset(m->f88, 0, m->f02 * m->f01 * sizeof(TILE5));
        for (x = 0; x < m->f01; x++) {
            for (y = 0; y < m->f02; y++) {
                i = m->f01 * y + x;
                m->f88[i].t0 = 0;
                m->f88[i].t1 = 0;
                m->f88[i].t2 = 0;
            }
        }
    }
}
```

### Uncertainties
- **Local index or repeated expression.** [verified] `m->f88` is reloaded before each of the three stores (`0x0040A9E7`, `0x0040A9FC`, `0x0040AA06`), but the index `eax` is computed only once. [hypothesis] Without `/Oa`, a byte store may modify `m->f01` just as well as `m->f88`: if the source repeated `m->f88[m->f01 * y + x]`, `f01` would also have to be reloaded. Hence the variable `i`. Variant to test: the expression repeated three times, without `i`.
- **Operand order of the product.**
  - For the `memset`, `[+2]` is loaded before `[+1]` and is the destination of `imul esi, ecx` (`0x0040A9B1`–`0x0040A9BF`): `m->f02 * m->f01` or `m->f01 * m->f02`?
  - For the index, it is `imul eax(f01), ecx(y)`: `m->f01 * y` or `y * m->f01`?
- **Type of `x` and `y`.** [verified] The loop-end tests are signed (`jl`, `0x0040AA17`, `0x0040AA21`). The entry tests are unsigned (`jbe`, `0x0040A9DA`, `0x0040A9E3`). [hypothesis] These entry tests are the evaluation of `0 < (unsigned char)field`: `x` and `y` are `int`s, compared with promoted `unsigned char`s.
- **Loop form.** `for` or `while` probably give the same code (loop rotation with an entry test). Order `x` outer, `y` inner: deduced from the registers, `esi` outside and `ecx` inside.
- **Test of `m`.** `if (m != NULL) {...}` or `if (m == NULL) return;`.
- **Fields written.** Fields `t0`–`t2` of a structure, or an `unsigned char *` with `i * 5 + 0/1/2`: same generated address, evaluation order possibly different.
- **`memset`.** Call to `memset` with `/Oi`, or `#pragma intrinsic(memset)` (see §8).

## 4. Function 2: `0x0040AB00`

**Choice**: present as is in maped (`0x00401260`). It has a field access and two `__cdecl` calls to a CRT function. It frees the structure created by `0x0040AA30`. It has 7 callers (`notes/data/functions-happy.csv`).

`0x0040B463` is `free` [hypothesis] (name proposed in `functions-happy.csv`): it frees what `0x0040B492` (`_malloc`, [verified] Function ID) allocated in `0x0040AA30`.

### Disassembly

```
0040AB00  56                       push   esi
0040AB01  8b 74 24 08              mov    esi, dword ptr [esp + 8]
0040AB05  8b 86 88 00 00 00        mov    eax, dword ptr [esi + 0x88]
0040AB0B  85 c0                    test   eax, eax
0040AB0D  74 09                    je     0x40ab18
0040AB0F  50                       push   eax
0040AB10  e8 4e 09 00 00           call   0x40b463  ; function 0x0040B463 (CRT)
0040AB15  83 c4 04                 add    esp, 4
0040AB18  85 f6                    test   esi, esi
0040AB1A  74 09                    je     0x40ab25
0040AB1C  56                       push   esi
0040AB1D  e8 41 09 00 00           call   0x40b463  ; function 0x0040B463 (CRT)
0040AB22  83 c4 04                 add    esp, 4
0040AB25  5e                       pop    esi
0040AB26  c3                       ret    
```

### Ghidra pseudo-C

```c
void __cdecl FUN_0040ab00(LPVOID param_1)

{
  if (*(LPVOID *)((int)param_1 + 0x88) != (LPVOID)0x0) {
    FUN_0040b463(*(LPVOID *)((int)param_1 + 0x88));
  }
  if (param_1 != (LPVOID)0x0) {
    FUN_0040b463(param_1);
  }
  return;
}
```

### C89 proposal

```c
void FUN_0040ab00(MAP5 *m)
{
    if (m->f88 != NULL)
        free(m->f88);
    if (m != NULL)
        free(m);
}
```

### Uncertainties
- [verified] `m->f88` is read before the `m != NULL` test (`0x0040AB05`, `0x0040AB18`): the order of the two `if`s is the source order.
- `if (m->f88)` or `if (m->f88 != NULL)`: same code expected.
- Parameter type: `MAP5 *` or `void *` with a cast. Only the `0x88` displacement is visible.

## 5. Function 3: `0x0040AC10`

**Choice**: present as is in maped (`0x00401370`). It makes two Allegro import calls through the IAT, uses a global and returns a boolean (`setne`). Only caller: `0x00403170`. The global `0x004450B4` is read by 10 functions (`notes/data/datarefs-happy.csv`).

### Disassembly

```
0040AC10  a1 b4 50 44 00           mov    eax, dword ptr [0x4450b4]  ; data 0x004450B4 (.data)
0040AC15  85 c0                    test   eax, eax
0040AC17  74 0a                    je     0x40ac23
0040AC19  50                       push   eax
0040AC1A  ff 15 2c 11 41 00        call   dword ptr [0x41112c]  ; unload_datafile
0040AC20  83 c4 04                 add    esp, 4
0040AC23  8b 44 24 04              mov    eax, dword ptr [esp + 4]
0040AC27  50                       push   eax
0040AC28  ff 15 6c 11 41 00        call   dword ptr [0x41116c]  ; load_datafile
0040AC2E  83 c4 04                 add    esp, 4
0040AC31  33 c9                    xor    ecx, ecx
0040AC33  85 c0                    test   eax, eax
0040AC35  0f 95 c1                 setne  cl
0040AC38  a3 b4 50 44 00           mov    dword ptr [0x4450b4], eax  ; data 0x004450B4 (.data)
0040AC3D  8b c1                    mov    eax, ecx
0040AC3F  c3                       ret    
```

### Ghidra pseudo-C

```c
bool __cdecl FUN_0040ac10(undefined4 param_1)

{
  if (DAT_004450b4 != 0) {
    unload_datafile(DAT_004450b4);
  }
  DAT_004450b4 = load_datafile(param_1);
  return DAT_004450b4 != 0;
}
```

### C89 proposal

```c
extern DATAFILE *DAT_004450b4;

int FUN_0040ac10(const char *filename)
{
    if (DAT_004450b4 != NULL)
        unload_datafile(DAT_004450b4);
    DAT_004450b4 = load_datafile(filename);
    return DAT_004450b4 != NULL;
}
```

### Uncertainties
- **Form of the `return`**: `return DAT_004450b4 != NULL;`, `return (DAT_004450b4 = load_datafile(filename)) != NULL;` or `return DAT_004450b4 ? 1 : 0;`. [verified] The store to the global (`0x0040AC38`) is placed between `setne` and `mov eax, ecx`.
- **Return type**: `int` (Ghidra proposes `bool`, which does not exist in C89).
- [verified] The parameter is reloaded into `eax` then pushed (`0x0040AC23`–`0x0040AC27`), instead of a `push dword ptr [esp+4]`. This is probably a compiler trait, not a source one.
- [verified] `0x004450B4` lies outside the raw part of `.data` (`.data`: VA `0x00412000`, raw size `0x2000`, virtual size `0x35C44`). [hypothesis] It is an uninitialised global (`DATAFILE *x;`), defined in another .c file or in this one.

## 6. Function 4: `0x00409010`

**Choice**: multiple branches (6 conditional jumps, nested `if`s), signed comparisons, two parameters. Only caller: `0x00409080`.

[hypothesis] These are two animation counters: 8 calls advance to the next frame, over 2 frames.

### Disassembly

```
00409010  8b 44 24 04              mov    eax, dword ptr [esp + 4]
00409014  33 c9                    xor    ecx, ecx
00409016  3b c1                    cmp    eax, ecx
00409018  74 2c                    je     0x409046
0040901A  a1 30 f5 42 00           mov    eax, dword ptr [0x42f530]  ; data 0x0042F530 (.data)
0040901F  40                       inc    eax
00409020  83 f8 08                 cmp    eax, 8
00409023  a3 30 f5 42 00           mov    dword ptr [0x42f530], eax  ; data 0x0042F530 (.data)
00409028  75 1c                    jne    0x409046
0040902A  a1 2c f5 42 00           mov    eax, dword ptr [0x42f52c]  ; data 0x0042F52C (.data)
0040902F  89 0d 30 f5 42 00        mov    dword ptr [0x42f530], ecx  ; data 0x0042F530 (.data)
00409035  40                       inc    eax
00409036  83 f8 01                 cmp    eax, 1
00409039  a3 2c f5 42 00           mov    dword ptr [0x42f52c], eax  ; data 0x0042F52C (.data)
0040903E  7e 06                    jle    0x409046
00409040  89 0d 2c f5 42 00        mov    dword ptr [0x42f52c], ecx  ; data 0x0042F52C (.data)
00409046  39 4c 24 08              cmp    dword ptr [esp + 8], ecx
0040904A  74 2c                    je     0x409078
0040904C  a1 24 3b 41 00           mov    eax, dword ptr [0x413b24]  ; data 0x00413B24 (.data)
00409051  40                       inc    eax
00409052  83 f8 08                 cmp    eax, 8
00409055  a3 24 3b 41 00           mov    dword ptr [0x413b24], eax  ; data 0x00413B24 (.data)
0040905A  75 1c                    jne    0x409078
0040905C  a1 28 3b 41 00           mov    eax, dword ptr [0x413b28]  ; data 0x00413B28 (.data)
00409061  89 0d 24 3b 41 00        mov    dword ptr [0x413b24], ecx  ; data 0x00413B24 (.data)
00409067  40                       inc    eax
00409068  83 f8 01                 cmp    eax, 1
0040906B  a3 28 3b 41 00           mov    dword ptr [0x413b28], eax  ; data 0x00413B28 (.data)
00409070  7e 06                    jle    0x409078
00409072  89 0d 28 3b 41 00        mov    dword ptr [0x413b28], ecx  ; data 0x00413B28 (.data)
00409078  c3                       ret    
```

### Ghidra pseudo-C

```c
void __cdecl FUN_00409010(int param_1,int param_2)

{
  if ((param_1 != 0) && (DAT_0042f530 = DAT_0042f530 + 1, DAT_0042f530 == 8)) {
    DAT_0042f530 = 0;
    DAT_0042f52c = DAT_0042f52c + 1;
    if (1 < DAT_0042f52c) {
      DAT_0042f52c = 0;
    }
  }
  if ((param_2 != 0) && (DAT_00413b24 = DAT_00413b24 + 1, DAT_00413b24 == 8)) {
    DAT_00413b24 = 0;
    DAT_00413b28 = DAT_00413b28 + 1;
    if (1 < DAT_00413b28) {
      DAT_00413b28 = 0;
    }
  }
  return;
}
```

### C89 proposal

```c
extern int DAT_0042f52c;
extern int DAT_0042f530;
extern int DAT_00413b24;
extern int DAT_00413b28;

void FUN_00409010(int a, int b)
{
    if (a != 0) {
        DAT_0042f530++;
        if (DAT_0042f530 == 8) {
            DAT_0042f530 = 0;
            DAT_0042f52c++;
            if (DAT_0042f52c > 1)
                DAT_0042f52c = 0;
        }
    }
    if (b != 0) {
        DAT_00413b24++;
        if (DAT_00413b24 == 8) {
            DAT_00413b24 = 0;
            DAT_00413b28++;
            if (DAT_00413b28 > 1)
                DAT_00413b28 = 0;
        }
    }
}
```

### Uncertainties
- **Final comparison**: `> 1` or `>= 2`. [verified] The code has `cmp eax, 1` / `jle` (`0x00409036`, `0x0040903E`). [hypothesis] `>= 2` would rather give `cmp eax, 2` / `jl`.
- **Form of the increment**: `x++; if (x == 8)` or `if (++x == 8)`, and `if (a && ...)` as a single condition (Ghidra's form). Same code expected for all these forms.
- **Types**: signed `int` [verified by `jle` and the 32-bit accesses]. The parameters are read as 32 bits (`0x00409010`, `0x00409046`): `int`, not `char`.
- **Placement of the globals.** [verified] `0x00413B24` and `0x00413B28` are in the raw part of `.data` (initial value 0). `0x0042F52C` and `0x0042F530` are beyond it. [hypothesis] The first two are written `= 0` in the source, the other two without an initialiser. No effect on the function's code, but to be reproduced later for the `.data` layout.
- [verified] `0x00403A40` repeats the first block, under the condition `[0x0042F520] == 0`: a macro is possible [hypothesis].

## 7. Function 5: `0x00406C80`

**Choice**:
- a wait loop on two `volatile` memory locations;
- a signed division by a constant (multiplication by `0x88888889`);
- a read of Allegro's `key[]` through the IAT;
- a boolean return.

Callers: `0x00406CD0`, `0x00406D60`, `0x00409080`.

[verified] `0x00442084` is incremented by `0x004018F0`. This callback is installed by `install_int(0x004018F0, 15)` (`0x00401F6B`–`0x00401F72`); it therefore runs every 15 ms. [hypothesis] The function waits `n` ms (`n / 15` ticks), and stops early if Escape (`key[KEY_ESC]`, `0x3B` = 59) is pressed. It returns 1 in that case.

### Disassembly

```
00406C80  8b 4c 24 04              mov    ecx, dword ptr [esp + 4]
00406C84  b8 89 88 88 88           mov    eax, 0x88888889
00406C89  f7 e9                    imul   ecx
00406C8B  03 d1                    add    edx, ecx
00406C8D  c1 fa 03                 sar    edx, 3
00406C90  8b c2                    mov    eax, edx
00406C92  c1 e8 1f                 shr    eax, 0x1f
00406C95  03 d0                    add    edx, eax
00406C97  c7 05 84 20 44 00 00 00 00 00 mov    dword ptr [0x442084], 0  ; data 0x00442084 (.data)
00406CA1  a1 84 20 44 00           mov    eax, dword ptr [0x442084]  ; data 0x00442084 (.data)
00406CA6  3b c2                    cmp    eax, edx
00406CA8  a1 c8 11 41 00           mov    eax, dword ptr [0x4111c8]  ; key
00406CAD  7d 11                    jge    0x406cc0
00406CAF  8a 48 3b                 mov    cl, byte ptr [eax + 0x3b]
00406CB2  84 c9                    test   cl, cl
00406CB4  75 0a                    jne    0x406cc0
00406CB6  8b 0d 84 20 44 00        mov    ecx, dword ptr [0x442084]  ; data 0x00442084 (.data)
00406CBC  3b ca                    cmp    ecx, edx
00406CBE  7c ef                    jl     0x406caf
00406CC0  33 c9                    xor    ecx, ecx
00406CC2  8a 50 3b                 mov    dl, byte ptr [eax + 0x3b]
00406CC5  84 d2                    test   dl, dl
00406CC7  0f 95 c1                 setne  cl
00406CCA  8b c1                    mov    eax, ecx
00406CCC  c3                       ret    
```

### Ghidra pseudo-C

```c
bool __cdecl FUN_00406c80(int param_1)

{
  DAT_00442084 = 0;
  do {
    if (param_1 / 0xf < 1) break;
  } while (key_exref[0x3b] == (code)0x0);
  return key_exref[0x3b] != (code)0x0;
}
```

**This pseudo-C is wrong.** Ghidra ignores the fact that `0x00442084` is reloaded (`0x00406CA1`, `0x00406CB6`): it treats it as ordinary memory, so the condition becomes `param_1 / 0xf < 1`. The disassembly is authoritative.

### C89 proposal

```c
extern volatile int DAT_00442084;

int FUN_00406c80(int ms)
{
    DAT_00442084 = 0;
    while (DAT_00442084 < ms / 15 && !key[KEY_ESC])
        ;
    return key[KEY_ESC] != 0;
}
```

### Uncertainties
- **`volatile`.** [verified] The global is reloaded right after being set to 0 (`0x00406C97`–`0x00406CA1`) and then on every iteration. `key[0x3B]` is reloaded on every iteration (`0x00406CAF`). [hypothesis] Both are `volatile`: it is declared for `key` (`keyboard.h:61`), deduced for `0x00442084`. In the callback `0x004018F0`, the increment is done with `mov`/`inc`/`mov` rather than `inc dword ptr [...]`, which is consistent with `volatile` [hypothesis].
- **Loop form.** [verified] The code first tests `counter < n` (`0x00406CA6`), then, inside the loop, `key` before `counter < n`. This is the rotation of `while (A && B)`. Possible equivalent variant: `for (...; counter < n; ) if (key[KEY_ESC]) break;`.
- **Quotient in a local.** `n = ms / 15` is computed before the reset to 0 (`0x00406C80`–`0x00406C95`). It may come from an invariant hoisted out of the loop by the compiler, or from a local variable `int n = ms / 15;` in the source.
- **Type of `ms`.** [verified] The division is signed: `imul`, `sar`, then the `shr 31` correction (`0x00406C89`–`0x00406C95`). `ms` is therefore an `int`, not an `unsigned`.
- **Return**: `!= 0`, `? 1 : 0` or `return key[KEY_ESC];`. The last one would be a `movsx` rather than a `setne`: it is ruled out [hypothesis].

## 8. What the code reveals about the options

The [verified] traits are observed in the bytes above. Their interpretation as options is [hypothesis]: the VC6 option equivalences quoted come from its documentation and have not been verified by compiling.

| Observed trait | Where | Interpretation |
|---|---|---|
| The caller pops the arguments (`add esp, 4` after `call`), `ret` without operand | `0x0040AB15`, `0x0040AC20`, `0x0040AC26` | [strong hypothesis] `__cdecl`, the default convention (`/Gd`). The Allegro imports are `__cdecl` by declaration (`almsvc.h:60`) |
| No `push ebp` / `mov ebp, esp`; arguments read at `[esp+N]`, with `N` shifted by 4 after each `push` (`[esp+4]` then `[esp+8]` after `push esi`, `0x0040AB01`) | all 5 | [strong hypothesis] `/Oy` (frame pointer omission), included in `/O1`, `/O2` and `/Ox`. Rules out a build without optimisation and `/GZ` |
| `memset` expanded into `rep stosd` + `and ecx, 3` + `rep stosb` | `0x0040A9C2`–`0x0040A9D5` | [hypothesis] intrinsics enabled (`/Oi`). According to the VC6 documentation, `/O2` = `/Og /Oi /Ot /Oy /Ob1 /Gs /Gf /Gy` and `/O1` = `/Og /Os /Oy /Ob1 /Gs /Gf /Gy`: `/Oi` points to `/O2` or `/Ox`, or else `/O1 /Oi`, or a `#pragma intrinsic`. Same clue elsewhere: `strcpy` of a constant as dword `mov`s (`0x0040AABA`–`0x0040AAE5`, `0x00401970`) |
| Division by 15 as a multiplication by the inverse | `0x00406C84`–`0x00406C95` | [hypothesis] speed optimisation. To test: check whether `/O1` (`/Os`) keeps the multiplication or falls back to `idiv`. This is the best `/O1` vs `/O2` test of the five |
| Invariant expressions hoisted out of the loop (`ms / 15`); in `0x00401970`, `10000 - i*1500` reduced to `ecx += 0x5DC` and an induction pointer compared with an end address | `0x00406C80`, `0x00401999`–`0x004019AF` | [hypothesis] `/Og` (global optimisations), included in `/O1` and `/O2` |
| Pointer `m->f88` reloaded after each byte store; fields `f01`/`f02` reloaded on every iteration | `0x0040A9E7`–`0x0040AA12` | [hypothesis] no `/Oa` or `/Ow` (aliasing assumed, default behaviour) |
| Zero extension by `xor r32, r32` + `mov r8, [m]` instead of `movzx` | `0x0040A9AD`–`0x0040A9B4`, `0x0040AA10` | [hypothesis] Pentium scheduling (`/GB` by default, or `/G5`). Discriminating test: compile with `/G6` to see whether `movzx` appears |
| Interleaved instructions: `cmp` then store then jump; load of `key` between `cmp` and `jge` | `0x00409020`–`0x00409028`, `0x00406CA6`–`0x00406CAD` | [hypothesis] `/O2` (`/Ot`) scheduling. To compare with `/O1` |
| Register set to 0 and reused for stores and comparisons with 0 (`ebx`, `ecx`) | `0x0040A9A5`, `0x00409014` | Present with `/O1` as with `/O2`, not very discriminating |
| Boolean via `xor` + `test` + `setne` | `0x0040AC31`–`0x0040AC35`, `0x00406CC0`–`0x00406CC7` | optimised VC6 idiom, not very discriminating |
| No internal loop alignment (targets `0x0040A9E5`, `0x00406CAF` unaligned, no padding inside functions) | — | consistent with VC6, which does not align loops |
| Functions aligned on 16, `0x90` padding (`notes/layout-happy.md`) | start of the 5 functions | [hypothesis] related to `/Gy` and to the linker. Not visible on decomp.me, which compares the function alone |
| No inlining of `0x0040A9A0` into its only caller `0x0040AA30`, in the same module | `0x0040AAE8` | [hypothesis] `/Ob1` (default of `/O2`) rather than `/Ob2`, unless 54 instructions exceed the `/Ob2` threshold |
| Allegro calls via `call [IAT]` without a thunk | `0x0040AC1A`, `0x0040AC28` | headers compiled with `__declspec(dllimport)` (DLL linking, without `ALLEGRO_STATICLINK`) |

**Summary** [hypothesis]: `cl /O2` or `/Ox`, default conventions (`/Gd`, `/Zp8`), without `/Oa`, `/GZ` or a stack frame. The exact code generation options (`/GB`, `/G5` or `/G6`) remain to be settled.

**Version.** The Rich header gives cl 12.00.8168 in all three exes [verified, `notes/STATUS.md`]. The repository does not say which VC6 service pack this number corresponds to. Nor does it say which of the VC6 compilers offered by decomp.me produces this number: to be checked on the site before testing.

## 9. Proposed test order (decomp.me, nothing has been run)

1. `0x00406C80` with `/O2` then `/O1`: division by 15 and scheduling.
2. `0x0040A9A0` with `/O2`: `memset` intrinsic, then the variant without the variable `i` (aliasing). Then `/G5`, `/G6`, `/GB` (`movzx` or not).
3. `0x00409010`: interleaving of the stores, `> 1` versus `>= 2`.
4. `0x0040AC10` and `0x0040AB00`: check of the calling convention, of the `dllimport` imports and of the `setne` idiom.
5. Redo the best configuration on each decomp.me VC6 compiler: the version will be the one that gives 100% on all five.

## 10. decomp.me results

### `0x00406C80`: 100% match (2026-10-27)

- Compiler: decomp.me's **VC 6.0 without service pack** preset (reported by the user). Scratch: https://decomp.me/scratch/42r3i
- Options: `/O2`, on top of the site's default options.
- Label: `_FUN_00406c80`. Target: assembly symbolised by hand from `0x00406C80`–`0x00406CCC` (`_DAT_00442084`, `__imp__key`).
- Context:

```c
#define KEY_ESC 59
extern __declspec(dllimport) volatile char key[];
extern volatile int DAT_00442084;
```

- Matching source:

```c
int FUN_00406c80(int ms)
{
    int n;

    n = ms / 15;
    DAT_00442084 = 0;
    while (DAT_00442084 < n && !key[KEY_ESC])
        ;
    return key[KEY_ESC] != 0;
}
```

Steps:
1. First proposal from §7, under `/O2`: the only difference was that `DAT_00442084 = 0` was placed before the division instead of after.
2. Quotient computed in the local variable `n`, before the reset to 0: full match.

What this result settles [verified by the match, for this function]:
- `/O2` produces the division by multiplication (`0x88888889`) and the observed scheduling. This is consistent with a speed optimisation setting, but `/O1` was not compared in detail: the user reports that `/O2` gives the best score.
- The compiler does not move the division before a `volatile` write. The quotient in a local is therefore part of the source.
- `while (A && B)` does give the observed loop (rotation with an entry test).
- `return key[KEY_ESC] != 0;` gives the `xor` / `setne` idiom.
- `volatile` is confirmed for `DAT_00442084` (incremented by the timer callback `0x004018F0`) and for `key`.
- Zero extension by `xor` + `mov r8` (no `movzx`) does not occur in this function: `/G5`, `/G6` or `/GB` remains to be settled on `0x0040A9A0`.

Not yet established:
- Exact VC6 version. According to the user, all decomp.me VC6 presets give the match except "SP5 + Processor Pack", whose score drops to 75%: the compiled code differs from the target. **SP5 + Processor Pack is therefore ruled out** [verified by decomp.me, reported by the user]. On the other hand, this function does not distinguish VC 6.0 without SP from the other service packs.
- [hypothesis] cl 12.00.8168 (Rich header) is the compiler of VC6 without SP, which would be consistent with this result. This is verified neither in the repository nor on decomp.me.

### `0x0040A9A0`: 100% match (2026-10-27)

- Compiler: VC 6.0 without SP. Options: `/O2`. Scratch: https://decomp.me/scratch/72ufv
- Label: `_FUN_0040a9a0`. Target: assembly symbolised by hand from `0x0040A9A0`–`0x0040AA26`. There is no external reference: only displacements within the structure.
- Context: `TILE5`, `MAP5`, declaration of `memset` (§2), without `#pragma intrinsic`.
- Source: the C89 proposal from §3, with the local index `i`, `m->f02 * m->f01` for the `memset` and `m->f01 * y + x` for the index.

Option variants (reported by the user):

| Options | Result |
|---|---|
| `/O2` | 100% |
| `/O2 /Gs` | 100% |
| `/O2 /GB` | 100% |
| `/O2 /G5` | 100% |
| `/O2 /G6` | **not** 100% |

What this result settles [verified by the match, for this function]:
- **`/G6` ruled out.** [hypothesis] The difference comes from zero extension: `xor` + `mov r8` in the target, `movzx` expected with `/G6`. The `/G6` diff was not recorded.
- `/GB` and `/G5` cannot be told apart here. [hypothesis] `/GB` is the default, so the simplest is to pass no `/G` option at all.
- `/Gs` has no visible effect: the function has no local variables on the stack.
- `/O2` alone produces the `memset` intrinsic (`rep stosd` + `rep stosb`), without `#pragma intrinsic`: this is consistent with `/Oi` being included in `/O2`.
- **Source form**: the index is indeed computed only once (local variable `i`). The proposed operand order is the right one for both products. The `for` loops over `int`s compared with `unsigned char`s produce the observed `jbe` entry tests and `jl` end tests.

Not yet established for this function:
- The variant without `i` (repeated expression) has not been tried. Nothing proves it differs.

Service packs (reported by the user): all VC6 presets give 100% except **SP5 + Processor Pack**, as for `0x00406C80`. The two functions therefore do not distinguish VC 6.0 without SP from SP1 to SP6 without Processor Pack. Both rule out SP5 + Processor Pack.

### `0x00409010`: 100% match (2026-10-27)

- Compiler: VC 6.0 without SP. Options: `/O2`. Scratch: https://decomp.me/scratch/8bHPJ
- Label: `_FUN_00409010`. Target: assembly symbolised by hand from `0x00409010`–`0x00409078` (`_DAT_0042f52c`, `_DAT_0042f530`, `_DAT_00413b24`, `_DAT_00413b28`).
- Context: the four `extern int`s. Source: the C89 proposal from §6, as is (`x++;` then `if (x == 8)`, and `> 1`).
- Service packs (reported by the user): all VC6 presets give 100% except SP5 + Processor Pack.

What this result settles [verified by the match, for this function]:
- The stores placed between `cmp` and the conditional jump, as well as the reset to 0 placed after the load of the other counter, come from `/O2` scheduling, not from the source.
- `ecx` = 0 is reused for comparisons and stores: this is compiler behaviour.
- `> 1` gives `cmp eax, 1` / `jle`. The `>= 2` variant has not been tried: nothing proves it differs.

### `0x0040AB00`: 100% match (2026-10-27)

- VC 6.0 without SP, `/O2`. Scratch: https://decomp.me/scratch/rtNCI
- Label `_FUN_0040ab00`. Context: `TILE5`, `MAP5`, `free`. Source: C89 proposal from §4, as is.
- [verified by the match] The order of the two `if`s, with `m->f88` read before the test of `m`, and the saving of `esi` do come from the source and the compiler.
- **Limitation**: the target calls `_free` because I symbolised it that way. This match therefore does not prove that `0x0040B463` is `free`, which remains [hypothesis].

### `0x0040AC10`: 100% match (2026-10-27)

- VC 6.0 without SP, `/O2`. Scratch: https://decomp.me/scratch/pJhdK
- Label `_FUN_0040ac10`. Context: `dllimport` declarations of `load_datafile` and `unload_datafile`, `extern DATAFILE *DAT_004450b4;`. Source: C89 proposal from §5, as is.
- [verified by the match] Several points are confirmed:
  - `return DAT_004450b4 != NULL;` after the assignment gives `xor` / `test` / `setne`, with the store to the global interleaved;
  - the `__declspec(dllimport)` declaration gives `call dword ptr [__imp__...]`;
  - the parameter is reloaded into `eax` before being pushed: this is compiler behaviour.

For these two functions, the user did not report any attempt with the service pack presets.

## 11. Conclusion

**5 functions out of 5 match at 100%** under **VC 6.0 without SP, `/O2`**, with the C89 proposals of this note. Only `0x00406C80` had to be corrected: the quotient was moved into a local variable, computed before the reset to 0.

| Function | Scratch | Other VC6 presets |
|---|---|---|
| `0x00406C80` | https://decomp.me/scratch/42r3i | all 100% except SP5 + Processor Pack (75%) |
| `0x0040A9A0` | https://decomp.me/scratch/72ufv | all 100% except SP5 + Processor Pack |
| `0x00409010` | https://decomp.me/scratch/8bHPJ | all 100% except SP5 + Processor Pack |
| `0x0040AB00` | https://decomp.me/scratch/rtNCI | not tried |
| `0x0040AC10` | https://decomp.me/scratch/pJhdK | not tried |

Established [verified by match on these 5 functions]:
- **Options**: `/O2` is enough. `/Gs`, `/GB` and `/G5` change nothing. **`/G6` does not match** (tried on `0x0040A9A0`).
- **Compiler**: **SP5 + Processor Pack does not match**. VC 6.0 without SP and the other service packs produce the same code on these functions.
- **Declarations**: those of Allegro (`__declspec(dllimport)`, `volatile char key[]`) reproduce the observed code.

Hypotheses:
- The original compiler is VC 6.0 without SP. This is consistent with these results, but none of the tested functions distinguishes it from SP1 to SP6 without Processor Pack. The Rich header (cl 12.00.8168) is the only remaining clue to settle it. The link between this number and a specific version is not verified.
- The original options are `/O2` without a `/G` option (default `/GB`), or `/O2 /G5`. Their effect on function alignment (16 bytes, `0x90`) is not visible on decomp.me and will be verified locally with reccmp.
