/*
 *  			T E X T . C
 *  
 *  Texture map lookup
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
 */
#ifndef lint
static char RCStext[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./material.h"
#include "./mathtab.h"
#include "./rdebug.h"

HIDDEN int txt_setup(), txt_render(), txt_print(), txt_free();
HIDDEN int ckr_setup(), ckr_render(), ckr_print(), ckr_free();
HIDDEN int bmp_setup(), bmp_render(), bmp_print(), bmp_free();
HIDDEN int tstm_render();
HIDDEN int star_render();
extern int mlib_zero();

struct mfuncs txt_mfuncs[] = {
	"texture",	0,		0,		MFI_UV,
	txt_setup,	txt_render,	txt_print,	txt_free,

	"checker",	0,		0,		MFI_UV,
	ckr_setup,	ckr_render,	ckr_print,	ckr_free,

	"testmap",	0,		0,		MFI_UV,
	mlib_zero,	tstm_render,	mlib_zero,	mlib_zero,

	"fakestar",	0,		0,		0,
	mlib_zero,	star_render,	mlib_zero,	mlib_zero,

	"bump",		0,		0,		MFI_UV|MFI_NORMAL,
	txt_setup,	bmp_render,	txt_print,	txt_free,

	(char *)0,	0,		0,		0,
	0,		0,		0,		0
};

struct txt_specific {
	unsigned char tx_transp[8];	/* RGB for transparency */
	char	tx_file[128];	/* Filename */
	int	tx_w;		/* Width of texture in pixels */
	int	tx_fw;		/* File width of texture in pixels */
	int	tx_l;		/* Length of pixels in lines */
	char	*tx_pixels;	/* Pixel holding area */
};
#define TX_NULL	((struct txt_specific *)0)

struct matparse txt_parse[] = {
#ifndef cray
	"transp",	(mp_off_ty)(TX_NULL->tx_transp),"%C",
	"file",		(mp_off_ty)(TX_NULL->tx_file),	"%s",
#else
	"transp",	(mp_off_ty)0,			"%C",
	"file",		(mp_off_ty)1,			"%s",
#endif
	"w",		(mp_off_ty)&(TX_NULL->tx_w),	"%d",
	"l",		(mp_off_ty)&(TX_NULL->tx_l),	"%d",
	"fw",		(mp_off_ty)&(TX_NULL->tx_fw),	"%d",
	(char *)0,	(mp_off_ty)0,			(char *)0
};

/*
 *			T X T _ R E A D
 *
 *  Load the texture into memory.
 *  Returns 0 on failure, 1 on success.
 */
HIDDEN int
txt_read( tp )
register struct txt_specific *tp;
{
	char *linebuf;
	register FILE *fp;
	register int i;

	if( (fp = fopen(tp->tx_file, "r")) == NULL )  {
		rt_log("txt_read(%s):  can't open\n", tp->tx_file);
		tp->tx_file[0] = '\0';
		return(0);
	}
	linebuf = rt_malloc(tp->tx_fw*3,"texture file line");
	tp->tx_pixels = rt_malloc(
		tp->tx_w * tp->tx_l * 3,
		tp->tx_file );
	for( i=0; i<tp->tx_l; i++ )  {
		if( fread(linebuf,1,tp->tx_fw*3,fp) != tp->tx_fw*3 ) {
			rt_log("txt_read: read error on %s\n", tp->tx_file);
			tp->tx_file[0] = '\0';
			(void)fclose(fp);
			rt_free(linebuf,"file line, error");
			return(0);
		}
		bcopy( linebuf, tp->tx_pixels + i*tp->tx_w*3, tp->tx_w*3 );
	}
	(void)fclose(fp);
	rt_free(linebuf,"texture file line");
	return(1);	/* OK */
}

/*
 *  			T X T _ R E N D E R
 *  
 *  Given a u,v coordinate within the texture ( 0 <= u,v <= 1.0 ),
 *  return a pointer to the relevant pixel.
 *
 *  Note that .pix files are stored left-to-right, bottom-to-top,
 *  which works out very naturally for the indexing scheme.
 */
HIDDEN
txt_render( ap, pp, swp, dp )
struct application	*ap;
struct partition	*pp;
struct shadework	*swp;
char	*dp;
{
	register struct txt_specific *tp =
		(struct txt_specific *)dp;
	fastf_t xmin, xmax, ymin, ymax;
	int line;
	int dx, dy;
	int x,y;
	register long r,g,b;

	/*
	 * If no texture file present, or if
	 * texture isn't and can't be read, give debug colors
	 */
	if( tp->tx_file[0] == '\0'  ||
	    ( tp->tx_pixels == (char *)0 && txt_read(tp) == 0 ) )  {
		VSET( swp->sw_color, swp->sw_uv.uv_u, 0, swp->sw_uv.uv_v );
		return(1);
	}

	/* u is left->right index, v is line number bottom->top */
	/* Don't filter more than 1/8 of the texture for 1 pixel! */
	if( swp->sw_uv.uv_du > 0.125 )  swp->sw_uv.uv_du = 0.125;
	if( swp->sw_uv.uv_dv > 0.125 )  swp->sw_uv.uv_dv = 0.125;

	if( swp->sw_uv.uv_du < 0 || swp->sw_uv.uv_dv < 0 )  {
		rt_log("txt_render uv=%g,%g, du dv=%g %g seg=%s\n",
			swp->sw_uv.uv_u, swp->sw_uv.uv_v, swp->sw_uv.uv_du, swp->sw_uv.uv_dv,
			pp->pt_inseg->seg_stp->st_name );
		swp->sw_uv.uv_du = swp->sw_uv.uv_dv = 0;
	}
	xmin = swp->sw_uv.uv_u - swp->sw_uv.uv_du;
	xmax = swp->sw_uv.uv_u + swp->sw_uv.uv_du;
	ymin = swp->sw_uv.uv_v - swp->sw_uv.uv_dv;
	ymax = swp->sw_uv.uv_v + swp->sw_uv.uv_dv;
	if( xmin < 0 )  xmin = 0;
	if( ymin < 0 )  ymin = 0;
	if( xmax > 1 )  xmax = 1;
	if( ymax > 1 )  ymax = 1;
	x = xmin * (tp->tx_w-1);
	y = ymin * (tp->tx_l-1);
	dx = (xmax - xmin) * (tp->tx_w-1);
	dy = (ymax - ymin) * (tp->tx_l-1);
	if( dx < 1 )  dx = 1;
	if( dy < 1 )  dy = 1;
/** rt_log("x=%d y=%d, dx=%d, dy=%d\n", x, y, dx, dy); **/
	r = g = b = 0;
	for( line=0; line<dy; line++ )  {
		register unsigned char *cp;
		register unsigned char *ep;
		cp = (unsigned char *)(tp->tx_pixels +
		     (y+line) * tp->tx_w * 3  +  x * 3);
		ep = cp + 3*dx;
		while( cp < ep )  {
			r += *cp++;
			g += *cp++;
			b += *cp++;
		}
	}
	r /= (dx*dy);
	g /= (dx*dy);
	b /= (dx*dy);
	/*
	 * Transparency mapping is enabled, and we hit a transparent spot.
	 * Fire another ray to determine the actual color
	 */
#ifndef crayXX
/* UNICOS 2.0 BUG */
	if( tp->tx_transp[3] == 0 ||
	    r != (tp->tx_transp[0]) ||
	    g != (tp->tx_transp[1]) ||
	    b != (tp->tx_transp[2]) )  {
		FAST fastf_t f;
		f = 1.0 / 255.0;
		VSET( swp->sw_color, r * f, g * f, b * f );
		return(1);
	}
#endif
	if( pp->pt_outhit->hit_dist >= INFINITY )  {
		rt_log("txt_render:  transparency on infinite object?\n");
		VSET( swp->sw_color, 0, 1, 0 );
		return(1);
	}
	if( (ap->a_level%100) > 5 )  {
		VSET( swp->sw_color, .1, .1, .1);
		return(1);
	}
	{
		auto struct application sub_ap;
		sub_ap = *ap;		/* struct copy */
		sub_ap.a_level = ap->a_level+1;
		VJOIN1( sub_ap.a_ray.r_pt, ap->a_ray.r_pt,
			pp->pt_outhit->hit_dist, ap->a_ray.r_dir );
		(void)rt_shootray( &sub_ap );
		VMOVE( swp->sw_color, sub_ap.a_color );
	}
	return(1);
}

/*
 *			T X T _ S E T U P
 */
HIDDEN int
txt_setup( rp, matparm, dpp )
register struct region *rp;
char	*matparm;
char	**dpp;
{
	register struct txt_specific *tp;

	GETSTRUCT( tp, txt_specific );
	*dpp = (char *)tp;

	tp->tx_file[0] = '\0';
	tp->tx_w = tp->tx_fw = tp->tx_l = -1;
	mlib_parse( matparm, txt_parse, (mp_off_ty)tp );
	if( tp->tx_w < 0 )  tp->tx_w = 512;
	if( tp->tx_l < 0 )  tp->tx_l = tp->tx_w;
	if( tp->tx_fw < 0 )  tp->tx_fw = tp->tx_w;
	tp->tx_pixels = (char *)0;
	return( txt_read(tp) );
}

/*
 *			T X T _ P R I N T
 */
HIDDEN int
txt_print( rp )
register struct region *rp;
{
	mlib_print(rp->reg_name, txt_parse, (mp_off_ty)rp->reg_udata);
}

/*
 *			T X T _ F R E E
 */
HIDDEN int
txt_free( cp )
char *cp;
{
	if( ((struct txt_specific *)cp)->tx_pixels )
		rt_free( ((struct txt_specific *)cp)->tx_pixels,
			((struct txt_specific *)cp)->tx_file );
	rt_free( cp, "txt_specific" );
}

struct ckr_specific  {
	unsigned char	ckr_a[8];	/* first RGB */
	unsigned char	ckr_b[8];	/* second RGB */
};
#define CKR_NULL ((struct ckr_specific *)0)

struct matparse ckr_parse[] = {
#ifndef cray
	"a",		(mp_off_ty)(CKR_NULL->ckr_a),	"%C",
	"b",		(mp_off_ty)(CKR_NULL->ckr_b),	"%C",
#else
	"a",		(mp_off_ty)0,			"%C",
	"b",		(mp_off_ty)1,			"%C",
#endif
	(char *)0,	(mp_off_ty)0,			(char *)0
};

/*
 *			C K R _ R E N D E R
 */
HIDDEN int
ckr_render( ap, pp, swp, dp )
struct application	*ap;
struct partition	*pp;
register struct shadework	*swp;
char	*dp;
{
	register struct ckr_specific *ckp =
		(struct ckr_specific *)dp;
	auto struct uvcoord uv;
	register unsigned char *cp;
	FAST fastf_t f;

	if( (swp->sw_uv.uv_u < 0.5 && swp->sw_uv.uv_v < 0.5) ||
	    (swp->sw_uv.uv_u >=0.5 && swp->sw_uv.uv_v >=0.5) )  {
		cp = ckp->ckr_a;
	} else {
		cp = ckp->ckr_b;
	}
	f = 1.0/255.;
	VSET( swp->sw_color, cp[0]*f, cp[1]*f, cp[2]*f );
}

/*
 *			C K R _ S E T U P
 */
HIDDEN int
ckr_setup( rp, matparm, dpp )
register struct region *rp;
char	*matparm;
char	**dpp;
{
	register struct ckr_specific *ckp;

	GETSTRUCT( ckp, ckr_specific );
	*dpp = (char *)ckp;
	mlib_parse( matparm, ckr_parse, (mp_off_ty)ckp );
	return(1);
}

/*
 *			C K R _ P R I N T
 */
HIDDEN int
ckr_print( rp )
register struct region *rp;
{
	mlib_print(rp->reg_name, ckr_parse, (mp_off_ty)rp->reg_udata);
}

/*
 *			C K R _ F R E E
 */
HIDDEN int
ckr_free( cp )
char *cp;
{
	rt_free( cp, "ckr_specific" );
}

/*
 *			T S T M _ R E N D E R
 *
 *  Render a map which varries red with U and blue with V values.
 *  Mostly useful for debugging ft_uv() routines.
 */
HIDDEN
tstm_render( ap, pp, swp, dp )
struct application	*ap;
struct partition	*pp;
register struct shadework	*swp;
char	*dp;
{
	VSET( swp->sw_color, swp->sw_uv.uv_u, 0, swp->sw_uv.uv_v );
	return(1);
}

static vect_t star_colors[] = {
	{ 0.825769, 0.415579, 0.125303 },	/* 3000 */
	{ 0.671567, 0.460987, 0.258868 },
	{ 0.587580, 0.480149, 0.376395 },
	{ 0.535104, 0.488881, 0.475879 },
	{ 0.497639, 0.493881, 0.556825 },
	{ 0.474349, 0.494836, 0.624460 },
	{ 0.456978, 0.495116, 0.678378 },
	{ 0.446728, 0.493157, 0.727269 },	/* 10000 */
	{ 0.446728, 0.493157, 0.727269 },	/* fake 11000 */
/***	{ 0.446728, 0.493157, 0.727269 },	/* fake 12000 */
/***	{ 0.446728, 0.493157, 0.727269 },	/* fake 13000 */
/***	{ 0.446728, 0.493157, 0.727269 },	/* fake 14000 */
/***	{ 0.446728, 0.493157, 0.727269 }	/* fake 15000 */
/***	{ 0.393433 0.488079 0.940423 },	/* 20000 */
};

/*
 *			S T A R _ R E N D E R
 */
HIDDEN
star_render( ap, pp, swp, dp )
register struct application *ap;
register struct partition *pp;
struct shadework	*swp;
char	*dp;
{
	/* Probably want to diddle parameters based on what part of sky */
	if( rand0to1() >= 0.98 )  {
		register int i;
		FAST fastf_t f;
		i = (sizeof(star_colors)-1) / sizeof(star_colors[0]);
		i = ((double)i) * rand0to1();
		f = rand0to1();
		VSCALE( swp->sw_color, star_colors[i], f );
	} else {
		VSETALL( swp->sw_color, 0 );
	}
}

struct phong_specific {
	int	shine;
	double	wgt_specular;
	double	wgt_diffuse;
	double	transmit;	/* Moss "transparency" */
	double	reflect;	/* Moss "transmission" */
	double	refrac_index;
} junk = {
	10, 0.7, 0.3, 0, 0, 1.0
};

/*
 *  			B M P _ R E N D E R
 *  
 *  Given a u,v coordinate within the texture ( 0 <= u,v <= 1.0 ),
 *  compute a new surface normal.
 *  For now we come up with a local coordinate system, and
 *  make bump perturbations from the red and blue channels of
 *  an RGB image.
 *
 *  Note that .pix files are stored left-to-right, bottom-to-top,
 *  which works out very naturally for the indexing scheme.
 */
HIDDEN
bmp_render( ap, pp, swp, dp )
struct application	*ap;
struct partition	*pp;
struct shadework	*swp;
char	*dp;
{
	register struct txt_specific *tp =
		(struct txt_specific *)dp;
	unsigned char *cp;
	fastf_t	pertU, pertV;
	vect_t	x, y;		/* world coordinate axis vectors */
	vect_t	u, v;		/* surface coord system vectors */
	int	i, j;		/* bump map pixel indicies */
	char *save;

	/*
	 * If no texture file present, or if
	 * texture isn't and can't be read, give debug color.
	 */
	if( tp->tx_file[0] == '\0'  ||
	    ( tp->tx_pixels == (char *)0 && txt_read(tp) == 0 ) )  {
		VSET( swp->sw_color, swp->sw_uv.uv_u, 0, swp->sw_uv.uv_v );
		return(1);
	}
	/* u is left->right index, v is line number bottom->top */
	if( swp->sw_uv.uv_u < 0 || swp->sw_uv.uv_u > 1 || swp->sw_uv.uv_v < 0 || swp->sw_uv.uv_v > 1 )  {
		rt_log("bmp_render:  bad u,v=%g,%g du,dv=%g,%g seg=%s\n",
			swp->sw_uv.uv_u, swp->sw_uv.uv_v,
			swp->sw_uv.uv_du, swp->sw_uv.uv_dv,
			pp->pt_inseg->seg_stp->st_name );
		VSET( swp->sw_color, 0, 1, 0 );
		return(1);
	}

	/* Find a local coordinate system */
	VSET( x, 1, 0, 0 );
	VSET( y, 0, 1, 0 );
	VCROSS( u, y, swp->sw_hit.hit_normal );
	VUNITIZE( u );
	VCROSS( v, swp->sw_hit.hit_normal, u );

	/* Find our RGB value */
	i = swp->sw_uv.uv_u * (tp->tx_w-1);
	j = swp->sw_uv.uv_v * (tp->tx_l-1);
	cp = (unsigned char *)(tp->tx_pixels +
	     (j) * tp->tx_w * 3  +  i * 3);
	pertU = (*cp - 128) / 256.0;
	pertV = (*(cp+2) - 128) / 256.0;

	VJOIN2( swp->sw_hit.hit_normal, swp->sw_hit.hit_normal, pertU, u, pertV, v );
	VUNITIZE( swp->sw_hit.hit_normal );

	/*phong_render( ap, pp, swp, &junk );*/
	return(1);
}
