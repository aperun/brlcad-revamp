/*
 *			E U C L I D - G . C
 *
 *  Program to convert Euclid file into a BRL-CAD NMG object.
 *
 *  Authors -
 *	Michael Markowski
 *	John R. Anderson
 *  
 *  Source -
 *	The US Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Distribution Status -
 *	Public Domain, Distribution Unlimitied.
 */
#ifndef lint
static char RCSid[] = "$Header$";
#endif

#include "conf.h"

#include <stdio.h>
#include <math.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "machine.h"
#include "externs.h"
#include "vmath.h"
#include "nmg.h"
#include "raytrace.h"
#include "rtgeom.h"
#include "rtlist.h"
#include "wdb.h"
#include "../librt/debug.h"

#define BRLCAD_TITLE_LENGTH	72
#define MAX_FACES_PER_REGION	8192
#define MAX_PTS_PER_FACE	8192

#define MAX(A, B) ((A) > (B) ? (A) : (B))

/* macro to determine if one bounding box in entirely within another
 * also returns true if the boxes are the same
 */
#define V3RPP1_IN_RPP2( _lo1 , _hi1 , _lo2 , _hi2 )	( \
	(_lo1)[X] >= (_lo2)[X] && (_hi1)[X] <= (_hi2)[X] && \
	(_lo1)[Y] >= (_lo2)[Y] && (_hi1)[Y] <= (_hi2)[Y] && \
	(_lo1)[Z] >= (_lo2)[Z] && (_hi1)[Z] <= (_hi2)[Z] )

#define inout(_i,_j)	in_out[_i*shell_count + _j]
#define OUTSIDE		1
#define INSIDE		(-1)
#define OUTER_SHELL	0x00000001
#define	INNER_SHELL	0x00000002
#define INVERT_SHELL	0x00000004


RT_EXTERN( fastf_t nmg_loop_plane_area , ( struct loopuse *lu , plane_t pl ) );
RT_EXTERN( struct faceuse *nmg_add_loop_to_face , (struct shell *s, struct faceuse *fu, struct vertex *verts[], int n, int dir ) );

extern int errno;

struct vlist {
	fastf_t		pt[3*MAX_PTS_PER_FACE];
	struct vertex	*vt[MAX_PTS_PER_FACE];
};

struct nmg_ptbl groups[11];

static int polysolids;
static int debug;
static char	usage[] = "Usage: %s [-i euclid_db] [-o brlcad_db] [-p] [-xX lvl]\n\t\t(-p indicates write as polysolids)\n ";
static struct rt_tol  tol;

main(argc, argv)
int	argc;
char	*argv[];
{
	char		*bfile, *efile;
	FILE		*fpin, *fpout;
	char		title[BRLCAD_TITLE_LENGTH];	/* BRL-CAD database title */
	register int	c;
	int i;

	fpin = stdin;
	fpout = stdout;
	efile = NULL;
	bfile = NULL;
	polysolids = 0;
	debug = 0;

        tol.magic = RT_TOL_MAGIC;
        tol.dist = 0.005;
        tol.dist_sq = tol.dist * tol.dist;
        tol.perp = 1e-6;
        tol.para = 1 - tol.perp;


	/* Get command line arguments. */
	while ((c = getopt(argc, argv, "d:vi:o:px:X:")) != EOF) {
		switch (c) {
		case 'd':
			tol.dist = atof( optarg );
			tol.dist_sq = tol.dist * tol.dist;
			break;
		case 'v':
			debug = 1;
			break;
		case 'i':
			efile = optarg;
			if ((fpin = fopen(efile, "r")) == NULL)
			{
				fprintf(stderr,	"%s: cannot open %s for reading\n",
					argv[0], efile);
				perror( argv[0] );
				exit(1);
			}
			break;
		case 'o':
			bfile = optarg;
			if ((fpout = fopen(bfile, "w")) == NULL) {
				fprintf(stderr,	"%s: cannot open %s for writing\n",
					argv[0], bfile);
				perror( argv[0] );
				exit(1);
			}
			break;
		case 'p':
			polysolids = 1;
			break;
		case 'x':
			sscanf( optarg, "%x", &rt_g.debug );
			break;
		case 'X':
			sscanf( optarg, "%x", &rt_g.NMG_debug );
			break;
		default:
			fprintf(stderr, usage, argv[0]);
			exit(1);
			break;
		}
	}

	/* Output BRL-CAD database header.  No problem if more than one. */
	if( efile == NULL )
		sprintf( title, "Conversion from EUCLID (tolerance distance = %gmm)", tol.dist );
	else
	{
		char tol_str[BRLCAD_TITLE_LENGTH];
		int title_len,tol_len;

		sprintf( tol_str, " (tolerance distance = %gmm)", tol.dist );
		sprintf( title, "%s", efile );
		title_len = strlen( title );
		tol_len =  strlen( tol_str );
		if( title_len + tol_len > BRLCAD_TITLE_LENGTH )
			strcat( &title[BRLCAD_TITLE_LENGTH-tol_len-1], tol_str );
		else
			strcat( title, tol_str );
	}

	mk_id( fpout, title );

	for( i=0 ; i<11 ; i++ )
		nmg_tbl( &groups[i] , TBL_INIT , (long *)NULL );

	euclid_to_brlcad(fpin, fpout);

	fclose(fpin);
	fclose(fpout);
}

