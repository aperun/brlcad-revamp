/*                    R H C _ B R E P . C P P
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
/** @file rhc_brep.cpp
 *
 * Convert a Right Hyperbolic Cylinder to b-rep form
 *
 */

#include "common.h"

#include "raytrace.h"
#include "rtgeom.h"
#include "brep.h"



/**
 *			R T _ R H C _ B R E P
 */
extern "C" void
rt_rhc_brep(ON_Brep **b, const struct rt_db_internal *ip, const struct bn_tol *tol)
{
   
    struct rt_rhc_internal	*eip;

    *b = NULL; 

    RT_CK_DB_INTERNAL(ip);
    eip = (struct rt_rhc_internal *)ip->idb_ptr;
    RT_RHC_CK_MAGIC(eip);

    *b = new ON_Brep();


    ON_TextLog dump_to_stdout;
    ON_TextLog* dump = &dump_to_stdout;
    
    point_t p1_origin, p2_origin;
    ON_3dPoint plane1_origin, plane2_origin;
    ON_3dVector plane_x_dir, plane_y_dir;
    
    //  First, find plane in 3 space corresponding to the  bottom face
    //  of the RHC.  
   
    vect_t tmp, x_dir, y_dir;
    
    VCROSS(x_dir, eip->rhc_H, eip->rhc_B);
    VUNITIZE(x_dir);
    VSCALE(x_dir, x_dir, eip->rhc_r);
    VMOVE(y_dir, eip->rhc_B);

    VMOVE(p1_origin, eip->rhc_V);
    plane1_origin = ON_3dPoint(p1_origin);
    plane_x_dir = ON_3dVector(x_dir);
    plane_y_dir = ON_3dVector(y_dir);
    const ON_Plane* rhc_bottom_plane = new ON_Plane(plane1_origin, plane_x_dir, plane_y_dir); 
   
    //  Next, create a hyperbolic curve corresponding to the shape of
    //  the hyperboloid in the plane.  Need the two end points and
    //  need to solve for the focus of the hyperbola
    point_t x_rev_dir, ep1, ep2, ep3, focus;
    VREVERSE(x_rev_dir, x_dir);

    VADD2(ep1, p1_origin, x_rev_dir);
    VADD2(ep2, p1_origin, eip->rhc_B);
    VADD2(ep3, p1_origin, x_dir);
    ON_3dPoint onp1 = ON_3dPoint(ep1);
    ON_3dPoint onp2 = ON_3dPoint(ep2);
    ON_3dPoint onp3 = ON_3dPoint(ep3);


    ON_NurbsCurve* hypnurbscurve = ON_NurbsCurve::New(3,true,3,3);
    hypnurbscurve->SetKnot(0, 0);
    hypnurbscurve->SetKnot(1, 0);
    hypnurbscurve->SetKnot(2, 1);
    hypnurbscurve->SetKnot(3, 1);
    hypnurbscurve->SetCV(0,ON_3dPoint(ep1));
    hypnurbscurve->SetCV(1,ON_3dPoint(ep2));
    hypnurbscurve->SetCV(2,ON_3dPoint(ep3));
//    bu_log("Valid nurbs curve: %d\n", hypnurbscurve->IsValid(dump));
//    hypnurbscurve->Dump(*dump);

    // Also need a staight line from the beginning to the end to
    // complete the loop

    ON_LineCurve* straightedge = new ON_LineCurve(onp3,onp1);   
   
    // Generate the bottom cap 
    ON_SimpleArray<ON_Curve*> boundary;
    boundary.Append(ON_Curve::Cast(hypnurbscurve)); 
    boundary.Append(ON_Curve::Cast(straightedge)); 
    
    ON_PlaneSurface* bp = new ON_PlaneSurface();
    bp->m_plane = (*rhc_bottom_plane);
    bp->SetDomain(0, -100.0, 100.0 );
    bp->SetDomain(1, -100.0, 100.0 );
    bp->SetExtents(0, bp->Domain(0) );
    bp->SetExtents(1, bp->Domain(1) );
    (*b)->m_S.Append(bp);
    const int bsi = (*b)->m_S.Count() - 1;
    ON_BrepFace& bface = (*b)->NewFace(bsi);
    (*b)->NewPlanarFaceLoop(bface.m_face_index, ON_BrepLoop::outer, boundary, true); 
    const ON_BrepLoop* bloop = (*b)->m_L.Last();
    bp->SetDomain(0, bloop->m_pbox.m_min.x, bloop->m_pbox.m_max.x );
    bp->SetDomain(1, bloop->m_pbox.m_min.y, bloop->m_pbox.m_max.y );
    bp->SetExtents(0,bp->Domain(0));
    bp->SetExtents(1,bp->Domain(1));
    (*b)->SetTrimIsoFlags(bface);

    // Now the side face and top cap - extrude the bottom face and set the cap flag to true
    vect_t vp2;
    VADD2(vp2,eip->rhc_V, eip->rhc_H);
    const ON_Curve* extrudepath = new ON_LineCurve(ON_3dPoint(eip->rhc_V), ON_3dPoint(vp2));
    ON_Brep& brep = *(*b);
    ON_BrepExtrudeFace(brep,0, *extrudepath, true);
  
}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8

