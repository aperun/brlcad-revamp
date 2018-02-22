/*                        N I R T . C X X
 * BRL-CAD
 *
 * Copyright (c) 2004-2018 United States Government as represented by
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
/** @file nirt.cxx
 *
 * Implementation of Natalie's Interactive Ray-Tracer (NIRT)
 * functionality.
 *
 */

/* BRL-CAD includes */
#include "common.h"

#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <set>
#include <vector>
#include <limits>

extern "C" {
#include "bu/color.h"
#include "bu/cmd.h"
#include "bu/malloc.h"
#include "bu/path.h"
#include "bu/units.h"
#include "bu/str.h"
#include "bu/vls.h"
#include "analyze.h"
}

/* NIRT segment types */
#define NIRT_HIT_SEG      1    /**< @brief Ray segment representing a solid region */
#define NIRT_MISS_SEG     2    /**< @brief Ray segment representing a gap */
#define NIRT_AIR_SEG      3    /**< @brief Ray segment representing an air region */
#define NIRT_OVERLAP_SEG  4    /**< @brief Ray segment representing an overlap region */

#define NIRT_SILENT_UNSET    0
#define NIRT_SILENT_YES      1
#define NIRT_SILENT_NO       -1

// Used by front end applications, but library needs to store and report as well.
#define NIRT_DEBUG_INTERACT  0x001
#define NIRT_DEBUG_SCRIPTS   0x002
#define NIRT_DEBUG_MAT       0x004

#define NIRT_DEBUG_BACKOUT   0x008
#define NIRT_DEBUG_HITS      0x010
#define NIRT_DEBUG_FMT        "\020\5HITS\4BACKOUT\3MAT\2SCRIPTS\1INTERACT"

#define NIRT_OVLP_RESOLVE            0
#define NIRT_OVLP_REBUILD_FASTGEN    1
#define NIRT_OVLP_REBUILD_ALL        2
#define NIRT_OVLP_RETAIN             3

#define NIRT_PRINTF_SPECIFIERS "difeEgGs"
#define NIRT_OUTPUT_TYPE_SPECIFIERS "rhpfmog"

HIDDEN const char *nirt_cmd_tbl_defs =
"x_orig,         FLOAT,         Ray origin X coordinate,"
"y_orig,         FLOAT,         Ray origin Y coordinate,"
"z_orig,         FLOAT,         Ray origin Z coordinate,"
"h,              FLOAT,         ,"
"v,              FLOAT,         ,"
"d_orig,         FLOAT,         ,"
"x_dir,          FNOUNIT,       Ray direction unit vector x component,"
"y_dir,          FNOUNIT,       Ray direction unit vector y component,"
"z_dir,          FNOUNIT,       Ray direction unit vector z component,"
"a,              FNOUNIT,       Azimuth,"
"e,              FNOUNIT,       Elevation,"
"x_in,           FLOAT,         ,"
"y_in,           FLOAT,         ,"
"z_in,           FLOAT,         ,"
"d_in,           FLOAT,         ,"
"x_out,          FLOAT,         ,"
"y_out,          FLOAT,         ,"
"z_out,          FLOAT,         ,"
"d_out,          FLOAT,         ,"
"los,            FLOAT,         ,"
"scaled_los,     FLOAT,         ,"
"path_name,      STRING,        ,"
"reg_name,       STRING,        ,"
"reg_id,         INT,           ,"
"obliq_in,       FNOUNIT,       ,"
"obliq_out,      FNOUNIT,       ,"
"nm_x_in,        FNOUNIT,       ,"
"nm_y_in,        FNOUNIT,       ,"
"nm_z_in,        FNOUNIT,       ,"
"nm_d_in,        FNOUNIT,       ,"
"nm_h_in,        FNOUNIT,       ,"
"nm_v_in,        FNOUNIT,       ,"
"nm_x_out,       FNOUNIT,       ,"
"nm_y_out,       FNOUNIT,       ,"
"nm_z_out,       FNOUNIT,       ,"
"nm_d_out,       FNOUNIT,       ,"
"nm_h_out,       FNOUNIT,       ,"
"nm_v_out,       FNOUNIT,       ,"
"ov_reg1_name,   STRING,        ,"
"ov_reg1_id,     INT,           ,"
"ov_reg2_name,   STRING,        ,"
"ov_reg2_id,     INT,           ,"
"ov_sol_in,      STRING,        ,"
"ov_sol_out,     STRING,        ,"
"ov_los,         FLOAT,         ,"
"ov_x_in,        FLOAT,         ,"
"ov_y_in,        FLOAT,         ,"
"ov_z_in,        FLOAT,         ,"
"ov_d_in,        FLOAT,         ,"
"ov_x_out,       FLOAT,         ,"
"ov_y_out,       FLOAT,         ,"
"ov_z_out,       FLOAT,         ,"
"ov_d_out,       FLOAT,         ,"
"surf_num_in,    INT,           ,"
"surf_num_out,   INT,           ,"
"claimant_count, INT,           ,"
"claimant_list,  STRING,        ,"
"claimant_listn, STRING,        ,"
"attributes,     STRING,        ,"
"x_gap_in,       FLOAT,         ,"
"y_gap_in,       FLOAT,         ,"
"z_gap_in,       FLOAT,         ,"
"gap_los,        FLOAT,         ,";


struct nirt_overlap {
    struct application *ap;
    struct partition *pp;
    struct region *reg1;
    struct region *reg2;
    fastf_t in_dist;
    fastf_t out_dist;
    point_t in_point;
    point_t out_point;
    struct nirt_overlap *forw;
    struct nirt_overlap *backw;
};
#define NIRT_OVERLAP_NULL ((struct nirt_overlap *)0)

/* This record structure doesn't have to correspond exactly to the above list
 * of available values, but it needs to retain sufficient information to
 * support the ability to generate all of them upon request. */
struct nirt_output_record {
    point_t orig;
    fastf_t h;
    fastf_t v;
    fastf_t d_orig;
    vect_t dir;
    fastf_t a;
    fastf_t e;
    point_t in;
    fastf_t d_in;
    point_t out;
    fastf_t d_out;
    fastf_t los;
    fastf_t scaled_los;
    struct bu_vls *path_name;
    struct bu_vls *reg_name;
    int reg_id;
    fastf_t obliq_in;
    fastf_t obliq_out;
    vect_t nm_in;
    fastf_t nm_d_in;
    fastf_t nm_h_in;
    fastf_t nm_v_in;
    vect_t nm_out;
    fastf_t nm_d_out;
    fastf_t nm_h_out;
    fastf_t nm_v_out;
    struct bu_vls *ov_reg1_name;
    int ov_reg1_id;
    struct bu_vls *ov_reg2_name;
    int ov_reg2_id;
    struct bu_vls *ov_sol_in;
    struct bu_vls *ov_sol_out;
    fastf_t ov_los;
    point_t ov_in;
    fastf_t ov_d_in;
    point_t ov_out;
    fastf_t ov_d_out;
    int surf_num_in;
    int surf_num_out;
    int claimant_count;
    struct bu_vls *claimant_list;
    struct bu_vls *claimant_listn; /* uses \n instead of space to separate claimants */
    struct bu_vls *attributes;
    point_t gap_in;
    fastf_t gap_los;
    struct nirt_overlap ovlp_list;
};



struct nirt_state_impl {
    /* Output options */
    struct bu_color *hit_odd_color;
    struct bu_color *hit_even_color;
    struct bu_color *void_color;
    struct bu_color *overlap_color;
    int print_header;
    int print_ident_flag;
    unsigned int rt_debug;
    unsigned int nirt_debug;

    /* Output */
    struct bu_vls *out;
    int out_accumulate;
    struct bu_vls *msg;
    struct bu_vls *err;
    int err_accumulate;
    struct bu_list s_vlist; /* used by the segs vlblock */
    struct bn_vlblock *segs;
    int plot_overlaps;
    //TODO - int segs_accumulate;
    int ret;  // return code to be returned by nirt_exec after execution

    /* Callbacks */
    nirt_hook_t h_state;   // any state change
    nirt_hook_t h_out;     // output
    nirt_hook_t h_msg;     // messages (not errors) not part of command output
    nirt_hook_t h_err;     // err changes
    nirt_hook_t h_segs;    // segement list is changed
    nirt_hook_t h_objs;    // active list of objects in scene changes
    nirt_hook_t h_frmts;   // output formatting is changed
    nirt_hook_t h_view;    // the camera view is changed


    /* state variables */
    double base2local;
    double local2base;
    int backout;
    int overlap_claims;
    int use_air;
    std::set<std::string> attrs;        // active attributes
    std::vector<std::string> active_paths; // active paths for raytracer
    struct nirt_output_record *vals;

    /* state alteration flags */
    bool b_state;   // updated for any state change
    bool b_segs;    // updated when segement list is changed
    bool b_objs;    // updated when active list of objects in scene changes
    bool b_frmts;   // updated when output formatting is changed
    bool b_view;    // updated when the camera view is changed

    /* internal containers for raytracing */
    struct application *ap;
    struct db_i *dbip;
    /* Note: Parallel structures are needed for operation w/ and w/o air */
    struct rt_i *rtip;
    struct resource *res;
    struct rt_i *rtip_air;
    struct resource *res_air;
    int need_reprep;

    /* internal format specifier arrays */
    struct bu_attribute_value_set *val_types;
    struct bu_attribute_value_set *val_docs;
    std::vector<std::pair<std::string,std::string> > fmt_ray;
    std::vector<std::pair<std::string,std::string> > fmt_head;
    std::vector<std::pair<std::string,std::string> > fmt_part;
    std::vector<std::pair<std::string,std::string> > fmt_foot;
    std::vector<std::pair<std::string,std::string> > fmt_miss;
    std::vector<std::pair<std::string,std::string> > fmt_ovlp;
    std::vector<std::pair<std::string,std::string> > fmt_gap;

    void *u_data; // user data
};

/**************************
 * Internal functionality *
 **************************/

HIDDEN void nmsg(struct nirt_state *nss, const char *fmt, ...) _BU_ATTR_PRINTF23;
HIDDEN void nmsg(struct nirt_state *nss, const char *fmt, ...)
{
    va_list ap;
    if (nss->i->h_msg) {
	struct bu_vls *vls = nss->i->msg;
	BU_CK_VLS(vls);
	va_start(ap, fmt);
	bu_vls_vprintf(vls, fmt, ap);
	(*nss->i->h_msg)(nss, nss->i->u_data);
	bu_vls_trunc(vls, 0);
    }
    va_end(ap);
}

HIDDEN void nout(struct nirt_state *nss, const char *fmt, ...) _BU_ATTR_PRINTF23;
HIDDEN void nout(struct nirt_state *nss, const char *fmt, ...)
{
    va_list ap;
    if (nss->i->h_out) {
	struct bu_vls *vls = nss->i->out;
	BU_CK_VLS(vls);
	va_start(ap, fmt);
	bu_vls_vprintf(vls, fmt, ap);
	(*nss->i->h_out)(nss, nss->i->u_data);
	bu_vls_trunc(vls, 0);
    }
    va_end(ap);
}

HIDDEN void nerr(struct nirt_state *nss, const char *fmt, ...) _BU_ATTR_PRINTF23;
HIDDEN void nerr(struct nirt_state *nss, const char *fmt, ...)
{
    va_list ap;
    if (nss->i->h_err){
	struct bu_vls *vls = nss->i->err;
	BU_CK_VLS(vls);
	va_start(ap, fmt);
	bu_vls_vprintf(vls, fmt, ap);
	(*nss->i->h_err)(nss, nss->i->u_data);
	bu_vls_trunc(vls, 0);
    }
    va_end(ap);
}

HIDDEN void ndbg(struct nirt_state *nss, int flag, const char *fmt, ...)
{
    va_list ap;
    if ((nss->i->nirt_debug & flag) && nss->i->h_err) {
	struct bu_vls *vls = nss->i->out;
	BU_CK_VLS(vls);
	va_start(ap, fmt);
	bu_vls_vprintf(vls, fmt, ap);
	(*nss->i->h_err)(nss, nss->i->u_data);
	bu_vls_trunc(vls, 0);
    }
    va_end(ap);
}

HIDDEN size_t
_nirt_find_first_unescaped(std::string &s, const char *keys, int offset)
{
    int off = offset;
    int done = 0;
    size_t candidate = std::string::npos;
    while (!done) {
	candidate = s.find_first_of(keys, off);
	if (!candidate || candidate == std::string::npos) return candidate;
	if (s.at(candidate - 1) == '\\') {
	    off = candidate + 1;
	} else {
	    done = 1;
	}
    }
    return candidate;
}

HIDDEN size_t
_nirt_find_first_unquoted(std::string &ts, const char *key, size_t offset)
{
    size_t q_start, q_end, pos;
    std::string s = ts;
    /* Start by initializing the position markers for quoted substrings. */
    q_start = _nirt_find_first_unescaped(s, "\"", offset);
    q_end = (q_start != std::string::npos) ? _nirt_find_first_unescaped(s, "\"", q_start + 1) : std::string::npos;

    pos = offset;
    while ((pos = s.find(key, pos)) != std::string::npos) {
	/* If we're inside matched quotes, only an un-escaped quote char means
	 * anything */
	if (q_end != std::string::npos && pos > q_start && pos < q_end) {
	    pos = q_end + 1;
	    q_start = _nirt_find_first_unescaped(s, "\"", q_end + 1);
	    q_end = (q_start != std::string::npos) ? _nirt_find_first_unescaped(s, "'\"", q_start + 1) : std::string::npos;
	    continue;
	}
	break;
    }
    return pos;
}

HIDDEN std::vector<std::string>
_nirt_string_split(std::string s)
{
    std::vector<std::string> substrs;
    std::string lstr = s;
    size_t pos = 0;
    while ((pos = _nirt_find_first_unquoted(lstr, ",", 0)) != std::string::npos) {
	std::string ss = lstr.substr(0, pos);
	substrs.push_back(ss);
	lstr.erase(0, pos + 1);
    }
    substrs.push_back(lstr);
    return substrs;
}

HIDDEN void
_nirt_trim_whitespace(std::string &s)
{
    size_t ep = s.find_last_not_of(" \t");
    size_t sp = s.find_first_not_of(" \t");
    if (sp == std::string::npos) {
	s.clear();
	return;
    }
    s = s.substr(sp, ep+1);
}

#if 0
HIDDEN std::string
_nirt_dbl_to_str(double d)
{
    // TODO - once we enable C++ 11 switch the precision below to std::numeric_limits<double>::max_digits10
    size_t prec = std::numeric_limits<double>::digits10 + 2;
    bu_log("prec: %d\n",prec);
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(prec) << d;
    std::string sd = ss.str();
    bu_log("dbl   : %.17f\nstr   : %s\n", d, sd.c_str());
    return sd;
}
#endif

HIDDEN double 
_nirt_str_to_dbl(std::string s)
{
    double d;
    // TODO - once we enable C++ 11 switch the precision below to std::numeric_limits<double>::max_digits10
    size_t prec = std::numeric_limits<double>::digits10 + 2;
    std::stringstream ss(s);
    ss >> std::setprecision(prec) >> std::fixed >> d;
    //bu_log("str   : %s\ndbl   : %.17f\n", s.c_str(), d);
    return d;
}
/********************************
 * Conversions and Calculations *
 ********************************/

HIDDEN void _nirt_grid2targ(struct nirt_state *nss)
{
    double ar = nss->i->vals->a * DEG2RAD;
    double er = nss->i->vals->e * DEG2RAD;
    nss->i->vals->orig[X] = - nss->i->vals->h * sin(ar) - nss->i->vals->v * cos(ar) * sin(er) + nss->i->vals->d_orig * cos(ar) * cos(er);
    nss->i->vals->orig[Y] =   nss->i->vals->h * cos(ar) - nss->i->vals->v * sin(ar) * sin(er) + nss->i->vals->d_orig * sin(ar) * cos(er);
    nss->i->vals->orig[Z] =   nss->i->vals->v * cos(er) + nss->i->vals->d_orig * sin(er);
}

