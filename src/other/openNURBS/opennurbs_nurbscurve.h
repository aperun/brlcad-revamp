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

////////////////////////////////////////////////////////////////
//
//   Definition of NURBS curve
//
////////////////////////////////////////////////////////////////

#if !defined(OPENNURBS_NURBSCURVE_INC_)
#define OPENNURBS_NURBSCURVE_INC_

class ON_NurbsCurve;
class ON_CLASS ON_NurbsCurve : public ON_Curve
{
  ON_OBJECT_DECLARE(ON_NurbsCurve);

public:
  /*
  Description:
    Use ON_NurbsCurve::New(...) instead of new ON_NurbsCurve(...)
  Returns:
    Pointer to an ON_NurbsCurve.  Destroy by calling delete.
  Remarks:
    See static ON_Brep* ON_Brep::New() for details.
  */
  static ON_NurbsCurve* New();
  static ON_NurbsCurve* New(
            const ON_NurbsCurve& nurbs_curve 
            );
  static ON_NurbsCurve* New(
            const ON_BezierCurve& bezier_curve 
            );
  static ON_NurbsCurve* New(
            int dimension,
            BOOL bIsRational,
            int order,
            int cv_count
            );

  ON_NurbsCurve();
  ON_NurbsCurve(const ON_NurbsCurve&);

  // Description:
  //   Create a NURBS curve equal to bezier with domain [0,1].
  // Parameters:
  //   bezier_curve - [in]
  ON_NurbsCurve(
        const ON_BezierCurve& bezier_curve
        );

  // Description:
  //   Create a NURBS curve with knot a cv memory allocated.
  // Parameters:
  //   dimension - [in] (>= 1)
  //   bIsRational - [in] TRUE to make a rational NURBS
  //   order - [in] (>= 2) The order=degree+1
  //   cv_count - [in] (>= order) number of control vertices
  ON_NurbsCurve(
          int dimension,
          BOOL bIsRational,
          int order,
          int cv_count
          );

  // virtual ON_Object::SizeOf override
  unsigned int SizeOf() const;

  // virtual ON_Object::DataCRC override
  ON__UINT32 DataCRC(ON__UINT32 current_remainder) const;

  /*
  Description:
    See if this and other are same NURBS geometry.
  Parameters:
    other - [in] other NURBS curve
    bIgnoreParameterization - [in] if true, parameterization
             and orientaion are ignored.
    tolerance - [in] tolerance to use when comparing
                     control points.
  Returns:
    true if curves are tne same.
  */
  bool IsDuplicate( 
          const ON_NurbsCurve& other, 
          bool bIgnoreParameterization,
          double tolerance = ON_ZERO_TOLERANCE 
          ) const;

  // Description:
  //   Zeros all fields.
  void Initialize(void);

  // Description:
  //   Create a NURBS curve with knot a cv memory allocated.
  // Parameters:
  //   dimension - [in] (>= 1)
  //   bIsRational - [in] TRUE to make a rational NURBS
  //   order - [in] (>= 2) The order=degree+1
  //   cv_count - [in] (>= order) number of control vertices
  bool Create( 
          int dimension,
          BOOL bIsRational,
          int order,
          int cv_count
          );

  // Description:
  //   Create a clamped uniform NURBS curve from a list
  //   of control points
  // Parameters:
  //   dimension - [in] 1, 2 or 3
  //   order - [in] (>=2) order=degree+1
  //   point_count - [in] (>=order) number of control vertices
  //   point - [in] array of control vertex locations.
  //   knot_delta - [in] (>0.0) knot spacing
  // Returns:
  //   TRUE if successful
  bool CreateClampedUniformNurbs( 
          int dimension,
          int order,
          int point_count,
          const ON_3dPoint* point,
          double knot_delta = 1.0
          );

  // Description:
  //   Create a periodic uniform NURBS curve from a list
  //   of control points
  // Parameters:
  //   dimension - [in] 1, 2 or 3
  //   order - [in] (>=2) order=degree+1
  //   point_count - [in] (>=max(3,order-1)) number of distinct control vertices
  //   point - [in] array of distinct control vertex locations.
  //   knot_delta - [in] (>0.0) knot spacing
  // Returns:
  //   TRUE if successful
  bool CreatePeriodicUniformNurbs( 
          int dimension,
          int order,
          int point_count,
          const ON_3dPoint* point,
          double knot_delta = 1.0
          );

