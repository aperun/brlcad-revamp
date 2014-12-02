/*                     I F C - G . C P P
 * BRL-CAD
 *
 * Copyright (c) 1994-2014 United States Government as represented by
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
/** @file ifc/ifc-g.cpp
 *
 * C++ main() for ifc-g converter.
 *
 */

#include "common.h"

#include <iostream>

#include "bu/getopt.h"
#include "bu/time.h"
#include "bu/file.h"
#include "bu/log.h"
#include "bu/str.h"
#include "bu/vls.h"

#include "IFCWrapper.h"

//
// include NIST ifc related headers
//
#include <sdai.h>
#include <STEPfile.h>
#include "schema.h"

void
usage()
{
    std::cerr << "Usage: ifc-g [-v] -o outfile.g infile.ifc \n" << std::endl;
}


int
main(int argc, char *argv[])
{
    int ret = 0;
    int64_t elapsedtime;

    elapsedtime = bu_gettime();

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
    char *output_file = (char *)NULL;
    bool verbose = false;
    int dry_run = 0;
    char *summary_log_file = (char *)NULL;
    while ((c = bu_getopt(argc, argv, "DS:vo:")) != -1) {
	switch (c) {
	    case 'D':
		dry_run = 1;
		break;
	    case 'S':
		summary_log_file = bu_optarg;
		break;
	    case 'o':
		output_file = bu_optarg;
		break;
	    case 'v':
		verbose = true;
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

    argc -= bu_optind;
    argv += bu_optind;

#ifndef OVERWRITE_WHILE_DEBUGGING
    /* check our inputs/outputs */
    if (bu_file_exists(output_file, NULL)) {
	bu_exit(1, "ERROR: refusing to overwrite existing output file:\"%s\". Please remove file or change output file name and try again.", output_file);
    }
#endif

    if (!bu_file_exists(argv[0], NULL) && !BU_STR_EQUAL(argv[0], "-")) {
	bu_exit(2, "ERROR: unable to read input \"%s\" STEP file", argv[0]);
    }

    std::string iflnm = argv[0];
    std::string oflnm = output_file;

    IFCWrapper *ifc = new IFCWrapper();

    ifc->dry_run = dry_run;
    ifc->summary_log_file= summary_log_file;

    ifc->Verbose(verbose);

    /* load STEP file */
    if (ifc->load(iflnm)) {

	ifc->printLoadStatistics();

	elapsedtime = bu_gettime() - elapsedtime;
	{
	    struct bu_vls vls = BU_VLS_INIT_ZERO;
	    int seconds = elapsedtime / 1000000;
	    int minutes = seconds / 60;
	    int hours = minutes / 60;

	    minutes = minutes % 60;
	    seconds = seconds %60;

	    bu_vls_printf(&vls, "Load time: %02d:%02d:%02d\n", hours, minutes, seconds);
	    std::cerr << bu_vls_addr(&vls) << std::endl;
	    bu_vls_free(&vls);
	}
    }
    delete ifc;

    return ret;
}


// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
