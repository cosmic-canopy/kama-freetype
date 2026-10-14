/* kft_build.h — how @kama/freetype compiles FreeType. Included FIRST by every wrapper in csrc/ft/, and by nothing
   else.

   FreeType builds with FT2_BUILD_LIBRARY and with a chosen option list and module list, named by the two macros
   below. Here they reach exactly FreeType's own translation units. As manifest `cflags` they would reach every
   translation unit of every consumer, kama's own C included (AGENTS.package.md, "Vendoring a C library"), so a
   consumer's `#include <ft2build.h>` sees FreeType's default public configuration and nothing of this one. The
   public headers do not change shape with these options (only FT_CONFIG_OPTION_USE_MODULE_ERRORS would, and it is
   off on both sides), so the two views agree on every type a consumer can reach.

   Both names are found on the include path: kama puts every directory that holds a `csources` file on it, which
   is csrc/ (kft_ftoption.h) and csrc/ft/ (kft_ftmodule.h, generated). */
#ifndef KFT_BUILD_H
#define KFT_BUILD_H

#define FT2_BUILD_LIBRARY 1

#undef FT_CONFIG_OPTIONS_H
#define FT_CONFIG_OPTIONS_H "kft_ftoption.h"

#undef FT_CONFIG_MODULES_H
#define FT_CONFIG_MODULES_H "kft_ftmodule.h"

#endif
