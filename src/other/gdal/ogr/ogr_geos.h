/******************************************************************************
 * $Id: ogr_geos.h 34523 2016-07-02 21:50:47Z goatbar $
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Definitions related to support for use of GEOS in OGR.
 *           This file is only intended to be pulled in by OGR implementation
 *           code directly accessing GEOS.
 * Author:   Frank Warmerdam <warmerdam@pobox.com>
 *
 ******************************************************************************
 * Copyright (c) 2004, Frank Warmerdam <warmerdam@pobox.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#ifndef OGR_GEOS_H_INCLUDED
#define OGR_GEOS_H_INCLUDED

#ifdef HAVE_GEOS
// To avoid accidental use of non reentrant GEOS API.
// (check only effective in GEOS >= 3.5)
#  define GEOS_USE_ONLY_R_API

#  include <geos_c.h>
#else

namespace geos {
    class Geometry;
};

#endif

#endif /* ndef OGR_GEOS_H_INCLUDED */
