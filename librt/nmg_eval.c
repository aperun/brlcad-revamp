/*			N M G _ E V A L . C
 *
 *	Evaluate boolean operations on NMG objects
 *
 *  Authors -
 *	Michael John Muuss
 *	Lee A. Butler
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1990 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSnmg_eval[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "externs.h"
#include "machine.h"
#include "vmath.h"
#include "nmg.h"
#include "raytrace.h"

extern void	nmg_m_reindex();

struct nmg_counter {
	long	max_structs;
	long	regions;
	long	shells;
	long	faces;
	long	face_loops;
	long	face_edges;
	long	face_lone_verts;
	long	wire_loops;
	long	wire_loop_edges;
	long	wire_edges;
	long	lone_verts;
};

/*
 *			N M G _ R E V E R S E _ F A C E
 *
 *  Reverse the orientation of a face.
 *  Manipulate both the topological and geometric aspects of the face.
 */
void
nmg_reverse_face( fu )
register struct faceuse	*fu;
{
	register struct faceuse	*fumate;
	register vectp_t	v;

	NMG_CK_FACEUSE(fu);
	fumate = fu->fumate_p;
	NMG_CK_FACEUSE(fumate);
	NMG_CK_FACE(fu->f_p);
	NMG_CK_FACE_G(fu->f_p->fg_p);

	/* reverse face normal vector */
	v = fu->f_p->fg_p->N;
	VREVERSE(v, v);
	v[H] *= -1.0;

	/* switch which face is "outside" face */
	if (fu->orientation == OT_SAME)  {
		if (fumate->orientation != OT_OPPOSITE)  {
			rt_log("nmg_reverse_face(fu=x%x) fu:SAME, fumate:%d\n",
				fu, fumate->orientation);
			rt_bomb("nmg_reverse_face() orientation clash\n");
		} else {
			fu->orientation = OT_OPPOSITE;
			fumate->orientation = OT_SAME;
		}
	} else if (fu->orientation == OT_OPPOSITE) {
		if (fumate->orientation != OT_SAME)  {
			rt_log("nmg_reverse_face(fu=x%x) fu:OPPOSITE, fumate:%d\n",
				fu, fumate->orientation);
			rt_bomb("nmg_reverse_face() orientation clash\n");
		} else {
			fu->orientation = OT_SAME;
			fumate->orientation = OT_OPPOSITE;
		}
	} else {
		/* Unoriented face? */
		rt_log("ERROR nmg_reverse_face(fu=x%x), orientation=%d.\n",
			fu, fu->orientation );
	}
}

/*
 *			N M G _ M V _ F U _ B E T W E E N _ S H E L L S
 *
 *  Move faceuse from 'src' shell to 'dest' shell.
 */
void
nmg_mv_fu_between_shells( dest, src, fu )
struct shell		*dest;
register struct shell	*src;
register struct faceuse	*fu;
{
	register struct faceuse	*fumate;

	NMG_CK_FACEUSE(fu);
	fumate = fu->fumate_p;
	NMG_CK_FACEUSE(fumate);

	if (fu->s_p != src) {
		rt_log("nmg_mv_fu_between_shells(dest=x%x, src=x%x, fu=x%x), fu->s_p=x%x isnt src shell\n",
			dest, src, fu, fu->s_p );
		rt_bomb("fu->s_p isnt source shell\n");
	}
	if (fumate->s_p != src) {
		rt_log("nmg_mv_fu_between_shells(dest=x%x, src=x%x, fu=x%x), fumate->s_p=x%x isn't src shell\n",
			dest, src, fu, fumate->s_p );
		rt_bomb("fumate->s_p isnt source shell\n");
	}

	/* Remove fu from src shell */
	NMG_LIST_DEQUEUE( &fu->l );
	if( NMG_LIST_IS_EMPTY( &src->fu_hd ) )  {
		/* This was the last fu in the list, bad news */
		rt_log("nmg_mv_fu_between_shells(dest=x%x, src=x%x, fu=x%x), fumate=x%x not in src shell\n",
			dest, src, fu, fumate );
		rt_bomb("src shell emptied before finding fumate\n");
	}

	/* Remove fumate from src shell */
	NMG_LIST_DEQUEUE( &fumate->l );

	/* Add fu and fumate to dest shell */
	NMG_LIST_APPEND( &dest->fu_hd, &fu->l );
	NMG_LIST_APPEND( &fu->l, &fumate->l );

	fu->s_p = dest;
	fumate->s_p = dest;
}

/*
 *			N M G _ M V _ L U _ B E T W E E N _ S H E L L S
 *
 *  Move a wire-loopuse from one shell to another.
 *  Note that this routine can not be used on loops in faces.
 */
void
nmg_mv_lu_between_shells( dest, src, lu )
struct shell		*dest;
register struct shell	*src;
register struct loopuse	*lu;
{
	register struct loopuse	*lumate;

	NMG_CK_LOOPUSE(lu);
	lumate = lu->lumate_p;
	NMG_CK_LOOPUSE(lumate);

	if( lu->up.s_p != src )  {
		rt_log("nmg_mv_lu_between_shells(dest=x%x, src=x%x, lu=x%x), lu->up.s_p=x%x isn't source shell\n",
			dest, src, lu, lu->up.s_p );
		rt_bomb("lu->up.s_p isn't source shell\n");
	}
	if( lumate->up.s_p != src )  {
		rt_log("nmg_mv_lu_between_shells(dest=x%x, src=x%x, lu=x%x), lumate->up.s_p=x%x isn't source shell\n",
			dest, src, lu, lumate->up.s_p );
		rt_bomb("lumate->up.s_p isn't source shell\n");
	}

	/* Remove lu from src shell */
	NMG_LIST_DEQUEUE( &lu->l );
	if( NMG_LIST_IS_EMPTY( &src->lu_hd ) )  {
		/* This was the last lu in the list */
		rt_log("nmg_mv_lu_between_shells(dest=x%x, src=x%x, lu=x%x), lumate=x%x not in src shell\n",
			dest, src, lu, lumate );
		rt_bomb("src shell emptied before finding lumate\n");
	}

	/* Remove lumate from src shell */
	NMG_LIST_DEQUEUE( &lumate->l );

	/* Add lu and lumate to dest shell */
	NMG_LIST_APPEND( &dest->lu_hd, &lu->l );
	NMG_LIST_APPEND( &lu->l, &lumate->l );

	lu->up.s_p = dest;
	lumate->up.s_p = dest;
}

/*
 *			N M G _ M V _ E U _ B E T W E E N _ S H E L L S
 */
