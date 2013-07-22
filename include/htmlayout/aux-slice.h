#ifndef __json_slice_h__
#define __json_slice_h__

/*
 * Terra Informatica Sciter and HTMLayout Engines
 * http://terrainformatica.com/sciter, http://terrainformatica.com/htmlayout 
 * 
 * slice - range of elements, start/length. That is what is known in D as array.
 * 
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 * 
 * (C) 2003-2006, Andrew Fedoniouk (andrew@terrainformatica.com)
 */

/**\file
 * \brief range of elements
 **/

#pragma warning( push )

// disable that warnings in VC 2005
#pragma warning(disable:4786) //identifier was truncated...
#pragma warning(disable:4996) //'strcpy' was declared deprecated
#pragma warning(disable:4100) //unreferenced formal parameter 

#include "aux-cvt.h"
#include "limits.h"

namespace aux 
{

template <typename T >
   struct slice
   {
      const T* start;
      unsigned int   length;

      slice(): start(0), length(0) {}
      slice(const T* start_, unsigned int length_) { start = start_; length = length_; }


      slice(const slice& src): start(src.start), length(src.length) {}
      slice(const T* start_, const T* end_): start(start_), length( max(end_-start_,0)) {}

      slice& operator = (const slice& src) { start = src.start; length = src.length; return *this; }

      const T*      end() const { return start + length; }

      bool operator == ( const slice& r ) const
      {
        if( length != r.length )
          return false;
        if( start == r.start )
          return true;
        for( unsigned int i = 0; i < length; ++i )
          if( start[i] != r.start[i] )
            return false;
        return true;
      }

      bool operator != ( const slice& r ) const { return !operator==(r); }

      T operator[] ( unsigned int idx ) const
      {
        assert( idx < length );
        if(idx < length)
          return start[idx];
        return 0;
      }

      T last() const
      {
        if(length)
          return start[length-1];
        return 0;
      }

      // [idx1..length)
      slice operator() ( unsigned int idx1 ) const
      {
        assert( idx1 < length );
        if ( idx1 < length )
            return slice( start + idx1, length - idx1 );
        return slice();
      }
      // [idx1..idx2)
      slice operator() ( unsigned int idx1, unsigned int idx2 ) const
      {
        assert( idx1 < length );
        assert( idx2 <= length );
        assert( idx1 <= idx2 );
        if ( idx1 < idx2 )
            return slice( start + idx1, idx2 - idx1 );
        return slice();
      }

      int index_of( T e ) const
      {
        for( unsigned int i = 0; i < length; ++i ) if( start[i] == e ) return i;
        return -1;
      }

      int last_index_of( T e ) const
      {
        for( unsigned int i = length; i > 0 ;) if( start[--i] == e ) return i;
        return -1;
      }

      int index_of( const slice& s ) const
      {
        if( s.length > length ) return -1;
        if( s.length == 0 ) return -1;
        unsigned int l = length - s.length;
        for( unsigned int i = 0; i < l ; ++i)
          if( start[i] == *s.start )
          {
            const T* p = s.start;
            unsigned int last = i + s.length;
            for( unsigned int j = i + 1; j < last; ++j )
              if( *(++p) != start[j])
                goto next_i;
            return i;
            next_i: continue;
          }
        return -1;
      }

      int last_index_of( const slice& s ) const
      {
        if( s.length > length ) return -1;
        if( s.length == 0 ) return -1;
        const T* ps = s.end() - 1;
        for( unsigned int i = length; i > 0 ; )
          if( start[--i] == *ps )
          {
            const T* p = ps;
            unsigned int j, first = i - s.length + 1;
            for( j = i; j > first; )
              if( *(--p) != start[--j])
                goto next_i;
            return j;
            next_i: continue;
          }
        return -1;
      }

      void prune(unsigned int from_start, unsigned int from_end = 0)
      {
        unsigned int s = from_start >= length? length : from_start; 
        unsigned int e = length - (from_end >= length? length: from_end);
        start += s;
        if( s < e ) length = e-s; 
        else length = 0;
      }
   

      bool like(const T* pattern) const;

   };

  #define MAKE_SLICE( T, D ) slice<T>(D, sizeof(D) / sizeof(D[0]))

  #ifdef _DEBUG

