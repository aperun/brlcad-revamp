/*
 *			N M G _ C L A S S . C
 *
 *  Subroutines to classify one object with respect to another.
 *  Possible classifications are AinB, AoutB, AinBshared, AinBanti.
 *
 *  The first set of routines (nmg_class_pt_xxx) are used to classify
 *  an arbitrary point specified by it's Cartesian coordinates,
 *  against various kinds of NMG elements.
 *  nmg_class_pt_f() and nmg_class_pt_s() are available to
 *  applications programmers for direct use, and have no side effects.
 *
 *  The second set of routines (class_xxx_vs_s) are used only to support
 *  the routine nmg_class_shells() mid-way through the NMG Boolean
 *  algorithm.  These routines operate with special knowledge about
 *  the state of the data structures after the intersector has been called,
 *  and depends on all geometric equivalences to have been converted into
 *  shared topology.
 *
 *  Authors -
 *	Lee A. Butler
 *	Michael John Muuss
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
#include "externs.h"
#include "vmath.h"
#include "nmg.h"
#include "rtlist.h"
#include "raytrace.h"
#include "./debug.h"

extern int nmg_class_nothing_broken;

/* XXX These should go the way of the dodo bird. */
#define INSIDE	32
#define ON_SURF	64
#define OUTSIDE	128

/*	Structure for keeping track of how close a point/vertex is to
 *	its potential neighbors.
 */
struct neighbor {
	union {
		CONST struct vertexuse *vu;
		CONST struct edgeuse *eu;
	} p;
	/* XXX For efficiency, this should have been dist_sq */
	fastf_t	dist;	/* distance from point to neighbor */
	int	class;	/* Classification of this neighbor */
};

static void	joint_hitmiss2 RT_ARGS( (struct neighbor *closest,
			CONST struct edgeuse *eu, CONST point_t pt,
			int code) );
static void	nmg_class_pt_e RT_ARGS( (struct neighbor *closest,
			CONST point_t pt, CONST struct edgeuse *eu,
			CONST struct rt_tol *tol) );
static void	nmg_class_pt_l RT_ARGS( (struct neighbor *closest, 
			CONST point_t pt, CONST struct loopuse *lu,
			CONST struct rt_tol *tol) );
static int	class_vu_vs_s RT_ARGS( (struct vertexuse *vu, struct shell *sB,
			long *classlist[4], CONST struct rt_tol	*tol) );
static int	class_eu_vs_s RT_ARGS( (struct edgeuse *eu, struct shell *s,
			long *classlist[4], CONST struct rt_tol	*tol) );
static int	class_lu_vs_s RT_ARGS( (struct loopuse *lu, struct shell *s,
			long *classlist[4], CONST struct rt_tol	*tol) );
static void	class_fu_vs_s RT_ARGS( (struct faceuse *fu, struct shell *s,
			long *classlist[4], CONST struct rt_tol	*tol) );

/*
 *			N M G _ C L A S S _ S T A T U S
 *
 *  Convert classification status to string.
 */
CONST char *
nmg_class_status(status)
int	status;
{
	switch(status)  {
	case INSIDE:
		return "INSIDE";
	case OUTSIDE:
		return "OUTSIDE";
	case ON_SURF:
		return "ON_SURF";
	}
	return "BOGUS_CLASS_STATUS";
}

/*
 *			N M G _ P R _ C L A S S _ S T A T U S
 */
void
nmg_pr_class_status( prefix, status )
char	*prefix;
int	status;
{
	rt_log("%s has classification status %s\n",
		prefix, nmg_class_status(status) );
}

/*
 *			J O I N T _ H I T M I S S 2
 *
 *	The ray hit an edge.  We have to decide whether it hit the
 *	edge, or missed it.
 *
 *  XXX DANGER:  This routine does not work well.
 *
 *  Called by -
 *	nmg_class_pt_e
 */
static void 
joint_hitmiss2(closest, eu, pt, code)
struct neighbor		*closest;
CONST struct edgeuse	*eu;
CONST point_t		pt;
int			code;
{
	CONST struct edgeuse *eu_rinf;

	eu_rinf = nmg_faceradial(eu);

	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		rt_log("joint_hitmiss2\n");
	}
	if( eu_rinf == eu )  {
		rt_bomb("joint_hitmiss2: radial eu is me?\n");
	}
	/* If eu_rinf == eu->eumate_p, thats OK, this is a dangling face,
	 * or a face that has not been fully hooked up yet.
	 * It's OK as long the the orientations both match.
	 */
	if( eu->up.lu_p->orientation == eu_rinf->up.lu_p->orientation ) {
		if (eu->up.lu_p->orientation == OT_SAME) {
			closest->class = NMG_CLASS_AonBshared;
		} else if (eu->up.lu_p->orientation == OT_OPPOSITE) {
			closest->class = NMG_CLASS_AoutB;
		} else {
			nmg_pr_lu_briefly(eu->up.lu_p, (char *)0);
			rt_bomb("joint_hitmiss2: bad loop orientation\n");
		}
		closest->dist = 0.0;
		switch(code)  {
		case 0:
			/* Hit the edge */
			closest->p.eu = eu;
			break;
		case 1:
			/* Hit first vertex */
			closest->p.vu = eu->vu_p;
			break;
		case 2:
			/* Hit second vertex */
			closest->p.vu = RT_LIST_PNEXT_CIRC(edgeuse,eu)->vu_p;
			break;
		}
		if (rt_g.NMG_debug & DEBUG_CLASSIFY) rt_log("\t\t%s\n", nmg_class_name(closest->class) );
	    	return;
	}

	/* XXX What goes here? */
	rt_bomb("nmg_class.c/joint_hitmiss2() unable to resolve ray/edge hit\n");
	rt_log("joint_hitmiss2: NO CODE HERE, assuming miss\n");

	if (rt_g.NMG_debug & (DEBUG_CLASSIFY|DEBUG_NMGRT) )  {
		nmg_euprint("Hard question time", eu);
		rt_log(" eu_rinf=x%x, eu->eumate_p=x%x, eu=x%x\n", eu_rinf, eu->eumate_p, eu);
		rt_log(" eu lu orient=%s, eu_rinf lu orient=%s\n",
			nmg_orientation(eu->up.lu_p->orientation),
			nmg_orientation(eu_rinf->up.lu_p->orientation) );
	}
}

/*
 *			N M G _ C L A S S _ P T _ E
 *
 *  XXX DANGER:  This routine does not work well.
 *
 *  Given the Cartesian coordinates of a point, determine if the point
 *  is closer to this edgeuse than the previous neighbor(s) as given
 *  in the "closest" structure.
 *  If it is, record how close the point is, and whether it is IN, ON, or OUT.
 *  The neighor's "p" element will indicate the edgeuse or vertexuse closest.
 *
 *  This routine should print everything indented two tab stops.
 *
 *  Implicit Return -
 *	Updated "closest" structure if appropriate.
 *
 *  Called by -
 *	nmg_class_pt_l
 */
