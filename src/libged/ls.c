/*                         L S . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2018 United States Government as represented by
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
/** @file libged/ls.c
 *
 * The ls command.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <string.h>

#include "bu/cmd.h"
#include "bu/getopt.h"
#include "bu/sort.h"
#include "bu/units.h"

#include "./ged_private.h"


static void
vls_long_dpp(struct ged *gedp,
	     struct directory **list_of_names,
	     int num_in_list,
	     int aflag,		/* print all objects */
	     int cflag,		/* print combinations */
	     int rflag,		/* print regions */
	     int sflag,		/* print solids */
	     int hflag,		/* use human readable units for size */
	     int ssflag)        /* sort by object size */
{
    int i;
    int isComb=0, isRegion=0;
    int isSolid=0;
    const char *type=NULL;
    size_t max_nam_len = 0;
    size_t max_type_len = 0;
    struct directory *dp;

    if (!ssflag) {
	bu_sort((void *)list_of_names,
		(unsigned)num_in_list, (unsigned)sizeof(struct directory *),
		cmpdirname, NULL);
    } else {
	bu_sort((void *)list_of_names,
		(unsigned)num_in_list, (unsigned)sizeof(struct directory *),
		cmpdlen, NULL);
    }

    for (i = 0; i < num_in_list; i++) {
	size_t len;

	dp = list_of_names[i];
	len = strlen(dp->d_namep);
	if (len > max_nam_len)
	    max_nam_len = len;

	if (dp->d_flags & RT_DIR_REGION)
	    len = 6; /* "region" */
	else if (dp->d_flags & RT_DIR_COMB)
	    len = 4; /* "comb" */
	else if (dp->d_flags & RT_DIR_SOLID) {
	    struct rt_db_internal intern;
	    len = 9; /* "primitive" */
	    if (rt_db_get_internal(&intern, dp, gedp->ged_wdbp->dbip, (fastf_t *)NULL, &rt_uniresource) >= 0) {
		len = strlen(intern.idb_meth->ft_label);
		rt_db_free_internal(&intern);
	    }
	} else {
	    switch (list_of_names[i]->d_major_type) {
		case DB5_MAJORTYPE_ATTRIBUTE_ONLY:
		    len = 6;
		    break;
		case DB5_MAJORTYPE_BINARY_MIME:
		    len = strlen("binary (mime)");
		    break;
		case DB5_MAJORTYPE_BINARY_UNIF:
		    len = strlen(binu_types[list_of_names[i]->d_minor_type]);
		    break;
	    }
	}

	if (len > max_type_len)
	    max_type_len = len;
    }

    /*
     * i - tracks the list item
     */
    for (i = 0; i < num_in_list; ++i) {
	dp = list_of_names[i];

	if (dp->d_flags & RT_DIR_COMB) {
	    isComb = 1;
	    isSolid = 0;
	    type = "comb";

	    if (dp->d_flags & RT_DIR_REGION) {
		isRegion = 1;
		type = "region";
	    } else
		isRegion = 0;
	} else if (dp->d_flags & RT_DIR_SOLID) {
	    struct rt_db_internal intern;
	    type = "primitive";
	    if (rt_db_get_internal(&intern, dp, gedp->ged_wdbp->dbip, (fastf_t *)NULL, &rt_uniresource) >= 0) {
		type = intern.idb_meth->ft_label;
		rt_db_free_internal(&intern);
	    }
	    isComb = isRegion = 0;
	    isSolid = 1;
	} else {
	    switch (dp->d_major_type) {
		case DB5_MAJORTYPE_ATTRIBUTE_ONLY:
		    isSolid = 0;
		    type = "global";
		    break;
		case DB5_MAJORTYPE_BINARY_MIME:
		    isSolid = 0;
		    isRegion = 0;
		    type = "binary(mime)";
		    break;
		case DB5_MAJORTYPE_BINARY_UNIF:
		    isSolid = 0;
		    isRegion = 0;
		    type = binu_types[dp->d_minor_type];
		    break;
	    }
	}

	/* print list item i */
	if (aflag ||
	    (!cflag && !rflag && !sflag) ||
	    (cflag && isComb) ||
	    (rflag && isRegion) ||
	    (sflag && isSolid)) {
	    bu_vls_printf(gedp->ged_result_str, "%s", dp->d_namep);
	    bu_vls_spaces(gedp->ged_result_str, (int)(max_nam_len - strlen(dp->d_namep)));
	    bu_vls_printf(gedp->ged_result_str, " %s", type);
	    if (type)
	       bu_vls_spaces(gedp->ged_result_str, (int)(max_type_len - strlen(type)));
	    bu_vls_printf(gedp->ged_result_str,  " %2d %2d ", dp->d_major_type, dp->d_minor_type);
	    if (!hflag) {
		bu_vls_printf(gedp->ged_result_str,  "%ld\n", (long)(dp->d_len));
	    } else {
		char hlen[6] = { '\0' };
		(void)bu_humanize_number(hlen, 5, (int64_t)dp->d_len, "",
			BU_HN_AUTOSCALE,
			BU_HN_B | BU_HN_NOSPACE | BU_HN_DECIMAL);
		bu_vls_printf(gedp->ged_result_str,  " %s\n", hlen);
	    }
	}
    }
}


