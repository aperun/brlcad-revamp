/*
 *			F B _ P A G E D _ I O . C
 * 
 *  Buffered frame buffer IO routines:
 *    fb_ioinit( fbp )
 *    fb_seek( fbp, x, y )
 *    fb_tell( fbp )
 *    fb_rpixel( fbp, pixelp )
 *    fb_wpixel( fbp, pixelp )
 *    fb_flush( fbp )
 *
 *  Authors -
 *	Mike J. Muuss
 *	Douglas P. Kingston III
 *	Gary S. Moss
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
#include "./fblocal.h"

#define PAGE_BYTES	(63*1024L)		/* Max # of bytes/dma. */
#define PAGE_PIXELS	(((PAGE_BYTES/sizeof(RGBpixel))/ifp->if_width)\
			 *ifp->if_width)
#define PAGE_SCANS	(ifp->if_ppixels/ifp->if_width)

_LOCAL_ int	_pagein(), _pageout();
 
/*
 *		F B _ I O I N I T
 *
 *  This initialization routine must be called before any buffered
 *  I/O routines in this file are used.
 */
int
fb_ioinit( ifp )
register FBIO	*ifp;
{
	if( ifp->if_debug & FB_DEBUG_BIO ) {
		fb_log( "fb_ioinit( 0x%lx )\n", (unsigned long)ifp );
	}

	ifp->if_pno = -1;		/* Force _pagein() initially.	*/
	ifp->if_pixcur = 0L;		/* Initialize pixel number.	*/
	if( ifp->if_pbase == PIXEL_NULL ) {
		/* Only allocate buffer once. */
		ifp->if_ppixels = PAGE_PIXELS;	/* Pixels/page.	*/
		if( ifp->if_ppixels > ifp->if_width * ifp->if_height )
			ifp->if_ppixels = ifp->if_width * ifp->if_height;
		if( (ifp->if_pbase = (RGBpixel *)malloc( ifp->if_ppixels * sizeof(RGBpixel) ))
			== PIXEL_NULL ) {
			Malloc_Bomb(ifp->if_ppixels * sizeof(RGBpixel));
			return	-1;
		}
	}
	ifp->if_pcurp = ifp->if_pbase;	/* Initialize pointer.	*/
	ifp->if_pendp = ifp->if_pbase+ifp->if_ppixels;
	return	0;
}

int
fb_seek( ifp, x, y )
register FBIO	*ifp;
int	x, y;
{
	long	pixelnum;
	long	pagepixel;

	if( ifp->if_debug & FB_DEBUG_BIO ) {
		fb_log( "fb_seek( 0x%lx, %d, %d )\n",
			(unsigned long)ifp, x, y );
	}

	if( x < 0 || y < 0 || x >= ifp->if_width || y >= ifp->if_height ) {
		fb_log(	"fb_seek : illegal address <%d,%d>.\n", x, y );
		return	-1;
	}
	pixelnum = ((long) y * (long) ifp->if_width) + x;
	pagepixel = (long) ifp->if_pno * ifp->if_ppixels;
	if( pixelnum < pagepixel || pixelnum >= (pagepixel + ifp->if_ppixels) ) {
		if( ifp->if_pdirty )
			if( _pageout( ifp ) == - 1 )
				return	-1;
		if( _pagein( ifp, (int) (pixelnum / (long) ifp->if_ppixels)) == -1 )
			return	-1;
	}
	ifp->if_pixcur = pixelnum;
	/* Compute new pixel pointer into page. */
	ifp->if_pcurp = ifp->if_pbase +
			(ifp->if_pixcur % ifp->if_ppixels);
	return	0;
}

int
fb_tell( ifp, xp, yp )
register FBIO	*ifp;
int	*xp, *yp;
{
	*yp = (int) (ifp->if_pixcur / ifp->if_width);
	*xp = (int) (ifp->if_pixcur % ifp->if_width);

	if( ifp->if_debug & FB_DEBUG_BIO ) {
		fb_log( "fb_tell( 0x%lx, 0x%x, 0x%x ) => (%4d,%4d)\n",
			(unsigned long)ifp, xp, yp, *xp, *yp );
	}

	return	0;
}

