/*
 *			F B C M R O T . C
 *
 * Function -
 *	Dynamicly rotate the color map
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1986 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>	
#include "fb.h"

double	atof();

extern int	getopt();
extern char	*optarg;
extern int	optind;

ColorMap cm1, cm2;
ColorMap *inp, *outp;

#ifdef SYSV
#define bzero(p,cnt)	memset(p,'\0',cnt);
#endif

int size = 512;
double fps = 0.0;	/* frames per second */
int increment = 1;
int onestep = 0;

FBIO *fbp;

static char usage[] = "\
Usage: fbcmrot [-h] [-i increment] steps_per_second\n";

get_args( argc, argv )
register char **argv;
{
	register int c;

	while ( (c = getopt( argc, argv, "hi:" )) != EOF )  {
		switch( c )  {
		case 'h':
			/* high-res */
			size = 1024;
			break;
		case 'i':
			/* increment */
			increment = atoi(optarg);
			break;

		default:		/* '?' */
			return(0);
		}
	}

	if( optind >= argc )  {
		/* no fps specified */
		fps = 0;
	} else {
		fps = atoi(argv[optind]);
		if( fps == 0 )
			onestep++;
	}

	if ( argc > ++optind )
		(void)fprintf( stderr, "fbcmrot: excess argument(s) ignored\n" );

	return(1);		/* OK */
}

main(argc, argv )
char **argv;
{
	register int i;
	int sec, usec;

	if ( !get_args( argc, argv ) )  {
		(void)fputs(usage, stderr);
		exit( 1 );
	}

	if( fps != 0 ) {
		sec = 1.0 / fps;
		usec = ((1.0 / fps) - sec) * 1000000;
	}

	if( (fbp = fb_open( NULL, size, size)) == FBIO_NULL )  {
		fprintf(stderr, "fbcmrot:  fb_open failed\n");
		return	1;
	}

	inp = &cm1;
	outp = &cm2;
	fb_rmap( fbp, inp );

	while(1)  {
		register int from;
		ColorMap *tp;

		/* Build color map for current value */
		for( i=0, from = increment; i < 256; i++, from++ ) {
			if( from < 0 )
				from += 256;
			else if( from > 255 )
				from -= 256;
			outp->cm_red[i]   = inp->cm_red[from];
			outp->cm_green[i] = inp->cm_green[from];
			outp->cm_blue[i]  = inp->cm_blue[from];
		}

		fb_wmap( fbp, outp );
		tp = outp;
		outp = inp;
		inp = tp;

		if( fps ) delay( sec, usec );
		if( onestep )
			break;
	}
	fb_close( fbp );
	return	0;
}

#ifdef BSD
#include <sys/time.h>
delay( s, us )
{
	struct timeval tv;

	tv.tv_sec = s;
	tv.tv_usec = us;
	select( 2, (char *)0, (char *)0, (char *)0, &tv );
}
#else
delay( s, us )
{
	sleep( s + (us/1000000) );
}
#endif
