/*
 *			T C L . C
 *
 *  Tcl interfaces to LIBRT routines.
 *
 *  LIBRT routines are not for casual command-line use;
 *  as a result, the Tcl name for the function should be exactly
 *  the same as the C name for the underlying function.
 *  Typing "rt_" is no hardship when writing Tcl procs, and
 *  clarifies the origin of the routine.
 *
 *  Authors -
 *	Michael John Muuss
 *      Glenn Durfee
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Notice -
 *	Re-distribution of this software is restricted, as described in
 *	your "Statement of Terms and Conditions for the Release of
 *	The BRL-CAD Package" agreement.
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 1997 by the United States Army
 *	in all countries except the USA.  All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (ARL)";
#endif

#include "conf.h"

#include <fcntl.h>
#include <sys/errno.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "tcl.h"

#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "bn.h"
#include "rtgeom.h"
#include "raytrace.h"
#include "externs.h"

#include "./debug.h"

/* defined in wdb_obj.c */
extern int Wdb_Init();

/* defined in dg_obj.c */
extern int Dgo_Init();

/* defined in view_obj.c */
extern int Vo_Init();

/************************************************************************
 *									*
 *		Tcl interface to Ray-tracing				*
 *									*
 ************************************************************************/

struct dbcmdstruct {
	char *cmdname;
	int (*cmdfunc)();
};

/*
 *			R T _ T C L _ P R _ H I T
 *
 *  Format a hit in a TCL-friendly format.
 *
 *  It is possible that a solid may have been removed from the
 *  directory after this database was prepped, so check pointers
 *  carefully.
 *
 *  It might be beneficial to use some format other than %g to
 *  give the user more precision.
 */
void
rt_tcl_pr_hit( interp, hitp, segp, rayp, flipflag )
Tcl_Interp	*interp;
struct hit	*hitp;
struct seg	*segp;
struct xray	*rayp;
int		flipflag;
{
	struct bu_vls	str;
	vect_t		norm;
	struct soltab	*stp;
	CONST struct directory	*dp;
	struct curvature crv;

	RT_CK_SEG(segp);
	stp = segp->seg_stp;
	RT_CK_SOLTAB(stp);
	dp = stp->st_dp;
	RT_CK_DIR(dp);

	RT_HIT_NORMAL( norm, hitp, stp, rayp, flipflag );
	RT_CURVATURE( &crv, hitp, flipflag, stp );

	bu_vls_init(&str);
	bu_vls_printf( &str, " {dist %g point {", hitp->hit_dist);
	bn_encode_vect( &str, hitp->hit_point );
	bu_vls_printf( &str, "} normal {" );
	bn_encode_vect( &str, norm );
	bu_vls_printf( &str, "} c1 %g c2 %g pdir {",
		crv.crv_c1, crv.crv_c2 );
	bn_encode_vect( &str, crv.crv_pdir );
	bu_vls_printf( &str, "} surfno %d", hitp->hit_surfno );
	if( stp->st_path.magic == DB_FULL_PATH_MAGIC )  {
		/* Magic is left 0 if the path is not filled in. */
		char	*sofar = db_path_to_string(&stp->st_path);
		bu_vls_printf( &str, " path ");
		bu_vls_strcat( &str, sofar );
		bu_free( (genptr_t)sofar, "path string" );
	}
	bu_vls_printf( &str, " solid %s}", dp->d_namep );

	Tcl_AppendResult( interp, bu_vls_addr( &str ), (char *)NULL );
	bu_vls_free( &str );
}

/*
 *			R T _ T C L _ A _ H I T
 */
int
rt_tcl_a_hit( ap, PartHeadp, segHeadp )
struct application	*ap;
struct partition	*PartHeadp;
struct seg		*segHeadp;
{
	Tcl_Interp *interp = (Tcl_Interp *)ap->a_uptr;
	register struct partition *pp;

	RT_CK_PT_HD(PartHeadp);

	for( pp=PartHeadp->pt_forw; pp != PartHeadp; pp = pp->pt_forw )  {
		RT_CK_PT(pp);
		Tcl_AppendResult( interp, "{in", (char *)NULL );
		rt_tcl_pr_hit( interp, pp->pt_inhit, pp->pt_inseg,
			&ap->a_ray, pp->pt_inflip );
		Tcl_AppendResult( interp, "\nout", (char *)NULL );
		rt_tcl_pr_hit( interp, pp->pt_outhit, pp->pt_outseg,
			&ap->a_ray, pp->pt_outflip );
		Tcl_AppendResult( interp,
			"\nregion ",
			pp->pt_regionp->reg_name,
			(char *)NULL );
		Tcl_AppendResult( interp, "}\n", (char *)NULL );
	}

	return 1;
}

/*
 *			R T _ T C L _ A _ M I S S
 */
int
rt_tcl_a_miss( ap )
struct application	*ap;
{
	Tcl_Interp *interp = (Tcl_Interp *)ap->a_uptr;

	return 0;
}


/*
 *			R T _ T C L _ S H O O T R A Y
 *
 *  Usage -
 *	procname shootray {P} dir|at {V}
 *
 *  Example -
 *	set glob_compat_mode 0
 *	.inmem rt_gettrees .rt all.g
 *	.rt shootray {0 0 0} dir {0 0 -1}
 *
 *	set tgt [bu_get_value_by_keyword V [concat type [.inmem get LIGHT]]]
 *	.rt shootray {20 -13.5 20} at $tgt
 *		
 *
 *  Returns -
 *	This "shootray" operation returns a nested set of lists. It returns
 *	a list of zero or more partitions. Inside each partition is a list
 *	containing an in, out, and region keyword, each with an associated
 *	value. The associated value for each "inhit" and "outhit" is itself
 *	a list containing a dist, point, normal, surfno, and solid keyword,
 *	each with an associated value.
 */
