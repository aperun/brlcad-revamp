/*
	SCCS id:	@(#) librle.c	1.6
	Last edit: 	3/26/85 at 17:47:29	G S M
	Retrieved: 	8/13/86 at 10:27:49
	SCCS archive:	/m/cad/librle/RCS/s.librle.c

	Author : Gary S. Moss, BRL.

	These routines are appropriate for RLE encoding a framebuffer image
	from a program.
	This library is derived from 'ik-rle' code authored by :
		Mike Muuss, BRL.  10/18/83.
		[With a heavy debt to Spencer Thomas, U. of Utah,
		 for the RLE file format].
 */
#if ! defined( lint )
static
char	sccsTag[] = "@(#) librle.c	1.6	last edit 3/26/85 at 17:47:29";
#endif
#include <stdio.h>
#include <fb.h>
#include <rle.h>
typedef unsigned char	u_char;

#define CUR red		/* Must be rightmost part of Pixel.		*/

/*	States for run detection					*/
#define	DATA	0
#define	RUN2	1
#define	INRUN	2

/* An addition to the STDIO suite, to avoid using fwrite() for
	writing 2 bytes of data.
 */
#define putshort(s) \
	do { register int v = s; \
		(void) putc( v & 0xFF, fp ); \
		(void) putc( (v>>8) & 0xFF, fp ); \
		if( rle_debug ) \
			{ \
			(void) putc( v & 0xFF, stderr ); \
			(void) putc( (v>>8) & 0xFF, stderr ); \
			} \
	} while( 0 )

#define UPPER 255			/* anything bigger ain't a byte */

/* short instructions */
#define mk_short_1(oper,a1)		/* one argument short */ \
	do { \
		(void) putc( oper, fp ); \
		(void) putc( a1, fp ); \
		if( rle_debug ) \
			{ \
			(void) putc( oper, stderr ); \
			(void) putc( a1, stderr ); \
			} \
	} while( 0 )

#define mk_short_2(oper,a1,a2)		/* two argument short */ \
	do { \
		(void) putc( oper, fp ); \
		(void) putc( a1, fp ); \
		if( rle_debug ) \
			{ \
			(void) putc( oper, stderr ); \
			(void) putc( a1, stderr ); \
			} \
		putshort( a2 ); \
	} while( 0 )

/* long instructions */
#define mk_long_1(oper,a1) \
	do { /* one argument long */ \
		(void) putc( LONG|(oper), fp ); \
		(void) putc( 0, fp ); \
		if( rle_debug ) \
			{ \
			(void) putc( LONG|(oper), stderr ); \
			(void) putc( 0, stderr ); \
			} \
		putshort( a1 ); \
	} while( 0 )

#define mk_long_2(oper,a1,a2) \
	do { /* two argument long */ \
		if( rle_debug ) \
			{ \
			(void) putc( LONG|(oper), stderr ); \
			(void) putc( 0, stderr ); \
			} \
		(void) putc( LONG|(oper), fp ); \
		(void) putc( 0, fp ); \
		putshort( a1 ); \
		putshort( a2 ); \
	} while( 0 )

/* Choose between long and short format instructions.			*/
#define mk_inst_1(oper,a1)    /* one argument inst */ \
	do { \
		if( a1 > UPPER )	mk_long_1(oper,a1); \
		else			mk_short_1(oper,a1); \
	} while( 0 )

#define mk_inst_2(oper,a1,a2) /* two argument inst */ \
	do { \
		if( a1 > UPPER )	mk_long_2(oper,a1,a2); \
		else			mk_short_2(oper,a1,a2); \
	} while( 0 )

/* Skip one or more blank lines in the RLE file.			*/
#define SkipBlankLines(nblank)	RSkipLines(nblank)

/* Select a color and do "carriage return" to start of scanline.
	color: 0 = Red, 1 = Green, 2 = Blue.
 */
#define SetColor(c)		RSetColor( _bw_flag ? 0 : c )

/* Skip a run of background.						*/
#define SkipPixels(nskip)	if( (nskip) > 0 ) RSkipPixels(nskip)

