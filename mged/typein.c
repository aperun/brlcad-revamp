/*
 *  			T Y P E I N
 *
 * This module contains functions which allow solid parameters to
 * be entered by keyboard.
 *
 * Functions -
 *	f_in		decides what solid needs to be entered and
 *			calls the appropriate solid parameter reader
 *	arb_in		reads ARB params from keyboard
 *	sph_in		reads sphere params from keyboard
 *	ell_in		reads params for all ELLs
 *	tor_in		gets params for torus from keyboard
 *	tgc_in		reads params for TGC from keyboard
 *	rcc_in		reads params for RCC from keyboard
 *	rec_in		reads params for REC from keyboard
 *	tec_in		reads params for TEC from keyboard
 *	trc_in		reads params for TRC from keyboard
 *	box_in		gets params for BOX and RAW from keyboard
 *	rpp_in		gets params for RPP from keyboard
 *	ars_in		gets ARS param from keyboard
 *	half_in		gets HALFSPACE params from keyboard
 *	rpc_in		reads right parabolic cylinder params from keyboard
 *	rhc_in		reads right hyperbolic cylinder params from keyboard
 *	epa_in		reads elliptical paraboloid params from keyboard
 *	ehy_in		reads elliptical hyperboloid params from keyboard
 *	eto_in		reads elliptical torus params from keyboard
 *	checkv		checks for zero vector from keyboard
 *	getcmd		reads and parses input parameters from keyboard
 *
 * Authors -
 *	Charles M. Kennedy
 *	Keith A. Applin
 *	Michael J. Muuss
 *	Michael J. Markowski
 *
 * Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *
 * Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <signal.h>
#include <stdio.h>
#include <math.h>
#ifdef BSD
#include <strings.h>
#else
#include <string.h>
#endif

#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "rtgeom.h"
#include "./ged.h"
#include "./dm.h"

void	aexists();
extern void f_quit();

int		args;		/* total number of args available */
int		argcnt;		/* holder for number of args added later */
int		vals;		/* number of args for s_values[] */
int		newargs;	/* number of args from getcmd() */
extern int	numargs;	/* number of args */
extern int	maxargs;	/* size of cmd_args[] */
extern char	*cmd_args[];	/* array of pointers to args */
char		**promp;	/* the prompt string */

char *p_half[] = {
	"Enter X, Y, Z of outward pointing normal vector: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter the distance from the origin: "
};

char *p_ars[] = {
	"Enter number of points per waterline, and number of waterlines: ",
	"Enter number of waterlines: ",
	"Enter X, Y, Z for First row point: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X  Y  Z",
	"Enter Y",
	"Enter Z",
};

char *p_arb[] = {
	"Enter X, Y, Z for point 1: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z for point 2: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z for point 3: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z for point 4: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z for point 5: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z for point 6: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z for point 7: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z for point 8: ",
	"Enter Y: ",
	"Enter Z: "
};

char *p_sph[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter radius: "
};

char *p_ell[] = {
	"Enter X, Y, Z of focus point 1: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of focus point 2: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter axis length L: "
};

char *p_ell1[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector A: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter radius of revolution: "
};

char *p_ellg[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector A: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector B: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector C: ",
	"Enter Y: ",
	"Enter Z: "
};

char *p_tor[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of normal vector: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter radius 1: ",
	"Enter radius 2: "
};

char *p_rcc[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of height (H) vector: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter radius: "
};

char *p_tec[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of height (H) vector: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector A: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector B: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter ratio: "
};

char *p_rec[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of height (H) vector: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector A: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector B: ",
	"Enter Y: ",
	"Enter Z: "
};

char *p_trc[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of height (H) vector: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter radius of base: ",
	"Enter radius of top: "
};

char *p_tgc[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of height (H) vector: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector A: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector B: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter scalar c: ",
	"Enter scalar d: "
};

char *p_box[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector H: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector W: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector D: ",
	"Enter Y: ",
	"Enter Z: "
};

char *p_rpp[] = {
	"Enter XMIN, XMAX, YMIN, YMAX, ZMIN, ZMAX: ",
	"Enter XMAX: ",
	"Enter YMIN, YMAX, ZMIN, ZMAX: ",
	"Enter YMAX: ",
	"Enter ZMIN, ZMAX: ",
	"Enter ZMAX: "
};

char *p_rpc[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z, of vector H: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z, of vector B: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter rectangular half-width, r: "
};

char *p_rhc[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z, of vector H: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z, of vector B: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter rectangular half-width, r: ",
	"Enter apex-to-asymptotes distance, c: "
};

char *p_epa[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z, of vector H: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z, of vector A: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter magnitude of vector B: "
};

char *p_ehy[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z, of vector H: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z, of vector A: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter magnitude of vector B: ",
	"Enter apex-to-asymptotes distance, c: "
};

char *p_eto[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z, of normal vector: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z, of vector C: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter radius of revolution, r: ",
	"Enter elliptical semi-minor axis, d: "
};

/*	F _ I N ( ) :  	decides which solid reader to call
 *			Used for manual entry of solids.
 */
