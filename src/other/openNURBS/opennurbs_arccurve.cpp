/* $Header$ */
/* $NoKeywords: $ */
/*
//
// Copyright (c) 1993-2001 Robert McNeel & Associates. All rights reserved.
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

ON_OBJECT_IMPLEMENT(ON_ArcCurve,ON_Curve,"CF33BE2A-09B4-11d4-BFFB-0010830122F0");


ON_ArcCurve::ON_ArcCurve()
{
  m_arc.Create(ON_xy_plane, 1.0, 2.0*ON_PI);
  m_t.m_t[0] = 0.0;
  m_t.m_t[1] = m_arc.Length();
  m_dim = 3;
}

ON_ArcCurve::ON_ArcCurve( const ON_Arc& A )
{
  m_arc = A;
  m_t.m_t[0] = 0.0;
  m_t.m_t[1] = m_arc.Length();
  if ( m_t.m_t[1] <= 0.0 )
    m_t.m_t[1] = 1.0;
  m_dim = 3;
}

ON_ArcCurve::ON_ArcCurve( const ON_Circle& circle )
{
  ON_ArcCurve::operator=(circle);
}

ON_ArcCurve::ON_ArcCurve( const ON_Arc& A, double t0, double t1 )
{
  m_arc = A;
  m_t.m_t[0] = t0;
  m_t.m_t[1] = t1;
  m_dim = 3;
}

ON_ArcCurve::ON_ArcCurve( const ON_Circle& circle, double t0, double t1 )
{
  m_arc = circle;
  m_t.m_t[0] = t0;
  m_t.m_t[1] = t1;
  m_dim = 3;
}

ON_ArcCurve::ON_ArcCurve( const ON_ArcCurve& src ) : ON_Curve(src)
{
  m_arc = src.m_arc;
  m_t = src.m_t;
  m_dim  = src.m_dim;
}

ON_ArcCurve::~ON_ArcCurve()
{
}


unsigned int ON_ArcCurve::SizeOf() const
{
  unsigned int sz = ON_Curve::SizeOf();
  sz += sizeof(*this) - sizeof(ON_Curve);
  return sz;
}

ON__UINT32 ON_ArcCurve::DataCRC(ON__UINT32 current_remainder) const
{
  current_remainder = ON_CRC32(current_remainder,sizeof(m_arc),&m_arc);
  current_remainder = ON_CRC32(current_remainder,sizeof(m_t),&m_t);
  current_remainder = ON_CRC32(current_remainder,sizeof(m_dim),&m_dim);
  return current_remainder;
}

ON_ArcCurve& ON_ArcCurve::operator=( const ON_ArcCurve& src )
{
  if ( this != &src ) {
    ON_Curve::operator=(src);
    m_arc = src.m_arc;
    m_t = src.m_t;
    m_dim  = src.m_dim;
  }
  return *this;
}

ON_ArcCurve& ON_ArcCurve::operator=( const ON_Arc& A )
{
  m_arc = A;
  m_t.m_t[0] = 0.0;
  m_t.m_t[1] = A.Length();
  if ( m_t.m_t[1] == 0.0 )
    m_t.m_t[1] = 1.0;
  m_dim = 3;
  return *this;
}

ON_ArcCurve& ON_ArcCurve::operator=(const ON_Circle& circle)
{
  m_arc = circle;
  m_t.m_t[0] = 0.0;
  m_t.m_t[1] = m_arc.Length();
  if ( m_t.m_t[1] <= 0.0 )
    m_t.m_t[1] = 1.0;
  m_dim = 3;
  return *this;
}

int ON_ArcCurve::Dimension() const
{
  return m_dim;
}

BOOL
ON_ArcCurve::GetBBox( // returns true if successful
         double* boxmin,    // minimum
         double* boxmax,    // maximum
         BOOL bGrowBox
         ) const
{
  BOOL rc = m_arc.IsValid();
  if (rc) {
    ON_BoundingBox bbox = m_arc.BoundingBox();
    if ( bGrowBox ) {
      if ( boxmin[0] > bbox.m_min.x ) boxmin[0] = bbox.m_min.x;
      if ( boxmin[1] > bbox.m_min.y ) boxmin[1] = bbox.m_min.y;
      if ( boxmax[0] < bbox.m_max.x ) boxmax[0] = bbox.m_max.x;
      if ( boxmax[1] < bbox.m_max.y ) boxmax[1] = bbox.m_max.y;
      if ( m_dim > 2 ) {
        if ( boxmin[2] > bbox.m_min.z ) boxmin[2] = bbox.m_min.z;
        if ( boxmax[2] < bbox.m_max.z ) boxmax[2] = bbox.m_max.z;
      }
    }
    else {
      boxmin[0] = bbox.m_min.x;
      boxmin[1] = bbox.m_min.y;
      boxmax[0] = bbox.m_max.x;
      boxmax[1] = bbox.m_max.y;
      if ( m_dim > 2 ) {
        boxmin[2] = bbox.m_min.z;
        boxmax[2] = bbox.m_max.z;
      }
    }
  }
  return rc;
}

BOOL
ON_ArcCurve::Transform( const ON_Xform& xform )
{
  TransformUserData(xform);
	DestroyCurveTree();
  return m_arc.Transform( xform );
}

BOOL ON_ArcCurve::IsValid( ON_TextLog* text_log ) const
{
  if ( !m_t.IsIncreasing() )
  {
    if ( 0 != text_log )
      text_log->Print("ON_ArcCurve - m_t=(%g,%g) - it should be an increasing interval.\n",m_t[0],m_t[1]);
    return false;
  }

  if ( !m_arc.IsValid() )
  {
    if ( 0 != text_log )
      text_log->Print("ON_ArcCurve m_arc is not valid\n");
    return false;
  }

  return true;
}

void ON_ArcCurve::Dump( ON_TextLog& dump ) const
{
  dump.Print( "ON_ArcCurve:  domain = [%g,%g]\n",m_t[0],m_t[1]);
  dump.PushIndent();
  dump.Print( "center = ");
  dump.Print( m_arc.plane.origin );
  dump.Print( "\nradius = %g\n",m_arc.radius);
  dump.Print( "length = %g\n",m_arc.Length());
  dump.PopIndent();
}

BOOL ON_ArcCurve::Write(
       ON_BinaryArchive& file // open binary file
     ) const
{
  BOOL rc = file.Write3dmChunkVersion(1,0);
  if (rc)
  {
    rc = file.WriteArc( m_arc );
    if (rc) rc = file.WriteInterval( m_t );
    if (rc) rc = file.WriteInt(m_dim);
  }
  return rc;
}

BOOL ON_ArcCurve::Read(
       ON_BinaryArchive& file // open binary file
     )
{
  int major_version = 0;
  int minor_version = 0;
  BOOL rc = file.Read3dmChunkVersion(&major_version,&minor_version);
  if (rc)
  {
    if (major_version==1)
    {
      // common to all 1.x  versions
      rc = file.ReadArc( m_arc );
      if (rc)
        rc = file.ReadInterval( m_t );
      if (rc)
        rc = file.ReadInt(&m_dim);
      if ( m_dim != 2 && m_dim != 3 )
        m_dim = 3;
    }
    else
      rc = 0;
  }
  return rc;
}


BOOL ON_ArcCurve::SetDomain( double t0, double t1 )
{
  BOOL rc = false;
  if ( t0 < t1 )
  {
    m_t.Set(t0,t1);
    rc = true;
  }
	DestroyCurveTree();
  return rc;
}


bool ON_ArcCurve::ChangeDimension( int desired_dimension )
{
  bool rc = (desired_dimension>=2 && desired_dimension<=3);
  if ( rc && m_dim != desired_dimension )
  {
  	DestroyCurveTree();
    if ( desired_dimension == 2 )
      m_dim = 2;
    else
      m_dim = 3;
  }
  return rc;
}


ON_Interval ON_ArcCurve::Domain() const
{
  return m_t;
}

BOOL ON_ArcCurve::ChangeClosedCurveSeam(
            double t ){
	bool rc = false;
	if( IsCircle() ){
		double angle_delta = m_t.NormalizedParameterAt(t);
		angle_delta*= 2*ON_PI;

		m_arc.Rotate(angle_delta, m_arc.plane.Normal());
		m_t = ON_Interval( t, m_t[1] + t - m_t[0]);
		rc = true;
	}
	return rc;
}


int ON_ArcCurve::SpanCount() const
{
  return 1;
}

BOOL ON_ArcCurve::GetSpanVector( double* s ) const
{
  s[0] = m_t[0];
  s[1] = m_t[1];
  return m_t.IsIncreasing();
}

int ON_ArcCurve::Degree() const
{
  return 2;
}


BOOL
ON_ArcCurve::IsLinear(  // true if curve locus is a line segment
      double // tolerance - formal parameter intentionally ignored in this virtual function
      ) const
{
  return false;
}

BOOL
ON_ArcCurve::IsArc( // true if curve locus in an arc or circle
      const ON_Plane* plane, // if not NULL, test is performed in this plane
      ON_Arc* arc,         // if not NULL and true is returned, then arc
                              // arc parameters are filled in
      double tolerance // tolerance to use when checking linearity
      ) const
{
  BOOL rc = (plane) ? IsInPlane(*plane,tolerance) : true;
  if (arc)
    *arc = m_arc;
  if (rc)
    rc = IsValid();
  return rc;
}

BOOL
ON_ArcCurve::IsPlanar(
      ON_Plane* plane, // if not NULL and true is returned, then plane parameters
                         // are filled in
      double tolerance // tolerance to use when checking linearity
      ) const
{
  if ( m_dim == 2 )
  {
    return ON_Curve::IsPlanar(plane,tolerance);
  }

  if ( plane )
    *plane = m_arc.plane;

  return true;
}

BOOL
ON_ArcCurve::IsInPlane(
      const ON_Plane& plane, // plane to test
      double tolerance // tolerance to use when checking linearity
      ) const
{
  return m_arc.IsInPlane( plane, tolerance );
}

BOOL
ON_ArcCurve::IsClosed() const
{
  return m_arc.IsCircle();
}

BOOL
ON_ArcCurve::IsPeriodic() const
{
  return m_arc.IsCircle();
}

BOOL
ON_ArcCurve::Reverse()
{
  BOOL rc = m_arc.Reverse();
  if (rc)
	{
    m_t.Reverse();
		DestroyCurveTree();
	}
  return true;
}


BOOL ON_ArcCurve::SetStartPoint(ON_3dPoint start_point)
{
  if (IsCircle())
    return false;
  BOOL rc = false;
  if ( m_dim == 3 || start_point.z == 0.0 )
  {
    ON_3dPoint P;
    ON_3dVector T;
    double t = Domain()[1];
    Ev1Der( t, P, T );
    T.Reverse();
    ON_Arc a;
    rc = a.Create( P, T, start_point );
    if ( rc )
    {
      a.Reverse();
      m_arc = a;
    }
    else {
      ON_3dPoint end_point = PointAt(Domain()[1]);
      if (end_point.DistanceTo(start_point) < ON_ZERO_TOLERANCE*m_arc.Radius()){
        //make arc into circle
        m_arc.plane.xaxis = end_point - m_arc.Center();
        m_arc.plane.xaxis.Unitize();
        m_arc.plane.yaxis = ON_CrossProduct(m_arc.Normal(), m_arc.plane.xaxis);
        m_arc.plane.yaxis.Unitize();
        m_arc.SetAngleRadians(2.0*ON_PI);
        rc = true;
      }
    }
  }
	DestroyCurveTree();
  return rc;
}


BOOL ON_ArcCurve::SetEndPoint(ON_3dPoint end_point)
{
  if (IsCircle())
    return false;
  BOOL rc = false;
  if ( m_dim == 3 || end_point.z == 0.0 )
  {
    ON_3dPoint P;
    ON_3dVector T;
    double t = Domain()[0];
    Ev1Der( t, P, T );
    ON_Arc a;
    rc = a.Create( P, T, end_point );
    if ( rc )
    {
      m_arc = a;
    }
    else {
      ON_3dPoint start_point = PointAt(Domain()[0]);
      if (end_point.DistanceTo(start_point) < ON_ZERO_TOLERANCE*m_arc.Radius()){
        //make arc into circle
        m_arc.plane.xaxis = start_point - m_arc.Center();
        m_arc.plane.xaxis.Unitize();
        m_arc.plane.yaxis = ON_CrossProduct(m_arc.Normal(), m_arc.plane.xaxis);
        m_arc.plane.yaxis.Unitize();
        m_arc.SetAngleRadians(2.0*ON_PI);
        rc = true;
      }
    }
  }
	DestroyCurveTree();
  return rc;
}

BOOL ON_ArcCurve::Evaluate( // returns false if unable to evaluate
       double t,       // evaluation parameter
       int der_count,  // number of derivatives (>=0)
       int v_stride,   // v[] array stride (>=Dimension())
       double* v,      // v[] array of length stride*(ndir+1)
       int, // side - formal parameter intentionally ignored in this virtual function
       int* // hint - formal parameter intentionally ignored in this virtual function
       ) const
{
  ON_3dVector d;
  BOOL rc = false;
  if ( m_t[0] < m_t[1] )
  {
    double rat = m_arc.DomainRadians().Length()/m_t.Length();
    double scale = 1.0;
    double a = m_arc.DomainRadians().ParameterAt( m_t.NormalizedParameterAt(t) );
    double c = cos(a)*m_arc.radius;
    double s = sin(a)*m_arc.radius;
    ON_3dPoint p = m_arc.plane.origin + c*m_arc.plane.xaxis + s*m_arc.plane.yaxis;
    v[0] = p.x;
    v[1] = p.y;
    if ( m_dim == 3 )
      v[2] = p.z;
    for ( int di = 1; di <= der_count; di++ ) {
      scale*=rat;
      a =  c;
      c = -s;
      s =  a;
      d =  c*m_arc.plane.xaxis + s*m_arc.plane.yaxis;
      v += v_stride;
      v[0] = d.x*scale;
      v[1] = d.y*scale;
      if ( m_dim == 3 )
        v[2] = d.z*scale;
    }
    rc = true;
  }
  return rc;
}

BOOL ON_ArcCurve::Trim( const ON_Interval& in )
{
  BOOL rc = in.IsIncreasing();
  if (rc) {
    double a = m_arc.AngleRadians();
    double t0 = m_t.NormalizedParameterAt(in.m_t[0]);
    double t1 = m_t.NormalizedParameterAt(in.m_t[1]);
    double a0 = t0*a;
    double a1 = t1*a;
		// Resulting ON_Arc must pass IsValid()
    if ( a1 - a0 > ON_ZERO_TOLERANCE && m_arc.SetAngleRadians(a1-a0) ) {
      if ( a0 != 0 )
        m_arc.Rotate( sin(a0), cos(a0), m_arc.plane.zaxis );
      m_t = in;
    }
    else
      rc = false;
    DestroyCurveTree();
  }
  return rc;
}

bool ON_ArcCurve::Extend(
  const ON_Interval& domain
  )

{
  if (IsClosed()) return false;

  double s0, s1;
  bool changed = false;
  GetDomain(&s0, &s1);
  if (domain[0] < s0){
    s0 = domain[0];
    changed = true;
  }
  if (domain[1] > s1){
    s1 = domain[1];
    changed = true;
  }
  if (!changed) return false;

  DestroyCurveTree();

  double a0 = m_arc.Domain().ParameterAt(Domain().NormalizedParameterAt(s0));
  double a1 = m_arc.Domain().ParameterAt(Domain().NormalizedParameterAt(s1));
  if (a1 > a0+2.0*ON_PI) {
    a1 = a0+2.0*ON_PI;
    s1 = Domain().ParameterAt(m_arc.Domain().NormalizedParameterAt(a1));
  }
  m_arc.Trim(ON_Interval(a0, a1));
  SetDomain(s0, s1);
  return true;
}

BOOL ON_ArcCurve::Split(
    double t,
    ON_Curve*& left_side,
    ON_Curve*& right_side
  ) const
{
  // make sure t is strictly inside the arc's domain
  ON_Interval arc_domain = Domain();
  ON_Interval arc_angles = m_arc.DomainRadians();
  if ( !arc_domain.Includes(t) )
    return false;
  double a = (arc_domain == arc_angles)
           ? t
           : arc_angles.ParameterAt(arc_domain.NormalizedParameterAt(t));
  if ( !arc_angles.Includes(a) )
    return false;

  // make sure input curves are ok.
  ON_ArcCurve* left_arc = 0;
  ON_ArcCurve* right_arc = 0;

  if ( 0 != left_side )
  {
    if ( left_side == right_side )
      return false;
    left_arc = ON_ArcCurve::Cast(left_side);
    if ( 0 == left_arc )
      return false;
    left_arc->DestroyCurveTree();
  }

  if ( 0 != right_side )
  {
    right_arc = ON_ArcCurve::Cast(right_side);
    if ( 0 == right_arc )
      return false;
    right_arc->DestroyCurveTree();
  }

  if ( 0 == left_arc )
  {
    left_arc = new ON_ArcCurve( *this );
  }
  else if ( this != left_arc )
  {
    left_arc->operator=(*this);
  }

  if ( 0 == right_arc )
  {
    right_arc = new ON_ArcCurve( *this );
  }
  else if ( this != right_arc )
  {
    right_arc->operator=(*this);
  }

  BOOL rc = false;
  if ( this != left_arc )
  {
    rc = left_arc->Trim( ON_Interval( arc_domain[0], t ) );
    if (rc)
     rc = right_arc->Trim( ON_Interval( t, arc_domain[1] ) );
  }
  else
  {
    rc = right_arc->Trim( ON_Interval( t, arc_domain[1] ) );
    if (rc)
      rc = left_arc->Trim( ON_Interval( arc_domain[0], t ) );
  }

  if ( rc )
  {
    if ( 0 == left_side )
      left_side = left_arc;
    if ( 0 == right_side )
      right_side = right_arc;
  }
  else
  {
    if ( 0 == left_side && this != left_arc )
    {
      delete left_arc;
      left_arc = 0;
    }
    if ( 0 == right_side && this != right_arc )
    {
      right_arc = 0;
      delete right_arc;
    }
  }
  return rc;
}




static double ArcDeFuzz( double d )
{
  // 0.0078125 = 1.0/128.0 exactly
  // Using 2^n scale factors insures no loss of precision
  // but preserves fractional values that are multiples of 1/128.
  //
  // Fuzz tol should be scale * 2^m * ON_EPSILON for m >= 1

  double f, i;
  f = modf( d*128.0, &i );
  if ( f != 0.0 && fabs(f) <= 1024.0*ON_EPSILON ) {
    d = i*0.0078125;
  }
  return d;
}

static BOOL NurbsCurveArc ( const ON_Arc& arc, int dim, ON_NurbsCurve& nurb )
{
  if ( !arc.IsValid() )
    return false;
  // makes a quadratic nurbs arc
  const ON_3dPoint center = arc.Center();
  double angle = arc.AngleRadians();
  ON_Interval dom = arc.DomainRadians();
  const double angle0 = dom[0];
  const double angle1 = dom[1];
  ON_3dPoint start_point = arc.StartPoint();
  //ON_3dPoint mid_point   = arc.PointAt(angle0 + 0.5*angle);
  ON_3dPoint end_point   = arc.EndPoint();

  ON_4dPoint CV[9];
  double knot[10];

	double a, b, c, w, winv;
	double *cv;
	int    j, span_count, cv_count;

	a = (0.5 + ON_SQRT_EPSILON)*ON_PI;

	if (angle <= a)
		span_count = 1;
	else if (angle <= 2.0*a)
		span_count = 2;
	else if (angle <= 3.0*a)
		span_count = 4; // TODO - make a 3 span case
	else
		span_count = 4;

	cv_count = 2*span_count + 1;

	switch(span_count) {
	case 1:
    CV[0] = start_point;
    CV[1] = arc.PointAt(angle0 + 0.50*angle);
    CV[2] = end_point;
		break;
	case 2:
    CV[0] = start_point;
    CV[1] = arc.PointAt(angle0 + 0.25*angle);
    CV[2] = arc.PointAt(angle0 + 0.50*angle);
    CV[3] = arc.PointAt(angle0 + 0.75*angle);
    CV[4] = end_point;
		angle *= 0.5;
		break;
	default: // 4 spans
    CV[0] = start_point;
    CV[1] = arc.PointAt(angle0 + 0.125*angle);
    CV[2] = arc.PointAt(angle0 + 0.250*angle);
    CV[3] = arc.PointAt(angle0 + 0.375*angle);
    CV[4] = arc.PointAt(angle0 + 0.500*angle);
    CV[5] = arc.PointAt(angle0 + 0.625*angle);
    CV[6] = arc.PointAt(angle0 + 0.750*angle);
    CV[7] = arc.PointAt(angle0 + 0.875*angle);
    CV[8] = end_point;
		angle *= 0.25;
		break;
	}

	a = cos(0.5*angle);
	b = a - 1.0;
	//c = (radius > 0.0) ? radius*angle : angle;
  c = angle;

	span_count *= 2;
	knot[0] = knot[1] = angle0; //0.0;
	for (j = 1; j < span_count; j += 2) {
    CV[j].x += b * center.x;
    CV[j].y += b * center.y;
    CV[j].z += b * center.z;
    CV[j].w = a;
		CV[j+1].w = 1.0;
		knot[j+1] = knot[j+2] = knot[j-1] + c;
	}
  knot[cv_count-1] = knot[cv_count] = angle1;
  for ( j = 1; j < span_count; j += 2 ) {
    w = CV[j].w;
    winv = 1.0/w;
    a = CV[j].x*winv;
    b = ArcDeFuzz(a);
    if ( a != b ) {
      CV[j].x = b*w;
    }
    a = CV[j].y*winv;
    b = ArcDeFuzz(a);
    if ( a != b ) {
      CV[j].y = b*w;
    }
    a = CV[j].z*winv;
    b = ArcDeFuzz(a);
    if ( a != b ) {
      CV[j].z = b*w;
    }
  }

  nurb.m_dim = (dim==2) ? 2 : 3;
  nurb.m_is_rat = 1;
  nurb.m_order = 3;
  nurb.m_cv_count = cv_count;
  nurb.m_cv_stride = (dim==2 ? 3 : 4);
  nurb.ReserveCVCapacity( nurb.m_cv_stride*cv_count );
  nurb.ReserveKnotCapacity( cv_count+1 );
  for ( j = 0; j < cv_count; j++ ) {
    cv = nurb.CV(j);
    cv[0] = CV[j].x;
    cv[1] = CV[j].y;
    if ( dim == 2 ) {
      cv[2] = CV[j].w;
    }
    else {
      cv[2] = CV[j].z;
      cv[3] = CV[j].w;
    }
    nurb.m_knot[j] = knot[j];
  }
  nurb.m_knot[cv_count] = knot[cv_count];
  return true;
}


int ON_Arc::GetNurbForm( ON_NurbsCurve& nurbscurve ) const

{
  BOOL rc = NurbsCurveArc ( *this, 3, nurbscurve );
  return (rc) ? 2 : 0;
}

bool ON_Arc::GetRadianFromNurbFormParameter(double NurbParameter, double* RadianParameter  ) const
{
  bool rc = false;
  ON_NurbsCurve nurbscurve;
  ON_4dPoint CV[9];
  double knot[10];
  nurbscurve.m_cv = &CV[0].x;
  nurbscurve.m_knot = knot;
  if (GetNurbForm( nurbscurve ))
  {
    ON_3dPoint p = nurbscurve.PointAt(NurbParameter);
    rc = ClosestPointTo(p,RadianParameter);
    if (IsCircle()){
      if (NurbParameter - DomainRadians()[0] < ON_ZERO_TOLERANCE &&
        DomainRadians()[1] - *RadianParameter < ON_ZERO_TOLERANCE) *RadianParameter = DomainRadians()[0];
      else if (DomainRadians()[1] - NurbParameter < ON_ZERO_TOLERANCE &&
        *RadianParameter - DomainRadians()[0] < ON_ZERO_TOLERANCE) *RadianParameter = DomainRadians()[1];
    }
  }
  return rc;


  /* bug in here somewhere
//returns false unless
//   0<= NurbParameter,  <= AngleRadians()*Radius()
	ON_NurbsCurve crv;

	if( !IsValid()|| RadianParameter==NULL)
		return false;

	ON_Interval NDomain(0, 0);	//nurb domain
	if( Radius()>0)
		NDomain[1] = Radius() * AngleRadians();

	ON_Interval ADomain = DomainRadians();
	if( NurbParameter==0){
		*RadianParameter=ADomain[0];
		return true;
	} else if(  fabs(NurbParameter- NDomain[1])<ON_EPSILON*NDomain[1]){
		*RadianParameter=ADomain[1];
		return true;
	}

	if( !NDomain.Includes(NurbParameter) )
		return false;

	if( !GetNurbForm(crv))
		return false;

	ON_3dPoint cp;
	cp = crv.PointAt(NurbParameter);
	cp -= Center();

	double x = ON_DotProduct(Plane().Xaxis(), cp);
	double y = ON_DotProduct(Plane().Yaxis(), cp);
	double theta = atan2(y,x);
	theta -= floor( (theta-ADomain[0])/(2*ON_PI)) * 2* ON_PI;
	*RadianParameter = theta;

#if defined(ON_DEBUG)
	double np2;
	GetNurbFormParameterFromRadian( *RadianParameter, &np2);
	ON_ASSERT(fabs(np2-NurbParameter)<=ON_EPSILON*( NurbParameter + 1.0) );
#endif

	return true;
  */
}


bool ON_Arc::GetNurbFormParameterFromRadian(double RadianParameter, double* NurbParameter ) const
{
	if(!IsValid() || NurbParameter==NULL)
		return false;

  ON_Interval ADomain = DomainRadians();

  double endtol = 10.0*ON_EPSILON*(fabs(ADomain[0]) + fabs(ADomain[1]));

  double del = RadianParameter - ADomain[0];
	if(del <= endtol && del >= -ON_SQRT_EPSILON)
  {
		*NurbParameter=ADomain[0];
		return true;
	}
  else {
    del = ADomain[1] - RadianParameter;
    if(del <= endtol && del >= -ON_SQRT_EPSILON){
		  *NurbParameter=ADomain[1];
		  return true;
    }
	}

	if( !ADomain.Includes(RadianParameter ) )
		return false;


	ON_NurbsCurve crv;

	if( !GetNurbForm(crv))
		return false;

	//Isolate a bezier that contains the solution
	int cnt = crv.SpanCount();
	int si =0;	//get span index
	int ki=0;		//knot index
	double ang = ADomain[0];
	ON_3dPoint cp;
	cp = crv.PointAt( crv.Knot(0) ) - Center();
	double x = ON_DotProduct(Plane().Xaxis(),cp);
	double y = ON_DotProduct(Plane().Yaxis(),cp);
	double at = atan2( y, x);	//todo make sure we dont go to far

	for( si=0, ki=0; si<cnt; si++, ki+=crv.KnotMultiplicity(ki) ){
		cp = crv.PointAt( crv.Knot(ki+2)) - Center();
		x = ON_DotProduct(Plane().Xaxis(),cp);
		y = ON_DotProduct(Plane().Yaxis(),cp);
		double at2 = atan2(y,x);
		if(at2>at)
			ang+=(at2-at);
		else
			ang += (2*ON_PI + at2 - at);
		at = at2;
		if( ang>RadianParameter)
			break;
	}

	ON_Interval BezDomain(crv.Knot(ki), crv.Knot(ki+2));

	ON_BezierCurve bez;
	if(!crv.ConvertSpanToBezier(ki,bez))
		return false;

 	ON_Xform COC;
	COC.ChangeBasis( ON_Plane(),Plane());


	bez.Transform(COC);	// change coordinates to circles local frame
	double a[3];							// Bez coefficients of a quadratic to solve
	for(int i=0; i<3; i++)
		a[i] = tan(RadianParameter)* bez.CV(i)[0] - bez.CV(i)[1];

	//Solve the Quadratic
	double descrim = (a[1]*a[1]) - a[0]*a[2];
	double squared = a[0]-2*a[1]+a[2];
	double tbez;
	if(fabs(squared)> ON_ZERO_TOLERANCE){
		ON_ASSERT(descrim>=0);
		descrim = sqrt(descrim);
		tbez = (a[0]-a[1] + descrim)/(a[0]-2*a[1]+a[2]);
		if( tbez<0 || tbez>1){
			double tbez2 = (a[0]-a[1]-descrim)/(a[0] - 2*a[1] + a[2]);
			if( fabs(tbez2 - .5)<fabs(tbez-.5) )
				tbez = tbez2;
		}

		ON_ASSERT(tbez>=-ON_ZERO_TOLERANCE && tbez<=1+ON_ZERO_TOLERANCE);
	}
	else{
		// Quadratic degenerates to linear
		tbez = 1.0;
		if(a[0]-a[2])
			tbez = a[0]/(a[0]-a[2]);
	}
	if(tbez<0)
		tbez=0.0;
	else if(tbez>1.0)
		tbez=1.0;


		//Debug ONLY Code  - check the result
//		double aa = a[0]*(1-tbez)*(1-tbez)  + 2*a[1]*tbez*(1-tbez) + a[2]*tbez*tbez;
//		double tantheta= tan(RadianParameter);
//		ON_3dPoint bezp;
//		bez.Evaluate(tbez, 0, 3, bezp);
//		double yx = bezp.y/bezp.x;


	*NurbParameter = BezDomain.ParameterAt(tbez);
	return true;

}

