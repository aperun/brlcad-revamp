/*			A N I M _ H A R D T R A C K . C
 *
 *  Animate the links and wheels of a tracked vehicle. It is assumed
 *  that the wheels do not translate with respect to the vehicle.
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

#ifndef M_PI_2
#define M_PI_2	(M_PI*0.5)
#endif

#define NW	num_wheels
#define NEXT(i)	(i+1)%NW
#define PREV(i)	(i+NW-1)%NW
typedef double *pdouble;

struct wheel {
vect_t		pos;	/* displacement of wheel from vehicle origin */
fastf_t		rad;	/* radius of wheel */
fastf_t		ang0;	/* angle where track meets wheel; 0 < a < 2pi */
fastf_t		ang1;	/* angle where track leaves wheel; 0 < a < 2pi */
fastf_t		arc;	/* arclength of contact between wheel and track */
};

struct track {
vect_t		pos0;	/* beginning point of track section */
vect_t		pos1;	/* end point of track section */
vect_t		dir;	/* unit vector: direction of track section */
fastf_t		len;	/* length of track section */
};

struct slope {
vect_t		dir;	/* vector from previous to current axle */
fastf_t		len;	/* length of vector described above */
};

struct all {
struct wheel 	w;	/* parameters describing the track around a wheel */
struct track	t;	/* track between this wheel and the previous wheel */
struct slope	s;	/* vector between this axle and the previous axle */
};

struct rlink {
vect_t		pos;	/* reverse of initial position */
fastf_t		ang;	/* initial angle */
};

/* external variables */
extern int optind;
extern char *optarg;

/* variables describing track geometry - used by main, trackprep, get_link */
struct all *x;
struct rlink *r;		/* reverse of initial locations of links */
int num_links, num_wheels;
fastf_t track_y, tracklen;

/* variables set by get_args */
char wheel_name[40];	/* base name of wheels */
int print_wheel;	/* flag: do wheel animation */
int links_placed;	/* flag: links are initially on the track */
int axes, cent;		/* flags: alternate axes, centroid specified */
int u_control;		/* flag: vehicle orientation specified by user */
int first_frame;	/* integer with which to begin numbering frames */
fastf_t radius;		/* common radius of all wheels */
fastf_t init_dist; 	/* initial distance of first link along track */
vect_t centroid, rcentroid;	/* alternate centroid and its reverse */
mat_t m_axes, m_rev_axes;	/* matrices to and from alternate axes */

