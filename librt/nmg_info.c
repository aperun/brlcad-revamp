/*
 *			N M G _ I N F O . C
 *
 *  A companion module to nmg_mod.c which contains routines to
 *  answer questions about n-Manifold Geometry data structures.
 *  None of these routines will modify the NMG structures in any way.
 *
 *  Authors -
 *	Michael John Muuss
 *	Lee A. Butler
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Notice -
 *	Re-distribution of this software is restricted, as described in
 *	your "Statement of Terms and Conditions for the Release of
 *	The BRL-CAD Pacakge" agreement.
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 1993 by the United States Army
 *	in all countries except the USA.  All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (ARL)";
#endif

#include "conf.h"
#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "nmg.h"
#include "raytrace.h"
#include "./debug.h"	/* For librt debug flags, XXX temp */


/************************************************************************
 *									*
 *				MODEL Routines				*
 *									*
 ************************************************************************/

/*
 *			N M G _ F I N D _ M O D E L
 *
 *  Given a pointer to the magic number in any NMG data structure,
 *  return a pointer to the model structure that contains that NMG item.
 *
 *  The reason for the register variable is to leave the argument variable
 *  unmodified;  this may aid debugging in event of a core dump.
 */
struct model *
nmg_find_model( magic_p_arg )
CONST long	*magic_p_arg;
{
	register CONST long	*magic_p = magic_p_arg;

top:
	if( magic_p == (long *)0 )  {
		rt_log("nmg_find_model(x%x) enountered null pointer\n",
			magic_p_arg );
		rt_bomb("nmg_find_model() null pointer\n");
		/* NOTREACHED */
	}

	switch( *magic_p )  {
	case NMG_MODEL_MAGIC:
		return( (struct model *)magic_p );
	case NMG_REGION_MAGIC:
		return( ((struct nmgregion *)magic_p)->m_p );
	case NMG_SHELL_MAGIC:
		return( ((struct shell *)magic_p)->r_p->m_p );
	case NMG_FACEUSE_MAGIC:
		magic_p = &((struct faceuse *)magic_p)->s_p->l.magic;
		goto top;
	case NMG_FACE_MAGIC:
		magic_p = &((struct face *)magic_p)->fu_p->l.magic;
		goto top;
	case NMG_LOOP_MAGIC:
		magic_p = ((struct loop *)magic_p)->lu_p->up.magic_p;
		goto top;
	case NMG_LOOPUSE_MAGIC:
		magic_p = ((struct loopuse *)magic_p)->up.magic_p;
		goto top;
	case NMG_EDGE_MAGIC:
		magic_p = ((struct edge *)magic_p)->eu_p->up.magic_p;
		goto top;
	case NMG_EDGEUSE_MAGIC:
		magic_p = ((struct edgeuse *)magic_p)->up.magic_p;
		goto top;
	case NMG_VERTEX_MAGIC:
		magic_p = &(RT_LIST_FIRST(vertexuse,
			&((struct vertex *)magic_p)->vu_hd)->l.magic);
		goto top;
	case NMG_VERTEXUSE_MAGIC:
		magic_p = ((struct vertexuse *)magic_p)->up.magic_p;
		goto top;

	default:
		rt_log("nmg_find_model() can't get model for magic=x%x (%s)\n",
			*magic_p, rt_identify_magic( *magic_p ) );
		rt_bomb("nmg_find_model() failure\n");
	}
	return( (struct model *)NULL );
}

void
nmg_model_bb( min_pt, max_pt, m )
point_t min_pt, max_pt;
CONST struct model *m;
{
	struct nmgregion *r;
	register int i;

	NMG_CK_MODEL(m);

	min_pt[0] = min_pt[1] = min_pt[2] = MAX_FASTF;
	max_pt[0] = max_pt[1] = max_pt[2] = -MAX_FASTF;

	for (RT_LIST_FOR(r, nmgregion, &m->r_hd)) {
		NMG_CK_REGION(r);
		NMG_CK_REGION_A(r->ra_p);

		for (i=0 ; i < 3 ; i++) {
			if (min_pt[i] > r->ra_p->min_pt[i])
				min_pt[i] = r->ra_p->min_pt[i];
			if (max_pt[i] < r->ra_p->max_pt[i])
				max_pt[i] = r->ra_p->max_pt[i];
		}
	}
}

/************************************************************************
 *									*
 *				SHELL Routines				*
 *									*
 ************************************************************************/

/*
 *			N M G _ S H E L L _ I S _ E M P T Y
 *
 *  See if this is an invalid shell
 *  i.e., one that has absolutely nothing in it,
 *  not even a lone vertexuse.
 *
 *  Returns -
 *	1	yes, it is completely empty
 *	0	no, not empty
 */
int
nmg_shell_is_empty(s)
register CONST struct shell *s;
{

	NMG_CK_SHELL(s);

	if( RT_LIST_NON_EMPTY( &s->fu_hd ) )  return 0;
	if( RT_LIST_NON_EMPTY( &s->lu_hd ) )  return 0;
	if( RT_LIST_NON_EMPTY( &s->eu_hd ) )  return 0;
	if( s->vu_p )  return 0;
	return 1;
}

/*				N M G _ F I N D _ S _ O F _ L U
 *
 *	return parent shell for loopuse
 *	formerly nmg_lups().
 */
struct shell *
nmg_find_s_of_lu(lu)
CONST struct loopuse *lu;
{
	if (*lu->up.magic_p == NMG_SHELL_MAGIC) return(lu->up.s_p);
	else if (*lu->up.magic_p != NMG_FACEUSE_MAGIC) 
		rt_bomb("nmg_find_s_of_lu() bad parent for loopuse\n");

	return(lu->up.fu_p->s_p);
}

/*				N M G _ F I N D _ S _ O F _ E U
 *
 *	return parent shell of edgeuse
 *	formerly nmg_eups().
 */
struct shell *
nmg_find_s_of_eu(eu)
CONST struct edgeuse *eu;
{
	if (*eu->up.magic_p == NMG_SHELL_MAGIC) return(eu->up.s_p);
	else if (*eu->up.magic_p != NMG_LOOPUSE_MAGIC)
		rt_bomb("nmg_find_s_of_eu() bad parent for edgeuse\n");

	return(nmg_find_s_of_lu(eu->up.lu_p));
}

/*
 *			N M G _ F I N D _ S _ O F _ V U
 *
 *  Return parent shell of vertexuse
 */
struct shell *
nmg_find_s_of_vu(vu)
CONST struct vertexuse *vu;
{
	NMG_CK_VERTEXUSE(vu);

	if( *vu->up.magic_p == NMG_LOOPUSE_MAGIC )
		return nmg_find_s_of_lu( vu->up.lu_p );
	return nmg_find_s_of_eu( vu->up.eu_p );
}

/************************************************************************
 *									*
 *				FACE Routines				*
 *									*
 ************************************************************************/

/*
 *			N M G _ F I N D _ F U _ O F _ E U
 *
 *	return a pointer to the faceuse that is the super-parent of this
 *	edgeuse.  If edgeuse has no grandparent faceuse, return NULL.
 */
struct faceuse *
nmg_find_fu_of_eu(eu)
CONST struct edgeuse *eu;
{
	NMG_CK_EDGEUSE(eu);

	if (*eu->up.magic_p == NMG_LOOPUSE_MAGIC &&
		*eu->up.lu_p->up.magic_p == NMG_FACEUSE_MAGIC)
			return eu->up.lu_p->up.fu_p;

	return (struct faceuse *)NULL;			
}

/*
 *			N M G _ F I N D _ F U _ O F _ L U
 */
struct faceuse *
nmg_find_fu_of_lu(lu)
CONST struct loopuse *lu;
{
	switch (*lu->up.magic_p) {
	case NMG_FACEUSE_MAGIC:
		return lu->up.fu_p;
	case NMG_SHELL_MAGIC:
		return (struct faceuse *)NULL;
	default:
	    rt_log("Error at %s %d:\nInvalid loopuse parent magic (0x%x %d)\n",
		__FILE__, __LINE__, *lu->up.magic_p, *lu->up.magic_p);
	    rt_bomb("nmg_find_fu_of_lu() giving up on loopuse");
	}
}


/*	N M G _ F I N D _ F U _ O F _ V U
 *
 *	return a pointer to the parent faceuse of the vertexuse
 *	or a null pointer if vu is not a child of a faceuse.
 */
struct faceuse *
nmg_find_fu_of_vu(vu)
CONST struct vertexuse *vu;
{
	NMG_CK_VERTEXUSE(vu);

	switch (*vu->up.magic_p) {
	case NMG_LOOPUSE_MAGIC:
		return nmg_find_fu_of_lu( vu->up.lu_p );
	case NMG_SHELL_MAGIC:
		rt_log("nmg_find_fu_of_vu() vertexuse is child of shell, can't find faceuse\n");
		return ((struct faceuse *)NULL);
		break;
	case NMG_EDGEUSE_MAGIC:
		switch (*vu->up.eu_p->up.magic_p) {
		case NMG_LOOPUSE_MAGIC:
			return nmg_find_fu_of_lu( vu->up.eu_p->up.lu_p );
			break;
		case NMG_SHELL_MAGIC:
			rt_log("nmg_find_fu_of_vu() vertexuse is child of shell/edgeuse, can't find faceuse\n");
			return ((struct faceuse *)NULL);
			break;
		}
		rt_log("Error at %s %d:\nInvalid loopuse parent magic 0x%x\n",
			__FILE__, __LINE__, *vu->up.lu_p->up.magic_p);
		abort();
		break;
	default:
		rt_log("Error at %s %d:\nInvalid vertexuse parent magic 0x%x\n",
			__FILE__, __LINE__,
			*vu->up.magic_p);
		abort();
		break;
	}
	rt_log("How did I get here %s %d?\n", __FILE__, __LINE__);
	rt_bomb("nmg_find_fu_of_vu()\n");
}
/*
 *			N M G _ F I N D _ F U _ W I T H _ F G _ I N _ S
 *
 *  Find a faceuse in shell s1 that shares the face_g_plane structure with
 *  fu2 and has the same orientation.
 *  This may be an OT_OPPOSITE faceuse, depending on orientation.
 *  Returns NULL if no such faceuse can be found in s1.
 *  fu2 may be in s1, or in some other shell.
 */