/* Output an enumerated set of intensities for current color channel.	*/
#define PutRun(color, num)	RRunData(num-1,color)

/* Opcode definitions.							*/
#define	RSkipLines(_n) \
		do { \
			if( rle_debug ) \
				(void)fprintf(stderr,"RSkipLines(%d)\n",(_n)); \
		  	mk_inst_1(RSkipLinesOp,(_n)); \
		} while( 0 )

#define	RSetColor(_c) \
		do { \
			if( rle_debug ) \
				(void)fprintf(stderr,"RSetColor(%d)\n",(_c)); \
			mk_short_1(RSetColorOp,(_c)); \
		} while( 0 )

#define	RSkipPixels(_n) \
		do { \
			if( rle_debug ) \
				(void)fprintf(stderr,"RSkipPixels(%d)\n",(_n)); \
			mk_inst_1(RSkipPixelsOp,(_n)); \
		} while( 0 )

#define	RNewLine() \
		do { \
			if( rle_debug ) \
				(void)fprintf(stderr,"RNewLine()\n"); \
			RSkipLines(1); \
		} while( 0 )

#define	RByteData(_n) \
		do { \
			if( rle_debug ) \
				(void)fprintf(stderr,"RByteData(%d)\n",_n); \
			mk_inst_1(RByteDataOp,_n); \
		} while( 0 )
#define	RRunData(_n,_c) \
		do { \
			if( rle_debug ) \
				(void)fprintf(stderr, \
					"RRunData(%d,%d)\n",_n,_c); \
			mk_inst_2(RRunDataOp,(_n),(_c)); \
		} while( 0 )

#define NSEG	1024/3		/* Number of run segments of storage */
static struct runs
	{
	Pixel *first;
	Pixel *last;
	} runs[NSEG];		/* ptrs to non-background run segs */

/* Global data.								*/
int	_bg_flag;
int	_bw_flag;
int	_cm_flag;
Pixel	_bg_pixel;

int	_ncmap = 3;	/* Default : (3) channels in color map.		*/
int	_cmaplen = 8;	/* Default : (8) log base 2 entries in map.	*/
int	_pixelbits = 8;	/* Default : (8) bits per pixel.		*/
int	rle_debug = 0;
int	rle_verbose = 0;

#define HIDDEN
HIDDEN void	_put_Data();
HIDDEN int	_put_Color_Map_Seg();
HIDDEN int	_put_Std_Map();
HIDDEN int	_get_Color_Map_Seg();
HIDDEN int	_get_Short();

/* Functions to read instructions, depending on format.			*/
HIDDEN int	(*_func_Get_Inst)();	/* Ptr to appropriate function.	*/
HIDDEN int	_get_Old_Inst();	/* Old format inst. reader.	*/
HIDDEN int	_get_New_Inst();	/* New extended inst. reader.	*/

static Xtnd_Rle_Header	w_setup;	/* Header being written out.	*/
static Xtnd_Rle_Header	r_setup;	/* Header being read in.	*/

/*	r l e _ w h d r ( )
 	This routine should be called after 'setfbsize()', unless the
	framebuffer image is the default size (512).
	This routine should be called before 'rle_encode_ln()' to set up
	the global data: _bg_flag, _bw_flag, _cm_flag, and _bg_pixel.
	Returns -1 for failure, 0 otherwise.
 */
