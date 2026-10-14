# @kama/freetype

[FreeType](https://freetype.org) 2.14.3 for [kama](https://kama-lang.org): font files in, glyph coverage bitmaps and
metrics out. A general binding of the part of FreeType that text needs, with FreeType itself vendored and compiled
into your program, so there is nothing to install on any target.

> **Status: new, not yet published.** Tested on Linux x86_64 (debug, release, AddressSanitizer) and on WebAssembly
> (emscripten, run under node). macOS and Windows are not yet run. Needs **kama ≥ 0.9.577**.

## Using it

```json
"dependencies": { "@kama/freetype": { "version": "^0.1.0" } }
```

```kama
import { core::println, freetype::FreeType, freetype::FreeTypeError, freetype::Face, freetype::Glyph,
         freetype::Hinting };

fn int32 main()
{
    Result<FreeType, FreeTypeError> started = FreeType.init();
    FreeType ft = match (give started) { case Ok(value: f): give f; case Err(error: e): { return 1; } };
    string path = "assets/fonts/Inter-Regular.ttf";
    Result<Face, FreeTypeError> opened = Face.open(library: ft, path: path, index: 0);
    Face face = match (give opened) { case Ok(value: f): give f; case Err(error: e): { return 1; } };
    Result<Unit, FreeTypeError> sized = face.setPixelSize(width: 0, height: 32);
    uint32 a = face.glyphIndex(codepoint: 65);
    Result<Glyph, FreeTypeError> drawn = face.render(glyph: a, hinting: Hinting::Light);
    match (drawn) {
        case Ok(value: g): { int32 w = g.width(); int32 h = g.rows(); println(s: "A is ${w} x ${h} pixels"); }
        case Err(error: e): { return 1; }
    };
    return 0;
}
```

- **`FreeType`** is the library. A face keeps it alive, so the order the two are dropped in does not matter.
- **`Face`**: `open(library:, path:, index:)` or `fromBytes(library:, bytes:, index:)` (it keeps its own copy).
  - Its names, flags and design metrics.
  - `setPixelSize`, then `sizeMetrics`.
  - `glyphIndex(codepoint:)`.
  - `render(glyph:, hinting:)`, which answers a `Glyph`, and `glyphMetrics` without a bitmap.
  - `advance`, and `kerning` from the `kern` table.
- **`Glyph`** is an owned 8-bit coverage bitmap, `width` × `rows`, with the top row first and tightly packed. Read it
  with `borrow glyph.view() as px { … }` or `glyph.at(x:, y:)`. It also carries its metrics.
- Units are FreeType's own. Design metrics are in font units. Everything at a size is **26.6** fixed point (64ths of a
  pixel).
- `Hinting`:
  - `Default` is the face's own hinting.
  - `Light` is the auto-hinter, vertical only. Most UI text wants this.
  - `None` uses no hinting.
  - `Auto` forces the auto-hinter.

The C API is there as well. `cincludes` puts FreeType's `include/` on your path, so C or C++ in your build (as in
`@kama/rmlui`) can `#include <ft2build.h>` and call FreeType directly. It gets FreeType's default public
configuration, which is the same shape this build uses.

## What it reads

The modules kept are `base`, `truetype`, `sfnt`, `smooth`, `psnames`, `autofit`, `cff`, `psaux` and `pshinter`. They
cover:

- TrueType (`glyf`) and OpenType CFF outlines, in single files and collections.
- Variable fonts (`ftmm`).
- Embedded bitmap strikes, when they are 8-bit. A 1-bit strike gives way to the face's outline. A bitmap-only face
  with 1-bit strikes is refused (`FreeTypeError::Glyph`).
- The `kern` table.
- The anti-aliased rasteriser and the auto-hinter.

Not here:

- Type 1, CID, PFR, Type 42, Windows FNT, PCF and BDF fonts.
- The monochrome and SDF rasterisers, OT-SVG glyphs, and FreeType's cache.

**The optional dependencies are off.** zlib (so no WOFF 1), libpng (so no PNG colour glyphs), bzip2, Brotli (so no
WOFF 2) and HarfBuzz are all off. Each is meant to become a package of its own (`@kama/zlib`, `@kama/png`,
`@kama/harfbuzz`) that this one can then depend on. There is no text shaping: lay a run out by advances and
`kerning`. That is what FreeType alone can do, and it is what RmlUi does.

## What is vendored, and why

`third_party/freetype/` holds the FreeType 2.14.3 release tarball's files, byte-identical. That is the tag
`VER-2-14-3` (`0a0221a1347e2f1e07c395263540026e9a0aa7c7`), the version `the-mallow-man` and `friendly-fire-department`
pinned. The tree holds:

- `LICENSE.TXT` and `docs/FTL.TXT`.
- `include/`.
- The `.c` and `.h` files of the kept modules.

`third_party/freetype/VENDORED` records the URL, the SHA-256, what was kept, what was not, and why. That file is the
FTL's "changes to the original files clearly indicated". `tools/vendor-freetype.sh` is the only thing that writes the
tree.

FreeType is vendored for three reasons:

- A consumer gets a working build on every target kama has, with nothing to install. wasm has no system libraries at
  all.
- A package cannot add `-I/opt/homebrew/include` for its consumer.
- A shipped binary links one known FreeType with one known set of options. friendly-fire-department measured MSYS2's
  FreeType pulling 14 more DLLs into an .exe, some of them LGPL.

