#ifndef lint
static char rcsid[] = "$Header$";
#endif
#include <stdio.h>
extern int Debug;
extern double Beta;
/*	sharpen	- return a sharpened tone mapped buffer.
 *
 * Entry:
 *	buf	- buffer to place bytes.
 *	size	- size of element
 *	num	- number of elements to read.
 *	file	- file to read from.
 *	Map	- tone scale mapping.
 *
 * Exit:
 *	returns 0 on EOF
 *		number of byes read otherwise.
 *
 * Uses:
 *	Debug	- Current debug level.
 *	Beta	- sharpening value.
 *
 * Calls:
 *	None.
 *
 * Method:
 *	if no sharpening just read a line tone scale and return.
 *	if first time called get space for processing and do setup.
 *	if first line then
 *		only use cur and next lines
 *	else if last line then
 *		only use cur and last lines
 *	else
 *		use last cur and next lines
 *	endif
 *
 * Author:
 *	Christopher T. Johnson
 *
 * $Log$
 * Revision 1.2  90/04/13  00:46:54  cjohnson
 * Clean up comments.
 * Fix error on second line.
 * 
 * Revision 1.1  90/04/13  00:31:49  cjohnson
 * Initial revision
 * 
 * 
 */
int
sharpen(buf,size,num,file,Map)
unsigned char *buf;
int  size;
int  num;
FILE *file;
unsigned char *Map;
{
	static unsigned char *last,*cur=0,*next;
	static int linelen;
	int result;

	register  int i,value;

/*
 *	if no sharpening going on then just read from the file and exit.
 */
	if (Beta == 0.0) {
		result = fread(buf, size, num, file);
		if (!result) return(result);
		for (i=0; i<size*num; i++) {
			buf[i] = Map[buf[i]];
		}
		return(result);
	}

/*
 *	if we are sharpening we depend on the pixel size being one char.
 */
	if (size != 1) {
		fprintf(stderr, "sharpen: size != 1.\n");
		exit(2);
	}

/*
 *	if this is the first time we have been called then get some
 *	space to load pixels into and read first and second line of
 *	the file.
 */
	if (!cur) {
		linelen=num;
		last = 0;
		cur  = (unsigned char *)malloc(linelen);
		next = (unsigned char *)malloc(linelen);
		result = fread(cur, 1, linelen, file);
		for (i=0; i<linelen;i++) cur[i] = Map[cur[i]];
		if (!result) return(result);	/* nothing there! */
		result = fread(next, 1, linelen, file);
		if (!result) {
			free(next);
			next = 0;
		} else {
			for (i=0; i<linelen;i++) cur[i] = Map[cur[i]];
		}
	} else {
		unsigned char *tmp;

		if (linelen != num) {
			fprintf(stderr,"sharpen: buffer size changed!\n");
			exit(2);
		}
/*
 *		rotate the buffers.
 */
		tmp=last;
		last=cur;
		cur=next;
		next=tmp;
		result=fread(next, 1, linelen, file);
/*
 *		if at EOF then free up a buffer and set next to NULL
 *		to flag EOF for the next time we are called.
 */
		if (!result) {
			free(next);
			next = 0;
		} else {
			for (i=0; i<linelen;i++) cur[i] = Map[cur[i]];
		}
	}
/*
 *	if cur is NULL then we are at EOF.  Memory leak here as
 *	we don't free last.
 */
	if (!cur) return(0);

/*
 * Value is the following Laplacian filter kernel:
 *		0.25
 *	0.25	-1.0	0.25
 *		0.25
 *
 * Thus value is zero in constant value areas and none zero on
 * edges.
 *
 * Page 335 of Digital Halftoning defines
 *	J     [n] = J[n] - Beta*Laplacian_filter[n]*J[n]
 *	 sharp
 *
 * J, J     , Laplacian_filter[n] are all in the range 0.0 to 1.0
 *     sharp
 *
 * The folowing is done in mostly interger mode, there is only one
 * floating point multiply done.
 */

/*
 *	if first line
 */
	if (!last) {
		i=0;
		value=next[i] + cur[i+1] - cur[i]*2;
		buf[i] = cur[i] - Beta*(value*cur[i]/(255*2));
		for (; i < linelen-1; i++) {
			value = next[i] + cur[i-1] + cur[i+1] - cur[i]*3;
			buf[i] = cur[i] - Beta*(value*cur[i]/(255*3));
		}
		value=next[i] + cur[i-1] - cur[i]*2;
		buf[i] = cur[i] - Beta*(value*cur[i]/(255*2));
/*
 *		first time through so we will need this buffer space
 *		the next time through.
 */
		last  = (unsigned char *)malloc(linelen);
/*
 *	if last line
 */
	} else if (!next) {
		i=0;
		value=last[i] + cur[i+1] - cur[i]*2;
		buf[i] = cur[i] - Beta*(value*cur[i]/(255*2));
		for (; i < linelen-1; i++) {
			value = last[i] + cur[i-1] + cur[i+1] - cur[i]*3;
			buf[i] = cur[i] - Beta*(value*cur[i]/(255*3));
		}
		value=last[i] + cur[i-1] - cur[i]*2;
		buf[i] = cur[i] - Beta*(value*cur[i]/(255*2));
/*
 *	all other lines.
 */
	} else {
		i=0;
		value=last[i] + next[i] + cur[i+1] - cur[i]*3;
		buf[i] = cur[i] - Beta*(value*cur[i]/(255*3));
		for (; i < linelen-1; i++) {
			value = last[i] + next[i] + cur[i-1] + cur[i+1]
			     - cur[i]*4;
			buf[i] = cur[i] - Beta*(value*cur[i]/(255*4));
		}
		value=last[i] + next[i] + cur[i-1] - cur[i]*3;
		buf[i] = cur[i] - Beta*(value*cur[i]/(255*3));
	}
	return(linelen);
}
