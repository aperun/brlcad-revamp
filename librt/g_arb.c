/*
 *  			G _ A R B . C
 *  
 *  Function -
 *  	Intersect a ray with an Arbitrary Regular Polyhedron with
 *  	as many as 8 vertices.
 *
 *  An ARB is a convex volume bounded by 4 (pyramid), 5 (wedge), or 6 (box)
 *  planes.  This analysis depends on the properties of objects with convex
 *  hulls.  Let the ray in question be defined such that any point X on the
 *  ray may be expressed as X = P + k D.  Intersect the ray with each of the
 *  planes bounding the ARB as discussed above, and record the values of the
 *  parametric distance k along the ray.
 *
 *  With outward pointing normal vectors,
 *  note that the ray enters the half-space defined by a plane when D cdot N <
 *  0, is parallel to the plane when D cdot N = 0, and exits otherwise.  Find
 *  the entry point farthest away from the starting point bold P, i.e.  it has
 *  the largest value of k among the entry points.
 *  The ray enters the solid at this point.
 *  Similarly, find the exit point closest to point P, i.e. it has
 *  the smallest value of k among the exit points.  The ray exits the solid
 *  here.
 *
 *  This algorithm is due to Cyrus & Beck, USAF.
 *  
 *  Author -
 *	Michael John Muuss
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
static char RCSarb[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "nmg.h"
#include "db.h"
#include "./debug.h"

/* The internal (in memory) form of an ARB8 -- 8 points in space */
struct arb_internal {
	fastf_t	arbi_pt[3*8];
};

/* Optionally, one of these for each face.  (Lazy evaluation) */
struct oface {
	fastf_t	arb_UVorig[3];		/* origin of UV coord system */
	fastf_t	arb_U[3];		/* unit U vector (along B-A) */
	fastf_t	arb_V[3];		/* unit V vector (perp to N and U) */
	fastf_t	arb_Ulen;		/* length of U basis (for du) */
	fastf_t	arb_Vlen;		/* length of V basis (for dv) */
};

/* One of these for each face */
struct aface {
	fastf_t	A[3];			/* "A" point */
	plane_t	peqn;			/* Plane equation, unit normal */
};

/* One of these for each ARB, custom allocated to size */
struct arb_specific  {
	int		arb_nmfaces;	/* number of faces */
	struct oface	*arb_opt;	/* pointer to optional info */
	struct aface	arb_face[4];	/* May really be up to [6] faces */
};

/* These hold temp values for the face being prep'ed */
struct prep_arb {
	vect_t		pa_center;	/* center point */
	int		pa_faces;	/* Number of faces done so far */
	int		pa_npts[6];	/* # of points on face's plane */
	int		pa_pindex[4][6]; /* subscr in arbi_pt[] */
	int		pa_reversed[6];	/* face normal was flipped */
	struct aface	pa_face[6];	/* required face info work area */
	struct oface	pa_opt[6];	/* optional face info work area */
	/* These elements must be initialized before using */
	fastf_t		pa_tol_sq;	/* points-are-equal tol sq */
	int		pa_doopt;	/* compute pa_opt[] stuff */
	char		*pa_name;	/* string for error messages */
};

/*
 *  Layout of arb in input record.
 *  Points are listed in "clockwise" order,
 *  to make proper outward-pointing face normals.
 *  (Although the cross product wants counter-clockwise order)
 */
static struct arb_info {
	char	*ai_title;
	int	ai_sub[4];
} rt_arb_info[6] = {
	{ "1234", 3, 2, 1, 0 },		/* "bottom" face */
	{ "8765", 4, 5, 6, 7 },		/* "top" face */
	{ "1485", 4, 7, 3, 0 },
	{ "2673", 2, 6, 5, 1 },
	{ "1562", 1, 5, 4, 0 },
	{ "4378", 7, 6, 2, 3 }
};

/*
 *			R T _ A R B _ A D D _ P T
 *
 *  Add another point to a struct arb_specific, checking for unique pts.
 *  The first two points are easy.  The third one triggers most of the
 *  plane calculations, and forth and subsequent ones are merely
 *  checked for validity.
 *
 *  Returns -
 *	 0	point was accepted
 *	-1	point was rejected
 */
