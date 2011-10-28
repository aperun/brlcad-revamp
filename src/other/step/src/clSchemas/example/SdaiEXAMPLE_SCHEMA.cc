#ifndef  SDAIEXAMPLE_SCHEMA_CC
#define  SDAIEXAMPLE_SCHEMA_CC
// This file was generated by fedex_plus.  You probably don't want to edit
// it since your modifications will be lost if fedex_plus is used to
// regenerate it.
/* $Id$  */ 
#ifndef  SCHEMA_H 
#include <schema.h> 
#endif 

#ifdef  SCL_LOGGING 
#include <fstream.h>
    extern ofstream *logStream;
#define SCLLOGFILE "scl.log"
#endif 

static int debug_access_hooks = 0;

#ifdef PART26

const char * sclHostName = CORBA::Orbix.myHost(); // Default is local host
#endif

Schema *s_example_schema =0;

/*	**************  TYPES  	*/
EnumTypeDescriptor 	*example_schemat_color;
//////////  ENUMERATION TYPE SdaiColor_var
const char * 
SdaiColor_var::element_at (int n) const  {
  switch (n)  {
  case Color__green	:  return "GREEN";
  case Color__white	:  return "WHITE";
  case Color__orange	:  return "ORANGE";
  case Color__yellow	:  return "YELLOW";
  case Color__red	:  return "RED";
  case Color__black	:  return "BLACK";
  case Color__brown	:  return "BROWN";
  case Color__blue	:  return "BLUE";
  case Color_unset	:
  default		:  return "UNSET";
  }
}

SdaiColor_var::SdaiColor_var (const char * n, EnumTypeDescriptor *et)
  : type(et)
{
  set_value (n);
}

SdaiColor_var::operator Color () const {
  switch (v) {
	case Color__green	:  return Color__green;
	case Color__white	:  return Color__white;
	case Color__orange	:  return Color__orange;
	case Color__yellow	:  return Color__yellow;
	case Color__red	:  return Color__red;
	case Color__black	:  return Color__black;
	case Color__brown	:  return Color__brown;
	case Color__blue	:  
	default		:  return Color__blue;
  }
}


SCLP23(Enum) * 
create_SdaiColor_var () 
{
    return new SdaiColor_var( "", example_schemat_color );
}


SdaiColor_vars::SdaiColor_vars( EnumTypeDescriptor *et )
    : enum_type(et)
{
}

SdaiColor_vars::~SdaiColor_vars()
{
}


STEPaggregate * 
create_SdaiColor_vars ()
{
    return new SdaiColor_vars( example_schemat_color );
}

//////////  END ENUMERATION color

TypeDescriptor 	*example_schemat_label;
TypeDescriptor 	*example_schemat_point;
TypeDescriptor 	*example_schemat_length_measure;

/*	**************  ENTITIES  	*/

/////////	 ENTITY poly_line

EntityDescriptor *example_schemae_poly_line =0;
AttrDescriptor *a_0points =0;
SdaiPoly_line::SdaiPoly_line( ) 
{

	/*  no SuperTypes */

    eDesc = example_schemae_poly_line;

    STEPattribute *a = new STEPattribute(*a_0points,  &_points);

    a -> set_null ();
    attributes.push (a);
}
SdaiPoly_line::SdaiPoly_line (SdaiPoly_line& e ) 
	{  CopyAs((SCLP23(Application_instance_ptr)) &e);	}
SdaiPoly_line::~SdaiPoly_line () {  }


#ifdef __O3DB__
void 
SdaiPoly_line::oodb_reInit ()
{	eDesc = example_schemae_poly_line;
	attributes [0].aDesc = a_0points;
}
#endif

SdaiPoly_line::SdaiPoly_line( SCLP23(Application_instance) *se, int *addAttrs)
{
	/* Set this to point to the head entity. */
    HeadEntity(se); 

	/*  no SuperTypes */

    eDesc = example_schemae_poly_line;

    STEPattribute *a = new STEPattribute(*a_0points,  &_points);
    a -> set_null ();
	/* Put attribute on this class' attributes list so the */
	/*access functions still work. */
    attributes.push (a);
	/* Put attribute on the attributes list for the */
	/* main inheritance heirarchy. */
    if(!addAttrs || addAttrs[0])
        se->attributes.push (a);
}

