/*                       P I X T I L E . C
 * BRL-CAD
 *
 * Copyright (c) 1986-2007 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file pixtile.c
 *
 *  Given multiple .pix files with ordinary lines of pixels,
 *  produce a single image with each image side-by-side,
 *  right to left, bottom to top on STDOUT.
 *
 *  Author -
 *	Michael John Muuss
 *
 */
#ifndef lint
static const char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif

#include "machine.h"			/* For bzero */
#include "bu.h"


int file_width = 64;	/* width of input sub-images in pixels (64) */
int file_height = 64;	/* height of input sub-images in scanlines (64) */
int scr_width = 512;	/* number of output pixels/line (512, 1024) */
int scr_height = 512;	/* number of output lines (512, 1024) */
char *base_name;		/* basename of input file(s) */
int framenumber = 0;	/* starting frame number (default is 0) */

char usage[] = "\
Usage: pixtile [-h] [-s squareinsize] [-w file_width] [-n file_height]\n\
	[-S squareoutsize] [-W out_width] [-N out_height]\n\
	[-o startframe] basename [file2 ... fileN] >file.pix\n";


/*
 *			G E T _ A R G S
 */
int
get_args(int argc, register char **argv)
{
	register int c;

	while ( (c = bu_getopt( argc, argv, "hs:w:n:S:W:N:o:" )) != EOF )  {
		switch( c )  {
		case 'h':
			/* high-res */
			scr_height = scr_width = 1024;
			break;
		case 's':
			/* square input file size */
			file_height = file_width = atoi(bu_optarg);
			break;
		case 'w':
			file_width = atoi(bu_optarg);
			break;
		case 'n':
			file_height = atoi(bu_optarg);
			break;
		case 'S':
			scr_height = scr_width = atoi(bu_optarg);
			break;
		case 'W':
			scr_width = atoi(bu_optarg);
			break;
		case 'N':
			scr_height = atoi(bu_optarg);
			break;
		case 'o':
			framenumber = atoi(bu_optarg);
			break;
		default:		/* '?' */
			return(0);	/* Bad */
		}
	}

	if( isatty(fileno(stdout)) )  {
		return(0);	/* Bad */
	}

	if( bu_optind >= argc )  {
		fprintf(stderr, "pixtile: basename or filename(s) missing\n");
		return(0);	/* Bad */
	}

	return(1);		/* OK */
}

int
main(int argc, char **argv)
{
	register int i;
	char *obuf;
	int im_line;		/* number of images across output scanline */
	int im_high;		/* number of images (swaths) high */
	int scanbytes;		/* bytes per input line */
	int swathbytes;		/* bytes per swath of images */
	int image;		/* current sub-image number */
	int rel = 0;		/* Relative image # within swath */
	int maximage;		/* Maximum # of images that will fit */
	int islist = 0;		/* set if a list, zero if basename */
	int is_stream = 0;	/* set if input is stream on stdin */
	char name[256];

	if( !get_args( argc, argv ) )  {
		(void)fputs(usage, stderr);
		exit( 1);
	}

	if( bu_optind+1 == argc )  {
		base_name = argv[bu_optind];
		islist = 0;
		if( base_name[0] == '-' && base_name[1] == '\0' )
			is_stream = 1;
	} else {
		islist = 1;
	}

	if( file_width < 1 ) {
		fprintf(stderr,"pixtile: width of %d out of range\n", file_width);
		exit(12);
	}

	scanbytes = file_width * 3;

	/* number of images across line */
	im_line = scr_width/file_width;

	/* number of images high */
	im_high = scr_height/file_height;

	/* One swath of images */
	swathbytes = scr_width * file_height * 3;

	maximage = im_line * im_high;

	if( (obuf = (char *)malloc( swathbytes )) == (char *)0 )  {
		(void)fprintf(stderr,"pixtile:  malloc %d failure\n", swathbytes );
		exit(10);
	}

	image = 0;
	while( image < maximage )  {
		bzero( obuf, swathbytes );
		/*
		 * Collect together one swath
		 */
		for( rel = 0; rel<im_line; rel++, image++, framenumber++ )  {
			int fd;

			if(image >= maximage )  {
				fprintf(stderr,"\npixtile: frame full\n");
				/* All swaths already written out */
				exit(0);
			}
			fprintf(stderr,"%d ", framenumber);  fflush(stdout);
			if( is_stream )  {
				fd = 0;		/* stdin */
			} else {
				if( islist )  {
					/* See if we read all the files */
					if( bu_optind >= argc )
						goto done;
					strcpy(name, argv[bu_optind++]);
				} else {
					sprintf(name,"%s.%d", base_name, framenumber);
				}
				if( (fd=open(name,0))<0 )  {
					perror(name);
					goto done;
				}
			}
			/* Read in .pix file.  Bottom to top */
			for( i=0; i<file_height; i++ )  {
				register int j;

				/* virtual image l/r offset */
				j = (rel*file_width);

				/* select proper scanline within image */
				j += i*scr_width;

				if( bu_mread( fd, &obuf[j*3], scanbytes ) != scanbytes ) {
				    perror("READ ERROR");
				    break;
				}
			}
			if( fd > 0 )  close(fd);
		}
		(void)write( 1, obuf, swathbytes );
		rel = 0;	/* in case we fall through */
	}
done:
	/* Flush partial frame? */
	if( rel != 0 )
		(void)write( 1, obuf, swathbytes );
	fprintf(stderr,"\n");
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