HIDDEN int
rt_arb_add_pt( point, title, pap, ptno )
register pointp_t point;
char		*title;
struct prep_arb	*pap;
int		ptno;	/* current point # on face */
{
	LOCAL vect_t	work;
	LOCAL vect_t	P_A;		/* new point minus A */
	FAST fastf_t	f;
	register struct aface	*afp;
	register struct oface	*ofp;

	afp = &pap->pa_face[pap->pa_faces];
	ofp = &pap->pa_opt[pap->pa_faces];

	/* The first 3 points are treated differently */
	switch( ptno )  {
	case 0:
		VMOVE( afp->A, point );
		if( pap->pa_doopt )  {
			VMOVE( ofp->arb_UVorig, point );
		}
		return(0);				/* OK */
	case 1:
		VSUB2( ofp->arb_U, point, afp->A );	/* B-A */
		f = MAGNITUDE( ofp->arb_U );
		if( NEAR_ZERO( f, SQRT_SMALL_FASTF ) )  {
			return(-1);			/* BAD */
		}
		ofp->arb_Ulen = f;
		f = 1/f;
		VSCALE( ofp->arb_U, ofp->arb_U, f );
		/* Note that arb_U is used to build N, below */
		return(0);				/* OK */
	case 2:
		VSUB2( P_A, point, afp->A );	/* C-A */
		/* Pts are given clockwise, so reverse terms of cross prod. */
		VCROSS( afp->peqn, P_A, ofp->arb_U );
		/* Check for co-linear, ie, |(B-A)x(C-A)| ~= 0 */
		f = MAGNITUDE( afp->peqn );
		if( NEAR_ZERO(f,0.005) )  {
			return(-1);			/* BAD */
		}
		f = 1/f;
		VSCALE( afp->peqn, afp->peqn, f );

		if( pap->pa_doopt )  {
			/*
			 * Get vector perp. to AB in face of plane ABC.
			 * Scale by projection of AC, make this V.
			 */
			VCROSS( work, afp->peqn, ofp->arb_U );
			VUNITIZE( work );
			f = VDOT( work, P_A );
			VSCALE( ofp->arb_V, work, f );
			f = MAGNITUDE( ofp->arb_V );
			ofp->arb_Vlen = f;
			f = 1/f;
			VSCALE( ofp->arb_V, ofp->arb_V, f );

			/* Check for new Ulen */
			VSUB2( P_A, point, ofp->arb_UVorig );
			f = VDOT( P_A, ofp->arb_U );
			if( f > ofp->arb_Ulen ) {
				ofp->arb_Ulen = f;
			} else if( f < 0.0 ) {
				VJOIN1( ofp->arb_UVorig, ofp->arb_UVorig, f,
					ofp->arb_U );
				ofp->arb_Ulen += (-f);
			}
		}

		/*
		 *  If C-A is clockwise from B-A, then the normal
		 *  points inwards, so we need to fix it here.
		 */
		VSUB2( work, afp->A, pap->pa_center );
		f = VDOT( work, afp->peqn );
		if( f < 0.0 )  {
			VREVERSE(afp->peqn, afp->peqn);	/* "fix" normal */
			pap->pa_reversed[pap->pa_faces] = 1;
		} else {
			pap->pa_reversed[pap->pa_faces] = 0;
		}
		afp->peqn[3] = VDOT( afp->peqn, afp->A );
		return(0);				/* OK */
	default:
		/* Merely validate 4th and subsequent points */
		if( pap->pa_doopt )  {
			VSUB2( P_A, point, ofp->arb_UVorig );
			/* Check for new Ulen, Vlen */
			f = VDOT( P_A, ofp->arb_U );
			if( f > ofp->arb_Ulen ) {
				ofp->arb_Ulen = f;
			} else if( f < 0.0 ) {
				VJOIN1( ofp->arb_UVorig, ofp->arb_UVorig, f,
					ofp->arb_U );
				ofp->arb_Ulen += (-f);
			}
			f = VDOT( P_A, ofp->arb_V );
			if( f > ofp->arb_Vlen ) {
				ofp->arb_Vlen = f;
			} else if( f < 0.0 ) {
				VJOIN1( ofp->arb_UVorig, ofp->arb_UVorig, f,
					ofp->arb_V );
				ofp->arb_Vlen += (-f);
			}
		}

		VSUB2( P_A, point, afp->A );
		VUNITIZE( P_A );		/* Checking direction only */
		f = VDOT( afp->peqn, P_A );
		if( ! NEAR_ZERO(f,0.005) )  {
			/* Non-planar face */
			rt_log("arb(%s): face %s[%d] non-planar, dot=%g\n",
				pap->pa_name, title, ptno, f );
#ifdef CONSERVATIVE
			return(-1);			/* BAD */
#endif
		}
		return(0);				/* OK */
	}
	/* NOTREACHED */
}

/*
 *			R T _ A R B _ M K _ P L A N E S
 *
 *  Given an arb_internal structure with 8 points in it,
 *  compute the face information.
 *
 *  Returns -
 *	 0	OK
 *	<0	failure
 */
HIDDEN int
rt_arb_mk_planes( pap, aip )
register struct prep_arb	*pap;
struct arb_internal		*aip;
{
	register fastf_t *op;		/* Used for scanning vectors */
	LOCAL vect_t	sum;		/* Sum of all endpoints */
	register int	i;
	register int	j;
	register int	k;
	LOCAL fastf_t	f;
	int		equiv_pts[8];

