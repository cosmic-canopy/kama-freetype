#!/bin/sh
# vendor-freetype.sh — pin one FreeType release into third_party/freetype/, run the licence gate over it, and
# regenerate what is derived from it: the wrapper translation units in csrc/ft/, csrc/ft/kft_ftmodule.h, the
# fthash notice in licenses/, and the `csources` block of kama.json.
#
# Vendored on purpose. A consumer of @kama/freetype gets a working build with nothing to install, on every
# target kama has (wasm has no system libraries, and a package cannot add `-I/opt/homebrew/include` for its
# consumer), and a shipped binary links one known FreeType with one known option set rather than whichever
# one a machine had: friendly-fire-department measured MSYS2's FreeType pulling 14 more DLLs, some LGPL, into
# an .exe. Upgrading is a change to VERSION, SHA256 and COMMIT, a run, a read of the diff, and a version bump.
#
# The release tarball, not a git snapshot. Its kept files are byte-identical to COMMIT, the tag VER-2-14-3
# that the-mallow-man's and friendly-fire-department's tools/fetch-freetype.sh pin (compared when this pin was
# set; the tarball adds only src/dlg and include/dlg, which are not kept).
#
# What lands, each file byte-identical to the release:
#   third_party/freetype/LICENSE.TXT, docs/FTL.TXT    the licence statement and the licence TAKEN (the FTL)
#   third_party/freetype/include/ft2build.h, include/freetype/   the public and internal headers
#   third_party/freetype/src/<module>/                the `.c` and `.h` files of each module in MODULES
# What does NOT: the other modules (Type 1, CID, PFR, Type 42, WinFNT, PCF, BDF, the mono rasteriser, SDF,
# OT-SVG, the cache), the optional dependencies' glue (src/gzip is zlib, src/bzip2, src/lzw), docs/ bar
# FTL.TXT (so docs/GPLv2.TXT never lands), the build systems, the tests, and dlg. And the files the licence
# gate refuses, below.
#
# THE LICENCE GATE. FreeType is dual-licensed (LICENSE.TXT): the FreeType License or GPLv2, the user's choice.
# This package takes the FTL, never GPLv2. Every kept source file must say it is under "the FreeType project
# license"; the only other notice allowed is an EXCEPTIONS entry, a permissive licence that LICENSE.TXT names
# and that ships with its notice in licenses/. A file under any other notice, or under none, is REFUSED: it is
# not vendored, and the build compiles without it (a refused file that a kept unit needs fails the build by
# name). A candidate that is neither FTL, an exception nor already on the REFUSED list FAILS the run, as does
# any kept file that carries copyleft text. So an upgrade cannot bring a new licence in unread.
#
# The FreeType build must not leak into the consumer. FreeType compiles with FT2_BUILD_LIBRARY and with this
# package's option and module lists, and a `cflags` entry would reach every translation unit of every
# consumer. So each FreeType compilation unit is compiled through a WRAPPER in csrc/ft/ that includes
# csrc/kft_build.h (which defines all three) and then the pristine `.c`. Consumers get only `include/`,
# through `cincludes`, with FreeType's default public configuration. Only csrc/ and csrc/ft/ hold `.c` files,
# so they are the only directories kama puts on every translation unit's include path.
#
# Generated here, never hand-edited: everything under third_party/freetype/, csrc/ft/, licenses/FreeType-
# fthash-MIT.txt, and the `csources` block of kama.json (from its `"csources": [` line to the `]` that closes
# it). Everything else in the manifest is hand-written and left alone. Needs curl, xz, python3.
set -eu

VERSION=2.14.3
SHA256=36bc4f1cc413335368ee656c42afca65c5a3987e8768cc28cf11ba775e785a5f
URL="https://download.savannah.gnu.org/releases/freetype/freetype-${VERSION}.tar.xz"
COMMIT=0a0221a1347e2f1e07c395263540026e9a0aa7c7   # refs/tags/VER-2-14-3^{}

