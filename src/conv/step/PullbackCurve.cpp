
#include "common.h"

#include "dvec.h"

#include <assert.h>
#include <vector>
#include <list>
#include <limits>

#include "tnt.h"
#include "jama_lu.h"
#include "opennurbs_ext.h"

#include "PullbackCurve.h"

using namespace std;
using namespace brlcad;

#define RANGE_HI 0.55
#define RANGE_LO 0.45
#define UNIVERSAL_SAMPLE_COUNT 1001

//numeric_limits<double> real;

typedef struct _bspline {
  int p; // degree
  int m; // num_knots-1
  int n; // num_samples-1 (aka number of control points)
  vector<double> params;
  vector<double> knots;
  ON_2dPointArray controls;
} BSpline;


class plane_ray {
public:
    vect_t n1;
    fastf_t d1;

    vect_t n2;
    fastf_t d2;
};

bool
isFlat(const ON_2dPoint& p1, const ON_2dPoint& m, const ON_2dPoint& p2, double flatness) {
  ON_Line line = ON_Line(ON_3dPoint(p1), ON_3dPoint(p2));
  return line.DistanceTo(ON_3dPoint(m)) <= flatness;
}

void
brep_get_plane_ray(ON_Ray& r, plane_ray& pr)
{
    vect_t v1;
    VMOVE(v1, r.m_dir);
    fastf_t min = MAX_FASTF;
    int index = -1;
    for (int i = 0; i < 3; i++) {
	// find the smallest component
	if (fabs(v1[i]) < min) {
	    min = fabs(v1[i]);
	    index = i;
	}
    }
    v1[index] += 1; // alter the smallest component
    VCROSS(pr.n1, v1, r.m_dir); // n1 is perpendicular to v1
    VUNITIZE(pr.n1);
    VCROSS(pr.n2, pr.n1, r.m_dir);       // n2 is perpendicular to v1 and n1
    VUNITIZE(pr.n2);
    pr.d1 = VDOT(pr.n1, r.m_origin);
    pr.d2 = VDOT(pr.n2, r.m_origin);
    TRACE1("n1:" << ON_PRINT3(pr.n1) << " n2:" << ON_PRINT3(pr.n2) << " d1:" << pr.d1 << " d2:" << pr.d2);
}

void
brep_r(const ON_Surface* surf, plane_ray& pr, pt2d_t uv, ON_3dPoint& pt, ON_3dVector& su, ON_3dVector& sv, pt2d_t R)
{
    assert(surf->Ev1Der(uv[0], uv[1], pt, su, sv));
    R[0] = VDOT(pr.n1,((fastf_t*)pt)) - pr.d1;
    R[1] = VDOT(pr.n2,((fastf_t*)pt)) - pr.d2;
}


void
brep_newton_iterate(const ON_Surface* surf, plane_ray& pr, pt2d_t R, ON_3dVector& su, ON_3dVector& sv, pt2d_t uv, pt2d_t out_uv)
{
    mat2d_t jacob = { VDOT(pr.n1,((fastf_t*)su)), VDOT(pr.n1,((fastf_t*)sv)),
		      VDOT(pr.n2,((fastf_t*)su)), VDOT(pr.n2,((fastf_t*)sv)) };
    mat2d_t inv_jacob;
    if (mat2d_inverse(inv_jacob, jacob)) {
	// check inverse validity
	pt2d_t tmp;
	mat2d_pt2d_mul(tmp, inv_jacob, R);
	pt2dsub(out_uv, uv, tmp);
    } else {
	TRACE2("inverse failed"); // XXX how to handle this?
	move(out_uv, uv);
    }
}

void
utah_ray_planes(const ON_Ray &r, ON_3dVector &p1, double &p1d, ON_3dVector &p2, double &p2d)
{
    ON_3dPoint ro(r.m_origin);
    ON_3dVector rdir(r.m_dir);
    double rdx, rdy, rdz;
    double rdxmag, rdymag, rdzmag;

    rdx = rdir.x;
    rdy = rdir.y;
    rdz = rdir.z;

    rdxmag = fabs(rdx);
    rdymag = fabs(rdy);
    rdzmag = fabs(rdz);

    if (rdxmag > rdymag && rdxmag > rdzmag)
	p1 = ON_3dVector(rdy, -rdx, 0);
    else
	p1 = ON_3dVector(0, rdz, -rdy);
	p1.Unitize();

    p2 = ON_CrossProduct(p1, rdir);

    p1d = -(p1 * ro);
    p2d = -(p2 * ro);
}
enum seam_direction
seam_direction(ON_2dPoint uv1,ON_2dPoint uv2) {
	if (NEAR_EQUAL(uv1.x,0.0,PBC_TOL) && NEAR_EQUAL(uv2.x,0.0,PBC_TOL)) {
    	//cout << "West" << endl;
    	return WEST_SEAM;
    } else if (NEAR_EQUAL(uv1.x,1.0,PBC_TOL) && NEAR_EQUAL(uv2.x,1.0,PBC_TOL)) {
    	//cout << "East" << endl;
       	return EAST_SEAM;
    } else if (NEAR_EQUAL(uv1.y,0.0,PBC_TOL) && NEAR_EQUAL(uv2.y,0.0,PBC_TOL)) {
    	//cout << "South" << endl;
       	return SOUTH_SEAM;
    } else if (NEAR_EQUAL(uv1.y,1.0,PBC_TOL) && NEAR_EQUAL(uv2.y,1.0,PBC_TOL)) {
    	//cout << "North" << endl;
       	return NORTH_SEAM;
    } else {
    	//cout << "What the???" << endl;
       	return UNKNOWN_SEAM_DIRECTION;
    }
}
#define BREP_INTERSECTION_ROOT_EPSILON 1e-6
bool
toUV(PBCData& data, ON_2dPoint& out_pt, double t, double knudge=0.0) {
  ON_3dPoint pointOnCurve = data.curve->PointAt(t);
  ON_3dPoint knudgedPointOnCurve = data.curve->PointAt(t+knudge);

  ON_2dPoint uv;
  if ( data.surftree->getSurfacePoint((const ON_3dPoint&)pointOnCurve,uv,(const ON_3dPoint&)knudgedPointOnCurve) > 0 ) {
	  out_pt.Set(uv.x,uv.y);
	  return true;
  } else {
	  if ( data.surftree->getSurfacePoint((const ON_3dPoint&)pointOnCurve,uv,(const ON_3dPoint&)knudgedPointOnCurve) > 0 ) {
	  	  out_pt.Set(uv.x,uv.y);
	  	  return true;
	  }
	  return false;
  }

  ON_3dVector dt;
  double a,b;
  data.curve->Ev1Der(t,pointOnCurve,dt);
  ON_3dVector tangent = data.curve->TangentAt(t);
  //data.surf->GetClosestPoint(pointOnCurve,&a,&b,0.0001);
  ON_Ray r(pointOnCurve,tangent);
  plane_ray pr;
  brep_get_plane_ray(r,pr);
  ON_3dVector p1;
  double p1d;
  ON_3dVector p2;
  double p2d;

  utah_ray_planes(r,p1, p1d, p2, p2d);

  VMOVE(pr.n1,p1);
  pr.d1 = p1d;
  VMOVE(pr.n2,p2);
  pr.d2 = p2d;

  try {
  const ON_Surface *surf = data.surftree->getSurface();
  ON_2dPoint uv = data.surftree->getClosestPointEstimate(knudgedPointOnCurve);
  ON_3dVector dir = surf->NormalAt(uv.x,uv.y);
  dir.Reverse();
  ON_Ray ray(pointOnCurve,dir);
  brep_get_plane_ray(ray,pr);
  //know use this as guess to iterate to closer solution
  pt2d_t Rcurr;
  pt2d_t new_uv;
  ON_3dPoint pt;
  ON_3dVector su,sv;
  bool found=false;
  fastf_t Dlast = MAX_FASTF;
  for (int i = 0; i < 10; i++) {
	brep_r(surf, pr, uv, pt, su, sv, Rcurr);
	fastf_t d = v2mag(Rcurr);
	if (d < BREP_INTERSECTION_ROOT_EPSILON) {
	    TRACE1("R:"<<ON_PRINT2(Rcurr));
	    found = true; break;
	} else if (d > Dlast) {
	    found = false; //break;
		break;
	    //return brep_edge_check(found, sbv, face, surf, ray, hits);
	}
	brep_newton_iterate(surf, pr, Rcurr, su, sv, uv, new_uv);
	move(uv, new_uv);
	Dlast = d;
  }

///////////////////////////////////////
  out_pt.Set(uv.x,uv.y);
  return true;
  } catch(...) {
	  return false;
  }
}
bool
toUV(SurfaceTree *surftree, const ON_Curve *curve,  ON_2dPoint& out_pt, double t, double knudge=0.0) {
  const ON_Surface *surf = surftree->getSurface();
  ON_3dPoint pointOnCurve = curve->PointAt(t);
  ON_3dPoint knudgedPointOnCurve = curve->PointAt(t+knudge);
  ON_3dVector dt;
  double a,b;
  curve->Ev1Der(t,pointOnCurve,dt);
  ON_3dVector tangent = curve->TangentAt(t);
  //data.surf->GetClosestPoint(pointOnCurve,&a,&b,0.0001);
  ON_Ray r(pointOnCurve,tangent);
  plane_ray pr;
  brep_get_plane_ray(r,pr);
  ON_3dVector p1;
  double p1d;
  ON_3dVector p2;
  double p2d;

  utah_ray_planes(r,p1, p1d, p2, p2d);

  VMOVE(pr.n1,p1);
  pr.d1 = p1d;
  VMOVE(pr.n2,p2);
  pr.d2 = p2d;

  try {
  ON_2dPoint uv = surftree->getClosestPointEstimate(knudgedPointOnCurve);
  ON_3dVector dir = surf->NormalAt(uv.x,uv.y);
  dir.Reverse();
  ON_Ray ray(pointOnCurve,dir);
  brep_get_plane_ray(ray,pr);
  //know use this as guess to iterate to closer solution
  pt2d_t Rcurr;
  pt2d_t new_uv;
  ON_3dPoint pt;
  ON_3dVector su,sv;
  bool found=false;
  fastf_t Dlast = MAX_FASTF;
  for (int i = 0; i < 10; i++) {
	brep_r(surf, pr, uv, pt, su, sv, Rcurr);
	fastf_t d = v2mag(Rcurr);
	if (d < BREP_INTERSECTION_ROOT_EPSILON) {
	    TRACE1("R:"<<ON_PRINT2(Rcurr));
	    found = true; break;
	} else if (d > Dlast) {
	    found = false; //break;
		break;
	    //return brep_edge_check(found, sbv, face, surf, ray, hits);
	}
	brep_newton_iterate(surf, pr, Rcurr, su, sv, uv, new_uv);
	move(uv, new_uv);
	Dlast = d;
  }

///////////////////////////////////////
  out_pt.Set(uv.x,uv.y);
  return true;
  } catch(...) {
	  return false;
  }
}