struct faceuse *
nmg_find_fu_with_fg_in_s( s1, fu2 )
CONST struct shell	*s1;
CONST struct faceuse	*fu2;
{
	struct faceuse		*fu1;
	struct face		*f2;
	register struct face_g_plane	*fg2;

	NMG_CK_SHELL(s1);
	NMG_CK_FACEUSE(fu2);

	f2 = fu2->f_p;
	NMG_CK_FACE(f2);
	fg2 = f2->g.plane_p;
	NMG_CK_FACE_G_PLANE(fg2);

	for( RT_LIST_FOR( fu1, faceuse, &s1->fu_hd ) )  {
		register struct face	*f1;
		register struct	face_g_plane	*fg1;
		int			flip1, flip2;

		NMG_CK_FACEUSE(fu1);
		f1 = fu1->f_p;
		NMG_CK_FACE(f1);
		fg1 = fu1->f_p->g.plane_p;
		NMG_CK_FACE_G_PLANE(fg1);

		if( fg1 != fg2 )  continue;

		if( fu1 == fu2 || fu1->fumate_p == fu2 )  continue;

		/* Face geometry matches, select fu1 or it's mate */
		flip1 = (fu1->orientation != OT_SAME) != (f1->flip != 0);
		flip2 = (fu2->orientation != OT_SAME) != (f2->flip != 0);
		if( flip1 == flip2 )  return fu1;
		return fu1->fumate_p;
	}
	return (struct faceuse *)NULL;
}

/*
 *			N M G _ M E A S U R E _ F U _ A N G L E
 *
 *  Return the angle in radians from the interior portion of the faceuse
 *  associated with edgeuse 'eu', measured in the coordinate system
 *  defined by xvec and yvec, which are known to be perpendicular to
 *  each other, and to the edge vector.
 *
 *  This is done by finding the "left-ward" vector for the edge in the
 *  face, which points into the interior of the face, and measuring
 *  the angle it forms relative to xvec and yvec.
 *
 *  Wire edges are indicated by always returning angle of -pi.
 *  That will be the only case for negative returns.
 */
double
nmg_measure_fu_angle( eu, xvec, yvec, zvec )
CONST struct edgeuse	*eu;
CONST vect_t		xvec;
CONST vect_t		yvec;
CONST vect_t		zvec;
{
	vect_t			left;
	double			ret;

	NMG_CK_EDGEUSE(eu);
	if( *eu->up.magic_p != NMG_LOOPUSE_MAGIC )  return -rt_pi;

	if( nmg_find_eu_leftvec( left, eu ) < 0 )  return -rt_pi;

	ret = rt_angle_measure( left, xvec, yvec );
	return ret;
}

/************************************************************************
 *									*
 *				LOOP Routines				*
 *									*
 ************************************************************************/

/*
 *			N M G _ F I N D _ L U _ O F _ V U
 */
struct loopuse *
nmg_find_lu_of_vu( vu )
CONST struct vertexuse *vu;
{
	NMG_CK_VERTEXUSE( vu );
	if ( *vu->up.magic_p == NMG_LOOPUSE_MAGIC )
		return vu->up.lu_p;

	if ( *vu->up.magic_p == NMG_SHELL_MAGIC )
		return (struct loopuse *)NULL;

	NMG_CK_EDGEUSE( vu->up.eu_p );

	if ( *vu->up.eu_p->up.magic_p == NMG_SHELL_MAGIC )
		return (struct loopuse *)NULL;

	NMG_CK_LOOPUSE( vu->up.eu_p->up.lu_p );

	return vu->up.eu_p->up.lu_p;
}

/*
 *			N M G _ L O O P _ I S _ A _ C R A C K
 *
 *  A "crack" is defined as a loop which has no area.
 *
 *  Example of a non-trivial "crack" loop:
 *
 *	                 <---- eu4 -----
 *	               C ############### D
 *	             | # ^ ---- eu3 --->
 *	             | # |
 *	           eu5 # eu2
 *	             | # |
 *	  <--- eu6 --V # |
 *	A ############ B 
 *	  --- eu1 ---->
 *
 *
 *  Returns -
 *	 0	Loop is not a "crack"
 *	!0	Loop *is* a "crack"
 */
int
nmg_loop_is_a_crack( lu )
CONST struct loopuse	*lu;
{
	struct edgeuse	*cur_eu;
	struct edgeuse	*cur_eumate;
	struct vertexuse *cur_vu;
	struct vertex	*cur_v;
	struct vertexuse *next_vu;
	struct vertex	*next_v;
	struct faceuse	*fu;
	struct vertexuse *test_vu;
	struct edgeuse	*test_eu;
	struct loopuse	*test_lu;
	int		ret = 0;

	NMG_CK_LOOPUSE(lu);
	if( *lu->up.magic_p != NMG_FACEUSE_MAGIC )  {
	    	if (rt_g.NMG_debug & DEBUG_BASIC)  rt_log("lu up is not faceuse\n");
		ret = 0;
		goto out;
	}
	fu = lu->up.fu_p;
	NMG_CK_FACEUSE(fu);

	if( RT_LIST_FIRST_MAGIC( &lu->down_hd ) != NMG_EDGEUSE_MAGIC )  {
	    	if (rt_g.NMG_debug & DEBUG_BASIC)  rt_log("lu down is not edgeuse\n");
		ret = 0;
		goto out;
	}

	/*
	 *  For every edgeuse, see if there is another edgeuse from 'lu',
	 *  radial around this edge, which is not this edgeuse's mate.
	 */
	for( RT_LIST_FOR( cur_eu, edgeuse, &lu->down_hd ) )  {
		NMG_CK_EDGEUSE(cur_eu);
		cur_eumate = cur_eu->eumate_p;
		NMG_CK_EDGEUSE(cur_eumate);
		cur_vu = cur_eu->vu_p;
		NMG_CK_VERTEXUSE(cur_vu);
		cur_v = cur_vu->v_p;
		NMG_CK_VERTEX(cur_v);

		next_vu = cur_eumate->vu_p;
		NMG_CK_VERTEXUSE(next_vu);
		next_v = next_vu->v_p;
		NMG_CK_VERTEX(next_v);
		/* XXX It might be more efficient to walk the radial list */
		/* See if the next vertex has an edge pointing back to cur_v */
		for( RT_LIST_FOR( test_vu, vertexuse, &next_v->vu_hd ) )  {
			if( *test_vu->up.magic_p != NMG_EDGEUSE_MAGIC )  continue;
			test_eu = test_vu->up.eu_p;
			NMG_CK_EDGEUSE(test_eu);
			if( test_eu == cur_eu )  continue;	/* skip self */
			if( test_eu == cur_eumate )  continue;	/* skip mates */
			if( *test_eu->up.magic_p != NMG_LOOPUSE_MAGIC )  continue;
			test_lu = test_eu->up.lu_p;
			if( test_lu != lu )  continue;
			/* Check departing edgeuse's NEXT vertex */
			if( test_eu->eumate_p->vu_p->v_p == cur_v )  goto match;
		}
		/* No path back, this can't be a crack, abort */
		ret = 0;
		goto out;
		
		/* One edgeuse matched, all the others have to as well */
match:		;
	}
	ret = 1;
out:
    	if (rt_g.NMG_debug & DEBUG_BASIC)  {
    		rt_log("nmg_loop_is_a_crack(lu=x%x) ret=%d\n", lu, ret );
    	}
	return ret;
}

/*
 *			N M G _ L O O P _ I S _ C C W
 *
 *  Determine if loop proceeds counterclockwise (CCW) around the
 *  provided normal vector (which may be either the face normal,
 *  or the anti-normal).
 *
 *  XXX Consider using John's loop area calculator instead.
 *
 *  Compute the "winding number" for the loop, by calculating all the angles.
 *  A simple square will have a total of +/- 2*pi.
 *  However, each "jaunt" into the interior will increase this total
 *  by pi.
 *  This information is probably useful, but here the only goal is to
 *  determine cw/ccw, so just make sure the total has reached +/- 2*pi.
 *
 *  Returns -
 *	+1	Loop is CCW, should be exterior loop.
 *	-1	Loop is CW, should be interior loop.
 *	 0	Unable to tell, error.
 *
 */