# The modules TrueType and OpenType text needs: the drivers for glyf (truetype) and CFF (cff, with psaux and
# pshinter) outlines, the SFNT tables, names, the auto-hinter, and the anti-aliased rasteriser.
MODULES="base truetype sfnt smooth psnames autofit cff psaux pshinter"

# FreeType's compilation units, as its own CMakeLists.txt names them: the core (ftsystem, ftinit, ftdebug,
# ftbase), the optional base APIs whose fonts are kept, and one amalgamation per module. Each becomes one
# wrapper in csrc/ft/.
UNITS="src/base/ftsystem.c src/base/ftinit.c src/base/ftdebug.c src/base/ftbase.c
       src/base/ftbbox.c src/base/ftbitmap.c src/base/ftfstype.c src/base/ftgasp.c src/base/ftglyph.c
       src/base/ftmm.c src/base/ftstroke.c src/base/ftsynth.c src/base/fttype1.c
       src/truetype/truetype.c src/sfnt/sfnt.c src/smooth/smooth.c src/psnames/psnames.c src/autofit/autofit.c
       src/cff/cff.c src/psaux/psaux.c src/pshinter/pshinter.c"

# Refused by the gate, each for the reason given; none is compiled in this configuration.
#   md5.c, md5.h                public domain (Solar Designer); compiled only with FT_DEBUG_LEVEL_TRACE, which
#                               csrc/kft_ftoption.h forces off
#   ft-hb-ft.c/.h, ft-hb-decls.h, ft-hb-types.h, hb-script-list.h
#                               HarfBuzz's "Old MIT"; compiled only with FT_CONFIG_OPTION_USE_HARFBUZZ, forced
#                               off. autofit.c includes ft-hb-ft.c whatever the option, so its wrapper lists
#                               autofit.c's other files instead (see the generator below)
#   config/ftmodule.h           states no licence in the file (the FTL's section 0 covers it all the same); FreeType's
#                               all-modules list, which csrc/ft/kft_ftmodule.h replaces
#   ftchapters.h                states no licence; the API reference's chapter list, included by nothing
REFUSED="src/base/md5.c src/base/md5.h
         src/autofit/ft-hb-ft.c src/autofit/ft-hb-ft.h src/autofit/ft-hb-decls.h src/autofit/ft-hb-types.h
         src/autofit/hb-script-list.h
         include/freetype/config/ftmodule.h include/freetype/ftchapters.h"

# Kept under a notice other than the FTL. src/base/fthash.c (compiled into ftbase.c, used by the auto-hinter
# and psaux) carries an MIT (X11) notice from the BDF driver's authors, which LICENSE.TXT names. Its notice is
# extracted to licenses/FreeType-fthash-MIT.txt, because a binary that ships it must carry it.
EXCEPTIONS="src/base/fthash.c include/freetype/internal/fthash.h"

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DEST="$ROOT/third_party/freetype"
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT

sha256() { if command -v sha256sum >/dev/null 2>&1; then sha256sum "$1"; else shasum -a 256 "$1"; fi | cut -d' ' -f1; }

# A local tarball (checked against the same hash) skips the download.
tarball="${FREETYPE_TARBALL:-}"
if [ -z "$tarball" ]; then
    tarball="$tmp/freetype.tar.xz"
    echo "vendor-freetype: fetching $URL"
    curl -fsSL -o "$tarball" "$URL"
fi
got=$(sha256 "$tarball")
if [ "$got" != "$SHA256" ]; then
    echo "vendor-freetype: sha256 mismatch for freetype-$VERSION: got $got, want $SHA256" >&2
    exit 1
fi

mkdir "$tmp/x"
tar -xJf "$tarball" -C "$tmp/x"
src="$tmp/x/freetype-$VERSION"
[ -f "$src/LICENSE.TXT" ] && [ -f "$src/docs/FTL.TXT" ] && [ -d "$src/src/base" ] || {
    echo "vendor-freetype: unexpected tarball layout" >&2; exit 1; }

