/*                    B O T T E S S . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2011 United States Government as represented by
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

/** @file libgcv/region_end.c
 *
 * Generate a tree of resolved BoT's, one per region. Roughly based
 * on UnBBoolean's j3dbool (and associated papers).
 *
 */

#include "common.h"

#include <math.h>	/* ceil */
#include <string.h>	/* memcpy */

#include "bn.h"
#include "raytrace.h"
#include "nmg.h"
#include "gcv.h"


struct gcv_data {
    void (*func)(struct nmgregion *, const struct db_full_path *, int, int, float [3]);
};


/* hijack the top four bits of mode. For these operations, the mode should
 * necessarily be 0x02 */
#define INSIDE		0x01
#define OUTSIDE		0x02
#define SAME		0x04
#define OPPOSITE	0x08
#define INVERTED	0x10

#define SOUP_MAGIC	0x534F5550	/* SOUP, 32b */
#define SOUP_CKMAG(_ptr) BU_CKMAG(_ptr, SOUP_MAGIC, "soup")

struct face_s {
    point_t vert[3], min, max;
    plane_t plane;
    uint32_t foo;
};


struct soup_s {
    uint32_t magic;
    struct face_s *faces;
    unsigned long int nfaces, maxfaces;
};


/* assume 4096, seems common enough. a portable way to get to PAGE_SIZE might be
 * better. */
HIDDEN const int faces_per_page = 4 * 4096 / sizeof(struct face_s);


HIDDEN int
soup_rm_face(struct soup_s *s, unsigned long int i)
{
    if(i>=s->nfaces) {
	bu_log("trying to remove a nonexisant face? %d/%d\n", i, s->nfaces);
	bu_bomb("Asploding\n");
    }
    memcpy(&s->faces[i], &s->faces[s->nfaces-1], sizeof(struct face_s));
    return s->nfaces--;
}


HIDDEN int
soup_add_face_precomputed(struct soup_s *s, point_t a, point_t b , point_t c, plane_t d, uint32_t foo)
{
    struct face_s *f;

    /* grow face array if needed */
    if(s->nfaces >= s->maxfaces) {
	bu_log("Resizing, %d aint' enough\n", s->nfaces);
	s->faces = bu_realloc(s->faces, (s->maxfaces += faces_per_page) * sizeof(struct face_s), "bot soup faces");
    }
    f = s->faces + s->nfaces;

    VMOVE(f->vert[0], a);
    VMOVE(f->vert[1], b);
    VMOVE(f->vert[2], c);

    HMOVE(f->plane, d);

    /* solve the bounding box (should this be VMINMAX?) */
    VMOVE(f->min, f->vert[0]); VMOVE(f->max, f->vert[0]);
    VMIN(f->min, f->vert[1]); VMAX(f->max, f->vert[1]);
    VMIN(f->min, f->vert[2]); VMAX(f->max, f->vert[2]);
    /* fluff the bounding box for fp fuzz */
    f->min[X]-=.1; f->min[Y]-=.1; f->min[Z]-=.1;
    f->max[X]+=.1; f->max[Y]+=.1; f->max[Z]+=.1;

    f->foo = foo;
    s->nfaces++;
    return 0;
}


HIDDEN int
soup_add_face(struct soup_s *s, point_t a, point_t b, point_t c, const struct bn_tol *tol) {
    plane_t p;

    /* solve the plane */
    bn_mk_plane_3pts(p, a, b, c, tol);

    return soup_add_face_precomputed(s, a, b, c, p, OUTSIDE);
}


/**********************************************************************/
/* stuff from the moller97 paper */


HIDDEN void
fisect2(
    point_t VTX0, point_t VTX1, point_t VTX2,
    fastf_t VV0, fastf_t VV1, fastf_t VV2,
    fastf_t D0, fastf_t D1, fastf_t D2,
    fastf_t *isect0, fastf_t *isect1, point_t isectpoint0, point_t isectpoint1)
{
    fastf_t tmp=D0/(D0-D1);
    fastf_t diff[3];

    *isect0=VV0+(VV1-VV0)*tmp;
    VSUB2(diff, VTX1, VTX0);
    VSCALE(diff, diff, tmp);
    VADD2(isectpoint0, diff, VTX0);

    tmp=D0/(D0-D2);
    *isect1=VV0+(VV2-VV0)*tmp;
    VSUB2(diff, VTX2, VTX0);
    VSCALE(diff, diff, tmp);
    VADD2(isectpoint1, VTX0, diff);
}


