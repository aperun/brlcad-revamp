/*
 *			I N S I D E 
 *
 *	Given an outside solid and desired thicknesses, finds
 *	an inside solid to produce those thicknesses.
 *
 * Functions -
 *	f_inside	reads all the input required
 *	arbin		finds inside of arbs
 *	tgcin		finds inside of tgcs
 *	ellgin		finds inside of ellgs
 *	torin		finds inside of tors
 *
 *  Authors -
 *	Keith A Applin
 *	Michael Markowski
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"

#include <signal.h>
#include <stdio.h>
#include <math.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "machine.h"
#include "vmath.h"
#include "db.h"			/* XXX needed for NAMESIZE */
#include "rtlist.h"
#include "nmg.h"
#include "rtgeom.h"
#include "raytrace.h"
#include "./ged.h"
#include "./sedit.h"
#include "externs.h"
#include "./solid.h"
#include "./dm.h"

RT_EXTERN( void nmg_invert_shell , ( struct shell *s , struct rt_tol *tol ) );
RT_EXTERN( struct shell *nmg_extrude_shell , ( struct shell *s, fastf_t thick , int normal_ward , int approximate , struct rt_tol *tol ) );

extern struct rt_db_internal	es_int;	/* from edsol.c */
extern struct rt_tol		mged_tol;	/* from ged.c */

extern char	**promp;	/* pointer to a pointer to a char */

static char *p_arb4[] = {
	"Enter thickness for face 123: ",
	"Enter thickness for face 124: ",
	"Enter thickness for face 234: ",
	"Enter thickness for face 134: ",
};

static char *p_arb5[] = {
	"Enter thickness for face 1234: ",
	"Enter thickness for face 125: ",
	"Enter thickness for face 235: ",
	"Enter thickness for face 345: ",
	"Enter thickness for face 145: ",
};

static char *p_arb6[] = {
	"Enter thickness for face 1234: ",
	"Enter thickness for face 2356: ",
	"Enter thickness for face 1564: ",
	"Enter thickness for face 125: ",
	"Enter thickness for face 346: ",
};

static char *p_arb7[] = {
	"Enter thickness for face 1234: ",
	"Enter thickness for face 567: ",
	"Enter thickness for face 145: ",
	"Enter thickness for face 2376: ",
	"Enter thickness for face 1265: ",
	"Enter thickness for face 3475: ",
};

static char *p_arb8[] = {
	"Enter thickness for face 1234: ",
	"Enter thickness for face 5678: ",
	"Enter thickness for face 1485: ",
	"Enter thickness for face 2376: ",
	"Enter thickness for face 1265: ",
	"Enter thickness for face 3478: ",
};

static char *p_tgcin[] = {
	"Enter thickness for base (AxB): ",
	"Enter thickness for top (CxD): ",
	"Enter thickness for side: ",
};

static char *p_rpcin[] = {
	"Enter thickness for front plate (contains V): ",
	"Enter thickness for back plate: ",
	"Enter thickness for top plate: ",
	"Enter thickness for body: ",
};

static char *p_rhcin[] = {
	"Enter thickness for front plate (contains V): ",
	"Enter thickness for back plate: ",
	"Enter thickness for top plate: ",
	"Enter thickness for body: ",
};

static char *p_epain[] = {
	"Enter thickness for top plate: ",
	"Enter thickness for body: ",
};

static char *p_ehyin[] = {
	"Enter thickness for top plate: ",
	"Enter thickness for body: ",
};

static char *p_etoin[] = {
	"Enter thickness for body: ",
};

static char *p_nmgin[] = {
	"Enter thickness for shell: ",
};

/*	F _ I N S I D E ( ) :	control routine...reads all data
 */