int
nmg_loop_is_ccw( lu, norm, tol )
CONST struct loopuse	*lu;
CONST plane_t		norm;
CONST struct rt_tol	*tol;
{
	vect_t		edge1, edge2;
	vect_t		left;
	struct edgeuse	*eu;
	struct edgeuse	*next_eu;
	struct vertexuse *this_vu, *next_vu, *third_vu;
	fastf_t		theta = 0;
	fastf_t		x,y;
	fastf_t		rad;
	fastf_t		npi;		/* n * pi, hopefully */
	double		n;		/* integer part of npi */
	fastf_t		residue;	/* fractional part of npi */
	int		n_angles=0;	/* number of edge/edge angles measured */
	int		n_skip=0;	/* number of edges skipped */
	int		ret;

	NMG_CK_LOOPUSE(lu);
	RT_CK_TOL(tol);
	if( RT_LIST_FIRST_MAGIC( &lu->down_hd ) != NMG_EDGEUSE_MAGIC )  return 0;

	if( nmg_loop_is_a_crack(lu) )  {
		ret = 0;
		goto out;
	}

	for( RT_LIST_FOR( eu, edgeuse, &lu->down_hd ) )  {
		next_eu = RT_LIST_PNEXT_CIRC( edgeuse, eu );
		this_vu = eu->vu_p;
		next_vu = eu->eumate_p->vu_p;
		third_vu = next_eu->eumate_p->vu_p;

		/* Skip topological 0-length edges */
		if( this_vu->v_p == next_vu->v_p  ||
		    next_vu->v_p == third_vu->v_p )  {
		    	n_skip++;
		    	continue;
		}

		/* Skip edges with calculated edge lengths near 0 */
		VSUB2( edge1, next_vu->v_p->vg_p->coord, this_vu->v_p->vg_p->coord );
		if( MAGSQ(edge1) < tol->dist_sq )  {
			n_skip++;
			continue;
		}
		VSUB2( edge2, third_vu->v_p->vg_p->coord, next_vu->v_p->vg_p->coord );
		if( MAGSQ(edge2) < tol->dist_sq )  {
			n_skip++;
			continue;
		}
		/* XXX Should use magsq info from above */
		VUNITIZE(edge1);
		VUNITIZE(edge2);

		/* Compute (loop)inward pointing "left" vector */
		VCROSS( left, norm, edge1 );
		y = VDOT( edge2, left );
		x = VDOT( edge2, edge1 );
		rad = atan2( y, x );

		if( rt_g.debug & DEBUG_MATH )  {
			nmg_pr_eu_briefly(eu,NULL);
			VPRINT("vu1", this_vu->v_p->vg_p->coord);
			VPRINT("vu2", next_vu->v_p->vg_p->coord);
			VPRINT("edge1", edge1);
			VPRINT("edge2", edge2);
			VPRINT("left", left);
			rt_log(" e1=%g, e2=%g, n=%g, l=%g\n", MAGNITUDE(edge1),
				MAGNITUDE(edge2), MAGNITUDE(norm), MAGNITUDE(left));
			rt_log("atan2(%g,%g) = %g\n", y, x, rad);
		}
		theta += rad;
		n_angles++;
	}
#if 0
	rt_log(" theta = %g (%g) n_angles=%d\n", theta, theta / rt_twopi, n_angles );
	nmg_face_lu_plot( lu, this_vu, this_vu );
#endif

	if( n_angles < 3 )  {
		ret = 0;
		goto out;
	}

	npi = theta * rt_invpi;		/* n * pi.  n should be >= 2 */

	/*  Check that 'npi' is very nearly an integer,
	 *  otherwise that is an indicator of trouble in the calculation.
	 */
	residue = modf( npi, &n );
	/* Sometimes, residue can be almost +/- 1.0, need to fold that in. */
	if( NEAR_ZERO( residue-1, 0.05 ) )  {
		residue -= 1;
		npi += 1;
	} else if( NEAR_ZERO( residue+1, 0.05 ) )  {
		residue += 1;
		npi -= 1;
	}

	if( !NEAR_ZERO( residue, 0.05 ) )  {
		rt_log("nmg_loop_is_ccw(x%x) npi=%g, n=%g, residue=%e, n_skip=%d\n",
			lu, npi, n, residue, n_skip );
	}

	/* "npi" value is normalized -1..+1, tolerance here is 1% */
	if( npi >= 2 - 0.05 )  {
		/* theta >= two pi, loop is CCW */
		ret = 1;
		goto out;
	}
	if( npi <= -2 + 0.05 )  {
		/* theta <= -two pi, loop is CW */
		ret = -1;
		goto out;
	}
	rt_log("nmg_loop_is_ccw(x%x):  unable to determine CW/CCW, theta=%g, winding=%g*pi (%d angles, %d skip)\n",
		theta, npi, n_angles, n_skip );

#if 0
	if( !(rt_g.debug & DEBUG_MATH) )  {
		int	save = rt_g.debug;
		/* Do it again, with details exhibited. */
		rt_g.debug |= DEBUG_MATH;
		(void)nmg_loop_is_ccw( lu, norm, tol );
		rt_g.debug = save;
	}
	nmg_pr_lu_briefly(lu, NULL);
	rt_log(" theta = %g, winding=%g*pi\n", theta, npi );
	
	rt_g.NMG_debug |= DEBUG_PLOTEM;
	nmg_face_lu_plot( lu, this_vu, this_vu );
	rt_bomb("nmg_loop_is_ccw()\n");
#else
	/* This can happen with zig-zag "crack" loops, as in Test4.r */
	ret = 0;
#endif
out:
    	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_loop_is_ccw(lu=x%x) ret=%d (%d angles, %d skip, winding=%g*pi)\n",
			lu, ret, n_angles, n_skip, npi);
    	}
	return ret;
}

/*
 *			N M G _ L O O P _ T O U C H E S _ S E L F
 *
 *  Search through all the vertices in a loop.
 *  If there are two distinct uses of one vertex in the loop,
 *  return true.
 *  This is useful for detecting "accordian pleats"
 *  unexpectedly showing up in a loop.
 *  Derrived from nmg_split_touchingloops().
 *
 *  Returns -
 *	vu	Yes, the loop touches itself at least once, at this vu.
 *	0	No, the loop does not touch itself.
 */
CONST struct vertexuse *
nmg_loop_touches_self( lu )
CONST struct loopuse	*lu;
{
	CONST struct edgeuse	*eu;
	CONST struct vertexuse	*vu;
	CONST struct vertex	*v;

	if( RT_LIST_FIRST_MAGIC( &lu->down_hd ) != NMG_EDGEUSE_MAGIC )
		return (CONST struct vertexuse *)0;

	/* For each edgeuse, get vertexuse and vertex */
	for( RT_LIST_FOR( eu, edgeuse, &lu->down_hd ) )  {
		CONST struct vertexuse	*tvu;

		vu = eu->vu_p;
		NMG_CK_VERTEXUSE(vu);
		v = vu->v_p;
		NMG_CK_VERTEX(v);

		/*
		 *  For each vertexuse on vertex list,
		 *  check to see if it points up to the this loop.
		 *  If so, then there is a duplicated vertex.
		 *  Ordinarily, the vertex list will be *very* short,
		 *  so this strategy is likely to be faster than
		 *  a table-based approach, for most cases.
		 */
		for( RT_LIST_FOR( tvu, vertexuse, &v->vu_hd ) )  {
			CONST struct edgeuse		*teu;
			CONST struct loopuse		*tlu;
			CONST struct loopuse		*newlu;

			if( tvu == vu )  continue;
			/*
			 *  Inline expansion of:
			 *	if(nmg_find_lu_of_vu(tvu) != lu) continue;
			 */
			if( *tvu->up.magic_p != NMG_EDGEUSE_MAGIC )  continue;
			teu = tvu->up.eu_p;
			NMG_CK_EDGEUSE(teu);
			if( *teu->up.magic_p != NMG_LOOPUSE_MAGIC )  continue;
			tlu = teu->up.lu_p;
			NMG_CK_LOOPUSE(tlu);
			if( tlu != lu )  continue;
			/*
			 *  Repeated vertex exists.
			 */
			return vu;
		}
	}
	return (CONST struct vertexuse *)0;
}

/************************************************************************
 *									*
 *				EDGE Routines				*
 *									*
 ************************************************************************/

/*
 *			N M G _ F I N D _ M A T C H I N G _ E U _ I N _ S
 *
 *  If shell s2 has an edge that connects the same vertices as eu1 connects,
 *  return the matching edgeuse in s2.
 *  This routine works properly regardless of whether eu1 is in s2 or not.
 *  A convenient wrapper for nmg_findeu().
 */
struct edgeuse *
nmg_find_matching_eu_in_s( eu1, s2 )
CONST struct edgeuse	*eu1;
CONST struct shell	*s2;
{
	CONST struct vertexuse	*vu1a, *vu1b;
	struct vertexuse	*vu2a, *vu2b;
	struct edgeuse		*eu2;

	NMG_CK_EDGEUSE(eu1);
	NMG_CK_SHELL(s2);

	vu1a = eu1->vu_p;
	vu1b = RT_LIST_PNEXT_CIRC( edgeuse, eu1 )->vu_p;
	NMG_CK_VERTEXUSE(vu1a);
	NMG_CK_VERTEXUSE(vu1b);
	if( (vu2a = nmg_find_v_in_shell( vu1a->v_p, s2, 1 )) == (struct vertexuse *)NULL )
		return (struct edgeuse *)NULL;
	if( (vu2b = nmg_find_v_in_shell( vu1b->v_p, s2, 1 )) == (struct vertexuse *)NULL )
		return (struct edgeuse *)NULL;

	/* Both vertices have vu's of eu's in s2 */

	eu2 = nmg_findeu( vu1a->v_p, vu1b->v_p, s2, eu1, 0 );
	return eu2;		/* May be NULL if no edgeuse found */
}

/*
 *			N M G _ F I N D E U
 *
 *  Find an edgeuse in a shell between a given pair of vertex structs.
 *
 *  If a given shell "s" is specified, then only edgeuses in that shell
 *  will be considered, otherwise all edgeuses in the model are fair game.
 *
 *  If a particular edgeuse "eup" is specified, then that edgeuse
 *  and it's mate will not be returned as a match.
 *
 *  If "dangling_only" is true, then an edgeuse will be matched only if
 *  there are no other edgeuses on the edge, i.e. the radial edgeuse is
 *  the same as the mate edgeuse.
 *
 *  Returns -
 *	edgeuse*	Edgeuse which matches the criteria
 *	NULL		Unable to find matching edgeuse
 */
struct edgeuse *
nmg_findeu(v1, v2, s, eup, dangling_only)
CONST struct vertex	*v1, *v2;
CONST struct shell	*s;
CONST struct edgeuse	*eup;
int		dangling_only;
{
	register CONST struct vertexuse	*vu;
	register CONST struct edgeuse	*eu;
	CONST struct edgeuse		*eup_mate;
	int				eup_orientation;

	NMG_CK_VERTEX(v1);
	NMG_CK_VERTEX(v2);
	if(s) NMG_CK_SHELL(s);

	if(eup)  {
		struct faceuse	*fu;
		NMG_CK_EDGEUSE(eup);
		eup_mate = eup->eumate_p;
		NMG_CK_EDGEUSE(eup_mate);
		if( (fu = nmg_find_fu_of_eu(eup)) )
			eup_orientation = fu->orientation;
		else
			eup_orientation = OT_SAME;
	} else {
		eup_mate = eup;			/* NULL */
		eup_orientation = OT_SAME;
	}

	if (rt_g.NMG_debug & DEBUG_FINDEU)
		rt_log("nmg_findeu() seeking eu!=%8x/%8x between (%8x, %8x) %s\n",
			eup, eup_mate, v1, v2,
			dangling_only ? "[dangling]" : "[any]" );

	for( RT_LIST_FOR( vu, vertexuse, &v1->vu_hd ) )  {
		NMG_CK_VERTEXUSE(vu);
		if (!vu->up.magic_p)
			rt_bomb("nmg_findeu() vertexuse in vu_hd list has null parent\n");

		/* Ignore self-loops and lone shell verts */
		if (*vu->up.magic_p != NMG_EDGEUSE_MAGIC )  continue;
		eu = vu->up.eu_p;

		/* Ignore edgeuses which don't run between the right verts */
		if( eu->eumate_p->vu_p->v_p != v2 )  continue;

		if (rt_g.NMG_debug & DEBUG_FINDEU )  {
			rt_log("nmg_findeu: check eu=%8x vertex=(%8x, %8x)\n",
				eu, eu->vu_p->v_p,
				eu->eumate_p->vu_p->v_p);
		}

		/* Ignore the edgeuse to be excluded */
		if( eu == eup || eu->eumate_p == eup )  {
			if (rt_g.NMG_debug & DEBUG_FINDEU )
				rt_log("\tIgnoring -- excluded edgeuse\n");
			continue;
		}

		/* See if this edgeuse is in the proper shell */
		if( s && nmg_find_s_of_eu(eu) != s )  {
		    	if (rt_g.NMG_debug & DEBUG_FINDEU)
		    		rt_log("\tIgnoring x%x -- eu in wrong shell s=%x\n", eu, eu->up.s_p);
			continue;
		}

		/* If it's not a dangling edge, skip on */
		if( dangling_only && eu->eumate_p != eu->radial_p) {
		    	if (rt_g.NMG_debug & DEBUG_FINDEU)  {
			    	rt_log("\tIgnoring %8x/%8x (radial=x%x)\n",
			    		eu, eu->eumate_p,
					eu->radial_p );
		    	}
			continue;
		}

	    	if (rt_g.NMG_debug & DEBUG_FINDEU)
		    	rt_log("\tFound %8x/%8x\n", eu, eu->eumate_p);

		if ( *eu->up.magic_p == NMG_LOOPUSE_MAGIC &&
		     *eu->up.lu_p->up.magic_p == NMG_FACEUSE_MAGIC &&
		     eup_orientation != eu->up.lu_p->up.fu_p->orientation)
			eu = eu->eumate_p;	/* Take other orient */
		goto out;
	}
	eu = (struct edgeuse *)NULL;
out:
	if (rt_g.NMG_debug & DEBUG_FINDEU)
	    	rt_log("nmg_findeu() returns x%x\n", eu);

	return (struct edgeuse *)eu;
}

