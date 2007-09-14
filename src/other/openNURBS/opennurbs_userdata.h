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

#if !defined(OPENNURBS_USERDATA_INC_)
#define OPENNURBS_USERDATA_INC_

class ON_CLASS ON_UserData : public ON_Object
{
  ON_OBJECT_DECLARE(ON_UserData);
public:
  ON_UserData();
  ON_UserData(const ON_UserData&);
  ON_UserData& operator=(const ON_UserData&);

  //////////
  // The destructor automatically removes the user data
  // from ON_Object::m_userdata_list.
  ~ON_UserData();

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

  /*
  Description:
    Overrides virtual ON_Object::Dump().
    Prints class name, description, and uuid.
  Parameters:
    text_log - [in] Information is sent to this text log.
  Remarks:
  */
  void Dump( ON_TextLog& text_log ) const;

  /*
  Description:
    Overrides virtual ON_Object::SizeOf().
  Returns:
    Approximate number of bytes this class uses.
  */
  unsigned int SizeOf() const;

  ////////
  // Returns object that owns the user data
  ON_Object* Owner() const;

  ////////
  // Used for traversing list of user data attached
  // to an object.
  ON_UserData* Next() const;

  ////////
  // Returns the class id which is not necessarily the
  // same as m_userdata_uuid.
  ON_UUID UserDataClassUuid() const;

  //////////
  // Returns TRUE if the user data is anonymous.  This happens
  // when the user data class is not defined at the time the
  // user data is read from an archive.  For example, if a class
  // derived from ON_UserData is defined in application A
  // but is not defined in application B, then the class can be
  // defined when an archive is written by A but not exist when
  // an archive is read by B.  In this case, the
  // user data is not lost, it is just read as ON_UnknownUserData
  // by application B.  If application B saves the parent
  // object in an archive, the unknown user data is resaved in
  // a form that can be read by application A.
  BOOL IsUnknownUserData() const;

  /*
  Parameters:
    description - [out] description of user data shown in
                        object properties dump.
  Returns:
    True if user data class is ready.
  */
  virtual
  BOOL GetDescription( ON_wString& description );

  /*
  Description:
    User will persist in binary archives if Archive() returns
    TRUE, m_application_uuid is not nil, and the virtual Read()
    and Write() are functions are overridden.

  Returns:
    TRUE if user data should persist in binary archives.
    FALSE if the user data should not be save in binary archives.

  Remarks:
    The default implementation returns FALSE.  If you override
    ON_UserData::Archive so that it returns true, then your
    constructor must set m_application_uuid, you must override
    the virtual ON_Object::Read and ON_Object::Write functions and
    you must CAREFULLY TEST your code.

    ON_UserData requires expert programming and testing skills.

    YOU SHOULD READ AND UNDERSTAND EVERY COMMENT IN THIS
    HEADER FILE IN BEFORE ATTEMPTING TO USE ON_UserData.
  */
  virtual
  BOOL Archive() const;

  /*
  Description:
    If Transform() return FALSE, then the userdata is destroyed when
    its parent object is transformed.  The default Transform()
    updates m_userdata_xform and returns TRUE.
    Carefully read the comments above m_userdata_xform
  */
  virtual
  BOOL Transform( const ON_Xform& );

  /*
  Description:
    This uuid is the value that must be passed to
    ON_Object::GetUserData() to retrieve
    this piece of user data.
  */
  ON_UUID m_userdata_uuid;

  /*
  Description:
    This uuid is used to identify the application that
    created this piece of user data.  In the case of
    Rhino, this is the id of the plug-in that created
    the user data. User data with a nil application id
    will not be saved in 3dm archives.
  */
  ON_UUID m_application_uuid;

  ////////
  // If m_userdata_copycount is 0, user data is not copied when
  // object is copied.  If > 0, user data is copied and m_copycount
  // is incremented when parent object is copied. The user data's
  // operator=() is used to copy.
  // The default ON_UserData::ON_UserData() constructor sets
  // m_userdata_copycount to zero.
  unsigned int m_userdata_copycount;

