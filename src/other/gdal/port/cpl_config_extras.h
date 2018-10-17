/* $Id: cpl_config_extras.h 37003 2016-12-23 14:54:07Z goatbar $ */

#ifndef INCLUDED_CPL_CONFIG_EXTRAS
#define INCLUDED_CPL_CONFIG_EXTRAS

#if defined(__APPLE__)

#ifdef __BIG_ENDIAN__
  #define HOST_FILLORDER FILLORDER_MSB2LSB
#else
  #define HOST_FILLORDER FILLORDER_LSB2MSB
#endif

#ifdef __LP64__
  #define SIZEOF_UNSIGNED_LONG 8
#else
  #define SIZEOF_UNSIGNED_LONG 4
#endif

#ifdef __LP64__
  #define SIZEOF_VOIDP 8
#else
  #define SIZEOF_VOIDP 4
#endif

#ifdef __BIG_ENDIAN__
  #define WORDS_BIGENDIAN 1
#else
  #undef WORDS_BIGENDIAN
#endif

#undef VSI_STAT64
#undef VSI_STAT64_T

#define VSI_STAT64 stat
#define VSI_STAT64_T stat

#endif // APPLE

#endif // INCLUDED_CPL_CONFIG_EXTRAS
