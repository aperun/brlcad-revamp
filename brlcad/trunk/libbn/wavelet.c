/*			W A V E L E T . C
 *
 *  Wavelet decompose/reconstruct operations
 *
 *	bn_wlt_1d_double_decompose(tbuffer, buffer, dimen, channels, limit)
 *	bn_wlt_1d_float_decompose (tbuffer, buffer, dimen, channels, limit)
 *	bn_wlt_1d_char_decompose  (tbuffer, buffer, dimen, channels, limit)
 *	bn_wlt_1d_short_decompose (tbuffer, buffer, dimen, channels, limit)
 *	bn_wlt_1d_int_decompose   (tbuffer, buffer, dimen, channels, limit)
 *	bn_wlt_1d_long_decompose  (tbuffer, buffer, dimen, channels, limit)
 *
 *	bn_wlt_1d_double_reconstruct(tbuffer, buffer, dimen, channels, sub_sz, limit)
 *	bn_wlt_1d_float_reconstruct (tbuffer, buffer, dimen, channels, sub_sz, limit)
 *	bn_wlt_1d_char_reconstruct  (tbuffer, buffer, dimen, channels, sub_sz, limit)
 *	bn_wlt_1d_short_reconstruct (tbuffer, buffer, dimen, channels, sub_sz, limit)
 *	bn_wlt_1d_int_reconstruct   (tbuffer, buffer, dimen, channels, sub_sz, limit)
 *	bn_wlt_1d_long_reconstruct  (tbuffer, buffer, dimen, channels, sub_sz, limit)
 *
 *	bn_wlt_2d_double_decompose(tbuffer, buffer, dimen, channels, limit)
 *	bn_wlt_2d_float_decompose (tbuffer, buffer, dimen, channels, limit)
 *	bn_wlt_2d_char_decompose  (tbuffer, buffer, dimen, channels, limit)
 *	bn_wlt_2d_short_decompose (tbuffer, buffer, dimen, channels, limit)
 *	bn_wlt_2d_int_decompose   (tbuffer, buffer, dimen, channels, limit)
 *	bn_wlt_2d_long_decompose  (tbuffer, buffer, dimen, channels, limit)
 *
 *	bn_wlt_2d_double_reconstruct(tbuffer, buffer, dimen, channels, sub_sz, limit)
 *	bn_wlt_2d_float_reconstruct (tbuffer, buffer, dimen, channels, sub_sz, limit)
 *	bn_wlt_2d_char_reconstruct  (tbuffer, buffer, dimen, channels, sub_sz, limit)
 *	bn_wlt_2d_short_reconstruct (tbuffer, buffer, dimen, channels, sub_sz, limit)
 *	bn_wlt_2d_int_reconstruct   (tbuffer, buffer, dimen, channels, sub_sz, limit)
 *	bn_wlt_2d_long_reconstruct  (tbuffer, buffer, dimen, channels, sub_sz, limit)
 *
 *	
 *  
 *  For greatest accuracy, it is preferable to convert everything to "double"
 *  and decompose/reconstruct with that.  However, there are useful 
 *  properties to performing the decomposition and/or reconstruction in 
 *  various data types (most notably char).
 *
 *  Rather than define all of these routines explicitly, we define
 *  2 macros "decompose" and "reconstruct" which embody the structure of
 *  the function (which is common to all of them).  We then instatiate
 *  these macros once for each of the data types.  It's ugly, but it
 *  assures that a change to the structure of one operation type 
 *  (decompose or reconstruct) occurs for all data types.
 *
 *
 *
 *
 *  bn_wlt_1d_*_decompose(tbuffer, buffer, dimen, channels, limit)
 *  Parameters:
 *	tbuffer     a temporary data buffer 1/2 as large as "buffer". See (1) below.
 *	buffer      pointer to the data to be decomposed
 *	dimen    the number of samples in the data buffer 
 *	channels the number of values per sample
 *	limit    the extent of the decomposition
 *
 *  Perform a Haar wavelet decomposition on the data in buffer "buffer".  The
 *  decomposition is done "in place" on the data, hence the values in "buffer"
 *  are not preserved, but rather replaced by their decomposition.
 *  The number of original samples in the buffer (parameter "dimen") and the
 *  decomposition limit ("limit") must both be a power of 2 (e.g. 512, 1024).
 *  The buffer is decomposed into "average" and "detail" halves until the
 *  size of the "average" portion reaches "limit".  Simultaneous 
 *  decomposition of multi-plane (e.g. pixel) data, can be performed by
 *  indicating the number of planes in the "channels" parameter.
 *  
 *  (1) The process requires a temporary buffer which is 1/2 the size of the
 *  longest span to be decomposed.  If the "tbuffer" argument is non-null then
 *  it is a pointer to a temporary buffer.  If the pointer is NULL, then a
 *  local temporary buffer will be allocated (and freed).
 *
 *  Examples:
 *	double dbuffer[512], cbuffer[256];
 *	...
 *	bn_wlt_1d_double_decompose(cbuffer, dbuffer, 512, 1, 1);
 *
 *    performs complete decomposition on the data in array "dbuffer".
 *
 *	double buffer[3][512];	 /_* 512 samples, 3 values/sample (e.g. RGB?)*_/
 *	double tbuffer[3][256];	 /_* the temporary buffer *_/
 *	...
 *	bn_wlt_1d_double_decompose(tbuffer, buffer, 512, 3, 1);
 *
 *    This will completely decompose the data in buffer.  The first sample will
 *    be the average of all the samples.  Alternatively:
 *
 *	bn_wlt_1d_double_decompose(tbuffer, buffer, 512, 3, 64);
 *
 *    decomposes buffer into a 64-sample "average image" and 3 "detail" sets.
 *
 *
 *
 *  bn_wlt_1d_*_reconstruct(tbuffer, buffer, dimen, channels, sub_sz, limit)
 *
 *
 *  Author -
 *	Lee A. Butler
 *
 *  Modifications -
 *      Christopher Sean Morrison
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Status -
 *	Public Domain, Distribution Unlimited.
 */

