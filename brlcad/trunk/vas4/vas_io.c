/*
 *			V A S _ I O . C
 *
 *  I/O routines to talk to a Lyon-Lamb VAS IV video animation controller.
 *
 *
 *  Authors -
 *	Steve Satterfield, USNA
 *	Joe Johnson, USNA
 *	Michael John Muuss, BRL
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Distribution Status -
 *	Public Domain, Distribution Unlimitied.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <sgtty.h>

#define RW 2
struct sgttyb user, vtty;

#include "./vas4.h"

#define VAS_PORT	"/dev/vas"
#define BAUD		B300

void	vas_response();

int vas_fd;
extern int debug;

/*
 * vas_open - attach to the VAS serial line
 *
 *	return a file descriptor of NULL on error
 */
void
vas_open()
{

	/* Open VAS Port */
	if((vas_fd=open(VAS_PORT,RW)) < 0){
		perror(VAS_PORT);
		exit(1);
	}

	/* Setup VAS line */
	vtty.sg_ispeed = BAUD;
	vtty.sg_ospeed = BAUD;
	vtty.sg_flags = RAW|EVENP|ODDP;
	ioctl(vas_fd,TIOCSETP,&vtty);
	ioctl(vas_fd,TIOCEXCL,&vtty);	/* exclusive use */
}

/*
 *			V A S _ R A W P U T C
 *
 *  Defeat the (typically desirable) ack/rexmit behavior of the
 *  vas_putc() routine.  Useful mostly for the 'V' (get_vtr_status)
 *  command, and the 'A' (send current activity) commands.
 */
int
vas_rawputc(c)
char c;
{
	int	got;

	got = write(vas_fd, &c, 1);
	if (got != 1)  {
		perror("VAS Write");
		return(got);
		/* Error recovery?? */
	}
	if(debug) fprintf(stderr,"vas_rawputc 0%o '%c'\n",c,c);
	return(got);
}

/*
 *			V A S _ P U T C
 *
 *  Output the specified character to the VAS.
 *  Wait for a 006 (^F) ACK of this character, 
 *  or for an "activity state" character,
 *  else retransmit.
 */
int
vas_putc(c)
char c;
{
	int	got;
	int	reply;
	int	count;

	for( count=0; count<20; count++ )  {
		got = write(vas_fd, &c, 1);
		if (got < 1)  {
			perror("VAS Write");
			return(got);
			/* Error recovery?? */
		}
		if(debug) fprintf(stderr,"vas_putc 0%o '%c'\n",c,c);

reread:
		reply=vas_getc();
		if( reply == 006 )
			return(got);		/* ACK */
		if( reply >= 0x60 && reply <= 0x78 )  {
			vas_response(reply);
			return(got);
		}
		if( reply == 007 )  {
			if(count>4) fprintf(stderr, "retry\n");
			sleep(1);
			continue;		/* NACK, please repeat */
		}
		fprintf(stderr,"vas4:  non-ACK rcvd for cmd %c\n",c);
		vas_response(reply);
		goto reread;		/* See if ACK is buffered up */
	}
	fprintf(stderr,"vas4:  unable to perform cmd %c after retries\n", c);
	return(-1);
}

/*
 *			V A S _ P U T S
 *
 *  Output a null terminated string to the VAS.
 */
void
vas_puts(s)
char *s;
{
	while (*s != NULL ) {
		vas_putc(*s++);
	}
}

/*
 *			V A S _ P U T N U M
 *
 *  Output a number in decimal to the VAS.
 */
void
vas_putnum(n)
int	n;
{
	char	buf[32];

	sprintf(buf,"%d",n);
	vas_puts(buf);
}


/*
 *			V A S _ G E T C
 *
 * Read a single character from the VAS, return EOF on error
 */
int
vas_getc()
{
	char c;

	if (read(vas_fd, &c, 1) > 0)  {
		if(debug)fprintf(stderr,"vas_getc: 0%o %c\n", c&0377, c&0377);
		return(c & 0377);
	}  else
		return(EOF);
}

/*
 *			V A S _ C L O S E
 */
void
vas_close()
{
	close(vas_fd);
	vas_fd = -1;
}

/*
 *			V A S _ A W A I T
 *
 *  Slurp up input characters, until designated one comes along.
 *  If too much time has been spent waiting, we should consider
 *  some way of bailing out.  Error recovery needs more attention.
 *
 *  Returns -
 *	0	all is well
 *	-1	failure
 */