/*
 *			N M G _ F I N D _ E U _ I N _ F A C E
 *
 *  An analog to nmg_findeu(), only restricted to searching a faceuse,
 *  rather than to a whole shell.
 */
struct edgeuse *
nmg_find_eu_in_face( v1, v2, fu, eup, dangling_only )
CONST struct vertex	*v1;
CONST struct vertex	*v2;
CONST struct faceuse	*fu;
CONST struct edgeuse	*eup;
int		dangling_only;
{
	register CONST struct vertexuse	*vu;
	register CONST struct edgeuse	*eu;
	CONST struct edgeuse		*eup_mate;
	int				eup_orientation;

	NMG_CK_VERTEX(v1);
	NMG_CK_VERTEX(v2);
	if(fu) NMG_CK_FACEUSE(fu);

	if(eup)  {
		struct faceuse	*fu;
		NMG_CK_EDGEUSE(eup);
		eup_mate = eup->eumate_p;
		NMG_CK_EDGEUSE(eup_mate);
		if( (fu = nmg_find_fu_of_eu(eup)) )
			eup_orientation = fu->orientation;
		else
			eup_orientation = OT_SAME;
	} else {
		eup_mate = eup;			/* NULL */
		eup_orientation = OT_SAME;
	}

	if (rt_g.NMG_debug & DEBUG_FINDEU)
		rt_log("nmg_find_eu_in_face() seeking eu!=%8x/%8x between (%8x, %8x) %s\n",
			eup, eup_mate, v1, v2,
			dangling_only ? "[dangling]" : "[any]" );

	for( RT_LIST_FOR( vu, vertexuse, &v1->vu_hd ) )  {
		NMG_CK_VERTEXUSE(vu);
		if (!vu->up.magic_p)
			rt_bomb("nmg_find_eu_in_face() vertexuse in vu_hd list has null parent\n");

		/* Ignore self-loops and lone shell verts */
		if (*vu->up.magic_p != NMG_EDGEUSE_MAGIC )  continue;
		eu = vu->up.eu_p;

		/* Ignore edgeuses which don't run between the right verts */
		if( eu->eumate_p->vu_p->v_p != v2 )  continue;

		if (rt_g.NMG_debug & DEBUG_FINDEU )  {
			rt_log("nmg_find_eu_in_face: check eu=%8x vertex=(%8x, %8x)\n",
				eu, eu->vu_p->v_p,
				eu->eumate_p->vu_p->v_p);
		}

		/* Ignore the edgeuse to be excluded */
		if( eu == eup || eu->eumate_p == eup )  {
			if (rt_g.NMG_debug & DEBUG_FINDEU )
				rt_log("\tIgnoring -- excluded edgeuse\n");
			continue;
		}

		/* See if this edgeuse is in the proper faceuse */
		if( fu && nmg_find_fu_of_eu(eu) != fu )  {
		    	if (rt_g.NMG_debug & DEBUG_FINDEU)
		    		rt_log("\tIgnoring x%x -- eu not in faceuse\n", eu);
			continue;
		}

		/* If it's not a dangling edge, skip on */
		if( dangling_only && eu->eumate_p != eu->radial_p) {
		    	if (rt_g.NMG_debug & DEBUG_FINDEU)  {
			    	rt_log("\tIgnoring %8x/%8x (radial=x%x)\n",
			    		eu, eu->eumate_p,
					eu->radial_p );
		    	}
			continue;
		}

	    	if (rt_g.NMG_debug & DEBUG_FINDEU)
		    	rt_log("\tFound %8x/%8x\n", eu, eu->eumate_p);

		if ( *eu->up.magic_p == NMG_LOOPUSE_MAGIC &&
		     *eu->up.lu_p->up.magic_p == NMG_FACEUSE_MAGIC &&
		     eup_orientation != eu->up.lu_p->up.fu_p->orientation)
			eu = eu->eumate_p;	/* Take other orient */
		goto out;
	}
	eu = (struct edgeuse *)NULL;
out:
	if (rt_g.NMG_debug & DEBUG_FINDEU)
	    	rt_log("nmg_find_eu_in_face() returns x%x\n", eu);

	return (struct edgeuse *)eu;
}

/*
 *			N M G _ F I N D _ E U _ O F _ V U
 *
 *  Return a pointer to the edgeuse which is the parent of this vertexuse.
 *
 *  A simple helper routine, which replaces the amazingly bad sequence of:
 * 	nmg_find_eu_with_vu_in_lu( nmg_find_lu_of_vu(vu), vu )
 *  that was being used in several places.
 */
struct edgeuse *
nmg_find_eu_of_vu(vu)
CONST struct vertexuse	*vu;
{
	NMG_CK_VERTEXUSE(vu);
	if( *vu->up.magic_p != NMG_EDGEUSE_MAGIC )
		return (struct edgeuse *)NULL;
	return vu->up.eu_p;
}

/*
 *			N M G _ F I N D _ E U _ W I T H _ V U _ I N _ L U
 *
 *  Find an edgeuse starting at a given vertexuse within a loopuse.
 */
struct edgeuse *
nmg_find_eu_with_vu_in_lu( lu, vu )
CONST struct loopuse		*lu;
CONST struct vertexuse	*vu;
{
	register struct edgeuse	*eu;

	NMG_CK_LOOPUSE(lu);
	NMG_CK_VERTEXUSE(vu);
	if( RT_LIST_FIRST_MAGIC(&lu->down_hd) != NMG_EDGEUSE_MAGIC )
		rt_bomb("nmg_find_eu_with_vu_in_lu: loop has no edges!\n");
	for( RT_LIST_FOR( eu, edgeuse, &lu->down_hd ) )  {
		NMG_CK_EDGEUSE(eu);
		if( eu->vu_p == vu )  return eu;
	}
	rt_bomb("nmg_find_eu_with_vu_in_lu:  Unable to find vu!\n");
	/* NOTREACHED */
	return((struct edgeuse *)NULL);
}

/*				N M G _ F A C E R A D I A L
 *
 *	Looking radially around an edge, find another edge in the same
 *	face as the current edge. (this could be the mate to the current edge)
 */
CONST struct edgeuse *
nmg_faceradial(eu)
CONST struct edgeuse *eu;
{
	CONST struct faceuse *fu;
	CONST struct edgeuse *eur;

	NMG_CK_EDGEUSE(eu);
	NMG_CK_LOOPUSE(eu->up.lu_p);
	fu = eu->up.lu_p->up.fu_p;
	NMG_CK_FACEUSE(fu);

	eur = eu->radial_p;

	while (*eur->up.magic_p != NMG_LOOPUSE_MAGIC ||
	    *eur->up.lu_p->up.magic_p != NMG_FACEUSE_MAGIC ||
	    eur->up.lu_p->up.fu_p->f_p != fu->f_p)
	    	eur = eur->eumate_p->radial_p;

	return(eur);
}


/*
 *			N M G _ R A D I A L _ F A C E _ E D G E _ I N _ S H E L L
 *
 *	looking radially around an edge, find another edge which is a part
 *	of a face in the same shell
 */
CONST struct edgeuse *
nmg_radial_face_edge_in_shell(eu)
CONST struct edgeuse *eu;
{
	CONST struct edgeuse *eur;
	CONST struct shell	*s;

	NMG_CK_EDGEUSE(eu);
	s = nmg_find_s_of_eu(eu);

	eur = eu->radial_p;
	NMG_CK_EDGEUSE(eur);

	while (eur != eu->eumate_p) {
		if (*eur->up.magic_p == NMG_LOOPUSE_MAGIC &&
		    *eur->up.lu_p->up.magic_p == NMG_FACEUSE_MAGIC &&
		    eur->up.lu_p->up.fu_p->s_p == s) {
			break; /* found another face in shell */
		} else {
			eur = eur->eumate_p->radial_p;
			NMG_CK_EDGEUSE(eur);
		}
	}
	return(eur);
}

/*
 *			N M G _ F I N D _ E D G E _ B E T W E E N _ 2 F U
 *
 *  Perform a topology search to determine if two faces (specified by
 *  their faceuses) share an edge in common.  If so, return an edgeuse
 *  in fu1 of that edge.
 *
 *  If there are multiple edgeuses in common, ensure that they all refer
 *  to the same edge_g_lseg geometry structure.  The intersection of two planes
 *  (non-coplanar) must be a single line.
 *
 *  Calling this routine when the two faces share face geometry
 *  and have more than one edge in common gives
 *  a NULL return, as there is no unique answer.
 *
 *  NULL is also returned if no common edge could be found.
 */
