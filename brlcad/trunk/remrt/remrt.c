/*
 *  			R E M R T . C
 *  
 *  Controller for network ray-tracing
 *  Operating as both a network client and server,
 *  starts remote invocations of "rtsrv" via "rsh", then
 *  handles incomming connections offering cycles,
 *  accepting input (solicited and unsolicited) via the pkg.c routines,
 *  and storing the results in files and/or a framebuffer.
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
 *	This software is Copyright (C) 1989 by the United States Army.
 *	All rights reserved.
 */
static char RCSid[] = "@(#)$Header$ (BRL)";

#include <stdio.h>
#define _BSD_TYPES		/* Needed for IRIX 5.0.1 */
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <math.h>
#ifdef SYSV
# include <string.h>
#else
# include <strings.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

/*
 *  The situation with sys/time.h and time.h is crazy.
 *  We need sys/time.h for struct timeval,
 *  and time.h for struct tm.
 *
 *  on BSD (and SGI 4D), sys/time.h includes time.h,
 *  on the XMP (UNICOS 3 & 4), time.h includes sys/time.h,
 *  on the Cray-2, there is no automatic including.
 *
 *  Note that on many SYSV machines, the Cakefile has to set BSD
 */
#if BSD && !SYSV
#  include <sys/time.h>		/* includes <time.h> */
#else
#  if CRAY1 && !__STDC__
#	include <time.h>	/* includes <sys/time.h> */
#  else
#	include <sys/time.h>
#	include <time.h>
#  endif
#endif

#include "machine.h"
#include "vmath.h"
#include "rtstring.h"
#include "rtlist.h"
#include "raytrace.h"
#include "fb.h"
#include "pkg.h"
#include "externs.h"
#include "../librt/debug.h"

#include "./list.h"
#include "./protocol.h"

#ifdef SYSV
# define vfork	fork
#endif

/* Needed to satisfy the reference created by including ../rt/opt.o */
struct command_tab rt_cmdtab[] = {
	(char *)0, (char *)0, (char *)0,
		0,		0, 0	/* END */
};

void		read_rc_file();
void		check_input();
void		addclient();
void		start_servers();
void		eat_script();
void		interactive_cmd();
void		prep_frame();
void		frame_is_done();
void		destroy_frame();
void		schedule();
void		list_remove();
void		write_fb();
void		repaint_fb();
void		size_display();
void		send_dirbuild();
void		send_gettrees();
void		send_restart();
void		send_loglvl();
void		send_matrix();
void		send_to_lines();
void		pr_list();
void		mathtab_constant();
void		add_host();
void		host_helper();
void		start_helper();
void		build_start_cmd();
void		drop_server();
void		send_do_lines();


FBIO *fbp = FBIO_NULL;		/* Current framebuffer ptr */
int cur_fbwidth;		/* current fb width */
int fbwidth;			/* fb width  - S command */
int fbheight;			/* fb height - S command */

int running = 0;		/* actually working on it */
int detached = 0;		/* continue after EOF */

/*
 * Package Handlers.
 */
void	ph_default();	/* foobar message handler */
void	ph_pixels();
void	ph_print();
void	ph_dirbuild_reply();
void	ph_gettrees_reply();
void	ph_version();
void	ph_cmd();
struct pkg_switch pkgswitch[] = {
	{ MSG_DIRBUILD_REPLY,	ph_dirbuild_reply,	"Dirbuild ACK" },
	{ MSG_GETTREES_REPLY,	ph_gettrees_reply,	"gettrees ACK" },
	{ MSG_MATRIX,	ph_default,	"Set Matrix" },
	{ MSG_LINES,	ph_default,	"Compute lines" },
	{ MSG_END,	ph_default,	"End" },
	{ MSG_PIXELS,	ph_pixels,	"Pixels" },
	{ MSG_PRINT,	ph_print,	"Log Message" },
	{ MSG_VERSION,	ph_version,	"Protocol version check" },
	{ MSG_CMD,	ph_cmd,		"Run one command" },
	{ 0,		0,		(char *)0 }
};

int clients;
int print_on = 1;
char *frame_script = NULL;

#define NFD 32
#define MAXSERVERS	NFD		/* No relay function yet */

struct list *FreeList;

/* -- */

struct frame {
	struct frame	*fr_forw;
	struct frame	*fr_back;
	int		fr_magic;	/* magic number */
	long		fr_number;	/* frame number */
	char		*fr_filename;	/* name of output file */
	int		fr_tempfile;	/* if !0, output file is temporary */
	/* options */
	int		fr_width;	/* frame width (pixels) */
	int		fr_height;	/* frame height (pixels) */
	struct list	fr_todo;	/* work still to be done */
	/* Timings */
	struct timeval	fr_start;	/* start time */
	struct timeval	fr_end;		/* end time */
	long		fr_nrays;	/* rays fired so far */
	double		fr_cpu;		/* CPU seconds used so far */
	/* Current view */
	struct rt_vls	fr_cmd;		/* RT options & command string */
	struct rt_vls	fr_after_cmd;	/* local commands, after frame done */
};
struct frame FrameHead;
struct frame *FreeFrame;
#define FRAME_NULL	((struct frame *)0)
#define FRAME_MAGIC	0xbafe12ce

#define CHECK_FRAME(_p)	CHECK_IT(_p, fr_magic, FRAME_MAGIC, "frame pointer")

#define CHECK_IT(_q,_el,_magic,_str)	\
	if( !(_q) )  { \
		rt_log("NULL %s in %s line %d\n", _str, __FILE__, __LINE__ ); \
		abort(); \
	} else if( (_q)->_el != _magic )  { \
		rt_log("ERROR %s=x%x magic was=x%x s/b=x%x in %s line %d\n", \
			_str, (_q), (_q)->_el, _magic, __FILE__, __LINE__ ); \
		abort(); \
	}


/*
 *  Macros to manage lists of frames
 */

#define GET_FRAME(p)	{ \
		if( ((p)=FreeFrame) == FRAME_NULL ) {\
			GETSTRUCT(p, frame); \
		} else { \
			FreeFrame = (p)->fr_forw; (p)->fr_forw = FRAME_NULL; \
		} \
		bzero( (p), sizeof(struct frame) ); \
		(p)->fr_magic = FRAME_MAGIC; \
		rt_vls_init( &(p)->fr_cmd ); \
		rt_vls_init( &(p)->fr_after_cmd ); \
	}
#define FREE_FRAME(p)	{ \
	rt_vls_free( &(p)->fr_cmd ); \
	rt_vls_free( &(p)->fr_after_cmd ); \
	(p)->fr_forw = FreeFrame; FreeFrame = (p); }

/* Insert "new" partition in front of "old" frame element */
#define INSERT_FRAME(new,old)	{ \
	(new)->fr_back = (old)->fr_back; \
	(old)->fr_back = (new); \
	(new)->fr_forw = (old); \
	(new)->fr_back->fr_forw = (new);  }

/* Append "new" partition after "old" frame element */
#define APPEND_FRAME(new,old)	{ \
	(new)->fr_forw = (old)->fr_forw; \
	(new)->fr_back = (old); \
	(old)->fr_forw = (new); \
	(new)->fr_forw->fr_back = (new);  }

/* Dequeue "cur" frame element from doubly-linked list */
#define DEQUEUE_FRAME(cur)	{ \
	(cur)->fr_forw->fr_back = (cur)->fr_back; \
	(cur)->fr_back->fr_forw = (cur)->fr_forw;  }

/* --- */

/*
 *  In order to preserve asynchrony, each server is marched through
 *  a series of state transitions.
 *  Each transition is triggered by some event being satisfied.
 *  The valid state transition sequences are:
 *
 *  If a "transient" server shows up, just to send in one command:
 *
 *	Original	New		Event
 *	--------	-----		-----
 *	UNUSED		NEW		connection rcvd
 *	NEW		CLOSING		ph_cmd pkg rcvd.
 *	CLOSING		UNUSED		next schedule() pass closes conn.
 *
 *  If a "permanent" server shows up:
 *
 *	Original	New		Event
 *	--------	-----		-----
 *	UNUSED		NEW		connection rcvd
 *	NEW		VERSOK		ph_version pkg rcvd.
 *					Optionally send loglvl & "cd" cmds.
 *	VERSOK		DOING_DIRBUILD	MSG_DIRBUILD sent
 *	DOING_DIRBUILD	READY		MSG_DIRBUILD_REPLY rcvd
 *
 * --	READY		DOING_GETTREES	new frame:  send_gettrees(), send_matrix()
 *	DOING_GETTREES	READY		MSG_GETTREES_REPLY rcvd
 *
 * --	READY		READY		call send_do_lines(),
 *					receive ph_pixels pkg.
 *
 * --	READY		CLOSING		drop_server called.  Requeue work.
 *	CLOSING		UNUSED		next schedule() pass closes conn.
 *
 * XXX need to split sending of db name & the treetops.
 * XXX treetops need to be resent at start of each frame.
 * XXX should probably re-vamp send_matrix routine.
 */

struct servers {
	struct pkg_conn	*sr_pc;		/* PKC_NULL means slot not in use */
	struct list	sr_work;
	struct ihost	*sr_host;	/* description of this host */
	int		sr_lump;	/* # lines to send at once */
	int		sr_state;	/* Server state, SRST_xxx */
#define SRST_UNUSED		0	/* unused slot */
#define SRST_NEW		1	/* connected, awaiting vers check */
#define SRST_VERSOK		2	/* version OK, no model loaded yet */
#define SRST_DOING_DIRBUILD	3	/* doing dirbuild, awaiting response */
#define SRST_READY		4	/* loaded, ready */
#define SRST_RESTART		5	/* about to restart */
#define SRST_CLOSING		6	/* Needs to be closed */
#define SRST_DOING_GETTREES	7	/* doing gettrees */
	struct frame	*sr_curframe;	/* ptr to current frame */
	/* Timings */
	struct timeval	sr_sendtime;	/* time of last sending */
	double		sr_l_elapsed;	/* last: elapsed_sec */
	double		sr_l_el_rate;	/* last: pix/elapsed_sec */
	double		sr_w_elapsed;	/* weighted avg: pix/elapsed_sec */
	double		sr_w_rays;	/* weighted avg: rays/elapsed_sec */
	double		sr_s_elapsed;	/* sum of pix/elapsed_sec */
	double		sr_sq_elapsed;	/* sum of pix/elapsed_sec squared */
	double		sr_l_cpu;	/* cpu_sec for last scanline */
	double		sr_s_cpu;	/* sum of all cpu times */
	int		sr_nsamp;	/* number of samples summed over */
	double		sr_l_percent;	/* last: percent of CPU */
} servers[MAXSERVERS];
#define SERVERS_NULL	((struct servers *)0)

/* Internal Host table */
struct ihost {
	char		*ht_name;	/* Official name of host */
	int		ht_flags;	/* Control info about this host */
#define HT_HOLD		0x1
	int		ht_when;	/* When to use this host */
#define HT_ALWAYS	1		/* Always try to use this one */
#define HT_NIGHT	2		/* Only use at night */
#define HT_PASSIVE	3		/* May call in, never initiated */
#define HT_RS		4		/* While rays/second > ht_rs */
#define HT_PASSRS	5		/* passive rays/second */
#define HT_HACKNIGHT	6		/* Only use during "hackers night" */
	int		ht_rs;		/* rays/second level */
	int		ht_rs_miss;	/* number of consecutive misses */
	int		ht_rs_wait;	/* # of auto adds to wait before */
					/* restarting this server */
	int		ht_where;	/* Where to find database */
#define HT_CD		1		/* cd to ht_path first */
#define HT_CONVERT	2		/* cd to ht_path, asc2g database */
#define HT_USE		3		/* cd to ht_path, use asc database */
					/* best of cd and convert */
	char		*ht_path;	/* remote directory to run in */
	struct ihost	*ht_next;
};
struct ihost		*HostHead;
#define IHOST_NULL	((struct ihost *)0)
struct ihost		*host_lookup_by_name();
struct ihost		*host_lookup_by_addr();
struct ihost		*host_lookup_by_hostent();
struct ihost		*make_default_host();

/* variables shared with viewing model */
extern double	AmbientIntensity;
extern double	azimuth, elevation;
extern int	lightmodel;
int		use_air = 0;

/* variables shared with worker() */
extern int	hypersample;
extern int	jitter;
extern fastf_t	rt_perspective;
extern fastf_t	aspect;
extern fastf_t	eye_backoff;
extern int	width;
extern int	height;

/* variables shared with do.c */
extern int	matflag;
extern int	interactive;
extern int	benchmark;
int		rdebug;
extern char	*outputfile;		/* output file name */
extern char	*framebuffer;
extern int	desiredframe;
extern int	finalframe;

extern struct rt_g	rt_g;

extern struct command_tab cmd_tab[];	/* given at end */

char	file_basename[128];	/* contains last component of file name */
char	file_fullname[128];	/* contains full file name */
char	object_list[512];	/* contains list of "MGED" objects */

FILE	*helper_fp;		/* pipe to rexec helper process */
char	ourname[512];

int	tcp_listen_fd;
extern int	pkg_permport;	/* libpkg/pkg_permserver() listen port */

int		rem_debug;		/* dispatcher debugging flag */

extern int	version[];	/* From vers.c */

/*
 *			T V S U B
 */
