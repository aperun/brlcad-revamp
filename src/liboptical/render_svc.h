/*                    R E N D E R _ S V C . H
 * BRL-CAD
 *
 * Copyright (c) 2012 United States Government as represented by
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

#ifndef RENDER_SVC_H
#define RENDER_SVC_H

#include <map>

#include "oslexec.h"

#ifdef OSL_NAMESPACE
namespace OSL_NAMESPACE {
#endif

namespace OSL {

using namespace OSL;


class SimpleRenderer : public RendererServices
{
public:
    // Just use 4x4 matrix for transformations
    typedef Matrix44 Transformation;

    SimpleRenderer () { }
    ~SimpleRenderer () { }

    virtual bool get_matrix (Matrix44 &result, TransformationPtr xform,
			     float time);
    virtual bool get_matrix (Matrix44 &result, ustring from, float time);

    virtual bool get_matrix (Matrix44 &result, TransformationPtr xform);
    virtual bool get_matrix (Matrix44 &result, ustring from);

    void name_transform (const char *name, const Transformation &xform);

    virtual bool get_array_attribute (void *renderstate, bool derivatives,
				      ustring object, TypeDesc type, ustring name,
				      int index, void *val );
    virtual bool get_attribute (void *renderstate, bool derivatives, ustring object,
				TypeDesc type, ustring name, void *val);
    virtual bool get_userdata (bool derivatives, ustring name, TypeDesc type,
			       void *renderstate, void *val);
    virtual bool has_userdata (ustring name, TypeDesc type, void *renderstate);
    virtual void *get_pointcloud_attr_query (ustring *attr_names,
					     TypeDesc *attr_types, int nattrs);
    virtual int  pointcloud (ustring filename, const OSL::Vec3 &center, float radius,
			     int max_points, void *attr_query, void **attr_outdata);

    virtual int pointcloud_search (ustring filename, const OSL::Vec3 &center,
				   float radius, int max_points, size_t *out_indices,
				   float *out_distances, int derivs_offset);

    virtual int pointcloud_get (ustring filename, size_t *indices, int count,
				ustring attr_name, TypeDesc attr_type,
				void *out_data);
private:
    typedef std::map <ustring, shared_ptr<Transformation> > TransformMap;
    TransformMap m_named_xforms;

#if 0
    // Example cached data for pointcloud implementation with Partio

    // OSL gets pointers to this but its definition is private.
    // Right now it only caches the types already converted to
    // Partio constants. This is what get_pointcloud_attr_query
    // returns
    struct AttrQuery
    {
	// Names of the attributes to query
	std::vector<ustring> attr_names;
	// Types as (enum Partio::ParticleAttributeType) of the
	// attributes in the query
	std::vector<int>     attr_partio_types;
	// For sanity checks, capacity of the output arrays
	int                  capacity;
    };

    // We will left this function as an exercise. It is only responsible
    // for loading the point cloud if it is not already in memory and so
    // on. You might or might not use Partio cache system
    Partio::ParticleData *get_pointcloud (ustring filename);

    // Keep a list so adding elements doesn't invalidate pointers.
    // Careful, don't use a vector here!
    std::list<AttrQuery> m_attr_queries;
#endif
};


}; // namespace OSL

#ifdef OSL_NAMESPACE
}; // end namespace OSL_NAMESPACE
using namespace OSL_NAMESPACE;
#endif


#endif /* RENDER_SVC_H */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
