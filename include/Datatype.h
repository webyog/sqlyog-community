/* Copyright (C) 2013 Webyog Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA

*/

/*
    All basic datatypes are typedef to project datatypes.

    This definition take care of both LINUX and WINDOWS.

    Instead of using primitive datatypes will be using project datatypes through out the source code.

*/

#ifndef _DATATYPE_H_
#define _DATATYPE_H_


#ifdef _WIN32
    
    #include <winsock.h>
	#include <mysql.h>
#else
       #include <limits.h>
	#include <mysql/mysql.h>
#endif

#pragma warning(disable:4100)       // format parameter not USED
#pragma warning(disable:4267)       // conversion from 'x' to 'y', possible loss of data
#pragma warning(disable:4127)       // conditional expression is constant
#pragma warning(disable:4706)       // assignment within conditional expression
#pragma warning(disable:4244)       // conversion from 'X' to 'Y', possible loss of data
#pragma warning(disable:4312)       // conversion from 'X' to 'Y' of greater size
#pragma warning(disable:4311)       // pointer truncation from 'X' to 'Y'
#pragma warning(disable:4018)       // '<' : signed/unsigned mismatch
#pragma warning(disable:249)        // '==' : signed/unsigned mismatch
#pragma warning(disable:4389)       // '!=' : signed/unsigned mismatch
#pragma warning(disable:4800)       // forcing value to bool 'true' or 'false' 

/*! MYSQL** pointer to MySQL pointer*/ 
typedef MYSQL** PMYSQL;

/*! unsigned int 32 bit */ 
typedef     unsigned int            wyUInt32;

typedef    unsigned long			wyULong;

/*! signed int 32 bit */ 
typedef     int                     wyInt32;

//typedef     long long				wyLong;

/*! unsigned short */
typedef wchar_t						wyWChar;

#ifdef _WIN32
typedef     __int64                 wyInt64;

/*!double */
typedef     double                  wyDouble;

/*! unsigned char */
typedef		unsigned char			wyUChar;

/*! signed char */ 
typedef		char					wyChar;

/*! unsigned char */ 
//typedef		char					wyUChar;
#else
typedef     long long int           wyInt64;
/*! signed char */ 
typedef		char					wyChar;

/*! unsigned char */
typedef		unsigned char			wyUChar;
#endif

/**
   * Wrapper over the primitive bool datatype.
   * The basic idea is that the primitive type can be defined as TRUE or 1.
   * This makes the return values ambiguous. Having our own enum value will make things more explicit and portable.
*/
enum wyBool
{
    wyTrue = 1,/**< wyTrue, Equivalent to TRUE. */  
    wyFalse = 0/**< wyFalse, Equivalent to FALSE. */  
};

#ifndef _WIN32
   extern int stricmp(const char * arg1, const char * arg2);
   extern my_ulonglong _atoi64(const char * nptr);
   extern int strnicmp(const char * arg1, const char * arg2, int count);
#endif

#ifndef _WIN32

    #define strcmp strcmp
    #define _strcmpi stricmp
    #define MAX_PATH PATH_MAX
    #define DWORD unsigned long
    #define _vsntprintf vsnprintf
    #define VOID void
    #define TRUE true
    #define FALSE false
    #define CHAR char
    #define INT int
    #define CONST const
    #define BOOL  bool
    #define LONG  long
    #define SHORT int
    #define ZeroMemory bzero
        
#endif         

#endif
