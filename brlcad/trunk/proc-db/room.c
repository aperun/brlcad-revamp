/*
 *			R O O M . C
 * 
 *  Program to generate procedural rooms.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright 
 *	This software is Copyright (C) 1988 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "db.h"
#include "vmath.h"
#include "wdb.h"

#include "../rt/mathtab.h"

#define HEIGHT	4000		/* 4 meter high walls */

#define EAST	1
#define NORTH	2
#define WEST	4
#define	SOUTH	8

mat_t	identity;
double degtorad = 0.0174532925199433;
double sin60;

struct mtab {
	char	mt_name[64];
	char	mt_param[96];
} mtab[] = {
	"plastic",	"",
	"glass",	"",
	"plastic",	"",
	"mirror",	"",
	"plastic",	"",
	"testmap",	"",
	"plastic",	""
};
int	nmtab = sizeof(mtab)/sizeof(struct mtab);

#define PICK_MAT	((rand() % nmtab) )

void	make_room(), make_walls(), make_pillar(), make_carpet();

main(argc, argv)
char	**argv;
{
	vect_t	norm;
	char	rgb[3];
	int	ix, iy;
	double	x, y;
	double	size;
	double	base;
	int	quant;
	char	name[64];
	vect_t	pos, aim;
	char	white[3];
	int	n;
	double	height, maxheight, minheight;
	struct wmember head;
	vect_t	bmin, bmax, bthick;
	vect_t	r1min, r1max, r1thick;
	vect_t	lwh;		/* length, width, height */
	vect_t	pbase;

	head.wm_forw = head.wm_back = &head;

	mat_idn( identity );
	sin60 = sin(60.0 * 3.14159265358979323846264 / 180.0);

	mk_id( stdout, "Procedural Rooms" );

	/* Create the building */
	VSET( bmin, 0, 0, 0 );
	VSET( bmax, 80000, 60000, HEIGHT );
	VSET( bthick, 100, 100, 100 );
	make_room( "bldg", bmin, bmax, bthick, &head );

	/* Create the first room */
	VSET( r1thick, 100, 100, 0 );
	VMOVE( r1min, bmin );
	VSET( r1max, 40000, 10000, HEIGHT );
	VADD2( r1max, r1min, r1max );
	make_walls( "rm1", r1min, r1max, r1thick, NORTH|EAST, &head );
	make_carpet( "rm1carpet", r1min, r1max, "carpet.pix", &head );

	/* Create the golden earth */
	VSET( norm, 0, 0, 1 );
	mk_half( stdout, "plane", -bthick[Z]-10.0, norm );
	rgb[0] = 240;	/* gold/brown */
	rgb[1] = 180;
	rgb[2] = 64;
	mk_mcomb( stdout, "plane.r", 1, 1, "", "", 1, rgb );
	mk_memb( stdout, "plane", identity, UNION );
	(void)mk_addmember( "plane.r", &head );

	/* Create the display pillars */
	size = 4000;	/* separation between centers */
	quant = 5;	/* pairs */
	VSET( lwh, 400, 400, 1000 );
	for( ix=quant-1; ix>=0; ix-- )  {
		x = 10000 + ix*size;
		VSET( pbase, x, 10000*.25, r1min[Z] );
		make_pillar( "Pil", ix, 0, pbase, lwh, &head );
		VSET( pbase, x, 10000*.75, r1min[Z] );
		make_pillar( "Pil", ix, 1, pbase, lwh, &head );
	}

#ifdef never
	/* Create some light */
	white[0] = white[1] = white[2] = 255;
	base = size*(quant/2+1);
	VSET( aim, 0, 0, 0 );
	VSET( pos, base, base, minheight+maxheight*rand0to1() );
	do_light( "l1", pos, aim, 1, 100.0, white, &head );
	VSET( pos, -base, base, minheight+maxheight*rand0to1() );
	do_light( "l2", pos, aim, 1, 100.0, white, &head );
	VSET( pos, -base, -base, minheight+maxheight*rand0to1() );
	do_light( "l3", pos, aim, 1, 100.0, white, &head );
	VSET( pos, base, -base, minheight+maxheight*rand0to1() );
	do_light( "l4", pos, aim, 1, 100.0, white, &head );
#endif

	/* Build the overall combination */
	mk_lfcomb( stdout, "room", 0, &head );
}

void
make_room( rname, imin, imax, thickness, headp )
char	*rname;
vect_t	imin;		/* Interior RPP min point */
vect_t	imax;
vect_t	thickness;
struct wmember *headp;
{
	struct wmember head;
	char	name[32];
	vect_t	omin;
	vect_t	omax;

	head.wm_forw = head.wm_back = &head;

	VSUB2( omin, imin, thickness );
	VADD2( omax, imax, thickness );

