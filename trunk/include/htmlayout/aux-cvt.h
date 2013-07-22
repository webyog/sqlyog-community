#ifndef __json_aux_h__
#define __json_aux_h__

/*
 * Terra Informatica Sciter and HTMLayout Engines
 * http://terrainformatica.com/sciter, http://terrainformatica.com/htmlayout 
 * 
 * basic primitives. 
 * 
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 * 
 * (C) 2003-2006, Andrew Fedoniouk (andrew@terrainformatica.com)
 */

/**\file
 * \brief primitives
 **/

/*

  pod::copy<T> - memcpy wrapper
  pod::move<T> - memmove wrapper
  pod::buffer<T> - dynamic buffer, string builder, etc.

  utf8::towcs() - utf8 to wchar_t* converter
  utf8::fromwcs() - wchar_t* to utf8 converter
  utf8::ostream  - raw ASCII/UNICODE -> UTF8 converter 
  utf8::oxstream - ASCII/UNICODE -> UTF8 converter with XML support

  inline bool streq(const char* s, const char* s1) - NULL safe string comparison function
  inline bool wcseq(const wchar* s, const wchar* s1) - NULL safe wide string comparison function
  inline bool streqi(const char* s, const char* s1) - the same, but case independent
  inline bool wcseqi(const wchar* s, const wchar* s1) - the same, but case independent

  w2a - helper object for const wchar_t* to const char* conversion
  a2w - helper object for const char* to const wchar_t* conversion
  w2utf - helper object for const wchar_t* to utf8 conversion
  utf2w - helper object for utf8 to const wchar_t* conversion

  t2w - const TCHAR* to const wchar_t* conversion, #definition
  w2t - const wchar_t* to const TCHAR* conversion, #definition

  itoa, itow - int to const char* converter
  atoi, wtoi - const char* to int converter (parser)
  ftoa, ftow - double to const char* converter

  
 */

#pragma once

#include <assert.h>
#include <wchar.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>

// disable that warnings in VC 2005

#pragma warning( push )

  #pragma warning(disable:4786) //identifier was truncated...
  #pragma warning(disable:4996) //'strcpy' was declared deprecated
  #pragma warning(disable:4100) //unreferenced formal parameter 

#ifndef byte
  typedef unsigned char   byte;
#endif

//#include "aux-slice.h"

// WARNING: macros below must be used only for passing parameters to functions!

/*
#if !defined(W2A) // wchar to multi-byte string converter (current locale)
#define W2A aux::w2a
#endif

#if !defined(A2W) // multi-byte to wchar string converter (current locale)
#define A2W aux::a2w
#endif

#if !defined(UTF2W) // utf-8 to wchar string converter
#define UTF2W aux::utf2w
#endif

#if !defined(W2UTF) // wchar to utf-8 string converter
#define W2UTF aux::w2utf
#endif

#if !defined(W2T) 
#if !defined(UNICODE) 
#define W2T(S) aux::w2a(S)
#else
#define W2T(S) (S)
#endif
#endif

#if !defined(T2W) 
#if !defined(UNICODE) 
#define T2W(S) aux::a2w(S)
#else
#define T2W(S) (S)
#endif
#endif

#if !defined(A2T) 
#if !defined(UNICODE) 
#define A2T(S) (S)
#else
#define A2T(S) aux::a2w(S)
#endif
#endif

#if !defined(T2A) 
#if !defined(UNICODE) 
#define T2A(S) (S)
#else
#define T2A(S) aux::w2a(S)
#endif
#endif

*/

#ifdef UNICODE
#define a2t( S ) aux::a2w(S)
#define t2a( S ) aux::w2a(S)
#define w2t( S ) (S)
#define t2w( S ) (S)
#define t2i( S ) aux::wtoi(S,0)
#define i2t( S ) aux::itow(S)
#else
#define a2t( S ) (S)
#define t2a( S ) (S)
#define w2t( S ) aux::w2a(S)
#define t2w( S ) aux::a2w(S)
#define t2i( S ) aux::atoi(S,0)
#define i2t( S ) aux::itoa(S)
#endif

#define w2u( S ) aux::w2utf(S)
#define u2w( S ) aux::utf2w(S)

