/*
 *			P K G . C
 *
 *  Routines to manage multiplexing and de-multiplexing synchronous
 *  and asynchronous messages across stream connections.
 *
 *  Functions -
 *	pkg_gshort	Get a 16-bit short from a char[2] array
 *	pkg_glong	Get a 32-bit long from a char[4] array
 *	pkg_pshort	Put a 16-bit short into a char[2] array
 *	pkg_plong	Put a 32-bit long into a char[4] array
 *	pkg_open	Open a network connection to a host/server
 *	pkg_transerver	Become a transient network server
 *	pkg_permserver	Create a network server, and listen for connection
 *	pkg_getclient	As permanent network server, accept a new connection
 *	pkg_close	Close a network connection
 *	pkg_send	Send a message on the connection
 *	pkg_2send	Send a two part message on the connection
 *	pkg_stream	Send a message that doesn't need a push
 *	pkg_flush	Empty the stream buffer of any queued messages
 *	pkg_waitfor	Wait for a specific msg, user buf, processing others
 *	pkg_bwaitfor	Wait for specific msg, malloc buf, processing others
 *	pkg_block	Wait until a full message has been read
 *
 *  Authors -
 *	Michael John Muuss
 *	Charles M. Kennedy
 *	Phillip Dykstra
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Distribution Status -
 *	Public Domain, Distribution Unlimitied.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#define _BSD_TYPES		/* Needed for IRIX 5.0.1 */
#include <sys/types.h>
#include <ctype.h>		/* used by inet_addr() routine, below */

#if defined(sgi) && !defined(mips)
# define IP2			/* Bypass horrible bug in netinet/tcp.h */
#endif

#ifdef BSD
/* 4.2BSD, 4.3BSD network stuff */
#if defined(__bsdi__)
# include <sys/param.h>	/* needed to pick up #define for BYTE_ORDER */
#endif

#include <sys/socket.h>
#include <sys/ioctl.h>		/* for FIONBIO */
#include <netinet/in.h>		/* for htons(), etc */
#include <netdb.h>
#include <netinet/tcp.h>	/* for TCP_NODELAY sockopt */
#undef LITTLE_ENDIAN		/* defined in netinet/{ip.h,tcp.h} */

#include "machine.h"		/* Can safely be removed for non-BRLCAD use */
#include "externs.h"		/* Can safely be removed for non-BRLCAD use */

/* Not all systems with "BSD Networking" include UNIX Domain sockets */
#  if !defined(sgi) && !defined(i386)
#	define HAS_UNIX_DOMAIN_SOCKETS
#	include <sys/un.h>		/* UNIX Domain sockets */
#  endif
#endif

#ifdef n16
/* Encore multimax */
# include <sys/h/socket.h>
# include <sys/ioctl.h>
# include <sys/netinet/in.h>
# include <sys/aux/netdb.h>
# include <sys/netinet/tcp.h>
# define BSD 42
# undef SYSV
#endif

/*
 *  The situation with sys/time.h and time.h is crazy.
 *  We need sys/time.h for struct timeval,
 *  and time.h for struct tm.
 *
 *  on BSD (and SGI 4D), sys/time.h includes time.h,
 *  on the XMP (UNICOS 3 & 4), time.h includes sys/time.h [non-STDC only],
 *  on the Cray-2, there is no automatic including.
 *
 *  Note that on many SYSV machines, the Cakefile has to set BSD
 */
#if BSD && !SYSV
#  include <sys/time.h>		/* includes <time.h> */
#else
#    if CRAY1 && !__STDC__
#	include <time.h>	/* includes <sys/time.h> */
#    else
#	include <sys/time.h>
#	include <time.h>
#    endif
#endif

#if defined(BSD) && !defined(CRAY) && !defined(mips) && !defined(n16) && !defined(i386)
#define	HAS_WRITEV
#endif

#ifdef HAS_WRITEV
# include <sys/uio.h>		/* for struct iovec (writev) */
#endif

#include <errno.h>
#include "pkg.h"

#if defined(SYSV) && !defined(bzero)
#	define bzero(str,n)		memset( str, '\0', n )
#	define bcopy(from,to,count)	memcpy( to, from, count )
#endif

#if defined(__stardent)
/* <sys/byteorder.h> seems to be wrong, and this is a LITTLE_ENDIAN machine */
#	undef	htons
#	define	htons(x)	((((x)&0xFF)<<8)|(((x)>>8)&0xFF))
#	undef	htonl
#	define	htonl(x)	( \
	((((x)    )&0xFF)<<24) | \
	((((x)>> 8)&0xFF)<<16) | \
	((((x)>>16)&0xFF)<< 8) | \
	((((x)>>24)&0xFF)    )   )
#endif

#if !__STDC__ && !USE_PROTOTYPES
extern char	*getenv();
extern char	*malloc();
extern char	*realloc();
extern void	perror();
#endif
extern int errno;

int pkg_nochecking = 0;	/* set to disable extra checking for input */
int pkg_permport = 0;	/* TCP port that pkg_permserver() is listening on XXX */

/* Internal Functions */
static struct pkg_conn *pkg_makeconn();
static void pkg_errlog();
static void pkg_perror();
static int pkg_mread();
static int pkg_dispatch();
static int pkg_gethdr();

static char errbuf[80];
static FILE	*pkg_debug;
static void	pkg_ck_debug();
static void	pkg_timestamp();
static void	pkg_checkin();

#define PKG_CK(p)	{if(p==PKC_NULL||p->pkc_magic!=PKG_MAGIC) {\
			sprintf(errbuf,"%s: bad pointer x%x line %d\n",__FILE__, p, __LINE__);\
			pkg_errlog(errbuf);abort();}}

#define	MAXQLEN	512	/* largest packet we will queue on stream */

/* A macro for logging a string message when the debug file is open */
#if 1
#define DMSG(s) if(pkg_debug) { \
	pkg_timestamp(); fprintf(pkg_debug,s); fflush(pkg_debug);}
#else
#define DMSG(s)	/**/
#endif

/*
 * Routines to insert/extract short/long's into char arrays,
 * independend of machine byte order and word-alignment.
 */

/*
 *			P K G _ G S H O R T
 */
unsigned short
pkg_gshort(msgp)
	char *msgp;
{
	register unsigned char *p = (unsigned char *) msgp;
#ifdef vax
	/*
	 * vax compiler doesn't put shorts in registers
	 */
	register unsigned long u;
#else
	register unsigned short u;
#endif

	u = *p++ << 8;
	return ((unsigned short)(u | *p));
}

/*
 *			P K G _ G L O N G
 */
unsigned long
pkg_glong(msgp)
	char *msgp;
{
	register unsigned char *p = (unsigned char *) msgp;
	register unsigned long u;

	u = *p++; u <<= 8;
	u |= *p++; u <<= 8;
	u |= *p++; u <<= 8;
	return (u | *p);
}

/*
 *			P K G _ P S H O R T
 */
char *
pkg_pshort(msgp, s)
register char *msgp;
register unsigned short s;
{

	msgp[1] = s;
	msgp[0] = s >> 8;
	return(msgp+2);
}

/*
 *			P K G _ P L O N G
 */
char *
pkg_plong(msgp, l)
register char *msgp;
register unsigned long l;
{

	msgp[3] = l;
	msgp[2] = (l >>= 8);
	msgp[1] = (l >>= 8);
	msgp[0] = l >> 8;
	return(msgp+4);
}

/*
 *			P K G _ O P E N
 *
 *  We are a client.  Make a connection to the server.
 *
 *  Returns PKC_ERROR on error.
 */
