/*
 *			I F _ U G . C
 *
 *  Ultra Network Technologies "Ultra Graphics" Display Device.
 *  			PRELIMINARY!
 *
 *  Authors -
 *	Michael John Muuss
 *	Phil Dykstra
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1986 by the United States Army.
 *	All rights reserved.
 *
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include "fb.h"
#include "./fblocal.h"
#include <ultra/ugraf.h>

extern int	fb_sim_readrect(), fb_sim_writerect();

#define	FBSAVE	"/usr/tmp/ultrafb"

static struct UG_PARAM	ug_param;
static struct UG_TBLK	ug_tblk;
static char	*ugbuf, *ugbuf2, *ugcurs;
static int	x_zoom, y_zoom;
static int	x_window, y_window;	/* upper left of window (4th quad) */

_LOCAL_ int	ug_dopen(),
		ug_dclose(),
		ug_dreset(),
		ug_dclear(),
		ug_bread(),
		ug_bwrite(),
		ug_cmread(),
		ug_cmwrite(),
		ug_viewport_set(),
		ug_window_set(),
		ug_zoom_set(),
		ug_curs_set(),
		ug_cmemory_addr(),
		ug_cscreen_addr(),
		ug_help();

/* This is the ONLY thing that we normally "export" */
FBIO ug_interface =  {
	ug_dopen,		/* device_open		*/
	ug_dclose,		/* device_close		*/
	ug_dreset,		/* device_reset		*/
	ug_dclear,		/* device_clear		*/
	ug_bread,		/* buffer_read		*/
	ug_bwrite,		/* buffer_write		*/
	ug_cmread,		/* colormap_read	*/
	ug_cmwrite,		/* colormap_write	*/
	ug_viewport_set,	/* viewport_set		*/
	ug_window_set,		/* window_set		*/
	ug_zoom_set,		/* zoom_set		*/
	ug_curs_set,		/* curs_set		*/
	ug_cmemory_addr,		/* cursor_move_memory_addr */
	ug_cscreen_addr,		/* cursor_move_screen_addr */
	fb_sim_readrect,
	fb_sim_writerect,
/* XXX add help here */
	"Ultra Graphics",		/* device description	*/
	1280,				/* max width		*/
	1024,				/* max height		*/
	"/dev/ug",			/* short device name	*/
	512,				/* default/current width  */
	512,				/* default/current height */
	-1,				/* file descriptor	*/
	PIXEL_NULL,			/* page_base		*/
	PIXEL_NULL,			/* page_curp		*/
	PIXEL_NULL,			/* page_endp		*/
	-1,				/* page_no		*/
	0,				/* page_dirty		*/
	0L,				/* page_curpos		*/
	0L,				/* page_pixels		*/
	0				/* debug		*/
};

ugprint( pp )
register struct UG_PARAM *pp;
{
	register struct UG_TBLK *tp;

	fprintf(stderr, "intern = %d\n", pp->intern);
	fprintf(stderr, "buffer = x%x\n", pp->buffer);
	fprintf(stderr, "dev_id = %s\n", pp->dev_id);
	fprintf(stderr, "dx = %d\n", pp->dx);
	fprintf(stderr, "dy = %d\n", pp->dy);
	fprintf(stderr, "term_type = %d\n", pp->term_type);
	fprintf(stderr, "blank = %d\n", pp->blank);
	fprintf(stderr, "buf_ctl = %d\n", pp->buf_ctl);
	fprintf(stderr, "link = x%x\n\n", pp->link);

	for( tp = pp->link; tp; tp = tp->link )  {
		fprintf(stderr, "addr = x%x\n", tp->addr);
		fprintf(stderr, "tx = %d\n", tp->tx);
		fprintf(stderr, "ty = %d\n", tp->ty);
		fprintf(stderr, "npixel = %d\n", tp->npixel);
		fprintf(stderr, "nline = %d\n", tp->nline);
		fprintf(stderr, "stride = %d\n", tp->stride);
		fprintf(stderr, "link = x%x\n\n", tp->link);
	}
}