void
nmg_mv_eu_between_shells( dest, src, eu )
struct shell		*dest;
register struct shell	*src;
register struct edgeuse	*eu;
{
	register struct edgeuse	*eumate;

	NMG_CK_EDGEUSE(eu);
	eumate = eu->eumate_p;
	NMG_CK_EDGEUSE(eumate);

	if (eu->up.s_p != src) {
		rt_log("nmg_mv_eu_between_shells(dest=x%x, src=x%x, eu=x%x), eu->up.s_p=x%x isnt src shell\n",
			dest, src, eu, eu->up.s_p );
		rt_bomb("eu->up.s_p isnt source shell\n");
	}
	if (eumate->up.s_p != src) {
		rt_log("nmg_mv_eu_between_shells(dest=x%x, src=x%x, eu=x%x), eumate->up.s_p=x%x isn't src shell\n",
			dest, src, eu, eumate->up.s_p );
		rt_bomb("eumate->up.s_p isnt source shell\n");
	}

	/* Remove eu from src shell */
	NMG_LIST_DEQUEUE( &eu->l );
	if( NMG_LIST_IS_EMPTY( &src->eu_hd ) )  {
		/* This was the last eu in the list, bad news */
		rt_log("nmg_mv_eu_between_shells(dest=x%x, src=x%x, eu=x%x), eumate=x%x not in src shell\n",
			dest, src, eu, eumate );
		rt_bomb("src shell emptied before finding eumate\n");
	}

	/* Remove eumate from src shell */
	NMG_LIST_DEQUEUE( &eumate->l );

	/* Add eu and eumate to dest shell */
	NMG_LIST_APPEND( &dest->eu_hd, &eu->l );
	NMG_LIST_APPEND( &eu->l, &eumate->l );

	eu->up.s_p = dest;
	eumate->up.s_p = dest;
}

/*
 *			N M G _ M V _ V U _ B E T W E E N _ S H E L L S
 *
 *  If this shell had a single vertexuse in it, move it to the other
 *  shell, but "promote" it to a vertex-with-self-loop along the way.
 */
void
nmg_mv_vu_between_shells( dest, src, vu )
struct shell		*dest;
register struct shell	*src;
register struct vertexuse	*vu;
{

	NMG_CK_VERTEXUSE( vu );
	NMG_CK_VERTEX( vu->v_p );

	(void)nmg_mlv( &(dest->l.magic), vu->v_p, OT_SAME );
	nmg_kvu( vu );
}

#define BACTION_KILL			1
#define BACTION_RETAIN			2
#define BACTION_RETAIN_AND_FLIP		3

static char	*nmg_baction_names[] = {
	"*undefined 0*",
	"BACTION_KILL",
	"BACTION_RETAIN",
	"BACTION_RETAIN_AND_FLIP",
	"*undefined 4*"
};

#define NMG_CLASS_BAD		8
static char	*nmg_class_names[] = {
	"onAinB",
	"onAonBshared",
	"onAonBanti",
	"onAoutB",
	"inAonB",
	"onAonBshared",
	"onAonBanti",
	"outAonB",
	"*BAD*"
};

/*
 * Actions are listed:
 *	(Aon)	onAinB, onAonBshared, onAonBanti-shared, onAoutB,
 *	(Bon)	inAonB, onAonBshared, onAonBanti-shared, outAonB
 */
static int		subtraction_actions[8] = {
	BACTION_KILL,
	BACTION_KILL,		/* shared */
	BACTION_RETAIN,		/* anti-shared */
	BACTION_RETAIN,

	BACTION_RETAIN_AND_FLIP,
	BACTION_KILL,
	BACTION_KILL,
	BACTION_KILL
};

static int		union_actions[8] = {
	BACTION_KILL,
	BACTION_RETAIN,		/* shared */
	BACTION_KILL,		/* anti-shared */
	BACTION_RETAIN,

	BACTION_KILL,
	BACTION_KILL,
	BACTION_KILL,
	BACTION_RETAIN
};

static int		intersect_actions[8] = {
	BACTION_RETAIN,
	BACTION_RETAIN,		/* shared */
	BACTION_RETAIN,		/* anti-shared ==> non-manifold result */
	BACTION_KILL,

	BACTION_RETAIN,
	BACTION_KILL,
	BACTION_KILL,
	BACTION_KILL
};

struct nmg_bool_state  {
	struct shell	*bs_dest;
	struct shell	*bs_src;
	int		bs_isA;		/* true if A, else doing B */
	struct nmg_ptbl	*bs_classtab;
	int		*bs_actions;
};

/*
 *			N M G _ E V A L U A T E _ B O O L E A N
 *
 *  Evaluate a boolean operation on the two shells "A" and "B",
 *  of the form "answer = A op B".
 *  As input, each element (loop-in-face, wire loop, wire edge, vertex)
 *  in both A and B has been classified as being
 *  "in", "on", or "out" of the other shell.
 *  Using these classifications, operate on the input shells.
 *  At the end, shell A contains the resultant object, and
 *  shell B is destroyed.
 *
 */
void
nmg_evaluate_boolean( sA, sB, op, class_table )
struct shell	*sA;
struct shell	*sB;
int		op;
struct nmg_ptbl class_table[];
{
	struct loopuse	*lu;
	struct faceuse	*fu;
	int		*actions;
	int		i;
	struct nmg_bool_state	bool_state;
	struct nmg_counter	counter;

	if (rt_g.NMG_debug & DEBUG_BOOLEVAL) {
		rt_log("nmg_evaluate_boolean(sA=x%x, sB=x%x, op=%d)\n",
			sA, sB, op );
		for( i=0; i<8; i++ )  {
			rt_log("	%6d %s\n",
				class_table[i].end,
				nmg_class_names[i] );
		}
	}

	switch( op )  {
	case NMG_BOOL_SUB:
		actions = subtraction_actions;
		break;
	case NMG_BOOL_ADD:
		actions = union_actions;
		break;
	case NMG_BOOL_ISECT:
		actions = intersect_actions;
		break;
	default:
		actions = union_actions;	/* shut up lint */
		rt_log("ERROR nmg_evaluate_boolean() op=%d.\n", op);
		rt_bomb("bad boolean\n");
	}

	/* Reindex structures before starting evaluation. */
	/* XXX this should be done before starting classification! */
	nmg_m_reindex( sA->r_p->m_p );

	bool_state.bs_dest = sA;
	bool_state.bs_src = sB;
	bool_state.bs_classtab = class_table;
	bool_state.bs_actions = actions;

	bool_state.bs_isA = 1;
	nmg_eval_shell( sA, &bool_state );