static void
nmg_class_pt_e(closest, pt, eu, tol)
struct neighbor		*closest;
CONST point_t		pt;
CONST struct edgeuse	*eu;
CONST struct rt_tol	*tol;
{
	vect_t	ptvec;	/* vector from lseg to pt */
	vect_t	left;	/* vector left of edge -- into inside of loop */
	CONST fastf_t	*eupt;
	CONST fastf_t	*matept;
	point_t pca;	/* point of closest approach from pt to edge lseg */
	fastf_t dist;	/* distance from pca to pt */
	fastf_t dot;
	int	code;

	NMG_CK_EDGEUSE(eu);
	NMG_CK_EDGEUSE(eu->eumate_p);
	NMG_CK_VERTEX_G(eu->vu_p->v_p->vg_p);
	NMG_CK_VERTEX_G(eu->eumate_p->vu_p->v_p->vg_p);
	RT_CK_TOL(tol);
	VSETALL(left, 0);

	eupt = eu->vu_p->v_p->vg_p->coord;
	matept = eu->eumate_p->vu_p->v_p->vg_p->coord;

	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		VPRINT("nmg_class_pt_e\tPt", pt);
		nmg_euprint("          \tvs. eu", eu);
	}
	/*
	 * Note that "pca" can be one of the edge endpoints,
	 * even if "pt" is far, far away.  This can be confusing.
	 */
	code = rt_dist_pt3_lseg3( &dist, pca, eupt, matept, pt, tol);
	if( code <= 0 )  dist = 0;
	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		rt_log("          \tcode=%d, dist: %g\n", code, dist);
		VPRINT("          \tpca:", pca);
	}

	if (dist >= closest->dist + tol->dist ) {
 		if(rt_g.NMG_debug & DEBUG_CLASSIFY) {
 			rt_log("\t\tskipping, earlier eu is closer (%g)\n", closest->dist);
 		}
		return;
 	}
 	if( dist >= closest->dist - tol->dist )  {
 		/*
 		 *  Distances are very nearly the same.
		 *
 		 *  If closest eu and this eu are from the same lu,
 		 *  and the earlier result was OUT, that's all it takes
 		 *  to decide things.
 		 */
 		if( closest->p.eu && closest->p.eu->up.lu_p == eu->up.lu_p )  {
 			if( closest->class == NMG_CLASS_AoutB ||
			    closest->class == NMG_CLASS_AonBshared )  {
		 		if(rt_g.NMG_debug & DEBUG_CLASSIFY)
					rt_log("\t\tSkipping, earlier eu from same lu at same dist, is OUT or ON.\n");
	 			return;
 			}
	 		if(rt_g.NMG_debug & DEBUG_CLASSIFY)
				rt_log("\t\tEarlier eu from same lu at same dist, is IN, continue processing.\n");
		} else {
			/*
	 		 *  If this is now a new lu, it is more complicated.
	 		 *  If "closest" result so far was a NMG_CLASS_AinB or
			 *  or NMG_CLASS_AonB, then keep it,
	 		 *  otherwise, replace that result with whatever we find
	 		 *  here.  Logic:  Two touching loops, one concave ("A")
			 *  which wraps around part of the other ("B"), with the
	 		 *  point inside A near the contact with B.  If loop B is
			 *  processed first, the closest result will be NMG_CLASS_AoutB,
	 		 *  and when loop A is visited the distances will be exactly
	 		 *  equal, not giving A a chance to claim it's hit.
	 		 */
	 		if( closest->class == NMG_CLASS_AinB ||
			    closest->class == NMG_CLASS_AonBshared )  {
		 		if(rt_g.NMG_debug & DEBUG_CLASSIFY)
					rt_log("\t\tSkipping, earlier eu from other another lu at same dist, is IN or ON\n");
	 			return;
	 		}
	 		if(rt_g.NMG_debug & DEBUG_CLASSIFY)
				rt_log("\t\tEarlier eu from other lu at same dist, is OUT, continue processing.\n");
		}
	}

	/* Plane hit point is closer to this edgeuse than previous one(s) */
	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		rt_log("\t\tCLOSER dist=%g (closest=%g), tol=%g\n",
			dist, closest->dist, tol->dist);
	}

	if (*eu->up.magic_p != NMG_LOOPUSE_MAGIC ||
	    *eu->up.lu_p->up.magic_p != NMG_FACEUSE_MAGIC) {
		rt_log("Trying to classify a pt (%g, %g, %g)\n\tvs a wire edge? (%g, %g, %g -> %g, %g, %g)\n",
	    		V3ARGS(pt),
	    		V3ARGS(eupt),
	    		V3ARGS(matept)
		);
	    	return;
	}

	if( code <= 2 )  {
		/* code==0:  The point is ON the edge! */
		/* code==1 or 2:  The point is ON a vertex! */
    		if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
    			rt_log("\t\tThe point is ON the edge, calling joint_hitmiss2()\n");
    		}
   		joint_hitmiss2(closest, eu, pt, code);
		return;
    	} else {
    		if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
			rt_log("\t\tThe point is not on the edge\n");
    		}
    	}

	/* The point did not lie exactly ON the edge, calculate in/out */

    	/* Get vector which lies on the plane, and points
    	 * left, towards the interior of the loop, regardless of
	 * whether it's an interior (OT_OPPOSITE) or exterior (OT_SAME) loop.
    	 */
	if( nmg_find_eu_leftvec( left, eu ) < 0 )  rt_bomb("nmg_class_pt_e() bad LEFT vector\n");

	VSUB2(ptvec, pt, pca);		/* pt - pca */
    	if (rt_g.NMG_debug & DEBUG_CLASSIFY)  {
		VPRINT("\t\tptvec unnorm", ptvec);
    		VPRINT("\t\tleft", left);
    	}
	VUNITIZE( ptvec );

	dot = VDOT(left, ptvec);
	if( NEAR_ZERO( dot, RT_DOT_TOL )  )  {
	    	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
	    		rt_log("\t\tpt lies on line of edge, outside verts. Skipping this edge\n");
		goto out;
	}

	if (dot >= 0.0) {
		closest->dist = dist;
		closest->p.eu = eu;
		closest->class = NMG_CLASS_AinB;
	    	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
	    		rt_log("\t\tpt is left of edge, INSIDE loop, dot=%g\n", dot);
	} else {
		closest->dist = dist;
		closest->p.eu = eu;
		closest->class = NMG_CLASS_AoutB;
	    	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
	    		rt_log("\t\tpt is right of edge, OUTSIDE loop\n");
	}

out:
	/* XXX Temporary addition for chasing bug in Bradley r5 */
	/* XXX Should at least add DEBUG_PLOTEM check, later */
	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		struct faceuse	*fu;
		char	buf[128];
		static int	num;
		FILE	*fp;
		long	*bits;
		point_t	mid_pt;
		point_t	left_pt;
		fu = eu->up.lu_p->up.fu_p;
		bits = (long *)rt_calloc( nmg_find_model(&fu->l.magic)->maxindex, sizeof(long), "bits[]");
		sprintf(buf,"faceclass%d.pl", num++);
		if( (fp = fopen(buf, "w")) == NULL) rt_bomb(buf);
		nmg_pl_fu( fp, fu, bits, 0, 0, 255 );	/* blue */
		pl_color( fp, 0, 255, 0 );	/* green */
		pdv_3line( fp, pca, pt );
		pl_color( fp, 255, 0, 0 );	/* red */
		VADD2SCALE( mid_pt, eupt, matept, 0.5 );
		VJOIN1( left_pt, mid_pt, 500, left);
		pdv_3line( fp, mid_pt, left_pt );
		fclose(fp);
		rt_free( (char *)bits, "bits[]");
		rt_log("wrote %s\n", buf);
	}
}


/*
 *			N M G _ C L A S S _ P T _ L
 *
 *  XXX DANGER:  This routine does not work well.
 *
 *  Given the coordinates of a point which lies on the plane of the face
 *  containing a loopuse, determine if the point is IN, ON, or OUT of the
 *  area enclosed by the loop.
 *
 *  Implicit Return -
 *	Updated "closest" structure if appropriate.
 *
 *  Called by -
 *	nmg_class_pt_loop()
 *		from: nmg_extrude.c / nmg_fix_overlapping_loops()
 *	nmg_classify_lu_lu()
 *		from: nmg_misc.c / nmg_split_loops_handler()
 */
static void
nmg_class_pt_l(closest, pt, lu, tol)
struct neighbor		*closest;
CONST point_t		pt;
CONST struct loopuse	*lu;
CONST struct rt_tol	*tol;
{
	vect_t		delta;
	pointp_t	lu_pt;
	fastf_t		dist;
	struct edgeuse	*eu;
	struct loop_g	*lg;

	NMG_CK_LOOPUSE(lu);
	NMG_CK_LOOP(lu->l_p);
	lg = lu->l_p->lg_p;
	NMG_CK_LOOP_G(lg);

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)  {
		VPRINT("nmg_class_pt_l\tPt:", pt);
	}

	if (*lu->up.magic_p != NMG_FACEUSE_MAGIC)
		return;

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)  {
		plane_t		peqn;
		nmg_pr_lu_briefly(lu, 0);
		NMG_GET_FU_PLANE( peqn, lu->up.fu_p );
		HPRINT("\tplane eqn", peqn);
	}

	if( !V3PT_IN_RPP_TOL( pt, lg->min_pt, lg->max_pt, tol ) )  {
		if (rt_g.NMG_debug & DEBUG_CLASSIFY)
			rt_log("\tPoint is outside loop RPP\n");
		return;
	}
	if (RT_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_EDGEUSE_MAGIC) {
		for (RT_LIST_FOR(eu, edgeuse, &lu->down_hd)) {
			nmg_class_pt_e(closest, pt, eu, tol);
			/* If point lies ON edge, we are done */
			if( closest->class == NMG_CLASS_AonBshared )  break;
		}
	} else if (RT_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_VERTEXUSE_MAGIC) {
		register struct vertexuse *vu;
		vu = RT_LIST_FIRST(vertexuse, &lu->down_hd);
		lu_pt = vu->v_p->vg_p->coord;
		VSUB2(delta, pt, lu_pt);
		if ( (dist = MAGNITUDE(delta)) < closest->dist) {
			if (lu->orientation == OT_OPPOSITE) {
				closest->class = NMG_CLASS_AoutB;
			} else if (lu->orientation == OT_SAME) {
				closest->class = NMG_CLASS_AonBshared;
			} else {
				nmg_pr_orient(lu->orientation, "\t");
				rt_bomb("nmg_class_pt_l: bad orientation for face loopuse\n");
			}
			if (rt_g.NMG_debug & DEBUG_CLASSIFY)
				rt_log("\t\t closer to loop pt (%g, %g, %g)\n",
					V3ARGS(lu_pt) );

			closest->dist = dist;
			closest->p.vu = vu;
		}
	} else {
		rt_bomb("nmg_class_pt_l() bad child of loopuse\n");
	}
	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("nmg_class_pt_l\treturning, closest=%g %s\n",
			closest->dist, nmg_class_name(closest->class) );
}

/*
 *			N M G _ C L A S S _ L U _ F U
 *
 *  This is intended as an internal routine to support nmg_lu_reorient().
 *
 *  Given a loopuse in a face, pick one of it's vertexuses, and classify
 *  that point with respect to all the rest of the loopuses in the face.
 *  The containment status of that point is the status of the loopuse.
 *
 *  If the first vertex chosen is "ON" another loop boundary,
 *  choose the next vertex and try again.  Only return an "ON"
 *  status if _all_ the vertices are ON.
 *
 *  The point is "A", and the face is "B".
 *
 *  Returns -
 *	NMG_CLASS_AinB		lu is INSIDE the area of the face.
 *	NMG_CLASS_AonBshared	ALL of lu is ON other loop boundaries.
 *	NMG_CLASS_AoutB		lu is OUTSIDE the area of the face.
 *
 *  Called by -
 *	nmg_mod.c, nmg_lu_reorient()
 */