/*
 *	A d d _ N M G _ t o _ D b
 *
 *	Write the nmg to a brl-cad style data base.
 */
static void
add_nmg_to_db(fpout, m, reg_id)
FILE		*fpout;
struct model	*m;
int		reg_id;
{
	char	id[80], *rname, *sname;
	int gift_ident;
	int group_id;
	struct nmgregion *r;
	struct shell *s;
	struct wmember head;

	RT_LIST_INIT( &head.l );

	r = RT_LIST_FIRST( nmgregion , &m->r_hd );
	s = RT_LIST_FIRST( shell , &r->s_hd );

	sprintf(id, "%d", reg_id);
	rname = malloc(sizeof(id) + 3);	/* Region name. */
	sname = malloc(sizeof(id) + 3);	/* Solid name. */

	sprintf(sname, "%s.s", id);
	if( polysolids )
		write_shell_as_polysolid( fpout , sname , s );
	else
	{
		int something_left=1;

		while( RT_LIST_NOT_HEAD( s, &r->s_hd ) )
		{
			struct shell *next_s;

			next_s = RT_LIST_PNEXT( shell, &s->l );
			if( nmg_simplify_shell( s ) )
			{
				if( nmg_ks( s ) )
				{
					/* we killed it all!!! */
					something_left = 0;
					break;
				}
			}
			s = next_s;
		}
		if( something_left )
			mk_nmg(fpout, sname,  m);		/* Make nmg object. */
	}

	gift_ident = reg_id % 100000;
	group_id = gift_ident/1000;
	if( group_id > 10 )
		group_id = 10;

	sprintf(rname, "%s.r", id);

	if( mk_addmember( sname, &head, WMOP_UNION ) == WMEMBER_NULL )
	{
		rt_log( "add_nmg_to_db: mk_addmember failed for solid %s\n" , sname );
		rt_bomb( "add_nmg_to_db: FAILED\n" );
	}

	if( mk_lrcomb( fpout, rname, &head, 1, (char *)NULL, (char *)NULL,
	    (unsigned char *)NULL, gift_ident, 0, 0, 100, 0 ) )
	{
		rt_log( "add_nmg_to_db: mk_rlcomb failed for region %s\n" , rname );
		rt_bomb( "add_nmg_to_db: FAILED\n" );
	}

	nmg_tbl( &groups[group_id] , TBL_INS , (long *)rname );

	rt_free( (char *)sname , "euclid-g: solid name" );
}

static void
build_groups( fpout )
FILE *fpout;
{
	int i,j;
	struct wmember head;
	struct wmember head_all;
	char group_name[18];

	RT_LIST_INIT( &head.l );
	RT_LIST_INIT( &head_all.l );

	for( i=0 ; i<11 ; i++ )
	{

		if( NMG_TBL_END( &groups[i] ) < 1 )
			continue;

		for( j=0 ; j<NMG_TBL_END( &groups[i] ) ; j++ )
		{
			char *region_name;

			region_name = (char *)NMG_TBL_GET( &groups[i] , j );
			if( mk_addmember( region_name , &head , WMOP_UNION ) == WMEMBER_NULL )
			{
				rt_log( "build_groups: mk_addmember failed for region %s\n" , region_name );
				rt_bomb( "build_groups: FAILED\n" );
			}
		}

		if( i < 10 )
			sprintf( group_name , "%dxxx_series" , i );
		else
			sprintf( group_name , "ids_over_9999" );

		j = mk_lfcomb( fpout , group_name , &head , 0 )
		if( j )
		{
			rt_log( "build_groups: mk_lcomb failed for group %s\n" , group_name );
			rt_bomb( "build_groups: mk_lcomb FAILED\n" );
		}

		if( mk_addmember( group_name , &head_all , WMOP_UNION ) == WMEMBER_NULL )
		{
			rt_log( "build_groups: mk_addmember failed for group %s\n" , group_name );
			rt_bomb( "build_groups: FAILED\n" );
		}
	}

	j = mk_lfcomb( fpout , "all" , &head_all , 0 )
	if( j )
		rt_bomb( "build_groups: mk_lcomb failed for group 'all'\n" );
}