#include <stdio.h>
#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "bn.h"


#ifdef __STDC__
#define decompose_1d(DATATYPE) bn_wlt_1d_ ## DATATYPE ## _decompose
#else
#define decompose_1d(DATATYPE) bn_wlt_1d_/**/DATATYPE/**/_decompose
#endif



#define make_wlt_1d_decompose(DATATYPE)  \
void \
decompose_1d(DATATYPE) \
( tbuffer, buffer, dimen, channels, limit ) \
DATATYPE *tbuffer;		/* temporary buffer */ \
DATATYPE *buffer;		/* data buffer */ \
unsigned long dimen;	/* # of samples in data buffer */ \
unsigned long channels;	/* # of data values per sample */ \
unsigned long limit;	/* extent of decomposition */ \
{ \
	register DATATYPE *detail; \
	register DATATYPE *avg; \
	unsigned long img_size; \
	unsigned long half_size; \
	int do_free = 0; \
	unsigned long x, x_tmp, d, i, j; \
	register fastf_t onehalf = (fastf_t)0.5; \
\
	CK_POW_2( dimen ); \
\
	if ( ! tbuffer ) { \
		tbuffer = (DATATYPE *)bu_malloc( \
				(dimen/2) * channels * sizeof( *buffer ), \
				"1d wavelet buffer"); \
		do_free = 1; \
	} \
\
	/* each iteration of this loop decomposes the data into 2 halves: \
	 * the "average image" and the "image detail" \
	 */ \
	for (img_size = dimen ; img_size > limit ; img_size = half_size ){ \
\
		half_size = img_size/2; \
		 \
		detail = tbuffer; \
		avg = buffer; \
\
		for ( x=0 ; x < img_size ; x += 2 ) { \
			x_tmp = x*channels; \
\
			for (d=0 ; d < channels ; d++, avg++, detail++) { \
				i = x_tmp + d; \
				j = i + channels; \
				*detail = (buffer[i] - buffer[j]) * onehalf; \
				*avg    = (buffer[i] + buffer[j]) * onehalf; \
			} \
		} \
\
		/* "avg" now points to the first element AFTER the "average \
		 * image" section, and hence is the START of the "image  \
		 * detail" portion.  Convenient, since we now want to copy \
		 * the contents of "tbuffer" (which holds the image detail) into \
		 * place. \
		 */ \
		memcpy(avg, tbuffer, sizeof(*buffer) * channels * half_size); \
	} \
	 \
	if (do_free) \
		bu_free( (genptr_t)tbuffer, "1d wavelet buffer"); \
}