int
nmg_class_lu_fu(lu, tol)
CONST struct loopuse	*lu;
CONST struct rt_tol	*tol;
{
	CONST struct faceuse	*fu;
	struct vertexuse	*vu;
	CONST struct vertex_g	*vg;
	struct edgeuse		*eu;
	struct edgeuse		*eu_first;
	fastf_t			dist;
	plane_t			n;
	int			class;

	NMG_CK_LOOPUSE(lu);
	RT_CK_TOL(tol);

	fu = lu->up.fu_p;
	NMG_CK_FACEUSE(fu);
	NMG_CK_FACE(fu->f_p);
	NMG_CK_FACE_G_PLANE(fu->f_p->g.plane_p);

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("nmg_class_lu_fu(lu=x%x) START\n", lu);

	/* Pick first vertex in loopuse, for point */
	if( RT_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_VERTEXUSE_MAGIC )  {
		vu = RT_LIST_FIRST(vertexuse, &lu->down_hd);
		eu = (struct edgeuse *)NULL;
	} else {
		eu = RT_LIST_FIRST(edgeuse, &lu->down_hd);
		NMG_CK_EDGEUSE(eu);
		vu = eu->vu_p;
	}
	eu_first = eu;
again:
	NMG_CK_VERTEXUSE(vu);
	NMG_CK_VERTEX(vu->v_p);
	vg = vu->v_p->vg_p;
	NMG_CK_VERTEX_G(vg);

	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		VPRINT("nmg_class_lu_fu\tPt:", vg->coord);
	}

	/* Validate distance from point to plane */
	NMG_GET_FU_PLANE( n, fu );
	if( (dist=fabs(DIST_PT_PLANE( vg->coord, n ))) > tol->dist )  {
		rt_log("nmg_class_lu_fu() ERROR, point (%g,%g,%g) not on face, dist=%g\n",
			V3ARGS(vg->coord), dist );
	}

	/* find the closest approach in this face to the projected point */
	class = nmg_class_pt_fu_except( vg->coord, fu, lu,
		NULL, NULL, NULL, 0, tol );

	/* If this vertex lies ON loop edge, must check all others. */
	if( class == NMG_CLASS_AonBshared )  {
		if( !eu_first )  {
			/* was self-loop, nothing more to test */
		} else {
			eu = RT_LIST_PNEXT_CIRC(edgeuse, &eu->l);
			if( eu != eu_first )  {
				vu = eu->vu_p;
				goto again;
			}
			/* all match, call it "ON" */
		}
	}

	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		rt_log("nmg_class_lu_fu(lu=x%x) END, ret=%s\n",
			lu,
			nmg_class_name(class) );
	}
	return class;
}

/* Ray direction vectors for Jordan curve algorithm */
static CONST point_t nmg_good_dirs[10] = {
#if 1
	3, 2, 1,	/* Normally the first dir */
#else
	1, 0, 0,	/* Make this first dir to wring out ray-tracer XXX */
#endif
	1, 0, 0,
	0, 1, 0,
	0, 0, 1,
	1, 1, 1,
	-3,-2,-1,
	-1,0, 0,
	0,-1, 0,
	0, 0,-1,
	-1,-1,-1
};

/*
 *			N M G _ C L A S S _ P T _ S
 *
 *  This is intended as a general user interface routine.
 *  Given the Cartesian coordinates for a point in space,
 *  return the classification for that point with respect to a shell.
 *
 *  The algorithm used is to fire a ray from the point, and count
 *  the number of times it crosses a face.
 *
 *  The point is "A", and the face is "B".
 *
 *  Returns -
 *	NMG_CLASS_AinB		pt is INSIDE the volume of the shell.
 *	NMG_CLASS_AonBshared	pt is ON the shell boundary.
 *	NMG_CLASS_AoutB		pt is OUTSIDE the volume of the shell.
 */
int
nmg_class_pt_s(pt, s, tol)
CONST point_t		pt;
CONST struct shell	*s;
CONST struct rt_tol	*tol;
{
	int		hitcount = 0;
	int		stat;
	point_t 	plane_pt;
	CONST struct faceuse	*fu;
	struct model	*m;
	long		*faces_seen;
	vect_t		region_diagonal;
	fastf_t		region_diameter;
	int		class;
	vect_t		projection_dir;
	int		try=0;
	struct xray	rp;

	NMG_CK_SHELL(s);
	RT_CK_TOL(tol);

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("nmg_class_pt_s:\tpt=(%g, %g, %g), s=x%x\n",
			V3ARGS(pt), s );

	if( !V3PT_IN_RPP_TOL(pt, s->sa_p->min_pt, s->sa_p->max_pt, tol) )  {
		if (rt_g.NMG_debug & DEBUG_CLASSIFY)
			rt_log("	OUT, point not in RPP\n");
		return NMG_CLASS_AoutB;
	}

	m = s->r_p->m_p;
	NMG_CK_MODEL(m);
	faces_seen = (long *)rt_calloc( m->maxindex, sizeof(long), "nmg_class_pt_s faces_seen[]" );

	/*
	 *  First pass:  Try hard to see if point is ON a face.
	 */
	for( RT_LIST_FOR(fu, faceuse, &s->fu_hd) )  {
		plane_t	n;

		/* If this face processed before, skip on */
		if( NMG_INDEX_TEST( faces_seen, fu->f_p ) )  continue;

		/* Only consider the outward pointing faceuses */
		if( fu->orientation != OT_SAME )  continue;

		/* See if this point lies on this face */
		NMG_GET_FU_PLANE( n, fu );
		if( (fabs(DIST_PT_PLANE(pt, n))) < tol->dist)  {
			/* Point lies on this plane, it may be possible to
			 * short circuit everything.
			 */
			class = nmg_class_pt_fu_except(pt, fu, (struct loopuse *)0,
				(void (*)())NULL, (void (*)())NULL, (char *)NULL, 0,
				tol);
			if( class == NMG_CLASS_AonBshared )  {
				/* Point is ON face, therefore it must be
				 * ON the shell also.
				 */
				class = NMG_CLASS_AonBshared;
				goto out;
			}
			if( class == NMG_CLASS_AinB )  {
				/* Point is IN face, therefor it must be
				 * ON the shell also.
				 */
				class = NMG_CLASS_AonBshared;
				goto out;
			}
			/* Point is OUTside face, its undecided. */
		}

		/* Mark this face as having been processed */
		NMG_INDEX_SET(faces_seen, fu->f_p);
	}


	/* If we got here, the point isn't ON any of the faces.
	 * Time to do the Jordan Curve Theorem.  We fire an arbitrary
	 * ray and count the number of crossings (in the positive direction)
	 * If that number is even, we're outside the shell, otherwise we're
	 * inside the shell.
	 */
	NMG_CK_REGION_A(s->r_p->ra_p);
	VSUB2( region_diagonal, s->r_p->ra_p->max_pt, s->r_p->ra_p->min_pt );
	region_diameter = MAGNITUDE(region_diagonal);

	/* Choose an unlikely direction */
	try = 0;
retry:
	VMOVE( projection_dir, nmg_good_dirs[try] );
	if( ++try > 10 )  rt_bomb("nmg_class_pt_s() retry count exhausted\n");
	VUNITIZE(projection_dir);

	if (rt_g.NMG_debug & DEBUG_CLASSIFY )
		rt_log("\tPt=(%g, %g, %g) dir=(%g, %g, %g), reg_diam=%g\n",
			V3ARGS(pt), V3ARGS(projection_dir), region_diameter);
	
	VMOVE(rp.r_pt, pt);
	VMOVE(rp.r_dir, projection_dir);


	/* get NMG ray-tracer to tell us if start point is inside or outside */
	class = nmg_class_ray_vs_shell(&rp, s, tol);
	if( class == NMG_CLASS_Unknown )  goto retry;

out:
	rt_free( (char *)faces_seen, "nmg_class_pt_s faces_seen[]" );
	if (rt_g.NMG_debug & DEBUG_CLASSIFY )
		rt_log("nmg_class_pt_s: returning %s, s=x%x, try=%d\n",
			nmg_class_name(class), s, try );
	return class;
}


/*
 *			C L A S S _ V U _ V S _ S
 *
 *	Classify a loopuse/vertexuse from shell A WRT shell B.
 */
