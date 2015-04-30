/*                      R A Y T R A C E . H
 * BRL-CAD
 *
 * Copyright (c) 1993-2014 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @addtogroup librt */
/** @{ */
/** @file raytrace.h
 *
 * All the data structures and manifest constants necessary for
 * interacting with the BRL-CAD LIBRT ray-tracing library.
 *
 * Note that this header file defines many internal data structures,
 * as well as the library's external (interface) data structures.
 * These are provided for the convenience of applications builders.
 * However, the internal data structures are subject to change in each
 * release.
 *
 */

/* TODO - put together a dot file mapping the relationships between
 * high level rt structures and include it in the doxygen comments
 * with the \dotfile command:
 * http://www.stack.nl/~dimitri/doxygen/manual/commands.html#cmddotfile*/

#ifndef RAYTRACE_H
#define RAYTRACE_H

#include "common.h"

/* interface headers */
#include "tcl.h"
#include "bu/avs.h"
#include "bu/bitv.h"
#include "bu/bu_tcl.h"
#include "bu/file.h"
#include "bu/hash.h"
#include "bu/hist.h"
#include "bu/malloc.h"
#include "bu/mapped_file.h"
#include "bu/list.h"
#include "bu/log.h"
#include "bu/parallel.h" /* needed for BU_SEM_LAST */
#include "bu/parse.h"
#include "bu/ptbl.h"
#include "bu/str.h"
#include "bu/vls.h"
#include "bn.h"
#include "db5.h"
#include "nmg.h"
#include "pc.h"
#include "rtgeom.h"


#include "rt/defines.h"
#include "rt/db_fullpath.h"


__BEGIN_DECLS

#include "rt/debug.h"

#include "rt/tol.h"

#include "rt/db_internal.h"

#include "rt/xray.h"

#include "rt/hit.h"

#include "rt/seg.h"

#include "rt/soltab.h"

#include "rt/mater.h"

#include "rt/region.h"

#include "rt/ray_partition.h"

#include "rt/space_partition.h"

#include "rt/mem.h"

#include "rt/db_instance.h"

#include "rt/directory.h"

#include "rt/nongeom.h"

#include "rt/tree.h"

#include "rt/wdb.h"

#include "rt/anim.h"

#include "rt/piece.h"

#include "rt/resource.h"

#include "rt/application.h"


/**
 * Definitions for librt.a which are global to the library regardless
 * of how many different models are being worked on
 */
struct rt_g {
    uint32_t		debug;		/**< @brief  !0 for debug, see librt/debug.h */
    /* DEPRECATED:  rtg_parallel is not used by LIBRT any longer (and will be removed) */
    int8_t		rtg_parallel;	/**< @brief  !0 = trying to use multi CPUs */
    struct bu_list	rtg_vlfree;	/**< @brief  head of bn_vlist freelist */
    uint32_t		NMG_debug;	/**< @brief  debug bits for NMG's see nmg.h */
    struct rt_wdb	rtg_headwdb;	/**< @brief  head of database object list */
};
#define RT_G_INIT_ZERO { 0, 0, BU_LIST_INIT_ZERO, 0, RT_WDB_INIT_ZERO }


/**
 * global ray-trace geometry state
 */
RT_EXPORT extern struct rt_g RTG;

#include "rt/rt_instance.h"


#define RT_NU_GFACTOR_DEFAULT	1.5	 /**< @brief  see rt_cut_it() for a description
					    of this */

#define	RT_PART_NUBSPT	0
#define RT_PART_NUGRID	1

/**
 * Applications that are going to use RT_ADD_VLIST and RT_GET_VLIST
 * are required to execute this macro once, first:
 *
 * BU_LIST_INIT(&RTG.rtg_vlfree);
 *
 * Note that RT_GET_VLIST and RT_FREE_VLIST are non-PARALLEL.
 */
#define RT_GET_VLIST(p) BN_GET_VLIST(&RTG.rtg_vlfree, p)

/** Place an entire chain of bn_vlist structs on the freelist */
#define RT_FREE_VLIST(hd) BN_FREE_VLIST(&RTG.rtg_vlfree, hd)

#define RT_ADD_VLIST(hd, pnt, draw) BN_ADD_VLIST(&RTG.rtg_vlfree, hd, pnt, draw)

/** Set a point size to apply to the vlist elements that follow. */
#define RT_VLIST_SET_POINT_SIZE(hd, size) BN_VLIST_SET_POINT_SIZE(&RTG.rtg_vlfree, hd, size)

/** Set a line width to apply to the vlist elements that follow. */
#define RT_VLIST_SET_LINE_WIDTH(hd, width) BN_VLIST_SET_LINE_WIDTH(&RTG.rtg_vlfree, hd, width)


/*
 * Replacements for definitions from vmath.h
 */
#undef V2PRINT
#undef VPRINT
#undef HPRINT
#define V2PRINT(a, b) bu_log("%s (%g, %g)\n", a, (b)[0], (b)[1]);
#define VPRINT(a, b) bu_log("%s (%g, %g, %g)\n", a, (b)[0], (b)[1], (b)[2])
#define HPRINT(a, b) bu_log("%s (%g, %g, %g, %g)\n", a, (b)[0], (b)[1], (b)[2], (b)[3])

/**
 * Table for driving generic command-parsing routines
 */
struct command_tab {
    const char *ct_cmd;
    const char *ct_parms;
    const char *ct_comment;
    int	(*ct_func)(const int, const char **);
    int	ct_min;		/**< @brief  min number of words in cmd */
    int	ct_max;		/**< @brief  max number of words in cmd */
};


/**
 * Used by MGED for labeling vertices of a solid.
 *
 * TODO - eventually this should fade into a general annotation
 * framework
 */
struct rt_point_labels {
    char str[8];
    point_t pt;
};


#include "rt/view.h"

#include "rt/functab.h"

#include "rt/private.h"

#include "rt/nmg.h"

/*****************************************************************
 *                                                               *
 *          Applications interface to the RT library             *
 *                                                               *
 *****************************************************************/

#include "rt/overlap.h"

#include "rt/pattern.h"

#include "rt/shoot.h"

#include "rt/timer.h"


/* apply a matrix transformation */
/**
 * apply a matrix transformation to a given input object, setting the
 * resultant transformed object as the output solid.  if freeflag is
 * set, the input object will be released.
 *
 * returns zero if matrix transform was applied, non-zero on failure.
 */
RT_EXPORT extern int rt_matrix_transform(struct rt_db_internal *output, const mat_t matrix, struct rt_db_internal *input, int free_input, struct db_i *dbip, struct resource *resource);

#include "rt/boolweave.h"

/* extend a cut box */

/* cut.c */

RT_EXPORT extern void rt_pr_cut_info(const struct rt_i	*rtip,
				     const char		*str);
RT_EXPORT extern void remove_from_bsp(struct soltab *stp,
				      union cutter *cutp,
				      struct bn_tol *tol);
RT_EXPORT extern void insert_in_bsp(struct soltab *stp,
				    union cutter *cutp);
RT_EXPORT extern void fill_out_bsp(struct rt_i *rtip,
				   union cutter *cutp,
				   struct resource *resp,
				   fastf_t bb[6]);

/**
 * Add a solid into a given boxnode, extending the lists there.  This
 * is used only for building the root node, which will then be
 * subdivided.
 *
 * Solids with pieces go onto a special list.
 */
RT_EXPORT extern void rt_cut_extend(union cutter *cutp,
				    struct soltab *stp,
				    const struct rt_i *rtip);
/* find RPP of one region */

/**
 * Calculate the bounding RPP for a region given the name of the
 * region node in the database.  See remarks in _rt_getregion() above
 * for name conventions.  Returns 0 for failure (and prints a
 * diagnostic), or 1 for success.
 */
RT_EXPORT extern int rt_rpp_region(struct rt_i *rtip,
				   const char *reg_name,
				   fastf_t *min_rpp,
				   fastf_t *max_rpp);

/**
 * Compute the intersections of a ray with a rectangular
 * parallelepiped (RPP) that has faces parallel to the coordinate
 * planes
 *
 * The algorithm here was developed by Gary Kuehl for GIFT.  A good
 * description of the approach used can be found in "??" by XYZZY and
 * Barsky, ACM Transactions on Graphics, Vol 3 No 1, January 1984.
 *
 * Note: The computation of entry and exit distance is mandatory, as
 * the final test catches the majority of misses.
 *
 * Note: A hit is returned if the intersect is behind the start point.
 *
 * Returns -
 * 0 if ray does not hit RPP,
 * !0 if ray hits RPP.
 *
 * Implicit return -
 * rp->r_min = dist from start of ray to point at which ray ENTERS solid
 * rp->r_max = dist from start of ray to point at which ray LEAVES solid
 */
RT_EXPORT extern int rt_in_rpp(struct xray *rp,
			       const fastf_t *invdir,
			       const fastf_t *min,
			       const fastf_t *max);


/**
 * Return pointer to cell 'n' along a given ray.  Used for debugging
 * of how space partitioning interacts with shootray.  Intended to
 * mirror the operation of rt_shootray().  The first cell is 0.
 */
RT_EXPORT extern const union cutter *rt_cell_n_on_ray(struct application *ap,
						      int n);

/*
 * The rtip->rti_CutFree list can not be freed directly because is
 * bulk allocated.  Fortunately, we have a list of all the
 * bu_malloc()'ed blocks.  This routine may be called before the first
 * frame is done, so it must be prepared for uninitialized items.
 */
RT_EXPORT extern void rt_cut_clean(struct rt_i *rtip);

/* Find the bounding box given a struct rt_db_internal : bbox.c */

/**
 *
 * Calculate the bounding RPP of the internal format passed in 'ip'.
 * The bounding RPP is returned in rpp_min and rpp_max in mm FIXME:
 * This function needs to be modified to eliminate the rt_gettree()
 * call and the related parameters. In that case calling code needs to
 * call another function before calling this function That function
 * must create a union tree with tr_a.tu_op=OP_SOLID. It can look as
 * follows : union tree * rt_comb_tree(const struct db_i *dbip, const
 * struct rt_db_internal *ip). The tree is set in the struct
 * rt_db_internal * ip argument.  Once a suitable tree is set in the
 * ip, then this function can be called with the struct rt_db_internal
 * * to return the BB properly without getting stuck during tree
 * traversal in rt_bound_tree()
 *
 * Returns -
 *  0 success
 * -1 failure, the model bounds could not be got
 *
 */
RT_EXPORT extern int rt_bound_internal(struct db_i *dbip,
				       struct directory *dp,
				       point_t rpp_min,
				       point_t rpp_max);

/* cmd.c */
/* Read semi-colon terminated line */

/*
 * Read one semi-colon terminated string of arbitrary length from the
 * given file into a dynamically allocated buffer.  Various commenting
 * and escaping conventions are implemented here.
 *
 * Returns:
 * NULL on EOF
 * char * on good read
 */
RT_EXPORT extern char *rt_read_cmd(FILE *fp);
/* do cmd from string via cmd table */

/*
 * Slice up input buffer into whitespace separated "words", look up
 * the first word as a command, and if it has the correct number of
 * args, call that function.  Negative min/max values in the tp
 * command table effectively mean that they're not bounded.
 *
 * Expected to return -1 to halt command processing loop.
 *
 * Based heavily on mged/cmd.c by Chuck Kennedy.
 *
 * DEPRECATED: needs to migrate to libbu
 */
RT_EXPORT extern int rt_do_cmd(struct rt_i *rtip,
			       const char *ilp,
			       const struct command_tab *tp);

/* The database library */

/* wdb.c */
/** @addtogroup wdb */
/** @{ */
/** @file librt/wdb.c
 *
 * Routines to allow libwdb to use librt's import/export interface,
 * rather than having to know about the database formats directly.
 *
 */
RT_EXPORT extern struct rt_wdb *wdb_fopen(const char *filename);


/**
 * Create a libwdb output stream destined for a disk file.  This will
 * destroy any existing file by this name, and start fresh.  The file
 * is then opened in the normal "update" mode and an in-memory
 * directory is built along the way, allowing retrievals and object
 * replacements as needed.
 *
 * Users can change the database title by calling: ???
 */
RT_EXPORT extern struct rt_wdb *wdb_fopen_v(const char *filename,
					    int version);


/**
 * Create a libwdb output stream destined for an existing BRL-CAD
 * database, already opened via a db_open() call.
 *
 * RT_WDB_TYPE_DB_DISK Add to on-disk database
 * RT_WDB_TYPE_DB_DISK_APPEND_ONLY Add to on-disk database, don't clobber existing names, use prefix
 * RT_WDB_TYPE_DB_INMEM Add to in-memory database only
 * RT_WDB_TYPE_DB_INMEM_APPEND_ONLY Ditto, but give errors if name in use.
 */
RT_EXPORT extern struct rt_wdb *wdb_dbopen(struct db_i *dbip,
					   int mode);


/**
 * Returns -
 *  0 and modified *internp;
 * -1 ft_import failure (from rt_db_get_internal)
 * -2 db_get_external failure (from rt_db_get_internal)
 * -3 Attempt to import from write-only (stream) file.
 * -4 Name not found in database TOC.
 *
 * NON-PARALLEL because of rt_uniresource
 */
RT_EXPORT extern int wdb_import(struct rt_wdb *wdbp,
				struct rt_db_internal *internp,
				const char *name,
				const mat_t mat);


/**
 * The caller must free "ep".
 *
 * Returns -
 *  0 OK
 * <0 error
 */
RT_EXPORT extern int wdb_export_external(struct rt_wdb *wdbp,
					 struct bu_external *ep,
					 const char *name,
					 int flags,
					 unsigned char minor_type);


/**
 * Convert the internal representation of a solid to the external one,
 * and write it into the database.
 *
 * The internal representation is always freed.  This is the analog of
 * rt_db_put_internal() for rt_wdb objects.
 *
 * Use this routine in preference to wdb_export() whenever the caller
 * already has an rt_db_internal structure handy.
 *
 * NON-PARALLEL because of rt_uniresource
 *
 * Returns -
 *  0 OK
 * <0 error
 */
RT_EXPORT extern int wdb_put_internal(struct rt_wdb *wdbp,
				      const char *name,
				      struct rt_db_internal *ip,
				      double local2mm);


/**
 * Export an in-memory representation of an object, as described in
 * the file h/rtgeom.h, into the indicated database.
 *
 * The internal representation (gp) is always freed.
 *
 * WARNING: The caller must be careful not to double-free gp,
 * particularly if it's been extracted from an rt_db_internal, e.g. by
 * passing intern.idb_ptr for gp.
 *
 * If the caller has an rt_db_internal structure handy already, they
 * should call wdb_put_internal() directly -- this is a convenience
 * routine intended primarily for internal use in LIBWDB.
 *
 * Returns -
 *  0 OK
 * <0 error
 */
RT_EXPORT extern int wdb_export(struct rt_wdb *wdbp,
				const char *name,
				void *gp,
				int id,
				double local2mm);
RT_EXPORT extern void wdb_init(struct rt_wdb *wdbp,
			       struct db_i   *dbip,
			       int           mode);


/**
 * Release from associated database "file", destroy dynamic data
 * structure.
 */
RT_EXPORT extern void wdb_close(struct rt_wdb *wdbp);


/**
 * Given the name of a database object or a full path to a leaf
 * object, obtain the internal form of that leaf.  Packaged separately
 * mainly to make available nice Tcl error handling.
 *
 * Returns -
 * BRLCAD_OK
 * BRLCAD_ERROR
 */
RT_EXPORT extern int wdb_import_from_path(struct bu_vls *logstr,
					  struct rt_db_internal *ip,
					  const char *path,
					  struct rt_wdb *wdb);


/**
 * Given the name of a database object or a full path to a leaf
 * object, obtain the internal form of that leaf.  Packaged separately
 * mainly to make available nice Tcl error handling. Additionally,
 * copies ts.ts_mat to matp.
 *
 * Returns -
 * BRLCAD_OK
 * BRLCAD_ERROR
 */
RT_EXPORT extern int wdb_import_from_path2(struct bu_vls *logstr,
					   struct rt_db_internal *ip,
					   const char *path,
					   struct rt_wdb *wdb,
					   matp_t matp);

/** @} */

/* db_anim.c */
RT_EXPORT extern struct animate *db_parse_1anim(struct db_i     *dbip,
						int             argc,
						const char      **argv);


/**
 * A common parser for mged and rt.  Experimental.  Not the best name
 * for this.
 */
RT_EXPORT extern int db_parse_anim(struct db_i     *dbip,
				   int             argc,
				   const char      **argv);

/**
 * Add a user-supplied animate structure to the end of the chain of
 * such structures hanging from the directory structure of the last
 * node of the path specifier.  When 'root' is non-zero, this matrix
 * is located at the root of the tree itself, rather than an arc, and
 * is stored differently.
 *
 * In the future, might want to check to make sure that callers
 * directory references are in the right database (dbip).
 */
RT_EXPORT extern int db_add_anim(struct db_i *dbip,
				 struct animate *anp,
				 int root);

/**
 * Perform the one animation operation.  Leave results in form that
 * additional operations can be cascaded.
 *
 * Note that 'materp' may be a null pointer, signifying that the
 * region has already been finalized above this point in the tree.
 */
RT_EXPORT extern int db_do_anim(struct animate *anp,
				mat_t stack,
				mat_t arc,
				struct mater_info *materp);

/**
 * Release chain of animation structures
 *
 * An unfortunate choice of name.
 */
RT_EXPORT extern void db_free_anim(struct db_i *dbip);

/**
 * Writes 'count' bytes into at file offset 'offset' from buffer at
 * 'addr'.  A wrapper for the UNIX write() sys-call that takes into
 * account syscall semaphores, stdio-only machines, and in-memory
 * buffering.
 *
 * Returns -
 * 0 OK
 * -1 FAILURE
 */
/* should be HIDDEN */
RT_EXPORT extern void db_write_anim(FILE *fop,
				    struct animate *anp);

/**
 * Parse one "anim" type command into an "animate" structure.
 *
 * argv[1] must be the "a/b" path spec,
 * argv[2] indicates what is to be animated on that arc.
 */
RT_EXPORT extern struct animate *db_parse_1anim(struct db_i *dbip,
						int argc,
						const char **argv);


/**
 * Free one animation structure
 */
RT_EXPORT extern void db_free_1anim(struct animate *anp);

/* search.c */

#include "./rt/search.h"

/* db_open.c */
/**
 * Ensure that the on-disk database has been completely written out of
 * the operating system's cache.
 */
RT_EXPORT extern void db_sync(struct db_i	*dbip);


/**
 * for db_open(), open the specified file as read-only
 */
#define DB_OPEN_READONLY "r"

/**
 * for db_open(), open the specified file as read-write
 */
#define DB_OPEN_READWRITE "rw"

/**
 * Open the named database.
 *
 * The 'name' parameter specifies the file or filepath to a .g
 * geometry database file for reading and/or writing.
 *
 * The 'mode' parameter specifies whether to open read-only or in
 * read-write mode, specified via the DB_OPEN_READONLY and
 * DB_OPEN_READWRITE symbols respectively.
 *
 * As a convenience, the returned db_t structure's dbi_filepath field
 * is a C-style argv array of dirs to search when attempting to open
 * related files (such as data files for EBM solids or texture-maps).
 * The default values are "." and the directory containing the ".g"
 * file.  They may be overridden by setting the environment variable
 * BRLCAD_FILE_PATH.
 *
 * Returns:
 * DBI_NULL error
 * db_i * success
 */
RT_EXPORT extern struct db_i *
db_open(const char *name, const char *mode);


/* create a new model database */
/**
 * Create a new database containing just a header record, regardless
 * of whether the database previously existed or not, and open it for
 * reading and writing.
 *
 * This routine also calls db_dirbuild(), so the caller doesn't need
 * to.
 *
 * Returns:
 * DBI_NULL on error
 * db_i * on success
 */
RT_EXPORT extern struct db_i *db_create(const char *name,
					int version);

/* close a model database */
/**
 * De-register a client of this database instance, if provided, and
 * close out the instance.
 */
RT_EXPORT extern void db_close_client(struct db_i *dbip,
				      long *client);

/**
 * Close a database, releasing dynamic memory Wait until last user is
 * done, though.
 */
RT_EXPORT extern void db_close(struct db_i *dbip);


/* dump a full copy of a database */
/**
 * Dump a full copy of one database into another.  This is a good way
 * of committing a ".inmem" database to a ".g" file.  The input is a
 * database instance, the output is a LIBWDB object, which could be a
 * disk file or another database instance.
 *
 * Returns -
 * -1 error
 * 0 success
 */
RT_EXPORT extern int db_dump(struct rt_wdb *wdbp,
			     struct db_i *dbip);

/**
 * Obtain an additional instance of this same database.  A new client
 * is registered at the same time if one is specified.
 */
RT_EXPORT extern struct db_i *db_clone_dbi(struct db_i *dbip,
					   long *client);


/**
 * Create a v5 database "free" object of the specified size, and place
 * it at the indicated location in the database.
 *
 * There are two interesting cases:
 * - The free object is "small".  Just write it all at once.
 * - The free object is "large".  Write header and trailer
 * separately
 *
 * @return 0 OK
 * @return -1 Fail.  This is a horrible error.
 */
RT_EXPORT extern int db5_write_free(struct db_i *dbip,
				    struct directory *dp,
				    size_t length);


/**
 * Change the size of a v5 database object.
 *
 * If the object is getting smaller, break it into two pieces, and
 * write out free objects for both.  The caller is expected to
 * re-write new data on the first one.
 *
 * If the object is getting larger, seek a suitable "hole" large
 * enough to hold it, throwing back any surplus, properly marked.
 *
 * If the object is getting larger and there is no suitable "hole" in
 * the database, extend the file, write a free object in the new
 * space, and write a free object in the old space.
 *
 * There is no point to trying to extend in place, that would require
 * two searches through the memory map, and doesn't save any disk I/O.
 *
 * Returns -
 * 0 OK
 * -1 Failure
 */
RT_EXPORT extern int db5_realloc(struct db_i *dbip,
				 struct directory *dp,
				 struct bu_external *ep);


/**
 * A routine for merging together the three optional parts of an
 * object into the final on-disk format.  Results in extra data
 * copies, but serves as a starting point for testing.  Any of name,
 * attrib, and body may be null.
 */