HIDDEN int
compute_intervals_isectline(struct face_s *f,
			    fastf_t VV0, fastf_t VV1, fastf_t VV2, fastf_t D0, fastf_t D1, fastf_t D2,
			    fastf_t D0D1, fastf_t D0D2, fastf_t *isect0, fastf_t *isect1,
			    fastf_t isectpoint0[3], fastf_t isectpoint1[3],
			    const struct bn_tol *tol)
{
    if(D0D1>0.0f)
	/* here we know that D0D2<=0.0 */
	/* that is D0, D1 are on the same side, D2 on the other or on the plane */
	fisect2(f->vert[2], f->vert[0], f->vert[1], VV2, VV0, VV1, D2, D0, D1, isect0, isect1, isectpoint0, isectpoint1);
    else if(D0D2>0.0f)
	/* here we know that d0d1<=0.0 */
	fisect2(f->vert[1], f->vert[0], f->vert[2], VV1, VV0, VV2, D1, D0, D2, isect0, isect1, isectpoint0, isectpoint1);
    else if(D1*D2>0.0f || !NEAR_ZERO(D0, tol->dist))
	/* here we know that d0d1<=0.0 or that D0!=0.0 */
	fisect2(f->vert[0], f->vert[1], f->vert[2], VV0, VV1, VV2, D0, D1, D2, isect0, isect1, isectpoint0, isectpoint1);
    else if(!NEAR_ZERO(D1, tol->dist))
	fisect2(f->vert[1], f->vert[0], f->vert[2], VV1, VV0, VV2, D1, D0, D2, isect0, isect1, isectpoint0, isectpoint1);
    else if(!NEAR_ZERO(D2, tol->dist))
	fisect2(f->vert[2], f->vert[0], f->vert[1], VV2, VV0, VV1, D2, D0, D1, isect0, isect1, isectpoint0, isectpoint1);
    else
	/* triangles are coplanar */
	return 1;
    return 0;
}


HIDDEN int
edge_edge_test(point_t V0, point_t U0, point_t U1, fastf_t Ax, fastf_t Ay, int i0, int i1)
{
    fastf_t Bx, By, Cx, Cy, e, d, f;

    Bx = U0[i0] - U1[i0];
    By = U0[i1] - U1[i1];
    Cx = V0[i0] - U0[i0];
    Cy = V0[i1] - U0[i1];
    f = Ay * Bx - Ax * By;
    d = By * Cx - Bx * Cy;
    if ((f > 0 && d >= 0 && d <= f) || (f < 0 && d <= 0 && d >= f)) {
	e = Ax * Cy - Ay * Cx;
	if (f > 0) {
	    if (e >= 0 && e <= f)
		return 1;
	} else if (e <= 0 && e >= f)
	    return 1;
    }
    return 0;
}


HIDDEN int
edge_against_tri_edges(point_t V0, point_t V1, point_t U0, point_t U1, point_t U2, int i0, int i1)
{
    fastf_t Ax, Ay;
    Ax=V1[i0]-V0[i0];
    Ay=V1[i1]-V0[i1];
    /* test edge U0, U1 against V0, V1 */
    if(edge_edge_test(V0, U0, U1, Ax, Ay, i0, i1)) return 1;
    /* test edge U1, U2 against V0, V1 */
    if(edge_edge_test(V0, U1, U2, Ax, Ay, i0, i1)) return 1;
    /* test edge U2, U1 against V0, V1 */
    if(edge_edge_test(V0, U2, U0, Ax, Ay, i0, i1)) return 1;
    return 0;
}


