/*                    C O N V E R S I O N . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2008 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file conversion.c
 *
 * handle ae/direction/grid conversions
 *
 * Author:
 *   Natalie L. Barker
 *
 * Date:
 *   Jan 90
 *
 */

#include "common.h"

#include <stdio.h>
#include <math.h>

#include "machine.h"
#include "vmath.h"

#include "./nirt.h"
#include "./usrfmt.h"


extern outval	ValTab[];

void
dir2ae(void)
{
    double square;

    azimuth() = ((direct(Y) == 0) && (direct(X) == 0)) ? 0.0 :
	atan2 ( -(direct(Y)), -(direct(X)) ) / deg2rad;

    square = sqrt(direct(X) * direct(X) + direct(Y) * direct(Y));
    elevation() = atan2 (-(direct(Z)), square) / deg2rad;
}


void
grid2targ(void)
{
    double	ar = azimuth() * deg2rad;
    double	er = elevation() * deg2rad;
    
    target(X) = - grid(HORZ) * sin(ar)
	- grid(VERT) * cos(ar) * sin(er)
	+ grid(DIST) * cos(ar) * cos(er);
    target(Y) =   grid(HORZ) * cos(ar)
	- grid(VERT) * sin(ar) * sin(er)
	+ grid(DIST) * sin(ar) * cos(er);
    target(Z) =   grid(VERT) * cos(er)
	+ grid(DIST) * sin(er);
}


void
targ2grid(void)
{
    double	ar = azimuth() * deg2rad;
    double	er = elevation() * deg2rad;
    
    grid(HORZ) = - target(X) * sin(ar)
	+ target(Y) * cos(ar);
    grid(VERT) = - target(X) * cos(ar) * sin(er)
	- target(Y) * sin(er) * sin(ar)
	+ target(Z) * cos(er);
    grid(DIST) =   target(X) * cos(er) * cos(ar)
	+ target(Y) * cos(er) * sin(ar)
	+ target(Z) * sin(er);
}


void
ae2dir(void)
{
    double	ar = azimuth() * deg2rad;
    double	er = elevation() * deg2rad;
    
    int		i;
    vect_t	dir;

    dir[X] = -cos(ar) * cos(er);
    dir[Y] = -sin(ar) * cos(er);
    dir[Z] = -sin(er);
    VUNITIZE( dir );
    for (i = 0; i < 3; ++i)
	direct(i) = dir[i];
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