  inline void slice_unittest()
  {
    int v1[] = { 0,1,2,3,4,5,6,7,8,9 };
    int v2[] = { 3,4,5 };
    int v3[] = { 0,1,2 };
    int v4[] = { 0,1,2,4 };
    int v5[] = { 1,1,2,3 };

    slice<int> s1 = MAKE_SLICE( int, v1 );
    slice<int> s2 = MAKE_SLICE( int, v2 );
    slice<int> s3 = MAKE_SLICE( int, v3 );
    slice<int> s4 = MAKE_SLICE( int, v4 );
    slice<int> s5 = MAKE_SLICE( int, v5 );

    assert( s1 != s2 );
    assert( s1(3,6) == s2 );
    assert( s1.index_of(3) == 3 );
    assert( s1.index_of(s2) == 3 );
    assert( s1.last_index_of(3) == 3 );
    assert( s1.last_index_of(s2) == 3 );

    assert( s1.index_of(s3) == 0 );
    assert( s1.last_index_of(s3) == 0 );

    assert( s1.index_of(s4) == -1 );
    assert( s1.last_index_of(s4) == -1 );

    assert( s1.index_of(s5) == -1 );
    assert( s1.last_index_of(s5) == -1 );

  }

  #endif

  template <typename CS>
    inline slice<CS> trim(slice<CS> str)
  {
    for( unsigned i = 0; i < str.length; ++i )
      if( isspace(str[0]) ) { ++str.start; --str.length; }
      else break;
    for( unsigned j = str.length - 1; j >= 0; --j )
      if( isspace(str[j]) ) --str.length;
      else break;
    return str;
  }

  typedef slice<char>    chars;
  typedef slice<wchar_t> wchars;
  typedef slice<byte>    bytes;

  inline a2w::a2w(slice<char> s):buffer(0), nu(0)
  {
     init(s.start,s.length);
  }
  inline w2a::w2a(slice<wchar_t> s):buffer(0), n(0)
  {
     init(s.start,s.length);
  }


  // Note: CS here is a string literal!
  #define __WTEXT(quote) L##quote     
  #define _WTEXT(quote) __WTEXT(quote)   

  // Note: CS here is a string literal!
  #define const_chars(CS) aux::slice<char>(CS,chars_in(CS))
  #define const_wchars(CS) aux::slice<wchar_t>(_WTEXT(CS),chars_in(_WTEXT(CS)))

  inline wchars  chars_of( const wchar_t *t ) {  return t? wchars(t,(unsigned int)wcslen(t)):wchars(); }
  inline chars   chars_of( const char *t ) {  return t? chars(t,(unsigned int)strlen(t)):chars(); }

   // simple tokenizer
  template <typename T >
      class tokens
      {
        const T* delimeters;
        const T* p;
        const T* tail;
        const T* start;
        const T* end;
        const bool  is_delimeter(T el)  { for(const T* t = delimeters;t && *t; ++t) if(el == *t) return true;  return false; }
        const T*    tok()               { for(;p < tail; ++p) if(is_delimeter(*p)) return p++; return p; }
      public:

        tokens(const T *text, size_t text_length, const T* separators): delimeters(separators)
        {
          start = p = text;
          tail = p + text_length;
          end = tok();
        }

        tokens(const aux::slice<T> s, const T* separators): delimeters(separators)
        {
          start = p = s.start;
          tail = p + s.length;
          end = tok();
        }

        bool next(slice<T>& v)
        {
          if(start < tail)
          {
            v.start = start;
            v.length = unsigned(end - start);
            start = p;
            end = tok();
            return true;
          }
          return false;
        }
      };

  typedef tokens<char> atokens;
  typedef tokens<wchar_t> wtokens;


    /****************************************************************************/
    //
    // idea was taken from Konstantin Knizhnik's FastDB
    // see http://www.garret.ru/
    // extended by [] operations
    //

  template <typename CT, CT sep = '-', CT end = ']' >
    struct charset
    {
      #define SET_SIZE (1 << (sizeof(CT) * CHAR_BIT))
      #define SIGNIFICANT_BITS_MASK unsigned( SET_SIZE - 1 ) 
      
      unsigned char codes[ SET_SIZE / CHAR_BIT ];

      unsigned charcode(CT c) // proper unsigned_cast
      {
        return SIGNIFICANT_BITS_MASK & unsigned(c);
      }
      
