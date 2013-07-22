/* Copyright (C) 2013 Webyog Inc

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


/* For documentation on LogMgmt class, please refer LogMgmt.h */

#include <stdio.h>
#include <string.h>
#ifdef _WIN32             /* ANSI compatible version          */
#include <stdarg.h>
#else                   /* UNIX compatible version          */
#include <varargs.h>
#endif

#include "LogMgmt.h"
#include "Datatype.h"

#define DEBUG_LOG_FILE "c:\\sqlyogdeb.log"

/** LogMgmt()
  *	@brief		Constructor for logger. Writes Enter function name to the log file
  **/

LogMgmt::LogMgmt ( const char * function )
{
	/* copy the function name as we will be using it to log EXIT function name also */
	strncpy ( m_function, function, 1024-1 );

	FILE	*fp=NULL;

	/* open the log file and check for error */
	fp = fopen ( DEBUG_LOG_FILE, "a+" );
	if ( fp )
	{
		/* log entering function */
		fprintf ( fp, "Enter %s\r\n", function );
		fclose ( fp );
	}
}

/** ~LogMgmt()
  *	@brief	Destructor for LogMgmt class. Writes exit function name which is kept as member variable.
  **/

LogMgmt::~LogMgmt ()
{
	FILE	*fp=NULL;

	/* open the log file and check for error */
	fp = fopen ( DEBUG_LOG_FILE, "a+" );
	if ( fp )
	{
		/* log exiting function */
		fprintf ( fp, "Exit %s\r\n", m_function );
		fclose ( fp );
	}
}

/** DEBUG_LOG()
  *	@brief	Log things in the file like sprintf
  **/

void DEBUG_LOG ( const char *format, ... )
{
#ifndef LOG_INFO
	return;	
#endif
#ifdef _DEBUG
	FILE	*fp;

	/* open the log file and check for error */
	fp = fopen ( DEBUG_LOG_FILE, "a+" );
	if ( !fp ) return;

	/* log entering function */
	va_list args;

	va_start ( args, format );
	vfprintf ( fp, format, args );
	va_end ( args );

	fprintf ( fp, "\r\n" );

	fclose ( fp );
#endif
}

/** DEBUG_LOGFILE()
  *	@brief	This logger function is called only once on start of the app. This copies the logging
  *			file name to the static variable LogMgmt::m_Filename. This allows for a new empty 
  *			file to be created for every session.
  *			This file is used to log everything. 
  **/
/*
void DEBUG_LOGFILE ( const char * file )
{
#ifndef LOG_INFO
	return;
#else
	FILE	*fp;

	// open the log file and check for error 
	strncpy ( LogMgmt::m_logfile, file, 1024-1 );
	fp = fopen ( LogMgmt::m_logfile, "w" );
	if ( !fp ) return;

	fclose ( fp );
#endif
}
*/