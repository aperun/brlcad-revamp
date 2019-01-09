/*                     S T E P - G . C P P
 * BRL-CAD
 *
 * Copyright (c) 1994-2019 United States Government as represented by
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

#include "bu/time.h"
#include "bu/file.h"
#include "bu/log.h"
#include "bu/opt.h"
#include "bu/vls.h"

//
// step-g related headers
//
#include <BRLCADWrapper.h>
#include <STEPWrapper.h>

//
// include NIST step related headers
//
#include <sdai.h>
#include <STEPfile.h>
#include "Factory.h"
#include "schema.h"

void
usage()
{
    std::cerr << "Usage: step-g [-v] -o outfile.g infile.stp \n" << std::endl;
}

struct OutputFile {
    char *filename;
    bool overwrite;
};

static int
parse_opt_O(struct bu_vls *error_msg, int argc, const char **argv, void *set_var)
{
    int ret;
    OutputFile *ofile = (OutputFile *)set_var;

    BU_OPT_CHECK_ARGV0(error_msg, argc, argv, "-O");

    ofile->overwrite = true;
    if (ofile) {
	ret = bu_opt_str(error_msg, argc, argv, &ofile->filename);
	return ret;
    } else {
	return -1;
    }
}

int
main(int argc, const char *argv[])
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
    static OutputFile ofile = {NULL, false};
    static bool verbose = false;
    static int dry_run = 0;
    static char *summary_log_file = (char *)NULL;
    struct bu_vls parse_msgs = BU_VLS_INIT_ZERO;
    struct bu_opt_desc options[] = {
	{"D", "", "",         NULL,        &dry_run,          "dry run (output file not written)"},
	{"v", "", "",         NULL,        &verbose,          "verbose"},
	{"O", "", "filename", parse_opt_O, &ofile,            "output file (overwrite)"},
	{"o", "", "filename", bu_opt_str,  &ofile.filename,   "output file"},
	{"S", "", "filename", bu_opt_str,  &summary_log_file, "summary file"},
	BU_OPT_DESC_NULL
    };

    ++argv; --argc;
    argc = bu_opt_parse(&parse_msgs, argc, argv, options);

    if (bu_vls_strlen(&parse_msgs) > 0) {
	bu_log("%s\n", bu_vls_cstr(&parse_msgs));
    }
    if (argc != 1 || BU_STR_EMPTY(ofile.filename)) {
	usage();
	bu_exit(1, NULL);
    }
    if (!ofile.overwrite) {
	/* check our inputs/outputs */
	if (bu_file_exists(ofile.filename, NULL)) {
	    bu_exit(1, "ERROR: refusing to overwrite existing output file:"
		        "\"%s\". Please remove the existing file, change the "
			"output file name, or use the '-O' option to force an"
			" overwrite.", ofile.filename);
	}
    }
    if (!bu_file_exists(argv[0], NULL) && !BU_STR_EQUAL(argv[0], "-")) {
	bu_exit(2, "ERROR: unable to read input \"%s\" STEP file", argv[0]);
    }

    std::string iflnm = argv[0];
    std::string oflnm = ofile.filename;

    STEPWrapper *step = new STEPWrapper();

    step->dry_run = dry_run;
    step->summary_log_file= summary_log_file;

    step->Verbose(verbose);

    /* load STEP file */
    if (step->load(iflnm)) {

	step->printLoadStatistics();

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
	elapsedtime = bu_gettime();

	BRLCADWrapper *dotg  = new BRLCADWrapper();
	if (!dotg) {
	    std::cerr << "ERROR: unable to create BRL-CAD instance" << std::endl;
	    ret = 3;
	} else {

	    dotg->dry_run = dry_run;
	    std::cerr << "Writing output file [" << oflnm << "] ...";
	    if (dotg->OpenFile(oflnm)) {
		step->convert(dotg);
		dotg->Close();
		std::cerr << "done!" << std::endl;
	    } else {
		std::cerr << "ERROR: unable to open BRL-CAD output file [" << oflnm << "]" << std::endl;
		ret = 4;
	    }

	    delete dotg;
	}
    }
    delete step;
    Factory::DeleteObjects();

    elapsedtime = bu_gettime() - elapsedtime;
    {
	struct bu_vls vls = BU_VLS_INIT_ZERO;
	int seconds = elapsedtime / 1000000;
	int minutes = seconds / 60;
	int hours = minutes / 60;

	minutes = minutes % 60;
	seconds = seconds %60;

	bu_vls_printf(&vls, "Convert time: %02d:%02d:%02d\n", hours, minutes, seconds);
	std::cerr << bu_vls_addr(&vls) << std::endl;
	bu_vls_free(&vls);
    }

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
