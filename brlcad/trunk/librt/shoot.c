#define NUgrid 0
/*
 *			S H O O T . C
 *
 *	Ray Tracing program shot coordinator.
 *  
 *  Given a ray, shoot it at all the relevant parts of the model,
 *  (building the finished_segs chain), and then call rt_boolregions()
 *  to build and evaluate the partition chain.
 *  If the ray actually hit anything, call the application's
 *  a_hit() routine with a pointer to the partition chain,
 *  otherwise, call the application's a_miss() routine.
 *
 *  It is important to note that rays extend infinitely only in the
 *  positive direction.  The ray is composed of all points P, where
 *
 *	P = r_pt + K * r_dir
 *
 *  for K ranging from 0 to +infinity.  There is no looking backwards.
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
 *	This software is Copyright (C) 1985,1991 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSshoot[] = "@(#)$Header$ (BRL)";
#endif

char rt_CopyRight_Notice[] = "@(#) Copyright (C) 1985,1991 by the United States Army";

#include "conf.h"

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./debug.h"

struct resource rt_uniresource;		/* Resources for uniprocessor */

#ifdef CRAY
#define AUTO register
#else
#define AUTO /*let the compiler decide*/
#endif

/* start NUgrid XXX  --  associated with cut.c */
#define RT_NUGRID_CELL(_array,_x,_y,_z)		(&(_array)[ \
	((((_z)*rt_nu_cells_per_axis[Y])+(_y))*rt_nu_cells_per_axis[X])+(_x) ])

struct rt_nu_axis {
	fastf_t	nu_spos;	/* cell start pos */
	fastf_t	nu_epos;	/* cell end pos */
	fastf_t	nu_width;	/* voxel size (end-start) */
};
extern struct nu_axis	*rt_nu_axis[3];
extern int		rt_nu_cells_per_axis[3];
extern union cutter	*rt_nu_grid;

#define NUGRID_T_SETUP(_ax,_cno)	\
	if( ap->a_ray.r_dir[_ax] == 0.0 )  { \
		ssp->tv[_ax] = INFINITY; \
	} else if( ap->a_ray.r_dir[_ax] < 0 ) { \
		ssp->tv[_ax] = (rt_nu_axis[_ax][_cno].nu_epos - ssp->newray.r_pt[_ax]) * \
			ssp->inv_dir[_ax]; \
	} else { \
		ssp->tv[_ax] = (rt_nu_axis[_ax][_cno].nu_spos - ssp->newray.r_pt[_ax]) * \
			ssp->inv_dir[_ax]; \
	}
#define NUGRID_T_ADV(_ax,_cno)  \
	if( ap->a_ray.r_dir[_ax] != 0 )  { \
		ssp->tv[_ax] += rt_nu_axis[_ax][_cno].nu_width * \
			ssp->abs_inv_dir[_ax]; \
	}

/* end NUgrid XXX */

#define BACKING_DIST	(-2.0)		/* mm to look behind start point */
#define OFFSET_DIST	0.01		/* mm to advance point into box */

struct shootray_status {
	fastf_t			dist_corr;	/* correction distance */
	fastf_t			odist_corr;
	fastf_t			box_start;
	fastf_t			obox_start;
	fastf_t			first_box_start;
	fastf_t			box_end;
	fastf_t			obox_end;
	fastf_t			model_start;
	fastf_t			model_end;
	struct xray		newray;		/* closer ray start */
	struct application	*ap;
	CONST union cutter	*lastcut;
	vect_t			inv_dir;	/* inverses of ap->a_ray.r_dir */
	/* Begin NUgrid additions */
	vect_t			abs_inv_dir;	/* absolute value of inv_dir */
	int			igrid[3];	/* integer cell coordinates */
	vect_t			tv;		/* next t intercept values */
	double			t0;		/* t val of cell "in" pt */
	double			t1;		/* t val of cell "out" pt */
	int			out_axis;	/* axis ray will leave through */
};


#if NUgrid
/*
 *			R T _ A D V A N C E _ T O _ N E X T _ C E L L
 *
 *  This version uses Gigante's non-uniform 3-D space grid/mesh discretization.
 */
