#ifndef KAMA_FREETYPE_H
#define KAMA_FREETYPE_H
/* The C @kama/freetype writes itself: glue only. The font engine is FreeType's, reached through these functions, so
   kama never spells a FreeType struct and never holds a pointer into one past the call that produced it.

   This file is a consumer of FreeType's public API, like any program's C: it sees FreeType's default public
   configuration, not the build configuration csrc/kft_build.h gives FreeType's own translation units.

   Shape:
     - A library is reference-counted. Every face retains the library it was opened with, so FreeType's rule that
       every face goes before its library holds whatever order kama drops them in.
     - A face opened from bytes owns a copy of them (FreeType reads the buffer for the face's whole life).
     - Numbers come back through an int32 array with named slots (KFT_FACE_*, KFT_GLYPH_*), so kama never mirrors a
       struct. 26.6 means FreeType's fixed point: 64ths of a pixel.
     - Every function that can fail returns FreeType's FT_Error: 0 is success. */
#include <stddef.h>
#include <stdint.h>

typedef struct kft_library kft_library;
typedef struct kft_face kft_face;

/* An error code's text (FreeType's FT_Error_String), as bytes and a length. Never NULL. */
const uint8_t *kft_error_string(int32_t error, int32_t *len);

/* ---- the library ---------------------------------------------------------------------------------------------- */
kft_library *kft_library_new(int32_t *error);        /* NULL on failure, the reason in *error */
void kft_library_release(kft_library *library);       /* the last release (its faces' included) frees it */
/* The FreeType version linked, as major * 10000 + minor * 100 + patch (21403 for 2.14.3). */
int32_t kft_library_version(const kft_library *library);

/* ---- faces ---------------------------------------------------------------------------------------------------- */
/* `index` is FreeType's face_index: the face within a collection (.ttc), and a named instance in its upper 16 bits. */
kft_face *kft_face_open(kft_library *library, const char *path, int32_t index, int32_t *error);
kft_face *kft_face_from_bytes(kft_library *library, const uint8_t *data, size_t len, int32_t index, int32_t *error);
void kft_face_free(kft_face *face);

#define KFT_FACE_NUM_FACES            0   /* faces in the file (a collection holds several) */
#define KFT_FACE_INDEX                1
#define KFT_FACE_NUM_GLYPHS           2
#define KFT_FACE_UNITS_PER_EM         3   /* 0 for a bitmap-only face */
#define KFT_FACE_ASCENDER             4   /* font units */
#define KFT_FACE_DESCENDER            5   /* font units, negative below the baseline */
#define KFT_FACE_HEIGHT               6   /* font units: the baseline-to-baseline distance */
#define KFT_FACE_MAX_ADVANCE_WIDTH    7   /* font units */
#define KFT_FACE_FLAGS                8   /* KFT_FLAG_* */
#define KFT_FACE_NUM_FIXED_SIZES      9
#define KFT_FACE_X_PPEM              10   /* the size set, in pixels; 0 before one is */
#define KFT_FACE_Y_PPEM              11
#define KFT_FACE_SIZE_ASCENDER       12   /* 26.6, at the size set */
#define KFT_FACE_SIZE_DESCENDER      13   /* 26.6 */
#define KFT_FACE_SIZE_HEIGHT         14   /* 26.6 */
#define KFT_FACE_SIZE_MAX_ADVANCE    15   /* 26.6 */
#define KFT_FACE_UNDERLINE_POSITION  16   /* font units */
#define KFT_FACE_UNDERLINE_THICKNESS 17   /* font units */
#define KFT_FACE_N                   18

#define KFT_FLAG_SCALABLE     1
#define KFT_FLAG_FIXED_WIDTH  2
#define KFT_FLAG_KERNING      4   /* the face has a legacy 'kern' table FT_Get_Kerning reads */
#define KFT_FLAG_VARIATION    8   /* a variable font (or a named instance of one) */
#define KFT_FLAG_ITALIC      16
#define KFT_FLAG_BOLD        32
#define KFT_FLAG_SFNT        64   /* TrueType or OpenType container */
#define KFT_FLAG_CFF        128   /* PostScript (CFF) outlines rather than TrueType ones */