static int 
class_vu_vs_s(vu, sB, classlist, tol)
struct vertexuse	*vu;
struct shell		*sB;
long			*classlist[4];
CONST struct rt_tol	*tol;
{
	struct vertexuse *vup;
	pointp_t pt;
	char	*reason;
	int	status;
	int	class;

	NMG_CK_VERTEXUSE(vu);
	NMG_CK_SHELL(sB);
	RT_CK_TOL(tol);

	pt = vu->v_p->vg_p->coord;

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("class_vu_vs_s(vu=x%x, v=x%x) pt=(%g,%g,%g)\n", vu, vu->v_p, V3ARGS(pt) );

	/* As a mandatory consistency measure, check for cached classification */
	reason = "of cached classification";
	if( NMG_INDEX_TEST(classlist[NMG_CLASS_AinB], vu->v_p) )  {
		status = INSIDE;
		goto out;
	}
	if( NMG_INDEX_TEST(classlist[NMG_CLASS_AonBshared], vu->v_p) )  {
		status = ON_SURF;
		goto out;
	}
	if( NMG_INDEX_TEST(classlist[NMG_CLASS_AonBanti], vu->v_p) )  {
		status = ON_SURF;
		goto out;
	}
	if( NMG_INDEX_TEST(classlist[NMG_CLASS_AoutB], vu->v_p) )  {
		status = OUTSIDE;
		goto out;
	}

	/* we use topology to determing if the vertex is "ON" the
	 * other shell.
	 */
	for(RT_LIST_FOR(vup, vertexuse, &vu->v_p->vu_hd)) {

		if (*vup->up.magic_p == NMG_LOOPUSE_MAGIC )  {
			if( nmg_find_s_of_lu(vup->up.lu_p) != sB) continue;
		    	NMG_INDEX_SET(classlist[NMG_CLASS_AonBshared], 
		    		vu->v_p );
		    	reason = "a loopuse of vertex is on shell";
		    	status = ON_SURF;
			goto out;
		} else if (*vup->up.magic_p == NMG_EDGEUSE_MAGIC )  {
			if( nmg_find_s_of_eu(vup->up.eu_p) != sB) continue;
		    	NMG_INDEX_SET(classlist[NMG_CLASS_AonBshared],
		    		vu->v_p );
		    	reason = "an edgeuse of vertex is on shell";
		    	status = ON_SURF;
			goto out;
		} else if( *vup->up.magic_p == NMG_SHELL_MAGIC )  {
			if( vup->up.s_p != sB ) continue;
		    	NMG_INDEX_SET(classlist[NMG_CLASS_AonBshared],
		    		vu->v_p );
		    	reason = "vertex is shell's lone vertex";
		    	status = ON_SURF;
			goto out;
		} else {
			rt_bomb("class_vu_vs_s() bad vertex UP pointer\n");
		}
	}

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("\tCan't classify vertex via topology\n");

	/* The topology doesn't tell us about the vertex being "on" the shell
	 * so now it's time to look at the geometry to determine the vertex
	 * relationship to the shell.
	 *
	 * The vertex should *not* lie ON any of the faces or
	 * edges, since the intersection algorithm would have combined the
	 * topology if that had been the case.
	 */
	/* XXX eventually, make this conditional on DEBUG_CLASSIFY */
	{
		/* Verify this assertion */
		struct vertex	*sv;
		if( (sv = nmg_find_pt_in_shell( sB, pt, tol ) ) )  {
			rt_log("vu=x%x, v=x%x, sv=x%x, pt=(%g,%g,%g)\n",
				vu, vu->v_p, sv, V3ARGS(pt) );
			rt_bomb("nmg_class_pt_s() vertex topology not shared properly\n");
		}
	}

	reason = "of nmg_class_pt_s()";
	class = nmg_class_pt_s(pt, sB, tol);
	
	if( class == NMG_CLASS_AoutB )  {
		NMG_INDEX_SET(classlist[NMG_CLASS_AoutB], vu->v_p);
		status = OUTSIDE;
	}  else if( class == NMG_CLASS_AinB )  {
		NMG_INDEX_SET(classlist[NMG_CLASS_AinB], vu->v_p);
		status = INSIDE;
	}  else if( class == NMG_CLASS_AonBshared )  {
		rt_pr_tol(tol);
		VPRINT("pt", pt);
		rt_bomb("class_vu_vs_s:  classifier found point ON, vertex topology should have been shared\n");
	}  else  {
		rt_log("class=%s\n", nmg_class_name(class) );
		VPRINT("pt", pt);
		rt_bomb("class_vu_vs_s: nmg_class_pt_s() failure\n");
	}

out:
	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		rt_log("class_vu_vs_s(vu=x%x) return %s because %s\n",
			vu, nmg_class_status(status), reason );
	}
	return(status);
}

/*
 *			C L A S S _ E U _ V S _ S
 */
static int 
class_eu_vs_s(eu, s, classlist, tol)
struct edgeuse	*eu;
struct shell	*s;
long		*classlist[4];
CONST struct rt_tol	*tol;
{
	int euv_cl, matev_cl;
	int	status;
	struct edgeuse *eup;
	point_t pt;
	pointp_t eupt, matept;
	char	*reason = "Unknown";
	int	class;

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		nmg_euprint("class_eu_vs_s\t", eu);

	NMG_CK_EDGEUSE(eu);	
	NMG_CK_SHELL(s);	
	RT_CK_TOL(tol);
	
	/* As a mandatory consistency measure, check for cached classification */
	reason = "of cached classification";
	if( NMG_INDEX_TEST(classlist[NMG_CLASS_AinB], eu->e_p) )  {
		status = INSIDE;
		goto out;
	}
	if( NMG_INDEX_TEST(classlist[NMG_CLASS_AonBshared], eu->e_p) )  {
		status = ON_SURF;
		goto out;
	}
	if( NMG_INDEX_TEST(classlist[NMG_CLASS_AonBanti], eu->e_p) )  {
		status = ON_SURF;
		goto out;
	}
	if( NMG_INDEX_TEST(classlist[NMG_CLASS_AoutB], eu->e_p) )  {
		status = OUTSIDE;
		goto out;
	}

	euv_cl = class_vu_vs_s(eu->vu_p, s, classlist, tol);
	matev_cl = class_vu_vs_s(eu->eumate_p->vu_p, s, classlist, tol);
	
	/* sanity check */
	if ((euv_cl == INSIDE && matev_cl == OUTSIDE) ||
	    (euv_cl == OUTSIDE && matev_cl == INSIDE)) {
	    	static int	num=0;
	    	char	buf[128];
	    	FILE	*fp;

	    	sprintf(buf, "class%d.pl", num++ );
	    	if( (fp = fopen(buf, "w")) == NULL ) rt_bomb(buf);
	    	nmg_pl_s( fp, s );
		/* A yellow line for the angry edge */
		pl_color(fp, 255, 255, 0);
		pdv_3line(fp, eu->vu_p->v_p->vg_p->coord,
			eu->eumate_p->vu_p->v_p->vg_p->coord );
		fclose(fp);

	    	nmg_pr_class_status("eu vu", euv_cl);
	    	nmg_pr_class_status("eumate vu", matev_cl);
	    	if( rt_g.debug || rt_g.NMG_debug )  {
		    	/* Do them over, so we can watch */
	    		rt_log("Edge not cut, doing it over\n");
	    		NMG_INDEX_CLEAR( classlist[NMG_CLASS_AinB], eu->vu_p);
	    		NMG_INDEX_CLEAR( classlist[NMG_CLASS_AoutB], eu->vu_p);
	    		NMG_INDEX_CLEAR( classlist[NMG_CLASS_AinB], eu->eumate_p->vu_p);
	    		NMG_INDEX_CLEAR( classlist[NMG_CLASS_AoutB], eu->eumate_p->vu_p);
/**	    		rt_g.debug |= DEBUG_MATH; **/
			rt_g.NMG_debug |= DEBUG_CLASSIFY;
			(void)class_vu_vs_s(eu->vu_p, s, classlist, tol);
			(void)class_vu_vs_s(eu->eumate_p->vu_p, s, classlist, tol);
		    	nmg_euprint("didn't this edge get cut?", eu);
		    	nmg_pr_eu(eu, "  ");
	    	}

		rt_log("wrote %s\n", buf);
		rt_bomb("class_eu_vs_s:  edge didn't get cut\n");
	}

	if (euv_cl == ON_SURF && matev_cl == ON_SURF) {
		/* check for radial uses of this edge by the shell */
		eup = eu->radial_p->eumate_p;
		do {
			NMG_CK_EDGEUSE(eup);
			if (nmg_find_s_of_eu(eup) == s) {
				NMG_INDEX_SET(classlist[NMG_CLASS_AonBshared],
					eu->e_p );
				reason = "a radial edgeuse is on shell";
				status = ON_SURF;
				goto out;
			}
			eup = eup->radial_p->eumate_p;
		} while (eup != eu->radial_p->eumate_p);

		/* although the two endpoints are "on" the shell,
		 * the edge would appear to be either "inside" or "outside",
		 * since there are no uses of this edge in the shell.
		 *
		 * So we classify the midpoint of the line WRT the shell.
		 *
		 *  Consider AE as being "eu", with M as the geometric midpoint.
		 *  Consider AD as being a side view of a perpendicular plane
		 *  in the other shell, which AE _almost_ lies in.
		 *  A problem occurs when the angle DAE is very small.
		 *  Point A is ON because of shared topology.
		 *  Point E is marked ON because it is within tolerance of
		 *  the face, even though there is no corresponding vertex.
		 *  Naturally, point M is also going to be found to be ON
		 *  because it is also within tolerance.
		 *
		 *         D
		 *        /|
		 *       / |
		 *      B  |
		 *     /   |
		 *    /    |
		 *   A--M--E
		 *   
		 *  This would seem to be an intersector problem.
		 */
		eupt = eu->vu_p->v_p->vg_p->coord;
		matept = eu->eumate_p->vu_p->v_p->vg_p->coord;
		VADD2SCALE(pt, eupt, matept, 0.5);

		if (rt_g.NMG_debug & DEBUG_CLASSIFY)
			VPRINT("class_eu_vs_s: midpoint of edge", pt);

		class = nmg_class_pt_s(pt, s, tol);
		reason = "midpoint classification (both verts ON)";
		if( class == NMG_CLASS_AoutB )  {
			NMG_INDEX_SET(classlist[NMG_CLASS_AoutB], eu->e_p);
			status = OUTSIDE;
		}  else if( class == NMG_CLASS_AinB )  {
			NMG_INDEX_SET(classlist[NMG_CLASS_AinB], eu->e_p);
			status = INSIDE;
		} else if( class == NMG_CLASS_AonBshared )  {
			FILE	*fp;
#if 0
			/* XXX bug hunt helper */
			rt_g.NMG_debug |= DEBUG_FINDEU;
		/* 	rt_g.NMG_debug |= DEBUG_MESH;	 */
			rt_g.NMG_debug |= DEBUG_BASIC;
			eup = nmg_findeu( eu->vu_p->v_p, eu->eumate_p->vu_p->v_p, s, eu, 0 );
			if(!eup) rt_log("Unable to find it\n");
			nmg_model_fuse( nmg_find_model((long*)eu), tol );
#endif
			nmg_pr_fu_around_eu( eu, tol );
			VPRINT("class_eu_vs_s: midpoint of edge", pt);
			if( (fp = fopen("shell1.pl", "w")) )  {
				nmg_pl_s(fp, s);
				fclose(fp);
				rt_log("wrote shell1.pl\n");
			}
			if( (fp = fopen("shell2.pl", "w")) )  {
				nmg_pl_shell(fp, eu->up.lu_p->up.fu_p->s_p, 1 );
				fclose(fp);
				rt_log("wrote shell2.pl\n");
			}
#if 0
			/* Another bug hunt helper -- re-run face/face isect */
			rt_g.NMG_debug |= DEBUG_POLYSECT;
			rt_g.NMG_debug |= DEBUG_PLOTEM;
			rt_g.NMG_debug |= DEBUG_BASIC;
			rt_log("class_eu_vs_s:  classifier found edge midpoint ON, edge topology should have been shared\n\n################ re-run face/face isect ############\n\n");
		{
			struct faceuse	*fu1 = eu->up.lu_p->up.fu_p;
			struct faceuse	*fu2;

			NMG_CK_FACEUSE(fu1);
			for( RT_LIST_FOR( fu2, faceuse, &s->fu_hd ) )  {
				NMG_CK_FACEUSE(fu2);
				if( fu2->orientation != OT_SAME )  continue;
				nmg_isect_two_generic_faces( fu1, fu2, tol );
			}
			eup = nmg_findeu( eu->vu_p->v_p, eu->eumate_p->vu_p->v_p, s, eu, 0 );
			if(!eup) rt_log("Unable to find it\n");
		}
#endif
			rt_bomb("class_eu_vs_s:  classifier found edge midpoint ON, edge topology should have been shared\n");
		}  else  {
			rt_log("class=%s\n", nmg_class_name(class) );
			nmg_euprint("Why wasn't this edge in or out?", eu);
			rt_bomb("class_eu_vs_s: nmg_class_pt_s() midpoint failure\n");
		}
		goto out;
	}

	if (euv_cl == OUTSIDE || matev_cl == OUTSIDE) {
		NMG_INDEX_SET(classlist[NMG_CLASS_AoutB], eu->e_p);
		reason = "at least one vert OUT";
		status = OUTSIDE;
		goto out;
	}
	if( euv_cl == INSIDE && matev_cl == INSIDE )  {
		NMG_INDEX_SET(classlist[NMG_CLASS_AinB], eu->e_p);
		reason = "both verts IN";
		status = INSIDE;
		goto out;
	}
	if( (euv_cl == INSIDE && matev_cl == ON_SURF) ||
	    (euv_cl == ON_SURF && matev_cl == INSIDE) )  {
		NMG_INDEX_SET(classlist[NMG_CLASS_AinB], eu->e_p);
		reason = "one vert IN, one ON";
		status = INSIDE;
		goto out;
	}
	rt_log("eu's vert is %s, mate's vert is %s\n",
		nmg_class_status(euv_cl), nmg_class_status(matev_cl) );
	rt_bomb("class_eu_vs_s() inconsistent edgeuse\n");
out:
	if (rt_g.NMG_debug & DEBUG_GRAPHCL)
		nmg_show_broken_classifier_stuff((long *)eu, classlist, nmg_class_nothing_broken, 0, (char *)NULL);
	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		rt_log("class_eu_vs_s(eu=x%x) return %s because %s\n",
			eu, nmg_class_status(status), reason );
	}
	return(status);
}

