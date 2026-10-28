# Legal review: what the repository may contain

Review of 2026-10-28, before the project is finalised. Sources: the text files shipped with the game (`game/readme.txt`, `game/maped.txt`, `game/loader.txt`), the strings of the binaries, the licence files of the third-party components, and the contents and history of the repository. This is an engineering review, not legal advice; the conclusions on the law are [hypothesis] throughout.

## 1. Who owns what

| Component | Rights holder | Source |
|---|---|---|
| `happy.exe` (code, texts) | Johan Peitz / Free Lunch Design, "(c) 2000 - 2003" | [verified] `readme.txt` (header, AUTHORS & CONTACT); string `"written by Johan Peitz (c) 2000"` in `happy.exe` (`0x004063A0`) |
| `maped.exe` | Johan Peitz, "(c) 2000 - 2002" | [verified] `maped.txt` header |
| `loader.exe` | **Eric Love**, a different author; no copyright notice, no licence | [verified] `loader.txt` ("written by Eric Love", "The loader was created by me (Eric Love)") |
| Graphics in `data/*.dat` | Johan Peitz (Christmas graphics) and **Ari Feldman's SpriteLib** (the rest) | [verified] `readme.txt` ("art by Johan Peitz and from Ari Feldman's SpriteLib"); SpriteLib licence number `129.16.30.119-952518357` in `readme.txt` and in `happy.exe` (`0x004063A0`) |
| Levels (`data/*levels.dat`, `maps/`) | Johan Peitz and Mark Hickford | [verified] `readme.txt` |
| Collision routine `FUN_00403E00` | possibly derived from PPCol (Ivan Baldo), licence unknown | [hypothesis] it only matched once written like PPCol's `check_pp_collision` (`notes/matching.md` §3); `readme.txt` thanks "Ivan Baldo for PPCOL". No licence text for PPCol was found online |
| `alleg40.dll`, Allegro headers | Allegro developers, **gift-ware** | [verified] `3rdparty/allegro-4.0.3/readme.txt` l. 149-159: "You may use, modify, redistribute, and generally hack it about in any way you like" |
| CRT statically linked in the three exes | Microsoft (VC6 `LIBC.LIB`) | [verified] byte identity with the local `LIBC.LIB` (`notes/matching.md` §4) |
| reccmp 0.1.7 (tool, not versioned) | isledecomp, **AGPL-3.0-or-later** | [verified] `License:` field of the installed `reccmp-0.1.7.dist-info/METADATA` |

## 2. What the game's licence allows

[verified] `readme.txt`, section DISTRIBUTING: the game is freeware; "you are encouraged to distribute this game (in it's original form)"; it "may not be included in a compilation or any other kind of commercial package without the permission from the author". Nothing else: no word on modification, reverse engineering, source code or derived works. `maped.txt` and `loader.txt` contain only a disclaimer.