_LOCAL_ int
ug_dopen( ifp, file, width, height )
FBIO	*ifp;
char	*file;
int	width, height;
{
	register int	i;
	int	status;
 	FILE	*fp;

	ug_param.dx = ug_param.dy = 0;
	ug_param.buffer = 0;			/* No copy buffer */
	ug_param.dev_id = (char *)0;
	ug_param.term_type = UG_C7400;
	ug_param.blank = 0;
	ug_param.buf_ctl = NULL;
	ug_param.link = NULL;

	if( width <= 0 )
		width = ifp->if_width;
	if( height <= 0 )
		height = ifp->if_height;
	if ( width > ifp->if_max_width) 
		width = ifp->if_max_width;

 	ifp->if_width = width;
	ifp->if_height = height;

	x_zoom = y_zoom = 1;
	x_window = 0;
	y_window = 0;

	if( (ugbuf = malloc( width*height*4 )) == NULL )  {
		fprintf(stderr,"ug_open: malloc failure\n");
		return(-1);
	}
	if( (ugbuf2 = malloc( width*height*4 )) == NULL )  {
		fprintf(stderr,"ug_open: malloc 2 failure\n");
		return(-1);
	}
	if ( ( ugcurs = malloc( 16 * 16 * 4)) == NULL ) {
		fprintf(stderr, "ug_open: malloc failure\n");
		return(-1);
	}

	/* Do this after the malloc call -- UNICOS swapping bug */
	if ( ( status = ugraf ( UG_OPEN | UG_NORESET, &ug_param ) ) != UGE_OK ) {
		perror ( "ugraf open" );
		fprintf(stderr, "ugraf open failed with %d\n", status );
		return(-1);
	}

	if( (fp = fopen(FBSAVE, "r")) != NULL ) {
		fread( ugbuf, 4, height*width, fp );
		fclose( fp );
		unlink(FBSAVE);
	}
	
	return(1);			/* OK */
}

_LOCAL_ int
ug_dclose( ifp )
FBIO	*ifp;
{
	int	status;
	FILE	*fp;
	extern int errno;

	/* save image to file */
	if( (fp = fopen(FBSAVE, "w")) != NULL ) {
		fwrite( ugbuf, 4, ifp->if_height*ifp->if_width, fp );
		fclose( fp );
	} else {
		fprintf( stderr, "can't save framebuffer, errno = %d\n", errno );
	}

	/* Send whole buffer out one last time */
	ug_tblk.tx = 0;
	ug_tblk.ty = 0;
	ug_tblk.npixel = ifp->if_width;
	ug_tblk.nline = ifp->if_height;
	ug_tblk.addr = (int *)ugbuf;
	write_ug ("dclose");

	/* Now, close down */
	if ( ( status = ugraf ( UG_CLOSE, &ug_param ) ) != UGE_OK ) {
		perror ( "ugraf close" );
		fprintf(stderr, "ugraf close failed with %d\n", status );
		exit ( 1 );
	}
	return;
}

_LOCAL_ int
ug_dreset( ifp )
FBIO	*ifp;
{
}

_LOCAL_ int
ug_dclear( ifp, pp )
FBIO	*ifp;
RGBpixel	*pp;
{
	if( pp == RGBPIXEL_NULL )  {
		bzero( ugbuf, ifp->if_width * ifp->if_height * 4 );
	} else {
		register char *cp;
		register int todo;

		cp = &ugbuf[0];
		for( todo = ifp->if_width * ifp->if_height; todo > 0; todo-- )  {
			cp++;
			*cp++ = (*pp)[BLU];
			*cp++ = (*pp)[GRN];
			*cp++ = (*pp)[RED];
		}
	}

	ug_tblk.tx = 0;
	ug_tblk.ty = 0;
	ug_tblk.npixel = ifp->if_width;
	ug_tblk.nline = ifp->if_height;
	ug_tblk.addr = (int *)ugbuf;

	/* Do it twice to fill both buffers */
	write_ug ("dclear1");
	write_ug ("dclear2");

	return(0);
}

_LOCAL_ int
ug_bread( ifp, x, y, pixelp, count )
FBIO	*ifp;
int	x, y;
RGBpixel	*pixelp;
int	count;
{
}

