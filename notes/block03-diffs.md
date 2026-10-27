# happy block_03: functions below 100% for a reason other than §3

Functions of `src/happy/block_03.c` whose reccmp diff (2026-10-27) contains something other than an end-of-array pointer or a call to an undecompiled function (`notes/matching.md` §3).

## `draw_level` (`0x00402620`, 2,166 bytes): 90.32%, then 100%

**Resolved on 2026-10-27** [verified: reccmp 100%, scratch comparator 693/693 instructions]. Three changes, all needed (each one alone left 87.8%-98.6%):
1. **no `x` / `y` variables**: the tile coordinates are written `i * 16 - sx` and `j * 16 - sy` in the calls, with plain loops `for (i = 0; i < 21; i++)` / `for (j = 0; j < 14; j++)`. VC6 creates the induction variables for `i * 16` and `j * 16` itself and replaces the tests of `i` and `j` by `cmp ..., 0x150` and `cmp ..., 0xe0`, as in the original. Its own temporaries take stack slots after the user variables, which gives the original slots (`x` at -0x18 between `sy` and `sx`); every form with an `x` variable (`x = i * 16` in the body, `x` in the `for` header, test on `x` or on `i`) kept `x` at -8 or -0xC;
2. the front tile loop also computes its index into `k` (`k = (ty + j) * DAT_00414130->w + i + tx;`) like the back one, instead of repeating the expression three times;
3. the HUD: `len = text_length(font, buf);` then `textout(..., 318 - len, ...)`: with the call nested in the argument, VC6 pushes `-1` and `2` before calling `text_length` and pops the stack of `itoa` first; with the local, the five calls (`itoa`, `text_length`, `textout`, `itoa`, `text_length`) leave their arguments until one `add esp, 0x40`, as in the original. Which variable holds the length is not observable (every local tried gives the same code).

The last two differences were the calls to `itoa` (`0x00402DBD`, `0x00402E25`): the CRT annotation said `__itoa`, reccmp names the recompiled target `_itoa`, the OLDNAMES alias that the source calls (`matching.md` §3, §4).

History (before the fix, 90.32%):

The control flow, the calls and their arguments are the same as the original's [verified: 693 instructions on both sides, `cmpobj` scratch comparator with relocations masked]. What differs:
1. **stack slots**: the original has `ox` -4, `oy` -8, `j` -0xC, `i` -0x10, `sy` -0x14, `x` -0x18, `sx` -0x1C, `ty` -0x20, `tx` -0x24, the tile index -0x28, `buf` -0x3C; the recompiled code puts `x` at -8 and `oy` at -0xC, `j` at -0x1C (and so on). Neither the declaration order nor the names change the slots [verified: permutations and renames give the same code]; they follow from the code itself;
2. **the outer loop of the two tile loops**: the original compares `x` with 336 (`cmp eax, 0x150`), the kept form compares `i` with 21 (`for (x = 0, i = 0; i < 21; x += 16, i++)`). The forms that compare `x` (`x < 336`, with the increments in either order) add 4 instructions elsewhere and give 90.07% instead of 91.63% (comparator ratio);
3. the HUD part: the stack adjustments after `textprintf` / `textprintf_centre` / `text_length` are grouped differently (`add esp, 0x38` and `0x40` in the original), and the constant 2 is not kept in `edi`, nor 2 in `bl` for the two tests of `f79`: consequences of the register allocation.

What was found on the way [verified]:
- the inner tile loop is controlled by `j` (`for (j = 0; j < 14; j++) { y = j * 16; ...`): VC6 replaces the test by `y < 224` (linear function test replacement) and keeps `j`, used in the tile index, as the original does. With `for (y = 0, j = 0; y < 224; y += 16, j++)`, VC6 merges `ty + j` into a new induction variable and drops `j` (80% instead of 90%);
- the tile index is a local (`k = (ty + j) * DAT_00414130->w + i + tx`): the original keeps `k * 5` in a stack slot across the calls;
- the camera is `MIN(MAX(fixtoi(player.x) - 159, 0), (w - 20) * 16)`, as in `kill_player` (the `fixtoi` calls are repeated by the macros);
- the function has an `ebp` frame because `fixtoi` uses the inline asm of `fixfloor` (`al386vc.h`).

To retry: the slot order, which probably follows the weights of the variables, and a form of the outer loop that compares `x` without the 4 extra instructions.
