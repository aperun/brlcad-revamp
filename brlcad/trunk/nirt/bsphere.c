/*	BSPHERE.C	*/
#ifndef lint
static char RCSid[] = "$Header$";
#endif

/*	INCLUDES	*/
#include	<stdio.h>
#include	<math.h>
#include	<machine.h>
#include	<vmath.h>
#include	<raytrace.h>
#include	"./nirt.h"
#include	"./usrfmt.h"

fastf_t	bsphere_radius;

void set_radius(rtip)

struct rt_i	*rtip;

{
    vect_t	diag;

    VSUB2(diag, rtip -> mdl_max, rtip -> mdl_min);
    bsphere_radius = MAGNITUDE(diag) * 0.5;
}
