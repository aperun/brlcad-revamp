/*                 Ellipse.cpp
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
/** @file Curve.cpp
 *
 * Routines to convert STEP "Curve" to BRL-CAD BREP
 * structures.
 *
 */

#include "STEPWrapper.h"
#include "Factory.h"

#include "Vertex.h"
#include "Ellipse.h"

#define CLASSNAME "Ellipse"
#define ENTITYNAME "Ellipse"
string Ellipse::entityname = Factory::RegisterClass(ENTITYNAME,(FactoryMethod)Ellipse::Create);

Ellipse::Ellipse() {
	step = NULL;
	id = 0;
}

Ellipse::Ellipse(STEPWrapper *sw,int STEPid) {
	step = sw;
	id = STEPid;
}

Ellipse::~Ellipse() {
}

bool
Ellipse::Load(STEPWrapper *sw,SCLP23(Application_instance) *sse) {
	step=sw;
	id = sse->STEPfile_id;

	if ( !Conic::Load(step,sse) ) {
		cout << CLASSNAME << ":Error loading base class ::Conic." << endl;
		return false;
	}

	// need to do this for local attributes to makes sure we have
	// the actual entity and not a complex/supertype parent
	sse = step->getEntity(sse,ENTITYNAME);

	semi_axis_1 = step->getRealAttribute(sse,"semi_axis_1");
	semi_axis_2 = step->getRealAttribute(sse,"semi_axis_2");

	return true;
}

void
Ellipse::Print(int level) {
	TAB(level); cout << CLASSNAME << ":" << name << "(";
	cout << "ID:" << STEPid() << ")" << endl;

	TAB(level); cout << "Attributes:" << endl;
	TAB(level+1); cout << "semi_axis_1:" << semi_axis_1 << endl;
	TAB(level+1); cout << "semi_axis_2:" << semi_axis_2 << endl;

	TAB(level); cout << "Inherited Attributes:" << endl;
	Conic::Print(level);
}

STEPEntity *
Ellipse::Create(STEPWrapper *sw,SCLP23(Application_instance) *sse){
	Factory::OBJECTS::iterator i;
	if ((i = Factory::FindObject(sse->STEPfile_id)) == Factory::objects.end()) {
		Ellipse *object = new Ellipse(sw,sse->STEPfile_id);

		Factory::AddObject(object);

		if (!object->Load(sw,sse)) {
			cerr << CLASSNAME << ":Error loading class in ::Create() method." << endl;
			delete object;
			return NULL;
		}
		return static_cast<STEPEntity *>(object);
	} else {
		return (*i).second;
	}
}
