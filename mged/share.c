/*
 *				S H A R E . C
 *
 * Description -
 *	Routines for sharing resources among display managers.
 *
 * Functions -
 *	f_share				command to share/unshare resources
 *	f_rset				command to set resources
 *
 * Source -
 *      SLAD CAD Team
 *      The U. S. Army Research Laboratory
 *      Aberdeen Proving Ground, Maryland  21005
 *
 * Author -
 *      Robert G. Parker
 *
 * Copyright Notice -
 *      This software is Copyright (C) 1998 by the United States Army.
 *      All rights reserved.
 */

#include "conf.h"
#include <math.h>
#include <stdio.h>
#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "bn.h"
#include "raytrace.h"
#include "externs.h"
#include "./ged.h"
#include "./mged_dm.h"

extern void mged_vls_struct_parse();

extern struct bu_structparse axes_vparse[];
extern struct bu_structparse color_scheme_vparse[];
extern struct bu_structparse grid_vparse[];
extern struct bu_structparse rubber_band_vparse[];
extern struct bu_structparse mged_vparse[];

#define RESOURCE_TYPE_ADC		0
#define RESOURCE_TYPE_AXES		1
#define RESOURCE_TYPE_COLOR_SCHEMES	2
#define RESOURCE_TYPE_GRID		3
#define RESOURCE_TYPE_MENU		4
#define RESOURCE_TYPE_MGED_VARIABLES	5
#define RESOURCE_TYPE_RUBBER_BAND	6
#define RESOURCE_TYPE_VIEW		7

#define SHARE_RESOURCE(uflag,str,resource,rc,dlp1,dlp2,vls,error_msg) { \
    if (uflag) { \
      struct str *strp; \
\
      if (dlp1->resource->rc == 1) {   /* already not sharing */ \
	bu_vls_free(&vls); \
	return TCL_OK; \
      } \
\
      --dlp1->resource->rc; \
      strp = dlp1->resource; \
      BU_GETSTRUCT(dlp1->resource, str); \
      *dlp1->resource = *strp;        /* struct copy */ \
    } else { \
      /* already sharing a menu */ \
      if (dlp1->resource == dlp2->resource) { \
	bu_vls_free(&vls); \
	return TCL_OK; \
      } \
\
      if (!--dlp2->resource->rc) \
	bu_free((genptr_t)dlp2->resource, error_msg); \
\
      dlp2->resource = dlp1->resource; \
      ++dlp1->resource->rc; \
    } \
}

/*
 * SYNOPSIS
 *	share [-u] res p1 [p2]
 *
 * DESCRIPTION
 *	Provides a mechanism to (un)share resources among display managers.
 *	Currently, there are eight different resources that can be shared.
 *	They are as follows:
 *		ADC AXES COLOR_SCHEMES GRID MENU MGED_VARIABLES RUBBER_BAND VIEW
 *
 * EXAMPLES
 *	share res_type p1 p2	--->	causes 'p1' to share its' resource of type 'res_type' with 'p2'
 *	share -u res_type p	--->	causes 'p' to no longer share resource of type 'res_type'
 */