RT_EXPORT extern void db5_export_object3(struct bu_external *out,
					 int			dli,
					 const char			*name,
					 const unsigned char	hidden,
					 const struct bu_external	*attrib,
					 const struct bu_external	*body,
					 int			major,
					 int			minor,
					 int			a_zzz,
					 int			b_zzz);


/**
 * The attributes are taken from ip->idb_avs
 *
 * If present, convert attributes to on-disk format.  This must happen
 * after exporting the body, in case the ft_export5() method happened
 * to extend the attribute set.  Combinations are one "solid" which
 * does this.
 *
 * The internal representation is NOT freed, that's the caller's job.
 *
 * The 'ext' pointer is accepted in uninitialized form, and an
 * initialized structure is always returned, so that the caller may
 * free it even when an error return is given.
 *
 * Returns -
 * 0 OK
 * -1 FAIL
 */
RT_EXPORT extern int rt_db_cvt_to_external5(struct bu_external *ext,
					    const char *name,
					    const struct rt_db_internal *ip,
					    double conv2mm,
					    struct db_i *dbip,
					    struct resource *resp,
					    const int major);


/*
 * Modify name of external object, if necessary.
 */
RT_EXPORT extern int db_wrap_v5_external(struct bu_external *ep,
					 const char *name);


/**
 * Get an object from the database, and convert it into its internal
 * representation.
 *
 * Applications and middleware shouldn't call this directly, they
 * should use the generic interface "rt_db_get_internal()".
 *
 * Returns -
 * <0 On error
 * id On success.
 */
RT_EXPORT extern int rt_db_get_internal5(struct rt_db_internal	*ip,
					 const struct directory	*dp,
					 const struct db_i	*dbip,
					 const mat_t		mat,
					 struct resource		*resp);


/**
 * Convert the internal representation of a solid to the external one,
 * and write it into the database.
 *
 * Applications and middleware shouldn't call this directly, they
 * should use the version-generic interface "rt_db_put_internal()".
 *
 * The internal representation is always freed.  (Not the pointer,
 * just the contents).
 *
 * Returns -
 * <0 error
 * 0 success
 */
RT_EXPORT extern int rt_db_put_internal5(struct directory	*dp,
					 struct db_i		*dbip,
					 struct rt_db_internal	*ip,
					 struct resource		*resp,
					 const int		major);


/**
 * Make only the front (header) portion of a free object.  This is
 * used when operating on very large contiguous free objects in the
 * database (e.g. 50 MBytes).
 */
RT_EXPORT extern void db5_make_free_object_hdr(struct bu_external *ep,
					       size_t length);


/**
 * Make a complete, zero-filled, free object.  Note that free objects
 * can sometimes get quite large.
 */
RT_EXPORT extern void db5_make_free_object(struct bu_external *ep,
					   size_t length);


/**
 * Given a variable-width length field in network order (XDR), store
 * it in *lenp.
 *
 * This routine processes signed values.
 *
 * Returns -
 * The number of bytes of input that were decoded.
 */
RT_EXPORT extern int db5_decode_signed(size_t			*lenp,
				       const unsigned char	*cp,
				       int			format);

/**
 * Given a variable-width length field in network order (XDR), store
 * it in *lenp.
 *
 * This routine processes unsigned values.
 *
 * Returns -
 * The number of bytes of input that were decoded.
 */
RT_EXPORT extern size_t db5_decode_length(size_t *lenp,
					  const unsigned char *cp,
					  int format);

/**
 * Given a number to encode, decide which is the smallest encoding
 * format which will contain it.
 */
RT_EXPORT extern int db5_select_length_encoding(size_t len);


RT_EXPORT extern void db5_import_color_table(char *cp);

/**
 * Convert the on-disk encoding into a handy easy-to-use
 * bu_attribute_value_set structure.
 *
 * Take advantage of the readonly_min/readonly_max capability so that
 * we don't have to bu_strdup() each string, but can simply point to
 * it in the provided buffer *ap.  Important implication: don't free
 * *ap until you're done with this avs.
 *
 * The upshot of this is that bu_avs_add() and bu_avs_remove() can be
 * safely used with this *avs.
 *
 * Returns -
 * >0 count of attributes successfully imported
 * -1 Error, mal-formed input
 */
RT_EXPORT extern int db5_import_attributes(struct bu_attribute_value_set *avs,
					   const struct bu_external *ap);

/**
 * Encode the attribute-value pair information into the external
 * on-disk format.
 *
 * The on-disk encoding is:
 *
 * name1 NULL value1 NULL ... nameN NULL valueN NULL NULL
 *
 * 'ext' is initialized on behalf of the caller.
 */
RT_EXPORT extern void db5_export_attributes(struct bu_external *ap,
					    const struct bu_attribute_value_set *avs);


/**
 * Returns -
 * 0 on success
 * -1 on EOF
 * -2 on error
 */
RT_EXPORT extern int db5_get_raw_internal_fp(struct db5_raw_internal	*rip,
					     FILE			*fp);

/**
 * Verify that this is a valid header for a BRL-CAD v5 database.
 *
 * Returns -
 * 0 Not valid v5 header
 * 1 Valid v5 header
 */
RT_EXPORT extern int db5_header_is_valid(const unsigned char *hp);

RT_EXPORT extern int db5_fwrite_ident(FILE *,
				      const char *,
				      double);


/**
 * Put the old region-id-color-table into the global object.  A null
 * attribute is set if the material table is empty.
 *
 * Returns -
 * <0 error
 * 0 OK
 */
RT_EXPORT extern int db5_put_color_table(struct db_i *dbip);
RT_EXPORT extern int db5_update_ident(struct db_i *dbip,
				      const char *title,
				      double local2mm);

/**
 *
 * Given that caller already has an external representation of the
 * database object, update it to have a new name (taken from
 * dp->d_namep) in that external representation, and write the new
 * object into the database, obtaining different storage if the size
 * has changed.
 *
 * Changing the name on a v5 object is a relatively expensive
 * operation.
 *
 * Caller is responsible for freeing memory of external
 * representation, using bu_free_external().
 *
 * This routine is used to efficiently support MGED's "cp" and "keep"
 * commands, which don't need to import and decompress objects just to
 * rename and copy them.
 *
 * Returns -
 * -1 error
 * 0 success
 */
RT_EXPORT extern int db_put_external5(struct bu_external *ep,
				      struct directory *dp,
				      struct db_i *dbip);

/**
 * Update an arbitrary number of attributes on a given database
 * object.  For efficiency, this is done without looking at the object
 * body at all.
 *
 * Contents of the bu_attribute_value_set are freed, but not the
 * struct itself.
 *
 * Returns -
 * 0 on success
 * <0 on error
 */
RT_EXPORT extern int db5_update_attributes(struct directory *dp,
					   struct bu_attribute_value_set *avsp,
					   struct db_i *dbip);

/**
 * A convenience routine to update the value of a single attribute.
 *
 * Returns -
 * 0 on success
 * <0 on error
 */
RT_EXPORT extern int db5_update_attribute(const char *obj_name,
					  const char *aname,
					  const char *value,
					  struct db_i *dbip);

/**
 * Replace the attributes of a given database object.
 *
 * For efficiency, this is done without looking at the object body at
 * all.  Contents of the bu_attribute_value_set are freed, but not the
 * struct itself.
 *
 * Returns -
 * 0 on success
 * <0 on error
 */
RT_EXPORT extern int db5_replace_attributes(struct directory *dp,
					    struct bu_attribute_value_set *avsp,
					    struct db_i *dbip);

/**
 *
 * Get attributes for an object pointed to by *dp
 *
 * returns:
 * 0 - all is well
 * <0 - error
 */
RT_EXPORT extern int db5_get_attributes(const struct db_i *dbip,
					struct bu_attribute_value_set *avs,
					const struct directory *dp);

/* db_comb.c */

/**
 * Return count of number of leaf nodes in this tree.
 */
RT_EXPORT extern size_t db_tree_nleaves(const union tree *tp);

/**
 * Take a binary tree in "V4-ready" layout (non-unions pushed below
 * unions, left-heavy), and flatten it into an array layout, ready for
 * conversion back to the GIFT-inspired V4 database format.
 *
 * This is done using the db_non_union_push() routine.
 *
 * If argument 'free' is non-zero, then the non-leaf nodes are freed
 * along the way, to prevent memory leaks.  In this case, the caller's
 * copy of 'tp' will be invalid upon return.
 *
 * When invoked at the very top of the tree, the op argument must be
 * OP_UNION.
 */
RT_EXPORT extern struct rt_tree_array *db_flatten_tree(struct rt_tree_array *rt_tree_array, union tree *tp, int op, int avail, struct resource *resp);

/**
 * Import a combination record from a V4 database into internal form.
 */
RT_EXPORT extern int rt_comb_import4(struct rt_db_internal	*ip,
				     const struct bu_external	*ep,
				     const mat_t		matrix,		/* NULL if identity */
				     const struct db_i		*dbip,
				     struct resource		*resp);

RT_EXPORT extern int rt_comb_export4(struct bu_external			*ep,
				     const struct rt_db_internal	*ip,
				     double				local2mm,
				     const struct db_i			*dbip,
				     struct resource			*resp);

/**
 * Produce a GIFT-compatible listing, one "member" per line,
 * regardless of the structure of the tree we've been given.
 */
RT_EXPORT extern void db_tree_flatten_describe(struct bu_vls	*vls,
					       const union tree	*tp,
					       int		indented,
					       int		lvl,
					       double		mm2local,
					       struct resource	*resp);

RT_EXPORT extern void db_tree_describe(struct bu_vls	*vls,
				       const union tree	*tp,
				       int		indented,
				       int		lvl,
				       double		mm2local);

RT_EXPORT extern void db_comb_describe(struct bu_vls	*str,
				       const struct rt_comb_internal	*comb,
				       int		verbose,
				       double		mm2local,
				       struct resource	*resp);

/**
 * OBJ[ID_COMBINATION].ft_describe() method
 */
RT_EXPORT extern int rt_comb_describe(struct bu_vls	*str,
				      const struct rt_db_internal *ip,
				      int		verbose,
				      double		mm2local,
				      struct resource *resp,
				      struct db_i *db_i);

/*==================== END g_comb.c / table.c interface ========== */

/**
 * As the v4 database does not really have the notion of "wrapping",
 * this function writes the object name into the proper place (a
 * standard location in all granules).
 */
RT_EXPORT extern void db_wrap_v4_external(struct bu_external *op,
					  const char *name);

/* Some export support routines */

/**
 * Support routine for db_ck_v4gift_tree().
 * Ensure that the tree below 'tp' is left-heavy, i.e. that there are
 * nothing but solids on the right side of any binary operations.
 *
 * Returns -
 * -1 ERROR
 * 0 OK
 */
RT_EXPORT extern int db_ck_left_heavy_tree(const union tree	*tp,
					   int		no_unions);

/**
 * Look a gift-tree in the mouth.
 *
 * Ensure that this boolean tree conforms to the GIFT convention that
 * union operations must bind the loosest.
 *
 * There are two stages to this check:
 * 1) Ensure that if unions are present they are all at the root of tree,
 * 2) Ensure non-union children of union nodes are all left-heavy
 * (nothing but solid nodes permitted on rhs of binary operators).
 *
 * Returns -
 * -1 ERROR
 * 0 OK
 */
RT_EXPORT extern int db_ck_v4gift_tree(const union tree *tp);

/**
 * Given a rt_tree_array array, build a tree of "union tree" nodes
 * appropriately connected together.  Every element of the
 * rt_tree_array array used is replaced with a TREE_NULL.  Elements
 * which are already TREE_NULL are ignored.  Returns a pointer to the
 * top of the tree.
 */
RT_EXPORT extern union tree *db_mkbool_tree(struct rt_tree_array *rt_tree_array,
					    size_t		howfar,
					    struct resource	*resp);

RT_EXPORT extern union tree *db_mkgift_tree(struct rt_tree_array *trees,
					    size_t subtreecount,
					    struct resource *resp);

/**
 * fills in rgb with the color for a given comb combination
 *
 * returns truthfully if a color could be got
 */
RT_EXPORT extern int rt_comb_get_color(unsigned char rgb[3], const struct rt_comb_internal *comb);

/* tgc.c */
RT_EXPORT extern void rt_pt_sort(fastf_t t[],
				 int npts);

/* ell.c */
RT_EXPORT extern void rt_ell_16pts(fastf_t *ov,
				   fastf_t *V,
				   fastf_t *A,
				   fastf_t *B);


/**
 * change all matching object names in the comb tree from old_name to
 * new_name
 *
 * calling function must supply an initialized bu_ptbl, and free it
 * once done.
 */
RT_EXPORT extern int db_comb_mvall(struct directory *dp,
				   struct db_i *dbip,
				   const char *old_name,
				   const char *new_name,
				   struct bu_ptbl *stack);

/* roots.c */
/** @addtogroup librt */
/** @{ */
/** @file librt/roots.c
 *
 * Find the roots of a polynomial
 *
 */

/**
 * WARNING: The polynomial given as input is destroyed by this
 * routine.  The caller must save it if it is important!
 *
 * NOTE : This routine is written for polynomials with real
 * coefficients ONLY.  To use with complex coefficients, the Complex
 * Math library should be used throughout.  Some changes in the
 * algorithm will also be required.
 */
RT_EXPORT extern int rt_poly_roots(bn_poly_t *eqn,
				   bn_complex_t roots[],
				   const char *name);

/** @} */
/* db_io.c */
RT_EXPORT extern int db_write(struct db_i	*dbip,
			      const void *	addr,
			      size_t		count,
			      off_t		offset);

/**
 * Add name from dp->d_namep to external representation of solid, and
 * write it into a file.
 *
 * Caller is responsible for freeing memory of external
 * representation, using bu_free_external().
 *
 * The 'name' field of the external representation is modified to
 * contain the desired name.  The 'ep' parameter cannot be const.
 *
 * THIS ROUTINE ONLY SUPPORTS WRITING V4 GEOMETRY.
 *
 * Returns -
 * <0 error
 * 0 OK
 *
 * NOTE: Callers of this should be using wdb_export_external()
 * instead.
 */
RT_EXPORT extern int db_fwrite_external(FILE			*fp,
					const char		*name,
					struct bu_external	*ep);

/* malloc & read records */

/**
 * Retrieve all records in the database pertaining to an object, and
 * place them in malloc()'ed storage, which the caller is responsible
 * for free()'ing.
 *
 * This loads the combination into a local record buffer.  This is in
 * external v4 format.
 *
 * Returns -
 * union record * - OK
 * (union record *)0 - FAILURE
 */
RT_EXPORT extern union record *db_getmrec(const struct db_i *,
					  const struct directory *dp);
/* get several records from db */

/**
 * Retrieve 'len' records from the database, "offset" granules into
 * this entry.
 *
 * Returns -
 * 0 OK
 * -1 FAILURE
 */
RT_EXPORT extern int db_get(const struct db_i *,
			    const struct directory *dp,
			    union record *where,
			    off_t offset,
			    size_t len);
/* put several records into db */

/**
 * Store 'len' records to the database, "offset" granules into this
 * entry.
 *
 * Returns:
 * 0 OK
 * non-0 FAILURE
 */
RT_EXPORT extern int db_put(struct db_i *,
			    const struct directory *dp,
			    union record *where,
			    off_t offset, size_t len);

/**
 * Obtains a object from the database, leaving it in external
 * (on-disk) format.
 *
 * The bu_external structure represented by 'ep' is initialized here,
 * the caller need not pre-initialize it.  On error, 'ep' is left
 * un-initialized and need not be freed, to simplify error recovery.
 * On success, the caller is responsible for calling
 * bu_free_external(ep);
 *
 * Returns -
 * -1 error
 * 0 success
 */
RT_EXPORT extern int db_get_external(struct bu_external *ep,
				     const struct directory *dp,
				     const struct db_i *dbip);

/**
 * Given that caller already has an external representation of the
 * database object, update it to have a new name (taken from
 * dp->d_namep) in that external representation, and write the new
 * object into the database, obtaining different storage if the size
 * has changed.
 *
 * Caller is responsible for freeing memory of external
 * representation, using bu_free_external().
 *
 * This routine is used to efficiently support MGED's "cp" and "keep"
 * commands, which don't need to import objects just to rename and
 * copy them.
 *
 * Returns -
 * <0 error
 * 0 success
 */
RT_EXPORT extern int db_put_external(struct bu_external *ep,
				     struct directory *dp,
				     struct db_i *dbip);

/* db_scan.c */
/* read db (to build directory) */
RT_EXPORT extern int db_scan(struct db_i *,
			     int (*handler)(struct db_i *,
					    const char *name,
					    off_t addr,
					    size_t nrec,
					    int flags,
					    void *client_data),
			     int do_old_matter,
			     void *client_data);
/* update db unit conversions */
#define db_ident(a, b, c)		+++error+++

/**
 * Update the _GLOBAL object, which in v5 serves the place of the
 * "ident" header record in v4 as the place to stash global
 * information.  Since every database will have one of these things,
 * it's no problem to update it.
 *
 * Returns -
 * 0 Success
 * -1 Fatal Error
 */
RT_EXPORT extern int db_update_ident(struct db_i *dbip,
				     const char *title,
				     double local2mm);

/**
 * Create a header for a v5 database.
 *
 * This routine has the same calling sequence as db_fwrite_ident()
 * which makes a v4 database header.
 *
 * In the v5 database, two database objects must be created to match
 * the semantics of what was in the v4 header:
 *
 * First, a database header object.
 *
 * Second, create a specially named attribute-only object which
 * contains the attributes "title=" and "units=" with the values of
 * title and local2mm respectively.
 *
 * Note that the current working units are specified as a conversion
 * factor to millimeters because database dimensional values are
 * always stored as millimeters (mm).  The units conversion factor
 * only affects the display and conversion of input values.  This
 * helps prevent error accumulation and improves numerical stability
 * when calculations are made.
 *
 * This routine should only be used by db_create().  Everyone else
 * should use db5_update_ident().
 *
 * Returns -
 * 0 Success
 * -1 Fatal Error
 */
RT_EXPORT extern int db_fwrite_ident(FILE *fp,
				     const char *title,
				     double local2mm);

/**
 * Initialize conversion factors given the v4 database unit
 */
RT_EXPORT extern void db_conversions(struct db_i *,
				     int units);

/**
 * Given a string, return the V4 database code representing the user's
 * preferred editing units.  The v4 database format does not have many
 * choices.
 *
 * Returns -
 * -1 Not a legal V4 database code
 * # The V4 database code number
 */
RT_EXPORT extern int db_v4_get_units_code(const char *str);

/* db5_scan.c */

/**
 * A generic routine to determine the type of the database, (v4 or v5)
 * and to invoke the appropriate db_scan()-like routine to build the
 * in-memory directory.
 *
 * It is the caller's responsibility to close the database in case of
 * error.
 *
 * Called from rt_dirbuild() and other places directly where a
 * raytrace instance is not required.
 *
 * Returns -
 * 0 OK
 * -1 failure
 */
RT_EXPORT extern int db_dirbuild(struct db_i *dbip);
RT_EXPORT extern struct directory *db5_diradd(struct db_i *dbip,
					      const struct db5_raw_internal *rip,
					      off_t laddr,
					      void *client_data);

/**
 * Scan a v5 database, sending each object off to a handler.
 *
 * Returns -
 * 0 Success
 * -1 Fatal Error
 */
RT_EXPORT extern int db5_scan(struct db_i *dbip,
			      void (*handler)(struct db_i *,
					      const struct db5_raw_internal *,
					      off_t addr,
					      void *client_data),
			      void *client_data);

/**
 * obtain the database version for a given database instance.
 *
 * presently returns only a 4 or 5 accordingly.
 */
RT_EXPORT extern int db_version(struct db_i *dbip);


/* db_corrupt.c */

/**
 * Detect whether a given geometry database file seems to be corrupt
 * or invalid due to flipped endianness.  Only relevant for v4
 * geometry files that are binary-incompatible with the runtime
 * platform.
 *
 * Returns true if flipping the endian type fixes all combination
 * member matrices.
 */
RT_EXPORT extern int rt_db_flip_endian(struct db_i *dbip);


/* db5_comb.c */

/**
 * Read a combination object in v5 external (on-disk) format, and
 * convert it into the internal format described in rtgeom.h
 *
 * This is an unusual conversion, because some of the data is taken
 * from attributes, not just from the object body.  By the time this
 * is called, the attributes will already have been cracked into
 * ip->idb_avs, we get the attributes from there.
 *
 * Returns -
 * 0 OK
 * -1 FAIL
 */
RT_EXPORT extern int rt_comb_import5(struct rt_db_internal *ip, const struct bu_external *ep, const mat_t mat, const struct db_i *dbip, struct resource *resp);

/* extrude.c */
RT_EXPORT extern int rt_extrude_import5(struct rt_db_internal *ip, const struct bu_external *ep, const mat_t mat, const struct db_i *dbip, struct resource *resp);


/**
 * "open" an in-memory-only database instance.  this initializes a
 * dbip for use, creating an inmem dbi_wdbp as the means to add
 * geometry to the directory (use wdb_export_external()).
 */
RT_EXPORT extern struct db_i * db_open_inmem(void);

/**
 * creates an in-memory-only database.  this is very similar to
 * db_open_inmem() with the exception that the this routine adds a
 * default _GLOBAL object.
 */
RT_EXPORT extern struct db_i * db_create_inmem(void);


/**
 * Transmogrify an existing directory entry to be an in-memory-only
 * one, stealing the external representation from 'ext'.
 */
RT_EXPORT extern void db_inmem(struct directory	*dp,
			       struct bu_external	*ext,
			       int		flags,
			       struct db_i	*dbip);

/* db_lookup.c */

/**
 * Return the number of "struct directory" nodes in the given
 * database.
 */
RT_EXPORT extern size_t db_directory_size(const struct db_i *dbip);

/**
 * For debugging, ensure that all the linked-lists for the directory
 * structure are intact.
 */
RT_EXPORT extern void db_ck_directory(const struct db_i *dbip);

/**
 * Returns -
 * 0 if the in-memory directory is empty
 * 1 if the in-memory directory has entries,
 * which implies that a db_scan() has already been performed.
 */
RT_EXPORT extern int db_is_directory_non_empty(const struct db_i	*dbip);

/**
 * Returns a hash index for a given string that corresponds with the
 * head of that string's hash chain.
 */
RT_EXPORT extern int db_dirhash(const char *str);