void
tvsub(tdiff, t1, t0)
struct timeval *tdiff, *t1, *t0;
{

	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

/*
 *			T V D I F F
 *
 *  Return t1 - t0, as a floating-point number of seconds.
 */
double
tvdiff(t1, t0)
struct timeval	*t1, *t0;
{
	return( (t1->tv_sec - t0->tv_sec) +
		(t1->tv_usec - t0->tv_usec) / 1000000. );
}

/*
 *			S T A M P
 *
 *  Return a string suitable for use as a timestamp.
 *  Mostly for stamping log messages with.
 */
char *
stamp()
{
	static char	buf[128];
	long		now;
	struct tm	*tmp;
	register char	*cp;

	(void)time( &now );
	tmp = localtime( &now );
	sprintf( buf, "%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
		tmp->tm_mon+1, tmp->tm_mday,
		tmp->tm_hour, tmp->tm_min, tmp->tm_sec );

	return(buf);
}

/*
 *			S T A T E _ T O _ S T R I N G
 *
 *  Return a pointer to a string, generally less than 8 bytes,
 *  that describes this state.
 */
char *
state_to_string( state )
int	state;
{
	static char	buf[128];

	switch( state )  {
	case SRST_NEW:
		return("..New..");
	case SRST_VERSOK:
		return("Vers_OK");
	case SRST_DOING_DIRBUILD:
		return("DirBuild");
	case SRST_READY:
		return(" Ready");
	case SRST_RESTART:
		return("Restart");
	case SRST_CLOSING:
		return("Closing");
	case SRST_DOING_GETTREES:
		return("GetTrees");
	}
	sprintf(buf, "UNKNOWN_x%x", state);
	return(buf);
}

/*
 *			M A I N
 */
int
main(argc, argv)
int	argc;
char	**argv;
{
	register struct servers *sp;

	/* Random inits */
	/*
	 * There is a problem in some hosts that gethostname() will only
	 * return the host name and *not* the fully qualified host name
	 * with domain name.
	 *
	 * gethostbyname() will return a host table (nameserver) entry
	 * where h_name is the "offical name", i.e. fully qualified.
	 * Therefore the following piece of code.
	 */
	{
		char temp[512];
		struct hostent *hp;
		gethostname(temp, sizeof(temp));
		hp = gethostbyname(temp);
		strcpy(ourname, hp->h_name);
	}
	fprintf(stderr,"%s %s %s\n", stamp(), ourname, version+5 );
	fflush(stderr);

	width = height = 512;			/* same size as RT */

	start_helper();

	FrameHead.fr_forw = FrameHead.fr_back = &FrameHead;
	for( sp = &servers[0]; sp < &servers[MAXSERVERS]; sp++ )  {
		sp->sr_work.li_forw = sp->sr_work.li_back = &sp->sr_work;
		sp->sr_pc = PKC_NULL;
		sp->sr_curframe = FRAME_NULL;
	}

	/* Listen for our PKG connections */
	if( (tcp_listen_fd = pkg_permserver("rtsrv", "tcp", 8, rt_log)) < 0 )  {
		int	i;
		char	num[8];
		/* Do it by the numbers */
		for(i=0; i<10; i++ )  {
			sprintf( num, "%d", 4446+i );
			if( (tcp_listen_fd = pkg_permserver(num, "tcp", 8, rt_log)) < 0 )
				continue;
			break;
		}
		if( i >= 10 )  {
			rt_log("Unable to find a port to listen on\n");
			exit(1);
		}
	}
	/* Now, pkg_permport has tcp port number */

	(void)signal( SIGPIPE, SIG_IGN );

	if( argc <= 1 )  {
		(void)signal( SIGINT, SIG_IGN );
		rt_log("%s Interactive REMRT on %s\n", stamp(), ourname );
		rt_log("%s Listening at port %d\n", stamp(), pkg_permport);
		clients = (1<<fileno(stdin));

		/* Read .remrtrc file to acquire server info */
		read_rc_file();

		/* Go until no more clients */
		while( clients )  {
			do_work(0);	/* no auto starting of servers */
		}
		/*
		 * Might want to see if any work remains, and if so,
		 * record it somewhere
		 */
		rt_log("%s Out of clients\n", stamp());
	} else {
		rt_log("%s Automatic REMRT on %s\n", stamp(), ourname );
		rt_log("%s Listening at port %d, reading script on stdin\n",
			stamp(), pkg_permport);
		clients = 0;

		/* parse command line args for sizes, etc */
		finalframe=-1;
		if( !get_args( argc, argv ) )  {
			fprintf(stderr,"remrt:  bad arg list\n");
			exit(1);
		}

		/* take note of database name and treetops */
		if( optind+2 > argc )  {
			fprintf(stderr,"remrt:  insufficient args\n");
			exit(2);
		}
		build_start_cmd( argc, argv, optind );

		/* Read .remrtrc file to acquire servers */
		read_rc_file();

		/* Collect up results of arg parsing */
		/* automatic: outputfile, width, height */
		if( framebuffer || outputfile == (char *)0 )  {
			init_fb(framebuffer);
		}

		/* Build queue of work to be done */
		if( !matflag )  {
			struct frame	*fr;
			char	buf[128];
			/* if not -M, queue off single az/el frame */
			GET_FRAME(fr);
			prep_frame(fr);
			sprintf(buf, "ae %g %g;", azimuth, elevation);
			rt_vls_strcat( &fr->fr_cmd, buf);
			if( create_outputfilename( fr ) < 0 )  {
				FREE_FRAME(fr);
			} else {
				APPEND_FRAME( fr, FrameHead.fr_back );
			}
		} else {
			/* if -M, read RT script from stdin */
			clients = 0;
			eat_script( stdin );
		}
		if(rem_debug>1) cd_frames( 0, (char **)0 );

		/* Compute until no work remains */
		running = 1;
		do_work(1);		/* auto start servers */
		rt_log("%s Task accomplished\n", stamp() );
	}
	return(0);			/* exit(0); */
}

/*
 *			D O _ W O R K
 */
do_work(auto_start)
int	auto_start;
{
	struct timeval	now;
	int		prev_serv;	/* previous # of connected servers */
	int		cur_serv;	/* current # of connected servers */
	register struct servers	*sp;

	/* Compute until no work remains */
	prev_serv = 0;

	if( FrameHead.fr_forw == &FrameHead )  {
		check_input( 30 );	/* delay up to 30 secs */
	}

	while( FrameHead.fr_forw != &FrameHead )  {
		if( auto_start )  {
			(void)gettimeofday( &now, (struct timezone *)0 );
			start_servers( &now );
		} else {
			if( clients == 0 )  break;
		}

		check_input( 30 );	/* delay up to 30 secs */

		(void)gettimeofday( &now, (struct timezone *)0 );
		schedule( &now );

		/* Count servers */
		cur_serv = 0;
		for( sp = &servers[0]; sp < &servers[MAXSERVERS]; sp++ )  {
			if( sp->sr_pc == PKC_NULL )  continue;
			cur_serv++;
		}
		if( cur_serv == 0 && prev_serv > cur_serv )  {
			rt_log("%s *** All servers down\n", stamp() );
			fflush(stdout);
		}
		prev_serv = cur_serv;
	}
}

/*
 *			R E A D _ R C _ F I L E
 *
 *  Read a .remrt file.  While this file can contain any valid commands,
 *  the intention is primarily to permit "automatic" registration of
 *  server hosts via "host" commands.
 */
void
read_rc_file()
{
	FILE	*fp;
	char	*home;
	char	path[128];

	if( (fp = fopen(".remrtrc", "r")) != NULL )  {
		source(fp);
		fclose(fp);
		return;
	}

	if( (home = getenv("HOME")) != NULL )  {
		sprintf( path, "%s/.remrtrc", home );
		if( (fp = fopen( path, "r" )) != NULL )  {
			source(fp);
			fclose(fp);
			return;
		}
	}

	if( (fp = fopen("/usr/brlcad/etc/.remrtrc", "r")) != NULL )  {
		source(fp);
		fclose(fp);
		return;
	}
}

/*
 *			C H E C K _ I N P U T
 */
void
check_input(waittime)
int waittime;
{
	static long	ibits;
	register int	i;
	register struct pkg_conn *pc;
	static struct timeval tv;
	register int	val;

	/* First, handle any packages waiting in internal buffers */
	for( i=0; i<NFD; i++ )  {
		pc = servers[i].sr_pc;
		if( pc == PKC_NULL )  continue;
		if( (val = pkg_process(pc)) < 0 )
			drop_server( &servers[i], "pkg_process() error" );
	}

	/* Second, hang in select() waiting for something to happen */
	tv.tv_sec = waittime;
	tv.tv_usec = 0;

	ibits = clients|(1<<tcp_listen_fd);
	val = select(32, (char *)&ibits, (char *)0, (char *)0, &tv);
	if( val < 0 )  {
		perror("select");
		return;
	}
	if( val==0 )  {
		/* At this point, ibits==0 */
		if(rem_debug>1) rt_log("%s select timed out after %d seconds\n", stamp(), waittime);
		return;
	}

	/* Third, accept any pending connections */
	if( ibits & (1<<tcp_listen_fd) )  {
		pc = pkg_getclient(tcp_listen_fd, pkgswitch, rt_log, 1);
		if( pc != PKC_NULL && pc != PKC_ERROR )
			addclient(pc);
		ibits &= ~(1<<tcp_listen_fd);
	}

	/* Fourth, get any new traffic off the network into libpkg buffers */
	for( i=0; i<NFD; i++ )  {
		register struct pkg_conn *pc;

		if( !feof(stdin) && i == fileno(stdin) )  continue;
		if( !(ibits&(1<<i)) )  continue;
		pc = servers[i].sr_pc;
		if( pc == PKC_NULL )  continue;
		val = pkg_suckin(pc);
		if( val < 0 ) {
			drop_server( &servers[i], "pkg_suckin() error" );
		} else if( val == 0 )  {
			drop_server( &servers[i], "EOF" );
		}
		ibits &= ~(1<<i);
	}

	/* Fifth, handle any new packages now waiting in internal buffers */
	for( i=0; i<NFD; i++ )  {
		pc = servers[i].sr_pc;
		if( pc == PKC_NULL )  continue;
		if( pkg_process(pc) < 0 )
			drop_server( &servers[i], "pkg_process() error" );
	}

	/* Finally, handle any command input (This can recurse via "read") */
	if( waittime>0 &&
	    !feof(stdin) &&
	    fileno(stdin) >= 0 &&
	    (ibits & (1<<(fileno(stdin)))) )  {
		interactive_cmd(stdin);
	}
}

/*
 *			A D D C L I E N T
 */
void
addclient(pc)
struct pkg_conn *pc;
{
	register struct servers *sp;
	register struct frame	*fr;
	struct ihost	*ihp;
	int		on = 1;
	auto int	fromlen;
	struct sockaddr_in from;
	int fd;

	fd = pc->pkc_fd;

	fromlen = sizeof (from);
	if (getpeername(fd, &from, &fromlen) < 0) {
		perror("getpeername");
		close(fd);
		return;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof (on)) < 0)
		perror("setsockopt (SO_KEEPALIVE)");

	if( (ihp = host_lookup_by_addr( &from, 1 )) == IHOST_NULL )  {
		/* Disaster */
		rt_log("abandoning this unknown server!!\n");
		close( fd );
		/* Maybe free the pkg struct? */
		return;
	}
	if( rem_debug )  rt_log("%s addclient(%s)\n", stamp(), ihp->ht_name);

	clients |= 1<<fd;

	sp = &servers[fd];
	bzero( (char *)sp, sizeof(*sp) );
	sp->sr_pc = pc;
	sp->sr_work.li_forw = sp->sr_work.li_back = &(sp->sr_work);
	sp->sr_state = SRST_NEW;
	sp->sr_curframe = FRAME_NULL;
	sp->sr_lump = 32;
	sp->sr_host = ihp;

	/* Clear any frame state that may remain from an earlier server */
	for( fr = FrameHead.fr_forw; fr != &FrameHead; fr = fr->fr_forw)  {
		CHECK_FRAME(fr);
	}
}

/*
 *			D R O P _ S E R V E R
 *
 *  Note that final connection closeout is handled in schedule(),
 *  to prevent recursion problems.
 */
void
drop_server(sp, why)
register struct servers	*sp;
char	*why;
{
	register struct list	*lhp, *lp;
	register struct pkg_conn *pc;
	register struct frame	*fr;
	int fd;
	int	oldstate;

	if( sp == SERVERS_NULL || sp->sr_host == IHOST_NULL )  {
		rt_log("drop_server(x%x), sr_host=0\n", sp);
		return;
	}
	oldstate = sp->sr_state;
	sp->sr_state = SRST_CLOSING;

	/* Only remark on servers that got out of "NEW" state.
	 * This keeps the one-shot commands from blathering here.
	 */
	if( oldstate != SRST_NEW )  {
		rt_log("%s dropping %s (%s)\n",
			 stamp(), sp->sr_host->ht_name, why);
	}

	pc = sp->sr_pc;
	if( pc == PKC_NULL )  {
		rt_log("drop_server(x%x), sr_pc=0\n", sp);
		return;
	}

	/* Clear the bits from "clients" now, to prevent further select()s */
	fd = pc->pkc_fd;
	if( fd <= 3 || fd >= NFD )  {
		rt_log("drop_server: fd=%d is unreasonable, forget it!\n", fd);
		return;
	}
	clients &= ~(1<<sp->sr_pc->pkc_fd);

	if( oldstate != SRST_READY )  return;

	/* Need to requeue any work that was in progress */
	lhp = &(sp->sr_work);
	while( (lp = lhp->li_forw) != lhp )  {
		fr = lp->li_frame;
		CHECK_FRAME(fr);
		DEQUEUE_LIST( lp );
		rt_log("%s requeueing fr%d %d..%d\n",
			stamp(),
			fr->fr_number,
			lp->li_start, lp->li_stop);
		APPEND_LIST( lp, &(fr->fr_todo) );
	}
}

/*
 *			S T A R T _ S E R V E R S
 *
 *  Scan the ihost table.  For all eligible hosts that don't
 *  presently have a server running, try to start a server.
 */
#define SERVER_CHECK_INTERVAL	(10*60)		/* seconds */
struct timeval	last_server_check_time;

void
start_servers( nowp )
struct timeval	*nowp;
{
	register struct ihost	*ihp;
	register struct servers	*sp;
	int	hackers_night;
	int	night;
	int	add;

	if( file_fullname[0] == '\0' )  return;

	if( tvdiff( nowp, &last_server_check_time ) < SERVER_CHECK_INTERVAL )
		return;

	/* Before seeking, note current (brief) status */
	cd_stat( 0, (char **)0 );

	rt_log("%s Seeking servers to start\n", stamp() );
	hackers_night = is_hackers_night( nowp );
	night = is_night( nowp );
	for( ihp = HostHead; ihp != IHOST_NULL; ihp = ihp->ht_next )  {

		/* Skip hosts which are not eligible for add/drop */
		add = 1;
		switch( ihp->ht_when )  {
		case HT_ALWAYS:
			break;
		case HT_NIGHT:
			if( night )
				add = 1;
			else
				add = 0;
			break;
		case HT_HACKNIGHT:
			if( hackers_night )
				add = 1;
			else
				add = 0;
			break;
		case HT_RS:
			if (ihp->ht_rs_wait-- <= 0) {
				add = 1;
			} else {
				add = 0;
			}
			break;
		default:
		case HT_PASSIVE:
		case HT_PASSRS:
			continue;
		}

		/* See if this host is already in contact as a server */
		for( sp = &servers[0]; sp < &servers[MAXSERVERS]; sp++ )  {
			if( sp->sr_pc == PKC_NULL )  continue;
			if( sp->sr_host != ihp )  continue;

			/* This host is a server */
			if( add == 0 )  {
				/* Drop this host -- out of time range */
				rt_log("%s Auto dropping %s:  out of time range\n",
					stamp(),
					ihp->ht_name );
				drop_server( sp, "outside time-of-day limits for this server" );
			} else {
				/* Host already serving, do nothing more */
			}
			goto next_host;
		}

		/* This host is not presently in contact */
		if( add && !(ihp->ht_flags & HT_HOLD))  {
			rt_log("%s Auto adding %s\n", stamp(), ihp->ht_name);
			add_host( ihp );
		}

next_host:	continue;
	}
	last_server_check_time = *nowp;		/* struct copy */
}

/*
 *			I S _ N I G H T
 *
 *  Indicate whether the given time is "night", ie, off-peak time.
 *  The simple algorithm used here does not take into account
 *  using machines in another time zone, nor is it nice to
 *  machines used by hackers who stay up late.
 */
int
is_night( tv )
struct timeval	*tv;
{
	struct tm	*tp;
	long		sec = tv->tv_sec;