  ////////
  // Updated if user data is attached to a piece of geometry that is
  // transformed and the virtual ON_UserData::Transform() is not
  // overridden.  If you override ON_UserData::Transform() and want
  // m_userdata_xform to be updated, then call the
  // ON_UserData::Transform() in your override.
  // The default constructor sets m_userdata_xform to the identity.
  ON_Xform m_userdata_xform;

private: // don't look and don't touch - these may change
  friend int ON_BinaryArchive::ReadObject( ON_Object** );
  friend bool ON_BinaryArchive::WriteObject( const ON_Object& );
  friend bool ON_BinaryArchive::ReadObjectUserData( ON_Object& );
  friend bool ON_BinaryArchive::WriteObjectUserData( const ON_Object& );
  friend class ON_Object;
  ON_Object* m_userdata_owner;
  ON_UserData* m_userdata_next;
};

class ON_CLASS ON_UnknownUserData : public ON_UserData
{
  ON_OBJECT_DECLARE(ON_UnknownUserData);
  // used to hold user data will application class is not loaded
  // at time data is read
public:
  ON_UnknownUserData();
  ON_UnknownUserData(const ON_UnknownUserData&);
  ~ON_UnknownUserData();
  ON_UnknownUserData& operator=(const ON_UnknownUserData&);

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

  void Dump( ON_TextLog& ) const;
  BOOL Write( ON_BinaryArchive& ) const;
  BOOL Read( ON_BinaryArchive& );

  unsigned int SizeOf() const; // return amount of memory used by user data
  BOOL GetDescription( ON_wString& ); // description of user data
  BOOL Archive() const;

  // Convert unknown user data to actual user data.  Useful if
  // definition of actual user data is dynamically linked after
  // archive containing user data is read.
  ON_UserData* Convert() const;

  /*
  Description:
    This is the uuid of the missing class.  This uuid
    is the 3rd parameter to the ON_OBJECT_IMPLEMENT()
    macro of the missing class.
  */
  ON_UUID m_unknownclass_uuid;
  int m_sizeof_buffer;
  void* m_buffer;
  int m_3dm_version; // version of 3dm archive data was read from
};

class ON_CLASS ON_UserStringList : public ON_UserData
{
  ON_OBJECT_DECLARE(ON_UserStringList);
public:

  ON_UserStringList();
  ~ON_UserStringList();

  // override virtual ON_Object::Dump function
  void Dump( ON_TextLog& text_log ) const;

  // override virtual ON_Object::Dump function
  unsigned int SizeOf() const;

  // override virtual ON_Object::Write function
  BOOL Write(ON_BinaryArchive& binary_archive) const;

  // override virtual ON_Object::Read function
  BOOL Read(ON_BinaryArchive& binary_archive);

  // override virtual ON_UserData::GetDescription function
  BOOL GetDescription( ON_wString& description );

  // override virtual ON_UserData::Archive function
  BOOL Archive() const;

  bool SetUserString( const wchar_t* key, const wchar_t* string_value );

  bool GetUserString( const wchar_t* key, ON_wString& string_value ) const;

  ON_ClassArray<ON_UserString> m_e;
};

class ON_CLASS ON_UserDataHolder : public ON_Object
{
public:
  /*
  Description:
    Transfers the user data from source_object to "this".
    When MoveUserDataFrom() returns source_object will not
    have any user data.  If "this" had user data when
    MoveUserDataFrom() was called, then that user data is
    destroyed.
  Parameters:
    source_object - [in] The "const" is a lie.  It is
      there because, in practice the source object is frequently
      const and const_cast ends up being excessively used.
  Returns:
    True if source_object had user data that was transfered
    to "this".  False if source_object had no user data.
    In any case, any user data that was on the input "this"
    is destroyed.
  */
  bool MoveUserDataFrom( const ON_Object& source_object );

  /*
  Description:
    Transfers the user data on "this" to source_object.
    When MoveUserDataTo() returns "this" will not have any
    user data.
  Parameters:
    source_object - [in] The "const" is a lie.  It is
      there because, in practice the source object is generally
      const and const_cast ends up being constantly used.
    bAppend - [in] if true, existing user data on source_object
      is left unchanged.  If false, existing user data on source_object
      is destroyed, even when there is no user data on "this".
  Returns:
    True if "this" had user data that was transfered to source_object.
    In any case, any user data that was on the input "this"
    is destroyed.
  */
  bool MoveUserDataTo(  const ON_Object& source_object, bool bAppend );

  BOOL IsValid( ON_TextLog* text_log = NULL ) const;
};

#endif
