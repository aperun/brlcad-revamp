/*
 *			R A Y T R A C E . H
 *
 * All the structures necessary for dealing with the RT ray tracer routines.
 *
 * $Revision$
 */

extern char *malloc();
extern void free();

/*
 *			X R A Y
 *
 * All necessary information about a ray.
 * Not called just "ray" to prevent conflicts with other VLD stuff.
 */
struct xray {
	point_t		r_pt;		/* Point at which ray starts */
	vect_t		r_dir;		/* Direction of ray (UNIT Length) */
	fastf_t		r_min;		/* entry dist to bounding sphere */
	fastf_t		r_max;		/* exit dist from bounding sphere */
	struct xray	*r_forw;	/* !0 -> ray after deflection */
};
#define RAY_NULL	((struct xray *)0)


/*
 *			H I T
 *
 *  Information about where a ray hits the surface
 *
 * Important Note:  Surface Normals always point OUT of a solid.
 */
struct hit {
	point_t		hit_point;	/* Intersection point */
	vect_t		hit_normal;	/* Surface Normal at hit_point */
	fastf_t		hit_dist;	/* dist from r_pt to hit_point */
};
#define HIT_NULL	((struct hit *)0)



/*
 *			S E G
 *
 * Intersection segment.
 *
 * Includes information about both endpoints of intersection.
 * Contains forward link to additional intersection segments
 * if the intersection spans multiple segments (eg, shooting
 * a ray through a torus).
 */
struct seg {
	int		seg_flag;
#define SEG_IN		0x4		/* IN point, normal, computed */
#define SEG_OUT		0x8		/* OUT point, normal, computed */
	struct hit	seg_in;		/* IN information */
	struct hit	seg_out;	/* OUT information */
	struct soltab	*seg_stp;	/* pointer back to soltab */
	struct seg	*seg_next;	/* non-zero if more segments */
};
#define SEG_NULL	((struct seg *)0)
extern struct seg *FreeSeg;		/* Head of freelist */
#define GET_SEG(p)    {	if( ((p)=FreeSeg) == SEG_NULL )  { \
				GETSTRUCT((p), seg); \
			} else { \
				FreeSeg = (p)->seg_next; \
			} \
			p->seg_next = SEG_NULL; }
#define FREE_SEG(p) {(p)->seg_next = FreeSeg; FreeSeg = (p);}


/*
 *			S O L T A B
 *
 * Internal information used to keep track of solids in the model
 */
struct soltab {
	int		st_id;		/* Solid ident */
	vect_t		st_center;	/* Center of bounding Sphere */
	fastf_t		st_radsq;	/* Bounding sphere Radius, squared */
	int		*st_specific;	/* -> ID-specific (private) struct */
	struct soltab	*st_forw;	/* Linked list of solids */
	char		*st_name;	/* Name of solid */
	struct region	*st_regionp;	/* Pointer to containing region */
	vect_t		st_min;		/* min X, Y, Z of bounding RPP */
	vect_t		st_max;		/* max X, Y, Z of bounding RPP */
	int		st_bin;		/* Temporary for boolean processing */
};
#define SOLTAB_NULL	((struct soltab *)0)

/*
 *  Values for Solid ID.
 */
#define ID_NULL		0	/* Unused */
#define ID_TOR		1	/* Toroid */
#define ID_TGC		2	/* Generalized Truncated General Cone */
#define ID_ELL		3	/* Ellipsoid */
#define ID_ARB8		4	/* Generalized ARB.  V + 7 vectors */
#define ID_ARS		5	/* ARS */
#define ID_HALF		6	/* Half-space */
#define ID_REC		7	/* Right Elliptical Cylinder [TGC special] */

struct functab {
	int		(*ft_prep)();
	struct seg 	*((*ft_shot)());
	int		(*ft_print)();
	char		*ft_name;
};
extern struct functab functab[];

#define EPSILON		0.0001
#define NEAR_ZERO(f)	( ((f) < 0) ? ((f) > -EPSILON) : ((f) < EPSILON) )
#define INFINITY	100000000.0


/*
 *			T R E E
 *
 *  Binary trees representing the Boolean operations which
 *  combine one or more solids into a region.
 */
#define MKOP(x)		((x)<<1)
#define OP_BINARY	0x01			/* Binary/Unary (leaf) flag */
#define BINOP(x)	((x)->tr_op & OP_BINARY)

#define OP_SOLID	MKOP(1)			/* Leaf:  tr_stp -> solid */
#define OP_UNION	MKOP(2)|OP_BINARY	/* Binary: L union R */
#define OP_INTERSECT	MKOP(3)|OP_BINARY	/* Binary: L intersect R */
#define OP_SUBTRACT	MKOP(4)|OP_BINARY	/* Binary: L intersect R */

union tree {
	int	tr_op;		/* Operation */
	struct tree_binary {
		int		tb_op;
		union tree	*tb_left;
		union tree	*tb_right;
	} tr_b;
	struct tree_unary {
		int		tu_op;
		struct soltab	*tu_stp;
	} tr_a;
};
#define tr_left		tr_b.tb_left
#define tr_right	tr_b.tb_right
#define tr_stp		tr_a.tu_stp
#define TREE_NULL	((union tree *)0)



/*
 *			R E G I O N
 *
 *  The region structure.
 */