const EntityAggregate_ptr 
SdaiPoly_line::points_() const
{
    return (EntityAggregate_ptr) &_points; 
}

void 
SdaiPoly_line::points_ (const EntityAggregate_ptr x)

	{ _points.ShallowCopy (*x); }

/////////	 END_ENTITY poly_line


/////////	 ENTITY shape

EntityDescriptor *example_schemae_shape =0;
AttrDescriptor *a_1item_name =0;
AttrDescriptor *a_2item_color =0;
AttrDescriptor *a_3number_of_sides =0;
SdaiShape::SdaiShape( ) 
{

	/*  no SuperTypes */

    eDesc = example_schemae_shape;

    STEPattribute *a = new STEPattribute(*a_1item_name,  &_item_name);
    a -> set_null ();
    attributes.push (a);

    a = new STEPattribute(*a_2item_color,  &_item_color);
    a -> set_null ();
    attributes.push (a);

    a = new STEPattribute(*a_3number_of_sides,  &_number_of_sides);
    a -> set_null ();
    attributes.push (a);
}
SdaiShape::SdaiShape (SdaiShape& e ) 
	{  CopyAs((SCLP23(Application_instance_ptr)) &e);	}
SdaiShape::~SdaiShape () {  }


#ifdef __O3DB__
void 
SdaiShape::oodb_reInit ()
{	eDesc = example_schemae_shape;
	attributes [0].aDesc = a_1item_name;
	attributes [1].aDesc = a_2item_color;
	attributes [2].aDesc = a_3number_of_sides;
}
#endif

SdaiShape::SdaiShape( SCLP23(Application_instance) *se, int *addAttrs)
{
	/* Set this to point to the head entity. */
    HeadEntity(se); 

	/*  no SuperTypes */

    eDesc = example_schemae_shape;

    STEPattribute *a = new STEPattribute(*a_1item_name,  &_item_name);
    a -> set_null ();
	/* Put attribute on this class' attributes list so the */
	/*access functions still work. */
    attributes.push (a);
	/* Put attribute on the attributes list for the */
	/* main inheritance heirarchy. */
    if(!addAttrs || addAttrs[0])
        se->attributes.push (a);

    a = new STEPattribute(*a_2item_color,  &_item_color);
    a -> set_null ();
	/* Put attribute on this class' attributes list so the */
	/*access functions still work. */
    attributes.push (a);
	/* Put attribute on the attributes list for the */
	/* main inheritance heirarchy. */
    if(!addAttrs || addAttrs[0])
        se->attributes.push (a);

    a = new STEPattribute(*a_3number_of_sides,  &_number_of_sides);
    a -> set_null ();
	/* Put attribute on this class' attributes list so the */
	/*access functions still work. */
    attributes.push (a);
	/* Put attribute on the attributes list for the */
	/* main inheritance heirarchy. */
    if(!addAttrs || addAttrs[0])
        se->attributes.push (a);
}

const SdaiLabel 
SdaiShape::item_name_() const
{
    return (const SdaiLabel) _item_name; 
}

void 
SdaiShape::item_name_ (const SdaiLabel x)

{
    _item_name = x; 
}

const SdaiColor_var 
SdaiShape::item_color_() const
{
    return (Color) _item_color; 
}

void 
SdaiShape::item_color_ (const SdaiColor_var x)

{
    _item_color.put (x); 
}

const SCLP23(Integer) 
SdaiShape::number_of_sides_() const
{
    return (const SCLP23(Integer)) _number_of_sides; 
}

void 
SdaiShape::number_of_sides_ (const SCLP23(Integer) x)

{
    _number_of_sides = x; 
}

/////////	 END_ENTITY shape


/////////	 ENTITY rectangle

