#ifndef SEEN_DM_PLOT
#define SEEN_DM_PLOT

/*
 * Display coordinate conversion:
 *  GED is using -2048..+2048,
 *  and we define the PLOT file to use the same space.  Easy!
 */
#define	GED_TO_PLOT(x)	(x)
#define PLOT_TO_GED(x)	(x)

struct plot_vars {
  struct bu_list l;
  FILE *up_fp;
  char ttybuf[BUFSIZ];
  int floating;
  int zclip;
  int grid;
  int is_3D;
  int is_pipe;
  int debug;
  vect_t clipmin;
  vect_t clipmax;
  struct bu_vls vls;
};

extern struct plot_vars head_plot_vars;

#endif /* SEEN_DM_PLOT */


/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
