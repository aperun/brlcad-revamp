/*
 *			D O Z O O M . C
 *
 * Functions -
 *	dozoom		Compute new zoom/rotation perspectives
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
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "./ged.h"
#include "externs.h"
#include "./solid.h"
#include "./sedit.h"
#include "./dm.h"

#define W_AXES 0
#define V_AXES 1
#define E_AXES 2

extern point_t e_axes_pos;
static void draw_axes();

mat_t	incr_change;
mat_t	modelchanges;
mat_t	identity;

/* Screen coords of actual eye position.  Usually it is at (0,0,+1),
 * but in head-tracking and VR applications, it can move.
 */
point_t	eye_pos_scr = { 0, 0, 1 };

struct solid	*FreeSolid;	/* Head of freelist */
struct solid	HeadSolid;	/* Head of solid table */
int		ndrawn;

/*
 *			P E R S P _ M A T
 *
 *  Compute a perspective matrix for a right-handed coordinate system.
 *  Reference: SGI Graphics Reference Appendix C
 *  (Note:  SGI is left-handed, but the fix is done in the Display Manger).
 */
static void
persp_mat( m, fovy, aspect, near, far, backoff )
mat_t	m;
fastf_t	fovy, aspect, near, far, backoff;
{
	mat_t	m2, tran;

	fovy *= 3.1415926535/180.0;

	mat_idn( m2 );
	m2[5] = cos(fovy/2.0) / sin(fovy/2.0);
	m2[0] = m2[5]/aspect;
	m2[10] = (far+near) / (far-near);
	m2[11] = 2*far*near / (far-near);	/* This should be negative */

	m2[14] = -1;		/* XXX This should be positive */
	m2[15] = 0;

	/* Move eye to origin, then apply perspective */
	mat_idn( tran );
	tran[11] = -backoff;
	mat_mul( m, m2, tran );
}

/*
 *
 *  Create a perspective matrix that transforms the +/1 viewing cube,
 *  with the acutal eye position (not at Z=+1) specified in viewing coords,
 *  into a related space where the eye has been sheared onto the Z axis
 *  and repositioned at Z=(0,0,1), with the same perspective field of view
 *  as before.
 *
 *  The Zbuffer clips off stuff with negative Z values.
 *
 *  pmat = persp * xlate * shear
 */
static void
mike_persp_mat( pmat, eye )
mat_t		pmat;
CONST point_t	eye;
{
	mat_t	shear;
	mat_t	persp;
	mat_t	xlate;
	mat_t	t1, t2;
	point_t	sheared_eye;
#if 0
	fastf_t	near, far;
	point_t	a,b;
#endif

	if( eye[Z] < SMALL )  {
		VPRINT("mike_persp_mat(): ERROR, z<0, eye", eye);
		return;
	}

	/* Shear "eye" to +Z axis */
	mat_idn(shear);
	shear[2] = -eye[X]/eye[Z];
	shear[6] = -eye[Y]/eye[Z];

	MAT4X3VEC( sheared_eye, shear, eye );
	if( !NEAR_ZERO(sheared_eye[X], .01) || !NEAR_ZERO(sheared_eye[Y], .01) )  {
		VPRINT("ERROR sheared_eye", sheared_eye);
		return;
	}
#if 0
VPRINT("sheared_eye", sheared_eye);
#endif

	/* Translate along +Z axis to put sheared_eye at (0,0,1). */
	mat_idn(xlate);
	/* XXX should I use MAT_DELTAS_VEC_NEG()?  X and Y should be 0 now */
	MAT_DELTAS( xlate, 0, 0, 1-sheared_eye[Z] );

	/* Build perspective matrix inline, substituting fov=2*atan(1,Z) */
	mat_idn( persp );
	/* From page 492 of Graphics Gems */
	persp[0] = sheared_eye[Z];	/* scaling: fov aspect term */
	persp[5] = sheared_eye[Z];	/* scaling: determines fov */

	/* From page 158 of Rogers Mathematical Elements */
	/* Z center of projection at Z=+1, r=-1/1 */
	persp[14] = -1;

	mat_mul( t1, xlate, shear );
	mat_mul( t2, persp, t1 );
#if 0
	/* t2 has perspective matrix, with Z ranging from -1 to +1.
	 * In order to control "near" and "far clipping planes,
	 * need to scale and translate in Z.
	 * For example, to get Z effective Z range of -1 to +11,
	 * divide Z by 12/2, then xlate by (6-1).
	 */
	t2[10] /= 6;		/* near+far/2 */
	MAT_DELTAS( xlate, 0, 0, -5 );
#else
	/* Now, move eye from Z=1 to Z=0, for clipping purposes */
	MAT_DELTAS( xlate, 0, 0, -1 );
#endif
	mat_mul( pmat, xlate, t2 );
#if 0
mat_print("pmat",pmat);

	/* Some quick checking */
	VSET( a, 0, 0, -1 );
	MAT4X3PNT( b, pmat, a );
	VPRINT("0,0,-1 ->", b);

	VSET( a, 1, 1, -1 );
	MAT4X3PNT( b, pmat, a );
	VPRINT("1,1,-1 ->", b);

	VSET( a, 0, 0, 0 );
	MAT4X3PNT( b, pmat, a );
	VPRINT("0,0,0 ->", b);

	VSET( a, 1, 1, 0 );
	MAT4X3PNT( b, pmat, a );
	VPRINT("1,1,0 ->", b);

	VSET( a, 1, 1, 1 );
	MAT4X3PNT( b, pmat, a );
	VPRINT("1,1,1 ->", b);

	VSET( a, 0, 0, 1 );
	MAT4X3PNT( b, pmat, a );
	VPRINT("0,0,1 ->", b);
#endif
}