double
randomPointFromRange(PBCData& data, ON_2dPoint& out, double lo, double hi)
{
  assert(lo < hi);
  double random_pos = drand48() * (RANGE_HI - RANGE_LO) + RANGE_LO;
  double newt = random_pos * (hi - lo) + lo;
  assert(toUV(data, out, newt));
  return newt;
}

bool
sample(PBCData& data,
       double t1,
       double t2,
       const ON_2dPoint& p1,
       const ON_2dPoint& p2)
{
  ON_2dPoint m;
  double t = randomPointFromRange(data, m, t1, t2);
  ON_2dPointArray * samples = (*data.segments.end());
  if (isFlat(p1, m, p2, data.flatness)) {
    samples->Append(p2);
  } else {
    sample(data, t1, t, p1, m);
    sample(data, t, t2, m, p2);
  }
  return true;
}

void
generateKnots(BSpline& bspline) {
  int num_knots = bspline.m + 1;
  bspline.knots.resize(num_knots);
  //cout << "knot size:" << bspline.knots.size() << endl;
  for (int i = 0; i <= bspline.p; i++) {
    bspline.knots[i] = 0.0;
  }
  for (int i = bspline.m-bspline.p; i <= bspline.m; i++) {
    bspline.knots[i] = 1.0;
  }
  for (int i = 1; i <= bspline.n-bspline.p; i++) {
    bspline.knots[bspline.p+i] = (double)i / (bspline.n-bspline.p+1.0);
  }
  //cout << "knot size:" << bspline.knots.size() << endl;
}

int
getKnotInterval(BSpline& bspline, double u) {
  int k = 0;
  while (u >= bspline.knots[k]) k++;
  k = (k == 0) ? k : k-1;
  return k;
}

int
getCoefficients(BSpline& bspline, Array1D<double>& N, double u) {
  // evaluate the b-spline basis function for the given parameter u
  // place the results in N[]
  N = 0.0;
  if (u == bspline.knots[0]) {
    N[0] = 1.0;
    return 0;
  } else if (u == bspline.knots[bspline.m]) {
    N[bspline.n] = 1.0;
    return bspline.n;
  }
  int k = getKnotInterval(bspline, u);
  N[k] = 1.0;
  for (int d = 1; d <= bspline.p; d++) {
    double uk_1 = bspline.knots[k+1];
    double uk_d_1 = bspline.knots[k-d+1];
    N[k-d] = ((uk_1 - u)/(uk_1 - uk_d_1)) * N[k-d+1];
    for (int i = k-d+1; i <= k-1; i++) {
      double ui = bspline.knots[i];
      double ui_1 = bspline.knots[i+1];
      double ui_d = bspline.knots[i+d];
      double ui_d_1 = bspline.knots[i+d+1];
      N[i] = ((u - ui)/(ui_d - ui)) * N[i] + ((ui_d_1 - u)/(ui_d_1 - ui_1))*N[i+1];
    }
    double uk = bspline.knots[k];
    double uk_d = bspline.knots[k+d];
    N[k] = ((u - uk)/(uk_d - uk)) * N[k];
  }
  return k;
}

void
printMatrix(Array1D<double>& m) {
	printf("---\n");
	for (int i = 0; i < m.dim1(); i++) {
		printf("% 5.5f ", m[i]);
	}
	printf("\n");
}

// XXX: this function sucks...
void generateParameters(BSpline& bspline) {
	double lastT = 0.0;
	bspline.params.resize(bspline.n + 1);
	Array2D<double> N(UNIVERSAL_SAMPLE_COUNT, bspline.n + 1);
	for (int i = 0; i < UNIVERSAL_SAMPLE_COUNT; i++) {
		double t = (double) i / (UNIVERSAL_SAMPLE_COUNT - 1);
		Array1D<double> n = Array1D<double> (N.dim2(), N[i]);
		getCoefficients(bspline, n, t);
		//printMatrix(n);
	}

	for (int i = 0; i < bspline.n + 1; i++) {
		double max = 0.0; //real.min();
		for (int j = 0; j < UNIVERSAL_SAMPLE_COUNT; j++) {
			double f = N[j][i];
			double t = (double) j / (UNIVERSAL_SAMPLE_COUNT - 1);
			if (f > max) {
				max = f;
				if (j == UNIVERSAL_SAMPLE_COUNT - 1)
					bspline.params[i] = t;
			} else if (f < max) {
				bspline.params[i] = lastT;
				break;
			}
			lastT = t;
		}
	}
}

void
printMatrix(Array2D<double>& m) {
  printf("---\n");
  for (int i = 0; i < m.dim1(); i++) {
    for (int j = 0; j < m.dim2(); j++) {
      printf("% 5.5f ", m[i][j]);
    }
    printf("\n");
  }
}