  // Description:
  //   Deallocate knot and cv memory.  Zeros all fields.
  void Destroy();

  virtual ~ON_NurbsCurve();

  // Description:
  //   Call if memory used by ON_NurbsCurve becomes invalid.
  void EmergencyDestroy(); 

	ON_NurbsCurve& operator=(const ON_NurbsCurve& src);

  // Description:
  //   Set NURBS curve equal to bezier with domain [0,1].
  // Parameters:
  //   bezier_curve - [in]
	ON_NurbsCurve& operator=(
    const ON_BezierCurve& bezier_curve
    );

  /////////////////////////////////////////////////////////////////
  // ON_Object overrides

  /*
  Description:
    Tests an object to see if its data members are correctly
    initialized.
  Parameters:
    text_log - [in] if the object is not valid and text_log
        is not NULL, then a brief englis description of the
        reason the object is not valid is appened to the log.
        The information appended to text_log is suitable for 
        low-level debugging purposes by programmers and is 
        not intended to be useful as a high level user 
        interface tool.
  Returns:
    @untitled table
    TRUE     object is valid
    FALSE    object is invalid, uninitialized, etc.
  Remarks:
    Overrides virtual ON_Object::IsValid
  */
  BOOL IsValid( ON_TextLog* text_log = NULL ) const;

  // Description:
  //   virtual ON_Object::Dump override
  void Dump( 
    ON_TextLog& dump
    ) const;

  // Description:
  //   virtual ON_Object::Write override
  BOOL Write(
         ON_BinaryArchive& binary_archive
       ) const;

  // Description:
  //   virtual ON_Object::Read override
  BOOL Read(
         ON_BinaryArchive& binary_archive
       );

  /////////////////////////////////////////////////////////////////
  // ON_Geometry overrides

  // Description:
  //   virtual ON_Geometry::Dimension override
  // Returns:
  //   value of m_dim
  int Dimension() const;

  // Description:
  //   virtual ON_Geometry::GetBBox override
  //   Calculates axis aligned bounding box.
  // Parameters:
  //   boxmin - [in/out] array of Dimension() doubles
  //   boxmax - [in/out] array of Dimension() doubles
  //   bGrowBox - [in] (default=FALSE) 
  //     If TRUE, then the union of the input bbox and the 
  //     object's bounding box is returned in bbox.  
  //     If FALSE, the object's bounding box is returned in bbox.
  // Returns:
  //   TRUE if object has bounding box and calculation was successful
  BOOL GetBBox( // returns TRUE if successful
         double* boxmin,
         double* boxmax,
         int bGrowBox = false
         ) const;

  // Description:
  //   virtual ON_Geometry::Transform override.
  //   Transforms the NURBS curve.
  //
  // Parameters:
  //   xform - [in] transformation to apply to object.
  //
  // Remarks:
  //   When overriding this function, be sure to include a call
  //   to ON_Object::TransformUserData() which takes care of 
  //   transforming any ON_UserData that may be attached to 
  //   the object.
  BOOL Transform( 
         const ON_Xform& xform
         );

  // virtual ON_Geometry::IsDeformable() override
  bool IsDeformable() const;

  // virtual ON_Geometry::MakeDeformable() override
  bool MakeDeformable();

  // Description:
  //   virtual ON_Geometry::SwapCoordinates override.
  //   Swaps control vertex coordinate values with indices i and j.
  // Parameters:
  //   i - [in] coordinate index
  //   j - [in] coordinate index
  BOOL SwapCoordinates(
        int i, 
        int j
        );

  // virtual ON_Geometry override
  bool Morph( const ON_SpaceMorph& morph );

  // virtual ON_Geometry override
  bool IsMorphable() const;

  /////////////////////////////////////////////////////////////////
  // ON_Curve overrides

  // Description:
  //   virtual ON_Curve::Domain override.
  // Returns:
  //   domain of the NURBS curve.
  ON_Interval Domain() const;

  // Description:
  //   virtual ON_Curve::SetDomain override.
  //   Set the domain of the curve
  // Parameters:
  //   t0 - [in]
  //   t1 - [in] new domain will be [t0,t1]
  // Returns:
  //   TRUE if successful.
  BOOL SetDomain(
        double t0, 
        double t1 
        );