int
rt_tcl_rt_shootray( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	struct application	*ap = (struct application *)clientData;
	struct rt_i		*rtip;

	if( argc != 5 )  {
		Tcl_AppendResult( interp,
				"wrong # args: should be \"",
				argv[0], " ", argv[1], " {P} dir|at {V}\"",
				(char *)NULL );
		return TCL_ERROR;
	}

	/* Could core dump */
	RT_AP_CHECK(ap);
	rtip = ap->a_rt_i;
	RT_CK_RTI_TCL(interp,rtip);

	if( bn_decode_vect( ap->a_ray.r_pt,  argv[2] ) != 3 ||
	    bn_decode_vect( ap->a_ray.r_dir, argv[4] ) != 3 )  {
		Tcl_AppendResult( interp,
			"badly formatted vector", (char *)NULL );
		return TCL_ERROR;
	}
	switch( argv[3][0] )  {
	case 'd':
		/* [4] is direction vector */
		break;
	case 'a':
		/* [4] is target point, build a vector from start pt */
		VSUB2( ap->a_ray.r_dir, ap->a_ray.r_dir, ap->a_ray.r_pt );
		break;
	default:
		Tcl_AppendResult( interp,
				"wrong keyword: '", argv[4],
				"', should be one of 'dir' or 'at'",
				(char *)NULL );
		return TCL_ERROR;
	}
	VUNITIZE( ap->a_ray.r_dir );	/* sanity */
	ap->a_hit = rt_tcl_a_hit;
	ap->a_miss = rt_tcl_a_miss;
	ap->a_uptr = (genptr_t)interp;

	(void)rt_shootray( ap );

	return TCL_OK;
}

/*
 *			R T _ T C L _ R T _ O N E H I T
 *  Usage -
 *	procname onehit
 *	procname onehit #
 */
int
rt_tcl_rt_onehit( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	struct application	*ap = (struct application *)clientData;
	struct rt_i		*rtip;
	char			buf[64];

	if( argc < 2 || argc > 3 )  {
		Tcl_AppendResult( interp,
				"wrong # args: should be \"",
				argv[0], " ", argv[1], " [#]\"",
				(char *)NULL );
		return TCL_ERROR;
	}

	/* Could core dump */
	RT_AP_CHECK(ap);
	rtip = ap->a_rt_i;
	RT_CK_RTI_TCL(interp,rtip);

	if( argc == 3 )  {
		ap->a_onehit = atoi(argv[2]);
	}
	sprintf(buf, "%d", ap->a_onehit );
	Tcl_AppendResult( interp, buf, (char *)NULL );
	return TCL_OK;
}

/*
 *			R T _ T C L _ R T _ N O _ B O O L
 *  Usage -
 *	procname no_bool
 *	procname no_bool #
 */
int
rt_tcl_rt_no_bool( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	struct application	*ap = (struct application *)clientData;
	struct rt_i		*rtip;
	char			buf[64];

	if( argc < 2 || argc > 3 )  {
		Tcl_AppendResult( interp,
				"wrong # args: should be \"",
				argv[0], " ", argv[1], " [#]\"",
				(char *)NULL );
		return TCL_ERROR;
	}

	/* Could core dump */
	RT_AP_CHECK(ap);
	rtip = ap->a_rt_i;
	RT_CK_RTI_TCL(interp,rtip);

	if( argc == 3 )  {
		ap->a_no_booleans = atoi(argv[2]);
	}
	sprintf(buf, "%d", ap->a_no_booleans );
	Tcl_AppendResult( interp, buf, (char *)NULL );
	return TCL_OK;
}

/*
 *			R T _ T C L _ R T _ C H E C K
 *
 *  Run some of the internal consistency checkers over the data structures.
 *
 *  Usage -
 *	procname check
 */
int
rt_tcl_rt_check( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	struct application	*ap = (struct application *)clientData;
	struct rt_i		*rtip;
	char			buf[64];

	if( argc != 2 )  {
		Tcl_AppendResult( interp,
				"wrong # args: should be \"",
				argv[0], " ", argv[1], "\"",
				(char *)NULL );
		return TCL_ERROR;
	}

	/* Could core dump */
	RT_AP_CHECK(ap);
	rtip = ap->a_rt_i;
	RT_CK_RTI_TCL(interp,rtip);

	rt_ck(rtip);

	return TCL_OK;
}

/*
 *			R T _ T C L _ R T _ P R E P
 *
 *  When run with no args, just prints current status of prepping.
 *
 *  Usage -
 *	procname prep
 *	procname prep use_air [hasty_prep]
 */
int
rt_tcl_rt_prep( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	struct application	*ap = (struct application *)clientData;
	struct rt_i		*rtip;
	struct bu_vls		str;

	if( argc < 2 || argc > 4 )  {
		Tcl_AppendResult( interp,
				"wrong # args: should be \"",
				argv[0], " ", argv[1],
				" [hasty_prep]\"",
				(char *)NULL );
		return TCL_ERROR;
	}

	/* Could core dump */
	RT_AP_CHECK(ap);
	rtip = ap->a_rt_i;
	RT_CK_RTI_TCL(interp,rtip);

	if( argc >= 3 && !rtip->needprep )  {
		Tcl_AppendResult( interp,
			argv[0], " ", argv[1],
			" invoked when model has already been prepped.\n",
			(char *)NULL );
		return TCL_ERROR;
	}

	if( argc == 4 )  rtip->rti_hasty_prep = atoi(argv[3]);

	/* If args were given, prep now. */
	if( argc >= 3 )  rt_prep_parallel( rtip, 1 );

	/* Now, describe the current state */
	bu_vls_init( &str );
	bu_vls_printf( &str, "hasty_prep %d dont_instance %d useair %d needprep %d",
		rtip->rti_hasty_prep,
		rtip->rti_dont_instance,
		rtip->useair,
		rtip->needprep
	);

	Tcl_AppendResult( interp, bu_vls_addr(&str), (char *)NULL );
	bu_vls_free( &str );
	return TCL_OK;
}