HIDDEN void _nirt_targ2grid(struct nirt_state *nss)
{
    double ar = nss->i->vals->a * DEG2RAD;
    double er = nss->i->vals->e * DEG2RAD;
    nss->i->vals->h = - nss->i->vals->orig[X] * sin(ar) + nss->i->vals->orig[Y] * cos(ar);
    nss->i->vals->v = - nss->i->vals->orig[X] * cos(ar) * sin(er) - nss->i->vals->orig[Y] * sin(er) * sin(ar) + nss->i->vals->orig[Z] * cos(er);
    nss->i->vals->d_orig =   nss->i->vals->orig[X] * cos(er) * cos(ar) + nss->i->vals->orig[Y] * cos(er) * sin(ar) + nss->i->vals->orig[Z] * sin(er);
}

HIDDEN void _nirt_dir2ae(struct nirt_state *nss)
{
    int zeroes = ZERO(nss->i->vals->dir[Y]) && ZERO(nss->i->vals->dir[X]);
    double square = sqrt(nss->i->vals->dir[X] * nss->i->vals->dir[X] + nss->i->vals->dir[Y] * nss->i->vals->dir[Y]);

    nss->i->vals->a = zeroes ? 0.0 : atan2 (-(nss->i->vals->dir[Y]), -(nss->i->vals->dir[X])) / DEG2RAD;
    nss->i->vals->e = atan2(-(nss->i->vals->dir[Z]), square) / DEG2RAD;
}

HIDDEN void _nirt_ae2dir(struct nirt_state *nss)
{
    vect_t dir;
    double ar = nss->i->vals->a * DEG2RAD;
    double er = nss->i->vals->e * DEG2RAD;

    dir[X] = -cos(ar) * cos(er);
    dir[Y] = -sin(ar) * cos(er);
    dir[Z] = -sin(er);
    VUNITIZE(dir);
    VMOVE(nss->i->vals->dir, dir);
}

HIDDEN double _nirt_backout(struct nirt_state *nss)
{
    double bov;
    point_t ray_point;
    vect_t diag, dvec, ray_dir, center_bsphere;
    fastf_t bsphere_diameter, dist_to_target, delta;

    if (!nss || !nss->i->backout) return 0.0;

    VMOVE(ray_point, nss->i->vals->orig);
    VMOVE(ray_dir, nss->i->vals->dir);

    VSUB2(diag, nss->i->ap->a_rt_i->mdl_max, nss->i->ap->a_rt_i->mdl_min);
    bsphere_diameter = MAGNITUDE(diag);

    /*
     * calculate the distance from a plane normal to the ray direction through the center of
     * the bounding sphere and a plane normal to the ray direction through the aim point.
     */
    VADD2SCALE(center_bsphere, nss->i->ap->a_rt_i->mdl_max, nss->i->ap->a_rt_i->mdl_min, 0.5);

    dist_to_target = DIST_PT_PT(center_bsphere, ray_point);

    VSUB2(dvec, ray_point, center_bsphere);
    VUNITIZE(dvec);
    delta = dist_to_target*VDOT(ray_dir, dvec);

    /*
     * this should put us about a bounding sphere radius in front of the bounding sphere
     */
    bov = bsphere_diameter + delta;

    return bov;
}

HIDDEN fastf_t
_nirt_get_obliq(fastf_t *ray, fastf_t *normal)
{
    fastf_t cos_obl;
    fastf_t obliquity;

    cos_obl = fabs(VDOT(ray, normal) / (MAGNITUDE(normal) * MAGNITUDE(ray)));
    if (cos_obl < 1.001) {
	if (cos_obl > 1)
	    cos_obl = 1;
	obliquity = acos(cos_obl);
    } else {
	fflush(stdout);
	fprintf (stderr, "Error:  cos(obliquity) > 1 (%g)\n", cos_obl);
	obliquity = 0;
	bu_exit(1, NULL);
    }

    /* convert obliquity to degrees */
    obliquity = fabs(obliquity * RAD2DEG);
    if (obliquity > 90 && obliquity <= 180)
	obliquity = 180 - obliquity;
    else if (obliquity > 180 && obliquity <= 270)
	obliquity = obliquity - 180;
    else if (obliquity > 270 && obliquity <= 360)
	obliquity = 360 - obliquity;

    return obliquity;
}

/********************
 * Raytracing setup *
 ********************/
HIDDEN struct rt_i *
_nirt_get_rtip(struct nirt_state *nss)
{
    if (!nss || nss->i->dbip == DBI_NULL || nss->i->active_paths.size() == 0) return NULL;

    if (nss->i->use_air) {
	if (nss->i->rtip_air == RTI_NULL) {
	    nss->i->rtip_air = rt_new_rti(nss->i->dbip); /* clones dbip, so we can operate on the copy */
	    nss->i->rtip_air->rti_dbip->dbi_fp = fopen(nss->i->dbip->dbi_filename, "rb"); /* get read-only fp */
	    if (nss->i->rtip_air->rti_dbip->dbi_fp == NULL) {
		rt_free_rti(nss->i->rtip_air);
		nss->i->rtip_air = RTI_NULL;
		return RTI_NULL;
	    }
	    nss->i->rtip_air->rti_dbip->dbi_read_only = 1;
	    rt_init_resource(nss->i->res_air, 0, nss->i->rtip_air);
	    nss->i->rtip_air->useair = 1;
	}
	return nss->i->rtip_air;
    }

    if (nss->i->rtip == RTI_NULL) {
	nss->i->rtip = rt_new_rti(nss->i->dbip); /* clones dbip, so we can operate on the copy */
	nss->i->rtip->rti_dbip->dbi_fp = fopen(nss->i->dbip->dbi_filename, "rb"); /* get read-only fp */
	if (nss->i->rtip->rti_dbip->dbi_fp == NULL) {
	    rt_free_rti(nss->i->rtip);
	    nss->i->rtip = RTI_NULL;
	    return RTI_NULL;
	}
	nss->i->rtip->rti_dbip->dbi_read_only = 1;
	rt_init_resource(nss->i->res, 0, nss->i->rtip);
    }
    return nss->i->rtip;
}

HIDDEN struct resource *
_nirt_get_resource(struct nirt_state *nss)
{
    if (!nss || nss->i->dbip == DBI_NULL || nss->i->active_paths.size() == 0) return NULL;
    if (nss->i->use_air) return nss->i->res_air;
    return nss->i->res;
}

HIDDEN int
_nirt_raytrace_prep(struct nirt_state *nss)
{
    int ocnt = 0;
    int acnt = 0;
    std::set<std::string>::iterator s_it;

    /* Based on current settings, pick the particular rtip */
    nss->i->ap->a_rt_i = _nirt_get_rtip(nss);
    nss->i->ap->a_resource = _nirt_get_resource(nss);

    /* Don't have enough info to prep yet - can happen if we're in a pre "nirt_init" state */
    if (!nss->i->ap->a_rt_i) return 0;

    // Prepare C-style arrays for rt prep
    const char **objs = (const char **)bu_calloc(nss->i->active_paths.size() + 1, sizeof(char *), "objs");
    const char **attrs = (const char **)bu_calloc(nss->i->attrs.size() + 1, sizeof(char *), "attrs");
    for (size_t ui = 0; ui < nss->i->active_paths.size(); ui++) {
	const char *o = nss->i->active_paths[ui].c_str();
	objs[ocnt] = o;
	ocnt++;
    }
    objs[ocnt] = NULL;
    for (s_it = nss->i->attrs.begin(); s_it != nss->i->attrs.end(); s_it++) {
	const char *a = (*s_it).c_str();
	attrs[acnt] = a;
	acnt++;
    }
    attrs[acnt] = NULL;
    nmsg(nss, "Get trees...\n");
    rt_clean(nss->i->ap->a_rt_i);
    if (rt_gettrees_and_attrs(nss->i->ap->a_rt_i, attrs, ocnt, objs, 1)) {
	nerr(nss, "rt_gettrees() failed\n");
	bu_free(objs, "objs");
	bu_free(attrs, "objs");
	return -1;
    }
    bu_free(objs, "objs");
    bu_free(attrs, "objs");

    nmsg(nss, "Prepping the geometry...\n");
    rt_prep(nss->i->ap->a_rt_i);
    nmsg(nss, "%s ", (nss->i->active_paths.size() == 1) ? "Object" : "Objects");
    for (size_t i = 0; i < nss->i->active_paths.size(); i++) {
	if (i == 0) {
	    nmsg(nss, "'%s'", nss->i->active_paths[i].c_str());
	} else {
	    nmsg(nss, " '%s'", nss->i->active_paths[i].c_str());
	}
    }
    nmsg(nss, " processed\n");
    nss->i->need_reprep = 0;
    return 0;
}

/*****************************************
 * Routines to support output formatting *
 *****************************************/

/* Count how many format specifiers we might find in a format string */
HIDDEN unsigned int
_nirt_fmt_sp_cnt(const char *fmt) {
    unsigned int fcnt = 0;
    const char *uos = NULL;
    for (uos = fmt; *(uos + 1) != '\0'; ++uos) {
	if (*uos == '%' && (*(uos + 1) == '%')) continue;
	if (*uos == '%' && (*(uos + 1) != '\0')) {
	    fcnt++;
	}
    }
    return fcnt;
}


/* Validate the width specification portion of the format specifier against
 * the system limits and types supported by NIRT.*/
#define NIRT_PRINTF_FLAGS "-+# "
HIDDEN int
_nirt_fmt_sp_flags_check(struct nirt_state *nss, std::string &fmt_sp)
{
    size_t sp = fmt_sp.find_first_of(NIRT_PRINTF_FLAGS);
    size_t ep = fmt_sp.find_last_of(NIRT_PRINTF_FLAGS);
    if (sp == std::string::npos) return 0; // no flags is acceptable
    std::string flags = fmt_sp.substr(sp, ep - sp + 1);
    size_t invalid_char = flags.find_first_not_of(NIRT_PRINTF_FLAGS);
    if (invalid_char != std::string::npos) {
	nerr(nss, "Error: invalid characters in flags substring (\"%s\") of format specifier \"%s\"\n", flags.c_str(), fmt_sp.c_str());
	return -1;
    }
    return 0;
}

/* Validate the precision specification portion of the format specifier against
 * the system limits */
#define NIRT_PRINTF_PRECISION "0123456789."
#define NIRT_PRINTF_MAXWIDTH 1000 //Arbitrary sanity boundary for width specification
HIDDEN int
_nirt_fmt_sp_width_precision_check(struct nirt_state *nss, std::string &fmt_sp)
{
    size_t sp = fmt_sp.find_first_of(NIRT_PRINTF_PRECISION);
    size_t ep = fmt_sp.find_last_of(NIRT_PRINTF_PRECISION);
    if (sp == std::string::npos) return 0; // no width/precision settings is acceptable
    std::string p = fmt_sp.substr(sp, ep - sp + 1);
    size_t invalid_char = p.find_first_not_of(NIRT_PRINTF_PRECISION);
    if (invalid_char != std::string::npos) {
	nerr(nss, "Error: invalid characters in precision substring (\"%s\") of format specifier \"%s\"\n", p.c_str(), fmt_sp.c_str());
	return -1;
    }

    /* If we've got one or more numbers, check them for sanity */
    size_t ps = p.find_first_of(".");
    std::string wn, pn;
    if (ps != std::string::npos) {
	if (ps == 0) {
	    pn = p.substr(1, std::string::npos);
	} else {
	    pn = p.substr(ps + 1, std::string::npos);
	    wn = p.substr(0, ps);
	}
    } else {
	wn = p;
    }
    if (wn.length() > 0) {
	// Width check
	struct bu_vls optparse_msg = BU_VLS_INIT_ZERO;
	int w = 0;
	const char *wn_cstr = wn.c_str();
	if (bu_opt_int(&optparse_msg, 1, (const char **)&wn_cstr, (void *)&w) == -1) {
	    nerr(nss, "Error: bu_opt value read failure reading format specifier width from substring \"%s\" of specifier \"%s\": %s\n", wn.c_str(), fmt_sp.c_str(), bu_vls_addr(&optparse_msg));
	    bu_vls_free(&optparse_msg);
	    return -1;
	}
	bu_vls_free(&optparse_msg);
	if (w > NIRT_PRINTF_MAXWIDTH) {
	    nerr(nss, "Error: width specification in format specifier substring \"%s\" of specifier \"%s\" exceeds allowed max width (%d)\n", wn.c_str(), fmt_sp.c_str(), NIRT_PRINTF_MAXWIDTH);
	    return -1;
	} 
    }
    if (pn.length() > 0) {
	// Precision check
	struct bu_vls optparse_msg = BU_VLS_INIT_ZERO;
	int p_num = 0;
	const char *pn_cstr = pn.c_str();
	if (bu_opt_int(&optparse_msg, 1, (const char **)&pn_cstr, (void *)&p_num) == -1) {
	    nerr(nss, "Error: bu_opt value read failure reading format specifier precision from substring \"%s\" of specifier \"%s\": %s\n", pn.c_str(), fmt_sp.c_str(), bu_vls_addr(&optparse_msg));
	    bu_vls_free(&optparse_msg);
	    return -1;
	}
	bu_vls_free(&optparse_msg);

	switch (fmt_sp.c_str()[fmt_sp.length()-1]) {
	    case 'd':
	    case 'i':
		if (p_num > std::numeric_limits<int>::digits10) {
		    nerr(nss, "Error: precision specification in format specifier substring \"%s\" of specifier \"%s\" exceeds allowed maximum (%d)\n", pn.c_str(), fmt_sp.c_str(), std::numeric_limits<int>::digits10);
		    return -1;
		} 
		break;
	    case 'f':
	    case 'e':
	    case 'E':
	    case 'g':
	    case 'G':
		// TODO - once we enable C++ 11 switch the test below to (p > std::numeric_limits<fastf_t>::max_digits10) and update the nerr msg.
		if (p_num > std::numeric_limits<fastf_t>::digits10 + 2) {
		    nerr(nss, "Error: precision specification in format specifier substring \"%s\" of specifier \"%s\" exceeds allowed maximum (%d)\n", pn.c_str(), fmt_sp.c_str(), std::numeric_limits<fastf_t>::digits10 + 2);
		    return -1;
		}
		break;
	    case 's':
		if ((size_t)p_num > pn.max_size()) {
		    nerr(nss, "Error: precision specification in format specifier substring \"%s\" of specifier \"%s\" exceeds allowed maximum (%d)\n", pn.c_str(), fmt_sp.c_str(), pn.max_size());
		    return -1;
		}
		break;
	    default:
		nerr(nss, "Error: NIRT internal error in format width/precision validation.\n");
		return -1;
		break;
	}
    }
    return 0;
}

HIDDEN int
_nirt_fmt_sp_validate(struct nirt_state *nss, std::string &fmt_sp)
{
    // Sanity check any sub-specifiers
    if (_nirt_fmt_sp_flags_check(nss, fmt_sp)) return -1;
    if (_nirt_fmt_sp_width_precision_check(nss, fmt_sp)) return -1;
    return 0;
}

/* Processes the first format specifier.  (We use _nirt_split_fmt to break up a format string into
 * substrings with one format specifier per string - _nirt_fmt_sp_get's job is to extract the actual
 * format specifier from those substrings.) */
HIDDEN int
_nirt_fmt_sp_get(struct nirt_state *nss, const char *fmt, std::string &fmt_sp)
{
    int found = 0;
    const char *uos = NULL;
    if (!fmt) return -1;
    /* Find uncommented '%' format specifier */
    for (uos = fmt; *uos != '\0'; ++uos) {
	if (*uos == '%' && (*(uos + 1) == '%')) continue;
	if (*uos == '%' && (*(uos + 1) != '\0')) {
	    found++;
	    break;
	}
    }
    if (!found) {
	nerr(nss, "Error: could not find format specifier in fmt substring \"%s\"\n", fmt);
	return -1;
    }

    // Find the terminating character of the specifier and build
    // the substring.
    std::string wfmt(uos);
    size_t ep = wfmt.find_first_of(NIRT_PRINTF_SPECIFIERS);
    if (ep == std::string::npos) {
	nerr(nss, "Error: could not find valid format specifier terminator in fmt substring \"%s\" - candidates are \"%s\"\n", fmt, NIRT_PRINTF_SPECIFIERS);
	return -1;
    }
    fmt_sp = wfmt.substr(0, ep+1);

    // Check for unsupported fmt features
    size_t invalid_char;
    invalid_char = fmt_sp.find_first_of("*");
    if (invalid_char != std::string::npos) {
	nerr(nss, "Error: NIRT format specifiers do not support wildcard ('*') characters for width or precision: \"%s\"\n", fmt_sp.c_str());
	fmt_sp.clear();
	return -1;
    }
    invalid_char = fmt_sp.find_first_of("hljztL");
    if (invalid_char != std::string::npos) {
	nerr(nss, "Error: NIRT format specifiers do not support length sub-specifiers: \"%s\"\n", fmt_sp.c_str());
	fmt_sp.clear();
	return -1;
    }

    return 0;
}

