/*
	Author:		Gary S. Moss
			U. S. Army Ballistic Research Laboratory
			Aberdeen Proving Ground
			Maryland 21005-5066
*/
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#ifndef DEBUG
#define NDEBUG
#define STATIC static
#else
#define STATIC
#endif

#include <assert.h>

#include <stdio.h>

#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "fb.h"

#include "./vecmath.h"
#include "./extern.h"

#define DEBUG_CELLFB	0
#define DEBUG_SPALLFB	0

/* Offset frame buffer coordinates to center of cell from grid lines. */
#define CenterCell(b)	(b += round( (fastf_t) zoom / 2.0 ))

/* Compare one RGB pixel to another. */
#define	SAMERGB(a,b)	((a)[RED] == (b)[RED] &&\
			 (a)[GRN] == (b)[GRN] &&\
			 (a)[BLU] == (b)[BLU])

static RGBpixel	pixbuf[MAXDEVWID];
static int	gridxmargin;
static int	gridymargin;

void
gridToFb( gx, gy, fxp, fyp )
int	gx, gy;
int	*fxp, *fyp;
	{
	/* map grid offsets to frame buffer coordinates */
	*fxp = (gx - gridxorg)*zoom + gridxmargin;
	*fyp = (gy - gridyorg)*zoom + gridymargin;
	return;
	}

void
paintGridFb()
	{	register int	gx, gy;
		register int	fx, fy;
		int		fxbeg, fybeg;
		int		fxfin, fyfin;
		int		fxorg, fyorg;
		int		fgridwid;
	if( fb_clear( fbiop, pixbkgr ) == -1 )
		return;
	gridxmargin = (devwid - (gridwidth*zoom)) / 2.0;
	gridymargin = (devhgt - (gridheight*zoom)) / 2.0;
	gridToFb( gridxorg, gridyorg, &fxbeg, &fybeg );
	gridToFb( gridxfin+1, gridyfin+1, &fxfin, &fyfin );
	gridToFb( 0, 0, &fxorg, &fyorg );
	CenterCell( fxorg ); /* center of cell */
	CenterCell( fyorg );
	if( zoom == 1 )
		{
		fgridwid = gridwidth + 2;
		fxfin++;
		}
	else
		fgridwid = gridwidth * zoom + 1;

	/* draw vertical lines */
	for( fx = 1; fx < fgridwid; fx++ )
		COPYRGB( pixbuf[fx], pixbkgr );
	for( fx = fxbeg; fx <= fxfin; fx += zoom )
		COPYRGB( pixbuf[fx-fxbeg], pixgrid[0] );
	for( fy = fybeg; fy <= fyfin; fy++ )
		(void) fb_write( fbiop, fxbeg, fy, pixbuf, fgridwid );
	for( fy = 0; fy < devwid; fy++ )
		(void) fb_write( fbiop, fxorg, fy, pixaxis, 1 );

	/* draw horizontal lines */
	if( zoom > 1 )
		for( fy = fybeg; fy <= fyfin; fy += zoom )
			(void) fb_write( fbiop,
					 fxbeg, fy, pixgrid, fgridwid );		for( fx = 0; fx < devwid; fx++ )
		COPYRGB( pixbuf[fx], pixaxis );
	(void) fb_write( fbiop, 0, fyorg, pixbuf, devwid );
	return;
	}

void
paintCellFb( ap, pixpaint, pixexpendable )
register struct application	*ap;
RGBpixel			*pixpaint;
RGBpixel			*pixexpendable;
	{	int		gx, gy;
		register int	gyfin, gxfin;
		register int	gxorg, gyorg;
		register int	x, y;
		int		cnt;
#if DEBUG_CELLFB
	rt_log( "paintCellFb: expendable {%d,%d,%d}\n",
		(*pixexpendable)[RED],
		(*pixexpendable)[GRN],
		(*pixexpendable)[BLU] );
#endif
	gridToFb( ap->a_x, ap->a_y, &gx, &gy );
	gxorg = gx+1;
	gyorg = gy+1;
	gxfin = zoom == 1 ? gx+zoom+1 : gx+zoom;
	gyfin = zoom == 1 ? gy+zoom+1 : gy+zoom;
	cnt = gxfin - gxorg;
	for( y = gyorg; y < gyfin; y++ )
		{
		if( zoom != 1 && (y - gy) % zoom == 0 )
			continue;
		RES_ACQUIRE( &rt_g.res_stats );
		(void) fb_read( fbiop, gxorg, y, pixbuf, cnt );
		RES_RELEASE( &rt_g.res_stats );
		for( x = gxorg; x < gxfin; x++ )
			{
			if( SAMERGB( pixbuf[x-gxorg], *pixexpendable )
				)
				{
#if DEBUG_CELLFB
				rt_log( "Clobbering:<%d,%d>{%d,%d,%d}\n",
					x, y,
					(pixbuf[x-gxorg])[RED],
					(pixbuf[x-gxorg])[GRN],
					(pixbuf[x-gxorg])[BLU] );
#endif
				COPYRGB( pixbuf[x-gxorg], *pixpaint );
				}
#if DEBUG_CELLFB
			else
				rt_log( "Preserving:<%d,%d>{%d,%d,%d}\n",
					x, y,
					(pixbuf[x-gxorg])[RED],
					(pixbuf[x-gxorg])[GRN],
					(pixbuf[x-gxorg])[BLU] );
#endif
			}
		RES_ACQUIRE( &rt_g.res_stats );
		(void) fb_write( fbiop, gxorg, y, pixbuf, cnt );
		RES_RELEASE( &rt_g.res_stats );
#if DEBUG_CELLFB
		rt_log( "paintCellFb: fb_write(%d,%d)\n", x, y );
#endif
		}
	return;
	}

void
paintSpallFb( ap )
register struct application	*ap;
	{	RGBpixel pixel;
		int x, y;
		int err;
		fastf_t	celldist;
#if DEBUG_SPALLFB
	rt_log( "paintSpallFb: a_x=%d a_y=%d a_cumlen=%g cellsz=%g zoom=%d\n",
		ap->a_x, ap->a_y, ap->a_cumlen, cellsz, zoom );
#endif
	pixel[RED] = ap->a_color[RED] * 255;
	pixel[GRN] = ap->a_color[GRN] * 255;
	pixel[BLU] = ap->a_color[BLU] * 255;
	gridToFb( ap->a_x, ap->a_y, &x, &y );
	CenterCell( x );	/* center of cell */
	CenterCell( y );
	celldist = ap->a_cumlen/cellsz * zoom;
	x = round( x + Dot( ap->a_ray.r_dir, gridhor ) * celldist );
	y = round( y + Dot( ap->a_ray.r_dir, gridver ) * celldist );
	RES_ACQUIRE( &rt_g.res_stats );
	err = fb_write( fbiop, x, y, pixel, 1 );
	RES_RELEASE( &rt_g.res_stats );
#if DEBUG_SPALLFB
	rt_log( "paintSpallFb:gridhor=<%g,%g,%g> gridver=<%g,%g,%g>\n",
		gridhor[X], gridhor[Y], gridhor[Z],
		gridver[X], gridver[Y], gridver[Z] );
	rt_log( "paintSpallFb:fb_write(x=%d,y=%d,pixel={%d,%d,%d})\n",
		x, y,
		(int) pixel[RED],
		(int) pixel[GRN],
		(int) pixel[BLU]
		);
#endif
	if( err == -1 )
		rt_log( "Write failed to pixel <%d,%d> from cell <%d,%d>.\n",
			x, y, ap->a_x, ap->a_y );
	return;
	}
