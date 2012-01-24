/*                 ApplicationContext.h
 * BRL-CAD
 *
 * Copyright (c) 1994-2012 United States Government as represented by
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
/** @file step/ApplicationContext.h
 *
 * Class definition used to convert STEP "ApplicationContext" to BRL-CAD BREP
 * structures.
 *
 */

#ifndef APPLICATION_CONTEXT_H_
#define APPLICATION_CONTEXT_H_

#include "STEPEntity.h"

#include "sdai.h"

// forward declaration of class
class ON_Brep;

class ApplicationContext: virtual public STEPEntity {
private:
    static string entityname;

protected:
    string application;

public:
    ApplicationContext();
    virtual ~ApplicationContext();
    ApplicationContext(STEPWrapper *sw, int step_id);
    bool Load(STEPWrapper *sw, SCLP23(Application_instance) *sse);
    virtual bool LoadONBrep(ON_Brep *brep);
    string ClassName();
    string Application();
    virtual void Print(int level);

    //static methods
    static STEPEntity *Create(STEPWrapper *sw, SCLP23(Application_instance) *sse);
};

#endif /* APPLICATION_CONTEXT_H_ */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
