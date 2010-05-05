/*                         P H O N G . H
 * BRL-CAD / ADRT
 *
 * Copyright (c) 2007-2010 United States Government as represented by
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
/** @file phong.h
 *
 */

#ifndef _RENDER_PHONG_H
#define _RENDER_PHONG_H

#include "render_internal.h"

BU_EXPORT BU_EXTERN(void render_phong_init, (render_t *render, char *usr));
BU_EXPORT BU_EXTERN(void render_phong_free, (render_t *render));
BU_EXPORT BU_EXTERN(void render_phong_work, (render_t *render, tie_t *tie, tie_ray_t *ray, TIE_3 *pixel));

#endif

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