main(argc,argv)
int argc;
char **argv;
{
	void anim_dir2mat(), anim_y_p_r2mat(), anim_add_trans(),anim_mat_print();
	int get_args(), get_link(), track_prep(), val, frame, go, i, count;
	fastf_t y_rot, distance, yaw, pitch, roll;
	vect_t p1, p2, p3, dir, dir2, wheel_now, wheel_prev;
	vect_t zero, position, vdelta, temp, to_track, to_front;
	mat_t mat_v, wmat, mat_x;
	FILE *stream;

	VSETALL(zero,0.0);
	VSETALL(to_track,0.0);
	VSETALL(centroid,0.0);
	VSETALL(rcentroid,0.0);
	init_dist = y_rot = radius= 0.0;
	first_frame = num_wheels = u_control = axes = cent = links_placed=0;
	MAT_IDN(mat_v);
	MAT_IDN(mat_x);
	MAT_IDN(wmat);
	MAT_IDN(m_axes);
	MAT_IDN(m_rev_axes);

	if (!get_args(argc,argv))
		fprintf(stderr,"Argument error.");

	if (axes || cent ){ /* vehicle has own reference frame */
		anim_add_trans(m_axes,centroid,zero);
		anim_add_trans(m_rev_axes,zero,rcentroid);
	}

	/* get track information from specified file */

	if (!(stream = fopen(*(argv+optind+1),"r"))){
		fprintf(stderr,"Anim_hardtrack: Could not open file %s.\n",*(argv+optind+1));
		return(0);
	}
	fscanf(stream,"%d %d %lf", &num_wheels, &num_links, &track_y);
		/*allocate memory for track information*/
	x = (struct all *) calloc(num_wheels,sizeof(struct all));
		/*read rest of track info */
	for (i=0;i<NW;i++){
		fscanf(stream,"%lf %lf",x[i].w.pos,x[i].w.pos+2);
		if (radius)
			x[i].w.rad = radius;
		else
			fscanf(stream,"%lf",& x[i].w.rad);
		x[i].w.pos[1] = track_y;
	}
	(void) fclose(stream);

	(void) track_prep();

	/* initialize to_track */
	VSET(to_track, 0.0, track_y, 0.0);
	VSET(to_front,1.0,0.0,0.0);

	/* main loop */
	distance = init_dist;
	if(u_control)
		frame = first_frame;
	else
		frame = first_frame-1; 
	for (val = 3; val > 2; frame++) {
		go = 1;
		/*p2 is current position. p3 is next;p1 is previous*/
		VMOVE(p1,p2);
		VMOVE(p2,p3);
		scanf("%*f");/*time stamp*/
		val = scanf("%lf %lf %lf", p3, p3+1, p3 + 2);
		if(u_control){
			scanf("%lf %lf %lf",&yaw,&pitch,&roll);
			anim_dy_p_r2mat(mat_v,yaw,pitch,roll);
			anim_add_trans(mat_v,p3,rcentroid);			
		}
		else { /* analyze positions for steering */
			/*get useful direction unit vectors*/
			if (frame == first_frame){ /* first frame*/
				VSUBUNIT(dir,p3,p2);
				VMOVE(dir2,dir);
			}
			else if (val < 3){ /*last frame*/
				VSUBUNIT(dir,p2,p1);
				VMOVE(dir2,dir);
			}
			else if (frame > first_frame){ /*normal*/
				VSUBUNIT(dir,p3,p1);
				VSUBUNIT(dir2,p2,p1);/*needed for vertical case*/
			}
			else go = 0;/*first time through loop;no p2*/
			
			/*create matrix which would move vehicle*/
			anim_dir2mat(mat_v,dir,dir2); 
			anim_add_trans(mat_v,p2,rcentroid); 
		}

		/*determine distance traveled*/
		VMOVE(wheel_prev,wheel_now);
		MAT4X3PNT(wheel_now,mat_v,to_track);
		if (frame > first_frame){  /* increment distance by distance moved */
			VSUB2(vdelta,wheel_now,wheel_prev);
			MAT3X3VEC(temp,mat_v,to_front);/*new front of vehicle*/
			distance += VDOT(temp,vdelta);/*portion of vdelta in line with track*/
		}

		if (go){
			printf("start %d;\nclean;\n", frame);
		        for (count=0;count<num_links;count++){
	        	        (void) get_link(position,&y_rot,distance+tracklen*count/num_links);
				anim_y_p_r2mat(wmat,0.0,y_rot+r[count].ang,0.0);
		        	anim_add_trans(wmat,position,r[count].pos);
		        	if ((axes || cent) && links_placed){ /* link moved from vehicle coords */
			        	mat_mul(mat_x,wmat,m_rev_axes);
		        		mat_mul(wmat,m_axes,mat_x);
		        	}
		        	else if (axes || cent){ /* link moved to vehicle coords */
		        		MAT_MOVE(mat_x,wmat);
		        		mat_mul(wmat,m_axes,mat_x);
		        	}
				printf("anim %s.%d matrix lmul\n", *(argv+optind),count);
		        	anim_mat_print(wmat,1);
			}
			if (print_wheel){
				for (count = 0;count<num_wheels;count++){
					anim_y_p_r2mat(wmat,0.0,-distance/x[count].w.rad,0.0);
					VREVERSE(temp,x[count].w.pos);
					anim_add_trans(wmat,x[count].w.pos,temp);
			        	if (axes || cent){
				        	mat_mul(mat_x,wmat,m_rev_axes);
			        		mat_mul(wmat,m_axes,mat_x);
			        	}
					printf("anim %s.%d matrix lmul\n",wheel_name,count);
					anim_mat_print(wmat,1);
				}
			}
			printf("end;\n");
		}
	}
}


