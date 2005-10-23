/*                       S U N - P I X . C
 * BRL-CAD
 *
 * Copyright (C) 1986-2005 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this file; see the file named COPYING for more
 * information.
 */
/** @file sun-pix.c
 *
 *  Program to take Sun bitmap files created with Sun's ``screendump''
 *  command, and convert them to pix(5) format files.
 *
 *  Authors -
 *	Phillip Dykstra
 *	Michael John Muuss
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 */
#ifndef lint
static const char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "common.h"

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "machine.h"
#include "bu.h"


/*
 * Description of Sun header for files containing raster images
 */
struct rasterfile {
	int	ras_magic;		/* magic number */
	int	ras_width;		/* width (pixels) of image */
	int	ras_height;		/* height (pixels) of image */
	int	ras_depth;		/* depth (1, 8, or 24 bits) of pixel */
	int	ras_length;		/* length (bytes) of image */
	int	ras_type;		/* type of file; see RT_* below */
	int	ras_maptype;		/* type of colormap; see RMT_* below */
	int	ras_maplength;		/* length (bytes) of following map */
	/* color map follows for ras_maplength bytes, followed by image */
} header;

char	inbuf[sizeof(struct rasterfile)];

#define	RAS_MAGIC	0x59a66a95

	/* Sun supported ras_type's */
#define RT_OLD		0	/* Raw pixrect image in 68000 byte order */
#define RT_STANDARD	1	/* Raw pixrect image in 68000 byte order */
#define RT_BYTE_ENCODED	2	/* Run-length compression of bytes */
#define RT_EXPERIMENTAL 0xffff	/* Reserved for testing */

	/* Sun registered ras_maptype's */
#define RMT_RAW		2
	/* Sun supported ras_maptype's */
#define RMT_NONE	0	/* ras_maplength is expected to be 0 */
#define RMT_EQUAL_RGB	1	/* red[ras_maplength/3],green[],blue[] */

/*
 * NOTES:
 * 	Each line of the image is rounded out to a multiple of 16 bits.
 *   This corresponds to the rounding convention used by the memory pixrect
 *   package (/usr/include/pixrect/memvar.h) of the SunWindows system.
 *	The ras_encoding field (always set to 0 by Sun's supported software)
 *   was renamed to ras_length in release 2.0.  As a result, rasterfiles
 *   of type 0 generated by the old software claim to have 0 length; for
 *   compatibility, code reading rasterfiles must be prepared to compute the
 *   true length from the width, height, and depth fields.
 */

int	pixout = 1;		/* 0 = bw(5) output, 1 = pix(5) output */
int	colorout = 0;
int	hflag;
int	inverted;
int	pure;			/* No Sun header */
int	verbose;
struct colors {
	unsigned char	CL_red;
	unsigned char	CL_green;
	unsigned char	CL_blue;
};
struct colors Cmap[256];

static char	*file_name;
static FILE	*fp;

char	usage[] = "\
Usage: sun-pix [-b -h -i -P -v -C] [sun.bitmap]\n";


#define NET_LONG_LEN	4	/* # bytes to network long */

unsigned long
getlong(char *msgp)
{
	register unsigned char *p = (unsigned char *) msgp;
	register unsigned long u;

	u = *p++; u <<= 8;
	u |= *p++; u <<= 8;
	u |= *p++; u <<= 8;
	return (u | *p);
}

int
get_args(int argc, register char **argv)
{
	register int c;

	while ( (c = bu_getopt( argc, argv, "bhiPvC" )) != EOF )  {
		switch( c )  {
		case 'b':
			pixout = 0;	/* bw(5) */
			break;
		case 'C':
			colorout = 1;	/* output just the color map */
			break;
		case 'h':
			hflag = 1;	/* print header */
			break;
		case 'i':
			inverted = 1;
			break;
		case 'P':
			pure = 1;
			break;
		case 'v':
			verbose = 1;
			break;

		default:		/* '?' */
			return(0);
		}
	}

	if( bu_optind >= argc )  {
		if( isatty(fileno(stdin)) )
			return(0);
		file_name = "-";
		fp = stdin;
	} else {
		file_name = argv[bu_optind];
		if( (fp = fopen(file_name, "r")) == NULL )  {
			(void)fprintf( stderr,
				"sun-pix: cannot open \"%s\" for reading\n",
				file_name );
			return(0);
		}
	}

	if ( argc > ++bu_optind )
		(void)fprintf( stderr, "sun-pix: excess argument(s) ignored\n" );

	return(1);		/* OK */
}