ON_NurbsCurve*
interpolateLocalCubicCurve(ON_2dPointArray &Q) {
	int num_samples = Q.Count();
	int num_segments = Q.Count() - 1;
	int qsize = num_samples + 3;
	ON_2dVector qarray[qsize];
	ON_2dVector *q = &qarray[1];

	for(int i=1; i < Q.Count(); i++) {
		q[i] = Q[i] - Q[i-1];
	}
	q[0] = 2.0*q[1] - q[2];
	q[-1] = 2.0*q[0] - q[1];

	q[num_samples+1] = 2*q[num_samples] - q[num_samples-1];
	q[num_samples+2] = 2*q[num_samples+1] - q[num_samples];

	ON_2dVector T[num_samples];
	double a[num_samples];
	for (int k=0; k < num_samples; k++ ) {
		ON_3dVector a = ON_CrossProduct(q[k-1],q[k]);
		ON_3dVector b = ON_CrossProduct(q[k+1],q[k+2]);
		double alength = a.Length();
		if (NEAR_ZERO(alength,PBC_TOL)) {
			a[k] = 1.0;
		} else {
			a[k] = (a.Length())/(a.Length() + b.Length());
		}
		T[k] = (1.0 - a[k])*q[k] + a[k]*q[k+1];
		T[k].Unitize();
	}
	ON_2dPointArray P[num_samples-1];
	ON_2dPointArray control_points;
	control_points.Append(Q[0]);
	for(int i=1; i<num_samples; i++) {
		ON_2dPoint P0 = Q[i-1];
		ON_2dPoint P3 = Q[i];
		ON_2dVector T0 = T[i-1];
		ON_2dVector T3 = T[i];

		double a,b,c;

		ON_2dVector vT0T3 = T0 + T3;
		ON_2dVector dP0P3 = P3 - P0;
		a = 16.0 - vT0T3.Length()*vT0T3.Length();
		b = 12.0*(dP0P3*vT0T3);
		c = -36.0*dP0P3.Length()*dP0P3.Length();

		double alpha = (-b + sqrt(b*b - 4.0*a*c))/(2.0*a);

		ON_2dPoint P1 = P0 + (1.0/3.0)*alpha*T0;
		control_points.Append(P1);
		ON_2dPoint P2 = P3 - (1.0/3.0)*alpha*T3;
		control_points.Append(P2);
		P[i-1].Append(P0);
		P[i-1].Append(P1);
		P[i-1].Append(P2);
		P[i-1].Append(P3);
	}
	control_points.Append(Q[num_samples-1]);

	//generateParameters(spline);
	double u[num_segments+1];
	double U[2*(num_segments-1) + 8];
	u[0] = 0.0;
	for(int k=0;k<num_segments;k++){
		u[k+1] = u[k] + 3.0*(P[k][1]-P[k][0]).Length();
	}
	int degree = 3;
    int n = control_points.Count();
    int p = degree;
    int m = n + p - 1;
    int dimension = 2;
	ON_NurbsCurve* c = ON_NurbsCurve::New(dimension,
						false,
						degree+1,
						n);
	c->ReserveKnotCapacity(m);
	for (int i = 0; i < degree; i++) {
		c->SetKnot(i,0.0);
	}
	for (int i = 1; i < num_segments; i++) {
		double knot_value = u[i]/u[num_segments];
		c->SetKnot(degree + 2*(i -1),knot_value);
		c->SetKnot(degree + 2*(i -1) + 1,knot_value);
	}
	int k = c->KnotCount();
	for (int i = m - p; i < m; i++) {
		c->SetKnot(i,1.0);
	}

	// insert the control points
	for (int i = 0; i < n; i++) {
		ON_3dPoint p = control_points[i];
		c->SetCV(i,p);
	}
	return c;
}

void
generateControlPoints(BSpline& bspline, ON_2dPointArray &samples)
{
  Array2D<double> bigN(bspline.n+1, bspline.n+1);
  //printMatrix(bigN);

  for (int i = 0; i < bspline.n+1; i++) {
    Array1D<double> n = Array1D<double>(bigN.dim2(), bigN[i]);
    getCoefficients(bspline, n, bspline.params[i]);
    //printMatrix(bigN);
  }
  Array2D<double> bigD(bspline.n+1,2);
  for (int i = 0; i < bspline.n+1; i++) {
    bigD[i][0] = samples[i].x;
    bigD[i][1] = samples[i].y;
  }

  //printMatrix(bigD);
  //printMatrix(bigN);

  JAMA::LU<double> lu(bigN);
  assert(lu.isNonsingular() > 0);
  Array2D<double> bigP = lu.solve(bigD); // big linear algebra black box here...

  // extract the control points
  for (int i = 0; i < bspline.n+1; i++) {
    ON_2dPoint& p = bspline.controls.AppendNew();
    p.x = bigP[i][0];
    p.y = bigP[i][1];
  }
}

ON_NurbsCurve*
newNURBSCurve(BSpline& spline, int dimension=3) {
  // we now have everything to complete our spline
  ON_NurbsCurve* c = ON_NurbsCurve::New(dimension,
					false,
					spline.p+1,
					spline.n+1);
  c->ReserveKnotCapacity(spline.knots.size()-2);
  for (int i = 1; i < spline.knots.size()-1; i++) {
    c->m_knot[i-1] = spline.knots[i];
  }

  for (int i = 0; i < spline.controls.Count(); i++) {
    c->SetCV(i, ON_3dPoint(spline.controls[i]));
  }

  return c;
}

ON_Curve*
interpolateCurve(ON_2dPointArray &samples) {
	bool useBezier = false;
  if (samples.Count() == 2) {
		// build a line
		return new ON_LineCurve(samples[0], samples[1]);
	} else if (useBezier == true) {
		ON_BezierCurve *bezier = new ON_BezierCurve(
				samples);
		ON_NurbsCurve nurbcurve;
		if (bezier->GetNurbForm(nurbcurve)) {
			return ON_NurbsCurve::New(*bezier);
		}
		// build a NURBS curve, then see if it can be simplified!
		BSpline spline;
		spline.p = 3;
		spline.n = samples.Count() - 1;
		spline.m = spline.n + spline.p + 1;
		generateKnots(spline);
		generateParameters(spline);
		generateControlPoints(spline, samples);
		ON_NurbsCurve* nurbs = newNURBSCurve(spline);

		// XXX - attempt to simplify here!

		return nurbs;
	} else {
		ON_NurbsCurve* nurbs;
		if (samples.Count() < 1000) {
			BSpline spline;
			spline.p = 3;
			spline.n = samples.Count()-1;
			spline.m = spline.n + spline.p + 1;
			generateKnots(spline);
			generateParameters(spline);
			generateControlPoints(spline, samples);
			nurbs = newNURBSCurve(spline,2);
		} else {
			// local vs. global interpolation for large point sampled curves
			nurbs = interpolateLocalCubicCurve(samples);
		}
		// XXX - attempt to simplify here!

		return nurbs;
    }
}

/*
 * rc = 0 Not on seam, 1 on East/West seam(umin/umax), 2 on North/South seam(vmin/vmax), 3 seam on both U/V boundaries
 */
int
IsAtSeam(const ON_Surface *surf, double u, double v) {
	int rc = 0;
	int i;
	for (i=0; i<2; i++){
		if (!surf->IsClosed(i))
		  continue;
		double p = (i) ? v : u;
		if (NEAR_EQUAL(p,surf->Domain(i)[0],PBC_TOL) || NEAR_EQUAL(p,surf->Domain(i)[1],PBC_TOL))
		  rc += (i+1);
	}

	return rc;
}