/* Given a key and a format specifier string, use the NIRT type to check that the
 * supplied key is an appropriate input for that specifier. */
HIDDEN int
_nirt_fmt_sp_key_check(struct nirt_state *nss, const char *key, std::string &fmt_sp)
{
    int key_ok = 1;
    const char *type = NULL;

    if (!nss || !nss->i->val_types || fmt_sp.length() == 0) {
	nerr(nss, "Internal NIRT format processing error.\n");
	return -1;
    }

    if (!key || strlen(key) == 0) {
	nerr(nss, "Format specifier \"%s\" has no matching NIRT value key\n", fmt_sp.c_str());
	return -1;
    }

    type = bu_avs_get(nss->i->val_types, key);
    if (!type) {
	nerr(nss, "Key \"%s\" supplied to format specifier \"%s\" is not a valid NIRT value key\n", key, fmt_sp.c_str());
	return -1;
    }

    switch (fmt_sp.c_str()[fmt_sp.length()-1]) {
	case 'd':
	case 'i':
	    if (!BU_STR_EQUAL(type, "INT")) key_ok = 0;
	    break;
	case 'f':
	case 'e':
	case 'E':
	case 'g':
	case 'G':
	    if (!BU_STR_EQUAL(type, "FLOAT") && !BU_STR_EQUAL(type, "FNOUNIT")) key_ok = 0;
	    break;
	case 's':
	    if (!BU_STR_EQUAL(type, "STRING")) key_ok = 0;
	    break;
	default:
	    nerr(nss, "Error: format specifier terminator \"%s\" is invalid/unsupported: \"%c\" \n", fmt_sp.c_str(), fmt_sp.c_str()[fmt_sp.length()-1]);
	    return -1;
	    break;
    }

    if (!key_ok) {
	nerr(nss, "Error: NIRT value key \"%s\" has type %s, which does not match the type expected by format specifier \"%s\" \n", key, bu_avs_get(nss->i->val_types, key), fmt_sp.c_str());
	return -1;
    }

    return 0;
}

/* Given a format string, produce an array of substrings each of which contain
 * at most one format specifier */
HIDDEN int
_nirt_split_fmt(const char *ofmt, char ***breakout)
{
    int i = 0;
    int fcnt = 0;
    const char *up = NULL;
    const char *uos = NULL;
    char **fstrs = NULL;
    struct bu_vls specifier = BU_VLS_INIT_ZERO;
    if (!ofmt) return -1;
    char *cfmt = bu_strdup(ofmt);
    char *fmt = cfmt;

    /* Count maximum possible number of format specifiers.  We have at most
     * fcnt+1 "units" to handle */
    fcnt = _nirt_fmt_sp_cnt(fmt);
    fstrs = (char **)bu_calloc(fcnt + 1, sizeof(const char *), "output formatters");

    /* initialize to just after the initial '"' char */
    uos = (fmt[0] == '"') ? fmt + 1 : fmt;
    if (fmt[strlen(fmt)-1] == '"') fmt[strlen(fmt)-1] = '\0';

    /* Find one specifier per substring breakout */
    fcnt = 0;
    while (*uos != '\0') {
	int have_specifier= 0;
	up = uos;
	/* Find first '%' format specifier*/
	for (; *uos != '\0'; ++uos) {
	    if (*uos == '%' && (*(uos + 1) == '%')) continue;
	    if (*uos == '%' && have_specifier) break;
	    if (*uos == '%' && !have_specifier) have_specifier = 1;
	}

	/* Store the format. */
	bu_vls_trunc(&specifier, 0);
	while (up != uos) {
	    if (*up == '\\') {
		switch (*(up + 1)) {
		    case 'n':
			bu_vls_putc(&specifier, '\n');
			up += 2;
			break;
		    case '\042':
		    case '\047':
		    case '\134':
			bu_vls_printf(&specifier, "%c", *(up + 1));
			up += 2;
			break;
		    default:
			bu_vls_putc(&specifier, *up++);
			break;
		}
	    } else {
		bu_vls_putc(&specifier, *up++);
	    }
	}
	fstrs[fcnt] = bu_strdup(bu_vls_addr(&specifier));
	fcnt++;
    }
    if (fcnt) {
	(*breakout) = (char **)bu_calloc(fcnt, sizeof(const char *), "breakout");
	for (i = 0; i < fcnt; i++) {
	    (*breakout)[i] = fstrs[i];
	}
	bu_free(fstrs, "temp container");
	bu_vls_free(&specifier);
	bu_free(cfmt, "fmt cpy");
	return fcnt;
    } else {
	bu_free(fstrs, "temp container");
	bu_vls_free(&specifier);
	bu_free(cfmt, "fmt cpy");
	return -1;
    }
}

/* Given a vector breakout of a format string, generate the NIRT report string
 * displaying the full, assembled format string and listing NIRT keys (Items) */
HIDDEN void
_nirt_print_fmt_str(struct bu_vls *ostr, std::vector<std::pair<std::string,std::string> > &fmt_vect)
{
    std::string fmt_str, fmt_keys;
    std::vector<std::pair<std::string,std::string> >::iterator f_it;

    if (!ostr) return;

    for(f_it = fmt_vect.begin(); f_it != fmt_vect.end(); f_it++) {
	for (std::string::size_type i = 0; i < (*f_it).first.size(); ++i) {
	    if ((*f_it).first[i] == '\n')  {
		fmt_str.push_back('\\');
		fmt_str.push_back('n');
	    } else {
		fmt_str.push_back((*f_it).first[i]);
	    }
	}
	fmt_keys.append(" ");
	fmt_keys.append((*f_it).second);
    }
    bu_vls_sprintf(ostr, "Format: \"%s\"\nItem(s):%s", fmt_str.c_str(), fmt_keys.c_str());
    bu_vls_trimspace(ostr);
}

/* Given a vector breakout of a format string, generate the NIRT fmt command that
 * created it */
HIDDEN void
_nirt_print_fmt_cmd(struct bu_vls *ostr, char f, std::vector<std::pair<std::string,std::string> > &fmt_vect)
{
    std::string fmt_str, fmt_keys;
    std::vector<std::pair<std::string,std::string> >::iterator f_it;

    if (!ostr) return;

    for(f_it = fmt_vect.begin(); f_it != fmt_vect.end(); f_it++) {
	for (std::string::size_type i = 0; i < (*f_it).first.size(); ++i) {
	    if ((*f_it).first[i] == '\n')  {
		fmt_str.push_back('\\');
		fmt_str.push_back('n');
	    } else {
		fmt_str.push_back((*f_it).first[i]);
	    }
	}
	fmt_keys.append(" ");
	fmt_keys.append((*f_it).second);
    }
    bu_vls_sprintf(ostr, "fmt %c \"%s\"%s\n", f, fmt_str.c_str(), fmt_keys.c_str());
}

/* Translate NIRT fmt substrings into fully evaluated printf output, while also
 * handling units on key values.  This should never be called using any fmt
 * string that hasn't been validated by _nirt_fmt_sp_validate and _nirt_fmt_sp_key_check */
#define nirt_print_key(keystr,val) if (BU_STR_EQUAL(key, keystr)) bu_vls_printf(ostr, fmt, val)
void
_nirt_print_fmt_substr(struct bu_vls *ostr, const char *fmt, const char *key, struct nirt_output_record *r, fastf_t base2local)
{
    if (!ostr || !fmt || !r) return;
    if (!key || !strlen(key)) {
	bu_vls_printf(ostr, fmt);
	return;
    }
    nirt_print_key("x_orig", r->orig[X] * base2local);
    nirt_print_key("y_orig", r->orig[Y] * base2local);
    nirt_print_key("z_orig", r->orig[Z] * base2local);
    nirt_print_key("h", r->h * base2local);
    nirt_print_key("v", r->v * base2local);
    nirt_print_key("d_orig", r->d_orig * base2local);
    nirt_print_key("x_dir", r->dir[X]);
    nirt_print_key("y_dir", r->dir[Y]);
    nirt_print_key("z_dir", r->dir[Z]);
    nirt_print_key("a", r->a);
    nirt_print_key("e", r->e);
    nirt_print_key("x_in", r->in[X] * base2local);
    nirt_print_key("y_in", r->in[Y] * base2local);
    nirt_print_key("z_in", r->in[Z] * base2local);
    nirt_print_key("d_in", r->d_in * base2local);
    nirt_print_key("x_out", r->out[X] * base2local);
    nirt_print_key("y_out", r->out[Y] * base2local);
    nirt_print_key("z_out", r->out[Z] * base2local);
    nirt_print_key("d_out", r->d_out * base2local);
    nirt_print_key("los", r->los * base2local);
    nirt_print_key("scaled_los", r->scaled_los * base2local);
    nirt_print_key("path_name", bu_vls_addr(r->path_name));
    nirt_print_key("reg_name", bu_vls_addr(r->reg_name));
    nirt_print_key("reg_id", r->reg_id);
    nirt_print_key("obliq_in", r->obliq_in);
    nirt_print_key("obliq_out", r->obliq_out);
    nirt_print_key("nm_x_in", r->nm_in[X]);
    nirt_print_key("nm_y_in", r->nm_in[Y]);
    nirt_print_key("nm_z_in", r->nm_in[Z]);
    nirt_print_key("nm_d_in", r->nm_d_in);
    nirt_print_key("nm_h_in", r->nm_h_in);
    nirt_print_key("nm_v_in", r->nm_v_in);
    nirt_print_key("nm_x_out", r->nm_out[X]);
    nirt_print_key("nm_y_out", r->nm_out[Y]);
    nirt_print_key("nm_z_out", r->nm_out[Z]);
    nirt_print_key("nm_d_out", r->nm_d_out);
    nirt_print_key("nm_h_out", r->nm_h_out);
    nirt_print_key("nm_v_out", r->nm_v_out);
    nirt_print_key("ov_reg1_name", bu_vls_addr(r->ov_reg1_name));
    nirt_print_key("ov_reg1_id", r->ov_reg1_id);
    nirt_print_key("ov_reg2_name", bu_vls_addr(r->ov_reg2_name));
    nirt_print_key("ov_reg2_id", r->ov_reg2_id);
    nirt_print_key("ov_sol_in", bu_vls_addr(r->ov_sol_in));
    nirt_print_key("ov_sol_out", bu_vls_addr(r->ov_sol_out));
    nirt_print_key("ov_los", r->ov_los * base2local);
    nirt_print_key("ov_x_in", r->ov_in[X] * base2local);
    nirt_print_key("ov_y_in", r->ov_in[Y] * base2local);
    nirt_print_key("ov_z_in", r->ov_in[Z] * base2local);
    nirt_print_key("ov_d_in", r->ov_d_in * base2local);
    nirt_print_key("ov_x_out", r->ov_out[X] * base2local);
    nirt_print_key("ov_y_out", r->ov_out[Y] * base2local);
    nirt_print_key("ov_z_out", r->ov_out[Z] * base2local);
    nirt_print_key("ov_d_out", r->ov_d_out * base2local);
    nirt_print_key("surf_num_in", r->surf_num_in);
    nirt_print_key("surf_num_out", r->surf_num_out);
    nirt_print_key("claimant_count", r->claimant_count);
    nirt_print_key("claimant_list", bu_vls_addr(r->claimant_list));
    nirt_print_key("claimant_listn", bu_vls_addr(r->claimant_listn));
    nirt_print_key("attributes", bu_vls_addr(r->attributes));
    nirt_print_key("x_gap_in", r->gap_in[X] * base2local);
    nirt_print_key("y_gap_in", r->gap_in[Y] * base2local);
    nirt_print_key("z_gap_in", r->gap_in[Z] * base2local);
    nirt_print_key("gap_los", r->gap_los * base2local);
}

/* Generate the full report string defined by the array of fmt,key pairs
 * associated with the supplied type, based on current values */
HIDDEN void
_nirt_report(struct nirt_state *nss, char type, struct nirt_output_record *r)
{
    struct bu_vls rstr = BU_VLS_INIT_ZERO;
    std::vector<std::pair<std::string,std::string> > *fmt_vect = NULL;
    std::vector<std::pair<std::string,std::string> >::iterator f_it;

    switch (type) {
	case 'r':
	    fmt_vect = &nss->i->fmt_ray;
	    break;
	case 'h':
	    fmt_vect = &nss->i->fmt_head;
	    break;
	case 'p':
	    fmt_vect = &nss->i->fmt_part;
	    break;
	case 'f':
	    fmt_vect = &nss->i->fmt_foot;
	    break;
	case 'm':
	    fmt_vect = &nss->i->fmt_miss;
	    break;
	case 'o':
	    fmt_vect = &nss->i->fmt_ovlp;
	    break;
	case 'g':
	    fmt_vect = &nss->i->fmt_gap;
	    break;
	default:
	    nerr(nss, "Error: NIRT internal error, attempt to report using unknown fmt type %c\n", type);
	    return;
    }
    for(f_it = fmt_vect->begin(); f_it != fmt_vect->end(); f_it++) {
	const char *key = (*f_it).second.c_str();
	const char *fmt = (*f_it).first.c_str();
	_nirt_print_fmt_substr(&rstr, fmt, key, r, nss->i->base2local);
    }
    nout(nss, "%s", bu_vls_addr(&rstr));
    bu_vls_free(&rstr);
}


/************************
 * Raytracing Callbacks *
 ************************/

HIDDEN struct nirt_overlap *
_nirt_find_ovlp(struct nirt_state *nss, struct partition *pp)
{
    struct nirt_overlap *op;

    for (op = nss->i->vals->ovlp_list.forw; op != &(nss->i->vals->ovlp_list); op = op->forw) {
	if (((pp->pt_inhit->hit_dist <= op->in_dist)
		    && (op->in_dist <= pp->pt_outhit->hit_dist)) ||
		((pp->pt_inhit->hit_dist <= op->out_dist)
		 && (op->in_dist <= pp->pt_outhit->hit_dist)))
	    break;
    }
    return (op == &(nss->i->vals->ovlp_list)) ? NIRT_OVERLAP_NULL : op;
}


HIDDEN void
_nirt_del_ovlp(struct nirt_overlap *op)
{
    op->forw->backw = op->backw;
    op->backw->forw = op->forw;
    bu_free((char *)op, "free op in del_ovlp");
}

HIDDEN void
_nirt_init_ovlp(struct nirt_state *nss)
{
    nss->i->vals->ovlp_list.forw = nss->i->vals->ovlp_list.backw = &(nss->i->vals->ovlp_list);
}