int
f_inside(argc, argv)
int argc;
char **argv;
{
	register int i;
	struct directory	*dp;
	struct directory	*outdp;
	mat_t newmat;
	int	cgtype;		/* cgtype ARB 4..8 */
	int	p1, p2, p3;
	int	nface;
	fastf_t	thick[6];
	plane_t	planes[6];
	struct rt_db_internal	intern;
	char	*newname;
	struct rt_tol	tol;

	int arg = 1;

	/* XXX These need to be improved */
	tol.magic = RT_TOL_MAGIC;
	tol.dist = 0.005;
	tol.dist_sq = tol.dist * tol.dist;
	tol.perp = 1e-6;
	tol.para = 1 - tol.perp;

	(void)signal( SIGINT, sig2);	/* allow interrupts */

	/* SCHEME:
	 *	if in solid edit, use "edited" solid
	 *	if in object edit, use "key" solid
	 *	else get solid name to use
	 */

	if( state == ST_S_EDIT ) {
		/* solid edit mode */
		/* apply es_mat editing to parameters */
		transform_editing_solid( &intern, es_mat, &es_int, 0 );
		outdp = illump->s_path[illump->s_last];

		(void)printf("Outside solid: ");
		for(i=0; i <= illump->s_last; i++) {
			(void)printf("/%s",illump->s_path[i]->d_namep);
		}
		(void)printf("\n");
	}  else if( state == ST_O_EDIT ) {
		/* object edit mode */
		if( illump->s_Eflag ) {
			(void)printf("Cannot find inside of a processed (E'd) region\n");
			return CMD_BAD;
		}
		/* use the solid at bottom of path (key solid) */
		/* apply es_mat and modelchanges editing to parameters */
		mat_mul(newmat, modelchanges, es_mat);
		transform_editing_solid( &intern, newmat, &es_int, 0 );
		outdp = illump->s_path[illump->s_last];

		(void)printf("Outside solid: ");
		for(i=0; i <= illump->s_last; i++) {
			(void)printf("/%s",illump->s_path[i]->d_namep);
		}
		(void)printf("\n");
	} else {
		/* Not doing any editing....ask for outside solid */
		if( argc < arg+1 ) {
			(void)printf("Enter name of outside solid: ");
			return CMD_MORE;
		}
		if( (outdp = db_lookup( dbip,  argv[arg], LOOKUP_NOISY )) == DIR_NULL )  
			return CMD_BAD;
		++arg;

		if( rt_db_get_internal( &intern, outdp, dbip, rt_identity ) < 0 ) {
			READ_ERR;
			return CMD_BAD;
		}
	}

	if( intern.idb_type == ID_ARB8 )  {
		/* find the comgeom arb type, & reorganize */
		int uvec[8],svec[8];

		if( rt_arb_get_cgtype( &cgtype , intern.idb_ptr, &tol , uvec , svec ) == 0 ) {
			rt_log("%s: BAD ARB\n",outdp->d_namep);
			return CMD_BAD;
		}

		/* must find new plane equations to account for
		 * any editing in the es_mat matrix or path to this solid.
		 */
		if( rt_arb_calc_planes( planes, intern.idb_ptr, cgtype, &tol ) < 0 )  {
			rt_log("rt_arb_calc_planes(%s): failed\n", outdp->d_namep);
			return CMD_BAD;
		}
	}
	/* "intern" is now loaded with the outside solid data */

	/* get the inside solid name */
	if( argc < arg+1 ) {
		(void)printf("Enter name of the inside solid: ");
		return CMD_MORE;
	}
	if( db_lookup( dbip, argv[arg], LOOKUP_QUIET ) != DIR_NULL ) {
		aexists( argv[arg] );
		return CMD_BAD;
	}
	if( (int)strlen(argv[arg]) >= NAMESIZE )  {
		(void)printf("Names are limited to %d characters\n", NAMESIZE-1);
		return CMD_BAD;
	}
	newname = argv[arg];
	++arg;

	/* get thicknesses and calculate parameters for newrec */
	switch( intern.idb_type )  {

	case ID_ARB8:
	    {
	    	struct rt_arb_internal *arb =
			(struct rt_arb_internal *)intern.idb_ptr;

		nface = 6;

		switch( cgtype ) {
		case 8:
			promp = p_arb8;
			break;

		case 7:
			promp = p_arb7;
			break;

		case 6:
			promp = p_arb6;
			nface = 5;
			VMOVE( arb->pt[5], arb->pt[6] );
			break;

		case 5:
			promp = p_arb5;
			nface = 5;
			break;

		case 4:
			promp = p_arb4;
			nface = 4;
			VMOVE( arb->pt[3], arb->pt[4] );
			break;
		}

		for(i=0; i<nface; i++) {
			if( argc < arg+1 ) {
				(void)printf("%s",promp[i]);
				return CMD_MORE;
			}
			thick[i] = atof(argv[arg]) * local2base;
			++arg;
		}

		if( arbin(&intern, thick, nface, cgtype, planes) )
			return CMD_BAD;
		break;
	    }

	case ID_TGC:
		promp = p_tgcin;
		for(i=0; i<3; i++) {
			if( argc < arg+1 ) {
				(void)printf("%s",promp[i]);
				return CMD_MORE;
			}
			thick[i] = atof( argv[arg] ) * local2base;
			++arg;
		}

		if( tgcin(&intern, thick) )
			return CMD_BAD;
		break;

	case ID_ELL:
		if( argc < arg+1 ) {
			(void)printf("Enter desired thickness: ");
			return CMD_MORE;
		}
		thick[0] = atof( argv[arg] ) * local2base;
		++arg;

		if( ellgin(&intern, thick) )
			return CMD_BAD;
		break;

	case ID_TOR:
		if( argc < arg+1 ) {
			(void)printf("Enter desired thickness: ");
			return CMD_MORE;
		}
		thick[0] = atof( argv[arg] ) * local2base;
		++arg;

		if( torin(&intern, thick) )
			return CMD_BAD;
		break;

	case ID_RPC:
		promp = p_rpcin;
		for (i = 0; i < 4; i++) {
			if( argc < arg+1 ) {
				(void)printf("%s",promp[i]);
				return CMD_MORE;
			}
			thick[i] = atof( argv[arg] ) * local2base;
			++arg;
		}

		if( rpcin(&intern, thick) )
			return CMD_BAD;
		break;

	case ID_RHC:
		promp = p_rhcin;
		for (i = 0; i < 4; i++) {
			if( argc < arg+1 ) {
				(void)printf("%s",promp[i]);
				return CMD_MORE;
			}
			thick[i] = atof( argv[arg] ) * local2base;
			++arg;
		}

		if( rhcin(&intern, thick) )
			return CMD_BAD;
		break;

	case ID_EPA:
		promp = p_epain;
		for (i = 0; i < 2; i++) {
			if( argc < arg+1 ) {
				(void)printf("%s",promp[i]);
				return CMD_MORE;
			}
			thick[i] = atof( argv[arg] ) * local2base;
			++arg;
		}

		if( epain(&intern, thick) )
			return CMD_BAD;
		break;

	case ID_EHY:
		promp = p_ehyin;
		for (i = 0; i < 2; i++) {
			if( argc < arg+1 ) {
				(void)printf("%s",promp[i]);
				return CMD_MORE;
			}
			thick[i] = atof( argv[arg] ) * local2base;
			++arg;
		}

		if( ehyin(&intern, thick) )
			return CMD_BAD;
		break;

	case ID_ETO:
		promp = p_etoin;
		for (i = 0; i < 1; i++) {
			if( argc < arg+1 ) {
				(void)printf("%s",promp[i]);
				return CMD_MORE;
			}
			thick[i] = atof( argv[arg] ) * local2base;
			++arg;
		}

		if( etoin(&intern, thick) )
			return CMD_BAD;
		break;

	case ID_NMG:
		promp = p_nmgin;
		if( argc < arg+1 ) {
			(void)printf( "%s" , promp[0] );
			return CMD_MORE;
		}
		thick[0] = atof( argv[arg] ) * local2base;
		++arg;
		if( nmgin( &intern , thick[0] ) )
			return CMD_BAD;
		break;

	default:
		(void)printf("Cannot find inside for '%s' solid\n",
			rt_functab[intern.idb_type].ft_name );
		return CMD_BAD;
	}

	/* don't allow interrupts while we update the database! */
	(void)signal( SIGINT, SIG_IGN);
 
	/* Add to in-core directory */
	if( (dp = db_diradd( dbip,  newname, -1, 0, DIR_SOLID )) == DIR_NULL )  {
	    	ALLOC_ERR;
		return CMD_BAD;
	}
	if( rt_db_put_internal( dp, dbip, &intern ) < 0 ) {
		WRITE_ERR;
		return CMD_BAD;
	}

	/* Draw the new solid */
	{
		char	*arglist[3];
		arglist[0] = "e";
		arglist[1] = newname;
		return f_edit( 2, arglist );
	}
}