	tp = localtime( &sec );

	/* Sunday(0) and Saturday(6) are "night" */
	if( tp->tm_wday == 0 || tp->tm_wday == 6 )  return(1);
	if( tp->tm_hour < 8 || tp->tm_hour >= 18 )  return(1);
	return(0);
}

/*
 *			I S _ H A C K E R S _ N I G H T
 *
 *  Indicate whether the given time is "night", ie, off-peak time,
 *  for a computer hacker, who stays up late.
 *  The simple algorithm used here does not take into account
 *  using machines in another time zone.
 */
int
is_hackers_night( tv )
struct timeval	*tv;
{
	struct tm	*tp;
	long		sec = tv->tv_sec;

	tp = localtime( &sec );

	/* Sunday(0) and Saturday(6) are "night" */
	if( tp->tm_wday == 0 || tp->tm_wday == 6 )  return(1);
	/* Hacking tends to run from 1300 to midnight, and on to 0400 */
	if( tp->tm_hour >= 4 && tp->tm_hour <= 12 )  return(1);
	return(0);
}

/*
 *			E A T _ S C R I P T
 *
 *  The general layout of an RT animation script is:
 *
 *	a once-only prelude that may give viewsize, etc.
 *
 *	the body between start & end commands
 *
 *	a trailer that follows the end command, before the next
 *	start command.  While this *might* include more changes
 *	of viewsize, etc, in actual practice, if it exists at all,
 *	it contains shell escapes, e.g., to compress the frame just
 *	finished.  As such, it should be performed locally, after
 *	the frame is done.
 *
 *  The caller is responsible for fopen()ing and fclose()ing the file.
 */
void
eat_script( fp )
FILE	*fp;
{
	char		*buf;
	char		*ebuf;
	char		*nsbuf;
	int		argc;
	char		*argv[64];
	struct rt_vls	prelude;
	struct rt_vls	body;
	struct rt_vls	finish;
	int		frame = 0;
	struct frame	*fr;

	rt_log("%s Starting to scan animation script\n", stamp() );

	rt_vls_init( &prelude );
	rt_vls_init( &body );
	rt_vls_init( &finish );

	/* Once only, collect up any prelude */
	while( (buf = rt_read_cmd( fp )) != (char *)0 )  {
		if( strncmp( buf, "start", 5 ) == 0 )  break;

		rt_vls_strcat( &prelude, buf );
		rt_vls_strcat( &prelude, ";" );
		rt_free( buf, "prelude line" );
	}
	if( buf == (char *)0 )  {
		rt_log("unexpected EOF while reading script for first frame 'start'\n");
		rt_vls_free( &prelude );
		return;
	}

	/* A "start" command has been seen, and is saved in buf[] */
	while( !feof(fp) )  {
		/* Gobble until "end" keyword seen */
		while( (ebuf = rt_read_cmd( fp )) != (char *)0 )  {
			if( strncmp( ebuf, "end", 3 ) == 0 )  {
				rt_free( ebuf, "end line" );
				break;
			}
			rt_vls_strcat( &body, ebuf );
			rt_vls_strcat( &body, ";" );
			rt_free( ebuf, "script body line" );
		}
		if( ebuf == (char *)0 )  {
			rt_log("unexpected EOF while reading script for frame %d\n", frame);
			break;
		}

		/* Gobble trailer until next "start" keyword seen */
		while( (nsbuf = rt_read_cmd( fp )) != (char *)0 )  {
			if( strncmp( nsbuf, "start", 5 ) == 0 )  {
				break;
			}
			rt_vls_strcat( &finish, nsbuf );
			rt_vls_strcat( &finish, ";" );
			rt_free( nsbuf, "script trailer line" );
		}

		/* buf[] has saved "start" line in it */
		argc = rt_split_cmd( argv, 64, buf );
		if( argc < 2 )  {
			rt_log("bad 'start' line\n");
			rt_free( buf, "bad start line" );
			goto out;
		}
		frame = atoi( argv[1] );
		if( frame < desiredframe )  {
			rt_vls_free( &body );
			goto bad;
		}
		if( finalframe >= 0 && frame > finalframe ) {
			rt_vls_free( &body );
			goto bad;
		}
		/* Might see if frame file exists 444 mode, then skip also */
		GET_FRAME(fr);
		fr->fr_number = frame;
		prep_frame(fr);
		rt_vls_vlscat( &fr->fr_cmd, &prelude );
		rt_vls_vlscatzap( &fr->fr_cmd, &body );
		rt_vls_vlscatzap( &fr->fr_after_cmd, &finish );
		if( create_outputfilename( fr ) < 0 )  {
			FREE_FRAME(fr);
		} else {
			APPEND_FRAME( fr, FrameHead.fr_back );
		}
bad:
		rt_free( buf, "command line" );
		buf = nsbuf;
		nsbuf = (char *)0;
	}
out:	
	rt_vls_free( &prelude );
	rt_vls_free( &body );
	rt_vls_free( &finish );

	/* For a few hundred frames, it all can take a little while */
	rt_log("%s Animation script loaded\n", stamp() );
}

/*
 *  			S T R I N G 2 I N T
 *  
 *  Convert a string to an integer.
 *  A leading "0x" implies HEX.
 *  If needed, octal might be done, but it seems unwise...
 *
 *  For general conversion, this is pretty feeble.  Is more needed?
 */
int
string2int(str)
register char *str;
{
	auto int ret;
	int cnt;

	ret = 0;
	if( str[0] == '0' && str[1] == 'x' )
		cnt = sscanf( str+2, "%x", &ret );
	else
		cnt = sscanf( str, "%d", &ret );
	if( cnt != 1 )
		rt_log("string2int(%s) = %d?\n", str, ret );
	return(ret);
}

/*
 *			G E T _ S E R V E R _ B Y _ N A M E
 */
struct servers *
get_server_by_name( str )
char *str;
{
	register struct servers *sp;
	struct ihost	*ihp;

	if( isdigit( *str ) )  {
		int	i;
		i = atoi( str );
		if( i < 0 || i >= MAXSERVERS )  return( SERVERS_NULL );
		return( &servers[i] );
	}

	if( (ihp = host_lookup_by_name( str, 0 )) == IHOST_NULL )
		return( SERVERS_NULL );

	for( sp = &servers[0]; sp < &servers[MAXSERVERS]; sp++ )  {
		if( sp->sr_pc == PKC_NULL )  continue;
		if( sp->sr_host == ihp )  return(sp);
	}
	return( SERVERS_NULL );
}

/*
 *			I N T E R A C T I V E _ C M D
 */
void
interactive_cmd(fp)
FILE *fp;
{
	char buf[BUFSIZ];
	register char *pos;
	register int i;

	pos = buf;

	/* Get first line */
	*pos = '\0';
	(void)fgets( pos, sizeof(buf), fp );
	i = strlen(buf);

	/* If continued, get more */
	while( pos[i-1]=='\n' && pos[i-2]=='\\' )  {
		pos += i-2;	/* zap NL and backslash */
		*pos = '\0';
		rt_log("-> "); (void)fflush(stderr);
		(void)fgets( pos, sizeof(buf)-strlen(buf), fp );
		i = strlen(pos);
	}

	if( feof(fp) ) {
		register struct servers *sp;

		if( fp != stdin )  return;

		/* Eof on stdin */
		clients &= ~(1<<fileno(fp));

		/* We might want to wait if something is running? */
		for( sp = &servers[0]; sp < &servers[MAXSERVERS]; sp++ )  {
			if( sp->sr_pc == PKC_NULL )  continue;
			if( !running )
				drop_server(sp, "EOF on Stdin" );
			else
				send_restart(sp);
		}
		/* The rest happens when the connections close */
		return;
	}
	if( (i=strlen(buf)) <= 0 )  return;

	/* Feeble allowance for comments */
	if( buf[0] == '#' )  return;

	(void)rt_do_cmd( (struct rt_i *)0, buf, cmd_tab );
}

/*
 *			P R E P _ F R A M E
 *
 * Fill in frame structure after reading MAT
 *  fr_number must have been set by caller.
 */
void
prep_frame(fr)
register struct frame *fr;
{
	register struct list *lp;
	char buf[BUFSIZ];

	CHECK_FRAME(fr);

	/* Get local buffer for image */
	fr->fr_width = width;
	fr->fr_height = height;

	rt_vls_trunc( &fr->fr_cmd, 0 );	/* Start fresh */
	sprintf(buf, "opt -w%d -n%d -H%d -p%g -U%d -J%x -A%g -l%d -E%g",
		fr->fr_width, fr->fr_height,
		hypersample, rt_perspective,
		use_air, jitter,
		AmbientIntensity, lightmodel,
		eye_backoff );
	rt_vls_strcat( &fr->fr_cmd, buf );
	if( interactive )  rt_vls_strcat( &fr->fr_cmd, " -I");
	if( benchmark )  rt_vls_strcat( &fr->fr_cmd, " -B");
	if( aspect != 1.0 )  {
		sprintf(buf, " -V%g", aspect);
		rt_vls_strcat( &fr->fr_cmd, buf );
	}
	rt_vls_strcat( &fr->fr_cmd, ";" );

	fr->fr_start.tv_sec = fr->fr_end.tv_sec = 0;
	fr->fr_start.tv_usec = fr->fr_end.tv_usec = 0;
	fr->fr_nrays = 0;
	fr->fr_cpu = 0.0;

	/* Build work list */
	fr->fr_todo.li_forw = fr->fr_todo.li_back = &(fr->fr_todo);
	GET_LIST(lp);
	lp->li_frame = fr;
	lp->li_start = 0;
	lp->li_stop = fr->fr_width*fr->fr_height-1;	/* last pixel # */
	APPEND_LIST( lp, fr->fr_todo.li_back );
}

/*
 *			D O _ A _ F R A M E
 */
void
do_a_frame()
{
	register struct frame *fr;
	if( running )  {
		rt_log("already running, please wait or STOP\n");
		return;
	}
	if( file_fullname[0] == '\0' )  {
		rt_log("need LOAD before GO\n");
		return;
	}
	if( (fr = FrameHead.fr_forw) == &FrameHead )  {
		rt_log("No frames to do!\n");
		return;
	}
	CHECK_FRAME(fr);
	running = 1;
}

/*
 *			C R E A T E _ O U T P U T F I L E N A M E
 *
 *  Build and save the file name.
 *  If the file will not be able to be written,
 *  signal error here.
 *
 *  Returns -
 *	-1	error, drop this frame
 *	 0	OK
 */
create_outputfilename( fr )
register struct frame	*fr;
{
	char	name[512];
	struct stat	sb;
	int		fd;

	CHECK_FRAME(fr);

	/* Always create a file name to write into */
	if( outputfile )  {
		sprintf( name, "%s.%d", outputfile, fr->fr_number );
		fr->fr_tempfile = 0;
	} else {
		sprintf( name, "remrt.pix.%d", fr->fr_number );
		fr->fr_tempfile = 1;
	}
	fr->fr_filename = rt_strdup( name );

	/*
	 *  There are several cases:
	 *	file does not exist, create it
	 *	file exists, is not writable -- skip this frame
	 *	file exists, is writable -- eliminate all non-black pixels
	 *		from work-to-do queue
	 */
	if( access( fr->fr_filename, 0 ) < 0 )  {
		/* File does not yet exist */
		if( (fd = creat( fr->fr_filename, 0644 )) < 0 )  {
			/* Unable to create new file */
			perror( fr->fr_filename );
			return( -1 );		/* skip this frame */
		}
		(void)close(fd);
		return(0);			/* OK */
	}
	/* The file exists */
	if( access( fr->fr_filename, 2 ) < 0 )  {
		/* File exists, and is not writable, skip this frame */
		perror( fr->fr_filename );
		return( -1 );			/* skip this frame */
	}
	/* The file exists and is writable */
	if( stat( fr->fr_filename, &sb ) >= 0 && sb.st_size > 0 )  {
		/* The file has existing contents, dequeue all non-black
		 * pixels.
		 */
		register FILE	*fp;
		char		pbuf[4];
		register int	pno;	/* index of next unread pixel */

		rt_log("%s Scanning %s for non-black pixels\n", stamp(),
			fr->fr_filename );
		if( (fp = fopen( fr->fr_filename, "r" )) == NULL )  {
			perror( fr->fr_filename );
			return(-1);
		}

		pno = 0;
		while( !feof( fp ) )  {
			register int	first, last;

			/* Read and skip over any black pixels */
			if( (int)fread( pbuf, 3, 1, fp ) < 1 )  break;
			pno++;
			if( pbuf[0] == 0 && pbuf[1] == 0 && pbuf[2] == 0 )
				continue;

			/*
			 *  Found a non-black pixel.
			 *  See how many more follow,
			 *  and delete the batch of them from the work queue.
			 */
			first = last = pno-1;

			while( !feof( fp ) )  {
				/* Read and skip over non-black pixels */
				if( (int)fread( pbuf, 3, 1, fp ) < 1 )  break;
				pno++;
				if( pbuf[0] == 0 && pbuf[1] == 0 && pbuf[2] == 0 )
					break;		/* black pixel */
				/* non-black */
				last = pno-1;
			}
			rt_log("%s Deleting non-black pixel range %d to %d inclusive\n",
				stamp(),
				first, last );
			list_remove( &(fr->fr_todo), first, last );
		}

		rt_log("%s Scanning complete\n", stamp() );
	}
	return(0);				/* OK */
}

/*
 *			F R A M E _ I S _ D O N E
 */
void
frame_is_done(fr)
register struct frame *fr;
{
	double	delta;

	CHECK_FRAME(fr);

	(void)gettimeofday( &fr->fr_end, (struct timezone *)0 );
	delta = tvdiff( &fr->fr_end, &fr->fr_start);
	if( delta < 0.0001 )  delta=0.0001;
	rt_log("%s Frame %d DONE: %g elapsed sec, %d rays/%g cpu sec\n",
		stamp(),
		fr->fr_number,
		delta,
		fr->fr_nrays,
		fr->fr_cpu );
	rt_log("%s  RTFM=%g rays/sec (%g rays/cpu sec)\n",
		stamp(),
		fr->fr_nrays/delta,
		fr->fr_nrays/fr->fr_cpu );

	/* Do any after-frame commands */
	if( rt_vls_strlen( &fr->fr_after_cmd ) > 0 )  {
		(void)rt_do_cmd( (struct rt_i *)0,
			RT_VLS_ADDR(&fr->fr_after_cmd), cmd_tab );
	}

	if( fr->fr_tempfile )  {
		/* Unlink temp file -- it is in framebuffer */
		if( unlink( fr->fr_filename ) < 0 )
			perror( fr->fr_filename );
	} else {
		/* Write-protect file, to prevent re-computation */
		if( chmod( fr->fr_filename, 0444 ) < 0 )
			perror( fr->fr_filename );
	}

	destroy_frame( fr );

	/* Run any end-of-frame script */
	if (frame_script) {
		char *cmd;
		cmd = malloc(strlen(frame_script) + strlen(fr->fr_filename) +
		    20); /* spaces and frame number */
		(void) sprintf(cmd,"%s %s %d",frame_script,fr->fr_filename,
		    fr->fr_number);
		if(rem_debug) rt_log("%s %s\n", stamp(), cmd);
		(void) system(cmd);
		(void) free(cmd);
	}
}

