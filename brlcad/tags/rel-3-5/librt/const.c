/*
 *			C O N S T . C
 *
 *  Constants used by the ray tracing library.
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1989 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSmat[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"

CONST double rt_pi     = 3.14159265358979323846;	/* pi */
CONST double rt_twopi  = 6.28318530717958647692;	/* pi*2 */
CONST double rt_halfpi = 1.57079632679489661923;	/* pi/2 */
CONST double rt_invpi  = 0.318309886183790671538;	/* 1/pi */
CONST double rt_inv2pi = 0.159154943091895335769;	/* 1/(pi*2) */

CONST double rt_inv255 = 1.0/255.0;