CONST struct edgeuse *
nmg_find_edge_between_2fu(fu1, fu2, tol)
CONST struct faceuse	*fu1;
CONST struct faceuse	*fu2;
CONST struct rt_tol	*tol;
{
	CONST struct loopuse	*lu1;
	CONST struct edgeuse	*ret = (CONST struct edgeuse *)NULL;
	int			coincident;

	NMG_CK_FACEUSE(fu1);
	NMG_CK_FACEUSE(fu2);
	RT_CK_TOL(tol);

	for( RT_LIST_FOR( lu1, loopuse, &fu1->lu_hd ) )  {
		CONST struct edgeuse	*eu1;
		NMG_CK_LOOPUSE(lu1);
		if( RT_LIST_FIRST_MAGIC(&lu1->down_hd) == NMG_VERTEXUSE_MAGIC )
			continue;
		for( RT_LIST_FOR( eu1, edgeuse, &lu1->down_hd ) )  {
			CONST struct edgeuse *eur;

			NMG_CK_EDGEUSE(eu1);
			/* Walk radially around the edge */
			eur = eu1->radial_p;
			NMG_CK_EDGEUSE(eur);

			while (eur != eu1->eumate_p) {
				if (*eur->up.magic_p == NMG_LOOPUSE_MAGIC &&
				    *eur->up.lu_p->up.magic_p == NMG_FACEUSE_MAGIC &&
				    eur->up.lu_p->up.fu_p->f_p == fu2->f_p) {
				    	/* Found the other face on this edge! */
				    	if( !ret )  {
				    		/* First common edge found */
				    		if( eur->up.lu_p->up.fu_p == fu2)  {
				    			ret = eur;
				    		} else {
				    			ret = eur->eumate_p;
				    		}
				    	} else {
				    		/* Previous edge found, check edge_g_lseg */
				    		if( eur->g.lseg_p != ret->g.lseg_p )  {
				    			rt_log("eur=x%x, eg_p=x%x;  ret=x%x, eg_p=x%x\n",
				    				eur, eur->g.lseg_p,
				    				ret, ret->g.lseg_p );
				    			nmg_pr_eg( eur->g.lseg_p, 0 );
				    			nmg_pr_eg( ret->g.lseg_p, 0 );
				    			nmg_pr_eu_endpoints( eur, 0 );
				    			nmg_pr_eu_endpoints( ret, 0 );

							coincident = nmg_2edgeuse_g_coincident( eur, ret, tol );
							if( coincident )  {
								/* Change eur to use ret's eg */
								rt_log("nmg_find_edge_between_2fu() belatedly fusing e1=x%x, eg1=x%x, e2=x%x, eg2=x%x\n",
									eur->e_p, eur->g.lseg_p,
									ret->e_p, ret->g.lseg_p );
								nmg_jeg( ret->g.lseg_p, eur->g.lseg_p );
								/* See if there are any others. */
								nmg_model_fuse( nmg_find_model(&eur->l.magic), tol );
							} else {
					    			rt_bomb("nmg_find_edge_between_2fu() 2 faces intersect with differing edge geometries?\n");
							}
				    		}
				    	}
				}
				/* Advance to next */
				eur = eur->eumate_p->radial_p;
				NMG_CK_EDGEUSE(eur);
			}
		}
	}
	if (rt_g.NMG_debug & DEBUG_BASIC)  rt_log("nmg_find_edge_between_2fu(fu1=x%x, fu2=x%x) edgeuse=x%x\n", fu1, fu2, ret);
	return ret;

}

/*
 *  Support for nmg_find_e_nearest_pt2().
 */
struct fen2d_state {
	char		*visited;	/* array of edges already visited */
	fastf_t		mindist;	/* current min dist */
	struct edge	*ep;		/* closest edge */
	mat_t		mat;
	point_t		pt2;
	CONST struct rt_tol	*tol;
};

static void
nmg_find_e_pt2_handler( lp, state, first )
long		*lp;
genptr_t	state;
int		first;
{
	register struct fen2d_state	*sp = (struct fen2d_state *)state;
	register struct edge		*e = (struct edge *)lp;
	fastf_t				dist_sq;
	point_t				a2, b2;
	struct vertex			*va, *vb;
	point_t				pca;
	int				code;

	RT_CK_TOL(sp->tol);
	NMG_CK_EDGE(e);

	/* If this edge has been processed before, do nothing more */
	if( !NMG_INDEX_FIRST_TIME(sp->visited, e) )  return;

	va = e->eu_p->vu_p->v_p;
	vb = e->eu_p->eumate_p->vu_p->v_p;
	NMG_CK_VERTEX(va);
	NMG_CK_VERTEX(vb);

	MAT4X3PNT( a2, sp->mat, va->vg_p->coord );
	MAT4X3PNT( b2, sp->mat, vb->vg_p->coord );

	code = rt_dist_pt2_lseg2( &dist_sq, pca, a2, b2, sp->pt2, sp->tol );

	if( code == 0 )  dist_sq = 0;

	if( dist_sq < sp->mindist )  {
		sp->mindist = dist_sq;
		sp->ep = e;
	}
}

/*
 *			N M G _ F I N D _ E _ N E A R E S T _ P T 2
 *
 *  A geometric search routine to find the edge that is neaest to
 *  the given point, when all edges are projected into 2D using
 *  the matrix 'mat'.
 *  Useful for finding the edge nearest a mouse click, for example.
 */
struct edge *
nmg_find_e_nearest_pt2( magic_p, pt2, mat, tol )
long		*magic_p;
CONST point_t	pt2;		/* 2d point */
CONST mat_t	mat;		/* 3d to 3d xform */
CONST struct rt_tol	*tol;
{
	struct model			*m;
	struct nmg_visit_handlers	htab;
	struct fen2d_state		st;

	RT_CK_TOL(tol);
	m = nmg_find_model( magic_p );
	NMG_CK_MODEL(m);

	st.visited = (char *)rt_calloc(m->maxindex+1, sizeof(char), "visited[]");
	st.mindist = INFINITY;
	VMOVE( st.pt2, pt2 );
	mat_copy( st.mat, mat );
	st.ep = (struct edge *)NULL;
	st.tol = tol;

	htab = nmg_visit_handlers_null;		/* struct copy */
	htab.vis_edge = nmg_find_e_pt2_handler;

	nmg_visit( magic_p, &htab, &st );

	rt_free( (char *)st.visited, "visited[]" );

	if( st.ep )  {
		NMG_CK_EDGE(st.ep);
		return st.ep;
	}
	return (struct edge *)NULL;
}

/*
 *			N M G _ E U _ 2 V E C S _ P E R P
 *
 *  Given an edgeuse, return two arbitrary unit-length vectors which
 *  are perpendicular to each other and to the edgeuse, such that
 *  they can be considered the +X and +Y axis, and the edgeuse is +Z.
 *  That is, X cross Y = Z.
 *
 *  Useful for erecting a coordinate system around an edge suitable
 *  for measuring the angles of other edges and faces with.
 */
void
nmg_eu_2vecs_perp( xvec, yvec, zvec, eu, tol )
vect_t		xvec;
vect_t		yvec;
vect_t		zvec;
CONST struct edgeuse	*eu;
CONST struct rt_tol	*tol;
{
	CONST struct vertex	*v1, *v2;
	fastf_t			len;

	NMG_CK_EDGEUSE(eu);
	v1 = eu->vu_p->v_p;
	NMG_CK_VERTEX(v1);
	v2 = eu->eumate_p->vu_p->v_p;
	NMG_CK_VERTEX(v2);
	if( v1 == v2 )  rt_bomb("nmg_eu_2vecs_perp() start&end vertex of edge are the same!\n");
	RT_CK_TOL(tol);

	NMG_CK_VERTEX_G(v1->vg_p);
	NMG_CK_VERTEX_G(v2->vg_p);
	VSUB2( zvec, v2->vg_p->coord, v1->vg_p->coord );
	len = MAGNITUDE(zvec);
	/* See if v1 == v2, within tol */
	if( len < tol->dist )  rt_bomb("nmg_eu_2vecs_perp(): 0-length edge (geometry)\n");
	len = 1 / len;
	VSCALE( zvec, zvec, len );

	mat_vec_perp( xvec, zvec );
	VCROSS( yvec, zvec, xvec );
}

/*
 *			N M G _ F I N D _ E U _ L E F T V E C
 *
 *  Given an edgeuse, if it is part of a faceuse, return the inward pointing
 *  "left" vector which points into the interior of this loop, and
 *  lies in the plane of the face.
 *
 *  This routine depends on the vertex ordering in an OT_SAME loopuse being
 *  properly CCW for exterior loops, and CW for interior (hole) loops.
 *
 *  Returns -
 *	-1	if edgeuse is not part of a faceuse.
 *	 0	if left vector successfully computed into caller's array.
 */
int
nmg_find_eu_leftvec( left, eu )
vect_t			left;
CONST struct edgeuse	*eu;
{
	CONST struct loopuse	*lu;
	CONST struct faceuse	*fu;
	vect_t			Norm;
	vect_t			edgevect;

	NMG_CK_EDGEUSE(eu);
	if( *eu->up.magic_p != NMG_LOOPUSE_MAGIC )  return -1;
	lu = eu->up.lu_p;
	NMG_CK_LOOPUSE(lu);
	if( *lu->up.magic_p != NMG_FACEUSE_MAGIC )  return -1;
	fu = lu->up.fu_p;
	NMG_CK_FACEUSE(fu);
	NMG_CK_FACE(fu->f_p);
	NMG_CK_FACE_G_PLANE(fu->f_p->g.plane_p);

	/* Get unit length Normal vector for edgeuse's faceuse */
	NMG_GET_FU_NORMAL( Norm, fu );

	VSUB2( edgevect, eu->eumate_p->vu_p->v_p->vg_p->coord,
		eu->vu_p->v_p->vg_p->coord );
	VUNITIZE( edgevect );

	VCROSS( left, Norm, edgevect );
	return 0;
}

/************************************************************************
 *									*
 *				VERTEX Routines				*
 *									*
 ************************************************************************/

/*
 *			N M G _ F I N D _ V _ I N _ F A C E
 *
 *	Perform a topological search to
 *	determine if the given vertex is contained in the given faceuse.
 *	If it is, return a pointer to the vertexuse which was found in the
 *	faceuse.
 *
 *  Returns NULL if not found.
 */
struct vertexuse *
nmg_find_v_in_face(v, fu)
CONST struct vertex	*v;
CONST struct faceuse	*fu;
{
	struct vertexuse *vu;
	struct edgeuse *eu;
	struct loopuse *lu;

	NMG_CK_VERTEX(v);

	for( RT_LIST_FOR( vu, vertexuse, &v->vu_hd ) )  {
		NMG_CK_VERTEXUSE(vu);
		if (*vu->up.magic_p == NMG_EDGEUSE_MAGIC) {
			eu = vu->up.eu_p;
			if (*eu->up.magic_p == NMG_LOOPUSE_MAGIC) {
				lu = eu->up.lu_p;
				if (*lu->up.magic_p == NMG_FACEUSE_MAGIC && lu->up.fu_p == fu)
					return(vu);
			}

		} else if (*vu->up.magic_p == NMG_LOOPUSE_MAGIC) {
			lu = vu->up.lu_p;
			if (*lu->up.magic_p == NMG_FACEUSE_MAGIC && lu->up.fu_p == fu)
				return(vu);
		}
	}
	return((struct vertexuse *)NULL);
}