/* finds inside arbs */
int
arbin(ip, thick, nface, cgtype, planes)
struct rt_db_internal	*ip;
fastf_t	thick[6];
int	nface;
int	cgtype;		/* # of points, 4..8 */
plane_t	planes[6];
{
	struct rt_arb_internal	*arb = (struct rt_arb_internal *)ip->idb_ptr;
	point_t		center_pt;
	int		num_pts=8;	/* number of points to solve using rt_arb_3face_intersect */
	int		i;

	RT_ARB_CK_MAGIC(arb);

	/* find reference point (center_pt[3]) to find direction of normals */
	rt_arb_centroid( center_pt, arb, cgtype );

	/* move new face planes for the desired thicknesses
	 * don't do this yet for an arb7 */
	if( cgtype != 7 )
	{
		for(i=0; i<nface; i++) {
			if( (planes[i][3] - VDOT(center_pt, &planes[i][0])) > 0.0 )
				thick[i] *= -1.0;
			planes[i][3] += thick[i];
		}
	}

	if( cgtype == 5 ) 
		num_pts = 4;	/* use rt_arb_3face_intersect for first 4 points */
	else if( cgtype == 7 )
		num_pts = 0;	/* don't use rt_arb_3face_intersect for any points */

	/* find the new vertices by intersecting the new face planes */
	for(i=0; i<num_pts; i++) {
		if( rt_arb_3face_intersect( arb->pt[i], planes, cgtype, i*3 ) < 0 )  {
			(void)printf("cannot find inside arb\n");
			return(1);
		}
	}

	/* The following is code for the special cases of arb5 and arb7
	 * These arbs have a vertex that is the intersection of four planes, and
	 * the inside solid may have a single vertex or an edge replacing this vertex
	 */
	if( cgtype == 5 )
	{
		/* Here we are only concerned with the one vertex where 4 planes intersect
		 * in the original solid
		 */
		point_t pt[4];
		fastf_t dist0,dist1;

		/* calculate the four possible intersect points */
		if( rt_mkpoint_3planes( pt[0] , planes[1] , planes[2] , planes[3] ) )
		{
			(void)printf( "Cannot find inside arb5\n" );
			(void)printf( "Cannot find intersection of three planes for point 0:\n" );
			(void)printf( "\t%f %f %f %f\n" , V4ARGS( planes[1] ) );
			(void)printf( "\t%f %f %f %f\n" , V4ARGS( planes[2] ) );
			(void)printf( "\t%f %f %f %f\n" , V4ARGS( planes[3] ) );
			return( 1 );
		}
		if( rt_mkpoint_3planes( pt[1] , planes[2] , planes[3] , planes[4] ) )
		{
			(void)printf( "Cannot find inside arb5\n" );
			(void)printf( "Cannot find intersection of three planes for point 1:\n" );
			(void)printf( "\t%f %f %f %f\n" , V4ARGS( planes[2] ) );
			(void)printf( "\t%f %f %f %f\n" , V4ARGS( planes[3] ) );
			(void)printf( "\t%f %f %f %f\n" , V4ARGS( planes[4] ) );
			return( 1 );
		}
		if( rt_mkpoint_3planes( pt[2] , planes[3] , planes[4] , planes[1] ) )
		{
			(void)printf( "Cannot find inside arb5\n" );
			(void)printf( "Cannot find intersection of three planes for point 2:\n" );
			(void)printf( "\t%f %f %f %f\n" , V4ARGS( planes[3] ) );
			(void)printf( "\t%f %f %f %f\n" , V4ARGS( planes[4] ) );
			(void)printf( "\t%f %f %f %f\n" , V4ARGS( planes[1] ) );
			return( 1 );
		}
		if( rt_mkpoint_3planes( pt[3] , planes[4] , planes[1] , planes[2] ) )
		{
			(void)printf( "Cannot find inside arb5\n" );
			(void)printf( "Cannot find intersection of three planes for point 3:\n" );
			(void)printf( "\t%f %f %f %f\n" , V4ARGS( planes[4] ) );
			(void)printf( "\t%f %f %f %f\n" , V4ARGS( planes[1] ) );
			(void)printf( "\t%f %f %f %f\n" , V4ARGS( planes[2] ) );
			return( 1 );
		}
			
		if( rt_pt3_pt3_equal( pt[0] , pt[1] , &mged_tol ) )
		{
			/* if any two of the calculates intersection points are equal,
			 * then all four must be equal
			 */
			for( i=4 ; i<8 ; i++ )
				VMOVE( arb->pt[i] , pt[0] );

			return( 0 );
		}

		/* There will be an edge where the four planes come together
		 * Two edges of intersection have been calculated
		 *     pt[0]<->pt[2]
		 *     pt[1]<->pt[3]
		 * the one closest to the non-invloved plane (planes[0]) is the
		 * one we want
		 */

		dist0 = DIST_PT_PLANE( pt[0] , planes[0] );
		if( dist0 < 0.0 )
			dist0 = (-dist0);

		dist1 = DIST_PT_PLANE( pt[1] , planes[0] );
		if( dist1 < 0.0 )
			dist1 = (-dist1);

		if( dist0 < dist1 )
		{
			VMOVE( arb->pt[5] , pt[0] );
			VMOVE( arb->pt[6] , pt[0] );
			VMOVE( arb->pt[4] , pt[2] );
			VMOVE( arb->pt[7] , pt[2] );
		}
		else
		{
			VMOVE( arb->pt[4] , pt[3] );
			VMOVE( arb->pt[5] , pt[3] );
			VMOVE( arb->pt[6] , pt[1] );
			VMOVE( arb->pt[7] , pt[1] );
		}
	}
	else if( cgtype == 7 )
	{
		struct model *m;
		struct nmgregion *r;
		struct shell *s;
		struct faceuse *fu;
		struct rt_tess_tol ttol;
		struct nmg_ptbl vert_tab;

		ttol.magic = RT_TESS_TOL_MAGIC;
		ttol.abs = mged_abs_tol;
		ttol.rel = mged_rel_tol;
		ttol.norm = mged_nrm_tol;

		/* Make a model to hold the inside solid */
		m = nmg_mm();

		/* get an NMG version of this arb7 */
		if( rt_functab[ip->idb_type].ft_tessellate( &r , m , ip , &ttol , &mged_tol ) )
		{
			(void)printf( "Cannot tessellate arb7\n" );
			rt_functab[ip->idb_type].ft_ifree( ip );
			return( 1 );
		}

		/* move face planes */
		for( i=0 ; i<nface ; i++ )
		{
			int found=0;

			/* look for the face plane with the same geometry as the arb7 planes */
			s = RT_LIST_FIRST( shell , &r->s_hd );
			for( RT_LIST_FOR( fu , faceuse , &s->fu_hd ) )
			{
				struct face_g *fg;
				plane_t pl;

				NMG_CK_FACEUSE( fu );
				if( fu->orientation != OT_SAME )
					continue;

				NMG_GET_FU_PLANE( pl , fu );
				if( rt_coplanar( planes[i] , pl , &mged_tol ) > 0 )
				{
					/* found the NMG face geometry that matches arb face i */
					found = 1;
					fg = fu->f_p->fg_p;
					NMG_CK_FACE_G( fg );

					/* move the face by distance "thick[i]" */
					if( fu->f_p->flip )
						fg->N[3] += thick[i];
					else
						fg->N[3] -= thick[i];

					break;
				}
			}
			if( !found )
			{
				(void)printf( "Could not move face plane for arb7, face #%d\n" , i );
				nmg_km( m );
				return( 1 );
			}
		}

		/* solve for new vertex geometry
		 * This does all the vertices
		 */
		nmg_tbl( &vert_tab , TBL_INIT , (long *)NULL );
		nmg_vertex_tabulate( &vert_tab , &m->magic );
		for( i=0 ; i<NMG_TBL_END( &vert_tab ) ; i++ )
		{
			struct vertex *v;

			v = (struct vertex *)NMG_TBL_GET( &vert_tab , i );
			NMG_CK_VERTEX( v );

			if( nmg_in_vert( v , 0 , &mged_tol ) )
			{
				(void)printf( "Could not find coordinates for inside arb7\n" );
				nmg_km( m );
				nmg_tbl( &vert_tab , TBL_FREE , (long *)NULL );
				return( 1 );
			}
		}
		nmg_tbl( &vert_tab , TBL_FREE , (long *)NULL );

		/* rebound model */
		nmg_rebound( m , &mged_tol );

		nmg_extrude_cleanup( s , 0 , &mged_tol );

		/* free old ip pointer */
		rt_db_free_internal( ip );

		/* put new solid in "ip" */
		ip->idb_type = ID_NMG;
		ip->idb_ptr = (genptr_t)m;
	}

	return(0);
}

