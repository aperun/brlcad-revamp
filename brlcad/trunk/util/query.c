/*
 *			Q U E R Y . C
 *
 * By default, query reads a line from standard input and echoes it
 * to standard output.
 *
 *  entry:
 *	-t	seconds to wait for user response.
 *	-r	default response
 *	-v	toggle verbose option (Default "r" in "t" seconds)
 *	-l	loop rather than respond.
 *
 *  Exit:
 *	<stdout>	The line read
 *		or	y
 *		or	response
 *
 *  Calls:
 *	get_args
 *
 *  Method:
 *	straight-forward.
 *
 *  Author:
 *	Christopher T. Johnson - 90/08/28
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <signal.h>

char	Yes_Response[] = "y";
int	Verbose=0;
char	*Response= Yes_Response;
int	Timeout=0;
int	Loop=0;
int	Done = 0;
extern 	int errno;

static char usage[] = "\
Usage: %s [-v] [-t seconds] [-r response ] [-l]\n";

extern char *optarg;

get_args( argc, argv )
register char **argv;
{
	register int c;

	while ( (c = getopt( argc, argv, "t:r:vl" )) != EOF )  {
		switch( c )  {
		case 't':
			Timeout = atoi(optarg);
			break;
		case 'r':
			Response = optarg;
			break;
		case 'v':
			Verbose = 1 - Verbose;
			break;
		case 'l':
			Loop = 1;
			break;
		default:		/* '?' */
			return(0);
		}
	}
	if (Timeout < 0) Timeout = 0;
	if (Loop & Timeout <= 0) Timeout=5;

	if (Loop) Verbose = 0;

	return(1);
}
void handler();

main( argc, argv )
int argc;
char *argv[];
{
	char line[80];
	char *eol;
	char *flag;

	if ( !get_args( argc, argv ) )  {
		(void) fprintf(stderr,usage,argv[0]);
		exit( 1 );
	}

#ifndef	SYSV
	(void) signal(SIGALRM, handler);
#endif
	Done = 0;

	for(;;) {
#ifdef	SYSV
		(void) signal(SIGALRM, handler);
#endif
		if (Verbose) {
			if (Timeout) {
				(void) fprintf(stderr, 
				    "(Default: %s in %d sec)", Response,
				    Timeout);
			} else {
				(void) fprintf(stderr,
				    "(Default: %s)", Response);
			}
		}
		
		if (Loop) {
			(void) fprintf(stderr,
			    "(Default: %s, loop in %d sec)", Response, Timeout);
		}
		if (Timeout) alarm(Timeout);

		flag = fgets(line, 80, stdin);
		if (Done) {
			if (Loop) {
				(void) fputs("\n\007", stderr);
				Done = 0;
			} else {
				(void) fputs(Response, stdout);
				(void) putchar('\n');
				exit(0);
			}
		} else if (flag == NULL) {
			(void) fputs(Response, stdout);
			exit(0);
		} else { /* good response */
			eol = strlen(line) + line;
			if (*(eol-1) == '\n') {
				--eol;
				*eol = '\0';
			}
			if (eol == line) {
				(void) fputs(Response, stdout);
			} else {
				(void) fputs(line, stdout);
			}
			(void)putchar('\n');
			exit(0);
		}
	}
}
void
handler()
{
	Done = 1;
}