static struct dbcmdstruct rt_tcl_rt_cmds[] = {
	"shootray",	rt_tcl_rt_shootray,
	"onehit",	rt_tcl_rt_onehit,
	"no_bool",	rt_tcl_rt_no_bool,
	"check",	rt_tcl_rt_check,
	"prep",		rt_tcl_rt_prep,
	(char *)0,	(int (*)())0
};

/*
 *			R T _ T C L _ R T
 *
 * Generic interface for the LIBRT_class manipulation routines.
 * Usage:
 *        procname dbCmdName ?args?
 * Returns: result of cmdName LIBRT operation.
 *
 * Objects of type 'procname' must have been previously created by 
 * the 'rt_gettrees' operation performed on a database object.
 *
 * Example -
 *	.inmem rt_gettrees .rt all.g
 *	.rt shootray {0 0 0} dir {0 0 -1}
 */
int
rt_tcl_rt( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	struct dbcmdstruct	*dbcmd;

	if( argc < 2 ) {
		Tcl_AppendResult( interp,
				  "wrong # args: should be \"", argv[0],
				  " command [args...]\"",
				  (char *)NULL );
		return TCL_ERROR;
	}

	for( dbcmd = rt_tcl_rt_cmds; dbcmd->cmdname != NULL; dbcmd++ ) {
		if( strcmp(dbcmd->cmdname, argv[1]) == 0 ) {
			return (*dbcmd->cmdfunc)( clientData, interp,
						  argc, argv );
		}
	}


	Tcl_AppendResult( interp, "unknown LIBRT command; must be one of:",
			  (char *)NULL );
	for( dbcmd = rt_tcl_rt_cmds; dbcmd->cmdname != NULL; dbcmd++ ) {
		Tcl_AppendResult( interp, " ", dbcmd->cmdname, (char *)NULL );
	}
	return TCL_ERROR;
}

/************************************************************************
 *									*
 *		Tcl interface to Combination import/export		*
 *									*
 ************************************************************************/

/*
 *		D B _ T C L _ T R E E _ D E S C R I B E
 *
 * Fills a Tcl_DString with a representation of the given tree appropriate
 * for processing by Tcl scripts.  The reason we use Tcl_DStrings instead
 * of bu_vlses is that Tcl_DStrings provide "start/end sublist" commands
 * and automatic escaping of Tcl-special characters.
 *
 * A tree 't' is represented in the following manner:
 *
 *	t := { l dbobjname { mat } }
 *	   | { l dbobjname }
 *	   | { u t1 t2 }
 * 	   | { n t1 t2 }
 *	   | { - t1 t2 }
 *	   | { ^ t1 t2 }
 *         | { ! t1 }
 *	   | { G t1 }
 *	   | { X t1 }
 *	   | { N }
 *	   | {}
 *
 * where 'dbobjname' is a string containing the name of a database object,
 *       'mat'       is the matrix preceeding a leaf,
 *       't1', 't2'  are trees (recursively defined).
 *
 * Notice that in most cases, this tree will be grossly unbalanced.
 */

void
db_tcl_tree_describe( dsp, tp )
Tcl_DString		*dsp;
union tree		*tp;
{
	if( !tp ) return;

	RT_CK_TREE(tp);
	switch( tp->tr_op ) {
	case OP_DB_LEAF:
		Tcl_DStringAppendElement( dsp, "l" );
		Tcl_DStringAppendElement( dsp, tp->tr_l.tl_name );
		if( tp->tr_l.tl_mat )  {
			struct bu_vls vls;
			bu_vls_init( &vls );
			bn_encode_mat( &vls, tp->tr_l.tl_mat );
			Tcl_DStringAppendElement( dsp, bu_vls_addr(&vls) );
			bu_vls_free( &vls );
		}
		break;

		/* This node is known to be a binary op */
	case OP_UNION:
		Tcl_DStringAppendElement( dsp, "u" );
		goto bin;
	case OP_INTERSECT:
		Tcl_DStringAppendElement( dsp, "n" );
		goto bin;
	case OP_SUBTRACT:
		Tcl_DStringAppendElement( dsp, "-" );
		goto bin;
	case OP_XOR:
		Tcl_DStringAppendElement( dsp, "^" );
	bin:
		Tcl_DStringStartSublist( dsp );
		db_tcl_tree_describe( dsp, tp->tr_b.tb_left );
		Tcl_DStringEndSublist( dsp );

		Tcl_DStringStartSublist( dsp );
		db_tcl_tree_describe( dsp, tp->tr_b.tb_right );
		Tcl_DStringEndSublist( dsp );

		break;

		/* This node is known to be a unary op */
	case OP_NOT:
		Tcl_DStringAppendElement( dsp, "!" );
		goto unary;
	case OP_GUARD:
		Tcl_DStringAppendElement( dsp, "G" );
		goto unary;
	case OP_XNOP:
		Tcl_DStringAppendElement( dsp, "X" );
	unary:
		Tcl_DStringStartSublist( dsp );
		db_tcl_tree_describe( dsp, tp->tr_b.tb_left );
		Tcl_DStringEndSublist( dsp );
		break;
			
	case OP_NOP:
		Tcl_DStringAppendElement( dsp, "N" );
		break;

	default:
		bu_log("db_tcl_tree_describe: bad op %d\n", tp->tr_op);
		bu_bomb("db_tcl_tree_describe\n");
	}
}

