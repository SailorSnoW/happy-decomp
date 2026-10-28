# STATUS

Last updated: 2026-10-28. The step-by-step log is in the git history.

## State: complete

Matching decompilation of the three exes: every game function is decompiled into C89 and recompiles with VC6 to the same machine code (`scripts/progress.py`, `notes/matching.md` §5).

| exe | game functions | at 100% |
|---|---:|---:|
| happy | 84 | 84 |
| maped | 50 | 50 |
| loader | 2 | 2 |

- `reccmp-datacmp`: happy 72, maped 37, loader 1 variables, 0 issues. The CRT is annotated as `LIBRARY` (381 annotations, `src/crt/`).
- End-of-array operands are compared by offset (`scripts/reccmp_run.py`, decided with the user on 2026-10-27, `notes/matching.md` §3).
- loader relinked without `/DEBUG`: `.text`, `.rdata`, `.data` byte-identical to the original.
- Ghidra can be rebuilt from git with `scripts/ghidra_sync.py --save happy maped loader` (`notes/matching.md` §3b); not tested end to end on a fresh import.

The matching decompilation is the end goal of the project (decided with the user on 2026-10-28; no port to modern C).

## Open questions

None affects a function score.

Publication (`notes/legal.md`): `src/` is a derived work of Johan Peitz's code (happy, maped) and Eric Love's (loader). Decided with the user on 2026-10-28: `src/` is published without asking them first. `README.md` and `LICENSE` (MIT, our own files only) are at the root.

Whole-exe layout:
- happy's OLDNAMES aliases: the original has two (`AliasObj60 ×2`), the sources call three (`itoa`, `stricmp`, `strcmpi`); which spelling is wrong is not observable (`notes/matching.md` §3).
- maped `__ld12tod` (`0x0040A81F` in the original, `0x0040A809` rebuilt): the only CRT function of maped not at its original address.
- One .c file or several per block, the `"OK"` string of happy, `0x0040B376` (`__fpclear` or `__rttosnpop`): `notes/layout-happy.md` §7, `notes/matching.md` §4.

Behaviour found in the code (details in the comments of `src/`):
- the `-fps` option of happy has no effect: its global is written and never read (`src/happy/data.c`, `DAT_00413b10`);
- `show_instructions` waits for `timer_ticks` without resetting it, so after the first tick the demonstration runs unthrottled [verified in the code, effect not observed] (`src/happy/block_07.c`);
- `controls[2]` and `controls[3]` are compared by `set_controls` but set by neither `default_settings` nor `set_controls` (`src/happy/block_06.c`);
- `FUN_00401160` copies `f14 = f13` from template `i` instead of `26 + i`; byte 256 of help text 0 is set to `'*'`, use unknown (`src/common/block_01.c`).
