/*                     O N _ B R E P . C P P
 * BRL-CAD
 *
 * Copyright (c) 2013 United States Government as represented by
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
/** @file ON_Brep.cpp
 *
 * File for writing out an ON_Brep structure into the STEPcode containers
 *
 */


// Make entity arrays for each of the m_V, m_S, etc arrays and create step instances of them,
// starting with the basic ones.
//
// The array indices in the ON_Brep will correspond to the step entity array locations that
// hold the step version of each entity.
//
// then, need to map the ON_Brep hierarchy to the corresponding STEP hierarchy
//
// brep -> advanced_brep_shape_representation
//         manifold_solid_brep
// faces-> closed_shell
//         advanced_face
//
// surface-> bspline_surface_with_knots
//           cartesian_point
//
// outer 3d trim loop ->  face_outer_bound
//                        edge_loop
//                        oriented_edge
//                        edge_curve
//                        bspline_curve_with_knots
//                        vertex_point
//                        cartesian_point
//
// 2d points -> point_on_surface
// 1d points to bound curve -> point_on_curve
//
// 2d trimming curves -> pcurve using point_on_surface? almost doesn't look as if there is a good AP203 way to represent 2d trimming curves...
//
//
// Note that STEPentity is the same thing as SDAI_Application_instance... see src/clstepcore/sdai.h line 220
//

#include "STEPEntity.h"

void
ON_3dPoint_to_Cartesian_point(ON_3dPoint *inpnt, SdaiCartesian_point *step_pnt) {
	RealAggregate_ptr coord_vals = step_pnt->coordinates_();
	RealNode *xnode = new RealNode();
	xnode->value = inpnt->x;
	coord_vals->AddNode(xnode);
	RealNode *ynode = new RealNode();
	ynode->value = inpnt->y;
	coord_vals->AddNode(ynode);
	RealNode *znode = new RealNode();
	znode->value = inpnt->z;
	coord_vals->AddNode(znode);
}

void
ON_3dVector_to_Direction(ON_3dVector *invect, SdaiDirection *step_direction) {
	invect->Unitize();
	RealAggregate_ptr coord_vals = step_direction->direction_ratios_();
	RealNode *xnode = new RealNode();
	xnode->value = invect->x;
	coord_vals->AddNode(xnode);
	RealNode *ynode = new RealNode();
	ynode->value = invect->y;
	coord_vals->AddNode(ynode);
	RealNode *znode = new RealNode();
	znode->value = invect->z;
	coord_vals->AddNode(znode);
}

void
ON_NurbsCurveCV_to_EntityAggregate(ON_NurbsCurve *incrv, SdaiB_spline_curve *step_crv, Registry *registry, InstMgr *instance_list) {
	EntityAggregate *control_pnts = step_crv->control_points_list_();
	ON_3dPoint cv_pnt;
	for (int i = 0; i < incrv->CVCount(); i++) {
		SdaiCartesian_point *step_cartesian = (SdaiCartesian_point *)registry->ObjCreate("CARTESIAN_POINT");
		instance_list->Append(step_cartesian, completeSE);
		incrv->GetCV(i, cv_pnt);
		ON_3dPoint_to_Cartesian_point(&(cv_pnt), step_cartesian);
		control_pnts->AddNode(new EntityNode((SDAI_Application_instance *)step_cartesian));
	}
}
#if 0
void
ON_RationalNurbsCurve_to_EntityAggregate(ON_NurbsCurve *incrv, SdaiRational_B_spline_curve *step_crv) {
}
#endif