    private:  
      void set ( CT from, CT to, bool v )    
      {
         for ( unsigned i = charcode(from); i <= charcode(to); ++i )
         {
           unsigned int bit = i & 7;
           unsigned int octet = i >> 3;
           if( v ) codes[octet] |= 1 << bit; else codes[octet] &= ~(1 << bit);
         }
      } 
      void init ( unsigned char v )  { memset(codes,v,(SET_SIZE >> 3)); }
    public:

      void parse ( const CT* &pp )
      {
        //assert( sizeof(codes) == sizeof(CT) * sizeof(bool));
        const CT *p = (const CT *) pp;
        unsigned char inv = *p == '^'? unsigned char(0xff):unsigned char(0);
        if ( inv ) { ++p; }
        init ( inv );
        if ( *p == sep ) set(sep,sep,inv == 0);
        while ( *p )
        {
          if ( p[0] == end ) { p++; break; }
          if ( p[1] == sep && p[2] != 0 ) { set (p[0], p[2], inv == 0 );  p += 3; }
          else { CT t = *p++; set(t,t, inv == 0); }
        }
        pp = (const CT *) p;
      }

      bool valid ( CT c )
      {
        unsigned int bit = charcode(c) & 7;
        unsigned int octet = charcode(c) >> 3;
        return (codes[octet] & (1 << bit)) != 0;
      } 
      #undef SET_SIZE
      #undef SIGNIFICANT_BITS_MASK
    };

  template <typename CT >
    inline int match ( slice<CT> cr, const CT *pattern )
    {
      const CT AnySubstring = '*';
      const CT AnyOneChar = '?';
      const CT AnyOneDigit = '#';

      const CT    *str = cr.start;
      const CT    *wildcard = 0;
      const CT    *strpos = 0;
      const CT    *matchpos = 0;

      charset<CT> cset;

      for (;;)
      {
        if ( *pattern == AnySubstring )
        {
          wildcard = ++pattern;
          strpos = str;
          if ( !matchpos ) matchpos = str;
        }
        else if ( *str == '\0' || str >= cr.end() )
        {
          return ( *pattern == '\0' ) ? int( matchpos - cr.start ) : -1;
        }
        else if ( *pattern == '[' )
        {
          pattern++;
          cset.parse ( pattern );
          if ( !cset.valid ( *str ) )
            return -1;
          if ( !matchpos )
            matchpos = str;
          str += 1;
        }
        else if ( *str == *pattern || *pattern == AnyOneChar )
        {
          if ( !matchpos ) matchpos = str;
          str += 1;
          pattern += 1;
        }
        else if ( *str == *pattern || *pattern == AnyOneDigit )
        {
          if ( !isdigit(*str )) return -1;
          if ( !matchpos ) matchpos = str;
          str += 1;
          pattern += 1;
        }
        else if ( wildcard )
        {
          str = ++strpos;
          pattern = wildcard;
        }
        else
          break;
      }
      return -1;
    }

  template <typename T >
    inline bool slice<T>::like ( const T *pattern ) const
    {
      return match<T>(*this,pattern) >= 0;
    }

  // chars to unsigned int
  // chars to int

  template <typename T>
      inline unsigned int to_uint(slice<T>& span, unsigned int base = 10)
  {
     unsigned int result = 0,value;
     const T *cp = span.start;
     const T *pend = span.end();

     while ( cp < pend && isspace(*cp) ) ++cp;

     if (!base)
     {
         base = 10;
         if (*cp == '0') {
             base = 8;
             cp++;
             if ((toupper(*cp) == 'X') && isxdigit(cp[1])) {
                     cp++;
                     base = 16;
             }
         }
     }
     else if (base == 16)
     {
         if (cp[0] == '0' && toupper(cp[1]) == 'X')
             cp += 2;
     }
     while ( cp < pend && isxdigit(*cp) &&
            (value = isdigit(*cp) ? *cp-'0' : toupper(*cp)-'A'+10) < base) {
             result = result*base + value;
             cp++;
     }
     span.length = (unsigned int)(cp - span.start);
     return result;
  }

  template <typename T>
      int to_int(slice<T>& span, unsigned int base = 10)
  {

     while (span.length > 0 && isspace(span[0]) ) { ++span.start; --span.length; }
     if(span[0] == '-')
     {
        ++span.start; --span.length;
        return - int(to_uint(span,base));
     }
     return to_uint(span,base);
  }

}

#pragma warning( pop )

#endif