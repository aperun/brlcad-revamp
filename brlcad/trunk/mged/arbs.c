/*
 *			A R B S . C
 *
 *	f_3ptarb()	finds arb8 defined by 3 points, 2 coordinates of
 *			a 4th point, and a thickness
 *
 *	f_rfarb()	finds arb8 defined by rot & fallback angles, one point,
 *			2 coordinates of the 3 remaining points, and a thickness
 *
 *  Author -
 *	Keith A. Applin
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1986 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"

#include <signal.h>
#include <stdio.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "rtlist.h"
#include "rtgeom.h"
#include "raytrace.h"
#include "externs.h"
#include "./ged.h"
#include "./dm.h"

extern int	newargs;
extern char	**promp;

static int	cgarbs();

char *p_arb3pt[] = {
	"Enter X, Y, Z for point 1: ",
	"Enter Y, Z: ",
	"Enter Z: ",
	"Enter X, Y, Z for point 2: ",
	"Enter Y, Z: ",
	"Enter Z: ",
	"Enter X, Y, Z for point 3: ",
	"Enter Y, Z: ",
	"Enter Z: "
};

/*
 *
 *	F _ 3 P T A R B ( ) :	finds arb8 given.....
 *				1.  3 points of one face
 *				2.  2 coordinates of the 4th point in that face
 *				3.  thickness
 *
 */
