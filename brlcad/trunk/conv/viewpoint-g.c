/*
 *	V I E W P O I N T - G
 *
 *  Converter from Viewpoint Datalabs coor/elem format
 *  to BRLCAD format.  Will assign vertex normals if they
 *  are present in the input files.  Two files are expected
 *  one containing vertex coordinates (and optional normals)
 *  and the second which lists the vertex numbers for each polygonal face.
 *
 *  Author:
 *	John R. Anderson
 *
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Notice -
 *	Re-distribution of this software is restricted, as described in
 *	your "Statement of Terms and Conditions for the Release of
 *	The BRL-CAD Pacakge" agreement.
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 1993 by the United States Army
 *	in all countries except the USA.  All rights reserved.
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "machine.h"
#include "db.h"
#include "externs.h"
#include "vmath.h"
#include "nmg.h"
#include "rtgeom.h"
#include "raytrace.h"
#include "wdb.h"
#include "../librt/debug.h"

RT_EXTERN( fastf_t nmg_loop_plane_area , (struct loopuse *lu , plane_t pl ) );

extern int errno;

#define START_ARRAY_SIZE	64
#define ARRAY_BLOCK_SIZE	64

/* structure for storing coordinates and associated vertex pointer */
struct viewpoint_verts
{
	point_t coord;
	vect_t norm;
	struct vertex *vp;
	short has_norm;
};

#define	LINELEN	256 /* max input line length from elements file */

static char *tok_sep=" ";		/* seperator used in input files */
static char *usage="viewpoint-g [-t tol] -c coord_file_name -e elements_file_name -o output_file_name";