/*
 *	E u c l i d _ t o _ B r l C a d
 *
 *	Convert a Euclid data base into a BRL-CAD data base.  This might or
 *	might not be correct, but a Euclid data base of faceted objects is
 *	assumed to be an ascii file of records of the following form:
 *
 *		RID FT A? B? NPT
 *		I0 X0 Y0 Z0
 *		I1 X1 Y1 Z1
 *	    	...
 *		Inpt Xnpt Ynpt Znpt
 *		Ip A B C D
 *
 *	where RID is an integer closed region id number,
 *
 *		FT is the facet type with the following values:
 *		   0: simple facet (no holes).
 *		   1: this facet is a hole.
 *		   2: this is a surface facet which will be given holes.
 *
 *		A? B? are unknown variables.
 *
 *		NPT is the number of data point coordinates which will follow.
 *
 *		Ij is a data point index number.
 *
 *		Xi Yi Zi are data point coordinates in mm.
 *
 *		A, B, C, D are the facet's plane equation coefficients and
 *		<A B C> is an outward pointing surface normal.
 */
euclid_to_brlcad(fpin, fpout)
FILE	*fpin, *fpout;
{
	char	str[80];
	int	reg_id;

	/* skip first string in file (what is it??) */
	if( fscanf( fpin , "%s" , str ) == EOF )
		rt_bomb( "Failed on first attempt to read input" );

	/* Id of first region. */
	if (fscanf(fpin, "%d", &reg_id) != 1) {
		fprintf(stderr, "euclid_to_brlcad: no region id\n");
		exit(1);
	}

	/* Convert each region to an individual nmg. */
	do {
		fprintf( stderr , "Converting region %d...\n", reg_id);
		reg_id = cvt_euclid_region(fpin, fpout, reg_id);
	} while (reg_id != -1);

	/* Build groups based on idents */
	build_groups( fpout );
}

static void
add_shells_to_db( fpout, shell_count, shells, reg_id )
FILE *fpout;
int shell_count;
struct shell *shells[];
int reg_id;
{
	struct model *m=(struct model *)NULL;
	char sol_name[80];
	char *reg_name;
	struct wmember head;
	int solid_no=0;
	int shell_no;
	int gift_ident, group_id;

	RT_LIST_INIT( &head.l );

	for( shell_no=0 ; shell_no < shell_count ; shell_no++ )
	{
		if( !shells[shell_no] )
			continue;

		NMG_CK_SHELL( shells[shell_no] );

		if( !m )
			m = nmg_find_model( &shells[shell_no]->l.magic );

		solid_no++;
		sprintf( sol_name , "%d.%d.s", reg_id, solid_no );

		if( mk_addmember( sol_name, &head, WMOP_UNION ) == WMEMBER_NULL )
		{
			rt_log( "add_shells_to_db: mk_addmember failed for solid %s\n" , sol_name );
			rt_bomb( "add_shells_to_db: FAILED\n" );
		}

		if( polysolids )
			write_shell_as_polysolid( fpout, sol_name, shells[shell_no] );
		else
		{
			struct model *m_tmp;
			struct nmgregion *r_tmp;
			struct shell *s=shells[shell_no];

			/* Move this shell to a seperate model and write to .g file */
			m_tmp = nmg_mmr();
			r_tmp = RT_LIST_FIRST( nmgregion , &m_tmp->r_hd );
			RT_LIST_DEQUEUE( &s->l );
			s->r_p = r_tmp;
			RT_LIST_APPEND( &r_tmp->s_hd, &s->l )

			nmg_m_reindex( m_tmp , 0 );

			nmg_rebound( m_tmp, &tol );

			mk_nmg( fpout , sol_name, m_tmp );

			nmg_km( m_tmp );

		}
	}

	nmg_m_reindex( m , 0 );

	gift_ident = reg_id % 100000;
	group_id = gift_ident/1000;
	if( group_id > 10 )
		group_id = 10;

	reg_name = (char *)rt_malloc( strcspn( sol_name, "." ) + 3, "reg_name" );
	sprintf( reg_name, "%d.r", reg_id );
	if( mk_lrcomb( fpout, reg_name, &head, 1, (char *)NULL, (char *)NULL,
	    (unsigned char *)NULL, gift_ident, 0, 0, 100, 0 ) )
	{
		rt_log( "add_nmg_to_db: mk_rlcomb failed for region %s\n" , reg_name );
		rt_bomb( "add_nmg_to_db: FAILED\n" );
	}

	nmg_tbl( &groups[group_id] , TBL_INS , (long *)reg_name );
}