	bool_state.bs_isA = 0;
	nmg_eval_shell( sB, &bool_state );

	/* Plot the result */
	if (rt_g.NMG_debug & DEBUG_BOOLEVAL && rt_g.NMG_debug & DEBUG_PLOTEM) {
		FILE	*fp;

		if ((fp=fopen("bool_ans.pl", "w")) == (FILE *)NULL) {
			(void)perror("bool_ans.pl");
			exit(-1);
		}
    		rt_log("plotting bool_ans.pl\n");
		nmg_pl_s( fp, sA );
		(void)fclose(fp);
	}

	if( NMG_LIST_NON_EMPTY( &sB->fu_hd ) )  {
		rt_log("WARNING:  nmg_evaluate_boolean():  shell B still has faces!\n");
	}
	if( NMG_LIST_NON_EMPTY( &sB->lu_hd ) )  {
		rt_log("WARNING:  nmg_evaluate_boolean():  shell B still has wire loops!\n");
	}
	if( NMG_LIST_NON_EMPTY( &sB->eu_hd ) )  {
		rt_log("WARNING:  nmg_evaluate_boolean():  shell B still has wire edges!\n");
	}
	if(sB->vu_p) {
		rt_log("WARNING:  nmg_evaluate_boolean():  shell B still has verts!\n");
	}

	/* Regardless of what is in it, kill shell B */
	nmg_ks( sB );

	/* Remove loops/edges/vertices that appear more than once in result */
	nmg_rm_redundancies( sA );

#if 0
	bzero( (char *)&counter, sizeof(counter) );
	nmg_m_count( &counter, sA->r_p->m_p );
	nmg_pr_count( &counter, "after nmg_rm_redundancies()" );
#endif
}

static int	nmg_eval_count = 0;	/* debug -- plot file numbering */

/*
 *			N M G _ E V A L _ S H E L L
 *
 *  Make a life-and-death decision on every element of a shell.
 *  Descend the "great chain of being" from the face to loop to edge
 *  to vertex, saving or demoting along the way.
 */
nmg_eval_shell( s, bs )
register struct shell	*s;
struct nmg_bool_state *bs;
{
	struct faceuse	*fu;
	struct faceuse	*nextfu;
	struct loopuse	*lu;
	struct loopuse	*nextlu;
	struct edgeuse	*eu;
	struct edgeuse	*nexteu;
	struct vertexuse *vu;
	struct vertex	*v;
	int		loops_retained;
	int		loops_flipped;

	NMG_CK_SHELL(s);

	/*
	 *  For each face in the shell, process all the loops in the face,
	 *  and then handle the face and all loops as a unit.
	 */
	nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
	fu = NMG_LIST_FIRST( faceuse, &s->fu_hd );
	while( NMG_LIST_MORE( fu, faceuse, &s->fu_hd ) )  {
		nextfu = NMG_LIST_PNEXT( faceuse, fu );

		/* Consider this face */
		NMG_CK_FACEUSE(fu);
		NMG_CK_FACE(fu->f_p);
		loops_retained = loops_flipped = 0;
		lu = NMG_LIST_FIRST( loopuse, &fu->lu_hd );
		while( NMG_LIST_MORE( lu, loopuse, &fu->lu_hd ) )  {
			nextlu = NMG_LIST_PNEXT( loopuse, lu );

			NMG_CK_LOOPUSE(lu);
			NMG_CK_LOOP( lu->l_p );
			switch( nmg_eval_action( (genptr_t)lu->l_p, bs ) )  {
			case BACTION_KILL:
				/* Kill by demoting loop to edges */
				if( nmg_demote_lu( lu ) )
					rt_log("nmg_eval_shell() nmg_demote_lu(x%x) fail\n", lu);
	nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
				lu = nextlu;
				continue;
			case BACTION_RETAIN:
				loops_retained++;
				break;
			case BACTION_RETAIN_AND_FLIP:
				loops_flipped++;
				break;
			}
			lu = nextlu;
		}

		if (rt_g.NMG_debug & DEBUG_BOOLEVAL)
			rt_log("faceuse x%x loops retained=%d, flipped=%d\n",
				fu, loops_retained, loops_flipped);

		/*
		 *  Here, faceuse will have 0 or more loopuses still in it.
		 *  Decide the fate of the face;  if the face dies,
		 *  then any remaining loops, edges, etc, will die too.
		 */
		if( NMG_LIST_IS_EMPTY( &fu->lu_hd ) )  {
			/* faceuse is empty, face & mate die */
			if (rt_g.NMG_debug & DEBUG_BOOLEVAL)
		    		rt_log("faceuse x%x empty, kill\n", fu);
			if( NMG_LIST_MORE(fu, faceuse, &s->fu_hd) &&
			    nextfu == fu->fumate_p )
				nextfu = NMG_LIST_PNEXT(faceuse, nextfu);
			nmg_kfu( fu );		/* kill face & mate */
	nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
			fu = nextfu;
			continue;
		}

		if( loops_flipped > 0 )  {
			if( loops_retained > 0 )  {
				rt_log("ERROR nmg_eval_shell() face both retained & flipped?\n");
				/* Just retain un-flipped, for now */
			} else {
				if (rt_g.NMG_debug & DEBUG_BOOLEVAL)
			    		rt_log("faceuse x%x flipped\n", fu);
				nmg_reverse_face( fu );
			}
		} else {
			if( loops_retained > 0 )  {
				if (rt_g.NMG_debug & DEBUG_BOOLEVAL)
			    		rt_log("faceuse x%x retained\n", fu);
			} else {
				rt_bomb("nmg_eval_shell() retaining face with no loops?\n");
			}
		}
		if( fu->s_p != bs->bs_dest )  {
			if (rt_g.NMG_debug & DEBUG_BOOLEVAL)
		    		rt_log("faceuse x%x moved to A shell\n", fu);
			if( NMG_LIST_MORE(fu, faceuse, &s->fu_hd) &&
			    nextfu == fu->fumate_p )
				nextfu = NMG_LIST_PNEXT(faceuse, nextfu);
			nmg_mv_fu_between_shells( bs->bs_dest, s, fu );
			fu = nextfu;
			continue;
		}
		fu = nextfu;
	}

