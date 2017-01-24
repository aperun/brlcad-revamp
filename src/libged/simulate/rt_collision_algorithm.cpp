/*      R T _ C O L L I S I O N _ A L G O R I T H M . C P P
 * BRL-CAD
 *
 * Copyright (c) 2014-2017 United States Government as represented by
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
/** @file rt_collision_algorithm.cpp
 *
 * A librt-based collision algorithm for RtCollisionShape pairs.
 *
 */


#include "common.h"


#ifdef HAVE_BULLET


#include "rt_collision_algorithm.hpp"
#include "rt_motion_state.hpp"
#include "utility.hpp"

#include "bu/log.h"
#include "bu/malloc.h"
#include "rt/pattern.h"
#include "rt/tol.h"

#include <cmath>


namespace
{


static const bool use_persistent_contacts = false;


HIDDEN btVector3
get_normal_world_on_b(const btRigidBody &body_a, const btRigidBody &body_b)
{
    // calculate the normal of the contact points as the resultant of the velocities -A and B
    const btVector3 v_a = body_a.getLinearVelocity();
    const btVector3 v_b = body_b.getLinearVelocity();

    btVector3 result(0.0, 0.0, 0.0);

    if (!v_b.fuzzyZero())
	result = v_b.normalized();

    if (!v_a.fuzzyZero())
	result -= v_a.normalized();

    if (!result.fuzzyZero())
	result.normalize();
    else {
	// near-zero velocities for both objects
	// objects are nearly motionless and may be either near each other or overlapping
	// select a random direction at this iteration
	// TODO: implement a better strategy

	const btScalar z = drand48();
	const btScalar q = std::sqrt(1.0 - z * z);
	const btScalar theta = drand48() * 2.0 * M_PI;
	result = btVector3(q * std::cos(theta), q * std::sin(theta), z);
    }

    return result;
}


HIDDEN std::pair<btVector3, btVector3>
get_overlap_aabb(const btRigidBody &body_a, const btRigidBody &body_b)
{
    std::pair<btVector3, btVector3> result(btVector3(0.0, 0.0, 0.0), btVector3(0.0,
					   0.0, 0.0));
    body_a.getAabb(result.first, result.second);

    btVector3 body_b_aabb_min(0.0, 0.0, 0.0), body_b_aabb_max(0.0, 0.0, 0.0);
    body_b.getAabb(body_b_aabb_min, body_b_aabb_max);
    result.second.setMin(body_b_aabb_max);
    result.first.setMax(body_b_aabb_min);

    return result;
}


HIDDEN void
free_xrays(xrays * const rays)
{
    BU_CK_LIST_HEAD(rays);
    RT_CK_RAY(&rays->ray);

    bu_list_free(&rays->l);
    bu_free(rays, "rays");
}


HIDDEN xrays *
generate_ray_grid(const btVector3 &center, const btScalar radius,
		  const btVector3 &normal)
{
    if (radius < 0.0 || !NEAR_EQUAL(normal.length(), 1.0, RT_DOT_TOL))
	bu_bomb("invalid argument");

    xrays * const result = static_cast<xrays *>(bu_malloc(sizeof(xrays), "result"));
    BU_LIST_INIT(&result->l);

    xray &center_ray = result->ray;
    center_ray.magic = RT_RAY_MAGIC;
    center_ray.index = 0;
    VMOVE(center_ray.r_pt, center);
    VMOVE(center_ray.r_dir, normal);

    // calculate the 'up' vector
    vect_t up_vect = VINIT_ZERO;
    {
	btVector3 up = normal.cross(btVector3(1.0, 0.0, 0.0));

	// use the y-axis if parallel to x-axis
	if (up.fuzzyZero())
	    up = normal.cross(btVector3(0.0, 1.0, 0.0));

	VMOVE(up_vect, up);
    }

    // NOTE: Bullet's collision tolerance is 4 units (4mm)
    const btScalar minimum_grid_size = 0.25; // arbitrary
    const btScalar grid_size = std::max(minimum_grid_size,
					radius / static_cast<btScalar>(1.0e1));

    rt_gen_circular_grid(result, &center_ray, radius, up_vect, grid_size);
    return result;
}


HIDDEN xrays *
get_rays(const btRigidBody &body_a, const btRigidBody &body_b)
{
    const btVector3 normal_world_on_b = get_normal_world_on_b(body_a, body_b);
    const std::pair<btVector3, btVector3> overlap_aabb = get_overlap_aabb(body_a,
	    body_b);

    // radius of the circle of rays
    // half of the diagonal of the overlap rpp so that rays will cover
    // the entire volume from all orientations
    const btScalar radius = (overlap_aabb.second - overlap_aabb.first).length() /
			    2.0;

    // step back from the overlap center, along the normal by `radius`,
    // to ensure that rays start from outside of the overlap region
    const btVector3 center = (overlap_aabb.first + overlap_aabb.second) / 2.0 -
			     radius * normal_world_on_b;

    return generate_ray_grid(center, radius, normal_world_on_b);
}


HIDDEN void
calculate_contact_points(btManifoldResult &result,
			 const btCollisionObjectWrapper &body_a_wrap,
			 const btCollisionObjectWrapper &body_b_wrap,
			 const simulate::RtInstance &rt_instance,
			 btIDebugDraw &debug_draw)
{
    const btRigidBody &body_a = *btRigidBody::upcast(
				    body_a_wrap.getCollisionObject());
    const btRigidBody &body_b = *btRigidBody::upcast(
				    body_b_wrap.getCollisionObject());
    const std::string &body_a_path = static_cast<const simulate::RtMotionState *>
				     (body_a.getMotionState())->get_path();
    const std::string &body_b_path = static_cast<const simulate::RtMotionState *>
				     (body_b.getMotionState())->get_path();

    simulate::AutoPtr<xrays, free_xrays> rays(get_rays(body_a, body_b));
    const std::vector<std::pair<btVector3, btVector3> > overlaps =
	rt_instance.get_overlaps(body_a_path, body_b_path, *rays.ptr);
    const btVector3 normal_world_on_b = btVector3(V3ARGS(rays.ptr->ray.r_dir));

    for (std::vector<std::pair<btVector3, btVector3> >::const_iterator it =
	     overlaps.begin(); it != overlaps.end(); ++it) {
	const btScalar depth = -(it->first - it->second).length();
	result.addContactPoint(normal_world_on_b, it->second, depth);
    }

    if (debug_draw.getDebugMode() & btIDebugDraw::DBG_DrawFrames) {
	xrays *entry = NULL;

	for (BU_LIST_FOR(entry, xrays, &rays.ptr->l)) {
	    const btVector3 point(V3ARGS(entry->ray.r_pt));
	    const btVector3 direction(V3ARGS(entry->ray.r_dir));

	    debug_draw.drawLine(point, point + direction * 4.0,
				debug_draw.getDefaultColors().m_contactPoint);
	}
    }
}


}


