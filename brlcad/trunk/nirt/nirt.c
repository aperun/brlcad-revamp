/*                        NIRT
 *
 *       This program is an Interactive Ray-Tracer
 *       
 *
 *       Written by:  Natalie L. Barker <barker@brl>
 *                    U.S. Army Ballistic Research Laboratory
 *
 *       Date:  Jan 90 -
 * 
 *       To compile:  /bin/cc -I/usr/include/brlcad nirt.c 
 *                    /usr/brlcad/lib/librt.a -lm -o nirt
 *
 *       To run:  nirt [-options] file.g object[s] 
 *
 *       Help menu:  nirt -?
 *
 */
#ifndef lint
static char RCSid[] = "$Header$";
#endif

/*	INCLUDES	*/
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <machine.h>
#include <vmath.h>
#include <raytrace.h>
#include "./nirt.h"
#include "./usrfmt.h"

extern void	cm_libdebug();

char		*db_name;	/* the name of the MGED file      */
com_table	ComTab[] =
		{
		    { "ae", az_el, "set/query azimuth and elevation", 
			"azimuth elevation" },
		    { "dir", dir_vect, "set/query direction vector", 
			"x-component y-component z-component" },
		    { "hv", grid_coor, "set/query gridplane coordinates",
			"horz vert [dist]" },
		    { "xyz", target_coor, "set/query target coordinates", 
			"X Y Z" },
		    { "s", shoot, "shoot a ray at the target" },
		    { "useair", use_air, "set/query use of air",
			"<0|1|2|...>" },
		    { "units", nirt_units, "set/query local units",
			"<mm|cm|m|in|ft>" },
		    { "fmt", format_output, "set/query output formats",
			"{rhpfmo} format item item ..." },
		    { "dest", direct_output, "set/query output destination",
			"file/pipe" },
		    { "statefile", state_file,
			"set/query name of state file", "file" },
		    { "dump", dump_state,
			"write current state of NIRT to the state file" },
		    { "load", load_state,
			"read new state for NIRT from the state file" },
		    { "print", print_item, "query an output item",
			"item" },
		    { "libdebug", cm_libdebug,
			"set/query librt debug flags", "hex_flag_value" },
		    { "!", sh_esc, "escape to the shell" },
		    { "q", quit, "quit" },
		    { "?", show_menu, "display this help menu" },
		    { 0 }
		};

/* Parallel structures needed for operation w/ and w/o air */
struct rt_i		*rti_tab[2];
struct rt_i		*rtip;
struct resource		res_tab[2];

struct application	ap;
struct nirt_obj		object_list = {"", 0};