int
f_in()
{
	register int i;
	register struct directory *dp;
	char			name[NAMESIZE+2];
	struct rt_db_internal	internal;
	struct rt_external	external;
	char			*new_cmd[3], **menu;
	int			ngran;		/* number of db granules */
	int			id;
	int			nvals, (*fn_in)();
	int			arb_in(), box_in(), ehy_in(), ell_in(),
				epa_in(), eto_in(), half_in(), rec_in(),
				rcc_in(), rhc_in(), rpc_in(), rpp_in(),
				sph_in(), tec_in(), tgc_in(), tor_in(),
				trc_in();

	(void)signal( SIGINT, sig2);	/* allow interrupts */

	/* Save the number of args loaded initially */
	args = numargs;
	argcnt = 0;
	vals = 0;

	/* Get the name of the solid to be created */
	while( args < 2 )  {
		(void)printf("Enter name of solid: ");
		argcnt = getcmd(args);
		/* Add any new args slurped up */
		args += argcnt;
	}
	if( db_lookup( dbip,  cmd_args[1], LOOKUP_QUIET ) != DIR_NULL )  {
		aexists( cmd_args[1] );
		return CMD_BAD;
	}
	if( (int)strlen(cmd_args[1]) >= NAMESIZE )  {
		(void)printf("ERROR, names are limited to %d characters\n", NAMESIZE-1);
		return CMD_BAD;
	}
	/* Save the solid name since cmd_args[] might get bashed */
	strcpy( name, cmd_args[1] );

	/* Get the solid type to be created and make it */
	while( args < 3 )  {
		(void)printf("Enter solid type: ");
		argcnt = getcmd(args);
		/* Add any new args slurped up */
		args += argcnt;
	}
	/*
	 * Decide which solid to make and get the rest of the args
	 * make name <half|arb[4-8]|sph|ell|ellg|ell1|tor|tgc|tec|
			rec|trc|rcc|box|raw|rpp|rpc|rhc|epa|ehy|eto>
	 */
	if( strcmp( cmd_args[2], "ebm" ) == 0 )  {
		if( strsol_in( &external, "ebm" ) < 0 )  {
			(void)printf("ERROR, EBM solid not made!\n");
			return CMD_BAD;
		}
		goto do_extern_update;
	} else if( strcmp( cmd_args[2], "vol" ) == 0 )  {
		if( strsol_in( &external, "vol" ) < 0 )  {
			(void)printf("ERROR, VOL solid not made!\n");
			return CMD_BAD;
		}
		goto do_extern_update;
	} else if( strcmp( cmd_args[2], "ars" ) == 0 )  {
		if (ars_in(args, cmd_args, &internal, &p_ars[0]) != 0)  {
			(void)printf("ERROR, ars not made!\n");
			if(internal.idb_type) rt_functab[internal.idb_type].
				ft_ifree( &internal );
			return CMD_BAD;
		}
		goto do_new_update;
	} else if( strcmp( cmd_args[2], "half" ) == 0 )  {
		nvals = 3*1 + 1;
		menu = p_half;
		fn_in = half_in;
	} else if( strncmp( cmd_args[2], "arb", 3 ) == 0 )  {
		nvals = 3*atoi(&cmd_args[2][3]);
		menu = p_arb;
		fn_in = arb_in;
	} else if( strcmp( cmd_args[2], "sph" ) == 0 )  {
		nvals = 3*1 + 1;
		menu = p_sph;
		fn_in = sph_in;
	} else if( strcmp( cmd_args[2], "ell" ) == 0 )  {
		nvals = 3*2 + 1;
		menu = p_ell;
		fn_in = ell_in;
	} else if( strcmp( cmd_args[2], "ellg" ) == 0 )  {
		nvals = 3*4;
		menu = p_ellg;
		fn_in = ell_in;
	} else if( strcmp( cmd_args[2], "ell1" ) == 0 )  {
		nvals = 3*2 + 1;
		menu = p_ell1;
		fn_in = ell_in;
	} else if( strcmp( cmd_args[2], "tor" ) == 0 )  {
		nvals = 3*2 + 2;
		menu = p_tor;
		fn_in = tor_in;
	} else if( strcmp( cmd_args[2], "tgc" ) == 0 ) {
		nvals = 3*4 + 2;
		menu = p_tgc;
		fn_in = tgc_in;
	} else if( strcmp( cmd_args[2], "tec" ) == 0 )  {
		nvals = 3*4 + 1;
		menu = p_tec;
		fn_in = tec_in;
	} else if( strcmp( cmd_args[2], "rec" ) == 0 )  {
		nvals = 3*4;
		menu = p_rec;
		fn_in = rec_in;
	} else if( strcmp( cmd_args[2], "trc" ) == 0 )  {
		nvals = 3*2 + 2;
		menu = p_trc;
		fn_in = trc_in;
	} else if( strcmp( cmd_args[2], "rcc" ) == 0 )  {
		nvals = 3*2 + 1;
		menu = p_rcc;
		fn_in = rcc_in;
	} else if( strcmp( cmd_args[2], "box" ) == 0 
		|| strcmp( cmd_args[2], "raw" ) == 0 )  {
		nvals = 3*4;
		menu = p_box;
		fn_in = box_in;
	} else if( strcmp( cmd_args[2], "rpp" ) == 0 )  {
		nvals = 6*1;
		menu = p_rpp;
		fn_in = rpp_in;
	} else if( strcmp( cmd_args[2], "rpc" ) == 0 )  {
		nvals = 3*3 + 1;
		menu = p_rpc;
		fn_in = rpc_in;
	} else if( strcmp( cmd_args[2], "rhc" ) == 0 )  {
		nvals = 3*3 + 2;
		menu = p_rhc;
		fn_in = rhc_in;
	} else if( strcmp( cmd_args[2], "epa" ) == 0 )  {
		nvals = 3*3 + 1;
		menu = p_epa;
		fn_in = epa_in;
	} else if( strcmp( cmd_args[2], "ehy" ) == 0 )  {
		nvals = 3*3 + 2;
		menu = p_ehy;
		fn_in = ehy_in;
	} else if( strcmp( cmd_args[2], "eto" ) == 0 )  {
		nvals = 3*3 + 2;
		menu = p_eto;
		fn_in = eto_in;
	} else {
		(void)printf("f_in:  %s is not a known primitive\n", cmd_args[2]);
		return CMD_BAD;
	}
	
	/* Read arguments */
	if( args < 3+nvals )  {
		(void)printf("%s", menu[args-3]);
		return CMD_MORE;
	}

	RT_INIT_DB_INTERNAL( &internal );
	if (fn_in(cmd_args, &internal, menu) != 0)  {
		(void)printf("ERROR, %s not made!\n", cmd_args[2]);
		if(internal.idb_type) rt_functab[internal.idb_type].
			ft_ifree( &internal );
		return CMD_BAD;
	}

do_new_update:
	RT_CK_DB_INTERNAL( &internal );
	id = internal.idb_type;
	if( rt_functab[id].ft_export( &external, &internal, local2base ) < 0 )  {
		printf("export failure\n");
		rt_functab[id].ft_ifree( &internal );
		return CMD_BAD;
	}
	rt_functab[id].ft_ifree( &internal );	/* free internal rep */

do_extern_update:
	/* don't allow interrupts while we update the database! */
	(void)signal( SIGINT, SIG_IGN);

	ngran = (external.ext_nbytes+sizeof(union record)-1) / sizeof(union record);
	if ((dp=db_diradd(dbip, name, -1L, ngran, DIR_SOLID)) == DIR_NULL ||
	     db_alloc(dbip, dp, ngran ) < 0) {
		db_free_external( &external );
	    	ALLOC_ERR;
		return CMD_BAD;
	}
	if (db_put_external( &external, dp, dbip ) < 0 )  {
		db_free_external( &external );
		WRITE_ERR;
		return CMD_BAD;
	}
	db_free_external( &external );

	/* draw the "made" solid */
	new_cmd[0] = "e";
	new_cmd[1] = name;
	new_cmd[2] = (char *)NULL;
	return f_edit( 2, new_cmd );
}

