/*
 *			S O L I D . C
 *
 *  Subroutine to convert solids from
 *  COMGEOM card decks into GED object files.  This conversion routine
 *  is used to translate between COMGEOM solids, and the
 *  more general GED solids.
 *
 *  Author -
 *	Michael John Muuss
 *
 *  Original Version -
 *	March, 1980
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1989 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include "machine.h"
#include "vmath.h"

extern FILE	*outfp;
extern int	version;
extern int	verbose;

extern double	getdouble();
extern int	sol_total, sol_work;

#define PI		3.14159265358979323846264	/* Approx */
#define	DEG2RAD		0.0174532925199433

char	scard[132];			/* Solid card buffer area */

/*
 *			G E T S O L D A T A
 *
 *  Obtain 'num' data items from input card(s).
 *  The first input card is already in global 'scard'.
 *
 *  Returns -
 *	 0	OK
 *	-1	failure
 */
int
getsoldata( dp, num, solid_num )
double	*dp;
int	num;
int	solid_num;
{
	int	cd;
	double	*fp;
	int	i;
	int	j;

	fp = dp;
	for( cd=1; num > 0; cd++ )  {
		if( cd != 1 )  {
			if( getline( scard, sizeof(scard), "solid continuation card" ) == EOF )  {
				printf("too few cards for solid %d\n",
					solid_num);
				return(-1);
			}
			/* continuation card
			 * solid type should be blank 
			 */
			if( (version==5 && scard[5] != ' ' ) ||
			    (version==4 && scard[3] != ' ' ) )  {
				printf("solid %d (continuation) card %d non-blank\n",
					solid_num, cd);
				return(-1);
			}
		}

		if( num < 6 )
			j = num;
		else
			j = 6;

		for( i=0; i<j; i++ )  {
			*fp++ = getdouble( scard, 10+i*10, 10 );
		}
		num -= j;
	}
	return(0);
}

/*
 *			G E T X S O L D A T A
 *
 *  Obtain 'num' data items from input card(s).
 *  All input cards must be freshly read.
 *
 *  Returns -
 *	 0	OK
 *	-1	failure
 */
int
getxsoldata( dp, num, solid_num )
double	*dp;
int	num;
int	solid_num;
{
	int	cd;
	double	*fp;
	int	i;
	int	j;

	fp = dp;
	for( cd=1; num > 0; cd++ )  {
		if( getline( scard, sizeof(scard), "x solid card" ) == EOF )  {
			printf("too few cards for solid %d\n",
				solid_num);
			return(-1);
		}
		if( cd != 1 )  {
			/* continuation card
			 * solid type should be blank 
			 */
			if( (version==5 && scard[5] != ' ' ) ||
			    (version==4 && scard[3] != ' ' ) )  {
				printf("solid %d (continuation) card %d non-blank\n",
					solid_num, cd);
				return(-1);
			}
		}

		if( num < 6 )
			j = num;
		else
			j = 6;

		for( i=0; i<j; i++ )  {
			*fp++ = getdouble( scard, 10+i*10, 10 );
		}
		num -= j;
	}
	return(0);
}

/*
 *			T R I M _ T R A I L _ S P A C E S
 */
trim_trail_spaces( cp )
register char	*cp;
{
	register char	*ep;

	ep = cp + strlen(cp) - 1;
	while( ep >= cp )  {
		if( *ep != ' ' )  break;
		*ep-- = '\0';
	}
}

/*
 *			G E T S O L I D
 *
 *  Returns -
 *	-1	error
 *	 0	conversion OK
 *	 1	EOF
 */
