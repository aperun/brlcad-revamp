
/*	R T . H  --- header file for the "rt" program
 *
 *
 *
 *	Things to support the "rt" program and the link to liboptical
 *
 */
/* wray.c */
extern void wray(register struct partition *pp,
		 register struct application *ap,
		 FILE *fp,
		 const vect_t	inormal);

extern void wraypts(vect_t	in,
		    vect_t	inorm,
		    vect_t	out,
		    int		id,
		    struct application *ap,
		    FILE	*fp);

extern void wraypaint(vect_t start,
		      vect_t norm,
		      int paint, 
		      struct application *ap,
		      FILE *fp);

/* shade.c */


/* opt.c */
extern int get_args( int argc, char **argv );
extern int		query_x;
extern int		query_y;
extern int		Query_one_pixel;
extern int		query_rdebug;
extern int		query_debug;

/* view.c */
#if 0
extern void view_eol(register struct application *ap);
extern void do_run( int a, int b );
extern void view_2init( register struct application *ap, char	*framename);
extern void view_setup(struct rt_i	*rtip);
extern void view_pixel(register struct application *ap);
extern void view_cleanup(struct rt_i	*rtip);
extern int view_end(struct application *ap);
extern int view_init(register struct application *ap,
		     char *file, 
		     char *obj,
		     int minus_o);
#else
extern void view_eol();
extern void do_run();
extern void view_2init();
extern void view_setup();
extern void view_pixel();
extern void view_cleanup();
extern void view_end();
extern int view_init();

#endif
/* do.c */
extern void do_ae(double azim, double elev);
extern int do_frame( int framenumber );
extern void def_tree( register struct rt_i *rtip );
extern int old_way( FILE *fp );

/*
 *
 *  
 *  Debugging flags for thr RT program itself.
 *  These flags follow the "-X" (cap X) option to the RT program.
 *  librt debugging is separately controlled.
 *  
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 *  
 *  $Revision$
 */
#define RDEBUG_OFF	0	/* No debugging */

extern int	rdebug;

/* These definitions are each for one bit */
/* Should be reogranized to put most useful ones first */
#define RDEBUG_HITS	0x00000001	/* 1 Print hits used by view() */
#define RDEBUG_MATERIAL	0x00000002	/* 2 Material properties */
#define RDEBUG_SHOWERR	0x00000004	/* 3 Colorful markers on errors */
#define RDEBUG_RTMEM	0x00000008	/* 4 Debug librt mem after startup */
#define RDEBUG_SHADE	0x00000010	/* 5 Shading calculation */
#define RDEBUG_PARSE	0x00000020	/* 6 Command parsing */
#define RDEBUG_LIGHT	0x00000040	/* 7 Debug lighting */
#define RDEBUG_REFRACT	0x00000080	/* 8 Debug reflection & refraction */

#define RDEBUG_STATS	0x00000200	/* 10 Print more statistics */
#define RDEBUG_RTMEM_END 0x00000400	/* 11 Print librt mem use on 'clean' */

/* These will cause binary debugging output */
#define RDEBUG_MISSPLOT	0x20000000	/* 30 plot(5) missed rays to stdout */
#define RDEBUG_RAYWRITE	0x40000000	/* 31 Ray(5V) view rays to stdout */
#define RDEBUG_RAYPLOT	0x80000000	/* 32 plot(5) rays to stdout */

/* Format string for rt_printb() */
#define RDEBUG_FORMAT	\
"\020\040RAYPLOT\037RAYWRITE\036MISSPLOT\
\013RTMEM_END\
\012STATS\010REFRACT\
\7LIGHT\6PARSE\5SHADE\4RTMEM\3SHOWERR\2MATERIAL\1HITS"


/*
 *	A Bit vector to determine how much stuff rt prints when not in
 *	debugging mode.
 *
 */
extern int	rt_verbosity;
/*	   flag_name		value		prints */
#define VERBOSE_LIBVERSIONS  0x00000001	/* Library version strings */
#define VERBOSE_MODELTITLE   0x00000002	/* model title */
#define VERBOSE_TOLERANCE    0x00000004	/* model tolerance */
#define VERBOSE_STATS	     0x00000008	/* stats about rt_gettrees() */
#define VERBOSE_FRAMENUMBER  0x00000010	/* current frame number */
#define VERBOSE_VIEWDETAIL   0x00000020	/* view specifications */
#define VERBOSE_LIGHTINFO    0x00000040	/* scene lights */
#define VERBOSE_INCREMENTAL  0x00000080	/* progressive/incremental state */
#define VERBOSE_MULTICPU     0x00000100	/* # of CPU's to be used */
#define VERBOSE_OUTPUTFILE   0x00000200	/* name of output image */

#define VERBOSE_FORMAT \
"\012OUTPUTFILE\011MULTICPU\010INCREMENTAL\7LIGHTINFO\6VIEWDETAIL\
\5FRAMENUMBER\4STATS\3TOLERANCE\2MODELTITLE\1LIBVERSIONS"
