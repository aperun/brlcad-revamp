/*
 *				V I E W _ O B J . C
 *
 * A view object contains the attributes and methods for
 * controlling the view.
 * 
 * Source -
 *	SLAD CAD Team
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *
 * Author -
 *	Robert G. Parker
 */
#include "conf.h"
#include <math.h>
#include "tcl.h"
#include "machine.h"
#include "externs.h"
#include "bu.h"
#include "vmath.h"
#include "bn.h"
#include "cmd.h"

struct view_obj {
  struct bu_list	l;
  struct bu_vls		vo_name;		/* view object name/cmd */
  fastf_t		vo_scale;
  fastf_t		vo_size;		/* 2.0 * scale */
  fastf_t		vo_invSize;		/* 1.0 / size */
  fastf_t 		vo_perspective;		/* perspective angle */
  vect_t		vo_aet;
  mat_t			vo_rotation;
  mat_t			vo_center;
  mat_t			vo_model2view;
  mat_t			vo_pmodel2view;
  mat_t			vo_view2model;
  mat_t			vo_pmat;		/* perspective matrix */
};

int Vo_Init();
int vo_open_tcl();

HIDDEN int vo_scale_tcl();
HIDDEN int vo_size_tcl();
HIDDEN int vo_invSize_tcl();
HIDDEN int vo_aet_tcl();
HIDDEN int vo_rot_tcl();
HIDDEN int vo_center_tcl();
HIDDEN int vo_model2view_tcl();
HIDDEN int vo_pmodel2view_tcl();
HIDDEN int vo_view2model_tcl();
HIDDEN int vo_perspective_tcl();
HIDDEN int vo_pmat_tcl();
HIDDEN int vo_update_tcl();
HIDDEN int vo_cmd();
HIDDEN void vo_update();
HIDDEN void vo_mat_aet();
HIDDEN void vo_mike_persp_mat();

HIDDEN struct view_obj HeadViewObj;		/* head of view object list */
HIDDEN point_t vo_eye_pos_scr = {0, 0, 1};

HIDDEN struct bu_cmdtab vo_cmds[] = 
{
	"scale",		vo_scale_tcl,
	"size",			vo_size_tcl,
	"invSize",		vo_invSize_tcl,
	"aet",			vo_aet_tcl,
	"rot",			vo_rot_tcl,
	"center",		vo_center_tcl,
	"model2view",		vo_model2view_tcl,
	"pmodel2view",		vo_pmodel2view_tcl,
	"view2model",		vo_view2model_tcl,
	"perspective",		vo_perspective_tcl,
	"pmat",			vo_pmat_tcl,
	"update",		vo_update_tcl,
	(char *)0,		(int (*)())0
};

HIDDEN int
vo_cmd(clientData, interp, argc, argv)
ClientData	clientData;
Tcl_Interp	*interp;
int		argc;
char		**argv;
{
  return bu_cmd(clientData, interp, argc, argv, vo_cmds, 1);
}

int
Vo_Init(interp)
Tcl_Interp *interp;
{
  BU_LIST_INIT(&HeadViewObj.l);

  return TCL_OK;
}

HIDDEN void
vo_deleteProc(clientData)
ClientData clientData;
{
  struct view_obj *vop = (struct view_obj *)clientData;

  bu_vls_free(&vop->vo_name);
  BU_LIST_DEQUEUE(&vop->l);
  bu_free((genptr_t)vop, "vo_deleteProc: vop");
}

/*
 * Open/create a view object.
 *
 * USAGE: vo_open [name [args]]
 */
