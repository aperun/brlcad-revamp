/*
 *			V E R T . C
 * Author -
 *	Christopher T. Johnson
 *
 * Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1986 by the United States Army.
 *	All rights reserved.
 *
 *
 * Vert.c is a routine to translate data formats.  The data formats are:
 *
 *	Host/Network		is the data in host format or local format
 *	signed/unsigned		Is the data signed?
 *	char/short/int/long/double
 *				Is the data 8bits, 16bits, 32bits, 64bits
 *				or a double?
 *
 *
 * The method of conversion is to convert up to double then back down the
 * the expected output format.
 *
 */
#ifndef line
static char RCSid[] = "$Header$ (BRL)";
#endif

#include "conf.h"

#include <stdio.h>
#include <ctype.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"

/*
 * Theses should be moved to a header file soon.
 */
#define CV_CHANNEL_MASK	0x00ff
#define CV_HOST_MASK	0x0100
#define CV_SIGNED_MASK	0x0200
#define CV_TYPE_MASK	0x1c00
#define CV_CONVERT_MASK 0x6000

#define CV_TYPE_SHIFT	10
#define CV_CONVERT_SHIFT 13

#define CV_8	0x0400
#define	CV_16	0x0800
#define CV_32	0x0c00
#define CV_64	0x1000
#define CV_D	0x1400

#define CV_CLIP		0x0000
#define CV_NORMAL	0x2000
#define CV_LIT		0x4000

#define	IND_NOTSET	0
#define IND_BIG		1
#define IND_LITTLE	2
#define IND_ILL		3		/* PDP-11 */
#define IND_CRAY	4

/* cv_cookie	Set's a bit vector after parsing an input string.
 *
 * Set up the conversion tables/flags for vert.
 *
 * Entry:
 *	in	format description.
 *
 * Exit:
 *	returns a 32 bit vector.
 *
 * Format description:
 *	[channels][h|n][s|u] c|s|i|l|d|8|16|32|64 [N|C|L]
 *
 * channels must be null or 1
 * Host | Network
 * signed | unsigned
 * char | short | integer | long | double | number of bits of integer
 * Normalize | Clip | low-order
 */
int
cv_cookie(in)
char *in;			/* input format */
{
	char *p;
	int collector;
	int result = 0x0000;	/* zero/one channel, Net, unsigned, char, clip */

	if (!in) return 0;
	if (!*in) return 0;


	collector = 0;
	for (p=in; *p && isdigit(*p); ++p) collector = collector*10 + (*p - '0');
	if (collector > 255) {
		collector = 255;
	} else if (collector == 0) {
		collector = 1;
	}
	result = collector;	/* number of channels set '|=' */

	if (!*p) return 0;

	if (*p == 'h') {
		result |= CV_HOST_MASK;
		++p;
	} else if (*p == 'n') {
		++p;
	}

	if (!*p) return 0;
	if (*p == 'u') {
		++p;
	} else if (*p == 's') {	/* could be 'signed' or 'short' */
		char *p2;
		p2 = p+1;
		if (*p2 && (islower(*p2) || isdigit(*p2))) {
			result |= CV_SIGNED_MASK;
			++p;
		}
	}

	if (!*p) return 0;
	switch (*p) {
	case 'c':
	case '8':
		result |= CV_8;
		break;
	case '1':
		p++;
		if (*p != '6') return 0;
		/* fall through */
	case 's':
		result |= CV_16;
		break;
	case '3':
		p++;
		if (*p != '2') return 0;
		/* fall through */
	case 'i':
		result |= CV_32;
		break;
	case '6':
		p++;
		if (*p != '4') return 0;
		/* fall through */
	case 'l':
		result |= CV_64;
		break;
	case 'd':
		result |= CV_D;
		break;
	default:
		return 0;
	}
	p++;

	if (!*p) return(result);
	if (*p == 'N') {
		result |= CV_NORMAL;
	} else if (*p == 'C') {
		result |= CV_CLIP;
	} else if (*p == 'L') {
		result |= CV_LIT;
	} else {
		return 0;
	}
	return(result);
}