python3 -I - "$src" "$ROOT" "$VERSION" "$URL" "$SHA256" "$COMMIT" "$MODULES" "$UNITS" "$REFUSED" "$EXCEPTIONS" <<'EOF'
import json, os, re, shutil, sys

src, root, version, url, sha, commit = sys.argv[1:7]
modules, units, refused, exceptions = (sys.argv[i].split() for i in range(7, 11))
dest = os.path.join(root, "third_party", "freetype")

def die(msg):
    sys.stderr.write("vendor-freetype: " + msg + "\n")
    sys.exit(1)

def read(path):
    with open(path, encoding="latin-1") as f:
        return f.read()

def norm(text):
    return re.sub(r"\s+", " ", re.sub(r"[*/#]", " ", text)).lower()

# ---- the candidates: every .c/.h of a kept module, and the whole public include tree ----------------------
candidates = []
for m in modules:
    d = os.path.join(src, "src", m)
    if not os.path.isdir(d):
        die("no module src/%s in the release" % m)
    for dp, _, fs in os.walk(d):
        candidates += [os.path.relpath(os.path.join(dp, f), src) for f in fs if f.endswith((".c", ".h"))]
for dp, _, fs in os.walk(os.path.join(src, "include", "freetype")):
    candidates += [os.path.relpath(os.path.join(dp, f), src) for f in fs if f.endswith(".h")]
candidates.append("include/ft2build.h")
candidates = sorted(set(candidates))

for p in refused + exceptions:
    if p not in candidates:
        die("%s is listed in the script but is not in this release: update the list" % p)

# ---- the licence gate -----------------------------------------------------------------------------------
FTL = "freetype project license"
MIT = "permission is hereby granted, free of charge, to any person obtaining a copy"
COPYLEFT = re.compile(r"gnu (lesser |library )?general public licen[cs]e|\bl?gplv?[23]?\b|affero|"
                      r"mozilla public licen[cs]e|eclipse public licen[cs]e|copyleft|creative commons|"
                      r"share ?alike|european union public licen[cs]e|\beupl\b|\bcddl\b")
kept, ftl, allowed, refusedSeen, failures = [], [], [], [], []
for p in candidates:
    text = read(os.path.join(src, p))
    head = norm(text[:6000])
    if FTL in head:
        kind = "ftl"
    elif p in exceptions and MIT in head:
        kind = "exception"
    else:
        kind = "refused"
    if p in refused:
        if kind != "refused":
            failures.append("%s is on REFUSED but now carries an acceptable notice: read it, then drop it from the list" % p)
        refusedSeen.append(p)
        continue
    if kind == "refused":
        cl = COPYLEFT.search(norm(text))
        notice = ("COPYLEFT text, %r" % cl.group(0)) if cl else \
                 "an MIT-style grant" if "permission is hereby granted" in head else \
                 "public domain" if "public domain" in head else "no licence statement"
        failures.append("%s: not under the FreeType project license (%s). Read it; then REFUSED (and keep it "
                        "out of the build) or EXCEPTIONS (with its notice in licenses/)" % (p, notice))
        continue
    m = COPYLEFT.search(norm(text))
    if m:
        failures.append("%s: copyleft text (%r)" % (p, m.group(0)))
        continue
    kept.append(p)
    (ftl if kind == "ftl" else allowed).append(p)

lic, ftltxt = read(os.path.join(src, "LICENSE.TXT")), read(os.path.join(src, "docs", "FTL.TXT"))
if "docs/FTL.TXT" not in lic or "GPLv2" not in lic:
    failures.append("LICENSE.TXT no longer offers the FTL / GPLv2 choice: read it")
if "The FreeType Project LICENSE" not in ftltxt:
    failures.append("docs/FTL.TXT is not the FreeType License")
if failures:
    die("licence gate FAILED:\n  " + "\n  ".join(failures))

# ---- copy, pristine -------------------------------------------------------------------------------------
if os.path.isdir(dest):
    shutil.rmtree(dest)