  /*
  Description:
    If this curve is closed, then modify it so that
    the start/end point is at curve parameter t.
  Parameters:
    t - [in] curve parameter of new start/end point.  The
             returned curves domain will start at t.
  Returns:
    TRUE if successful.
  Remarks:
    Overrides virtual ON_Curve::ChangeClosedCurveSeam
  */
  BOOL ChangeClosedCurveSeam( 
            double t 
            );

  // Description:
  //   virtual ON_Curve::SpanCount override.
  //   Get number of nonempty smooth (c-infinity) spans in curve
  // Returns:
  //   Number of nonempty smooth (c-infinity) spans.
  // Remarks:
  //   A nonempty span is bracked by knots m_knot[i] < m_knot[i+1]
  //   with m_order-2 <= i < m_cv_count-1.
  int SpanCount() const;

  // Description:
  //   virtual ON_Curve::GetSpanVector override.
  //   Get number of parameters of distinct knots in NURBS curve's domain.
  // Parameters:
  //   knot_values - [out] an array of length SpanCount()+1 is 
  //       filled in with the distinct knot values in the list
  ///      (m_knot[m_order-2],...,m_knot[m_cv_count-1)
  // Returns:
  //   TRUE if successful
  BOOL GetSpanVector(
         double* knot_values
         ) const; // 

  // Description:
  //   virtual ON_Curve::Degree override.
  // Returns:
  //   m_order-1
  int Degree() const; 

  // Description:
  //   virtual ON_Curve::GetParameterTolerance override.
  BOOL GetParameterTolerance( // returns tminus < tplus: parameters tminus <= s <= tplus
         double t,
         double* tminus,
         double* tplus
         ) const;

  // Description:
  //   virtual ON_Curve::IsLinear override.
  BOOL IsLinear(
        double tolerance = ON_ZERO_TOLERANCE
        ) const;

  /*
  Description:
    Several types of ON_Curve can have the form of a polyline including
    a degree 1 ON_NurbsCurve, an ON_PolylineCurve, and an ON_PolyCurve
    all of whose segments are some form of polyline.  IsPolyline tests
    a curve to see if it can be represented as a polyline.
  Parameters:
    pline_points - [out] if not NULL and TRUE is returned, then the
        points of the polyline form are returned here.
    t - [out] if not NULL and TRUE is returned, then the parameters of
        the polyline points are returned here.
  Returns:
    @untitled table
    0        curve is not some form of a polyline
    >=2      number of points in polyline form
  */
  int IsPolyline(
        ON_SimpleArray<ON_3dPoint>* pline_points = NULL,
        ON_SimpleArray<double>* pline_t = NULL
        ) const;

  // Description:
  //   virtual ON_Curve::IsArc override.
  BOOL IsArc(
        const ON_Plane* plane = NULL,
        ON_Arc* arc = NULL,
        double tolerance = ON_ZERO_TOLERANCE
        ) const;

  // Description:
  //   virtual ON_Curve::IsPlanar override.
  BOOL IsPlanar(
        ON_Plane* plane = NULL,
        double tolerance = ON_ZERO_TOLERANCE
        ) const;

  // Description:
  //   virtual ON_Curve::IsInPlane override.
  BOOL IsInPlane(
        const ON_Plane& test_plane,
        double tolerance = ON_ZERO_TOLERANCE
        ) const;

  // Description:
  //   virtual ON_Curve::IsClosed override.
  // Returns:
  //   TRUE if NURBS curve is closed. (Either curve has
  //   clamped end knots and euclidean location of start
  //   CV = euclidean location of end CV, or curve is
  //   periodic.)
  BOOL IsClosed() const;

  // Description:
  //   virtual ON_Curve::IsPeriodic override.
  // Returns:
  //   TRUE if NURBS curve is periodic (degree > 1,
  //   periodic knot vector, last degree many CVs 
  //   are duplicates of first degree many CVs).
  BOOL IsPeriodic() const;
  
