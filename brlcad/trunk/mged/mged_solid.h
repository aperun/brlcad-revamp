/*
 *	Solids structure definition
 */
#define MAX_PATH	8	/* Maximum depth of path */
struct solid  {
	float	s_size;		/* Distance across solid, in model space */
	vect_t	s_center;	/* Center point of solid, in model space */
	unsigned s_addr;	/* Display processor's core address */
	unsigned s_bytes;	/* Display processor's core length */
	struct veclist *s_vlist;/* Pointer to vector list */
	int	s_vlen;		/* Length of vector list (structs) */
	struct directory *s_path[MAX_PATH];	/* Full `path' name */
	char	s_last;		/* index of last path element */
	char	s_flag;		/* UP = object visible, DOWN = obj invis */
	char	s_iflag;	/* UP = illuminated, DOWN = regular */
	char	s_soldash;	/* solid/dashed line flag */
	char	s_Eflag;	/* flag - not a solid but an "E'd" region */
	struct solid *s_forw;	/* forward link */
	struct solid *s_back;	/* backward link */
};
#define SOLID_NULL	((struct solid *)0)
extern struct solid	*FreeSolid;	/* Head of freelist */
extern struct solid	HeadSolid;	/* Head of doubly linked solid tab */
extern int		ndrawn;

#define GET_SOLID(p)    { if( ((p)=FreeSolid) == SOLID_NULL )  { \
			p = (struct solid *)malloc(sizeof(struct solid)); \
			  if( p == SOLID_NULL )  {\
				printf("GETSOLID: malloc failed\n"); \
				no_memory = 2; \
			  } \
			} else { \
				FreeSolid = (p)->s_forw; \
			} }
#define FREE_SOLID(p) {(p)->s_forw = FreeSolid; FreeSolid = (p); \
	if((p)->s_vlist) (void)free((char *)(p)->s_vlist); \
	(p)->s_vlist = 0; }

#define FOR_ALL_SOLIDS(p)  \
	for( p=HeadSolid.s_forw; p != &HeadSolid; p = p->s_forw )

/* Insert "new" solid in front of "old" solid */
#define INSERT_SOLID(new,old)	{ \
	(new)->s_back = (old)->s_back; \
	(old)->s_back = (new); \
	(new)->s_forw = (old); \
	(new)->s_back->s_forw = (new);  }

/* Append "new" solid after "old" solid */
#define APPEND_SOLID(new,old)	{ \
	(new)->s_forw = (old)->s_forw; \
	(new)->s_back = (old); \
	(old)->s_forw = (new); \
	(new)->s_forw->s_back = (new);  }

/* Dequeue "cur" solid from doubly-linked list */
#define DEQUEUE_SOLID(cur)	{ \
	(cur)->s_forw->s_back = (cur)->s_back; \
	(cur)->s_back->s_forw = (cur)->s_forw;  }