#if defined(__STDC__) 
#define reconstruct(DATATYPE ) bn_wlt_1d_ ## DATATYPE ## _reconstruct
#else
#define reconstruct(DATATYPE) bn_wlt_1d_/**/DATATYPE/**/_reconstruct
#endif

#define make_wlt_1d_reconstruct( DATATYPE ) \
void \
reconstruct(DATATYPE) \
( tbuffer, buffer, dimen, channels, subimage_size, limit )\
DATATYPE *tbuffer; \
DATATYPE *buffer; \
unsigned long dimen; \
unsigned long channels; \
unsigned long subimage_size; \
unsigned long limit; \
{ \
	register DATATYPE *detail; \
	register DATATYPE *avg; \
	unsigned long img_size; \
	unsigned long dbl_size; \
	int do_free = 0; \
	unsigned long x_tmp, d, x, i, j; \
\
	CK_POW_2( subimage_size ); \
	CK_POW_2( dimen ); \
	CK_POW_2( limit ); \
\
        if ( ! (subimage_size < dimen) ) { \
		bu_log("%s:%d Dimension %d should be greater than subimage size (%d)\n", \
			__FILE__, __LINE__, dimen, subimage_size); \
		bu_bomb("reconstruct"); \
	} \
\
        if ( ! (subimage_size < limit) ) { \
		bu_log("%s:%d Channels limit %d should be greater than subimage size (%d)\n", \
			__FILE__, __LINE__, limit, subimage_size); \
		bu_bomb("reconstruct"); \
	} \
\
        if ( ! (limit <= dimen) ) { \
		bu_log("%s:%d Dimension %d should be greater than or equal to the channels limit (%d)\n", \
			__FILE__, __LINE__, dimen, limit); \
		bu_bomb("reconstruct"); \
	} \
\
\
	if ( ! tbuffer ) { \
		tbuffer = ( DATATYPE *)bu_malloc((dimen/2) * channels * sizeof( *buffer ), \
				"1d wavelet reconstruct tmp buffer"); \
		do_free = 1; \
	} \
\
	/* Each iteration of this loop reconstructs an image twice as \
	 * large as the original using a "detail image". \
	 */ \
	for (img_size=subimage_size ; img_size < limit ; img_size=dbl_size) { \
		dbl_size = img_size * 2; \
\
		d = img_size * channels; \
		detail = &buffer[ d ]; \
\
		/* copy the original or "average" data to temporary buffer */ \
		avg = tbuffer; \
		memcpy(avg, buffer, sizeof(*buffer) * d ); \
\
\
		for (x=0 ; x < dbl_size ; x += 2 ) { \
			x_tmp = x * channels; \
			for (d=0 ; d < channels ; d++, avg++, detail++ ) { \
				i = x_tmp + d; \
				j = i + channels; \
				buffer[i] = *avg + *detail; \
				buffer[j] = *avg - *detail; \
			} \
		} \
	} \
\
	if (do_free) \
		bu_free( (genptr_t)tbuffer, \
			"1d wavelet reconstruct tmp buffer"); \
}

/* Believe it or not, this is where the actual code is generated */

make_wlt_1d_decompose(double)
make_wlt_1d_reconstruct(double)

make_wlt_1d_decompose(float)
make_wlt_1d_reconstruct(float)

make_wlt_1d_decompose(char)
make_wlt_1d_reconstruct(char)

make_wlt_1d_decompose(int)
make_wlt_1d_reconstruct(int)

make_wlt_1d_decompose(short)
make_wlt_1d_reconstruct(short)

make_wlt_1d_decompose(long)
make_wlt_1d_reconstruct(long)


