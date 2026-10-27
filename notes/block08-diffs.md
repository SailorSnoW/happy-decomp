# happy block_08: functions below 100% for a reason other than §3

Functions of `src/happy/block_08.c` whose reccmp diff (2026-10-27) contains something other than an end-of-array pointer or a call to an undecompiled function (`notes/matching.md` §3).

## `main` (`0x0040A830`): 81.90%, 83.81%, then 100%

**Resolved on 2026-10-27** [verified: reccmp 100%]: the start menu loop is written on a flag, like the `while (!done)` of `custom_game`:
```c
done = 0;
while (!done) {
    if (start_menu() < 0)
        goto quit;
    if (DAT_00413b20 != 2 || custom_game() != 0)
        done = 1;
}
```
VC6 threads the jumps through the known values of `done` after the loop optimisations, so the loop is never rotated: the head `call start_menu` stays the target of the back edge and the exit block stays at the end. Four other flag forms give the same code (`done = 1;` then `done = 0` when the custom game returns 0; `done = custom_game();` when `DAT_00413b20 == 2`, otherwise 1; nested `if`s); a `do`/`while (!done)` with nested `if`/`else` gives 95.10%. The form kept is the simplest one.

History (before the fix):

Two differences [verified]:
1. the calls to `0x00409870` (start menu) and `0x00409F30` (custom game), not decompiled yet (§3);
2. the loop around the start menu: the original does not rotate it, the recompiled code does.

Original:
```
0x40a884  call init_game
0x40a889  call 0x409870          ; loop head
0x40a88e  test eax, eax
0x40a890  jl 0x40a95e            ; shutdown_game, at the end of the function
0x40a896  cmp dword ptr [DAT_00413b20], 2
0x40a89d  jne 0x40a8a8           ; introduction
0x40a89f  call 0x409f30
0x40a8a4  test eax, eax
0x40a8a6  je 0x40a889            ; back to the loop head
0x40a8a8  ...
```
Recompiled: the head is copied at the bottom of the loop (`jne <intro>` / `call FUN_00409870` / `test eax, eax` / `jge <loop>`) and the exit block (`call shutdown_game` ... `ret`) is placed right after the loop instead of at the end of the function. The rest of the function (argument loop, introduction, menu loop) is identical; only jump offsets differ after the moved block.

Shapes tried on 2026-10-27, all rotated the same way with `/O2` (scratch compilations, `/FAs` listings):
- `do { if (FUN_00409870() < 0) goto quit; } while (DAT_00413b20 == 2 && FUN_00409f30() == 0);` (kept in `src/`);
- `for (;;)` with `goto quit` then `break` on `DAT_00413b20 != 2` / `FUN_00409f30() != 0` (one `if` or two);
- a backward `goto start` (with `&&`, or with nested `if`s);
- `while (FUN_00409870() >= 0) { if (... == 0) continue; ... }`, with the menu loop inside, leaving through `goto quit` or `break; break;`;
- `if (FUN_00409870() >= 0) { if (...) goto start; ... }`;
- `shutdown_game(); return 0;` written in both exits (VC6 merges them);
- the result in a dead local first (`sel = FUN_00409870(); if (sel < 0) ...`).

The same rotation happens in minimal test functions (`f()`, `g()`, `G` external), also when `f` is defined in the same file. Two shapes are **not** rotated, but neither matches:
- `do { r = FUN_00409870(); } while (r >= 0 && DAT_00413b20 == 2 && FUN_00409f30() == 0); if (r >= 0) { ... }`: no copy of the head, but `r` lives in `esi` (`mov esi, eax`) and is tested again after the loop;
- `/O1` instead of `/O2`: this loop matches, but the next test becomes `cmp dword ptr [DAT_00413b20], 0` where the original has `mov eax, [DAT_00413b20]` / `test eax, eax`, so the file was not compiled with `/O1` [hypothesis: the rest of the function is `/O2` code].

[hypothesis] The original has a shape that keeps the head from being copied (a value live after the loop that VC6 later eliminates, or a control flow that is not visible in the result). Retried once both callees were at 100%: same result. Not the cause in `custom_game`, whose `while (!done)` loop matched (its first block is not a conditional exit).