	/*
	 *  For each loop in the shell, process.
	 *  Each loop is either a wire-loop, or a vertex-with-self-loop.
	 *  Only consider wire loops here.
	 */
	nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
	lu = NMG_LIST_FIRST( loopuse, &s->lu_hd );
	while( NMG_LIST_MORE( lu, loopuse, &s->lu_hd ) )  {
		nextlu = NMG_LIST_PNEXT( loopuse, lu );

		NMG_CK_LOOPUSE(lu);
		if( NMG_LIST_FIRST_MAGIC( &lu->down_hd ) == NMG_VERTEXUSE_MAGIC )  {
			/* ignore vertex-with-self-loop */
			lu = nextlu;
			continue;
		}
		NMG_CK_LOOP( lu->l_p );
		switch( nmg_eval_action( (genptr_t)lu->l_p, bs ) )  {
		case BACTION_KILL:
			/* Demote the loopuse into wire edges */
			/* kill loop & mate */
			if( nmg_demote_lu( lu ) )
				rt_log("nmg_eval_shell() nmg_demote_lu(x%x) fail\n", lu);
	nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
			lu = nextlu;
			continue;
		case BACTION_RETAIN:
		case BACTION_RETAIN_AND_FLIP:
			if( lu->up.s_p == bs->bs_dest )  break;
			nmg_mv_lu_between_shells( bs->bs_dest, s, lu );
			lu = nextlu;
			continue;
		}
		lu = nextlu;
	}

	/*
	 *  For each wire-edge in the shell, ...
	 */
	nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
	eu = NMG_LIST_FIRST( edgeuse, &s->eu_hd );
	while( NMG_LIST_MORE( eu, edgeuse, &s->eu_hd ) )  {
		nexteu = NMG_LIST_PNEXT( edgeuse, eu );

		/* Consider this edge */
		NMG_CK_EDGE( eu->e_p );
		switch( nmg_eval_action( (genptr_t)eu->e_p, bs ) )  {
		case BACTION_KILL:
			/* Demote the edegeuse (and mate) into vertices */
			if( NMG_LIST_MORE(eu, edgeuse, &s->eu_hd) &&
			    nexteu == eu->eumate_p )
				nexteu = NMG_LIST_PNEXT(edgeuse, nexteu);
			if( nmg_demote_eu( eu ) )
				rt_log("nmg_eval_shell() nmg_demote_eu(x%x) fail\n", eu);
	nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
			eu = nexteu;
			continue;
		case BACTION_RETAIN:
		case BACTION_RETAIN_AND_FLIP:
			if( eu->up.s_p == bs->bs_dest )  break;
			if( NMG_LIST_MORE(eu, edgeuse, &s->eu_hd) &&
			    nexteu == eu->eumate_p )
				nexteu = NMG_LIST_PNEXT(edgeuse, nexteu);
			nmg_mv_eu_between_shells( bs->bs_dest, s, eu );
			eu = nexteu;
			continue;
		}
		eu = nexteu;
	}

	/*
	 *  For each lone vertex-with-self-loop, process.
	 *  Note that these are intermixed in the loop list.
	 *  Each loop is either a wire-loop, or a vertex-with-self-loop.
	 *  Only consider cases of vertex-with-self-loop here.
	 *
	 *  This case has to be handled separately, because a wire-loop
	 *  may be demoted to a set of wire-edges above, some of which
	 *  may be retained.  The non-retained wire-edges may in turn
	 *  be demoted into vertex-with-self-loop objects above,
	 *  which will be processed here.
	 */
	nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
	lu = NMG_LIST_FIRST( loopuse, &s->lu_hd );
	while( NMG_LIST_MORE( lu, loopuse, &s->lu_hd ) )  {
		nextlu = NMG_LIST_PNEXT( loopuse, lu );

		NMG_CK_LOOPUSE(lu);
		if( NMG_LIST_FIRST_MAGIC( &lu->down_hd ) != NMG_VERTEXUSE_MAGIC )  {
			/* ignore any remaining wire-loops */
			lu = nextlu;
			continue;
		}
		vu = NMG_LIST_PNEXT( vertexuse, &lu->down_hd );
		NMG_CK_VERTEXUSE( vu );
		NMG_CK_VERTEX( vu->v_p );
		switch( nmg_eval_action( (genptr_t)vu->v_p, bs ) )  {
		case BACTION_KILL:
			/* Eliminate the loopuse, and mate */
			if( NMG_LIST_MORE(lu, loopuse, &lu->down_hd) &&
			    nextlu == lu->lumate_p )
				nextlu = NMG_LIST_PNEXT(loopuse, nextlu);
			nmg_klu( lu );
			lu = nextlu;
			continue;
		case BACTION_RETAIN:
		case BACTION_RETAIN_AND_FLIP:
			if( lu->up.s_p == bs->bs_dest )  break;
			if( NMG_LIST_MORE(lu, loopuse, &lu->down_hd) &&
			    nextlu == lu->lumate_p )
				nextlu = NMG_LIST_PNEXT(loopuse, nextlu);
			nmg_mv_lu_between_shells( bs->bs_dest, s, lu );
			lu = nextlu;
			continue;
		}
		lu = nextlu;
	}

	/*
	 * Final case:  shell of a single vertexuse
	 */
	if( vu = s->vu_p )  {
		NMG_CK_VERTEXUSE( vu );
		NMG_CK_VERTEX( vu->v_p );
		switch( nmg_eval_action( (genptr_t)vu->v_p, bs ) )  {
		case BACTION_KILL:
			nmg_kvu( vu );
	nmg_eval_plot( bs, nmg_eval_count++, 0 );	/* debug */
			s->vu_p = (struct vertexuse *)0;	/* sanity */
			break;
		case BACTION_RETAIN:
		case BACTION_RETAIN_AND_FLIP:
			if( vu->up.s_p == bs->bs_dest )  break;
			nmg_mv_vu_between_shells( bs->bs_dest, s, vu );
			s->vu_p = (struct vertexuse *)0;	/* sanity */
			break;
		}
	}
	nmg_eval_plot( bs, nmg_eval_count++, 1 );	/* debug */
}

/*
 *			N M G _ E V A L _ A C T I O N
 *
 *  Given a pointer to some NMG data structure,
 *  search the 4 classification lists to determine it's classification.
 *  (XXX In the future, this should be done with one big array).
 *  Then, return the action code for an item of that classification.
 */