  /*
  Description:
    Search for a derivatitive, tangent, or curvature discontinuity.
  Parameters:
    c - [in] type of continity to test for.  If ON::C1_continuous
    t0 - [in] search begins at t0
    t1 - [in] (t0 < t1) search ends at t1
    t - [out] if a discontinuity is found, the *t reports the
          parameter at the discontinuity.
    hint - [in/out] if GetNextDiscontinuity will be called repeatedly,
       passing a "hint" with initial value *hint=0 will increase the speed
       of the search.       
    dtype - [out] if not NULL, *dtype reports the kind of discontinuity
        found at *t.  A value of 1 means the first derivative or unit tangent
        was discontinuous.  A value of 2 means the second derivative or
        curvature was discontinuous.
    cos_angle_tolerance - [in] default = cos(1 degree) Used only when
        c is ON::G1_continuous or ON::G2_continuous.  If the cosine
        of the angle between two tangent vectors 
        is <= cos_angle_tolerance, then a G1 discontinuity is reported.
    curvature_tolerance - [in] (default = ON_SQRT_EPSILON) Used only when
        c is ON::G2_continuous.  If K0 and K1 are curvatures evaluated
        from above and below and |K0 - K1| > curvature_tolerance,
        then a curvature discontinuity is reported.
  Returns:
    TRUE if a discontinuity was found on the interior of the interval (t0,t1).
  Remarks:
    Overrides ON_Curve::GetNextDiscontinuity.
  */
  bool GetNextDiscontinuity( 
                  ON::continuity c,
                  double t0,
                  double t1,
                  double* t,
                  int* hint=NULL,
                  int* dtype=NULL,
                  double cos_angle_tolerance=0.99984769515639123915701155881391,
                  double curvature_tolerance=ON_SQRT_EPSILON
                  ) const;

  /*
  Description:
    Test continuity at a curve parameter value.
  Parameters:
    c - [in] continuity to test for
    t - [in] parameter to test
    hint - [in] evaluation hint
    point_tolerance - [in] if the distance between two points is
        greater than point_tolerance, then the curve is not C0.
    d1_tolerance - [in] if the difference between two first derivatives is
        greater than d1_tolerance, then the curve is not C1.
    d2_tolerance - [in] if the difference between two second derivatives is
        greater than d2_tolerance, then the curve is not C2.
    cos_angle_tolerance - [in] default = cos(1 degree) Used only when
        c is ON::G1_continuous or ON::G2_continuous.  If the cosine
        of the angle between two tangent vectors 
        is <= cos_angle_tolerance, then a G1 discontinuity is reported.
    curvature_tolerance - [in] (default = ON_SQRT_EPSILON) Used only when
        c is ON::G2_continuous.  If K0 and K1 are curvatures evaluated
        from above and below and |K0 - K1| > curvature_tolerance,
        then a curvature discontinuity is reported.
  Returns:
    TRUE if the curve has at least the c type continuity at the parameter t.
  Remarks:
    Overrides ON_Curve::IsContinuous.
  */
  bool IsContinuous(
    ON::continuity c,
    double t, 
    int* hint = NULL,
    double point_tolerance=ON_ZERO_TOLERANCE,
    double d1_tolerance=ON_ZERO_TOLERANCE,
    double d2_tolerance=ON_ZERO_TOLERANCE,
    double cos_angle_tolerance=0.99984769515639123915701155881391,
    double curvature_tolerance=ON_SQRT_EPSILON
    ) const;

  /*
  Description:
    Force the curve to start at a specified point.
  Parameters:
    start_point - [in]
  Returns:
    TRUE if successful.
  Remarks:
    Some end points cannot be moved.  Be sure to check return
    code.
  See Also:
    ON_Curve::SetEndPoint
    ON_Curve::PointAtStart
    ON_Curve::PointAtEnd
  */
  //virtual
  BOOL SetStartPoint(
          ON_3dPoint start_point
          );

  /*
  Description:
    Force the curve to end at a specified point.
  Parameters:
    end_point - [in]
  Returns:
    TRUE if successful.
  Remarks:
    Some end points cannot be moved.  Be sure to check return
    code.
  See Also:
    ON_Curve::SetStartPoint
    ON_Curve::PointAtStart
    ON_Curve::PointAtEnd
  */
  //virtual
  BOOL SetEndPoint(
          ON_3dPoint end_point
          );

  // Description:
  //   virtual ON_Curve::Reverse override.
  //   Reverse parameterizatrion by negating all knots
  //   and reversing the order of the control vertices.
  // Remarks:
  //   Domain changes from [a,b] to [-b,-a]
  BOOL Reverse();       

  // Description:
  //   virtual ON_Curve::Evaluate override.
  BOOL Evaluate( // returns FALSE if unable to evaluate
         double,         // evaluation parameter
         int,            // number of derivatives (>=0)
         int,            // array stride (>=Dimension())
         double*,        // array of length stride*(ndir+1)
         int = 0,        // optional - determines which side to evaluate from
                         //         0 = default
                         //      <  0 to evaluate from below, 
                         //      >  0 to evaluate from above
         int* = 0        // optional - evaluation hint (int) used to speed
                         //            repeated evaluations
         ) const;