EntityDescriptor *example_schemae_rectangle =0;
AttrDescriptor *a_4height =0;
AttrDescriptor *a_5width =0;
SdaiRectangle::SdaiRectangle( ) 
{

	/*  parent: SdaiShape  */
	/* Ignore the first parent since it is */
 	/* part of the main inheritance hierarchy */

    eDesc = example_schemae_rectangle;

    STEPattribute *a = new STEPattribute(*a_4height,  &_height);
    a -> set_null ();
    attributes.push (a);

    a = new STEPattribute(*a_5width,  &_width);
    a -> set_null ();
    attributes.push (a);
}
SdaiRectangle::SdaiRectangle (SdaiRectangle& e ) 
	{  CopyAs((SCLP23(Application_instance_ptr)) &e);	}
SdaiRectangle::~SdaiRectangle () {  }

#ifdef __OSTORE__

void 
SdaiRectangle::Access_hook_in(void *object, 
				enum os_access_reason reason, void *user_data, 
				void *start_range, void *end_range)
{
    if(debug_access_hooks)
        cout << "SdaiRectangle: virtual access function." << endl;
    SdaiRectangle_access_hook_in(object, reason, user_data, start_range, end_range);
}

void 
SdaiRectangle_access_hook_in(void *object, 
	enum os_access_reason reason, void *user_data, 
	void *start_range, void *end_range)
{
    if(debug_access_hooks)
        cout << "SdaiRectangle: non-virtual access function." << endl;
    SCLP23(Application_instance) *sent = (SCLP23(Application_instance) *)object;
    if(debug_access_hooks)
        cout << "STEPfile_id: " << sent->STEPfile_id << endl;
    SdaiRectangle *ent = (SdaiRectangle *)sent;
//    SdaiRectangle *ent = (SdaiRectangle *)object;
    if(ent->eDesc == 0)
        ent->eDesc = example_schemae_rectangle;
    ent->attributes[3].aDesc = a_4height;
    ent->attributes[4].aDesc = a_5width;
}

SCLP23(Application_instance_ptr) 
create_SdaiRectangle(os_database *db) 
{
    if(db)
    {
        SCLP23(DAObject_ptr) ap = new (db, SdaiRectangle::get_os_typespec()) 
                                   SdaiRectangle;
        return (SCLP23(Application_instance_ptr)) ap;
//        return (SCLP23(Application_instance_ptr)) new (db, SdaiRectangle::get_os_typespec()) 
//                                   SdaiRectangle;
    }
    else
        return (SCLP23(Application_instance_ptr)) new SdaiRectangle; 
}
#endif

#ifdef __O3DB__
void 
SdaiRectangle::oodb_reInit ()
{	eDesc = example_schemae_rectangle;
	attributes [3].aDesc = a_4height;
	attributes [4].aDesc = a_5width;
}
#endif

SdaiRectangle::SdaiRectangle (SCLP23(Application_instance) *se, int *addAttrs) : SdaiShape(se, (addAttrs ? &addAttrs[1] : 0)) 
{
	/* Set this to point to the head entity. */
    HeadEntity(se); 

	/*  parent: SdaiShape  */
	/* Ignore the first parent since it is */
 	/* part of the main inheritance hierarchy */

    eDesc = example_schemae_rectangle;

    STEPattribute *a = new STEPattribute(*a_4height,  &_height);
    a -> set_null ();
	/* Put attribute on this class' attributes list so the */
	/*access functions still work. */
    attributes.push (a);
	/* Put attribute on the attributes list for the */
	/* main inheritance heirarchy. */
    if(!addAttrs || addAttrs[0])
        se->attributes.push (a);

    a = new STEPattribute(*a_5width,  &_width);
    a -> set_null ();
	/* Put attribute on this class' attributes list so the */
	/*access functions still work. */
    attributes.push (a);
	/* Put attribute on the attributes list for the */
	/* main inheritance heirarchy. */
    if(!addAttrs || addAttrs[0])
        se->attributes.push (a);
}

const SdaiLength_measure 
SdaiRectangle::height_() const
{
    return (const SdaiLength_measure) _height; 
}

void 
SdaiRectangle::height_ (const SdaiLength_measure x)

{
    _height = x; 
}

const SdaiLength_measure 
SdaiRectangle::width_() const
{
    return (const SdaiLength_measure) _width; 
}

