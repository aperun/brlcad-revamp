/*                 Unit.h
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
/** @file Unit.h
 *
 * Class definition used to convert STEP "Unit" to BRL-CAD BREP
 * structures.
 *
 */

#ifndef UNIT_H_
#define UNIT_H_

#include "STEPEntity.h"

class Unit : public STEPEntity {
private:
	static string entityname;

protected:

public:
	Unit();
	virtual ~Unit();
	Unit(STEPWrapper *sw,int STEPid);
	bool Load(STEPWrapper *sw,SCLP23(Application_instance) *sse);
	virtual void Print(int level);

	//static methods
	static STEPEntity *Create(STEPWrapper *sw,SCLP23(Application_instance) *sse);
};

#endif /* UNIT_H_ */