/*
 *			N M G _ F I N D _ V _ I N _ S H E L L
 *
 *  Search shell "s" for a vertexuse that refers to vertex "v".
 *  For efficiency, the search is done on the uses of "v".
 *
 *  If "edges_only" is set, only a vertexuse from an edgeuse will
 *  be returned, otherwise, vu's from self-loops and lone-shell-vu's
 *  are also candidates.
 */
struct vertexuse *
nmg_find_v_in_shell( v, s, edges_only )
CONST struct vertex	*v;
CONST struct shell	*s;
int			edges_only;
{
	struct vertexuse	*vu;

	for( RT_LIST_FOR( vu, vertexuse, &v->vu_hd ) )  {
		NMG_CK_VERTEXUSE(vu);

		if( *vu->up.magic_p == NMG_LOOPUSE_MAGIC )  {
			if( edges_only )  continue;
			if( nmg_find_s_of_lu( vu->up.lu_p ) == s )
				return vu;
			continue;
		}
		if( *vu->up.magic_p == NMG_SHELL_MAGIC )  {
			if( edges_only )  continue;
			if( vu->up.s_p == s )
				return vu;
			continue;
		}
		if( *vu->up.magic_p != NMG_EDGEUSE_MAGIC )
			rt_bomb("nmg_find_v_in_shell(): bad vu up ptr\n");

		/* vu is being used by an edgeuse */
		if( nmg_find_s_of_eu( vu->up.eu_p ) == s )
			return vu;
	}
	return (struct vertexuse *)NULL;
}

/*
 *			N M G _ F I N D _ P T _ I N _ L U
 *
 *  Conduct a geometric search for a vertex in loopuse 'lu' which is
 *  "identical" to the given point, within the specified tolerance.
 *  The loopuse may be part of a face, or it may be wires.
 *
 *  Returns -
 *	NULL			No vertex matched
 *	(struct vertexuse *)	A matching vertexuse from that loopuse.
 */
struct vertexuse *
nmg_find_pt_in_lu(lu, pt, tol)
CONST struct loopuse	*lu;
CONST point_t		pt;
CONST struct rt_tol	*tol;
{
	struct edgeuse		*eu;
	vect_t			delta;
	struct vertex		*v;
	register struct vertex_g *vg;
	int			magic1;

	magic1 = RT_LIST_FIRST_MAGIC( &lu->down_hd );
	if (magic1 == NMG_VERTEXUSE_MAGIC) {
		struct vertexuse	*vu;
		vu = RT_LIST_FIRST(vertexuse, &lu->down_hd);
		v = vu->v_p;
		NMG_CK_VERTEX(v);
		if( !(vg = v->vg_p) )
			return ((struct vertexuse *)NULL);
		NMG_CK_VERTEX_G(vg);
		VSUB2(delta, vg->coord, pt);
		if ( MAGSQ(delta) <= tol->dist_sq)
			return(vu);
		return ((struct vertexuse *)NULL);
	}
	if (magic1 != NMG_EDGEUSE_MAGIC) {
		rt_bomb("nmg_find_pt_in_lu() Bogus child of loop\n");
	}

	for( RT_LIST_FOR( eu, edgeuse, &lu->down_hd ) )  {
		v = eu->vu_p->v_p;
		NMG_CK_VERTEX(v);
		if( !(vg = v->vg_p) )  continue;
		NMG_CK_VERTEX_G(vg);
		VSUB2(delta, vg->coord, pt);
		if ( MAGSQ(delta) <= tol->dist_sq)
			return(eu->vu_p);
	}
	return ((struct vertexuse *)NULL);

}

/*
 *			N M G _ F I N D _ P T _ I N _ F A C E
 *
 *  Conduct a geometric search for a vertex in face 'fu' which is
 *  "identical" to the given point, within the specified tolerance.
 *
 *  Returns -
 *	NULL			No vertex matched
 *	(struct vertexuse *)	A matching vertexuse from that face.
 */
struct vertexuse *
nmg_find_pt_in_face(fu, pt, tol)
CONST struct faceuse	*fu;
CONST point_t		pt;
CONST struct rt_tol	*tol;
{
	register struct loopuse	*lu;
	struct vertexuse	*vu;

	NMG_CK_FACEUSE(fu);
	RT_CK_TOL(tol);

	for( RT_LIST_FOR( lu, loopuse, &fu->lu_hd ) )  {
		NMG_CK_LOOPUSE(lu);
		if( (vu = nmg_find_pt_in_lu(lu, pt, tol)) )
			return vu;
	}
	return ((struct vertexuse *)NULL);
}

/*
 *			N M G _ F I N D _ P T _ I N _ S H E L L
 *
 *  Given a point in 3-space and a shell pointer, try to find a vertex
 *  anywhere in the shell which is within sqrt(tol_sq) distance of
 *  the given point.
 *
 *  The algorithm is a brute force, and should be used sparingly.
 *  Mike Gigante's NUgrid technique would work well here, if
 *  searching was going to be needed for more than a few points.
 *
 *  Returns -
 *	pointer to vertex with matching geometry
 *	NULL
 *
 *  XXX Why does this return a vertex, while it's helpers return a vertexuse?
 */
struct vertex *
nmg_find_pt_in_shell( s, pt, tol )
CONST struct shell	*s;
CONST point_t		pt;
CONST struct rt_tol	*tol;
{
	CONST struct faceuse	*fu;
	CONST struct loopuse	*lu;
	CONST struct edgeuse	*eu;
	CONST struct vertexuse	*vu;
	struct vertex		*v;
	CONST struct vertex_g	*vg;
	vect_t		delta;

	NMG_CK_SHELL(s);
	RT_CK_TOL(tol);

	fu = RT_LIST_FIRST(faceuse, &s->fu_hd);
	while (RT_LIST_NOT_HEAD(fu, &s->fu_hd) ) {
		/* Shell has faces */
		NMG_CK_FACEUSE(fu);
		if( (vu = nmg_find_pt_in_face( fu, pt, tol )) )
			return(vu->v_p);

		if (RT_LIST_PNEXT(faceuse, fu) == fu->fumate_p)
			fu = RT_LIST_PNEXT_PNEXT(faceuse, fu);
		else
			fu = RT_LIST_PNEXT(faceuse, fu);
	}

	/* Wire loopuses */
	lu = RT_LIST_FIRST(loopuse, &s->lu_hd);
	while (RT_LIST_NOT_HEAD(lu, &s->lu_hd) ) {
		NMG_CK_LOOPUSE(lu);
		if( (vu = nmg_find_pt_in_lu(lu, pt, tol)) )
			return vu->v_p;

		if (RT_LIST_PNEXT(loopuse, lu) == lu->lumate_p)
			lu = RT_LIST_PNEXT_PNEXT(loopuse, lu);
		else
			lu = RT_LIST_PNEXT(loopuse, lu);
	}

	/* Wire edgeuses */
	for (RT_LIST_FOR(eu, edgeuse, &s->eu_hd)) {
		NMG_CK_EDGEUSE(eu);
		NMG_CK_VERTEXUSE(eu->vu_p);
		v = eu->vu_p->v_p;
		NMG_CK_VERTEX(v);
		if( (vg = v->vg_p) )  {
			NMG_CK_VERTEX_G(vg);
			VSUB2( delta, vg->coord, pt );
			if( MAGSQ(delta) <= tol->dist_sq )
				return(v);
		}
	}

	/* Lone vertexuse */
	if (s->vu_p) {
		NMG_CK_VERTEXUSE(s->vu_p);
		v = s->vu_p->v_p;
		NMG_CK_VERTEX(v);
		if( (vg = v->vg_p) )  {
			NMG_CK_VERTEX_G( vg );
			VSUB2( delta, vg->coord, pt );
			if( MAGSQ(delta) <= tol->dist_sq )
				return(v);
		}
	}
	return( (struct vertex *)0 );
}

/*
 *			N M G _ F I N D _ P T _ I N _ M O D E L
 *
 *  Brute force search of the entire model to find a vertex that
 *  matches this point.
 *  XXX Shouldn't this return the _closest_ match, not just the
 *  XXX first match within tolerance?
 */
struct vertex *
nmg_find_pt_in_model( m, pt, tol )
CONST struct model	*m;
CONST point_t		pt;
CONST struct rt_tol	*tol;
{
	struct nmgregion	*r;
	struct shell		*s;
	struct vertex		*v;

	NMG_CK_MODEL(m);
	RT_CK_TOL(tol);

	for( RT_LIST_FOR( r, nmgregion, &m->r_hd ) )  {
		NMG_CK_REGION(r);
		for( RT_LIST_FOR( s, shell, &r->s_hd ) )  {
			NMG_CK_SHELL(s);
			if( (v = nmg_find_pt_in_shell( s, pt, tol )) )  {
				NMG_CK_VERTEX(v);
				return v;
			}
		}
	}
	return (struct vertex *)NULL;
}

/*
 *			N M G _ I S _ V E R T E X _ I N _ E D G E L I S T
 *
 *  Returns -
 *	1	If found
 *	0	If not found
 */
int
nmg_is_vertex_in_edgelist( v, hd )
register CONST struct vertex	*v;
CONST struct rt_list		*hd;
{
	register CONST struct edgeuse	*eu;

	NMG_CK_VERTEX(v);
	for( RT_LIST_FOR( eu, edgeuse, hd ) )  {
		NMG_CK_EDGEUSE(eu);
		NMG_CK_VERTEXUSE(eu->vu_p);
		NMG_CK_VERTEX(eu->vu_p->v_p);
		if( eu->vu_p->v_p == v )  return(1);
	}
	return(0);
}

/*
 *			N M G _ I S _ V E R T E X _ I N _ L O O P L I S T
 *
 *  Returns -
 *	1	If found
 *	0	If not found
 */
