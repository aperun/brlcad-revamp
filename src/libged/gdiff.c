/*                         G D I F F . C
 * BRL-CAD
 *
 * Copyright (c) 2014 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file libged/gdiff.c
 *
 * The gdiff command.
 *
 */

#include "common.h"

#include <string.h>

#include "bu/cmd.h"
#include "bu/getopt.h"
#include "analyze.h"

#include "./ged_private.h"

HIDDEN const char *
gdiff_usage()
{
    return NULL;
}

int
ged_gdiff(struct ged *gedp, int argc, const char *argv[])
{
    size_t i;
    struct analyze_raydiff_results *results;
    struct bn_tol tol = {BN_TOL_MAGIC, BN_TOL_DIST, BN_TOL_DIST * BN_TOL_DIST, 1.0e-6, 1.0 - 1.0e-6 };
/* these get set in the while and used immediately after, but the use is commented out.
    int left_dbip_specified = 0;
    int right_dbip_specified = 0;
*/
    int c = 0;
    int do_diff_raytrace = 0;
    int view_left = 0;
    int view_right = 0;
    int view_overlap = 0;
    struct bu_vls tmpstr = BU_VLS_INIT_ZERO;
    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    if (argc < 3) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s", gdiff_usage());
	return GED_ERROR;
    }

    /* skip command name */
    bu_optind = 1;
    bu_opterr = 1;

    /* parse args */
    while ((c=bu_getopt(argc, (char * const *)argv, "O:N:vhRlrb?")) != -1) {
	if (bu_optopt == '?')
	    c='h';
	switch (c) {
	    case 'O' :
/*	use is commented out after this while
		left_dbip_specified = 1;
*/
		bu_vls_sprintf(&tmpstr, "%s", bu_optarg);
		/*bu_log("Have origin database: %s", bu_vls_addr(&tmpstr));*/
		break;
	    case 'N' :
/*	use is commented out after this while
		right_dbip_specified = 1;
*/
		bu_vls_sprintf(&tmpstr, "%s", bu_optarg);
		/*bu_log("Have new database: %s", bu_vls_addr(&tmpstr));*/
		break;
	    case 'v' :
		/*bu_log("Reporting mode is verbose");*/
		break;
	    case 'R' :
		do_diff_raytrace = 1;
		/*bu_log("Raytrace based evaluation of differences between objects.");*/
		break;
	    case 'l' :
		view_left = 1;
		break;
	    case 'b' :
		view_overlap = 1;
		break;
	    case 'r' :
		view_right = 1;
		break;
	    default:
		bu_vls_printf(gedp->ged_result_str, "Usage: %s", gdiff_usage());
		return GED_ERROR;
	}
    }

    /* There are possible convention-based interpretations of 1, 2, 3, 4 and n args
     * beyond those used as options.  For the shortest cases, the interpretation depends
     * on whether one or two .g files are known:
     *
     * a) No .g file specified
     * 1 - objname (error)
     * 1 - file.g (error)
     * 2 - file.g objname (error)
     * 2 - obj1 obj2 (error)
     * 3 - file.g obj1 obj2 (diff two objects in file.g)
     *
     * b) Current .g file known
     * 1 - objname (error)
     * 1 - file.g (diff full .g file contents)
     * 2 - file.g objname (diff obj between current.g and file.g)
     * 2 - obj1 obj2 (diff two objects in current .g)
     * 3 - file.g obj1 obj2 (diff all listed objects between current.g and file.g)
     *
     * .g file args must always come first.
     *
     * A maximum of two .g files can be specified - any additional specification
     * of .g files is an error.
     *
     * When only one environment is specified, all other args must define pairs of
     * objects to compare.
     *
     * When two environments are known, all args will be compared by their instances
     * in the first environment and the second, not against each other in either
     * environment.
     *
     * When there is a current .g environment and two additional .g files are
     * specified, the argv environments will override use of the "current" .g environment.

!!! If this is uncommented, uncomment the left_dbip_specified and right_dbip_specified
!!! definitions, as well as where they're set in the getopt while()
     if ((argc - bu_optind) == 2) {
	 bu_log("left: %s", argv[bu_optind]);
	 bu_log("right: %s", argv[bu_optind+1]);
     } else {
	if ((argc - bu_optind) == 1) {
	    if (left_dbip_specified || right_dbip_specified)
		bu_log("obj_name: %s", argv[bu_optind]);
	} else {
	    bu_vls_printf(gedp->ged_result_str, "Usage: %s", gdiff_usage());
	    return GED_ERROR;
	}
     }
     */


    if ((argc - bu_optind) != 2) {
	return GED_ERROR;
    }
