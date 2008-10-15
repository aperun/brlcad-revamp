/* $Header$ */
/* $NoKeywords: $ */
/*
//
// Copyright (c) 1993-2007 Robert McNeel & Associates. All rights reserved.
// Rhinoceros is a registered trademark of Robert McNeel & Assoicates.
//
// THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
// ALL IMPLIED WARRANTIES OF FITNESS FOR ANY PARTICULAR PURPOSE AND OF
// MERCHANTABILITY ARE HEREBY DISCLAIMED.
//				
// For complete openNURBS copyright information see <http://www.opennurbs.org>.
//
////////////////////////////////////////////////////////////////
*/

#include "opennurbs.h"

const ON_Plane ON_xy_plane( ON_3dPoint(0.0,0.0,0.0),
			      ON_3dVector(1.0,0.0,0.0),
			      ON_3dVector(0.0,1.0,0.0) 
			      );

const ON_Plane ON_yz_plane( ON_3dPoint(0.0,0.0,0.0),
			      ON_3dVector(0.0,1.0,0.0), 
			      ON_3dVector(0.0,0.0,1.0) 
			      );

const ON_Plane ON_zx_plane( ON_3dPoint(0.0,0.0,0.0),
			      ON_3dVector(0.0,0.0,1.0),
			      ON_3dVector(1.0,0.0,0.0)
			      );

const ON_Plane ON_Plane::World_xy=ON_xy_plane;

ON_Plane::ON_Plane() 
	: origin(0.0,0.0,0.0), 
	  xaxis(1.0,0.0,0.0), yaxis(0.0,1.0,0.0), zaxis(0.0,0.0,1.0)
{
  plane_equation.x = plane_equation.y = plane_equation.d = 0.0;
  plane_equation.z = 1.0;
}

ON_Plane::ON_Plane(
    const ON_3dPoint&  P, // point on the plane
    const ON_3dVector& N  // non-zero normal to the plane
    )
{
  CreateFromNormal(P,N);
}

ON_Plane::ON_Plane(
    const ON_3dPoint&  P, // point on the plane
    const ON_3dVector& X, // non-zero vector in plane
    const ON_3dVector& Y  // another vector in the plane not parallel to X
    )
{
  CreateFromFrame(P,X,Y);
}

ON_Plane::ON_Plane(
    const ON_3dPoint& P,  // point on the plane
    const ON_3dPoint& Q,  // point on the plane
    const ON_3dPoint& R   // point on the plane
    )
{
  CreateFromPoints(P,Q,R);
}

ON_Plane::ON_Plane(
    const double e[4]    // equation of plane
    )
{
  CreateFromEquation(e);
}

ON_Plane::~ON_Plane()
{}

ON_3dPoint ON_Plane::PointAt( double s, double t ) const
{
  return (origin + s*xaxis + t*yaxis);
}

ON_3dPoint ON_Plane::PointAt( double s, double t, double c ) const
{
  return (origin + s*xaxis + t*yaxis + c*zaxis);
}


ON_Line ON_Plane::IsoLine( // iso parametric line
       int dir,              // 0 first parameter varies and second parameter is constant
			 //   e.g., line(t) = plane(t,c)
			 // 1 first parameter is constant and second parameter varies
			 //   e.g., line(t) = plane(c,t)
       double c           // c = value of constant parameter 
       ) const
{
  ON_Line line;
  if ( dir ) {
    line.from = PointAt( 0.0, c );
    line.to   = PointAt( 1.0, c );
  }
  else {
    line.from = PointAt( c, 0.0 );
    line.to   = PointAt( c, 1.0 );
  }
  return line;
}

double ON_Plane::DistanceTo( const ON_3dPoint& point ) const
{
  // signed distance
  // N.B. equation may not be normalized
  return ( point - origin)*zaxis;
}

