#!/usr/bin/env python3
# font-facts.py — read the test fonts' own tables and write what the tests expect of FreeType to tests/src/facts.kama.
#
#   python3 -I tools/font-facts.py           # (re)write tests/src/facts.kama
#   python3 -I tools/font-facts.py --check   # exit 1 when tests/src/facts.kama is not what the fonts say
#
# The expectations come from the fonts, by script, never typed from memory (AGENTS.package.md, "tests/ is one
# program"). This reads the SFNT tables directly, with no font library, so FreeType is checked against the file rather
# than against itself: units per em ('head'), the line metrics FreeType reports (sfobjs.c: the OS/2 typo metrics when
# fsSelection bit 7 USE_TYPO_METRICS is set, else 'hhea'), the glyph count ('maxp'), code point to glyph ('cmap'
# format 4 or 12, Windows Unicode), advances ('hmtx') and kerning pairs ('kern' format 0).
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FONTS = os.path.join(ROOT, "tests", "fonts")
OUT = os.path.join(ROOT, "tests", "src", "facts.kama")


class Font:
    def __init__(self, path):
        with open(path, "rb") as f:
            self.data = f.read()
        self.tag = self.data[:4]
        n = struct.unpack(">H", self.data[4:6])[0]
        self.tables = {}
        for i in range(n):
            tag, _, off, length = struct.unpack(">4sIII", self.data[12 + 16 * i:28 + 16 * i])
            self.tables[tag.decode("latin-1")] = (off, length)

    def table(self, tag):
        off, length = self.tables[tag]
        return self.data[off:off + length]

    def upm(self):
        return struct.unpack(">H", self.table("head")[18:20])[0]

    def glyphs(self):
        return struct.unpack(">H", self.table("maxp")[4:6])[0]

    def line(self):
        os2 = self.table("OS/2") if "OS/2" in self.tables else None
        if os2 is not None and struct.unpack(">H", os2[62:64])[0] & 128:
            asc, desc, gap = struct.unpack(">hhh", os2[68:74])
        else:
            asc, desc, gap = struct.unpack(">hhh", self.table("hhea")[4:10])
        return asc, desc, asc - desc + gap

    def advance(self, glyph):
        n = struct.unpack(">H", self.table("hhea")[34:36])[0]
        hmtx = self.table("hmtx")
        i = min(glyph, n - 1)
        return struct.unpack(">H", hmtx[4 * i:4 * i + 2])[0]

    def glyph(self, codepoint):
        cmap = self.table("cmap")
        n = struct.unpack(">H", cmap[2:4])[0]
        subs = {}
        for i in range(n):
            pid, eid, off = struct.unpack(">HHI", cmap[4 + 8 * i:12 + 8 * i])
            subs[(pid, eid)] = off
        if (3, 10) in subs:
            return self._format12(cmap[subs[(3, 10)]:], codepoint)
        return self._format4(cmap[subs[(3, 1)]:], codepoint)

    @staticmethod
    def _format12(t, cp):
        assert struct.unpack(">H", t[0:2])[0] == 12
        groups = struct.unpack(">I", t[12:16])[0]
        for g in range(groups):
            start, end, first = struct.unpack(">III", t[16 + 12 * g:28 + 12 * g])
            if start <= cp <= end:
                return first + cp - start
        return 0

    @staticmethod
    def _format4(t, cp):
        assert struct.unpack(">H", t[0:2])[0] == 4
        segs = struct.unpack(">H", t[6:8])[0] // 2
        ends = struct.unpack(">%dH" % segs, t[14:14 + 2 * segs])
        o = 16 + 2 * segs
        starts = struct.unpack(">%dH" % segs, t[o:o + 2 * segs])
        deltas = struct.unpack(">%dh" % segs, t[o + 2 * segs:o + 4 * segs])
        ro = o + 4 * segs
        ranges = struct.unpack(">%dH" % segs, t[ro:ro + 2 * segs])
        for s in range(segs):
            if starts[s] <= cp <= ends[s]:
                if ranges[s] == 0:
                    return (cp + deltas[s]) & 0xFFFF
                at = ro + 2 * s + ranges[s] + 2 * (cp - starts[s])
                g = struct.unpack(">H", t[at:at + 2])[0]
                return (g + deltas[s]) & 0xFFFF if g else 0
        return 0

    def kern(self, left, right):
        if "kern" not in self.tables:
            return None
        t = self.table("kern")
        version, n = struct.unpack(">HH", t[0:4])
        assert version == 0
        off = 4
        for _ in range(n):
            _, length, coverage = struct.unpack(">HHH", t[off:off + 6])
            if coverage >> 8 == 0 and coverage & 1:   # format 0, horizontal
                pairs = struct.unpack(">H", t[off + 6:off + 8])[0]
                for p in range(pairs):
                    l, r, v = struct.unpack(">HHh", t[off + 14 + 6 * p:off + 20 + 6 * p])
                    if (l, r) == (left, right):
                        return v
            off += length
        return 0


