/*
 *  			C V . C
 *  
 *  Converts .vg files to .g (latest style, with idents).
 *  
 *  Authors -
 *	Mike Muuss, BRL, 5/25/84.
 *	Keith A. Applin
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
#include "db.h"

union record rec;

char line[256];

main(argc, argv)
int argc;
char *argv[];
{
	static int ifd, ofd;
	static int units;
	static int count;
	static int i;
	double factor;

	if( argc != 3 )  {
		printf("Usage: conv-vg2g file.vg file.g\n");
		exit(11);
	}
	if( (ifd = open( argv[1], 0 )) < 0 )  {
		perror(argv[1]);
		exit(12);
	}
	if( (ofd = creat(argv[2], 0664)) < 0 )  {
		perror(argv[2]);
		exit(13);
	}

	/* check for conversion from version 3 to version 4 */
	read(ifd, &rec, sizeof rec);
	if(rec.u_id == ID_IDENT) {
		/* have an mged type file - check its version */
		if( strcmp(rec.i.i_version, ID_VERSION) == 0 ) {
			(void)printf("%s: NO conversion necessary\n", argv[1]);
			(void)putchar(7);
			return;
		}

		else {
			/* convert from version 3 to version 4 */
			(void)printf("convert from ver 3 to ver 4\n");
			units = ID_IN_UNIT;
			rec.i.i_version[0] = '\0';
			strcpy(rec.i.i_version, ID_VERSION);
		}
	}
	else {
		lseek(ifd, 0L, 0);
		/* have an old vged file to convert */

		/* units are inportant now because:
		 *	The ged data records will be stored in a constant BASE unit
		 *	of MiliMeters (MM).
		 *	At any time the ged user can change his local units.
		 *    	Hence cv must know the original units of the ".vg" file so
		 *	that they can be converted to BASE units.
		 */
		(void)printf("* *  V E R Y    I M P O R T A N T    N O T I C E  * *\n");
		(void)printf("    You must KNOW the units of the %s file\n",argv[1]);
		(void)printf("    If you don't know, DON'T guess....find out\n");
		(void)putchar( 7 );

		rec.i.i_id = ID_IDENT;
		strcpy( rec.i.i_version, ID_VERSION );
		rec.i.i_units = 100;
		while( rec.i.i_units < ID_MM_UNIT || rec.i.i_units > ID_FT_UNIT )  {
			printf("Units: 1=mm, 2=cm, 3=meters, 4=inches, 5=feet\nUnits? ");
			gets( line );
			sscanf( line, "%d", &units );
			rec.i.i_units = units;
			printf("units=%d\n", rec.i.i_units);
		}
		printf("Title? "); fflush(stdout);
		gets( line );
		strncpy( rec.i.i_title, line, 71 );
		rec.i.i_title[71] = '\0';
		printf("Title=%s\n", rec.i.i_title );
	}

	write( ofd, &rec, sizeof rec );
	count = 1;

	/* find the units scale factor */
	switch( units ) {

	case ID_MM_UNIT:
		factor = 1.0;			/* from MM to MM */
		break;

	case ID_CM_UNIT:
		factor = 10.0;			/* from CM to MM */
		break;

	case ID_M_UNIT:
		factor = 1000.0;		/* from M to MM */
		break;

	case ID_IN_UNIT:
		factor = 25.4;			/* from IN to MM */
		break;

	case ID_FT_UNIT:
		factor = 304.8;			/* from FT to MM */
		break;
	default:
		printf("eh?\n");
	}

top:
	while( read( ifd, &rec, sizeof rec ) == sizeof rec )  {
		switch( rec.u_id )  {

		case ID_IDENT:
			printf("Unexpected ID record encountered in input\n");
			printf("Title=%s\n", rec.i.i_title );
			break;
		case ID_FREE:
			goto top;
		case ID_SOLID:
			if( rec.s.s_name[0] == '\0' )
				goto top;
			for(i=0; i<24; i++)
				rec.s.s_values[i] *= factor;
			break;
		case ID_COMB:
			if( rec.c.c_name[0] == '\0' )  {
				/* Skip deleted junk */
				lseek( ifd, (long)rec.c.c_length *
					(long)(sizeof rec), 1 );
				goto top;
			}
			break;
		case ID_ARS_B:
			for(i=0; i<24; i++)
				rec.b.b_values[i] *= factor;
			break;
		case ID_ARS_A:
			if( rec.a.a_name[0] == '\0' )  {
				/* Skip deleted junk */
				lseek( ifd, (long)(rec.a.a_totlen) *
					(long)(sizeof rec), 1 );
				goto top;
			}
			rec.a.a_xmin *= factor;
			rec.a.a_xmax *= factor;
			rec.a.a_ymin *= factor;
			rec.a.a_ymax *= factor;
			rec.a.a_zmin *= factor;
			rec.a.a_zmax *= factor;
			break;
		case ID_P_HEAD:
		case ID_P_DATA:
			break;

		default:
			printf("Garbage record in database\n");
			exit(42);
		case ID_MEMB:
			/* flip translation to other side */
#define m rec.M.m_mat
			m[3] *= factor;
			m[7] *= factor;
			m[11] *= factor;
			break;
		}
		write( ofd, &rec, sizeof(rec) );
		count++;
	}
	printf("%d database granules written\n", count);
}

mat_pr( title, mp )
char *title;
float *mp;
{
	register int i;

	printf("MATRIX %s:\n  ", title);
	for(i=0; i<16; i++)  {
		printf(" %8.3f", mp[i]);
		if( (i&3) == 3 ) printf("\n  ");
	}
}