/*
 *	R e a d _ E u c l i d _ R e g i o n
 *
 *	Make a list of indices into global vertex coordinate array.
 *	This list represents the face under construction.
 */
int
cvt_euclid_region(fp, fpdb, reg_id)
FILE	*fp, *fpdb;
int	reg_id;
{
	int	cur_id, face, facet_type, hole_face, i, j,
		lst[MAX_PTS_PER_FACE], np, nv, shell_count;
	struct faceuse	*outfaceuses[MAX_PTS_PER_FACE];
	struct model	*m;	/* Input/output, nmg model. */
	struct nmgregion *r;
	struct shell	*s;
	struct faceuse	*fu;
	struct vertex	*vertlist[MAX_PTS_PER_FACE];
	struct vlist	vert;

	m = nmg_mm();		/* Make nmg model. */
	r = nmg_mrsv(m);	/* Make region, empty shell, vertex. */
	s = RT_LIST_FIRST(shell, &r->s_hd);

	nv = 0;			/* Initially no vertices for this region. */
	face = 0;		/* No faces either. */
	/* Grab all the faces for one region. */
	do {
		/* Get vertices for a single face. */
		facet_type = read_euclid_face(lst, &np, fp, &vert, &nv);

		if( np > 2 )
		{

			/* Make face out of vertices in lst. */
			for (i = 0; i < np; i++)
				vertlist[i] = vert.vt[lst[i]];

			switch(facet_type) {
			case 0:	/* Simple facet (no holes). */
				outfaceuses[face] = nmg_cface(s, vertlist, np);
				face++;
				break;

			case 1:	/* Facet is a hole. */
				nmg_add_loop_to_face(s, outfaceuses[hole_face],
					vertlist, np, OT_OPPOSITE);
				break;

			case 2:	/* Facet will be given at least one hole. */
				outfaceuses[face] = nmg_cface(s, vertlist, np);
				hole_face = face;
				face++;
				break;

			default:
				fprintf(stderr, "cvt_euclid_face: in region %d, face %d is an unknown facet type\n", reg_id, face);
				break;
			}

			/* Save (possibly) newly created vertex structs. */
			for (i = 0; i < np; i++)
				vert.vt[lst[i]] = vertlist[i];
		}

		/* Get next face's region id. */
		if (fscanf(fp, "%d", &cur_id) != 1)
			cur_id = -1;
	} while (reg_id == cur_id);

	/* Associate the vertex geometry, ccw. */
	if( debug )
		rt_log( "Associating vertex geometry:\n" );
	for (i = 0; i < nv; i++)
	{
		if (vert.vt[i])
		{
			if( debug )
				rt_log( "\t( %g %g %g )\n" , V3ARGS( &vert.pt[3*i] ) );
			nmg_vertex_gv(vert.vt[i], &vert.pt[3*i]);
		}
	}

	/* Associate the face geometry. */
	if( debug )
		rt_log( "Associating face geometry:\n" );
	for (i = 0; i < face; i++)
	{
		/* calculate plane for this faceuse */
		if( nmg_calc_face_g( outfaceuses[i] ) )
		{
			rt_log( "nmg_calc_face_g failed\n" );
			nmg_pr_fu_briefly( outfaceuses[i], "" );
		}
	}

	/* Glue edges of outward pointing face uses together. */
	nmg_gluefaces(outfaceuses, face);

	/* Compute "geometry" for model, region, and shell */
	nmg_rebound( m , &tol );

	/* fix the normals */
	s = RT_LIST_FIRST( shell , &r->s_hd );
	nmg_fix_normals( s, &tol );

	/* verify face plane calculations */
	for (i = 0; i < face; i++)
	{
		plane_t pl;
		struct loopuse *lu;
		struct edgeuse *eu;
		struct vertexuse *vu;
		fastf_t dist_to_plane;
		int triangulate=0;

		NMG_GET_FU_PLANE( pl, outfaceuses[i] );

		/* check if all the vertices for this face lie on the plane */
		for( RT_LIST_FOR( lu, loopuse, &outfaceuses[i]->lu_hd ) )
		{
			NMG_CK_LOOPUSE( lu );

			if( RT_LIST_FIRST_MAGIC( &lu->down_hd ) == NMG_VERTEXUSE_MAGIC )
			{
				vu = RT_LIST_FIRST( vertexuse, &lu->down_hd );
				dist_to_plane = DIST_PT_PLANE( vu->v_p->vg_p->coord, pl );
				if( dist_to_plane > tol.dist || dist_to_plane < -tol.dist )
				{
					triangulate = 1;
					break;
				}
			}
			else
			{
				for( RT_LIST_FOR( eu, edgeuse, &lu->down_hd ) )
				{
					NMG_CK_EDGEUSE( eu );
					vu = eu->vu_p;
					dist_to_plane = DIST_PT_PLANE( vu->v_p->vg_p->coord, pl );
					if( dist_to_plane > tol.dist || dist_to_plane < -tol.dist )
					{
						triangulate = 1;
						break;
					}
				}
				if( triangulate )
					break;
			}
		}

		if( triangulate )
		{
			/* Need to triangulate this face */
			nmg_triangulate_fu( outfaceuses[i], &tol );

			/* split each triangular loop into its own face */
			fu = outfaceuses[i];
			if( fu->orientation != OT_SAME )
				fu = fu->fumate_p;
			if( fu->orientation != OT_SAME )
			{
				rt_log( "cvt_euclid_region: face (fu = x%x) with no OT_SAME use!!\n", fu );
				nmg_pr_fu_briefly( fu , "" );
				rt_bomb( "cvt_euclid_region: face with no OT_SAME use\n" );
			}
			(void)nmg_split_loops_into_faces( &fu->l.magic, &tol );
			if( nmg_calc_face_g( fu ) )
			{
				rt_log( "cvt_euclid_region: nmg_calc_face_g failed!!\n" );
				rt_bomb( "euclid-g: Could not calculate new face geometry\n" );
			}
		}
	}

	for( RT_LIST_FOR( fu, faceuse, &s->fu_hd ) )
	{
		int found=0;

		if( fu->orientation != OT_SAME )
			continue;

		for( j=0; j<face; j++ )
		{
			if( fu == outfaceuses[j] || fu->fumate_p == outfaceuses[j] )
			{
				found = 1;
				break;
			}
		}
		if( !found )
			nmg_calc_face_g( fu );
	}
#if 0
	/* if the shell we just built has a void shell inside, nmg_fix_normals will
	 * point the normals of the void shell in the wrong direction. This section
	 * of code looks for such a situation and reverses the normals of the void shell
	 *
	 * first decompose the shell into maximally connected shells
	 */
	if( (shell_count = nmg_decompose_shell( s , &tol )) > 1 )
	{
		/* This shell has more than one part */
		struct shell **shells;
		struct nmg_ptbl verts;
		int shell1_no, shell2_no, outer_shell_count=0;
		short *in_out;
		short *shell_inout;

		shell_inout = (short *)rt_calloc( shell_count, sizeof( short ), "shell_inout" );
		in_out = (short *)rt_calloc( shell_count * shell_count , sizeof( short ) , "in_out" );

		nmg_tbl( &verts , TBL_INIT , (long *)NULL );

		shells = (struct shell **)rt_calloc( shell_count, sizeof( struct shell *), "shells" );

		/* fuse geometry */
		(void)nmg_model_vertex_fuse( m, &tol );

		i = 0;

		if( debug )
			rt_log( "\nShell decomposed into %d sub-shells\n", shell_count );


		/* insure that bounding boxes are available
		 * and that all the shells are closed.
		 */
		shell1_no = (-1);
		for( RT_LIST_FOR( s , shell , &r->s_hd ) )
		{
			shells[++shell1_no] = s;
			if( !s->sa_p )
				nmg_shell_a( s , &tol );

			if( nmg_check_closed_shell( s , &tol ) )
			{
				rt_log( "Warning: Region %d is not a closed surface\n" , reg_id );
				rt_log( "\tCreating new faces to close region\n" );
				nmg_close_shell( s , &tol );
				if( nmg_check_closed_shell( s , &tol ) )
					rt_bomb( "Cannot close shell\n" );
			}
		}

		shell1_no = (-1);
		for( RT_LIST_FOR( s, shell , &r->s_hd ) )
		{
			struct shell *s2;
			int outside=0;
			int inside=0;

			shell1_no++;

			nmg_vertex_tabulate( &verts, &s->l.magic );

			shell2_no = (-1);
			for( RT_LIST_FOR( s2 , shell , &r->s_hd ) )
			{
				int j;
				int in=0;
				int on=0;
				int out=0;

				shell2_no++;

				if( s2 == s )
					continue;

				for( j=0 ; j<NMG_TBL_END( &verts ) ; j++ )
				{
					struct vertex *v;

					v = (struct vertex *)NMG_TBL_GET( &verts , j );
					NMG_CK_VERTEX( v );

					if( nmg_find_v_in_shell( v, s2, 1 ) )
					{
						on++;
						continue;
					}

					switch( nmg_class_pt_s(v->vg_p->coord, s2, &tol) )
					{
						case NMG_CLASS_AinB:
							in++;
							break;
						case NMG_CLASS_AonBshared:
							on++;
							break;
						case NMG_CLASS_AoutB:
							out++;
							break;
						default:
							rt_bomb( "UNKNOWN CLASS!!!\n" );
							break;
					}
				}

				if( out > in )
					inout( shell1_no, shell2_no ) = OUTSIDE;
				else
					inout( shell1_no, shell2_no ) = INSIDE;

			}

			nmg_tbl( &verts , TBL_RST, (long *)NULL );
		}

		nmg_tbl( &verts, TBL_FREE, (long *)NULL );

		/* determine which shells are outer shells, which are inner,
		 * and which must be inverted
		 */
		for( shell1_no=0 ; shell1_no < shell_count ; shell1_no++ )
		{
			int outers=0,inners=0;

			shell_inout[shell1_no] = 0;
			for( shell2_no=0 ; shell2_no < shell_count ; shell2_no++ )
			{

				if( shell1_no == shell2_no )
					continue;

				if( inout( shell1_no, shell2_no ) == OUTSIDE )
					outers++;
				else if( inout( shell1_no, shell2_no ) == INSIDE )
					inners++;
			}

			if( outers && !inners )
				shell_inout[shell1_no] |= OUTER_SHELL;
			if( inners )
				shell_inout[shell1_no] |= INNER_SHELL;
			if( inners%2 )
			{
				shell_inout[shell1_no] |= INVERT_SHELL;
				nmg_invert_shell( shells[shell1_no], &tol );
			}
		}

		/* join inner shells to outer shells */
		for( shell1_no=0 ; shell1_no < shell_count ; shell1_no++ )
		{
			struct shell *outer_shell=(struct shell *)NULL;

			if( shell_inout[shell1_no] & OUTER_SHELL )
				continue;

			if( !shell_inout[shell1_no] & INNER_SHELL )
				rt_bomb( "Found a shell that is neither inner nor outer!!!\n" );

			/* Look for an outer shell to take this inner shell */
			for( shell2_no=0 ; shell2_no < shell_count ; shell2_no++ )
			{
				if( shell2_no == shell1_no )
					continue;

				if( !shells[shell2_no] )
					continue;

				if( inout( shell1_no, shell2_no ) == INSIDE &&
					shell_inout[shell2_no] & OUTER_SHELL )
				{
					outer_shell = shells[shell2_no];
					break;
				}

			}
			if( !outer_shell )
				rt_bomb( "Cannot find outer shell for inner shell!!!\n" );

			/* Place this inner shell in the outer shell */
			nmg_js( outer_shell, shells[shell1_no], &tol );
			shells[shell1_no] = (struct shell *)NULL;
		}

		/* Check and count the outer shells */
		for( shell1_no=0 ; shell1_no < shell_count ; shell1_no++ )
		{
			if( !shells[shell1_no] )
				continue;

			if( shell_inout[shell1_no] & INNER_SHELL )
				rt_bomb( "An inner shell was not placed in an outer shell!!\n" );

			outer_shell_count++;
		}

		if( outer_shell_count < 1 )
			rt_bomb( "No shells!!!!\n" );
		else if( outer_shell_count == 1 )
			add_nmg_to_db( fpdb, m, reg_id );
		else
			add_shells_to_db( fpdb, shell_count, shells, reg_id );

		rt_free( (char *)shells, "shells" );
		rt_free( (char *)shell_inout, "shell_inout" );
		rt_free( (char *)in_out, "in_out" );
	}
	else
#endif
		add_nmg_to_db( fpdb, m, reg_id );

	nmg_km(m);				/* Safe to kill model now. */

	return(cur_id);
}

