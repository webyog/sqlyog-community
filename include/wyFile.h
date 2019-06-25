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

# ifndef _WYFILE_H
# define _WYFILE_H

#include "Datatype.h"
#include "wyString.h"
#include <stdarg.h>
#include <ctype.h>

#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <stdarg.h>
#include <wchar.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>

//#ifdef _WIN32
//#define YO_RDONLY	_O_RDONLY
//#define YO_WRONLY	_O_WRONLY
//#define YO_BINARY	_O_BINARY
//#define YO_TRUNC	_O_TRUNC	
//#define YO_APPEND	_O_APPEND
//#define YO_CREAT	_O_CREAT
//#define YO_IREAD	_S_IREAD
//#define YO_IWRITE	_S_IWRITE
//#endif

class wyFile
{
public:

	//Constructor & Descripter
	wyFile();
	~wyFile();

	void SetFilename(wyString * pFileName);

	//Checking whether the temp fie is existing or not
	/**
	@param pFileName : IN File name 
	return wyTrue if exist elese return wyFalse
	*/
	wyBool		  CheckIfFileExists();

	//Gets the system Temp folder path
	/**
	@is_longpath : IN to get the long path name
	@return wyFalse if API failed, else return wyTrue
	*/
	wyBool			GetTempFilePath(wyString *pFile); 

	//To remove the file
	/**
	@return wyTrue if removed successfully else return wyFalse
	*/
	wyInt32			RemoveFile();

	//Open the Temp file with permissions
	/**
	@param pFlags : IN flags to open() function
	@param pMode  : IN mode of the file to be opened
	@paarm inheritable:can created file be inherited by child process
	@return value of open() function
	*/
	wyInt32			OpenWithPermission(wyInt32 pAccessMode, wyInt32 pCreationDisposition, wyBool inheritable=wyFalse);

	//To close the file
	/**
	@return return value of close() function
	*/
	wyInt32			Close();
	
	/******Variables************/
	
	//File descripter
	HANDLE m_hfile;
	wyString m_filename;
};
#endif
