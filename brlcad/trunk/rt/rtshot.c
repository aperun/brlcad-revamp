/*
 *			R T S H O T . C 
 *
 *  Demonstration Ray Tracing main program, using RT library.
 *  Fires a single ray, given any two of these three parameters:
 *	start point
 *	at point
 *	direction vector
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
 *	This software is Copyright (C) 1987 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSrt[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./rdebug.h"
#include "../librt/debug.h"

extern int	getopt();
extern char	*optarg;
extern int	optind;

char	usage[] = "\
Usage:  rtshot [options] model.g objects...\n\
 -U #		Set use_air flag\n\
 -x #		Set librt debug flags\n\
 -d # # #	Set direction vector\n\
 -p # # #	Set starting point\n\
 -a # # #	Set shoot-at point\n";

extern double	atof();
extern char	*sbrk();

int		rdebug;			/* RT program debugging (not library) */

int		npsw = 1;		/* Run serially */
int		interactive = 0;	/* human is watching results */

struct resource		resource;
struct application	ap;

int		set_dir = 0;
int		set_pt = 0;
int		set_at = 0;
vect_t		at_vect;
int		use_air = 0;		/* Handling of air */

extern int hit(), miss();

/*
 *			M A I N
 */
main(argc, argv)
int argc;
char **argv;
{
	static struct rt_i *rtip;
	static vect_t temp;
	char *title_file;
	char idbuf[132];		/* First ID record info */

	RES_INIT( &rt_g.res_syscall );
	RES_INIT( &rt_g.res_worker );
	RES_INIT( &rt_g.res_stats );
	RES_INIT( &rt_g.res_results );

	if( argc < 3 )  {
		(void)fputs(usage, stderr);
		exit(1);
	}
	argc--;
	argv++;

	while( argv[0][0] == '-' ) switch( argv[0][1] )  {
	case 'U':
		sscanf( argv[1], "%d", &use_air );
		argc -= 2;
		argv += 2;
		break;
	case 'x':
		sscanf( argv[1], "%x", &rt_g.debug );
		fprintf(stderr,"librt rt_g.debug=x%x\n", rt_g.debug);
		argc -= 2;
		argv += 2;
		break;

	case 'd':
		if( argc < 4 )  goto err;
		ap.a_ray.r_dir[X] = atof( argv[1] );
		ap.a_ray.r_dir[Y] = atof( argv[2] );
		ap.a_ray.r_dir[Z] = atof( argv[3] );
		set_dir = 1;
		argc -= 4;
		argv += 4;
		continue;

	case 'p':
		if( argc < 4 )  goto err;
		ap.a_ray.r_pt[X] = atof( argv[1] );
		ap.a_ray.r_pt[Y] = atof( argv[2] );
		ap.a_ray.r_pt[Z] = atof( argv[3] );
		set_pt = 1;
		argc -= 4;
		argv += 4;
		continue;

	case 'a':
		if( argc < 4 )  goto err;
		at_vect[X] = atof( argv[1] );
		at_vect[Y] = atof( argv[2] );
		at_vect[Z] = atof( argv[3] );
		set_at = 1;
		argc -= 4;
		argv += 4;
		continue;

	default:
err:
		(void)fputs(usage, stderr);
		exit(1);
	}
	if( argc < 2 )  {
		fprintf(stderr,"rtshot: MGED database not specified\n");
		(void)fputs(usage, stderr);
		exit(1);
	}

	if( set_dir + set_pt + set_at != 2 )  goto err;

	/* Load database */
	title_file = argv[0];
	argv++;
	argc--;
	if( (rtip=rt_dirbuild(title_file, idbuf, sizeof(idbuf))) == RTI_NULL ) {
		fprintf(stderr,"rtshot:  rt_dirbuild failure\n");
		exit(2);
	}
	ap.a_rt_i = rtip;
	fprintf(stderr, "db title:  %s\n", idbuf);
	rtip->useair = use_air;

	/* Walk trees */
	while( argc > 0 )  {
		if( rt_gettree(rtip, argv[0]) < 0 )
			fprintf(stderr,"rt_gettree(%s) FAILED\n", argv[0]);
		argc--;
		argv++;
	}

	/* Compute r_dir and r_pt from the inputs */
	if( set_at )  {
		if( set_dir ) {
			vect_t	diag;
			fastf_t	viewsize;
			VSUB2( diag, rtip->mdl_max, rtip->mdl_min );
			viewsize = MAGNITUDE( diag );
			VJOIN1( ap.a_ray.r_pt, at_vect,
				-viewsize/2.0, ap.a_ray.r_dir );
		} else {
			/* set_pt */
			VSUB2( ap.a_ray.r_dir, at_vect, ap.a_ray.r_pt );
		}
	}
	VUNITIZE( ap.a_ray.r_dir );

	VPRINT( "Pnt", ap.a_ray.r_pt );
	VPRINT( "Dir", ap.a_ray.r_dir );

	/* Shoot Ray */
	ap.a_hit = hit;
	ap.a_miss = miss;
	ap.a_resource = &resource;
	(void)rt_shootray( &ap );

	return(0);
}

hit( ap, PartHeadp )
register struct application *ap;
struct partition *PartHeadp;
{
	register struct partition *pp;
	register struct hit *hitp;
	register struct soltab *stp;
	struct curvature cur;

	for( pp=PartHeadp->pt_forw; pp != PartHeadp; pp = pp->pt_forw )  {
		stp = pp->pt_inseg->seg_stp;
		rt_log("\n--- Hit %s of region %s\n",
			stp->st_name, pp->pt_regionp->reg_name );

		hitp = pp->pt_inhit;
		RT_HIT_NORM( hitp, stp, &(ap->a_ray) );
		if( pp->pt_inflip )
			VREVERSE( hitp->hit_normal, hitp->hit_normal );
		rt_pr_hit( "  In", hitp );
		RT_CURVE( &cur, hitp, stp );
		VPRINT("PDir", cur.crv_pdir );
		rt_log(" c1=%g\n", cur.crv_c1);
		rt_log(" c2=%g\n", cur.crv_c2);

		/* outhit - no solid name, no curvature? */
		stp = pp->pt_outseg->seg_stp;
		hitp = pp->pt_outhit;
		RT_HIT_NORM( hitp, stp, &(ap->a_ray) );
		if( pp->pt_outflip )
			VREVERSE( hitp->hit_normal, hitp->hit_normal );
		rt_pr_hit( " Out", hitp );
	}
	return(0);
}

miss()
{
	rt_log("missed\n");
	return(0);
}

#if defined(SYSV)
#if !defined(bcopy)
bcopy(from,to,count)
{
	memcpy( to, from, count );
}
#endif
#if !defined(bzero)
bzero(str,n)
{
	memset( str, '\0', n );
}
#endif
#endif