/*
 *	R e a d _ E u c l i d _ F a c e
 *
 *	Read in vertices from a Euclid facet file and store them in an
 *	array of nmg vertex structures.  Then make a list of indices of these
 *	vertices in the vertex array.  This list represents the face under
 *	construction.
 *
 *	XXX Fix this!  Only allows set max of points and assumes
 *	no errors during reading...
 */
int
read_euclid_face(lst, ni, fp, vert, nv)
FILE	*fp;
int	*lst, *ni, *nv;
struct vlist	*vert;
{
	fastf_t	num_points, x, y, z, a, b, c, d;
	int	i, j, k, facet_type;

	/* Description of record. */
	fscanf(fp, "%d %*lf %*lf %lf", &facet_type, &num_points);
	*ni = (int)num_points;
	
	/* Read in data points. */
	for (i = 0; i < *ni; i++) {
		fscanf(fp, "%*d %lf %lf %lf", &x, &y, &z);

		if ((lst[i] = find_vert(vert, *nv, x, y, z)) == -1)
			lst[i] = store_vert(vert, nv, x, y, z);
	}

	/* Read in plane equation. */
	fscanf(fp, "%*d %lf %lf %lf %lf", &a, &b, &c, &d);

	/* Remove duplicate points (XXX this should be improved). */
	for (i = 0; i < *ni; i++)
		for (j = i+1; j < *ni; j++)
			if (lst[i] == lst[j]) {
				int increment;
				
				if( j == i+1 || (i == 0 && j == (*ni-1))  )
					increment = 1;
				else if( j == i+2 )
				{
					j = i+1;
					increment = 2;
				}
				else
				{
					fprintf( stderr , "warning: removing distant duplicates\n" );
					increment = 1;
				}

				for (k = j ; k < *ni-increment; k++)
					lst[k] = lst[k + increment];
				*ni -= increment;
			}