/**
 * This routine ensures that ret_name is not already in the
 * directory. If it is, it tries a fixed number of times to modify
 * ret_name before giving up. Note - most of the time, the hash for
 * ret_name is computed once.
 *
 * Inputs -
 * dbip database instance pointer
 * ret_name the original name
 * noisy to blather or not
 *
 * Outputs -
 * ret_name the name to use
 * headp pointer to the first (struct directory *) in the bucket
 *
 * Returns -
 * 0 success
 * <0 fail
 */
RT_EXPORT extern int db_dircheck(struct db_i *dbip,
				 struct bu_vls *ret_name,
				 int noisy,
				 struct directory ***headp);
/* convert name to directory ptr */

/**
 * This routine takes a name and looks it up in the directory table.
 * If the name is present, a pointer to the directory struct element
 * is returned, otherwise NULL is returned.
 *
 * If noisy is non-zero, a print occurs, else only the return code
 * indicates failure.
 *
 * Returns -
 * struct directory if name is found
 * RT_DIR_NULL on failure
 */
RT_EXPORT extern struct directory *db_lookup(const struct db_i *,
					     const char *name,
					     int noisy);
/* lookup directory entries based on attributes */

/**
 * lookup directory entries based on directory flags (dp->d_flags) and
 * attributes the "dir_flags" arg is a mask for the directory flags
 * the *"avs" is an attribute value set used to select from the
 * objects that *pass the flags mask. if "op" is 1, then the object
 * must have all the *attributes and values that appear in "avs" in
 * order to be *selected. If "op" is 2, then the object must have at
 * least one of *the attribute/value pairs from "avs".
 *
 * dir_flags are in the form used in struct directory (d_flags)
 *
 * for op:
 * 1 -> all attribute name/value pairs must be present and match
 * 2 -> at least one of the name/value pairs must be present and match
 *
 * returns a ptbl list of selected directory pointers an empty list
 * means nothing met the requirements a NULL return means something
 * went wrong.
 */
RT_EXPORT extern struct bu_ptbl *db_lookup_by_attr(struct db_i *dbip,
						   int dir_flags,
						   struct bu_attribute_value_set *avs,
						   int op);

/* add entry to directory */

/**
 * Add an entry to the directory.  Try to make the regular path
 * through the code as fast as possible, to speed up building the
 * table of contents.
 *
 * dbip is a pointer to a valid/opened database instance
 *
 * name is the string name of the object being added
 *
 * laddr is the offset into the file to the object
 *
 * len is the length of the object, number of db granules used
 *
 * flags are defined in raytrace.h (RT_DIR_SOLID, RT_DIR_COMB, RT_DIR_REGION,
 * RT_DIR_INMEM, etc.) for db version 5, ptr is the minor_type
 * (non-null pointer to valid unsigned char code)
 *
 * an laddr of RT_DIR_PHONY_ADDR means that database storage has not
 * been allocated yet.
 */
RT_EXPORT extern struct directory *db_diradd(struct db_i *,
					     const char *name,
					     off_t laddr,
					     size_t len,
					     int flags,
					     void *ptr);
RT_EXPORT extern struct directory *db_diradd5(struct db_i *dbip,
					      const char *name,
					      off_t				laddr,
					      unsigned char			major_type,
					      unsigned char 			minor_type,
					      unsigned char			name_hidden,
					      size_t				object_length,
					      struct bu_attribute_value_set	*avs);

/* delete entry from directory */

/**
 * Given a pointer to a directory entry, remove it from the linked
 * list, and free the associated memory.
 *
 * It is the responsibility of the caller to have released whatever
 * structures have been hung on the d_use_hd bu_list, first.
 *
 * Returns -
 * 0 on success
 * non-0 on failure
 */
RT_EXPORT extern int db_dirdelete(struct db_i *,
				  struct directory *dp);
RT_EXPORT extern int db_fwrite_ident(FILE *,
				     const char *,
				     double);

/**
 * For debugging, print the entire contents of the database directory.
 */
RT_EXPORT extern void db_pr_dir(const struct db_i *dbip);

/**
 * Change the name string of a directory entry.  Because of the
 * hashing function, this takes some extra work.
 *
 * Returns -
 * 0 on success
 * non-0 on failure
 */
RT_EXPORT extern int db_rename(struct db_i *,
			       struct directory *,
			       const char *newname);


/**
 * Updates the d_nref fields (which count the number of times a given
 * entry is referenced by a COMBination in the database).
 *
 */
RT_EXPORT extern void db_update_nref(struct db_i *dbip,
				     struct resource *resp);


/**
 * DEPRECATED: Use db_ls() instead of this function.
 *
 * Appends a list of all database matches to the given vls, or the
 * pattern itself if no matches are found.  Returns the number of
 * matches.
 */
DEPRECATED RT_EXPORT extern int db_regexp_match_all(struct bu_vls *dest,
					 struct db_i *dbip,
					 const char *pattern);

/* db_ls.c */
/**
 * db_ls takes a database instance pointer and assembles a directory
 * pointer array of objects in the database according to a set of
 * flags.  An optional pattern can be supplied for match filtering
 * via globbing rules (see bu_fnmatch).  If pattern is NULL, filtering
 * is performed using only the flags.
 *
 * The caller is responsible for freeing the array.
 *
 * Returns -
 * integer count of objects in dpv
 * struct directory ** array of objects in dpv via argument
 *
 */
#define DB_LS_PRIM         0x1    /* filter for primitives (solids)*/
#define DB_LS_COMB         0x2    /* filter for combinations */
#define DB_LS_REGION       0x4    /* filter for regions */
#define DB_LS_HIDDEN       0x8    /* include hidden objects in results */
#define DB_LS_NON_GEOM     0x10   /* filter for non-geometry objects */
#define DB_LS_TOPS         0x20   /* filter for objects un-referenced by other objects */
/* TODO - implement this flag
#define DB_LS_REGEX	   0x40*/ /* interpret pattern using regex rules, instead of
				     globbing rules (default) */
RT_EXPORT extern size_t db_ls(const struct db_i *dbip,
			      int flags,
			      const char *pattern,
			      struct directory ***dpv);

/**
 * convert an argv list of names to a directory pointer array.
 *
 * If db_lookup fails for any individual argv, an empty directory
 * structure is created and assigned the name and RT_DIR_PHONY_ADDR
 *
 * The returned directory ** structure is NULL terminated.
 */
RT_EXPORT extern struct directory **db_argv_to_dpv(const struct db_i *dbip,
						   const char **argv);


/**
 * convert a directory pointer array to an argv char pointer array.
 */
RT_EXPORT extern char **db_dpv_to_argv(struct directory **dpv);


/* db_flags.c */
/**
 * Given the internal form of a database object, return the
 * appropriate 'flags' word for stashing in the in-memory directory of
 * objects.
 */
RT_EXPORT extern int db_flags_internal(const struct rt_db_internal *intern);


/* XXX - should use in db5_diradd() */
/**
 * Given a database object in "raw" internal form, return the
 * appropriate 'flags' word for stashing in the in-memory directory of
 * objects.
 */
RT_EXPORT extern int db_flags_raw_internal(const struct db5_raw_internal *intern);

/* db_alloc.c */

/* allocate "count" granules */
RT_EXPORT extern int db_alloc(struct db_i *,
			      struct directory *dp,
			      size_t count);
/* delete "recnum" from entry */
RT_EXPORT extern int db_delrec(struct db_i *,
			       struct directory *dp,
			       int recnum);
/* delete all granules assigned dp */
RT_EXPORT extern int db_delete(struct db_i *,
			       struct directory *dp);
/* write FREE records from 'start' */
RT_EXPORT extern int db_zapper(struct db_i *,
			       struct directory *dp,
			       size_t start);

/**
 * This routine is called by the RT_GET_DIRECTORY macro when the
 * freelist is exhausted.  Rather than simply getting one additional
 * structure, we get a whole batch, saving overhead.
 */
RT_EXPORT extern void db_alloc_directory_block(struct resource *resp);

/**
 * This routine is called by the GET_SEG macro when the freelist is
 * exhausted.  Rather than simply getting one additional structure, we
 * get a whole batch, saving overhead.  When this routine is called,
 * the seg resource must already be locked.  malloc() locking is done
 * in bu_malloc.
 */
RT_EXPORT extern void rt_alloc_seg_block(struct resource *res);

/* db_tree.c */

/**
 * Duplicate the contents of a db_tree_state structure, including a
 * private copy of the ts_mater field(s) and the attribute/value set.
 */
RT_EXPORT extern void db_dup_db_tree_state(struct db_tree_state *otsp,
					   const struct db_tree_state *itsp);

/**
 * Release dynamic fields inside the structure, but not the structure
 * itself.
 */
RT_EXPORT extern void db_free_db_tree_state(struct db_tree_state *tsp);

/**
 * In most cases, you don't want to use this routine, you want to
 * struct copy mged_initial_tree_state or rt_initial_tree_state, and
 * then set ts_dbip in your copy.
 */
RT_EXPORT extern void db_init_db_tree_state(struct db_tree_state *tsp,
					    struct db_i *dbip,
					    struct resource *resp);
RT_EXPORT extern struct combined_tree_state *db_new_combined_tree_state(const struct db_tree_state *tsp,
									const struct db_full_path *pathp);
RT_EXPORT extern struct combined_tree_state *db_dup_combined_tree_state(const struct combined_tree_state *old);
RT_EXPORT extern void db_free_combined_tree_state(struct combined_tree_state *ctsp);
RT_EXPORT extern void db_pr_tree_state(const struct db_tree_state *tsp);
RT_EXPORT extern void db_pr_combined_tree_state(const struct combined_tree_state *ctsp);

/**
 * Handle inheritance of material property found in combination
 * record.  Color and the material property have separate inheritance
 * interlocks.
 *
 * Returns -
 * -1 failure
 * 0 success
 * 1 success, this is the top of a new region.
 */
RT_EXPORT extern int db_apply_state_from_comb(struct db_tree_state *tsp,
					      const struct db_full_path *pathp,
					      const struct rt_comb_internal *comb);

/**
 * Updates state via *tsp, pushes member's directory entry on *pathp.
 * (Caller is responsible for popping it).
 *
 * Returns -
 * -1 failure
 * 0 success, member pushed on path
 */
RT_EXPORT extern int db_apply_state_from_memb(struct db_tree_state *tsp,
					      struct db_full_path *pathp,
					      const union tree *tp);

/**
 * Returns -
 * -1 found member, failed to apply state
 * 0 unable to find member 'cp'
 * 1 state applied OK
 */
RT_EXPORT extern int db_apply_state_from_one_member(struct db_tree_state *tsp,
						    struct db_full_path *pathp,
						    const char *cp,
						    int sofar,
						    const union tree *tp);

/**
 * The search stops on the first match.
 *
 * Returns -
 * tp if found
 * TREE_NULL if not found in this tree
 */
RT_EXPORT extern union tree *db_find_named_leaf(union tree *tp, const char *cp);

/**
 * The search stops on the first match.
 *
 * Returns -
 * TREE_NULL if not found in this tree
 * tp if found
 * *side == 1 if leaf is on lhs.
 * *side == 2 if leaf is on rhs.
 *
 */
RT_EXPORT extern union tree *db_find_named_leafs_parent(int *side,
							union tree *tp,
							const char *cp);
RT_EXPORT extern void db_tree_del_lhs(union tree *tp,
				      struct resource *resp);
RT_EXPORT extern void db_tree_del_rhs(union tree *tp,
				      struct resource *resp);

/**
 * Given a name presumably referenced in a OP_DB_LEAF node, delete
 * that node, and the operation node that references it.  Not that
 * this may not produce an equivalent tree, for example when rewriting
 * (A - subtree) as (subtree), but that will be up to the caller/user
 * to adjust.  This routine gets rid of exactly two nodes in the tree:
 * leaf, and op.  Use some other routine if you wish to kill the
 * entire rhs below "-" and "intersect" nodes.
 *
 * The two nodes deleted will have their memory freed.
 *
 * If the tree is a single OP_DB_LEAF node, the leaf is freed and *tp
 * is set to NULL.
 *
 * Returns -
 * -3 Internal error
 * -2 Tree is empty
 * -1 Unable to find OP_DB_LEAF node specified by 'cp'.
 * 0 OK
 */
RT_EXPORT extern int db_tree_del_dbleaf(union tree **tp,
					const char *cp,
					struct resource *resp,
					int nflag);

/**
 * Multiply on the left every matrix found in a DB_LEAF node in a
 * tree.
 */
RT_EXPORT extern void db_tree_mul_dbleaf(union tree *tp,
					 const mat_t mat);

/**
 * This routine traverses a combination (union tree) in LNR order and
 * calls the provided function for each OP_DB_LEAF node.  Note that
 * this routine does not go outside this one combination!!!!
 *
 * was previously named comb_functree()
 */
RT_EXPORT extern void db_tree_funcleaf(struct db_i		*dbip,
				       struct rt_comb_internal	*comb,
				       union tree		*comb_tree,
                                       void (*leaf_func)(struct db_i *, struct rt_comb_internal *, union tree *,
                                                         void *, void *, void *, void *),
				       void *		user_ptr1,
				       void *		user_ptr2,
				       void *		user_ptr3,
				       void *		user_ptr4);

/**
 * Starting with possible prior partial path and corresponding
 * accumulated state, follow the path given by "new_path", updating
 * *tsp and *total_path with full state information along the way.  In
 * a better world, there would have been a "combined_tree_state" arg.
 *
 * Parameter 'depth' controls how much of 'new_path' is used:
 *
 * 0 use all of new_path
 * >0 use only this many of the first elements of the path
 * <0 use all but this many path elements.
 *
 * A much more complete version of rt_plookup() and pathHmat().  There
 * is also a TCL interface.
 *
 * Returns -
 * 0 success (plus *tsp is updated)
 * -1 error (*tsp values are not useful)
 */
RT_EXPORT extern int db_follow_path(struct db_tree_state *tsp,
				    struct db_full_path *total_path,
				    const struct db_full_path *new_path,
				    int noisy,
				    long pdepth);

/**
 * Follow the slash-separated path given by "cp", and update *tsp and
 * *total_path with full state information along the way.
 *
 * A much more complete version of rt_plookup().
 *
 * Returns -
 * 0 success (plus *tsp is updated)
 * -1 error (*tsp values are not useful)
 */
RT_EXPORT extern int db_follow_path_for_state(struct db_tree_state *tsp,
					      struct db_full_path *pathp,
					      const char *orig_str, int noisy);

/**
 * Recurse down the tree, finding all the leaves (or finding just all
 * the regions).
 *
 * ts_region_start_func() is called to permit regions to be skipped.
 * It is not intended to be used for collecting state.
 */
RT_EXPORT extern union tree *db_recurse(struct db_tree_state	*tsp,
					struct db_full_path *pathp,
					struct combined_tree_state **region_start_statepp,
					void *client_data);
RT_EXPORT extern union tree *db_dup_subtree(const union tree *tp,
					    struct resource *resp);
RT_EXPORT extern void db_ck_tree(const union tree *tp);


/**
 * Release all storage associated with node 'tp', including children
 * nodes.
 */
RT_EXPORT extern void db_free_tree(union tree *tp,
				   struct resource *resp);


/**
 * Re-balance this node to make it left heavy.  Union operators will
 * be moved to left side.  when finished "tp" MUST still point to top
 * node of this subtree.
 */
RT_EXPORT extern void db_left_hvy_node(union tree *tp);


/**
 * If there are non-union operations in the tree, above the region
 * nodes, then rewrite the tree so that the entire tree top is nothing
 * but union operations, and any non-union operations are clustered
 * down near the region nodes.
 */
RT_EXPORT extern void db_non_union_push(union tree *tp,
					struct resource *resp);


/**
 * Return a count of the number of "union tree" nodes below "tp",
 * including tp.
 */
RT_EXPORT extern int db_count_tree_nodes(const union tree *tp,
					 int count);


/**
 * Returns -
 * 1 if this tree contains nothing but union operations.
 * 0 if at least one subtraction or intersection op exists.
 */
RT_EXPORT extern int db_is_tree_all_unions(const union tree *tp);
RT_EXPORT extern int db_count_subtree_regions(const union tree *tp);
RT_EXPORT extern int db_tally_subtree_regions(union tree	*tp,
					      union tree	**reg_trees,
					      int		cur,
					      int		lim,
					      struct resource *resp);

/**
 * This is the top interface to the "tree walker."
 *
 * Parameters:
 *	rtip		rt_i structure to database (open with rt_dirbuild())
 *	argc		# of tree-tops named
 *	argv		names of tree-tops to process
 *	init_state	Input parameter: initial state of the tree.
 *			For example:  rt_initial_tree_state,
 *			and mged_initial_tree_state.
 *
 * These parameters are pointers to callback routines.  If NULL, they
 * won't be called.
 *
 *	reg_start_func	Called at beginning of each region, before
 *			visiting any nodes within the region.  Return
 *			0 if region should be skipped without
 *			recursing, otherwise non-zero.  DO NOT USE FOR
 *			OTHER PURPOSES!  For example, can be used to
 *			quickly skip air regions.
 *
 *	reg_end_func    Called after all nodes within a region have been
 *			recursively processed by leaf_func.  If it
 *			wants to retain 'curtree' then it may steal
 *			that pointer and return TREE_NULL.  If it
 *			wants us to clean up some or all of that tree,
 *			then it returns a non-null (union tree *)
 *			pointer, and that tree is safely freed in a
 *			non-parallel section before we return.
 *
 *	leaf_func	Function to process a leaf node.  It is actually
 *			invoked from db_recurse() from
 *			_db_walk_subtree().  Returns (union tree *)
 *			representing the leaf, or TREE_NULL if leaf
 *			does not exist or has an error.
 *
 *
 * This routine will employ multiple CPUs if asked, but is not
 * multiply-parallel-recursive.  Call this routine with ncpu > 1 from
 * serial code only.  When called from within an existing thread, ncpu
 * must be 1.
 *
 * If ncpu > 1, the caller is responsible for making sure that
 * RTG.rtg_parallel is non-zero.
 *
 * Plucks per-cpu resources out of rtip->rti_resources[].  They need
 * to have been initialized first.
 *
 * Returns -
 * -1 Failure to prepare even a single sub-tree
 * 0 OK
 */
RT_EXPORT extern int db_walk_tree(struct db_i *dbip,
				  int argc,
				  const char **argv,
				  int ncpu,
				  const struct db_tree_state *init_state,
				  int (*reg_start_func) (struct db_tree_state * /*tsp*/,
							 const struct db_full_path * /*pathp*/,
							 const struct rt_comb_internal * /* combp */,
							 void *client_data),
				  union tree *(*reg_end_func) (struct db_tree_state * /*tsp*/,
							       const struct db_full_path * /*pathp*/,
							       union tree * /*curtree*/,
							       void *client_data),
				  union tree *(*leaf_func) (struct db_tree_state * /*tsp*/,
							    const struct db_full_path * /*pathp*/,
							    struct rt_db_internal * /*ip*/,
							    void *client_data),
				  void *client_data);

/**
 * 'arc' may be a null pointer, signifying an identity matrix.
 * 'materp' may be a null pointer, signifying that the region has
 * already been finalized above this point in the tree.
 */
RT_EXPORT extern void db_apply_anims(struct db_full_path *pathp,
				     struct directory *dp,
				     mat_t stck,
				     mat_t arc,
				     struct mater_info *materp);

/**
 * Given the name of a region, return the matrix which maps model
 * coordinates into "region" coordinates.
 *
 * Returns:
 * 0 OK
 * <0 Failure
 */
RT_EXPORT extern int db_region_mat(mat_t		m,		/* result */
				   struct db_i	*dbip,
				   const char	*name,
				   struct resource *resp);


/**
 * XXX given that this routine depends on rtip, it should be called
 * XXX rt_shader_mat().
 *
 * Given a region, return a matrix which maps model coordinates into
 * region "shader space".  This is a space where points in the model
 * within the bounding box of the region are mapped into "region"
 * space (the coordinate system in which the region is defined).  The
 * area occupied by the region's bounding box (in region coordinates)
 * are then mapped into the unit cube.  This unit cube defines "shader
 * space".
 *
 * Returns:
 * 0 OK
 * <0 Failure
 */
RT_EXPORT extern int rt_shader_mat(mat_t			model_to_shader,	/* result */
				   const struct rt_i	*rtip,
				   const struct region	*rp,
				   point_t			p_min,	/* input/output: shader/region min point */
				   point_t			p_max,	/* input/output: shader/region max point */
				   struct resource		*resp);

/**
 * Fills a bu_vls with a representation of the given tree appropriate
 * for processing by Tcl scripts.
 *
 * A tree 't' is represented in the following manner:
 *
 * t := { l dbobjname { mat } }
 *	   | { l dbobjname }
 *	   | { u t1 t2 }
 * 	   | { n t1 t2 }
 *	   | { - t1 t2 }
 *	   | { ^ t1 t2 }
 *         | { ! t1 }
 *	   | { G t1 }
 *	   | { X t1 }
 *	   | { N }
 *	   | {}
 *
 * where 'dbobjname' is a string containing the name of a database object,
 *       'mat'       is the matrix preceding a leaf,
 *       't1', 't2'  are trees (recursively defined).
 *
 * Notice that in most cases, this tree will be grossly unbalanced.
 */
RT_EXPORT extern int db_tree_list(struct bu_vls *vls, const union tree *tp);

/**
 * Take a TCL-style string description of a binary tree, as produced
 * by db_tree_list(), and reconstruct the in-memory form of that tree.
 */
RT_EXPORT extern union tree *db_tree_parse(struct bu_vls *vls, const char *str, struct resource *resp);


/* dir.c */

/**
 * Read named MGED db, build toc.
 */
RT_EXPORT extern struct rt_i *rt_dirbuild(const char *filename, char *buf, int len);

/**
 * Get an object from the database, and convert it into its internal
 * representation.
 *
 * Returns -
 * <0 On error
 * id On success.
 */
RT_EXPORT extern int rt_db_get_internal(struct rt_db_internal	*ip,
					const struct directory	*dp,
					const struct db_i	*dbip,
					const mat_t		mat,
					struct resource		*resp);

/**
 * Convert the internal representation of a solid to the external one,
 * and write it into the database.  On success only, the internal
 * representation is freed.
 *
 * Returns -
 * <0 error
 * 0 success
 */
RT_EXPORT extern int rt_db_put_internal(struct directory	*dp,
					struct db_i		*dbip,
					struct rt_db_internal	*ip,
					struct resource		*resp);