int
nmg_eval_action( ptr, bs )
long				*ptr;
register struct nmg_bool_state	*bs;
{
	register int	ret;
	register int	class;

	if( bs->bs_isA )  {
		if( nmg_tbl( &bs->bs_classtab[NMG_CLASS_AinB], TBL_LOC, ptr ) >= 0 )  {
			class = NMG_CLASS_AinB;
			ret = bs->bs_actions[NMG_CLASS_AinB];
			goto out;
		}
		if( nmg_tbl( &bs->bs_classtab[NMG_CLASS_AonBshared], TBL_LOC, ptr ) >= 0 )  {
			class = NMG_CLASS_AonBshared;
			ret = bs->bs_actions[NMG_CLASS_AonBshared];
			goto out;
		}
		if( nmg_tbl( &bs->bs_classtab[NMG_CLASS_AonBanti], TBL_LOC, ptr ) >= 0 )  {
			class = NMG_CLASS_AonBanti;
			ret = bs->bs_actions[NMG_CLASS_AonBanti];
			goto out;
		}
		if( nmg_tbl( &bs->bs_classtab[NMG_CLASS_AoutB], TBL_LOC, ptr ) >= 0 )  {
			class = NMG_CLASS_AoutB;
			ret = bs->bs_actions[NMG_CLASS_AoutB];
			goto out;
		}
		rt_log("nmg_eval_action(ptr=x%x) %s has no A classification, retaining\n",
			ptr, nmg_identify_magic( *((long *)ptr) ) );
		class = NMG_CLASS_BAD;
		ret = BACTION_RETAIN;
		goto out;
	}

	/* is B */
	if( nmg_tbl( &bs->bs_classtab[NMG_CLASS_BinA], TBL_LOC, ptr ) >= 0 )  {
		class = NMG_CLASS_BinA;
		ret = bs->bs_actions[NMG_CLASS_BinA];
		goto out;
	}
	if( nmg_tbl( &bs->bs_classtab[NMG_CLASS_BonAshared], TBL_LOC, ptr ) >= 0 )  {
		class = NMG_CLASS_BonAshared;
		ret = bs->bs_actions[NMG_CLASS_BonAshared];
		goto out;
	}
	if( nmg_tbl( &bs->bs_classtab[NMG_CLASS_BonAanti], TBL_LOC, ptr ) >= 0 )  {
		class = NMG_CLASS_BonAanti;
		ret = bs->bs_actions[NMG_CLASS_BonAanti];
		goto out;
	}
	if( nmg_tbl( &bs->bs_classtab[NMG_CLASS_BoutA], TBL_LOC, ptr ) >= 0 )  {
		class = NMG_CLASS_BoutA;
		ret = bs->bs_actions[NMG_CLASS_BoutA];
		goto out;
	}
	rt_log("nmg_eval_action(ptr=x%x) %s has no B classification, retaining\n",
		ptr, nmg_identify_magic( *((long *)ptr) ) );
	class = NMG_CLASS_BAD;
	ret = BACTION_RETAIN;
out:
	if (rt_g.NMG_debug & DEBUG_BOOLEVAL) {
		rt_log("nmg_eval_action(ptr=x%x) %s %s %s %s\n",
			ptr, bs->bs_isA ? "A" : "B",
			nmg_identify_magic( *((long *)ptr) ),
			nmg_class_names[class],
			nmg_baction_names[ret] );
	}
	return(ret);
}

/****/

nmg_find_vertex_in_edgelist( v, hd )
register struct vertex	*v;
struct nmg_list		*hd;
{
	register struct edgeuse	*eu;

	NMG_CK_VERTEX(v);
	for( NMG_LIST( eu, edgeuse, hd ) )  {
		NMG_CK_EDGEUSE(eu);
		NMG_CK_VERTEXUSE(eu->vu_p);
		NMG_CK_VERTEX(eu->vu_p->v_p);
		if( eu->vu_p->v_p == v )  return(1);
	}
	return(0);
}

nmg_find_vertex_in_looplist( v, hd, singletons )
register struct vertex	*v;
struct nmg_list		*hd;
int			singletons;
{
	register struct loopuse	*lu;
	long			magic1;

	NMG_CK_VERTEX(v);
	for( NMG_LIST( lu, loopuse, hd ) )  {
		NMG_CK_LOOPUSE(lu);
		magic1 = NMG_LIST_FIRST_MAGIC( &lu->down_hd );
		if( magic1 == NMG_VERTEXUSE_MAGIC )  {
			register struct vertexuse	*vu;
			if( !singletons )  continue;
			vu = NMG_LIST_FIRST(vertexuse, &lu->down_hd );
			NMG_CK_VERTEXUSE(vu);
			NMG_CK_VERTEX(vu->v_p);
			if( vu->v_p == v )  return(1);
		} else if( magic1 == NMG_EDGEUSE_MAGIC )  {
			if( nmg_find_vertex_in_edgelist( v, &lu->down_hd ) )
				return(1);
		} else {
			rt_bomb("nmg_find_vertex_in_loopuse() bad magic\n");
		}
	}
	return(0);
}

nmg_find_vertex_in_facelist( v, hd )
register struct vertex	*v;
struct nmg_list		*hd;
{
	register struct faceuse	*fu;

	NMG_CK_VERTEX(v);
	for( NMG_LIST( fu, faceuse, hd ) )  {
		NMG_CK_FACEUSE(fu);
		if( nmg_find_vertex_in_looplist( v, &fu->lu_hd, 1 ) )
			return(1);
	}
	return(0);
}

nmg_find_edge_in_edgelist( e, hd )
struct edge	*e;
struct nmg_list	*hd;
{
	register struct edgeuse	*eu;

	NMG_CK_EDGE(e);
	for( NMG_LIST( eu, edgeuse, hd ) )  {
		NMG_CK_EDGEUSE(eu);
		NMG_CK_EDGE(eu->e_p);
		if( e == eu->e_p )  return(1);
	}
	return(0);
}

nmg_find_edge_in_looplist( e, hd )
struct edge	*e;
struct nmg_list	*hd;
{
	register struct loopuse	*lu;
	long			magic1;

	NMG_CK_EDGE(e);
	for( NMG_LIST( lu, loopuse, hd ) )  {
		NMG_CK_LOOPUSE(lu);
		magic1 = NMG_LIST_FIRST_MAGIC( &lu->down_hd );
		if( magic1 == NMG_VERTEXUSE_MAGIC )  {
			/* Loop of a single vertex does not have an edge */
			continue;
		} else if( magic1 == NMG_EDGEUSE_MAGIC )  {
			if( nmg_find_edge_in_edgelist( e, &lu->down_hd ) )
				return(1);
		} else {
			rt_bomb("nmg_find_edge_in_loopuse() bad magic\n");
		}
	}
	return(0);
}

nmg_find_edge_in_facelist( e, hd )
struct edge	*e;
struct nmg_list	*hd;
{
	register struct faceuse	*fu;