int
fb_wpixel( ifp, pixelp )
register FBIO	*ifp;
RGBpixel 	*pixelp;
{
#ifdef NEVER
	if( ifp->if_debug & FB_DEBUG_BRW ) {
		fb_log( "fb_wpixel( 0x%lx, &[%3d,%3d,%3d] )\n",
			(unsigned long)ifp,
			(*pixelp)[RED], (*pixelp)[GRN], (*pixelp)[BLU] );
	}
#endif NEVER

	if( ifp->if_pno == -1 )
		if( _pagein( ifp, ifp->if_pixcur / ifp->if_ppixels ) == -1 )
			return	-1;

	COPYRGB( *(ifp->if_pcurp), *pixelp );
	ifp->if_pcurp++;	/* position in page */
	ifp->if_pixcur++;	/* position in framebuffer */
	ifp->if_pdirty = 1;	/* page referenced (dirty) */
	if( ifp->if_pcurp >= ifp->if_pendp ) {
		if( _pageout( ifp ) == -1 )
			return	-1;
		ifp->if_pno = -1;
	}
	return	0;
}

int
fb_rpixel( ifp, pixelp )
register FBIO	*ifp;
RGBpixel	*pixelp;
{
	if( ifp->if_pno == -1 )
		if( _pagein( ifp, ifp->if_pixcur / ifp->if_ppixels ) == -1 )
			return	-1;

	COPYRGB( *pixelp, *(ifp->if_pcurp) );
	ifp->if_pcurp++;	/* position in page */
	ifp->if_pixcur++;	/* position in framebuffer */
	if( ifp->if_pcurp >= ifp->if_pendp ) {
		if( _pageout( ifp ) == -1 )
			return	-1;
		ifp->if_pno = -1;
	}

#ifdef NEVER
	if( ifp->if_debug & FB_DEBUG_BRW ) {
		fb_log( "fb_rpixel( 0x%lx, 0x%lx ) => [%3d,%3d,%3d]\n",
			(unsigned long)ifp, (unsigned long)pixelp,
			(*pixelp)[RED], (*pixelp)[GRN], (*pixelp)[BLU] );
	}
#endif NEVER

	return	0;
}

int
fb_flush( ifp )
register FBIO	*ifp;
{
	if( ifp->if_debug & FB_DEBUG_BIO ) {
		fb_flush( "fb_flush( 0x%lx )\n", (unsigned long)ifp );
	}

	if( ifp->if_pdirty )
		if( _pageout( ifp ) == -1 )
			return	-1;

	ifp->if_pdirty = 0;
	return	0;
}

_LOCAL_ int
_pageout( ifp )
register FBIO	*ifp;
{
	int	scans, first_scan;

	/*fb_log( "_pageout(%d)\n", ifp->if_pno );*/

	if( ifp->if_pno < 0 )	/* Already paged out, return 1.	*/
		return	1;

	first_scan = ifp->if_pno * PAGE_SCANS;
	if( first_scan + PAGE_SCANS > ifp->if_height )
		scans = ifp->if_height - first_scan;
	else
		scans = PAGE_SCANS;

	return	fb_write(	ifp,
				0,
				first_scan,
				ifp->if_pbase,
				scans * ifp->if_width
				);
}

_LOCAL_ int
_pagein( ifp, pageno )
register FBIO	*ifp;
int	pageno;
{
	int	scans, first_scan;

	/*fb_log( "_pagein(%d)\n", pageno );*/

	/* Set pixel pointer to beginning of page. */
	ifp->if_pcurp = ifp->if_pbase;
	ifp->if_pno = pageno;
	ifp->if_pdirty = 0;

	first_scan = ifp->if_pno * PAGE_SCANS;
	if( first_scan + PAGE_SCANS > ifp->if_height )
		scans = ifp->if_height - first_scan;
	else
		scans = PAGE_SCANS;

	return	fb_read(	ifp,
				0,
				first_scan,
				ifp->if_pbase,
				scans * ifp->if_width
				);
}