void
cv_fmt_cookie( buf, buflen, cookie )
char	*buf;
int	buflen;
int	cookie;
{
	register char *cp = buf;
	int	len;

	buflen--;
	if( cookie == 0 )  {
		strncpy( cp, "bogus!", buflen );
		return;
	}
	/* Make sure there is enough room */
	if (buflen < 5) {
		if( buflen < 0 )  fprintf(stderr, "cv_pr_cookie:  call me with a bigger buffer\n");
		return;
	}
	sprintf( cp, "%d", cookie & CV_CHANNEL_MASK );
	len = strlen(cp);
	cp += len;
	buflen -= len;

	if( cookie & CV_HOST_MASK )  {
		*cp++ = 'h';
		buflen--;
	} else {
		*cp++ = 'n';
		buflen--;
	}

	if( cookie & CV_SIGNED_MASK )  {
		*cp++ = 's';
		buflen--;
	} else {
		*cp++ = 'u';
		buflen--;
	}

	if (buflen < 3) {
		if( buflen < 0 )  fprintf(stderr, "cv_pr_cookie:  call me with a bigger buffer\n");
		return;
	}
		
	switch( cookie & CV_TYPE_MASK )  {
	case CV_8:
		*cp++ = '8';
		buflen--;
		break;
	case CV_16:
		strncpy( cp, "16", buflen );
		cp += 2;
		buflen -= 2;
		break;
	case CV_32:
		strncpy( cp, "32", buflen );
		cp += 2;
		buflen -= 2;
		break;
	case CV_64:
		strncpy( cp, "64", buflen );
		cp += 2;
		buflen -= 2;
		break;
	case CV_D:
		*cp++ = 'd';
		buflen -= 1;
		break;
	default:
		*cp++ = '?';
		buflen -= 1;
		break;
	}

	switch( cookie & CV_CONVERT_MASK )  {
	case CV_CLIP:
		*cp++ = 'C';
		buflen -= 1;
		break;
	case CV_NORMAL:
		*cp++ = 'N';
		buflen -= 1;
		break;
	case CV_LIT:
		*cp++ = 'L';
		buflen -= 1;
		break;
	default:
		*cp++ = 'X';
		buflen -= 1;
		break;
	}
	*cp = '\0';
	if( buflen < 0 )  fprintf(stderr, "cv_pr_cookie:  call me with a bigger buffer\n");
}

void
cv_pr_cookie( title, cookie )
char	*title;
int	cookie;
{
	char	buf[128];

	cv_fmt_cookie( buf, sizeof(buf), cookie );
	fprintf( stderr, "%s cookie '%s' (x%x)\n", title, buf, cookie );
}

/* cv - convert from one format to another.
 *
 * Entry:
 *	in	input pointer
 *	out	output pointer
 *	count	number of entries to convert.
 *	size	size of output buffer.
 *	infmt	input format
 *	outfmt	output format
 *
 */
cv(out, outfmt, size, in, infmt, count)
genptr_t out;
char	*outfmt;
int	size;
genptr_t in;
char	*infmt;
int	count;
{
	int	incookie, outcookie;
	incookie = cv_cookie(infmt);
	outcookie = cv_cookie(outfmt);
	return(cv_w_cookie(out, outcookie, size, in, incookie, count));
}

/*
 *			C V _ O P T I M I Z E
 *
 *  It is always more efficient to handle host data, rather than network.
 *  If host and network formats are the same, and the request was for
 *  network format, modify the cookie to request host format.
 */