bool ON_ArcCurve::GetClosestPoint( const ON_3dPoint& test_point,
        double* t,       // parameter of local closest point returned here
        double maximum_distance,
        const ON_Interval* sub_domain
        ) const
{
  double a, s, d;
  ON_Interval domain = Domain();
  if (sub_domain)
  {
    if ( !sub_domain->IsIncreasing() )
      return false;
    domain.Intersection(*sub_domain);
    if ( !domain.IsIncreasing() )
      return false;
  }


  bool rc = m_arc.ClosestPointTo( test_point, &a );
  if ( rc )
  {
    s = m_t.ParameterAt( m_arc.DomainRadians().NormalizedParameterAt(a) );
    if ( sub_domain )
    {
      if ( s < sub_domain->Min() || s > sub_domain->Max())
      {
        double dist0 = test_point.DistanceTo(PointAt(domain[0]));
        double dist1 = test_point.DistanceTo(PointAt(domain[1]));
        s = domain[(dist0 <= dist1)?0:1];
      }
    }
    if ( maximum_distance > 0.0 )
    {
      d = test_point.DistanceTo(PointAt(s));
      if ( d > maximum_distance )
        rc = false;
    }
    if (rc && t)
      *t = s;
  }
  return rc;
}

BOOL ON_ArcCurve::GetLength(
        double* length,               // length returned here
        double, // fractional_tolerance - formal parameter intentionally ignored in this virtual function
        const ON_Interval* sub_domain // default = NULL
        ) const
{
	if( sub_domain && sub_domain->IsDecreasing() )
		return false;
  else if ( sub_domain ) {
		ON_Interval scratch_domain = m_t;
		if( !scratch_domain.Intersection(*sub_domain))
			return false;
		else
			sub_domain=&scratch_domain;
    double a0 = m_arc.DomainRadians().ParameterAt(m_t.NormalizedParameterAt(sub_domain->Min()));
    double a1 = m_arc.DomainRadians().ParameterAt(m_t.NormalizedParameterAt(sub_domain->Max()));
    *length = fabs((a1-a0)*m_arc.radius);
  }
  else {
    *length = m_arc.Length();
  }
  return true;
}