HIDDEN int
point_in_tri(point_t V0, point_t U0, point_t U1, point_t U2, int i0, int i1)
{
    fastf_t a, b, c, d0, d1, d2;
    /* is T1 completly inside T2? */
    /* check if V0 is inside tri(U0, U1, U2) */
    a=U1[i1]-U0[i1];
    b=-(U1[i0]-U0[i0]);
    c=-a*U0[i0]-b*U0[i1];
    d0=a*V0[i0]+b*V0[i1]+c;

    a=U2[i1]-U1[i1];
    b=-(U2[i0]-U1[i0]);
    c=-a*U1[i0]-b*U1[i1];
    d1=a*V0[i0]+b*V0[i1]+c;

    a=U0[i1]-U2[i1];
    b=-(U0[i0]-U2[i0]);
    c=-a*U2[i0]-b*U2[i1];
    d2=a*V0[i0]+b*V0[i1]+c;
    if(d0*d1>0.0)
	if(d0*d2>0.0) return 1;
    return 0;
}


int
coplanar_tri_tri(vect_t N, vect_t V0, vect_t V1, vect_t V2, vect_t U0, vect_t U1, vect_t U2)
{
    vect_t A;
    short i0, i1;
    /* first project onto an axis-aligned plane, that maximizes the area */
    /* of the triangles, compute indices: i0, i1. */
    A[0]=fabs(N[0]);
    A[1]=fabs(N[1]);
    A[2]=fabs(N[2]);
    if(A[0]>A[1]) {
	if(A[0]>A[2]) {
	    i0=1;      /* A[0] is greatest */
	    i1=2;
	} else {
	    i0=0;      /* A[2] is greatest */
	    i1=1;
	}
    } else {  /* A[0]<=A[1] */
	if(A[2]>A[1]) {
	    i0=0;      /* A[2] is greatest */
	    i1=1;
	} else {
	    i0=0;      /* A[1] is greatest */
	    i1=2;
	}
    }

    /* test all edges of triangle 1 against the edges of triangle 2 */
    if(edge_against_tri_edges(V0, V1, U0, U1, U2, i0, i1))return 1;
    if(edge_against_tri_edges(V1, V2, U0, U1, U2, i0, i1))return 1;
    if(edge_against_tri_edges(V2, V0, U0, U1, U2, i0, i1))return 1;

    /* finally, test if tri1 is totally contained in tri2 or vice versa */
    if(point_in_tri(V0, U0, U1, U2, i0, i1))return 1;
    if(point_in_tri(U0, V0, V1, V2, i0, i1))return 1;

    return 0;
}