extern "C" int
_nirt_if_hit(struct application *ap, struct partition *part_head, struct seg *UNUSED(finished_segs))
{
    struct bu_list *vhead;
    struct nirt_state *nss = (struct nirt_state *)ap->a_uptr;
    struct nirt_output_record *vals = nss->i->vals;
    fastf_t ar = nss->i->vals->a * DEG2RAD;
    fastf_t er = nss->i->vals->e * DEG2RAD;
    int part_nm = 0;
    struct nirt_overlap *ovp;
    point_t inormal;
    point_t onormal;
    struct partition *part;
    int ev_odd = 1; /* first partition is colored as "odd" */

    _nirt_report(nss, 'r', vals);
    _nirt_report(nss, 'h', vals);

    if (nss->i->overlap_claims == NIRT_OVLP_REBUILD_FASTGEN) {
	rt_rebuild_overlaps(part_head, ap, 1);
    } else if (nss->i->overlap_claims == NIRT_OVLP_REBUILD_ALL) {
	rt_rebuild_overlaps(part_head, ap, 0);
    }

    for (part = part_head->pt_forw; part != part_head; part = part->pt_forw) {
	++part_nm;

	RT_HIT_NORMAL(inormal, part->pt_inhit, part->pt_inseg->seg_stp,
		&ap->a_ray, part->pt_inflip);
	RT_HIT_NORMAL(onormal, part->pt_outhit, part->pt_outseg->seg_stp,
		&ap->a_ray, part->pt_outflip);

	/* Update the output values */
	if (part_nm > 1) {
	    VMOVE(vals->gap_in, vals->out);
	    // Stash the previous d_out value for use after we calculate the new d_in
	    vals->gap_los = vals->d_out;
	}

	VMOVE(vals->in, part->pt_inhit->hit_point);
	VMOVE(vals->out, part->pt_outhit->hit_point);
	VMOVE(vals->nm_in, inormal);
	VMOVE(vals->nm_out, onormal);

	ndbg(nss, NIRT_DEBUG_HITS, "Partition %d entry: (%g, %g, %g) exit: (%g, %g, %g)\n",
		part_nm, V3ARGS(vals->in), V3ARGS(vals->out));

	vals->d_in = vals->in[X] * cos(er) * cos(ar) + vals->in[Y] * cos(er) * sin(ar) + vals->in[Z] * sin(er);
	vals->d_out = vals->out[X] * cos(er) * cos(ar) + vals->out[Y] * cos(er) * sin(ar) + vals->out[Z] * sin(er);

	vals->nm_d_in = vals->nm_in[X] * cos(er) * cos(ar) + vals->nm_in[Y] * cos(er) * sin(ar) + vals->nm_in[Z] * sin(er);
	vals->nm_h_in = vals->nm_in[X] * (-sin(ar)) + vals->nm_in[Y] * cos(ar);
	vals->nm_v_in = vals->nm_in[X] * (-sin(er)) * cos(ar) + vals->nm_in[Y] * (-sin(er)) * sin(ar) + vals->nm_in[Z] * cos(er);

	vals->nm_d_out = vals->nm_out[X] * cos(er) * cos(ar) + vals->nm_out[Y] * cos(er) * sin(ar) + vals->nm_out[Z] * sin(er);
	vals->nm_h_out = vals->nm_out[X] * (-sin(ar)) + vals->nm_out[Y] * cos(ar);
	vals->nm_v_out = vals->nm_out[X] * (-sin(er)) * cos(ar) + vals->nm_out[Y] * (-sin(er)) * sin(ar) + vals->nm_out[Z] * cos(er);

	vals->los = vals->d_in - vals->d_out;
	vals->scaled_los = 0.01 * vals->los * part->pt_regionp->reg_los;

	if (part_nm > 1) {
	    /* old d_out - new d_in */
	    vals->gap_los = vals->gap_los - vals->d_in;

	    if (vals->gap_los > 0) {
		_nirt_report(nss, 'g', vals);
		/* vlist segment for gap */
		vhead = bn_vlblock_find(nss->i->segs, nss->i->void_color->buc_rgb[RED], nss->i->void_color->buc_rgb[GRN], nss->i->void_color->buc_rgb[BLU]);
		BN_ADD_VLIST(nss->i->segs->free_vlist_hd, vhead, vals->gap_in, BN_VLIST_LINE_MOVE);
		BN_ADD_VLIST(nss->i->segs->free_vlist_hd, vhead, vals->in, BN_VLIST_LINE_DRAW);
		nss->i->b_segs = true;
	    }
	}

	bu_vls_sprintf(vals->path_name, "%s", part->pt_regionp->reg_name);
	{
	    char *tmp_regname = (char *)bu_calloc(bu_vls_strlen(vals->path_name) + 1, sizeof(char), "tmp reg_name");
	    bu_basename(part->pt_regionp->reg_name, tmp_regname);
	    bu_vls_sprintf(vals->reg_name, "%s", tmp_regname);
	    bu_free(tmp_regname, "tmp reg_name");
	}

	vals->reg_id = part->pt_regionp->reg_regionid;
	vals->surf_num_in = part->pt_inhit->hit_surfno;
	vals->surf_num_out = part->pt_outhit->hit_surfno;
	vals->obliq_in = _nirt_get_obliq(ap->a_ray.r_dir, inormal);
	vals->obliq_out = _nirt_get_obliq(ap->a_ray.r_dir, onormal);

	bu_vls_trunc(vals->claimant_list, 0);
	bu_vls_trunc(vals->claimant_listn, 0);
	if (part->pt_overlap_reg == 0) {
	    vals->claimant_count = 1;
	} else {
	    struct region **rpp;
	    struct bu_vls tmpcp = BU_VLS_INIT_ZERO;
	    vals->claimant_count = 0;
	    for (rpp = part->pt_overlap_reg; *rpp != REGION_NULL; ++rpp) {
		char *base = NULL;
		if (vals->claimant_count) bu_vls_strcat(vals->claimant_list, " ");
		vals->claimant_count++;
		bu_vls_sprintf(&tmpcp, "%s", (*rpp)->reg_name);
		base = bu_basename(bu_vls_addr(&tmpcp), NULL);
		bu_vls_strcat(vals->claimant_list, base);
		bu_free(base, "bu_basename");
	    }
	    bu_vls_free(&tmpcp);

	    /* insert newlines instead of spaces for listn */
	    bu_vls_sprintf(vals->claimant_listn, "%s", bu_vls_addr(vals->claimant_list));
	    for (char *cp = bu_vls_addr(vals->claimant_listn); *cp != '\0'; ++cp) {
		if (*cp == ' ') *cp = '\n';
	    }
	}

	bu_vls_trunc(vals->attributes, 0);
	std::set<std::string>::iterator a_it;
	for (a_it = nss->i->attrs.begin(); a_it != nss->i->attrs.end(); a_it++) {
	    const char *key = (*a_it).c_str();
	    const char *val = bu_avs_get(&part->pt_regionp->attr_values, key);
	    if (val != NULL) {
		bu_vls_printf(vals->attributes, "%s=%s ", key, val);
	    }
	}

	_nirt_report(nss, 'p', vals);

	/* vlist segment for hit */
	if (ev_odd % 2) {
	    vhead = bn_vlblock_find(nss->i->segs, nss->i->hit_odd_color->buc_rgb[RED], nss->i->hit_odd_color->buc_rgb[GRN], nss->i->hit_odd_color->buc_rgb[BLU]);
	} else {
	    vhead = bn_vlblock_find(nss->i->segs, nss->i->hit_even_color->buc_rgb[RED], nss->i->hit_even_color->buc_rgb[GRN], nss->i->hit_even_color->buc_rgb[BLU]);
	}
	BN_ADD_VLIST(nss->i->segs->free_vlist_hd, vhead, vals->in, BN_VLIST_LINE_MOVE);
	BN_ADD_VLIST(nss->i->segs->free_vlist_hd, vhead, vals->out, BN_VLIST_LINE_DRAW);
	nss->i->b_segs = true;

	while ((ovp = _nirt_find_ovlp(nss, part)) != NIRT_OVERLAP_NULL) {

	    // TODO - do this more cleanly
	    char *copy_ovlp_reg1 = bu_strdup(ovp->reg1->reg_name);
	    char *copy_ovlp_reg2 = bu_strdup(ovp->reg2->reg_name);
	    char *t1 = (char *)bu_calloc(strlen(copy_ovlp_reg1), sizeof(char), "if_hit sval2");
	    bu_basename(copy_ovlp_reg1, t1);
	    bu_vls_sprintf(vals->ov_reg1_name, "%s", t1);
	    bu_free(t1, "t1");
	    bu_free(copy_ovlp_reg1, "cp1");
	    char *t2 = (char *)bu_calloc(strlen(copy_ovlp_reg2), sizeof(char), "if_hit sval3");
	    bu_basename(copy_ovlp_reg2, t2);
	    bu_vls_sprintf(vals->ov_reg2_name, "%s", t2);
	    bu_free(t2, "t2");
	    bu_free(copy_ovlp_reg2, "cp2");


	    vals->ov_reg1_id = ovp->reg1->reg_regionid;
	    vals->ov_reg2_id = ovp->reg2->reg_regionid;
	    bu_vls_sprintf(vals->ov_sol_in, "%s", part->pt_inseg->seg_stp->st_dp->d_namep);
	    bu_vls_sprintf(vals->ov_sol_out, "%s", part->pt_outseg->seg_stp->st_dp->d_namep);
	    VMOVE(vals->ov_in, ovp->in_point);
	    VMOVE(vals->ov_out, ovp->out_point);

	    vals->ov_d_in = vals->d_orig - ovp->in_dist; // TODO looks sketchy in NIRT - did they really mean target(D) ?? -> (VTI_XORIG + 3 -> VTI_H)
	    vals->ov_d_out = vals->d_orig - ovp->out_dist; // TODO looks sketchy in NIRT - did they really mean target(D) ?? -> (VTI_XORIG + 3 -> VTI_H)
	    vals->ov_los = vals->ov_d_in - vals->ov_d_out;

	    _nirt_report(nss, 'o', vals);

	    /* vlist segment for overlap */
	    if (nss->i->plot_overlaps) {
		vhead = bn_vlblock_find(nss->i->segs, nss->i->overlap_color->buc_rgb[RED], nss->i->overlap_color->buc_rgb[GRN], nss->i->overlap_color->buc_rgb[BLU]);
		BN_ADD_VLIST(nss->i->segs->free_vlist_hd, vhead, vals->ov_in, BN_VLIST_LINE_MOVE);
		BN_ADD_VLIST(nss->i->segs->free_vlist_hd, vhead, vals->ov_out, BN_VLIST_LINE_DRAW);
		nss->i->b_segs = true;
	    }

	    _nirt_del_ovlp(ovp);
	}
    }

    _nirt_report(nss, 'f', vals);

    if (vals->ovlp_list.forw != &(vals->ovlp_list)) {
	nerr(nss, "Previously unreported overlaps.  Shouldn't happen\n");
	ovp = vals->ovlp_list.forw;
	while (ovp != &(vals->ovlp_list)) {
	    nerr(nss, " OVERLAP:\n\t%s %s (%g %g %g) %g\n", ovp->reg1->reg_name, ovp->reg2->reg_name, V3ARGS(ovp->in_point), ovp->out_dist - ovp->in_dist);
	    ovp = ovp->forw;
	}
    }

    return HIT;
}

extern "C" int
_nirt_if_miss(struct application *ap)
{
    struct nirt_state *nss = (struct nirt_state *)ap->a_uptr;
    _nirt_report(nss, 'r', nss->i->vals);
    _nirt_report(nss, 'm', nss->i->vals);
    return MISS;
}

extern "C" int
_nirt_if_overlap(struct application *ap, struct partition *pp, struct region *reg1, struct region *reg2, struct partition *InputHdp)
{
    struct nirt_state *nss = (struct nirt_state *)ap->a_uptr;
    struct nirt_overlap *o;
    BU_ALLOC(o, struct nirt_overlap);
    o->ap = ap;
    o->pp = pp;
    o->reg1 = reg1;
    o->reg2 = reg2;
    o->in_dist = pp->pt_inhit->hit_dist;
    o->out_dist = pp->pt_outhit->hit_dist;
    VJOIN1(o->in_point, ap->a_ray.r_pt, pp->pt_inhit->hit_dist, ap->a_ray.r_dir);
    VJOIN1(o->out_point, ap->a_ray.r_pt, pp->pt_outhit->hit_dist, ap->a_ray.r_dir);

    /* Insert the new overlap into the list of overlaps */
    o->forw = nss->i->vals->ovlp_list.forw;
    o->backw = &(nss->i->vals->ovlp_list);
    o->forw->backw = o;
    nss->i->vals->ovlp_list.forw = o;

    /* Match current BRL-CAD default behavior */
    return rt_defoverlap (ap, pp, reg1, reg2, InputHdp);
}

/**************
 *  Commands  *
 **************/

struct nirt_cmd_desc {
    const char *cmd;  // Must match the eventual bu_cmtab key
    const char *desc;
    const char *args;
};

static const struct nirt_cmd_desc nirt_descs[] = {
    { "attr",           "select attributes",                             NULL },
    { "ae",             "set/query azimuth and elevation",               "azimuth elevation" },
    { "dir",            "set/query direction vector",                    "x-component y-component z-component" },
    { "diff",           "test a ray result against a supplied default",  "[-t tol] partition_info" },
    { "hv",             "set/query gridplane coordinates",               "horz vert [dist]" },
    { "xyz",            "set/query target coordinates",                  "X Y Z" },
    { "s",              "shoot a ray at the target",                     NULL },
    { "backout",        "back out of model",                             NULL },
    { "useair",         "set/query use of air",                          "<0|1|2|...>" },
    { "units",          "set/query local units",                         "<mm|cm|m|in|ft>" },
    { "overlap_claims", "set/query overlap rebuilding/retention",        "<0|1|2|3>" },
    { "fmt",            "set/query output formats",                      "{rhpfmog} format item item ..." },
    { "print",          "query an output item",                          "item" },
    { "bot_minpieces",  "Get/Set value for rt_bot_minpieces (0 means do not use pieces, default is 32)", "min_pieces" },
    { "libdebug",       "set/query librt debug flags",                   "hex_flag_value" },
    { "debug",          "set/query nirt debug flags",                    "hex_flag_value" },
    { "q",              "quit",                                          NULL },
    { "?",              "display this help menu",                        NULL },
    { (char *)NULL,     NULL,                                            NULL}
};

HIDDEN const char *
_nirt_get_desc_args(const char *key)
{
    const struct nirt_cmd_desc *d;
    for (d = nirt_descs; d->cmd; d++) {
	if (BU_STR_EQUAL(d->cmd, key)) return d->args;
    }
    return NULL;
}

extern "C" int
_nirt_cmd_attr(void *ns, int argc, const char *argv[])
{
    if (!ns) return -1;
    struct nirt_state *nss = (struct nirt_state *)ns;
    int i = 0;
    int ac = 0;
    int print_help = 0;
    int attr_print = 0;
    int attr_flush = 0;
    int val_attrs = 0;
    size_t orig_size = nss->i->attrs.size();
    struct bu_vls optparse_msg = BU_VLS_INIT_ZERO;
    struct bu_opt_desc d[5];
    BU_OPT(d[0],  "h", "help",  "",  NULL,   &print_help, "print help and exit");
    BU_OPT(d[1],  "p", "print", "",  NULL,   &attr_print, "print active attributes");
    BU_OPT(d[2],  "f", "flush", "",  NULL,   &attr_flush, "clear attribute list");
    BU_OPT(d[3],  "v", "",      "",  NULL,   &val_attrs,  "validate attributes - only accept attributes used by one or more objects in the active database");
    BU_OPT_NULL(d[4]);
    const char *help = bu_opt_describe(d, NULL);
    const char *ustr = "Usage: attr <opts> <attribute_name_1> <attribute_name_2> ...\nNote: attr with no options and no attributes to add will print the list of active attributes.\nOptions:";

    argv++; argc--;

    if ((ac = bu_opt_parse(&optparse_msg, argc, (const char **)argv, d)) == -1) {
	nerr(nss, "Error: bu_opt value read failure: %s\n\n%s\n%s\n", bu_vls_addr(&optparse_msg), ustr, help);
	if (help) bu_free((char *)help, "help str");
	bu_vls_free(&optparse_msg);
	return -1;
    }
    bu_vls_free(&optparse_msg);

    if (attr_flush) nss->i->attrs.clear();

    if (print_help || (attr_print && ac > 0)) {
	nerr(nss, "%s\n%s", ustr, help);
	if (help) bu_free((char *)help, "help str");
	return -1;
    }
    if (attr_print || ac == 0) {
	std::set<std::string>::iterator a_it;
	for (a_it = nss->i->attrs.begin(); a_it != nss->i->attrs.end(); a_it++) {
	    nout(nss, "\"%s\"\n", (*a_it).c_str());
	}
	return 0;
    }

    for (i = 0; i < ac; i++) {
	const char *sattr = db5_standard_attribute(db5_standardize_attribute(argv[i]));
	if (val_attrs) {
	    //TODO - check against the .g for any usage of the attribute
	}
	/* If there is a standardized version of this attribute that is
	 * different from the supplied version, activate it as well. */
	if (sattr) {
	    nss->i->attrs.insert(sattr);
	} else {
	    nss->i->attrs.insert(argv[i]);
	}
    }

    if (nss->i->attrs.size() != orig_size) {
	nss->i->need_reprep = 1;
    }
    return 0;
}