rle_whdr( fp, ncolors, bgflag, cmflag, bgpixel )
FILE		*fp;
int		ncolors, bgflag, cmflag;
Pixel		*bgpixel;
	{
	/* Magic numbers for output file.				*/
	register int	bbw;
	static int	x_magic = XtndRMAGIC; /* Extended magic number.	*/

	/* If black and white mode, compute NTSC value of background.	*/
	if( ncolors == 1 )
		{
		if( rle_verbose )
			(void) fprintf( stderr,
					"Image being saved as monochrome.\n"
					);
		bbw = 0.35*bgpixel->red+0.55*bgpixel->green+0.1*bgpixel->blue;
		}
	w_setup.h_xpos = 0;
	w_setup.h_ypos = 0;
	w_setup.h_xlen = _fbsize;
	w_setup.h_ylen = _fbsize;
	w_setup.h_flags = bgflag ? H_CLEARFIRST : 0;
	w_setup.h_ncolors = ncolors;
	w_setup.h_pixelbits = _pixelbits;
	w_setup.h_ncmap = cmflag ? _ncmap : 0;
	w_setup.h_cmaplen = _cmaplen;
	w_setup.h_background[0] = ncolors == 0 ? bbw : bgpixel->red;
	w_setup.h_background[1] = ncolors == 0 ? bbw : bgpixel->green;
	w_setup.h_background[2] = ncolors == 0 ? bbw : bgpixel->blue;

	if( fseek( fp, 0L, 0 ) == -1 )
		{
		(void) fprintf( stderr, "Seek to RLE header failed!\n" );
		return	-1;
		}
	if( fwrite( (char *) &x_magic, sizeof(short), 1, fp ) != 1 )
		{
		(void) fprintf( stderr, "Write of magic number failed!\n" );
		return	-1;
		}
	if( rle_debug )
		(void) fprintf( stderr, "Magic=0x%x\n", x_magic );
	if( fwrite( (char *) &w_setup, sizeof w_setup, 1, fp ) != 1 )
		{
		(void) fprintf( stderr, "Write of RLE header failed!\n" );
		return	-1;
		}
	if( rle_debug )
		prnt_XSetup( "Setup structure written", &w_setup );
	_bg_flag = bgflag;
	_bw_flag = ncolors == 1;
	_cm_flag = cmflag;
	_bg_pixel = *bgpixel;
	return	0;
	}

/*	r l e _ r h d r ( )
	This routine should be called before 'rle_decode_ln()' or 'rle_rmap()'
	to position the file pointer correctily and set up the global flags
	_bw_flag and _cm_flag, and to fill in _bg_pixel if necessary, and
	to pass information back to the caller in flags, bgpixel, xlen,
	ylen, xpos and ypos.
	Returns -1 for failure, 0 otherwise.
 */