struct pkg_conn *
pkg_open( host, service, protocol, uname, passwd, switchp, errlog )
char *host;
char *service;
char *protocol;
char *uname;
char *passwd;
struct pkg_switch *switchp;
void (*errlog)();
{
	struct sockaddr_in sinme;		/* Client */
	struct sockaddr_in sinhim;		/* Server */
#ifdef HAS_UNIX_DOMAIN_SOCKETS
	struct sockaddr_un sunhim;		/* Server, UNIX Domain */
#endif
	register struct hostent *hp;
	register int netfd;
	struct	sockaddr *addr;			/* UNIX or INET addr */
	int	addrlen;			/* length of address */

	pkg_ck_debug();
	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_open(%s, %s, %s, %s, (passwd), switchp=x%x, errlog=x%x)\n",
			host, service, protocol, uname,
			switchp, errlog );
		fflush(pkg_debug);
	}

	/* Check for default error handler */
	if( errlog == NULL )
		errlog = pkg_errlog;

	bzero((char *)&sinhim, sizeof(sinhim));
	bzero((char *)&sinme, sizeof(sinme));

#ifdef HAS_UNIX_DOMAIN_SOCKETS
	if( host == NULL || strlen(host) == 0 || strcmp(host,"unix") == 0 ) {
		/* UNIX Domain socket, port = pathname */
		sunhim.sun_family = AF_UNIX;
		strncpy( sunhim.sun_path, service, sizeof(sunhim.sun_path) );
		addr = (struct sockaddr *) &sunhim;
		addrlen = strlen(sunhim.sun_path) + 2;
		goto ready;
	}
#endif

	/* Determine port for service */
	if( atoi(service) > 0 )  {
		sinhim.sin_port = htons((unsigned short)atoi(service));
	} else {
#ifdef BSD
		register struct servent *sp;
		if( (sp = getservbyname( service, "tcp" )) == NULL )  {
			sprintf(errbuf,"pkg_open(%s,%s): unknown service\n",
				host, service );
			errlog(errbuf);
			return(PKC_ERROR);
		}
		sinhim.sin_port = sp->s_port;
#endif
	}

	/* Get InterNet address */
	if( atoi( host ) > 0 )  {
		/* Numeric */
		sinhim.sin_family = AF_INET;
		sinhim.sin_addr.s_addr = inet_addr(host);
	} else {
#ifdef BSD
		if( (hp = gethostbyname(host)) == NULL )  {
			sprintf(errbuf,"pkg_open(%s,%s): unknown host\n",
				host, service );
			errlog(errbuf);
			return(PKC_ERROR);
		}
		sinhim.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr, (char *)&sinhim.sin_addr, hp->h_length);
#endif
	}
	addr = (struct sockaddr *) &sinhim;
	addrlen = sizeof(struct sockaddr_in);

#ifdef BSD
ready:
	if( (netfd = socket(addr->sa_family, SOCK_STREAM, 0)) < 0 )  {
		pkg_perror( errlog, "pkg_open:  client socket" );
		return(PKC_ERROR);
	}

#if BSD >= 43 && defined(TCP_NODELAY)
	/* SunOS 3.x defines it but doesn't support it! */
	if( addr->sa_family == AF_INET ) {
		int	on = 1;
		if( setsockopt( netfd, IPPROTO_TCP, TCP_NODELAY,
		    (char *)&on, sizeof(on) ) < 0 )  {
			pkg_perror( errlog, "pkg_open: setsockopt TCP_NODELAY" );
		}
	}
#endif

	if( connect(netfd, addr, addrlen) < 0 )  {
		pkg_perror( errlog, "pkg_open: client connect" );
		close(netfd);
		return(PKC_ERROR);
	}
#endif
	return( pkg_makeconn(netfd, switchp, errlog) );
}

/*
 *  			P K G _ T R A N S E R V E R
 *  
 *  Become a one-time server on the open connection.
 *  A client has already called and we have already answered.
 *  This will be a servers starting condition if he was created
 *  by a process like the UNIX inetd.
 *
 *  Returns PKC_ERROR or a pointer to a pkg_conn structure.
 */
struct pkg_conn *
pkg_transerver( switchp, errlog )
struct pkg_switch *switchp;
void (*errlog)();
{
	pkg_ck_debug();
	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_transerver(switchp=x%x, errlog=x%x)\n",
			switchp, errlog );
		fflush(pkg_debug);
	}

	/*
	 * XXX - Somehow the system has to know what connection
	 * was accepted, it's protocol, etc.  For UNIX/inetd
	 * we use stdin.
	 */
	return( pkg_makeconn( fileno(stdin), switchp, errlog ) );
}

/*
 *  			P K G _ P E R M S E R V E R
 *  
 *  We are now going to be a server for the indicated service.
 *  Hang a LISTEN, and return the fd to select() on waiting for
 *  new connections.
 *
 *  Returns fd to listen on (>=0), -1 on error.
 */
int
pkg_permserver( service, protocol, backlog, errlog )
char *service;
char *protocol;
int backlog;
void (*errlog)();
{
	struct sockaddr_in sinme;
#ifdef HAS_UNIX_DOMAIN_SOCKETS
	struct sockaddr_un sunme;		/* UNIX Domain */
#endif
	register struct servent *sp;
	struct	sockaddr *addr;			/* UNIX or INET addr */
	int	addrlen;			/* length of address */
	int	pkg_listenfd;
	int	on = 1;

	pkg_ck_debug();
	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_permserver(%s, %s, backlog=%d, errlog=x%x\n",
			service, protocol, backlog, errlog );
		fflush(pkg_debug);
	}

	/* Check for default error handler */
	if( errlog == NULL )
		errlog = pkg_errlog;

	bzero((char *)&sinme, sizeof(sinme));

#ifdef HAS_UNIX_DOMAIN_SOCKETS
	if( service != NULL && service[0] == '/' ) {
		/* UNIX Domain socket */
		strncpy( sunme.sun_path, service, sizeof(sunme.sun_path) );
		sunme.sun_family = AF_UNIX;
		addr = (struct sockaddr *) &sunme;
		addrlen = strlen(sunme.sun_path) + 2;
		goto ready;
	}
#endif
	/* Determine port for service */
	if( atoi(service) > 0 )  {
		sinme.sin_port = htons((unsigned short)atoi(service));
	} else {
#ifdef BSD
		if( (sp = getservbyname( service, "tcp" )) == NULL )  {
			sprintf(errbuf,
				"pkg_permserver(%s,%d): unknown service\n",
				service, backlog );
			errlog(errbuf);
			return(-1);
		}
		sinme.sin_port = sp->s_port;
#endif
	}
	pkg_permport = sinme.sin_port;		/* XXX -- needs formal I/F */
	sinme.sin_family = AF_INET;
	addr = (struct sockaddr *) &sinme;
	addrlen = sizeof(struct sockaddr_in);

#ifdef BSD
ready:
	if( (pkg_listenfd = socket(addr->sa_family, SOCK_STREAM, 0)) < 0 )  {
		pkg_perror( errlog, "pkg_permserver:  socket" );
		return(-1);
	}

	if( addr->sa_family == AF_INET ) {
		if( setsockopt( pkg_listenfd, SOL_SOCKET, SO_REUSEADDR,
		    (char *)&on, sizeof(on) ) < 0 )  {
			pkg_perror( errlog, "pkg_permserver: setsockopt SO_REUSEADDR" );
		}
#if BSD >= 43 && defined(TCP_NODELAY)
		/* SunOS 3.x defines it but doesn't support it! */
		if( setsockopt( pkg_listenfd, IPPROTO_TCP, TCP_NODELAY,
		    (char *)&on, sizeof(on) ) < 0 )  {
			pkg_perror( errlog, "pkg_permserver: setsockopt TCP_NODELAY" );
		}
#endif
	}

	if( bind(pkg_listenfd, addr, addrlen) < 0 )  {
		pkg_perror( errlog, "pkg_permserver: bind" );
		close(pkg_listenfd);
		return(-1);
	}

	if( backlog > 5 )  backlog = 5;
	if( listen(pkg_listenfd, backlog) < 0 )  {
		pkg_perror( errlog, "pkg_permserver:  listen" );
		close(pkg_listenfd);
		return(-1);
	}