bool ON_Plane::GetDistanceToBoundingBox(const ON_BoundingBox& Box,
								double* min, double* max) const
{
  //min and max signed distance.  Returns false if plane normal has zero length.

  ON_3dVector UnitNormal = Normal();
  if (!UnitNormal.Unitize()) 
    return false;

  double mind, maxd;
  mind = maxd = (Box.Min() - Origin())*UnitNormal;
  int i0, i1, i2;
  for (i0=0;i0<2;i0++)
  {
    for(i1=0;i1<2;i1++) 
    {
      for (i2=0;i2<2;i2++) 
      {
	if (i0||i1||i2) 
	{
	  ON_3dPoint P;
	  P[0]=(i0)?Box.Max()[0]:Box.Min()[0];
	  P[1]=(i1)?Box.Max()[1]:Box.Min()[1];
	  P[2]=(i2)?Box.Max()[2]:Box.Min()[2];
	  double d = (P - Origin())*UnitNormal;
	  //double dd = P.DistanceTo(ClosestPointTo(P));
	  if (d < mind) 
	    mind=d;
	  else if (d > maxd) 
	    maxd=d;
	}
      }
    }
  }
  *min = mind;
  *max = maxd;
  return true;
}


//double ON_Plane::EquationAt( const ON_3dPoint& p) const
//{
//  return plane_equation.ValueAt(p);
//}

//double ON_Plane::EquationAt( const ON_4dPoint& p) const
//{
//  return plane_equation.ValueAt(p);
//}

bool ON_Plane::UpdateEquation()
{
  // computes equation[] from origin and zaxis.
  return plane_equation.Create(origin,zaxis);
}

const ON_3dPoint& ON_Plane::Origin() const
{
  return origin;
}

const ON_3dVector& ON_Plane::Xaxis() const
{
  return xaxis;
}

const ON_3dVector& ON_Plane::Yaxis() const
{
  return yaxis;
}

const ON_3dVector& ON_Plane::Normal() const
{
  return zaxis;
}

void ON_Plane::SetOrigin( const ON_3dPoint& origin_point)
{
  origin = origin_point;
  UpdateEquation();
}


bool ON_Plane::ClosestPointTo( ON_3dPoint p, double* s, double* t ) const
{
  const ON_3dVector v = p - origin;
  if ( s )
    *s = v*xaxis;
  if ( t )
    *t = v*yaxis;
  return true;
}

ON_3dPoint ON_Plane::ClosestPointTo( ON_3dPoint p ) const
{
  double s, t;
  ClosestPointTo( p, &s, &t );
  return PointAt( s, t );
}


bool ON_Plane::operator==(const ON_Plane& other) const
{
  return (origin==other.origin && xaxis==other.xaxis && yaxis==other.yaxis && zaxis==other.zaxis)
    ? true : false;
}

bool ON_Plane::operator!=(const ON_Plane& other) const
{
  return ON_Plane::operator==(other)?false:true;
}


bool ON_Plane::CreateFromNormal(
    const ON_3dPoint&  P, // point on the plane
    const ON_3dVector& N  // non-zero normal to the plane
    )
{
  origin = P;
  zaxis = N;
  bool b = zaxis.Unitize();
  xaxis.PerpendicularTo( zaxis );
  xaxis.Unitize();
  yaxis = ON_CrossProduct( zaxis, xaxis );
  yaxis.Unitize();

  UpdateEquation();

  return b;
}

bool ON_Plane::CreateFromFrame(
    const ON_3dPoint&  P, // point on the plane
    const ON_3dVector& X, // non-zero vector in plane
    const ON_3dVector& Y  // another non-zero vector in the plane
    )
{
  origin = P;

  xaxis = X;
  xaxis.Unitize();
  yaxis = Y - ON_DotProduct( Y, xaxis)*xaxis;
  yaxis.Unitize();
  zaxis = ON_CrossProduct( xaxis, yaxis );
  bool b = zaxis.Unitize();
  UpdateEquation();
  if ( b )
  {
    // 11 February 2004 Dale Lear
    //     Add more validation checks.
    b = IsValid();
    if ( b )
    {
      // make sure zaxis is perp to Y
      if ( fabs(Y*zaxis) > ON_SQRT_EPSILON*Y.Length() )
	b = false;
    }
  }
  return b;
}

