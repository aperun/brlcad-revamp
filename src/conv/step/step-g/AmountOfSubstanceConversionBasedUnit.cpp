/*                 AmountOfSubstanceConversionBasedUnit.cpp
 * BRL-CAD
 *
 * Copyright (c) 1994-2016 United States Government as represented by
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
/** @file step/AmountOfSubstanceConversionBasedUnit.cpp
 *
 * Routines to convert STEP "AmountOfSubstanceConversionBasedUnit" to BRL-CAD BREP
 * structures.
 *
 */

#include "STEPWrapper.h"
#include "Factory.h"

#include "MeasureValue.h"
#include "Unit.h"
#include "MeasureWithUnit.h"
#include "AmountOfSubstanceConversionBasedUnit.h"

#define CLASSNAME "AmountOfSubstanceConversionBasedUnit"
#define ENTITYNAME "Amount_Of_Substance_Conversion_Based_Unit"
string AmountOfSubstanceConversionBasedUnit::entityname = Factory::RegisterClass(ENTITYNAME, (FactoryMethod)AmountOfSubstanceConversionBasedUnit::Create);

AmountOfSubstanceConversionBasedUnit::AmountOfSubstanceConversionBasedUnit()
{
    step = NULL;
    id = 0;
}

AmountOfSubstanceConversionBasedUnit::AmountOfSubstanceConversionBasedUnit(STEPWrapper *sw, int step_id)
{
    step = sw;
    id = step_id;
}

AmountOfSubstanceConversionBasedUnit::~AmountOfSubstanceConversionBasedUnit()
{
}

bool
AmountOfSubstanceConversionBasedUnit::Load(STEPWrapper *sw, SDAI_Application_instance *sse)
{
    step = sw;
    id = sse->STEPfile_id;


    // load base class attributes
    if (!AmountOfSubstanceUnit::Load(step, sse)) {
	std::cout << CLASSNAME << ":Error loading base class ::Unit." << std::endl;
	sw->entity_status[id] = STEP_LOAD_ERROR;
	return false;
    }
    if (!ConversionBasedUnit::Load(step, sse)) {
	std::cout << CLASSNAME << ":Error loading base class ::Unit." << std::endl;
	sw->entity_status[id] = STEP_LOAD_ERROR;
	return false;
    }
    sw->entity_status[id] = STEP_LOADED;
    return true;
}

void
AmountOfSubstanceConversionBasedUnit::Print(int level)
{
    TAB(level);
    std::cout << CLASSNAME << ":" << "(";
    std::cout << "ID:" << STEPid() << ")" << std::endl;

    TAB(level);
    std::cout << "Inherited Attributes:" << std::endl;
    AmountOfSubstanceUnit::Print(level + 1);
    ConversionBasedUnit::Print(level + 1);

}

STEPEntity *
AmountOfSubstanceConversionBasedUnit::GetInstance(STEPWrapper *sw, int id)
{
    return new AmountOfSubstanceConversionBasedUnit(sw, id);
}

STEPEntity *
AmountOfSubstanceConversionBasedUnit::Create(STEPWrapper *sw, SDAI_Application_instance *sse)
{
    return STEPEntity::CreateEntity(sw, sse, GetInstance, CLASSNAME);
}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