extern "C" int
_nirt_cmd_az_el(void *ns, int argc, const char *argv[])
{
    double az = 0.0;
    double el = 0.0;
    int ret = 0;
    struct bu_vls opt_msg = BU_VLS_INIT_ZERO;
    struct nirt_state *nss = (struct nirt_state *)ns;
    if (!ns) return -1;

    if (argc == 1) {
	nout(nss, "(az, el) = (%4.2f, %4.2f)\n", nss->i->vals->a, nss->i->vals->e);
	return 0;
    }

    argv++; argc--;

    if (argc != 2) {
	nerr(nss, "Usage:  ae %s\n", _nirt_get_desc_args("ae"));
	return -1;
    }

    if ((ret = bu_opt_fastf_t(&opt_msg, 1, argv, (void *)&az)) == -1) {
	nerr(nss, "Error: bu_opt value read failure: %s\n", bu_vls_addr(&opt_msg));
	goto azel_done;
    }

    if (fabs(az) > 360) {
	nerr(nss, "Error:  |azimuth| <= 360\n");
	ret = -1;
	goto azel_done;
    }

    argc--; argv++;
    if ((ret = bu_opt_fastf_t(&opt_msg, 1, argv, (void *)&el)) == -1) {
	nerr(nss, "%s\n", bu_vls_addr(&opt_msg));
	goto azel_done;
    }

    if (fabs(el) > 90) {
	nerr(nss, "Error:  |elevation| <= 90\n");
	ret = -1;
	goto azel_done;
    }

    nss->i->vals->a = az;
    nss->i->vals->e = el;
    _nirt_ae2dir(nss);
    ret = 0;

azel_done:
    bu_vls_free(&opt_msg);
    return ret;
}


extern "C" int
_nirt_cmd_diff(void *ns, int argc, const char *argv[])
{
    if (!ns) return -1;
    struct nirt_state *nss = (struct nirt_state *)ns;
    int ac = 0;
    int print_help = 0;
    double tol = 0;
    int have_ray = 0;
    struct bu_vls optparse_msg = BU_VLS_INIT_ZERO;
    struct bu_opt_desc d[3];
    // TODO - add reporting options for enabling/disabling region/path, partition length, normal, and overlap ordering diffs.
    // For example, if r1 and r2 have the same shape in space, we may want to suppress name based differences and look at
    // other aspects of the shotline.  Also need sorting options for output - max partition diff first, max normal diff first,
    // max numerical delta, etc...
    BU_OPT(d[0],  "h", "help",       "",             NULL,   &print_help, "print help and exit");
    BU_OPT(d[1],  "t", "tol",   "<val>",  &bu_opt_fastf_t,   &tol,        "set diff tolerance");
    BU_OPT_NULL(d[2]);
    const char *help = bu_opt_describe(d, NULL);
    const char *ustr = "Usage: diff [opts] shotfile";

    argv++; argc--;

    if ((ac = bu_opt_parse(&optparse_msg, argc, (const char **)argv, d)) == -1) {
	nerr(nss, "Error: bu_opt value read failure: %s\n\n%s\n%s\n", bu_vls_addr(&optparse_msg), ustr, help);
	if (help) bu_free((char *)help, "help str");
	bu_vls_free(&optparse_msg);
	return -1;
    }
    bu_vls_free(&optparse_msg);

    if (print_help || (ac != 1) || !argv[0]) {
	nerr(nss, "%s\n%s", ustr, help);
	if (help) bu_free((char *)help, "help str");
	return -1;
    }

    std::string line;
    std::ifstream ifs;
    ifs.open(argv[0]);
    if (!ifs.is_open()) {
	nerr(nss, "Error: could not open file %s\n", argv[0]);
	return -1;
    }

    while (std::getline(ifs, line)) {
	if (!line.compare(0, 1, "#")) continue; // Skip comments
	if (!line.compare(0, 5, "Ray: ")) {
	    if (have_ray) {
		// Have ray already - execute current ray, store results in
		// diff database (if any diffs were found), then clear expected
		// results and old ray
		bu_log("\n\nanother ray!\n\n\n");
	    }
	    // Read ray
	    vect_t dir;
	    point_t orig;
	    std::string rstr = line.substr(5);
	    have_ray = 1;
	    std::vector<std::string> substrs = _nirt_string_split(rstr);
	    if (substrs.size() != 6) {
		nerr(nss, "Error processing ray line \"%s\"!\nExpected 6 elements, found %d\n", line.c_str(), substrs.size());
		return -1;
	    }
	    bu_log("Found Ray:\n");
	    orig[X] = _nirt_str_to_dbl(substrs[0]);
	    orig[Y] = _nirt_str_to_dbl(substrs[1]);
	    orig[Z] = _nirt_str_to_dbl(substrs[2]);
	    dir[X] = _nirt_str_to_dbl(substrs[3]);
	    dir[Y] = _nirt_str_to_dbl(substrs[4]);
	    dir[Z] = _nirt_str_to_dbl(substrs[5]);
	    VMOVE(nss->i->vals->dir, dir);
	    VMOVE(nss->i->vals->orig, orig);
	    _nirt_targ2grid(nss);
	}
	if (!line.compare(0, 5, "HIT: ")) {
	    std::string hstr = line.substr(5);
	    std::vector<std::string> substrs = _nirt_string_split(hstr);
	    if (substrs.size() != 15) {
		nerr(nss, "Error processing ray line \"%s\"!\nExpected 15 elements, found %d\n", hstr.c_str(), substrs.size());
		return -1;
	    }
	    point_t in, out;
	    double los, scaled_los, d_in, d_out, obliq_in, obliq_out;
	    bu_log("Found HIT:\n");
	    bu_log("reg_name: %s\n", substrs[0].c_str());
	    bu_log("reg_path: %s\n", substrs[1].c_str());
	    bu_log("reg_id: %s\n", substrs[2].c_str());
	    in[X] = _nirt_str_to_dbl(substrs[3]);
	    in[Y] = _nirt_str_to_dbl(substrs[4]);
	    in[Z] = _nirt_str_to_dbl(substrs[5]);
	    d_in = _nirt_str_to_dbl(substrs[6]);
	    out[X] = _nirt_str_to_dbl(substrs[7]);
	    out[Y] = _nirt_str_to_dbl(substrs[8]);
	    out[Z] = _nirt_str_to_dbl(substrs[9]);
	    d_out = _nirt_str_to_dbl(substrs[10]);
	    los = _nirt_str_to_dbl(substrs[11]);
	    scaled_los = _nirt_str_to_dbl(substrs[12]);
	    obliq_in = _nirt_str_to_dbl(substrs[13]);
	    obliq_out = _nirt_str_to_dbl(substrs[14]);
	    bu_log("in: %0.17f, %0.17f, %0.17f\n", V3ARGS(in));
	    bu_log("out: %0.17f, %0.17f, %0.17f\n", V3ARGS(out));
	    bu_log("d_in: %0.17f d_out: %0.17f\n", d_in, d_out);
	    bu_log("los: %0.17f  scaled_los: %0.17f\n", los, scaled_los);
	    bu_log("obliq_in: %0.17f  obliq_out: %0.17f\n", obliq_in, obliq_out);
	}
	if (!line.compare(0, 5, "GAP: ")) {
	    std::string gstr = line.substr(5);
	    bu_log("Found GAP: %s\n", gstr.c_str());
	}
	if (!line.compare(0, 6, "MISS: ")) {
	    std::string mstr = line.substr(6);
	    bu_log("Found MISS: %s\n", mstr.c_str());
	}
	if (!line.compare(0, 9, "OVERLAP: ")) {
	    std::string ostr = line.substr(9);
	    bu_log("Found OVERLAP: %s\n", ostr.c_str());
	}
	//bu_log("line: %s\n", line.c_str());
    }

    if (have_ray) {
	// Execute work.
	//
	// Thought - if we have rays but no pre-defined output, write out the
	// expected output to stdout - in this mode diff will generate a diff
	// input file from a supplied list of rays.
    }

    // Report diff results according to the supplied options.

    ifs.close();

    return 0;
}


extern "C" int
_nirt_cmd_dir_vect(void *ns, int argc, const char *argv[])
{
    vect_t dir = VINIT_ZERO;
    struct bu_vls opt_msg = BU_VLS_INIT_ZERO;
    struct nirt_state *nss = (struct nirt_state *)ns;
    if (!ns) return -1;

    if (argc == 1) {
	nout(nss, "(x, y, z) = (%4.2f, %4.2f, %4.2f)\n", V3ARGS(nss->i->vals->dir));
	return 0;
    }

    argc--; argv++;
    if (argc != 3) {
	nerr(nss, "Usage:  dir %s\n", _nirt_get_desc_args("dir"));
	return -1;
    }

    if (bu_opt_vect_t(&opt_msg, 3, argv, (void *)&dir) == -1) {
	nerr(nss, "%s\n", bu_vls_addr(&opt_msg));
	bu_vls_free(&opt_msg);
	return -1;
    }

    VUNITIZE(dir);
    VMOVE(nss->i->vals->dir, dir);
    _nirt_dir2ae(nss);
    return 0;
}

extern "C" int
_nirt_cmd_grid_coor(void *ns, int argc, const char *argv[])
{
    vect_t grid = VINIT_ZERO;
    struct bu_vls opt_msg = BU_VLS_INIT_ZERO;
    struct nirt_state *nss = (struct nirt_state *)ns;
    if (!ns) return -1;

    if (argc == 1) {
	VSET(grid, nss->i->vals->h, nss->i->vals->v, nss->i->vals->d_orig);
	VSCALE(grid, grid, nss->i->base2local);
	nout(nss, "(h, v, d) = (%4.2f, %4.2f, %4.2f)\n", V3ARGS(grid));
	return 0;
    }

    argc--; argv++;
    if (argc != 2 && argc != 3) {
	nerr(nss, "Usage:  hv %s\n", _nirt_get_desc_args("hv"));
	return -1;
    }
    if (bu_opt_fastf_t(&opt_msg, 1, argv, (void *)&(nss->i->vals->h)) == -1) {
	nerr(nss, "%s\n", bu_vls_addr(&opt_msg));
	bu_vls_free(&opt_msg);
	return -1;
    }
    argc--; argv++;
    if (bu_opt_fastf_t(&opt_msg, 1, argv, (void *)&(nss->i->vals->v)) == -1) {
	nerr(nss, "%s\n", bu_vls_addr(&opt_msg));
	bu_vls_free(&opt_msg);
	return -1;
    }
    argc--; argv++;
    if (argc) {
	if (bu_opt_fastf_t(&opt_msg, 1, argv, (void *)&(nss->i->vals->d_orig)) == -1) {
	    nerr(nss, "%s\n", bu_vls_addr(&opt_msg));
	    bu_vls_free(&opt_msg);
	    return -1;
	}
    } else {
	grid[2] = nss->i->vals->d_orig;
    }

    VSCALE(grid, grid, nss->i->local2base);
    nss->i->vals->h = grid[0];
    nss->i->vals->v = grid[1];
    nss->i->vals->d_orig = grid[2];
    _nirt_grid2targ(nss);

    return 0;
}

extern "C" int
_nirt_cmd_target_coor(void *ns, int argc, const char *argv[])
{
    vect_t target = VINIT_ZERO;
    struct bu_vls opt_msg = BU_VLS_INIT_ZERO;
    struct nirt_state *nss = (struct nirt_state *)ns;
    if (!ns) return -1;

    if (argc == 1) {
	VMOVE(target, nss->i->vals->orig);
	VSCALE(target, target, nss->i->base2local);
	nout(nss, "(x, y, z) = (%4.2f, %4.2f, %4.2f)\n", V3ARGS(target));
	return 0;
    }

    argc--; argv++;
    if (argc != 3) {
	nerr(nss, "Usage:  xyz %s\n", _nirt_get_desc_args("xyz"));
	return -1;
    }

    if (bu_opt_vect_t(&opt_msg, 3, argv, (void *)&target) == -1) {
	nerr(nss, "%s\n", bu_vls_addr(&opt_msg));
	bu_vls_free(&opt_msg);
	return -1;
    }

    VSCALE(target, target, nss->i->local2base);
    VMOVE(nss->i->vals->orig, target);
    _nirt_targ2grid(nss);

    return 0;
}

extern "C" int
_nirt_cmd_shoot(void *ns, int argc, const char **UNUSED(argv))
{
    int i;
    struct nirt_state *nss = (struct nirt_state *)ns;
    if (!ns || !nss->i->ap) return -1;

    if (argc != 1) {
	nerr(nss, "Usage:  s\n");
	return -1;
    }

    /* If we have no active rtip, we don't need to prep or shoot */
    if (!_nirt_get_rtip(nss)) {
	return 0;
    } else {
	if (nss->i->need_reprep) {
	    /* If we need to (re)prep, do it now. Failure is an error. */
	    if (_nirt_raytrace_prep(nss)) {
		nerr(nss, "Error: raytrace prep failed!\n");
		return -1;
	    }
	} else {
	    /* Based on current settings, tell the ap which rtip to use */
	    nss->i->ap->a_rt_i = _nirt_get_rtip(nss);
	    nss->i->ap->a_resource = _nirt_get_resource(nss);
	}
    }

    double bov = _nirt_backout(nss);
    for (i = 0; i < 3; ++i) {
	nss->i->vals->orig[i] = nss->i->vals->orig[i] + (bov * -1*(nss->i->vals->dir[i]));
	nss->i->ap->a_ray.r_pt[i] = nss->i->vals->orig[i];
	nss->i->ap->a_ray.r_dir[i] = nss->i->vals->dir[i];
    }

    ndbg(nss, NIRT_DEBUG_BACKOUT,
	    "Backing out %g units to (%g %g %g), shooting dir is (%g %g %g)\n",
	    bov * nss->i->base2local,
	    nss->i->ap->a_ray.r_pt[0] * nss->i->base2local,
	    nss->i->ap->a_ray.r_pt[1] * nss->i->base2local,
	    nss->i->ap->a_ray.r_pt[2] * nss->i->base2local,
	    V3ARGS(nss->i->ap->a_ray.r_dir));

    // TODO - any necessary initialization for data collection by callbacks
    _nirt_init_ovlp(nss);
    (void)rt_shootray(nss->i->ap);

    // Undo backout
    for (i = 0; i < 3; ++i) {
	nss->i->vals->orig[i] = nss->i->vals->orig[i] - (bov * -1*(nss->i->vals->dir[i]));
    }

    return 0;
}

extern "C" int
_nirt_cmd_backout(void *ns, int argc, const char *argv[])
{
    struct nirt_state *nss = (struct nirt_state *)ns;
    if (!ns) return -1;

    if (argc == 1) {
	nout(nss, "backout = %d\n", nss->i->backout);
	return 0;
    }

    if (argc == 2 && BU_STR_EQUAL(argv[1], "0")) {
	nss->i->backout = 0;
	return 0;
    }
    if (argc == 2 && BU_STR_EQUAL(argv[1], "1")) {
	nss->i->backout = 1;
	return 0;
    }

    nerr(nss, "backout must be set to 0 (off) or 1 (on)\n");

    return -1;
}

extern "C" int
_nirt_cmd_use_air(void *ns, int argc, const char *argv[])
{
    struct nirt_state *nss = (struct nirt_state *)ns;
    struct bu_vls optparse_msg = BU_VLS_INIT_ZERO;
    if (!ns) return -1;
    if (argc == 1) {
	nout(nss, "use_air = %d\n", nss->i->use_air);
	return 0;
    }
    if (argc > 2) {
	nerr(nss, "Usage:  useair %s\n", _nirt_get_desc_args("useair"));
	return -1;
    }
    if (bu_opt_int(&optparse_msg, 1, (const char **)&(argv[1]), (void *)&nss->i->use_air) == -1) {
	nerr(nss, "Error: bu_opt value read failure: %s\n\nUsage:  useair %s\n", bu_vls_addr(&optparse_msg), _nirt_get_desc_args("useair"));
	return -1;
    }
    return 0;
}