/* finds inside of tgc */
int
tgcin(ip, thick)
struct rt_db_internal	*ip;
fastf_t	thick[6];
{
	struct rt_tgc_internal	*tgc = (struct rt_tgc_internal *)ip->idb_ptr;
	fastf_t	mag[5], nmag[5];
	vect_t	unitH, unitA, unitB;
	vect_t	unitC, unitD;
	vect_t	hwork;
	fastf_t	aa[2], ab[2], ba[2], bb[2];
	fastf_t	dt, h1, h2, ht, dtha, dthb;
	fastf_t	s, d4, d5, ctan, t3;
	register int i, j, k;
	double ratio;

	thick[3] = thick[2];
	RT_TGC_CK_MAGIC(tgc);

	mag[0] = MAGNITUDE( tgc->h );
	mag[1] = MAGNITUDE( tgc->a );
	mag[2] = MAGNITUDE( tgc->b );
	mag[3] = MAGNITUDE( tgc->c );
	mag[4] = MAGNITUDE( tgc->d );

	if( (ratio = (mag[2] / mag[1])) < .8 )
		thick[3] = thick[3] / (1.016447 * pow(ratio, .071834));
	VSCALE(unitH, tgc->h, 1.0/mag[0]);
	VSCALE(unitA, tgc->a, 1.0/mag[1]);
	VSCALE(unitB, tgc->b, 1.0/mag[2]);
	VSCALE(unitC, tgc->c, 1.0/mag[3]);
	VSCALE(unitD, tgc->d, 1.0/mag[4]);

	VCROSS(hwork, unitA, unitB);
	if( (dt = VDOT(hwork, unitH)) == 0.0 ) {
		(void)printf("BAD cylinder\n");
		return(1);
	}
	else if( dt < 0.0 )
		dt = (-dt);

	h1 = thick[0] / dt;
	h2 = thick[1] / dt;
	if( (ht = dt * mag[0]) == 0.0 ) {
		(void)printf("Cannot find the inside cylinder\n");
		return(1);
	}
	dtha = VDOT(unitA, tgc->h);
	dthb = VDOT(unitB, tgc->h);
	s = 1.0;
	for(k=0; k<2; k++) {
		if( k )
			s = -1.0;
		d4 = s * dtha + mag[3];
		d5 = s * dthb + mag[4];
		ctan = (mag[1] - d4) / ht;
		t3 = thick[2] * sqrt( (mag[1] - d4)*(mag[1] - d4) + ht*ht ) / ht;
		aa[k] = mag[1] - t3 - (thick[0]*ctan);
		ab[k] = d4 - t3 + (thick[1]*ctan);

		ctan = (mag[2] - d5) / ht;
		t3 = thick[3] * sqrt( (mag[2]-d5)*(mag[2]-d5) + ht*ht ) / ht;
		ba[k] = mag[2] - t3 - (thick[0]*ctan);
		bb[k] = d5 - t3 + (thick[1]*ctan);
	}

	nmag[0] = mag[0] - (h1+h2);
	nmag[1] = ( aa[0] + aa[1] ) * .5;
	nmag[2] = ( ba[0] + ba[1] ) * .5;
	nmag[3] = (ab[0] + ab[1]) * .5;
	nmag[4] = (bb[0] + bb[1]) * .5;
	VSCALE(tgc->h, unitH, nmag[0] );
	VSCALE(tgc->a, unitA, nmag[1] );
	VSCALE(tgc->b, unitB, nmag[2] );
	VSCALE(tgc->c, unitC, nmag[3] );
	VSCALE(tgc->d, unitD, nmag[4] );

	for( i=0; i<3; i++ )  {
		tgc->v[i] += unitH[i]*h1 + .5 *
		    ( (aa[0] - aa[1])*unitA[i] + (ba[0] - ba[1])*unitB[i] );
	}
	return(0);
}