static write_ug (str)
char *str;
{
	int	status;

	ug_tblk.link = NULL;
	ug_tblk.stride = 0;

/**	ug_param.buf_ctl = UG_SW | UG_SW_H;	/* Flip after dump */
	ug_param.buf_ctl = UG_SW;	/* Flip after dump */
	ug_param.link = &ug_tblk;

	if ( ( status = ugraf ( UG_WRITE, &ug_param ) ) != UGE_OK ) {
		perror ( "ugraf write" );
		fprintf(stderr, "ugraf write error %d at %s\n", status, str );
		ugprint( &ug_param );
		return(-1);
	}

#ifndef never
	if ( ( status = ugraf ( UG_WAIT, &ug_param ) ) != UGE_OK ) {
		fprintf(stderr,"ugraf wait failed with %d at %s\n", status, str );
		perror ( "ugraf wait" );
		exit ( 1 );
	}
#endif
}

_LOCAL_ int
ug_bwrite( ifp, x, y, pixelp, count )
FBIO	*ifp;
register int	x, y;
register char	*pixelp;
int	count;
{
	register char	*cp;
	register int	todo;
	int	start_y;
	
	y = ifp->if_height-1 - y;
	start_y = y;
	cp = &ugbuf[ ((y * ifp->if_width) + x)*4 ];
	for( todo = count; todo > 0; todo--, pixelp+=3 )  {
		if( ++x > ifp->if_width )  {
			y--;	/* 1st quadrant now */
			x = 0;
			cp = &ugbuf[ ((y * ifp->if_width) + x)*4 ];
		}
		cp++;
		*cp++ = (pixelp)[BLU];
		*cp++ = (pixelp)[GRN];
		*cp++ = (pixelp)[RED];
	}

	/* check for special zoom/window display */
	if( x_zoom != 1 || y_zoom != 1 || x_window != 0 || y_window != 0 ) {
		zandw( ifp );
		return( count );
	}

	ug_tblk.tx = 0;
	ug_tblk.ty = y;
	ug_tblk.npixel = ifp->if_width;
	ug_tblk.nline = start_y - y +1;
	ug_tblk.addr = (int *)(ugbuf + y*4*ifp->if_width);

	write_ug ( );
	write_ug ( );		/* Simulate a single buffered device */

	return(count);
}

_LOCAL_ int
ug_cmread( ifp, cmp )
FBIO	*ifp;
ColorMap	*cmp;
{
}

_LOCAL_ int
ug_cmwrite( ifp, cmp )
FBIO	*ifp;
ColorMap	*cmp;
{
}

_LOCAL_ int
ug_viewport_set( ifp, left, top, right, bottom )
FBIO	*ifp;
int	left, top, right, bottom;
{
}

_LOCAL_ int
ug_window_set( ifp, x, y )
FBIO	*ifp;
int	x, y;
{
	int	ugx, ugy;

	/*
	 *  To start with, we are given the 1st quadrant coordinates
	 *  of the CENTER of the region we wish to view.  Since the
	 *  Ultra device is fourth quadrant,
	 *  first find the upper left corner of the rectangle
	 *  to window in on, accounting for the zoom factor too.
	 *  Then convert from first to fourth for the Ultra.
	 *  The order of these conversions is significant.
	 */
	ugx = x - (ifp->if_width / x_zoom)/2;
	ugy = y + (ifp->if_height / y_zoom)/2 - 1;
	ugy = ifp->if_height-1-ugy;		/* q1 -> q4 */

	/* save q4 upper left */
	x_window = ugx;
	y_window = ugy;

	zandw( ifp );
}

_LOCAL_ int
ug_zoom_set( ifp, x, y )
FBIO	*ifp;
int	x, y;
{
	/* Window needs to be set as well XXX */
	if( x < 1 )  x=1;
	if( y < 1 )  y=1;
	if( x > 256 )  x=256;
	if( y > 256 )  y=256;

	x_zoom = x;
	y_zoom = y;

	zandw( ifp );
}

_LOCAL_ int
ug_curs_set( ifp, bits, xbits, ybits, xorig, yorig )
FBIO	*ifp;
unsigned char *bits;
int	xbits, ybits;
int	xorig, yorig;
{
}

