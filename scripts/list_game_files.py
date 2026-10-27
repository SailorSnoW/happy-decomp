"""Read-only inventory of game/data and game/maps.

For each file: path, size, extension, first 32 bytes in hex,
ASCII magic if any. Writes notes/data/game-files.csv and prints the table.
Modifies nothing in game/.

Usage: python -I scripts/list_game_files.py
"""
import csv
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
GAME = ROOT / "game"
OUT = ROOT / "notes" / "data" / "game-files.csv"
DIRS = ["data", "maps"]


def ascii_preview(b):
    return "".join(chr(c) if 32 <= c < 127 else "." for c in b)


def main():
    rows = []
    for d in DIRS:
        for p in sorted((GAME / d).rglob("*")):
            if not p.is_file():
                continue
            with open(p, "rb") as f:
                head = f.read(32)
            rows.append({
                "path": p.relative_to(GAME).as_posix(),
                "size": p.stat().st_size,
                "ext": p.suffix.lower(),
                "head32_hex": head.hex(" "),
                "head32_ascii": ascii_preview(head),
            })
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with open(OUT, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    for r in rows:
        print(f"{r['path']:45} {r['size']:>8}  {r['head32_hex']}  |{r['head32_ascii']}|")


if __name__ == "__main__":
    main()
