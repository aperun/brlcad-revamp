/*                     S T E P - G . C P P
 * BRL-CAD
 *
 * Copyright (c) 1994-2011 United States Government as represented by
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
/** @file step/step-g.cpp
 *
 * C++ main() for step-g converter.
 *
 */

#include "common.h"

#include <iostream>

#include "bu.h"

//
// include NIST step related headers
//
#include <sdai.h>
#include <STEPfile.h>
#include <SdaiCONFIG_CONTROL_DESIGN.h>
//
// step-g related headers
//
#include <STEPWrapper.h>
#include <BRLCADWrapper.h>

#include "Factory.h"


extern void SchemaInit (class Registry &);


void
usage()
{
    std::cerr << "Usage: step-g -o outfile.g infile.stp \n" << std::endl;
}


int
main(int argc, char *argv[])
{
    /*
     * You have to initialize the schema before you do anything else.
     * This initializes all of the registry information for the schema
     * you plan to use.  The SchemaInit() function is generated by
     * fedex_plus.  See the 'extern' stmt above.
     *
     * The registry is always going to be in memory.
     */

    // InstMgr instance_list;
    // Registry registry (SchemaInit);
    // STEPfile sfile (registry, instance_list);

    // process command line arguments
    int c;
    char *output_file=(char *)NULL;
    while ((c=bu_getopt(argc, argv, "o:")) != -1)
    {
	switch (c)
	{
	    case 'o':
		output_file = bu_optarg;
		break;
	    default:
		usage();
		bu_exit(1, NULL);
		break;
	}
    }

    if (bu_optind >= argc || output_file == (char *)NULL) {
	usage();
	bu_exit(1, NULL);
    }

    if (bu_file_exists(output_file)) {
	bu_exit(1, "ERROR - refusing to overwrite existing %s.", output_file);
    }
    
    argc -= bu_optind;
    argv += bu_optind;

    std::string iflnm = argv[0];
    std::string oflnm = output_file;

    STEPWrapper *step = new STEPWrapper();

    /* load STEP file */
    if (step->load(iflnm)) {

	step->printLoadStatistics();

	BRLCADWrapper *dotg  = new BRLCADWrapper();

	if (dotg->OpenFile(oflnm.c_str())) {
	    step->convert(dotg);
	} else {
	    std::cerr << "Error opening BRL-CAD output file -" << oflnm << std::endl;
	}

	dotg->Close();

	Factory::DeleteObjects();
	delete dotg;
    }
    delete step;

    return 0;
}


// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