#endif
	return(pkg_listenfd);
}

/*
 *			P K G _ G E T C L I E N T
 *
 *  Given an fd with a listen outstanding, accept the connection.
 *  When poll == 0, accept is allowed to block.
 *  When poll != 0, accept will not block.
 *
 *  Returns -
 *	>0		ptr to pkg_conn block of new connection
 *	PKC_NULL	accept would block, try again later
 *	PKC_ERROR	fatal error
 */
struct pkg_conn *
pkg_getclient(fd, switchp, errlog, nodelay)
struct pkg_switch *switchp;
void (*errlog)();
{
#ifdef BSD
	struct sockaddr_in from;
	register int s2;
	auto int fromlen = sizeof (from);
	auto int onoff;

	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_getclient(fd=%d, switchp=x%x, errlog=x%x, nodelay=%d)\n",
			fd, switchp, errlog, nodelay );
		fflush(pkg_debug);
	}

	/* Check for default error handler */
	if( errlog == NULL )
		errlog = pkg_errlog;

#ifdef FIONBIO
	if(nodelay)  {
		onoff = 1;
		if( ioctl(fd, FIONBIO, &onoff) < 0 )
			pkg_perror( errlog, "pkg_getclient: FIONBIO 1" );
	}
#endif
	do  {
#if __STDC__
		s2 = accept(fd, (struct sockaddr *)&from, &fromlen);
#else
		s2 = accept(fd, (char *)&from, &fromlen);
#endif
		if (s2 < 0) {
			if(errno == EINTR)
				continue;
			if(errno == EWOULDBLOCK)
				return(PKC_NULL);
			pkg_perror( errlog, "pkg_getclient: accept" );
			return(PKC_ERROR);
		}
	}  while( s2 < 0);
#ifdef FIONBIO
	if(nodelay)  {		
		onoff = 0;
		if( ioctl(fd, FIONBIO, &onoff) < 0 )
			pkg_perror( errlog, "pkg_getclient: FIONBIO 2" );
		if( ioctl(s2, FIONBIO, &onoff) < 0 )
			pkg_perror( errlog, "pkg_getclient: FIONBIO 3");
	}
#endif

	return( pkg_makeconn(s2, switchp, errlog) );
#endif
}

/*
 *			P K G _ M A K E C O N N
 *
 *  Internal.
 *  Malloc and initialize a pkg_conn structure.
 *  We have already connected to a client or server on the given
 *  file descriptor.
 *
 *  Returns -
 *	>0		ptr to pkg_conn block of new connection
 *	PKC_ERROR	fatal error
 */
static
struct pkg_conn *
pkg_makeconn(fd, switchp, errlog)
struct pkg_switch *switchp;
void (*errlog)();
{
	register struct pkg_conn *pc;

	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_makeconn(fd=%d, switchp=x%x, errlog=x%x)\n",
			fd, switchp, errlog );
		fflush(pkg_debug);
	}

	/* Check for default error handler */
	if( errlog == NULL )
		errlog = pkg_errlog;

	if( (pc = (struct pkg_conn *)malloc(sizeof(struct pkg_conn)))==PKC_NULL )  {
		pkg_perror(errlog, "pkg_makeconn: malloc failure\n" );
		return(PKC_ERROR);
	}
	bzero( (char *)pc, sizeof(struct pkg_conn) );
	pc->pkc_magic = PKG_MAGIC;
	pc->pkc_fd = fd;
	pc->pkc_switch = switchp;
	pc->pkc_errlog = errlog;
	pc->pkc_left = -1;
	pc->pkc_buf = (char *)0;
	pc->pkc_curpos = (char *)0;
	pc->pkc_strpos = 0;
	pc->pkc_incur = pc->pkc_inend = 0;
	return(pc);
}

/*
 *  			P K G _ C L O S E
 *  
 *  Gracefully release the connection block and close the connection.
 */
void
pkg_close(pc)
register struct pkg_conn *pc;
{
	PKG_CK(pc);
	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_close(pc=x%x) fd=%d\n",
			pc, pc->pkc_fd );
		fflush(pkg_debug);
	}

	/* Flush any queued stream output first. */
	if( pc->pkc_strpos > 0 )  {
		(void)pkg_flush( pc );
	}

	if( pc->pkc_buf != (char *)0 )  {
		sprintf(errbuf,"pkg_close(x%x):  partial input pkg discarded, buf=x%x\n",
			pc, pc->pkc_buf);
		pc->pkc_errlog(errbuf);
		(void)free( pc->pkc_buf );
	}
	(void)close(pc->pkc_fd);
	pc->pkc_fd = -1;		/* safety */
	pc->pkc_buf = (char *)0;	/* safety */
	pc->pkc_magic = 0;		/* safety */
	(void)free( (char *)pc );
}

/*
 *			P K G _ M R E A D
 *
 * Internal.
 * This function performs the function of a read(II) but will
 * call read(II) multiple times in order to get the requested
 * number of characters.  This can be necessary because pipes
 * and network connections don't deliver data with the same
 * grouping as it is written with.  Written by Robert S. Miles, BRL.
 *
 *  Superceeded by pkg_inget() in this version.
 *  This code is retained because of it's general usefulness.
 */
static int
pkg_mread(pc, bufp, n)
struct pkg_conn *pc;
register char	*bufp;
int	n;
{
	int fd;
	register int	count = 0;
	register int	nread;

	fd = pc->pkc_fd;
	do {
		nread = read(fd, bufp, (unsigned)n-count);
		if(nread < 0)  {
			pkg_perror(pc->pkc_errlog, "pkg_mread");
			return(-1);
		}
		if(nread == 0)
			return((int)count);
		count += (unsigned)nread;
		bufp += nread;
	 } while(count < n);

	return((int)count);
}

/*
 *  			P K G _ S E N D
 *
 *  Send the user's data, prefaced with an identifying header which
 *  contains a message type value.  All header fields are exchanged
 *  in "network order".
 *
 *  Note that the whole message (header + data) should be transmitted
 *  by TCP with only one TCP_PUSH at the end, due to the use of writev().
 *
 *  Returns number of bytes of user data actually sent.
 */
int
pkg_send( type, buf, len, pc )
int type;
char *buf;
int len;
register struct pkg_conn *pc;
{
#ifdef HAS_WRITEV
	static struct iovec cmdvec[2];
#endif
	static struct pkg_header hdr;
	register int i;

	PKG_CK(pc);
	if( len < 0 )  len=0;

	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_send(type=%d, buf=x%x, len=%d, pc=x%x)\n",
			 type, buf, len, pc );
		fflush(pkg_debug);
	}

	/* Check for any pending input, no delay */
	/* Input may be read, but not acted upon, to prevent deep recursion */
	pkg_checkin( pc, 1 );

	/* Flush any queued stream output first. */
	if( pc->pkc_strpos > 0 )  {
		/*
		 * Buffered output is already queued, and needs to be
		 * flushed before sending this one.  If this pkg will
		 * also fit in the buffer, add it to the stream, and
		 * then send the whole thing with one flush.
		 * Otherwise, just flush, and proceed.
		 */
		if( len <= MAXQLEN && len <= PKG_STREAMLEN -
		    sizeof(struct pkg_header) - pc->pkc_strpos )  {
			(void)pkg_stream( type, buf, len, pc );
			return( (pkg_flush(pc) < 0) ? -1 : len );
		}
		if( pkg_flush( pc ) < 0 )
			return(-1);	/* assumes 2nd write would fail too */
	}

	pkg_pshort( hdr.pkh_magic, PKG_MAGIC );
	pkg_pshort( hdr.pkh_type, type );	/* should see if valid type */
	pkg_plong( hdr.pkh_len, len );