void 
SdaiRectangle::width_ (const SdaiLength_measure x)

{
    _width = x; 
}
	/* The first parent's access functions are */
	/* above or covered by inherited functions. */

/////////	 END_ENTITY rectangle


/////////	 ENTITY square

EntityDescriptor *example_schemae_square =0;
SdaiSquare::SdaiSquare( ) 
{

	/*  parent: SdaiRectangle  */
	/* Ignore the first parent since it is */
 	/* part of the main inheritance hierarchy */

    eDesc = example_schemae_square;
}
SdaiSquare::SdaiSquare (SdaiSquare& e ) 
	{  CopyAs((SCLP23(Application_instance_ptr)) &e);	}
SdaiSquare::~SdaiSquare () {  }

#ifdef __O3DB__
void 
SdaiSquare::oodb_reInit ()
{	eDesc = example_schemae_square;
}
#endif

SdaiSquare::SdaiSquare (SCLP23(Application_instance) *se, int *addAttrs) : SdaiRectangle(se, (addAttrs ? &addAttrs[1] : 0)) 
{
	/* Set this to point to the head entity. */
    HeadEntity(se); 

	/*  parent: SdaiRectangle  */
	/* Ignore the first parent since it is */
 	/* part of the main inheritance hierarchy */

    eDesc = example_schemae_square;
}
	/* The first parent's access functions are */
	/* above or covered by inherited functions. */

/////////	 END_ENTITY square


/////////	 ENTITY triangle

EntityDescriptor *example_schemae_triangle =0;
AttrDescriptor *a_6side1_length =0;
AttrDescriptor *a_7side2_length =0;
AttrDescriptor *a_8side3_length =0;
SdaiTriangle::SdaiTriangle( ) 
{

	/*  parent: SdaiShape  */
	/* Ignore the first parent since it is */
 	/* part of the main inheritance hierarchy */

    eDesc = example_schemae_triangle;

    STEPattribute *a = new STEPattribute(*a_6side1_length,  &_side1_length);
    a -> set_null ();
    attributes.push (a);

    a = new STEPattribute(*a_7side2_length,  &_side2_length);
    a -> set_null ();
    attributes.push (a);

    a = new STEPattribute(*a_8side3_length,  &_side3_length);
    a -> set_null ();
    attributes.push (a);
}
SdaiTriangle::SdaiTriangle (SdaiTriangle& e ) 
	{  CopyAs((SCLP23(Application_instance_ptr)) &e);	}
SdaiTriangle::~SdaiTriangle () {  }

#ifdef __O3DB__
void 
SdaiTriangle::oodb_reInit ()
{	eDesc = example_schemae_triangle;
	attributes [3].aDesc = a_6side1_length;
	attributes [4].aDesc = a_7side2_length;
	attributes [5].aDesc = a_8side3_length;
}
#endif

SdaiTriangle::SdaiTriangle (SCLP23(Application_instance) *se, int *addAttrs) : SdaiShape(se, (addAttrs ? &addAttrs[1] : 0)) 
{
	/* Set this to point to the head entity. */
    HeadEntity(se); 

	/*  parent: SdaiShape  */
	/* Ignore the first parent since it is */
 	/* part of the main inheritance hierarchy */

    eDesc = example_schemae_triangle;

    STEPattribute *a = new STEPattribute(*a_6side1_length,  &_side1_length);
    a -> set_null ();
	/* Put attribute on this class' attributes list so the */
	/*access functions still work. */
    attributes.push (a);
	/* Put attribute on the attributes list for the */
	/* main inheritance heirarchy. */
    if(!addAttrs || addAttrs[0])
        se->attributes.push (a);

    a = new STEPattribute(*a_7side2_length,  &_side2_length);
    a -> set_null ();
	/* Put attribute on this class' attributes list so the */
	/*access functions still work. */
    attributes.push (a);
	/* Put attribute on the attributes list for the */
	/* main inheritance heirarchy. */
    if(!addAttrs || addAttrs[0])
        se->attributes.push (a);

    a = new STEPattribute(*a_8side3_length,  &_side3_length);
    a -> set_null ();
	/* Put attribute on this class' attributes list so the */
	/*access functions still work. */
    attributes.push (a);
	/* Put attribute on the attributes list for the */
	/* main inheritance heirarchy. */
    if(!addAttrs || addAttrs[0])
        se->attributes.push (a);
}

