/*
 *				W D B _ O B J . C
 *
 *  A database object contains the attributes and
 *  methods for controlling a BRLCAD database.
 * 
 *  Authors -
 *	Michael John Muuss
 *      Glenn Durfee
 *	Robert G. Parker
 *
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Notice -
 *	Re-distribution of this software is restricted, as described in
 *	your "Statement of Terms and Conditions for the Release of
 *	The BRL-CAD Package" license agreement.
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 2000 by the United States Army
 *	in all countries except the USA.  All rights reserved.
 */
#ifndef lint
static const char RCSid[] = "@(#)$Header$ (ARL)";
#endif

#include "conf.h"
#include <ctype.h>

#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <math.h>
#if unix
# include <fcntl.h>
# include <sys/errno.h>
#endif
#include "tcl.h"
#include "machine.h"
#include "externs.h"
#include "cmd.h"		/* this includes bu.h */
#include "vmath.h"
#include "bn.h"
#include "db.h"
#include "mater.h"
#include "rtgeom.h"
#include "raytrace.h"
#include "wdb.h"

#include "./debug.h"

/* defined in mater.c */
extern void rt_insert_color( struct mater *newp );

#define WDB_TCL_READ_ERR { \
	Tcl_AppendResult(interp, "Database read error, aborting.\n", (char *)NULL); \
	}

#define WDB_TCL_READ_ERR_return { \
	WDB_TCL_READ_ERR; \
	return TCL_ERROR; }

#define WDB_TCL_WRITE_ERR { \
	Tcl_AppendResult(interp, "Database write error, aborting.\n", (char *)NULL); \
	WDB_TCL_ERROR_RECOVERY_SUGGESTION; }

#define WDB_TCL_WRITE_ERR_return { \
	WDB_TCL_WRITE_ERR; \
	return TCL_ERROR; }

#define WDB_TCL_ALLOC_ERR { \
	Tcl_AppendResult(interp, "\
An error has occured while adding a new object to the database.\n", (char *)NULL); \
	WDB_TCL_ERROR_RECOVERY_SUGGESTION; }

#define WDB_TCL_ALLOC_ERR_return { \
	WDB_TCL_ALLOC_ERR; \
	return TCL_ERROR; }

#define WDB_TCL_DELETE_ERR(_name){ \
	Tcl_AppendResult(interp, "An error has occurred while deleting '", _name,\
	"' from the database.\n", (char *)NULL);\
	WDB_TCL_ERROR_RECOVERY_SUGGESTION; }

#define WDB_TCL_DELETE_ERR_return(_name){  \
	WDB_TCL_DELETE_ERR(_name); \
	return TCL_ERROR;  }

/* A verbose message to attempt to soothe and advise the user */
#define	WDB_TCL_ERROR_RECOVERY_SUGGESTION\
        Tcl_AppendResult(interp, "\
The in-memory table of contents may not match the status of the on-disk\n\
database.  The on-disk database should still be intact.  For safety,\n\
you should exit now, and resolve the I/O problem, before continuing.\n", (char *)NULL)

#define WDB_READ_ERR { \
	bu_log("Database read error, aborting\n"); }

#define WDB_READ_ERR_return { \
	WDB_READ_ERR; \
	return;  }

#define WDB_WRITE_ERR { \
	bu_log("Database write error, aborting.\n"); \
	WDB_ERROR_RECOVERY_SUGGESTION; }	

#define WDB_WRITE_ERR_return { \
	WDB_WRITE_ERR; \
	return;  }

/* For errors from db_diradd() or db_alloc() */
#define WDB_ALLOC_ERR { \
	bu_log("\nAn error has occured while adding a new object to the database.\n"); \
	WDB_ERROR_RECOVERY_SUGGESTION; }

#define WDB_ALLOC_ERR_return { \
	WDB_ALLOC_ERR; \
	return;  }

/* A verbose message to attempt to soothe and advise the user */
#define	WDB_ERROR_RECOVERY_SUGGESTION\
        bu_log(WDB_ERROR_RECOVERY_MESSAGE)

#define WDB_ERROR_RECOVERY_MESSAGE "\
The in-memory table of contents may not match the status of the on-disk\n\
database.  The on-disk database should still be intact.  For safety,\n\
you should exit now, and resolve the I/O problem, before continuing.\n"

#define	WDB_TCL_CHECK_READ_ONLY \
	if (wdbp->dbip->dbi_read_only) {\
		Tcl_AppendResult(interp, "Sorry, this database is READ-ONLY\n", (char *)NULL); \
		return TCL_ERROR; \
	}

#define WDB_MAX_LEVELS 12
#define WDB_CPEVAL	0
#define WDB_LISTPATH	1
#define WDB_LISTEVAL	2

struct wdb_trace_data {
	Tcl_Interp		*wtd_interp;
	struct db_i		*wtd_dbip;
	struct directory	*wtd_path[WDB_MAX_LEVELS];
	struct directory	*wtd_obj[WDB_MAX_LEVELS];
	mat_t			wtd_xform;
	int			wtd_objpos;
	int			wtd_prflag;
	int			wtd_flag;
};

/* defined in libbn/bn_tcl.c */
BU_EXTERN(void		bn_tcl_mat_print, (Tcl_Interp *interp, const char *title, const mat_t m));

/* from librt/tcl.c */
extern int rt_tcl_rt();
extern int rt_tcl_import_from_path();
extern void rt_generic_make();

/* from librt/wdb_comb_std.c */
extern int wdb_comb_std_tcl();

/* from db5_scan.c */
HIDDEN int db5_scan();

int wdb_init_obj();
int wdb_get_tcl();
int wdb_attr_tcl();
int wdb_attr_rm_tcl();
int wdb_pathsum_cmd();

static int wdb_open_tcl();
static int wdb_close_tcl();
static int wdb_decode_dbip();
static struct db_i *wdb_prep_dbip();

static int wdb_cmd();
static int wdb_match_tcl();
static int wdb_put_tcl();
static int wdb_adjust_tcl();
static int wdb_form_tcl();
static int wdb_tops_tcl();
static int wdb_rt_gettrees_tcl();
static int wdb_shells_tcl();
static int wdb_dump_tcl();
static int wdb_dbip_tcl();
static int wdb_ls_tcl();
static int wdb_list_tcl();
static int wdb_pathsum_tcl();
static int wdb_expand_tcl();
static int wdb_kill_tcl();
static int wdb_killall_tcl();
static int wdb_killtree_tcl();
static void wdb_killtree_callback();
static int wdb_copy_tcl();
static int wdb_move_tcl();
static int wdb_move_all_tcl();
static int wdb_concat_tcl();
static int wdb_copyeval_tcl();
static int wdb_dup_tcl();
static int wdb_group_tcl();
static int wdb_remove_tcl();
static int wdb_region_tcl();
static int wdb_comb_tcl();
static int wdb_find_tcl();
static int wdb_which_tcl();
static int wdb_title_tcl();
static int wdb_tree_tcl();
static int wdb_color_tcl();
static int wdb_prcolor_tcl();
static int wdb_tol_tcl();
static int wdb_push_tcl();
static int wdb_whatid_tcl();
static int wdb_keep_tcl();
static int wdb_cat_tcl();
static int wdb_instance_tcl();
static int wdb_observer_tcl();
static int wdb_reopen_tcl();
static int wdb_make_bb_tcl();
static int wdb_make_name_tcl();
static int wdb_units_tcl();
static int wdb_hide_tcl();
static int wdb_unhide_tcl();
static int wdb_xpush_tcl();
static int wdb_showmats_tcl();
static int wdb_nmg_collapse_tcl();
static int wdb_nmg_simplify_tcl();
static int wdb_summary_tcl();
static int wdb_pathlist_tcl();
static int wdb_lt_tcl();
static int wdb_version_tcl();

static void wdb_deleteProc();
static void wdb_deleteProc_rt();

static void wdb_do_trace();
static void wdb_trace();

int wdb_cmpdirname();
void wdb_vls_col_item();
void wdb_vls_col_eol();
void wdb_vls_col_pr4v();
void wdb_vls_long_dpp();
void wdb_vls_line_dpp();
void wdb_do_list();
struct directory ** wdb_getspace();
struct directory *wdb_combadd();
void wdb_identitize();
static void wdb_dir_summary();
static struct directory ** wdb_dir_getspace();
static union tree *wdb_pathlist_leaf_func();

static struct bu_cmdtab wdb_cmds[] = {
	{"adjust",	wdb_adjust_tcl},
	{"attr",	wdb_attr_tcl},
	{"attr_rm",	wdb_attr_rm_tcl},
	{"c",		wdb_comb_std_tcl},
	{"cat",		wdb_cat_tcl},
	{"close",	wdb_close_tcl},
	{"color",	wdb_color_tcl},
	{"comb",	wdb_comb_tcl},
	{"concat",	wdb_concat_tcl},
	{"copyeval",	wdb_copyeval_tcl},
	{"cp",		wdb_copy_tcl},
	{"dbip",	wdb_dbip_tcl},
	{"dump",	wdb_dump_tcl},
	{"dup",		wdb_dup_tcl},
	{"expand",	wdb_expand_tcl},
	{"find",	wdb_find_tcl},
	{"form",	wdb_form_tcl},
	{"g",		wdb_group_tcl},
	{"get",		wdb_get_tcl},
	{"hide",	wdb_hide_tcl},
	{"i",		wdb_instance_tcl},
	{"keep",	wdb_keep_tcl},
	{"kill",	wdb_kill_tcl},
	{"killall",	wdb_killall_tcl},
	{"killtree",	wdb_killtree_tcl},
	{"l",		wdb_list_tcl},
	{"listeval",	wdb_pathsum_tcl},
	{"ls",		wdb_ls_tcl},
	{"lt",		wdb_lt_tcl},
	{"make_bb",	wdb_make_bb_tcl},
	{"make_name",	wdb_make_name_tcl},
	{"match",	wdb_match_tcl},
	{"mv",		wdb_move_tcl},
	{"mvall",	wdb_move_all_tcl},
	{"nmg_collapse",	wdb_nmg_collapse_tcl},
	{"nmg_simplify",	wdb_nmg_simplify_tcl},
	{"observer",	wdb_observer_tcl},
	{"open",	wdb_reopen_tcl},
	{"pathlist",	wdb_pathlist_tcl},
	{"paths",	wdb_pathsum_tcl},
	{"prcolor",	wdb_prcolor_tcl},
	{"push",	wdb_push_tcl},
	{"put",		wdb_put_tcl},
	{"r",		wdb_region_tcl},
	{"rm",		wdb_remove_tcl},
	{"rt_gettrees",	wdb_rt_gettrees_tcl},
	{"shells",	wdb_shells_tcl},
	{"showmats",	wdb_showmats_tcl},
	{"summary",	wdb_summary_tcl},
	{"title",	wdb_title_tcl},
	{"tol",		wdb_tol_tcl},
	{"tops",	wdb_tops_tcl},
	{"tree",	wdb_tree_tcl},
	{"unhide",	wdb_unhide_tcl},
	{"units",	wdb_units_tcl},
	{"version",	wdb_version_tcl},
	{"whatid",	wdb_whatid_tcl},
	{"whichair",	wdb_which_tcl},
	{"whichid",	wdb_which_tcl},
	{"xpush",	wdb_xpush_tcl},
#if 0
	/* Commands to be added */
	{"comb_color",	wdb_comb_color_tcl},
	{"copymat",	wdb_copymat_tcl},
	{"getmat",	wdb_getmat_tcl},
	{"putmat",	wdb_putmat_tcl},
	{"which_shader",	wdb_which_shader_tcl},
	{"rcodes",	wdb_rcodes_tcl},
	{"wcodes",	wdb_wcodes_tcl},
	{"rmater",	wdb_rmater_tcl},
	{"wmater",	wdb_wmater_tcl},
	{"analyze",	wdb_analyze_tcl},
	{"inside",	wdb_inside_tcl},
#endif
	{(char *)NULL,	(int (*)())0 }
};