int track_prep()/*run once at the beginning to establish important track info*/
{
	int i;
	fastf_t phi, costheta, link_angle, arc_angle;
	vect_t difference, link_cent;

	/* first loop - get inter axle slopes and start/end angles */
	for (i=0;i<NW;i++){
		/*calculate current slope vector*/
		VSUB2(x[i].s.dir,x[i].w.pos,x[PREV(i)].w.pos);
		x[i].s.len = MAGNITUDE(x[i].s.dir);
		/*calculate end angle of previous wheel - atan2(y,x)*/
		phi = atan2(x[i].s.dir[2],x[i].s.dir[0]);/*absolute angle of slope*/
		costheta = (x[PREV(i)].w.rad - x[i].w.rad)/x[i].s.len;/*cosine of special angle*/
		x[PREV(i)].w.ang1 = phi +  acos(costheta);
		while (x[PREV(i)].w.ang1 < 0.0)
			x[PREV(i)].w.ang1 += 2.0*M_PI;
		x[i].w.ang0 = x[PREV(i)].w.ang1;
	}

	/* second loop - handle concavities */
        for (i=0;i<NW;i++){
                arc_angle = x[i].w.ang0 - x[i].w.ang1;
                while (arc_angle < 0.0)
                        arc_angle += 2.0*M_PI;
                if (arc_angle > M_PI) { /* concave */
                        x[i].w.ang0 = 0.5*(x[i].w.ang0 + x[i].w.ang1);
                        x[i].w.ang1 = x[i].w.ang0;
                        x[i].w.arc = 0.0;
                }
                else { /* convex - angles are already correct */
                        x[i].w.arc = arc_angle;
                }
        }

	/* third loop - calculate geometry of track segments */
	tracklen = 0.0;
	for (i=0;i<NW;i++){
		/*calculate endpoints of track segment*/
		x[i].t.pos1[0] = x[i].w.pos[0] + x[i].w.rad*cos(x[i].w.ang0);
		x[i].t.pos1[1] = x[i].w.pos[1];
		x[i].t.pos1[2] = x[i].w.pos[2] + x[i].w.rad*sin(x[i].w.ang0);
		x[i].t.pos0[0] = x[PREV(i)].w.pos[0] + x[PREV(i)].w.rad*cos(x[PREV(i)].w.ang1);
		x[i].t.pos0[1] = x[PREV(i)].w.pos[1];
		x[i].t.pos0[2] = x[PREV(i)].w.pos[2] + x[PREV(i)].w.rad*sin(x[PREV(i)].w.ang1);
		/*calculate length and direction*/
		VSUB2(difference,x[i].t.pos1,x[i].t.pos0);
		x[i].t.len = MAGNITUDE(difference);
		VSCALE((x[i].t.dir),difference,(1.0/x[i].t.len));

		/*calculate arclength and total track length*/
		tracklen += x[i].t.len;
		tracklen += x[i].w.arc*x[i].w.rad;
	}

	/* for a track with links already placed, get initial positions*/
	r = (struct rlink *) calloc(num_links, sizeof(struct rlink));
	if (links_placed)
		for (i=0;i<num_links;i++){
			get_link(link_cent,&link_angle,init_dist + tracklen*i/num_links);
			VREVERSE(r[i].pos,link_cent);
			r[i].ang = -link_angle;
		}
	return(0);
}

	
int get_link(pos,angle_p,dist) 
fastf_t *angle_p, dist;
vect_t pos;
{
	int i;
	vect_t temp;
	while (dist >= tracklen) /*periodicize*/
		dist -= tracklen;
	while (dist < 0.0)
		dist += tracklen;
	for (i=0;i<NW;i++){
		if ( (dist  -= x[i].t.len) < 0 ){
			VSCALE(temp,(x[i].t.dir),dist);
			VADD2(pos,x[i].t.pos1,temp);
			*angle_p = atan2(x[i].t.dir[2],x[i].t.dir[0]);
			return(2*i);
		}
		if ((dist -= x[i].w.rad*x[i].w.arc) < 0){
			*angle_p = dist/x[i].w.rad;
			*angle_p = x[i].w.ang1 - *angle_p;/*from x-axis to link*/
			pos[0] = x[i].w.pos[0] + x[i].w.rad*cos(*angle_p);
			pos[1] = x[i].w.pos[1];
			pos[2] = x[i].w.pos[2] + x[i].w.rad*sin(*angle_p);
			*angle_p -= M_PI_2; /*angle of clockwise tangent to circle*/
			return(2*i+1);
		}
	}
	return -1;
}