#ifdef HAS_WRITEV
	cmdvec[0].iov_base = (caddr_t)&hdr;
	cmdvec[0].iov_len = sizeof(hdr);
	cmdvec[1].iov_base = (caddr_t)buf;
	cmdvec[1].iov_len = len;

	/*
	 * TODO:  set this FD to NONBIO.  If not all output got sent,
	 * loop in select() waiting for capacity to go out, and
	 * reading input as well.  Prevents deadlocking.
	 */
	if( (i = writev( pc->pkc_fd, cmdvec, (len>0)?2:1 )) != len+sizeof(hdr) )  {
		if( i < 0 )  {
			pkg_perror(pc->pkc_errlog, "pkg_send: writev");
			return(-1);
		}
		sprintf(errbuf,"pkg_send of %d+%d, wrote %d\n",
			sizeof(hdr), len, i);
		(pc->pkc_errlog)(errbuf);
		return(i-sizeof(hdr));	/* amount of user data sent */
	}
#else
	/*
	 *  On the assumption that buffer copying is less expensive than
	 *  having this transmission broken into two network packets
	 *  (with TCP, each with a "push" bit set),
	 *  merge it all into one buffer here, unless size is enormous.
	 */
	if( len + sizeof(hdr) <= 16*1024 )  {
		char	tbuf[16*1024];

		bcopy( (char *)&hdr, tbuf, sizeof(hdr) );
		if( len > 0 )
			bcopy( buf, tbuf+sizeof(hdr), len );
		if( (i = write( pc->pkc_fd, tbuf, len+sizeof(hdr) )) != len+sizeof(hdr) )  {
			if( i < 0 )  {
				if( errno == EBADF )  return(-1);
				pkg_perror(pc->pkc_errlog, "pkg_send: tbuf write");
				return(-1);
			}
			sprintf(errbuf,"pkg_send of %d, wrote %d\n",
				len, i-sizeof(hdr) );
			(pc->pkc_errlog)(errbuf);
			return(i-sizeof(hdr));	/* amount of user data sent */
		}
		return(len);
	}
	/* Send them separately */
	if( (i = write( pc->pkc_fd, (char *)&hdr, sizeof(hdr) )) != sizeof(hdr) )  {
		if( i < 0 )  {
			if( errno == EBADF )  return(-1);
			pkg_perror(pc->pkc_errlog, "pkg_send: header write");
			return(-1);
		}
		sprintf(errbuf,"pkg_send header of %d, wrote %d\n", sizeof(hdr), i);
		(pc->pkc_errlog)(errbuf);
		return(-1);		/* amount of user data sent */
	}
	if( len <= 0 )  return(0);
	if( (i = write( pc->pkc_fd, buf, len )) != len )  {
		if( i < 0 )  {
			if( errno == EBADF )  return(-1);
			pkg_perror(pc->pkc_errlog, "pkg_send: write");
			return(-1);
		}
		sprintf(errbuf,"pkg_send of %d, wrote %d\n", len, i);
		(pc->pkc_errlog)(errbuf);
		return(i);		/* amount of user data sent */
	}
#endif
	return(len);
}

/*
 *			P K G _ 2 S E N D
 *
 *  Exactly like pkg_send, except user's data is located in
 *  two disjoint buffers, rather than one.
 *  Fiendishly useful!
 */
int
pkg_2send( type, buf1, len1, buf2, len2, pc )
int type;
char *buf1, *buf2;
int len1, len2;
register struct pkg_conn *pc;
{
#ifdef HAS_WRITEV
	static struct iovec cmdvec[3];
#endif
	static struct pkg_header hdr;
	struct timeval tv;
	long bits;
	register int i;

	PKG_CK(pc);
	if( len1 < 0 )  len1=0;
	if( len2 < 0 )  len2=0;

	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_send2(type=%d, buf1=x%x, len1=%d, buf2=x%x, len2=%d, pc=x%x)\n",
			 type, buf1, len1, buf2, len2, pc );
		fflush(pkg_debug);
	}

	/* Check for any pending input, no delay */
	/* Input may be read, but not acted upon, to prevent deep recursion */
	pkg_checkin( pc, 1 );

	/* Flush any queued stream output first. */
	if( pc->pkc_strpos > 0 )  {
		if( pkg_flush( pc ) < 0 )
			return(-1);	/* assumes 2nd write would fail too */
	}

	pkg_pshort( hdr.pkh_magic, PKG_MAGIC );
	pkg_pshort( hdr.pkh_type, type );	/* should see if valid type */
	pkg_plong( hdr.pkh_len, len1+len2 );

#ifdef HAS_WRITEV
	cmdvec[0].iov_base = (caddr_t)&hdr;
	cmdvec[0].iov_len = sizeof(hdr);
	cmdvec[1].iov_base = (caddr_t)buf1;
	cmdvec[1].iov_len = len1;
	cmdvec[2].iov_base = (caddr_t)buf2;
	cmdvec[2].iov_len = len2;

	/*
	 * TODO:  set this FD to NONBIO.  If not all output got sent,
	 * loop in select() waiting for capacity to go out, and
	 * reading input as well.  Prevents deadlocking.
	 */
	if( (i = writev(pc->pkc_fd, cmdvec, 3)) != len1+len2+sizeof(hdr) )  {
		if( i < 0 )  {
			pkg_perror(pc->pkc_errlog, "pkg_2send: writev");
			return(-1);
		}
		sprintf(errbuf,"pkg_2send of %d+%d+%d, wrote %d\n",
			sizeof(hdr), len1, len2, i);
		(pc->pkc_errlog)(errbuf);
		return(i-sizeof(hdr));	/* amount of user data sent */
	}