/*
 *			D E S T R O Y _ F R A M E
 */
void
destroy_frame( fr )
register struct frame	*fr;
{
	register struct list	*lp;

	CHECK_FRAME(fr);

	/*
	 *  Need to remove any pending work.
	 *  What about work already assigned that will dribble in?
	 */
	while( (lp = fr->fr_todo.li_forw) != &(fr->fr_todo) )  {
		DEQUEUE_LIST( lp );
		FREE_LIST(lp);
	}

	if( fr->fr_filename )  {
		rt_free( fr->fr_filename, "filename" );
		fr->fr_filename = (char *)0;
	}
	DEQUEUE_FRAME(fr);
	FREE_FRAME(fr);
}

/*
 *			T H I S _ F R A M E _ D O N E
 *
 *  All work on this frame is done when there is no more work to be sent out,
 *  and none of the servers have outstanding assignments for this frame.
 *
 *  Returns -
 *	 0	not done
 *	!0	all done
 */
int
this_frame_done( fr )
register struct frame	*fr;
{
	register struct servers	*sp;
	register struct list	*lhp, *lp;

	CHECK_FRAME(fr);

	if( fr->fr_todo.li_forw != &(fr->fr_todo) )
		return(1);		/* more work still to be sent */

	for( sp = &servers[0]; sp < &servers[MAXSERVERS]; sp++ )  {
		if( sp->sr_pc == PKC_NULL )  continue;
		lhp = &(sp->sr_work);
		for( lp = lhp->li_forw; lp != lhp; lp=lp->li_forw )  {
			if( fr != lp->li_frame )  continue;
			return(0);		/* nope, still more work */
		}
	}
	return(1);			/* All done */
}

/*
 *			A L L _ S E R V E R S _ I D L E
 *
 *  Returns -
 *	!0	All servers are idle
 *	 0	Some servers still busy
 */
int
all_servers_idle()
{
	register struct servers	*sp;

	for( sp = &servers[0]; sp < &servers[MAXSERVERS]; sp++ )  {
		if( sp->sr_pc == PKC_NULL )  continue;
		if( sp->sr_state != SRST_READY )  continue;
		if( sp->sr_work.li_forw == &(sp->sr_work) )  continue;
		return(0);		/* nope, still more work */
	}
	return(1);			/* All done */
}

/*
 *			A L L _ D O N E
 *
 *  All work is done when there is no more work to be sent out,
 *  and there is no more work pending in any of the servers.
 *
 *  Returns -
 *	 0	not done
 *	!0	all done
 */
int
all_done()
{
	register struct servers	*sp;
	register struct frame	*fr;

	for( fr = FrameHead.fr_forw; fr != &FrameHead; fr = fr->fr_forw)  {
		CHECK_FRAME(fr);
		if( fr->fr_todo.li_forw == &(fr->fr_todo) )
			continue;
		return(0);		/* nope, still more work */
	}

	if( all_servers_idle() )
		return(1);		/* All done */

	return(0);			/* nope, still more work */
}

/*
 *			S C H E D U L E
 *
 *  This routine is called by the main loop, after each batch of PKGs
 *  have arrived.
 *
 *  If there is work to do, and a free server, send work.
 *  One assignment will be given to each free server.  If there are
 *  servers that do not have a proper number of assignments (to ensure
 *  good pipelining), then additional passes will be made, untill all
 *  servers have a proper number of assignments.
 *
 *  There could be some difficulties with the linked lists changing
 *  while in an unsolicited pkg receive that happens when a pkg send
 *  operation is performed under here.  scheduler_going flag is
 *  insurance against unintended recursion, which should be enough.
 *
 *  When done, we leave the last finished frame around for attach/release.
 */
void
schedule( nowp )
struct timeval	*nowp;
{
	register struct servers *sp;
	register struct frame *fr;
	register struct frame *fr2;
	int		another_pass;
	static int	scheduler_going = 0;	/* recursion protection */
	int		ret;

	if( scheduler_going )  {
		/* recursion protection */
		return;
	}
	scheduler_going = 1;

	if( file_fullname[0] == '\0' )  goto out;

	/* Handle various state transitions */
	for( sp = &servers[0]; sp < &servers[MAXSERVERS]; sp++ )  {
		if( sp->sr_pc == PKC_NULL )  continue;

		switch( sp->sr_state )  {
		case SRST_VERSOK:
			/* advance this server to SRST_DOING_DIRBUILD */
			send_loglvl(sp);

			/* An error may have caused connection to drop */
			if( sp->sr_pc != PKC_NULL )
				send_dirbuild(sp);
			break;

		case SRST_CLOSING:
			/* Handle final closing */
			if(rem_debug>1) rt_log("%s Final close on %s\n", stamp(), sp->sr_host->ht_name);
			clients &= ~(1<<sp->sr_pc->pkc_fd);
			pkg_close(sp->sr_pc);

			sp->sr_pc = PKC_NULL;
			sp->sr_state = SRST_UNUSED;
			sp->sr_host = IHOST_NULL;

			break;
		}
	}

	/* Look for finished frames */
	fr = FrameHead.fr_forw;
	while( fr && fr != &FrameHead )  {
		CHECK_FRAME(fr);
		if( fr->fr_todo.li_forw != &(fr->fr_todo) )
			goto next_frame;	/* unassigned work remains */

		if( this_frame_done( fr ) )  {
			/* No servers are still working on this frame */
			fr2 = fr->fr_forw;
			frame_is_done( fr );	/* will dequeue */
			fr = fr2;
			continue;
		}
next_frame: ;
		fr = fr->fr_forw;
	}

	if( !running )  goto out;
	if( all_done() )  {
		running = 0;
		rt_log("%s All work done!\n", stamp() );
		if( detached )  exit(0);
		goto out;
	}

	/* Keep assigning work until all servers are fully loaded */
top:
	for( fr = FrameHead.fr_forw; fr != &FrameHead; fr = fr->fr_forw)  {
		CHECK_FRAME(fr);
		do {
			another_pass = 0;
			if( fr->fr_todo.li_forw == &(fr->fr_todo) )
				break;	/* none waiting here */

			/*
			 *  This loop attempts to make one assignment to
			 *  each of the workers, before looping back to
			 *  make additional assignments.
			 *  This should keep all workers evenly "stoked".
			 */
			for( sp = &servers[0]; sp < &servers[MAXSERVERS]; sp++ )  {
				if( sp->sr_pc == PKC_NULL )  continue;

				if( (ret = task_server(fr,sp,nowp)) < 0 )  {
					/* fr is no longer valid */
					goto top;
				} else if( ret > 0 )  {
					another_pass++;
				}
			}
		} while( another_pass > 0 );
	}
	/* No work remains to be assigned, or servers are stuffed full */
out:
	scheduler_going = 0;
	return;
}

#define MIN_ASSIGNMENT_TIME	5		/* desired seconds/result */
#define TARDY_SERVER_INTERVAL	(9*60)		/* max seconds of silence */
#define N_SERVER_ASSIGNMENTS	3		/* desired # of assignments */

/*
 *			N U M B E R _ O F _ R E A D Y _ S E R V E R S
 *
 *  Returns the number of servers that are ready (or busy) with
 *  computing frames.
 */
int
number_of_ready_servers()
{
	register struct servers	*sp;
	register int		count = 0;

	for( sp = &servers[0]; sp < &servers[MAXSERVERS]; sp++ )  {
		if( sp->sr_pc == PKC_NULL )  continue;
		if( sp->sr_state == SRST_READY ||
		    sp->sr_state == SRST_DOING_GETTREES )
			count++;
	}
	return  count;
}

/*
 *			A S S I G N M E N T _ T I M E
 *
 *  Determine how many seconds of work should be assigned to
 *  a worker, given the current configuration of workers.
 *  One overall goal is to keep the dispatcher (us) from
 *  having to process more than one response every two seconds,
 *  to ensure adequate processing power will be available to
 *  handle the LOG messages, starting new servers, writing
 *  results to the disk and/or framebuffer, etc.
 */
int
assignment_time()
{
	int	sec;

	sec = 2 * number_of_ready_servers();
	if( sec < MIN_ASSIGNMENT_TIME )
		return  MIN_ASSIGNMENT_TIME;
	return  sec;
}

/*
 *			T A S K _ S E R V E R
 *
 *  If this server is ready, and has fewer than N_SERVER_ASSIGNMENTS,
 *  dispatch one unit of work to it.
 *  The return code indicates if the server is sated or not.
 *
 *  Returns -
 *	-1	when 'fr' is no longer valid
 *	0	when this server winds up with a full workload
 *	1	when this server needs additional work
 */
int
task_server( fr, sp, nowp )
register struct servers	*sp;
register struct frame	*fr;
struct timeval		*nowp;
{
	register struct list	*lp;
	int			a, b;
	int			lump;

	if( sp->sr_pc == PKC_NULL )  return(0);

	if( sp->sr_state != SRST_READY )
		return(0);	/* not running yet */

	CHECK_FRAME(fr);

	/* Sanity check */
	if( fr->fr_filename == (char *)0 ||
	    fr->fr_filename[0] == '\0' )  {
		rt_log("task_server: fr %d: null filename!\n",
			fr->fr_number);
		destroy_frame( fr );	/* will dequeue */
		return(-1);		/* restart scan */
	}

	/*
	 *  Check for tardy server.
	 *  The assignments are estimated to take about MIN_ASSIGNMENT_TIME
	 *  seconds, so waiting many minutes is unreasonable.
	 *  However, if the picture "suddenly" became very complex,
	 *  or a system got very busy,
	 *  the estimate could be quite low.
	 *  This mechanism exists mostly to protect against servers that
	 *  go into "black hole" mode while REMRT is running unattended.
	 */
	if( server_q_len(sp) > 0 &&
	    sp->sr_sendtime.tv_sec > 0 && 
	    tvdiff( nowp, &sp->sr_sendtime ) > TARDY_SERVER_INTERVAL )  {
		rt_log("%s %s: *TARDY*\n", stamp(), sp->sr_host->ht_name);
		drop_server( sp, "tardy" );
		return(0);	/* not worth giving another assignment */
	}

	if( server_q_len(sp) >= N_SERVER_ASSIGNMENTS )
		return(0);	/* plenty busy */

	if( (lp = fr->fr_todo.li_forw) == &(fr->fr_todo) )  {
		/*  No more work to assign in this frame,
		 *  on next pass, caller should advance to next frame.
		 */
		return(1);	/* need more work */
	}

	/*
	 *  Send the server the necessary view information about this frame,
	 *  if the server is not currently working on this frame.
	 *  Note that the "next" frame may be a frame that the server
	 *  has worked on before (perhaps due to work requeued when
	 *  a tardy server was dropped), yet we still must re-send the
	 *  viewpoint.
	 */
	if( sp->sr_curframe != fr )  {
		sp->sr_curframe = fr;
		send_matrix( sp, fr );
		/* XXX should see if this is necessary... */
		send_gettrees( sp, fr );
		/* Now in state SRST_DOING_GETTREES */
		return(1);	/* need more work */
	}

	/* Special handling for the first assignment of each frame */
	if( fr->fr_start.tv_sec == 0 )  {
		/* Note when actual work on this frame was started */
		(void)gettimeofday( &fr->fr_start, (struct timezone *)0 );
	}

	/*
	 *  Make this assignment size based on weighted average of
	 *  past behavior.  Using pixels/elapsed_sec metric takes into
	 *  account:
	 *	remote processor speed
	 *	remote processor load
	 *	available network bandwidth & load
	 *	local processing delays
	 */
	/* Base new assignment on desired result rate & measured speed */
	lump = assignment_time() * sp->sr_w_elapsed;
	/* If for an interactive demo, limit assignment to 1 scanline */
	if( interactive && lump > fr->fr_width )  lump = fr->fr_width;
	/* Limit growth in assignment size to 2X each assignment */
	if( lump > 2*sp->sr_lump )  lump = 2*sp->sr_lump;
	/* Provide some bounds checking */
	if( lump < 32 )  lump = 32;
	else if( lump > REMRT_MAX_PIXELS )  lump = REMRT_MAX_PIXELS;
	sp->sr_lump = lump;

	a = lp->li_start;
	b = a+sp->sr_lump-1;	/* work increment */
	if( b >= lp->li_stop )  {
		b = lp->li_stop;
		DEQUEUE_LIST( lp );
		FREE_LIST( lp );
		lp = LIST_NULL;
	} else
		lp->li_start = b+1;

	/* Record newly allocated pixel range */
	GET_LIST(lp);
	lp->li_frame = fr;
	lp->li_start = a;
	lp->li_stop = b;
	APPEND_LIST( lp, sp->sr_work.li_back );
	send_do_lines( sp, a, b, fr->fr_number );

	/* See if server will need more assignments */
	if( server_q_len(sp) < N_SERVER_ASSIGNMENTS )
		return(1);
	return(0);
}

/*
 *			S E R V E R _ Q _ L E N
 *
 *  Report number of assignments that a server has
 */
int
server_q_len( sp )
register struct servers	*sp;
{
	register struct list	*lp;
	register int		count;

	count = 0;
	for( lp = sp->sr_work.li_forw; lp != &(sp->sr_work); lp = lp->li_forw )  {
		count++;
	}
	return(count);
}

/*
 *			R E A D _ M A T R I X
 *
 *  Read an old-style matrix.
 *
 *  Returns -
 *	 1	OK
 *	 0	EOF, no matrix read.
 *	-1	error, unable to read matrix.
 */
int
read_matrix( fp, fr )
register FILE *fp;
register struct frame *fr;
{
	register int i;
	char number[128];
	char	cmd[128];

	CHECK_FRAME(fr);

	/* Visible part is from -1 to +1 in view space */
	if( fscanf( fp, "%s", number ) != 1 )  goto eof;
	sprintf( cmd, "viewsize %s; eye_pt ", number );
	rt_vls_strcat( &(fr->fr_cmd), cmd );

	for( i=0; i<3; i++ )  {
		if( fscanf( fp, "%s", number ) != 1 )  goto out;
		sprintf( cmd, "%s ", number );
		rt_vls_strcat( &fr->fr_cmd, cmd );
	}

	sprintf( cmd, "; viewrot " );
	rt_vls_strcat( &fr->fr_cmd, cmd );

	for( i=0; i < 16; i++ )  {
		if( fscanf( fp, "%s", number ) != 1 )  goto out;
		sprintf( cmd, "%s ", number );
		rt_vls_strcat( &fr->fr_cmd, cmd );
	}
	rt_vls_strcat( &fr->fr_cmd, "; ");

	if( feof(fp) ) {
eof:
		rt_log("read_matrix: EOF on old style frame file.\n");
		return(-1);
	}
	return(0);			/* OK */
out:
	rt_log("read_matrix:  unable to parse old style entry\n");
	return(-1);
}

/*
 *			P H _ D E F A U L T
 */