int
f_share(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
  register int uflag = 0;		/* unshare flag */
  struct dm_list *dlp1, *dlp2;
  struct bu_vls vls;

  bu_vls_init(&vls);

  if (argc != 4) {
    bu_vls_printf(&vls, "help share");
    Tcl_Eval(interp, bu_vls_addr(&vls));

    bu_vls_free(&vls);
    return TCL_ERROR;
  }

  if (argv[1][0] == '-' && argv[1][1] == 'u') {
    uflag = 1;
    --argc;
    ++argv;
  }

  FOR_ALL_DISPLAYS(dlp1, &head_dm_list.l)
    if (!strcmp(argv[2], bu_vls_addr(&dlp1->dml_dmp->dm_pathName)))
      break;

  if (dlp1 == &head_dm_list) {
    Tcl_AppendResult(interp, "share: unrecognized pathName - ",
		     argv[2], "\n", (char *)NULL);

    bu_vls_free(&vls);
    return TCL_ERROR;
  }

  if (!uflag) {
    FOR_ALL_DISPLAYS(dlp2, &head_dm_list.l)
      if (!strcmp(argv[3], bu_vls_addr(&dlp2->dml_dmp->dm_pathName)))
	break;

    if (dlp2 == &head_dm_list) {
      Tcl_AppendResult(interp, "share: unrecognized pathName - ",
		       argv[3], "\n", (char *)NULL);

      bu_vls_free(&vls);
      return TCL_ERROR;
    }

    /* same display manager */
    if (dlp1 == dlp2) {
      bu_vls_free(&vls);
      return TCL_OK;
    }
  }

  switch (argv[1][0]) {
  case 'a':
  case 'A':
    if (argv[1][1] == 'd' || argv[1][1] == 'D')
      SHARE_RESOURCE(uflag,_adc_state,dml_adc_state,adc_rc,dlp1,dlp2,vls,"share: adc_state")
    else if (argv[1][1] == 'x' || argv[1][1] == 'X')
      SHARE_RESOURCE(uflag,_axes_state,dml_axes_state,ax_rc,dlp1,dlp2,vls,"share: axes_state")
    else {
      bu_vls_printf(&vls, "share: resource type '%s' unknown\n", argv[1]);
      Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);

      bu_vls_free(&vls);
      return TCL_ERROR;
    }
    break;
  case 'c':
  case 'C':
    SHARE_RESOURCE(uflag,_color_scheme,dml_color_scheme,cs_rc,dlp1,dlp2,vls,"share: color_scheme")
    break;
  case 'g':
  case 'G':
    SHARE_RESOURCE(uflag,_grid_state,dml_grid_state,gr_rc,dlp1,dlp2,vls,"share: grid_state")
    break;
  case 'm':
  case 'M':
    SHARE_RESOURCE(uflag,_menu_state,dml_menu_state,ms_rc,dlp1,dlp2,vls,"share: menu_state")
    break;
  case 'r':
  case 'R':
    SHARE_RESOURCE(uflag,_rubber_band,dml_rubber_band,rb_rc,dlp1,dlp2,vls,"share: rubber_band")
    break;
  case 'v':
  case 'V':
    if (argv[1][1] == 'a' || argv[1][1] == 'A')
      SHARE_RESOURCE(uflag,_mged_variables,dml_mged_variables,mv_rc,dlp1,dlp2,vls,"share: mged_variables")
    else if (argv[1][1] == 'i' || argv[1][1] == 'I')
      SHARE_RESOURCE(uflag,_view_state,dml_view_state,vs_rc,dlp1,dlp2,vls,"share: view_state")
    else {
      bu_vls_printf(&vls, "share: resource type '%s' unknown\n", argv[1]);
      Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);

      bu_vls_free(&vls);
      return TCL_ERROR;
    }

    break;
  default:
    bu_vls_printf(&vls, "share: resource type '%s' unknown\n", argv[1]);
    Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);

    bu_vls_free(&vls);
    return TCL_ERROR;
  }

  if (!uflag) 
    dlp2->dml_dirty = 1;	/* need to redraw this guy */

  bu_vls_free(&vls);
  return TCL_OK;
}

/*
 * SYNOPSIS
 *	rset res_type res vals
 *
 * DESCRIPTION
 *	Provides a mechanism to set resource values for some resource.
 *
 * EXAMPLES
 *	rset c bg 0 0 50	--->	sets the background color to dark blue
 */
int
f_rset (clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  int rflag;
  struct bu_vls vls;

  bu_vls_init(&vls);

  if (argc < 2) {
    bu_vls_printf(&vls, "help rset");
    Tcl_Eval(interp, bu_vls_addr(&vls));

    bu_vls_free(&vls);
    return TCL_ERROR;
  }

  switch (argv[1][0]) {
  case 'a':
  case 'A':
    if (argv[1][1] == 'd' || argv[1][1] == 'D')
      bu_vls_printf(&vls, "rset: no support available for the 'adc' resource");
    else if (argv[1][1] == 'x' || argv[1][1] == 'X')
      mged_vls_struct_parse(&vls, "Axes", axes_vparse,
			    (CONST char *)axes_state, argc-1, argv+1);
    else {
      bu_vls_printf(&vls, "rset: resource type '%s' unknown\n", argv[1]);
      Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);

      bu_vls_free(&vls);
      return TCL_ERROR;
    }
    break;
  case 'c':
  case 'C':
    mged_vls_struct_parse(&vls, "Color Schemes", color_scheme_vparse,
			  (CONST char *)color_scheme, argc-1, argv+1);
    break;
  case 'g':
  case 'G':
    mged_vls_struct_parse(&vls, "Grid", grid_vparse,
			  (CONST char *)grid_state, argc-1, argv+1);
    break;
  case 'm':
  case 'M':
    bu_vls_printf(&vls, "rset: no support available for the 'menu' resource");
    break;
  case 'r':
  case 'R':
    mged_vls_struct_parse(&vls, "Rubber Band", rubber_band_vparse,
			  (CONST char *)rubber_band, argc-1, argv+1);
    break;
  case 'v':
  case 'V':
    if (argv[1][1] == 'a' || argv[1][1] == 'A')
      mged_vls_struct_parse(&vls, "mged variables", mged_vparse,
				(CONST char *)mged_variables, argc-1, argv+1);
    else if (argv[1][1] == 'i' || argv[1][1] == 'I')
      bu_vls_printf(&vls, "rset: no support available for the 'view' resource");
    else {
      bu_vls_printf(&vls, "rset: resource type '%s' unknown\n", argv[1]);
      Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);

      bu_vls_free(&vls);
      return TCL_ERROR;
    }

    break;
  default:
    bu_vls_printf(&vls, "rset: resource type '%s' unknown\n", argv[1]);
    Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);

    bu_vls_free(&vls);
    return TCL_ERROR;
  }

  Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
  bu_vls_free(&vls);

  return TCL_OK;
}