HIDDEN int
tri_tri_intersect_with_isectline(struct soup_s *UNUSED(left), struct soup_s *UNUSED(right), struct face_s *lf, struct face_s *rf, int *coplanar, point_t *isectpt, const struct bn_tol *tol)
{
    vect_t D, isectpointA1={0}, isectpointA2={0}, isectpointB1={0}, isectpointB2={0};
    fastf_t d1, d2, du0, du1, du2, dv0, dv1, dv2, du0du1, du0du2, dv0dv1, dv0dv2, vp0, vp1, vp2, up0, up1, up2, b, c, max, isect1[2]={0, 0}, isect2[2]={0, 0};
    int i, smallest1=0, smallest2=0;

    /* compute plane equation of triangle(lf->vert[0], lf->vert[1], lf->vert[2]) */
    d1=-VDOT(lf->plane, lf->vert[0]);
    /* plane equation 1: lf->plane.X+d1=0 */

    /* put rf->vert[0], rf->vert[1], rf->vert[2] into plane equation 1 to compute signed distances to the plane*/
    du0=VDOT(lf->plane, rf->vert[0])+d1;
    du1=VDOT(lf->plane, rf->vert[1])+d1;
    du2=VDOT(lf->plane, rf->vert[2])+d1;

    du0du1=du0*du1;
    du0du2=du0*du2;

    if(du0du1>0.0f && du0du2>0.0f) /* same sign on all of them + not equal 0 ? */
	return 0;                    /* no intersection occurs */

    /* compute plane of triangle (rf->vert[0], rf->vert[1], rf->vert[2]) */
    d2=-VDOT(rf->plane, rf->vert[0]);
    /* plane equation 2: rf->plane.X+d2=0 */

    /* put lf->vert[0], lf->vert[1], lf->vert[2] into plane equation 2 */
    dv0=VDOT(rf->plane, lf->vert[0])+d2;
    dv1=VDOT(rf->plane, lf->vert[1])+d2;
    dv2=VDOT(rf->plane, lf->vert[2])+d2;

    dv0dv1=dv0*dv1;
    dv0dv2=dv0*dv2;

    if(dv0dv1>0.0f && dv0dv2>0.0f) /* same sign on all of them + not equal 0 ? */
	return 0;                    /* no intersection occurs */

    /* compute direction of intersection line */
    VCROSS(D, lf->plane, rf->plane);

    /* compute and i to the largest component of D */
    max=fabs(D[0]);
    i=0;
    b=fabs(D[1]);
    c=fabs(D[2]);
    if(b>max) max=b, i=1;
    if(c>max) max=c, i=2;

    /* this is the simplified projection onto L*/
    vp0=lf->vert[0][i];
    vp1=lf->vert[1][i];
    vp2=lf->vert[2][i];

    up0=rf->vert[0][i];
    up1=rf->vert[1][i];
    up2=rf->vert[2][i];

    /* compute interval for triangle 1 */
    *coplanar=compute_intervals_isectline(lf, vp0, vp1, vp2, dv0, dv1, dv2, dv0dv1, dv0dv2, &isect1[0], &isect1[1], isectpointA1, isectpointA2, tol);
    if(*coplanar)
	return coplanar_tri_tri(lf->plane, lf->vert[0], lf->vert[1], lf->vert[2], rf->vert[0], rf->vert[1], rf->vert[2]);

    /* compute interval for triangle 2 */
    compute_intervals_isectline(rf, up0, up1, up2, du0, du1, du2, du0du1, du0du2, &isect2[0], &isect2[1], isectpointB1, isectpointB2, tol);

    /* sort so that a<=b */
    smallest1 = smallest2 = 0;
#define SORT2(a, b, smallest) if(a>b) { fastf_t _c; _c=a; a=b; b=_c; smallest=1; }
    SORT2(isect1[0], isect1[1], smallest1);
    SORT2(isect2[0], isect2[1], smallest2);
#undef SORT2

    if(isect1[1]<isect2[0] || isect2[1]<isect1[0])
	return 0;

    /* at this point, we know that the triangles intersect */

    if (isect2[0] < isect1[0]) {
	if (smallest1 == 0)
	    VMOVE(isectpt[0], isectpointA1)
	else
	    VMOVE(isectpt[0], isectpointA2)
		if (isect2[1] < isect1[1])
		    if (smallest2 == 0)
			VMOVE(isectpt[1], isectpointB2)
		    else
			VMOVE(isectpt[1], isectpointB1)
		else if (smallest1 == 0)
		    VMOVE(isectpt[1], isectpointA2)
		else
		    VMOVE(isectpt[1], isectpointA1)
    } else {
	if (smallest2 == 0)
	    VMOVE(isectpt[0], isectpointB1)
	else
	    VMOVE(isectpt[0], isectpointB2)
		if (isect2[1] > isect1[1])
		    if (smallest1 == 0)
			VMOVE(isectpt[1], isectpointA2)
		    else
			VMOVE(isectpt[1], isectpointA1)
		else if (smallest2 == 0)
		    VMOVE(isectpt[1], isectpointB2)
		else
		    VMOVE(isectpt[1], isectpointB1)
    }
    return 1;
}


/**********************************************************************/

long int splitz = 0;
long int splitty = 0;