int
IsAtSingularity(const ON_Surface *surf, double u, double v) {
	// 0 = south, 1 = east, 2 = north, 3 = west
	//cerr << "IsAtSingularity = u,v - " << u << "," << v << endl;
	//cerr << "surf->Domain(0) - " << surf->Domain(0)[0] << "," << surf->Domain(0)[1] << endl;
	//cerr << "surf->Domain(1) - " << surf->Domain(1)[0] << "," << surf->Domain(1)[1] << endl;
    if (NEAR_EQUAL(u,surf->Domain(0)[0],PBC_TOL)) {
		if (surf->IsSingular(3))
			return 3;
	} else if (NEAR_EQUAL(u,surf->Domain(0)[1],PBC_TOL)) {
		if (surf->IsSingular(1))
			return 1;
	}
	if (NEAR_EQUAL(v,surf->Domain(1)[0],PBC_TOL)) {
		if (surf->IsSingular(0))
			return 0;
	} else if (NEAR_EQUAL(v,surf->Domain(1)[1],PBC_TOL)) {
		if (surf->IsSingular(2))
			return 2;
	}
	return -1;
}

ON_Curve*
test1_pullback_curve(const SurfaceTree* surfacetree,
		const ON_Curve* curve,
		double tolerance,
		double flatness) {
	ON_NurbsCurve* orig = curve->NurbsCurve();
	bool isRational = false;
	  // we now have everything to complete our spline
	ON_NurbsCurve* c = ON_NurbsCurve::New(curve->Dimension(),
			isRational,
			orig->Degree()+1,
			orig->m_cv_count+1);

	int numKnots = orig->Degree() + orig->m_cv_count -1;
	  c->ReserveKnotCapacity(numKnots);
	  double *span = new double[numKnots];
	  if ( orig->GetSpanVector(span) ) {
		  for (int i = 0; i < numKnots; i++) {
			c->SetKnot(i,span[i]);
		  }
	  }

	  ON_2dPoint uv;
	  for (int i = 0; i < orig->m_cv_count; i++) {
		  ON_3dPoint p;
		  if ( orig->GetCV(i,p) ) {
			  if ( surfacetree->getSurfacePoint((const ON_3dPoint&)p,uv,(const ON_3dPoint&)p) > 0 ) {
				    c->SetCV(i, p);
			    }
		  }
	  }

	  return c;
}

ON_Curve*
test2_pullback_curve(const SurfaceTree* surfacetree,
		const ON_Curve* curve,
		double tolerance,
		double flatness) {
  PBCData data;
  data.tolerance = tolerance;
  data.flatness = flatness;
  data.curve = curve;
  data.surftree = (SurfaceTree*)surfacetree;
  ON_2dPointArray *samples= new ON_2dPointArray();

  data.segments.push_back(samples);

  // Step 1 - adaptively sample the curve
  double tmin, tmax;
  data.curve->GetDomain(&tmin, &tmax);
  int numKnots = curve->SpanCount();
  double *knots = new double[numKnots+1];
  curve->GetSpanVector(knots);

  int samplesperknotinterval;
  int degree = curve->Degree();

  if (degree > 1) {
	  samplesperknotinterval = 3*degree;
  } else {
	  samplesperknotinterval = 18*degree;
  }
  ON_2dPoint pt;
  for (int i=0; i<=numKnots; i++) {
	  if (i <= numKnots/2) {
		  if (i>0) {
			  double delta = (knots[i] - knots[i-1])/(double)samplesperknotinterval;
			  for (int j=1; j<samplesperknotinterval; j++){
				  if ( toUV(data, pt, knots[i-1]+j*delta, PBC_TOL) ) {
					  samples->Append(pt);
				  } else {
					  //cout << "didn't find point on surface" << endl;
				  }
			  }
		  }
		  if (toUV(data, pt, knots[i], PBC_TOL)) {
			  samples->Append(pt);
		  } else {
			  //cout << "didn't find point on surface" << endl;
		  }
	  } else {
		  if (i>0) {
			  double delta = (knots[i] - knots[i-1])/(double)samplesperknotinterval;
			  for (int j=1; j<samplesperknotinterval; j++){
				  if ( toUV(data, pt, knots[i-1]+j*delta, -PBC_TOL)) {
					  samples->Append(pt);
				  } else {
					  //cout << "didn't find point on surface" << endl;
				  }
			  }
			  if ( toUV(data, pt, knots[i],-PBC_TOL) ) {
				  samples->Append(pt);
			  } else {
				  //cout << "didn't find point on surface" << endl;
			  }
		  }
	  }
  }
  delete [] knots;

  cerr << endl << "samples:" << endl;
  for (int i = 0; i < samples->Count(); i++) {
		  cerr << i << "- " << (*samples)[i].x << "," << (*samples)[i].y << endl;
  }

  /*
  ON_Surface *surf = (ON_Surface *)data.surftree->getSurface();
  ON_3dPoint p;
  for (int i = 0; i < data.samples.Count(); i++) {
	  p=surf->PointAt(data.samples[i].x,data.samples[i].y);
	  cerr << data.samples[i].x << "," << data.samples[i].y;
	  cerr << " --> "<< p.x << "," << p.y << "," << p.z << endl;
  }
*/
  return interpolateCurve(*samples);
}
ON_2dPointArray *
pullback_samples(PBCData* data,
		double t,
		double s) {
	const ON_Curve* curve= data->curve;
	ON_2dPointArray *samples= new ON_2dPointArray();
	int numKnots = curve->SpanCount();
	double *knots = new double[numKnots+1];
	curve->GetSpanVector(knots);

	int istart = 0;
	while (t >= knots[istart])
		istart++;

	if (istart > 0) {
		istart--;
		knots[istart] = t;
	}

	int istop = numKnots;
	while (s <= knots[istop])
		istop--;

	if (istop < numKnots) {
		istop++;
		knots[istop] = s;
	}

	//TODO: remove debugging code
	//cerr << "t - " << t << " istart - " << istart << "knots[istart] - " << knots[istart] << endl;
	//cerr << "s - " << s << " istop - " << istop << "knots[istop] - " << knots[istop] << endl;

	int samplesperknotinterval;
	int degree = curve->Degree();

	if (degree > 1) {
		samplesperknotinterval = 3*degree;
	} else {
		samplesperknotinterval = 18*degree;
	}
	ON_2dPoint pt;
	for (int i=istart; i<=istop; i++) {
		if (i <= numKnots/2) {
			if (i>0) {
				double delta = (knots[i] - knots[i-1])/(double)samplesperknotinterval;
				for (int j=1; j<samplesperknotinterval; j++){
					if ( toUV(*data, pt, knots[i-1]+j*delta, PBC_FROM_OFFSET) ) {
						samples->Append(pt);
					} else {
						//cout << "didn't find point on surface" << endl;
					}
				}
			}
			if (toUV(*data, pt, knots[i], PBC_FROM_OFFSET)) {
				samples->Append(pt);
			} else {
				//cout << "didn't find point on surface" << endl;
			}
		} else {
			if (i>0) {
				double delta = (knots[i] - knots[i-1])/(double)samplesperknotinterval;
				for (int j=1; j<samplesperknotinterval; j++){
					if ( toUV(*data, pt, knots[i-1]+j*delta, -PBC_FROM_OFFSET)) {
						samples->Append(pt);
					} else {
						//cout << "didn't find point on surface" << endl;
					}
				}
				if ( toUV(*data, pt, knots[i],-PBC_FROM_OFFSET) ) {
					samples->Append(pt);
				} else {
					//cout << "didn't find point on surface" << endl;
				}
			}
		}
	}
	delete [] knots;
	return samples;
}
PBCData *
test2_pullback_samples(const SurfaceTree* surfacetree,
		const ON_Curve* curve,
		double tolerance,
		double flatness) {
  PBCData *data = new PBCData;
  data->tolerance = tolerance;
  data->flatness = flatness;
  data->curve = curve;
  data->surftree = (SurfaceTree*)surfacetree;
  const ON_Surface *surf = data->surftree->getSurface();

  double tmin, tmax;
  data->curve->GetDomain(&tmin, &tmax);

  if(surf->IsClosed(0) || surf->IsClosed(1) ) {
	  if ((tmin < 0.0) && (tmax > 0.0)) {
		  ON_2dPoint uv;
		  if ( toUV(*data, uv, 0.0, PBC_TOL) ) {
			  if (IsAtSeam(surf,uv.x,uv.y) > 0) {
				  ON_2dPointArray *samples1 = pullback_samples(data,tmin,0.0);
				  ON_2dPointArray *samples2 = pullback_samples(data,0.0,tmax);
				  if (samples1 != NULL) {
					  data->segments.push_back(samples1);
				  }
				  if (samples2 != NULL) {
					  data->segments.push_back(samples2);
				  }
				  cerr << "need to divide curve across the seam" << endl;
			  } else {
				  ON_2dPointArray *samples = pullback_samples(data,tmin,tmax);
				  if (samples != NULL) {
					  data->segments.push_back(samples);
				  }
			  }
		  } else {
			  cerr << "pullback_samples:Error: cannot evaluate curve at parameter 0.0" << endl;
			  return NULL;
		  }
	  } else {
		  ON_2dPointArray *samples = pullback_samples(data,tmin,tmax);
		  if (samples != NULL) {
			  data->segments.push_back(samples);
		  }
	  }
  } else {
	  ON_2dPointArray *samples = pullback_samples(data,tmin,tmax);
	  if (samples != NULL) {
		  data->segments.push_back(samples);
	  }
  }
  return data;
}
bool
has_singularity(const ON_Surface *surf) {
	bool ret = false;
	// 0 = south, 1 = east, 2 = north, 3 = west
	for(int i=0;i<4;i++){
		if (surf->IsSingular(i)) {
/*
			switch (i) {
			case 0:
				cout << "Singular South" << endl;
				break;
			case 1:
				cout << "Singular East" << endl;
				break;
			case 2:
				cout << "Singular North" << endl;
				break;
			case 3:
				cout << "Singular West" << endl;
			}
*/
			ret= true;
		}
	}
	return ret;
}
bool is_closed(const ON_Surface *surf) {
	bool ret = false;
	// dir  0 = "s", 1 = "t"
	for(int i=0;i<2;i++){
		if (surf->IsClosed(i)) {
//			switch (i) {
//			case 0:
//				cout << "Closed in U" << endl;
//				break;
//			case 1:
//				cout << "Closed in V" << endl;
//			}
			ret = true;
		}
	}
	return ret;
}
bool
check_pullback_closed( list<PBCData*> &pbcs) {
	list<PBCData*>::iterator d = pbcs.begin();
	const ON_Surface *surf = (*d)->surftree->getSurface();
	//TODO:
	// 0 = U, 1 = V
	if (surf->IsClosed(0) && surf->IsClosed(1)) {
		//TODO: need to check how torus UV looks to determine checks
		cerr << "Is this some kind of torus????" << endl;
	} else if (surf->IsClosed(0)) {
		//check_pullback_closed_U(pbcs);
		cout << "check closed in U" << endl;
	} else if (surf->IsClosed(1)) {
		//check_pullback_closed_V(pbcs);
		cout << "check closed in V" << endl;
	}
	return true;
}

