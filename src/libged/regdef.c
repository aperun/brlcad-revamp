/*                        R E G D E F . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2016 United States Government as represented by
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
/** @file libged/regdef.c
 *
 * The regdef command.
 *
 */

#include "common.h"
#include <stdlib.h>
#include "ged.h"

int
ged_regdef(struct ged *gedp, int argc, const char *argv[])
{
    int item, air, los, mat;
    static const char *usage = "item air los mat";

    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
    GED_CHECK_READ_ONLY(gedp, GED_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    /* Get region defaults */
    if (argc == 1) {
	bu_vls_printf(gedp->ged_result_str, "ident %d air %d los %d material %d",
		      gedp->ged_wdbp->wdb_item_default,
		      gedp->ged_wdbp->wdb_air_default,
		      gedp->ged_wdbp->wdb_los_default,
		      gedp->ged_wdbp->wdb_mat_default);
	return GED_OK;
    }

    if (argc < 2 || 5 < argc) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_ERROR;
    }

    if (sscanf(argv[1], "%d", &item) != 1) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_ERROR;
    }
    gedp->ged_wdbp->wdb_item_default = item;

    if (argc == 2)
	return GED_OK;

    if (sscanf(argv[2], "%d", &air) != 1) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_ERROR;
    }
    gedp->ged_wdbp->wdb_air_default = air;
    if (air) {
	item = 0;
	gedp->ged_wdbp->wdb_item_default = 0;
    }

    if (argc == 3)
	return GED_OK;

    if (sscanf(argv[3], "%d", &los) != 1) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_ERROR;
    }
    gedp->ged_wdbp->wdb_los_default = los;

    if (argc == 4)
	return GED_OK;

    if (sscanf(argv[4], "%d", &mat) != 1) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_ERROR;
    }
    gedp->ged_wdbp->wdb_mat_default = mat;

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