const SdaiLength_measure 
SdaiTriangle::side1_length_() const
{
    return (const SdaiLength_measure) _side1_length; 
}

void 
SdaiTriangle::side1_length_ (const SdaiLength_measure x)

{
    _side1_length = x; 
}

const SdaiLength_measure 
SdaiTriangle::side2_length_() const
{
    return (const SdaiLength_measure) _side2_length; 
}

void 
SdaiTriangle::side2_length_ (const SdaiLength_measure x)

{
    _side2_length = x; 
}

const SdaiLength_measure 
SdaiTriangle::side3_length_() const
{
    return (const SdaiLength_measure) _side3_length; 
}

void 
SdaiTriangle::side3_length_ (const SdaiLength_measure x)

{
    _side3_length = x; 
}
	/* The first parent's access functions are */
	/* above or covered by inherited functions. */

/////////	 END_ENTITY triangle


/////////	 ENTITY circle

EntityDescriptor *example_schemae_circle =0;
AttrDescriptor *a_9radius =0;
SdaiCircle::SdaiCircle( ) 
{

	/*  parent: SdaiShape  */
	/* Ignore the first parent since it is */
 	/* part of the main inheritance hierarchy */

    eDesc = example_schemae_circle;

    STEPattribute *a = new STEPattribute(*a_9radius,  &_radius);
    a -> set_null ();
    attributes.push (a);
}
SdaiCircle::SdaiCircle (SdaiCircle& e ) 
	{  CopyAs((SCLP23(Application_instance_ptr)) &e);	}
SdaiCircle::~SdaiCircle () {  }

#ifdef __O3DB__
void 
SdaiCircle::oodb_reInit ()
{	eDesc = example_schemae_circle;
	attributes [3].aDesc = a_9radius;
}
#endif

SdaiCircle::SdaiCircle (SCLP23(Application_instance) *se, int *addAttrs) : SdaiShape(se, (addAttrs ? &addAttrs[1] : 0)) 
{
	/* Set this to point to the head entity. */
    HeadEntity(se); 

	/*  parent: SdaiShape  */
	/* Ignore the first parent since it is */
 	/* part of the main inheritance hierarchy */

    eDesc = example_schemae_circle;

    STEPattribute *a = new STEPattribute(*a_9radius,  &_radius);
    a -> set_null ();
	/* Put attribute on this class' attributes list so the */
	/*access functions still work. */
    attributes.push (a);
	/* Put attribute on the attributes list for the */
	/* main inheritance heirarchy. */
    if(!addAttrs || addAttrs[0])
        se->attributes.push (a);
}

const SCLP23(Real) 
SdaiCircle::radius_() const
{
    return (const SCLP23(Real)) _radius; 
}

void 
SdaiCircle::radius_ (const SCLP23(Real) x)

{
    _radius = x; 
}
	/* The first parent's access functions are */
	/* above or covered by inherited functions. */

/////////	 END_ENTITY circle


/////////	 ENTITY line

EntityDescriptor *example_schemae_line =0;
AttrDescriptor *a_10end_point_one =0;
AttrDescriptor *a_11end_point_two =0;
SdaiLine::SdaiLine( ) 
{

	/*  no SuperTypes */

    eDesc = example_schemae_line;

    STEPattribute *a = new STEPattribute(*a_10end_point_one, (SCLP23(Application_instance_ptr) *) &_end_point_one);
    a -> set_null ();
    attributes.push (a);

    a = new STEPattribute(*a_11end_point_two, (SCLP23(Application_instance_ptr) *) &_end_point_two);
    a -> set_null ();
    attributes.push (a);
}
SdaiLine::SdaiLine (SdaiLine& e ) 
	{  CopyAs((SCLP23(Application_instance_ptr)) &e);	}
SdaiLine::~SdaiLine () {  }