/* XXX move to nmg_info.c */
/*
 *			N M G _ 2 L U _ I D E N T I C A L
 *
 *  Given two edgeuses in different faces that share a common edge,
 *  determine if they are from identical loops or not.
 *
 *  Note that two identical loops in an anti-shared pair of faces
 *  (faces with opposite orientations) will also have opposite orientations.
 *
 *  Returns -
 *	0	Loops not identical
 *	1	Loops identical, faces are ON-shared
 *	2	Loops identical, faces are ON-anti-shared
 *	3	Loops identical, at least one is a wire loop.
 */
int
nmg_2lu_identical( eu1, eu2 )
CONST struct edgeuse	*eu1;
CONST struct edgeuse	*eu2;
{
	CONST struct loopuse	*lu1;
	CONST struct loopuse	*lu2;
	CONST struct edgeuse	*eu1_first;
	CONST struct faceuse	*fu1;
	CONST struct faceuse	*fu2;
	int			ret;

	NMG_CK_EDGEUSE(eu1);
	NMG_CK_EDGEUSE(eu2);

	if( eu1->e_p != eu2->e_p )  rt_bomb("nmg_2lu_identical() differing edges?\n");

	/* get the starting vertex of each edgeuse to be the same. */
	if (eu2->vu_p->v_p != eu1->vu_p->v_p) {
		eu2 = eu2->eumate_p;
		if (eu2->vu_p->v_p != eu1->vu_p->v_p)
			rt_bomb("nmg_2lu_identical() radial edgeuse doesn't share verticies\n");
	}

	lu1 = eu1->up.lu_p;
	lu2 = eu2->up.lu_p;

	NMG_CK_LOOPUSE(lu1);
	NMG_CK_LOOPUSE(lu2);

    	/* march around the two loops to see if they 
    	 * are the same all the way around.
    	 */
	eu1_first = eu1;
	do {
		if( eu1->vu_p->v_p != eu2->vu_p->v_p )  {
			ret = 0;
			goto out;
		}
		eu1 = RT_LIST_PNEXT_CIRC(edgeuse, &eu1->l);
		eu2 = RT_LIST_PNEXT_CIRC(edgeuse, &eu2->l);
	} while ( eu1 != eu1_first );

	if( *lu1->up.magic_p != NMG_FACEUSE_MAGIC ||
	    *lu2->up.magic_p != NMG_FACEUSE_MAGIC )  {
		ret = 3;	/* one is a wire loop */
	    	goto out;
	    }

	fu1 = lu1->up.fu_p;
	fu2 = lu2->up.fu_p;

	if( fu1->f_p->g.plane_p != fu2->f_p->g.plane_p )  {
		vect_t fu1_norm,fu2_norm;

		rt_log("nmg_2lu_identical() loops lu1=x%x lu2=x%x are shared, face geometry is not? fg1=x%x, fg2=x%x\n",
			lu1, lu2, fu1->f_p->g.plane_p, fu2->f_p->g.plane_p);
		rt_log("---- fu1, f=x%x, flip=%d\n", fu1->f_p, fu1->f_p->flip);
		nmg_pr_fg(fu1->f_p->g.magic_p, 0);
		nmg_pr_fu_briefly(fu1, 0);

		rt_log("---- fu2, f=x%x, flip=%d\n", fu2->f_p, fu2->f_p->flip);
		nmg_pr_fg(fu2->f_p->g.magic_p, 0);
		nmg_pr_fu_briefly(fu2, 0);

		/* Drop back to using a geometric calculation */
		if( fu1->orientation != fu2->orientation )
			NMG_GET_FU_NORMAL( fu2_norm, fu2->fumate_p )
		else
			NMG_GET_FU_NORMAL( fu2_norm, fu2 )

		NMG_GET_FU_NORMAL( fu1_norm, fu1 );
		if( VDOT( fu1_norm, fu2_norm ) < 0.0 )
			ret = 2;	/* ON anti-shared */
		else
			ret = 1;	/* ON shared */
		goto out;
	}

	if( rt_g.NMG_debug & DEBUG_BASIC )  {
		rt_log("---- fu1, f=x%x, flip=%d\n", fu1->f_p, fu1->f_p->flip);
		nmg_pr_fu_briefly(fu1, 0);
		rt_log("---- fu2, f=x%x, flip=%d\n", fu2->f_p, fu2->f_p->flip);
		nmg_pr_fu_briefly(fu2, 0);
	}

	/*
	 *  The two loops are identical, compare the two faces.
	 *  Only raw face orientations count here.
	 *  Loopuse and faceuse orientations do not matter.
	 */
	if( fu1->f_p->flip != fu2->f_p->flip )
		ret = 2;		/* ON anti-shared */
	else
		ret = 1;		/* ON shared */
out:
	if( rt_g.NMG_debug & DEBUG_BASIC )  {
		rt_log("nmg_2lu_identical(eu1=x%x, eu2=x%x) ret=%d\n",
			eu1, eu2, ret);
	}
	return ret;
}