/* finds inside of torus */
int
torin(ip, thick)
struct rt_db_internal	*ip;
fastf_t			thick[6];
{
	struct rt_tor_internal	*tor = (struct rt_tor_internal *)ip->idb_ptr;

	RT_TOR_CK_MAGIC(tor);
	if( thick[0] == 0.0 )
		return(0);

	if( thick[0] < 0 ) {
		if( (tor->r_h - thick[0]) > (tor->r_a + .01) ) {
			(void)printf("cannot do: r2 > r1\n");
			return(1);
		}
	}
	if( thick[0] >= tor->r_h ) {
		(void)printf("cannot do: r2 <= 0\n");
		return(1);
	}

	tor->r_h = tor->r_h - thick[0];
	return(0);
}


/* finds inside ellg */
int
ellgin(ip, thick)
struct rt_db_internal	*ip;
fastf_t	thick[6];
{
	struct rt_ell_internal	*ell = (struct rt_ell_internal *)ip->idb_ptr;
	int i, j, k, order[3];
	fastf_t mag[3], nmag[3];
	fastf_t ratio;

	if( thick[0] <= 0.0 ) 
		return(0);
	thick[2] = thick[1] = thick[0];	/* uniform thickness */

	RT_ELL_CK_MAGIC(ell);
	mag[0] = MAGNITUDE(ell->a);
	mag[1] = MAGNITUDE(ell->b);
	mag[2] = MAGNITUDE(ell->c);

	for(i=0; i<3; i++) {
		order[i] = i;
	}

	for(i=0; i<2; i++) {
		k = i + 1;
		for(j=k; j<3; j++) {
			if(mag[i] < mag[j])
				order[i] = j;
		}
	}

	if( (ratio = mag[order[1]] / mag[order[0]]) < .8 )
		thick[order[1]] = thick[order[1]]/(1.016447*pow(ratio,.071834));
	if( (ratio = mag[order[2]] / mag[order[1]]) < .8 )
		thick[order[2]] = thick[order[2]]/(1.016447*pow(ratio,.071834));

	for(i=0; i<3; i++) {
		if( (nmag[i] = mag[i] - thick[i]) <= 0.0 )
			(void)printf("Warning: new vector [%d] length <= 0 \n", i);
	}
	VSCALE(ell->a, ell->a, nmag[0]/mag[0]);
	VSCALE(ell->b, ell->b, nmag[1]/mag[1]);
	VSCALE(ell->c, ell->c, nmag[2]/mag[2]);
	return(0);
}