#define OPT_STR "b:d:f:i:lr:w:u"

int get_args(argc,argv)
int argc;
char **argv;
{
	fastf_t yaw, pch, rll;
	void anim_dx_y_z2mat(), anim_dz_y_x2mat();
	int c, i;
	axes = cent = links_placed = print_wheel = 0;
        while ( (c=getopt(argc,argv,OPT_STR)) != EOF) {
                i=0;
                switch(c){
                case 'b':
                	optind -= 1;
                        sscanf(argv[optind+(i++)],"%lf", &yaw );
                        sscanf(argv[optind+(i++)],"%lf", &pch );
                        sscanf(argv[optind+(i++)],"%lf", &rll );
			optind += 3;
			anim_dx_y_z2mat(m_axes, rll, -pch, yaw);
			anim_dz_y_x2mat(m_rev_axes, -rll, pch, -yaw);
			axes = 1;
                        break;
                case 'd':
                        optind -= 1;
                        sscanf(argv[optind+(i++)],"%lf",centroid);
                        sscanf(argv[optind+(i++)],"%lf",centroid+1);
                        sscanf(argv[optind+(i++)],"%lf",centroid+2);
                        optind += 3;
                        VREVERSE(rcentroid,centroid);
                	cent = 1;
                        break;
                case 'f':
                	sscanf(optarg,"%d",&first_frame);
                        break;
                case 'i':
                	sscanf(optarg,"%lf",&init_dist);
                	break;
                case 'l':
                	links_placed = 1;
                	break;
                case 'r':
                	sscanf(optarg,"%lf",&radius);
                	break;
                case 'w':
                	sscanf(optarg,"%s",wheel_name);
                	print_wheel = 1;
                        break;
                case 'u':
                	u_control = 1;
                	break;
                default:
                        fprintf(stderr,"Unknown option: -%c\n",c);
                        return(0);
                }
        }
        return(1);
}

void show_info(which)/* for debugging - -1:track 0:both 1:link*/
int which;
{
	int i;
	if (which <=0){
		fprintf(stderr,"track length: %f\n",tracklen);
		fprintf(stderr,"link length: %f\n",tracklen/num_links);
		for (i=0;i<NW;i++){
			fprintf(stderr,"wheel %d: %f %f %f %f %f %f\n",i,x[i].w.pos[0],x[i].w.pos[1],x[i].w.pos[2],x[i].w.rad,x[i].w.ang1,x[i].w.arc);
			fprintf(stderr,"track %d: %f %f %f %f %f %f %f\n",i,x[i].t.pos1[0],x[i].t.pos1[1],x[i].t.pos1[2],x[i].t.dir[0],x[i].t.dir[1],x[i].t.dir[2],x[i].t.len);
			fprintf(stderr,"slope %d: %f %f %f %f\n",i,x[i].s.dir[0],x[i].s.dir[1],x[i].s.dir[2],x[i].s.len);
		}
	}
/*	if (which >= 0){
		fprintf(stderr,"%d %f %f %f %f\n",count,position[0],position[1],position[2],y_rot);
	}*/
}

