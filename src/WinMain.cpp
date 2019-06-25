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

#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
//#include <vld.h>
#include "Scintilla.h"
#include "FrameWindowHelper.h"
#include "ExportMultiFormat.h"
#include "htmlayout.h"
#include "Global.h"
#include "CommonHelper.h"
#include "GUIHelper.h"
#include "MiniDump.h"

#ifndef COMMUNITY
#include"openssl\applink.c"
#endif

#ifndef VC6
#include <gdiplus.h>
#include <GdiPlusGraphics.h> 
#include <olectl.h>
#endif

#define STATUS_BAR_SECTIONS 7

GLOBALS		    *pGlobals;
HACCEL		    g_accel;

#if defined (_DEBUG) && defined (ENTERPRISE)
//#define			BLACKBOX					
#endif 

//	Function	:
//					The main function of the application.
//					The start of the software.

wyInt32 WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hprevinstance,
                    PSTR cmdline, wyInt32 icmdshow)
{
	HINSTANCE	hrich, hsci;
	
#ifdef BLACKBOX
	HINSTANCE	hblackbox;
#endif

	FrameWindow    *mainwin;
	MSG			    msg;

	wyBool	isVistaAnd32BitSQLyog = wyFalse;
	pGlobals = new GLOBALS;
    
    _ASSERT(pGlobals != NULL);
	//DEBUG_ENTER("WinMain");
	//call mysql_library_init to avoid crash
	if (mysql_library_init(0, NULL, NULL)) {
    DisplayErrorText(1, _("could not initialize MySQL library"));
    return 0;
  }
	// First clear every stuff in the pglobals variable.
	InitGlobals(pGlobals);

	InitWinSock();

	///*Checks whether commandline argument as got explicit path for .ini, and other logfiles.
	//also filename whose contents as to be displayed in Sqlyog Editor
	//Otion can be any of one
	//i.   SQLyogEnt.exe -dir "E:\path" -f "D:\test.sql"
	//ii.  SQLyogEnt.exe -dir "E:\path" "D:\test.sql"
	//iii. SQLyogEnt.exe  -f "D:\test.sql" -dir "E:\path" -f
	//-dir stands for explicit path for .ini, tags, logs, favourites , etc
	//-f stands for file to be oponed in editor
	//*/
	//Gets the Attributes passed through command line(-f, -dir)
	if(pGlobals->m_configdirpath.GetLength())
//unsigned long len=0;
//    if(len=pGlobals->m_configdirpath.GetLength())
		pGlobals->m_configdirpath.Clear();

	if(GetConfigDetails(cmdline) == wyFalse)
		return 0;

	CreateInitFile();

#ifndef  _WIN64
	OSVERSIONINFOEX osvi;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	//get the OS version and set the display style accordingly
	if (GetVersionEx((OSVERSIONINFO*)&osvi)) {
		if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0 && osvi.wProductType == VER_NT_WORKSTATION)
			isVistaAnd32BitSQLyog = wyTrue;
	}
#endif

#ifdef _DEBUG
    HANDLE          hlogfile = NULL;
    
    hlogfile = CreateFile(L"SQLyog_debug_log.txt", GENERIC_WRITE, FILE_SHARE_WRITE, 
                           NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

#else
	
	//Initialize the Path for .INI & .dmp files
	CreateInitFile();

	if (isVistaAnd32BitSQLyog == wyFalse) {
		MiniDump    dumpcrash;

		if (IsCrashDumpSupported() == wyTrue)
			dumpcrash.InitDumpDetails(pGlobals->m_configdirpath.GetString());
	}
	
#endif

   _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_WNDW);
   _CrtSetReportFile(_CRT_WARN, hlogfile);
   _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_WNDW);
   _CrtSetReportFile(_CRT_ERROR, hlogfile);
   _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_WNDW);
   _CrtSetReportFile(_CRT_ASSERT, hlogfile);

   _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE );
   _CrtSetReportFile(_CRT_WARN, hlogfile);
   _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE );
   _CrtSetReportFile(_CRT_ERROR, hlogfile);
   _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE );
   _CrtSetReportFile(_CRT_ASSERT, hlogfile);

   _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );


	VERIFY(hsci = ::LoadLibrary(L"SciLexer.dll"));
	
    if(!hsci)
	{
		DisplayErrorText(GetLastError(), _("Error loading SciLexer.dll"));
		VERIFY(FreeLibrary(hsci));
        return 0;
	}

    pGlobals->m_statusbarsections = STATUS_BAR_SECTIONS;

	/* register scintilla */
	//Scintilla_RegisterClasses(NULL);

