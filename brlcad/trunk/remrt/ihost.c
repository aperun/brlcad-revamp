/*
 *			I H O S T . C
 *
 *  Internal host table routines.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Status -
 *	Public Domain, Distribution Unlimited.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (ARL)";
#endif

#include "conf.h"

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <math.h>
#ifdef USE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "machine.h"
#include "vmath.h"
#include "rtstring.h"
#include "rtlist.h"
#include "raytrace.h"
#include "externs.h"

#include "./ihost.h"

struct rt_list	HostHead;

/*
 *			G E T _ O U R _ H O S T N A M E
 *
 * There is a problem in some hosts that gethostname() will only
 * return the host name and *not* the fully qualified host name
 * with domain name.
 *
 * gethostbyname() will return a host table (nameserver) entry
 * where h_name is the "offical name", i.e. fully qualified.
 * Therefore the following piece of code.
 */
char *
get_our_hostname()
{
	char temp[512];
	struct hostent *hp;

	/* Init list head here */
	RT_LIST_INIT( &HostHead );

	gethostname(temp, sizeof(temp));

	hp = gethostbyname(temp);

	return rt_strdup(hp->h_name);
}

/*
 *			H O S T _ L O O K U P _ B Y _ H O S T E N T
 *
 *  We have a hostent structure, of which, the only thing of interest is
 *  the host name.  Go from name to address back to name, to get formal name.
 *
 *  Used by host_lookup_by_addr, too.
 */
struct ihost *
host_lookup_by_hostent( addr, enter )
struct hostent	*addr;
int		enter;
{
	register struct ihost	*ihp;

	if( !(addr == gethostbyname(addr->h_name)) )
		return IHOST_NULL;
	if( !(addr == gethostbyaddr(addr->h_addr_list[0],
	    sizeof(struct in_addr), addr->h_addrtype)) )
		return IHOST_NULL;
	/* Now addr->h_name points to the "formal" name of the host */

	/* Search list for existing instance */
	for( RT_LIST_FOR( ihp, ihost, &HostHead ) )  {
		CK_IHOST(ihp);

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
	RT_LIST_INSERT( &HostHead, &ihp->l );

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

	addr_tmp = from->sin_addr.s_addr;
	addr = gethostbyaddr( (char *)&from->sin_addr, sizeof (struct in_addr),
		from->sin_family);
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
	for( RT_LIST_FOR( ihp, ihost, &HostHead ) )  {
		CK_IHOST(ihp);
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
		sockhim.sin_addr.s_addr = inet_addr(name);
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