bool ON_Plane::CreateFromEquation(
    const double e[4]    // equation of plane
    )
{
  bool b = false;
  plane_equation.x = e[0];
  plane_equation.y = e[1];
  plane_equation.z = e[2];
  plane_equation.d = e[3];

  zaxis.x = e[0];
  zaxis.y = e[1];
  zaxis.z = e[2];

  double d = zaxis.Length();
  if ( d > 0.0 ) {
    d = 1.0/d;
    origin = (-d*plane_equation.d)*zaxis;
    b = true;
  }
  xaxis.PerpendicularTo( zaxis );
  xaxis.Unitize();
  yaxis = ON_CrossProduct( zaxis, xaxis );
  yaxis.Unitize();
  return b;
}

bool ON_Plane::CreateFromPoints(
	  const ON_3dPoint& P,  // point on the plane
	  const ON_3dPoint& Q,  // point on the plane
	  const ON_3dPoint& R   // point on the plane
	  )
{
  origin = P;
  bool rc = zaxis.PerpendicularTo(P,Q,R);
  xaxis = Q - P;
  xaxis.Unitize();
  yaxis = ON_CrossProduct( zaxis, xaxis );
  yaxis.Unitize();

  if ( !plane_equation.Create(origin,zaxis) )
    rc = false;

  return rc;
}

bool ON_Plane::IsValid() const
{
  if ( !plane_equation.IsValid() )
    return false;


  double x = plane_equation.ValueAt(origin);
  if ( fabs(x) >  ON_ZERO_TOLERANCE )
  {
    double tol = fabs(origin.MaximumCoordinate()) + fabs(plane_equation.d);
    if ( tol > 1000.0 && origin.IsValid() )
    {
      // 8 September 2003 Chuck and Dale:
      //   Fixing discrepancy between optimized and debug behavior.
      //   In this case, the ON_ZERO_TOLERANCE test worked in release
      //   and failed in debug. The principal behind this fix is valid
      //   for release builds too.
      //   For large point coordinates or planes far from the origin,
      //   the best we can hope for is to kill the first 15 or so decimal
      //   places.
      tol *= (ON_EPSILON*10.0);
      if ( fabs(x) > tol )
	return false;
    }
    else
      return false;
  }

  if ( !ON_IsRightHandFrame( xaxis, yaxis, zaxis ) )
    return false;

  ON_3dVector N = plane_equation;
  N.Unitize();
  x = ON_DotProduct( N, zaxis );
  if ( fabs(x-1.0) >  ON_SQRT_EPSILON )
    return false;

  return true;
}


bool ON_Plane::Transform( const ON_Xform& xform )
{
  ON_3dPoint origin_pt = xform*origin;
  // use care tranforming vectors 
  ON_3dVector xaxis_vec = (xform*(origin+xaxis)) - origin_pt;
  ON_3dVector yaxis_vec = (xform*(origin+yaxis)) - origin_pt;

  return CreateFromFrame( origin_pt, xaxis_vec, yaxis_vec );
}


