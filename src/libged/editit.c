/*                        E D I T I T . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2010 United States Government as represented by
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
/** @file editit.c
 *
 * The editit function.
 *
 */

#include <stdlib.h>

#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#  include <sys/wait.h>
#endif

#include "bio.h"
#include "ged.h"

int
_ged_editit(char *editstring, char *filename)
{
    int pid = 0;
    int xpid = 0;
    char **avtmp;
    const char *terminal = (char *)NULL;
    const char *terminal_opt = (char *)NULL;
    const char *editor = (char *)NULL;
    const char *editor_opt = (char *)NULL;
    const char *file = (const char *)filename;

    int stat = 0;
#if defined(SIGINT) && defined(SIGQUIT)
    void (*s2)();
    void (*s3)();
#endif

    /* convert the edit string into pieces suitable for arguments to execlp */

    avtmp = (char **)bu_malloc(sizeof(char *)*5, "ged_editit: editstring args");
    bu_argv_from_string(avtmp, 4, editstring);
    
    
    if (avtmp[0]) terminal = avtmp[0];
    if (avtmp[1]) terminal_opt = avtmp[1];
    if (avtmp[2]) editor = avtmp[2];
    if (avtmp[3]) editor_opt = avtmp[3];

    /* print a message to let the user know they need to quit their
     * editor before the application will come back to life.
     */
    {
	int length;
	struct bu_vls str;
	struct bu_vls sep;
	bu_vls_init(&str);
	bu_vls_init(&sep);
	bu_log("Invoking %s on %s\n\n", editor, file);
	bu_vls_sprintf(&str, "\nNOTE: You must QUIT %s before %s will respond and continue.\n", bu_basename(editor), bu_getprogname());
	for (length = bu_vls_strlen(&str) - 2; length > 0; length--) {
	    bu_vls_putc(&sep, '*');
	}
	bu_log("%V%V%V\n\n", &sep, &str, &sep);
	bu_vls_free(&str);
	bu_vls_free(&sep);
    }

#if defined(SIGINT) && defined(SIGQUIT)
    s2 = signal(SIGINT, SIG_IGN);
    s3 = signal(SIGQUIT, SIG_IGN);
#endif

#ifdef HAVE_UNISTD_H
    if ((pid = fork()) < 0) {
	perror("fork");
	return (0);
    }
#endif

    if (pid == 0) {
	/* Don't call bu_log() here in the child! */

#if defined(SIGINT) && defined(SIGQUIT)
	/* deja vu */
	(void)signal(SIGINT, SIG_DFL);
	(void)signal(SIGQUIT, SIG_DFL);
#endif

	{
#if defined(_WIN32) && !defined(__CYGWIN__)
	    char buffer[RT_MAXLINE + 1] = {0};
	    STARTUPINFO si = {0};
	    PROCESS_INFORMATION pi = {0};
	    si.cb = sizeof(STARTUPINFO);
	    si.lpReserved = NULL;
	    si.lpReserved2 = NULL;
	    si.cbReserved2 = 0;
	    si.lpDesktop = NULL;
	    si.dwFlags = 0;

	    snprintf(buffer, RT_MAXLINE, "%s %s", editor, file);

	    CreateProcess(NULL, buffer, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
	    WaitForSingleObject(pi.hProcess, INFINITE);
	    return 1;
#else
	    if (strcmp(terminal,"(null)") == 0) {
    		(void)execlp(editor, editor, file, NULL);
	    } else {
		(void)execlp(terminal, terminal, terminal_opt, editor, file, NULL);
	    }
#endif
	    /* should not reach */
	    perror(editor);
	    bu_exit(1, NULL);
	}
    }

#ifdef HAVE_UNISTD_H
    /* wait for the editor to terminate */
    while ((xpid = wait(&stat)) >= 0) {
	if (xpid == pid) {
	    break;
	}
    }
#endif

#if defined(SIGINT) && defined(SIGQUIT)
    (void)signal(SIGINT, s2);
    (void)signal(SIGQUIT, s3);
#endif

    bu_free((genptr_t)avtmp, "ged_editit: avtmp");
    return (!stat);
}


/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
