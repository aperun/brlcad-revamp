#include "component.h"
#include "hit.h"
#include "adrt_common.h"
#include <stdio.h>


void render_component_init(render_t *render) {
  render->work = render_component_work;
  render->free = render_component_free;
}


void render_component_free(render_t *render) {
}


static void* component_hit(tie_ray_t *ray, tie_id_t *id, tie_tri_t *tri, void *ptr) {
  common_triangle_t *t = ((common_triangle_t *)(tri->ptr));

  ray->depth++;
  if(t->mesh->flags & 0x3)
    return(t->mesh);

  return(0);
}


void render_component_work(render_t *render, tie_t *tie, tie_ray_t *ray, TIE_3 *pixel) {
  tie_id_t id;
  common_mesh_t	*m;
  TIE_3 vec;
  tfloat angle;



  if((m = (common_mesh_t *)tie_work(tie, ray, &id, component_hit, NULL))) {
    /* Flip normal to face ray origin (via dot product check) */
    if(ray->dir.v[0] * id.norm.v[0] + ray->dir.v[1] * id.norm.v[1] + ray->dir.v[2] * id.norm.v[2] > 0)
      MATH_VEC_MUL_SCALAR(id.norm, id.norm, -1.0);

    /* shade solid */
    pixel->v[0] = m->flags & 0x1 ? 0.8 : 0.2;
    pixel->v[1] = 0.2;
    pixel->v[2] = m->flags & 0x2 ? 0.8 : 0.2;
    MATH_VEC_SUB(vec, ray->pos, id.pos);
    MATH_VEC_UNITIZE(vec);
    MATH_VEC_DOT(angle, vec, id.norm);
    MATH_VEC_MUL_SCALAR((*pixel), (*pixel), (angle*0.8));
  } else if(ray->depth) {
    pixel->v[0] += 0.2;
    pixel->v[1] += 0.2;
    pixel->v[2] += 0.2;
  }
}
