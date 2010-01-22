/*                       A N A L Y Z E . H
 * BRL-CAD
 *
 * Copyright (c) 2008-2010 United States Government as represented by
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
/** @file analyze.h
 *
 * Functions provided by the LIBANALYZE geometry analysis library.
 *
 */

#ifndef __ANALYZE_H__
#define __ANALYZE_H__

#include "raytrace.h"

__BEGIN_DECLS

#ifndef ANALYZE_EXPORT
#  if defined(_WIN32) && !defined(__CYGWIN__) && defined(BRLCAD_DLL)
#    ifdef ANALYZE_EXPORT_DLL
#      define ANALYZE_EXPORT __declspec(dllexport)
#    else
#      define ANALYZE_EXPORT __declspec(dllimport)
#    endif
#  else
#    define ANALYZE_EXPORT
#  endif
#endif


/*
 *     Overlap specific structures
 */

struct region_pair {
    struct bu_list l;
    union {
        char *name;
        struct region *r1;
    } r;
    struct region *r2;
    unsigned long count;
    double max_dist;
    vect_t coord;
};


/**
 *     region_pair for gqa 
 */
ANALYZE_EXPORT BU_EXTERN(struct region_pair *add_unique_pair, (struct region_pair *list, struct region *r1, struct region *r2, double dist, point_t pt));


__END_DECLS

#endif /* __ANALYZE_H__ */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
