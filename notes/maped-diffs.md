# maped: functions below 100%

State on 2026-10-27, `scripts/progress.py maped`: 50/50 GAME functions annotated, 42 at 100%. This note lists the 8 others with their reccmp diff. Five only differ by an end-of-array pointer (`notes/matching.md` §3); three by register allocation.

Update 2026-10-28: the five end-of-array functions are at 100% with `scripts/reccmp_run.py` (§3); `pick_tile` is at 100%; `auto_mask` is at 96.86%. **48/50 at 100% (92.4% of the bytes)**; left: `set_screen_mode` and `auto_mask`.

## End-of-array pointers only (§3)

Each diff was read: the operand of a `cmp` against the address past an array is the only line [verified].

| Function | Score | Operand (original / recompiled) |
|---|---:|---|
| `FUN_00401160` (`0x004016A0`, common block_01) | 98.75% | `help_texts[0][10]` / `DAT_004107a0[2]+2`, end of `DAT_00414140[].f1a` (100% in maped while no global followed the array in the recompiled exe) |
| `draw_objects` (`0x00402370`) | 98.94% | `misc_datafile` / `0x42a3b0`, end of `DAT_00414140` |
| `load_objects` (`0x00403200`) | 98.25% | `help_texts[0][10]` / `DAT_0042a840[10]`, end of `DAT_00414140[].f1a` |
| `shift_map` (`0x00403EB0`) | 99.15% | `misc_datafile` / `0x42a3b0` |
| `editor_loop` (`0x00404C10`) | 99.73% | three loops over `DAT_00414140` |

## `set_screen_mode` (`0x00401A20`), 85.94%

Same instructions except the register of the map pointer in the 640x480 branch. Original: after `map_save`, `mov eax, [DAT_00410110]` and the two compares use `eax`; `ebp` and `edi` are pushed in the middle of the function, where the clamps begin. Recompiled: the pointer goes to `ebp` in the branch, is reloaded into `ebp` right after the merged `set_gfx_mode` call (the path that skips the call keeps it), so `ebp` and `edi` are pushed at the start.

Shapes tried on 2026-10-27 (scratch comparator, `/O2`): the `if` and the `else` swapped; nested `if`s instead of `&&`; `mode == 0` / `!mode`; an empty `if` with the call in the `else`; a `MAP *` local for the clamps, or for the branch; the `big` test in a local; the `map_save` call in a comma expression; `card` initialised in its declaration; the `MIN` arguments swapped. All give the same allocation.

Shapes tried on 2026-10-28 (same allocation, 85.94%): the 4 or 6 lines of clamps in a `static __inline` function; the size test in an `__inline` function; nested `if`s; a `goto` past the clamps' block or a label; an empty or dead `else`; a `switch (mode)`; a mode variable then two calls; the size test through `(&DAT_00410110)[0]` or a `MAP *` local; `DAT_00410110` defined (or `static`) in the file; `acc8`/`acc4` through locals or a pointer. [verified] What decides it: with the size test on another global, the rest of the function matches (`push ebp` / `push edi` in the middle); with the clamps on constants, the branch value still goes to a register that is reloaded after the call (`edx`). So VC6 sees the clamps' load of `DAT_00410110` as partially redundant with the branch's load and keeps it on the path without the call (partial redundancy elimination). A store through an unknown pointer between the merge and the clamps (`*(int *)DAT_00422410 = 0;`, a kill) gives the original `eax` in the branch; the original has no such store, so the source shape that hides the redundancy is still unknown [hypothesis: something between the merge and the clamps that VC6 treats as a kill without emitting code].

## `auto_mask` (`0x00404500`), 96.86% (80.54% before 2026-10-28)

Current shape [verified, 2026-10-28]: `old = 0;` at the top next to `m = 0;` (the original zeroes `edx` there, before the `imul` of the index: the summer cover mask), the winter base mask saved in `old` (`ecx`), `m = MAX(m, old);` then the store from `m` (`mov [t + 2], bl`), and the tile indexed in each access (`DAT_00410110->tiles[i].t0`) rather than through a `TILE *` (97.56% on the object comparator, against 96.35% with the pointer). The prologue, both chains and the two ends now match.

One difference left: the winter tile address is in `esi` in the recompiled code, `edx` in the original. Both branches then begin with the same instructions (`mov ecx, [ecx + 0x88]` / `lea eax, [eax + eax*4]` / `push esi` / `lea esi, [ecx + eax]`), and VC6 merges them above the `je` (and the final store into one tail); the original keeps them in each branch because its registers differ.

Shapes tried on 2026-10-28 (scratch generators, about 150 combinations): the summer cover mask in `old` or a variable of its own, zeroed at the top (before or after the index, in a chained assignment, in the declarations) or in the branch; `m = MAX(m, old)`, `if (old > m) m = old`, `t->t2 = MAX(m, old)`; a `TILE *` in both branches, one, or none, a second pointer for the summer branch; the index recomputed in one branch or both; `x` reused as `m`, `y` as a mask; an early `return` instead of the `else`; `m = 0` inside the branches; the branches swapped (the layout follows the source: no); declaration order (24 random orders, no effect); the summer chain (identical for the base and cover tiles) as a `static __inline` function, worse (66-81%). [hypothesis] The original's register for the winter tile pointer comes from a shape not found yet.

The first version of this note follows (shapes tried on 2026-10-27).


Same control flow and constants (370 instructions on both sides in the best shape). Two allocation differences [verified]:
- the mask variable: `ebx` in the original (the register that held `x` before), `esi` in the recompiled code; the final `mov [t + 2], bl` becomes a copy through `eax`;
- the original zeroes `edx` at the start and uses it as the cover mask of the summer branch (`cmp ebx, edx` before the store); with a variable zeroed at the start, VC6 instead hoists the load of `DAT_00410110->tiles` above the test of `MAP.f79`, which the original does not do.

Shapes tried (`scratchpad` generator, 16 combinations, plus 14 others): index in a local, recomputed in every access, or stored back into `x` or `y`; a `TILE *` assigned in each branch or before; the cover mask in its own variable (zeroed at the top, in the declaration, or in the branch) or in the winter variable `old`; `MAX` arguments swapped; the parameters reused as the masks; an early `return` instead of the `else`. Kept: index in a local, a `TILE *` per branch, `old` reused as the summer cover mask (best score). [hypothesis] The original has a shape where the cover mask lives in a scratch register without the hoisting; not found.

## `pick_tile` (`0x00404B00`), 100% since 2026-10-28

[verified] The row is a counter multiplied in the call: `for (i = 0, row = -1; ...) { if (i % 16 == 0) row++; draw_rle_sprite(..., i % 16 * 18, row * 18); }`. VC6 strength-reduces `row * 18` and puts the start value (-18) after the loop guard, as the original does. `y = 0` with `y - 18` in the call also gives 100%; the counter was kept because it mirrors `i % 16 * 18`. Every `y = -18` form stayed at 97.83%; an explicit guard (`i = 0; if (i < n) { y = -18; for (; ...) }`) also gives 100% but is not a natural shape.

Before (2026-10-27): One instruction: `mov ebx, -18` (the row coordinate) is placed after the loop guard in the original, before it in the recompiled code. Tried: the initialisation in the `for`, before it, in the declaration, before `clear_bitmap`; a column variable; the declarations reordered; the counter reused as `my`. The test of the selection (`mx <= 15`, `my = my * 16 + mx` reusing `my`) matches.
