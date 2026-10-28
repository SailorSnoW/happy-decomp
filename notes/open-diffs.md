# Functions below 100%

State on 2026-10-28 (`scripts/progress.py`): happy 84/84, maped 48/50, loader 2/2. The two functions left differ by register allocation only; the shapes already tried are listed so that they are not tried again. The shapes that did work elsewhere are in `notes/matching.md` §3a.

## maped `set_screen_mode` (`0x00401A20`), 85.94%

Same instructions except the register of the map pointer in the 640x480 branch. Original: after `map_save`, `mov eax, [DAT_00410110]` and the two compares use `eax`; `ebp` and `edi` are pushed in the middle of the function, where the clamps begin. Recompiled: the pointer goes to `ebp` in the branch, is reloaded into `ebp` right after the merged `set_gfx_mode` call (the path that skips the call keeps it), so `ebp` and `edi` are pushed at the start.

[verified] Mechanism: with the size test on another global, the rest of the function matches (`push ebp` / `push edi` in the middle); with the clamps on constants, the branch value still goes to a register that is reloaded after the call (`edx`). So VC6 sees the clamps' load of `DAT_00410110` as partially redundant with the branch's load and keeps it on the path without the call (partial redundancy elimination). A store through an unknown pointer between the merge and the clamps (`*(int *)DAT_00422410 = 0;`, a kill) gives the original `eax` in the branch; the original has no such store. [hypothesis] Something between the merge and the clamps that VC6 treats as a kill without emitting code.

Tried without effect (same allocation, 85.94%):
- 2026-10-27: the `if` and the `else` swapped; nested `if`s instead of `&&`; `mode == 0` / `!mode`; an empty `if` with the call in the `else`; a `MAP *` local for the clamps, or for the branch; the `big` test in a local; the `map_save` call in a comma expression; `card` initialised in its declaration; the `MIN` arguments swapped;
- 2026-10-28: the 4 or 6 lines of clamps in a `static __inline` function; the size test in an `__inline` function; nested `if`s; a `goto` past the clamps' block or a label; an empty or dead `else`; a `switch (mode)`; a mode variable then two calls; the size test through `(&DAT_00410110)[0]` or a `MAP *` local; `DAT_00410110` defined (or `static`) in the file; `acc8`/`acc4` through locals or a pointer.

## maped `auto_mask` (`0x00404500`), 96.86%

Current shape [verified 2026-10-28]: `old = 0;` at the top next to `m = 0;` (the original zeroes `edx` there, before the `imul` of the index: the summer cover mask), the winter base mask saved in `old` (`ecx`), `m = MAX(m, old);` then the store from `m` (`mov [t + 2], bl`), and the tile indexed in each access (`DAT_00410110->tiles[i].t0`) rather than through a `TILE *` (97.56% on the object comparator, against 96.35% with the pointer). The prologue, both chains and the two ends match.

One difference left: the winter tile address is in `esi` in the recompiled code, `edx` in the original. Both branches then begin with the same instructions (`mov ecx, [ecx + 0x88]` / `lea eax, [eax + eax*4]` / `push esi` / `lea esi, [ecx + eax]`), and VC6 merges them above the `je` (and the final store into one tail); the original keeps them in each branch because its registers differ.

Tried without effect (2026-10-28, scratch generators, about 150 combinations): the summer cover mask in `old` or a variable of its own, zeroed at the top (before or after the index, in a chained assignment, in the declarations) or in the branch; `m = MAX(m, old)`, `if (old > m) m = old`, `t->t2 = MAX(m, old)`; a `TILE *` in both branches, one, or none, a second pointer for the summer branch; the index recomputed in one branch or both; `x` reused as `m`, `y` as a mask; an early `return` instead of the `else`; `m = 0` inside the branches; the branches swapped (the layout follows the source: no); declaration order (24 random orders, no effect); the summer chain (identical for the base and cover tiles) as a `static __inline` function, worse (66-81%). Before (2026-10-27, 80.54%): index in a local, recomputed in every access, or stored back into `x` or `y`; the parameters reused as the masks; `MAX` arguments swapped.

[hypothesis] The original's register for the winter tile pointer comes from a shape not found yet.
