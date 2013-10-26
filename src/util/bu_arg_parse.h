/*                            B U . H
 * BRL-CAD
 *
 * Copyright (c) 2004-2013 United States Government as represented by
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

#ifndef BU_ARG_PARSE_H
#define BU_ARG_PARSE_H

#include "common.h"

#ifdef HAVE_STRING_H
#  include <string.h>
#endif

#include "bu.h"

#define BU_ARG_PARSE_BUFSZ 256

enum { BU_TRUE  = 1 };
enum { BU_FALSE = 0 };

enum { BU_ARG_MAGIC = 0x2189165c }; /**< arg structs */

/* all in this part of the header MUST have "C" linkage */
#ifdef __cplusplus
extern "C" {
#endif

/* using ideas from Cliff */
/* types of parse arg results */
typedef enum {
  BU_ARG_PARSE_SUCCESS = 0,
  BU_ARG_PARSE_ERR
} bu_arg_parse_result_t;

/* types of arg values */
typedef enum {
  BU_ARG_UNDEFINED,
  BU_ARG_BOOL,
  BU_ARG_DOUBLE,
  BU_ARG_LONG,
  BU_ARG_STRING
} bu_arg_valtype_t;

/* TCLAP arg types */
typedef enum {
  BU_ARG_MultiArg,
  BU_ARG_MultiSwitchArg,
  BU_ARG_SwitchArg,
  BU_ARG_UnlabeledMultiArg,
  BU_ARG_UnlabeledValueArg,
  BU_ARG_ValueArg
} bu_arg_t;

typedef enum {
  BU_ARG_NOT_REQUIRED = 0,
  BU_ARG_REQUIRED = 1
} bu_arg_req_t;

/* OBSOLETE
typedef struct bu_arg_value {
  bu_arg_valtype_t typ;
  union u_typ {

    long l;
    bu_vls_t s;
    double d;
  } u;
} bu_arg_value_t;
*/

typedef struct bu_arg_value4 {
  union u_typ4 {
    /* important that first field is integral type */
    long l; /* also use as bool */
    double d;
  } u4;
  char buf[BU_ARG_PARSE_BUFSZ];
} bu_arg_value4_t;

/* TCLAP::Arg */
/* OBSOLETE
typedef struct bu_arg_vars_type {
  bu_arg_t arg_type;
  bu_vls_t flag;
  bu_vls_t name;
  bu_vls_t desc;
  bu_arg_req_t req;
  bu_arg_req_t valreq;
  bu_arg_value_t val;
} bu_arg_vars;
*/

/* initialization */
/* OBSOLETE
bu_arg_vars *
bu_arg_switch(const char *flag,
              const char *name,
              const char *desc,
              const char *def_val);
bu_arg_vars *
bu_arg_unlabeled_value(const char *name,
                       const char *desc,
                       const char *def_val,
                       const bu_arg_req_t required,
                       const bu_arg_valtype_t val_typ);
*/

/* structs for static initialization */
/* TCLAP::Arg */
  /* OBSOLETE
typedef struct bu_arg_vars_type2 {
  bu_arg_t arg_type;
  const char *flag;
  const char *name;
  const char *desc;
  const bu_arg_req_t req;
  const bu_arg_valtype_t val_typ;
  const char *def_val;
} bu_arg_vars2;
*/

/* use this struct to cast an unknown bu_arg_* type for data access */
typedef struct {
  uint32_t magic;                  	 /* BU_ARG_MAGIC                              */
  bu_arg_t arg_type;                     /* enum: type of TCLAP arg                   */
  /* return data */
  bu_arg_value4_t retval;                /* use for data transfer with TCLAP code     */
} bu_arg_general_t;

typedef struct {
  uint32_t magic;                  	 /* BU_ARG_MAGIC                              */
  bu_arg_t arg_type;                     /* enum: type of TCLAP arg                   */
  /* return data */
  bu_arg_value4_t retval;                /* use for data transfer with TCLAP code     */

  const char *flag;                      /* the "short" option, may be empty ("")     */
  const char *name;                      /* the "long" option                         */
  const char *desc;                      /* a brief description                       */
  int def_val;                           /* always false on init                      */
} bu_arg_switch_t;

typedef struct {
  uint32_t magic;                  	 /* BU_ARG_MAGIC                              */
  bu_arg_t arg_type;                     /* enum: type of TCLAP arg                   */
  /* return data */
  bu_arg_value4_t retval;                /* use for data transfer with TCLAP code     */

  const char *flag;                      /* the "short" option, may be empty ("")     */
  const char *name;                      /* the "long" option                         */
  const char *desc;                      /* a brief description                       */
  bu_arg_req_t req;                      /* bool: is arg required?                    */
  bu_arg_valtype_t val_typ;              /* enum: value type                          */
  const char *def_val;                   /* default value (if any)                    */
} bu_arg_unlabeled_value_t;

#define BU_ARG_SWITCH_INIT(_struct, _flag_str, _name_str, _desc_str) \
 _struct.magic = BU_ARG_MAGIC; \
 _struct.arg_type = BU_ARG_SwitchArg; \
 _struct.flag = _flag_str; \
 _struct.name = _name_str; \
 _struct.desc = _desc_str; \
 _struct.def_val = BU_FALSE; \
 memset(_struct.retval.buf, 0, sizeof(char) * BU_ARG_PARSE_BUFSZ);

#define BU_ARG_UNLABELED_VALUE_INIT(_struct, _flag_str, _name_str, _desc_str, \
                        _required_bool, _val_typ, _def_val_str) \
 _struct.magic = BU_ARG_MAGIC; \
 _struct.arg_type = BU_ARG_UnlabeledValueArg; \
 _struct.flag = _flag_str; \
 _struct.name = _name_str; \
 _struct.desc = _desc_str; \
 _struct.req = _required_bool; \
 _struct.val_typ = _val_typ; \
 _struct.def_val = _def_val_str; \
 memset(_struct.retval.buf, 0, sizeof(char) * BU_ARG_PARSE_BUFSZ);