/* finds inside of rpc, not quite right - r needs to be smaller */
int
rpcin(ip, thick)
struct rt_db_internal	*ip;
fastf_t	thick[4];
{
	struct rt_rpc_internal	*rpc = (struct rt_rpc_internal *)ip->idb_ptr;
	fastf_t			b, bp, th, rp, yp;
	vect_t			Bu, Hu, Norm, Ru;

	RT_RPC_CK_MAGIC(rpc);

	/* get unit coordinate axes */
	VMOVE( Bu, rpc->rpc_B );
	VMOVE( Hu, rpc->rpc_H );
	VCROSS( Ru, Hu, Bu );
	VUNITIZE( Bu );
	VUNITIZE( Hu );
	VUNITIZE( Ru );

	b = MAGNITUDE(rpc->rpc_B);
	VJOIN2( rpc->rpc_V, rpc->rpc_V, thick[0], Hu, thick[2], Bu );
	VSCALE( rpc->rpc_H, Hu, MAGNITUDE(rpc->rpc_H) - thick[0] - thick[1] );
	VSCALE( rpc->rpc_B, Bu, b - thick[2] - thick[3] );
#if 0
	bp = b - thick[2] - thick[3];
	rp = rpc->rpc_r - thick[3];	/* !!! ESTIMATE !!! */
	yp = rp * sqrt( (bp - thick[2])/bp );
	VSET( Norm,
		0.,
		2 * bp * yp/(rp * rp),
		-1.);
	VUNITIZE(Norm)
	th = thick[3] / Norm[Y];
	rpc->rpc_r -= th;
#endif
	rpc->rpc_r -= thick[3];

	return(0);
}

