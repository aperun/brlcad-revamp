/******************************************************************************
 * $Id: internal_qhull_headers.h 34641 2016-07-12 10:54:28Z rouault $
 *
 * Project:  GDAL
 * Purpose:  Includes internal qhull headers
 * Author:   Even Rouault <even dot rouault at spatialys dot com>
 *
 ******************************************************************************
 * Copyright (c) 2015, Even Rouault <even dot rouault at spatialys dot com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#ifndef INTERNAL_QHULL_HEADERS_H
#define INTERNAL_QHULL_HEADERS_H

#ifdef HAVE_GCC_SYSTEM_HEADER
#pragma GCC system_header
#endif

#if defined(__MINGW64__)
/* See https://github.com/scipy/scipy/issues/3237 */
/* This ensures that ptr_intT is a long lon on MinGW 64 */
#define _MSC_VER 1
#endif

// To avoid issue with icc that defines a template in qhull_a.h
#if defined(__INTEL_COMPILER)
#define QHULL_OS_WIN
#endif

/* Below a lot of renames and static definition of the symbols so as */
/* to avoid "contaminating" with a potential external qhull */
typedef struct setT gdal_setT;
typedef struct facetT gdal_facetT;
typedef struct vertexT gdal_vertexT;
typedef struct qhT gdal_qhT;
typedef struct ridgeT gdal_ridgeT;
#define gdal_realT double
#define gdal_pointT double
#define gdal_realT double
#define gdal_coordT double
#define gdal_boolT unsigned int

#define qhmem gdal_qhmem
#define qh_rand_seed gdal_qh_rand_seed
#define qh_qh gdal_qh_qh
#define qh_qhstat gdal_qh_qhstat
#define qh_version gdal_qh_version
#define qhull_inuse gdal_qhull_inuse

#define qh_compare_vertexpoint gdal_qh_compare_vertexpoint
static int qh_compare_vertexpoint();

#define qh_intcompare gdal_qh_intcompare

#ifdef notdef

Generated by the following Python script + manual cleaning of the result

f = open('headers.txt')
for line in f.readlines():
    line = line[0:-1].strip()
    if len(line) > 3 and line[0] != '#' and (line[-2:] == ');' or line[-1:] == ',') and \
       line.find('(') > 0 and line.find('=') < 0 and line.find('typedef') < 0 and line.find('are used by') < 0 and \
       line.find('&') < 0 and line.find('"') < 0 and line.find(' ') < line.find('('):
        line = line[0:line.find('(')].strip()
        last_star = line.rfind('*')
        last_space = line.rfind(' ')
        if last_star > last_space:
            (type, name) = (line[0:last_star+1], line[last_star+1:])
        else:
            (type, name) = (line[0:last_space], line[last_space+1:])
        type = type.strip()
        print('#define %s gdal_%s' % (name, name))
        if type.find('void') != 0 and type.find('int') != 0 and type.find('char') != 0 and type.find('double') != 0 and type.find('unsigned') != 0:
            type = 'gdal_' + type
        print("static %s %s();" % (type, name))
#endif