CONST union cutter *
rt_advance_to_next_cell( ssp )
register struct shootray_status	*ssp;
{
	register CONST union cutter	*cutp;
	int			push_flag = 0;
	double			fraction;
	int			exponent;
	register CONST struct application *ap = ssp->ap;

	if( ssp->lastcut == &(ap->a_rt_i->rti_inf_box) )  {
	     	if( rt_g.debug & DEBUG_ADVANCE )  {
	     		rt_log("rt_advance_to_next_cell(): finished infinite solids\n");
	     	}
		return(CUTTER_NULL);
	}
	for(;;)  {
		if( ssp->lastcut == CUTTER_NULL )  {
			/*
			 *  First time through -- find starting cell.
			 *  For now, linear search to find x,y,z cell indices.
			 *  This should become a binary search
			 *  All distance values (t0, etc) are expressed
			 *  relative to ssp->first_box_start.
			 */
			register int	x, y, z;
			int	in_axis;
			int	j;

			ssp->dist_corr = ssp->first_box_start;
			VJOIN1( ssp->newray.r_pt, ap->a_ray.r_pt,
				ssp->dist_corr, ap->a_ray.r_dir );
			if( rt_g.debug&DEBUG_ADVANCE) {
				rt_log("rt_advance_to_next_cell() dist_corr=%g\n",
					ssp->dist_corr );
				rt_log("rt_advance_to_next_cell() newray.r_pt=(%g, %g, %g)\n",
					V3ARGS( ssp->newray.r_pt ) );
			}

			if( ssp->newray.r_pt[X] < rt_nu_axis[X][0].nu_spos )
				break;
			for( x=0; x < rt_nu_cells_per_axis[X]; x++ )  {
				if( ssp->newray.r_pt[X] < rt_nu_axis[X][x].nu_epos )
					break;
			}
			if( x >= rt_nu_cells_per_axis[X] )  break;

			if( ssp->newray.r_pt[Y] < rt_nu_axis[Y][0].nu_spos )
				break;
			for( y=0; y < rt_nu_cells_per_axis[Y]; y++ )  {
				if( ssp->newray.r_pt[Y] < rt_nu_axis[Y][y].nu_epos )
					break;
			}
			if( y >= rt_nu_cells_per_axis[Y] )  break;

			if( ssp->newray.r_pt[Z] < rt_nu_axis[Z][0].nu_spos )
				break;
			for( z=0; z < rt_nu_cells_per_axis[Z]; z++ )  {
				if( ssp->newray.r_pt[Z] < rt_nu_axis[Z][z].nu_epos )
					break;
			}
			if( z >= rt_nu_cells_per_axis[Z] )  break;
			cutp = RT_NUGRID_CELL( rt_nu_grid, x, y, z );

			/*
			 *  Prepare for efficient advancing from
			 *  cell to cell.
			 */
			ssp->igrid[X] = x;
			ssp->igrid[Y] = y;
			ssp->igrid[Z] = z;
if(rt_g.debug&DEBUG_ADVANCE)rt_log("igrid=(%d, %d, %d)\n", ssp->igrid[X], ssp->igrid[Y], ssp->igrid[Z]);

			/* tv[] has intercepts with walls of first cell on ray path */
			NUGRID_T_SETUP( X, x );
			NUGRID_T_SETUP( Y, y );
			NUGRID_T_SETUP( Z, z );

			/*
			 *  Find face (axis) of entry into first cell
			 *  using Cyrus&Beck -- max initial t value.
			 */
			if( ssp->tv[X] >= ssp->tv[Y] && ssp->tv[X] < INFINITY )  {
				in_axis = X;
				ssp->t0 = ssp->tv[X];
			} else {
				in_axis = Y;
				ssp->t0 = ssp->tv[Y];
			}
			if( ssp->tv[Z] > ssp->t0 && ssp->tv[Z] < INFINITY)  {
				in_axis = Z;
				ssp->t0 = ssp->tv[Z];
			}
if(rt_g.debug&DEBUG_ADVANCE) VPRINT("Entry tv[]", ssp->tv);
if(rt_g.debug&DEBUG_ADVANCE)rt_log("Entry axis is %s, t0=%g\n", in_axis==X ? "X" : (in_axis==Y?"Y":"Z"), ssp->t0);

			/* Advance to next exits */
			NUGRID_T_ADV( X, x );
			NUGRID_T_ADV( Y, y );
			NUGRID_T_ADV( Z, z );
if(rt_g.debug&DEBUG_ADVANCE) VPRINT("Exit tv[]", ssp->tv);

			/* XXX?Ensure that next exit is after first entrance */
			while( ssp->tv[X] < ssp->t0 && ap->a_ray.r_dir[X] != 0.0 )  {
				rt_log("*** at t=%g, pt[X]=%g\n",
					ssp->tv[X], ssp->newray.r_pt[X] + ssp->tv[X] *
					ap->a_ray.r_dir[X] );
				NUGRID_T_ADV( X, x );
				rt_log("*** advancing tv[X] to %g\n", ssp->tv[X]);
				rt_log("*** at t=%g, pt[X]=%g\n",
					ssp->tv[X], ssp->newray.r_pt[X] + ssp->tv[X] *
					ap->a_ray.r_dir[X] );
				rt_bomb("advancing\n");
			}
			while( ssp->tv[Y] < ssp->t0 && ap->a_ray.r_dir[Y] != 0.0 )  {
				rt_log("*** at t=%g, pt[Y]=%g\n",
					ssp->tv[Y], ssp->newray.r_pt[Y] + ssp->tv[Y] *
					ap->a_ray.r_dir[Y] );
				NUGRID_T_ADV( Y, y );
				rt_log("*** advancing tv[Y] to %g\n", ssp->tv[Y]);
				rt_log("*** at t=%g, pt[Y]=%g\n",
					ssp->tv[Y], ssp->newray.r_pt[Y] + ssp->tv[Y] *
					ap->a_ray.r_dir[Y] );
				rt_bomb("advancing\n");
			}
			while( ssp->tv[Z] < ssp->t0 && ap->a_ray.r_dir[Z] != 0.0 )  {
				rt_log("*** at t=%g, pt[Z]=%g\n",
					ssp->tv[Z], ssp->newray.r_pt[Z] + ssp->tv[Z] *
					ap->a_ray.r_dir[Z] );
				NUGRID_T_ADV( Z, z );
				rt_log("*** advancing tv[Z] to %g\n", ssp->tv[Z]);
				rt_log("*** at t=%g, pt[Z]=%g\n",
					ssp->tv[Z], ssp->newray.r_pt[Z] + ssp->tv[Z] *
					ap->a_ray.r_dir[Z] );
				rt_bomb("advancing\n");
			}
		} else {
			/* Advance from previous cell to next cell */
			/* Take next step, finding ray entry distance */
			ssp->t0 = ssp->t1;
			NUGRID_T_ADV( ssp->out_axis, ssp->igrid[ssp->out_axis] );
			if( ap->a_ray.r_dir[ssp->out_axis] > 0 ) {
				if( ++(ssp->igrid[ssp->out_axis]) >= rt_nu_cells_per_axis[ssp->out_axis] )
					break;
			} else {
				if( --(ssp->igrid[ssp->out_axis]) < 0 )
					break;
			}
if(rt_g.debug&DEBUG_ADVANCE)rt_log("igrid=(%d, %d, %d)\n", ssp->igrid[X], ssp->igrid[Y], ssp->igrid[Z]);
			/* XXX This too can be optimized */
			cutp = RT_NUGRID_CELL( rt_nu_grid,
				ssp->igrid[X], ssp->igrid[Y], ssp->igrid[Z] );
		}
		/* find minimum exit t value */
		if( ssp->tv[X] < ssp->tv[Y] )  {
			if( ssp->tv[Z] < ssp->tv[X] )  {
				ssp->out_axis = Z;
				ssp->t1 = ssp->tv[Z];
			} else {
				ssp->out_axis = X;
				ssp->t1 = ssp->tv[X];
			}
		} else {
			if( ssp->tv[Z] < ssp->tv[Y] )  {
				ssp->out_axis = Z;
				ssp->t1 = ssp->tv[Z];
			} else {
				ssp->out_axis = Y;
				ssp->t1 = ssp->tv[Y];
			}
		}
if(rt_g.debug&DEBUG_ADVANCE)rt_log("Exit axis is %s, t1=%g\n", ssp->out_axis==X ? "X" : (ssp->out_axis==Y?"Y":"Z"), ssp->t1);

		if(cutp==CUTTER_NULL || cutp->cut_type != CUT_BOXNODE)
			rt_bomb("rt_advance_to_next_cell(): leaf not boxnode");

		/* Don't get stuck within the same box */
		if( cutp==ssp->lastcut )  {
			rt_bomb("rt_advance_to_next_cel():  stuck in same cell\n");
		}
		ssp->lastcut = cutp;

		/*
		 *  To make life easier on the ray/solid intersectors,
		 *  give them a point "near" their solid, by
		 *  concocting a point roughly in this cell.
		 *  NOTE:  floating point error may cause this point to
		 *  be slightly outside the cell, which does not matter.
		 */
		ssp->dist_corr = ssp->model_start + ssp->t0;
		VJOIN1( ssp->newray.r_pt, ap->a_ray.r_pt,
			ssp->dist_corr, ap->a_ray.r_dir );
		if( rt_g.debug&DEBUG_ADVANCE) {
			rt_log("rt_advance_to_next_cell() dist_corr=%g\n",
				ssp->dist_corr );
			rt_log("rt_advance_to_next_cell() newray.r_pt=(%g, %g, %g)\n",
				V3ARGS( ssp->newray.r_pt ) );
		}

		ssp->newray.r_min = 0;
		ssp->newray.r_max = ssp->t1 - ssp->t0;

		ssp->box_start = ssp->first_box_start + ssp->t0;
		ssp->box_end = ssp->first_box_start + ssp->t1;
	     	if( rt_g.debug & DEBUG_ADVANCE )  {
			rt_log("rt_advance_to_next_cell() box=(%g, %g)\n",
				ssp->box_start, ssp->box_end );
	     	}
		return(cutp);
	}
	/* Off the end of the model RPP */
# if 0
	if( ap->a_rt_i->rti_inf_box.bn.bn_len > 0 )  {
	     	if( rt_g.debug & DEBUG_ADVANCE )  {
	     		rt_log("rt_advance_to_next_cell(): escaped model RPP, checking infinite solids\n");
	     	}
		ssp->lastcut = &(ap->a_rt_i->rti_inf_box);
		return &(ap->a_rt_i->rti_inf_box);
	}
# endif
     	if( rt_g.debug & DEBUG_ADVANCE )  {
     		rt_log("rt_advance_to_next_cell(): escaped model RPP\n");
     	}
	return(CUTTER_NULL);
}
#else
/*
 *			R T _ A D V A N C E _ T O _ N E X T _ C E L L
 *
 *  This version uses Muuss' non-uniform binary space partitioning tree.
 */