	/*
	 *  Determine a point which is guaranteed to be within the solid.
	 *  This is done by averaging all the vertices.  This center is
	 *  needed for rt_arb_add_pt, which demands a point inside the solid.
	 *  The center of the enclosing RPP strategy used for the bounding
	 *  sphere can be tricked by thin plates which are non-axis aligned,
	 *  so this dual-strategy is required.  (What a bug hunt!).
	 */
	VSETALL( sum, 0 );
	op = &aip->arbi_pt[0*ELEMENTS_PER_VECT];
#	include "noalias.h"
	for( i=0; i<8; i++ )  {
		VADD2( sum, sum, op );
		op += ELEMENTS_PER_VECT;
	}
	VSCALE( pap->pa_center, sum, 0.125 );	/* sum/8 */

	/*
	 *  Find all points that are equivalent, within the specified tol.
	 *  Build the array equiv_pts[] so that it is indexed by
	 *  vertex number, and returns the lowest numbered equivalent
	 *  vertex (or it's own vertex number, if non-equivalent).
	 */
	equiv_pts[0] = 0;
	for( i=1; i<8; i++ )  {
		register pointp_t	point;
		point = &aip->arbi_pt[i*ELEMENTS_PER_VECT];
		for( j = i-1; j >= 0; j-- )  {
			/* Compare vertices I and J */
			LOCAL vect_t		work;

			VSUB2( work, point, &aip->arbi_pt[j*ELEMENTS_PER_VECT] );
			if( MAGSQ( work ) < pap->pa_tol_sq )  {
				/* Points I and J are the same, J is lower */
				equiv_pts[i] = equiv_pts[j];
				goto next_point;
			}
		}
		equiv_pts[i] = i;
	next_point: ;
	}
	if( rt_g.debug & DEBUG_ARB8 )  {
		rt_log("arb(%s) equiv_pts[] = %d %d %d %d %d %d %d %d\n",
		pap->pa_name,
		equiv_pts[0], equiv_pts[1], equiv_pts[2], equiv_pts[3],
		equiv_pts[4], equiv_pts[5], equiv_pts[6], equiv_pts[7]);
	}

	pap->pa_faces = 0;
	for( i=0; i<6; i++ )  {
		int		npts;

		npts = 0;
		for( j=0; j<4; j++ )  {
			int	pt_index;

			pt_index = rt_arb_info[i].ai_sub[j];
			if( rt_g.debug & DEBUG_ARB8 )  {
				rt_log("face %d, j=%d, npts=%d, orig_vert=%d, vert=%d\n",
					i, j, npts,
					pt_index, equiv_pts[pt_index] );
			}
			pt_index = equiv_pts[pt_index];

			/* Verify that this point is not the same
			 * as an earlier point, by checking point indices
			 */
#			include "noalias.h"
			for( k = npts-1; k >= 0; k-- )  {
				if( pap->pa_pindex[k][pap->pa_faces] == pt_index )  {
					/* Point is the same -- skip it */
					goto skip_pt;
				}
			}
			if( rt_arb_add_pt(
			    &aip->arbi_pt[pt_index*ELEMENTS_PER_VECT],
			    rt_arb_info[i].ai_title, pap, npts ) == 0 )  {
				/* Point was accepted */
				pap->pa_pindex[npts][pap->pa_faces] = pt_index;
				npts++;
			}

skip_pt:		;
		}

		if( npts < 3 )  {
			/* This face is BAD */
			continue;
		}

		if( pap->pa_doopt )  {
			register struct oface	*ofp;

			ofp = &pap->pa_opt[pap->pa_faces];
			/* Scale U and V basis vectors by
			 * the inverse of Ulen and Vlen
			 */
			ofp->arb_Ulen = 1.0 / ofp->arb_Ulen;
			ofp->arb_Vlen = 1.0 / ofp->arb_Vlen;
			VSCALE( ofp->arb_U, ofp->arb_U, ofp->arb_Ulen );
			VSCALE( ofp->arb_V, ofp->arb_V, ofp->arb_Vlen );
		}

		pap->pa_npts[pap->pa_faces] = npts;
		pap->pa_faces++;
	}
	if( pap->pa_faces < 4  || pap->pa_faces > 6 )  {
		rt_log("arb(%s):  only %d faces present\n",
			pap->pa_name, pap->pa_faces);
		return(-1);			/* Error */
	}
	return(0);			/* OK */
}

/*
 *			R T _ A R B _ S E T U P
 *
 *  This is packaged as a separate function, so that it can also be
 *  called "on the fly" from the UV mapper.
 *
 *  Returns -
 *	 0	OK
 *	!0	failure
 */