int
Wdb_Init(interp)
     Tcl_Interp *interp;
{
	(void)Tcl_CreateCommand(interp, "wdb_open", wdb_open_tcl,
				(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

	return TCL_OK;
}

/*
 *			W D B _ C M D
 *
 * Generic interface for database commands.
 * Usage:
 *        procname cmd ?args?
 *
 * Returns: result of wdb command.
 */
static int
wdb_cmd(clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int     argc;
     char    **argv;
{
	return bu_cmd(clientData, interp, argc, argv, wdb_cmds, 1);
}

/*
 * Called by Tcl when the object is destroyed.
 */
static void
wdb_deleteProc(clientData)
     ClientData clientData;
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	/* free observers */
	bu_observer_free(&wdbp->wdb_observers);

	/* notify drawable geometry objects of the impending close */
	dgo_impending_wdb_close(wdbp, wdbp->wdb_interp);

	RT_CK_WDB(wdbp);
	BU_LIST_DEQUEUE(&wdbp->l);
	bu_vls_free(&wdbp->wdb_name);
	wdb_close(wdbp);
}

/*
 * Create an command/object named "oname" in "interp"
 * using "wdbp" as its state.
 */
int
wdb_init_obj(Tcl_Interp		*interp,
	     struct rt_wdb	*wdbp,	/* pointer to object */
	     char		*oname)	/* object name */
{
	if (wdbp == RT_WDB_NULL) {
		Tcl_AppendResult(interp, "wdb_open ", oname, " failed", NULL);
		return TCL_ERROR;
	}

	/* initialize rt_wdb */
	bu_vls_init(&wdbp->wdb_name);
	bu_vls_strcpy(&wdbp->wdb_name, oname);

#if 0
	/*XXXX already initialize by wdb_dbopen */
	/* initilize tolerance structures */
	wdbp->wdb_ttol.magic = RT_TESS_TOL_MAGIC;
	wdbp->wdb_ttol.abs = 0.0;		/* disabled */
	wdbp->wdb_ttol.rel = 0.01;
	wdbp->wdb_ttol.norm = 0.0;		/* disabled */

	wdbp->wdb_tol.magic = BN_TOL_MAGIC;
	wdbp->wdb_tol.dist = 0.005;
	wdbp->wdb_tol.dist_sq = wdbp->wdb_tol.dist * wdbp->wdb_tol.dist;
	wdbp->wdb_tol.perp = 1e-6;
	wdbp->wdb_tol.para = 1 - wdbp->wdb_tol.perp;
#endif

	/* initialize tree state */
	wdbp->wdb_initial_tree_state = rt_initial_tree_state;  /* struct copy */
	wdbp->wdb_initial_tree_state.ts_ttol = &wdbp->wdb_ttol;
	wdbp->wdb_initial_tree_state.ts_tol = &wdbp->wdb_tol;

	/* default region ident codes */
	wdbp->wdb_item_default = 1000;
	wdbp->wdb_air_default = 0;
	wdbp->wdb_mat_default = 1;
	wdbp->wdb_los_default = 100;

	/* resource structure */
	wdbp->wdb_resp = &rt_uniresource;

	BU_LIST_INIT(&wdbp->wdb_observers.l);
	wdbp->wdb_interp = interp;

	/* append to list of rt_wdb's */
	BU_LIST_APPEND(&rt_g.rtg_headwdb.l,&wdbp->l);

	/* Instantiate the newprocname, with clientData of wdbp */
	/* Beware, returns a "token", not TCL_OK. */
	(void)Tcl_CreateCommand(interp, oname, wdb_cmd,
				(ClientData)wdbp, wdb_deleteProc);

	/* Return new function name as result */
	Tcl_AppendResult(interp, oname, (char *)NULL);
	
	return TCL_OK;
}

/*
 *			W D B _ O P E N _ T C L
 *
 *  A TCL interface to wdb_fopen() and wdb_dbopen().
 *
 *  Implicit return -
 *	Creates a new TCL proc which responds to get/put/etc. arguments
 *	when invoked.  clientData of that proc will be rt_wdb pointer
 *	for this instance of the database.
 *	Easily allows keeping track of multiple databases.
 *
 *  Explicit return -
 *	wdb pointer, for more traditional C-style interfacing.
 *
 *  Example -
 *	set wdbp [wdb_open .inmem inmem $dbip]
 *	.inmem get box.s
 *	.inmem close
 *
 *	wdb_open db file "bob.g"
 *	db get white.r
 *	db close
 */
static int
wdb_open_tcl(ClientData	clientData,
	     Tcl_Interp	*interp,
	     int	argc,
	     char	**argv)
{
	struct rt_wdb *wdbp;

	if (argc == 1) {
		/* get list of database objects */
		for (BU_LIST_FOR(wdbp, rt_wdb, &rt_g.rtg_headwdb.l))
			Tcl_AppendResult(interp, bu_vls_addr(&wdbp->wdb_name), " ", (char *)NULL);

		return TCL_OK;
	}

	if (argc < 3 || 4 < argc) {
#if 0
		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib wdb_open");
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
#else
		Tcl_AppendResult(interp, "\
Usage: wdb_open\n\
       wdb_open newprocname file filename\n\
       wdb_open newprocname disk $dbip\n\
       wdb_open newprocname disk_append $dbip\n\
       wdb_open newprocname inmem $dbip\n\
       wdb_open newprocname inmem_append $dbip\n\
       wdb_open newprocname db filename\n\
       wdb_open newprocname filename\n",
				 NULL);
		return TCL_ERROR;
#endif
	}

	/* Delete previous proc (if any) to release all that memory, first */
	(void)Tcl_DeleteCommand(interp, argv[1]);

	if (argc == 3 || strcmp(argv[2], "db") == 0) {
		struct db_i	*dbip;
		int i;

		if (argc == 3)
			i = 2;
		else
			i = 3;

		if ((dbip = wdb_prep_dbip(interp, argv[i])) == DBI_NULL)
			return TCL_ERROR;
		RT_CK_DBI_TCL(interp,dbip);

		wdbp = wdb_dbopen(dbip, RT_WDB_TYPE_DB_DISK);
	} else if (strcmp(argv[2], "file") == 0) {
		wdbp = wdb_fopen( argv[3] );
	} else {
		struct db_i	*dbip;

		if (wdb_decode_dbip(interp, argv[3], &dbip) != TCL_OK)
			return TCL_ERROR;

		if (strcmp( argv[2], "disk" ) == 0)
			wdbp = wdb_dbopen(dbip, RT_WDB_TYPE_DB_DISK);
		else if (strcmp(argv[2], "disk_append") == 0)
			wdbp = wdb_dbopen(dbip, RT_WDB_TYPE_DB_DISK_APPEND_ONLY);
		else if (strcmp( argv[2], "inmem" ) == 0)
			wdbp = wdb_dbopen(dbip, RT_WDB_TYPE_DB_INMEM);
		else if (strcmp( argv[2], "inmem_append" ) == 0)
			wdbp = wdb_dbopen(dbip, RT_WDB_TYPE_DB_INMEM_APPEND_ONLY);
		else {
			Tcl_AppendResult(interp, "wdb_open ", argv[2],
					 " target type not recognized", NULL);
			return TCL_ERROR;
		}
	}

	return wdb_init_obj(interp, wdbp, argv[1]);
}

int
wdb_decode_dbip(interp, dbip_string, dbipp)
     Tcl_Interp *interp;
     char *dbip_string;
     struct db_i **dbipp;
{

	*dbipp = (struct db_i *)atol(dbip_string);

	/* Could core dump */
	RT_CK_DBI_TCL(interp,*dbipp);

	return TCL_OK;
}

/*
 * Open/Create the database and build the in memory directory.
 */
struct db_i *
wdb_prep_dbip(interp, filename)
     Tcl_Interp *interp;
     char *filename;
{
	struct db_i *dbip;

	/* open database */
	if (((dbip = db_open(filename, "r+w")) == DBI_NULL) &&
	    ((dbip = db_open(filename, "r"  )) == DBI_NULL)) {
		/*
		 * Check to see if we can access the database
		 */
#if unix
		if (access(filename, R_OK|W_OK) != 0 && errno != ENOENT) {
			perror(filename);
			return DBI_NULL;
		}
#endif
#if WIN32
#endif

		/* db_create does a db_dirbuild */
		if ((dbip = db_create(filename, 5)) == DBI_NULL) {
			Tcl_AppendResult(interp,
					 "wdb_open: failed to create ", filename,
					 (char *)NULL);
			if (dbip == DBI_NULL)
				Tcl_AppendResult(interp,
						 "opendb: no database is currently opened!", \
						 (char *)NULL);

			return DBI_NULL;
		}
	} else
		/* --- Scan geometry database and build in-memory directory --- */
		db_dirbuild(dbip);


	return dbip;
}

/****************** Database Object Methods ********************/
int
wdb_close_cmd(struct rt_wdb	*wdbp,
	      Tcl_Interp	*interp,
	      int		argc,
	      char 		**argv)
{
	struct bu_vls vls;

	if (argc != 1) {
		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib wdb_close");
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	/*
	 * Among other things, this will call wdb_deleteProc.
	 * Note - wdb_deleteProc is being passed clientdata.
	 *        It ought to get interp as well.
	 */
	Tcl_DeleteCommand(interp, bu_vls_addr(&wdbp->wdb_name));

	return TCL_OK;
}

/*
 * Close a BRLCAD database object.
 *
 * USAGE:
 *	  procname close
 */
static int
wdb_close_tcl(ClientData	clientData,
	      Tcl_Interp	*interp,
	      int		argc,
	      char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_close_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_reopen_cmd(struct rt_wdb	*wdbp,
	       Tcl_Interp	*interp,
	       int		argc,
	       char 		**argv)
{
	struct db_i *dbip;
	struct bu_vls vls;

	/* get database filename */
	if (argc == 1) {
		Tcl_AppendResult(interp, wdbp->dbip->dbi_filename, (char *)NULL);
		return TCL_OK;
	}

	/* set database filename */
	if (argc == 2) {
		if ((dbip = wdb_prep_dbip(interp, argv[1])) == DBI_NULL) {
			return TCL_ERROR;
		}

		/* XXXnotify observers */
		/* notify drawable geometry objects associated with this database */
		dgo_zapall(wdbp, interp);

		/* close current database */
		db_close(wdbp->dbip);

		wdbp->dbip = dbip;

		/* --- Scan geometry database and build in-memory directory --- */
		db_dirbuild(wdbp->dbip);

		Tcl_AppendResult(interp, wdbp->dbip->dbi_filename, (char *)NULL);
		return TCL_OK;
	}

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "helplib_alias wdb_reopen %s", argv[0]);
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
}

/*
 *
 * Usage:
 *        procname open [filename]
 */
static int
wdb_reopen_tcl(ClientData	clientData,
	       Tcl_Interp	*interp,
	       int		argc,
	       char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_reopen_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_match_cmd(struct rt_wdb	*wdbp,
	      Tcl_Interp	*interp,
	      int		argc,
	      char 		**argv)
{
	struct bu_vls	matches;

	RT_CK_WDB_TCL(interp,wdbp);
	
	/* Verify that this wdb supports lookup operations
	   (non-null dbip) */
	if (wdbp->dbip == 0) {
		Tcl_AppendResult( interp, "this database does not support lookup operations" );
		return TCL_ERROR;
	}

	bu_vls_init(&matches);
	for (++argv; *argv != NULL; ++argv) {
		if (db_regexp_match_all(&matches, wdbp->dbip, *argv) > 0)
			bu_vls_strcat(&matches, " ");
	}
	bu_vls_trimspace(&matches);
	Tcl_AppendResult(interp, bu_vls_addr(&matches), (char *)NULL);
	bu_vls_free(&matches);
	return TCL_OK;
}

/*
 *			W D B _ M A T C H _ T C L
 *
 * Returns (in interp->result) a list (possibly empty) of all matches to
 * the (possibly wildcard-containing) arguments given.
 * Does *NOT* return tokens that do not match anything, unlike the
 * "expand" command.
 */

static int
wdb_match_tcl(ClientData	clientData,
	      Tcl_Interp	*interp,
	      int		argc,
	      char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_match_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_get_cmd(struct rt_wdb	*wdbp,
	    Tcl_Interp		*interp,
	    int			argc,
	    char 		**argv)
{
	int			status;
	struct rt_db_internal	intern;

	if (argc < 2 || argc > 3) {
		Tcl_AppendResult(interp,
				 "wrong # args: should be \"", argv[0],
				 " objName ?attr?\"", (char *)NULL);
		return TCL_ERROR;
	}

	/* Verify that this wdb supports lookup operations
	   (non-null dbip) */
	if (wdbp->dbip == 0) {
		Tcl_AppendResult(interp,
				 "db does not support lookup operations",
				 (char *)NULL);
		return TCL_ERROR;
	}

	if (rt_tcl_import_from_path(interp, &intern, argv[1], wdbp) == TCL_ERROR)
		return TCL_ERROR;

	status = intern.idb_meth->ft_tclget(interp, &intern, argv[2]);
	rt_db_free_internal(&intern, &rt_uniresource);
	return status;
}

/*
 *			W D B _ G E T_ T C L
 *
 **
 ** For use with Tcl, this routine accepts as its first argument the name
 ** of an object in the database.  If only one argument is given, this routine
 ** then fills the result string with the (minimal) attributes of the item.
 ** If a second, optional, argument is provided, this function looks up the
 ** property with that name of the item given, and returns it as the result
 ** string.
 **/
/* NOTE: This is called directly by gdiff/g_diff.c */

int
wdb_get_tcl(ClientData	clientData,
	    Tcl_Interp	*interp,
	    int		argc,
	    char	**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_get_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_put_cmd(struct rt_wdb	*wdbp,
	    Tcl_Interp		*interp,
	    int			argc,
	    char 		**argv)
{
	struct rt_db_internal			intern;
	register const struct rt_functab	*ftp;
	int					i;
	char				       *name;
	char				        type[16];

	if (argc < 3) {
		Tcl_AppendResult(interp,
				 "wrong # args: should be db put objName objType attrs",
				 (char *)NULL);
		return TCL_ERROR;
	}

	name = argv[1];
    
	/* Verify that this wdb supports lookup operations (non-null dbip).
	 * stdout/file wdb objects don't, but can still be written to.
	 * If not, just skip the lookup test and write the object
	 */
	if (wdbp->dbip && db_lookup(wdbp->dbip, argv[1], LOOKUP_QUIET) != DIR_NULL ) {
		Tcl_AppendResult(interp, argv[1], " already exists",
				 (char *)NULL);
		return TCL_ERROR;
	}

	RT_INIT_DB_INTERNAL(&intern);

	for (i = 0; argv[2][i] != 0 && i < 16; i++) {
		type[i] = isupper(argv[2][i]) ? tolower(argv[2][i]) :
			argv[2][i];
	}
	type[i] = 0;

	ftp = rt_get_functab_by_label(type);
	if (ftp == NULL) {
		Tcl_AppendResult(interp, type,
				 " is an unknown object type.",
				 (char *)NULL);
		return TCL_ERROR;
	}

	if (ftp->ft_make) {
		ftp->ft_make(ftp, &intern, 0.0);
	} else {
		rt_generic_make(ftp, &intern, 0.0);
	}

	if (ftp->ft_tcladjust(interp, &intern, argc-3, argv+3, &rt_uniresource) == TCL_ERROR) {
		rt_db_free_internal(&intern, &rt_uniresource);
		return TCL_ERROR;
	}

	if (wdb_put_internal(wdbp, name, &intern, 1.0) < 0)  {
		Tcl_AppendResult(interp, "wdb_put_internal(", argv[1],
				 ") failure", (char *)NULL);
		rt_db_free_internal(&intern, &rt_uniresource);
		return TCL_ERROR;
	}

	rt_db_free_internal( &intern, &rt_uniresource );
	return TCL_OK;
}

/*
 *			W D B _ P U T _ T C L
 **
 ** Creates an object and stuffs it into the databse.
 ** All arguments must be specified.  Object cannot already exist.
 **/

static int
wdb_put_tcl(ClientData	clientData,
	    Tcl_Interp	*interp,
	    int		argc,
	    char	**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_put_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_adjust_cmd(struct rt_wdb	*wdbp,
	       Tcl_Interp	*interp,
	       int		argc,
	       char 		**argv)
{
	register struct directory	*dp;
	int				 status;
	char				*name;
	struct rt_db_internal		 intern;

	if (argc < 4) {
		Tcl_AppendResult(interp,
				 "wrong # args: should be \"db adjust objName attr value ?attr? ?value?...\"",
				 (char *)NULL);
		return TCL_ERROR;
	}
	name = argv[1];

	/* Verify that this wdb supports lookup operations (non-null dbip) */
	RT_CK_DBI_TCL(interp,wdbp->dbip);

	dp = db_lookup(wdbp->dbip, name, LOOKUP_QUIET);
	if (dp == DIR_NULL) {
		Tcl_AppendResult(interp, name, ": not found",
				 (char *)NULL );
		return TCL_ERROR;
	}

	status = rt_db_get_internal(&intern, dp, wdbp->dbip, (matp_t)NULL, &rt_uniresource);
	if (status < 0) {
		Tcl_AppendResult(interp, "rt_db_get_internal(", name,
				 ") failure", (char *)NULL );
		return TCL_ERROR;
	}
	RT_CK_DB_INTERNAL(&intern);

	/* Find out what type of object we are dealing with and tweak it. */
	RT_CK_FUNCTAB(intern.idb_meth);

	status = intern.idb_meth->ft_tcladjust(interp, &intern, argc-2, argv+2, &rt_uniresource);
	if( status == TCL_OK && wdb_put_internal(wdbp, name, &intern, 1.0) < 0)  {
		Tcl_AppendResult(interp, "wdb_export(", name,
				 ") failure", (char *)NULL);
		rt_db_free_internal(&intern, &rt_uniresource);
		return TCL_ERROR;
	}

	/* notify observers */
	bu_observer_notify(interp, &wdbp->wdb_observers, bu_vls_addr(&wdbp->wdb_name));

	return status;
}

/*
 *			W D B _ A D J U S T _ T C L
 *
 **
 ** For use with Tcl, this routine accepts as its first argument an item in
 ** the database; as its remaining arguments it takes the properties that
 ** need to be changed and their values.
 *
 *  Example of adjust operation on a solid:
 *	.inmem adjust LIGHT V { -46 -13 5 }
 *
 *  Example of adjust operation on a combination:
 *	.inmem adjust light.r rgb { 255 255 255 }
 */

static int
wdb_adjust_tcl(ClientData	clientData,
	       Tcl_Interp	*interp,
	       int		argc,
	       char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_adjust_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_form_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char 		**argv)
{
	const struct rt_functab		*ftp;

	if (argc != 2) {
		Tcl_AppendResult(interp, "wrong # args: should be \"db form objType\"",
				 (char *)NULL);
		return TCL_ERROR;
	}

	if ((ftp = rt_get_functab_by_label(argv[1])) == NULL) {
		Tcl_AppendResult(interp, "There is no geometric object type \"",
				 argv[1], "\".", (char *)NULL);
		return TCL_ERROR;
	}
	return ftp->ft_tclform(ftp, interp);
}

/*
 *			W D B _ F O R M _ T C L
 */
static int
wdb_form_tcl(ClientData clientData,
	     Tcl_Interp *interp,
	     int	argc,
	     char	**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_form_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_tops_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char 		**argv)
{
	register struct directory *dp;
	register int i;

	RT_CK_WDB_TCL(interp, wdbp);
	RT_CK_DBI_TCL(interp, wdbp->dbip);

	/* Can this be executed only sometimes?
	   Perhaps a "dirty bit" on the database? */
	db_update_nref(wdbp->dbip, &rt_uniresource);
	
	for (i = 0; i < RT_DBNHASH; i++)
		for (dp = wdbp->dbip->dbi_Head[i];
		     dp != DIR_NULL;
		     dp = dp->d_forw)  {
			if (dp->d_nref == 0)
				Tcl_AppendElement( interp, dp->d_namep);
		}
	return TCL_OK;
}

/*
 *			W D B _ T O P S _ T C L
 *
 *  NON-PARALLEL because of rt_uniresource
 */
static int
wdb_tops_tcl(ClientData	clientData,
	     Tcl_Interp	*interp,
	     int	argc,
	     char	**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_tops_cmd(wdbp, interp, argc-1, argv+1);
}

/*
 *			R T _ T C L _ D E L E T E P R O C _ R T
 *
 *  Called when the named proc created by rt_gettrees() is destroyed.
 */
static void
wdb_deleteProc_rt(ClientData clientData)
{
	struct application	*ap = (struct application *)clientData;
	struct rt_i		*rtip;

	RT_AP_CHECK(ap);
	rtip = ap->a_rt_i;
	RT_CK_RTI(rtip);

	rt_free_rti(rtip);
	ap->a_rt_i = (struct rt_i *)NULL;

	bu_free( (genptr_t)ap, "struct application" );
}

int
wdb_rt_gettrees_cmd(struct rt_wdb	*wdbp,
		    Tcl_Interp		*interp,
		    int			argc,
		    char 		**argv)
{
	struct rt_i		*rtip;
	struct application	*ap;
	struct resource		*resp;
	char			*newprocname;

	RT_CK_WDB_TCL(interp, wdbp);
	RT_CK_DBI_TCL(interp, wdbp->dbip);

	if (argc < 3) {
		Tcl_AppendResult(interp,
				 "rt_gettrees: wrong # args: should be \"",
				 argv[0],
				 " newprocname [-i] [-u] treetops...\"", (char *)NULL );
		return TCL_ERROR;
	}

	rtip = rt_new_rti(wdbp->dbip);
	newprocname = argv[1];

	/* Delete previous proc (if any) to release all that memory, first */
	(void)Tcl_DeleteCommand(interp, newprocname);

	while (argv[2][0] == '-') {
		if (strcmp( argv[2], "-i") == 0) {
			rtip->rti_dont_instance = 1;
			argc--;
			argv++;
			continue;
		}
		if (strcmp(argv[2], "-u") == 0) {
			rtip->useair = 1;
			argc--;
			argv++;
			continue;
		}
		break;
	}

	if (rt_gettrees(rtip, argc-2, (const char **)&argv[2], 1) < 0) {
		Tcl_AppendResult(interp,
				 "rt_gettrees() returned error", (char *)NULL);
		rt_free_rti(rtip);
		return TCL_ERROR;
	}

	/* Establish defaults for this rt_i */
	rtip->rti_hasty_prep = 1;	/* Tcl isn't going to fire many rays */

	/*
	 *  In case of multiple instances of the library, make sure that
	 *  each instance has a separate resource structure,
	 *  because the bit vector lengths depend on # of solids.
	 *  And the "overwrite" sequence in Tcl is to create the new
	 *  proc before running the Tcl_CmdDeleteProc on the old one,
	 *  which in this case would trash rt_uniresource.
	 *  Once on the rti_resources list, rt_clean() will clean 'em up.
	 */
	BU_GETSTRUCT(resp, resource);
	rt_init_resource(resp, 0, rtip);
	BU_ASSERT_PTR( BU_PTBL_GET(&rtip->rti_resources, 0), !=, NULL );

	BU_GETSTRUCT(ap, application);
	ap->a_magic = RT_AP_MAGIC;
	ap->a_resource = resp;
	ap->a_rt_i = rtip;
	ap->a_purpose = "Conquest!";

	rt_ck(rtip);

	/* Instantiate the proc, with clientData of wdb */
	/* Beware, returns a "token", not TCL_OK. */
	(void)Tcl_CreateCommand(interp, newprocname, rt_tcl_rt,
				(ClientData)ap, wdb_deleteProc_rt);

	/* Return new function name as result */
	Tcl_AppendResult(interp, newprocname, (char *)NULL);

	return TCL_OK;
}

/*
 *			W D B _ R T _ G E T T R E E S _ T C L
 *
 *  Given an instance of a database and the name of some treetops,
 *  create a named "ray-tracing" object (proc) which will respond to
 *  subsequent operations.
 *  Returns new proc name as result.
 *
 *  Example:
 *	.inmem rt_gettrees .rt all.g light.r
 */
static int
wdb_rt_gettrees_tcl(ClientData	clientData,
		    Tcl_Interp     *interp,
		    int		argc,
		    char	      **argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_rt_gettrees_cmd(wdbp, interp, argc-1, argv+1);
}

struct showmats_data {
	Tcl_Interp	*smd_interp;
	int		smd_count;
	char		*smd_child;
	mat_t		smd_mat;
};

static void
Do_showmats(struct db_i			*dbip,
	    struct rt_comb_internal	*comb,
	    union tree			*comb_leaf,
	    genptr_t			user_ptr1,
	    genptr_t			user_ptr2,
	    genptr_t			user_ptr3)
{
	struct showmats_data	*smdp;

	RT_CK_DBI(dbip);
	RT_CK_TREE(comb_leaf);

	smdp = (struct showmats_data *)user_ptr1;

	if (strcmp(comb_leaf->tr_l.tl_name, smdp->smd_child))
		return;

	smdp->smd_count++;
	if (smdp->smd_count > 1) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "\n\tOccurrence #%d:\n", smdp->smd_count);
		Tcl_AppendResult(smdp->smd_interp, bu_vls_addr(&vls), (char *)NULL);
		bu_vls_free(&vls);
	}

	bn_tcl_mat_print(smdp->smd_interp, "", comb_leaf->tr_l.tl_mat);
	if (smdp->smd_count == 1) {
		mat_t tmp_mat;
		if (comb_leaf->tr_l.tl_mat) {
			bn_mat_mul(tmp_mat, smdp->smd_mat, comb_leaf->tr_l.tl_mat);
			MAT_COPY(smdp->smd_mat, tmp_mat);
		}
	}
}

int
wdb_showmats_cmd(struct rt_wdb	*wdbp,
		 Tcl_Interp	*interp,
		 int		argc,
		 char		**argv)
{
	struct showmats_data sm_data;
	char *parent;
	struct directory *dp;
	int max_count=1;

	if (argc != 2) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_showmats %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	sm_data.smd_interp = interp;
	MAT_IDN(sm_data.smd_mat);

	parent = strtok(argv[1], "/");
	while ((sm_data.smd_child = strtok((char *)NULL, "/")) != NULL) {
		struct rt_db_internal	intern;
		struct rt_comb_internal *comb;

		if ((dp = db_lookup(wdbp->dbip, parent, LOOKUP_NOISY)) == DIR_NULL)
			return TCL_ERROR;

		Tcl_AppendResult(interp, parent, "\n", (char *)NULL);

		if (!(dp->d_flags & DIR_COMB)) {
			Tcl_AppendResult(interp, "\tThis is not a combination\n", (char *)NULL);
			break;
		}

		if (rt_db_get_internal(&intern, dp, wdbp->dbip, (fastf_t *)NULL, &rt_uniresource) < 0)
			WDB_TCL_READ_ERR_return;
		comb = (struct rt_comb_internal *)intern.idb_ptr;

		sm_data.smd_count = 0;

		if (comb->tree)
			db_tree_funcleaf(wdbp->dbip, comb, comb->tree, Do_showmats,
					 (genptr_t)&sm_data, (genptr_t)NULL, (genptr_t)NULL);
		rt_comb_ifree(&intern, &rt_uniresource);

		if (!sm_data.smd_count) {
			Tcl_AppendResult(interp, sm_data.smd_child, " is not a member of ",
					 parent, "\n", (char *)NULL);
			return TCL_ERROR;
		}
		if (sm_data.smd_count > max_count)
			max_count = sm_data.smd_count;

		parent = sm_data.smd_child;
	}
	Tcl_AppendResult(interp, parent, "\n", (char *)NULL);

	if (max_count > 1)
		Tcl_AppendResult(interp, "\nAccumulated matrix (using first occurrence of each object):\n", (char *)NULL);
	else
		Tcl_AppendResult(interp, "\nAccumulated matrix:\n", (char *)NULL);

	bn_tcl_mat_print(interp, "", sm_data.smd_mat);

	return TCL_OK;
}

static int
wdb_showmats_tcl(ClientData	clientData,
		 Tcl_Interp	*interp,
		 int		argc,
		 char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;
	
	return wdb_showmats_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_shells_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char		**argv)
{
	struct directory *old_dp,*new_dp;
	struct rt_db_internal old_intern,new_intern;
	struct model *m_tmp,*m;
	struct nmgregion *r_tmp,*r;
	struct shell *s_tmp,*s;
	int shell_count=0;
	struct bu_vls shell_name;
	long **trans_tbl;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc != 2) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_shells %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	if ((old_dp = db_lookup(wdbp->dbip,  argv[1], LOOKUP_NOISY)) == DIR_NULL)
		return TCL_ERROR;

	if (rt_db_get_internal(&old_intern, old_dp, wdbp->dbip, bn_mat_identity, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "rt_db_get_internal() error\n", (char *)NULL);
		return TCL_ERROR;
	}

	if (old_intern.idb_type != ID_NMG) {
		Tcl_AppendResult(interp, "Object is not an NMG!!!\n", (char *)NULL);
		return TCL_ERROR;
	}

	m = (struct model *)old_intern.idb_ptr;
	NMG_CK_MODEL(m);

	bu_vls_init(&shell_name);
	for (BU_LIST_FOR(r, nmgregion, &m->r_hd)) {
		for (BU_LIST_FOR(s, shell, &r->s_hd)) {
			s_tmp = nmg_dup_shell(s, &trans_tbl, &wdbp->wdb_tol);
			bu_free((genptr_t)trans_tbl, "trans_tbl");

			m_tmp = nmg_mmr();
			r_tmp = BU_LIST_FIRST(nmgregion, &m_tmp->r_hd);

			BU_LIST_DEQUEUE(&s_tmp->l);
			BU_LIST_APPEND(&r_tmp->s_hd, &s_tmp->l);
			s_tmp->r_p = r_tmp;
			nmg_m_reindex(m_tmp, 0);
			nmg_m_reindex(m, 0);

			bu_vls_printf(&shell_name, "shell.%d", shell_count);
			while (db_lookup(wdbp->dbip, bu_vls_addr( &shell_name), 0) != DIR_NULL) {
				bu_vls_trunc(&shell_name, 0);
				shell_count++;
				bu_vls_printf(&shell_name, "shell.%d", shell_count);
			}

			/* Export NMG as a new solid */
			RT_INIT_DB_INTERNAL(&new_intern);
			new_intern.idb_type = ID_NMG;
			new_intern.idb_meth = &rt_functab[ID_NMG];
			new_intern.idb_ptr = (genptr_t)m_tmp;

			if ((new_dp=db_diradd(wdbp->dbip, bu_vls_addr(&shell_name), -1, 0,
					      DIR_SOLID, (genptr_t)&new_intern.idb_type)) == DIR_NULL) {
				WDB_TCL_ALLOC_ERR_return;
			}

			/* make sure the geometry/bounding boxes are up to date */
			nmg_rebound(m_tmp, &wdbp->wdb_tol);


			if (rt_db_put_internal(new_dp, wdbp->dbip, &new_intern, &rt_uniresource) < 0) {
				/* Free memory */
				nmg_km(m_tmp);
				Tcl_AppendResult(interp, "rt_db_put_internal() failure\n", (char *)NULL);
				return TCL_ERROR;
			}
			/* Internal representation has been freed by rt_db_put_internal */
			new_intern.idb_ptr = (genptr_t)NULL;
		}
	}
	bu_vls_free(&shell_name);

	return TCL_OK;
}

static int
wdb_shells_tcl(ClientData	clientData,
	       Tcl_Interp	*interp,
	       int		argc,
	       char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_shells_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_dump_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char		**argv)
{
	struct rt_wdb	*op;
	int		ret;

	RT_CK_WDB_TCL(interp, wdbp);
	RT_CK_DBI_TCL(interp, wdbp->dbip);

	if (argc != 2) {
		Tcl_AppendResult(interp,
				 "dump: wrong # args: should be \"",
				 "dump filename.g", (char *)NULL);
		return TCL_ERROR;
	}

	if ((op = wdb_fopen(argv[1])) == RT_WDB_NULL) {
		Tcl_AppendResult(interp, "dump:  ", argv[1],
				 ": cannot create", (char *)NULL);
		return TCL_ERROR;
	}

	ret = db_dump(op, wdbp->dbip);
	wdb_close(op);

	if (ret < 0) {
		Tcl_AppendResult(interp, "dump ", argv[1],
				 ": db_dump() error", (char *)NULL);
		return TCL_ERROR;
	}

	return TCL_OK;
}

/*
 *			W D B _ D U M P _ T C L
 *
 *  Write the current state of a database object out to a file.
 *
 *  Example:
 *	.inmem dump "/tmp/foo.g"
 */
static int
wdb_dump_tcl(ClientData	clientData,
	     Tcl_Interp	*interp,
	     int	argc,
	     char	**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_dump_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_dbip_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char		**argv)
{
	struct bu_vls vls;

	bu_vls_init(&vls);

	if (argc != 1) {
		bu_vls_printf(&vls, "helplib_alias wdb_dbip %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	bu_vls_printf(&vls, "%lu", (unsigned long)wdbp->dbip);
	Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
	bu_vls_free(&vls);
	return TCL_OK;
}

/*
 *
 * Usage:
 *        procname dbip
 *
 * Returns: database objects dbip.
 */
static int
wdb_dbip_tcl(ClientData	clientData,
	     Tcl_Interp	*interp,
	     int	argc,
	     char	**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_dbip_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_ls_cmd(struct rt_wdb	*wdbp,
	   Tcl_Interp		*interp,
	   int			argc,
	   char 		**argv)
{
	struct bu_vls vls;
	register struct directory *dp;
	register int i;
	int c;
	int aflag = 0;		/* print all objects without formatting */
	int cflag = 0;		/* print combinations */
	int rflag = 0;		/* print regions */
	int sflag = 0;		/* print solids */
	int lflag = 0;		/* use long format */
	struct directory **dirp;
	struct directory **dirp0 = (struct directory **)NULL;

	bu_vls_init(&vls);

	if (argc < 1 || MAXARGS < argc) {
		bu_vls_printf(&vls, "helplib_alias wdb_ls %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	bu_optind = 1;	/* re-init bu_getopt() */
	while ((c = bu_getopt(argc, argv, "acrsl")) != EOF) {
		switch (c) {
		case 'a':
			aflag = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		default:
			bu_vls_printf(&vls, "Unrecognized option - %c", c);
			Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
			bu_vls_free(&vls);
			return TCL_ERROR;
		}
	}
	argc -= (bu_optind - 1);
	argv += (bu_optind - 1);

	/* create list of objects in database */
	if (argc > 1) {
		/* Just list specified names */
		dirp = wdb_getspace(wdbp->dbip, argc-1);
		dirp0 = dirp;
		/*
		 * Verify the names, and add pointers to them to the array.
		 */
		for (i = 1; i < argc; i++) {
			if ((dp = db_lookup(wdbp->dbip, argv[i], LOOKUP_NOISY)) == DIR_NULL)
				continue;
			*dirp++ = dp;
		}
	} else {
		/* Full table of contents */
		dirp = wdb_getspace(wdbp->dbip, 0);	/* Enough for all */
		dirp0 = dirp;
		/*
		 * Walk the directory list adding pointers (to the directory
		 * entries) to the array.
		 */
		for (i = 0; i < RT_DBNHASH; i++)
			for (dp = wdbp->dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw) {
				if( !aflag && (dp->d_flags & DIR_HIDDEN) )
					continue;
				*dirp++ = dp;
			}
	}

	if (lflag)
		wdb_vls_long_dpp(&vls, dirp0, (int)(dirp - dirp0),
				 aflag, cflag, rflag, sflag);
	else if (aflag || cflag || rflag || sflag)
		wdb_vls_line_dpp(&vls, dirp0, (int)(dirp - dirp0),
				 aflag, cflag, rflag, sflag);
	else
		wdb_vls_col_pr4v(&vls, dirp0, (int)(dirp - dirp0));

	Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
	bu_vls_free(&vls);
	bu_free((genptr_t)dirp0, "wdb_getspace dp[]");

	return TCL_OK;
}

/*
 *
 * Usage:
 *        procname ls [args]
 *
 * Returns: list objects in this database object.
 */
static int
wdb_ls_tcl(ClientData	clientData,
	   Tcl_Interp	*interp,
	   int		argc,
	   char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_ls_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_list_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char 		**argv)
{
	register struct directory	*dp;
	register int			arg;
	struct bu_vls			str;
	int				id;
	int				recurse = 0;
	char				*listeval = "listeval";
	struct rt_db_internal		intern;

	if (argc < 2 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_list %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	if (argc > 1 && strcmp(argv[1], "-r") == 0) {
		recurse = 1;

		/* skip past used args */
		--argc;
		++argv;
	}

	/* skip past used args */
	--argc;
	++argv;

	bu_vls_init(&str);

	for (arg = 0; arg < argc; arg++) {
		if (recurse) {
			char *tmp_argv[3];

			tmp_argv[0] = listeval;
			tmp_argv[1] = argv[arg];
			tmp_argv[2] = (char *)NULL;

			wdb_pathsum_cmd(wdbp, interp, 2, tmp_argv);
		} else if (strchr(argv[arg], '/')) {
			struct db_tree_state ts;
			struct db_full_path path;

			db_full_path_init( &path );
			ts = wdbp->wdb_initial_tree_state;     /* struct copy */
			ts.ts_dbip = wdbp->dbip;
			ts.ts_resp = &rt_uniresource;
			MAT_IDN(ts.ts_mat);

			if (db_follow_path_for_state(&ts, &path, argv[arg], 1))
				continue;

			dp = DB_FULL_PATH_CUR_DIR( &path );

			if ((id = rt_db_get_internal(&intern, dp, wdbp->dbip, ts.ts_mat, &rt_uniresource)) < 0) {
				Tcl_AppendResult(interp, "rt_db_get_internal(", dp->d_namep,
						 ") failure", (char *)NULL );
				continue;
			}

			db_free_full_path( &path );

			bu_vls_printf( &str, "%s:  ", argv[arg] );

			if (rt_functab[id].ft_describe(&str, &intern, 99, wdbp->dbip->dbi_base2local, &rt_uniresource) < 0)
				Tcl_AppendResult(interp, dp->d_namep, ": describe error", (char *)NULL);

			rt_db_free_internal(&intern, &rt_uniresource);
		} else {
			if ((dp = db_lookup(wdbp->dbip, argv[arg], LOOKUP_NOISY)) == DIR_NULL)
				continue;

			wdb_do_list(wdbp->dbip, interp, &str, dp, 99);	/* very verbose */
		}
	}

	Tcl_AppendResult(interp, bu_vls_addr(&str), (char *)NULL);
	bu_vls_free(&str);

	return TCL_OK;
}

/*
 *
 *  Usage:
 *        procname l [-r] arg(s)
 *
 *  List object information, verbose.
 */
static int
wdb_list_tcl(clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int     argc;
     char    **argv;
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_list_cmd(wdbp, interp, argc-1, argv+1);
}

static void
wdb_do_trace(struct db_i		*dbip,
	     struct rt_comb_internal	*comb,
	     union tree			*comb_leaf,
	     genptr_t			user_ptr1,
	     genptr_t			user_ptr2,
	     genptr_t			user_ptr3)
{
	int			*pathpos;
	matp_t			old_xlate;
	mat_t			new_xlate;
	struct directory	*nextdp;
	struct wdb_trace_data	*wtdp;

	RT_CK_DBI(dbip);
	RT_CK_TREE(comb_leaf);

	if ((nextdp = db_lookup(dbip, comb_leaf->tr_l.tl_name, LOOKUP_NOISY)) == DIR_NULL)
		return;

	pathpos = (int *)user_ptr1;
	old_xlate = (matp_t)user_ptr2;
	wtdp = (struct wdb_trace_data *)user_ptr3;

	if (comb_leaf->tr_l.tl_mat) {
		bn_mat_mul(new_xlate, old_xlate, comb_leaf->tr_l.tl_mat);
	} else {
		MAT_COPY(new_xlate, old_xlate);
	}

	wdb_trace(nextdp, (*pathpos)+1, new_xlate, wtdp);
}

static void
wdb_trace(register struct directory	*dp,
	  int				pathpos,
	  const mat_t			old_xlate,
	  struct wdb_trace_data		*wtdp)
{
	struct rt_db_internal	intern;
	struct rt_comb_internal	*comb;
	int			i;
	int			id;
	struct bu_vls		str;

#if 0
	if (dbip == DBI_NULL)
		return;
#endif

	bu_vls_init(&str);

	if (pathpos >= WDB_MAX_LEVELS) {
		struct bu_vls tmp_vls;

		bu_vls_init(&tmp_vls);
		bu_vls_printf(&tmp_vls, "nesting exceeds %d levels\n", WDB_MAX_LEVELS);
		Tcl_AppendResult(wtdp->wtd_interp, bu_vls_addr(&tmp_vls), (char *)NULL);
		bu_vls_free(&tmp_vls);

		for (i=0; i<WDB_MAX_LEVELS; i++)
			Tcl_AppendResult(wtdp->wtd_interp, "/", wtdp->wtd_path[i]->d_namep, (char *)NULL);

		Tcl_AppendResult(wtdp->wtd_interp, "\n", (char *)NULL);
		return;
	}

	if (dp->d_flags & DIR_COMB) {
		if (rt_db_get_internal(&intern, dp, wtdp->wtd_dbip, (fastf_t *)NULL, &rt_uniresource) < 0)
			WDB_READ_ERR_return;

		wtdp->wtd_path[pathpos] = dp;
		comb = (struct rt_comb_internal *)intern.idb_ptr;
		if (comb->tree)
			db_tree_funcleaf(wtdp->wtd_dbip, comb, comb->tree, wdb_do_trace,
				(genptr_t)&pathpos, (genptr_t)old_xlate, (genptr_t)wtdp);
		rt_comb_ifree(&intern, &rt_uniresource);
		return;
	}

	/* not a combination  -  should have a solid */

	/* last (bottom) position */
	wtdp->wtd_path[pathpos] = dp;

	/* check for desired path */
	if( wtdp->wtd_flag == WDB_CPEVAL ) {
		for (i=0; i<=pathpos; i++) {
			if (wtdp->wtd_path[i]->d_addr != wtdp->wtd_obj[i]->d_addr) {
				/* not the desired path */
				return;
			}
		}
	} else {
		for (i=0; i<wtdp->wtd_objpos; i++) {
			if (wtdp->wtd_path[i]->d_addr != wtdp->wtd_obj[i]->d_addr) {
				/* not the desired path */
				return;
			}
		}
	}

	/* have the desired path up to objpos */
	MAT_COPY(wtdp->wtd_xform, old_xlate);
	wtdp->wtd_prflag = 1;

	if (wtdp->wtd_flag == WDB_CPEVAL)
		return;

	/* print the path */
	for (i=0; i<pathpos; i++)
		Tcl_AppendResult(wtdp->wtd_interp, "/", wtdp->wtd_path[i]->d_namep, (char *)NULL);

	if (wtdp->wtd_flag == WDB_LISTPATH) {
		bu_vls_printf( &str, "/%s:\n", dp->d_namep );
		Tcl_AppendResult(wtdp->wtd_interp, bu_vls_addr(&str), (char *)NULL);
		bu_vls_free(&str);
		return;
	}

	/* NOTE - only reach here if wtd_flag == WDB_LISTEVAL */
	Tcl_AppendResult(wtdp->wtd_interp, "/", (char *)NULL);
	if ((id=rt_db_get_internal(&intern, dp, wtdp->wtd_dbip, wtdp->wtd_xform, &rt_uniresource)) < 0) {
		Tcl_AppendResult(wtdp->wtd_interp, "rt_db_get_internal(", dp->d_namep,
				 ") failure", (char *)NULL );
		return;
	}
	bu_vls_printf(&str, "%s:\n", dp->d_namep);
	if (rt_functab[id].ft_describe(&str, &intern, 1, wtdp->wtd_dbip->dbi_base2local, &rt_uniresource) < 0)
		Tcl_AppendResult(wtdp->wtd_interp, dp->d_namep, ": describe error\n", (char *)NULL);
	rt_db_free_internal(&intern, &rt_uniresource);
	Tcl_AppendResult(wtdp->wtd_interp, bu_vls_addr(&str), (char *)NULL);
	bu_vls_free(&str);
}

int
wdb_pathsum_cmd(struct rt_wdb	*wdbp,
		Tcl_Interp	*interp,
		int		argc,
		char 		**argv)
{
	int			i, pos_in;
	struct wdb_trace_data	wtd;

	if (argc < 2 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias %s%s %s", "wdb_", argv[0], argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	/*
	 *	paths are matched up to last input member
	 *      ANY path the same up to this point is considered as matching
	 */

	/* initialize wtd */
	wtd.wtd_interp = interp;
	wtd.wtd_dbip = wdbp->dbip;
	wtd.wtd_flag = WDB_CPEVAL;
	wtd.wtd_prflag = 0;

	pos_in = 1;

	/* find out which command was entered */
	if (strcmp(argv[0], "paths") == 0) {
		/* want to list all matching paths */
		wtd.wtd_flag = WDB_LISTPATH;
	}
	if (strcmp(argv[0], "listeval") == 0) {
		/* want to list evaluated solid[s] */
		wtd.wtd_flag = WDB_LISTEVAL;
	}

	if (argc == 2 && strchr(argv[1], '/')) {
		char *tok;
		wtd.wtd_objpos = 0;

		tok = strtok(argv[1], "/");
		while (tok) {
			if ((wtd.wtd_obj[wtd.wtd_objpos++] = db_lookup(wdbp->dbip, tok, LOOKUP_NOISY)) == DIR_NULL)
				return TCL_ERROR;
			tok = strtok((char *)NULL, "/");
		}
	} else {
		wtd.wtd_objpos = argc-1;

		/* build directory pointer array for desired path */
		for (i=0; i<wtd.wtd_objpos; i++) {
			if ((wtd.wtd_obj[i] = db_lookup(wdbp->dbip, argv[pos_in+i], LOOKUP_NOISY)) == DIR_NULL)
				return TCL_ERROR;
		}
	}

	MAT_IDN(wtd.wtd_xform);

	wdb_trace(wtd.wtd_obj[0], 0, bn_mat_identity, &wtd);

	if (wtd.wtd_prflag == 0) {
		/* path not found */
		Tcl_AppendResult(interp, "PATH:  ", (char *)NULL);
		for (i=0; i<wtd.wtd_objpos; i++)
			Tcl_AppendResult(interp, "/", wtd.wtd_obj[i]->d_namep, (char *)NULL);

		Tcl_AppendResult(interp, "  NOT FOUND\n", (char *)NULL);
	}

	return TCL_OK;
}


/*
 *			W D B _ P A T H S U M _ T C L
 *
 *  Common code for several direct db methods: listeval, paths
 *  Also used as support routine for "l" (list) command.
 *
 *  1.  produces path for purposes of matching
 *  2.  gives all paths matching the input path OR
 *  3.  gives a summary of all paths matching the input path
 *	including the final parameters of the solids at the bottom
 *	of the matching paths
 *
 * Usage:
 *        procname (WDB_LISTEVAL|paths) args(s)
 */
static int
wdb_pathsum_tcl(ClientData	clientData,
		Tcl_Interp	*interp,
		int		argc,
		char		**argv)
{
	struct rt_wdb	*wdbp = (struct rt_wdb *)clientData;

	return wdb_pathsum_cmd(wdbp, interp, argc-1, argv+1);
}


static void
wdb_scrape_escapes_AppendResult(Tcl_Interp	*interp,
				char		*str)
{
	char buf[2];
	buf[1] = '\0';
    
	while (*str) {
		buf[0] = *str;
		if (*str != '\\') {
			Tcl_AppendResult(interp, buf, NULL);
		} else if (*(str+1) == '\\') {
			Tcl_AppendResult(interp, buf, NULL);
			++str;
		}
		if (*str == '\0')
			break;
		++str;
	}
}

int
wdb_expand_cmd(struct rt_wdb	*wdbp,
	       Tcl_Interp	*interp,
	       int		argc,
	       char 		**argv)
{
	register char *pattern;
	register struct directory *dp;
	register int i, whicharg;
	int regexp, nummatch, thismatch, backslashed;

	if (argc < 1 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "help wdb_expand");
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	nummatch = 0;
	backslashed = 0;
	for (whicharg = 1; whicharg < argc; whicharg++) {
		/* If * ? or [ are present, this is a regular expression */
		pattern = argv[whicharg];
		regexp = 0;
		do {
			if ((*pattern == '*' || *pattern == '?' || *pattern == '[') &&
			    !backslashed) {
				regexp = 1;
				break;
			}
			if (*pattern == '\\' && !backslashed)
				backslashed = 1;
			else
				backslashed = 0;
		} while (*pattern++);

		/* If it isn't a regexp, copy directly and continue */
		if (regexp == 0) {
			if (nummatch > 0)
				Tcl_AppendResult(interp, " ", NULL);
			wdb_scrape_escapes_AppendResult(interp, argv[whicharg]);
			++nummatch;
			continue;
		}
	
		/* Search for pattern matches.
		 * If any matches are found, we do not have to worry about
		 * '\' escapes since the match coming from dp->d_namep will be
		 * clean. In the case of no matches, just copy the argument
		 * directly.
		 */

		pattern = argv[whicharg];
		thismatch = 0;
		for (i = 0; i < RT_DBNHASH; i++) {
			for (dp = wdbp->dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw) {
				if (!db_regexp_match(pattern, dp->d_namep))
					continue;
				/* Successful match */
				if (nummatch == 0)
					Tcl_AppendResult(interp, dp->d_namep, NULL);
				else 
					Tcl_AppendResult(interp, " ", dp->d_namep, NULL);
				++nummatch;
				++thismatch;
			}
		}
		if (thismatch == 0) {
			if (nummatch > 0)
				Tcl_AppendResult(interp, " ", NULL);
			wdb_scrape_escapes_AppendResult(interp, argv[whicharg]);
		}
	}

	return TCL_OK;
}

/*
 * Performs wildcard expansion (matched to the database elements)
 * on its given arguments.  The result is returned in interp->result.
 *
 * Usage:
 *        procname expand [args]
 */
static int
wdb_expand_tcl(clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int     argc;
     char    **argv;
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_expand_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_kill_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char		**argv)
{
	register struct directory *dp;
	register int i;
	int	is_phony;
	int	verbose = LOOKUP_NOISY;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc < 2 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_kill %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	/* skip past "-f" */
	if (argc > 1 && strcmp(argv[1], "-f") == 0) {
		verbose = LOOKUP_QUIET;
		argc--;
		argv++;
	}

	for (i = 1; i < argc; i++) {
		if ((dp = db_lookup(wdbp->dbip,  argv[i], verbose)) != DIR_NULL) {
			is_phony = (dp->d_addr == RT_DIR_PHONY_ADDR);

			/* don't worry about phony objects */
			if (is_phony)
				continue;

			/* notify drawable geometry objects associated with this database object */
			dgo_eraseobjall_callback(wdbp->dbip, interp, dp);

			if (db_delete(wdbp->dbip, dp) < 0 ||
			    db_dirdelete(wdbp->dbip, dp) < 0) {
				/* Abort kill processing on first error */
				Tcl_AppendResult(interp,
						 "an error occurred while deleting ",
						 argv[i], (char *)NULL);
				return TCL_ERROR;
			}
		}
	}

	return TCL_OK;
}

/*
 * Usage:
 *        procname kill arg(s)
 */
static int
wdb_kill_tcl(ClientData	clientData,
	     Tcl_Interp	*interp,
	     int	argc,
	     char	**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_kill_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_killall_cmd(struct rt_wdb	*wdbp,
		Tcl_Interp	*interp,
		int		argc,
		char 		**argv)
{
	register int			i,k;
	register struct directory	*dp;
	struct rt_db_internal		intern;
	struct rt_comb_internal		*comb;
	int				ret;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc < 2 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias  wdb_killall %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	ret = TCL_OK;

	/* Examine all COMB nodes */
	for (i = 0; i < RT_DBNHASH; i++) {
		for (dp = wdbp->dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw) {
			if (!(dp->d_flags & DIR_COMB))
				continue;

			if (rt_db_get_internal(&intern, dp, wdbp->dbip, (fastf_t *)NULL, &rt_uniresource) < 0) {
				Tcl_AppendResult(interp, "rt_db_get_internal(", dp->d_namep,
						 ") failure", (char *)NULL );
				ret = TCL_ERROR;
				continue;
			}
			comb = (struct rt_comb_internal *)intern.idb_ptr;
			RT_CK_COMB(comb);

			for (k=1; k<argc; k++) {
				int	code;

				code = db_tree_del_dbleaf(&(comb->tree), argv[k], &rt_uniresource);
				if (code == -1)
					continue;	/* not found */
				if (code == -2)
					continue;	/* empty tree */
				if (code < 0) {
					Tcl_AppendResult(interp, "  ERROR_deleting ",
							 dp->d_namep, "/", argv[k],
							 "\n", (char *)NULL);
					ret = TCL_ERROR;
				} else {
					Tcl_AppendResult(interp, "deleted ",
							 dp->d_namep, "/", argv[k],
							 "\n", (char *)NULL);
				}
			}

			if (rt_db_put_internal(dp, wdbp->dbip, &intern, &rt_uniresource) < 0) {
				Tcl_AppendResult(interp,
						 "ERROR: Unable to write new combination into database.\n",
						 (char *)NULL);
				ret = TCL_ERROR;
				continue;
			}
		}
	}

	if (ret != TCL_OK) {
		Tcl_AppendResult(interp,
				 "KILL skipped because of earlier errors.\n",
				 (char *)NULL);
		return ret;
	}

	/* ALL references removed...now KILL the object[s] */
	/* reuse argv[] */
	argv[0] = "kill";
	return wdb_kill_cmd(wdbp, interp, argc, argv);
}

/*
 * Kill object[s] and remove all references to the object[s].
 *
 * Usage:
 *        procname killall arg(s)
 */
static int
wdb_killall_tcl(ClientData	clientData,
		Tcl_Interp	*interp,
		int		argc,
		char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_killall_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_killtree_cmd(struct rt_wdb	*wdbp,
		 Tcl_Interp	*interp,
		 int		argc,
		 char 		**argv)
{
	register struct directory *dp;
	register int i;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc < 2 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_killtree %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	for (i=1; i<argc; i++) {
		if ((dp = db_lookup(wdbp->dbip, argv[i], LOOKUP_NOISY)) == DIR_NULL)
			continue;

		/* ignore phony objects */
		if (dp->d_addr == RT_DIR_PHONY_ADDR)
			continue;

		db_functree(wdbp->dbip, dp,
			    wdb_killtree_callback, wdb_killtree_callback,
			    wdbp->wdb_resp, (genptr_t)interp);
	}

	return TCL_OK;
}

/*
 * Kill all paths belonging to an object.
 *
 * Usage:
 *        procname killtree arg(s)
 */
static int
wdb_killtree_tcl(ClientData	clientData,
		 Tcl_Interp	*interp,
		 int		argc,
		 char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_killtree_cmd(wdbp, interp, argc-1, argv+1);
}

/*
 *			K I L L T R E E
 */
static void
wdb_killtree_callback(struct db_i		*dbip,
		      register struct directory *dp,
		      genptr_t			*ptr)
{
	Tcl_Interp *interp = (Tcl_Interp *)ptr;

	if (dbip == DBI_NULL)
		return;

	Tcl_AppendResult(interp, "KILL ", (dp->d_flags & DIR_COMB) ? "COMB" : "Solid",
			 ":  ", dp->d_namep, "\n", (char *)NULL);

	/* notify drawable geometry objects associated with this database object */
	dgo_eraseobjall_callback(interp, dbip, dp);

	if (db_delete(dbip, dp) < 0 || db_dirdelete(dbip, dp) < 0) {
		Tcl_AppendResult(interp,
				 "an error occurred while deleting ",
				 dp->d_namep, "\n", (char *)NULL);
	}
}

int
wdb_copy_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char 		**argv)
{
	register struct directory *proto;
	register struct directory *dp;
	struct bu_external external;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc != 3) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_copy %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	if ((proto = db_lookup(wdbp->dbip,  argv[1], LOOKUP_NOISY)) == DIR_NULL)
		return TCL_ERROR;

	if (db_lookup(wdbp->dbip, argv[2], LOOKUP_QUIET) != DIR_NULL) {
		Tcl_AppendResult(interp, argv[2], ":  already exists", (char *)NULL);
		return TCL_ERROR;
	}

	if (db_get_external(&external , proto , wdbp->dbip)) {
		Tcl_AppendResult(interp, "Database read error, aborting", (char *)NULL);
		return TCL_ERROR;
	}

	if ((dp=db_diradd(wdbp->dbip, argv[2], -1, 0, proto->d_flags, (genptr_t)&proto->d_minor_type)) == DIR_NULL ) {
		Tcl_AppendResult(interp,
				 "An error has occured while adding a new object to the database.",
				 (char *)NULL);
		return TCL_ERROR;
	}

	if (db_put_external(&external, dp, wdbp->dbip) < 0) {
		bu_free_external(&external);
		Tcl_AppendResult(interp, "Database write error, aborting", (char *)NULL);
		return TCL_ERROR;
	}
	bu_free_external(&external);

	return TCL_OK;
}

/*
 * Usage:
 *        procname cp from to
 */
static int
wdb_copy_tcl(ClientData	clientData,
	     Tcl_Interp	*interp,
	     int	argc,
	     char	**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_copy_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_move_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char 		**argv)
{
	register struct directory	*dp;
	struct rt_db_internal		intern;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc != 3) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_move %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	if ((dp = db_lookup(wdbp->dbip,  argv[1], LOOKUP_NOISY)) == DIR_NULL)
		return TCL_ERROR;

	if (db_lookup(wdbp->dbip, argv[2], LOOKUP_QUIET) != DIR_NULL) {
		Tcl_AppendResult(interp, argv[2], ":  already exists", (char *)NULL);
		return TCL_ERROR;
	}

	if (rt_db_get_internal(&intern, dp, wdbp->dbip, (fastf_t *)NULL, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "Database read error, aborting", (char *)NULL);
		return TCL_ERROR;
	}

	/*  Change object name in the in-memory directory. */
	if (db_rename(wdbp->dbip, dp, argv[2]) < 0) {
		rt_db_free_internal(&intern, &rt_uniresource);
		Tcl_AppendResult(interp, "error in db_rename to ", argv[2],
				 ", aborting", (char *)NULL);
		return TCL_ERROR;
	}

	/* Re-write to the database.  New name is applied on the way out. */
	if (rt_db_put_internal(dp, wdbp->dbip, &intern, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "Database write error, aborting", (char *)NULL);
		return TCL_ERROR;
	}

	return TCL_OK;
}

/*
 * Rename an object.
 *
 * Usage:
 *        procname mv from to
 */
static int
wdb_move_tcl(ClientData	clientData,
	     Tcl_Interp	*interp,
	     int	argc,
	     char	**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_move_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_move_all_cmd(struct rt_wdb	*wdbp,
		 Tcl_Interp	*interp,
		 int		argc,
		 char 		**argv)
{
	register int	i;
	register struct directory *dp;
	struct rt_db_internal	intern;
	struct rt_comb_internal *comb;
	struct bu_ptbl		stack;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc != 3) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_moveall %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	if (wdbp->dbip->dbi_version < 5 && (int)strlen(argv[2]) > NAMESIZE) {
		struct bu_vls tmp_vls;

		bu_vls_init(&tmp_vls);
		bu_vls_printf(&tmp_vls, "ERROR: name length limited to %d characters in v4 databases\n", NAMESIZE);
		Tcl_AppendResult(interp, bu_vls_addr(&tmp_vls), (char *)NULL);
		bu_vls_free(&tmp_vls);
		return TCL_ERROR;
	}

	/* rename the record itself */
	if ((dp = db_lookup(wdbp->dbip, argv[1], LOOKUP_NOISY )) == DIR_NULL)
		return TCL_ERROR;

	if (db_lookup(wdbp->dbip, argv[2], LOOKUP_QUIET) != DIR_NULL) {
		Tcl_AppendResult(interp, argv[2], ":  already exists", (char *)NULL);
		return TCL_ERROR;
	}

	/*  Change object name in the directory. */
	if (db_rename(wdbp->dbip, dp, argv[2]) < 0) {
		Tcl_AppendResult(interp, "error in rename to ", argv[2],
				 ", aborting", (char *)NULL);
		return TCL_ERROR;
	}

	/* Change name in the file */
	if (rt_db_get_internal(&intern, dp, wdbp->dbip, (fastf_t *)NULL, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "Database read error, aborting", (char *)NULL);
		return TCL_ERROR;
	}

	if (rt_db_put_internal(dp, wdbp->dbip, &intern, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "Database write error, aborting", (char *)NULL);
		return TCL_ERROR;
	}

	bu_ptbl_init(&stack, 64, "combination stack for wdb_mvall_cmd");

	/* Examine all COMB nodes */
	for (i = 0; i < RT_DBNHASH; i++) {
		for (dp = wdbp->dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw) {
			union tree	*comb_leaf;
			int		done=0;
			int		changed=0;

			if (!(dp->d_flags & DIR_COMB))
				continue;

			if (rt_db_get_internal(&intern, dp, wdbp->dbip, (fastf_t *)NULL, &rt_uniresource) < 0)
				continue;
			comb = (struct rt_comb_internal *)intern.idb_ptr;

			bu_ptbl_reset(&stack);
			/* visit each leaf in the combination */
			comb_leaf = comb->tree;
			if (comb_leaf) {
				while (!done) {
					while(comb_leaf->tr_op != OP_DB_LEAF) {
						bu_ptbl_ins(&stack, (long *)comb_leaf);
						comb_leaf = comb_leaf->tr_b.tb_left;
					}

					if (!strcmp(comb_leaf->tr_l.tl_name, argv[1])) {
						bu_free(comb_leaf->tr_l.tl_name, "comb_leaf->tr_l.tl_name");
						comb_leaf->tr_l.tl_name = bu_strdup(argv[2]);
						changed = 1;
					}

					if (BU_PTBL_END(&stack) < 1) {
						done = 1;
						break;
					}
					comb_leaf = (union tree *)BU_PTBL_GET(&stack, BU_PTBL_END(&stack)-1);
					if (comb_leaf->tr_op != OP_DB_LEAF) {
						bu_ptbl_rm( &stack, (long *)comb_leaf );
						comb_leaf = comb_leaf->tr_b.tb_right;
					}
				}
			}

			if (changed) {
				if (rt_db_put_internal(dp, wdbp->dbip, &intern, &rt_uniresource)) {
					bu_ptbl_free( &stack );
					rt_db_free_internal( &intern, &rt_uniresource );
					Tcl_AppendResult(interp,
							 "Database write error, aborting",
							 (char *)NULL);
					return TCL_ERROR;
				}
			}
			else
				rt_db_free_internal(&intern, &rt_uniresource);
		}
	}

	bu_ptbl_free(&stack);
	return TCL_OK;
}

/*
 * Rename all occurences of an object
 *
 * Usage:
 *        procname mvall from to
 */
static int
wdb_move_all_tcl(clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int     argc;
     char    **argv;
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_move_all_cmd(wdbp, interp, argc-1, argv+1);
}

static void
wdb_do_update(struct db_i		*dbip,
	      struct rt_comb_internal	*comb,
	      union tree		*comb_leaf,
	      genptr_t			user_ptr1,
	      genptr_t			user_ptr2,
	      genptr_t			user_ptr3)
{
	char	*mref;
	char	mref4[RT_NAMESIZE+2];
	struct bu_vls mref5;
	struct bu_vls *prestr;
	int	*ncharadd;

	if(dbip == DBI_NULL)
		return;

	RT_CK_DBI(dbip);
	RT_CK_TREE(comb_leaf);

	ncharadd = (int *)user_ptr1;
	prestr = (struct bu_vls *)user_ptr2;

	if( dbip->dbi_version < 5 ) {
		mref = mref4;
		(void)strncpy(mref, bu_vls_addr( prestr ), *ncharadd);
		(void)strncpy(mref+(*ncharadd),
			      comb_leaf->tr_l.tl_name,
			      RT_NAMESIZE-(*ncharadd) );
	} else {
		bu_vls_init( &mref5 );
		bu_vls_vlscat( &mref5, prestr );
		bu_vls_strcat( &mref5, comb_leaf->tr_l.tl_name );
		mref = bu_vls_addr( &mref5 );
	}
	bu_free(comb_leaf->tr_l.tl_name, "comb_leaf->tr_l.tl_name");
	comb_leaf->tr_l.tl_name = bu_strdup(mref);

	if( dbip->dbi_version >= 5 )
		bu_vls_free( &mref5 );
}

static int wdb_dir_add BU_ARGS((struct db_i *input_dbip, const char
	*name, long laddr, int len, int flags, genptr_t ptr));

struct dir_add_stuff {
	Tcl_Interp	*interp;
	struct db_i	*main_dbip;		/* the main database */
	struct rt_wdb	*wdbp;
};

/*
 *			W D B _ D I R _ A D D 5
 * V5 version of wdb_dir_add
 */
static void
wdb_dir_add5(struct db_i			*dbip,		/* db_i to add this object to */
	     const struct db5_raw_internal	*rip,
	     long				laddr,
	     genptr_t				client_data)
{
	struct bu_vls 		local5;
	char			*local;
	struct bu_external	ext;
	struct dir_add_stuff	*dasp = (struct dir_add_stuff *)client_data;
	struct db5_raw_internal	raw;
	struct directory	*dp;

	RT_CK_DBI( dbip );

	if( rip->h_dli == DB5HDR_HFLAGS_DLI_HEADER_OBJECT )  return;
	if( rip->h_dli == DB5HDR_HFLAGS_DLI_FREE_STORAGE ) {
		/* Record available free storage */
		rt_memfree( &(dbip->dbi_freep), rip->object_length, laddr );
		return;
	}
	
	/* If somehow it doesn't have a name, ignore it */
	if( rip->name.ext_buf == NULL )  return;

	if(RT_G_DEBUG&DEBUG_DB) {
		bu_log("wdb_dir_add5(dbip=x%x, name='%s', addr=x%x, len=%d)\n",
			dbip, (char *)rip->name.ext_buf, rip->object_length );
	}

	/* add to its own directory first */
	(void)db5_diradd( dbip, rip, laddr, NULL );

	/* do not concat GLOBAL object */
	if( rip->major_type == DB5_MAJORTYPE_ATTRIBUTE_ONLY &&
	    rip->minor_type == 0 )
		return;

	/* now add this object to the current database */
	bu_vls_init( &local5 );

	/* Add the prefix, if any */
	if( dasp->wdbp->wdb_ncharadd > 0 ) {
		bu_vls_vlscat( &local5, &dasp->wdbp->wdb_prestr );
		bu_vls_strcat( &local5, rip->name.ext_buf );
	} else {
		bu_vls_strcat( &local5, rip->name.ext_buf );
	}
	local = bu_vls_addr( &local5 );

	if( rip->minor_type != DB5_MINORTYPE_BRLCAD_COMBINATION ) {

		/* export object */
		db5_export_object3( &ext, rip->h_dli, local, 0, &rip->attributes,
			    &rip->body, rip->major_type, rip->minor_type,
			    rip->a_zzz, rip->b_zzz );

		if( db5_get_raw_internal_ptr( &raw, ext.ext_buf ) == NULL ) {
			Tcl_AppendResult(dasp->interp,
					 "db5_get_raw_internal_ptr() failed for ",
					 local,
					 ". This object ignored.\n",
					 (char *)NULL );
			bu_vls_free( &local5 );
			bu_free( ext.ext_buf, "ext.ext_buf" );
			return;
		}

		/* add to the main directory */
		dp = (struct directory *)db5_diradd( dasp->main_dbip, &raw, -1L, NULL );
		dp->d_len = 0;

		/* write to main database */
		if( db_put_external5( &ext, dp, dasp->main_dbip ) ) {
			Tcl_AppendResult(dasp->interp,
					 "db_put_external5() failed for ",
					 rip->name.ext_buf,
					 ". This object ignored.\n",
					 (char *)NULL );
			bu_vls_free( &local5 );
			bu_free( ext.ext_buf, "ext.ext_buf" );
			return;
		}

		Tcl_AppendResult(dasp->interp,
				 "Added ",
				 local,
				 "\n",
				 (char *)NULL );

		bu_vls_free( &local5 );
		bu_free( ext.ext_buf, "ext.ext_buf" );
	} else if( rip->major_type == DB5_MAJORTYPE_BRLCAD &&
		   rip->minor_type == DB5_MINORTYPE_BRLCAD_COMBINATION ) { 
		struct rt_db_internal in;
		struct rt_comb_internal *comb;

		/* export object */
		db5_export_object3( &ext, rip->h_dli, local, 0, &rip->attributes,
			    &rip->body, rip->major_type, rip->minor_type,
			    rip->a_zzz, rip->b_zzz );

		if( db5_get_raw_internal_ptr( &raw, ext.ext_buf ) == NULL ) {
			Tcl_AppendResult(dasp->interp,
					 "db5_get_raw_internal_ptr() failed for ",
					 local,
					 ". This object ignored.\n",
					 (char *)NULL );
			bu_vls_free( &local5 );
			bu_free( ext.ext_buf, "ext.ext_buf" );
			return;
		}

		RT_INIT_DB_INTERNAL( &in );
		if( rip->attributes.ext_nbytes > 0 ) {
			if( db5_import_attributes( &in.idb_avs, &rip->attributes ) < 0 )  {
				Tcl_AppendResult(dasp->interp,
						 "db5_import_attributes() failed for ",
						 local,
						 " (combination), this object will be missing attributes.\n",
						 (char *)NULL );
			}
		} else {
			in.idb_avs.magic = BU_AVS_MAGIC;
			in.idb_avs.count = 0;
			in.idb_avs.max = 0;
		}

		if( rt_comb_import5( &in, &rip->body, NULL, dasp->main_dbip, dasp->wdbp->wdb_resp, 0 ) ) {
			Tcl_AppendResult(dasp->interp,
					 "rt_comb_import5() Failed for ",
					 local,
					 ". Skipping this combination.\n",
					 (char *)NULL );
			bu_vls_free( &local5 );
			bu_free( ext.ext_buf, "ext.ext_buf" );
			return;
		}
		comb = (struct rt_comb_internal *)in.idb_ptr;
		RT_CHECK_COMB( comb );
		if (dasp->wdbp->wdb_ncharadd && comb->tree) {
			db_tree_funcleaf(dasp->main_dbip, comb, comb->tree, wdb_do_update,
			 (genptr_t)&(dasp->wdbp->wdb_ncharadd),
			 (genptr_t)&(dasp->wdbp->wdb_prestr), (genptr_t)NULL);
		}

		/* add to the main directory */
		dp = (struct directory *)db5_diradd( dasp->main_dbip, &raw, -1L, NULL );
		dp->d_len = 0;

		if (rt_db_put_internal(dp, dasp->main_dbip, &in, dasp->wdbp->wdb_resp ) < 0) {
			Tcl_AppendResult(dasp->interp,
				 "Failed writing ",
				 dp->d_namep, " to database\n", (char *)NULL);
			bu_vls_free( &local5 );
			bu_free( ext.ext_buf, "ext.ext_buf" );
			rt_db_free_internal( &in, dasp->wdbp->wdb_resp );
			return;
		}
		Tcl_AppendResult(dasp->interp,
				 "Added combination ",
				 local,
				 "\n",
				 (char *)NULL );
		bu_vls_free( &local5 );
		rt_db_free_internal( &in, dasp->wdbp->wdb_resp );
		bu_free( ext.ext_buf, "ext.ext_buf" );
	} else {
		Tcl_AppendResult(dasp->interp,
				 "Skipping non-BRLCAD object ",
				 local,
				 ", not yet supported\n",
				 (char *)NULL );
			bu_vls_free( &local5 );
			bu_free( ext.ext_buf, "ext.ext_buf" );
			return;
	}
}

/*
 *			W D B _ D I R _ A D D
 *
 *  Add a solid or conbination from an auxillary database
 *  into the primary database.
 */
static int
wdb_dir_add(register struct db_i	*input_dbip,
	    register const char		*name,
	    long			laddr,
	    int				len,
	    int				flags,
	    genptr_t			ptr)
{
	register struct directory *input_dp;
	register struct directory *dp;
	struct rt_db_internal intern;
	struct rt_comb_internal *comb;
	struct bu_vls		local5;
	char			*local;
	char			local4[RT_NAMESIZE+2+2];
	unsigned char		type='0';
	struct dir_add_stuff	*dasp = (struct dir_add_stuff *)ptr;

	RT_CK_DBI(input_dbip);

	/* Add the prefix, if any */
	if( dasp->main_dbip->dbi_version < 5 ) {
		local = local4;
		if (dasp->wdbp->wdb_ncharadd > 0) {
			(void)strncpy(local, bu_vls_addr( &dasp->wdbp->wdb_prestr ), dasp->wdbp->wdb_ncharadd);
			(void)strncpy(local+dasp->wdbp->wdb_ncharadd, name, RT_NAMESIZE-dasp->wdbp->wdb_ncharadd);
		} else {
			(void)strncpy(local, name, RT_NAMESIZE);
		}
		local[RT_NAMESIZE] = '\0';
	} else {
		bu_vls_init( &local5 );
		bu_vls_vlscat( &local5, &dasp->wdbp->wdb_prestr );
		bu_vls_strcat( &local5, name );
		local = bu_vls_addr( &local5 );
	}
		
	/* Look up this new name in the existing (main) database */
	if ((dp = db_lookup(dasp->main_dbip, local, LOOKUP_QUIET)) != DIR_NULL) {
		register int	c;
		char		*loc2;
		char		loc2_4[RT_NAMESIZE+2+2];

		/* This object already exists under the (prefixed) name */
		/* Protect the database against duplicate names! */
		/* Change object names, but NOT any references made by combinations. */
		if( dasp->main_dbip->dbi_version < 5 ) {
			loc2 = loc2_4;
			(void)strncpy( loc2, local, RT_NAMESIZE );
			/* Shift name right two characters, and further prefix */
			strncpy(local+2, loc2, RT_NAMESIZE-2);
			local[1] = '_';			/* distinctive separater */
			local[RT_NAMESIZE] = '\0';	/* ensure null termination */

			for (c = 'A'; c <= 'Z'; c++) {
				local[0] = c;
				if ((dp = db_lookup(dasp->main_dbip, local, LOOKUP_QUIET)) == DIR_NULL)
					break;
			}
		} else {
			for( c = 'A' ; c <= 'Z' ; c++ ) {
				bu_vls_trunc( &local5, 0 );
				bu_vls_putc( &local5, c );
				bu_vls_putc( &local5, '_' );
				bu_vls_strncat( &local5, bu_vls_addr( &dasp->wdbp->wdb_prestr ), dasp->wdbp->wdb_ncharadd);
				bu_vls_strcat( &local5, name );
				local = bu_vls_addr( &local5 );
				if ((dp = db_lookup(dasp->main_dbip, local, LOOKUP_QUIET)) == DIR_NULL)
					break;
			}
			loc2 = bu_vls_addr( &local5 ) + 2;
		}
		if (c > 'Z') {
			Tcl_AppendResult(dasp->interp,
					 "wdb_dir_add: Duplicate of name '",
					 local, "', ignored\n", (char *)NULL);
			if( dasp->main_dbip->dbi_version >= 5 )
				bu_vls_free( &local5 );
			return 0;
		}
		Tcl_AppendResult(dasp->interp,
				 "mged_dir_add: Duplicate of '",
				 loc2, "' given new name '",
				 local, "'\nYou should have used the 'dup' command to detect this,\nand then specified a prefix for the 'dbconcat' command.\n");
	}

	/* First, register this object in input database */
	/* use bogus type here (since we don't know it) */
	if ((input_dp = db_diradd(input_dbip, name, laddr, len, flags, (genptr_t)&type)) == DIR_NULL) {
		if( dasp->main_dbip->dbi_version >= 5 )
			bu_vls_free( &local5 );
		return(-1);
	}

	if (rt_db_get_internal(&intern, input_dp, input_dbip, (fastf_t *)NULL, &rt_uniresource) < 0) {
		Tcl_AppendResult(dasp->interp, "Database read error, aborting\n", (char *)NULL);
		if (db_delete(dasp->main_dbip, dp) < 0 ||
		    db_dirdelete(dasp->main_dbip, dp) < 0) {
			Tcl_AppendResult(dasp->interp, "Database write error, aborting\n", (char *)NULL);
		}
		if( dasp->main_dbip->dbi_version >= 5 )
			bu_vls_free( &local5 );
	    	/* Abort processing on first error */
		return -1;
	}

	/* Then, register a new object in the main database */
	if ((dp = db_diradd(dasp->main_dbip, local, -1L, 0, flags, (genptr_t)&intern.idb_type)) == DIR_NULL) {
		if( dasp->main_dbip->dbi_version >= 5 )
			bu_vls_free( &local5 );
		return(-1);
	}

	/* Update any references.  Name is already correct. */
	if (flags & DIR_SOLID) {
		Tcl_AppendResult(dasp->interp,
				 "adding solid '",
				 local, "'\n", (char *)NULL);
		if (dasp->main_dbip->dbi_version < 5 && (dasp->wdbp->wdb_ncharadd + strlen(name)) > (unsigned)RT_NAMESIZE)
			Tcl_AppendResult(dasp->interp,
					 "WARNING: solid name \"",
					 bu_vls_addr( &dasp->wdbp->wdb_prestr ), name,
					 "\" truncated to \"",
					 local, "\"\n", (char *)NULL);
	} else if(flags & DIR_COMB) {
		Tcl_AppendResult(dasp->interp,
				 "adding  comb '",
				 local, "'\n", (char *)NULL);

		/* Update all the member records */
		comb = (struct rt_comb_internal *)intern.idb_ptr;
		if (dasp->wdbp->wdb_ncharadd && comb->tree) {
			db_tree_funcleaf(dasp->main_dbip, comb, comb->tree, wdb_do_update,
			 (genptr_t)&(dasp->wdbp->wdb_ncharadd),
			 (genptr_t)&(dasp->wdbp->wdb_prestr), (genptr_t)NULL);
		}
	} else {
		Tcl_AppendResult(dasp->interp,
				 "WARNING: object name \"",
				 bu_vls_addr( &dasp->wdbp->wdb_prestr ), name,
				 "\" is of an unsupported type, not copied.\n",
				 (char *)NULL);
		return -1;
	}

	if (rt_db_put_internal(dp, dasp->main_dbip, &intern, &rt_uniresource) < 0) {
		Tcl_AppendResult(dasp->interp,
				 "Failed writing ",
				 dp->d_namep, " to database\n", (char *)NULL);
		if( dasp->main_dbip->dbi_version >= 5 )
			bu_vls_free( &local5 );
		return( -1 );
	}

	if( dasp->main_dbip->dbi_version >= 5 )
		bu_vls_free( &local5 );
	return 0;
}

int
wdb_concat_cmd(struct rt_wdb	*wdbp,
	       Tcl_Interp	*interp,
	       int		argc,
	       char 		**argv)
{
	struct db_i		*newdbp;
	int			bad = 0;
	struct dir_add_stuff	das;
	int			version;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc != 3) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_concat %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	bu_vls_trunc( &wdbp->wdb_prestr, 0 );
	if (strcmp(argv[2], "/") != 0) {
		(void)bu_vls_strcpy(&wdbp->wdb_prestr, argv[2]);
	}

	if( wdbp->dbip->dbi_version < 5 ) {
		if ((wdbp->wdb_ncharadd = bu_vls_strlen(&wdbp->wdb_prestr)) > 12) {
			wdbp->wdb_ncharadd = 12;
			bu_vls_trunc( &wdbp->wdb_prestr, 12 );
		}
	} else {
		wdbp->wdb_ncharadd = bu_vls_strlen(&wdbp->wdb_prestr);
	}

	/* open the input file */
	if ((newdbp = db_open(argv[1], "r")) == DBI_NULL) {
		perror(argv[1]);
		Tcl_AppendResult(interp, "concat: Can't open ",
				 argv[1], (char *)NULL);
		return TCL_ERROR;
	}

	/* Scan new database, adding everything encountered. */
	version = db_get_version( newdbp );
	if( version > 4 && wdbp->dbip->dbi_version < 5 ) {
		Tcl_AppendResult(interp, "concat: databases are incompatible, convert ",
				 wdbp->dbip->dbi_filename, " to version 5 first",
				 (char *)NULL );
		return TCL_ERROR;
	}

	das.interp = interp;
	das.main_dbip = wdbp->dbip;
	das.wdbp = wdbp;
	if (version < 5) {
		if (db_scan(newdbp, wdb_dir_add, 1, (genptr_t)&das) < 0) {
			Tcl_AppendResult(interp, "concat: db_scan failure", (char *)NULL);
			bad = 1;	
			/* Fall through, to close off database */
		}
	} else {
		if (db5_scan(newdbp, wdb_dir_add5, (genptr_t)&das) < 0) {
			Tcl_AppendResult(interp, "concat: db_scan failure", (char *)NULL);
			bad = 1;	
			/* Fall through, to close off database */
		}
	}
	rt_mempurge(&(newdbp->dbi_freep));        /* didn't really build a directory */

	/* Free all the directory entries, and close the input database */
	db_close(newdbp);

	db_sync(wdbp->dbip);	/* force changes to disk */

	return bad ? TCL_ERROR : TCL_OK;
}

/*
 *  Concatenate another GED file into the current file.
 *
 * Usage:
 *        procname concat file.g prefix
 */
static int
wdb_concat_tcl(ClientData	clientData,
	       Tcl_Interp	*interp,
	       int		argc,
	       char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_concat_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_copyeval_cmd(struct rt_wdb	*wdbp,
		 Tcl_Interp	*interp,
		 int		argc,
		 char 		**argv)
{
	struct directory	*dp;
	struct rt_db_internal	internal, new_int;
	mat_t			start_mat;
	int			id;
	int			i;
	int			endpos;
	struct wdb_trace_data	wtd;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc < 3 || 27 < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "help wdb_copyeval");
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	/* initialize wtd */
	wtd.wtd_interp = interp;
	wtd.wtd_dbip = wdbp->dbip;
	wtd.wtd_flag = WDB_CPEVAL;
	wtd.wtd_prflag = 0;

	/* check if new solid name already exists in description */
	if (db_lookup(wdbp->dbip, argv[1], LOOKUP_QUIET) != DIR_NULL) {
		Tcl_AppendResult(interp, argv[1], ": already exists\n", (char *)NULL);
		return TCL_ERROR;
	}

	MAT_IDN(start_mat);

	/* build directory pointer array for desired path */
	if (argc == 3 && strchr(argv[2], '/')) {
		char *tok;

		endpos = 0;

		tok = strtok(argv[2], "/");
		while (tok) {
			if ((wtd.wtd_obj[endpos++] = db_lookup(wdbp->dbip, tok, LOOKUP_NOISY)) == DIR_NULL)
				return TCL_ERROR;
			tok = strtok((char *)NULL, "/");
		}
	} else {
		for (i=2; i<argc; i++) {
			if ((wtd.wtd_obj[i-2] = db_lookup(wdbp->dbip, argv[i], LOOKUP_NOISY)) == DIR_NULL)
				return TCL_ERROR;
		}
		endpos = argc - 2;
	}

	wtd.wtd_objpos = endpos - 1;

	/* Make sure that final component in path is a solid */
	if ((id = rt_db_get_internal(&internal, wtd.wtd_obj[endpos - 1], wdbp->dbip, bn_mat_identity, &rt_uniresource)) < 0) {
		Tcl_AppendResult(interp, "import failure on ",
				 argv[argc-1], "\n", (char *)NULL);
		return TCL_ERROR;
	}

	if (id >= ID_COMBINATION) {
		rt_db_free_internal(&internal, &rt_uniresource);
		Tcl_AppendResult(interp, "final component on path must be a solid!!!\n", (char *)NULL );
		return TCL_ERROR;
	}

	wdb_trace(wtd.wtd_obj[0], 0, start_mat, &wtd);

	if (wtd.wtd_prflag == 0) {
		Tcl_AppendResult(interp, "PATH:  ", (char *)NULL);

		for (i=0; i<wtd.wtd_objpos; i++)
			Tcl_AppendResult(interp, "/", wtd.wtd_obj[i]->d_namep, (char *)NULL);

		Tcl_AppendResult(interp, "  NOT FOUND\n", (char *)NULL);
		rt_db_free_internal(&internal, &rt_uniresource);
		return TCL_ERROR;
	}

	/* Have found the desired path - wdb_xform is the transformation matrix */
	/* wdb_xform matrix calculated in wdb_trace() */

	/* create the new solid */
	RT_INIT_DB_INTERNAL(&new_int);
	if (rt_generic_xform(&new_int, wtd.wtd_xform,
			     &internal, 0, wdbp->dbip, &rt_uniresource)) {
		rt_db_free_internal(&internal, &rt_uniresource);
		Tcl_AppendResult(interp, "wdb_copyeval_cmd: rt_generic_xform failed\n", (char *)NULL);
		return TCL_ERROR;
	}

	if ((dp=db_diradd(wdbp->dbip, argv[1], -1L, 0,
			  wtd.wtd_obj[endpos-1]->d_flags,
			  (genptr_t)&new_int.idb_type)) == DIR_NULL) {
		rt_db_free_internal(&internal, &rt_uniresource);
		rt_db_free_internal(&new_int, &rt_uniresource);
		WDB_TCL_ALLOC_ERR_return;
	}

	if (rt_db_put_internal(dp, wdbp->dbip, &new_int, &rt_uniresource) < 0) {
		rt_db_free_internal(&internal, &rt_uniresource);
		rt_db_free_internal(&new_int, &rt_uniresource);
		WDB_TCL_WRITE_ERR_return;
	}
	rt_db_free_internal(&internal, &rt_uniresource);
	rt_db_free_internal(&new_int, &rt_uniresource);

	return TCL_OK;
}

/*
 *  
 *
 * Usage:
 *        procname copyeval new_solid path_to_solid
 */
static int
wdb_copyeval_tcl(ClientData	clientData,
		 Tcl_Interp	*interp,
		 int		argc,
		 char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_copyeval_cmd(wdbp, interp, argc-1, argv+1);
}

BU_EXTERN(int wdb_dir_check, ( struct
db_i *input_dbip, const char *name, long laddr, int len, int flags,
genptr_t ptr));

struct dir_check_stuff {
 	struct db_i	*main_dbip;
	struct rt_wdb	*wdbp;
	struct directory **dup_dirp;
};

BU_EXTERN(void wdb_dir_check5, ( struct db_i *input_dbip, const struct db5_raw_internal *rip, long addr, genptr_t ptr));

void
wdb_dir_check5(register struct db_i		*input_dbip,
	       const struct db5_raw_internal	*rip,
	       long				addr,
	       genptr_t				ptr)
{
	char			*name;
	struct directory	*dupdp;
	struct bu_vls		local;
	struct dir_check_stuff	*dcsp = (struct dir_check_stuff *)ptr;

	if (dcsp->main_dbip == DBI_NULL)
		return;

	RT_CK_DBI(input_dbip);
	RT_CK_RIP( rip );

	if( rip->h_dli == DB5HDR_HFLAGS_DLI_HEADER_OBJECT ) return;
	if( rip->h_dli == DB5HDR_HFLAGS_DLI_FREE_STORAGE ) return;

	name = (char *)rip->name.ext_buf;

	if( name == (char *)NULL ) return;

	/* do not compare _GLOBAL */
	if( rip->major_type == DB5_MAJORTYPE_ATTRIBUTE_ONLY &&
	    rip->minor_type == 0 )
		return;

	/* Add the prefix, if any */
	bu_vls_init( &local );
	if( dcsp->main_dbip->dbi_version < 5 ) {
		if (dcsp->wdbp->wdb_ncharadd > 0) {
			bu_vls_strncpy( &local, bu_vls_addr( &dcsp->wdbp->wdb_prestr ), dcsp->wdbp->wdb_ncharadd );
			bu_vls_strcat( &local, name );
		} else {
			bu_vls_strncpy( &local, name, RT_NAMESIZE );
		}
		bu_vls_trunc( &local, RT_NAMESIZE );
	} else {
		if (dcsp->wdbp->wdb_ncharadd > 0) {
			(void)bu_vls_vlscat( &local, &dcsp->wdbp->wdb_prestr );
			(void)bu_vls_strcat( &local, name );
		} else {
			(void)bu_vls_strcat( &local, name );
		}
	}
		
	/* Look up this new name in the existing (main) database */
	if ((dupdp = db_lookup(dcsp->main_dbip, bu_vls_addr( &local ), LOOKUP_QUIET)) != DIR_NULL) {
		/* Duplicate found, add it to the list */
		dcsp->wdbp->wdb_num_dups++;
		*dcsp->dup_dirp++ = dupdp;
	}
	return;
}

/*
 *			W D B _ D I R _ C H E C K
 *
 * Check a name against the global directory.
 */
int
wdb_dir_check(input_dbip, name, laddr, len, flags, ptr)
     register struct db_i	*input_dbip;
     register const char	*name;
     long			laddr;
     int			len;
     int			flags;
     genptr_t			ptr;
{
	struct directory	*dupdp;
	struct bu_vls		local;
	struct dir_check_stuff	*dcsp = (struct dir_check_stuff *)ptr;

	if (dcsp->main_dbip == DBI_NULL)
		return 0;

	RT_CK_DBI(input_dbip);

	/* Add the prefix, if any */
	bu_vls_init( &local );
	if( dcsp->main_dbip->dbi_version < 5 ) {
		if (dcsp->wdbp->wdb_ncharadd > 0) {
			bu_vls_strncpy( &local, bu_vls_addr( &dcsp->wdbp->wdb_prestr ), dcsp->wdbp->wdb_ncharadd );
			bu_vls_strcat( &local, name );
		} else {
			bu_vls_strncpy( &local, name, RT_NAMESIZE );
		}
		bu_vls_trunc( &local, RT_NAMESIZE );
	} else {
		if (dcsp->wdbp->wdb_ncharadd > 0) {
			bu_vls_vlscat( &local, &dcsp->wdbp->wdb_prestr );
			bu_vls_strcat( &local, name );
		} else {
			bu_vls_strcat( &local, name );
		}
	}
		
	/* Look up this new name in the existing (main) database */
	if ((dupdp = db_lookup(dcsp->main_dbip, bu_vls_addr( &local ), LOOKUP_QUIET)) != DIR_NULL) {
		/* Duplicate found, add it to the list */
		dcsp->wdbp->wdb_num_dups++;
		*dcsp->dup_dirp++ = dupdp;
	}
	bu_vls_free( &local );
	return 0;
}

int
wdb_dup_cmd(struct rt_wdb	*wdbp,
	    Tcl_Interp		*interp,
	    int			argc,
	    char		**argv)
{
	struct db_i		*newdbp = DBI_NULL;
	struct directory	**dirp0 = (struct directory **)NULL;
	struct bu_vls vls;
	struct dir_check_stuff	dcs;

	if (argc < 2 || 3 < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_dup %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	bu_vls_trunc( &wdbp->wdb_prestr, 0 );
	if (argc == 3)
		(void)bu_vls_strcpy(&wdbp->wdb_prestr, argv[2]);

	wdbp->wdb_num_dups = 0;
	if( wdbp->dbip->dbi_version < 5 ) {
		if ((wdbp->wdb_ncharadd = bu_vls_strlen(&wdbp->wdb_prestr)) > 12) {
			wdbp->wdb_ncharadd = 12;
			bu_vls_trunc( &wdbp->wdb_prestr, 12 );
		}
	} else {
		wdbp->wdb_ncharadd = bu_vls_strlen(&wdbp->wdb_prestr);
	}

	/* open the input file */
	if ((newdbp = db_open(argv[1], "r")) == DBI_NULL) {
		perror(argv[1]);
		Tcl_AppendResult(interp, "dup: Can't open ", argv[1], (char *)NULL);
		return TCL_ERROR;
	}

	Tcl_AppendResult(interp, "\n*** Comparing ",
			wdbp->dbip->dbi_filename,
			 "  with ", argv[1], " for duplicate names\n", (char *)NULL);
	if (wdbp->wdb_ncharadd) {
		Tcl_AppendResult(interp, "  For comparison, all names in ",
				 argv[1], " were prefixed with:  ",
				 bu_vls_addr( &wdbp->wdb_prestr ), "\n", (char *)NULL);
	}

	/* Get array to hold names of duplicates */
	if ((dirp0 = wdb_getspace(wdbp->dbip, 0)) == (struct directory **) 0) {
		Tcl_AppendResult(interp, "f_dup: unable to get memory\n", (char *)NULL);
		db_close( newdbp );
		return TCL_ERROR;
	}

	/* Scan new database for overlaps */
	dcs.main_dbip = wdbp->dbip;
	dcs.wdbp = wdbp;
	dcs.dup_dirp = dirp0;
	if( newdbp->dbi_version < 5 ) {
		if (db_scan(newdbp, wdb_dir_check, 0, (genptr_t)&dcs) < 0) {
			Tcl_AppendResult(interp, "dup: db_scan failure", (char *)NULL);
			bu_free((genptr_t)dirp0, "wdb_getspace array");
			db_close(newdbp);
			return TCL_ERROR;
		}
	} else {
		if( db5_scan( newdbp, wdb_dir_check5, (genptr_t)&dcs) < 0) {
			Tcl_AppendResult(interp, "dup: db_scan failure", (char *)NULL);
			bu_free((genptr_t)dirp0, "wdb_getspace array");
			db_close(newdbp);
			return TCL_ERROR;
		}
	}
	rt_mempurge( &(newdbp->dbi_freep) );        /* didn't really build a directory */

	bu_vls_init(&vls);
	wdb_vls_col_pr4v(&vls, dirp0, (int)(dcs.dup_dirp - dirp0));
	bu_vls_printf(&vls, "\n -----  %d duplicate names found  -----", wdbp->wdb_num_dups);
	Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
	bu_vls_free(&vls);
	bu_free((genptr_t)dirp0, "wdb_getspace array");
	db_close(newdbp);

	return TCL_OK;
}

/*
 * Usage:
 *        procname dup file.g [prefix]
 */
static int
wdb_dup_tcl(clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int     argc;
     char    **argv;
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_dup_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_group_cmd(struct rt_wdb	*wdbp,
	      Tcl_Interp	*interp,
	      int		argc,
	      char 		**argv)
{
	register struct directory *dp;
	register int i;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc < 3 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_group %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	/* get objects to add to group */
	for (i = 2; i < argc; i++) {
		if ((dp = db_lookup(wdbp->dbip, argv[i], LOOKUP_NOISY)) != DIR_NULL) {
			if (wdb_combadd(interp, wdbp->dbip, dp, argv[1], 0,
					WMOP_UNION, 0, 0, wdbp) == DIR_NULL)
				return TCL_ERROR;
		}  else
			Tcl_AppendResult(interp, "skip member ", argv[i], "\n", (char *)NULL);
	}
	return TCL_OK;
}

/*
 * Usage:
 *        procname g groupname object1 object2 .... objectn
 */
static int
wdb_group_tcl(ClientData	clientData,
	      Tcl_Interp	*interp,
	      int		argc,
	      char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_group_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_remove_cmd(struct rt_wdb	*wdbp,
	       Tcl_Interp	*interp,
	       int		argc,
	       char 		**argv)
{
	register struct directory	*dp;
	register int			i;
	int				num_deleted;
	struct rt_db_internal		intern;
	struct rt_comb_internal		*comb;
	int				ret;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc < 3 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_remove %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	if ((dp = db_lookup(wdbp->dbip,  argv[1], LOOKUP_NOISY)) == DIR_NULL)
		return TCL_ERROR;

	if ((dp->d_flags & DIR_COMB) == 0) {
		Tcl_AppendResult(interp, "rm: ", dp->d_namep,
				 " is not a combination", (char *)NULL );
		return TCL_ERROR;
	}

	if (rt_db_get_internal(&intern, dp, wdbp->dbip, (fastf_t *)NULL, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "Database read error, aborting", (char *)NULL);
		return TCL_ERROR;
	}

	comb = (struct rt_comb_internal *)intern.idb_ptr;
	RT_CK_COMB(comb);

	/* Process each argument */
	num_deleted = 0;
	ret = TCL_OK;
	for (i = 2; i < argc; i++) {
		if (db_tree_del_dbleaf( &(comb->tree), argv[i], &rt_uniresource ) < 0) {
			Tcl_AppendResult(interp, "  ERROR_deleting ",
					 dp->d_namep, "/", argv[i],
					 "\n", (char *)NULL);
			ret = TCL_ERROR;
		} else {
			Tcl_AppendResult(interp, "deleted ",
					 dp->d_namep, "/", argv[i],
					 "\n", (char *)NULL);
			num_deleted++;
		}
	}

	if (rt_db_put_internal(dp, wdbp->dbip, &intern, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "Database write error, aborting", (char *)NULL);
		return TCL_ERROR;
	}

	return ret;
}

/*
 * Remove members from a combination.
 *
 * Usage:
 *        procname remove comb object(s)
 */
static int
wdb_remove_tcl(clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int     argc;
     char    **argv;
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_remove_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_region_cmd(struct rt_wdb	*wdbp,
	       Tcl_Interp	*interp,
	       int		argc,
	       char 		**argv)
{
	register struct directory	*dp;
	int				i;
	int				ident, air;
	char				oper;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc < 4 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_region %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

 	ident = wdbp->wdb_item_default;
 	air = wdbp->wdb_air_default;

	/* Check for even number of arguments */
	if (argc & 01) {
		Tcl_AppendResult(interp, "error in number of args!", (char *)NULL);
		return TCL_ERROR;
	}

	if (db_lookup(wdbp->dbip, argv[1], LOOKUP_QUIET) == DIR_NULL) {
		/* will attempt to create the region */
		if (wdbp->wdb_item_default) {
			struct bu_vls tmp_vls;

			wdbp->wdb_item_default++;
			bu_vls_init(&tmp_vls);
			bu_vls_printf(&tmp_vls, "Defaulting item number to %d\n",
				wdbp->wdb_item_default);
			Tcl_AppendResult(interp, bu_vls_addr(&tmp_vls), (char *)NULL);
			bu_vls_free(&tmp_vls);
		}
	}

	/* Get operation and solid name for each solid */
	for (i = 2; i < argc; i += 2) {
		if (argv[i][1] != '\0') {
			Tcl_AppendResult(interp, "bad operation: ", argv[i],
					 " skip member: ", argv[i+1], "\n", (char *)NULL);
			continue;
		}
		oper = argv[i][0];
		if ((dp = db_lookup(wdbp->dbip,  argv[i+1], LOOKUP_NOISY )) == DIR_NULL) {
			Tcl_AppendResult(interp, "skipping ", argv[i+1], "\n", (char *)NULL);
			continue;
		}

		if (oper != WMOP_UNION && oper != WMOP_SUBTRACT && oper != WMOP_INTERSECT) {
			struct bu_vls tmp_vls;

			bu_vls_init(&tmp_vls);
			bu_vls_printf(&tmp_vls, "bad operation: %c skip member: %s\n",
				      oper, dp->d_namep );
			Tcl_AppendResult(interp, bu_vls_addr(&tmp_vls), (char *)NULL);
			bu_vls_free(&tmp_vls);
			continue;
		}

		/* Adding region to region */
		if (dp->d_flags & DIR_REGION) {
			Tcl_AppendResult(interp, "Note: ", dp->d_namep,
					 " is a region\n", (char *)NULL);
		}

		if (wdb_combadd(interp, wdbp->dbip, dp,
				argv[1], 1, oper, ident, air, wdbp) == DIR_NULL) {
			Tcl_AppendResult(interp, "error in combadd", (char *)NULL);
			return TCL_ERROR;
		}
	}

	if (db_lookup(wdbp->dbip, argv[1], LOOKUP_QUIET) == DIR_NULL) {
		/* failed to create region */
		if (wdbp->wdb_item_default > 1)
			wdbp->wdb_item_default--;
		return TCL_ERROR;
	}

	return TCL_OK;
}

/*
 * Usage:
 *        procname r rname object(s)
 */
static int
wdb_region_tcl(clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int     argc;
     char    **argv;
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_region_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_comb_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char 		**argv)
{
	register struct directory *dp;
	char	*comb_name;
	register int	i;
	char	oper;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc < 4 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_comb %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	/* Check for odd number of arguments */
	if (argc & 01) {
		Tcl_AppendResult(interp, "error in number of args!", (char *)NULL);
		return TCL_ERROR;
	}

	/* Save combination name, for use inside loop */
	comb_name = argv[1];
	if ((dp=db_lookup(wdbp->dbip, comb_name, LOOKUP_QUIET)) != DIR_NULL) {
		if (!(dp->d_flags & DIR_COMB)) {
			Tcl_AppendResult(interp,
					 "ERROR: ", comb_name,
					 " is not a combination", (char *)0 );
			return TCL_ERROR;
		}
	}

	/* Get operation and solid name for each solid */
	for (i = 2; i < argc; i += 2) {
		if (argv[i][1] != '\0') {
			Tcl_AppendResult(interp, "bad operation: ", argv[i],
					 " skip member: ", argv[i+1], "\n", (char *)NULL);
			continue;
		}
		oper = argv[i][0];
		if ((dp = db_lookup(wdbp->dbip,  argv[i+1], LOOKUP_NOISY)) == DIR_NULL) {
			Tcl_AppendResult(interp, "skipping ", argv[i+1], "\n", (char *)NULL);
			continue;
		}

		if (oper != WMOP_UNION && oper != WMOP_SUBTRACT && oper != WMOP_INTERSECT) {
			struct bu_vls tmp_vls;

			bu_vls_init(&tmp_vls);
			bu_vls_printf(&tmp_vls, "bad operation: %c skip member: %s\n",
				      oper, dp->d_namep);
			Tcl_AppendResult(interp, bu_vls_addr(&tmp_vls), (char *)NULL);
			continue;
		}

		if (wdb_combadd(interp, wdbp->dbip, dp, comb_name, 0, oper, 0, 0, wdbp) == DIR_NULL) {
			Tcl_AppendResult(interp, "error in combadd", (char *)NULL);
			return TCL_ERROR;
		}
	}

	if (db_lookup(wdbp->dbip, comb_name, LOOKUP_QUIET) == DIR_NULL) {
		Tcl_AppendResult(interp, "Error:  ", comb_name,
				 " not created", (char *)NULL);
		return TCL_ERROR;
	}

	return TCL_OK;
}

/*
 * Create or add to the end of a combination, with one or more solids,
 * with explicitly specified operations.
 *
 * Usage:
 *        procname comb comb_name opr1 sol1 opr2 sol2 ... oprN solN
 */
static int
wdb_comb_tcl(clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int     argc;
     char    **argv;
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_comb_cmd(wdbp, interp, argc-1, argv+1);
}

static void
wdb_find_ref(struct db_i		*dbip,
	     struct rt_comb_internal	*comb,
	     union tree			*comb_leaf,
	     genptr_t			object,
	     genptr_t			comb_name_ptr,
	     genptr_t			user_ptr3)
{
	char *obj_name;
	char *comb_name;
	Tcl_Interp *interp = (Tcl_Interp *)user_ptr3;

	RT_CK_TREE(comb_leaf);

	obj_name = (char *)object;
	if (strcmp(comb_leaf->tr_l.tl_name, obj_name))
		return;

	comb_name = (char *)comb_name_ptr;

	Tcl_AppendElement(interp, comb_name);
}

int
wdb_find_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char 		**argv)
{
	register int				i,k;
	register struct directory		*dp;
	struct rt_db_internal			intern;
	register struct rt_comb_internal	*comb=(struct rt_comb_internal *)NULL;

	if (argc < 2 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_find %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	/* Examine all COMB nodes */
	for (i = 0; i < RT_DBNHASH; i++) {
		for (dp = wdbp->dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw) {
			if (!(dp->d_flags & DIR_COMB))
				continue;

			if (rt_db_get_internal(&intern, dp, wdbp->dbip, (fastf_t *)NULL, &rt_uniresource) < 0) {
				Tcl_AppendResult(interp, "Database read error, aborting", (char *)NULL);
				return TCL_ERROR;
			}

			comb = (struct rt_comb_internal *)intern.idb_ptr;
			for (k=1; k<argc; k++)
				db_tree_funcleaf(wdbp->dbip, comb, comb->tree, wdb_find_ref, (genptr_t)argv[k], (genptr_t)dp->d_namep, (genptr_t)interp);

			rt_db_free_internal(&intern, &rt_uniresource);
		}
	}

	return TCL_OK;
}

/*
 * Usage:
 *        procname find object(s)
 */
static int
wdb_find_tcl(clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int     argc;
     char    **argv;
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_find_cmd(wdbp, interp, argc-1, argv+1);
}

struct wdb_id_names {
	struct bu_list l;
	struct bu_vls name;		/* name associated with region id */
};

struct wdb_id_to_names {
	struct bu_list l;
	int id;				/* starting id (i.e. region id or air code) */
	struct wdb_id_names headName;	/* head of list of names */
};

int
wdb_which_cmd(struct rt_wdb	*wdbp,
	      Tcl_Interp	*interp,
	      int		argc,
	      char 		**argv)
{
	register int	i,j;
	register struct directory *dp;
	struct rt_db_internal intern;
	struct rt_comb_internal *comb;
	struct wdb_id_to_names headIdName;
	struct wdb_id_to_names *itnp;
	struct wdb_id_names *inp;
	int isAir;
	int sflag;

	if (argc < 2 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_%s %s", argv[0], argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	if (!strcmp(argv[0], "whichair"))
		isAir = 1;
	else
		isAir = 0;

	if (strcmp(argv[1], "-s") == 0) {
		--argc;
		++argv;

		if (argc < 2) {
			struct bu_vls vls;

			bu_vls_init(&vls);
			bu_vls_printf(&vls, "helplib_alias wdb_%s %s", argv[-1], argv[-1]);
			Tcl_Eval(interp, bu_vls_addr(&vls));
			bu_vls_free(&vls);
			return TCL_ERROR;
		}

		sflag = 1;
	} else {
		sflag = 0;
	}

	BU_LIST_INIT(&headIdName.l);

	/* Build list of id_to_names */
	for (j=1; j<argc; j++) {
		int n;
		int start, end;
		int range;
		int k;

		n = sscanf(argv[j], "%d%*[:-]%d", &start, &end);
		switch (n) {
		case 1:
			for (BU_LIST_FOR(itnp,wdb_id_to_names,&headIdName.l))
				if (itnp->id == start)
					break;

			/* id not found */
			if (BU_LIST_IS_HEAD(itnp,&headIdName.l)) {
				BU_GETSTRUCT(itnp,wdb_id_to_names);
				itnp->id = start;
				BU_LIST_INSERT(&headIdName.l,&itnp->l);
				BU_LIST_INIT(&itnp->headName.l);
			}

			break;
		case 2:
			if (start < end)
				range = end - start + 1;
			else if (end < start) {
				range = start - end + 1;
				start = end;
			} else
				range = 1;

			for (k = 0; k < range; ++k) {
				int id = start + k;

				for (BU_LIST_FOR(itnp,wdb_id_to_names,&headIdName.l))
					if (itnp->id == id)
						break;

				/* id not found */
				if (BU_LIST_IS_HEAD(itnp,&headIdName.l)) {
					BU_GETSTRUCT(itnp,wdb_id_to_names);
					itnp->id = id;
					BU_LIST_INSERT(&headIdName.l,&itnp->l);
					BU_LIST_INIT(&itnp->headName.l);
				}
			}

			break;
		}
	}

	/* Examine all COMB nodes */
	for (i = 0; i < RT_DBNHASH; i++) {
		for (dp = wdbp->dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw) {
			if (!(dp->d_flags & DIR_REGION))
				continue;

			if (rt_db_get_internal( &intern, dp, wdbp->dbip, (fastf_t *)NULL, &rt_uniresource ) < 0) {
				Tcl_AppendResult(interp, "Database read error, aborting", (char *)NULL);
				return TCL_ERROR;
			}
			comb = (struct rt_comb_internal *)intern.idb_ptr;
			if (comb->region_id != 0 && comb->aircode != 0) {
				Tcl_AppendResult(interp, "ERROR: ", dp->d_namep,
						 " has id and aircode!!!\n", (char *)NULL);
				continue;
			}

			/* check to see if the region id or air code matches one in our list */
			for (BU_LIST_FOR(itnp,wdb_id_to_names,&headIdName.l)) {
				if ((!isAir && comb->region_id == itnp->id) ||
				    (isAir && comb->aircode == itnp->id)) {
					/* add region name to our name list for this region */
					BU_GETSTRUCT(inp,wdb_id_names);
					bu_vls_init(&inp->name);
					bu_vls_strcpy(&inp->name, dp->d_namep);
					BU_LIST_INSERT(&itnp->headName.l,&inp->l);
					break;
				}
			}

			rt_comb_ifree( &intern, &rt_uniresource );
		}
	}

	/* place data in interp and free memory */
	 while (BU_LIST_WHILE(itnp,wdb_id_to_names,&headIdName.l)) {
		if (!sflag) {
			struct bu_vls vls;

			bu_vls_init(&vls);
			bu_vls_printf(&vls, "Region[s] with %s %d:\n",
				      isAir ? "air code" : "ident", itnp->id);
			Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
			bu_vls_free(&vls);
		}

		while (BU_LIST_WHILE(inp,wdb_id_names,&itnp->headName.l)) {
			if (sflag)
				Tcl_AppendElement(interp, bu_vls_addr(&inp->name));
			else
				Tcl_AppendResult(interp, "   ", bu_vls_addr(&inp->name),
						 "\n", (char *)NULL);

			BU_LIST_DEQUEUE(&inp->l);
			bu_vls_free(&inp->name);
			bu_free((genptr_t)inp, "which: inp");
		}

		BU_LIST_DEQUEUE(&itnp->l);
		bu_free((genptr_t)itnp, "which: itnp");
	}

	return TCL_OK;
}

/*
 * Usage:
 *        procname whichair/whichid [-s] id(s)
 */
static int
wdb_which_tcl(clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int     argc;
     char    **argv;
{
	struct rt_wdb	*wdbp = (struct rt_wdb *)clientData;

	return wdb_which_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_title_cmd(struct rt_wdb	*wdbp,
	      Tcl_Interp	*interp,
	      int		argc,
	      char 		**argv)
{
	struct bu_vls	title;
	int		bad = 0;

	RT_CK_WDB(wdbp);
	RT_CK_DBI(wdbp->dbip);

	if (argc < 1 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_title %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	/* get title */
	if (argc == 1) {
		Tcl_AppendResult(interp, wdbp->dbip->dbi_title, (char *)NULL);
		return TCL_OK;
	}

	WDB_TCL_CHECK_READ_ONLY;

	/* set title */
	bu_vls_init(&title);
	bu_vls_from_argv(&title, argc-1, argv+1);

	if (db_update_ident(wdbp->dbip, bu_vls_addr(&title), wdbp->dbip->dbi_base2local) < 0) {
		Tcl_AppendResult(interp, "Error: unable to change database title");
		bad = 1;
	}

	bu_vls_free(&title);
	return bad ? TCL_ERROR : TCL_OK;
}

/*
 * Change or return the database title.
 *
 * Usage:
 *        procname title [description]
 */
static int
wdb_title_tcl(ClientData	clientData,
	      Tcl_Interp	*interp,
	      int		argc,
	      char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_title_cmd(wdbp, interp, argc-1, argv+1);
}

static int
wdb_list_children(struct rt_wdb		*wdbp,
		  Tcl_Interp		*interp,
		  register struct directory *dp)
{
	register int			i;
	struct rt_db_internal		intern;
	struct rt_comb_internal		*comb;

	if (!(dp->d_flags & DIR_COMB))
		return TCL_OK;

	if (rt_db_get_internal(&intern, dp, wdbp->dbip, (fastf_t *)NULL, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "Database read error, aborting", (char *)NULL);
		return TCL_ERROR;
	}
	comb = (struct rt_comb_internal *)intern.idb_ptr;

	if (comb->tree) {
		struct bu_vls vls;
		int node_count;
		int actual_count;
		struct rt_tree_array *rt_tree_array;

		if (comb->tree && db_ck_v4gift_tree(comb->tree) < 0) {
			db_non_union_push(comb->tree, &rt_uniresource);
			if (db_ck_v4gift_tree(comb->tree) < 0) {
				Tcl_AppendResult(interp, "Cannot flatten tree for listing", (char *)NULL);
				return TCL_ERROR;
			}
		}
		node_count = db_tree_nleaves(comb->tree);
		if (node_count > 0) {
			rt_tree_array = (struct rt_tree_array *)bu_calloc( node_count,
									   sizeof( struct rt_tree_array ), "tree list" );
			actual_count = (struct rt_tree_array *)db_flatten_tree(
				rt_tree_array, comb->tree, OP_UNION,
				1, &rt_uniresource ) - rt_tree_array;
			BU_ASSERT_PTR( actual_count, ==, node_count );
			comb->tree = TREE_NULL;
		} else {
			actual_count = 0;
			rt_tree_array = NULL;
		}

		bu_vls_init(&vls);
		for (i=0 ; i<actual_count ; i++) {
			char op;

			switch (rt_tree_array[i].tl_op) {
			case OP_UNION:
				op = 'u';
				break;
			case OP_INTERSECT:
				op = '+';
				break;
			case OP_SUBTRACT:
				op = '-';
				break;
			default:
				op = '?';
				break;
			}

			bu_vls_printf(&vls, "{%c %s} ", op, rt_tree_array[i].tl_tree->tr_l.tl_name);
			db_free_tree( rt_tree_array[i].tl_tree, &rt_uniresource );
		}
		Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)0);
		bu_vls_free(&vls);

		if (rt_tree_array)
			bu_free((char *)rt_tree_array, "printnode: rt_tree_array");
	}
	rt_db_free_internal(&intern, &rt_uniresource);

	return TCL_OK;
}

int
wdb_lt_cmd(struct rt_wdb	*wdbp,
	   Tcl_Interp		*interp,
	   int			argc,
	   char 		**argv)
{
	register struct directory	*dp;
	struct bu_vls			vls;

	if (argc != 2)
		goto bad;

	if ((dp = db_lookup(wdbp->dbip, argv[1], LOOKUP_NOISY)) == DIR_NULL)
		goto bad;

	return wdb_list_children(wdbp, interp, dp);

 bad:
	bu_vls_init(&vls);
	bu_vls_printf(&vls, "helplib_alias wdb_lt %s", argv[0]);
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
}

/*
 * Usage:
 *        procname lt object
 */
static int
wdb_lt_tcl(ClientData	clientData,
	   Tcl_Interp	*interp,
	   int     	argc,
	   char    	**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_lt_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_version_cmd(struct rt_wdb	*wdbp,
		Tcl_Interp	*interp,
		int		argc,
		char 		**argv)
{
	struct bu_vls	vls;

	bu_vls_init(&vls);

	if (argc != 1) {
		bu_vls_printf(&vls, "helplib_alias wdb_version %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	bu_vls_printf(&vls, "%d", wdbp->dbip->dbi_version);
	Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)0);
	bu_vls_free(&vls);

	return TCL_OK;
}

/*
 * Usage:
 *        procname version
 */
static int
wdb_version_tcl(ClientData	clientData,
		Tcl_Interp	*interp,
		int     	argc,
		char    	**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_version_cmd(wdbp, interp, argc-1, argv+1);
}

/*
 *			W D B _ P R I N T _ N O D E
 *
 *  NON-PARALLEL due to rt_uniresource
 */
static void
wdb_print_node(struct rt_wdb		*wdbp,
	       Tcl_Interp		*interp,
	       register struct directory *dp,
	       int			pathpos,
	       char			prefix,
	       int			cflag)
{	
	register int			i;
	register struct directory	*nextdp;
	struct rt_db_internal		intern;
	struct rt_comb_internal		*comb;

	if (cflag && !(dp->d_flags & DIR_COMB))
		return;

	for (i=0; i<pathpos; i++) 
		Tcl_AppendResult(interp, "\t", (char *)NULL);

	if (prefix) {
		struct bu_vls tmp_vls;

		bu_vls_init(&tmp_vls);
		bu_vls_printf(&tmp_vls, "%c ", prefix);
		Tcl_AppendResult(interp, bu_vls_addr(&tmp_vls), (char *)NULL);
		bu_vls_free(&tmp_vls);
	}

	Tcl_AppendResult(interp, dp->d_namep, (char *)NULL);
	/* Output Comb and Region flags (-F?) */
	if(dp->d_flags & DIR_COMB)
		Tcl_AppendResult(interp, "/", (char *)NULL);
	if(dp->d_flags & DIR_REGION)
		Tcl_AppendResult(interp, "R", (char *)NULL);

	Tcl_AppendResult(interp, "\n", (char *)NULL);

	if(!(dp->d_flags & DIR_COMB))
		return;

	/*
	 *  This node is a combination (eg, a directory).
	 *  Process all the arcs (eg, directory members).
	 */

	if (rt_db_get_internal(&intern, dp, wdbp->dbip, (fastf_t *)NULL, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "Database read error, aborting", (char *)NULL);
		return;
	}
	comb = (struct rt_comb_internal *)intern.idb_ptr;

	if (comb->tree) {
		int node_count;
		int actual_count;
		struct rt_tree_array *rt_tree_array;

		if (comb->tree && db_ck_v4gift_tree(comb->tree) < 0) {
			db_non_union_push(comb->tree, &rt_uniresource);
			if (db_ck_v4gift_tree(comb->tree) < 0) {
				Tcl_AppendResult(interp, "Cannot flatten tree for listing", (char *)NULL);
				return;
			}
		}
		node_count = db_tree_nleaves(comb->tree);
		if (node_count > 0) {
			rt_tree_array = (struct rt_tree_array *)bu_calloc( node_count,
									   sizeof( struct rt_tree_array ), "tree list" );
			actual_count = (struct rt_tree_array *)db_flatten_tree(
				rt_tree_array, comb->tree, OP_UNION,
				1, &rt_uniresource ) - rt_tree_array;
			BU_ASSERT_PTR( actual_count, ==, node_count );
			comb->tree = TREE_NULL;
		} else {
			actual_count = 0;
			rt_tree_array = NULL;
		}

		for (i=0 ; i<actual_count ; i++) {
			char op;

			switch (rt_tree_array[i].tl_op) {
			case OP_UNION:
				op = 'u';
				break;
			case OP_INTERSECT:
				op = '+';
				break;
			case OP_SUBTRACT:
				op = '-';
				break;
			default:
				op = '?';
				break;
			}

			if ((nextdp = db_lookup(wdbp->dbip, rt_tree_array[i].tl_tree->tr_l.tl_name, LOOKUP_NOISY)) == DIR_NULL) {
				int j;
				struct bu_vls tmp_vls;
  			
				for (j=0; j<pathpos+1; j++) 
					Tcl_AppendResult(interp, "\t", (char *)NULL);

				bu_vls_init(&tmp_vls);
				bu_vls_printf(&tmp_vls, "%c ", op);
				Tcl_AppendResult(interp, bu_vls_addr(&tmp_vls), (char *)NULL);
				bu_vls_free(&tmp_vls);

				Tcl_AppendResult(interp, rt_tree_array[i].tl_tree->tr_l.tl_name, "\n", (char *)NULL);
			} else
				wdb_print_node(wdbp, interp, nextdp, pathpos+1, op, cflag);
			db_free_tree( rt_tree_array[i].tl_tree, &rt_uniresource );
		}
		if(rt_tree_array) bu_free((char *)rt_tree_array, "printnode: rt_tree_array");
	}
	rt_db_free_internal(&intern, &rt_uniresource);
}

int
wdb_tree_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char 		**argv)
{
	register struct directory	*dp;
	register int			j;
	int				cflag = 0;

	if (argc < 2 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_tree %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	if (argv[1][0] == '-' && argv[1][1] == 'c') {
		cflag = 1;
		--argc;
		++argv;
	}

	for (j = 1; j < argc; j++) {
		if (j > 1)
			Tcl_AppendResult(interp, "\n", (char *)NULL);
		if ((dp = db_lookup(wdbp->dbip, argv[j], LOOKUP_NOISY)) == DIR_NULL)
			continue;
		wdb_print_node(wdbp, interp, dp, 0, 0, cflag);
	}

	return TCL_OK;
}

/*
 * Usage:
 *        procname tree object(s)
 */
static int
wdb_tree_tcl(clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int     argc;
     char    **argv;
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_tree_cmd(wdbp, interp, argc-1, argv+1);
}

/*
 *  			W D B _ C O L O R _ P U T R E C
 *  
 *  Used to create a database record and get it written out to a granule.
 *  In some cases, storage will need to be allocated.
 */
static void
wdb_color_putrec(register struct mater	*mp,
		 Tcl_Interp		*interp,
		 struct db_i		*dbip)
{
	struct directory dir;
	union record rec;

	/* we get here only if database is NOT read-only */

	rec.md.md_id = ID_MATERIAL;
	rec.md.md_low = mp->mt_low;
	rec.md.md_hi = mp->mt_high;
	rec.md.md_r = mp->mt_r;
	rec.md.md_g = mp->mt_g;
	rec.md.md_b = mp->mt_b;

	/* Fake up a directory entry for db_* routines */
	RT_DIR_SET_NAMEP( &dir, "color_putrec" );
	dir.d_magic = RT_DIR_MAGIC;
	dir.d_flags = 0;

	if (mp->mt_daddr == MATER_NO_ADDR) {
		/* Need to allocate new database space */
		if (db_alloc(dbip, &dir, 1) < 0) {
			Tcl_AppendResult(interp,
					 "Database alloc error, aborting",
					 (char *)NULL);
			return;
		}
		mp->mt_daddr = dir.d_addr;
	} else {
		dir.d_addr = mp->mt_daddr;
		dir.d_len = 1;
	}

	if (db_put(dbip, &dir, &rec, 0, 1) < 0) {
		Tcl_AppendResult(interp,
				 "Database write error, aborting",
				 (char *)NULL);
		return;
	}
}

/*
 *  			W D B _ C O L O R _ Z A P R E C
 *  
 *  Used to release database resources occupied by a material record.
 */
static void
wdb_color_zaprec(register struct mater	*mp,
		 Tcl_Interp		*interp,
		 struct db_i		*dbip)
{
	struct directory dir;

	/* we get here only if database is NOT read-only */
	if (mp->mt_daddr == MATER_NO_ADDR)
		return;

	dir.d_magic = RT_DIR_MAGIC;
	RT_DIR_SET_NAMEP( &dir, "color_zaprec" );
	dir.d_len = 1;
	dir.d_addr = mp->mt_daddr;
	dir.d_flags = 0;

	if (db_delete(dbip, &dir) < 0) {
		Tcl_AppendResult(interp,
				 "Database delete error, aborting",
				 (char *)NULL);
		return;
	}
	mp->mt_daddr = MATER_NO_ADDR;
}

int
wdb_color_cmd(struct rt_wdb	*wdbp,
	      Tcl_Interp	*interp,
	      int		argc,
	      char 		**argv)
{
	register struct mater *newp,*next_mater;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc != 6) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_color %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	if (wdbp->dbip->dbi_version < 5) {
		/* Delete all color records from the database */
		newp = rt_material_head;
		while (newp != MATER_NULL) {
			next_mater = newp->mt_forw;
			wdb_color_zaprec(newp, interp, wdbp->dbip);
			newp = next_mater;
		}

		/* construct the new color record */
		BU_GETSTRUCT(newp, mater);
		newp->mt_low = atoi(argv[1]);
		newp->mt_high = atoi(argv[2]);
		newp->mt_r = atoi(argv[3]);
		newp->mt_g = atoi(argv[4]);
		newp->mt_b = atoi(argv[5]);
		newp->mt_daddr = MATER_NO_ADDR;		/* not in database yet */

		/* Insert new color record in the in-memory list */
		rt_insert_color(newp);

		/* Write new color records for all colors in the list */
		newp = rt_material_head;
		while (newp != MATER_NULL) {
			next_mater = newp->mt_forw;
			wdb_color_putrec(newp, interp, wdbp->dbip);
			newp = next_mater;
		}
	} else {
		struct bu_vls colors;

		/* construct the new color record */
		BU_GETSTRUCT(newp, mater);
		newp->mt_low = atoi(argv[1]);
		newp->mt_high = atoi(argv[2]);
		newp->mt_r = atoi(argv[3]);
		newp->mt_g = atoi(argv[4]);
		newp->mt_b = atoi(argv[5]);
		newp->mt_daddr = MATER_NO_ADDR;		/* not in database yet */

		/* Insert new color record in the in-memory list */
		rt_insert_color(newp);

		/*
		 * Gather color records from the in-memory list to build
		 * the _GLOBAL objects regionid_colortable attribute.
		 */
		newp = rt_material_head;
		bu_vls_init(&colors);
		while (newp != MATER_NULL) {
			next_mater = newp->mt_forw;
			bu_vls_printf(&colors, "{%d %d %d %d %d} ", newp->mt_low, newp->mt_high,
				      newp->mt_r, newp->mt_g, newp->mt_b);
			newp = next_mater;
		}

		db5_update_attribute("_GLOBAL", "regionid_colortable", bu_vls_addr(&colors), wdbp->dbip);
		bu_vls_free(&colors);
	}

	return TCL_OK;
}

/*
 * Usage:
 *        procname color low high r g b
 */
static int
wdb_color_tcl(ClientData	clientData,
	      Tcl_Interp	*interp,
	      int		argc,
	      char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_color_cmd(wdbp, interp, argc-1, argv+1);
}

static void
wdb_pr_mater(register struct mater	*mp,
	     Tcl_Interp			*interp,
	     int			*ccp,
	     int			*clp)
{
	char buf[128];
	struct bu_vls vls;

	bu_vls_init(&vls);

	(void)sprintf(buf, "%5d..%d", mp->mt_low, mp->mt_high );
	wdb_vls_col_item(&vls, buf, ccp, clp);
	(void)sprintf( buf, "%3d,%3d,%3d", mp->mt_r, mp->mt_g, mp->mt_b);
	wdb_vls_col_item(&vls, buf, ccp, clp);
	wdb_vls_col_eol(&vls, ccp, clp);

	Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
	bu_vls_free(&vls);
}

int
wdb_prcolor_cmd(struct rt_wdb	*wdbp,
		Tcl_Interp	*interp,
		int		argc,
		char 		**argv)
{
	register struct mater *mp;
	int col_count = 0;
	int col_len = 0;

	if (argc != 1) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_prcolor %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	if (rt_material_head == MATER_NULL) {
		Tcl_AppendResult(interp, "none", (char *)NULL);
		return TCL_OK;
	}

	for (mp = rt_material_head; mp != MATER_NULL; mp = mp->mt_forw)
		wdb_pr_mater(mp, interp, &col_count, &col_len);

	return TCL_OK;
}

/*
 * Usage:
 *        procname prcolor
 */
static int
wdb_prcolor_tcl(clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int     argc;
     char    **argv;
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_prcolor_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_tol_cmd(struct rt_wdb	*wdbp,
	    Tcl_Interp		*interp,
	    int			argc,
	    char		**argv)
{
	struct bu_vls vls;
	double	f;

	if (argc < 1 || 3 < argc){
		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_tol %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	/* print all tolerance settings */
	if (argc == 1) {
		Tcl_AppendResult(interp, "Current tolerance settings are:\n", (char *)NULL);
		Tcl_AppendResult(interp, "Tesselation tolerances:\n", (char *)NULL );

		if (wdbp->wdb_ttol.abs > 0.0) {
			bu_vls_init(&vls);
			bu_vls_printf(&vls, "\tabs %g mm\n", wdbp->wdb_ttol.abs);
			Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
			bu_vls_free(&vls);
		} else {
			Tcl_AppendResult(interp, "\tabs None\n", (char *)NULL);
		}

		if (wdbp->wdb_ttol.rel > 0.0) {
			bu_vls_init(&vls);
			bu_vls_printf(&vls, "\trel %g (%g%%)\n",
				      wdbp->wdb_ttol.rel, wdbp->wdb_ttol.rel * 100.0 );
			Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
			bu_vls_free(&vls);
		} else {
			Tcl_AppendResult(interp, "\trel None\n", (char *)NULL);
		}

		if (wdbp->wdb_ttol.norm > 0.0) {
			int	deg, min;
			double	sec;

			bu_vls_init(&vls);
			sec = wdbp->wdb_ttol.norm * bn_radtodeg;
			deg = (int)(sec);
			sec = (sec - (double)deg) * 60;
			min = (int)(sec);
			sec = (sec - (double)min) * 60;

			bu_vls_printf(&vls, "\tnorm %g degrees (%d deg %d min %g sec)\n",
				      wdbp->wdb_ttol.norm * bn_radtodeg, deg, min, sec);
			Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
			bu_vls_free(&vls);
		} else {
			Tcl_AppendResult(interp, "\tnorm None\n", (char *)NULL);
		}

		bu_vls_init(&vls);
		bu_vls_printf(&vls,"Calculational tolerances:\n");
		bu_vls_printf(&vls,
			      "\tdistance = %g mm\n\tperpendicularity = %g (cosine of %g degrees)",
			      wdbp->wdb_tol.dist, wdbp->wdb_tol.perp,
			      acos(wdbp->wdb_tol.perp)*bn_radtodeg);
		Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
		bu_vls_free(&vls);

		return TCL_OK;
	}

	/* get the specified tolerance */
	if (argc == 2) {
		int status = TCL_OK;

		bu_vls_init(&vls);

		switch (argv[1][0]) {
		case 'a':
			if (wdbp->wdb_ttol.abs > 0.0)
				bu_vls_printf(&vls, "%g", wdbp->wdb_ttol.abs);
			else
				bu_vls_printf(&vls, "None");
			break;
		case 'r':
			if (wdbp->wdb_ttol.rel > 0.0)
				bu_vls_printf(&vls, "%g", wdbp->wdb_ttol.rel);
			else
				bu_vls_printf(&vls, "None");
			break;
		case 'n':
			if (wdbp->wdb_ttol.norm > 0.0)
				bu_vls_printf(&vls, "%g", wdbp->wdb_ttol.norm);
			else
				bu_vls_printf(&vls, "None");
			break;
		case 'd':
			bu_vls_printf(&vls, "%g", wdbp->wdb_tol.dist);
			break;
		case 'p':
			bu_vls_printf(&vls, "%g", wdbp->wdb_tol.perp);
			break;
		default:
			bu_vls_printf(&vls, "unrecognized tolerance type - %s", argv[1]);
			status = TCL_ERROR;
			break;
		}

		Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
		bu_vls_free(&vls);
		return status;
	}

	/* set the specified tolerance */
	if (sscanf(argv[2], "%lf", &f) != 1) {
		bu_vls_init(&vls);
		bu_vls_printf(&vls, "bad tolerance - %s", argv[2]);
		Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
		bu_vls_free(&vls);

		return TCL_ERROR;
	}

	switch (argv[1][0]) {
	case 'a':
		/* Absolute tol */
		if (f <= 0.0)
			wdbp->wdb_ttol.abs = 0.0;
		else
			wdbp->wdb_ttol.abs = f;
		break;
	case 'r':
		if (f < 0.0 || f >= 1.0) {
			   Tcl_AppendResult(interp,
					    "relative tolerance must be between 0 and 1, not changed\n",
					    (char *)NULL);
			   return TCL_ERROR;
		}
		/* Note that a value of 0.0 will disable relative tolerance */
		wdbp->wdb_ttol.rel = f;
		break;
	case 'n':
		/* Normal tolerance, in degrees */
		if (f < 0.0 || f > 90.0) {
			Tcl_AppendResult(interp,
					 "Normal tolerance must be in positive degrees, < 90.0\n",
					 (char *)NULL);
			return TCL_ERROR;
		}
		/* Note that a value of 0.0 or 360.0 will disable this tol */
		wdbp->wdb_ttol.norm = f * bn_degtorad;
		break;
	case 'd':
		/* Calculational distance tolerance */
		if (f < 0.0) {
			Tcl_AppendResult(interp,
					 "Calculational distance tolerance must be positive\n",
					 (char *)NULL);
			return TCL_ERROR;
		}
		wdbp->wdb_tol.dist = f;
		wdbp->wdb_tol.dist_sq = wdbp->wdb_tol.dist * wdbp->wdb_tol.dist;
		break;
	case 'p':
		/* Calculational perpendicularity tolerance */
		if (f < 0.0 || f > 1.0) {
			Tcl_AppendResult(interp,
					 "Calculational perpendicular tolerance must be from 0 to 1\n",
					 (char *)NULL);
			return TCL_ERROR;
		}
		wdbp->wdb_tol.perp = f;
		wdbp->wdb_tol.para = 1.0 - f;
		break;
	default:
		bu_vls_init(&vls);
		bu_vls_printf(&vls, "unrecognized tolerance type - %s", argv[1]);
		Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
		bu_vls_free(&vls);

		return TCL_ERROR;
	}

	return TCL_OK;
}

/*
 * Usage:
 *        procname tol [abs|rel|norm|dist|perp [#]]
 *
 *  abs #	sets absolute tolerance.  # > 0.0
 *  rel #	sets relative tolerance.  0.0 < # < 1.0
 *  norm #	sets normal tolerance, in degrees.
 *  dist #	sets calculational distance tolerance
 *  perp #	sets calculational normal tolerance.
 *
 */
static int
wdb_tol_tcl(ClientData	clientData,
	    Tcl_Interp	*interp,
	    int		argc,
	    char	**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_tol_cmd(wdbp, interp, argc-1, argv+1);
}

/* structure to hold all solids that have been pushed. */
struct wdb_push_id {
	long	magic;
	struct wdb_push_id *forw, *back;
	struct directory *pi_dir;
	mat_t	pi_mat;
};

#define WDB_MAGIC_PUSH_ID	0x50495323
#define FOR_ALL_WDB_PUSH_SOLIDS(_p,_phead) \
	for(_p=_phead.forw; _p!=&_phead; _p=_p->forw)
struct wdb_push_data {
	Tcl_Interp		*interp;
	struct wdb_push_id	pi_head;
	int			push_error;
};

/*
 *		P U S H _ L E A F
 *
 * This routine must be prepared to run in parallel.
 *
 * This routine is called once for eas leaf (solid) that is to
 * be pushed.  All it does is build at push_id linked list.  The 
 * linked list could be handled by bu_list macros but it is simple
 * enough to do hear with out them.
 */
static union tree *
wdb_push_leaf(struct db_tree_state	*tsp,
	      struct db_full_path	*pathp,
	      struct rt_db_internal	*ip,
	      genptr_t			client_data)
{
	union tree	*curtree;
	struct directory *dp;
	register struct wdb_push_id *pip;
	struct wdb_push_data *wpdp = (struct wdb_push_data *)client_data;

	RT_CK_TESS_TOL(tsp->ts_ttol);
	BN_CK_TOL(tsp->ts_tol);
	RT_CK_RESOURCE(tsp->ts_resp);

	dp = pathp->fp_names[pathp->fp_len-1];

	if (RT_G_DEBUG&DEBUG_TREEWALK) {
		char *sofar = db_path_to_string(pathp);

		Tcl_AppendResult(wpdp->interp, "wdb_push_leaf(",
				ip->idb_meth->ft_name,
				 ") path='", sofar, "'\n", (char *)NULL);
		bu_free((genptr_t)sofar, "path string");
	}
/*
 * XXX - This will work but is not the best method.  dp->d_uses tells us
 * if this solid (leaf) has been seen before.  If it hasn't just add
 * it to the list.  If it has, search the list to see if the matricies
 * match and do the "right" thing.
 *
 * (There is a question as to whether dp->d_uses is reset to zero
 *  for each tree walk.  If it is not, then d_uses is NOT a safe
 *  way to check and this method will always work.)
 */
	bu_semaphore_acquire(RT_SEM_WORKER);
	FOR_ALL_WDB_PUSH_SOLIDS(pip,wpdp->pi_head) {
		if (pip->pi_dir == dp ) {
			if (!bn_mat_is_equal(pip->pi_mat,
					     tsp->ts_mat, tsp->ts_tol)) {
				char *sofar = db_path_to_string(pathp);

				Tcl_AppendResult(wpdp->interp, "wdb_push_leaf: matrix mismatch between '", sofar,
						 "' and prior reference.\n", (char *)NULL);
				bu_free((genptr_t)sofar, "path string");
				wpdp->push_error = 1;
			}

			bu_semaphore_release(RT_SEM_WORKER);
			RT_GET_TREE(curtree, tsp->ts_resp);
			curtree->magic = RT_TREE_MAGIC;
			curtree->tr_op = OP_NOP;
			return curtree;
		}
	}
/*
 * This is the first time we have seen this solid.
 */
	pip = (struct wdb_push_id *) bu_malloc(sizeof(struct wdb_push_id), "Push ident");
	pip->magic = WDB_MAGIC_PUSH_ID;
	pip->pi_dir = dp;
	MAT_COPY(pip->pi_mat, tsp->ts_mat);
	pip->back = wpdp->pi_head.back;
	wpdp->pi_head.back = pip;
	pip->forw = &wpdp->pi_head;
	pip->back->forw = pip;
	bu_semaphore_release(RT_SEM_WORKER);
	RT_GET_TREE( curtree, tsp->ts_resp );
	curtree->magic = RT_TREE_MAGIC;
	curtree->tr_op = OP_NOP;
	return curtree;
}
/*
 * A null routine that does nothing.
 */
static union tree *
wdb_push_region_end(register struct db_tree_state *tsp,
		    struct db_full_path		*pathp,
		    union tree			*curtree,
		    genptr_t			client_data)
{
	return curtree;
}

int
wdb_push_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char 		**argv)
{
	struct wdb_push_data	*wpdp;
	struct wdb_push_id	*pip;
	struct rt_db_internal	es_int;
	int			i;
	int			ncpu;
	int			c;
	int			old_debug;
	int			push_error;
	extern 	int		bu_optind;
	extern	char		*bu_optarg;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc < 2 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_push %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	RT_CHECK_DBI(wdbp->dbip);

	BU_GETSTRUCT(wpdp,wdb_push_data);
	wpdp->interp = interp;
	wpdp->push_error = 0;
	wpdp->pi_head.magic = WDB_MAGIC_PUSH_ID;
	wpdp->pi_head.forw = wpdp->pi_head.back = &wpdp->pi_head;
	wpdp->pi_head.pi_dir = (struct directory *) 0;

	old_debug = RT_G_DEBUG;

	/* Initial values for options, must be reset each time */
	ncpu = 1;

	/* Parse options */
	bu_optind = 1;	/* re-init bu_getopt() */
	while ((c=bu_getopt(argc, argv, "P:d")) != EOF) {
		switch (c) {
		case 'P':
			ncpu = atoi(bu_optarg);
			if (ncpu<1) ncpu = 1;
			break;
		case 'd':
			rt_g.debug |= DEBUG_TREEWALK;
			break;
		case '?':
		default:
		  Tcl_AppendResult(interp, "push: usage push [-P processors] [-d] root [root2 ...]\n", (char *)NULL);
			break;
		}
	}

	argc -= bu_optind;
	argv += bu_optind;

	/*
	 * build a linked list of solids with the correct
	 * matrix to apply to each solid.  This will also
	 * check to make sure that a solid is not pushed in two
	 * different directions at the same time.
	 */
	i = db_walk_tree(wdbp->dbip, argc, (const char **)argv,
			 ncpu,
			 &wdbp->wdb_initial_tree_state,
			 0,				/* take all regions */
			 wdb_push_region_end,
			 wdb_push_leaf, (genptr_t)wpdp);

	/*
	 * If there was any error, then just free up the solid
	 * list we just built.
	 */
	if (i < 0 || wpdp->push_error) {
		while (wpdp->pi_head.forw != &wpdp->pi_head) {
			pip = wpdp->pi_head.forw;
			pip->forw->back = pip->back;
			pip->back->forw = pip->forw;
			bu_free((genptr_t)pip, "Push ident");
		}
		rt_g.debug = old_debug;
		bu_free((genptr_t)wpdp, "wdb_push_tcl: wpdp");
		Tcl_AppendResult(interp,
				 "push:\tdb_walk_tree failed or there was a solid moving\n\tin two or more directions",
				 (char *)NULL);
		return TCL_ERROR;
	}
/*
 * We've built the push solid list, now all we need to do is apply
 * the matrix we've stored for each solid.
 */
	FOR_ALL_WDB_PUSH_SOLIDS(pip,wpdp->pi_head) {
		if (rt_db_get_internal(&es_int, pip->pi_dir, wdbp->dbip, pip->pi_mat, &rt_uniresource) < 0) {
			Tcl_AppendResult(interp, "f_push: Read error fetching '",
				   pip->pi_dir->d_namep, "'\n", (char *)NULL);
			wpdp->push_error = -1;
			continue;
		}
		RT_CK_DB_INTERNAL(&es_int);

		if (rt_db_put_internal(pip->pi_dir, wdbp->dbip, &es_int, &rt_uniresource) < 0) {
			Tcl_AppendResult(interp, "push(", pip->pi_dir->d_namep,
					 "): solid export failure\n", (char *)NULL);
		}
		rt_db_free_internal(&es_int, &rt_uniresource);
	}

	/*
	 * Now use the wdb_identitize() tree walker to turn all the
	 * matricies in a combination to the identity matrix.
	 * It would be nice to use db_tree_walker() but the tree
	 * walker does not give us all combinations, just regions.
	 * This would work if we just processed all matricies backwards
	 * from the leaf (solid) towards the root, but all in all it
	 * seems that this is a better method.
	 */

	while (argc > 0) {
		struct directory *db;
		db = db_lookup(wdbp->dbip, *argv++, 0);
		if (db)
			wdb_identitize(db, wdbp->dbip, interp);
		--argc;
	}

	/*
	 * Free up the solid table we built.
	 */
	while (wpdp->pi_head.forw != &wpdp->pi_head) {
		pip = wpdp->pi_head.forw;
		pip->forw->back = pip->back;
		pip->back->forw = pip->forw;
		bu_free((genptr_t)pip, "Push ident");
	}

	rt_g.debug = old_debug;
	push_error = wpdp->push_error;
	bu_free((genptr_t)wpdp, "wdb_push_tcl: wpdp");

	return push_error ? TCL_ERROR : TCL_OK;
}

/*
 * The push command is used to move matrices from combinations 
 * down to the solids. At some point, it is worth while thinking
 * about adding a limit to have the push go only N levels down.
 *
 * the -d flag turns on the treewalker debugging output.
 * the -P flag allows for multi-processor tree walking (not useful)
 *
 * Usage:
 *        procname push object(s)
 */
static int
wdb_push_tcl(ClientData	clientData,
	     Tcl_Interp	*interp,
	     int	argc,
	     char	**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_push_cmd(wdbp, interp, argc-1, argv+1);
}

static void
increment_uses(struct db_i	*db_ip,
	       struct directory	*dp,
	       genptr_t		ptr)
{
	RT_CK_DIR(dp);

	dp->d_uses++;
}

static void
increment_nrefs(struct db_i		*db_ip,
		struct directory	*dp,
		genptr_t		ptr)
{
	RT_CK_DIR(dp);

	dp->d_nref++;
}

struct object_use
{
	struct bu_list		l;
	struct directory	*dp;
	mat_t			xform;
	int			used;
};

static void
Free_uses(struct db_i		*dbip,
	  struct directory	*dp,
	  genptr_t		ptr)
{
	struct object_use *use;

	RT_CK_DIR(dp);

	while (BU_LIST_NON_EMPTY(&dp->d_use_hd)) {
		use = BU_LIST_FIRST(object_use, &dp->d_use_hd);
		if (!use->used) {
			/* never used, so delete directory entry.
			 * This could actually delete the original, buts that's O.K.
			 */
			db_dirdelete(dbip, use->dp);
		}

		BU_LIST_DEQUEUE(&use->l);
		bu_free((genptr_t)use, "Free_uses: use");
	}
}

static void
Make_new_name(struct db_i	*dbip,
	      struct directory	*dp,
	      genptr_t		ptr)
{
	struct object_use *use;
	int use_no;
	int digits;
	int suffix_start;
	int name_length;
	int j;
	char format_v4[25], format_v5[25];
	struct bu_vls name_v5;
	char name_v4[NAMESIZE];
	char *name;

	/* only one use and not referenced elsewhere, nothing to do */
	if (dp->d_uses < 2 && dp->d_uses == dp->d_nref)
		return;

	/* check if already done */
	if (BU_LIST_NON_EMPTY(&dp->d_use_hd))
		return;

	digits = log10((double)dp->d_uses) + 2.0;
	sprintf(format_v5, "%%s_%%0%dd", digits);
	sprintf(format_v4, "_%%0%dd", digits);

	name_length = strlen(dp->d_namep);
	if (name_length + digits + 1 > NAMESIZE - 1)
		suffix_start = NAMESIZE - digits - 2;
	else
		suffix_start = name_length;

	if (dbip->dbi_version >= 5)
		bu_vls_init(&name_v5);
	j = 0;
	for (use_no=0 ; use_no<dp->d_uses ; use_no++) {
		j++;
		use = (struct object_use *)bu_malloc( sizeof( struct object_use ), "Make_new_name: use" );

		/* set xform for this object_use to all zeros */
		MAT_ZERO(use->xform);
		use->used = 0;
		if (dbip->dbi_version < 5) {
			NAMEMOVE(dp->d_namep, name_v4);
			name_v4[NAMESIZE-1] = '\0';                /* ensure null termination */
		}

		/* Add an entry for the original at the end of the list
		 * This insures that the original will be last to be modified
		 * If original were modified earlier, copies would be screwed-up
		 */
		if (use_no == dp->d_uses-1 && dp->d_uses == dp->d_nref)
			use->dp = dp;
		else {
			if (dbip->dbi_version < 5) {
				sprintf(&name_v4[suffix_start], format_v4, j);
				name = name_v4;
			} else {
				bu_vls_trunc(&name_v5, 0);
				bu_vls_printf(&name_v5, format_v5, dp->d_namep, j);
				name = bu_vls_addr(&name_v5);
			}

			/* Insure that new name is unique */
			while (db_lookup( dbip, name, 0 ) != DIR_NULL) {
				j++;
				if (dbip->dbi_version < 5) {
					sprintf(&name_v4[suffix_start], format_v4, j);
					name = name_v4;
				} else {
					bu_vls_trunc(&name_v5, 0);
					bu_vls_printf(&name_v5, format_v5, dp->d_namep, j);
					name = bu_vls_addr(&name_v5);
				}
			}

			/* Add new name to directory */
			if ((use->dp = db_diradd(dbip, name, -1, 0, dp->d_flags,
						 (genptr_t)&dp->d_minor_type)) == DIR_NULL) {
				WDB_ALLOC_ERR_return;
			}
		}

		/* Add new directory pointer to use list for this object */
		BU_LIST_INSERT(&dp->d_use_hd, &use->l);
	}

	if (dbip->dbi_version >= 5)
		bu_vls_free(&name_v5);
}

static struct directory *
Copy_solid(struct db_i		*dbip,
	   struct directory	*dp,
	   mat_t		xform,
	   Tcl_Interp		*interp,
	   struct rt_wdb	*wdbp)
{
	struct directory *found;
	struct rt_db_internal sol_int;
	struct object_use *use;

	RT_CK_DIR(dp);

	if (!(dp->d_flags & DIR_SOLID)) {
		Tcl_AppendResult(interp, "Copy_solid: ", dp->d_namep,
				 " is not a solid!!!!\n", (char *)NULL);
		return (DIR_NULL);
	}

	/* If no transformation is to be applied, just use the original */
	if (bn_mat_is_identity(xform)) {
		/* find original in the list */
		for (BU_LIST_FOR(use, object_use, &dp->d_use_hd)) {
			if (use->dp == dp && use->used == 0) {
				use->used = 1;
				return (dp);
			}
		}
	}

	/* Look for a copy that already has this transform matrix */
	for (BU_LIST_FOR(use, object_use, &dp->d_use_hd)) {
		if (bn_mat_is_equal(xform, use->xform, &wdbp->wdb_tol)) {
			/* found a match, no need to make another copy */
			use->used = 1;
			return(use->dp);
		}
	}

	/* get a fresh use */
	found = DIR_NULL;
	for (BU_LIST_FOR(use, object_use, &dp->d_use_hd)) {
		if (use->used)
			continue;

		found = use->dp;
		use->used = 1;
		MAT_COPY(use->xform, xform);
		break;
	}

	if (found == DIR_NULL && dp->d_nref == 1 && dp->d_uses == 1) {
		/* only one use, take it */
		found = dp;
	}

	if (found == DIR_NULL) {
		Tcl_AppendResult(interp, "Ran out of uses for solid ",
				 dp->d_namep, "\n", (char *)NULL);
		return (DIR_NULL);
	}

	if (rt_db_get_internal(&sol_int, dp, dbip, xform, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "Cannot import solid ",
				 dp->d_namep, "\n", (char *)NULL);
		return (DIR_NULL);
	}

	RT_CK_DB_INTERNAL(&sol_int);
	if (rt_db_put_internal(found, dbip, &sol_int, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "Cannot write copy solid (", found->d_namep,
				 ") to database\n", (char *)NULL);
		return (DIR_NULL);
	}

	return (found);
}

static struct directory *Copy_object();

HIDDEN void
Do_copy_membs(struct db_i		*dbip,
	      struct rt_comb_internal	*comb,
	      union tree		*comb_leaf,
	      genptr_t			user_ptr1,
	      genptr_t			user_ptr2,
	      genptr_t			user_ptr3)
{
	struct directory	*dp;
	struct directory	*dp_new;
	mat_t			new_xform;
	matp_t			xform;
	Tcl_Interp		*interp;
	struct rt_wdb		*wdbp;

	RT_CK_DBI(dbip);
	RT_CK_TREE(comb_leaf);

	if ((dp=db_lookup(dbip, comb_leaf->tr_l.tl_name, LOOKUP_QUIET)) == DIR_NULL)
		return;

	xform = (matp_t)user_ptr1;
	interp = (Tcl_Interp *)user_ptr2;
	wdbp = (struct rt_wdb *)user_ptr3;

	/* apply transform matrix for this arc */
	if (comb_leaf->tr_l.tl_mat) {
		bn_mat_mul(new_xform, xform, comb_leaf->tr_l.tl_mat);
	} else {
		MAT_COPY(new_xform, xform);
	}

	/* Copy member with current tranform matrix */
	if ((dp_new=Copy_object(dbip, dp, new_xform, interp, wdbp)) == DIR_NULL) {
		Tcl_AppendResult(interp, "Failed to copy object ",
				 dp->d_namep, "\n", (char *)NULL);
		return;
	}

	/* replace member name with new copy */
	bu_free(comb_leaf->tr_l.tl_name, "comb_leaf->tr_l.tl_name");
	comb_leaf->tr_l.tl_name = bu_strdup(dp_new->d_namep);

	/* make transform for this arc the identity matrix */
	if (!comb_leaf->tr_l.tl_mat) {
		comb_leaf->tr_l.tl_mat = (matp_t)bu_malloc(sizeof(mat_t), "tl_mat");
	}
	MAT_IDN(comb_leaf->tr_l.tl_mat);
}

static struct directory *
Copy_comb(struct db_i		*dbip,
	  struct directory	*dp,
	  mat_t			xform,
	  Tcl_Interp		*interp,
	  struct rt_wdb		*wdbp)
{
	struct object_use *use;
	struct directory *found;
	struct rt_db_internal intern;
	struct rt_comb_internal *comb;

	RT_CK_DIR(dp);

	/* Look for a copy that already has this transform matrix */
	for (BU_LIST_FOR(use, object_use, &dp->d_use_hd)) {
		if (bn_mat_is_equal(xform, use->xform, &wdbp->wdb_tol)) {
			/* found a match, no need to make another copy */
			use->used = 1;
			return (use->dp);
		}
	}

	/* if we can't get records for this combination, just leave it alone */
	if (rt_db_get_internal(&intern, dp, dbip, (fastf_t *)NULL, &rt_uniresource) < 0)
		return (dp);
	comb = (struct rt_comb_internal *)intern.idb_ptr;

	/* copy members */
	if (comb->tree)
		db_tree_funcleaf(dbip, comb, comb->tree, Do_copy_membs,
				 (genptr_t)xform, (genptr_t)interp, (genptr_t)wdbp);

	/* Get a use of this object */
	found = DIR_NULL;
	for (BU_LIST_FOR(use, object_use, &dp->d_use_hd)) {
		/* Get a fresh use of this object */
		if (use->used)
			continue;	/* already used */
		found = use->dp;
		use->used = 1;
		MAT_COPY(use->xform, xform);
		break;
	}

	if (found == DIR_NULL && dp->d_nref == 1 && dp->d_uses == 1) {
		/* only one use, so take original */
		found = dp;
	}

	if (found == DIR_NULL) {
		Tcl_AppendResult(interp, "Ran out of uses for combination ",
				 dp->d_namep, "\n", (char *)NULL);
		return (DIR_NULL);
	}

	if (rt_db_put_internal(found, dbip, &intern, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "rt_db_put_internal failed for ", dp->d_namep,
				 "\n", (char *)NULL);
		rt_comb_ifree(&intern, &rt_uniresource);
		return(DIR_NULL);
	}

	return(found);
}

static struct directory *
Copy_object(struct db_i		*dbip,
	    struct directory	*dp,
	    mat_t		xform,
	    Tcl_Interp		*interp,
	    struct rt_wdb	*wdbp)
{
	RT_CK_DIR(dp);

	if (dp->d_flags & DIR_SOLID)
		return (Copy_solid(dbip, dp, xform, interp, wdbp));
	else
		return (Copy_comb(dbip, dp, xform, interp, wdbp));
}

HIDDEN void
Do_ref_incr(struct db_i			*dbip,
	    struct rt_comb_internal	*comb,
	    union tree			*comb_leaf,
	    genptr_t			user_ptr1,
	    genptr_t			user_ptr2,
	    genptr_t			user_ptr3)
{
	struct directory *dp;

	RT_CK_DBI(dbip);
	RT_CK_TREE(comb_leaf);

	if ((dp = db_lookup(dbip, comb_leaf->tr_l.tl_name, LOOKUP_QUIET)) == DIR_NULL)
		return;

	dp->d_nref++;
}

int
wdb_xpush_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char 		**argv)
{
	struct directory *old_dp;
	struct rt_db_internal intern;
	struct rt_comb_internal *comb;
	struct bu_ptbl tops;
	mat_t xform;
	int i;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc != 2) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_xpush %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	/* get directory pointer for arg */
	if ((old_dp = db_lookup(wdbp->dbip,  argv[1], LOOKUP_NOISY)) == DIR_NULL)
		return TCL_ERROR;

	/* Initialize use and reference counts of all directory entries */
	for (i=0 ; i<RT_DBNHASH ; i++) {
		struct directory *dp;

		for (dp=wdbp->dbip->dbi_Head[i]; dp!=DIR_NULL; dp=dp->d_forw) {
			if (!(dp->d_flags & (DIR_SOLID | DIR_COMB)))
				continue;

			dp->d_uses = 0;
			dp->d_nref = 0;
		}
	}

	/* Count uses in the tree being pushed (updates dp->d_uses) */
	db_functree(wdbp->dbip, old_dp, increment_uses, increment_uses, &rt_uniresource, NULL);

	/* Do a simple reference count to find top level objects */
	for (i=0 ; i<RT_DBNHASH ; i++) {
		struct directory *dp;

		for (dp=wdbp->dbip->dbi_Head[i] ; dp!=DIR_NULL ; dp=dp->d_forw) {
			struct rt_db_internal intern;
			struct rt_comb_internal *comb;

			if (dp->d_flags & DIR_SOLID)
				continue;

			if (!(dp->d_flags & (DIR_SOLID | DIR_COMB)))
				continue;

			if (rt_db_get_internal(&intern, dp, wdbp->dbip, (fastf_t *)NULL, &rt_uniresource) < 0)
				WDB_TCL_READ_ERR_return;
			comb = (struct rt_comb_internal *)intern.idb_ptr;
			if (comb->tree)
				db_tree_funcleaf(wdbp->dbip, comb, comb->tree, Do_ref_incr,
						 (genptr_t )NULL, (genptr_t )NULL, (genptr_t )NULL);
			rt_comb_ifree(&intern, &rt_uniresource);
		}
	}

	/* anything with zero references is a tree top */
	bu_ptbl_init(&tops, 0, "tops for xpush");
	for (i=0; i<RT_DBNHASH; i++) {
		struct directory *dp;

		for (dp=wdbp->dbip->dbi_Head[i]; dp!=DIR_NULL; dp=dp->d_forw) {
			if (dp->d_flags & DIR_SOLID)
				continue;

			if (!(dp->d_flags & (DIR_SOLID | DIR_COMB )))
				continue;

			if (dp->d_nref == 0)
				bu_ptbl(&tops, BU_PTBL_INS, (long *)dp);
		}
	}

	/* now re-zero the reference counts */
	for (i=0 ; i<RT_DBNHASH ; i++) {
		struct directory *dp;

		for (dp=wdbp->dbip->dbi_Head[i]; dp!=DIR_NULL; dp=dp->d_forw) {
			if (!(dp->d_flags & (DIR_SOLID | DIR_COMB)))
				continue;

			dp->d_nref = 0;
		}
	}

	/* accurately count references in entire model */
	for (i=0; i<BU_PTBL_END(&tops); i++) {
		struct directory *dp;

		dp = (struct directory *)BU_PTBL_GET(&tops, i);
		db_functree(wdbp->dbip, dp, increment_nrefs, increment_nrefs, &rt_uniresource, NULL);
	}

	/* Free list of tree-tops */
	bu_ptbl(&tops, BU_PTBL_FREE, (long *)NULL);

	/* Make new names */
	db_functree(wdbp->dbip, old_dp, Make_new_name, Make_new_name, &rt_uniresource, NULL);

	MAT_IDN(xform);

	/* Make new objects */
	if (rt_db_get_internal(&intern, old_dp, wdbp->dbip, (fastf_t *)NULL, &rt_uniresource) < 0) {
		bu_log("ERROR: cannot load %s feom the database!!!\n", old_dp->d_namep);
		bu_log("\tNothing has been changed!!\n");
		db_functree(wdbp->dbip, old_dp, Free_uses, Free_uses, &rt_uniresource, NULL);
		return TCL_ERROR;
	}

	comb = (struct rt_comb_internal *)intern.idb_ptr;
	if (!comb->tree) {
		db_functree(wdbp->dbip, old_dp, Free_uses, Free_uses, &rt_uniresource, NULL);
		return TCL_OK;
	}

	db_tree_funcleaf(wdbp->dbip, comb, comb->tree, Do_copy_membs,
			 (genptr_t)xform, (genptr_t)interp, (genptr_t)wdbp);

	if (rt_db_put_internal(old_dp, wdbp->dbip, &intern, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "rt_db_put_internal failed for ", old_dp->d_namep,
				 "\n", (char *)NULL);
		rt_comb_ifree(&intern, &rt_uniresource);
		db_functree(wdbp->dbip, old_dp, Free_uses, Free_uses, &rt_uniresource, NULL);
		return TCL_ERROR;
	}

	/* Free use lists and delete unused directory entries */
	db_functree(wdbp->dbip, old_dp, Free_uses, Free_uses, &rt_uniresource, NULL);

	return TCL_OK;
}

static int
wdb_xpush_tcl(ClientData	clientData,
	     Tcl_Interp		*interp,
	     int		argc,
	     char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_xpush_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_whatid_cmd(struct rt_wdb	*wdbp,
	       Tcl_Interp	*interp,
	       int		argc,
	       char 		**argv)
{
	struct directory	*dp;
	struct rt_db_internal	intern;
	struct rt_comb_internal	*comb;
	struct bu_vls		vls;

	if (argc != 2) {
		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_whatid %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	if ((dp=db_lookup(wdbp->dbip, argv[1], LOOKUP_NOISY )) == DIR_NULL )
		return TCL_ERROR;

	if (!(dp->d_flags & DIR_REGION)) {
		Tcl_AppendResult(interp, argv[1], " is not a region", (char *)NULL );
		return TCL_ERROR;
	}

	if (rt_db_get_internal(&intern, dp, wdbp->dbip, (fastf_t *)NULL, &rt_uniresource) < 0)
		return TCL_ERROR;
	comb = (struct rt_comb_internal *)intern.idb_ptr;

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "%d", comb->region_id);
	rt_comb_ifree(&intern, &rt_uniresource);
	Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
	bu_vls_free(&vls);

	return TCL_OK;
}

/*
 * Usage:
 *        procname whatid object
 */
static int
wdb_whatid_tcl(ClientData	clientData,
	       Tcl_Interp	*interp,
	       int		argc,
	       char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_whatid_cmd(wdbp, interp, argc-1, argv+1);
}

struct wdb_node_data {
	FILE	     *fp;
	Tcl_Interp   *interp;
};

/*
 *			W D B _ N O D E _ W R I T E
 *
 *  Support for the 'keep' method.
 *  Write each node encountered exactly once.
 */
void
wdb_node_write(struct db_i		*dbip,
	       register struct directory *dp,
	       genptr_t			ptr)
{
	struct rt_wdb	*keepfp = (struct rt_wdb *)ptr;
	struct bu_external	ext;

	RT_CK_WDB(keepfp);

	if (dp->d_nref++ > 0)
		return;		/* already written */

	if (db_get_external(&ext, dp, dbip) < 0)
		WDB_READ_ERR_return;
	if (wdb_export_external(keepfp, &ext, dp->d_namep, dp->d_flags, dp->d_minor_type) < 0)
		WDB_WRITE_ERR_return;
}

int
wdb_keep_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char 		**argv)
{
	struct rt_wdb		*keepfp;
	register struct directory *dp;
	struct bu_vls		title;
	register int		i;
	struct db_i		*new_dbip;

	if (argc < 3 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_keep %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	/* First, clear any existing counts */
	for (i = 0; i < RT_DBNHASH; i++) {
		for (dp = wdbp->dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw)
			dp->d_nref = 0;
	}

	/* Alert user if named file already exists */
	if ((new_dbip = db_open(argv[1], "w")) !=  DBI_NULL  &&
	    (keepfp = wdb_dbopen(new_dbip, RT_WDB_TYPE_DB_DISK)) != NULL) {
		Tcl_AppendResult(interp, "keep:  appending to '", argv[1],
				 "'\n", (char *)NULL);

		/* --- Scan geometry database and build in-memory directory --- */
		db_dirbuild(new_dbip);
	} else {
		/* Create a new database */
		if ((keepfp = wdb_fopen(argv[1])) == NULL) {
			perror(argv[1]);
			return TCL_ERROR;
		}
	}
	
	/* ident record */
	bu_vls_init(&title);
	bu_vls_strcat(&title, "Parts of: ");
	bu_vls_strcat(&title, wdbp->dbip->dbi_title);

	if (db_update_ident(keepfp->dbip, bu_vls_addr(&title), wdbp->dbip->dbi_local2base) < 0) {
		perror("fwrite");
		Tcl_AppendResult(interp, "db_update_ident() failed\n", (char *)NULL);
		wdb_close(keepfp);
		bu_vls_free(&title);
		return TCL_ERROR;
	}
	bu_vls_free(&title);

	for (i = 2; i < argc; i++) {
		if ((dp = db_lookup(wdbp->dbip, argv[i], LOOKUP_NOISY)) == DIR_NULL)
			continue;
		db_functree(wdbp->dbip, dp, wdb_node_write, wdb_node_write, &rt_uniresource, (genptr_t)keepfp);
	}

	wdb_close(keepfp);
	return TCL_OK;
}

/*
 * Usage:
 *        procname keep file object(s)
 */
static int
wdb_keep_tcl(ClientData	clientData,
	     Tcl_Interp *interp,
	     int	argc,
	     char	**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_keep_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_cat_cmd(struct rt_wdb	*wdbp,
	    Tcl_Interp		*interp,
	    int			argc,
	    char 		**argv)
{
	register struct directory	*dp;
	register int			arg;
	struct bu_vls			str;

	if (argc < 2 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_cat %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	bu_vls_init(&str);
	for (arg = 1; arg < argc; arg++) {
		if ((dp = db_lookup(wdbp->dbip, argv[arg], LOOKUP_NOISY)) == DIR_NULL)
			continue;

		bu_vls_trunc(&str, 0);
		wdb_do_list(wdbp->dbip, interp, &str, dp, 0);	/* non-verbose */
		Tcl_AppendResult(interp, bu_vls_addr(&str), "\n", (char *)NULL);
	}
	bu_vls_free(&str);

	return TCL_OK;
}

/*
 * Usage:
 *        procname cat object(s)
 */
static int
wdb_cat_tcl(ClientData	clientData,
	    Tcl_Interp	*interp,
	    int		argc,
	    char	**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_cat_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_instance_cmd(struct rt_wdb	*wdbp,
		 Tcl_Interp	*interp,
		 int		argc,
		 char 		**argv)
{
	register struct directory	*dp;
	char				oper;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc < 3 || 4 < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_instance %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	if ((dp = db_lookup(wdbp->dbip,  argv[1], LOOKUP_NOISY)) == DIR_NULL)
		return TCL_ERROR;

	oper = WMOP_UNION;
	if (argc == 4)
		oper = argv[3][0];

	if (oper != WMOP_UNION &&
	    oper != WMOP_SUBTRACT &&
	    oper != WMOP_INTERSECT) {
		struct bu_vls tmp_vls;

		bu_vls_init(&tmp_vls);
		bu_vls_printf(&tmp_vls, "bad operation: %c\n", oper);
		Tcl_AppendResult(interp, bu_vls_addr(&tmp_vls), (char *)NULL);
		bu_vls_free(&tmp_vls);
		return TCL_ERROR;
	}

	if (wdb_combadd(interp, wdbp->dbip, dp, argv[2], 0, oper, 0, 0, wdbp) == DIR_NULL)
		return TCL_ERROR;

	return TCL_OK;
}

/*
 * Add instance of obj to comb.
 *
 * Usage:
 *        procname i obj comb [op]
 */
static int
wdb_instance_tcl(ClientData	clientData,
		 Tcl_Interp	*interp,
		 int		argc,
		 char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_instance_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_observer_cmd(struct rt_wdb	*wdbp,
		 Tcl_Interp	*interp,
		 int		argc,
		 char 		**argv)
{
	if (argc < 2) {
		struct bu_vls vls;

		/* return help message */
		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_observer %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	return bu_cmd((ClientData)&wdbp->wdb_observers,
		      interp, argc - 1, argv + 1, bu_observer_cmds, 0);
}

/*
 * Attach/detach observers to/from list.
 *
 * Usage:
 *	  procname observer cmd [args]
 *
 */
static int
wdb_observer_tcl(ClientData	clientData,
		 Tcl_Interp	*interp,
		 int		argc,
		 char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_observer_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_make_bb_cmd(struct rt_wdb	*wdbp,
		Tcl_Interp	*interp,
		int		argc,
		char 		**argv)
{
	struct rt_i		*rtip;
	int			i;
	point_t			rpp_min,rpp_max;
	struct db_full_path	path;
	struct directory	*dp;
	struct rt_arb_internal	*arb;
	struct rt_db_internal	new_intern;
	struct region		*regp;
	char			*new_name;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc < 3 || MAXARGS < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_make_bb %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	/* Since arguments may be paths, make sure first argument isn't */
	if (strchr(argv[1], '/')) {
		Tcl_AppendResult(interp, "Do not use '/' in solid names: ", argv[1], "\n", (char *)NULL);
		return TCL_ERROR;
	}

	new_name = argv[1];
	if (db_lookup(wdbp->dbip, new_name, LOOKUP_QUIET) != DIR_NULL) {
		Tcl_AppendResult(interp, new_name, " already exists\n", (char *)NULL);
		return TCL_ERROR;
	}

	/* Make a new rt_i instance from the existing db_i sructure */
	if ((rtip=rt_new_rti(wdbp->dbip)) == RTI_NULL) {
		Tcl_AppendResult(interp, "rt_new_rti failure for ", wdbp->dbip->dbi_filename,
				 "\n", (char *)NULL);
		return TCL_ERROR;
	}

	/* Get trees for list of objects/paths */
	for (i = 2 ; i < argc ; i++) {
		int gottree;

		/* Get full_path structure for argument */
		db_full_path_init(&path);
		if (db_string_to_path(&path,  rtip->rti_dbip, argv[i])) {
			Tcl_AppendResult(interp, "db_string_to_path failed for ",
					 argv[i], "\n", (char *)NULL );
			rt_clean(rtip);
			bu_free((genptr_t)rtip, "wdb_make_bb_cmd: rtip");
			return TCL_ERROR;
		}

		/* check if we alerady got this tree */
		gottree = 0;
		for (BU_LIST_FOR(regp, region, &(rtip->HeadRegion))) {
			struct db_full_path tmp_path;

			db_full_path_init(&tmp_path);
			if (db_string_to_path(&tmp_path, rtip->rti_dbip, regp->reg_name)) {
				Tcl_AppendResult(interp, "db_string_to_path failed for ",
						 regp->reg_name, "\n", (char *)NULL);
				rt_clean(rtip);
				bu_free((genptr_t)rtip, "wdb_make_bb_cmd: rtip");
				return TCL_ERROR;
			}
			if (path.fp_names[0] == tmp_path.fp_names[0])
				gottree = 1;
			db_free_full_path(&tmp_path);
			if (gottree)
				break;
		}

		/* if we don't already have it, get it */
		if (!gottree && rt_gettree(rtip, path.fp_names[0]->d_namep)) {
			Tcl_AppendResult(interp, "rt_gettree failed for ",
					 argv[i], "\n", (char *)NULL );
			rt_clean(rtip);
			bu_free((genptr_t)rtip, "wdb_make_bb_cmd: rtip");
			return TCL_ERROR;
		}
		db_free_full_path(&path);
	}

	/* prep calculates bounding boxes of solids */
	rt_prep(rtip);

	/* initialize RPP bounds */
	VSETALL(rpp_min, MAX_FASTF);
	VREVERSE(rpp_max, rpp_min);
	for (i = 2 ; i < argc ; i++) {
		vect_t reg_min, reg_max;
		struct region *regp;
		const char *reg_name;

		/* check if input name is a region */
		for (BU_LIST_FOR(regp, region, &(rtip->HeadRegion))) {
			reg_name = regp->reg_name;
			if (*argv[i] != '/' && *reg_name == '/')
				reg_name++;

			if (!strcmp( reg_name, argv[i]))
				goto found;
				
		}
		goto not_found;

found:
		if (regp != REGION_NULL) {
			/* input name was a region  */
			if (rt_bound_tree(regp->reg_treetop, reg_min, reg_max)) {
				Tcl_AppendResult(interp, "rt_bound_tree failed for ",
						 regp->reg_name, "\n", (char *)NULL);
				rt_clean(rtip);
				bu_free((genptr_t)rtip, "wdb_make_bb_cmd: rtip");
				return TCL_ERROR;
			}
			VMINMAX(rpp_min, rpp_max, reg_min);
			VMINMAX(rpp_min, rpp_max, reg_max);
		} else {
			int name_len;
not_found:

			/* input name may be a group, need to check all regions under
			 * that group
			 */
			name_len = strlen( argv[i] );
			for (BU_LIST_FOR( regp, region, &(rtip->HeadRegion))) {
				reg_name = regp->reg_name;
				if (*argv[i] != '/' && *reg_name == '/')
					reg_name++;

				if (strncmp(argv[i], reg_name, name_len))
					continue;

				/* This is part of the group */
				if (rt_bound_tree(regp->reg_treetop, reg_min, reg_max)) {
					Tcl_AppendResult(interp, "rt_bound_tree failed for ",
							 regp->reg_name, "\n", (char *)NULL);
					rt_clean(rtip);
					bu_free((genptr_t)rtip, "wdb_make_bb_cmd: rtip");
					return TCL_ERROR;
				}
				VMINMAX(rpp_min, rpp_max, reg_min);
				VMINMAX(rpp_min, rpp_max, reg_max);
			}
		}
	}

	/* build bounding RPP */
	arb = (struct rt_arb_internal *)bu_malloc(sizeof(struct rt_arb_internal), "arb");
	VMOVE(arb->pt[0], rpp_min);
	VSET(arb->pt[1], rpp_min[X], rpp_min[Y], rpp_max[Z]);
	VSET(arb->pt[2], rpp_min[X], rpp_max[Y], rpp_max[Z]);
	VSET(arb->pt[3], rpp_min[X], rpp_max[Y], rpp_min[Z]);
	VSET(arb->pt[4], rpp_max[X], rpp_min[Y], rpp_min[Z]);
	VSET(arb->pt[5], rpp_max[X], rpp_min[Y], rpp_max[Z]);
	VMOVE(arb->pt[6], rpp_max);
	VSET(arb->pt[7], rpp_max[X], rpp_max[Y], rpp_min[Z]);
	arb->magic = RT_ARB_INTERNAL_MAGIC;

	/* set up internal structure */
	RT_INIT_DB_INTERNAL(&new_intern);
	new_intern.idb_type = ID_ARB8;
	new_intern.idb_meth = &rt_functab[ID_ARB8];
	new_intern.idb_ptr = (genptr_t)arb;

	if ((dp=db_diradd( wdbp->dbip, new_name, -1L, 0, DIR_SOLID, (genptr_t)&new_intern.idb_type)) == DIR_NULL) {
		Tcl_AppendResult(interp, "Cannot add ", new_name, " to directory\n", (char *)NULL);
		return TCL_ERROR;
	}

	if (rt_db_put_internal(dp, wdbp->dbip, &new_intern, wdbp->wdb_resp) < 0) {
		rt_db_free_internal(&new_intern, wdbp->wdb_resp);
		Tcl_AppendResult(interp, "Database write error, aborting.\n", (char *)NULL);
		return TCL_ERROR;
	}

	rt_clean(rtip);
	bu_free((genptr_t)rtip, "wdb_make_bb_cmd: rtip");
	return TCL_OK;
}

/*
 *	Build an RPP bounding box for the list of objects
 *	and/or paths passed to this routine
 *
 *	Usage:
 *		dbobjname make_bb bbname obj(s)
 */
static int
wdb_make_bb_tcl(ClientData	clientData,
		Tcl_Interp	*interp,
		int		argc,
		char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_make_bb_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_make_name_cmd(struct rt_wdb	*wdbp,
		  Tcl_Interp	*interp,
		  int		argc,
		  char 		**argv)
{
	struct bu_vls	obj_name;
	char		*cp, *tp;
	static int	i = 0;
	int		len;

	switch (argc) {
	case 2:
		if (strcmp(argv[1], "-s") != 0)
			break;
		else {
			i = 0;
			return TCL_OK;
		}
	case 3:
		{
			int	new_i;

			if ((strcmp(argv[1], "-s") == 0)
			    && (sscanf(argv[2], "%d", &new_i) == 1)) {
				i = new_i;
				return TCL_OK;
			}
		}
	default:
		{
			struct bu_vls	vls;

			bu_vls_init(&vls);
			bu_vls_printf(&vls, "helplib_alias wdb_make_name %s", argv[0]);
			Tcl_Eval(interp, bu_vls_addr(&vls));
			bu_vls_free(&vls);
			return TCL_ERROR;
		}
	}

	bu_vls_init(&obj_name);
	for (cp = argv[1], len = 0; *cp != '\0'; ++cp, ++len) {
		if (*cp == '@') {
			if (*(cp + 1) == '@')
				++cp;
			else
				break;
		}
		bu_vls_putc(&obj_name, *cp);
	}
	bu_vls_putc(&obj_name, '\0');
	tp = (*cp == '\0') ? "" : cp + 1;

	do {
		bu_vls_trunc(&obj_name, len);
		bu_vls_printf(&obj_name, "%d", i++);
		bu_vls_strcat(&obj_name, tp);
	}
	while (db_lookup(wdbp->dbip, bu_vls_addr(&obj_name), LOOKUP_QUIET) != DIR_NULL);
	Tcl_AppendResult(interp, bu_vls_addr(&obj_name), (char *) NULL);
	bu_vls_free(&obj_name);
	return TCL_OK;
}

/*
 *
 * Generate an identifier that is guaranteed not to be the name
 * of any object currently in the database.
 *
 * Usage:
 *	dbobjname make_name (template | -s [num])
 *
 */
static int
wdb_make_name_tcl(ClientData	clientData,
		  Tcl_Interp	*interp,
		  int		argc,
		  char		**argv)

{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_make_name_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_units_cmd(struct rt_wdb	*wdbp,
	      Tcl_Interp	*interp,
	      int		argc,
	      char 		**argv)
{
	double		loc2mm;
	struct bu_vls 	vls;
	const char	*str;
	int 		sflag = 0;

	bu_vls_init(&vls);
	if (argc < 1 || 2 < argc) {
		bu_vls_printf(&vls, "helplib_alias wdb_units %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	if (argc == 2 && strcmp(argv[1], "-s") == 0) {
		--argc;
		++argv;

		sflag = 1;
	}

	if (argc < 2) {
		str = bu_units_string(wdbp->dbip->dbi_local2base);
		if (!str) str = "Unknown_unit";

		if (sflag)
			bu_vls_printf(&vls, "%s", str);
		else
			bu_vls_printf(&vls, "You are editing in '%s'.  1 %s = %g mm \n",
				      str, str, wdbp->dbip->dbi_local2base );

		Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
		bu_vls_free(&vls);
		return TCL_OK;
	}

	/* Allow inputs of the form "25cm" or "3ft" */
	if ((loc2mm = bu_mm_value(argv[1]) ) <= 0) {
		Tcl_AppendResult(interp, argv[1], ": unrecognized unit\n",
				 "valid units: <um|mm|cm|m|km|in|ft|yd|mi>\n", (char *)NULL);
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

        if (db_update_ident(wdbp->dbip, wdbp->dbip->dbi_title, loc2mm) < 0) {
		Tcl_AppendResult(interp,
				 "Warning: unable to stash working units into database\n",
				 (char *)NULL);
        }

	wdbp->dbip->dbi_local2base = loc2mm;
	wdbp->dbip->dbi_base2local = 1.0 / loc2mm;

	str = bu_units_string(wdbp->dbip->dbi_local2base);
	if (!str) str = "Unknown_unit";
	bu_vls_printf(&vls, "You are now editing in '%s'.  1 %s = %g mm \n",
		      str, str, wdbp->dbip->dbi_local2base );
	Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
	bu_vls_free(&vls);

	return TCL_OK;
}

/*
 * Set/get the database units. 
 *
 * Usage:
 *        dbobjname units [str]
 */
static int
wdb_units_tcl(ClientData	clientData,
	      Tcl_Interp	*interp,
	      int		argc,
	      char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_units_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_hide_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char 		**argv)
{
	struct directory		*dp;
	struct db_i			*dbip;
	struct bu_external		ext;
	struct bu_external		tmp;
	struct db5_raw_internal		raw;
	int				i;

	WDB_TCL_CHECK_READ_ONLY;

	if( argc < 2 ) {
		struct bu_vls vls;

		bu_vls_init( &vls );
		bu_vls_printf(&vls, "helplib_alias wdb_hide %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	RT_CK_WDB( wdbp );

	dbip = wdbp->dbip;

	RT_CK_DBI( dbip );
	if( dbip->dbi_version < 5 ) {
		Tcl_AppendResult(interp, "The \"hide\" command is only valid for version 5 databases and later\n",
				 (char *)NULL );
		return TCL_ERROR;
	}

	for( i=1 ; i<argc ; i++ ) {
		if( (dp = db_lookup( dbip, argv[i], LOOKUP_NOISY )) == DIR_NULL ) {
			continue;
		}

		RT_CK_DIR( dp );

		if( dp->d_major_type == DB5_MAJORTYPE_BRLCAD ) {
			int no_hide=0;

			/* warn the user that this might be a bad idea */
			if( isatty(fileno(stdin)) && isatty(fileno(stdout))) {
				char line[80];

				/* classic interactive MGED */
				while( 1 ) {
					bu_log( "Hiding BRL-CAD geometry (%s) is generaly a bad idea.\n", dp->d_namep );
					bu_log( "This may cause unexpected problems with other commands.\n" );
					bu_log( "Are you sure you want to do this?? (y/n)\n" );
					(void)fgets( line, sizeof( line ), stdin );
					if( line[0] == 'y' || line[0] == 'Y' ) break;
					if( line[0] == 'n' || line[0] == 'N' ) {
						no_hide = 1;
						break;
					}
				}
			} else if( Tcl_GetVar2Ex( interp, "tk_version", NULL, TCL_GLOBAL_ONLY ) ) {
				struct bu_vls vls;

				/* Tk is active, we can pop-up a window */
				bu_vls_init( &vls );
				bu_vls_printf( &vls, "Hiding BRL-CAD geometry (%s) is generaly a bad idea.\n", dp->d_namep );
				bu_vls_strcat( &vls, "This may cause unexpected problems with other commands.\n" );
				bu_vls_strcat( &vls, "Are you sure you want to do this??" );
				(void)Tcl_ResetResult( interp );
				if( Tcl_VarEval( interp, "tk_messageBox -type yesno ",
						 "-title Warning -icon question -message {",
						 bu_vls_addr( &vls ), "}",
						 (char *)NULL ) != TCL_OK ) {
					bu_log( "Unable to post question!!!\n" );
				} else {
					char *result;

					result = Tcl_GetStringResult( interp );
					if( !strcmp( result, "no" ) ) {
						no_hide = 1;
					}
					(void)Tcl_ResetResult( interp );
				}
			}
			if( no_hide )
				continue;
		}

		BU_INIT_EXTERNAL(&ext);

		if( db_get_external( &ext, dp, dbip ) < 0 ) {
			Tcl_AppendResult(interp, "db_get_external failed for ",
					 dp->d_namep, " \n", (char *)NULL );
			continue;
		}

		if (db5_get_raw_internal_ptr(&raw, ext.ext_buf) == NULL) {
			Tcl_AppendResult(interp, "db5_get_raw_internal_ptr() failed for ",
					 dp->d_namep, " \n", (char *)NULL );
			bu_free_external( &ext );
			continue;
		}

		raw.h_name_hidden = (unsigned char)(0x1);

		BU_INIT_EXTERNAL( &tmp );
		db5_export_object3( &tmp, DB5HDR_HFLAGS_DLI_APPLICATION_DATA_OBJECT,
			dp->d_namep,
			raw.h_name_hidden,
			&raw.attributes,
			&raw.body,
			raw.major_type, raw.minor_type,
			raw.a_zzz, raw.b_zzz );
		bu_free_external( &ext );

		if( db_put_external( &tmp, dp, dbip ) ) {
			Tcl_AppendResult(interp, "db_put_external() failed for ",
					 dp->d_namep, " \n", (char *)NULL );
			bu_free_external( &tmp );
			continue;
		}
		bu_free_external( &tmp );
		dp->d_flags |= DIR_HIDDEN;
	}

	return TCL_OK;
}

static int
wdb_hide_tcl(ClientData	clientData,
	     Tcl_Interp	*interp,
	     int	argc,
	     char	**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_hide_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_unhide_cmd(struct rt_wdb	*wdbp,
	       Tcl_Interp	*interp,
	       int		argc,
	       char 		**argv)
{
	struct directory		*dp;
	struct db_i			*dbip;
	struct bu_external		ext;
	struct bu_external		tmp;
	struct db5_raw_internal		raw;
	int				i;

	WDB_TCL_CHECK_READ_ONLY;

	if( argc < 2 ) {
		struct bu_vls vls;

		bu_vls_init( &vls );
		bu_vls_printf(&vls, "helplib_alias wdb_unhide %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	RT_CK_WDB( wdbp );

	dbip = wdbp->dbip;

	RT_CK_DBI( dbip );
	if( dbip->dbi_version < 5 ) {
		Tcl_AppendResult(interp, "The \"unhide\" command is only valid for version 5 databases and later\n",
				 (char *)NULL );
		return TCL_ERROR;
	}

	for( i=1 ; i<argc ; i++ ) {
		if( (dp = db_lookup( dbip, argv[i], LOOKUP_NOISY )) == DIR_NULL ) {
			continue;
		}

		RT_CK_DIR( dp );

		BU_INIT_EXTERNAL(&ext);

		if( db_get_external( &ext, dp, dbip ) < 0 ) {
			Tcl_AppendResult(interp, "db_get_external failed for ",
					 dp->d_namep, " \n", (char *)NULL );
			continue;
		}

		if (db5_get_raw_internal_ptr(&raw, ext.ext_buf) == NULL) {
			Tcl_AppendResult(interp, "db5_get_raw_internal_ptr() failed for ",
					 dp->d_namep, " \n", (char *)NULL );
			bu_free_external( &ext );
			continue;
		}

		raw.h_name_hidden = (unsigned char)(0x0);

		BU_INIT_EXTERNAL( &tmp );
		db5_export_object3( &tmp, DB5HDR_HFLAGS_DLI_APPLICATION_DATA_OBJECT,
			dp->d_namep,
			raw.h_name_hidden,
			&raw.attributes,
			&raw.body,
			raw.major_type, raw.minor_type,
			raw.a_zzz, raw.b_zzz );
		bu_free_external( &ext );

		if( db_put_external( &tmp, dp, dbip ) ) {
			Tcl_AppendResult(interp, "db_put_external() failed for ",
					 dp->d_namep, " \n", (char *)NULL );
			bu_free_external( &tmp );
			continue;
		}
		bu_free_external( &tmp );
		dp->d_flags &= (~DIR_HIDDEN);
	}

	return TCL_OK;
}

static int
wdb_unhide_tcl(ClientData	clientData,
	       Tcl_Interp	*interp,
	       int		argc,
	       char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_unhide_cmd(wdbp, interp, argc-1, argv+1);
}

int wdb_attr_rm_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char 		**argv)
{
	int			i;
	struct directory	*dp;
	struct bu_attribute_value_set avs;

	/* this is only valid for v5 databases */
	if( wdbp->dbip->dbi_version < 5 ) {
		Tcl_AppendResult(interp, "Attributes are only available in database version 5 and later\n", (char *)NULL );
		return TCL_ERROR;
	}

	if (argc < 2 ) {
		Tcl_AppendResult(interp,
				 "wrong # args: should be \"", argv[0],
				 " objName [attribute] [[attribute] [attribute]...]\"", (char *)NULL);
		return TCL_ERROR;
	}
	/* Verify that this wdb supports lookup operations
	   (non-null dbip) */
	if (wdbp->dbip == 0) {
		Tcl_AppendResult(interp,
				 "db does not support lookup operations",
				 (char *)NULL);
		return TCL_ERROR;
	}

	if( (dp=db_lookup( wdbp->dbip, argv[1], LOOKUP_NOISY)) == DIR_NULL )
		return TCL_ERROR;

	
	if( db5_get_attributes( wdbp->dbip, &avs, dp ) ) {
		Tcl_AppendResult(interp,
				 "Cannot get attributes for object ", dp->d_namep, "\n", (char *)NULL );
		return TCL_ERROR;
	}

	i = 2;
	while( i < argc ) {
		(void)bu_avs_remove( &avs, argv[i] );
		i++;
	}
	if( db5_replace_attributes( dp, &avs, wdbp->dbip ) ) {
		Tcl_AppendResult(interp, "Error: failed to update attributes\n", (char *)NULL );
		bu_avs_free( &avs );
		return TCL_ERROR;
	}

	/* avs is freed by db5_update_attributes() */
	return TCL_OK;
}

int
wdb_attr_cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char 		**argv)
{
	int			i;
	struct directory	*dp;
	struct bu_attribute_value_set avs;
	struct bu_attribute_value_pair	*avpp;

	/* this is only valid for v5 databases */
	if( wdbp->dbip->dbi_version < 5 ) {
		Tcl_AppendResult(interp, "Attributes are only available in database version 5 and later\n", (char *)NULL );
		return TCL_ERROR;
	}

	if (argc < 2 ) {
		Tcl_AppendResult(interp,
				 "wrong # args: should be \"", argv[0],
				 " objName [attribute] [value] [[attribute] [value]...]\"", (char *)NULL);
		return TCL_ERROR;
	}

	/* Verify that this wdb supports lookup operations
	   (non-null dbip) */
	if (wdbp->dbip == 0) {
		Tcl_AppendResult(interp,
				 "db does not support lookup operations",
				 (char *)NULL);
		return TCL_ERROR;
	}

	if( (dp=db_lookup( wdbp->dbip, argv[1], LOOKUP_NOISY)) == DIR_NULL )
		return TCL_ERROR;

	
	if( db5_get_attributes( wdbp->dbip, &avs, dp ) ) {
		Tcl_AppendResult(interp,
				 "Cannot get attributes for object ", dp->d_namep, "\n", (char *)NULL );
		return TCL_ERROR;
	}

	if( argc == 2 ) {
		/* just list all the attributes */
		avpp = avs.avp;
		for( i=0 ; i < avs.count ; i++, avpp++ ) {
			Tcl_AppendResult(interp, avpp->name, " {", avpp->value, "} ", (char *)NULL );
		}
	} else if( argc == 3 ) {
		/* just getting a single attribute */
		const char *val;

		val = bu_avs_get( &avs, argv[2] );
		if( !val ) {
			Tcl_AppendResult(interp, "Object ", dp->d_namep, " does not have a ", argv[2], " attribute\n", (char *)NULL );
			bu_avs_free( &avs );
			return TCL_ERROR;
		}
		Tcl_AppendResult(interp, val, (char *)NULL );
	} else {
		/* setting attribute/value pairs */
		if( argc % 2 ) {
			Tcl_AppendResult(interp, "Error: attribute names and values must be in pairs!!!\n", (char *)NULL );
			bu_avs_free( &avs );
			return TCL_ERROR;
		}

		i = 2;
		while( i < argc ) {
			(void)bu_avs_add( &avs, argv[i], argv[i+1] );
			i += 2;
		}
		if( db5_update_attributes( dp, &avs, wdbp->dbip ) ) {
			Tcl_AppendResult(interp, "Error: failed to update attributes\n", (char *)NULL );
			bu_avs_free( &avs );
			return TCL_ERROR;
		}

		/* avs is freed by db5_update_attributes() */
		return TCL_OK;
	}

	bu_avs_free( &avs );
	return TCL_OK;
}

int
wdb_attr_tcl(ClientData	clientData,
	     Tcl_Interp     *interp,
	     int		argc,
	     char	      **argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_attr_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_attr_rm_tcl(ClientData	clientData,
	     Tcl_Interp     *interp,
	     int		argc,
	     char	      **argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_attr_rm_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_nmg_simplify_cmd(struct rt_wdb	*wdbp,
		     Tcl_Interp		*interp,
		     int		argc,
		     char 		**argv)
{
	struct directory *dp;
	struct rt_db_internal nmg_intern;
	struct rt_db_internal new_intern;
	struct model *m;
	struct nmgregion *r;
	struct shell *s;
	int do_all=1;
	int do_arb=0;
	int do_tgc=0;
	int do_poly=0;
	char *new_name;
	char *nmg_name;
	int success = 0;
	int shell_count=0;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc < 3 || 4 < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_nmg_simplify %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	RT_INIT_DB_INTERNAL(&new_intern);

	if (argc == 4) {
		do_all = 0;
		if (!strncmp(argv[1], "arb", 3))
			do_arb = 1;
		else if (!strncmp(argv[1], "tgc", 3))
			do_tgc = 1;
		else if (!strncmp(argv[1], "poly", 4))
			do_poly = 1;
		else {
			Tcl_AppendResult(interp,
					 "Usage: nmg_simplify [arb|ell|tgc|poly] new_solid_name nmg_solid\n",
					 (char *)NULL);
			return TCL_ERROR;
		}

		new_name = argv[2];
		nmg_name = argv[3];
	} else {
		new_name = argv[1];
		nmg_name = argv[2];
	}

	if (db_lookup(wdbp->dbip, new_name, LOOKUP_QUIET) != DIR_NULL) {
		Tcl_AppendResult(interp, new_name, " already exists\n", (char *)NULL);
		return TCL_ERROR;
	}

	if ((dp=db_lookup(wdbp->dbip, nmg_name, LOOKUP_QUIET)) == DIR_NULL) {
		Tcl_AppendResult(interp, nmg_name, " does not exist\n", (char *)NULL);
		return TCL_ERROR;
	}

	if (rt_db_get_internal(&nmg_intern, dp, wdbp->dbip, bn_mat_identity, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "rt_db_get_internal() error\n", (char *)NULL);
		return TCL_ERROR;
	}

	if (nmg_intern.idb_type != ID_NMG) {
		Tcl_AppendResult(interp, nmg_name, " is not an NMG solid\n", (char *)NULL);
		rt_db_free_internal(&nmg_intern, &rt_uniresource);
		return TCL_ERROR;
	}

	m = (struct model *)nmg_intern.idb_ptr;
	NMG_CK_MODEL(m);

	/* count shells */
	for (BU_LIST_FOR(r, nmgregion, &m->r_hd)) {
		for (BU_LIST_FOR(s, shell, &r->s_hd))
			shell_count++;
	}

	if ((do_arb || do_all) && shell_count == 1) {
		struct rt_arb_internal arb_int;

		if (nmg_to_arb(m, &arb_int)) {
			new_intern.idb_ptr = (genptr_t)(&arb_int);
			new_intern.idb_type = ID_ARB8;
			new_intern.idb_meth = &rt_functab[ID_ARB8];
			success = 1;
		} else if (do_arb) {
			/* see if we can get an arb by simplifying the NMG */

			r = BU_LIST_FIRST( nmgregion, &m->r_hd );
			s = BU_LIST_FIRST( shell, &r->s_hd );
			nmg_shell_coplanar_face_merge( s, &wdbp->wdb_tol, 1 );
			if (!nmg_kill_cracks(s)) {
				(void) nmg_model_edge_fuse( m, &wdbp->wdb_tol );
				(void) nmg_model_edge_g_fuse( m, &wdbp->wdb_tol );
				(void) nmg_unbreak_region_edges( &r->l.magic );
				if (nmg_to_arb(m, &arb_int)) {
					new_intern.idb_ptr = (genptr_t)(&arb_int);
					new_intern.idb_type = ID_ARB8;
					new_intern.idb_meth = &rt_functab[ID_ARB8];
					success = 1;
				}
			}
			if (!success) {
				rt_db_free_internal( &nmg_intern, &rt_uniresource );
				Tcl_AppendResult(interp, "Failed to construct an ARB equivalent to ",
						 nmg_name, "\n", (char *)NULL);
				return TCL_OK;
			}
		}
	}

	if ((do_tgc || do_all) && !success && shell_count == 1) {
		struct rt_tgc_internal tgc_int;

		if (nmg_to_tgc(m, &tgc_int, &wdbp->wdb_tol)) {
			new_intern.idb_ptr = (genptr_t)(&tgc_int);
			new_intern.idb_type = ID_TGC;
			new_intern.idb_meth = &rt_functab[ID_TGC];
			success = 1;
		} else if (do_tgc) {
			rt_db_free_internal( &nmg_intern, &rt_uniresource );
			Tcl_AppendResult(interp, "Failed to construct a TGC equivalent to ",
					 nmg_name, "\n", (char *)NULL);
			return TCL_OK;
		}
	}

	/* see if we can get an arb by simplifying the NMG */
	if ((do_arb || do_all) && !success && shell_count == 1) {
		struct rt_arb_internal arb_int;

		r = BU_LIST_FIRST( nmgregion, &m->r_hd );
		s = BU_LIST_FIRST( shell, &r->s_hd );
		nmg_shell_coplanar_face_merge( s, &wdbp->wdb_tol, 1 );
		if (!nmg_kill_cracks(s)) {
			(void) nmg_model_edge_fuse( m, &wdbp->wdb_tol );
			(void) nmg_model_edge_g_fuse( m, &wdbp->wdb_tol );
			(void) nmg_unbreak_region_edges( &r->l.magic );
			if (nmg_to_arb(m, &arb_int )) {
				new_intern.idb_ptr = (genptr_t)(&arb_int);
				new_intern.idb_type = ID_ARB8;
				new_intern.idb_meth = &rt_functab[ID_ARB8];
				success = 1;
			}
			else if (do_arb) {
				rt_db_free_internal( &nmg_intern, &rt_uniresource );
				Tcl_AppendResult(interp, "Failed to construct an ARB equivalent to ",
						 nmg_name, "\n", (char *)NULL);
				return TCL_OK;
			}
		}
	}

	if ((do_poly || do_all) && !success) {
		struct rt_pg_internal *poly_int;

		poly_int = (struct rt_pg_internal *)bu_malloc( sizeof( struct rt_pg_internal ), "f_nmg_simplify: poly_int" );

		if (nmg_to_poly( m, poly_int, &wdbp->wdb_tol)) {
			new_intern.idb_ptr = (genptr_t)(poly_int);
			new_intern.idb_type = ID_POLY;
			new_intern.idb_meth = &rt_functab[ID_POLY];
			success = 1;
		}
		else if (do_poly) {
			rt_db_free_internal( &nmg_intern, &rt_uniresource );
			Tcl_AppendResult(interp, nmg_name, " is not a closed surface, cannot make a polysolid\n", (char *)NULL);
			return TCL_OK;
		}
	}

	if (success) {
		r = BU_LIST_FIRST( nmgregion, &m->r_hd );
		s = BU_LIST_FIRST( shell, &r->s_hd );

		if (BU_LIST_NON_EMPTY( &s->lu_hd))
			Tcl_AppendResult(interp, "wire loops in ", nmg_name,
					 " have been ignored in conversion\n", (char *)NULL);

		if (BU_LIST_NON_EMPTY(&s->eu_hd))
			Tcl_AppendResult(interp, "wire edges in ", nmg_name,
					 " have been ignored in conversion\n", (char *)NULL);

		if (s->vu_p)
			Tcl_AppendResult(interp, "Single vertexuse in shell of ", nmg_name,
					 " has been ignored in conversion\n", (char *)NULL);

		rt_db_free_internal( &nmg_intern, &rt_uniresource );

		if ((dp=db_diradd(wdbp->dbip, new_name, -1L, 0, DIR_SOLID, (genptr_t)&new_intern.idb_type)) == DIR_NULL) {
			Tcl_AppendResult(interp, "Cannot add ", new_name, " to directory\n", (char *)NULL );
			return TCL_ERROR;
		}

		if (rt_db_put_internal(dp, wdbp->dbip, &new_intern, &rt_uniresource) < 0) {
			rt_db_free_internal( &new_intern, &rt_uniresource );
			WDB_TCL_WRITE_ERR_return;
		}
		return TCL_OK;
	}

	Tcl_AppendResult(interp, "simplification to ", argv[1],
			 " is not yet supported\n", (char *)NULL);
	return TCL_ERROR;
}

/*
 * Usage:
 *        procname nmg_simplify [arb|tgc|ell|poly] new_solid nmg_solid
 */
static int
wdb_nmg_simplify_tcl(ClientData		clientData,
		     Tcl_Interp		*interp,
		     int		argc,
		     char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_nmg_simplify_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_nmg_collapse_cmd(struct rt_wdb	*wdbp,
		      Tcl_Interp	*interp,
		      int		argc,
		      char 		**argv)
{
	char *new_name;
	struct model *m;
	struct rt_db_internal intern;
	struct directory *dp;
	long count;
	char count_str[32];
	fastf_t tol_coll;
	fastf_t min_angle;

	WDB_TCL_CHECK_READ_ONLY;

	if (argc < 4) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_nmg_collapse %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	if (strchr(argv[2], '/')) {
		Tcl_AppendResult(interp, "Do not use '/' in solid names: ", argv[2], "\n", (char *)NULL);
		return TCL_ERROR;
	}

	new_name = argv[2];
	
	if (db_lookup(wdbp->dbip, new_name, LOOKUP_QUIET) != DIR_NULL) {
		Tcl_AppendResult(interp, new_name, " already exists\n", (char *)NULL);
		return TCL_ERROR;
	}

	if ((dp=db_lookup(wdbp->dbip, argv[1], LOOKUP_NOISY)) == DIR_NULL)
		return TCL_ERROR;

	if (dp->d_flags & DIR_COMB) {
		Tcl_AppendResult(interp, argv[1], " is a combination, only NMG solids are allowed here\n", (char *)NULL );
		return TCL_ERROR;
	}

	if (rt_db_get_internal(&intern, dp, wdbp->dbip, (matp_t)NULL, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "Failed to get internal form of ", argv[1], "!!!!\n", (char *)NULL);
		return TCL_ERROR;
	}

	if (intern.idb_type != ID_NMG) {
		Tcl_AppendResult(interp, argv[1], " is not an NMG solid!!!!\n", (char *)NULL);
		rt_db_free_internal(&intern, &rt_uniresource);
		return TCL_ERROR;
	}

	tol_coll = atof(argv[3]) * wdbp->dbip->dbi_local2base;
	if (tol_coll <= 0.0) {
		Tcl_AppendResult(interp, "tolerance distance too small\n", (char *)NULL);
		return TCL_ERROR;
	}

	if (argc == 5) {
		min_angle = atof(argv[4]);
		if (min_angle < 0.0) {
			Tcl_AppendResult(interp, "Minimum angle cannot be less than zero\n", (char *)NULL);
			return TCL_ERROR;
		}
	} else
		min_angle = 0.0;

	m = (struct model *)intern.idb_ptr;
	NMG_CK_MODEL(m);

	/* triangulate model */
	nmg_triangulate_model(m, &wdbp->wdb_tol);

	count = nmg_edge_collapse(m, &wdbp->wdb_tol, tol_coll, min_angle);

	if ((dp=db_diradd(wdbp->dbip, new_name, -1L, 0, DIR_SOLID, (genptr_t)&intern.idb_type)) == DIR_NULL) {
		Tcl_AppendResult(interp, "Cannot add ", new_name, " to directory\n", (char *)NULL);
		rt_db_free_internal(&intern, &rt_uniresource);
		return TCL_ERROR;
	}

	if (rt_db_put_internal(dp, wdbp->dbip, &intern, &rt_uniresource) < 0) {
		rt_db_free_internal(&intern, &rt_uniresource);
		WDB_TCL_WRITE_ERR_return;
	}

	rt_db_free_internal(&intern, &rt_uniresource);

	sprintf(count_str, "%ld", count);
	Tcl_AppendResult(interp, count_str, " edges collapsed\n", (char *)NULL);

	return TCL_OK;
}

/*
 * Usage:
 *        procname nmg_collapse nmg_solid new_solid maximum_error_distance [minimum_allowed_angle]
 */
static int
wdb_nmg_collapse_tcl(ClientData	clientData,
		      Tcl_Interp	*interp,
		      int		argc,
		      char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_nmg_collapse_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_summary_cmd(struct rt_wdb	*wdbp,
		Tcl_Interp	*interp,
		int		argc,
		char 		**argv)
{
	register char *cp;
	int flags = 0;
	int bad = 0;

	if (argc < 1 || 2 < argc) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_summary %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	if (argc <= 1) {
		wdb_dir_summary(wdbp->dbip, interp, 0);
		return TCL_OK;
	}

	cp = argv[1];
	while (*cp)  switch(*cp++) {
	case 's':
		flags |= DIR_SOLID;
		break;
	case 'r':
		flags |= DIR_REGION;
		break;
	case 'g':
		flags |= DIR_COMB;
		break;
	default:
		Tcl_AppendResult(interp, "summary:  S R or G are only valid parmaters\n",
				 (char *)NULL);
		bad = 1;
		break;
	}

	wdb_dir_summary(wdbp->dbip, interp, flags);
	return bad ? TCL_ERROR : TCL_OK;
}

/*
 * Usage:
 *        procname 
 */
static int
wdb_summary_tcl(ClientData	clientData,
		Tcl_Interp	*interp,
		int		argc,
		char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_summary_cmd(wdbp, interp, argc-1, argv+1);
}

int
wdb_pathlist_cmd(struct rt_wdb	*wdbp,
		 Tcl_Interp	*interp,
		 int		argc,
		 char 		**argv)
{
	if (argc < 2) {
		struct bu_vls vls;

		bu_vls_init(&vls);
		bu_vls_printf(&vls, "helplib_alias wdb_pathlist %s", argv[0]);
		Tcl_Eval(interp, bu_vls_addr(&vls));
		bu_vls_free(&vls);
		return TCL_ERROR;
	}

	if (db_walk_tree(wdbp->dbip, argc-1, (const char **)argv+1, 1,
			 &wdbp->wdb_initial_tree_state,
			 0, 0, wdb_pathlist_leaf_func, (genptr_t)interp) < 0) {
		Tcl_AppendResult(interp, "wdb_pathlist: db_walk_tree() error", (char *)NULL);
		return TCL_ERROR;
	}

	return TCL_OK;
}

/*
 * Usage:
 *        procname 
 */
static int
wdb_pathlist_tcl(ClientData	clientData,
		 Tcl_Interp	*interp,
		 int		argc,
		 char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb_pathlist_cmd(wdbp, interp, argc-1, argv+1);
}

#if 0
/* skeleton functions for wdb_obj methods */
int
wdb__cmd(struct rt_wdb	*wdbp,
	     Tcl_Interp		*interp,
	     int		argc,
	     char 		**argv)
{
}

/*
 * Usage:
 *        procname 
 */
static int
wdb__tcl(ClientData	clientData,
	 Tcl_Interp	*interp,
	 int		argc,
	 char		**argv)
{
	struct rt_wdb *wdbp = (struct rt_wdb *)clientData;

	return wdb__cmd(wdbp, interp, argc-1, argv+1);
}
#endif

/****************** utility routines ********************/

/*
 *			W D B _ C M P D I R N A M E
 *
 * Given two pointers to pointers to directory entries, do a string compare
 * on the respective names and return that value.
 *  This routine was lifted from mged/columns.c.
 */
int
wdb_cmpdirname(const genptr_t a,
	       const genptr_t b)
{
	register struct directory **dp1, **dp2;

	dp1 = (struct directory **)a;
	dp2 = (struct directory **)b;
	return( strcmp( (*dp1)->d_namep, (*dp2)->d_namep));
}

#define RT_TERMINAL_WIDTH 80
#define RT_COLUMNS ((RT_TERMINAL_WIDTH + RT_NAMESIZE - 1) / RT_NAMESIZE)

/*
 *			V L S _ C O L _ I T E M
 */
void
wdb_vls_col_item(struct bu_vls	*str,
		 register char	*cp,
		 int		*ccp,		/* column count pointer */
		 int		*clp)		/* column length pointer */
{
	/* Output newline if last column printed. */
	if (*ccp >= RT_COLUMNS || (*clp+RT_NAMESIZE-1) >= RT_TERMINAL_WIDTH) {
		/* line now full */
		bu_vls_putc(str, '\n');
		*ccp = 0;
	} else if (*ccp != 0) {
		/* Space over before starting new column */
		do {
			bu_vls_putc(str, ' ');
			++*clp;
		}  while ((*clp % RT_NAMESIZE) != 0);
	}
	/* Output string and save length for next tab. */
	*clp = 0;
	while (*cp != '\0') {
		bu_vls_putc(str, *cp);
		++cp;
		++*clp;
	}
	++*ccp;
}

/*
 */
void
wdb_vls_col_eol(struct bu_vls	*str,
		int		*ccp,
		int		*clp)
{
	if (*ccp != 0)		/* partial line */
		bu_vls_putc(str, '\n');
	*ccp = 0;
	*clp = 0;
}

/*
 *			W D B _ V L S _ C O L _ P R 4 V
 *
 *  Given a pointer to a list of pointers to names and the number of names
 *  in that list, sort and print that list in column order over four columns.
 *  This routine was lifted from mged/columns.c.
 */
void
wdb_vls_col_pr4v(struct bu_vls		*vls,
		 struct directory	**list_of_names,
		 int			num_in_list)
{
#if 0
	int lines, i, j, namelen, this_one;

	qsort((genptr_t)list_of_names,
	      (unsigned)num_in_list, (unsigned)sizeof(struct directory *),
	      (int (*)())wdb_cmpdirname);

	/*
	 * For the number of (full and partial) lines that will be needed,
	 * print in vertical format.
	 */
	lines = (num_in_list + 3) / 4;
	for (i=0; i < lines; i++) {
		for (j=0; j < 4; j++) {
			this_one = j * lines + i;
			/* Restrict the print to 16 chars per spec. */
			bu_vls_printf(vls,  "%.16s", list_of_names[this_one]->d_namep);
			namelen = strlen(list_of_names[this_one]->d_namep);
			if (namelen > 16)
				namelen = 16;
			/*
			 * Region and ident checks here....  Since the code
			 * has been modified to push and sort on pointers,
			 * the printing of the region and ident flags must
			 * be delayed until now.  There is no way to make the
			 * decision on where to place them before now.
			 */
			if (list_of_names[this_one]->d_flags & DIR_COMB) {
				bu_vls_putc(vls, '/');
				namelen++;
			}
			if (list_of_names[this_one]->d_flags & DIR_REGION) {
				bu_vls_putc(vls, 'R');
				namelen++;
			}
			/*
			 * Size check (partial lines), and line termination.
			 * Note that this will catch the end of the lines
			 * that are full too.
			 */
			if (this_one + lines >= num_in_list) {
				bu_vls_putc(vls, '\n');
				break;
			} else {
				/*
				 * Pad to next boundary as there will be
				 * another entry to the right of this one. 
				 */
				while (namelen++ < 20)
					bu_vls_putc(vls, ' ');
			}
		}
	}
#else
	int lines, i, j, k, namelen, this_one;
	int	maxnamelen;	/* longest name in list */
	int	cwidth;		/* column width */
	int	numcol;		/* number of columns */

	qsort((genptr_t)list_of_names,
	      (unsigned)num_in_list, (unsigned)sizeof(struct directory *),
	      (int (*)())wdb_cmpdirname);

	/* 
	 * Traverse the list of names, find the longest name and set the
	 * the column width and number of columns accordingly.
	 * If the longest name is greater than 80 characters, the number of columns
	 * will be one.
	 */
	maxnamelen = 0;
	for (k=0; k < num_in_list; k++) {
		namelen = strlen(list_of_names[k]->d_namep);
		if (namelen > maxnamelen)
			maxnamelen = namelen;
	}

	if (maxnamelen <= 16) 
		maxnamelen = 16;
	cwidth = maxnamelen + 4;

	if (cwidth > 80)
		cwidth = 80;
	numcol = RT_TERMINAL_WIDTH / cwidth;
     
	/*
	 * For the number of (full and partial) lines that will be needed,
	 * print in vertical format.
	 */
	lines = (num_in_list + (numcol - 1)) / numcol;
	for (i=0; i < lines; i++) {
		for (j=0; j < numcol; j++) {
			this_one = j * lines + i;
			bu_vls_printf(vls, "%s", list_of_names[this_one]->d_namep);
			namelen = strlen( list_of_names[this_one]->d_namep);

			/*
			 * Region and ident checks here....  Since the code
			 * has been modified to push and sort on pointers,
			 * the printing of the region and ident flags must
			 * be delayed until now.  There is no way to make the
			 * decision on where to place them before now.
			 */
			if (list_of_names[this_one]->d_flags & DIR_COMB) {
				bu_vls_putc(vls, '/');
				namelen++;
			}

			if (list_of_names[this_one]->d_flags & DIR_REGION) {
				bu_vls_putc(vls, 'R');
				namelen++;
			}

			/*
			 * Size check (partial lines), and line termination.
			 * Note that this will catch the end of the lines
			 * that are full too.
			 */
			if (this_one + lines >= num_in_list) {
				bu_vls_putc(vls, '\n');
				break;
			} else {
				/*
				 * Pad to next boundary as there will be
				 * another entry to the right of this one. 
				 */
				while( namelen++ < cwidth)
					bu_vls_putc(vls, ' ');
			}
		}
	}
#endif
}

void
wdb_vls_long_dpp(struct bu_vls		*vls,
		 struct directory	**list_of_names,
		 int			num_in_list,
		 int			aflag,		/* print all objects */
		 int			cflag,		/* print combinations */
		 int			rflag,		/* print regions */
		 int			sflag)		/* print solids */
{
	int i;
	int isComb, isRegion;
	int isSolid;
	const char *type;
	int max_nam_len = 0;
	int max_type_len = 0;
	struct directory *dp;

	qsort((genptr_t)list_of_names,
	      (unsigned)num_in_list, (unsigned)sizeof(struct directory *),
	      (int (*)())wdb_cmpdirname);

	for (i=0 ; i < num_in_list ; i++) {
		int len;

		dp = list_of_names[i];
		len = strlen(dp->d_namep);
		if (len > max_nam_len)
			max_nam_len = len;

		if (dp->d_flags & DIR_REGION)
			len = 6;
		else if (dp->d_flags & DIR_COMB)
			len = 4;
		else if (dp->d_major_type == DB5_MAJORTYPE_ATTRIBUTE_ONLY)
			len = 6;
		else
			len = strlen(rt_functab[dp->d_minor_type].ft_label);

		if (len > max_type_len)
			max_type_len = len;
	}

	/*
	 * i - tracks the list item
	 */
	for (i=0; i < num_in_list; ++i) {
		if (list_of_names[i]->d_flags & DIR_COMB) {
			isComb = 1;
			isSolid = 0;
			type = "comb";

			if (list_of_names[i]->d_flags & DIR_REGION) {
				isRegion = 1;
				type = "region";
			} else
				isRegion = 0;
		} else {
			isComb = isRegion = 0;
			isSolid = 1;
			type = rt_functab[list_of_names[i]->d_minor_type].ft_label;
		}

		if (list_of_names[i]->d_major_type == DB5_MAJORTYPE_ATTRIBUTE_ONLY) {
			isSolid = 0;
			type = "global";
		}

		/* print list item i */
		dp = list_of_names[i];
		if (aflag ||
		    (!cflag && !rflag && !sflag) ||
		    (cflag && isComb) ||
		    (rflag && isRegion) ||
		    (sflag && isSolid)) {
			bu_vls_printf(vls, "%s", dp->d_namep );
			bu_vls_spaces(vls, max_nam_len - strlen( dp->d_namep ) );
			bu_vls_printf(vls, " %s", type );
			bu_vls_spaces(vls, max_type_len - strlen( type ) );
			bu_vls_printf(vls,  " %2d %2d %d\n",
				      dp->d_major_type, dp->d_minor_type, dp->d_len);
		}
	}
}

/*
 *			W D B _ V L S _ L I N E _ D P P
 *
 *  Given a pointer to a list of pointers to names and the number of names
 *  in that list, sort and print that list on the same line.
 *  This routine was lifted from mged/columns.c.
 */
void
wdb_vls_line_dpp(struct bu_vls	*vls,
		 struct directory **list_of_names,
		 int		num_in_list,
		 int		aflag,	/* print all objects */
		 int		cflag,	/* print combinations */
		 int		rflag,	/* print regions */
		 int		sflag)	/* print solids */
{
	int i;
	int isComb, isRegion;
	int isSolid;

	qsort( (genptr_t)list_of_names,
	       (unsigned)num_in_list, (unsigned)sizeof(struct directory *),
	       (int (*)())wdb_cmpdirname);

	/*
	 * i - tracks the list item
	 */
	for (i=0; i < num_in_list; ++i) {
		if (list_of_names[i]->d_flags & DIR_COMB) {
			isComb = 1;
			isSolid = 0;

			if (list_of_names[i]->d_flags & DIR_REGION)
				isRegion = 1;
			else
				isRegion = 0;
		} else {
			isComb = isRegion = 0;
			isSolid = 1;
		}

		/* print list item i */
		if (aflag ||
		    (!cflag && !rflag && !sflag) ||
		    (cflag && isComb) ||
		    (rflag && isRegion) ||
		    (sflag && isSolid)) {
			bu_vls_printf(vls,  "%s ", list_of_names[i]->d_namep);
		}
	}
}

/*
 *			W D B _ G E T S P A C E
 *
 * This routine walks through the directory entry list and mallocs enough
 * space for pointers to hold:
 *  a) all of the entries if called with an argument of 0, or
 *  b) the number of entries specified by the argument if > 0.
 *  This routine was lifted from mged/dir.c.
 */
struct directory **
wdb_getspace(struct db_i	*dbip,
	     register int	num_entries)
{
	register struct directory **dir_basep;

	if (num_entries < 0) {
		bu_log("wdb_getspace: was passed %d, used 0\n",
		       num_entries);
		num_entries = 0;
	}

	if (num_entries == 0)  num_entries = db_get_directory_size(dbip);

	/* Allocate and cast num_entries worth of pointers */
	dir_basep = (struct directory **) bu_malloc((num_entries+1) * sizeof(struct directory *),
						    "wdb_getspace *dir[]" );
	return(dir_basep);
}

/*
 *			W D B _ D O _ L I S T
 */
void
wdb_do_list(struct db_i		*dbip,
	    Tcl_Interp		*interp,
	    struct bu_vls	*outstrp,
	    register struct directory *dp,
	    int			verbose)
{
	int			id;
	struct rt_db_internal	intern;

	RT_CK_DBI(dbip);

	if( dp->d_major_type == DB5_MAJORTYPE_ATTRIBUTE_ONLY ) {
		/* this is the _GLOBAL object */
		struct bu_attribute_value_set avs;
		struct bu_attribute_value_pair	*avp;

		bu_vls_strcat( outstrp, dp->d_namep );
		bu_vls_strcat( outstrp, ": global attributes object\n" );
		if( db5_get_attributes( dbip, &avs, dp ) ) {
			Tcl_AppendResult(interp, "Cannot get attributes for ", dp->d_namep,
					 "\n", (char *)NULL );
			return;
		}
		for( BU_AVS_FOR( avp, &avs ) ) {
			if( !strcmp( avp->name, "units" ) ) {
				double conv;
				const char *str;

				conv = atof( avp->value );
				bu_vls_strcat( outstrp, "\tunits: " );
				if( (str=bu_units_string( conv ) ) == NULL ) {
					bu_vls_strcat( outstrp, "Unrecognized units\n" );
				} else {
					bu_vls_strcat( outstrp, str );
					bu_vls_putc( outstrp, '\n' );
				}
			} else {
				bu_vls_putc( outstrp, '\t' );
				bu_vls_strcat( outstrp, avp->name );
				bu_vls_strcat( outstrp, ": " );
				bu_vls_strcat( outstrp, avp->value );
				bu_vls_putc( outstrp, '\n' );
			}
		}
	} else {

		if ((id = rt_db_get_internal(&intern, dp, dbip,
					     (fastf_t *)NULL, &rt_uniresource)) < 0) {
			Tcl_AppendResult(interp, "rt_db_get_internal(", dp->d_namep,
					 ") failure\n", (char *)NULL);
			return;
		}

		bu_vls_printf(outstrp, "%s:  ", dp->d_namep);
		
		if (rt_functab[id].ft_describe(outstrp, &intern,
					       verbose, dbip->dbi_base2local, &rt_uniresource) < 0)
			Tcl_AppendResult(interp, dp->d_namep, ": describe error\n", (char *)NULL);
		rt_db_free_internal(&intern, &rt_uniresource);
	}
}

/*
 *			W D B _ C O M B A D D
 *
 * Add an instance of object 'objp' to combination 'name'.
 * If the combination does not exist, it is created.
 * region_flag is 1 (region), or 0 (group).
 *
 *  Preserves the GIFT semantics.
 */
struct directory *
wdb_combadd(Tcl_Interp			*interp,
	    struct db_i			*dbip,
	    register struct directory	*objp,
	    char			*combname,
	    int				region_flag,	/* true if adding region */
	    int				relation,	/* = UNION, SUBTRACT, INTERSECT */
	    int				ident,		/* "Region ID" */
	    int				air,		/* Air code */
	    struct rt_wdb		*wdbp)
{
	register struct directory *dp;
	struct rt_db_internal intern;
	struct rt_comb_internal *comb;
	union tree *tp;
	struct rt_tree_array *tree_list;
	int node_count;
	int actual_count;

	/*
	 * Check to see if we have to create a new combination
	 */
	if ((dp = db_lookup(dbip,  combname, LOOKUP_QUIET)) == DIR_NULL) {
		int flags;

		if (region_flag)
			flags = DIR_REGION | DIR_COMB;
		else
			flags = DIR_COMB;

		RT_INIT_DB_INTERNAL(&intern);
		intern.idb_type = ID_COMBINATION;
		intern.idb_meth = &rt_functab[ID_COMBINATION];

		/* Update the in-core directory */
		if ((dp = db_diradd(dbip, combname, -1, 0, flags, (genptr_t)&intern.idb_type)) == DIR_NULL)  {
			Tcl_AppendResult(interp, "An error has occured while adding '",
					 combname, "' to the database.\n", (char *)NULL);
			return DIR_NULL;
		}

		BU_GETSTRUCT(comb, rt_comb_internal);
		intern.idb_ptr = (genptr_t)comb;
		comb->magic = RT_COMB_MAGIC;
		bu_vls_init(&comb->shader);
		bu_vls_init(&comb->material);
		comb->region_id = -1;
		comb->tree = TREE_NULL;

		if (region_flag) {
			struct bu_vls tmp_vls;

			comb->region_flag = 1;
			comb->region_id = ident;
			comb->aircode = air;
			comb->los = wdbp->wdb_los_default;
			comb->GIFTmater = wdbp->wdb_mat_default;
			bu_vls_init(&tmp_vls);
			bu_vls_printf(&tmp_vls,
				      "Creating region id=%d, air=%d, GIFTmaterial=%d, los=%d\n",
				      ident, air, 
					wdbp->wdb_mat_default,
					wdbp->wdb_los_default);
			Tcl_AppendResult(interp, bu_vls_addr(&tmp_vls), (char *)NULL);
			bu_vls_free(&tmp_vls);
		}

		RT_GET_TREE( tp, &rt_uniresource );
		tp->magic = RT_TREE_MAGIC;
		tp->tr_l.tl_op = OP_DB_LEAF;
		tp->tr_l.tl_name = bu_strdup( objp->d_namep );
		tp->tr_l.tl_mat = (matp_t)NULL;
		comb->tree = tp;

		if (rt_db_put_internal(dp, dbip, &intern, &rt_uniresource) < 0) {
			Tcl_AppendResult(interp, "Failed to write ", dp->d_namep, (char *)NULL );
			return DIR_NULL;
		}
		return dp;
	} else if (!(dp->d_flags & DIR_COMB)) {
		Tcl_AppendResult(interp, combname, " exists, but is not a combination\n", (char *)NULL);
		return DIR_NULL;
	}

	/* combination exists, add a new member */
	if (rt_db_get_internal(&intern, dp, dbip, (fastf_t *)NULL, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "read error, aborting\n", (char *)NULL);
		return DIR_NULL;
	}

	comb = (struct rt_comb_internal *)intern.idb_ptr;
	RT_CK_COMB(comb);

	if (region_flag && !comb->region_flag) {
		Tcl_AppendResult(interp, combname, ": not a region\n", (char *)NULL);
		return DIR_NULL;
	}

	if (comb->tree && db_ck_v4gift_tree(comb->tree) < 0) {
		db_non_union_push(comb->tree, &rt_uniresource);
		if (db_ck_v4gift_tree(comb->tree) < 0) {
			Tcl_AppendResult(interp, "Cannot flatten tree for editing\n", (char *)NULL);
			rt_db_free_internal(&intern, &rt_uniresource);
			return DIR_NULL;
		}
	}

	/* make space for an extra leaf */
	node_count = db_tree_nleaves( comb->tree ) + 1;
	tree_list = (struct rt_tree_array *)bu_calloc( node_count,
						       sizeof( struct rt_tree_array ), "tree list" );

	/* flatten tree */
	if (comb->tree) {
		actual_count = 1 + (struct rt_tree_array *)db_flatten_tree(
			tree_list, comb->tree, OP_UNION, 1, &rt_uniresource )
			- tree_list;
		BU_ASSERT_LONG( actual_count, ==, node_count );
		comb->tree = TREE_NULL;
	}

	/* insert new member at end */
	switch (relation) {
	case '+':
		tree_list[node_count - 1].tl_op = OP_INTERSECT;
		break;
	case '-':
		tree_list[node_count - 1].tl_op = OP_SUBTRACT;
		break;
	default:
		Tcl_AppendResult(interp, "unrecognized relation (assume UNION)\n",
				 (char *)NULL );
	case 'u':
		tree_list[node_count - 1].tl_op = OP_UNION;
		break;
	}

	/* make new leaf node, and insert at end of list */
	RT_GET_TREE( tp, &rt_uniresource );
	tree_list[node_count-1].tl_tree = tp;
	tp->tr_l.magic = RT_TREE_MAGIC;
	tp->tr_l.tl_op = OP_DB_LEAF;
	tp->tr_l.tl_name = bu_strdup( objp->d_namep );
	tp->tr_l.tl_mat = (matp_t)NULL;

	/* rebuild the tree */
	comb->tree = (union tree *)db_mkgift_tree( tree_list, node_count, &rt_uniresource );

	/* and finally, write it out */
	if (rt_db_put_internal(dp, dbip, &intern, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "Failed to write ", dp->d_namep, (char *)NULL);
		return DIR_NULL;
	}

	bu_free((char *)tree_list, "combadd: tree_list");

	return (dp);
}

static void
wdb_do_identitize(struct db_i		*dbip,
		  struct rt_comb_internal *comb,
		  union tree		*comb_leaf,
		  genptr_t		user_ptr1,
		  genptr_t		user_ptr2,
		  genptr_t		user_ptr3)
{
	struct directory *dp;
	Tcl_Interp *interp = (Tcl_Interp *)user_ptr1;

	RT_CK_DBI(dbip);
	RT_CK_TREE(comb_leaf);

	if (!comb_leaf->tr_l.tl_mat) {
		comb_leaf->tr_l.tl_mat = (matp_t)bu_malloc(sizeof(mat_t), "tl_mat");
	}
	MAT_IDN(comb_leaf->tr_l.tl_mat);
	if ((dp = db_lookup(dbip, comb_leaf->tr_l.tl_name, LOOKUP_NOISY)) == DIR_NULL)
		return;

	wdb_identitize(dp, dbip, interp);
}

/*
 *			W D B _ I D E N T I T I Z E ( ) 
 *
 *	Traverses an objects paths, setting all member matrices == identity
 *
 */
void
wdb_identitize(struct directory	*dp,
	       struct db_i	*dbip,
	       Tcl_Interp	*interp)
{
	struct rt_db_internal intern;
	struct rt_comb_internal *comb;

	if (dp->d_flags & DIR_SOLID)
		return;
	if (rt_db_get_internal(&intern, dp, dbip, (fastf_t *)NULL, &rt_uniresource) < 0) {
		Tcl_AppendResult(interp, "Database read error, aborting\n", (char *)NULL);
		return;
	}
	comb = (struct rt_comb_internal *)intern.idb_ptr;
	if (comb->tree) {
		db_tree_funcleaf(dbip, comb, comb->tree, wdb_do_identitize,
				 (genptr_t)interp, (genptr_t)NULL, (genptr_t)NULL);
		if (rt_db_put_internal(dp, dbip, &intern, &rt_uniresource) < 0) {
			Tcl_AppendResult(interp, "Cannot write modified combination (", dp->d_namep,
					 ") to database\n", (char *)NULL );
			return;
		}
	}
}

/*
 *  			W D B _ D I R _ S U M M A R Y
 *
 * Summarize the contents of the directory by categories
 * (solid, comb, region).  If flag is != 0, it is interpreted
 * as a request to print all the names in that category (eg, DIR_SOLID).
 */
static void
wdb_dir_summary(struct db_i	*dbip,
		Tcl_Interp	*interp,
		int		flag)
{
	register struct directory *dp;
	register int i;
	static int sol, comb, reg;
	struct directory **dirp;
	struct directory **dirp0 = (struct directory **)NULL;
	struct bu_vls vls;

	bu_vls_init(&vls);

	sol = comb = reg = 0;
	for (i = 0; i < RT_DBNHASH; i++)  {
		for (dp = dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw) {
			if (dp->d_flags & DIR_SOLID)
				sol++;
			if (dp->d_flags & DIR_COMB) {
				if (dp->d_flags & DIR_REGION)
					reg++;
				else
					comb++;
			}
		}
	}

	bu_vls_printf(&vls, "Summary:\n");
	bu_vls_printf(&vls, "  %5d solids\n", sol);
	bu_vls_printf(&vls, "  %5d region; %d non-region combinations\n", reg, comb);
	bu_vls_printf(&vls, "  %5d total objects\n\n", sol+reg+comb );

	if (flag == 0) {
		Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
		bu_vls_free(&vls);
		return;
	}

	/* Print all names matching the flags parameter */
	/* THIS MIGHT WANT TO BE SEPARATED OUT BY CATEGORY */
	
	dirp = wdb_dir_getspace(dbip, 0);
	dirp0 = dirp;
	/*
	 * Walk the directory list adding pointers (to the directory entries
	 * of interest) to the array
	 */
	for (i = 0; i < RT_DBNHASH; i++)
		for(dp = dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw)
			if (dp->d_flags & flag)
				*dirp++ = dp;

	wdb_vls_col_pr4v(&vls, dirp0, (int)(dirp - dirp0));
	Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
	bu_vls_free(&vls);
	bu_free((genptr_t)dirp0, "dir_getspace");
}

/*
 *			W D B _ D I R _ G E T S P A C E
 *
 * This routine walks through the directory entry list and mallocs enough
 * space for pointers to hold:
 *  a) all of the entries if called with an argument of 0, or
 *  b) the number of entries specified by the argument if > 0.
 */
static struct directory **
wdb_dir_getspace(struct db_i	*dbip,
		 register int	num_entries)
{
	register struct directory *dp;
	register int i;
	register struct directory **dir_basep;

	if (num_entries < 0) {
		bu_log( "dir_getspace: was passed %d, used 0\n",
			num_entries);
		num_entries = 0;
	}
	if (num_entries == 0) {
		/* Set num_entries to the number of entries */
		for (i = 0; i < RT_DBNHASH; i++)
			for(dp = dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw)
				num_entries++;
	}

	/* Allocate and cast num_entries worth of pointers */
	dir_basep = (struct directory **) bu_malloc((num_entries+1) * sizeof(struct directory *),
						    "dir_getspace *dir[]");
	return dir_basep;
}

/*
 *			P A T H L I S T _ L E A F _ F U N C
 */
static union tree *
wdb_pathlist_leaf_func(struct db_tree_state	*tsp,
		       struct db_full_path	*pathp,
		       struct rt_db_internal	*ip,
		       genptr_t			client_data)
{
	Tcl_Interp	*interp = (Tcl_Interp *)client_data;
	char		*str;

	RT_CK_FULL_PATH(pathp);
	RT_CK_DB_INTERNAL(ip);

	str = db_path_to_string(pathp);

	Tcl_AppendElement(interp, str);

	bu_free((genptr_t)str, "path string");
	return TREE_NULL;
}