int
f_3ptarb( argc, argv )
int	argc;
char	**argv;
{
	int			i, solve;
	int			ngran;
	vect_t			vec1;
	vect_t			vec2;
	fastf_t			pt4[2], length, thick;
	vect_t			norm;
	fastf_t			ndotv;
	char			name[NAMESIZE+2];
	struct directory	*dp;
	struct rt_db_internal	internal;
	struct rt_external	external;
	struct rt_arb_internal	*aip;

	/* interupts */
	(void)signal( SIGINT, sig2 );

	/* get the arb name */
	if( argc < 2 ) {
		rt_log("Enter name for this arb: ");
		return CMD_MORE;
	}

	if( db_lookup( dbip, argv[1], LOOKUP_QUIET) != DIR_NULL ) {
		rt_log("%s:  already exists\n", argv[1]);
		return CMD_BAD;
	}

	if( (int)strlen(argv[1]) >= NAMESIZE ) {
		rt_log("Names are limited to %d charscters\n",NAMESIZE-1);
		return CMD_BAD;
	}
	strcpy( name , argv[1] );

	/* read the three points */
	promp = &p_arb3pt[0];
	if( argc < 11 ) {
		rt_log("%s", promp[argc-2]);
		return CMD_MORE;
	}

	/* preliminary calculations to check input so far */
	for(i=0; i<3; i++) {
		vec1[i] = atof(argv[i+2]) - atof(argv[i+5]);
		vec2[i] = atof(argv[i+2]) - atof(argv[i+8]);
	}
	VCROSS(norm, vec1, vec2);
	length = MAGNITUDE( norm );
	if(length == 0.0) {
		rt_log("points are colinear\n");
		return CMD_BAD;
	}
	VSCALE(norm, norm, 1.0/length);

	if( argc < 12 ) {
		rt_log("Enter coordinate to solve for (x, y, or z): ");
		return CMD_MORE;
	}

	switch( argv[11][0] ) {

	case 'x':
		if(norm[0] == 0.0) {
			rt_log("X not unique in this face\n");
			return CMD_BAD;
		}
		solve = X;

		if( argc < 13 ) {			
			rt_log("Enter the Y, Z coordinate values: ");
			return CMD_MORE;
		}
		if( argc < 14 ) {
			rt_log("Enter the Z coordinate value: ");
			return CMD_MORE;
		}
		pt4[0] = atof( argv[12] ) * local2base;
		pt4[1] = atof( argv[13] ) * local2base;
		break;

	case 'y':
		if(norm[1] == 0.0) {
			rt_log("Y not unique in this face\n");
			return CMD_BAD;
		}
		solve = Y;

		if ( argc < 13 ) {
			rt_log("Enter the X, Z coordinate values: ");
			return CMD_MORE;
		}
		if ( argc < 14 ) {
			rt_log("Enter the Z coordinate value: ");
			return CMD_MORE;
		}

		pt4[0] = atof( argv[12] ) * local2base;
		pt4[1] = atof( argv[13] ) * local2base;
		break;

	case 'z':
		if(norm[2] == 0.0) {
			rt_log("Z not unique in this face\n");
			return CMD_BAD;
		}
		solve = Z;

		if ( argc < 13 ) {
			rt_log("Enter the X, Y coordinate values: ");
			return CMD_MORE;
		}
		if ( argc < 14 ) {
			rt_log("Enter the Y coordinate value: ");
			return CMD_MORE;
		}
		pt4[0] = atof( argv[12] ) * local2base;
		pt4[1] = atof( argv[13] ) * local2base;
		break;

	default:
		rt_log("coordinate must be x, y, or z\n");
		return CMD_BAD;
	}


	if ( argc < 15 ) {
		rt_log("Enter thickness for this arb: ");
		return CMD_MORE;
	}

	if( (thick = (atof( argv[14] ))) == 0.0 ) {
		rt_log("thickness = 0.0\n");
		return CMD_BAD;
	}

	RT_INIT_DB_INTERNAL( &internal );
	internal.idb_type = ID_ARB8;
	internal.idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_arb_internal) , "rt_arb_internal" );
	aip = (struct rt_arb_internal *)internal.idb_ptr;
	aip->magic = RT_ARB_INTERNAL_MAGIC;

	for(i=0; i<8; i++) {
		VSET( aip->pt[i] , 0.0 , 0.0 , 0.0 );
	}

	for(i=0; i<3; i++) {
		/* the three given vertices */
		VSET( aip->pt[i] , atof( argv[i*3+2] )*local2base , atof( argv[i*3+3] )*local2base , atof( argv[i*3+4] )*local2base );
	}

	thick *= local2base;

	ndotv = VDOT( aip->pt[0], norm );

	switch( solve ) {

		case X:
			/* solve for x-coord of 4th point */
			aip->pt[3][Y] = pt4[0];		/* y-coord */
			aip->pt[3][Z] = pt4[1]; 	/* z-coord */
			aip->pt[3][X] =  ( ndotv
					- norm[Y] * aip->pt[3][Y]
					- norm[Z] * aip->pt[3][Z])
					/ norm[X];	/* x-coord */
		break;

		case Y:
			/* solve for y-coord of 4th point */
			aip->pt[3][X] = pt4[0];		/* x-coord */
			aip->pt[3][Z] = pt4[1]; 	/* z-coord */
			aip->pt[3][Y] =  ( ndotv
					- norm[X] * aip->pt[3][X]
					- norm[Z] * aip->pt[3][Z])
					/ norm[Y];	/* y-coord */
		break;

		case Z:
			/* solve for z-coord of 4th point */
			aip->pt[3][X] = pt4[0];		/* x-coord */
			aip->pt[3][Y] = pt4[1]; 	/* y-coord */
			aip->pt[3][Z] =  ( ndotv
					- norm[X] * aip->pt[3][X]
					- norm[Y] * aip->pt[3][Y])
					/ norm[Z];	/* z-coord */
		break;

		default:
			rt_log("bad coordinate to solve for\n");
			return CMD_BAD;

	}

	/* calculate the remaining 4 vertices */
	for(i=0; i<4; i++) {
		VJOIN1( aip->pt[i+4] , aip->pt[i] , thick , norm );
	}

	/* Don't use local2base here, coordinates are already converted to mm above */
	if( rt_functab[internal.idb_type].ft_export( &external, &internal, (double)1.0 ) < 0 )
	{
		rt_log( "f_3ptarb: export failure\n" );
		rt_functab[internal.idb_type].ft_ifree( &internal );
		return CMD_BAD;
	}
	rt_functab[internal.idb_type].ft_ifree( &internal );   /* free internal rep */

	/* no interuprts */
	(void)signal( SIGINT, SIG_IGN );

	ngran = (external.ext_nbytes+sizeof(union record)-1) / sizeof(union record);
	if( (dp = db_diradd( dbip, name, -1L, ngran, DIR_SOLID)) == DIR_NULL ||
	    db_alloc( dbip, dp, 1 ) < 0 ) {
	    	db_free_external( &external );
	    	ALLOC_ERR;
		return CMD_BAD;
	}

	if (db_put_external( &external, dp, dbip ) < 0 ) 
	{
		db_free_external( &external );
		WRITE_ERR;
		return CMD_BAD;
	}
	db_free_external( &external );

	/* draw the "made" solid */
	return f_edit( 2, argv ); /* depends on solid name being in argv[1] */
}