void
ph_default(pc, buf)
register struct pkg_conn *pc;
char *buf;
{
	register int i;

	for( i=0; pc->pkc_switch[i].pks_handler != NULL; i++ )  {
		if( pc->pkc_switch[i].pks_type == pc->pkc_type )  break;
	}
	rt_log("ctl: unable to handle %s message: len %d",
		pc->pkc_switch[i].pks_title, pc->pkc_len);
	*buf = '*';
	(void)free(buf);
}

/*
 *			P H _ D I R B U I L D _ R E P L Y
 *
 *  The server answers our MSG_DIRBUILD with various prints, etc,
 *  and then responds with a MSG_DIRBUILD_REPLY in return, which indicates
 *  that he is ready to accept work now.
 */
void
ph_dirbuild_reply(pc, buf)
register struct pkg_conn *pc;
char *buf;
{
	register struct servers *sp;

	sp = &servers[pc->pkc_fd];
	rt_log("%s %s dirbuild OK (%s)\n",
		stamp(),
		sp->sr_host->ht_name,
		buf );
	if(buf) (void)free(buf);
	if( sp->sr_pc != pc )  {
		rt_log("unexpected MSG_DIRBUILD_REPLY from fd %d\n", pc->pkc_fd);
		return;
	}
	if( sp->sr_state != SRST_DOING_DIRBUILD )  {
		rt_log("MSG_DIRBUILD_REPLY in state %d?\n", sp->sr_state);
		drop_server( sp, "wrong state" );
		return;
	}
	sp->sr_state = SRST_READY;
}

/*
 *			P H _ G E T T R E E S _ R E P L Y
 *
 *  The server answers our MSG_GETTREES with various prints, etc,
 *  and then responds with a MSG_GETTREES_REPLY in return, which indicates
 *  that he is ready to accept work now.
 */
void
ph_gettrees_reply(pc, buf)
register struct pkg_conn *pc;
char *buf;
{
	register struct servers *sp;

	sp = &servers[pc->pkc_fd];
	rt_log("%s %s gettrees OK (%s)\n",
		stamp(),
		sp->sr_host->ht_name,
		buf );
	if(buf) (void)free(buf);
	if( sp->sr_pc != pc )  {
		rt_log("unexpected MSG_GETTREES_REPLY from fd %d\n", pc->pkc_fd);
		return;
	}
	if( sp->sr_state != SRST_DOING_GETTREES )  {
		rt_log("MSG_GETTREES_REPLY in state %s?\n",
			state_to_string(sp->sr_state) );
		drop_server( sp, "wrong state" );
		return;
	}
	sp->sr_state = SRST_READY;
}

/*
 *			P H _ P R I N T
 */
void
ph_print(pc, buf)
register struct pkg_conn *pc;
char *buf;
{
	if(print_on)  {
		rt_log("%s %s: %s",
			stamp(),
			servers[pc->pkc_fd].sr_host->ht_name,
			buf );
		if( buf[strlen(buf)-1] != '\n' )
			rt_log("\n");
	}
	if(buf) (void)free(buf);
}

/*
 *			P H _ V E R S I O N
 */
void
ph_version(pc, buf)
register struct pkg_conn *pc;
char	*buf;
{
	register struct servers	*sp;

	sp = &servers[pc->pkc_fd];
	if( strcmp( PROTOCOL_VERSION, buf ) != 0 )  {
		rt_log("ERROR %s: protocol version mis-match\n",
			sp->sr_host->ht_name);
		rt_log("  local='%s'\n", PROTOCOL_VERSION );
		rt_log(" remote='%s'\n", buf );
		drop_server( sp, "version mismatch" );
	} else {
		if( sp->sr_state != SRST_NEW )  {
			rt_log("NOTE %s:  VERSION message unexpected\n",
				sp->sr_host->ht_name);
		}
		sp->sr_state = SRST_VERSOK;
	}
	if(buf) (void)free(buf);
}

/*
 *			P H _ C M D
 */
void
ph_cmd(pc, buf)
register struct pkg_conn *pc;
char	*buf;
{
	register struct servers	*sp;

	sp = &servers[pc->pkc_fd];
	rt_log("%s %s: cmd '%s'\n", stamp(), sp->sr_host->ht_name, buf );
	(void)rt_do_cmd( (struct rt_i *)0, buf, cmd_tab );
	if(buf) (void)free(buf);
	drop_server( sp, "one-shot command" );
}

/*
 *			P H _ P I X E L S
 *
 *  When a scanline is received from a server, file it away.
 */
void
ph_pixels(pc, buf)
register struct pkg_conn *pc;
char *buf;
{
	register int		i;
	register struct servers	*sp;
	register struct frame	*fr;
	register struct list	*lp;
	struct line_info	info;
	struct timeval		tvnow;
	int			npix;
	int			fd;
	int			cnt;
	struct	rt_external	ext;

	(void)gettimeofday( &tvnow, (struct timezone *)0 );

	sp = &servers[pc->pkc_fd];
	if( sp->sr_state != SRST_READY &&
	    sp->sr_state != SRST_DOING_GETTREES )  {
		rt_log("%s Ignoring MSG_PIXELS from %s\n",
			stamp(), sp->sr_host->ht_name);
		goto out;
	}

	/*
	 *  If the elapsed time is less than MIN_ELAPSED_TIME, the package
	 *  was probably waiting in either the kernel's or libraries
	 *  input buffer.  Don't use these statistics.
	 */
#define MIN_ELAPSED_TIME	0.1
	if( (sp->sr_l_elapsed = tvdiff( &tvnow, &sp->sr_sendtime )) < MIN_ELAPSED_TIME )
		sp->sr_l_elapsed = MIN_ELAPSED_TIME;

	/* Consider the next assignment to have been sent "now" */
	(void)gettimeofday( &sp->sr_sendtime, (struct timezone *)0 );
	rt_struct_buf(&ext, (genptr_t) buf, pc->pkc_len);

	i = rt_struct_import( (genptr_t)&info, desc_line_info, &ext );
	if( i < 0 )  {
		rt_log("rt_struct_import error, %d\n", i);
		drop_server( sp, "rt_struct_import error" );
		goto out;
	}
	if( rem_debug )  {
		rt_log("%s %s %d/%d..%d, ray=%d, cpu=%.2g, el=%g\n",
			stamp(),
			sp->sr_host->ht_name,
			info.li_frame, info.li_startpix, info.li_endpix,
			info.li_nrays, info.li_cpusec, sp->sr_l_elapsed );
	}

	if( (lp = sp->sr_work.li_forw) == &(sp->sr_work) )  {
		rt_log("%s responded with pixels when none were assigned!\n",
			sp->sr_host->ht_name );
		drop_server( sp, "server responded, no assignment" );
		goto out;
	}

	/*
	 *  Here we require that the server return results in the
	 *  same order that they were assigned.
	 *  If the reply deviates in any way from the assignment,
	 *  then the server is dropped.
	 */
	fr = lp->li_frame;
	CHECK_FRAME(fr);

	if( info.li_frame != fr->fr_number )  {
		rt_log("%s: frame number mismatch, got=%d, assigned=%d\n",
			sp->sr_host->ht_name,
			info.li_frame, fr->fr_number );
		drop_server( sp, "frame number mismatch" );
		goto out;
	}
	if( info.li_startpix != lp->li_start ||
	    info.li_endpix != lp->li_stop )  {
		rt_log("%s:  assignment mismatch, sent %d..%d, got %d..%d\n",
			sp->sr_host->ht_name,
	    		lp->li_start, lp->li_stop,
	    		info.li_startpix, info.li_endpix );
	    	drop_server( sp, "pixel assignment mismatch");
	    	goto out;
	}

	if( info.li_startpix < 0 ||
	    info.li_endpix >= fr->fr_width*fr->fr_height )  {
		rt_log("pixel numbers out of range\n");
		drop_server( sp, "pixel out of range" );
		goto out;
	}

	/* Stash pixels in bottom-to-top .pix order */
	npix = info.li_endpix - info.li_startpix + 1;
	i = npix*3;
	if( pc->pkc_len - ext.ext_nbytes < i )  {
		rt_log("short scanline, s/b=%d, was=%d\n",
			i, pc->pkc_len - ext.ext_nbytes );
		i = pc->pkc_len - ext.ext_nbytes;
		drop_server( sp, "short scanline" );
		goto out;
	}
	/* Write pixels into file */
	/* Later, can implement FD cache here */
	if( (fd = open( fr->fr_filename, 2 )) < 0 )  {
		perror( fr->fr_filename );
		/* The bad fd will trigger a write error, below */
	} else if( lseek( fd, info.li_startpix*3L, 0 ) < 0 )  {
		perror( fr->fr_filename );
		(void)close(fd);	/* prevent write */
		fd = -1;
		/* The bad fd will trigger a write error, below */
	}
	if( (cnt = write( fd, buf+ext.ext_nbytes, i )) != i )  {
		perror( fr->fr_filename );
		rt_log("write s/b %d, got %d\n", i, cnt );
		/*
		 *  Generally, a write error is caused by lack of disk space.
		 *  In any case, it is indicative of bad problems.
		 *  Stop assigning new work.
		 */
		/* XXX should re-queue this assignment */
		rt_log("%s disk write error, preparing graceful STOP\n", stamp() );
		cd_stop( 0, (char **)0 );

		/* Dropping the (innocent) server will requeue the work */
		drop_server( sp, "disk write error" );

		/* Return, as if nothing had happened. */
		(void)close(fd);
		goto out;
	}
	(void)close(fd);

	/* If display attached, also draw it */
	if( fbp != FBIO_NULL )  {
		write_fb( buf + ext.ext_nbytes, fr,
			info.li_startpix, info.li_endpix+1 );
	}

	/*
	 *  Stash the statistics that came back.
	 *  Only perform weighted averages if elapsed times are reasonable.
	 */
	fr->fr_nrays += info.li_nrays;
	fr->fr_cpu += info.li_cpusec;
	sp->sr_l_percent = info.li_percent;
	if( sp->sr_l_elapsed > MIN_ELAPSED_TIME )  {
		double	blend1;	/* fraction of historical value to use */
		double	blend2;	/* fraction of new value to use */

		if( sp->sr_l_elapsed > assignment_time() )  {
			/*
			 *  Took longer than expected, put more weight on
			 *  this sample, and less on the historical values.
			 */
			blend1 = 0.5;
		} else {
			/*
			 *  Took less time than expected, don't get excited.
			 *  Place more emphasis on historical values.
			 */
			blend1 = 0.8;
		}
		blend2 = 1 - blend1;

		sp->sr_l_el_rate = npix / sp->sr_l_elapsed;
		sp->sr_w_elapsed = blend1 * sp->sr_w_elapsed +
				blend2 * sp->sr_l_el_rate;
		sp->sr_w_rays = blend1 * sp->sr_w_rays +
				blend2 * (info.li_nrays/sp->sr_l_elapsed);
		sp->sr_l_cpu = info.li_cpusec;
		sp->sr_s_cpu += info.li_cpusec;
		sp->sr_s_elapsed += sp->sr_l_el_rate;
		sp->sr_sq_elapsed += sp->sr_l_el_rate * sp->sr_l_el_rate;
		sp->sr_nsamp++;
	}

	/* Remove from work list */
	list_remove( &(sp->sr_work), info.li_startpix, info.li_endpix );

/*
 * Check to see if this host is load limited.  If the host is loaded
 * limited check to see if it is time to drop this server.
 */
	if (sp->sr_host->ht_when == HT_RS ||
	    sp->sr_host->ht_when == HT_PASSRS) {
		if(sp->sr_host->ht_rs >= info.li_nrays / sp->sr_l_elapsed) {
			if (++sp->sr_host->ht_rs_miss > 60 ) {
				sp->sr_host->ht_rs_miss = 0;
				sp->sr_host->ht_rs_wait = 3;
				drop_server(sp, "rays/second low");
			}
		} else {
			sp->sr_host->ht_rs_miss /= 2;
		}
	}
out:
	if(buf) (void)free(buf);
}

/*
 *			L I S T _ R E M O V E
 *
 * Given pointer to head of list of ranges, remove the range that's done
 */
void
list_remove( lhp, a, b )
register struct list *lhp;
int		a, b;
{
	register struct list *lp;

	for( lp=lhp->li_forw; lp != lhp; lp=lp->li_forw )  {
		if( lp->li_start == a )  {
			if( lp->li_stop == b )  {
				DEQUEUE_LIST(lp);
				FREE_LIST(lp);
				return;
			}
			lp->li_start = b+1;
			return;
		}
		if( lp->li_stop == b )  {
			lp->li_stop = a-1;
			return;
		}
		if( a > lp->li_stop )  continue;
		if( b < lp->li_start ) continue;
		/* Need to split range into two ranges */
		/* (start..a-1) and (b+1..stop) */
		{
			register struct list *lp2;
			rt_log("splitting range into (%d %d) (%d %d)\n",
				lp->li_start, a-1,
				b+1, lp->li_stop);
			GET_LIST(lp2);
			*lp2 = *lp;	/* struct copy li_frame, etc */
			lp2->li_start = b+1;
			lp2->li_stop = lp->li_stop;
			lp->li_stop = a-1;
			APPEND_LIST( lp2, lp );
			return;
		}
	}
}

/*
 *			W R I T E _ F B
 *
 *  Buffer 'pp' contains pixels numbered 'a' through (not including) 'b'.
 *  Write them out, clipping them to the current screen.
 */
void
write_fb( pp, fr, a, b )
unsigned char 	*pp;
struct frame	*fr;
int		a;
int		b;
{
	register int	x, y;	/* screen coordinates of pixel 'a' */
	int	offset;		/* pixel offset beyond 'pp' */
	int	pixels_todo;	/* # of pixels in buffer to be written */
	int	write_len;	/* # of pixels to write on this scanline */
	int	len_to_eol;	/* # of pixels from 'a' to end of scanline */

	CHECK_FRAME(fr);

	size_display(fr);

	x = a % fr->fr_width;
	y = (a / fr->fr_width) % fr->fr_height;
	pixels_todo = b - a;

	/* Simple case -- use multiple scanline writes */
	if( fr->fr_width == fb_getwidth(fbp) )  {
		fb_write( fbp, x, y,
			pp, pixels_todo );
		return;
	}

	/*
	 *  Hard case -- clip drawn region to the framebuffer.
	 *  The image may be larger than the framebuffer, in which
	 *  case the excess is discarded.
	 *  The image may be smaller than the actual framebuffer size,
	 *  in which case libfb is probably providing zooming.
	 */
	offset = 0;
	while( pixels_todo > 0 )  {
		if( fr->fr_width < fb_getwidth(fbp) )  {
			/* zoomed case */
			write_len = fr->fr_width - x;
		} else {
			/* clipping case */
			write_len = fb_getwidth(fbp) - x;
		}
		len_to_eol = fr->fr_width - x;
		if( write_len > pixels_todo )  write_len = pixels_todo;
		if( write_len > 0 )
			fb_write( fbp, x, y, pp+offset, write_len );
		offset += len_to_eol;
		y = (y+1) % fr->fr_height;
		x = 0;
		pixels_todo -= len_to_eol;
	}
}