bool ON_Plane::Morph( const ON_SpaceMorph& morph )
{
  ON_Plane mp;
  double s = sqrt( fabs(origin.MaximumCoordinate())*ON_SQRT_EPSILON + ON_ZERO_TOLERANCE );
  mp.xaxis = morph.MorphVector(origin,s*xaxis);
  mp.yaxis = morph.MorphVector(origin,s*yaxis);
  mp.zaxis = morph.MorphVector(origin,s*zaxis);
  origin = morph.MorphPoint(origin);
  UpdateEquation();
  bool bx = mp.xaxis.Unitize();
  bool by = mp.yaxis.Unitize();
  bool bz = mp.zaxis.Unitize();
  if (!bx)
  {
    mp.xaxis = ON_CrossProduct(mp.yaxis,mp.zaxis);
    bx = mp.xaxis.Unitize();
  }
  if (!by)
  {
    mp.yaxis = ON_CrossProduct(mp.zaxis,mp.xaxis);
    by = mp.yaxis.Unitize();
  }
  if (!bz)
  {
    mp.zaxis = ON_CrossProduct(mp.xaxis,mp.yaxis);
    bz = mp.zaxis.Unitize();
  }

  mp.origin.Set(0.0,0.0,0.0);
  mp.UpdateEquation();
  bool rc = mp.IsValid();
  ON_3dVector x, y, z;
  if ( rc )
  {
    x = mp.xaxis;
    y = mp.yaxis;
    z = mp.zaxis;
  }
  else
  {
    x = ON_CrossProduct(mp.yaxis,mp.zaxis);
    y = ON_CrossProduct(mp.zaxis,mp.xaxis);
    z = ON_CrossProduct(mp.xaxis,mp.yaxis);
    x.Unitize();
    y.Unitize();
    z.Unitize();
    x = mp.xaxis + x;
    y = mp.yaxis + y;
    z = mp.zaxis + z;
    x.Unitize();
    y.Unitize();
    z.Unitize();
    rc = mp.CreateFromFrame(ON_origin,x,y);
    if (rc)
    {
      x = mp.xaxis;
      y = mp.yaxis;
      z = mp.zaxis;
    }
    else
    {
      rc = mp.CreateFromFrame(ON_origin,y,z);
      if ( rc )
      {
	y = mp.xaxis;
	z = mp.yaxis;
	x = mp.zaxis;
      }
      else
      {
	rc = mp.CreateFromFrame(ON_origin,z,x);
	if (rc)
	{
	  z = mp.xaxis;
	  x = mp.yaxis;
	  y = mp.zaxis;
	}
	else
	{
	  rc = mp.CreateFromNormal(ON_origin,z);
	  if (rc)
	  {
	    x = mp.xaxis;
	    y = mp.yaxis;
	    z = mp.zaxis;
	  }
	}
      }
    }
  }

  if (rc)
  {
    xaxis = x;
    yaxis = y;
    zaxis = z;
    UpdateEquation();
  }

  return rc;
}


bool ON_Plane::SwapCoordinates( int i, int j )
{
  bool rc = false;
  if ( 0 <= i && i < 3 && 0 <= j && j < 3 ) {
    ON_Xform xform(1);
    xform[i][i] = 0.0;
    xform[j][j] = 0.0;
    xform[i][j] = 1.0;
    xform[j][i] = 1.0;
    rc = Transform(xform);
  }
  return rc;
}


// rotate plane about its origin_pt
bool ON_Plane::Rotate(
      double s,                 // sin(angle)
      double c,                 // cos(angle)
      const ON_3dVector& axis // axis of rotation
      )
{
  bool rc = true;
  if ( axis == zaxis ) {
    ON_3dVector x = c*xaxis + s*yaxis;
    ON_3dVector y = c*yaxis - s*xaxis;
    xaxis = x;
    yaxis = y;
  }
  else {
    ON_3dPoint origin_pt = origin;
    rc = Rotate( s, c, axis, origin );
    origin = origin_pt; // to kill any fuzz
  }
  return rc;
}

bool ON_Plane::Rotate(
      double angle,           // angle in radians
      const ON_3dVector& axis // axis of rotation
      )
{
  return Rotate( sin(angle), cos(angle), axis );
}