/* the getters (signature should ALMOST stay the same for static and pointer inits) */
  /* OBSOLETE
int bu_arg_get_bool(bu_arg_vars *arg);
long bu_arg_get_long(bu_arg_vars *arg);
double bu_arg_get_double(bu_arg_vars *arg);
const char *bu_arg_get_string(bu_arg_vars *arg);
  */

/* but use tmp names while dual/triple/quadruple use in effect */
/* using file transfer */
  /* OBSOLETE

int bu_arg_get_bool2(void *arg);
long bu_arg_get_long2(void *arg);
double bu_arg_get_double2(void *arg);
void bu_arg_get_string2(void *arg, char buf[], const size_t buflen);
  */

/* using stack buf transfer */
int bu_arg_get_bool4(void *arg);
long bu_arg_get_long4(void *arg);
double bu_arg_get_double4(void *arg);
void bu_arg_get_string4(void *arg, char buf[]);

/* the action: all in one function */
  /* OBSOLETE
int bu_arg_parse(bu_ptbl_t *args, int argc, char * const argv[]);
  */

/* for use with static struct init (tmp name) and file transfer */
  /* OBSOLETE
int bu_arg_parse2(void *args[], int argc, char * const argv[]);
  */

/* for use with static struct init (tmp name) and stack buf transfer */
int bu_arg_parse4(void *args[], int argc, char * const argv[]);

/* free arg memory for any strings */
  /* OBSOLETE
void bu_arg_free(bu_ptbl_t *args);
  */

/* exit AND free memory */
  /* OBSOLETE
void bu_arg_exit(const int status, const char *msg, bu_ptbl_t *args);
  */

/* all in this header MUST have "C" linkage */
#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* BU_ARG_PARSE_H */