CONST union cutter *
rt_advance_to_next_cell( ssp )
register struct shootray_status	*ssp;
{
	register CONST union cutter	*cutp;
	int			push_flag = 0;
	double			fraction;
	int			exponent;
	register CONST struct application *ap = ssp->ap;
	register fastf_t	px, py, pz;	/* Adjusted ray pt */

	for(;;)  {
		/*
		 *  The point corresponding to the box_start distance
		 *  may not be in the "right" place,
		 *  due to the effects of floating point fuzz:
		 *  1)  The point might lie just outside
		 *	the model RPP, resulting in the point not
		 *	falling within the RPP of the indicated cell, or
		 *  2)	The poing might lie just a little bit on the
		 *	wrong side of the cell wall, resulting in
		 *	the ray getting "stuck", and needing rescuing
		 *	all the time by the error recovery code below.
		 *  Therefore, "nudge" the point just slightly into the
		 *  next cell by adding OFFSET_DIST.
		 *  XXX At present, a cell is never less than 1mm wide.
		 *  XXX The value of OFFSET_DIST should be some percentage
		 *	of the cell's smallest dimension,
		 *	rather than an absolute distance in mm.
		 *	This will prevent doing microscopic models.
		 */
		ssp->dist_corr = ssp->box_start + OFFSET_DIST;
top:
		if( ssp->dist_corr >= ssp->model_end )
			break;	/* done! */
		/* VJOIN1( pt, ap->a_ray.r_pt, ssp->dist_corr, ap->a_ray.r_dir ); */
		px = ap->a_ray.r_pt[X] + ssp->dist_corr * ap->a_ray.r_dir[X];
		py = ap->a_ray.r_pt[Y] + ssp->dist_corr * ap->a_ray.r_dir[Y];
		pz = ap->a_ray.r_pt[Z] + ssp->dist_corr * ap->a_ray.r_dir[Z];
		if( rt_g.debug&DEBUG_ADVANCE) {
			rt_log("rt_advance_to_next_cell() dist_corr=%g, pt=(%g, %g, %g)\n",
				ssp->dist_corr, px, py, pz );
		}

		cutp = &(ap->a_rt_i->rti_CutHead);
		while( cutp->cut_type == CUT_CUTNODE )  {
			switch( cutp->cn.cn_axis )  {
			case X:
				if( px >= cutp->cn.cn_point )  {
					cutp=cutp->cn.cn_r;
				}  else  {
					cutp=cutp->cn.cn_l;
				}
				break;
			case Y:
				if( py >= cutp->cn.cn_point )  {
					cutp=cutp->cn.cn_r;
				}  else  {
					cutp=cutp->cn.cn_l;
				}
				break;
			case Z:
				if( pz >= cutp->cn.cn_point )  {
					cutp=cutp->cn.cn_r;
				}  else  {
					cutp=cutp->cn.cn_l;
				}
				break;
			}
		}

		if(cutp==CUTTER_NULL || cutp->cut_type != CUT_BOXNODE)
			rt_bomb("rt_advance_to_next_cell(): leaf not boxnode");

		/* Ensure point is located in the indicated cell */
		if( px < cutp->bn.bn_min[X] || px > cutp->bn.bn_max[X] ||
		    py < cutp->bn.bn_min[Y] || py > cutp->bn.bn_max[Y] ||
		    pz < cutp->bn.bn_min[Z] || pz > cutp->bn.bn_max[Z] )  {
			rt_log("rt_advance_to_next_cell(): point not in cell, advancing\n");
#if 0
		    	rt_log(" pt (%.20e,%.20e,%.20e)\n", px, py, pz );
		    	rt_log("  min (%.20e,%.20e,%.20e)\n", V3ARGS(cutp->bn.bn_min) );
		    	rt_log("  max (%.20e,%.20e,%.20e)\n", V3ARGS(cutp->bn.bn_max) );
			rt_log("pt=(%g,%g,%g)\n", px, py, pz );
		     	rt_pr_cut( cutp, 0 );
#endif
			/*
			 * Move newray point further into new box.  Try again.
			 */
			ssp->dist_corr += OFFSET_DIST;
		    	goto top;
		}

		/* Don't get stuck within the same box for long */
		if( cutp==ssp->lastcut )  {
push:			;
#if 0
			rt_log("%d,%d box push odist_corr=%.20e n=%.20e model_end=%.20e\n",
				ap->a_x, ap->a_y,
				ssp->odist_corr, ssp->dist_corr, ssp->model_end );
			rt_log("box_start o=%.20e n=%.20e, box_end o=%.20e n=%.20e\n",
				ssp->obox_start, ssp->box_start,
				ssp->obox_end, ssp->box_end );
#endif
#if 0
			VPRINT("a_ray.r_pt", ap->a_ray.r_pt);
			rt_log("Point=(%g,%g,%g)\n", px, py, pz );
			VPRINT("Dir", ssp->newray.r_dir);
		     	rt_pr_cut( cutp, 0 );
#endif

			/* Advance 1mm, or smallest value that hardware
			 * floating point resolution will allow.
			 */
			fraction = frexp( ssp->box_end, &exponent );
#if 0
rt_log("frexp: box_end=%g, fract=%g, exp=%d\n", ssp->box_end, fraction, exponent);
#endif
			if( exponent <= 0 )  {
				/* Never advance less than 1mm */
				ssp->box_start = ssp->box_end + 1.0;
			} else {
				if( rt_g.debug & DEBUG_ADVANCE )  {
					rt_log("exp=%d, fraction=%.20e\n", exponent, fraction);
				}
				if( sizeof(fastf_t) <= 4 )
					fraction += 1.0e-5;
				else
					fraction += 1.0e-14;
				ssp->box_start = ldexp( fraction, exponent );
#if 0
rt_log("ldexp: box_end=%g, fract=%g, exp=%d\n", ssp->box_end, fraction, exponent);
#endif
			}
			if( rt_g.debug & DEBUG_ADVANCE )  {
				rt_log("push: was=%.20e, now=%.20e\n",
					ssp->box_end, ssp->box_start);
			}
			push_flag++;
			continue;
		}
		if( push_flag )  {
			push_flag = 0;
#if 0
			rt_log("%d,%d Escaped %d. dist_corr=%g, box_start=%g, box_end=%g\n",
				ap->a_x, ap->a_y, push_flag,
				ssp->dist_corr, ssp->box_start, ssp->box_end );
#endif
		}
		ssp->lastcut = cutp;

		ssp->newray.r_pt[X] = px;
		ssp->newray.r_pt[Y] = py;
		ssp->newray.r_pt[Z] = pz;

		if( !rt_in_rpp(&ssp->newray, ssp->inv_dir,
		     cutp->bn.bn_min, cutp->bn.bn_max) )  {
			rt_log("\nrt_advance_to_next_cell():  MISSED BOX\nrmin,rmax(%.20e,%.20e) box(%.20e,%.20e)\n",
				ssp->newray.r_min, ssp->newray.r_max,
				ssp->box_start, ssp->box_end);
		     	goto push;
		}
		ssp->odist_corr = ssp->dist_corr;
		ssp->obox_start = ssp->box_start;
		ssp->obox_end = ssp->box_end;
		ssp->box_start = ssp->dist_corr + ssp->newray.r_min;
		ssp->box_end = ssp->dist_corr + ssp->newray.r_max;
	     	if( rt_g.debug & DEBUG_ADVANCE )  {
			rt_log("rt_advance_to_next_cell() box=(%g, %g)\n",
				ssp->box_start, ssp->box_end );
	     	}
		return(cutp);
	}
	/* Off the end of the model RPP */
     	if( rt_g.debug & DEBUG_ADVANCE )  {
     		rt_log("rt_advance_to_next_cell(): escaped model RPP\n");
     	}
	return(CUTTER_NULL);
}
#endif