#ifdef __O3DB__
void 
SdaiLine::oodb_reInit ()
{	eDesc = example_schemae_line;
	attributes [0].aDesc = a_10end_point_one;
	attributes [1].aDesc = a_11end_point_two;
}
#endif

SdaiLine::SdaiLine( SCLP23(Application_instance) *se, int *addAttrs)
{
	/* Set this to point to the head entity. */
    HeadEntity(se); 

	/*  no SuperTypes */

    eDesc = example_schemae_line;

    STEPattribute *a = new STEPattribute(*a_10end_point_one, (SCLP23(Application_instance_ptr) *) &_end_point_one);
    a -> set_null ();
	/* Put attribute on this class' attributes list so the */
	/*access functions still work. */
    attributes.push (a);
	/* Put attribute on the attributes list for the */
	/* main inheritance heirarchy. */
    if(!addAttrs || addAttrs[0])
        se->attributes.push (a);

    a = new STEPattribute(*a_11end_point_two, (SCLP23(Application_instance_ptr) *) &_end_point_two);
    a -> set_null ();
	/* Put attribute on this class' attributes list so the */
	/*access functions still work. */
    attributes.push (a);
	/* Put attribute on the attributes list for the */
	/* main inheritance heirarchy. */
    if(!addAttrs || addAttrs[0])
        se->attributes.push (a);
}

const SdaiCartesian_point_ptr 
SdaiLine::end_point_one_() const
{
    return (SdaiCartesian_point_ptr) _end_point_one; 
}

void 
SdaiLine::end_point_one_ (const SdaiCartesian_point_ptr x)

{
    _end_point_one = x; 
}

const SdaiCartesian_point_ptr 
SdaiLine::end_point_two_() const
{
    return (SdaiCartesian_point_ptr) _end_point_two; 
}

void 
SdaiLine::end_point_two_ (const SdaiCartesian_point_ptr x)

{
    _end_point_two = x; 
}

/////////	 END_ENTITY line


/////////	 ENTITY cartesian_point

EntityDescriptor *example_schemae_cartesian_point =0;
AttrDescriptor *a_12x =0;
AttrDescriptor *a_13y =0;
AttrDescriptor *a_14z =0;
SdaiCartesian_point::SdaiCartesian_point( ) 
{

	/*  no SuperTypes */

    eDesc = example_schemae_cartesian_point;

    STEPattribute *a = new STEPattribute(*a_12x,  &_x);
    a -> set_null ();
    attributes.push (a);

    a = new STEPattribute(*a_13y,  &_y);
    a -> set_null ();
    attributes.push (a);

    a = new STEPattribute(*a_14z,  &_z);
    a -> set_null ();
    attributes.push (a);
}
SdaiCartesian_point::SdaiCartesian_point (SdaiCartesian_point& e ) 
	{  CopyAs((SCLP23(Application_instance_ptr)) &e);	}
SdaiCartesian_point::~SdaiCartesian_point () {  }

#ifdef __O3DB__
void 
SdaiCartesian_point::oodb_reInit ()
{	eDesc = example_schemae_cartesian_point;
	attributes [0].aDesc = a_12x;
	attributes [1].aDesc = a_13y;
	attributes [2].aDesc = a_14z;
}
#endif

SdaiCartesian_point::SdaiCartesian_point( SCLP23(Application_instance) *se, int *addAttrs)
{
	/* Set this to point to the head entity. */
    HeadEntity(se); 

	/*  no SuperTypes */

    eDesc = example_schemae_cartesian_point;

    STEPattribute *a = new STEPattribute(*a_12x,  &_x);
    a -> set_null ();
	/* Put attribute on this class' attributes list so the */
	/*access functions still work. */
    attributes.push (a);
	/* Put attribute on the attributes list for the */
	/* main inheritance heirarchy. */
    if(!addAttrs || addAttrs[0])
        se->attributes.push (a);

    a = new STEPattribute(*a_13y,  &_y);
    a -> set_null ();
	/* Put attribute on this class' attributes list so the */
	/*access functions still work. */
    attributes.push (a);
	/* Put attribute on the attributes list for the */
	/* main inheritance heirarchy. */
    if(!addAttrs || addAttrs[0])
        se->attributes.push (a);

    a = new STEPattribute(*a_14z,  &_z);
    a -> set_null ();
	/* Put attribute on this class' attributes list so the */
	/*access functions still work. */
    attributes.push (a);
	/* Put attribute on the attributes list for the */
	/* main inheritance heirarchy. */
    if(!addAttrs || addAttrs[0])
        se->attributes.push (a);
}