vo_open_tcl(clientData, interp, argc, argv)
ClientData      clientData;
Tcl_Interp      *interp;
int             argc;
char            **argv;
{
  struct view_obj *vop;

  if (argc == 1) {
    /* get list of view objects */
    for (BU_LIST_FOR(vop, view_obj, &HeadViewObj.l))
      Tcl_AppendResult(interp, bu_vls_addr(&vop->vo_name), " ", (char *)NULL);

    return TCL_OK;
  }

  /* check to see if view object exists */
  for (BU_LIST_FOR(vop, view_obj, &HeadViewObj.l)) {
    if (strcmp(argv[1],bu_vls_addr(&vop->vo_name)) == 0) {
      Tcl_AppendResult(interp, "vo_open: ", argv[1],
		       " exists.\n", (char *)NULL);
      return TCL_ERROR;
    }
  }

  BU_GETSTRUCT(vop,view_obj);
  bu_vls_init(&vop->vo_name);
  bu_vls_strcpy(&vop->vo_name,argv[1]);
  vop->vo_scale = 1.0;
  bn_mat_idn(vop->vo_rotation);
  bn_mat_idn(vop->vo_center);
  vo_update(vop);

  (void)Tcl_CreateCommand(interp,
			  bu_vls_addr(&vop->vo_name),
			  vo_cmd,
			  (ClientData)vop,
			  vo_deleteProc);
  BU_LIST_APPEND(&HeadViewObj.l,&vop->l);

  /* Return new function name as result */
  Tcl_ResetResult(interp);
  Tcl_AppendResult(interp, bu_vls_addr(&vop->vo_name), (char *)NULL);
  return TCL_OK;
}

/*
 * Get or set the view scale.
 */
HIDDEN int
vo_scale_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  struct view_obj *vop = (struct view_obj *)clientData;
  struct bu_vls vls;
  fastf_t scale;

  if (argc == 2) { /* get view scale */
    bu_vls_init(&vls);
    bu_vls_printf(&vls, "%g", vop->vo_scale);
    Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
    bu_vls_free(&vls);

    return TCL_OK;
  }

  if (argc == 3) {  /* set view scale */
    if (sscanf(argv[2], "%lf", &scale) != 1) {
      Tcl_AppendResult(interp, "bad scale value - ",
		       argv[2], (char *)NULL);
      return TCL_ERROR;
    }

    vop->vo_scale = scale;
    vop->vo_size = 2.0 * scale;
    vop->vo_invSize = 1.0 / vop->vo_size;

    return TCL_OK;
  }

  /* compose error message */
  bu_vls_init(&vls);
  bu_vls_printf(&vls, "helplib vo_scale");
  Tcl_Eval(interp, bu_vls_addr(&vls));
  bu_vls_free(&vls);

  return TCL_ERROR;
}

/*
 * Get or set the view size.
 */
HIDDEN int
vo_size_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  struct view_obj *vop = (struct view_obj *)clientData;
  struct bu_vls vls;
  fastf_t size;

  /* get view size */
  if (argc == 2) {
    bu_vls_init(&vls);
    bu_vls_printf(&vls, "%g", vop->vo_size);
    Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
    bu_vls_free(&vls);

    return TCL_OK;
  }

  /* set view size */
  if (argc == 3) {
    if (sscanf(argv[2], "%lf", &size) != 1) {
      Tcl_AppendResult(interp, "bad size value - ",
		       argv[2], (char *)NULL);
      return TCL_ERROR;
    }

    vop->vo_size = size;
    vop->vo_invSize = 1.0 / size;
    vop->vo_scale = 0.5 * size;

    return TCL_OK;
  }

  /* compose error message */
  bu_vls_init(&vls);
  bu_vls_printf(&vls, "helplib vo_size");
  Tcl_Eval(interp, bu_vls_addr(&vls));
  bu_vls_free(&vls);

  return TCL_ERROR;
}

/*
 * Get the inverse view size.
 */
HIDDEN int
vo_invSize_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  struct view_obj *vop = (struct view_obj *)clientData;
  struct bu_vls vls;
  fastf_t size;

  if (argc == 2) {
    bu_vls_init(&vls);
    bu_vls_printf(&vls, "%g", vop->vo_invSize);
    Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
    bu_vls_free(&vls);

    return TCL_OK;
  }

  /* compose error message */
  bu_vls_init(&vls);
  bu_vls_printf(&vls, "helplib vo_invSize");
  Tcl_Eval(interp, bu_vls_addr(&vls));
  bu_vls_free(&vls);

  return TCL_ERROR;
}

