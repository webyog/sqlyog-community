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

#ifdef _WIN32
#if _MSC_VER < 1300
#define DECLSPEC_DEPRECATED
// VC6: change this path to your Platform SDK headers
#include "M:\\dev7\\vs\\devtools\\common\\win32sdk\\include\\dbghelp.h"			// must be XP version of file
#else
// VC7: ships with updated headers
#include "dbghelp.h"
#endif

#include "DataType.h"
#include "WyString.h"

// based on dbghelp.h
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hprocess, DWORD pid, HANDLE hfile, MINIDUMP_TYPE dumptype,
									CONST PMINIDUMP_EXCEPTION_INFORMATION exceptionparam,
									CONST PMINIDUMP_USER_STREAM_INFORMATION userstreamparam,
									CONST PMINIDUMP_CALLBACK_INFORMATION callbackparam);

class MiniDump
{
private:
	static wyWChar m_appname[32];
    static MINIDUMPWRITEDUMP m_dump;
    static HANDLE m_hfile;
    static HMODULE m_hdll;

	//Expilcit Path for .dmp if defined
	static wyString	m_dumpfilepath;

    void    CreateUniqueDumpFile();
    static  LONG WINAPI HandleSQLyogCrashDump(struct _EXCEPTION_POINTERS *pexceptioninfo);

	///This will delete all zero byte dump files from SQLyog folder
	/**
	@returns void,
	*/
	void	DelZeroByteDumpFiles();

public:
    MiniDump();
    ~MiniDump();

	void InitDumpDetails(const wyChar *dumppath);
	
};

#endif