HIDDEN int
rt_arb_setup( stp, rec, rtip, uv_wanted )
struct soltab	*stp;
union record	*rec;
struct rt_i	*rtip;
int		uv_wanted;
{
	register int		i;
	struct arb_internal	ai;
	struct prep_arb		pa;

	if( rec == (union record *)0 )  {
		rec = db_getmrec( rtip->rti_dbip, stp->st_dp );
		i = rt_arb_import( &ai, rec, stp->st_pathmat );
		rt_free( (char *)rec, "arb record" );
	} else {
		i = rt_arb_import( &ai, rec, stp->st_pathmat );
	}
	if( i < 0 )  {
		rt_log("rt_arb_setup(%s): db import failure\n", stp->st_name);
		return(-1);		/* BAD */
	}

	pa.pa_doopt = uv_wanted;
	pa.pa_name = stp->st_name;
	pa.pa_tol_sq = 0.005;

	if( rt_arb_mk_planes( &pa, &ai ) < 0 )  {
		return(-2);		/* Error */
	}

	/*
	 *  Allocate a private copy of the accumulated parameters
	 *  of exactly the right size.
	 *  The size to malloc is chosen based upon the
	 *  exact number of faces.
	 */
	{
		register struct arb_specific	*arbp;
		if( (arbp = (struct arb_specific *)stp->st_specific) == 0 )  {
			arbp = (struct arb_specific *)rt_malloc(
				sizeof(struct arb_specific) +
				sizeof(struct aface) * (pa.pa_faces - 4),
				"arb_specific" );
			stp->st_specific = (genptr_t)arbp;
		}
		arbp->arb_nmfaces = pa.pa_faces;
		bcopy( (char *)pa.pa_face, (char *)arbp->arb_face,
			pa.pa_faces * sizeof(struct aface) );

		if( uv_wanted )  {
			register struct oface	*ofp;

			/*
			 * To avoid a multi-processor race here,
			 * copy the data first, THEN update arb_opt,
			 * because arb_opt doubles as the "UV avail" flag.
			 */
			ofp = (struct oface *)rt_malloc(
				pa.pa_faces * sizeof(struct oface), "arb_opt");
			bcopy( (char *)pa.pa_opt, (char *)ofp,
				pa.pa_faces * sizeof(struct oface) );
			arbp->arb_opt = ofp;
		} else {
			arbp->arb_opt = (struct oface *)0;
		}
	}

	/*
	 * Compute bounding sphere which contains the bounding RPP.
	 * Find min and max of the point co-ordinates to find the
	 * bounding RPP.  Note that this center is NOT guaranteed
	 * to be contained within the solid!
	 */
	{
		register fastf_t	*op;
		LOCAL vect_t		work;
		register fastf_t	f;

		op = &ai.arbi_pt[0];
#		include "noalias.h"
		for( i=0; i< 8; i++ ) {
			VMINMAX( stp->st_min, stp->st_max, op );
			op += ELEMENTS_PER_VECT;
		}
		VADD2SCALE( stp->st_center, stp->st_min, stp->st_max, 0.5 );
		VSUB2SCALE( work, stp->st_max, stp->st_min, 0.5 );

		f = work[X];
		if( work[Y] > f )  f = work[Y];
		if( work[Z] > f )  f = work[Z];
		stp->st_aradius = f;
		stp->st_bradius = MAGNITUDE(work);
	}
	return(0);		/* OK */
}

/*
 *  			R T _ A R B _ P R E P
 *
 *  This is the actual LIBRT "prep" interface.
 *
 *  Returns -
 *	 0	OK
 *	!0	failure
 */
int
rt_arb_prep( stp, rec, rtip )
struct soltab	*stp;
union record	*rec;
struct rt_i	*rtip;
{
	return( rt_arb_setup( stp, rec, rtip, 0 ) );
}

/*
 *  			R T _ A R B _ P R I N T
 */
void
rt_arb_print( stp )
register struct soltab *stp;
{
	register struct arb_specific *arbp =
		(struct arb_specific *)stp->st_specific;
	register struct aface	*afp;
	register int i;

	if( arbp == (struct arb_specific *)0 )  {
		rt_log("arb(%s):  no faces\n", stp->st_name);
		return;
	}
	rt_log("%d faces:\n", arbp->arb_nmfaces);
	for( i=0; i < arbp->arb_nmfaces; i++ )  {
		afp = &(arbp->arb_face[i]);
		VPRINT( "A", afp->A );
		HPRINT( "Peqn", afp->peqn );
		if( arbp->arb_opt )  {
			register struct oface	*op;
			op = &(arbp->arb_opt[i]);
			VPRINT( "UVorig", op->arb_UVorig );
			VPRINT( "U", op->arb_U );
			VPRINT( "V", op->arb_V );
			rt_log( "Ulen = %g, Vlen = %g\n",
				op->arb_Ulen, op->arb_Vlen);
		}
	}
}

/*
 *			R T _ A R B _ S H O T
 *  
 * Function -
 *	Shoot a ray at an ARB8.
 *
 * Algorithm -
 * 	The intersection distance is computed for each face.
 *  The largest IN distance and the smallest OUT distance are
 *  used as the entry and exit points.
 *  
 * Returns -
 *	0	MISS
 *  	segp	HIT
 */
struct seg *
rt_arb_shot( stp, rp, ap )
struct soltab *stp;
register struct xray *rp;
struct application	*ap;
{
	struct arb_specific *arbp = (struct arb_specific *)stp->st_specific;
	LOCAL int		iplane, oplane;
	LOCAL fastf_t		in, out;	/* ray in/out distances */
	register struct aface	*afp;
	register int		j;