bool
check_pullback_singular_east( list<PBCData*> &pbcs) {
	list<PBCData *>::iterator cs = pbcs.begin();
	const ON_Surface *surf = (*cs)->surftree->getSurface();
	double umin,umax;
	ON_2dPoint *prev = NULL;
	ON_2dPoint *noprev = NULL;

	surf->GetDomain(0,&umin,&umax);
	cout << "Umax: " << umax << endl;
	while(cs!=pbcs.end()) {
		PBCData *data = (*cs);
		list<ON_2dPointArray *>::iterator si = data->segments.begin();
		int segcnt = 0;
		while (si != data->segments.end()) {
			ON_2dPointArray *samples = (*si);
			cerr << endl << "Segment:" << ++segcnt << endl;
			if (true) {
				int ilast = samples->Count() - 1;
				  cerr << endl << 0 << "- " << (*samples)[0].x << "," << (*samples)[0].y << endl;
				  cerr << ilast << "- " << (*samples)[ilast].x << "," << (*samples)[ilast].y << endl;
			} else {
			  for (int i = 0; i < samples->Count(); i++) {
				  if (NEAR_EQUAL((*samples)[i].x,umax,PBC_TOL)) {
					  if (prev != NULL) {
						  cerr << "prev - " << prev->x << "," << prev->y << endl;
					  } else {
						  noprev = &(*samples)[i];
					  }
					  cerr << i << "- " << (*samples)[i].x << "," << (*samples)[i].y << endl << endl;
				  }
				  prev = &(*samples)[i];
			  }
			}
			si ++;
		}
		  cs++;
	}
	//cerr << "noprev - " << noprev->x << "," << noprev->y << endl;
	//cerr << "last - " << prev->x << "," << prev->y << endl;
	return true;
}

bool
check_pullback_singular( list<PBCData*> &pbcs) {
	list<PBCData*>::iterator d = pbcs.begin();
	const ON_Surface *surf = (*d)->surftree->getSurface();
	int cnt = 0;

	for(int i=0;i<4;i++){
		if (surf->IsSingular(i)) {
			cnt++;
		}
	}
	if (cnt > 2) {
		//TODO: I don't think this makes sense but check out
		cerr << "Is this some kind of sickness????" << endl;
		return false;
	} else if (cnt == 2) {
		if (surf->IsSingular(0) && surf->IsSingular(2)) {
			cout << "check singular North-South" << endl;
		} else if (surf->IsSingular(1) && surf->IsSingular(2)) {
			cout << "check singular East-West" << endl;
		} else {
			//TODO: I don't think this makes sense but check out
			cerr << "Is this some kind of sickness????" << endl;
			return false;
		}
	} else {
		if (surf->IsSingular(0)) {
			cout << "check singular South" << endl;
		} else if (surf->IsSingular(1)) {
			cout << "check singular East" << endl;
			if (check_pullback_singular_east(pbcs)) {
				return true;
			}
		} else if (surf->IsSingular(2)) {
			cout << "check singular North" << endl;
		} else if (surf->IsSingular(3)) {
			cout << "check singular West" << endl;
		}
	}
	return true;
}

