"""reccmp-reccmp with pointers just outside a variable compared by their offset.

Usage: same arguments as reccmp-reccmp, run with the Python where reccmp 0.1.7
is installed (the global Python 3.12, not .venv):
    python scripts/reccmp_run.py --target HAPPY --verbose 0x00406b80
    python scripts/reccmp_run.py --audit --target HAPPY --silent --json x.json
scripts/progress.py runs it in place of reccmp-reccmp. --audit (removed from
the arguments before reccmp sees them) prints each pair of lines it treated as
equal, to stderr, for review.

Why (decided with the user on 2026-10-27, notes/matching.md §3): a loop over a
whole global array compiles to `cmp reg, &array[N].field`, an address past the
end of the array (or `&array[0].field - 1`, just before it). reccmp names an
address after the variable that contains it, which is whatever the linker put
next to the array: another variable in each exe (`scroll_text[10]` in the
original, `hiscores[0].name+10` in the rebuild), so identical code counts as a
difference. The order of the uninitialised globals depends on their names
(COMMON, notes/matching.md §3), which are not the original ones.

What it changes: after reccmp has turned both listings into text, each pair of
lines that the diff aligns one to one and that differ only by address operands
is checked. Two operands are the same when their addresses are at the same
offset from one matched variable (`orig - var.orig == recomp - var.recomp`),
at most BOUND bytes before its start or after its end. Both are then written
`var+offset (OFFSET)` (e.g. `DAT_00414140+0xb0a (OFFSET)`). Anything else
(another register, another instruction, an address that is not at the same
offset of the same variable) is left to reccmp. The reccmp package is not
modified: the patch is applied at run time, to reccmp 0.1.7 only.

Scope [verified 2026-10-27, --audit on HAPPY and MAPED]: in the game code, only
end-of-array operands (`cmp`) and `hiscores - 1` (enter_hiscore). In the CRT
(LIBRARY, not counted by progress.py), also the unannotated CRT variables next
to errno and wind (`__doserrno` = errno+0x4...), which are at the same offset in
both exes. Residual risk: a wrong global in the rebuild that happens to sit at
the same distance from the same variable as the right one in the original
would pass; such a mistake also shows elsewhere (reccmp-datacmp, other uses).
"""
import difflib
import importlib.metadata
import re
import sys

from reccmp.compare import functions as rc_functions
from reccmp.types import EntityType, ImageId

# Largest distance outside a variable that is still compared by offset: one
# element past the end of an array of structures, or a field before its start.
BOUND = 64
HEX = re.compile(r"\b0x[0-9a-f]+\b")

_init = rc_functions.FunctionComparator.__post_init__
_compare = rc_functions.FunctionComparator.compare_function
_assembly = rc_functions.FunctionComparator._compare_function_assembly


def _recording(lookup, seen):
    """Wraps a name lookup so that every name it returns maps back to its address."""
    def wrapper(addr, exact=False, indirect=False):
        name = lookup(addr, exact=exact, indirect=indirect)
        if name is not None and not indirect:
            seen.setdefault(name, addr)
        return name
    return wrapper


def __post_init__(self):
    _init(self)
    self._seen = {ImageId.ORIG: {}, ImageId.RECOMP: {}}
    self.orig_sanitize.name_lookup = _recording(
        self.orig_sanitize.name_lookup, self._seen[ImageId.ORIG])
    self.recomp_sanitize.name_lookup = _recording(
        self.recomp_sanitize.name_lookup, self._seen[ImageId.RECOMP])
    self._variables = None


def compare_function(self, match):
    for seen in self._seen.values():
        seen.clear()
    return _compare(self, match)


def _names(self, image_id, sanitize):
    """name -> address for the function just parsed on one side."""
    names = dict(self._seen[image_id])
    for addr, name in sanitize.replacements.items():
        names.setdefault(name, addr)
    return names


def _variables_list(self):
    if self._variables is None:
        self._variables = []
        for ent in self.db.get_matches_by_type(EntityType.DATA):
            size = ent.any_size(ImageId.ORIG)
            name = ent.best_name()
            if size and name:
                self._variables.append((ent.orig_addr, ent.recomp_addr, size, name))
    return self._variables


def _same_offset(self, orig_addr, recomp_addr):
    """Name of the operand when both addresses are at the same offset of one
    matched variable, else None."""
    best = None
    for orig, recomp, size, name in _variables_list(self):
        offset = orig_addr - orig
        if recomp_addr - recomp != offset or not -BOUND <= offset < size + BOUND:
            continue
        outside = -offset if offset < 0 else max(0, offset - size + 1)
        if best is None or outside < best[0]:
            best = (outside, name, offset)
    if best is None:
        return None
    _, name, offset = best
    sign = "-" if offset < 0 else "+"
    return "%s%s%#x (OFFSET)" % (name, sign, abs(offset))


def _split(line, names):
    """Splits an instruction into its text and its address operands."""
    keys = sorted(names, key=len, reverse=True)
    pattern = "|".join([re.escape(k) for k in keys] + [HEX.pattern])
    return re.split("(%s)" % pattern, line)


def _address(token, names):
    if token in names:
        return names[token]
    if HEX.fullmatch(token):
        return int(token, 16)
    return None


AUDIT = False


def _same_line(self, orig_line, recomp_line, orig_names, recomp_names):
    o = _split(orig_line, orig_names)
    r = _split(recomp_line, recomp_names)
    if len(o) != len(r):
        return None
    out = []
    changed = False
    for k, (a, b) in enumerate(zip(o, r)):
        if k % 2 == 0 or a == b:
            if a != b:
                return None
            out.append(a)
            continue
        oa, ra = _address(a, orig_names), _address(b, recomp_names)
        if oa is None or ra is None:
            return None
        name = _same_offset(self, oa, ra)
        if name is None:
            return None
        out.append(name)
        changed = True
    return "".join(out) if changed else None


def _compare_function_assembly(self, orig, recomp, split_points):
    orig_names = _names(self, ImageId.ORIG, self.orig_sanitize)
    recomp_names = _names(self, ImageId.RECOMP, self.recomp_sanitize)
    orig_text = [x[1] for x in orig]
    recomp_text = [x[1] for x in recomp]
    sm = difflib.SequenceMatcher(None, orig_text, recomp_text, autojunk=False)
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag != "replace" or i2 - i1 != j2 - j1:
            continue
        for k in range(i2 - i1):
            line = _same_line(self, orig_text[i1 + k], recomp_text[j1 + k],
                              orig_names, recomp_names)
            if line is not None:
                if AUDIT:
                    print("AUDIT %s | %s | %s" % (orig_text[i1 + k],
                          recomp_text[j1 + k], line), file=sys.stderr)
                orig[i1 + k] = (orig[i1 + k][0], line)
                recomp[j1 + k] = (recomp[j1 + k][0], line)
    return _assembly(self, orig, recomp, split_points)


def patch():
    version = importlib.metadata.version("reccmp")
    if version != "0.1.7":
        sys.exit("reccmp_run.py was written for reccmp 0.1.7, found %s" % version)
    cls = rc_functions.FunctionComparator
    cls.__post_init__ = __post_init__
    cls.compare_function = compare_function
    cls._compare_function_assembly = _compare_function_assembly


if __name__ == "__main__":
    if "--audit" in sys.argv:
        sys.argv.remove("--audit")
        AUDIT = True
    patch()
    from reccmp.tools.asmcmp import main
    sys.exit(main())