/**
 * Given a pointer to a list of pointers to names and the number of names
 * in that list, sort and print that list on the same line.
 */
static void
vls_line_dpp(struct ged *gedp,
	     struct directory **list_of_names,
	     int num_in_list,
	     int aflag,	/* print all objects */
	     int cflag,	/* print combinations */
	     int rflag,	/* print regions */
	     int sflag,	/* print solids */
	     int ssflag) /* sort by size */
{
    int i;
    int isComb, isRegion;
    int isSolid;

    if (!ssflag) {
	bu_sort((void *)list_of_names,
		(unsigned)num_in_list, (unsigned)sizeof(struct directory *),
		cmpdirname, NULL);
    } else {
	bu_sort((void *)list_of_names,
		(unsigned)num_in_list, (unsigned)sizeof(struct directory *),
		cmpdlen, NULL);
    }

    /*
     * i - tracks the list item
     */
    for (i = 0; i < num_in_list; ++i) {
	if (list_of_names[i]->d_flags & RT_DIR_COMB) {
	    isComb = 1;
	    isSolid = 0;

	    if (list_of_names[i]->d_flags & RT_DIR_REGION)
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
	    bu_vls_printf(gedp->ged_result_str,  "%s ", list_of_names[i]->d_namep);
	    _ged_results_add(gedp->ged_results, list_of_names[i]->d_namep);
	}
    }
}


/**
 * List objects in this database
 */