/*
 *			N M G _ R E C L A S S I F Y _ L U _ E U
 *
 *  Make all the edges and vertices of a loop carry the same classification
 *  as the loop.
 *  There is no intrinsic way to tell if an edge is "shared" or
 *  "antishared", except by reference to it's loopuse, but the heritage
 *  of the edgeuse makes a difference to the boolean evaluator.
 *
 *  "newclass" should only be AonBshared or AonBanti.
 */
void
nmg_reclassify_lu_eu( lu, classlist, newclass )
struct loopuse	*lu;
long		*classlist[4];
int		newclass;
{
	struct vertexuse	*vu;
	struct edgeuse		*eu;
	struct vertex		*v;

	NMG_CK_LOOPUSE(lu);

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_reclassify_lu_eu(lu=x%x, classlist=x%x, newclass=%s)\n",
			lu, classlist, nmg_class_name(newclass) );
	}

	if( newclass != NMG_CLASS_AonBshared && newclass != NMG_CLASS_AonBanti )
		rt_bomb("nmg_reclassify_lu_eu() bad newclass\n");

	if (RT_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_VERTEXUSE_MAGIC)  {
		vu = RT_LIST_FIRST(vertexuse, &lu->down_hd);
		NMG_CK_VERTEXUSE(vu);
		v = vu->v_p;
		NMG_CK_VERTEX(v);

		if( NMG_INDEX_TEST(classlist[NMG_CLASS_AinB], v) ||
		    NMG_INDEX_TEST(classlist[NMG_CLASS_AoutB], v) )
			rt_bomb("nmg_reclassify_lu_eu() changing in/out vert of lone vu loop to ON?\n");

		/* Clear out other classifications */
		NMG_INDEX_CLEAR(classlist[NMG_CLASS_AonBshared], v);
		NMG_INDEX_CLEAR(classlist[NMG_CLASS_AonBanti], v);
		/* Establish new classification */
		NMG_INDEX_SET(classlist[newclass], v);
		return;
	}

	for( RT_LIST_FOR( eu, edgeuse, &lu->down_hd ) )  {
		struct edge	*e;
		NMG_CK_EDGEUSE(eu);
		e = eu->e_p;
		NMG_CK_EDGE(e);

		if( NMG_INDEX_TEST(classlist[NMG_CLASS_AinB], e) ||
		    NMG_INDEX_TEST(classlist[NMG_CLASS_AoutB], e) )
			rt_bomb("nmg_reclassify_lu_eu() changing in/out edge to ON?\n");

		/* Clear out other edge classifications */
		NMG_INDEX_CLEAR(classlist[NMG_CLASS_AonBshared], e);
		NMG_INDEX_CLEAR(classlist[NMG_CLASS_AonBanti], e);
		/* Establish new edge classification */
		NMG_INDEX_SET(classlist[newclass], e);

		/* Same thing for the vertices.  Only need to do start vert here. */
		vu = eu->vu_p;
		NMG_CK_VERTEXUSE(vu);
		v = vu->v_p;
		NMG_CK_VERTEX(v);

		if( NMG_INDEX_TEST(classlist[NMG_CLASS_AinB], v) ||
		    NMG_INDEX_TEST(classlist[NMG_CLASS_AoutB], v) )
			rt_bomb("nmg_reclassify_lu_eu() changing in/out vert to ON?\n");

		/* Clear out other classifications */
		NMG_INDEX_CLEAR(classlist[NMG_CLASS_AonBshared], v);
		NMG_INDEX_CLEAR(classlist[NMG_CLASS_AonBanti], v);
		/* Establish new classification */
		NMG_INDEX_SET(classlist[newclass], v);
	}
}

/*
 *			C L A S S _ L U _ V S _ S
 *
 *  Called by -
 *	class_fu_vs_s
 */
