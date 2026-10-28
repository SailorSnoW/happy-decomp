# STATUS

Last updated: 2026-10-28. The detailed log of each step is in the git history (commit messages and the earlier versions of this file).

## Current state

Matching decompilation: **the three exes are fully annotated and build without stubs**; 135 of 136 GAME functions match at 100% (`scripts/progress.py`, `notes/matching.md` §5).

| exe | GAME functions | at 100% | bytes at 100% | below 100% |
|---|---:|---:|---:|---|
| happy | 84 | 84 | 100% | — |
| maped | 50 | 49 | 98.1% | `set_screen_mode` 85.94% |
| loader | 2 | 2 | 100% | — |

- `reccmp-datacmp`: happy 72, maped 37, loader 1 variables, 0 issues. The CRT is annotated as `LIBRARY` (381 annotations, `scripts/crt_names.py`).
- End-of-array operands are compared by offset (`scripts/reccmp_run.py`, decided with the user on 2026-10-27, `notes/matching.md` §3).
- loader relinked without `/DEBUG`: `.text`, `.rdata`, `.data` byte-identical to the original.
- Ghidra is in sync with `src/` and can be rebuilt from a fresh import with `scripts/ghidra_sync.py --save happy maped loader` (`notes/matching.md` §3b). Not tested end to end on a fresh import.

## Remaining

- The function below 100% (maped `set_screen_mode`): `notes/open-diffs.md` (shapes already tried).
- Next phase per `CLAUDE.md`: the C17 port, verified by comparing its behaviour with the matching build.

## Open questions

Matching:
- `set_screen_mode`: which construct makes VC6 drop the partial redundancy between the branch's and the clamps' loads of `DAT_00410110` without emitting code?

Whole-exe layout (no effect on the function scores):
- happy's OLDNAMES aliases: the original has two (`AliasObj60 ×2`), the sources call three (`itoa`, `stricmp`, `strcmpi`); which spelling is wrong is not observable (`notes/matching.md` §3).
- maped `__ld12tod` (`0x0040A81F` in the original, `0x0040A809` rebuilt): the only CRT function of maped not at its original address.
- One .c file or several per block, the `"OK"` string of happy, `0x0040B376` (`__fpclear` or `__rttosnpop`): `notes/layout-happy.md` §9, `notes/matching.md` §4.

Behaviour found in the code, worth keeping in mind for the C17 port (details in the comments of `src/`):
- the `-fps` option of happy has no effect: its global is written and never read (`src/happy/data.c`, `DAT_00413b10`);
- `show_instructions` waits for `timer_ticks` without resetting it, so after the first tick the demonstration runs unthrottled [verified in the code, effect not observed] (`src/happy/block_07.c`);
- `controls[2]` and `controls[3]` are compared by `set_controls` but set by neither `default_settings` nor `set_controls` (`src/happy/block_06.c`);
- `FUN_00401160` copies `f14 = f13` from template `i` instead of `26 + i`; byte 256 of help text 0 is set to `'*'`, use unknown (`src/common/block_01.c`).

## History

| Date | Steps |
|---|---|
| 2026-10-27 | Environment check; game documentation (`notes/game-docs.md`); Allegro 4.0.3 identified (`notes/allegro.md`); inventory of happy, maped and loader (`notes/layout-*.md`, `notes/data/`); VC6 + `/O2` chosen on decomp.me and checked locally, reccmp end-to-end test (`notes/toolchain-vc6.md`); Allegro headers in `3rdparty/`; `dev-setup` skill; repository translated to English. |
| 2026-10-27 | Matching steps 1-17: infrastructure (`build.py`, `progress.py`, `crt_names.py`), happy block by block (map module and utilities in `src/common/`), `PLAYER` structure, maped, loader, Ghidra sync, end-of-array operands compared by offset. |
| 2026-10-28 | Steps 18-19: `bonus_room` and `pick_tile` at 100%, `auto_mask` 96.86%; the missing GAME functions created in Ghidra; `ghidra_sync.py` rebuilds the Ghidra project from git; `compile_commands.py` for clangd. |
| 2026-10-28 | Repository cleanup: removed the one-off inventory scripts and notes (decomp.me candidates, shared-code analysis, the resolved diff notes); the remaining diffs are in `notes/open-diffs.md`. |
| 2026-10-28 | `play_level` at 100% (the products through Allegro 3.x's `fmul` alias): happy 84/84. |
| 2026-10-28 | `auto_mask` at 100% (the winter base mask saved after the cover tile is read): maped 49/50. |