char *p_rfin[] = {
	"Enter X, Y, Z of the known point: ",
	"Enter Y,Z: ",
	"Enter Z: "
};

/*	F _ R F A R B ( ) :	finds arb8 given.....
 *
 *		1. one point
 *		2. 2 coordinates of 3 other points
 *		3. rot and fallback angles
 *		4. thickness
 */
int
f_rfarb( argc, argv )
int	argc;
char	**argv;
{
	struct directory	*dp;
	int			i;
	int			solve[3];
	int			ngran;
	char			name[NAMESIZE+2];
	fastf_t			pt[3][2];
	fastf_t			thick, rota, fba;
	vect_t			norm;
	fastf_t			ndotv;
	struct rt_db_internal	internal;
	struct rt_external	external;
	struct rt_arb_internal	*aip;

	/* interupts */
	(void)signal( SIGINT, sig2 );

	/* get the arb name */
	if( argc < 2 ) {
		rt_log("Enter name for this arb: ");
		return CMD_MORE;
	}
	if( db_lookup( dbip, argv[1], LOOKUP_QUIET) != DIR_NULL ) {
		rt_log("%s:  already exists\n", argv[1]);
		return CMD_BAD;
	}

	if( (int)strlen(argv[1]) >= NAMESIZE ) {
		rt_log("Names are limited to %d charscters\n",NAMESIZE-1);
		return CMD_BAD;
	}
	strcpy( name , argv[1] );

	/* read the known point */
	promp = &p_rfin[0];
	if( argc < 5 ) {
		rt_log("%s", promp[argc-2]);
		return CMD_MORE;
	}

	if ( argc < 6 ) {
		rt_log("Enter ROTATION angle (deg): ");
		return CMD_MORE;
	}

	rota = atof( argv[5] ) * degtorad;

	if ( argc < 7 ) {
		rt_log("Enter FALL BACK angle (deg): ");
		return CMD_MORE;
	}

	fba = atof( argv[6] ) * degtorad;

	/* calculate plane defined by these angles */
	norm[0] = cos(fba) * cos(rota);
	norm[1] = cos(fba) * sin(rota);
	norm[2] = sin(fba);

	for(i=0; i<3; i++) {
		if( argc < 8+3*i ) {
			rt_log("POINT %d...",i+2);
			rt_log("Enter coordinate to solve for (x, y, or z): ");
			return CMD_MORE;
		}

		switch( argv[7+3*i][0] ) {
		case 'x':
			if(norm[0] == 0.0) {
				rt_log("X not unique in this face\n");
				return CMD_BAD;
			}
			solve[i] = X;

			if( argc < 7+3*i+2 ) {
				rt_log("Enter the Y, Z coordinate values: ");
				return CMD_MORE;
			}
			if( argc < 7+3*i+3 ) {
				rt_log("Enter the Z coordinate value: ");
				return CMD_MORE;
			}
			pt[i][0] = atof( argv[7+3*i+1] ) * local2base;
			pt[i][1] = atof( argv[7+3*i+2] ) * local2base;
			break;

		case 'y':
			if(norm[1] == 0.0) {
				rt_log("Y not unique in this face\n");
				return CMD_BAD;
			}
			solve[i] = Y;

			if( argc < 7+3*i+2 ) {
				rt_log("Enter the X, Z coordinate values: ");
				return CMD_MORE;
			}
			if( argc < 7+3*i+3 ) {
				rt_log("Enter the Z coordinate value: ");							
				return CMD_MORE;
			}
			pt[i][0] = atof( argv[7+3*i+1] ) * local2base;
			pt[i][1] = atof( argv[7+3*i+2] ) * local2base;
			break;

		case 'z':
			if(norm[2] == 0.0) {
				rt_log("Z not unique in this face\n");
				return CMD_BAD;
			}
			solve[i] = Z;

			if( argc < 7+3*i+2 ) {
				rt_log("Enter the X, Y coordinate values: ");
				return CMD_MORE;
			}
			if( argc < 7+3*i+3 ) {
				rt_log("Enter the Y coordinate value: ");
				return CMD_MORE;
			}
			pt[i][0] = atof( argv[7+3*i+1] ) * local2base;
			pt[i][1] = atof( argv[7+3*i+2] ) * local2base;
			break;

		default:
			rt_log("coordinate must be x, y, or z\n");
			return CMD_BAD;
		}
	}

	if( argc < 8+3*3 ) {
		rt_log("Enter thickness for this arb: ");
		return CMD_MORE;
	}
	if( (thick = (atof( argv[7+3*3] ))) == 0.0 ) {
		rt_log("thickness = 0.0\n");
		return CMD_BAD;
	}
	thick *= local2base;

	RT_INIT_DB_INTERNAL( &internal );
	internal.idb_type = ID_ARB8;
	internal.idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_arb_internal) , "rt_arb_internal" );
	aip = (struct rt_arb_internal *)internal.idb_ptr;
	aip->magic = RT_ARB_INTERNAL_MAGIC;

	for(i=0; i<8; i++) {
		VSET( aip->pt[i] , 0.0 , 0.0 , 0.0 );
	}

	VSET( aip->pt[0] , atof(argv[2])*local2base , atof(argv[3])*local2base , atof(argv[4])*local2base );

	ndotv = VDOT( aip->pt[0], norm );

	/* calculate the unknown coordinate for points 2,3,4 */
	for(i=0; i<3; i++) {
		int j;
		j = i+1;

		switch( solve[i] ) {
			case X:
				aip->pt[j][Y] = pt[i][0];
				aip->pt[j][Z] = pt[i][1];
				aip->pt[j][X] = ( ndotv
					- norm[1] * aip->pt[j][Y]
					- norm[2] * aip->pt[j][Z])
					/ norm[0];
			break;
			case Y:
				aip->pt[j][X] = pt[i][0];
				aip->pt[j][Z] = pt[i][1];
				aip->pt[j][Y] = ( ndotv
					- norm[0] * aip->pt[j][X]
					- norm[2] * aip->pt[j][Z])
					/ norm[1];
			break;
			case Z:
				aip->pt[j][X] = pt[i][0];
				aip->pt[j][Y] = pt[i][1];
				aip->pt[j][Z] = ( ndotv
					- norm[0] * aip->pt[j][X]
					- norm[1] * aip->pt[j][Y])
					/ norm[2];
			break;

			default:
				return CMD_BAD;
		}
	}

	/* calculate the remaining 4 vertices */
	for(i=0; i<4; i++) {
		VJOIN1( aip->pt[i+4] , aip->pt[i] , thick , norm );
	}

	if( rt_functab[internal.idb_type].ft_export( &external, &internal, local2base ) < 0 )
	{
		rt_log( "f_3ptarb: export failure\n" );
		rt_functab[internal.idb_type].ft_ifree( &internal );
		return CMD_BAD;
	}
	rt_functab[internal.idb_type].ft_ifree( &internal );   /* free internal rep */

	/* no interuprts */
	(void)signal( SIGINT, SIG_IGN );

	ngran = (external.ext_nbytes+sizeof(union record)-1) / sizeof(union record);
	if( (dp = db_diradd( dbip, name, -1L, ngran, DIR_SOLID)) == DIR_NULL ||
	    db_alloc( dbip, dp, 1 ) < 0 )
	    {
	    	db_free_external( &external );
	    	ALLOC_ERR;
		return CMD_BAD;
	    }

	if (db_put_external( &external, dp, dbip ) < 0 )
	{
		db_free_external( &external );
		WRITE_ERR;
		return CMD_BAD;
	}
	db_free_external( &external );

	/* draw the "made" solid */
	return f_edit( 2, argv );	/* depends on solid name being in argv[1] */
}