/*
 *			S T R S O L _ I N
 *
 *  Read string solid info from keyboard
 *  "in" name ebm|vol arg(s)
 */
int
strsol_in( ep, sol )
struct rt_external	*ep;
char			*sol;
{
	struct rt_vls	str;
	union record	*rec;

	/* Read at least one "arg(s)" */
	while( args < (3 + 1) )  {
		(void)printf("%s Arg? ", sol);
		if( (argcnt = getcmd(args)) < 0 )  {
			return(-1);	/* failure */
		}
		args += argcnt;
	}

	RT_INIT_EXTERNAL(ep);
	ep->ext_nbytes = sizeof(union record)*DB_SS_NGRAN;
	ep->ext_buf = (genptr_t)rt_calloc( 1, ep->ext_nbytes, "ebm external");
	rec = (union record *)ep->ext_buf;

	RT_VLS_INIT( &str );
	rt_vls_from_argv( &str, args-3, &cmd_args[3] );

	rec->ss.ss_id = DBID_STRSOL;
	strncpy( rec->ss.ss_keyword, "ebm", NAMESIZE-1 );
	strncpy( rec->ss.ss_args, rt_vls_addr(&str), DB_SS_LEN-1 );
	rt_vls_free( &str );

	return(0);		/* OK */
}

/*
 *			A R S _ I N
 */
int
ars_in( argc, argv, intern, promp )
int			argc;
char			**argv;
struct rt_db_internal	*intern;
char			*promp[];
{
	register struct rt_ars_internal	*arip;
	register int			i;
	int			total_points;
	int			cv;	/* current curve (waterline) # */
	int			axis;	/* current fastf_t in waterline */
	int			ncurves_minus_one;

	while (args < 5) {
		(void)printf("%s", promp[args-3]);
		if ((argcnt=getcmd(args)) < 0) {
			return(1);	/* failure */
		}
		args += argcnt;
	}

	intern->idb_type = ID_ARS;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_ars_internal), "rt_ars_internal");
	arip = (struct rt_ars_internal *)intern->idb_ptr;
	arip->magic = RT_ARS_INTERNAL_MAGIC;

	if ((arip->pts_per_curve = atoi(cmd_args[3])) < 3 ||
	    (arip->ncurves =  atoi(cmd_args[4])) < 3 ) {
	    	printf("Invalid number of lines or pts_per_curve\n");
		return(1);
	}
	printf("Waterlines: %d, curve points: %d\n", arip->ncurves, arip->pts_per_curve);
	ncurves_minus_one = arip->ncurves - 1;

	total_points = arip->ncurves * arip->pts_per_curve;

	arip->curves = (fastf_t **)rt_malloc(
		(arip->ncurves+1) * sizeof(fastf_t **), "ars curve ptrs" );
	for( i=0; i < arip->ncurves; i++ )  {
		/* Leave room for first point to be repeated */
		arip->curves[i] = (fastf_t *)rt_malloc(
		    (arip->pts_per_curve+1) * sizeof(point_t),
		    "ars curve" );
	}

	while (args < 8) {
		(void)printf("%s", promp[args-3]);
		if ((argcnt=getcmd(args)) < 0) {
			return(1);	/* failure */
		}
		args += argcnt;
	}
	/* fill in the point of the first row */
	arip->curves[0][0] = atof(cmd_args[5]);
	arip->curves[0][1] = atof(cmd_args[6]);
	arip->curves[0][2] = atof(cmd_args[7]);

	/* The first point is duplicated across the first curve */
	for (i=1 ; i < arip->pts_per_curve ; ++i) {
		VMOVE( arip->curves[0]+3*i, arip->curves[0] );
	}

	cv = 1;
	axis = 0;
	/* scan each of the other points we've already got */
	for (i=8 ; i < args && i < total_points * ELEMENTS_PER_PT ; ++i) {

		arip->curves[cv][axis] = atof(cmd_args[i]);
		if (++axis >= arip->pts_per_curve * ELEMENTS_PER_PT) {
			axis = 0;
			cv++;
		}
	}

	/* go get the waterline points from the user */
	while( cv < arip->ncurves )  {
		if (cv < ncurves_minus_one)
			(void)printf("%s for Waterline %d, Point %d : ",
				promp[5+axis%3], cv, axis/3 );
		else
			(void)printf("%s for point of last waterline : ",
				promp[5+axis%3] );

		/* Get some more input */
		*cmd_args[0] = '\0';
		if ((argcnt = getcmd(1)) < 0)
			return(1);

		/* scan each of the args we've already got */
		for (i=1 ; i < argcnt+1 &&
		    cv < arip->ncurves && axis < 3*arip->pts_per_curve; i++ )  {
			arip->curves[cv][axis] = atof(cmd_args[i]);
			if (++axis >= arip->pts_per_curve * ELEMENTS_PER_PT) {
				axis = 0;
				cv++;
			}
		}
		if( cv >= ncurves_minus_one && axis >= ELEMENTS_PER_PT )  break;
	}

	/* The first point is duplicated across the last curve */
	for (i=1 ; i < arip->pts_per_curve ; ++i) {
		VMOVE( arip->curves[ncurves_minus_one]+3*i,
			arip->curves[ncurves_minus_one] );
	}
	return(0);
}