extern "C" int
_nirt_cmd_units(void *ns, int argc, const char *argv[])
{
    double ncv = 0.0;
    struct nirt_state *nss = (struct nirt_state *)ns;
    if (!ns) return -1;

    if (argc == 1) {
	nout(nss, "units = '%s'\n", bu_units_string(nss->i->local2base));
	return 0;
    }

    if (argc > 2) {
	nerr(nss, "Usage:  units %s\n", _nirt_get_desc_args("units"));
	return -1;
    }

    if (BU_STR_EQUAL(argv[1], "default")) {
	nss->i->base2local = nss->i->dbip->dbi_base2local;
	nss->i->local2base = nss->i->dbip->dbi_local2base;
	return 0;
    }

    ncv = bu_units_conversion(argv[1]);
    if (ncv <= 0.0) {
	nerr(nss, "Invalid unit specification: '%s'\n", argv[1]);
	return -1;
    }
    nss->i->base2local = 1.0/ncv;
    nss->i->local2base = ncv;

    return 0;
}

extern "C" int
_nirt_cmd_do_overlap_claims(void *ns, int argc, const char *argv[])
{
    int valid_specification = 0;
    struct nirt_state *nss = (struct nirt_state *)ns;
    if (!ns) return -1;

    if (argc == 1) {
	switch (nss->i->overlap_claims) {
	    case NIRT_OVLP_RESOLVE:
		nout(nss, "resolve (0)\n");
		return 0;
	    case NIRT_OVLP_REBUILD_FASTGEN:
		nout(nss, "rebuild_fastgen (1)\n");
		return 0;
	    case NIRT_OVLP_REBUILD_ALL:
		nout(nss, "rebuild_all (2)\n");
		return 0;
	    case NIRT_OVLP_RETAIN:
		nout(nss, "retain (3)\n");
		return 0;
	    default:
		nerr(nss, "Error: overlap_clams is set to invalid value %d\n", nss->i->overlap_claims);
		return -1;
	}
    }

    if (argc > 2) {
	nerr(nss, "Usage:  overlap_claims %s\n", _nirt_get_desc_args("overlap_claims"));
	return -1;
    }

    if (BU_STR_EQUAL(argv[1], "resolve") || BU_STR_EQUAL(argv[1], "0")) {
	nss->i->overlap_claims = NIRT_OVLP_RESOLVE;
	valid_specification = 1;
    }
    if (BU_STR_EQUAL(argv[1], "rebuild_fastgen") || BU_STR_EQUAL(argv[1], "1")) {
	nss->i->overlap_claims = NIRT_OVLP_REBUILD_FASTGEN;
	valid_specification = 1;
    }
    if (BU_STR_EQUAL(argv[1], "rebuild_all") || BU_STR_EQUAL(argv[1], "2")) {
	nss->i->overlap_claims = NIRT_OVLP_REBUILD_ALL;
	valid_specification = 1;
    }
    if (BU_STR_EQUAL(argv[1], "retain") || BU_STR_EQUAL(argv[1], "3")) {
	nss->i->overlap_claims = NIRT_OVLP_RETAIN;
	valid_specification = 1;
    }

    if (!valid_specification) {
	nerr(nss, "Error: Invalid overlap_claims specification: '%s'\n", argv[1]);
	return -1;
    }

    if (nss->i->rtip) nss->i->rtip->rti_save_overlaps = (nss->i->overlap_claims > 0);
    if (nss->i->rtip_air) nss->i->rtip_air->rti_save_overlaps = (nss->i->overlap_claims > 0);

    return 0;
}

extern "C" int
_nirt_cmd_format_output(void *ns, int argc, const char **argv)
{
    struct nirt_state *nss = (struct nirt_state *)ns;
    char type;
    if (!ns) return -1;

    if (argc < 2 || strlen(argv[1]) != 1 || BU_STR_EQUAL(argv[1], "?")) {
	// TODO - need a *much* better help for fmt
	nerr(nss, "Usage:  fmt %s\n", _nirt_get_desc_args("fmt"));
	return -1;
    }

    type = argv[1][0];
    if (!strchr(NIRT_OUTPUT_TYPE_SPECIFIERS, type)) {
	nerr(nss, "Error: unknown fmt type %c\n", type);
	return -1;
    }

    argc--; argv++;
    if (argc == 1) {
	// print out current fmt str for the specified type
	struct bu_vls ostr = BU_VLS_INIT_ZERO;
	switch (type) {
	    case 'r':
		_nirt_print_fmt_str(&ostr, nss->i->fmt_ray);
		break;
	    case 'h':
		_nirt_print_fmt_str(&ostr, nss->i->fmt_head);
		break;
	    case 'p':
		_nirt_print_fmt_str(&ostr, nss->i->fmt_part);
		break;
	    case 'f':
		_nirt_print_fmt_str(&ostr, nss->i->fmt_foot);
		break;
	    case 'm':
		_nirt_print_fmt_str(&ostr, nss->i->fmt_miss);
		break;
	    case 'o':
		_nirt_print_fmt_str(&ostr, nss->i->fmt_ovlp);
		break;
	    case 'g':
		_nirt_print_fmt_str(&ostr, nss->i->fmt_gap);
		break;
	    default:
		nerr(nss, "Error: unknown fmt type %s\n", argv[1]);
		return -1;
		break;
	}
	nout(nss, "%s\n", bu_vls_addr(&ostr));
	bu_vls_free(&ostr);
	return 0;
    }

    argc--; argv++;
    if (argc) {

	int ac_prev = 0;
	int fmt_cnt = 0;
	int fmt_sp_count = 0;
	std::vector<std::pair<std::string,std::string> > fmt_tmp;
	const char *fmtstr = argv[0];
	char **fmts = NULL;

	/* Empty format strings are easy */
	if (strlen(argv[0]) == 0 || BU_STR_EQUAL(argv[0], "\"\"") || BU_STR_EQUAL(argv[0], "''")) {
	    goto set_fmt;
	}

	/* We now must take the format string and do a lot of validation on it so we
	 * can safely use it to dynamically format output.  This is a classic problem in C:
	 * CWE-134: Use of Externally-Controlled Format String. See
	 * http://cwe.mitre.org/data/definitions/134.html for more details. */

	if ((fmt_cnt = _nirt_split_fmt(fmtstr, &fmts)) <= 0) {
	    nerr(nss, "Error: failure parsing format string \"%s\"\n", fmtstr);
	    return -1;
	}
	argc--; argv++;

	for (int i = 0; i < fmt_cnt; i++) {
	    unsigned int fc = _nirt_fmt_sp_cnt(fmts[i]);
	    const char *key = NULL;
	    if (fc > 0) {
		std::string fs;
		if (!argc) {
		    nerr(nss, "Error: failure parsing format string \"%s\" - missing value for format specifier in substring \"%s\"\n", fmtstr, fmts[i]);
		    return -1;
		}
		key = argv[0];
		fmt_sp_count++;

		// Get fmt_sp substring and perform validation with the matching NIRT value key
		if (_nirt_fmt_sp_get(nss, fmts[i], fs)) return -1;
		if (_nirt_fmt_sp_validate(nss, fs)) return -1;
		if (_nirt_fmt_sp_key_check(nss, key, fs)) return -1;

		argc--; argv++;
	    }
	    if (key) {
		fmt_tmp.push_back(std::make_pair(std::string(fmts[i]), std::string(key)));
	    } else {
		/* If there are no format specifiers, we don't need any key */
		fmt_tmp.push_back(std::make_pair(std::string(fmts[i]), ""));
	    }
	}

	/* we don't care about leftovers if they're empty (looks like
	 * bu_argv_from_string can empty argv strings...) */
	ac_prev = 0;
	while (argc > 0 && ac_prev != argc) {
	    if (!strlen(argv[0])) {
		ac_prev = argc;
		argc--; argv++;
	    } else {
		ac_prev = argc;
	    }
	}

	/* If we *do* have leftover we we care about, error out */
	if (argc) {
	    nerr(nss, "Error: fmt string \"%s\" has %d format %s, but has %d %s specified\n", fmtstr, fmt_sp_count, (fmt_sp_count == 1) ? "specifier" : "specifiers", fmt_sp_count + argc, ((fmt_sp_count + argc) == 1) ? "key" : "keys");
	    return -1;
	}

	/* OK, validation passed - we should be good.  Scrub and replace pre-existing
	 * formatting instructions with the new. */
set_fmt:
	switch (type) {
	    case 'r':
		nss->i->fmt_ray.clear();
		nss->i->fmt_ray = fmt_tmp;
		break;
	    case 'h':
		nss->i->fmt_head.clear();
		nss->i->fmt_head = fmt_tmp;
		break;
	    case 'p':
		nss->i->fmt_part.clear();
		nss->i->fmt_part = fmt_tmp;
		break;
	    case 'f':
		nss->i->fmt_foot.clear();
		nss->i->fmt_foot = fmt_tmp;
		break;
	    case 'm':
		nss->i->fmt_miss.clear();
		nss->i->fmt_miss = fmt_tmp;
		break;
	    case 'o':
		nss->i->fmt_ovlp.clear();
		nss->i->fmt_ovlp = fmt_tmp;
		break;
	    case 'g':
		nss->i->fmt_gap.clear();
		nss->i->fmt_gap = fmt_tmp;
		break;
	    default:
		nerr(nss, "Error: unknown fmt type %s\n", argv[1]);
		return -1;
		break;
	}
    }

    return 0;
}

extern "C" int
_nirt_cmd_print_item(void *ns, int argc, const char **argv)
{
    int i = 0;
    struct bu_vls oval = BU_VLS_INIT_ZERO;
    struct nirt_state *nss = (struct nirt_state *)ns;
    if (!ns) return -1;

    if (argc == 1) {
	nerr(nss, "Usage:  print %s\n", _nirt_get_desc_args("print"));
	return 0;
    }

    argc--; argv++;

    for (i = 0; i < argc; i++) {
	const char *val_type = bu_avs_get(nss->i->val_types, argv[i]);
	bu_vls_trunc(&oval, 0);
	if (!val_type) {
	    nerr(nss, "Error: item %s is not a valid NIRT value key\n", argv[i]);
	    bu_vls_free(&oval);
	    return -1;
	}
	if (BU_STR_EQUAL(val_type, "INT")) {
	    _nirt_print_fmt_substr(&oval, "%d", argv[i], nss->i->vals, nss->i->base2local);
	    nout(nss, "%s\n", bu_vls_addr(&oval));
	    continue;
	}
	if (BU_STR_EQUAL(val_type, "FLOAT") || BU_STR_EQUAL(val_type, "FNOUNIT")) {
	    _nirt_print_fmt_substr(&oval, "%g", argv[i], nss->i->vals, nss->i->base2local);
	    nout(nss, "%s\n", bu_vls_addr(&oval));
	    continue;
	}
	if (BU_STR_EQUAL(val_type, "STRING")) {
	    _nirt_print_fmt_substr(&oval, "%s", argv[i], nss->i->vals, nss->i->base2local);
	    nout(nss, "%s\n", bu_vls_addr(&oval));
	    continue;
	}
	nerr(nss, "Error: item %s has unknown value type %s\n", argv[i], val_type);
	bu_vls_free(&oval);
	return -1;
    }

    bu_vls_free(&oval);
    return 0;
}

extern "C" int
_nirt_cmd_bot_minpieces(void *ns, int argc, const char **argv)
{
    /* Ew - rt_bot_minpieces is a librt global.  Why isn't it
     * a part of the specific rtip? */
    int ret = 0;
    long minpieces = 0;
    struct bu_vls opt_msg = BU_VLS_INIT_ZERO;
    struct nirt_state *nss = (struct nirt_state *)ns;
    if (!ns) return -1;

    if (argc == 1) {
	nout(nss, "rt_bot_minpieces = %d\n", (unsigned int)rt_bot_minpieces);
	return 0;
    }

    argc--; argv++;

    if (argc > 1) {
	nerr(nss, "Usage:  bot_minpieces %s\n", _nirt_get_desc_args("bot_minpieces"));
	return -1;
    }

    if ((ret = bu_opt_long(&opt_msg, 1, argv, (void *)&minpieces)) == -1) {
	nerr(nss, "Error: bu_opt value read failure reading minpieces value: %s\n", bu_vls_addr(&opt_msg));
	goto bot_minpieces_done;
    }

    if (minpieces < 0) {
	nerr(nss, "Error: rt_bot_minpieces cannot be less than 0\n");
	ret = -1;
	goto bot_minpieces_done;
    }

    if (rt_bot_minpieces != (size_t)minpieces) {
	rt_bot_minpieces = minpieces;
	bu_vls_free(&opt_msg);
	nss->i->need_reprep = 1;
    }

bot_minpieces_done:
    bu_vls_free(&opt_msg);
    return ret;
}

extern "C" int
_nirt_cmd_librt_debug(void *ns, int argc, const char **argv)
{
    int ret = 0;
    long dflg = 0;
    struct bu_vls msg = BU_VLS_INIT_ZERO;
    struct nirt_state *nss = (struct nirt_state *)ns;
    if (!ns) return -1;

    /* Sigh... another librt global, not rtip specific... */
    if (argc == 1) {
	bu_vls_printb(&msg, "librt debug ", RT_G_DEBUG, NIRT_DEBUG_FMT);
	nout(nss, "%s\n", bu_vls_addr(&msg));
	goto librt_nirt_debug_done;
    }

    argc--; argv++;

    if (argc > 1) {
	nerr(nss, "Usage:  libdebug %s\n", _nirt_get_desc_args("libdebug"));
	return -1;
    }

    if ((ret = bu_opt_long_hex(&msg, 1, argv, (void *)&dflg)) == -1) {
	nerr(nss, "%s\n", bu_vls_addr(&msg));
	goto librt_nirt_debug_done;
    }

    if (dflg < 0 || dflg > UINT32_MAX) {
	nerr(nss, "Error: LIBRT debug flag cannot be less than 0 or greater than %d\n", UINT32_MAX);
	ret = -1;
	goto librt_nirt_debug_done;
    }

    RT_G_DEBUG = (uint32_t)dflg;
    bu_vls_printb(&msg, "librt debug ", RT_G_DEBUG, NIRT_DEBUG_FMT);
    nout(nss, "%s\n", bu_vls_addr(&msg));

librt_nirt_debug_done:
    bu_vls_free(&msg);
    return ret;
}

extern "C" int
_nirt_cmd_debug(void *ns, int argc, const char **argv)
{
    int ret = 0;
    long dflg = 0;
    struct bu_vls msg = BU_VLS_INIT_ZERO;
    struct nirt_state *nss = (struct nirt_state *)ns;
    if (!ns) return -1;

    if (argc == 1) {
	bu_vls_printb(&msg, "debug ", nss->i->nirt_debug, NIRT_DEBUG_FMT);
	nout(nss, "%s\n", bu_vls_addr(&msg));
	goto nirt_debug_done;
    }

    argc--; argv++;

    if (argc > 1) {
	nerr(nss, "Usage:  debug %s\n", _nirt_get_desc_args("debug"));
	return -1;
    }

    if ((ret = bu_opt_long_hex(&msg, 1, argv, (void *)&dflg)) == -1) {
	nerr(nss, "%s\n", bu_vls_addr(&msg));
	goto nirt_debug_done;
    }

    if (dflg < 0) {
	nerr(nss, "Error: NIRT debug flag cannot be less than 0\n");
	ret = -1;
	goto nirt_debug_done;
    }

    nss->i->nirt_debug = (unsigned int)dflg;
    bu_vls_printb(&msg, "debug ", nss->i->nirt_debug, NIRT_DEBUG_FMT);
    nout(nss, "%s\n", bu_vls_addr(&msg));

nirt_debug_done:
    bu_vls_free(&msg);
    return ret;
}

extern "C" int
_nirt_cmd_quit(void *ns, int UNUSED(argc), const char **UNUSED(argv))
{
    nmsg((struct nirt_state *)ns, "Quitting...\n");
    return 1;
}

extern "C" int
_nirt_cmd_show_menu(void *ns, int UNUSED(argc), const char **UNUSED(argv))
{
    int longest = 0;
    const struct nirt_cmd_desc *d;
    struct nirt_state *nss = (struct nirt_state *)ns;
    if (!ns) return -1;
    for (d = nirt_descs; d->cmd != NULL; d++) {
	int l = strlen(d->cmd);
	if (l > longest) longest = l;
    }
    for (d = nirt_descs; d->cmd != NULL; d++) {
	nout(nss, "%*s %s\n", longest, d->cmd, d->desc);
    }
    return 0;
}

