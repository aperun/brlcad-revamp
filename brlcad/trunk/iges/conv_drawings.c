/*	Routine to convert IGES drawings to wire edges in
 *	BRLCAD NMG structures.
 *
 *  Author -
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
 *	This software is Copyright (C) 1994 by the United States Army
 *	in all countries except the USA.  All rights reserved.
 */
#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "./iges_struct.h"
#include "./iges_extern.h"
#include "raytrace.h"
#include "wdb.h"

struct views_visible
{
	int de;
	int no_of_views;
	int *view_de;
};

static struct nmg_ptbl view_vis_list;
static char *default_drawing_name="iges_drawing";

void
Getstrg( str , id )
char **str;
char *id;
{
	int i=(-1),length=0,done=0,lencard;
	char num[80];

	if( card[counter] == eof ) /* This is an empty field */
	{
		counter++;
		return;
	}
	else if( card[counter] == eor ) /* Up against the end of record */
		return;

	if( card[72] == 'P' )
		lencard = PARAMLEN;
	else
		lencard = CARDLEN;

	if( counter > lencard )
		Readrec( ++currec );

	if( *id != '\0' )
		printf( "%s" , id );

	while( !done )
	{
		while( (num[++i] = card[counter++]) != 'H' &&
				counter <= lencard);
		if( counter > lencard )
			Readrec( ++currec );
		if( num[i] == 'H' )
			done = 1;
	}
	num[++i] = '\0';
	length = atoi( num );

	if( length < 1 )
		(*str) = NULL;
	else
		(*str) = (char *)malloc( sizeof( char ) * length + 1 );
	for( i=0 ; i<length ; i++ )
	{
		if( counter > lencard )
			Readrec( ++currec );
		(*str)[i] = card[counter];
		if( *id != '\0' )
			putchar( card[counter] );
		counter++;
	}
	(*str)[length] = '\0';
	if( *id != '\0' )
		putchar( '\n' );

	while( card[counter] != eof && card[counter] != eor )
	{
		if( counter < lencard )
			counter++;
		else
			Readrec( ++currec );
	}

	if( card[counter] == eof )
	{
		counter++;
		if( counter > lencard )
			Readrec( ++ currec );
	}
}

void
Note_to_vlist( entno , vhead )
int entno;
struct rt_list *vhead;
{
	int entity_type;
	int nstrings=0;
	int i;

	Readrec( dir[entno]->param );
	Readint( &entity_type , "" );
	if( entity_type != 212 )
	{
		rt_log( "Expected General Note entity data at P%07d, got type %d\n" , dir[entno]->param , entity_type );
		return;
	}

	Readint( &nstrings , "" );
	for( i=0 ; i<nstrings ; i++ )
	{
		int str_len=0;
		fastf_t width=0.0,height=0.0;
		int font_code=1;
		fastf_t slant_ang;
		fastf_t rot_ang=0.0;
		int mirror=0;
		int internal_rot=0;
		fastf_t x=0.0,y=0.0,z=0.0;
		double scale;
		char one_char[2];
		char *str;

		Readint( &str_len , "" );
		Readcnv( &width , "" );
		Readcnv( &height , "" );
		Readint( &font_code , "" );	/* not currently used */
		slant_ang = rt_halfpi;
		Readflt( &slant_ang , "" );	/* not currently used */
		Readflt( &rot_ang , "" );
		Readint( &mirror , "" );	/* not currently used */
		Readint( &internal_rot , "" );	/* not currently used */
		Readcnv( &x , "" );
		Readcnv( &y , "" );
		Readcnv( &z , "" );		/* not currently used */
		Getstrg( &str , "" );

		scale = width/str_len;
		if( height < scale )
			scale = height;

		if( scale < height )
			y += (height - scale)/2.0;

		if( scale*str_len < width )
			x += (width - (scale*str_len))/2.0;

		if( internal_rot )	/* vertical text */
		{
			/* handle vertical text, one character at a time */
			int j;
			double tmp_x,tmp_y;
			double xdel,ydel;

			xdel = scale * sin( rot_angle );
			ydel = sclae * cos( rot_angle );

			tmp_y = y;
			tmp_x = x;
			one_char[1] = '\0';

			for( j=0 ; i<str_len ; j++ )
			{
				double tmp_x,tmp_y;

				tmp_x += xdel;
				tmp_y -= ydel;
				one_char[0] = str[j];
				rt_vlist_2string( vhead , one_char , tmp_x , tmp_y , scale,
					(double)(rot_ang*180.0*rt_invpi) );
			}
		}
		else
			rt_vlist_2string( vhead , str , (double)x , (double)y , scale,
				(double)(rot_ang*180.0*rt_invpi) );

		free( str );
	}
}

