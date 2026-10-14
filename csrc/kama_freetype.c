/* kama_freetype.c — the glue kama_freetype.h declares. Read the header first: it carries the shape. */
#include "kama_freetype.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H
#include FT_FONT_FORMATS_H

#include <stdlib.h>
#include <string.h>

/* Every number kama spells is FreeType's, or is mapped here; the slot counts are what src/freetype.kama sizes its
   arrays with. An upgrade that moves one fails here, by name. */
_Static_assert(KFT_FACE_N == 18, "src/freetype.kama sizes the face array as 18");
_Static_assert(KFT_GLYPH_N == 12, "src/freetype.kama sizes the glyph array as 12");
_Static_assert(sizeof(FT_UInt) == sizeof(uint32_t), "a glyph index crosses as uint32");

struct kft_library {
    FT_Library lib;
    int refs;   /* the kama handle, plus one per open face */
};

struct kft_face {
    kft_library *library;
    FT_Face face;
    uint8_t *bytes;   /* the copy a face from memory reads; NULL for a face from a file */
};

static const uint8_t *bytes_of(const char *s, int32_t *len)
{
    if (s == NULL) { s = ""; }
    *len = (int32_t)strlen(s);
    return (const uint8_t *)s;
}

const uint8_t *kft_error_string(int32_t error, int32_t *len)
{
    const char *s = FT_Error_String((FT_Error)error);
    return bytes_of(s != NULL ? s : "unknown FreeType error", len);
}

/* ---- the library ------------------------------------------------------------------------------------------------ */

kft_library *kft_library_new(int32_t *error)
{
    kft_library *l = (kft_library *)calloc(1, sizeof *l);
    if (l == NULL) { *error = FT_Err_Out_Of_Memory; return NULL; }
    FT_Error e = FT_Init_FreeType(&l->lib);
    if (e != 0) { free(l); *error = e; return NULL; }
    l->refs = 1;
    *error = 0;
    return l;
}

void kft_library_release(kft_library *library)
{
    if (library == NULL) { return; }
    if (--library->refs > 0) { return; }
    FT_Done_FreeType(library->lib);
    free(library);
}

int32_t kft_library_version(const kft_library *library)
{
    FT_Int major = 0, minor = 0, patch = 0;
    FT_Library_Version(library->lib, &major, &minor, &patch);
    return (int32_t)(major * 10000 + minor * 100 + patch);
}

/* ---- faces ------------------------------------------------------------------------------------------------------ */

static kft_face *adopt(kft_library *library, FT_Face f, uint8_t *bytes)
{
    kft_face *face = (kft_face *)calloc(1, sizeof *face);
    if (face == NULL) {
        FT_Done_Face(f);
        free(bytes);
        return NULL;
    }
    face->library = library;
    face->face = f;
    face->bytes = bytes;
    library->refs++;
    return face;
}

kft_face *kft_face_open(kft_library *library, const char *path, int32_t index, int32_t *error)
{
    FT_Face f = NULL;
    FT_Error e = FT_New_Face(library->lib, path, (FT_Long)index, &f);
    if (e != 0) { *error = e; return NULL; }
    kft_face *face = adopt(library, f, NULL);
    *error = face != NULL ? 0 : FT_Err_Out_Of_Memory;
    return face;
}

kft_face *kft_face_from_bytes(kft_library *library, const uint8_t *data, size_t len, int32_t index, int32_t *error)
{
    if (len == 0 || len > (size_t)0x7fffffff) { *error = FT_Err_Invalid_Argument; return NULL; }
    uint8_t *copy = (uint8_t *)malloc(len);
    if (copy == NULL) { *error = FT_Err_Out_Of_Memory; return NULL; }
    memcpy(copy, data, len);
    FT_Face f = NULL;
    FT_Error e = FT_New_Memory_Face(library->lib, copy, (FT_Long)len, (FT_Long)index, &f);
    if (e != 0) { free(copy); *error = e; return NULL; }
    kft_face *face = adopt(library, f, copy);
    *error = face != NULL ? 0 : FT_Err_Out_Of_Memory;
    return face;
}

void kft_face_free(kft_face *face)
{
    if (face == NULL) { return; }
    FT_Done_Face(face->face);
    free(face->bytes);
    kft_library_release(face->library);
    free(face);
}

static int32_t clamp32(FT_Long v)
{
    if (v > 0x7fffffffL) { return 0x7fffffff; }
    if (v < -0x7fffffffL) { return -0x7fffffff; }
    return (int32_t)v;
}