/*
 *			D B _ T C L _ T R E E _ P A R S E
 *
 *  Take a TCL-style string description of a binary tree, as produced by
 *  db_tcl_tree_describe(), and reconstruct
 *  the in-memory form of that tree.
 */
union tree *
db_tcl_tree_parse( interp, str )
Tcl_Interp	*interp;
char		*str;
{
	int	argc;
	char	**argv;
	union tree	*tp = TREE_NULL;
	union tree	*lhs;
	union tree	*rhs;

	/* Skip over leading spaces in input */
	while( *str && isspace(*str) ) str++;

	if( Tcl_SplitList( interp, str, &argc, &argv ) != TCL_OK )
		return TREE_NULL;

	if( argc <= 0 || argc > 3 )  {
		Tcl_AppendResult( interp, "db_tcl_tree_parse: tree node does not have 1, 2 or 2 elements: ",
			str, "\n", (char *)NULL );
		goto out;
	}

#if 0
Tcl_AppendResult( interp, "\n\ndb_tcl_tree_parse(): ", str, "\n", NULL );

Tcl_AppendResult( interp, "db_tcl_tree_parse() arg0=", argv[0], NULL );
if(argc > 1 ) Tcl_AppendResult( interp, "\n\targ1=", argv[1], NULL );
if(argc > 2 ) Tcl_AppendResult( interp, "\n\targ2=", argv[2], NULL );
Tcl_AppendResult( interp, "\n\n", NULL);
#endif

	if( argv[0][1] != '\0' )  {
		Tcl_AppendResult( interp, "db_tcl_tree_parse() operator is not single character: ",
			argv[0], (char *)NULL );
		goto out;
	}

	switch( argv[0][0] )  {
	case 'l':
		/* Leaf node: {l name {mat}} */
		BU_GETUNION( tp, tree );
		tp->tr_l.magic = RT_TREE_MAGIC;
		tp->tr_op = OP_DB_LEAF;
		tp->tr_l.tl_name = bu_strdup( argv[1] );
		if( argc == 3 )  {
			tp->tr_l.tl_mat = (matp_t)bu_malloc( sizeof(mat_t), "tl_mat");
			if( bn_decode_mat( tp->tr_l.tl_mat, argv[2] ) != 16 )  {
				Tcl_AppendResult( interp, "db_tcl_tree_parse: unable to parse matrix '",
					argv[2], "', using all-zeros", (char *)NULL );
				bn_mat_zero( tp->tr_l.tl_mat );
			}
		}
		break;

	case 'u':
		/* Binary: Union: {u {lhs} {rhs}} */
		BU_GETUNION( tp, tree );
		tp->tr_b.tb_op = OP_UNION;
		goto binary;
	case 'n':
		/* Binary: Intersection */
		BU_GETUNION( tp, tree );
		tp->tr_b.tb_op = OP_INTERSECT;
		goto binary;
	case '-':
		/* Binary: Union */
		BU_GETUNION( tp, tree );
		tp->tr_b.tb_op = OP_SUBTRACT;
		goto binary;
	case '^':
		/* Binary: Xor */
		BU_GETUNION( tp, tree );
		tp->tr_b.tb_op = OP_XOR;
		goto binary;
binary:
		tp->tr_b.magic = RT_TREE_MAGIC;
		if( argv[1] == (char *)NULL || argv[2] == (char *)NULL )  {
			Tcl_AppendResult( interp, "db_tcl_tree_parse: binary operator ",
				argv[0], " has insufficient operands in ",
				str, (char *)NULL );
			bu_free( (char *)tp, "union tree" );
			tp = TREE_NULL;
			goto out;
		}
		tp->tr_b.tb_left = db_tcl_tree_parse( interp, argv[1] );
		if( tp->tr_b.tb_left == TREE_NULL )  {
			bu_free( (char *)tp, "union tree" );
			tp = TREE_NULL;
			goto out;
		}
		tp->tr_b.tb_right = db_tcl_tree_parse( interp, argv[2] );
		if( tp->tr_b.tb_left == TREE_NULL )  {
			db_free_tree( tp->tr_b.tb_left );
			bu_free( (char *)tp, "union tree" );
			tp = TREE_NULL;
			goto out;
		}
		break;

	case '!':
		/* Unary: not {! {lhs}} */
		BU_GETUNION( tp, tree );
		tp->tr_b.tb_op = OP_NOT;
		goto unary;
	case 'G':
		/* Unary: GUARD {G {lhs}} */
		BU_GETUNION( tp, tree );
		tp->tr_b.tb_op = OP_GUARD;
		goto unary;
	case 'X':
		/* Unary: XNOP {X {lhs}} */
		BU_GETUNION( tp, tree );
		tp->tr_b.tb_op = OP_XNOP;
		goto unary;
unary:
		tp->tr_b.magic = RT_TREE_MAGIC;
		if( argv[1] == (char *)NULL )  {
			Tcl_AppendResult( interp, "db_tcl_tree_parse: unary operator ",
				argv[0], " has insufficient operands in ",
				str, "\n", (char *)NULL );
			bu_free( (char *)tp, "union tree" );
			tp = TREE_NULL;
			goto out;
		}
		tp->tr_b.tb_left = db_tcl_tree_parse( interp, argv[1] );
		if( tp->tr_b.tb_left == TREE_NULL )  {
			bu_free( (char *)tp, "union tree" );
			tp = TREE_NULL;
			goto out;
		}
		break;

	case 'N':
		/* NOP: no args.  {N} */
		BU_GETUNION( tp, tree );
		tp->tr_b.tb_op = OP_XNOP;
		tp->tr_b.magic = RT_TREE_MAGIC;
		break;

	default:
		Tcl_AppendResult( interp, "db_tcl_tree_parse: unable to interpret operator '",
			argv[1], "'\n", (char *)NULL );
	}

out:
	free( (char *)argv);		/* not bu_free() */
	return tp;
}

