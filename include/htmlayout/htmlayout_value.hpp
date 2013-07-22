/*
 * Terra Informatica Lightweight Embeddable HTMLayout control
 * http://terrainformatica.com/htmlayout
 * 
 * HTMLayout value class. 
 * 
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 * 
 * (C) 2003-2004, Andrew Fedoniouk (andrew@terrainformatica.com)
 */

/**\file
 * \brief value, aka variant, aka discriminated union
 **/

#ifndef __htmlayout_value_hpp__
#define __htmlayout_value_hpp__

#pragma warning(disable:4786) //identifier was truncated...

#include <assert.h>

#include "value.h"

#include <map>
#include <string>
#include <vector>

// DISCLAIMER: this code is using primitives from std:: namespace probably 
// in the not efficient manner. (I am not using std:: anywhere beside of this file)
// Feel free to change it. Would be nice if you will drop me your updates.

#pragma once


namespace json
{
  inline value parse( aux::wchars text, bool open_mode = false )
  {
    value v;
    UINT numcp = HTMLayoutParseValue(text.start, text.length, open_mode?1:0, &v);
#ifdef _DEBUG
    if( numcp )
    {
      aux::wchars problem = text;
      problem.prune(text.length - numcp);
      // problem.start is where parsing error was encountered 
    }
#else
    numcp;
#endif 
    return v;
  }
  

}

#endif