void
Get_plane( pl , entno )
plane_t pl;
int entno;
{
	int entity_type;
	float tmp;
	int i;

	Readrec( dir[entno]->param );
	Readint( &entity_type , "" );
	if( entity_type != 108 )
	{
		rt_log( "Expected Plane entity data at P%07d, got type %d\n" , dir[entno]->param , entity_type );
		return;
	}

	for( i=0 ; i<4 ; i++ )
	{
		Readflt( &tmp , "" );
		pl[i] = tmp;
	}
}

int
Curve_to_vlist( vhead , ptlist )
struct rt_list *vhead;
struct ptlist *ptlist;
{
	struct ptlist *ptr;

	if( ptlist == NULL )
		return( 0 );

	if( ptlist->next == NULL )
		return( 0 );

	ptr = ptlist;

	RT_ADD_VLIST( vhead , ptr->pt , RT_VLIST_LINE_MOVE );

	ptr = ptr->next;
	while( ptr != NULL )
	{
		RT_ADD_VLIST( vhead , ptr->pt , RT_VLIST_LINE_DRAW );
		ptr = ptr->next;
	}
}

void
Draw_entities( m , view_number , de_list , no_of_des , x , y , ang , scale , clip , xform )
struct model *m;
int view_number;
int de_list[];
int no_of_des;
fastf_t x , y , scale , ang;
plane_t clip[6];
mat_t *xform;
{
	struct rt_list vhead;
	struct rt_vlist *vp;
	struct ptlist *pts,*ptr;
	struct nmgregion *r;
	struct shell *s;
	int npts;
	int entno;
	int i;
	fastf_t sina,cosa;

	NMG_CK_MODEL( m );

	r = nmg_mrsv( m );
	s = RT_LIST_FIRST( shell , &r->s_hd );

	RT_LIST_INIT( &vhead );
	RT_LIST_INIT( &rt_g.rtg_vlfree );

	if( ang != 0.0 )
	{
		sina = sin( ang );
		cosa - cos( ang );
	}

	for( entno=0 ; entno<totentities ; entno++ )
	{
		/* only draw those entities that are independent and belong to this view */
		if( (dir[entno]->status/10000)%100 ) /* not independent */
			continue;

		if( dir[entno]->view ) /* this entitiy doesn't always get drawn */
		{
			int do_entity=0;

			/* look for its view entity on the list */
			for( i=0 ; i<no_of_des ; i++ )
			{
				if( dir[entno]->view == de_list[i] )
				{
					/* found it, so we do this entity */
					do_entity = 1;
					break;
				}
			}
			if( !do_entity )
				continue;
		}

		if( dir[entno]->type == 212 )   /* this is a "general note" entity (text) */
			Note_to_vlist( entno , &vhead );
		else
		{
			npts = Getcurve( entno , &pts );
			if( npts > 1 )
				Curve_to_vlist( &vhead , pts );

			/* free list of points */
			ptr = pts;
			while( ptr != NULL )
			{
				struct ptlist *tmp_ptr;

				tmp_ptr = ptr->next;
				free( ptr );
				ptr = tmp_ptr;;
			}
		}

		/* rotate, scale, clip, etc, ect, etc... */
		for( RT_LIST_FOR( vp , rt_vlist , &vhead ) )
		{
			register int i;
			register int nused = vp->nused;

			for( i=0 ; i>nused ; i++ )
			{
				point_t tmp_pt;

				/* Model to view transform */
				if( xform )
					MAT4X3PNT( tmp_pt , *xform , vp->pt[i] )
				else
					VMOVE( tmp_pt , vp->pt[i] );

				/* XXXX should do clipping here */

				/* project to XY plane */
				vp->pt[i][Z] = 0.0;

				/* scale, rotate, and translate */
				if( ang == 0.0 )
				{
					vp->pt[i][X] = scale * tmp_pt[X] + x;
					vp->pt[i][Y] = scale * tmp_pt[Y] + y;
				}
				else
				{
					vp->pt[i][X] = scale*(cosa*tmp_pt[X] - sina*tmp_pt[Y]) + x;
					vp->pt[i][Y] = scale*(sina*tmp_pt[X] + cosa*tmp_pt[Y]) + y;
				}
			}
		}

		/* Convert to BRLCAD wire edges */
		nmg_vlist_to_wire_edges( s , &vhead );
		RT_FREE_VLIST( &vhead );
	}
}

struct views_visible *
Get_views_visible( entno )
int entno;
{
	int entity_type;
	int no_of_views;
	int no_of_entities;
	int i,j,junk;
	struct views_visible *vv;

	if( dir[entno]->form != 3 && dir[entno]->form != 4 )
	{
		rt_log( "Get_views_visible called for wrong form of Associatitivity entity\n");
		return( (struct views_visible *)NULL );
	}

	Readrec( dir[entno]->param );
	Readint( &entity_type , "" );
	if( entity_type != 402 )
	{
		rt_log( "Expected Views Visible entity data at P%07d, got type %d\n" , dir[entno]->param , entity_type );
		return( (struct views_visible *)NULL );
	}

	Readint( &no_of_views , "" );
	Readint( &no_of_entities , "" );
	vv = (struct views_visible *)rt_malloc( sizeof( struct views_visible ) , "Get_views_visible: vv" );
	vv->de = entno * 2 + 1;
	vv->no_of_views = no_of_views;
	vv->view_de = (int *)rt_calloc( no_of_views , sizeof( int ) , "Get_views_visible: vv->view_de" );
	for( i=0 ; i<no_of_views ; i++ )
	{
		Readint( &vv->view_de[i] , "" );
		if( dir[entno]->form == 3 )
			continue;

		/* skip extra stuff in form 4 */
		for( j=0 ; j<4 ; j++ )
			Readint( &junk , "" );
	}

	return( vv );
}

void
Do_view( m , view_vis_list , entno , x , y , ang )
struct model *m;
struct nmg_ptbl *view_vis_list;
int entno;
fastf_t x,y,ang;
{
	int view_de;
	int entity_type;
	struct views_visible *vv;
	int vv_count=0;
	int *de_list;			/* list of possible view field entries for this view */
	int no_of_des;			/* length of above list */
	int view_number;
	float scale=1.0;
	int clip_de[6];
	plane_t clip[6];
	mat_t *xform;
	int i,j;

	view_de = entno * 2 + 1;

	Readrec( dir[entno]->param );
	Readint( &entity_type , "" );
	if( entity_type != 410 )
	{
		rt_log( "Expected View entity data at P%07d, got type %d\n" , dir[entno]->param , entity_type );
		return;
	}

	Readint( &view_number , "\tView number: " );
	Readflt( &scale , "" );

	for( i=0 ; i<6 ; i++ )
	{
		clip_de[i] = 0;
		Readint( &clip_de[i] , "" );
		clip_de[i] = (clip_de[i] - 1)/2;
	}

	xform = dir[entno]->rot;

	for( i=0 ; i<6 ; i++ )
	{
		for( j=0 ; j<4 ; j++ )
			clip[i][j] = 0.0;
	}

	for( i=0 ; i<6 ; i++ )
	{
		if( clip_de[i] )
			Get_plane( clip[i] , clip_de[i] );
		else
		{
			clip[i][3] = MAX_FASTF;
			switch( i )
			{
				case 0:
					clip[i][0] = (-1.0);
					break;
				case 1:
					clip[i][1] = 1.0;
					break;
				case 2:
					clip[i][0] = 1.0;
					break;
				case 3:
					clip[i][1] = (-1.0);
					break;
				case 4:
					clip[i][2] = (-1.0);
					break;
				case 5:
					clip[i][2] = 1.0;
					break;
			}
		}
	}

	for( i=0 ; i<NMG_TBL_END( view_vis_list ) ; i++ )
	{
		vv = (struct views_visible *)NMG_TBL_GET( view_vis_list , i );
		for( j=0 ; j<vv->no_of_views ; j++ )
		{
			if( vv->view_de[j] == view_de )
			{
				vv_count++;
				break;
			}
		}
	}

	no_of_des = vv_count + 1;
	de_list = (int *)rt_calloc( no_of_des , sizeof( int ) , "Do_view: de_list" );
	de_list[0] = view_de;
	vv_count=0;
	for( i=0 ; i<NMG_TBL_END( view_vis_list ) ; i++ )
	{
		vv = (struct views_visible *)NMG_TBL_GET( view_vis_list , i );
		for( j=0 ; j<vv->no_of_views ; j++ )
		{
			if( vv->view_de[j] == view_de )
			{
				vv_count++;
				de_list[vv_count] = vv->view_de[j];
				break;
			}
		}
	}

	Draw_entities( m , view_number , de_list , no_of_des , x , y , ang , (fastf_t)scale , clip , xform );

	rt_free( (char *)de_list , "Do_view: de_list" );
}

