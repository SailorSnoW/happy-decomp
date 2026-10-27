# maped: functions below 100%

State on 2026-10-27, `scripts/progress.py maped`: 50/50 GAME functions annotated, 42 at 100%. This note lists the 8 others with their reccmp diff. Five only differ by an end-of-array pointer (`notes/matching.md` §3); three by register allocation.

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

## `auto_mask` (`0x00404500`), 80.54%

Same control flow and constants (370 instructions on both sides in the best shape). Two allocation differences [verified]:
- the mask variable: `ebx` in the original (the register that held `x` before), `esi` in the recompiled code; the final `mov [t + 2], bl` becomes a copy through `eax`;
- the original zeroes `edx` at the start and uses it as the cover mask of the summer branch (`cmp ebx, edx` before the store); with a variable zeroed at the start, VC6 instead hoists the load of `DAT_00410110->tiles` above the test of `MAP.f79`, which the original does not do.

Shapes tried (`scratchpad` generator, 16 combinations, plus 14 others): index in a local, recomputed in every access, or stored back into `x` or `y`; a `TILE *` assigned in each branch or before; the cover mask in its own variable (zeroed at the top, in the declaration, or in the branch) or in the winter variable `old`; `MAX` arguments swapped; the parameters reused as the masks; an early `return` instead of the `else`. Kept: index in a local, a `TILE *` per branch, `old` reused as the summer cover mask (best score). [hypothesis] The original has a shape where the cover mask lives in a scratch register without the hoisting; not found.

## `pick_tile` (`0x00404B00`), 97.83%

One instruction: `mov ebx, -18` (the row coordinate) is placed after the loop guard in the original, before it in the recompiled code. Tried: the initialisation in the `for`, before it, in the declaration, before `clear_bitmap`; a column variable; the declarations reordered; the counter reused as `my`. The test of the selection (`mx <= 15`, `my = my * 16 + mx` reusing `my`) matches.