void kft_face_info(const kft_face *face, int32_t *values, int32_t n);   /* fills values[0 .. min(n, KFT_FACE_N)) */
/* The family and style names, as bytes and a length; "" (never NULL) when the font has none. */
const uint8_t *kft_face_family(const kft_face *face, int32_t *len);
const uint8_t *kft_face_style(const kft_face *face, int32_t *len);

int32_t kft_face_set_pixel_size(kft_face *face, int32_t width, int32_t height);
/* The glyph for a Unicode code point in the face's Unicode charmap; 0 (".notdef") when it has none. */
uint32_t kft_face_glyph_index(kft_face *face, uint32_t codepoint);

/* How glyphs are hinted: the face's own (bytecode, or the auto-hinter where it has none), the auto-hinter's light
   mode (vertical only, keeps shapes and advances closest to the design), none, or the auto-hinter forced. */
#define KFT_HINT_DEFAULT 0
#define KFT_HINT_LIGHT   1
#define KFT_HINT_NONE    2
#define KFT_HINT_AUTO    3

/* Load glyph `glyph` into the face's slot, and (render) rasterise it to an 8-bit coverage bitmap. The slot is the
   face's one: the next load replaces it. An embedded strike is used when it is 8-bit; a 1-bit one gives way to the
   outline, and a bitmap-only face with 1-bit strikes answers FT_Err_Unimplemented_Feature. */
int32_t kft_face_load_glyph(kft_face *face, uint32_t glyph, int32_t hinting);
int32_t kft_face_render_glyph(kft_face *face, uint32_t glyph, int32_t hinting);

#define KFT_GLYPH_INDEX       0
#define KFT_GLYPH_WIDTH       1   /* the bitmap, in pixels: 0 x 0 for a blank glyph, and before a render */
#define KFT_GLYPH_ROWS        2
#define KFT_GLYPH_LEFT        3   /* the bitmap's left edge, from the pen position */
#define KFT_GLYPH_TOP         4   /* the bitmap's top row, above the baseline */
#define KFT_GLYPH_ADVANCE_X   5   /* 26.6 */
#define KFT_GLYPH_ADVANCE_Y   6   /* 26.6 */
#define KFT_GLYPH_METRIC_WIDTH    7   /* 26.6: the outline's box */
#define KFT_GLYPH_METRIC_HEIGHT   8
#define KFT_GLYPH_BEARING_X       9
#define KFT_GLYPH_BEARING_Y      10
#define KFT_GLYPH_RENDERED       11   /* 1 when the slot holds an 8-bit coverage bitmap */
#define KFT_GLYPH_N              12

void kft_glyph_info(const kft_face *face, int32_t *values, int32_t n);
/* The slot's coverage bitmap, copied tightly packed (width bytes a row, top row first) into dst. Answers the bytes
   it needs (width * rows), copying only when cap is at least that; -1 when the slot holds no coverage bitmap. */
int32_t kft_glyph_copy_bitmap(const kft_face *face, uint8_t *dst, size_t cap);

/* The horizontal advance of `glyph` at the size set, 26.6, without rendering it (FreeType's FT_Get_Advance). */
int32_t kft_face_advance(kft_face *face, uint32_t glyph, int32_t hinting, int32_t *advance);

/* The kerning between two glyphs from the face's 'kern' table. Mode: KFT_KERN_* below. 0 for a face without one. */
#define KFT_KERN_DEFAULT  0   /* 26.6, grid-fitted */
#define KFT_KERN_UNFITTED 1   /* 26.6 */
#define KFT_KERN_UNSCALED 2   /* font units */
int32_t kft_face_kerning(kft_face *face, uint32_t left, uint32_t right, int32_t mode, int32_t *x, int32_t *y);

#endif
