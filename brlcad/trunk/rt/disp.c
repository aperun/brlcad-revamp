/*
 *  Quickie program to display spectral curves on the framebuffer.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Status -
 *	Public Domain, Distribution Unlimited.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (ARL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "rtstring.h"
#include "raytrace.h"
#include "spectrum.h"
#include "fb.h"
#include "tcl.h"
#include "tk.h"

int	width = 64;
int	height = 64;

char	*basename = "mtherm";
char	spectrum_name[100];

FBIO	*fbp;

struct rt_spectrum	*spectrum;

struct rt_spect_sample	*ss;

char	*pixels;

fastf_t	maxval, minval;

Tcl_Interp	*interp;
Tk_Window	tkwin;

int	doit(), doit1();

int
getspectrum( cd, interp, argc, argv )
ClientData	cd;
Tcl_Interp	*interp;
int		argc;
char		*argv[];
{
	int	wl;

	if( argc != 2 )  {
		interp->result = "Usage: getspectrum wl";
		return TCL_ERROR;
	}
	wl = atoi(argv[2]);

	RT_CK_SPECTRUM(spectrum);

	if( wl < 0 || wl >= spectrum->nwave )  {
		interp->result = "wavelength out of range";
		return TCL_ERROR;
	}
	sprintf( interp->result, "%g", spectrum->wavel[wl] );
	return TCL_OK;
}

int
getspectval( cd, interp, argc, argv )
ClientData	cd;
Tcl_Interp	*interp;
int		argc;
char		*argv[];
{
	struct rt_spect_sample	*sp;
	int	x, y, wl;
	char	*cp;

	if( argc != 4 )  {
		interp->result = "Usage: getspect x y wl";
		return TCL_ERROR;
	}
	x = atoi(argv[1]);
	y = atoi(argv[2]);
	wl = atoi(argv[3]);

	RT_CK_SPECTRUM(spectrum);

	if( x < 0 || x > width || y < 0 || y > height )  {
		interp->result = "x or y out of range";
		return TCL_ERROR;
	}
	if( wl < 0 || wl >= spectrum->nwave )  {
		interp->result = "wavelength out of range";
		return TCL_ERROR;
	}
	cp = (char *)ss;
	cp = cp + (y * width + x) * RT_SIZEOF_SPECT_SAMPLE(spectrum);
	sp = (struct rt_spect_sample *)cp;
	RT_CK_SPECT_SAMPLE(sp);
	sprintf( interp->result, "%g", sp->val[wl] );
	return TCL_OK;
}

/*
 *  TCL interface to LIBFB.
 *  Points at lower left corner of selected pixel.
 */
int
tcl_fb_cursor( cd, interp, argc, argv )
ClientData	cd;
Tcl_Interp	*interp;
int		argc;
char		*argv[];
{
	FBIO	*ifp;
	int	mode, x, y;

	if( argc != 5 )  {
		interp->result = "Usage: fb_cursor fbp mode x y";
		return TCL_ERROR;
	}
	ifp = (FBIO *)atoi(argv[1]);
	mode = atoi(argv[2]);
	x = atoi(argv[3]);
	y = atoi(argv[4]);

	ifp = fbp;	/* XXX hack, ignore tcl arg. */

	FB_CK_FBIO(ifp);
	if( fb_cursor( ifp, mode, x, y ) < 0 )  {
		interp->result = "fb_cursor got error from library";
		return TCL_ERROR;
	}
	return TCL_OK;
}

