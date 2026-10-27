"""Read-only inspection of the formats of game/data/*.dat and game/maps/**/*.map.

.dat: encrypted Allegro packfile header + LZSS stream. Two variants observed:
  - "old": only the LZSS flag bytes are XORed with the password;
  - "new": the whole stream after the header is XORed with the password, header mask ^ 42.
The password tested is "happy" (string found in happy.exe and maped.exe).
The decompressed datafiles are written to captures/unpacked/; the list of objects
to notes/data/datafiles.csv.

.map: checks the hypothesis size = 7045 + 5 * W * H with W, H at bytes 1 and 2.

Usage: python -I scripts/inspect_formats.py
"""
import csv
import pathlib
import struct

ROOT = pathlib.Path(__file__).resolve().parent.parent
GAME = ROOT / "game"
UNPACKED = ROOT / "captures" / "unpacked"
CSV_OUT = ROOT / "notes" / "data" / "datafiles.csv"
PASSWORD = b"happy"
F_PACK_MAGIC = 0x736C6821  # "slh!"


def header_mask(pw):
    mask = 0
    for i, c in enumerate(pw):
        mask ^= c << ((i & 3) * 8)
    return mask


def unlzss(data, flag_key=None):
    n, f = 4096, 18
    ring = bytearray(n)
    r = n - f
    out = bytearray()
    flags = 0
    i = 0
    kpos = 0
    while True:
        flags >>= 1
        if not flags & 256:
            if i >= len(data):
                break
            c = data[i]
            i += 1
            if flag_key:
                c ^= flag_key[kpos]
                kpos = (kpos + 1) % len(flag_key)
            flags = c | 0xFF00
        if flags & 1:
            if i >= len(data):
                break
            c = data[i]
            i += 1
            out.append(c)
            ring[r] = c
            r = (r + 1) & (n - 1)
        else:
            if i + 1 >= len(data):
                break
            a, b = data[i], data[i + 1]
            i += 2
            a |= (b & 0xF0) << 4
            for k in range((b & 0x0F) + 3):
                c = ring[(a + k) & (n - 1)]
                out.append(c)
                ring[r] = c
                r = (r + 1) & (n - 1)
    return bytes(out)


def unpack_dat(raw):
    hdr = struct.unpack(">I", raw[:4])[0]
    mask = header_mask(PASSWORD)
    if hdr == F_PACK_MAGIC ^ mask:
        return "old", unlzss(raw[4:], PASSWORD)
    if hdr == F_PACK_MAGIC ^ mask ^ 42:
        body = bytes(c ^ PASSWORD[(k + 4) % len(PASSWORD)] for k, c in enumerate(raw[4:]))
        return "new", unlzss(body)
    return None, None


def tag(b):
    return b.decode("latin-1")


def parse_objects(buf, pos, prefix, rows, fname):
    count = struct.unpack(">I", buf[pos:pos + 4])[0]
    pos += 4
    for _ in range(count):
        props = {}
        while buf[pos:pos + 4] == b"prop":
            ptype = tag(buf[pos + 4:pos + 8])
            size = struct.unpack(">I", buf[pos + 8:pos + 12])[0]
            props[ptype] = buf[pos + 12:pos + 12 + size].decode("latin-1")
            pos += 12 + size
        otype = tag(buf[pos:pos + 4])
        size1, size2 = struct.unpack(">ii", buf[pos + 4:pos + 12])
        data_start = pos + 12
        name = prefix + props.get("NAME", "")
        rows.append({
            "file": fname, "name": name, "type": otype, "size": size2,
            "date": props.get("DATE", ""), "orig": props.get("ORIG", ""),
        })
        if otype == "FILE":
            parse_objects(buf, data_start, name + "/", rows, fname)
        pos = data_start + size1
    return pos


def main():
    UNPACKED.mkdir(parents=True, exist_ok=True)
    rows = []
    print("== datafiles")
    for p in sorted((GAME / "data").glob("*")):
        raw = p.read_bytes()
        variant, data = unpack_dat(raw)
        if data is None or data[:4] != b"ALL.":
            print(f"{p.name}: not recognised")
            continue
        (UNPACKED / p.name).write_bytes(data)
        n0 = len(rows)
        parse_objects(data, 4, "", rows, p.name)
        types = {}
        for r in rows[n0:]:
            types[r["type"]] = types.get(r["type"], 0) + 1
        print(f"{p.name}: encryption {variant}, {len(raw)} -> {len(data)} bytes, "
              f"{len(rows) - n0} objects {types}")
    with open(CSV_OUT, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)

    print("== maps (size = 7045 + 5*W*H?)")
    for p in sorted((GAME / "maps").rglob("*.map")):
        raw = p.read_bytes()
        w, h, sx, sy = raw[1], raw[2], raw[3], raw[4]
        name = raw[5:].split(b"\0", 1)[0].decode("latin-1")
        ok = len(raw) == 7045 + 5 * w * h
        print(f"{p.relative_to(GAME).as_posix():40} v={raw[0]} W={w:3} H={h:3} "
              f"b3={sx:3} b4={sy:3} {len(raw):6} {'OK' if ok else 'MISMATCH'}  {name!r}")


if __name__ == "__main__":
    main()