int
cv_optimize( cookie )
register int	cookie;
{
	static int Indian = IND_NOTSET;
	int	fmt;

	if( cookie & CV_HOST_MASK )
		return cookie;		/* already in most efficient form */

	/* This is a network format request */
	fmt  =  cookie & CV_TYPE_MASK;

	/* Run time check:  which kind of integers does this machine have? */
	if (Indian == IND_NOTSET) {
		unsigned long int	testval;
		register int		i;
		for (i=0; i<4; i++) {
			((char *)&testval)[i] = i+1;
		}
		if (sizeof (long int) == 8) {
			Indian = IND_CRAY;	/* is this good enough? */
		} else if (testval == 0x01020304) {
			Indian = IND_BIG;
		} else if (testval == 0x04030201) {
			Indian = IND_LITTLE;
		} else if (testval == 0x02010403) {
			Indian = IND_ILL;
		}
	}

	switch(fmt)  {
	case CV_D:
#		if IEEE_FLOAT
			cookie |= CV_HOST_MASK;	/* host uses network fmt */
#		endif
		return cookie;
	case CV_8:
		return cookie | CV_HOST_MASK;	/* bytes already host format */
	case CV_16:
	case CV_32:
	case CV_64:
		/* host is big-endian, so is network */
		if( Indian == IND_BIG )
			cookie |= CV_HOST_MASK;
		return cookie;
	}
	return 0;			/* ERROR */
}

/*
 *			C V _ I T E M L E N
 *
 *  Returns the number of bytes each "item" of type "cookie" occupies.
 */
int
cv_itemlen( cookie )
register int	cookie;
{
	register int	fmt = (cookie & CV_TYPE_MASK) >> CV_TYPE_SHIFT;
	static int host_size_table[8] = {0, sizeof(char),
		sizeof(short), sizeof(int),
		sizeof(long int), sizeof(double)};
	static int net_size_table[8] = {0,1,2,4,8,8};

	if( cookie & CV_HOST_MASK )
		return host_size_table[fmt];
	return net_size_table[fmt];
}

/* cv_w_cookie - convert with cookie
 *
 * Entry:
 *	in		input pointer
 *	incookie	input format cookie.
 *	count		number of entries to convert.
 *	out		output pointer.
 *	outcookie	output format cookie.
 *	size		size of output buffer in bytes;
 *
 *
 * A worst case would be:	ns16 on vax to ns32
 *	ns16 	-> hs16
 *		-> hd
 *		-> hs32
 *		-> ns32
 * The worst case is probably the easiest to deal with because all steps are
 * done.  The more difficult cases are when only a subset of steps need to
 * be done.
 *
 * Method:
 *	HOSTDBL defined as true or false
 *	if ! hostother then
 *		hostother = (Indian == IND_BIG) ? SAME : DIFFERENT;
 *	fi
 *	if (infmt == double) then
 *		if (HOSTDBL == SAME) {
 *			inIsHost = host;
 *		fi
 *	else
 *		if (hostother == SAME) {
 *			inIsHost = host;
 *		fi
 *	fi
 *	if (outfmt == double) then
 *		if (HOSTDBL == SAME) {
 *			outIsHost == host;
 *	else
 *		if (hostother == SAME) {
 *			outIsHost = host;
 *		fi
 *	fi
 *	if (infmt == outfmt) {
 *		if (inIsHost == outIsHost) {
 *			copy(in,out)
 *			exit
 *		else if (inIsHost == net) {
 *			ntoh?(in,out);
 *			exit
 *		else
 *			hton?(in,out);
 *			exit
 *		fi
 *	fi
 *
 *	while not done {
 *		from = in;
 *
 *		if (inIsHost == net) {
 *			ntoh?(from,t1);
 *			from = t1;
 *		fi
 *		if (infmt != double ) {
 *			if (outIsHost == host) {
 *				to = out;
 *			else
 *				to = t2;
 *			fi
 *			castdbl(from,to);
 *			from = to;
 *		fi
 *
 *		if (outfmt == double ) {
 *			if (outIsHost == net) {
 *				hton?(from,out);
 *			fi
 *		else 
 *			if (outIsHost == host) {
 *				dblcast(from,out);
 *			else
 *				dblcast(from,t3);
 *				hton?(t3,out);
 *			fi
 *		fi
 *	done
 */