BOOL ON_ArcCurve::GetNormalizedArcLengthPoint(
        double s,
        double* t,
        double, // fractional_tolerance - formal parameter intentionally ignored in this virtual function
        const ON_Interval* sub_domain
        ) const
{
  ON_Interval domain = (sub_domain) ? *sub_domain : Domain();
  *t = domain.ParameterAt(s);
  return true;
}

BOOL ON_ArcCurve::GetNormalizedArcLengthPoints(
        int count,
        const double* s,
        double* t,
        double, // absolute_tolerance   - formal parameter intentionally ignored in this virtual function
        double, // fractional_tolerance - formal parameter intentionally ignored in this virtual function
        const ON_Interval* sub_domain
        ) const
{
  if ( count > 0 || s != NULL && t != NULL )
  {
    if ( !sub_domain )
      sub_domain = &m_t;
    int i;
    for ( i = 0; i < count; i++ )
    {
      t[i] = sub_domain->ParameterAt( s[i] );
    }
  }
  return true;
}

BOOL ON_ArcCurve::GetLocalClosestPoint( const ON_3dPoint& test_point,
        double seed_parameter,
        double* t,
        const ON_Interval* sub_domain
        ) const
{
  if (!GetClosestPoint( test_point, t, 0.0, sub_domain ))
    return false;

  if (IsCircle() && (!sub_domain || sub_domain->Includes(Domain()))){
    //if closest point is near seam, use seed do determine which side.
    if (seed_parameter < Domain().ParameterAt(0.01) && *t > Domain().ParameterAt(0.99))
      *t = Domain()[0];
    else if (seed_parameter > Domain().ParameterAt(0.99) && *t < Domain().ParameterAt(0.01))
      *t = Domain()[1];
  }

  return true;

}


