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
	Purpose: A simple extra specific logger for SQLyog and other similar projects

	There are basically three macros or functions. 

	DEBUG_LOGFILE
		sets the log file for output

	DEBUG_ENTER ( functionname as string )
		This macro will open the log file and enter the function name as "Enter functionname"
		It will create LogMgmt class which on destructor will log "Exit functionname" 

	DEBUG_LOG
		sprintf() like function to log stuff to a file if extra info is needed to be logged

	The main idea about this class is to find out which function the program crashes. With beautiful 
	debugger present these days, we wont even require it but sometimes program crashes in release
	mode but not in release mode. 

	The usage is very simple.

	Call DEBUG_LOGFILE in main function or where the app starts and specify the log file to start logging.
	This details is stored in a static variable.

	Whenever you enter a function just call DEBUG_ENTER with the function name. The macro will create
	an instance of LogMgmt class. The constructor will write in the log file:

	Enter "functionname"

	The destructor of the class will write in the log file

	Exit "functionname"

	Therefore, for a program to perform correctly, each entry to a function should have a corresponding 
	exit line. If the program crashes then it wont have a corresponding EXIT line so we know where it crashed.

	If you want to log some values then you can use DEBUG_LOG that has exactly sprintf like syntax.
*/

#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

#ifndef __YOGLOGGER__
#define __YOGLOGGER__

class LogMgmt
{

private:

	char			m_function[1024];

public:

	static char		m_logfile[MAX_PATH+1];

	LogMgmt  ( const char * function );
	~LogMgmt ();

};

#ifdef LOG_INFO
	#define		DEBUG_ENTER(a)		LogMgmt	y(a)
#else
	#define		DEBUG_ENTER(a)		
#endif

/* Variable argument macro is not supported in C++ yet so we we have a similar function */

void DEBUG_LOG ( const char * format, ... );
void DEBUG_LOGFILE ( const char * file );

#endif
