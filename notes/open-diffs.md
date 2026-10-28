# Functions below 100%

State on 2026-10-28 (`scripts/progress.py`): happy 84/84, maped 49/50, loader 2/2. The function left differs by register allocation only; the shapes already tried are listed so that they are not tried again. The shapes that did work elsewhere are in `notes/matching.md` §3a.

## maped `set_screen_mode` (`0x00401A20`), 85.94%

Same instructions except the register of the map pointer in the 640x480 branch. Original: after `map_save`, `mov eax, [DAT_00410110]` and the two compares use `eax`; `ebp` and `edi` are pushed in the middle of the function, where the clamps begin. Recompiled: the pointer goes to `ebp` in the branch, is reloaded into `ebp` right after the merged `set_gfx_mode` call (the path that skips the call keeps it), so `ebp` and `edi` are pushed at the start.

[verified] Mechanism: with the size test on another global, the rest of the function matches (`push ebp` / `push edi` in the middle); with the clamps on constants, the branch value still goes to a register that is reloaded after the call (`edx`). So VC6 sees the clamps' load of `DAT_00410110` as partially redundant with the branch's load and keeps it on the path without the call (partial redundancy elimination). A store through an unknown pointer between the merge and the clamps (`*(int *)DAT_00422410 = 0;`, a kill) gives the original `eax` in the branch; the original has no such store. [hypothesis] Something between the merge and the clamps that VC6 treats as a kill without emitting code.

Tried without effect (same allocation, 85.94%):
- 2026-10-27: the `if` and the `else` swapped; nested `if`s instead of `&&`; `mode == 0` / `!mode`; an empty `if` with the call in the `else`; a `MAP *` local for the clamps, or for the branch; the `big` test in a local; the `map_save` call in a comma expression; `card` initialised in its declaration; the `MIN` arguments swapped;
- 2026-10-28: the 4 or 6 lines of clamps in a `static __inline` function; the size test in an `__inline` function; nested `if`s; a `goto` past the clamps' block or a label; an empty or dead `else`; a `switch (mode)`; a mode variable then two calls; the size test through `(&DAT_00410110)[0]` or a `MAP *` local; `DAT_00410110` defined (or `static`) in the file; `acc8`/`acc4` through locals or a pointer.