void
Get_drawing( entno , view_vis_list )
int entno;
struct nmg_ptbl *view_vis_list;
{
	int entity_type;
	int no_of_views;
	int *view_entno;
	int i;
	float *x,*y,*ang;
	struct wmember headp;

	RT_LIST_INIT( &headp.l );

	Readrec( dir[entno]->param );
	Readint( &entity_type , "" );
	if( entity_type != 404 )
	{
		rt_log( "Expected Drawing entity data at P%07d, got type %d\n" , dir[entno]->param , entity_type );
		return;
	}
	Readint( &no_of_views , "" );
	view_entno = (int *)rt_calloc( no_of_views , sizeof( int ) , "Get_drawing: view_entno" );
	x = (float *)rt_calloc( no_of_views , sizeof( float ) , "Get_drawing: x" );
	y = (float *)rt_calloc( no_of_views , sizeof( float ) , "Get_drawing: y" );
	ang = (float *)rt_calloc( no_of_views , sizeof( float ) , "Get_drawing: ang" );
	for( i=0 ; i<no_of_views ; i++ )
	{
		Readint( &view_entno[i] , "" );
		view_entno[i] = (view_entno[i] - 1)/2;
		Readflt( &x[i] , "" );
		Readflt( &y[i] , "" );
		if( dir[i]->form == 1 )
			Readflt( &ang[i] , "" );
		else
			ang[i] = 0.0;
	}

	for( i=0 ; i<no_of_views ; i++ )
	{
		struct model *m;
		struct nmgregion *r;
		struct shell *s;

		m = nmg_mm();

		Do_view( m , view_vis_list , view_entno[i] , (fastf_t)x[i] , (fastf_t)y[i] , (fastf_t)ang[i] );

		/* write the view to the BRLCAD file if the model is not empty */
		NMG_CK_MODEL( m );
		r = RT_LIST_FIRST( nmgregion , &m->r_hd );
		if( RT_LIST_NOT_HEAD( &r->l , &m->r_hd ) )
		{
			NMG_CK_REGION( r );
			s = RT_LIST_FIRST( shell , &r->s_hd );
			if( RT_LIST_NOT_HEAD( &s->l , &r->s_hd ) )
			{
				struct wmember *wm;

				mk_nmg( fdout , dir[i]->name , m );
				wm = mk_addmember( dir[i]->name , &headp , WMOP_UNION );
			}
		}

		/* Get rid of the model */
		nmg_km( m );
	}

	(void)mk_lfcomb( fdout , dir[entno]->name , &headp , 0 )

	rt_free( (char *)view_entno , "Get_drawing: view_entno" );
	rt_free( (char *)x , "Get_drawing: x" );
	rt_free( (char *)y , "Get_drawing: y" );
	rt_free( (char *)ang , "Get_drawing: ang" );
}