/*
 *			R T _ S H O O T R A Y
 *
 *  Note that the direction vector r_dir
 *  must have unit length;  this is mandatory, and is not ordinarily checked,
 *  in the name of efficiency.
 *
 *  Input:  Pointer to an application structure, with these mandatory fields:
 *	a_ray.r_pt	Starting point of ray to be fired
 *	a_ray.r_dir	UNIT VECTOR with direction to fire in (dir cosines)
 *	a_hit		Routine to call when something is hit
 *	a_miss		Routine to call when ray misses everything
 *
 *  Calls user's a_miss() or a_hit() routine as appropriate.
 *  Passes a_hit() routine list of partitions, with only hit_dist
 *  fields valid.  Normal computation deferred to user code,
 *  to avoid needless computation here.
 *
 *  Formal Return: whatever the application function returns (an int).
 *
 *  NOTE:  The appliction functions may call rt_shootray() recursively.
 *	Thus, none of the local variables may be static.
 *
 *  To prevent having to lock the statistics variables in a PARALLEL
 *  environment, all the statistics variables have been moved into
 *  the 'resource' structure, which is allocated per-CPU.
 */
int
rt_shootray( ap )
register struct application *ap;
{
	struct shootray_status	ss;
	struct seg		new_segs;	/* from solid intersections */
	struct seg		waiting_segs;	/* awaiting rt_boolweave() */
	struct seg		finished_segs;	/* processed by rt_boolweave() */
	AUTO fastf_t		last_bool_start;
	AUTO union bitv_elem	*solidbits;	/* bits for all solids shot so far */
	AUTO bitv_t		*regionbits;	/* bits for all involved regions */
	AUTO char		*status;
	auto struct partition	InitialPart;	/* Head of Initial Partitions */
	auto struct partition	FinalPart;	/* Head of Final Partitions */
	AUTO struct soltab	**stpp;
	register CONST union cutter *cutp;
	struct resource		*resp;
	AUTO struct rt_i	*rtip;
	CONST int		debug_shoot = rt_g.debug & DEBUG_SHOOT;

	RT_AP_CHECK(ap);
	if( ap->a_resource == RESOURCE_NULL )  {
		ap->a_resource = &rt_uniresource;
		rt_uniresource.re_magic = RESOURCE_MAGIC;
		if(rt_g.debug)rt_log("rt_shootray:  defaulting a_resource to &rt_uniresource\n");
	}
	ss.ap = ap;
	rtip = ap->a_rt_i;
	RT_CK_RTI( rtip );
	resp = ap->a_resource;
	RT_RESOURCE_CHECK(resp);

	if(rt_g.debug&(DEBUG_ALLRAYS|DEBUG_SHOOT|DEBUG_PARTITION)) {
		rt_g.rtg_logindent += 2;
		rt_log("\n**********shootray cpu=%d  %d,%d lvl=%d (%s)\n",
			resp->re_cpu,
			ap->a_x, ap->a_y,
			ap->a_level,
			ap->a_purpose != (char *)0 ? ap->a_purpose : "?" );
		rt_log("Pnt (%g, %g, %g) a_onehit=%d\n",
			V3ARGS(ap->a_ray.r_pt),
			ap->a_onehit );
		VPRINT("Dir", ap->a_ray.r_dir);
	}
	if(RT_BADVEC(ap->a_ray.r_pt)||RT_BADVEC(ap->a_ray.r_dir))  {
		rt_log("\n**********shootray cpu=%d  %d,%d lvl=%d (%s)\n",
			resp->re_cpu,
			ap->a_x, ap->a_y,
			ap->a_level,
			ap->a_purpose != (char *)0 ? ap->a_purpose : "?" );
		VPRINT(" r_pt", ap->a_ray.r_pt);
		VPRINT("r_dir", ap->a_ray.r_dir);
		rt_bomb("rt_shootray() bad ray\n");
	}

	if( rtip->needprep )
		rt_prep(rtip);

	InitialPart.pt_forw = InitialPart.pt_back = &InitialPart;
	InitialPart.pt_magic = PT_HD_MAGIC;
	FinalPart.pt_forw = FinalPart.pt_back = &FinalPart;
	FinalPart.pt_magic = PT_HD_MAGIC;

	RT_LIST_INIT( &new_segs.l );
	RT_LIST_INIT( &waiting_segs.l );
	RT_LIST_INIT( &finished_segs.l );

	/* see rt_get_bitv() for details on how bitvector len is set. */
	GET_BITV( rtip, solidbits, resp );
	bzero( (char *)solidbits, rtip->rti_bv_bytes );
	regionbits = &solidbits->be_v[
		2+RT_BITV_BITS2WORDS(rtip->nsolids)];

	/* Verify that direction vector has unit length */
	if(rt_g.debug) {
		FAST fastf_t f, diff;
		f = MAGSQ(ap->a_ray.r_dir);
		if( NEAR_ZERO(f, 0.0001) )  {
			rt_bomb("rt_shootray:  zero length dir vector\n");
			return(0);
		}
		diff = f - 1;
		if( !NEAR_ZERO( diff, 0.0001 ) )  {
			rt_log("rt_shootray: non-unit dir vect (x%d y%d lvl%d)\n",
				ap->a_x, ap->a_y, ap->a_level );
			f = 1/f;
			VSCALE( ap->a_ray.r_dir, ap->a_ray.r_dir, f );
		}
	}

	/* Compute the inverse of the direction cosines */
	if( ap->a_ray.r_dir[X] < -SQRT_SMALL_FASTF )  {
		ss.abs_inv_dir[X] = -(ss.inv_dir[X]=1.0/ap->a_ray.r_dir[X]);
	} else if( ap->a_ray.r_dir[X] > SQRT_SMALL_FASTF )  {
		ss.abs_inv_dir[X] =  (ss.inv_dir[X]=1.0/ap->a_ray.r_dir[X]);
	} else {
		ap->a_ray.r_dir[X] = 0.0;
		ss.abs_inv_dir[X] = ss.inv_dir[X] = INFINITY;
	}
	if( ap->a_ray.r_dir[Y] < -SQRT_SMALL_FASTF )  {
		ss.abs_inv_dir[Y] = -(ss.inv_dir[Y]=1.0/ap->a_ray.r_dir[Y]);
	} else if( ap->a_ray.r_dir[Y] > SQRT_SMALL_FASTF )  {
		ss.abs_inv_dir[Y] =  (ss.inv_dir[Y]=1.0/ap->a_ray.r_dir[Y]);
	} else {
		ap->a_ray.r_dir[Y] = 0.0;
		ss.abs_inv_dir[Y] = ss.inv_dir[Y] = INFINITY;
	}
	if( ap->a_ray.r_dir[Z] < -SQRT_SMALL_FASTF )  {
		ss.abs_inv_dir[Z] = -(ss.inv_dir[Z]=1.0/ap->a_ray.r_dir[Z]);
	} else if( ap->a_ray.r_dir[Z] > SQRT_SMALL_FASTF )  {
		ss.abs_inv_dir[Z] =  (ss.inv_dir[Z]=1.0/ap->a_ray.r_dir[Z]);
	} else {
		ap->a_ray.r_dir[Z] = 0.0;
		ss.abs_inv_dir[Z] = ss.inv_dir[Z] = INFINITY;
	}

	/*
	 *  If there are infinite solids in the model,
	 *  shoot at all of them, all of the time
	 *  (until per-solid classification is implemented)
	 *  before considering the model RPP.
	 *  This code is a streamlined version of the real version.
	 */
	if( rtip->rti_inf_box.bn.bn_len > 0 )  {
		/* Consider all objects within the box */
		cutp = &(rtip->rti_inf_box);
		stpp = &(cutp->bn.bn_list[cutp->bn.bn_len-1]);
		for( ; stpp >= cutp->bn.bn_list; stpp-- )  {
			register struct soltab *stp = *stpp;

			/* Shoot a ray */
			if(debug_shoot)rt_log("shooting %s\n", stp->st_name);
			BITSET( solidbits->be_v, stp->st_bit );
			resp->re_shots++;
			if( rt_functab[stp->st_id].ft_shot( 
			    stp, &ap->a_ray, ap, &waiting_segs ) <= 0 )  {
				resp->re_shot_miss++;
				continue;	/* MISS */
			}
			resp->re_shot_hit++;
		}
	}

	/*
	 *  If ray does not enter the model RPP, skip on.
	 *  If ray ends exactly at the model RPP, trace it.
	 */
	if( !rt_in_rpp( &ap->a_ray, ss.inv_dir, rtip->mdl_min, rtip->mdl_max )  ||
	    ap->a_ray.r_max < 0.0 )  {
	    	/* XXX it would be better to do the infinite objects here,
	    	 * XXX and let rt_advance_to_next_cell mention them otherwise,
	    	 * XXX such as when rays escape from the model RPP.
	    	 */
	    	if( RT_LIST_NON_EMPTY( &waiting_segs.l ) )  {
	    		/* Go handle the infinite objects we hit */
	    		ss.model_end = INFINITY;
	    		goto weave;
	    	}
		resp->re_nmiss_model++;
		ap->a_return = ap->a_miss( ap );
		status = "MISS model";
		goto out;
	}

	/*
	 *  The interesting part of the ray starts at distance 0.
	 *  If the ray enters the model at a negative distance,
	 *  (ie, the ray starts within the model RPP),
	 *  we only look at little bit behind (BACKING_DIST) to see if we are
	 *  just coming out of something, but never further back than
	 *  the intersection with the model RPP.
	 *  If the ray enters the model at a positive distance,
	 *  we always start there.
	 *  It is vital that we never pick a start point outside the
	 *  model RPP, or the space partitioning tree will pick the
	 *  wrong box and the ray will miss it.
	 *
	 *  BACKING_DIST should probably be determined by floating point
	 *  noise factor due to model RPP size -vs- number of bits of
	 *  floating point mantissa significance, rather than a constant,
	 *  but that is too hideous to think about here.
	 *  Also note that applications that really depend on knowing
	 *  what region they are leaving from should probably back their
	 *  own start-point up, rather than depending on it here, but
	 *  it isn't much trouble here.
	 */
	ss.box_start = ss.model_start = ap->a_ray.r_min;
	ss.box_end = ss.model_end = ap->a_ray.r_max;

	if( ss.box_start < BACKING_DIST )
		ss.box_start = BACKING_DIST; /* Only look a little bit behind */

	ss.first_box_start = ss.box_start;
	ss.lastcut = CUTTER_NULL;
	last_bool_start = BACKING_DIST;
	ss.newray = ap->a_ray;		/* struct copy */
	ss.odist_corr = ss.obox_start = ss.obox_end = -99;

	/*
	 *  While the ray remains inside model space,
	 *  push from box to box until ray emerges from
	 *  model space again (or first hit is found, if user is impatient).
	 *  It is vitally important to always stay within the model RPP, or
	 *  the space partitoning tree will pick wrong boxes & miss them.
	 */
	while( (cutp = rt_advance_to_next_cell( &ss )) != CUTTER_NULL )  {
		if(debug_shoot) {
			rt_pr_cut( cutp, 0 );
		}

		if( cutp->bn.bn_len <= 0 )  {
			/* Push ray onwards to next box */
			ss.box_start = ss.box_end;
			continue;
		}

		/* Consider all objects within the box */
		stpp = &(cutp->bn.bn_list[cutp->bn.bn_len-1]);
		for( ; stpp >= cutp->bn.bn_list; stpp-- )  {
			register struct soltab *stp = *stpp;

			/* On m35.g, this block of code eats 15% of CPU! */
			if( BITTEST( solidbits->be_v, stp->st_bit ) )  {
				continue;	/* already shot */
			}

			/* Shoot a ray */
			BITSET( solidbits->be_v, stp->st_bit );

			/* Check against bounding RPP, if desired by solid */
			if( rt_functab[stp->st_id].ft_use_rpp )  {
				if( !rt_in_rpp( &ss.newray, ss.inv_dir,
				    stp->st_min, stp->st_max ) )  {
					if(debug_shoot)rt_log("rpp miss %s\n", stp->st_name);
					resp->re_prune_solrpp++;
					continue;	/* MISS */
				}
				if( ss.dist_corr + ss.newray.r_max < BACKING_DIST )  {
					if(debug_shoot)rt_log("rpp skip %s, dist_corr=%g, r_max=%g\n", stp->st_name, ss.dist_corr, ss.newray.r_max);
					resp->re_prune_solrpp++;
					continue;	/* MISS */
				}
			}

			if(debug_shoot)rt_log("shooting %s\n", stp->st_name);
			resp->re_shots++;
			RT_LIST_INIT( &(new_segs.l) );
			if( rt_functab[stp->st_id].ft_shot( 
			    stp, &ss.newray, ap, &new_segs ) <= 0 )  {
				resp->re_shot_miss++;
				continue;	/* MISS */
			}

			/* Add seg chain to list awaiting rt_boolweave() */
			{
				register struct seg *s2;
				while(RT_LIST_WHILE(s2,seg,&(new_segs.l)))  {
					RT_LIST_DEQUEUE( &(s2->l) );
					/* Restore to original distance */
					s2->seg_in.hit_dist += ss.dist_corr;
					s2->seg_out.hit_dist += ss.dist_corr;
					RT_LIST_INSERT( &(waiting_segs.l), &(s2->l) );
				}
			}
			resp->re_shot_hit++;
		}

		/*
		 *  If a_onehit <= 0, then the ray is traced to +infinity.
		 *
		 *  If a_onehit > 0, then it indicates how many hit points
		 *  (which are greater than the ray start point of 0.0)
		 *  the application requires, ie, partitions with inhit >= 0.
		 *  If this box yielded additional segments,
		 *  immediately weave them into the partition list,
		 *  and perform final boolean evaluation.
		 *  If this results in the required number of final
		 *  partitions, then cease ray-tracing and hand the
		 *  partitions over to the application.
		 *  All partitions will have valid in and out distances.
		 */
		if( ap->a_onehit > 0 && RT_LIST_NON_EMPTY( &(waiting_segs.l) ) )  {
			int	done;

			/* Weave these segments into partition list */
			rt_boolweave( &finished_segs, &waiting_segs, &InitialPart, ap );

			/* Evaluate regions upto box_end */
			done = rt_boolfinal( &InitialPart, &FinalPart,
				last_bool_start, ss.box_end, regionbits, ap );
			last_bool_start = ss.box_end;

			/* See if enough partitions have been acquired */
			if( done > 0 )  goto hitit;
		}

		/* Push ray onwards to next box */
		ss.box_start = ss.box_end;
	}

	/*
	 *  Ray has finally left known space --
	 *  Weave any remaining segments into the partition list.
	 */
weave:
	if( RT_LIST_NON_EMPTY( &(waiting_segs.l) ) )  {
		rt_boolweave( &finished_segs, &waiting_segs, &InitialPart, ap );
	}

	/* finished_segs chain now has all segments hit by this ray */
	if( RT_LIST_IS_EMPTY( &(finished_segs.l) ) )  {
		ap->a_return = ap->a_miss( ap );
		status = "MISS solids";
		goto out;
	}

	/*
	 *  All intersections of the ray with the model have
	 *  been computed.  Evaluate the boolean trees over each partition.
	 */
	(void)rt_boolfinal( &InitialPart, &FinalPart, BACKING_DIST,
		rtip->rti_inf_box.bn.bn_len > 0 ? INFINITY : ss.model_end,
		regionbits, ap);

	if( FinalPart.pt_forw == &FinalPart )  {
		ap->a_return = ap->a_miss( ap );
		status = "MISS bool";
		RT_FREE_PT_LIST( &InitialPart, resp );
		RT_FREE_SEG_LIST( &finished_segs, resp );
		goto out;
	}

	/*
	 *  Ray/model intersections exist.  Pass the list to the
	 *  user's a_hit() routine.  Note that only the hit_dist
	 *  elements of pt_inhit and pt_outhit have been computed yet.
	 *  To compute both hit_point and hit_normal, use the
	 *
	 *  	RT_HIT_NORM( hitp, stp, rayp )
	 *
	 *  macro.  To compute just hit_point, use
	 *
	 *  VJOIN1( hitp->hit_point, rp->r_pt, hitp->hit_dist, rp->r_dir );
	 */
hitit:
	if(debug_shoot)  rt_pr_partitions(rtip,&FinalPart,"a_hit()");

	/*
	 *  Before recursing, release storage for unused Initial partitions.
	 *  finished_segs can not be released yet, because FinalPart
	 *  partitions will point to hits in those segments.
	 */
	RT_FREE_PT_LIST( &InitialPart, resp );

	ap->a_return = ap->a_hit( ap, &FinalPart );
	status = "HIT";

	RT_FREE_SEG_LIST( &finished_segs, resp );
	RT_FREE_PT_LIST( &FinalPart, resp );

	/*
	 * Processing of this ray is complete.
	 */
out:
	/*  Free dynamic resources.  */
	if( solidbits != BITV_NULL)  {
		FREE_BITV( solidbits, resp );
	}

	/*
	 *  Record essential statistics in per-processor data structure.
	 */
	resp->re_nshootray++;

	/* Terminate any logging */
	if(rt_g.debug&(DEBUG_ALLRAYS|DEBUG_SHOOT|DEBUG_PARTITION))  {
		if( rt_g.rtg_logindent > 0 )
			rt_g.rtg_logindent -= 2;
		else
			rt_g.rtg_logindent = 0;
		rt_log("----------shootray cpu=%d  %d,%d lvl=%d (%s) %s ret=%d\n",
			resp->re_cpu,
			ap->a_x, ap->a_y,
			ap->a_level,
			ap->a_purpose != (char *)0 ? ap->a_purpose : "?",
			status, ap->a_return);
	}
	return( ap->a_return );
}