static int 
class_lu_vs_s(lu, s, classlist, tol)
struct loopuse		*lu;
struct shell		*s;
long			*classlist[4];
CONST struct rt_tol	*tol;
{
	int class;
	unsigned int	in, outside, on;
	struct edgeuse *eu, *p;
	struct loopuse *q_lu;
	struct vertexuse *vu;
	long		magic1;
	char		*reason = "Unknown";
	int		seen_error = 0;
	int		status = 0;

	NMG_CK_LOOPUSE(lu);
	NMG_CK_SHELL(s);
	RT_CK_TOL(tol);

	/* check to see if loop is already in one of the lists */
	if( NMG_INDEX_TEST(classlist[NMG_CLASS_AinB], lu->l_p) )  {
		reason = "of classlist";
		status = INSIDE;
		goto out;
	}

	if( NMG_INDEX_TEST(classlist[NMG_CLASS_AonBshared], lu->l_p) ||
	    NMG_INDEX_TEST(classlist[NMG_CLASS_AonBanti], lu->l_p) )  {
		reason = "of classlist";
		status = ON_SURF;
		goto out;
	}

	if( NMG_INDEX_TEST(classlist[NMG_CLASS_AoutB], lu->l_p) )  {
		reason = "of classlist";
		status = OUTSIDE;
		goto out;
	}

	magic1 = RT_LIST_FIRST_MAGIC( &lu->down_hd );
	if (magic1 == NMG_VERTEXUSE_MAGIC) {
		/* Loop of a single vertex */
		reason = "of vertex classification";
		vu = RT_LIST_PNEXT( vertexuse, &lu->down_hd );
		NMG_CK_VERTEXUSE(vu);
		class = class_vu_vs_s(vu, s, classlist, tol);
		switch (class) {
		case INSIDE:
			NMG_INDEX_SET(classlist[NMG_CLASS_AinB], lu->l_p);
			break;
		case OUTSIDE:
			NMG_INDEX_SET(classlist[NMG_CLASS_AoutB], lu->l_p);
			 break;
		case ON_SURF:
			NMG_INDEX_SET(classlist[NMG_CLASS_AonBshared], lu->l_p);
			break;
		default:
			rt_bomb("class_lu_vs_s: bad vertexloop classification\n");
		}
		status = class;
		goto out;
	} else if (magic1 != NMG_EDGEUSE_MAGIC) {
		rt_bomb("class_lu_vs_s: bad child of loopuse\n");
	}

	/* loop is collection of edgeuses */
retry:
	in = outside = on = 0;
	for (RT_LIST_FOR(eu, edgeuse, &lu->down_hd)) {
		/* Classify each edgeuse */
		class = class_eu_vs_s(eu, s, classlist, tol);
		switch (class) {
		case INSIDE	: ++in; 
				break;
		case OUTSIDE	: ++outside;
				break;
		case ON_SURF	: ++on;
				break;
		default		: rt_bomb("class_lu_vs_s: bad class for edgeuse\n");
		}
	}

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("class_lu_vs_s: Loopuse edges in:%d on:%d out:%d\n", in, on, outside);

	if (in > 0 && outside > 0) {
		FILE *fp;
		rt_log("Loopuse edges in:%d on:%d out:%d, turning on DEBUG_CLASSIFY\n", in, on, outside);
		if( rt_g.NMG_debug & DEBUG_CLASSIFY )  {
			char		buf[128];
			static int	num;
			long		*b;
			struct model	*m;

			m = nmg_find_model(lu->up.magic_p);
			b = (long *)rt_calloc(m->maxindex, sizeof(long), "nmg_pl_lu flag[]");

			for(RT_LIST_FOR(eu, edgeuse, &lu->down_hd)) {
				if (NMG_INDEX_TEST(classlist[NMG_CLASS_AinB], eu->e_p))
					nmg_euprint("In:  edgeuse", eu);
				else if (NMG_INDEX_TEST(classlist[NMG_CLASS_AoutB], eu->e_p))
					nmg_euprint("Out: edgeuse", eu);
				else if (NMG_INDEX_TEST(classlist[NMG_CLASS_AonBshared], eu->e_p))
					nmg_euprint("OnShare:  edgeuse", eu);
				else if (NMG_INDEX_TEST(classlist[NMG_CLASS_AonBanti], eu->e_p))
					nmg_euprint("OnAnti:  edgeuse", eu);
				else
					nmg_euprint("BAD: edgeuse", eu);
			}

			sprintf(buf, "badloop%d.pl", num++);
			if ((fp=fopen(buf, "w")) != NULL) {
				nmg_pl_lu(fp, lu, b, 255, 255, 255);
				nmg_pl_s(fp, s);
				fclose(fp);
				rt_log("wrote %s\n", buf);
			}
			nmg_pr_lu(lu, "");
			nmg_stash_model_to_file( "class.g", nmg_find_model((long *)lu), "class_ls_vs_s: loop transits plane of shell/face?");
			rt_free( (char *)b, "nmg_pl_lu flag[]" );
		}
		rt_g.NMG_debug |= DEBUG_CLASSIFY;
		if(seen_error)
			rt_bomb("class_lu_vs_s: loop transits plane of shell/face?\n");
		seen_error = 1;
		goto retry;
	}
	if (outside > 0) {
		NMG_INDEX_SET(classlist[NMG_CLASS_AoutB], lu->l_p);
		reason = "edgeuses were OUT and ON";
		status = OUTSIDE;
		goto out;
	} else if (in > 0) {
		NMG_INDEX_SET(classlist[NMG_CLASS_AinB], lu->l_p);
		reason = "edgeuses were IN and ON";
		status = INSIDE;
		goto out;
	} else if (on == 0)
		rt_bomb("class_lu_vs_s: alright, who's the wiseguy that stole my edgeuses?\n");


	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("\tAll edgeuses of loop are ON\n");

	/* since all of the edgeuses of this loop are "on" the other shell,
	 * we need to see if this loop is "on" the other shell
	 *
	 * if we're a wire edge loop, simply having all edges "on" is
	 *	sufficient.
	 *
	 * foreach radial edgeuse
	 * 	if edgeuse vertex is same and edgeuse parent shell is the one
	 *	 	desired, then....
	 *
	 *		p = edgeuse, q = radial edgeuse
	 *		while p's vertex equals q's vertex and we
	 *			haven't come full circle
	 *			move p and q forward
	 *		if we made it full circle, loop is on
	 */

	if (*lu->up.magic_p == NMG_SHELL_MAGIC) {
		NMG_INDEX_SET(classlist[NMG_CLASS_AonBshared], lu->l_p);
		reason = "loop is a wire loop in the shell";
		status = ON_SURF;
		goto out;
	}

	NMG_CK_FACEUSE(lu->up.fu_p);

	eu = RT_LIST_FIRST(edgeuse, &lu->down_hd);
	for(
	    eu = eu->radial_p->eumate_p;
	    eu != RT_LIST_FIRST(edgeuse, &lu->down_hd);
	    eu = eu->radial_p->eumate_p
	)  {
		int	code;

		/* if the radial edge is a part of a loop which is part of
		 * a face, then it's one that we might be "on"
		 */
		if( *eu->up.magic_p != NMG_LOOPUSE_MAGIC ) continue;
	    	q_lu = eu->up.lu_p;
		if( *q_lu->up.magic_p != NMG_FACEUSE_MAGIC ) continue;

		if( q_lu == lu )  continue;

		/* Only consider faces from shell 's' */
		if( q_lu->up.fu_p->s_p != s )  continue;

		code = nmg_2lu_identical( eu,
			RT_LIST_FIRST(edgeuse, &lu->down_hd) );
	    	switch(code)  {
	    	default:
	    	case 0:
	    		/* Not identical */
	    		break;
	    	case 1:
	    		/* ON-shared */
	    		if( lu->orientation == OT_OPPOSITE )  {
			    	NMG_INDEX_SET(classlist[NMG_CLASS_AonBanti],
			    		lu->l_p );
				if (rt_g.NMG_debug & DEBUG_CLASSIFY)
					rt_log("Loop is on-antishared (lu orient is OT_OPPOSITE)\n");
				nmg_reclassify_lu_eu( lu, classlist, NMG_CLASS_AonBanti );
	    		}  else {
			    	NMG_INDEX_SET(classlist[NMG_CLASS_AonBshared],
			    		lu->l_p );
				if (rt_g.NMG_debug & DEBUG_CLASSIFY)
					rt_log("Loop is on-shared\n");
	    			/* no need to reclassify, edges were previously marked as AonBshared */
	    		}
			reason = "edges identical with radial face, normals colinear";
	    		status = ON_SURF;
	    		goto out;
	    	case 2:
	    		/* ON-antishared */
	    		if( lu->orientation == OT_OPPOSITE )  {
				NMG_INDEX_SET(classlist[NMG_CLASS_AonBshared],
					lu->l_p );
				if (rt_g.NMG_debug & DEBUG_CLASSIFY)
					rt_log("Loop is on-shared (lu orient is OT_OPPOSITE)\n");
	    			/* no need to reclassify, edges were previously marked as AonBshared */
	    		}  else  {
				NMG_INDEX_SET(classlist[NMG_CLASS_AonBanti],
					lu->l_p );
				if (rt_g.NMG_debug & DEBUG_CLASSIFY)
					rt_log("Loop is on-antishared\n");
				nmg_reclassify_lu_eu( lu, classlist, NMG_CLASS_AonBanti );
	    		}
			reason = "edges identical with radial face, normals opposite";
			status = ON_SURF;
			goto out;
	    	case 3:
	    		rt_bomb("class_lu_vs_s() unexpected wire ON\n");
		}
	}



	/* OK, the edgeuses are all "on", but the loop isn't.  Time to
	 * decide if the loop is "out" or "in".  To do this, we look for
	 * an edgeuse radial to one of the edgeuses in the loop which is
	 * a part of a face in the other shell.  If/when we find such a
	 * radial edge, we check the direction (in/out) of the faceuse normal.
	 * if the faceuse normal is pointing out of the shell, we are outside.
	 * if the faceuse normal is pointing into the shell, we are inside.
	 */

	for (RT_LIST_FOR(eu, edgeuse, &lu->down_hd)) {

		p = eu->radial_p;
		do {
			if (*p->up.magic_p == NMG_LOOPUSE_MAGIC &&
			    *p->up.lu_p->up.magic_p == NMG_FACEUSE_MAGIC &&
			    p->up.lu_p->up.fu_p->s_p == s) {

			    	if (p->up.lu_p->up.fu_p->orientation == OT_OPPOSITE) {
			    		NMG_INDEX_SET(classlist[NMG_CLASS_AinB],
			    			lu->l_p );
					if (rt_g.NMG_debug & DEBUG_CLASSIFY)
						rt_log("Loop is INSIDE\n");
			    		reason = "radial faceuse is OT_OPPOSITE";
			    		status = INSIDE;
			    		goto out;
			    	} else if (p->up.lu_p->up.fu_p->orientation == OT_SAME) {
			    		NMG_INDEX_SET(classlist[NMG_CLASS_AoutB],
			    			lu->l_p );
					if (rt_g.NMG_debug & DEBUG_CLASSIFY)
						rt_log("Loop is OUTSIDE\n");
			    		reason = "radial faceuse is OT_SAME";
			    		status = OUTSIDE;
					goto out;
			    	} else {
			    		rt_bomb("class_lu_vs_s() bad fu orientation\n");
			    	}
			}
			p = p->eumate_p->radial_p;
		} while (p != eu->eumate_p);

	}

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("Loop is OUTSIDE 'cause it isn't anything else\n");

	/* Since we didn't find any radial faces to classify ourselves against
	 * and we already know that the edges are all "on" that must mean that
	 * the loopuse is "on" a wireframe portion of the shell.
	 */
	NMG_INDEX_SET( classlist[NMG_CLASS_AoutB], lu->l_p );
	reason = "loopuse is ON a wire loop in the shell";
	status = OUTSIDE;
out:
	if (rt_g.NMG_debug & DEBUG_CLASSIFY) {
		rt_log("class_lu_vs_s(lu=x%x) return %s because %s\n",
			lu, nmg_class_status(status), reason );
	}
	return status;
}

/*
 *			C L A S S _ F U _ V S _ S
 *
 *  Called by -
 *	nmg_class_shells()
 */
static void 
class_fu_vs_s(fu, s, classlist, tol)
struct faceuse		*fu;
struct shell		*s;
long			*classlist[4];
CONST struct rt_tol	*tol;
{
	struct loopuse *lu;
	plane_t		n;
	
	NMG_CK_FACEUSE(fu);
	NMG_CK_SHELL(s);

	NMG_GET_FU_PLANE( n, fu );

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
        	PLPRINT("\nclass_fu_vs_s plane equation:", n);

	for (RT_LIST_FOR(lu, loopuse, &fu->lu_hd))
		(void)class_lu_vs_s(lu, s, classlist, tol);

	if (rt_g.NMG_debug & DEBUG_CLASSIFY)
		rt_log("class_fu_vs_s() END\n");
}

/*
 *			N M G _ C L A S S _ S H E L L S
 *
 *	Classify one shell WRT the other shell
 *
 *  Implicit return -
 *	Each element's classification will be represented by a
 *	SET entry in the appropriate classlist[] array.
 *
 *  Called by -
 *	nmg_bool.c
 */
void
nmg_class_shells(sA, sB, classlist, tol)
struct shell	*sA;
struct shell	*sB;
long		*classlist[4];
CONST struct rt_tol	*tol;
{
	struct faceuse *fu;
	struct loopuse *lu;
	struct edgeuse *eu;

	NMG_CK_SHELL(sA);
	NMG_CK_SHELL(sB);
	RT_CK_TOL(tol);

	if (rt_g.NMG_debug & DEBUG_CLASSIFY &&
	    RT_LIST_NON_EMPTY(&sA->fu_hd))
		rt_log("nmg_class_shells - doing faces\n");

	fu = RT_LIST_FIRST(faceuse, &sA->fu_hd);
	while (RT_LIST_NOT_HEAD(fu, &sA->fu_hd)) {

		class_fu_vs_s(fu, sB, classlist, tol);

		if (RT_LIST_PNEXT(faceuse, fu) == fu->fumate_p)
			fu = RT_LIST_PNEXT_PNEXT(faceuse, fu);
		else
			fu = RT_LIST_PNEXT(faceuse, fu);
	}
	
	if (rt_g.NMG_debug & DEBUG_CLASSIFY &&
	    RT_LIST_NON_EMPTY(&sA->lu_hd))
		rt_log("nmg_class_shells - doing loops\n");

	lu = RT_LIST_FIRST(loopuse, &sA->lu_hd);
	while (RT_LIST_NOT_HEAD(lu, &sA->lu_hd)) {

		(void)class_lu_vs_s(lu, sB, classlist, tol);

		if (RT_LIST_PNEXT(loopuse, lu) == lu->lumate_p)
			lu = RT_LIST_PNEXT_PNEXT(loopuse, lu);
		else
			lu = RT_LIST_PNEXT(loopuse, lu);
	}

	if (rt_g.NMG_debug & DEBUG_CLASSIFY &&
	    RT_LIST_NON_EMPTY(&sA->eu_hd))
		rt_log("nmg_class_shells - doing edges\n");

	eu = RT_LIST_FIRST(edgeuse, &sA->eu_hd);
	while (RT_LIST_NOT_HEAD(eu, &sA->eu_hd)) {

		(void)class_eu_vs_s(eu, sB, classlist, tol);

		if (RT_LIST_PNEXT(edgeuse, eu) == eu->eumate_p)
			eu = RT_LIST_PNEXT_PNEXT(edgeuse, eu);
		else
			eu = RT_LIST_PNEXT(edgeuse, eu);
	}

	if (sA->vu_p) {
		if (rt_g.NMG_debug)
			rt_log("nmg_class_shells - doing vertex\n");
		(void)class_vu_vs_s(sA->vu_p, sB, classlist, tol);
	}
}