/* ------------------------------ old way ------------------------------ */

static void	points();

static union record input;		/* Holds an object file record */

/* TYPE_ARB()	returns specific ARB type of record rec.  The record rec
 *		is also rearranged to "standard" form.
 */
type_arb( rec )
union record *rec;
{
	int i;
	static int uvec[8], svec[11];

	if( rec->s.s_type != GENARB8 )
		return( 0 );

	input = *rec;		/* copy */

	/* convert input record to points */
	points();

	if( cgarbs(uvec, svec) == 0 )
		return(0);

	/* convert to vectors in the rec record */
	VMOVE(&rec->s.s_values[0], &input.s.s_values[0]);
	for(i=3; i<=21; i+=3) {
		VSUB2(&rec->s.s_values[i], &input.s.s_values[i], &input.s.s_values[0]);
	}

	return( input.s.s_cgtype );

}

#define NO	0
#define YES	1
	
/*
 * C G A R B S :   determines COMGEOM arb types from GED general arbs
 */
static int
cgarbs( uvec, svec )
register int *uvec;	/* array of unique points */
register int *svec;	/* array of like points */
{
	register int i,j;
	static int numuvec, unique, done;
	static int si;

	done = NO;		/* done checking for like vectors */

	svec[0] = svec[1] = 0;
	si = 2;

	for(i=0; i<7; i++) {
		unique = YES;
		if(done == NO)
			svec[si] = i;
		for(j=i+1; j<8; j++) {
			int tmp;
			vect_t vtmp;

			VSUB2( vtmp, &input.s.s_values[i*3], &input.s.s_values[j*3] );

			if( fabs(vtmp[0]) > 0.0001) tmp = 0;
			else 	if( fabs(vtmp[1]) > 0.0001) tmp = 0;
			else 	if( fabs(vtmp[2]) > 0.0001) tmp = 0;
			else tmp = 1;

			if( tmp ) {
				if( done == NO )
					svec[++si] = j;
				unique = NO;
			}
		}
		if( unique == NO ) {  	/* point i not unique */
			if( si > 2 && si < 6 ) {
				svec[0] = si - 1;
				if(si == 5 && svec[5] >= 6)
					done = YES;
				si = 6;
			}
			if( si > 6 ) {
				svec[1] = si - 5;
				done = YES;
			}
		}
	}

	if( si > 2 && si < 6 ) 
		svec[0] = si - 1;
	if( si > 6 )
		svec[1] = si - 5;
	for(i=1; i<=svec[1]; i++)
		svec[svec[0]+1+i] = svec[5+i];
	for(i=svec[0]+svec[1]+2; i<11; i++)
		svec[i] = -1;
	/* find the unique points */
	numuvec = 0;
	for(j=0; j<8; j++) {
		unique = YES;
		for(i=2; i<svec[0]+svec[1]+2; i++) {
			if( j == svec[i] ) {
				unique = NO;
				break;
			}
		}
		if( unique == YES )
			uvec[numuvec++] = j;
	}

	/* put comgeom solid typpe into s_cgtype */
	switch( numuvec ) {

	case 8:
		input.s.s_cgtype = ARB8;  /* ARB8 */
		break;

	case 6:
		input.s.s_cgtype = ARB7;	/* ARB7 */
		break;

	case 4:
		if(svec[0] == 2)
			input.s.s_cgtype = ARB6;	/* ARB6 */
		else
			input.s.s_cgtype = ARB5;	/* ARB5 */
		break;

	case 2:
		input.s.s_cgtype = ARB4;	/* ARB4 */
		break;

	default:
		rt_log("solid: %s  bad number of unique vectors (%d)\n",
			input.s.s_name, numuvec);
		return(0);
	}
	return( numuvec );
}

