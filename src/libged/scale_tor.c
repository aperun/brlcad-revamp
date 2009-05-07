/*                         S C A L E _ T O R . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2009 United States Government as represented by
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
/** @file scale_tor.c
 *
 * The scale_tor command.
 *
 */

#include "common.h"

#include <string.h>
#include "bio.h"

#include "cmd.h"
#include "rtgeom.h"
#include "raytrace.h"

#include "./ged_private.h"


int
ged_scale_tor(struct ged *gedp, int argc, const char *argv[])
{
    struct rt_db_internal intern;
    struct rt_tor_internal *tor;
    fastf_t sf;
    mat_t mat;
    char *last;
    struct directory *dp;
    static const char *usage = "tor a|h sf";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_READ_ONLY(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 4) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    if (argv[2][1] != '\0') {
	bu_vls_printf(&gedp->ged_result_str, "bad tor attribute - %s", argv[2]);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[3], "%lf", &sf) != 1 ||
	sf <= SQRT_SMALL_FASTF) {
	bu_vls_printf(&gedp->ged_result_str, "bad scale factor - %s", argv[3]);
	return BRLCAD_ERROR;
    }

    if ((last = strrchr(argv[1], '/')) == NULL)
	last = (char *)argv[1];
    else
	++last;

    if (last[0] == '\0') {
	bu_vls_printf(&gedp->ged_result_str, "illegal input - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    if ((dp = db_lookup(gedp->ged_wdbp->dbip, last, LOOKUP_QUIET)) == DIR_NULL) {
	bu_vls_printf(&gedp->ged_result_str, "%s not found", argv[1]);
	return BRLCAD_ERROR;
    }

    if (wdb_import_from_path2(&gedp->ged_result_str, &intern, argv[1], gedp->ged_wdbp, mat) == BRLCAD_ERROR)
	return BRLCAD_ERROR;

    if (intern.idb_major_type != DB5_MAJORTYPE_BRLCAD ||
	intern.idb_minor_type != DB5_MINORTYPE_BRLCAD_TOR) {
	bu_vls_printf(&gedp->ged_result_str, "Object not an TOR");
	rt_db_free_internal(&intern, &rt_uniresource);

	return BRLCAD_ERROR;
    }

    tor = (struct rt_tor_internal *)intern.idb_ptr;
    RT_TOR_CK_MAGIC(tor);

    switch (argv[2][0]) {
    case 'a':
    case 'A':
	tor->r_a *= sf;
	if (tor->r_a < SMALL)
	    tor->r_a = 4*SMALL;
	break;
    case 'h':
    case 'H':
	tor->r_h *= sf;
	if (tor->r_h < SMALL)
	    tor->r_h = 4*SMALL;
	break;
    default:
	bu_vls_printf(&gedp->ged_result_str, "bad tor attribute - %s", argv[2]);
	rt_db_free_internal(&intern, &rt_uniresource);

	return BRLCAD_ERROR;
    }

    GED_DB_PUT_INTERNAL(gedp, dp, &intern, &rt_uniresource, BRLCAD_ERROR);

    return BRLCAD_OK;
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