	in = -INFINITY;
	out = INFINITY;
	iplane = oplane = -1;

	/* consider each face */
	for( afp = &arbp->arb_face[j=arbp->arb_nmfaces-1]; j >= 0; j--, afp-- )  {
		FAST fastf_t	dn;		/* Direction dot Normal */
		FAST fastf_t	dxbdn;
		FAST fastf_t	s;

		dxbdn = VDOT( afp->peqn, rp->r_pt ) - afp->peqn[3];
		if( (dn = -VDOT( afp->peqn, rp->r_dir )) < -SQRT_SMALL_FASTF )  {
			/* exit point, when dir.N < 0.  out = min(out,s) */
			if( out > (s = dxbdn/dn) )  {
				out = s;
				oplane = j;
			}
		} else if ( dn > SQRT_SMALL_FASTF )  {
			/* entry point, when dir.N > 0.  in = max(in,s) */
			if( in < (s = dxbdn/dn) )  {
				in = s;
				iplane = j;
			}
		}  else  {
			/* ray is parallel to plane when dir.N == 0.
			 * If it is outside the solid, stop now.
			 * Allow very small amount of slop, to catch
			 * rays that lie very nearly in the plane of a face.
			 */
			if( dxbdn > SQRT_SMALL_FASTF )
				return( SEG_NULL );	/* MISS */
		}
		if( in > out )
			return( SEG_NULL );	/* MISS */
	}
	/* Validate */
	if( iplane == -1 || oplane == -1 )  {
		rt_log("rt_arb_shoot(%s): 1 hit => MISS\n",
			stp->st_name);
		return( SEG_NULL );	/* MISS */
	}
	if( in >= out || out >= INFINITY )
		return( SEG_NULL );	/* MISS */

	{
		register struct seg *segp;

		GET_SEG( segp, ap->a_resource );
		segp->seg_stp = stp;
		segp->seg_in.hit_dist = in;
		segp->seg_in.hit_private = (char *)iplane;

		segp->seg_out.hit_dist = out;
		segp->seg_out.hit_private = (char *)oplane;
		return(segp);			/* HIT */
	}
	/* NOTREACHED */
}

#define SEG_MISS(SEG)		(SEG).seg_stp=(struct soltab *) 0;	
/*
 *			R T _ A R B _ V S H O T
 *
 *  This is the Becker vector version
 */
void
rt_arb_vshot( stp, rp, segp, n, resp)
struct soltab	       *stp[]; /* An array of solid pointers */
struct xray		*rp[]; /* An array of ray pointers */
struct  seg            segp[]; /* array of segs (results returned) */
int		 	    n; /* Number of ray/object pairs */
struct resource         *resp; /* pointer to a list of free segs */
{
	register int    j, i;
	register struct arb_specific *arbp;
	FAST fastf_t	dn;		/* Direction dot Normal */
	FAST fastf_t	dxbdn;
	FAST fastf_t	s;

	/* Intialize return values */
#	include "noalias.h"
	for(i = 0; i < n; i++){
		segp[i].seg_stp = stp[i];	/* Assume hit, if 0 then miss */
                segp[i].seg_in.hit_dist = -INFINITY;        /* used as in */
                segp[i].seg_in.hit_private = (char *) -1;   /* used as iplane */
                segp[i].seg_out.hit_dist = INFINITY;        /* used as out */
                segp[i].seg_out.hit_private = (char *) -1;  /* used as oplane */
                segp[i].seg_next = SEG_NULL;
	}

	/* consider each face */
	for(j = 0; j < 6; j++)  {
		/* for each ray/arb_face pair */
#		include "noalias.h"
		for(i = 0; i < n; i++)  {
			if (stp[i] == 0) continue;	/* skip this ray */
			if ( segp[i].seg_stp == 0 ) continue;	/* miss */

			arbp= (struct arb_specific *) stp[i]->st_specific;
			if ( arbp->arb_nmfaces <= j )
				continue; /* faces of this ARB are done */

			dxbdn = VDOT( arbp->arb_face[j].peqn, rp[i]->r_pt ) -
				arbp->arb_face[j].peqn[3];
			if( (dn = -VDOT( arbp->arb_face[j].peqn, rp[i]->r_dir )) <
							-SQRT_SMALL_FASTF )  {
			   /* exit point, when dir.N < 0.  out = min(out,s) */
			   if( segp[i].seg_out.hit_dist > (s = dxbdn/dn) )  {
			   	   segp[i].seg_out.hit_dist = s;
				   segp[i].seg_out.hit_private = (char *) j;
			   }
			} else if ( dn > SQRT_SMALL_FASTF )  {
			   /* entry point, when dir.N > 0.  in = max(in,s) */
			   if( segp[i].seg_in.hit_dist < (s = dxbdn/dn) )  {
				   segp[i].seg_in.hit_dist = s;
				   segp[i].seg_in.hit_private = (char *) j;
			   }
		        }  else  {
			   /* ray is parallel to plane when dir.N == 0.
			    * If it is outside the solid, stop now */
			   if( dxbdn > SQRT_SMALL_FASTF ) {
				SEG_MISS(segp[i]);		/* MISS */
			   }
			}
		        if(segp[i].seg_in.hit_dist > segp[i].seg_out.hit_dist) {
			   SEG_MISS(segp[i]);		/* MISS */
			}
		} /* for each ray/arb_face pair */
	} /* for each arb_face */