void
print_pullback_data(string str, list<PBCData*> &pbcs, bool justendpoints) {
	list<PBCData*>::iterator cs = pbcs.begin();
	int trimcnt=0;
	if (justendpoints) {
		// print out endpoints before
		cerr << "EndPoints " << str << ":"<< endl;
		while(cs!=pbcs.end()) {
			PBCData *data = (*cs);
			const ON_Surface *surf = data->surftree->getSurface();
			list<ON_2dPointArray *>::iterator si = data->segments.begin();
			int segcnt = 0;
			while (si != data->segments.end()) {
				ON_2dPointArray *samples = (*si);
				cerr << endl << "  Segment:" << ++segcnt << endl;
				int ilast = samples->Count() - 1;
				cerr << "    T:" << ++trimcnt << endl;
				int i=0;
				int singularity=IsAtSingularity(surf,(*samples)[i].x,(*samples)[i].y);
				int seam=IsAtSeam(surf,(*samples)[i].x,(*samples)[i].y);
				cerr << "--------";
				if ((seam>0) && (singularity>=0)) {
					cerr << " S/S  " << (*samples)[i].x << "," << (*samples)[i].y << endl;
				} else if (seam>0) {
					cerr << " Seam " << (*samples)[i].x << "," << (*samples)[i].y << endl;
				} else if (singularity>=0) {
					cerr << " Sing " << (*samples)[i].x << "," << (*samples)[i].y << endl;
				} else {
					cerr << "      " << (*samples)[i].x << "," << (*samples)[i].y << endl;
				}
				i=ilast;
				singularity=IsAtSingularity(surf,(*samples)[i].x,(*samples)[i].y);
				seam=IsAtSeam(surf,(*samples)[i].x,(*samples)[i].y);
				cerr << "        ";
				if ((seam>0) && (singularity>=0)) {
					cerr << " S/S  " << (*samples)[i].x << "," << (*samples)[i].y << endl;
				} else if (seam>0) {
					cerr << " Seam " << (*samples)[i].x << "," << (*samples)[i].y << endl;
				} else if (singularity>=0) {
					cerr << " Sing " << (*samples)[i].x << "," << (*samples)[i].y << endl;
				} else {
					cerr << "      " << (*samples)[i].x << "," << (*samples)[i].y << endl;
				}
				si++;
			}
			cs++;
		}
	} else {
		// print out all points
		trimcnt=0;
		cs = pbcs.begin();
		cerr << str << ":" << endl;
		while(cs!=pbcs.end()) {
			PBCData *data = (*cs);
			const ON_Surface *surf = data->surftree->getSurface();
			list<ON_2dPointArray *>::iterator si = data->segments.begin();
			int segcnt = 0;
			while (si != data->segments.end()) {
				ON_2dPointArray *samples = (*si);
				cerr << endl << "  Segment:" << ++segcnt << endl;
				int ilast = samples->Count() - 1;
				cerr << "    T:" << ++trimcnt << endl;
				for(int i = 0; i < samples->Count(); i++) {
					int singularity=IsAtSingularity(surf,(*samples)[i].x,(*samples)[i].y);
					int seam=IsAtSeam(surf,(*samples)[i].x,(*samples)[i].y);
					if (i == 0) {
						cerr << "--------";
					} else {
						cerr << "        ";
					}
					if ((seam>0) && (singularity>=0)) {
						cerr << " S/S  " << (*samples)[i].x << "," << (*samples)[i].y << endl;
					} else if (seam>0) {
						cerr << " Seam " << (*samples)[i].x << "," << (*samples)[i].y << endl;
					} else if (singularity>=0) {
						cerr << " Sing " << (*samples)[i].x << "," << (*samples)[i].y << endl;
					} else {
						cerr << "      " << (*samples)[i].x << "," << (*samples)[i].y << endl;
					}
				}
				si++;
			}
			cs++;
		}
	}
	/////
}
/*
 * run through curve loop to determine correct start/end
 * points resolving ambiguities when point lies on a seam or
 * singularity
 */
