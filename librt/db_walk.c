/*
 *			D B _ W A L K . C
 *
 * Functions -
 *	db_functree	No-frills tree-walk
 *	comb_functree	No-frills combination-walk
 *
 *
 *  Authors -
 *	Michael John Muuss
 *	John R. Anderson
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1988 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static const char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"

#include <stdio.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"

#include "./debug.h"

void
db_functree_subtree( dbip, tp, comb_func, leaf_func, resp, client_data )
struct db_i	*dbip;
union tree	*tp;
void		(*comb_func)BU_ARGS((struct db_i *, struct directory *, genptr_t));
void		(*leaf_func)BU_ARGS((struct db_i *, struct directory *, genptr_t));
struct resource *resp;
genptr_t	client_data;
{
	struct directory *dp;

	if( !tp )
		return;

	RT_CHECK_DBI( dbip );
	RT_CK_TREE( tp );
	RT_CK_RESOURCE( resp );

	switch( tp->tr_op )  {

		case OP_DB_LEAF:
			if( (dp=db_lookup( dbip, tp->tr_l.tl_name, LOOKUP_NOISY )) == DIR_NULL )
				return;
			db_functree( dbip, dp, comb_func, leaf_func, resp, client_data);
			break;

		case OP_UNION:
		case OP_INTERSECT:
		case OP_SUBTRACT:
		case OP_XOR:
			db_functree_subtree( dbip, tp->tr_b.tb_left, comb_func, leaf_func, resp, client_data );
			db_functree_subtree( dbip, tp->tr_b.tb_right, comb_func, leaf_func, resp, client_data );
			break;
		default:
			bu_log( "db_functree_subtree: unrecognized operator %d\n", tp->tr_op );
			bu_bomb( "db_functree_subtree: unrecognized operator\n" );
	}
}

/*
 *			D B _ F U N C T R E E
 *
 *  This subroutine is called for a no-frills tree-walk,
 *  with the provided subroutines being called at every combination
 *  and leaf (solid) node, respectively.
 *
 *  This routine is recursive, so no variables may be declared static.
 *  
 */
void
db_functree( dbip, dp, comb_func, leaf_func, resp, client_data)
struct db_i	*dbip;
struct directory *dp;
void		(*comb_func)BU_ARGS((struct db_i *, struct directory *, genptr_t));
void		(*leaf_func)BU_ARGS((struct db_i *, struct directory *, genptr_t));
struct resource *resp;
genptr_t	client_data;
{
	register int		i;

	RT_CK_DBI(dbip);
	if(rt_g.debug&DEBUG_DB) bu_log("db_functree(%s) x%x, x%x, comb=x%x, leaf=x%x, client_data=x%x\n",
		dp->d_namep, dbip, dp, comb_func, leaf_func, client_data );

	if( dp->d_flags & DIR_COMB )  {
		if( dbip->dbi_version < 5 ) {
			register union record	*rp;
			register struct directory *mdp;
			/*
			 * Load the combination into local record buffer
			 * This is in external v4 format.
			 */
			if( (rp = db_getmrec( dbip, dp )) == (union record *)0 )
				return;

			/* recurse */
			for( i=1; i < dp->d_len; i++ )  {
				if( (mdp = db_lookup( dbip, rp[i].M.m_instname,
				    LOOKUP_NOISY )) == DIR_NULL )
					continue;
				db_functree( dbip, mdp, comb_func, leaf_func, resp, client_data );
			}
			bu_free( (char *)rp, "db_functree record[]" );
		} else {
			struct rt_db_internal in;
			struct rt_comb_internal *comb;
			union tree *tp;
			int id;

			if( (id=rt_db_get_internal5( &in, dp, dbip, NULL, resp )) < 0 )
				return;

			comb = (struct rt_comb_internal *)in.idb_ptr;
			tp = comb->tree;
			db_functree_subtree( dbip, tp, comb_func, leaf_func, resp, client_data );
			rt_db_free_internal( &in, resp );
		}

		/* Finally, the combination itself */
		if( comb_func )
			comb_func( dbip, dp, client_data );

	} else if( dp->d_flags & DIR_SOLID )  {
		if( leaf_func )
			leaf_func( dbip, dp, client_data );
	} else {
		bu_log("db_functree:  %s is neither COMB nor SOLID?\n",
			dp->d_namep );
	}
}