cv_w_cookie(out, outcookie, size, in, incookie, count)
genptr_t out;
int	outcookie;
int	size;
genptr_t in;
int	incookie;
int	count;
{
	int	work_count = 4096;
	int	number_done = 0;
	int	inIsHost,outIsHost,infmt,outfmt,insize,outsize;
	int	bufsize;
	genptr_t	t1,t2,t3;
	genptr_t	from;
	genptr_t	to;
	genptr_t	hold;
	register int i;

/*
 * Work_count is the size of the working buffer.  If count is smaller
 * than the default work_count (4096) use the smaller number.
 */

	if (work_count > count) work_count = count;

	incookie = cv_optimize( incookie );
	outcookie = cv_optimize( outcookie );

/*
 * break out the conversion code and the format code.
 * Conversion is net<-->host.
 * Format is 8/16/32/64/D casting.
 */
	inIsHost = incookie & CV_HOST_MASK;	/* not zero if host */
	outIsHost= outcookie& CV_HOST_MASK;
	infmt  =  incookie & CV_TYPE_MASK;
	outfmt = outcookie & CV_TYPE_MASK;
/*
 * Find the number of bytes required for one item of each kind.
 */
	outsize = cv_itemlen( outcookie );
	insize = cv_itemlen( incookie );

/*
 * If the input format is the same as the output format then the
 * most that has to be done is a host to net or net to host conversion.
 */
	if (infmt == outfmt) {

/*
 * Input format is the same as output format, do we need to do a
 * host/net conversion?
 */
		if (inIsHost == outIsHost) {

/*
 * No conversion required.
 * Check the amount of space remaining before doing the bcopy.
 */
			if (count * outsize > size) {
				number_done = size / outsize;
			} else {
				number_done = count;
			}

/*
 * This is the simplest case, binary copy and out.
 */
			(void) bcopy((genptr_t) in, (genptr_t) out,
			    number_done * outsize);
			return(number_done);

/*
 * Well it's still the same format but the conversion are different.
 * Only one of the *vert variables can be HOST therefore if
 * inIsHost != HOST then outIsHost must be host format.
 */

		} else if (inIsHost != CV_HOST_MASK) { /* net format */
			switch(incookie & (CV_SIGNED_MASK | CV_TYPE_MASK)) {
			case CV_SIGNED_MASK | CV_16:
				return(	ntohss(out, size, in, count));
			case CV_16:
				return( ntohus(out, size, in, count));
			case CV_SIGNED_MASK | CV_32:
				return( ntohsl(out, size, in, count));
			case CV_32:
				return( ntohul(out, size, in, count));
			case CV_D:
				(void) ntohd(out, in, count);
				return(count);
			}

/*
 * Since inIsHost != outIsHost and inIsHost == HOST then outIsHost must be
 * in net format.  call the correct subroutine to do the conversion.
 */
		} else {
			switch(incookie & (CV_SIGNED_MASK | CV_TYPE_MASK)) {
			case CV_SIGNED_MASK | CV_16:
				return(	htonss(out, size, in, count));
			case CV_16:
				return( htonus(out, size, in, count));
			case CV_SIGNED_MASK | CV_32:
				return( htonsl(out, size, in, count));
			case CV_32:
				return( htonul(out, size, in, count));
			case CV_D:
				(void) htond(out, in, count);
				return(count);
			}
		}
	}
/*
 * If we get to this point then the input format is known to be
 * of a diffrent type than the output format.  This will require
 * a cast to, from or to and from double.
 *
 * because of the number of steps is not know to begin with, we get
 * three working buffers.  The size of a double is the largest of
 * any of the sizes we may be dealing with.
 */

	bufsize = work_count * sizeof(double);
	t1 = (genptr_t) rt_malloc(bufsize, "vert.c: t1");
	t2 = (genptr_t) rt_malloc(bufsize, "vert.c: t2");
	t3 = (genptr_t) rt_malloc(bufsize, "vert.c: t3");

/*
 * From here on we will be working on a chunk of process at a time.
 */
	while ( size >= outsize  && number_done < count) {
		int remaining;

/*
 * Size is the number of bytes that the caller said was available.
 * We need the check to make sure that we will not convert to many
 * entries, overflowing the output buffer.
 */

/*
 * Get number of full entries that can be converted
 */
		remaining = size / outsize;

/*
 * If number of entries that would fit in the output buffer is
 * larger than the number of entries left to convert(based on
 * count and number done), set remaining to request count minus
 * the number of conversions already completed.
 */
		if (remaining > count - number_done) {
			remaining = count - number_done;
		}
/*
 * If we are in the last chuck, set the work count to take up
 * the slack.
 */
		if (remaining < work_count) work_count = remaining;

/*
 * All input at any stage will come from the "from" pointer.  We
 * start with the from pointer pointing to the input buffer.
 */
		from = in;

/*
 * We will be processing work_count entries of insize bytes each, so
 * we set the in pointer to be ready for the next time through the loop.
 */
		in = ((char *) in) + work_count * insize;

/*
 * If the input is in net format convert it host format.
 * Because we know that the input format is not equal to the output
 * this means that there will be at least two conversions taking place
 * if the input is in net format.  (from net to host then at least one cast)
 */
		if (inIsHost != CV_HOST_MASK) { /* net format */
			switch(incookie & (CV_SIGNED_MASK | CV_TYPE_MASK)) {
			case CV_SIGNED_MASK | CV_16:
				(void) ntohss(t1, bufsize , from, work_count);
				break;
			case CV_16:
				(void) ntohus(t1, bufsize , from, work_count);
				break;
			case CV_SIGNED_MASK | CV_32:
				(void) ntohsl(t1, bufsize , from, work_count);
				break;
			case CV_32:
				(void) ntohul(t1, bufsize , from, work_count);
				break;
			case CV_D:
				(void) ntohd(t1, from, work_count);
				break;
			}
/*
 * Point the "from" pointer to the host format.
 */
			from = t1;
		}


/*
 * "From" is a pointer to a HOST format buffer.
 */

/*
 * If the input format is not double then there must be a cast to
 * double.
 */
		if (infmt != CV_D) {

/*
 * if the output conversion is HOST and output format is DOUBLE
 * then this will be the last step.
 */
			if (outIsHost == CV_HOST_MASK && outfmt == CV_D) {
				to = out;
			} else {
				to = t2;
			}

			hold = to;
/*
 * Cast the input format to double.
 */
			switch(incookie & (CV_SIGNED_MASK | CV_TYPE_MASK)) {
			case CV_SIGNED_MASK | CV_8:
				for (i=0; i< work_count; i++) {
					*((double *)to) = *((SIGNED char *)from);
					to = (genptr_t)(((double *)to) + 1);
					from = ((char *)from) + 1;
				}
				break;
			case CV_8:
				for(i=0; i < work_count; i++) {
					*((double *)to) = *((unsigned char *)from);
					to = (genptr_t)(((double *)to) + 1);
					from = (genptr_t)(((unsigned char *)from) + 1);
				}
				break;
			case CV_SIGNED_MASK | CV_16:
				for (i=0; i < work_count; i++) {
					*((double *)to) = *((SIGNED short *)from);
					to = (genptr_t)(((double *)to) + 1);
					from = (genptr_t)(((SIGNED short *)from) + 1);
				}
				break;
			case CV_16:
				for (i=0; i < work_count; i++) {
					*((double *)to) = *((unsigned short *)from);
					to = (genptr_t)(((double *)to) + 1);
					from = (genptr_t)(((unsigned short *)from) + 1);
				}
				break;
			case CV_SIGNED_MASK | CV_32:
				for (i=0; i < work_count; i++) {
					*((double *)to) = *((SIGNED long int *)from);
					to = (genptr_t)(((double *)to) + 1);
					from =  (genptr_t)(((SIGNED long int *)from) + 1);
				}
				break;
			case CV_32:
				for (i=0; i < work_count; i++) {
					*((double *)to) = *((unsigned long int *) from);
					to = (genptr_t)(((double *)to) + 1);
					from = (genptr_t)(((unsigned long int *)from) + 1);
				}
				break;
			}
			from = hold;
		}

		if (outfmt != CV_D) {
/*
 * The input point is now pointing to a double in host format.  If the
 * output is also in host format then the next conversion will be
 * the last conversion, set the destination to reflect this.
 */

			if (outIsHost == CV_HOST_MASK) {
				to = out;
			} else {
				to = t3;
			}

/*
 * The ouput format is something other than DOUBLE (tested for earlier),
 * do a cast from double to requested format.
 */
			hold = to;

			switch (outcookie & (CV_SIGNED_MASK | CV_TYPE_MASK)) {
			case CV_SIGNED_MASK | CV_8:
				for (i=0; i<work_count; i++) {
					*((SIGNED char *)to) = *((double *)from);
					to = (genptr_t)(((SIGNED char *)to) + 1);
					from = (genptr_t)(((double *)from) + 1);
				}
				break;
			case CV_8:
				for (i=0; i<work_count; i++) {
					*((unsigned char *)to) =
					    (unsigned char)(*((double *)from));
					to = (genptr_t)(((unsigned char *)to) + 1);
					from = (genptr_t)(((double *)from) + 1);
				}
				break;
			case CV_SIGNED_MASK | CV_16:
				for (i=0; i<work_count; i++) {
					*((SIGNED short int *)to) =
					    *((double *)from);
					to = (genptr_t)(((SIGNED short int *)to) + 1);
					from = (genptr_t)(((double *)from) + 1);
				}
				break;
			case CV_16:
				for (i=0; i<work_count; i++) {
					*((unsigned short int *)to) =
					    *((double *)from);
					to = (genptr_t)(((unsigned short int *)to) + 1);
					from = (genptr_t)(((double *)from) + 1);
				}
				break;
			case CV_SIGNED_MASK | CV_32:
				for (i=0; i<work_count; i++) {
					*((SIGNED long int *)to) =
					    *((double *)from);
					to = (genptr_t)(((SIGNED long int *)to) + 1);
					from = (genptr_t)(((double *)from) + 1);
				}
				break;
			case CV_32:
				for (i=0; i<work_count; i++) {
					*((unsigned long int *)to) =
					    *((double *)from);
					to = (genptr_t)(((unsigned long int *)to) + 1);
					from = (genptr_t)(((double *)from) + 1);
				}
				break;
			}
			from = hold;
/*
 * The input is now pointing to a host formated buffer of the requested
 * output format.
 */

/*
 * If the output conversion is network then do a host to net call
 * for either 16 or 32 bit values using Host TO Network All Short | Long
 */
			if (outIsHost != CV_HOST_MASK) {
				switch (outfmt) {
				case CV_D:
					(void) htond(out,from, work_count);
					break;
#if 0
				case CV_16:
					(void) htonas(out, bufsize, from,
					    work_count);
					break;
				case CV_32:
					(void) htonal(out, bufsize, from,
					    work_count);
					break;
#endif
				}
			}
					
		}
/*
 * move the output pointer.
 * reduce the amount of space remaining in the output buffer.
 * Increament the count of values converted.
 */
		out = ((char *)out) + work_count * outsize;
		size -= work_count * outsize;
		number_done += work_count;
	}
/*
 * All Done!  Clean up and leave.
 */
	rt_free(t1, "vert.c: t1");
	rt_free(t2, "vert.c: t2");
	rt_free(t3, "vert.c: t3");
	return(number_done);
}