#else
	/*
	 *  On the assumption that buffer copying is less expensive than
	 *  having this transmission broken into two network packets
	 *  (with TCP, each with a "push" bit set),
	 *  merge it all into one buffer here, unless size is enormous.
	 */
	if( len1 + len2 + sizeof(hdr) <= 16*1024 )  {
		char	tbuf[16*1024];

		bcopy( (char *)&hdr, tbuf, sizeof(hdr) );
		if( len1 > 0 )
			bcopy( buf1, tbuf+sizeof(hdr), len1 );
		if( len2 > 0 )
			bcopy( buf2, tbuf+sizeof(hdr)+len1, len2 );
		if( (i = write( pc->pkc_fd, tbuf, len1+len2+sizeof(hdr) )) != len1+len2+sizeof(hdr) )  {
			if( i < 0 )  {
				if( errno == EBADF )  return(-1);
				pkg_perror(pc->pkc_errlog, "pkg_2send: tbuf write");
				return(-1);
			}
			sprintf(errbuf,"pkg_2send of %d+%d, wrote %d\n",
				len1, len2, i-sizeof(hdr) );
			(pc->pkc_errlog)(errbuf);
			return(i-sizeof(hdr));	/* amount of user data sent */
		}
		return(len1+len2);
	}
	/* Send it in three pieces */
	if( (i = write( pc->pkc_fd, (char *)&hdr, sizeof(hdr) )) != sizeof(hdr) )  {
		if( i < 0 )  {
			if( errno == EBADF )  return(-1);
			pkg_perror(pc->pkc_errlog, "pkg_2send: header write");
			sprintf(errbuf, "pkg_2send write(%d, x%x, %d) ret=%d\n",
				pc->pkc_fd, &hdr, sizeof(hdr), i );
			(pc->pkc_errlog)(errbuf);
			return(-1);
		}
		sprintf(errbuf,"pkg_2send of %d+%d+%d, wrote header=%d\n",
			sizeof(hdr), len1, len2, i );
		(pc->pkc_errlog)(errbuf);
		return(-1);		/* amount of user data sent */
	}
	if( (i = write( pc->pkc_fd, buf1, len1 )) != len1 )  {
		if( i < 0 )  {
			if( errno == EBADF )  return(-1);
			pkg_perror(pc->pkc_errlog, "pkg_2send: write buf1");
			sprintf(errbuf, "pkg_2send write(%d, x%x, %d) ret=%d\n",
				pc->pkc_fd, buf1, len1, i );
			(pc->pkc_errlog)(errbuf);
			return(-1);
		}
		sprintf(errbuf,"pkg_2send of %d+%d+%d, wrote len1=%d\n",
			sizeof(hdr), len1, len2, i );
		(pc->pkc_errlog)(errbuf);
		return(i);		/* amount of user data sent */
	}
	if( len2 <= 0 )  return(i);
	if( (i = write( pc->pkc_fd, buf2, len2 )) != len2 )  {
		if( i < 0 )  {
			if( errno == EBADF )  return(-1);
			pkg_perror(pc->pkc_errlog, "pkg_2send: write buf2");
			sprintf(errbuf, "pkg_2send write(%d, x%x, %d) ret=%d\n",
				pc->pkc_fd, buf2, len2, i );
			(pc->pkc_errlog)(errbuf);
			return(-1);
		}
		sprintf(errbuf,"pkg_2send of %d+%d+%d, wrote len2=%d\n",
			sizeof(hdr), len1, len2, i );
		(pc->pkc_errlog)(errbuf);
		return(len1+i);		/* amount of user data sent */
	}
#endif
	return(len1+len2);
}

/*
 *  			P K G _ S T R E A M
 *
 *  Exactly like pkg_send except no "push" is necessary here.
 *  If the packet is sufficiently small (MAXQLEN) it will be placed
 *  in the pkc_stream buffer (after flushing this buffer if there
 *  insufficient room).  If it is larger than this limit, it is sent
 *  via pkg_send (who will do a pkg_flush if there is already data in
 *  the stream queue).
 *
 *  Returns number of bytes of user data actually sent (or queued).
 */
int
pkg_stream( type, buf, len, pc )
int type;
char *buf;
int len;
register struct pkg_conn *pc;
{
	static struct pkg_header hdr;

	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_stream(type=%d, buf=x%x, len=%d, pc=x%x)\n",
			 type, buf, len, pc );
		fflush(pkg_debug);
	}

	if( len > MAXQLEN )
		return( pkg_send(type, buf, len, pc) );

	if( len > PKG_STREAMLEN - sizeof(struct pkg_header) - pc->pkc_strpos )
		pkg_flush( pc );

	/* Queue it */
	pkg_pshort( hdr.pkh_magic, PKG_MAGIC );
	pkg_pshort( hdr.pkh_type, type );	/* should see if valid type */
	pkg_plong( hdr.pkh_len, len );

	bcopy( (char *)&hdr, &(pc->pkc_stream[pc->pkc_strpos]),
		sizeof(struct pkg_header) );
	pc->pkc_strpos += sizeof(struct pkg_header);
	bcopy( buf, &(pc->pkc_stream[pc->pkc_strpos]), len );
	pc->pkc_strpos += len;

	return( len + sizeof(struct pkg_header) );
}

/*
 *  			P K G _ F L U S H
 *
 *  Flush any pending data in the pkc_stream buffer.
 *
 *  Returns < 0 on failure, else number of bytes sent.
 */
int
pkg_flush( pc )
register struct pkg_conn *pc;
{
	register int	i;

	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_flush( pc=x%x )\n",
			pc );
		fflush(pkg_debug);
	}

	if( pc->pkc_strpos <= 0 ) {
		pc->pkc_strpos = 0;	/* sanity for < 0 */
		return( 0 );
	}

	if( (i = write(pc->pkc_fd,pc->pkc_stream,pc->pkc_strpos)) != pc->pkc_strpos )  {
		if( i < 0 ) {
			if( errno == EBADF )  return(-1);
			pkg_perror(pc->pkc_errlog, "pkg_flush: write");
			return(-1);
		}
		sprintf(errbuf,"pkg_flush of %d, wrote %d\n",
			pc->pkc_strpos, i);
		(pc->pkc_errlog)(errbuf);
		pc->pkc_strpos -= i;
		return( i );	/* amount of user data sent */
	}
	pc->pkc_strpos = 0;
	return( i );
}

/*
 *  			P K G _ W A I T F O R
 *
 *  This routine implements a blocking read on the network connection
 *  until a message of 'type' type is received.  This can be useful for
 *  implementing the synchronous portions of a query/reply exchange.
 *  All messages of any other type are processed by pkg_block().
 *
 *  Returns the length of the message actually received, or -1 on error.
 */
int
pkg_waitfor( type, buf, len, pc )
int type;
char *buf;
int len;
register struct pkg_conn *pc;
{
	register int i;

	PKG_CK(pc);
	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_waitfor(type=%d, buf=x%x, len=%d, pc=x%x)\n",
			type, buf, len, pc );
		fflush(pkg_debug);
	}
again:
	if( pc->pkc_left >= 0 )  {
		/* Finish up remainder of partially received message */
		if( pkg_block( pc ) < 0 )
			return(-1);
	}

	if( pc->pkc_buf != (char *)0 )  {
		pc->pkc_errlog("pkg_waitfor:  buffer clash\n");
		return(-1);
	}
	if( pkg_gethdr( pc, buf ) < 0 )  return(-1);
	if( pc->pkc_type != type )  {
		/* A message of some other type has unexpectedly arrived. */
		if( pc->pkc_len > 0 )  {
			if( (pc->pkc_buf = (char *)malloc(pc->pkc_len+2)) == NULL )  {
				pkg_perror(pc->pkc_errlog, "pkg_waitfor: malloc failed");
				return(-1);
			}
			pc->pkc_curpos = pc->pkc_buf;
		}
		goto again;
	}
	pc->pkc_left = -1;
	if( pc->pkc_len == 0 )
		return(0);

	/* See if incomming message is larger than user's buffer */
	if( pc->pkc_len > len )  {
		register char *bp;
		int excess;
		sprintf(errbuf,
			"pkg_waitfor:  message %d exceeds buffer %d\n",
			pc->pkc_len, len );
		(pc->pkc_errlog)(errbuf);
		if( (i = pkg_inget( pc, buf, len )) != len )  {
			sprintf(errbuf,
				"pkg_waitfor:  pkg_inget %d gave %d\n", len, i );
			(pc->pkc_errlog)(errbuf);
			return(-1);
		}
		excess = pc->pkc_len - len;	/* size of excess message */
		if( (bp = (char *)malloc(excess)) == NULL )  {
			pkg_perror(pc->pkc_errlog, "pkg_waitfor: excess message, malloc failed");
			return(-1);
		}
		if( (i = pkg_inget( pc, bp, excess )) != excess )  {
			sprintf(errbuf,
				"pkg_waitfor: pkg_inget of excess, %d gave %d\n",
				excess, i );
			(pc->pkc_errlog)(errbuf);
			(void)free(bp);
			return(-1);
		}
		(void)free(bp);
		return(len);		/* truncated, but OK */
	}

	/* Read the whole message into the users buffer */
	if( (i = pkg_inget( pc, buf, pc->pkc_len )) != pc->pkc_len )  {
		sprintf(errbuf,
			"pkg_waitfor:  pkg_inget %d gave %d\n", pc->pkc_len, i );
		(pc->pkc_errlog)(errbuf);
		return(-1);
	}
	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_waitfor() message type=%d arrived\n", type);
		fflush(pkg_debug);
	}
	pc->pkc_buf = (char *)0;
	pc->pkc_curpos = (char *)0;
	pc->pkc_left = -1;		/* safety */
	return( pc->pkc_len );
}