/*
 *		R E P A I N T _ F B
 *
 *  Repaint the frame buffer from the stored file.
 *  Sort of a cheap "pix-fb", built in.
 */
void
repaint_fb( fr )
register struct frame *fr;
{
	register int	y;
	unsigned char	*line;
	int		nby;
	FILE		*fp;
	int		w;
	int		cnt;

	if( fbp == FBIO_NULL ) return;
	CHECK_FRAME(fr);
	size_display(fr);

	if( fr->fr_filename == (char *)0 )  return;

	/* Draw the accumulated image */
	nby = 3*fr->fr_width;
	line = (unsigned char *)rt_malloc( nby, "scanline" );
	if( (fp = fopen( fr->fr_filename, "r" )) == NULL )  {
		perror( fr->fr_filename );
		rt_free( (char *)line, "scanline" );
		return;
	}
	w = fr->fr_width;
	if( w > fb_getwidth(fbp) )  w = fb_getwidth(fbp);

	for( y=0; y < fr->fr_height; y++ )  {
		cnt = fread( (char *)line, nby, 1, fp );
		/* Write out even partial results, then quit */
		fb_write( fbp, 0, y, line, w );
		if( cnt != 1 )  break;
	}
	rt_free( (char *)line, "scanline" );
}

/*
 *			I N I T _ F B
 */
int
init_fb(name)
char *name;
{
	register int xx, yy;

	if( fbp != FBIO_NULL )  fb_close(fbp);

	xx = fbwidth;
	yy = fbheight;
	if( xx <= 0 )
		xx = width;
	if( yy <= 0 )
		yy = height;

	while( xx < width )
		xx <<= 1;
	while( yy < height )
		yy <<= 1;
	if( (fbp = fb_open( name?name:framebuffer, xx, yy )) == FBIO_NULL )  {
		rt_log("fb_open %d,%d failed\n", xx, yy);
		return(-1);
	}
	/* New way:  center, zoom */
	fb_view( fbp, xx/2, yy/2,
		fb_getwidth(fbp)/xx, fb_getheight(fbp)/yy );

	cur_fbwidth = 0;
	return(0);
}

/*
 *			S I Z E _ D I S P L A Y
 */
void
size_display(fr)
register struct frame	*fr;
{
	CHECK_FRAME(fr);
	if( cur_fbwidth == fr->fr_width )
		return;
	if( fbp == FBIO_NULL )
		return;
	if( fr->fr_width > fb_getwidth(fbp) )  {
		rt_log("Warning:  fb not big enough for %d pixels, display truncated\n", fr->fr_width );
		cur_fbwidth = fr->fr_width;
		return;
	}
	cur_fbwidth = fr->fr_width;

	fb_zoom( fbp, fb_getwidth(fbp)/fr->fr_width,
		fb_getheight(fbp)/fr->fr_height );
	/* center the view */
	fb_window( fbp, fr->fr_width/2, fr->fr_height/2 );
}

/*
 *			S E N D _ D I R B U I L D
 */
void
send_dirbuild(sp)
register struct servers *sp;
{
	register struct ihost	*ihp;
	char	cmd[512];

	if( sp->sr_pc == PKC_NULL )  return;
	if( file_fullname[0] == '\0' || sp->sr_state != SRST_VERSOK )  return;

	ihp = sp->sr_host;
	switch( ihp->ht_where )  {
	case HT_CD:
		if( pkg_send( MSG_CD, ihp->ht_path, strlen(ihp->ht_path)+1, sp->sr_pc ) < 0 )
			drop_server(sp, "MSG_CD send error");
		break;
	case HT_CONVERT:
		/* Conversion into current dir was done when server was started */
		break;
	default:
		rt_log("send_dirbuild: ht_where=%d unimplemented\n", ihp->ht_where);
		drop_server(sp, "bad ht_where");
		return;
	}

	if( pkg_send( MSG_DIRBUILD, file_basename, strlen(file_basename)+1,
	    sp->sr_pc ) < 0
	)  {
		drop_server(sp, "MSG_DIRBUILD pkg_send error");
		return;
	}
	sp->sr_state = SRST_DOING_DIRBUILD;
}

/*
 *			S E N D _ R E S T A R T
 */
void
send_restart(sp)
register struct servers *sp;
{
	if( sp->sr_pc == PKC_NULL )  return;

	if( pkg_send( MSG_RESTART, "", 0, sp->sr_pc ) < 0 )
		drop_server(sp, "MSG_RESTART pkg_send error");
	sp->sr_state = SRST_RESTART;
}

/*
 *			S E N D _ L O G L V L
 */
void
send_loglvl(sp)
register struct servers *sp;
{
	if( sp->sr_pc == PKC_NULL )  return;

	if( pkg_send( MSG_LOGLVL, print_on?"1":"0", 2, sp->sr_pc ) < 0 )
		drop_server(sp, "MSG_LOGLVL pkg_send error");
}

/*
 *			S E N D _ M A T R I X
 *
 *  Send current options, and the view matrix information.
 */
void
send_matrix(sp, fr)
struct servers *sp;
register struct frame *fr;
{
	CHECK_FRAME(fr);
	if( sp->sr_pc == PKC_NULL )  return;
	if( pkg_send( MSG_MATRIX,
	    RT_VLS_ADDR(&fr->fr_cmd), rt_vls_strlen(&fr->fr_cmd)+1, sp->sr_pc
	    ) < 0 )
		drop_server(sp, "MSG_MATRIX pkg_send error");
}

/*
 *			S E N D _ G E T T R E E S
 *
 *  Send args for rt_gettrees.
 */
void
send_gettrees(sp, fr)
struct servers		*sp;
register struct frame	*fr;
{
	CHECK_FRAME(fr);
	if( sp->sr_pc == PKC_NULL )  return;
	if( pkg_send( MSG_GETTREES,
	    object_list, strlen(object_list)+1, sp->sr_pc
	    ) < 0 )  {
		drop_server(sp, "MSG_GETTREES pkg_send error");
		return;
	}
	sp->sr_state = SRST_DOING_GETTREES;
}

/*
 *			S E N D _ D O _ L I N E S
 */
void
send_do_lines( sp, start, stop, framenum )
register struct servers *sp;
int		start;
int		stop;
int		framenum;
{
	char obuf[128];

	if( sp->sr_pc == PKC_NULL )  return;

	sprintf( obuf, "%d %d %d", start, stop, framenum );
	if( pkg_send( MSG_LINES, obuf, strlen(obuf)+1, sp->sr_pc ) < 0 )
		drop_server(sp, "MSG_LINES pkg_send error");

	(void)gettimeofday( &sp->sr_sendtime, (struct timezone *)0 );
}

/*
 *			P R _ L I S T
 */
void
pr_list( lhp )
register struct list *lhp;
{
	register struct list *lp;

	for( lp = lhp->li_forw; lp != lhp; lp = lp->li_forw  )  {
		if( lp->li_frame == 0 )  {
			rt_log("\t%d..%d frame *NULL*??\n",
			lp->li_start, lp->li_stop );
		} else {
			rt_log("\t%d..%d frame %d\n",
				lp->li_start, lp->li_stop,
				lp->li_frame->fr_number );
		}
	}
}

void
mathtab_constant()
{
	/* Called on -B (benchmark) flag, by get_args() */
}

/*
 *			A D D _ H O S T
 *
 *  There are two message formats:
 *	HT_CD		host, port, rem_dir
 *	HT_CONVERT	host, port, rem_dir, loc_db, rem_db
 */
void
add_host( ihp )
struct ihost	*ihp;
{
	if (ihp->ht_flags & HT_HOLD) return;	/* Not allowed to use */
	/* Send message to helper process */
	switch( ihp->ht_where )  {
	case HT_CD:
		fprintf( helper_fp,
			"%s %d %s\n",
			ihp->ht_name, pkg_permport, ihp->ht_path );
		break;
	case HT_CONVERT:
		if( file_fullname[0] == '\0' )  {
			rt_log("unable to add CONVERT host %s until database given\n",
				ihp->ht_name);
			return;
		}
		fprintf( helper_fp,
			"%s %d %s %s %s\n",
			ihp->ht_name, pkg_permport, ihp->ht_path,
			file_fullname, file_basename );
		break;
	default:
		rt_log("add_host:  ht_where=%d?\n", ihp->ht_where );
		break;
	}
	fflush( helper_fp );

	/* Wait briefly to try and catch the incomming connection,
	 * in case there are several of these spawned in a row.
	 */
	check_input(1);
}

#ifdef sgi
#	define RSH	"/usr/bsd/rsh"
#endif
#if CRAY || m68k
/*	m68k: Need this for MAC II under AUX as well */
#	define RSH	"/usr/bin/remsh"
#endif
#ifndef RSH
#	define RSH	"/usr/ucb/rsh"
#endif

/*
 *			H O S T _ H E L P E R
 *
 *  This loop runs in the child process of the real REMRT, and is directed
 *  to initiate contact with new hosts via a one-way pipe from the parent.
 *  In some cases, starting RTSRV on the indicated host is sufficient;
 *  in other cases, the portable version of the database needs to be
 *  sent first.
 *
 *  For now, a limitation is that the local and remote databases are
 *  given the same name.  If relative path names are used, this should
 *  not be a problem, but this could be changed later.
 */
void
host_helper(fp)
FILE	*fp;
{
	char	line[512];
	char	cmd[128];
	char	host[128];
	char	loc_db[128];
	char	rem_db[128];
	char	rem_dir[128];
	int	port;
	int	cnt;
	int	i;
	int	pid;

	while(1)  {
		line[0] = '\0';
		(void)fgets( line, sizeof(line), fp );
		if( feof(fp) )  break;

		loc_db[0] = '\0';
		rem_db[0] = '\0';
		rem_dir[0] = '\0';
		cnt = sscanf( line, "%s %d %s %s %s",
			host, &port, rem_dir, loc_db, rem_db );
		if( cnt != 3 && cnt != 5 )  {
			rt_log("host_helper: cnt=%d, aborting\n", cnt);
			break;
		}

		if( cnt == 3 )  {
			sprintf(cmd,
				"cd %s; rtsrv %s %d",
				rem_dir, ourname, port );
			if(rem_debug)  {
				rt_log("%s %s\n", stamp(), cmd);
				fflush(stdout);
			}

			pid = fork();
			if( pid == 0 )  {
				/* 1st level child */
				(void)close(0);
				for(i=3; i<40; i++)  (void)close(i);
				if( vfork() == 0 )  {
					/* worker Child */

					/* First, try direct exec. */
					execl(
						RSH,
						"rsh", host,
						"-n", cmd, 0 );

					/* Second, try $PATH exec */
					execlp(
						"rsh",
						"rsh", host,
						"-n", cmd, 0 );
					perror("rsh execl");
					_exit(0);
				}
				_exit(0);
			} else if( pid < 0 ) {
				perror("fork");
			} else {
				(void)wait(0);
			}
		} else {
			sprintf(cmd,
			 "g2asc<%s|%s %s \"cd %s; asc2g>%s; rtsrv %s %d\"",
				loc_db,
				RSH, host,
				rem_dir, rem_db,
				ourname, port );
			if(rem_debug)  {
				rt_log("%s %s\n", stamp(), cmd);
				fflush(stdout);
			}

			pid = fork();
			if( pid == 0 )  {
				/* 1st level child */
				(void)close(0);
				for(i=3; i<40; i++)  (void)close(i);

				if( vfork() == 0 )  {
					/* worker Child */
					execl("/bin/sh", "remrt_sh", 
						"-c", cmd, 0);
					perror("/bin/sh");
					_exit(0);
				}
				_exit(0);
			} else if( pid < 0 ) {
				perror("fork");
			} else {
				(void)wait(0);
			}
		}
	}
}

/*
 *			S T A R T _ H E L P E R
 */
void
start_helper()
{
	int	fds[2];
	int	pid;

	if( pipe(fds) < 0 )  {
		perror("pipe");
		exit(1);
	}

	pid = fork();
	if( pid == 0 )  {
		/* Child process */
		FILE	*fp;

		(void)close(fds[1]);
		if( (fp = fdopen( fds[0], "r" )) == (FILE *)NULL )  {
			perror("fdopen");
			exit(3);
		}
		host_helper( fp );
		/* No more commands from parent */
		exit(0);
	} else if( pid < 0 )  {
		perror("fork");
		exit(2);
	}
	/* Parent process */
	if( (helper_fp = fdopen( fds[1], "w" )) == (FILE *)NULL )  {
		perror("fdopen");
		exit(4);
	}
	(void)close(fds[0]);
}

/*
 *			B U I L D _ S T A R T _ C M D
 */
void
build_start_cmd( argc, argv, startc )
int	argc;
char	**argv;
int	startc;
{
	register char	*cp;
	register int	i;
	int		len;

	if( startc+2 > argc )  {
		rt_log("build_start_cmd:  need file and at least one object\n");
		file_fullname[0] = '\0';
		return;
	}

	strncpy( file_fullname, argv[startc], sizeof(file_fullname) );

	/* Save last component of file name */
	if( (cp = strrchr( argv[startc], '/' )) != (char *)0 )  {
		(void)strncpy( file_basename, cp+1, sizeof(file_basename) );
	} else {
		(void)strncpy( file_basename, argv[startc], sizeof(file_basename) );
	}

	/* Build new object_list[] string */
	cp = object_list;
	for( i=startc+1; i < argc; i++ )  {
		if( i > startc+1 )  *cp++ = ' ';
		len = strlen( argv[i] );
		bcopy( argv[i], cp, len );
		cp += len;
	}
	*cp++ = '\0';
}

/*
 *			H O S T _ L O O K U P _ B Y _ H O S T E N T
 */
struct ihost *
host_lookup_by_hostent( addr, enter )
struct hostent	*addr;
int		enter;
{
	register struct ihost	*ihp;

	/* Search list for existing instance */
	for( ihp = HostHead; ihp != IHOST_NULL; ihp = ihp->ht_next )  {
		if( strcmp( ihp->ht_name, addr->h_name ) != 0 )
			continue;
		return( ihp );
	}
	if( enter == 0 )
		return( IHOST_NULL );

	/* If not found and enter==1, enter in host table w/defaults */
	/* Note: gethostbyxxx() routines keep stuff in a static buffer */
	return( make_default_host( addr->h_name ) );
}

/*
 *			M A K E _ D E F A U L T _ H O S T
 *
 *  Add a new host entry to the list of known hosts, with
 *  default parameters.
 *  This routine is used to handle unexpected volunteers.
 */
struct ihost *
make_default_host( name )
char	*name;
{
	register struct ihost	*ihp;

	GETSTRUCT( ihp, ihost );

	/* Make private copy of host name -- callers have static buffers */
	ihp->ht_name = rt_strdup( name );

	/* Default host parameters */
	ihp->ht_flags = 0x0;
	ihp->ht_when = HT_PASSIVE;
	ihp->ht_where = HT_CONVERT;
	ihp->ht_path = "/tmp";

	/* Add to linked list of known hosts */
	ihp->ht_next = HostHead;
	HostHead = ihp;
	return(ihp);
}