/**
 * Put an object in internal format out onto a file in external
 * format.  Used by LIBWDB.
 *
 * Can't really require a dbip parameter, as many callers won't have
 * one.
 *
 * THIS ROUTINE ONLY SUPPORTS WRITING V4 GEOMETRY.
 *
 * Returns -
 * 0 OK
 * <0 error
 */
RT_EXPORT extern int rt_fwrite_internal(FILE *fp,
					const char *name,
					const struct rt_db_internal *ip,
					double conv2mm);
RT_EXPORT extern void rt_db_free_internal(struct rt_db_internal *ip);


/**
 * Convert an object name to a rt_db_internal pointer
 *
 * Looks up the named object in the directory of the specified model,
 * obtaining a directory pointer.  Then gets that object from the
 * database and constructs its internal representation.  Returns
 * ID_NULL on error, otherwise returns the type of the object.
 */
RT_EXPORT extern int rt_db_lookup_internal(struct db_i *dbip,
					   const char *obj_name,
					   struct directory **dpp,
					   struct rt_db_internal *ip,
					   int noisy,
					   struct resource *resp);

RT_EXPORT extern void rt_optim_tree(union tree *tp,
				    struct resource *resp);

/**
 * This subroutine is called for a no-frills tree-walk, with the
 * provided subroutines being called at every combination and leaf
 * (solid) node, respectively.
 *
 * This routine is recursive, so no variables may be declared static.
 */
RT_EXPORT extern void db_functree(struct db_i *dbip,
				  struct directory *dp,
				  void (*comb_func)(struct db_i *,
						    struct directory *,
						    void *),
				  void (*leaf_func)(struct db_i *,
						    struct directory *,
						    void *),
				  struct resource *resp,
				  void *client_data);

/* mirror.c */
RT_EXPORT extern struct rt_db_internal *rt_mirror(struct db_i *dpip,
						  struct rt_db_internal *ip,
						  point_t mirror_pt,
						  vect_t mirror_dir,
						  struct resource *resp);

/*
  RT_EXPORT extern void db_preorder_traverse(struct directory *dp,
  struct db_traverse *dtp);
*/

/* arb8.c */
RT_EXPORT extern int rt_arb_get_cgtype(
    int *cgtype,
    struct rt_arb_internal *arb,
    const struct bn_tol *tol,
    register int *uvec,  /* array of indexes to unique points in arb->pt[] */
    register int *svec); /* array of indexes to like points in arb->pt[] */
RT_EXPORT extern int rt_arb_std_type(const struct rt_db_internal *ip,
				     const struct bn_tol *tol);
RT_EXPORT extern void rt_arb_centroid(point_t                       *cent,
				      const struct rt_db_internal   *ip);
RT_EXPORT extern int rt_arb_calc_points(struct rt_arb_internal *arb, int cgtype, const plane_t planes[6], const struct bn_tol *tol);		/* needs wdb.h for arg list */
RT_EXPORT extern int rt_arb_check_points(struct rt_arb_internal *arb,
					 int cgtype,
					 const struct bn_tol *tol);
RT_EXPORT extern int rt_arb_3face_intersect(point_t			point,
					    const plane_t		planes[6],
					    int			type,		/* 4..8 */
					    int			loc);
RT_EXPORT extern int rt_arb_calc_planes(struct bu_vls		*error_msg_ret,
					struct rt_arb_internal	*arb,
					int			type,
					plane_t			planes[6],
					const struct bn_tol	*tol);
RT_EXPORT extern int rt_arb_move_edge(struct bu_vls		*error_msg_ret,
				      struct rt_arb_internal	*arb,
				      vect_t			thru,
				      int			bp1,
				      int			bp2,
				      int			end1,
				      int			end2,
				      const vect_t		dir,
				      plane_t			planes[6],
				      const struct bn_tol	*tol);
RT_EXPORT extern int rt_arb_edit(struct bu_vls		*error_msg_ret,
				 struct rt_arb_internal	*arb,
				 int			arb_type,
				 int			edit_type,
				 vect_t			pos_model,
				 plane_t			planes[6],
				 const struct bn_tol	*tol);
RT_EXPORT extern int rt_arb_find_e_nearest_pt2(int *edge, int *vert1, int *vert2, const struct rt_db_internal *ip, const point_t pt2, const mat_t mat, fastf_t ptol);

/* epa.c */
RT_EXPORT extern void rt_ell(fastf_t *ov,
			     const fastf_t *V,
			     const fastf_t *A,
			     const fastf_t *B,
			     int sides);

/* pipe.c */
RT_EXPORT extern void rt_vls_pipept(struct bu_vls *vp,
				    int seg_no,
				    const struct rt_db_internal *ip,
				    double mm2local);
RT_EXPORT extern void rt_pipept_print(const struct wdb_pipept *pipept, double mm2local);
RT_EXPORT extern int rt_pipe_ck(const struct bu_list *headp);

/* metaball.c */
struct rt_metaball_internal;
RT_EXPORT extern void rt_vls_metaballpt(struct bu_vls *vp,
					const int pt_no,
					const struct rt_db_internal *ip,
					const double mm2local);
RT_EXPORT extern void rt_metaballpt_print(const struct wdb_metaballpt *metaball, double mm2local);
RT_EXPORT extern int rt_metaball_ck(const struct bu_list *headp);
RT_EXPORT extern fastf_t rt_metaball_point_value(const point_t *p,
						 const struct rt_metaball_internal *mb);
RT_EXPORT extern int rt_metaball_point_inside(const point_t *p,
					      const struct rt_metaball_internal *mb);
RT_EXPORT extern int rt_metaball_lookup_type_id(const char *name);
RT_EXPORT extern const char *rt_metaball_lookup_type_name(const int id);
RT_EXPORT extern int rt_metaball_add_point(struct rt_metaball_internal *,
					   const point_t *loc,
					   const fastf_t fldstr,
					   const fastf_t goo);

/* rpc.c */
RT_EXPORT extern int rt_mk_parabola(struct rt_pt_node *pts,
				    fastf_t r,
				    fastf_t b,
				    fastf_t dtol,
				    fastf_t ntol);

/* memalloc.c -- non PARALLEL routines */

/**
 * Takes:		& pointer of map,
 * size.
 *
 * Returns:	NULL Error
 * <addr> Otherwise
 *
 * Comments:
 * Algorithm is first fit.
 */
RT_EXPORT extern size_t rt_memalloc(struct mem_map **pp,
				    size_t size);

/**
 * Takes:		& pointer of map,
 * size.
 *
 * Returns:	NULL Error
 * <addr> Otherwise
 *
 * Comments:
 * Algorithm is BEST fit.
 */
RT_EXPORT extern struct mem_map * rt_memalloc_nosplit(struct mem_map **pp,
						      size_t size);

/**
 * Returns:	NULL Error
 * <addr> Otherwise
 *
 * Comments:
 * Algorithm is first fit.
 * Free space can be split
 */
RT_EXPORT extern size_t rt_memget(struct mem_map **pp,
				  size_t size,
				  off_t place);

/**
 * Takes:
 * size,
 * address.
 *
 * Comments:
 * The routine does not check for wrap around when increasing sizes
 * or changing addresses.  Other wrap-around conditions are flagged.
 */
RT_EXPORT extern void rt_memfree(struct mem_map **pp,
				 size_t size,
				 off_t addr);

/**
 * Take everything on the current memory chain, and place it on the
 * freelist.
 */
RT_EXPORT extern void rt_mempurge(struct mem_map **pp);

/**
 * Print a memory chain.
 */
RT_EXPORT extern void rt_memprint(struct mem_map **pp);

/**
 * Return all the storage used by the rt_mem_freemap.
 */
RT_EXPORT extern void rt_memclose(void);


RT_EXPORT extern struct bn_vlblock *rt_vlblock_init(void);


RT_EXPORT extern void rt_vlblock_free(struct bn_vlblock *vbp);

RT_EXPORT extern struct bu_list *rt_vlblock_find(struct bn_vlblock *vbp,
						 int r,
						 int g,
						 int b);

/* ars.c */
RT_EXPORT extern void rt_hitsort(struct hit h[],
				 int nh);

/* pg.c */
RT_EXPORT extern int rt_pg_to_bot(struct rt_db_internal *ip,
				  const struct bn_tol *tol,
				  struct resource *resp0);
RT_EXPORT extern int rt_pg_plot(struct bu_list		*vhead,
				struct rt_db_internal	*ip,
				const struct rt_tess_tol *ttol,
				const struct bn_tol	*tol,
				const struct rt_view_info *info);
RT_EXPORT extern int rt_pg_plot_poly(struct bu_list		*vhead,
				     struct rt_db_internal	*ip,
				     const struct rt_tess_tol	*ttol,
				     const struct bn_tol	*tol);

/* hf.c */
RT_EXPORT extern int rt_hf_to_dsp(struct rt_db_internal *db_intern);

/* dsp.c */
RT_EXPORT extern int dsp_pos(point_t out,
			     struct soltab *stp,
			     point_t p);

/* pr.c */
RT_EXPORT extern void rt_pr_soltab(const struct soltab *stp);
RT_EXPORT extern void rt_pr_region(const struct region *rp);
RT_EXPORT extern void rt_pr_partitions(const struct rt_i *rtip,
				       const struct partition	*phead,
				       const char *title);
RT_EXPORT extern void rt_pr_pt_vls(struct bu_vls *v,
				   const struct rt_i *rtip,
				   const struct partition *pp);
RT_EXPORT extern void rt_pr_pt(const struct rt_i *rtip,
			       const struct partition *pp);
RT_EXPORT extern void rt_pr_seg_vls(struct bu_vls *,
				    const struct seg *);
RT_EXPORT extern void rt_pr_seg(const struct seg *segp);
RT_EXPORT extern void rt_pr_hit(const char *str,
				const struct hit	*hitp);
RT_EXPORT extern void rt_pr_hit_vls(struct bu_vls *v,
				    const char *str,
				    const struct hit *hitp);
RT_EXPORT extern void rt_pr_hitarray_vls(struct bu_vls *v,
					 const char *str,
					 const struct hit *hitp,
					 int count);
RT_EXPORT extern void rt_pr_tree(const union tree *tp,
				 int lvl);
RT_EXPORT extern void rt_pr_tree_vls(struct bu_vls *vls,
				     const union tree *tp);
RT_EXPORT extern char *rt_pr_tree_str(const union tree *tree);
RT_EXPORT extern void rt_pr_tree_val(const union tree *tp,
				     const struct partition *partp,
				     int pr_name,
				     int lvl);
RT_EXPORT extern void rt_pr_fallback_angle(struct bu_vls *str,
					   const char *prefix,
					   const double angles[5]);
RT_EXPORT extern void rt_find_fallback_angle(double angles[5],
					     const vect_t vec);
RT_EXPORT extern void rt_pr_tol(const struct bn_tol *tol);

/* regionfix.c */
/** @addtogroup librt */
/** @{ */
/** @file librt/regionfix.c
 *
 * Subroutines for adjusting old GIFT-style region-IDs, to take into
 * account the presence of instancing.
 *
 */

/**
 * Apply any deltas to reg_regionid values to allow old applications
 * that use the reg_regionid number to distinguish between different
 * instances of the same prototype region.
 *
 * Called once, from rt_prep(), before raytracing begins.
 */
RT_EXPORT extern void rt_regionfix(struct rt_i *rtip);

/* table.c */
RT_EXPORT extern int rt_id_solid(struct bu_external *ep);
RT_EXPORT extern const struct rt_functab *rt_get_functab_by_label(const char *label);
RT_EXPORT extern int rt_generic_xform(struct rt_db_internal	*op,
				      const mat_t		mat,
				      struct rt_db_internal	*ip,
				      int			avail,
				      struct db_i		*dbip,
				      struct resource		*resp);


/* prep.c */
RT_EXPORT extern void rt_plot_all_bboxes(FILE *fp,
					 struct rt_i *rtip);
RT_EXPORT extern void rt_plot_all_solids(FILE		*fp,
					 struct rt_i	*rtip,
					 struct resource	*resp);

RT_EXPORT extern int rt_find_paths(struct db_i *dbip,
				   struct directory *start,
				   struct directory *end,
				   struct bu_ptbl *paths,
				   struct resource *resp);

RT_EXPORT extern struct bu_bitv *rt_get_solidbitv(size_t nbits,
						  struct resource *resp);

#include "rt/prep.h"

/** @} */

/* shoot.c */
/** @addtogroup ray */
/** @{ */
/** @file librt/shoot.c
 *
 * Ray Tracing program shot coordinator.
 *
 * This is the heart of LIBRT's ray-tracing capability.
 *
 * Given a ray, shoot it at all the relevant parts of the model,
 * (building the finished_segs chain), and then call rt_boolregions()
 * to build and evaluate the partition chain.  If the ray actually hit
 * anything, call the application's a_hit() routine with a pointer to
 * the partition chain, otherwise, call the application's a_miss()
 * routine.
 *
 * It is important to note that rays extend infinitely only in the
 * positive direction.  The ray is composed of all points P, where
 *
 * P = r_pt + K * r_dir
 *
 * for K ranging from 0 to +infinity.  There is no looking backwards.
 *
 */


/**
 * To be called only in non-parallel mode, to tally up the statistics
 * from the resource structure(s) into the rt instance structure.
 *
 * Non-parallel programs should call
 * rt_add_res_stats(rtip, RESOURCE_NULL);
 * to have the default resource results tallied in.
 */
RT_EXPORT extern void rt_add_res_stats(struct rt_i *rtip,
				       struct resource *resp);
/* Tally stats into struct rt_i */
RT_EXPORT extern void rt_zero_res_stats(struct resource *resp);


RT_EXPORT extern void rt_res_pieces_clean(struct resource *resp,
					  struct rt_i *rtip);


/**
 * Allocate the per-processor state variables needed to support
 * rt_shootray()'s use of 'solid pieces'.
 */
RT_EXPORT extern void rt_res_pieces_init(struct resource *resp,
					 struct rt_i *rtip);
RT_EXPORT extern void rt_vstub(struct soltab *stp[],
			       struct xray *rp[],
			       struct  seg segp[],
			       int n,
			       struct application	*ap);

/** @} */


/* tree.c */
/** @addtogroup librt */
/** @{ */
/** @file librt/tree.c
 *
 * Ray Tracing library database tree walker.
 *
 * Collect and prepare regions and solids for subsequent ray-tracing.
 *
 */


/**
 * Calculate the bounding RPP of the region whose boolean tree is
 * 'tp'.  The bounding RPP is returned in tree_min and tree_max, which
 * need not have been initialized first.
 *
 * Returns -
 * 0 success
 * -1 failure (tree_min and tree_max may have been altered)
 */
RT_EXPORT extern int rt_bound_tree(const union tree	*tp,
				   vect_t		tree_min,
				   vect_t		tree_max);

/**
 * Eliminate any references to NOP nodes from the tree.  It is safe to
 * use db_free_tree() here, because there will not be any dead solids.
 * They will all have been converted to OP_NOP nodes by
 * _rt_tree_kill_dead_solid_refs(), previously, so there is no need to
 * worry about multiple db_free_tree()'s repeatedly trying to free one
 * solid that has been instanced multiple times.
 *
 * Returns -
 * 0 this node is OK.
 * -1 request caller to kill this node
 */
RT_EXPORT extern int rt_tree_elim_nops(union tree *,
				       struct resource *resp);

/** @} */


/* vlist.c */
/** @addtogroup librt */
/** @{ */
/** @file librt/vlist.c
 *
 * Routines for the import and export of vlist chains as:
 * Network independent binary,
 * BRL-extended UNIX-Plot files.
 *
 */

/* FIXME: Has some stuff mixed in here that should go in LIBBN */
/************************************************************************
 *									*
 *			Generic VLBLOCK routines			*
 *									*
 ************************************************************************/

RT_EXPORT extern struct bn_vlblock *bn_vlblock_init(struct bu_list	*free_vlist_hd,	/* where to get/put free vlists */
						    int		max_ent);

RT_EXPORT extern struct bn_vlblock *	rt_vlblock_init(void);


RT_EXPORT extern void rt_vlblock_free(struct bn_vlblock *vbp);

RT_EXPORT extern struct bu_list *rt_vlblock_find(struct bn_vlblock *vbp,
						 int r,
						 int g,
						 int b);


/************************************************************************
 *									*
 *			Generic BN_VLIST routines			*
 *									*
 ************************************************************************/

/**
 * Returns the description of a vlist cmd.
 */
RT_EXPORT extern const char *rt_vlist_get_cmd_description(int cmd);

/**
 * Validate an bn_vlist chain for having reasonable values inside.
 * Calls bu_bomb() if not.
 *
 * Returns -
 * npts Number of point/command sets in use.
 */
RT_EXPORT extern int rt_ck_vlist(const struct bu_list *vhead);


/**
 * Duplicate the contents of a vlist.  Note that the copy may be more
 * densely packed than the source.
 */
RT_EXPORT extern void rt_vlist_copy(struct bu_list *dest,
				    const struct bu_list *src);


/**
 * The macro RT_FREE_VLIST() simply appends to the list
 * &RTG.rtg_vlfree.  Now, give those structures back to bu_free().
 */
RT_EXPORT extern void bn_vlist_cleanup(struct bu_list *hd);


/**
 * XXX This needs to remain a LIBRT function.
 */
RT_EXPORT extern void rt_vlist_cleanup(void);



/************************************************************************
 *									*
 *			Binary VLIST import/export routines		*
 *									*
 ************************************************************************/

/**
 * Convert a vlist chain into a blob of network-independent binary,
 * for transmission to another machine.
 *
 * The result is stored in a vls string, so that both the address and
 * length are available conveniently.
 */
RT_EXPORT extern void rt_vlist_export(struct bu_vls *vls,
				      struct bu_list *hp,
				      const char *name);


/**
 * Convert a blob of network-independent binary prepared by
 * vls_vlist() and received from another machine, into a vlist chain.
 */
RT_EXPORT extern void rt_vlist_import(struct bu_list *hp,
				      struct bu_vls *namevls,
				      const unsigned char *buf);


/************************************************************************
 *									*
 *			UNIX-Plot VLIST import/export routines		*
 *									*
 ************************************************************************/

/**
 * Output a bn_vlblock object in extended UNIX-plot format, including
 * color.
 */
RT_EXPORT extern void rt_plot_vlblock(FILE *fp,
				      const struct bn_vlblock *vbp);


/**
 * Output a vlist as an extended 3-D floating point UNIX-Plot file.
 * You provide the file.  Uses libplot3 routines to create the
 * UNIX-Plot output.
 */
RT_EXPORT extern void rt_vlist_to_uplot(FILE *fp,
					const struct bu_list *vhead);
RT_EXPORT extern int rt_process_uplot_value(struct bu_list **vhead,
					    struct bn_vlblock *vbp,
					    FILE *fp,
					    int c,
					    double char_size,
					    int mode);


/**
 * Read a BRL-style 3-D UNIX-plot file into a vector list.  For now,
 * discard color information, only extract vectors.  This might be
 * more naturally located in mged/plot.c
 */
RT_EXPORT extern int rt_uplot_to_vlist(struct bn_vlblock *vbp,
				       FILE *fp,
				       double char_size,
				       int mode);


/**
 * Used by MGED's "labelvert" command.
 */
RT_EXPORT extern void rt_label_vlist_verts(struct bn_vlblock *vbp,
					   struct bu_list *src,
					   mat_t mat,
					   double sz,
					   double mm2local);
/** @} */
/* sketch.c */
RT_EXPORT extern int curve_to_vlist(struct bu_list		*vhead,
				    const struct rt_tess_tol	*ttol,
				    point_t			V,
				    vect_t			u_vec,
				    vect_t			v_vec,
				    struct rt_sketch_internal *sketch_ip,
				    struct rt_curve		*crv);

RT_EXPORT extern int rt_check_curve(const struct rt_curve *crv,
				    const struct rt_sketch_internal *skt,
				    int noisy);

RT_EXPORT extern void rt_curve_reverse_segment(uint32_t *lng);
RT_EXPORT extern void rt_curve_order_segments(struct rt_curve *crv);

RT_EXPORT extern void rt_copy_curve(struct rt_curve *crv_out,
				    const struct rt_curve *crv_in);

RT_EXPORT extern void rt_curve_free(struct rt_curve *crv);
RT_EXPORT extern void rt_copy_curve(struct rt_curve *crv_out,
				    const struct rt_curve *crv_in);
RT_EXPORT extern struct rt_sketch_internal *rt_copy_sketch(const struct rt_sketch_internal *sketch_ip);
RT_EXPORT extern int curve_to_tcl_list(struct bu_vls *vls,
				       struct rt_curve *crv);

/* htbl.c */

RT_EXPORT extern void rt_htbl_init(struct rt_htbl *b, size_t len, const char *str);

/**
 * Reset the table to have no elements, but retain any existing storage.
 */
RT_EXPORT extern void rt_htbl_reset(struct rt_htbl *b);

/**
 * Deallocate dynamic hit buffer and render unusable without a
 * subsequent rt_htbl_init().
 */
RT_EXPORT extern void rt_htbl_free(struct rt_htbl *b);

/**
 * Allocate another hit structure, extending the array if necessary.
 */
RT_EXPORT extern struct hit *rt_htbl_get(struct rt_htbl *b);

/************************************************************************
 *									*
 *			NMG Support Function Declarations		*
 *									*
 ************************************************************************/
#if defined(NMG_H)

/* From file nmg_mk.c */
/*	MAKE routines */
RT_EXPORT extern struct model *nmg_mm(void);
RT_EXPORT extern struct model *nmg_mmr(void);
RT_EXPORT extern struct nmgregion *nmg_mrsv(struct model *m);
RT_EXPORT extern struct shell *nmg_msv(struct nmgregion *r_p);
RT_EXPORT extern struct faceuse *nmg_mf(struct loopuse *lu1);
RT_EXPORT extern struct loopuse *nmg_mlv(uint32_t *magic,
					 struct vertex *v,
					 int orientation);
RT_EXPORT extern struct edgeuse *nmg_me(struct vertex *v1,
					struct vertex *v2,
					struct shell *s);