int ON_ArcCurve::GetNurbForm( // returns 0: unable to create NURBS representation
                 //            with desired accuracy.
                 //         1: success - returned NURBS parameterization
                 //            matches the curve's to wthe desired accuracy
                 //         2: success - returned NURBS point locus matches
                 //            the curve's to the desired accuracy but, on
                 //            the interior of the curve's domain, the
                 //            curve's parameterization and the NURBS
                 //            parameterization may not match to the
                 //            desired accuracy.
      ON_NurbsCurve& c,
      double tolerance,
      const ON_Interval* subdomain  // OPTIONAL subdomain of arc
      ) const
{
  int rc = 0;
  if ( subdomain )
  {
    ON_ArcCurve trimmed_arc(*this);
    if ( trimmed_arc.Trim(*subdomain) )
    {
      rc = trimmed_arc.GetNurbForm( c, tolerance, NULL );
    }
  }
  else if ( m_t.IsIncreasing() && m_arc.IsValid() )
  {
    if ( NurbsCurveArc( m_arc, m_dim, c ) )
    {
      rc = 2;
      c.SetDomain( m_t[0], m_t[1] );
    }
  }
  return rc;
}

int ON_ArcCurve::HasNurbForm( // returns 0: unable to create NURBS representation
                 //            with desired accuracy.
                 //         1: success - returned NURBS parameterization
                 //            matches the curve's to wthe desired accuracy
                 //         2: success - returned NURBS point locus matches
                 //            the curve's to the desired accuracy but, on
                 //            the interior of the curve's domain, the
                 //            curve's parameterization and the NURBS
                 //            parameterization may not match to the
                 //            desired accuracy.
                 ) const

