/*
 *			P L A N E . C
 *
 *  Some useful routines for dealing with planes and lines.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Distribution Status -
 *	Public Domain, Distribution Unlimited
 */
#ifndef lint
static char RCSplane[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>

#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./debug.h"

/*
 *			R T _ P T 3 _ P T 3 _ E Q U A L
 *
 *  Returns -
 *	1	if the two points are equal, within the tolerance
 *	0	if the two points are not "the same"
 */
int
rt_pt3_pt3_equal( a, b, tol )
CONST point_t		a;
CONST point_t		b;
CONST struct rt_tol	*tol;
{
	vect_t	diff;

	RT_CK_TOL(tol);
	VSUB2( diff, b, a );
	if( MAGSQ( diff ) < tol->dist_sq )  return 1;
	return 0;
}

/*
 *			R T _ P T 2 _ P T 2 _ E Q U A L
 *
 *  Returns -
 *	1	if the two points are equal, within the tolerance
 *	0	if the two points are not "the same"
 */
int
rt_pt2_pt2_equal( a, b, tol )
CONST point_t		a;
CONST point_t		b;
CONST struct rt_tol	*tol;
{
	vect_t	diff;

	RT_CK_TOL(tol);
	VSUB2_2D( diff, b, a );
	if( MAGSQ_2D( diff ) < tol->dist_sq )  return 1;
	return 0;
}

/*
 *			R T _ 3 P T S _ C O L L I N E A R
 *
 *  Check to see if three points are collinear.
 *
 *  The algorithm is designed to work properly regardless of the
 *  order in which the points are provided.
 *
 *  Returns (boolean) -
 *	1	If 3 points are collinear
 *	0	If they are not
 */
int
rt_3pts_collinear(a, b, c, tol)
point_t	a, b, c;
struct rt_tol	*tol;
{
	fastf_t	mag_ab, mag_bc, mag_ca, max_len, dist_sq;
	fastf_t cos_a, cos_b, cos_c;
	vect_t	ab, bc, ca;
	int max_edge_no;

	VSUB2(ab, b, a);
	VSUB2(bc, c, b);
	VSUB2(ca, a, c);
	mag_ab = MAGNITUDE(ab);
	mag_bc = MAGNITUDE(bc);
	mag_ca = MAGNITUDE(ca);

	/* find longest edge */
	max_len = mag_ab;
	max_edge_no = 1;

	if( mag_bc > max_len )
	{
		max_len = mag_bc;
		max_edge_no = 2;
	}

	if( mag_ca > max_len )
	{
		max_len = mag_ca;
		max_edge_no = 3;
	}

	switch( max_edge_no )
	{
		case 1:
			cos_b = (-VDOT( ab , bc ))/(mag_ab * mag_bc );
			dist_sq = mag_bc*mag_bc*( 1.0 - cos_b*cos_b);
			break;
		case 2:
			cos_c = (-VDOT( bc , ca ))/(mag_bc * mag_ca );
			dist_sq = mag_ca*mag_ca*(1.0 - cos_c*cos_c);
			break;
		case 3:
			cos_a = (-VDOT( ca , ab ))/(mag_ca * mag_ab );
			dist_sq = mag_ab*mag_ab*(1.0 - cos_a*cos_a);
			break;
	}

	if( dist_sq <= tol->dist_sq )
		return( 1 );
	else
		return( 0 );
}


/*
 *			R T _ 3 P T S _ D I S T I N C T
 *
 *  Check to see if three points are all distinct, i.e.,
 *  ensure that there is at least sqrt(dist_tol_sq) distance
 *  between every pair of points.
 *
 *  Returns (boolean) -
 *	1	If all three points are distinct
 *	0	If two or more points are closer together than dist_tol_sq
 */
int
rt_3pts_distinct( a, b, c, tol )
CONST point_t		a, b, c;
CONST struct rt_tol	*tol;
{
	vect_t	B_A;
	vect_t	C_A;
	vect_t	C_B;

	RT_CK_TOL(tol);
	VSUB2( B_A, b, a );
	if( MAGSQ( B_A ) <= tol->dist_sq )  return(0);
	VSUB2( C_A, c, a );
	if( MAGSQ( C_A ) <= tol->dist_sq )  return(0);
	VSUB2( C_B, c, b );
	if( MAGSQ( C_B ) <= tol->dist_sq )  return(0);
	return(1);
}

/*
 *			R T _ M K _ P L A N E _ 3 P T S
 *
 *  Find the equation of a plane that contains three points.
 *  Note that normal vector created is expected to point out (see vmath.h),
 *  so the vector from A to C had better be counter-clockwise
 *  (about the point A) from the vector from A to B.
 *  This follows the BRL-CAD outward-pointing normal convention, and the
 *  right-hand rule for cross products.
 *
 *
 *			C
 *	                *
 *	                |\
 *	                | \
 *	   ^     N      |  \
 *	   |      \     |   \
 *	   |       \    |    \
 *	   |C-A     \   |     \
 *	   |         \  |      \
 *	   |          \ |       \
 *	               \|        \
 *	                *---------*
 *	                A         B
 *			   ----->
 *		            B-A
 *
 *  If the points are given in the order A B C (eg, *counter*-clockwise),
 *  then the outward pointing surface normal N = (B-A) x (C-A).
 *
 *  Explicit Return -
 *	 0	OK
 *	-1	Failure.  At least two of the points were not distinct,
 *		or all three were colinear.
 *
 *  Implicit Return -
 *	plane	The plane equation is stored here.
 */
int
rt_mk_plane_3pts( plane, a, b, c, tol )
plane_t			plane;
CONST point_t		a, b, c;
CONST struct rt_tol	*tol;
{
	vect_t	B_A;
	vect_t	C_A;
	vect_t	C_B;
	register fastf_t mag;

	RT_CK_TOL(tol);

	VSUB2( B_A, b, a );
	if( MAGSQ( B_A ) <= tol->dist_sq )  return(-1);
	VSUB2( C_A, c, a );
	if( MAGSQ( C_A ) <= tol->dist_sq )  return(-1);
	VSUB2( C_B, c, b );
	if( MAGSQ( C_B ) <= tol->dist_sq )  return(-1);

	VCROSS( plane, B_A, C_A );

	/* Ensure unit length normal */
	if( (mag = MAGNITUDE(plane)) <= SQRT_SMALL_FASTF )
		return(-1);	/* FAIL */
	mag = 1/mag;
	VSCALE( plane, plane, mag );

	/* Find distance from the origin to the plane */
	/* XXX Should do with pt that has smallest magnitude (closest to origin) */
	plane[3] = VDOT( plane, a );

#if 0
	/* Check to be sure that angle between A-Origin and N vect < 89 degrees */
	/* XXX Could complain for pts on axis-aligned plane, far from origin */
	mag = MAGSQ(a);
	if( mag > tol->dist_sq )  {
		/* cos(89 degrees) = 0.017452406, reciprocal is 57.29 */
		if( plane[3]/sqrt(mag) < 0.017452406 )  {
			rt_log("rt_mk_plane_3pts() WARNING: plane[3] value is suspect\n");
		}
	}
#endif
	return(0);		/* OK */
}

/*
 *			R T _ M K P O I N T _ 3 P L A N E S
 *
 *  Given the description of three planes, compute the point of intersection,
 *  if any.
 *
 *  Find the solution to a system of three equations in three unknowns:
 *
 *	Px * Ax + Py * Ay + Pz * Az = -A3;
 *	Px * Bx + Py * By + Pz * Bz = -B3;
 *	Px * Cx + Py * Cy + Pz * Cz = -C3;
 *
 *  or
 *
 *	[ Ax  Ay  Az ]   [ Px ]   [ -A3 ]
 *	[ Bx  By  Bz ] * [ Py ] = [ -B3 ]
 *	[ Cx  Cy  Cz ]   [ Pz ]   [ -C3 ]
 *
 *
 *  Explitic Return -
 *	 0	OK
 *	-1	Failure.  Intersection is a line or plane.
 *
 *  Implicit Return -
 *	pt	The point of intersection is stored here.
 */
int
rt_mkpoint_3planes( pt, a, b, c )
point_t		pt;
CONST plane_t	a, b, c;
{
	vect_t	v1, v2, v3;
	register fastf_t det;

	/* Find a vector perpendicular to both planes B and C */
	VCROSS( v1, b, c );

	/*  If that vector is perpendicular to A,
	 *  then A is parallel to either B or C, and no intersection exists.
	 *  This dot&cross product is the determinant of the matrix M.
	 *  (I suspect there is some deep significance to this!)
	 */
	det = VDOT( a, v1 );
	if( NEAR_ZERO( det, SQRT_SMALL_FASTF ) )  return(-1);

	VCROSS( v2, a, c );
	VCROSS( v3, a, b );

	det = 1/det;
	pt[X] = det*(a[3]*v1[X] - b[3]*v2[X] + c[3]*v3[X]);
	pt[Y] = det*(a[3]*v1[Y] - b[3]*v2[Y] + c[3]*v3[Y]);
	pt[Z] = det*(a[3]*v1[Z] - b[3]*v2[Z] + c[3]*v3[Z]);
	return(0);
}

/*
 *			R T _ I S E C T _ L I N E 3 _ P L A N E
 *
 *  Intersect an infinite line (specified in point and direction vector form)
 *  with a plane that has an outward pointing normal.
 *  The direction vector need not have unit length.
 *
 *  Explicit Return -
 *	-2	missed (ray is outside halfspace)
 *	-1	missed (ray is inside)
 *	 0	line lies on plane
 *	 1	hit (ray is entering halfspace)
 *	 2	hit (ray is leaving)
 *
 *  Implicit Return -
 *	The value at *dist is set to the parametric distance of the intercept
 */
int
rt_isect_line3_plane( dist, pt, dir, plane, tol )
fastf_t		*dist;
CONST point_t	pt;
CONST vect_t	dir;
CONST plane_t	plane;
CONST struct rt_tol	*tol;
{
	register fastf_t	slant_factor;
	register fastf_t	norm_dist;

	RT_CK_TOL(tol);

	norm_dist = plane[3] - VDOT( plane, pt );
	slant_factor = VDOT( plane, dir );

	if( slant_factor < -SQRT_SMALL_FASTF )  {
		*dist = norm_dist/slant_factor;
		return 1;			/* HIT, entering */
	} else if( slant_factor > SQRT_SMALL_FASTF )  {
		*dist = norm_dist/slant_factor;
		return 2;			/* HIT, leaving */
	}

	/*
	 *  Ray is parallel to plane when dir.N == 0.
	 */
	*dist = 0;		/* sanity */
	if( norm_dist < -tol->dist )
		return -2;	/* missed, outside */
	if( norm_dist > tol->dist )
		return -1;	/* missed, inside */
	return 0;		/* Ray lies in the plane */
}

/*
 *			R T _ I S E C T _ 2 P L A N E S
 *
 *  Given two planes, find the line of intersection between them,
 *  if one exists.
 *  The line of intersection is returned in parametric line
 *  (point & direction vector) form.
 *
 *  In order that all the geometry under consideration be in "front"
 *  of the ray, it is necessary to pass the minimum point of the model
 *  RPP.  If this convention is unnecessary, just pass (0,0,0) as rpp_min.
 *
 *  Explicit Return -
 *	 0	OK, line of intersection stored in `pt' and `dir'.
 *	-1	FAIL, planes are identical (co-planar)
 *	-2	FAIL, planes are parallel and distinct
 *	-3	FAIL, unable to find line of intersection
 *
 *  Implicit Returns -
 *	pt	Starting point of line of intersection
 *	dir	Direction vector of line of intersection (unit length)
 */
int
rt_isect_2planes( pt, dir, a, b, rpp_min, tol )
point_t		pt;
vect_t		dir;
CONST plane_t	a;
CONST plane_t	b;
CONST vect_t	rpp_min;
CONST struct rt_tol	*tol;
{
	register fastf_t	d;
	LOCAL vect_t		abs_dir;
	LOCAL plane_t		pl;
	int			i;

	if( (i = rt_coplanar( a, b, tol )) != 0 )  {
		if( i > 0 )
			return(-1);	/* FAIL -- coplanar */
		return(-2);		/* FAIL -- parallel & distinct */
	}

	/* Direction vector for ray is perpendicular to both plane normals */
	VCROSS( dir, a, b );
	VUNITIZE( dir );		/* safety */

	/*
	 *  Select an axis-aligned plane which has it's normal pointing
	 *  along the same axis as the largest magnitude component of
	 *  the direction vector.
	 *  If the largest magnitude component is negative, reverse the
	 *  direction vector, so that model is "in front" of start point.
	 */
	abs_dir[X] = (dir[X] >= 0) ? dir[X] : (-dir[X]);
	abs_dir[Y] = (dir[Y] >= 0) ? dir[Y] : (-dir[Y]);
	abs_dir[Z] = (dir[Z] >= 0) ? dir[Z] : (-dir[Z]);

	if( abs_dir[X] >= abs_dir[Y] )  {
		if( abs_dir[X] >= abs_dir[Z] )  {
			VSET( pl, 1, 0, 0 );	/* X */
			pl[3] = rpp_min[X];
			if( dir[X] < 0 )  {
				VREVERSE( dir, dir );
			}
		} else {
			VSET( pl, 0, 0, 1 );	/* Z */
			pl[3] = rpp_min[Z];
			if( dir[Z] < 0 )  {
				VREVERSE( dir, dir );
			}
		}
	} else {
		if( abs_dir[Y] >= abs_dir[Z] )  {
			VSET( pl, 0, 1, 0 );	/* Y */
			pl[3] = rpp_min[Y];
			if( dir[Y] < 0 )  {
				VREVERSE( dir, dir );
			}
		} else {
			VSET( pl, 0, 0, 1 );	/* Z */
			pl[3] = rpp_min[Z];
			if( dir[Z] < 0 )  {
				VREVERSE( dir, dir );
			}
		}
	}

	/* Intersection of the 3 planes defines ray start point */
	if( rt_mkpoint_3planes( pt, pl, a, b ) < 0 )
		return(-3);	/* FAIL -- no intersection */

	return(0);		/* OK */
}

/*
 *			R T _ I S E C T _ L I N E 2 _ L I N E 2
 *
 *  Intersect two lines, each in given in parametric form:
 *
 *	X = P + t * D
 *  and
 *	X = A + u * C
 *
 *  While the parametric form is usually used to denote a ray
 *  (ie, positive values of the parameter only), in this case
 *  the full line is considered.
 *
 *  The direction vectors C and D need not have unit length.
 *
 *  Explicit Return -
 *	-1	no intersection, lines are parallel.
 *	 0	lines are co-linear
 *			dist[0] gives distance from P to A,
 *			dist[1] gives distance from P to (A+C) [not same as below]
 *	 1	intersection found (t and u returned)
 *			dist[0] gives distance from P to isect,
 *			dist[1] gives distance from A to isect.
 *
 *  Implicit Returns -
 *	When explicit return > 0, dist[0] and dist[1] are the
 *	line parameters of the intersection point on the 2 rays.
 *	The actual intersection coordinates can be found by
 *	substituting either of these into the original ray equations.
 *
 *  Note that for lines which are very nearly parallel, but not
 *  quite parallel enough to have the determinant go to "zero",
 *  the intersection can turn up in surprising places.
 *  (e.g. when det=1e-15 and det1=5.5e-17, t=0.5)
 */
int
rt_isect_line2_line2( dist, p, d, a, c, tol )
fastf_t			*dist;			/* dist[2] */
CONST point_t		p;
CONST vect_t		d;
CONST point_t		a;
CONST vect_t		c;
CONST struct rt_tol	*tol;
{
	fastf_t			hx, hy;		/* A - P */
	register fastf_t	det;
	register fastf_t	det1;

	RT_CK_TOL(tol);
	if( rt_g.debug & DEBUG_MATH )  {
		rt_log("rt_isect_line2_line2() p=(%g,%g), d=(%g,%g)\n\t\t\ta=(%g,%g), c=(%g,%g)\n",
			V2ARGS(p), V2ARGS(d), V2ARGS(a), V2ARGS(c) );
	}

	/*
	 *  From the two components q and r, form a system
	 *  of 2 equations in 2 unknowns.
	 *  Solve for t and u in the system:
	 *
	 *	Px + t * Dx = Ax + u * Cx
	 *	Py + t * Dy = Ay + u * Cy
	 *  or
	 *	t * Dx - u * Cx = Ax - Px
	 *	t * Dy - u * Cy = Ay - Py
	 *
	 *  Let H = A - P, resulting in:
	 *
	 *	t * Dx - u * Cx = Hx
	 *	t * Dy - u * Cy = Hy
	 *
	 *  or
	 *
	 *	[ Dx  -Cx ]   [ t ]   [ Hx ]
	 *	[         ] * [   ] = [    ]
	 *	[ Dy  -Cy ]   [ u ]   [ Hy ]
	 *
	 *  This system can be solved by direct substitution, or by
	 *  finding the determinants by Cramers rule:
	 *
	 *	             [ Dx  -Cx ]
	 *	det(M) = det [         ] = -Dx * Cy + Cx * Dy
	 *	             [ Dy  -Cy ]
	 *
	 *  If det(M) is zero, then the lines are parallel (perhaps colinear).
	 *  Otherwise, exactly one solution exists.
	 */
	det = c[X] * d[Y] - d[X] * c[Y];

	/*
	 *  det(M) is non-zero, so there is exactly one solution.
	 *  Using Cramer's rule, det1(M) replaces the first column
	 *  of M with the constant column vector, in this case H.
	 *  Similarly, det2(M) replaces the second column.
	 *  Computation of the determinant is done as before.
	 *
	 *  Now,
	 *
	 *	                  [ Hx  -Cx ]
	 *	              det [         ]
	 *	    det1(M)       [ Hy  -Cy ]   -Hx * Cy + Cx * Hy
	 *	t = ------- = --------------- = ------------------
	 *	     det(M)       det(M)        -Dx * Cy + Cx * Dy
	 *
	 *  and
	 *
	 *	                  [ Dx   Hx ]
	 *	              det [         ]
	 *	    det2(M)       [ Dy   Hy ]    Dx * Hy - Hx * Dy
	 *	u = ------- = --------------- = ------------------
	 *	     det(M)       det(M)        -Dx * Cy + Cx * Dy
	 */
	hx = a[X] - p[X];
	hy = a[Y] - p[Y];
	det1 = (c[X] * hy - hx * c[Y]);

	/* XXX This zero tolerance here should actually be
	 * XXX determined by something like
	 * XXX max(c[X], c[Y], d[X], d[Y]) / MAX_FASTF_DYNAMIC_RANGE
	 * XXX In any case, nothing smaller than 1e-16
	 */
#define DETERMINANT_TOL		1.0e-14		/* XXX caution on non-IEEE machines */
	if( NEAR_ZERO( det, DETERMINANT_TOL ) )  {
		/* Lines are parallel */
		if( !NEAR_ZERO( det1, DETERMINANT_TOL ) )  {
			/* Lines are NOT co-linear, just parallel */
			if( rt_g.debug & DEBUG_MATH )  {
				rt_log("\tparallel, not co-linear.  det=%e, det1=%g\n", det, det1);
			}
			return -1;	/* parallel, no intersection */
		}

		/*
		 *  Lines are co-linear.
		 *  Determine t as distance from P to A.
		 *  Determine u as distance from P to (A+C).  [special!]
		 *  Use largest direction component, for numeric stability
		 *  (and avoiding division by zero).
		 */
		if( fabs(d[X]) >= fabs(d[Y]) )  {
			dist[0] = hx/d[X];
			dist[1] = (hx + c[X]) / d[X];
		} else {
			dist[0] = hy/d[Y];
			dist[1] = (hy + c[Y]) / d[Y];
		}
		if( rt_g.debug & DEBUG_MATH )  {
			rt_log("\tcolinear, t = %g, u = %g\n", dist[0], dist[1] );
		}
		return 0;	/* Lines co-linear */
	}
	if( rt_g.debug & DEBUG_MATH )  {
		/* XXX This print is temporary */
rt_log("\thx=%g, hy=%g, det=%g, det1=%g, det2=%g\n", hx, hy, det, det1, (d[X] * hy - hx * d[Y]) );
	}
	det = 1/det;
	dist[0] = det * det1;
	dist[1] = det * (d[X] * hy - hx * d[Y]);
	if( rt_g.debug & DEBUG_MATH )  {
		rt_log("\tintersection, t = %g, u = %g\n", dist[0], dist[1] );
	}

#if 0
	/* XXX This isn't any good.
	 * 1)  Sometimes, dist[0] is very large.  Only caller can tell whether
	 *     that is useful to him or not.
	 * 2)  Sometimes, the difference between the two hit points is
	 *     not much more than tol->dist.  Either hit point is perfectly
	 *     good;  the caller just needs to be careful and not use *both*.
	 */
	{
		point_t		hit1, hit2;
		vect_t		diff;
		fastf_t		dist_sq;

		VJOIN1_2D( hit1, p, dist[0], d );
		VJOIN1_2D( hit2, a, dist[1], c );
		VSUB2_2D( diff, hit1, hit2 );
		dist_sq = MAGSQ_2D( diff );
		if( dist_sq >= tol->dist_sq )  {
			if( rt_g.debug & DEBUG_MATH || dist_sq < 100*tol->dist_sq )  {
				rt_log("rt_isect_line2_line2(): dist=%g >%g, inconsistent solution, hit1=(%g,%g), hit2=(%g,%g)\n",
					sqrt(dist_sq), tol->dist,
					hit1[X], hit1[Y], hit2[X], hit2[Y]);
			}
			return -2;	/* s/b -1? */
		}
	}
#endif

	return 1;		/* Intersection found */
}

/*
 *			R T _ I S E C T _ L I N E 2 _ L S E G 2
 *
 *  Intersect a line in parametric form:
 *
 *	X = P + t * D
 *
 *  with a line segment defined by two distinct points A and B=(A+C).
 *
 *  XXX probably should take point B, not vector C.  Sigh.
 *
 *  Explicit Return -
 *	-4	A and B are not distinct points
 *	-3	Lines do not intersect
 *	-2	Intersection exists, but outside segemnt, < A
 *	-1	Intersection exists, but outside segment, > B
 *	 0	Lines are co-linear (special meaning of dist[1])
 *	 1	Intersection at vertex A
 *	 2	Intersection at vertex B (A+C)
 *	 3	Intersection between A and B
 *
 *  Implicit Returns -
 *	t	When explicit return >= 0, t is the parameter that describes
 *		the intersection of the line and the line segment.
 *		The actual intersection coordinates can be found by
 *		solving P + t * D.  However, note that for return codes
 *		1 and 2 (intersection exactly at a vertex), it is
 *		strongly recommended that the original values passed in
 *		A or B are used instead of solving P + t * D, to prevent
 *		numeric error from creeping into the position of
 *		the endpoints.
 */
int
rt_isect_line2_lseg2( dist, p, d, a, c, tol )
fastf_t			*dist;		/* dist[2] */
CONST point_t		p;
CONST vect_t		d;
CONST point_t		a;
CONST vect_t		c;
CONST struct rt_tol	*tol;
{
	register fastf_t f;
	fastf_t		ctol;
	int		ret;
	point_t		b;

	RT_CK_TOL(tol);
	if( rt_g.debug & DEBUG_MATH )  {
		rt_log("rt_isect_line2_lseg2() p=(%g,%g), pdir=(%g,%g)\n\t\t\ta=(%g,%g), adir=(%g,%g)\n",
			V2ARGS(p), V2ARGS(d), V2ARGS(a), V2ARGS(c) );
	}

	/*
	 *  To keep the values of u between 0 and 1,
	 *  C should NOT be scaled to have unit length.
	 *  However, it is a good idea to make sure that
	 *  C is a non-zero vector, (ie, that A and B are distinct).
	 */
	if( (ctol = MAGSQ_2D(c)) <= tol->dist_sq )  {
		ret = -4;		/* points A and B are not distinct */
		goto out;
	}

	/*
	 *  Detecting colinearity is difficult, and very very important.
	 *  As a first step, check to see if both points A and B lie
	 *  within tolerance of the line.  If so, then the line segment AC
	 *  is ON the line.
	 */
	VADD2_2D( b, a, c );
	if( rt_distsq_line2_point2( p, d, a ) <= tol->dist_sq  &&
	    (ctol=rt_distsq_line2_point2( p, d, b )) <= tol->dist_sq )  {
		if( rt_g.debug & DEBUG_MATH )  {
rt_log("b=(%g, %g), b_dist_sq=%g\n", V2ARGS(b), ctol);
			rt_log("rt_isect_line2_lseg2() pts A and B within tol of line\n");
		}
	    	/* Find the parametric distance along the ray */
	    	dist[0] = rt_dist_pt2_along_line2( p, d, a );
	    	dist[1] = rt_dist_pt2_along_line2( p, d, b );
	    	ret = 0;		/* Colinear */
	    	goto out;
	}

	if( (ret = rt_isect_line2_line2( dist, p, d, a, c, tol )) < 0 )  {
		/* Lines are parallel, non-colinear */
		ret = -3;		/* No intersection found */
		goto out;
	}
	if( ret == 0 )  {
		fastf_t	dtol;
		/*  Lines are colinear */
		/*  If P within tol of either endpoint (0, 1), make exact. */
		dtol = tol->dist / sqrt(MAGSQ_2D(d));
		if( rt_g.debug & DEBUG_MATH )  {
			rt_log("rt_isect_line2_lseg2() dtol=%g, dist[0]=%g, dist[1]=%g\n",
				dtol, dist[0], dist[1]);
		}
		if( dist[0] > -dtol && dist[0] < dtol )  dist[0] = 0;
		else if( dist[0] > 1-dtol && dist[0] < 1+dtol ) dist[0] = 1;

		if( dist[1] > -dtol && dist[1] < dtol )  dist[1] = 0;
		else if( dist[1] > 1-dtol && dist[1] < 1+dtol ) dist[1] = 1;
		ret = 0;		/* Colinear */
		goto out;
	}

	/*
	 *  The two lines are claimed to intersect at a point.
	 *  First, validate that hit point represented by dist[0]
	 *  is in fact on and between A--B.
	 *  (Nearly parallel lines can result in odd situations here).
	 *  The performance hit of doing this is vastly preferable
	 *  to returning wrong answers.  Know a faster algorithm?
	 */
	{
		fastf_t		ab_dist = 0;
		point_t		hit_pt;
		point_t		hit2;

		VJOIN1_2D( hit_pt, p, dist[0], d );
		VJOIN1_2D( hit2, a, dist[1], c );
		/* Check both hit point value calculations */
		if( rt_pt2_pt2_equal( a, hit_pt, tol ) ||
		    rt_pt2_pt2_equal( a, hit2, tol ) )  {
			dist[1] = 0;
			ret = 1;	/* Intersect is at A */
		}
		if( rt_pt2_pt2_equal( b, hit_pt, tol ) ||
		    rt_pt2_pt2_equal( b, hit_pt, tol ) )  {
			dist[1] = 1;
			ret = 2;	/* Intersect is at B */
		}

		ret = rt_isect_pt2_lseg2( &ab_dist, a, b, hit_pt, tol );
		if( rt_g.debug & DEBUG_MATH )  {
			/* XXX This is temporary */
			V2PRINT("a", a);
			V2PRINT("hit", hit_pt);
			V2PRINT("b", b);
rt_log("rt_isect_pt2_lseg2() hit2d=(%g,%g) ab_dist=%g, ret=%d\n", hit_pt[X], hit_pt[Y], ab_dist, ret);
rt_log("\tother hit2d=(%g,%g)\n", hit2[X], hit2[Y] );
		}
		if( ret <= 0 )  {
			if( ab_dist < 0 )  {
				ret = -2;	/* Intersection < A */
			} else {
				ret = -1;	/* Intersection >B */
			}
			goto out;
		}
		if( ret == 1 )  {
			dist[1] = 0;
			ret = 1;	/* Intersect is at A */
			goto out;
		}
		if( ret == 2 )  {
			dist[1] = 1;
			ret = 2;	/* Intersect is at B */
			goto out;
		}
		/* ret == 3, hit_pt is between A and B */

		if( !rt_between( a[X], hit_pt[X], b[X], tol ) ||
		    !rt_between( a[Y], hit_pt[Y], b[Y], tol ) ) {
		    	rt_bomb("rt_isect_line2_lseg2() hit_pt not between A and B!\n");
		}
	}

	/*
	 *  If the dist[1] parameter is outside the range (0..1),
	 *  reject the intersection, because it falls outside
	 *  the line segment A--B.
	 *
	 *  Convert the tol->dist into allowable deviation in terms of
	 *  (0..1) range of the parameters.
	 */
	ctol = tol->dist / sqrt(ctol);
	if( rt_g.debug & DEBUG_MATH )  {
		rt_log("rt_isect_line2_lseg2() ctol=%g, dist[1]=%g\n", ctol, dist[1]);
	}
	if( dist[1] < -ctol )  {
		ret = -2;		/* Intersection < A */
		goto out;
	}
	if( (f=(dist[1]-1)) > ctol )  {
		ret = -1;		/* Intersection > B */
		goto out;
	}

	/* Check for ctoly intersection with one of the verticies */
	if( dist[1] < ctol )  {
		dist[1] = 0;
		ret = 1;		/* Intersection at A */
		goto out;
	}
	if( f >= -ctol )  {
		dist[1] = 1;
		ret = 2;		/* Intersection at B */
		goto out;
	}
	ret = 3;			/* Intersection between A and B */
out:
	if( rt_g.debug & DEBUG_MATH )  {
		rt_log("rt_isect_line2_lseg2() dist[0]=%g, dist[1]=%g, ret=%d\n",
			dist[0], dist[1], ret);
	}
	return ret;
}

/*
 *			R T _ I S E C T _ L S E G 2  _ L S E G 2
 *
 *  Intersect two 2D line segments, defined by two points and two vectors.
 *  The vectors are unlikely to be unit length.
 *
 *  Explicit Return -
 *	-2	missed (line segments are parallel)
 *	-1	missed (colinear and non-overlapping)
 *	 0	hit (line segments colinear and overlapping)
 *	 1	hit (normal intersection)
 *
 *  Implicit Return -
 *	The value at dist[] is set to the parametric distance of the intercept
 *	dist[0] is parameter along p, range 0 to 1, to intercept.
 *	dist[1] is parameter along q, range 0 to 1, to intercept.
 *	If within distance tolerance of the endpoints, these will be
 *	exactly 0.0 or 1.0, to ease the job of caller.
 *
 *  Special note:  when return code is "0" for co-linearity, dist[1] has
 *  an alternate interpretation:  it's the parameter along p (not q)
 *  which takes you from point p to the point (q + qdir), i.e., it's
 *  the endpoint of the q linesegment, since in this case there may be
 *  *two* intersections, if q is contained within span p to (p + pdir).
 *  And either may be -10 if the point is outside the span.
 */
int
rt_isect_lseg2_lseg2( dist, p, pdir, q, qdir, tol )
fastf_t		*dist;
CONST point_t	p;
CONST vect_t	pdir;
CONST point_t	q;
CONST vect_t	qdir;
struct rt_tol	*tol;
{
	fastf_t	dx, dy;
	fastf_t	det;		/* determinant */
	fastf_t	det1, det2;
	fastf_t	b,c;
	fastf_t	hx, hy;		/* H = Q - P */
	fastf_t	ptol, qtol;	/* length in parameter space == tol->dist */
	int	status;

	RT_CK_TOL(tol);
	if( rt_g.debug & DEBUG_MATH )  {
		rt_log("rt_isect_lseg2_lseg2() p=(%g,%g), pdir=(%g,%g)\n\t\tq=(%g,%g), qdir=(%g,%g)\n",
			V2ARGS(p), V2ARGS(pdir), V2ARGS(q), V2ARGS(qdir) );
	}

	status = rt_isect_line2_line2( dist, p, pdir, q, qdir, tol );
	if( status < 0 )  {
		/* Lines are parallel, non-colinear */
		return -1;	/* No intersection */
	}
	if( status == 0 )  {
		int	nogood = 0;
		/* Lines are colinear */
		/*  If P within tol of either endpoint (0, 1), make exact. */
		ptol = tol->dist / sqrt( MAGSQ_2D(pdir) );
		if( rt_g.debug & DEBUG_MATH )  {
			rt_log("ptol=%g\n", ptol);
		}
		if( dist[0] > -ptol && dist[0] < ptol )  dist[0] = 0;
		else if( dist[0] > 1-ptol && dist[0] < 1+ptol ) dist[0] = 1;

		if( dist[1] > -ptol && dist[1] < ptol )  dist[1] = 0;
		else if( dist[1] > 1-ptol && dist[1] < 1+ptol ) dist[1] = 1;

		if( dist[1] < 0 || dist[1] > 1 )  nogood = 1;
		if( dist[0] < 0 || dist[0] > 1 )  nogood++;
		if( nogood >= 2 )
			return -1;	/* colinear, but not overlapping */
		if( rt_g.debug & DEBUG_MATH )  {
			rt_log("  HIT colinear!\n");
		}
		return 0;		/* colinear and overlapping */
	}
	/* Lines intersect */
	/*  If within tolerance of an endpoint (0, 1), make exact. */
	ptol = tol->dist / sqrt( MAGSQ_2D(pdir) );
	if( dist[0] > -ptol && dist[0] < ptol )  dist[0] = 0;
	else if( dist[0] > 1-ptol && dist[0] < 1+ptol ) dist[0] = 1;

	qtol = tol->dist / sqrt( MAGSQ_2D(qdir) );
	if( dist[1] > -qtol && dist[1] < qtol )  dist[1] = 0;
	else if( dist[1] > 1-qtol && dist[1] < 1+qtol ) dist[1] = 1;

	if( rt_g.debug & DEBUG_MATH )  {
		rt_log("ptol=%g, qtol=%g\n", ptol, qtol);
	}
	if( dist[0] < 0 || dist[0] > 1 || dist[1] < 0 || dist[1] > 1 ) {
		if( rt_g.debug & DEBUG_MATH )  {
			rt_log("  MISS\n");
		}
		return -1;		/* missed */
	}
	if( rt_g.debug & DEBUG_MATH )  {
		rt_log("  HIT!\n");
	}
	return 1;			/* hit, normal intersection */
}

/*
 *			R T _ I S E C T _ L S E G 3  _ L S E G 3
 *
 *  Intersect two 3D line segments, defined by two points and two vectors.
 *  The vectors are unlikely to be unit length.
 *
 *  Explicit Return -
 *	-2	missed (line segments are parallel)
 *	-1	missed (colinear and non-overlapping)
 *	 0	hit (line segments colinear and overlapping)
 *	 1	hit (normal intersection)
 *
 *  Implicit Return -
 *	The value at dist[] is set to the parametric distance of the intercept
 *	dist[0] is parameter along p, range 0 to 1, to intercept.
 *	dist[1] is parameter along q, range 0 to 1, to intercept.
 *	If within distance tolerance of the endpoints, these will be
 *	exactly 0.0 or 1.0, to ease the job of caller.
 *
 *  Special note:  when return code is "0" for co-linearity, dist[1] has
 *  an alternate interpretation:  it's the parameter along p (not q)
 *  which takes you from point p to the point (q + qdir), i.e., it's
 *  the endpoint of the q linesegment, since in this case there may be
 *  *two* intersections, if q is contained within span p to (p + pdir).
 *  And either may be -10 if the point is outside the span.
 */
int
rt_isect_lseg3_lseg3( dist, p, pdir, q, qdir, tol )
fastf_t		*dist;
CONST point_t	p;
CONST vect_t	pdir;
CONST point_t	q;
CONST vect_t	qdir;
struct rt_tol	*tol;
{
	fastf_t	dx, dy;
	fastf_t	det;		/* determinant */
	fastf_t	det1, det2;
	fastf_t	b,c;
	fastf_t	hx, hy;		/* H = Q - P */
	fastf_t	ptol, qtol;	/* length in parameter space == tol->dist */
	fastf_t	pmag, qmag;
	int	status;

	RT_CK_TOL(tol);
	if( rt_g.debug & DEBUG_MATH )  {
		rt_log("rt_isect_lseg3_lseg3() p=(%g,%g), pdir=(%g,%g)\n\t\tq=(%g,%g), qdir=(%g,%g)\n",
			V2ARGS(p), V2ARGS(pdir), V2ARGS(q), V2ARGS(qdir) );
	}

	status = rt_isect_line3_line3( &dist[0], &dist[1], p, pdir, q, qdir, tol );
	if( status < 0 )  {
		/* Lines are parallel, non-colinear */
		return -1;	/* No intersection */
	}
	pmag = MAGNITUDE(pdir);
	if( pmag < SQRT_SMALL_FASTF )
		rt_bomb("rt_isect_lseg3_lseg3: |p|=0\n");
	if( status == 0 )  {
		int	nogood = 0;
		/* Lines are colinear */
		/*  If P within tol of either endpoint (0, 1), make exact. */
		ptol = tol->dist / pmag;
		if( rt_g.debug & DEBUG_MATH )  {
			rt_log("ptol=%g\n", ptol);
		}
		if( dist[0] > -ptol && dist[0] < ptol )  dist[0] = 0;
		else if( dist[0] > 1-ptol && dist[0] < 1+ptol ) dist[0] = 1;

		if( dist[1] > -ptol && dist[1] < ptol )  dist[1] = 0;
		else if( dist[1] > 1-ptol && dist[1] < 1+ptol ) dist[1] = 1;

		if( dist[1] < 0 || dist[1] > 1 )  nogood = 1;
		if( dist[0] < 0 || dist[0] > 1 )  nogood++;
		if( nogood >= 2 )
			return -1;	/* colinear, but not overlapping */
		if( rt_g.debug & DEBUG_MATH )  {
			rt_log("  HIT colinear!\n");
		}
		return 0;		/* colinear and overlapping */
	}
	/* Lines intersect */
	/*  If within tolerance of an endpoint (0, 1), make exact. */
	ptol = tol->dist / pmag;
	if( dist[0] > -ptol && dist[0] < ptol )  dist[0] = 0;
	else if( dist[0] > 1-ptol && dist[0] < 1+ptol ) dist[0] = 1;

	qmag = MAGNITUDE(qdir);
	if( qmag < SQRT_SMALL_FASTF )
		rt_bomb("rt_isect_lseg3_lseg3: |q|=0\n");
	qtol = tol->dist / qmag;
	if( dist[1] > -qtol && dist[1] < qtol )  dist[1] = 0;
	else if( dist[1] > 1-qtol && dist[1] < 1+qtol ) dist[1] = 1;

	if( rt_g.debug & DEBUG_MATH )  {
		rt_log("ptol=%g, qtol=%g\n", ptol, qtol);
	}
	if( dist[0] < 0 || dist[0] > 1 || dist[1] < 0 || dist[1] > 1 ) {
		if( rt_g.debug & DEBUG_MATH )  {
			rt_log("  MISS\n");
		}
		return -1;		/* missed */
	}
	if( rt_g.debug & DEBUG_MATH )  {
		rt_log("  HIT!\n");
	}
	return 1;			/* hit, normal intersection */
}

/*
 *			R T _ I S E C T _ L I N E 3 _ L I N E 3
 *
 *  Intersect two lines, each in given in parametric form:
 *
 *	X = P + t * D
 *  and
 *	X = A + u * C
 *
 *  While the parametric form is usually used to denote a ray
 *  (ie, positive values of the parameter only), in this case
 *  the full line is considered.
 *
 *  The direction vectors C and D need not have unit length.
 *
 *  Explicit Return -
 *	-2	no intersection, lines are parallel.
 *	-1	no intersection
 *	 0	lines are co-linear (t returned for u=0 to give distance to A)
 *	 1	intersection found (t and u returned)
 *
 *  Implicit Returns -
 *
 *	t,u	When explicit return >= 0, t and u are the
 *		line parameters of the intersection point on the 2 rays.
 *		The actual intersection coordinates can be found by
 *		substituting either of these into the original ray equations.
 *
 * XXX It would be sensible to change the t,u pair to dist[2].
 */
int
rt_isect_line3_line3( t, u, p, d, a, c, tol )
fastf_t			*t;
fastf_t			*u;
CONST point_t		p;
CONST vect_t		d;
CONST point_t		a;
CONST vect_t		c;
CONST struct rt_tol	*tol;
{
	LOCAL vect_t		n;
	LOCAL vect_t		abs_n;
	LOCAL vect_t		h;
	register fastf_t	det;
	register fastf_t	det1;
	register short int	q,r,s;

	RT_CK_TOL(tol);

	/*
	 *  Any intersection will occur in the plane with surface
	 *  normal D cross C, which may not have unit length.
	 *  The plane containing the two lines will be a constant
	 *  distance from a plane with the same normal that contains
	 *  the origin.  Therfore, the projection of any point on the
	 *  plane along N has the same length.
	 *  Verify that this holds for P and A.
	 *  If N dot P != N dot A, there is no intersection, because
	 *  P and A must lie on parallel planes that are different
	 *  distances from the origin.
	 */
	VCROSS( n, d, c );
	det = VDOT( n, p ) - VDOT( n, a );
	if( !NEAR_ZERO( det, tol->dist ) )  {
		return(-1);		/* No intersection */
	}

	/*
	 *  Solve for t and u in the system:
	 *
	 *	Px + t * Dx = Ax + u * Cx
	 *	Py + t * Dy = Ay + u * Cy
	 *	Pz + t * Dz = Az + u * Cz
	 *
	 *  This system is over-determined, having 3 equations in 2 unknowns.
	 *  However, the intersection problem is really only a 2-dimensional
	 *  problem, being located in the surface of a plane.
	 *  Therefore, the "least important" of these equations can
	 *  be initially ignored, leaving a system of 2 equations in
	 *  2 unknowns.
	 *
	 *  Find the component of N with the largest magnitude.
	 *  This component will have the least effect on the parameters
	 *  in the system, being most nearly perpendicular to the plane.
	 *  Denote the two remaining components by the
	 *  subscripts q and r, rather than x,y,z.
	 *  Subscript s is the smallest component, used for checking later.
	 */
	abs_n[X] = (n[X] >= 0) ? n[X] : (-n[X]);
	abs_n[Y] = (n[Y] >= 0) ? n[Y] : (-n[Y]);
	abs_n[Z] = (n[Z] >= 0) ? n[Z] : (-n[Z]);
	if( abs_n[X] >= abs_n[Y] )  {
		if( abs_n[X] >= abs_n[Z] )  {
			/* X is largest in magnitude */
			q = Y;
			r = Z;
			s = X;
		} else {
			/* Z is largest in magnitude */
			q = X;
			r = Y;
			s = Z;
		}
	} else {
		if( abs_n[Y] >= abs_n[Z] )  {
			/* Y is largest in magnitude */
			q = X;
			r = Z;
			s = Y;
		} else {
			/* Z is largest in magnitude */
			q = X;
			r = Y;
			s = Z;
		}
	}

#if 0
	/* XXX Use rt_isect_line2_line2() here */
	/* move the 2d vectors around */
	rt_isect_line2_line2( &dist, p, d, a, c, tol );
#endif

	/*
	 *  From the two components q and r, form a system
	 *  of 2 equations in 2 unknowns:
	 *
	 *	Pq + t * Dq = Aq + u * Cq
	 *	Pr + t * Dr = Ar + u * Cr
	 *  or
	 *	t * Dq - u * Cq = Aq - Pq
	 *	t * Dr - u * Cr = Ar - Pr
	 *
	 *  Let H = A - P, resulting in:
	 *
	 *	t * Dq - u * Cq = Hq
	 *	t * Dr - u * Cr = Hr
	 *
	 *  or
	 *
	 *	[ Dq  -Cq ]   [ t ]   [ Hq ]
	 *	[         ] * [   ] = [    ]
	 *	[ Dr  -Cr ]   [ u ]   [ Hr ]
	 *
	 *  This system can be solved by direct substitution, or by
	 *  finding the determinants by Cramers rule:
	 *
	 *	             [ Dq  -Cq ]
	 *	det(M) = det [         ] = -Dq * Cr + Cq * Dr
	 *	             [ Dr  -Cr ]
	 *
	 *  If det(M) is zero, then the lines are parallel (perhaps colinear).
	 *  Otherwise, exactly one solution exists.
	 */
	VSUB2( h, a, p );
	det = c[q] * d[r] - d[q] * c[r];
	det1 = (c[q] * h[r] - h[q] * c[r]);		/* see below */
	/* XXX This should be no smaller than 1e-16.  See rt_isect_line2_line2 for details */
	if( NEAR_ZERO( det, DETERMINANT_TOL ) )  {
		/* Lines are parallel */
		if( !NEAR_ZERO( det1, DETERMINANT_TOL ) )  {
			/* Lines are NOT co-linear, just parallel */
			return -2;	/* parallel, no intersection */
		}

		/* Lines are co-linear */
		/* Compute t for u=0 as a convenience to caller */
		*u = 0;
		/* Use largest direction component */
		if( fabs(d[q]) >= fabs(d[r]) )  {
			*t = h[q]/d[q];
		} else {
			*t = h[r]/d[r];
		}
		return(0);	/* Lines co-linear */
	}

	/*
	 *  det(M) is non-zero, so there is exactly one solution.
	 *  Using Cramer's rule, det1(M) replaces the first column
	 *  of M with the constant column vector, in this case H.
	 *  Similarly, det2(M) replaces the second column.
	 *  Computation of the determinant is done as before.
	 *
	 *  Now,
	 *
	 *	                  [ Hq  -Cq ]
	 *	              det [         ]
	 *	    det1(M)       [ Hr  -Cr ]   -Hq * Cr + Cq * Hr
	 *	t = ------- = --------------- = ------------------
	 *	     det(M)       det(M)        -Dq * Cr + Cq * Dr
	 *
	 *  and
	 *
	 *	                  [ Dq   Hq ]
	 *	              det [         ]
	 *	    det2(M)       [ Dr   Hr ]    Dq * Hr - Hq * Dr
	 *	u = ------- = --------------- = ------------------
	 *	     det(M)       det(M)        -Dq * Cr + Cq * Dr
	 */
	det = 1/det;
	*t = det * det1;
	*u = det * (d[q] * h[r] - h[q] * d[r]);

	/*
	 *  Check that these values of t and u satisfy the 3rd equation
	 *  as well!
	 *  XXX It isn't clear that "det" is exactly a model-space distance.
	 */
	det = *t * d[s] - *u * c[s] - h[s];
	if( !NEAR_ZERO( det, tol->dist ) )  {
		/* XXX This tolerance needs to be much less loose than
		 * XXX SQRT_SMALL_FASTF.  What about DETERMINANT_TOL?
		 */
		/* Inconsistent solution, lines miss each other */
		return(-1);
	}

	/*  To prevent errors, check the answer.
	 *  Not returning bogus results to our caller is worth the extra time.
	 */
	{
		point_t		hit1, hit2;

		VJOIN1( hit1, p, *t, d );
		VJOIN1( hit2, a, *u, c );
		if( !rt_pt3_pt3_equal( hit1, hit2, tol ) )  {
			rt_log("rt_isect_line3_line3(): BOGUS RESULT, hit1=(%g,%g,%g), hit2=(%g,%g,%g)\n",
				hit1[X], hit1[Y], hit1[Z], hit2[X], hit2[Y], hit2[Z]);
			return -1;
		}
	}

	return(1);		/* Intersection found */
}

/*
 *			R T _ I S E C T _ L I N E _ L S E G
 *
 *  Intersect a line in parametric form:
 *
 *	X = P + t * D
 *
 *  with a line segment defined by two distinct points A and B.
 *
 *  Explicit Return -
 *	-4	A and B are not distinct points
 *	-3	Intersection exists, < A (t is returned)
 *	-2	Intersection exists, > B (t is returned)
 *	-1	Lines do not intersect
 *	 0	Lines are co-linear (t for A is returned)
 *	 1	Intersection at vertex A
 *	 2	Intersection at vertex B
 *	 3	Intersection between A and B
 *
 *  Implicit Returns -
 *	t	When explicit return >= 0, t is the parameter that describes
 *		the intersection of the line and the line segment.
 *		The actual intersection coordinates can be found by
 *		solving P + t * D.  However, note that for return codes
 *		1 and 2 (intersection exactly at a vertex), it is
 *		strongly recommended that the original values passed in
 *		A or B are used instead of solving P + t * D, to prevent
 *		numeric error from creeping into the position of
 *		the endpoints.
 */
/* XXX should probably be called rt_isect_line3_lseg3() */
/* XXX should probably be changed to return dist[2] */
int
rt_isect_line_lseg( t, p, d, a, b, tol )
fastf_t			*t;
CONST point_t		p;
CONST vect_t		d;
CONST point_t		a;
CONST point_t		b;
CONST struct rt_tol	*tol;
{
	LOCAL vect_t	c;		/* Direction vector from A to B */
	auto fastf_t	u;		/* As in, A + u * C = X */
	register fastf_t f;
	register int	ret;
	fastf_t		fuzz;

	RT_CK_TOL(tol);

	VSUB2( c, b, a );
	/*
	 *  To keep the values of u between 0 and 1,
	 *  C should NOT be scaled to have unit length.
	 *  However, it is a good idea to make sure that
	 *  C is a non-zero vector, (ie, that A and B are distinct).
	 */
	if( (fuzz = MAGSQ(c)) < tol->dist_sq )  {
		return(-4);		/* points A and B are not distinct */
	}

	/*
	 *  Detecting colinearity is difficult, and very very important.
	 *  As a first step, check to see if both points A and B lie
	 *  within tolerance of the line.  If so, then the line segment AC
	 *  is ON the line.
	 */
	if( rt_distsq_line3_pt3( p, d, a ) <= tol->dist_sq  &&
	    rt_distsq_line3_pt3( p, d, b ) <= tol->dist_sq )  {
		if( rt_g.debug & DEBUG_MATH )  {
			rt_log("rt_isect_line3_lseg3() pts A and B within tol of line\n");
		}
	    	/* Find the parametric distance along the ray */
		*t = rt_dist_pt3_along_line3( p, d, a );
		/*** dist[1] = rt_dist_pt3_along_line3( p, d, b ); ***/
		return 0;		/* Colinear */
	}

	if( (ret = rt_isect_line3_line3( t, &u, p, d, a, c, tol )) < 0 )  {
		/* No intersection found */
		return( -1 );
	}
	if( ret == 0 )  {
		/* co-linear (t was computed for point A, u=0) */
		return( 0 );
	}

	/*
	 *  The two lines intersect at a point.
	 *  If the u parameter is outside the range (0..1),
	 *  reject the intersection, because it falls outside
	 *  the line segment A--B.
	 *
	 *  Convert the tol->dist into allowable deviation in terms of
	 *  (0..1) range of the parameters.
	 */
	fuzz = tol->dist / sqrt(fuzz);
	if( u < -fuzz )
		return(-3);		/* Intersection < A */
	if( (f=(u-1)) > fuzz )
		return(-2);		/* Intersection > B */

	/* Check for fuzzy intersection with one of the verticies */
	if( u < fuzz )
		return( 1 );		/* Intersection at A */
	if( f >= -fuzz )
		return( 2 );		/* Intersection at B */

	return(3);			/* Intersection between A and B */
}

/*
 *			R T _ D I S T _ L I N E 3_ P T 3
 *
 *  Given a parametric line defined by PT + t * DIR and a point A,
 *  return the closest distance between the line and the point.
 *
 *  'dir' need not have unit length.
 *
 *  Return -
 *	Distance
 */
double
rt_dist_line3_pt3( pt, dir, a )
CONST point_t	pt;
CONST vect_t	dir;
CONST point_t	a;
{
	LOCAL vect_t		f;
	register fastf_t	FdotD;

	VSUB2( f, pt, a );
	if( (FdotD = MAGNITUDE(dir)) <= SMALL_FASTF )
		return 0.0;
	FdotD = VDOT( f, dir ) / FdotD;
	if( (FdotD = VDOT( f, f ) - FdotD * FdotD ) <= SMALL_FASTF )
		return(0.0);
	return( sqrt(FdotD) );
}

/*
 *			R T _ D I S T S Q _ L I N E 3 _ P T 3
 *
 *  Given a parametric line defined by PT + t * DIR and a point A,
 *  return the square of the closest distance between the line and the point.
 *
 *  'dir' need not have unit length.
 *
 *  Return -
 *	Distance squared
 */
double
rt_distsq_line3_pt3( pt, dir, a )
CONST point_t	pt;
CONST vect_t	dir;
CONST point_t	a;
{
	LOCAL vect_t		f;
	register fastf_t	FdotD;

	VSUB2( f, pt, a );
	if( (FdotD = MAGNITUDE(dir)) <= SMALL_FASTF )
		return 0.0;
	FdotD = VDOT( f, dir ) / FdotD;
	if( (FdotD = VDOT( f, f ) - FdotD * FdotD ) <= SMALL_FASTF )
		return(0.0);
	return FdotD;
}

/*
 *			R T _ D I S T _ L I N E _ O R I G I N
 *
 *  Given a parametric line defined by PT + t * DIR,
 *  return the closest distance between the line and the origin.
 *
 *  'dir' need not have unit length.
 *
 *  Return -
 *	Distance
 */
double
rt_dist_line_origin( pt, dir )
CONST point_t	pt;
CONST vect_t	dir;
{
	register fastf_t	PTdotD;

	if( (PTdotD = MAGNITUDE(dir)) <= SMALL_FASTF )
		return 0.0;
	PTdotD = VDOT( pt, dir ) / PTdotD;
	if( (PTdotD = VDOT( pt, pt ) - PTdotD * PTdotD ) <= SMALL_FASTF )
		return(0.0);
	return( sqrt(PTdotD) );
}
/*
 *			R T _ D I S T _ L I N E 2 _ P O I N T 2
 *
 *  Given a parametric line defined by PT + t * DIR and a point A,
 *  return the closest distance between the line and the point.
 *
 *  'dir' need not have unit length.
 *
 *  Return -
 *	Distance
 */
double
rt_dist_line2_point2( pt, dir, a )
CONST point_t	pt;
CONST vect_t	dir;
CONST point_t	a;
{
	LOCAL vect_t		f;
	register fastf_t	FdotD;

	VSUB2_2D( f, pt, a );
	if( (FdotD = sqrt(MAGSQ_2D(dir))) <= SMALL_FASTF )
		return 0.0;
	FdotD = VDOT_2D( f, dir ) / FdotD;
	if( (FdotD = VDOT_2D( f, f ) - FdotD * FdotD ) <= SMALL_FASTF )
		return(0.0);
	return( sqrt(FdotD) );
}

/*
 *			R T _ D I S T S Q _ L I N E 2 _ P O I N T 2
 *
 *  Given a parametric line defined by PT + t * DIR and a point A,
 *  return the closest distance between the line and the point, squared.
 *
 *  'dir' need not have unit length.
 *
 *  Return -
 *	Distance squared
 */
double
rt_distsq_line2_point2( pt, dir, a )
CONST point_t	pt;
CONST vect_t	dir;
CONST point_t	a;
{
	LOCAL vect_t		f;
	register fastf_t	FdotD;

	VSUB2_2D( f, pt, a );
	if( (FdotD = sqrt(MAGSQ_2D(dir))) <= SMALL_FASTF )
		return 0.0;
	FdotD = VDOT_2D( f, dir ) / FdotD;
	if( (FdotD = VDOT_2D( f, f ) - FdotD * FdotD ) <= SMALL_FASTF )
		return(0.0);
	return( FdotD );
}

/*
 *			R T _ A R E A _ O F _ T R I A N G L E
 *
 *  Returns the area of a triangle.
 *  Algorithm by Jon Leech 3/24/89.
 */
double
rt_area_of_triangle( a, b, c )
register CONST point_t a, b, c;
{
	register double	t;
	register double	area;

	t =	a[Y] * (b[Z] - c[Z]) -
		b[Y] * (a[Z] - c[Z]) +
		c[Y] * (a[Z] - b[Z]);
	area  = t*t;
	t =	a[Z] * (b[X] - c[X]) -
		b[Z] * (a[X] - c[X]) +
		c[Z] * (a[X] - b[X]);
	area += t*t;
	t = 	a[X] * (b[Y] - c[Y]) -
		b[X] * (a[Y] - c[Y]) +
		c[X] * (a[Y] - b[Y]);
	area += t*t;

	return( 0.5 * sqrt(area) );
}


/*
 *			R T _ I S E C T _ P T _ L S E G
 *
 * Intersect a point P with the line segment defined by two distinct
 * points A and B.
 *	
 * Explicit Return
 *	-2	P on line AB but outside range of AB,
 *			dist = distance from A to P on line.
 *	-1	P not on line of AB within tolerance
 *	1	P is at A
 *	2	P is at B
 *	3	P is on AB, dist = distance from A to P on line.
 *	
 *    B *
 *	|  
 *    P'*-tol-*P 
 *	|    /  _
 *    dist  /   /|
 *	|  /   /
 *	| /   / AtoP
 *	|/   /
 *    A *   /
 *	
 *	tol = distance limit from line to pt P;
 *	dist = distance from A to P'
 */
int rt_isect_pt_lseg(dist, a, b, p, tol)
fastf_t			*dist;		/* distance along line from A to P */
CONST point_t		a, b, p;	/* points for line and intersect */
CONST struct rt_tol	*tol;
{
	vect_t	AtoP,
		BtoP,
		AtoB,
		ABunit;	/* unit vector from A to B */
	fastf_t	APprABunit;	/* Mag of projection of AtoP onto ABunit */
	fastf_t	distsq;

	RT_CK_TOL(tol);

	VSUB2(AtoP, p, a);
	if (MAGSQ(AtoP) < tol->dist_sq)
		return(1);	/* P at A */

	VSUB2(BtoP, p, b);
	if (MAGSQ(BtoP) < tol->dist_sq)
		return(2);	/* P at B */

	VSUB2(AtoB, b, a);
	VMOVE(ABunit, AtoB);
	distsq = MAGSQ(ABunit);
	if( distsq < tol->dist_sq )
		return -1;	/* A equals B, and P isn't there */
	distsq = 1/sqrt(distsq);
	VSCALE( ABunit, ABunit, distsq );

	/* Similar to rt_dist_line_pt, except we
	 * never actually have to do the sqrt that the other routine does.
	 */

	/* find dist as a function of ABunit, actually the projection
	 * of AtoP onto ABunit
	 */
	APprABunit = VDOT(AtoP, ABunit);

	/* because of pythgorean theorem ... */
	distsq = MAGSQ(AtoP) - APprABunit * APprABunit;
	if (distsq > tol->dist_sq)
		return(-1);	/* dist pt to line too large */

	/* Distance from the point to the line is within tolerance. */
	*dist = VDOT(AtoP, AtoB) / MAGSQ(AtoB);

	if (*dist > 1.0 || *dist < 0.0)	/* P outside AtoB */
		return(-2);

	return(3);	/* P on AtoB */
}

/*
 *			R T _ I S E C T _ P T 2 _ L S E G 2
 *
 * Intersect a point P with the line segment defined by two distinct
 * points A and B.
 *	
 * Explicit Return
 *	-2	P on line AB but outside range of AB,
 *			dist = distance from A to P on line.
 *	-1	P not on line of AB within tolerance
 *	1	P is at A
 *	2	P is at B
 *	3	P is on AB, dist = distance from A to P on line.
 *	
 *    B *
 *	|  
 *    P'*-tol-*P 
 *	|    /  _
 *    dist  /   /|
 *	|  /   /
 *	| /   / AtoP
 *	|/   /
 *    A *   /
 *	
 *	tol = distance limit from line to pt P;
 *	dist = distance from A to P'
 */
int
rt_isect_pt2_lseg2(dist, a, b, p, tol)
fastf_t			*dist;		/* distance along line from A to P */
CONST point_t		a, b, p;	/* points for line and intersect */
CONST struct rt_tol	*tol;
{
	vect_t	AtoP,
		BtoP,
		AtoB,
		ABunit;	/* unit vector from A to B */
	fastf_t	APprABunit;	/* Mag of projection of AtoP onto ABunit */
	fastf_t	distsq;

	RT_CK_TOL(tol);

	VSUB2_2D(AtoP, p, a);
	if (MAGSQ_2D(AtoP) < tol->dist_sq)
		return(1);	/* P at A */

	VSUB2_2D(BtoP, p, b);
	if (MAGSQ_2D(BtoP) < tol->dist_sq)
		return(2);	/* P at B */

	VSUB2_2D(AtoB, b, a);
	VMOVE_2D(ABunit, AtoB);
	distsq = MAGSQ_2D(ABunit);
	if( distsq < tol->dist_sq )  {
		if( rt_g.debug & DEBUG_MATH )  {
			rt_log("distsq A=%g\n", distsq);
		}
		return -1;	/* A equals B, and P isn't there */
	}
	distsq = 1/sqrt(distsq);
	VSCALE_2D( ABunit, ABunit, distsq );

	/* Similar to rt_dist_line_pt, except we
	 * never actually have to do the sqrt that the other routine does.
	 */

	/* find dist as a function of ABunit, actually the projection
	 * of AtoP onto ABunit
	 */
	APprABunit = VDOT_2D(AtoP, ABunit);

	/* because of pythgorean theorem ... */
	distsq = MAGSQ_2D(AtoP) - APprABunit * APprABunit;
	if (distsq > tol->dist_sq) {
		if( rt_g.debug & DEBUG_MATH )  {
			VPRINT("ABunit", ABunit);
			rt_log("distsq B=%g\n", distsq);
		}
		return(-1);	/* dist pt to line too large */
	}

	/* Distance from the point to the line is within tolerance. */
	*dist = VDOT_2D(AtoP, AtoB) / MAGSQ_2D(AtoB);

	if (*dist > 1.0 || *dist < 0.0)	/* P outside AtoB */
		return(-2);

	return(3);	/* P on AtoB */
}

/*
 *			R T _ D I S T _ P T 3 _ L S E G 3
 *
 *  Find the distance from a point P to a line segment described
 *  by the two endpoints A and B, and the point of closest approach (PCA).
 *
 *			P
 *		       *
 *		      /.
 *		     / .
 *		    /  .
 *		   /   . (dist)
 *		  /    .
 *		 /     .
 *		*------*--------*
 *		A      PCA	B
 *
 *  There are six distinct cases, with these return codes -
 *	0	P is within tolerance of lseg AB.  *dist isn't 0: (SPECIAL!!!)
 *		  *dist = parametric dist = |PCA-A| / |B-A|.  pca=computed.
 *	1	P is within tolerance of point A.  *dist = 0, pca=A.
 *	2	P is within tolerance of point B.  *dist = 0, pca=B.
 *	3	P is to the "left" of point A.  *dist=|P-A|, pca=A.
 *	4	P is to the "right" of point B.  *dist=|P-B|, pca=B.
 *	5	P is "above/below" lseg AB.  *dist=|PCA-P|, pca=computed.
 *
 * This routine was formerly called rt_dist_pt_lseg().
 *
 * XXX For efficiency, a version of this routine that provides the
 * XXX distance squared would be faster.
 */
int
rt_dist_pt3_lseg3( dist, pca, a, b, p, tol )
fastf_t		*dist;
point_t		pca;
CONST point_t	a, b, p;
CONST struct rt_tol *tol;
{
	vect_t	PtoA;		/* P-A */
	vect_t	PtoB;		/* P-B */
	vect_t	AtoB;		/* B-A */
	fastf_t	P_A_sq;		/* |P-A|**2 */
	fastf_t	P_B_sq;		/* |P-B|**2 */
	fastf_t	B_A;		/* |B-A| */
	fastf_t	B_A_sq;
	fastf_t	t;		/* distance along ray of projection of P */

	RT_CK_TOL(tol);

	if( rt_g.debug & DEBUG_MATH )  {
		rt_log("rt_dist_pt3_lseg3() a=(%g,%g,%g) b=(%g,%g,%g)\n\tp=(%g,%g,%g), tol->dist=%g sq=%g\n",
			V3ARGS(a),
			V3ARGS(b),
			V3ARGS(p),
			tol->dist, tol->dist_sq );
	}

	/* Check proximity to endpoint A */
	VSUB2(PtoA, p, a);
	if( (P_A_sq = MAGSQ(PtoA)) < tol->dist_sq )  {
		/* P is within the tol->dist radius circle around A */
		VMOVE( pca, a );
		if( rt_g.debug & DEBUG_MATH )  rt_log("  at A\n");
		*dist = 0.0;
		return 1;
	}

	/* Check proximity to endpoint B */
	VSUB2(PtoB, p, b);
	if( (P_B_sq = MAGSQ(PtoB)) < tol->dist_sq )  {
		/* P is within the tol->dist radius circle around B */
		VMOVE( pca, b );
		if( rt_g.debug & DEBUG_MATH )  rt_log("  at B\n");
		*dist = 0.0;
		return 2;
	}

	VSUB2(AtoB, b, a);
	B_A = sqrt( B_A_sq = MAGSQ(AtoB) );

	/* compute distance (in actual units) along line to PROJECTION of
	 * point p onto the line: point pca
	 */
	t = VDOT(PtoA, AtoB) / B_A;
	if( rt_g.debug & DEBUG_MATH )  {
		rt_log("rt_dist_pt3_lseg3() B_A=%g, t=%g\n",
			B_A, t );
	}

	if( t <= 0 )  {
		/* P is "left" of A */
		if( rt_g.debug & DEBUG_MATH )  rt_log("  left of A\n");
		VMOVE( pca, a );
		*dist = sqrt(P_A_sq);
		return 3;
	}
	if( t < B_A )  {
		/* PCA falls between A and B */
		register fastf_t	dsq;
		fastf_t			param_dist;	/* parametric dist */

		/* Find PCA */
		param_dist = t / B_A;		/* Range 0..1 */
		VJOIN1(pca, a, param_dist, AtoB);

		/* Find distance from PCA to line segment (Pythagorus) */
		if( (dsq = P_A_sq - t * t ) <= tol->dist_sq )  {
			if( rt_g.debug & DEBUG_MATH )  rt_log("  ON lseg\n");
			/* Distance from PCA to lseg is zero, give param instead */
			*dist = param_dist;	/* special! */
			return 0;
		}
		if( rt_g.debug & DEBUG_MATH )  rt_log("  closest to lseg\n");
		*dist = sqrt(dsq);
		return 5;
	}
	/* P is "right" of B */
	if( rt_g.debug & DEBUG_MATH )  rt_log("  right of B\n");
	VMOVE(pca, b);
	*dist = sqrt(P_B_sq);
	return 4;
}

/*
 *			R T _ D I S T _ P T 2 _ L S E G 2
 *
 *  Find the distance from a point P to a line segment described
 *  by the two endpoints A and B, and the point of closest approach (PCA).
 *
 *			P
 *		       *
 *		      /.
 *		     / .
 *		    /  .
 *		   /   . (dist)
 *		  /    .
 *		 /     .
 *		*------*--------*
 *		A      PCA	B
 *
 *  There are six distinct cases, with these return codes -
 *	0	P is within tolerance of lseg AB.  *dist isn't 0: (SPECIAL!!!)
 *		  *dist = parametric dist = |PCA-A| / |B-A|.  pca=computed.
 *	1	P is within tolerance of point A.  *dist = 0, pca=A.
 *	2	P is within tolerance of point B.  *dist = 0, pca=B.
 *	3	P is to the "left" of point A.  *dist=|P-A|**2, pca=A.
 *	4	P is to the "right" of point B.  *dist=|P-B|**2, pca=B.
 *	5	P is "above/below" lseg AB.  *dist=|PCA-P|**2, pca=computed.
 *
 *
 *  Patterned after rt_dist_pt3_lseg3().
 */
int
rt_dist_pt2_lseg2( dist_sq, pca, a, b, p, tol )
fastf_t		*dist_sq;
point_t		pca;
CONST point_t	a, b, p;
CONST struct rt_tol *tol;
{
	vect_t	PtoA;		/* P-A */
	vect_t	PtoB;		/* P-B */
	vect_t	AtoB;		/* B-A */
	fastf_t	P_A_sq;		/* |P-A|**2 */
	fastf_t	P_B_sq;		/* |P-B|**2 */
	fastf_t	B_A;		/* |B-A| */
	fastf_t	B_A_sq;
	fastf_t	t;		/* distance along ray of projection of P */

	RT_CK_TOL(tol);

	if( rt_g.debug & DEBUG_MATH )  {
		rt_log("rt_dist_pt3_lseg3() a=(%g,%g,%g) b=(%g,%g,%g)\n\tp=(%g,%g,%g), tol->dist=%g sq=%g\n",
			V3ARGS(a),
			V3ARGS(b),
			V3ARGS(p),
			tol->dist, tol->dist_sq );
	}


	/* Check proximity to endpoint A */
	VSUB2_2D(PtoA, p, a);
	if( (P_A_sq = MAGSQ_2D(PtoA)) < tol->dist_sq )  {
		/* P is within the tol->dist radius circle around A */
		VMOVE( pca, a );
		if( rt_g.debug & DEBUG_MATH )  rt_log("  at A\n");
		*dist_sq = 0.0;
		return 1;
	}

	/* Check proximity to endpoint B */
	VSUB2_2D(PtoB, p, b);
	if( (P_B_sq = MAGSQ_2D(PtoB)) < tol->dist_sq )  {
		/* P is within the tol->dist radius circle around B */
		VMOVE( pca, b );
		if( rt_g.debug & DEBUG_MATH )  rt_log("  at B\n");
		*dist_sq = 0.0;
		return 2;
	}

	VSUB2_2D(AtoB, b, a);
	B_A = sqrt( B_A_sq = MAGSQ_2D(AtoB) );

	/* compute distance (in actual units) along line to PROJECTION of
	 * point p onto the line: point pca
	 */
	t = VDOT_2D(PtoA, AtoB) / B_A;
	if( rt_g.debug & DEBUG_MATH )  {
		rt_log("rt_dist_pt3_lseg3() B_A=%g, t=%g\n",
			B_A, t );
	}

	if( t <= 0 )  {
		/* P is "left" of A */
		if( rt_g.debug & DEBUG_MATH )  rt_log("  left of A\n");
		VMOVE( pca, a );
		*dist_sq = P_A_sq;
		return 3;
	}
	if( t < B_A )  {
		/* PCA falls between A and B */
		register fastf_t	dsq;
		fastf_t			param_dist;	/* parametric dist */

		/* Find PCA */
		param_dist = t / B_A;		/* Range 0..1 */
		VJOIN1(pca, a, param_dist, AtoB);

		/* Find distance from PCA to line segment (Pythagorus) */
		if( (dsq = P_A_sq - t * t ) <= tol->dist_sq )  {
			if( rt_g.debug & DEBUG_MATH )  rt_log("  ON lseg\n");
			/* Distance from PCA to lseg is zero, give param instead */
			*dist_sq = param_dist;	/* special! Not squared. */
			return 0;
		}
		if( rt_g.debug & DEBUG_MATH )  rt_log("  closest to lseg\n");
		*dist_sq = dsq;
		return 5;
	}
	/* P is "right" of B */
	if( rt_g.debug & DEBUG_MATH )  rt_log("  right of B\n");
	VMOVE(pca, b);
	*dist_sq = P_B_sq;
	return 4;
}

/*
 *			R T _ R O T A T E _ B B O X
 *
 *  Transform a bounding box (RPP) by the given 4x4 matrix.
 *  There are 8 corners to the bounding RPP.
 *  Each one needs to be transformed and min/max'ed.
 *  This is not minimal, but does fully contain any internal object,
 *  using an axis-aligned RPP.
 */
void
rt_rotate_bbox( omin, omax, mat, imin, imax )
point_t		omin;
point_t		omax;
CONST mat_t	mat;
CONST point_t	imin;
CONST point_t	imax;
{
	point_t	local;		/* vertex point in local coordinates */
	point_t	model;		/* vertex point in model coordinates */

#define ROT_VERT( a, b, c )  \
	VSET( local, a[X], b[Y], c[Z] ); \
	MAT4X3PNT( model, mat, local ); \
	VMINMAX( omin, omax, model ) \

	ROT_VERT( imin, imin, imin );
	ROT_VERT( imin, imin, imax );
	ROT_VERT( imin, imax, imin );
	ROT_VERT( imin, imax, imax );
	ROT_VERT( imax, imin, imin );
	ROT_VERT( imax, imin, imax );
	ROT_VERT( imax, imax, imin );
	ROT_VERT( imax, imax, imax );
#undef ROT_VERT
}

/*
 *			R T _ R O T A T E _ P L A N E
 *
 *  Transform a plane equation by the given 4x4 matrix.
 */
void
rt_rotate_plane( oplane, mat, iplane )
plane_t		oplane;
CONST mat_t	mat;
CONST plane_t	iplane;
{
	point_t		orig_pt;
	point_t		new_pt;

	/* First, pick a point that lies on the original halfspace */
	VSCALE( orig_pt, iplane, iplane[3] );

	/* Transform the surface normal */
	MAT4X3VEC( oplane, mat, iplane );

	/* Transform the point from original to new halfspace */
	MAT4X3PNT( new_pt, mat, orig_pt );

	/*
	 *  The transformed normal is all that is required.
	 *  The new distance is found from the transformed point on the plane.
	 */
	oplane[3] = VDOT( new_pt, oplane );
}

/*
 *			R T _ C O P L A N A R
 *
 *  Test if two planes are identical.  If so, their dot products will be
 *  either +1 or -1, with the distance from the origin equal in magnitude.
 *
 *  Returns -
 *	-1	not coplanar, parallel but distinct
 *	 0	not coplanar, not parallel.  Planes intersect.
 *	+1	coplanar, same normal direction
 *	+2	coplanar, opposite normal direction
 */
int
rt_coplanar( a, b, tol )
CONST plane_t		a;
CONST plane_t		b;
CONST struct rt_tol	*tol;
{
	register fastf_t	f;
	register fastf_t	dot;

	RT_CK_TOL(tol);

	/* Check to see if the planes are parallel */
	dot = VDOT( a, b );
	if( dot >= 0 )  {
		/* Normals head in generally the same directions */
		if( dot < tol->para )
			return(0);	/* Planes intersect */

		/* Planes have "exactly" the same normal vector */
		f = a[3] - b[3];
		if( NEAR_ZERO( f, tol->dist ) )  {
			return(1);	/* Coplanar, same direction */
		}
		return(-1);		/* Parallel but distinct */
	}
	/* Normals head in generally opposite directions */
	if( -dot < tol->para )
		return(0);		/* Planes intersect */

	/* Planes have "exactly" opposite normal vectors */
	f = a[3] + b[3];
	if( NEAR_ZERO( f, tol->dist ) )  {
		return(2);		/* Coplanar, opposite directions */
	}
	return(-1);			/* Parallel but distinct */
}

/*
 *			R T _ A N G L E _ M E A S U R E
 *
 *  Using two perpendicular vectors (x_dir and y_dir) which lie
 *  in the same plane as 'vec', return the angle (in radians) of 'vec'
 *  from x_dir, going CCW around the perpendicular x_dir CROSS y_dir.
 *
 *  Trig note -
 *
 *  theta = atan2(x,y) returns an angle in the range -pi to +pi.
 *  Here, we need an angle in the range of 0 to 2pi.
 *  This could be implemented by adding 2pi to theta when theta is negative,
 *  but this could have nasty numeric ambiguity right in the vicinity
 *  of theta = +pi, which is a very critical angle for the applications using
 *  this routine.
 *  So, an alternative formulation is to compute gamma = atan2(-x,-y),
 *  and then theta = gamma + pi.  Now, any error will occur in the
 *  vicinity of theta = 0, which can be handled much more readily.
 *
 *  If theta is negative, or greater than two pi,
 *  wrap it around.
 *  These conditions only occur if there are problems in atan2().
 *
 *  Returns -
 *	vec == x_dir returns 0,
 *	vec == y_dir returns pi/2,
 *	vec == -x_dir returns pi,
 *	vec == -y_dir returns 3*pi/2.
 */
double
rt_angle_measure( vec, x_dir, y_dir )
vect_t	vec;
CONST vect_t	x_dir;
CONST vect_t	y_dir;
{
	fastf_t		xproj, yproj;
	fastf_t		gamma;
	fastf_t		ang;

	xproj = -VDOT( vec, x_dir );
	yproj = -VDOT( vec, y_dir );
	gamma = atan2( yproj, xproj );	/* -pi..+pi */
	ang = rt_pi + gamma;		/* 0..+2pi */
	if( ang < 0 )  {
		return rt_twopi + ang;
	} else if( ang > rt_twopi )  {
		return ang - rt_twopi;
	}
	return ang;
}

/*
 *			R T _ D I S T _ P T 3 _ A L O N G _ L I N E 3
 *
 *  Return the parametric distance t of a point X along a line defined
 *  as a ray, i.e. solve X = P + t * D.
 *  If the point X does not lie on the line, then t is the distance of
 *  the perpendicular projection of point X onto the line.
 */
double
rt_dist_pt3_along_line3( p, d, x )
CONST point_t	p;
CONST vect_t	d;
CONST point_t	x;
{
	vect_t	x_p;

	VSUB2( x_p, x, p );
	return VDOT( x_p, d );
}


/*
 *			R T _ D I S T _ P T 2 _ A L O N G _ L I N E 2
 *
 *  Return the parametric distance t of a point X along a line defined
 *  as a ray, i.e. solve X = P + t * D.
 *  If the point X does not lie on the line, then t is the distance of
 *  the perpendicular projection of point X onto the line.
 */
double
rt_dist_pt2_along_line2( p, d, x )
CONST point_t	p;
CONST vect_t	d;
CONST point_t	x;
{
	vect_t	x_p;
	double	ret;

	VSUB2_2D( x_p, x, p );
	ret = VDOT_2D( x_p, d );
	if( rt_g.debug & DEBUG_MATH )  {
		rt_log("rt_dist_pt2_along_line2() p=(%g, %g), d=(%g, %g), x=(%g, %g) ret=%g\n",
			V2ARGS(p),
			V2ARGS(d),
			V2ARGS(x),
			ret );
	}
	return ret;
}

/*
 *  Returns -
 *	1	if left <= mid <= right
 *	0	if mid is not in the range.
 */
int
rt_between( left, mid, right, tol )
double	left;
double	mid;
double	right;
CONST struct rt_tol	*tol;
{
	RT_CK_TOL(tol);

	if( left < right )  {
		if( NEAR_ZERO(left-right, tol->dist*0.1) )  {
			left -= tol->dist*0.1;
			right += tol->dist*0.1;
		}
		if( mid < left || mid > right )  goto fail;
		return 1;
	}
	/* The 'right' value is lowest */
	if( NEAR_ZERO(left-right, tol->dist*0.1) )  {
		right -= tol->dist*0.1;
		left += tol->dist*0.1;
	}
	if( mid < right || mid > left )  goto fail;
	return 1;
fail:
	if( rt_g.debug & DEBUG_MATH )  {
		rt_log("rt_between( %.17e, %.17e, %.17e ) ret=0 FAIL\n",
			left, mid, right);
	}
	return 0;
}