#ifdef __STDC__
#define decompose_2d( DATATYPE ) bn_wlt_2d_ ## DATATYPE ## _decompose
#else
#define decompose_2d(DATATYPE) bn_wlt_2d_/* */DATATYPE/* */_decompose
#endif

#define make_wlt_2d_decompose(DATATYPE) \
void \
decompose_2d(DATATYPE) \
(tbuffer, buffer, dimen, channels, limit) \
DATATYPE *tbuffer; \
DATATYPE *buffer; \
unsigned long dimen; \
unsigned long channels; \
unsigned long limit; \
{ \
	register DATATYPE *detail; \
	register DATATYPE *avg; \
	register DATATYPE *ptr; \
	unsigned long img_size; \
	unsigned long half_size; \
	unsigned long x, y, x_tmp, y_tmp, d, i, j; \
	int do_free = 0; \
	register fastf_t onehalf = (fastf_t)0.5; \
\
	CK_POW_2( dimen ); \
\
	if ( ! tbuffer ) { \
		tbuffer = (DATATYPE *)bu_malloc( \
				(dimen/2) * channels * sizeof( *buffer ), \
				"1d wavelet buffer"); \
		do_free = 1; \
	} \
\
	/* each iteration of this loop decomposes the data into 4 quarters: \
	 * the "average image", the horizontal detail, the vertical detail \
	 * and the horizontal-vertical detail \
	 */ \
	for (img_size = dimen ; img_size > limit ; img_size = half_size ) { \
		half_size = img_size/2; \
\
		/* do a horizontal detail decomposition first */ \
		for (y=0 ; y < img_size ; y++ ) { \
			y_tmp = y * dimen * channels; \
\
			detail = tbuffer; \
			avg = &buffer[y_tmp]; \
\
			for (x=0 ; x < img_size ; x += 2 ) { \
				x_tmp = x*channels + y_tmp; \
\
				for (d=0 ; d < channels ; d++, avg++, detail++){ \
					i = x_tmp + d; \
					j = i + channels; \
					*detail = (buffer[i] - buffer[j]) * onehalf; \
					*avg    = (buffer[i] + buffer[j]) * onehalf; \
				} \
			} \
			/* "avg" now points to the first element AFTER the \
			 * "average image" section, and hence is the START \
			 * of the "image detail" portion.  Convenient, since \
			 * we now want to copy the contents of "tbuffer" (which \
			 * holds the image detail) into place. \
			 */ \
			memcpy(avg, tbuffer, sizeof(*buffer) * channels * half_size); \
		} \
\
		/* Now do the vertical decomposition */ \
		for (x=0 ; x < img_size ; x ++ ) { \
			x_tmp = x*channels; \
\
			detail = tbuffer; \
			avg = &buffer[x_tmp]; \
\
			for (y=0 ; y < img_size ; y += 2) { \
				y_tmp =y*dimen*channels + x_tmp; \
\
				for (d=0 ; d < channels ; d++, avg++, detail++) { \
					i = y_tmp + d; \
					j = i + dimen*channels; \
					*detail = (buffer[i] - buffer[j]) * onehalf; \
					*avg    = (buffer[i] + buffer[j]) * onehalf; \
				} \
				avg += (dimen-1)*channels; \
			} \
\
			/* "avg" now points to the element ABOVE the \
			 * last "average image" pixel or the first "detail" \
			 * location in the user buffer. \
			 * \
			 * There is no memcpy for the columns, so we have to \
			 * copy the data back to the user buffer ourselves. \
			 */ \
			detail = tbuffer; \
			for (y=half_size ; y < img_size ; y++) { \
				for (d=0; d < channels ; d++) { \
					*avg++ = *detail++; \
				} \
				avg += (dimen-1)*channels; \
			} \
		} \
	} \
}

#define make_wlt_2d_reconstruct(DATATYPE) /* DATATYPE */

make_wlt_2d_decompose(double)
make_wlt_2d_decompose(float)
make_wlt_2d_decompose(char)
make_wlt_2d_decompose(int)
make_wlt_2d_decompose(short)
make_wlt_2d_decompose(long)