/*
 *			H O S T _ L O O K U P _ B Y _ A D D R
 */
struct ihost *
host_lookup_by_addr( from, enter )
struct sockaddr_in	*from;
int	enter;
{
	register struct ihost	*ihp;
	struct hostent	*addr;
	unsigned long	addr_tmp;
	char		name[64];

#if CRAY && OLDTCP
	addr_tmp = from->sin_addr;
	addr = gethostbyaddr(&addr_tmp, sizeof (struct in_addr),
		from->sin_family);
#else
	addr_tmp = from->sin_addr.s_addr;
	addr = gethostbyaddr(&from->sin_addr, sizeof (struct in_addr),
		from->sin_family);
#endif
	if( addr != NULL )
		return( host_lookup_by_hostent( addr, enter ) );

	/* Host name is not known */
	addr_tmp = ntohl(addr_tmp);
	sprintf( name, "%d.%d.%d.%d",
		(addr_tmp>>24) & 0xff,
		(addr_tmp>>16) & 0xff,
		(addr_tmp>> 8) & 0xff,
		(addr_tmp    ) & 0xff );
	if( enter == 0 )  {
		rt_log("%s: unknown host\n");
		return( IHOST_NULL );
	}

	/* See if this host has been previously entered by number */
	for( ihp = HostHead; ihp != IHOST_NULL; ihp = ihp->ht_next )  {
		if( strcmp( ihp->ht_name, name ) == 0 )
			return( ihp );
	}

	/* Create a new hostent structure */
	return( make_default_host( name ) );
}

/*
 *			H O S T _ L O O K U P _ B Y _ N A M E
 */
struct ihost *
host_lookup_by_name( name, enter )
char	*name;
int	enter;
{
	struct sockaddr_in	sockhim;
	struct hostent		*addr;

	/* Determine name to be found */
	if( isdigit( *name ) )  {
		/* Numeric */
		sockhim.sin_family = AF_INET;
#if CRAY && OLDTCP
		sockhim.sin_addr = inet_addr(name);
#else
		sockhim.sin_addr.s_addr = inet_addr(name);
#endif
		return( host_lookup_by_addr( &sockhim, enter ) );
	} else {
		addr = gethostbyname(name);
	}
	if( addr == NULL )  {
		rt_log("%s:  bad host\n", name);
		return( IHOST_NULL );
	}
	return( host_lookup_by_hostent( addr, enter ) );
}

/*** Commands ***/

int
cd_load( argc, argv )
int	argc;
char	**argv;
{
	register struct servers *sp;

	if( running )  {
		rt_log("Can't load while running!!\n");
		return -1;
	}

	/* Really ought to reset here, too */
	if(file_fullname[0] != '\0' )  {
		rt_log("Was loaded with %s, restarting all\n", file_fullname);
		for( sp = &servers[0]; sp < &servers[MAXSERVERS]; sp++ )  {
			if( sp->sr_pc == PKC_NULL )  continue;
			send_restart( sp );
		}
	}

	build_start_cmd( argc, argv, 1 );
	return 0;
}

/*
 *			C D _ D E B U G
 *
 *  Set/toggle the local (dispatcher) debugging flag
 */
cd_debug( argc, argv )
int	argc;
char	**argv;
{
	if( argc <= 1 )  {
		rem_debug = !rem_debug;
	} else {
		sscanf( argv[1], "%x", &rem_debug );
	}
	rt_log("%s Dispatcher debug=x%x\n", stamp(), rem_debug );
	return 0;
}

/*
 *			C D _ R D E B U G
 *
 *  Send a string to the command processor on all the remote workers.
 *  Typically this would be of the form "opt -x42;"
 */
cd_rdebug( argc, argv )
int	argc;
char	**argv;
{
	register struct servers *sp;
	int		len;
	struct rt_vls	cmd;

	rt_vls_init( &cmd );
	rt_vls_strcpy( &cmd, "opt " );
	rt_vls_strcat( &cmd, argv[1] );
	len = rt_vls_strlen( &cmd )+1;
	for( sp = &servers[0]; sp < &servers[MAXSERVERS]; sp++ )  {
		if( sp->sr_pc == PKC_NULL )  continue;
		if( pkg_send( MSG_OPTIONS, RT_VLS_ADDR(&cmd), len, sp->sr_pc) < 0 )
			drop_server(sp, "MSG_OPTIONS pkg_send error");
	}
	return 0;
}

cd_f( argc, argv )
int	argc;
char	**argv;
{

	width = height = atoi( argv[1] );
	if( width < 4 || width > 16*1024 )
		width = 64;
	rt_log("width=%d, height=%d, takes effect after next MAT\n",
		width, height);
	return 0;
}

cd_S( argc, argv )
int	argc;
char	**argv;
{
	fbwidth = fbheight = atoi( argv[1] );
	if( fbwidth < 4 || fbwidth > 16*1024 )
		fbwidth = 512;
	if( fbheight < 4 || fbheight > 16*1024 )
		fbheight = 512;
	rt_log("fb width=%d, height=%d, takes effect after next attach\n",
		fbwidth, fbheight);
	return 0;
}

cd_N( argc, argv )
int	argc;
char	**argv;
{
	fbheight = atoi( argv[1] );
	if( fbheight < 4 || fbheight > 16*1024 )
		fbheight = 512;
	rt_log("fb height=%d, takes effect after next attach\n",
		fbheight);
	return 0;
}

cd_hyper( argc, argv )
int	argc;
char	**argv;
{
	hypersample = atoi( argv[1] );
	rt_log("hypersample=%d, takes effect after next MAT\n", hypersample);
	return 0;
}

cd_bench( argc, argv )
int	argc;
char	**argv;
{
	benchmark = atoi( argv[1] );
	rt_log("Benchmark flag=%d, takes effect after next MAT\n", benchmark);
	return 0;
}

cd_persp( argc, argv )
int	argc;
char	**argv;
{
	rt_perspective = atof( argv[1] );
	if( rt_perspective < 0.0 )  rt_perspective = 0.0;
	rt_log("perspective angle=%g, takes effect after next MAT\n", rt_perspective);
	return 0;
}

cd_read( argc, argv )
int	argc;
char	**argv;
{
	register FILE *fp;

	if( (fp = fopen(argv[1], "r")) == NULL )  {
		perror(argv[1]);
		return -1;
	}
	source(fp);
	fclose(fp);
	rt_log("%s 'read' command done\n", stamp());
	return 0;
}

source(fp)
FILE	*fp;
{
	while( !feof(fp) )  {
		/* do one command from file */
		interactive_cmd(fp);
		/* Without delay, see if anything came in */
		check_input(0);
	}
}

cd_detach( argc, argv )
int	argc;
char	**argv;
{
	detached = 1;
	clients &= ~(1<<0);	/* drop stdin */
	close(0);
	return 0;
}

cd_file( argc, argv )
int	argc;
char	**argv;
{
	if(outputfile)  rt_free(outputfile, "outputfile");
	outputfile = rt_strdup( argv[1] );
	return 0;
}

/*
 *			C D _ M A T
 *
 *  Read one specific matrix from an old-format eyepoint file.
 */
cd_mat( argc, argv )
int	argc;
char	**argv;
{
	register FILE *fp;
	register struct frame *fr;
	int	i;

	GET_FRAME(fr);
	if( argc >= 3 )  {
		fr->fr_number = atoi(argv[2]);
	} else {
		fr->fr_number = 0;
	}
	prep_frame(fr);

	if( (fp = fopen(argv[1], "r")) == NULL )  {
		perror(argv[1]);
		return -1;
	}

	/* Find the one desired frame */
	for( i=fr->fr_number; i>=0; i-- )  {
		if(read_matrix( fp, fr ) <= 0 )  {
			rt_log("mat: failure\n");
			fclose(fp);
			return -1;
		}
	}
	fclose(fp);

	if( create_outputfilename( fr ) < 0 )  {
		FREE_FRAME(fr);
	} else {
		APPEND_FRAME( fr, FrameHead.fr_back );
	}
	return 0;
}

cd_movie( argc, argv )
int	argc;
char	**argv;
{
	register FILE	*fp;
	register struct frame	*fr;
	struct frame		dummy_frame;
	int		a,b;
	int		i;

	/* movie mat a b */
	if( running )  {
		rt_log("already running, please wait\n");
		return -1;
	}
	if( file_fullname[0] == '\0' )  {
		rt_log("need LOAD before MOVIE\n");
		return -1;
	}
	a = atoi( argv[2] );
	b = atoi( argv[3] );
	if( (fp = fopen(argv[1], "r")) == NULL )  {
		perror(argv[1]);
		return -1;
	}
	/* Skip over unwanted beginning frames */
	for( i=0; i<a; i++ )  {
		if(read_matrix( fp, &dummy_frame ) <= 0 )  {
			rt_log("movie:  error in old style frame list\n");
			fclose(fp);
			return -1;
		}
	}
	for( i=a; i<b; i++ )  {
		int	ret;
		GET_FRAME(fr);
		fr->fr_number = i;
		prep_frame(fr);
		if( (ret=read_matrix( fp, fr )) < 0 )  {
			rt_log("movie:  frame %d bad\n", i);
			fclose(fp);
			return -1;
		}
		if( ret == 0 )  break;			/* EOF */
		if( create_outputfilename( fr ) < 0 )  {
			FREE_FRAME(fr);
		} else {
			APPEND_FRAME( fr, FrameHead.fr_back );
		}
	}
	fclose(fp);
	rt_log("Movie ready\n");
	return 0;
}

cd_add( argc, argv )
int	argc;
char	**argv;
{
	register int i;
	struct ihost	*ihp;

	for( i=1; i<argc; i++ )  {
		if( (ihp = host_lookup_by_name( argv[i], 0 )) != IHOST_NULL )  {
			add_host( ihp );
		}
	}
	return 0;
}

cd_drop( argc, argv )
int	argc;
char	**argv;
{
	register struct servers *sp;

	sp = get_server_by_name( argv[1] );
	if( sp == SERVERS_NULL || sp->sr_pc == PKC_NULL )  return -1;
	drop_server(sp, "drop command issued");
	return 0;
}

cd_hold( argc, argv )
int	argc;
char	**argv;
{
	register struct servers *sp;
	register struct ihost *ihp;

	ihp = host_lookup_by_name( argv[1], 0);
	if (ihp == IHOST_NULL) return -1;
	ihp->ht_flags |= HT_HOLD;

	sp = get_server_by_name( argv[1] );
	if ( sp == SERVERS_NULL || sp->sr_pc == PKC_NULL) return -1;
	drop_server(sp, "hold command issued");
	return 0;
}

cd_resume( argc, argv )
int	argc;
char	**argv;
{
	struct ihost	*ihp;

	ihp = host_lookup_by_name( argv[1], 0);
	if (ihp == IHOST_NULL ) return -1;
	ihp->ht_flags &= ~HT_HOLD;
	add_host( ihp );
	return 0;
}

cd_restart( argc, argv )
int	argc;
char	**argv;
{
	register struct servers *sp;

	if( argc <= 1 )  {
		/* Restart all */
		rt_log("%s Restarting all\n", stamp() );
		for( sp = &servers[0]; sp < &servers[MAXSERVERS]; sp++ )  {
			if( sp->sr_pc == PKC_NULL )  continue;
			send_restart( sp );
		}
		return -1;
	}
	sp = get_server_by_name( argv[1] );
	if( sp == SERVERS_NULL || sp->sr_pc == PKC_NULL )  return -1;
	send_restart( sp );
	/* The real action takes place when he closes the conn */
	return 0;
}

cd_stop( argc, argv )
int	argc;
char	**argv;
{
	rt_log("%s No more scanlines being scheduled, done soon\n", stamp() );
	running = 0;
	return 0;
}

cd_reset( argc, argv )
int	argc;
char	**argv;
{
	register struct frame *fr;

	if( running )  {
		rt_log("must STOP before RESET!\n");
		return -1;
	}
	do {
		fr = FrameHead.fr_forw;
		CHECK_FRAME(fr);
		destroy_frame( fr );
	} while( FrameHead.fr_forw != &FrameHead );
	return 0;
}

cd_attach( argc, argv )
int	argc;
char	**argv;
{
	register struct frame *fr;
	char		*name;

	if( argc <= 1 )  {
		name = (char *)0;
	} else {
		name = argv[1];
	}
	if( init_fb(name) < 0 )  return -1;
	if( fbp == FBIO_NULL ) return -1;
	if( (fr = FrameHead.fr_forw) == &FrameHead )  return -1;
	CHECK_FRAME(fr);

	repaint_fb( fr );
	return 0;
}

cd_release( argc, argv )
int	argc;
char	**argv;
{
	if(fbp != FBIO_NULL) fb_close(fbp);
	fbp = FBIO_NULL;
	return 0;
}


/*
 *			C D _ F R A M E S
 *
 *  Sumarize frames waiting
 *	Usage: frames [-v]
 */
cd_frames( argc, argv )
int	argc;
char	**argv;
{
	register struct frame *fr;
	register int	i;

	rt_log("%s Frames waiting:\n", stamp() );
	for(fr=FrameHead.fr_forw; fr != &FrameHead; fr=fr->fr_forw) {
		CHECK_FRAME(fr);
		rt_log("%5d\twidth=%d, height=%d\n",
			fr->fr_number,
			fr->fr_width, fr->fr_height );

		if( argc <= 1 )  continue;
		if( fr->fr_filename )  {
			rt_log("\tfile=%s%s\n",
				fr->fr_filename,
				fr->fr_tempfile ? " **TEMPORARY**" : "" );
		}
		rt_log("\tnrays = %d, cpu sec=%g\n", fr->fr_nrays, fr->fr_cpu);
		pr_list( &(fr->fr_todo) );
		rt_log("\tcmd=%s\n", RT_VLS_ADDR(&fr->fr_cmd) );
		rt_log("\tafter_cmd=%s\n", RT_VLS_ADDR(&fr->fr_after_cmd) );
	}
	return 0;
}

cd_memprint( argc, argv)
int	argc;
char	**argv;
{
	if (strcmp(argv[1],"on")==0) {
		rt_g.debug |= (DEBUG_MEM|DEBUG_MEM_FULL);
	} else if (strcmp(argv[1], "off") == 0) {
		rt_g.debug &= ~(DEBUG_MEM|DEBUG_MEM_FULL);
	} else {
		rt_prmem("memprint command");
	}
	return 0;
}

/*
 *			C D _ S T A T
 *
 *  Brief version
 */
