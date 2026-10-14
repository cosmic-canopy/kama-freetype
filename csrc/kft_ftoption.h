/* kft_ftoption.h — the FreeType options @kama/freetype compiles with: FreeType's own defaults (the pristine
   include/freetype/config/ftoption.h), less every optional dependency, plus error strings. FT_CONFIG_OPTIONS_H
   names this file inside FreeType's translation units only (kft_build.h).

   FreeType's own advice for a custom configuration is to copy ftoption.h and edit it. Including the pristine file
   and then changing the few lines that differ keeps third_party/ byte-identical and makes the difference this
   short list. Nothing undefined here feeds one of the macros ftoption.h derives at its end (TT_USE_BYTECODE_
   INTERPRETER, TT_SUPPORT_COLRV1 and the rest), so they stay as FreeType computes them. */
#ifndef KFT_FTOPTION_H
#define KFT_FTOPTION_H

#include <freetype/config/ftoption.h>

/* The optional dependencies stay OFF. Each would be a library of its own, and the owner wants each as its own
   package later (@kama/zlib, @kama/png, @kama/harfbuzz). They are undefined rather than left at the default, so a
   consumer's `-D` cannot turn one on behind this package's back.
     zlib: FreeType's src/gzip (the zlib licence) is not vendored. So WOFF 1 fonts and gzip-compressed PCF are
           unsupported. LZW goes the same way (src/lzw is not vendored either).
     libpng: colour bitmap glyphs stored as PNG (sbix, CBDT) do not load. Plain embedded bitmaps still do.
     bzip2, Brotli: no bzip2-compressed fonts, no WOFF 2.
     HarfBuzz: the auto-hinter covers scripts without its OpenType feature analysis. That is also why
           src/autofit's HarfBuzz files are not vendored (tools/vendor-freetype.sh, REFUSED). */
#undef FT_CONFIG_OPTION_USE_ZLIB
#undef FT_CONFIG_OPTION_SYSTEM_ZLIB
#undef FT_CONFIG_OPTION_USE_LZW
#undef FT_CONFIG_OPTION_USE_BZIP2
#undef FT_CONFIG_OPTION_USE_PNG
#undef FT_CONFIG_OPTION_USE_BROTLI
#undef FT_CONFIG_OPTION_USE_HARFBUZZ
#undef FT_CONFIG_OPTION_USE_HARFBUZZ_DYNAMIC

/* OT-SVG glyphs need the `ot-svg` module and an external SVG renderer, and neither is here. With the option off,
   such a font's outlines are used. */
#undef FT_CONFIG_OPTION_SVG

/* Debugging and logging stay off. Their code is not vendored (md5.c is refused, dlg is not kept). */
#undef FT_DEBUG_LEVEL_ERROR
#undef FT_DEBUG_LEVEL_TRACE
#undef FT_DEBUG_LOGGING
#undef FT_DEBUG_AUTOFIT
#undef FT_DEBUG_MEMORY

/* FT_Error_String answers a sentence rather than NULL. The binding's error messages use it. */
#ifndef FT_CONFIG_OPTION_ERROR_STRINGS
#define FT_CONFIG_OPTION_ERROR_STRINGS
#endif

#endif