  bool GetClosestPoint( 
          const ON_3dPoint&, // test_point
          double* t,       // parameter of local closest point returned here
          double maximum_distance = 0.0,  // maximum_distance
          const ON_Interval* sub_domain = NULL // sub_domain
          ) const;

  // Description:
  //   Get the length of the curve.
  // Parameters:
  //   length - [out] length returned here.
  //   t - [out] parameter such that the length of the curve
  //      from its start to t is arc_length.
  //   fractional_tolerance - [in] desired fractional precision.
  //       fabs(("exact" length from start to t) - arc_length)/arc_length <= fractional_tolerance
  //   sub_domain - [in] If not NULL, the calculation is performed on
  //       the specified sub-domain of the curve.
  // Returns:
  //   TRUE if returned if the length calculation is successful.
  //   FALSE is returned if the length is not calculated.
  // Remarks:
  //   The arc length will be computed so that
  //   (returned length - real length)/(real length) <= fractional_tolerance
  //   More simply, if you want N significant figures in the answer, set the
  //   fractional_tolerance to 1.0e-N.  For "nice" curves, 1.0e-8 works
  //   fine.  For very high degree NURBS and NURBS with bad parametrizations,
  //   use larger values of fractional_tolerance.
  BOOL GetLength(
          double* length,
          double fractional_tolerance = 1.0e-8,
          const ON_Interval* sub_domain = NULL
          ) const;

  /*
  Description:
    Looks for segments that are shorter than tolerance
    that can be removed. If bRemoveShortSegments is true,
    then the short segments are removed. Does not change the 
    domain, but it will change the relative parameterization.
  Parameters:
    tolerance - [in]
    bRemoveShortSegments - [in] If true, then short segments
                                are removed.
  Returns:
    True if removable short segments can were found.
    False if no removable short segments can were found.
  See Also:
    ON_NurbsCurve::RepairBadKnots
  */
  bool RemoveShortSegments(
    double tolerance,
    bool bRemoveShortSegments = true
    );

  /*
  Description:
    Looks for problems caused by knots that are close together
    or have mulitplicity >= order. If bRepair is true, the problems
    are fixed.  Does not change the domain.
  Parameters:
    knot_tolerance - [in] >= 0  When in doubt, use zero.
    bRepair - [in] If true, then problems are repaired.
      Otherwise this function looks for problemsn that
      can be repaired, but does not modify the curve.
  Returns:
    True if bad knots were found and can be repaired.
  See Also:
    ON_NurbsCurve::RemoveShortSegments
  */
  bool RepairBadKnots(
    double knot_tolerance=0.0,
    bool bRepair = true
    );

  // Description:
  //   virtual ON_Curve::Trim override.
  BOOL Trim( const ON_Interval& );

  // Description:
  //   Where possible, analytically extends curve to include domain.
  // Parameters:
  //   domain - [in] if domain is not included in curve domain, 
  //   curve will be extended so that its domain includes domain.  
  //   Will not work if curve is closed. Original curve is identical
  //   to the restriction of the resulting curve to the original curve domain, 
  // Returns:
  //   true if successful.
  bool Extend(
    const ON_Interval& domain
    );

  // Description:
  //   virtual ON_Curve::Split override.
  //
  // Split() divides the curve at the specified parameter.  The parameter
  // must be in the interior of the curve's domain.  The pointers passed
  // to ON_NurbsCurve::Split must either be NULL or point to an ON_NurbsCurve.
  // If the pointer is NULL, then a curve will be created
  // in Split().  You may pass "this" as one of the pointers to Split().
  // For example, 
  //
  //   ON_NurbsCurve right_side;
  //   crv.Split( crv.Domain().Mid() &crv, &right_side );
  //
  // would split crv at the parametric midpoint, put the left side in crv,
  // and return the right side in right_side.
  BOOL Split(
      double,    // t = curve parameter to split curve at
      ON_Curve*&, // left portion returned here (must be an ON_NurbsCurve)
      ON_Curve*&  // right portion returned here (must be an ON_NurbsCurve)
    ) const;