/*	ntohss	Network TO Host Signed Short
 *
 * It is assumed that this routine will only be called if there is
 * real work to do.  Ntohs does no checking to see if it is reasonable
 * to do any conversions.
 *
 * Entry:
 *	in	generic pointer for input.
 *	count	number of shorts to be generated.
 *	out	short pointer for output
 *	size	number of bytes of space reserved for out.
 *
 * Exit:
 *	returns	number of conversions done.
 *
 * Calls:
 *	none.
 *
 * Method:
 *	Straight-forward.
 */
int
ntohss(out, size, in, count)
register SIGNED short	*out;
int			size;
register genptr_t	in;
int			count;
{
	int limit;
	register int i;

	limit = size / sizeof(SIGNED short);
	if (limit < count) count = limit;

	for (i=0; i<count; i++) {
		*out++ = ((SIGNED char *)in)[0] << 8 | ((unsigned char *)in)[1];
		/* XXX This needs sign extension here for the case of
		 * XXX a negative 2-byte input on a 4 or 8 byte machine.
		 * XXX The "signed char" trick isn't enough.
		 * XXX Use your Cyber trick w/magic numbers or something.
		 */
		in = ((char *)in) + 2;
	}
	return(count);
}
int
ntohus(out, size, in, count)
register unsigned short	*out;
int			size;
register genptr_t	in;
int			count;
{
	int limit;
	register int i;

	limit = size / sizeof(unsigned short);
	if (limit < count) count = limit;

	for (i=0; i<count; i++) {
		*out++ = ((unsigned char *)in)[0]<<8 |
		    ((unsigned char *)in)[1];
		in = ((char *)in) + 2;
	}
	return(count);
}
int
ntohsl(out, size, in, count)
register SIGNED long int	*out;
int				size;
register genptr_t		in;
int				count;
{
	int limit;
	register int i;

	limit = size / sizeof(SIGNED long int);
	if (limit < count) count = limit;

	for (i=0; i<count; i++) {
		*out++ = ((SIGNED char *)in)[0] << 24 |
		    ((unsigned char *)in)[1] << 16 | 
		    ((unsigned char *)in)[2] << 8  |
		    ((unsigned char *)in)[3];
		/* XXX Sign extension here */
		in = ((char *)in) + 4;
	}
	return(count);
}
int
ntohul(out, size, in, count)
register unsigned long int	*out;
int				size;
register genptr_t		in;
int				count;
{
	int limit;
	register int i;

	limit = size / sizeof(unsigned long int);
	if (limit < count) count = limit;

	for (i=0; i<count; i++) {
		*out++ = ((unsigned char *)in)[0] << 24 |
		    ((unsigned char *)in)[1] << 16 |
		    ((unsigned char *)in)[2] <<  8 |
		    ((unsigned char *)in)[3];
		in = ((char *)in) + 4;
	}
	return(count);
}