static void
points()
{
	register int i;

	for(i=3; i<=21; i+=3) {
		VADD2(&input.s.s_values[i],&input.s.s_values[i],&input.s.s_values[0]);
	}
}

/* ------------------------------ new way ------------------------------ */
/* XXX This should move to librt/g_arb.c at some point */

#define NO	0
#define YES	1
	
/*
 *			R T _ A R B _ G E T _ C G T Y P E
 *
 * C G A R B S :   determines COMGEOM arb types from GED general arbs
 *
 *  Inputs -
 *
 *  Returns -
 *	#	Number of distinct edge vectors
 *		(Number of entries in uvec array)
 *
 *  Implicit returns -
 *	*cgtype		Comgeom type (number range 4..8;  ARB4 .. ARB8).
 *	uvec[8]
 *	svec[11]
 *			Entries [0] and [1] are special
 */
int
rt_arb_get_cgtype( cgtype, arb, tol, uvec, svec )
int			*cgtype;
struct rt_arb_internal	*arb;
CONST struct rt_tol	*tol;
register int *uvec;	/* array of unique points */
register int *svec;	/* array of like points */
{
	register int i,j;
	int	numuvec, unique, done;
	int	si;

	RT_ARB_CK_MAGIC(arb);
	RT_CK_TOL(tol);

	done = NO;		/* done checking for like vectors */

	svec[0] = svec[1] = 0;
	si = 2;

	for(i=0; i<7; i++) {
		unique = YES;
		if(done == NO)
			svec[si] = i;
		for(j=i+1; j<8; j++) {
			int tmp;
			vect_t vtmp;

			VSUB2( vtmp, arb->pt[i], arb->pt[j] );

			if( fabs(vtmp[0]) > tol->dist) tmp = 0;
			else 	if( fabs(vtmp[1]) > tol->dist) tmp = 0;
			else 	if( fabs(vtmp[2]) > tol->dist) tmp = 0;
			else tmp = 1;

			if( tmp ) {
				if( done == NO )
					svec[++si] = j;
				unique = NO;
			}
		}
		if( unique == NO ) {  	/* point i not unique */
			if( si > 2 && si < 6 ) {
				svec[0] = si - 1;
				if(si == 5 && svec[5] >= 6)
					done = YES;
				si = 6;
			}
			if( si > 6 ) {
				svec[1] = si - 5;
				done = YES;
			}
		}
	}

	if( si > 2 && si < 6 ) 
		svec[0] = si - 1;
	if( si > 6 )
		svec[1] = si - 5;
	for(i=1; i<=svec[1]; i++)
		svec[svec[0]+1+i] = svec[5+i];
	for(i=svec[0]+svec[1]+2; i<11; i++)
		svec[i] = -1;

	/* find the unique points */
	numuvec = 0;
	for(j=0; j<8; j++) {
		unique = YES;
		for(i=2; i<svec[0]+svec[1]+2; i++) {
			if( j == svec[i] ) {
				unique = NO;
				break;
			}
		}
		if( unique == YES )
			uvec[numuvec++] = j;
	}

	/* Figure out what kind of ARB this is */
	switch( numuvec ) {

	case 8:
		*cgtype = ARB8;		/* ARB8 */
		break;

	case 6:
		*cgtype = ARB7;		/* ARB7 */
		break;

	case 4:
		if(svec[0] == 2)
			*cgtype = ARB6;	/* ARB6 */
		else
			*cgtype = ARB5;	/* ARB5 */
		break;

	case 2:
		*cgtype = ARB4;		/* ARB4 */
		break;

	default:
		rt_log("rt_arb_get_cgtype: bad number of unique vectors (%d)\n",
			numuvec);
		return(0);
	}
#if 0
	rt_log("uvec: ");
	for(j=0; j<8; j++) rt_log("%d, ", uvec[j]);
	rt_log("\nsvec: ");
	for(j=0; j<11; j++ ) rt_log("%d, ", svec[j]);
	rt_log("\n");
#endif
	return( numuvec );
}