  // Description:
  //   virtual ON_Curve::GetNurbForm override.
  int GetNurbForm( // returns 0: unable to create NURBS representation
                   //            with desired accuracy.
                   //         1: success - returned NURBS parameterization
                   //            matches the curve's to wthe desired accuracy
                   //         2: success - returned NURBS point locus matches
                   //            the curve's to the desired accuracy but, on
                   //            the interior of the curve's domain, the 
                   //            curve's parameterization and the NURBS
                   //            parameterization may not match to the 
                   //            desired accuracy.
        ON_NurbsCurve&,
        double = 0.0,
        const ON_Interval* = NULL     // OPTIONAL subdomain of curve
        ) const;

  // Description:
  //   virtual ON_Curve::HasNurbForm override.
  int HasNurbForm( // returns 0: unable to create NURBS representation
                   //            with desired accuracy.
                   //         1: success - returned NURBS parameterization
                   //            matches the curve's to wthe desired accuracy
                   //         2: success - returned NURBS point locus matches
                   //            the curve's to the desired accuracy but, on
                   //            the interior of the curve's domain, the 
                   //            curve's parameterization and the NURBS
                   //            parameterization may not match to the 
                   //            desired accuracy.
        ) const;

  // Description:
  //   virtual ON_Curve::GetCurveParameterFromNurbFormParameter override
  BOOL GetCurveParameterFromNurbFormParameter(
        double, // nurbs_t
        double* // curve_t
        ) const;

  // Description:
  //   virtual ON_Curve::GetNurbFormParameterFromCurveParameter override
  BOOL GetNurbFormParameterFromCurveParameter(
        double, // curve_t
        double* // nurbs_t
        ) const;

  /// virtual ON_Curve override
  int NumIntersectionsWith(const ON_Line& segment) const;    

public:

  /////////////////////////////////////////////////////////////////
  // Interface

  bool IsRational(  // TRUE if NURBS curve is rational
        void
        ) const;
  
  int CVSize(       // number of doubles per control vertex 
        void        // = IsRational() ? Dim()+1 : Dim()
        ) const;
  
  int Order(        // order = degree + 1
        void
        ) const;
	
  int CVCount(      // number of control vertices
        void 
        ) const;

  int KnotCount(    // total number of knots in knot vector
        void
        ) const;
  
  /*
  Description:
    Expert user function to get a pointer to control vertex
    memory.  If you are not an expert user, please use
    ON_NurbsCurve::GetCV( ON_3dPoint& ) or 
    ON_NurbsCurve::GetCV( ON_4dPoint& ).
  Parameters:
    cv_index - [in]
  Returns:
    Pointer to control vertex.
  Remarks:
    If the NURBS curve is rational, the format of the 
    returned array is a homogeneos rational point with
    length m_dim+1.  If the NURBS curve is not rational, 
    the format of the returned array is a nonrational 
    euclidean point with length m_dim.
  See Also
    ON_NurbsCurve::CVStyle
    ON_NurbsCurve::GetCV
    ON_NurbsCurve::Weight
  */
  double* CV(
        int cv_index
        ) const;

  /*
  Description:
    Returns the style of control vertices in the m_cv array.
  Returns:
    @untitled table
    ON::not_rational                m_is_rat is FALSE
    ON::homogeneous_rational        m_is_rat is TRUE
  */
  ON::point_style CVStyle() const;


  double Weight(        // get value of control vertex weight
        int             // CV index ( >= 0 and < CVCount() )
        ) const;

  BOOL SetWeight(      // get value of control vertex weight
        int,            // CV index ( >= 0 and < CVCount() )
        double
        );

  BOOL SetCV(              // set a single control vertex
        int,              // CV index ( >= 0 and < CVCount() )
        ON::point_style, // style of input point
        const double*     // value of control vertex
        );

  BOOL SetCV(               // set a single control vertex
        int,               // CV index ( >= 0 and < CVCount() )
        const ON_3dPoint& // value of control vertex
                           // If NURBS is rational, weight
                           // will be set to 1.
        );

  BOOL SetCV(              // set a single control vertex
        int,              // CV index ( >= 0 and < CVCount() )
        const ON_4dPoint& // value of control vertex
                          // If NURBS is not rational, euclidean
                          // location of homogeneous point will
                          // be used.
        );

  BOOL GetCV(              // get a single control vertex
        int,              // CV index ( >= 0 and < CVCount() )
        ON::point_style, // style to use for output point
        double*           // array of length >= CVSize()
        ) const;