/*
    bu_log("left: %s", argv[bu_optind]);
    bu_log("right: %s", argv[bu_optind+1]);
    */
    if (do_diff_raytrace) {
	analyze_raydiff(&results, gedp->ged_wdbp->dbip, argv[bu_optind], argv[bu_optind+1], &tol);

	/* TODO - may want to integrate with a "regular" diff and report intelligently.  Needs
	 * some thought. */
	if (BU_PTBL_LEN(results->left) > 0 || BU_PTBL_LEN(results->right) > 0) {
	    bu_vls_printf(gedp->ged_result_str, "1");
	} else {
	    bu_vls_printf(gedp->ged_result_str, "0");
	}

	/* For now, graphical output is the main output of this mode, so if we don't have any
	 * specifics do all */
	if (!view_left && !view_overlap && !view_right) {
	    view_left = 1;
	    view_right = 1;
	    view_overlap = 1;
	}

	if (view_left || view_overlap || view_right) {
	    /* Visualize the differences */
	    struct bu_list *vhead;
	    point_t a, b;
	    struct bn_vlblock *vbp;
	    struct bu_list local_vlist;
	    BU_LIST_INIT(&local_vlist);
	    vbp = bn_vlblock_init(&local_vlist, 32);

	    /* Clear any previous diff drawing */
	    if (db_lookup(gedp->ged_wdbp->dbip, "diff_visualff", LOOKUP_QUIET) != RT_DIR_NULL)
		dl_erasePathFromDisplay(gedp->ged_gdp->gd_headDisplay, gedp->ged_wdbp->dbip, gedp->ged_free_vlist_callback, "diff_visualff", 1, gedp->freesolid);
	    if (db_lookup(gedp->ged_wdbp->dbip, "diff_visualff0000", LOOKUP_QUIET) != RT_DIR_NULL)
		dl_erasePathFromDisplay(gedp->ged_gdp->gd_headDisplay, gedp->ged_wdbp->dbip, gedp->ged_free_vlist_callback, "diff_visualff0000", 1, gedp->freesolid);
	    if (db_lookup(gedp->ged_wdbp->dbip, "diff_visualffffff", LOOKUP_QUIET) != RT_DIR_NULL)
		dl_erasePathFromDisplay(gedp->ged_gdp->gd_headDisplay, gedp->ged_wdbp->dbip, gedp->ged_free_vlist_callback, "diff_visualffffff", 1, gedp->freesolid);

	    /* Draw left-only lines */
	    if (view_left) {
		for (i = 0; i < BU_PTBL_LEN(results->left); i++) {
		    struct diff_seg *dseg = (struct diff_seg *)BU_PTBL_GET(results->left, i);
		    VMOVE(a, dseg->in_pt);
		    VMOVE(b, dseg->out_pt);
		    vhead = bn_vlblock_find(vbp, 255, 0, 0); /* should be red */
		    BN_ADD_VLIST(vbp->free_vlist_hd, vhead, a, BN_VLIST_LINE_MOVE);
		    BN_ADD_VLIST(vbp->free_vlist_hd, vhead, b, BN_VLIST_LINE_DRAW);
		}
	    }
	    /* Draw overlap lines */
	    if (view_overlap) {
		for (i = 0; i < BU_PTBL_LEN(results->both); i++) {
		    struct diff_seg *dseg = (struct diff_seg *)BU_PTBL_GET(results->both, i);
		    VMOVE(a, dseg->in_pt);
		    VMOVE(b, dseg->out_pt);
		    vhead = bn_vlblock_find(vbp, 255, 255, 255); /* should be white */
		    BN_ADD_VLIST(vbp->free_vlist_hd, vhead, a, BN_VLIST_LINE_MOVE);
		    BN_ADD_VLIST(vbp->free_vlist_hd, vhead, b, BN_VLIST_LINE_DRAW);

		}
	    }
	    /* Draw right lines */
	    if (view_right) {
		for (i = 0; i < BU_PTBL_LEN(results->right); i++) {
		    struct diff_seg *dseg = (struct diff_seg *)BU_PTBL_GET(results->right, i);
		    VMOVE(a, dseg->in_pt);
		    VMOVE(b, dseg->out_pt);
		    vhead = bn_vlblock_find(vbp, 0, 0, 255); /* should be blue */
		    BN_ADD_VLIST(vbp->free_vlist_hd, vhead, a, BN_VLIST_LINE_MOVE);
		    BN_ADD_VLIST(vbp->free_vlist_hd, vhead, b, BN_VLIST_LINE_DRAW);
		}
	    }

	    _ged_cvt_vlblock_to_solids(gedp, vbp, "diff_visual", 0);

	    bn_vlist_cleanup(&local_vlist);
	    bn_vlblock_free(vbp);
	}
	analyze_raydiff_results_free(results);
    }

    return GED_OK;
}


/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