main (argc, argv)
int argc;
char **argv;
{
    char                db_title[TITLE_LEN+1];/* title from MGED file      */
    extern char		*local_unit[];
    extern char		local_u_name[];
    extern double	base2local;
    extern double	local2base;
    FILE		*fPtr;
    int                 i;               /* counter                       */
    int			Ch;		/* Option name */
    int			use_of_air = 0;
    outval		*vtp;
    extern outval	ValTab[];
    extern int 		optind;		/* index from getopt(3C) */
    extern char		*optarg;	/* argument from getopt(3C) */

    /* FUNCTIONS */
    int                    if_overlap();    /* routine if you overlap         */
    int             	   if_hit();        /* routine if you hit target      */
    int             	   if_miss();       /* routine if you miss target     */
    void                   interact();      /* handle user interaction        */
    void                   do_rt_gettree();
    void                   printusage();
    void		   grid2targ();
    void		   targ2grid();
    void		   ae2dir();
    void		   dir2ae();
    double	           dist_default();  /* computes grid[DIST] default val*/
    int	           	   str_dbl();	
    void		   az_el();
    void		   sh_esc();
    void		   grid_coor();
    void		   target_coor();
    void		   dir_vect();
    void		   quit();
    void		   show_menu();
    void		   print_item();
    void		   shoot();

    /* Handle command-line options */
    while ((Ch = getopt(argc, argv, OPT_STRING)) != EOF)
        switch (Ch)
        {
            case 'x':
		sscanf( optarg, "%x", &rt_g.debug );
		break;
            case 'u':
                if (sscanf(optarg, "%d", &use_of_air) != 1)
                {
                    (void) fprintf(stderr,
                        "Illegal use-air specification: '%s'\n", optarg);
                    exit (1);
                }
                break;
            case '?':
	    default:
                printusage();
                exit (Ch != '?');
        }
    if (argc - optind < 2)
    {
	printusage();
	exit (1);
    }
    if (use_of_air && (use_of_air != 1))
    {
	fprintf(stderr,
	    "Warning: useair=%d specified, will set to 1\n", use_of_air);
	use_of_air = 1;
    }
    db_name = argv[optind];

    /* build directory for target object */
    printf("Database file:  '%s'\n", db_name);
    printf("Building the directory...");
    if ((rtip = rt_dirbuild( db_name , db_title, TITLE_LEN )) == RTI_NULL)
    {
	fflush(stdout);
	fprintf(stderr, "Could not load file %s\n", db_name);
	exit(1);
    }
    rti_tab[use_of_air] = rtip;
    rti_tab[1 - use_of_air] = RTI_NULL;
    putchar('\n');
    rtip -> useair = use_of_air;

    printf("Prepping the geometry...");
    while (++optind < argc)    /* prepare the objects that are to be included */
	do_rt_gettree( rtip, argv[optind], 1 );
 
    /* Initialize the table of resource structures */
    res_tab[use_of_air].re_magic =
	(res_tab[1 - use_of_air].re_magic = RESOURCE_MAGIC);

    /* initialization of the application structure */
    ap.a_hit = if_hit;        /* branch to if_hit routine            */
    ap.a_miss = if_miss;      /* branch to if_miss routine           */
    ap.a_overlap = if_overlap;/* branch to if_overlap routine        */
    ap.a_onehit = 0;          /* continue through shotline after hit */
    ap.a_resource = &res_tab[use_of_air];
    ap.a_purpose = "NIRT ray";
    ap.a_rt_i = rtip;         /* rt_i pointer                        */
    ap.a_zero1 = 0;           /* sanity check, sayth raytrace.h      */
    ap.a_zero2 = 0;           /* sanity check, sayth raytrace.h      */

    rt_prep( rtip );

    /* initialize variables */
    azimuth() = 0.0;
    elevation() = 0.0;
    direct(X) = -1.0; 
    direct(Y) = 0.0;
    direct(Z) = 0.0;
    grid(HORZ) = 0.0;
    grid(DIST) = dist_default();     /* extreme of the target */
    grid(VERT) = 0.0;
    grid(DIST) = 0.0;
    grid2targ();

    /* initialize the output specification */
    default_ospec();

    /* initialize NIRT's local units */
    strncpy(local_u_name, local_unit[rtip -> rti_dbip -> dbi_localunit], 64);
    base2local = rtip -> rti_dbip -> dbi_base2local;
    local2base = rtip -> rti_dbip -> dbi_local2base;

    /* Run the run-time configuration file, if it exists */
    if ((fPtr = fopenrc()) != NULL)
    {
	interact(fPtr);
	fclose(fPtr);
    }

    printf("Database title: '%s'\n", db_title);
    printf("Database units: '%s'\n", local_u_name);
    printf("model_min = (%g, %g, %g)    model_max = (%g, %g, %g)\n",
	rtip -> mdl_min[X] * base2local,
	rtip -> mdl_min[Y] * base2local,
	rtip -> mdl_min[Z] * base2local,
	rtip -> mdl_max[X] * base2local,
	rtip -> mdl_max[Y] * base2local,
	rtip -> mdl_max[Z] * base2local);

    /* Perform the user interface */
    interact(stdin);
}
 
#if 0
char	usage[] = "\
Usage: 'nirt [options] model.g objects...'\n\
Options:\n\
 -u n    specifies use of air (default 0)\n\
";
#endif
char	usage[] = "\
Usage: 'nirt [-u n] [-x f] model.g objects...'\n\
";

void printusage() 
{
    (void) fputs(usage, stderr);
}

void do_rt_gettree(rip, object_name, save)
struct rt_i	*rip;
char 		*object_name;
int		save;		/* Add object_name to object_list? */
{
    static struct nirt_obj	*op = &object_list;

    if (rt_gettree( rip, object_name ) == -1)
    {
	fflush(stdout);
	fprintf (stderr, "rt_gettree() could not preprocess object '%s'\n", 
	    object_name);
	printusage();
	exit(1);
    }
    if (save)
    {
	if (op == NULL)
	{
	    fputs("Ran out of memory\n", stderr);
	    exit (1);
	}
	op -> obj_name = object_name;
	op -> obj_next = (struct nirt_obj *) malloc(sizeof(struct nirt_obj));
	op = op -> obj_next;
	if (op != NULL)
	    op -> obj_next = NULL;
    }
    putchar('\n');
    printf("Object '%s' processed\n", object_name);
}