def facts():
    lato = Font(os.path.join(FONTS, "LatoLatin-Regular.ttf"))
    scp = Font(os.path.join(FONTS, "SourceCodePro-Regular.otf"))
    assert lato.tag == b"\x00\x01\x00\x00" and scp.tag == b"OTTO"
    la, ld, lh = lato.line()
    sa, sd, sh = scp.line()
    A, T, o, W, i = ord("A"), ord("T"), ord("o"), ord("W"), ord("i")
    lines = [
        ("LATO_UPM", lato.upm()),
        ("LATO_ASCENDER", la),
        ("LATO_DESCENDER", ld),
        ("LATO_HEIGHT", lh),
        ("LATO_GLYPHS", lato.glyphs()),
        ("LATO_GLYPH_A", lato.glyph(A)),
        ("LATO_GLYPH_T", lato.glyph(T)),
        ("LATO_GLYPH_O", lato.glyph(o)),
        ("LATO_ADVANCE_A", lato.advance(lato.glyph(A))),
        ("LATO_KERN_TO", lato.kern(lato.glyph(T), lato.glyph(o))),
        ("LATO_KERN_AA", lato.kern(lato.glyph(A), lato.glyph(A))),
        ("SCP_UPM", scp.upm()),
        ("SCP_ASCENDER", sa),
        ("SCP_DESCENDER", sd),
        ("SCP_GLYPHS", scp.glyphs()),
        ("SCP_GLYPH_A", scp.glyph(A)),
        ("SCP_GLYPH_W", scp.glyph(W)),
        ("SCP_GLYPH_I", scp.glyph(i)),
        ("SCP_ADVANCE", scp.advance(scp.glyph(A))),
    ]
    # The monospaced face is monospaced: the tests lean on it.
    assert scp.advance(scp.glyph(W)) == scp.advance(scp.glyph(i)) == scp.advance(scp.glyph(A))
    assert lato.kern(lato.glyph(T), lato.glyph(o)) < 0, "Lato kerns T-o"
    out = ["// GENERATED by tools/font-facts.py from the test fonts' own tables. Do not edit: re-run the script.",
           "// What FreeType must report for tests/fonts/LatoLatin-Regular.ttf (TrueType) and SourceCodePro-Regular.otf",
           "// (OpenType CFF): design units, glyph indices, advances and kerning, read with no font library.",
           "",
           "export { %s };" % ", ".join(n for n, _ in lines),
           ""]
    for name, value in lines:
        kind = "uint32" if "GLYPH_" in name and name not in ("LATO_GLYPHS", "SCP_GLYPHS") else "int32"
        out.append("comptime %s %s = %d;" % (kind, name, value))
    return "\n".join(out) + "\n"


def main():
    text = facts()
    if sys.argv[1:] == ["--check"]:
        have = open(OUT).read() if os.path.exists(OUT) else ""
        if have != text:
            sys.stderr.write("font-facts: tests/src/facts.kama is stale: run python3 -I tools/font-facts.py\n")
            sys.exit(1)
        return
    with open(OUT, "w") as f:
        f.write(text)
    print("font-facts: wrote tests/src/facts.kama")


main()