/* finds inside of rhc, not quite right */
int
rhcin(ip, thick)
struct rt_db_internal	*ip;
fastf_t	thick[4];
{
	struct rt_rhc_internal	*rhc = (struct rt_rhc_internal *)ip->idb_ptr;
	fastf_t			rn;
	point_t			Vn;
	vect_t			Bn, Hn, Bu, Hu, Ru;

	RT_RHC_CK_MAGIC(rhc);
	
	VMOVE( Vn, rhc->rhc_V );
	VMOVE( Bn, rhc->rhc_B );
	VMOVE( Hn, rhc->rhc_H );
	rn = rhc->rhc_r;
	
	/* get unit coordinate axes */
	VMOVE( Bu, Bn );
	VMOVE( Hu, Hn );
	VCROSS( Ru, Hu, Bu );
	VUNITIZE( Bu );
	VUNITIZE( Hu );
	VUNITIZE( Ru );

	VJOIN2( rhc->rhc_V, rhc->rhc_V, thick[0], Hu, thick[2], Bu );
	VSCALE( rhc->rhc_H, Hu, MAGNITUDE(rhc->rhc_H) - thick[0] - thick[1] );
	VSCALE( rhc->rhc_B, Bu, MAGNITUDE(rhc->rhc_B) - thick[2] - thick[3] );
	rhc->rhc_r -= thick[3];

	return(0);
}