HIDDEN int
split_face_single(struct soup_s *s, unsigned long int fid, point_t isectpt[2], struct face_s *UNUSED(opp_face), const struct bn_tol *tol)
{
    struct face_s *f = s->faces+fid;
    int i, j, isv[2] = {0, 0};

#define VERT_INT 0x10
#define LINE_INT 0x20
#define FACE_INT 0x40
#define ALL_INT  (VERT_INT|LINE_INT|FACE_INT)

    /****** START hoistable ******/
    for(i=0;i<2;i++) for(j=0;j<3;j++) {
	if(isv[i] == 0) {
	    fastf_t dist;

	    switch( bn_isect_pt_lseg( &dist, (fastf_t *)&f->vert[j], (fastf_t *)&f->vert[j==2?0:j+1], (fastf_t *)&isectpt[i], tol) ) {
		case -2: case -1: continue;
		case 1: isv[i] = VERT_INT|j; break;
		case 2: isv[i] = VERT_INT|(j==2?0:j+1); break;
		case 3: isv[i] = LINE_INT|j; break;
		default: bu_log("Whu?\n"); break;
	    }
	}
    }

    /*** test if intersect is middle of face ***/
    for(i=0;i<2;i++)
	/* test for face in plane */
	if(isv[i] == 0)	/* assume that the isectpt is necessarily on the vert, line or
			   face... if it's not seen on the vert or line, it must be face.
			   This should probably be a real check. */
	    isv[i] = FACE_INT;

    if(isv[0] == 0 || isv[1] == 0) {
	/*
	   bu_log("Something real bad %x %x\n", isv[0], isv[1]);
	   */
	return -1;
    }

    if((isv[0]&ALL_INT) > (isv[1]&ALL_INT)) {
	int tmpi;
	point_t tmpp;
	VMOVE(tmpp, isectpt[0]);
	VMOVE(isectpt[0], isectpt[1]);
	VMOVE(isectpt[1], tmpp);
	tmpi = isv[0];
	isv[0]=isv[1];
	isv[1]=tmpi;
    }

    /****** END hoistable ******/

    /* test if both ends of the intersect line are on vertices */
    /* if VERT+VERT, abort */
    if((isv[0]&VERT_INT) && (isv[1]&VERT_INT))
	return 1;

    /* if VERT+LINE, break into 2 */
    if(isv[0]&VERT_INT && isv[1]&LINE_INT) {
	int k = isv[0]&~ALL_INT;
	soup_add_face_precomputed(s, f->vert[k], isectpt[1], f->vert[k==2?0:k+1], f->plane, 0);
	soup_add_face_precomputed(s, f->vert[k], f->vert[k==0?2:k-1], isectpt[1], f->plane, 0);
	soup_rm_face(s, fid);
	return 2;
    }
    return 0;

    /* if LINE+LINE, break into 3, figure out which side has two verts and cut * that */
    if(isv[0]&LINE_INT && isv[1]&LINE_INT) {
	bu_log("Splitting into 3 %x %x (LINE/LINE)\n", isv[0], isv[1]);
	return 3;
    }

    /* if VERT+FACE, break into 3, intersect is one line, other two to the * opposing verts */
    if(isv[0]&VERT_INT ) {
	bu_log("Splitting i nto 3: Vert+face? %x %x\n", isv[0], isv[1]);
	return 3;
    }

    /* if LINE+FACE, break into 3 */
    if(isv[0]&LINE_INT ) {
	/* test if face extends to vert? could be 2 at that point... */
	bu_log("Splitting into 3?: Line+face? %x %x\n", isv[0], isv[1]);
	return 3;
    }

    /* if FACE+FACE, break into 3 */
    if(isv[0]&FACE_INT ) {
	/* extend intersect line to triangle edges, could be 2 or 3? */
	bu_log("Splitting into 3?: face+face? %x %x\n", isv[0], isv[1]);
	return 3;
    }
#undef VERT_INT
#undef LINE_INT
#undef ALL_INT
#undef FACE_INT
    /* this should never be reached */
    bu_log("derp?\n");

    return 0;
}


/* returns 0 to continue, 1 if the left face was split, 2 if the right face was
 * split */
HIDDEN int
split_face(struct soup_s *left, unsigned long int left_face, struct soup_s *right, unsigned long int right_face, const struct bn_tol *tol) {
    struct face_s *lf, *rf;
    vect_t isectpt[2] = {{0, 0, 0}, {0, 0, 0}};
    int coplanar, r = 0;

    lf = left->faces+left_face;
    rf = right->faces+right_face;

    splitz++;
    if(tri_tri_intersect_with_isectline(left, right, lf, rf, &coplanar, (point_t *)isectpt, tol) != 0 && !VNEAR_EQUAL(isectpt[0], isectpt[1], tol->dist)) {
	splitty++;

	if(split_face_single(left, left_face, isectpt, &right->faces[right_face], tol) > 1) r|=0x1;
	if(split_face_single(right, right_face, isectpt, &left->faces[left_face], tol) > 1) r|=0x2;
    }

    return r;
}