rle_rhdr( fp, flags, bgpixel, xlen, ylen, xpos, ypos )
FILE		*fp;
int		*flags;
register Pixel	*bgpixel;
int		*xlen, *ylen;
int		*xpos, *ypos;
	{
	static short	x_magic;
	static char	*verbage[] =
		{
		"Frame buffer image saved in Old Run Length Encoded form\n",
		"Frame buffer image saved in B&W RLE form\n",
		"File not in RLE format, can't display (magic=0x%x)\n",
		"Saved with background color %d %d %d\n",
		"Saved in overlay mode\n",
		"Saved as a straight box image\n",
		NULL
		};

	if( fseek( fp, 0L, 0 ) == -1 )
		{
		(void) fprintf( stderr, "Seek to RLE header failed!\n" );
		return	-1;
		}
	if( fread( (char *)&x_magic, sizeof(short), 1, fp ) != 1 )
		{
		(void) fprintf( stderr, "Read of magic word failed!\n" );
		return	-1;
		}
	if( rle_debug )
		(void) fprintf( stderr, "Magic=0x%x\n", x_magic );
	if( x_magic != XtndRMAGIC )
		{ Old_Rle_Header	setup;
		if( fread( (char *) &setup, sizeof(setup), 1, fp ) != 1 )
			{
			(void) fprintf( stderr,
					"Read of Old RLE header failed!\n"
					);
			return	-1;
			}
		r_setup.h_xpos = setup.xpos;
		r_setup.h_ypos = setup.ypos;
		r_setup.h_xlen = setup.xsize;
		r_setup.h_ylen = setup.ysize;
		switch( x_magic & ~0xff) {
		case RMAGIC :
			if( rle_verbose )
				(void) fprintf( stderr,	verbage[0] );
			r_setup.h_ncolors = 3;
			break;
		case WMAGIC :
			if( rle_verbose )
				(void) fprintf( stderr, verbage[1] );
			r_setup.h_ncolors = 1;
			break;
		default:
			(void) fprintf(	stderr, verbage[2], x_magic & ~0xff);
			return	-1;
		} /* End switch */
		switch( x_magic & 0xFF ) {
		case 'B' : /* Background given.				*/
			r_setup.h_flags = H_CLEARFIRST;
			r_setup.h_background[0] = setup.bg_r;
			r_setup.h_background[1] = setup.bg_g;
			r_setup.h_background[2] = setup.bg_b;
			break;
		default: /* Straight 'box' save.			*/
			r_setup.h_flags = H_BOXSAVE;
			r_setup.h_background[0] = 0;
			r_setup.h_background[1] = 0;
			r_setup.h_background[2] = 0;
			if( rle_verbose )
				(void) fprintf( stderr, verbage[5] );
			break;
		} /* End switch */
		r_setup.h_pixelbits = 8;
		r_setup.h_ncmap = setup.map ? 3 : 0;
		r_setup.h_cmaplen = 8;
		_func_Get_Inst = _get_Old_Inst;
		} /* End if */
	else
		{
		if( fread( (char *)&r_setup, sizeof(Xtnd_Rle_Header), 1, fp )
		    != 1
			)
			{
			(void) fprintf( stderr,
					"Read of RLE header failed!\n"
					);
			return	-1;
			}
		_func_Get_Inst = _get_New_Inst;
		}
	if( rle_verbose )
		(void) fprintf( stderr,
				"Positioned at (%d, %d), size (%d %d)\n",
				r_setup.h_xpos,
				r_setup.h_ypos,
				r_setup.h_xlen,
				r_setup.h_ylen
				);
	if( r_setup.h_flags == H_CLEARFIRST )
		{
		if( rle_verbose )
			(void) fprintf( stderr,
					verbage[3],
					r_setup.h_background[0],
					r_setup.h_background[1],
					r_setup.h_background[2]
					);
		if( bgpixel != (Pixel *) NULL )
			{
			/* No command-line backgr., use saved values.	*/
			_bg_pixel.red = r_setup.h_background[0];
			_bg_pixel.green = r_setup.h_background[1];
			_bg_pixel.blue = r_setup.h_background[2];
			*bgpixel = _bg_pixel;
			}
		}
	_bw_flag = r_setup.h_ncolors == 1;
	if( r_setup.h_flags & H_CLEARFIRST )
		*flags = NO_BOX_SAVE;
	else
		*flags = 0;
	if( r_setup.h_ncmap == 0 )
		*flags |= NO_COLORMAP;
	if( r_setup.h_ncolors == 0 )
		*flags |= NO_IMAGE;
	*xlen = r_setup.h_xlen;
	*ylen = r_setup.h_ylen;
	*xpos = r_setup.h_xpos;
	*ypos = r_setup.h_ypos;
	if( rle_debug )
		prnt_XSetup( "Setup structure read", &r_setup );
	return	0;
	}

/*	r l e _ r m a p ( )
	Read a color map in RLE format.
	Returns -1 upon failure, 0 otherwise.
 */
rle_rmap( fp, cmap )
FILE		*fp;
ColorMap	*cmap;
	{
	if( rle_verbose )
		(void) fprintf( stderr, "Reading color map\n");
	if(	_get_Color_Map_Seg( fp, cmap->cm_red ) == -1
	     ||	_get_Color_Map_Seg( fp, cmap->cm_green ) == -1
	     ||	_get_Color_Map_Seg( fp, cmap->cm_blue ) == -1
		)
		return	-1;
	else
		return	0;
	}

/*	r l e _ w m a p ( )
	Write a color map in RLE format.
	Returns -1 upon failure, 0 otherwise.
 */
rle_wmap( fp, cmap )
FILE		*fp;
ColorMap	*cmap;
	{
	if( w_setup.h_ncmap == 0 )
		{
		(void) fprintf( stderr,
		"Writing color map conflicts with header information!\n"
				);
		(void) fprintf( stderr,
				"rle_whdr(arg 2 == 0) No map written.\n"
				);
		return	-1;
		}
	if( rle_verbose )
		(void) fprintf( stderr, "Writing color map\n" );
	if( cmap == (ColorMap *) NULL )
		{
		return _put_Std_Map( fp );
		}		
	if(	_put_Color_Map_Seg( fp, cmap->cm_red ) == -1
	     ||	_put_Color_Map_Seg( fp, cmap->cm_green ) == -1
	     ||	_put_Color_Map_Seg( fp, cmap->cm_blue ) == -1
		)
		return	-1;
	else
		return	0;
	}

