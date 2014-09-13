/***************************************************************************/
/*                                                                         */
/*  ftsmerrs.h                                                             */
/*                                                                         */
/*    smooth renderer error codes (specification only).                    */
/*                                                                         */
/*  Copyright 2001, 2012 by                                                */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* This file is used to define the smooth renderer error enumeration     */
  /* constants.                                                            */
  /*                                                                       */
  /*************************************************************************/

#ifndef __FTSMERRS_H__
#define __FTSMERRS_H__

#include FT_MODULE_ERRORS_H

#undef __FTERRORS_H__

#undef  FT_ERR_PREFIX
#define FT_ERR_PREFIX  Smooth_Err_
#define FT_ERR_BASE    FT_Mod_Err_Smooth

#include FT_ERRORS_H

#endif /* __FTSMERRS_H__ */


/* END */
