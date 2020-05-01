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

#ifndef __APPINFO__
#define __APPINFO__

#include "Version.h"

#define STR(X)                                      #X
#define STRINGIZE(X)                                STR(X)

#define MAJOR_VERSION								STRINGIZE(MAJOR_VERSION_INT)
#define MINOR_VERSION								STRINGIZE(MINOR_VERSION_INT)
#define UPDATE_VERSION								STRINGIZE(UPDATE_VERSION_INT)

#if defined(__LP64__) || defined(_WIN64)
#define PLATFORM_STRING                             " (64 bit)"
#else
#define PLATFORM_STRING                             " (32 bit)"
#endif

#define	APPVERSION									"v" MAJOR_VERSION "." MINOR_VERSION "." UPDATE_VERSION EXTRAINFO PLATFORM_STRING

#define	SQLITE_APPVERSION_MAJOR						"10"
#define	SQLITE_APPVERSION_MINOR						"5"
#define	COMPANY_COPYRIGHT							"(c) 2001-2020 Webyog Inc."

#define FILEVER                                     MAJOR_VERSION_INT ## , ## MINOR_VERSION_INT ## , ## UPDATE_VERSION_INT ## , ## RELEASE_VERSION_INT
#define STRFILEVER                                  MAJOR_VERSION "." MINOR_VERSION "." STRINGIZE(UPDATE_VERSION_INT) "." STRINGIZE(RELEASE_VERSION_INT)

#ifdef WIN64
#define FILEDESCSTR									"SQLyog - MySQL GUI - 64 bit"
#else
#define FILEDESCSTR									"SQLyog - MySQL GUI"
#endif


#endif

