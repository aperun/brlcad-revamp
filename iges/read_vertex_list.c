/*
 *  Authors -
 *	John R. Anderson
 *
 *  Source -
 *	SLAD/BVLD/VMB
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1993 by the United States Army.
 *	All rights reserved.
 */

#include "conf.h"

#include <stdio.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "wdb.h"
#include "./iges_struct.h"
#include "./iges_extern.h"

struct iges_vertex_list *
Read_vertex_list( vert_de )
int vert_de;
{
	struct iges_vertex_list	*vertex_list;
	int			entityno;
	int			sol_num;
	int			i;

	entityno = (vert_de - 1)/2;

	/* Acquiring Data */

	if( dir[entityno]->param <= pstart )
	{
		printf( "Illegal parameter pointer for entity D%07d (%s)\n" ,
				dir[entityno]->direct , dir[entityno]->name );
		return( (struct iges_vertex_list *)NULL );
	}

	Readrec( dir[entityno]->param );
	Readint( &sol_num , "" );
	if( sol_num != 502 )
	{
		/* this is not an vertex list entity */
		rt_log( "Read_vertex_list: entity at DE %d is not an vertex list entity\n" , vert_de );
		return( (struct iges_vertex_list *)NULL );
	}

	vertex_list = (struct iges_vertex_list *)rt_malloc( sizeof( struct iges_vertex_list )  ,
			"Read_vertex_list: iges_vertex_list" );

	vertex_list->vert_de = vert_de;
	vertex_list->next = NULL;
	Readint( &vertex_list->no_of_verts , "" );
	vertex_list->i_verts = (struct iges_vertex *)rt_calloc( vertex_list->no_of_verts , sizeof( struct iges_vertex ) ,
			"Read_vertex_list: iges_vertex" );

	for( i=0 ; i<vertex_list->no_of_verts ; i++ )
	{
		Readcnv( &vertex_list->i_verts[i].pt[X] , "" );
		Readcnv( &vertex_list->i_verts[i].pt[Y] , "" );
		Readcnv( &vertex_list->i_verts[i].pt[Z] , "" );
		vertex_list->i_verts[i].v = (struct vertex *)NULL;
	}

	return( vertex_list );
}