struct region  {
	char		*reg_name;	/* Identifying string */
	union tree	*reg_treetop;	/* Pointer to boolean tree */
	short		reg_regionid;	/* Region ID code;  index to ? */
	short		reg_aircode;	/* ?? */
	short		reg_material;	/* Material */
	short		reg_los;	/* equivalent LOS estimate ?? */
	struct region	*reg_forw;	/* linked list of all regions */
	struct region	*reg_active;	/* linked list of hit regions */
};
#define REGION_NULL	((struct region *)0)



/*
 *  			P A R T I T I O N
 *
 *  Partitions of a ray
 */
#define NBINS	200			/* # bins: # solids hit in ray */

struct partition {
	unsigned char	pt_solhit[NBINS];	/* marks for solids hit */
	struct soltab	*pt_instp;		/* IN solid pointer */
	struct hit	*pt_inhit;		/* IN hit pointer */
	struct soltab	*pt_outstp;		/* OUT solid pointer */
	struct hit	*pt_outhit;		/* OUT hit ptr */
	struct region	*pt_regionp;		/* ptr to containing region */
	struct partition *pt_forw;		/* forwards link */
	struct partition *pt_back;		/* backwards link */
};
#define pt_indist	pt_inhit->hit_dist
#define pt_outdist	pt_outhit->hit_dist

#define PT_NULL	((struct partition *)0)
extern struct partition *FreePart;		 /* Head of freelist */

/* Initialize all the bins to FALSE, clear out structure */
#define GET_PT_INIT(p)	\
	{ GET_PT(p);bzero( ((char *) (p)), sizeof(struct partition) ); }

#define GET_PT(p)   { if( ((p)=FreePart) == PT_NULL )  { \
				GETSTRUCT((p), partition); \
			} else { \
				FreePart = (p)->pt_forw; \
			}  }
#define FREE_PT(p) {(p)->pt_forw = FreePart; FreePart = (p);}

/* Insert "new" partition in front of "old" partition */
#define INSERT_PT(new,old)	{ \
	(new)->pt_back = (old)->pt_back; \
	(old)->pt_back = (new); \
	(new)->pt_forw = (old); \
	(new)->pt_back->pt_forw = (new);  }

/* Append "new" partition after "old" partition */
#define APPEND_PT(new,old)	{ \
	(new)->pt_forw = (old)->pt_forw; \
	(new)->pt_back = (old); \
	(old)->pt_forw = (new); \
	(new)->pt_forw->pt_back = (new);  }

/* Dequeue "cur" partition from doubly-linked list */
#define DEQUEUE_PT(cur)	{ \
	(cur)->pt_forw->pt_back = (cur)->pt_back; \
	(cur)->pt_back->pt_forw = (cur)->pt_forw;  }

/*
 *		A P P L I C A T I O N
 */
struct application  {
	/* THESE ELEMENTS ARE MANDATORY */
	struct xray a_ray;	/* Actual ray to be shot */
	int	(*a_hit)();	/* routine to call when shot hits model */
	int	(*a_miss)();	/* routine to call when shot misses */
	int	a_x;		/* Screen X of ray, where applicable */
	int	a_y;		/* Screen Y of ray, where applicable */
	/* THE FOLLOWING ROUTINES ARE MAINLINE & APPLICATION SPECIFIC */
	int	a_user;		/* application-specific value */
	int	(*a_init)();	/* routine to init application */
	int	(*a_eol)();	/* routine for end of scan-line */
	int	(*a_end)();	/* routine to end application */
};

/*
 *  Global variables used by the RT library.
 */
extern int ged_fd;		/* fd of object file */
extern int one_hit_flag;	/* non-zero to return first hit only */
extern int debug;		/* non-zero for debugging, see debug.h */
extern long nsolids;		/* total # of solids participating */
extern long nregions;		/* total # of regions participating */
extern long nshots;		/* # of ray-meets-solid "shots" */
extern long nmiss;		/* # of ray-misses-solid's-sphere "shots" */
extern struct soltab *HeadSolid;/* pointer to list of solids in model */

/*
 *  Global routines to interface with the RT library.
 */
extern void get_tree();			/* Get expr tree for object */
extern void shootray();			/* Shoot a ray */
extern void rtbomb();			/* Exit with error message */
extern void timer_prep();		/* Start the timer */
extern double timer_print();		/* Stop timer, print, return time */
extern void dir_build();		/* Read named GED db, build toc */
extern void pr_seg();				/* Print seg struct */

/* The matrix math routines */
extern void mat_zero(), mat_idn(), mat_copy(), mat_mul(), matXvec();
extern void mat_inv(), mat_trn(), mat_ae(), mat_angles();
extern void vtoh_move(), htov_move(), mat_print();

/*
 *  Internal routines in RT library.
 *  Not for general use.
 */
extern struct directory *dir_lookup();	/* Look up name in toc */
extern struct directory *dir_add();	/* Add name to toc */
extern char *strdup();			/* Duplicate str w/malloc */
extern void bool_regions();		/* Eval booleans */
extern int bool_eval();			/* Eval bool tree node */
extern int fdiff();			/* Approx Floating compare */
extern double reldiff();		/* Relative Difference */
extern void pr_bins();			/* Print bins */
extern void pr_region();		/* Print a region */
extern void pr_tree();			/* Print an expr tree */
extern void fastf_float();		/* convert float->fastf_t */

/*
 *  Library routines used by the RT library.
 */
extern long	lseek();
extern int	read(), write();
extern char	*malloc();
extern void	free();