RT_EXPORT extern struct edgeuse *nmg_meonvu(struct vertexuse *vu);
RT_EXPORT extern struct loopuse *nmg_ml(struct shell *s);
/*	KILL routines */
RT_EXPORT extern int nmg_keg(struct edgeuse *eu);
RT_EXPORT extern int nmg_kvu(struct vertexuse *vu);
RT_EXPORT extern int nmg_kfu(struct faceuse *fu1);
RT_EXPORT extern int nmg_klu(struct loopuse *lu1);
RT_EXPORT extern int nmg_keu(struct edgeuse *eu);
RT_EXPORT extern int nmg_keu_zl(struct shell *s,
				const struct bn_tol *tol);
RT_EXPORT extern int nmg_ks(struct shell *s);
RT_EXPORT extern int nmg_kr(struct nmgregion *r);
RT_EXPORT extern void nmg_km(struct model *m);
/*	Geometry and Attribute routines */
RT_EXPORT extern void nmg_vertex_gv(struct vertex *v,
				    const point_t pt);
RT_EXPORT extern void nmg_vertex_g(struct vertex *v,
				   fastf_t x,
				   fastf_t y,
				   fastf_t z);
RT_EXPORT extern void nmg_vertexuse_nv(struct vertexuse *vu,
				       const vect_t norm);
RT_EXPORT extern void nmg_vertexuse_a_cnurb(struct vertexuse *vu,
					    const fastf_t *uvw);
RT_EXPORT extern void nmg_edge_g(struct edgeuse *eu);
RT_EXPORT extern void nmg_edge_g_cnurb(struct edgeuse *eu,
				       int order,
				       int n_knots,
				       fastf_t *kv,
				       int n_pts,
				       int pt_type,
				       fastf_t *points);
RT_EXPORT extern void nmg_edge_g_cnurb_plinear(struct edgeuse *eu);
RT_EXPORT extern int nmg_use_edge_g(struct edgeuse *eu,
				    uint32_t *eg);
RT_EXPORT extern void nmg_loop_g(struct loop *l,
				 const struct bn_tol *tol);
RT_EXPORT extern void nmg_face_g(struct faceuse *fu,
				 const plane_t p);
RT_EXPORT extern void nmg_face_new_g(struct faceuse *fu,
				     const plane_t pl);
RT_EXPORT extern void nmg_face_g_snurb(struct faceuse *fu,
				       int u_order,
				       int v_order,
				       int n_u_knots,
				       int n_v_knots,
				       fastf_t *ukv,
				       fastf_t *vkv,
				       int n_rows,
				       int n_cols,
				       int pt_type,
				       fastf_t *mesh);
RT_EXPORT extern void nmg_face_bb(struct face *f,
				  const struct bn_tol *tol);
RT_EXPORT extern void nmg_shell_a(struct shell *s,
				  const struct bn_tol *tol);
RT_EXPORT extern void nmg_region_a(struct nmgregion *r,
				   const struct bn_tol *tol);
/*	DEMOTE routines */
RT_EXPORT extern int nmg_demote_lu(struct loopuse *lu);
RT_EXPORT extern int nmg_demote_eu(struct edgeuse *eu);
/*	MODIFY routines */
RT_EXPORT extern void nmg_movevu(struct vertexuse *vu,
				 struct vertex *v);
RT_EXPORT extern void nmg_je(struct edgeuse *eudst,
			     struct edgeuse *eusrc);
RT_EXPORT extern void nmg_unglueedge(struct edgeuse *eu);
RT_EXPORT extern void nmg_jv(struct vertex *v1,
			     struct vertex *v2);
RT_EXPORT extern void nmg_jfg(struct face *f1,
			      struct face *f2);
RT_EXPORT extern void nmg_jeg(struct edge_g_lseg *dest_eg,
			      struct edge_g_lseg *src_eg);

/* From nmg_mod.c */
/*	REGION Routines */
RT_EXPORT extern void nmg_merge_regions(struct nmgregion *r1,
					struct nmgregion *r2,
					const struct bn_tol *tol);

/*	SHELL Routines */
RT_EXPORT extern void nmg_shell_coplanar_face_merge(struct shell *s,
						    const struct bn_tol *tol,
						    const int simplify);
RT_EXPORT extern int nmg_simplify_shell(struct shell *s);
RT_EXPORT extern void nmg_rm_redundancies(struct shell *s,
					  const struct bn_tol *tol);
RT_EXPORT extern void nmg_sanitize_s_lv(struct shell *s,
					int orient);
RT_EXPORT extern void nmg_s_split_touchingloops(struct shell *s,
						const struct bn_tol *tol);
RT_EXPORT extern void nmg_s_join_touchingloops(struct shell		*s,
					       const struct bn_tol	*tol);
RT_EXPORT extern void nmg_js(struct shell	*s1,
			     struct shell	*s2,
			     const struct bn_tol	*tol);
RT_EXPORT extern void nmg_invert_shell(struct shell		*s);

/*	FACE Routines */
RT_EXPORT extern struct faceuse *nmg_cmface(struct shell *s,
					    struct vertex **vt[],
					    int n);
RT_EXPORT extern struct faceuse *nmg_cface(struct shell *s,
					   struct vertex **vt,
					   int n);
RT_EXPORT extern struct faceuse *nmg_add_loop_to_face(struct shell *s,
						      struct faceuse *fu,
						      struct vertex **verts,
						      int n,
						      int dir);
RT_EXPORT extern int nmg_fu_planeeqn(struct faceuse *fu,
				     const struct bn_tol *tol);
RT_EXPORT extern void nmg_gluefaces(struct faceuse *fulist[],
				    int n,
				    const struct bn_tol *tol);
RT_EXPORT extern int nmg_simplify_face(struct faceuse *fu);
RT_EXPORT extern void nmg_reverse_face(struct faceuse *fu);
RT_EXPORT extern void nmg_mv_fu_between_shells(struct shell *dest,
					       struct shell *src,
					       struct faceuse *fu);
RT_EXPORT extern void nmg_jf(struct faceuse *dest_fu,
			     struct faceuse *src_fu);
RT_EXPORT extern struct faceuse *nmg_dup_face(struct faceuse *fu,
					      struct shell *s);
/*	LOOP Routines */
RT_EXPORT extern void nmg_jl(struct loopuse *lu,
			     struct edgeuse *eu);
RT_EXPORT extern struct vertexuse *nmg_join_2loops(struct vertexuse *vu1,
						   struct vertexuse *vu2);
RT_EXPORT extern struct vertexuse *nmg_join_singvu_loop(struct vertexuse *vu1,
							struct vertexuse *vu2);
RT_EXPORT extern struct vertexuse *nmg_join_2singvu_loops(struct vertexuse *vu1,
							  struct vertexuse *vu2);
RT_EXPORT extern struct loopuse *nmg_cut_loop(struct vertexuse *vu1,
					      struct vertexuse *vu2);
RT_EXPORT extern struct loopuse *nmg_split_lu_at_vu(struct loopuse *lu,
						    struct vertexuse *vu);
RT_EXPORT extern struct vertexuse *nmg_find_repeated_v_in_lu(struct vertexuse *vu);
RT_EXPORT extern void nmg_split_touchingloops(struct loopuse *lu,
					      const struct bn_tol *tol);
RT_EXPORT extern int nmg_join_touchingloops(struct loopuse *lu);
RT_EXPORT extern int nmg_get_touching_jaunts(const struct loopuse *lu,
					     struct bu_ptbl *tbl,
					     int *need_init);
RT_EXPORT extern void nmg_kill_accordions(struct loopuse *lu);
RT_EXPORT extern int nmg_loop_split_at_touching_jaunt(struct loopuse		*lu,
						      const struct bn_tol	*tol);
RT_EXPORT extern void nmg_simplify_loop(struct loopuse *lu);
RT_EXPORT extern int nmg_kill_snakes(struct loopuse *lu);
RT_EXPORT extern void nmg_mv_lu_between_shells(struct shell *dest,
					       struct shell *src,
					       struct loopuse *lu);
RT_EXPORT extern void nmg_moveltof(struct faceuse *fu,
				   struct shell *s);
RT_EXPORT extern struct loopuse *nmg_dup_loop(struct loopuse *lu,
					      uint32_t *parent,
					      long **trans_tbl);
RT_EXPORT extern void nmg_set_lu_orientation(struct loopuse *lu,
					     int is_opposite);
RT_EXPORT extern void nmg_lu_reorient(struct loopuse *lu);
/*	EDGE Routines */
RT_EXPORT extern struct edgeuse *nmg_eusplit(struct vertex *v,
					     struct edgeuse *oldeu,
					     int share_geom);
RT_EXPORT extern struct edgeuse *nmg_esplit(struct vertex *v,
					    struct edgeuse *eu,
					    int share_geom);
RT_EXPORT extern struct edgeuse *nmg_ebreak(struct vertex *v,
					    struct edgeuse *eu);
RT_EXPORT extern struct edgeuse *nmg_ebreaker(struct vertex *v,
					      struct edgeuse *eu,
					      const struct bn_tol *tol);
RT_EXPORT extern struct vertex *nmg_e2break(struct edgeuse *eu1,
					    struct edgeuse *eu2);
RT_EXPORT extern int nmg_unbreak_edge(struct edgeuse *eu1_first);
RT_EXPORT extern int nmg_unbreak_shell_edge_unsafe(struct edgeuse *eu1_first);
RT_EXPORT extern struct edgeuse *nmg_eins(struct edgeuse *eu);
RT_EXPORT extern void nmg_mv_eu_between_shells(struct shell *dest,
					       struct shell *src,
					       struct edgeuse *eu);
/*	VERTEX Routines */
RT_EXPORT extern void nmg_mv_vu_between_shells(struct shell *dest,
					       struct shell *src,
					       struct vertexuse *vu);

/* From nmg_info.c */
/* Model routines */
RT_EXPORT extern struct model *nmg_find_model(const uint32_t *magic_p);
RT_EXPORT extern struct shell *nmg_find_shell(const uint32_t *magic_p);
RT_EXPORT extern void nmg_model_bb(point_t min_pt,
				   point_t max_pt,
				   const struct model *m);


/* Shell routines */
RT_EXPORT extern int nmg_shell_is_empty(const struct shell *s);
RT_EXPORT extern struct shell *nmg_find_s_of_lu(const struct loopuse *lu);
RT_EXPORT extern struct shell *nmg_find_s_of_eu(const struct edgeuse *eu);
RT_EXPORT extern struct shell *nmg_find_s_of_vu(const struct vertexuse *vu);

/* Face routines */
RT_EXPORT extern struct faceuse *nmg_find_fu_of_eu(const struct edgeuse *eu);
RT_EXPORT extern struct faceuse *nmg_find_fu_of_lu(const struct loopuse *lu);
RT_EXPORT extern struct faceuse *nmg_find_fu_of_vu(const struct vertexuse *vu);
RT_EXPORT extern struct faceuse *nmg_find_fu_with_fg_in_s(const struct shell *s1,
							  const struct faceuse *fu2);
RT_EXPORT extern double nmg_measure_fu_angle(const struct edgeuse *eu,
					     const vect_t xvec,
					     const vect_t yvec,
					     const vect_t zvec);

/* Loop routines */
RT_EXPORT extern struct loopuse*nmg_find_lu_of_vu(const struct vertexuse *vu);
RT_EXPORT extern int nmg_loop_is_a_crack(const struct loopuse *lu);
RT_EXPORT extern int	nmg_loop_is_ccw(const struct loopuse *lu,
					const plane_t norm,
					const struct bn_tol *tol);
RT_EXPORT extern const struct vertexuse *nmg_loop_touches_self(const struct loopuse *lu);
RT_EXPORT extern int nmg_2lu_identical(const struct edgeuse *eu1,
				       const struct edgeuse *eu2);

/* Edge routines */
RT_EXPORT extern struct edgeuse *nmg_find_matching_eu_in_s(const struct edgeuse	*eu1,
							   const struct shell	*s2);
RT_EXPORT extern struct edgeuse	*nmg_findeu(const struct vertex *v1,
					    const struct vertex *v2,
					    const struct shell *s,
					    const struct edgeuse *eup,
					    int dangling_only);
RT_EXPORT extern struct edgeuse *nmg_find_eu_in_face(const struct vertex *v1,
						     const struct vertex *v2,
						     const struct faceuse *fu,
						     const struct edgeuse *eup,
						     int dangling_only);
RT_EXPORT extern struct edgeuse *nmg_find_e(const struct vertex *v1,
					    const struct vertex *v2,
					    const struct shell *s,
					    const struct edge *ep);
RT_EXPORT extern struct edgeuse *nmg_find_eu_of_vu(const struct vertexuse *vu);
RT_EXPORT extern struct edgeuse *nmg_find_eu_with_vu_in_lu(const struct loopuse *lu,
							   const struct vertexuse *vu);
RT_EXPORT extern const struct edgeuse *nmg_faceradial(const struct edgeuse *eu);
RT_EXPORT extern const struct edgeuse *nmg_radial_face_edge_in_shell(const struct edgeuse *eu);
RT_EXPORT extern const struct edgeuse *nmg_find_edge_between_2fu(const struct faceuse *fu1,
								 const struct faceuse *fu2,
								 const struct bn_tol *tol);
RT_EXPORT extern struct edge *nmg_find_e_nearest_pt2(uint32_t *magic_p,
						     const point_t pt2,
						     const mat_t mat,
						     const struct bn_tol *tol);
RT_EXPORT extern struct edgeuse *nmg_find_matching_eu_in_s(const struct edgeuse *eu1,
							   const struct shell *s2);
RT_EXPORT extern void nmg_eu_2vecs_perp(vect_t xvec,
					vect_t yvec,
					vect_t zvec,
					const struct edgeuse *eu,
					const struct bn_tol *tol);
RT_EXPORT extern int nmg_find_eu_leftvec(vect_t left,
					 const struct edgeuse *eu);
RT_EXPORT extern int nmg_find_eu_left_non_unit(vect_t left,
					       const struct edgeuse	*eu);
RT_EXPORT extern struct edgeuse *nmg_find_ot_same_eu_of_e(const struct edge *e);

/* Vertex routines */
RT_EXPORT extern struct vertexuse *nmg_find_v_in_face(const struct vertex *,
						      const struct faceuse *);
RT_EXPORT extern struct vertexuse *nmg_find_v_in_shell(const struct vertex *v,
						       const struct shell *s,
						       int edges_only);
RT_EXPORT extern struct vertexuse *nmg_find_pt_in_lu(const struct loopuse *lu,
						     const point_t pt,
						     const struct bn_tol *tol);
RT_EXPORT extern struct vertexuse *nmg_find_pt_in_face(const struct faceuse *fu,
						       const point_t pt,
						       const struct bn_tol *tol);
RT_EXPORT extern struct vertex *nmg_find_pt_in_shell(const struct shell *s,
						     const point_t pt,
						     const struct bn_tol *tol);
RT_EXPORT extern struct vertex *nmg_find_pt_in_model(const struct model *m,
						     const point_t pt,
						     const struct bn_tol *tol);
RT_EXPORT extern int nmg_is_vertex_in_edgelist(const struct vertex *v,
					       const struct bu_list *hd);
RT_EXPORT extern int nmg_is_vertex_in_looplist(const struct vertex *v,
					       const struct bu_list *hd,
					       int singletons);
RT_EXPORT extern struct vertexuse *nmg_is_vertex_in_face(const struct vertex *v,
							 const struct face *f);
RT_EXPORT extern int nmg_is_vertex_a_selfloop_in_shell(const struct vertex *v,
						       const struct shell *s);
RT_EXPORT extern int nmg_is_vertex_in_facelist(const struct vertex *v,
					       const struct bu_list *hd);
RT_EXPORT extern int nmg_is_edge_in_edgelist(const struct edge *e,
					     const struct bu_list *hd);
RT_EXPORT extern int nmg_is_edge_in_looplist(const struct edge *e,
					     const struct bu_list *hd);
RT_EXPORT extern int nmg_is_edge_in_facelist(const struct edge *e,
					     const struct bu_list *hd);
RT_EXPORT extern int nmg_is_loop_in_facelist(const struct loop *l,
					     const struct bu_list *fu_hd);

/* Tabulation routines */
RT_EXPORT extern void nmg_vertex_tabulate(struct bu_ptbl *tab,
					  const uint32_t *magic_p);
RT_EXPORT extern void nmg_vertexuse_normal_tabulate(struct bu_ptbl *tab,
						    const uint32_t *magic_p);
RT_EXPORT extern void nmg_edgeuse_tabulate(struct bu_ptbl *tab,
					   const uint32_t *magic_p);
RT_EXPORT extern void nmg_edge_tabulate(struct bu_ptbl *tab,
					const uint32_t *magic_p);
RT_EXPORT extern void nmg_edge_g_tabulate(struct bu_ptbl *tab,
					  const uint32_t *magic_p);
RT_EXPORT extern void nmg_face_tabulate(struct bu_ptbl *tab,
					const uint32_t *magic_p);
RT_EXPORT extern void nmg_edgeuse_with_eg_tabulate(struct bu_ptbl *tab,
						   const struct edge_g_lseg *eg);
RT_EXPORT extern void nmg_edgeuse_on_line_tabulate(struct bu_ptbl *tab,
						   const uint32_t *magic_p,
						   const point_t pt,
						   const vect_t dir,
						   const struct bn_tol *tol);
RT_EXPORT extern void nmg_e_and_v_tabulate(struct bu_ptbl *eutab,
					   struct bu_ptbl *vtab,
					   const uint32_t *magic_p);
RT_EXPORT extern int nmg_2edgeuse_g_coincident(const struct edgeuse	*eu1,
					       const struct edgeuse	*eu2,
					       const struct bn_tol	*tol);

/* From nmg_extrude.c */
RT_EXPORT extern void nmg_translate_face(struct faceuse *fu, const vect_t Vec);
RT_EXPORT extern int nmg_extrude_face(struct faceuse *fu, const vect_t Vec, const struct bn_tol *tol);
RT_EXPORT extern struct vertexuse *nmg_find_vertex_in_lu(const struct vertex *v, const struct loopuse *lu);
RT_EXPORT extern void nmg_fix_overlapping_loops(struct shell *s, const struct bn_tol *tol);
RT_EXPORT extern void nmg_break_crossed_loops(struct shell *is, const struct bn_tol *tol);
RT_EXPORT extern struct shell *nmg_extrude_cleanup(struct shell *is, const int is_void, const struct bn_tol *tol);
RT_EXPORT extern void nmg_hollow_shell(struct shell *s, const fastf_t thick, const int approximate, const struct bn_tol *tol);
RT_EXPORT extern struct shell *nmg_extrude_shell(struct shell *s, const fastf_t dist, const int normal_ward, const int approximate, const struct bn_tol *tol);

/* From nmg_pr.c */
RT_EXPORT extern char *nmg_orientation(int orientation);
RT_EXPORT extern void nmg_pr_orient(int orientation,
				    const char *h);
RT_EXPORT extern void nmg_pr_m(const struct model *m);
RT_EXPORT extern void nmg_pr_r(const struct nmgregion *r,
			       char *h);
RT_EXPORT extern void nmg_pr_sa(const struct shell_a *sa,
				char *h);
RT_EXPORT extern void nmg_pr_lg(const struct loop_g *lg,
				char *h);
RT_EXPORT extern void nmg_pr_fg(const uint32_t *magic,
				char *h);
RT_EXPORT extern void nmg_pr_s(const struct shell *s,
			       char *h);
RT_EXPORT extern void  nmg_pr_s_briefly(const struct shell *s,
					char *h);
RT_EXPORT extern void nmg_pr_f(const struct face *f,
			       char *h);
RT_EXPORT extern void nmg_pr_fu(const struct faceuse *fu,
				char *h);
RT_EXPORT extern void nmg_pr_fu_briefly(const struct faceuse *fu,
					char *h);
RT_EXPORT extern void nmg_pr_l(const struct loop *l,
			       char *h);
RT_EXPORT extern void nmg_pr_lu(const struct loopuse *lu,
				char *h);
RT_EXPORT extern void nmg_pr_lu_briefly(const struct loopuse *lu,
					char *h);
RT_EXPORT extern void nmg_pr_eg(const uint32_t *eg,
				char *h);
RT_EXPORT extern void nmg_pr_e(const struct edge *e,
			       char *h);
RT_EXPORT extern void nmg_pr_eu(const struct edgeuse *eu,
				char *h);
RT_EXPORT extern void nmg_pr_eu_briefly(const struct edgeuse *eu,
					char *h);
RT_EXPORT extern void nmg_pr_eu_endpoints(const struct edgeuse *eu,
					  char *h);
RT_EXPORT extern void nmg_pr_vg(const struct vertex_g *vg,
				char *h);
RT_EXPORT extern void nmg_pr_v(const struct vertex *v,
			       char *h);
RT_EXPORT extern void nmg_pr_vu(const struct vertexuse *vu,
				char *h);
RT_EXPORT extern void nmg_pr_vu_briefly(const struct vertexuse *vu,
					char *h);
RT_EXPORT extern void nmg_pr_vua(const uint32_t *magic_p,
				 char *h);
RT_EXPORT extern void nmg_euprint(const char *str,
				  const struct edgeuse *eu);
RT_EXPORT extern void nmg_pr_ptbl(const char *title,
				  const struct bu_ptbl *tbl,
				  int verbose);
RT_EXPORT extern void nmg_pr_ptbl_vert_list(const char *str,
					    const struct bu_ptbl *tbl,
					    const fastf_t *mag);
RT_EXPORT extern void nmg_pr_one_eu_vecs(const struct edgeuse *eu,
					 const vect_t xvec,
					 const vect_t yvec,
					 const vect_t zvec,
					 const struct bn_tol *tol);
RT_EXPORT extern void nmg_pr_fu_around_eu_vecs(const struct edgeuse *eu,
					       const vect_t xvec,
					       const vect_t yvec,
					       const vect_t zvec,
					       const struct bn_tol *tol);
RT_EXPORT extern void nmg_pr_fu_around_eu(const struct edgeuse *eu,
					  const struct bn_tol *tol);
RT_EXPORT extern void nmg_pl_lu_around_eu(const struct edgeuse *eu);
RT_EXPORT extern void nmg_pr_fus_in_fg(const uint32_t *fg_magic);

/* From nmg_misc.c */
RT_EXPORT extern int nmg_snurb_calc_lu_uv_orient(const struct loopuse *lu);
RT_EXPORT extern void nmg_snurb_fu_eval(const struct faceuse *fu,
					const fastf_t u,
					const fastf_t v,
					point_t pt_on_srf);
RT_EXPORT extern void nmg_snurb_fu_get_norm(const struct faceuse *fu,
					    const fastf_t u,
					    const fastf_t v,
					    vect_t norm);