/*
 *  Map "display plate coordinates" (which can just be the screen viewing cube), 
 *  into [-1,+1] coordinates, with perspective.
 *  Per "High Resolution Virtual Reality" by Michael Deering,
 *  Computer Graphics 26, 2, July 1992, pp 195-201.
 *
 *  L is lower left corner of screen, H is upper right corner.
 *  L[Z] is the front (near) clipping plane location.
 *  H[Z] is the back (far) clipping plane location.
 *
 *  This corresponds to the SGI "window()" routine, but taking into account
 *  skew due to the eyepoint being offset parallel to the image plane.
 *
 *  The gist of the algorithm is to translate the display plate to the
 *  view center, shear the eye point to (0,0,1), translate back,
 *  then apply an off-axis perspective projection.
 *
 *  Another (partial) reference is "A comparison of stereoscopic cursors
 *  for the interactive manipulation of B-splines" by Barham & McAllister,
 *  SPIE Vol 1457 Stereoscopic Display & Applications, 1991, pg 19.
 */
static void
deering_persp_mat( m, l, h, eye )
mat_t		m;
CONST point_t	l;	/* lower left corner of screen */
CONST point_t	h;	/* upper right (high) corner of screen */
CONST point_t	eye;	/* eye location.  Traditionally at (0,0,1) */
{
	vect_t	diff;	/* H - L */
	vect_t	sum;	/* H + L */

	VSUB2( diff, h, l );
	VADD2( sum, h, l );

	m[0] = 2 * eye[Z] / diff[X];
	m[1] = 0;
	m[2] = ( sum[X] - 2 * eye[X] ) / diff[X];
	m[3] = -eye[Z] * sum[X] / diff[X];

	m[4] = 0;
	m[5] = 2 * eye[Z] / diff[Y];
	m[6] = ( sum[Y] - 2 * eye[Y] ) / diff[Y];
	m[7] = -eye[Z] * sum[Y] / diff[Y];

	/* Multiplied by -1, to do right-handed Z coords */
	m[8] = 0;
	m[9] = 0;
	m[10] = -( sum[Z] - 2 * eye[Z] ) / diff[Z];
	m[11] = -(-eye[Z] + 2 * h[Z] * eye[Z]) / diff[Z];

	m[12] = 0;
	m[13] = 0;
	m[14] = -1;
	m[15] = eye[Z];

/* XXX May need to flip Z ? (lefthand to righthand?) */
}

/*
 *			D O Z O O M
 *
 *	This routine reviews all of the solids in the solids table,
 * to see if they  visible within the current viewing
 * window.  If they are, the routine computes the scale and appropriate
 * screen position for the object.
 */