for p in kept + ["LICENSE.TXT", "docs/FTL.TXT"]:
    os.makedirs(os.path.dirname(os.path.join(dest, p)), exist_ok=True)
    shutil.copyfile(os.path.join(src, p), os.path.join(dest, p))
for dp, _, fs in os.walk(dest):
    for f in fs:
        if f.upper().startswith(("GPL", "COPYING")):
            die("a copyleft licence text landed: %s" % os.path.join(dp, f))

# ---- the wrappers: one per unit, csrc/ft/ft_<module>_<unit>.c -------------------------------------------
ftdir = os.path.join(root, "csrc", "ft")
if os.path.isdir(ftdir):
    shutil.rmtree(ftdir)
os.makedirs(ftdir)
HEADER = ("/* GENERATED by tools/vendor-freetype.sh from FreeType %s's %s. Do not edit: re-run the script.\n"
          "   One FreeType compilation unit, compiled with this package's configuration (../kft_build.h) and nothing\n"
          "   of it reaching a consumer's translation units. */\n")
INC = '#include "../../third_party/freetype/%s"\n'
wrappers = []
for u in units:
    if u not in kept:
        die("unit %s is not a kept file" % u)
    body = read(os.path.join(src, u))
    incs = re.findall(r'^\s*#\s*include\s+"([^"]+\.c)"', body, re.M)
    dropped = [i for i in incs if os.path.join(os.path.dirname(u), i) in refused]
    out = HEADER % (version, u) + '\n#include "../kft_build.h"\n\n'
    if dropped:
        # The unit includes a refused file whatever the configuration, so list its other files instead. Only
        # an amalgamation qualifies: nothing but FT_MAKE_OPTION_SINGLE_OBJECT and `.c` includes.
        code = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
        lines = [l.strip() for l in code.splitlines() if l.strip()]
        for l in lines:
            if not (re.match(r'#\s*define\s+FT_MAKE_OPTION_SINGLE_OBJECT$', l) or re.match(r'#\s*include\s+"[^"]+\.c"$', l)):
                die("%s includes a refused file and is not a plain amalgamation (%r): wrap it by hand" % (u, l))
        out += "/* %s, less %s (refused by the licence gate; compiled out in this configuration). */\n" % (
            os.path.basename(u), ", ".join(dropped))
        out += "#define FT_MAKE_OPTION_SINGLE_OBJECT\n"
        for i in incs:
            if i not in dropped:
                out += INC % os.path.join(os.path.dirname(u), i)
    else:
        out += INC % u
    parts = u.split("/")
    name = "ft_%s_%s" % (parts[1], os.path.splitext(parts[-1])[0])
    with open(os.path.join(ftdir, name + ".c"), "w") as f:
        f.write(out)
    wrappers.append("csrc/ft/%s.c" % name)

# ---- csrc/ft/kft_ftmodule.h: FreeType's own module list, less the modules not kept -----------------------
CLASS_MODULE = {
    "autofit_module_class": "autofit", "tt_driver_class": "truetype", "cff_driver_class": "cff",
    "psaux_module_class": "psaux", "psnames_module_class": "psnames", "pshinter_module_class": "pshinter",
    "sfnt_module_class": "sfnt", "ft_smooth_renderer_class": "smooth",
}
uses = re.findall(r"^FT_USE_MODULE\(\s*(\w+),\s*(\w+)\s*\)\s*$", read(os.path.join(src, "include/freetype/config/ftmodule.h")), re.M)
if not uses:
    die("include/freetype/config/ftmodule.h holds no FT_USE_MODULE lines")
chosen = [(t, c) for (t, c) in uses if CLASS_MODULE.get(c) in modules]
missing = set(m for m in modules if m != "base") - set(CLASS_MODULE[c] for (_, c) in chosen)
if missing:
    die("no module class registered for %s" % " ".join(sorted(missing)))