cd_stat( argc, argv )
int	argc;
char	**argv;
{
	register struct servers *sp;
    	int	frame;
	char	*s;
	char	buf[48];
	char	*state;

	s = stamp();

	/* Print work assignments */
	if(interactive) rt_log("%s Interactive mode\n", s);
	rt_log("%s Worker assignment interval=%d seconds:\n",
		s, assignment_time() );
	rt_log("   Server   Last  Last   Average  Cur   Machine\n");
	rt_log("    State   Lump Elapsed pix/sec Frame   Name \n");
	rt_log("  -------- ----- ------- ------- ----- -------------\n");
	for( sp = &servers[0]; sp < &servers[MAXSERVERS]; sp++ )  {
		if( sp->sr_pc == PKC_NULL )  continue;

		/* Ignore one-shot command interfaces */
		if( sp->sr_state == SRST_NEW )  continue;

		if( sp->sr_curframe != FRAME_NULL )  {
			CHECK_FRAME(sp->sr_curframe);
			frame = sp->sr_curframe->fr_number;
		}  else  {
			frame = -1;
		}

		if( sp->sr_state != SRST_READY )  {
			state = state_to_string(sp->sr_state);
		}  else  {
			sprintf( buf, "Running%d",
				server_q_len(sp) );
			state = buf;
		}
		rt_log("  %8s %5d %7g %7g %5d %s\n",
			state,
			sp->sr_lump,
			sp->sr_l_elapsed,
			sp->sr_w_elapsed,
			frame,
			sp->sr_host->ht_name );
	}
	return 0;
}

/*
 *			C D _ S T A T U S
 *
 *  Full status version
 */
cd_status( argc, argv )
int	argc;
char	**argv;
{
	register struct servers *sp;
    	int	num;
	char	*s;

	s = stamp();

	if( file_fullname[0] == '\0' )  {
		rt_log("No model loaded yet\n");
	} else {
		rt_log("\n%s %s\n",
			s,
			running ? "RUNNING" : "Halted" );
		rt_log("%s %s objects=%s\n",
			s, file_fullname, object_list );
	}

	if( fbp != FBIO_NULL )
		rt_log("%s Framebuffer is %s\n", s, fbp->if_name);
	else
		rt_log("%s No framebuffer\n", s );
	if( outputfile )
		rt_log("%s Output file: %s.###\n", s, outputfile );
	rt_log("%s Printing of remote messages is %s\n",
		s, print_on?"ON":"Off" );
    	rt_log("%s Listening at %s, port %d\n",
		s, ourname, pkg_permport);

	/* Print work assignments */
	rt_log("%s Worker assignment interval=%d seconds:\n",
		s, assignment_time() );
	for( sp = &servers[0]; sp < &servers[MAXSERVERS]; sp++ )  {
		if( sp->sr_pc == PKC_NULL )  continue;
		rt_log("  %2d  %s %s",
			sp->sr_pc->pkc_fd, sp->sr_host->ht_name,
			state_to_string(sp->sr_state) );
		if( sp->sr_curframe != FRAME_NULL )  {
			CHECK_FRAME(sp->sr_curframe);
			rt_log(" frame %d, assignments=%d\n",
				sp->sr_curframe->fr_number,
				server_q_len(sp) );
		}  else  {
			rt_log("\n");
		}
		num = sp->sr_nsamp<=0 ? 1 : sp->sr_nsamp;
		rt_log("\tlast:  elapsed=%g, cpu=%g, lump=%d\n",
			sp->sr_l_elapsed,
			sp->sr_l_cpu,
			sp->sr_lump );
		rt_log("\t avg:  elapsed=%gp/s, cpu=%g, weighted=%gp/s\n",
			(sp->sr_s_elapsed/num),
			sp->sr_s_cpu/num,
			sp->sr_w_elapsed);
		rt_log("\t r/s:  weighted=%gr/s missed = %d\n",
			sp->sr_w_rays,
			sp->sr_host->ht_rs_miss );

		if( rem_debug )
			pr_list( &(sp->sr_work) );
	}
	return 0;
}

cd_clear( argc, argv )
int	argc;
char	**argv;
{
	if( fbp == FBIO_NULL )  return -1;
	fb_clear( fbp, PIXEL_NULL );
	cur_fbwidth = 0;
	return 0;
}

cd_print( argc, argv )
int	argc;
char	**argv;
{
	register struct servers *sp;

	if( argc > 1 )
		print_on = atoi(argv[1]);
	else
		print_on = !print_on;	/* toggle */

	for( sp = &servers[0]; sp < &servers[MAXSERVERS]; sp++ )  {
		if( sp->sr_pc == PKC_NULL )  continue;
		send_loglvl( sp );
	}
	rt_log("%s Printing of remote messages is %s\n",
		stamp(),
		print_on?"ON":"Off" );
	return 0;
}

cd_go( argc, argv )
int	argc;
char	**argv;
{
	do_a_frame();
	return 0;
}

cd_wait( argc, argv )
int	argc;
char	**argv;
{
	struct timeval	now;

	clients &= ~(1<<fileno(stdin));
	if( running )  {
		/*
		 *  When running, WAIT command waits for all
		 *  outstanding frames to be completed.
		 */
		while( clients && FrameHead.fr_forw != &FrameHead )  {
			check_input( 30 );	/* delay up to 30 secs */

			(void)gettimeofday( &now, (struct timezone *)0 );
			schedule( &now );
		}
	} else {
		/*
		 *  When stopped, WAIT command waits for all
		 *  servers to finish their assignments.
		 */
		while( !all_servers_idle() )  {
			rt_log("%s Stopped, waiting for servers to become idle\n", stamp() );
			check_input( 30 );	/* delay up to 30 secs */
		}
		rt_log("%s All servers idle\n", stamp() );
	}
	clients |= 1<<fileno(stdin);
	return 0;
}

cd_help( argc, argv )
int	argc;
char	**argv;
{
	register struct command_tab	*tp;

	for( tp = cmd_tab; tp->ct_cmd != (char *)0; tp++ )  {
		rt_log("%s %s\t\t%s\n",
			tp->ct_cmd, tp->ct_parms,
			tp->ct_comment );
	}
	return 0;
}

/*
 * host name always|night|passive cd|convert path
 */
cd_host( argc, argv )
int	argc;
char	**argv;
{
	register struct ihost	*ihp;
	int argpoint = 1;

	if( argc < 5 )  {
		rt_log("%s Registered Host Table:\n", stamp() );
		for( ihp = HostHead; ihp != IHOST_NULL; ihp=ihp->ht_next )  {
			rt_log("  %s 0x%x ", ihp->ht_name, ihp->ht_flags);
			switch(ihp->ht_when)  {
			case HT_ALWAYS:
				rt_log("always ");
				break;
			case HT_NIGHT:
				rt_log("night ");
				break;
			case HT_HACKNIGHT:
				rt_log("hacknight ");
				break;
			case HT_PASSIVE:
				rt_log("passive ");
				break;
			case HT_RS:
				rt_log("r/s %d ",ihp->ht_rs);
				break;
			case HT_PASSRS:
				rt_log("passive r/s %d ",ihp->ht_rs);
				break;
			default:
				rt_log("?when? ");
				break;
			}
			switch(ihp->ht_where)  {
			case HT_CD:
				rt_log("cd %s\n", ihp->ht_path);
				break;
			case HT_CONVERT:
				rt_log("convert %s\n", ihp->ht_path);
				break;
			default:
				rt_log("?where?\n");
				break;
			}
		}
		return 0;
	}

	if( (ihp = host_lookup_by_name( argv[argpoint++], 1 )) == IHOST_NULL )
		return -1;

	/* When */
	if( strcmp( argv[argpoint], "always" ) == 0 ) {
		ihp->ht_when = HT_ALWAYS;
	} else if( strcmp( argv[argpoint], "night" ) == 0 )  {
		ihp->ht_when = HT_NIGHT;
	} else if( strcmp( argv[argpoint], "hacknight" ) == 0 )  {
		ihp->ht_when = HT_HACKNIGHT;
	} else if( strcmp( argv[argpoint], "passive" ) == 0 )  {
		ihp->ht_when = HT_PASSIVE;
	} else if( strcmp( argv[argpoint], "rs" ) == 0 ) {
		ihp->ht_when = HT_RS;
	} else if ( strcmp( argv[argpoint], "passrs" ) == 0 ) {
		ihp->ht_when = HT_PASSRS;
	} else {
		rt_log("unknown 'when' string '%s'\n", argv[argpoint]);
	}
	++argpoint;
	if (ihp->ht_when == HT_RS ||
	    ihp->ht_when == HT_PASSRS) {
		ihp->ht_rs_miss = 0;
		ihp->ht_rs_wait = 0;
		ihp->ht_rs = 500;
		if ( isdigit(*argv[argpoint])) {
			ihp->ht_rs = atoi(argv[argpoint++]);
		}
	    }

	/* Where */
	if( strcmp( argv[argpoint], "cd" ) == 0 )  {
		ihp->ht_where = HT_CD;
		ihp->ht_path = rt_strdup( argv[argpoint+1] );
	} else if( strcmp( argv[argpoint], "convert" ) == 0 )  {
		ihp->ht_where = HT_CONVERT;
		ihp->ht_path = rt_strdup( argv[argpoint+1] );
	} else if (strcmp( argv[argpoint], "use" ) == 0 ) {
		ihp->ht_where = HT_USE;
		ihp->ht_path = rt_strdup( argv[argpoint+1]);
	} else {
		rt_log("unknown 'where' string '%s'\n", argv[argpoint] );
	}
	return 0;
}

/*
 *			C D _ E X I T
 */
cd_exit( argc, argv )
int	argc;
char	**argv;
{
	exit(0);
	return 0;
}

/* 		C D _ F R A M E
 *
 * Entry:
 *	argc	argument count
 *	argv	argument list
 *
 * Exit:
 *	frame_script is set to the shell script to execute.
 *
 */
cd_EOFrame( argc, argv )
int	argc;
char	**argv;
{
	if (frame_script) {
		(void) free(frame_script);
		frame_script = (char *)0;
	}

	if (strcmp(argv[1], "off") != 0 ) {
		frame_script = rt_strdup(argv[1]);
	}
	return 0;
}

struct command_tab cmd_tab[] = {
	"load",	"file obj(s)",	"specify database and treetops",
		cd_load,	3, 99,
	"read", "file",		"source a command file",
		cd_read,	2, 2,
	"file", "name",		"base name for storing frames",
		cd_file,	2, 2,
	"mat", "file [frame]",	"read one matrix from file",
		cd_mat,		2, 3,
	"movie", "file start_frame end_frame",	"read movie",
		cd_movie,	4, 4,
	"add", "host(s)",	"attach to hosts",
		cd_add,		2, 99,
	"drop", "host",		"drop first instance of 'host'",
		cd_drop,	2, 2,
	"hold", "host",		"Hold a host from processing",
		cd_hold,	2, 2,
	"resume","host",	"resume a host processing",
		cd_resume,	2, 2,
	"restart", "[host]",	"restart one or all hosts",
		cd_restart,	1, 2,
	"go", "",		"start scheduling frames",
		cd_go,		1, 1,
	"stop", "",		"stop scheduling work",
		cd_stop,	1, 1,
	"reset", "",		"purge frame list of all work",
		cd_reset,	1, 1,
	"frames", "[-v]",	"summarize waiting frames",
		cd_frames,	1, 2,
	"stat", "",		"brief worker status",
		cd_stat,	1, 1,
	"status", "",		"full worker status",
		cd_status,	1, 1,
	"detach", "",		"detatch from interactive keyboard",
		cd_detach,	1, 1,
	"host", "name always|night|passive|rs[ rays/sec]|passrs[ rays/sec] cd|convert path", "register server host",
		cd_host,	1, 6,
	"wait", "",		"wait for current work assignment to finish",
		cd_wait,	1, 1,
	"exit", "",		"terminate remrt",
		cd_exit,	1, 1,
	"EOFrame", "EOFrame command|'off'", "Run command/script on dispatcher at End Of Frame",
		cd_EOFrame,	2, 2,
	/* FRAME BUFFER */
	"attach", "[fb]",	"attach to frame buffer",
		cd_attach,	1, 2,
	"release", "",		"release frame buffer",
		cd_release,	1, 1,
	"clear", "",		"clear framebuffer",
		cd_clear,	1, 1,
	"S", "square_size",	"set square frame buffer size",
		cd_S,		2, 2,
	"N", "square_height",	"set height of frame buffer",
		cd_N,		2, 2,
	/* FLAGS */
	"debug", "[hex_flags]",	"set local debugging flag bits",
		cd_debug,	1, 2,
	"rdebug", "options",	"set remote debugging via 'opt' command",
		cd_rdebug,	2, 2,
	"f", "square_size",	"set square frame size",
		cd_f,		2, 2,
	"s", "square_size",	"set square frame size",
		cd_f,		2, 2,
	"-H", "hypersample",	"set number of hypersamples/pixel",
		cd_hyper,	2, 2,
	"-B", "0|1",		"set benchmark flag",
		cd_bench,	2, 2,
	"p", "angle",		"set perspective angle (degrees) 0=ortho",
		cd_persp,	2, 2,
	"print", "[0|1]",	"set/toggle remote message printing",
		cd_print,	1, 2,
	"memprint", "on|off|NULL",	"debug dump of memory usage",
		cd_memprint,	1, 2,
	/* HELP */
	"?", "",		"help",
		cd_help,	1, 1,
	(char *)0, (char *)0, (char *)0,
		0,		0, 0	/* END */
};

#ifdef CRAY2
gettimeofday( tvp, tzp )
struct timeval	*tvp;
struct timezone	*tzp;
{
	tvp->tv_sec = time( (long *)0 );
	tvp->tv_usec = 0;
}
#endif
/* XXX ---  This should be part of inout.c */

#define	RT_GETPUT_MAGIC_1	0x15cb
#define RT_GETPUT_MAGIC_2	0xbc51
int
rt_struct_buf( ext, buf )
struct rt_external *ext;
genptr_t buf;
{
	register long i, len;

	RT_INIT_EXTERNAL(ext);
	ext->ext_buf = buf;
	i = (((unsigned char *)(ext->ext_buf))[0] << 8) |
	     ((unsigned char *)(ext->ext_buf))[1];
	len = (((unsigned char *)(ext->ext_buf))[2] << 24) |
	      (((unsigned char *)(ext->ext_buf))[3] << 16) |
	      (((unsigned char *)(ext->ext_buf))[4] <<  8) |
	       ((unsigned char *)(ext->ext_buf))[5];
	if ( i != RT_GETPUT_MAGIC_1) {
		rt_log("ERROR: bad getput buffer header x%x, s/b x%x, was %s(x%x), file %s, line %d\n",
		    ext->ext_buf, RT_GETPUT_MAGIC_1,
		    rt_identify_magic( i), i, __FILE__, __LINE__);
		rt_bomb("bad getput buffer");
	}
	ext->ext_nbytes = len;
	i = (((unsigned char *)(ext->ext_buf))[len-2] <<8) | 
	     ((unsigned char *)(ext->ext_buf))[len-1];
	if ( i != RT_GETPUT_MAGIC_2) {
		rt_log("ERROR: bad getput buffer x%x, s/b x%x, was %s(x%x), file %s, line %s\n",
		    ext->ext_buf, RT_GETPUT_MAGIC_2,
		    rt_identify_magic( i), i, __FILE__, __LINE__);
		rt_bomb("Bad getput buffer");
	}
	return(1);
}