#define i2a( I ) aux::itoa(I)
#define i2w( I ) aux::itow(I)

#define a2i( S ) aux::atoi(S,0)
#define w2i( S ) aux::wtoi(S,0)


inline void* zalloc ( size_t sz)
{
    void* p = malloc(sz);
    if(p) memset(p,0,sz);
    return p;
}

//elements in array literal
#define items_in(a) (sizeof(a)/sizeof(a[0]))
//chars in sting literal
#define chars_in(s) (sizeof(s) / sizeof(s[0]) - 1)

/**pod namespace - POD primitives. **/
namespace pod
{
  template <typename T> void copy ( T* dst, const T* src, size_t nelements)
  {
      memcpy(dst,src,nelements*sizeof(T));
  }
  template <typename T> void move ( T* dst, const T* src, size_t nelements)
  {
      memmove(dst,src,nelements*sizeof(T));
  }

  /** buffer  - in-memory dynamic buffer implementation. **/
  template <typename T>
    class buffer 
    {
      T*              _body;
      size_t          _allocated;
      size_t          _size;

      T*  reserve(size_t size)
      {
        size_t newsize = _size + size;
        if( newsize > _allocated ) 
        {
          _allocated = (_allocated * 3) / 2;
          if(_allocated < newsize) _allocated = newsize;
          T *newbody = new T[_allocated];
          copy(newbody,_body,_size);
          delete[] _body;
          _body = newbody;
        }
        return _body + _size;
      }

    public:

      buffer():_size(0)        { _body = new T[_allocated = 256]; }
      ~buffer()                { delete[] _body;  }

      const T * data()   
      {  
               if(_size == _allocated) reserve(1); 
               _body[_size] = 0; return _body; 
      }

      size_t length() const         { return _size; }

      void push(T c)                { *reserve(1) = c; ++_size; }
      void push(const T *pc, size_t sz) { copy(reserve(sz),pc,sz); _size += sz; }

      void clear()                  { _size = 0; }

    };

    typedef buffer<byte> byte_buffer; 
    typedef buffer<wchar_t> wchar_buffer; 
    typedef buffer<char> char_buffer; 
}

namespace utf8 
{ 
  // convert utf8 code unit sequence to wchar_t sequence

  inline bool towcs(const byte *utf8, size_t length, pod::wchar_buffer& outbuf)
  {
    if(!utf8 || length == 0) return true;
    const byte* pc = (const byte*)utf8;
    const byte* last = pc + length;
    unsigned int b;
    unsigned int num_errors = 0;
    while (pc < last) 
    {
      b = *pc++;

      if( !b ) break; // 0 - is eos in all utf encodings

      if ((b & 0x80) == 0)
      {
        // 1-byte sequence: 000000000xxxxxxx = 0xxxxxxx
        ;
      } 
      else if ((b & 0xe0) == 0xc0) 
      {
        // 2-byte sequence: 00000yyyyyxxxxxx = 110yyyyy 10xxxxxx
        if(pc == last) { outbuf.push('?'); ++num_errors; break; }
        b = (b & 0x1f) << 6;
        b |= (*pc++ & 0x3f);
      } 
      else if ((b & 0xf0) == 0xe0) 
      {
        // 3-byte sequence: zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx
        if(pc >= last - 1) { outbuf.push('?'); ++num_errors; break; }
        
        b = (b & 0x0f) << 12;
        b |= (*pc++ & 0x3f) << 6;
        b |= (*pc++ & 0x3f);
        if(b == 0xFEFF &&
           outbuf.length() == 0) // bom at start
             continue; // skip it
      } 
      else if ((b & 0xf8) == 0xf0) 
      {
        // 4-byte sequence: 11101110wwwwzzzzyy + 110111yyyyxxxxxx = 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
        if(pc >= last - 2) { outbuf.push('?'); break; }

        b = (b & 0x07) << 18;
        b |= (*pc++ & 0x3f) << 12;
        b |= (*pc++ & 0x3f) << 6;
        b |= (*pc++ & 0x3f);
        // b shall contain now full 21-bit unicode code point.
        assert((b & 0x1fffff) == b);
        if((b & 0x1fffff) != b)
        {
          outbuf.push('?');
          ++num_errors;
          continue;
        }
//#pragma warning( suppress:4127 ) //  warning C4127: conditional expression is constant
        if( sizeof(wchar_t) == 2 ) // Seems like Windows, wchar_t is utf16 code units sequence there.
        {
          outbuf.push( wchar_t(0xd7c0 + (b >> 10)) );
          outbuf.push( wchar_t(0xdc00 | (b & 0x3ff)) );
        }
//#pragma warning( suppress:4127 ) //  warning C4127: conditional expression is constant
        else if( sizeof(wchar_t) >= 4 ) // wchar_t is full ucs-4 (21-bit)
        {
          outbuf.push( wchar_t(b) );
        }
        else
        {
          assert(0); // what? wchar_t is single byte here?
        }
      } 
      else 
      {
        assert(0); //bad start for UTF-8 multi-byte sequence"
        ++num_errors;
        b = '?';
      }
      outbuf.push( wchar_t(b) );
    }
    return num_errors == 0;
  }