/*   H A L F _ I N ( ) :   	reads halfspace parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
half_in(cmd_argvs, intern)
char			*cmd_argvs[];
struct rt_db_internal	*intern;
{
	int			i;
	struct rt_half_internal	*hip;

	intern->idb_type = ID_HALF;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_half_internal),
		"rt_half_internal" );
	hip = (struct rt_half_internal *)intern->idb_ptr;
	hip->magic = RT_HALF_INTERNAL_MAGIC;

	for (i = 0; i < ELEMENTS_PER_PLANE; i++) {
		hip->eqn[i] = atof(cmd_argvs[3+i]);
	}
	VUNITIZE( hip->eqn );
	
	if (MAGNITUDE(hip->eqn) < RT_LEN_TOL) {
		(void)printf("ERROR, normal vector is too small!\n");
		return(1);	/* failure */
	}
	
	return(0);	/* success */
}

/*   A R B _ I N ( ) :   	reads arb parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
arb_in(cmd_argvs, intern)
char			*cmd_argvs[];
struct rt_db_internal	*intern;
{
	int			argcnt, i, j, n;
	struct rt_arb_internal	*aip;

	intern->idb_type = ID_ARB8;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_arb_internal),
		"rt_arb_internal" );
	aip = (struct rt_arb_internal *)intern->idb_ptr;
	aip->magic = RT_ARB_INTERNAL_MAGIC;

	n = atoi(&cmd_argvs[2][3]);	/* get # from "arb#" */
	for (j = 0; j < n; j++)
		for (i = 0; i < ELEMENTS_PER_PT; i++)
			aip->pt[j][i] = atof(cmd_argvs[3+i+3*j]);

	if (!strcmp("arb4", cmd_argvs[2])) {
		VMOVE( aip->pt[7], aip->pt[3] );
		VMOVE( aip->pt[6], aip->pt[3] );
		VMOVE( aip->pt[5], aip->pt[3] );
		VMOVE( aip->pt[4], aip->pt[3] );
		VMOVE( aip->pt[3], aip->pt[0] );
	} else if (!strcmp("arb5", cmd_argvs[2])) {
		VMOVE( aip->pt[7], aip->pt[4] );
		VMOVE( aip->pt[6], aip->pt[4] );
		VMOVE( aip->pt[5], aip->pt[4] );
	} else if (!strcmp("arb6", cmd_argvs[2])) {
		VMOVE( aip->pt[7], aip->pt[5] );
		VMOVE( aip->pt[6], aip->pt[5] );
		VMOVE( aip->pt[5], aip->pt[4] );
	} else if (!strcmp("arb7", cmd_argvs[2])) {
		VMOVE( aip->pt[7], aip->pt[4] );
	}

	return(0);	/* success */
}

/*   S P H _ I N ( ) :   	reads sph parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
sph_in(cmd_argvs, intern)
char			*cmd_argvs[];
struct rt_db_internal	*intern;
{
	fastf_t			r;
	int			i;
	struct rt_ell_internal	*sip;

	intern->idb_type = ID_ELL;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_ell_internal),
		"rt_ell_internal" );
	sip = (struct rt_ell_internal *)intern->idb_ptr;
	sip->magic = RT_ELL_INTERNAL_MAGIC;

	for (i = 0; i < ELEMENTS_PER_PT; i++) {
		sip->v[i] = atof(cmd_argvs[3+i]);
	}
	r = atof(cmd_argvs[6]);
	VSET( sip->a, r, 0., 0. );
	VSET( sip->b, 0., r, 0. );
	VSET( sip->c, 0., 0., r );
	
	if (r < RT_LEN_TOL) {
		(void)printf("ERROR, radius must be greater than zero!\n");
		return(1);	/* failure */
	}
	
	return(0);	/* success */
}

/*   E L L _ I N ( ) :   	reads ell parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
ell_in(cmd_argvs, intern)
char			*cmd_argvs[];
struct rt_db_internal	*intern;
{
	fastf_t			len, mag_b, r_rev, vals[12];
	int			i, n;
	struct rt_ell_internal	*eip;

	n = 7;				/* ELL and ELL1 have seven params */
	if (cmd_argvs[2][3] == 'g')	/* ELLG has twelve */
		n = 12;

	intern->idb_type = ID_ELL;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_ell_internal),
		"rt_ell_internal" );
	eip = (struct rt_ell_internal *)intern->idb_ptr;
	eip->magic = RT_ELL_INTERNAL_MAGIC;

	/* convert typed in args to reals */
	for (i = 0; i < n; i++) {
		vals[i] = atof(cmd_argvs[3+i]);
	}

	if (!strcmp("ellg", cmd_argvs[2])) {	/* everything's ok */
		/* V, A, B, C */
		VMOVE( eip->v, &vals[0] );
		VMOVE( eip->a, &vals[3] );
		VMOVE( eip->b, &vals[6] );
		VMOVE( eip->c, &vals[9] );
		return(0);
	}
	
	if (!strcmp("ell", cmd_argvs[2])) {
		/* V, f1, f2, len */
		/* convert ELL format into ELL1 format */
		len = vals[6];
		/* V is halfway between the foci */
		VADD2( eip->v, &vals[0], &vals[3] );
		VSCALE( eip->v, eip->v, 0.5);
		VSUB2( eip->b, &vals[3], &vals[0] );
		mag_b = MAGNITUDE( eip->b );
		if ( NEAR_ZERO( mag_b, RT_LEN_TOL )) {
			fprintf(stderr, "ERROR, foci are coincident!\n");
			return(1);
		}
		/* calculate A vector */
		VSCALE( eip->a, eip->b, .5*len/mag_b );
		/* calculate radius of revolution (for ELL1 format) */
		r_rev = sqrt( MAGSQ( eip->a ) - (mag_b*.5)*(mag_b*.5) );
	} else if (!strcmp("ell1", cmd_argvs[2])) {
		/* V, A, r */
		VMOVE( eip->v, &vals[0] );
		VMOVE( eip->a, &vals[3] );
		r_rev = vals[6];
	}
	
	/* convert ELL1 format into ELLG format */
	/* calculate B vector */
	vec_ortho( eip->b, eip->a );
	VUNITIZE( eip->b );
	VSCALE( eip->b, eip->b, r_rev);

	/* calculate C vector */
	VCROSS( eip->c, eip->a, eip->b );
	VUNITIZE( eip->c );
	VSCALE( eip->c, eip->c, r_rev );
	return(0);	/* success */
}