/*
 *			R T _ I N _ R P P
 *
 *  Compute the intersections of a ray with a rectangular parallelpiped (RPP)
 *  that has faces parallel to the coordinate planes
 *
 *  The algorithm here was developed by Gary Kuehl for GIFT.
 *  A good description of the approach used can be found in
 *  "??" by XYZZY and Barsky,
 *  ACM Transactions on Graphics, Vol 3 No 1, January 1984.
 *
 * Note -
 *  The computation of entry and exit distance is mandatory, as the final
 *  test catches the majority of misses.
 *
 * Note -
 *  A hit is returned if the intersect is behind the start point.
 *
 *  Returns -
 *	 0  if ray does not hit RPP,
 *	!0  if ray hits RPP.
 *
 *  Implicit return -
 *	rp->r_min = dist from start of ray to point at which ray ENTERS solid
 *	rp->r_max = dist from start of ray to point at which ray LEAVES solid
 */
rt_in_rpp( rp, invdir, min, max )
struct xray		*rp;
register CONST fastf_t *invdir;	/* inverses of rp->r_dir[] */
register CONST fastf_t *min;
register CONST fastf_t *max;
{
	register CONST fastf_t	*pt = &rp->r_pt[0];
	register fastf_t	sv;
#define st sv			/* reuse the register */
	register fastf_t	rmin = -INFINITY;
	register fastf_t	rmax =  INFINITY;

	/* Start with infinite ray, and trim it down */

	/* X axis */
	if( *invdir < 0.0 )  {
		/* Heading towards smaller numbers */
		/* if( *min > *pt )  miss */
		if(rmax > (sv = (*min - *pt) * *invdir) )
			rmax = sv;
		if( rmin < (st = (*max - *pt) * *invdir) )
			rmin = st;
	}  else if( *invdir > 0.0 )  {
		/* Heading towards larger numbers */
		/* if( *max < *pt )  miss */
		if(rmax > (st = (*max - *pt) * *invdir) )
			rmax = st;
		if( rmin < ((sv = (*min - *pt) * *invdir)) )
			rmin = sv;
	}  else  {
		/*
		 *  Direction cosines along this axis is NEAR 0,
		 *  which implies that the ray is perpendicular to the axis,
		 *  so merely check position against the boundaries.
		 */
		if( (*min > *pt) || (*max < *pt) )
			return(0);	/* MISS */
	}

	/* Y axis */
	pt++; invdir++; max++; min++;
	if( *invdir < 0.0 )  {
		if(rmax > (sv = (*min - *pt) * *invdir) )
			rmax = sv;
		if( rmin < (st = (*max - *pt) * *invdir) )
			rmin = st;
	}  else if( *invdir > 0.0 )  {
		if(rmax > (st = (*max - *pt) * *invdir) )
			rmax = st;
		if( rmin < ((sv = (*min - *pt) * *invdir)) )
			rmin = sv;
	}  else  {
		if( (*min > *pt) || (*max < *pt) )
			return(0);	/* MISS */
	}

	/* Z axis */
	pt++; invdir++; max++; min++;
	if( *invdir < 0.0 )  {
		if(rmax > (sv = (*min - *pt) * *invdir) )
			rmax = sv;
		if( rmin < (st = (*max - *pt) * *invdir) )
			rmin = st;
	}  else if( *invdir > 0.0 )  {
		if(rmax > (st = (*max - *pt) * *invdir) )
			rmax = st;
		if( rmin < ((sv = (*min - *pt) * *invdir)) )
			rmin = sv;
	}  else  {
		if( (*min > *pt) || (*max < *pt) )
			return(0);	/* MISS */
	}

	/* If equal, RPP is actually a plane */
	if( rmin > rmax )
		return(0);	/* MISS */

	/* HIT.  Only now do rp->r_min and rp->r_max have to be written */
	rp->r_min = rmin;
	rp->r_max = rmax;
	return(1);		/* HIT */
}