{
  if (!IsValid())
    return 0;
  return 2;
}

BOOL ON_ArcCurve::GetCurveParameterFromNurbFormParameter(
      double nurbs_t,
      double* curve_t
      ) const
{
  double radians;

  double arcnurb_t = m_arc.DomainRadians().ParameterAt(m_t.NormalizedParameterAt(nurbs_t));

  BOOL rc = m_arc.GetRadianFromNurbFormParameter(arcnurb_t,&radians);
  *curve_t = m_t.ParameterAt( m_arc.DomainRadians().NormalizedParameterAt(radians) );

  return rc;
}

BOOL ON_ArcCurve::GetNurbFormParameterFromCurveParameter(
      double curve_t,
      double* nurbs_t
      ) const
{
  double radians = m_arc.DomainRadians().ParameterAt(m_t.NormalizedParameterAt(curve_t));
  double arcnurb_t;
  BOOL rc = m_arc.GetNurbFormParameterFromRadian(radians,&arcnurb_t);
  *nurbs_t = m_t.ParameterAt(m_arc.DomainRadians().NormalizedParameterAt(arcnurb_t));
  return rc;
}

bool ON_ArcCurve::IsCircle() const
{
  return m_arc.IsCircle() ? true : false;
}

double ON_ArcCurve::Radius() const
{
	return m_arc.Radius();
}

