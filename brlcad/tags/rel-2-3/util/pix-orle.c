/*
 *			P I X - R L E . C
 *
 *  Encode a .pix file using the RLE library
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
#include "rle.h"

static FILE	*outfp = stdout;
static FILE	*infp = stdin;

static int	file_width = 512;
static int	file_height = 512;

static char	usage[] = "\
Usage: pix-rle [-h -d -v] [-s squarefilesize]\n\
	[-w file_width] [-n file_height] [file.pix] [file.rle]\n\
\n\
If omitted, the .pix file is taken from stdin\n\
and the .rle file is written to stdout\n";

/*
 *			G E T _ A R G S
 */
static int
get_args( argc, argv )
register char	**argv;
{
	register int	c;
	extern int	optind;
	extern char	*optarg;

	while( (c = getopt( argc, argv, "dhs:w:n:v" )) != EOF )  {
		switch( c )  {
		case 'd':
			/* For debugging RLE library */
			rle_debug = 1;
			break;
		case 'h':
			/* high-res */
			file_height = file_width = 1024;
			break;
		case 's':
			/* square file size */
			file_height = file_width = atoi(optarg);
			break;
		case 'w':
			file_width = atoi(optarg);
			break;
		case 'n':
			file_height = atoi(optarg);
			break;
		case 'v':
			/* Verbose */
			rle_verbose = 1;
			break;
		case '?':
			return	0;
		}
	}
	if( argv[optind] != NULL )  {
		if( (infp = fopen( argv[optind], "r" )) == NULL )  {
			perror(argv[optind]);
			return	0;
		}
		optind++;
	}
	if( argv[optind] != NULL )  {
		if( access( argv[optind], 0 ) == 0 )  {
			(void) fprintf( stderr,
				"\"%s\" already exists.\n",
				argv[optind] );
			exit( 1 );
		}
		if( (outfp = fopen( argv[optind], "w" )) == NULL )  {
			perror(argv[optind]);
			return	0;
		}
	}
	if( argc > ++optind )
		(void) fprintf( stderr, "Excess arguments ignored\n" );

	if( isatty(fileno(infp)) || isatty(fileno(outfp)) )
		return 0;
	return	1;
}

/*
 *			M A I N
 */
main( argc, argv )
int	argc;
char	*argv[];
{
	register RGBpixel *scan_buf;
	register int	y;

	if( !get_args( argc, argv ) )  {
		(void)fputs(usage, stderr);
		exit( 1 );
	}
	scan_buf = (RGBpixel *)malloc( sizeof(RGBpixel) * file_width );

	rle_wlen( file_width, file_height, 1 );
	rle_wpos( 0, 0, 1 );		/* Start position is origin */

	/* Write RLE header, ncolors=3, bgflag=0 */
	if( rle_whdr( outfp, 3, 0, 0, RGBPIXEL_NULL ) == -1 )
		return	1;

	/* Read image a scanline at a time, and encode it */
	for( y = 0; y < file_height; y++ )  {
		if(rle_debug)fprintf(stderr,"encoding line %d\n", y);
		if( fread( (char *)scan_buf, sizeof(RGBpixel), file_width, infp ) != file_width)  {
			(void) fprintf(	stderr,
				"read of %d pixels on line %d failed!\n",
				file_width, y );
				return	1;
		}

		if( rle_encode_ln( outfp, scan_buf ) == -1 )
			return	1;
	}
	fclose( infp );
	fclose( outfp );
	return	0;
}