/*
 * Get or set the azimuth, elevation and twist.
 */
HIDDEN int
vo_aet_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  struct view_obj *vop = (struct view_obj *)clientData;
  struct bu_vls vls;

  if (argc == 2) { /* get aet */
    bu_vls_init(&vls);
    bn_encode_vect(&vls, vop->vo_aet);
    Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
    bu_vls_free(&vls);

    return TCL_OK;
  } else if (argc == 3) {  /* set aet */
    mat_t m;

    if (bn_decode_vect(vop->vo_aet, argv[2]) != 3)
      return TCL_ERROR;

    vo_mat_aet(vop);

    return TCL_OK;
  }

  /* compose error message */
  bu_vls_init(&vls);
  bu_vls_printf(&vls, "helplib vo_aet");
  Tcl_Eval(interp, bu_vls_addr(&vls));
  bu_vls_free(&vls);

  return TCL_ERROR;
}

/*
 * Get or set the rotation matrix.
 */
HIDDEN int
vo_rot_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  struct view_obj *vop = (struct view_obj *)clientData;
  struct bu_vls vls;

  if (argc == 2) { /* get rotation matrix */
    bu_vls_init(&vls);
    bn_encode_mat(&vls, vop->vo_rotation);
    Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
    bu_vls_free(&vls);

    return TCL_OK;
  } else if (argc == 3) {  /* set rotation matrix */
    if (bn_decode_mat(vop->vo_rotation, argv[2]) != 16)
      return TCL_ERROR;

    return TCL_OK;
  }

  /* compose error message */
  bu_vls_init(&vls);
  bu_vls_printf(&vls, "helplib vo_rot");
  Tcl_Eval(interp, bu_vls_addr(&vls));
  bu_vls_free(&vls);

  return TCL_ERROR;
}

/*
 * Get or set the view center.
 */
HIDDEN int
vo_center_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  struct view_obj *vop = (struct view_obj *)clientData;
  vect_t center;
  struct bu_vls vls;

  if (argc == 2) { /* get view center */
    MAT_DELTAS_GET_NEG(center, vop->vo_center);
    bu_vls_init(&vls);
    bn_encode_vect(&vls, center);
    Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
    bu_vls_free(&vls);

    return TCL_OK;
  } else if (argc == 3) {  /* set view center */
    if (bn_decode_vect(center, argv[2]) != 3)
      return TCL_ERROR;

    MAT_DELTAS_VEC_NEG(vop->vo_center, center);

    return TCL_OK;
  }

  /* compose error message */
  bu_vls_init(&vls);
  bu_vls_printf(&vls, "helplib vo_center");
  Tcl_Eval(interp, bu_vls_addr(&vls));
  bu_vls_free(&vls);

  return TCL_ERROR;
}

/*
 * Get the model2view matrix.
 */
HIDDEN int
vo_model2view_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  struct view_obj *vop = (struct view_obj *)clientData;
  struct bu_vls vls;

  if (argc == 2) {
    bu_vls_init(&vls);
    bn_encode_mat(&vls, vop->vo_model2view);
    Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
    bu_vls_free(&vls);

    return TCL_OK;
  }

  /* compose error message */
  bu_vls_init(&vls);
  bu_vls_printf(&vls, "helplib vo_model2view");
  Tcl_Eval(interp, bu_vls_addr(&vls));
  bu_vls_free(&vls);

  return TCL_ERROR;
}

/*
 * Get the pmodel2view matrix.
 */
