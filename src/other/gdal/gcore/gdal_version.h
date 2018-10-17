/* $Id: gdal_version.h 39236 2017-06-23 11:26:14Z rouault $ */

/* -------------------------------------------------------------------- */
/*      GDAL Version Information.                                       */
/* -------------------------------------------------------------------- */

#ifndef GDAL_VERSION_MAJOR
#  define GDAL_VERSION_MAJOR    2
#  define GDAL_VERSION_MINOR    2
#  define GDAL_VERSION_REV      1
#  define GDAL_VERSION_BUILD    0
#endif

/* GDAL_COMPUTE_VERSION macro introduced in GDAL 1.10 */
/* Must be used ONLY to compare with version numbers for GDAL >= 1.10 */
#ifndef GDAL_COMPUTE_VERSION
#define GDAL_COMPUTE_VERSION(maj,min,rev) ((maj)*1000000+(min)*10000+(rev)*100)
#endif

/* Note: the formula to compute GDAL_VERSION_NUM has changed in GDAL 1.10 */
#ifndef GDAL_VERSION_NUM
#  define GDAL_VERSION_NUM      (GDAL_COMPUTE_VERSION(GDAL_VERSION_MAJOR,GDAL_VERSION_MINOR,GDAL_VERSION_REV)+GDAL_VERSION_BUILD)
#endif

#ifndef GDAL_RELEASE_DATE
#  define GDAL_RELEASE_DATE     20170623
#endif
#ifndef GDAL_RELEASE_NAME
#  define GDAL_RELEASE_NAME     "2.2.1"
#endif