int
vas_await(c, sec)
int	c;
int	sec;
{
	int	reply;
	int	count;

	for(count=0; count<20; count++)  {
		reply = vas_getc();
		if(debug) vas_response(reply);
		if( reply == c )  return(0);	/* OK */
		if(!debug) vas_response(reply);
	}
	return(-1);			/* BAD:  too many bad chars */
}

/*
 *			V A S _ R E S P O N S E
 *
 *  Attempt to interpret a reply as something sensible.
 *  This may not work in all cases (such as a multi-character response),
 *  but certainly beats looking at single-character codes.
 */
void
vas_response(c)
char	c;
{
	fprintf(stderr,"---Got 0%o '%c' ", c, c);
	switch(c)  {
	case 6:
		fprintf(stderr,"last command accepted\n");
		break;
	case 7:
		fprintf(stderr,"***Command ignored at current activity level\n");
		break;
	case 'I':
		fprintf(stderr,"Initialized.  Controller is ready for operation\n");
		break;
	case 'P':
		fprintf(stderr,"Program cmd accepted\n");
		break;
	case 'F':
		fprintf(stderr,"Frame rate cmd accepted\n");
		break;
	case 'E':
		fprintf(stderr,"Update cmd accepted\n");
		break;
	case 'U':
		fprintf(stderr,"Update cmd accepted\n");
		break;
	case 'S':
		fprintf(stderr,"Search command accepted, ready for E/E\n");
		break;
	case 'W':
		fprintf(stderr,"After E/E, search began, scene is not correct\n");
		break;
	case 'B':
		fprintf(stderr,"After E/E, search began, frame code lost while checking scene\n");
		break;
	case 'N':
		fprintf(stderr,"After E/E, search for frame fails (preceding frame not found)\n");
		break;
	case 'R':
		fprintf(stderr,"Ready to accept Record command\n");
		break;
	case 'M':
		fprintf(stderr,"Preroll fails after Record cmd, backspace for retry begins\n");
		break;
	case 'X':
		fprintf(stderr,"Notice:  2 frames before cut-in\n");
		break;
	case 'Y':
		fprintf(stderr,"Notice:  2 frames before cut-out\n");
		break;
	case 'D':
		fprintf(stderr,"Recording done, starting backspacing for next preroll\n");
		break;
	case 'J':
		fprintf(stderr,"Jaunt:  standby timeout;  tape moving back to Title\n");
		break;
	case 'T':
		fprintf(stderr,"Trash:  recording interrupted by STOP\n");
		break;
	case 'Q':
		fprintf(stderr,"Quit:  Ending EDIT mode\n");
		break;
	case 'L':
		fprintf(stderr,"Located sought frame\n");
		break;
	case 'K':
		fprintf(stderr,"Knave:  Search-for-frame failed\n");
		break;

	/**** Current activity states ****/
	/**** These are all suspect, as they don't match the BRL manual ***/
	case '`':
		fprintf(stderr,"Idling:  Power-on condition or newly initialized\n");
		break;
	case 'a':
		fprintf(stderr,"Register function is active\n");
		break;
	case 'b':
		fprintf(stderr,"Accepting programming for a recording\n");
		break;
	case 'c':
		fprintf(stderr,"Accepting programming for an edit recording\n");
		break;
	case 'd':
		fprintf(stderr,"Flashing E/E switch; ready to search for frame\n");
		break;
	case 'e':
		fprintf(stderr,"Checking for position on correct scene\n");
		break;
	case 'f':
		fprintf(stderr,"Ready to record next recording or TITLE\n");
		break;
	case 'g':
		fprintf(stderr,"Prerolling, about to make recording\n");
		break;
	case 'h':
		fprintf(stderr,"Recording in progress\n");
		break;
	case 'i':
		fprintf(stderr,"Backspacing for next preroll and recording\n");
		break;
	case 'j':
		fprintf(stderr,"Searching for frame preceding next to record\n");
		break;
	case 'k':
		fprintf(stderr,"Accepting programming for Frame Change\n");
		break;
	case 'l':
		fprintf(stderr,"Accepting programming for HOLD\n");
		break;
	case 'm':
		fprintf(stderr,"Displaying a warning message\n");
		break;
	case 'n':
		fprintf(stderr,"Ready to record first recording on old scene\n");
		break;
	case 'o':
		fprintf(stderr,"Holding momentarily before allowing to RECORD\n");
		break;

	default:
		fprintf(stderr,"???unknown???\n");
		break;
	}
}
