/*
 * File:	cluster.c
 * Description:	Routines for building a VDS vertex tree using a VERY simple
 *		octree-based clustering scheme.
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "geom.h"
#include "vds.h"
#include "vector.h"


/*
 * Function:	clusterOctree
 * Description:	Builds an octree over the given leaf nodes using
 *		vdsClusterNodes().  Takes an array <nodes> of vdsNode pointers
 *		that represent vertices in the original model (i.e., leaf nodes
 *		in the vertex tree to be generated).  This array is partitioned
 *		into eight subarrays by splitting across the x, y, and z
 *		midplanes of the tightest-fitting bounding cube, and
 *		clusterOctree() is called recursively on each subarray.
 *		Finally, vdsClusterNodes() is called on the 2-8 nodes returned
 *		by these recursive calls, and clusterOctree returns the newly
 *		created internal node.
 */
vdsNode *clusterOctree(vdsNode **nodes, int nnodes, int depth)
{
    vdsNode *thisnode;
    vdsNode *children[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
    int nchildren = 0;
    vdsNode **childnodes[8];
    int nchildnodes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int i, j;
    vdsVec3 min, max, center, average = {0, 0, 0};

    assert(depth < VDS_MAXDEPTH);
    /* Overestimate array size needs for childnodes now; shrink later */
    for (i = 0; i < 8; i++) {
	childnodes[i] = (vdsNode **) malloc(sizeof(vdsNode *) * nnodes);
	assert(childnodes[i] != NULL);
    }
    /* Find the min and max bounds of nodes, and accumulate average coord */
    VEC3_COPY(min, nodes[0]->coord);
    VEC3_COPY(max, nodes[0]->coord);
    for (i = 0; i < nnodes; i++) {
	for (j = 0; j < 3; j++) {
	    if (nodes[i]->coord[j] > max[j]) {
		max[j] = nodes[i]->coord[j];
	    }
	    if (nodes[i]->coord[j] < min[j]) {
		min[j] = nodes[i]->coord[j];
	    }
	}
	VEC3_ADD(average, average, nodes[i]->coord);
    }
    VEC3_SCALE(average, 1.0 / (float) nnodes, average);
    VEC3_AVERAGE(center, min, max);
    if (VEC3_EQUAL(min, max)) {
	printf("Coincident vertices; partitioning among child nodes.\n");
	for (i = 0; i < nnodes; i++) {
	    int index = i % VDS_MAXDEGREE;
	    childnodes[index][nchildnodes[index]] = nodes[i];
	    nchildnodes[index] ++;
	}
    } else {
	/* Partition the nodes among the 8 octants defined by min and max */
	for (i = 0; i < nnodes; i++) {
	    int whichchild = 0;
	    if (nodes[i]->coord[0] > center[0]) {
		whichchild |= 1;
	    }
	    if (nodes[i]->coord[1] > center[1]) {
		whichchild |= 2;
	    }
	    if (nodes[i]->coord[2] > center[2]) {
		whichchild |= 4;
	    }

	    childnodes[whichchild][nchildnodes[whichchild]] = nodes[i];
	    nchildnodes[whichchild] ++;
	}
    }
    /* Resize childnodes arrays to use only as much space as necessary */
    for (i = 0; i < 8; i++) {
	childnodes[i] = (vdsNode **)
		realloc(childnodes[i], sizeof(vdsNode *) * nchildnodes[i]);
    }
    /* Recurse or store non-empty children */
    for (i = 0; i < 8; i++) {
	if (nchildnodes[i] > 0) {
	    if (nchildnodes[i] == 1) {	/* 1 node in octant; store directly  */
		children[nchildren] = childnodes[i][0];
	    } else {		/* 2 or more nodes in octant; recurse*/
		children[nchildren] =
		    clusterOctree(childnodes[i], nchildnodes[i], depth + 1);
	    }
	    nchildren++;
	}

    }
    /* Finally, cluster nonempty children; this node is the resulting parent */
    thisnode = vdsClusterNodes(nchildren, children,
		       average[0], average[1], average[2]);
    for (i = 0; i < 8; i++) {
	if (nchildnodes[i]) {
	    free(childnodes[i]);
	}
    }
    return thisnode;
}

/*
 * Function:	generateVertexTree
 * Description:	Given the triangles and vertices listed in the object <obj>,
 * 		generates obj->vertexTree.  Calls vdsAddNode() on all vertices
 *		in obj->v, vdsAddTri() on all triangles in obj->t, and calls
 *		clusterOctree() to cluster the nodes with vdsClusterNodes().
 *		Lastly, calls vdsEndVertexTree() to finalize the vertex tree.
 */
void generateVertexTree(object *obj)
{
    vdsNode *leafnodes;
    vdsNode **leafptrs;
    int i;

    vdsBeginVertexTree();
    vdsBeginGeometry();
    for (i = 0; i < obj->nv; i++) {
	vdsAddNode(obj->v[i].coord[0], obj->v[i].coord[1], obj->v[i].coord[2]);
    }
    for (i = 0; i < obj->nt; i++) {
	vdsByte3 grey = {190, 190, 190};
	triangle *t = &obj->t[i];
	vdsVec3 n0, n1, n2;
	vertex *v0, *v1, *v2;

	v0 = &obj->v[t->verts[0] - 1];
	v1 = &obj->v[t->verts[1] - 1];
	v2 = &obj->v[t->verts[2] - 1];
	VEC3_COPY(n0, v0->hasNormal ? v0->normal : t->normal);
	VEC3_COPY(n1, v1->hasNormal ? v1->normal : t->normal);
	VEC3_COPY(n2, v2->hasNormal ? v2->normal : t->normal);
	vdsAddTri(t->verts[0] - 1, t->verts[1] - 1, t->verts[2] - 1,
		  n0, n1, n2, grey, grey, grey);
    }
    leafnodes = vdsEndGeometry();
    leafptrs = (vdsNode **) malloc(sizeof(vdsNode *) * obj->nv);
    for (i = 0; i < obj->nv; i++) {
	leafptrs[i] = &leafnodes[i];
    }
    clusterOctree(leafptrs, obj->nv, 0);
    obj->vertexTree = vdsEndVertexTree();
    free(leafptrs);
}
