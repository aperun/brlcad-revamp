/*                     U T I L _ R A N D . C
 * BRL-CAD / ADRT
 *
 * Copyright (c) 2002-2007 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file util_rand.c
 *
 *  Comments -
 *      Utilities Library - Rand
 *
 *  Author -
 *      Justin L. Shumaker
 *
 *  Source -
 *      The U. S. Army Research Laboratory
 *      Aberdeen Proving Ground, Maryland  21005-5068  USA
 *
 * $Id$
 */

#include "util_rand.h"
/* Mersenne Twister Algorithm */

/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */


/* Tempering parameters */   
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)


/* the array for the state vector  */
static unsigned long mt[N];
/* mti==N+1 means mt[N] is not initialized */
static int mti=N+1;


/* initializing the array with a NONZERO seed */
void math_rand_seed(unsigned long seed) {
  /*
   * setting initial seeds to mt[N] using
   * the generator Line 25 of Table 1 in
   * [KNUTH 1981, The Art of Computer Programming
   *    Vol. 2 (2nd Ed.), pp102]
   */
  mt[0] = seed & 0xffffffff;
  for(mti = 1; mti<N; mti++)
    mt[mti] = (69069 * mt[mti-1]) & 0xffffffff;
}


double math_rand() {
  unsigned long y;
  static unsigned long mag01[2]={0x0, MATRIX_A};

  /* generate N words at one time */
  if(mti >= N) {
    int kk;

    /* if sgenrand() has not been called, a default initial seed is used */
    if(mti == N+1)
      math_rand_seed(4357);

    for(kk = 0; kk < N-M; kk++) {
      y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
      mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
    }

    for(; kk < N-1; kk++) {
      y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
      mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
    }

    y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
    mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];

    mti = 0;
  }
  
  y = mt[mti++];
  y ^= TEMPERING_SHIFT_U(y);
  y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
  y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
  y ^= TEMPERING_SHIFT_L(y);

  return ( (double)y / (unsigned long)0xffffffff );
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