	return(facet_type);
}

/*
 *	F i n d _ V e r t
 *
 *	Try to locate a geometric point in the list of vertices.  If found,
 *	return the index number within the vertex array, otherwise return
 *	a -1.
 */
int
find_vert(vert, nv, x, y, z)
struct vlist	*vert;
int		nv;
fastf_t		x, y, z;
{
	int	found, i;

/* XXX What's good here? */
#define ZERO_TOL 0.0001

	found = 0;
	for (i = 0; i < nv; i++) {
		if (NEAR_ZERO(x - vert->pt[3*i+0], ZERO_TOL)
			&& NEAR_ZERO(y - vert->pt[3*i+1], ZERO_TOL)
			&& NEAR_ZERO(z - vert->pt[3*i+2], ZERO_TOL))
		{
			found = 1;
			break;
		}
	}
	if (!found)
		return( -1 );
	else
		return( i );
}

/*
 *	S t o r e _ V e r t
 *
 *	Store vertex in an array of vertices.
 */
int
store_vert(vert, nv, x, y, z)
struct vlist	*vert;
int	*nv;
fastf_t	x, y, z;
{
	vert->pt[*nv*3+0] = x;
	vert->pt[*nv*3+1] = y;
	vert->pt[*nv*3+2] = z;
	vert->vt[*nv] = (struct vertex *)NULL;

	++*nv;

	if (*nv > MAX_PTS_PER_FACE) {
		fprintf(stderr,
		"read_euclid_face: no more vertex room\n");
		exit(1);
	}

	return(*nv - 1);
}