/*
 *			R T _ B I T V _ O R
 */
void
rt_bitv_or( out, in, nbits )
register bitv_t *out;
register bitv_t *in;
int nbits;
{
	register int words;

	words = RT_BITV_BITS2WORDS(nbits);
#ifdef VECTORIZE
#	include "noalias.h"
	for( --words; words >= 0; words-- )
		out[words] |= in[words];
#else
	while( words-- > 0 )
		*out++ |= *in++;
#endif
}

/*
 *  			R T _ G E T _ B I T V
 *  
 *  This routine is called by the GET_BITV macro when the freelist
 *  is exhausted.  Rather than simply getting one additional structure,
 *  we get a whole batch, saving overhead.  When this routine is called,
 *  the bitv resource must already be locked.
 *  malloc() locking is done in rt_malloc.
 *
 *  Also note that there is a bit of trickery going on here:
 *  the *real* size of be_v[] array is determined at runtime, here.
 */
void
rt_get_bitv(rtip, res)
struct rt_i		*rtip;
register struct resource *res;
{
	register char *cp;
	register int bytes;
	register int size;		/* size of structure to really get */

	RT_CK_RTI(rtip);
	RT_RESOURCE_CHECK(res);

	size = rtip->rti_bv_bytes;
	if( size < 1 )  rt_bomb("rt_get_bitv");
	size = (size+sizeof(long)-1) & ~(sizeof(long)-1);
	bytes = rt_byte_roundup(16*size);
	cp = rt_malloc(bytes, "rt_get_bitv");

	while( bytes >= size )  {
		((union bitv_elem *)cp)->be_next = res->re_bitv;
		res->re_bitv = (union bitv_elem *)cp;
		res->re_bitvlen++;
		cp += size;
		bytes -= size;
	}
}