/*   T O R _ I N ( ) :   	reads tor parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
tor_in(cmd_argvs, intern)
char			*cmd_argvs[];
struct rt_db_internal	*intern;
{
	fastf_t			r;
	int			i;
	struct rt_tor_internal	*tip;

	intern->idb_type = ID_TOR;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_tor_internal),
		"rt_tor_internal" );
	tip = (struct rt_tor_internal *)intern->idb_ptr;
	tip->magic = RT_TOR_INTERNAL_MAGIC;

	for (i = 0; i < ELEMENTS_PER_PT; i++) {
		tip->v[i] = atof(cmd_argvs[3+i]);
		tip->h[i] = atof(cmd_argvs[6+i]);
	}
	tip->r_a = atof(cmd_argvs[9]);
	tip->r_h = atof(cmd_argvs[10]);
	/* Check for radius 2 >= radius 1 */
	if( tip->r_a <= tip->r_h )  {
		(void)printf("ERROR, radius 2 >= radius 1 ....\n");
		return(1);	/* failure */
	}
	
	if (MAGNITUDE( tip->h ) < RT_LEN_TOL) {
		(void)printf("ERROR, normal must be greater than zero!\n");
		return(1);	/* failure */
	}
	
	return(0);	/* success */
}

/*   T G C _ I N ( ) :   	reads tgc parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
tgc_in(cmd_argvs, intern)
char			*cmd_argvs[];
struct rt_db_internal	*intern;
{
	fastf_t			r1, r2;
	int			i;
	struct rt_tgc_internal	*tip;

	intern->idb_type = ID_TGC;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_tgc_internal),
		"rt_tgc_internal" );
	tip = (struct rt_tgc_internal *)intern->idb_ptr;
	tip->magic = RT_TGC_INTERNAL_MAGIC;

	for (i = 0; i < ELEMENTS_PER_PT; i++) {
		tip->v[i] = atof(cmd_argvs[3+i]);
		tip->h[i] = atof(cmd_argvs[6+i]);
		tip->a[i] = atof(cmd_argvs[9+i]);
		tip->b[i] = atof(cmd_argvs[12+i]);
	}
	r1 = atof(cmd_argvs[15]);
	r2 = atof(cmd_argvs[16]);
	
	if (MAGNITUDE(tip->h) < RT_LEN_TOL
		|| MAGNITUDE(tip->a) < RT_LEN_TOL
		|| MAGNITUDE(tip->b) < RT_LEN_TOL
		|| r1 < RT_LEN_TOL || r2 < RT_LEN_TOL) {
		(void)printf("ERROR, all dimensions must be greater than zero!\n");
		return(1);	/* failure */
	}

	/* calculate C */
	VMOVE( tip->c, tip->a );
	VUNITIZE( tip->c );
	VSCALE( tip->c, tip->c, r1);

	/* calculate D */
	VMOVE( tip->d, tip->b );
	VUNITIZE( tip->d );
	VSCALE( tip->d, tip->d, r2);
	
	return(0);	/* success */
}

/*   R C C _ I N ( ) :   	reads rcc parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
rcc_in(cmd_argvs, intern)
char			*cmd_argvs[];
struct rt_db_internal	*intern;
{
	fastf_t			r;
	int			i;
	struct rt_tgc_internal	*tip;
	vect_t			work;

	intern->idb_type = ID_TGC;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_tgc_internal),
		"rt_tgc_internal" );
	tip = (struct rt_tgc_internal *)intern->idb_ptr;
	tip->magic = RT_TGC_INTERNAL_MAGIC;

	for (i = 0; i < ELEMENTS_PER_PT; i++) {
		tip->v[i] = atof(cmd_argvs[3+i]);
		tip->h[i] = atof(cmd_argvs[6+i]);
	}
	r = atof(cmd_argvs[9]);
	
	if (MAGNITUDE(tip->h) < RT_LEN_TOL || r < RT_LEN_TOL) {
		(void)printf("ERROR, all dimensions must be greater than zero!\n");
		return(1);	/* failure */
	}

	vec_ortho( tip->a, tip->h );
	VUNITIZE( tip->a );
	VCROSS( tip->b, tip->h, tip->a );
	VUNITIZE( tip->b );

	VSCALE( tip->a, tip->a, r );
	VSCALE( tip->b, tip->b, r );
	VMOVE( tip->c, tip->a );
	VMOVE( tip->d, tip->b );
	
	return(0);	/* success */
}

/*   T E C _ I N ( ) :   	reads tec parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
tec_in(cmd_argvs, intern)
char			*cmd_argvs[];
struct rt_db_internal	*intern;
{
	fastf_t			ratio;
	int			i;
	struct rt_tgc_internal	*tip;

	intern->idb_type = ID_TGC;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_tgc_internal),
		"rt_tgc_internal" );
	tip = (struct rt_tgc_internal *)intern->idb_ptr;
	tip->magic = RT_TGC_INTERNAL_MAGIC;

	for (i = 0; i < ELEMENTS_PER_PT; i++) {
		tip->v[i] = atof(cmd_argvs[3+i]);
		tip->h[i] = atof(cmd_argvs[6+i]);
		tip->a[i] = atof(cmd_argvs[9+i]);
		tip->b[i] = atof(cmd_argvs[12+i]);
	}
	ratio = atof(cmd_argvs[15]);
	if (MAGNITUDE(tip->h) < RT_LEN_TOL
		|| MAGNITUDE(tip->a) < RT_LEN_TOL
		|| MAGNITUDE(tip->b) < RT_LEN_TOL
		|| ratio < RT_LEN_TOL) {
		(void)printf("ERROR, all dimensions must be greater than zero!\n");
		return(1);	/* failure */
	}

	VSCALE( tip->c, tip->a, 1./ratio );	/* C vector */
	VSCALE( tip->d, tip->b, 1./ratio );	/* D vector */
	
	return(0);	/* success */
}

