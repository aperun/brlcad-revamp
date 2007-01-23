/*                          D M - T K . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2007 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file dm-tk.c
 *
 *  Routines specific to MGED's use of LIBDM's Tk display manager.
 *
 *  Author -
 *	Timothy Myers
 *
 *  Source -
 *	SLAD CAD Team
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *
 */

#ifndef lint
static const char RCSid[] = "@(#)$Header$ (BRL)";
#endif
/*#ifdef DM_TK */
#include "common.h"

#include <stdlib.h>
#include <math.h>
#include "tk.h"
#include <X11/Xutil.h>

#ifdef HAVE_STRING_H
#  include <string.h>
#else
#  include <strings.h>
#endif

#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "mater.h"
#include "raytrace.h"
#include "dm_xvars.h"
#include "dm-tk.h"

#include "./ged.h"
#include "./sedit.h"
#include "./mged_dm.h"

extern int _Tk_open_existing();	/* defined in libfb/if_ogl.c */
extern int common_dm();			/* defined in dm-generic.c */
extern void dm_var_init(struct dm_list *initial_dm_list);		/* defined in attach.c */

static int Tk_dm(int argc, char **argv);
static void dirty_hook(void);
static void zclip_hook(void);

#ifdef USE_PROTOTYPES
static Tk_GenericProc Tk_doevent;
#else
static int Tk_doevent();
#endif

struct bu_structparse Tk_vparse[] = {
  {"%f",  1, "bound",		 DM_O(dm_bound),	dirty_hook},
  {"%d",  1, "useBound",	 DM_O(dm_boundFlag),	dirty_hook},
  {"%d",  1, "zclip",		 DM_O(dm_zclip),	zclip_hook},
  {"%d",  1, "debug",		 DM_O(dm_debugLevel),	BU_STRUCTPARSE_FUNC_NULL},
  {"",	  0, (char *)0,		 0,			BU_STRUCTPARSE_FUNC_NULL}
};

int
Tk_dm_init(struct dm_list	*o_dm_list,
	  int			argc,
	  char			*argv[])
{
	struct bu_vls vls;

	dm_var_init(o_dm_list);

	/* register application provided routines */
	cmd_hook = Tk_dm;

	Tk_DeleteGenericHandler(doEvent, (ClientData)NULL);
	if ((dmp = dm_open(interp, DM_TYPE_TK, argc-1, argv)) == DM_NULL)
		return TCL_ERROR;

	/* keep display manager in sync */
	dmp->dm_perspective = mged_variables->mv_perspective_mode;

	eventHandler = Tk_doevent;
	Tk_CreateGenericHandler(doEvent, (ClientData)NULL);
	(void)DM_CONFIGURE_WIN(dmp);

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "mged_bind_dm %s", bu_vls_addr(&pathName));
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);

	return TCL_OK;
}

void
debug_fb_open(void)
{
	char *Tk_name = "/dev/TK";

	if ((fbp = (FBIO *)calloc(sizeof(FBIO), 1)) == FBIO_NULL) {
		Tcl_AppendResult(interp, "Tk_dm_init: failed to allocate framebuffer memory\n",
				 (char *)NULL);
		return;
	}

	*fbp = tk_interface; /* struct copy */
	fbp->if_name = malloc((unsigned)strlen(Tk_name) + 1);
	(void)strcpy(fbp->if_name, Tk_name);

	/* Mark OK by filling in magic number */
	fbp->if_magic = FB_MAGIC;

/*	_Tk_open_existing(fbp,
			   ((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
			   ((struct x_vars *)dmp->dm_vars.priv_vars)->pix,
			   ((struct dm_xvars *)dmp->dm_vars.pub_vars)->win,
			   ((struct dm_xvars *)dmp->dm_vars.pub_vars)->cmap,
			   ((struct dm_xvars *)dmp->dm_vars.pub_vars)->vip,
			   dmp->dm_width, dmp->dm_height,
			   ((struct x_vars *)dmp->dm_vars.priv_vars)->gc);
*/
}

/*
   This routine is being called from doEvent() to handle Expose events.
*/
static int
Tk_doevent(ClientData	clientData,
	  XEvent	*eventPtr)
{
	if (eventPtr->type == Expose && eventPtr->xexpose.count == 0) {
		dirty = 1;

		/* no further processing of this event */
		return TCL_RETURN;
	}

	/* allow further processing of this event */
	return TCL_OK;
}
	    
static int
Tk_dm(int	argc,
     char	*argv[])
{
	  if (!strcmp(argv[0], "set")) {
		  struct bu_vls	vls;

		  bu_vls_init(&vls);

		  if (argc < 2) {
			  /* Bare set command, print out current settings */
			  bu_vls_struct_print2(&vls, "dm_Tk internal variables", Tk_vparse, (const char *)dmp );
		  } else if (argc == 2) {
			  bu_vls_struct_item_named(&vls, Tk_vparse, argv[1], (const char *)dmp, ',');
		  } else {
			  struct bu_vls tmp_vls;

			  bu_vls_init(&tmp_vls);
			  bu_vls_printf(&tmp_vls, "%s=\"", argv[1]);
			  bu_vls_from_argv(&tmp_vls, argc-2, (const char **)argv+2);
			  bu_vls_putc(&tmp_vls, '\"');
			  bu_struct_parse(&tmp_vls, Tk_vparse, (char *)dmp);
			  bu_vls_free(&tmp_vls);
		  }

		  Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
		  bu_vls_free(&vls);

		  return TCL_OK;
	  }

	  return common_dm(argc, argv);
}

static void
dirty_hook(void)
{
	dirty = 1;
}

static void
zclip_hook(void)
{
	view_state->vs_vop->vo_zclip = dmp->dm_zclip;
	dirty_hook();
}
/*#endif*/
/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
