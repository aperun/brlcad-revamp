/*                           U - F . C
 * BRL-CAD
 *
 * Copyright (C) 2004-2005 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this file; see the file named COPYING for more
 * information.
 */
/** @file u-f.c
 *		I - F . C
 *
 *  Convert shorts to floats.
 *
 *	% i-f [-n || scale]
 *
 *	-n will normalize the data (scale between -1.0 and +1.0).
 *
 *  Phil Dykstra - 5 Nov 85.
 */
#include "common.h"



#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include <stdio.h>
#include <math.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for atof() */
#endif


#include "machine.h"

unsigned short	ibuf[512];
float	obuf[512];

static char usage[] = "\
Usage: i-f [-n || scale] < shorts > floats\n";

int main(int argc, char **argv)
{
	int	i, num;
	double	scale;

	scale = 1.0;

	if( argc > 1 ) {
		if( strcmp( argv[1], "-n" ) == 0 )
			scale = 1.0 / 65536.0;
		else
			scale = atof( argv[1] );
		argc--;
	}

	if( argc > 1 || scale == 0 || isatty(fileno(stdin)) ) {
		fputs( usage, stderr );
		exit( 1 );
	}

	while( (num = fread( &ibuf[0], sizeof( ibuf[0] ), 512, stdin)) > 0 ) {
		if( scale == 1.0 ) {
			for( i = 0; i < num; i++ )
				obuf[i] = ibuf[i];
		} else {
			for( i = 0; i < num; i++ )
				obuf[i] = (double)ibuf[i] * scale;
		}
		fwrite( &obuf[0], sizeof( obuf[0] ), num, stdout );
	}

	return 0;
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