double ON_ArcCurve::AngleRadians() const
{
	return m_arc.AngleRadians();
}

double ON_ArcCurve::AngleDegrees() const
{
	return m_arc.AngleDegrees();
}

/*
Description:
  ON_CircleCurve is obsolete.
  This code exists so v2 files can be read.
*/
class ON__OBSOLETE__CircleCurve : public ON_ArcCurve
{
public:
  static const ON_ClassId m_ON_CircleCurve_class_id;
  const ON_ClassId* ClassId() const;
  BOOL Read(
         ON_BinaryArchive&  // open binary file
       );
};

static ON_Object* CreateNewON_CircleCurve()
{

  // must create an ON_CircleCurve so virtual
  // ON_CircleCurve::Read will be used to read
  // archive objects with uuid CF33BE29-09B4-11d4-BFFB-0010830122F0
  return new ON__OBSOLETE__CircleCurve();
}

const ON_ClassId ON__OBSOLETE__CircleCurve::m_ON_CircleCurve_class_id("ON__OBSOLETE__CircleCurve",
                                                           "ON_ArcCurve",
                                                           CreateNewON_CircleCurve,
                                                           "CF33BE29-09B4-11d4-BFFB-0010830122F0");

const ON_ClassId* ON__OBSOLETE__CircleCurve::ClassId() const
{
  // so write will save ON_ArcCurve uuid
  return &ON_ArcCurve::m_ON_ArcCurve_class_id;
}

BOOL ON__OBSOLETE__CircleCurve::Read(
       ON_BinaryArchive& file // open binary file
     )
{
  int major_version = 0;
  int minor_version = 0;
  BOOL rc = file.Read3dmChunkVersion(&major_version,&minor_version);
  if (rc)
  {
    if (major_version==1)
    {
      // common to all 1.x versions
      ON_Circle circle;
      rc = file.ReadCircle( circle );
      m_arc = circle;
      if (rc)
        rc = file.ReadInterval( m_t );
      if (rc)
        rc = file.ReadInt(&m_dim);
      if ( m_dim != 2 && m_dim != 3 )
        m_dim = 3;
    }
  }

  return rc;
}