	NMG_CK_EDGE(e);
	for( NMG_LIST( fu, faceuse, hd ) )  {
		NMG_CK_FACEUSE(fu);
		if( nmg_find_edge_in_looplist( e, &fu->lu_hd ) )
			return(1);
	}
	return(0);
}

nmg_find_loop_in_facelist( l, fu_hd )
struct loop	*l;
struct nmg_list	*fu_hd;
{
	register struct faceuse	*fu;
	register struct loopuse	*lu;

	NMG_CK_LOOP(l);
	for( NMG_LIST( fu, faceuse, fu_hd ) )  {
		NMG_CK_FACEUSE(fu);
		for( NMG_LIST( lu, loopuse, &fu->lu_hd ) )  {
			NMG_CK_LOOPUSE(lu);
			NMG_CK_LOOP(lu->l_p);
			if( l == lu->l_p )  return(1);
		}
	}
	return(0);
}

/*
 *			N M G _ R M _ R E D U N D A N C I E S
 *
 *  Remove all redundant parts between the different "levels" of a shell.
 *  Remove wire loops that match face loops.
 *  Remove wire edges that match edges in wire loops or face loops.
 *  Remove lone vertices (stored as wire loops on a single vertex) that
 *  match vertices in a face loop, wire loop, or wire edge.
 */
nmg_rm_redundancies(s)
struct shell	*s;
{
	struct faceuse	*fu;
	struct loopuse	*lu;
	struct edgeuse	*eu;
	struct vertexuse	*vu;
	long		magic1;

	NMG_CK_SHELL(s);

	if( NMG_LIST_NON_EMPTY( &s->fu_hd ) )  {
		/* Compare wire loops -vs- loops in faces */
		lu = NMG_LIST_FIRST( loopuse, &s->lu_hd );
		while( NMG_LIST_MORE( lu, loopuse, &s->lu_hd ) )  {
			register struct loopuse	*nextlu;
			NMG_CK_LOOPUSE(lu);
			NMG_CK_LOOP(lu->l_p);
			nextlu = NMG_LIST_PNEXT( loopuse, lu );
			if( nmg_find_loop_in_facelist( lu->l_p, &s->fu_hd ) )  {
				/* Dispose of wire loop (and mate)
				 * which match face loop */
				if( NMG_LIST_MORE(lu, loopuse, &lu->down_hd) &&
				    nextlu == lu->lumate_p )
					nextlu = NMG_LIST_PNEXT(loopuse, nextlu);
				nmg_klu( lu );
			}
			lu = nextlu;
		}

		/* Compare wire edges -vs- edges in loops in faces */
		eu = NMG_LIST_FIRST( edgeuse, &s->eu_hd );
		while( NMG_LIST_MORE( eu, edgeuse, &s->eu_hd ) )  {
			register struct edgeuse *nexteu;
			NMG_CK_EDGEUSE(eu);
			NMG_CK_EDGE(eu->e_p);
			nexteu = NMG_LIST_PNEXT( edgeuse, eu );
			if( nmg_find_edge_in_facelist( eu->e_p, &s->fu_hd ) )  {
				/* Dispose of wire edge (and mate) */
				if( NMG_LIST_MORE(eu, edgeuse, &s->eu_hd) &&
				    nexteu == eu->eumate_p )
					nexteu = NMG_LIST_PNEXT(edgeuse, nexteu);
				nmg_keu(eu);
			}
			eu = nexteu;
		}
	}

	/* Compare wire edges -vs- edges in wire loops */
	eu = NMG_LIST_FIRST( edgeuse, &s->eu_hd );
	while( NMG_LIST_MORE( eu, edgeuse, &s->eu_hd ) )  {
		register struct edgeuse *nexteu;
		NMG_CK_EDGEUSE(eu);
		NMG_CK_EDGE(eu->e_p);
		nexteu = NMG_LIST_PNEXT( edgeuse, eu );
		if( nmg_find_edge_in_looplist( eu->e_p, &s->lu_hd ) )  {
			/* Kill edge use and mate */
			if( NMG_LIST_MORE(eu, edgeuse, &s->eu_hd) &&
			    nexteu == eu->eumate_p )
				nexteu = NMG_LIST_PNEXT(edgeuse, nexteu);
			nmg_keu(eu);
		}
		eu = nexteu;
	}

	/* Compare lone vertices against everything else */
	/* Individual vertices are stored as wire loops on a single vertex */
	lu = NMG_LIST_FIRST( loopuse, &s->lu_hd );
	while( NMG_LIST_MORE( lu, loopuse, &s->lu_hd ) )  {
		register struct loopuse	*nextlu;
		NMG_CK_LOOPUSE(lu);
		nextlu = NMG_LIST_PNEXT( loopuse, lu );
		magic1 = NMG_LIST_FIRST_MAGIC( &lu->down_hd );
		if( magic1 != NMG_VERTEXUSE_MAGIC )  {
			lu = nextlu;
			continue;
		}
		vu = NMG_LIST_PNEXT( vertexuse, &lu->down_hd );
		NMG_CK_VERTEXUSE(vu);
		NMG_CK_VERTEX(vu->v_p);
		if( nmg_find_vertex_in_facelist( vu->v_p, &s->fu_hd ) ||
		    nmg_find_vertex_in_looplist( vu->v_p, &s->lu_hd,0 ) ||
		    nmg_find_vertex_in_edgelist( vu->v_p, &s->eu_hd ) )  {
		    	/* Kill lu and mate */
			if( NMG_LIST_MORE(lu, loopuse, &lu->down_hd) &&
			    nextlu == lu->lumate_p )
				nextlu = NMG_LIST_PNEXT(loopuse, nextlu);
			nmg_klu( lu );
			lu = nextlu;
			continue;
		}
	}

	/* There really shouldn't be a lone vertex by now */
	if( s->vu_p )  rt_log("nmg_rm_redundancies() lone vertex?\n");
}

/*****/

nmg_pr_count( ctr, str )
struct nmg_counter	*ctr;
char			*str;
{
	rt_log("nmg_pr_count(%s)\n", str);
	rt_log("\t%6d max_structs\n", ctr->max_structs);
	rt_log("\t%6d regions\n", ctr->regions);
	rt_log("\t%6d shells\n", ctr->shells);
	rt_log("\t%6d faces\n", ctr->faces);
	rt_log("\t%6d face_loops\n", ctr->face_loops);
	rt_log("\t%6d face_edges\n", ctr->face_edges);
	rt_log("\t%6d face_lone_verts\n", ctr->face_lone_verts);
	rt_log("\t%6d wire_loops\n", ctr->wire_loops);
	rt_log("\t%6d wire_loop_edges\n", ctr->wire_loop_edges);
	rt_log("\t%6d wire_edges\n", ctr->wire_edges);
	rt_log("\t%6d lone_verts\n", ctr->lone_verts);
}