	/*
	 *  Validate for each ray/arb_face pair
	 *  Segment was initialized as "good" (seg_stp set valid);
	 *  that is revoked here on misses.
	 */
#	include "noalias.h"
	for(i = 0; i < n; i++){
		if (stp[i] == 0) continue;		/* skip this ray */
		if ( segp[i].seg_stp == 0 ) continue;	/* missed */

		if( segp[i].seg_in.hit_private == (char *) -1 ||
		    segp[i].seg_out.hit_private == (char *) -1 )  {
			SEG_MISS(segp[i]);		/* MISS */
		}
		else if(segp[i].seg_in.hit_dist >= segp[i].seg_out.hit_dist ||
			segp[i].seg_out.hit_dist >= INFINITY ) {
			SEG_MISS(segp[i]);		/* MISS */
		}
	}
}

/*
 *  			R T _ A R B _ N O R M
 *
 *  Given ONE ray distance, return the normal and entry/exit point.
 */
void
rt_arb_norm( hitp, stp, rp )
register struct hit *hitp;
struct soltab *stp;
register struct xray *rp;
{
	register struct arb_specific *arbp =
		(struct arb_specific *)stp->st_specific;
	int	h;

	VJOIN1( hitp->hit_point, rp->r_pt, hitp->hit_dist, rp->r_dir );
	h = (int)hitp->hit_private;
	VMOVE( hitp->hit_normal, arbp->arb_face[h].peqn );
}

/*
 *			R T _ A R B _ C U R V E
 *
 *  Return the "curvature" of the ARB face.
 *  Pick a principle direction orthogonal to normal, and 
 *  indicate no curvature.
 */
void
rt_arb_curve( cvp, hitp, stp )
register struct curvature *cvp;
register struct hit *hitp;
struct soltab *stp;
{

	vec_ortho( cvp->crv_pdir, hitp->hit_normal );
	cvp->crv_c1 = cvp->crv_c2 = 0;
}

/*
 *  			R T _ A R B _ U V
 *  
 *  For a hit on a face of an ARB, return the (u,v) coordinates
 *  of the hit point.  0 <= u,v <= 1.
 *  u extends along the arb_U direction defined by B-A,
 *  v extends along the arb_V direction defined by Nx(B-A).
 */
void
rt_arb_uv( ap, stp, hitp, uvp )
struct application *ap;
struct soltab *stp;
register struct hit *hitp;
register struct uvcoord *uvp;
{
	register struct arb_specific *arbp =
		(struct arb_specific *)stp->st_specific;
	int	h;
	LOCAL vect_t P_A;
	LOCAL fastf_t r;

	if( arbp->arb_opt == (struct oface *)0 )  {
		register int	ret = 0;

		/*
		 *  The double check of arb_opt is to avoid the case
		 *  where another processor did the UV setup while
		 *  this processor was waiting in RES_ACQUIRE().
		 */
		RES_ACQUIRE( &rt_g.res_model );
		if( arbp->arb_opt == (struct oface *)0 )  {
			ret = rt_arb_setup(stp, (union record *)0, ap->a_rt_i, 1);
		}
		RES_RELEASE( &rt_g.res_model );

		if( ret != 0 || arbp->arb_opt == (struct oface *)0 )  {
			rt_log("rt_arb_uv(%s) dyanmic setup failure st_specific=x%x, optp=x%x\n",
				stp->st_name,
		    		stp->st_specific, arbp->arb_opt );
			return;
		}
		if(rt_g.debug&DEBUG_SOLIDS)  rt_pr_soltab( stp );
	}

	h = (int)hitp->hit_private;

	VSUB2( P_A, hitp->hit_point, arbp->arb_opt[h].arb_UVorig );
	/* Flipping v is an artifact of how the faces are built */
	uvp->uv_u = VDOT( P_A, arbp->arb_opt[h].arb_U );
	uvp->uv_v = 1.0 - VDOT( P_A, arbp->arb_opt[h].arb_V );
	if( uvp->uv_u < 0 || uvp->uv_v < 0 || uvp->uv_u > 1 || uvp->uv_v > 1 )  {
		rt_log("arb_uv: bad uv=%g,%g\n", uvp->uv_u, uvp->uv_v);
		/* Fix it up */
		if( uvp->uv_u < 0 )  uvp->uv_u = (-uvp->uv_u);
		if( uvp->uv_v < 0 )  uvp->uv_v = (-uvp->uv_v);
	}
	r = ap->a_rbeam + ap->a_diverge * hitp->hit_dist;
	uvp->uv_du = r * arbp->arb_opt[h].arb_Ulen;
	uvp->uv_dv = r * arbp->arb_opt[h].arb_Vlen;
}