int
nmg_is_vertex_in_looplist( v, hd, singletons )
register CONST struct vertex	*v;
CONST struct rt_list		*hd;
int				singletons;
{
	register CONST struct loopuse	*lu;
	long			magic1;

	NMG_CK_VERTEX(v);
	for( RT_LIST_FOR( lu, loopuse, hd ) )  {
		NMG_CK_LOOPUSE(lu);
		magic1 = RT_LIST_FIRST_MAGIC( &lu->down_hd );
		if( magic1 == NMG_VERTEXUSE_MAGIC )  {
			register CONST struct vertexuse	*vu;
			if( !singletons )  continue;
			vu = RT_LIST_FIRST(vertexuse, &lu->down_hd );
			NMG_CK_VERTEXUSE(vu);
			NMG_CK_VERTEX(vu->v_p);
			if( vu->v_p == v )  return(1);
		} else if( magic1 == NMG_EDGEUSE_MAGIC )  {
			if( nmg_is_vertex_in_edgelist( v, &lu->down_hd ) )
				return(1);
		} else {
			rt_bomb("nmg_is_vertex_in_loopuse() bad magic\n");
		}
	}
	return(0);
}

/*
 *	N M G _ I S _ V E R T E X _ A _ S E L F L O O P _ I N _ S H E L L
 *
 *  Check to see if a given vertex is used within a shell
 *  by a wire loopuse which is a loop of a single vertex.
 *  The search could either be by the shell lu_hd, or the vu_hd.
 *
 *  Returns -
 *	0	vertex is not part of that kind of loop in the shell.
 *	1	vertex is part of a selfloop in the given shell.
 */
int
nmg_is_vertex_a_selfloop_in_shell(v, s)
CONST struct vertex	*v;
CONST struct shell	*s;
{
	CONST struct vertexuse *vu;

	NMG_CK_VERTEX(v);
	NMG_CK_SHELL(s);

	/* try to find the vertex in a loopuse of this shell */
	for (RT_LIST_FOR(vu, vertexuse, &v->vu_hd)) {
		register CONST struct loopuse	*lu;
		NMG_CK_VERTEXUSE(vu);
		if (*vu->up.magic_p != NMG_LOOPUSE_MAGIC )  continue;
		lu = vu->up.lu_p;
		NMG_CK_LOOPUSE(lu);
		if( *lu->up.magic_p != NMG_SHELL_MAGIC )  continue;
		NMG_CK_SHELL(lu->up.s_p);
		if( lu->up.s_p == s)
			return 1;
	}
	return 0;
}

/*
 *			N M G _ I S _ V E R T E X _ I N _ F A C E L I S T
 *
 *  Returns -
 *	1	If found
 *	0	If not found
 */
int
nmg_is_vertex_in_facelist( v, hd )
register CONST struct vertex	*v;
CONST struct rt_list		*hd;
{
	register CONST struct faceuse	*fu;

	NMG_CK_VERTEX(v);
	for( RT_LIST_FOR( fu, faceuse, hd ) )  {
		NMG_CK_FACEUSE(fu);
		if( nmg_is_vertex_in_looplist( v, &fu->lu_hd, 1 ) )
			return(1);
	}
	return(0);
}

/*
 *			N M G _ I S _ E D G E _ I N _ E D G E L I S T
 *
 *  Returns -
 *	1	If found
 *	0	If not found
 */
int
nmg_is_edge_in_edgelist( e, hd )
CONST struct edge	*e;
CONST struct rt_list	*hd;
{
	register CONST struct edgeuse	*eu;

	NMG_CK_EDGE(e);
	for( RT_LIST_FOR( eu, edgeuse, hd ) )  {
		NMG_CK_EDGEUSE(eu);
		NMG_CK_EDGE(eu->e_p);
		if( e == eu->e_p )  return(1);
	}
	return(0);
}

/*
 *			N M G _ I S _ E D G E _ I N _ L O O P L I S T
 *
 *  Returns -
 *	1	If found
 *	0	If not found
 */
int
nmg_is_edge_in_looplist( e, hd )
CONST struct edge	*e;
CONST struct rt_list	*hd;
{
	register CONST struct loopuse	*lu;
	long			magic1;

	NMG_CK_EDGE(e);
	for( RT_LIST_FOR( lu, loopuse, hd ) )  {
		NMG_CK_LOOPUSE(lu);
		magic1 = RT_LIST_FIRST_MAGIC( &lu->down_hd );
		if( magic1 == NMG_VERTEXUSE_MAGIC )  {
			/* Loop of a single vertex does not have an edge */
			continue;
		} else if( magic1 == NMG_EDGEUSE_MAGIC )  {
			if( nmg_is_edge_in_edgelist( e, &lu->down_hd ) )
				return(1);
		} else {
			rt_bomb("nmg_is_edge_in_loopuse() bad magic\n");
		}
	}
	return(0);
}

/*
 *			N M G _ I S _ E D G E _ I N _ F A C E L I S T
 *
 *  Returns -
 *	1	If found
 *	0	If not found
 */
int
nmg_is_edge_in_facelist( e, hd )
CONST struct edge	*e;
CONST struct rt_list	*hd;
{
	register CONST struct faceuse	*fu;

	NMG_CK_EDGE(e);
	for( RT_LIST_FOR( fu, faceuse, hd ) )  {
		NMG_CK_FACEUSE(fu);
		if( nmg_is_edge_in_looplist( e, &fu->lu_hd ) )
			return(1);
	}
	return(0);
}

/*
 *			N M G _ I S _ L O O P _ I N _ F A C E L I S T
 *
 *  Returns -
 *	1	If found
 *	0	If not found
 */
int
nmg_is_loop_in_facelist( l, fu_hd )
CONST struct loop	*l;
CONST struct rt_list	*fu_hd;
{
	register CONST struct faceuse	*fu;
	register CONST struct loopuse	*lu;

	NMG_CK_LOOP(l);
	for( RT_LIST_FOR( fu, faceuse, fu_hd ) )  {
		NMG_CK_FACEUSE(fu);
		for( RT_LIST_FOR( lu, loopuse, &fu->lu_hd ) )  {
			NMG_CK_LOOPUSE(lu);
			NMG_CK_LOOP(lu->l_p);
			if( l == lu->l_p )  return(1);
		}
	}
	return(0);
}

struct vf_state {
	char		*visited;
	struct nmg_ptbl	*tabl;
};

/*
 *			N M G _ 2 R V F _ H A N D L E R
 *
 *  A private support routine for nmg_vertex_tabulate().
 *  Having just visited a vertex, if this is the first time,
 *  add it to the nmg_ptbl array.
 */
static void
nmg_2rvf_handler( vp, state, first )
long		*vp;
genptr_t	state;
int		first;
{
	register struct vf_state *sp = (struct vf_state *)state;
	register struct vertex	*v = (struct vertex *)vp;

	NMG_CK_VERTEX(v);
	/* If this vertex has been processed before, do nothing more */
	if( !NMG_INDEX_FIRST_TIME(sp->visited, v) )  return;

	nmg_tbl( sp->tabl, TBL_INS, vp );
}

/*
 *			N M G _ V E R T E X _ T A B U L A T E
 *
 *  Given a pointer to any nmg data structure,
 *  build an nmg_ptbl list which has every vertex
 *  pointer from there on "down" in the model, each one listed exactly once.
 */
void
nmg_vertex_tabulate( tab, magic_p )
struct nmg_ptbl		*tab;
CONST long		*magic_p;
{
	struct model		*m;
	struct vf_state		st;
	struct nmg_visit_handlers	handlers;

	m = nmg_find_model( magic_p );
	NMG_CK_MODEL(m);

	st.visited = (char *)rt_calloc(m->maxindex+1, sizeof(char), "visited[]");
	st.tabl = tab;

	(void)nmg_tbl( tab, TBL_INIT, 0 );

	handlers = nmg_visit_handlers_null;		/* struct copy */
	handlers.vis_vertex = nmg_2rvf_handler;
	nmg_visit( magic_p, &handlers, (genptr_t)&st );

	rt_free( (char *)st.visited, "visited[]");
}

/*
 *			N M G _ 2 E D G E U S E _ H A N D L E R
 *
 *  A private support routine for nmg_edgeuse_tabulate().
 *  Having just visited a edgeuse, if this is the first time,
 *  add it to the nmg_ptbl array.
 */
static void
nmg_2edgeuse_handler( eup, state, first )
long		*eup;
genptr_t	state;
int		first;
{
	register struct vf_state *sp = (struct vf_state *)state;
	register struct edgeuse	*eu = (struct edgeuse *)eup;

	NMG_CK_EDGEUSE(eu);
	/* If this edgeuse has been processed before, do nothing more */
	if( !NMG_INDEX_FIRST_TIME(sp->visited, eu) )  return;

	nmg_tbl( sp->tabl, TBL_INS, eup );
}

/*
 *			N M G _ E D G E U S E _ T A B U L A T E
 *
 *  Given a pointer to any nmg data structure,
 *  build an nmg_ptbl list which has every edgeuse
 *  pointer from there on "down" in the model, each one listed exactly once.
 */
void
nmg_edgeuse_tabulate( tab, magic_p )
struct nmg_ptbl		*tab;
CONST long		*magic_p;
{
	struct model		*m;
	struct vf_state		st;
	struct nmg_visit_handlers	handlers;

	m = nmg_find_model( magic_p );
	NMG_CK_MODEL(m);

	st.visited = (char *)rt_calloc(m->maxindex+1, sizeof(char), "visited[]");
	st.tabl = tab;

	(void)nmg_tbl( tab, TBL_INIT, 0 );

	handlers = nmg_visit_handlers_null;		/* struct copy */
	handlers.bef_edgeuse = nmg_2edgeuse_handler;
	nmg_visit( magic_p, &handlers, (genptr_t)&st );

	rt_free( (char *)st.visited, "visited[]");
}

/*
 *			N M G _ 2 E D G E _ H A N D L E R
 *
 *  A private support routine for nmg_edge_tabulate().
 *  Having just visited a edge, if this is the first time,
 *  add it to the nmg_ptbl array.
 */
static void
nmg_2edge_handler( ep, state, first )
long		*ep;
genptr_t	state;
int		first;
{
	register struct vf_state *sp = (struct vf_state *)state;
	register struct edge	*e = (struct edge *)ep;

	NMG_CK_EDGE(e);
	/* If this edge has been processed before, do nothing more */
	if( !NMG_INDEX_FIRST_TIME(sp->visited, e) )  return;

	nmg_tbl( sp->tabl, TBL_INS, ep );
}

/*
 *			N M G _ E D G E _ T A B U L A T E
 *
 *  Given a pointer to any nmg data structure,
 *  build an nmg_ptbl list which has every edge
 *  pointer from there on "down" in the model, each one listed exactly once.
 */
