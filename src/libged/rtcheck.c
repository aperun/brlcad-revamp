/*                         R T C H E C K . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2009 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file rtcheck.c
 *
 * The rtcheck command.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "bio.h"

#include "cmd.h"
#include "solid.h"

#include "./ged_private.h"


struct ged_rtcheck {
#ifdef _WIN32
    HANDLE		fd;
    HANDLE		hProcess;
    DWORD		pid;
#ifdef TCL_OK
    Tcl_Channel		chan;
#else
    genptr_t		chan;
#endif
#else
    int			fd;
    int			pid;
#endif
    FILE		*fp;
    struct bn_vlblock	*vbp;
    struct bu_list	*vhead;
    double		csize;
    struct ged		*gedp;
    Tcl_Interp		*interp;
};

struct ged_rtcheck_output {
#ifdef _WIN32
    HANDLE	fd;
    Tcl_Channel	chan;
#else
    int		fd;
#endif
    struct ged	*gedp;
    Tcl_Interp	*interp;
};


static void ged_rtcheck_vector_handler(ClientData clientData, int mask);
static void ged_rtcheck_output_handler(ClientData clientData, int mask);


/*
 * Check for overlaps in the current view.
 *
 * Usage:
 *        rtcheck options
 *
 */
int
ged_rtcheck(struct ged *gedp, int argc, const char *argv[])
{
    register char **vp;
    register int i;
#ifndef _WIN32
    int	pid;
    int	i_pipe[2];	/* object reads results for building vectors */
    int	o_pipe[2];	/* object writes view parameters */
    int	e_pipe[2];	/* object reads textual results */
#else
    HANDLE	i_pipe[2], pipe_iDup;	/* MGED reads results for building vectors */
    HANDLE	o_pipe[2], pipe_oDup;	/* MGED writes view parameters */
    HANDLE	e_pipe[2], pipe_eDup;	/* MGED reads textual results */
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    char line[2048];
    char name[256];
#endif
    FILE	*fp;
    struct ged_rtcheck *rtcp;
    struct ged_rtcheck_output *rtcop;
    vect_t eye_model;
    static const char *usage = "options";

    const char *bin;
    char rtcheck[256] = {0};

    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
    GED_CHECK_DRAWABLE(gedp, GED_ERROR);
    GED_CHECK_VIEW(gedp, GED_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    bin = bu_brlcad_root("bin", 1);
    if (bin) {
#ifdef _WIN32
	snprintf(rtcheck, 256, "\"%s/%s\"", bin, argv[0]);
#else
	snprintf(rtcheck, 256, "%s/%s", bin, argv[0]);
#endif
    }

#ifndef _WIN32
    vp = &gedp->ged_gdp->gd_rt_cmd[0];
    *vp++ = rtcheck;
    *vp++ = "-M";
    for (i=1; i < argc; i++)
	*vp++ = (char *)argv[i];
    *vp++ = gedp->ged_wdbp->dbip->dbi_filename;

    /*
     * Now that we've grabbed all the options, if no args remain,
     * append the names of all stuff currently displayed.
     * Otherwise, simply append the remaining args.
     */
    if (i == argc) {
	gedp->ged_gdp->gd_rt_cmd_len = vp - gedp->ged_gdp->gd_rt_cmd;
	gedp->ged_gdp->gd_rt_cmd_len += ged_build_tops(gedp,
					       vp,
					       &gedp->ged_gdp->gd_rt_cmd[MAXARGS]);
    } else {
	while (i < argc)
	    *vp++ = (char *)argv[i++];
	*vp = 0;
	vp = &gedp->ged_gdp->gd_rt_cmd[0];
	while (*vp)
	    Tcl_AppendResult(brlcad_interp, *vp++, " ", (char *)NULL);

	Tcl_AppendResult(brlcad_interp, "\n", (char *)NULL);
    }

    (void)pipe(i_pipe);
    (void)pipe(o_pipe);
    (void)pipe(e_pipe);

    if ((pid = fork()) == 0) {
	/* Redirect stdin, stdout and stderr */
	(void)close(0);
	(void)dup(o_pipe[0]);
	(void)close(1);
	(void)dup(i_pipe[1]);
	(void)close(2);
	(void)dup(e_pipe[1]);

	/* close pipes */
	(void)close(i_pipe[0]);
	(void)close(i_pipe[1]);
	(void)close(o_pipe[0]);
	(void)close(o_pipe[1]);
	(void)close(e_pipe[0]);
	(void)close(e_pipe[1]);

	for (i=3; i < 20; i++)
	    (void)close(i);

	(void)execvp(gedp->ged_gdp->gd_rt_cmd[0], gedp->ged_gdp->gd_rt_cmd);
	perror(gedp->ged_gdp->gd_rt_cmd[0]);
	exit(16);
    }

    /* As parent, send view information down pipe */
    (void)close(o_pipe[0]);
    fp = fdopen(o_pipe[1], "w");

    ged_rt_set_eye_model(gedp, eye_model);
    ged_rt_write(gedp, fp, eye_model);
    (void)fclose(fp);

    /* close write end of pipes */
    (void)close(i_pipe[1]);
    (void)close(e_pipe[1]);

    BU_GETSTRUCT(rtcp, ged_rtcheck);

    /* initialize the rtcheck struct */
    rtcp->fd = i_pipe[0];
    rtcp->fp = fdopen(i_pipe[0], "r");
    rtcp->pid = pid;
    rtcp->vbp = rt_vlblock_init();
    rtcp->vhead = rt_vlblock_find(rtcp->vbp, 0xFF, 0xFF, 0x00);
    rtcp->csize = gedp->ged_gvp->gv_scale * 0.01;
    rtcp->gedp = gedp;
    rtcp->interp = brlcad_interp;

    /* register file handlers */
    Tcl_CreateFileHandler(i_pipe[0], TCL_READABLE,
			  ged_rtcheck_vector_handler, (ClientData)rtcp);

    BU_GETSTRUCT(rtcop, ged_rtcheck_output);
    rtcop->fd = e_pipe[0];
    rtcop->gedp = gedp;
    rtcop->interp = brlcad_interp;
    Tcl_CreateFileHandler(rtcop->fd,
			  TCL_READABLE,
			  ged_rtcheck_output_handler,
			  (ClientData)rtcop);

    return GED_OK;
#else
    /* _WIN32 */
    vp = &gedp->ged_gdp->gd_rt_cmd[0];
    *vp++ = rtcheck;
    *vp++ = "-M";
    for (i=1; i < argc; i++)
	*vp++ = (char *)argv[i];

    {
	char buf[512];

	snprintf(buf, 512, "\"%s\"", gedp->ged_wdbp->dbip->dbi_filename);
	*vp++ = buf;
    }

    /*
     * Now that we've grabbed all the options, if no args remain,
     * append the names of all stuff currently displayed.
     * Otherwise, simply append the remaining args.
     */
    if (i == argc) {
	gedp->ged_gdp->gd_rt_cmd_len = vp - gedp->ged_gdp->gd_rt_cmd;
	gedp->ged_gdp->gd_rt_cmd_len += ged_build_tops(gedp,
					       vp,
					       &gedp->ged_gdp->gd_rt_cmd[MAXARGS]);
    } else {
	while (i < argc)
	    *vp++ = (char *)argv[i++];
	*vp = 0;
	vp = &gedp->ged_gdp->gd_rt_cmd[0];
	while (*vp)
	    Tcl_AppendResult(brlcad_interp, *vp++, " ", (char *)NULL);

	Tcl_AppendResult(brlcad_interp, "\n", (char *)NULL);
    }


    memset((void *)&si, 0, sizeof(STARTUPINFO));
    memset((void *)&pi, 0, sizeof(PROCESS_INFORMATION));
    memset((void *)&sa, 0, sizeof(SECURITY_ATTRIBUTES));

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    /* Create a pipe for the child process's STDERR. */
    CreatePipe( &e_pipe[0], &e_pipe[1], &sa, 0);

    /* Create noninheritable read handle and close the inheritable read handle. */
    DuplicateHandle( GetCurrentProcess(), e_pipe[0],
		     GetCurrentProcess(),  &pipe_eDup,
		     0,  FALSE,
		     DUPLICATE_SAME_ACCESS );
    CloseHandle( e_pipe[0]);

    /* Create a pipe for the child process's STDOUT. */
    CreatePipe( &o_pipe[0], &o_pipe[1], &sa, 0);

    /* Create noninheritable write handle and close the inheritable writehandle. */
    DuplicateHandle( GetCurrentProcess(), o_pipe[1],
		     GetCurrentProcess(),  &pipe_oDup ,
		     0,  FALSE,
		     DUPLICATE_SAME_ACCESS );
    CloseHandle( o_pipe[1]);

    /* Create a pipe for the child process's STDIN. */
    CreatePipe(&i_pipe[0], &i_pipe[1], &sa, 0);

    /* Duplicate the read handle to the pipe so it is not inherited. */
    DuplicateHandle(GetCurrentProcess(), i_pipe[0],
		    GetCurrentProcess(), &pipe_iDup,
		    0, FALSE,                  /* not inherited */
		    DUPLICATE_SAME_ACCESS );
    CloseHandle(i_pipe[0]);


    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = NULL;
    si.lpReserved2 = NULL;
    si.cbReserved2 = 0;
    si.lpDesktop = NULL;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdInput   = o_pipe[0];
    si.hStdOutput  = i_pipe[1];
    si.hStdError   = e_pipe[1];
    si.wShowWindow = SW_HIDE;

    snprintf(line, sizeof(line), "%s ", gedp->ged_gdp->gd_rt_cmd[0]);
    for (i=1; i < gedp->ged_gdp->gd_rt_cmd_len; i++) {
	snprintf(name, sizeof(name), "%s ", gedp->ged_gdp->gd_rt_cmd[i]);
	bu_strlcat(line, name, sizeof(line));
    }

    CreateProcess(NULL, line, NULL, NULL, TRUE,
		  DETACHED_PROCESS, NULL, NULL,
		  &si, &pi);

    /* close read end of pipe */
    CloseHandle(o_pipe[0]);

    /* close write end of pipes */
    (void)CloseHandle(i_pipe[1]);
    (void)CloseHandle(e_pipe[1]);

    /* As parent, send view information down pipe */
    fp = _fdopen(_open_osfhandle((HFILE)pipe_oDup, _O_TEXT), "wb");
    setmode(_fileno(fp), O_BINARY);

    ged_rt_set_eye_model(gedp, eye_model);
    ged_rt_write(gedp, fp, eye_model);
    (void)fclose(fp);

    BU_GETSTRUCT(rtcp, ged_rtcheck);

    /* initialize the rtcheck struct */
    rtcp->fd = pipe_iDup;
    rtcp->fp = _fdopen( _open_osfhandle((HFILE)pipe_iDup, _O_TEXT), "rb" );
    setmode(_fileno(rtcp->fp), O_BINARY);
    rtcp->hProcess = pi.hProcess;
    rtcp->pid = pi.dwProcessId;
    rtcp->vbp = rt_vlblock_init();
    rtcp->vhead = rt_vlblock_find(rtcp->vbp, 0xFF, 0xFF, 0x00);
    rtcp->csize = gedp->ged_gvp->gv_scale * 0.01;
    rtcp->gedp = gedp;
    rtcp->interp = brlcad_interp;

    rtcp->chan = Tcl_MakeFileChannel(pipe_iDup, TCL_READABLE);
    Tcl_CreateChannelHandler(rtcp->chan, TCL_READABLE,
			     ged_rtcheck_vector_handler,
			     (ClientData)rtcp);

    BU_GETSTRUCT(rtcop, ged_rtcheck_output);
    rtcop->fd = pipe_eDup;
    rtcop->chan = Tcl_MakeFileChannel(pipe_eDup, TCL_READABLE);
    rtcop->gedp = gedp;
    rtcop->interp = brlcad_interp;
    Tcl_CreateChannelHandler(rtcop->chan,
			     TCL_READABLE,
			     ged_rtcheck_output_handler,
			     (ClientData)rtcop);
    return GED_OK;
#endif
}

#ifndef _WIN32
static void
ged_rtcheck_vector_handler(ClientData clientData, int mask)
{
    register struct ged_display_list *gdlp;
    register struct ged_display_list *next_gdlp;
    int value;
    struct solid *sp;
    struct ged_rtcheck *rtcp = (struct ged_rtcheck *)clientData;

    /* Get vector output from rtcheck */
    if ((value = getc(rtcp->fp)) == EOF) {
	int retcode;
	int rpid;

	Tcl_DeleteFileHandler(rtcp->fd);
	fclose(rtcp->fp);

	gdlp = BU_LIST_NEXT(ged_display_list, &rtcp->gedp->ged_gdp->gd_headDisplay);
	while (BU_LIST_NOT_HEAD(gdlp, &rtcp->gedp->ged_gdp->gd_headDisplay)) {
	    next_gdlp = BU_LIST_PNEXT(ged_display_list, gdlp);

	    FOR_ALL_SOLIDS(sp, &gdlp->gdl_headSolid)
		sp->s_flag = DOWN;

	    gdlp = next_gdlp;
	}

	/* Add overlay */
	ged_cvt_vlblock_to_solids(rtcp->gedp, rtcp->vbp, "OVERLAPS", 0);
	rt_vlblock_free(rtcp->vbp);

	/* wait for the forked process */
	while ((rpid = wait(&retcode)) != rtcp->pid && rpid != -1) {
	    
	    ged_wait_status(&rtcp->gedp->ged_result_str, retcode);
	}

#if 0
	dgo_notify(rtcp->dgop, rtcp->interp);
#endif

	/* free rtcp */
	bu_free((genptr_t)rtcp, "ged_rtcheck_vector_handler: rtcp");

	return;
    }

    (void)rt_process_uplot_value(&rtcp->vhead,
				 rtcp->vbp,
				 rtcp->fp,
				 value,
				 rtcp->csize,
				 rtcp->gedp->ged_gdp->gd_uplotOutputMode);
}

static void
ged_rtcheck_output_handler(ClientData clientData, int mask)
{
    int count;
    char line[RT_MAXLINE] = {0};
    struct ged_rtcheck_output *rtcop = (struct ged_rtcheck_output *)clientData;

    /* Get textual output from rtcheck */
    count = read((int)rtcop->fd, line, RT_MAXLINE);
    if (count <= 0) {
	if (count < 0) {
	    perror("READ ERROR");
	}
	Tcl_DeleteFileHandler(rtcop->fd);
	close(rtcop->fd);

	if (rtcop->gedp->ged_gdp->gd_rtCmdNotify != (void (*)())0)
	    rtcop->gedp->ged_gdp->gd_rtCmdNotify();

	bu_free((genptr_t)rtcop, "ged_rtcheck_output_handler: rtcop");
	return;
    }

    line[count] = '\0';
    if (rtcop->gedp->ged_output_handler != (void (*)())0)
	rtcop->gedp->ged_output_handler(rtcop->gedp, line);
    else
	bu_vls_printf(&rtcop->gedp->ged_result_str, "%s", line);
}

#else

void
ged_rtcheck_vector_handler(ClientData clientData, int mask)
{
    register struct ged_display_list *gdlp;
    register struct ged_display_list *next_gdlp;
    int value;
    struct solid *sp;
    struct ged_rtcheck *rtcp = (struct ged_rtcheck *)clientData;

    /* Get vector output from rtcheck */
    if (feof(rtcp->fp)) {
	Tcl_DeleteChannelHandler(rtcp->chan,
				 ged_rtcheck_vector_handler,
				 (ClientData)rtcp);
	Tcl_Close(rtcp->interp, rtcp->chan);

	gdlp = BU_LIST_NEXT(ged_display_list, &rtcp->gedp->ged_gdp->gd_headDisplay);
	while (BU_LIST_NOT_HEAD(gdlp, &rtcp->gedp->ged_gdp->gd_headDisplay)) {
	    next_gdlp = BU_LIST_PNEXT(ged_display_list, gdlp);

	    FOR_ALL_SOLIDS(sp, &gdlp->gdl_headSolid)
		sp->s_flag = DOWN;

	    gdlp = next_gdlp;
	}

	/* Add overlay */
	ged_cvt_vlblock_to_solids(rtcp->gedp, rtcp->vbp, "OVERLAPS", 0);
	rt_vlblock_free(rtcp->vbp);

	/* wait for the forked process */
	WaitForSingleObject( rtcp->hProcess, INFINITE );

#if 0
	dgo_notify(rtcp->dgop, rtcp->interp);
#endif

	/* free rtcp */
	bu_free((genptr_t)rtcp, "ged_rtcheck_vector_handler: rtcp");

	return;
    }

    value = getc(rtcp->fp);
    (void)rt_process_uplot_value(&rtcp->vhead,
				 rtcp->vbp,
				 rtcp->fp,
				 value,
				 rtcp->csize,
				 rtcp->gedp->ged_gdp->gd_uplotOutputMode);
}

void
ged_rtcheck_output_handler(ClientData clientData, int mask)
{
    int count;
    char line[RT_MAXLINE];
    struct ged_rtcheck_output *rtcop = (struct ged_rtcheck_output *)clientData;

    /* Get textual output from rtcheck */
    if (Tcl_Eof(rtcop->chan) ||
	(!ReadFile(rtcop->fd, line, RT_MAXLINE, &count, 0))) {

	Tcl_DeleteChannelHandler(rtcop->chan,
				 ged_rtcheck_output_handler,
				 (ClientData)rtcop);
	Tcl_Close(rtcop->interp, rtcop->chan);

	if (rtcop->gedp->ged_gdp->gd_rtCmdNotify != (void (*)())0)
	    rtcop->gedp->ged_gdp->gd_rtCmdNotify();

	bu_free((genptr_t)rtcop, "ged_rtcheck_output_handler: rtcop");

	return;
    }

    line[count] = '\0';
    if (rtcop->gedp->ged_output_handler != (void (*)())0)
	rtcop->gedp->ged_output_handler(rtcop->gedp, line);
    else
	bu_vls_printf(&rtcop->gedp->ged_result_str, "%s", line);
}

#endif

void
ged_wait_status(struct bu_vls	*log,
		int		status)
{
    int	sig = status & 0x7f;
    int	core = status & 0x80;
    int	ret = status >> 8;

    if (status == 0) {
	bu_vls_printf(log, "Normal exit\n");
	return;
    }

    bu_vls_printf(log, "Abnormal exit x%x", status);

    if (core)
	bu_vls_printf(log, ", core dumped");

    if (sig)
	bu_vls_printf(log, ", terminating signal = %d", sig);
    else
	bu_vls_printf(log, ", return (exit) code = %d", ret);
}


/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