/*
 *  			P K G _ B W A I T F O R
 *
 *  This routine implements a blocking read on the network connection
 *  until a message of 'type' type is received.  This can be useful for
 *  implementing the synchronous portions of a query/reply exchange.
 *  All messages of any other type are processed by pkg_block().
 *
 *  The buffer to contain the actual message is acquired via malloc(),
 *  and the caller must free it.
 *
 *  Returns pointer to message buffer, or NULL.
 */
char *
pkg_bwaitfor( type, pc )
int type;
register struct pkg_conn *pc;
{
	register int i;
	register char *tmpbuf;

	PKG_CK(pc);
	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_bwaitfor(type=%d, pc=x%x)\n",
			type, pc );
		fflush(pkg_debug);
	}
	do  {
		/* Finish any unsolicited msg */
		if( pc->pkc_left >= 0 )
			if( pkg_block(pc) < 0 )
				return((char *)0);
		if( pc->pkc_buf != (char *)0 )  {
			pc->pkc_errlog("pkg_bwaitfor:  buffer clash\n");
			return((char *)0);
		}
		if( pkg_gethdr( pc, (char *)0 ) < 0 )
			return((char *)0);
	}  while( pc->pkc_type != type );

	pc->pkc_left = -1;
	if( pc->pkc_len == 0 )
		return((char *)0);

	/* Read the whole message into the dynamic buffer */
	if( (i = pkg_inget( pc, pc->pkc_buf, pc->pkc_len )) != pc->pkc_len )  {
		sprintf(errbuf,
			"pkg_bwaitfor:  pkg_inget %d gave %d\n", pc->pkc_len, i );
		(pc->pkc_errlog)(errbuf);
	}
	tmpbuf = pc->pkc_buf;
	pc->pkc_buf = (char *)0;
	pc->pkc_curpos = (char *)0;
	pc->pkc_left = -1;		/* safety */
	/* User must free the buffer */
	return( tmpbuf );
}

/*
 *  			P K G _ P R O C E S S
 *
 *
 *  This routine should be called to process all PKGs that are
 *  stored in the internal buffer pkc_inbuf.  This routine does
 *  no I/O, and is used in a "polling" paradigm.
 *
 *  Only after pkg_process() has been called on all PKG connections
 *  should the user process suspend itself in a select() operation,
 *  otherwise packages that have been read into the internal buffer
 *  will remain unprocessed, potentially forever.
 *
 *  If an error code is returned, then select() must NOT be called
 *  until pkg_process has been called again.
 *
 *  A plausable code sample might be:
 *
 *	for(;;)  {
 *		if( pkg_process( pc ) < 0 )  {
 *			printf("pkg_process error encountered\n");
 *			continue;
 *		}
 *		if( bsdselect( pc->pkc_fd, 99, 0 ) != 0 )  {
 *			if( pkg_suckin( pc ) <= 0 )  {
 *				printf("pkg_suckin error or EOF\n");
 *				break;
 *			}
 *		}
 *		if( pkg_process( pc ) < 0 )  {
 *			printf("pkg_process error encountered\n");
 *			continue;
 *		}
 *		do_other_stuff();
 *	}
 *
 *  Note that the first call to pkg_process() handles all buffered packages
 *  before a potentially long delay in select().
 *  The second call to pkg_process() handles any new packages obtained
 *  either directly by pkg_suckin() or as a byproduct of a handler.
 *  This double checking is absolutely necessary, because
 *  the use of pkg_send() or other pkg routines either in the actual
 *  handlers or in do_other_stuff() can cause pkg_suckin() to be
 *  called to bring in more packages.
 *
 *  Returns -
 *	<0	some internal error encountered; DO NOT call select() next.
 *	 0	All ok, no packages processed
 *	>0	All ok, return is # of packages processed (for the curious)
 */
int
pkg_process(pc)
register struct pkg_conn *pc;
{
	register int	len;
	register int	available;
	register int	errcnt;
	register int	ret;
	int		goodcnt;

	goodcnt = 0;

	PKG_CK(pc);
	/* This loop exists only to cut off "hard" errors */
	for( errcnt=0; errcnt < 500; )  {
		available = pc->pkc_inend - pc->pkc_incur;	/* amt in input buf */
		if( pkg_debug )  {
			if( pc->pkc_left < 0 )  {
				sprintf(errbuf, "awaiting new header");
			} else if( pc->pkc_left > 0 )  {
				sprintf(errbuf, "need more data");
			} else {
				sprintf(errbuf, "pkg is all here");
			}
			pkg_timestamp();
			fprintf( pkg_debug,
				"pkg_process(pc=x%x) pkc_left=%d %s (avail=%d)\n",
				pc, pc->pkc_left, errbuf, available );
			fflush(pkg_debug);
		}
		if( pc->pkc_left < 0 )  {
			/*
			 *  Need to get a new PKG header.
			 *  Do so ONLY if the full header is already in the
			 *  internal buffer, to prevent blocking in pkg_gethdr().
			 */
			if( available < sizeof(struct pkg_header) )
				break;

			if( pkg_gethdr( pc, (char *)0 ) < 0 )  {
				DMSG("pkg_gethdr < 0\n");
				errcnt++;
				continue;
			}

			if( pc->pkc_left < 0 )  {
				/* pkg_gethdr() didn't get a header */
				DMSG("pkc_left still < 0 after pkg_gethdr()\n");
				errcnt++;
				continue;
			}
		}
		/*
		 *  Now pkc_left >= 0, implying header has been obtained.
		 *  Find amount still available in input buffer.
		 */
		available = pc->pkc_inend - pc->pkc_incur;

		/* copy what is here already, and dispatch when all here */
		if( pc->pkc_left > 0 )  {
			if( available <= 0 )  break;

			/* Sanity check -- buffer must be allocated by now */
			if( pc->pkc_curpos == 0 )  {
				DMSG("curpos=0\n");
				errcnt++;
				continue;
			}

			if( available > pc->pkc_left )  {
				/* There is more in input buf than just this pkg */
				len = pc->pkc_left; /* trim to amt needed */
			} else {
				/* Take all that there is */
				len = available;
			}
			len = pkg_inget( pc, pc->pkc_curpos, len );
			pc->pkc_curpos += len;
			pc->pkc_left -= len;
			if( pc->pkc_left > 0 )  {
				/*
				 *  Input buffer is exhausted, but more
				 *  data is needed to finish this package.
				 */
				break;
			}
		}

		if( pc->pkc_left != 0 )  {
			/* Somehow, a full PKG has not yet been obtained */
			DMSG("pkc_left != 0\n");
			errcnt++;
			continue;
		}

		/* Now, pkc_left == 0, dispatch the message */
		if( pkg_dispatch(pc) <= 0 )  {
			/* something bad happened */
			DMSG("pkg_dispatch failed\n");
			errcnt++;
		} else {
			/* it worked */
			goodcnt++;
		}
	}

	if( errcnt > 0 )  {
		ret = -errcnt;
	} else {
		ret = goodcnt;
	}

	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_process() ret=%d, pkc_left=%d, errcnt=%d, goodcnt=%d\n",
			ret, pc->pkc_left, errcnt, goodcnt);
		fflush(pkg_debug);
	}
	return( ret );
}