RT_EXPORT extern void nmg_snurb_fu_get_norm_at_vu(const struct faceuse *fu,
						  const struct vertexuse *vu,
						  vect_t norm);
RT_EXPORT extern void nmg_find_zero_length_edges(const struct model *m);
RT_EXPORT extern struct face *nmg_find_top_face_in_dir(const struct shell *s,
						       int dir, long *flags);
RT_EXPORT extern struct face *nmg_find_top_face(const struct shell *s,
						int *dir,
						long *flags);
RT_EXPORT extern int nmg_find_outer_and_void_shells(struct nmgregion *r,
						    struct bu_ptbl ***shells,
						    const struct bn_tol *tol);
RT_EXPORT extern int nmg_mark_edges_real(const uint32_t *magic_p);
RT_EXPORT extern void nmg_tabulate_face_g_verts(struct bu_ptbl *tab,
						const struct face_g_plane *fg);
RT_EXPORT extern void nmg_isect_shell_self(struct shell *s,
					   const struct bn_tol *tol);
RT_EXPORT extern struct edgeuse *nmg_next_radial_eu(const struct edgeuse *eu,
						    const struct shell *s,
						    const int wires);
RT_EXPORT extern struct edgeuse *nmg_prev_radial_eu(const struct edgeuse *eu,
						    const struct shell *s,
						    const int wires);
RT_EXPORT extern int nmg_radial_face_count(const struct edgeuse *eu,
					   const struct shell *s);
RT_EXPORT extern int nmg_check_closed_shell(const struct shell *s,
					    const struct bn_tol *tol);
RT_EXPORT extern int nmg_move_lu_between_fus(struct faceuse *dest,
					     struct faceuse *src,
					     struct loopuse *lu);
RT_EXPORT extern void nmg_loop_plane_newell(const struct loopuse *lu,
					    plane_t pl);
RT_EXPORT extern fastf_t nmg_loop_plane_area(const struct loopuse *lu,
					     plane_t pl);
RT_EXPORT extern fastf_t nmg_loop_plane_area2(const struct loopuse *lu,
					      plane_t pl,
					      const struct bn_tol *tol);
RT_EXPORT extern int nmg_calc_face_plane(struct faceuse *fu_in,
					 plane_t pl);
RT_EXPORT extern int nmg_calc_face_g(struct faceuse *fu);
RT_EXPORT extern fastf_t nmg_faceuse_area(const struct faceuse *fu);
RT_EXPORT extern fastf_t nmg_shell_area(const struct shell *s);
RT_EXPORT extern fastf_t nmg_region_area(const struct nmgregion *r);
RT_EXPORT extern fastf_t nmg_model_area(const struct model *m);
/* Some stray rt_ plane functions here */
RT_EXPORT extern void nmg_purge_unwanted_intersection_points(struct bu_ptbl *vert_list,
							     fastf_t *mag,
							     const struct faceuse *fu,
							     const struct bn_tol *tol);
RT_EXPORT extern int nmg_in_or_ref(struct vertexuse *vu,
				   struct bu_ptbl *b);
RT_EXPORT extern void nmg_rebound(struct model *m,
				  const struct bn_tol *tol);
RT_EXPORT extern void nmg_count_shell_kids(const struct model *m,
					   size_t *total_wires,
					   size_t *total_faces,
					   size_t *total_points);
RT_EXPORT extern void nmg_close_shell(struct shell *s,
				      const struct bn_tol *tol);
RT_EXPORT extern struct shell *nmg_dup_shell(struct shell *s,
					     long ***copy_tbl,
					     const struct bn_tol *tol);
RT_EXPORT extern struct edgeuse *nmg_pop_eu(struct bu_ptbl *stack);
RT_EXPORT extern void nmg_reverse_radials(struct faceuse *fu,
					  const struct bn_tol *tol);
RT_EXPORT extern void nmg_reverse_face_and_radials(struct faceuse *fu,
						   const struct bn_tol *tol);
RT_EXPORT extern int nmg_shell_is_void(const struct shell *s);
RT_EXPORT extern void nmg_propagate_normals(struct faceuse *fu_in,
					    long *flags,
					    const struct bn_tol *tol);
RT_EXPORT extern void nmg_connect_same_fu_orients(struct shell *s);
RT_EXPORT extern void nmg_fix_decomposed_shell_normals(struct shell *s,
						       const struct bn_tol *tol);
RT_EXPORT extern struct model *nmg_mk_model_from_region(struct nmgregion *r,
							int reindex);
RT_EXPORT extern void nmg_fix_normals(struct shell *s_orig,
				      const struct bn_tol *tol);
RT_EXPORT extern int nmg_break_long_edges(struct shell *s,
					  const struct bn_tol *tol);
RT_EXPORT extern struct faceuse *nmg_mk_new_face_from_loop(struct loopuse *lu);
RT_EXPORT extern int nmg_split_loops_into_faces(uint32_t *magic_p,
						const struct bn_tol *tol);
RT_EXPORT extern int nmg_decompose_shell(struct shell *s,
					 const struct bn_tol *tol);
RT_EXPORT extern void nmg_stash_model_to_file(const char *filename,
					      const struct model *m,
					      const char *title);
RT_EXPORT extern int nmg_unbreak_region_edges(uint32_t *magic_p);
RT_EXPORT extern void nmg_vlist_to_eu(struct bu_list *vlist,
				      struct shell *s);
RT_EXPORT extern int nmg_mv_shell_to_region(struct shell *s,
					    struct nmgregion *r);
RT_EXPORT extern int nmg_find_isect_faces(const struct vertex *new_v,
					  struct bu_ptbl *faces,
					  int *free_edges,
					  const struct bn_tol *tol);
RT_EXPORT extern int nmg_simple_vertex_solve(struct vertex *new_v,
					     const struct bu_ptbl *faces,
					     const struct bn_tol *tol);
RT_EXPORT extern int nmg_ck_vert_on_fus(const struct vertex *v,
					const struct bn_tol *tol);
RT_EXPORT extern void nmg_make_faces_at_vert(struct vertex *new_v,
					     struct bu_ptbl *int_faces,
					     const struct bn_tol *tol);
RT_EXPORT extern void nmg_kill_cracks_at_vertex(const struct vertex *vp);
RT_EXPORT extern int nmg_complex_vertex_solve(struct vertex *new_v,
					      const struct bu_ptbl *faces,
					      const int free_edges,
					      const int approximate,
					      const struct bn_tol *tol);
RT_EXPORT extern int nmg_bad_face_normals(const struct shell *s,
					  const struct bn_tol *tol);
RT_EXPORT extern int nmg_faces_are_radial(const struct faceuse *fu1,
					  const struct faceuse *fu2);
RT_EXPORT extern int nmg_move_edge_thru_pt(struct edgeuse *mv_eu,
					   const point_t pt,
					   const struct bn_tol *tol);
RT_EXPORT extern void nmg_vlist_to_wire_edges(struct shell *s,
					      const struct bu_list *vhead);
RT_EXPORT extern void nmg_follow_free_edges_to_vertex(const struct vertex *vpa,
						      const struct vertex *vpb,
						      struct bu_ptbl *bad_verts,
						      const struct shell *s,
						      const struct edgeuse *eu,
						      struct bu_ptbl *verts,
						      int *found);
RT_EXPORT extern void nmg_glue_face_in_shell(const struct faceuse *fu,
					     struct shell *s,
					     const struct bn_tol *tol);
RT_EXPORT extern int nmg_open_shells_connect(struct shell *dst,
					     struct shell *src,
					     const long **copy_tbl,
					     const struct bn_tol *tol);
RT_EXPORT extern int nmg_in_vert(struct vertex *new_v,
				 const int approximate,
				 const struct bn_tol *tol);
RT_EXPORT extern void nmg_mirror_model(struct model *m);
RT_EXPORT extern int nmg_kill_cracks(struct shell *s);
RT_EXPORT extern int nmg_kill_zero_length_edgeuses(struct model *m);
RT_EXPORT extern void nmg_make_faces_within_tol(struct shell *s,
						const struct bn_tol *tol);
RT_EXPORT extern void nmg_intersect_loops_self(struct shell *s,
					       const struct bn_tol *tol);
RT_EXPORT extern struct edge_g_cnurb *rt_join_cnurbs(struct bu_list *crv_head);
RT_EXPORT extern struct edge_g_cnurb *rt_arc2d_to_cnurb(point_t i_center,
							point_t i_start,
							point_t i_end,
							int point_type,
							const struct bn_tol *tol);
RT_EXPORT extern int nmg_break_edge_at_verts(struct edge *e,
					     struct bu_ptbl *verts,
					     const struct bn_tol *tol);
RT_EXPORT extern void nmg_isect_shell_self(struct shell *s,
					   const struct bn_tol *tol);
RT_EXPORT extern fastf_t nmg_loop_plane_area(const struct loopuse *lu,
					     plane_t pl);
RT_EXPORT extern int nmg_break_edges(uint32_t *magic_p,
				     const struct bn_tol *tol);
RT_EXPORT extern int nmg_lu_is_convex(struct loopuse *lu,
				      const struct bn_tol *tol);
RT_EXPORT extern int nmg_to_arb(const struct model *m,
				struct rt_arb_internal *arb_int);
RT_EXPORT extern int nmg_to_tgc(const struct model *m,
				struct rt_tgc_internal *tgc_int,
				const struct bn_tol *tol);
RT_EXPORT extern int nmg_to_poly(const struct model *m,
				 struct rt_pg_internal *poly_int,
				 const struct bn_tol *tol);
RT_EXPORT extern struct rt_bot_internal *nmg_bot(struct shell *s,
						 const struct bn_tol *tol);

RT_EXPORT extern int nmg_simplify_shell_edges(struct shell *s,
					      const struct bn_tol *tol);
RT_EXPORT extern int nmg_edge_collapse(struct model *m,
				       const struct bn_tol *tol,
				       const fastf_t tol_coll,
				       const fastf_t min_angle);

/* From nmg_copy.c */
RT_EXPORT extern struct model *nmg_clone_model(const struct model *original);

/* bot.c */
RT_EXPORT extern size_t rt_bot_get_edge_list(const struct rt_bot_internal *bot,
					     size_t **edge_list);
RT_EXPORT extern int rt_bot_edge_in_list(const size_t v1,
					 const size_t v2,
					 const size_t edge_list[],
					 const size_t edge_count0);
RT_EXPORT extern int rt_bot_plot(struct bu_list		*vhead,
				 struct rt_db_internal	*ip,
				 const struct rt_tess_tol *ttol,
				 const struct bn_tol	*tol,
				 const struct rt_view_info *info);
RT_EXPORT extern int rt_bot_plot_poly(struct bu_list		*vhead,
				      struct rt_db_internal	*ip,
				      const struct rt_tess_tol *ttol,
				      const struct bn_tol	*tol);
RT_EXPORT extern int rt_bot_find_v_nearest_pt2(const struct rt_bot_internal *bot,
					       const point_t	pt2,
					       const mat_t	mat);
RT_EXPORT extern int rt_bot_find_e_nearest_pt2(int *vert1, int *vert2, const struct rt_bot_internal *bot, const point_t pt2, const mat_t mat);
RT_EXPORT extern fastf_t rt_bot_propget(struct rt_bot_internal *bot,
					const char *property);
RT_EXPORT extern int rt_bot_vertex_fuse(struct rt_bot_internal *bot,
					const struct bn_tol *tol);
RT_EXPORT extern int rt_bot_face_fuse(struct rt_bot_internal *bot);
RT_EXPORT extern int rt_bot_condense(struct rt_bot_internal *bot);
RT_EXPORT extern int rt_bot_smooth(struct rt_bot_internal *bot,
				   const char *bot_name,
				   struct db_i *dbip,
				   fastf_t normal_tolerance_angle);
RT_EXPORT extern int rt_bot_flip(struct rt_bot_internal *bot);
RT_EXPORT extern int rt_bot_sync(struct rt_bot_internal *bot);
RT_EXPORT extern struct rt_bot_list * rt_bot_split(struct rt_bot_internal *bot);
RT_EXPORT extern struct rt_bot_list * rt_bot_patches(struct rt_bot_internal *bot);
RT_EXPORT extern void rt_bot_list_free(struct rt_bot_list *headRblp,
				       int fbflag);

RT_EXPORT extern int rt_bot_same_orientation(const int *a,
					     const int *b);

RT_EXPORT extern int rt_bot_tess(struct nmgregion **r,
				 struct model *m,
				 struct rt_db_internal *ip,
				 const struct rt_tess_tol *ttol,
				 const struct bn_tol *tol);

/* BREP drawing routines */
RT_EXPORT extern int rt_brep_plot(struct bu_list		*vhead,
				 struct rt_db_internal		*ip,
				 const struct rt_tess_tol	*ttol,
				 const struct bn_tol		*tol,
				 const struct rt_view_info *info);
RT_EXPORT extern int rt_brep_plot_poly(struct bu_list		*vhead,
					  const struct db_full_path *pathp,
				      struct rt_db_internal	*ip,
				      const struct rt_tess_tol	*ttol,
				      const struct bn_tol	*tol,
				      const struct rt_view_info *info);
/* BREP validity test */
RT_EXPORT extern int rt_brep_valid(struct rt_db_internal *ip, struct bu_vls *log);

/* From nmg_tri.c */
RT_EXPORT extern void nmg_triangulate_shell(struct shell *s,
					    const struct bn_tol  *tol);


RT_EXPORT extern void nmg_triangulate_model(struct model *m,
					    const struct bn_tol *tol);
RT_EXPORT extern int nmg_triangulate_fu(struct faceuse *fu,
					const struct bn_tol *tol);
RT_EXPORT extern void nmg_dump_model(struct model *m);

/*  nmg_tri_mc.c */
RT_EXPORT extern void nmg_triangulate_model_mc(struct model *m,
					       const struct bn_tol *tol);
RT_EXPORT extern int nmg_mc_realize_cube(struct shell *s,
					 int pv,
					 point_t *edges,
					 const struct bn_tol *tol);
RT_EXPORT extern int nmg_mc_evaluate(struct shell *s,
				     struct rt_i *rtip,
				     const struct db_full_path *pathp,
				     const struct rt_tess_tol *ttol,
				     const struct bn_tol *tol);

/* nmg_manif.c */
RT_EXPORT extern int nmg_dangling_face(const struct faceuse *fu,
				       const char *manifolds);
/* static paint_face */
/* static set_edge_sub_manifold */
/* static set_loop_sub_manifold */
/* static set_face_sub_manifold */
RT_EXPORT extern char *nmg_shell_manifolds(struct shell *sp,
					   char *tbl);
RT_EXPORT extern char *nmg_manifolds(struct model *m);

/* nmg.c */
RT_EXPORT extern int nmg_ray_segs(struct ray_data	*rd);

/* torus.c */
RT_EXPORT extern int rt_num_circular_segments(double maxerr,
					      double radius);

/* tcl.c */
/** @addtogroup librt */
/** @{ */
/** @file librt/tcl.c
 *
 * Tcl interfaces to LIBRT routines.
 *
 * LIBRT routines are not for casual command-line use; as a result,
 * the Tcl name for the function should be exactly the same as the C
 * name for the underlying function.  Typing "rt_" is no hardship when
 * writing Tcl procs, and clarifies the origin of the routine.
 *
 */


/**
 * Allow specification of a ray to trace, in two convenient formats.
 *
 * Examples -
 * {0 0 0} dir {0 0 -1}
 * {20 -13.5 20} at {10 .5 3}
 */
RT_EXPORT extern int rt_tcl_parse_ray(Tcl_Interp *interp,
				      struct xray *rp,
				      const char *const*argv);


/**
 * Format a "union cutter" structure in a TCL-friendly format.  Useful
 * for debugging space partitioning
 *
 * Examples -
 * type cutnode
 * type boxnode
 * type nugridnode
 */
RT_EXPORT extern void rt_tcl_pr_cutter(Tcl_Interp *interp,
				       const union cutter *cutp);
RT_EXPORT extern int rt_tcl_cutter(ClientData clientData,
				   Tcl_Interp *interp,
				   int argc,
				   const char *const*argv);


/**
 * Format a hit in a TCL-friendly format.
 *
 * It is possible that a solid may have been removed from the
 * directory after this database was prepped, so check pointers
 * carefully.
 *
 * It might be beneficial to use some format other than %g to give the
 * user more precision.
 */
RT_EXPORT extern void rt_tcl_pr_hit(Tcl_Interp *interp, struct hit *hitp, const struct seg *segp, int flipflag);


/**
 * Generic interface for the LIBRT_class manipulation routines.
 *
 * Usage:
 * procname dbCmdName ?args?
 * Returns: result of cmdName LIBRT operation.
 *
 * Objects of type 'procname' must have been previously created by the
 * 'rt_gettrees' operation performed on a database object.
 *
 * Example -
 *	.inmem rt_gettrees .rt all.g
 *	.rt shootray {0 0 0} dir {0 0 -1}
 */
RT_EXPORT extern int rt_tcl_rt(ClientData clientData,
			       Tcl_Interp *interp,
			       int argc,
			       const char **argv);


/************************************************************************************************
 *												*
 *			Tcl interface to the Database						*
 *												*
 ************************************************************************************************/

/**
 * Given the name of a database object or a full path to a leaf
 * object, obtain the internal form of that leaf.  Packaged separately
 * mainly to make available nice Tcl error handling.
 *
 * Returns -
 * TCL_OK
 * TCL_ERROR
 */
RT_EXPORT extern int rt_tcl_import_from_path(Tcl_Interp *interp,
					     struct rt_db_internal *ip,
					     const char *path,
					     struct rt_wdb *wdb);
RT_EXPORT extern void rt_generic_make(const struct rt_functab *ftp, struct rt_db_internal *intern);


/**
 * Add all the supported Tcl interfaces to LIBRT routines to the list
 * of commands known by the given interpreter.
 *
 * wdb_open creates database "objects" which appear as Tcl procs,
 * which respond to many operations.
 *
 * The "db rt_gettrees" operation in turn creates a ray-traceable
 * object as yet another Tcl proc, which responds to additional
 * operations.
 *
 * Note that the MGED mainline C code automatically runs "wdb_open db"
 * and "wdb_open .inmem" on behalf of the MGED user, which exposes all
 * of this power.
 */
RT_EXPORT extern void rt_tcl_setup(Tcl_Interp *interp);
RT_EXPORT extern int Sysv_Init(Tcl_Interp *interp);


/**
 * Allows LIBRT to be dynamically loaded to a vanilla tclsh/wish with
 * "load /usr/brlcad/lib/libbu.so"
 * "load /usr/brlcad/lib/libbn.so"
 * "load /usr/brlcad/lib/librt.so"
 */
RT_EXPORT extern int Rt_Init(Tcl_Interp *interp);


/**
 * Take a db_full_path and append it to the TCL result string.
 *
 * NOT moving to db_fullpath.h because it is evil Tcl_Interp api
 */
RT_EXPORT extern void db_full_path_appendresult(Tcl_Interp *interp,
						const struct db_full_path *pp);


/**
 * Expects the Tcl_obj argument (list) to be a Tcl list and extracts
 * list elements, converts them to int, and stores them in the passed
 * in array. If the array_len argument is zero, a new array of
 * appropriate length is allocated. The return value is the number of
 * elements converted.
 */
RT_EXPORT extern int tcl_obj_to_int_array(Tcl_Interp *interp,
					  Tcl_Obj *list,
					  int **array,
					  int *array_len);


/**
 * Expects the Tcl_obj argument (list) to be a Tcl list and extracts
 * list elements, converts them to fastf_t, and stores them in the
 * passed in array. If the array_len argument is zero, a new array of
 * appropriate length is allocated. The return value is the number of
 * elements converted.
 */
RT_EXPORT extern int tcl_obj_to_fastf_array(Tcl_Interp *interp,
					    Tcl_Obj *list,
					    fastf_t **array,
					    int *array_len);


/**
 * interface to above tcl_obj_to_int_array() routine. This routine
 * expects a character string instead of a Tcl_Obj.
 *
 * Returns the number of elements converted.
 */
RT_EXPORT extern int tcl_list_to_int_array(Tcl_Interp *interp,
					   char *char_list,
					   int **array,
					   int *array_len);


/**
 * interface to above tcl_obj_to_fastf_array() routine. This routine
 * expects a character string instead of a Tcl_Obj.
 *
 * returns the number of elements converted.
 */
RT_EXPORT extern int tcl_list_to_fastf_array(Tcl_Interp *interp,
					     const char *char_list,
					     fastf_t **array,
					     int *array_len);

/** @} */
/* rhc.c */
RT_EXPORT extern int rt_mk_hyperbola(struct rt_pt_node *pts,
				     fastf_t r,
				     fastf_t b,
				     fastf_t c,
				     fastf_t dtol,
				     fastf_t ntol);


/* nmg_class.c */
RT_EXPORT extern int nmg_classify_pt_loop(const point_t pt,
					  const struct loopuse *lu,
					  const struct bn_tol *tol);

RT_EXPORT extern int nmg_classify_s_vs_s(struct shell *s,
					 struct shell *s2,
					 const struct bn_tol *tol);

RT_EXPORT extern int nmg_classify_lu_lu(const struct loopuse *lu1,
					const struct loopuse *lu2,
					const struct bn_tol *tol);

RT_EXPORT extern int nmg_class_pt_f(const point_t pt,
				    const struct faceuse *fu,
				    const struct bn_tol *tol);
RT_EXPORT extern int nmg_class_pt_s(const point_t pt,
				    const struct shell *s,
				    const int in_or_out_only,
				    const struct bn_tol *tol);

/* From nmg_pt_fu.c */
RT_EXPORT extern int nmg_eu_is_part_of_crack(const struct edgeuse *eu);

RT_EXPORT extern int nmg_class_pt_lu_except(point_t		pt,
					    const struct loopuse	*lu,
					    const struct edge		*e_p,
					    const struct bn_tol	*tol);

RT_EXPORT extern int nmg_class_pt_fu_except(const point_t pt,
					    const struct faceuse *fu,
					    const struct loopuse *ignore_lu,
					    void (*eu_func)(struct edgeuse *, point_t, const char *),
					    void (*vu_func)(struct vertexuse *, point_t, const char *),
					    const char *priv,
					    const int call_on_hits,
					    const int in_or_out_only,
					    const struct bn_tol *tol);

