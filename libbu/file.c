/*
 *			F I L E . C
 *
 *  General I/O for ASCII files
 *
 *  Author -
 *	Paul Tanenbaum
 *
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Notice -
 *	Re-distribution of this software is restricted, as described in
 *	your "Statement of Terms and Conditions for the Release of
 *	The BRL-CAD Package" agreement.
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 1997 by the United States Army
 *	in all countries except the USA.  All rights reserved.
 */
static char RCSrtstring[] = "@(#)$Header$ (BRL)";

#include "conf.h"

#include <stdio.h>
#include <ctype.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#if defined(HAVE_STDARG_H)
# include <stdarg.h>
#endif

#include "machine.h"
#include "externs.h"
#include "bu.h"

/*
 *		XXX	Warning!	XXX
 *
 *	The following initialization of bu_stdin is essentially
 *	an inline version of bu_fopen() and bu_vls_init().  As such,
 *	it depends heavily on the definitions of struct bu_file and
 *	struct bu_vls in ../h/bu.h
 *
 *		XXX			XXX
 */
char	dmy_eos = '\0';
BU_FILE	bu_iob[1] = {
    {
	BU_FILE_MAGIC, stdin, "stdin",
	{
	    BU_VLS_MAGIC, (char *) 0, 0, 0, 0
	},
	&dmy_eos, 1, 0, '#', -1
    }
};

/*
 *			B U _ F O P E N
 *
 */
BU_FILE *bu_fopen (fname, type)

register char	*fname;
register char	*type;

{
    BU_FILE	*bfp;
    FILE	*fp;

    if ((fp = fopen(fname, type)) == NULL)
	return (NULL);

    bfp = (BU_FILE *) bu_malloc(sizeof(BU_FILE), "bu_file struct");

    bfp -> file_magic = BU_FILE_MAGIC;
    bfp -> file_ptr = fp;
    bfp -> file_name = fname;
    bu_vls_init(&(bfp -> file_buf));
    bfp -> file_bp = bu_vls_addr(&(bfp -> file_buf));
    bfp -> file_needline = 1;
    bfp -> file_linenm = 0;
    bfp -> file_comment = '#';
    bfp -> file_buflen = -1;

    return (bfp);
}

/*
 *			B U _ F C L O S E
 *
 *	Close the file and free the associated memory
 */
int bu_fclose (bfp)

register BU_FILE	*bfp;

{
    int	close_status;

    BU_CK_FILE(bfp);

    close_status = fclose(bfp -> file_ptr);

    if (bfp != bu_stdin)
    {
	bfp -> file_magic = 0;
	bfp -> file_ptr = NULL;
	bfp -> file_name = (char *) 0;
	bfp -> file_bp = (char *) 0;
	bu_vls_free(&(bfp -> file_buf));
	bu_free((genptr_t) bfp, "bu_file struct");
    }
    return (close_status);
}

/*
 *			B U _ F G E T C
 *
 */
int bu_fgetc (bfp)

register BU_FILE	*bfp;

{
    char	*cp;
    int		comment_char;	/* The comment character */
    int		strip_comments;	/* Should I strip comments? */

    BU_CK_FILE(bfp);

    strip_comments = isprint(comment_char = bfp -> file_comment);

    /*
     *	If buffer is empty, note that it's time for a new line of input
     */
    if ((*(bfp -> file_bp) == '\0') && ! (bfp -> file_needline))
    {
	bfp -> file_needline = 1;
	return ('\n');
    }

    /*
     *    If it's time to grab a line of input from the file, do so.
     */
    if (bfp -> file_needline)
    {
	bu_vls_trunc(&(bfp -> file_buf), 0);
	if (bu_vls_gets(&(bfp -> file_buf), bfp -> file_ptr) == -1)
	    return (EOF);
	bfp -> file_bp = bu_vls_addr(&(bfp -> file_buf));
	++(bfp -> file_linenm);

	if (strip_comments)
	{
	    bfp -> file_buflen = -1;
	    for (cp = bfp -> file_bp; *cp != '\0'; ++cp)
		if (*cp == comment_char)
		{
		    bfp -> file_buflen = (bfp -> file_buf).vls_len;
		    bu_vls_trunc(&(bfp -> file_buf), cp - bfp -> file_bp);
		    if (cp == bfp -> file_bp)
			return ('\n');
		    break;
		}
	}
	bfp -> file_needline = 0;
    }
    
    return (*(bfp -> file_bp)++);
}

/*
 *			B U _ P R I N T F I L E
 *
 *	Diagnostic routine to print out the contents of a struct bu_file
 */
void bu_printfile (bfp)

register BU_FILE	*bfp;

{
    BU_CK_FILE(bfp);

    bu_log("File     '%s'...\n", bfp -> file_name);
    bu_log("  ptr      %x\n", bfp -> file_ptr);
    bu_log("  buf      '%s'\n", bu_vls_addr(&(bfp -> file_buf)));
    bu_log("  bp       %d", bfp -> file_bp - bu_vls_addr(&(bfp -> file_buf)));
    bu_log(": '%c' (%03o)\n", *(bfp -> file_bp), *(bfp -> file_bp));
    bu_log("  needline %d\n", bfp -> file_needline);
    bu_log("  linenm   %d\n", bfp -> file_linenm);
    bu_log("  comment  '%c' (%d)\n",
	bfp -> file_comment, bfp -> file_comment);
    bu_log("  buflen   %d\n", bfp -> file_buflen);
}

/*
 *			B U _ F I L E _ E R R
 *
 *	Print out a syntax error message about a BU_FILE
 */
void bu_file_err (bfp, text1, text2, cursor_pos)

register BU_FILE	*bfp;
register char		*text1, *text2;
register int		cursor_pos;

{
    char		*cp;
    int			buflen;
    int			i;
    int			stripped_length;

    BU_CK_FILE(bfp);

    /*
     *	Show any trailing comments
     */
    if ((buflen = bfp -> file_buflen) > -1)
    {
	stripped_length = (bfp -> file_buf).vls_len;
	*(bu_vls_addr(&(bfp -> file_buf)) + stripped_length) =
	    bfp -> file_comment;
	(bfp -> file_buf).vls_len = buflen;
    }
    else
	stripped_length = -1;

    /*
     *	Print out the first line of the error message
     */
    if (text1 && (*text1 != '\0'))
	bu_log("%s: ", text1);
    bu_log("Error: file %s, line %d: %s\n",
	bfp -> file_name, bfp -> file_linenm, text2);
    bu_log("%s\n", bu_vls_addr(&(bfp -> file_buf)));

    /*
     *	Print out position-indicating arrow, if requested
     */
    if ((cursor_pos >= 0)
     && (cursor_pos < bu_vls_strlen(&(bfp -> file_buf))))
    {
	cp = bu_vls_addr(&(bfp -> file_buf));
	for (i = 0; i < cursor_pos; ++i)
	    if (*cp++ == '\t')
		bu_log("\t");
	    else
		bu_log("-");
	bu_log("^\n");
    }

    /*
     *	Hide the comments again
     */
    if (stripped_length > -1)
	bu_vls_trunc(&(bfp -> file_buf), stripped_length);
}