/* finds inside of epa, not quite right */
int
epain(ip, thick)
struct rt_db_internal	*ip;
fastf_t	thick[2];
{
	struct rt_epa_internal	*epa = (struct rt_epa_internal *)ip->idb_ptr;
	vect_t			Hu;

	RT_EPA_CK_MAGIC(epa);
	
	VMOVE( Hu, epa->epa_H );
	VUNITIZE( Hu );

	VJOIN1( epa->epa_V, epa->epa_V, thick[0], Hu );
	VSCALE( epa->epa_H, Hu, MAGNITUDE(epa->epa_H) - thick[0] - thick[1] );
	epa->epa_r1 -= thick[1];
	epa->epa_r2 -= thick[1];

	return(0);
}

/* finds inside of ehy, not quite right, */
int
ehyin(ip, thick)
struct rt_db_internal	*ip;
fastf_t	thick[2];
{
	struct rt_ehy_internal	*ehy = (struct rt_ehy_internal *)ip->idb_ptr;
	vect_t			Hu;

	RT_EHY_CK_MAGIC(ehy);
	
	VMOVE( Hu, ehy->ehy_H );
	VUNITIZE( Hu );
	
	VJOIN1( ehy->ehy_V, ehy->ehy_V, thick[0], Hu );
	VSCALE( ehy->ehy_H, Hu, MAGNITUDE(ehy->ehy_H) - thick[0] - thick[1] );
	ehy->ehy_r1 -= thick[1];
	ehy->ehy_r2 -= thick[1];

	return(0);
}

/* finds inside of eto */
int
etoin(ip, thick)
struct rt_db_internal	*ip;
fastf_t	thick[1];
{
	fastf_t			c;
	struct rt_eto_internal	*eto = (struct rt_eto_internal *)ip->idb_ptr;

	RT_ETO_CK_MAGIC(eto);

	c = 1. - thick[0]/MAGNITUDE(eto->eto_C);
	VSCALE( eto->eto_C, eto->eto_C, c );
	eto->eto_rd -= thick[0];

	return(0);
}

/* find inside for NMG */
int
nmgin( ip , thick )
struct rt_db_internal	*ip;
fastf_t thick;
{
	struct model *m;
	struct nmgregion *r;

	if( ip->idb_type != ID_NMG )
		return( 1 );

	m = (struct model *)ip->idb_ptr;
	NMG_CK_MODEL( m );

	r = RT_LIST_FIRST( nmgregion ,  &m->r_hd );
	while( RT_LIST_NOT_HEAD( r , &m->r_hd ) )
	{
		struct nmgregion *next_r;
		struct shell *s;

		NMG_CK_REGION( r );

		next_r = RT_LIST_PNEXT( nmgregion , &r->l );

		s = RT_LIST_FIRST( shell , &r->s_hd );
		while( RT_LIST_NOT_HEAD( s , &r->s_hd ) )
		{
			struct shell *next_s;

			next_s = RT_LIST_PNEXT( shell , &s->l );

			(void)nmg_extrude_shell( s , thick , 0 , 0 , &mged_tol );

			s = next_s;
		}

		if( RT_LIST_IS_EMPTY( &r->s_hd ) )
			nmg_kr( r );

		r = next_r;
	}

	if( RT_LIST_IS_EMPTY( &m->r_hd ) )
	{
		(void)printf( "No inside created\n" );
		nmg_km( m );
		return( 1 );
	}
	else
		return( 0 );
}
