/***************************************************************************/
/*                                                                         */
/*  pshnterr.h                                                             */
/*                                                                         */
/*    PS Hinter error codes (specification only).                          */
/*                                                                         */
/*  Copyright 2003-2016 by                                                 */
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
  /* This file is used to define the PSHinter error enumeration constants. */
  /*                                                                       */
  /*************************************************************************/

#ifndef PSHNTERR_H_
#define PSHNTERR_H_

#include FT_MODULE_ERRORS_H

#undef FTERRORS_H_

#undef  FT_ERR_PREFIX
#define FT_ERR_PREFIX  PSH_Err_
#define FT_ERR_BASE    FT_Mod_Err_PShinter

#include FT_ERRORS_H

#endif /* PSHNTERR_H_ */


/* END */
