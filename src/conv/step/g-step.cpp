/*                    G -  S T E P . C P P
 * BRL-CAD
 *
 * Copyright (c) 2013 United States Government as represented by
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
/** @file step/g-step.cpp
 *
 * C++ main() for g-step converter.
 *
 */

#include "common.h"

#include <iostream>

#include "bu.h"

//
// step-g related headers
//
#include <BRLCADWrapper.h>
#include <STEPWrapper.h>
#include "ON_Brep.h"

//
// include NIST step related headers
//
#include <sdai.h>
#include <STEPfile.h>
#ifdef AP203e2
#  include <SdaiAP203_CONFIGURATION_CONTROLLED_3D_DESIGN_OF_MECHANICAL_PARTS_AND_ASSEMBLIES_MIM_LF.h>
#else
#  include <SdaiCONFIG_CONTROL_DESIGN.h>
#endif


#include "Factory.h"
#include "schema.h"

void
usage()
{
    std::cerr << "Usage: g-step -o outfile.stp infile.g \n" << std::endl;
}


int
main(int argc, char *argv[])
{
    int ret = 0;

    // process command line arguments
    int c;
    char *output_file = (char *)NULL;
    while ((c = bu_getopt(argc, argv, "o:")) != -1) {
	switch (c) {
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

    argc -= bu_optind;
    argv += bu_optind;

    /* check our inputs/outputs */
    if (bu_file_exists(output_file, NULL)) {
	bu_exit(1, "ERROR: refusing to overwrite existing output file:\"%s\". Please remove file or change output file name and try again.", output_file);
    }

    if (!bu_file_exists(argv[0], NULL) && !BU_STR_EQUAL(argv[0], "-")) {
	bu_exit(2, "ERROR: unable to read input \"%s\" .g file", argv[0]);
    }

    std::string iflnm = argv[0];
    std::string oflnm = output_file;

    /* load .g file */
    BRLCADWrapper *dotg  = new BRLCADWrapper();
    if (!dotg) {
	std::cerr << "ERROR: unable to create BRL-CAD instance" << std::endl;
	return 3;
    }
    if (!dotg->load(iflnm)) {
	std::cerr << "ERROR: unable to open BRL-CAD input file [" << oflnm << "]" << std::endl;
        delete dotg;
        return 2;
    }

    struct db_i *dbip = dotg->GetDBIP();
    struct directory *dp = db_lookup(dbip, "brep.s", LOOKUP_QUIET);
    if (dp == RT_DIR_NULL) {
	std::cerr << "ERROR: cannot find " << "brep.s" << "\n" << std::endl;
        delete dotg;
        return 1;
    }

    struct rt_db_internal intern;
    struct rt_brep_internal *bi;
    rt_db_get_internal(&intern, dp, dbip, bn_mat_identity, &rt_uniresource);
    RT_CK_DB_INTERNAL(&intern);
    bi = (struct rt_brep_internal*)intern.idb_ptr;
    //RT_BREP_TEST_MAGIC(bi);
    ON_Brep *brep = bi->brep;
    ON_wString wstr;
    ON_TextLog dump(wstr);
    brep->Dump(dump);
    ON_String ss = wstr;
    bu_log("Brep:\n %s\n", ss.Array());

    Registry *registry = new Registry( SchemaInit );
    InstMgr instance_list;
    STEPfile *sfile = new STEPfile(*registry, instance_list);

    registry->ResetSchemas();
    registry->ResetEntities();
    ON_BRep_to_STEP(brep, registry, &instance_list);

    sfile->WriteExchangeFile(std::cout);

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