/* For debugging */
rt_DB_rpp( rp, invdir, min, max )
register struct xray *rp;
register CONST fastf_t *invdir;	/* inverses of rp->r_dir[] */
register CONST fastf_t *min;
register CONST fastf_t *max;
{
	register CONST fastf_t *pt = &rp->r_pt[0];
	FAST fastf_t sv;

	/* Start with infinite ray, and trim it down */
	rp->r_min = -INFINITY;
	rp->r_max = INFINITY;

VPRINT("r_pt ", pt);
VPRINT("r_dir", rp->r_dir);
VPRINT("min  ", min);
VPRINT("max  ", max);
	/* X axis */
	if( rp->r_dir[X] < 0.0 )  {
		/* Heading towards smaller numbers */
		/* if( *min > *pt )  miss */
		sv = (*min - *pt) * *invdir;
rt_log("sv=%g, r_max=%g\n", sv, rp->r_max);
		if(rp->r_max > sv)
			rp->r_max = sv;
		st = (*max - *pt) * *invdir;
rt_log("st=%g, r_min=%g\n", st, rp->r_min);
		if( rp->r_min < st )
			rp->r_min = st;
rt_log("r_min=%g, r_max=%g\n", rp->r_min, rp->r_max);
	}  else if( rp->r_dir[X] > 0.0 )  {
		/* Heading towards larger numbers */
		/* if( *max < *pt )  miss */
		st = (*max - *pt) * *invdir;
rt_log("st=%g, r_max=%g\n", st, rp->r_max);
		if(rp->r_max > st)
			rp->r_max = st;
		sv = (*min - *pt) * *invdir;
rt_log("sv=%g, r_min=%g\n", sv, rp->r_min);
		if( rp->r_min < sv )
			rp->r_min = sv;
rt_log("r_min=%g, r_max=%g\n", rp->r_min, rp->r_max);
	}  else  {
		/*
		 *  Direction cosines along this axis is NEAR 0,
		 *  which implies that the ray is perpendicular to the axis,
		 *  so merely check position against the boundaries.
		 */
		if( (*min > *pt) || (*max < *pt) )
			goto miss;
	}

	/* Y axis */
	pt++; invdir++; max++; min++;
	if( rp->r_dir[Y] < 0.0 )  {
		/* Heading towards smaller numbers */
		/* if( *min > *pt )  miss */
		sv = (*min - *pt) * *invdir;
rt_log("sv=%g, r_max=%g\n", sv, rp->r_max);
		if(rp->r_max > sv)
			rp->r_max = sv;
		st = (*max - *pt) * *invdir;
rt_log("st=%g, r_min=%g\n", st, rp->r_min);
		if( rp->r_min < st )
			rp->r_min = st;
rt_log("r_min=%g, r_max=%g\n", rp->r_min, rp->r_max);
	}  else if( rp->r_dir[Y] > 0.0 )  {
		/* Heading towards larger numbers */
		/* if( *max < *pt )  miss */
		st = (*max - *pt) * *invdir;
rt_log("st=%g, r_max=%g\n", st, rp->r_max);
		if(rp->r_max > st)
			rp->r_max = st;
		sv = (*min - *pt) * *invdir;
rt_log("sv=%g, r_min=%g\n", sv, rp->r_min);
		if( rp->r_min < sv )
			rp->r_min = sv;
rt_log("r_min=%g, r_max=%g\n", rp->r_min, rp->r_max);
	}  else  {
		if( (*min > *pt) || (*max < *pt) )
			goto miss;
	}

	/* Z axis */
	pt++; invdir++; max++; min++;
	if( rp->r_dir[Z] < 0.0 )  {
		/* Heading towards smaller numbers */
		/* if( *min > *pt )  miss */
		sv = (*min - *pt) * *invdir;
rt_log("sv=%g, r_max=%g\n", sv, rp->r_max);
		if(rp->r_max > sv)
			rp->r_max = sv;
		st = (*max - *pt) * *invdir;
rt_log("st=%g, r_min=%g\n", st, rp->r_min);
		if( rp->r_min < st )
			rp->r_min = st;
rt_log("r_min=%g, r_max=%g\n", rp->r_min, rp->r_max);
	}  else if( rp->r_dir[Z] > 0.0 )  {
		/* Heading towards larger numbers */
		/* if( *max < *pt )  miss */
		st = (*max - *pt) * *invdir;
rt_log("st=%g, r_max=%g\n", st, rp->r_max);
		if(rp->r_max > st)
			rp->r_max = st;
		sv = (*min - *pt) * *invdir;
rt_log("sv=%g, r_min=%g\n", sv, rp->r_min);
		if( rp->r_min < sv )
			rp->r_min = sv;
rt_log("r_min=%g, r_max=%g\n", rp->r_min, rp->r_max);
	}  else  {
		if( (*min > *pt) || (*max < *pt) )
			goto miss;
	}

	/* If equal, RPP is actually a plane */
	if( rp->r_min > rp->r_max )
		goto miss;
	rt_log("HIT:  %g..%g\n", rp->r_min, rp->r_max );
	return(1);		/* HIT */
miss:
	rt_log("MISS\n");
	return(0);		/* MISS */
}

