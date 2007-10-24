/* 
 * pkgd.c --
 *
 *	This file contains a simple Tcl package "pkgd" that is intended
 *	for testing the Tcl dynamic loading facilities.	 It can be used
 *	in both safe and unsafe interpreters.
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 */

#include "tcl.h"

/*
 * Prototypes for procedures defined later in this file:
 */

static int    Pkgd_SubObjCmd _ANSI_ARGS_((ClientData clientData,
		Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]));
static int    Pkgd_UnsafeObjCmd _ANSI_ARGS_((ClientData clientData,
		Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]));

/*
 *----------------------------------------------------------------------
 *
 * Pkgd_SubObjCmd --
 *
 *	This procedure is invoked to process the "pkgd_sub" Tcl command.
 *	It expects two arguments and returns their difference.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
Pkgd_SubObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj * CONST objv[];	/* Argument objects. */
{
    int first, second;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "num num");
	return TCL_ERROR;
    }
    if ((Tcl_GetIntFromObj(interp, objv[1], &first) != TCL_OK)
	    || (Tcl_GetIntFromObj(interp, objv[2], &second) != TCL_OK)) {
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(first - second));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Pkgd_UnsafeCmd --
 *
 *	This procedure is invoked to process the "pkgd_unsafe" Tcl command.
 *	It just returns a constant string.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
Pkgd_UnsafeObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj * CONST objv[];	/* Argument objects. */
{
    Tcl_SetObjResult(interp, Tcl_NewStringObj("unsafe command invoked", -1));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Pkgd_Init --
 *
 *	This is a package initialization procedure, which is called
 *	by Tcl when this package is to be added to an interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Pkgd_Init(interp)
    Tcl_Interp *interp;		/* Interpreter in which the package is
				 * to be made available. */
{
    int code;

    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }
    code = Tcl_PkgProvide(interp, "Pkgd", "7.3");
    if (code != TCL_OK) {
	return code;
    }
    Tcl_CreateObjCommand(interp, "pkgd_sub", Pkgd_SubObjCmd,
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "pkgd_unsafe", Pkgd_UnsafeObjCmd,
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Pkgd_SafeInit --
 *
 *	This is a package initialization procedure, which is called
 *	by Tcl when this package is to be added to an unsafe interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Pkgd_SafeInit(interp)
    Tcl_Interp *interp;		/* Interpreter in which the package is
				 * to be made available. */
{
    int code;

    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }
    code = Tcl_PkgProvide(interp, "Pkgd", "7.3");
    if (code != TCL_OK) {
	return code;
    }
    Tcl_CreateObjCommand(interp, "pkgd_sub", Pkgd_SubObjCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    return TCL_OK;
}