HIDDEN struct soup_s *
bot2soup(struct rt_bot_internal *bot, const struct bn_tol *tol)
{
    struct soup_s *s;
    unsigned long int i;

    RT_BOT_CK_MAGIC(bot);

    if(bot->orientation != RT_BOT_CCW)
	bu_bomb("Bad orientation out of nmg_bot\n");

    s = bu_malloc(sizeof(struct soup_s), "bot soup");
    s->magic = SOUP_MAGIC;
    s->nfaces = 0;
    s->maxfaces = ceil(bot->num_faces / (double)faces_per_page) * faces_per_page;
    s->faces = bu_malloc(sizeof(struct face_s) * s->maxfaces, "bot soup faces");

    for(i=0;i<bot->num_faces;i++)
	soup_add_face(s, bot->vertices+3*bot->faces[i*3+0], bot->vertices+3*bot->faces[i*3+1], bot->vertices+3*bot->faces[i*3+2], tol);

    return s;
}


HIDDEN struct nmgregion *
soup2nmg(struct soup_s *soup, const struct bn_tol *UNUSED(tol))
{
    unsigned int i;
    struct nmgregion *r = NULL;
#if 0
    struct model *m;
    struct shell *s;
    struct vertex *vert[3], **f_vert[3];
#endif

    SOUP_CKMAG(soup);

    for(i=0; i < soup->nfaces; i++) {
	bu_log("  facet normal %f %f %f\n", V3ARGS(soup->faces[i].plane));
	bu_log("    outer loop\n");
	bu_log("      vertex %f %f %f\n", V3ARGS(soup->faces[i].vert[0]));
	bu_log("      vertex %f %f %f\n", V3ARGS(soup->faces[i].vert[1]));
	bu_log("      vertex %f %f %f\n", V3ARGS(soup->faces[i].vert[2]));
	bu_log("    endloop\n");
	bu_log("  endfacet\n");
    }
#if 0
    m = nmg_mmr();
    r = nmg_mrsv(m);
    s = BU_LIST_FIRST(shell, &r->s_hd);

    f_vert[0] = &vert[0];
    f_vert[1] = &vert[1];
    f_vert[2] = &vert[2];

    for(i=0; i < soup->nfaces; i++) {
	struct faceuse *fu;

	memset((char *)vert, 0, sizeof(vert));
	fu = nmg_cmface(s, f_vert, 3);
	nmg_vertex_gv(vert[0], soup->faces[i].vert[0]);
	nmg_vertex_gv(vert[1], soup->faces[i].vert[1]);
	nmg_vertex_gv(vert[2], soup->faces[i].vert[2]);
	nmg_calc_face_g(fu);

	if(nmg_fu_planeeqn(fu, tol))
	    bu_log("Tiny tri!\n");
    }

    if(nmg_kill_cracks(s))
	if(nmg_ks(s))
	    return NULL;
#endif

    return r;
}


HIDDEN void
free_soup(struct soup_s *s) {
    if(s == NULL)
	bu_bomb("null soup");
    if(s->faces)
	bu_free(s->faces, "bot soup faces");
    bu_free(s, "bot soup");
    return;
}


HIDDEN union tree *
invert(union tree *tree)
{
    struct soup_s *s;
    unsigned long int i;

    RT_CK_TREE(tree);
    if(tree->tr_op != OP_NMG_TESS) {
	bu_log("Erm, this isn't an nmg tess\n");
	return tree;
    }
    s = (struct soup_s *)tree->tr_d.td_r->m_p;
    SOUP_CKMAG(s);

    for(i=0;i<s->nfaces;i++) {
	struct face_s *f = s->faces+i;
	point_t t;
	VMOVE(t, f->vert[0]);
	VMOVE(f->vert[0], f->vert[1]);
	VMOVE(f->vert[0], t);
	/* flip the inverted bit. */
	f->foo^=INVERTED;
    }

    return tree;
}


