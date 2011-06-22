/* $NetBSD: setprogname.c,v 1.3 2003/07/26 19:24:44 salo Exp $ */

/*
 * Copyright (c) 2001 Christopher G. Demetriou
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>
 */

#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: setprogname.c,v 1.3 2003/07/26 19:24:44 salo Exp $");
#endif /* LIBC_SCCS and not lint */

/* In NetBSD, the program name is set by crt0.  It can't be overridden. */
#undef	REALLY_SET_PROGNAME

#include <stdlib.h>

#if WIN32
# define REALLY_SET_PROGNAME 1
char *__progname;
#endif

#ifdef REALLY_SET_PROGNAME
#include <string.h>

# ifndef WIN32
extern const char *__progname;
# endif
#endif

/*ARGSUSED*/
void
setprogname(const char *progname)
{

#ifdef REALLY_SET_PROGNAME
	__progname = strrchr(progname, '/');
	if (__progname == NULL)
		__progname = progname;
	else
		__progname++;
#endif
}
