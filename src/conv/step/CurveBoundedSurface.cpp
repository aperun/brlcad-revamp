/*                 CurveBoundedSurface.cpp
 * BRL-CAD
 *
 * Copyright (c) 1994-2009 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file CurveBoundedSurface.cpp
 *
 * Routines to interface to STEP "CurveBoundedSurface".
 *
 */

#include "STEPWrapper.h"
#include "Factory.h"

#include "Surface.h"
#include "BoundaryCurve.h"
#include "CurveBoundedSurface.h"

#define CLASSNAME "CurveBoundedSurface"
#define ENTITYNAME "Curve_Bounded_Surface"
string CurveBoundedSurface::entityname = Factory::RegisterClass(ENTITYNAME,(FactoryMethod)CurveBoundedSurface::Create);

CurveBoundedSurface::CurveBoundedSurface() {
	step=NULL;
	id = 0;
	basis_surface = NULL;
}

CurveBoundedSurface::CurveBoundedSurface(STEPWrapper *sw,int STEPid) {
	step=sw;
	id = STEPid;
	basis_surface = NULL;
}

CurveBoundedSurface::~CurveBoundedSurface() {
	boundaries.clear();
}

bool
CurveBoundedSurface::Load(STEPWrapper *sw, SCLP23(Application_instance) *sse) {

	step=sw;
	id = sse->STEPfile_id;

	if ( !BoundedSurface::Load(step,sse) ) {
		cout << CLASSNAME << ":Error loading base class ::BoundedSurface." << endl;
		return false;
	}

	// need to do this for local attributes to makes sure we have
	// the actual entity and not a complex/supertype parent
	sse = step->getEntity(sse,ENTITYNAME);

	if (basis_surface == NULL) {
		SCLP23(Application_instance) *entity = step->getEntityAttribute(sse,"basis_surface");
		if (entity) {
			basis_surface = dynamic_cast<Surface *>(Factory::CreateObject(sw,entity));
		} else {
			cerr << CLASSNAME << ": error loading 'basis_surface' attribute." << endl;
			return false;
		}
	}

	if (boundaries.empty()) {
		LIST_OF_ENTITIES *l = step->getListOfEntities(sse,"boundaries");
		LIST_OF_ENTITIES::iterator i;
		for(i=l->begin();i!=l->end();i++) {
			SCLP23(Application_instance) *entity = (*i);
			if (entity) {
				BoundaryCurve *aAF = dynamic_cast<BoundaryCurve *>(Factory::CreateObject(sw,entity));

				boundaries.push_back(aAF);
			} else {
				cerr << CLASSNAME  << ": Unhandled entity in attribute 'cfs_faces'." << endl;
				return false;
			}
		}
		l->clear();
		delete l;
	}

	implicit_outer = step->getBooleanAttribute(sse,"implicit_outer");

	return true;
}

void
CurveBoundedSurface::Print(int level) {
	TAB(level); cout << CLASSNAME << ":" << name << "(";
	cout << "ID:" << STEPid() << ")" << endl;

	TAB(level); cout << "Attributes:" << endl;
	basis_surface->Print(level+1);

	TAB(level+1); cout << "boundaries:" << endl;
	LIST_OF_BOUNDARIES::iterator i;
	for(i=boundaries.begin(); i != boundaries.end(); ++i) {
		(*i)->Print(level+1);
	}

	TAB(level+1); cout << "implicit_outer:" << step->getBooleanString((SCLBOOL_H(Bool))implicit_outer) << endl;

	TAB(level); cout << "Inherited Attributes:" << endl;
	BoundedSurface::Print(level+1);
}

STEPEntity *
CurveBoundedSurface::Create(STEPWrapper *sw, SCLP23(Application_instance) *sse) {
	Factory::OBJECTS::iterator i;
	if ((i = Factory::FindObject(sse->STEPfile_id)) == Factory::objects.end()) {
		CurveBoundedSurface *object = new CurveBoundedSurface(sw,sse->STEPfile_id);

		Factory::AddObject(object);

		if (!object->Load(sw, sse)) {
			cerr << CLASSNAME << ":Error loading class in ::Create() method." << endl;
			delete object;
			return NULL;
		}
		return static_cast<STEPEntity *>(object);
	} else {
		return (*i).second;
	}
}

bool
CurveBoundedSurface::LoadONBrep(ON_Brep *brep)
{
	cerr << "Error: ::LoadONBrep(ON_Brep *brep) not implemented for " << entityname << endl;
	return false;
}


// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
