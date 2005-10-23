/*                      C O N T O U R S . C
 * BRL-CAD
 *
 * Copyright (C) 1986-2005 United States Government as represented by
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
/** @file contours.c
 *
 *  Program to read "Brain Mapping Project" data and plot it.
 *
 *  Author -
 *	Michael John Muuss
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *
 */
#ifndef lint
static const char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "common.h"



#include <stdio.h>

int	x,y,z;
int	npts;
char	name[128];

main(void)
{
  register int i;

  pl_3space( stdout, -32768,  -32768,  -32768, 32767, 32767, 32767 );
  while( !feof(stdin) )  {
    if( scanf( "%d %d %s", &npts, &z, name ) != 3 )  break;
    for( i=0; i<npts; i++ )  {
      if( scanf( "%d %d", &x, &y ) != 2 )
	fprintf(stderr,"bad xy\n");
      if( i==0 )
	pl_3move( stdout, x, y, z );
      else
	pl_3cont( stdout, x, y, z );
    }
    /* Close curves? */
  }
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