/*
 *			R T _ C O M B _ T C L G E T
 *
 *  Sets the interp->result string to a description of the given combination.
 *  Entered via rt_functab[].ft_tclget().
 */
int
rt_comb_tclget( interp, intern, item )
Tcl_Interp			*interp;
CONST struct rt_db_internal	*intern;
CONST char			*item;
{
	CONST struct rt_comb_internal *comb;
	char buf[128];
	Tcl_DString	ds;

	RT_CK_DB_INTERNAL(intern);
	comb = (struct rt_comb_internal *)intern->idb_ptr;
	RT_CK_COMB_TCL(interp,comb);

	if( item==0 ) {
		/* Print out the whole combination. */
		Tcl_DStringInit( &ds );

		Tcl_DStringAppendElement( &ds, "comb" );
		Tcl_DStringAppendElement( &ds, "region" );
		if( comb->region_flag ) {
			Tcl_DStringAppendElement( &ds, "yes" );

			Tcl_DStringAppendElement( &ds, "id" );
			sprintf( buf, "%d", comb->region_id );
			Tcl_DStringAppendElement( &ds, buf );

			if( comb->aircode )  {
				Tcl_DStringAppendElement( &ds, "air" );
				sprintf( buf, "%d", comb->aircode );
				Tcl_DStringAppendElement( &ds, buf );
			}
			if( comb->los )  {
				Tcl_DStringAppendElement( &ds, "los" );
				sprintf( buf, "%d", comb->los );
				Tcl_DStringAppendElement( &ds, buf );
			}

			if( comb->GIFTmater )  {
				Tcl_DStringAppendElement( &ds, "GIFTmater" );
				sprintf( buf, "%d", comb->GIFTmater );
				Tcl_DStringAppendElement( &ds, buf );
			}
		} else {
			Tcl_DStringAppendElement( &ds, "no" );
		}
		
		if( comb->rgb_valid ) {
			Tcl_DStringAppendElement( &ds, "rgb" );
			sprintf( buf, "%d %d %d", V3ARGS(comb->rgb) );
			Tcl_DStringAppendElement( &ds, buf );
		}

		if( bu_vls_strlen(&comb->shader) > 0 )  {
			Tcl_DStringAppendElement( &ds, "shader" );
			Tcl_DStringAppendElement( &ds, bu_vls_addr(&comb->shader) );
		}

		if( bu_vls_strlen(&comb->material) > 0 )  {
			Tcl_DStringAppendElement( &ds, "material" );
			Tcl_DStringAppendElement( &ds, bu_vls_addr(&comb->material) );
		}

		if( comb->inherit ) {
			Tcl_DStringAppendElement( &ds, "inherit" );
			Tcl_DStringAppendElement( &ds, "yes" );
		}

		Tcl_DStringAppendElement( &ds, "tree" );
		Tcl_DStringStartSublist( &ds );
		db_tcl_tree_describe( &ds, comb->tree );
		Tcl_DStringEndSublist( &ds );

		Tcl_DStringResult( interp, &ds );
		Tcl_DStringFree( &ds );

		return TCL_OK;
	} else {
		/* Print out only the requested item. */
		register int i;
		char itemlwr[128];

		for( i = 0; i < 128 && item[i]; i++ ) {
			itemlwr[i] = (isupper(item[i]) ? tolower(item[i]) :
				      item[i]);
		}
		itemlwr[i] = 0;

		if( strcmp(itemlwr, "region")==0 ) {
			strcpy( buf, comb->region_flag ? "yes" : "no" );
		} else if( strcmp(itemlwr, "id")==0 ) {
			if( !comb->region_flag ) goto not_region;
			sprintf( buf, "%d", comb->region_id );
		} else if( strcmp(itemlwr, "air")==0 ) {
			if( !comb->region_flag ) goto not_region;
			sprintf( buf, "%d", comb->aircode );
		} else if( strcmp(itemlwr, "los")==0 ) {
			if( !comb->region_flag ) goto not_region;
			sprintf( buf, "%d", comb->los );
		} else if( strcmp(itemlwr, "giftmater")==0 ) {
			if( !comb->region_flag ) goto not_region;
			sprintf( buf, "%d", comb->GIFTmater );
		} else if( strcmp(itemlwr, "rgb")==0 ) {
			if( comb->rgb_valid )
				sprintf( buf, "%d %d %d", V3ARGS(comb->rgb) );
			else
				strcpy( buf, "invalid" );
		} else if( strcmp(itemlwr, "shader")==0 ) {
			Tcl_AppendResult( interp, bu_vls_addr(&comb->shader),
					  (char *)NULL );
			return TCL_OK;
		} else if( strcmp(itemlwr, "material")==0 ) {
			Tcl_AppendResult( interp, bu_vls_addr(&comb->material),
					  (char *)NULL );
			return TCL_OK;
		} else if( strcmp(itemlwr, "inherit")==0 ) {
			strcpy( buf, comb->inherit ? "yes" : "no" );
		} else if( strcmp(itemlwr, "tree")==0 ) {
			Tcl_DStringInit( &ds );
			db_tcl_tree_describe( &ds, comb->tree );
			Tcl_DStringResult( interp, &ds );
			Tcl_DStringFree( &ds );
			return TCL_OK;
		} else {
			Tcl_AppendResult( interp, "no such attribute",
					  (char *)NULL );
			return TCL_ERROR;
		}

		Tcl_AppendResult( interp, buf, (char *)NULL );
		return TCL_OK;

	not_region:
		Tcl_AppendResult( interp, "item not valid for non-region",
				  (char *)NULL );
		return TCL_ERROR;
	}
}