HIDDEN int
vo_pmodel2view_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  struct view_obj *vop = (struct view_obj *)clientData;
  struct bu_vls vls;

  if (argc == 2) {
    bu_vls_init(&vls);
    bn_encode_mat(&vls, vop->vo_pmodel2view);
    Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
    bu_vls_free(&vls);

    return TCL_OK;
  }

  /* compose error message */
  bu_vls_init(&vls);
  bu_vls_printf(&vls, "helplib vo_pmodel2view");
  Tcl_Eval(interp, bu_vls_addr(&vls));
  bu_vls_free(&vls);

  return TCL_ERROR;
}

/*
 * Get the view2model matrix.
 */
HIDDEN int
vo_view2model_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  struct view_obj *vop = (struct view_obj *)clientData;
  struct bu_vls vls;

  if (argc == 2) {
    bu_vls_init(&vls);
    bn_encode_mat(&vls, vop->vo_view2model);
    Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
    bu_vls_free(&vls);

    return TCL_OK;
  }

  /* compose error message */
  bu_vls_init(&vls);
  bu_vls_printf(&vls, "helplib vo_view2model");
  Tcl_Eval(interp, bu_vls_addr(&vls));
  bu_vls_free(&vls);

  return TCL_ERROR;
}

/*
 * Get/set the perspective angle.
 *
 * Usage:
 *        procname perspective [angle]
 *
 */
HIDDEN int
vo_perspective_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  struct view_obj *vop = (struct view_obj *)clientData;
  struct bu_vls vls;
  fastf_t perspective;

  /* get the perspective angle */
  if (argc == 2) {
    bu_vls_init(&vls);
    bu_vls_printf(&vls, "%g", vop->vo_perspective);
    Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
    bu_vls_free(&vls);

    return TCL_OK;
  }

  /* set the perspective angle */
  if (argc == 3) {
    if (sscanf(argv[2], "%lf", &perspective) != 1) {
      Tcl_AppendResult(interp, "bad perspective angle - ",
		       argv[2], (char *)NULL);
      return TCL_ERROR;
    }

    vop->vo_perspective = perspective;
    vo_mike_persp_mat(vop->vo_pmat, vo_eye_pos_scr);

    return TCL_OK;
  }

  /* Compose error message */
  bu_vls_init(&vls);
  bu_vls_printf(&vls, "helplib vo_pmat");
  Tcl_Eval(interp, bu_vls_addr(&vls));
  bu_vls_free(&vls);

  return TCL_ERROR;
}

/*
 * Get the perspective matrix.
 */
HIDDEN int
vo_pmat_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  struct view_obj *vop = (struct view_obj *)clientData;
  struct bu_vls vls;

  if (argc == 2) {
    bu_vls_init(&vls);
    bn_encode_mat(&vls, vop->vo_pmat);
    Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
    bu_vls_free(&vls);

    return TCL_OK;
  }

  /* compose error message */
  bu_vls_init(&vls);
  bu_vls_printf(&vls, "helplib vo_pmat");
  Tcl_Eval(interp, bu_vls_addr(&vls));
  bu_vls_free(&vls);

  return TCL_ERROR;
}

HIDDEN void
vo_update(vop)
struct view_obj *vop;
{
  vect_t work, work1;
  vect_t temp, temp1;

  bn_mat_mul(vop->vo_model2view,
	     vop->vo_rotation,
	     vop->vo_center);
  vop->vo_model2view[15] = vop->vo_scale;
  bn_mat_inv(vop->vo_view2model, vop->vo_model2view);

  /* Find current azimuth, elevation, and twist angles */
  VSET( work , 0.0, 0.0, 1.0 );       /* view z-direction */
  MAT4X3VEC( temp , vop->vo_view2model , work );
  VSET( work1 , 1.0, 0.0, 0.0 );      /* view x-direction */
  MAT4X3VEC( temp1 , vop->vo_view2model , work1 );

  /* calculate angles using accuracy of 0.005, since display
   * shows 2 digits right of decimal point */
  bn_aet_vec( &vop->vo_aet[0],
	      &vop->vo_aet[1],
	      &vop->vo_aet[2],
	      temp , temp1 , (fastf_t)0.005 );