/*
 * Encode/decode functions for RT_BYTE_ENCODED images:
 *
 * The "run-length encoding" is of the form
 *
 *	<byte><byte>...<ESC><0>...<byte><ESC><count><byte>...
 *
 * where the counts are in the range 0..255 and the actual number of
 * instances of <byte> is <count>+1 (i.e. actual is 1..256). One- or
 * two-character sequences are left unencoded; three-or-more character
 * sequences are encoded as <ESC><count><byte>.  <ESC> is the character
 * code 128.  Each single <ESC> in the input data stream is encoded as
 * <ESC><0>, because the <count> in this scheme can never be 0 (actual
 * count can never be 1).  <ESC><ESC> is encoded as <ESC><1><ESC>.
 *
 * This algorithm will fail (make the "compressed" data bigger than the
 * original data) only if the input stream contains an excessive number of
 * one- and two-character sequences of the <ESC> character.
 */

#define ESCAPE		128

int
decoderead(unsigned char *buf, int size, int length, FILE *fp)

   		     		/* should be one! */
   		       		/* number of items to read */
    		    		/* input file pointer */
{
	static	int	repeat = -1;
	static	int	lastchar = 0;
	int		number_read;

	number_read = 0;

	if (size != 1) {
		fprintf(stderr,"decoderead: unable to process size = %d.\n",
			size);
		exit(1);
	}

	while (length) {
		if (repeat >= 0) {
			*buf = lastchar;
			--length;
			++buf;
			number_read++;
			--repeat;
		} else {
			lastchar = getc(fp);
			if (lastchar < 0) return(number_read);
			if (lastchar == ESCAPE) {
				repeat = getc(fp);
				if (repeat <0) return(number_read);
				if (repeat == 0) {
					*buf = ESCAPE;
					++buf;
					number_read++;
					--length;
					--repeat;
				} else {
					lastchar = getc(fp);
					if (lastchar < 0) return(number_read);
				}
			} else {
				*buf = lastchar;
				--length;
				++buf;
				++number_read;
			}
		}
	}
	return(number_read);
}

unsigned char bits[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };

int
main(int argc, char **argv)
{
	register int	x;
	register int	off = 0;
	register int	on = 255;
	register int	width;			/* line width in bits */
	register int	scanbytes;		/* bytes/line (padded to 16 bits) */
	unsigned char	buf[4096];

	fp = stdin;
	if ( !get_args( argc, argv ) || (isatty(fileno(stdout)) && (hflag == 0)) ) {
		(void)fputs(usage, stderr);
		exit( 1 );
	}
	if( inverted ) {
		off = 255;
		on = 0;
	}

	if( !pure )  {
		register long nbits;

		fread( inbuf, sizeof(struct rasterfile), 1, fp );

		header.ras_magic = getlong( &inbuf[NET_LONG_LEN*0] );
		header.ras_width = getlong( &inbuf[NET_LONG_LEN*1] );
		header.ras_height = getlong( &inbuf[NET_LONG_LEN*2] );
		header.ras_depth = getlong( &inbuf[NET_LONG_LEN*3] );
		header.ras_length = getlong( &inbuf[NET_LONG_LEN*4] );
		header.ras_type = getlong( &inbuf[NET_LONG_LEN*5] );
		header.ras_maptype = getlong( &inbuf[NET_LONG_LEN*6] );
		header.ras_maplength = getlong( &inbuf[NET_LONG_LEN*7] );

		if( header.ras_magic != RAS_MAGIC )  {
			fprintf(stderr,
				"sun-pix: bad magic number, was x%x, s/b x%x\n",
				header.ras_magic, RAS_MAGIC );
			exit(1);
		}

		/* Width is rounded up to next multiple of 16 bits */
		nbits = header.ras_width * header.ras_depth;
		nbits = (nbits + 15) & ~15;
		header.ras_width = nbits / header.ras_depth;

		if(verbose)  {
			fprintf( stderr,
				"ras_width = %d, ras_height = %d\nras_depth = %d, ras_length = %d\n",
				header.ras_width, header.ras_height,
				header.ras_depth, header.ras_length );
			fprintf( stderr,
				"ras_type = %d, ras_maptype = %d, ras_maplength = %d\n",
				header.ras_type,
				header.ras_maptype,
				header.ras_maplength );
		}
		if( hflag ) {
			printf( "-w%d -n%d\n", header.ras_width, header.ras_height );
			exit( 0 );
		}
	} else {
		/* "pure" bitmap */
		header.ras_type = RT_STANDARD;
		header.ras_depth = 1;
	}

	switch( header.ras_type )  {
	case RT_OLD:		/* ??? */
	case RT_BYTE_ENCODED:
	case RT_STANDARD:
		break;
	default:
		fprintf(stderr,"sun-pix:  Unable to process type %d images\n",
			header.ras_type );
		exit(1);
	}

	width = header.ras_width;
	x = 0;

	switch( header.ras_depth )  {
	case 1:
		/* 1-bit image */
		/*  Gobble colormap -- ought to know what to do with it */
		for( x=0; x<header.ras_maplength; x++)  {
			(void)getc(fp);
		}
		if (colorout) {
			fprintf(stdout,"%d\t%04x %04x %04x\n",off,off<<8,
			    off<<8,off<<8);
			fprintf(stdout,"%d\t%04x %04x %04x\n",on,on<<8,
			    on<<8,on<<8);
			break;
		}

		scanbytes = ((width + 15) & ~15L) / 8;
		while( (header.ras_type == RT_BYTE_ENCODED) ?
		    decoderead(buf, sizeof(*buf), scanbytes, fp) :
		    fread(buf, sizeof(*buf), scanbytes, fp) ) {
			for( x = 0; x < width; x++ ) {
				if( buf[x>>3] & bits[x&7] ) {
					putchar(on);
					if(pixout){putchar(on);putchar(on);}
				} else {
					putchar(off);
					if(pixout){putchar(off);putchar(off);}
				}
			}
		}
		break;
	case 8:
		/* 8-bit image */
		if (header.ras_maptype != RMT_EQUAL_RGB) {
			fprintf(stderr,"sun-pix:  unable to handle depth=8, maptype = %d.\n",
				header.ras_maptype);
			exit(1);
		}
		scanbytes = width;
		for (x = 0; x < header.ras_maplength/3; x++) {
			if (inverted) {
				Cmap[x].CL_red = 255-(unsigned char)getc(fp);
			} else {
				Cmap[x].CL_red = getc(fp);
			}
		}
		for (x = 0; x < header.ras_maplength/3; x++) {
			if (inverted) {
				Cmap[x].CL_green = 255-(unsigned char)getc(fp);
			} else {
				Cmap[x].CL_green = getc(fp);
			}
		}
		for (x = 0; x < header.ras_maplength/3; x++) {
			if (inverted) {
				Cmap[x].CL_blue = 255-(unsigned char) getc(fp);
			} else {
				Cmap[x].CL_blue = getc(fp);
			}
		}
		if (colorout) {
			for (x = 0; x <header.ras_maplength/3; x++) {
				fprintf(stdout,"%d\t%04x %04x %04x\n",
				    x, Cmap[x].CL_red<<8, Cmap[x].CL_green<<8,
				    Cmap[x].CL_blue<<8);
			}
			break;
		}

		while ((header.ras_type == RT_BYTE_ENCODED) ?
		    decoderead(buf, sizeof(*buf), scanbytes, fp):
		    fread(buf, sizeof(*buf), scanbytes, fp) ) {
			for (x=0; x < width; x++ ) {
				if (pixout) {
					putchar(Cmap[buf[x]].CL_red);
					putchar(Cmap[buf[x]].CL_green);
					putchar(Cmap[buf[x]].CL_blue);
				} else {
					putchar(buf[x]);
				}
			}
		}
		break;
	default:
		fprintf(stderr,"sun-pix:  unable to handle depth=%d\n",
			header.ras_depth );
		exit(1);
	}
	exit(0);
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