/*   R E C _ I N ( ) :   	reads rec parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
rec_in(cmd_argvs, intern)
char			*cmd_argvs[];
struct rt_db_internal	*intern;
{
	int			i;
	struct rt_tgc_internal	*tip;
	vect_t			work;

	intern->idb_type = ID_TGC;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_tgc_internal),
		"rt_tgc_internal" );
	tip = (struct rt_tgc_internal *)intern->idb_ptr;
	tip->magic = RT_TGC_INTERNAL_MAGIC;

	for (i = 0; i < ELEMENTS_PER_PT; i++) {
		tip->v[i] = atof(cmd_argvs[3+i]);
		tip->h[i] = atof(cmd_argvs[6+i]);
		tip->a[i] = atof(cmd_argvs[9+i]);
		tip->b[i] = atof(cmd_argvs[12+i]);
	}
	
	if (MAGNITUDE(tip->h) < RT_LEN_TOL
		|| MAGNITUDE(tip->a) < RT_LEN_TOL
		|| MAGNITUDE(tip->b) < RT_LEN_TOL ) {
		(void)printf("ERROR, all dimensions must be greater than zero!\n");
		return(1);	/* failure */
	}

	VMOVE( tip->c, tip->a );		/* C vector */
	VMOVE( tip->d, tip->b );		/* D vector */
	
	return(0);	/* success */
}

/*   T R C _ I N ( ) :   	reads trc parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
trc_in(cmd_argvs, intern)
char			*cmd_argvs[];
struct rt_db_internal	*intern;
{
	fastf_t			r1, r2;
	int			i;
	struct rt_tgc_internal	*tip;

	intern->idb_type = ID_TGC;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_tgc_internal),
		"rt_tgc_internal" );
	tip = (struct rt_tgc_internal *)intern->idb_ptr;
	tip->magic = RT_TGC_INTERNAL_MAGIC;

	for (i = 0; i < ELEMENTS_PER_PT; i++) {
		tip->v[i] = atof(cmd_argvs[3+i]);
		tip->h[i] = atof(cmd_argvs[6+i]);
	}
	r1 = atof(cmd_argvs[9]);
	r2 = atof(cmd_argvs[10]);
	
	if (MAGNITUDE(tip->h) < RT_LEN_TOL
		|| r1 < RT_LEN_TOL || r2 < RT_LEN_TOL) {
		(void)printf("ERROR, all dimensions must be greater than zero!\n");
		return(1);	/* failure */
	}

	vec_ortho( tip->a, tip->h );
	VUNITIZE( tip->a );
	VCROSS( tip->b, tip->h, tip->a );
	VUNITIZE( tip->b );
	VMOVE( tip->c, tip->a );
	VMOVE( tip->d, tip->b );

	VSCALE( tip->a, tip->a, r1 );
	VSCALE( tip->b, tip->b, r1 );
	VSCALE( tip->c, tip->c, r2 );
	VSCALE( tip->d, tip->d, r2 );
	
	return(0);	/* success */
}

/*   B O X _ I N ( ) :   	reads box parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
box_in(cmd_argvs, intern)
char			*cmd_argvs[];
struct rt_db_internal	*intern;
{
	int			i;
	struct rt_arb_internal	*aip;
	vect_t			Dpth, Hgt, Vrtx, Wdth;

	intern->idb_type = ID_ARB8;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_arb_internal),
		"rt_arb_internal" );
	aip = (struct rt_arb_internal *)intern->idb_ptr;
	aip->magic = RT_ARB_INTERNAL_MAGIC;

	for (i = 0; i < ELEMENTS_PER_PT; i++) {
		Vrtx[i] = atof(cmd_argvs[3+i]);
		Hgt[i] = atof(cmd_argvs[6+i]);
		Wdth[i] = atof(cmd_argvs[9+i]);
		Dpth[i] = atof(cmd_argvs[12+i]);
	}
	
	if (MAGNITUDE(Dpth) < RT_LEN_TOL || MAGNITUDE(Hgt) < RT_LEN_TOL
		|| MAGNITUDE(Wdth) < RT_LEN_TOL) {
		(void)printf("ERROR, dimensions must all be greater than zero!\n");
		return(1);	/* failure */
	}

	if (!strcmp("box", cmd_argvs[2])) {
		VMOVE( aip->pt[0], Vrtx );
		VADD2( aip->pt[1], Vrtx, Wdth );
		VADD3( aip->pt[2], Vrtx, Wdth, Hgt );
		VADD2( aip->pt[3], Vrtx, Hgt );
		VADD2( aip->pt[4], Vrtx, Dpth );
		VADD3( aip->pt[5], Vrtx, Dpth, Wdth );
		VADD4( aip->pt[6], Vrtx, Dpth, Wdth, Hgt );
		VADD3( aip->pt[7], Vrtx, Dpth, Hgt );
	} else { /* "raw" */
		VMOVE( aip->pt[0], Vrtx );
		VADD2( aip->pt[1], Vrtx, Wdth );
		VADD2( aip->pt[3], Vrtx, Hgt );	/* next lines fliped for 4d uopt bug */
		VMOVE( aip->pt[2], aip->pt[1] );
		VADD2( aip->pt[4], Vrtx, Dpth );
		VADD3( aip->pt[5], Vrtx, Dpth, Wdth );
		VMOVE( aip->pt[6], aip->pt[5] );
		VADD3( aip->pt[7], Vrtx, Dpth, Hgt );
	}

	return(0);	/* success */
}

