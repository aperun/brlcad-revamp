/*
 *			M A T E R . C
 *
 *  Interface for writing region-id-based color tables to the database.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Notice -
 *	Re-distribution of this software is restricted, as described in
 *	your "Statement of Terms and Conditions for the Release of
 *	The BRL-CAD Package" license agreement.
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 2000 by the United States Army
 *	in all countries except the USA.  All rights reserved.
 */
#ifndef lint
static const char RCSid[] = "@(#)$Header$ (ARL)";
#endif

#include "conf.h"

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "bn.h"
#include "rtgeom.h"
#include "db.h"
#include "raytrace.h"
#include "wdb.h"
#include "mater.h"

/*
 *  Given that the color table has been built up by successive calls
 *  to rt_color_addrec(),  write it into the database.
 */
void
mk_write_color_table( struct rt_wdb *ofp )
{
#if 0
	register struct mater *mp;
	union record	record;

	BU_ASSERT_LONG( mk_version, ==, 4 );

	for( mp = rt_material_head; mp != MATER_NULL; mp = mp->mt_forw )  {
		record.md.md_id = ID_MATERIAL;
		record.md.md_flags = 0;
		record.md.md_low = mp->mt_low;
		record.md.md_hi = mp->mt_high;
		record.md.md_r = mp->mt_r;
		record.md.md_g = mp->mt_g;
		record.md.md_b = mp->mt_b;

		/* This record has no name field! */

/* XXX examine mged/mater.c: color_putrec() */

		/* Write out the record */
		(void)fwrite( (char *)&record, sizeof record, 1, ofpxx );
	}
#else
	bu_bomb("mk_write_color_tale() not yet implemented\n");
#endif
}