getsolid()
{
	char	given_solid_num[16];
	char	solid_type[16];
	int	i;
	double	r1, r2;
	vect_t	work;
	double	m1, m2;		/* Magnitude temporaries */
	char	name[16+2];
	double	dd[4*6];	/* 4 cards of 6 nums each */
	double	tmp[3*8];	/* 8 vectors of 3 nums each */
#define D(_i)	(&(dd[_i*3]))
#define T(_i)	(&(tmp[_i*3]))

	if( (i = getline( scard, sizeof(scard), "solid card" )) == EOF )  {
		printf("getsolid: unexpected EOF\n");
		return( 1 );
	}

	switch( version )  {
	case 5:
		strncpy( given_solid_num, scard+0, 5 );
		given_solid_num[5] = '\0';
		strncpy( solid_type, scard+5, 5 );
		solid_type[5] = '\0';
		break;
	case 4:
		strncpy( given_solid_num, scard+0, 3 );
		given_solid_num[3] = '\0';
		strncpy( solid_type, scard+3, 7 );
		solid_type[7] = '\0';
		break;
	case 1:
		/* DoE/MORSE version, believed to be original MAGIC format */
		strncpy( given_solid_num, scard+5, 4 );
		given_solid_num[4] = '\0';
		strncpy( solid_type, scard+2, 3 );
		break;
	default:
		fprintf(stderr,"getsolid() version %d unimplemented\n", version);
		exit(1);
		break;
	}
	/* Trim trailing spaces */
	trim_trail_spaces( given_solid_num );
	trim_trail_spaces( solid_type );

	/* another solid - increment solid counter
	 * rather than using number from the card, which may go into
	 * pseudo-hex format in version 4 models (due to 3 column limit).
	 */
	sol_work++;
	if( version == 5 )  {
		if( (i = getint( scard, 0, 5 )) != sol_work )  {
			printf("expected solid card %d, got %d, abort\n",
				sol_work, i );
			return(1);
		}
	}

	/* Reduce solid type to lower case */
	{
		register char	*cp;
		register char	c;

		cp = solid_type;
		while( (c = *cp) != '\0' )  {
			if( !isascii(c) )  {
				*cp++ = '?';
			} else if( isupper(c) )  {
				*cp++ = tolower(c);
			} else {
				*cp++;
			}
		}
	}

	namecvt( sol_work, name, 's' );
	if(verbose) col_pr( name );

	if( strcmp( solid_type, "end" ) == 0 )  {
		/* DoE/MORSE version 1 format */
		return(1);		/* END */
	}

	if( strcmp( solid_type, "ars" ) == 0 )  {
		int		ncurves;
		int		pts_per_curve;
		double		**curve;
		int		ret;

		ncurves = getint( scard, 10, 10 );
		pts_per_curve = getint( scard, 20, 10 );

		/* Allocate curves pointer array */
		if( (curve = (double **)malloc((ncurves+1)*sizeof(double *))) == ((double **)0) )  {
			printf("malloc failure for ARS %d\n", sol_work);
			return(-1);
		}
		/* Allocate space for a curve, and read it in */
		for( i=0; i<ncurves; i++ )  {
			if( (curve[i] = (double *)malloc(
			    (pts_per_curve+1)*3*sizeof(double) )) ==
			    ((double *)0) )  {
				printf("malloc failure for ARS %d curve %d\n",
					sol_work, i);
				return(-1);
			}
			/* Get data for this curve */
			if( getxsoldata( curve[i], pts_per_curve*3, sol_work ) < 0 )  {
				printf("ARS %d: getxsoldata failed, curve %d\n",
					sol_work, i);
				return(-1);
			}
		}
		if( (ret = mk_ars( outfp, name, ncurves, pts_per_curve, curve )) < 0 )  {
			printf("mk_ars(%s) failed\n", name );
			/* Need to free memory; 'ret' is returned below */
		}

		for( i=0; i<ncurves; i++ )  {
			free( (char *)curve[i] );
		}
		free( (char *)curve);
		return(ret);
	}

	if( strcmp( solid_type, "rpp" ) == 0 )  {
		double	min[3], max[3];

		if( getsoldata( dd, 2*3, sol_work ) < 0 )
			return(-1);
		VSET( min, dd[0], dd[2], dd[4] );
		VSET( max, dd[1], dd[3], dd[5] );
		return( mk_rpp( outfp, name, min, max ) );
	}

	if( strcmp( solid_type, "box" ) == 0 )  {
		if( getsoldata( dd, 4*3, sol_work ) < 0 )
			return(-1);
		VMOVE( T(0), D(0) );
		VADD2( T(1), D(0), D(2) );
		VADD3( T(2), D(0), D(2), D(1) );
		VADD2( T(3), D(0), D(1) );

		VADD2( T(4), D(0), D(3) );
		VADD3( T(5), D(0), D(3), D(2) );
		VADD4( T(6), D(0), D(3), D(2), D(1) );
		VADD3( T(7), D(0), D(3), D(1) );
		return( mk_arb8( outfp, name, tmp ) );
	}

	if( strcmp( solid_type, "raw" ) == 0 ||
	    strcmp( solid_type, "wed" ) == 0		/* DoE name */
	)  {
		if( getsoldata( dd, 4*3, sol_work ) < 0 )
			return(-1);
		VMOVE( T(0), D(0) );
		VADD2( T(1), D(0), D(2) );
		VMOVE( T(2), T(1) );
		VADD2( T(3), D(0), D(1) );

		VADD2( T(4), D(0), D(3) );
		VADD3( T(5), D(0), D(3), D(2) );
		VMOVE( T(6), T(5) );
		VADD3( T(7), D(0), D(3), D(1) );
		return( mk_arb8( outfp, name, tmp ) );
	}

	if( strcmp( solid_type, "rvw" ) == 0 )  {
		/* Right Vertical Wedge (Origin: DoE/MORSE) */
		double	a2, theta, phi, h2;
		double	a2theta;
		double	angle1, angle2;
		vect_t	a, b, c;

		if( getsoldata( dd, 1*3+4, sol_work ) < 0 )
			return(-1);
		a2 = dd[3];		/* XY side length */
		theta = dd[4];
		phi = dd[5];
		h2 = dd[6];		/* height in +Z */

		angle1 = (phi+theta-90) * DEG2RAD;
		angle2 = (phi+theta) * DEG2RAD;
		a2theta = a2 * tan(theta * DEG2RAD);

		VSET( a, a2theta*cos(angle1), a2theta*sin(angle1), 0 );
		VSET( b, -a2*cos(angle2), -a2*sin(angle2), 0 );
		VSET( c, 0, 0, h2 );
		
		VSUB2( T(0), D(0), b );
		VMOVE( T(1), D(0) );
		VMOVE( T(2), D(0) );
		VADD2( T(3), T(0), a );

		VADD2( T(4), T(0), c );
		VADD2( T(5), T(1), c );
		VMOVE( T(6), T(5) );
		VADD2( T(7), T(3), c );
		return( mk_arb8( outfp, name, tmp ) );
	}

	if( strcmp( solid_type, "arb8" ) == 0 )  {
		if( getsoldata( dd, 8*3, sol_work ) < 0 )
			return(-1);
		return( mk_arb8( outfp, name, dd ) );
	}

	if( strcmp( solid_type, "arb7" ) == 0 )  {
		if( getsoldata( dd, 7*3, sol_work ) < 0 )
			return(-1);
		VMOVE( D(7), D(4) );
		return( mk_arb8( outfp, name, dd ) );
	}

	if( strcmp( solid_type, "arb6" ) == 0 )  {
		if( getsoldata( dd, 6*3, sol_work ) < 0 )
			return(-1);
		/* Note that the ordering is important, as data is in D(4), D(5) */
		VMOVE( D(7), D(5) );
		VMOVE( D(6), D(5) );
		VMOVE( D(5), D(4) );
		return( mk_arb8( outfp, name, dd ) );
	}

	if( strcmp( solid_type, "arb5" ) == 0 )  {
		if( getsoldata( dd, 5*3, sol_work ) < 0 )
			return(-1);
		VMOVE( D(5), D(4) );
		VMOVE( D(6), D(4) );
		VMOVE( D(7), D(4) );
		return( mk_arb8( outfp, name, dd ) );
	}

	if( strcmp( solid_type, "arb4" ) == 0 )  {
		if( getsoldata( dd, 4*3, sol_work ) < 0 )
			return(-1);
		return( mk_arb4( outfp, name, dd ) );
	}

	if( strcmp( solid_type, "rcc" ) == 0 )  {
		/* V, H, r */
		if( getsoldata( dd, 2*3+1, sol_work ) < 0 )
			return(-1);
		return( mk_rcc( outfp, name, D(0), D(1), dd[6] ) );
	}

	if( strcmp( solid_type, "rec" ) == 0 )  {
		/* V, H, A, B */
		if( getsoldata( dd, 4*3, sol_work ) < 0 )
			return(-1);
		return( mk_tgc( outfp, name, D(0), D(1),
			D(2), D(3), D(2), D(3) ) );
	}

	if( strcmp( solid_type, "trc" ) == 0 )  {
		/* V, H, r1, r2 */
		if( getsoldata( dd, 2*3+2, sol_work ) < 0 )
			return(-1);
		return( mk_trc( outfp, name, D(0), D(1), dd[6], dd[7] ) );
	}

	if( strcmp( solid_type, "tec" ) == 0 )  {
		/* V, H, A, B, p */
		if( getsoldata( dd, 4*3+1, sol_work ) < 0 )
			return(-1);
		r1 = 1.0/dd[12];	/* P */
		VSCALE( D(4), D(2), r1 );
		VSCALE( D(5), D(3), r1 );
		return( mk_tgc( outfp, name, D(0), D(1),
			D(2), D(3), D(4), D(5) ) );
	}

	if( strcmp( solid_type, "tgc" ) == 0 )  {
		/* V, H, A, B, r1, r2 */
		if( getsoldata( dd, 4*3+2, sol_work ) < 0 )
			return(-1);
		r1 = dd[12] / MAGNITUDE( D(2) );	/* A/|A| * C */
		r2 = dd[13] / MAGNITUDE( D(3) );	/* B/|B| * D */
		VSCALE( D(4), D(2), r1 );
		VSCALE( D(5), D(3), r2 );
		return( mk_tgc( outfp, name, D(0), D(1),
			D(2), D(3), D(4), D(5) ) );
	}

	if( strcmp( solid_type, "sph" ) == 0 )  {
		/* V, radius */
		if( getsoldata( dd, 1*3+2, sol_work ) < 0 )
			return(-1);
		return( mk_sph( outfp, name, D(0), dd[3] ) );
	}

	if( version <= 4 && strcmp( solid_type, "ell" ) == 0 )  {
		/* Foci F1, F2, major axis length L */
		vect_t	v;

		/*
		 * For simplicity, we convert ELL to ELL1, then
		 * fall through to ELL1 code.
		 * Format of ELL is F1, F2, len
		 * ELL1 format is V, A, r
		 */
		if( getsoldata( dd, 2*3+1, sol_work ) < 0 )
			return(-1);
		VADD2SCALE( v, D(0), D(1), 0.5 ); /* V is midpoint */

		VSUB2( work, D(1), D(0) );	/* work holds F2 -  F1 */
		m1 = MAGNITUDE( work );
		r2 = 0.5 * dd[6] / m1;
		VSCALE( D(1), work, r2 );	/* A */

		dd[6] = sqrt( MAGSQ( D(1) ) -
			(m1 * 0.5)*(m1 * 0.5) );	/* r */
		VMOVE( D(0), v );
		goto ell1;
	}

	if( (version == 5 && strcmp( solid_type, "ell" ) == 0)  ||
	    strcmp( solid_type, "ell1" ) == 0 )  {
		/* V, A, r */
	    	/* GIFT4 name is "ell1", GIFT5 name is "ell" */
		if( getsoldata( dd, 2*3+1, sol_work ) < 0 )
			return(-1);

ell1:
		r1 = dd[6];		/* R */
		VMOVE( work, D(0) );
		work[0] += PI;
		work[1] += PI;
		work[2] += PI;
		VCROSS( D(2), work, D(1) );
		m1 = r1/MAGNITUDE( D(2) );
		VSCALE( D(2), D(2), m1 );

		VCROSS( D(3), D(1), D(2) );
		m2 = r1/MAGNITUDE( D(3) );
		VSCALE( D(3), D(3), m2 );

		/* Now we have V, A, B, C */
		return( mk_ell( outfp, name, D(0), D(1), D(2), D(3) ) );
	}

	if( strcmp( solid_type, "ellg" ) == 0 )  {
		/* V, A, B, C */
		if( getsoldata( dd, 4*3, sol_work ) < 0 )
			return(-1);
		return( mk_ell( outfp, name, D(0), D(1), D(2), D(3) ) );
	}

	if( strcmp( solid_type, "tor" ) == 0 )  {
		/* V, N, r1, r2 */
		if( getsoldata( dd, 2*3+2, sol_work ) < 0 )
			return(-1);
		return( mk_tor( outfp, name, D(0), D(1), dd[6], dd[7] ) );
	}

	if( strcmp( solid_type, "haf" ) == 0 )  {
		/* N, d */
		if( getsoldata( dd, 1*3+1, sol_work ) < 0 )
			return(-1);
		return( mk_half( outfp, name, D(0), dd[3] ) );
	}

	if( strcmp( solid_type, "arbn" ) == 0 )  {
		return( read_arbn( name ) );
	}

	/*
	 *  The solid type string is defective,
	 *  or that solid is not currently supported.
	 */
	printf("getsolid:  no support for solid type '%s'\n", solid_type );
	return(-1);
}