HIDDEN void
split_faces(union tree *left_tree, union tree *right_tree, const struct bn_tol *tol)
{
    struct soup_s *l, *r;
    unsigned long int i, j;

    RT_CK_TREE(left_tree);
    RT_CK_TREE(right_tree);
    l = (struct soup_s *)left_tree->tr_d.td_r->m_p;
    r = (struct soup_s *)right_tree->tr_d.td_r->m_p;
    SOUP_CKMAG(l);
    SOUP_CKMAG(r);

    /* this is going to be big and hairy. Has to walk both meshes finding
     * all intersections and split intersecting faces so there are edges at
     * the intersections. Initially going to be O(n^2), then change to use
     * space partitioning (binning? octree? kd?). */
    for(i=0;i<l->nfaces;i++) {
	struct face_s *lf = l->faces+i, *rf = NULL;
	int ret;

	for(j=0;j<r->nfaces;j++) {
	    rf = r->faces+j;
	    /* quick bounding box test */
	    if(lf->min[X]>rf->max[X] || lf->max[X]>lf->max[X] ||
	       lf->min[Y]>rf->max[Y] || lf->max[Y]>lf->max[Y] ||
	       lf->min[Z]>rf->max[Z] || lf->max[Z]>lf->max[Z])
		continue;
	    /* two possibly overlapping faces found */
	    ret = split_face(l, i, r, j, tol);
	    if(ret&0x1) i--;
	    if(ret&0x2) j--;
	}
    }
}


HIDDEN union tree *
compose(union tree *left_tree, union tree *right_tree, unsigned long int face_status1, unsigned long int face_status2, unsigned long int face_status3)
{
    struct soup_s *l, *r;
    int i;

    RT_CK_TREE(left_tree);
    RT_CK_TREE(right_tree);
    l = (struct soup_s *)left_tree->tr_d.td_r->m_p;
    r = (struct soup_s *)right_tree->tr_d.td_r->m_p;

    /* remove unnecessary faces and compose a single new internal */
    for(i=l->nfaces;i<=0;i--)
	if(l->faces[i].foo != face_status1)
	    soup_rm_face(l, i);
    for(i=r->nfaces;i<=0;i--)
	if(r->faces[i].foo != face_status3)
	    soup_rm_face(l, i);
    face_status1=face_status2=face_status3;

    free_soup(r);
    bu_free(right_tree, "union tree");
    return left_tree;
}


HIDDEN union tree *
evaluate(union tree *tr, const struct rt_tess_tol *ttol, const struct bn_tol *tol)
{
    RT_CK_TREE(tr);

    switch(tr->tr_op) {
	case OP_NOP:
	    return tr;
	case OP_NMG_TESS:
	    /* ugh, keep it as nmg_tess and just shove the rt_bot_internal ptr
	     * in as nmgregion. :/ Also, only doing the first shell of the first
	     * model. Primitives should only provide a single shell, right? */
	    {
		struct rt_db_internal ip;
		struct nmgregion *nmgr = BU_LIST_FIRST(nmgregion, &tr->tr_d.td_r->m_p->r_hd);
		/* the bot temporary format may be unnecessary if we can walk
		 * the nmg shells and generate soup from them directly. */
		struct rt_bot_internal *bot = nmg_bot(BU_LIST_FIRST(shell, &nmgr->s_hd), tol);

		/* causes a crash.
		   nmg_kr(nmgr);
		   free(nmgr);
		*/

		tr->tr_d.td_r->m_p = (struct model *)bot2soup(bot, tol);
		SOUP_CKMAG((struct soup_s *)tr->tr_d.td_r->m_p);

		/* fill in a db_internal with our new bot so we can free it */
		RT_DB_INTERNAL_INIT(&ip);
		ip.idb_major_type = DB5_MAJORTYPE_BRLCAD;
		ip.idb_minor_type = ID_BOT;
		ip.idb_meth = &rt_functab[ID_BOT];
		ip.idb_ptr = bot;
		ip.idb_meth->ft_ifree(&ip);
	    }
	    return tr;
	case OP_UNION:
	case OP_INTERSECT:
	case OP_SUBTRACT:
	    RT_CK_TREE(tr->tr_b.tb_left);
	    RT_CK_TREE(tr->tr_b.tb_right);
	    tr->tr_b.tb_left = evaluate(tr->tr_b.tb_left, ttol, tol);
	    tr->tr_b.tb_right = evaluate(tr->tr_b.tb_right, ttol, tol);
	    RT_CK_TREE(tr->tr_b.tb_left);
	    RT_CK_TREE(tr->tr_b.tb_right);
	    SOUP_CKMAG(tr->tr_b.tb_left->tr_d.td_r->m_p);
	    SOUP_CKMAG(tr->tr_b.tb_right->tr_d.td_r->m_p);
	    split_faces(tr->tr_b.tb_left, tr->tr_b.tb_right, tol);
	    RT_CK_TREE(tr->tr_b.tb_left);
	    RT_CK_TREE(tr->tr_b.tb_right);
	    SOUP_CKMAG(tr->tr_b.tb_left->tr_d.td_r->m_p);
	    SOUP_CKMAG(tr->tr_b.tb_right->tr_d.td_r->m_p);
	    break;
	default:
	    bu_bomb("bottess evaluate(): bad op (first pass)\n");
    }

    switch(tr->tr_op) {
	case OP_UNION:
	    return compose(tr->tr_b.tb_left, tr->tr_b.tb_right, OUTSIDE, SAME, OUTSIDE);
	case OP_INTERSECT:
	    return compose(tr->tr_b.tb_left, tr->tr_b.tb_right, INSIDE, SAME, INSIDE);
	case OP_SUBTRACT:
	    return invert(compose(tr->tr_b.tb_left, invert(tr->tr_b.tb_right), OUTSIDE, OPPOSITE, INSIDE));
	default:
	    bu_bomb("bottess evaluate(): bad op (second pass, CSG)\n");
    }
    bu_bomb("Got somewhere I shouldn't have\n");
    return NULL;
}