/*
 *			R T _ A R B _ S T D _ T Y P E
 *
 *  Given an ARB in internal form, return it's specific ARB type.
 *
 *  Set tol.dist = 0.0001 to obtain past behavior.
 *
 *  Returns -
 *	0	Error in input ARB
 *	4	ARB4
 *	5	ARB5
 *	6	ARB6
 *	7	ARB7
 *	8	ARB8
 *
 *  Implicit return -
 *	rt_arb_internal pt[] array reorganized into GIFT "standard" order.
 */
int
rt_arb_std_type( ip, tol )
struct rt_db_internal	*ip;
CONST struct rt_tol	*tol;
{
	struct rt_arb_internal	*arb;
	int uvec[8], svec[11];
	int	cgtype = 0;

	RT_CK_DB_INTERNAL(ip);
	RT_CK_TOL(tol);

	if( ip->idb_type != ID_ARB8 )  rt_bomb("rt_arb_std_type: not ARB!\n");

	arb = (struct rt_arb_internal *)ip->idb_ptr;
	RT_ARB_CK_MAGIC(arb);

	if( rt_arb_get_cgtype( &cgtype, arb, tol, uvec, svec ) == 0 )
		return(0);

	return( cgtype );
}


/* 
 *			R T _ A R B _ C E N T R O I D
 *
 * Find the center point for the arb whose values are in the s array,
 * with the given number of verticies.  Return the point in center_pt.
 * WARNING: The s array is dbfloat_t's not fastf_t's.
 */
void
rt_arb_centroid( center_pt, arb, npoints )
point_t			center_pt;
struct rt_arb_internal	*arb;
int			npoints;
{
	register int	j;
	fastf_t		div;
	point_t		sum;

	RT_ARB_CK_MAGIC(arb);

	VSETALL(sum, 0);

	for( j=0; j < npoints; j++ )  {
		VADD2( sum, sum, arb->pt[j] );
	}
	div = 1.0 / npoints;
	VSCALE( center_pt, sum, div );
}