read_arbn( name )
char	*name;
{
	int	npt;			/* # vertex pts to be read in */
	int	npe;			/* # planes from 3 vertex points */
	int	neq;			/* # planes from equation */
	int	nae;			/* # planes from az,el & vertex index */
	int	nface;			/* total number of faces */
	double	*input_points = (double *)0;
	double	*vertex = (double *)0;	/* vertex list of final solid */
	int	last_vertex;		/* index of first unused vertex */
	int	max_vertex;		/* size of vertex array */
	int	*used = (int *)0;	/* plane eqn use count */
	plane_t	*eqn = (plane_t *)0;	/* plane equations */
	int	cur_eq = 0;		/* current (free) equation number */
	int	symm = 0;		/* symmetry about Y used */
	register int	i;
	int	j;
	int	k;
	register int	m;
	point_t	cent;			/* centroid of arbn */

	npt = getint( scard, 10+0*10, 10 );
	npe = getint( scard, 10+1*10, 10 );
	neq = getint( scard, 10+2*10, 10 );
	nae = getint( scard, 10+3*10, 10 );

	nface = npe + neq + nae;
	if( npt < 1 )  {
		/* Having one point is necessary to compute centroid */
		printf("arbn defined without at least one point\n");
bad:
		if(npt>0) eat( (npt+1)/2 );	/* vertex input_points */
		if(npe>0) eat( (npe+5)/6 );	/* vertex pt index numbers */
		if(neq>0) eat( neq );		/* plane eqns? */
		if(nae>0) eat( (nae+1)/2 );	/* az el & vertex index? */
		return(-1);
	}

	/* Allocate storage for plane equations */
	if( (eqn = (plane_t *)malloc(nface*sizeof(plane_t))) == (plane_t *)0 )  {
		printf("arbn plane equation malloc failure\n");
		goto bad;
	}
	/* Allocate storage for per-plane use count */
	if( (used = (int *)malloc(nface*sizeof(int))) == (int *)0 )  {
		printf("arbn use count malloc failure\n");
		goto bad;
	}

	if( npt >= 1 )  {
		/* Obtain vertex input_points */
		if( (input_points = (double *)malloc(npt*3*sizeof(double))) == (double *)0 )  {
			printf("arbn point malloc failure\n");
			goto bad;
		}
		if( getxsoldata( input_points, npt*3, sol_work ) < 0 )
			goto bad;
	}

	/* Get planes defined by three points, 6 per card */
	for( i=0; i<npe; i += 6 )  {
		if( getline( scard, sizeof(scard), "arbn vertex point indices" ) == EOF )  {
			printf("too few cards for arbn %d\n",
				sol_work);
			return(-1);
		}
		for( j=0; j<6; j++ )  {
			int	q,r,s;
			point_t	a,b,c;

			q = getint( scard, 10+j*10+0, 4 );
			r = getint( scard, 10+j*10+4, 3 );
			s = getint( scard, 10+j*10+7, 3 );

			if( q == 0 || r == 0 || s == 0 ) continue;

			if( q < 0 )  {
				VMOVE( a, &input_points[((-q)-1)*3] );
				a[Y] = -a[Y];
				symm = 1;
			} else {
				VMOVE( a, &input_points[((q)-1)*3] );
			}

			if( r < 0 )  {
				VMOVE( b, &input_points[((-r)-1)*3] );
				b[Y] = -b[Y];
				symm = 1;
			} else {
				VMOVE( b, &input_points[((r)-1)*3] );
			}

			if( s < 0 )  {
				VMOVE( c, &input_points[((-s)-1)*3] );
				c[Y] = -c[Y];
				symm = 1;
			} else {
				VMOVE( c, &input_points[((s)-1)*3] );
			}
			if( rt_mk_plane_3pts( eqn[cur_eq], a,b,c ) < 0 )  {
				printf("arbn degenerate plane\n");
				VPRINT("a", a);
				VPRINT("b", b);
				VPRINT("c", c);
				continue;
			}
			cur_eq++;
		}
	}

	/* Get planes defined by their equation */
	for( i=0; i < neq; i++ )  {
		register double	scale;
		if( getline( scard, sizeof(scard), "arbn plane equation card" ) == EOF )  {
			printf("too few cards for arbn %d\n",
				sol_work);
			return(-1);
		}
		eqn[cur_eq][0] = getdouble( scard, 10+0*10, 10 );
		eqn[cur_eq][1] = getdouble( scard, 10+1*10, 10 );
		eqn[cur_eq][2] = getdouble( scard, 10+2*10, 10 );
		eqn[cur_eq][3] = getdouble( scard, 10+3*10, 10 );
		scale = MAGNITUDE(eqn[cur_eq]);
		if( scale < SMALL )  {
			printf("arbn plane normal too small\n");
			continue;
		}
		scale = 1/scale;
		VSCALE( eqn[cur_eq], eqn[cur_eq], scale );
		eqn[cur_eq][3] *= scale;
		cur_eq++;
	}

	/* Get planes defined by azimuth, elevation, and pt, 2 per card */
	for( i=0; i < nae;  i += 2 )  {
		if( getline( scard, sizeof(scard), "arbn az/el card" ) == EOF )  {
			printf("too few cards for arbn %d\n",
				sol_work);
			return(-1);
		}
		for( j=0; j<2; j++ )  {
			double	az, el;
			int	vertex;
			double	cos_el;
			point_t	pt;

			az = getdouble( scard, 10+j*30+0*10, 10 ) * DEG2RAD;
			el = getdouble( scard, 10+j*30+1*10, 10 ) * DEG2RAD;
			vertex = getint( scard, 10+j*30+2*10, 10 );
			if( vertex == 0 )  break;
			cos_el = cos(el);
			eqn[cur_eq][X] = cos(az)*cos_el;
			eqn[cur_eq][Y] = sin(az)*cos_el;
			eqn[cur_eq][Z] = sin(el);

			if( vertex < 0 )  {
				VMOVE( pt, &input_points[((-vertex)-1)*3] );
				pt[Y] = -pt[Y];
			} else {
				VMOVE( pt, &input_points[((vertex)-1)*3] );
			}
			eqn[cur_eq][3] = VDOT(pt, eqn[cur_eq]);
			cur_eq++;
		}
	}
	if( nface != cur_eq )  {
		printf("arbn expected %d faces, got %d\n", nface, cur_eq);
		return(-1);
	}

	/* Average all given points together to find centroid */
	/* This is why there must be at least one (two?) point given */
	VSETALL(cent, 0);
	for( i=0; i<npt; i++ )  {
		VADD2( cent, cent, &input_points[i*3] );
	}
	VSCALE( cent, cent, 1.0/npt );
	if( symm )  cent[Y] = 0;

	/* Point normals away from centroid */
	for( i=0; i<nface; i++ )  {
		double	dist;

		dist = VDOT( eqn[i], cent ) - eqn[i][3];
		/* If dist is negative, 'cent' is inside halfspace */
#define DIST_TOL	(1.0e-8)
#define DIST_TOL_SQ	(1.0e-10)
		if( dist < -DIST_TOL )  continue;
		if( dist > DIST_TOL )  {
			/* Flip halfspace over */
			VREVERSE( eqn[i], eqn[i] );
			eqn[i][3] = -eqn[i][3];
		} else {
			/* Centroid lies on this face */
			printf("arbn centroid lies on face\n");
			return(-1);
		}
		
	}

	/* Release storage for input points */
	free( (char *)input_points );
	input_points = (double *)0;


	/*
	 *  ARBN must be convex.  Test for concavity.
	 *  Byproduct is an enumeration of all the verticies.
	 */
	last_vertex = max_vertex = 0;

	/* Zero face use counts */
	for( i=0; i<nface; i++ )  {
		used[i] = 0;
	}
	for( i=0; i<nface-2; i++ )  {
		for( j=i+1; j<nface-1; j++ )  {
			double	dot;
			int	point_count;	/* # points on this line */

			/* If normals are parallel, no intersection */
			dot = VDOT( eqn[i], eqn[j] );
			if( !NEAR_ZERO( dot, 0.999999 ) )  continue;

			point_count = 0;
			for( k=j+1; k<nface; k++ )  {
				point_t	pt;

				if( rt_mkpoint_3planes( pt, eqn[i], eqn[j], eqn[k] ) < 0 )  continue;

				/* See if point is outside arb */
				for( m=0; m<nface; m++ )  {
					if( i==m || j==m || k==m )  continue;
					if( VDOT(pt, eqn[m])-eqn[m][3] > DIST_TOL )
						goto next_k;
				}
				/* See if vertex already was found */
				for( m=0; m<last_vertex; m++ )  {
					vect_t	dist;
					VSUB2( dist, pt, &vertex[m*3] );
					if( MAGSQ(dist) < DIST_TOL_SQ )
						goto next_k;
				}

				/*
				 *  Add point to vertex array.
				 *  If more room needed, realloc.
				 */
				if( last_vertex >= max_vertex )  {
					if( max_vertex == 0 )   {
						max_vertex = 3;
						vertex = (double *)malloc( max_vertex*3*sizeof(double) );
					} else {
						max_vertex *= 10;
						vertex = (double *)realloc( (char *)vertex, max_vertex*3*sizeof(double) );
					}
					if( vertex == (double *)0 )  {
						printf("arbn vertex realloc fail\n");
						goto bad;
					}
				}

				VMOVE( &vertex[last_vertex*3], pt );
				last_vertex++;
				point_count++;

				/* Increment "face used" counts */
				used[i]++;
				used[j]++;
				used[k]++;
next_k:				;
			}
			if( point_count > 2 )  {
				printf("arbn: warning, point_count on line=%d\n", point_count);
			}
		}
	}

	/* If any planes were not used, then arbn is not convex */
	for( i=0; i<nface; i++ )  {
		if( used[i] != 0 )  continue;	/* face was used */
		printf("arbn face %d unused, solid is not convex\n", i);
		return(-1);
	}

	/* Write out the solid ! */
	i = mk_arbn( outfp, name, nface, eqn );

	if( input_points )  free( (char *)input_points );
	if( vertex )  free( (char *)vertex );
	if( eqn )  free( (char *)eqn );
	if( used )  free( (char *)used );

	return(i);
}

/*
 *			E A T
 *
 *  Eat the indicated number of input lines
 */
eat( count )
int	count;
{
	char	lbuf[132];
	int	i;

	for( i=0; i<count; i++ )  {
		getline( lbuf, sizeof(lbuf), "eaten card" );
	}
}