/*	r l e _ d e c o d e _ l n ( )
	Decode one scanline into 'scan_buf'.
	Buffer is assumed to be filled with background color.
	Returns -1 on failure, 1 if buffer is altered
	and 0 if untouched.
 */
rle_decode_ln( fp, scan_buf )
FILE	*fp;
Pixel	*scan_buf;
	{
	static int	lines_to_skip = 0;
	static int	opcode, datum;
	static short	word;

	register int	n;
	register u_char	*pp = (u_char *) scan_buf; /* Pointer into pixel. */
	register int	dirty_flag = 0;

	if( lines_to_skip > 0 )
		{
		lines_to_skip--;
		return	dirty_flag;
		}
	while( (*_func_Get_Inst)( fp, &opcode, &datum ) != EOF )
		{
		switch( opcode )
			{
			case RSkipLinesOp :
				lines_to_skip = datum;
				if( rle_debug )
					(void) fprintf( stderr,
							"Skip Lines %d\n",
							lines_to_skip
							);
				if( lines_to_skip-- < 1 )
					return	-1;
				else
					return	dirty_flag;
			case RSetColorOp:
				if( rle_debug )
					(void) fprintf( stderr,
							"Set Color %d\n",
							datum
							);
			/*
			 * Select "color channel" that following ops go to.
			 *
			 * Set `pp' to point to starting pixel element;
			 * by adding STRIDE to pp, will move to corresponding
			 * color element in next pixel.
			 * If Black & White image:  point to left-most
			 * byte (Red for Ikonas) in long,
			 * and Run and Data will ignore strides below.
			 */
				if( _bw_flag )
					n = 0;
				else
					n = datum;
				switch( n )
					{
					case 0:
						pp = &(scan_buf->red);
						break;
					case 1:
						pp = &(scan_buf->green);
						break;
					case 2:
						pp = &(scan_buf->blue);
						break;
					default:
						(void) fprintf( stderr,
							"Bad color %d\n",
								n
								);
						return	-1;
					}
				break;
			case RSkipPixelsOp:
				n = datum;
				if( rle_debug )
					(void) fprintf( stderr,
							"Skip Pixels %d\n",
							n
							);
				pp += n * STRIDE; /* advance pixel ptr */
				break;
			case RByteDataOp:
				{ register int bytes = 0;
				n = datum + 1;
				if( rle_debug )
					(void) fprintf( stderr,
						"Byte Data, count=%d.\n",
							n
							);
	
				if( ! _bw_flag )
					{
					while( n-- > 0 )
						{
						*pp = getc( fp );
						pp += STRIDE;
						bytes++;
						}
					}
				else
					{ /* Ugh, black & white.	*/
					register u_char c;
					while( n-- > 0 )
						{
						*pp++ = c = getc( fp );
						*pp++ = c;
						*pp++ = c;
						*pp++ = c;
						}
					}
				if( feof( fp ) )
					{
					(void) fprintf( stderr,
				"unexpected EOF while reading BYTE DATA\N"
							);
					return	-1;
					}
				if( (datum + 1) & 1 )
					{
					/* word align file ptr */
					(void) getc( fp );
					bytes++;
					}
				dirty_flag = 1;
				if( rle_debug )
					(void) fprintf( stderr, "%d bytes read.\n", bytes );
				break;
			}
			case RRunDataOp:
				n = datum + 1;
				{
				register char *p = (char *) &word;
				*p++ = getc( fp );
				*p++ = getc( fp );
				}
				if(rle_debug)
					(void) fprintf( stderr,	
						"Run-Data(len=%d,inten=%d)\n",
							n,
							word
							);
	
				if( ! _bw_flag )
					{
					register u_char inten = (u_char)word;
					while( n-- > 0 )
						{
						*pp = inten;
						pp += STRIDE;
						}
					}
				else
					{ /* Ugh, black & white.		*/
					while( n-- > 0 )
						{
						*pp++ = (u_char) word;
						*pp++ = (u_char) word;
						*pp++ = (u_char) word;
						}
					}
				dirty_flag = 1;
				break;
			default:
				(void) fprintf( stderr,
					"Unrecognized opcode: %d (x%x x%x)\n",
						opcode, opcode, datum
						);
				if( ! rle_debug )
					return	-1;
			}
		}
	return	dirty_flag;
	}

