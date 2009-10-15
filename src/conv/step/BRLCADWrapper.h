/*                 BRLCADWrapper.h
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
/** @file BRLCADWrapper.h
 *
 * Class definition for C++ wrapper to BRL-CAD database functions.
 *
 */

#ifndef BRLCADWRAPPER_H_
#define BRLCADWRAPPER_H_

class ON_Brep;

class BRLCADWrapper {
private:
	string filename;
	struct rt_wdb *outfp;
	static int sol_reg_cnt;

public:
	BRLCADWrapper();
	virtual ~BRLCADWrapper();
	bool OpenFile( const char * flnm);
	bool WriteHeader();
	bool WriteSphere(double *center, double radius);
	bool WriteBrep(string name,ON_Brep *brep);
	bool Close();
};

#endif /* BRLCADWRAPPER_H_ */