/*
 *			R T _ C O M B _ T C L A D J U S T
 *
 *  Example -
 *	rgb "1 2 3" ...
 *
 *  Invoked via rt_functab[].ft_tcladjust()
 */
int
rt_comb_tcladjust( interp, intern, argc, argv )
Tcl_Interp		*interp;
struct rt_db_internal	*intern;
int			argc;
char			**argv;
{
	struct rt_comb_internal	       *comb;
	char	buf[128];
	int	i;
	double	d;

	RT_CK_DB_INTERNAL(intern);
	comb = (struct rt_comb_internal *)intern->idb_ptr;
	RT_CK_COMB(comb);

	while( argc >= 2 ) {
		/* Force to lower case */
		for( i=0; i<128 && argv[0][i]!='\0'; i++ )
			buf[i] = isupper(argv[0][i])?tolower(argv[0][i]):argv[0][i];
		buf[i] = 0;

		if( strcmp(buf, "region")==0 ) {
			if( strcmp( argv[1], "none" ) == 0 )
				comb->region_flag = 0;
			else
			{
				if( Tcl_GetBoolean( interp, argv[1], &i )!= TCL_OK )
					return TCL_ERROR;
				comb->region_flag = (char)i;
			}
		} else if( strcmp(buf, "temp")==0 ) {
			if( !comb->region_flag ) goto not_region;
			if( strcmp( argv[1], "none" ) == 0 )
				comb->temperature = 0.0;
			else
			{
				if( Tcl_GetDouble( interp, argv[1], &d ) != TCL_OK )
					return TCL_ERROR;
				comb->temperature = (float)d;
			}
		} else if( strcmp(buf, "id")==0 ) {
			if( !comb->region_flag ) goto not_region;
			if( strcmp( argv[1], "none" ) == 0 )
				comb->region_id = 0;
			else
			{
				if( Tcl_GetInt( interp, argv[1], &i ) != TCL_OK )
					return TCL_ERROR;
				comb->region_id = (short)i;
			}
		} else if( strcmp(buf, "air")==0 ) {
			if( !comb->region_flag ) goto not_region;
			if( strcmp( argv[1], "none" ) == 0 )
				comb->aircode = 0;
			else
			{
				if( Tcl_GetInt( interp, argv[1], &i ) != TCL_OK)
					return TCL_ERROR;
				comb->aircode = (short)i;
			}
		} else if( strcmp(buf, "los")==0 ) {
			if( !comb->region_flag ) goto not_region;
			if( strcmp( argv[1], "none" ) == 0 )
				comb->los = 0;
			else
			{
				if( Tcl_GetInt( interp, argv[1], &i ) != TCL_OK )
					return TCL_ERROR;
				comb->los = (short)i;
			}
		} else if( strcmp(buf, "giftmater")==0 ) {
			if( !comb->region_flag ) goto not_region;
			if( strcmp( argv[1], "none" ) == 0 )
				comb->GIFTmater = 0;
			else
			{
				if( Tcl_GetInt( interp, argv[1], &i ) != TCL_OK )
					return TCL_ERROR;
				comb->GIFTmater = (short)i;
			}
		} else if( strcmp(buf, "rgb")==0 ) {
			if( strcmp(argv[1], "invalid")==0 || strcmp( argv[1], "none" ) == 0 ) {
				comb->rgb[0] = comb->rgb[1] =
					comb->rgb[2] = 0;
				comb->rgb_valid = 0;
			} else {
				unsigned int r, g, b;
				i = sscanf( argv[1], "%u %u %u",
					&r, &g, &b );
				if( i != 3 )   {
					Tcl_AppendResult( interp, "adjust rgb ",
						argv[1], ": not valid rgb 3-tuple\n", (char *)NULL );
					return TCL_ERROR;
				}
				comb->rgb[0] = (unsigned char)r;
				comb->rgb[1] = (unsigned char)g;
				comb->rgb[2] = (unsigned char)b;
				comb->rgb_valid = 1;
			}
		} else if( strcmp(buf, "shader" )==0 ) {
			bu_vls_trunc( &comb->shader, 0 );
			if( strcmp( argv[1], "none" ) )
			{
				bu_vls_strcat( &comb->shader, argv[1] );
				/* Leading spaces boggle the combination exporter */
				bu_vls_trimspace( &comb->shader );
			}
		} else if( strcmp(buf, "material" )==0 ) {
			bu_vls_trunc( &comb->material, 0 );
			if( strcmp( argv[1], "none" ) )
			{
				bu_vls_strcat( &comb->material, argv[1] );
				bu_vls_trimspace( &comb->material );
			}
		} else if( strcmp(buf, "inherit" )==0 ) {
			if( strcmp( argv[1], "none" ) == 0 )
				comb->inherit = 0;
			else
			{
				if( Tcl_GetBoolean( interp, argv[1], &i ) != TCL_OK )
					return TCL_ERROR;
				comb->inherit = (char)i;
			}
		} else if( strcmp(buf, "tree" )==0 ) {
			union tree	*new;

			if( strcmp( argv[1], "none" ) == 0 )
			{
				db_free_tree( comb->tree );
				comb->tree = TREE_NULL;
			}
			else
			{
				new = db_tcl_tree_parse( interp, argv[1] );
				if( new == TREE_NULL )  {
					Tcl_AppendResult( interp, "db adjust tree: bad tree '",
						argv[1], "'\n", (char *)NULL );
					return TCL_ERROR;
				}
				if( comb->tree )
					db_free_tree( comb->tree );
				comb->tree = new;
			}
		} else {
			Tcl_AppendResult( interp, "db adjust ", buf,
					  ": no such attribute",
					  (char *)NULL );
			return TCL_ERROR;
		}
		argc -= 2;
		argv += 2;
	}

	return TCL_OK;

 not_region:
	Tcl_AppendResult( interp, "adjusting attribute ",
		buf, " is not valid for a non-region combination.",
			  (char *)NULL );
	return TCL_ERROR;
}