/*
 *			R T _ A R B _ F R E E
 */
void
rt_arb_free( stp )
register struct soltab *stp;
{
	register struct arb_specific *arbp =
		(struct arb_specific *)stp->st_specific;

	if( arbp->arb_opt )
		rt_free( (char *)arbp->arb_opt, "arb_opt" );
	rt_free( (char *)arbp, "arb_specific" );
}

#define ARB_FACE( valp, a, b, c, d ) \
	ADD_VL( vhead, &valp[a*3], 0 ); \
	ADD_VL( vhead, &valp[b*3], 1 ); \
	ADD_VL( vhead, &valp[c*3], 1 ); \
	ADD_VL( vhead, &valp[d*3], 1 );

/*
 *  			R T _ A R B _ P L O T
 *
 *  Plot an ARB by tracing out four "U" shaped contours
 *  This draws each edge only once.
 *  XXX No checking for degenerate faces is done, but probably should be.
 */
int
rt_arb_plot( rp, matp, vhead, dp )
register union record	*rp;
register matp_t		matp;
struct vlhead		*vhead;
struct directory	*dp;
{
	struct arb_internal	ai;

	if( rt_arb_import( &ai, rp, matp ) < 0 )  {
		rt_log("rt_arb_plot(%s): db import failure\n", dp->d_namep);
		return(-1);
	}

	ARB_FACE( ai.arbi_pt, 0, 1, 2, 3 );
	ARB_FACE( ai.arbi_pt, 4, 0, 3, 7 );
	ARB_FACE( ai.arbi_pt, 5, 4, 7, 6 );
	ARB_FACE( ai.arbi_pt, 1, 5, 6, 2 );
	return(0);
}

/*
 *			R T _ A R B _ C L A S S
 */
int
rt_arb_class()
{
	return(0);
}

/*
 *			R T _ A R B _ I M P O R T
 *
 *  Import an ARB8 from the database format to the internal format.
 *  There are two parts to this:  First, the database is presently
 *  single precision binary floating point.
 *  Secondly, the ARB in the database is represented as a vector
 *  from the origin to the first point, and 7 vectors
 *  from the first point to the remaining points.  In 1979 it seemed
 *  like a good idea...
 *
 *  Convert from vector to point notation
 *  by rotating each vector and adding in the base vector.
 */
int
rt_arb_import( aip, rp, matp )
struct arb_internal	*aip;
union record		*rp;
register matp_t		matp;
{
	register int		i;
	register fastf_t	*ip;
	register fastf_t	*op;
	LOCAL vect_t		work;
	LOCAL fastf_t		vec[3*8];
	
	/* Check record type */
	if( rp->u_id != ID_SOLID )  {
		rt_log("rt_arb_import: defective record, id=x%x\n", rp->u_id);
		return(-1);
	}

	/* Convert from database to internal format */
	rt_fastf_float( vec, rp->s.s_values, 8 );

	/*
	 * Convert from vector to point notation for drawing.
	 */
	MAT4X3PNT( &aip->arbi_pt[0], matp, &vec[0] );

	ip = &vec[1*3];
	op = &aip->arbi_pt[1*3];
#	include "noalias.h"
	for( i=1; i<8; i++ )  {
		VADD2( work, &vec[0], ip );
		MAT4X3PNT( op, matp, work );
		ip += 3;
		op += ELEMENTS_PER_VECT;
	}
	return(0);			/* OK */
}

/*
 *			R T _ A R B _ T E S S
 *
 *  "Tessellate" an ARB into an NMG data structure.
 *  Purely a mechanical transformation of one faceted object
 *  into another.
 *
 *  Returns -
 *	-1	failure
 *	 0	OK.  *r points to nmgregion that holds this tessellation.
 */