  inline bool fromwcs(const wchar_t* wcs, size_t length, pod::byte_buffer& outbuf)
  {
    const wchar_t *pc = wcs;
    const wchar_t *end = pc + length;
    unsigned int  num_errors = 0;
    for(unsigned int c = *pc; pc < end ; c = *(++pc)) 
    {
      if (c < (1 << 7)) 
      {
        outbuf.push(byte(c));
      } 
      else if (c < (1 << 11)) 
      {
        outbuf.push(byte((c >> 6) | 0xc0));
        outbuf.push(byte((c & 0x3f) | 0x80));
      } 
      else if (c < (1 << 16)) 
      {
        outbuf.push(byte((c >> 12) | 0xe0));
        outbuf.push(byte(((c >> 6) & 0x3f) | 0x80));
        outbuf.push(byte((c & 0x3f) | 0x80));
      } 
      else if (c < (1 << 21)) 
      {
        outbuf.push(byte((c >> 18) | 0xf0));
        outbuf.push(byte(((c >> 12) & 0x3f) | 0x80));
        outbuf.push(byte(((c >> 6) & 0x3f) | 0x80));
        outbuf.push(byte((c & 0x3f) | 0x80));
      }
      else 
        ++num_errors;
    }
    return num_errors == 0;
  }

  // UTF8 stream

  // class T must have two methods:
  //   void push(unsigned char c)
  //   void push(const unsigned char *pc, size_t sz)
  
  // bool X - true - XML markup character conversion (characters '<','>',etc).
  //          false - no conversion at all. 

  template <class T, bool X = true>
  class ostream_t : public T
  {
  public:
    ostream_t()
    { 
      // utf8 byte order mark
      static unsigned char BOM[] = { 0xEF, 0xBB, 0xBF };
      T::push(BOM, sizeof(BOM));
    }

    // intended to handle only ascii-7 strings
    // use this for markup output 
    ostream_t& operator << (const char* str) 
    { 
      T::push((const unsigned char*)str,strlen(str)); return *this; 
    }

    ostream_t& operator << (char c) 
    { 
      T::push((unsigned char)c); return *this; 
    }

    // use UNICODE chars for value output
    ostream_t& operator << (const wchar_t* wstr)
    {
      const wchar_t *pc = wstr;
      for(unsigned int c = *pc; c ; c = *(++pc)) 
      {
        if(X)
          switch(c) 
          {
              case '<': *this << "&lt;"; continue;
              case '>': *this << "&gt;"; continue;
              case '&': *this << "&amp;"; continue;
              case '"': *this << "&quot;"; continue;
              case '\'': *this << "&apos;"; continue;
          }
        if (c < (1 << 7)) 
        {
         T::push (byte(c));
        } 
        else if (c < (1 << 11)) {
         T::push (byte((c >> 6) | 0xc0));
         T::push (byte((c & 0x3f) | 0x80));
        } 
        else if (c < (1 << 16)) {
         T::push (byte((c >> 12) | 0xe0));
         T::push (byte(((c >> 6) & 0x3f) | 0x80));
         T::push (byte((c & 0x3f) | 0x80));
        } 
        else if (c < (1 << 21)) 
        {
         T::push (byte((c >> 18) | 0xf0));
         T::push (byte(((c >> 12) & 0x3f) | 0x80));
         T::push (byte(((c >> 6) & 0x3f) | 0x80));
         T::push (byte((c & 0x3f) | 0x80));
        }
      }
      return *this;
    }
    ostream_t& operator << (const std::wstring& str)
    {
      return *this << (str.c_str());
    }

  };