int
ged_ls(struct ged *gedp, int argc, const char *argv[])
{
    struct directory *dp;
    size_t i;
    int c;
    int aflag = 0;		/* print all objects without formatting */
    int cflag = 0;		/* print combinations */
    int rflag = 0;		/* print regions */
    int sflag = 0;		/* print solids */
    int lflag = 0;		/* use long format */
    int qflag = 0;		/* quiet flag - do a quiet lookup */
    int hflag = 0;		/* use human readable units for size in long format */
    int ssflag = 0;		/* sort by size in long format */
    int attr_flag = 0;		/* arguments are attribute name/value pairs */
    int or_flag = 0;		/* flag indicating that any one attribute match is sufficient
				 * default is all attributes must match.
				 */
    struct directory **dirp;
    struct directory **dirp0 = (struct directory **)NULL;
    static const char *usage = "[-A name/value pairs] OR [-acrslopq] object(s)";

    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);
    ged_results_clear(gedp->ged_results);

    bu_optind = 1;	/* re-init bu_getopt() */
    while ((c = bu_getopt(argc, (char * const *)argv, "acrslopqAHS")) != -1) {
	switch (c) {
	    case 'A':
		attr_flag = 1;
		break;
	    case 'o':
		or_flag = 1;
		break;
	    case 'a':
		aflag = 1;
		break;
	    case 'c':
		cflag = 1;
		break;
	    case 'q':
		qflag = 1;
		break;
	    case 'r':
		rflag = 1;
		break;
	    case 's':
	    case 'p':
		sflag = 1;
		break;
	    case 'l':
		lflag = 1;
		break;
	    case 'H':
		hflag = 1;
		break;
	    case 'S':
		ssflag = 1;
		break;
	    default:
		bu_vls_printf(gedp->ged_result_str, "Unrecognized option - %c", c);
		_ged_results_add(gedp->ged_results, bu_vls_addr(gedp->ged_result_str));
		return GED_ERROR;
	}
    }
    /* skip options processed plus command name */
    argc -= bu_optind;
    argv += bu_optind;

    /* create list of selected objects from database */
    if (attr_flag) {
	/* select objects based on attributes */
	struct bu_ptbl *tbl;
	struct bu_attribute_value_set avs;
	int dir_flags;
	int op;
	if ((argc < 2) || (argc%2 != 0)) {
	    /* should be even number of name/value pairs */
	    bu_log("ls -A option expects even number of 'name value' pairs\n");
	    bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	    _ged_results_add(gedp->ged_results, bu_vls_addr(gedp->ged_result_str));
	    return TCL_ERROR;
	}

	if (or_flag) {
	    op = 2;
	} else {
	    op = 1;
	}

	dir_flags = 0;
	if (aflag) dir_flags = -1;
	if (cflag) dir_flags = RT_DIR_COMB;
	if (sflag) dir_flags = RT_DIR_SOLID;
	if (rflag) dir_flags = RT_DIR_REGION;
	if (!dir_flags) dir_flags = -1 ^ RT_DIR_HIDDEN;

	bu_avs_init(&avs, argc, "wdb_ls_cmd avs");
	for (i = 0; i < (size_t)argc; i += 2) {
	    if (or_flag) {
		bu_avs_add_nonunique(&avs, (char *)argv[i], (char *)argv[i+1]);
	    } else {
		bu_avs_add(&avs, (char *)argv[i], (char *)argv[i+1]);
	    }
	}

	tbl = db_lookup_by_attr(gedp->ged_wdbp->dbip, dir_flags, &avs, op);
	bu_avs_free(&avs);

	dirp = _ged_getspace(gedp->ged_wdbp->dbip, BU_PTBL_LEN(tbl));
	dirp0 = dirp;
	for (i = 0; i < BU_PTBL_LEN(tbl); i++) {
	    *dirp++ = (struct directory *)BU_PTBL_GET(tbl, i);
	}

	bu_ptbl_free(tbl);
	bu_free((char *)tbl, "wdb_ls_cmd ptbl");
    } else if (argc > 0) {
	/* Just list specified names */
	dirp = _ged_getspace(gedp->ged_wdbp->dbip, argc);
	dirp0 = dirp;
	/*
	 * Verify the names, and add pointers to them to the array.
	 */
	for (i = 0; i < (size_t)argc; i++) {
	    if (qflag) {
		dp = db_lookup(gedp->ged_wdbp->dbip, argv[i], LOOKUP_QUIET);
	    } else {
		dp = db_lookup(gedp->ged_wdbp->dbip, argv[i], LOOKUP_NOISY);
	    }
	    if (dp  == RT_DIR_NULL)
		continue;
	    *dirp++ = dp;
	}
    } else {
	/* Full table of contents */
	dirp = _ged_getspace(gedp->ged_wdbp->dbip, 0);	/* Enough for all */
	dirp0 = dirp;
	/*
	 * Walk the directory list adding pointers (to the directory
	 * entries) to the array.
	 */
	for (i = 0; i < RT_DBNHASH; i++)
	    for (dp = gedp->ged_wdbp->dbip->dbi_Head[i]; dp != RT_DIR_NULL; dp = dp->d_forw) {
		if (!aflag && (dp->d_flags & RT_DIR_HIDDEN))
		    continue;
		*dirp++ = dp;
	    }
    }

    if (lflag)
	vls_long_dpp(gedp, dirp0, (int)(dirp - dirp0), aflag, cflag, rflag, sflag, hflag, ssflag);
    else if (aflag || cflag || rflag || sflag)
	vls_line_dpp(gedp, dirp0, (int)(dirp - dirp0), aflag, cflag, rflag, sflag, ssflag);
    else {
	_ged_vls_col_pr4v(gedp->ged_result_str, dirp0, (int)(dirp - dirp0), 0, ssflag);
	_ged_results_add(gedp->ged_results, bu_vls_addr(gedp->ged_result_str));
    }

    bu_free((void *)dirp0, "_ged_getspace dp[]");

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