const SdaiPoint 
SdaiCartesian_point::x_() const
{
    return (const SdaiPoint) _x; 
}

void 
SdaiCartesian_point::x_ (const SdaiPoint x)

{
    _x = x; 
}

const SdaiPoint 
SdaiCartesian_point::y_() const
{
    return (const SdaiPoint) _y; 
}

void 
SdaiCartesian_point::y_ (const SdaiPoint x)

{
    _y = x; 
}

const SdaiPoint 
SdaiCartesian_point::z_() const
{
    return (const SdaiPoint) _z; 
}

void 
SdaiCartesian_point::z_ (const SdaiPoint x)

{
    _z = x; 
}

/////////	 END_ENTITY cartesian_point


SCLP23(Model_contents_ptr) create_SdaiModel_contents_example_schema()
{ return new SdaiModel_contents_example_schema ; }

SdaiModel_contents_example_schema::SdaiModel_contents_example_schema()
{
    SCLP23(Entity_extent_ptr) eep = (SCLP23(Entity_extent_ptr))0;

    eep = new SCLP23(Entity_extent);
    eep->definition_(example_schemae_poly_line);
    _folders.Append(eep);

        eep = new SCLP23(Entity_extent);
    eep->definition_(example_schemae_shape);
    _folders.Append(eep);

        eep = new SCLP23(Entity_extent);
    eep->definition_(example_schemae_rectangle);
    _folders.Append(eep);

        eep = new SCLP23(Entity_extent);
    eep->definition_(example_schemae_square);
    _folders.Append(eep);

        eep = new SCLP23(Entity_extent);
    eep->definition_(example_schemae_triangle);
    _folders.Append(eep);

        eep = new SCLP23(Entity_extent);
    eep->definition_(example_schemae_circle);
    _folders.Append(eep);

        eep = new SCLP23(Entity_extent);
    eep->definition_(example_schemae_line);
    _folders.Append(eep);

        eep = new SCLP23(Entity_extent);
    eep->definition_(example_schemae_cartesian_point);
    _folders.Append(eep);

}

SdaiPoly_line__set_var SdaiModel_contents_example_schema::SdaiPoly_line_get_extents()
{
    return (SdaiPoly_line__set_var)((_folders.retrieve(0))->instances_());
}

SdaiShape__set_var SdaiModel_contents_example_schema::SdaiShape_get_extents()
{
    return (SdaiShape__set_var)((_folders.retrieve(1))->instances_());
}

SdaiRectangle__set_var SdaiModel_contents_example_schema::SdaiRectangle_get_extents()
{
    return (SdaiRectangle__set_var)((_folders.retrieve(2))->instances_());
}

SdaiSquare__set_var SdaiModel_contents_example_schema::SdaiSquare_get_extents()
{
    return (SdaiSquare__set_var)((_folders.retrieve(3))->instances_());
}

SdaiTriangle__set_var SdaiModel_contents_example_schema::SdaiTriangle_get_extents()
{
    return (SdaiTriangle__set_var)((_folders.retrieve(4))->instances_());
}

SdaiCircle__set_var SdaiModel_contents_example_schema::SdaiCircle_get_extents()
{
    return (SdaiCircle__set_var)((_folders.retrieve(5))->instances_());
}

SdaiLine__set_var SdaiModel_contents_example_schema::SdaiLine_get_extents()
{
    return (SdaiLine__set_var)((_folders.retrieve(6))->instances_());
}

SdaiCartesian_point__set_var SdaiModel_contents_example_schema::SdaiCartesian_point_get_extents()
{
    return (SdaiCartesian_point__set_var)((_folders.retrieve(7))->instances_());
}
#endif
