/*
 *			B W - P S . C
 *
 *  Convert a black and white (bw) file to an 8-bit PostScript image.
 *
 *  Author -
 *	Phillip Dykstra
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
#include <math.h>	/* for atof() */
#include <time.h>	/* for ctime() */

#define	DEFAULT_SIZE	6.75		/* default output size in inches */
#define	MAX_BYTES	(64*128)	/* max bytes per image chunk */

extern int	getopt();
extern char	*optarg;
extern int	optind;

static int	encapsulated = 0;	/* encapsulated postscript */
static int	inverse = 0;	/* inverse video (RFU) */
static int	center = 0;	/* center output on 8.5 x 11 page */

static int	width = 512;	/* input size in pixels */
static int	height = 512;
static double	outwidth;	/* output image size in inches */
static double	outheight;
static int	xpoints;	/* output image size in points */
static int	ypoints;

static char	*file_name;
static FILE	*infp;

static char usage[] = "\
Usage: bw-ps [-e] [-c] [-h] [-s squareinsize] [-w in_width] [-n in_height]\n\
        [-S inches_square] [-W width_inches] [-N height_inches] [file.bw]\n";

get_args( argc, argv )
register char **argv;
{
	register int c;

	while ( (c = getopt( argc, argv, "ehics:w:n:S:W:N:" )) != EOF )  {
		switch( c )  {
		case 'e':
			/* Encapsulated PostScript */
			encapsulated++;
			break;
		case 'h':
			/* high-res */
			height = width = 1024;
			break;
		case 'i':
			inverse = 1;
			break;
		case 'c':
			center = 1;
			break;
		case 's':
			/* square file size */
			height = width = atoi(optarg);
			break;
		case 'w':
			width = atoi(optarg);
			break;
		case 'n':
			height = atoi(optarg);
			break;
		case 'S':
			/* square file size */
			outheight = outwidth = atof(optarg);
			break;
		case 'W':
			outwidth = atof(optarg);
			break;
		case 'N':
			outheight = atof(optarg);
			break;

		default:		/* '?' */
			return(0);
		}
	}

	if( optind >= argc )  {
		if( isatty(fileno(stdin)) )
			return(0);
		file_name = "[stdin]";
		infp = stdin;
	} else {
		file_name = argv[optind];
		if( (infp = fopen(file_name, "r")) < 0 )  {
			(void)fprintf( stderr,
				"bw-ps: cannot open \"%s\" for reading\n",
				file_name );
			return(0);
		}
		/*fileinput++;*/
	}

	if ( argc > ++optind )
		(void)fprintf( stderr, "bw-ps: excess argument(s) ignored\n" );

	return(1);		/* OK */
}

main( argc, argv )
int	argc;
char	**argv;
{
	FILE	*ofp = stdout;
	int	num = 0;
	int	scans_per_patch, bytes_per_patch;
	int	y;

	outwidth = outheight = DEFAULT_SIZE;

	if ( !get_args( argc, argv ) )  {
		(void)fputs(usage, stderr);
		exit( 1 );
	}

	if( encapsulated ) {
		xpoints = width;
		ypoints = height;
	} else {
		xpoints = outwidth * 72 + 0.5;
		ypoints = outheight * 72 + 0.5;
	}
	prolog(ofp, file_name, xpoints, ypoints);

	scans_per_patch = MAX_BYTES / width;
	if( scans_per_patch > height )
		scans_per_patch = height;
	bytes_per_patch = scans_per_patch * width;

	for( y = 0; y < height; y += scans_per_patch ) {
		/* start a patch */
		fprintf(ofp, "save\n");
		fprintf(ofp, "%d %d 8 [%d 0 0 %d 0 %d] {<\n ",
			width, scans_per_patch,		/* patch size */
			width, height,			/* total size = 1.0 */
			-y );				/* patch y origin */

		/* data */
		num = 0;
		while( num < bytes_per_patch ) {
			fprintf( ofp, "%02x", getc(infp) );
			if( (++num % 32) == 0 )
				fprintf( ofp, "\n " );
		}

		/* close this patch */
		fprintf(ofp, ">} image\n");
		fprintf(ofp, "restore\n");
	}

	postlog( ofp );
}

prolog( fp, name, width, height )
FILE	*fp;
char	*name;
int	width, height;		/* in points */
{
	long	ltime;

	ltime = time(0);

	if( encapsulated ) {
		fputs( "%!PS-Adobe-2.0 EPSF-1.2\n", fp );
		fputs( "%%Creator: bw-ps\n", fp );
		fprintf(fp, "%%%%CreationDate: %s", ctime(&ltime) );
		fprintf(fp, "%%%%Title: %s\n", name );
		fputs( "%%Pages: 0\n", fp );
	} else {
		fputs( "%!PS-Adobe-1.0\n", fp );
		fputs( "%begin(plot)\n", fp );
		/*fputs( "%%DocumentFonts:  Courier\n", fp );*/
		fprintf(fp, "%%%%Title: %s\n", name );
		fputs( "%%Creator: bw-ps\n", fp );
		fprintf(fp, "%%%%CreationDate: %s", ctime(&ltime) );
	}
	fprintf(fp, "%%%%BoundingBox: 0 0 %d %d\n", width, height );
	fputs( "%%EndComments\n\n", fp );

	if( center ) {
		int	xtrans, ytrans;
		xtrans = (8.5*72 - width)/2.0;
		ytrans = (11*72 - height)/2.0;
		fprintf( fp, "%d %d translate\n", xtrans, ytrans );
	}
	fprintf( fp, "%d %d scale\n\n", width, height );
}

postlog( fp )
FILE	*fp;
{
	if( !encapsulated )
		fputs( "%end(plot)\n", fp );
	if( center )
		fputs( "\nshowpage\n", fp );	/*XXX*/
}