/*   R P P _ I N ( ) :   	reads rpp parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
rpp_in(cmd_argvs, intern)
char			*cmd_argvs[];
struct rt_db_internal	*intern;
{
	fastf_t			xmin, xmax, ymin, ymax, zmin, zmax;
	int			i;
	struct rt_arb_internal	*aip;
	vect_t			Dpth, Hgt, Vrtx, Wdth;

	intern->idb_type = ID_ARB8;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_arb_internal),
		"rt_arb_internal" );
	aip = (struct rt_arb_internal *)intern->idb_ptr;
	aip->magic = RT_ARB_INTERNAL_MAGIC;

	xmin = atof(cmd_argvs[3+0]);
	xmax = atof(cmd_argvs[3+1]);
	ymin = atof(cmd_argvs[3+2]);
	ymax = atof(cmd_argvs[3+3]);
	zmin = atof(cmd_argvs[3+4]);
	zmax = atof(cmd_argvs[3+5]);

	if (xmin >= xmax) {
		(void)printf("ERROR, XMIN greater than XMAX!\n");
		return(1);	/* failure */
	}
	if (ymin >= ymax) {
		(void)printf("ERROR, YMIN greater than YMAX!\n");
		return(1);	/* failure */
	}
	if (zmin >= zmax) {
		(void)printf("ERROR, ZMIN greater than ZMAX!\n");
		return(1);	/* failure */
	}

	VSET( aip->pt[0], xmax, ymin, zmin );
	VSET( aip->pt[1], xmax, ymax, zmin );
	VSET( aip->pt[2], xmax, ymax, zmax );
	VSET( aip->pt[3], xmax, ymin, zmax );
	VSET( aip->pt[4], xmin, ymin, zmin );
	VSET( aip->pt[5], xmin, ymax, zmin );
	VSET( aip->pt[6], xmin, ymax, zmax );
	VSET( aip->pt[7], xmin, ymin, zmax );

	return(0);	/* success */
}

/*   R P C _ I N ( ) :   	reads rpc parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
rpc_in(cmd_argvs, intern)
char			*cmd_argvs[];
struct rt_db_internal	*intern;
{
	int			i;
	struct rt_rpc_internal	*rip;

	intern->idb_type = ID_RPC;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_rpc_internal),
		"rt_rpc_internal" );
	rip = (struct rt_rpc_internal *)intern->idb_ptr;
	rip->rpc_magic = RT_RPC_INTERNAL_MAGIC;

	for (i = 0; i < ELEMENTS_PER_PT; i++) {
		rip->rpc_V[i] = atof(cmd_argvs[3+i]);
		rip->rpc_H[i] = atof(cmd_argvs[6+i]);
		rip->rpc_B[i] = atof(cmd_argvs[9+i]);
	}
	rip->rpc_r = atof(cmd_argvs[12]);
	
	if (MAGNITUDE(rip->rpc_H) < RT_LEN_TOL
		|| MAGNITUDE(rip->rpc_B) < RT_LEN_TOL
		|| rip->rpc_r <= RT_LEN_TOL) {
		(void)printf("ERROR, height, breadth, and width must be greater than zero!\n");
		return(1);	/* failure */
	}
	
	return(0);	/* success */
}

/*   R H C _ I N ( ) :   	reads rhc parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
rhc_in(cmd_argvs, intern)
char			*cmd_argvs[];
struct rt_db_internal	*intern;
{
	int			i;
	struct rt_rhc_internal	*rip;

	intern->idb_type = ID_RHC;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_rhc_internal),
		"rt_rhc_internal" );
	rip = (struct rt_rhc_internal *)intern->idb_ptr;
	rip->rhc_magic = RT_RHC_INTERNAL_MAGIC;

	for (i = 0; i < ELEMENTS_PER_PT; i++) {
		rip->rhc_V[i] = atof(cmd_argvs[3+i]);
		rip->rhc_H[i] = atof(cmd_argvs[6+i]);
		rip->rhc_B[i] = atof(cmd_argvs[9+i]);
	}
	rip->rhc_r = atof(cmd_argvs[12]);
	rip->rhc_c = atof(cmd_argvs[13]);
	
	if (MAGNITUDE(rip->rhc_H) < RT_LEN_TOL
		|| MAGNITUDE(rip->rhc_B) < RT_LEN_TOL
		|| rip->rhc_r <= RT_LEN_TOL || rip->rhc_c <= RT_LEN_TOL) {
		(void)printf("ERROR, height, breadth, and width must be greater than zero!\n");
		return(1);	/* failure */
	}
	
	return(0);	/* success */
}

/*   E P A _ I N ( ) :   	reads epa parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
epa_in(cmd_argvs, intern)
char			*cmd_argvs[];
struct rt_db_internal	*intern;
{
	int			i;
	struct rt_epa_internal	*rip;

	intern->idb_type = ID_EPA;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_epa_internal),
		"rt_epa_internal" );
	rip = (struct rt_epa_internal *)intern->idb_ptr;
	rip->epa_magic = RT_EPA_INTERNAL_MAGIC;

	for (i = 0; i < ELEMENTS_PER_PT; i++) {
		rip->epa_V[i] = atof(cmd_argvs[3+i]);
		rip->epa_H[i] = atof(cmd_argvs[6+i]);
		rip->epa_Au[i] = atof(cmd_argvs[9+i]);
	}
	rip->epa_r1 = MAGNITUDE(rip->epa_Au);
	rip->epa_r2 = atof(cmd_argvs[12]);
	VUNITIZE(rip->epa_Au);
	
	if (MAGNITUDE(rip->epa_H) < RT_LEN_TOL
		|| rip->epa_r1 <= RT_LEN_TOL || rip->epa_r2 <= RT_LEN_TOL) {
		(void)printf("ERROR, height and axes must be greater than zero!\n");
		return(1);	/* failure */
	}

	if (rip->epa_r2 > rip->epa_r1) {
		(void)printf("ERROR, |A| must be greater than |B|!\n");
		return(1);	/* failure */
	}
	
	return(0);	/* success */
}