[hypothesis: seen in a web search summary, the page itself refused the connection on 2026-10-28] The allegro.cc depot entry of the Xmas Edition, a community listing, says "Source Code: Not Available" (<https://www.allegro.cc/depot/HappylandAdventures-XmasEdition>). For comparison, another game by the same author, Alex 4, is packaged by Debian under GPL-2+ (<https://sources.debian.org/copyright/license/alex4/1.1-10>), which suggests the author is open to releasing source [hypothesis].

So the freeware licence grants the right to copy and play the unmodified game, not the right to publish a derived work of its code.

## 3. Legal frame [hypothesis]

- EU (Directive 2009/24/EC, art. 5(3) and 6; in France, CPI art. L122-6-1): studying a program and decompiling it are allowed in narrow cases (observing its behaviour; decompiling for interoperability), and the information obtained may not be communicated to others beyond what interoperability requires. **No exception covers publishing a matching decompilation**: C written to compile to the same machine code reproduces the expression of the program, so it is an adaptation of the work and needs the author's permission. US law reaches the same point by a different route (fair use is uncertain at best).
- Practice: public matching decompilations (Super Mario 64, Ocarina of Time, LEGO Island, where reccmp comes from) publish the code without binaries or assets and rely on the rights holder's tolerance. That is a practice, not a right; the usual consequence is a takedown request.
- Facts, addresses, hashes, names chosen by us, and our own analysis are not protected; short attributed quotations are allowed (CPI art. L122-5, 3° a).

## 4. Contents of the repository

[verified] `git log --all --diff-filter=A --name-only`: no game binary, data file, map, Ghidra project or build output was ever committed; every file in the history is source, notes, scripts or Allegro text.

| Item | Contents | Verdict |
|---|---|---|
| `src/common`, `src/happy`, `src/maped` | decompiled code of Johan Peitz's programs, including their strings (credits, passwords, hiscore names, help texts, SpriteLib licence number) | **derived work: needs Johan Peitz's permission** to be published cleanly. Cannot carry a licence of ours |
| `src/loader` | decompiled code of Eric Love's program | **derived work: needs Eric Love's permission** |
| `src/crt/*.h` | names and addresses of CRT functions | OK (facts) |
| `notes/*.md`, `notes/data/*.csv` | own analysis; addresses, sizes, short strings; `datafiles.csv` lists the object names of the datafiles and the author's original paths (`c:\djgpp\fld\...`) | OK. `game-docs.md` quotes 19 lines of `readme.txt` with attribution: keep it to short quotations |
| `reccmp-project.yml`, `reccmp/*.csv` | SHA-256 of the originals, addresses | OK (facts) |
| `3rdparty/allegro-4.0.3/` | Allegro headers and `.def`, licence and `AUTHORS` included | OK (gift-ware). Keep `readme.txt` and `AUTHORS` next to the headers |
| `scripts/` | own code. `reccmp_run.py` imports reccmp and replaces three methods at run time, without copying reccmp code [verified: only wrappers around `difflib` and reccmp's own `_compare_function_assembly`] | OK. If a licence is chosen for it, it must be compatible with AGPL-3.0 (MIT, BSD, GPL-3.0, AGPL-3.0 all are) |
| `.claude/`, `CLAUDE.md`, `flake.nix`, `.mcp.json` | own files; `flake.nix` only holds a URL and hash of a Wine build | OK |

Must never enter the repository (all already ignored by `.gitignore`, except where noted):
- `game/`: the exes, the datafiles (code and art of Johan Peitz, SpriteLib art licensed per registration), the maps, the full text files, `unins000.*` (installer log of the user's machine). The game may be shared "in its original form", but a git repository of another project is not that form;
- `build/`: a rebuilt exe is a full copy of the game code plus Microsoft's CRT;
- VC6 (`tools/`, `LIBC.LIB`): Microsoft licence;
- the Ghidra project and `captures/`: they contain the binaries.
- `*.map` and `*.pcx` (levels and screenshots) are ignored too, as a safety net outside `game/` (added on 2026-10-28; the linker map files of `build.py` are under `build/` anyway).

## 5. Before publishing

**Decision (with the user, 2026-10-28): `src/` is published**, all three exes included, without asking the authors first.

1. Asking the authors stays possible later and would settle the status of `src/`: Johan Peitz (contact in `readme.txt`: info at freelunchdesign.com; the domain has not been checked) for `happy` and `maped`, ideally with a licence (e.g. GPL-2+, as for Alex 4); Eric Love for `loader` (address of 2003 in `loader.txt`, probably dead).
2. **README.md** at the root [done 2026-10-28]: no game file is included and each user supplies their own copy (with the SHA-256 of `reccmp-project.yml`); credits (Johan Peitz / Free Lunch Design, Mark Hickford, Eric Love, Ari Feldman's SpriteLib, Ivan Baldo's PPCol, the Allegro developers); no affiliation with the authors; contact for removal requests.
3. **LICENSE** [done 2026-10-28, chosen with the user]: no licence can be granted on `src/` without the authors' permission. MIT (compatible with AGPL-3.0, §4) for our own files only, with an explicit scope that excludes `src/` and `3rdparty/`.
4. Keep the safeguards: never `git add -f` under `game/`, `captures/` or `build/`; check `git status` before each push.