/* 	r l e _ e n c o d e _ l n ( )
	Encode a given scanline of pixels into RLE format.
	Returns -1 upon failure, 0 otherwise.
 */
rle_encode_ln( fp, scan_buf )
FILE	*fp;
Pixel	*scan_buf;
	{
	register Pixel *scan_p = scan_buf;
	register Pixel *last_p = &scan_buf[w_setup.h_xlen];
	register int	i;
	register int	color;		/* holds current color */
	register int	nseg;		/* number of non-bg run segments */

	if( _bg_flag )
		{
		if( (nseg = _bg_Get_Runs( scan_p, last_p )) == -1 )
			return	-1;
		}
	else
		{
		runs[0].first = scan_p;
		runs[0].last = last_p;
		nseg = 1;
		}
	if( nseg <= 0 )
		{
		RSkipLines( 1 );
		return	0;
		}
	if( _bw_flag )
		{
		register Pixel *pixelp;
		/* Compute NTSC Black & White in blue row.		*/
		for( pixelp=scan_p; pixelp <= last_p; pixelp++ )
			pixelp->blue =  .35 * pixelp->red +
					.55 * pixelp->green +
					.10 * pixelp->blue;
		}

	/* do all 3 colors */
	for( color = 0; color < 3; color++ )
		{
		if( _bw_flag && color != 2 )
			continue;

		SetColor( color );
		if( runs[0].first != scan_p )
			{
			SkipPixels( runs[0].first - scan_p );
			}
		for( i = 0; i < nseg; i++ )
			{
			_encode_Seg_Color( fp, i, color );
			/* Move along to next segment for encoding,
				if this was not the last segment.
			 */
			if( i < nseg-1 )
				SkipPixels( runs[i+1].first-runs[i].last-1 );
			}
		}
	RNewLine();
	return	0;
	}

/*	_ b g _ G e t _ R u n s ( )
	Fill the 'runs' segment array from 'pixelp' to 'endpix'.
	This routine will fail and return -1 if the array fills up
	before all pixels are processed, otherwise a 'nseg' is returned.
 */
_bg_Get_Runs( pixelp, endpix )
register Pixel *pixelp;
register Pixel *endpix;
	{
	/* find non-background runs */
	register int	nseg = 0;
	while( pixelp <= endpix && nseg < NSEG )
		{
		if(	pixelp->red != _bg_pixel.red
		    ||	pixelp->green != _bg_pixel.green
		    ||	pixelp->blue != _bg_pixel.blue
			)
			{
			/* We have found the start of a segment */
			runs[nseg].first = pixelp++;

			/* find the end of this run */
			while(	pixelp <= endpix
			    &&	 (	pixelp->red != _bg_pixel.red
				   ||	pixelp->green != _bg_pixel.green
				   ||	pixelp->blue != _bg_pixel.blue
		    		 )
				)
				pixelp++;

			/* last pixel in segment */
			runs[nseg++].last = pixelp-1;
			}
		pixelp++;
		}
	if( rle_verbose )
		(void) fprintf( stderr," (%d segments)\n", nseg );
	if( nseg >= NSEG )
		{
		(void) fprintf( stderr,
				"Encoding incomplete, " );
		(void) fprintf( stderr, 
				"segment array 'runs[%d]' is full!\n",
				NSEG
				);
		return	-1;
		}
	return	nseg;
	}

/*	_ e n c o d e _ S e g _ C o l o r ( )
	Encode a segment, 'seg', for specified 'color'.
 */