int
tcl_appinit(inter)
Tcl_Interp	*inter;
{
	interp = inter;	/* set global var */
	if( Tcl_Init(interp) == TCL_ERROR )  {
		return TCL_ERROR;
	}

	/* Run tk.tcl script */
	if( Tk_Init(interp) == TCL_ERROR )  return TCL_ERROR;

	Tcl_CreateCommand(interp, "fb_cursor", tcl_fb_cursor, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

	Tcl_CreateCommand(interp, "doit", doit, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(interp, "doit1", doit1, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

	Tcl_CreateCommand(interp, "getspectval", getspectval, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(interp, "getspectrum", getspectrum, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

	Tcl_LinkVar( interp, "minval", (char *)&minval, TCL_LINK_DOUBLE );
	Tcl_LinkVar( interp, "maxval", (char *)&maxval, TCL_LINK_DOUBLE );

	Tcl_LinkVar( interp, "width", (char *)&width, TCL_LINK_INT );
	Tcl_LinkVar( interp, "height", (char *)&height, TCL_LINK_INT );

	return TCL_OK;
}

main( argc, argv )
char	**argv;
{
	int	len;
	int	fd;
	int	i;

	if( (fbp = fb_open( NULL, width, height )) == FBIO_NULL )  {
		rt_bomb("Unable to open fb\n");
	}
	fb_view( fbp, width/2, height/2, fb_getwidth(fbp)/width, fb_getheight(fbp)/height );

	/* Read spectrum definition */
	sprintf( spectrum_name, "%s.spect", basename );
	spectrum = (struct rt_spectrum *)rt_read_spectrum( spectrum_name );
	if( spectrum == NULL )  {
		rt_bomb("Unable to read spectrum\n");
	}

	len = width * height * RT_SIZEOF_SPECT_SAMPLE(spectrum);
	ss = (struct rt_spect_sample *)rt_malloc( len, "rt_spect_sample" );
	pixels = rt_malloc( width * height * 3, "pixels[]" );

	if( (fd = open(basename, 0)) <= 0 )  {
		perror(basename);
		rt_bomb("Unable to open spectral samples\n");
	}
	if( read( fd, (char *)ss, len ) != len )  {
		rt_bomb("Read of spectral samples failed\n");
	}
	close(fd);


	find_minmax();
	rt_log("min = %g, max=%g Watts\n", minval, maxval );

	Tk_Main( argc, argv, tcl_appinit );
	/* NOTREACHED */

	return 0;
}

int
doit( cd, interp, argc, argv )
ClientData	cd;
Tcl_Interp	*interp;
int		argc;
char		*argv[];
{
	int	wl;
	char	cmd[96];

	for( wl = 0; wl < spectrum->nwave; wl++ )  {
		sprintf( cmd, "doit1 %d", wl );
		Tcl_Eval( interp, cmd );
	}

}

int
doit1( cd, interp, argc, argv )
ClientData	cd;
Tcl_Interp	*interp;
int		argc;
char		*argv[];
{
	int	wl;
	char	buf[32];

	if( argc != 2 )  {
		interp->result = "Usage: doit1 wavel#";
		return TCL_ERROR;
	}
	wl = atoi(argv[1]);
	if( wl < 0 || wl >= spectrum->nwave )  {
		interp->result = "Wavelength number out of range";
		return TCL_ERROR;
	}

	rt_log("%d: %g um to %g um\n",
		wl,
		spectrum->wavel[wl] * 0.001,
		spectrum->wavel[wl+1] * 0.001 );
	rescale(wl);
	fb_writerect( fbp, 0, 0, width, height, pixels );
	fb_poll(fbp);

	/* set global variables */
	sprintf(buf, "%d", wl);
	Tcl_SetVar(interp, "wavel", buf, TCL_GLOBAL_ONLY);
	sprintf(buf, "%g", spectrum->wavel[wl] * 0.001);
	Tcl_SetVar(interp, "lambda", buf, TCL_GLOBAL_ONLY);

	return TCL_OK;
}

/*
 */
find_minmax()
{
	char			*cp;
	int			todo;
	register fastf_t	max, min;
	int		nbytes;
	int		j;

	cp = (char *)ss;
	nbytes = RT_SIZEOF_SPECT_SAMPLE(spectrum);

	max = -INFINITY;
	min =  INFINITY;

	for( todo = width * height; todo > 0; todo--, cp += nbytes )  {
		struct rt_spect_sample	*sp;
		sp = (struct rt_spect_sample *)cp;
		RT_CK_SPECT_SAMPLE(sp);
		for( j = 0; j < spectrum->nwave; j++ )  {
			register fastf_t	v;

			if( (v = sp->val[j]) > max )  max = v;
			if( v < min )  min = v;
		}
	}
	maxval = max;
	minval = min;
}

/*
 */
rescale(wav)
int	wav;
{
	char		*cp;
	char		*pp;
	int		todo;
	int		nbytes;
	fastf_t		scale;

	cp = (char *)ss;
	nbytes = RT_SIZEOF_SPECT_SAMPLE(spectrum);

	pp = pixels;

	scale = 255 / (maxval - minval);

	for( todo = width * height; todo > 0; todo--, cp += nbytes, pp += 3 )  {
		struct rt_spect_sample	*sp;
		register int		val;

		sp = (struct rt_spect_sample *)cp;
		RT_CK_SPECT_SAMPLE(sp);

		val = (sp->val[wav] - minval) * scale;
		if( val > 255 )  val = 255;
		else if( val < 0 ) val = 0;
		pp[0] = pp[1] = pp[2] = val;
	}
}
