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

## `bonus_room` (`0x00405DB0`): 99.34%, then 100%

**Resolved on 2026-10-28** [verified: reccmp 100% with `reccmp_run.py`, scratch comparator 458/458]: the first walk (`x` from -16 to 144) and the fall (`y` from 70 to 156) use **the same variable** (`pos` in `src/`). VC6 then numbers it before `x` (the second walk and the fall), and reloads it first after the inner loop, as the original does. Found by renaming the variable of the first walk to each other local: only `y` gives 100% (`c`, `frame`, `count`, `j`: 99.78%). The end-of-array pointer of the first loop is handled by `reccmp_run.py` (`matching.md` §3).

History (before the fix):

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

Retried on 2026-10-27 (scratch comparator), without effect on the swap: every order of the five statements after the inner loop of the fall (the current order is the best one), `y` replaced by `i`, a separate variable for `x` in the second walk and the fall (declared first, last or before `y`) or for the first walk, `for` with `y++` and `x -= 4` in the header (either order), `while (++y < 156)`, `x = x - 4`, `y++` in the call. Deriving the horizontal position from `y` (`x - (y - 70) * 4 - j * 20` and variants, the `draw_level` fix) is worse (81%-87%).

## `play_level` (`0x00404C50`, 4,443 bytes), 99.84% ("100% effective match")

One instruction differs [verified 2026-10-27]: in the deceleration towards 0 from the left (`player.f0c = MIN(0, player.f0c + fixmul(player.f18, f))`), the original adds into the register that holds the `fixmul` result (`add eax, ecx` / `test eax, eax`, `0x004050DA`), the recompiled code into the one that holds `player.f0c` (`add ecx, eax`). reccmp counts it as an effective match. Tried without effect: the operands in either order, `MIN(a, 0)` (2 instructions fewer), an `if`/`else`, `a - -b`. Retried on 2026-10-27: the two evaluations written differently in a `?:` (`fixmul(...) + f0c` for the test, `f0c + fixmul(...)` for the value, and the reverse), `x > 0` instead of `0 < x`, `itofix(0)`, `0L`, `(fixed)0`, casts of the sum or of `fixmul`, `-(-f0c - fixmul(...))`, `if (f0c + fixmul(...) > 0) f0c = 0; else f0c += fixmul(...);`: all give the same instruction; `fixadd`, `MID`, `fixmul(-f18, f)` are worse.

Retried on 2026-10-28:
- in the whole function: every merge of two locals among `last`, `py`, `t4`, `px`, `c`, `i` (the `bonus_room` fix), `PLAYER.f0c` / `f18` typed `fixed`, `fixmul(f, player.f18)`, the `if` inverted (`<= 0`, `< 1`), `-MAX(0, -f0c - ...)`, `MIN(e, 0)`: same instruction or worse;
- in a reduced test (the movement block, lines 754-797, alone in a function, scratch): it gives the same `add ecx, eax` as the full function, so the cause is local. Moving or removing the rest of the block (the slopes, the acceleration, the `f2c = 8` test, `else if`, the two `if`s swapped), casts of either operand, unary `+`, `* 1`, `- -`: always `add ecx, eax`;
- the only form that adds into the `fixmul` result is a `player.f0c` held in a register variable (`v = player.f0c; ... MIN(0, v + fixmul(...))` gives `add eax, esi`), which does not match the stores of the original. [hypothesis] The original compiler kept `player.f0c` as a live common subexpression in `ecx` at that point, for a reason not found in the C.

Found on the way [verified, scratch comparator]: `int done = 0; int last = time_left;` as initialisers (99.14% instead of 93.89%: the order of the first loads and stores); `MIN(player.f18, ...)` and `MAX(-player.f18, ...)` with the limit first; `abs()` (inlined as `cdq` / `xor` / `sub`) instead of Allegro's `ABS`; `(f34 < 4 && f34) || (f38 && f38 < 4)`; `time_left <= -1` instead of `< 0` (`cmp eax, -1` / `jg`), which also lets VC6 keep 0 in `esi` for the rest of the loop; `FALSE`/`TRUE` of Allegro (`TRUE` is -1) for `player.f20` (`neg` / `sbb` / `neg` / `dec`); `PLAYER.f4f` is signed (`movsx`).