_encode_Seg_Color( fp, seg, color )
FILE	*fp;
int	color;
register int	seg;
	{
	static Pixel *data_p;
	static Pixel *last_p;

	switch( color )
		{
		case 0:
			data_p = (Pixel *) &(runs[seg].first->red);
			last_p = (Pixel *) &(runs[seg].last->red);
			break;
		case 1:
			data_p = (Pixel *) &(runs[seg].first->green);
			last_p = (Pixel *) &(runs[seg].last->green);
			break;
		case 2:
			data_p = (Pixel *) &(runs[seg].first->blue);
			last_p = (Pixel *) &(runs[seg].last->blue);
			break;
		}
	_encode_Segment( fp, data_p, last_p );
	return;
	}

/*	_ e n c o d e _ S e g m e n t ( )
	Output code for segment.
 */
_encode_Segment( fp, data_p, last_p )
FILE		*fp;
register Pixel	*data_p;
register Pixel	*last_p;
	{
	register Pixel	*runs_p = data_p;
	register Pixel	*pixelp;
	register int	state = DATA;
	register u_char	runval = data_p->CUR;

	for( pixelp = data_p + 1; pixelp <= last_p; pixelp++ )
		{
		switch( state )
			{
			case DATA :
				if( runval == pixelp->CUR )
					{
					/* 2 in a row, may be a run */
					state = RUN2;
					}
				else
					{
					/* continue accumulating data, look
						for a new run starting here,
						too
					 */
					runval = pixelp->CUR;
					runs_p = pixelp;
					}
				break;
			case RUN2:
				if( runval == pixelp->CUR )
					{
					/* 3 in a row is a run */
					state = INRUN;
					/* Flush out data sequence
						encountered before this run
					 */
					_put_Data(	fp,
							&(data_p->CUR),
						 	runs_p-data_p
						 	);
					}
				else
					{
					/* not a run, */
					/* but maybe this starts one */
					state = DATA;
					runval = pixelp->CUR;
					runs_p = pixelp;
					}
				break;
			case INRUN:
				if( runval != pixelp->CUR )
					{
					/* if run out */
					state = DATA;
					PutRun(	runval,
						pixelp - runs_p
						);
					/* who knows, might be more */
					runval = pixelp->CUR;
					runs_p = pixelp;
					/* starting new 'data' run */
					data_p = pixelp;
					}
				break;
				}
			}
		/* Write out last portion of section being encoded.	*/
		if( state == INRUN )
			{
			PutRun( runval, pixelp - runs_p );
			}
		else
			{
			_put_Data( fp, &data_p->CUR, pixelp - data_p );
			}
	return;
	}

/*	_ p u t _ D a t a ( )
	Put one or more pixels of byte data into the output file.
 */
HIDDEN void
_put_Data( fp, cp, n )
register FILE	*fp;
register u_char *cp;
int	n;
	{
	register int	count = n;
	register int	bytes = 0;
	if( count == 0 )
		return;
	RByteData(n-1);

	while( count-- > 0 )
		{
		(void) putc( (int) *cp, fp );
		cp += STRIDE;
		++bytes;
		}
	if( n & 1 )
		{
		(void) putc( 0, fp );	/* short align output */
		++bytes;
		}
	if( rle_debug )
		(void) fprintf( stderr, "%d bytes written.\n", bytes );
	return;
	}

/*	_ g e t _ C o l o r _ M a p _ S e g ( )
	Read the color map stored in the RLE file.
	The RLE format stores color map entries as short integers, so
	we have to stuff them into u_chars.
 */
HIDDEN
_get_Color_Map_Seg( fp, cmap_seg )
FILE	*fp;
register u_char	*cmap_seg;
	{
	short	rle_cmap[256];
	register short	*cm = rle_cmap;
	register int	i;

	if( fread( (char *) rle_cmap, sizeof(short), 256, fp ) != 256 )
		{
		(void) fprintf( stderr,	"Failed to read color map!\n" );
		return	-1;
		}
	for( i = 0; i < 256; ++i )
		{
		*cmap_seg++ = (u_char) *cm++;
		}
	return	0;
	}

