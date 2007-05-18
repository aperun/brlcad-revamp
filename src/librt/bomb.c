/*                          B O M B . C
 * BRL-CAD
 *
 * Copyright (c) 1998-2007 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @addtogroup librt */
/** @{ */
/** @file ./librt/bomb.c
 *
 *  Checks LIBRT-specific error flags, then
 *  hands the error off to LIBBU.
 *
 *  Author -
 *	Michael John Muuss
 *
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 */
/** @} */

#ifndef lint
static const char RCSid[] = "@(#)$Header$ (ARL)";
#endif

#include "common.h"

#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "bn.h"
#include "raytrace.h"

#undef rt_bomb		/* in case compat4.h defines one */


/**
 * R T _ B O M B
 *
 * @deprecated
 *
 * Do not use.  Use bu_bomb() instead.
 */
void
rt_bomb(const char *s)
{
    bu_log("\
WARNING: rt_bomb() is deprecated and will likely disappear in\n\
a future release of BRL-CAD.  Applications should utilize\n\
bu_bomb() instead for fatal errors.\n\
\n");
    if(RT_G_DEBUG || rt_g.NMG_debug )
	bu_debug |= BU_DEBUG_COREDUMP;
    bu_bomb(s);
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