bool
resolve_pullback_seams(list<PBCData*> &pbcs) {
	list<PBCData*>::iterator cs;

	//TODO: remove debugging
	if (false)
		print_pullback_data("Before seam cleanup",pbcs,false);

	///// Loop through and fix any seam ambiguities
	ON_2dPoint *prev = NULL;
	bool complete = true;
	cs = pbcs.begin();
	while(cs!=pbcs.end()) {
		PBCData *data = (*cs);
		const ON_Surface *surf = data->surftree->getSurface();
		double umin,umax,umid;
		double vmin,vmax,vmid;
		surf->GetDomain(0,&umin,&umax);
		surf->GetDomain(1,&vmin,&vmax);
		umid = (umin+umax)/2.0;
		vmid = (vmin+vmax)/2.0;

		list<ON_2dPointArray *>::iterator si = data->segments.begin();
		int segcnt = 0;
		while (si != data->segments.end()) {
			ON_2dPointArray *samples = (*si);
			for(int i = 0; i < samples->Count(); i++) {
				int singularity=IsAtSingularity(surf,(*samples)[i].x,(*samples)[i].y);
				if (singularity < 0) {
					int seam=IsAtSeam(surf,(*samples)[i].x,(*samples)[i].y);
					if ((seam > 0) ) {
						if (prev != NULL) {
							//cerr << " at seam " << seam << " but has prev" << endl;
							//cerr << "    prev: " << prev->x << "," << prev->y << endl;
							//cerr << "    curr: " << data->samples[i].x << "," << data->samples[i].y << endl;
							switch (seam) {
							case 1: //east/west
								if (prev->x < umid) {
									(*samples)[i].x = umin;
								} else {
									(*samples)[i].x = umax;
								}
								break;
							case 2: //north/south
								if (prev->y < vmid) {
									(*samples)[i].y = vmin;
								} else {
									(*samples)[i].y = vmax;
								}
								break;
							case 3: //both
								if (prev->x < umid) {
									(*samples)[i].x = umin;
								} else {
									(*samples)[i].x = umax;
								}
								if (prev->y < vmid) {
									(*samples)[i].y = vmin;
								} else {
									(*samples)[i].y = vmax;
								}
							}
						} else {
							//cerr << " at seam and no prev" << endl;
							complete = false;
						}
					} else {
						prev = &(*samples)[i];
					}
				} else {
					prev = NULL;
				}
			}
			if ((!complete) && (prev != NULL)) {
				complete=true;
				for(int i = samples->Count()-2; i >= 0; i--) {
					int singularity=IsAtSingularity(surf,(*samples)[i].x,(*samples)[i].y);
					if (singularity < 0) {
						int seam=IsAtSeam(surf,(*samples)[i].x,(*samples)[i].y);
						if ((seam > 0) ) {
							if (prev != NULL) {
								//cerr << " at seam " << seam << " but has prev" << endl;
								//cerr << "    prev: " << prev->x << "," << prev->y << endl;
								//cerr << "    curr: " << data->samples[i].x << "," << data->samples[i].y << endl;
								switch (seam) {
								case 1: //east/west
									if (prev->x < umid) {
										(*samples)[i].x = umin;
									} else {
										(*samples)[i].x = umax;
									}
									break;
								case 2: //north/south
									if (prev->y < vmid) {
										(*samples)[i].y = vmin;
									} else {
										(*samples)[i].y = vmax;
									}
									break;
								case 3: //both
									if (prev->x < umid) {
										(*samples)[i].x = umin;
									} else {
										(*samples)[i].x = umax;
									}
									if (prev->y < vmid) {
										(*samples)[i].y = vmin;
									} else {
										(*samples)[i].y = vmax;
									}
								}
							} else {
								//cerr << " at seam and no prev" << endl;
								complete = false;
							}
						} else {
							prev = &(*samples)[i];
						}
					} else {
						prev = NULL;
					}
				}
			}
			prev = NULL;
			si++;
		}
//		cerr << " P1:" << endl;
//		for(int i = 0; i < data->samples.Count(); i++) {
//			if (i == 0) {
//				cerr << "--------" << data->samples[i].x << "," << data->samples[i].y << endl;
//			} else {
//				cerr << "        " << data->samples[i].x << "," << data->samples[i].y << endl;
//			}
//		}
		cs++;
	}
	if (!complete) {
		list<PBCData*>::reverse_iterator rcs;
		complete = true;
		rcs = pbcs.rbegin();
		while(rcs!=pbcs.rend()) {
			PBCData *data = (*rcs);
			const ON_Surface *surf = data->surftree->getSurface();
			double umin,umax,umid;
			double vmin,vmax,vmid;
			surf->GetDomain(0,&umin,&umax);
			surf->GetDomain(1,&vmin,&vmax);
			umid = (umin+umax)/2.0;
			vmid = (vmin+vmax)/2.0;

			list<ON_2dPointArray *>::reverse_iterator rsi = data->segments.rbegin();
			int segcnt = 0;
			while (rsi != data->segments.rend()) {
				ON_2dPointArray *samples = (*rsi);
				for(int i = samples->Count()-1; i >= 0; i--) {
					int singularity=IsAtSingularity(surf,(*samples)[i].x,(*samples)[i].y);
					if (singularity < 0) {
						int seam=IsAtSeam(surf,(*samples)[i].x,(*samples)[i].y);
						if ((seam > 0) ) {
							if (prev != NULL) {
								//cerr << " at seam " << seam << " but has prev" << endl;
								//cerr << "    prev: " << prev->x << "," << prev->y << endl;
								//cerr << "    curr: " << data->samples[i].x << "," << data->samples[i].y << endl;
								switch (seam) {
								case 1: //east/west
									if (prev->x < umid) {
										(*samples)[i].x = umin;
									} else {
										(*samples)[i].x = umax;
									}
									break;
								case 2: //north/south
									if (prev->y < vmid) {
										(*samples)[i].y = vmin;
									} else {
										(*samples)[i].y = vmax;
									}
									break;
								case 3: //both
									if (prev->x < umid) {
										(*samples)[i].x = umin;
									} else {
										(*samples)[i].x = umax;
									}
									if (prev->y < vmid) {
										(*samples)[i].y = vmin;
									} else {
										(*samples)[i].y = vmax;
									}
								}
							} else {
								//cerr << " at seam and no prev" << endl;
								complete = false;
							}
						} else {
							prev = &(*samples)[i];
						}
					} else {
						prev = NULL;
					}
				}
				prev = NULL;
				rsi++;
			}
			rcs++;
		}
//		cerr << " P2:" << endl;
//		for(int i = 0; i < data->samples.Count(); i++) {
//			if (i == 0) {
//				cerr << "--------" << data->samples[i].x << "," << data->samples[i].y << endl;
//			} else {
//				cerr << "        " << data->samples[i].x << "," << data->samples[i].y << endl;
//			}
//		}
	}

	if (!complete) {
		//cerr << "must be all seams pick one" << endl;
		prev = NULL;
		complete = true;
		cs = pbcs.begin();
		while(cs!=pbcs.end()) {
			PBCData *data = (*cs);
			const ON_Surface *surf = data->surftree->getSurface();
			double umin,umax,umid;
			double vmin,vmax,vmid;
			surf->GetDomain(0,&umin,&umax);
			surf->GetDomain(1,&vmin,&vmax);
			umid = (umin+umax)/2.0;
			vmid = (vmin+vmax)/2.0;

			list<ON_2dPointArray *>::iterator si = data->segments.begin();
			int segcnt = 0;
			while (si != data->segments.end()) {
				ON_2dPointArray *samples = (*si);
				for(int i = 0; i < samples->Count(); i++) {
					int seam;
					if ((seam=IsAtSeam(surf,(*samples)[i].x,(*samples)[i].y)) > 0) {
						if (prev != NULL) {
							//cerr << " at seam " << seam << " but has prev" << endl;
							//cerr << "    prev: " << prev->x << "," << prev->y << endl;
							//cerr << "    curr: " << data->samples[i].x << "," << data->samples[i].y << endl;
							switch (seam) {
							case 1: //east/west
								if (prev->x < umid) {
									(*samples)[i].x = umin;
								} else {
									(*samples)[i].x = umax;
								}
								break;
							case 2: //north/south
								if (prev->y < vmid) {
									(*samples)[i].y = vmin;
								} else {
									(*samples)[i].y = vmax;
								}
								break;
							case 3: //both
								if (prev->x < umid) {
									(*samples)[i].x = umin;
								} else {
									(*samples)[i].x = umax;
								}
								if (prev->y < vmid) {
									(*samples)[i].y = vmin;
								} else {
									(*samples)[i].y = vmax;
								}
							}
						} else {
							//cerr << " at seam and no prev" << endl;
							complete = false;
							if ((seam == 1) || (seam ==2)) {
								prev = &(*samples)[i];
							}
						}
					} else {
						prev = &(*samples)[i];
					}
				}
				si++;
			}
	//		cerr << " P1:" << endl;
	//		for(int i = 0; i < data->samples.Count(); i++) {
	//			if (i == 0) {
	//				cerr << "--------" << data->samples[i].x << "," << data->samples[i].y << endl;
	//			} else {
	//				cerr << "        " << data->samples[i].x << "," << data->samples[i].y << endl;
	//			}
	//		}
			cs++;
		}
		if (!complete) {
			list<PBCData*>::reverse_iterator rcs;
			complete = true;
			rcs = pbcs.rbegin();
			while(rcs!=pbcs.rend()) {
				PBCData *data = (*rcs);
				const ON_Surface *surf = data->surftree->getSurface();
				double umin,umax,umid;
				double vmin,vmax,vmid;
				surf->GetDomain(0,&umin,&umax);
				surf->GetDomain(1,&vmin,&vmax);
				umid = (umin+umax)/2.0;
				vmid = (vmin+vmax)/2.0;

				list<ON_2dPointArray *>::reverse_iterator rsi = data->segments.rbegin();
				int segcnt = 0;
				while (rsi != data->segments.rend()) {
					ON_2dPointArray *samples = (*rsi);
					for(int i = samples->Count()-1; i >= 0; i--) {
						int seam;
						if ((seam=IsAtSeam(surf,(*samples)[i].x,(*samples)[i].y)) > 0) {
							if (prev != NULL) {
								//cerr << " at seam " << seam << " but has prev" << endl;
								//cerr << "    prev: " << prev->x << "," << prev->y << endl;
								//cerr << "    curr: " << data->samples[i].x << "," << data->samples[i].y << endl;
								switch (seam) {
								case 1: //east/west
									if (prev->x < umid) {
										(*samples)[i].x = umin;
									} else {
										(*samples)[i].x = umax;
									}
									break;
								case 2: //north/south
									if (prev->y < vmid) {
										(*samples)[i].y = vmin;
									} else {
										(*samples)[i].y = vmax;
									}
									break;
								case 3: //both
									if (prev->x < umid) {
										(*samples)[i].x = umin;
									} else {
										(*samples)[i].x = umax;
									}
									if (prev->y < vmid) {
										(*samples)[i].y = vmin;
									} else {
										(*samples)[i].y = vmax;
									}
								}
							} else {
								//cerr << " at seam and no prev" << endl;
								complete = false;
							}
						} else {
							prev = &(*samples)[i];
						}
					}
					rsi++;
				}
				rcs++;
			}
	//		cerr << " P2:" << endl;
	//		for(int i = 0; i < data->samples.Count(); i++) {
	//			if (i == 0) {
	//				cerr << "--------" << data->samples[i].x << "," << data->samples[i].y << endl;
	//			} else {
	//				cerr << "        " << data->samples[i].x << "," << data->samples[i].y << endl;
	//			}
	//		}
		}
	}
	//TODO: remove debugging
	if (false)
		print_pullback_data("After seam cleanup",pbcs,false);

	return true;
}

/*
 * run through curve loop to determine correct start/end
 * points resolving ambiguities when point lies on a seam or
 * singularity
 */