// rotate plane about a point and axis
bool ON_Plane::Rotate(
      double sin_angle,          // sin(angle)
      double cos_angle,          // cos(angle)
      const ON_3dVector& axis, // axis of rotation
      const ON_3dPoint& center // center of rotation
      )
{
  bool rc = false;
  ON_Xform rot;
  if ( center == origin ) {
    rot.Rotation( sin_angle, cos_angle, axis, ON_origin );
    xaxis = rot*xaxis;
    yaxis = rot*yaxis;
    zaxis = rot*zaxis;
    UpdateEquation();
  }
  else {
    rot.Rotation( sin_angle, cos_angle, axis, center );
    rc = Transform( rot );
  }
  return rc;
}

bool ON_Plane::Rotate(
      double a,                   // angle in radians
      const ON_3dVector& axis,  // axis of rotation
      const ON_3dPoint& origin_pt  // center of rotation
      )
{
  return Rotate( sin(a), cos(a), axis, origin_pt );
}

bool ON_Plane::Translate(
      const ON_3dVector& delta
      )
{
  ON_Xform tr;
  tr.Translation( delta );
  return Transform( tr );
}

bool ON_Plane::Flip()
{
  ON_3dVector v = xaxis;
  xaxis = yaxis;
  yaxis = v;
  zaxis = -zaxis;
  UpdateEquation();
  return true;
}


int ON_ArePointsOnPlane( // returns 0=no, 1 = yes, 2 = pointset is (to tolerance) a single point on the line
	int dim,     // 2 or 3
	int is_rat,
	int count, 
	int stride, const double* point,
	const ON_BoundingBox& bbox, // if needed, use ON_GetBoundingBox(dim,is_rat,count,stride,point)
	const ON_Plane& plane,  // line to test
	double tolerance
	)
{
  double w;
  int rc, i, j, k;

  rc = 0;
  ON_ASSERT_OR_RETURN( plane.IsValid(), rc ); 
  ON_ASSERT_OR_RETURN( bbox.IsValid(), rc ); 
  ON_ASSERT_OR_RETURN( tolerance >= 0.0, rc );
  if ( count < 1 )
    return rc;
  ON_ASSERT_OR_RETURN( dim == 2 || dim == 3, rc );
  ON_ASSERT_OR_RETURN( stride >= (is_rat?(dim+1):dim) && point != NULL, rc );

  if ( tolerance == 0.0 ) {
    tolerance = bbox.Tolerance();
  }

  ON_3dPoint Q;

  // test bounding box to quickly detect the common coordinate axis cases
  rc = (count == 1 || bbox.Diagonal().Length() <= tolerance) ? 2 : 1;
  for ( i = 0; rc && i < 2; i++ ) {
    Q.x = bbox[i].x;
    for ( j = 0; rc && j < 2; j++) {
      Q.y = bbox[j].y;
      for ( k = 0; rc && k < 2; k++) {
	Q.z = bbox[k].z;
	if ( Q.DistanceTo( plane.ClosestPointTo( Q ) ) > tolerance )
	  rc = 0;
      }
    }
  }

  if ( !rc ) {
    // test points one by one
    Q.Zero();
    rc = (count == 1 || bbox.Diagonal().Length() <= tolerance) ? 2 : 1;
    if ( is_rat ) {
      for ( i = 0; i < count; i++ ) {
	w = point[dim];
	if ( w == 0.0 ) {
	  ON_ERROR("rational point has zero weight");
	  return 0;
	}
	ON_ArrayScale( dim, 1.0/w, point, &Q.x );
	if ( Q.DistanceTo( plane.ClosestPointTo( Q ) ) > tolerance ) {
	  rc = 0;
	  break;
	}
	point += stride;
      }
    }
    else {
      for ( i = 0; i < count; i++ ) {
	memcpy( &Q.x, point, dim*sizeof(Q.x) );
	if ( Q.DistanceTo( plane.ClosestPointTo( Q ) ) > tolerance ) {
	  rc = 0;
	  break;
	}
	point += stride;
      }
    }
  }

  return rc;
}