/*
 *			P K G _ D I S P A T C H
 *
 *  Internal.
 *  Given that a whole message has arrived, send it to the appropriate
 *  User Handler, or else grouse.
 *  Returns -1 on fatal error, 0 on no handler, 1 if all's well.
 */
static int
pkg_dispatch(pc)
register struct pkg_conn *pc;
{
	register int i;

	PKG_CK(pc);
	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_dispatch(pc=x%x) type=%d, buf=x%x, len=%d\n",
			pc, pc->pkc_type, pc->pkc_buf, pc->pkc_len );
		fflush(pkg_debug);
	}
	if( pc->pkc_left != 0 )  return(-1);

	/* Whole message received, process it via switchout table */
	for( i=0; pc->pkc_switch[i].pks_handler != NULL; i++ )  {
		register char *tempbuf;

		if( pc->pkc_switch[i].pks_type != pc->pkc_type )
			continue;
		/*
		 * NOTICE:  User Handler must free() message buffer!
		 * WARNING:  Handler may recurse back to pkg_suckin() --
		 * reset all connection state variables first!
		 */
		tempbuf = pc->pkc_buf;
		pc->pkc_buf = (char *)0;
		pc->pkc_curpos = (char *)0;
		pc->pkc_left = -1;		/* safety */
		/* pc->pkc_type, pc->pkc_len are preserved for handler */
		pc->pkc_switch[i].pks_handler(pc, tempbuf);
		return(1);
	}
	sprintf(errbuf,"pkg_dispatch:  no handler for message type %d, len %d\n",
		pc->pkc_type, pc->pkc_len );
	(pc->pkc_errlog)(errbuf);
	(void)free(pc->pkc_buf);
	pc->pkc_buf = (char *)0;
	pc->pkc_curpos = (char *)0;
	pc->pkc_left = -1;		/* safety */
	return(0);
}
/*
 *			P K G _ G E T H D R
 *
 *  Internal.
 *  Get header from a new message.
 *  Returns:
 *	1	when there is some message to go look at
 *	-1	on fatal errors
 */
static int
pkg_gethdr( pc, buf )
register struct pkg_conn *pc;
char *buf;
{
	register int i;

	PKG_CK(pc);
	if( pc->pkc_left >= 0 )  return(1);	/* go get it! */

	/*
	 *  At message boundary, read new header.
	 *  This will block until the new header arrives (feature).
	 */
	if( (i = pkg_inget( pc, &(pc->pkc_hdr),
	    sizeof(struct pkg_header) )) != sizeof(struct pkg_header) )  {
		if(i > 0) {
			sprintf(errbuf,"pkg_gethdr: header read of %d?\n", i);
			(pc->pkc_errlog)(errbuf);
		}
		return(-1);
	}
	while( pkg_gshort(pc->pkc_hdr.pkh_magic) != PKG_MAGIC )  {
		int	c;
		c = *((unsigned char *)&pc->pkc_hdr);
		if( isascii(c) && isprint(c) )  {
			sprintf(errbuf,
				"pkg_gethdr: skipping noise x%x %c\n",
				c, c );
		} else {
			sprintf(errbuf,
				"pkg_gethdr: skipping noise x%x\n",
				c );
		}
		(pc->pkc_errlog)(errbuf);
		/* Slide over one byte and try again */
		bcopy( ((char *)&pc->pkc_hdr)+1, (char *)&pc->pkc_hdr, sizeof(struct pkg_header)-1);
		if( (i=pkg_inget( pc,
		    ((char *)&pc->pkc_hdr)+sizeof(struct pkg_header)-1,
		    1 )) != 1 )  {
			sprintf(errbuf,"pkg_gethdr: hdr read=%d?\n",i);
		    	(pc->pkc_errlog)(errbuf);
			return(-1);
		}
	}
	pc->pkc_type = pkg_gshort(pc->pkc_hdr.pkh_type);	/* host order */
	pc->pkc_len = pkg_glong(pc->pkc_hdr.pkh_len);
	if( pc->pkc_len < 0 )  pc->pkc_len = 0;
	pc->pkc_buf = (char *)0;
	pc->pkc_left = pc->pkc_len;
	if( pc->pkc_left == 0 )  return(1);		/* msg here, no data */

	if( buf )  {
		pc->pkc_buf = buf;
	} else {
		/* Prepare to read message into dynamic buffer */
		if( (pc->pkc_buf = (char *)malloc(pc->pkc_len+2)) == NULL )  {
			pkg_perror(pc->pkc_errlog, "pkg_gethdr: malloc fail");
			return(-1);
		}
	}
	pc->pkc_curpos = pc->pkc_buf;
	return(1);			/* something ready */
}

/*
 *  			P K G _ B L O C K
 *  
 *  This routine blocks, waiting for one complete message to arrive from
 *  the network.  The actual handling of the message is done with
 *  pkg_dispatch(), which invokes the user-supplied message handler.
 *
 *  This routine can be used in a loop to pass the time while waiting
 *  for a flag to be changed by the arrival of an asynchronous message,
 *  or for the arrival of a message of uncertain type.
 *
 *  The companion routine is pkg_process(), which does not block.
 *  
 *  Control returns to the caller after one full message is processed.
 *  Returns -1 on error, etc.
 */
int
pkg_block(pc)
register struct pkg_conn *pc;
{
	PKG_CK(pc);
	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_block(pc=x%x)\n",
			pc );
		fflush(pkg_debug);
	}

	/* If no read operation going now, start one. */
	if( pc->pkc_left < 0 )  {
		if( pkg_gethdr( pc, (char *)0 ) < 0 )  return(-1);
		/* Now pkc_left >= 0 */
	}

	/* Read the rest of the message, blocking if necessary */
	if( pc->pkc_left > 0 )  {
		if( pkg_inget( pc, pc->pkc_curpos, pc->pkc_left ) != pc->pkc_left )  {
			pc->pkc_left = -1;
			return(-1);
		}
		pc->pkc_left = 0;
	}

	/* Now, pkc_left == 0, dispatch the message */
	return( pkg_dispatch(pc) );
}

extern int sys_nerr;
extern char *sys_errlist[];

/*
 *			P K G _ P E R R O R
 *
 *  Produce a perror on the error logging output.
 */
static void
pkg_perror( errlog, s )
void (*errlog)();
char *s;
{
	if( errno >= 0 && errno < sys_nerr ) {
		sprintf( errbuf, "%s: %s\n", s, sys_errlist[errno] );
		errlog( errbuf );
	} else {
		sprintf( errbuf, "%s: errno=%d\n", s, errno );
		errlog( errbuf );
	}
}

/*
 *			P K G _ E R R L O G
 *
 *  Default error logger.  Writes to stderr.
 */
static void
pkg_errlog( s )
char *s;
{
	if( pkg_debug )  {
		pkg_timestamp();
		fputs( s, pkg_debug );
		fflush(pkg_debug);
	}
	fputs( s, stderr );
}

/*
 *			P K G _ C K _ D E B U G
 */
static void
pkg_ck_debug()
{
	char	*place;
	char	buf[128];

	if( pkg_debug )  return;
	if( (place = (char *)getenv("LIBPKG_DEBUG")) == (char *)0 )  {
		sprintf( buf, "/tmp/pkg.log" );
		place = buf;
	}
	/* Named file must exist and be writeable */
	if( access( place, 2 ) < 0 )  return;
	if( (pkg_debug = fopen( place, "a" )) == NULL )  return;

	/* Log version number of this code */
	pkg_timestamp();
	fprintf( pkg_debug, "pkg_ck_debug %s\n", RCSid );
}

/*
 *			P K G _ T I M E S T A M P
 *
 *  Output a timestamp to the log, suitable for starting each line with.
 */
