# @kama/freetype: what is true in THIS repo

`AGENTS.md` (the language) and `AGENTS.package.md` (a publishable library) are written by `kama agents install` and are
not hand-edited. This file is the project-specific third, and it is the one to edit. README.md holds the design. These
are the working rules.

- **`tools/vendor-freetype.sh` owns four things.** It writes `third_party/freetype/`, `csrc/ft/` (the wrapper units
  and `kft_ftmodule.h`), `licenses/FreeType-fthash-MIT.txt`, and the `csources` block of `kama.json`. None of them is
  edited by hand. To change the modules, units or licence lists, edit the constants at the top of the script and run
  it. Read the licence gate's verdict and the diff.
- **The licence gate is the rule, not a suggestion.** Every kept file must carry the FreeType project license notice,
  or be an `EXCEPTIONS` entry whose notice ships in `licenses/`. Never add a file to `EXCEPTIONS` that is copyleft, or
  that a consumer could not ship under a notice. FreeType is taken under the FTL, never GPLv2.
- **Nothing of FreeType's build reaches a consumer.** `FT2_BUILD_LIBRARY` and the option and module macros live in
  `csrc/kft_build.h`, and only the wrappers include it. Never move them into `cflags`: a dependency's `cflags` reach
  every translation unit of the consumer. `csrc/kft_ftoption.h` is the option policy. Turning an optional dependency
  on there means vendoring it first, as its own package.
- **`csrc/kama_freetype.{h,c}` is glue only**, and `src/freetype.kama` is the only kama file that `extern`s it. The
  `extern fn` lines are spelled from the header: `const T *` is `UnsafeConstPtr`. The slot counts of the two int32
  arrays are `_Static_assert`ed in the C, and a new slot changes both files in the same commit.
- **The gate is both scripts.** `tools/test.sh` (Linux: debug, release, AddressSanitizer) and `tools/test-wasm.sh`
  (emscripten, under node). A change to `src/` or `csrc/` is not proven until both pass.
- **Test expectations come from the fonts.** `tools/font-facts.py` writes `tests/src/facts.kama` from the fonts' own
  tables, and `tools/test.sh` fails when it is stale. Never type a font's number into a test.
