/*
 * tclIOCmd.c --
 *
 *	Contains the definitions of most of the Tcl commands relating to IO.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 */

#include "tclInt.h"

/*
 * Callback structure for accept callback in a TCP server.
 */

typedef struct AcceptCallback {
    char *script;			/* Script to invoke. */
    Tcl_Interp *interp;			/* Interpreter in which to run it. */
} AcceptCallback;

/*
 * Static functions for this file:
 */

static void		AcceptCallbackProc(ClientData callbackData,
			    Tcl_Channel chan, char *address, int port);
static void		RegisterTcpServerInterpCleanup(Tcl_Interp *interp,
			    AcceptCallback *acceptCallbackPtr);
static void		TcpAcceptCallbacksDeleteProc(ClientData clientData,
			    Tcl_Interp *interp);
static void		TcpServerCloseProc(ClientData callbackData);
static void		UnregisterTcpServerInterpCleanupProc(
			    Tcl_Interp *interp,
			    AcceptCallback *acceptCallbackPtr);

/*
 *----------------------------------------------------------------------
 *
 * Tcl_PutsObjCmd --
 *
 *	This function is invoked to process the "puts" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Produces output on a channel.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_PutsObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;		/* The channel to puts on. */
    Tcl_Obj *string;		/* String to write. */
    int newline;		/* Add a newline at end? */
    const char *channelId; /* Name of channel for puts. */
    int result;			/* Result of puts operation. */
    int mode;			/* Mode in which channel is opened. */

    switch (objc) {
    case 2: /* [puts $x] */
	string = objv[1];
	newline = 1;
	channelId = "stdout";
	break;

    case 3: /* [puts -nonewline $x] or [puts $chan $x] */
	if (strcmp(Tcl_GetString(objv[1]), "-nonewline") == 0) {
	    newline = 0;
	    channelId = "stdout";
	} else {
	    newline = 1;
	    channelId = Tcl_GetString(objv[1]);
	}
	string = objv[2];
	break;

    case 4: /* [puts -nonewline $chan $x] or [puts $chan $x nonewline] */
	if (strcmp(Tcl_GetString(objv[1]), "-nonewline") == 0) {
	    channelId = Tcl_GetString(objv[2]);
	    string = objv[3];
	} else {
	    /*
	     * The code below provides backwards compatibility with an old
	     * form of the command that is no longer recommended or
	     * documented.
	     */

	    char *arg;
	    int length;

	    arg = Tcl_GetStringFromObj(objv[3], &length);
	    if ((length != 9)
		    || (strncmp(arg, "nonewline", (size_t) length) != 0)) {
		Tcl_AppendResult(interp, "bad argument \"", arg,
			"\": should be \"nonewline\"", NULL);
		return TCL_ERROR;
	    }
	    channelId = Tcl_GetString(objv[1]);
	    string = objv[2];
	}
	newline = 0;
	break;

    default:
	/* [puts] or [puts some bad number of arguments...] */
	Tcl_WrongNumArgs(interp, 1, objv, "?-nonewline? ?channelId? string");
	return TCL_ERROR;
    }

    chan = Tcl_GetChannel(interp, channelId, &mode);
    if (chan == (Tcl_Channel) NULL) {
	return TCL_ERROR;
    }
    if ((mode & TCL_WRITABLE) == 0) {
	Tcl_AppendResult(interp, "channel \"", channelId,
		"\" wasn't opened for writing", NULL);
	return TCL_ERROR;
    }

    result = Tcl_WriteObj(chan, string);
    if (result < 0) {
	goto error;
    }
    if (newline != 0) {
	result = Tcl_WriteChars(chan, "\n", 1);
	if (result < 0) {
	    goto error;
	}
    }
    return TCL_OK;

    /*
     * TIP #219.
     * Capture error messages put by the driver into the bypass area and put
     * them into the regular interpreter result. Fall back to the regular
     * message if nothing was found in the bypass.
     */

  error:
    if (!TclChanCaughtErrorBypass(interp, chan)) {
	Tcl_AppendResult(interp, "error writing \"", channelId, "\": ",
		Tcl_PosixError(interp), NULL);
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FlushObjCmd --
 *
 *	This function is called to process the Tcl "flush" command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May cause output to appear on the specified channel.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_FlushObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;		/* The channel to flush on. */
    char *channelId;
    int mode;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId");
	return TCL_ERROR;
    }
    channelId = Tcl_GetString(objv[1]);
    chan = Tcl_GetChannel(interp, channelId, &mode);
    if (chan == (Tcl_Channel) NULL) {
	return TCL_ERROR;
    }
    if ((mode & TCL_WRITABLE) == 0) {
	Tcl_AppendResult(interp, "channel \"", channelId,
		"\" wasn't opened for writing", NULL);
	return TCL_ERROR;
    }

    if (Tcl_Flush(chan) != TCL_OK) {
	/*
	 * TIP #219.
	 * Capture error messages put by the driver into the bypass area and
	 * put them into the regular interpreter result. Fall back to the
	 * regular message if nothing was found in the bypass.
	 */

	if (!TclChanCaughtErrorBypass(interp, chan)) {
	    Tcl_AppendResult(interp, "error flushing \"", channelId, "\": ",
		    Tcl_PosixError(interp), NULL);
	}
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetsObjCmd --
 *
 *	This function is called to process the Tcl "gets" command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May consume input from channel.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_GetsObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;		/* The channel to read from. */
    int lineLen;		/* Length of line just read. */
    int mode;			/* Mode in which channel is opened. */
    char *name;
    Tcl_Obj *linePtr;

    if ((objc != 2) && (objc != 3)) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId ?varName?");
	return TCL_ERROR;
    }
    name = Tcl_GetString(objv[1]);
    chan = Tcl_GetChannel(interp, name, &mode);
    if (chan == (Tcl_Channel) NULL) {
	return TCL_ERROR;
    }
    if ((mode & TCL_READABLE) == 0) {
	Tcl_AppendResult(interp, "channel \"", name,
		"\" wasn't opened for reading", NULL);
	return TCL_ERROR;
    }

    linePtr = Tcl_NewObj();

    lineLen = Tcl_GetsObj(chan, linePtr);
    if (lineLen < 0) {
	if (!Tcl_Eof(chan) && !Tcl_InputBlocked(chan)) {
	    Tcl_DecrRefCount(linePtr);

	    /*
	     * TIP #219.
	     * Capture error messages put by the driver into the bypass area
	     * and put them into the regular interpreter result. Fall back to
	     * the regular message if nothing was found in the bypass.
	     */
	    if (!TclChanCaughtErrorBypass(interp, chan)) {
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp, "error reading \"", name, "\": ",
			Tcl_PosixError(interp), NULL);
	    }
	    return TCL_ERROR;
	}
	lineLen = -1;
    }
    if (objc == 3) {
	if (Tcl_ObjSetVar2(interp, objv[2], NULL, linePtr,
		TCL_LEAVE_ERR_MSG) == NULL) {
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(lineLen));
	return TCL_OK;
    } else {
	Tcl_SetObjResult(interp, linePtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ReadObjCmd --
 *
 *	This function is invoked to process the Tcl "read" command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May consume input from channel.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_ReadObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;		/* The channel to read from. */
    int newline, i;		/* Discard newline at end? */
    int toRead;			/* How many bytes to read? */
    int charactersRead;		/* How many characters were read? */
    int mode;			/* Mode in which channel is opened. */
    char *name;
    Tcl_Obj *resultPtr;

    if ((objc != 2) && (objc != 3)) {
	Interp *iPtr;

    argerror:
	iPtr = (Interp *) interp;
	Tcl_WrongNumArgs(interp, 1, objv, "channelId ?numChars?");

	/*
	 * Do not append directly; that makes ensembles using this command as
	 * a subcommand produce the wrong message.
	 */

	iPtr->flags |= INTERP_ALTERNATE_WRONG_ARGS;
	Tcl_WrongNumArgs(interp, 1, objv, "?-nonewline? channelId");
	iPtr->flags &= ~INTERP_ALTERNATE_WRONG_ARGS;
	return TCL_ERROR;
    }

    i = 1;
    newline = 0;
    if (strcmp(Tcl_GetString(objv[1]), "-nonewline") == 0) {
	newline = 1;
	i++;
    }

    if (i == objc) {
	goto argerror;
    }

    name = Tcl_GetString(objv[i]);
    chan = Tcl_GetChannel(interp, name, &mode);
    if (chan == (Tcl_Channel) NULL) {
	return TCL_ERROR;
    }
    if ((mode & TCL_READABLE) == 0) {
	Tcl_AppendResult(interp, "channel \"", name,
		"\" wasn't opened for reading", NULL);
	return TCL_ERROR;
    }
    i++;	/* Consumed channel name. */

    /*
     * Compute how many bytes to read, and see whether the final newline
     * should be dropped.
     */

    toRead = -1;
    if (i < objc) {
	char *arg;

	arg = Tcl_GetString(objv[i]);
	if (isdigit(UCHAR(arg[0]))) { /* INTL: digit */
	    if (Tcl_GetIntFromObj(interp, objv[i], &toRead) != TCL_OK) {
		return TCL_ERROR;
	    }
	} else if (strcmp(arg, "nonewline") == 0) {
	    newline = 1;
	} else {
	    Tcl_AppendResult(interp, "bad argument \"", arg,
		    "\": should be \"nonewline\"", NULL);
	    return TCL_ERROR;
	}
    }

    resultPtr = Tcl_NewObj();
    Tcl_IncrRefCount(resultPtr);
    charactersRead = Tcl_ReadChars(chan, resultPtr, toRead, 0);
    if (charactersRead < 0) {
	/*
	 * TIP #219.
	 * Capture error messages put by the driver into the bypass area and
	 * put them into the regular interpreter result. Fall back to the
	 * regular message if nothing was found in the bypass.
	 */

	if (!TclChanCaughtErrorBypass(interp, chan)) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendResult(interp, "error reading \"", name, "\": ",
		    Tcl_PosixError(interp), NULL);
	}
	Tcl_DecrRefCount(resultPtr);
	return TCL_ERROR;
    }

    /*
     * If requested, remove the last newline in the channel if at EOF.
     */

    if ((charactersRead > 0) && (newline != 0)) {
	char *result;
	int length;

	result = Tcl_GetStringFromObj(resultPtr, &length);
	if (result[length - 1] == '\n') {
	    Tcl_SetObjLength(resultPtr, length - 1);
	}
    }
    Tcl_SetObjResult(interp, resultPtr);
    Tcl_DecrRefCount(resultPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SeekObjCmd --
 *
 *	This function is invoked to process the Tcl "seek" command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Moves the position of the access point on the specified channel.  May
 *	flush queued output.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_SeekObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;		/* The channel to tell on. */
    Tcl_WideInt offset;		/* Where to seek? */
    int mode;			/* How to seek? */
    Tcl_WideInt result;		/* Of calling Tcl_Seek. */
    char *chanName;
    int optionIndex;
    static const char *originOptions[] = {
	"start", "current", "end", NULL
    };
    static int modeArray[] = {SEEK_SET, SEEK_CUR, SEEK_END};

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId offset ?origin?");
	return TCL_ERROR;
    }
    chanName = Tcl_GetString(objv[1]);
    chan = Tcl_GetChannel(interp, chanName, NULL);
    if (chan == (Tcl_Channel) NULL) {
	return TCL_ERROR;
    }
    if (Tcl_GetWideIntFromObj(interp, objv[2], &offset) != TCL_OK) {
	return TCL_ERROR;
    }
    mode = SEEK_SET;
    if (objc == 4) {
	if (Tcl_GetIndexFromObj(interp, objv[3], originOptions, "origin", 0,
		&optionIndex) != TCL_OK) {
	    return TCL_ERROR;
	}
	mode = modeArray[optionIndex];
    }

    result = Tcl_Seek(chan, offset, mode);
    if (result == Tcl_LongAsWide(-1)) {
	/*
	 * TIP #219.
	 * Capture error messages put by the driver into the bypass area and
	 * put them into the regular interpreter result. Fall back to the
	 * regular message if nothing was found in the bypass.
	 */
	if (!TclChanCaughtErrorBypass(interp, chan)) {
	    Tcl_AppendResult(interp, "error during seek on \"", chanName,
		    "\": ", Tcl_PosixError(interp), NULL);
	}
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TellObjCmd --
 *
 *	This function is invoked to process the Tcl "tell" command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_TellObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;		/* The channel to tell on. */
    char *chanName;
    Tcl_WideInt newLoc;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId");
	return TCL_ERROR;
    }

    /*
     * Try to find a channel with the right name and permissions in the IO
     * channel table of this interpreter.
     */

    chanName = Tcl_GetString(objv[1]);
    chan = Tcl_GetChannel(interp, chanName, NULL);
    if (chan == (Tcl_Channel) NULL) {
	return TCL_ERROR;
    }

    newLoc = Tcl_Tell(chan);

    /*
     * TIP #219.
     * Capture error messages put by the driver into the bypass area and put
     * them into the regular interpreter result.
     */

    if (TclChanCaughtErrorBypass(interp, chan)) {
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewWideIntObj(newLoc));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CloseObjCmd --
 *
 *	This function is invoked to process the Tcl "close" command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May discard queued input; may flush queued output.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_CloseObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;		/* The channel to close. */
    char *arg;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId");
	return TCL_ERROR;
    }

    arg = Tcl_GetString(objv[1]);
    chan = Tcl_GetChannel(interp, arg, NULL);
    if (chan == (Tcl_Channel) NULL) {
	return TCL_ERROR;
    }

    if (Tcl_UnregisterChannel(interp, chan) != TCL_OK) {
	/*
	 * If there is an error message and it ends with a newline, remove the
	 * newline. This is done for command pipeline channels where the error
	 * output from the subprocesses is stored in interp's result.
	 *
	 * NOTE: This is likely to not have any effect on regular error
	 * messages produced by drivers during the closing of a channel,
	 * because the Tcl convention is that such error messages do not have
	 * a terminating newline.
	 */

	Tcl_Obj *resultPtr;
	char *string;
	int len;

	resultPtr = Tcl_GetObjResult(interp);
	if (Tcl_IsShared(resultPtr)) {
	    resultPtr = Tcl_DuplicateObj(resultPtr);
	    Tcl_SetObjResult(interp, resultPtr);
	}
	string = Tcl_GetStringFromObj(resultPtr, &len);
	if ((len > 0) && (string[len - 1] == '\n')) {
	    Tcl_SetObjLength(resultPtr, len - 1);
	}
	return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FconfigureObjCmd --
 *
 *	This function is invoked to process the Tcl "fconfigure" command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May modify the behavior of an IO channel.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_FconfigureObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    char *chanName, *optionName, *valueName;
    Tcl_Channel chan;		/* The channel to set a mode on. */
    int i;			/* Iterate over arg-value pairs. */

    if ((objc < 2) || (((objc % 2) == 1) && (objc != 3))) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"channelId ?optionName? ?value? ?optionName value?...");
	return TCL_ERROR;
    }

    chanName = Tcl_GetString(objv[1]);
    chan = Tcl_GetChannel(interp, chanName, NULL);
    if (chan == (Tcl_Channel) NULL) {
	return TCL_ERROR;
    }

    if (objc == 2) {
	Tcl_DString ds;		/* DString to hold result of calling
				 * Tcl_GetChannelOption. */

	Tcl_DStringInit(&ds);
	if (Tcl_GetChannelOption(interp, chan, NULL, &ds) != TCL_OK) {
	    Tcl_DStringFree(&ds);
	    return TCL_ERROR;
	}
	Tcl_DStringResult(interp, &ds);
	return TCL_OK;
    } else if (objc == 3) {
	Tcl_DString ds;		/* DString to hold result of calling
				 * Tcl_GetChannelOption. */

	Tcl_DStringInit(&ds);
	optionName = Tcl_GetString(objv[2]);
	if (Tcl_GetChannelOption(interp, chan, optionName, &ds) != TCL_OK) {
	    Tcl_DStringFree(&ds);
	    return TCL_ERROR;
	}
	Tcl_DStringResult(interp, &ds);
	return TCL_OK;
    }

    for (i = 3; i < objc; i += 2) {
	optionName = Tcl_GetString(objv[i-1]);
	valueName = Tcl_GetString(objv[i]);
	if (Tcl_SetChannelOption(interp, chan, optionName, valueName)
		!= TCL_OK) {
	    return TCL_ERROR;
	}
    }

    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_EofObjCmd --
 *
 *	This function is invoked to process the Tcl "eof" command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Sets interp's result to boolean true or false depending on whether the
 *	specified channel has an EOF condition.
 *
 *---------------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_EofObjCmd(
    ClientData unused,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;
    int dummy;
    char *arg;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId");
	return TCL_ERROR;
    }

    arg = Tcl_GetString(objv[1]);
    chan = Tcl_GetChannel(interp, arg, &dummy);
    if (chan == NULL) {
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(Tcl_Eof(chan)));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ExecObjCmd --
 *
 *	This function is invoked to process the "exec" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_ExecObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    /*
     * This function generates an argv array for the string arguments. It
     * starts out with stack-allocated space but uses dynamically-allocated
     * storage if needed.
     */

    Tcl_Obj *resultPtr;
    const char **argv;
    char *string;
    Tcl_Channel chan;
    int argc, background, i, index, keepNewline, result, skip, length;
    int ignoreStderr;
    static const char *options[] = {
	"-ignorestderr", "-keepnewline", "--", NULL
    };
    enum options {
	EXEC_IGNORESTDERR, EXEC_KEEPNEWLINE, EXEC_LAST
    };

    /*
     * Check for any leading option arguments.
     */

    keepNewline = 0;
    ignoreStderr = 0;
    for (skip = 1; skip < objc; skip++) {
	string = Tcl_GetString(objv[skip]);
	if (string[0] != '-') {
	    break;
	}
	if (Tcl_GetIndexFromObj(interp, objv[skip], options, "switch",
		TCL_EXACT, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (index == EXEC_KEEPNEWLINE) {
	    keepNewline = 1;
	} else if (index == EXEC_IGNORESTDERR) {
	    ignoreStderr = 1;
	} else {
	    skip++;
	    break;
	}
    }
    if (objc <= skip) {
	Tcl_WrongNumArgs(interp, 1, objv, "?switches? arg ?arg ...?");
	return TCL_ERROR;
    }

    /*
     * See if the command is to be run in background.
     */

    background = 0;
    string = Tcl_GetString(objv[objc - 1]);
    if ((string[0] == '&') && (string[1] == '\0')) {
	objc--;
	background = 1;
    }

    /*
     * Create the string argument array "argv". Make sure argv is large enough
     * to hold the argc arguments plus 1 extra for the zero end-of-argv word.
     */

    argc = objc - skip;
    argv = (const char **)
	    TclStackAlloc(interp, (unsigned)(argc + 1) * sizeof(char *));

    /*
     * Copy the string conversions of each (post option) object into the
     * argument vector.
     */

    for (i = 0; i < argc; i++) {
	argv[i] = Tcl_GetString(objv[i + skip]);
    }
    argv[argc] = NULL;
    chan = Tcl_OpenCommandChannel(interp, argc, argv, (background ? 0 :
	    (ignoreStderr ? TCL_STDOUT : TCL_STDOUT|TCL_STDERR)));

    /*
     * Free the argv array.
     */

    TclStackFree(interp);	/* argv */

    if (chan == (Tcl_Channel) NULL) {
	return TCL_ERROR;
    }

    if (background) {
	/*
	 * Store the list of PIDs from the pipeline in interp's result and
	 * detach the PIDs (instead of waiting for them).
	 */

	TclGetAndDetachPids(interp, chan);
	if (Tcl_Close(interp, chan) != TCL_OK) {
	    return TCL_ERROR;
	}
	return TCL_OK;
    }

    resultPtr = Tcl_NewObj();
    if (Tcl_GetChannelHandle(chan, TCL_READABLE, NULL) == TCL_OK) {
	if (Tcl_ReadChars(chan, resultPtr, -1, 0) < 0) {
	    /*
	     * TIP #219.
	     * Capture error messages put by the driver into the bypass area
	     * and put them into the regular interpreter result. Fall back to
	     * the regular message if nothing was found in the bypass.
	     */

	    if (!TclChanCaughtErrorBypass (interp, chan)) {
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp, "error reading output from command: ",
			Tcl_PosixError(interp), NULL);
		Tcl_DecrRefCount(resultPtr);
	    }
	    return TCL_ERROR;
	}
    }

    /*
     * If the process produced anything on stderr, it will have been returned
     * in the interpreter result. It needs to be appended to the result
     * string.
     */

    result = Tcl_Close(interp, chan);
    Tcl_AppendObjToObj(resultPtr, Tcl_GetObjResult(interp));

    /*
     * If the last character of the result is a newline, then remove the
     * newline character.
     */

    if (keepNewline == 0) {
	string = Tcl_GetStringFromObj(resultPtr, &length);
	if ((length > 0) && (string[length - 1] == '\n')) {
	    Tcl_SetObjLength(resultPtr, length - 1);
	}
    }
    Tcl_SetObjResult(interp, resultPtr);

    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_FblockedObjCmd --
 *
 *	This function is invoked to process the Tcl "fblocked" command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Sets interp's result to boolean true or false depending on whether the
 *	preceeding input operation on the channel would have blocked.
 *
 *---------------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_FblockedObjCmd(
    ClientData unused,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;
    int mode;
    char *arg;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId");
	return TCL_ERROR;
    }

    arg = Tcl_GetString(objv[1]);
    chan = Tcl_GetChannel(interp, arg, &mode);
    if (chan == NULL) {
	return TCL_ERROR;
    }
    if ((mode & TCL_READABLE) == 0) {
	Tcl_AppendResult(interp, "channel \"", arg,
		"\" wasn't opened for reading", NULL);
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(Tcl_InputBlocked(chan)));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_OpenObjCmd --
 *
 *	This function is invoked to process the "open" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_OpenObjCmd(
    ClientData notUsed,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int pipeline, prot;
    const char *modeString, *what;
    Tcl_Channel chan;

    if ((objc < 2) || (objc > 4)) {
	Tcl_WrongNumArgs(interp, 1, objv, "fileName ?access? ?permissions?");
	return TCL_ERROR;
    }
    prot = 0666;
    if (objc == 2) {
	modeString = "r";
    } else {
	modeString = Tcl_GetString(objv[2]);
	if (objc == 4) {
	    if (Tcl_GetIntFromObj(interp, objv[3], &prot) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }

    pipeline = 0;
    what = Tcl_GetString(objv[1]);
    if (what[0] == '|') {
	pipeline = 1;
    }

    /*
     * Open the file or create a process pipeline.
     */

    if (!pipeline) {
	chan = Tcl_FSOpenFileChannel(interp, objv[1], modeString, prot);
    } else {
	int mode, seekFlag, cmdObjc, binary;
	const char **cmdArgv;

	if (Tcl_SplitList(interp, what+1, &cmdObjc, &cmdArgv) != TCL_OK) {
	    return TCL_ERROR;
	}

	mode = TclGetOpenModeEx(interp, modeString, &seekFlag, &binary);
	if (mode == -1) {
	    chan = NULL;
	} else {
	    int flags = TCL_STDERR | TCL_ENFORCE_MODE;

	    switch (mode & (O_RDONLY | O_WRONLY | O_RDWR)) {
	    case O_RDONLY:
		flags |= TCL_STDOUT;
		break;
	    case O_WRONLY:
		flags |= TCL_STDIN;
		break;
	    case O_RDWR:
		flags |= (TCL_STDIN | TCL_STDOUT);
		break;
	    default:
		Tcl_Panic("Tcl_OpenCmd: invalid mode value");
		break;
	    }
	    chan = Tcl_OpenCommandChannel(interp, cmdObjc, cmdArgv, flags);
	    if (binary) {
		Tcl_SetChannelOption(interp, chan, "-translation", "binary");
	    }
	}
	ckfree((char *) cmdArgv);
    }
    if (chan == (Tcl_Channel) NULL) {
	return TCL_ERROR;
    }
    Tcl_RegisterChannel(interp, chan);
    Tcl_AppendResult(interp, Tcl_GetChannelName(chan), NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TcpAcceptCallbacksDeleteProc --
 *
 *	Assocdata cleanup routine called when an interpreter is being deleted
 *	to set the interp field of all the accept callback records registered
 *	with the interpreter to NULL. This will prevent the interpreter from
 *	being used in the future to eval accept scripts.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deallocates memory and sets the interp field of all the accept
 *	callback records to NULL to prevent this interpreter from being used
 *	subsequently to eval accept scripts.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
TcpAcceptCallbacksDeleteProc(
    ClientData clientData,	/* Data which was passed when the assocdata
				 * was registered. */
    Tcl_Interp *interp)		/* Interpreter being deleted - not used. */
{
    Tcl_HashTable *hTblPtr;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch hSearch;
    AcceptCallback *acceptCallbackPtr;

    hTblPtr = (Tcl_HashTable *) clientData;
    for (hPtr = Tcl_FirstHashEntry(hTblPtr, &hSearch);
	    hPtr != NULL; hPtr = Tcl_NextHashEntry(&hSearch)) {
	acceptCallbackPtr = (AcceptCallback *) Tcl_GetHashValue(hPtr);
	acceptCallbackPtr->interp = NULL;
    }
    Tcl_DeleteHashTable(hTblPtr);
    ckfree((char *) hTblPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * RegisterTcpServerInterpCleanup --
 *
 *	Registers an accept callback record to have its interp field set to
 *	NULL when the interpreter is deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When, in the future, the interpreter is deleted, the interp field of
 *	the accept callback data structure will be set to NULL. This will
 *	prevent attempts to eval the accept script in a deleted interpreter.
 *
 *----------------------------------------------------------------------
 */

static void
RegisterTcpServerInterpCleanup(
    Tcl_Interp *interp,		/* Interpreter for which we want to be
				 * informed of deletion. */
    AcceptCallback *acceptCallbackPtr)
				/* The accept callback record whose interp
				 * field we want set to NULL when the
				 * interpreter is deleted. */
{
    Tcl_HashTable *hTblPtr;	/* Hash table for accept callback records to
				 * smash when the interpreter will be
				 * deleted. */
    Tcl_HashEntry *hPtr;	/* Entry for this record. */
    int isNew;			/* Is the entry new? */

    hTblPtr = (Tcl_HashTable *)
	    Tcl_GetAssocData(interp, "tclTCPAcceptCallbacks", NULL);

    if (hTblPtr == NULL) {
	hTblPtr = (Tcl_HashTable *) ckalloc((unsigned) sizeof(Tcl_HashTable));
	Tcl_InitHashTable(hTblPtr, TCL_ONE_WORD_KEYS);
	(void) Tcl_SetAssocData(interp, "tclTCPAcceptCallbacks",
		TcpAcceptCallbacksDeleteProc, (ClientData) hTblPtr);
    }

    hPtr = Tcl_CreateHashEntry(hTblPtr, (char *) acceptCallbackPtr, &isNew);
    if (!isNew) {
	Tcl_Panic("RegisterTcpServerCleanup: damaged accept record table");
    }
    Tcl_SetHashValue(hPtr, (ClientData) acceptCallbackPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * UnregisterTcpServerInterpCleanupProc --
 *
 *	Unregister a previously registered accept callback record. The interp
 *	field of this record will no longer be set to NULL in the future when
 *	the interpreter is deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prevents the interp field of the accept callback record from being set
 *	to NULL in the future when the interpreter is deleted.
 *
 *----------------------------------------------------------------------
 */

static void
UnregisterTcpServerInterpCleanupProc(
    Tcl_Interp *interp,		/* Interpreter in which the accept callback
				 * record was registered. */
    AcceptCallback *acceptCallbackPtr)
				/* The record for which to delete the
				 * registration. */
{
    Tcl_HashTable *hTblPtr;
    Tcl_HashEntry *hPtr;

    hTblPtr = (Tcl_HashTable *) Tcl_GetAssocData(interp,
	    "tclTCPAcceptCallbacks", NULL);
    if (hTblPtr == NULL) {
	return;
    }

    hPtr = Tcl_FindHashEntry(hTblPtr, (char *) acceptCallbackPtr);
    if (hPtr != NULL) {
	Tcl_DeleteHashEntry(hPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * AcceptCallbackProc --
 *
 *	This callback is invoked by the TCP channel driver when it accepts a
 *	new connection from a client on a server socket.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Whatever the script does.
 *
 *----------------------------------------------------------------------
 */

static void
AcceptCallbackProc(
    ClientData callbackData,	/* The data stored when the callback was
				 * created in the call to
				 * Tcl_OpenTcpServer. */
    Tcl_Channel chan,		/* Channel for the newly accepted
				 * connection. */
    char *address,		/* Address of client that was accepted. */
    int port)			/* Port of client that was accepted. */
{
    AcceptCallback *acceptCallbackPtr;
    Tcl_Interp *interp;
    char *script;
    char portBuf[TCL_INTEGER_SPACE];
    int result;

    acceptCallbackPtr = (AcceptCallback *) callbackData;

    /*
     * Check if the callback is still valid; the interpreter may have gone
     * away, this is signalled by setting the interp field of the callback
     * data to NULL.
     */

    if (acceptCallbackPtr->interp != NULL) {

	script = acceptCallbackPtr->script;
	interp = acceptCallbackPtr->interp;

	Tcl_Preserve((ClientData) script);
	Tcl_Preserve((ClientData) interp);

	TclFormatInt(portBuf, port);
	Tcl_RegisterChannel(interp, chan);

	/*
	 * Artificially bump the refcount to protect the channel from being
	 * deleted while the script is being evaluated.
	 */

	Tcl_RegisterChannel(NULL, chan);

	result = Tcl_VarEval(interp, script, " ", Tcl_GetChannelName(chan),
		" ", address, " ", portBuf, NULL);
	if (result != TCL_OK) {
	    Tcl_BackgroundError(interp);
	    Tcl_UnregisterChannel(interp, chan);
	}

	/*
	 * Decrement the artificially bumped refcount. After this it is not
	 * safe anymore to use "chan", because it may now be deleted.
	 */

	Tcl_UnregisterChannel(NULL, chan);

	Tcl_Release((ClientData) interp);
	Tcl_Release((ClientData) script);

    } else {
	/*
	 * The interpreter has been deleted, so there is no useful way to
	 * utilize the client socket - just close it.
	 */

	Tcl_Close(NULL, chan);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TcpServerCloseProc --
 *
 *	This callback is called when the TCP server channel for which it was
 *	registered is being closed. It informs the interpreter in which the
 *	accept script is evaluated (if that interpreter still exists) that
 *	this channel no longer needs to be informed if the interpreter is
 *	deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	In the future, if the interpreter is deleted this channel will no
 *	longer be informed.
 *
 *----------------------------------------------------------------------
 */

static void
TcpServerCloseProc(
    ClientData callbackData)	/* The data passed in the call to
				 * Tcl_CreateCloseHandler. */
{
    AcceptCallback *acceptCallbackPtr;
				/* The actual data. */

    acceptCallbackPtr = (AcceptCallback *) callbackData;
    if (acceptCallbackPtr->interp != NULL) {
	UnregisterTcpServerInterpCleanupProc(acceptCallbackPtr->interp,
		acceptCallbackPtr);
    }
    Tcl_EventuallyFree((ClientData) acceptCallbackPtr->script, TCL_DYNAMIC);
    ckfree((char *) acceptCallbackPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SocketObjCmd --
 *
 *	This function is invoked to process the "socket" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates a socket based channel.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SocketObjCmd(
    ClientData notUsed,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    static const char *socketOptions[] = {
	"-async", "-myaddr", "-myport","-server", NULL
    };
    enum socketOptions {
	SKT_ASYNC, SKT_MYADDR, SKT_MYPORT, SKT_SERVER
    };
    int optionIndex, a, server, port;
    char *arg, *copyScript, *host, *script;
    char *myaddr = NULL;
    int myport = 0;
    int async = 0;
    Tcl_Channel chan;
    AcceptCallback *acceptCallbackPtr;

    server = 0;
    script = NULL;

    if (TclpHasSockets(interp) != TCL_OK) {
	return TCL_ERROR;
    }

    for (a = 1; a < objc; a++) {
	arg = Tcl_GetString(objv[a]);
	if (arg[0] != '-') {
	    break;
	}
	if (Tcl_GetIndexFromObj(interp, objv[a], socketOptions, "option",
		TCL_EXACT, &optionIndex) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch ((enum socketOptions) optionIndex) {
	case SKT_ASYNC:
	    if (server == 1) {
		Tcl_AppendResult(interp,
			"cannot set -async option for server sockets", NULL);
		return TCL_ERROR;
	    }
	    async = 1;
	    break;
	case SKT_MYADDR:
	    a++;
	    if (a >= objc) {
		Tcl_AppendResult(interp,
			"no argument given for -myaddr option", NULL);
		return TCL_ERROR;
	    }
	    myaddr = Tcl_GetString(objv[a]);
	    break;
	case SKT_MYPORT: {
	    char *myPortName;

	    a++;
	    if (a >= objc) {
		Tcl_AppendResult(interp,
			"no argument given for -myport option", NULL);
		return TCL_ERROR;
	    }
	    myPortName = Tcl_GetString(objv[a]);
	    if (TclSockGetPort(interp, myPortName, "tcp", &myport) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	}
	case SKT_SERVER:
	    if (async == 1) {
		Tcl_AppendResult(interp,
			"cannot set -async option for server sockets", NULL);
		return TCL_ERROR;
	    }
	    server = 1;
	    a++;
	    if (a >= objc) {
		Tcl_AppendResult(interp,
			"no argument given for -server option", NULL);
		return TCL_ERROR;
	    }
	    script = Tcl_GetString(objv[a]);
	    break;
	default:
	    Tcl_Panic("Tcl_SocketObjCmd: bad option index to SocketOptions");
	}
    }
    if (server) {
	host = myaddr;		/* NULL implies INADDR_ANY */
	if (myport != 0) {
	    Tcl_AppendResult(interp, "option -myport is not valid for servers",
		    NULL);
	    return TCL_ERROR;
	}
    } else if (a < objc) {
	host = Tcl_GetString(objv[a]);
	a++;
    } else {
	Interp *iPtr;

    wrongNumArgs:
	iPtr = (Interp *) interp;
	Tcl_WrongNumArgs(interp, 1, objv,
		"?-myaddr addr? ?-myport myport? ?-async? host port");
	iPtr->flags |= INTERP_ALTERNATE_WRONG_ARGS;
	Tcl_WrongNumArgs(interp, 1, objv,
		"-server command ?-myaddr addr? port");
	iPtr->flags &= ~INTERP_ALTERNATE_WRONG_ARGS;
	return TCL_ERROR;
    }

    if (a == objc-1) {
	if (TclSockGetPort(interp, Tcl_GetString(objv[a]), "tcp",
		&port) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	goto wrongNumArgs;
    }

    if (server) {
	acceptCallbackPtr = (AcceptCallback *) ckalloc((unsigned)
		sizeof(AcceptCallback));
	copyScript = ckalloc((unsigned) strlen(script) + 1);
	strcpy(copyScript, script);
	acceptCallbackPtr->script = copyScript;
	acceptCallbackPtr->interp = interp;
	chan = Tcl_OpenTcpServer(interp, port, host, AcceptCallbackProc,
		(ClientData) acceptCallbackPtr);
	if (chan == (Tcl_Channel) NULL) {
	    ckfree(copyScript);
	    ckfree((char *) acceptCallbackPtr);
	    return TCL_ERROR;
	}

	/*
	 * Register with the interpreter to let us know when the interpreter
	 * is deleted (by having the callback set the interp field of the
	 * acceptCallbackPtr's structure to NULL). This is to avoid trying to
	 * eval the script in a deleted interpreter.
	 */

	RegisterTcpServerInterpCleanup(interp, acceptCallbackPtr);

	/*
	 * Register a close callback. This callback will inform the
	 * interpreter (if it still exists) that this channel does not need to
	 * be informed when the interpreter is deleted.
	 */

	Tcl_CreateCloseHandler(chan, TcpServerCloseProc,
		(ClientData) acceptCallbackPtr);
    } else {
	chan = Tcl_OpenTcpClient(interp, port, host, myaddr, myport, async);
	if (chan == (Tcl_Channel) NULL) {
	    return TCL_ERROR;
	}
    }
    Tcl_RegisterChannel(interp, chan);
    Tcl_AppendResult(interp, Tcl_GetChannelName(chan), NULL);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FcopyObjCmd --
 *
 *	This function is invoked to process the "fcopy" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Moves data between two channels and possibly sets up a background copy
 *	handler.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_FcopyObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel inChan, outChan;
    char *arg;
    int mode, i;
    int toRead, index;
    Tcl_Obj *cmdPtr;
    static const char* switches[] = { "-size", "-command", NULL };
    enum { FcopySize, FcopyCommand };

    if ((objc < 3) || (objc > 7) || (objc == 4) || (objc == 6)) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"input output ?-size size? ?-command callback?");
	return TCL_ERROR;
    }

    /*
     * Parse the channel arguments and verify that they are readable or
     * writable, as appropriate.
     */

    arg = Tcl_GetString(objv[1]);
    inChan = Tcl_GetChannel(interp, arg, &mode);
    if (inChan == (Tcl_Channel) NULL) {
	return TCL_ERROR;
    }
    if ((mode & TCL_READABLE) == 0) {
	Tcl_AppendResult(interp, "channel \"", arg,
		"\" wasn't opened for reading", NULL);
	return TCL_ERROR;
    }
    arg = Tcl_GetString(objv[2]);
    outChan = Tcl_GetChannel(interp, arg, &mode);
    if (outChan == (Tcl_Channel) NULL) {
	return TCL_ERROR;
    }
    if ((mode & TCL_WRITABLE) == 0) {
	Tcl_AppendResult(interp, "channel \"", arg,
		"\" wasn't opened for writing", NULL);
	return TCL_ERROR;
    }

    toRead = -1;
    cmdPtr = NULL;
    for (i = 3; i < objc; i += 2) {
	if (Tcl_GetIndexFromObj(interp, objv[i], switches, "switch", 0,
		(int *) &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch (index) {
	case FcopySize:
	    if (Tcl_GetIntFromObj(interp, objv[i+1], &toRead) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	case FcopyCommand:
	    cmdPtr = objv[i+1];
	    break;
	}
    }

    return TclCopyChannel(interp, inChan, outChan, toRead, cmdPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclChanPendingObjCmd --
 *
 *	This function is invoked to process the Tcl "chan pending" 
 *	command (TIP #287). See the user documentation for details on 
 *	what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Sets interp's result to the number of bytes of buffered input or
 *	output (depending on whether the first argument is "input" or
 *	"output"), or -1 if the channel wasn't opened for that mode.
 *
 *---------------------------------------------------------------------------
 */

	/* ARGSUSED */
int
TclChanPendingObjCmd(
    ClientData unused,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;
    int index, mode;
    char *arg;
    static const char *options[] = {"input", "output", (char *) NULL};
    enum options {PENDING_INPUT, PENDING_OUTPUT};

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "mode channelId");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "mode", 0, 
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    arg = Tcl_GetString(objv[2]);
    chan = Tcl_GetChannel(interp, arg, &mode);
    if (chan == NULL) {
	return TCL_ERROR;
    }

    switch ((enum options) index) {
    case PENDING_INPUT:
	if ((mode & TCL_READABLE) == 0) {
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(-1));
	} else {
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(Tcl_InputBuffered(chan)));
	}
	break;
    case PENDING_OUTPUT:
	if ((mode & TCL_WRITABLE) == 0) {
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(-1));
	} else {
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(Tcl_OutputBuffered(chan)));
	}
	break;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ChanTruncateObjCmd --
 *
 *	This function is invoked to process the "chan truncate" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Truncates a channel (or rather a file underlying a channel).
 *
 *----------------------------------------------------------------------
 */

int
TclChanTruncateObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;
    int mode;
    Tcl_WideInt length;
    char *chanName;

    if ((objc < 2) || (objc > 3)) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId ?length?");
	return TCL_ERROR;
    }
    chanName = TclGetString(objv[1]);
    chan = Tcl_GetChannel(interp, chanName, &mode);
    if (chan == NULL) {
	return TCL_ERROR;
    }

    if (objc == 3) {
	/*
	 * User is supplying an explicit length.
	 */

	if (Tcl_GetWideIntFromObj(interp, objv[2], &length) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (length < 0) {
	    Tcl_AppendResult(interp,
		    "cannot truncate to negative length of file", NULL);
	    return TCL_ERROR;
	}
    } else {
	/*
	 * User wants to truncate to the current file position.
	 */

	length = Tcl_Tell(chan);
	if (length == Tcl_WideAsLong(-1)) {
	    Tcl_AppendResult(interp,
		    "could not determine current location in \"", chanName,
		    "\": ", Tcl_PosixError(interp), NULL);
	    return TCL_ERROR;
	}
    }

    if (Tcl_TruncateChannel(chan, length) != TCL_OK) {
	Tcl_AppendResult(interp, "error during truncate on \"", chanName,
		"\": ", Tcl_PosixError(interp), NULL);
	return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */

