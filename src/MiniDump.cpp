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

#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <shlobj.h>
#include "MiniDump.h"
#include "Global.h"
#include "DataType.h"

extern PGLOBALS pGlobals;

#ifdef _WIN32

wyWChar MiniDump::m_appname[32] = {0};
HANDLE MiniDump::m_hfile;
HMODULE MiniDump::m_hdll;
wyString MiniDump::m_dumpfilepath;

MINIDUMPWRITEDUMP MiniDump::m_dump;

#ifdef _CONSOLE 
#define DUMPNAME "SJA"
#define CRASH_MSG "SJA crashed!, dump file saved in "
#define FAIL_MSG "Failed to save SJA dump file!"
#define MSGCAPTION "SJA diagnostic message"
#else
#define DUMPNAME "SQLyog"
#define CRASH_MSG "SQLyog crashed. \nDump file saved in "
#define FAIL_MSG "Failed to save SQLyog dump file!"
#define MSGCAPTION "SQLyog diagnostic message"
#endif

#define DBGHELP L"DBGHELP.DLL not found! or DBGHELP.DLL in your system is too old for SQLyog debug information!, \n\
To get debug information you need dbghelp.dll version no older than 5.1.\n\
You must copy such newer file to SQLyog installation folder."



MiniDump::MiniDump()
{
    m_hfile = INVALID_HANDLE_VALUE;
    m_hdll  = NULL;
}

MiniDump::~MiniDump()
{
    wyWChar     fullpath[MAX_PATH] = {0};
    wyString    fullpathstr, appnamestr;

    if(m_hfile != INVALID_HANDLE_VALUE)
    {
		if(m_dumpfilepath.GetLength())
		{
			fullpathstr.SetAs(m_dumpfilepath);
		}

        else if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, fullpath)))
		{
            fullpathstr.SetAs(fullpath);

			fullpathstr.Add("\\SQLyog");
		}
            
        appnamestr.SetAs(m_appname);
        fullpathstr.AddSprintf("\\%s", appnamestr.GetString());

        CloseHandle(m_hfile);
        DeleteFile(fullpathstr.GetAsWideChar());
    }

    if(m_hdll)
        FreeLibrary(m_hdll);
}

void 
MiniDump::InitDumpDetails(const wyChar *dumppath)
{
    wyWChar		dbghelppath[_MAX_PATH];
	wyWChar		*pslash;

	if(m_dumpfilepath.GetLength())
		m_dumpfilepath.Clear();

	if(dumppath && strlen(dumppath))
		m_dumpfilepath.SetAs(dumppath);

    CreateUniqueDumpFile();

    ::SetUnhandledExceptionFilter(HandleSQLyogCrashDump);
            
	if (GetModuleFileName(NULL, dbghelppath, _MAX_PATH))
	{
		pslash = wcsrchr(dbghelppath, '\\');
		if (pslash)
		{
			wcscpy(pslash+1, L"DBGHELP.DLL" );
			m_hdll = ::LoadLibrary(dbghelppath);
		}
	}

    if(!m_hdll)
	{
		// load any version we can
		m_hdll = ::LoadLibrary(L"DBGHELP.DLL");
	}
    
	if(m_hdll)
        m_dump = (MINIDUMPWRITEDUMP)::GetProcAddress(m_hdll, "MiniDumpWriteDump");

    return;
}

LONG 
MiniDump::HandleSQLyogCrashDump(struct _EXCEPTION_POINTERS *pexceptioninfo)
{
    LONG        retval = EXCEPTION_CONTINUE_SEARCH;
    wyWChar     msg[512] = {0}, apppath[MAX_PATH];
    BOOL        ok;
    wyString    dumppath, appnamestr;

    if(m_hfile == INVALID_HANDLE_VALUE || !m_hdll || !m_dump)
    {
        MessageBox(NULL, DBGHELP, TEXT(MSGCAPTION), MB_OK | MB_TASKMODAL | MB_ICONERROR);
        return retval;
    }
        
    _MINIDUMP_EXCEPTION_INFORMATION exinfo;

	exinfo.ThreadId = ::GetCurrentThreadId();
	exinfo.ExceptionPointers = pexceptioninfo;
	exinfo.ClientPointers = NULL;
	
    // write the dump
	ok = m_dump(GetCurrentProcess(), GetCurrentProcessId(), m_hfile, 
                                    MiniDumpNormal, &exinfo, NULL, NULL );

	if(ok)
	{
		if(m_dumpfilepath.GetLength())
		{
			dumppath.SetAs(m_dumpfilepath);
		}

        else if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, apppath)))
        {
            dumppath.SetAs(apppath);
            /*appnamestr.SetAs(m_appname);*/
            dumppath.Add("\\SQLyog");
        }

		appnamestr.SetAs(m_appname);
		dumppath.AddSprintf("\\%s", appnamestr.GetString());

        _snwprintf(msg, 511, L"%s%s", TEXT(CRASH_MSG), dumppath.GetAsWideChar());
        MessageBox(NULL, msg, TEXT(MSGCAPTION), MB_OK | MB_TASKMODAL | MB_ICONERROR);
        retval  = EXCEPTION_EXECUTE_HANDLER;
	}
	else
        MessageBox(NULL, TEXT(FAIL_MSG), TEXT(MSGCAPTION), MB_OK | MB_TASKMODAL | MB_ICONERROR);
    
    ::CloseHandle(m_hfile);
    m_hfile = INVALID_HANDLE_VALUE;
        
	return retval;
}

void 
MiniDump::CreateUniqueDumpFile()
{
    wyWChar     basename[] = L"_Dump_", filename[32], fullpath[MAX_PATH + 1] = {0};
    wyInt32     count = 0;
    wyString    filenamestr, fullpathstr, apppathstr, pathfile;

	if(m_dumpfilepath.GetLength())
	{
		fullpathstr.SetAs(m_dumpfilepath);
	}

    else if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, fullpath)))
    {
        fullpathstr.SetAs(fullpath);
        fullpathstr.Add("\\SQLyog");
    }
    else
        return;
	
    /*apppathstr.SetAs(apppath);
    directory = strrchr(apppathstr.GetString(), '\\');

    if(!directory)
        return;

    *(directory+1) = '\0';*/
    
	//This will delete all zero byte dump files from SQLyog folder
	DelZeroByteDumpFiles();
    
    for(count = 0; count < 999; count++)
    {

        _snwprintf(filename, 31, L"%s%s%03d.dmp", TEXT(DUMPNAME), basename, count);
        filenamestr.SetAs(filename);
		
        pathfile.SetAs(fullpathstr);
        
		pathfile.AddSprintf("\\%s", filenamestr.GetString());
		
        m_hfile = ::CreateFile(pathfile.GetAsWideChar(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, 
                                  CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

        if(m_hfile != INVALID_HANDLE_VALUE)
        {
            wcscpy(m_appname, filename);
            break;
        }
    }

    //All the 1000 dump files are existing force a filecreation
    if(m_hfile == INVALID_HANDLE_VALUE)
    {
        count = 0;
         _snwprintf(filename, 31, L"%s%s%d", TEXT(DUMPNAME), basename, count);
         filenamestr.SetAs(filename);
         fullpathstr.Add(filenamestr.GetString());
         m_hfile = ::CreateFile(fullpathstr.GetAsWideChar(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, 
                                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if(m_hfile != INVALID_HANDLE_VALUE)
            wcscpy(m_appname, filename);
    }
    return;
}

///This will delete all zero byte dump files from SQLyog folder
void 
MiniDump::DelZeroByteDumpFiles()
{
	wyWChar				fullpath[MAX_PATH + 1] = {0};
    wyInt32				pos, extlen;
    wyString			fullpathstr, filename, extstr;
	HANDLE				hfile, holdfile;
	DWORD				dwfilesize;
	WIN32_FIND_DATA		wfdd;
	bool				res;
	
	if(m_dumpfilepath.GetLength())
	{
		fullpathstr.Sprintf("%s\\*", m_dumpfilepath.GetString());
	}

    else if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, fullpath)))
    {
        fullpathstr.SetAs(fullpath);
        fullpathstr.Add("\\SQLyog\\*");
    }
    else
        return;
	
	//for getting the first file
	hfile = FindFirstFile(fullpathstr.GetAsWideChar(), &wfdd); 		

	if(hfile == INVALID_HANDLE_VALUE) 
		return ;
	do
	{
		//if filename is . or ..
		if (wcscmp(L".", wfdd.cFileName) == 0 || wcscmp(L"..", wfdd.cFileName) == 0)
			 res = ::FindNextFile(hfile , &wfdd);
		else
		{
			filename.SetAs(wfdd.cFileName);

			//checking for extension
			pos = filename.FindChar('.');
			if(pos != -1)
			{
				extlen = filename.GetLength() -  (pos + 1);
				if(extlen != 0)
				{
					extstr.SetAs(filename.Substr(pos + 1, extlen));

					//chcking for .dmp (dump) files
					if(extstr.GetLength() != 0 && extstr.CompareI("dmp") == 0)
					{
						fullpathstr.SetAs(fullpath);
						fullpathstr.AddSprintf("\\SQLyog\\%s", filename.GetString());

						holdfile	=	CreateFile(fullpathstr.GetAsWideChar(), GENERIC_WRITE, FILE_SHARE_DELETE, NULL, 
										OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

						if(holdfile != INVALID_HANDLE_VALUE)
						{	
							//for getting the file size
							dwfilesize = GetFileSize(holdfile, NULL);
							CloseHandle(holdfile);

							//if file size is zero, we will delete that file
							if(dwfilesize == 0)
								DeleteFile(fullpathstr.GetAsWideChar());
						}
					}
				}
			}
			//for getting the next file in SQLyof directory
			res = ::FindNextFile(hfile , &wfdd);
		}		
	}
	while(res);

	::FindClose(hfile);	
}

#endif