bool
resolve_pullback_singularities(list<PBCData*> &pbcs) {
	list<PBCData*>::iterator cs = pbcs.begin();

	//TODO: remove debugging
	if (false)
		print_pullback_data("Before singularity cleanup",pbcs,false);

	///// Loop through and fix any seam ambiguities
	ON_2dPoint *prev = NULL;
	bool complete = false;
	int checkcnt = 0;

	prev = NULL;
	complete = false;
	checkcnt = 0;
	while (!complete && (checkcnt < 2)) {
		cs = pbcs.begin();
		complete = true;
		checkcnt++;
		//cerr << "Checkcnt - " << checkcnt << endl;
		while(cs!=pbcs.end()) {
			int singularity;
			prev=NULL;
			PBCData *data = (*cs);
			const ON_Surface *surf = data->surftree->getSurface();
			list<ON_2dPointArray *>::iterator si = data->segments.begin();
			int segcnt = 0;
			while (si != data->segments.end()) {
				ON_2dPointArray *samples = (*si);
				int last = samples->Count() - 1;
				for(int i = 0; i < samples->Count(); i++) {
					// 0 = south, 1 = east, 2 = north, 3 = west
					if ((singularity=IsAtSingularity(surf,(*samples)[i].x,(*samples)[i].y)) >= 0) {
						if (prev != NULL) {
							//cerr << " at singularity " << singularity << " but has prev" << endl;
							//cerr << "    prev: " << prev->x << "," << prev->y << endl;
							//cerr << "    curr: " << data->samples[i].x << "," << data->samples[i].y << endl;
							switch (singularity) {
							case 0: //south
								(*samples)[i].x = prev->x;
								(*samples)[i].y = surf->Domain(1)[0];
								break;
							case 1: //east
								(*samples)[i].y = prev->y;
								(*samples)[i].x = surf->Domain(0)[1];
								break;
							case 2: //north
								(*samples)[i].x = prev->x;
								(*samples)[i].y = surf->Domain(1)[1];
								break;
							case 3: //west
								(*samples)[i].y = prev->y;
								(*samples)[i].x = surf->Domain(0)[0];
							}
							prev = NULL;
							//cerr << "    curr now: " << data->samples[i].x << "," << data->samples[i].y << endl;
						} else {
							//cerr << " at singularity " << singularity << " and no prev" << endl;
							//cerr << "    curr: " << data->samples[i].x << "," << data->samples[i].y << endl;
							complete = false;
						}
					} else {
						prev = &(*samples)[i];
					}
				}
				if (!complete) {
					//cerr << "Lets work backward:" << endl;
					for(int i = samples->Count()-2; i >= 0; i--) {
						// 0 = south, 1 = east, 2 = north, 3 = west
						if ((singularity=IsAtSingularity(surf,(*samples)[i].x,(*samples)[i].y)) >= 0) {
							if (prev != NULL) {
								//cerr << " at singularity " << singularity << " but has prev" << endl;
								//cerr << "    prev: " << prev->x << "," << prev->y << endl;
								//cerr << "    curr: " << data->samples[i].x << "," << data->samples[i].y << endl;
								switch (singularity) {
								case 0: //south
									(*samples)[i].x = prev->x;
									(*samples)[i].y = surf->Domain(1)[0];
									break;
								case 1: //east
									(*samples)[i].y = prev->y;
									(*samples)[i].x = surf->Domain(0)[1];
									break;
								case 2: //north
									(*samples)[i].x = prev->x;
									(*samples)[i].y = surf->Domain(1)[1];
									break;
								case 3: //west
									(*samples)[i].y = prev->y;
									(*samples)[i].x = surf->Domain(0)[0];
								}
								prev = NULL;
								//cerr << "    curr now: " << data->samples[i].x << "," << data->samples[i].y << endl;
							} else {
								//cerr << " at singularity " << singularity << " and no prev" << endl;
								//cerr << "    curr: " << data->samples[i].x << "," << data->samples[i].y << endl;
								complete = false;
							}
						} else {
							prev = &(*samples)[i];
						}
					}
				}
				si++;
			}
			cs++;
		}
	}

	//TODO: remove debugging
	if (false)
		print_pullback_data("After singularity cleanup",pbcs,false);

	return true;
}

bool
check_pullback_data(list<PBCData*> &pbcs) {
	bool resolvable = true;
	bool resolved = false;
	list<PBCData*>::iterator d = pbcs.begin();

	//TODO: remove debugging code
	if (false)
		print_pullback_data("Before cleanup",pbcs,false);

	const ON_Surface *surf = (*d)->surftree->getSurface();

	bool singular = has_singularity(surf);
	bool closed = is_closed(surf);
	if (!singular && !closed) { //shouldn't have to do anything
		return true;
	}
	if (closed) {
		if (!resolve_pullback_seams(pbcs)) {
			cerr << "Error: Can not resolve seam ambiguities." << endl;
			return false;
		}
	}
	if (singular) {
		if (!resolve_pullback_singularities(pbcs)) {
			cerr << "Error: Can not resolve singular ambiguities." << endl;
		}
	}
	//TODO: remove debugging code
	if (false)
		print_pullback_data("After cleanup",pbcs,false);

	return true;
}

int
check_pullback_singularity_bridge(const ON_Surface *surf, const ON_2dPoint &p1,const ON_2dPoint &p2) {
	if (has_singularity(surf)) {
		int is,js;
		if (((is=IsAtSingularity(surf,p1.x,p1.y)) >= 0) && ((js=IsAtSingularity(surf,p2.x,p2.y)) >= 0)) {
			//create new singular trim
			if (is == js) {
				return is;
			}
		}
	}
	return -1;
}

int
check_pullback_seam_bridge(const ON_Surface *surf, const ON_2dPoint &p1,const ON_2dPoint &p2) {
	if (is_closed(surf)) {
		int is,js;
		if (((is=IsAtSeam(surf,p1.x,p1.y)) >= 0) && ((js=IsAtSeam(surf,p2.x,p2.y)) >= 0)) {
			//create new seam trim
			if (is == js) {
				return is;
			}
		}
	}
	return -1;
}

ON_Curve*
pullback_curve(const SurfaceTree* surfacetree,
		const ON_Curve* curve,
		double tolerance,
		double flatness) {
  PBCData data;
  data.tolerance = tolerance;
  data.flatness = flatness;
  data.curve = curve;
  data.surftree = (SurfaceTree*)surfacetree;
  ON_2dPointArray samples;
  data.segments.push_back(&samples);

  // Step 1 - adaptively sample the curve
  double tmin, tmax;
  data.curve->GetDomain(&tmin, &tmax);
  ON_2dPoint& start = samples.AppendNew(); // new point is added to samples and returned
  if (!toUV(data, start, tmin,0.001)) { return NULL; } // fails if first point is out of tolerance!

  ON_2dPoint uv;
  ON_3dPoint p = curve->PointAt(tmin);
  ON_3dPoint from = curve->PointAt(tmin+0.0001);
  SurfaceTree *st = (SurfaceTree *)surfacetree;
  if ( !st->getSurfacePoint((const ON_3dPoint&)p,uv,(const ON_3dPoint&)from) > 0 ) {
  	cerr << "Error: Can not get surface point." << endl;
  }

  ON_2dPoint p1, p2;
  double psize = tmax - tmin;

  const ON_Surface *surf = (data.surftree)->getSurface();

  toUV(data, p1, tmin, PBC_TOL);
  ON_3dPoint a = surf->PointAt(p1.x,p1.y);
  toUV(data, p2, tmax,-PBC_TOL);
  ON_3dPoint b = surf->PointAt(p2.x,p2.y);

  p = curve->PointAt(tmax);
  from = curve->PointAt(tmax-0.0001);
  if ( !st->getSurfacePoint((const ON_3dPoint&)p,uv,(const ON_3dPoint&)from) > 0 ) {
	  	cerr << "Error: Can not get surface point." << endl;
  }

  if (!sample(data, tmin, tmax, p1, p2)) {
    return NULL;
  }

  for (int i = 0; i < samples.Count(); i++) {
    cerr << samples[i].x << "," << samples[i].y << endl;
  }

  return interpolateCurve(samples);
}

ON_Curve*
pullback_seam_curve(enum seam_direction seam_dir,
		const SurfaceTree* surfacetree,
		const ON_Curve* curve,
		double tolerance,
		double flatness) {
  PBCData data;
  data.tolerance = tolerance;
  data.flatness = flatness;
  data.curve = curve;
  data.surftree = (SurfaceTree*)surfacetree;
  ON_2dPointArray samples;
  data.segments.push_back(&samples);

  // Step 1 - adaptively sample the curve
  double tmin, tmax;
  data.curve->GetDomain(&tmin, &tmax);
  ON_2dPoint& start = samples.AppendNew(); // new point is added to samples and returned
  if (!toUV(data, start, tmin,0.001)) { return NULL; } // fails if first point is out of tolerance!

  ON_2dPoint p1, p2;
  double psize = tmax - tmin;

  toUV(data, p1, tmin, PBC_TOL);
  toUV(data, p2, tmax,-PBC_TOL);
  if (!sample(data, tmin, tmax, p1, p2)) {
    return NULL;
  }

  for (int i = 0; i < samples.Count(); i++) {
	  if (seam_dir == NORTH_SEAM) {
		  samples[i].y = 1.0;
	  } else if (seam_dir == EAST_SEAM) {
		  samples[i].x = 1.0;
	  } else if (seam_dir == SOUTH_SEAM) {
		  samples[i].y = 0.0;
	  } else if (seam_dir == WEST_SEAM) {
		  samples[i].x = 0.0;
	  }
    cerr << samples[i].x << "," << samples[i].y << endl;
  }

  return interpolateCurve(samples);
}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
