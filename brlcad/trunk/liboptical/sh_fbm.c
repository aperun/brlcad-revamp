/*
 *			S H _ F B M . C
 *  Author -
 *	Lee A. Butler
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Notice -
 *	Re-distribution of this software is restricted, as described in
 *	your "Statement of Terms and Conditions for the Release of
 *	The BRL-CAD Package" license agreement.
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 1997 by the United States Army
 *	in all countries except the USA.  All rights reserved.
 */
#ifndef lint
static char RCSsh_fbm[] = "@(#)$Header$ (ARL)";
#endif

#include "conf.h"

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "shadefuncs.h"
#include "shadework.h"
#include "rtprivate.h"

#if !defined(M_PI)
#define M_PI            3.14159265358979323846
#endif

struct fbm_specific {
	double	lacunarity;
	double	h_val;
	double	octaves;
	double	offset;
	double	gain;
	double	distortion;
	point_t	scale;	/* scale coordinate space */
};

static struct fbm_specific fbm_defaults = {
	2.1753974,	/* lacunarity */
	1.0,		/* h_val */
	4,		/* octaves */
	0.0,		/* offset */
	0.0,		/* gain */
	1.0,		/* distortion */
	{ 1.0, 1.0, 1.0 }	/* scale */
	};

#define FBM_NULL	((struct fbm_specific *)0)
#define FBM_O(m)	offsetof(struct fbm_specific, m)
#define FBM_AO(m)	bu_offsetofarray(struct fbm_specific, m)

struct bu_structparse fbm_parse[] = {
	{"%f",	1, "lacunarity",	FBM_O(lacunarity),	BU_STRUCTPARSE_FUNC_NULL },
	{"%f",	1, "H", 		FBM_O(h_val),		BU_STRUCTPARSE_FUNC_NULL },
	{"%f",	1, "octaves", 		FBM_O(octaves),		BU_STRUCTPARSE_FUNC_NULL },
	{"%f",	1, "gain",		FBM_O(gain),		BU_STRUCTPARSE_FUNC_NULL },
	{"%f",	1, "distortion",	FBM_O(distortion),	BU_STRUCTPARSE_FUNC_NULL },
	{"%f",	1, "l",			FBM_O(lacunarity),	BU_STRUCTPARSE_FUNC_NULL },
	{"%d",	1, "o", 		FBM_O(octaves),		BU_STRUCTPARSE_FUNC_NULL },
	{"%f",	1, "g",			FBM_O(gain),		BU_STRUCTPARSE_FUNC_NULL },
	{"%f",	1, "d",			FBM_O(distortion),	BU_STRUCTPARSE_FUNC_NULL },
	{"%f",  3, "scale",		FBM_AO(scale),		BU_STRUCTPARSE_FUNC_NULL },
	{"",	0, (char *)0,		0,			BU_STRUCTPARSE_FUNC_NULL }
};

HIDDEN int	fbm_setup(), fbm_render();
HIDDEN void	fbm_print(), fbm_free();

struct mfuncs fbm_mfuncs[] = {
	{"bump_fbm",	0,		0,		MFI_NORMAL|MFI_HIT|MFI_UV,
	fbm_setup,	fbm_render,	fbm_print,	fbm_free },

	{(char *)0,	0,		0,		0,
	0,		0,		0,		0 }
};



/*
 *	F B M _ S E T U P
 */
HIDDEN int
fbm_setup( rp, matparm, dpp )
register struct region *rp;
struct bu_vls	*matparm;
char	**dpp;
{
	register struct fbm_specific *fbm;

	BU_CK_VLS( matparm );
	BU_GETSTRUCT( fbm, fbm_specific );
	*dpp = (char *)fbm;

	memcpy(fbm, &fbm_defaults, sizeof(struct fbm_specific) );
	if (rdebug&RDEBUG_SHADE)
		bu_log("fbm_setup\n");

	if (bu_struct_parse( matparm, fbm_parse, (char *)fbm ) < 0 )
		return(-1);

	if (rdebug&RDEBUG_SHADE)
		bu_struct_print( rp->reg_name, fbm_parse, (char *)fbm );

	return(1);
}

/*
 *	F B M _ P R I N T
 */
HIDDEN void
fbm_print( rp, dp )
register struct region *rp;
char	*dp;
{
	bu_struct_print( rp->reg_name, fbm_parse, (char *)dp );
}

/*
 *	F B M _ F R E E
 */
HIDDEN void
fbm_free( cp )
char *cp;
{
	bu_free( cp, "fbm_specific" );
}

/*
 *	F B M _ R E N D E R
 */
int
fbm_render( ap, pp, swp, dp )
struct application	*ap;
struct partition	*pp;
struct shadework	*swp;
char	*dp;
{
	register struct fbm_specific *fbm_sp =
		(struct fbm_specific *)dp;
	vect_t v_noise;
	point_t pt;

	if (rdebug&RDEBUG_SHADE)
		bu_struct_print( "foo", fbm_parse, (char *)fbm_sp );

	pt[0] = swp->sw_hit.hit_point[0] * fbm_sp->scale[0];
	pt[1] = swp->sw_hit.hit_point[1] * fbm_sp->scale[1];
	pt[2] = swp->sw_hit.hit_point[2] * fbm_sp->scale[2];

	bn_noise_vec(pt, v_noise);

	VSCALE(v_noise, v_noise, fbm_sp->distortion);

	if (rdebug&RDEBUG_SHADE)
		bu_log("fbm_render: point (%g %g %g) becomes (%g %g %g)\n\tv_noise (%g %g %g)\n",
			V3ARGS(swp->sw_hit.hit_point),
			V3ARGS(pt),
			V3ARGS(v_noise));

	VADD2(swp->sw_hit.hit_normal, swp->sw_hit.hit_normal, v_noise);
	VUNITIZE(swp->sw_hit.hit_normal);

	return(1);
}