#ifndef VC6
	/* startup gdiplus...this is required to display image in BLOB window */
	ULONG_PTR	gditoken;
	Gdiplus::GdiplusStartupInput startupinput;
	Gdiplus::GdiplusStartup(&gditoken, &startupinput, NULL);
#endif

    pGlobals->m_hinstance   = hinstance;

	VERIFY(hrich		= ::LoadLibrary(L"Riched20.dll"));

#ifdef BLACKBOX
	VERIFY(hblackbox	= ::LoadLibrary(L"BlackBox.dll"));
#endif

	/* check for library problems */
	if(!hrich)
	{
		DisplayErrorText(GetLastError(), _("Error loading riched20.dll"));
		VERIFY(FreeLibrary(hsci));
		return 0;
	}
//Removing offline helpfile from version 12.05
//#ifndef _DEBUG
//	VERIFY(LoadHelpFile());
//#endif
    
	pGlobals->m_findreplace	= wyFalse;
    CreateCustomTabIconList();

	// Initialize the common controls.
    InitCustomControls();
    CollectCurrentWindowsVersion();
    pGlobals->m_modulenamelength = GetModuleNameLength();
    wyTheme::SubclassControls();

    if(!pGlobals->m_modulenamelength)
    {
        DisplayErrorText(GetLastError(), _("GetModuleFileName length failed!"));
        return 0;
    }

    SetLanguage(NULL, wyTrue);
    wyTheme::Init();

    // Create the mainwindow.
	mainwin = new FrameWindow(pGlobals->m_hinstance);
	
    // Set the global.
	pGlobals->m_pcmainwin     = mainwin;
	
	if(!mainwin || !mainwin->Create())
	{
		VERIFY(FreeLibrary(hsci));
		return 0;
	}

	// Initialize the accel window.
	VERIFY(g_accel	= LoadAccelerators(pGlobals->m_hinstance, MAKEINTRESOURCE(IDR_ACCELERATOR)));
	SetForegroundWindow(mainwin->m_hwndmain);
	mainwin->CreateConnDialog(wyTrue);

	// The main loop for messages.
	while(GetMessage(&msg, NULL, 0, 0))
    {
        if(pGlobals->m_pcmainwin->m_finddlg)
        {
			if((pGlobals->m_pcmainwin->m_finddlg && IsWindowVisible(pGlobals->m_pcmainwin->m_finddlg)&& 
					IsDialogMessage(pGlobals->m_pcmainwin->m_finddlg, &msg)))
            {
					pGlobals->m_findreplace = wyTrue;
					continue;
			}
		}



		if(!TranslateMDISysAccel(pGlobals->m_hwndclient, &msg))	
        {
			if(!(TranslateAccelerator(mainwin->GetHwnd(), g_accel, &msg)))
            {
                /// code to catch Accel(short-cuts for Save & Revert) key-press on Create/Alter table tabbed interface
                if(pGlobals->m_pcmainwin->m_htabinterface && IsDialogMessage(pGlobals->m_pcmainwin->m_htabinterface, &msg))
                {
                    continue;
                }
                
                if(pGlobals->m_pcmainwin->m_hwndtooltip && msg.message == WM_LBUTTONDOWN)
                {
                    FrameWindow::ShowQueryExecToolTip(wyFalse);
                }

				pGlobals->m_findreplace = wyFalse;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}		
	}
	
    ImageList_Destroy(pGlobals->m_hiconlist);
	VERIFY(FreeLibrary(hrich));
	
#ifdef BLACKBOX
	VERIFY(FreeLibrary(hblackbox));
#endif

	VERIFY(FreeLibrary(hsci));

#ifndef COMMUNITY
    if(pGlobals->m_entinst)
        VERIFY(FreeLibrary(pGlobals->m_entinst));
#endif
    // Release resources used by the critical section object.
    DeleteCriticalSection(&pGlobals->m_csglobal);
	DeleteCriticalSection(&pGlobals->m_csiniglobal);
	DeleteCriticalSection(&pGlobals->m_cssshglobal);
	delete mainwin;
	delete pGlobals;

#ifndef VC6
	/* free up gdi plus */
	Gdiplus::GdiplusShutdown(gditoken);
#endif

   _CrtDumpMemoryLeaks();

   //Socket dll cleanup
   WSACleanup();
    
#ifdef _DEBUG
    if(hlogfile)
        CloseHandle(hlogfile);
#endif
   
	//fixed my_once_alloc() memory leak
	mysql_library_end();
    wyTheme::FreeResources();

	return msg.wParam;
}