/*****/
int
htonss(out, size, in, count)
genptr_t		out;
int			size;
register short		*in;
int			count;
{
	int		limit;
	register int	i;
	register unsigned char *cp = (unsigned char *)out;
	register int	val;

	limit = size / 2;
	if( count > limit )  count = limit;

	for (i=0; i<count; i++) {
		*cp++ = (val = *in++)>>8;
		*cp++ = val;
	}
	return(count);
}
int
htonus(out, size, in, count)
genptr_t		out;
int			size;
register unsigned short	*in;
int			count;
{
	int		limit;
	register int	i;
	register unsigned char *cp = (unsigned char *)out;
	register int	val;

	limit = size / 2;
	if( count > limit )  count = limit;

	for (i=0; i<count; i++) {
		*cp++ = (val = *in++)>>8;
		*cp++ = val;
	}
	return(count);
}
int
htonsl(out, size, in, count)
genptr_t		out;
int			size;
register long		*in;
int			count;
{
	int		limit;
	register int	i;
	register unsigned char *cp = (unsigned char *)out;
	register long	val;

	limit = size / 4;
	if( count > limit )  count = limit;

	for (i=0; i<count; i++) {
		*cp++ = (val = *in++)>>24;
		*cp++ = val>>16;
		*cp++ = val>> 8;
		*cp++ = val;
	}
	return(count);
}
int
htonul(out, size, in, count)
genptr_t		out;
int			size;
register unsigned long	*in;
int			count;
{
	int		limit;
	register int	i;
	register unsigned char *cp = (unsigned char *)out;
	register long	val;

	limit = size / 4;
	if( count > limit )  count = limit;

	for (i=0; i<count; i++) {
		*cp++ = (val = *in++)>>24;
		*cp++ = val>>16;
		*cp++ = val>> 8;
		*cp++ = val;
	}
	return(count);
}