nmg_m_count( ctr, m )
struct model		*m;
struct nmg_counter	*ctr;
{
	struct nmgregion	*r;
	struct shell		*s;
	struct faceuse		*fu;
	struct loopuse		*lu;
	struct edgeuse		*eu;

	NMG_CK_MODEL(m);
	ctr->max_structs = m->maxindex;
	for( NMG_LIST( r, nmgregion, &m->r_hd ) )  {
		NMG_CK_REGION(r);
		ctr->regions++;
		for( NMG_LIST( s, shell, &r->s_hd ) )  {
			NMG_CK_SHELL(s);
			ctr->shells++;
			/* Faces in shell */
			for( NMG_LIST( fu, faceuse, &s->fu_hd ) )  {
				NMG_CK_FACEUSE(fu);
				ctr->faces++;
				for( NMG_LIST( lu, loopuse, &fu->lu_hd ) )  {
					NMG_CK_LOOPUSE(lu);
					if( NMG_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_VERTEXUSE_MAGIC )  {
						ctr->face_lone_verts++;
						continue;
					}
					ctr->face_loops++;
					for( NMG_LIST( eu, edgeuse, &lu->down_hd ) )  {
						NMG_CK_EDGEUSE(eu);
						ctr->face_edges++;
					}
				}
			}
			/* Wire loops in shell */
			for( NMG_LIST( lu, loopuse, &s->lu_hd ) )  {
				NMG_CK_LOOPUSE(lu);
				if( NMG_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_VERTEXUSE_MAGIC )  {
					ctr->lone_verts++;
					continue;
				}
				ctr->wire_loops++;
				for( NMG_LIST( eu, edgeuse, &lu->down_hd ) )  {
					NMG_CK_EDGEUSE(eu);
					ctr->wire_loop_edges++;
				}
			}
			/* Wire edges in shell */
			for( NMG_LIST( eu, edgeuse, &s->eu_hd ) )  {
				NMG_CK_EDGEUSE(eu);
				ctr->wire_edges++;
			}
		}
	}
}

/*
 *			N M G _ E V A L _ P L O T
 *
 *  Called from nmg_eval_shell
 */
nmg_eval_plot( bs, num, delay )
struct nmg_bool_state	*bs;
int		num;
int		delay;
{
	FILE	*fp;
	char	fname[128];
	struct faceuse	*fu;
	struct nmg_ptbl b;
	int	do_plot = 0;
	int	do_anim = 0;

	if (rt_g.NMG_debug & DEBUG_BOOLEVAL && rt_g.NMG_debug & DEBUG_PLOTEM)
		do_plot = 1;
	if( rt_g.NMG_debug & DEBUG_PL_ANIM )  do_anim = 1;

	if( !do_plot && !do_anim )  return;

	if( do_plot )  {
		/* get space for list of items processed */
		(void)nmg_tbl(&b, TBL_INIT, (long *)0);	

		sprintf(fname, "/tmp/nmg_eval%d.pl", num);
		if( (fp = fopen(fname,"w")) == NULL )  {
			perror(fname);
			return;
		}
		rt_log("Plotting %s\n", fname);

		nmg_pl_s( fp, bs->bs_dest );
		nmg_pl_s( fp, bs->bs_src );

		fclose(fp);

		(void)nmg_tbl(&b, TBL_FREE, (long *)NULL);
	}

	if( do_anim )  {
		extern void (*nmg_vlblock_anim_upcall)();
		struct rt_vlblock	*vbp;

		/* get space for list of items processed */
		(void)nmg_tbl(&b, TBL_INIT, (long *)0);	
		vbp = rt_vlblock_init();

		nmg_vlblock_s( vbp, bs->bs_dest, 0 );
		nmg_vlblock_s( vbp, bs->bs_src, 0 );

		(void)nmg_tbl(&b, TBL_FREE, (long *)NULL);

		/* Cause animation of boolean operation as it proceeds! */
		if( nmg_vlblock_anim_upcall )  {
#if 0
			/* if requested, delay 1/4 second */
			(*nmg_vlblock_anim_upcall)( vbp,
				delay ? 250000 : 0 );
#else
			/* Go full speed */
			(*nmg_vlblock_anim_upcall)( vbp, 0 );
#endif
		} else {
			rt_log("null nmg_vlblock_anim_upcall, no animation\n");
		}
		rt_vlblock_free(vbp);
	}
}

#define NMG_HIGH_BIT	0x80000000

/*
 *			N M G _ M _ R E I N D E X
 *
 *  Reassign index numbers to all the data structures in a model.
 *  The model structure will get index 0, all others will be sequentially
 *  assigned after that.
 */
void
nmg_m_reindex( m )
struct model	*m;
{
	struct nmgregion	*r;
	struct shell		*s;
	struct faceuse		*fu;
	struct loopuse		*lu;
	register struct edgeuse		*eu;
	register struct vertexuse	*vu;
	register int		newindex;

	NMG_CK_MODEL(m);
	if( m->index != 0 )  rt_log("nmg_m_reindex() m->index=%d\n", m->index);