/************************************************************************************************
 *												*
 *			Tcl interface to the Database						*
 *												*
 ************************************************************************************************/

/*
 *			R T _ T C L _ I M P O R T _ F R O M _ P A T H
 *
 *  Given the name of a database object or a full path to a leaf object,
 *  obtain the internal form of that leaf.
 *  Packaged separately mainly to make available nice Tcl error handling.
 *
 *  Returns -
 *	TCL_OK
 *	TCL_ERROR
 */
int
rt_tcl_import_from_path( interp, ip, path, wdb )
Tcl_Interp		*interp;
struct rt_db_internal	*ip;
CONST char		*path;
struct rt_wdb		*wdb;
{
	struct db_i	*dbip;
	int		status;

	/* Can't run RT_CK_DB_INTERNAL(ip), it hasn't been filled in yet */
	RT_CK_WDB(wdb);
	dbip = wdb->dbip;
	RT_CK_DBI(dbip);

#if 0
	dp = db_lookup( dbip, path, LOOKUP_QUIET );
	if( dp == NULL ) {
		Tcl_AppendResult( interp, path, ": not found\n",
				  (char *)NULL );
		return TCL_ERROR;
	}

	status = rt_db_get_internal( ip, dp, dbip, (matp_t)NULL );
	if( status < 0 ) {
		Tcl_AppendResult( interp, "rt_tcl_import_from_path failure: ",
				  path, (char *)NULL );
		return TCL_ERROR;
	}
#else
	if( strchr( path, '/' ) )
	{
		/* This is a path */
		struct db_tree_state	ts;
		struct db_full_path	old_path;
		struct db_full_path	new_path;
		struct directory	*dp_curr;
		int			ret;

		db_init_db_tree_state( &ts, dbip );
		db_full_path_init(&old_path);
		db_full_path_init(&new_path);

		if( db_string_to_path( &new_path, dbip, path ) < 0 )  {
			Tcl_AppendResult(interp, "rt_tcl_import_from_path: '",
				path, "' contains unknown object names\n", (char *)NULL);
			return TCL_ERROR;
		}

		dp_curr = DB_FULL_PATH_CUR_DIR( &new_path );
		ret = db_follow_path( &ts, &old_path, &new_path, LOOKUP_NOISY );
		db_free_full_path( &old_path );
		db_free_full_path( &new_path );

		if( ret < 0 )  {
			Tcl_AppendResult(interp, "rt_tcl_import_from_path: '",
				path, "' is a bad path\n", (char *)NULL );
			return TCL_ERROR;
		}

		status = wdb_import( wdb, ip, dp_curr->d_namep, ts.ts_mat );
		if( status == -4 )  {
			Tcl_AppendResult( interp, dp_curr->d_namep,
					" not found in path ", path, "\n",
					(char *)NULL );
			return TCL_ERROR;
		}
		if( status < 0 ) {
			Tcl_AppendResult( interp, "wdb_import failure: ",
					  dp_curr->d_namep, (char *)NULL );
			return TCL_ERROR;
		}
	}
	else
	{
		status = wdb_import( wdb, ip, path, (matp_t)NULL );
		if( status == -4 )  {
			Tcl_AppendResult( interp, path, ": not found\n",
					  (char *)NULL );
			return TCL_ERROR;
		}
		if( status < 0 ) {
			Tcl_AppendResult( interp, "wdb_import failure: ",
					  path, (char *)NULL );
			return TCL_ERROR;
		}
	}
#endif
	return TCL_OK;
}

/*
 *			R T _ P A R S E T A B _ T C L G E T
 *
 *  This is the generic routine to be listed in rt_functab[].ft_tclget
 *  for those solid types which are fully described by their ft_parsetab
 *  entry.
 *
 *  'attr' is specified to retrieve only one attribute, rather than all.
 *  Example:  "db get ell.s B" to get only the B vector.
 */
int
rt_parsetab_tclget( interp, intern, attr )
Tcl_Interp			*interp;
CONST struct rt_db_internal	*intern;
CONST char			*attr;
{
	register CONST struct bu_structparse	*sp = NULL;
	register CONST struct rt_functab	*ftp;
	int                     status;
	Tcl_DString             ds;
	struct bu_vls           str;

	RT_CK_DB_INTERNAL( intern );
	ftp = intern->idb_meth;
	RT_CK_FUNCTAB(ftp);

	sp = ftp->ft_parsetab;
	if( !sp )  {
		Tcl_AppendResult( interp, ftp->ft_label,
 " {a Tcl output routine for this type of object has not yet been implemented}",
		  (char *)NULL );
		return TCL_ERROR;
	}

	bu_vls_init( &str );
	Tcl_DStringInit( &ds );

	if( attr == (char *)0 ) {
		/* Print out solid type and all attributes */
		Tcl_DStringAppendElement( &ds, ftp->ft_label );
		while( sp->sp_name != NULL ) {
			Tcl_DStringAppendElement( &ds, sp->sp_name );
			bu_vls_trunc( &str, 0 );
			bu_vls_struct_item( &str, sp,
					 (char *)intern->idb_ptr, ' ' );
			Tcl_DStringAppendElement( &ds, bu_vls_addr(&str) );
			++sp;
		}
		status = TCL_OK;
	} else {
		if( bu_vls_struct_item_named( &str, sp, attr,
				   (char *)intern->idb_ptr, ' ') < 0 ) {
			bu_vls_printf(&str,
				"Objects of type %s do not have a %s attribute.",
				ftp->ft_label, attr);
			status = TCL_ERROR;
		} else {
			status = TCL_OK;
		}
		Tcl_DStringAppendElement( &ds, bu_vls_addr(&str) );
	}

	Tcl_DStringResult( interp, &ds );
	Tcl_DStringFree( &ds );
	bu_vls_free( &str );

	return status;
}