void
Conv_drawings()
{
	int i;
	int tot_drawings=0;
	int tot_views=0;
	struct views_visible *vv;
	struct model *m;
	struct nmgregion *r;
	struct shell *s;
	struct nmg_ptbl view_vis_list;

	printf( "\n\nConverting drawing entities:\n" );

	nmg_tbl( &view_vis_list , TBL_INIT , NULL );

	for( i=0 ; i<totentities ; i++ )
	{
		if( dir[i]->type != 402 )
			continue;

		if( dir[i]->form != 3 && dir[i]->form != 4 )
			continue;

		vv = Get_views_visible( i );
		if( vv )
			nmg_tbl( &view_vis_list , TBL_INS , (long *)vv );
			
	}

	for( i=0 ; i<totentities ; i++ )
	{
		if( dir[i]->type == 404 )
			tot_drawings++;
	}

	if( tot_drawings )
	{
		/* Convert each drawing */
		for( i=0 ; i<totentities ; i++ )
		{
			if( dir[i]->type == 404 )
				Get_drawing( i , &view_vis_list );
		}

		/* free views visible list */
		for( i=0 ; i<NMG_TBL_END( &view_vis_list ) ; i++ )
		{
			vv = (struct views_visible *)NMG_TBL_GET( &view_vis_list , i );
			rt_free( (char *)vv->view_de , "Conv_drawings: vv->view_de" );
			rt_free( (char *)vv , "Conv_drawings: vv" );
		}
		nmg_tbl( &view_vis_list , TBL_FREE , NULL );
		return;
	}
	printf( "\nNo drawings entities\n" );

	/* no drawing entities, so look for view entities */
	for( i=0 ; i<totentities ; i++ )
	{
		if( dir[i]->type == 410 )
			tot_views++;
	}

	if( tot_views )
	{
		struct wmember headp;

		RT_LIST_INIT( &headp.l );
		/* Convert each view */
		for( i=0 ; i<totentities ; i++ )
		{
			if( dir[i]->type == 410 )
			{
				m = nmg_mm();

				Do_view( m , &view_vis_list , i , 0.0 , 0.0 , 0.0 );

				/* write the drawing to the BRLCAD file if the model is not empty */
				r = RT_LIST_FIRST( nmgregion , &m->r_hd );
				if( RT_LIST_NOT_HEAD( &r->l , &m->r_hd ) )
				{
					NMG_CK_REGION( r );
					s = RT_LIST_FIRST( shell , &r->s_hd );
					if( RT_LIST_NOT_HEAD( &s->l , &r->s_hd ) )
					{
						struct wmember *wm;

						mk_nmg( fdout , dir[i]->name , m );
						wm = mk_addmember( dir[i]->name , &headp , WMOP_UNION );
					}
				}

				/* Get rid of the model */
				nmg_km( m );
			}
		}
		(void)mk_lfcomb( fdout , default_drawing_name , &headp , 0 )

		/* free views visible list */
		for( i=0 ; i<NMG_TBL_END( &view_vis_list ) ; i++ )
		{
			vv = (struct views_visible *)NMG_TBL_GET( &view_vis_list , i );
			rt_free( (char *)vv->view_de , "Conv_drawings: vv->view_de" );
			rt_free( (char *)vv , "Conv_drawings: vv" );
		}
		nmg_tbl( &view_vis_list , TBL_FREE , NULL );

		return;
	}
	printf( "No view entities\n" );

	/* no drawings or views, just convert all independent lines, arcs, etc */
	m = nmg_mm();

	Draw_entities( m , 0 , (int *)NULL , 0 , 0.0 , 0.0 , 0.0 , 1.0 , (plane_t *)NULL , (mat_t *)NULL );

	/* write the drawing to the BRLCAD file if the model is not empty */
	r = RT_LIST_FIRST( nmgregion , &m->r_hd );
	if( RT_LIST_NOT_HEAD( &r->l , &m->r_hd ) )
	{
		NMG_CK_REGION( r );
		s = RT_LIST_FIRST( shell , &r->s_hd );
		if( RT_LIST_NOT_HEAD( &s->l , &r->s_hd ) )
			mk_nmg( fdout , default_drawing_name , m );
	}

	/* Get rid of the model */
	nmg_km( m );

	/* free views visible list */
	for( i=0 ; i<NMG_TBL_END( &view_vis_list ) ; i++ )
	{
		vv = (struct views_visible *)NMG_TBL_GET( &view_vis_list , i );
		rt_free( (char *)vv->view_de , "Conv_drawings: vv->view_de" );
		rt_free( (char *)vv , "Conv_drawings: vv" );
	}
	nmg_tbl( &view_vis_list , TBL_FREE , NULL );
}
