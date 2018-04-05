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

#include "GUIHelper.h"
#include "EditorFont.h"
#include "Verify.h"
#include "MDIWindow.h"
#include "Global.h"
#include "CommonHelper.h"
#include "MySQLVersionHelper.h"
#include "SQLMaker.h"
#include "sqlite3.h"
#include "WyMySQLError.h"
#include "ClientMySQLWrapper.h"
#include "SQLFormatter.h"
#include "WyCrc.h"
#include "TabResult.h"
#include "CCustomComboBox.h"
#include <time.h>
#include <shlwapi.h>
#include <SQLTokenizer.h>

#ifndef COMMUNITY
#include "DatabaseSearch.h"
#endif


#define SQLITE_SYNCMODE			"FULL"		
#define COLWID_APPVERSION_MAJOR "1"
#define COLWID_APPVERSION_MINOR "0"
#define SQLITE_JOURNALMODE      "WAL"



extern PGLOBALS pGlobals;
#define			READONLYUNCHECKTILLHERE 18
#define         DEF_BULK_SIZE           1024

DlgControl::DlgControl(HWND hwnd, wyInt32 id, RECT* rect, wyBool issizex, wyBool issizey)
{
    m_hwnd = hwnd;
    m_rect.left = rect->left;
    m_rect.top = rect->top;
    m_rect.right = rect->right;
    m_rect.bottom = rect->bottom;
    m_id = id;
    m_issizex = issizex;
    m_issizey = issizey;
}

DlgControl::~DlgControl()
{
}

DltElement::DltElement(TVITEM &tvi)
{
    wyInt32 len = wcslen(tvi.pszText) + 1;
    m_tvi.cchTextMax = len;
    m_tvi.iImage = tvi.iImage;
    m_tvi.iSelectedImage = tvi.iSelectedImage;
    m_tvi.lParam = tvi.lParam;
    m_tvi.state = tvi.state;
    m_tvi.stateMask = tvi.stateMask;
    m_tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_STATE;
    m_tvi.pszText = new wyWChar[len];
    wcscpy(m_tvi.pszText, tvi.pszText);
}

DltElement::~DltElement()
{
    delete(m_tvi.pszText);
}

OBDltElementParam::OBDltElementParam(LPARAM value)
{
    memset(m_filterText, 0, sizeof(m_filterText));
    m_lvalue = value;
}

OBDltElementParam::~OBDltElementParam()
{

}



wyInt32 
callback_count(void *count, wyInt32 argc, wyChar **argv, wyChar **azColName)
{
	wyInt32 *concount =(wyInt32 *)count;
	return ++(*concount);
	
}

wyInt32 
YogSqliteExec(sqlite3 *db, const wyChar *query, sqlite3_callback xCallback, 
                void *parg)//, wyChar **errmsg)
{
	
    wyInt32 retcode, trycount =0;
    
    do{
        retcode = sqlite3_exec(db, query, xCallback, parg, NULL);

        trycount++;

    }while(retcode == SQLITE_BUSY && trycount < 3);

    return retcode;
}

wyInt32
SetSqliteJournalMode(sqlite3 *db)
{
    wyString    query;
    wyInt32     retcode;

    query.Sprintf("PRAGMA journal_mode = %s", SQLITE_JOURNALMODE);
    retcode = YogSqliteExec(db, query.GetString(), NULL, 0);

    return retcode;
}

wyInt32 
SetSqliteSyncMode(sqlite3 *db)
{
    wyString    query;
    wyInt32     retcode;

	query.Sprintf("PRAGMA synchronous = %s", SQLITE_SYNCMODE); //Full mode
    retcode = YogSqliteExec(db, query.GetString(), NULL, 0);

    return retcode;
}

wyInt32
SetSQLiteCache(sqlite3 *db)
{
	wyString    query;
    wyInt32     retcode;

	query.SetAs("PRAGMA cache_size = 8000"); //Full mode
    retcode = YogSqliteExec(db, query.GetString(), NULL, 0);

    return retcode;	
}

wyInt32
SetSQLitePage(sqlite3 *db)
{
	wyString    query;
    wyInt32     retcode;

	query.SetAs("PRAGMA page_size = 4096");  //Full mode
    retcode = YogSqliteExec(db, query.GetString(), NULL, 0);

    return retcode;	
}

/*	This will set the temporary storage area for sqlite, 
	The values are FILE, MEMORY and DEFAULT  */
wyInt32  
SetTempStore(sqlite3 * hdb)
{
    wyString    query;
    wyInt32     retcode;

	query.SetAs("PRAGMA temp_store=MEMORY");
	retcode = YogSqliteExec(hdb, query.GetString(), NULL, 0);

    return retcode;
}

void 
InitTagsArray(tagArray *tags, wyInt32 tagtype, wyBool typeflag)
{
   	tags->maxcount	= 1;
	tags->rowcount	= 0;
	tags->type		= tagtype;
    tags->bflag     = typeflag;
	tags->keys      = (wyChar**)calloc(sizeof(wyChar*), tags->maxcount);
}

wyBool 
OpenKeyWordsDB(sqlite3 **phdb)
{
    wyWChar				directory[MAX_PATH+64] = {0};
    wyInt32				count = 0, rc;
    HANDLE				hfind;
    WIN32_FIND_DATAW	fdata;
    wyString			directoryname;

    VERIFY(count = GetModuleFileName(NULL, directory, MAX_PATH -1));
		
	directory[count - pGlobals->m_modulenamelength - 1] = '\0';
	
	wcscat(directory, L"\\");
	wcscat(directory, L"Keywords.db");
	
	hfind = FindFirstFile(directory, &fdata);			
		
	if(hfind == INVALID_HANDLE_VALUE)	/* File is existing */
		return wyFalse;
	
	FindClose(hfind);
	
	directoryname.SetAs(directory);

	rc = sqlite3_open_v2(directoryname.GetString(), phdb, SQLITE_OPEN_READONLY, 0);
	
	if(rc != SQLITE_OK)
		return wyFalse;

	//sqlite3_profile(sqlite_, wySQLite::Profile, this);
	
	sqlite3_busy_timeout(*phdb, SQLITE_TIMEOUT);
    
	SetSqliteSyncMode(*phdb);
	SetSQLitePage(*phdb);
	SetSQLiteCache(*phdb);

    return wyTrue;
}

wyBool					
GetClusterdbSupportForFk(MDIWindow *wnd)
{
	wyString	query, str1;
	wyChar		*major, *minor;
	wyChar		seps[] = ".";
	wyInt32		majorver = 0, minorver = 0;
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	res = NULL;
	query.Sprintf(GET_NDB_VERSION_STRING);
	res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);
	if(!res)
	{
		return wyFalse;
	}
	VERIFY(myrow = wnd->m_tunnel->mysql_fetch_row(res));
	if(!myrow)
	{
		wnd->m_tunnel->mysql_free_result(res);
		return wyFalse;
	}
	str1.SetAs(myrow[0]);
	str1.FindIAndReplace("ndb-","");
	major = strtok((wyChar*)str1.GetString(), seps);
	minor = strtok(NULL, seps);								
	majorver = atoi(major);
	minorver = atoi(minor);
	wnd->m_tunnel->mysql_free_result(res);
	if(majorver > 7 || (majorver >= 7 && minorver >= 3))
	{
		return wyTrue;
	}
	else
	{
		return wyFalse;
	}
}

wyChar *
GetUtf8String(const wyChar *ansistr)
{
	wyInt32 length		= 0;
	wyChar  *utfstr		= {0};
	WCHAR   *unicodestr	= {0};

	// get the number of widechars required
	length	=	MultiByteToWideChar(CP_ACP, 0,(LPCSTR)ansistr, -1, NULL, NULL);
	unicodestr = new WCHAR[length+1];

	// get the converted data
	MultiByteToWideChar(CP_ACP, 0,(LPCSTR)ansistr, -1,(LPWSTR)unicodestr, length);

	// now convert it to utf-8.
	// the only way to convert an ansi text to utf-8 is this two way.
	length		=	WideCharToMultiByte(CP_UTF8, 0,(LPWSTR)unicodestr, -1, NULL, NULL, NULL, NULL);
	// allocate space
	utfstr		=	new wyChar[length+1];
	WideCharToMultiByte(CP_UTF8, 0,(LPWSTR)unicodestr, -1,(LPSTR)utfstr, length, NULL, NULL);

	delete[]unicodestr; 
	return	utfstr;
}

WCHAR*
GetWideString(const wyChar *strtoconv)
{
	wyInt32  len;
	WCHAR	*pwidechar;

	len = MultiByteToWideChar(CP_ACP, 0, strtoconv, -1,  NULL,  0);

	// some extra buffer
	len += 64;
	pwidechar = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, len*(sizeof(WCHAR)));
	MultiByteToWideChar(CP_ACP, 0, strtoconv, -1, pwidechar, len);

	return pwidechar;

}

// This function ia wrapper to the Win32 messagebox. Before calling MessageBox it gets the HWND
// which has focus and after the message box has been shown it returns the focus back to that HWND
// It returns the return value of the MessageBox originally intented.
wyInt32 
yog_message(HWND hwnd, LPCWSTR text, LPCWSTR caption, wyUInt32 type)
{
	wyInt32     ret;
	HWND        hwndfocus;
    MDIWindow   *wnd;
    TabEditor   *ptabeditor = NULL;
    TabMgmt     *ptabmgmt = NULL;

	hwndfocus = GetFocus();

	ret = MessageBox(hwnd, text, caption, type );

    wnd = GetActiveWin();
    
    if(wnd)
        ptabeditor = wnd->GetActiveTabEditor();
    
	if(wnd && ptabeditor)
    {
		ptabmgmt = ptabeditor->m_pctabmgmt;

        UpdateWindow(wnd->m_pctabmodule->GetHwnd());

		UpdateWindow(hwnd);
	}

	SetFocus(hwndfocus);

	return ret;
}

// Function returns the MDIWindow*	of the active MDI windows
MDIWindow *
GetActiveWin()
{
	HWND        hwndmdi;
	MDIWindow   *pcquerywnd;

	if(!pGlobals->m_hwndclient)
		return NULL;

	hwndmdi	= (HWND)SendMessage(pGlobals->m_hwndclient, WM_MDIGETACTIVE, 0, 0);

	if(!hwndmdi)
		return NULL;

	pcquerywnd  = (MDIWindow*)GetWindowLongPtr(hwndmdi, GWLP_USERDATA);
	return pcquerywnd;
}

MDIWindow *
GetActiveWinByPost()
{
	HWND        hwndmdi;
	MDIWindow   *pcquerywnd = NULL;
	THREAD_MSG_PARAM tmp = {0};

    //set the lparam sent
    tmp.m_lparam = 0;

	if(!pGlobals->m_hwndclient)
		return NULL;
    if(GetWindowThreadProcessId(pGlobals->m_hwndclient, NULL) == GetCurrentThreadId())
    {
        hwndmdi	= (HWND)SendMessage(pGlobals->m_hwndclient, WM_MDIGETACTIVE, 0, 0);
		if(!hwndmdi)
			return NULL;
		pcquerywnd  = (MDIWindow*)GetWindowLongPtr(hwndmdi, GWLP_USERDATA);
    }
    else
    {
		if(WaitForSingleObject(pGlobals->m_pcmainwin->m_sqlyogcloseevent, 0) != WAIT_OBJECT_0 )
			return NULL;
		//create an event to synchronize the worker thread and ui thread
		tmp.m_hevent = CreateEvent(NULL, TRUE, FALSE, NULL);

		//now post the message to ui thread and wait for the event to be set
		PostMessage(pGlobals->m_hwndclient, UM_MDIGETACTIVE, (WPARAM)0, (LPARAM)&tmp);
	
		if(WaitForSingleObject(tmp.m_hevent, 1000) != WAIT_TIMEOUT)
		{
			pcquerywnd = (MDIWindow*)tmp.m_lparam;
		}
	
		CloseHandle(tmp.m_hevent);
	}
	return pcquerywnd;
}

wyInt32 
GetBulkSize(wyInt32 * bulksize)
{
	wyWChar		*lpfileport=0;
	wyWChar		directory[MAX_PATH+1]= {0};
    wyInt32     defbulksize = 0;
	wyString	dirstr;
	
	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	defbulksize = wyIni::IniGetInt(GENERALPREFA, "DefBulkSize", DEF_BULK_SIZE, dirstr.GetString());

    if(defbulksize)
        *bulksize = 0;
    else
	    *bulksize = wyIni::IniGetInt(GENERALPREFA, "BulkSize", DEF_BULK_SIZE, dirstr.GetString());

	return 1;
}

///*rotates bit right */
//void
//RotateBitRight(UCHAR *str)
//{
//	wyInt32     count;
//
//	for(count = 0; str[count]; count++)
//		str[count] = (((str[count])>>(1)) | ((str[count])<<(8 - (1))));
//
//    return;
//}
//
//// We keep the name in encrypted form.
//// so we do a bit rotation of 1 on the left before writing it into the registry.
//void 
//RotateBitLeft(UCHAR *str)
//{
//	wyInt32     count;
//
//    for(count = 0; str[count]; count++)
//		str[count] = (((str[count])<<(1)) | ((str[count])>>(8 - (1))));
//
//	return;
//}



wyBool 
CheckForAutoCompleteDir(wyString &path)
{
    wyWChar     fullpath[MAX_PATH + 10] = {0};	
	wyString    fullpathstr;
	wyBool		isappdatafolder = wyFalse;
    
	if(!pGlobals->m_autocompletetagsdir)
		return wyFalse;

	fullpathstr.Sprintf("%s", pGlobals->m_autocompletetagsdir);
	//if tags path is not found or path is empty in ini file  set to the default path
	if(fullpathstr.GetLength() == 0)		
	{
		if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, fullpath)))
		{
			wcscat(fullpath, L"\\SQLyog\\");
			wcscat(fullpath, L"Tags");
		}
		else
			return wyFalse;
        
		isappdatafolder = wyTrue;

		fullpathstr.SetAs(fullpath);	
		strcpy(pGlobals->m_autocompletetagsdir, fullpathstr.GetString());
	}

	if(!CreateDirectory(fullpathstr.GetAsWideChar(), NULL))
	{
        if(GetLastError() == ERROR_ALREADY_EXISTS)
			path.SetAs(fullpathstr);
        
		//If explicit path is invalid then use the 'AppData' as default for 'Tags'
		else if(isappdatafolder == wyFalse && SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, fullpath)))
		{			
			wcscat(fullpath, L"\\SQLyog\\");
			wcscat(fullpath, L"Tags");
			fullpathstr.SetAs(fullpath);	

			if(!CreateDirectory(fullpathstr.GetAsWideChar(), NULL))
			{
				if(GetLastError() == ERROR_ALREADY_EXISTS)
					path.SetAs(fullpathstr);
			}		
		}	

		else
			return wyFalse;
    }
    else
		path.SetAs(fullpathstr);

	strcpy(pGlobals->m_autocompletetagsdir, path.GetString());

	return wyTrue;	
}

// Handle the File menu items
wyBool					
ChangeFileMenuItem(HMENU hmenu)
{
	MDIWindow		*pcquerywnd;
	wyInt32 ids[] = {IDM_FILE_SAVEAS, IDM_FILE_SAVESQL};
	wyInt32 tabicon, itemcount;
	CTCITEM quetabitem = {0};
	TabEditor*		tabquery;

	//To Remove all FORMATTER OPTIONS if this is 'Professional'
	if(pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_PRO || pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_NORMAL)
	{
		HandleFileMenuOptionsProEnt(hmenu);
	}
		
	VERIFY(pcquerywnd = GetActiveWin());
	
	 if(!pcquerywnd)
		 return wyFalse;

	if(CustomTab_GetItemCount(pcquerywnd->m_pctabmodule->m_hwnd) <= 1)
		EnableMenuItem(hmenu, ID_FILE_CLOSETAB, MF_GRAYED | MF_BYCOMMAND);
	
	pGlobals->m_pcmainwin->ReInitLatestFiles();

	tabicon = pcquerywnd->m_pctabmodule->GetActiveTabImage();
	if( tabicon == IDI_QUERY_16 )
	{
		quetabitem.m_mask = quetabitem.m_mask |CTBIF_LPARAM|CTBIF_IMAGE; 
		CustomTab_GetItem(pcquerywnd->m_pctabmodule->m_hwnd, CustomTab_GetCurSel(pcquerywnd->m_pctabmodule->m_hwnd), &quetabitem);
		tabquery = (TabEditor*)quetabitem.m_lparam;
		if( tabquery->m_peditorbase->m_filename.GetLength() == 0)
			EnableMenuItem(hmenu, ID_FILE_RENAMETAB, MF_ENABLED | MF_BYCOMMAND);
		else
			EnableMenuItem(hmenu, ID_FILE_RENAMETAB, MF_GRAYED | MF_BYCOMMAND);
	}
	else
		EnableMenuItem(hmenu, ID_FILE_RENAMETAB, MF_GRAYED | MF_BYCOMMAND);

	if(!pGlobals->m_pcmainwin->m_sessionfile.GetLength())
			EnableMenuItem(hmenu, ID_CLOSESESSION, MF_GRAYED | MF_BYCOMMAND);
		else
			EnableMenuItem(hmenu, ID_CLOSESESSION, MF_ENABLED);

	if(tabicon == IDI_DATASEARCH || tabicon == IDI_TABLEINDEX || tabicon == IDI_CREATETABLE || tabicon == IDI_ALTERTABLE)
    {
        for(itemcount=0; itemcount < (sizeof(ids)/sizeof(ids[0])); itemcount++)
			VERIFY(EnableMenuItem(hmenu, ids[itemcount], MF_GRAYED)!= -1);		
	}

	


	return wyTrue;
}


// Function enables and disables various edit menu item like cut, copy, paste etc. depending upon the current status
// of the edit box in the query window.
wyBool
ChangeEditMenuItem(HMENU hmenu)
{
	CHARRANGE		chr;
	HWND			hwnd;
    wyInt32			itemcount, state = MF_ENABLED, tabicon = 0;

	MDIWindow		*pcquerywnd;
    EditorBase		*pceditorbase;
	
	pcquerywnd = GetActiveWin(); 

    wyInt32 ids[] = {IDM_EDIT_UNDO, IDM_EDIT_REDO, IDM_EDIT_PASTE, IDM_EDIT_SELECTALL, IDM_EDIT_CUT,
                     IDM_EDIT_COPY, IDM_COPYNORMALIZED, IDM_EDIT_REPLACE, IDM_EDIT_FIND, IDM_EDIT_FINDNEXT, IDM_EDIT_GOTO,
                     ID_EDIT_LISTALLTAGS, ID_EDIT_LISTMATCHINGTAGS, ID_EDIT_LISTFUNCTIONANDROUTINEPARAMETERS, ACCEL_EXECUTE_MENU, ACCEL_EXECUTEALL,
                     ACCEL_QUERYUPDATE, ACCEL_EXECUTESEL, IDM_EDIT_ADVANCED_UPPER, IDM_EDIT_ADVANCED_LOWER,
					 ACCEL_FORMATALLQUERIES, ACCEL_FORMATCURRENTQUERY, ACCEL_FORMATSELECTEDQUERY, ID_EXPLAIN_EXPLAIN, ID_EXPLAIN_EXTENDED};

    wyInt32 ids2[] = { IDM_REFRESHOBJECT, IDM_COLLAPSEOBJECT, IDM_EDIT_RESULT_TEXT, IDC_EDIT_SHOWOBJECT,
                       IDC_EDIT_SHOWRESULT, IDC_EDIT_SHOWEDIT, 
                       IDM_EDIT_ADVANCED_COMMENT, IDM_EDIT_ADVANCED_REMOVE, 
					   ID_EDIT_SWITCHTABSTOLEFT, ID_EDIT_SWITCHTABSTORIGHT, ID_EDIT_INSERTFILE};

	wyInt32 idsdisable[] = {IDC_EDIT_SHOWRESULT, IDC_EDIT_SHOWEDIT, IDM_EDIT_ADVANCED_UPPER,
							IDM_EDIT_ADVANCED_LOWER, IDM_EDIT_ADVANCED_COMMENT, IDM_EDIT_ADVANCED_REMOVE, ID_EDIT_INSERTFILE};

	/*wyInt32 disableforsearch[] = {IDC_EDIT_SHOWRESULT, IDC_EDIT_SHOWEDIT, IDM_EDIT_ADVANCED_UPPER,
							IDM_EDIT_ADVANCED_LOWER, IDM_EDIT_ADVANCED_COMMENT, IDM_EDIT_ADVANCED_REMOVE, ID_EDIT_INSERTFILE};*/                

	//setting Execute query submenus and object browser refresh  when switching F5 and F9 functionalities
	ChangeMenuItemOnPref(hmenu, MNUEDIT_INDEX);

	if(pGlobals->m_conncount == 0)
		state = MF_GRAYED;
	               
    // Disable All first
    for(itemcount=0; itemcount < (sizeof(ids)/sizeof(ids[0])); itemcount++)
        VERIFY(EnableMenuItem(hmenu, ids[itemcount], MF_GRAYED)!= -1);

    for(itemcount=0; itemcount < (sizeof(ids2)/sizeof(ids2[0])); itemcount++)
		VERIFY(EnableMenuItem(hmenu, ids2[itemcount], state)!= -1);

	//To Remove all FORMATTER OPTIONS if this is 'Professional'
	if(pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_PRO)
		HandleFomatterOptionsPRO(hmenu);

    //ToRemove all EXPLAIN OPTIONS if this is not ULTIMATE
    if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_ULTIMATE && pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_TRIAL)
    {
        HandleRemoveExplainOptions(hmenu);
    }

	if(!pcquerywnd)
		return wyTrue;

	if(CustomTab_GetItemCount(pcquerywnd->m_pctabmodule->m_hwnd) <= 1)
	{
		EnableMenuItem(hmenu, ID_EDIT_SWITCHTABSTOLEFT, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_EDIT_SWITCHTABSTORIGHT, MF_GRAYED | MF_BYCOMMAND);
	}

	hwnd         = GetFocus();
		
	//Disable menu items if the current tab is QB or SD
	tabicon = pcquerywnd->m_pctabmodule->GetActiveTabImage();
	
	if(tabicon == IDI_DATASEARCH || pcquerywnd->GetActiveTabEditor() == NULL)
    {
        for(itemcount=0; itemcount < (sizeof(idsdisable)/sizeof(idsdisable[0])); itemcount++)
			VERIFY(EnableMenuItem(hmenu, idsdisable[itemcount], MF_GRAYED)!= -1);		
		if ((pcquerywnd->GetActiveHistoryTab() && hwnd == pcquerywnd->GetActiveHistoryTab()->m_hwnd) 
		    || (pcquerywnd->GetActiveInfoTab() && hwnd == pcquerywnd->GetActiveInfoTab()->m_hwnd))
		{
			EnableMenuItem(hmenu, IDM_EDIT_FIND, MF_ENABLED);
			EnableMenuItem(hmenu, IDM_EDIT_FINDNEXT, MF_ENABLED);
		}

		if(tabicon == IDI_HISTORY || tabicon == IDI_TABLEINDEX || tabicon == IDI_CREATETABLE || 
            tabicon == IDI_ALTERTABLE || tabicon == IDI_SCHEMADESIGNER_16 || tabicon == IDI_QUERYBUILDER_16)
			EnableMenuItem(hmenu, IDM_EDIT_RESULT_TEXT, MF_DISABLED);

		return wyTrue;

   }
	
	pceditorbase = pcquerywnd->GetActiveTabEditor()->m_peditorbase;		
	
	if(hwnd == pcquerywnd->m_pcqueryobject->m_hwnd)
		return wyTrue;

	chr.cpMin = SendMessage(pceditorbase->m_hwnd, SCI_GETSELECTIONSTART, 0, 0);
	chr.cpMax = SendMessage(pceditorbase->m_hwnd, SCI_GETSELECTIONEND, 0, 0);

	if(SendMessage(pceditorbase->m_hwnd, WM_GETTEXTLENGTH, 0, 0))
    {
		VERIFY(EnableMenuItem(hmenu, ACCEL_EXECUTE_MENU, MF_ENABLED)!= -1);
		VERIFY(EnableMenuItem(hmenu, ACCEL_EXECUTEALL, MF_ENABLED)!= -1);
		VERIFY(EnableMenuItem(hmenu, IDM_EDIT_GOTO, MF_ENABLED)!= -1);

        if(pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_ULTIMATE || pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_TRIAL)
        {   
            SetExplainMenuItems(hmenu, pceditorbase->m_hwnd);
            //VERIFY(EnableMenuItem(hmenu, ID_EXPLAIN_EXPLAIN, MF_ENABLED)!= -1);
		    //VERIFY(EnableMenuItem(hmenu, ID_EXPLAIN_EXTENDED, MF_ENABLED)!= -1);
        }

		if(pceditorbase->GetAdvancedEditor() == wyFalse)
		{
			VERIFY(EnableMenuItem(hmenu, ACCEL_QUERYUPDATE, MF_ENABLED)!= -1);

			/*if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
			{
				VERIFY(EnableMenuItem(hmenu, ACCEL_FORMATALLQUERIES, MF_ENABLED)!= -1);
				VERIFY(EnableMenuItem(hmenu, ACCEL_FORMATCURRENTQUERY , MF_ENABLED)!= -1);
			}
		
			if(chr.cpMin != chr.cpMax )
			{
				VERIFY(EnableMenuItem(hmenu, ACCEL_EXECUTESEL, MF_ENABLED)!= -1);
				if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO && IsQuerySupportFormatting(pceditorbase->m_hwnd) == wyTrue)
					VERIFY(EnableMenuItem(hmenu, ACCEL_FORMATSELECTEDQUERY, MF_ENABLED)!= -1);
			}*/
		}
        else
		{	
			VERIFY(EnableMenuItem(hmenu, ACCEL_EXECUTE_MENU, MF_GRAYED) != -1);
			/*if(chr.cpMin != chr.cpMax )
			{
				if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO && IsQuerySupportFormatting(pceditorbase->m_hwnd) == wyTrue)
					VERIFY(EnableMenuItem(hmenu, ACCEL_FORMATSELECTEDQUERY, MF_ENABLED)!= -1);
			}*/
		}

        if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
		{
			VERIFY(EnableMenuItem(hmenu, ACCEL_FORMATALLQUERIES, MF_ENABLED)!= -1);
			VERIFY(EnableMenuItem(hmenu, ACCEL_FORMATCURRENTQUERY , MF_ENABLED)!= -1);
            VERIFY(EnableMenuItem(hmenu, ACCEL_FORMATSELECTEDQUERY , MF_ENABLED)!= -1);
		}
		
		/*if(chr.cpMin != chr.cpMax )
		{
			VERIFY(EnableMenuItem(hmenu, ACCEL_EXECUTESEL, MF_ENABLED)!= -1);
			if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO && IsQuerySupportFormatting(pceditorbase->m_hwnd) == wyTrue)
				VERIFY(EnableMenuItem(hmenu, ACCEL_FORMATSELECTEDQUERY, MF_ENABLED)!= -1);
		}*/
	}

#ifndef COMMUNITY
	if(pcquerywnd->m_conninfo.m_isreadonly == wyTrue)
	{
		VERIFY(EnableMenuItem(hmenu, ACCEL_QUERYUPDATE, MF_GRAYED)!= -1);
	}
#endif

	// Check if UNDO, REDO and PASTE can be done.
	if((hwnd == pceditorbase->m_hwnd)&&(SendMessage(hwnd, SCI_CANUNDO, 0, 0)))
		VERIFY(EnableMenuItem(hmenu, IDM_EDIT_UNDO, MF_ENABLED)!= -1);
	
	if((hwnd == pceditorbase->m_hwnd)&&(SendMessage(hwnd, SCI_CANREDO, 0, 0)))
		VERIFY(EnableMenuItem(hmenu, IDM_EDIT_REDO, MF_ENABLED)!= -1);

	if((hwnd == pceditorbase->m_hwnd)&&(SendMessage(hwnd, SCI_CANPASTE, 0, 0)))
		VERIFY(EnableMenuItem(hmenu, IDM_EDIT_PASTE, MF_ENABLED)!= -1);

	// check whether everything can be selected or not.
	if((hwnd == pceditorbase->m_hwnd)&&(SendMessage(hwnd, SCI_GETTEXTLENGTH, 0, 0))> 0)
    {
		VERIFY( EnableMenuItem(hmenu, IDM_EDIT_SELECTALL, MF_ENABLED)!= -1);
		//VERIFY(EnableMenuItem(hmenu, IDM_EDIT_CLEAR, MF_ENABLED)!= -1);		
	}

	// also check if the editor is viewable or not.
	if(pcquerywnd->GetActiveTabEditor()->m_iseditwnd)
		VERIFY(EnableMenuItem(hmenu, ID_EDIT_INSERTTEMPLATES, MF_ENABLED)!= -1);
	else
		VERIFY(EnableMenuItem(hmenu, ID_EDIT_INSERTTEMPLATES, MF_GRAYED)!= -1);

	// Copy, Make Upper case, Make Lower case
    EnableMenuItem(hmenu, IDM_EDIT_CUT, chr.cpMin != chr.cpMax ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hmenu, IDM_EDIT_COPY, chr.cpMin != chr.cpMax ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hmenu, IDM_COPYNORMALIZED, chr.cpMin != chr.cpMax ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hmenu, IDM_EDIT_ADVANCED_UPPER, chr.cpMin != chr.cpMax ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hmenu, IDM_EDIT_ADVANCED_LOWER, chr.cpMin != chr.cpMax ? MF_ENABLED : MF_GRAYED);

	// Now see if find and replace will work. because it must be disabled if the focus is not on 
	// edit or result edit then we disable it.
	hwnd = GetFocus();
	if(hwnd == pceditorbase->m_hwnd)
	{
		VERIFY(EnableMenuItem(hmenu, IDM_EDIT_FIND, MF_ENABLED)!= -1);
        VERIFY(EnableMenuItem(hmenu, IDM_EDIT_FINDNEXT, MF_ENABLED)!= -1);
		VERIFY(EnableMenuItem(hmenu, IDM_EDIT_REPLACE, MF_ENABLED)!= -1);
	}
	else 
	{
		{
			VERIFY(EnableMenuItem(hmenu, IDM_EDIT_REPLACE, MF_GRAYED)!= -1);
			VERIFY(EnableMenuItem(hmenu, IDM_EDIT_FIND, MF_GRAYED)!= -1);
            VERIFY(EnableMenuItem(hmenu, IDM_EDIT_FINDNEXT, MF_GRAYED)!= -1);
		}
	}
	
	return wyTrue;
}

void
GetCurrentQueryForAuto(HWND hwndedit, MDIWindow *wnd, wyString *query, wyInt32 &pos)
{
	wyInt32				curpos, curline = 0, length, querycount = 0;
	SQLTokenizer		*tok;
	const wyChar        *token;
	const wyChar		*dumpQuery;
	wyChar              *buffer;
	wyUInt32            linenum = 0, len = 0;              
	wyChar              delimiter[256] = {0};
	wyInt32             isdel = 0;
	wyUInt32			nstrlen;
	wyInt32				numquery = 0;
	wyInt32             delimiterlen = 0;
	wyString			tempQuery;
	wyInt32				charCount = 0;

	GetCurrentQuery(hwndedit, wnd, query, pos);
	if(pos <= 0 || query->GetLength() == 0)
	{
		return;
	}

	tempQuery.SetAs(query->Substr(0, pos));
	
	dumpQuery = tempQuery.GetString();

	for(charCount= 0; charCount < pos; charCount++)
	{
		if(dumpQuery[charCount] == '\n')
		{
			curline++;
		}
	}

	curpos = pos;  
	nstrlen = query->GetLength();
	if(nstrlen == 0)
		return;
	buffer = AllocateBuff(nstrlen + 1);
	if(!buffer)
		return;
	
	memcpy(buffer, query->GetString(), nstrlen);
	
	length		= nstrlen;
	tok = new SQLTokenizer(wnd->m_tunnel, &wnd->m_mysql, SRC_BUFFER, (void*)buffer, length);
	if(!tok)
	{
		free(buffer);
		return;
	}
	len = nstrlen;
	length		= 0;
    linenum = curline;

	delimiterlen  = strlen(delimiter);
	
	while(token = tok->GetQuery(&len, &linenum, &isdel, delimiter))
	{
		len = strlen(token); 		
		linenum = curline;	
		
		if(curpos < (wyInt32)(length+len+(numquery+1)) ||( querycount == 0 && curpos< (wyInt32)(length + len + numquery + 1 + GetQuerySpaceLen(len, buffer))))
		{
			if(delimiterlen == 0)
				pos = pos - (querycount + length);
			query->SetAs(token);
			free(buffer);
			delete tok;
			return;
		}
        querycount++;
		/* increase the internal length of the query */	
		length += ((len)+ strlen(delimiter));
		
		/* increase the query count */
		numquery++;
	}
	free(buffer);
	delete tok;
}

void
GetCurrentQuery(HWND hwndedit, MDIWindow *wnd, wyString *query, wyInt32 &pos)
{
	wyInt32 curpos,     curline, length, querycount = 0;
	SQLTokenizer		*tok;
	const wyChar        *token;
	wyChar              *buffer;
	wyUInt32            linenum = 0, len = 0;              
	wyChar              delimiter[256] = {0};
	wyInt32             isdel = 0;
	wyUInt32			nstrlen;
	wyInt32				numquery = 0;
	wyInt32             delimiterlen = 0;

	VERIFY(curpos = SendMessage(hwndedit, SCI_GETCURRENTPOS, 0, 0));
	VERIFY(curline = SendMessage(hwndedit, SCI_LINEFROMPOSITION, curpos, 0));

    pos = curpos;   
	nstrlen = SendMessage(hwndedit, SCI_GETTEXTLENGTH, 0, 0);
	if(nstrlen == 0)
		return;
	buffer = AllocateBuff(nstrlen + 1);
	if(!buffer)
		return;
	SendMessage(hwndedit, SCI_GETTEXT, (WPARAM)nstrlen+1, (LPARAM)buffer);

	length		= nstrlen;
	tok = new SQLTokenizer(wnd->m_tunnel, &wnd->m_mysql, SRC_BUFFER, (void*)buffer, length);
	if(!tok)
	{
		free(buffer);
		return;
	}
	len = nstrlen;
	length		= 0;
    linenum = curline;

	delimiterlen  = strlen(delimiter);
	
	while(token = tok->GetQuery(&len, &linenum, &isdel, delimiter))
	{
		len = strlen(token); 		
		linenum = curline;	
		delimiterlen  = strlen(delimiter);
		
		if(curpos < (wyInt32)(length+len+(numquery+1)) ||( querycount == 0 && curpos< (wyInt32)(length + len + numquery + 1 + GetQuerySpaceLen(len, buffer))))
		{
			if(delimiterlen == 0 || !strcmp(delimiter, ";"))
				pos = pos - (querycount + length) + strlen(delimiter);
			else
				pos = pos - length;
			query->SetAs(token);
			free(buffer);
			delete tok;
			return;
		}
        querycount++;
		/* increase the internal length of the query */	
		length += ((len)+ strlen(delimiter));
		
		/* increase the query count */
		numquery++;
	}
	free(buffer);
	delete tok;
}


void
SetExplainMenuItems(HMENU hmenu, HWND hwnd)
{
    wyInt32			    start, end, curpos, curline, queryLength = 0; 
    wyString            query, queryExplain, tempdump;
    MENUITEMINFO        lpmii = {0};
    wyChar              *tmp = NULL;
    MDIWindow           *wnd = GetActiveWin();
    SQLFormatter        formatter;

    start	= SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	end		= SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);
    curpos	= SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
	curline = SendMessage(hwnd, SCI_LINEFROMPOSITION, curpos, 0);
    
    lpmii.cbSize = sizeof(MENUITEMINFO);
    lpmii.fMask = MIIM_STRING | MIIM_ID | MIIM_DATA;
    
    query.Clear();
    if((end - start) > 1)
    {
        tmp = AllocateBuff(end - start + 1);
		SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)tmp);
        tempdump.SetAs(tmp);
        free(tmp);
    }
    else
    {
        GetCurrentQuery(hwnd, GetActiveWin(), &tempdump, curpos);
    }
    
    formatter.GetQueryWtOutComments(&tempdump, &query);

    queryLength = query.GetLength();

    if(queryLength)
    {       
        if(queryLength > 15)
            queryExplain.Sprintf("EXPLAIN %.15s...", query.GetString());
        else
            queryExplain.Sprintf("EXPLAIN %s", query.GetString());

        lpmii.wID = ID_EXPLAIN_EXPLAIN;
        lpmii.dwItemData = (ULONG_PTR)queryExplain.GetAsWideChar();
        lpmii.cch = queryExplain.GetLength() + 1;
        lpmii.dwTypeData = queryExplain.GetAsWideChar();
        SetMenuItemInfo(hmenu, ID_EXPLAIN_EXPLAIN, FALSE, &lpmii);
        
        if(queryLength > 10)
            queryExplain.Sprintf("EXPLAIN EXTENDED %.10s...", query.GetString());
        else
            queryExplain.Sprintf("EXPLAIN EXTENDED %s", query.GetString());

        
        lpmii.wID = ID_EXPLAIN_EXTENDED;
        lpmii.dwItemData = (ULONG_PTR)queryExplain.GetAsWideChar();
        lpmii.cch = queryExplain.GetLength() + 1;
        lpmii.dwTypeData = queryExplain.GetAsWideChar();
        SetMenuItemInfo(hmenu, ID_EXPLAIN_EXTENDED, FALSE, &lpmii);
        
        if(IsQuerySELECT(query.GetString()) || 
            (IsMySQL563(wnd->m_tunnel, &wnd->m_mysql) && IsQueryDeleteInsertReplaceUpdate(query.GetString())))
        {
            VERIFY(EnableMenuItem(hmenu, ID_EXPLAIN_EXPLAIN, MF_ENABLED)!= -1);
		    VERIFY(EnableMenuItem(hmenu, ID_EXPLAIN_EXTENDED, MF_ENABLED)!= -1);
            return;
        }
    }
    else
    {
        queryExplain.SetAs("EXPLAIN <Query>");
        lpmii.wID = ID_EXPLAIN_EXPLAIN;
        lpmii.dwItemData = (ULONG_PTR)queryExplain.GetAsWideChar();
        lpmii.cch = queryExplain.GetLength() + 1;
        lpmii.dwTypeData = queryExplain.GetAsWideChar();
        SetMenuItemInfo(hmenu, ID_EXPLAIN_EXPLAIN, FALSE, &lpmii);
        
        queryExplain.SetAs("EXPLAIN EXTENDED <Query>");
        lpmii.wID = ID_EXPLAIN_EXTENDED;
        lpmii.dwItemData = (ULONG_PTR)queryExplain.GetAsWideChar();
        lpmii.cch = queryExplain.GetLength() + 1;
        lpmii.dwTypeData = queryExplain.GetAsWideChar();
        SetMenuItemInfo(hmenu, ID_EXPLAIN_EXTENDED, FALSE, &lpmii);
    }

    VERIFY(EnableMenuItem(hmenu, ID_EXPLAIN_EXPLAIN, MF_DISABLED)!= -1);
	VERIFY(EnableMenuItem(hmenu, ID_EXPLAIN_EXTENDED, MF_DISABLED)!= -1);
    DrawMenuBar(pGlobals->m_pcmainwin->m_hwndmain);
}

//To remove all EXPLAIN options if this is ULTIMATE
void
HandleRemoveExplainOptions(HMENU hmenu)
{   
    HMENU	hsubmenu;
	wyInt32	itemcount = 0;	
	wyWChar name[MAX_PATH];

	hsubmenu = GetSubMenu(hmenu, 3); //Formatter options
	itemcount = GetMenuItemCount(hsubmenu);
		    
	if(itemcount)
	{
		GetMenuString(hmenu, 3, name, MAX_PATH, MF_BYPOSITION); 

		if(!wcsicmp(name, _(L"Explain")))
		{
            RemoveMenu(hmenu, ID_EXPLAIN_EXPLAIN, MF_BYCOMMAND);
            RemoveMenu(hmenu, ID_EXPLAIN_EXTENDED, MF_BYCOMMAND);
    
			RemoveMenu(hmenu, 3, MF_BYPOSITION);
			DrawMenuBar(pGlobals->m_pcmainwin->m_hwndmain);
		}
	}
}


//To Remove all FORMATTER OPTIONS if this is 'Professional'
VOID
HandleFomatterOptionsPRO(HMENU hmenu)
{
	HMENU	hsubmenu;
	wyInt32	itemcount = 0;	
	wyWChar name[MAX_PATH];

	hsubmenu = GetSubMenu(hmenu, 5); //Formatter options
	itemcount = GetMenuItemCount(hsubmenu);
		    
	if(itemcount)
	{
		GetMenuString(hmenu, 5, name, MAX_PATH, MF_BYPOSITION); 

		if(!wcsicmp(name, _(L"S&QL Formatter")))
		{
			RemoveMenu(hmenu, 5, MF_BYPOSITION);
			DrawMenuBar(pGlobals->m_pcmainwin->m_hwndmain);
		}
	}		
}

//Remove(New SD, New QB, Recent SD files) options for PRO
VOID
HandleFileMenuOptionsProEnt(HMENU hmenu)
{
	wyInt32	itemcount = 0;	

	if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO && pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_NORMAL)
	{
		return;
	}

	itemcount = GetMenuItemCount(hmenu);
	
	if(itemcount)
	{
		/*GetMenuString(hmenu, 17, name, MAX_PATH, MF_BYPOSITION); 

		if(!wcsicmp(name, _(L"R&ecent Schema Design Files")))
			RemoveMenu(hmenu, 17, MF_BYPOSITION);*/
		
		if(pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_PRO)
		{
			RemoveMenu(hmenu, ID_QUERYBUILDER, MF_BYCOMMAND);
			RemoveMenu(hmenu, ID_SCHEMADESIGNER, MF_BYCOMMAND);	
			RemoveMenu(hmenu, ID_STARTTRANSACTION_WITHNOMODIFIER, MF_BYCOMMAND);
			RemoveMenu(hmenu, ID_COMMIT_WITHNOMODIFIER, MF_BYCOMMAND);
			RemoveMenu(hmenu, ID_ROLLBACK_TRANSACTION, MF_BYCOMMAND);
		}
		if(pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_PRO ||
			pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_NORMAL)
		{
			RemoveMenu(hmenu, ID_DATASEARCH, MF_BYCOMMAND);	
		}

		DrawMenuBar(pGlobals->m_pcmainwin->m_hwndmain);
	}
}

VOID
HandleDBMenuOptionsPRO(HMENU hmenu)
{
	wyInt32	itemcount = 0;	

	if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
		return;

	itemcount = GetMenuItemCount(hmenu);

	if(itemcount)
	{		
		RemoveMenu(hmenu, ID_POWERTOOLS_SCHEDULEEXPORT2, MF_BYCOMMAND);
		RemoveMenu(hmenu, ID_IMPORT_EXTERNAL_DATA, MF_BYCOMMAND);	

		DrawMenuBar(pGlobals->m_pcmainwin->m_hwndmain);
	}
}

//Removing all menu items otherthan re-build tags
VOID
HandlePowertoolsOptionsPRO(HMENU hmenu)
{
	wyInt32 count = 0, i = 0;

	if(!hmenu)
		return;

	count = GetMenuItemCount(hmenu);

	for(i = count - 2; i >= 0 ; i--)
	{
		RemoveMenu(hmenu, i, MF_BYPOSITION);		
	}

	if(count > 1)
		DrawMenuBar(pGlobals->m_pcmainwin->m_hwndmain);
}

// Function disables or enables Tool menu item depending upon what type of item is 
wyBool
EnableToolItems(HMENU hmenu)
{
    MySQLDataEx*    data = NULL;
	MDIWindow*      pcquerywnd = GetActiveWin();
    TabTableData*   ptabledata;
#ifndef COMMUNITY
    TabDbSearch*    pdbsearch;
#endif

    EnableMenuItem(hmenu, ID_TOOLS_CHANGELANGUAGE, 
        pGlobals->m_pcmainwin->m_languagecount ? MF_ENABLED : MF_GRAYED | MF_BYCOMMAND);

    if(pcquerywnd->GetActiveTabEditor())
    {
        data = pcquerywnd->GetActiveTabEditor()->m_pctabmgmt->GetResultData();
        EnableMenuItem(hmenu, IDM_IMEX_EXPORTDATA, 
                data && data->m_datares ? MF_ENABLED : MF_GRAYED | MF_BYCOMMAND);
    }
    else if(pcquerywnd->m_pctabmodule->GetActiveTabImage() == IDI_TABLE)
    {
        ptabledata = (TabTableData*)pcquerywnd->m_pctabmodule->GetActiveTabType();
        EnableMenuItem(hmenu, IDM_IMEX_EXPORTDATA, 
                ptabledata->m_tabledata && ptabledata->m_tabledata->m_datares ? MF_ENABLED : MF_GRAYED | MF_BYCOMMAND);
    }
#ifndef COMMUNITY
    else if(pcquerywnd->m_pctabmodule->GetActiveTabImage() == IDI_DATASEARCH)
    {
        pdbsearch = (TabDbSearch*)pcquerywnd->m_pctabmodule->GetActiveTabType();
        data = pdbsearch->m_pdataview->GetData();
        EnableMenuItem(hmenu, IDM_IMEX_EXPORTDATA, 
                data && data->m_datares ? MF_ENABLED : MF_GRAYED | MF_BYCOMMAND);
    }
#endif
    else
    {
        EnableMenuItem(hmenu, IDM_IMEX_EXPORTDATA, MF_GRAYED | MF_BYCOMMAND);
    }
    EnableDisableExportMenuItem();
#ifndef COMMUNITY
	if(pcquerywnd->m_conninfo.m_isreadonly == wyTrue)
	{
		EnableMenuItem(hmenu, ID_TOOLS_FLUSH,  MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_IMEX_TEXTFILE2, MF_GRAYED | MF_BYCOMMAND);
	}
#endif
    return wyTrue;
}

// Function to enable and disable all the items in DB menu depending upon the
// item selected in the object browser.

wyBool
EnableDBItems(HMENU hmenu)
{
	wyInt32	image, size, advsize, count, itemcount, state;
	wyInt32	nid[] = {ID_DB_TABLE_MAKER,IDM_ALTERDATABASE, ID_OPEN_COPYDATABASE, ID_OBJECT_TRUNCATEDATABASE, 
                     ID_OBJECT_DROPDATABASE, ID_OBJECT_EMPTYDATABASE, ID_OBJECT_CREATESCHEMA/*, ID_DATABASE_REBUILDTAGS vgladcode*/};
	wyInt32	advnid[] = { ID_DB_CREATEVIEW, ID_DB_CREATESTOREDPROCEDURE, ID_DB_CREATEFUNCTION , ID_DB_CREATETRIGGER};

	MDIWindow   *pcquerywnd;

	
	if(pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_PRO)
		HandleDBMenuOptionsPRO(hmenu);

	VERIFY(pcquerywnd = GetActiveWin());
	//setting object browser refresh  when switching F5 and F9 functionalities
	ChangeMenuItemOnPref(hmenu, MNUDB_INDEX);

	// fisrt disable all the items in the database.
	state = MF_GRAYED;

	itemcount = GetMenuItemCount(hmenu);
	
	if(itemcount == 5)
		return wyTrue;
	
	size	=	sizeof(nid)/ sizeof(nid[0]);
	advsize	=	sizeof(advnid)/ sizeof(advnid[0]);

	for(count = 0; count < size; count++)
		EnableMenuItem(hmenu, nid[count], state | MF_BYCOMMAND);

	for(count = 0; count < advsize; count++)
		EnableMenuItem(hmenu, advnid[count], state | MF_BYCOMMAND);



	//first disable create event menu 
	EnableMenuItem(hmenu, ID_DB_CREATEEVENT, MF_GRAYED| MF_BYCOMMAND);

#ifndef COMMUNITY
	if(pcquerywnd->m_conninfo.m_isreadonly == wyTrue)
	{
		EnableMenuItem(hmenu, ID_IMEX_TEXTFILE, MF_GRAYED| MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_IMPORT_EXTERNAL_DATA, MF_GRAYED| MF_BYCOMMAND);
		EnableMenuItem(hmenu, IDM_CREATEDATABASE, MF_GRAYED| MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_OBJECT_CREATESCHEMA, MF_ENABLED| MF_BYCOMMAND);
	/*	EnableMenuItem(hmenu, ID_DATABASE_REBUILDTAGS, MF_ENABLED| MF_BYCOMMAND); vgladcode*/

		//EnableMenuItem(hmenu, ID_OPEN_COPYDATABASE, MF_GRAYED| MF_BYCOMMAND);
		return wyTrue;
	}
#endif
	// check whether database item is selected or not.
	image = pcquerywnd->m_pcqueryobject->GetSelectionImage();

	if(image == NDATABASE ||  image == NTABLES || image == NTABLE || image == NPRIMARYKEY || image == NCOLUMN || image == NINDEX || 
		image == NPRIMARYINDEX || image == NSP || image == NSPITEM || image == NVIEWS || image == NVIEWSITEM || image == NEVENTITEM || 
		image == NEVENTS || image == NFUNC || image == NFUNCITEM || image == NTRIGGER || image == NTRIGGERITEM ||  image == NFOLDER)
	{
		state = MF_ENABLED;

		for(count = 0; count < size; count++)
			EnableMenuItem(hmenu, nid[count], state | MF_BYCOMMAND);

		//if(pGlobals->m_isautocomplete == wyFalse)				vgladcode
		//EnableMenuItem(hmenu, ID_DATABASE_REBUILDTAGS, MF_GRAYED| MF_BYCOMMAND);

		if(! IsMySQL41(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql ))
			EnableMenuItem(hmenu, IDM_ALTERDATABASE, MF_GRAYED);

		if(IsMySQL5010(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql))
		{
			for(count = 0; count < advsize; count++)
				EnableMenuItem(hmenu, advnid[count], state | MF_BYCOMMAND);
			if(IsMySQL516(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql))
				EnableMenuItem(hmenu, ID_DB_CREATEEVENT, MF_ENABLED|MF_BYCOMMAND);
		}
	}
	
	return wyTrue;

}

// Function to enable and disable all the items in Table menu depending upon the
// item selected in the object browser.
wyBool
EnableTableItems(HMENU hmenu)
{
	HMENU hmenuotheroptions = NULL,	hmenuengine = NULL;
	MDIWindow   *pcquerywnd;
	wyInt32 image, size, count, state, uptorelation = 5;
	wyInt32 nid[] = {ID_OPEN_COPYTABLE, ID_TABLE_MAKER, ID_OBJECT_TABLEEDITOR, ID_OBJECT_MANINDEX, 
                     IDM_TABLE_RELATION,  ID_IMPORT_FROMCSV, ID_IMPORT_FROMXML,
                     ID_OBJECT_COPYTABLE, ID_OBJECT_RENAMETABLE, ID_OBJECT_CLEARTABLE, ID_OBJECT_REORDER, 
                     ID_OBJECT_DROPTABLE, ID_OBJECT_REORDER, 
                     ID_OBJECT_INSERTSTMT, ID_OBJECT_UPDATESTMT, ID_OBJECT_DELETESTMT, ID_TABLE_MAKER, ID_OBJECT_CREATETRIGGER, 
					 ID_OBJECT_VIEWDATA, ID_TABLE_OPENINNEWTAB, ID_OBJECT_ADVANCED,
                     ID_IMPORTEXPORT_TABLESEXPORTTABLES, ID_EXPORT_AS, ID_OBJECT_SELECTSTMT, ID_EXPORT_EXPORTTABLEDATA};

	if(pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_PRO)
		HandleDBMenuOptionsPRO(hmenu);

	VERIFY(pcquerywnd	=	GetActiveWin());

	// first grey all the item
	state = MF_GRAYED;

	size = sizeof(nid)/ sizeof(nid[0]);

	for(count = 0; count < size; count++)
		EnableMenuItem(hmenu, nid[count], state | MF_BYCOMMAND);

    if(!pcquerywnd)
        return wyFalse;
	
	// check whether database item is selected or not.
	image = pcquerywnd->m_pcqueryobject->GetSelectionImage();

	if((image == NTABLES || image == NDATABASE ))
    {
#ifndef COMMUNITY
		if(pcquerywnd->m_conninfo.m_isreadonly == wyFalse)
        //For database enable only - 'Create Table' 
		{
#endif COMMUNITY
		EnableMenuItem(hmenu, ID_TABLE_MAKER, MF_ENABLED | MF_BYCOMMAND);

        //for tables folder 
        if(image == NTABLES)
        {
            EnableMenuItem(hmenu, ID_OPEN_COPYTABLE, MF_ENABLED | MF_BYCOMMAND);
            EnableMenuItem(hmenu, ID_IMPORTEXPORT_TABLESEXPORTTABLES, MF_ENABLED | MF_BYCOMMAND);
        }
#ifndef COMMUNITY
		}
#endif COMMUNITY
    }	
	else if(image == NTABLE || image == NCOLUMN || image == NPRIMARYKEY || image == NINDEX || image == NPRIMARYINDEX)
	{
		state = MF_ENABLED;
		count = 0;
#ifndef COMMUNITY
	if(pcquerywnd->m_conninfo.m_isreadonly == wyTrue)
		count = READONLYUNCHECKTILLHERE; //nid before 18 must remain disabled for readonly
#endif
		for(; count < size; count++)
			EnableMenuItem(hmenu, nid[count], state | MF_BYCOMMAND);

		if(!IsMySQL5010(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql))
			EnableMenuItem(hmenu, ID_OBJECT_CREATETRIGGER,  MF_GRAYED | MF_BYCOMMAND);
		if(!IsMySQL5500(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql))
			EnableMenuItem(hmenu,  ID_IMPORT_FROMXML,  MF_GRAYED | MF_BYCOMMAND);

	}
	else if(image == NFOLDER)
	{
#ifndef COMMUNITY
		if(pcquerywnd->m_conninfo.m_isreadonly == wyTrue)
        state = MF_GRAYED;
#else
		state = MF_ENABLED;
#endif
		for(count = 0; count < uptorelation; count++)
			VERIFY(EnableMenuItem(hmenu, nid[count], state | MF_BYCOMMAND)!= -1);
	}

    //If the Active tab is QB should disable these items.
	if(!pcquerywnd->GetActiveTabEditor() || (pcquerywnd->m_pctabmodule->GetActiveTabImage() == IDI_DATASEARCH))
    {
        VERIFY(EnableMenuItem(hmenu, ID_OBJECT_INSERTSTMT, MF_GRAYED | MF_BYCOMMAND)!= -1);
		VERIFY(EnableMenuItem(hmenu, ID_OBJECT_UPDATESTMT, MF_GRAYED | MF_BYCOMMAND)!= -1);
		VERIFY(EnableMenuItem(hmenu, ID_OBJECT_DELETESTMT, MF_GRAYED | MF_BYCOMMAND)!= -1);
        VERIFY(EnableMenuItem(hmenu, ID_OBJECT_SELECTSTMT, MF_GRAYED | MF_BYCOMMAND)!= -1);        
    }  
#ifndef COMMUNITY
	if(pcquerywnd->m_conninfo.m_isreadonly == wyTrue)
	{
		EnableMenuItem(hmenu, ID_IMPORT_EXTERNAL_DATA, MF_GRAYED | MF_BYCOMMAND);
	}
	else
	{
		EnableMenuItem(hmenu, ID_IMPORT_EXTERNAL_DATA, MF_BYCOMMAND);
	}
#endif
	
	/*Adding Table engines to Table menu.
	 Gets the 'Other Table Option' sub meno
	*/
	VERIFY(hmenuotheroptions = GetSubMenu(hmenu, 9));
	if(!hmenuotheroptions)
		return wyFalse;
	
	//Gets the 'Engine' sub menu
	VERIFY(hmenuengine = GetSubMenu(hmenuotheroptions, 6));
	if(!hmenuengine)
		return wyFalse;
    
	pcquerywnd->InsertEnginesMenuItems(hmenuengine);	  
	return wyTrue;
}

// Function to enable and disable all the items in Table menu depending upon the
// item selected in the object browser.
wyBool
EnableColumnItems(HMENU hmenu)
{
	wyInt32      image;
	MDIWindow	*pcquerywnd;

	pcquerywnd = GetActiveWin();

	if(!pcquerywnd)
	{
		DisableAdvMenu(hmenu);
		return wyFalse;
	}
	// if mysql version is below 5.1 then dissable all event menus	
	if(!IsMySQL5010(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql))
		DisableAdvMenu(hmenu);

	// check whether database item is selected or not.
	image = pcquerywnd->m_pcqueryobject->GetSelectionImage();
#ifndef COMMUNITY
	if(pcquerywnd->m_conninfo.m_isreadonly == wyTrue)
	{
		DisableAdvMenu(hmenu);
        EnableMenuItem(hmenu, ID_TABLE_OPENINNEWTAB, MF_ENABLED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_OBJECT_DROPFIELD, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_OBJECT_MAINMANINDEX, MF_GRAYED | MF_BYCOMMAND);
		if(image == NVIEWSITEM)
		{
			EnableMenuItem(hmenu, ID_OBJECT_VIEWDATA, MF_ENABLED | MF_BYCOMMAND);
			EnableMenuItem(hmenu, ID_OBJECT_EXPORTVIEW, MF_ENABLED | MF_BYCOMMAND);
		}
		return wyTrue;
	}
#endif	
	if(image == NCOLUMN || image == NPRIMARYKEY)	
    {
        DisableAdvMenu(hmenu);

		EnableMenuItem(hmenu, ID_OBJECT_DROPFIELD, MF_ENABLED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_COLUMNS_DROPINDEX, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_OBJECT_MAINMANINDEX, MF_GRAYED | MF_BYCOMMAND); 
        EnableMenuItem(hmenu, ID_COLUMNS_MANAGECOLUMNS, MF_ENABLED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_INDEXES_CREATEINDEX , MF_ENABLED | MF_BYCOMMAND);
	} 
    else if(image == NINDEX || image == NPRIMARYINDEX)
    {
        EnableMenuItem(hmenu, ID_OBJECT_DROPFIELD, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_COLUMNS_DROPINDEX, MF_GRAYED | MF_BYCOMMAND);
        EnableMenuItem(hmenu, ID_COLUMNS_MANAGECOLUMNS, MF_GRAYED | MF_BYCOMMAND);
        
        DisableAdvMenu(hmenu);

        EnableMenuItem(hmenu, ID_INDEXES_CREATEINDEX, MF_ENABLED | MF_BYCOMMAND);
        EnableMenuItem(hmenu, ID_INDEXES_EDITINDEX, MF_ENABLED | MF_BYCOMMAND);
        EnableMenuItem(hmenu, ID_INDEXES_DROPINDEX, MF_ENABLED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_OBJECT_MAINMANINDEX, MF_ENABLED | MF_BYCOMMAND); 
	}
    else if(image == NSPITEM)	
    {
		EnableMenuItem(hmenu, ID_OBJECT_DROPFIELD, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_COLUMNS_DROPINDEX, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_OBJECT_MAINMANINDEX, MF_GRAYED | MF_BYCOMMAND); 

		DisableAdvMenu(hmenu);
		EnableMenuItem(hmenu, ID_OBJECT_ALTERSTOREDPROCEDURE, MF_ENABLED | MF_BYCOMMAND); 
		EnableMenuItem(hmenu, ID_OBJECT_DROPSTOREDPROCEDURE, MF_ENABLED | MF_BYCOMMAND); 

	} 
	else if(image == NEVENTITEM)
	{
		EnableMenuItem(hmenu, ID_OBJECT_DROPFIELD, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_COLUMNS_DROPINDEX, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_OBJECT_MAINMANINDEX, MF_GRAYED | MF_BYCOMMAND); 

		DisableAdvMenu(hmenu);
		EnableMenuItem(hmenu, ID_OBJECT_ALTEREVENT, MF_ENABLED | MF_BYCOMMAND); 
		EnableMenuItem(hmenu, ID_OBJECT_DROPEVENT, MF_ENABLED | MF_BYCOMMAND); 
		EnableMenuItem(hmenu, ID_OBJECT_RENAMEEVENT, MF_ENABLED | MF_BYCOMMAND); 

	}
    else if(image == NFUNCITEM)	
    {
		EnableMenuItem(hmenu, ID_OBJECT_DROPFIELD, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_COLUMNS_DROPINDEX, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_OBJECT_MAINMANINDEX, MF_GRAYED | MF_BYCOMMAND); 

		DisableAdvMenu(hmenu);

		EnableMenuItem(hmenu, ID_OBJECT_ALTERFUNCTION, MF_ENABLED | MF_BYCOMMAND); 
		EnableMenuItem(hmenu, ID_OBJECT_DROPFUNCTION, MF_ENABLED | MF_BYCOMMAND); 

	} 
    else if(image == NVIEWSITEM)	
    {
		EnableMenuItem(hmenu, ID_OBJECT_DROPFIELD, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_COLUMNS_DROPINDEX, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_OBJECT_MAINMANINDEX, MF_GRAYED | MF_BYCOMMAND); 
		DisableAdvMenu(hmenu);
		EnableMenuItem(hmenu, ID_OBJECT_ALTERVIEW, MF_ENABLED | MF_BYCOMMAND); 
		EnableMenuItem(hmenu, ID_OBJECT_DROPVIEW, MF_ENABLED | MF_BYCOMMAND); 
		EnableMenuItem(hmenu, ID_OBJECTS_RENAMEVIEW, MF_ENABLED | MF_BYCOMMAND); 
        EnableMenuItem(hmenu, ID_OBJECT_EXPORTVIEW, MF_ENABLED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_OBJECT_VIEWDATA, MF_ENABLED | MF_BYCOMMAND);
        EnableMenuItem(hmenu, ID_TABLE_OPENINNEWTAB, MF_ENABLED | MF_BYCOMMAND);
	} 
    else if(image == NTRIGGERITEM)	
    {
		EnableMenuItem(hmenu, ID_OBJECT_DROPFIELD, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_COLUMNS_DROPINDEX, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_OBJECT_MAINMANINDEX, MF_GRAYED | MF_BYCOMMAND); 
		DisableAdvMenu(hmenu);
		EnableMenuItem(hmenu, ID_OBJECT_ALTERTRIGGER, MF_ENABLED | MF_BYCOMMAND); 
		EnableMenuItem(hmenu, ID_OBJECT_DROPTRIGGER, MF_ENABLED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_OBJECTS_RENAMETRIGGER, MF_ENABLED | MF_BYCOMMAND); 
	} 
    else 
    {
        EnableMenuItem(hmenu, ID_INDEXES_EDITINDEX, MF_GRAYED | MF_BYCOMMAND);
        EnableMenuItem(hmenu, ID_INDEXES_DROPINDEX, MF_GRAYED | MF_BYCOMMAND);
        EnableMenuItem(hmenu, ID_OBJECT_DROPFIELD, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_COLUMNS_DROPINDEX, MF_GRAYED | MF_BYCOMMAND);
        DisableAdvMenu(hmenu);
        EnableMenuItem(hmenu, ID_COLUMNS_MANAGECOLUMNS, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_OBJECT_MAINMANINDEX, MF_GRAYED | MF_BYCOMMAND);
        EnableMenuItem(hmenu, ID_INDEXES_CREATEINDEX, MF_GRAYED | MF_BYCOMMAND);
        
	}
	
	if(image != NSERVER && IsMySQL5010(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql))
	{
		EnableMenuItem(hmenu, ID_OBJECT_CREATEFUNCTION, MF_ENABLED | MF_BYCOMMAND); 
		EnableMenuItem(hmenu, ID_OBJECT_CREATESTOREDPROCEDURE, MF_ENABLED | MF_BYCOMMAND); 
		EnableMenuItem(hmenu, ID_OBJECT_CREATEVIEW, MF_ENABLED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_OBJECT_CREATETRIGGER, MF_ENABLED | MF_BYCOMMAND); 
		if(IsMySQL516(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql))
			EnableMenuItem(hmenu, ID_OBJECT_CREATEEVENT, MF_ENABLED | MF_BYCOMMAND);
	}
		
		if(image == NTABLE || image == NFOLDER || image == NINDEX || image == NPRIMARYINDEX)
		{
			EnableMenuItem(hmenu, ID_OBJECT_MAINMANINDEX, MF_ENABLED | MF_BYCOMMAND);
			EnableMenuItem(hmenu, ID_INDEXES_CREATEINDEX, MF_ENABLED | MF_BYCOMMAND);
		}

		if(image == NTABLE || image == NFOLDER || image == NCOLUMN || image == NPRIMARYKEY)
			EnableMenuItem(hmenu, ID_COLUMNS_MANAGECOLUMNS, MF_ENABLED | MF_BYCOMMAND);

	return wyTrue;
}

wyBool
EnableFavoriteMenu(HMENU  hmenu)
{	
	wyInt32     count, itemcount, tabicon = 0;
	MDIWindow   *wnd ;
	
	wnd   = GetActiveWin();
	count = GetMenuItemCount(hmenu);

	if(wnd)
	{
		tabicon = wnd->m_pctabmodule->GetActiveTabImage();
	}

    //The Favourite menu should disable either the tab is QB or no MDI window
	if(!wnd || tabicon == IDI_DATASEARCH || !wnd->GetActiveTabEditor())	
	{
		EnableMenuItem(hmenu,0, MF_GRAYED | MF_BYPOSITION);
	
		for(itemcount = FAV_ITEM_START_INDEX;  itemcount < count ; itemcount++)
			EnableMenuItem(hmenu, itemcount, MF_GRAYED | MF_BYPOSITION);
	}
	else
	{
		for(itemcount = 0;  itemcount < count; itemcount++)
			EnableMenuItem(hmenu, itemcount, MF_ENABLED | MF_BYPOSITION);
	}

	/*  always disable the <empty> menu */
	EnableMenuItem(hmenu, IDM_EMPTY, MF_GRAYED  | MF_BYCOMMAND);
		
	return wyFalse;
}

void 
DisableAdvMenu(HMENU hmenu)
{
    wyInt32 nid[] = { ID_OBJECT_CREATEVIEW, ID_OBJECT_ALTERVIEW, ID_OBJECT_DROPVIEW, ID_OBJECTS_RENAMEVIEW, ID_OBJECT_EXPORTVIEW,ID_OBJECT_VIEWDATA,
                      ID_OBJECT_CREATESTOREDPROCEDURE, ID_OBJECT_ALTERSTOREDPROCEDURE, 
                      ID_OBJECT_DROPSTOREDPROCEDURE, ID_OBJECT_CREATEFUNCTION, ID_OBJECT_ALTERFUNCTION, 
                      ID_OBJECT_DROPFUNCTION, ID_OBJECT_CREATETRIGGER, ID_OBJECT_ALTERTRIGGER, 
                      ID_OBJECT_DROPTRIGGER, ID_OBJECTS_RENAMETRIGGER, ID_INDEXES_CREATEINDEX, ID_INDEXES_EDITINDEX,
					  ID_INDEXES_DROPINDEX, ID_COLUMNS_MANAGECOLUMNS, 
  ID_OBJECT_CREATEEVENT, ID_OBJECT_ALTEREVENT, ID_OBJECT_DROPEVENT, ID_OBJECT_RENAMEEVENT};

    wyInt32 size = sizeof(nid)/ sizeof(nid[0]), count;

	for(count = 0; count < size; count++)
	    EnableMenuItem(hmenu, nid[count], MF_GRAYED | MF_BYCOMMAND);
}

void 
AddToolBarButtons(HINSTANCE hinst, HWND htoolbar, HIMAGELIST himglist, wyInt32 *command, 
                            wyInt32 size, wyUInt32 (*states)[2], wyInt32 *imgres)
{
    wyInt32     iconid, count;
	HICON		hicon;
    TBBUTTON	tbb[MAX_TAB_BUTTON_COUNT]; 
    
    for(count = 0; count < size; count++)
	{
		//hicon = (HICON)LoadImage(hinst, MAKEINTRESOURCE(*(imgres+count)), 
          //  IMAGE_ICON, ICON_SIZE, ICON_SIZE, LR_DEFAULTCOLOR);
		hicon = LoadIcon(hinst, MAKEINTRESOURCE(*(imgres+count)));
		VERIFY((iconid= ImageList_AddIcon(himglist, hicon))!= -1);
		VERIFY(DestroyIcon(hicon));
		
		memset(&tbb[count], 0, sizeof(TBBUTTON));

		tbb[count].iBitmap = MAKELONG(iconid,0);
		tbb[count].idCommand = *(command + count);
		tbb[count].fsState = (wyChar)(*(*(states + count) + 0));
		tbb[count].fsStyle = (wyChar)(*(*(states + count) + 1));
	}

	VERIFY(SendMessage(htoolbar, TB_ADDBUTTONS,(WPARAM)size,(LPARAM)&tbb)!= FALSE);
    return;
}

void  
InitConInfo(ConnectionInfo &consrc, ConnectionInfo &contgt)
{
    contgt.m_title.SetAs(consrc.m_title.GetLength()?consrc.m_title.GetString():"");
    
    contgt.m_tunnel = consrc.m_tunnel;
    contgt.m_mysql = consrc.m_mysql;
    contgt.m_hprocess = consrc.m_hprocess;
    
    contgt.m_host.SetAs(consrc.m_host);
    contgt.m_user.SetAs(consrc.m_user);
	contgt.m_pwd.SetAs(consrc.m_pwd);
	contgt.m_isstorepwd = consrc.m_isstorepwd;
	contgt.m_port = consrc.m_port;

	contgt.m_iscompress = consrc.m_iscompress;
#ifndef COMMUNITY
//	if(pGlobals->m_entlicense.CompareI("Professional") != 0)
	contgt.m_isreadonly = consrc.m_isreadonly;
#endif	
	contgt.m_isdeftimeout = consrc.m_isdeftimeout;
	contgt.m_strwaittimeout.SetAs(consrc.m_strwaittimeout);
	//contgt.m_ispwdcleartext = consrc.m_ispwdcleartext;
    contgt.m_keepaliveinterval = consrc.m_keepaliveinterval;

    contgt.m_codepage.m_cpname.SetAs(consrc.m_codepage.m_cpname.GetString());
    contgt.m_codepage.m_cpdesc.SetAs(consrc.m_codepage.m_cpdesc.GetLength()?consrc.m_codepage.m_cpdesc.GetString():"");
    contgt.m_codepage.m_encodingname.SetAs(consrc.m_codepage.m_encodingname.GetLength()?consrc.m_codepage.m_encodingname.GetString():"");
	contgt.m_codepage.m_ncharset = consrc.m_codepage.m_ncharset;
    contgt.m_db.SetAs(consrc.m_db.GetLength()?consrc.m_db.GetString():"");
	    
    contgt.m_isssh = consrc.m_isssh;
    contgt.m_localhost.SetAs(consrc.m_localhost.GetLength()?consrc.m_localhost.GetString():"");
    contgt.m_localport = consrc.m_localport;
    contgt.m_origmysqlport = consrc.m_origmysqlport;
    contgt.m_sshhost.SetAs(consrc.m_sshhost.GetLength()?consrc.m_sshhost.GetString():"");
    contgt.m_sshport = consrc.m_sshport;
    contgt.m_sshpwd.SetAs(consrc.m_sshpwd.GetLength()?consrc.m_sshpwd.GetString():"");
    contgt.m_sshuser.SetAs(consrc.m_sshuser.GetLength()?consrc.m_sshuser.GetString():"");
    contgt.m_privatekeypath.SetAs(consrc.m_privatekeypath);
    contgt.m_ispassword = consrc.m_ispassword;
	
	contgt.m_ishttp=consrc.m_ishttp ;
	contgt.m_url.SetAs(consrc.m_url.GetString());
	contgt.m_timeout = consrc.m_timeout ;
	contgt.m_ishttpuds=consrc.m_ishttpuds ;
	contgt.m_httpudspath.SetAs(consrc.m_httpudspath.GetString());

	contgt.m_issslchecked = consrc.m_issslchecked;
    contgt.m_issslauthchecked = consrc.m_issslauthchecked;
    contgt.m_clicert.SetAs(consrc.m_clicert);
    contgt.m_clikey.SetAs(consrc.m_clikey);
    contgt.m_cacert.SetAs(consrc.m_cacert);
    contgt.m_cipher.SetAs(consrc.m_cipher);

	contgt.m_sshpipeends.m_hreadpipe = consrc.m_sshpipeends.m_hreadpipe;
	contgt.m_sshpipeends.m_hreadpipe2 = consrc.m_sshpipeends.m_hreadpipe2;
	contgt.m_sshpipeends.m_hwritepipe = consrc.m_sshpipeends.m_hwritepipe;
	contgt.m_sshpipeends.m_hwritepipe2 = consrc.m_sshpipeends.m_hwritepipe2;


	contgt.m_rgbconn   =  consrc.m_rgbconn;
	contgt.m_rgbfgconn =  consrc.m_rgbfgconn;

	contgt.m_isglobalsqlmode = consrc.m_isglobalsqlmode;
	contgt.m_sqlmode.SetAs(consrc.m_sqlmode);
    contgt.m_initcommand.SetAs(consrc.m_initcommand);
}


void  
InitializeConnectionInfo(ConnectionInfo &con)
{
    con.m_codepage.m_cpname.Clear();
    con.m_codepage.m_cpdesc.Clear();
    con.m_codepage.m_encodingname.Clear();
	con.m_codepage.m_ncharset = 0;
    con.m_db.Clear();
    con.m_hprocess = NULL;
    con.m_isssh = wyFalse;
    con.m_localhost.Clear();
    con.m_localport = 0;
    con.m_mysql = NULL;
    con.m_origmysqlport = 0;
    con.m_sshhost.Clear();
    con.m_sshport = 0;
    con.m_sshpwd.Clear();
    con.m_sshuser.Clear();
    con.m_title.Clear();
    con.m_tunnel = NULL;
	con.m_sshpipeends.m_hreadpipe = INVALID_HANDLE_VALUE;
	con.m_sshpipeends.m_hwritepipe = INVALID_HANDLE_VALUE;
	con.m_sshpipeends.m_hreadpipe2 = INVALID_HANDLE_VALUE;
	con.m_sshpipeends.m_hwritepipe2 = INVALID_HANDLE_VALUE;
	con.m_sshsocket = NULL;
	con.m_rgbconn   = COLOR_WHITE;
	con.m_rgbfgconn   = RGB(0, 0, 0);
//	con.m_ispwdcleartext = wyFalse;
	con.m_isstorepwd = wyFalse;
	con.m_ishttp = wyFalse;
}

void 
SetCharacterSet(MDIWindow *wnd, Tunnel *tunnel, MYSQL *mysql, wyString &cpname, wyBool reconnect, wyBool profile)
{
	wyString		query;
	wyBool			ismysql41 = IsMySQL41(tunnel, &mysql);
    
	if(tunnel->IsTunnel())
	{
		if(ismysql41 == wyTrue)
			cpname.SetAs("utf8");
		tunnel->SetCharset(cpname.GetString());
		return;
	}

	//if(ismysql41 == wyTrue)
	//{
	//	//query.Sprintf("set names '%s'", "utf8");	
 //       if(reconnect == wyTrue)
	//	res = ExecuteAndGetResult(wnd, tunnel, &mysql, query, profile);
 //       else
 //           res = SjaExecuteAndGetResult(tunnel, &mysql, query);
	//					
	//	tunnel->mysql_free_result(res);
	//}
	return;
}

wyBool
yog_enddialog(HWND hdlg, INT_PTR nresult)
{
	MDIWindow   *pcquerywnd	= GetActiveWin();
	wyInt32     ret = ::EndDialog(hdlg, nresult);

	if(pcquerywnd)
		UpdateWindow(pcquerywnd->m_hwnd);

    if(ret)
        return wyTrue;

    return wyFalse;
}


wyBool
IsQueryEmpty(const wyWChar *query)
{
	wyInt32 count = 0;

	while(query[count]!= 0)
	{
		if(!(iswspace(query[count])))
			return wyFalse;
		count++;
	}

	return wyTrue;
}

// Function to format and show standard MYSQL error given by mySQL C API
void
ShowMySQLError(HWND hdlg, Tunnel * tunnel, PMYSQL mysql, const wyChar * query, wyBool val)
{
	MySQLError errorobj;
	if(pGlobals->m_conrestore)
		return;
	errorobj.Show(hdlg, tunnel, mysql, query, val);

}

//Show waaarnings dialog
void
ShowMySQLWarnings(HWND hdlg, Tunnel * tunnel, PMYSQL mysql, const wyChar * query, MYSQL_RES *reswarning)
{
	MySQLError errorobj;
	errorobj.Show(hdlg, tunnel, mysql, query, wyFalse, reswarning);
}


// Function to format and show standard MYSQL error given by mySQL C API
void
ShowErrorMessage(HWND hdlg, const wyChar * query, const wyChar *errmsg)
{
	MySQLError errorobj;
	errorobj.ShowDirectErrMsg(hdlg, query, errmsg);
}


// If the mysql connection is successful then write the details in the ini file so
// that when the user connects again we can give him the same details.
void
WriteConnDetails(HWND hdlg)
{
	wyInt32		writeret;
	wyWChar     directory[MAX_PATH] = {0}, *lpfileport = 0;
	//wyWChar     name[SIZE_512] = {0};
	wyWChar		*conname = NULL;
	wyInt32     ret, index, txtlen = 0;
	wyString	namestr, dirstr;
    HWND        hcb = GetDlgItem(hdlg,IDC_DESC);
    HWND        hwndCombo = (HWND)SendMessage(hcb, CCBM_GETCOMBOHWND, NULL, NULL);
	
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH - 1, directory, &lpfileport);

	if(ret == 0)
		return;
	
	dirstr.SetAs(directory);

	VERIFY((index = SendMessage(hcb, CB_GETCURSEL, 0, 0))!= CB_ERR);

	txtlen = (SendMessage(hcb, CB_GETLBTEXTLEN, index, 0));

	conname = AllocateBuffWChar(txtlen + 1);

	VERIFY((SendMessage(hwndCombo, CB_GETLBTEXT, index,(LPARAM)conname)));

	namestr.SetAs(conname);
	free(conname);

	// now write it in the profilestring file.
	VERIFY(writeret = wyIni::IniWriteString(SECTION_NAME, "Host", namestr.GetString(), dirstr.GetString()));

	//VERIFY((index = SendMessage(GetDlgItem(hdlg, IDC_DEFSRV), CB_GETCURSEL, 0, 0))!= CB_ERR);
	//VERIFY((SendMessage(GetDlgItem(hdlg, IDC_DEFSRV), CB_GETLBTEXT, index, (LPARAM)name)));

	// now write it in the profilestring file.
	//namestr.SetAs(name);
	//VERIFY(writeret = WritePrivateProfileStringA(SECTION_NAME, "DefSrv", namestr.GetString(), dirstr.GetString()));

	return;
}

wyBool
ShowHelp(wyChar *helpid)
{
	wyString helplink;
	//removing offline helpfile from version 12.05
	/*helplink.Sprintf("%s::/%s", pGlobals->m_helpfile, helpid);
	HtmlHelp(::GetDesktopWindow(), helplink.GetAsWideChar(), HH_DISPLAY_TOPIC, NULL);*/
	helplink.Sprintf("%s", helpid);
	ShellExecute(NULL, L"open", helplink.GetAsWideChar(), NULL, NULL, SW_SHOWNORMAL);
	return wyTrue;
}

wyInt32 
compareid(const void *arg1, const void *arg2)
{
	yogIcons	*icons1 =(yogIcons*)arg1;
	yogIcons	*icons2 =(yogIcons*)arg2;

	if(icons1->m_menuid < icons2->m_menuid)
		return -1;
	else if(icons1->m_menuid > icons2->m_menuid)
		return 1;
	else
		return 0;
}

 // if connection exists the return value is true as well as connspe will be valid
wyInt32 
IsConnectionExists(HWND hwndcombo, wyString &path, wyWChar *connname, wyChar *connspe)
{
    wyInt32		cursel;
	wyString	connspestr, tmpconnname, allsecnames, conn;
    wyChar      *tempconsecname, *allsectionnames;
    wyChar      seps[] = ";";
    HWND        hwndc = (HWND) SendMessage(hwndcombo, CCBM_GETCOMBOHWND, NULL, NULL); 

	if(hwndcombo)
	{
		VERIFY((cursel = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0))!= CB_ERR);
		VERIFY(SendMessage(hwndc, CB_GETLBTEXT, (WPARAM)cursel, (LPARAM)connname));
	}

    wyIni::IniGetSection(&allsecnames, &path);

    allsectionnames = (wyChar *)allsecnames.GetString();

    tempconsecname = strtok(allsectionnames, seps);
	while(tempconsecname)
	{
        conn.SetAs(tempconsecname);
		
       wyIni::IniGetString(conn.GetString(), "Name", "", &tmpconnname, path.GetString());

        if(tmpconnname.GetLength() == 0)
        {
            tempconsecname = strtok(NULL, seps);
			continue;
        }
				
		if(0 == wcsicmp(connname, tmpconnname.GetAsWideChar()))
        {
            sprintf(connspe, "%s", conn.GetString());
		    return wyTrue;
        }

        tempconsecname = strtok(NULL, seps);
    }

    return wyFalse;
}

//post 8.01
//This function is not using for Post 8.01 (flicker solve)
void 
RepaintTabModule()
{
	//EditorBase *peditorbase;
	//MDIWindow	*pquerywindow = GetActiveWin();

   // VERIFY(pquerywindow);

   // if(!pquerywindow->GetActiveTabEditor())
     //   return;

	
    //peditorbase = pquerywindow->GetActiveTabEditor()->m_peditorbase;
	//InvalidateRect(peditorbase->m_hwnd , NULL, FALSE);
	//UpdateWindow(peditorbase->m_hwnd);

    //InvalidateRect(pquerywindow->GetActiveTabEditor()->m_pctabmgmt->m_hwnd, NULL, FALSE);
	//UpdateWindow(pquerywindow->GetActiveTabEditor()->m_pctabmgmt->m_hwnd);

    return;
}

/* simple function that returns the correct index by taking the resultset and the column names */
/* returns the index value or -1 on error */
wyInt32 
GetFieldIndex(MYSQL_RES * result, wyChar *colname, Tunnel *tunnel, PMYSQL mysql)
{
	wyUInt32        num_fields, count;
	MYSQL_FIELD		*fields;
    wyString		fieldnamestr;

	num_fields	= tunnel->mysql_num_fields(result);
	fields		= tunnel->mysql_fetch_fields(result);

	for(count = 0; count < num_fields; count++)
	{
		fieldnamestr.SetAs(fields[count].name);

		if(0 == stricmp(fieldnamestr.GetString(), colname))
			return count;
	}

	_ASSERT(0);
	return -1;
}


// Function to display formatted error message in a yog_message.
// It will show the szExtraText first with the standard system error at end.
void
DisplayErrorText(wyUInt32 dwlasterror, const wyChar *extratext, HWND hwnd)
{
 	wyString    comperr;
	wyWChar      *mess;
        wyString	message;
    wyUInt32    len, lasterror = 0;
    wyUInt32    flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS |
									FORMAT_MESSAGE_FROM_SYSTEM; 
	if(pGlobals->m_conrestore)
		return;
    VERIFY(len = FormatMessage(flags, NULL, dwlasterror, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPWSTR)&mess, 0, NULL));

	// null terminate the buffer
	mess[len] = 0;

	// if error length is zero set message to null
	if(!len)
		return;

	message.SetAs(mess);

	lasterror = GetLastError();
	comperr.Sprintf("%s\n%d:%s", extratext, lasterror, message.GetString());
	yog_message(hwnd, comperr.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK | MB_TASKMODAL);

	// free the memory allocated by system
	VERIFY(LocalFree(mess) == NULL);

	return;
}


void
CombineErrorText(wyUInt32 dwlasterror, const wyChar *extratext, HWND hwnd, wyString *finalmessage,wyString filename)
{
 	wyString    comperr;
	wyWChar      *mess;
    wyString	message;
	static int i=0;
    wyUInt32    len, lasterror = 0;
    wyUInt32    flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS |
									FORMAT_MESSAGE_FROM_SYSTEM; 
	if(pGlobals->m_conrestore)
		return;
    VERIFY(len = FormatMessage(flags, NULL, dwlasterror, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPWSTR)&mess, 0, NULL));

	// null terminate the buffer
	mess[len] = 0;

	// if error length is zero set message to null
	if(!len)
		return;

	message.SetAs(mess);
	lasterror = GetLastError();
	comperr.Sprintf("%s\n%d:%s", extratext, lasterror, message.GetString());
	finalmessage->AddSprintf("%s : %s\n",filename.GetString(),comperr.GetString());
	/*
*/
	// free the memory allocated by system
	i++;
	VERIFY(LocalFree(mess) == NULL);

	return;
}

wyInt32 
GetMySQLError(HWND hdlg, Tunnel * tunnel, PMYSQL mysql, wyString &err)
{
	wyUInt32  mysqlerrno;
    wyInt32    ret = wyTrue;
    MDIWindow *wnd;
	wyString	cpname;

    err.Clear();
	mysqlerrno = (*mysql)->net.last_errno;

	/* it may happen that due to http error the errornumber is 0 so we return */
	if(0 == mysqlerrno)
		return 0;

	if(tunnel->IsTunnel()&& mysqlerrno > 12000)
		err.Sprintf(_("HTTP Error No. %d\n%s"), mysqlerrno, tunnel->mysql_error(*mysql));
	else
	{
		err.Sprintf(_("Error No. %d\n%s"), mysqlerrno, (*mysql)->net.last_error);

		if(mysqlerrno == MYSQL_SERVER_GONE)
		{
			wnd = GetActiveWin();
            if(wnd)
            {
				if(!wnd->m_conninfo.m_codepage.m_cpname.GetLength())
					cpname.SetAs("utf8");

				else
					cpname.SetAs(wnd->m_conninfo.m_codepage.m_cpname);

			    /* starting from 4.02 we issue query like show variables like and set the client value to the database */
			    /*only required in direct connection as in HTTP we execute it always in the server side */
			    SetCharacterSet(tunnel, *mysql, (wyChar*)cpname.GetString());

			    /* if its tunnel then we need the server version */
			    if(tunnel->IsTunnel())
				    ret = tunnel->GetMySQLVersion(*mysql);

			    if(ret && IsMySQL5010(tunnel, mysql))
					wnd->SetDefaultSqlMode(tunnel, mysql, wyTrue);
            }
		}
	}
	UpdateWindow(hdlg);
	return err.GetLength();
}

// tells whether the OS is Win95/98/Me or not.
wyBool
IsWin9x()
{
	// check if its supported.
	DWORD dwversion = GetVersion();

	// Get the Windows version.
	DWORD dwwindowsmajorversion = (DWORD)(LOBYTE(LOWORD(dwversion)));

	// Get the build number.
	if(!(dwwindowsmajorversion > 4))
		return wyTrue;
	else
		return wyFalse;
}

// tells whether the OS is Win95/98/Me or not.
wyBool
IsCrashDumpSupported()
{
	// check if its supported.
	DWORD dwversion = GetVersion();

	// Get the Windows version.
	DWORD dwwindowsmajorversion = (DWORD)(LOBYTE(LOWORD(dwversion)));

	// Get the build number. only on XP as well as win2000 or above
	if(dwwindowsmajorversion > 4)
		return wyTrue;
	else
		return wyFalse;
}

wyBool
OverWriteFile(HWND hwnd, const wyWChar *filename)
{
	HANDLE		hfile;
    wyInt32     create;
	wyString    errmsg;
	wyString	filenamestr(filename);

	hfile =  CreateFile((LPCWSTR)filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if(hfile != INVALID_HANDLE_VALUE)
	{
		errmsg.Sprintf(_("%s already exists.\nDo you want to replace it?"), filenamestr.GetString());
		create = yog_message(hwnd, errmsg.GetAsWideChar(), _(L"Save As..."), MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON2);

		VERIFY(CloseHandle(hfile));

		if(create != IDYES)
			return wyFalse;
	}

	return wyTrue;
}
  
void HandlerToSetWindowPos(HWND hwnd)
{
    RECT    rc;
    wyInt32 xx = GetSystemMetrics(SM_CXSCREEN);
    wyInt32 yy = GetSystemMetrics(SM_CYSCREEN);

	GetWindowRect(GetParent(hwnd), &rc);
        
    SetWindowPos(GetParent(hwnd), NULL, ((rc.left + xx/2 - (rc.right - rc.left)/ 2)),
                ((rc.top + yy/2 - (rc.bottom - rc.top)/ 2)), 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

    return;
}

//setting Execute query submenus and object browser refresh when switching F5 and F9 functionalities
void
ChangeMenuItemOnPref(HMENU hmenu,  wyInt32 menuindex)
{
	//wyWChar		    *menuname = NULL;
	HMENU				submenu;
	//wyWChar				menustring [SIZE_128] = {0};
	//MENUITEMINFO		lpmii = {0};
	wyInt32				position = 2;	//change in position of submenu item execute query

	// if  switching refreshkey from F9 to F5 and Execute Query short cut keys
	if(pGlobals->m_isrefreshkeychange == wyFalse)
	{	
		//wcscpy(menustring, _(L"Refresh &Object Browser\tF5"));
		if(!pGlobals->m_menurefreshobj)
		{
			pGlobals->m_menurefreshobj = wcsdup(_(L"Refresh &Object Browser\tF5"));
		}
		if(menuindex == MNUDB_INDEX)// if it is DB menu 
		{
			SetMenuOnPreferenceChange(hmenu,  pGlobals->m_menurefreshobj, IDM_DB_REFRESHOBJECT);
			return;		
		}
		SetMenuOnPreferenceChange(hmenu,  pGlobals->m_menurefreshobj, IDM_REFRESHOBJECT);

		if(menuindex == MNU_OTHEROPT_INDEX) 
			return;

		submenu = GetSubMenu(hmenu, position);// handle to excute query submenus
		if(!submenu)						  
			return;
		//setting excute query submenus

		if(!pGlobals->m_menucurrentquery)
		{
			pGlobals->m_menucurrentquery = wcsdup(_(L"Exe&cute Query\tF9"));
		}
		//wcscpy(menustring, _(L"Execute &Current Query\tF9"));
		SetMenuOnPreferenceChange(hmenu, pGlobals->m_menucurrentquery, ACCEL_EXECUTE_MENU); 

		if(!pGlobals->m_menuselectedquery)
		{
			pGlobals->m_menuselectedquery = wcsdup(_(L"Execute &Selected Query\tCtrl+F9"));
		}
		//wcscpy(menustring, _(L"Execute &Selected Query\tCtrl+F9"));
		SetMenuOnPreferenceChange(hmenu, pGlobals->m_menuselectedquery, ACCEL_EXECUTESEL);

		if(!pGlobals->m_menuallquery)
		{
			pGlobals->m_menuallquery = wcsdup(_(L"Execute &All Queries\tCtrl+F9"));
		}
	
		//wcscpy(menustring, _(L"Execute &All Queries\tShift+F9"));
		SetMenuOnPreferenceChange(hmenu, pGlobals->m_menuallquery, ACCEL_EXECUTEALL);		
		
	}
	else
	{
		if(!pGlobals->m_menurefreshobj)
		{
			pGlobals->m_menurefreshobj = wcsdup(_(L"Refresh &Object Browser\tF9"));
		}

		//wcscpy(menustring, _(L"Refresh &Object Browser\tF9"));
		if(menuindex == MNUDB_INDEX)// if it is DB menu 
		{
			SetMenuOnPreferenceChange(hmenu,  pGlobals->m_menurefreshobj, IDM_DB_REFRESHOBJECT);
			return;		
		}
		SetMenuOnPreferenceChange(hmenu,  pGlobals->m_menurefreshobj, IDM_REFRESHOBJECT);

		if(menuindex == MNU_OTHEROPT_INDEX) // if it is objectbrowser menu (popup window containing three menus)
			return;
				
		submenu = GetSubMenu(hmenu, position);   // handle to excute query submenus
		if(!submenu)							// if it is DB menu return 
			return;	
		//setting excute query submenus
		//wcscpy(menustring, _(L"Execute &Current Query\tF5"));
		if(!pGlobals->m_menucurrentquery)
		{
			pGlobals->m_menucurrentquery = wcsdup(_(L"Exe&cute Query\tF5"));
		}
		SetMenuOnPreferenceChange(hmenu, pGlobals->m_menucurrentquery, ACCEL_EXECUTE_MENU);
		
		//wcscpy(menustring, _(L"Execute &Selected Query\tCtrl+F5"));
		if(!pGlobals->m_menuselectedquery)
		{
			pGlobals->m_menuselectedquery = wcsdup( _(L"Execute &Selected Query\tCtrl+F5"));
		}
		SetMenuOnPreferenceChange(hmenu, pGlobals->m_menuselectedquery, ACCEL_EXECUTESEL);
		
		//wcscpy(menustring, _(L"Execute &All Queries\tShift+F5"));
		if(!pGlobals->m_menuallquery)
		{
			pGlobals->m_menuallquery = wcsdup(_(L"Execute &All Queries\tCtrl+F5"));
		}
		SetMenuOnPreferenceChange(hmenu, pGlobals->m_menuallquery, ACCEL_EXECUTEALL);
	}

	//if(pGlobals->m_isautocomplete == wyFalse)
	//{
	//VERIFY(EnableMenuItem(hmenu, ID_DATABASE_REBUILDTAGS, MF_GRAYED | MF_BYCOMMAND) != -1);
	//}
	//else
	//{
	//
	//}
    		
}
//setting menu item information
void
SetMenuOnPreferenceChange(HMENU hmenu, wyWChar	*menustring, wyInt32 menuid)
{
//	wyWChar		    *menuname = NULL;
	//wyString		strng;
	MENUITEMINFO    lpmii = {0};

	//menuname =  _wcsdup(menustring);
	
	lpmii.cbSize		= sizeof(MENUITEMINFO);
	lpmii.fMask			= MIIM_STRING | MIIM_ID | MIIM_DATA;
	lpmii.wID			= menuid;
	lpmii.dwItemData	= (ULONG_PTR)menustring;
	lpmii.cch			= wcslen(menustring) + 1;
	lpmii.dwTypeData	= menustring;

	VERIFY(::SetMenuItemInfo(hmenu, menuid, FALSE, &lpmii));
	
}

//Handle the ListView ListBox Selection
void HandleListViewCheckboxItems(HWND hListView, LPARAM lParam, wyBool isautomated)
{
	wyInt32				start = 0 ,end = 0;
	static HWND			hwnd = NULL;
	LPNMLISTVIEW lpnm = (LPNMLISTVIEW)lParam;;
	static wyInt32		lastRow			= -1 ;
	static wyBool		processing		= wyFalse ;

	if(hListView == NULL || hwnd != hListView)
	{
		lastRow			= -1 ;
		processing		= wyFalse ;
	}

	if(hListView == NULL || isautomated || (lpnm->uNewState & LVIS_STATEIMAGEMASK) == 0)
		return;
	
	hwnd = hListView;

	if(lastRow == -1)
	{
		lastRow = lpnm->iItem;
		return;
	}

	if(processing == wyTrue)
		return;

	if(!(GetKeyState(VK_SHIFT) & SHIFTED))
	{
		lastRow = lpnm->iItem;
		return;
	}
	
	processing = wyTrue;

	start = lpnm->iItem;
	end = lastRow;
	lastRow = lpnm->iItem;

	wyBool checkState;

	checkState = wyFalse;

	if(ListView_GetCheckState(hwnd, start))
		checkState = wyTrue;
	
	if(start > end)
	{
		int tmp = start;
		start = end;
		end = tmp;
	}

	for(int i=start; i<=end; i++)
		ListView_SetCheckState(hwnd, i, checkState);

	processing = wyFalse;
	return;
}


//Handle the tree view item
HTREEITEM
HandleTreeViewItem(LPARAM lParam, WPARAM  wParam, wyBool spacekeydown)
{
	DWORD               dwpos;
	wyWChar             temptext[SIZE_1024] = {0};
    static HWND         hwndfrom = NULL;
    static HTREEITEM    hlastclickitem = NULL;
    HTREEITEM           htemp, hactual = NULL, hlastitem = NULL;
    wyUInt32            direction = TVGN_NEXT;
    wyInt32             checkstate = 0;
    wyBool              isshiftkey = wyFalse;
    TVITEM		        tvi;
    TVHITTESTINFO       ht = {0};
	LPNMHDR	            lpnmh = (LPNMHDR)lParam;
	LPNMTVKEYDOWN       ptvkd = (LPNMTVKEYDOWN) lParam ;
    
	dwpos = GetMessagePos();
	ht.pt.x = GET_X_LPARAM(dwpos);
 	ht.pt.y = GET_Y_LPARAM(dwpos);
	

	VERIFY(MapWindowPoints(HWND_DESKTOP, lpnmh->hwndFrom, &ht.pt, 1));
	TreeView_HitTest(lpnmh->hwndFrom, &ht);	

	// checking mouse click (space key down) on Tree View Item  
	if(TVHT_ONITEMSTATEICON & ht.flags || spacekeydown == wyTrue)
	{
		if(ptvkd->wVKey == VK_SPACE)  // if we press space key we need to get selected item htreeitem HTREEITEM 
			ht.hItem = TreeView_GetSelection(lpnmh->hwndFrom);
        
        //check whether the shift key is pressed and set the flag
        if(GetKeyState(VK_SHIFT) & SHIFTED)
            isshiftkey = wyTrue;

        //select the item
		TreeView_SelectItem(lpnmh->hwndFrom, ht.hItem);

        //check whether the window handle are same, and the parent of last clicked item and selected item are same
        if(hwndfrom != lpnmh->hwndFrom || 
           TreeView_GetParent(lpnmh->hwndFrom, hlastclickitem) != TreeView_GetParent(lpnmh->hwndFrom, ht.hItem))
        {
            hwndfrom = lpnmh->hwndFrom;
            hlastclickitem = ht.hItem;
        }

        if(isshiftkey == wyTrue && ht.hItem != hlastclickitem)
        {
            //get the parent of the item
            htemp = TreeView_GetParent(lpnmh->hwndFrom, ht.hItem);

            //loop through the item to determine the seek direction
            for(htemp = TreeView_GetChild(lpnmh->hwndFrom, htemp); htemp; 
                htemp = TreeView_GetNextSibling(lpnmh->hwndFrom, htemp))
            {
                if(htemp == hlastclickitem)
                {
                    //if the firs encounter is the last clicked item, then we need to seek in forward dir
                    direction = TVGN_NEXT;

                    //find the item after the current item, this mark the end of items to be checked/unchecked
                    hlastitem = TreeView_GetNextSibling(lpnmh->hwndFrom, ht.hItem);

                    break;
                }

                if(htemp == ht.hItem)
                {
                    //if the firs encounter is the current item, then we need to seek in backward dir
                    direction = TVGN_PREVIOUS;

                    //find the item before the current item, this mark the end of items to be checked/unchecked 
                    hlastitem = TreeView_GetPrevSibling(lpnmh->hwndFrom, ht.hItem);

                    break;
                }
            }

            //find the check state to be used
            if(TreeView_GetCheckState(lpnmh->hwndFrom, hlastclickitem) == 
               TreeView_GetCheckState(lpnmh->hwndFrom, ht.hItem))
            {
                checkstate = TreeView_GetCheckState(lpnmh->hwndFrom, hlastclickitem);
            }
            else
            {
                checkstate = !TreeView_GetCheckState(lpnmh->hwndFrom, hlastclickitem);
            }
        }
        //no shift key pressed
        else
        {
            hlastitem = TreeView_GetNextSibling(lpnmh->hwndFrom, ht.hItem);
            hlastclickitem = ht.hItem;
        }

        hactual = ht.hItem;

        //traverse through the items between last click item and last item calculated
        for(ht.hItem = hlastclickitem; ht.hItem != hlastitem; 
            ht.hItem = (HTREEITEM)SendMessage(lpnmh->hwndFrom, TVM_GETNEXTITEM, (WPARAM)direction, (LPARAM)ht.hItem))
        {
            //for items other than selected item
            if(ht.hItem != hactual)
            {
                TreeView_SetCheckState(lpnmh->hwndFrom, ht.hItem, checkstate);
            }

            ZeroMemory(&tvi, sizeof(TVITEM));

		    tvi.mask       = TVIF_IMAGE | TVIF_TEXT;
		    tvi.pszText    = temptext;
		    tvi.cchTextMax = SIZE_1024;
		    tvi.hItem      = ht.hItem;

		    // get the image index
		    TreeView_GetItem(lpnmh->hwndFrom, &tvi);		
    		
  		    switch(tvi.iImage)
		    {
		    //To check or uncheck child nodes if parent is selected
		    case NTABLES:
		    case NVIEWS:
		    case NFUNC:
		    case NSP:
		    case NEVENTS:
		    case NTRIGGER:
 			    ClickOnFolderTreeViewItem(lpnmh->hwndFrom, ht.hItem);
			    break;
    		
		    //To check or uncheck parent node if any child selected
		    case NTABLE:
		    case NVIEWSITEM:
		    case NTRIGGERITEM:
		    case NFUNCITEM:
		    case NSPITEM:
		    case NEVENTITEM:
 			    CheckUnCheckTreeViewItems(lpnmh->hwndFrom, ht.hItem);
			    break;

		    default:
			    return NULL;
		    }

            //for items other than selected item
            if(ht.hItem != hactual)
            {
                TreeView_SetCheckState(lpnmh->hwndFrom, ht.hItem, !checkstate);
            }
        }

        //set the last click item
        hlastclickitem = hactual;
	}

    return hactual;
}

void
InitCharacterSetCombo(HWND hwnd)
{
	HWND    hwndcombo = NULL;
    wyString    query, charsetstr;
    MYSQL_ROW   myrow;
    MYSQL_RES   *myres;
    wyInt32     index;
    MDIWindow   *wnd = NULL;

	VERIFY(wnd = GetActiveWin());
    
	VERIFY(hwndcombo = GetDlgItem(hwnd, IDC_CHARSETCOMBO));
    query.SetAs("show charset");

    myres = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);
    if(!myres)
	{
        ShowMySQLError(hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return;
	}
    while(myrow = wnd->m_tunnel->mysql_fetch_row(myres))
    {
        charsetstr.SetAs(myrow[0]);
        SendMessage(hwndcombo , CB_ADDSTRING, 0,(LPARAM)charsetstr.GetAsWideChar());
    }
    if((index = SendMessage(hwndcombo , CB_ADDSTRING, 0,(LPARAM)TEXT(STR_DEFAULT))) != CB_ERR)
        SendMessage(hwndcombo, CB_SELECTSTRING, index, (LPARAM)TEXT(STR_DEFAULT));

	if(myres)
	{
		wnd->m_tunnel->mysql_free_result(myres);
	}
}

void ClickOnFolderTreeViewItem(HWND treehandle, HTREEITEM htreeitem)
{
	
	HTREEITEM  hitemchilds, hitemtemp;
	wyInt32    state ;

	state = TreeView_GetCheckState(treehandle, htreeitem); 						
	
	VERIFY(hitemchilds = TreeView_GetChild(treehandle, htreeitem));

	for(hitemtemp = hitemchilds; hitemtemp; hitemtemp= TreeView_GetNextSibling(treehandle, hitemtemp)) 
		TreeView_SetCheckState(treehandle, hitemtemp, !state);// set all its childs baesd on its parent

}
///Based on child items check un check root (if all childs are uncheck then remove check of parent
/// if any one child is check  then check its corresponding parent
wyBool CheckUnCheckTreeViewItems(HWND treehandle, HTREEITEM htreeitem)
{	

	wyInt32    state , parstate, objects = 0;
	HTREEITEM  hitempar, hitemtemp;

	parstate = TreeView_GetCheckState(treehandle, htreeitem); 


	VERIFY(hitempar = TreeView_GetParent(treehandle, htreeitem));

	for(hitemtemp =TreeView_GetChild(treehandle, hitempar);
	                                                hitemtemp; hitemtemp= TreeView_GetNextSibling(treehandle, hitemtemp)) 
	{	
		 state = TreeView_GetCheckState(treehandle, hitemtemp);
		 if(state == 1)// if child is selected then increment no of  selected child elements
			 objects += 1;		
	}
	// increment are decrement no of  selected child elements value based on present selected item check state 
	objects += (parstate ? (-1):1);

	/// if any one child is check  then check its corresponding parent
	TreeView_SetCheckState(treehandle, hitempar,(objects?1:0));	
	
	return wyTrue;
}

wyBool  GetServerCharset(wyString &charset)
{
	wyString        query;
	MYSQL_RES		*res;
	MYSQL_ROW		row;

	MDIWindow *wnd = GetActiveWin();

	query.Sprintf("show variables like 'character_set_server'");
	res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);

	if(!res)
	{
        ShowMySQLError(wnd->m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return wyFalse;
	}

	while(row = wnd->m_tunnel->mysql_fetch_row(res))
	{ 
		charset.SetAs(row[1]);
	}

	wnd->m_tunnel->mysql_free_result(res);

	return wyTrue;
}

void               
FetchSelectedCharset(HWND hwnd, wyString *selname ,wyBool iscopy)
{
    wyInt32     length = 0, ncursel = 0;
    wyWChar     *charsetname = NULL;
    wyString    chsetnamestr;
	HWND        hwndcombo = NULL;

	if(!selname)
		return;

	VERIFY(hwndcombo = GetDlgItem(hwnd, IDC_CHARSETCOMBO));

    ncursel = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0);;
    
    if(iscopy == wyFalse)
    {
        length  = SendMessage(hwndcombo, CB_GETLBTEXTLEN, ncursel, NULL);
	    charsetname    = AllocateBuffWChar(length + 1);
	    SendMessage(hwndcombo, CB_GETLBTEXT,(WPARAM)ncursel,(LPARAM)charsetname);
    }
    else
    {
        length = GetWindowTextLength(hwndcombo);
        charsetname    = AllocateBuffWChar(length + 1);
        GetWindowText(hwndcombo, charsetname, length + 1);
    }

 	selname->SetAs(charsetname);
    free(charsetname);
}

// return wytrue if  atleast one object is selected in the treeview else return wyfalse 
wyBool  IsObjectsSelected(HWND hwnd)
{
	HTREEITEM   rootitem;
	wyInt32     check = 0;
	
	VERIFY(rootitem = TreeView_GetRoot(hwnd));
    
	for(; rootitem; rootitem = TreeView_GetNextSibling(hwnd, rootitem))
	{
		check = TreeView_GetCheckState(hwnd, rootitem);
		if(check)
			return wyTrue;		
	}
	return wyFalse;
}

wyBool 
GetGUICreateTableString(MDIWindow *wnd, Tunnel * tunnel, PMYSQL mysql, const wyChar *db, 
					 const wyChar* tbl, wyString &strcreate, wyString &query, wyBool isbacktick)
{
	MYSQL_ROW			myrow;	
	MYSQL_RES			*res = NULL;
	wyChar              *err;
    wyBool              find = wyFalse;

	strcreate.Clear();

	/* select count(*)to get the number of rows from desired table */ 
    if(isbacktick == wyTrue)
	{
		if(db)
			query.Sprintf("show create table `%s`.`%s`", db, tbl);
		else
			query.Sprintf("show create table `%s`", tbl);
	}
	else
	{
		if(db)
			query.Sprintf("show create table `%s`.`%s`", db, tbl);
		else
			query.Sprintf("show create table `%s`", tbl);
	}
													
	res = ExecuteAndGetResult(wnd, tunnel, mysql, query);//, wyFalse);
    
	if(!res)
	{
			err = (wyChar*)tunnel->mysql_error(*mysql);
			return wyFalse;
	}

	myrow = tunnel->mysql_fetch_row(res);
	
	if(myrow && myrow[1])// check for the on update current_timestamp
	{
		strcreate.SetAs(myrow[1]);
		find = wyTrue;
	}
	
	tunnel->mysql_free_result(res);
	//mysql_free_result(res);
	return  find;
}

/*Getting the View statement from IF
`db.view` is not used here(only view name used) if use Schema sync will make issue
*/
wyBool	
GetCreateViewStatement(MDIWindow *wnd, Tunnel * tunnel, PMYSQL pmysql, const wyChar *db, 
											const wyChar* tb, wyString *strcreate)
{
	wyString	query;
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	wyBool		ismysql41 = IsMySQL41(tunnel, pmysql);

	if(!tunnel || !db || !tb || !wnd || !strcreate)
		return wyFalse;

	ismysql41 = IsMySQL41(tunnel, pmysql);

	if(strcreate->GetLength())
		strcreate->Clear();

	//query.Sprintf("select VIEW_DEFINITION from INFORMATION_SCHEMA.VIEWS where TABLE_SCHEMA = '%s' and TABLE_NAME = '%s'", db,  tb);
	query.Sprintf("SHOW CREATE VIEW `%s`.`%s`", db, tb); 
	
    res = ExecuteAndGetResult(wnd, tunnel, pmysql, query, wyFalse); 

	if(!res)
	{
		ShowMySQLError(wnd->m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return wyFalse;
	}

	myrow = wnd->m_tunnel->mysql_fetch_row(res);

	if(!myrow)
	{ 
		ShowMySQLError(wnd->m_hwnd, tunnel, pmysql, query.GetString());
		tunnel->mysql_free_result(res);
		return wyFalse;
	}
	if(myrow[1])
	{
		strcreate->SetAs(myrow[1]);
		//strcreate->Sprintf("create view `%s` as %s", tb, myrow[1]);
	}
		
	tunnel->mysql_free_result(res);

	return wyTrue;
}

//Gets the mySQL variable 'lower_case_table_names'
//http://dev.mysql.com/doc/refman/5.0/en/identifier-case-sensitivity.html
wyInt32
GetmySQLCaseVariable(MDIWindow *wnd)
{
	wyString	query, dbname, casevalue;
	wyInt32		fldindex, value;
	wyBool		ismysql41 = wyFalse; 
	MYSQL_RES*	res;
	MYSQL_ROW	row;

	if(!wnd)
		return wyTrue;

	ismysql41 = wnd->m_ismysql41;  

	query.Sprintf("show variables like 'lower_case_table_names'");
	res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query, wyFalse);
	//res = SjaExecuteAndGetResult(tunnel, mysql, query);
	if(!res && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		//VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));
		//ShowMySQLError(pGlobals->m_pcmainwin->m_hwndmain, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return wyFalse;
	}

	fldindex = GetFieldIndex(wnd->m_tunnel, res, "Value"); 
    row =  wnd->m_tunnel->mysql_fetch_row(res);
	if(!row)
	{
		wnd->m_tunnel->mysql_free_result(res);
		return wyFalse;
	}
	
	casevalue.SetAs(row[fldindex], ismysql41);
	wnd->m_tunnel->mysql_free_result(res);

	value = casevalue.GetAsInt32();

	return value;        
}

wyBool
IsLowercaseFS(MDIWindow *wnd)
{
	wyString	query, dbname, casevalue;
	wyInt32		fldindex;
	wyBool		ismysql41 = wyFalse; 
	MYSQL_RES*	res;
	MYSQL_ROW	row;
	wyBool		islowercasefs;
	if(!wnd)
		return wyTrue;

	ismysql41 = wnd->m_ismysql41;  

	query.Sprintf("show variables like 'lower_case_file_system'");
	res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query, wyFalse);
	//res = SjaExecuteAndGetResult(tunnel, mysql, query);
	if(!res && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		return wyTrue;
	}

	fldindex = GetFieldIndex(wnd->m_tunnel, res, "Value"); 
    row =  wnd->m_tunnel->mysql_fetch_row(res);
	if(!row)
	{
		wnd->m_tunnel->mysql_free_result(res);
		return wyTrue;
	}
	
	casevalue.SetAs(row[fldindex], ismysql41);
	if(casevalue.CompareI("ON") == 0)
		islowercasefs = wyTrue;
	else
		islowercasefs = wyFalse;

	wnd->m_tunnel->mysql_free_result(res);

	return islowercasefs;        
}

//function used in   querythread and autocompleteent 
wyInt32 GetQuerySpaceLen(int orglen, const wyChar *query)
{
    wyInt32 count;

    wyInt32 length = strlen(query);

    for(count = length-1; count >= 0; count--)
    {
        if(isspace(query[count]) == 0)
            break;
    }

    if(count < (length-1) && query[count] == ';')
        count--;

    if(orglen >= count - 1)
     return (length - count);

    return 0;
}

//Fills Content-Type combo
void FillContenttypeCombo(HWND hwndcombo)
{
	SendMessage(hwndcombo, CB_RESETCONTENT, 0, 0);
	SendMessage(hwndcombo, CB_ADDSTRING, 0,(LPARAM)L"application/x-www-form-urlencoded");
	SendMessage(hwndcombo, CB_ADDSTRING, 0,(LPARAM)L"text/plain");
	SendMessage(hwndcombo, CB_ADDSTRING, 0,(LPARAM)L"text/html");
	SendMessage(hwndcombo, CB_ADDSTRING, 0,(LPARAM)L"text/xml");
	SendMessage(hwndcombo, CB_ADDSTRING, 0,(LPARAM)L"application/xml"); 
}

//Get menu string
wyInt32 
GetMenuItemName(wyWChar *menustring, wyWChar *cmdstring)
{
    wyInt32 count = 0, cmdlen = 0;

    for(count = 0; *(menustring+count); count++, cmdlen++)
	{
		if(*(menustring + count) == C_TAB)
        {
			count++; cmdlen++;
			break;
		}

		*(cmdstring+cmdlen) = *(menustring+count);
	}
    return cmdlen;
}

//Gets Keyboard shortcut for a menu item
wyInt32 
GetKBShortcut(wyWChar *menustring, wyWChar *shortcut)
{
    wyInt32 count = 0, scutlen = 0;

    for(count = 0; *(menustring+count); count++)
	{
		if(*(menustring + count) == C_TAB)
        {
			count++;
			break;
		}
	}

    for(;*(menustring+count); count++, scutlen++)
    {
        *(shortcut+scutlen) = *(menustring+count);
    }
    return scutlen;
}

wyBool
ChangeCRToLF(wyChar * text)
{
	wyUInt32	i=0;

	while(text[i])
	{
		if(text[i] == C_CARRIAGE)
			text[i] = '\n';

		i++;
	}
	return wyTrue;
}

wyBool
AddCRToLF(wyChar * text, wyChar * temp)
{
	wyUInt32	i=0;
	wyUInt32	j=0;

	while(text[i])
	{
		temp[j++] = text[i];
		if(text[i+1])
		{
		if(text[i+1] == C_NEWLINE && text[i] != C_CARRIAGE)
			temp[j++] = '\r';
		}
		i++;
	}
	return wyTrue;
}

wyBool
OnItemExpanding(TREEVIEWPARAMS &treeparam, wyBool isrefreshnode, wyBool iscalledfromusermanager)
{
	wyString        olddatabase;
	wyBool          ret;
	HTREEITEM		hitem;
	HWND		hwnd;
	LPTVITEM	tvi;
	Tunnel		*tunnel;
	PMYSQL		mysql;
	wyWChar		*database;
	wyBool		issingletable;
	wyBool		isopentables;
	wyBool		isrefresh, checkstate;

	hwnd = treeparam.hwnd;
	tvi = treeparam.tvi;
	tunnel = treeparam.tunnel;
	mysql = treeparam.mysql;
	database = treeparam.database;
	issingletable = treeparam.issingletable;
	isopentables = treeparam.isopentables;
	isrefresh = treeparam.isrefresh;
	checkstate = treeparam.checkboxstate;
		
	// Get the pointer to class of its parent window to get pointer to MYSQL structure.
	// Also get the TVITEM of the expanding item.
	//wnd = (MDIWindow*)GetWindowLongPtr(m_hwndparent, GWLP_USERDATA);

	//_ASSERT(wnd);
	//GetOldDatabase(olddatabase);

	if(tvi && tvi->iImage == NSERVER) // no need to do anything here;
		return wyTrue;

	if(tvi && tvi->hItem)
		TreeView_SelectItem(hwnd, tvi->hItem);

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	if(isrefreshnode == wyTrue && isopentables == wyTrue)
		SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);

	///So identifies item collapsed/expanded not double click
	//m_isexpcollapse = wyTrue;

	// Grouping together all table(s)
	if(!tvi || tvi->iImage == NDATABASE && !(tvi->state & TVIS_EXPANDEDONCE))
	{	
		if(!tvi)
			hitem = TVI_ROOT;

		else
			hitem = tvi->hItem;

		InsertTableParent(hwnd, hitem, isrefreshnode); 
					
		/* starting from v4.2 we support stored procedures, functions, views and triggers */
		/* just a template to check MySQL version, in final release we have to uncomment it */

		if(IsMySQL5010(tunnel, mysql) == wyTrue)
		{
			InsertViews(hwnd, hitem);
			InsertStoredProcs(hwnd, hitem);
			InsertFunctions(hwnd, hitem);

                        if(iscalledfromusermanager == wyFalse)
                        {
			         InsertTriggers(hwnd, hitem);
			         if(IsMySQL516(tunnel, mysql) == wyTrue)
				          InsertEvents(hwnd, hitem);			
		        }
		}

		//if IsOpenTablesInOb() is wyTrue,then we expand Tables folder in Ob ,then no need to set redraw the object browser to True.
		//In Tables folder expand, we will set redraw to True 
		//if(IsOpenTablesInOB())
		if(isopentables == wyTrue)
			return wyTrue;
	}

	else if(tvi->iImage == NTABLES && issingletable == wyFalse && !(tvi->state & TVIS_EXPANDEDONCE))
	{
		/// Adds all the tables
		if(InsertTables(hwnd, tunnel, mysql, tvi->hItem, database, checkstate, isrefreshnode) == wyFalse)
            goto cleanup;	

		TreeView_SortChildren(hwnd, tvi->hItem, TRUE);
		
	}
	else if(tvi->iImage == NTABLE && !(tvi->state & TVIS_EXPANDEDONCE))
	{
		/// Adds the columns and Indexes
		ret = ExpandTableName(hwnd, tunnel, mysql, tvi->hItem, isrefreshnode, iscalledfromusermanager);		

		//TreeView_SortChildren(hwnd, tvi->hItem, TRUE);
	}

	else if(tvi->iImage == NSP && !(tvi->state & TVIS_EXPANDEDONCE))
	{
		ExpandProcedures(hwnd, tunnel, mysql, tvi->hItem, database, checkstate, isrefreshnode);
		TreeView_SortChildren(hwnd, tvi->hItem, TRUE);
	}
	else if(tvi->iImage == NFUNC && !(tvi->state & TVIS_EXPANDEDONCE))
	{
		ExpandFunctions(hwnd, tunnel, mysql, tvi->hItem, database, checkstate, isrefreshnode);
		TreeView_SortChildren(hwnd, tvi->hItem, TRUE);
	}
	else if(tvi->iImage == NVIEWS && !(tvi->state & TVIS_EXPANDEDONCE))
	{
		ExpandViews(hwnd, tunnel, mysql, tvi->hItem, database, checkstate, isrefreshnode);
		TreeView_SortChildren(hwnd, tvi->hItem, TRUE);
	}
	else if(tvi->iImage == NEVENTS && !(tvi->state & TVIS_EXPANDEDONCE))
	{
		ExpandEvents(hwnd, tunnel, mysql, tvi->hItem, database, checkstate, isrefreshnode);
		TreeView_SortChildren(hwnd, tvi->hItem, TRUE);
	}
	else if(tvi->iImage == NTRIGGER && !(tvi->state & TVIS_EXPANDEDONCE))
	{
		ExpandTriggers(hwnd, tunnel, mysql, tvi->hItem, database, checkstate, isrefreshnode);
		TreeView_SortChildren(hwnd, tvi->hItem, TRUE);
	}

	    
cleanup:   
	
	if(isrefreshnode == wyTrue && isopentables == wyTrue)
		SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);

	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return wyTrue;
}  

// Insert the node -- Tables
wyBool
InsertTableParent(HWND hwnd, HTREEITEM hDatabase, wyBool isrefresh)
{
	HTREEITEM	htables;
		
	SetCursor(LoadCursor(NULL, IDC_WAIT));
    	
	if(hDatabase)
	{
		if(hDatabase != TVI_ROOT)
			DeleteChildNodes(hwnd, hDatabase, isrefresh);

		// Inserts the node "Tables"
		htables = InsertNode(hwnd, hDatabase, TXT_TABLES, NTABLES, NTABLES, 0);
		InsertDummyNode(hwnd, htables);
	}

	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return wyTrue;
}

HTREEITEM  
InsertDummyNode(HWND hwnd, HTREEITEM hparent)
{
    TVITEM				tvi;
	TVINSERTSTRUCT		tvins;

	tvi.mask			= TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE | TVIF_PARAM;
	tvi.pszText			= (wyWChar*)L"dummy";
	tvi.cchTextMax		= wcslen(tvi.pszText);
	tvi.iImage			= NTABLE;
	tvi.iSelectedImage	= NTABLE;
    tvi.lParam          = 0;

	tvins.hParent		= hparent;
	tvins.hInsertAfter	= TVI_LAST;
	tvins.item			= tvi;

	return TreeView_InsertItem(hwnd, &tvins);
}

// Function to add all the views for the db.
wyBool
InsertViews(HWND hwnd, HTREEITEM hdatabase)
{
	HTREEITEM			htable;

	SetCursor(LoadCursor(NULL, IDC_WAIT));
    htable = InsertNode(hwnd, hdatabase, TXT_VIEWS, NVIEWS, NVIEWS, 0);
    InsertDummyNode(hwnd, htable);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	
	return wyTrue;
}

wyBool
InsertEvents(HWND hwnd, HTREEITEM hdatabase)
{
	HTREEITEM			htable;

	SetCursor(LoadCursor(NULL, IDC_WAIT));
	htable = InsertNode(hwnd, hdatabase, TXT_EVENTS, NEVENTS, NEVENTS, 0);
    InsertDummyNode(hwnd, htable);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	
	return wyTrue;
}

// Function to add all the triggers.
wyBool
InsertTriggers(HWND hwnd, HTREEITEM hdatabase)
{
	HTREEITEM			htable;

	SetCursor(LoadCursor(NULL, IDC_WAIT));
	htable = InsertNode(hwnd, hdatabase, TXT_TRIGGERS, NTRIGGER, NTRIGGER, 0);
    InsertDummyNode(hwnd, htable);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	
	return wyTrue;	
}

// Function to add all the stored procs.
wyBool
InsertStoredProcs(HWND hwnd, HTREEITEM hdatabase)
{
	HTREEITEM			htable;

	SetCursor(LoadCursor(NULL, IDC_WAIT));
    htable = InsertNode(hwnd, hdatabase, TXT_PROCEDURES, NSP, NSP, 0);
	InsertDummyNode(hwnd, htable);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	
	return wyTrue;
}

// Function to add all the functions for the db.
wyBool
InsertFunctions(HWND hwnd, HTREEITEM hdatabase)
{
	HTREEITEM			htable;

    SetCursor(LoadCursor(NULL, IDC_WAIT));
    htable = InsertNode(hwnd, hdatabase, TXT_FUNCTIONS, NFUNC, NFUNC, 0);
	InsertDummyNode(hwnd, htable);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	
	return wyTrue;
}

/* Function to add all the tables in the database.
database will be passed from other dialogs other than object browser, for object browser db name is the parent node text
*/
wyBool
InsertTables(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, HTREEITEM hdatabase, wyWChar *dbname, wyBool checkboxstate, wyBool isrefresh)
{
	wyInt32             count;
	wyString            query, myrowstr, db, dbstr;
    wyWChar             database[SIZE_512] = {0};
	wyChar				db1[SIZE_512] = {0};
	HTREEITEM			htable;
	wyBool				ismysql41 = IsMySQL41(tunnel, mysql), retbool = wyTrue;
	MYSQL_RES			*myres = NULL;
	MYSQL_ROW			myrow;
	MDIWindow			*wnd = NULL;
	wyBool				checkstate = wyFalse;

	VERIFY(wnd = GetActiveWin());
		
	if(!dbname && !hdatabase)
		return wyFalse;
	
	SetCursor(LoadCursor(NULL, IDC_WAIT));

	// First get the database name.	
	if(!dbname)
		retbool = GetNodeText(hwnd, TreeView_GetParent(hwnd, hdatabase), database, SIZE_512);		
	
	else
	{
		wcscpy(database, dbname);
		checkstate = wyTrue;
	}
	
	if(retbool == wyFalse)
		return wyFalse;

	// Now delete the first dummy column.
	DeleteChildNodes(hwnd, hdatabase, isrefresh);
	db.SetAs(database);
	strcpy(db1, db.GetString());
	dbstr.SetAs(db1, wyFalse);
		
	count = PrepareShowTable(tunnel, mysql, db, query);

	myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);
	if(!myres)
    {
		ShowMySQLError(hwnd, tunnel, mysql,query.GetString());
		return wyFalse;
    }
	
	if(tunnel->mysql_error(*mysql)[0] != '\0')
		return wyFalse;
	
	// move thru the resultset which has all the table names and them to the tree view.
	while(myrow = tunnel->mysql_fetch_row(myres))	
	{
		myrowstr.SetAs(myrow[0], ismysql41);
		htable = InsertNode(hwnd, hdatabase, myrowstr.GetAsWideChar(), NTABLE, NTABLE, 1, checkstate, checkboxstate);
		
		//dummy node insert only for table in the case of the object browser
		if(checkstate == wyFalse)
			InsertDummyNode(hwnd, htable);
	}

	tunnel->mysql_free_result(myres);
		
	// Goback to the default cursor.
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	
	return wyTrue;
}

wyBool
ExpandTableName(HWND hwnd, Tunnel* tunnel, PMYSQL mysql, HTREEITEM htable, wyBool isrefresh, wyBool iscalledfromusermanager)
{
	wyInt32	ret;
    wyWChar	tablename[SIZE_512] = {0};
	HCURSOR	hcursor;
	TVITEM	tvi;
	
	// Get the table name.
	tvi.mask		=	TVIF_TEXT | TVIF_PARAM;
	tvi.hItem		=	htable;
	tvi.pszText		=	tablename;
	tvi.cchTextMax	=	SIZE_512 - 1;
	
	ret = TreeView_GetItem(hwnd, &tvi);
	_ASSERT(ret);
		
	// Set cursor to wait and lockwindow update for less flickering.
	hcursor	= GetCursor();
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	ShowCursor(1);

	// First delete the first dummy column.
	//TreeView_DeleteItem(m_hwnd, TreeView_GetChild(m_hwnd, htable));
	DeleteChildNodes(hwnd, htable, isrefresh);
	
	// insert column information.
	ret = InsertColumns(hwnd, tunnel, mysql, htable,(wyWChar*)tablename, NULL, iscalledfromusermanager);
	if(ret == wyFalse)
		return wyFalse;

	// insert index information.
    if(iscalledfromusermanager == wyFalse)
    {
	ret = InsertIndex(hwnd, tunnel, mysql, htable,(wyWChar*)tablename);

	if(ret == wyFalse)
		return wyFalse;
    }


	// Set the cursor back.
	SetCursor(hcursor);
	ShowCursor(1);

	return wyTrue;
} 
wyBool
ExpandProcedures(HWND hwnd, Tunnel* tunnel, PMYSQL mysql, HTREEITEM hsp, wyWChar *dbname, wyBool checkstate, wyBool isrefresh)
{
	wyString            query, spdetails, rowstr, dbnamestr;
    wyWChar             database[SIZE_512]={0};
	wyBool				ismysql41 = IsMySQL41(tunnel, mysql);
	HCURSOR				hcursor;
	MYSQL_RES			*myres;
	MYSQL_ROW			myrow;
	MDIWindow			*wnd = NULL;
	wyBool				ischeckbox = wyFalse;
	wyBool				iscollate = wyFalse;
	VERIFY(wnd = GetActiveWin());

	// First get the database of the table.
	if(dbname)
	{
		wcscpy(database, dbname);
		ischeckbox = wyTrue;
	}    
	else
	{
		GetNodeText(hwnd, TreeView_GetParent(hwnd, hsp), database, SIZE_512);	
	}
    
	// Set cursor to wait and lockwindow update for less flickering.
	hcursor	= GetCursor();
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	ShowCursor(1);
	
	DeleteChildNodes(hwnd, hsp, isrefresh);
	dbnamestr.SetAs(database);

	//we use collate utf8_bin only if lower_case_table_names is 0 and lower_case_file_system is OFF
	if(GetmySQLCaseVariable(wnd) == 0)
		if(!IsLowercaseFS(wnd))
			iscollate = wyTrue;
	GetSelectProcedureStmt(dbnamestr.GetString(), query, iscollate);

    myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);
    if(!myres)
    {
		ShowMySQLError(hwnd, tunnel, mysql, query.GetString());
		return wyFalse;
    }

	while(myrow = tunnel->mysql_fetch_row(myres))	
	{
		rowstr.SetAs(myrow[0], ismysql41);
		InsertNode(hwnd, hsp, rowstr.GetAsWideChar(), NSPITEM, NSPITEM, 0, ischeckbox, checkstate);		
	}	

	tunnel->mysql_free_result(myres);

	SetCursor(hcursor);
	ShowCursor(1);

	return wyTrue;
} 

wyBool
ExpandFunctions(HWND hwnd , Tunnel* tunnel, PMYSQL mysql, HTREEITEM hfunction, wyWChar *dbname, wyBool checkstate, wyBool isrefresh)
{
	wyString            query, functiondetails, rowstr, dbnamestr;
    wyWChar             database[SIZE_512] = {0};
	wyBool				ismysql41 = IsMySQL41(tunnel, mysql);
	HCURSOR				hcursor;
	MYSQL_RES			*myres;
	MYSQL_ROW			myrow;
	MDIWindow			*wnd = NULL;
	wyBool				ischeckbox = wyFalse;
	wyBool				iscollate = wyFalse;
	VERIFY(wnd = GetActiveWin());
	
	//Gets the database name
	if(dbname)
	{
		wcscpy(database, dbname);
		ischeckbox = wyTrue;
	}
	else
	{
		GetNodeText(hwnd, TreeView_GetParent(hwnd, hfunction), database, SIZE_512);		
	}
	// Set cursor to wait and lockwindow update for less flickering.
	hcursor	= GetCursor();
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	ShowCursor(1);

	DeleteChildNodes(hwnd, hfunction, isrefresh);

	//we use collate utf8_bin only if lower_case_table_names is 0 and lower_case_file_system is OFF
	if(GetmySQLCaseVariable(wnd) == 0)
		if(!IsLowercaseFS(wnd))
			iscollate = wyTrue;
	dbnamestr.SetAs(database);
	GetSelectFunctionStmt(dbnamestr.GetString(), query, iscollate);

    myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);
    if(!myres)
    {
		ShowMySQLError(hwnd, tunnel, mysql, query.GetString());
		return wyFalse;
    }

	if(myres == NULL)
		return wyTrue;

	while(myrow = tunnel->mysql_fetch_row(myres))	
	{
		rowstr.SetAs(myrow[0], ismysql41);
		InsertNode(hwnd, hfunction, rowstr.GetAsWideChar(), NFUNCITEM, NFUNCITEM, 0, ischeckbox, checkstate);		
	}
		
	tunnel->mysql_free_result(myres);

	SetCursor(hcursor);
	ShowCursor(1);

	return wyTrue;
} 

wyBool
ExpandViews(HWND hwnd, Tunnel* tunnel, PMYSQL mysql, HTREEITEM hview, wyWChar *dbname, wyBool checkstate, wyBool isrefresh)
{
	wyString            query, rowstr, dbnamestr;
    wyWChar             database[SIZE_512] = {0};
	wyBool				ismysql41 = IsMySQL41(tunnel, mysql);
	HCURSOR				hcursor;	
	MYSQL_RES			*myres;
	MYSQL_ROW			myrow;
	MDIWindow			*wnd = NULL;
	wyBool				ischeckbox = wyFalse;
	
	VERIFY(wnd = GetActiveWin());

	//Gets the database name
	if(dbname)
	{
		wcscpy(database, dbname);
		ischeckbox = wyTrue;
	}
	else
	{		
		GetNodeText(hwnd, TreeView_GetParent(hwnd, hview), database, SIZE_512);
	}

	// Set cursor to wait and lockwindow update for less flickering.
	hcursor	= GetCursor();
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	ShowCursor(1);

	DeleteChildNodes(hwnd, hview, isrefresh);
	dbnamestr.SetAs(database);
	GetSelectViewStmt(dbnamestr.GetString(), query);

    myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);
    if(!myres)
    {
		ShowMySQLError(hwnd, tunnel, mysql, query.GetString());
		return wyFalse;
    }

	while(myrow = tunnel->mysql_fetch_row(myres))	
	{
		rowstr.SetAs(myrow[0], ismysql41);
		InsertNode(hwnd, hview, rowstr.GetAsWideChar(), NVIEWSITEM, NVIEWSITEM, 0, ischeckbox, checkstate);		
	}

	tunnel->mysql_free_result(myres);

	SetCursor(hcursor);
	ShowCursor(1);

	return wyTrue;
} 
wyBool
ExpandEvents(HWND hwnd, Tunnel* tunnel, PMYSQL mysql, HTREEITEM hevent, wyWChar *dbname, wyBool checkstate, wyBool isrefresh)
{
	wyString            query, rowstr, dbnamestr;
    wyWChar             database[SIZE_512] = {0};
	wyBool				ismysql41 = IsMySQL41(tunnel, mysql);
	HCURSOR				hcursor;	
	MYSQL_RES			*myres;
	MYSQL_ROW			myrow;
	MDIWindow			*wnd = NULL;
	wyBool				ischeckbox = wyFalse;
	wyBool				iscollate = wyFalse;
	VERIFY(wnd = GetActiveWin());

	//Gets the database name
	if(dbname)
	{
		wcscpy(database, dbname);
		ischeckbox = wyTrue;
	}
	else
	{
		GetNodeText(hwnd, TreeView_GetParent(hwnd, hevent), database, SIZE_512);		
	}

	// Set cursor to wait and lockwindow update for less flickering.
	hcursor	= GetCursor();
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	ShowCursor(1);

	DeleteChildNodes(hwnd, hevent, isrefresh);
	dbnamestr.SetAs(database);

	//we use collate utf8_bin only if lower_case_table_names is 0 and lower_case_file_system is OFF
	if(GetmySQLCaseVariable(wnd) == 0)
		if(!IsLowercaseFS(wnd))
			iscollate = wyTrue;
	GetSelectEventStmt(dbnamestr.GetString(), query, iscollate);

    myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);
    if(!myres)
    {
		ShowMySQLError(hwnd, tunnel, mysql, query.GetString());
		return wyFalse;
    }

	while(myrow = tunnel->mysql_fetch_row(myres))	
	{
		rowstr.SetAs(myrow[0], ismysql41);
		InsertNode(hwnd, hevent, rowstr.GetAsWideChar(), NEVENTITEM , NEVENTITEM , NULL, ischeckbox, checkstate);
	}

	tunnel->mysql_free_result(myres);
	
	SetCursor(hcursor);
	ShowCursor(1);

	return wyTrue;
} 

wyBool
ExpandTriggers(HWND hwnd, Tunnel* tunnel, PMYSQL mysql, HTREEITEM htrigger, wyWChar *dbname, wyBool checkstate, wyBool isrefresh)
{
	wyString            query, dbnamestr, tablenamestr, rowstr; 
    wyWChar             database[SIZE_512] = {0};
	HCURSOR				hcursor;    
	MYSQL_RES			*myres;
	MYSQL_ROW			myrow;
    wyBool              ismysql41 = IsMySQL41(tunnel, mysql);                
	MDIWindow			*wnd = NULL;
	wyBool				ischeckbox = wyFalse;

	VERIFY(wnd = GetActiveWin());

	//Gets the database name
	if(dbname)
	{
		wcscpy(database, dbname);
		ischeckbox = wyTrue;
	}
	else
	{
		GetNodeText(hwnd, TreeView_GetParent(hwnd, htrigger), database, SIZE_512);		
	}

	// Set cursor to wait and lockwindow update for less flickering.
	hcursor	= GetCursor();
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	ShowCursor(1);

	DeleteChildNodes(hwnd, htrigger, isrefresh);
	
	dbnamestr.SetAs(database);
	
	//query.Sprintf("select `TRIGGER_NAME` from `INFORMATION_SCHEMA`.`TRIGGERS` where `TRIGGER_SCHEMA` = '%s'", dbnamestr.GetString());
	query.Sprintf("SHOW TRIGGERS from `%s`", dbnamestr.GetString());
	    
    myres = ExecuteAndGetResult(wnd, tunnel, mysql, query);
	if(!myres)
    {
		ShowMySQLError(hwnd, tunnel, mysql, query.GetString());
		return wyFalse;
    }

	while(myrow = tunnel->mysql_fetch_row(myres))	
	{
		rowstr.SetAs(myrow[0], ismysql41);
		InsertNode(hwnd, htrigger, rowstr.GetAsWideChar(), NTRIGGERITEM, NTRIGGERITEM, NULL, ischeckbox, checkstate);	
	}	

	tunnel->mysql_free_result(myres);

	SetCursor(hcursor);
	ShowCursor(1);

	return wyTrue;
} 

void 
DeleteChildNodes(HWND hwnd, HTREEITEM hparent, wyBool isrefresh)
{
	HTREEITEM	ht;
	//BOOL		state = (isrefresh == wyTrue)? FALSE : TRUE;

	TreeView_SelectItem(hwnd, hparent);

	if(isrefresh == wyFalse)
		LockWindowUpdate(hwnd);
	
	while(1)
	{
		ht = TreeView_GetChild(hwnd, hparent);

		if(ht != NULL)
			TreeView_DeleteItem(hwnd, ht);
		else
			break;
	}
	
	if(isrefresh == wyFalse)
	{
		LockWindowUpdate(NULL);

		/*While refreshing the Invalidation makes Objectbrowser becomes blank, and also when create new 5x objects.
		But this needed to avoid flickering while Expand/collapse nodes
		*/	
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
	}
}

HTREEITEM  
InsertNode(HWND hwnd, HTREEITEM hparent, const wyWChar *caption, wyInt32 image, wyInt32 selimage, LPARAM lparam, wyBool ischekbox, wyBool checkstate)
{		
	TVITEM				tvi;
	TVINSERTSTRUCT		tvins;
	HTREEITEM			htreeitem;
	wyInt32				state = 0;

	tvi.mask			= TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE | TVIF_PARAM;
	tvi.pszText			= (wyWChar*)caption;
	tvi.cchTextMax		= wcslen(tvi.pszText);
	tvi.iImage			= image;
	tvi.iSelectedImage	= selimage;
    tvi.lParam			= lparam;

	tvins.hParent		= hparent;
	tvins.hInsertAfter	= TVI_LAST;
	tvins.item			= tvi;

	htreeitem =  TreeView_InsertItem(hwnd, &tvins);

	//if treeview node having checkboxe
	if(ischekbox == wyTrue)
	{
		if(checkstate == wyFalse)
			state = 0;

		else
			state = TreeView_GetCheckState(hwnd, hparent);

		TreeView_SetCheckState(hwnd, htreeitem, state)
	}

	return htreeitem;
}

wyBool 
GetNodeText(HWND hwnd, HTREEITEM hitem, wyWChar *buff, wyInt32 buffsize)
{
    TVITEM		tvi;
    wyInt32     ret;

    tvi.mask	    = TVIF_TEXT;
	tvi.hItem	    = hitem;
	tvi.cchTextMax	= buffsize - 1;
	tvi.pszText		= buff;

	ret = TreeView_GetItem(hwnd, &tvi);

    if(ret)
        return wyTrue;

    return wyFalse;
}

wyInt32  
GetNodeText(HWND hwnd, wyWChar *buff, wyInt32 buffsize)
{
	wyInt32     image, ret;
	HTREEITEM   hselitem;

	hselitem = TreeView_GetSelection(hwnd);
	
	if(!hselitem)
		return 0;

	TVITEM		tvi;
	
	tvi.mask		= TVIF_TEXT;
	tvi.hItem		= hselitem;
	tvi.pszText		= buff;
	tvi.cchTextMax	= buffsize - 1;

	ret = TreeView_GetItem(hwnd, &tvi);

	_ASSERT(ret);

	if(ret == wyFalse)
		return 0;

	image = GetItemImage(hwnd, hselitem);

	switch(image)
	{
	case NSERVER:
	case NFOLDER:
	case NTRIGGER:
	case NVIEWS:
	case NFUNC:
	case NSP:
	case NPRIMARYINDEX:
	case NINDEX:
	case NTABLES:
		return 0;

	case NPRIMARYKEY:
	case NCOLUMN:
		GetColumnName(buff);
		break;
	}
	return wcslen(buff);	
}

/**
db != NULL when Refresh the Column node, all other times 'db' will be NULL
*/
wyBool
InsertColumns(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, HTREEITEM htable, wyWChar *table, wyWChar *db, wyBool iscalledfromusermanager)
{
	wyInt32				nullval = 2, keyval = 3;
	wyString            query, columndetails;
    wyWChar             database[SIZE_512] = {0};
	HTREEITEM			htemp;
	MYSQL_RES			*myres;
	MYSQL_ROW			myrow;
    wyString			databasenamestr, tablenamestr;
	wyString			myrowstr, myrow1str, myrow2str;
	wyBool				ismysql41 = IsMySQL41(tunnel, mysql), retbool;
	wyString			keystr;

    if(!db)
	{
		retbool =  GetNodeText(hwnd, TreeView_GetParent(hwnd, TreeView_GetParent(hwnd, htable)), database, SIZE_512);
		if(retbool == wyFalse)
			return wyFalse;
		
		databasenamestr.SetAs(database);
	}

	else
		databasenamestr.SetAs(db);


	tablenamestr.SetAs(table);

	query.Sprintf("describe `%s`.`%s`", databasenamestr.GetString(), tablenamestr.GetString());

    myres = ExecuteAndGetResult(GetActiveWin(), tunnel, mysql, query);
	if(!myres)
    {
		ShowMySQLError(hwnd, tunnel, mysql, query.GetString());
		return wyFalse;
    }

    //While refreshing no need to create Main folder
	if(!db)
		htemp = InsertNode(hwnd, htable, TXT_COLUMNS, NFOLDER, NFOLDER, 0);

	else
		htemp = htable;

	nullval = GetFieldIndex(myres, "null", tunnel, mysql);
	keyval		= GetFieldIndex(myres, "key", tunnel, mysql);

	while(myrow = tunnel->mysql_fetch_row(myres))	
	{
		if(myrow[0])
			myrowstr.SetAs(myrow[0], ismysql41);
		if(myrow[1])
            myrow1str.SetAs(myrow[1], ismysql41);
		if(myrow[nullval])
			myrow2str.SetAs(myrow[nullval], ismysql41);
		
		// Now create the column detail with data we get from the result set and add them in the object browser.
        if(iscalledfromusermanager == wyTrue)
        {
            columndetails.Sprintf("%s", myrowstr.GetString());
        }
        else
        {
		columndetails.Sprintf("%s, %s",(wyChar*)myrowstr.GetString(),(wyChar*)myrow1str.GetString());
		if(stricmp((wyChar*)myrow2str.GetString(), "YES") == 0)
			columndetails.AddSprintf(", Nullable");
        }
        
		if(myrow[keyval])
		{
			keystr.SetAs(myrow[keyval], ismysql41);
			//Checking for primary key,if column is a Primary Key then we will show the Key Icon for that column. 
			if(iscalledfromusermanager == wyFalse && keystr.CompareI("PRI") == 0)
			{
				InsertNode(hwnd, htemp, columndetails.GetAsWideChar(), NPRIMARYKEY, NPRIMARYKEY, 0);
				continue;
			}
		}
        InsertNode(hwnd, htemp, columndetails.GetAsWideChar(), NCOLUMN, NCOLUMN, 0);
	}

	tunnel->mysql_free_result(myres);
	return wyTrue;
}

// Function to insert index information about the table in the object browser.
wyBool
InsertIndex(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, HTREEITEM htable, wyWChar *table, wyWChar *db)
{
	wyString            query, indexdetails;
    wyWChar              database[SIZE_512] = {0};
	HTREEITEM			htemp;
	MYSQL_RES			*myres;
	MYSQL_ROW			myrow;
        wyString         databasenamestr, tablenamestr;            
	wyString			indexname, columnname, colstr;
	wyBool				isunique = wyFalse;
	wyBool				ismysql41 = GetActiveWin()->m_ismysql41;
	wyInt32				colname, keyname; 

    if(!db)
	{
		GetNodeText(hwnd, TreeView_GetParent(hwnd, TreeView_GetParent(hwnd, htable)), database, SIZE_512 - 1);
		databasenamestr.SetAs(database);
	}
	else
		databasenamestr.SetAs(db);
	
	tablenamestr.SetAs(table);

	query.Sprintf("show index from `%s`.`%s`", databasenamestr.GetString(), tablenamestr.GetString());
    myres = ExecuteAndGetResult(GetActiveWin(), tunnel, mysql, query);

	if(tunnel->mysql_error(*mysql)[0] != '\0')
		return wyFalse;

	if(myres == NULL)
		return wyTrue;

	if(!db)
		htemp = InsertNode(hwnd, htable, TXT_INDEXES, NFOLDER, NFOLDER, 0);

	else
		htemp = htable;

	colname = GetFieldIndex(myres, "column_name", tunnel, mysql);
	keyname = GetFieldIndex(myres, "key_name", tunnel, mysql);

	while(myrow = tunnel->mysql_fetch_row(myres))	
	{
		//if index contains multiple columns
		if((strcmp(indexname.GetString(), myrow[keyname]))== 0)
		{
			colstr.SetAs(myrow[colname], ismysql41);
			indexdetails.Add(colstr.GetString());
			indexdetails.Add(", ");
		}
		else
		{
			//Inserting index node to treeview
			if(indexdetails.GetLength() > 0)
			{
				indexdetails.Strip(2);
				indexdetails.Add(")");

				//if it is unique.
				//for primary key we are not checking for unique
				if(isunique == wyTrue && indexname.CompareI("Primary") != 0)
					indexdetails.Add(", Unique");

				if(indexname.CompareI("Primary") == 0)
					InsertNode(hwnd, htemp, indexdetails.GetAsWideChar(), NPRIMARYINDEX, NPRIMARYINDEX, 0);
				else
					InsertNode(hwnd, htemp, indexdetails.GetAsWideChar(), NINDEX, NINDEX, 0);

				indexdetails.Clear();
				isunique = wyFalse;
			}

			//for a new index
			indexname.SetAs(myrow[keyname], ismysql41);
			columnname.SetAs(myrow[colname], ismysql41);
			indexdetails.Sprintf("%s (%s", indexname.GetString(), columnname.GetString());
			indexdetails.Add(", ");

			// check whether its unique.
			if(stricmp(myrow[GetFieldIndex(myres,"non_unique", tunnel, mysql)], "0")== 0)
				isunique = wyTrue;
		}
	}

	//last index
	if(indexdetails.GetLength() > 0)
	{
		indexdetails.Strip(2);
		indexdetails.Add(")");
		if(isunique == wyTrue && indexname.CompareI("Primary") != 0)
			indexdetails.Add(", Unique");

		if(indexname.CompareI("Primary") == 0)
			InsertNode(hwnd, htemp, indexdetails.GetAsWideChar(), NPRIMARYINDEX, NPRIMARYINDEX, 0);
		else
        InsertNode(hwnd, htemp, indexdetails.GetAsWideChar(), NINDEX, NINDEX, 0);
	}

	tunnel->mysql_free_result(myres);
	return wyTrue;
}

wyInt32
GetItemImage(HWND hwnd, HTREEITEM hitem)
{
	TVITEM		tvi;
	
	memset(&tvi, 0, sizeof(TVITEM));

    tvi.mask = TVIF_IMAGE;
	tvi.hItem = hitem;

	TreeView_GetItem(hwnd, &tvi);
	return tvi.iImage;
}

wyInt32
GetObjectsMyRes(HWND hwnd, MDIWindow *wnd , Tunnel *tunnel, PMYSQL mysql, MYSQL_RES *myres , wyString *query)
{
	wyInt32 count;
	myres = ExecuteAndGetResult(wnd, tunnel, mysql, *query);
	if(!(myres))
    {
		ShowMySQLError(hwnd, tunnel, mysql, query->GetString());
		return -1;
    }
	
	if(tunnel->mysql_error(*mysql)[0] != '\0')
		return -1;

	count = (myres)->row_count;
	tunnel->mysql_free_result(myres);
	return count;
}

/*For exporitn single table from object browser
Gets the all tables from object browser , and inserts into Export treeview.
'Check' the table that to be exported
*/
wyBool 
ProcessSingleTable(HWND hwnd, HWND hwndtree, wyWChar *table)
{
	HTREEITEM	htreechild;
	wyString    seltable;
	MDIWindow   *wnd = GetActiveWin();
	HTREEITEM   htreeitem = NULL,hitem, hitemtemp = NULL;
	TVITEM		tvi;
	wyWChar     buff[SIZE_512];
	HWND		hwndobj = wnd->m_pcqueryobject->m_hwnd;
	//HWND		hwndtree = GetDlgItem(hwnd, IDC_EXPORTTREE);
	wyInt32		image;

	if(!wnd)
		return wyFalse;	
		
	VERIFY(hitem = TreeView_GetSelection(hwndobj));

	image = wnd->m_pcqueryobject->GetSelectionImage();

	if(image == NCOLUMN || image == NPRIMARYKEY || image == NINDEX || image == NPRIMARYINDEX)
		VERIFY(hitem = TreeView_GetParent(hwndobj, TreeView_GetParent(hwndobj, hitem)));

	VERIFY(hitem = TreeView_GetParent(hwndobj, TreeView_GetParent(hwndobj, hitem)));
		
	htreeitem = TreeView_GetRoot(hwndtree);
	TreeView_Expand(hwndtree, htreeitem, TVE_EXPAND);
	DeleteChildNodes(hwndtree, htreeitem);

	hitem	=	TreeView_GetChild(hwndobj, hitem);
    
	//Gets node from objectbrowser and insert into treeview.
	for(hitem = TreeView_GetChild(hwndobj, hitem); hitem != NULL; hitem = TreeView_GetNextSibling(hwndobj, hitem))
	{
		tvi.mask	    = TVIF_TEXT;
		tvi.hItem	    = hitem;
		tvi.cchTextMax	= SIZE_512 - 1;
		tvi.pszText		= buff;

		VERIFY(TreeView_GetItem(hwndobj, &tvi));	
		
		seltable.SetAs(buff);

		htreechild = InsertNode(hwndtree, htreeitem, seltable.GetAsWideChar(), NTABLE, NTABLE, 0, wyFalse);
		
		//Its the node to be checked(table selected to process)
		if(wcscmp(seltable.GetAsWideChar(), table) == 0)
			hitemtemp = htreechild;	 //stores to a temperory variable	
	}
	
	if(hitemtemp)
	{
		//Checks the parent node ('Tables')
		TreeView_SetCheckState(hwndtree, htreeitem, TRUE);

		//Select , checks, ensure visible the child node(table)
		SetFocus(hwndtree);
		TreeView_SetCheckState(hwndtree, hitemtemp, TRUE);
		TreeView_EnsureVisible(hwndtree, hitemtemp);
		TreeView_SelectItem(hwndtree, hitemtemp);
	}

	return wyTrue;
}

///Store dialog co-rdinates while closing
void
StoreDialogPersist(HWND hwnd, wyChar *dlgprfifix)
{
	RECT        rc;
    wyWChar     *lpfileport=0;
    wyString    value, dirstr;
	wyWChar     directory[MAX_PATH + 1] = {0};
	wyInt32		tmpleft = 0, tmptop = 0;
	wyString	tmpstr;

    if(!dlgprfifix)
		return;

	if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyFalse)
        return;
	
	dirstr.SetAs(directory);
    
	VERIFY(GetWindowRect(hwnd, &rc));

	if(rc.left < 0)
	{
		tmpleft = -rc.left;
		rc.left = 0;		
	}
	if(rc.top < 0)
	{
		tmptop = -rc.top;
		rc.top = 0;
	}

	// write the values.
	value.Sprintf("%d", rc.left);
	wyIni::IniWriteString(dlgprfifix, "Left", value.GetString(), dirstr.GetString());

	value.Sprintf("%d", rc.top);
	wyIni::IniWriteString(dlgprfifix, "Top", value.GetString(), dirstr.GetString());
	
	value.Sprintf("%d", rc.right + tmpleft);
	wyIni::IniWriteString(dlgprfifix, "Right", value.GetString(), dirstr.GetString());
		
	value.Sprintf("%d", rc.bottom + tmptop);
	wyIni::IniWriteString(dlgprfifix, "Bottom", value.GetString(), dirstr.GetString());

	return;
}

///Initiaise the dialog with stored cordinates
void			
SetDialogPos(HWND hwnd, wyChar *dlgprfifix)
{
	RECT	    rc,rcedit, rcobj;
	wyWChar     *lpfileport=0;
	wyWChar     directory[MAX_PATH + 1] = {0};
	wyString	dirstr, section;
	wyInt32	    screenwidth, width = 0, screenheight, height = 0, twidth = 0, theight = 0;
	wyInt32		virtx, virty;
	wyInt32		diffht, diffwd;
	wyInt32		xprim, yprim, tempht = 0, tempwd = 0;
	HDC			hdcdlg;
	MDIWindow	*wnd = NULL;
	wyInt32		cxfull, cyfull, pixelsperinch, remdinch, mmwidth, left = 0, top = 0;

	if(!dlgprfifix)
		return;

    if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyFalse)
        return;

	cxfull = GetSystemMetrics(SM_CXFULLSCREEN);
	cyfull = GetSystemMetrics(SM_CYFULLSCREEN);
	
	//Screen resolution
	screenwidth = GetSystemMetrics(SM_CXSCREEN);
	screenheight = GetSystemMetrics(SM_CYSCREEN); 

	//Gets screen usable area	
	RECT rctparam;
	SystemParametersInfo(SPI_GETWORKAREA,0,&rctparam,0);

	//,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
	//Screen width in pixels
	hdcdlg = GetWindowDC(hwnd);	
	mmwidth = GetDeviceCaps(hdcdlg, HORZSIZE);
	ReleaseDC(hwnd, hdcdlg);
	
	//Pixels / inch
	pixelsperinch = screenwidth / mmwidth;
	//20 inches
	remdinch = pixelsperinch * 20;

	left = rctparam.left + remdinch / 2;
	top = rctparam.top + remdinch / 2;

	//,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
	
	//SchemaSync
	if(!strcmp(dlgprfifix, SCHEMASYNC_SECTION))
	{
		section.SetAs(SCHEMASYNC_SECTION);
		
		//----------------------
		
		twidth = rctparam.right -  left - remdinch / 2;
		theight = rctparam.bottom - top - remdinch / 2;

		//----------------------

		//twidth = cxfull - remdinch * 2;
		//theight = cyfull;

		tempwd = cxfull - twidth;
        tempht = cyfull - theight;

		//left = 0;
		//top = 0;
	}

	//For Create/Alter table
	else if(!strcmp(dlgprfifix, TABLEMAKER_SECTION))
	{
		VERIFY(wnd = GetActiveWin());
		if(!wnd)
			return;
		
		section.SetAs(TABLEMAKER_SECTION);

		VERIFY  (GetWindowRect (wnd->GetQueryObject()->m_hwnd, &rcobj ) );
		VERIFY(GetWindowRect(wnd->GetTabModule()->GetHwnd(), &rcedit));   

		left = rcobj.left+5;
		top = rcobj.top+5;
		twidth = rcedit.right-rcobj.left-15;
		theight = rcobj.bottom-rcobj.top-15;

		tempwd =  twidth;
        tempht =  theight;
	}

	//Manage Relationship dialog
	else if(!strcmp(dlgprfifix, MANRELATION_SECTION) || !strcmp(dlgprfifix, INDEXMANAGER_SECTION)
		|| !strcmp(dlgprfifix, HANDLERELATION_SECTION) || !strcmp(dlgprfifix,  HANDLEINDEX_SECTION)
		|| !strcmp(dlgprfifix, BLOBVIEWER_SECTION) || !strcmp(dlgprfifix, USERMANAGER_SECTION)
		|| /*!strcmp(dlgprfifix, SHOWVALUE_SECTION) ||*/ !strcmp(dlgprfifix, SHOWSUMMARY_SECTION)
		|| !strcmp(dlgprfifix, SHOWPREVIEW_SECTION) || !strcmp(dlgprfifix, KEYSHORTCUT_SECTION)
		|| !strcmp(dlgprfifix, COLUMNMAP_SECTION))
	{
		VERIFY(wnd = GetActiveWin());
		if(!wnd)
			return;

		left = rctparam.left;
		top = rctparam.top;
		twidth = 600;
		theight = 400;

		if(!strcmp(dlgprfifix, MANRELATION_SECTION))
			section.SetAs(MANRELATION_SECTION);

		else if(!strcmp(dlgprfifix, INDEXMANAGER_SECTION))
			section.SetAs(INDEXMANAGER_SECTION);

		else if(!strcmp(dlgprfifix, USERMANAGER_SECTION))
			section.SetAs(USERMANAGER_SECTION);
		
		else if(!strcmp(dlgprfifix, HANDLERELATION_SECTION))
		{
			twidth = 438;
			theight = 440;
			section.SetAs(HANDLERELATION_SECTION);
		}

		else if(!strcmp(dlgprfifix, HANDLEINDEX_SECTION))
		{
			twidth = 315;
			theight = 273;
			section.SetAs(HANDLEINDEX_SECTION);
		}

		else if(!strcmp(dlgprfifix, BLOBVIEWER_SECTION))
		{
			twidth = 350;
			theight = 520;
			section.SetAs(BLOBVIEWER_SECTION);
		}
		/*else if(!strcmp(dlgprfifix, SHOWVALUE_SECTION))
		{
			twidth = 350;
			theight = 350;
			section.SetAs(SHOWVALUE_SECTION);
		}*/
		else if(!strcmp(dlgprfifix, SHOWSUMMARY_SECTION))
		{
			twidth = 350;
			theight = 350;
			section.SetAs(SHOWSUMMARY_SECTION);
		}
		else if(!strcmp(dlgprfifix, SHOWPREVIEW_SECTION))
		{
			twidth = 350;
			theight = 350;
			section.SetAs(SHOWPREVIEW_SECTION);
		}
		else if(!strcmp(dlgprfifix, KEYSHORTCUT_SECTION))
		{
			twidth = 500;
			theight = 350;
			section.SetAs(KEYSHORTCUT_SECTION);
		}
		else if(!strcmp(dlgprfifix, COLUMNMAP_SECTION))
			section.SetAs(COLUMNMAP_SECTION);
		
		VERIFY(GetWindowRect (wnd->GetQueryObject()->m_hwnd, &rcobj ) );
		VERIFY(GetWindowRect(wnd->GetTabModule()->GetHwnd(), &rcedit)); 

		tempwd = cxfull - twidth;
        tempht = cyfull - theight;
	}

	else
		return;
		
	dirstr.SetAs(directory);

	rc.left = wyIni::IniGetInt(section.GetString(), "Left", left, dirstr.GetString());
	if(rc.left < 0)
		rc.left = 0;
	
	rc.top	= wyIni::IniGetInt(section.GetString(), "Top", top, dirstr.GetString());
	if(rc.top < 0)
		rc.top = top;

	rc.right = wyIni::IniGetInt(section.GetString(), "Right", width - rc.left, dirstr.GetString());
	width = rc.right - rc.left;
		
	rc.bottom = wyIni::IniGetInt(section.GetString(), "Bottom", height - rc.top, dirstr.GetString());
	height = rc.bottom - rc.top;
	
	virtx = GetSystemMetrics(SM_XVIRTUALSCREEN);
	virty = GetSystemMetrics(SM_YVIRTUALSCREEN);
		
	xprim = GetSystemMetrics(SM_CXSCREEN);
	yprim = GetSystemMetrics(SM_CYSCREEN);		

	diffht = screenheight + virty;
    diffwd = screenwidth + virtx; 

	//Make the dialog in default pos if width/heght is zero, or grater than screen size
	if((height - rc.top <= 100 || width - rc.left <= 100) || 
		((height - rc.top >= cyfull || width - rc.left >= cxfull)))
	{		
        
		if(!top)
			rc.top = tempht / 2;
		else
			rc.top = top;

		if(!left)
			rc.left = tempwd / 2;
		else
			rc.left = left;

		rc.right =  twidth + rc.left;
		rc.bottom = theight + rc.top;
	}

	else
	{	
		//if left is grater than screen width
		if(rc.left >= cxfull) 
		{
			rc.left = rc.right - width;
			rc.right = rc.left + width;
		}
        
		if(rc.right <= virtx) // lf left side
		{
			rc.left = 0;
			rc.right = width;
		}
		
		if(rc.top >= screenheight) //if down 
		{
			rc.top = screenheight - height;
			rc.bottom = rc.top + height;		
		}
		
		//if top grater than the screen height
		if(rc.top >= cyfull)
		{
			rc.top = rc.bottom - height;
			rc.bottom = rc.top + height;
		}

		if(rc.bottom <= virty) // if top
		{
			rc.top = 0;
			rc.bottom = height;
		}
	}

	wyInt32 rem = 0;
	if(rc.left < rctparam.left)
	{
		rem = rc.right - rc.left;
		if(rem >= rctparam.right - rctparam.left)
			rem = rctparam.right - rctparam.left - remdinch;

		rc.left = rctparam.left;		
		rc.right = rc.left + rem;
	}

	if(rc.right > rctparam.right)
	{
		rem = rc.right - rc.left;
		if(rem >= rctparam.right - rctparam.left)
			rem = rctparam.right - rctparam.left - remdinch;

		rc.right = rctparam.right;
		rc.left = rc.right - rem;
	}

	if(rc.top < rctparam.top)
	{
		rem = rc.bottom - rc.top;
		if(rem >= rctparam.bottom - rctparam.top)
			rem = rctparam.bottom - rctparam.top - remdinch;

		rc.top = rctparam.top;		
		rc.right = rc.top + rem;
	}

	if(rc.bottom > rctparam.bottom)
	{
		rem = rc.bottom - rc.top;
		if(rem >= rctparam.bottom - rctparam.top)
			rem = rctparam.bottom - rctparam.top - remdinch;

		rc.bottom = rctparam.bottom;
		rc.left = rc.bottom - rem;
	}

	
	MoveWindow(hwnd, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, FALSE);
}

void
HandleIndexDlgsFlicker(HWND hwnddlg, HWND hwndgrid)
{
	RECT rctgrd, rctdlg, rcttmp;
	
	GetClientRect(hwnddlg, &rctdlg);

	GetWindowRect(hwndgrid, &rctgrd);
	MapWindowPoints(NULL, hwnddlg, (LPPOINT)&rctgrd, 2);

	InvalidateRect(hwnddlg, NULL, FALSE);
	UpdateWindow(hwnddlg);
    
	//top of grid
	rcttmp.top = 0;
	rcttmp.left = rctgrd.left;
	rcttmp.right = rctgrd.right;
	rcttmp.bottom = rctgrd.top;
	InvalidateRect(hwnddlg, &rcttmp, TRUE);
	UpdateWindow(hwnddlg);	

	//bottom of grid
	rcttmp.top = rctgrd.bottom;
	rcttmp.left = rctgrd.left;
	rcttmp.right = rctgrd.right;
	rcttmp.bottom = rctdlg.bottom;
	InvalidateRect(hwnddlg, &rcttmp, TRUE);
	UpdateWindow(hwnddlg);	

	//left of grid
	rcttmp.top = 0;
	rcttmp.left = 0;
	rcttmp.right = rctgrd.left;
	rcttmp.bottom = rctdlg.bottom;
	InvalidateRect(hwnddlg, &rcttmp, TRUE);
	UpdateWindow(hwnddlg);	

	//Right of grid
	rcttmp.top = 0;
	rcttmp.left = rctgrd.right;
	rcttmp.right = rctdlg.right;
	rcttmp.bottom = rctdlg.bottom;
	InvalidateRect(hwnddlg, &rcttmp, TRUE);
	UpdateWindow(hwnddlg);	   
}

//Resize function for Show-> processlist, status, values, warning(csv import, Table->Abcvancs properties
void
ShowValueResize(HWND hwnd, HWND hwndgrid)
{
	HWND	hwndok = NULL, hwndrefresh = NULL, hwndkill = NULL;
	RECT	rctgrid, rctbutton, rctdlg, rcttemp;
	wyInt32	buttonht = 0, buttonwd = 0;
	wyInt32 top = 0, left = 0;

	hwndok = GetDlgItem(hwnd, IDOK);
	hwndrefresh = GetDlgItem(hwnd, IDC_REFRESH);
	hwndkill = GetDlgItem(hwnd, IDC_KILL);

	GetClientRect(hwnd, &rctdlg);

	//grid
	MoveWindow(hwndgrid, 8, 8, rctdlg.right - 16, rctdlg.bottom - 50, TRUE); 

	GetClientRect(hwndkill, &rctbutton);

	buttonht = rctbutton.bottom;
	buttonwd = rctbutton.right;

	GetWindowRect(hwndgrid, &rctgrid);
	MapWindowPoints(NULL, hwnd, (LPPOINT)&rctgrid, 2);

	top = rctdlg.bottom - buttonht - 10;
	left = rctgrid.right - buttonwd; 

	MoveWindow(hwndok, left, top, buttonwd, buttonht, TRUE);//ok

	GetWindowRect(hwndok, &rctbutton);
	MapWindowPoints(NULL, hwnd, (LPPOINT)&rctbutton, 2);

	left = rctbutton.left - buttonwd - 5;

	MoveWindow(hwndkill, left, top, buttonwd, buttonht, TRUE);//kill process button

	left = rctgrid.left;

    MoveWindow(hwndrefresh, left, top, buttonwd, buttonht, TRUE);//refresh button

	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd);

	//Avoid painting issue at bottom of dialog
	rcttemp.left = rctdlg.left;
	rcttemp.right = rctdlg.right;
	rcttemp.bottom = rctdlg.bottom;
	rcttemp.top = rctgrid.bottom;

	InvalidateRect(hwnd, &rcttemp, TRUE);
	UpdateWindow(hwnd);
}

wyBool
    FormatAndAddResultSet(Tunnel *tunnel, MYSQL_RES *myres, HWND hwndedit, MySQLDataEx *mdata, wyBool isformatted, wyBool istextview)
{
	wyChar              *result = NULL;
	wyString			resultstr;
	wyWChar				*reswchar = {0};
	wyBool				ismysql41 = GetActiveWin()->m_ismysql41;
	MYSQL_FIELD			*myfield = tunnel->mysql_fetch_fields(myres);

	// we check if is formatted then we return.
   	if(isformatted == wyTrue)
		return wyTrue;
	if(mdata != NULL)
		result = FormatResultSet(tunnel, myres, myfield, mdata);


	if(ismysql41 == wyFalse)
        resultstr.SetAs(result, wyFalse);
	else
		resultstr.SetAs(result);

	if(istextview == wyFalse)
    {
	reswchar = resultstr.GetAsWideChar();
	SendMessage(hwndedit, WM_SETTEXT, 0,(LPARAM)reswchar);	
}
	else
    {
        reswchar = resultstr.GetAsWideChar();
        resultstr.SetAs(reswchar);
        SendMessage(hwndedit, SCI_SETREADONLY, FALSE, 0);
        SendMessage(hwndedit, SCI_SETTEXT, resultstr.GetLength(),(LPARAM)resultstr.GetString());	
        SendMessage(hwndedit, SCI_SETREADONLY, TRUE, 0);
    }
		
	VERIFY(HeapFree(GetProcessHeap(), NULL, result)); 
	isformatted = wyTrue;

	return isformatted;
}



wyBool 
GetChildMaximized()
{	
	wyInt32 		ret; 
	wyWChar			directory[MAX_PATH + 1]={0}, *lpfileport=0;
	wyString		dirstr;
	MDIWindow		*wnd;
	wyInt32			lstyle = 0;

	VERIFY(wnd = GetActiveWin());
	
	//It is checking current active window state.
	if(wnd)
	{
		lstyle = GetWindowLongPtr(wnd->m_hwnd, GWL_STYLE);
		ret = lstyle & WS_MAXIMIZE;
			return ret? wyTrue: wyFalse;	
	}
	else
	{
		//if there is no current active window then it will take value from ini file
		SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
		dirstr.SetAs(directory);
		
        ret = wyIni::IniGetInt(SECTION_NAME, "ChildMaximized", 1, dirstr.GetString());

		return ret? wyTrue: wyFalse;	
	}
}

void 
SetChildMaximized(HWND hwnd)
{
	wyInt32     lstyle = 0;
	wyWChar     *lpfileport=0;
	wyWChar     directory[MAX_PATH + 1] = {0};
	wyString	dirstr,value;

	if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyFalse)
	    return ;
	
	dirstr.SetAs(directory);
	lstyle = GetWindowLongPtr(hwnd, GWL_STYLE);

	if(lstyle & WS_MAXIMIZE)
		value.Sprintf("1");
	else
		value.Sprintf("0");

	wyIni::IniWriteString(SECTION_NAME, "ChildMaximized", value.GetString(), dirstr.GetString());
	
	return ;
}

HICON
CreateIcon(wyUInt32 iconid)
{
	HICON icon;
    icon = LoadIcon(pGlobals->m_pcmainwin->GetHinstance(), MAKEINTRESOURCE(iconid));        

    return icon;
}


void
SetMTIColor(HWND hwnd)
{
    wyWChar     directory[MAX_PATH+1] = {0}, *lpfileport = 0;
	wyString	dirstr;
    COLORREF selectionColor,bgColor,fgColor;
    if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyTrue)
    {
        dirstr.SetAs(directory);
        selectionColor   =   wyIni::IniGetInt(GENERALPREFA, "MTISelectionColor",   DEF_TEXTSELECTION, dirstr.GetString());
        bgColor=wyIni::IniGetInt(GENERALPREFA, "MTIBgColor", DEF_BKGNDEDITORCOLOR, dirstr.GetString()); 
        fgColor=wyIni::IniGetInt(GENERALPREFA, "MTIFgColor", DEF_NORMALCOLOR, dirstr.GetString()); 

        SendMessage(hwnd,SCI_SETSELBACK,1,selectionColor);
        SendMessage(hwnd, SCI_STYLESETBACK, STYLE_DEFAULT, (LPARAM)bgColor);
        SendMessage( hwnd, SCI_SETCARETFORE,bgColor ^ 0xFFFFFF,0); //Change Caret color in editor window
        SendMessage(hwnd, SCI_STYLESETFORE, SCE_MYSQL_DEFAULT, fgColor);
        SendMessage(hwnd, SCI_STYLESETBACK, SCE_MYSQL_DEFAULT, bgColor);
        SendMessage(hwnd, SCI_STYLESETBOLD, SCE_MYSQL_DEFAULT, FALSE);
    }
}


//Set scintilla editor properties
void
SetScintillaModes(HWND hwndeditor, wyString &keywordstring, 
				  wyString &functionstring, wyBool formini, wyBool ireadonly, wyChar* lexlanguage)
{
    //set the lexer language 
	SendMessage(hwndeditor, SCI_SETLEXERLANGUAGE, 0, (LPARAM)lexlanguage);

	EditorFont::FormatEditor(hwndeditor, formini, keywordstring, functionstring);
	
	SendMessage(hwndeditor, SCI_SETMARGINWIDTHN, 0, 0);
    
	//now scintilla can accept utf8 characters
	SendMessage(hwndeditor, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
		
	// change wordwrap.
	SendMessage(hwndeditor, SCI_SETWRAPMODE, SC_WRAP_WORD, SC_WRAP_WORD);
    	
	//Make the scintilla editors read only
	if(ireadonly == wyTrue)
	{
		SendMessage(hwndeditor, SCI_SETREADONLY, false, 0);  
		SendMessage(hwndeditor, SCI_CLEARALL, 0, 0);
		SendMessage(hwndeditor, SCI_SETREADONLY, true, 0);	 
	}

    SendMessage(hwndeditor, SCI_SETVIRTUALSPACEOPTIONS, SCVS_RECTANGULARSELECTION, 0);
}

//Add Size gripper at the right-bottom corner of the resizable dialogs
void	
DrawSizeGripOnPaint(HWND hdlg)
{
	HDC				hdc;
	RECT            rectdlg;
	PAINTSTRUCT		ps;	

	hdc = BeginPaint(hdlg, &ps); 
						
	GetClientRect(hdlg, &rectdlg);
	rectdlg.left = rectdlg.right-GetSystemMetrics(SM_CXHSCROLL);
	rectdlg.top = rectdlg.bottom-GetSystemMetrics(SM_CYVSCROLL);
				
	//Draws the gripper
	DrawFrameControl(hdc, &rectdlg, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);

	EndPaint(hdlg, &ps);
}

/// To paint the Tables on Schema designer, QB,(also editor) once minimize and then maximize the Application
void
RepaintTabOnSize()
{
	EnumChildWindows(pGlobals->m_hwndclient, EnumChildProc, NULL);	

	return;
}

BOOL CALLBACK
EnumChildProc(HWND hwnd, LPARAM lParam)
{
	MDIWindow*		wnd;
	wyWChar  		classname[SIZE_128]={0};
	TabTypes		*ptabtypes = NULL;

	VERIFY(GetClassName(hwnd, classname, SIZE_128-1));

	if((wcscmp(classname, QUERY_WINDOW_CLASS_NAME_STR)== 0))
	{
		VERIFY(wnd =(MDIWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if(!wnd)
			return FALSE;
		
		ptabtypes = wnd->m_pctabmodule->GetActiveTabType();
		if(!ptabtypes || !ptabtypes->m_hwnd)
			return FALSE;

		InvalidateRect(ptabtypes->m_hwnd, NULL, TRUE);
		UpdateWindow(ptabtypes->m_hwnd);		
	}
	return TRUE;
}

void
GetTime(wyInt64 sec, wyString &timestring)
{
	wyInt32	hh, mm, ss, ms, mus;
	
    /*mus = sec % 100000;
    mus = mus % 1000;

    if(mus >= 500)
    {
        sec += (1000 - sec);
    }*/

    timestring.Clear();
    ss = sec / 1000000;
	mus = sec % 1000000;
    ms = mus / 1000;
    mus = mus % 1000;
	mm = ss / 60;
	ss = ss % 60;
	hh = mm / 60;
	mm = mm % 60;

    if(hh)
    {
        timestring.AddSprintf("%d hr", hh); 

        if(mm)
        {
            timestring.AddSprintf(" %d min", mm);
        }
    }
    else if(mm)
    {
        timestring.AddSprintf("%d min", mm);

        if(ss)
        {
            timestring.AddSprintf(" %d sec", ss);
        }
    }
    else if(ms)
    {
        timestring.AddSprintf("%d.%03d sec", ss, ms);
    }
    else
    {
        timestring.AddSprintf("%d sec", ss);
    }
}

wyBool 
InsertTab(HWND hwndcustomtab, wyInt32 index, wyInt32 image, wyString &caption, LPARAM lparam)
{
    CTCITEM         ctci;
    wyInt32         ret;

    ctci.m_mask         = CTBIF_TEXT | CTBIF_IMAGE | CTBIF_LPARAM;
	ctci.m_iimage       = image;	
	ctci.m_psztext      = (wyChar*)caption.GetString();
	ctci.m_cchtextmax   = caption.GetLength();
	ctci.m_lparam		= lparam;

	ret = CustomTab_InsertItem(hwndcustomtab, index, &ctci);
    if(ret == -1)
        return wyFalse;

    return wyTrue;
}

//Get the Font settings for Prefernce->Editor->Others, for setting the selected font to the controls
HFONT
GetOtherControlsFont(HWND hwnd)
{
	HDC			dc;
	wyUInt32    fontitalic;
	wyInt32     pixel, high;
	wyInt32     ret, fontheight;
	wyWChar     directory[MAX_PATH + 1] = {0};
	wyWChar		*lpfileport = 0;
	wyString	fontnamestr, dirstr;	
	HFONT		hfont = NULL;

	dc		=	GetDC(hwnd);

	if(!dc)
		return NULL;

	high = GetDeviceCaps(dc, LOGPIXELSY);

    VERIFY(ReleaseDC(GetParent(hwnd), dc));	
	
	// Get the complete path.
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);

	if(ret == 0)
		return NULL;
	
	dirstr.SetAs(directory);

	// get font height, font style, fontname from ini file
	wyIni::IniGetString("DataFont", "FontName", "Courier New", &fontnamestr, dirstr.GetString());
	pixel = wyIni::IniGetInt("DataFont", "FontSize", 10, dirstr.GetString());
	fontitalic = wyIni::IniGetInt("DataFont", "FontItalic", 0, dirstr.GetString());
	
	fontheight = (wyInt32)((pixel) * high/ 72.0);
	fontheight = -MulDiv(pixel, high, 72.0);
	
	VERIFY(hfont = CreateFont (fontheight, 0, 0, 0, 0, fontitalic, 0, 0, 0, 0, 0, 0, 0, fontnamestr.GetAsWideChar()));

	return hfont;
	
}

//Gets keywords and Functions for Scintilla editor
wyBool 
GetAllScintillaKeyWordsAndFunctions(wyBool bkeyword, wyString &buffer)
{
	wyString    query;
	sqlite3	    *hdb;

    buffer.Clear();

    if(OpenKeyWordsDB(&hdb) == wyFalse)
        return wyFalse;
	
    query.Sprintf("select distinct object_name as obj_name, object_type as obj_type from objects where object_type = %d  order by lower(obj_name)", 
                    (bkeyword)?AC_PRE_KEYWORD:AC_PRE_FUNCTION);
		
	YogSqliteExec(hdb, query.GetString(), MDIWindow::callback_tags , &buffer);
			
	sqlite3_close(hdb);
		
	return wyTrue;
}

//Is the selected query supporting formatting or not
wyBool
IsQuerySupportFormatting(HWND hwnd)
{
	wyChar			*query = NULL;		
	wyInt32         len = 0;
	wyString		selquery;
	wyBool			ret = wyFalse;

	len = SendMessage(hwnd, SCI_GETTEXTLENGTH, 0, 0);
    query = AllocateBuff(len + 1);
	SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)query);

	selquery.SetAs(query);
	selquery.LTrim();

	if((selquery.FindI("SELECT") == 0) || (selquery.FindI("EXPLAIN") == 0) || (selquery.FindI("UPDATE") == 0)
								||(selquery.FindI("DELETE") == 0) ||(selquery.FindI("INSERT") == 0) 
                                || (selquery.FindI("CREATE") == 0) ||(selquery.FindI("ALTER") == 0) ||(selquery.FindI("GRANT") == 0))
		ret = wyTrue;

	free(query);
	
	return ret;
}

//Check whether the query is 'SELECT' query or not

wyBool		
IsComment(HWND hwndeditor, wyBool isfunc)
{
	wyInt32 pos	= SendMessage(hwndeditor, SCI_GETCURRENTPOS, 0, 0);

	if(pos == 0)
		return wyFalse;

	pos = (isfunc) ? (pos - 2) : (pos - 1);

	wyInt32 style	= SendMessage(hwndeditor, SCI_GETSTYLEAT, pos, 0);
	
	

	if(style == SCE_MYSQL_COMMENT || 
	   style == SCE_MYSQL_COMMENTLINE || 
       style == SCE_MYSQL_HIDDENCOMMAND ||
	   style == SCE_MYSQL_SQSTRING ||
       style == SCE_MYSQL_DQSTRING)
       //|| 
	   //style == SCE_C_CHARACTER)
	   
	     return wyTrue;
	
	return wyFalse;
}

//Setting the Static Text Font
HFONT
GetStaticTextFont(HWND hwnd)
{
	HDC	        hdc = GetDC(hwnd);
	wyInt32     fontheight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	HFONT		hfont;

	hfont = CreateFont(fontheight, 0, 0, 0,
		        FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, L"Verdana");
	
	ReleaseDC(hwnd, hdc);

    return hfont;   
}

//this function will create one sqlite database if it is not exists in appdata folder and write all column widths to that file 
void 
SaveColumnWidth(HWND hwndgrid, wyString *dbname, wyString *tblname, wyString *colname,wyInt32 column)
{
	sqlite3		*hdb = NULL;/* SQLite db handle */
	wyWChar     path[MAX_PATH] = {0};
	wyString	fullpath;
	wyInt32		ret;
	wyString	query;
	wyInt32		count = 0;
			
	if(pGlobals->m_configdirpath.GetLength())
	{
		fullpath.SetAs(pGlobals->m_configdirpath);
	}

	// to get application data path
    else if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path))) 
	{
		fullpath.SetAs(path);
		fullpath.Add("\\SQLyog");
	}
    else
        return;

	fullpath.AddSprintf("\\%s", SQLITEFILE_DATATAB);

	ret = sqlite3_open(fullpath.GetString(), &hdb);

	 if(ret != SQLITE_OK || !hdb)
       return;

	sqlite3_busy_timeout(hdb, SQLITE_TIMEOUT);
    	
    SetSqliteSyncMode(hdb);
	SetSQLitePage(hdb);
	SetSQLiteCache(hdb);
	SetTempStore(hdb);

	query.Sprintf("select * from version_info");
	YogSqliteExec(hdb, query.GetString(), callback_count, &count);

	if(!count)
    {
	    query.Sprintf("create table version_info(major real, minor real)");
	    YogSqliteExec(hdb, query.GetString(), NULL, 0);

	    query.Sprintf("insert into version_info values(%s, 0.%s)", 
                COLWID_APPVERSION_MAJOR, COLWID_APPVERSION_MINOR);
	    YogSqliteExec(hdb, query.GetString(), NULL, 0);
    }

	query.Sprintf("create table column_widths(dbname varchar(256), tablename varchar(256), \
                    colname varchar(256), colwidth integer, PRIMARY KEY(dbname, tablename, colname))");

	YogSqliteExec(hdb, query.GetString(), NULL, 0);

	//write all column width to database file
	WriteColumnWidthToFile(hdb, hwndgrid, dbname, tblname, colname, column);

	if(hdb)
	    sqlite3_close(hdb);
}

//Write all column widths to SQLite Database file
void 
WriteColumnWidthToFile(sqlite3 *hdb, HWND hwndgrid, wyString *dbname, wyString *tblname, wyString *colname, wyInt32 column)
{
	wyUInt32	width;
	wyString	columnname, orgname;
	wyString	query;
	wyInt32		ret;
	
	CustomGrid_GetColumnWidth(hwndgrid, column, &width);
		
	//ENTER THE TRANSACTION
	YogSqliteExec(hdb, "BEGIN", NULL, 0);

	query.Sprintf("insert or replace into column_widths values ('%s','%s','%s',%d )", dbname->GetString(),
			tblname->GetString(),
			colname->GetString(), width);

	ret = YogSqliteExec(hdb, query.GetString(), NULL, 0);
	
	YogSqliteExec(hdb, "COMMIT", NULL, 0);
}

//this will return the column width stored in the SQlite Db if that column is exists in that DB 
wyInt32 
GetColumnWidthFromFile(wyString *dbname, wyString *table,wyString *colname)
{
	sqlite3		*hdb = NULL;/* SQLite db handle */
	wyWChar     path[MAX_PATH] = {0};
	wyString	fullpath;
	wyInt32		ret;
	wyString	query;
	wyInt32     width = 0;
	
	if(pGlobals->m_configdirpath.GetLength())
	{
		fullpath.SetAs(pGlobals->m_configdirpath);
	}

	// to get application data path
    else if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path))) 
	{
		fullpath.SetAs(path);
		fullpath.Add("\\SQLyog");
	}
    else
        return 0;

	fullpath.AddSprintf("\\%s", SQLITEFILE_DATATAB);

	ret = sqlite3_open(fullpath.GetString(), &hdb);

	 if(ret != SQLITE_OK)
       return 0;

	sqlite3_busy_timeout(hdb, SQLITE_TIMEOUT);
    	
    SetSqliteSyncMode(hdb);
	SetSQLitePage(hdb);
	SetSQLiteCache(hdb);
	SetTempStore(hdb);

	query.Sprintf("select colwidth from column_widths where dbname = '%s' and tablename = '%s' and colname = '%s'", dbname->GetString(), table->GetString(), colname->GetString());
	YogSqliteExec(hdb, query.GetString(), callback_width, &width);
	
	if(hdb)
		sqlite3_close(hdb);
	
	if(width == 0 && table->Find("reorder",0)!=-1)
		width = 115;

    return width;	
}

wyBool
HandleDatatabLimitPersistance(const wyChar *dbname, const wyChar *table, wyInt64 *startrows, wyInt32 *numrows, wyBool insertdata)
{
	//return;
	sqlite3		*hdb = NULL;/* SQLite db handle */
	wyWChar     path[MAX_PATH] = {0};
	wyString	fullpath;
	wyString	query;
	wyInt32     ret = 0, count = 0, rows = DEF_LIMIT;
	wyInt64     start = 0;
    wyBool      retvalue = wyTrue;

	if(pGlobals->m_configdirpath.GetLength())
	{
		fullpath.SetAs(pGlobals->m_configdirpath);
	}
	// to get application data path
    else if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path))) 
	{
		fullpath.SetAs(path);
		fullpath.Add("\\SQLyog");
	}
    else
    {
        return wyFalse;
    }

	fullpath.AddSprintf("\\%s", SQLITEFILE_DATATAB);

	ret = sqlite3_open(fullpath.GetString(), &hdb);

	 if(ret != SQLITE_OK)
     {
       return wyFalse;
     }

	sqlite3_busy_timeout(hdb, SQLITE_TIMEOUT);
    	
    SetSqliteSyncMode(hdb);
	SetSQLitePage(hdb);
	SetSQLiteCache(hdb);
	SetTempStore(hdb);

	if(insertdata == wyFalse)
	{
		query.Sprintf("select colwidth from column_widths where dbname = '%s' and tablename = '%s%s'", 
			dbname, table, TABLEDATAOPTION_STARTROWS);
		
		YogSqliteExec(hdb, query.GetString(), callback_strtrows64, &start);

		query.Sprintf("select colwidth from column_widths where dbname = '%s' and tablename = '%s%s'",
						dbname, table, TABLEDATAOPTION_NUMROWS);

		YogSqliteExec(hdb, query.GetString(), callback_width, &rows);

		*startrows = start;
		*numrows = rows;
		
        if(start < 0 || rows < 0)
        {
            retvalue = wyFalse;
        }
	}

	else
	{
		query.Sprintf("select * from version_info");
		YogSqliteExec(hdb, query.GetString(), callback_count, &count);

		if(!count)
		{
			query.Sprintf("create table version_info(major real, minor real)");
			YogSqliteExec(hdb, query.GetString(), NULL, 0);

			query.Sprintf("insert into version_info values(%s, 0.%s)", 
					COLWID_APPVERSION_MAJOR, COLWID_APPVERSION_MINOR);
			YogSqliteExec(hdb, query.GetString(), NULL, 0);
		}

		query.Sprintf("create table column_widths(dbname varchar(256), tablename varchar(256), \
						colname varchar(256), colwidth integer, PRIMARY KEY(dbname, tablename, colname))");

		YogSqliteExec(hdb, query.GetString(), NULL, 0);

		//ENTER THE TRANSACTION
		YogSqliteExec(hdb, "BEGIN", NULL, 0);

		query.Sprintf("insert or replace into column_widths values ('%s','%s%s','',%I64d )", 
			dbname,	table, TABLEDATAOPTION_STARTROWS, *startrows);

		ret = YogSqliteExec(hdb, query.GetString(), NULL, 0);
	
		query.Sprintf("insert or replace into column_widths values ('%s','%s%s','',%d )", 
			dbname,	table, TABLEDATAOPTION_NUMROWS, *numrows);

		ret = YogSqliteExec(hdb, query.GetString(), NULL, 0);

		YogSqliteExec(hdb, "COMMIT", NULL, 0);
	}

	//query.Sprintf("select colwidth from column_widths where dbname = '%s' and tablename = '%s' and colname = '%s'", dbname->GetString(), table->GetString(), colname->GetString());
	//YogSqliteExec(hdb, query.GetString(), callback_width, &width, &errmsg);
	
	//sqlite3_free(
	if(hdb)
		sqlite3_close(hdb);
	    
	return retvalue;
}

wyInt32
HandleTableViewPersistance(const wyChar *dbname, const wyChar *table, wyInt32 viewtype, wyBool insertdata)
{
	sqlite3		*hdb = NULL;/* SQLite db handle */
	wyWChar     path[MAX_PATH] = {0};
	wyString	fullpath;
	wyString	query;
	wyInt32     ret = 0, count = 0, viewx = 0;
	
	if(pGlobals->m_configdirpath.GetLength())
	{
		fullpath.SetAs(pGlobals->m_configdirpath);
	}

	// to get application data path
    else if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path))) 
	{
		fullpath.SetAs(path);
		fullpath.Add("\\SQLyog");
	}
    else
        return 0;

	fullpath.AddSprintf("\\%s", SQLITEFILE_DATATAB);

	ret = sqlite3_open(fullpath.GetString(), &hdb);

	 if(ret != SQLITE_OK)
       return 0;

	sqlite3_busy_timeout(hdb, SQLITE_TIMEOUT);
    	
    SetSqliteSyncMode(hdb);
	SetSQLitePage(hdb);
	SetSQLiteCache(hdb);
	SetTempStore(hdb);

	if(insertdata == wyTrue)
	{
		query.Sprintf("select * from version_info");
		YogSqliteExec(hdb, query.GetString(), callback_count, &count);

		if(!count)
		{
			query.Sprintf("create table version_info(major real, minor real)");
			YogSqliteExec(hdb, query.GetString(), NULL, 0);

			query.Sprintf("insert into version_info values(%s, 0.%s)", 
					COLWID_APPVERSION_MAJOR, COLWID_APPVERSION_MINOR);
			YogSqliteExec(hdb, query.GetString(), NULL, 0);
		}

		query.Sprintf("create table table_view(dbname varchar(256), tablename varchar(256), \
						viewtype integer, PRIMARY KEY(dbname, tablename))");

		YogSqliteExec(hdb, query.GetString(), NULL, 0);

		//ENTER THE TRANSACTION
		//YogSqliteExec(hdb, "BEGIN", NULL, 0, &errmsg);

		query.Sprintf("insert or replace into table_view values ('%s','%s',%d)", 
						dbname,	table, viewtype);

		ret = YogSqliteExec(hdb, query.GetString(), NULL, 0);
	
		//YogSqliteExec(hdb, "COMMIT", NULL, 0, &errmsg);
	}
	else
	{
		query.Sprintf("select viewtype from table_view where dbname = '%s' and tablename = '%s'", 
						dbname, table);
		
		YogSqliteExec(hdb, query.GetString(), callback_width, &viewx);			
		
		if(hdb)
		{
			sqlite3_close(hdb);
		}
		
        if(viewx == 1)
        {
#ifndef COMMUNITY
            if(!pGlobals->m_entlicense.CompareI("Professional"))
	        {
		        viewx = 0;
            }
#else
            viewx = 0;
#endif
        }

        if(viewx < 0 || viewx > 2)
        {
            viewx = 0;
        }

		return viewx;		
	}
		
	if(hdb)
	{
		sqlite3_close(hdb);
	}

    return viewtype;
}

wyInt32
HandleResulttabPagingQueryPersistance(wyUInt32 querycrc, wyInt32 *highlimit)
{
	sqlite3		*hdb = NULL;/* SQLite db handle */
	wyWChar     path[MAX_PATH] = {0};
	wyString	fullpath;
	wyString	query;
	wyInt32     ret = 0, count = 0, start = 0;
	time_t		timeval;
    
	//If this option is not enables Write/Read to SQLllite table is not doing.
	if(pGlobals->m_resuttabpageenabled == wyFalse)
		return 0;
	
	if(pGlobals->m_configdirpath.GetLength())
	{
		fullpath.SetAs(pGlobals->m_configdirpath);
	}

	// to get application data path
    else if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path))) 
	{
		fullpath.SetAs(path);
		fullpath.Add("\\SQLyog");
	}
    else
        return 0;
	
	fullpath.AddSprintf("\\%s", SQLITEFILE_DATATAB);

	ret = sqlite3_open(fullpath.GetString(), &hdb);

	 if(ret != SQLITE_OK)
       return 0;

	sqlite3_busy_timeout(hdb, SQLITE_TIMEOUT);
    	
    SetSqliteSyncMode(hdb);
	SetSQLitePage(hdb);
	SetSQLiteCache(hdb);
	SetTempStore(hdb);
	
	if(highlimit == NULL)
	{
		query.Sprintf("select num_rows from query_options where query_crc = '%u'", querycrc);

		YogSqliteExec(hdb, query.GetString(), callback_width, &start);

		if(start > 0)
		{
			//Gets the time & update ir
			timeval =  time(NULL);

			query.Sprintf("insert or replace into query_options values ('%u','%d', '%u')", 
			querycrc, start, timeval);
		}

		if(hdb)
			sqlite3_close(hdb);
		
		return start;		
	}

	else
	{		
		//Gets the time
		timeval =  time(NULL);
				
		query.Sprintf("select * from version_info");
		YogSqliteExec(hdb, query.GetString(), callback_count, &count);

		if(!count)
		{
			query.Sprintf("create table version_info(major real, minor real)");
			YogSqliteExec(hdb, query.GetString(), NULL, 0);

			query.Sprintf("insert into version_info values(%s, 0.%s)", 
					COLWID_APPVERSION_MAJOR, COLWID_APPVERSION_MINOR);
			YogSqliteExec(hdb, query.GetString(), NULL, 0);
		}

		query.Sprintf("create table query_options(query_crc integer PRIMARY KEY, num_rows integer, query_time integer)");

		YogSqliteExec(hdb, query.GetString(), NULL, 0);

		//ENTER THE TRANSACTION
		YogSqliteExec(hdb, "BEGIN", NULL, 0);

		query.Sprintf("insert or replace into query_options values ('%u','%d', '%u')", 
			querycrc, *highlimit, timeval);

		ret = YogSqliteExec(hdb, query.GetString(), NULL, 0);
	
		/*query.Sprintf("insert or replace into column_widths values ('%s','%s%s','',%d )", 
			dbname,	table, TABLEDATAOPTION_NUMROWS, *numrows);*/

		//ret = YogSqliteExec(hdb, query.GetString(), NULL, 0, &errmsg);

		YogSqliteExec(hdb, "COMMIT", NULL, 0);
	}

	if(hdb)
		sqlite3_close(hdb);

	return 0;
}

wyInt32 
callback_width(void *count, wyInt32 argc, wyChar **argv, wyChar **azColName)
{
	wyInt32	*width = (wyInt32 *)count;
	
	if(argc != 0) 
		*width = atoi(argv[0]);
	
     return 0;	
}

wyInt32		
callback_strtrows64(void *count, wyInt32 argc, wyChar **argv, wyChar **azColName)
{
	wyInt64	*width = (wyInt64 *)count;
	
	if(argc != 0) 
		*width = _atoi64(argv[0]);
	
     return 0;	
}

// Function to implement the find replace dialog box. This dialog box helps you in finding 
// a text in the query edit box and also replace it.
wyBool
FindTextOrReplace(MDIWindow * pcquerywnd, wyBool uIsReplace)
{
	static FINDREPLACE  fr;
	static wyWChar      texttofind[FIND_STR_LEN + 1];
	static wyWChar      texttoreplace[FIND_STR_LEN + 1];
	wyChar				*textfind = NULL; 
   	wyUInt32            start, end;
	EditorBase			*peditorbase = NULL;
    HWND                hwndfocus = NULL, hwnd = NULL;
	wyString			findtextstr;
    wyInt32             ret, len = 0;
	TabMgmt				*tabmgmt = NULL;
	TabEditor			*ptabeditor = NULL;
	TabHistory			*ptabhistory = NULL;
	TabObject			*ptabobject = NULL;
    TabTableData        *ptabtabledata = NULL;
#ifndef COMMUNITY
    TabDbSearch         *ptabdbsearch = NULL;
#endif
	FindAndReplace**    pfindrep = NULL;
    wyInt32             tabicon = pcquerywnd->m_pctabmodule->GetActiveTabImage();
    DataView*           pdataview;

	ptabeditor = pcquerywnd->GetActiveTabEditor();  
	if(ptabeditor)
	{
		peditorbase = ptabeditor->m_peditorbase; 
	}
    else if(tabicon == IDI_TABLE)
    {
        ptabtabledata = (TabTableData*)pcquerywnd->m_pctabmodule->GetActiveTabType();
    }
#ifndef COMMUNITY
    else if(tabicon == IDI_DATASEARCH)
    {
        ptabdbsearch = (TabDbSearch*)pcquerywnd->m_pctabmodule->GetActiveTabType();
    }
#endif
	else
	{
		ptabhistory = pcquerywnd->GetActiveHistoryTab();
		ptabobject = pcquerywnd->GetActiveInfoTab();
	}
	

	if(!ptabeditor && !ptabhistory && !ptabobject && !ptabtabledata)
    {	
#ifndef COMMUNITY
        if(!ptabdbsearch)
            return wyFalse;
#else
        return wyFalse;
#endif
    }

	// If the dialog box is available then we dont do anything.
	if(pGlobals->m_pcmainwin->m_finddlg && IsWindowVisible(pGlobals->m_pcmainwin->m_finddlg))
	{		
		SetFocus(pGlobals->m_pcmainwin->m_finddlg);
		return wyTrue;
	}
      	
    hwndfocus = GetFocus();
	if(peditorbase && hwndfocus == peditorbase->m_hwnd)
	{
		hwnd = peditorbase->m_hwnd;
		pfindrep = &peditorbase->m_findreplace;
	}
	else if (ptabhistory && hwndfocus == ptabhistory->m_hwnd)
	{
		hwnd = ptabhistory->m_hwnd;
		pfindrep = &ptabhistory->m_findreplace;
	}
    else if (ptabobject && hwndfocus == ptabobject->m_pobjinfo->m_hwnd)
	{
		hwnd = hwndfocus;
        pfindrep = &ptabobject->m_pobjinfo->m_findreplace;
	}
     else if(ptabtabledata && ptabtabledata->m_tabledata && ptabtabledata->m_tabledata->m_viewtype == TEXT 
        && hwndfocus == (ptabtabledata->m_tableview->GetActiveDispWindow()))
    {
        hwnd = hwndfocus;
        pfindrep = &ptabtabledata->m_tableview->m_findreplace;
    }
#ifndef COMMUNITY
     else if(ptabdbsearch && ptabdbsearch->m_pdataview->GetData() && ptabdbsearch->m_pdataview->GetData()->m_viewtype == TEXT
        && hwndfocus == (ptabdbsearch->m_pdataview->GetActiveDispWindow()))
	{
        hwnd = hwndfocus;
        pfindrep = &ptabdbsearch->m_pdataview->m_findreplace;
    }
#endif
	else
	{
		if(!ptabeditor || !(tabmgmt = ptabeditor->m_pctabmgmt))
        {
			return wyFalse;
        }

        if((pdataview = tabmgmt->GetDataView()) && pdataview->GetData() && 
            pdataview->GetData()->m_viewtype == TEXT && hwndfocus == pdataview->GetActiveDispWindow())
        {
            pfindrep = &pdataview->m_findreplace;
            hwnd = hwndfocus;
        }
        else if(tabmgmt->GetActiveTabIcon() == IDI_HISTORY && tabmgmt->m_phistory)
        {
            hwnd = tabmgmt->m_phistory->m_hwndedit;
            pfindrep = &tabmgmt->m_phistory->m_findreplace;
        }
        else if(tabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX && tabmgmt->m_pqueryobj && 
            hwndfocus == tabmgmt->m_pqueryobj->m_pobjectinfo->m_hwnd)
        {
            hwnd = hwndfocus;
            pfindrep = &tabmgmt->m_pqueryobj->m_pobjectinfo->m_findreplace;
        }
    }
	
	// set the flag.
	pGlobals->m_findreplace = wyTrue;

	// These lines are commented to remember last string in find dialog box
	// First clear the structure.
		
	// prepare the structure.
	memset(&fr, 0, sizeof(FINDREPLACE));

	fr.lStructSize = sizeof(FINDREPLACE);
	
	// Now check which window has the focus.
	if(hwnd)
	{
		// Get the current selection if any.
		start = SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
		end   = SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);

		len = (end - start)+ 1;
		textfind = AllocateBuff(len); 
		ret = SendMessage(hwnd, SCI_GETSELTEXT, (WPARAM)len, (LPARAM)textfind);
	
		findtextstr.SetAs(textfind );
		len = findtextstr.GetLength();
		if(len > 256)
		{
			textfind[256] = 0;
			findtextstr.SetAs(textfind);
		}
		free(textfind );

		////To remember last string in find dialog box
		if(len != 0)
			wcscpy(texttofind, findtextstr.GetAsWideChar());
		
		fr.hwndOwner = hwnd;

        //create only if it is not available
		*pfindrep = new FindAndReplace(hwnd);
		hwnd = NULL;

	}
	

    if(!findtextstr.GetLength())
        wcscpy(texttofind, pGlobals->m_pcmainwin->m_findtext.GetAsWideChar());
    else
        pGlobals->m_pcmainwin->m_findtext.SetAs(findtextstr);
    
    fr.lpstrFindWhat = texttofind;
    fr.wFindWhatLen  = FIND_STR_LEN;
    	
    if(pGlobals->m_pcmainwin->m_frstruct.hwndOwner == NULL)
    {
        pGlobals->m_pcmainwin->m_frstruct.hwndOwner = fr.hwndOwner;
        pGlobals->m_pcmainwin->m_frstruct.Flags = fr.Flags = FR_DOWN;
    }
    else
        fr.Flags = pGlobals->m_pcmainwin->m_frstruct.Flags; 

	if(uIsReplace)
	{
		fr.lpstrReplaceWith		=	texttoreplace;
		fr.wReplaceWithLen		=	FIND_STR_LEN;
        fr.Flags |= FR_DOWN;

	    //Replace dialog box will prompt only in query editor but not in result and table data tab.
		if(GetFocus()== peditorbase->m_hwnd)
			pGlobals->m_pcmainwin->m_finddlg	=	ReplaceText(&fr);
	}
	else
		pGlobals->m_pcmainwin->m_finddlg	=	FindText(&fr);

    //subclass the FindText dialog
    if(!pGlobals->m_pcmainwin->m_findproc)
        pGlobals->m_pcmainwin->m_findproc = (WNDPROC)SetWindowLongPtr(pGlobals->m_pcmainwin->m_finddlg, 
                                                                   GWLP_WNDPROC, 
                                                                   (LONG_PTR)FindAndReplace::FindWndProc);

	return wyTrue;
}

//here keywords or function cases are converting to the upper or lower based on preference
//keywords or functions are identifying by its char style bit used for displaying
//each character in scintilla editor.
//based on the colouring of character in editor it will have different char style bits
//keyword type character  char style bit value is three 
//function type character  char style bit value is four 
//same coloured characters in  scintilla will have same char style bit
void
GetStyledText(HWND hwndeditor, wyChar *text, TextRange *range, 
						 wyInt32 kwcase, wyInt32 funcase)
{
	wyUInt32 styledlen;
	wyChar* styledtext = NULL; 
	
	wyInt32 count, pos;
    
	styledlen = 2 * (range->chrg.cpMax - range->chrg.cpMin) + 2;

	//styledtext = (wyChar*) malloc(styledlen + 10);
	styledtext = AllocateBuff(styledlen + 10);
	range->lpstrText = styledtext;

	SendMessage(hwndeditor, SCI_GETSTYLEDTEXT, 0, LPARAM(range));


	count = 1;
	
	for(pos = 0; text[pos] && count < styledlen; pos++)
	{
        if(styledtext[count] == SCE_MYSQL_MAJORKEYWORD)		
		{
			if(kwcase == 0)
				text[pos] = toupper(text[pos]);
			else if (kwcase == 1)
                text[pos] = tolower(text[pos]);
		}
        else if(styledtext[count] == SCE_MYSQL_FUNCTION)			
		{
			if(funcase == 0)
				text[pos] = toupper(text[pos]);
			else if(funcase == 1)
				text[pos] = tolower(text[pos]);
		}
		count += 2;
	}

	if(styledtext)
		free(styledtext);

}

//copy the scintilla styled text (with cases) to clip board
void
CopyStyledTextToClipBoard(HWND hwnd)
{
	wyChar   *text;
	wyUInt32 len;
	TextRange range;
	wyInt32 start, end;
	wyString buffer;
	wyInt32 kwcase, funcase;

	kwcase = GetKeyWordCase();
	funcase = GetFunCase();	

	start = SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	end   = SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);

	if(start >= end)
		return;
	
	len = (end - start)+ 1;
    text = AllocateBuff(len + 10);
	SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)text);

	range.chrg.cpMin = start;
	range.chrg.cpMax = end;
	range.lpstrText = NULL;	

	//this function will give styled buffer
	GetStyledText(hwnd, text, &range, kwcase, funcase);

	SendMessage(hwnd, SCI_COPYTEXT, len + 1, (LPARAM)text);	
	
	free(text);
}

//if query editor is tab history or tab object and query editor
//if editor is any one of above types then only we need use copy styledtext process to save to clipboard 
//for normal controls we can use WM_COPY to copy to clipboard
//because we have used SCI_STYLEDTEXT message to change the keywordcase to upper or lower in scintilla
//they are just displaying what ever case we want but they are not changing the internel buffer
//due to this we are unable to copy or cut buffer with styled cases . 	
// so user will not able to copy buffer along with cases what ever it shown in window
// to solve this issue we are manuvally doing case convertion of KW OR FUN to buffer
wyBool
HandleScintillaStyledCopy(HWND hwnd, WPARAM wparam)
{
	wyInt32 state = 0;

	state = GetKeyState(VK_CONTROL);
	if(state & 0x8000)
	{
		if(wparam == VK_INSERT || wparam == 'C')
		{		
			CopyStyledTextToClipBoard(hwnd);
			return wyTrue;		
		}		
	}

	return wyFalse;
}//All the grids except "Result" tab and "Table Data" tab, if user resizes the grid column, we will store 
//the resized width in SQLite Db using Databasename as "__webyog" and table name is specific for each grid  
wyBool
OnGridSplitterMove(HWND hwndgrid, wyString *tblname, WPARAM wparam)
{
	wyString	dbname(RETAINWIDTH_DBNAME), colname; 
	wyWChar		column[SIZE_256 + 1];

	if(IsRetainColumnWidth() == wyTrue)
	{
		// Get the column title
		CustomGrid_GetColumnTitle(hwndgrid, (wyUInt32)wparam, column);
		colname.SetAs(column);

		//saving the new column width when user resizes the column
		SaveColumnWidth(hwndgrid, &dbname, tblname, &colname, (wyUInt32)wparam);
	}
	return wyTrue;

}

void
GetFormatQuery(wyString *query, wyBool ishtmlform)
{
	FormatOpts   opts;
	wyString     formatequery;
	SQLFormatter *formatter;
	wyBool       isformate;	

	formatter = new SQLFormatter();

	if(!formatter)
		return;

	formatter->SetFormatOptions(&opts);

	/*Setting always this for HTML tolve issue reported in "#8144"
	(padding lot of spaces for align As and Alias in column level, so to avoid this)*/
	if(ishtmlform == wyTrue)
		opts.m_colalign = wyFalse;

	isformate = formatter->StartQueryFormat(query, &opts, ishtmlform);

	if(isformate == wyTrue)	
		query->SetAs(formatter->m_queryFormate.GetString());
	
	if(formatter)
		delete formatter;
	
	return;
}


//Identify whether LIMIT present in Outer query or not
wyBool
IstoAddLimitClausewithSelect(const wyChar* query)
{
	
	wyInt32			regexret = 0, sub;
	wyString		pattern, res, uncommstr, tempsbstr, temp, tempdump, fromstr;
	SQLFormatter    formatter;
    
	tempdump.SetAs(query);

	formatter.GetQueryWtOutComments(&tempdump, &uncommstr);  

	tempdump.SetAs(uncommstr);

	pattern.SetAs("^\\([\\s]*");
		
	regexret = formatter.MatchStringAndGetResult(tempdump.GetString(),
					(wyChar*)pattern.GetString(),PCRE_COMPLIE_OPTIONS, &res,
					wyTrue, wyTrue);
	    
	if(regexret != -1)
	{
		while(1)
		{
			//crash fix http://forums.webyog.com/index.php?showtopic=7438
			if(!(tempdump.Substr(formatter.m_matchendpos, tempdump.GetLength())))
				return wyFalse;

			res.SetAs(tempdump.Substr(formatter.m_matchendpos, tempdump.GetLength()));

			sub = GetStrLenWithInParanthesis(res.GetString(), wyTrue);
			
			if(sub == -1)
				break;

			res.SetCharAt(sub, ' ');

			pattern.SetAs("^\\([\\s]*");
		
			tempdump.SetAs(res);

			regexret = formatter.MatchStringAndGetResult(tempdump.GetString(),
						(wyChar*)pattern.GetString(),PCRE_COMPLIE_OPTIONS, &res,
						wyTrue, wyTrue);

			if(regexret == -1)
				break;			
		}
	}
	
	pattern.SetAs("^[\\s]*SELECT[\\s\'\"\\*]");
	
	regexret = formatter.MatchStringAndGetResult(tempdump.GetString(),
					(wyChar*)pattern.GetString(),PCRE_COMPLIE_OPTIONS, &res,
					wyTrue, wyTrue);

	if(regexret == -1)
		return wyFalse;

	tempsbstr.SetAs(res);
	
	pattern.SetAs("(\\([\\s]*(SELECT)\\*?[\\s]*)");

	regexret = formatter.MatchStringAndGetResult(tempsbstr.GetString(),
	(wyChar*)pattern.GetString(),PCRE_COMPLIE_OPTIONS, &res,
	wyTrue, wyTrue);

	fromstr.SetAs(tempsbstr.Substr(0, formatter.m_matchstartpos));


	if(regexret != 0)
	{
		pattern.SetAs("(\\([\\s]*(SELECT)\\*?[\\s]*)");
	}
	
	while(1)
	{				
		pattern.SetAs("(\\([\\s]*(SELECT)\\*?[\\s]*)");

		regexret = formatter.MatchStringAndGetResult(tempsbstr.GetString(),
		(wyChar*)pattern.GetString(),PCRE_COMPLIE_OPTIONS, &res,
		wyTrue, wyTrue);

		if(regexret < 0)
			break;

		tempsbstr.SetAs(res);

		sub = GetStrLenWithInParanthesis(tempsbstr.GetString(), wyTrue);
		
		tempsbstr.Replace(0, sub+1, "");
	
		if(sub == -1)
			break;
	}				
	
	tempdump.Sprintf(" %s %s", fromstr.GetString(), tempsbstr.GetString());
	
	pattern.SetAs("[\\s]*[^\\d\\w]*(FROM)[\\s]+");
	regexret = formatter.MatchStringAndGetResult(tempdump.GetString(),
	(wyChar*)pattern.GetString(),PCRE_COMPLIE_OPTIONS, &res,
	wyTrue, wyTrue);
	
	if(regexret == -1)
		return wyFalse;

	//Check SELECT options for those LIMIT clause is not valid
	if(CheckforOtherSelectOptions(&formatter, &tempdump) == wyFalse)
		return wyFalse;
	
	//Looking for LIMIT already present or not
	pattern.SetAs("[\\s]*[^\\d\\w\'\"`](LIMIT)[\\s]+[^\\w\\(\\)\'\"`]*[\\d]*");
	regexret = formatter.MatchStringAndGetResult(tempdump.GetString(),
	(wyChar*)pattern.GetString(),PCRE_COMPLIE_OPTIONS, &res,
	wyTrue, wyTrue);	
		
	if(regexret == -1)	
		return wyTrue;

	else
		return wyFalse;
}

/*Check SELECT options for those LIMIT clause is not valid
PROCEDURE, INTO OUTFILE/DUMP FILE
*/
wyBool
CheckforOtherSelectOptions(VOID *ptr,wyString *selectstmt) 
{
	wyString	 pattern;
	wyString	 res;
	wyInt32		 regexret = 0;
	SQLFormatter *formatter = NULL;

	formatter =(SQLFormatter*) ptr;

	if(!formatter)
		return wyFalse;
	
	// 1. PROCEDURE
	pattern.SetAs("[\\s]*[^\\d\\w\'\"`](PROCEDURE)[\\s]+[^\\w\\(\\)\'\"`]*");
	regexret = formatter->MatchStringAndGetResult(selectstmt->GetString(),
	(wyChar*)pattern.GetString(),PCRE_COMPLIE_OPTIONS, &res,
	wyTrue, wyTrue);

	if(regexret == 1)	
		return wyFalse;

	//INTO FILE/DUP FILE/OUT FILE
	pattern.SetAs("[\\s]*[^\\d\\w\'\"`](INTO)[\\s]+[^\\w\\(\\)\'\"`]*");
	regexret = formatter->MatchStringAndGetResult(selectstmt->GetString(),
	(wyChar*)pattern.GetString(),PCRE_COMPLIE_OPTIONS, &res,
	wyTrue, wyTrue);

	if(regexret == 1)	
		return wyFalse;
	
	//4. FOR UPDATE
	pattern.SetAs("[\\s]*[^\\d\\w\'\"`](FOR)[\\s]+(UPDATE)[\\s]*[^\\w\\(\\)\'\"`]*");
	regexret = formatter->MatchStringAndGetResult(selectstmt->GetString(),
	(wyChar*)pattern.GetString(),PCRE_COMPLIE_OPTIONS, &res,
	wyTrue, wyTrue);

	if(regexret == 1)	
		return wyFalse;

	//5. LOCK IN SHARE MODE
	pattern.SetAs("[\\s]*[^\\d\\w\'\"`](LOCK)[\\s]+(IN)[\\s]+(SHARE)[\\s]*(MODE)[\\s]*[^\\w\\(\\)\'\"`]*");
	regexret = formatter->MatchStringAndGetResult(selectstmt->GetString(),
	(wyChar*)pattern.GetString(),PCRE_COMPLIE_OPTIONS, &res,
	wyTrue, wyTrue);
	
	if(regexret == 1)	
		return wyFalse;

	return wyTrue;
}

/*
Checking AUTO_INCREMENT option with CREATE TABLE statement.
If found just trim it
*/
wyBool
CheckAutoIncrementTableOption(wyString *createtableorg)
{
	wyInt32		 regexret = 0;
	wyString	 res, pattern, tmpstr;
	SQLFormatter formatter;

	if(!createtableorg || !createtableorg->GetLength())
		return wyFalse;
	
	pattern.SetAs("(AUTO_INCREMENT)[\\s]*=[\\s\\d]*");

	tmpstr.SetAs(*createtableorg);
	
	regexret = formatter.MatchStringAndGetResult(createtableorg->GetString(),
							(wyChar*)pattern.GetString(),PCRE_COMPLIE_OPTIONS, &res,
							wyTrue, wyTrue);
	
	//If fmatch the pattern trim the AUTO_INCREMENT option and sets to variable
	if(regexret == 1)	
	{
		if(formatter.m_matchstartpos > 0)
		{
			tmpstr.SetAs(createtableorg->Substr(0, formatter.m_matchstartpos));
			tmpstr.Add(res.GetString());
		}
	}

	createtableorg->SetAs(tmpstr);	

	return wyTrue;
}

//Check whether the Query to be executed from editor is SELECT uery or not(for PQA)
wyBool
IsQuerySELECT(const wyChar* query)
{	
	wyString	querytemp, querynocomments;
	wyString pattern;
	wyInt32  matchret;
	wyChar *searchpos = NULL, *querystr;
	wyChar  sepchar;
	SQLFormatter formatter;
	
	if(!query)
		return wyFalse;

	querytemp.SetAs(query);
	
	formatter.GetQueryWtOutComments(&querytemp, &querynocomments);
    
	querytemp.SetAs(querynocomments);
	querytemp.LTrim();
	
	pattern.SetAs("(^(--\\h|--\\t|#)[^\\n]+\\n\\s*select|^select");
	pattern.Add("|\\/\\*!(\\d+)?\\s*SELECT)[\\s*'\"\\(]");


	matchret = IsMatchStringPattern(querytemp.GetString(), (wyChar*)pattern.GetString(), 
			PCRE_UTF8|PCRE_CASELESS);

	if(matchret == NOTMATCHED)
	{
		if(querytemp.GetCharAt(0) == '/' && querytemp.GetCharAt(1) == '*')
		{
			
			querystr = (wyChar*)querytemp.GetString();
			searchpos = strstr(querystr, "*/");

			if(!searchpos)
				return wyFalse;

			querytemp.Erase(0, (searchpos - querystr + 2));
			querytemp.LTrim();

			sepchar = querytemp.GetCharAt(6);			

			if(querytemp.CompareNI("select", 6) == 0 || 
				(isspace(sepchar) != 0 || sepchar == '"' || sepchar == '\'' || sepchar == '('))
				return wyTrue;
			else
				return wyFalse;
		}

		return wyFalse;
	}

	return wyTrue;	
}

wyBool
IsQueryDeleteInsertReplaceUpdate(const char *query)
{
    wyString	querytemp, querynocomments;
	wyString pattern;
	wyInt32  matchret;
	wyChar *searchpos = NULL, *querystr;
	wyChar  sepchar;
	SQLFormatter formatter;
	
	if(!query)
		return wyFalse;

	querytemp.SetAs(query);
	
	formatter.GetQueryWtOutComments(&querytemp, &querynocomments);
    
	querytemp.SetAs(querynocomments);
	querytemp.LTrim();
	
	pattern.SetAs("(^(--\\h|--\\t|#)[^\\n]+\\n\\s*insert|^insert");
	pattern.Add("|\\/\\*!(\\d+)?\\s*INSERT)[\\s*'\"\\(]");
    pattern.Add("|(^(--\\h|--\\t|#)[^\\n]+\\n\\s*delete|^delete");
    pattern.Add("|\\/\\*!(\\d+)?\\s*DELETE)[\\s*'\"\\(]");
    pattern.Add("|(^(--\\h|--\\t|#)[^\\n]+\\n\\s*update|^update");
    pattern.Add("|\\/\\*!(\\d+)?\\s*UPDATE)[\\s*'\"\\(]");
    pattern.Add("|(^(--\\h|--\\t|#)[^\\n]+\\n\\s*replace|^replace");
    pattern.Add("|\\/\\*!(\\d+)?\\s*REPLACE)[\\s*'\"\\(]");


	matchret = IsMatchStringPattern(querytemp.GetString(), (wyChar*)pattern.GetString(), 
			PCRE_UTF8|PCRE_CASELESS);

	if(matchret == NOTMATCHED)
	{
		if(querytemp.GetCharAt(0) == '/' && querytemp.GetCharAt(1) == '*')
		{
			
			querystr = (wyChar*)querytemp.GetString();
			searchpos = strstr(querystr, "*/");

			if(!searchpos)
				return wyFalse;

			querytemp.Erase(0, (searchpos - querystr + 2));
			querytemp.LTrim();

			sepchar = querytemp.GetCharAt(6);			

			if(querytemp.CompareNI("insert", 6) == 0 || querytemp.CompareNI("update", 6) == 0 || querytemp.CompareNI("delete", 6) == 0 || querytemp.CompareNI("replace", 7) == 0|| 
				(isspace(sepchar) != 0 || sepchar == '"' || sepchar == '\'' || sepchar == '('))
				return wyTrue;
			else
				return wyFalse;
		}

		return wyFalse;
	}

	return wyTrue;	
}

/*Format the 'Create View' statement for 'View DDL Information'
Find the 'SELECT' in view statement, extrat it , Format and apend it.
*/
VOID
FormatCreateViewStatement(wyString *query, const wyChar *objectname, wyBool ishtml)
{
	wyString selectstmt, viewstmt, tempquery;
	wyInt32 startpos = 0, endpos = 0, selectpos = 0;
	wyBool istrimright = wyFalse;

	if(!query)
		return;

	tempquery.SetAs(*query);

	//looking for view name in CREATE statement
	startpos = tempquery.FindI((wyChar*)objectname, 0);

	if(startpos == -1)
		return;

	//Looking for SELECT 
	selectpos = tempquery.FindI("select", startpos + strlen(objectname));
	if(selectpos <= 0)
		return;

	//Check if paraenthesis'(' before SELECT, if so we need to remove closing paranthesis at end of CREATE statement
	if(tempquery.GetCharAt(selectpos - 1) == '(')
		istrimright = wyTrue;

	endpos = tempquery.GetLength();

	//Remove closing paranthesis at the end of SELECT statement
	if(istrimright == wyTrue)
		endpos -= 1;

	if(!endpos || selectpos >= endpos)
		return;

	//SELECT statement to be formatted
	selectstmt.SetAs(tempquery.Substr(selectpos, endpos - selectpos));
	
	viewstmt.SetAs(tempquery.Substr(0, selectpos));

	//Get the 'SELECT' query to be formatted
	GetFormatQuery(&selectstmt, ishtml);

	if(istrimright == wyTrue)
		selectstmt.Add(")");

	if(ishtml == wyTrue)
		tempquery.Sprintf("%s<br>%s", viewstmt.GetString(), selectstmt.GetString());

	else
		tempquery.Sprintf("%s\r\n%s", viewstmt.GetString(), selectstmt.GetString());

	query->SetAs(tempquery);
}

//Format the Create table schema by removing new line and trim extra spaces
wyBool
FormatCreateTablestatement(wyString *schema)
{
	wyInt32		i = 0 ; 
	wyInt32		lenschema = 0; //length of schema
	wyInt32 numbacktick = 0; //tracking back-ticks
	wyString schematable; 
	wyChar  *dupschema = NULL; //for duplicating schema

	if(!schema || !schema->GetLength())
		return wyFalse;

	schematable.SetAs(*schema);
	
	//Remove all new lines
	schematable.FindAndReplace("\r", "");
	schematable.FindAndReplace("\n", "");

	lenschema = schematable.GetLength();
	dupschema = strdup(schematable.GetString());

	if(!dupschema)
		return wyFalse;

	schematable.Clear();

	while(dupschema[i] && i <= lenschema)
	{
		//we do'nt remove space beween 2 back ticks
		if(dupschema[i] == '`')
		{
			numbacktick++;

			if(!(numbacktick % 2))
			{
				numbacktick = 0;
				schematable.AddSprintf("%c", dupschema[i]);			            			
				i++;
				continue;
			}
		}
				
		if(numbacktick % 2)
		{
			schematable.AddSprintf("%c", dupschema[i]);
			i++;

			continue;
		}
				
		//Allowing single space only
		if(dupschema[i] == ' ' || dupschema[i] == '(')
		{
			schematable.AddSprintf("%c", dupschema[i]);

			while(dupschema[++i] == ' ');
			
			if(dupschema[i] == '`')
				continue;
			
		}

        schematable.AddSprintf("%c", dupschema[i]);		

		i++;
	}

	free(dupschema);

	schematable.AddSprintf("%c", '\0');

	schema->SetAs(schematable);

	return wyTrue;
}

wyBool
FormatQueryByRemovingSpaceAndNewline(wyString *schema)
{	
	wyInt32		i = 0 ; 
	wyInt32		lenschema = 0; //length of schema
	wyInt32 numbacktick = 0; //tracking back-ticks
	wyString schematable; 
	wyChar  *dupschema = NULL; //for duplicating schema

	if(!schema || !schema->GetLength())
		return wyFalse;

	schematable.SetAs(*schema);
	
	//Remove all new lines
	schematable.FindAndReplace("\r", "");
	schematable.FindAndReplace("\n", " ");

	dupschema = strdup(schematable.GetString());

	if(!dupschema)
		return wyFalse;

    lenschema = strlen(dupschema);
	schematable.Clear();

	while(i < lenschema && dupschema[i])
	{
		//we do'nt remove space beween 2 back ticks
		if(dupschema[i] == '`' || dupschema[i] == '\'' || dupschema[i] == '"')
		{
			numbacktick++;

			if(!(numbacktick % 2))
			{
				numbacktick = 0;
				schematable.AddSprintf("%c", dupschema[i]);			            			
				i++;
				continue;
			}
		}
				
		if(numbacktick % 2)
		{
			schematable.AddSprintf("%c", dupschema[i]);
			i++;

			continue;
		}
				
		//Allowing single space only
		if(dupschema[i] == ' ' || dupschema[i] == '\t')
		{
			schematable.Add(" ");

			while((i <= lenschema) && (dupschema[i] == ' ') || (dupschema[i] == '\t'))
			{
				i++;
			}
			
			
			if(dupschema[i] == '`' || dupschema[i] == '\'' || dupschema[i] == '"')
				continue;			
		}

        schematable.AddSprintf("%c", dupschema[i]);		

		i++;
	}

	free(dupschema);

	schematable.AddSprintf("%c", '\0');

	schema->SetAs(schematable);

	return wyTrue;
}

void
CopyWithoutWhiteSpaces(HWND hwnd)
{
    wyString    scitext;
    wyUInt32    styledlen;
	wyChar*     styledtext = NULL; 
	wyInt32     count, pos;
    wyChar      *text;
	wyUInt32    len;
	TextRange   range;
	wyInt32     start, end;
	wyString    buffer;
    wyInt32     kwcase, funcase;
    HGLOBAL		hglbcopy;
	LPWSTR		lpstrcopy;
    wyWChar		*buffercb;
    wyUInt32    length;

	kwcase = GetKeyWordCase();
	funcase = GetFunCase();	

	start = SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
	end   = SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);

	if(start >= end)
		return;
	
	len = (end - start)+ 1;
    text = AllocateBuff(len + 10);
	SendMessage(hwnd, SCI_GETSELTEXT, 0, (LPARAM)text);

	range.chrg.cpMin = start;
	range.chrg.cpMax = end;
	range.lpstrText = NULL;	
    
    styledlen = 2 * (range.chrg.cpMax - range.chrg.cpMin) + 2;

	//styledtext = (wyChar*) malloc(styledlen + 10);
	styledtext = AllocateBuff(styledlen + 10);
	range.lpstrText = styledtext;

    SendMessage(hwnd, SCI_GETSTYLEDTEXT, 0, LPARAM(&range));


	count = 1;

	for(pos = 0; text[pos] && count < styledlen;)
	{
        if(styledtext[count] != SCE_MYSQL_STRING && styledtext[count] != SCE_MYSQL_DQSTRING
            && styledtext[count] != SCE_MYSQL_SQSTRING)		
		{
			if(text[pos] == '\r')
            {
                pos++;
                count += 2;
                continue;
            }

            if(text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\n')
		    {
			    scitext.Add(" ");

    			while((text[pos]) && (text[pos] == ' ') || (text[pos] == '\t') 
                    || (text[pos] == '\r') || (text[pos] == '\n'))
	    		{
		    		pos++;
                    count += 2;
			    }
			
		    }
            
            if(count < styledlen && styledtext[count] == SCE_MYSQL_MAJORKEYWORD)		
		    {
			    if(kwcase == 0)
				    text[pos] = toupper(text[pos]);
    			else if (kwcase == 1)
                    text[pos] = tolower(text[pos]);
		    }
            else if(count < styledlen && styledtext[count] == SCE_MYSQL_FUNCTION)			
		    {
			    if(funcase == 0)
				    text[pos] = toupper(text[pos]);
			    else if(funcase == 1)
				    text[pos] = tolower(text[pos]);
		    }
        }

        if(text[pos] && count < styledlen)
        {
            scitext.AddSprintf("%c", text[pos]);
            pos++;
            count += 2;
        }
	}

	if(styledtext)
		free(styledtext);
    if(text)
        free(text);

    // copy data to the buffer
    hglbcopy = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, ((scitext.GetLength() +  1) * 2));
	if(!hglbcopy)
        return;

	lpstrcopy = (wyWChar*)GlobalLock(hglbcopy);	
	buffercb = scitext.GetAsWideChar(&length);
	wcsncpy(lpstrcopy, buffercb, length);

    if(GlobalUnlock(hglbcopy) != NO_ERROR)
        return;

	if(!(OpenClipboard(hwnd)))
		return;
	
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, hglbcopy);
	CloseClipboard();
	return;

 }


//resizes keyboard shortcut dialog
void
KeyShortCutResize(HWND hwnd)
{
	HWND    hwndedit, hwndbutton, hwndline;
	RECT	rctedit, rctbutton, rctdlg, rcttmp, rctline;
	wyInt32	buttonht = 0, buttonwd = 0, height = 0, width = 0;
	wyInt32 top = 0, left = 0;

	hwndbutton = GetDlgItem(hwnd, IDOK);
	hwndedit = GetDlgItem(hwnd, IDC_KEYSHORTS);
	hwndline = GetDlgItem(hwnd, IDC_KEYSTATIC);

	GetClientRect(hwnd, &rctdlg);
	GetClientRect(hwndbutton, &rctbutton);
	GetClientRect(hwndedit, &rctedit);
	GetWindowRect(hwndline, &rctline);
	
	//For OK button
	buttonht = rctbutton.bottom;
	buttonwd = rctbutton.right;

	top = rctdlg.bottom - buttonht - WIDTHBETBUTTON;
	left = rctdlg.right - buttonwd - WIDTHBETBUTTON; 

	MoveWindow(hwndbutton, left, top, buttonwd, buttonht, TRUE);

	//For bottomline
	MoveWindow(hwndline, rctdlg.left + WIDTHBETBUTTON, top - WIDTHBETBUTTON, (rctdlg.right - WIDTHBETBUTTON)
				- (rctdlg.left + WIDTHBETBUTTON), rctline.bottom - rctline.top, FALSE);
	MapWindowPoints(NULL, hwnd, (LPPOINT)&rctline, 2);


	//For edit box
	width = 45;
	height = top -  25;
	MoveWindow(hwndedit, rctdlg.left + width, rctdlg.top + WIDTHBETBUTTON, (rctdlg.right - 10) - 
				(rctdlg.left + width), height, FALSE);
	MapWindowPoints(NULL, hwnd, (LPPOINT)&rctedit, 2);

	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd);

	//left of edit box
	rcttmp.top = 0;
	rcttmp.left = rctdlg.left;
	rcttmp.right = rctedit.left;
	rcttmp.bottom = rctdlg.bottom;
	InvalidateRect(hwnd, &rcttmp, TRUE);
	UpdateWindow(hwnd);	

	//top of edit box
	rcttmp.top = 0;
	rcttmp.left = rctdlg.left;
	rcttmp.right = rctdlg.right ;
	rcttmp.bottom = rctedit.top;
	InvalidateRect(hwnd, &rcttmp, TRUE);
	UpdateWindow(hwnd);	

	//bottom of edit box
	rcttmp.top = rctedit.bottom;
	rcttmp.left = rctdlg.left;
	rcttmp.right = rctdlg.right;
	rcttmp.bottom = rctdlg.bottom;
	InvalidateRect(hwnd, &rcttmp, TRUE);
	UpdateWindow(hwnd);	

	//right of edit box
	rcttmp.top = 0;
	rcttmp.left = rctedit.right;
	rcttmp.right = rctdlg.right;
	rcttmp.bottom = rctdlg.bottom;
	InvalidateRect(hwnd, &rcttmp, TRUE);
	UpdateWindow(hwnd);	
}

wyBool
CreateInitFileInApplData()
{
    wyWChar  path[MAX_PATH+1] = {0};
    HANDLE  hfile;

	//If explict path is defined
	if(pGlobals->m_configdirpath.GetLength())
	{
		//wcscpy(path, pGlobals->m_configdirpath.GetAsWideChar());
		wcsncpy(path, pGlobals->m_configdirpath.GetAsWideChar(), MAX_PATH);
		path[MAX_PATH] = '\0';
	}
	
    else if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path)))
        wcscat(path, L"\\SQLyog");
	
	else
		return wyFalse;

	/* first we will create a directory that will hold .ini file with name SQLyog.*/
	if(!CreateDirectory(path, NULL))
	{
		/* If the folder is there, then we will continue the process, otherwise we will return false */
		if(GetLastError()!= ERROR_ALREADY_EXISTS)
			return wyFalse;
	}

	wcscat(path, L"\\");
	wcscat(path, L"sqlyog.ini");

	hfile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL,
						OPEN_EXISTING, NULL, NULL);

	// if it exists then we return.
	if(hfile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hfile);
		return wyTrue;
	}

	// It does not exist.
	VERIFY(hfile = CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL));
    			
	if(hfile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hfile);
		return wyTrue;
	}
    
    return wyFalse;
}

// This function creates the sqlyog.ini file in the current directory.
// So that we always have a ini file whenever we start the application.
wyBool
CreateInitFile()
{
	wyWChar     path[MAX_PATH] = {0};
	wyString	fullpath;

	if(pGlobals->m_configdirpath.GetLength())
	{
		//wcscpy(path, pGlobals->m_configdirpath.GetAsWideChar());
		wcsncpy(path, pGlobals->m_configdirpath.GetAsWideChar(), MAX_PATH - 1);
		path[MAX_PATH - 1] = '\0';
	}	
    else if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path)))
	{
        wcscat(path, L"\\SQLyog");
	}

	//Check Appdata/SQLyog pth is present, if not present it creating it
	if(!PathFileExists(path))
	{
		if(!CreateDirectory(path, NULL))
		{
			/* If the folder is there, then we will continue the process, otherwise we will return false */
			if(GetLastError() != ERROR_ALREADY_EXISTS)
			{
				return wyFalse;
			}
		}
	}
	return wyTrue;
	
}

/*Checks whether commandline argument as got explicit path for .ini, and other logfiles.
also filename whose contents as to be displayed in Sqlyog Editor
Otion can be any of one
i.   SQLyogEnt.exe -dir "E:\path" -f "D:\test.sql"
ii.  SQLyogEnt.exe -dir "E:\path" "D:\test.sql"
iii. SQLyogEnt.exe  -f "D:\test.sql" -dir "E:\path" -f
-dir stands for explicit path for .ini, tags, logs, favourites, themes etc
-f stands for file to be oponed in editor
*/
wyBool		
GetConfigDetails(wyChar *cmdline)
{
	wyString	filename, configstr, fname, dirname, sqlpath;
	wyInt32		cmdpos = 0, dirpos = 0, dirstartpos = 0, direndpos = 0, fpos = 0;
	
	if(cmdline || strlen(cmdline))
	{		
		filename.SetAs(cmdline);   // fetch the filename from commandline argument
		
		filename.RTrim();
		filename.LTrim();
			
		dirpos = filename.FindI("-dir", cmdpos);
		fpos = filename.FindI("-f");

		if(dirpos != -1)
		{
			dirstartpos = filename.FindI("\"", dirpos);
			direndpos = filename.FindI("\"", dirstartpos + 1);

			if(direndpos >0 && dirstartpos > 0 && direndpos > dirstartpos + 1)
			{
				configstr.SetAs(filename.Substr(dirstartpos + 1, direndpos - (dirstartpos + 1)));
				configstr.LTrim();
				configstr.RTrim();
				pGlobals->m_configdirpath.SetAs(configstr);	
					
				if(filename.FindI("-f", 0) != -1)
					FetchFileNameFromCmdline(filename.GetString());
				
				else if(direndpos + 1 < filename.GetLength())
				{
					fname.SetAs(filename.Substr(direndpos + 1, filename.GetLength() - (direndpos + 1)));

					fname.LTrim();
					fname.RTrim();
	                    
					FetchFileNameFromCmdline(fname.GetString());
				}
			}

			else if(fpos != -1)
			{
				if(fpos > dirpos)
				{					
					if((fpos - (dirpos + strlen("-dir"))) > (dirpos + strlen("-dir")))
					{
						dirname.SetAs(filename.Substr(dirpos + strlen("-dir"), fpos - (dirpos + strlen("-dir"))));

						dirname.LTrim();
						dirname.RTrim();

						pGlobals->m_configdirpath.SetAs(dirname);
					}
					
					if((filename.GetLength() - (direndpos + 1)) > fpos)
						fname.SetAs(filename.Substr(fpos, filename.GetLength() - (direndpos + 1)));

					FetchFileNameFromCmdline(fname.GetString());
				}

				else if(fpos < dirpos)
				{
                    if((filename.GetLength() - fpos) > (dirpos + strlen("-dir")))
					{
						dirname.SetAs(filename.Substr(dirpos + strlen("-dir"), filename.GetLength() - fpos));

						dirname.LTrim();
						dirname.RTrim();

						pGlobals->m_configdirpath.SetAs(dirname);
					}

					if((dirpos - (fpos + strlen("-f"))) > (fpos + strlen("-f")))
						fname.SetAs(filename.Substr(fpos + strlen("-f"), dirpos - (fpos + strlen("-f"))));

					FetchFileNameFromCmdline(fname.GetString());
				}				
			}
            
			else if(filename.FindI("-f") == -1)
			{
				if(filename.GetLength() - (dirpos + strlen("-dir")) > (dirpos + strlen("-dir")))

					dirname.SetAs(filename.Substr(dirpos + strlen("-dir"), filename.GetLength() - 
												 (dirpos + strlen("-dir"))));

				dirname.LTrim();
				dirname.RTrim();	

				pGlobals->m_configdirpath.SetAs(dirname);
			}

			else
			{
				DisplayErrorText(GetLastError(), _("Error with path used"));                
				return wyFalse;
			}
		} 		
			        				
		//Gets the filename to be oponed
		else
			FetchFileNameFromCmdline(filename.GetString());
	}
    
	return wyTrue;
}

wyBool
HandleTimeoutOption(HWND hwnd, ConnectionInfo *coninfo, wyUInt32 idcheck, wyUInt32 iduncheck)
{		
	EnableWindow(GetDlgItem(hwnd, idcheck), TRUE);
	EnableWindow(GetDlgItem(hwnd, iduncheck), TRUE);

	//Limit to 6 digits
	SendMessage(GetDlgItem(hwnd, IDC_TIMEOUTEDIT), EM_LIMITTEXT, 6, 0);
	SendMessage(GetDlgItem(hwnd, IDC_TIMEOUTEDIT1), EM_LIMITTEXT, 6, 0);

	SendMessage(GetDlgItem(hwnd, idcheck), BM_SETCHECK, BST_CHECKED, 0);

	if(idcheck == IDC_TIMEOUTOPT)
        EnableWindow(GetDlgItem(hwnd, IDC_TIMEOUTEDIT), TRUE);

	else if(idcheck == IDC_TIMEOUTOPT1)
        EnableWindow(GetDlgItem(hwnd, IDC_TIMEOUTEDIT1), TRUE);
	    
	if(SendMessage(GetDlgItem(hwnd, iduncheck), BM_GETCHECK, 0, 0)== BST_CHECKED)
		SendMessage(GetDlgItem(hwnd, iduncheck), BM_SETCHECK, BST_UNCHECKED, 0);	

	/*else
		return wyFalse;*/

	if(iduncheck == IDC_TIMEOUTDEF)
	{
		if(coninfo)
			coninfo->m_isdeftimeout = wyFalse;
		EnableWindow(GetDlgItem(hwnd, IDC_TIMEOUTEDIT), TRUE);
	}

	else if(iduncheck == IDC_TIMEOUTDEF1)
	{
		if(coninfo)
			coninfo->m_isdeftimeout = wyFalse;
		EnableWindow(GetDlgItem(hwnd, IDC_TIMEOUTEDIT1), TRUE);
	}

	if(iduncheck == IDC_TIMEOUTOPT)
	{
		EnableWindow(GetDlgItem(hwnd, IDC_TIMEOUTEDIT), FALSE);
		
		if(coninfo)
			coninfo->m_isdeftimeout = wyTrue;	
	}

	else if(iduncheck == IDC_TIMEOUTOPT1)
	{
		EnableWindow(GetDlgItem(hwnd, IDC_TIMEOUTEDIT1), FALSE);
		
		if(coninfo)
			coninfo->m_isdeftimeout = wyTrue;	
	}

	EnableWindow(GetDlgItem(hwnd, IDC_SAVE), TRUE);	

	return wyTrue;
}

//Formatting the queries in HTML format
void
FormatHtmlModeSQLStatement(wyString *query, wyBool &ismorewidth)
{
	wyChar		*newquery;
	wyInt32		counter = 0, len = 0, tempcount = 0;
	ismorewidth = wyFalse;
	
	if(!query)
		return;

	len = query->GetLength();
			
	VERIFY(newquery = _strdup(query->GetString()));

	query->SetAs("<br/>");	

	if(!newquery)
		return;

	for(counter; counter < len; counter++)
    {		
		switch(newquery[counter])
		{		
		case C_NEWLINE:
			tempcount = 0;
			query->Add("<br/>");
			break;			

		case C_SPACE:
			tempcount ++;
			query->Add("&nbsp;");
			break;
		
		case C_TAB: 
			tempcount += 4; 
			query->Add("&nbsp;&nbsp;&nbsp;&nbsp;");
			break;

		default:
			{
				tempcount++;
				
				//For adding over-flow property if more lengthy string
				if(tempcount >= 300)
				{
					tempcount = 0;
					if(ismorewidth == wyFalse)
						ismorewidth = wyTrue;
				}
				query->AddSprintf("%c", newquery[counter]);							
			}
		}
	}

	query->Add("<br/>");

	free(newquery);	
}

/**
Handle long sting in Html mode
Since html dll we used support only CSS2 the WORD-BREAK wont work, 
so break it manually
*/
VOID
FormatHtmlLongString(wyString *htmlstr)
{
	wyChar		*newquery;
	wyInt32		counter = 0, len = 0, tempcount = 0;
	
	if(!htmlstr)
		return;

	len = htmlstr->GetLength();

	VERIFY(newquery = _strdup(htmlstr->GetString()));

	if(!newquery)
		return;

	htmlstr->SetAs("");

	for(counter; counter < len; counter++)
    {		
		switch(newquery[counter])
		{		
		case C_SPACE:								
		case C_TAB:
			tempcount = 0;
			htmlstr->Add("<br>");
			break;

		case ',':
			tempcount = 0;
			htmlstr->Add(",<br>");
			break;
			
		default:
			{
				tempcount++;
				
				//just break after 50 chars
				if(tempcount >= 50)
				{
					tempcount = 0;
                    htmlstr->Add("<br>");					
				}
				htmlstr->AddSprintf("%c", newquery[counter]);							
			}
		}
	}

	free(newquery);		
}

//function initializes the choose color structure and opens up the choose color dialog.
wyBool
ChColor(HWND hwnd, LPCOLORREF rgbcolor)
{
	BOOL  ret;
	static COLORREF acrCustClr[16];

	CHOOSECOLOR  cc = {0};

	cc.lStructSize	= sizeof(cc);
	cc.hwndOwner	= hwnd;
	cc.lpCustColors =(LPDWORD)acrCustClr;
	cc.rgbResult	=(DWORD)*rgbcolor;
	cc.Flags		= CC_FULLOPEN | CC_RGBINIT;
 
	ret = ChooseColor(&cc);

	if(!ret){
		return wyFalse;
	}
	

	*rgbcolor = cc.rgbResult;  

	InvalidateRect(hwnd, NULL, TRUE);

	return wyTrue;
}

//Sets colors for combobox and add custom color option
wyBool
OnDrawColorCombo(HWND hwnd, LPDRAWITEMSTRUCT lpds, COLORREF rgbcolor)
{
	HDC hdc;
	RECT rc;
	HBRUSH   hbrush, hbrBackground;
	ConnectionBase *conbase  = NULL;
	COLORREF  cr             = COLOR_WHITE;
	unsigned long  tempcolor = -1;
	RECT temprect;
		
	hdc = lpds->hDC;
    rc = lpds->rcItem;

	hbrBackground = CreateSolidBrush(COLOR_WHITE);

	conbase = pGlobals->m_pcmainwin->m_connection;

	switch (lpds->itemAction)
	{
		case ODA_DRAWENTIRE:
			switch (lpds->CtlID)
			{
			case IDC_COLORCOMBO:
			case IDC_COLORCOMBO1:

				rc = lpds->rcItem;
				
				//fill the combobox with options
				if(lpds->itemID == 11)
					TextOut(hdc, rc.left, rc.top + 2, _(L" Other Colors..."), 16);
				else
				{
					if(lpds->itemID == -1)
					{
						//if connection color is there
						if(rgbcolor  >= 0)
						{
							//if color is -1, then set color as system color because
							//TreeView_SetBkColor will take -1 as system color
							if(rgbcolor == tempcolor)
								cr = GetSysColor(COLOR_WINDOW);
							else
								cr = rgbcolor;
						}
						else
							cr = COLOR_WHITE;
					}
					else
						cr = (COLORREF)lpds->itemData;

					FillRect(hdc, &rc, hbrBackground);
					hbrush = CreateSolidBrush((COLORREF)cr);
					FillRect(hdc, &rc, hbrush);
					DeleteObject(hbrush);
					FrameRect(hdc, &rc, (HBRUSH) GetStockObject(WHITE_BRUSH));
				}
				break;
				
			case IDC_COLORCOMBO2:
			//case IDC_COLORCOMBO3:
				//rect of color rectangle
				
				temprect.bottom = rc.bottom - 2;
				temprect.top = rc.top + 2;
				temprect.right = 25;
				temprect.left = rc.left + 2;
		
		
				if(lpds->itemID == 2)
				{
					TextOut(hdc, rc.left, rc.top + 2, _(L" Other Colors..."), 16);
					
				}
				else
				{
					if(lpds->itemID == -1)
					{
						//if connection color is there
						ExtTextOut(hdc, rc.left + 30, rc.top + 2, ETO_CLIPPED | ETO_OPAQUE, &lpds->rcItem, 
								_(L"Selected Color"), wcslen(_(L"Selected Color")), NULL);
						if(rgbcolor  >= 0)
						{
							//if color is -1, then set color as system color because
							//TreeView_SetBkColor will take -1 as system color
							if(rgbcolor == tempcolor)
								cr = GetSysColor(COLOR_WINDOW);
							else
								cr = rgbcolor;
						}
						else
							cr = COLOR_WHITE;
					
						hbrush = CreateSolidBrush((COLORREF)cr);
						FillRect(hdc, &temprect, hbrush);
						DeleteObject(hbrush);
						FrameRect(hdc, &temprect, (HBRUSH)GetStockObject(BLACK_BRUSH));
						FrameRect(hdc, &rc, (HBRUSH) GetStockObject(WHITE_BRUSH));
					}
					else
					{
						cr = (COLORREF)lpds->itemData;
						if(lpds->itemID == 0)
						{
							ExtTextOut(hdc, rc.left + 30, rc.top + 2, ETO_CLIPPED | ETO_OPAQUE, &lpds->rcItem, 
								_(L"Default"), wcslen(_(L"Default")), NULL);
							conbase->m_setcolor = wyTrue;
						}
						if(lpds->itemID == 1)
						{
							ExtTextOut(hdc, rc.left + 30, rc.top + 2, ETO_CLIPPED | ETO_OPAQUE, &lpds->rcItem, 
								_(L"Auto Contrast"), wcslen(_(L"Auto Contrast")), NULL);
							conbase->m_setcolor = wyTrue;
						}
						hbrush = CreateSolidBrush((COLORREF)cr);
						FillRect(hdc, &temprect, hbrush);
						DeleteObject(hbrush);
						FrameRect(hdc, &temprect, (HBRUSH)GetStockObject(BLACK_BRUSH));
						FrameRect(hdc, &rc, (HBRUSH) GetStockObject(WHITE_BRUSH));
					}
				}
				break;
			}
			break;
		
		case ODA_SELECT:
		{
			rc = lpds->rcItem;
			hdc = lpds->hDC;
			
			//highlight selected item with grey color
			if (lpds->itemState & ODS_SELECTED)
				hbrush = CreateSolidBrush(RGB(200,200,200));
			else
				hbrush = hbrBackground;
			
			FrameRect(hdc, &rc, hbrush);
			DeleteObject(hbrush);

			
		}
					
		case ODA_FOCUS:
			
			break;

		
	}
	DeleteObject(hbrBackground);

	return wyTrue;
}

//Command handler for color combo box
void
OnWmCommandColorCombo(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	wyInt32 count = 0;
	ConnectionBase *conbase = NULL;
	conbase = pGlobals->m_pcmainwin->m_connection;
	wyInt32 ctrl = LOWORD(wparam);
	COLORREF  tempcolor;
	
	if(ctrl == IDC_COLORCOMBO1 || ctrl == IDC_COLORCOMBO)
		tempcolor = conbase->m_rgbconnection;
	else
		tempcolor = conbase->m_rgbconnectionfg;
	
	if(HIWORD(wparam) == CBN_DROPDOWN)
	{
		if(conbase->m_setcolor == wyTrue)
		{
			SendMessage((HWND)lparam, CB_SETCURSEL, -1, 0);
			conbase->m_setcolor = wyFalse;
		}
	}
	else if(HIWORD(wparam)== CBN_SELCHANGE)
	{
		count = SendMessage((HWND)lparam, CB_GETCURSEL, 0, 0);
			
		if(count == 11||(ctrl == IDC_COLORCOMBO2&&count==2))
		{
			if(ChColor(hwnd, &tempcolor) == wyTrue)
			{
				if(ctrl == IDC_COLORCOMBO1 || ctrl == IDC_COLORCOMBO)
				{
					conbase->m_rgbconnection = tempcolor;
					ColorComboInitValues(GetDlgItem(hwnd,ctrl));
					ColorComboFgInitValues(GetDlgItem(hwnd,IDC_COLORCOMBO2),wyTrue, tempcolor);
				}
				else
				{
					conbase->m_rgbconnectionfg = tempcolor;
					ColorComboFgInitValues(GetDlgItem(hwnd,IDC_COLORCOMBO2), wyTrue, conbase->m_rgbconnection);

				}
				conbase->m_changeobcolor = wyTrue;
				conbase->m_setcolor = wyTrue;
				
			}
			else
			{
				conbase->m_changeobcolor = wyFalse;
				conbase->m_setcolor = wyTrue;
				ColorComboInitValues(GetDlgItem(hwnd,ctrl));
				ColorComboFgInitValues(GetDlgItem(hwnd,IDC_COLORCOMBO2),wyTrue, conbase->m_rgbconnection);
			}
		}
		else
		{
			if(ctrl == IDC_COLORCOMBO1 || ctrl == IDC_COLORCOMBO)
			{
				conbase->m_rgbconnection = SendMessage((HWND)lparam, CB_GETITEMDATA, count, 0);
				///*ColorComboInitValues(GetDlgItem(hwnd,ctrl));
				ColorComboFgInitValues(GetDlgItem(hwnd,IDC_COLORCOMBO2), wyTrue, conbase->m_rgbconnection);
			}
			else
			{
				conbase->m_rgbconnectionfg = SendMessage((HWND)lparam, CB_GETITEMDATA, count, 0);
				//ColorComboFgInitValues(GetDlgItem(hwnd,IDC_COLORCOMBO2), wyTrue, conbase->m_rgbconnection);
			}
			conbase->m_changeobcolor = wyTrue;
		}
	}

	return;
}

//fill color values for combo box
wyBool
ColorComboInitValues(HWND hwndcombo)
{
	wyInt32     color;
	wyInt32		conncolor[] = {COLOR_WHITE, COLOR_RED, COLOR_GREEN1, COLOR_BLUE2, 
								  COLOR_YELLOW, COLOR_BLUE, COLOR_GREEN,  
								  COLOR_PINK, COLOR_BLUE3, COLOR_GREEN2, COLOR_BROWN};

	SendMessage(hwndcombo, CB_RESETCONTENT, 0, 0);
	
	//for color combobox 
	for (color = 0; color <= (sizeof(conncolor)/sizeof(conncolor[0])); color++)
	{
		if(color == (sizeof(conncolor)/sizeof(conncolor[0])))
			SendMessage(hwndcombo, CB_INSERTSTRING, color, (LPARAM)_(" Other Colors..."));
		else
			SendMessage(hwndcombo, CB_INSERTSTRING, color, (LPARAM) conncolor[color]);
	}

	return wyTrue;
}

wyBool
ColorComboFgInitValues(HWND hwndcombo,wyBool isBk, COLORREF bkcolor)
{
	wyInt64     color;
	
	SendMessage(hwndcombo, CB_RESETCONTENT, 0, 0);
	if(!isBk)
		bkcolor = pGlobals->m_pcmainwin->m_connection->m_rgbconnection;
	wyInt64		conncolor[] = {RGB(0, 0, 0), bkcolor^0xFFFFFF};


	
	//for color combobox 
	for (color = 0; color < 2; color++)
		SendMessage(hwndcombo, CB_INSERTSTRING, color, (LPARAM) conncolor[color]);
	
	SendMessage(hwndcombo, CB_INSERTSTRING, color, (LPARAM)_(" Other Colors..."));

	return wyTrue;
}

//Keeping the CRC of query and No. of rows keeps persistance
wyInt32
HandleQueryPersistance(wyChar *query, wyInt32 *highlimit)
{
	wyUInt32		crcquery = 0;
	wyCrc			crc;
	wyInt32			pos = 0, numrows = 0;
	wyChar			*tempquery = NULL;
	wyString		lowercasequery;
		
	tempquery = FormatQuery(query, pos);
	
	lowercasequery.SetAs(tempquery);
			
	lowercasequery.RTrim();
	lowercasequery.LTrim();

	lowercasequery.ToLower();

	crcquery = crc.CrcFast((const unsigned char*)lowercasequery.GetString(), lowercasequery.GetLength());
		
	if(tempquery)
	{
		free(tempquery);
		tempquery = NULL;
	}

	numrows = HandleResulttabPagingQueryPersistance(crcquery, highlimit);

	return numrows;
}

//Set the font for a particular window
void
WindowFontSet(HWND hwnd, wyInt32 fontheight)
{
	HDC	        hdc;
    wyInt32     height;
    static HFONT  hfont = NULL;

	if(!hfont)
	{
	
	VERIFY(hdc = GetDC(hwnd));

	height = -MulDiv(fontheight, GetDeviceCaps(hdc, LOGPIXELSY), 72);

	hfont = CreateFont(height, 0, 0, 0,
		        FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, L"Verdana");
		
	ReleaseDC(hwnd, hdc);
	}

	SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, TRUE);	

    DeleteFont(hfont);

    return;	
}

//Sets the dialog cordinates according to dialog persistance save in ini
wyBool
SetInitPos(HWND hwnd, wyChar *dlgprfifix)
{
	RECT	    rc, dlgrect, rcedit, rcobj;
    wyWChar     *lpfileport=0;
	wyWChar     directory[MAX_PATH + 1] = {0};
	wyString	dirstr, section;
	wyInt32	    screenwidth, width = 0, screenheight, height = 0;
	wyInt32		virtx, virty, tempht = 0, tempwd = 0, twidth = 0, theight = 0;
	wyInt32		diffht, diffwd, left = 0, top = 0;
	wyInt32		xprim, yprim, cxfull, cyfull, remdinch, mmwidth, pixelsperinch;
	HDC         hdcdlg;
	wyInt32		numberofmonitors;
    MDIWindow   *wnd;
	
    if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyFalse)
	{
        return wyFalse;
	}
	
	if(!dlgprfifix)
	{
		return wyFalse;
	}

	screenwidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	screenheight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	cxfull = GetSystemMetrics(SM_CXFULLSCREEN);
	cyfull = GetSystemMetrics(SM_CYFULLSCREEN);

	dirstr.SetAs(directory);

	//get dialog's rect. By default the dialog will be in center of the screen
	GetWindowRect(hwnd, &dlgrect);

	//Gets screen usable area	
	RECT rctparam;
	SystemParametersInfo(SPI_GETWORKAREA,0,&rctparam,0);

	//Screen width in pixels
	hdcdlg = GetWindowDC(hwnd);	
	mmwidth = GetDeviceCaps(hdcdlg, HORZSIZE);
	ReleaseDC(hwnd, hdcdlg);
		
	//Pixels / inch
	pixelsperinch = screenwidth / mmwidth;

	//20 inches
	remdinch = pixelsperinch * 20;	

	left = rctparam.left + remdinch / 2;
	top = rctparam.top + remdinch / 2;

	//SchemaSync
	if(!strcmp(dlgprfifix, SCHEMASYNC_SECTION))
	{
		section.SetAs(SCHEMASYNC_SECTION);
		
		twidth = rctparam.right -  left - remdinch / 2;
		theight = rctparam.bottom - top - remdinch / 2;

		tempwd = cxfull - twidth;
        tempht = cyfull - theight;

	}
	//For Create/Alter table
	else if(!strcmp(dlgprfifix, TABLEMAKER_SECTION))
	{
		VERIFY(wnd = GetActiveWin());
		if(!wnd)
		{
			return wyFalse;
		}
		
		section.SetAs(TABLEMAKER_SECTION);

		VERIFY(GetWindowRect(wnd->GetTabModule()->GetHwnd(), &rcedit));
		VERIFY  (GetWindowRect (wnd->GetQueryObject()->m_hwnd, &rcobj ) );

		left = rcobj.left + 5;
		top = rcobj.top + 5;
		twidth = rcedit.right - rcobj.left - 15;
		theight = rcobj.bottom - rcobj.top - 15;

		tempwd =  twidth;
        tempht =  theight;
	}

	//Manage Relationship dialog
	else if(!strcmp(dlgprfifix, MANRELATION_SECTION) || !strcmp(dlgprfifix, INDEXMANAGER_SECTION)
		|| !strcmp(dlgprfifix, HANDLERELATION_SECTION) || !strcmp(dlgprfifix,  HANDLEINDEX_SECTION)
		|| !strcmp(dlgprfifix, BLOBVIEWER_SECTION) || !strcmp(dlgprfifix, USERMANAGER_SECTION)
		|| !strcmp(dlgprfifix, SHOWSUMMARY_SECTION)|| !strcmp(dlgprfifix, SHOWPREVIEW_SECTION)
		|| !strcmp(dlgprfifix, KEYSHORTCUT_SECTION)|| !strcmp(dlgprfifix, COLUMNMAP_SECTION) 
		|| !strcmp(dlgprfifix, SHOWVALUE_PROCESSLIST_SECTION)
		|| !strcmp(dlgprfifix, SHOWVALUE_STATUS_SECTION)
		|| !strcmp(dlgprfifix, SHOWVALUE_VARIABLE_SECTION)
		|| !strcmp(dlgprfifix, SHOWVALUE_WARNING_SECTION))
	{
		left = rctparam.left;
		top = rctparam.top;
		twidth = 600;
		theight = 400;

		if(!strcmp(dlgprfifix, MANRELATION_SECTION))
		{
			section.SetAs(MANRELATION_SECTION);
		}
		else if(!strcmp(dlgprfifix, INDEXMANAGER_SECTION))
		{
			section.SetAs(INDEXMANAGER_SECTION);
		}
		else if(!strcmp(dlgprfifix, USERMANAGER_SECTION))
		{
			section.SetAs(USERMANAGER_SECTION);
		}
		else if(!strcmp(dlgprfifix, HANDLERELATION_SECTION))
		{
			twidth = 438;
			theight = 440;
			section.SetAs(HANDLERELATION_SECTION);
		}
		else if(!strcmp(dlgprfifix, HANDLEINDEX_SECTION))
		{
			twidth = 315;
			theight = 273;
			section.SetAs(HANDLEINDEX_SECTION);
		}
		else if(!strcmp(dlgprfifix, BLOBVIEWER_SECTION))
		{
			twidth = 350;
			theight = 520;
			section.SetAs(BLOBVIEWER_SECTION);
		}
		else if(!strcmp(dlgprfifix, SHOWVALUE_PROCESSLIST_SECTION))
		{
			twidth = 350;
			theight = 350;
			section.SetAs(SHOWVALUE_PROCESSLIST_SECTION);
		}
		else if(!strcmp(dlgprfifix, SHOWVALUE_STATUS_SECTION))
		{
			twidth = 350;
			theight = 350;
			section.SetAs(SHOWVALUE_STATUS_SECTION);
		}
		else if(!strcmp(dlgprfifix, SHOWVALUE_VARIABLE_SECTION))
		{
			twidth = 350;
			theight = 350;
			section.SetAs(SHOWVALUE_VARIABLE_SECTION);
		}
		else if(!strcmp(dlgprfifix, SHOWVALUE_WARNING_SECTION))
		{
			twidth = 350;
			theight = 350;
			section.SetAs(SHOWVALUE_WARNING_SECTION);
		}
		else if(!strcmp(dlgprfifix, SHOWSUMMARY_SECTION))
		{
			twidth = 350;
			theight = 350;
			section.SetAs(SHOWSUMMARY_SECTION);
		}
		else if(!strcmp(dlgprfifix, SHOWPREVIEW_SECTION))
		{
			twidth = 350;
			theight = 350;
			section.SetAs(SHOWPREVIEW_SECTION);
		}
		else if(!strcmp(dlgprfifix, KEYSHORTCUT_SECTION))
		{
			twidth = 500;
			theight = 350;
			section.SetAs(KEYSHORTCUT_SECTION);
		}
		else if(!strcmp(dlgprfifix, COLUMNMAP_SECTION))
		{
			section.SetAs(COLUMNMAP_SECTION);
		}

		tempwd = cxfull - twidth;
        tempht = cyfull - theight;
	}
	else
	{
		return wyFalse;
	}
		
	dirstr.SetAs(directory);

	numberofmonitors = GetSystemMetrics(SM_CMONITORS);

	rc.left = wyIni::IniGetInt(section.GetString(), "Left", left, dirstr.GetString());
	if(numberofmonitors == 1)
	{
		if(rc.left < 0)
		{
			rc.left = 0;
		}
	}
	
	rc.top	= wyIni::IniGetInt(section.GetString(), "Top", top, dirstr.GetString());

	if(numberofmonitors == 1)
	{
		if(rc.top < 0)
		{
			rc.top = 0;
		}
	}
	
	rc.right = wyIni::IniGetInt(section.GetString(), "Right", width - rc.left, dirstr.GetString());
		
	rc.bottom = wyIni::IniGetInt(section.GetString(), "Bottom", height - rc.top, dirstr.GetString());
	
	width = rc.right - rc.left;
	height = rc.bottom - rc.top;
	
	virtx = GetSystemMetrics(SM_XVIRTUALSCREEN);
	virty = GetSystemMetrics(SM_YVIRTUALSCREEN);
		
	xprim = GetSystemMetrics(SM_CXSCREEN);
	yprim = GetSystemMetrics(SM_CYSCREEN);		

	diffht = screenheight + virty;
    diffwd = screenwidth + virtx; 

	//Make the dialog in default pos if width/heght is zero, or grater than screen size
	//check whether the 2nd monitor is on corner or not before closing the dialog
	if(screenheight >= yprim && virtx >=0 && rc.left <= (virtx))
	{
		if(rc.top <= 0)
		{
			if(!top)
			{
				rc.top = tempht / 2;
			}
			else
			{
				rc.top = top;
			}

			if(!left)
			{
				rc.left = tempwd / 2;
			}
			else
			{
				rc.left = left;
			}

			rc.right =  twidth + rc.left;
			rc.bottom = theight + rc.top;

			/*rc.left = 0;
			rc.right = rc.left + width;
			rc.top = 0;
			rc.bottom = rc.top + height;*/
		}
		else if(rc.top >= yprim) 
		{
			rc.left = 0;
			rc.right = rc.left + width;
			rc.top = yprim - height;
			rc.bottom = rc.top + height;
		}
	}
	else if(screenheight >= yprim && virtx <= 0 && rc.left >= (xprim))
	{
		if(rc.top <= 0)
		{
			rc.left = xprim - width;
			rc.right = rc.left + width;
			rc.top = 0;
			rc.bottom = rc.top + height;
		}
		else if(rc.top >= yprim)
		{
			rc.left = xprim - width;
			rc.right = rc.left + width;
			rc.top = yprim - height;
			rc.bottom = rc.top + height;
		}
	}
	
	//if dialog is on right side of screen
	if(rc.left >= screenwidth) 
	{
		rc.left =  screenwidth - width;
		rc.right = rc.left + width;
	}
	else if(rc.left >= diffwd) 
	{
		// If dialog was in 2nd monitor and moved 2nd monitor from right to left and then open dialog 
        rc.left =  diffwd - width;
		rc.right = rc.left + width;
	}
	
	if((rc.right <= virtx) && (rc.bottom <= virty))
	{
		if(!left)
		{
			rc.left = tempwd / 2;
		}
		else
		{
			rc.left = left;
		}

		rc.right =  twidth + rc.left;

		if(!top)
		{
			rc.top = tempht / 2;
		}
		else
		{
			rc.top = top;
		}

		rc.bottom = theight + rc.top;
	}

	//if dialog is on left side of screen 
	if(rc.right <= virtx)
	{
		rc.left = 0;
		rc.right = width;
	}

	//if dialog is at top of screen 
	if(rc.bottom <= virty)
	{
		rc.top = 0;
		rc.bottom = height;
	}
		
	//if dialog is at bottom of screen 
	if(rc.top >= screenheight)
	{
		rc.top = screenheight - height;
		rc.bottom = rc.top + height;		
	}
	else if(rc.top >= diffht)
	{
		// If dialog was in 2nd monitor and user moved the 2nd monitor
		//from bottom to top and then open dialog 
		rc.top = diffht - height;
		rc.bottom = rc.top + height;
	}
	
	MoveWindow(hwnd, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, FALSE);

	return wyTrue;
}

//Keeps the dialog cordinates in .ini for dialog persistance
wyBool
SaveInitPos(HWND hwnd, wyChar *dlgprfifix)
{
	RECT         rc;
    wyWChar      *lpfileport=0;
    wyString     value, dirstr;
	wyWChar      directory[MAX_PATH + 1] = {0};

    if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyFalse)
        return wyFalse;
	
	VERIFY(GetWindowRect(hwnd, &rc));
	
	dirstr.SetAs(directory);

	// write the values.
	value.Sprintf("%d", rc.left);
	wyIni::IniWriteString(dlgprfifix, "Left", value.GetString(), dirstr.GetString());

	value.Sprintf("%d", rc.top);
	wyIni::IniWriteString(dlgprfifix, "Top", value.GetString(), dirstr.GetString());
	
	value.Sprintf("%d", rc.right);
	wyIni::IniWriteString(dlgprfifix, "Right", value.GetString(), dirstr.GetString());
	
	value.Sprintf("%d", rc.bottom);
	wyIni::IniWriteString(dlgprfifix, "Bottom", value.GetString(), dirstr.GetString());


	return wyTrue;

}

//Checks to see whether the given field(window) is blank or not
wyInt32
IsFieldBlank(HWND hwnd)
{
    wyInt32         count;
    wyWChar*        temp;
    wyString        alias;
    const wyChar*   ptr;
    
    count = (wyInt32)GetWindowTextLength(hwnd);
    
    if(count == 0)
        return -1;

    temp = new wyWChar[count + 1];
    GetWindowText(hwnd, temp, count + 1);
    alias.SetAs(temp);
    delete temp;
    ptr = alias.GetString();

    while(*ptr)
    {
        if(*ptr != ' ' && *ptr != '\t')
            return 1;
        ++ptr;
    }

    return 0;
}

/**
GEts the file type by checking the extension
*/
SQLyogFileType
OpenFileType(wyString *filename)
{	
	wyString extn;
#ifndef COMMUNITY
	wyWChar *ext;
#endif
	if(!filename)
		return FILEINVALID;

	//For Commubnity & PRO, there's no SD & QB

#ifdef COMMUNITY 
	return SQLFILE;
#else
	if(!pGlobals->m_entlicense.CompareI("Professional"))
	{
		return SQLFILE;
	}

	//Find the file extension
	ext = PathFindExtension(filename->GetAsWideChar());

	if(!ext)
		return SQLFILE;

	extn.SetAs(ext);
		
	if(!extn.CompareI(".queryxml"))
		return QBFILE;

	else if(!extn.CompareI(".schemaxml"))
		return SDFILE;

	else if(!extn.CompareI(""))
		{
		filename->Add(".sql");
		return SQLFILE;
	}
	else
		return SQLFILE;
#endif
}

VOID
GetsTableEngineName(MDIWindow *wnd, const wyChar *dbname, const wyChar *tablename, wyString *engine)
{
	wyString query;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row = NULL;

	if(!IsMySQL5010(wnd->m_tunnel, &wnd->m_mysql))
		return;
	
	query.Sprintf("select `ENGINE` from `INFORMATION_SCHEMA`.`TABLES` where `TABLE_SCHEMA` = '%s' and `TABLE_NAME` = '%s' and `TABLE_TYPE` = 'BASE TABLE'", dbname, tablename);

	res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);
	
	if(!res)
    {
		ShowMySQLError(wnd->m_hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return;
    }

	row = wnd->m_tunnel->mysql_fetch_row(res);

	if(!row || !row[0])
	{
		wnd->m_tunnel->mysql_free_result(res);
		return;
	}

	engine->SetAs(row[0], wnd->m_ismysql41);

	wnd->m_tunnel->mysql_free_result(res);
}

//Function performs find next on F3
void OnAccelFindNext(MDIWindow* wnd)
{
	EditorBase          *peditorbase = NULL;
    HWND                hwndfocus = NULL, hwnd = NULL;
	TabMgmt             *tabmgmt = NULL;
	TabEditor           *ptabeditor = NULL;
	TabHistory			*ptabhistory = NULL;
	TabObject			*ptabobject = NULL;
    TabTableData        *ptabtabledata = NULL;
    DataView*           pdataview;
#ifndef COMMUNITY
    TabDbSearch         *ptabdbsearch = NULL;
#endif
    FindAndReplace**    pfindrep = NULL;
    wyInt32             tabimage = wnd->m_pctabmodule->GetActiveTabImage();

    ptabeditor = wnd->GetActiveTabEditor();     

	if(ptabeditor)
    {
		peditorbase = ptabeditor->m_peditorbase;
	}
    else if(tabimage == IDI_TABLE)
    {
        ptabtabledata = (TabTableData*)wnd->m_pctabmodule->GetActiveTabType();
    }
#ifndef COMMUNITY
    else if(tabimage == IDI_DATASEARCH)
    {
        ptabdbsearch = (TabDbSearch*)wnd->m_pctabmodule->GetActiveTabType();
    }
#endif
    else
	{
		ptabhistory = wnd->GetActiveHistoryTab();
		ptabobject = wnd->GetActiveInfoTab();
	}


	if(!ptabeditor && !ptabhistory && !ptabobject && !ptabtabledata)
    {	
#ifndef COMMUNITY
        if(!ptabdbsearch)
            return;
#else
        return;
#endif
    }

	      	
    hwndfocus = GetFocus();

    //get the FindReplace pointer for the supported windows
    if(peditorbase && peditorbase->m_hwnd && (hwndfocus == peditorbase->m_hwnd))
    {
        hwnd = peditorbase->m_hwnd;
        pfindrep = &peditorbase->m_findreplace;
    }
    else if(ptabobject && ptabobject->m_pobjinfo && (hwndfocus == ptabobject->m_pobjinfo->m_hwnd))
    {
		hwnd = hwndfocus;
        pfindrep = &ptabobject->m_pobjinfo->m_findreplace;
	}
	else if(ptabhistory && ptabhistory->m_hwnd && (hwndfocus == ptabhistory->m_hwnd))
    {
		hwnd = ptabhistory->m_hwnd;
        pfindrep = &ptabhistory->m_findreplace;
	}
    else if(ptabtabledata && ptabtabledata->m_tabledata->m_viewtype == TEXT 
        && hwndfocus == (ptabtabledata->m_tableview->GetActiveDispWindow()))
    {
        hwnd = hwndfocus;
        pfindrep = &ptabtabledata->m_tableview->m_findreplace;
    }
#ifndef COMMUNITY
    else if(ptabdbsearch && ptabdbsearch->m_pdataview->GetData()->m_viewtype == TEXT
        && hwndfocus == (ptabdbsearch->m_pdataview->GetActiveDispWindow()))
	{
        pfindrep = &ptabdbsearch->m_pdataview->m_findreplace;
    }
#endif
    else
    {
        if(!ptabeditor || !(tabmgmt = ptabeditor->m_pctabmgmt))
        {
			return;
        }

        if((pdataview = tabmgmt->GetDataView()) && pdataview->GetData() && 
            pdataview->GetData()->m_viewtype == TEXT && hwndfocus == pdataview->GetActiveDispWindow())
        {
            pfindrep = &pdataview->m_findreplace;
            hwnd = hwndfocus;
        }
        else if(tabmgmt->GetActiveTabIcon() == IDI_HISTORY && tabmgmt->m_phistory)
        {
            hwnd = tabmgmt->m_phistory->m_hwndedit;
            pfindrep = &tabmgmt->m_phistory->m_findreplace;
        }
        else if(tabmgmt->GetActiveTabIcon() == IDI_TABLEINDEX && tabmgmt->m_pqueryobj && 
            hwndfocus == tabmgmt->m_pqueryobj->m_pobjectinfo->m_hwnd)
        {
            hwnd = hwndfocus;
            pfindrep = &tabmgmt->m_pqueryobj->m_pobjectinfo->m_findreplace;
        }
    }
    
    //if a search string is already present and window handle is valid then dont open the find next dialog
    if(pGlobals->m_pcmainwin->m_findtext.GetLength() && hwnd)
    {
        if(!pGlobals->m_pcmainwin->m_finddlg)
        {
            pGlobals->m_findreplace = wyTrue;
        }
        else
        {
            hwnd = pGlobals->m_pcmainwin->m_frstruct.hwndOwner;
        }

        pGlobals->m_pcmainwin->m_frstruct.hwndOwner = hwnd;
        pGlobals->m_pcmainwin->m_frstruct.lpstrFindWhat = pGlobals->m_pcmainwin->m_findtext.GetAsWideChar();

        //if no FindReplace is allocated, then allocate it
        if(*pfindrep == NULL)
            *pfindrep = new FindAndReplace(hwnd);
       
        //send find next message to the window identified by hwnd
        SendMessage(hwnd, pGlobals->m_pcmainwin->m_findmsg, (WPARAM)0, 
                    (LPARAM)&pGlobals->m_pcmainwin->m_frstruct);
    }
    else
    {
        FindTextOrReplace(wnd, wyFalse);
    }
}//Handling Drag & Frop files 
void
HandleOnDragAndDropFiles(HDROP phandle, MDIWindow *wnd)
{	
	wyInt32		filecount = 0;
	wyInt32		i = 0;
	wyWChar		fname[MAX_PATH];
	wyString	filename,finalerrormessage;
	
	filecount = DragQueryFile(phandle, 0xFFFFFFFF, NULL, 0);
	finalerrormessage.SetAs("");
	for(i = 0; i < filecount; i++)
	{
		DragQueryFile(phandle, i, fname, MAX_PATH);
		
		filename.SetAs(fname);		
		wnd->OpenFileinTab(&filename,&finalerrormessage);		
	}	

	if(finalerrormessage.GetLength())
		yog_message(NULL, finalerrormessage.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK | MB_TASKMODAL);
	//Release the memory
	DragFinish(phandle);
}

void
SeekCurrentRowFromMySQLResult(MYSQL_RES *res, MYSQL_ROWS **rowswalker, Tunnel *tunnel, MYSQL_ROW *currentrow, wyULong **rowlength)
{	
	wyInt32			fldcount, i;
	wyULong			*rowlen = NULL, *templen = NULL;
    
	*currentrow = NULL;
	fldcount = res->field_count;

	if(!(*rowswalker))
		*rowswalker = res->data_cursor;
	else
		*rowswalker = (*rowswalker)->next;

	if(!(*rowswalker))
	{
		return;
	}			
	
	res->current_row = (*rowswalker)->data;
	*currentrow = res->current_row;					
	
	if(!(*currentrow))
	{
		return;			
	}
	
	if(rowlength)
	{
		rowlen = tunnel->mysql_fetch_lengths(res);
		
		templen = (wyULong*)malloc(fldcount * sizeof(wyULong));
					
		for(i = 0; i < fldcount; i++)
		{
			templen[i] = rowlen[i];
		}

		*rowlength = templen;
	}

    return;
}

//Sets connection name with connection color in combobox
wyBool
OnDrawConnNameCombo(HWND hwndcombo, LPDRAWITEMSTRUCT lpds,  wyBool isconndbname)
{
	wyInt32		ncount, i, j;
	wyString	connstr, dirstr, conncount;
	wyInt32		topindex;
	/*wyWChar     directory[MAX_PATH + 1], *lpfileport=0;
    
	wyInt32 ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	if(ret == 0)
		return wyFalse;
	dirstr.SetAs(directory);
	*/
	//fill the combobox with options
	ncount = SendMessage(hwndcombo, CB_GETCOUNT, 0, 0);
	switch (lpds->itemAction)
		{
			case ODA_DRAWENTIRE:
				{
					if(ncount > CBMINVISIBLEITEM)
					{
						topindex = SendMessage(hwndcombo, CB_GETTOPINDEX, 0, 0);
						j = topindex + CBMINVISIBLEITEM;
					}
					else
					{
						topindex = 0;
						j = ncount;
					}
				//	pGlobals->m_pcmainwin->m_connection->PopulateColorArray(hwndcombo, &dirstr);
					for(i = topindex; i < j; i++)
					{
						if(isconndbname == wyTrue)
						{
							OnDrawComboItem(hwndcombo, lpds, NULL, wyTrue);
						}
						else
						{
						OnDrawComboItem(hwndcombo, lpds, pGlobals->m_pcmainwin->m_connection->m_arrayofcolor[lpds->itemID]);
					}
				}
				}
				break;
			
			case ODA_SELECT:
				if(isconndbname == wyTrue)
				{
					OnDrawComboItem(hwndcombo, lpds, NULL, wyTrue);
				}
				else
				{
					if(lpds->itemID != (UINT)-1)
					OnDrawComboItem(hwndcombo, lpds, pGlobals->m_pcmainwin->m_connection->m_arrayofcolor[lpds->itemID]);
				}
				break;

			case ODA_FOCUS:
				DrawFocusRect(lpds->hDC, &lpds->rcItem);
				break;
		}

	return wyTrue;
}

//Sets connection name with connection color in combobox
wyBool
OnDrawComboItem(HWND hwndcombo, LPDRAWITEMSTRUCT lpds, COLORREF cr, wyBool isconndbname)
{
	HDC			hdc;
	RECT		rc, temprect;
	HBRUSH		hbrush;
	wyInt32		length;
	wyString	 connstr;
	wyWChar		*connname = NULL;
	COLORREF	clrBackground; 
    COLORREF	clrForeground; 
	wyString    defaultstr, refreshstr, seldb;
	wyBool      nocolorrect = wyFalse;
	LPDIFFCOMBOITEM	pdiffcombo = NULL;
    static wyWChar   *defonn = DEFAULT_CONN;
    static wyWChar   *defdb = SELECTDB, *defref = REFRESHLIST;
	
	defaultstr.SetAs(defonn);
	refreshstr.SetAs(defref);
	seldb.SetAs(defdb);


	defaultstr.SetAs(defonn);

	hdc = lpds->hDC;
	rc = lpds->rcItem;

	if(isconndbname == wyTrue)
	{
		VERIFY(pdiffcombo =(LPDIFFCOMBOITEM)SendMessage(hwndcombo, CB_GETITEMDATA, lpds->itemID, 0));
	}

	//rect of color rectangle
	temprect.bottom = rc.bottom - 2;
	temprect.top = rc.top + 2;
	temprect.right = 25;
	temprect.left = rc.left + 2;

	length  = SendMessage(hwndcombo, CB_GETLBTEXTLEN, lpds->itemID, NULL);
						
    if(length > 0)
    {
	connname    = AllocateBuffWChar(length + 1);

	//Get connection name
	SendMessage(hwndcombo, CB_GETLBTEXT,(WPARAM)lpds->itemID, (LPARAM)connname);

	connstr.SetAs(connname);

	//if default connection string then no need to draw color rect
	if((connstr.Compare(defaultstr) == 0) || (connstr.Compare(refreshstr) == 0) || (connstr.Compare(seldb) == 0) )
	{
		nocolorrect = wyTrue;
	}
	else
	{
		nocolorrect = wyFalse;
	}

	free(connname);
    }
	// The colors depend on whether the item is selected. 
	if(lpds->itemState & ODS_SELECTED)
	{
		clrForeground = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
		clrBackground = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
	}
	else
	{
		clrForeground = SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
		clrBackground = SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
	}
	
	//write connection name
	if(nocolorrect == wyFalse)
	{
		ExtTextOut(hdc, rc.left + 30, rc.top + 2, ETO_CLIPPED | ETO_OPAQUE, &lpds->rcItem, 
							connstr.GetAsWideChar(), length, NULL); 
	}
	else
	{
		ExtTextOut(hdc, rc.left + 2, rc.top + 2, ETO_CLIPPED | ETO_OPAQUE, &lpds->rcItem, 
							connstr.GetAsWideChar(), length, NULL); 
	}
					           
	// Restore the previous colors. 
	SetTextColor(hdc, clrForeground); 
	SetBkColor(hdc, clrBackground); 
	
	//draw color rect for saved connection name only
	if(nocolorrect == wyFalse)
	{
		//fill the item with connection color
		if(isconndbname == wyTrue)
		{
			hbrush = CreateSolidBrush((COLORREF)pdiffcombo->info->m_rgbconn);
		}
		else
		{
		hbrush = CreateSolidBrush((COLORREF)cr);
		}
		FillRect(hdc, &temprect, hbrush);
		DeleteObject(hbrush);

		FrameRect(hdc, &temprect, (HBRUSH)GetStockObject(BLACK_BRUSH));
	}

	FrameRect(hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));

	return wyTrue;
}

//Function to get measurestruct of custom draw combobox
wyBool
OnComboMeasureItem(LPMEASUREITEMSTRUCT lpmis)
{
	//lpmis->itemWidth = 10;
	//lpmis->itemHeight = 16;

	return wyTrue;
}

//Function handles scintilla notification messages like paranthesis matching
wyInt32
OnScintillaNotification(WPARAM wparam, LPARAM lparam, wyBool isquerytab)
{
    LPNMHDR         lpnmhdr = (LPNMHDR)lparam;
    EditorBase*     peditorbase = (EditorBase*)GetWindowLongPtr(lpnmhdr->hwndFrom, GWLP_USERDATA);
    wyInt32         p1, p2, style = 0;
    wyChar          c = 0;
    SCNotification* pscn;
    wyInt32         temp;
    wyBool          isposvariableset = wyFalse;

    switch(lpnmhdr->code)
    {
        //for paranthesis highlighting
        case SCN_UPDATEUI:
            pscn = (SCNotification*)lparam;
            SendMessage(lpnmhdr->hwndFrom, SCI_BRACEBADLIGHT, (WPARAM)-1, 0);
            
            p1 = SendMessage(lpnmhdr->hwndFrom, SCI_GETCURRENTPOS, 0, 0);

            if(pscn->updated & SC_UPDATE_CONTENT)
            {
                style = SendMessage(lpnmhdr->hwndFrom, SCI_GETSTYLEAT, (WPARAM)p1 - 1, 0);
                c = SendMessage(lpnmhdr->hwndFrom, SCI_GETCHARAT, (WPARAM)p1 - 1, 0);

                if(style == SCE_MYSQL_OPERATOR && (c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']'))
                {
                    isposvariableset = wyTrue;
                    p1--;
                }
            }
            
            if(isposvariableset == wyFalse)
            {
                style = SendMessage(lpnmhdr->hwndFrom, SCI_GETSTYLEAT, (WPARAM)p1, 0);
                c = SendMessage(lpnmhdr->hwndFrom, SCI_GETCHARAT, (WPARAM)p1, 0);
            }

            if(style == SCE_MYSQL_OPERATOR && (c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']'))
            {
                p2 = SendMessage(lpnmhdr->hwndFrom, SCI_BRACEMATCH, (WPARAM)p1, 0);            

                if(p2 != -1)
                {
                    SendMessage(lpnmhdr->hwndFrom, SCI_BRACEHIGHLIGHT, (WPARAM)p1, (LPARAM)p2);
                }
                else
                {
                    SendMessage(lpnmhdr->hwndFrom, SCI_BRACEBADLIGHT, (WPARAM)p1, 0);
                }
            }
            break;

        //for dirty mark handling
        case SCN_SAVEPOINTREACHED:
        case SCN_SAVEPOINTLEFT:
            if(isquerytab == wyFalse)
            {
                return 0;
            }

            if(peditorbase && peditorbase->m_isdiscardchange == wyFalse)
            {
                peditorbase->m_edit = (lpnmhdr->code == SCN_SAVEPOINTREACHED) ? wyFalse : wyTrue;
                GetActiveWin()->SetQueryWindowTitle();
            }
            
            break;

        //fold margin handling
        case SCN_MARGINCLICK:
            if(isquerytab == wyFalse)
            {
                return 0;
            }

            pscn = (SCNotification*)lparam;
            p1 = SendMessage(lpnmhdr->hwndFrom, SCI_LINEFROMPOSITION, (WPARAM)pscn->position, 0);
            p2 = SendMessage(lpnmhdr->hwndFrom, SCI_GETCURRENTPOS, 0, 0);
            p2 = SendMessage(lpnmhdr->hwndFrom, SCI_LINEFROMPOSITION, (WPARAM)p2, 0);

            if(p1 != -1 && (SendMessage(lpnmhdr->hwndFrom, SCI_GETFOLDLEVEL, (WPARAM)p1, 0) & SC_FOLDLEVELHEADERFLAG))
            {
                SendMessage(lpnmhdr->hwndFrom, SCI_TOGGLEFOLD, (WPARAM)p1, 0);

                if(!SendMessage(lpnmhdr->hwndFrom, SCI_GETLINEVISIBLE, (WPARAM)p2, 0))
                {
                    SendMessage(lpnmhdr->hwndFrom, SCI_GOTOLINE, (WPARAM)p1, 0);
                }
            }

            break;

        //buffer modification handling
        case SCN_MODIFIED:
            if(isquerytab == wyFalse)
            {
                return 0;
            }
			SetEvent(pGlobals->m_pcmainwin->m_sessionchangeevent);
            pscn = (SCNotification*)lparam;

            if(pscn->modificationType & SC_MOD_BEFOREINSERT || pscn->modificationType & SC_MOD_BEFOREDELETE)
            {
                p1 = SendMessage(lpnmhdr->hwndFrom, SCI_GETCURRENTPOS, 0, 0);
                p1 = SendMessage(lpnmhdr->hwndFrom, SCI_LINEFROMPOSITION, (WPARAM)p1, 0);
                
                if(!SendMessage(lpnmhdr->hwndFrom, SCI_GETFOLDEXPANDED, (WPARAM)p1, 0))
                {
                    SendMessage(lpnmhdr->hwndFrom, SCI_TOGGLEFOLD, (WPARAM)p1, 0);
                }
            }
            else if(pscn->modificationType & SC_MOD_INSERTTEXT && peditorbase->m_acwithquotedidentifier)
            {
                temp = peditorbase->m_acwithquotedidentifier;
                peditorbase->m_acwithquotedidentifier = 0;
                PostMessage(lpnmhdr->hwndFrom, UM_ADDBACKQUOTEONAC, (WPARAM)pscn->position, (LPARAM)temp);
            }

        default:
            return 0;
    }

    return 1;
}

void
SetParanthesisHighlighting(HWND hwnd)
{
    COLORREF forecolor = RGB(0,0,0), backcolor = COLOR_WHITE;
    wyWChar		*lpfileport = 0;
	wyWChar		directory[MAX_PATH+1] = {0};

    if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyTrue)
    {
        //Chnage line number margin color
        wyString	dirstr(directory);        
        forecolor   =   wyIni::IniGetInt(GENERALPREFA, "BraceBadFgColor",   DEF_BRACEBADFG, dirstr.GetString());
        backcolor   =   wyIni::IniGetInt(GENERALPREFA, "BraceBadBgColor",   DEF_BRACEBADBG, dirstr.GetString());
        SendMessage(hwnd, SCI_STYLESETBACK, (WPARAM)STYLE_BRACEBAD, (LPARAM)backcolor);
        SendMessage(hwnd, SCI_STYLESETFORE, (WPARAM)STYLE_BRACEBAD, (LPARAM)forecolor);
        forecolor   =   wyIni::IniGetInt(GENERALPREFA, "BraceLightFgColor",   DEF_BRACELIGHTFG, dirstr.GetString());
        backcolor   =   wyIni::IniGetInt(GENERALPREFA, "BraceLightBgColor",   DEF_BRACELIGHTBG, dirstr.GetString());
        SendMessage(hwnd, SCI_STYLESETFORE, (WPARAM)STYLE_BRACELIGHT, (LPARAM)forecolor);
        SendMessage(hwnd, SCI_STYLESETBACK, (WPARAM)STYLE_BRACELIGHT, (LPARAM)backcolor);
    }
    else
    {
        SendMessage(hwnd, SCI_STYLESETBOLD, (WPARAM)STYLE_BRACELIGHT, (LPARAM)true);
        SendMessage(hwnd, SCI_STYLESETFORE, (WPARAM)STYLE_BRACELIGHT, (LPARAM)DEF_BRACELIGHTFG);
        SendMessage(hwnd, SCI_STYLESETBACK, (WPARAM)STYLE_BRACELIGHT, (LPARAM)DEF_BRACELIGHTBG);
        SendMessage(hwnd, SCI_STYLESETBACK, (WPARAM)STYLE_BRACEBAD, (LPARAM)DEF_BRACEBADBG);
        SendMessage(hwnd, SCI_STYLESETFORE, (WPARAM)STYLE_BRACEBAD, (LPARAM)DEF_BRACEBADFG);
    }
}

void
EnableFolding(HWND hwnd)
{
	COLORREF forecolor = RGB(0,0,0), backcolor = COLOR_WHITE, texturecolor=COLOR_WHITE;
    wyWChar		*lpfileport = 0;
	wyWChar		directory[MAX_PATH+1] = {0};

    if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyTrue)
    {
        //Chnage line number margin color
        wyString	dirstr(directory);
        
        forecolor   =   wyIni::IniGetInt(GENERALPREFA, "FoldingMarginFgColor",   RGB(0,0,0), dirstr.GetString());
        backcolor   =   wyIni::IniGetInt(GENERALPREFA, "FoldingMarginbackgroundColor",   DEF_MARGINNUMBER, dirstr.GetString());
		texturecolor=   wyIni::IniGetInt(GENERALPREFA, "FoldingMarginTextureColor",   COLOR_WHITE, dirstr.GetString());
    }
    SendMessage(hwnd, SCI_SETPROPERTY, (WPARAM)"fold", (LPARAM)"1");
    SendMessage(hwnd, SCI_SETPROPERTY, (WPARAM)"fold.comment", (LPARAM)"1");
    SendMessage(hwnd, SCI_SETPROPERTY, (WPARAM)"fold.compact", (LPARAM)"0");
    SendMessage(hwnd, SCI_SETPROPERTY, (WPARAM)"fold.sql.only.begin", (LPARAM)"0");
    
    //SendMessage(hwnd, SCI_SETFOLDMARGINCOLOUR, 0, 0);
    SendMessage(hwnd, SCI_SETFOLDMARGINHICOLOUR, 1, texturecolor);
    SendMessage(hwnd, SCI_SETFOLDMARGINCOLOUR, 1, backcolor);
    //SendMessage(hwnd, SCI_SETFOLDMARGINHICOLOUR, 1, backcolor);

    SendMessage(hwnd, SCI_SETMODEVENTMASK, (WPARAM)SC_MODEVENTMASKALL, 0); 
    SendMessage(hwnd, SCI_SETMARGINTYPEN, (WPARAM)2, (LPARAM)SC_MARGIN_SYMBOL);
    SendMessage(hwnd, SCI_SETMARGINWIDTHN, (WPARAM)2, (LPARAM)14);
    SendMessage(hwnd, SCI_SETMARGINMASKN, (WPARAM)2, (LPARAM)SC_MASK_FOLDERS);
	SendMessage(hwnd, SCI_SETMARGINSENSITIVEN, (WPARAM)2, (LPARAM)1);

    SendMessage(hwnd, SCI_MARKERDEFINE, (WPARAM)SC_MARKNUM_FOLDEROPEN, (LPARAM)SC_MARK_BOXMINUS);
	SendMessage(hwnd, SCI_MARKERSETFORE, (WPARAM)SC_MARKNUM_FOLDEROPEN, (LPARAM)backcolor);
	SendMessage(hwnd, SCI_MARKERSETBACK, (WPARAM)SC_MARKNUM_FOLDEROPEN, (LPARAM)forecolor);

    SendMessage(hwnd, SCI_MARKERDEFINE, (WPARAM)SC_MARKNUM_FOLDER, (LPARAM)SC_MARK_BOXPLUS);
	SendMessage(hwnd, SCI_MARKERSETFORE, (WPARAM)SC_MARKNUM_FOLDER, (LPARAM)backcolor);
	SendMessage(hwnd, SCI_MARKERSETBACK, (WPARAM)SC_MARKNUM_FOLDER, (LPARAM)forecolor);

    SendMessage(hwnd, SCI_MARKERDEFINE, (WPARAM)SC_MARKNUM_FOLDERSUB, (LPARAM)SC_MARK_VLINE);
	SendMessage(hwnd, SCI_MARKERSETFORE, (WPARAM)SC_MARKNUM_FOLDERSUB, (LPARAM)backcolor);
	SendMessage(hwnd, SCI_MARKERSETBACK, (WPARAM)SC_MARKNUM_FOLDERSUB, (LPARAM)forecolor);

    SendMessage(hwnd, SCI_MARKERDEFINE, (WPARAM)SC_MARKNUM_FOLDERTAIL, (LPARAM)SC_MARK_LCORNER);
	SendMessage(hwnd, SCI_MARKERSETFORE, (WPARAM)SC_MARKNUM_FOLDERTAIL, (LPARAM)backcolor);
	SendMessage(hwnd, SCI_MARKERSETBACK, (WPARAM)SC_MARKNUM_FOLDERTAIL, (LPARAM)forecolor);    

    SendMessage(hwnd, SCI_MARKERDEFINE, (WPARAM)SC_MARKNUM_FOLDEREND, (LPARAM)SC_MARK_BOXPLUSCONNECTED);
	SendMessage(hwnd, SCI_MARKERSETFORE, (WPARAM)SC_MARKNUM_FOLDEREND, (LPARAM)backcolor);
	SendMessage(hwnd, SCI_MARKERSETBACK, (WPARAM)SC_MARKNUM_FOLDEREND, (LPARAM)forecolor);   

    SendMessage(hwnd, SCI_MARKERDEFINE, (WPARAM)SC_MARKNUM_FOLDEROPENMID, (LPARAM)SC_MARK_BOXMINUSCONNECTED);
	SendMessage(hwnd, SCI_MARKERSETFORE, (WPARAM)SC_MARKNUM_FOLDEROPENMID, (LPARAM)backcolor);
	SendMessage(hwnd, SCI_MARKERSETBACK, (WPARAM)SC_MARKNUM_FOLDEROPENMID, (LPARAM)forecolor);   

    SendMessage(hwnd, SCI_MARKERDEFINE, (WPARAM)SC_MARKNUM_FOLDERMIDTAIL, (LPARAM)SC_MARK_TCORNER);
	SendMessage(hwnd, SCI_MARKERSETFORE, (WPARAM)SC_MARKNUM_FOLDERMIDTAIL, (LPARAM)backcolor);
	SendMessage(hwnd, SCI_MARKERSETBACK, (WPARAM)SC_MARKNUM_FOLDERMIDTAIL, (LPARAM)forecolor); 

    SendMessage(hwnd, SCI_SETFOLDFLAGS, (WPARAM)SC_FOLDFLAG_LINEAFTER_CONTRACTED, 0);
}

void
PositionWindowCenterToMonitor(HWND handletoget, HWND handletoset)
{
    HMONITOR    hmonitor;
    MONITORINFO mi;
    wyInt32     width, height;
    RECT        rc;

    //find the monitor where the dialog is
    hmonitor = MonitorFromWindow(handletoget, MONITOR_DEFAULTTONEAREST);
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hmonitor, &mi);        

    //size and position the window in the center of the monitor
    GetWindowRect(handletoset, &rc);
    width = rc.right - rc.left;
    height = rc.bottom - rc.top;
    rc.left = mi.rcMonitor.left + (mi.rcMonitor.right - mi.rcMonitor.left - width) / 2;
    rc.top = mi.rcMonitor.top  + (mi.rcMonitor.bottom - mi.rcMonitor.top - height) / 2;
    SetWindowPos(handletoset, NULL, rc.left, rc.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

}

//Converting string to Base2 format to handling BIT type
void
ConvertStringToBinary(wyChar *data,  wyUInt32 len, wyInt32 colwidth, wyString *str)
{	
	wyInt32			bitcount = 8, i, length, total = 0;
	wyString		binstr, tempval, revstr, bitreevstr;
	unsigned char	ascii;
	
	colwidth = -1;

	if(!data)
	{
		str->SetAs("(NULL)");
		return;
	}
		
	if(!stricmp(data, "(NULL)"))
	{
		str->SetAs("(NULL)");
		return;
	}
	tempval.SetAs(data, wyTrue);

	length = len;
			
	if((tempval.GetCharAt(0) == 'b' || tempval.GetCharAt(0) == 'x') && tempval.GetCharAt(1) == '\'' && tempval.GetCharAt(length-1)=='\'')
	{			
		str->SetAs(data);
		return;
	}	
	wyBool isnullchar = wyFalse;
	wyInt32 onepos = 0;

	/*if(colwidth <= 0)
	{
		colwidth = len;
		tempcolwidth = len;
	}*/

	//Convert string to binary
	for(i = len - 1; i >=0 ; i--)
	{			
		ascii = data[i];
		if(!ascii)
		{
			isnullchar = wyTrue;
			revstr.Replace(0, 0, "00000000");
			isnullchar = wyFalse;
			continue;
		}
		//Adding 0s if NULL character inbetween
		/*if(isnullchar == wyTrue)
		{
			revstr.Replace(0, 0, "00000000");
			isnullchar = wyFalse;
		}*/
		//For handling the BIT column width
		//if(colwidth > 0)
		//{		
		//	if(tempcolwidth % 8 && (len <= 1))// || i < (len - 1)))
		//	{
		//		bitcount = tempcolwidth % 8;				
		//	}
		//	else if(tempcolwidth / 8)
		//	{
		//		bitcount = 8;
		//	}
		//	else
		//	{
		//		break;
		//	}
		//	tempcolwidth -= bitcount;
		//}
		//else
		{
			bitcount = 8;
		}
		while(ascii > 0 && bitcount && (colwidth == -1 || total < colwidth))
		{
			bitcount--;
			
			(ascii%2) ? binstr.Add("1"):binstr.Add("0");
			ascii = ascii/2;	
			
			total++;			
		}

		if(binstr.GetLength())
		{	
			//Padding zeros
			if(i > 0 && bitcount)// total < colwidth)
			{					
				while(bitcount > 0)
				{					
					binstr.Add("0");
					bitcount--;		
					total++;
				}
			}

			bitreevstr.SetAs(strrev((char*)binstr.GetString()));
						
			revstr.Replace(0, 0, bitreevstr.GetString());			
		}
				
		binstr.Clear();
		bitreevstr.Clear();
	}
	if(!revstr.GetLength())
	{
		revstr.SetAs("0");
	}
	else
	{
		//Removes the zeros if any
		onepos = revstr.Find("1", 0);
		if(onepos > 0)
		{
			revstr.Erase(0, onepos);
		}
	}

	str->AddSprintf("b'%s'", revstr.GetString());
}

//...Handles the OnMouseOverChangeIcon event
LRESULT CALLBACK StaticDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WNDPROC origWndProc = (WNDPROC) GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
		return 0;
	case WM_MOUSEMOVE:
		SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(32649)));
		return 0;
	}
	return CallWindowProc(origWndProc, hwnd, message, wParam, lParam);
}

wyInt32	SetAsLink(HDC hdc)
{
	wyUInt32  	fontheight;
	
	fontheight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);

	 
	pGlobals->m_pcmainwin->m_hfontlink = CreateFont(fontheight, 0, 0, 0, 0, 0, TRUE, 0, 0, 0, 0, 0, 0, L"Verdana");
	SelectObject(hdc, pGlobals->m_pcmainwin->m_hfontlink);
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, RGB(0, 0, 255));

	return (wyInt32)GetStockObject(HOLLOW_BRUSH);
}

void SetFilePathToCurrentDirectory(wyString *dir)
{
	TCHAR curdir[MAX_PATH];
	wyString wyDir;

	GetCurrentDirectory(MAX_PATH-1, curdir);
	wyDir.SetAs(curdir);
	wyDir.Add("\\");
	wyDir.Add(dir->GetString());
	dir->SetAs(wyDir);
}

void OpenLogFile(wyString *str)
{

	if(!((int)ShellExecute(NULL, L"OPEN", str->GetAsWideChar(), NULL, NULL, SW_SHOWNORMAL) > 32))	//returns <=32 in case of error..
	{
		int ret;
		ret = (int) ShellExecute(NULL, L"OPEN", L"notepad.exe", str->GetAsWideChar(), NULL, SW_SHOWNORMAL);
		if(!(ret > 32))																				//returns <=32 in case of error..
		{
			char err[255];
			switch(ret)
			{
			case ERROR_FILE_NOT_FOUND:
				strcpy(err, "File not found..");
				break;
			case ERROR_PATH_NOT_FOUND:
				strcpy(err, "Path not found..");
				break;
			case ERROR_BAD_FORMAT:
				strcpy(err, "File is invalid");
				break;
			case SE_ERR_ACCESSDENIED:
				strcpy(err, "Access denied");
				break;
			case SE_ERR_ASSOCINCOMPLETE:
				strcpy(err, "Invalid file name association");
				break;
			case SE_ERR_DDEBUSY:
				strcpy(err, "DDE transaction could not be completed");
				break;
			case SE_ERR_DDEFAIL:
				strcpy(err, "DDE transaction failed");
				break;
			case SE_ERR_DDETIMEOUT:
				strcpy(err, "Request timed out");
				break;
			case SE_ERR_DLLNOTFOUND:
				strcpy(err, "DLL not found");
				break;
			case SE_ERR_NOASSOC:
				strcpy(err, "File not associated with any application");
				break;
			case SE_ERR_OOM:
				strcpy(err, "Not enough memory");
				break;
			case SE_ERR_SHARE:
				strcpy(err, "Sharing violation occured");
				break;
			}
			wyString wyErr;
			wyErr.SetAs(err);
			MessageBox(NULL, wyErr.GetAsWideChar(), L"Error", MB_OK);
		}						
	}
}

wyInt32 IsAnyListItemChecked(HWND hListView)
{
    wyInt32 count;
    
    count = ListView_GetItemCount(hListView);
    for(int i=0; i<count; i++)
        if(ListView_GetCheckState(hListView,i))
            return 1;
    
    return 0;
}

wyBool 
SetWindowPositionFromINI(HWND hwnd, wyChar* sectionname, wyInt32 minwidth, wyInt32 minheight)
{
    wyWChar     directory[MAX_PATH];
    wyWChar*    lpfileport = NULL;
    wyString    dirstr, section;
    RECT        rect;
    wyInt32     width, height;
    HMONITOR    hmonitor;
    MONITORINFO mi;
    HWND        hwndparent = GetParent(hwnd);

    if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyFalse)
    {
        return wyFalse;
    }

    dirstr.SetAs(directory);
    section.SetAs(sectionname);

    //read the co-ordinates
    rect.left = wyIni::IniGetInt(section.GetString(), "Left", 0, dirstr.GetString());
    rect.top	= wyIni::IniGetInt(section.GetString(), "Top", 0, dirstr.GetString());
    rect.right = wyIni::IniGetInt(section.GetString(), "Right", 0, dirstr.GetString());
    rect.bottom = wyIni::IniGetInt(section.GetString(), "Bottom", 0, dirstr.GetString());
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

    //validate the width and height
    if(width < minwidth || height < minheight)
    {
        return wyFalse;
    }

    //get the monitor from the rectangle formed by reaading the ini
    hmonitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONULL);

    //if there is no such monitor we will bring it back to the native monitor but the size will be intact
    if(hmonitor == NULL)
    {
        //find the monitor where the dialog is
        if((hmonitor = MonitorFromWindow(hwndparent, MONITOR_DEFAULTTONULL)) == NULL)
        {
            return wyFalse;
        }

        //get the monitor info
        mi.cbSize = sizeof(mi);
        GetMonitorInfo(hmonitor, &mi);        

        //size and position the window in the center of the monitor
        rect.left = mi.rcMonitor.left + (mi.rcMonitor.right - mi.rcMonitor.left - width) / 2;
        rect.top = mi.rcMonitor.top  + (mi.rcMonitor.bottom - mi.rcMonitor.top - height) / 2;
        SetWindowPos(hwnd, NULL, rect.left, rect.top, width, height, SWP_NOZORDER);
    }
    else
    {
        //get the monitor info
        mi.cbSize = sizeof(mi);
        GetMonitorInfo(hmonitor, &mi);

        if(rect.right <= mi.rcWork.left || rect.bottom <= mi.rcWork.top || 
           mi.rcWork.right <= rect.left || mi.rcWork.bottom <= rect.top)
        {
            //size and position the window in the center of the monitor in case if it goes out of the work area
            rect.left = mi.rcMonitor.left + (mi.rcMonitor.right - mi.rcMonitor.left - width) / 2;
            rect.top = mi.rcMonitor.top  + (mi.rcMonitor.bottom - mi.rcMonitor.top - height) / 2;
            SetWindowPos(hwnd, NULL, rect.left, rect.top, width, height, SWP_NOZORDER);
        }
        else
        {
            //set the window position
            SetWindowPos(hwnd, NULL, rect.left, rect.top, width, height, SWP_NOZORDER);
        }
    }

    return wyTrue;
}

void 
FreeMenuOwnerDrawItem(HMENU hmenu, wyInt32 pos)
{
    /*wyInt32         i, count;
    MENUITEMINFO	mif;
    wyWChar*        newbuf;
    HMENU           hsubmenu;

    count = GetMenuItemCount(hmenu);

    for(i = pos != -1 ? pos : 0; pos != -1 ? (i == pos) : (i < count); ++i)
    {
        memset(&mif, 0, sizeof(mif));
		mif.cbSize = sizeof(mif);
        mif.fMask  = MIIM_TYPE | MIIM_DATA | MIIM_ID;
        mif.fType  = MFT_SEPARATOR;

		GetMenuItemInfo(hmenu, i, TRUE, &mif);

		if(mif.fType & MFT_SEPARATOR)
        {
            continue;
        }

        newbuf = (wyWChar*)mif.dwItemData;

        if(newbuf)
        {
            free(newbuf);
            newbuf = NULL;
        }

        if((hsubmenu = GetSubMenu(hmenu, i)))
        {
            FreeMenuOwnerDrawItem(hsubmenu);
        }
    }*/
}

wyInt64
GetHighPrecisionTickCount()
{
    LARGE_INTEGER           count;
    static LARGE_INTEGER    freq = {0};
    static BOOL             ishighres = QueryPerformanceFrequency(&freq);
    wyInt64                 time, temp;

    if(ishighres && QueryPerformanceCounter(&count))
    {
        temp = freq.QuadPart / 1000000;

        if(temp)
        {
            time = count.QuadPart / temp;
            return time;
        }
    }
    
    time = GetTickCount();
    time *= 1000;

    return time;
}

int
GetDatabasesFromOB(wyString*** dblist)
{
	HTREEITEM   hitem;
    TVITEM      tvi;
    wyWChar     buffer[SIZE_128];
	MDIWindow*	wnd;
	wnd = GetActiveWin();
	*dblist = NULL;
	wyString** newdblist = NULL;
	wyInt32 newdbcount = 0;

	//get the root node
	hitem = TreeView_GetRoot(wnd->m_pcqueryobject->m_hwnd);
        
    //loop throught the tree view node
    for(hitem = TreeView_GetChild(wnd->m_pcqueryobject->m_hwnd, hitem); hitem; 
        hitem = TreeView_GetNextSibling(wnd->m_pcqueryobject->m_hwnd, hitem))
    {
        memset(&tvi, 0, sizeof(tvi));
        tvi.hItem = hitem;
        tvi.mask = TVIF_TEXT | TVIF_IMAGE;
        buffer[0] = 0;
        tvi.pszText = buffer;
        tvi.cchTextMax = SIZE_128;
        TreeView_GetItem(wnd->m_pcqueryobject->m_hwnd, &tvi);
        newdbcount = newdbcount + 1;
        
        //allocate the memmory
        if(newdblist)
        {
            newdblist = (wyString**)realloc(newdblist, newdbcount * sizeof(wyString*));
        }
        else
        {
            newdblist = (wyString**)malloc(sizeof(wyString*));
        }

        //initialize with placement new
        newdblist[newdbcount - 1] = new wyString(tvi.pszText);
    }
	*dblist = newdblist;
	return newdbcount;
}

wyInt32 GetTextLenOfStaticWindow(HWND hwnd)
{
    HDC         hdc;
    RECT        rect = {0};
    wyInt32     size = 0;
    wyWChar*    buffer = NULL;
	wyString	buff;
	HFONT       hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    if(!hwnd)
        return 0;

    size = GetWindowTextLength(hwnd);
    if(size)
    {
        buffer = new wyWChar[size + 1];
        GetWindowText(hwnd, buffer, size + 1);
		buff.SetAs(buffer);

		hdc = GetDC(hwnd);
		hfont = (HFONT)SelectObject(hdc, hfont);
		memset(&rect, 0, sizeof(rect));
		DrawTextEx(hdc, buff.GetAsWideChar(), -1, &rect, DT_CALCRECT|DT_INTERNAL, NULL);
		ReleaseDC(hwnd, hdc);

        free(buffer);
        buffer = NULL;

        size = rect.right;
    }
    return size;
}

wyInt32 GetStaticTextHeight(HWND hwnd)
{
    HDC         hdc;
    RECT        rect = {0};
    wyInt32     size = 0;
    wyWChar*    buffer = NULL;
	wyString	buff;
	HFONT       hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    if(!hwnd)
        return 0;

    size = GetWindowTextLength(hwnd);
    if(size)
    {
        buffer = new wyWChar[size + 1];
        GetWindowText(hwnd, buffer, size + 1);
		buff.SetAs(buffer);

		hdc = GetDC(hwnd);
		hfont = (HFONT)SelectObject(hdc, hfont);
		memset(&rect, 0, sizeof(rect));
		DrawTextEx(hdc, buff.GetAsWideChar(), -1, &rect, DT_CALCRECT|DT_INTERNAL, NULL);
		ReleaseDC(hwnd, hdc);

        free(buffer);
        buffer = NULL;

        size = rect.bottom;
    }
    return size;
}

RECT GetTextSize(wyWChar* text, HWND hwnd, HFONT hfont)
{
    HDC         hdc;
    RECT        rect = {0};
		
    if(!text)
        return rect;
	
	hdc = GetDC(hwnd);
	SelectObject(hdc, (HFONT)hfont);

	memset(&rect, 0, sizeof(rect));
	DrawText(hdc, text, -1, &rect, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
	ReleaseDC(hwnd, hdc);

	return rect;
}

void
CreateCustomTabIconList()
{
    wyInt32 i, iconcount;
    HICON   hicon;
    wyInt32 icons[] = {IDI_CIRCLE1, IDI_CIRCLE2, IDI_CIRCLE3, IDI_CIRCLE4, IDI_CIRCLE5, IDI_CIRCLE6, IDI_CIRCLE7, IDI_CIRCLE8};
    
    iconcount = sizeof(icons)/sizeof(icons[0]);
    pGlobals->m_hiconlist = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, iconcount, 0);

    for(i = 0; i < iconcount; ++i)
    {
        hicon =(HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(icons[i]), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        ImageList_AddIcon(pGlobals->m_hiconlist, hicon);
        DestroyIcon(hicon);
    }
}

wyBool
GetTabOpenPersistence(wyInt32 tabimage, wyBool isset, wyBool valuetoset)
{
    wyWChar         directory[MAX_PATH + 1]={0}, *lpfileport=0;
	wyString		dirstr;
    const wyChar*   key = NULL;
    wyBool          isopen = wyFalse;

	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
    dirstr.SetAs(directory);

    switch(tabimage)
    {
        case IDI_TABLE:
            key = "TableDataTabOpen";
            break;

        case IDI_HISTORY:
            key = "HistoryTabOpen";
            break;
        
        case IDI_TABLEINDEX:
            key = "InfoTabOpen";
            break;
    }

    if(key)
    {
        if(isset == wyTrue)
        {
            isopen = valuetoset;
            wyIni::IniWriteInt(GENERALPREFA, key, isopen == wyTrue ? 1 : 0, dirstr.GetString());
        }
        else
        {
            isopen = wyIni::IniGetInt(GENERALPREFA, key, 1, dirstr.GetString()) ? wyTrue : wyFalse;
        }    
    }

    return isopen;
}

void 
EnableDisableExportMenuItem()
{
    wyInt32     ret = 0;
    wyWChar     directory[MAX_PATH + 1], *lpfileport=0;
    HMENU       hmenu = NULL;
    
    ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
    if(ret == 0)
    {
        EnableMenuItem(hmenu, ID_EXPORT_EXPORTCONNECTIONDETAILS, MF_GRAYED | MF_BYCOMMAND);
        return;
    }

    hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain);

    if(wyIni::IniIsConnectionExists(directory) == wyTrue)
    {
        EnableMenuItem(hmenu, ID_EXPORT_EXPORTCONNECTIONDETAILS, MF_ENABLED);
    }
    else
    {
        EnableMenuItem(hmenu, ID_EXPORT_EXPORTCONNECTIONDETAILS, MF_GRAYED | MF_BYCOMMAND);
    }
}

LPARAM
TV_GetLParam(HWND hwnd, HTREEITEM hti)
{
    TVITEM tvi;
    
    memset(&tvi, 0, sizeof(TVITEM));
    tvi.hItem = hti;
    tvi.mask = TVIF_PARAM;
    TreeView_GetItem(hwnd, &tvi);

    return(tvi.lParam);
}

void 
EscapeHtmlToBuffer(wyString* buffer, const wyChar* text)
{
	while(*text)
	{
		switch(*text)
		{
		case '<':
			buffer->Add("&lt;");
			break;

		case '>':
			buffer->Add("&gt;");
			break;

		case '&':
			buffer->Add("&amp;");
			break;

		case '\"':
			buffer->Add("&quot;");
			break;

		case ' ':
			buffer->Add("&nbsp;");
			break;


		default:
			buffer->AddSprintf("%c", *text);						
			break;						
		}
		
		text++;
	}
}
wyBool 
CreateSessionFile()
{
	wyWChar  path[MAX_PATH+1] = {0};
    HANDLE  hfile;

	//If explict path is defined
	if(pGlobals->m_configdirpath.GetLength())
	{
		//wcscpy(path, pGlobals->m_configdirpath.GetAsWideChar());
		wcsncpy(path, pGlobals->m_configdirpath.GetAsWideChar(), MAX_PATH);
		path[MAX_PATH] = '\0';
	}
	
    else if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path)))
        wcscat(path, L"\\SQLyog");
	
	else
		return wyFalse;

	/* first we will create a directory that will hold .ini file with name SQLyog.*/
	if(!CreateDirectory(path, NULL))
	{
		/* If the folder is there, then we will continue the process, otherwise we will return false */
		if(GetLastError()!= ERROR_ALREADY_EXISTS)
			return wyFalse;
	}

	wcscat(path, L"\\conrestore.ini");
	
	hfile = CreateFile(path, GENERIC_READ, FILE_SHARE_WRITE, NULL,
						CREATE_ALWAYS, NULL, NULL);

	if(hfile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hfile);
		return wyTrue;
	}

	return wyFalse;
}

wyBool 
GetSessionFile(wyWChar *path)
{
	//wyWChar  path[MAX_PATH+1] = {0};
    HANDLE  hfile;
	DWORD	filesize = 0;

	//If explict path is defined
	if(pGlobals->m_configdirpath.GetLength())
	{
		wcsncpy(path, pGlobals->m_configdirpath.GetAsWideChar(), MAX_PATH);
		path[MAX_PATH] = '\0';
	}
	
    else if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path)))
        wcscat(path, L"\\SQLyog");
	
	else
		return wyFalse;

	/* first we will create a directory that will hold .ini file with name SQLyog.*/
	if(!CreateDirectory(path, NULL))
	{
		/* If the folder is there, then we will continue the process, otherwise we will return false */
		if(GetLastError()!= ERROR_ALREADY_EXISTS)
			return wyFalse;
	}

	wcscat(path, L"\\conrestore.ini");

	hfile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, NULL, NULL);

	if(hfile != INVALID_HANDLE_VALUE)
	{
		
		filesize = GetFileSize(hfile, NULL);
		if(filesize > 0)
		{
			CloseHandle(hfile);
			return wyTrue;
		}
		CloseHandle(hfile);
		return wyFalse;
	}

	return wyFalse;
}

wyBool GetSessionDetails(wyWChar* conn, wyWChar* path, ConnectionInfo *conninfo, wyIni *inimgr)
{
	wyString connstr, pathstr;
	connstr.SetAs(conn);
	pathstr.SetAs(path);
	wyBool isfocussed;
	
	
	//inimgr.IniGetSectionDetailsInit(&connstr , &pathstr);
	inimgr->IniGetString2(connstr.GetString(), "Name", "", &conninfo->m_title, pathstr.GetString());
	inimgr->IniGetString2(connstr.GetString(), "Host", "localhost", &conninfo->m_host, pathstr.GetString());
	inimgr->IniGetString2(connstr.GetString(), "User", "root", &conninfo->m_user, pathstr.GetString());
	conninfo->m_isstorepwd = inimgr->IniGetInt2(connstr.GetString(), "StorePassword", 1, pathstr.GetString()) ? wyTrue: wyFalse;
	inimgr->IniGetString2(connstr.GetString(), "Password", "", &conninfo->m_pwd, pathstr.GetString());
	DecodePassword(conninfo->m_pwd);
	conninfo->m_port = inimgr->IniGetInt2(connstr.GetString(), "Port", 3306, pathstr.GetString());
	
	

	inimgr->IniGetString2(connstr.GetString(), "Database","", &conninfo->m_db, pathstr.GetString());
	conninfo->m_iscompress = inimgr->IniGetInt2(connstr.GetString(), "compressedprotocol", 0, pathstr.GetString()) ? wyTrue: wyFalse;
#ifndef COMMUNITY
//	if(pGlobals->m_entlicense.CompareI("Professional") != 0)
		conninfo->m_isreadonly = inimgr->IniGetInt2(connstr.GetString(), "readonly", 0, pathstr.GetString()) ? wyTrue: wyFalse;
#endif
	//Wait_TimeOut
	conninfo->m_isdeftimeout = inimgr->IniGetInt2(connstr.GetString(), "defaulttimeout", conninfo->m_isdeftimeout, pathstr.GetString())? wyTrue: wyFalse;
	inimgr->IniGetString2(connstr.GetString(), "waittimeoutvalue", WAIT_TIMEOUT_SERVER, &conninfo->m_strwaittimeout, pathstr.GetString());
			
#ifndef COMMUNITY

	conninfo->m_ishttp = inimgr->IniGetInt2(connstr.GetString(), "Tunnel", wyFalse, pathstr.GetString())? wyTrue: wyFalse;
	inimgr->IniGetString2 (connstr.GetString(), "Http", "", &conninfo->m_url, pathstr.GetString());
	conninfo->m_timeout = inimgr->IniGetInt2(connstr.GetString(), "HTTPTime", 30000, pathstr.GetString());
	conninfo->m_ishttpuds = inimgr->IniGetInt2(connstr.GetString(), "HTTPuds", 0, pathstr.GetString()) ? wyTrue: wyFalse;
	inimgr->IniGetString2(connstr.GetString(), "HTTPudsPath", "", &conninfo->m_httpudspath, pathstr.GetString());
			
	TUNNELAUTH *auth = &pGlobals->m_pcmainwin->m_tunnelauth;
	wyString	chalusername, chalpwd;
	wyString	proxy, proxyusername, proxypwd,contenttype;
	if(auth)
	{
		/* get the two auth flags */
		auth->ischallenge = (bool)inimgr->IniGetInt2(connstr.GetString(), "Is401", 0, pathstr.GetString()); 
		auth->isproxy = (bool)inimgr->IniGetInt2(connstr.GetString(), "IsProxy", 0, pathstr.GetString()); 

		/* get the proxy auth info and decode the password too */
		inimgr->IniGetString2(connstr.GetString(), "Proxy", "", &proxy, pathstr.GetString());
		auth->proxyport = inimgr->IniGetInt2(connstr.GetString(), "ProxyPort", 808, pathstr.GetString());
		inimgr->IniGetString2(connstr.GetString(), "ProxyUser", "", &proxyusername, pathstr.GetString());
		inimgr->IniGetString2(connstr.GetString(), "ProxyPwd", "", &proxypwd, pathstr.GetString());
	
		wcscpy(auth->proxy, proxy.GetAsWideChar());
		wcscpy(auth->proxyusername, proxyusername.GetAsWideChar());
		wcscpy(auth->proxypwd, proxypwd.GetAsWideChar());

		if(proxypwd.GetLength())
			DecodePassword(proxypwd);
		wcscpy(auth->proxypwd, proxypwd.GetAsWideChar());

		/* get the 401 error and decode the password too */
		inimgr->IniGetString2(connstr.GetString(), "401User", "", &chalusername, pathstr.GetString());
		inimgr->IniGetString2(connstr.GetString(), "401Pwd", "", &chalpwd, pathstr.GetString());

		wcscpy(auth->chalusername, chalusername.GetAsWideChar());

		if(chalpwd.GetLength())
			DecodePassword(chalpwd);

		wcscpy(auth->chalpwd, chalpwd.GetAsWideChar());

		//get the content-type info
		inimgr->IniGetString2(connstr.GetString(), "ContentType", "text/xml", &contenttype, pathstr.GetString());
		wcscpy(auth->content_type, contenttype.GetAsWideChar());

		//Encode con.info and query to base64 or not
		auth->isbase64encode = (inimgr->IniGetInt2(connstr.GetString(), "HttpEncode", 0, pathstr.GetString()))? true : false;
	}

	conninfo->m_isssh = inimgr->IniGetInt2(connstr.GetString(), "SSH",	0, pathstr.GetString()) ? wyTrue: wyFalse;
	inimgr->IniGetString2(connstr.GetString(), "SshUser", "", &conninfo->m_sshuser, pathstr.GetString());
	inimgr->IniGetString2(connstr.GetString(), "SshPwd", "", &conninfo->m_sshpwd, pathstr.GetString());
	DecodePassword(conninfo->m_sshpwd);
	inimgr->IniGetString2(connstr.GetString(), "SshHost", "", &conninfo->m_sshhost, pathstr.GetString());
	conninfo->m_sshport = inimgr->IniGetInt2 (connstr.GetString(), "SshPort", 0, pathstr.GetString());
	inimgr->IniGetString2(connstr.GetString(), "SshForHost", "", &conninfo->m_forhost, pathstr.GetString());
	conninfo->m_ispassword = inimgr->IniGetInt2(connstr.GetString(), "SshPasswordRadio", 1, pathstr.GetString()) ? wyTrue: wyFalse;
	inimgr->IniGetString2(connstr.GetString(), "SSHPrivateKeyPath", "", &conninfo->m_privatekeypath, pathstr.GetString());
	conninfo->m_issshsavepassword =  inimgr->IniGetInt2 (connstr.GetString(), "SshSavePassword", 1, pathstr.GetString())? wyTrue: wyFalse;

	conninfo->m_issslchecked = inimgr->IniGetInt2(connstr.GetString(), "SslChecked", 0, pathstr.GetString())? wyTrue: wyFalse;
	conninfo->m_issslauthchecked = inimgr->IniGetInt2 (connstr.GetString(), "SshAuth", 0, pathstr.GetString()) ? wyTrue: wyFalse;
	inimgr->IniGetString2(connstr.GetString(), "Client_Key", "" , &conninfo->m_clikey, pathstr.GetString());
	inimgr->IniGetString2(connstr.GetString(), "Client_Cert", "", &conninfo->m_clicert, pathstr.GetString());
	inimgr->IniGetString2(connstr.GetString(), "CA", "", &conninfo->m_cacert, pathstr.GetString());
	inimgr->IniGetString2(connstr.GetString(), "Cipher", "", &conninfo->m_cipher, pathstr.GetString());
#endif
	
	conninfo->m_rgbconn = inimgr->IniGetInt2(connstr.GetString(), "ObjectbrowserBkcolor",	RGB(255,255,255), pathstr.GetString());
	conninfo->m_rgbfgconn = inimgr->IniGetInt2(connstr.GetString(), "ObjectbrowserFgcolor",	0, pathstr.GetString());

	conninfo->m_isglobalsqlmode = inimgr->IniGetInt2(connstr.GetString(), "sqlmode_global", 0, pathstr.GetString()) ? wyTrue: wyFalse;
	inimgr->IniGetString2(connstr.GetString(), "sqlmode_value", "", &conninfo->m_sqlmode, pathstr.GetString());
	inimgr->IniGetString2(connstr.GetString(), "init_command", "", &conninfo->m_initcommand, pathstr.GetString());
	conninfo->m_keepaliveinterval = inimgr->IniGetInt2(connstr.GetString(), "keep_alive", 0, pathstr.GetString());
	isfocussed = inimgr->IniGetInt2(connstr.GetString(), "is_focussed", 0, pathstr.GetString()) ? wyTrue : wyFalse;
	//inimgr.IniGetSectionDetailsFinalize();
	return isfocussed;

}

wyBool GetTabDetailsFromTable(wyWChar* path, wyInt32 id, List* temptablist)
{
	
  
   
    HANDLE				hfind;
    WIN32_FIND_DATAW	fdata;
    wyString			directoryname;
	wySQLite			*sqliteobj;
	wyString			sqlitequery,sqliteerr;
	sqlite3_stmt    *res;
	const wyChar    *colval = NULL;
	wyInt32				tabtype;
	
	tabdetailelem  *temptabdetail;

 //   if(pGlobals->m_configdirpath.GetLength())
	//{
	//	wcscat(directory, pGlobals->m_configdirpath.GetAsWideChar());
	//	wcscat(directory, L"\\connrestore.db");
	//}
	//else
	//{
	//	if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, directory)))
	//	{
	//		wcscat(directory, L"\\SQLyog\\");	
	//		wcscat(directory, L"connrestore.db");
	//	}
	//	else 
	//		return wyFalse;
	//}
	hfind = FindFirstFile(path, &fdata);			
	directoryname.SetAs(path);

	sqliteobj = new wySQLite();
	//sqliteobj->Open(directoryname.GetString());


	if(hfind == INVALID_HANDLE_VALUE)
	{
		//no db found
		//done
		return wyFalse;
	}
	sqliteobj->Open(directoryname.GetString(), wyTrue);
	sqlitequery.Sprintf("SELECT * from tabdetails where Id = %d ORDER BY position",id);
	sqliteobj->Prepare(&res, sqlitequery.GetString());
	while(sqliteobj->Step(&res, wyFalse) && sqliteobj->GetLastCode() == SQLITE_ROW)
	{
		//Id INTEGER ,Tabid INTEGER,position INTEGER, title TEXT,tooltip TEXT,isfile INTEGER, isfocussed INTEGER, content TEXT
		//also add tabtype
		colval = sqliteobj->GetText(&res , "Tabtype");
		if(colval)
		{
			tabtype = sqliteobj->GetInt(&res , "Tabtype");
			if(tabtype == IDI_QUERYBUILDER_16 || tabtype == IDI_SCHEMADESIGNER_16 )
			{
#ifdef COMMUNITY
				continue;
#endif
			if(pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_PRO )
				continue;

			}
		}
		else
			//fail
			tabtype = IDI_QUERY_16;
		
		temptabdetail = new tabdetailelem();
		temptabdetail->m_id = id;
		temptabdetail->m_iimage = tabtype;
		 colval = sqliteobj->GetText(&res , "Tabid");
		 if(colval)
			 temptabdetail->m_tabid = sqliteobj->GetInt(&res , "Tabid");
		 else
			 //fail
			 temptabdetail->m_tabid = 0;
		 

		 colval = sqliteobj->GetText(&res , "position");
		 if(colval)
			 temptabdetail->m_position = sqliteobj->GetInt(&res , "position");
		 else
			 //fail
			 temptabdetail->m_position = 0;

		 colval = sqliteobj->GetText(&res , "leftortoppercent");
		 if(colval)
			 temptabdetail->m_leftortoppercent = sqliteobj->GetInt(&res , "leftortoppercent");
		 else
			 //fail
			 temptabdetail->m_leftortoppercent = 50;

		 colval = sqliteobj->GetText(&res , "isfile");
		 if(colval)
			 temptabdetail->m_isfile = sqliteobj->GetInt(&res , "isfile")?wyTrue:wyFalse;
		 else
			 temptabdetail->m_isfile = wyFalse;

		 colval = sqliteobj->GetText(&res , "isedited");
		 if(colval)
			 temptabdetail->m_isedited = sqliteobj->GetInt(&res , "isedited")?wyTrue:wyFalse;
		 else
			 temptabdetail->m_isedited = wyFalse;

		 colval = sqliteobj->GetText(&res , "isfocussed");
		 if(colval)
			 temptabdetail->m_isfocussed = sqliteobj->GetInt(&res , "isfocussed")?wyTrue:wyFalse;
		 else
			 temptabdetail->m_isfocussed = wyFalse;

		 colval = sqliteobj->GetText(&res , "title");
		 if(colval)
			 temptabdetail->m_psztext.SetAs(colval);

		 colval = sqliteobj->GetText(&res , "tooltip");
		 if(colval)
			 temptabdetail->m_tooltiptext.SetAs(colval);

		 colval = sqliteobj->GetText(&res , "content");
		 if(colval)
			 temptabdetail->m_content.SetAs(colval);

		 temptablist->Insert(temptabdetail);

	}
	sqliteobj->Finalize(&res);
	sqliteobj->Close();
	delete sqliteobj;
	return wyTrue;
}

wyBool GetHistoryDetailsFromTable(wyWChar* path, wyInt32 id, wyString *historydata)
{
	wyString pathstr;
	//wyBool isfocussed = wyFalse;
    
    HANDLE				hfind;
    WIN32_FIND_DATAW	fdata;
    wyString			directoryname;
	wySQLite			*sqliteobj;
	wyString			sqlitequery,sqliteerr;
	sqlite3_stmt    *res;
	const wyChar    *colval = NULL;
	//wyInt32			 position = 0;
	
	pathstr.SetAs(path);
	hfind = FindFirstFile(path, &fdata);			
	directoryname.SetAs(path);
	sqliteobj = new wySQLite();

	if(hfind == INVALID_HANDLE_VALUE)
	{
		//no db found
		//done
		return wyFalse;
	}
	sqliteobj->Open(directoryname.GetString(), wyTrue);
	sqlitequery.Sprintf("SELECT * from historydetails where Id = %d",id);
	sqliteobj->Prepare(&res, sqlitequery.GetString());
	if(sqliteobj->Step(&res, wyFalse) && sqliteobj->GetLastCode() == SQLITE_ROW)
	{
		colval = sqliteobj->GetText(&res , "historydata");
		if(colval)
			 historydata->SetAs(colval);
	}
	sqliteobj->Finalize(&res);
	sqliteobj->Close();
	delete sqliteobj;
	return wyTrue;
}

wyBool GetOBDetailsFromTable(wyWChar* path, wyInt32 id, wyString *obdb)
{
	wyString pathstr;
	//wyBool isfocussed = wyFalse;
    
    HANDLE				hfind;
    WIN32_FIND_DATAW	fdata;
    wyString			directoryname;
	wySQLite			*sqliteobj;
	wyString			sqlitequery,sqliteerr;
	sqlite3_stmt    *res;
	const wyChar    *colval = NULL;
	//wyInt32			 position = 0;
	
	pathstr.SetAs(path);
	hfind = FindFirstFile(path, &fdata);			
	directoryname.SetAs(path);
	sqliteobj = new wySQLite;

	if(hfind == INVALID_HANDLE_VALUE)
	{
		//no db found
		//done
		return wyFalse;
	}
	sqliteobj->Open(directoryname.GetString(), wyTrue);
	sqlitequery.Sprintf("SELECT * from obdetails where Id = %d",id);
	sqliteobj->Prepare(&res, sqlitequery.GetString());
	if(sqliteobj->Step(&res, wyFalse) && sqliteobj->GetLastCode() == SQLITE_ROW)
	{
		colval = sqliteobj->GetText(&res , "obdb");
		if(colval)
			 obdb->SetAs(colval);
			 
	}
	sqliteobj->Finalize(&res);
	sqliteobj->Close();
	delete sqliteobj;
	return wyTrue;
}

wyBool GetSessionfileDetailsFromTable(wyWChar* path, wyInt32 *isedited, wyString *obdb)
{
	wyString pathstr;
	//wyBool isfocussed = wyFalse;
    
    HANDLE				hfind;
    WIN32_FIND_DATAW	fdata;
    wyString			directoryname;
	wySQLite			*sqliteobj;
	wyString			sqlitequery,sqliteerr;
	sqlite3_stmt    *res;
	const wyChar    *colval = NULL;
	//wyInt32			 position = 0;
	
	pathstr.SetAs(path);
	hfind = FindFirstFile(path, &fdata);			
	directoryname.SetAs(path);
	sqliteobj = new wySQLite;

	if(hfind == INVALID_HANDLE_VALUE)
	{
		//no db found
		//done
		return wyFalse;
	}
	sqliteobj->Open(directoryname.GetString(), wyTrue);
	sqlitequery.Sprintf("SELECT * from sessiondetails where Id = 1");
	sqliteobj->Prepare(&res, sqlitequery.GetString());
	if(sqliteobj->Step(&res, wyFalse) && sqliteobj->GetLastCode() == SQLITE_ROW)
	{
		colval = sqliteobj->GetText(&res , "filename");
		if(colval)
			 obdb->SetAs(colval);

		colval = sqliteobj->GetText(&res , "isedited");
		 if(colval)
			 *isedited = sqliteobj->GetInt(&res , "isedited");
	}
	sqliteobj->Finalize(&res);
	return wyTrue;
}

wyBool GetSessionDetailsFromTable(wyWChar* path, ConnectionInfo *conninfo, wyInt32 id, MDIlist* tempmdilist)
{
	wyString pathstr;
	wyBool isfocussed = wyFalse;
    
    HANDLE				hfind;
    WIN32_FIND_DATAW	fdata;
    wyString			directoryname;
	wySQLite			*sqliteobj;
	wyString			sqlitequery,sqliteerr;
	sqlite3_stmt    *res;
	const wyChar    *colval = NULL;
	wyInt32			 position = 0;
	
	pathstr.SetAs(path);
	hfind = FindFirstFile(path, &fdata);			
	directoryname.SetAs(path);
	sqliteobj = new wySQLite;

	if(hfind == INVALID_HANDLE_VALUE)
	{
		//no db found
		//done
		return wyFalse;
	}
	sqliteobj->Open(directoryname.GetString(), wyTrue);
	sqlitequery.Sprintf("SELECT * from conndetails where Id = %d",id);
	sqliteobj->Prepare(&res, sqlitequery.GetString());
	if(sqliteobj->Step(&res, wyFalse) && sqliteobj->GetLastCode() == SQLITE_ROW)
	{
		 //= sqliteobj->GetInt(&res, 0);
		 colval = sqliteobj->GetText(&res , "position");
		 if(colval)
			 position = sqliteobj->GetInt(&res , "position");

		 colval = sqliteobj->GetText(&res , "Name");
		 if(colval)
			 conninfo->m_title.SetAs(colval);

		 colval = sqliteobj->GetText(&res , "Host");
		 if(colval)
			 conninfo->m_host.SetAs(colval);
		 else
			 conninfo->m_host.SetAs("localhost");

		 colval = sqliteobj->GetText(&res , "User");
		 if(colval)
			 conninfo->m_user.SetAs(colval);
		 else
			 conninfo->m_user.SetAs("root");

		 colval = sqliteobj->GetText(&res , "StorePassword");
		 if(colval)
			 conninfo->m_isstorepwd = sqliteobj->GetInt(&res , "StorePassword") ? wyTrue:wyFalse;
		 else
			 conninfo->m_isstorepwd = wyFalse;

		 colval = sqliteobj->GetText(&res , "Password"); 
		 if(colval)
			 conninfo->m_pwd.SetAs(colval);
		 DecodePassword(conninfo->m_pwd);

		 colval = sqliteobj->GetText(&res , "Port");
		 if(colval)
			 conninfo->m_port = sqliteobj->GetInt(&res , "Port");
		 else
			 conninfo->m_port = 3306;

		 colval = sqliteobj->GetText(&res , "Database");
		 if(colval)
			 conninfo->m_db.SetAs(colval);

		 colval = sqliteobj->GetText(&res , "compressedprotocol");
		 if(colval)
			 conninfo->m_iscompress = sqliteobj->GetInt(&res , "compressedprotocol") ? wyTrue:wyFalse;
		 else
			 conninfo->m_iscompress = wyFalse;

		 colval = sqliteobj->GetText(&res , "defaulttimeout");
		 if(colval)
			 conninfo->m_isdeftimeout = sqliteobj->GetInt(&res , "defaulttimeout") ? wyTrue : wyFalse;
		 
		 colval = sqliteobj->GetText(&res , "waittimeoutvalue");
		 if(colval)
			 conninfo->m_strwaittimeout.SetAs(colval);
		 else
			 conninfo->m_strwaittimeout.SetAs(WAIT_TIMEOUT_SERVER);

#ifndef COMMUNITY
		 colval = sqliteobj->GetText(&res , "readonly");
		 if(colval)
		//	 if(pGlobals->m_entlicense.CompareI("Professional") != 0)
			 conninfo->m_isreadonly = sqliteobj->GetInt(&res , "readonly") ? wyTrue:wyFalse;
		 else
			 conninfo->m_isreadonly = wyFalse;

		 colval = sqliteobj->GetText(&res , "Tunnel");
		 if(colval)
			 conninfo->m_ishttp = sqliteobj->GetInt(&res , "Tunnel") ? wyTrue:wyFalse;
		 else
			 conninfo->m_ishttp = wyFalse;

		 colval = sqliteobj->GetText(&res , "Http");
		 if(colval)
			 conninfo->m_url.SetAs(colval);

		 colval = sqliteobj->GetText(&res , "HTTPTime");
		 if(colval)
			 conninfo->m_timeout = sqliteobj->GetInt(&res , "HTTPTime");
		 else
			 conninfo->m_timeout = 30000;

		 colval = sqliteobj->GetText(&res , "HTTPuds");
		 if(colval)
			 conninfo->m_ishttpuds = sqliteobj->GetInt(&res , "HTTPuds")?wyTrue : wyFalse;
		 else
			 conninfo->m_ishttpuds = wyFalse;

		 colval = sqliteobj->GetText(&res , "HTTPudsPath");
		 if(colval)
			 conninfo->m_httpudspath.SetAs(colval);
		 
		 TUNNELAUTH *auth = &pGlobals->m_pcmainwin->m_tunnelauth;
		 wyString	chalusername, chalpwd;
		 wyString	proxy, proxyusername, proxypwd,contenttype;
		 if(auth)
		 {

			 colval = sqliteobj->GetText(&res , "Is401");
			 if(colval)
				 auth->ischallenge = sqliteobj->GetInt(&res , "Is401") ? wyTrue:wyFalse;
			 else
				 auth->ischallenge = wyFalse;

			 colval = sqliteobj->GetText(&res , "IsProxy");
			 if(colval)
				 auth->isproxy = sqliteobj->GetInt(&res , "IsProxy") ? wyTrue:wyFalse;
			 else
				 auth->isproxy = wyFalse;

			 colval = sqliteobj->GetText(&res , "Proxy");
			 if(colval)
				 proxy.SetAs(colval);

			 colval = sqliteobj->GetText(&res , "ProxyPort");
			 if(colval)
				 auth->proxyport = sqliteobj->GetInt(&res , "ProxyPort");
			 else
				 auth->proxyport = 808;

			 colval = sqliteobj->GetText(&res , "ProxyUser");
			 if(colval)
				 proxyusername.SetAs(colval);

			 colval = sqliteobj->GetText(&res , "ProxyPwd");
			 if(colval)
				 proxypwd.SetAs(colval);

			 wcscpy(auth->proxy, proxy.GetAsWideChar());
			 wcscpy(auth->proxyusername, proxyusername.GetAsWideChar());
			 wcscpy(auth->proxypwd, proxypwd.GetAsWideChar());

			 if(proxypwd.GetLength())
				DecodePassword(proxypwd);
			 wcscpy(auth->proxypwd, proxypwd.GetAsWideChar());

			 colval = sqliteobj->GetText(&res , "User401");
			 if(colval)
				 chalusername.SetAs(colval);

			 colval = sqliteobj->GetText(&res , "Pwd401");
			 if(colval)
				 chalpwd.SetAs(colval);

			 wcscpy(auth->chalusername, chalusername.GetAsWideChar());

			 if(chalpwd.GetLength())
				DecodePassword(chalpwd);

			 wcscpy(auth->chalpwd, chalpwd.GetAsWideChar());

			 colval = sqliteobj->GetText(&res , "ContentType");
			 if(colval)
				 contenttype.SetAs(colval);
			 else
				 contenttype.SetAs("text/xml");
			 wcscpy(auth->content_type, contenttype.GetAsWideChar());

			 colval = sqliteobj->GetText(&res , "HttpEncode");
			 if(colval)
				 auth->isbase64encode = sqliteobj->GetInt(&res , "HttpEncode") ? wyTrue:wyFalse;
			 else
				 auth->isbase64encode = wyFalse;
		 }
		 colval = sqliteobj->GetText(&res , "SSH");
		 if(colval)
			 conninfo->m_isssh = sqliteobj->GetInt(&res , "SSH") ? wyTrue:wyFalse;
		 else
			 conninfo->m_isssh = wyFalse;

		 colval = sqliteobj->GetText(&res , "SshUser");
		 if(colval)
			 conninfo->m_sshuser.SetAs(colval);

		 colval = sqliteobj->GetText(&res , "SshPwd");
		 if(colval)
			 conninfo->m_sshpwd.SetAs(colval);
		 DecodePassword(conninfo->m_sshpwd);

		 colval = sqliteobj->GetText(&res , "SshHost");
		 if(colval)
			 conninfo->m_sshhost.SetAs(colval);

		 colval = sqliteobj->GetText(&res , "SshPort");
		 if(colval)
			 conninfo->m_sshport = sqliteobj->GetInt(&res , "SshPort");
		 else
			 conninfo->m_sshport = 3306;

		 colval = sqliteobj->GetText(&res , "SshForHost");
		 if(colval)
			 conninfo->m_forhost.SetAs(colval);

		 colval = sqliteobj->GetText(&res , "SshPasswordRadio");
		 if(colval)
			 conninfo->m_ispassword = sqliteobj->GetInt(&res , "SshPasswordRadio") ? wyTrue:wyFalse;
		 else
			 conninfo->m_ispassword = wyTrue;

		 colval = sqliteobj->GetText(&res , "SSHPrivateKeyPath");
		 if(colval)
			 conninfo->m_privatekeypath.SetAs(colval);

		 colval = sqliteobj->GetText(&res , "SshSavePassword");
		 if(colval)
			 conninfo->m_issshsavepassword = sqliteobj->GetInt(&res , "SshSavePassword") ? wyTrue:wyFalse;
		 else
			 conninfo->m_issshsavepassword = wyTrue;

		 colval = sqliteobj->GetText(&res , "SslChecked");
		 if(colval)
			 conninfo->m_issslchecked = sqliteobj->GetInt(&res , "SslChecked") ? wyTrue:wyFalse;
		 else
			 conninfo->m_issslchecked = wyFalse;

		 colval = sqliteobj->GetText(&res , "SshAuth");
		 if(colval)
			 conninfo->m_issslauthchecked = sqliteobj->GetInt(&res , "SshAuth") ? wyTrue:wyFalse;
		 else
			 conninfo->m_issslauthchecked = wyFalse;

		 colval = sqliteobj->GetText(&res , "Client_Key");
		 if(colval)
			 conninfo->m_clikey.SetAs(colval);

		 colval = sqliteobj->GetText(&res , "Client_Cert");
		 if(colval)
			 conninfo->m_clicert.SetAs(colval);


		 colval = sqliteobj->GetText(&res , "CA");
		 if(colval)
			 conninfo->m_cacert.SetAs(colval);

		 colval = sqliteobj->GetText(&res , "Cipher");
		 if(colval)
			 conninfo->m_cipher.SetAs(colval);
#endif

		 colval = sqliteobj->GetText(&res , "ObjectbrowserBkcolor");
		 if(colval)
			 conninfo->m_rgbconn = sqliteobj->GetInt(&res , "ObjectbrowserBkcolor");
		 else
			 conninfo->m_rgbconn = RGB(255,255,255);

		 colval = sqliteobj->GetText(&res , "ObjectbrowserFgcolor");
		 if(colval)
			 conninfo->m_rgbfgconn = sqliteobj->GetInt(&res , "ObjectbrowserFgcolor");
		 else
			 conninfo->m_rgbfgconn = 0;

		 colval = sqliteobj->GetText(&res , "sqlmode_value");
		 if(colval)
			 conninfo->m_sqlmode.SetAs(colval);

		 colval = sqliteobj->GetText(&res , "sqlmode_global");
		 if(colval)
			 conninfo->m_isglobalsqlmode = sqliteobj->GetInt(&res , "sqlmode_global") ? wyTrue:wyFalse;
		 else
			 conninfo->m_isglobalsqlmode = wyFalse;

		 colval = sqliteobj->GetText(&res , "init_command");
		 if(colval)
			 conninfo->m_initcommand.SetAs(colval);

		 colval = sqliteobj->GetText(&res , "keep_alive");
		 if(colval)
			 conninfo->m_keepaliveinterval = sqliteobj->GetInt(&res , "keep_alive");
		 else
			 conninfo->m_keepaliveinterval = 0;

		 colval = sqliteobj->GetText(&res , "isfocussed");
		 if(colval)
			 isfocussed = sqliteobj->GetInt(&res , "isfocussed") ? wyTrue:wyFalse;
		 else
			 isfocussed = wyFalse;


	}
	sqliteobj->Finalize(&res);

	sqlitequery.Sprintf("SELECT * from obdetails where Id = %d",id);
	sqliteobj->Prepare(&res, sqlitequery.GetString());
	if(sqliteobj->Step(&res, wyFalse) && sqliteobj->GetLastCode() == SQLITE_ROW)
	{
		colval = sqliteobj->GetText(&res , "obdb");
		if(colval)
			tempmdilist->m_obdetails.SetAs(colval);
	}
	sqliteobj->Finalize(&res);

	sqlitequery.Sprintf("SELECT * from historydetails where Id = %d",id);
	sqliteobj->Prepare(&res, sqlitequery.GetString());
	if(sqliteobj->Step(&res, wyFalse) && sqliteobj->GetLastCode() == SQLITE_ROW)
	{
		colval = sqliteobj->GetText(&res , "historydata");
		if(colval)
			tempmdilist->m_historydata.SetAs(colval);
			 
	}
	sqliteobj->Finalize(&res);

	sqliteobj->Close();
	delete sqliteobj;
	tempmdilist->m_id = id;
	//tempmdilist->mdi = wnd;
	tempmdilist->m_ispresent = wyTrue;
	tempmdilist->m_position = position;
	tempmdilist->m_rgbconn = conninfo->m_rgbconn;
	tempmdilist->m_rgbfgconn = conninfo->m_rgbfgconn;
	tempmdilist->m_isfocussed = isfocussed;

	
	return isfocussed;

}

void	
WriteFullSectionToFile(FILE *out_stream, wyInt32 conno, ConnectionInfo *coninfo, const wyChar *title, wyBool isfocussed)
{
	wyString temp, pass, tempstr;
	TUNNELAUTH *auth = NULL;

	temp.Sprintf("[Connection %d]\r\n", conno);
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("Name=%s\r\n", title);
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("Host=%s\r\n", coninfo->m_host.GetString());
	fputs(temp.GetString(), out_stream);
	
	temp.Sprintf("User=%s\r\n", coninfo->m_user.GetString());
	fputs(temp.GetString(), out_stream);

	if(coninfo->m_ishttp)
	{
		pass.SetAs(coninfo->m_tunnel->GetPwd());
	}
	else 
	{
		pass.SetAs(coninfo->m_pwd);
	}
	EncodePassword(pass);
	temp.Sprintf("Password=%s\r\n", pass.GetString());
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("Port=%d\r\n", coninfo->m_port);
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("StorePassword=%d\r\n", coninfo->m_isstorepwd);
	fputs(temp.GetString(), out_stream);
	
	temp.Sprintf("keep_alive=%d\r\n", coninfo->m_keepaliveinterval);
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("Database=%s\r\n", coninfo->m_db.GetString());
	fputs(temp.GetString(), out_stream);
	
	temp.Sprintf("compressedprotocol=%d\r\n", coninfo->m_iscompress);
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("defaulttimeout=%d\r\n", coninfo->m_isdeftimeout);
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("waittimeoutvalue=%s\r\n", coninfo->m_strwaittimeout.GetString());
	fputs(temp.GetString(), out_stream);

#ifndef COMMUNITY
	temp.Sprintf("readonly=%d\r\n", coninfo->m_isreadonly);
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("Tunnel=%d\r\n", coninfo->m_ishttp);
	fputs(temp.GetString(), out_stream);
	
	temp.Sprintf("Http=%s\r\n", coninfo->m_url.GetString());
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("HTTPTime=%d\r\n", coninfo->m_timeout);
	fputs(temp.GetString(), out_stream);
	
	temp.Sprintf("HTTPuds=%d\r\n", coninfo->m_ishttpuds);
	fputs(temp.GetString(), out_stream);
	
	temp.Sprintf("HTTPudsPath=%s\r\n", coninfo->m_httpudspath.GetString());
	fputs(temp.GetString(), out_stream);

	auth = &pGlobals->m_pcmainwin->m_tunnelauth;

	if(auth)
	{
		if(auth->ischallenge)
			fputs("Is401=1\r\n", out_stream);
		else
			fputs("Is401=0\r\n", out_stream);
		
		if(auth->isproxy)
			fputs("IsProxy=1\r\n", out_stream);
		else
			fputs("IsProxy=0\r\n", out_stream);
		
		tempstr.SetAs(auth->proxy);
		temp.Sprintf("Proxy=%s\r\n", tempstr.GetString());
		fputs(temp.GetString(), out_stream);

		tempstr.SetAs(auth->proxyusername);
		temp.Sprintf("ProxyUser=%s\r\n", tempstr.GetString());
		fputs(temp.GetString(), out_stream);
		
		tempstr.SetAs(auth->proxypwd);
		if(tempstr.GetLength() != 0)
				EncodePassword(tempstr);
    
		temp.Sprintf("ProxyPwd=%s\r\n", tempstr.GetString());
		fputs(temp.GetString(), out_stream);
		
		tempstr.Sprintf("%d", auth->proxyport);
		temp.Sprintf("ProxyPort=%s\r\n", tempstr.GetLength()?tempstr.GetString():"");
		fputs(temp.GetString(), out_stream);

		tempstr.SetAs(auth->chalusername);
		temp.Sprintf("401User=%s\r\n", tempstr.GetString());
		fputs(temp.GetString(), out_stream);
		
		tempstr.SetAs(auth->chalpwd);
		if(tempstr.GetLength())
			EncodePassword(tempstr);

		temp.Sprintf("401Pwd=%s\r\n", tempstr.GetString());
		fputs(temp.GetString(), out_stream);
		
		tempstr.SetAs(auth->content_type);
		temp.Sprintf("ContentType=%s\r\n", tempstr.GetString());
		fputs(temp.GetString(), out_stream);

		temp.Sprintf("HttpEncode=%d\r\n", auth->isbase64encode);
		fputs(temp.GetString(), out_stream);
	}

	temp.Sprintf("SSH=%d\r\n", coninfo->m_isssh);
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("SshUser=%s\r\n", coninfo->m_sshuser.GetString());
	fputs(temp.GetString(), out_stream);

	tempstr.SetAs(coninfo->m_sshpwd);
	if(tempstr.GetLength())
	{
		EncodePassword(tempstr);
	}

	temp.Sprintf("SshPwd=%s\r\n", tempstr.GetString());
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("SshHost=%s\r\n", coninfo->m_sshhost.GetString());
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("SshPort=%d\r\n", coninfo->m_sshport);
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("SshForHost=%s\r\n", coninfo->m_forhost.GetString());
	fputs(temp.GetString(), out_stream);
	
	temp.Sprintf("SshPasswordRadio=%d\r\n", coninfo->m_ispassword);
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("SSHPrivateKeyPath=%s\r\n", coninfo->m_privatekeypath.GetString());
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("SshSavePassword=%d\r\n", coninfo->m_issshsavepassword);
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("SslChecked=%d\r\n", coninfo->m_issslchecked);
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("SshAuth=%d\r\n", coninfo->m_issslauthchecked);
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("Client_Key=%s\r\n", coninfo->m_clikey.GetString());
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("Client_Cert=%s\r\n", coninfo->m_clicert.GetString());
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("CA=%s\r\n", coninfo->m_cacert.GetString());
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("Cipher=%s\r\n", coninfo->m_cipher.GetString());
	fputs(temp.GetString(), out_stream);

#endif

	temp.Sprintf("ObjectbrowserBkcolor=%d\r\n", coninfo->m_rgbconn);
	fputs(temp.GetString(), out_stream);		

	temp.Sprintf("ObjectbrowserFgcolor=%d\r\n", coninfo->m_rgbfgconn);
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("sqlmode_global=%d\r\n", coninfo->m_isglobalsqlmode);
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("sqlmode_value=%s\r\n", coninfo->m_sqlmode.GetString());
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("init_command=%s\r\n", coninfo->m_initcommand.GetString());
	fputs(temp.GetString(), out_stream);

	temp.Sprintf("is_focussed=%d\r\n", isfocussed);
	fputs(temp.GetString(), out_stream);
}

//wyBool
//WriteTabDetailsToTable(tabeditorelem *temptabeditorele, CTCITEM quetabitem, wyInt32 tabid, wyInt32 position, wyInt32 id, TabEditor *tabqueryactive, MDIWindow *wnd)
//{
//	sqlite3_stmt*   stmt;
//	TabEditor		*tabquery;
//	wyString		testquery, sqlitequery;
//	
//	tabquery = (TabEditor*)quetabitem.m_lparam;
//	temptabeditorele->m_ispresent = wyTrue;
//	temptabeditorele->m_pctabeditor = tabquery;
//	temptabeditorele->m_tabid = tabid;
//	temptabeditorele->m_position = position;
//	temptabeditorele->m_color = quetabitem.m_color;
//	temptabeditorele->m_fgcolor = quetabitem.m_fgcolor;
//	temptabeditorele->m_isedited = quetabitem.m_isedited;
//	CustomTab_GetTitle(wnd->m_pctabmodule->m_hwnd, position, &temptabeditorele->m_psztext);
//	CustomTab_GetTooltip(wnd->m_pctabmodule->m_hwnd, position, &temptabeditorele->m_tooltiptext);
//	temptabeditorele->m_isfile = quetabitem.m_isfile;
//	temptabeditorele->m_leftortoppercent = tabquery->m_pcetsplitter->GetLeftTopPercent();
//	if(tabquery == tabqueryactive)
//		temptabeditorele->m_isfocussed = wyTrue;
//	else
//		temptabeditorele->m_isfocussed = wyFalse;
//	wnd->m_listtabeditor->Insert(temptabeditorele);
//	temptabeditorele->m_pctabeditor->m_peditorbase->GetCompleteText(testquery);
//	temptabeditorele->m_leftortoppercent = tabquery->m_pcetsplitter->GetLeftTopPercent();
//	sqlitequery.Sprintf("INSERT INTO tabdetails (Id, Tabid, position,leftortoppercent,isedited, title, tooltip, isfile,isfocussed,content) VALUES \
//												(? , ?, ?, ?, ?, ?, ?, ?, ?, ?)");
//
//	pGlobals->m_sqliteobj->Prepare(&stmt, sqlitequery.GetString());
//
//	//sqlitequery.Sprintf("INSERT INTO tabdetails (Id, Tabid, position,leftortoppercent,isedited, title, tooltip, isfile,isfocussed,content) 
//	//VALUES (%d , %d, %d, %d, %d, \"%s\", \"%s\", %d, %d, \"%s\")",
//	//tempmdilist->m_id,temptabeditorele->m_tabid,temptabeditorele->m_position,temptabeditorele->m_leftortoppercent,temptabeditorele->m_isedited,temptabeditorele->m_psztext.GetString(),temptabeditorele->m_tooltiptext.GetString(),temptabeditorele->m_isfile,temptabeditorele->m_isfocussed,testquery.GetString());
//	pGlobals->m_sqliteobj->SetInt(&stmt, 1, id);
//	pGlobals->m_sqliteobj->SetInt(&stmt, 2, temptabeditorele->m_tabid);
//	pGlobals->m_sqliteobj->SetInt(&stmt, 3, temptabeditorele->m_position);
//	pGlobals->m_sqliteobj->SetInt(&stmt, 4, temptabeditorele->m_leftortoppercent);
//	pGlobals->m_sqliteobj->SetInt(&stmt, 5, temptabeditorele->m_isedited);
//	pGlobals->m_sqliteobj->SetText(&stmt, 6, temptabeditorele->m_psztext.GetString());
//	pGlobals->m_sqliteobj->SetText(&stmt, 7,	temptabeditorele->m_tooltiptext.GetString());
//	pGlobals->m_sqliteobj->SetInt(&stmt, 8,	temptabeditorele->m_isfile);
//	pGlobals->m_sqliteobj->SetInt(&stmt, 9,	temptabeditorele->m_isfocussed);
//	pGlobals->m_sqliteobj->SetText(&stmt, 10,	testquery.GetString());
//	
//	pGlobals->m_sqliteobj->Step(&stmt, wyFalse);
//	pGlobals->m_sqliteobj->Finalize(&stmt);						
//
//	return wyTrue;
//
//}

wyBool	
WriteFullSectionToTable(wyString *sqlitequery, wyInt32 id, wyInt32 position, ConnectionInfo *coninfo, const wyChar *title, wyBool isfocussed,wySQLite	*ssnsqliteobj)
{
	wyString temp, pass, tempstr;
	TUNNELAUTH *auth = NULL;
	sqlite3_stmt*   stmt;
	wySQLite	*sqliteobj;
	sqliteobj = ssnsqliteobj ? ssnsqliteobj : pGlobals->m_sqliteobj;

	sqlitequery->Sprintf("INSERT INTO conndetails (Id ,position ,ObjectbrowserBkcolor  ,ObjectbrowserFgcolor  ,isfocussed ,Name   ,Host   ,User   ,Password   ,Port  ,StorePassword  ,keep_alive  ,Database   ,compressedprotocol  ,defaulttimeout  ,waittimeoutvalue  ,Tunnel  ,Http   ,HTTPTime  ,HTTPuds  ,HTTPudsPath   , Is401  ,IsProxy  ,Proxy   , ProxyUser  , ProxyPwd   , ProxyPort   , User401 , Pwd401 , ContentType , HttpEncode  ,SSH  ,SshUser  ,SshPwd  ,SshHost  ,SshPort  ,SshForHost  ,SshPasswordRadio  ,SSHPrivateKeyPath  ,SshSavePassword  ,SslChecked  ,SshAuth  ,Client_Key  ,Client_Cert  ,CA  ,Cipher  ,sqlmode_global  ,sqlmode_value ,init_command ,readonly ) VALUES \
												  (?   ,?       ,?						,?						  ,?	     ,?       ,?       ,?       ,?       ,?       ,?           ,?          ,?          ,?			       ,?				       ,?			 ,?       ,?       ,?       ,?       ,?					,?       ,?       ,?       ,?		       ,?	       ,?	       ,?	       ,?	       ,?	       ,?       ,?       ,?       ,?       ,?       ,?       ,?				 ,?			       ,?			       ,?			       ,?		    ,?       ,?          ,?           ,?     ,?       ,?			  ,?			,?			,?)");
	
	sqliteobj->Prepare(&stmt,sqlitequery->GetString());
	

	sqliteobj->SetInt(&stmt, 1, id);

	sqliteobj->SetInt(&stmt, 2, position);

	sqliteobj->SetInt(&stmt, 3, coninfo->m_rgbconn);

	sqliteobj->SetInt(&stmt, 4, coninfo->m_rgbfgconn);

	sqliteobj->SetInt(&stmt, 5, isfocussed);

	sqliteobj->SetText(&stmt, 6, title);

	sqliteobj->SetText(&stmt, 7, coninfo->m_host.GetLength()?coninfo->m_host.GetString():"");

	sqliteobj->SetText(&stmt, 8, coninfo->m_user.GetLength()?coninfo->m_user.GetString():"");


	if(coninfo->m_ishttp)
	{
		pass.SetAs(coninfo->m_tunnel->GetPwd());
	}
	else 
	{
		pass.SetAs(coninfo->m_pwd);
	}
	EncodePassword(pass);

	sqliteobj->SetText(&stmt, 9, pass.GetLength()?pass.GetString():"");

	sqliteobj->SetInt(&stmt, 10, coninfo->m_port);

	sqliteobj->SetInt(&stmt, 11, coninfo->m_isstorepwd);

	sqliteobj->SetInt(&stmt, 12, coninfo->m_keepaliveinterval);

	sqliteobj->SetText(&stmt, 13, coninfo->m_db.GetLength()?coninfo->m_db.GetString():"");

	sqliteobj->SetInt(&stmt, 14, coninfo->m_iscompress);

	sqliteobj->SetInt(&stmt, 15, coninfo->m_isdeftimeout);

	sqliteobj->SetText(&stmt, 16, coninfo->m_strwaittimeout.GetLength()?coninfo->m_strwaittimeout.GetString():"");


#ifndef COMMUNITY

	sqliteobj->SetInt(&stmt, 17, coninfo->m_ishttp);

	sqliteobj->SetText(&stmt, 18, coninfo->m_url.GetLength()?coninfo->m_url.GetString():"");

	sqliteobj->SetInt(&stmt, 19, coninfo->m_timeout);

	sqliteobj->SetInt(&stmt, 20, coninfo->m_ishttpuds);

	sqliteobj->SetText(&stmt, 21, coninfo->m_httpudspath.GetLength()?coninfo->m_httpudspath.GetString():"");


	auth = &pGlobals->m_pcmainwin->m_tunnelauth;

	if(auth)
	{
		if(auth->ischallenge)
			//sqlitequery->AddSprintf("1 ,");
			sqliteobj->SetInt(&stmt, 22, 1);
			//fputs("Is401=1\r\n", out_stream);
		else
			//sqlitequery->AddSprintf("0 ,");
			sqliteobj->SetInt(&stmt, 22, 0);
			//fputs("Is401=0\r\n", out_stream);
		
		if(auth->isproxy)
			//sqlitequery->AddSprintf("1 ,");
			sqliteobj->SetInt(&stmt, 23, 1);
			//fputs("IsProxy=1\r\n", out_stream);
		else
			//sqlitequery->AddSprintf("0 ,");
			sqliteobj->SetInt(&stmt, 23, 0);

		
		tempstr.SetAs(auth->proxy);

		sqliteobj->SetText(&stmt, 24, tempstr.GetLength()? tempstr.GetString():"");


		tempstr.SetAs(auth->proxyusername);

		sqliteobj->SetText(&stmt, 25, tempstr.GetLength()? tempstr.GetString():"");

		
		tempstr.SetAs(auth->proxypwd);
		if(tempstr.GetLength() != 0)
				EncodePassword(tempstr);

		sqliteobj->SetText(&stmt, 26, tempstr.GetLength()? tempstr.GetString():"");

		
		tempstr.Sprintf("%d", auth->proxyport);

		sqliteobj->SetInt(&stmt, 27, auth->proxyport);


		tempstr.SetAs(auth->chalusername);

		sqliteobj->SetText(&stmt, 28, tempstr.GetLength()? tempstr.GetString():"");

		
		tempstr.SetAs(auth->chalpwd);
		if(tempstr.GetLength())
			EncodePassword(tempstr);
		sqliteobj->SetText(&stmt, 29, tempstr.GetLength()? tempstr.GetString():"");

		tempstr.SetAs(auth->content_type);

		sqliteobj->SetText(&stmt, 30, tempstr.GetLength()? tempstr.GetString():"");

		sqliteobj->SetInt(&stmt, 31, auth->isbase64encode);

	}

	//sqlitequery->AddSprintf("%d ,", coninfo->m_isssh);
	sqliteobj->SetInt(&stmt, 32, coninfo->m_isssh);

	sqliteobj->SetText(&stmt, 33, coninfo->m_sshuser.GetLength()?coninfo->m_sshuser.GetString():"");

	tempstr.SetAs(coninfo->m_sshpwd);
	if(tempstr.GetLength())
	{
		EncodePassword(tempstr);
	}

	sqliteobj->SetText(&stmt, 34, tempstr.GetLength()? tempstr.GetString():"");

	sqliteobj->SetText(&stmt, 35, coninfo->m_sshhost.GetLength()?coninfo->m_sshhost.GetString():"");

	sqliteobj->SetInt(&stmt, 36, coninfo->m_sshport);

	sqliteobj->SetText(&stmt, 37, coninfo->m_forhost.GetLength()?coninfo->m_forhost.GetString():"");

	sqliteobj->SetInt(&stmt, 38, coninfo->m_ispassword);

	sqliteobj->SetText(&stmt, 39, coninfo->m_privatekeypath.GetLength()?coninfo->m_privatekeypath.GetString():"");

	sqliteobj->SetInt(&stmt, 40, coninfo->m_issshsavepassword);

	sqliteobj->SetInt(&stmt, 41, coninfo->m_issslchecked);

	sqliteobj->SetInt(&stmt, 42, coninfo->m_issslauthchecked);

	sqliteobj->SetText(&stmt, 43, coninfo->m_clikey.GetLength()?coninfo->m_clikey.GetString():"");

	sqliteobj->SetText(&stmt, 44, coninfo->m_clicert.GetLength()?coninfo->m_clicert.GetString():"");

	sqliteobj->SetText(&stmt, 45, coninfo->m_cacert.GetLength()?coninfo->m_cacert.GetString():"");

	sqliteobj->SetText(&stmt, 46, coninfo->m_cipher.GetLength()?coninfo->m_cipher.GetString():"");


#endif


	sqliteobj->SetInt(&stmt, 47, coninfo->m_isglobalsqlmode);

	sqliteobj->SetText(&stmt, 48,  coninfo->m_sqlmode.GetLength()?coninfo->m_sqlmode.GetString():"");

	sqliteobj->SetText(&stmt, 49,  coninfo->m_initcommand.GetLength()?coninfo->m_initcommand.GetString():"");

	sqliteobj->SetInt(&stmt, 50, coninfo->m_isreadonly);

	sqliteobj->Step(&stmt, wyFalse);
	sqliteobj->Finalize(&stmt);

	return wyTrue;
}