void kft_face_info(const kft_face *face, int32_t *values, int32_t n)
{
    int32_t v[KFT_FACE_N];
    const FT_Face f = face->face;
    int32_t flags = 0;
    memset(v, 0, sizeof v);
    v[KFT_FACE_NUM_FACES] = clamp32(f->num_faces);
    v[KFT_FACE_INDEX] = clamp32(f->face_index & 0xFFFF);
    v[KFT_FACE_NUM_GLYPHS] = clamp32(f->num_glyphs);
    v[KFT_FACE_UNITS_PER_EM] = (int32_t)f->units_per_EM;
    v[KFT_FACE_ASCENDER] = (int32_t)f->ascender;
    v[KFT_FACE_DESCENDER] = (int32_t)f->descender;
    v[KFT_FACE_HEIGHT] = (int32_t)f->height;
    v[KFT_FACE_MAX_ADVANCE_WIDTH] = (int32_t)f->max_advance_width;
    if (FT_IS_SCALABLE(f)) { flags |= KFT_FLAG_SCALABLE; }
    if (FT_IS_FIXED_WIDTH(f)) { flags |= KFT_FLAG_FIXED_WIDTH; }
    if (FT_HAS_KERNING(f)) { flags |= KFT_FLAG_KERNING; }
    if (FT_HAS_MULTIPLE_MASTERS(f) || FT_IS_NAMED_INSTANCE(f)) { flags |= KFT_FLAG_VARIATION; }
    if (f->style_flags & FT_STYLE_FLAG_ITALIC) { flags |= KFT_FLAG_ITALIC; }
    if (f->style_flags & FT_STYLE_FLAG_BOLD) { flags |= KFT_FLAG_BOLD; }
    if (FT_IS_SFNT(f)) { flags |= KFT_FLAG_SFNT; }
    const char *format = FT_Get_Font_Format(f);
    if (format != NULL && strcmp(format, "CFF") == 0) { flags |= KFT_FLAG_CFF; }
    v[KFT_FACE_FLAGS] = flags;
    v[KFT_FACE_NUM_FIXED_SIZES] = (int32_t)f->num_fixed_sizes;
    if (f->size != NULL) {
        v[KFT_FACE_X_PPEM] = (int32_t)f->size->metrics.x_ppem;
        v[KFT_FACE_Y_PPEM] = (int32_t)f->size->metrics.y_ppem;
        v[KFT_FACE_SIZE_ASCENDER] = clamp32(f->size->metrics.ascender);
        v[KFT_FACE_SIZE_DESCENDER] = clamp32(f->size->metrics.descender);
        v[KFT_FACE_SIZE_HEIGHT] = clamp32(f->size->metrics.height);
        v[KFT_FACE_SIZE_MAX_ADVANCE] = clamp32(f->size->metrics.max_advance);
    }
    v[KFT_FACE_UNDERLINE_POSITION] = (int32_t)f->underline_position;
    v[KFT_FACE_UNDERLINE_THICKNESS] = (int32_t)f->underline_thickness;
    for (int32_t i = 0; i < n && i < KFT_FACE_N; i++) { values[i] = v[i]; }
}

const uint8_t *kft_face_family(const kft_face *face, int32_t *len) { return bytes_of(face->face->family_name, len); }
const uint8_t *kft_face_style(const kft_face *face, int32_t *len) { return bytes_of(face->face->style_name, len); }

int32_t kft_face_set_pixel_size(kft_face *face, int32_t width, int32_t height)
{
    if (width < 0 || height < 0 || (width == 0 && height == 0)) { return FT_Err_Invalid_Pixel_Size; }
    return FT_Set_Pixel_Sizes(face->face, (FT_UInt)width, (FT_UInt)height);
}

uint32_t kft_face_glyph_index(kft_face *face, uint32_t codepoint)
{
    return (uint32_t)FT_Get_Char_Index(face->face, (FT_ULong)codepoint);
}

static FT_Int32 load_flags(int32_t hinting)
{
    switch (hinting) {
        case KFT_HINT_LIGHT: return FT_LOAD_TARGET_LIGHT;
        case KFT_HINT_NONE:  return FT_LOAD_NO_HINTING;
        case KFT_HINT_AUTO:  return FT_LOAD_FORCE_AUTOHINT;
        default:             return FT_LOAD_DEFAULT;
    }
}

int32_t kft_face_load_glyph(kft_face *face, uint32_t glyph, int32_t hinting)
{
    return FT_Load_Glyph(face->face, (FT_UInt)glyph, load_flags(hinting));
}