extern "C" int
_nirt_cmd_draw(void *ns, int argc, const char *argv[])
{
    struct nirt_state *nss = (struct nirt_state *)ns;
    if (!ns || argc < 2) return -1;
    size_t orig_size = nss->i->active_paths.size();
    for (int i = 1; i < argc; i++) {
	if (std::find(nss->i->active_paths.begin(), nss->i->active_paths.end(), argv[i]) == nss->i->active_paths.end()) {
	    nss->i->active_paths.push_back(argv[i]);
	}
    }
    if (nss->i->active_paths.size() != orig_size) nss->i->need_reprep = 1;
    return 0;
}

extern "C" int
_nirt_cmd_erase(void *ns, int argc, const char **argv)
{
    struct nirt_state *nss = (struct nirt_state *)ns;
    if (!ns || argc < 2) return -1;
    size_t orig_size = nss->i->active_paths.size();
    for (int i = 1; i < argc; i++) {
	std::vector<std::string>::iterator v_it = std::find(nss->i->active_paths.begin(), nss->i->active_paths.end(), argv[i]);
	if (v_it != nss->i->active_paths.end()) {
	    bu_log("erase_cmd: erasing %s\n", argv[i]);
	    nss->i->active_paths.erase(v_it);
	}
    }
    if (nss->i->active_paths.size() > 0 && nss->i->active_paths.size() != orig_size) nss->i->need_reprep = 1;
    return 0;
}

extern "C" int
_nirt_cmd_state(void *ns, int argc, const char *argv[])
{
    unsigned char rgb[3];
    struct bu_vls optmsg = BU_VLS_INIT_ZERO;
    struct nirt_state *nss = (struct nirt_state *)ns;
    if (!ns) return -1;
    if (argc == 1) {
	// TODO
	return 0;
    }
    argc--; argv++;
    if (BU_STR_EQUAL(argv[0], "oddcolor")) {
	if (argc == 1) {
	    if (bu_color_to_rgb_chars(nss->i->hit_odd_color, (unsigned char *)rgb)) return -1;
	    nout(nss, "%c %c %c", rgb[0], rgb[1], rgb[2]);
	    return 0;
	}
	if (bu_opt_color(&optmsg, argc, argv, (void *)nss->i->hit_odd_color) == -1) {
	    nerr(nss, "Error: bu_opt color read failure reading parsing");
	    for (int i = 0; i < argc; i++) {nerr(nss, " %s", argv[i]);}
	    nerr(nss, ": %s\n", bu_vls_addr(&optmsg));
	    bu_vls_free(&optmsg);
	    return -1;
	}
    }
    if (BU_STR_EQUAL(argv[0], "evencolor")) {
	if (argc == 1) {
	    if (bu_color_to_rgb_chars(nss->i->hit_even_color, (unsigned char *)rgb)) return -1;
	    nout(nss, "%c %c %c", rgb[0], rgb[1], rgb[2]);
	    return 0;
	}
	if (bu_opt_color(&optmsg, argc, argv, (void *)nss->i->hit_even_color) == -1) {
	    nerr(nss, "Error: bu_opt color read failure reading parsing");
	    for (int i = 0; i < argc; i++) {nerr(nss, " %s", argv[i]);}
	    nerr(nss, ": %s\n", bu_vls_addr(&optmsg));
	    bu_vls_free(&optmsg);
	    return -1;
	}

    }
    if (BU_STR_EQUAL(argv[0], "voidcolor")) {
	if (argc == 1) {
	    if (bu_color_to_rgb_chars(nss->i->void_color, (unsigned char *)rgb)) return -1;
	    nout(nss, "%c %c %c", rgb[0], rgb[1], rgb[2]);
	    return 0;
	}
	if (bu_opt_color(&optmsg, argc, argv, (void *)nss->i->void_color) == -1) {
	    nerr(nss, "Error: bu_opt color read failure reading parsing");
	    for (int i = 0; i < argc; i++) {nerr(nss, " %s", argv[i]);}
	    nerr(nss, ": %s\n", bu_vls_addr(&optmsg));
	    bu_vls_free(&optmsg);
	    return -1;
	}

    }
    if (BU_STR_EQUAL(argv[0], "overlapcolor")) {
	if (argc == 1) {
	    if (bu_color_to_rgb_chars(nss->i->overlap_color, (unsigned char *)rgb)) return -1;
	    nout(nss, "%c %c %c", rgb[0], rgb[1], rgb[2]);
	    return 0;
	}
	if (bu_opt_color(&optmsg, argc, argv, (void *)nss->i->overlap_color) == -1) {
	    nerr(nss, "Error: bu_opt color read failure reading parsing");
	    for (int i = 0; i < argc; i++) {nerr(nss, " %s", argv[i]);}
	    nerr(nss, ": %s\n", bu_vls_addr(&optmsg));
	    bu_vls_free(&optmsg);
	    return -1;
	}
    }


    if (BU_STR_EQUAL(argv[0], "plot_overlaps")) {
	if (argc == 1) {
	    nout(nss, "%d", nss->i->plot_overlaps);
	    return 0;
	}
	if (argc == 2 && BU_STR_EQUAL(argv[1], "0")) {
	    nss->i->plot_overlaps = 0;
	    return 0;
	}
	if (argc == 2 && BU_STR_EQUAL(argv[1], "1")) {
	    nss->i->plot_overlaps = 1;
	    return 0;
	}
	nerr(nss, "Error - valid plot_overlaps values are 0 or 1");
	return -1;
    }

    if (BU_STR_EQUAL(argv[0], "out_accumulate")) {
	int setting = 0;
	if (argc == 2) {
	    nout(nss, "%d\n", nss->i->out_accumulate);
	    return 0;
	}
	(void)bu_opt_int(NULL, 1, (const char **)&(argv[2]), (void *)&setting);
	nss->i->out_accumulate = setting;
	return 0;
    }
    if (BU_STR_EQUAL(argv[0], "err_accumulate")) {
	int setting = 0;
	if (argc == 2) {
	    nout(nss, "%d\n", nss->i->err_accumulate);
	    return 0;
	}
	(void)bu_opt_int(NULL, 1, (const char **)&(argv[2]), (void *)&setting);
	nss->i->err_accumulate = setting;
	return 0;
    }
    if (BU_STR_EQUAL(argv[0], "model_bounds")) {
	if (argc != 1) {
	    nerr(nss, "TODO - state cmd help\n");
	    return -1;
	}
	if (nss->i->need_reprep) {
	    if (_nirt_raytrace_prep(nss)) {
		nerr(nss, "Error: raytrace prep failed!\n");
		return -1;
	    }
	}
	if (nss->i->ap->a_rt_i) {
	    double base2local = nss->i->ap->a_rt_i->rti_dbip->dbi_base2local;
	    nout(nss, "model_min = (%g, %g, %g)    model_max = (%g, %g, %g)\n",
		    nss->i->ap->a_rt_i->mdl_min[X] * base2local,
		    nss->i->ap->a_rt_i->mdl_min[Y] * base2local,
		    nss->i->ap->a_rt_i->mdl_min[Z] * base2local,
		    nss->i->ap->a_rt_i->mdl_max[X] * base2local,
		    nss->i->ap->a_rt_i->mdl_max[Y] * base2local,
		    nss->i->ap->a_rt_i->mdl_max[Z] * base2local);
	} else {
	    nout(nss, "model_min = (0, 0, 0)    model_max = (0, 0, 0)\n");
	}
	return 0;
    }
    if (BU_STR_EQUAL(argv[0], "-d")) {
	struct bu_vls dumpstr = BU_VLS_INIT_ZERO;
	bu_vls_printf(&dumpstr, "#  file created by the dump command of nirt\n");
	bu_vls_printf(&dumpstr, "xyz %g %g %g\n", V3ARGS(nss->i->vals->orig));
	bu_vls_printf(&dumpstr, "dir %g %g %g\n", V3ARGS(nss->i->vals->dir));
	bu_vls_printf(&dumpstr, "useair %d\n", nss->i->use_air);
	bu_vls_printf(&dumpstr, "units %s\n", bu_units_string(nss->i->local2base));
	bu_vls_printf(&dumpstr, "overlap_claims ");
	switch (nss->i->overlap_claims) {
	    case NIRT_OVLP_RESOLVE:
		bu_vls_printf(&dumpstr, "resolve\n");
		break;
	    case NIRT_OVLP_REBUILD_FASTGEN:
		bu_vls_printf(&dumpstr, "rebuild_fastgen\n");
		break;
	    case NIRT_OVLP_REBUILD_ALL:
		bu_vls_printf(&dumpstr, "rebuild_all\n");
		break;
	    case NIRT_OVLP_RETAIN:
		bu_vls_printf(&dumpstr, "retain\n");
		break;
	    default:
		bu_vls_printf(&dumpstr, "error: invalid overlap_clams value %d\n", nss->i->overlap_claims);
		break;
	}
	struct bu_vls ostr = BU_VLS_INIT_ZERO;
	_nirt_print_fmt_cmd(&ostr, 'r', nss->i->fmt_ray);
	bu_vls_printf(&dumpstr, "%s", bu_vls_addr(&ostr));
	_nirt_print_fmt_cmd(&ostr, 'h', nss->i->fmt_head);
	bu_vls_printf(&dumpstr, "%s", bu_vls_addr(&ostr));
	_nirt_print_fmt_cmd(&ostr, 'p', nss->i->fmt_part);
	bu_vls_printf(&dumpstr, "%s", bu_vls_addr(&ostr));
	_nirt_print_fmt_cmd(&ostr, 'f', nss->i->fmt_foot);
	bu_vls_printf(&dumpstr, "%s", bu_vls_addr(&ostr));
	_nirt_print_fmt_cmd(&ostr, 'm', nss->i->fmt_miss);
	bu_vls_printf(&dumpstr, "%s", bu_vls_addr(&ostr));
	_nirt_print_fmt_cmd(&ostr, 'o', nss->i->fmt_ovlp);
	bu_vls_printf(&dumpstr, "%s", bu_vls_addr(&ostr));
	_nirt_print_fmt_cmd(&ostr, 'g', nss->i->fmt_gap);
	bu_vls_printf(&dumpstr, "%s", bu_vls_addr(&ostr));
	nout(nss, "%s", bu_vls_addr(&dumpstr));
	bu_vls_free(&dumpstr);
	bu_vls_free(&ostr);
    }

    return 0;
}

const struct bu_cmdtab _libanalyze_nirt_cmds[] = {
    { "attr",           _nirt_cmd_attr},
    { "ae",             _nirt_cmd_az_el},
    { "diff",           _nirt_cmd_diff},
    { "dir",            _nirt_cmd_dir_vect},
    { "hv",             _nirt_cmd_grid_coor},
    { "xyz",            _nirt_cmd_target_coor},
    { "s",              _nirt_cmd_shoot},
    { "backout",        _nirt_cmd_backout},
    { "useair",         _nirt_cmd_use_air},
    { "units",          _nirt_cmd_units},
    { "overlap_claims", _nirt_cmd_do_overlap_claims},
    { "fmt",            _nirt_cmd_format_output},
    { "print",          _nirt_cmd_print_item},
    { "bot_minpieces",  _nirt_cmd_bot_minpieces},
    { "libdebug",       _nirt_cmd_librt_debug},
    { "debug",          _nirt_cmd_debug},
    { "q",              _nirt_cmd_quit},
    { "?",              _nirt_cmd_show_menu},
    // New commands...
    { "draw",           _nirt_cmd_draw},
    { "erase",          _nirt_cmd_erase},
    { "state",          _nirt_cmd_state},
    { (char *)NULL,     NULL}
};




/* Parse command line command and execute */
HIDDEN int
_nirt_exec_cmd(struct nirt_state *ns, const char *cmdstr)
{
    int ac = 0;
    int ac_max = 0;
    int i = 0;
    int ret = 0;
    char **av = NULL;
    size_t q_start, q_end, pos, cmt;
    std::string entry;
    if (!ns || !cmdstr || strlen(cmdstr) > entry.max_size()) return -1;
    std::string s(cmdstr);
    std::stringstream ss(s);
    // get an upper limit on the size of argv
    while (std::getline(ss, entry, ' ')) ac_max++;
    ss.clear();
    ss.seekg(0, ss.beg);

    /* If part of the line is commented, skip that part */
    cmt = _nirt_find_first_unescaped(s, "#", 0);
    if (cmt != std::string::npos) {
	s.erase(cmt);
    }

    _nirt_trim_whitespace(s);
    if (!s.length()) return 0;

    /* Start by initializing the position markers for quoted substrings. */
    q_start = _nirt_find_first_unescaped(s, "\"", 0);
    q_end = (q_start != std::string::npos) ? _nirt_find_first_unescaped(s, "\"", q_start + 1) : std::string::npos;

    /* get an argc/argv array for bu_cmd style command execution */
    av = (char **)bu_calloc(ac_max+1, sizeof(char *), "av");

    pos = 0; ac = 0;
    while ((pos = s.find_first_of(" \t", pos)) != std::string::npos) {
	/* If we're inside matched quotes, only an un-escaped quote char means
	 * anything */
	if (q_end != std::string::npos && pos > q_start && pos < q_end) {
	    pos = q_end + 1;
	    continue;
	}
	/* Extract and operate on the command specific subset string */
	std::string cmd = s.substr(0, pos);
	if (cmd.find_first_not_of(" \t") != std::string::npos) {
	    av[ac] = bu_strdup(cmd.c_str());
	    ac++;
	}

	/* Prepare the rest of the script string for processing */
	s.erase(0, pos + 1);
	q_start = _nirt_find_first_unescaped(s, "\"", 0);
	q_end = (q_start != std::string::npos) ? _nirt_find_first_unescaped(s, "\"", q_start + 1) : std::string::npos;
	pos = 0;
    }
    if (s.length() > 0) {
	av[ac] = bu_strdup(s.c_str());
	ac++;
    }
    av[ac] = NULL;

    if (bu_cmd(_libanalyze_nirt_cmds, ac, (const char **)av, 0, (void *)ns, &ret) != BRLCAD_OK) ret = -1;

    for (i = 0; i < ac_max; i++) {
	if (av[i]) bu_free(av[i], "free av str");
    }
    bu_free(av, "free av array");
    return ret;
}

/* Parse command line script into individual commands, and run
 * the supplied callback on each command. The semicolon ';' char
 * denotes the end of one command and the beginning of another, unless
 * the semicolon is inside single or double quotes*/
HIDDEN int
_nirt_parse_script(struct nirt_state *ns, const char *script)
{
    if (!ns || !script) return -1;
    int ret = 0;
    std::string s(script);
    std::string cmd;
    struct bu_vls tcmd = BU_VLS_INIT_ZERO;
    size_t pos = 0;
    size_t q_start, q_end, cmt;

    /* If part of the line is commented, skip that part */
    cmt = _nirt_find_first_unescaped(s, "#", 0);
    if (cmt != std::string::npos) {
	s.erase(cmt);
    }

    _nirt_trim_whitespace(s);
    if (!s.length()) return 0;

    /* Start by initializing the position markers for quoted substrings. */
    q_start = _nirt_find_first_unescaped(s, "\"", 0);
    q_end = (q_start != std::string::npos) ? _nirt_find_first_unescaped(s, "\"", q_start + 1) : std::string::npos;

    /* Slice and dice to get individual commands. */
    while ((pos = _nirt_find_first_unquoted(s, ";", pos)) != std::string::npos) {
	/* If we're inside matched quotes, only an un-escaped quote char means
	 * anything */
	if (q_end != std::string::npos && pos > q_start && pos < q_end) {
	    pos = q_end + 1;
	    continue;
	}
	/* Extract and operate on the command specific subset string */
	cmd = s.substr(0, pos);
	bu_vls_sprintf(&tcmd, "%s", cmd.c_str());
	bu_vls_trimspace(&tcmd);
	if (bu_vls_strlen(&tcmd) > 0) {
	    if (strchr(bu_vls_addr(&tcmd), ';')) {
		ret = _nirt_parse_script(ns, bu_vls_addr(&tcmd));
	    } else {
		ret = _nirt_exec_cmd(ns, bu_vls_addr(&tcmd));
	    }
	    if (ret) goto nps_done;
	}

	/* Prepare the rest of the script string for processing */
	s.erase(0, pos + 1);
	q_start = _nirt_find_first_unescaped(s, "\"", 0);
	q_end = (q_start != std::string::npos) ? _nirt_find_first_unescaped(s, "\"", q_start + 1) : std::string::npos;
	pos = 0;
    }

    /* If there's anything left over, it's a tailing command or a single command */
    bu_vls_sprintf(&tcmd, "%s", s.c_str());
    bu_vls_trimspace(&tcmd);
    if (bu_vls_strlen(&tcmd) > 0) {
	ret = _nirt_exec_cmd(ns, bu_vls_addr(&tcmd));
    }

nps_done:
    bu_vls_free(&tcmd);
    return ret;
}