	sprintf( name, "o%s", rname );
	mk_rpp( stdout, name, omin, omax );
	(void)mk_addmember( name, &head );

	sprintf( name, "i%s", rname );
	mk_rpp( stdout, name, imin, imax );
	mk_addmember( name, &head )->wm_op = SUBTRACT;

	mk_lfcomb( stdout, rname, 1, &head );
	(void)mk_addmember( rname, headp );
}

void
make_walls( rname, imin, imax, thickness, bits, headp )
char	*rname;
vect_t	imin;		/* Interior RPP min point */
vect_t	imax;
vect_t	thickness;
int	bits;
struct wmember *headp;
{
	struct wmember head;
	char	name[32];
	vect_t	omin, omax;	/* outer dimensions */
	vect_t	wmin, wmax;
	int	mask;

	head.wm_forw = head.wm_back = &head;

	/* thickness[Z] = 0; */

	/*
	 *  Set exterior dimensions to interior dimensions.
	 *  Then, thicken them as necessary due to presence of
	 *  exterior walls.
	 *  It may be useful to return the exterior min,max.
	 */
	VMOVE( omin, imin );
	VMOVE( omax, imax );
	if( bits & EAST )
		omax[X] += thickness[X];
	if( bits & WEST )
		omin[X] -= thickness[X];
	if( bits & NORTH )
		omax[Y] += thickness[Y];
	if( bits & SOUTH )
		omin[Y] -= thickness[Y];

	for( mask=8; mask > 0; mask >>= 1 )  {
		if( (bits & mask) == 0 )  continue;

		VMOVE( wmin, omin );
		VMOVE( wmax, omax );

		switch( mask )  {
		case SOUTH:
			/* South (-Y) wall */
			sprintf( name, "S%s", rname );
			wmax[Y] = imin[Y];
			break;
		case WEST:
			/* West (-X) wall */
			sprintf( name, "W%s", rname );
			wmax[X] = imin[X];
			break;
		case NORTH:
			/* North (+Y) wall */
			sprintf( name, "N%s", rname );
			wmin[Y] = imax[Y];
			break;
		case EAST:
			/* East (+X) wall */
			sprintf( name, "E%s", rname );
			wmin[X] = imax[X];
			break;
		}
		mk_rpp( stdout, name, wmin, wmax );
		(void)mk_addmember( name, &head );
	}

	mk_lfcomb( stdout, rname, 1, &head );
	(void)mk_addmember( rname, headp );
}

void
make_pillar( prefix, ix, iy, center, lwh, headp )
char	*prefix;
int	ix;
int	iy;
vect_t	center;			/* center of base */
vect_t	lwh;
struct wmember *headp;
{
	vect_t	min, max;
	char	rgb[4];		/* needs all 4 */
	char	pilname[32], rname[32], sname[32], oname[32];
	int	i;
	struct wmember head;
	struct wmember *wp;

	head.wm_forw = head.wm_back = &head;

	sprintf( pilname, "%s%d,%d", prefix, ix, iy );
	sprintf( rname, "%s.r", pilname );
	sprintf( sname, "%s.s", pilname );
	sprintf( oname, "Obj%d,%d", ix, iy );

	VMOVE( min, center );
	min[X] -= lwh[X];
	min[Y] -= lwh[Y];
	VADD2( max, center, lwh );
	mk_rpp( stdout, sname, min, max );

	/* Needs to be in a region, with color!  */
	get_rgb(rgb);
	i = PICK_MAT;
	mk_mcomb( stdout, rname, 1, 1,
		mtab[i].mt_name, mtab[i].mt_param, 1, rgb );
	mk_memb( stdout, sname, identity, UNION );

	(void)mk_addmember( rname, &head );
	wp = mk_addmember( oname, &head );
	MAT_DELTAS( wp->wm_mat, center[X], center[Y], center[Z]+lwh[Z] );
	mk_lfcomb( stdout, pilname, 0, &head );

	(void)mk_addmember( pilname, headp );
}

void
make_carpet( rname, min, max, file, headp )
char	*rname;
vect_t	min, max;
char	*file;
struct wmember *headp;
{
	char	sname[32];
	char	args[128];
	vect_t	cmin, cmax;

	VMOVE( cmin, min );
	VMOVE( cmax, max );
	cmax[Z] = cmin[Z] + 10;		/* not very plush carpet */
	min[Z] = cmax[Z];		/* raise the caller's floor */

	sprintf( sname, "%s.s", rname );
	sprintf( args, "texture file=%s;plastic", file );
	mk_rpp( stdout, sname, cmin, cmax );
	mk_mcomb( stdout, rname, 1, 1,
		"stack", args,
		0, (char *)0 );
	mk_memb( stdout, sname, identity, UNION );

	(void)mk_addmember( rname, headp );
}