bool ON_BRep_to_STEP(ON_Brep *brep, Registry *registry, InstMgr *instance_list)
{
	std::vector<STEPentity *> cartesian_pnts;
	std::vector<STEPentity *> vertex_pnts;
	std::vector<STEPentity *> three_dimensional_curves;
	std::vector<STEPentity *> edge_curves;
	std::vector<STEPentity *> oriented_edges;
	std::vector<STEPentity *> edge_loops;
	std::vector<STEPentity *> outer_bounds;
	std::vector<STEPentity *> surfaces;
	std::vector<STEPentity *> faces;
	//STEPentity *closed_shell = new STEPentity;
	//STEPentity *manifold_solid_brep = new STEPentity;
	//STEPentity *advanced_brep = new STEPentity;

	/* Set initial container capacities */
	cartesian_pnts.resize(brep->m_V.Count(), NULL);
	vertex_pnts.resize(brep->m_V.Count(), NULL);
	three_dimensional_curves.resize(brep->m_C3.Count(), NULL);
	edge_curves.resize(brep->m_E.Count(), NULL);
	oriented_edges.resize(2*(brep->m_E.Count()), NULL);
	edge_loops.resize(brep->m_L.Count(), NULL);
	outer_bounds.resize(brep->m_F.Count(), NULL);
	surfaces.resize(brep->m_S.Count(), NULL);
	faces.resize(brep->m_F.Count(), NULL);

        // Set up vertices and associated cartesian points
	for (int i = 0; i < brep->m_V.Count(); ++i) {
                // Cartesian points (actual 3D geometry)
		cartesian_pnts.at(i) = registry->ObjCreate("CARTESIAN_POINT");
		instance_list->Append(cartesian_pnts.at(i), completeSE);
		ON_3dPoint v_pnt = brep->m_V[i].Point();
		ON_3dPoint_to_Cartesian_point(&(v_pnt), (SdaiCartesian_point *)cartesian_pnts.at(i));
                // Vertex points (topological, references actual 3D geometry)
		vertex_pnts.at(i) = registry->ObjCreate("VERTEX_POINT");
		((SdaiVertex_point *)vertex_pnts.at(i))->vertex_geometry_((const SdaiPoint_ptr)cartesian_pnts.at(i));
		instance_list->Append(vertex_pnts.at(i), completeSE);
	}

	// 3D curves
	for (int i = 0; i < brep->m_C3.Count(); ++i) {
		int curve_converted = 0;
		ON_Curve* curve = brep->m_C3[i];
		/* Supported curve types */
		ON_ArcCurve *a_curve = ON_ArcCurve::Cast(curve);
		ON_LineCurve *l_curve = ON_LineCurve::Cast(curve);
		ON_NurbsCurve *n_curve = ON_NurbsCurve::Cast(curve);
		ON_PolyCurve *p_curve = ON_PolyCurve::Cast(curve);

		if (a_curve && !curve_converted) {
			std::cout << "Have ArcCurve\n";
		}
		if (l_curve && !curve_converted) {
			std::cout << "Have LineCurve\n";
			ON_Line *m_line = &(l_curve->m_line);
			/* In STEP, a line consists of a cartesian point and a 3D vector.  Since
			 * it does not appear that OpenNURBS data structures reference m_V points
			 * for these constructs, create our own */
			three_dimensional_curves.at(i) = registry->ObjCreate("LINE");
			SdaiLine *curr_line = (SdaiLine *)three_dimensional_curves.at(i);
			curr_line->pnt_((SdaiCartesian_point *)registry->ObjCreate("CARTESIAN_POINT"));
			ON_3dPoint_to_Cartesian_point(&(m_line->from), curr_line->pnt_());
			curr_line->dir_((SdaiVector *)registry->ObjCreate("VECTOR"));
			SdaiVector *curr_dir = curr_line->dir_();
			curr_dir->orientation_((SdaiDirection *)registry->ObjCreate("DIRECTION"));
			ON_3dVector on_dir = m_line->Direction();
			ON_3dVector_to_Direction(&(on_dir), curr_line->dir_()->orientation_());
			curr_line->dir_()->magnitude_(m_line->Length());
			instance_list->Append(curr_line->pnt_(), completeSE);
			instance_list->Append(curr_dir->orientation_(), completeSE);
			instance_list->Append(curr_line->dir_(), completeSE);
			instance_list->Append(three_dimensional_curves.at(i), completeSE);
		}
		if (p_curve && !curve_converted) {
			std::cout << "Have PolyCurve\n";
		}
		if (n_curve && !curve_converted) {
			std::cout << "Have NurbsCurve\n";
			//MakePiecewiseBezier
			if (n_curve->IsRational()) {
				three_dimensional_curves.at(i) = registry->ObjCreate("RATIONAL_B_SPLINE_CURVE");
			} else {
				three_dimensional_curves.at(i) = registry->ObjCreate("B_SPLINE_CURVE");
				SdaiB_spline_curve *curr_curve = (SdaiB_spline_curve *)three_dimensional_curves.at(i);
				curr_curve->degree_(n_curve->Degree());
				ON_NurbsCurveCV_to_EntityAggregate(n_curve, curr_curve, registry, instance_list);
			}

			instance_list->Append(three_dimensional_curves.at(i), completeSE);
		}

		/* Whatever this is, if it's not a supported type and it does have
		 * a NURBS form, use that */

	}

	return true;
}