void
nmg_edge_tabulate( tab, magic_p )
struct nmg_ptbl		*tab;
CONST long		*magic_p;
{
	struct model		*m;
	struct vf_state		st;
	struct nmg_visit_handlers	handlers;

	m = nmg_find_model( magic_p );
	NMG_CK_MODEL(m);

	st.visited = (char *)rt_calloc(m->maxindex+1, sizeof(char), "visited[]");
	st.tabl = tab;

	(void)nmg_tbl( tab, TBL_INIT, 0 );

	handlers = nmg_visit_handlers_null;		/* struct copy */
	handlers.vis_edge = nmg_2edge_handler;
	nmg_visit( magic_p, &handlers, (genptr_t)&st );

	rt_free( (char *)st.visited, "visited[]");
}

/*
 *			N M G _ E D G E _ G _ H A N D L E R
 *
 *  A private support routine for nmg_edge_g_tabulate().
 *  Having just visited an edge_g_lseg, if this is the first time,
 *  add it to the nmg_ptbl array.
 */
static void
nmg_edge_g_handler( ep, state, first )
long		*ep;
genptr_t	state;
int		first;
{
	register struct vf_state *sp = (struct vf_state *)state;

	NMG_CK_EDGE_G_EITHER(ep);

	/* If this edge has been processed before, do nothing more */
	switch( *ep )  {
	case NMG_EDGE_G_LSEG_MAGIC:
		if( !NMG_INDEX_FIRST_TIME(sp->visited, ((struct edge_g_lseg *)ep)) )
			return;
		break;
	case NMG_EDGE_G_CNURB_MAGIC:
		if( !NMG_INDEX_FIRST_TIME(sp->visited, ((struct edge_g_cnurb *)ep)) )
			return;
		break;
	}

	nmg_tbl( sp->tabl, TBL_INS, ep );
}

/*
 *			N M G _ E D G E _ G _ T A B U L A T E
 *
 *  Given a pointer to any nmg data structure,
 *  build an nmg_ptbl list which has every edge
 *  pointer from there on "down" in the model, each one listed exactly once.
 */
void
nmg_edge_g_tabulate( tab, magic_p )
struct nmg_ptbl		*tab;
CONST long		*magic_p;
{
	struct model		*m;
	struct vf_state		st;
	struct nmg_visit_handlers	handlers;

	m = nmg_find_model( magic_p );
	NMG_CK_MODEL(m);

	st.visited = (char *)rt_calloc(m->maxindex+1, sizeof(char), "visited[]");
	st.tabl = tab;

	(void)nmg_tbl( tab, TBL_INIT, 0 );

	handlers = nmg_visit_handlers_null;		/* struct copy */
	handlers.vis_edge_g = nmg_edge_g_handler;
	nmg_visit( magic_p, &handlers, (genptr_t)&st );

	rt_free( (char *)st.visited, "visited[]");
}

/*
 *			N M G _ 2 F A C E _ H A N D L E R
 *
 *  A private support routine for nmg_face_tabulate().
 *  Having just visited a face, if this is the first time,
 *  add it to the nmg_ptbl array.
 */
static void
nmg_2face_handler( fp, state, first )
long		*fp;
genptr_t	state;
int		first;
{
	register struct vf_state *sp = (struct vf_state *)state;
	register struct face	*f = (struct face *)fp;

	NMG_CK_FACE(f);
	/* If this face has been processed before, do nothing more */
	if( !NMG_INDEX_FIRST_TIME(sp->visited, f) )  return;

	nmg_tbl( sp->tabl, TBL_INS, fp );
}

/*
 *			N M G _ F A C E _ T A B U L A T E
 *
 *  Given a pointer to any nmg data structure,
 *  build an nmg_ptbl list which has every face
 *  pointer from there on "down" in the model, each one listed exactly once.
 */
void
nmg_face_tabulate( tab, magic_p )
struct nmg_ptbl		*tab;
CONST long		*magic_p;
{
	struct model		*m;
	struct vf_state		st;
	struct nmg_visit_handlers	handlers;

	m = nmg_find_model( magic_p );
	NMG_CK_MODEL(m);

	st.visited = (char *)rt_calloc(m->maxindex+1, sizeof(char), "visited[]");
	st.tabl = tab;

	(void)nmg_tbl( tab, TBL_INIT, 0 );

	handlers = nmg_visit_handlers_null;		/* struct copy */
	handlers.vis_face = nmg_2face_handler;
	nmg_visit( magic_p, &handlers, (genptr_t)&st );

	rt_free( (char *)st.visited, "visited[]");
}

struct eg_state {
	char			*visited;
	struct nmg_ptbl		*tabl;
	CONST struct edge_g_lseg	*eg;
};

/*
 *			N M G _ 2 E D G E G E O M _ H A N D L E R
 *
 *  A private support routine for nmg_edgegeom_tabulate().
 *  Having just visited an edgeuse, if this is the first time,
 *  and this edgeuse uses the designated edge geometry,
 *  add it to the nmg_ptbl array.
 */
static void
nmg_2edgegeom_handler( longp, state, first )
long		*longp;
genptr_t	state;
int		first;
{
	register struct eg_state *sp = (struct eg_state *)state;
	register struct edgeuse	*eu = (struct edgeuse *)longp;

	NMG_CK_EDGEUSE(eu);
	/* If this edgeuse has been processed before, do nothing more */
	if( !NMG_INDEX_FIRST_TIME(sp->visited, eu) )  return;

	if( eu->g.lseg_p != sp->eg )  return;

	nmg_tbl( sp->tabl, TBL_INS, longp );
}

/*
 *			N M G _ E D G E U S E _ W I T H _ E G _ T A B U L A T E
 *
 *  Given a pointer to any nmg data structure,
 *  build an nmg_ptbl list which cites every edgeuse
 *  pointer from there on "down" in the model
 *  that uses edge geometry "eg".
 */
void
nmg_edgeuse_with_eg_tabulate( tab, magic_p, eg )
struct nmg_ptbl		*tab;
CONST long		*magic_p;
CONST struct edge_g_lseg	*eg;
{
	struct model		*m;
	struct eg_state		st;
	struct nmg_visit_handlers	handlers;

	m = nmg_find_model( magic_p );
	NMG_CK_MODEL(m);
	NMG_CK_EDGE_G_LSEG(eg);

	/* XXX Need to re-write this to use eg->eu_hd list instead */

	st.visited = (char *)rt_calloc(m->maxindex+1, sizeof(char), "visited[]");
	st.tabl = tab;
	st.eg = eg;

	(void)nmg_tbl( tab, TBL_INIT, 0 );

	handlers = nmg_visit_handlers_null;		/* struct copy */
	handlers.bef_edgeuse = nmg_2edgegeom_handler;
	nmg_visit( magic_p, &handlers, (genptr_t)&st );

	rt_free( (char *)st.visited, "visited[]");
}

struct edge_line_state {
	char			*visited;
	struct nmg_ptbl		*tabl;
	point_t			pt;
	vect_t			dir;
	CONST struct rt_tol	*tol;
};

/*
 *			N M G _ L I N E _ H A N D L E R
 *
 *  A private support routine for nmg_edgeuse_on_line_tabulate.
 *  Having just visited an edgeuse, if this is the first time,
 *  and both vertices of this edgeuse lie within tol of the line,
 *  add it to the nmg_ptbl array.
 */
static void
nmg_line_handler( longp, state, first )
long		*longp;
genptr_t	state;
int		first;
{
	register struct edge_line_state *sp = (struct edge_line_state *)state;
	register struct edgeuse	*eu = (struct edgeuse *)longp;

	NMG_CK_EDGEUSE(eu);
	/* If this edgeuse has been processed before, do nothing more */
	if( !NMG_INDEX_FIRST_TIME(sp->visited, eu) )  return;

	/*  If the lines are not generally parallel, don't bother with
	 *  checking the endpoints.  This helps reject very short edges
	 *  which are colinear only by virtue of being very small.
	 */
	RT_CK_TOL(sp->tol);
	NMG_CK_EDGE_G_LSEG(eu->g.lseg_p);
#if 1
	/* XXX This assumes unit vectors.  These are not.  Fixing this causes lots of error messages, though. */
	/* ERROR: vu=x10063d54 v=x10050578 is on isect line, tvu=x10063614 eu=x10081b30 isn't */
	if( fabs( VDOT( eu->g.lseg_p->e_dir, sp->dir ) ) < sp->tol->para )
		return;
#else
	/* sp->tol->para and RT_DOT_TOL are too tight. 0.1 is 5 degrees */
	if( fabs( VDOT( eu->g.lseg_p->e_dir, sp->dir ) ) < 0.9 * MAGNITUDE(eu->g.lseg_p->e_dir) * MAGNITUDE(sp->dir) )
		return;
#endif
	if( rt_distsq_line3_pt3( sp->pt, sp->dir, eu->vu_p->v_p->vg_p->coord )
	    > sp->tol->dist_sq )
		return;
	if( rt_distsq_line3_pt3( sp->pt, sp->dir, eu->eumate_p->vu_p->v_p->vg_p->coord )
	    > sp->tol->dist_sq )
		return;

	/* Both points are within tolerance, add edgeuse to the list */
	nmg_tbl( sp->tabl, TBL_INS, longp );
}

/*
 *			N M G _ E D G E U S E _ O N _ L I N E _ T A B U L A T E
 *
 *  Given a pointer to any nmg data structure,
 *  build an nmg_ptbl list which cites every edgeuse
 *  pointer from there on "down" in the model
 *  that has both vertices within tolerance of the given line.
 */
void
nmg_edgeuse_on_line_tabulate( tab, magic_p, pt, dir, tol )
struct nmg_ptbl		*tab;
CONST long		*magic_p;
CONST point_t		pt;
CONST vect_t		dir;
CONST struct rt_tol	*tol;
{
	struct model		*m;
	struct edge_line_state		st;
	struct nmg_visit_handlers	handlers;

	m = nmg_find_model( magic_p );
	NMG_CK_MODEL(m);
	RT_CK_TOL(tol);

	st.visited = (char *)rt_calloc(m->maxindex+1, sizeof(char), "visited[]");
	st.tabl = tab;
	VMOVE( st.pt, pt );
	VMOVE( st.dir, dir );
	st.tol = tol;

	(void)nmg_tbl( tab, TBL_INIT, 0 );

	handlers = nmg_visit_handlers_null;		/* struct copy */
	handlers.bef_edgeuse = nmg_line_handler;
	nmg_visit( magic_p, &handlers, (genptr_t)&st );

	rt_free( (char *)st.visited, "visited[]");
}
