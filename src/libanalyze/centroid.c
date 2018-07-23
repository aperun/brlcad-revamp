/*                      C E N T R O I D . C
 * BRL-CAD
 *
 * Copyright (c) 2015-2018 United States Government as represented by
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
/** @file centroid.c
 *
 * Libanalyze utility to calculate the centroid using generic
 * method.
 *
 */

#include "common.h"

#include "analyze.h"
#include "./analyze_private.h"

point_t *
analyze_centroid(struct current_state *state, const char *name)
{
    point_t *cent;
    int i, view, obj = 0;
    double avg_mass;

    for (i = 0; i < state->num_objects; i++) {
	if(!(bu_strcmp(state->objs[i].o_name, name))) {
	    obj = i;
	    break;
	}
    }

    BU_GET(cent, point_t);
    avg_mass = 0.0;
    for (view = 0; view < state->num_views; view++)
	avg_mass += state->objs[obj].o_mass[view];

    avg_mass /= state->num_views;
    if (!ZERO(avg_mass)) {
	point_t centroid = VINIT_ZERO;
	fastf_t inv_total_mass = 1.0/avg_mass;

	for (view = 0; view < state->num_views; view++) {
	    vect_t torque;
	    fastf_t cell_area = state->area[view] / state->shots[view];

	    VSCALE(torque, &state->objs[obj].o_lenTorque[view*3], cell_area);
	    VADD2(centroid, centroid, torque);
	}

	VSCALE(centroid, centroid, 1.0/(fastf_t)state->num_views);
	VSCALE(centroid, centroid, inv_total_mass);
	(*cent)[0] = centroid[X];
	(*cent)[1] = centroid[Y];
	(*cent)[2] = centroid[Z];
    }
    return cent;
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