  BOOL GetCV(              // get a single control vertex
        int,              // CV index ( >= 0 and < CVCount() )
        ON_3dPoint&      // gets euclidean cv when NURBS is rational
        ) const;

  BOOL GetCV(              // get a single control vertex
        int,              // CV index ( >= 0 and < CVCount() )
        ON_4dPoint&      // gets homogeneous cv
        ) const;

  // Description:
  //   Set knot value.
  // Parameters:
  //   knot_index - [in] 0 <= knot_index <= KnotCount()-1
  //   knot_value - [in]
  // Remarks:
  //   m_knot[] must exist.  Use ReserveKnotCapacity to
  //   allocate m_knot[].
  // Returns:
  //   TRUE if successful
  // See Also:
  //   ON_NurbsCurve::ReserveKnotCapacity
  bool SetKnot(
        int knot_index,
        double knot_value
        );

  // Description:
  //   Get knot value.
  // Parameters:
  //   knot_index - [in] 0 <= knot_index <= KnotCount()-1
  // Returns:
  //   knot value = m_knot[knot_index]
  // See Also:
  //   ON_NurbsCurve::SetKnot, ON_NurbsCurve::KnotMultiplicity
  double Knot(
        int knot_index
        ) const;

  // Description:
  //   Get knot multiplicity.
  // Parameters:
  //   knot_index - [in] 0 <= knot_index <= KnotCount()-1
  // Returns:
  //   knot multiplicity = m_knot[knot_index]
  // See Also:
  //   ON_NurbsCurve::SetKnot, ON_NurbsCurve::Knot, 
  //   ON_NurbsCurve::InsertKnot
  int KnotMultiplicity(
        int knot_index
        ) const;

  // Description:
  //   Get pointer to knot vector array.
  // Returns:
  //   pointer to knot vector array (m_knot).
  // See Also:
  //   ON_NurbsCurve::SetKnot, ON_NurbsCurve::Knot, 
  //   ON_NurbsCurve::InsertKnot
  const double* Knot() const;

  // Description:
  //   Make knot vector a clamped uniform knot vector
  //   based on the current values of m_order and m_cv_count.
  //   Does not change values of control vertices.
  // Parameters:
  //   delta - [in] (>0.0) knot spacing.
  // Returns:
  //   TRUE if successful.
  // Remarks:
  //   Allocates m_knot[] if it is not big enough.
  // See Also:
  //   ON_MakeClampedUniformKnotVector
  bool MakeClampedUniformKnotVector( 
    double delta = 1.0 
    );

  // Description:
  //   Make knot vector a periodic uniform knot vector
  //   based on the current values of m_order and m_cv_count.
  //   Does not change values of control vertices.
  // Parameters:
  //   delta - [in] (>0.0) knot spacing.
  // Returns:
  //   TRUE if successful.
  // Remarks:
  //   Allocates m_knot[] if it is not big enough.
  // See Also:
  //   ON_MakePeriodicUniformKnotVector
  bool MakePeriodicUniformKnotVector( 
    double delta = 1.0 
    );

  bool IsClamped( // determine if knot vector is clamped
        int = 2 // end to check: 0 = start, 1 = end, 2 = start and end
        ) const;
  
  double SuperfluousKnot(
           int // 0 = start, 1 = end
           ) const;

  double GrevilleAbcissa(
           int   // index (0 <= index < CVCount(dir)
           ) const;

  bool GetGrevilleAbcissae( // see ON_GetGrevilleAbcissae() for details
           double*   // g[cv_count]
           ) const;

  bool ZeroCVs(); // zeros control vertices and, if rational, sets weights to 1

  // Description:
  //   Clamp end knots.  Does not modify control points.
  // Parameters:
  //   end - [in] 0 = clamp start, 1 = clamp end, 2 = clamp start and end
  // Returns:
  //   TRUE if successful
  bool ClampEnd(
            int end
            );

  // Description:
  //   Insert a knot and update cv locations.
  // Parameters:
  //   knot_value - [in] m_knot[order-2] < knot_value < m_knot[m_cv_count-1]
  //   knot_multiplicity - [in] 1 to degree - includes multiplicity of existing knots.
  // Remarks:
  //   Does not change parameterization or locus of curve.
  // Returns:
  //   TRUE if successful
  bool InsertKnot( 
            double knot_value,
            int knot_multiplicity
            );