  /* Force azimuth range to be [0,360] */
  if((NEAR_ZERO(vop->vo_aet[1] - 90.0,(fastf_t)0.005) ||
      NEAR_ZERO(vop->vo_aet[1] + 90.0,(fastf_t)0.005)) &&
     vop->vo_aet[0] < 0 &&
     !NEAR_ZERO(vop->vo_aet[0],(fastf_t)0.005))
    vop->vo_aet[0] += 360.0;
  else if(NEAR_ZERO(vop->vo_aet[0],(fastf_t)0.005))
    vop->vo_aet[0] = 0.0;

  /* apply the perspective angle to model2view */
  bn_mat_mul(vop->vo_pmodel2view, vop->vo_pmat, vop->vo_model2view);
}

HIDDEN int
vo_update_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  struct view_obj *vop = (struct view_obj *)clientData;

  if (argc != 2) {
    struct bu_vls vls;

    bu_vls_init(&vls);
    bu_vls_printf(&vls, "helplib vo_update");
    Tcl_Eval(interp, bu_vls_addr(&vls));
    bu_vls_free(&vls);

    return TCL_ERROR;
  }

  vo_update(vop);
  return TCL_OK;
}

HIDDEN void
vo_mat_aet(vop)
struct view_obj *vop;
{
  mat_t tmat;
  fastf_t twist;
  fastf_t c_twist;
  fastf_t s_twist;

  bn_mat_angles(vop->vo_rotation,
		270.0 + vop->vo_aet[1],
		0.0,
		270.0 - vop->vo_aet[0]);

  twist = -vop->vo_aet[2] * bn_degtorad;
  c_twist = cos(twist);
  s_twist = sin(twist);
  bn_mat_zrot(tmat, s_twist, c_twist);
  bn_mat_mul2(tmat, vop->vo_rotation);
}

/*
 *  This code came from mged/dozoom.c.
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
HIDDEN void
vo_mike_persp_mat(pmat, eye)
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
  bn_mat_idn(shear);
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
  bn_mat_idn(xlate);
  /* XXX should I use MAT_DELTAS_VEC_NEG()?  X and Y should be 0 now */
  MAT_DELTAS( xlate, 0, 0, 1-sheared_eye[Z] );

  /* Build perspective matrix inline, substituting fov=2*atan(1,Z) */
  bn_mat_idn( persp );
  /* From page 492 of Graphics Gems */
  persp[0] = sheared_eye[Z];	/* scaling: fov aspect term */
  persp[5] = sheared_eye[Z];	/* scaling: determines fov */

  /* From page 158 of Rogers Mathematical Elements */
  /* Z center of projection at Z=+1, r=-1/1 */
  persp[14] = -1;

  bn_mat_mul( t1, xlate, shear );
  bn_mat_mul( t2, persp, t1 );
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
  bn_mat_mul( pmat, xlate, t2 );
#if 0
  bn_mat_print("pmat",pmat);

  /* Some quick checking */
  VSET( a, 0.0, 0.0, -1.0 );
  MAT4X3PNT( b, pmat, a );
  VPRINT("0,0,-1 ->", b);

  VSET( a, 1.0, 1.0, -1.0 );
  MAT4X3PNT( b, pmat, a );
  VPRINT("1,1,-1 ->", b);

  VSET( a, 0.0, 0.0, 0.0 );
  MAT4X3PNT( b, pmat, a );
  VPRINT("0,0,0 ->", b);

  VSET( a, 1.0, 1.0, 0.0 );
  MAT4X3PNT( b, pmat, a );
  VPRINT("1,1,0 ->", b);

  VSET( a, 1.0, 1.0, 1.0 );
  MAT4X3PNT( b, pmat, a );
  VPRINT("1,1,1 ->", b);

  VSET( a, 0.0, 0.0, 1.0 );
  MAT4X3PNT( b, pmat, a );
  VPRINT("0,0,1 ->", b);
#endif
}