//////////////////////////
/* Public API functions */
//////////////////////////

extern "C" int
nirt_init(struct nirt_state *ns)
{
    unsigned int rgb[3] = {0, 0, 0};
    struct nirt_state_impl *n = NULL;

    if (!ns) return -1;

    /* Get memory */
    n = new nirt_state_impl;
    ns->i = n;
    BU_GET(n->hit_odd_color, struct bu_color);
    BU_GET(n->hit_even_color, struct bu_color);
    BU_GET(n->void_color, struct bu_color);
    BU_GET(n->overlap_color, struct bu_color);

    /* Strictly speaking these initializations are more
     * than an "alloc" function needs to do, but we want
     * a nirt state to be "functional" even though it
     * is not set up with a database (it's true/proper
     * initialization.)*/
    bu_color_from_rgb_chars(n->hit_odd_color, (unsigned char *)&rgb);
    bu_color_from_rgb_chars(n->hit_even_color, (unsigned char *)&rgb);
    bu_color_from_rgb_chars(n->void_color, (unsigned char *)&rgb);
    bu_color_from_rgb_chars(n->overlap_color, (unsigned char *)&rgb);
    n->print_header = 0;
    n->print_ident_flag = 0;
    n->rt_debug = 0;
    n->nirt_debug = 0;

    BU_GET(n->out, struct bu_vls);
    bu_vls_init(n->out);
    n->out_accumulate = 0;
    BU_GET(n->msg, struct bu_vls);
    bu_vls_init(n->msg);
    BU_GET(n->err, struct bu_vls);
    bu_vls_init(n->err);
    n->err_accumulate = 0;
    BU_LIST_INIT(&(n->s_vlist));
    n->segs = bn_vlblock_init(&(n->s_vlist), 32);
    n->plot_overlaps = 1;
    n->ret = 0;

    n->h_state = NULL;
    n->h_out = NULL;
    n->h_msg = NULL;
    n->h_err = NULL;
    n->h_segs = NULL;
    n->h_objs = NULL;
    n->h_frmts = NULL;
    n->h_view = NULL;

    n->base2local = 0.0;
    n->local2base = 0.0;
    n->use_air = 0;
    n->backout = 0;

    n->b_state = false;
    n->b_segs = false;
    n->b_objs = false;
    n->b_frmts = false;
    n->b_view = false;

    BU_GET(n->ap, struct application);
    n->dbip = DBI_NULL;
    BU_GET(n->res, struct resource);
    BU_GET(n->res_air, struct resource);
    n->rtip = RTI_NULL;
    n->rtip_air = RTI_NULL;
    n->need_reprep = 1;

    BU_GET(n->val_types, struct bu_attribute_value_set);
    BU_GET(n->val_docs, struct bu_attribute_value_set);
    bu_avs_init_empty(n->val_types);
    bu_avs_init_empty(n->val_docs);

    BU_GET(n->vals, struct nirt_output_record);
    BU_GET(n->vals->path_name, struct bu_vls);
    BU_GET(n->vals->reg_name, struct bu_vls);
    BU_GET(n->vals->ov_reg1_name, struct bu_vls);
    BU_GET(n->vals->ov_reg2_name, struct bu_vls);
    BU_GET(n->vals->ov_sol_in, struct bu_vls);
    BU_GET(n->vals->ov_sol_out, struct bu_vls);
    BU_GET(n->vals->claimant_list, struct bu_vls);
    BU_GET(n->vals->claimant_listn, struct bu_vls);
    BU_GET(n->vals->attributes, struct bu_vls);

    VSETALL(n->vals->orig, 0.0);
    n->vals->h = 0.0;
    n->vals->v = 0.0;
    n->vals->d_orig = 0.0;

    bu_vls_init(n->vals->path_name);
    bu_vls_init(n->vals->reg_name);
    bu_vls_init(n->vals->ov_reg1_name);
    bu_vls_init(n->vals->ov_reg2_name);
    bu_vls_init(n->vals->ov_sol_in);
    bu_vls_init(n->vals->ov_sol_out);
    bu_vls_init(n->vals->claimant_list);
    bu_vls_init(n->vals->claimant_listn);
    bu_vls_init(n->vals->attributes);

    /* Populate the output key and type information */
    {
	std::string s(nirt_cmd_tbl_defs);
	std::string entry, key;
	std::stringstream ss(s);
	int place = 0;
	while (std::getline(ss, entry, ',')) {
	    std::string tentry;
	    size_t sb = entry.find_first_not_of(" \t");
	    size_t se = entry.find_last_not_of(" \t");
	    if (sb != std::string::npos && se != std::string::npos) {
		tentry = entry.substr(sb, se - sb + 1);
	    }
	    if (!place) { key = tentry; place++; continue; }
	    if (place == 1) {
		bu_avs_add(n->val_types, key.c_str(), tentry.c_str());
		place++;
		continue;
	    }
	    if (place == 2) {
		if (!tentry.empty()) {
		    bu_avs_add(n->val_docs, key.c_str(), tentry.c_str());
		}
		place = 0;
	    }
	}
    }

    n->u_data = NULL;

    /* Initial direction is -x */
    VSET(n->vals->dir, -1, 0, 0);

    /* Set up the default NIRT formatting output */
    std::ifstream fs;
    std::string fmtline;
    fs.open(bu_brlcad_data("nirt/default.nrt", 0), std::fstream::binary);
    if (!fs.is_open()) {
	bu_log("Error - could not load default NIRT formatting file: %s\n", bu_brlcad_data("nirt/default.nrt", 0));
	return -1;
    }
    while (std::getline(fs, fmtline)) {
	if (nirt_exec(ns, fmtline.c_str()) < 0) {
	    bu_log("Error - could not load default NIRT formatting file: %s\n", bu_brlcad_data("nirt/default.nrt", 0));
	    return -1;
	}
    }

    fs.close();

    return 0;
}

extern "C" int
nirt_init_dbip(struct nirt_state *ns, struct db_i *dbip)
{
    if (!ns || !dbip) return -1;

    RT_CK_DBI(dbip);

    ns->i->dbip = db_clone_dbi(dbip, NULL);

    /* initialize the application structure */
    RT_APPLICATION_INIT(ns->i->ap);
    ns->i->ap->a_hit = _nirt_if_hit;        /* branch to if_hit routine */
    ns->i->ap->a_miss = _nirt_if_miss;      /* branch to if_miss routine */
    ns->i->ap->a_overlap = _nirt_if_overlap;/* branch to if_overlap routine */
    ns->i->ap->a_logoverlap = rt_silent_logoverlap;
    ns->i->ap->a_onehit = 0;               /* continue through shotline after hit */
    ns->i->ap->a_purpose = "NIRT ray";
    ns->i->ap->a_rt_i = _nirt_get_rtip(ns);         /* rt_i pointer */
    ns->i->ap->a_resource = _nirt_get_resource(ns); /* note: resource is initialized by get_rtip */
    ns->i->ap->a_zero1 = 0;           /* sanity check, sayth raytrace.h */
    ns->i->ap->a_zero2 = 0;           /* sanity check, sayth raytrace.h */
    ns->i->ap->a_uptr = (void *)ns;

    /* If we've already got something, go ahead and prep */
    if (ns->i->need_reprep && ns->i->active_paths.size() > 0) {
	if (_nirt_raytrace_prep(ns)) {
	    nerr(ns, "Error - raytrace prep failed during initialization!\n");
	    return -1;
	}
    }

    /* By default use the .g units */
    ns->i->base2local = dbip->dbi_base2local;
    ns->i->local2base = dbip->dbi_local2base;
    return 0;
}


extern "C" int
nirt_clear_dbip(struct nirt_state *ns)
{
    if (!ns) return -1;

    db_close(ns->i->dbip);
    ns->i->dbip = NULL;

    /* clear relevant parts of the application structure */
    if (ns->i->rtip) rt_clean(ns->i->rtip);
    if (ns->i->rtip_air) rt_clean(ns->i->rtip_air);
    ns->i->ap->a_rt_i = NULL;
    ns->i->ap->a_resource = NULL;

    ns->i->active_paths.clear();

    ns->i->base2local = 0.0;
    ns->i->local2base = 0.0;
    return 0;
}

void
nirt_destroy(struct nirt_state *ns)
{
    if (!ns) return;
    bu_vls_free(ns->i->err);
    bu_vls_free(ns->i->msg);
    bu_vls_free(ns->i->out);
    bn_vlist_cleanup(&(ns->i->s_vlist));
    bn_vlblock_free(ns->i->segs);

    if (ns->i->rtip != RTI_NULL) rt_free_rti(ns->i->rtip);
    if (ns->i->rtip_air != RTI_NULL) rt_free_rti(ns->i->rtip_air);

    db_close(ns->i->dbip);

    bu_vls_free(ns->i->vals->path_name);
    bu_vls_free(ns->i->vals->reg_name);
    bu_vls_free(ns->i->vals->ov_reg1_name);
    bu_vls_free(ns->i->vals->ov_reg2_name);
    bu_vls_free(ns->i->vals->ov_sol_in);
    bu_vls_free(ns->i->vals->ov_sol_out);
    bu_vls_free(ns->i->vals->claimant_list);
    bu_vls_free(ns->i->vals->claimant_listn);
    bu_vls_free(ns->i->vals->attributes);

    BU_PUT(ns->i->vals->path_name, struct bu_vls);
    BU_PUT(ns->i->vals->reg_name, struct bu_vls);
    BU_PUT(ns->i->vals->ov_reg1_name, struct bu_vls);
    BU_PUT(ns->i->vals->ov_reg2_name, struct bu_vls);
    BU_PUT(ns->i->vals->ov_sol_in, struct bu_vls);
    BU_PUT(ns->i->vals->ov_sol_out, struct bu_vls);
    BU_PUT(ns->i->vals->claimant_list, struct bu_vls);
    BU_PUT(ns->i->vals->claimant_listn, struct bu_vls);
    BU_PUT(ns->i->vals->attributes, struct bu_vls);
    BU_PUT(ns->i->vals, struct nirt_output_record);

    BU_PUT(ns->i->res, struct resource);
    BU_PUT(ns->i->res_air, struct resource);
    BU_PUT(ns->i->err, struct bu_vls);
    BU_PUT(ns->i->msg, struct bu_vls);
    BU_PUT(ns->i->out, struct bu_vls);
    BU_PUT(ns->i->ap, struct application);
    BU_PUT(ns->i->hit_odd_color, struct bu_color);
    BU_PUT(ns->i->hit_even_color, struct bu_color);
    BU_PUT(ns->i->void_color, struct bu_color);
    BU_PUT(ns->i->overlap_color, struct bu_color);
    delete ns->i;
}

/* Note - defined inside the execute-once do-while loop to allow for putting a
 * semicolon at the end of a NIRT_HOOK() statement (makes code formatters
 * happier) */
#define NIRT_HOOK(btype,htype) \
    do { if (ns->i->btype && ns->i->htype){(*ns->i->htype)(ns, ns->i->u_data);} } \
    while (0)

int
nirt_exec(struct nirt_state *ns, const char *script)
{
    ns->i->ret = 0;
    ns->i->b_state = false;
    ns->i->b_segs = false;
    ns->i->b_objs = false;
    ns->i->b_frmts = false;
    ns->i->b_view = false;

    /* Unless told otherwise, clear the textual outputs
     * before each nirt_exec call. */
    if (!ns->i->out_accumulate) bu_vls_trunc(ns->i->out, 0);
    if (!ns->i->err_accumulate) bu_vls_trunc(ns->i->err, 0);

    if (script) {
	ns->i->ret = _nirt_parse_script(ns, script);
    } else {
	return 0;
    }

    NIRT_HOOK(b_state, h_state);
    NIRT_HOOK(b_segs, h_segs);
    NIRT_HOOK(b_objs, h_objs);
    NIRT_HOOK(b_frmts, h_frmts);
    NIRT_HOOK(b_view, h_view);

    return ns->i->ret;
}

void *
nirt_udata(struct nirt_state *ns, void *u_data)
{
    if (!ns) return NULL;
    if (u_data) ns->i->u_data = u_data;
    return ns->i->u_data;
}

void
nirt_hook(struct nirt_state *ns, nirt_hook_t hf, int flag)
{
    if (!ns || !hf) return;
    switch (flag) {
	case NIRT_ALL:
	    ns->i->h_state = hf;
	    break;
	case NIRT_OUT:
	    ns->i->h_out = hf;
	    break;
	case NIRT_MSG:
	    ns->i->h_msg = hf;
	    break;
	case NIRT_ERR:
	    ns->i->h_err = hf;
	    break;
	case NIRT_SEGS:
	    ns->i->h_segs = hf;
	    break;
	case NIRT_OBJS:
	    ns->i->h_objs = hf;
	    break;
	case NIRT_FRMTS:
	    ns->i->h_frmts = hf;
	    break;
	case NIRT_VIEW:
	    ns->i->h_view = hf;
	    break;
	default:
	    bu_log("Error: unknown hook type %d\n", flag);
	    return;
    }
}

#define NCFC(nf) ((flags & nf) && !all_set) || (all_set &&  !((flags & nf)))

void
nirt_clear(struct nirt_state *ns, int flags)
{
    int all_set;
    if (!ns) return;
    all_set = (flags & NIRT_ALL) ? 1 : 0;

    if (NCFC(NIRT_OUT)) {
	bu_vls_trunc(ns->i->out, 0);
    }

    if (NCFC(NIRT_ERR)) {
	bu_vls_trunc(ns->i->err, 0);
    }

    if (NCFC(NIRT_SEGS)) {
    }

    if (NCFC(NIRT_OBJS)) {
    }

    if (NCFC(NIRT_FRMTS)) {
    }

    if (NCFC(NIRT_VIEW)) {
    }

    /* Standard outputs handled - now, if NIRT_ALL was
     * supplied, clear any internal state. */
    if (flags == NIRT_ALL) {
    }

}

void
nirt_log(struct bu_vls *o, struct nirt_state *ns, int output_type)
{
    if (!o || !ns) return;
    switch (output_type) {
	case NIRT_ALL:
	    bu_vls_sprintf(o, "%s", "NIRT_ALL: TODO\n");
	    break;
	case NIRT_OUT:
	    bu_vls_strcpy(o, bu_vls_addr(ns->i->out));
	    break;
	case NIRT_MSG:
	    bu_vls_strcpy(o, bu_vls_addr(ns->i->msg));
	    break;
	case NIRT_ERR:
	    bu_vls_strcpy(o, bu_vls_addr(ns->i->err));
	    break;
	case NIRT_SEGS:
	    bu_vls_sprintf(o, "%s", "NIRT_SEGS: TODO\n");
	    break;
	case NIRT_OBJS:
	    bu_vls_sprintf(o, "%s", "NIRT_OBJS: TODO\n");
	    break;
	case NIRT_FRMTS:
	    bu_vls_sprintf(o, "%s", "NIRT_FRMTS: TODO\n");
	    break;
	case NIRT_VIEW:
	    bu_vls_sprintf(o, "%s", "NIRT_VIEW: TODO\n");
	    break;
	default:
	    bu_log("Error: unknown output type %d\n", output_type);
	    return;
    }

}

int
nirt_help(struct bu_vls *h, struct nirt_state *ns,  bu_opt_format_t UNUSED(output_type))
{
    if (!h || !ns) return -1;
    return 0;
}



// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