void
dozoom(which_eye)
int	which_eye;
{
	register struct solid *sp;
	FAST fastf_t ratio;
	fastf_t		inv_viewsize;
	mat_t		pmat;
	mat_t		tmat, tvmat;
	mat_t		new;
	matp_t		mat;

	ndrawn = 0;
	inv_viewsize = 1 / VIEWSIZE;

	/*
	 * Draw all solids not involved in an edit.
	 */
	if( mged_variables.perspective <= 0 && eye_pos_scr[Z] == 1.0 )  {
		mat = model2view;
	} else {
		/*
		 *  There are two strategies that could be used:
		 *  1)  Assume a standard head location w.r.t. the
		 *  screen, and fix the perspective angle.
		 *  2)  Based upon the perspective angle, compute
		 *  where the head should be to achieve that field of view.
		 *  Try strategy #2 for now.
		 */
		fastf_t	to_eye_scr;	/* screen space dist to eye */
		fastf_t eye_delta_scr;	/* scr, 1/2 inter-occular dist */
		point_t	l, h, eye;

		/* Determine where eye should be */
		to_eye_scr = 1 / tan(mged_variables.perspective * rt_degtorad * 0.5);

#define SCR_WIDTH_PHYS	330	/* Assume a 330 mm wide screen */

		eye_delta_scr = mged_variables.eye_sep_dist * 0.5 / SCR_WIDTH_PHYS;

		VSET( l, -1, -1, -1 );
		VSET( h, 1, 1, 200.0 );
if(which_eye) {
printf("d=%gscr, d=%gmm, delta=%gscr\n", to_eye_scr, to_eye_scr * SCR_WIDTH_PHYS, eye_delta_scr);
VPRINT("l", l);
VPRINT("h", h);
}
		VSET( eye, 0, 0, to_eye_scr );

		mat_idn(tmat);
		tmat[11] = -1;
		mat_mul( tvmat, tmat, model2view );

		switch(which_eye)  {
		case 0:
			/* Non-stereo case */
			mat = model2view;
/* XXX hack */
#define HACK 0
#if !HACK
if( 1 ) {
#else
if( mged_variables.faceplate > 0 )  {
#endif
			if( eye_pos_scr[Z] == 1.0 )  {
				/* This way works, with reasonable Z-clipping */
				persp_mat( pmat, mged_variables.perspective,
					1.0, 0.01, 1.0e10, 1.0 );
			} else {
				/* This way does not have reasonable Z-clipping,
				 * but includes shear, for GDurf's testing.
				 */
				deering_persp_mat( pmat, l, h, eye_pos_scr );
			}
} else {
			/* New way, should handle all cases */
			mike_persp_mat( pmat, eye_pos_scr );
}
#if HACK
mat_print("pmat", pmat);
#endif
			break;
		case 1:
			/* R */
			mat = model2view;
			eye[X] = eye_delta_scr;
			deering_persp_mat( pmat, l, h, eye );
			break;
		case 2:
			/* L */
			mat = model2view;
			eye[X] = -eye_delta_scr;
			deering_persp_mat( pmat, l, h, eye );
			break;
		}
		mat_mul( new, pmat, mat );
		mat = new;
	}

	dmp->dmr_newrot( mat, which_eye );

	FOR_ALL_SOLIDS( sp )  {
		sp->s_flag = DOWN;		/* Not drawn yet */
		/* If part of object rotation, will be drawn below */
		if( sp->s_iflag == UP )
			continue;

		ratio = sp->s_size * inv_viewsize;

		/*
		 * Check for this object being bigger than 
		 * dmp->dmr_bound * the window size, or smaller than a speck.
		 */
		 if( ratio >= dmp->dmr_bound || ratio < 0.001 )
		 	continue;

		if( dmp->dmr_object( sp, mat, ratio, sp==illump ) )  {
			sp->s_flag = UP;
			ndrawn++;
		}
	}

  if(mged_variables.w_axes)
    draw_axes(W_AXES);  /* draw world view axis */

  if(mged_variables.v_axes)
    draw_axes(V_AXES);  /* draw view axis */

	/*
	 *  Draw all solids involved in editing.
	 *  They may be getting transformed away from the other solids.
	 */
	if( state == ST_VIEW )
		return;

	if( mged_variables.perspective <= 0 )  {
		mat = model2objview;
	} else {
		mat_mul( new, pmat, model2objview );
		mat = new;
	}
	dmp->dmr_newrot( mat, which_eye );
	inv_viewsize /= modelchanges[15];

	FOR_ALL_SOLIDS( sp )  {
		/* Ignore all objects not being rotated */
		if( sp->s_iflag != UP )
			continue;

		ratio = sp->s_size * inv_viewsize;

		/*
		 * Check for this object being bigger than 
		 * dmp->dmr_bound * the window size, or smaller than a speck.
		 */
		 if( ratio >= dmp->dmr_bound || ratio < 0.001 )
		 	continue;

		if( dmp->dmr_object( sp, mat, ratio, 1 ) )  {
			sp->s_flag = UP;
			ndrawn++;
		}
	}

  if(mged_variables.e_axes)
    draw_axes(E_AXES); /* draw edit axis */
}

/*
 * Draw view, edit or world axes.
 */
static void
draw_axes(axes)
int axes;
{
  struct solid sp;
  struct rt_vlist vlist;
  int i, j;
  double ox, oy;
  point_t a1, a2;
  point_t m1, m2;
  point_t   v1, v2;
  mat_t mr_mat;   /* model rotations */
  static char *labels[] = {"X", "Y", "Z"};

  mat_idn(mr_mat);
  dmp->dmr_newrot(mr_mat, 0);

  sp.s_vlist.forw = &sp.s_vlist;
  sp.s_vlist.back = &sp.s_vlist;
  BU_LIST_APPEND(&sp.s_vlist, (struct bu_list *)&vlist);
  sp.s_soldash = 0;

  if(axes_color_hook)
    (*axes_color_hook)(axes, sp.s_color);
  else{/* use default color */
    switch(axes){
    case E_AXES:
      sp.s_color[0] = 230;
      sp.s_color[1] = 50;
      sp.s_color[2] = 50;
      break;
    case W_AXES:
      sp.s_color[0] = 150;
      sp.s_color[1] = 230;
      sp.s_color[2] = 150;
      break;
    case V_AXES:
    default:
      sp.s_color[0] = 150;
      sp.s_color[1] = 150;
      sp.s_color[2] = 230;
      break;
    }
  }

  vlist.nused = 6;

  /* load vlist with axes */
  for(i = 0; i < 3; ++i){

    for(j = 0; j < 3; ++j){
      if(i == j){
	if(axes == V_AXES && mged_variables.v_axes > 1){
	  a1[j] = -0.125;
	  a2[j] = 0.125;
	}else{
	  a1[j] = -0.25;
	  a2[j] = 0.25;
	}
      }else{
	a1[j] = 0.0;
	a2[j] = 0.0;
      }
    }

    if(axes == W_AXES){ /* world axes */
      m1[X] = Viewscale*a1[X];
      m1[Y] = Viewscale*a1[Y];
      m1[Z] = Viewscale*a1[Z];
      m2[X] = Viewscale*a2[X];
      m2[Y] = Viewscale*a2[Y];
      m2[Z] = Viewscale*a2[Z];
    }else if(axes == V_AXES){  /* create view axes */
      /* build axes in view coodinates */

      /* apply rotations */
      MAT4X3PNT(v1, Viewrot, a1);
      MAT4X3PNT(v2, Viewrot, a2);

      /* possibly translate */
      if(mged_variables.v_axes > 1){
	switch(mged_variables.v_axes){
	case 2:     /* lower left */
	  ox = -0.8;
	  oy = -0.7;
	  break;
	case 3:     /* upper left */
	  ox = -0.425;
	  oy = 0.8;
	  break;
	case 4:     /* upper right */
	  ox = 0.8;
	  oy = 0.8;
	  break;
	case 5:     /* lower right */
	  ox = 0.8;
	  oy = -0.7;
	  break;
	default:    /* center */
	  ox = 0;
	  oy = 0;
	  break;
	}

	v1[X] += ox;
	v1[Y] += oy;
	v2[X] += ox;
	v2[Y] += oy;
      }

      /* convert view to model coordinates */
      MAT4X3PNT(m1, view2model, v1);
      MAT4X3PNT(m2, view2model, v2);
    }else{  /* create edit axes */
      if(state == ST_S_EDIT || state == ST_O_EDIT){
	/* build edit axes in model coordinates */

	  /* apply rotations */
	  MAT4X3PNT(m1, acc_rot_sol, a1);
	  MAT4X3PNT(m2, acc_rot_sol, a2);
#if 1
	  /* apply scale and translations */
	  m1[X] = Viewscale*m1[X] + e_axes_pos[X];
	  m1[Y] = Viewscale*m1[Y] + e_axes_pos[Y];
	  m1[Z] = Viewscale*m1[Z] + e_axes_pos[Z];
	  m2[X] = Viewscale*m2[X] + e_axes_pos[X];
	  m2[Y] = Viewscale*m2[Y] + e_axes_pos[Y];
	  m2[Z] = Viewscale*m2[Z] + e_axes_pos[Z];
#else
	  m1[X] = Viewscale*m1[X] + es_keypoint[X];
	  m1[Y] = Viewscale*m1[Y] + es_keypoint[Y];
	  m1[Z] = Viewscale*m1[Z] + es_keypoint[Z];
	  m2[X] = Viewscale*m2[X] + es_keypoint[X];
	  m2[Y] = Viewscale*m2[Y] + es_keypoint[Y];
	  m2[Z] = Viewscale*m2[Z] + es_keypoint[Z];
#endif
      }else
	return;
    }

    /* load axes */
    VMOVE(vlist.pt[i*2], m1);
    vlist.cmd[i*2] = RT_VLIST_LINE_MOVE;
    VMOVE(vlist.pt[i*2 + 1], m2);
    vlist.cmd[i*2 + 1] = RT_VLIST_LINE_DRAW;

    /* convert point m2 from model to view space */
    MAT4X3PNT(v2, model2view, m2);

    /* label axes */
    dmp->dmr_puts(labels[i], ((int)(2048.0 * v2[X])) + 15,
		  ((int)(2048.0 * v2[Y])) + 15, 1, DM_YELLOW);
  }

  dmp->dmr_newrot(model2view, 0);

  /* draw axes */
  dmp->dmr_object( &sp, model2view, (double)1.0, 0 );
}