/* From nmg_plot.c */
RT_EXPORT extern void nmg_pl_shell(FILE *fp,
				   const struct shell *s,
				   int fancy);

RT_EXPORT extern void nmg_vu_to_vlist(struct bu_list *vhead,
				      const struct vertexuse	*vu);
RT_EXPORT extern void nmg_eu_to_vlist(struct bu_list *vhead,
				      const struct bu_list	*eu_hd);
RT_EXPORT extern void nmg_lu_to_vlist(struct bu_list *vhead,
				      const struct loopuse	*lu,
				      int			poly_markers,
				      const vectp_t		norm);
RT_EXPORT extern void nmg_snurb_fu_to_vlist(struct bu_list		*vhead,
					    const struct faceuse	*fu,
					    int			poly_markers);
RT_EXPORT extern void nmg_s_to_vlist(struct bu_list		*vhead,
				     const struct shell	*s,
				     int			poly_markers);
RT_EXPORT extern void nmg_r_to_vlist(struct bu_list		*vhead,
				     const struct nmgregion	*r,
				     int			poly_markers);
RT_EXPORT extern void nmg_m_to_vlist(struct bu_list	*vhead,
				     struct model	*m,
				     int		poly_markers);
RT_EXPORT extern void nmg_offset_eu_vert(point_t			base,
					 const struct edgeuse	*eu,
					 const vect_t		face_normal,
					 int			tip);
/* plot */
RT_EXPORT extern void nmg_pl_v(FILE	*fp,
			       const struct vertex *v,
			       long *b);
RT_EXPORT extern void nmg_pl_e(FILE *fp,
			       const struct edge *e,
			       long *b,
			       int red,
			       int green,
			       int blue);
RT_EXPORT extern void nmg_pl_eu(FILE *fp,
				const struct edgeuse *eu,
				long *b,
				int red,
				int green,
				int blue);
RT_EXPORT extern void nmg_pl_lu(FILE *fp,
				const struct loopuse *fu,
				long *b,
				int red,
				int green,
				int blue);
RT_EXPORT extern void nmg_pl_fu(FILE *fp,
				const struct faceuse *fu,
				long *b,
				int red,
				int green,
				int blue);
RT_EXPORT extern void nmg_pl_s(FILE *fp,
			       const struct shell *s);
RT_EXPORT extern void nmg_pl_r(FILE *fp,
			       const struct nmgregion *r);
RT_EXPORT extern void nmg_pl_m(FILE *fp,
			       const struct model *m);
RT_EXPORT extern void nmg_vlblock_v(struct bn_vlblock *vbp,
				    const struct vertex *v,
				    long *tab);
RT_EXPORT extern void nmg_vlblock_e(struct bn_vlblock *vbp,
				    const struct edge *e,
				    long *tab,
				    int red,
				    int green,
				    int blue);
RT_EXPORT extern void nmg_vlblock_eu(struct bn_vlblock *vbp,
				     const struct edgeuse *eu,
				     long *tab,
				     int red,
				     int green,
				     int blue,
				     int fancy);
RT_EXPORT extern void nmg_vlblock_euleft(struct bu_list			*vh,
					 const struct edgeuse		*eu,
					 const point_t			center,
					 const mat_t			mat,
					 const vect_t			xvec,
					 const vect_t			yvec,
					 double				len,
					 const struct bn_tol		*tol);
RT_EXPORT extern void nmg_vlblock_around_eu(struct bn_vlblock		*vbp,
					    const struct edgeuse	*arg_eu,
					    long			*tab,
					    int			fancy,
					    const struct bn_tol	*tol);
RT_EXPORT extern void nmg_vlblock_lu(struct bn_vlblock *vbp,
				     const struct loopuse *lu,
				     long *tab,
				     int red,
				     int green,
				     int blue,
				     int fancy);
RT_EXPORT extern void nmg_vlblock_fu(struct bn_vlblock *vbp,
				     const struct faceuse *fu,
				     long *tab, int fancy);
RT_EXPORT extern void nmg_vlblock_s(struct bn_vlblock *vbp,
				    const struct shell *s,
				    int fancy);
RT_EXPORT extern void nmg_vlblock_r(struct bn_vlblock *vbp,
				    const struct nmgregion *r,
				    int fancy);
RT_EXPORT extern void nmg_vlblock_m(struct bn_vlblock *vbp,
				    const struct model *m,
				    int fancy);
/* visualization helper routines */
RT_EXPORT extern void nmg_pl_edges_in_2_shells(struct bn_vlblock	*vbp,
					       long			*b,
					       const struct edgeuse	*eu,
					       int			fancy,
					       const struct bn_tol	*tol);
RT_EXPORT extern void nmg_pl_isect(const char		*filename,
				   const struct shell	*s,
				   const struct bn_tol	*tol);
RT_EXPORT extern void nmg_pl_comb_fu(int num1,
				     int num2,
				     const struct faceuse *fu1);
RT_EXPORT extern void nmg_pl_2fu(const char *str,
				 const struct faceuse *fu1,
				 const struct faceuse *fu2,
				 int show_mates);
/* graphical display of classifier results */
RT_EXPORT extern void nmg_show_broken_classifier_stuff(uint32_t	*p,
						       char	**classlist,
						       int	all_new,
						       int	fancy,
						       const char	*a_string);
RT_EXPORT extern void nmg_face_plot(const struct faceuse *fu);
RT_EXPORT extern void nmg_2face_plot(const struct faceuse *fu1,
				     const struct faceuse *fu2);
RT_EXPORT extern void nmg_face_lu_plot(const struct loopuse *lu,
				       const struct vertexuse *vu1,
				       const struct vertexuse *vu2);
RT_EXPORT extern void nmg_plot_lu_ray(const struct loopuse		*lu,
				      const struct vertexuse		*vu1,
				      const struct vertexuse		*vu2,
				      const vect_t			left);
RT_EXPORT extern void nmg_plot_ray_face(const char *fname,
					point_t pt,
					const vect_t dir,
					const struct faceuse *fu);
RT_EXPORT extern void nmg_plot_lu_around_eu(const char		*prefix,
					    const struct edgeuse	*eu,
					    const struct bn_tol	*tol);
RT_EXPORT extern int nmg_snurb_to_vlist(struct bu_list		*vhead,
					const struct face_g_snurb	*fg,
					int			n_interior);
RT_EXPORT extern void nmg_cnurb_to_vlist(struct bu_list *vhead,
					 const struct edgeuse *eu,
					 int n_interior,
					 int cmd);

/**
 * global nmg animation plot callback
 */
RT_EXPORT extern void (*nmg_plot_anim_upcall)(void);

/**
 * global nmg animation vblock callback
 */
RT_EXPORT extern void (*nmg_vlblock_anim_upcall)(void);

/**
 * global nmg mged display debug callback
 */
RT_EXPORT extern void (*nmg_mged_debug_display_hack)(void);

/**
 * edge use distance tolerance
 */
RT_EXPORT extern double nmg_eue_dist;


/* from nmg_mesh.c */
RT_EXPORT extern int nmg_mesh_two_faces(struct faceuse *fu1,
					struct faceuse *fu2,
					const struct bn_tol	*tol);

RT_EXPORT extern void nmg_radial_join_eu(struct edgeuse *eu1,
					 struct edgeuse *eu2,
					 const struct bn_tol *tol);
RT_EXPORT extern void nmg_mesh_faces(struct faceuse *fu1,
				     struct faceuse *fu2,
				     const struct bn_tol *tol);
RT_EXPORT extern int nmg_mesh_face_shell(struct faceuse *fu1,
					 struct shell *s,
					 const struct bn_tol *tol);
RT_EXPORT extern int nmg_mesh_shell_shell(struct shell *s1,
					  struct shell *s2,
					  const struct bn_tol *tol);
RT_EXPORT extern double nmg_measure_fu_angle(const struct edgeuse *eu,
					     const vect_t xvec,
					     const vect_t yvec,
					     const vect_t zvec);

/* from nmg_bool.c */
RT_EXPORT extern struct nmgregion *nmg_do_bool(struct nmgregion *s1,
					       struct nmgregion *s2,
					       const int oper, const struct bn_tol *tol);
RT_EXPORT extern void nmg_shell_coplanar_face_merge(struct shell *s,
						    const struct bn_tol *tol,
						    const int simplify);
RT_EXPORT extern int nmg_two_region_vertex_fuse(struct nmgregion *r1,
						struct nmgregion *r2,
						const struct bn_tol *tol);
RT_EXPORT extern union tree *nmg_booltree_leaf_tess(struct db_tree_state *tsp,
						    const struct db_full_path *pathp,
						    struct rt_db_internal *ip,
						    void *client_data);
RT_EXPORT extern union tree *nmg_booltree_leaf_tnurb(struct db_tree_state *tsp,
						     const struct db_full_path *pathp,
						     struct rt_db_internal *ip,
						     void *client_data);
RT_EXPORT extern int nmg_bool_eval_silent;	/* quell output from nmg_booltree_evaluate */
RT_EXPORT extern union tree *nmg_booltree_evaluate(union tree *tp,
						   const struct bn_tol *tol,
						   struct resource *resp);
RT_EXPORT extern int nmg_boolean(union tree *tp,
				 struct model *m,
				 const struct bn_tol *tol,
				 struct resource *resp);

/* from nmg_class.c */
RT_EXPORT extern void nmg_class_shells(struct shell *sA,
				       struct shell *sB,
				       char **classlist,
				       const struct bn_tol *tol);

/* from nmg_fcut.c */
/* static void ptbl_vsort */
RT_EXPORT extern int nmg_ck_vu_ptbl(struct bu_ptbl	*p,
				    struct faceuse	*fu);
RT_EXPORT extern double nmg_vu_angle_measure(struct vertexuse	*vu,
					     vect_t x_dir,
					     vect_t y_dir,
					     int assessment,
					     int in);
RT_EXPORT extern int nmg_wedge_class(int	ass,	/* assessment of two edges forming wedge */
				     double	a,
				     double	b);
RT_EXPORT extern void nmg_sanitize_fu(struct faceuse	*fu);
RT_EXPORT extern void nmg_unlist_v(struct bu_ptbl	*b,
				   fastf_t *mag,
				   struct vertex	*v);
RT_EXPORT extern struct edge_g_lseg *nmg_face_cutjoin(struct bu_ptbl *b1,
						      struct bu_ptbl *b2,
						      fastf_t *mag1,
						      fastf_t *mag2,
						      struct faceuse *fu1,
						      struct faceuse *fu2,
						      point_t pt,
						      vect_t dir,
						      struct edge_g_lseg *eg,
						      const struct bn_tol *tol);
RT_EXPORT extern void nmg_fcut_face_2d(struct bu_ptbl *vu_list,
				       fastf_t *mag,
				       struct faceuse *fu1,
				       struct faceuse *fu2,
				       struct bn_tol *tol);
RT_EXPORT extern int nmg_insert_vu_if_on_edge(struct vertexuse *vu1,
					      struct vertexuse *vu2,
					      struct edgeuse *new_eu,
					      struct bn_tol *tol);
/* nmg_face_state_transition */

#define nmg_mev(_v, _u) nmg_me((_v), (struct vertex *)NULL, (_u))

/* From nmg_eval.c */
RT_EXPORT extern void nmg_ck_lu_orientation(struct loopuse *lu,
					    const struct bn_tol *tolp);
RT_EXPORT extern const char *nmg_class_name(int class_no);
RT_EXPORT extern void nmg_evaluate_boolean(struct shell	*sA,
					   struct shell	*sB,
					   int		op,
					   char		**classlist,
					   const struct bn_tol	*tol);

/* The following functions cannot be publicly declared because struct
 * nmg_bool_state is private to nmg_eval.c
 */
/* nmg_eval_shell */
/* nmg_eval_action */
/* nmg_eval_plot */


/* From nmg_rt_isect.c */
RT_EXPORT extern void nmg_rt_print_hitlist(struct bu_list *hd);

RT_EXPORT extern void nmg_rt_print_hitmiss(struct hitmiss *a_hit);

RT_EXPORT extern int nmg_class_ray_vs_shell(struct xray *rp,
					    const struct shell *s,
					    const int in_or_out_only,
					    const struct bn_tol *tol);

RT_EXPORT extern void nmg_isect_ray_model(struct ray_data *rd);

/* From nmg_ck.c */
RT_EXPORT extern void nmg_vvg(const struct vertex_g *vg);
RT_EXPORT extern void nmg_vvertex(const struct vertex *v,
				  const struct vertexuse *vup);
RT_EXPORT extern void nmg_vvua(const uint32_t *vua);
RT_EXPORT extern void nmg_vvu(const struct vertexuse *vu,
			      const uint32_t *up_magic_p);
RT_EXPORT extern void nmg_veg(const uint32_t *eg);
RT_EXPORT extern void nmg_vedge(const struct edge *e,
				const struct edgeuse *eup);
RT_EXPORT extern void nmg_veu(const struct bu_list	*hp,
			      const uint32_t *up_magic_p);
RT_EXPORT extern void nmg_vlg(const struct loop_g *lg);
RT_EXPORT extern void nmg_vloop(const struct loop *l,
				const struct loopuse *lup);
RT_EXPORT extern void nmg_vlu(const struct bu_list	*hp,
			      const uint32_t *up);
RT_EXPORT extern void nmg_vfg(const struct face_g_plane *fg);
RT_EXPORT extern void nmg_vface(const struct face *f,
				const struct faceuse *fup);
RT_EXPORT extern void nmg_vfu(const struct bu_list	*hp,
			      const struct shell *s);
RT_EXPORT extern void nmg_vsshell(const struct shell *s,
				 const struct nmgregion *r);
RT_EXPORT extern void nmg_vshell(const struct bu_list *hp,
				 const struct nmgregion *r);
RT_EXPORT extern void nmg_vregion(const struct bu_list *hp,
				  const struct model *m);
RT_EXPORT extern void nmg_vmodel(const struct model *m);

/* checking routines */
RT_EXPORT extern void nmg_ck_e(const struct edgeuse *eu,
			       const struct edge *e,
			       const char *str);
RT_EXPORT extern void nmg_ck_vu(const uint32_t *parent,
				const struct vertexuse *vu,
				const char *str);
RT_EXPORT extern void nmg_ck_eu(const uint32_t *parent,
				const struct edgeuse *eu,
				const char *str);
RT_EXPORT extern void nmg_ck_lg(const struct loop *l,
				const struct loop_g *lg,
				const char *str);
RT_EXPORT extern void nmg_ck_l(const struct loopuse *lu,
			       const struct loop *l,
			       const char *str);
RT_EXPORT extern void nmg_ck_lu(const uint32_t *parent,
				const struct loopuse *lu,
				const char *str);
RT_EXPORT extern void nmg_ck_fg(const struct face *f,
				const struct face_g_plane *fg,
				const char *str);
RT_EXPORT extern void nmg_ck_f(const struct faceuse *fu,
			       const struct face *f,
			       const char *str);
RT_EXPORT extern void nmg_ck_fu(const struct shell *s,
				const struct faceuse *fu,
				const char *str);
RT_EXPORT extern int nmg_ck_eg_verts(const struct edge_g_lseg *eg,
				     const struct bn_tol *tol);
RT_EXPORT extern int nmg_ck_geometry(const struct model *m,
				     const struct bn_tol *tol);
RT_EXPORT extern int nmg_ck_face_worthless_edges(const struct faceuse *fu);
RT_EXPORT extern void nmg_ck_lueu(const struct loopuse *lu, const char *s);
RT_EXPORT extern int nmg_check_radial(const struct edgeuse *eu, const struct bn_tol *tol);
RT_EXPORT extern int nmg_eu_2s_orient_bad(const struct edgeuse	*eu,
					  const struct shell	*s1,
					  const struct shell	*s2,
					  const struct bn_tol	*tol);
RT_EXPORT extern int nmg_ck_closed_surf(const struct shell *s,
					const struct bn_tol *tol);
RT_EXPORT extern int nmg_ck_closed_region(const struct nmgregion *r,
					  const struct bn_tol *tol);
RT_EXPORT extern void nmg_ck_v_in_2fus(const struct vertex *vp,
				       const struct faceuse *fu1,
				       const struct faceuse *fu2,
				       const struct bn_tol *tol);
RT_EXPORT extern void nmg_ck_vs_in_region(const struct nmgregion *r,
					  const struct bn_tol *tol);


/* From nmg_inter.c */
RT_EXPORT extern struct vertexuse *nmg_make_dualvu(struct vertex *v,
						   struct faceuse *fu,
						   const struct bn_tol *tol);
RT_EXPORT extern struct vertexuse *nmg_enlist_vu(struct nmg_inter_struct	*is,
						 const struct vertexuse *vu,
						 struct vertexuse *dualvu,
						 fastf_t dist);
RT_EXPORT extern void nmg_isect2d_prep(struct nmg_inter_struct *is,
				       const uint32_t *assoc_use);
RT_EXPORT extern void nmg_isect2d_cleanup(struct nmg_inter_struct *is);
RT_EXPORT extern void nmg_isect2d_final_cleanup(void);
RT_EXPORT extern int nmg_isect_2faceuse(point_t pt,
					vect_t dir,
					struct faceuse *fu1,
					struct faceuse *fu2,
					const struct bn_tol *tol);
RT_EXPORT extern void nmg_isect_vert2p_face2p(struct nmg_inter_struct *is,
					      struct vertexuse *vu1,
					      struct faceuse *fu2);
RT_EXPORT extern struct edgeuse *nmg_break_eu_on_v(struct edgeuse *eu1,
						   struct vertex *v2,
						   struct faceuse *fu,
						   struct nmg_inter_struct *is);
RT_EXPORT extern void nmg_break_eg_on_v(const struct edge_g_lseg	*eg,
					struct vertex		*v,
					const struct bn_tol	*tol);
RT_EXPORT extern int nmg_isect_2colinear_edge2p(struct edgeuse	*eu1,
						struct edgeuse	*eu2,
						struct faceuse		*fu,
						struct nmg_inter_struct	*is,
						struct bu_ptbl		*l1,
						struct bu_ptbl		*l2);
RT_EXPORT extern int nmg_isect_edge2p_edge2p(struct nmg_inter_struct	*is,
					     struct edgeuse		*eu1,
					     struct edgeuse		*eu2,
					     struct faceuse		*fu1,
					     struct faceuse		*fu2);
RT_EXPORT extern int nmg_isect_construct_nice_ray(struct nmg_inter_struct	*is,
						  struct faceuse		*fu2);
RT_EXPORT extern void nmg_enlist_one_vu(struct nmg_inter_struct	*is,
					const struct vertexuse	*vu,
					fastf_t			dist);
RT_EXPORT extern int	nmg_isect_line2_edge2p(struct nmg_inter_struct	*is,
					       struct bu_ptbl		*list,
					       struct edgeuse		*eu1,
					       struct faceuse		*fu1,
					       struct faceuse		*fu2);
RT_EXPORT extern void nmg_isect_line2_vertex2(struct nmg_inter_struct	*is,
					      struct vertexuse	*vu1,
					      struct faceuse		*fu1);
RT_EXPORT extern int nmg_isect_two_ptbls(struct nmg_inter_struct	*is,
					 const struct bu_ptbl	*t1,
					 const struct bu_ptbl	*t2);
RT_EXPORT extern struct edge_g_lseg	*nmg_find_eg_on_line(const uint32_t *magic_p,
							     const point_t		pt,
							     const vect_t		dir,
							     const struct bn_tol	*tol);
RT_EXPORT extern int nmg_k0eu(struct vertex	*v);
RT_EXPORT extern struct vertex *nmg_repair_v_near_v(struct vertex		*hit_v,
						    struct vertex		*v,
						    const struct edge_g_lseg	*eg1,
						    const struct edge_g_lseg	*eg2,
						    int			bomb,
						    const struct bn_tol	*tol);
RT_EXPORT extern struct vertex *nmg_search_v_eg(const struct edgeuse	*eu,
						int			second,
						const struct edge_g_lseg	*eg1,
						const struct edge_g_lseg	*eg2,
						struct vertex		*hit_v,
						const struct bn_tol	*tol);
RT_EXPORT extern struct vertex *nmg_common_v_2eg(struct edge_g_lseg	*eg1,
						 struct edge_g_lseg	*eg2,
						 const struct bn_tol	*tol);
RT_EXPORT extern int nmg_is_vertex_on_inter(struct vertex *v,
					    struct faceuse *fu1,
					    struct faceuse *fu2,
					    struct nmg_inter_struct *is);
RT_EXPORT extern void nmg_isect_eu_verts(struct edgeuse *eu,
					 struct vertex_g *vg1,
					 struct vertex_g *vg2,
					 struct bu_ptbl *verts,
					 struct bu_ptbl *inters,
					 const struct bn_tol *tol);
RT_EXPORT extern void nmg_isect_eu_eu(struct edgeuse *eu1,
				      struct vertex_g *vg1a,
				      struct vertex_g *vg1b,
				      vect_t dir1,
				      struct edgeuse *eu2,
				      struct bu_ptbl *verts,
				      struct bu_ptbl *inters,
				      const struct bn_tol *tol);
RT_EXPORT extern void nmg_isect_eu_fu(struct nmg_inter_struct *is,
				      struct bu_ptbl		*verts,
				      struct edgeuse		*eu,
				      struct faceuse          *fu);
RT_EXPORT extern void nmg_isect_fu_jra(struct nmg_inter_struct	*is,
				       struct faceuse		*fu1,
				       struct faceuse		*fu2,
				       struct bu_ptbl		*eu1_list,
				       struct bu_ptbl		*eu2_list);
RT_EXPORT extern void nmg_isect_line2_face2pNEW(struct nmg_inter_struct *is,
						struct faceuse *fu1, struct faceuse *fu2,
						struct bu_ptbl *eu1_list,
						struct bu_ptbl *eu2_list);
RT_EXPORT extern int	nmg_is_eu_on_line3(const struct edgeuse	*eu,
					   const point_t		pt,
					   const vect_t		dir,
					   const struct bn_tol	*tol);
RT_EXPORT extern struct edge_g_lseg	*nmg_find_eg_between_2fg(const struct faceuse	*ofu1,
								 const struct faceuse	*fu2,
								 const struct bn_tol	*tol);
RT_EXPORT extern struct edgeuse *nmg_does_fu_use_eg(const struct faceuse	*fu1,
						    const uint32_t	*eg);
RT_EXPORT extern int rt_line_on_plane(const point_t	pt,
				      const vect_t	dir,
				      const plane_t	plane,
				      const struct bn_tol	*tol);
