/* rle_config.h
 * 
 * Automatically generated by make-config-h script.
 * DO NOT EDIT THIS FILE.
 * Edit include/makefile.src and the configuration file instead.
 */

#define USE_STDARG 1
#define USE_PROTOTYPES 1

#define X11 X11
#define ABEKASA60 ABEKASA60
#define ABEKASA62 ABEKASA62
#define ALIAS ALIAS
#define CUBICOMP CUBICOMP
#define CONST_DECL const
#define GIF GIF
#define GRAYFILES GRAYFILES
#define MACPAINT MACPAINT
#define POSTSCRIPT POSTSCRIPT
#define TARGA TARGA
#define TIFF2p4 TIFF2p4
#define VICAR VICAR
#define WASATCH WASATCH
#define WAVEFRONT WAVEFRONT
#define GCC GCC
#define HPUX800CC HPUX800CC
#define NO_RANLIB NO_RANLIB
#define SYS_V_SETPGRP SYS_V_SETPGRP
#define USE_SHARED_LIB USE_SHARED_LIB
#define USE_STRING_H USE_STRING_H
#define USE_TIME_H USE_TIME_H
#define VOID_STAR VOID_STAR
/* -*-C-*- */
/***************** From rle_config.tlr *****************/

/* CONST_DECL must be defined as 'const' or nothing. */
#ifdef CONST_DECL
#undef CONST_DECL
#define CONST_DECL const

#else
#define CONST_DECL

#endif

/* A define for getx11. */
#ifndef USE_XLIBINT_H
#define XLIBINT_H_NOT_AVAILABLE
#endif

/* Typedef for void * so we can use it consistently. */
#ifdef VOID_STAR
typedef	void *void_star;
#else
typedef char *void_star;
#endif

#ifdef USE_STDLIB_H
#include <stdlib.h>
#else

/* Some programs include files from other packages that also declare
 * malloc.  Avoid double declaration by #define NO_DECLARE_MALLOC
 * before including this file.
 */
#ifndef NO_DECLARE_MALLOC
#ifdef USE_PROTOTYPES
#   include <sys/types.h>	/* For size_t. */
    extern void_star malloc( size_t );
    extern void_star calloc( size_t, size_t );
    extern void_star realloc( void_star, size_t );
    extern void free( void_star );
#else
    extern void_star malloc();
    extern void_star realloc();
    extern void_star calloc();
    extern void free();
    extern void cfree();
#endif /* USE_PROTOTYPES */
#endif /* NO_DECLARE_MALLOC */

#ifdef USE_PROTOTYPES
extern char *getenv( CONST_DECL char *name );
#else
extern char *getenv();
#endif

#endif /* USE_STDLIB_H */

#ifdef USE_STRING_H
    /* SYS V string routines. */
#   include <string.h>
#   define index strchr
#   define rindex strrchr
#else
    /* BSD string routines. */
#   include <strings.h>
/* Really, should define USE_STRING_H if __STDC__, but be safe. */
#ifdef __STDC__
#   define index strchr
#   define rindex strrchr
#endif /* __STDC__ */
#endif /* USE_STRING_H */

#ifdef NEED_BSTRING
    /* From bstring.c. */
    /*****************************************************************
     * TAG( bstring bzero )
     * 'Byte string' functions.
     */
#   define bzero( _str, _n )		memset( _str, '\0', _n )
#   define bcopy( _from, _to, _count )	memcpy( _to, _from, _count )
#endif

#ifdef NEED_SETLINEBUF
#   define setlinebuf( _s )	setvbuf( (_s), NULL, _IOLBF, 0 )
#endif

/* include common.h so that HAVE_ symbols may be used to work around
 * compilation support issues.  e.g. sys_errlist on solaris. 
 */
#include "common.h"