_LOCAL_ int
ug_cmemory_addr( ifp, mode, x, y )
FBIO	*ifp;
int	mode;
int	x, y;
{
	int	i;
	char	*cp;

	/* display a fresh image in both buffers */
	ug_tblk.tx = 0;
	ug_tblk.ty = 0;
	ug_tblk.npixel = ifp->if_width;
	ug_tblk.nline = ifp->if_height;
	ug_tblk.addr = (int *)ugbuf;

	write_ug("cursor");
	write_ug("cursor");

	/* build a cursor */
	cp = &ugcurs[0];	
	
	for(i = 0; i <= 16 * 16; i++) {
		cp++;
		*cp++ = 255;
		*cp++ = 255;
		*cp++ = 255;
	}

	y = ifp->if_height-1-y;		/* q1 -> q4 */
	y = y - y_window;
	x = x - x_window;
	x *= x_zoom;
	y *= y_zoom;

	ug_tblk.tx = x;
	ug_tblk.tx &= ~03;
	ug_tblk.ty = y;
	ug_tblk.npixel = 16;
	ug_tblk.nline = 16;
	ug_tblk.addr = (int *)ugcurs;	

	write_ug();
}

_LOCAL_ int
ug_cscreen_addr( ifp, mode, x, y )
FBIO	*ifp;
int	mode;
int	x, y;
{
	int	i;
	char	*cp;

	/* write a fresh image in both buffers */
	ug_tblk.tx = 0;
	ug_tblk.ty = 0;
	ug_tblk.npixel = ifp->if_width;
	ug_tblk.nline = ifp->if_height;
	ug_tblk.addr = (int *)ugbuf;

	write_ug("cursor");
	write_ug("cursor");
	
	/* build a cursor */
	cp = &ugcurs[0];	
	
	for(i = 0; i <= 16 * 16; i++) {
		cp++;
		*cp++ = 255;
		*cp++ = 255;
		*cp++ = 255;
	}
	
	y = ifp->if_width-1-y;		/* 1st quadrant */

	ug_tblk.tx = x;
	ug_tblk.tx &= ~03;
	ug_tblk.ty = y;
	ug_tblk.npixel = 16;
	ug_tblk.nline = 16;
	ug_tblk.addr = (int *)ugcurs;	

	write_ug();
}

/* zoom and window */
zandw( ifp )
FBIO *ifp;
{
	int	x, y;
	int	xz, yz;
	int	numx, numy;
	char	*ip, *op;

/*	return;*/

	/* bound the window parameters - XXX */
	if( x_window < 0 ) x_window = 0;
	if( y_window < 0 ) y_window = 0;

	bzero( ugbuf2, ifp->if_width*ifp->if_height*4 );	/*XXX*/

	numx = (ifp->if_width-x_window) / x_zoom;
	numy = (ifp->if_height-y_window) / y_zoom;

fprintf( stderr, "numx,y= %d, %d; zoomx,y= %d, %d; windowx,y= %d, %d\n", numx, numy, x_zoom, y_zoom, x_window, y_window );

	for( y = 0; y < numy; y++ ) {
		for( yz = 0; yz < y_zoom; yz++ ) {
			for( x = 0; x < numx; x++ ) {
				ip = &ugbuf[ ((y+y_window)*ifp->if_width+(x+x_window))*4 ];
				for( xz = 0; xz < x_zoom; xz++ ) {
					op = &ugbuf2[ ((y*y_zoom+yz)*ifp->if_width
					      + (x*x_zoom+xz))*4 ];
					*op++ = *ip;
					*op++ = ip[1];
					*op++ = ip[2];
					*op++ = ip[3];
				}
			}
		}
	}

	ug_tblk.tx = 0;
	ug_tblk.ty = 0;
	ug_tblk.npixel = ifp->if_width;
	ug_tblk.nline = ifp->if_height;
	ug_tblk.addr = (int *)(ugbuf2);

	write_ug();
}

_LOCAL_ int
ug_help( ifp )
FBIO	*ifp;
{
	fb_log( "Description: %s\n", ug_interface.if_type );
	fb_log( "Device: %s\n", ifp->if_name );
	fb_log( "Max width/height: %d %d\n",
		ug_interface.if_max_width,
		ug_interface.if_max_height );
	fb_log( "Default width/height: %d %d\n",
		ug_interface.if_width,
		ug_interface.if_height );
	return(0);
}