#define SEG_MISS(SEG)		(SEG).seg_stp=(struct soltab *) 0;	

/* Stub function which will "similate" a call to a vector shot routine */
void
rt_vstub( stp, rp, segp, n, ap )
struct soltab	       *stp[]; /* An array of solid pointers */
struct xray		*rp[]; /* An array of ray pointers */
struct  seg            segp[]; /* array of segs (results returned) */
int		  	    n; /* Number of ray/object pairs */
struct application	*ap; /* pointer to an application */
{
	register int    i;
	register struct seg *tmp_seg;
	struct seg	seghead;

	RT_LIST_INIT( &(seghead.l) );

	/* go through each ray/solid pair and call a scalar function */
	for (i = 0; i < n; i++) {
		if (stp[i] != 0){ /* skip call if solid table pointer is NULL */
			/* do scalar call, place results in segp array */
			if( rt_functab[stp[i]->st_id].ft_shot(stp[i], rp[i], ap, &seghead) <= 0 )  {
				SEG_MISS(segp[i]);
			} else {
				tmp_seg = RT_LIST_FIRST(seg, &(seghead.l) );
				RT_LIST_DEQUEUE( &(tmp_seg->l) );
				segp[i] = *tmp_seg; /* structure copy */
				RT_FREE_SEG(tmp_seg, ap->a_resource);
			}
		}
	}
}

/*
 *			R T _ P R _ L I B R A R Y _ V E R S I O N
 *
 *  In case anyone actually cares, print out library's compilation version.
 */
void
rt_pr_library_version()
{
	rt_log("%s", rt_version);
}

/*
 *			R T _ A D D _ R E S _ S T A T S
 *
 *  To be called only in non-parallel mode, to tally up the statistics
 *  from the resource structure(s) into the rt instance structure.
 *
 *  Non-parallel programs should call
 *	rt_add_res_stats( rtip, RESOURCE_NULL );
 *  to have the default resource results tallied in.
 */
void
rt_add_res_stats( rtip, resp )
register struct rt_i		*rtip;
register struct resource	*resp;
{
	RT_CK_RTI( rtip );

	if( resp == RESOURCE_NULL )  resp = &rt_uniresource;
	RT_CK_RESOURCE( resp );

	rtip->rti_nrays += resp->re_nshootray;
	rtip->nmiss_model += resp->re_nmiss_model;

	rtip->nshots += resp->re_shots;
	rtip->nhits += resp->re_shot_hit;
	rtip->nmiss += resp->re_shot_miss;

	rtip->nmiss_solid += resp->re_prune_solrpp;

	/* Zero out resource totals, so repeated calls are not harmful */
	resp->re_nshootray = 0;
	resp->re_nmiss_model = 0;

	resp->re_shots = 0;
	resp->re_shot_hit = 0;
	resp->re_shot_miss = 0;

	resp->re_prune_solrpp = 0;
}
