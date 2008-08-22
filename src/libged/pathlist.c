/*                         P A T H L I S T . C
 * BRL-CAD
 *
 * Copyright (c) 2008 United States Government as represented by
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
/** @file pathlist.c
 *
 * The pathlist command.
 *
 */

#include "common.h"

#include <string.h>

#include "bio.h"
#include "cmd.h"
#include "ged_private.h"

static union tree *
ged_pathlist_leaf_func(struct db_tree_state	*tsp,
		       struct db_full_path	*pathp,
		       struct rt_db_internal	*ip,
		       genptr_t			client_data);

static int pathListNoLeaf = 0;

int
ged_pathlist(struct ged *gedp, int argc, const char *argv[])
{
    static const char *usage = "name";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);
    gedp->ged_result = GED_RESULT_NULL;
    gedp->ged_result_flags = 0;

    /* must be wanting help */
    if (argc == 1) {
	gedp->ged_result_flags |= GED_RESULT_FLAGS_HELP_BIT;
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_OK;
    }

    if (3 < argc) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    pathListNoLeaf = 0;

    if (argc == 3) {
	if (!strcmp(argv[1], "-noleaf"))
	    pathListNoLeaf = 1;

	++argv;
	--argc;
    }

    if (db_walk_tree(gedp->ged_wdbp->dbip, argc-1, (const char **)argv+1, 1,
		     &gedp->ged_wdbp->wdb_initial_tree_state,
		     0, 0, ged_pathlist_leaf_func, (genptr_t)gedp) < 0) {
	bu_vls_printf(&gedp->ged_result_str, "ged_pathlist: db_walk_tree() error");
	return BRLCAD_ERROR;
    }

    return BRLCAD_OK;
}

/*
 *			P A T H L I S T _ L E A F _ F U N C
 */
static union tree *
ged_pathlist_leaf_func(struct db_tree_state	*tsp,
		       struct db_full_path	*pathp,
		       struct rt_db_internal	*ip,
		       genptr_t			client_data)
{
    struct ged *gedp = (struct ged *)client_data;
    char *str;

    RT_CK_FULL_PATH(pathp);
    RT_CK_DB_INTERNAL(ip);

    if (pathListNoLeaf) {
	--pathp->fp_len;
	str = db_path_to_string(pathp);
	++pathp->fp_len;
    } else
	str = db_path_to_string(pathp);

    bu_vls_printf(&gedp->ged_result_str, " %s", str);

    bu_free((genptr_t)str, "path string");
    return TREE_NULL;
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
