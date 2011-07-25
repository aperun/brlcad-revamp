/* minwrpngconf.h: headers to make a minimal png-write-only library
 *
 * Copyright (c) 2007, 2010-2011 Glenn Randers-Pehrson
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 *
 * Derived from pngcrush.h, Copyright 1998-2007, Glenn Randers-Pehrson
 */

#ifndef MINWRPNGCONF_H
#define MINWRPNGCONF_H

/* To include pngusr.h set -DPNG_USER_CONFIG in CPPFLAGS */

/* List options to turn off features of the build that do not
 * affect the API (so are not recorded in pnglibconf.h)
 */

#define PNG_NO_WARNINGS

#endif /* MINWRPNGCONF_H */
