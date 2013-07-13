/*                      B W H I S T E Q . C
 * BRL-CAD
 *
 * Copyright (c) 1986-2013 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file util/bwhisteq.c
 *
 * Build up the histogram of a picture and output the "equalized"
 * version of it on stdout.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "bu.h"


#define BINSIZE 256

long bin[BINSIZE];
unsigned char new[BINSIZE];

#define rand01()	((random()&0xffff)/65535.0)

FILE *fp;

static const char usage[] = "Usage: bwhisteq [-v] file.bw > file.equalized\n";

int verbose = 0;

int
main(int argc, char **argv)
{
    int n, i;
    unsigned char buf[512];
    unsigned char obuf[512];
    unsigned char *bp;
    int left[BINSIZE], right[BINSIZE];
    double hint, havg;
    long r;
    size_t ret;

    if (argc > 1 && BU_STR_EQUAL(argv[1], "-v")) {
	verbose++;
	argc--;
	argv++;
    }

    if (argc != 2 || isatty(fileno(stdout))) {
	bu_exit(1, "%s", usage);
    }

    if ((fp = fopen(argv[1], "r")) == NULL) {
	bu_exit(2, "bwhisteq: Can't open \"%s\"\n", argv[1]);
    }

    /* Tally up the intensities */
    havg = 0.0;
    while ((n = fread(buf, sizeof(*buf), 512, fp)) > 0) {
	bp = &buf[0];
	for (i = 0; i < n; i++)
	    bin[ *bp++ ]++;
	havg += n;
    }
    havg /= (double)BINSIZE;

    r = 0;
    hint = 0;
    for (i = 0; i < BINSIZE; i++) {
	left[i] = r;
	hint += bin[i];
	while (hint > havg) {
	    hint -= havg;
	    r++;
	}
	right[i] = r;
#ifdef METHOD2
	new[i] = right[i] - left[i];
#else
	new[i] = (left[i] + right[i]) / 2;
#endif /* Not METHOD2 */
    }

    if (verbose) {
	for (i = 0; i < BINSIZE; i++)
	    fprintf(stderr, "new[%d] = %d\n", i, new[i]);
    }

    bu_fseek(fp, 0, 0);
    while ((n = fread(buf, 1, 512, fp)) > 0) {
	for (i = 0; i < n; i++) {
	    long idx = buf[i];

	    if (idx < 0)
		idx = 0;
	    if (idx > BINSIZE-1)
		idx = BINSIZE-1;

	    if (left[idx] == right[idx])
		obuf[i] = left[idx];
	    else {
#ifdef METHOD2
		obuf[i] = left[idx] + new[idx] * rand01();
#else
		obuf[i] = new[idx];
#endif /* Not METHOD2 */
	    }
	}
	ret = fwrite(obuf, 1, n, stdout);
	if (ret == 0) {
	    perror("fwrite");
	    break;
	}
    }
    return 0;
}


/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