with open(os.path.join(ftdir, "kft_ftmodule.h"), "w") as f:
    f.write("/* GENERATED by tools/vendor-freetype.sh: FreeType %s's include/freetype/config/ftmodule.h, in its order,\n"
            "   less every module this package does not vendor. ftinit.c registers exactly these. Do not edit. */\n\n" % version)
    for t, c in chosen:
        f.write("FT_USE_MODULE( %s, %s )\n" % (t, c))

# ---- licenses/FreeType-fthash-MIT.txt: the exception's notice, verbatim from its header ------------------
hdr = read(os.path.join(src, "src/base/fthash.c"))
m = re.search(r"/\*\s*\n(\s*\*\s*Copyright.*?THE USE OR OTHER DEALINGS IN THE SOFTWARE\.)\s*\n\s*\*/", hdr, re.S)
if not m:
    die("could not find the MIT notice in src/base/fthash.c")
notice = "\n".join(re.sub(r"^\s*\* ?", "", l) for l in m.group(1).splitlines()).strip() + "\n"
os.makedirs(os.path.join(root, "licenses"), exist_ok=True)
with open(os.path.join(root, "licenses", "FreeType-fthash-MIT.txt"), "w") as f:
    f.write("FreeType %s: src/base/fthash.c and include/freetype/internal/fthash.h (hashing, compiled into every\n"
            "build of @kama/freetype). The notice, verbatim from the file's header:\n\n" % version + notice)

# ---- VENDORED -------------------------------------------------------------------------------------------
with open(os.path.join(dest, "VENDORED"), "w") as f:
    f.write("FreeType %s\n%s\nsha256 %s\ngit: tag VER-%s = commit %s (every kept file is byte-identical to it)\n\n"
            % (version, url, sha, version.replace(".", "-"), commit))
    f.write("Licence TAKEN: the FreeType License (docs/FTL.TXT). FreeType is dual FTL / GPLv2 (LICENSE.TXT); this\n"
            "package elects the FTL and never GPLv2, so docs/GPLv2.TXT is not vendored.\n\n")
    f.write("Modules kept: %s\nCompiled units (one wrapper each in csrc/ft/):\n  %s\n\n" % (" ".join(modules), "\n  ".join(units)))
    f.write("Licence gate: %d candidate files checked, %d kept, %d refused.\n" % (len(candidates), len(kept), len(refusedSeen)))
    f.write("  kept under the FTL: %d\n  kept under an MIT (X11) notice LICENSE.TXT names (licenses/FreeType-fthash-MIT.txt):\n    %s\n"
            % (len(ftl), "\n    ".join(allowed)))
    f.write("  refused (not vendored; none is compiled in this configuration):\n    %s\n\n" % "\n    ".join(refusedSeen))
    f.write("Not vendored: the other modules, src/gzip (zlib), src/bzip2, src/lzw, src/dlg, builds/, docs/ bar FTL.TXT,\n"
            "tests/. The optional dependencies (zlib, libpng, bzip2, Brotli, HarfBuzz) are off: csrc/kft_ftoption.h.\n")

# ---- kama.json: the csources block ----------------------------------------------------------------------
entries = ["csrc/kama_freetype.c"] + sorted(wrappers)
path = os.path.join(root, "kama.json")
lines = open(path).read().split("\n")
try:
    start = next(i for i, l in enumerate(lines) if l == '  "csources": [')
    end = next(i for i in range(start + 1, len(lines)) if lines[i].startswith("  ]"))
except StopIteration:
    die('kama.json has no `  "csources": [` ... `  ]` block to regenerate')
block = ['    "%s"%s' % (e, "," if k < len(entries) - 1 else "") for k, e in enumerate(entries)]
text = "\n".join(lines[:start + 1] + block + lines[end:])
json.loads(text)
open(path, "w").write(text)

print("vendor-freetype: licence gate: %d checked, %d kept (%d FTL, %d MIT exception), %d refused"
      % (len(candidates), len(kept), len(ftl), len(allowed), len(refusedSeen)))
print("vendor-freetype: FreeType %s in third_party/freetype; %d wrappers in csrc/ft; kama.json csources regenerated"
      % (version, len(wrappers)))
EOF