int
rt_arb_tess( r, m, rp, mat, dp, abs_tol, rel_tol, norm_tol )
struct nmgregion	**r;
struct model		*m;
union record		*rp;
mat_t			mat;
struct directory	*dp;
double			abs_tol;
double			rel_tol;
double			norm_tol;
{
	struct shell		*s;
	struct arb_internal	ai;
	struct prep_arb		pa;
	register int		i;
	struct faceuse		*fu[6];
	struct vertex		*verts[8];
	struct vertex		**vertp[4];

	if( rt_arb_import( &ai, rp, mat ) < 0 )  {
		rt_log("rt_arb_tess(%s): import failure\n", dp->d_namep);
		return(-1);
	}
	bzero( (char *)&pa, sizeof(pa) );
	pa.pa_doopt = 0;		/* no UV stuff */
	pa.pa_name = dp->d_namep;
	pa.pa_tol_sq = 0.005;	/* XXX need real tolerance here! XXX */
	if( rt_arb_mk_planes( &pa, &ai ) < 0 )  return(-2);

	for( i=0; i<8; i++ )  verts[i] = (struct vertex *)0;

	*r = nmg_mrsv( m );	/* Make region, empty shell, vertex */
	s = NMG_LIST_FIRST(shell, &(*r)->s_hd);

	/* Process each face */
	for( i=0; i < pa.pa_faces; i++ )  {
		if( pa.pa_reversed[i] == 0 )  {
			/* Normal orientation */
			vertp[0] = &verts[pa.pa_pindex[0][i]];
			vertp[1] = &verts[pa.pa_pindex[1][i]];
			vertp[2] = &verts[pa.pa_pindex[2][i]];
			if( pa.pa_npts[i] > 3 ) {
				vertp[3] = &verts[pa.pa_pindex[3][i]];
			}
		} else {
			register struct vertex	***vertpp = vertp;
			if( pa.pa_npts[i] > 3 ) {
				*vertpp++ = &verts[pa.pa_pindex[3][i]];
			}
			*vertpp++ = &verts[pa.pa_pindex[2][i]];
			*vertpp++ = &verts[pa.pa_pindex[1][i]];
			*vertpp++ = &verts[pa.pa_pindex[0][i]];
		}
		if( rt_g.debug & DEBUG_ARB8 )  {
			rt_log("face %d, npts=%d, verts %d %d %d %d\n",
				i, pa.pa_npts[i],
				pa.pa_pindex[0][i], pa.pa_pindex[1][i],
				pa.pa_pindex[2][i], pa.pa_pindex[3][i] );
		}
		if( (fu[i] = nmg_cmface( s, vertp, pa.pa_npts[i] )) == 0 )  {
			rt_log("rt_arb_tess(%s): nmg_cmface() fail on face %d\n", i);
			continue;
		}
	}

	/* Associate vertex geometry */
	for( i=0; i<8; i++ )
		if(verts[i]) nmg_vertex_gv(verts[i], &ai.arbi_pt[3*i]);

	/* Associate face geometry */
	for( i=0; i < pa.pa_faces; i++ )  {
#if 1
		/* We already know the plane equations, this is fast */
		nmg_face_g( fu[i], pa.pa_face[i].peqn );
#else
		/* For the cautious, ensure topology and geometry match */
		rt_mk_nmg_planeeqn( fu[i] );
#endif
	}

	/* Compute "geometry" for region and shell */
	nmg_region_a( *r );
	return(0);
}

/*
 *			R T _ M K _ N M G _ P L A N E E Q N
 *
 *  This routine is just a hack, for getting started quickly.
 *
 *  If face was built in clockwise manner from points A, B, C, then
 *	A is at eu
 *	B is at eu->last, and
 *	C is at eu->next
 *  as a consequence of the way nmg_cmface() makes the face.
 */
rt_mk_nmg_planeeqn( fu )
struct faceuse	*fu;
{
	struct edgeuse		*eu, *eu_last, *eu_next;
	struct loopuse		*lu;
	plane_t			plane;
	struct vertex_g		*a, *b, *c;

	if( fu == (struct faceuse *)0 )  {
		rt_bomb("rt_mk_nmg_planeeqn(): null faceuse\n");
	}

	lu = NMG_LIST_FIRST(loopuse, &fu->lu_hd);
	NMG_CK_LOOPUSE(lu);

	eu = NMG_LIST_FIRST(edgeuse, &lu->down_hd);
	NMG_CK_EDGEUSE(eu);

	eu_last = NMG_LIST_PLAST_CIRC(edgeuse, eu);
	eu_next = NMG_LIST_PNEXT_CIRC(edgeuse, eu);
	NMG_CK_EDGEUSE(eu_last);
	NMG_CK_EDGEUSE(eu_next);

	a = eu->vu_p->v_p->vg_p;
	b = eu_last->vu_p->v_p->vg_p;
	c = eu_next->vu_p->v_p->vg_p;
	NMG_CK_VERTEX_G(a);
	NMG_CK_VERTEX_G(b);
	NMG_CK_VERTEX_G(c);

	if (rt_mk_plane_3pts(plane, a->coord, b->coord, c->coord) < 0 ) {
		rt_log("rt_mk_nmg_planeeqn(): rt_mk_plane_3pts failed on (%g,%g,%g) (%g,%g,%g) (%g,%g,%g)\n",
			V3ARGS( a->coord ),
			V3ARGS( b->coord ),
			V3ARGS( c->coord ) );
	    	HPRINT("plane", plane);
	}
	else if (plane[0] == 0.0 && plane[1] == 0.0 && plane[2] == 0.0) {
		rt_log("rt_mk_nmg_planeeqn():  Bad plane equation from rt_mk_plane_3pts\n" );
	    	HPRINT("plane", plane);
	}
	else nmg_face_g( fu, plane);
}