/*	N M G _ C L A S S I F Y _ P T _ L O O P
 *
 *	A generally available interface to nmg_class_pt_l
 *
 *	returns the classification from nmg_class_pt_l
 *	or a (-1) on error
 *
 *  Called by -
 *	nmg_extrude.c / nmg_fix_overlapping_loops()
 *
 *  XXX DANGER:  Calls nmg_class_pt_l(), which does not work well.
 */
int
nmg_classify_pt_loop( pt , lu , tol )
CONST point_t pt;
CONST struct loopuse *lu;
CONST struct rt_tol *tol;
{
	struct neighbor	closest;
	struct faceuse *fu;
	plane_t n;
	fastf_t dist;

	NMG_CK_LOOPUSE( lu );
	RT_CK_TOL( tol );

rt_log("DANGER: nmg_classify_pt_loop() is calling nmg_class_pt_l(), which does not work well\n");
	if( *lu->up.magic_p != NMG_FACEUSE_MAGIC )
	{
		rt_log( "nmg_classify_pt_loop: lu not part of a faceuse!!\n" );
		return( -1 );
	}

	fu = lu->up.fu_p;

	/* Validate distance from point to plane */
	NMG_GET_FU_PLANE( n, fu );
	if( (dist=fabs(DIST_PT_PLANE( pt, n ))) > tol->dist )  {
		rt_log("nmg_classify_pt_l() ERROR, point (%g,%g,%g) not on face, dist=%g\n",
			V3ARGS(pt), dist );
		return( -1 );
	}


	/* find the closest approach in this face to the projected point */
	closest.dist = MAX_FASTF;
	closest.p.eu = (struct edgeuse *)NULL;
	closest.class = NMG_CLASS_AoutB;	/* default return */

	nmg_class_pt_l( &closest , pt , lu , tol );

	return( closest.class );
}

/*	N M G _ C L A S S I F Y _ L U _ L U
 *
 *	Generally available classifier for
 *	determining if one loop is within another
 *
 *	returns classification based on nmg_class_pt_l results
 *
 *  Called by -
 *	nmg_misc.c / nmg_split_loops_handler()
 *
 */
int
nmg_classify_lu_lu( lu1 , lu2 , tol )
CONST struct loopuse *lu1,*lu2;
CONST struct rt_tol *tol;
{
	struct faceuse *fu1,*fu2;
	struct edgeuse *eu;
	int same_loop;

	NMG_CK_LOOPUSE( lu1 );
	NMG_CK_LOOPUSE( lu2 );
	RT_CK_TOL( tol );

	if( rt_g.NMG_debug & DEBUG_CLASSIFY )
		rt_log( "nmg_classify_lu_lu( lu1=x%x , lu2=x%x )\n", lu1, lu2 );

	if( lu1 == lu2 || lu1 == lu2->lumate_p )
		return( NMG_CLASS_AonBshared );

	if( *lu1->up.magic_p != NMG_FACEUSE_MAGIC )
	{
		rt_log( "nmg_classify_lu_lu: lu1 not part of a faceuse\n" );
		return( -1 );
	}

	if( *lu2->up.magic_p != NMG_FACEUSE_MAGIC )
	{
		rt_log( "nmg_classify_lu_lu: lu2 not part of a faceuse\n" );
		return( -1 );
	}

	fu1 = lu1->up.fu_p;
	NMG_CK_FACEUSE( fu1 );
	fu2 = lu2->up.fu_p;
	NMG_CK_FACEUSE( fu2 );

	if( fu1->f_p != fu2->f_p )
	{
		rt_log( "nmg_classify_lu_lu: loops are not in same face\n" );
		return( -1 );
	}

	/* do simple check for two loops of the same vertices */
	if( RT_LIST_FIRST_MAGIC( &lu1->down_hd ) == NMG_EDGEUSE_MAGIC &&
	    RT_LIST_FIRST_MAGIC( &lu2->down_hd ) == NMG_EDGEUSE_MAGIC )
	{
		struct edgeuse *eu1_start,*eu2_start;
		struct edgeuse *eu1,*eu2;

		same_loop = 1;
		eu1_start = RT_LIST_FIRST( edgeuse , &lu1->down_hd );
		NMG_CK_EDGEUSE( eu1_start );
		eu2_start = RT_LIST_FIRST( edgeuse , &lu2->down_hd );
		NMG_CK_EDGEUSE( eu2_start );
		while( RT_LIST_NOT_HEAD( eu2_start , &lu2->down_hd ) &&
			eu2_start->vu_p->v_p != eu1_start->vu_p->v_p )
			{
				NMG_CK_EDGEUSE( eu2_start );
				eu2_start = RT_LIST_PNEXT( edgeuse , &eu2_start->l );
			}

		if( RT_LIST_NOT_HEAD( eu2_start , &lu2->down_hd ) &&
			eu1_start->vu_p->v_p == eu2_start->vu_p->v_p )
		{
			/* check the rest of the loop */
			eu1 = eu1_start;
			eu2 = eu2_start;
			while( RT_LIST_NOT_HEAD( eu1 , &lu1->down_hd ) )
			{
				if( eu1->vu_p->v_p != eu2->vu_p->v_p )
				{
					same_loop = 0;
					break;
				}
				eu1 = RT_LIST_PNEXT( edgeuse , &eu1->l );
				eu2 = RT_LIST_PNEXT_CIRC( edgeuse , &eu2->l );
			}

			if( same_loop )
				return( NMG_CLASS_AonBshared );

			/* maybe the other way round */
			same_loop = 1;
			eu1 = eu1_start;
			eu2 = eu2_start;
			while( RT_LIST_NOT_HEAD( eu1 , &lu1->down_hd ) )
			{
				if( eu1->vu_p->v_p != eu2->vu_p->v_p )
				{
					same_loop = 0;
					break;
				}
				eu1 = RT_LIST_PNEXT( edgeuse , &eu1->l );
				eu2 = RT_LIST_PPREV_CIRC( edgeuse , &eu2->l );
			}

			if( same_loop )
				return( NMG_CLASS_AonBshared );
		}
	}
	else if( RT_LIST_FIRST_MAGIC( &lu1->down_hd ) == NMG_VERTEXUSE_MAGIC &&
		 RT_LIST_FIRST_MAGIC( &lu2->down_hd ) == NMG_VERTEXUSE_MAGIC )
	{
		struct vertexuse *vu1,*vu2;

		vu1 = RT_LIST_FIRST( vertexuse , &lu1->down_hd );
		vu2 = RT_LIST_FIRST( vertexuse , &lu2->down_hd );

		if( vu1->v_p == vu2->v_p )
			return( NMG_CLASS_AonBshared );
		else
			return( NMG_CLASS_AoutB );
	}

	if( RT_LIST_FIRST_MAGIC( &lu1->down_hd ) == NMG_VERTEXUSE_MAGIC )
	{
		struct vertexuse *vu;
		struct vertex_g *vg;

		vu = RT_LIST_FIRST( vertexuse , &lu1->down_hd );
		NMG_CK_VERTEXUSE( vu );
		vg = vu->v_p->vg_p;
		NMG_CK_VERTEX_G( vg );

		return( nmg_class_pt_lu_except( vg->coord, lu2, (struct edgeuse *)NULL, tol ));
	}

	for( RT_LIST_FOR( eu , edgeuse , &lu1->down_hd ) )
	{
		struct vertex_g *vg;
		int class;

		NMG_CK_EDGEUSE( eu );

		vg = eu->vu_p->v_p->vg_p;
		NMG_CK_VERTEX_G( vg );

		class = nmg_class_pt_lu_except( vg->coord, lu2, (struct edgeuse *)NULL, tol);
		if( class != NMG_CLASS_AonBshared && class != NMG_CLASS_AonBanti )
		{
			if( lu2->orientation == OT_SAME )
				return( class );
			else
			{
				if( class == NMG_CLASS_AinB )
					return( NMG_CLASS_AoutB );
				if( class == NMG_CLASS_AoutB )
					return( NMG_CLASS_AinB );
			}
		}
	}

	return( NMG_CLASS_AonBshared );
}