/*   E H Y _ I N ( ) :   	reads ehy parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
ehy_in(cmd_argvs, intern)
char			*cmd_argvs[];
struct rt_db_internal	*intern;
{
	int			i;
	struct rt_ehy_internal	*rip;

	intern->idb_type = ID_EHY;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_ehy_internal),
		"rt_ehy_internal" );
	rip = (struct rt_ehy_internal *)intern->idb_ptr;
	rip->ehy_magic = RT_EHY_INTERNAL_MAGIC;

	for (i = 0; i < ELEMENTS_PER_PT; i++) {
		rip->ehy_V[i] = atof(cmd_argvs[3+i]);
		rip->ehy_H[i] = atof(cmd_argvs[6+i]);
		rip->ehy_Au[i] = atof(cmd_argvs[9+i]);
	}
	rip->ehy_r1 = MAGNITUDE(rip->ehy_Au);
	rip->ehy_r2 = atof(cmd_argvs[12]);
	rip->ehy_c = atof(cmd_argvs[13]);
	VUNITIZE(rip->ehy_Au);
	
	if (MAGNITUDE(rip->ehy_H) < RT_LEN_TOL
		|| rip->ehy_r1 <= RT_LEN_TOL || rip->ehy_r2 <= RT_LEN_TOL
		|| rip->ehy_c <= RT_LEN_TOL) {
		(void)printf("ERROR, height, axes, and distance to asymptotes must be greater than zero!\n");
		return(1);	/* failure */
	}

	if (rip->ehy_r2 > rip->ehy_r1) {
		(void)printf("ERROR, |A| must be greater than |B|!\n");
		return(1);	/* failure */
	}
	
	return(0);	/* success */
}

/*   E T O _ I N ( ) :   	reads eto parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
eto_in(cmd_argvs, intern)
char			*cmd_argvs[];
struct rt_db_internal	*intern;
{
	int			i;
	struct rt_eto_internal	*eip;

	intern->idb_type = ID_ETO;
	intern->idb_ptr = (genptr_t)rt_malloc( sizeof(struct rt_eto_internal),
		"rt_eto_internal" );
	eip = (struct rt_eto_internal *)intern->idb_ptr;
	eip->eto_magic = RT_ETO_INTERNAL_MAGIC;

	for (i = 0; i < ELEMENTS_PER_PT; i++) {
		eip->eto_V[i] = atof(cmd_argvs[3+i]);
		eip->eto_N[i] = atof(cmd_argvs[6+i]);
		eip->eto_C[i] = atof(cmd_argvs[9+i]);
	}
	eip->eto_r = atof(cmd_argvs[12]);
	eip->eto_rd = atof(cmd_argvs[13]);
	
	if (MAGNITUDE(eip->eto_N) < RT_LEN_TOL
		|| MAGNITUDE(eip->eto_C) < RT_LEN_TOL
		|| eip->eto_r <= RT_LEN_TOL || eip->eto_rd <= RT_LEN_TOL) {
		(void)printf("ERROR, normal, axes, and radii must be greater than zero!\n");
		return(1);	/* failure */
	}

	if (eip->eto_rd > MAGNITUDE(eip->eto_C)) {
		(void)printf("ERROR, |C| must be greater than |D|!\n");
		return(1);	/* failure */
	}
	
	return(0);	/* success */
}

/*   C H E C K V ( ) :		checks for zero vector at cmd_args[loc]
 *				returns 0 if vector non-zero
 *				       -1 if vector is zero
 */
int
checkv( loc )
int loc;
{
	register int	i;
	vect_t	work;

	for(i=0; i<3; i++) {
		work[i] = atof(cmd_args[(loc+i)]);
	}
	if( MAGNITUDE(work) < 1e-10 ) {
		(void)printf("ERROR, zero vector ....\n");
		return(-1);	/* zero vector */
	}
	return(0);	/* vector is non-zero */
}

/*   G E T C M D ( ) :	gets keyboard input command lines, parses and
 *			saves pointers to the beginning of each element
 *			starting at cmd_args[pos] and returns:
 *				the number of args	if successful
 *						-1	if unsuccessful
 */
int
getcmd(pos)
int pos;
{
	register char *lp;
	register char *lp1;

	newargs = 0;
	/*
	 * Now we go to the last argument string read and then position
	 * ourselves at the end of the string.  This is IMPORTANT so we
	 * don't overwrite what we've already read into line[].
	 */
	lp = cmd_args[pos-1];		/* Beginning of last arg string */
	while( *lp++ != '\0' )  {	/* Get positioned at end of string */
		;
	}

	/* Read input line */
	(void)fgets( lp, MAXLINE, stdin );

	/* Check for Control-D (EOF) */
	if( feof( stdin ) )  {
		/* Control-D typed, let's hit the road */
		f_quit();
		/* NOTREACHED */
	}

	cmd_args[newargs + pos] = lp;

	if( *lp == '\n' )
		return(0);		/* NOP */

	/* In case first character is not "white space" */
	if( (*lp != ' ') && (*lp != '\t') && (*lp != '\0') )
		newargs++;		/* holds # of args */

	for( ; *lp != '\0'; lp++ )  {
		if( (*lp == ' ') || (*lp == '\t') || (*lp == '\n') )  {
			*lp = '\0';
			lp1 = lp + 1;
			if( (*lp1 != ' ') && (*lp1 != '\t') &&
			    (*lp1 != '\n') && (*lp1 != '\0') )  {
				if( (newargs + pos) >= maxargs )  {
					(void)printf("More than %d arguments, excess flushed\n", maxargs);
					cmd_args[maxargs] = (char *)0;
					return(maxargs - pos);
				}
				cmd_args[newargs + pos] = lp1;
			    	newargs++;
			}
		}
		/* Finally, a non-space char */
	}
	/* Null terminate pointer array */
	cmd_args[newargs + pos] = (char *)0;
	return(newargs);
}
