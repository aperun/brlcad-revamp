/*
 *			B R L C A D _ P A T H . C
 *
 *  A support routine to provide the executable code with the path
 *  to where the BRL-CAD programs and libraries are installed.
 *
 *  Author -
 *	Michael John Muuss
 *
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Status -
 *      Public Domain, Distribution Unlimitied.
 */
static const char RCSbrlcad_path[] = "@(#)$Header$ (BRL)";

#include "common.h"



#include <stdio.h>
#include <sys/stat.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "machine.h"
#include "bu.h"

/*
 *			B U _ F I L E _ E X I S T S
 *
 *  Returns boolean -
 *	1	The given filename exists.
 *	0	The given filename does not exist.
 */
int
bu_file_exists(const char *path)
{
	struct	stat	sbuf;

	if( path == NULL )  return 0;			/* FAIL */

	/* defined in unistd.h */
#if defined(F_OK)
	if( access( path, F_OK )  == 0 )  return 1;	/* OK */
#endif

	if( stat( path, &sbuf ) == 0 )  return 1;	/* OK */
	return 0;					/* FAIL */
}

/*
 *			B U _ B R L C A D _ P A T H
 *
 *  Locate where the BRL-CAD programs and libraries are located,
 *  contatenate on the rest of the string provided by the caller,
 *  and return a pointer to a STATIC buffer with the full path.
 *  It is the caller's responsibility to call bu_strdup() or make
 *  other provisions to save the returned string, before calling again.
 *  bu_bomb() if unable to find the base path.
 *
 */
char *
bu_brlcad_path(const char *rhs, int fail_quietly)
{
	static char	result[256];
	char		*lhs;

	lhs = INSTALL_DIRECTORY ;
	if( bu_file_exists(lhs) )
		goto ok;

	if( (lhs = getenv("BRLCAD_ROOT")) != NULL )  {
		if( bu_file_exists(lhs) )
			goto ok;
		if (fail_quietly) {
		  return NULL;
		}

		bu_log("\
Unable to find the directory that BRL-CAD is installed in while seeking: \n\
	%s\n\
This version of LIBBU was compiled to expect BRL-CAD in: \n\
	%s\n\
but it is no longer there. \n\
  After you manually locate BRL-CAD,\n\
please set your environment variable BRLCAD_ROOT to the correct path,\n\
and re-run this program.\n\
\n\
csh/tcsh users:\n\
	setenv BRLCAD_ROOT %s\n\
sh/bash users:\n\
	BRLCAD_ROOT=%s; export BRLCAD_ROOT\n",
	       rhs, INSTALL_DIRECTORY , lhs, lhs);
	}

	/* Can't find the BRL-CAD root directory, this is fatal! */
	/* Could use some huristics here, but being overly clever is probably bad. */

	if (fail_quietly) {
	  return NULL;
	}


	bu_log("\
Unable to find the directory that BRL-CAD is installed in while seeking: \n\
	%s\n\
This version of LIBBU was compiled to expect BRL-CAD in: \n\
	%s\n\
but it is no longer there. \n\
  After you manually locate BRL-CAD,\n\
please set your environment variable BRLCAD_ROOT to the correct path,\n\
and re-run this program.\n\
\n\
csh/tcsh users:\n\
	setenv BRLCAD_ROOT /path/to/brlcad\n\
sh/bash users:\n\
	BRLCAD_ROOT=/path/to/brlcad; export BRLCAD_ROOT\n",
	       rhs, INSTALL_DIRECTORY );

	bu_bomb("bu_brlcad_path()");

ok:
#ifndef WIN32
	sprintf(result, "%s/%s", lhs, rhs );
#else
	if(strcmp(rhs,"")==0)
		sprintf(result, "%s", lhs );
	else
		sprintf(result, "%s\\%s", lhs, rhs );
#endif
	if( bu_file_exists(result) )
		return result;			/* OK */

	if (fail_quietly) {
	  return NULL;
	}

	bu_log("\
Unable to find '%s' within the BRL-CAD\n\
software installed in '%s'.\n\
This copy of BRL-CAD does not appear to be fully installed.\n\
Please contact your system administrator for assistance.\n",
	rhs, lhs );
	bu_bomb("bu_brlcad_path()");

	return NULL;				/* NOTREACHED */
}