**The licence gate.** `tools/vendor-freetype.sh` checks the licence notice of every candidate file in the kept modules
and in `include/`, and fails the run on anything it has not been told about. At 2.14.3 it checks **306 files and
keeps 297**:

- 295 are under the FreeType License.
- 2 are under an MIT notice: `src/base/fthash.c` and its header. FreeType's `LICENSE.TXT` names them.

It **refuses 9**, and none of them is compiled in this configuration:

- `md5.c`/`md5.h`: public domain, compiled only with tracing on.
- Five HarfBuzz-derived files under "Old MIT", compiled only with HarfBuzz on.
- Two headers that state no licence and that this build does not use.

Any copyleft text in a kept file also fails the run, and `docs/GPLv2.TXT` is never copied.

**How it is built without leaking into yours.** FreeType compiles with `FT2_BUILD_LIBRARY` and this package's option
and module lists. A manifest `cflags` entry reaches every translation unit of every consumer, so they are not set
that way. Instead:

- Each FreeType compilation unit is a generated wrapper in `csrc/ft/`. The wrapper includes `csrc/kft_build.h` (which
  defines those three things) and then the pristine `.c`.
- `csrc/kft_ftoption.h` is FreeType's own `ftoption.h` with the optional dependencies undefined and error strings
  on.
- `csrc/ft/kft_ftmodule.h` is FreeType's module list, cut down to the kept modules.

Your code sees only `include/`. FreeType is compiled at your build's optimisation level. To keep it at release
speed in a debug build, add `"debug": { "optimize": ["freetype"] }` to your root manifest.

## The licence taken, and why

FreeType is dual-licensed: the **FreeType License (FTL)** or the **GPLv2**, at the user's choice. **This package
takes the FTL**, because:

- the FTL is a BSD-style licence with a credit clause;
- it is commercial-friendly;
- it does not reach the rest of the program.

GPLv2 would force copyleft on everything linked with it. The kama code here is **MIT OR Apache-2.0**.

## What a program that ships this must carry

`licenses/manifest.tsv` lists the texts. Both apply to **every** build that links this package, native and web:

1. **FreeType's FTL**: `third_party/freetype/docs/FTL.TXT`. A binary's documentation must say that the software is
   based in part on the work of the FreeType Team (a credits screen counts). The FTL's suggested wording:

   > Portions of this software are copyright © 2026 The FreeType Project (https://freetype.org). All rights reserved.

2. **The MIT notice of FreeType's hashing code**: `licenses/FreeType-fthash-MIT.txt`. It comes from the Computing
   Research Lab at New Mexico State University and from Francesco Zappa Nardelli. `src/base/fthash.c` is compiled
   into every build.

Nothing else from FreeType reaches a binary. The two test fonts (SIL OFL 1.1) are never part of a program, and they
are not published.

## Tests

```sh
KAMA=/path/to/kama tools/test.sh        # Linux: debug, release, and debug under AddressSanitizer
KAMA=/path/to/kama tools/test-wasm.sh   # wasm via emscripten (EMSDK, default ~/emsdk), run under node
```

`tests/` is one program, **59 checks**. It passes on every leg: debug, release, AddressSanitizer with leak checking,
and wasm debug and release. It uses two committed fonts (`tests/fonts/SOURCES`):

- Lato: TrueType, with a `kern` table.
- Source Code Pro: OpenType CFF, monospaced.

Every number the tests expect of a font comes from `tools/font-facts.py`, which reads the font's own tables with no
font library. The facts are units per em, line metrics, glyph count, `cmap`, `hmtx` and `kern` pairs. The tests
check:

- That the face and its flags are read right, and that both fonts' tables come through unchanged.
- Scaled advances and kerning against the tables.
- Rendered bitmaps: their size, that every row of `A` has ink, ink coverage, solid and anti-aliased pixels, the gap
  between `A`'s feet, `g` reaching below the baseline, and twice the size giving twice the rows.
- That a CFF face renders through `cff`/`psaux`/`pshinter`.
- That a face from bytes renders the same pixels as one from the file, after the caller's buffer is gone.
- That errors carry FreeType's message.
- That a face still renders after its `FreeType` is dropped.

## Upgrading FreeType

1. Change `VERSION`, `SHA256` and `COMMIT` in `tools/vendor-freetype.sh`.
2. Run it.
3. Read the licence gate's verdict and `git diff`.
4. Run both test scripts.
5. Bump this package's version.

A new file in a kept module that is not under the FTL stops the run until someone has read it.

## Publishing

`kama publish` ships exactly what git tracks, minus `publish.exclude` (`tools/`, `tests/`, the agent notes). The three
packages publish in order: **@kama/freetype**, then **@kama/rmlui**, then **@kama/ui**. Each switches its `path`
dependency on the one before to a `^0.1.0` registry version just before it publishes, because a published package
cannot carry a `path` dependency. This one depends on nothing and goes first.

## License

`@kama/freetype`'s own code is licensed under either of

- Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE))
- MIT license ([LICENSE-MIT](LICENSE-MIT))

at your option. FreeType, vendored in `third_party/freetype`, is used under the FreeType License. The section above
says what a program that ships it carries.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted for inclusion in `@kama/freetype` by
you, as defined in the Apache-2.0 license, shall be dual licensed as above, without any additional terms or conditions.