  bool MakeRational();

  bool MakeNonRational();

  bool IncreaseDegree(
          int desired_degree
          );

  bool ChangeDimension(
          int desired_dimension
          );

  bool Append( const ON_NurbsCurve& );

  /////////////////////////////////////////////////////////////////
  // Tools for managing CV and knot memory
  bool ReserveCVCapacity(
    int // number of doubles to reserve
    );
  bool ReserveKnotCapacity(
    int // number of doubles to reserve
    );

  //////////
  // returns the length of the control polygon
  double ControlPolygonLength() const;

  ////////
  // Converts a span of the NURBS curve into a bezier.  If
  // the span is empty 
  // (m_knot[span_index+m_order-2] == m_knot[span_index+m_order-1]),
  // then FALSE is returned.
  bool ConvertSpanToBezier(
      int,            // span_index (0 <= span_index <= m_cv_count-m_order)
      ON_BezierCurve& // bezier returned here
      ) const;

  ////////
  // Returns TRUE if the NURBS curve has bezier spans 
  // (all distinct knots have multiplitity = degree)
  bool HasBezierSpans() const;

  /*
  Description:
    Clamps ends and adds knots so the NURBS curve has bezier spans 
   (all distinct knots have multiplitity = degree).
  Paremeters:
    bSetEndWeightsToOne - [in] If true and the first or last weight is
       not one, then the first and last spans are reparameterized so 
       that the end weights are one.
  Returns:
    true if successful.
  */      
  bool MakePiecewiseBezier( 
        bool bSetEndWeightsToOne = false
        );
  

  int BezierSpanCount() const;
  bool CopyBezierSpan(int span, ON_BezierCurve& b);  
  const ON_BezierCurve* BezierSpan(int span) const;

private:
  // XXX: ensure the cache is reset if the curve is modified!
  ON_BezierCurve* m_cached_bez;

  /////////////////////////////////////////////////////////////////
  // Implementation
public:
  // NOTE: These members are left "public" so that expert users may efficiently
  //       create NURBS curves using the default constructor and borrow the
  //       knot and CV arrays from their native NURBS representation.
  //       No technical support will be provided for users who access these
  //       members directly.  If you can't get your stuff to work, then use
  //       the constructor with the arguments and the SetKnot() and SetCV()
  //       functions to fill in the arrays.

  int     m_dim;            // (>=1)

  int     m_is_rat;         // 1 for rational B-splines. (Rational control
                            // vertices use homogeneous form.)
                            // 0 for non-rational B-splines. (Control
                            // verticies do not have a weight coordinate.)

  int     m_order;          // order = degree+1 (>=2)

  int     m_cv_count;       // number of control vertices ( >= order )

  // knot vector memory

  int     m_knot_capacity;  // If m_knot_capacity > 0, then m_knot[]
                            // is an array of at least m_knot_capacity
                            // doubles whose memory is managed by the
                            // ON_NurbsCurve class using rhmalloc(),
                            // onrealloc(), and rhfree().
                            // If m_knot_capacity is 0 and m_knot is
                            // not NULL, then  m_knot[] is assumed to
                            // be big enough for any requested operation
                            // and m_knot[] is not deleted by the
                            // destructor.

  double* m_knot;           // Knot vector. ( The knot vector has length
                            // m_order+m_cv_count-2. )
  
  // control vertex net memory

  int     m_cv_stride;      // The pointer to start of "CV[i]" is
                            //   m_cv + i*m_cv_stride.

  int     m_cv_capacity;    // If m_cv_capacity > 0, then m_cv[] is an array
                            // of at least m_cv_capacity doubles whose
                            // memory is managed by the ON_NurbsCurve
                            // class using rhmalloc(), onrealloc(), and rhfree().
                            // If m_cv_capacity is 0 and m_cv is not
                            // NULL, then m_cv[] is assumed to be big enough
                            // for any requested operation and m_cv[] is not
                            // deleted by the destructor.

  double* m_cv;             // Control points.
                            // If m_is_rat is FALSE, then control point is
                            //
                            //          ( CV(i)[0], ..., CV(i)[m_dim-1] ).
                            //
                            // If m_is_rat is TRUE, then the control point
                            // is stored in HOMOGENEOUS form and is
                            //
                            //           [ CV(i)[0], ..., CV(i)[m_dim] ].
                            //
};


#endif
