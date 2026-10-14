# Changelog

All notable changes to `@kama/freetype`. The format is [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
this package follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] — unreleased

The first version. FreeType was previously fetched and built by CMake inside `@kama/rmlui` (and in
the-mallow-man and friendly-fire-department before it). It is now vendored here and compiled by kama.

- FreeType 2.14.3 is vendored by `tools/vendor-freetype.sh`, pinned by SHA-256. It is the same release as the tag
  `VER-2-14-3` the games pinned. The FTL is taken.
  - The modules kept: base, truetype, sfnt, smooth, psnames, autofit, cff, psaux and pshinter.
  - zlib, libpng, bzip2, Brotli and HarfBuzz are off.
- A licence gate. It checks 306 files and keeps 297: 295 under the FTL, and 2 under fthash's MIT notice, which
  ships in `licenses/`. It refuses 9, none of which this configuration compiles.
- FreeType compiles through generated wrapper units, so its build macros never reach a consumer. Consumers get only
  `include/`.
- The kama API: `FreeType`, `Face` (from a file or from bytes; names, flags, metrics, pixel size, glyph index,
  render, glyph metrics, advance, kerning), `Glyph` (an owned 8-bit coverage bitmap), and `FreeTypeError`.
- `tests/`: 59 checks against two committed OFL fonts, with the expected numbers read from the fonts by
  `tools/font-facts.py`. Green on Linux x86_64 in debug, release and AddressSanitizer, and on wasm (node) in debug
  and release.