  // raw ASCII/UNICODE -> UTF8 converter 
  typedef ostream_t<pod::byte_buffer,false> ostream;
  // ASCII/UNICODE -> UTF8 converter with XML support
  typedef ostream_t<pod::byte_buffer,true> oxstream;


} // namespace utf8

namespace aux 
{
  template <typename T> struct slice;

  template <class T>
  inline T
    limit ( T v, T minv, T maxv )
  {
    assert(minv < maxv);
    if (minv >= maxv)
      return minv;
    if (v > maxv) return maxv;
    if (v < minv) return minv;
    return v;
  }

  // safe string comparison
  inline bool streq(const char* s, const char* s1)
  {
    if( s && s1 )
      return strcmp(s,s1) == 0;
    return false;
  }

  // safe wide string comparison
  inline bool wcseq(const wchar_t* s, const wchar_t* s1)
  {
    if( s && s1 )
      return wcscmp(s,s1) == 0;
    return false;
  }

  // safe case independent string comparison
  inline bool streqi(const char* s, const char* s1)
  {
    if( s && s1 )
      return _stricmp(s,s1) == 0;
    return false;
  }

  // safe case independent wide string comparison
  inline bool wcseqi(const wchar_t* s, const wchar_t* s1)
  {
    if( s && s1 )
      return wcsicmp(s,s1) == 0;
    return false;
  }

  // helper convertor objects wchar_t to ACP and vice versa
  class w2a 
  {
    char  local[16];
    char* buffer;
    unsigned int n;

    void init(const wchar_t* wstr, unsigned int nu)
      {
      n = WideCharToMultiByte(CP_ACP,0,wstr,nu,0,0,0,0);
      buffer = (n < (16-1))? local:new char[n+1];
        WideCharToMultiByte(CP_ACP,0,wstr,nu,buffer,n,0,0);
        buffer[n] = 0;
      }
  public:
    explicit w2a(const wchar_t* wstr):buffer(0),n(0)
      {
      if(wstr)
        init(wstr,(unsigned int)wcslen(wstr));
    }
    explicit w2a(const std::wstring& wstr):buffer(0),n(0)
    { 
      init(wstr.c_str(),(unsigned int)wstr.length());
    }
    explicit w2a(slice<wchar_t> s);

    ~w2a() { if(buffer != local) delete[] buffer;  }
    unsigned int length() const { return n; }    
    operator const char*() { return buffer; }
  };

  class a2w 
  {
    wchar_t  local[16];
    wchar_t* buffer;
    unsigned int nu;
    void init(const char* str, unsigned int n)
      {
      nu = MultiByteToWideChar(CP_THREAD_ACP,0,str,n,0,0);
      buffer = ( nu < (16-1) )? local: new wchar_t[nu+1];
        MultiByteToWideChar(CP_ACP,0,str,n,buffer,nu);
        buffer[nu] = 0;
      }
  public:
    explicit a2w(const char* str):buffer(0), nu(0)
      {
      if(str)
        init(str, (unsigned int)strlen(str) );
    }
    explicit a2w(slice<char> s);
    ~a2w() {  if(buffer != local) delete[] buffer;  }
    unsigned int length() const { return nu; }
    operator const wchar_t*() { return buffer; }
  };

  // helper convertor objects wchar_t to utf8 and vice versa
  class utf2w 
  {
    pod::wchar_buffer buffer;
  public:
    explicit utf2w(const byte* utf8, size_t length = 0)
    { 
      if(utf8)
      {
        if( length == 0) length = strlen((const char*)utf8);
        utf8::towcs(utf8, length ,buffer);
      }
    }
    explicit utf2w(const char* utf8, size_t length = 0)
    { 
      if(utf8)
      {
        if( length == 0) length = strlen(utf8);
        utf8::towcs((const byte*)utf8, length ,buffer);
      }
    }
    ~utf2w() {}

