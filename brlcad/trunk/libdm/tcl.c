#include "conf.h"
#include "tcl.h"
#include "machine.h"
#include "externs.h"
#include "bu.h"
#include "dm.h"

extern int dm_best_type();
extern char *dm_best_name();
extern int dm_name2type();
extern char **dm_names();
extern char *dm_type2name();
extern int *dm_types();

static int dm_best_type_tcl();
static int dm_best_name_tcl();
static int dm_name2type_tcl();
static int dm_names_tcl();
static int dm_type2name_tcl();
static int dm_types_tcl();

struct cmdtab {
  char *ct_name;
  int (*ct_func)();
};

static struct cmdtab cmdtab[] = {
  "dm_best_type", dm_best_type_tcl,
  "dm_best_name", dm_best_name_tcl,
  "dm_name2type", dm_name2type_tcl,
  "dm_names", dm_names_tcl,
  "dm_type2name", dm_type2name_tcl,
  "dm_types", dm_types_tcl,
  0, 0
};

int
dm_Tcl_Init(interp)
Tcl_Interp *interp;
{
  register struct cmdtab *ctp;

  for(ctp = cmdtab; ctp->ct_name != NULL; ctp++){
    (void)Tcl_CreateCommand(interp, ctp->ct_name, ctp->ct_func,
			   (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
  }

  return TCL_OK;
}

int
dm_best_type_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  struct bu_vls vls;

  bu_vls_init(&vls);

  if(argc != 1){
    bu_vls_printf(&vls, "help dm_best_type");
    Tcl_Eval(interp, bu_vls_addr(&vls));
    bu_vls_free(&vls);
    return TCL_ERROR;
  }

  bu_vls_printf(&vls, "%d", dm_best_type());
  Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
  bu_vls_free(&vls);

  return TCL_OK;
}

int
dm_best_name_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  if(argc != 1){
    struct bu_vls vls;

    bu_vls_init(&vls);
    bu_vls_printf(&vls, "help dm_best_type");
    Tcl_Eval(interp, bu_vls_addr(&vls));
    bu_vls_free(&vls);
    return TCL_ERROR;
  }

  Tcl_AppendResult(interp, dm_best_name(), (char *)NULL);

  return TCL_OK;
}

int
dm_name2type_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  int type;
  struct bu_vls vls;

  bu_vls_init(&vls);

  if(argc != 2){
    bu_vls_printf(&vls, "help dm_name2type");
    Tcl_Eval(interp, bu_vls_addr(&vls));
    bu_vls_free(&vls);
    return TCL_ERROR;
  }

  type = dm_name2type(argv[1]);
  bu_vls_printf(&vls, "%d", type);
  Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
  bu_vls_free(&vls);

  return TCL_OK;
}

int
dm_names_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  struct bu_vls vls;
  char **names;
  char **cpp;

  bu_vls_init(&vls);

  if(argc != 1){
    bu_vls_printf(&vls, "help dm_names");
    Tcl_Eval(interp, bu_vls_addr(&vls));
    bu_vls_free(&vls);
    return TCL_ERROR;
  }

  names = dm_names();
  bu_vls_printf(&vls, "{");
  for(cpp = names; *cpp != (char *)NULL; ++cpp)
    bu_vls_printf(&vls, " %s", *cpp);

  bu_vls_printf(&vls, " }");
  Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
  bu_vls_free(&vls);
  bu_free((genptr_t)names, "dm_names_tcl: names");

  return TCL_OK;
}

int
dm_type2name_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  char *name;

  if(argc != 2){
    struct bu_vls vls;

    bu_vls_init(&vls);
    bu_vls_printf(&vls, "help dm_type2name");
    Tcl_Eval(interp, bu_vls_addr(&vls));
    bu_vls_free(&vls);
    return TCL_ERROR;
  }

  name = dm_type2name(atoi(argv[1]));
  Tcl_AppendResult(interp, name, (char *)NULL);

  return TCL_OK;
}

int
dm_types_tcl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  struct bu_vls vls;
  int *types;
  int *ip;

  bu_vls_init(&vls);

  if(argc != 1){
    bu_vls_printf(&vls, "help dm_types");
    Tcl_Eval(interp, bu_vls_addr(&vls));
    bu_vls_free(&vls);
    return TCL_ERROR;
  }

  types = dm_types();
  bu_vls_printf(&vls, "{");
  for(ip = types; *ip != DM_TYPE_BAD; ++ip)
    bu_vls_printf(&vls, " %d", *ip);

  bu_vls_printf(&vls, " }");
  Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
  bu_vls_free(&vls);
  bu_free((genptr_t)types, "dm_types_tcl: types");

  return TCL_OK;
}
