/*	C A N O N I Z E --- queue an image to a Canon CLC500 via qpr/MDQS
 *
 */
#include <stdio.h>
#include <string.h>
#include "./canon.h"

char cmdbuf[64]="/usr/mdqs/bin/qpr -q "; /* queue name filled in by main() */

/*	Q U E U E
 *
 *	send an image off into the printer queuing system
 */
void
queue(fd)
FILE *fd;
{
	char img_buffer[8 * 1024];
	int img_bytes, i, args, bytes_read;
	FILE *pfd;
	
	img_bytes = width * height * 3;

	/* open a pipe to the queuing program */
	if ((pfd = popen(cmdbuf, "w")) == (FILE *)NULL) {
			fprintf(stderr, "%s: ", progname);
			perror(cmdbuf);
			exit(-1);
	}

	if (ipu_debug)
		fprintf(stderr, "pipe open\n");

	/* write the command line options out to the data stream */
	for (args=1 ; args < arg_c ; args++) {

		if (!strcmp(arg_v[args], "-a")) {
			fprintf(pfd, " -w %d -n %d", width, height);
		} if (!strcmp(arg_v[args], "-d")) {
			args += 2;	/* skip device specification */
		} if (!strcmp(arg_v[args], "-v") ||
		      !strcmp(arg_v[args], "-V")) {
			continue;	/* skip verbose specification */
		} else {
			fprintf(pfd, " %s", arg_v[args]);
		}
	}
	fprintf(pfd, "\n");

	if (ipu_debug)
		fprintf(stderr, "args written\n");

	/* write the image down the pipe */
	for( bytes_read = 0 ;
	    bytes_read < img_bytes &&
	    (i = fread(img_buffer, 1, sizeof(img_buffer), fd)) ;
	    bytes_read += i ) {
		fwrite(img_buffer, 1, i, pfd);
	}

	if (ipu_debug)
		fprintf(stderr, "image written\n");

	pclose(pfd);
	free(img_buffer);

	if (ipu_debug)
		fprintf(stderr, "image queued\n");
}


int
main(ac, av)
int ac;
char *av[];
{
	int arg_ind;
	FILE *fd;


	/* copy the relevant command line options */
	if ((arg_ind = parse_args(ac, av)) >= ac) {
		if (isatty(fileno(stdin)))
			usage("Specify image on cmd line or redirect from standard input\n");

		if (autosize) usage("Cannot autosize stdin\n");

		strncat(cmdbuf, print_queue, sizeof(cmdbuf)-strlen(cmdbuf)-1);

		queue(stdin);

		return(0);
	}

	strncat(cmdbuf, print_queue, sizeof(cmdbuf)-strlen(cmdbuf)-1);

	for ( ; arg_ind < ac ; arg_ind++) {
		if (autosize &&
		    !fb_common_file_size( &width, &height, av[arg_ind], 3)) {
		    	fprintf(stderr,
		    		"unable to autosize \"%s\"\n",
		    		av[arg_ind]);
		    	return(-1);
		}

		if ((fd=fopen(av[arg_ind], "r")) == (FILE *)NULL) {
			fprintf(stderr, "%s: ", progname);
			perror(av[arg_ind]);
			return(-1);
		}
		queue(fd);
		fclose(fd);
	}

}