    operator const wchar_t*() { return buffer.data(); }
    unsigned int length() const { return (unsigned int)buffer.length(); }

    pod::wchar_buffer& get_buffer() { return buffer; }

  };

  class w2utf 
  {
    pod::byte_buffer buffer;
  public:
    explicit w2utf(const wchar_t* wstr)
    { 
      if(wstr)
      {
        size_t nu = wcslen(wstr);
        utf8::fromwcs(wstr,nu,buffer);
      }
    }
    explicit w2utf(const std::wstring& str)
    {
      utf8::fromwcs(str.c_str(),str.length(),buffer);
    }
    ~w2utf() {}
    operator const byte*() { return buffer.data(); }
    operator const char*() { return (const char*)buffer.data(); }
    unsigned int length() const { return (unsigned int)buffer.length(); }
  };


  /** Integer to string converter.
      Use it as ostream << itoa(234) 
  **/
  class itoa 
  {
    char buffer[38];
  public:
    itoa(int n, int radix = 10)
    { 
      _itoa(n,buffer,radix);
    }
    operator const char*() { return buffer; }
  };

  /** Integer to wstring converter.
      Use it as wostream << itow(234) 
  **/

  class itow 
  {
    wchar_t buffer[38];
  public:
    itow(int n, int radix = 10)
    { 
      _itow(n,buffer,radix);
    }
    operator const wchar_t*() { return buffer; }
  };

  /** Float to string converter.
      Use it as ostream << ftoa(234.1); or
      Use it as ostream << ftoa(234.1,"pt"); or
  **/
  class ftoa 
  {
    char buffer[64];
  public:
    ftoa(double d, const char* units = "", int fractional_digits = 1)
    { 
      _snprintf(buffer, 64, "%.*f%s", fractional_digits, d, units );
      buffer[63] = 0;
    }
    operator const char*() { return buffer; }
  };

  /** Float to wstring converter.
      Use it as wostream << ftow(234.1); or
      Use it as wostream << ftow(234.1,"pt"); or
  **/
  class ftow
  {
    wchar_t buffer[64];
  public:
    ftow(double d, const wchar_t* units = L"", int fractional_digits = 1)
    { 
      _snwprintf(buffer, 64, L"%.*f%s", fractional_digits, d, units );
      buffer[63] = 0;
    }
    operator const wchar_t*() { return buffer; }
  };

 /** wstring to integer parser.
  **/
  inline int wtoi(const wchar_t *s, int default_value = 0) 
  { 
    if( !s ) return default_value;
    wchar_t *lastptr;
    long i = wcstol( s, &lastptr, 10 );
    return (lastptr != s)? (int)i : default_value;
  }

/** string to integer parser.
  **/
   inline int atoi(const char *s, int default_value = 0) 
  { 
    if( !s ) return default_value;
    char *lastptr;
    long i = strtol( s, &lastptr, 10 );
    return (lastptr != s)? (int)i : default_value;
  }

  // class T must have two methods:
  //   void push(wchar_t c)
  //   void push(const wchar_t *pc, size_t sz)
  template <class T>
  class ostream_t : public T
  {
  public:
    ostream_t() {}

    // intended to handle only ascii-7 strings
    // use this for markup output 
    ostream_t& operator << (const char* str) 
    { 
      if(!str) return *this;
      while( *str ) T::push(*str++); 
      return *this; 
    }

    ostream_t& operator << (char c) 
    { 
      T::push(c); return *this; 
    }

    // intended to handle only ascii-7 strings
    // use this for markup output 
    ostream_t& operator << (const wchar_t* str) 
    { 
      if(!str || !str[0]) return *this;
      T::push(str,wcslen(str)); return *this; 
    }

    ostream_t& operator << (wchar_t c) 
    { 
      T::push(c); return *this; 
    }
  };

  // wostream - a.k.a. wstring builder - buffer for dynamic composition of wchar_t strings 
  typedef ostream_t<pod::wchar_buffer> wostream;
}

#pragma warning( pop )

#endif
