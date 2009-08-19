/*                    E L L _ B R E P . C P P
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
/** @file ell_brep.cpp
 *
 * Convert a Generalized Ellipsoid to b-rep form
 *
 */

#include "common.h"

#include "raytrace.h"
#include "rtgeom.h"
#include "brep.h"



/**
 *			R T _ E T O _ B R E P
 */
extern "C" void
rt_eto_brep(ON_Brep **b, const struct rt_db_internal *ip, const struct bn_tol *tol)
{
    mat_t	R;
    mat_t	S;
    struct rt_eto_internal	*eip;

    *b = NULL; 

    RT_CK_DB_INTERNAL(ip);
    eip = (struct rt_eto_internal *)ip->idb_ptr;
    RT_ETO_CK_MAGIC(eip);

    point_t p_origin;
    vect_t v1, x_dir, y_dir;
    ON_3dPoint plane_origin, plane_x_dir, plane_y_dir;

    double ell_axis_len_1, ell_axis_len_2;
    
    //  First, find a plane in 3 space with both x and y axis
    //  along an axis of the ellipse to be rotated, and its
    //  coordinate origin at the center of the ellipse.
    //
    //  To identify a point on the eto suitable for use (there
    //  are of course infinitely many such points described by
    //  a circle at radius eto_r from the eto vertex) obtain
    //  a vector at a right angle to the eto normal, unitize it
    //  and scale it.

    bn_vec_ortho( v1, eip->eto_N );
    VUNITIZE( v1 );
    VSCALE(v1, v1, eip->eto_r);
    VSET(p_origin, v1[0], v1[1], v1[2]);
    VMOVE(x_dir, eip->eto_C);
    VUNITIZE( x_dir );
    bn_vec_ortho( y_dir, eip->eto_C );
    VUNITIZE( y_dir );
    plane_origin = ON_3dPoint(p_origin);
    plane_x_dir = ON_3dVector(x_dir);
    plane_y_dir = ON_3dVector(y_dir);
   
    const ON_Plane* ell_plane = new ON_Plane(plane_origin, plane_x_dir, plane_y_dir); 
    
    
   
    //  Once the plane has been created, create the ellipse
    //  within the plane.
    ell_axis_len_1 = MAGNITUDE(eip->eto_C);
    ell_axis_len_2 = eip->eto_rd;
    ON_Ellipse* ellipse = new ON_Ellipse(ell_plane, ell_axis_len_1, ell_axis_len_2);


    //  Generate an ON_Curve from the ellipse and revolve it
    //  around eto_N 
 
    ON_NurbsCurve* ellcurve = ellipse->GetNurbsForm();
    ON_Line* revaxis = eip->eto_V, eip->eto_N; 
    
	ON_RevSurface* eto_surf = ON_RevSurface::New();
    eto_surf->m_curve = ellcurve;
    eto_surf->m_axis = revaxis;
    eto_surf->m_angle = angle;
    eto_surf->m_t = t;

    /* Create brep with one face*/
    *b = ON_Brep::New();
    (*b)->NewFace(*brep_ell_surf(unit2model));
//    (*b)->Standardize();
 //   (*b)->Compact();
}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8