/*
 *			R T _ C O M B _ T C L F O R M
 */
int
rt_comb_tclform( ftp, interp )
CONST struct rt_functab *ftp;
Tcl_Interp		*interp;
{
	RT_CK_FUNCTAB(ftp);

	Tcl_AppendResult( interp,
"region {%s} id {%d} air {%d} los {%d} GIFTmater {%d} rgb {%d %d %d} \
shader {%s} material {%s} inherit {%s} tree {%s}", (char *)NULL );
	return TCL_OK;
}

/*
 *			R T _ C O M B _ M A K E
 *
 *  Create a blank combination with appropriate values.
 *
 *  Called via rt_functab[ID_COMBINATION].ft_make().
 */
void
rt_comb_make( ftp, intern, diameter )
CONST struct rt_functab	*ftp;
struct rt_db_internal	*intern;
double			diameter;
{
	struct rt_comb_internal *comb;

	intern->idb_type = ID_COMBINATION;
	intern->idb_meth = &rt_functab[ID_COMBINATION];
	intern->idb_ptr = bu_calloc( sizeof(struct rt_comb_internal), 1,
					    "rt_comb_internal" );

	comb = (struct rt_comb_internal *)intern->idb_ptr;
	comb->magic = (long)RT_COMB_MAGIC;
	comb->temperature = -1;
	comb->tree = (union tree *)0;
	comb->region_flag = 1;
	comb->region_id = 0;
	comb->aircode = 0;
	comb->GIFTmater = 0;
	comb->los = 0;
	comb->rgb_valid = 0;
	comb->rgb[0] = comb->rgb[1] = comb->rgb[2] = 0;
	bu_vls_init( &comb->shader );
	bu_vls_init( &comb->material );
	comb->inherit = 0;
}

/*
 *			R T _ G E N E R I C _ M A K E
 *
 *  This one assumes that making all the parameters null is fine.
 *  (More generally, diameter == 0 means make 'em all zero, otherwise
 *  build something interesting to look at.)
 */
void
rt_generic_make( ftp, intern, diameter )
CONST struct rt_functab	*ftp;
struct rt_db_internal	*intern;
double			diameter;
{
	intern->idb_type = ftp - rt_functab;
	BU_ASSERT(&rt_functab[intern->idb_type] == ftp);

	intern->idb_meth = ftp;
	intern->idb_ptr = bu_calloc( ftp->ft_internal_size, 1, "rt_generic_make");
	*((long *)(intern->idb_ptr)) = ftp->ft_internal_magic;
}

/*
 *			R T _ P A R S E T A B _ T C L A D J U S T
 *
 *  For those solids entirely defined by their parsetab.
 *  Invoked via rt_functab[].ft_tcladjust()
 */
int
rt_parsetab_tcladjust( interp, intern, argc, argv )
Tcl_Interp		*interp;
struct rt_db_internal	*intern;
int			argc;
char			**argv;
{
	CONST struct rt_functab	*ftp;

	RT_CK_DB_INTERNAL(intern);
	ftp = intern->idb_meth;
	RT_CK_FUNCTAB(ftp);

	if( ftp->ft_parsetab == (struct bu_structparse *)NULL ) {
		Tcl_AppendResult( interp, ftp->ft_label,
			  " type objects do not yet have a 'db put' (tcladjust) handler.",
			  (char *)NULL );
		return TCL_ERROR;
	}

	return bu_structparse_argv(interp, argc, argv, ftp->ft_parsetab,
				(char *)intern->idb_ptr );
}

/*
 *			R T _ P A R S E T A B _ T C L F O R M
 *
 *  Invoked via rt_functab[].ft_tclform()
 */
int
rt_parsetab_tclform( ftp, interp)
CONST struct rt_functab	*ftp;
Tcl_Interp		*interp;
{
	RT_CK_FUNCTAB(ftp);

	if( ftp->ft_parsetab )  {
		bu_structparse_get_terse_form( interp, ftp->ft_parsetab );
		return TCL_OK;
	}
	Tcl_AppendResult(interp, ftp->ft_label,
		" is a valid object type, but a 'form' routine has not yet been implemented.",
		(char *)NULL );
	return TCL_ERROR;
}


/*
 *			R T _ T C L _ S E T U P
 *
 *  Add all the supported Tcl interfaces to LIBRT routines to
 *  the list of commands known by the given interpreter.
 *
 *  wdb_open creates database "objects" which appear as Tcl procs,
 *  which respond to many operations.
 *  The "db rt_gettrees" operation in turn creates a ray-traceable object
 *  as yet another Tcl proc, which responds to additional operations.
 *
 *  Note that the MGED mainline C code automatically runs
 *  "wdb_open db" and "wdb_open .inmem" on behalf of the MGED user,
 *  which exposes all of this power.
 */
void
rt_tcl_setup(interp)
     Tcl_Interp *interp;
{
	/* initialize database objects */
	Wdb_Init(interp);

	/* initialize drawable geometry objects */
	Dgo_Init(interp);

	/* initialize view objects */
	Vo_Init(interp);

	Tcl_SetVar(interp, "rt_version", (char *)rt_version+5, TCL_GLOBAL_ONLY);
}