namespace simulate
{


RtCollisionAlgorithm::CreateFunc::CreateFunc(const RtInstance &rt_instance,
	btIDebugDraw &debug_draw) :
    m_rt_instance(rt_instance),
    m_debug_draw(debug_draw)
{}


btCollisionAlgorithm *
RtCollisionAlgorithm::CreateFunc::CreateCollisionAlgorithm(
    btCollisionAlgorithmConstructionInfo &cinfo,
    const btCollisionObjectWrapper * const body_a_wrap,
    const btCollisionObjectWrapper * const body_b_wrap)
{
    if (!body_a_wrap || !body_b_wrap)
	bu_bomb("missing argument");

    void * const ptr = cinfo.m_dispatcher1->allocateCollisionAlgorithm(sizeof(
			   RtCollisionAlgorithm));
    return new(ptr) RtCollisionAlgorithm(NULL, cinfo, body_a_wrap, body_b_wrap,
					 m_rt_instance, m_debug_draw);
}


RtCollisionAlgorithm::RtCollisionAlgorithm(
    btPersistentManifold * const manifold,
    const btCollisionAlgorithmConstructionInfo &cinfo,
    const btCollisionObjectWrapper * const body_a_wrap,
    const btCollisionObjectWrapper * const body_b_wrap,
    const RtInstance &rt_instance, btIDebugDraw &debug_draw) :
    btCollisionAlgorithm(cinfo),
    m_owns_manifold(false),
    m_manifold(manifold),
    m_rt_instance(rt_instance),
    m_debug_draw(debug_draw)
{
    if (!body_a_wrap || !body_b_wrap)
	bu_bomb("missing argument");

    const btCollisionObject &body_a = *body_a_wrap->getCollisionObject();
    const btCollisionObject &body_b = *body_b_wrap->getCollisionObject();

    if (!m_manifold && m_dispatcher->needsCollision(&body_a, &body_b)) {
	m_manifold = m_dispatcher->getNewManifold(&body_a, &body_b);
	m_owns_manifold = true;
    }
}


RtCollisionAlgorithm::~RtCollisionAlgorithm()
{
    if (m_owns_manifold && m_manifold)
	m_dispatcher->releaseManifold(m_manifold);
}


void
RtCollisionAlgorithm::processCollision(const btCollisionObjectWrapper * const
				       body_a_wrap,
				       const btCollisionObjectWrapper * const body_b_wrap,
				       const btDispatcherInfo &, btManifoldResult * const result)
{
    if (!body_a_wrap || !body_b_wrap || !result)
	bu_bomb("missing argument");

    if (!m_manifold)
	bu_bomb("missing manifold");

    result->setPersistentManifold(m_manifold);

    if (!use_persistent_contacts)
	m_manifold->clearManifold();

    calculate_contact_points(*result, *body_a_wrap, *body_b_wrap, m_rt_instance,
			     m_debug_draw);

    if (use_persistent_contacts && m_owns_manifold)
	result->refreshContactPoints();
}


btScalar
RtCollisionAlgorithm::calculateTimeOfImpact(btCollisionObject * const body_a,
	btCollisionObject * const body_b, const btDispatcherInfo &UNUSED(dispatch_info),
	btManifoldResult * const result)
{
    if (!body_a || !body_b || !result)
	bu_bomb("missing argument");

    return 1.0;
}


void
RtCollisionAlgorithm::getAllContactManifolds(btManifoldArray &manifold_array)
{
    if (m_owns_manifold && m_manifold)
	manifold_array.push_back(m_manifold);
}


}


#endif


// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
