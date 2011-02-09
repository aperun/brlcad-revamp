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

#if !defined(OPENNURBS_BASE64_INC_)
#define OPENNURBS_BASE64_INC_

//////////////////////////////////////////////////////////////////////////////////////////

class ON_CLASS ON_EncodeBase64
{
public:
  ON_EncodeBase64();
  virtual ~ON_EncodeBase64();

  void Begin();

  // Calling Encode will generate at least
  // sizeof_buffer/57 and at most (sizeof_buffer+56)/57
  // calls to Output().  Every callback to Output() will
  // have m_output_count = 76.
  void Encode(const void* buffer, size_t sizeof_buffer);

  // Calling End may generate a single call to Output()
  // If it does generate a single call to Output(),
  // then m_output_count will be between 1 and 76.
  void End(); // may generate a single call to Output().

  // With a single exception, when Output() is called,
  // 57 input bytes have been encoded into 76 output
  // characters with ASCII codes A-Z, a-z, 0-9, +, /.
  // m_output_count will be 76
  // m_output[0...(m_output_count-1)] will be the base 64
  // encoding.
  // m_output[m_output_count] = 0.
  // The Output() function can modify the values of m_output[]
  // and m_output_count anyway it wants.
  virtual void Output();

  // Total number of bytes passed to Encode().
  int m_encode_count;

  // When the virtual Output() is called, there are m_output_count (1 to 76)
  // characters of base64 encoded output in m_output[].  The remainder of 
  // the m_output[] array is zero.  The Output function may modify the
  // contents of m_output[] any way it sees fit.
  int  m_output_count;
  char m_output[80];
  
private:
  // input waiting to be encoded
  // At most 56 bytes can be waiting to be processed in m_input[].
  unsigned int  m_unused2; // Here for alignment purposes. Never used by opennurbs.
  unsigned int  m_input_count;
  unsigned char m_input[64];

  void EncodeHelper1(const unsigned char*, char*);
  void EncodeHelper2(const unsigned char*, char*);
  void EncodeHelper3(const unsigned char*, char*);
  void EncodeHelper57(const unsigned char*);
};

//////////////////////////////////////////////////////////////////////////////////////////

class ON_CLASS ON_DecodeBase64
{
public:
  ON_DecodeBase64();
  virtual ~ON_DecodeBase64();

  void Begin();

  // Decode will generate zero or more callbacks to the
  // virtual Output() function.  If the base 64 encoded information
  // is in pieces, you can call Decode() for each piece.  For example,
  // if your encoded information is in a text file, you might call
  // Decode() for every line in the file.  Decode() returns 0 if
  // there is nothing in base64str to decode or if it detects an
  // error that prevents any further decoding.  The function Error()
  // can be used to determine if an error occured.  Otherwise,
  // Decode() returns a pointer to the location in the string where
  // it stopped decoding because it detected a character, like a null
  // terminator, an end of line character, or any other character
  // that could not be part of the base 64 encoded information.
  const char* Decode(const char* base64str);
  const char* Decode(const char* base64str, size_t base64str_count);
  const wchar_t* Decode(const wchar_t* base64str);
  const wchar_t* Decode(const wchar_t* base64str, size_t base64str_count);

  // You must call End() when Decode() returns 0 or when you have
  // reached the end of your encoded information.  End() may
  // callback to Output() zero or one time.  If all the information
  // passed to Decode() was successfully decoded, then End()
  // returns true.  If something was not decoded, then End()
  // returns false.
  bool End();

  // Override the virtual Output() callback function to process the 
  // decoded output.  Each time Output() is called there are m_output_count
  // bytes in the m_output[] array.
  // Every call to Decode() can result in zero, one, or many callbacks
  // to Output().  Calling End() may result in zero or one callbacks
  // to Output().
  virtual void Output();

  // m_decode_count = total number of input base64 characters
  // that Decode() has decoded.
  unsigned int m_decode_count; 

  int  m_output_count; // 0 to 512
  unsigned char m_output[512];

  // Call if your Output() function detects an error and
  // wants to stop further decoding.
  void SetError();

  // Returns true if an error occured during decoding because
  // invalid input was passed to Decode().
  bool Error() const;

private:
  int m_status; // 1: error - decoding stopped
                // 2: '=' encountered as 3rd char in Decode()
                // 3: successfully parsed "**=="
                // 4: successfully parsed "***="
                // 5: End() successfully called.

  // cached encoded input from previous call to Decode() 
  int m_cache_count;
  int m_cache[4];

  void DecodeHelper1(); // decodes "**==" quartet into 1 byte
  void DecodeHelper2(); // decodes "***=" quartet into 2 bytes
};

#endif