/*	_ p u t _ C o l o r _ M a p _ S e g ( )
	Output color map segment to RLE file as shorts.
 */
HIDDEN
_put_Color_Map_Seg( fp, cmap_seg )
FILE	*fp;
register u_char	*cmap_seg;
	{
	short		rle_cmap[256];
	register short	*cm = rle_cmap;
	register int	i;

	for( i = 0; i < 256; ++i )
		{
		*cm++ = (short) *cmap_seg++;
		}
	if( fwrite( (char *) rle_cmap, sizeof(rle_cmap), 1, fp ) != 1 )
		{
		(void) fprintf(	stderr,
				"Write of color map segment failed!\n"
				);
		return	-1;
		}
	return	0;
	}

/*	_ p u t _ S t d _ M a p ( )
	Output standard color map to RLE file as shorts.
 */
HIDDEN
_put_Std_Map( fp )
FILE	*fp;
	{
	short		rle_cmap[256*3];
	register short	*cm = rle_cmap;
	register int	i, segment;

	for( segment = 0; segment < 3; segment++ )
		for( i = 0; i < 256; ++i )
			{
			*cm++ = (short) i;
			}
	if( fwrite( (char *) rle_cmap, sizeof(rle_cmap), 1, fp ) != 1 )
		{
		(void) fprintf(	stderr,
				"Write of standard color map failed!\n"
				);
		return	-1;
		}
	return	0;
	}

_get_New_Inst( fp, opcode, datum )
FILE	*fp;
int	*opcode;
int	*datum;
	{
	Xtnd_Inst	instruction;
	if( fread( (char *) &instruction, sizeof(instruction), 1, fp ) != 1 )
		{
		return	EOF;
		}
	if( LONGP(instruction) )
		{ short	long_data;
		*opcode = OPCODE(instruction);
		if( _get_Short( fp, &long_data ) == EOF )
			return	EOF;
		*datum = long_data;
		}
	else
		{
		*opcode = instruction.opcode;
		*datum = DATUM(instruction);
		}
	if( rle_debug )
		(void) fprintf( stderr,
				"op %d, datum %d\n",
				*opcode,
				*datum
				);
	return	1;
	}

_get_Old_Inst( fp, opcode, datum )
FILE	*fp;
int	*opcode;
int	*datum;
	{
	Old_Inst	instruction;
	if( _get_Short( fp, (short *) &instruction ) == EOF )
		return	EOF;
	*opcode = instruction.opcode;
	*datum = instruction.datum;
	if( rle_debug )
		(void) fprintf( stderr,
				"op %d, datum %d\n",
				*opcode,
				*datum
				);
	return	1;
	}
	

/*	_ g e t _ S h o r t ( )
 */
HIDDEN int
_get_Short( fp, word )
FILE	*fp;
short	*word;
	{
	if( fread( (char *) word, sizeof(short), 1, fp ) != 1 )
		{
		(void) fprintf( stderr, "get_Short() Read failed!\n" );
		return	EOF;
		}
	SWAB( *word );
	return	1;
	}

prnt_XSetup( msg, setup )
char				*msg;
register Xtnd_Rle_Header	*setup;
	{
	(void) fprintf( stderr, "%s : \n", msg );
	(void) fprintf( stderr,
			"\th_xpos=%d, h_ypos=%d\n\th_xlen=%d, h_ylen=%d\n",
			setup->h_xpos, setup->h_ypos,
			setup->h_xlen, setup->h_ylen
			);
	(void) fprintf( stderr,
			"\th_flags=0x%x\n\th_ncolors=%d\n\th_pixelbits=%d\n",
			setup->h_flags, setup->h_ncolors, setup->h_pixelbits
			);
	(void) fprintf( stderr,
			"\th_ncmap=%d\n\th_cmaplen=%d\n",
			setup->h_ncmap, setup->h_cmaplen
			);
	(void) fprintf( stderr,
			"\th_background=[%d %d %d]\n",
			setup->h_background[0],
			setup->h_background[1],
			setup->h_background[2]
			);
	return;
	}
