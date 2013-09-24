/*                      P R O G N A M E . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2013 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE /* must come before common.h */
#endif

#include "common.h"

#ifdef HAVE_SYS_PARAM_H /* for MAXPATHLEN */
#  include <sys/param.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "bio.h"

#include "bu.h"

/* internal storage for bu_getprogname/bu_setprogname */
static char bu_progname[MAXPATHLEN] = {0};
const char *DEFAULT_PROGNAME = "(BRL-CAD)";


const char *
bu_argv0_full_path(void)
{
    static char buffer[MAXPATHLEN] = {0};

    const char *argv0 = bu_getprogname();
    const char *which = bu_which(argv0);

    if (argv0[0] == BU_DIR_SEPARATOR) {
	/* seems to already be a full path */
	snprintf(buffer, MAXPATHLEN, "%s", argv0);
	return buffer;
    }

    /* running from PATH */
    if (which) {
	snprintf(buffer, MAXPATHLEN, "%s", which);
	return buffer;
    }

    while (argv0[0] == '.' && argv0[1] == BU_DIR_SEPARATOR) {
	/* remove ./ if present, relative paths are appended to pwd */
	argv0 += 2;
    }

    /* running from relative dir */
    bu_getcwd(buffer, MAXPATHLEN);
    snprintf(buffer+strlen(buffer), MAXPATHLEN-strlen(buffer), "%c%s", BU_DIR_SEPARATOR, argv0);
    if (bu_file_exists(buffer, NULL)) {
	return buffer;
    }

    /* give up */
    snprintf(buffer, MAXPATHLEN, "%s", argv0);
    return buffer;
}


const char *
bu_getprogname(void)
{
    const char *name = bu_progname;
    char *tmp_basename = NULL;

#ifdef HAVE_PROGRAM_INVOCATION_NAME
    /* GLIBC provides a way */
    if (name[0] == '\0' && program_invocation_name)
	name = program_invocation_name;
#endif

#ifdef HAVE_GETPROGNAME
    /* BSD's libc provides a way */
    if (name[0] == '\0') {
	name = getprogname(); /* not malloc'd memory, may return NULL */
	if (!name)
	    name = bu_progname;
    }
#endif

    tmp_basename = bu_basename(name);

    if (BU_STR_EQUAL(tmp_basename, ".") || BU_STR_EQUAL(tmp_basename, "/")) {
	bu_semaphore_acquire(BU_SEM_SYSCALL);
	bu_strlcpy(bu_progname, DEFAULT_PROGNAME, MAXPATHLEN);
	bu_semaphore_release(BU_SEM_SYSCALL);
    } else {
	bu_semaphore_acquire(BU_SEM_SYSCALL);
	bu_strlcpy(bu_progname, tmp_basename, MAXPATHLEN);
	bu_semaphore_release(BU_SEM_SYSCALL);
    }

    bu_free(tmp_basename, "tmp_basename free");

    return bu_progname;
}


void
bu_setprogname(const char *argv0)
{
    bu_semaphore_acquire(BU_SEM_SYSCALL);
    if (argv0) {
	bu_strlcpy(bu_progname, argv0, MAXPATHLEN);
    } else {
	bu_progname[0] = '\0'; /* zap */
    }
    bu_semaphore_release(BU_SEM_SYSCALL);

    return;
}


/* DEPRECATED: Do not use. */
const char *
bu_argv0(void)
{
    return bu_getprogname();
}


/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