RT_EXPORT extern void nmg_cut_lu_into_coplanar_and_non(struct loopuse *lu,
						       plane_t pl,
						       struct nmg_inter_struct *is);
RT_EXPORT extern void nmg_check_radial_angles(char *str,
					      struct shell *s,
					      const struct bn_tol *tol);
RT_EXPORT extern int nmg_faces_can_be_intersected(struct nmg_inter_struct *bs,
						  const struct faceuse *fu1,
						  const struct faceuse *fu2,
						  const struct bn_tol *tol);
RT_EXPORT extern void nmg_isect_two_generic_faces(struct faceuse		*fu1,
						  struct faceuse		*fu2,
						  const struct bn_tol	*tol);
RT_EXPORT extern void nmg_crackshells(struct shell *s1,
				      struct shell *s2,
				      const struct bn_tol *tol);
RT_EXPORT extern int nmg_fu_touchingloops(const struct faceuse *fu);


/* From nmg_index.c */
RT_EXPORT extern int nmg_index_of_struct(const uint32_t *p);
RT_EXPORT extern void nmg_m_set_high_bit(struct model *m);
RT_EXPORT extern void nmg_m_reindex(struct model *m, long newindex);
RT_EXPORT extern void nmg_vls_struct_counts(struct bu_vls *str,
					    const struct nmg_struct_counts *ctr);
RT_EXPORT extern void nmg_pr_struct_counts(const struct nmg_struct_counts *ctr,
					   const char *str);
RT_EXPORT extern uint32_t **nmg_m_struct_count(struct nmg_struct_counts *ctr,
						    const struct model *m);
RT_EXPORT extern void nmg_pr_m_struct_counts(const struct model	*m,
					     const char		*str);
RT_EXPORT extern void nmg_merge_models(struct model *m1,
				       struct model *m2);
RT_EXPORT extern long nmg_find_max_index(const struct model *m);

/* From nmg_rt.c */

/* From dspline.c */

/**
 * Initialize a spline matrix for a particular spline type.
 */
RT_EXPORT extern void rt_dspline_matrix(mat_t m, const char *type,
					const double	tension,
					const double	bias);

/**
 * m		spline matrix (see rt_dspline4_matrix)
 * a, b, c, d	knot values
 * alpha:	0..1	interpolation point
 *
 * Evaluate a 1-dimensional spline at a point "alpha" on knot values
 * a, b, c, d.
 */
RT_EXPORT extern double rt_dspline4(mat_t m,
				    double a,
				    double b,
				    double c,
				    double d,
				    double alpha);

/**
 * pt		vector to receive the interpolation result
 * m		spline matrix (see rt_dspline4_matrix)
 * a, b, c, d	knot values
 * alpha:	0..1 interpolation point
 *
 * Evaluate a spline at a point "alpha" between knot pts b & c. The
 * knots and result are all vectors with "depth" values (length).
 *
 */
RT_EXPORT extern void rt_dspline4v(double *pt,
				   const mat_t m,
				   const double *a,
				   const double *b,
				   const double *c,
				   const double *d,
				   const int depth,
				   const double alpha);

/**
 * Interpolate n knot vectors over the range 0..1
 *
 * "knots" is an array of "n" knot vectors.  Each vector consists of
 * "depth" values.  They define an "n" dimensional surface which is
 * evaluated at the single point "alpha".  The evaluated point is
 * returned in "r"
 *
 * Example use:
 *
 * double result[MAX_DEPTH], knots[MAX_DEPTH*MAX_KNOTS];
 * mat_t m;
 * int knot_count, depth;
 *
 * knots = bu_malloc(sizeof(double) * knot_length * knot_count);
 * result = bu_malloc(sizeof(double) * knot_length);
 *
 * rt_dspline4_matrix(m, "Catmull", (double *)NULL, 0.0);
 *
 * for (i=0; i < knot_count; i++)
 *   get a knot(knots, i, knot_length);
 *
 * rt_dspline_n(result, m, knots, knot_count, knot_length, alpha);
 *
 */
RT_EXPORT extern void rt_dspline_n(double *r,
				   const mat_t m,
				   const double *knots,
				   const int n,
				   const int depth,
				   const double alpha);

/* From nmg_fuse.c */
RT_EXPORT extern int nmg_is_common_bigloop(const struct face *f1,
					   const struct face *f2);
RT_EXPORT extern void nmg_region_v_unique(struct nmgregion *r1,
					  const struct bn_tol *tol);
RT_EXPORT extern int nmg_ptbl_vfuse(struct bu_ptbl *t,
				    const struct bn_tol *tol);
RT_EXPORT extern int	nmg_region_both_vfuse(struct bu_ptbl *t1,
					      struct bu_ptbl *t2,
					      const struct bn_tol *tol);
/* nmg_two_region_vertex_fuse replaced with nmg_vertex_fuse */
RT_EXPORT extern int nmg_vertex_fuse(const uint32_t *magic_p,
				     const struct bn_tol *tol);
RT_EXPORT extern int nmg_cnurb_is_linear(const struct edge_g_cnurb *cnrb);
RT_EXPORT extern int nmg_snurb_is_planar(const struct face_g_snurb *srf,
					 const struct bn_tol *tol);
RT_EXPORT extern void nmg_eval_linear_trim_curve(const struct face_g_snurb *snrb,
						 const fastf_t uvw[3],
						 point_t xyz);
RT_EXPORT extern void nmg_eval_trim_curve(const struct edge_g_cnurb *cnrb,
					  const struct face_g_snurb *snrb,
					  const fastf_t t, point_t xyz);
/* nmg_split_trim */
RT_EXPORT extern void nmg_eval_trim_to_tol(const struct edge_g_cnurb *cnrb,
					   const struct face_g_snurb *snrb,
					   const fastf_t t0,
					   const fastf_t t1,
					   struct bu_list *head,
					   const struct bn_tol *tol);
/* nmg_split_linear_trim */
RT_EXPORT extern void nmg_eval_linear_trim_to_tol(const struct edge_g_cnurb *cnrb,
						  const struct face_g_snurb *snrb,
						  const fastf_t uvw1[3],
						  const fastf_t uvw2[3],
						  struct bu_list *head,
						  const struct bn_tol *tol);
RT_EXPORT extern int nmg_cnurb_lseg_coincident(const struct edgeuse *eu1,
					       const struct edge_g_cnurb *cnrb,
					       const struct face_g_snurb *snrb,
					       const point_t pt1,
					       const point_t pt2,
					       const struct bn_tol *tol);
RT_EXPORT extern int	nmg_cnurb_is_on_crv(const struct edgeuse *eu,
					    const struct edge_g_cnurb *cnrb,
					    const struct face_g_snurb *snrb,
					    const struct bu_list *head,
					    const struct bn_tol *tol);
RT_EXPORT extern int nmg_edge_fuse(const uint32_t *magic_p,
				   const struct bn_tol *tol);
RT_EXPORT extern int nmg_edge_g_fuse(const uint32_t *magic_p,
					   const struct bn_tol	*tol);
RT_EXPORT extern int nmg_ck_fu_verts(struct faceuse *fu1,
				     struct face *f2,
				     const struct bn_tol *tol);
RT_EXPORT extern int nmg_ck_fg_verts(struct faceuse *fu1,
				     struct face *f2,
				     const struct bn_tol *tol);
RT_EXPORT extern int	nmg_two_face_fuse(struct face	*f1,
					  struct face *f2,
					  const struct bn_tol *tol);
RT_EXPORT extern int nmg_model_face_fuse(struct model *m,
					 const struct bn_tol *tol);
RT_EXPORT extern int nmg_break_all_es_on_v(uint32_t *magic_p,
					   struct vertex *v,
					   const struct bn_tol *tol);
RT_EXPORT extern int nmg_break_e_on_v(const uint32_t *magic_p,
				      const struct bn_tol *tol);
/* DEPRECATED: use nmg_break_e_on_v */
RT_EXPORT extern int nmg_model_break_e_on_v(const uint32_t *magic_p,
					    const struct bn_tol *tol);
RT_EXPORT extern int nmg_model_fuse(struct model *m,
				    const struct bn_tol *tol);

/* radial routines */
RT_EXPORT extern void nmg_radial_sorted_list_insert(struct bu_list *hd,
						    struct nmg_radial *rad);
RT_EXPORT extern void nmg_radial_verify_pointers(const struct bu_list *hd,
						 const struct bn_tol *tol);
RT_EXPORT extern void nmg_radial_verify_monotone(const struct bu_list	*hd,
						 const struct bn_tol	*tol);
RT_EXPORT extern void nmg_insure_radial_list_is_increasing(struct bu_list	*hd,
							   fastf_t amin, fastf_t amax);
RT_EXPORT extern void nmg_radial_build_list(struct bu_list		*hd,
					    struct bu_ptbl		*shell_tbl,
					    int			existing,
					    struct edgeuse		*eu,
					    const vect_t		xvec,
					    const vect_t		yvec,
					    const vect_t		zvec,
					    const struct bn_tol	*tol);
RT_EXPORT extern void nmg_radial_merge_lists(struct bu_list		*dest,
					     struct bu_list		*src,
					     const struct bn_tol	*tol);
RT_EXPORT extern int	 nmg_is_crack_outie(const struct edgeuse	*eu,
					    const struct bn_tol	*tol);
RT_EXPORT extern struct nmg_radial	*nmg_find_radial_eu(const struct bu_list *hd,
							    const struct edgeuse *eu);
RT_EXPORT extern const struct edgeuse *nmg_find_next_use_of_2e_in_lu(const struct edgeuse	*eu,
								     const struct edge	*e1,
								     const struct edge	*e2);
RT_EXPORT extern void nmg_radial_mark_cracks(struct bu_list	*hd,
					     const struct edge	*e1,
					     const struct edge	*e2,
					     const struct bn_tol	*tol);
RT_EXPORT extern struct nmg_radial *nmg_radial_find_an_original(const struct bu_list	*hd,
								const struct shell	*s,
								const struct bn_tol	*tol);
RT_EXPORT extern int nmg_radial_mark_flips(struct bu_list		*hd,
					   const struct shell	*s,
					   const struct bn_tol	*tol);
RT_EXPORT extern int nmg_radial_check_parity(const struct bu_list	*hd,
					     const struct bu_ptbl	*shells,
					     const struct bn_tol	*tol);
RT_EXPORT extern void nmg_radial_implement_decisions(struct bu_list		*hd,
						     const struct bn_tol	*tol,
						     struct edgeuse		*eu1,
						     vect_t xvec,
						     vect_t yvec,
						     vect_t zvec);
RT_EXPORT extern void nmg_pr_radial(const char *title,
				    const struct nmg_radial	*rad);
RT_EXPORT extern void nmg_pr_radial_list(const struct bu_list *hd,
					 const struct bn_tol *tol);
RT_EXPORT extern void nmg_do_radial_flips(struct bu_list *hd);
RT_EXPORT extern void nmg_do_radial_join(struct bu_list *hd,
					 struct edgeuse *eu1ref,
					 vect_t xvec, vect_t yvec, vect_t zvec,
					 const struct bn_tol *tol);
RT_EXPORT extern void nmg_radial_join_eu_NEW(struct edgeuse *eu1,
					     struct edgeuse *eu2,
					     const struct bn_tol *tol);
RT_EXPORT extern void nmg_radial_exchange_marked(struct bu_list		*hd,
						 const struct bn_tol	*tol);
RT_EXPORT extern void nmg_s_radial_harmonize(struct shell		*s,
					     const struct bn_tol	*tol);
RT_EXPORT extern void nmg_s_radial_check(struct shell		*s,
					 const struct bn_tol	*tol);
RT_EXPORT extern void nmg_r_radial_check(const struct nmgregion	*r,
					 const struct bn_tol	*tol);


RT_EXPORT extern struct edge_g_lseg	*nmg_pick_best_edge_g(struct edgeuse *eu1,
							      struct edgeuse *eu2,
							      const struct bn_tol *tol);

/* nmg_visit.c */
RT_EXPORT extern void nmg_visit_vertex(struct vertex			*v,
				       const struct nmg_visit_handlers	*htab,
				       void *			state);
RT_EXPORT extern void nmg_visit_vertexuse(struct vertexuse		*vu,
					  const struct nmg_visit_handlers	*htab,
					  void *			state);
RT_EXPORT extern void nmg_visit_edge(struct edge			*e,
				     const struct nmg_visit_handlers	*htab,
				     void *			state);
RT_EXPORT extern void nmg_visit_edgeuse(struct edgeuse			*eu,
					const struct nmg_visit_handlers	*htab,
					void *			state);
RT_EXPORT extern void nmg_visit_loop(struct loop			*l,
				     const struct nmg_visit_handlers	*htab,
				     void *			state);
RT_EXPORT extern void nmg_visit_loopuse(struct loopuse			*lu,
					const struct nmg_visit_handlers	*htab,
					void *			state);
RT_EXPORT extern void nmg_visit_face(struct face			*f,
				     const struct nmg_visit_handlers	*htab,
				     void *			state);
RT_EXPORT extern void nmg_visit_faceuse(struct faceuse			*fu,
					const struct nmg_visit_handlers	*htab,
					void *			state);
RT_EXPORT extern void nmg_visit_shell(struct shell			*s,
				      const struct nmg_visit_handlers	*htab,
				      void *			state);
RT_EXPORT extern void nmg_visit_region(struct nmgregion		*r,
				       const struct nmg_visit_handlers	*htab,
				       void *			state);
RT_EXPORT extern void nmg_visit_model(struct model			*model,
				      const struct nmg_visit_handlers	*htab,
				      void *			state);
RT_EXPORT extern void nmg_visit(const uint32_t		*magicp,
				const struct nmg_visit_handlers	*htab,
				void *			state);

/* db5_types.c */
RT_EXPORT extern int db5_type_tag_from_major(char	**tag,
					     const int	major);

RT_EXPORT extern int db5_type_descrip_from_major(char	**descrip,
						 const int	major);

RT_EXPORT extern int db5_type_tag_from_codes(char	**tag,
					     const int	major,
					     const int	minor);

RT_EXPORT extern int db5_type_descrip_from_codes(char	**descrip,
						 const int	major,
						 const int	minor);

RT_EXPORT extern int db5_type_codes_from_tag(int	*major,
					     int	*minor,
					     const char	*tag);

RT_EXPORT extern int db5_type_codes_from_descrip(int	*major,
						 int	*minor,
						 const char	*descrip);

RT_EXPORT extern size_t db5_type_sizeof_h_binu(const int minor);

RT_EXPORT extern size_t db5_type_sizeof_n_binu(const int minor);


/* db5_attr.c */
/**
 * Define standard attribute types in BRL-CAD geometry.  (See the
 * attributes manual page.)  These should be a collective enumeration
 * starting from 0 and increasing without any gaps in the numbers so
 * db5_standard_attribute() can be used as an index-based iterator.
 */
enum {
    ATTR_REGION = 0,
    ATTR_REGION_ID,
    ATTR_MATERIAL_ID,
    ATTR_AIR,
    ATTR_LOS,
    ATTR_COLOR,
    ATTR_SHADER,
    ATTR_INHERIT,
    ATTR_TIMESTAMP, /* a binary attribute */
    ATTR_NULL
};

/* Enum to characterize status of attributes */
enum {
    ATTR_STANDARD = 0,
    ATTR_USER_DEFINED,
    ATTR_UNKNOWN_ORIGIN
};

struct db5_attr_ctype {
    int attr_type;    /* ID from the main attr enum list */
    int is_binary;   /* false for ASCII attributes; true for binary attributes */
    int attr_subtype; /* ID from attribute status enum list */

    /* names should be specified with alphanumeric characters
     * (lower-case letters, no white space) and will act as unique
     * keys to an object's attribute list */
    const char *name;         /* the "standard" name */
    const char *description;
    const char *examples;
    /* list of alternative names for this attribute: */
    const char *aliases;
    /* property name */
    const char *property;
    /* a longer description for lists outside a table */
    const char *long_description;
};

RT_EXPORT extern const struct db5_attr_ctype db5_attr_std[];

/* Container for holding user-registered attributes */
struct db5_registry{
    void *internal; /**< @brief implementation-specific container for holding information*/
};

/**
 * Initialize a user attribute registry
 *
 * PRIVATE: this is new API and should be considered private for the
 * time being.
 */
RT_EXPORT extern void db5_attr_registry_init(struct db5_registry *registry);

/**
 * Free a user attribute registry
 *
 * PRIVATE: this is new API and should be considered private for the
 * time being.
 */
RT_EXPORT extern void db5_attr_registry_free(struct db5_registry *registry);

/**
 * Register a user attribute
 *
 * PRIVATE: this is new API and should be considered private for the
 * time being.
 */
RT_EXPORT extern int db5_attr_create(struct db5_registry *registry,
	                             int attr_type,
				     int is_binary,
				     int attr_subtype,
				     const char *name,
				     const char *description,
				     const char *examples,
				     const char *aliases,
				     const char *property,
				     const char *long_description);

/**
 * Register a user attribute
 *
 * PRIVATE: this is new API and should be considered private for the
 * time being.
 */
RT_EXPORT extern int db5_attr_register(struct db5_registry *registry,
	                               struct db5_attr_ctype *attribute);

/**
 * De-register a user attribute
 *
 * PRIVATE: this is new API and should be considered private for the
 * time being.
 */
RT_EXPORT extern int db5_attr_deregister(struct db5_registry *registry,
	                                 const char *name);

/**
 * Look to see if a specific attribute is registered
 *
 * PRIVATE: this is new API and should be considered private for the
 * time being.
 */
RT_EXPORT extern struct db5_attr_ctype *db5_attr_get(struct db5_registry *registry,
	                                             const char *name);

/**
 * Get an array of pointers to all registered attributes
 *
 * PRIVATE: this is new API and should be considered private for the
 * time being.
 */
RT_EXPORT extern struct db5_attr_ctype **db5_attr_dump(struct db5_registry *registry);


/**
 * Function returns the string name for a given standard attribute
 * index.  Index values returned from db5_standardize_attribute()
 * correspond to the names returned from this function, returning the
 * "standard" name.  Callers may also iterate over all names starting
 * with an index of zero until a NULL is returned.
 *
 * PRIVATE: this is new API and should be considered private for the
 * time being.
 */
RT_EXPORT extern const char *db5_standard_attribute(int idx);


/**
 * Function returns the string definition for a given standard
 * attribute index.  Index values returned from
 * db5_standardize_attribute_def() correspond to the definition
 * returned from this function, returning the "standard" definition.
 * Callers may also iterate over all names starting with an index of
 * zero until a NULL is returned.
 *
 * PRIVATE: this is new API and should be considered private for the
 * time being.
 */
RT_EXPORT extern const char *db5_standard_attribute_def(int idx);

/**
 * Function for recognizing various versions of the DB5 standard
 * attribute names that have been used - returns the attribute type of
 * the supplied attribute name, or -1 if it is not a recognized
 * variation of the standard attributes.
 *
 * PRIVATE: this is new API and should be considered private for the
 * time being.
 */
RT_EXPORT extern int db5_is_standard_attribute(const char *attrname);

/**
 * Ensures that an attribute set containing standard attributes with
 * non-standard/old/deprecated names gets the standard name added.  It
 * will update the first non-standard name encountered, but will leave
 * any subsequent matching attributes found unmodified if they have
 * different values.  Such "conflict" attributes must be resolved
 * manually.
 *
 * Returns the number of conflicting attributes.
 *
 * PRIVATE: this is new API and should be considered private for the
 * time being.
 */
RT_EXPORT extern size_t db5_standardize_avs(struct bu_attribute_value_set *avs);

/**
 * PRIVATE: this is new API and should be considered private for the
 * time being.
 */
RT_EXPORT extern int db5_standardize_attribute(const char *attr);
/**
 * PRIVATE: this is new API and should be considered private for the
 * time being.
 */
RT_EXPORT extern void db5_sync_attr_to_comb(struct rt_comb_internal *comb, const struct bu_attribute_value_set *avs, struct directory *dp);
/**
 * PRIVATE: this is new API and should be considered private for the
 * time being.
 */
RT_EXPORT extern void db5_sync_comb_to_attr(struct bu_attribute_value_set *avs, const struct rt_comb_internal *comb);

/* Convenience macros */
#define ATTR_STD(attr) db5_standard_attribute(db5_standardize_attribute(attr))


#endif

/* defined in binary_obj.c */
RT_EXPORT extern int rt_mk_binunif(struct rt_wdb *wdbp,
				   const char *obj_name,
				   const char *file_name,
				   unsigned int minor_type,
				   size_t max_count);


/* defined in db5_bin.c */

/**
 * Free the storage associated with a binunif_internal object
 */
RT_EXPORT extern void rt_binunif_free(struct rt_binunif_internal *bip);

/**
 * Diagnostic routine
 */
RT_EXPORT extern void rt_binunif_dump(struct rt_binunif_internal *bip);

/* defined in cline.c */

/**
 * radius of a FASTGEN cline element.
 *
 * shared with rt/do.c
 */
RT_EXPORT extern fastf_t rt_cline_radius;

/* defined in bot.c */
/* TODO - these global variables need to be rolled in to the rt_i structure */
RT_EXPORT extern size_t rt_bot_minpieces;
RT_EXPORT extern size_t rt_bot_tri_per_piece;
RT_EXPORT extern int rt_bot_sort_faces(struct rt_bot_internal *bot,
				       size_t tris_per_piece);
RT_EXPORT extern int rt_bot_decimate(struct rt_bot_internal *bot,
				     fastf_t max_chord_error,
				     fastf_t max_normal_error,
				     fastf_t min_edge_length);

/*
 *  Constants provided and used by the RT library.
 */

/**
 * initial tree start for db tree walkers.
 *
 * Also used by converters in conv/ directory.  Don't forget to
 * initialize ts_dbip before use.
 */
RT_EXPORT extern const struct db_tree_state rt_initial_tree_state;


/** @file librt/vers.c
 *
 * version information about LIBRT
 *
 */

/**
 * report version information about LIBRT
 */
RT_EXPORT extern const char *rt_version(void);


/* WARNING - The function below is *HIGHLY* experimental and will certainly
 * change */
RT_EXPORT void
rt_generate_mesh(int **faces, int *num_faces, point_t **points, int *num_pnts,
	                        struct db_i *dbip, const char *obj, fastf_t delta);


__END_DECLS

#endif /* RAYTRACE_H */
/** @} */
/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
