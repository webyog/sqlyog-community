#ifndef __HTMENGINE_AUX_H__
#define __HTMENGINE_AUX_H__


#pragma warning( push )

// disable that warnings in VC 2005
  #pragma warning(disable:4786) //identifier was truncated...
  #pragma warning(disable:4996) //'strcpy' was declared deprecated
  #pragma warning(disable:4100) //unreferenced formal parameter 

#include <stdio.h>
#include <stdlib.h>
#include <string>

typedef unsigned int uint;

#include "aux-cvt.h"
#include "aux-slice.h"
#include "htmlayout.h"

//
// This file is a part of 
// Terra Informatica Lightweight Embeddable HTMEngine control SDK
// Created by Andrew Fedoniouk @ TerraInformatica.com
//

//
// Auxiliary classes and functions
//

#define SIZED_STRUCT( type_name, var_name ) type_name var_name; memset(&var_name,0,sizeof(var_name)); var_name.cbSize = sizeof(var_name)


namespace htmlayout
{

  inline std::string url_escape( const wchar_t* text, bool space_to_plus = false)
  {
    char buff[ 2048 ] = {0};
    unsigned len = HTMLayoutUrlEscape(text,space_to_plus,buff,2048);
    return std::string(buff,len);
  }

  inline std::wstring url_unescape( const char* text)
  {
    wchar_t buff[ 2048 ] = {0};
    unsigned len = HTMLayoutUrlUnescape(text,buff,2048);
    return std::wstring(buff,len);
  }

  struct color
  {
    unsigned char r,g,b,t;

    color()
      : r(0),g(0),b(0),t(0xff) {}

    color(unsigned int rgbt )
      : r(unsigned char(rgbt & 0xff)),g(unsigned char((rgbt & 0xff00) >> 8)),b(unsigned char((rgbt & 0xff0000) >> 16)),t(unsigned char((rgbt & 0xff000000) >> 24)) {}

    color(unsigned red, unsigned green, unsigned blue )
      : r(unsigned char(red)),g(unsigned char(green)),b(unsigned char(blue)),t(0) {}

    color(unsigned red, unsigned green, unsigned blue, unsigned transparency )
      : r(unsigned char(red)),g(unsigned char(green)),b(unsigned char(blue)),t(unsigned char(transparency)) {}
    color(const color& c): r(c.r),g(c.g),b(c.b),t(c.t) {}
    color& operator=(const color& c) { r = c.r; g = c.g; b = c.b; t = c.t; return *this; }
    
    bool transparent() const { return t == 0xff; }

    static color parse(aux::wchars text, color default_value = color())
    {
      if(text.length == 0) 
        return default_value;
      if(text[0] == '#') // #xxx, #xxxx, #xxxxxx, #xxxxxxxx
      {
        int R = 0, G = 0, B = 0, T = 0; 
        switch( text.length )
        {
          case 4: swscanf( text.start+1 ,L"%1x%1x%1x",&R,&G,&B); R <<= 4; G <<= 4; B <<= 4; break;
          case 5: swscanf( text.start+1 ,L"%1x%1x%1x%1x",&R,&G,&B,&T); R <<= 4; G <<= 4; B <<= 4; T <<= 4; break;
          case 7: swscanf( text.start+1 ,L"%2x%2x%2x",&R,&G,&B); break;
          case 9: swscanf( text.start+1 ,L"%2x%2x%2x%2x",&R,&G,&B,&T); break;
          default: return default_value;
        }
        return color(R,G,B,T);
      }
      else if( text.like(L"rgb(*)") ) // rgb(r,g,b), rgb(r,g,b,a)
      {
        text.start += 4; text.length -= 5;
        unsigned ca[4] = {0}; aux::wchars tok; aux::wtokens toks(text, L","); 
        for(int n = 0; n < 4; ++n)
        {
          if(!toks.next(tok)) break;
          ca[n] = aux::to_uint(tok);
          if( tok.start[tok.length] == '%' )  ca[n] = (ca[n] * 255) / 100;
        }
        return color( aux::limit(ca[0],0U,255U), aux::limit(ca[1],0U,255U), aux::limit(ca[2],0U,255U), aux::limit(ca[3],0U,255U));
      }
      return default_value;
    }
  };

}

/* other stuff moved to json-aux.h */

#pragma warning( pop )

#endif