	/* First pass:  just set the high bit on all index words */
	for( NMG_LIST( r, nmgregion, &m->r_hd ) )  {
		NMG_CK_REGION(r);
		r->index |= NMG_HIGH_BIT;
		for( NMG_LIST( s, shell, &r->s_hd ) )  {
			NMG_CK_SHELL(s);
			s->index |= NMG_HIGH_BIT;
			/* Faces in shell */
			for( NMG_LIST( fu, faceuse, &s->fu_hd ) )  {
				NMG_CK_FACEUSE(fu);
				fu->index |= NMG_HIGH_BIT;
				fu->f_p->index |= NMG_HIGH_BIT;
				/* Loops in face */
				for( NMG_LIST( lu, loopuse, &fu->lu_hd ) )  {
					NMG_CK_LOOPUSE(lu);
					lu->index |= NMG_HIGH_BIT;
					lu->l_p->index |= NMG_HIGH_BIT;
					if( NMG_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_VERTEXUSE_MAGIC )  {
						/* Loop of Lone vertex */
						vu = NMG_LIST_FIRST( vertexuse, &lu->down_hd );
						NMG_CK_VERTEXUSE(vu);
						vu->index |= NMG_HIGH_BIT;
						vu->v_p->index |= NMG_HIGH_BIT;
						continue;
					}
					for( NMG_LIST( eu, edgeuse, &lu->down_hd ) )  {
						NMG_CK_EDGEUSE(eu);
						eu->index |= NMG_HIGH_BIT;
						eu->e_p->index |= NMG_HIGH_BIT;
						vu = eu->vu_p;
						NMG_CK_VERTEXUSE(vu);
						vu->index |= NMG_HIGH_BIT;
						vu->v_p->index |= NMG_HIGH_BIT;
					}
				}
			}
			/* Wire loops in shell */
			for( NMG_LIST( lu, loopuse, &s->lu_hd ) )  {
				NMG_CK_LOOPUSE(lu);
				lu->index |= NMG_HIGH_BIT;
				lu->l_p->index |= NMG_HIGH_BIT;
				if( NMG_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_VERTEXUSE_MAGIC )  {
					/* Wire loop of Lone vertex */
					vu = NMG_LIST_FIRST( vertexuse, &lu->down_hd );
					NMG_CK_VERTEXUSE(vu);
					vu->index |= NMG_HIGH_BIT;
					vu->v_p->index |= NMG_HIGH_BIT;
					continue;
				}
				for( NMG_LIST( eu, edgeuse, &lu->down_hd ) )  {
					NMG_CK_EDGEUSE(eu);
					eu->index |= NMG_HIGH_BIT;
					eu->e_p->index |= NMG_HIGH_BIT;
					vu = eu->vu_p;
					NMG_CK_VERTEXUSE(vu);
					vu->index |= NMG_HIGH_BIT;
					vu->v_p->index |= NMG_HIGH_BIT;
				}
			}
			/* Wire edges in shell */
			for( NMG_LIST( eu, edgeuse, &s->eu_hd ) )  {
				NMG_CK_EDGEUSE(eu);
				eu->index |= NMG_HIGH_BIT;
				eu->e_p->index |= NMG_HIGH_BIT;
				vu = eu->vu_p;
				NMG_CK_VERTEXUSE(vu);
				vu->index |= NMG_HIGH_BIT;
				vu->v_p->index |= NMG_HIGH_BIT;
			}
			/* Lone vertex in shell */
			if( vu = s->vu_p )  {
				NMG_CK_VERTEXUSE(vu);
				vu->index |= NMG_HIGH_BIT;
				vu->v_p->index |= NMG_HIGH_BIT;
			}
		}
	}

#define	NMG_ASSIGN_NEW_INDEX(_p)	\
	{ if( ((_p)->index & NMG_HIGH_BIT) != 0 ) \
		(_p)->index = newindex++; }

	/* Second pass:  assign new index number */
	newindex = 1;	/* model remains index 0 */
	for( NMG_LIST( r, nmgregion, &m->r_hd ) )  {
		NMG_CK_REGION(r);
		NMG_ASSIGN_NEW_INDEX(r);
		for( NMG_LIST( s, shell, &r->s_hd ) )  {
			NMG_CK_SHELL(s);
			NMG_ASSIGN_NEW_INDEX(s);
			/* Faces in shell */
			for( NMG_LIST( fu, faceuse, &s->fu_hd ) )  {
				NMG_CK_FACEUSE(fu);
				NMG_ASSIGN_NEW_INDEX(fu);
				NMG_ASSIGN_NEW_INDEX(fu->f_p);
				/* Loops in face */
				for( NMG_LIST( lu, loopuse, &fu->lu_hd ) )  {
					NMG_CK_LOOPUSE(lu);
					NMG_ASSIGN_NEW_INDEX(lu);
					NMG_ASSIGN_NEW_INDEX(lu->l_p);
					if( NMG_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_VERTEXUSE_MAGIC )  {
						/* Loop of Lone vertex */
						vu = NMG_LIST_FIRST( vertexuse, &lu->down_hd );
						NMG_CK_VERTEXUSE(vu);
						NMG_ASSIGN_NEW_INDEX(vu);
						NMG_ASSIGN_NEW_INDEX(vu->v_p);
						continue;
					}
					for( NMG_LIST( eu, edgeuse, &lu->down_hd ) )  {
						NMG_CK_EDGEUSE(eu);
						NMG_ASSIGN_NEW_INDEX(eu);
						NMG_ASSIGN_NEW_INDEX(eu->e_p);
						vu = eu->vu_p;
						NMG_CK_VERTEXUSE(vu);
						NMG_ASSIGN_NEW_INDEX(vu);
						NMG_ASSIGN_NEW_INDEX(vu->v_p);
					}
				}
			}
			/* Wire loops in shell */
			for( NMG_LIST( lu, loopuse, &s->lu_hd ) )  {
				NMG_CK_LOOPUSE(lu);
				NMG_ASSIGN_NEW_INDEX(lu);
				NMG_ASSIGN_NEW_INDEX(lu->l_p);
				if( NMG_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_VERTEXUSE_MAGIC )  {
					/* Wire loop of Lone vertex */
					vu = NMG_LIST_FIRST( vertexuse, &lu->down_hd );
					NMG_CK_VERTEXUSE(vu);
					NMG_ASSIGN_NEW_INDEX(vu);
					NMG_ASSIGN_NEW_INDEX(vu->v_p);
					continue;
				}
				for( NMG_LIST( eu, edgeuse, &lu->down_hd ) )  {
					NMG_CK_EDGEUSE(eu);
					NMG_ASSIGN_NEW_INDEX(eu);
					NMG_ASSIGN_NEW_INDEX(eu->e_p);
					vu = eu->vu_p;
					NMG_CK_VERTEXUSE(vu);
					NMG_ASSIGN_NEW_INDEX(vu);
					NMG_ASSIGN_NEW_INDEX(vu->v_p);
				}
			}
			/* Wire edges in shell */
			for( NMG_LIST( eu, edgeuse, &s->eu_hd ) )  {
				NMG_CK_EDGEUSE(eu);
				NMG_ASSIGN_NEW_INDEX(eu);
				NMG_ASSIGN_NEW_INDEX(eu->e_p);
				vu = eu->vu_p;
				NMG_CK_VERTEXUSE(vu);
				NMG_ASSIGN_NEW_INDEX(vu);
				NMG_ASSIGN_NEW_INDEX(vu->v_p);
			}
			/* Lone vertex in shell */
			if( vu = s->vu_p )  {
				NMG_CK_VERTEXUSE(vu);
				NMG_ASSIGN_NEW_INDEX(vu);
				NMG_ASSIGN_NEW_INDEX(vu->v_p);
			}
		}
	}
#if 0
rt_log("nmg_m_reindex() oldmax=%d, newmax=%d\n", m->maxindex, newindex );
#endif
	m->maxindex = newindex;
}