static void
pkg_timestamp()
{
	long		now;
	struct tm	*tmp;
	register char	*cp;

	if( !pkg_debug )  return;
	(void)time( &now );
	tmp = localtime( &now );
	fprintf(pkg_debug, "%2.2d/%2.2d %2.2d:%2.2d:%2.2d [%5d] ",
		tmp->tm_mon+1, tmp->tm_mday,
		tmp->tm_hour, tmp->tm_min, tmp->tm_sec,
		getpid() );
	/* Don't fflush here, wait for rest of line */
}

/*
 *			P K G _ S U C K I N
 *
 *  Suck all data from the operating system into the internal buffer.
 *  This is done with large buffers, to maximize the efficiency of the
 *  data transfer from kernel to user.
 *
 *  It is expected that the read() system call will return as much
 *  data as the kernel has, UP TO the size indicated.
 *  The only time the read() may be expected to block is when the
 *  kernel does not have any data at all.
 *  Thus, it is wise to call call this routine only if:
 *	a)  select() has indicated the presence of data, or
 *	b)  blocking is acceptable.
 *
 *  This routine is the only place where data is taken off the network.
 *  All input is appended to the internal buffer for later processing.
 *
 *  Subscripting was used for pkc_incur/pkc_inend to avoid having to
 *  recompute pointers after a realloc().
 *
 *  Returns -
 *	-1	on error
 *	 0	on EOF
 *	 1	success
 */
int
pkg_suckin(pc)
register struct pkg_conn	*pc;
{
	int	waste;
	int	avail;
	int	got;
	int	ret;

	got = 0;
	PKG_CK(pc);

	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_suckin() incur=%d, inend=%d, inlen=%d\n",
			pc->pkc_incur, pc->pkc_inend, pc->pkc_inlen );
		fflush(pkg_debug);
	}

	/* If no buffer allocated yet, get one */
	if( pc->pkc_inbuf == (char *)0 || pc->pkc_inlen <= 0 )  {
		pc->pkc_inlen = PKG_STREAMLEN;
		if( (pc->pkc_inbuf = (char *)malloc(pc->pkc_inlen)) == (char *)0 )  {
			pc->pkc_errlog("pkg_suckin malloc failure\n");
			pc->pkc_inlen = 0;
			ret = -1;
			goto out;
		}
		pc->pkc_incur = pc->pkc_inend = 0;
	}

	if( pc->pkc_incur >= pc->pkc_inend )  {
		/* Reset to beginning of buffer */
		pc->pkc_incur = pc->pkc_inend = 0;
	}

	/* If cur point is near end of buffer, recopy data to buffer front */
	if( pc->pkc_incur >= (pc->pkc_inlen * 8) / 7 )  {
		register int	ammount;

		ammount = pc->pkc_inend - pc->pkc_incur;
		/* This copy can not overlap itself, because of 7/8 above */
		bcopy( &pc->pkc_inbuf[pc->pkc_incur],
			pc->pkc_inbuf, ammount );
		pc->pkc_incur = 0;
		pc->pkc_inend = ammount;
	}

	/* If remaining buffer space is small, make buffer bigger */
	avail = pc->pkc_inlen - pc->pkc_inend;
	if( avail < 10 * sizeof(struct pkg_header) )  {
		pc->pkc_inlen <<= 1;
		if( pkg_debug)  {
			pkg_timestamp();
			fprintf(pkg_debug,
				"pkg_suckin: realloc inbuf to %d\n",
				pc->pkc_inlen );
			fflush(pkg_debug);
		}
		if( (pc->pkc_inbuf = (char *)realloc(pc->pkc_inbuf, pc->pkc_inlen)) == (char *)0 )  {
			pc->pkc_errlog("pkg_suckin realloc failure\n");
			pc->pkc_inlen = 0;
			ret = -1;
			goto out;
		}
	}

	/* Take as much as the system will give us, up to buffer size */
	if( (got = read( pc->pkc_fd, &pc->pkc_inbuf[pc->pkc_inend], avail )) <= 0 )  {
		if( got == 0 )  {
			if( pkg_debug )  {
				pkg_timestamp();
				fprintf(pkg_debug,
					"pkg_suckin: fd=%d, read for %d returned 0\n",
					avail, pc->pkc_fd );
				fflush(pkg_debug);
			}
			ret = 0;	/* EOF */
			goto out;
		}
		pkg_perror(pc->pkc_errlog, "pkg_suckin: read");
		sprintf(errbuf, "pkg_suckin: read(%d, x%x, %d) ret=%d inbuf=x%x, inend=%d\n",
			pc->pkc_fd, &pc->pkc_inbuf[pc->pkc_inend], avail,
			got,
			pc->pkc_inbuf, pc->pkc_inend );
		(pc->pkc_errlog)(errbuf);
		ret = -1;
		goto out;
	}
	if( got > avail )  {
		pc->pkc_errlog("pkg_suckin:  read more bytes than desired\n");
		got = avail;
	}
	pc->pkc_inend += got;
	ret = 1;
out:
	if( pkg_debug )  {
		pkg_timestamp();
		fprintf( pkg_debug,
			"pkg_suckin() ret=%d, got %d, total=%d\n",
			ret, got, pc->pkc_inend - pc->pkc_incur );
		fflush(pkg_debug);
	}
	return(ret);
}

/*
 *			P K G _ C H E C K I N
 *
 *  This routine is called whenever it is necessary to see if there
 *  is more input that can be read.
 *  If input is available, it is read into pkc_inbuf[].
 *  If nodelay is set, poll without waiting.
 */
static void
pkg_checkin(pc, nodelay)
register struct pkg_conn	*pc;
int		nodelay;
{
	struct timeval	tv;
	long		bits;
	register int	i;
	extern int	errno;

	/* Check socket for unexpected input */
	tv.tv_sec = 0;
	if( nodelay )
		tv.tv_usec = 0;		/* poll -- no waiting */
	else
		tv.tv_usec = 20000;	/* 20 ms */
	bits = 1 << pc->pkc_fd;
	i = select( pc->pkc_fd+1, &bits, (char *)0, (char *)0, &tv );
	if( pkg_debug )  {
		pkg_timestamp();
		fprintf(pkg_debug,
			"pkg_checkin: select on fd %d returned %d, bits=x%x\n",
			pc->pkc_fd,
			i, bits );
		fflush(pkg_debug);
	}
	if( i > 0 )  {
		if( bits != 0 )  {
			(void)pkg_suckin(pc);
		} else {
			/* Odd condition */
			sprintf(errbuf,
				"pkg_checkin: select returned %d, bits=0\n",
				i );
			(pc->pkc_errlog)(errbuf);
		}
	} else if( i < 0 )  {
		/* Error condition */
		if( errno != EINTR && errno != EBADF )
			pkg_perror(pc->pkc_errlog, "pkg_checkin: select");
	}
}

/*
 *			P K G _ I N G E T
 *
 *  A functional replacement for mread(), through the
 *  first level input buffer.
 *  This will block if the required number of bytes are not available.
 *  The number of bytes actually transferred is returned.
 */
int
pkg_inget( pc, buf, count )
register struct pkg_conn	*pc;
char		*buf;
int		count;
{
	register int	len;
	register int	todo = count;

	while( todo > 0 )  {
		
		while( (len = pc->pkc_inend - pc->pkc_incur) <= 0 )  {
			/* This can block */
			if( pkg_suckin( pc ) < 1 )
				return( count - todo );
		}
		/* Input Buffer has some data in it, move to caller's buffer */
		if( len > todo )  len = todo;
		bcopy( &pc->pkc_inbuf[pc->pkc_incur], buf, len );
		pc->pkc_incur += len;
		buf += len;
		todo -= len;
	}
	return( count );
}