#define qh_backnormal gdal_qh_backnormal
static void qh_backnormal();
#define qh_distplane gdal_qh_distplane
static void qh_distplane();
#define qh_findbest gdal_qh_findbest
static gdal_facetT * qh_findbest();
#define qh_findbesthorizon gdal_qh_findbesthorizon
static gdal_facetT * qh_findbesthorizon();
#define qh_findbestnew gdal_qh_findbestnew
static gdal_facetT * qh_findbestnew();
#define qh_gausselim gdal_qh_gausselim
static void qh_gausselim();
#define qh_getangle gdal_qh_getangle
static gdal_realT qh_getangle();
#define qh_getcenter gdal_qh_getcenter
static gdal_pointT * qh_getcenter();
#define qh_getcentrum gdal_qh_getcentrum
static gdal_pointT * qh_getcentrum();
#define qh_getdistance gdal_qh_getdistance
static gdal_realT qh_getdistance();
#define qh_normalize gdal_qh_normalize
static void qh_normalize();
#define qh_normalize2 gdal_qh_normalize2
static void qh_normalize2();
#define qh_projectpoint gdal_qh_projectpoint
static gdal_pointT * qh_projectpoint();
#define qh_setfacetplane gdal_qh_setfacetplane
static void qh_setfacetplane();
#define qh_sethyperplane_det gdal_qh_sethyperplane_det
static void qh_sethyperplane_det();
#define qh_sethyperplane_gauss gdal_qh_sethyperplane_gauss
static void qh_sethyperplane_gauss();
#define qh_sharpnewfacets gdal_qh_sharpnewfacets
static gdal_boolT qh_sharpnewfacets();
#define qh_copypoints gdal_qh_copypoints
static gdal_coordT * qh_copypoints();
#define qh_crossproduct gdal_qh_crossproduct
static void qh_crossproduct();
#define qh_determinant gdal_qh_determinant
static gdal_realT qh_determinant();
#define qh_detjoggle gdal_qh_detjoggle
static gdal_realT qh_detjoggle();
#define qh_detroundoff gdal_qh_detroundoff
static void qh_detroundoff();
#define qh_detsimplex gdal_qh_detsimplex
static gdal_realT qh_detsimplex();
#define qh_distnorm gdal_qh_distnorm
static gdal_realT qh_distnorm();
#define qh_distround gdal_qh_distround
static gdal_realT qh_distround();
#define qh_divzero gdal_qh_divzero
static gdal_realT qh_divzero();
#define qh_facetarea gdal_qh_facetarea
static gdal_realT qh_facetarea();
#define qh_facetarea_simplex gdal_qh_facetarea_simplex
static gdal_realT qh_facetarea_simplex();
#define qh_facetcenter gdal_qh_facetcenter
static gdal_pointT * qh_facetcenter();
#define qh_findgooddist gdal_qh_findgooddist
static gdal_facetT * qh_findgooddist();
#define qh_getarea gdal_qh_getarea
static void qh_getarea();
#define qh_gram_schmidt gdal_qh_gram_schmidt
static gdal_boolT qh_gram_schmidt();
#define qh_inthresholds gdal_qh_inthresholds
static gdal_boolT qh_inthresholds();
#define qh_joggleinput gdal_qh_joggleinput
static void qh_joggleinput();
#define qh_maxabsval gdal_qh_maxabsval
static gdal_realT  * qh_maxabsval();
#define qh_maxmin gdal_qh_maxmin
static gdal_setT   * qh_maxmin();
#define qh_maxouter gdal_qh_maxouter
static gdal_realT qh_maxouter();
#define qh_maxsimplex gdal_qh_maxsimplex
static void qh_maxsimplex();
#define qh_minabsval gdal_qh_minabsval
static gdal_realT qh_minabsval();
#define qh_mindiff gdal_qh_mindiff
static int qh_mindiff();
#define qh_orientoutside gdal_qh_orientoutside
static gdal_boolT qh_orientoutside();
#define qh_outerinner gdal_qh_outerinner
static void qh_outerinner();
#define qh_pointdist gdal_qh_pointdist
static gdal_coordT qh_pointdist();
#define qh_printmatrix gdal_qh_printmatrix
static void qh_printmatrix();
#define qh_printpoints gdal_qh_printpoints
static void qh_printpoints();
#define qh_projectinput gdal_qh_projectinput
static void qh_projectinput();
#define qh_projectpoints gdal_qh_projectpoints
static void qh_projectpoints();
#define qh_rotateinput gdal_qh_rotateinput
static void qh_rotateinput();
#define qh_rotatepoints gdal_qh_rotatepoints
static void qh_rotatepoints();
#define qh_scaleinput gdal_qh_scaleinput
static void qh_scaleinput();
#define qh_scalelast gdal_qh_scalelast
static void qh_scalelast();
#define qh_scalepoints gdal_qh_scalepoints
static void qh_scalepoints();
#define qh_sethalfspace gdal_qh_sethalfspace
static gdal_boolT qh_sethalfspace();
#define qh_sethalfspace_all gdal_qh_sethalfspace_all
static gdal_coordT * qh_sethalfspace_all();
#define qh_voronoi_center gdal_qh_voronoi_center
static gdal_pointT * qh_voronoi_center();
#define dfacet gdal_dfacet
static void dfacet();
#define dvertex gdal_dvertex
static void dvertex();
#define qh_compare_facetarea gdal_qh_compare_facetarea
static int qh_compare_facetarea();
#define qh_compare_facetmerge gdal_qh_compare_facetmerge
static int qh_compare_facetmerge();
#define qh_compare_facetvisit gdal_qh_compare_facetvisit
static int qh_compare_facetvisit();
#define qh_copyfilename gdal_qh_copyfilename
static void qh_copyfilename();
#define qh_countfacets gdal_qh_countfacets
static void qh_countfacets();
#define qh_detvnorm gdal_qh_detvnorm
static gdal_pointT * qh_detvnorm();
#define qh_detvridge gdal_qh_detvridge
static gdal_setT   * qh_detvridge();
#define qh_detvridge3 gdal_qh_detvridge3
static gdal_setT   * qh_detvridge3();
#define qh_eachvoronoi gdal_qh_eachvoronoi
static int qh_eachvoronoi();
#define qh_eachvoronoi_all gdal_qh_eachvoronoi_all
static int qh_eachvoronoi_all();
#define qh_facet2point gdal_qh_facet2point
static void qh_facet2point();
#define qh_facetvertices gdal_qh_facetvertices
static gdal_setT   * qh_facetvertices();
#define qh_geomplanes gdal_qh_geomplanes
static void qh_geomplanes();
#define qh_markkeep gdal_qh_markkeep
static void qh_markkeep();
#define qh_markvoronoi gdal_qh_markvoronoi
static gdal_setT   * qh_markvoronoi();
#define qh_order_vertexneighbors gdal_qh_order_vertexneighbors
static void qh_order_vertexneighbors();
#define qh_prepare_output gdal_qh_prepare_output
static void qh_prepare_output();
#define qh_printafacet gdal_qh_printafacet
static void qh_printafacet();
#define qh_printbegin gdal_qh_printbegin
static void qh_printbegin();
#define qh_printcenter gdal_qh_printcenter
static void qh_printcenter();
#define qh_printcentrum gdal_qh_printcentrum
static void qh_printcentrum();
#define qh_printend gdal_qh_printend
static void qh_printend();
#define qh_printend4geom gdal_qh_printend4geom
static void qh_printend4geom();
#define qh_printextremes gdal_qh_printextremes
static void qh_printextremes();
#define qh_printextremes_2d gdal_qh_printextremes_2d
static void qh_printextremes_2d();
#define qh_printextremes_d gdal_qh_printextremes_d
static void qh_printextremes_d();
#define qh_printfacet gdal_qh_printfacet
static void qh_printfacet();
#define qh_printfacet2math gdal_qh_printfacet2math
static void qh_printfacet2math();
#define qh_printfacet2geom gdal_qh_printfacet2geom
static void qh_printfacet2geom();
#define qh_printfacet2geom_points gdal_qh_printfacet2geom_points
static void qh_printfacet2geom_points();
#define qh_printfacet3math gdal_qh_printfacet3math
static void qh_printfacet3math();
#define qh_printfacet3geom_nonsimplicial gdal_qh_printfacet3geom_nonsimplicial
static void qh_printfacet3geom_nonsimplicial();
#define qh_printfacet3geom_points gdal_qh_printfacet3geom_points
static void qh_printfacet3geom_points();
#define qh_printfacet3geom_simplicial gdal_qh_printfacet3geom_simplicial
static void qh_printfacet3geom_simplicial();
#define qh_printfacet3vertex gdal_qh_printfacet3vertex
static void qh_printfacet3vertex();
#define qh_printfacet4geom_nonsimplicial gdal_qh_printfacet4geom_nonsimplicial
static void qh_printfacet4geom_nonsimplicial();
#define qh_printfacet4geom_simplicial gdal_qh_printfacet4geom_simplicial
static void qh_printfacet4geom_simplicial();
#define qh_printfacetNvertex_nonsimplicial gdal_qh_printfacetNvertex_nonsimplicial
static void qh_printfacetNvertex_nonsimplicial();
#define qh_printfacetNvertex_simplicial gdal_qh_printfacetNvertex_simplicial
static void qh_printfacetNvertex_simplicial();
#define qh_printfacetheader gdal_qh_printfacetheader
static void qh_printfacetheader();
#define qh_printfacetridges gdal_qh_printfacetridges
static void qh_printfacetridges();
#define qh_printfacets gdal_qh_printfacets
static void qh_printfacets();
#define qh_printhyperplaneintersection gdal_qh_printhyperplaneintersection
static void qh_printhyperplaneintersection();
#define qh_printneighborhood gdal_qh_printneighborhood
static void qh_printneighborhood();
#define qh_printline3geom gdal_qh_printline3geom
static void qh_printline3geom();
#define qh_printpoint gdal_qh_printpoint
static void qh_printpoint();
#define qh_printpointid gdal_qh_printpointid
static void qh_printpointid();
#define qh_printpoint3 gdal_qh_printpoint3
static void qh_printpoint3();
#define qh_printpoints_out gdal_qh_printpoints_out
static void qh_printpoints_out();
#define qh_printpointvect gdal_qh_printpointvect
static void qh_printpointvect();
#define qh_printpointvect2 gdal_qh_printpointvect2
static void qh_printpointvect2();
#define qh_printridge gdal_qh_printridge
static void qh_printridge();
#define qh_printspheres gdal_qh_printspheres
static void qh_printspheres();
#define qh_printvdiagram gdal_qh_printvdiagram
static void qh_printvdiagram();
#define qh_printvdiagram2 gdal_qh_printvdiagram2
static int qh_printvdiagram2();
#define qh_printvertex gdal_qh_printvertex
static void qh_printvertex();
#define qh_printvertexlist gdal_qh_printvertexlist
static void qh_printvertexlist();
#define qh_printvertices gdal_qh_printvertices
static void qh_printvertices();
#define qh_printvneighbors gdal_qh_printvneighbors
static void qh_printvneighbors();
#define qh_printvoronoi gdal_qh_printvoronoi
static void qh_printvoronoi();
#define qh_printvnorm gdal_qh_printvnorm
static void qh_printvnorm();
#define qh_printvridge gdal_qh_printvridge
static void qh_printvridge();
#define qh_produce_output gdal_qh_produce_output
static void qh_produce_output();
#define qh_produce_output2 gdal_qh_produce_output2
static void qh_produce_output2();
#define qh_projectdim3 gdal_qh_projectdim3
static void qh_projectdim3();
#define qh_readfeasible gdal_qh_readfeasible
static int qh_readfeasible();
#define qh_readpoints gdal_qh_readpoints
static gdal_coordT * qh_readpoints();
#define qh_setfeasible gdal_qh_setfeasible
static void qh_setfeasible();
#define qh_skipfacet gdal_qh_skipfacet
static gdal_boolT qh_skipfacet();
#define qh_skipfilename gdal_qh_skipfilename
static char   * qh_skipfilename();
#define qh_qhull gdal_qh_qhull
static void qh_qhull();
#define qh_addpoint gdal_qh_addpoint
static gdal_boolT qh_addpoint();
#define qh_printsummary gdal_qh_printsummary
static void qh_printsummary();
#define qh_errexit gdal_qh_errexit
static void qh_errexit();
#define qh_errprint gdal_qh_errprint
static void qh_errprint();
#define qh_new_qhull gdal_qh_new_qhull
static int qh_new_qhull();
#define qh_printfacetlist gdal_qh_printfacetlist
static void qh_printfacetlist();
#define qh_printhelp_degenerate gdal_qh_printhelp_degenerate
static void qh_printhelp_degenerate();
#define qh_printhelp_narrowhull gdal_qh_printhelp_narrowhull
static void qh_printhelp_narrowhull();
#define qh_printhelp_singular gdal_qh_printhelp_singular
static void qh_printhelp_singular();
#define qh_user_memsizes gdal_qh_user_memsizes
static void qh_user_memsizes();
#define qh_exit gdal_qh_exit
static void qh_exit();
#define qh_free gdal_qh_free
static void qh_free();
#define qh_malloc gdal_qh_malloc
static void   * qh_malloc();
#define qh_fprintf gdal_qh_fprintf
static void qh_fprintf(FILE *fp, int msgcode, const char *fmt, ... );
/*#define qh_fprintf_rbox gdal_qh_fprintf_rbox*/
/*static void qh_fprintf_rbox(FILE *fp, int msgcode, const char *fmt, ... );*/
#define qh_findbest gdal_qh_findbest
static gdal_facetT * qh_findbest();
#define qh_findbestnew gdal_qh_findbestnew
static gdal_facetT * qh_findbestnew();
#define qh_gram_schmidt gdal_qh_gram_schmidt
static gdal_boolT qh_gram_schmidt();
#define qh_outerinner gdal_qh_outerinner
static void qh_outerinner();
#define qh_printsummary gdal_qh_printsummary
static void qh_printsummary();
#define qh_projectinput gdal_qh_projectinput
static void qh_projectinput();
#define qh_randommatrix gdal_qh_randommatrix
static void qh_randommatrix();
#define qh_rotateinput gdal_qh_rotateinput
static void qh_rotateinput();
#define qh_scaleinput gdal_qh_scaleinput
static void qh_scaleinput();
#define qh_setdelaunay gdal_qh_setdelaunay
static void qh_setdelaunay();
#define qh_sethalfspace_all gdal_qh_sethalfspace_all
static gdal_coordT  * qh_sethalfspace_all();
#define qh_clock gdal_qh_clock
static unsigned long qh_clock();
#define qh_checkflags gdal_qh_checkflags
static void qh_checkflags();
#define qh_clear_outputflags gdal_qh_clear_outputflags
static void qh_clear_outputflags();
#define qh_freebuffers gdal_qh_freebuffers
static void qh_freebuffers();
#define qh_freeqhull gdal_qh_freeqhull
static void qh_freeqhull();
#define qh_freeqhull2 gdal_qh_freeqhull2
static void qh_freeqhull2();
#define qh_init_A gdal_qh_init_A
static void qh_init_A();
#define qh_init_B gdal_qh_init_B
static void qh_init_B();
#define qh_init_qhull_command gdal_qh_init_qhull_command
static void qh_init_qhull_command();
/*#define qh_initbuffers gdal_qh_initbuffers*/
/*static void qh_initbuffers();*/
#define qh_initflags gdal_qh_initflags
static void qh_initflags();
#define qh_initqhull_buffers gdal_qh_initqhull_buffers
static void qh_initqhull_buffers();
#define qh_initqhull_globals gdal_qh_initqhull_globals
static void qh_initqhull_globals();
#define qh_initqhull_mem gdal_qh_initqhull_mem
static void qh_initqhull_mem();
#define qh_initqhull_outputflags gdal_qh_initqhull_outputflags
static void qh_initqhull_outputflags();
#define qh_initqhull_start gdal_qh_initqhull_start
static void qh_initqhull_start();
#define qh_initqhull_start2 gdal_qh_initqhull_start2
static void qh_initqhull_start2();
#define qh_initthresholds gdal_qh_initthresholds
static void qh_initthresholds();
#define qh_option gdal_qh_option
static void qh_option();
/*#define qh_restore_qhull gdal_qh_restore_qhull*/
/*static void qh_restore_qhull();*/
/*#define qh_save_qhull gdal_qh_save_qhull*/
/*static gdal_qhT    * qh_save_qhull();*/
#define dfacet gdal_dfacet
static void dfacet();
#define dvertex gdal_dvertex
static void dvertex();
#define qh_printneighborhood gdal_qh_printneighborhood
static void qh_printneighborhood();
#define qh_produce_output gdal_qh_produce_output
static void qh_produce_output();
#define qh_readpoints gdal_qh_readpoints
static gdal_coordT * qh_readpoints();
#define qh_meminit gdal_qh_meminit
static void qh_meminit();
#define qh_memfreeshort gdal_qh_memfreeshort
static void qh_memfreeshort();
#define qh_check_output gdal_qh_check_output
static void qh_check_output();
#define qh_check_points gdal_qh_check_points
static void qh_check_points();
#define qh_facetvertices gdal_qh_facetvertices
static gdal_setT   * qh_facetvertices();
#define qh_findbestfacet gdal_qh_findbestfacet
static gdal_facetT * qh_findbestfacet();
#define qh_nearvertex gdal_qh_nearvertex
static gdal_vertexT * qh_nearvertex();
#define qh_point gdal_qh_point
static gdal_pointT * qh_point();
#define qh_pointfacet gdal_qh_pointfacet
static gdal_setT   * qh_pointfacet();
#define qh_pointid gdal_qh_pointid
static int qh_pointid();
#define qh_pointvertex gdal_qh_pointvertex
static gdal_setT   * qh_pointvertex();
#define qh_setvoronoi_all gdal_qh_setvoronoi_all
static void qh_setvoronoi_all();
#define qh_triangulate gdal_qh_triangulate
static void qh_triangulate();
/*#define qh_rboxpoints gdal_qh_rboxpoints*/
/*static int qh_rboxpoints();*/
/*#define qh_errexit_rbox gdal_qh_errexit_rbox*/
/*static void qh_errexit_rbox();*/
#define qh_collectstatistics gdal_qh_collectstatistics
static void qh_collectstatistics();
#define qh_printallstatistics gdal_qh_printallstatistics
static void qh_printallstatistics();
/*#define machines gdal_machines*/
/*static gdal_of machines();*/
#define qh_memalloc gdal_qh_memalloc
static void * qh_memalloc();
#define qh_memfree gdal_qh_memfree
static void qh_memfree();
#define qh_memfreeshort gdal_qh_memfreeshort
static void qh_memfreeshort();
#define qh_meminit gdal_qh_meminit
static void qh_meminit();
#define qh_meminitbuffers gdal_qh_meminitbuffers
static void qh_meminitbuffers();
#define qh_memsetup gdal_qh_memsetup
static void qh_memsetup();
#define qh_memsize gdal_qh_memsize
static void qh_memsize();
#define qh_memstatistics gdal_qh_memstatistics
static void qh_memstatistics();
#define qh_memtotal gdal_qh_memtotal
static void qh_memtotal();
/*#define qh_mergefacet gdal_qh_mergefacet*/
/*static gdal_if qh_mergefacet();*/
#define qh_premerge gdal_qh_premerge
static void qh_premerge();
#define qh_postmerge gdal_qh_postmerge
static void qh_postmerge();
#define qh_all_merges gdal_qh_all_merges
static void qh_all_merges();
#define qh_appendmergeset gdal_qh_appendmergeset
static void qh_appendmergeset();
#define qh_basevertices gdal_qh_basevertices
static gdal_setT   * qh_basevertices();
#define qh_checkconnect gdal_qh_checkconnect
static void qh_checkconnect();
#define qh_checkzero gdal_qh_checkzero
static gdal_boolT qh_checkzero();
#define qh_compareangle gdal_qh_compareangle
static int qh_compareangle();
#define qh_comparemerge gdal_qh_comparemerge
static int qh_comparemerge();
#define qh_comparevisit gdal_qh_comparevisit
static int qh_comparevisit();
#define qh_copynonconvex gdal_qh_copynonconvex
static void qh_copynonconvex();
#define qh_degen_redundant_facet gdal_qh_degen_redundant_facet
static void qh_degen_redundant_facet();
#define qh_degen_redundant_neighbors gdal_qh_degen_redundant_neighbors
static void qh_degen_redundant_neighbors();
#define qh_find_newvertex gdal_qh_find_newvertex
static gdal_vertexT * qh_find_newvertex();
#define qh_findbest_test gdal_qh_findbest_test
static void qh_findbest_test();
#define qh_findbestneighbor gdal_qh_findbestneighbor
static gdal_facetT * qh_findbestneighbor();
#define qh_flippedmerges gdal_qh_flippedmerges
static void qh_flippedmerges();
#define qh_forcedmerges gdal_qh_forcedmerges
static void qh_forcedmerges();
#define qh_getmergeset gdal_qh_getmergeset
static void qh_getmergeset();
#define qh_getmergeset_initial gdal_qh_getmergeset_initial
static void qh_getmergeset_initial();
#define qh_hashridge gdal_qh_hashridge
static void qh_hashridge();
#define qh_hashridge_find gdal_qh_hashridge_find
static gdal_ridgeT * qh_hashridge_find();
#define qh_makeridges gdal_qh_makeridges
static void qh_makeridges();
#define qh_mark_dupridges gdal_qh_mark_dupridges
static void qh_mark_dupridges();
#define qh_maydropneighbor gdal_qh_maydropneighbor
static void qh_maydropneighbor();
#define qh_merge_degenredundant gdal_qh_merge_degenredundant
static int qh_merge_degenredundant();
#define qh_merge_nonconvex gdal_qh_merge_nonconvex
static void qh_merge_nonconvex();
#define qh_mergecycle gdal_qh_mergecycle
static void qh_mergecycle();
#define qh_mergecycle_all gdal_qh_mergecycle_all
static void qh_mergecycle_all();
#define qh_mergecycle_facets gdal_qh_mergecycle_facets
static void qh_mergecycle_facets();
#define qh_mergecycle_neighbors gdal_qh_mergecycle_neighbors
static void qh_mergecycle_neighbors();
#define qh_mergecycle_ridges gdal_qh_mergecycle_ridges
static void qh_mergecycle_ridges();
#define qh_mergecycle_vneighbors gdal_qh_mergecycle_vneighbors
static void qh_mergecycle_vneighbors();
#define qh_mergefacet gdal_qh_mergefacet
static void qh_mergefacet();
#define qh_mergefacet2d gdal_qh_mergefacet2d
static void qh_mergefacet2d();
#define qh_mergeneighbors gdal_qh_mergeneighbors
static void qh_mergeneighbors();
#define qh_mergeridges gdal_qh_mergeridges
static void qh_mergeridges();
#define qh_mergesimplex gdal_qh_mergesimplex
static void qh_mergesimplex();
#define qh_mergevertex_del gdal_qh_mergevertex_del
static void qh_mergevertex_del();
#define qh_mergevertex_neighbors gdal_qh_mergevertex_neighbors
static void qh_mergevertex_neighbors();
#define qh_mergevertices gdal_qh_mergevertices
static void qh_mergevertices();
#define qh_neighbor_intersections gdal_qh_neighbor_intersections
static gdal_setT   * qh_neighbor_intersections();
#define qh_newvertices gdal_qh_newvertices
static void qh_newvertices();
#define qh_reducevertices gdal_qh_reducevertices
static gdal_boolT qh_reducevertices();
#define qh_redundant_vertex gdal_qh_redundant_vertex
static gdal_vertexT * qh_redundant_vertex();
#define qh_remove_extravertices gdal_qh_remove_extravertices
static gdal_boolT qh_remove_extravertices();
#define qh_rename_sharedvertex gdal_qh_rename_sharedvertex
static gdal_vertexT * qh_rename_sharedvertex();
#define qh_renameridgevertex gdal_qh_renameridgevertex
static void qh_renameridgevertex();
#define qh_renamevertex gdal_qh_renamevertex
static void qh_renamevertex();
#define qh_test_appendmerge gdal_qh_test_appendmerge
static gdal_boolT qh_test_appendmerge();
#define qh_test_vneighbors gdal_qh_test_vneighbors
static gdal_boolT qh_test_vneighbors();
#define qh_tracemerge gdal_qh_tracemerge
static void qh_tracemerge();
#define qh_tracemerging gdal_qh_tracemerging
static void qh_tracemerging();
#define qh_updatetested gdal_qh_updatetested
static void qh_updatetested();
#define qh_vertexridges gdal_qh_vertexridges
static gdal_setT   * qh_vertexridges();
#define qh_vertexridges_facet gdal_qh_vertexridges_facet
static void qh_vertexridges_facet();
#define qh_willdelete gdal_qh_willdelete
static void qh_willdelete();
#define qh_appendfacet gdal_qh_appendfacet
static void qh_appendfacet();
#define qh_appendvertex gdal_qh_appendvertex
static void qh_appendvertex();
#define qh_attachnewfacets gdal_qh_attachnewfacets
static void qh_attachnewfacets();
#define qh_checkflipped gdal_qh_checkflipped
static gdal_boolT qh_checkflipped();
#define qh_delfacet gdal_qh_delfacet
static void qh_delfacet();
#define qh_deletevisible gdal_qh_deletevisible
static void qh_deletevisible();
#define qh_facetintersect gdal_qh_facetintersect
static gdal_setT   * qh_facetintersect();
#define qh_gethash gdal_qh_gethash
static int qh_gethash();
#define qh_makenewfacet gdal_qh_makenewfacet
static gdal_facetT * qh_makenewfacet();
#define qh_makenewplanes gdal_qh_makenewplanes
static void qh_makenewplanes();
#define qh_makenew_nonsimplicial gdal_qh_makenew_nonsimplicial
static gdal_facetT * qh_makenew_nonsimplicial();
#define qh_makenew_simplicial gdal_qh_makenew_simplicial
static gdal_facetT * qh_makenew_simplicial();
#define qh_matchneighbor gdal_qh_matchneighbor
static void qh_matchneighbor();
#define qh_matchnewfacets gdal_qh_matchnewfacets
static void qh_matchnewfacets();
#define qh_matchvertices gdal_qh_matchvertices
static gdal_boolT qh_matchvertices();
#define qh_newfacet gdal_qh_newfacet
static gdal_facetT * qh_newfacet();
#define qh_newridge gdal_qh_newridge
static gdal_ridgeT * qh_newridge();
#define qh_pointid gdal_qh_pointid
static int qh_pointid();
#define qh_removefacet gdal_qh_removefacet
static void qh_removefacet();
#define qh_removevertex gdal_qh_removevertex
static void qh_removevertex();
#define qh_updatevertices gdal_qh_updatevertices
static void qh_updatevertices();
#define qh_addhash gdal_qh_addhash
static void qh_addhash();
#define qh_check_bestdist gdal_qh_check_bestdist
static void qh_check_bestdist();
#define qh_check_maxout gdal_qh_check_maxout
static void qh_check_maxout();
#define qh_check_output gdal_qh_check_output
static void qh_check_output();
#define qh_check_point gdal_qh_check_point
static void qh_check_point();
#define qh_check_points gdal_qh_check_points
static void qh_check_points();
#define qh_checkconvex gdal_qh_checkconvex
static void qh_checkconvex();
#define qh_checkfacet gdal_qh_checkfacet
static void qh_checkfacet();
#define qh_checkflipped_all gdal_qh_checkflipped_all
static void qh_checkflipped_all();
#define qh_checkpolygon gdal_qh_checkpolygon
static void qh_checkpolygon();
#define qh_checkvertex gdal_qh_checkvertex
static void qh_checkvertex();
#define qh_clearcenters gdal_qh_clearcenters
static void qh_clearcenters();
#define qh_createsimplex gdal_qh_createsimplex
static void qh_createsimplex();
#define qh_delridge gdal_qh_delridge
static void qh_delridge();
#define qh_delvertex gdal_qh_delvertex
static void qh_delvertex();
#define qh_facet3vertex gdal_qh_facet3vertex
static gdal_setT   * qh_facet3vertex();
#define qh_findbestfacet gdal_qh_findbestfacet
static gdal_facetT * qh_findbestfacet();
#define qh_findbestlower gdal_qh_findbestlower
static gdal_facetT * qh_findbestlower();
#define qh_findfacet_all gdal_qh_findfacet_all
static gdal_facetT * qh_findfacet_all();
#define qh_findgood gdal_qh_findgood
static int qh_findgood();
#define qh_findgood_all gdal_qh_findgood_all
static void qh_findgood_all();
#define qh_furthestnext gdal_qh_furthestnext
static void qh_furthestnext();
#define qh_furthestout gdal_qh_furthestout
static void qh_furthestout();
#define qh_infiniteloop gdal_qh_infiniteloop
static void qh_infiniteloop();
#define qh_initbuild gdal_qh_initbuild
static void qh_initbuild();
#define qh_initialhull gdal_qh_initialhull
static void qh_initialhull();
#define qh_initialvertices gdal_qh_initialvertices
static gdal_setT   * qh_initialvertices();
#define qh_isvertex gdal_qh_isvertex
static gdal_vertexT * qh_isvertex();
#define qh_makenewfacets gdal_qh_makenewfacets
static gdal_vertexT * qh_makenewfacets();
#define qh_matchduplicates gdal_qh_matchduplicates
static void qh_matchduplicates();
#define qh_nearcoplanar gdal_qh_nearcoplanar
static void qh_nearcoplanar();
#define qh_nearvertex gdal_qh_nearvertex
static gdal_vertexT * qh_nearvertex();
#define qh_newhashtable gdal_qh_newhashtable
static int qh_newhashtable();
#define qh_newvertex gdal_qh_newvertex
static gdal_vertexT * qh_newvertex();
#define qh_nextridge3d gdal_qh_nextridge3d
static gdal_ridgeT * qh_nextridge3d();
#define qh_outcoplanar gdal_qh_outcoplanar
static void qh_outcoplanar();
#define qh_point gdal_qh_point
static gdal_pointT * qh_point();
#define qh_point_add gdal_qh_point_add
static void qh_point_add();
#define qh_pointfacet gdal_qh_pointfacet
static gdal_setT   * qh_pointfacet();
#define qh_pointvertex gdal_qh_pointvertex
static gdal_setT   * qh_pointvertex();
#define qh_prependfacet gdal_qh_prependfacet
static void qh_prependfacet();
#define qh_printhashtable gdal_qh_printhashtable
static void qh_printhashtable();
#define qh_printlists gdal_qh_printlists
static void qh_printlists();
#define qh_resetlists gdal_qh_resetlists
static void qh_resetlists();
#define qh_setvoronoi_all gdal_qh_setvoronoi_all
static void qh_setvoronoi_all();
#define qh_triangulate gdal_qh_triangulate
static void qh_triangulate();
#define qh_triangulate_facet gdal_qh_triangulate_facet
static void qh_triangulate_facet();
#define qh_triangulate_link gdal_qh_triangulate_link
static void qh_triangulate_link();
#define qh_triangulate_mirror gdal_qh_triangulate_mirror
static void qh_triangulate_mirror();
#define qh_triangulate_null gdal_qh_triangulate_null
static void qh_triangulate_null();
#define qh_vertexintersect gdal_qh_vertexintersect
static void qh_vertexintersect();
#define qh_vertexintersect_new gdal_qh_vertexintersect_new
static gdal_setT   * qh_vertexintersect_new();
#define qh_vertexneighbors gdal_qh_vertexneighbors
static void qh_vertexneighbors();
#define qh_vertexsubset gdal_qh_vertexsubset
static gdal_boolT qh_vertexsubset();
#define qh_qhull gdal_qh_qhull
static void qh_qhull();
#define qh_addpoint gdal_qh_addpoint
static gdal_boolT qh_addpoint();
#define qh_buildhull gdal_qh_buildhull
static void qh_buildhull();
#define qh_buildtracing gdal_qh_buildtracing
static void qh_buildtracing();
#define qh_build_withrestart gdal_qh_build_withrestart
static void qh_build_withrestart();
#define qh_errexit2 gdal_qh_errexit2
static void qh_errexit2();
#define qh_findhorizon gdal_qh_findhorizon
static void qh_findhorizon();
#define qh_nextfurthest gdal_qh_nextfurthest
static gdal_pointT * qh_nextfurthest();
#define qh_partitionall gdal_qh_partitionall
static void qh_partitionall();
#define qh_partitioncoplanar gdal_qh_partitioncoplanar
static void qh_partitioncoplanar();
#define qh_partitionpoint gdal_qh_partitionpoint
static void qh_partitionpoint();
#define qh_partitionvisible gdal_qh_partitionvisible
static void qh_partitionvisible();
#define qh_precision gdal_qh_precision
static void qh_precision();
#define qh_printsummary gdal_qh_printsummary
static void qh_printsummary();
#define qh_appendprint gdal_qh_appendprint
static void qh_appendprint();
#define qh_freebuild gdal_qh_freebuild
static void qh_freebuild();
#define qh_freebuffers gdal_qh_freebuffers
static void qh_freebuffers();
/*#define qh_initbuffers gdal_qh_initbuffers*/
/*static void qh_initbuffers();*/
#define qh_allstatA gdal_qh_allstatA
static void qh_allstatA();
#define qh_allstatB gdal_qh_allstatB
static void qh_allstatB();
#define qh_allstatC gdal_qh_allstatC
static void qh_allstatC();
#define qh_allstatD gdal_qh_allstatD
static void qh_allstatD();
#define qh_allstatE gdal_qh_allstatE
static void qh_allstatE();
#define qh_allstatE2 gdal_qh_allstatE2
static void qh_allstatE2();
#define qh_allstatF gdal_qh_allstatF
static void qh_allstatF();
#define qh_allstatG gdal_qh_allstatG
static void qh_allstatG();
#define qh_allstatH gdal_qh_allstatH
static void qh_allstatH();
#define qh_freebuffers gdal_qh_freebuffers
static void qh_freebuffers();
/*#define qh_initbuffers gdal_qh_initbuffers*/
/*static void qh_initbuffers();*/
#define qh_setaddsorted gdal_qh_setaddsorted
static void qh_setaddsorted();
#define qh_setaddnth gdal_qh_setaddnth
static void qh_setaddnth();
#define qh_setappend gdal_qh_setappend
static void qh_setappend();
#define qh_setappend_set gdal_qh_setappend_set
static void qh_setappend_set();
#define qh_setappend2ndlast gdal_qh_setappend2ndlast
static void qh_setappend2ndlast();
#define qh_setcheck gdal_qh_setcheck
static void qh_setcheck();
#define qh_setcompact gdal_qh_setcompact
static void qh_setcompact();
#define qh_setcopy gdal_qh_setcopy
static gdal_setT * qh_setcopy();
#define qh_setdel gdal_qh_setdel
static void * qh_setdel();
#define qh_setdellast gdal_qh_setdellast
static void * qh_setdellast();
#define qh_setdelnth gdal_qh_setdelnth
static void * qh_setdelnth();
#define qh_setdelnthsorted gdal_qh_setdelnthsorted
static void * qh_setdelnthsorted();
#define qh_setdelsorted gdal_qh_setdelsorted
static void * qh_setdelsorted();
#define qh_setduplicate gdal_qh_setduplicate
static gdal_setT * qh_setduplicate();
#define qh_setequal gdal_qh_setequal
static int qh_setequal();
#define qh_setequal_except gdal_qh_setequal_except
static int qh_setequal_except();
#define qh_setequal_skip gdal_qh_setequal_skip
static int qh_setequal_skip();
#define qh_setendpointer gdal_qh_setendpointer
static void ** qh_setendpointer();
#define qh_setfree gdal_qh_setfree
static void qh_setfree();
#define qh_setfree2 gdal_qh_setfree2
static void qh_setfree2();
#define qh_setfreelong gdal_qh_setfreelong
static void qh_setfreelong();
#define qh_setin gdal_qh_setin
static int qh_setin();
#define qh_setindex gdal_qh_setindex
static int qh_setindex();
#define qh_setlarger gdal_qh_setlarger
static void qh_setlarger();
#define qh_setlast gdal_qh_setlast
static void * qh_setlast();
#define qh_setnew gdal_qh_setnew
static gdal_setT * qh_setnew();
#define qh_setnew_delnthsorted gdal_qh_setnew_delnthsorted
static gdal_setT * qh_setnew_delnthsorted();
#define qh_setprint gdal_qh_setprint
static void qh_setprint();
#define qh_setreplace gdal_qh_setreplace
static void qh_setreplace();
#define qh_setsize gdal_qh_setsize
static int qh_setsize();
#define qh_settemp gdal_qh_settemp
static gdal_setT * qh_settemp();
#define qh_settempfree gdal_qh_settempfree
static void qh_settempfree();
#define qh_settempfree_all gdal_qh_settempfree_all
static void qh_settempfree_all();
#define qh_settemppop gdal_qh_settemppop
static gdal_setT * qh_settemppop();
#define qh_settemppush gdal_qh_settemppush
static void qh_settemppush();
#define qh_settruncate gdal_qh_settruncate
static void qh_settruncate();
#define qh_setunique gdal_qh_setunique
static int qh_setunique();
#define qh_setzero gdal_qh_setzero
static void qh_setzero();
#define qh_argv_to_command gdal_qh_argv_to_command
static int qh_argv_to_command();
#define qh_argv_to_command_size gdal_qh_argv_to_command_size
static int qh_argv_to_command_size();
#define qh_rand gdal_qh_rand
static int qh_rand();
#define qh_srand gdal_qh_srand
static void qh_srand();
#define qh_randomfactor gdal_qh_randomfactor
static gdal_realT qh_randomfactor();
#define qh_randommatrix gdal_qh_randommatrix
static void qh_randommatrix();
#define qh_strtol gdal_qh_strtol
static int qh_strtol();
#define qh_strtod gdal_qh_strtod
static double qh_strtod();
#define qh_allstatA gdal_qh_allstatA
static void qh_allstatA();
#define qh_allstatB gdal_qh_allstatB
static void qh_allstatB();
#define qh_allstatC gdal_qh_allstatC
static void qh_allstatC();
#define qh_allstatD gdal_qh_allstatD
static void qh_allstatD();
#define qh_allstatE gdal_qh_allstatE
static void qh_allstatE();
#define qh_allstatE2 gdal_qh_allstatE2
static void qh_allstatE2();
#define qh_allstatF gdal_qh_allstatF
static void qh_allstatF();
#define qh_allstatG gdal_qh_allstatG
static void qh_allstatG();
#define qh_allstatH gdal_qh_allstatH
static void qh_allstatH();
#define qh_allstatI gdal_qh_allstatI
static void qh_allstatI();
#define qh_allstatistics gdal_qh_allstatistics
static void qh_allstatistics();
#define qh_collectstatistics gdal_qh_collectstatistics
static void qh_collectstatistics();
#define qh_freestatistics gdal_qh_freestatistics
static void qh_freestatistics();
#define qh_initstatistics gdal_qh_initstatistics
static void qh_initstatistics();
#define qh_newstats gdal_qh_newstats
static gdal_boolT qh_newstats();
#define qh_nostatistic gdal_qh_nostatistic
static gdal_boolT qh_nostatistic();
#define qh_printallstatistics gdal_qh_printallstatistics
static void qh_printallstatistics();
#define qh_printstatistics gdal_qh_printstatistics
static void qh_printstatistics();
#define qh_printstatlevel gdal_qh_printstatlevel
static void qh_printstatlevel();
#define qh_printstats gdal_qh_printstats
static void qh_printstats();
#define qh_stddev gdal_qh_stddev
static gdal_realT qh_stddev();

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4324 )
#pragma warning( disable : 4032 )
#pragma warning( disable : 4306 )  /* e.g 'type cast' : conversion from 'long' to 'facetT *' of greater size */
#endif

#include "internal_libqhull/libqhull.h"
#include "internal_libqhull/libqhull.c"
#include "internal_libqhull/poly.c"
#include "internal_libqhull/poly2.c"
#include "internal_libqhull/mem.c"
#include "internal_libqhull/user.c"
#include "internal_libqhull/global.c"
/*#include "userprintf.c"*/
#include "internal_libqhull/random.c"
#include "internal_libqhull/qset.c"
#include "internal_libqhull/io.c"
#include "internal_libqhull/usermem.c"
#include "internal_libqhull/geom.c"
#include "internal_libqhull/geom2.c"
#include "internal_libqhull/stat.c"
#include "internal_libqhull/merge.c"

#ifdef _MSC_VER
#pragma warning( pop )
#endif

/* Replaces userprintf.c implementation */
static void qh_fprintf(CPL_UNUSED FILE *fp, CPL_UNUSED int msgcode, const char *fmt, ... )
{
    va_list args;
    va_start(args, fmt);
    CPLErrorV(CE_Warning, CPLE_AppDefined, fmt, args);
    va_end(args);
}

#endif
