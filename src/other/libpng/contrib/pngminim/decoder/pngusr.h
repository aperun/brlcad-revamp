/* minrdpngconf.h: headers to make a minimal png-read-only library
 *
 * Copyright (c) 2007, 2009 Glenn Randers-Pehrson
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 *
 * Derived from pngcrush.h, Copyright 1998-2007, Glenn Randers-Pehrson
 */

#ifndef MINRDPNGCONF_H
#define MINRDPNGCONF_H

#define PNG_NO_WARNINGS
#define png_warning(s1,s2) ""
#define png_chunk_warning(s1,s2) ""
#define PNG_NO_ERROR_TEXT
#define png_error(s1,s2) png_err(s1)
#define png_chunk_error(s1,s2) png_err(s1)

#define PNG_NO_READ_BGR
#define PNG_NO_READ_GAMMA
#define PNG_NO_READ_BACKGROUND
#define PNG_NO_READ_QUANTIZE
#define PNG_NO_READ_INVERT
#define PNG_NO_READ_SHIFT
#define PNG_NO_READ_PACK
#define PNG_NO_READ_PACKSWAP
#define PNG_NO_READ_FILLER
#define PNG_NO_READ_SWAP
#define PNG_NO_READ_SWAP_ALPHA
#define PNG_NO_READ_INVERT_ALPHA
#define PNG_NO_READ_RGB_TO_GRAY
#define PNG_NO_READ_USER_TRANSFORM
#define PNG_NO_READ_bKGD
#define PNG_NO_READ_cHRM
#define PNG_NO_READ_gAMA
#define PNG_NO_READ_hIST
#define PNG_NO_READ_iCCP
#define PNG_NO_READ_oFFs
#define PNG_NO_READ_pCAL
#define PNG_NO_READ_pHYs
#define PNG_NO_READ_sBIT
#define PNG_NO_READ_sCAL
#define PNG_NO_READ_sPLT
#define PNG_NO_READ_sRGB
#define PNG_NO_READ_TEXT
#define PNG_NO_READ_tIME
#define PNG_NO_READ_UNKNOWN_CHUNKS
#define PNG_NO_READ_USER_CHUNKS
#define PNG_NO_READ_EMPTY_PLTE
#define PNG_NO_READ_OPT_PLTE
#define PNG_NO_READ_STRIP_ALPHA

#define PNG_NO_WRITE_SUPPORTED

#define PNG_NO_INFO_IMAGE
#define PNG_NO_IO_STATE
#define PNG_NO_USER_MEM
#define PNG_NO_FIXED_POINT_SUPPORTED
#define PNG_NO_MNG_FEATURES
#define PNG_NO_USER_TRANSFORM_PTR
#define PNG_NO_HANDLE_AS_UNKNOWN
#define PNG_NO_CONSOLE_IO
#define PNG_NO_ZALLOC_ZERO
#define PNG_NO_ERROR_NUMBERS
#define PNG_NO_EASY_ACCESS
#define PNG_NO_PROGRESSIVE_READ
#define PNG_NO_USER_LIMITS
#define PNG_NO_SET_USER_LIMITS
#define PNG_NO_TIME_RFC1123

#endif /* MINRDPNGCONF_H */