long int lsplitz=0;
long int lsplitty=0;

union tree *
gcv_bottess_region_end(struct db_tree_state *tsp, const struct db_full_path *pathp, union tree *curtree, genptr_t client_data)
{
    union tree *ret_tree;
    void (*write_region)(struct nmgregion *, const struct db_full_path *, int, int, float [3]);

    if (!tsp || !curtree || !pathp || !client_data) {
	bu_log("INTERNAL ERROR: gcv_region_end missing parameters\n");
	return TREE_NULL;
    }

    write_region = ((struct gcv_data *)client_data)->func;
    if (!write_region) {
	bu_log("INTERNAL ERROR: gcv_region_end missing conversion callback function\n");
	return TREE_NULL;
    }

    RT_CK_FULL_PATH(pathp);
    RT_CK_TREE(curtree);
    RT_CK_TESS_TOL(tsp->ts_ttol);
    BN_CK_TOL(tsp->ts_tol);
    NMG_CK_MODEL(*tsp->ts_m);

    splitz=0;
    splitty=0;

    if (!tsp || !curtree || !pathp || !client_data) {
	bu_log("INTERNAL ERROR: gcv_bottess_region_end missing parameters\n");
	return TREE_NULL;
    }

    RT_CK_FULL_PATH(pathp);
    RT_CK_TREE(curtree);
    RT_CK_TESS_TOL(tsp->ts_ttol);
    BN_CK_TOL(tsp->ts_tol);
    NMG_CK_MODEL(*tsp->ts_m);

    if (curtree->tr_op == OP_NOP)
	return curtree;

    bu_log("solid %s\n", db_path_to_string(pathp));
    ret_tree = evaluate(curtree, tsp->ts_ttol, tsp->ts_tol);
    soup2nmg((struct soup_s *)ret_tree->tr_d.td_r->m_p, tsp->ts_tol);
    bu_log("endsolid %s\n", db_path_to_string(pathp));

    lsplitz+=splitz;
    lsplitz+=splitz;
    lsplitty+=splitty;
    splitz=0;
    splitty=0;

#if 0
    curtree->tr_d.td_r = soup2nmg((struct soup_s *)ret_tree->tr_d.td_r->m_p, tsp->ts_tol);
    curtree->tr_op = OP_NMG_TESS;

    write_region(curtree->tr_d.td_r, pathp, tsp->ts_regionid, tsp->ts_gmater, tsp->ts_mater.ma_color);
#endif

    return NULL;
}


union tree *
gcv_bottess(int argc, const char **argv, struct db_i *dbip, struct rt_tess_tol *ttol)
{
    struct db_tree_state tree_state = rt_initial_tree_state;
    tree_state.ts_ttol = ttol;

    db_walk_tree(dbip, argc, argv, 1, &tree_state, NULL, gcv_bottess_region_end, nmg_booltree_leaf_tess, NULL);

    return NULL;
}


/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
