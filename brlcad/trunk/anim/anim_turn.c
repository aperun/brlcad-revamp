/*			A N I M _ T U R N . C
 *
 *	Animate front-wheel steered vehicles.
 *
 *  This is a filter which operates on animation tables. Given an
 *  animation table for the position of the front axle, anim_turn produces
 *  an animation table for position and orientation. Options provide for
 *  animating the wheels and/or steering wheel.
 * 
 *  Author -
 *	Carl J. Nuzman
 *  
 *  Source -
 *      The U. S. Army Research Laboratory
 *      Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Notice -
 *      Re-distribution of this software is restricted, as described in
 *      your "Statement of Terms and Conditions for the Release of
 *      The BRL-CAD Pacakge" agreement.
 *
 *  Copyright Notice -
 *      This software is Copyright (C) 1993 by the United States Army
 *      in all countries except the USA.  All rights reserved.
 */

#include "conf.h"
#include <math.h>
#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "anim.h"

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif


extern int optind;
extern char *optarg;

int print_int = 1;
int angle_set = 0;
int turn_wheels = 0;
fastf_t length, angle, radius;
fastf_t factor = 1.0;

main(argc,argv)
int argc;
char **argv;
{
	int count;
	fastf_t val, time, roll_ang, yaw,sign;
	vect_t v, point, front, back, zero, temp1, temp2;
	mat_t m_from_world, m_to_world;
	double mat_atan2();

	VSETALL(zero, 0.0);
	length = angle = radius = roll_ang = 0.0;

	if (!get_args(argc,argv))
		fprintf(stderr,"ascript: Get_args error");

	if (!angle_set) { /* set angle if not yet done */
		scanf("%*f%*[^-0123456789]");
		VSCAN(temp1);
		scanf("%*f%*[^-0123456789]");
		VSCAN(temp2);
		angle = mat_atan2( (temp2[1]-temp1[1]),(temp2[0]-temp1[0]) );
		rewind(stdin);
	}
	count = 0;
	while (1) {
		/* read one line of table */
		val = scanf("%lf%*[^-0123456789]",&time); /*read time,ignore garbage*/
		val = scanf("%lf %lf %lf", point, point+1, point +2);
		if (val < 3) {
			break;
		}

		/*update to and from matrices */

		if (count) { /* not first time through */
			/* calculate matrices corrsponding to last position*/
			y_p_r2mat(m_to_world,angle,0.0,0.0);
			add_trans(m_to_world,front,zero);
			y_p_r2mat(m_from_world,-angle,0.0,0.0);
			VREVERSE(temp1,front);
			add_trans(m_from_world,zero,temp1);
		
			/* calculate new position for front and back axles */
			/* front goes to the point, back slides along objects*/
			/* current front to back axis */
			MAT4X3PNT(v,m_from_world,point);/* put point in vehicle coordinates*/
			if (v[1] > length) {
				fprintf(stderr,"anim_turn: Distance between positions greater than length of vehicle - ABORTING\n");
				break;
			}
			temp2[0] = v[0] - sqrt(length*length - v[1]*v[1]); /*calculate back*/
			temp2[1] = temp2[2] = 0.0;
			MAT4X3PNT(back,m_to_world,temp2);/*put "back" in world coordinates*/
			VMOVE(front,point);

			/*calculate new angle of vehicle*/
			VSUB2(temp1,front,back);
			angle = mat_atan2(temp1[1],temp1[0]);
		}
		else { /*first time through */
			/*angle is already determined*/
			VMOVE(front, point);
		}

		/*calculate turn angles and print table*/
		
		if (turn_wheels){ 
			if (v[0] >= 0)
				sign = 1.0;
			else
				sign = -1.0;
			yaw = mat_atan2(sign*v[1],sign*v[0]);
			if (radius > VDIVIDE_TOL)
				roll_ang -= sign * MAGNITUDE(v) / radius;

			if (!(count%print_int))
				printf("%f %f %f 0.0\n",time,factor*RTOD*yaw,RTOD*roll_ang);
		}
		else { /* print position and orientation of vehicle */
			if (!(count%print_int))
				printf("%f %f %f %f %f 0.0 0.0\n",time,front[0],front[1],front[2], RTOD * angle);
		}
		count++;
	}

}

#define OPT_STR "r:l:a:f:p:"

int get_args(argc,argv)
int argc;
char **argv;
{
	int c;
	while ( (c=getopt(argc,argv,OPT_STR)) != EOF) {
		switch(c){
		case 'l':
			sscanf(optarg,"%lf",&length);
			break;
		case 'a':
			sscanf(optarg,"%lf",&angle);
			angle *= DTOR; /* degrees to radians */
			angle_set = 1;
			break;
		case 'r':
			sscanf(optarg,"%lf",&radius);
			turn_wheels = 1;
			break;
		case 'f':
			turn_wheels = 1;
			sscanf(optarg,"%lf",&factor);
			break;
		case 'p':
			sscanf(optarg,"%d",&print_int);
			break;
		default:
			fprintf(stderr,"Unknown option: -%c\n",c);
			return(0);
		}
	}
	return(1);
}