int32_t kft_face_render_glyph(kft_face *face, uint32_t glyph, int32_t hinting)
{
    const FT_Int32 flags = load_flags(hinting);
    const FT_GlyphSlot s = face->face->glyph;
    FT_Error e = FT_Load_Glyph(face->face, (FT_UInt)glyph, flags);
    if (e != 0) { return e; }
    if (s->format == FT_GLYPH_FORMAT_BITMAP && s->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) {
        /* An embedded strike that is not 8-bit coverage (a 1-bit one; colour needs libpng, which is off). A face with
           outlines renders those instead; a bitmap-only one has nothing this binding can hand out. */
        if (!FT_IS_SCALABLE(face->face)) { return FT_Err_Unimplemented_Feature; }
        e = FT_Load_Glyph(face->face, (FT_UInt)glyph, flags | FT_LOAD_NO_BITMAP);
        if (e != 0) { return e; }
    }
    if (s->format == FT_GLYPH_FORMAT_BITMAP) { return 0; }   /* an 8-bit embedded strike is already coverage */
    return FT_Render_Glyph(s, hinting == KFT_HINT_LIGHT ? FT_RENDER_MODE_LIGHT : FT_RENDER_MODE_NORMAL);
}

static int rendered(const FT_GlyphSlot s)
{
    return s->format == FT_GLYPH_FORMAT_BITMAP && s->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY;
}

void kft_glyph_info(const kft_face *face, int32_t *values, int32_t n)
{
    int32_t v[KFT_GLYPH_N];
    const FT_GlyphSlot s = face->face->glyph;
    memset(v, 0, sizeof v);
    v[KFT_GLYPH_INDEX] = (int32_t)s->glyph_index;
    if (rendered(s)) {
        v[KFT_GLYPH_WIDTH] = (int32_t)s->bitmap.width;
        v[KFT_GLYPH_ROWS] = (int32_t)s->bitmap.rows;
        v[KFT_GLYPH_LEFT] = (int32_t)s->bitmap_left;
        v[KFT_GLYPH_TOP] = (int32_t)s->bitmap_top;
        v[KFT_GLYPH_RENDERED] = 1;
    }
    v[KFT_GLYPH_ADVANCE_X] = clamp32(s->advance.x);
    v[KFT_GLYPH_ADVANCE_Y] = clamp32(s->advance.y);
    v[KFT_GLYPH_METRIC_WIDTH] = clamp32(s->metrics.width);
    v[KFT_GLYPH_METRIC_HEIGHT] = clamp32(s->metrics.height);
    v[KFT_GLYPH_BEARING_X] = clamp32(s->metrics.horiBearingX);
    v[KFT_GLYPH_BEARING_Y] = clamp32(s->metrics.horiBearingY);
    for (int32_t i = 0; i < n && i < KFT_GLYPH_N; i++) { values[i] = v[i]; }
}

int32_t kft_glyph_copy_bitmap(const kft_face *face, uint8_t *dst, size_t cap)
{
    const FT_GlyphSlot s = face->face->glyph;
    if (!rendered(s)) { return -1; }
    const size_t width = s->bitmap.width, rows = s->bitmap.rows;
    const size_t need = width * rows;
    if (need > (size_t)0x7fffffff) { return -1; }
    if (cap < need || need == 0) { return (int32_t)need; }
    /* FreeType's pitch is the row stride, negative for a bottom-up bitmap; `buffer` is its first row either way. */
    const int pitch = s->bitmap.pitch;
    for (size_t r = 0; r < rows; r++) {
        const unsigned char *row = pitch >= 0 ? s->bitmap.buffer + r * (size_t)pitch
                                              : s->bitmap.buffer + (rows - 1 - r) * (size_t)(-pitch);
        memcpy(dst + r * width, row, width);
    }
    return (int32_t)need;
}

int32_t kft_face_advance(kft_face *face, uint32_t glyph, int32_t hinting, int32_t *advance)
{
    FT_Fixed a = 0;
    FT_Error e = FT_Get_Advance(face->face, (FT_UInt)glyph, load_flags(hinting), &a);
    /* Scaled advances come back 16.16; 26.6 is everything else this binding answers. */
    *advance = e == 0 ? clamp32((a + 512) >> 10) : 0;
    return e;
}

int32_t kft_face_kerning(kft_face *face, uint32_t left, uint32_t right, int32_t mode, int32_t *x, int32_t *y)
{
    FT_Vector k = { 0, 0 };
    FT_UInt m = mode == KFT_KERN_UNFITTED ? FT_KERNING_UNFITTED
              : mode == KFT_KERN_UNSCALED ? FT_KERNING_UNSCALED : FT_KERNING_DEFAULT;
    FT_Error e = FT_Get_Kerning(face->face, (FT_UInt)left, (FT_UInt)right, m, &k);
    *x = e == 0 ? clamp32(k.x) : 0;
    *y = e == 0 ? clamp32(k.y) : 0;
    return e;
}
