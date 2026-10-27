# happy block_05: functions below 100% for a reason other than §3

Functions of `src/happy/block_05.c` whose reccmp diff (2026-10-27) contains something other than an end-of-array pointer or a call to an undecompiled function (`notes/matching.md` §3).

## `take_item` (`0x00403A80`): 85.55%, then 100%

Only difference [verified]: the comparison `++DAT_0042f550 == DAT_00440048`.

Original:
```
0x403b4a  mov al, byte ptr [DAT_0042f550]
0x403b4f  movsx edx, byte ptr [DAT_00440048]
0x403b56  inc al
0x403b58  mov byte ptr [DAT_0042f550], al
0x403b5d  mov eax, dword ptr [DAT_0042f550]
0x403b62  and eax, 0xff
0x403b67  cmp eax, edx
```
Recompiled: `xor ecx, ecx` / `mov cl, al` / `cmp ecx, edx` (the incremented value is reused). The difference in register allocation then spreads to the rest of the function (the flags of `DAT_00414040[t].f4` in `cl` instead of `dl`, etc.), hence the low score.

Cause [verified with test programs, `matching.md` §3a]: the original compiler knew the alignment of `DAT_0042f550`. With `DAT_0042f550` declared as the first byte of a 4-byte structure (temporary test, not kept), the whole function matches the original instruction for instruction, the stack slots included.

Variants tried without effect: separate increment and comparison, operands swapped, casts `(int)`, `(unsigned)`, `(char)`, `volatile`.

**Resolved on 2026-10-27** (decided with the user): `0x0042F500`-`0x0042F567` is declared as one structure, `PLAYER player` (`happy.h`); `take_item` is at 100%.

## `bonus_room` (`0x00405DB0`), 99.34%

Three differences [verified]:
1. end-of-array pointer of the loop over `DAT_00414140` (§3);
2. the call to `0x00404C50`, not decompiled (§3);
3. after the inner loop of the fall (`0x004061E4`), the two reloads are swapped:

Original:
```
0x4061e4  mov esi, dword ptr [esp + 0x1c]   ; y
0x4061e8  mov ebp, dword ptr [esp + 0x18]   ; x
```
Recompiled: `mov ebp, [esp + 0x18]` then `mov esi, [esp + 0x1c]`. The slots, registers and the rest of the code are the same.

Variants tried without effect: every declaration order of `x` and `y`, `y` replaced by `n` (the original reuses the stack slot of `n` for `y`), `while` and `for` instead of `do`/`while`, `x -= 4` before `y++`, another loop variable for the inner loop.
