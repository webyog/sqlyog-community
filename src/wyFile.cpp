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


#include "wyFile.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>

wyFile::wyFile()
{
	m_hfile = NULL;
}

wyFile::~wyFile()
{
	Close();
}

void wyFile::SetFilename(wyString * pFileName) 
{
	m_filename.SetAs(pFileName->GetString());
}


wyBool
wyFile::GetTempFilePath(wyString *pTempPath)
{
	wyWChar path[MAX_PATH + 1];
			
	if(GetTempPath(MAX_PATH, path) == 0)
		return wyFalse;
	
	/*if(GetLongPathName(path, longpath, MAX_PATH) == 0) 
	{
		return wyFalse;
	}*/

	pTempPath->SetAs(path);
	return wyTrue;	
}

wyInt32
wyFile::OpenWithPermission(wyInt32 pAccessMode, wyInt32 pCreationDisposition,wyBool inheritable)
{	
	wyInt32 ret = 0;
	m_hfile = CreateFile((LPCWSTR)m_filename.GetAsWideChar(), 
						 pAccessMode, 0, NULL, pCreationDisposition, 
						 FILE_ATTRIBUTE_NORMAL, NULL);

	if(m_hfile == INVALID_HANDLE_VALUE) {
		m_hfile = NULL;
		ret = -1;
	}
	else if(inheritable)
	{
	//child process(plink.exe) shoudn't inherit this file.
		if(!SetHandleInformation(m_hfile, HANDLE_FLAG_INHERIT, 0))
		{
		//
		}
	}
	return ret;
}

wyInt32 
wyFile::Close()
{
	wyInt32 ret = 0;
	if (m_hfile == NULL) {
		return ret;
	}
	ret = CloseHandle(m_hfile);
	m_hfile = NULL;
	return ret;
}

wyBool
wyFile::CheckIfFileExists()
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	if (m_filename.GetLength() == 0) 
	{
		return wyFalse;
	}

	hFind = FindFirstFile(m_filename.GetAsWideChar(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) 
	{
		return wyFalse;
	} 
	else 
	{
		FindClose(hFind);
		return wyTrue;
	}
}

wyInt32 
wyFile::RemoveFile()
{
	wyInt32 ret = 0;

	if(m_filename.GetLength())
	{
		ret = DeleteFile(m_filename.GetAsWideChar());
	}

	return ret;
}
