/*
 *  			C O L U M N S
 *  
 *  A set of routines for printing columns of data.
 *
 * Functions -
 *	col_item	Called to print an item
 *	col_putchar	Called to annotate an item
 *	col_eol		Called to end a line
 *	cmpdirname	Comparison function for col_pr4v
 *	col_pr4v	Called to sort and print directory entry names
 *			  vertically in four columns (ala "ls -C")
 *
 *  Authors -
 *	Michael John Muuss
 *	Robert Jon Reschly Jr.
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "db.h"			/* for NAMESIZE */
#include "raytrace.h"
#include "./ged.h"

static int	col_count;		/* names listed on current line */
static int	col_len;		/* length of previous name */
#define	COLUMNS	((80 + NAMESIZE - 1) / NAMESIZE)

extern int qsort(), strcmp();

void
col_item(cp)
register char *cp;
{
	/* Output newline if last column printed. */
	if ( col_count >= COLUMNS )  {	/* line now full */
		(void)putchar( '\n' );
		col_count = 0;
	} else if ( col_count != 0 ) {
		/* Tab to start column. */
		do
			(void)putchar( '\t' );
		while ( (col_len += 8) < NAMESIZE );
	}
	/* Output string and save length for next tab. */
	col_len = 0;
	while ( *cp != '\0' )  {
		(void)putchar( *cp );	/* is a macro */
		++cp;
		++col_len;
	}
	col_count++;
#undef	COLUMNS
}

void
col_putchar(c)
char c;
{
	(void)putchar(c);
	col_len++;
}

void
col_eol()
{
	if ( col_count != 0 )		/* partial line */
		(void)putchar( '\n' );
	col_count = 0;
	col_len = 0;
}

/*
 *			C M P D I R N A M E
 *
 * Given two pointers to pointers to directory entries, do a string compare
 * on the respective names and return that value.
 */
int
cmpdirname(dp1, dp2)
struct directory **dp1, **dp2;
{
	return( strcmp( (*dp1)->d_namep, (*dp2)->d_namep));
}

/*
 *				C O L _ P R 4 V
 *
 *  Given a pointer to a list of pointers to names and the number of names
 *  in that list, sort and print that list in column order over four columns.
 */
void
col_pr4v( list_of_names, num_in_list)
struct directory **list_of_names;
int num_in_list;
{
	int lines, i, j, namelen, this_one;

	qsort(list_of_names, num_in_list, sizeof(struct directory *),
	  cmpdirname);
	/*
	 * For the number of (full and partial) lines that will be needed,
	 * print in vertical format.
	 */
	lines = (num_in_list + 3) / 4;
	for( i=0; i < lines; i++) {
		for( j=0; j < 4; j++) {
			this_one = j * lines + i;
			/* Restrict the print to 16 chars per spec. */
			printf( "%.16s", list_of_names[this_one]->d_namep);
			namelen = strlen( list_of_names[this_one]->d_namep);
			if( namelen > 16)
				namelen = 16;
			/*
			 * Region and ident checks here....  Since the code
			 * has been modified to push and sort on pointers,
			 * the printing of the region and ident flags must
			 * be delayed until now.  There is no way to make the
			 * decision on where to place them before now.
			 */
			if(list_of_names[this_one]->d_flags & DIR_COMB) {
				putchar('/');
				namelen++;
			}
			if(list_of_names[this_one]->d_flags & DIR_REGION) {
				putchar('R');
				namelen++;
			}
			/*
			 * Size check (partial lines), and line termination.
			 * Note that this will catch the end of the lines
			 * that are full too.
			 */
			if( this_one + lines >= num_in_list) {
				putchar('\n');
				break;
			} else {
				/*
				 * Pad to next boundary as there will be
				 * another entry to the right of this one. 
				 */
				while( namelen++ < 20)
					putchar(' ');
			}
		}
	}
}