main( int argc , char *argv[] )
{
	register int c;
	FILE *coords,*elems;		/* input file pointers */
	FILE *out_fp;			/* output file pointers */
	char *base_name;		/* title and top level group name */
	char *coords_name;		/* input coordinates file name */
	char *elems_name;		/* input elements file name */
	float x,y,z,nx,ny,nz;		/* vertex and normal coords */
	char *ptr1,*ptr2;
	int name_len;
	struct rt_tol tol;
	int done=0;
	int i;
	int no_of_verts;
	int no_of_faces=0;
	char line[LINELEN];
	struct nmg_ptbl vertices;	/* table of vertices for one face */
	struct nmg_ptbl faces;		/* table of faces for one element */
	struct nmg_ptbl names;		/* table of element names */
	struct model *m;
	struct nmgregion *r;
	struct shell *s;
	struct faceuse *fu;
	struct viewpoint_verts *verts;	/* array of structures holding coordinates and normals */
	struct wmember reg_head,*wmem;

        /* XXX These need to be improved */
        tol.magic = RT_TOL_MAGIC;
        tol.dist = 0.01;
        tol.dist_sq = tol.dist * tol.dist;
        tol.perp = 1e-6;
        tol.para = 1 - tol.perp;

	out_fp = stdout;
	coords = NULL;
	elems = NULL;

	if( argc < 2 )
		rt_bomb( usage );

	/* get command line arguments */
	while ((c = getopt(argc, argv, "t:c:e:o:")) != EOF)
	{
		switch( c )
		{
			case 't': /* tolerance */
				tol.dist = atof( optarg );
				tol.dist_sq = tol.dist * tol.dist;
				break;
			case 'c': /* input coordinates file name */
				coords_name = (char *)rt_malloc( strlen( optarg ) + 1 , "Viewpoint-g: base name" );
				strcpy( coords_name , optarg );
				if( (coords = fopen( coords_name , "r" )) == NULL )
				{
					rt_log( "Cannot open %s\n" , coords_name );
					perror( "viewpoint-g" );
					rt_bomb( "Cannot open input file" );
				}
				break;
			case 'e': /* input elements file name */
				elems_name = (char *)rt_malloc( strlen( optarg ) + 1 , "Viewpoint-g: base name" );
				strcpy( elems_name , optarg );
				if( (elems = fopen( elems_name , "r" )) == NULL )
				{
					rt_log( "Cannot open %s\n" , elems_name );
					perror( "viewpoint-g" );
					rt_bomb( "Cannot open input file" );
				}
				break;
			case 'o': /* output file name */
				if( (out_fp = fopen( optarg , "w" )) == NULL )
				{
					rt_log( "Cannot open %s\n" , optarg );
					perror( "tankill-g" );
					rt_bomb( "Cannot open output file\n" );
				}
				break;
			default:
				rt_bomb( usage );
				break;
		}
	}

	/* Must have some input */
	if( coords == NULL || elems == NULL )
		rt_bomb( usage );

	/* build a title for the BRLCAD database */
	ptr1 = strrchr( coords_name , '/' );
	if( ptr1 == NULL )
		ptr1 = coords_name;
	else
		ptr1++;
	ptr2 = strchr( ptr1 , '.' );

	if( ptr2 == NULL )
		name_len = strlen( ptr1 );
	else
		name_len = ptr2 - ptr1;

	base_name = (char *)rt_malloc( name_len + 1 , "base_name" );
	strncpy( base_name , ptr1 , name_len );

	/* make title record */	
	mk_id( out_fp , base_name );

	/* count vertices */
	no_of_verts = 1;
	while( fgets( line , LINELEN , coords ) != NULL )
		no_of_verts++;

	/* allocate memory to store vertex coordinates and normal coordinates and a pointer to
	 * an NMG vertex structure */
	verts = (struct viewpoint_verts *)rt_calloc( no_of_verts , sizeof( struct viewpoint_verts ) , "viewpoint-g: vertex list" );

	/* Now read the vertices again and store them */
	rewind( coords );
	while( fgets( line , LINELEN ,  coords ) != NULL )
	{
		int number_scanned;
		number_scanned = sscanf( line , "%d,%f,%f,%f,%f,%f,%f" , &i , &x , &z , &y , &nx , &ny , &nz );
		if( number_scanned < 4 )
			break;
		if( i >= no_of_verts )
			rt_log( "vertex number too high (%d) only allowed for %d\n" , i , no_of_verts );
		VSET( verts[i].coord , x , y , z );

		if( number_scanned == 7 ) /* we get normals too!!! */
		{
			VSET( verts[i].norm , nx , ny , nz );
			verts[i].has_norm = 1;
		}
	}

	/* Let the user know that something is happening */
	fprintf( stderr , "%d vertices\n" , no_of_verts-1 );

	/* initialize tables */
	nmg_tbl( &vertices , TBL_INIT , NULL );
	nmg_tbl( &faces , TBL_INIT , NULL );
	nmg_tbl( &names , TBL_INIT , NULL );

	while( !done )
	{
		char *name,*curr_name,*ptr;
		int eof=0;

		/* Find an element name that has not already been processed */
		curr_name = NULL;
		done = 1;
		while( fgets( line , LINELEN , elems ) != NULL )
		{
			line[strlen(line)-1] = '\0';
			name = strtok( line , tok_sep );
			if( NMG_TBL_END( &names ) == 0 )
			{
				/* this is the first element processed */
				curr_name = rt_malloc( sizeof( name ) + 1 , "viewpoint-g: component name" );
				strcpy( curr_name , name );

				/* add this name to the table */
				nmg_tbl( &names , TBL_INS , (long *)curr_name );
				done = 0;
				break;
			}
			else
			{
				int found=0;

				/* check the list to see if this name is already there */
				for( i=0 ; i<NMG_TBL_END( &names ) ; i++ )
				{
					if( !strcmp( (char *)NMG_TBL_GET( &names , i ) , name ) )
					{
						/* found it, so go back and read the next line */
						found = 1;
						break;
					}
				}
				if( !found )
				{
					/* didn't find name, so this becomes the current name */
					curr_name = rt_malloc( sizeof( name ) + 1 , "viewpoint-g: component name" );
					strcpy( curr_name , name );

					/* add it to the table */
					nmg_tbl( &names , TBL_INS , (long *)curr_name );
					done = 0;
					break;
				}
			}
		}

		/* if no current name, then we are done */
		if( curr_name == NULL )
			break;

		/* Hopefully, the user is still around */
		fprintf( stderr , "\tMaking %s\n" , curr_name );

		/* make basic nmg structures */
		m = nmg_mm();
		r = nmg_mrsv( m );
		s = RT_LIST_FIRST( shell , &r->s_hd );

		/* set all vertex pointers to NULL so that different models don't share vertices */
		for( i=0 ; i<no_of_verts ; i++ )
			verts[i].vp = (struct vertex *)NULL;

		/* read elements file and make faces */
		while( !eof )
		{
			/* loop through vertex numbers */
			while( (ptr = strtok( (char *)NULL , tok_sep ) ) != NULL )
			{
				i = atoi( ptr );
				if( i >= no_of_verts )
					rt_log( "vertex number too high in element (%d) only allowed for %d\n" , i , no_of_verts );

				/* put vertex pointer in list for this face */
				nmg_tbl( &vertices , TBL_INS , (long *)(&verts[i].vp) );
			}

			if( NMG_TBL_END( &vertices ) )
			{
				/* make face */
				fu = nmg_cmface( s , (struct vertex ***)NMG_TBL_BASEADDR( &vertices ) , NMG_TBL_END( &vertices ) );
				no_of_faces++;

				/* put faceuse in list for the current named object */
				nmg_tbl( &faces , TBL_INS , (long *)fu );

				/* restart the vertex list for the next face */
				nmg_tbl( &vertices , TBL_RST , NULL );
			}

			/* skip elements with the wrong name */
			name = NULL;
			while( name == NULL || strcmp( name , curr_name ) )
			{
				/* check for enf of file */
				if( fgets( line , LINELEN , elems ) == NULL )
				{
					eof = 1;
					break;
				}

				/* get name from input line (first item on line) */
				line[strlen(line)-1] = '\0';
				name = strtok( line ,  tok_sep );
			}
			
		}

		/* assign geometry */
		for( i=0 ; i<no_of_verts ; i++ )
		{
			if( verts[i].vp )
			{
				NMG_CK_VERTEX( verts[i].vp );
				nmg_vertex_gv( verts[i].vp , verts[i].coord );

				/* check if a vertex normal exists */
				if( verts[i].has_norm )
				{
					struct vertexuse *vu;

					/* assign this normal to all uses of this vertex */
					for( RT_LIST_FOR( vu , vertexuse , &verts[i].vp->vu_hd ) )
					{
						NMG_CK_VERTEXUSE( vu );
						nmg_vertexuse_nv( vu , verts[i].norm );
					}
				}
			}
		}

		/* calculate plane equations for faces */
		for (RT_LIST_FOR(s, shell, &r->s_hd))
		{
		    NMG_CK_SHELL( s );
		    fu = RT_LIST_FIRST( faceuse , &s->fu_hd );
		    while( RT_LIST_NOT_HEAD( fu , &s->fu_hd))
		    {
		    	struct faceuse *next_fu;

		        NMG_CK_FACEUSE( fu );

		    	next_fu = RT_LIST_NEXT( faceuse , &fu->l );
		        if( fu->orientation == OT_SAME )
		    	{
		                if( nmg_fu_planeeqn( fu , &tol ) )
		    		{
		    			struct loopuse *lu;
		    			fastf_t area;
		    			plane_t pl;

		    			lu = RT_LIST_FIRST( loopuse , &fu->lu_hd );
		    			area = nmg_loop_plane_area( lu , pl );
		    			if( area <= 0.0 )
		    			{
		    				struct faceuse *kill_fu;

		    				rt_log( "ERROR: Can't get plane for face\n" );

		    				kill_fu = fu;
		    				if( next_fu == kill_fu->fumate_p )
		    					next_fu = RT_LIST_NEXT( faceuse , &next_fu->l );
		    				nmg_kfu( kill_fu );
		    			}

		    			nmg_face_g( fu , pl );
		    		}
		    	}
		    	fu = next_fu;
		    }
		}

		/* glue faces together */
		nmg_gluefaces( (struct faceuse **)NMG_TBL_BASEADDR( &faces) , NMG_TBL_END( &faces ) );

		/* restart the list of faces for the next object */
		nmg_tbl( &faces , TBL_RST , NULL );

		/* write the nmg to the output file */
		mk_nmg( out_fp , curr_name  , m );

		/* kill the current model */
		nmg_km( m );

		/* rewind the elements file for the next object */
		rewind( elems );
	}

	fprintf( stderr , "%d polygons\n" , no_of_faces );

	/* make a top level group with all the objects as members */
	RT_LIST_INIT( &reg_head.l );
	for( i=0 ; i<NMG_TBL_END( &names ) ; i++ )
	{
		if( (wmem = mk_addmember( (char *)NMG_TBL_GET( &names , i ) , &reg_head , WMOP_UNION ) ) == WMEMBER_NULL )
		{
			rt_log( "Cannot make top level group\n" );
			exit( 1 );
		}
	}

	fprintf( stderr , "Making top level group (%s)\n" , base_name );
	if( mk_lcomb( out_fp , base_name , &reg_head , 0, (char *)0, (char *)0, (char *)0, 0 ) )
		rt_log( "viewpoint-g: Error in making top level group" );

}
