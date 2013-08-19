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

#include <malloc.h>
#include "scintilla.h"

#ifndef COMMUNITY
#include "tinyxml.h"
#endif

#include "MDIWindow.h"
#include "Global.h"
#include "GUIHelper.h"
#include "ObjectBrowser.h"
#include "EditorFont.h"
#include "ExportMultiFormat.h"
#include "CommonHelper.h"
#include "FrameWindowHelper.h"
#include "MySQLVersionHelper.h"
#include "ExportMultiFormat.h"
#include "ClientMySQLWrapper.h"
#include "SQLMaker.h"
#include "FileHelper.h"
#include "GUIHelper.h"
#include "TabEditorSplitter.h"
#include "ConnectionTab.h"
#include "TableTabInterface.h"
#include "TableTabInterfaceTabMgmt.h"

#ifndef COMMUNITY
#include "ConnectionEnt.h"
#include "HelperEnt.h"
#include "TabQueryBuilder.h"
#include "TabSchemaDesigner.h"
#include"ProfilerAdvisors.h"
#include "DatabaseSearch.h"
#else
#include "CommunityRibbon.h"
#endif

extern	PGLOBALS		pGlobals;

#define SD_FILEOPENERROR	 _(L"Could not open Schema file.")
#define OPEN_BIGFILEWARNING	 _(L"This file is too big for the editor. Consider to use Tools->Restore From SQL Dump... instead")

#define IDC_CTAB   WM_USER+116

#define KEEP_ALIVE  300

void logtext1(wyChar *buff)
{
#ifdef _DEBUG
    FILE	*fp = fopen ( "c:\\SQLyog_log.log", "a+");
	fprintf(fp, "%s\n" , buff);
	fclose ( fp );
#endif
}

// Implementation of the CQueryWindow class.
// It encapsulates the main MDI child window of the application.

MDIWindow::MDIWindow(HWND hwnd, ConnectionInfo * conninfo, wyString &dbname, wyString &title)
{
    // Initialize the critical section one time only.
    InitializeCriticalSection(&m_cs);
	
	m_hwnd			= NULL;
	m_lastfocus		= NULL;
	m_hwndparent	= hwnd;
	m_mysql			= conninfo->m_mysql;
    m_stopmysql     = conninfo->m_mysql;
	m_tunnel		= conninfo->m_tunnel;
	m_stopquery		= 0;
	m_executing		= wyFalse;
	m_pingexecuting = wyFalse;
	m_isthreadbusy	= wyFalse;
	m_execoption	= INVALID;
	m_isobjbrowvis	= wyTrue;
	m_bopenflag		= wyFalse;
	m_psqlite		= NULL;
    m_queryevt      = NULL;
	m_dragged		= wyFalse;    
    m_isactivated   = wyFalse;
	m_dragimaglist  = NULL;
    m_isprofilerrequire = wyFalse;

	m_myresstatusfirst			= NULL;
	m_myresstatussecond			= NULL;
	m_myresstatusforquery		= NULL;
	m_myresprofileresultquery	= NULL;

	m_querysuccessful			= 0;
	m_ismdiclosealltabs			= wyFalse;

	m_isprofilesupported		= wyTrue;
	m_ispqacheck				= wyFalse;
	m_issessionstatusreconnected = wyFalse;
	m_isstauscostmeasured		= wyFalse;
	m_isshowprofilereconnected	= wyFalse;
	m_isstatuscostmeasurereconnected = wyFalse;
	m_isprofilecheckover		= wyFalse;
    m_isselectquery				= wyFalse; 
	m_isqueryprofileidfound		= wyFalse;

	m_ismanualresizing			= wyFalse;
	m_historyedit				= NULL;

	
    m_acinterface       = new AutoCompleteInterface();

	m_filterdb.SetAs(dbname.GetString());
	m_title.SetAs(title.GetString());
	m_severversion = 0;

    m_keepaliveinterval = conninfo->m_keepaliveinterval;
	
	//flag for closing mdi window
	m_iswinclosed = wyFalse;
	
	/* copy the connection info as we require some info to create plink shell et al. */
    InitConInfo(*conninfo, m_conninfo);
    
    GetScintillaKeyWordsAndFunctions(m_tunnel, m_mysql, wyTrue, m_keywordstring);
    GetScintillaKeyWordsAndFunctions(m_tunnel, m_mysql, wyFalse, m_functionstring);

    m_mdisaveselection = -1;

#ifndef COMMUNITY
	m_constatusparm = NULL;
#endif
	m_acinitevent = NULL; 
	m_threadidinit = NULL;
	m_htooltipthrd = NULL;

	m_actooltipevent = NULL;
	m_actooltipwaitevent = NULL;

    m_hthrdkeepalive = NULL;
    m_hevtkeepalive = NULL;
	m_hevtexecution = NULL;
	m_postactivatemsg = wyTrue;
}

MDIWindow::~MDIWindow()
{
	if(m_pctabmodule)
		delete m_pctabmodule;

    delete m_acinterface;
	delete m_pcqueryobject;
	delete m_pcqueryvsplitter;
	
    if(m_tunnel)
        delete m_tunnel;

    // Release resources used by the critical section object.
    DeleteCriticalSection(&m_cs);
}

wyBool
MDIWindow::Create()
{
    HANDLE hfile;
	wyString  title;
	wyUInt32 tid;

	
	CreateQueryWindow(m_hwndparent, &m_mysql);
	SelectDefaultDatabase();
	SetObjBrowVis();
	SetResultTextOrGrid();

	//title for  connection tab
   if(m_filterdb.GetLength())
   {
		title.Sprintf("%s/%s %s", m_title.GetString(), m_filterdb.GetString(),
						m_tunneltitle.GetString());
   }
	else
	{
		title.Sprintf("%s %s", m_title.GetString(), m_tunneltitle.GetString());
	}

   	//tab interface, create a tab with new connection
    //create connection tab control
	pGlobals->m_pcmainwin->m_conntab->InsertConnectionTab(&title, (LPARAM)this);

	//Setting focus to History tab.
	//m_pctabmodule->GetActiveTabEditor()->m_pctabmgmt->SelectQueryInfoTab();
				
	// checks wheather the connection is established and commandline as got filename to be displayed on Sqlyog Editor
	if(pGlobals->m_isconnected == wyTrue && (pGlobals->m_filename.GetLength() != 0))
	{
		hfile = CreateFile(pGlobals->m_filename.GetAsWideChar(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);                     
	    if(hfile == INVALID_HANDLE_VALUE)
	    {
		    DisplayErrorText(GetLastError(), _("Could not open file."));
		    return wyFalse;
	    }

		WriteSQLToEditor(hfile);     //Writes the file content into Query Editor
		pGlobals->m_filename.Clear(); // Clears the file name after writing the file contents
	}


    // start the keep alive timer if it is not a http connection

    if(m_tunnel->IsTunnel() == false && m_keepaliveinterval)
    {
        m_hevtkeepalive = CreateEvent(NULL, TRUE, FALSE, NULL);
		m_hevtexecution = CreateEvent(NULL, TRUE, TRUE, NULL);
        m_hthrdkeepalive = (HANDLE)_beginthreadex(NULL, 0, MDIWindow::KeepAliveThreadProc, this, 0, &tid);
		SetTimer(m_hwnd, KEEP_ALIVE, (m_keepaliveinterval*1000), NULL);
    }
	else if(m_tunnel->IsTunnel())
	{
		m_keepaliveinterval = 0;
	}

    return wyTrue;
}

// Create the main window.
// This is a MDI client window and not any other normal window.
HWND
MDIWindow::CreateQueryWindow(HWND hwnd, PMYSQL mysql)
{
	wyString		title;
	HWND			hwndquery;
	MDICREATESTRUCT mdicreate;
	wyBool			isMaximized = GetChildMaximized();

	mdicreate.szClass =(LPCWSTR)QUERY_WINDOW_CLASS_NAME_STR;
	mdicreate.szTitle =(LPCWSTR)_(L"Query");
	mdicreate.hOwner  = pGlobals->m_pcmainwin->GetHinstance();
	mdicreate.x       = CW_USEDEFAULT;
	mdicreate.y       = CW_USEDEFAULT;
	mdicreate.cx      = CW_USEDEFAULT;
	mdicreate.cy      = CW_USEDEFAULT;
	mdicreate.style   = WS_CLIPCHILDREN | WS_CLIPSIBLINGS ;
	mdicreate.lParam  =(LPARAM)this;
	if(isMaximized)
		mdicreate.style |= WS_MAXIMIZE;
	else
		mdicreate.style |= WS_OVERLAPPED;
	
	VERIFY(hwndquery	=(HWND)SendMessage(hwnd, WM_MDICREATE, 0,(LPARAM)(LPMDICREATESTRUCT)&mdicreate)); 

	// Create the title string.
	// Create the server name.
	if(m_tunnel->IsTunnel() == false)
	{
		if(m_conninfo.m_isssh)
		{
			m_tunneltitle.Sprintf(_("- %s@%s - Using SSH tunnel to %s"), (*mysql)->user, 
								m_conninfo.m_localhost.GetString(), m_conninfo.m_sshhost.GetString());
		}
		else
		{
			m_tunneltitle.Sprintf("- %s@%s", (*mysql)->user,
								(strlen((*mysql)->host)!= 0)?(*mysql)->host : "localhost");
		}

	}
	else
	{
		m_tunneltitle.Sprintf(_("- %s@%s - Using HTTP tunneler at %s"), m_tunnel->GetUser(),
						(strlen(m_tunnel->GetHost())!= 0)? m_tunnel->GetHost(): "localhost", m_tunnel->GetHttpAddr());
	}

	//if user selected and database name, then add it to title
	if(m_filterdb.GetLength())
		title.Sprintf("%s/%s %s", m_title.GetString(), m_filterdb.GetString(), m_tunneltitle.GetString());
	else
		title.Sprintf("%s %s", m_title.GetString(), m_tunneltitle.GetString());

	SetWindowText(hwndquery, title.GetAsWideChar());
		
	m_hwnd = hwndquery;
	
	/* starting from 4.02 we issue query like show variables like and set the client value to the database */
	/* only required in direct connection as in HTTP we execute it always in the server side */
	SetCharacterSet(this, m_tunnel, *mysql, m_conninfo.m_codepage.m_cpname, wyTrue, wyTrue);

	//if(IsMySQL5010(m_tunnel, mysql))
		//SetDefaultSqlMode(m_tunnel, &m_mysql, wyTrue);

	m_statusbartext.SetAs(_(L" Connection to server successful..."));

	pGlobals->m_pcmainwin->AddTextInStatusBar(m_statusbartext.GetAsWideChar());

    m_acinterface->HandlerBuildTags(this);

	//set saved color for connection as object browser color
	m_conninfo.m_rgbconn = pGlobals->m_pcmainwin->m_connection->m_rgbobbkcolor;
	m_conninfo.m_rgbfgconn = pGlobals->m_pcmainwin->m_connection->m_rgbobfgcolor;

	//save the connection number, to save from object browser
	m_currentconn.SetAs(pGlobals->m_pcmainwin->m_connection->m_currentconn);

    return hwndquery;
}

/* Function to set sql_mode, it is used bs a hack to STRICT_ALL_TABLES */
void
MDIWindow::SetDefaultSqlMode(Tunnel * tunnel, PMYSQL pmysql, wyBool reconnect, wyBool profile)
{
	wyString		query;
	MYSQL_RES		*res;
	
	query.Sprintf("set sql_mode=''");
    
    if(reconnect == wyTrue)
		res = ExecuteAndGetResult(this, tunnel, pmysql, query, profile);
    else
        res = SjaExecuteAndGetResult(tunnel, pmysql, query);
		
	if(!res && m_tunnel->mysql_affected_rows(*pmysql)== -1)
		return;

	m_tunnel->mysql_free_result(res);

	return;
}

// Function to set the defualt database when a new query window is selected.
void
MDIWindow::SelectDefaultDatabase()
{
	wyString		query;
	wyWChar         *tempdb;
	wyWChar         *token;
	wyWChar			seps[] = L";";
	MYSQL_RES       *res;

	// Now change the cursor to wait mode.
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	
	// if the user has not given any database name then we return.
	if(m_filterdb.GetLength()== 0)
		return;

    tempdb = AllocateBuffWChar(m_filterdb.GetLength()+ 1);
	wcscpy(tempdb, m_filterdb.GetAsWideChar());
	
	// else we select the first database as the default database.
	token =(wyWChar*)wcstok(tempdb, seps);
	
	wyString	tokenstr(token);

	// Get the original database so if the user dosnt select a database or an error occurs
	// then change it to the current database. i mean show it in combodropdown.
	query.Sprintf("use `%s`", tokenstr.GetString());
	
	res  =  ExecuteAndGetResult(this, m_tunnel, &(m_mysql), query);

	if((!res && m_tunnel->mysql_affected_rows(m_mysql) == -1))
		goto cleanup;
	else
	{   
		pGlobals->m_pcmainwin->AddTextInCombo(token);
		pGlobals->m_lastdatabase.SetAs(token);
		m_database.SetAs(token);
		m_tunnel->SetDB(tokenstr.GetString());
        free(tempdb);
	}
 
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	return;

cleanup:
	pGlobals->m_pcmainwin->AddTextInCombo(NODBSELECTED);
	pGlobals->m_lastdatabase.SetAs(NODBSELECTED);
    free(tempdb);
    return;

}

// Window procedure for the window.

LRESULT	CALLBACK
MDIWindow::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	MDIWindow*      pcquerywnd = (MDIWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    DWORD			thrid;
	ConnectionBase* conbase = NULL;
    wyInt32         exstyle;
	wyString		errmsg;
    
	switch(message)
	{
    /*case WM_COMMAND:
        switch(LOWORD(wparam))
        {
            case IDC_OBJECTFILTER:
                switch(HIWORD(wparam))
                {
                    case EN_UPDATE:
                        pcquerywnd->m_pcqueryobject->HandleOBFilter(hwnd, lparam);
                        break;
                }
            break;
        }
        break;*/
    //case WM_CTLCOLOREDIT:
    //    hdc = (HDC)wparam;
    //    hedit = (HWND)lparam;
    //    COLORREF    backcolor, forecolor;

    //    if(pGlobals->m_pcmainwin->m_connection->m_rgbobbkcolor >= 0)
		  //  backcolor  = pGlobals->m_pcmainwin->m_connection->m_rgbobbkcolor;
	   // else
		  //  backcolor = COLOR_WHITE;

	   // if(pGlobals->m_pcmainwin->m_connection->m_rgbobfgcolor >= 0)
		  //  forecolor  = pGlobals->m_pcmainwin->m_connection->m_rgbobfgcolor;
	   // else
		  //  forecolor = backcolor ^ 0xFFFFFF;
    //    
    //    //SetBkMode(hdc, TRANSPARENT);
    //    SetTextColor(hdc, forecolor); 
    //    SetBkColor(hdc, backcolor);
    //    return (BOOL)GetStockObject(HOLLOW_BRUSH);

    //    break;
    
    case WM_MDIACTIVATE:
        pcquerywnd->OnMDIActivate(hwnd, wparam, lparam);
		break;

	case WM_INITDLGVALUES:
		pcquerywnd->OnInitDialog();
        break;

	case WM_NCCREATE:
		{
			LPCREATESTRUCT		pCreatestruct = (LPCREATESTRUCT)lparam;
			LPMDICREATESTRUCT	pMdicreate	  = (LPMDICREATESTRUCT)pCreatestruct->lpCreateParams;
			pcquerywnd						  = (MDIWindow*)pMdicreate->lParam;
 
			SetWindowLongPtr(hwnd, GWLP_USERDATA,(LONG_PTR)pcquerywnd);

            if(wyTheme::IsSysmenuEnabled() == wyFalse)
            {
                SetWindowLongPtr(hwnd, GWL_STYLE, GetWindowLongPtr(hwnd, GWL_STYLE) & (~WS_SYSMENU));
            }

            exstyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, exstyle | WS_EX_ACCEPTFILES);
		}

		return wyTrue;

	case WM_CREATE:
		if(IsMySQL41(pcquerywnd->m_tunnel, &(pcquerywnd->m_mysql)))
			pcquerywnd->m_ismysql41 = wyTrue;
		else
			pcquerywnd->m_ismysql41 = wyFalse;

		pcquerywnd->OnWmCreate(hwnd);
		if(pcquerywnd->m_postactivatemsg == wyTrue)
			PostMessage(hwnd, WM_MDIACTIVATE, 0, (LPARAM)hwnd);
		break;
     
	case AUTO_THR_EXIT: 
        {
            pcquerywnd->m_acinterface->HandlerOnAutoThreadExit(pcquerywnd, lparam);
            if(WaitForSingleObject(pcquerywnd->m_psqlite->m_hmainevt, WAIT_FOR_AUTOTHREADEXIT) == WAIT_TIMEOUT)
            {
                GetExitCodeThread(pcquerywnd->m_psqlite->m_hstorethrd, &thrid);
                TerminateThread(pcquerywnd->m_psqlite->m_hstorethrd, thrid);
            }
            CloseHandle(pcquerywnd->m_psqlite->m_hstorethrd);
            CloseHandle(pcquerywnd->m_psqlite->m_hmainevt);
        }
        break;

    case AUTO_THR_EXIT2:
        pcquerywnd->m_psqlite->m_signalled = wyTrue;
        break;

    case AUTO_CHILD_THR_EXIT:
        pGlobals->m_pcmainwin->m_connection->CloseThreads(pcquerywnd);
        break;

	case WM_SIZE:
		pcquerywnd->Resize();
		
		//8.04, for avoiding painting issues at border of con. window when 'Restored'
		if(wparam == SIZE_RESTORED && pcquerywnd->m_ismanualresizing == wyFalse)
			InvalidateRect(hwnd, NULL, TRUE);
		
		pcquerywnd->m_ismanualresizing = wyFalse;
		break;						

	case WM_SIZING:
		pcquerywnd->m_ismanualresizing = wyTrue;		
		break;


	case WM_QUERYENDSESSION:
	case WM_CLOSE:
		if(pcquerywnd->OnWmClose(hwnd) == 0 )
		{
			//if connection is not closed, then set the flags to false
			pcquerywnd->m_iswinclosed = wyFalse;
			pGlobals->m_pcmainwin->m_closetab = wyFalse;
            return 0;
		}
		else
		{
			pcquerywnd->m_iswinclosed = wyTrue;
			pGlobals->m_pcmainwin->m_closetab = wyTrue;
		}
        break;
    
    case WM_DESTROY:
        break;

    case WM_NCDESTROY:
		delete pcquerywnd;
		break;

	case WM_NOTIFY:
		if(pcquerywnd->OnWmNotify(hwnd, wparam, lparam) == wyTrue)
            return 1;

		break;

	case WM_MOUSEMOVE:
		// Destrying Drag image for Query Builder
		if(pcquerywnd->m_dragimaglist)
			ImageList_Destroy((HIMAGELIST)pcquerywnd->m_dragimaglist);
		ImageList_DragLeave(hwnd);   
		ImageList_EndDrag();
		pcquerywnd->m_dragged = wyFalse;

		if(pcquerywnd->m_executing == wyTrue)
		{
			SetCursor(LoadCursor(NULL, IDC_WAIT));
			break;
		}
		
		
        pcquerywnd->OnMouseMove();
		if(wparam & WM_LBUTTONDOWN)
		{
			if(pcquerywnd->m_dragged == wyTrue)
				SetCapture(hwnd);
		}
		break;

	case WM_HELP:
		ShowHelp("Getting%20started%20SQLyog%20GUI%20for%20MySQL.htm");
		break;

	case UM_TAGFILE_UPDATE_START:
        pcquerywnd->m_pcquerystatus->ShowInformation(((wyString*)lparam)->GetAsWideChar(), 0);
		break;
		
	case UM_TAGFILE_UPDATE_END:
		{
			conbase = pGlobals->m_pcmainwin->m_connection;

            pcquerywnd->m_pcquerystatus->ShowInformation((wyWChar *)lparam, 0);
			if((wyWChar *)lparam)
				free((wyWChar *)lparam);

			if(pGlobals->m_pcmainwin && conbase &&	conbase->m_isbuiltactagfile == wyFalse)
			{
				conbase->m_isbuiltactagfile = wyTrue;

				//Set the 'AutocompleteTagbuilded' sets to 1 once the buit is completed
				pcquerywnd->m_acinterface->UpdateTagBuildFlag();				
			}
		}
		break;
		
	case UM_TAGFILE_UPDATE_CLOSE:
        pcquerywnd->m_pcquerystatus->ShowInformation(L" ", 0);		
		break;
		
	case UM_REFRESHOBJECT:	
		pcquerywnd->OnRefreshObject();
		break;

	case UM_FOCUS:
		pcquerywnd->OnUMFocus();
        break;

	case WM_CTLCOLORSTATIC:
		return pcquerywnd->OnDlgProcColorStatic(wparam, lparam);			
		
	case WM_MEASUREITEM:
		return wyTrue; 

    case WM_DROPFILES:
        if(pcquerywnd->m_executing == wyFalse)
            HandleOnDragAndDropFiles((HDROP)wparam, pcquerywnd);
        return 1;

	case WM_TIMER:
		if(wparam == KEEP_ALIVE && pcquerywnd->m_executing == wyFalse && pcquerywnd->m_isthreadbusy == wyFalse)
		{
           pcquerywnd->HandleKeepAliveTimer();
		}
        return wyTrue;

}

//Community ribbon hyper link
#ifdef COMMUNITY  
	if((HIWORD(wparam) == STN_CLICKED)&&(LOWORD(wparam) == IDC_COMMTITLE))
	    ShellExecute(NULL, L"open", TEXT(BUYURL_TOOLBARRIBBON), NULL, NULL, SW_SHOWNORMAL);
#endif

	return DefMDIChildProc(hwnd, message, wparam, lparam);
}

void 
MDIWindow::OnMDIActivate(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	wyInt32   index;
    
	/// Change the content of the combo box.

    pGlobals->m_pcmainwin->AddTextInCombo(m_database.GetAsWideChar());
	pGlobals->m_pcmainwin->AddTextInStatusBar(m_statusbartext.GetAsWideChar());

    if(m_hwnd == (HWND)lparam)
    {
	    if(m_executing == wyFalse)
	    {
		    SetResultTextOrGrid();
		    SetObjBrowVis();
		    DrawMenuBar(pGlobals->m_pcmainwin->m_hwndmain);
		    SetVariousStatusInfo();
		    EnableToolOnNoQuery();	

		    ::SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_CHANGEBITMAP,(WPARAM)IDM_EXECUTE,(LPARAM)3);
		    ::SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_CHANGEBITMAP,(WPARAM)ACCEL_EXECUTEALL,(LPARAM)4);
	    } 
	    else
	    {
		    pGlobals->m_pcmainwin->EnableToolButtonsAndCombo(
                                pGlobals->m_pcmainwin->m_hwndtool,
                                pGlobals->m_pcmainwin->m_hwndsecondtool,
                                pGlobals->m_pcmainwin->m_hwndtoolcombo,
                                wyFalse);

		    /* specifically change the bitmap of the correct toolbar icon and enable it */
		    if(m_execoption == SINGLE)
		    {
			    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_CHANGEBITMAP,(WPARAM)IDM_EXECUTE,(LPARAM)0);
			    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)IDM_EXECUTE,(LPARAM)TBSTATE_ENABLED);
		    } 
            else if(m_execoption == ALL)
            { 
			    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_CHANGEBITMAP,(WPARAM)ACCEL_EXECUTEALL,(LPARAM)0);
			    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ACCEL_EXECUTEALL,(LPARAM)TBSTATE_ENABLED);
		    }
	    }

        m_isactivated = wyTrue;

		//select the tab for active window
		index = pGlobals->m_pcmainwin->m_conntab->GetActiveTabIndex(pGlobals->m_pcmainwin->m_hwndconntab, hwnd);
		
		if(index > -1)
		{
			CustomTab_SetCurSel(pGlobals->m_pcmainwin->m_hwndconntab, index);
			CustomTab_EnsureVisible(pGlobals->m_pcmainwin->m_hwndconntab, index, wyTrue); 	
		}
		
		PostMessage(m_hwnd, UM_FOCUS, 0, 0);
    } 
    else
    {
        if(m_hwnd == (HWND)wparam)
        {
            if(IsChild(m_hwnd, GetFocus()))
            {
                m_lastfocus = GetFocus();
			}
        }
    }

    //if find dialog is open, close it
	if(pGlobals->m_pcmainwin->m_finddlg)
	{
		DestroyWindow(pGlobals->m_pcmainwin->m_finddlg);
		pGlobals->m_pcmainwin->m_finddlg = NULL;
	}
    return;
}

wyBool 
MDIWindow::OnInitDialog()
{
	/* now every thing is cool so we need to select the old database for the connection into this one */
	wyInt32				index;
    wyWChar			    db[512] = {0};
	wyString            query, dbstr;
	COMBOBOXEXITEM		cbi = {0};
	MYSQL_RES           *res = NULL;

	VERIFY((index = SendMessage(pGlobals->m_pcmainwin->m_hwndtoolcombo, CB_GETCURSEL, 0, 0))!= CB_ERR);	

	cbi.mask        = CBEIF_TEXT ;
	cbi.pszText     = db;
	cbi.cchTextMax  = sizeof(db)-1;
	cbi.iItem       = (wyInt32)index;

	VERIFY(SendMessage(pGlobals->m_pcmainwin->m_hwndtoolcombo, CBEM_GETITEM, 0,(LPARAM)&cbi));
    
    if(db && SendMessage(pGlobals->m_pcmainwin->m_hwndtoolcombo, CB_GETCURSEL, 0, 0) > 0)
    {
		dbstr.SetAs(db);
	    query.Sprintf("use `%s`", db);
	    res = ExecuteAndGetResult(this, m_tunnel, &m_mysql, query);
    }

	if(!res)
		return wyFalse;
	else
		mysql_free_result(res);


    return wyTrue;
}

void 
MDIWindow::OnWmCreate(HWND hwnd)
{
	EditorBase	*peditorbase;

    CreateQueryStatus(hwnd);
	CreateVSplitter(hwnd);
	CreateObjectBrowser(hwnd);
	m_acinterface->HandlerInitAutoComplete(this);
	
	CreateTabController(hwnd);  
	SetWindowPositions();
	pGlobals->m_pcmainwin->AddTextInCombo(m_database.GetAsWideChar());
	PostMessage(hwnd, UM_REFRESHOBJECT, 0, 0);

	if(m_pctabmodule->GetActiveTabEditor())
    {
        peditorbase = m_pctabmodule->GetActiveTabEditor()->m_peditorbase;
	    
        if(peditorbase)
        {
            PostMessage(peditorbase->m_hwnd, UM_FOCUS, 0, 0); 
            SetFocus(peditorbase->m_hwnd);
            ShowCursor(TRUE);
        }
    }

	return;
}

wyInt32 
MDIWindow::OnWmClose(HWND hwnd)
{
	m_ismdiclosealltabs = wyTrue;

    wyInt32     tabcount, count;
    wyString    msg;
	CTCITEM		item = {0};
	item.m_mask = CTBIF_IMAGE;
	HWND		hwndTabModule = m_pctabmodule->GetHwnd();

	if(m_keepaliveinterval)
	{
		WaitForSingleObject(m_hevtexecution, INFINITE);
		/*KillTimer(m_hwnd, KEEP_ALIVE);
		CloseHandle(m_hevtkeepalive);
		CloseHandle(m_hevtexecution);
		TerminateThread(m_hthrdkeepalive,1);
		m_hthrdkeepalive = NULL;*/
		
	}
	if(m_executing)
	{
			
		yog_message(hwnd , _(L"Could not close connection. Query(s)are being executed."), 
                           pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);
		return 0;
	}
	
	tabcount = CustomTab_GetItemCount(hwndTabModule);

	//Avoid flicker while closing all tabs
	SendMessage(m_pctabmodule->GetHwnd(), WM_SETREDRAW, FALSE, 0);

    if(pGlobals->m_pcmainwin->m_iscloseallmdi == wyTrue)
        m_mdisaveselection = pGlobals->m_pcmainwin->m_framewndsaveselection;

	for(count = 0; count < tabcount; count++)
	{
		CustomTab_SetCurSel(hwndTabModule, 0);
        CustomTab_EnsureVisible(hwndTabModule, 0, wyFalse);
		
		//To persist history/info tabs
		CustomTab_GetItem(hwndTabModule, 0, &item);

		if(!CustomTab_DeleteItem(hwndTabModule, 0))
		{			
			m_ismdiclosealltabs = wyFalse;
            pGlobals->m_pcmainwin->m_framewndsaveselection = m_mdisaveselection = -1;	
			return 0;
	    }
        else
        {
            if(pGlobals->m_pcmainwin->m_iscloseallmdi == wyTrue)
                pGlobals->m_pcmainwin->m_framewndsaveselection = m_mdisaveselection;
        }
	}

	//Paint after deleting all tabs to avoid flickering
	SendMessage(hwndTabModule, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(hwndTabModule, NULL, FALSE);
	UpdateWindow(hwndTabModule);

	m_ismdiclosealltabs = wyFalse;

	SetCursor(LoadCursor(NULL, IDC_WAIT));
	
	pGlobals->m_pcmainwin->m_connection->UpdateSqliteNodes(this);

	if(m_mysql)
	{
		if(m_keepaliveinterval)
		{
			KillTimer(m_hwnd, KEEP_ALIVE);
			CloseHandle(m_hevtkeepalive);
			CloseHandle(m_hevtexecution);
			TerminateThread(m_hthrdkeepalive,1);
			m_keepaliveinterval = 0;
		}
		m_tunnel->mysql_close(m_mysql);
	}

	if(m_conninfo.m_hprocess != INVALID_HANDLE_VALUE)
	{
		//If SSH connection then close handles at end
		if(m_conninfo.m_isssh == wyTrue)
			OnExitSSHConnection(&m_conninfo.m_sshpipeends);

		if(m_conninfo.m_sshsocket)
			closesocket(m_conninfo.m_sshsocket);
           
		m_conninfo.m_sshsocket = NULL;

		VERIFY(TerminateProcess(m_conninfo.m_hprocess, 1));
	}
	
	//delete the tab focussed if connection is closed
	if(m_iswinclosed == wyFalse)
	{
		m_iswinclosed = wyTrue;

		SendMessage(pGlobals->m_pcmainwin->m_hwndconntab, WM_SETREDRAW, (WPARAM)FALSE, 0);
		pGlobals->m_pcmainwin->m_conntab->DeleteConnectionTabItem(pGlobals->m_pcmainwin->m_hwndconntab);
		SendMessage(pGlobals->m_pcmainwin->m_hwndconntab, WM_SETREDRAW, (WPARAM)TRUE, 0);
		UpdateWindow(pGlobals->m_pcmainwin->m_hwndconntab);
	}

	pGlobals->m_conncount--;
	pGlobals->m_pcmainwin->SetConnectionNumber();
	SetChildMaximized(hwnd);//writing mdi window state to ini file
	pGlobals->m_pcmainwin->OnActiveConn();

	if(pGlobals->m_conncount == 0 && pGlobals->m_pcmainwin->m_hwndconntab)
	{
		if(pGlobals->m_pcmainwin->m_iscloseallmdi == wyTrue)
		{
			DestroyWindow(pGlobals->m_pcmainwin->m_hwndconntab);
			pGlobals->m_pcmainwin->m_hwndconntab = NULL;
		}
		else
		{
			PostMessage(pGlobals->m_pcmainwin->m_hwndmain, UM_DESTROY_CONNTAB, 0, 0);
		}
	}

    /*if(m_tunnel->IsTunnel() == false && m_keepaliveinterval)
        KillTimer(m_hwnd, KEEP_ALIVE);*/

	pGlobals->m_pcmainwin->ResizeToolBar();

	SetCursor(LoadCursor(NULL, IDC_ARROW));       
    
    return 1;    
}

wyInt32 
MDIWindow::OnWmNotify(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	LPNMHDR			lpnmhdr =(LPNMHDR)lparam;
    wyInt32			image;
    TabEditor		*ptabeditor = NULL;
    HMENU           hmenu, htrackmenu;
    POINT           pnt;
    TabTypes*       ptab;
    TabQueryTypes*  ptabqtype;
	
	switch(lpnmhdr->code)
	{
    case TVN_ITEMEXPANDING:
		return m_pcqueryobject->OnItemExpandingHelper((LPNMTREEVIEW)lparam, this);
	
	case TVN_ITEMEXPANDED:
		m_pcqueryobject->OnItemExpanded(&((LPNMTREEVIEW)lparam)->itemNew );
		break;
			
	case TVN_BEGINLABELEDIT:
		image = m_pcqueryobject->GetSelectionImage();
		
		if(image != NTABLE && image != NVIEWSITEM && image != NSPITEM && 
            image != NEVENTITEM &&	image != NFUNCITEM && image != NTRIGGERITEM)
			return 1;

        if(!m_pcqueryobject->m_AllowRename)
            return 1;

        break;
    case TVN_SELCHANGING:
        return m_pcqueryobject->OnItemSelectionChanging((LPNMTREEVIEW)lparam);
        break;
	case TVN_SELCHANGED:
        /// Shutting down the filtering option && sorting .
        ptabeditor = GetActiveTabEditor();
        m_pcqueryobject->OnSelChanged(((LPNMTREEVIEW)lparam)->itemNew.hItem, (LPNMTREEVIEW)lparam);

        if(((LPNMTREEVIEW)lparam)->action == TVC_UNKNOWN)
            SetFocus(m_pcqueryobject->m_hwndFilter);
        else
            PostMessage(m_pcqueryobject->m_hwnd, UM_FOCUS, 0, 0);

		break;

	// the following things works in a little abusurd manner.
	// if the user has returned without doing anhthing then we return without doing anything.
	// if the renaming was done properly then we change the label value and return.
	// if any error occurs then we post a message to again edit it and return for thet timebeing.
		// this gives the user an opportunity to edit it again.
	case TVN_ENDLABELEDIT:
		return OnTvnEndLabelEdit(lparam);

		//for Dragging item from ObjectBrowser
	case TVN_BEGINDRAG:
        ptabeditor = GetActiveTabEditor();
        if(!ptabeditor)
        {
		    m_dragged = wyTrue;
		    m_pcqueryobject->m_dragged = wyTrue;           
        }		
		break;

        // Tab is going to change
    case CTCN_SELCHANGING:
        
		if(m_executing == wyTrue /*|| m_pingexecuting == wyTrue*/)
        {
            FrameWindow::ShowQueryExecToolTip(wyTrue);
			return 0;
        }

        FrameWindow::ShowQueryExecToolTip(wyFalse);
		OnCtcnSelChanging(hwnd, lpnmhdr->idFrom); // before event
        return 1;
		break;
	
        // tab changed
	case CTCN_SELCHANGE:
		if(m_executing == wyTrue /*|| m_pingexecuting == wyTrue*/)
			break;

		OnCtcnSelChange(hwnd, lpnmhdr->idFrom, lparam); // After event
		break;
	
	case CTCN_TABCLOSING:
		return OnCtcnTabClosing(hwnd, (LPNMCTC)lparam);

	case CTCN_TABCLOSED:
        //OnCtcnTabClose(hwnd, (LPNMCTC)lparam);

        if(lpnmhdr->idFrom == IDC_CTAB)
        {
            ((LPNMCTC)lparam)->retvalue = 1;
        }

		break;	

	case CTCN_WMDESTROY:
		break;

	case CTCN_LBUTTONDBLCLK:
		if(m_executing == wyTrue /*|| m_pingexecuting == wyTrue*/)
        {
            FrameWindow::ShowQueryExecToolTip();
        }
        else if(lpnmhdr->idFrom == IDC_CTAB)
		{
			//open query tab on double click on empty space
			pGlobals->m_pcmainwin->CreateNewQueryEditor(GetActiveWin());
		}
		break;
	
    case CTCN_GETCHILDWINDOWS:
        OnCtcnGetChildWindows(hwnd, lparam);
        break;

    case CTCN_PLUSBUTTONCLICK:
        LoadQueryTabPlusMenu(lparam);
        break;

    case CTCN_ONCONTEXTMENU:
        if(lpnmhdr->idFrom == IDC_CTAB && CustomTab_GetItemCount(lpnmhdr->hwndFrom) > 1)
        {
            hmenu = LoadMenu(pGlobals->m_hinstance, MAKEINTRESOURCE(IDR_TABMENU));
            LocalizeMenu(hmenu);
	        htrackmenu = GetSubMenu(hmenu, 0);
            wyTheme::SetMenuItemOwnerDraw(htrackmenu);
            GetCursorPos(&pnt);
            TrackPopupMenu(htrackmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt.x, pnt.y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL);
            FreeMenuOwnerDrawItem(htrackmenu);
            DestroyMenu(hmenu);
        }
        break;
        
    case CTCN_PAINTTIMERSTART:
        if(lpnmhdr->idFrom == IDC_CTAB)
        {
            if((ptab = m_pctabmodule->GetActiveTabType()))
            {
                ptab->SetBufferedDrawing(wyTrue);
            }
        }
        else
        {
            ptabeditor = m_pctabmodule->GetActiveTabEditor();

            if(ptabeditor && ptabeditor->m_pctabmgmt && (ptabqtype = ptabeditor->m_pctabmgmt->GetActiveTabType()))
            {
                ptabqtype->SetBufferedDrawing(wyTrue);
            }
        }

        CustomTab_SetBufferedDrawing(wyTrue);
        break;

    case CTCN_PAINTTIMEREND:
        CustomTab_SetBufferedDrawing(wyFalse);

        if(lpnmhdr->idFrom == IDC_CTAB)
        {
            if((ptab = m_pctabmodule->GetActiveTabType()))
            {
                ptab->SetBufferedDrawing(wyFalse);
            }
        }
        else
        {
            ptabeditor = m_pctabmodule->GetActiveTabEditor();

            if(ptabeditor && ptabeditor->m_pctabmgmt && (ptabqtype = ptabeditor->m_pctabmgmt->GetActiveTabType()))
            {
                ptabqtype->SetBufferedDrawing(wyFalse);
            }
        }
        break;
	
	case NM_RCLICK:
		SendMessage(m_pcqueryobject->m_hwnd, WM_CUSTCONTEXTMENU, 0, GetMessagePos()); 
		break;
	}
    return 0;
}

wyInt32 
MDIWindow::OnTvnEndLabelEdit(LPARAM lparam)
{
	wyBool ret;

	LPNMTVDISPINFO ptvdi =(LPNMTVDISPINFO)lparam;
	
	if(ptvdi->item.pszText == NULL)
		return wyTrue;

	ret = m_pcqueryobject->EndRename(ptvdi);

	if(ret == wyFalse)
	{
		PostMessage(m_pcqueryobject->m_hwnd, UM_REEDITLABEL, 0, (LPARAM)ptvdi->item.hItem);
		return 0;
	}

#ifndef COMMUNITY
	if(m_pctabmodule->GetActiveTabImage() == IDI_DATASEARCH)
	{
		TabDbSearch *tabsearch = NULL; 

		tabsearch = dynamic_cast<TabDbSearch*>(m_pctabmodule->GetActiveTabType());
		
        if(tabsearch)
		{
			tabsearch->SetSearchScope();
		    tabsearch->EnableDisableSearchButton();
		}
	}
#endif
	return 1;
}

void 
MDIWindow::OnCtcnSelChanging(HWND hwnd, wyInt32 tabid)
{
	TabTypes	*ptabtypes = NULL;
    	
	if(tabid == IDC_CTAB && (ptabtypes = m_pctabmodule->GetActiveTabType()))
	{
		ptabtypes->OnTabSelChanging();
	}

    return;   
}

void 
MDIWindow::OnCtcnGetChildWindows(HWND hwnd, LPARAM lparam)
{
    TabTypes	*ptabtypes = NULL;
	wyInt32		id;
    NMCTC*      pnmctc = (NMCTC*)lparam;
	
    if(pnmctc && pnmctc->hdr.idFrom == IDC_CTAB)
	{	
		id = CustomTab_GetCurSel(GetDlgItem(hwnd, pnmctc->hdr.idFrom));      
        ptabtypes = m_pctabmodule->GetActiveTabType();
        ptabtypes->OnGetChildWindows(id, lparam);
	}
}

void 
MDIWindow::OnCtcnSelChange(HWND hwnd, wyInt32 tabid, LPARAM lparam)
{
    TabMgmt		*ptabmgmt;
	TabTypes	*ptabtypes = NULL, *pprevtabtypes = NULL;
	wyInt32		image, curtab;
    LPNMCTC     lpnmctc = (LPNMCTC)lparam;
    CTCITEM     ctci = {0};
    HWND        hwndtab = GetDlgItem(hwnd, tabid);

    if(tabid == IDC_CTAB)
	{
        ctci.m_mask = CTBIF_LPARAM;
        curtab = CustomTab_GetCurSel(hwndtab);
        CustomTab_GetItem(hwndtab, curtab, &ctci);
        ptabtypes = (TabTypes*)ctci.m_lparam;

        if(lpnmctc && lpnmctc->count >= 0)
        {
            CustomTab_GetItem(lpnmctc->hdr.hwndFrom, lpnmctc->count, &ctci);
            pprevtabtypes = (TabTypes*)ctci.m_lparam;
            pprevtabtypes->ShowTabContent(curtab, wyFalse);
        }

        m_pcquerystatus->AddNumRows(0, wyTrue);
        m_pcquerystatus->AddTickCount(m_tunnel, 0, 0, wyTrue);

        if(m_pctabmodule)
        {
            image = m_pctabmodule->GetActiveTabImage();
            
            if(image == IDI_CREATETABLE || tabid == IDI_ALTERTABLE)
            {
                pGlobals->m_pcmainwin->m_htabinterface = ptabtypes->m_hwnd;
            }
            else
            {
                pGlobals->m_pcmainwin->m_htabinterface = NULL;
            }
        }
        
        if(lpnmctc)
        {
            ptabtypes->ShowTabContent(curtab, wyTrue);
        }
        
        ptabtypes->OnTabSelChange();
	}
	else if(tabid == IDC_QUERYTAB)
	{
        ptabmgmt = lpnmctc ? (TabMgmt*)CustomTab_GetLongData(lpnmctc->hdr.hwndFrom) :
            (m_pctabmodule->GetActiveTabEditor() ? m_pctabmodule->GetActiveTabEditor()->m_pctabmgmt : NULL);

        if(ptabmgmt)
        {
		    ptabmgmt->OnTabSelChange(lpnmctc);
        }
	}
    
    return;
}

wyInt32 
MDIWindow::OnCtcnTabClosing(HWND hwnd, LPNMCTC lpnmctc)
{
    wyInt32     ret = 0;
	TabTypes*   ptabtypes = NULL;
    CTCITEM     item;

    if(lpnmctc->hdr.idFrom == IDC_CTAB && lpnmctc->count != -1)
	{
        item.m_mask = CTBIF_IMAGE | CTBIF_LPARAM;
        CustomTab_GetItem(lpnmctc->hdr.hwndFrom, lpnmctc->count, &item);
        ptabtypes = (TabTypes*)item.m_lparam;
        ret = ptabtypes->OnTabClosing(lpnmctc->lparam ? wyTrue : wyFalse);

		if(ret)
        {
            ptabtypes->CloseTab(lpnmctc->count);

            if(item.m_iimage != IDI_HISTORY)
            {
                delete ptabtypes;
            }

            return ret;
        }		
	}
	
	else if(lpnmctc->hdr.idFrom == IDC_QUERYTAB)
	{
		return 1;
	}
    
    return 0;
}

void 
MDIWindow::OnCtcnTabClose(HWND hwnd, LPNMCTC lpnmctc)
{
	TabTypes*   ptabtypes = NULL;
    CTCITEM     item;

    if(lpnmctc->hdr.idFrom == IDC_CTAB && lpnmctc->count != -1)
	{
        item.m_mask = CTBIF_LPARAM;
        CustomTab_GetItem(lpnmctc->hdr.hwndFrom, lpnmctc->count, &item);
        ptabtypes = (TabTypes*)item.m_lparam;
		ptabtypes->OnTabClose();
	}
}

void
MDIWindow::OnCtnLButtonUp(HWND hwnd, wyInt32 tabid, LPARAM Position)
{
#ifndef COMMUNITY
	wyInt32		    id;
	TabQueryBuilder *ptabqb;
	MDIWindow		*wnd;
	CTCITEM         item;

	item.m_mask = CTBIF_IMAGE | CTBIF_LPARAM;
	
	VERIFY(wnd = GetActiveWin());

	if(tabid == IDC_CTAB) 
	{
		id = CustomTab_GetCurSel(GetDlgItem(hwnd,  tabid));
		CustomTab_GetItem(GetDlgItem(hwnd, tabid), id, &item);
	
		if(item.m_iimage == IDI_QUERYBUILDER_16)
		{
			ptabqb = (TabQueryBuilder*)m_pctabmodule->GetActiveTabType();

			if(wnd->m_dragged == wyTrue)
			    ptabqb->DropTable(hwnd, Position);
		}
	}

#endif
    return;
	
}

void 
MDIWindow::OnMouseMove()
{
	/* change the cursor to hour glass if its executing */
	MDIWindow		*q;

	VERIFY(q = GetActiveWin());

	if(q && (q->m_executing ))
		SetCursor(LoadCursor(NULL, IDC_WAIT));

    if(q && q->m_dragged == wyTrue)
        SetCursor(LoadCursor(NULL, IDC_NO));

	return;    
}

void
MDIWindow::OnRefreshObject()
{
    SendMessage(m_pcqueryobject->m_hwnd, WM_SETREDRAW, FALSE, 0);
    m_pcqueryobject->RefreshObjectBrowser(m_tunnel, &m_mysql);
    SendMessage(m_pcqueryobject->m_hwnd, WM_SETREDRAW, TRUE, 0);
	m_statusbartext.SetAs(_(L" Objectbrowser refreshed..."));
    pGlobals->m_pcmainwin->AddTextInStatusBar(_(L" Objectbrowser refreshed..."));
    HandleToolBar();
    return;
}

void
MDIWindow::OnUMFocus()
{
    if(m_lastfocus)
         SetFocus(m_lastfocus);
    else
    {
        //if(CustomTab_GetItemCount(m_pctabmodule->GetActiveTabEditor()->m_pctabmgmt->m_hwnd) != -1)
        //{
        //    m_pctabmodule->GetActiveTabEditor()->m_pctabmgmt->OnTabSelChange();
        //    //CustomTab_SetCurSel(m_pctabmodule->GetActiveTabEditor()->m_pctabmgmt->m_hwnd, 0);
        //}
        SetFocus(m_pcqueryobject->m_hwnd);
    }

    return;
}


// Creattion of various child windows.
// We keep a pointer to all these classes as member variables.
void
MDIWindow::CreateObjectBrowser(HWND hwnd)
{
	m_pcqueryobject = new CQueryObject(hwnd,(CHAR*)m_filterdb.GetString());
	m_pcqueryobject->Create();

	return;
}

void
MDIWindow::CreateTabController(HWND hwnd)
{	
	GetLimitValues();
    IsResultTabPagingEnabled();
    GetTabPositions();

    m_pctabmodule = new TabModule(hwnd);
	m_pctabmodule->SetParentPtr(this);
	m_pctabmodule->Create(this);
	
	return;
}

void
MDIWindow::CreateQueryStatus(HWND hwnd)
{
    m_pcquerystatus = pGlobals->m_pcmainwin->m_statusbarmgmt;
	return;
}

void
MDIWindow::CreateVSplitter(HWND hwnd)
{
	if(pGlobals->m_pcmainwin->m_leftpercent == 0)
	{
		m_pcqueryvsplitter	= new FrameWindowSplitter(hwnd, MDI_VERTICAL_POS);
		m_pcqueryvsplitter->Create();
		m_pcqueryvsplitter->m_lastleftpercent = MDI_VERTICAL_POS;
	}
	else
	{
		m_pcqueryvsplitter	= new FrameWindowSplitter(hwnd, pGlobals->m_pcmainwin->m_leftpercent);
		m_pcqueryvsplitter->Create();
		m_pcqueryvsplitter->m_lastleftpercent = pGlobals->m_pcmainwin->m_leftpercent;
	}

return;
}

const wyChar *
MDIWindow::GetMySQLHost()
{
	if(m_tunnel->IsTunnel())
		return m_tunnel->GetHost();
	else
		return m_mysql->host;
}

const wyChar *
MDIWindow::GetMySQLUser()
{
	if(m_tunnel->IsTunnel())
		return m_tunnel->GetUser();
	else
		return m_mysql->user;
}

FrameWindowSplitter*			
MDIWindow::GetVSplitter()
{
	return m_pcqueryvsplitter;
}

StatusBarMgmt*
MDIWindow::GetStatus()
{
	return m_pcquerystatus;
}

/* enables disables all the window on start/end of execution */
void
MDIWindow::EnableWindowOnQueryExecution(wyBool enable)
{
	CTCITEM		item;
   	TabEditor*  te; 
	BOOL		status;
    HWND        hwndtab;
    wyInt32     i;

	status = (enable == wyTrue) ? TRUE : FALSE;
    hwndtab = m_pctabmodule->GetHwnd();

    item.m_mask = CTBIF_LPARAM;
    //CustomTab_BlockEvents(hwndtab, enable == wyTrue ? wyFalse : wyTrue);
    i = CustomTab_GetCurSel(m_pctabmodule->GetHwnd());
    CustomTab_GetItem(hwndtab, i, &item);
    te = (TabEditor *)item.m_lparam;

    if(te && te->m_pctabmgmt)
    {
        EnableWindow(te->m_pctabmgmt->m_hwnd, status);
    }
    
   /* if(te->m_pctabmgmt)
    {
        for(count = CustomTab_GetItemCount(te->m_pctabmgmt->m_hwnd), j = 0; j < count; ++j)
        {
            item.m_mask = CTBIF_IMAGE | CTBIF_LPARAM;
            CustomTab_GetItem(te->m_pctabmgmt->m_hwnd, j, &item);

            if(item.m_iimage == IDI_QUERYRECORD)
            {
                ptr = (CDataViewQuery*)item.m_lparam;
                
                if(enable == wyFalse)
                {
                    ptr->StartExecute(this);
                }
                else
                {
                    ptr->EndExecute(this);
                }
            }
            else if(item.m_iimage == IDI_TABLE)
            {
                if(enable == wyFalse)
                {
                    te->m_pctabmgmt->m_insert->StartExecute();
                }
                else
                {
                    te->m_pctabmgmt->m_insert->EndExecute();
                }
            }
        }
    }*/

	EnableWindow(m_pcqueryobject->m_hwnd, status);

    if(enable == wyFalse)
    {
        CustomTab_StartIconAnimation(hwndtab, i, 100);
    }
    else
    {
        CustomTab_StopIconAnimation(hwndtab, i);
    }

    if(te->m_peditorbase)
    {
        SendMessage(te->m_peditorbase->m_hwnd, SCI_SETREADONLY, enable == wyFalse ? 1 : 0, 0);
    }
	
	return;
}

void
MDIWindow::EnableToolOnNoQuery()
{
	wyInt32			image, size, count;
	wyInt32			nid[] = {ID_OPEN_COPYDATABASE, ID_OBJECT_CREATESCHEMA,  ID_EXPORT_AS, 
                             ID_OBJECT_COPYTABLE, ID_OBJECT_INSERTUPDATE, ID_OBJECT_TABLEEDITOR, 
                             ID_OBJECT_MAINMANINDEX, ACCEL_MANREL };
	wyInt32			nidd[] = {ID_EXPORT_AS, ID_OBJECT_COPYTABLE, ID_OBJECT_INSERTUPDATE, 
                              ID_OBJECT_TABLEEDITOR, ID_OBJECT_MAINMANINDEX, ACCEL_MANREL };
    wyInt32			viewdata[] = {ID_OBJECT_VIEWDATA, ID_TABLE_OPENINNEWTAB};

	pGlobals->m_pcmainwin->EnableToolButtonsAndCombo(
                                pGlobals->m_pcmainwin->m_hwndtool,
                                pGlobals->m_pcmainwin->m_hwndsecondtool,
                                pGlobals->m_pcmainwin->m_hwndtoolcombo, wyTrue);

	image = m_pcqueryobject->GetSelectionImage();

	size = sizeof(nid)/ sizeof(nid[0]);

	// first enable all the buttons.
	/*for(count = 0; count < size; count++)
		SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)nid[count], TBSTATE_ENABLED);*/
	
	if(image == NSERVER)
    {
		for(count = 0; count < size; count++)
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)nid[count], TBSTATE_INDETERMINATE);

		// disable the view data as it is different
		SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[0], TBSTATE_INDETERMINATE);
        SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[1], TBSTATE_INDETERMINATE);
		
	}
    else if(image == NDATABASE)
    {
		for(count = 0; count < (sizeof(nidd)/ sizeof(nidd[0])); count++)
			SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)nidd[count], TBSTATE_INDETERMINATE);

		// disable the view data as it is different
		SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[0], TBSTATE_INDETERMINATE);
        SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)viewdata[1], TBSTATE_INDETERMINATE);
	}	

	return;
}

//Sets the community tab header title text color and background.
wyInt32
MDIWindow::OnDlgProcColorStatic(WPARAM wparam, LPARAM lparam)
{
    HDC			hdc = NULL;
#ifdef COMMUNITY
	wyInt32		identifier = 0;
	COLORREF	color, clr;
    HBRUSH      hbr;
#endif
    hdc = (HDC)wparam;
	
#ifdef COMMUNITY	
	
    identifier = GetDlgCtrlID((HWND)lparam);
	color = GetSysColor(COLOR_BTNFACE);
	
	if(identifier == IDC_COMMTITLE)
	{				
		SetBkMode(hdc, TRANSPARENT);
        clr = RGB_DARK_BLUE;
        wyTheme::GetLinkColor(LINK_COLOR_MDICHILD, &clr);
        SetTextColor(hdc, clr);

        if(wyTheme::GetBrush(BRUSH_MDICHILD, &hbr))
        {
            return (wyInt32)hbr;
        }
        else
        {
            SetDCBrushColor(hdc, color);
		    return (wyInt32)GetStockObject(DC_BRUSH);
        }
	}
#endif

    if((HWND)lparam == m_pcqueryobject->m_hwndStParent)
    {
		COLORREF bkcolor = TreeView_GetBkColor(m_pcqueryobject->m_hwnd);

        SetBkMode(hdc, OPAQUE);
		SetBkColor(hdc, bkcolor);
        SetDCBrushColor(hdc, bkcolor);
        SetTextColor(hdc, TreeView_GetTextColor(m_pcqueryobject->m_hwnd));
        return (INT_PTR)CreateSolidBrush(bkcolor);//(wyInt32)GetStockObject(DC_BRUSH);
        
        /*if(wyTheme::GetBrush(BRUSH_MDICHILD, &hbr))
        {
            return (wyInt32)hbr;
        }
        else
        {
            return (wyInt32)GetSysColorBrush(COLOR_BTNFACE);
        }*/
    }

	return 1;
}

wyBool
MDIWindow::StopQuery()
{
	/* we basically create another connection and if successful then we kill the current MYSQL connection 
	   and replace the current mysql pointer with this one */
	/* this only works when we have direct or SSH tunneled connection so we can safely use global mysql functions */
	MYSQL		*mysql, *temp;
    wyWChar		curdb[SIZE_512] = {0};
	wyString	currentdatabase;

    pGlobals->m_pcmainwin->GetCurrentSelDB(curdb, SIZE_512 - 1);
	
	currentdatabase.SetAs(curdb);

	VERIFY(m_executing);
	
	temp = mysql = m_tunnel->mysql_init((MYSQL*)0);

    EnterCriticalSection(&m_cs);	
		
	SetMySQLOptions(&m_conninfo, m_tunnel, &mysql);

	if(!m_tunnel->mysql_real_connect(mysql, m_mysql->host, m_mysql->user, m_mysql->passwd, NULL, 
                           m_mysql->port, NULL, CLIENT_MULTI_RESULTS | CLIENT_REMEMBER_OPTIONS, NULL))
    {
		ShowMySQLError(m_hwnd, m_tunnel, &temp, NULL, wyTrue);
		m_tunnel->mysql_close(temp);
		SetCursor(LoadCursor(NULL, IDC_ARROW));
        LeaveCriticalSection(&m_cs);
		return wyFalse;
	}
	
	UseDatabase(currentdatabase, mysql, m_tunnel);
	
	
    if(m_tunnel->IsTunnel()&& !m_tunnel->CheckCorrectTunnelVersion(mysql))
    {
        ShowMySQLError(m_hwnd, m_tunnel, &temp, NULL, wyTrue);
		m_tunnel->mysql_close(temp);
		SetCursor(LoadCursor(NULL, IDC_ARROW));
        LeaveCriticalSection(&m_cs);
		return wyFalse;
	}

	StopQueryExecution();

	/* connection successful so now we need to kill the pid */
	//DEBUG_LOG("StopQuery::mysql_kill");

	if(mysql_kill(mysql, m_mysql->thread_id))
    {
		ShowMySQLError(m_hwnd, m_tunnel, &mysql, NULL, wyTrue);
		m_tunnel->mysql_close(temp);
        LeaveCriticalSection(&m_cs);
		return wyFalse;
	}

	/* close the current connection and change the pointer to the new one */
	//DEBUG_LOG("StopQuery::mysql_close");
	//sudhi
	if(m_mysql)
		mysql_close(m_mysql);
	//sudhi
	m_mysql = mysql;

	//set the SQl_MODE = ''
	if(IsMySQL5010(m_tunnel, &m_mysql))
		SetDefaultSqlMode(m_tunnel, &m_mysql, wyFalse);

	//m_executing = wyFalse;

    LeaveCriticalSection(&m_cs);

	return wyTrue;
}

// After creating vrious windows we set the Window Postion, so that no window overlap other window.
void
MDIWindow::SetWindowPositions()
{
	VERIFY(SetWindowPos(m_pcqueryobject->GetHwnd(), HWND_TOPMOST, 0,0,0,0, SWP_NOSIZE | SWP_NOMOVE));
	VERIFY(SetWindowPos(m_pcqueryvsplitter->GetHwnd(), HWND_TOPMOST, 0,0,0,0, SWP_NOSIZE | SWP_NOMOVE));
	
	VERIFY(SetWindowPos(m_pctabmodule->GetHwnd(), HWND_TOPMOST, 0,0,0,0, SWP_NOSIZE | SWP_NOMOVE));  
	return;
}

// This function makes the title dirty after every change i.e it adds a asterix at the end to show 
// the content of the edit box has change.
wyBool
MDIWindow::SetDirtyTitle()
{
	wyInt32			len, tabimageid;
	wyWChar          title[MAX_PATH + 10];
         EditorBase		*peditorbase;

	tabimageid = m_pctabmodule->GetActiveTabImage();
  
	len = GetWindowText(m_hwnd, title, MAX_PATH);
	_ASSERT(len != 0);

	if(title[len-1] != '*')
	{
		title[len] = '*';
		title[len+1] = 0;
		SetWindowText(m_hwnd, title);

		// For Query Editor
		if(tabimageid != IDI_QUERYBUILDER_16 && tabimageid != IDI_SCHEMADESIGNER_16)
		{
			peditorbase =	m_pctabmodule->GetActiveTabType()->m_peditorbase;
			peditorbase->m_edit = wyTrue;                 
		}

		SetQueryWindowTitle();
	}

	return wyTrue;
}

// Resizes the window.
// Basically it calls the resize function of all its child window in orderly manner to implement
// change of size of all its child window.
void
MDIWindow::Resize()
{
	m_pcqueryvsplitter->Resize();

	m_pctabmodule->Resize();	
	m_pcqueryobject->Resize();

	return;
}

//handles file save command
wyBool
MDIWindow::HandleFileSave()
{
	TabEditor			*ptabeditor;
	wyInt32		tabimageid, sel;
    wyBool      retval = wyFalse;

	tabimageid = m_pctabmodule->GetActiveTabImage();
	switch(tabimageid)
	{
		case IDI_SCHEMADESIGNER_16:
			{
			#ifndef COMMUNITY
			TabSchemaDesigner	*ptabsd;
			ptabsd = (TabSchemaDesigner*)m_pctabmodule->GetActiveTabType();
			if(ptabsd)
				retval = ptabsd->WriteToXMLfile();			
			#endif
			break;
			}
			

	case IDI_QUERYBUILDER_16:
		{
			#ifndef COMMUNITY
            TabQueryBuilder     *ptabqb;
            ptabqb = (TabQueryBuilder*)m_pctabmodule->GetActiveTabType();
            
            if(ptabqb)
                retval = ptabqb->WriteToXMLfile(wyFalse);
            #endif
		    break;
        }

	case IDI_DATASEARCH:
		break;

	case IDI_HISTORY:
		TabHistory *ptabhist;
		ptabhist = m_pctabmodule->GetActiveHistoryTab();
		if(ptabhist)
			TabHistory::SaveHistory(ptabhist->m_hwnd);
		break;

	default:        
		{
			ptabeditor = GetActiveTabEditor();
			
            if(ptabeditor)
				retval = SaveFile();
			
            break;
		}		
	}

    //make the tab visible
    if(retval == wyTrue)
    {
        sel = CustomTab_GetCurSel(m_pctabmodule->m_hwnd);

        if(sel != -1)
            CustomTab_EnsureVisible(m_pctabmodule->m_hwnd, sel, wyFalse);
    }

	return retval;
}

// This function implements the save file option. you can save your sql queries in a sql file 
// for later use.
wyBool
MDIWindow::SaveFile(wyBool istabclose)
{
	wyBool		ret;
	HCURSOR		hcursor;
    wyWChar     filename[MAX_PATH+1]={0};
	wyString	filenamestr;
    EditorBase	*peditorbase;
	
	//The save file only for QueryEditor , not for QB
    if(GetActiveTabEditor() == NULL)
        return wyFalse;
	
	hcursor = GetCursor();

	SetCursor(LoadCursor(NULL, IDC_WAIT));
	ShowCursor(1);
	
    peditorbase = GetActiveTabEditor()->m_peditorbase;
	
	if(peditorbase->m_filename.GetLength() == 0)				
	{
		if(InitFile(m_hwnd, filename, SQLINDEX, MAX_PATH))
		{
			filenamestr.SetAs(filename);

			ret = WriteSQLToFile(peditorbase->m_hwnd, filenamestr, istabclose);

			if(ret)
			{
				peditorbase->m_filename.SetAs(filenamestr.GetString());
				peditorbase->m_isfilesave = wyTrue;
				peditorbase->m_edit = wyFalse;
				peditorbase->m_save = wyTrue;
                SendMessage(peditorbase->m_hwnd, SCI_SETSAVEPOINT, 0, 0);

				pGlobals->m_pcmainwin->WriteLatestFile((wyWChar*)peditorbase->m_filename.GetAsWideChar());
				
				m_pctabmodule->Resize();			
				
				SetQueryWindowTitle();
				peditorbase->m_isfilesave = wyFalse;
			}
			else
				return wyFalse;
		}
		else 
			return wyFalse;
	} 
    else  
    {
		ret = WriteSQLToFile(peditorbase->m_hwnd,peditorbase->m_filename, istabclose);

		if(ret)
		{
			peditorbase->m_isfilesave = wyTrue;
			peditorbase->m_edit = wyFalse;
			peditorbase->m_save = wyTrue;
            SendMessage(peditorbase->m_hwnd, SCI_SETSAVEPOINT, 0, 0);
            
			SetQueryWindowTitle();

			peditorbase->m_isfilesave = wyTrue;
		}
		else

			//If error occures while saving file then close action is cancelled
			return wyFalse;
	}

	SetCursor(hcursor);
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	ShowCursor(1);
	
	return wyTrue;
}

wyBool				
MDIWindow::HandleSaveAsFile()
{
	TabEditor	*ptabeditor;
	wyInt32		tabimageid, sel;
    wyBool      retval = wyFalse;

	tabimageid = m_pctabmodule->GetActiveTabImage();
	switch(tabimageid)
	{
	case IDI_SCHEMADESIGNER_16:
		{
			#ifndef COMMUNITY
			TabSchemaDesigner	*ptabsd;
			ptabsd = (TabSchemaDesigner*)m_pctabmodule->GetActiveTabType();
			
            if(ptabsd)
				retval = ptabsd->WriteToXMLfile(wyTrue);
			#endif
            break;
		}


	case IDI_QUERYBUILDER_16:
        {
			#ifndef COMMUNITY
            TabQueryBuilder     *ptabqb;
            ptabqb = (TabQueryBuilder*)m_pctabmodule->GetActiveTabType();
            
            if(ptabqb)
                retval = ptabqb->WriteToXMLfile(wyTrue);
            #endif
            break;
        }

	case IDI_DATASEARCH:
		break;

	case IDI_HISTORY:
		TabHistory *ptabhist;
		ptabhist = m_pctabmodule->GetActiveHistoryTab();
		if(ptabhist)
			TabHistory::SaveHistory(ptabhist->m_hwnd);
		break;
		
	default:        
		{
			ptabeditor = GetActiveTabEditor();

			if(ptabeditor)
				 retval = SaveAsFile();				

			break;
		}		
	}

    //make the tab visible
    if(retval == wyTrue)
    {
        sel = CustomTab_GetCurSel(m_pctabmodule->m_hwnd);

        if(sel != -1)
            CustomTab_EnsureVisible(m_pctabmodule->m_hwnd, sel, wyFalse);
    }

	return retval;
}


// This function implements if the user wants to save the current file in a differet file name.
// This function works in the same as SaveFile if the user has not saved the file before and 
// selects this option.
wyBool
MDIWindow::SaveAsFile()
{
	wyBool	    ret;
	HCURSOR		hcursor;
    wyWChar     filename[MAX_PATH+1]={0};
    EditorBase  *peditorbase;
	wyString     tempfilename;
       	
    hcursor = GetCursor();

	SetCursor(LoadCursor(NULL, IDC_WAIT));
	ShowCursor(1);
	
	peditorbase = m_pctabmodule->GetActiveTabType()->m_peditorbase;
	
	if(InitFile(m_hwnd,filename, SQLINDEX, MAX_PATH))
	{
       	//Saving the fileneme in local variable
		tempfilename.SetAs(filename);
		
		ret = WriteSQLToFile(peditorbase->m_hwnd, tempfilename);
		
		if(ret)
		{
			peditorbase->m_filename.SetAs(tempfilename);
			
			peditorbase->m_isfilesave = wyTrue;
			peditorbase->m_edit = wyFalse;
			peditorbase->m_save = wyTrue;
            SendMessage(peditorbase->m_hwnd, SCI_SETSAVEPOINT, 0, 0);
			pGlobals->m_pcmainwin->WriteLatestFile((wyWChar*)peditorbase->m_filename.GetAsWideChar());
			
			m_pctabmodule->Resize();

			SetQueryWindowTitle();

			peditorbase->m_isfilesave = wyFalse;
		}
		else
			return wyFalse;
	}
	SetCursor(hcursor);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	ShowCursor(1);
	
	return wyTrue;
}

//When a file to be opened in SQLyog tab
wyBool
MDIWindow::HandleFileOpen(wyInt32 filetype, wyBool issametab)
{
	wyInt32			ret = 0;
    wyBool          retval = wyFalse;
	wyWChar			openfilename[MAX_PATH + 1] = {0};
	wyString		filename;
			
	//Get File 'open' dialog
	ret = InitOpenFile(m_hwnd, openfilename, filetype, MAX_PATH);

	if(!ret)
		return wyFalse;
	
	filename.SetAs(openfilename);

	retval = OpenFileinTab(&filename, issametab);

	return retval;	
}

/*
-Open file
-SQLyof File types(.sql, .schemaxml, .queryxml)
-issametab : This set wyTrue when use context menu of each tab type
- isrecentfiles: This sets wyTrue when used 'Recent Files' menu
*/
wyBool
MDIWindow::OpenFileinTab(wyString *filename, wyBool issametab, wyBool isrecentfiles)
{
	wyBool			retval = wyFalse;
	wyInt32			sel = 0;
	SQLyogFileType  filetype;
    
	filetype = OpenFileType(filename);
		
	switch(filetype)
	{	
		case SDFILE:
			{			
				retval = OpenSchemaFile(filename, issametab, isrecentfiles);
				break;
			}
		
		case QBFILE:
			{			
				retval = OpenQBFile(filename, issametab, isrecentfiles); 
				break;
			}
			break;

		case FILEINVALID:
			return wyFalse;

		default:        
			{
				retval = OpenSQLFile(filename, issametab, isrecentfiles);
				break;
			}		
	}

	//make the tab visible
	if(retval == wyTrue)
	{
		sel = CustomTab_GetCurSel(m_pctabmodule->m_hwnd);

		if(sel != -1)
			CustomTab_EnsureVisible(m_pctabmodule->m_hwnd, sel, wyFalse);
	}
	
	
    return retval;

}

// This function opens the Schema file asked by user and copies its content in the schema designer.
wyBool
MDIWindow::OpenSchemaFile(wyString *filename, wyBool issametab, wyBool isrecentfiles)
{
#ifndef COMMUNITY

	wyString		    fname;
	TiXmlDocument		*doc;
	wyString			jobbuff;
	TabSchemaDesigner	*ptabsd;
    MDIWindow*          wnd;
	wyBool				ret = wyTrue;
	wyInt32				retmsg = 0, img = 0;

    VERIFY(wnd = GetActiveWin());

    if(!wnd)
        return wyFalse;

	if(!filename || !filename->GetLength())
		return wyFalse;

	// Gets the white space
	TiXmlBase::SetCondenseWhiteSpace(false);
	
	//To open file in new schema designer
	if(issametab == wyFalse)
	{
		m_pctabmodule->CreateSchemaDesigner(this);
		ptabsd = (TabSchemaDesigner*)m_pctabmodule->GetActiveTabType();

		if(!ptabsd)
			return wyFalse;
	}

	else
	{	
		img = m_pctabmodule->GetActiveTabImage();
		if(img != IDI_SCHEMADESIGNER_16)
			m_pctabmodule->CreateSchemaDesigner(this);
			//return wyFalse;

		ptabsd = (TabSchemaDesigner*)m_pctabmodule->GetActiveTabType();

		if(!ptabsd)
			return wyFalse;

		retmsg = ptabsd->OnTabClosing(wyFalse);
				
		if(!retmsg)
			return wyFalse;

        //clear SD
        ptabsd->ClearSD();
	}
	
		
	doc = new TiXmlDocument();
		
	fname.SetAs(*filename);

    //Set the query window title
    wnd->SetQueryWindowTitle();
	//Handles the .schemadesign file
	ret = ptabsd->HandleSchemaDesignfile(fname.GetAsWideChar(), &jobbuff);
	if(ret == wyFalse)
		return ret;
		
	doc->Parse(jobbuff.GetString());
	
	ptabsd->CanvasLoadXML(doc, &fname);
				
	//If Schema Design file is valid then add to recent files
	if(ptabsd->m_save == wyTrue && isrecentfiles == wyFalse)
		pGlobals->m_pcmainwin->WriteLatestFile(fname.GetAsWideChar());

#endif

	return wyTrue;
}

// This function opens the SQL file asked from user and copies its content in the edit box.
wyBool
MDIWindow::OpenSQLFile(wyString *filename, wyBool issametab, wyBool isrecentfiles)
{
	HANDLE			hfile;
    EditorBase		*peditorbase;
    
	//To open file in new querytab.
	if(issametab == wyFalse)
		m_pctabmodule->CreateQueryEditorTab(this);

	if(!GetActiveTabEditor() || !GetActiveTabEditor()->m_peditorbase)
		m_pctabmodule->CreateQueryEditorTab(this);

	peditorbase = GetActiveTabEditor()->m_peditorbase;
	
	m_bopenflag = wyTrue;
			
	//Gets the file handle
	hfile = CreateFile(filename->GetAsWideChar(), GENERIC_READ, 
						FILE_SHARE_READ, NULL, 
						OPEN_EXISTING, 
						FILE_ATTRIBUTE_NORMAL, NULL);                     

	if(hfile == INVALID_HANDLE_VALUE)
	{
        DisplayErrorText(GetLastError(), _("Could not open file."), peditorbase->m_hwnd);
		return wyFalse;
	}
	
	if(issametab == wyFalse)
		peditorbase->m_filename.SetAs(*filename);

	SendMessage(peditorbase->m_hwnd, WM_SETREDRAW, (WPARAM)FALSE, (LPARAM)0);
				
	if(WriteSQLToEditor(hfile, issametab) == wyFalse)
	{
		SendMessage(peditorbase->m_hwnd, WM_SETREDRAW, (WPARAM)TRUE, (LPARAM)0);

		SetFocus(peditorbase->m_hwnd);
		return wyFalse;
	}
	
	if(isrecentfiles == wyFalse)
		pGlobals->m_pcmainwin->WriteLatestFile(filename->GetAsWideChar());

	SendMessage(peditorbase->m_hwnd, WM_SETREDRAW, (WPARAM)TRUE, (LPARAM)0);
	InvalidateRect(peditorbase->m_hwnd, NULL, TRUE);
	
	if(issametab == wyTrue && peditorbase->m_save == wyTrue)
		peditorbase->m_edit = wyTrue;

	else if(issametab == wyTrue)
		peditorbase->m_edit = wyTrue;

	else
	{
		peditorbase->m_save = wyTrue;
		peditorbase->m_edit = wyFalse;
        SendMessage(peditorbase->m_hwnd, SCI_SETSAVEPOINT, 0, 0);
	}
		
    SetFocus(peditorbase->GetHWND());
	
	return wyTrue;
}

// This function initiate the process of opening queryxml
wyBool
MDIWindow::OpenQBFile(wyString *filename, wyBool issametab, wyBool isrecentfiles)
{
#ifndef COMMUNITY

	wyString		    fname;
	TiXmlDocument		*doc;
	wyBool				ret = wyFalse;
	wyString			jobbuff;
    TabQueryBuilder 	*ptabqb = NULL;
    MDIWindow*          wnd = NULL;
	wyInt32				retmsg = 0, img = 0;

    VERIFY(wnd = GetActiveWin());

	if(!wnd || !filename || !filename->GetLength())
        return wyFalse;

	// Gets the white space
	TiXmlBase::SetCondenseWhiteSpace(false);
	
	if(issametab == wyFalse)
	{
		m_pctabmodule->CreateQueryBuilderTab(this);
		ptabqb = (TabQueryBuilder*)m_pctabmodule->GetActiveTabType();

		if(!ptabqb)
			return wyFalse;
	}

	else
	{	
		img = m_pctabmodule->GetActiveTabImage();
		if(img != IDI_QUERYBUILDER_16)
			m_pctabmodule->CreateQueryBuilderTab(this);
			//return wyFalse;
        
		ptabqb = (TabQueryBuilder*)m_pctabmodule->GetActiveTabType();
		
		if(!ptabqb)
			return wyFalse;

		retmsg = ptabqb->OnTabClosing(wyFalse);
		
        if(!retmsg)
			return wyFalse;
        
        //clear the query builder tab
        ptabqb->ClearQueryBuilder();
	
	}
		
	doc = new TiXmlDocument();
	fname.SetAs(*filename);

    //set the file name to empty and call SetQueryWindowTitle() to reset the tab title
	wnd->SetQueryWindowTitle();
    ret = ptabqb->HandleQueryBuilderfile(fname.GetAsWideChar(), &jobbuff);
	
    if(ret == wyFalse)
		return ret;
		
	doc->Parse(jobbuff.GetString());

    //m_isautomated is set to wyTrue to automate the existing QB generation routines
	ptabqb->m_isautomated = wyTrue;
    ptabqb->LoadQueryXML(doc, &fname);
    ptabqb->m_isautomated = wyFalse;
    
    //If QB file is valid then add to recent files
	if(ptabqb->m_filename.GetLength() && ptabqb->m_isedited == wyFalse && isrecentfiles == wyFalse)
        pGlobals->m_pcmainwin->WriteLatestFile(fname.GetAsWideChar());

#endif

	return wyTrue;
}

// Function writes the data from the SQL file whose path is passed as parameter into the function.
wyBool
MDIWindow::WriteSQLToEditor(HANDLE hfile, wyBool issametab)
{
	DWORD		dwbytesread;
	HCURSOR		hcursor;
    EditorBase	*peditorbase = NULL;
	TabEditor	*ptabeditor = NULL;
	wyString	datastr, codepage;
    wyInt32     headersize = 0, fileformat = 0, size;
    wyWChar     *buff = NULL;
    wyChar      temp[SIZE_1024 + 1];
    wyBool      isfirstiteration = wyTrue;
    wyChar*      data;
    
	ptabeditor = GetActiveTabEditor();

	if(!ptabeditor)
	{
		if(hfile)
			VERIFY(CloseHandle(hfile));
		return wyFalse;
	}

	peditorbase = ptabeditor->m_peditorbase;

	hcursor = GetCursor();
	SetCursor(LoadCursor(NULL, IDC_WAIT));				
	ShowCursor(1);

    if(issametab == wyTrue)
    {
        size = GetFileSize(hfile, NULL);
        data = new wyChar[size + 2];
    }
    else
    {
        size = SIZE_1024;
        data = temp;
    }

    while(ReadFile(hfile, data, size, &dwbytesread, NULL) && dwbytesread)
	{
        headersize = 0;
        data[dwbytesread/sizeof(wyChar)] = 0;

        if(isfirstiteration == wyTrue)
        {
            fileformat = DetectFileFormat(data, SIZE_1024 < dwbytesread ? SIZE_1024 : dwbytesread, &headersize);
	    }

        switch(fileformat)
	    {
            case NCP_UTF16:
		        buff = (wyWChar *)(data + headersize);
                buff[(dwbytesread - headersize)/sizeof(wyWChar)] = 0;
                datastr.SetAs(buff);
                break;

            case NCP_UTF8:
                 datastr.SetAs(data + headersize);
                 break;

            default:
                datastr.SetAs(data + headersize);

                // there is a chance that the data may be Utf8 without BOM, so we are checking for the pattern
		        if(isfirstiteration == wyTrue && CheckForUtf8(datastr) == wyTrue)
	            {
		            datastr.SetAs(data + headersize);
                    fileformat = NCP_UTF8;
	            }
	            else
                {
                    datastr.SetAs(data, wyFalse);
                }
        };

	    if(issametab == wyTrue)
	    {
            SendMessage(peditorbase->m_hwnd , SCI_REPLACESEL,(WPARAM)datastr.GetLength(),(LPARAM)datastr.GetString());
	    }
        else
        {
            SendMessage(peditorbase->m_hwnd , SCI_APPENDTEXT, (WPARAM)datastr.GetLength(),(LPARAM)datastr.GetString());
        }

        isfirstiteration = wyFalse;
    }

	EditorFont::SetLineNumberWidth(peditorbase->m_hwnd);

	if(issametab == wyFalse)
	{
		peditorbase->m_save = wyTrue;
		//m_pctabmodule->Resize();
		/*SetQueryWindowTitle();

		SetWindowText(m_hwnd, m_title.GetAsWideChar());*/
        SendMessage(peditorbase->m_hwnd, SCI_EMPTYUNDOBUFFER, 0, 0);
	}
    else
    {
        delete[] data;
    }

	//SetWindowText(m_hwnd, m_title.GetAsWideChar());
	VERIFY (CloseHandle(hfile));
	
	SetCursor(hcursor);
	ShowCursor(1);
	
	SetFocus(peditorbase->m_hwnd);

	return wyTrue;
}

// Ask or confirmation to save file and return the value.
wyInt32
MDIWindow::ConfirmSaveFile()
{
	wyInt32		ret;
	wyString	msg;
	EditorBase	*peditorbase;

	peditorbase  =	m_pctabmodule->GetActiveTabType()->m_peditorbase;

	if(peditorbase->m_save == wyFalse)      
        	msg.Sprintf(_("The content of this tab has changed.\n\nDo you want to save the changes?"));
	else
		msg.Sprintf(_("The content of the %s file has changed.\n\nDo you want to save the changes?"),
					 peditorbase->m_filename.GetString());

	ret = yog_message(m_hwnd, msg.GetAsWideChar(), 
						pGlobals->m_appname.GetAsWideChar(), 
						MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON1);	

	return ret;
}

// Write the current SQL into a file. Basically this function is called after the file processing 
// in the save option has been done.
wyBool
MDIWindow::WriteSQLToFile(HWND hwnd, wyString &filename, wyBool istabclose)
{
	wyUInt32	len;
	wyInt32		ret;
	DWORD		dwbyteswritten;		
	HANDLE		hfile;
	HCURSOR		hcursor;
	
	TextRange range;
	wyChar		*text = NULL;
	wyInt32 kwcase;
	wyInt32 funcase;
	wyInt32 count = 1;
	wyUInt32 cpytextlen =0;
	wyUInt32 lowrange = 0;
	wyUInt32 highrange = 0;

	len = SendMessage(hwnd, SCI_GETTEXTLENGTH, 0, 0);     

	hcursor = GetCursor();
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	ShowCursor(1); 
	
	kwcase = GetKeyWordCase();
	funcase = GetFunCase();

	hfile = CreateFile((LPCWSTR)filename.GetAsWideChar(), GENERIC_WRITE, 0, NULL, 
		               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if(hfile == INVALID_HANDLE_VALUE)
	{
		//If tab is closing and file is not saved 
		if(istabclose == wyTrue)
			DisplayErrorText(GetLastError(), _("File could not be saved. Close action was cancelled."));
		else
			DisplayErrorText(GetLastError(), _("Could not create file."));
		
		return wyFalse;
	}

	while(highrange < len)
	{
		if(len < (65536 + highrange))
			highrange = len;
		else
			highrange += count * 65536 ;

		range.chrg.cpMin = lowrange;
		range.chrg.cpMax = highrange;	
		cpytextlen = highrange - lowrange;

		text = AllocateBuff(cpytextlen + 10);
		range.lpstrText = text;

		SendMessage(hwnd, SCI_GETTEXTRANGE, 0,(LPARAM)&range);		
		
		range.lpstrText = NULL;

		//if both kwcase and function cases are unchanged, no need to alter the original text
		if(kwcase != 2 || funcase != 2)
			GetStyledText(hwnd, text, &range, kwcase, funcase);

		ret = WriteFile(hfile, text, cpytextlen, &dwbyteswritten, NULL);

		free(text);

		if(ret == 0)	
		{
			DisplayErrorText(GetLastError(), _("Could not write into file."));
			return wyFalse;
		}
		lowrange = highrange;
	}	

	VERIFY (CloseHandle(hfile));
	SetCursor(hcursor);
	ShowCursor(1);

	//SendMessage(hwnd, SCI_EMPTYUNDOBUFFER, 0, 0);    
	
	return wyTrue;
}

// Function to set the check of result in grid or text when this window is activated.
wyBool
MDIWindow::SetResultTextOrGrid()
{
	wyInt32	    lstyle;
	HMENU	    hmenu, hsubmenu;
    TabEditor   *ptabeditor = NULL;

	VERIFY(hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain));

	// we get the style because if the window is maximized then the menu index changes.
	lstyle = GetWindowLongPtr(m_hwnd, GWL_STYLE);

	if((lstyle & WS_MAXIMIZE) && wyTheme::IsSysmenuEnabled(m_hwnd))
		hsubmenu  =  GetSubMenu(hmenu, 2);
	else
		hsubmenu  =  GetSubMenu(hmenu, 1);
	
    ptabeditor = GetActiveTabEditor();

	if(ptabeditor && ptabeditor->m_istextresult) 
		CheckMenuItem(hsubmenu, IDM_EDIT_RESULT_TEXT, MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(hsubmenu, IDM_EDIT_RESULT_TEXT, MF_BYCOMMAND | MF_UNCHECKED);

	return wyTrue;
}

// Function to set whether the user wants object browser or not.
wyBool
MDIWindow::SetObjBrowVis()
{
	wyInt32	lstyle;
	HMENU	hmenu, hsubmenu;

	VERIFY(hmenu  = GetMenu(pGlobals->m_pcmainwin->m_hwndmain));

	// we get the style because if the window is maximized then the menu index changes.
	lstyle = GetWindowLongPtr(m_hwnd, GWL_STYLE);

	if((lstyle & WS_MAXIMIZE) && wyTheme::IsSysmenuEnabled(m_hwnd))
		hsubmenu = GetSubMenu(hmenu, 2);
	else
		hsubmenu = GetSubMenu(hmenu, 1);
	
	if(m_isobjbrowvis)
		CheckMenuItem(hsubmenu, IDC_EDIT_SHOWOBJECT, MF_BYCOMMAND | MF_UNCHECKED);
	else
		CheckMenuItem(hsubmenu, IDC_EDIT_SHOWOBJECT, MF_BYCOMMAND | MF_CHECKED);

	if(m_pctabmodule->GetActiveTabType()->m_isresultwnd)
		CheckMenuItem(hsubmenu, IDC_EDIT_SHOWRESULT, MF_BYCOMMAND | MF_UNCHECKED);
	else
		CheckMenuItem(hsubmenu, IDC_EDIT_SHOWRESULT, MF_BYCOMMAND | MF_CHECKED);

	if(m_pctabmodule->GetActiveTabType()->m_iseditwnd)
		CheckMenuItem(hsubmenu, IDC_EDIT_SHOWEDIT, MF_BYCOMMAND | MF_UNCHECKED);
	else
		CheckMenuItem(hsubmenu, IDC_EDIT_SHOWEDIT, MF_BYCOMMAND | MF_CHECKED);
	
	return wyTrue;
}

// Function to SHOW/HIDE the object browser when the user select HIDE Object Browser in the
// edit menu.

wyBool
MDIWindow::ShowOrHideObjBrowser()
{
    if(m_isobjbrowvis == wyTrue)	
    {
		m_pcqueryvsplitter->m_leftortoppercent = m_pcqueryvsplitter->m_lastleftpercent;
	} 
    else	
    {
		//On hiding and showing object browser,splitter position is not same
		//m_pcqueryvsplitter->m_leftortoppercent = m_pcqueryvsplitter->m_leftortoppercent;
		m_pcqueryvsplitter->m_lastleftpercent = m_pcqueryvsplitter->m_leftortoppercent;
		m_pcqueryvsplitter->m_leftortoppercent = 0;
	}

    CustomTab_SetPaintTimer(m_pctabmodule->m_hwnd);
    Resize();
	return wyTrue;
}


// Function to SHOW/HIDE the result window when the user select HIDE Result Window in the
// edit menu.
wyBool
MDIWindow::ShowOrHideResultWindow()
{
	TabEditorSplitter   *ptaesplitter = NULL; 
	TabTypes	        *ptabeditor	= NULL;
	EditorBase		    *peditorbase	= NULL;

	ptabeditor = m_pctabmodule->GetActiveTabType();

	if(ptabeditor->m_isresultwnd == wyTrue)
    {
		ptaesplitter = ptabeditor->m_pcetsplitter;

		if(ptaesplitter->m_leftortoppercent == ptaesplitter->m_lasttoppercent && 
           ptaesplitter->m_leftortoppercent == 100 && 
           ptaesplitter->m_lasttoppercent == 100)
        {
            ptaesplitter->m_lasttoppercent = 50;
            ptaesplitter->m_leftortoppercent = ptaesplitter->m_lasttoppercent;
        }
		else
            ptaesplitter->m_leftortoppercent = ptaesplitter->m_lasttoppercent;

		peditorbase = m_pctabmodule->GetActiveTabType()->m_peditorbase;
			
        CustomTab_SetPaintTimer(m_pctabmodule->m_hwnd);
		ShowWindow(peditorbase->m_hwnd, SW_HIDE);
		Resize();
	} else	if(ptabeditor->m_iseditwnd == wyTrue)
    {
		ptaesplitter =ptabeditor->m_pcetsplitter;

		ptaesplitter->m_lasttoppercent = ptaesplitter->m_leftortoppercent;
        
        if(ptaesplitter->m_lasttoppercent == ptaesplitter->m_leftortoppercent && 
           ptaesplitter->m_leftortoppercent == 100 && 
           ptaesplitter->m_lasttoppercent == 100)
        {
            ptaesplitter->m_lasttoppercent = 50;
            ptaesplitter->m_leftortoppercent = ptaesplitter->m_lasttoppercent;
        }
        else
		    ptaesplitter->m_leftortoppercent = 100;
		
        CustomTab_SetPaintTimer(m_pctabmodule->m_hwnd);
		Resize();	
	}

	return wyTrue;
}

// Function to SHOW/HIDE the edit window.
wyBool
MDIWindow::ShowOrHideEditWindow()
{
	TabEditorSplitter	*ptesplitter	= NULL; 
	TabEditor			*ptabeditor		= NULL; 

    ptabeditor = GetActiveTabEditor();

	if(!ptabeditor)	
            return wyTrue;   	

	if(ptabeditor->m_iseditwnd == wyTrue)
    {
		ptesplitter = ptabeditor->m_pcetsplitter;

		ptesplitter->m_leftortoppercent = ptesplitter->m_lasttoppercent;		
	} 
	
	else if(ptabeditor->m_isresultwnd == wyTrue)
    {
		ptesplitter = ptabeditor->m_pcetsplitter;

		ptesplitter->m_lasttoppercent = ptesplitter->m_leftortoppercent;
		ptesplitter->m_leftortoppercent = 0;
		
		SetFocus(m_hwnd);		
	}

    CustomTab_SetPaintTimer(m_pctabmodule->m_hwnd);
	Resize();
	return wyTrue;
}

// Fumction sets various information of the querywindow tab data when the window is seitched.
wyBool
MDIWindow::SetVariousStatusInfo()
{
    if(m_pctabmodule->m_hwnd)
	{
        OnCtcnSelChange(m_hwnd, IDC_CTAB); 
	}

	return wyTrue;
}

// Function to show the template menu when the user use the keyboard to popup the template menu.
wyBool
MDIWindow::ShowTemplateMenu()
{
	wyInt32		lStyle;
	POINT		pt;
	HMENU		hmenu, hsubmenu, htempmenu;
	EditorBase	*peditorbase = NULL;

	VERIFY (hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain));
	
	// first check whether the window is maximized or not and then get the shortcut key.
	lStyle = GetWindowLongPtr(m_hwnd, GWL_STYLE);

	if((lStyle & WS_MAXIMIZE) && wyTheme::IsSysmenuEnabled(m_hwnd))
		VERIFY (hsubmenu = GetSubMenu(hmenu, 2));
	else
		VERIFY (hsubmenu = GetSubMenu(hmenu, 1));

	VERIFY (htempmenu = GetSubMenu(hsubmenu, 11));
	VERIFY (GetCaretPos(&pt));

    peditorbase = m_pctabmodule->GetActiveTabType()->m_peditorbase;
		
	VERIFY (ClientToScreen(peditorbase->m_hwnd, &pt));		

	// track it at the current cursor pos.
	VERIFY (TrackPopupMenu(htempmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x+10, pt.y+10, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));

	return wyTrue;
}

// Function to show the FLUSH dialoog box. The dialog box is initialized with the querywindow
// pointer and from there we call the flush function to execute the query.
wyInt32
MDIWindow::ShowFlushDlg()
{
	wyInt32		ret;
	//EditorBase	*peditorbase;

	//Post 8.01
	/*if(GetActiveTabEditor() != NULL)
    {
        peditorbase    = GetActiveTabEditor()->m_peditorbase;
	
		//InvalidateRect(peditorbase->m_hwnd, NULL, FALSE);
		//UpdateWindow(peditorbase->m_hwnd);
    }*/
	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_FLUSH), m_hwnd, 
							MDIWindow::FlushWndProc,(LPARAM)this);

	return ret;
    
}

// DLgProc for the Flush Dialog Box.
INT_PTR CALLBACK
MDIWindow::FlushWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    wyInt32    ret;
	MDIWindow *pcquerywnd = (MDIWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
        LocalizeWindow(hwnd);
		SendMessage(GetDlgItem(hwnd, IDC_FL_LOGS), BM_SETCHECK, TRUE, 0);

		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		return 0;

	case WM_INITDLGVALUES:
		pcquerywnd->AddFlushPersistence(hwnd);
		break;

	case WM_HELP:
		ShowHelp("Flush%20in%20SQLyog%20MySQL%20Tool.htm");
		return wyTrue;	

	case WM_DESTROY:
		if(pcquerywnd->m_p_flush)
        {
			delete pcquerywnd->m_p_flush;
			pcquerywnd->m_p_flush = NULL;
		}
		break;

	case WM_COMMAND:
		pcquerywnd->EnableDisableFlushButton(hwnd);
		switch(LOWORD(wparam))
		{
		case IDOK:
             return pcquerywnd->OnFlushOK(hwnd);

		case IDC_FLUSH_ALL:
			/* if all is checked we need to enable disable all the buttons */
			ret = SendMessage(GetDlgItem(hwnd, IDC_FLUSH_ALL), BM_GETCHECK, 0, 0);
            pcquerywnd->EnableDisableFlushOptions(hwnd, (ret)?wyFalse:wyTrue);
			break;

        case IDCANCEL:
			yog_enddialog(hwnd, 1);
		}
	}
	return 0;
}

wyInt32 
MDIWindow::OnFlushOK(HWND hwnd)
{
    HWND    hwndtext;
    wyInt32 writetobin = 0;

	VERIFY (hwndtext = GetDlgItem(hwnd, IDC_MSG));

	if(IsDlgButtonChecked(hwnd, IDC_FL_BINLOG) && IsMySQL41(m_tunnel, &m_mysql) == wyTrue)
		writetobin = 1;

	if((IsDlgButtonChecked(hwnd, IDC_FLUSH_ALL)) == BST_CHECKED)
         return FlushAll(hwnd, hwndtext, writetobin);
    else
        return FlushSpecific(hwnd, hwndtext, writetobin);
}

wyInt32 
MDIWindow::FlushAll(HWND hwnd, HWND hwndtext, wyInt32 writetobin)
{
    wyInt32	    size, count;
    wyString    query;
    MYSQL_RES   *res;
    wyBool      rc, success = wyTrue;
    wyChar      flushquery[][SIZE_64] = {"logs", "hosts", "privileges", 
			                             "tables", "status", "des_key_file",
			                             "query cache", "user_resources",
			                             "tables with read lock" };

    size = sizeof(flushquery) / sizeof(flushquery[0]);

    for(count = 0; count < size; count++)
    {
        rc = ExecuteFlush(flushquery[count], hwndtext, writetobin, hwnd);
        if(rc == wyFalse)
        {
	        success = wyFalse;
	        return 1;
        }
    }

    /* if everything is ok then we need to unlock table for 
    flush tables with read lock */
    if(success == wyTrue)
    {
        query.SetAs("unlock tables");

        res = ExecuteAndGetResult(this, m_tunnel, &m_mysql, query);
        						
        if(!res && m_tunnel->mysql_affected_rows(m_mysql)== -1)
        {
	       ShowMySQLError(hwnd, m_tunnel, &m_mysql, query.GetString());
	        SendMessage(hwndtext, WM_SETTEXT, 0,(LPARAM)_(L"Error in operation."));
	        return 0;
        }

        m_tunnel->mysql_free_result(res);
    }
    return 1;
}

wyInt32 
MDIWindow::FlushSpecific(HWND hwnd, HWND hwndtext, wyInt32 writetobin)
{
    MYSQL_RES   *res;
    wyString    query;

    if((IsDlgButtonChecked(hwnd, IDC_FL_LOGS))== BST_CHECKED)
		ExecuteFlush("logs", hwndtext, writetobin, hwnd);

	if((IsDlgButtonChecked(hwnd, IDC_FL_HOSTS))== BST_CHECKED)
		ExecuteFlush("hosts", hwndtext, writetobin, hwnd);

	if((IsDlgButtonChecked(hwnd, IDC_FL_PRIV))== BST_CHECKED)
		ExecuteFlush("privileges", hwndtext, writetobin, hwnd);
    
    if((IsDlgButtonChecked(hwnd, IDC_FL_TABLES))== BST_CHECKED)
	    ExecuteFlush("tables", hwndtext, writetobin, hwnd);

	if((IsDlgButtonChecked(hwnd, IDC_FL_STATUS))== BST_CHECKED)
		ExecuteFlush("status", hwndtext, writetobin, hwnd);

	if((IsDlgButtonChecked(hwnd, IDC_FL_DES))== BST_CHECKED)
        ExecuteFlush("des_key_file", hwndtext, writetobin, hwnd);

	if((IsDlgButtonChecked(hwnd, IDC_FL_CACHE))== BST_CHECKED)
        ExecuteFlush("query cache", hwndtext, writetobin, hwnd);

	if((IsDlgButtonChecked(hwnd, IDC_FL_RESOURCE))== BST_CHECKED)
        ExecuteFlush("user_resources", hwndtext, writetobin, hwnd);

	if((IsDlgButtonChecked(hwnd, IDC_FL_TABLESREAD))== BST_CHECKED)
	{
		if(IsNewMySQL(m_tunnel, &m_mysql))
        {
			ExecuteFlush("tables with read lock", hwndtext, writetobin, hwnd);

			/* if everything is ok then we need to unlock table for 
				flush tables with read lock */
			query.SetAs("unlock tables");
            res = ExecuteAndGetResult(this, m_tunnel, &m_mysql, query);

            if(!res && m_tunnel->mysql_affected_rows(m_mysql)== -1)
			{
				ShowMySQLError(hwnd, m_tunnel, &m_mysql, query.GetString());
				SendMessage(hwndtext, WM_SETTEXT, 0,(LPARAM)_(L"Error in operation."));
				return 0;
			}
    		
			m_tunnel->mysql_free_result(res);

		} 
        else 
			ExecuteFlush("tables", hwndtext, BST_UNCHECKED, hwnd);
	}
    return 1;
}

wyBool
MDIWindow::EnableDisableFlushOptions(HWND hwnd, wyInt32 state)
{

    wyInt32 counter, checkstate, uncheckcount = 0;
	wyInt32	id[] = {	IDC_FL_LOGS, IDC_FL_HOSTS, IDC_FL_PRIV,
							IDC_FL_STATUS, IDC_FL_TABLES, IDC_FL_TABLESREAD,
							IDC_FL_DES, IDC_FL_CACHE, IDC_FL_RESOURCE };
	wyInt32	ids[] = {   IDC_FLUSH_ALL, IDC_FL_BINLOG};
	
	wyInt32	size = sizeof(id)/sizeof(id[0]);
	

	for(counter = 0; counter < size; counter++)
	{
		checkstate = SendMessage(GetDlgItem(hwnd, id[counter]), BM_GETCHECK, 0, 0);

		if(checkstate == BST_CHECKED)
			EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
		else
			uncheckcount++;

		EnableWindow(GetDlgItem(hwnd, id[counter]), state);
	}

	if(uncheckcount == size)
	{
		uncheckcount = 0;
		size = sizeof(ids) / sizeof(ids[0]); 

		for(counter = 0; counter < size; counter++)
		{
			checkstate = SendMessage(GetDlgItem(hwnd, ids[counter]), BM_GETCHECK, 0, 0);

			if(checkstate == BST_CHECKED)
			{
				EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
				break;
			}
			else
				uncheckcount++;
		}
		if(uncheckcount == size)
			EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
	}
	
	return wyTrue;
}
wyBool
MDIWindow::EnableDisableFlushButton(HWND hwnd)
{
	wyInt32 counter, checkstate, uncheckcount = 0;
	wyInt32	id[] = {	IDC_FL_LOGS, IDC_FL_HOSTS, IDC_FL_PRIV,
							IDC_FL_STATUS, IDC_FL_TABLES, IDC_FL_TABLESREAD,
							IDC_FL_DES, IDC_FL_CACHE, IDC_FL_RESOURCE, IDC_FLUSH_ALL, IDC_FL_BINLOG };
	
	wyInt32	size = sizeof(id)/sizeof(id[0]);
	
	for(counter = 0; counter < size; counter++)
	{
		checkstate = SendMessage(GetDlgItem(hwnd, id[counter]), BM_GETCHECK, 0, 0);

		if(checkstate == BST_CHECKED)
		{
			EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
			break;
		}
		else
		{
			uncheckcount++;
		}

	}
	if(uncheckcount == size)
		EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
	
	return wyTrue;
}


wyBool
MDIWindow::AddFlushPersistence(HWND hwnd)
{
    wyInt32 ret;
	
	m_p_flush = new Persist;
	m_p_flush->Create("FLUSH");

	m_p_flush->Add(hwnd, IDC_FL_LOGS, "FlushLogs", "0", CHECKBOX);
	m_p_flush->Add(hwnd, IDC_FL_HOSTS, "FlushHosts", "0", CHECKBOX);
	m_p_flush->Add(hwnd, IDC_FL_PRIV, "FlushPrivs", "0", CHECKBOX);
	m_p_flush->Add(hwnd, IDC_FL_STATUS, "FlushStatus", "0", CHECKBOX);
	m_p_flush->Add(hwnd, IDC_FL_TABLES, "FlushTables", "0", CHECKBOX);
	m_p_flush->Add(hwnd, IDC_FL_TABLESREAD, "FlushRead", "0", CHECKBOX);
	m_p_flush->Add(hwnd, IDC_FL_DES, "FlushDesKey", "0", CHECKBOX);
	m_p_flush->Add(hwnd, IDC_FL_CACHE, "FlushCache", "0", CHECKBOX);
	m_p_flush->Add(hwnd, IDC_FL_RESOURCE, "FlushResource", "0", CHECKBOX);

	m_p_flush->Add(hwnd, IDC_FLUSH_ALL, "FlushAll", "0", CHECKBOX);
	m_p_flush->Add(hwnd, IDC_FL_BINLOG, "FlushBinLog", "0", CHECKBOX);
	
	/* if all is selected then we need to disable everything */
	ret = SendMessage(GetDlgItem(hwnd, IDC_FLUSH_ALL), BM_GETCHECK, 0, 0);
    EnableDisableFlushOptions(hwnd, (ret)?wyFalse:wyTrue);

	if(IsMySQL41(m_tunnel, &m_mysql) == wyFalse)
    {
        SendMessage(GetDlgItem(hwnd, IDC_FL_BINLOG), BM_SETCHECK, BST_UNCHECKED, 0);

		EnableWindow(GetDlgItem(hwnd, IDC_FL_BINLOG), FALSE);
    }

	return wyTrue;
}

// Function to execute the various commands.
// The query is sent as a text biffer and if unsuccessful then we set an error in the edit box
// whose HWND is sent as parameter.
wyBool
MDIWindow::ExecuteFlush(wyString  flushquery, HWND hwndEdit, 
						  wyInt32 writetobin, HWND hwnddlg)
{
	wyString	query;
	MYSQL_RES	*res;

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	query.Sprintf("flush %s",(BST_CHECKED==writetobin)?("NO_WRITE_TO_BINLOG "):(""));
	query.Add(flushquery.GetString());
	
	res  = ExecuteAndGetResult(this, m_tunnel, &(m_mysql), query);
	
	if(!res && m_tunnel->mysql_affected_rows(m_mysql)== -1)
	{
		ShowMySQLError(hwnddlg, m_tunnel, &(m_mysql), query.GetString());
		SendMessage(hwndEdit, WM_SETTEXT, 0,(LPARAM)_(L"Error in operation."));
		return wyFalse;
	}

	m_tunnel->mysql_free_result(res);
	SendMessage(hwndEdit, WM_SETTEXT, 0,(LPARAM)_(L"Operation successful"));
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return wyTrue;
}

// Function to show the template dialog box.
// The function returns the STRING RESOURCE ID of the item selected.
// We use this resource to get a string in the resource and add it to the edit box at the current
// Selection.
wyBool
MDIWindow::ShowTemplateDlg()
{
	Listinfo    *listret;
	wyInt32     ret;
    TabTypes	*ptabeditor  = NULL; 
	EditorBase	*peditorbase = NULL;
	wyWChar		buff[SIZE_1024] = {0};
    wyString    temp;

	ptabeditor = m_pctabmodule->GetActiveTabType();
	
	if(ptabeditor->m_iseditwnd == wyFalse)
    {
		yog_message(m_hwnd, _(L"Please make the Query window visible to use this option"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
		return wyFalse;
	}

	//Post 8.01
	//RepaintTabModule();

	listret = (Listinfo *)DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_TEMPLATE), m_hwnd, 
		MDIWindow::TemplateWndProc,(LPARAM)this);

	if(listret != 0)
	{
		ret = LoadString(pGlobals->m_hinstance, listret->m_value, buff, ((SIZE_1024-1) * sizeof(wyWChar)));
        temp.SetAs(buff);
		// Every time create a new tab to contain the template
		m_pctabmodule->CreateQueryEditorTab(this);

		peditorbase	= m_pctabmodule->GetActiveTabType()->m_peditorbase; 
        peditorbase->m_isdiscardchange = wyTrue;
        SendMessage(peditorbase->m_hwnd, SCI_SETTEXT, 0, (LPARAM)temp.GetString());
        SendMessage(peditorbase->m_hwnd, SCI_SETSAVEPOINT, 0, 0);
        SendMessage(peditorbase->m_hwnd, SCI_EMPTYUNDOBUFFER, 0, 0);
        peditorbase->m_isdiscardchange = wyFalse;

		EditorFont::SetLineNumberWidth(peditorbase->m_hwnd);
		free(listret);

		return wyFalse;
	}

	peditorbase	= m_pctabmodule->GetActiveTabType()->m_peditorbase; 
    if(peditorbase)
	    SetFocus(peditorbase->m_hwnd);		

	return wyTrue;
}

// Window procedure for the template dialog box.

INT_PTR CALLBACK
MDIWindow::TemplateWndProc(HWND hwnd, UINT pMessage, WPARAM wparam, LPARAM lparam)
{
	MDIWindow * pcquerywnd = (MDIWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(pMessage)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
        LocalizeWindow(hwnd);
		pcquerywnd->InitListBox(GetDlgItem(hwnd, IDC_TEMPLIST), GetDlgItem(hwnd, IDC_SAMPLE));
		return wyTrue;

		case WM_INITDLGVALUES:
			pcquerywnd->InitTempDlg(hwnd);
			break;

		case WM_HELP:
			ShowHelp("SQL%20Templates%20SQLyog%20MySQL%20Front%20End.htm");
			return wyTrue;

		case WM_DESTROY:
			SaveInitPos(hwnd, SQLTEMPLATE_SECTION);
			//delete the dialog control list
			DlgControl* pdlgctrl;
			while((pdlgctrl = (DlgControl*)pcquerywnd->m_controllist.GetFirst()))
			{
				pcquerywnd->m_controllist.Remove(pdlgctrl);
				delete pdlgctrl;
			}

			pcquerywnd->FreeList((HWND)lparam);
			break;

	case WM_SIZE:
		pcquerywnd->SQLTempResize(hwnd);
		break;

		case WM_GETMINMAXINFO:
			pcquerywnd->OnWMSizeInfo(lparam);
			break;

		case WM_PAINT:
			//draws the resize gripper at bottom-right corner of dialog
			DrawSizeGripOnPaint(hwnd);
			break;

	case WM_COMMAND:
        pcquerywnd->OnTemplateWmCommand(hwnd, wparam, lparam);
        break;
	}
	return 0;
}

void
MDIWindow::OnTemplateWmCommand(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    switch(HIWORD(wparam))
	{
	case LBN_SELCHANGE:
		ShowTmpPreview((HWND)lparam, GetDlgItem(hwnd, IDC_SAMPLE));
		break;

	case LBN_DBLCLK:
		CloseTmpDlg((HWND)lparam, hwnd);
		break;
	}

	switch(LOWORD(wparam))
	{
	case IDOK:
		CloseTmpDlg(GetDlgItem(hwnd, IDC_TEMPLIST), hwnd);
		break;

	case IDCANCEL:
		yog_enddialog(hwnd, 0);
		break;
	}
    return;
}

// Initializes the template dialog box with the template items.
wyBool
MDIWindow::InitListBox(HWND hwndlist, HWND hwndEdit)
{	
	wyString		tempvaluestr;
	TEMPLATEVALUE	tmpvalue[] = 
	{
		{ IDM_EDIT_TEMP_SELECTSTMT,		L"Select", 0 },
		{ IDM_EDIT_TEMP_JOINSTMT,		L"Join", 0 },
		{ IDM_EDIT_TEMP_INSERT1,		L"Insert", 0},
		{ IDM_EDIT_TEMP_INSERT2,		L"Insert-Select", 0 },
		{ IDM_EDIT_TEMP_UPDATE,			L"Update", 0 },
		{ IDM_EDIT_TEMP_DELETE,			L"Delete", 0},
		{ IDM_EDIT_TEMP_TRUNCATE,		L"Truncate", 0 },
		{ IDM_EDIT_TEMP_REPLACE,		L"Replace", 0 },
		{ IDM_EDIT_TEMP_LOADDATA,		L"Load Data", 0 },
		{ IDM_EDIT_TEMP_CREATE1,		L"Create Table", 0 },
		{ IDM_EDIT_TEMP_CREATE2,		L"Create Database", 0 },
		{ IDM_EDIT_TEMP_CREATE3,		L"Create Index", 0 },
		{ IDM_EDIT_TEMP_DROP1,			L"Drop Database", 0 },
		{ IDM_EDIT_TEMP_DROP2,			L"Drop Table", 0 },
		{ IDM_EDIT_TEMP_ALTER,			L"Alter Table", 0 },
		{ IDM_EDIT_TEMP_ALTERDB,		L"Alter Database", 0 },
		{ IDM_EDIT_TEMP_RENAME,			L"Rename Table", 0 },
		{ IDM_EDIT_TEMP_USER1,			L"Use", 0 },
		{ IDM_EDIT_TEMP_USER2,			L"Describe", 0 },
		{ IDM_EDIT_TEMP_BEGIN,			L"Begin/Commit/Rollback", 0 },
		{ IDM_EDIT_TEMP_LOCK,			L"Lock/Unlock", 0 },
		{ IDM_EDIT_TEMP_TRANSACTION,	L"Transaction", 0 },
		{ IDM_EDIT_TEMP_BACKUP_RESTORE, L"Backup/Restore", 0 },
		{ IDM_EDIT_TEMP_CREATEPROCEDURE, L"Create Procedure", 1 },
		{ IDM_EDIT_TEMP_CREATEFUNCTION, L"Create Function", 1 },
		{ IDM_EDIT_TEMP_ALTERPROCEDURE, L"Alter Procedure", 1 },
		{ IDM_EDIT_TEMP_ALTERFUNCTION,	L"Alter Function", 1 },
		{ IDM_EDIT_TEMP_DROPPROCEDURE,	L"Drop Procedure", 0 },
		{ IDM_EDIT_TEMP_DROPFUNCTION,	L"Drop Function", 0 },
		{ IDM_EDIT_TEMP_CREATEVIEW,		L"Create View",0 },
		{ IDM_EDIT_TEMP_ALTERVIEW,		L"Alter View", 0 },
		{ IDM_EDIT_TEMP_DROPVIEW,		L"Drop View", 0 },
		{ IDM_EDIT_TEMP_CREATETRIGGER,	L"Create Trigger", 1 },
		{ IDM_EDIT_TEMP_DROPTRIGGER,	L"Drop Trigger", 0 },
		{ IDM_EDIT_TEMP_CREATEEVENT,    L"Create Event", 1},
		{ IDM_EDIT_TEMP_ALTEREVENT,     L"Alter Event", 0},
		{ IDM_EDIT_TEMP_DROPEVENT,      L"Drop Event", 0}
	};
	wyInt32 size = sizeof(tmpvalue)/ sizeof(tmpvalue[0]);
	wyInt32 templatecount, ret;

	for(templatecount = 0; templatecount < size; templatecount++)
	{
		VERIFY ((ret = SendMessage(hwndlist, LB_ADDSTRING, 0,(LPARAM)tmpvalue[templatecount].m_caption))!= LB_ERR);

		Listinfo *plistinfo =(Listinfo *)malloc(sizeof(Listinfo)* 1);

		plistinfo->m_value = tmpvalue[templatecount].m_idvalue;
		plistinfo->m_adv	 = tmpvalue[templatecount].m_type;
		tempvaluestr.SetAs(tmpvalue[templatecount].m_caption);
		strncpy(plistinfo->m_caption, tempvaluestr.GetString(), tempvaluestr.GetLength());
	
		VERIFY ((ret = SendMessage(hwndlist, LB_SETITEMDATA, ret, (LPARAM)plistinfo)) != LB_ERR);
	}

	SendMessage(GetParent(hwndlist),WM_INITDLGVALUES,0,0);
	SendMessage(hwndlist, LB_SETCURSEL, 0, 0);
	ShowTmpPreview(hwndlist, hwndEdit);

	return wyTrue;
}

wyBool
MDIWindow::InitTempDlg(HWND hwnd)
{
	m_hwndtemp = hwnd;
	
	GetClientRect(m_hwndtemp, &m_dlgrect);
	GetWindowRect(m_hwndtemp, &m_wndrect);
	
	//set the initial position of the dialog
	SetWindowPositionFromINI(m_hwndtemp, SQLTEMPLATE_SECTION, 
		m_wndrect.right - m_wndrect.left, 
		m_wndrect.bottom - m_wndrect.top);
	
	GetCtrlRects();
	PositionCtrls();
	return wyTrue;

}


// Function shows the template of the item selected from the listbox on every change.
wyBool
MDIWindow::ShowTmpPreview(HWND hwndlist, HWND hwndedit)
{
	wyInt32		ret;
    wyWChar	    preview[SIZE_1024] = {0};
	Listinfo    *pitemdata;
	wyInt32     itemdata;

	VERIFY ((ret = SendMessage(hwndlist, LB_GETCURSEL, 0, 0))!= LB_ERR);

	VERIFY ((itemdata = SendMessage(hwndlist, LB_GETITEMDATA, ret, 0))!= LB_ERR);

	pitemdata = (Listinfo *)itemdata;

	VERIFY ((ret = LoadString(pGlobals->m_hinstance, pitemdata->m_value, preview, (SIZE_1024 - 1)))   != LB_ERR);

	SendMessage(hwndedit, WM_SETTEXT, 0,(LPARAM)preview);
	
	return wyTrue;
}

wyBool
MDIWindow::FreeList(HWND hwndlist)
{
	wyInt32     ret;
	Listinfo    *pitemdata;
	wyInt32     itemdata;

	VERIFY((ret = SendMessage(hwndlist, LB_GETCURSEL, 0, 0))!= LB_ERR);
	
	VERIFY((itemdata = SendMessage(hwndlist, LB_GETITEMDATA, ret, 0))!= LB_ERR);

	pitemdata =(Listinfo*)itemdata;
        if(pitemdata)
	      free(pitemdata);   

	return wyTrue;
}

// Function get the id from the selected text and close the dialog with returning the itemdata of
// the selected item. This itemdata is nothing but the string table id.
wyBool
MDIWindow::CloseTmpDlg(HWND hwndlist, HWND hwnddlg)
{

	wyInt32	    ret, sel;
	wyInt32     itemdata;
	Listinfo    *pitemdata;

	ret = SendMessage(hwndlist, LB_GETCURSEL, 0, 0);

	if(ret == LB_ERR)
    {
		yog_message(hwnddlg, _(L"Select a template to insert"), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);
		return wyFalse;
	}

	VERIFY (sel = SendMessage(hwndlist, LB_GETSEL, ret, 0)!= LB_ERR);

	if(sel > 0)
	{
		itemdata    = SendMessage(hwndlist, LB_GETITEMDATA, ret, 0);
		pitemdata   = (Listinfo *)itemdata;
		VERIFY (yog_enddialog(hwnddlg,(wyInt32)pitemdata));
		return wyTrue;
	}
	else
	{
		VERIFY (yog_enddialog(hwnddlg, 0));
		return wyTrue;		
	}
}

StatusBarMgmt* 
MDIWindow::GetQueryStatus()
{
	return m_pcquerystatus;
}

void 
MDIWindow::HandleToolBar()
{
	if(m_executing == wyFalse)
		pGlobals->m_pcmainwin->HandleFirstToolBar();

	return;
}

void 
MDIWindow::SetQueryWindowTitle()
{
	wyString	newtitle, mdititle;
	wyInt32		tabimageid = 0;
    TabTypes    *ptabtypes = m_pctabmodule->GetActiveTabType();
		    
	tabimageid = m_pctabmodule->GetActiveTabImage();	
	
    // Create the title string. Create the server name.
	if(!m_tunnel->IsTunnel())
    {
		if(m_conninfo.m_isssh == wyTrue)
		{
			m_tunneltitle.Sprintf(_("- %s@%s - Using SSH tunnel to %s"), m_mysql->user,
			                     m_conninfo.m_localhost.GetString(), m_conninfo.m_sshhost.GetString());
		}
		else
		{
			m_tunneltitle.Sprintf("- %s@%s", m_mysql->user,
			                    (strlen(m_mysql->host)!= 0)? m_mysql->host : "localhost");
		}
	}
	else
	{
		m_tunneltitle.Sprintf(_("- %s@%s - Using HTTP tunneler at %s"), m_tunnel->GetUser(),
			                 (strlen(m_tunnel->GetHost())!= 0)? m_tunnel->GetHost(): "localhost", m_tunnel->GetHttpAddr());
	}

	
	switch(tabimageid)
	{
	case IDI_CREATEVIEW:
	case IDI_ALTERVIEW:
	case IDI_CREATETRIGGER:
	case IDI_ALTERTRIGGER:
	case IDI_CREATEEVENT:
	case IDI_ALTEREVENT:
	case IDI_CREATEFUNCTION:
	case IDI_ALTERFUNCTION:
	case IDI_CREATEPROCEDURE:
	case IDI_ALTERPROCEDURE:
	case IDI_QUERY_16:
		TabEditorTitles(&m_tunneltitle);		
		break;

    case IDI_ALTERTABLE:
    case IDI_CREATETABLE:
        ((TableTabInterface*)ptabtypes)->TabInterfaceTitle(&m_tunneltitle);
        //m_tunneltitle.Add("*");
        //..Write here the code to get Modified Window Title

        break;

#ifndef COMMUNITY
	case IDI_SCHEMADESIGNER_16:
		TabSchemaDesignerTitles(&m_tunneltitle);		
		break;
#endif
	
#ifndef COMMUNITY
    case IDI_QUERYBUILDER_16:
        TabQueryBuilderTitles(&m_tunneltitle);
        break;
#endif

	default:
		break;
	}

    //if user selected and database name, then add it to title
	if(m_pcqueryobject->m_seldatabase.GetLength())
		mdititle.Sprintf("%s/%s %s", m_title.GetString(), m_pcqueryobject->m_seldatabase.GetString(), m_tunneltitle.GetString());
	else
		mdititle.Sprintf("%s %s", m_title.GetString(), m_tunneltitle.GetString());

	// Setting the MDI window title
	SetWindowText(m_hwnd, mdititle.GetAsWideChar());

	return;
}

void
MDIWindow::TabEditorTitles(wyString *mdititle)
{
	wyString	newtitle;
	EditorBase	*peditorbase = NULL;
	TabEditor	*ptabeditor = NULL;
	
	ptabeditor = GetActiveTabEditor();
	peditorbase = ptabeditor->m_peditorbase;	
	
	//Checks whether commandline argument as got filename whose contents as to be displayed in Sqlyog Editor
	if(pGlobals->m_filename.GetLength())
		peditorbase->m_filename.SetAs(pGlobals->m_filename);

	
	//Set the file name
	if(peditorbase && ((peditorbase->m_save == wyTrue) || (peditorbase->m_edit == wyTrue)))
	{
		if((peditorbase->m_save == wyTrue) && (peditorbase->m_edit == wyTrue))
		{
			//Seta tab name
			m_pctabmodule->SetTabName(peditorbase->m_filename.GetAsWideChar(), wyTrue, wyTrue);
			
			newtitle.Sprintf("%s*", peditorbase->m_filename.GetString());
		}

		else if(peditorbase->m_save == wyTrue)
		{
			//Set tab name
			m_pctabmodule->SetTabName(peditorbase->m_filename.GetAsWideChar(), wyTrue);

			newtitle.Sprintf("%s", peditorbase->m_filename.GetString());
		}

		//SetWindowText(peditorbase->m_hwndfilename, newtitle.GetAsWideChar());
	}

	// Set the window title
	//if we made any change in editor or if we are changing the editor by FInd replace dialog
	if(peditorbase && (peditorbase->m_edit == wyTrue))            
	{
		if(mdititle->GetCharAt(mdititle->GetLength()-1) != '*')
			mdititle->Add("*");
	}	
}

void
MDIWindow::TabSchemaDesignerTitles(wyString *mdititle)
{
#ifndef COMMUNITY

	wyString			newtitle;
	TableElem			*ptabinfo = NULL;
	TabSchemaDesigner	*ptabsd;

	ptabsd = (TabSchemaDesigner*)m_pctabmodule->GetActiveTabType();
	if(!ptabsd)
		return;
	
	if(ptabsd->m_filewndtitle.GetLength())
		ptabsd->m_filewndtitle.Clear();
	
	ptabinfo = (TableElem*)ptabsd->m_listTable.GetFirst();
	if(ptabinfo)
	{
		ptabsd->m_filewndtitle.Sprintf(_("Database: %s"), ptabinfo->m_db.GetString());
		
		/*if(ptabsd->m_save == wyTrue)
		{
			ptabsd->m_filewndtitle.AddSprintf(" - ");
		}*/
	}

	if(ptabsd->m_save == wyTrue || ptabsd->m_isedited == wyTrue)
	{
        if(ptabsd->m_save == wyTrue && ptabsd->m_isedited == wyTrue)
		{
			m_pctabmodule->SetTabName(ptabsd->m_filename.GetAsWideChar(), wyFalse,wyTrue);
			newtitle.Sprintf("%s*", ptabsd->m_filename.GetString());
		}

		else if(ptabsd->m_save == wyTrue)
		{
			m_pctabmodule->SetTabName(ptabsd->m_filename.GetAsWideChar(), wyFalse);
			newtitle.Sprintf("%s", ptabsd->m_filename.GetString());
		}

		/*if(newtitle.GetLength())
			ptabsd->m_filewndtitle.AddSprintf("%s", newtitle.GetString());*/				
	}

	if(ptabsd->m_isedited == wyTrue || ptabsd->m_isnotempty == wyTrue)            
	{
		if(mdititle->GetCharAt(mdititle->GetLength()-1) != '*')
			mdititle->Add("*");
	}

    //if there is no file name, then set it to default
    if(!ptabsd->m_filename.GetLength())
		m_pctabmodule->SetTabName(_(L"Schema Designer"), wyFalse, wyFalse);

	// Set the filename window
	if(ptabsd->m_filewndtitle.GetLength())
		SetWindowText(ptabsd->m_hwndfilename, ptabsd->m_filewndtitle.GetAsWideChar());
    else
        SetWindowText(ptabsd->m_hwndfilename, L"");
	
	//To hide the filename window
	//else
	//	ptabsd->Resize();

#endif
}

//Function to change the MDI and QB tab titles when the query builder is activated
void
MDIWindow::TabQueryBuilderTitles(wyString *mdititle)
{
#ifndef COMMUNITY

	TabQueryBuilder 	*ptabqb;
    wyString temp;

	ptabqb = (TabQueryBuilder*)m_pctabmodule->GetActiveTabType();
	
    if(!ptabqb)
		return;

    //if there is a file name, then set it as tab header and change the tool bar caption
    if(ptabqb->m_filename.GetLength())
    {
		m_pctabmodule->SetTabName(ptabqb->m_filename.GetAsWideChar(), wyFalse, ptabqb->m_isedited);

        if(ptabqb->m_isedited == wyTrue)
        {
            if(ptabqb->m_filewndtitle.GetCharAt(ptabqb->m_filewndtitle.GetLength() - 1) != '*')
                ptabqb->m_filewndtitle.Add("*");
        }
        else
        {
            if(ptabqb->m_filewndtitle.GetCharAt(ptabqb->m_filewndtitle.GetLength() - 1) == '*')
                ptabqb->m_filewndtitle.Erase(ptabqb->m_filewndtitle.GetLength() - 1, 1);
        }

        //set the tool bar caption
        //SetWindowText(ptabqb->m_hwndfilename, ptabqb->m_filewndtitle.GetAsWideChar());
    }
    // if there is no filename, set the default
    else
    {
        temp.SetAs(_("Query Builder"));
        m_pctabmodule->SetTabName(temp.GetAsWideChar(), wyFalse, wyFalse);

        //set the tool bar caption
        SetWindowText(ptabqb->m_hwndfilename, ptabqb->m_filewndtitle.GetAsWideChar());
    }
    
    //add * if m_isedited is set
    if(ptabqb->m_isedited == wyTrue)
        if(mdititle->GetCharAt(mdititle->GetLength()-1) != '*')
	        mdititle->Add("*");

#endif
}

wyBool 
MDIWindow::GetMySQLCharsetLength()
// it is used to get the wyChar length whether it is MULTIBYTE or not.....
{
	wyInt32     len = 0;
	wyString    query;
	MYSQL_RES	*res;
	MYSQL_ROW   row;

	if(!IsMySQL41(m_tunnel, &m_mysql))
		return wyFalse;

	query.Sprintf("select MAXLEN from INFORMATION_SCHEMA.CHARACTER_SETS where CHARACTER_SET_NAME ='%s'", m_conninfo.m_codepage.m_cpname.GetString());
    
	res = ExecuteAndGetResult(this, m_tunnel, &m_mysql, query);
	
	if(!res && m_tunnel->mysql_affected_rows(m_mysql)== -1)
		return wyFalse ;

	row = m_tunnel->mysql_fetch_row(res);
	if(!row[0])
		return wyFalse;

	len = (wyInt32)atoi(row[0]);

	m_tunnel->mysql_free_result(res);
	
	if(len > 1)
		return wyTrue; // it is multibyte...
	else
		return wyFalse; // non multibyte...
}


HWND 
MDIWindow::GetHwnd()
{
	return m_hwnd;
}

TabModule*	
MDIWindow::GetTabModule()
{
	return m_pctabmodule;
}

CQueryObject* 
MDIWindow::GetQueryObject()
{
	return m_pcqueryobject;
}

wyBool 
MDIWindow::GetScintillaKeyWordsAndFunctions(Tunnel *tunnel, MYSQL *mysql, wyBool bkeyword, wyString &buffer)
{
	wyString    query;
	sqlite3	    *hdb;

    buffer.Clear();

    if(!tunnel || OpenKeyWordsDB(&hdb) == wyFalse)
	{
        return wyFalse;
	}
	
    query.Sprintf("select distinct object_name as obj_name, object_type as obj_type from objects where object_version <= %d and object_type = %d  order by lower(obj_name)", 
                    GetVersionNo(tunnel, &mysql), (bkeyword)?AC_PRE_KEYWORD:AC_PRE_FUNCTION);
		
	YogSqliteExec(hdb, query.GetString(), callback_tags, &buffer);
		
	sqlite3_close(hdb);

	return wyTrue;
}

wyInt32 
MDIWindow::callback_tags(void *t, wyInt32 argc, wyChar **argv, wyChar **colname)
{
	wyString *str = (wyString*)t;
  
	if(argv[0])
        str->AddSprintf("%s ", argv[0]);

	return 0;
}

wyChar **  
MDIWindow::GetKeywords(wyInt32 *functioncount)
{	
	sqlite3		*hdb;
    tagArray    tags;
	wyString    query;

    if(OpenKeyWordsDB(&hdb) == wyFalse)
        return NULL;
	
    InitTagsArray(&tags, AC_PRE_KEYWORD, wyFalse);

	query.Sprintf("select obj_name, obj_type from(select distinct object_name as obj_name, object_type as obj_type from objects where object_version <= %d)where obj_type = %d order by lower(obj_name)", 
                    GetVersionNo(m_tunnel, &m_mysql), AC_PRE_FUNCTION);
		
	YogSqliteExec(hdb, query.GetString(), callback_tags2, &tags);
	sqlite3_close(hdb);
			
	*functioncount = tags.rowcount;

	return tags.keys;
}

wyInt32  
MDIWindow::callback_tags2(void *t, wyInt32 argc, wyChar **argv, wyChar **colname)
{
	tagArray	*tags =(tagArray *)t;
	wyString	buff;
    wyInt32     len;
  
    if(argv[0])
	{
		if(tags->rowcount == tags->maxcount)
		{
			tags->maxcount += ROWSIZE_128;
			tags->keys = (wyChar**)realloc(tags->keys, sizeof(wyChar*)* tags->maxcount);
		}
				
		if(tags->bflag)	
			len = buff.Sprintf("%s?%s", argv[0], argv[1]); /* for db 3 and for table 4*/
		else
			len = buff.Sprintf("%s", argv[0]); /* for db 3 and for table 4*/

        tags->keys[tags->rowcount] = (wyChar*)calloc(sizeof(wyChar), len + 1);
							
		strcpy(tags->keys[tags->rowcount], buff.GetString());
		
		tags->rowcount++;
	}
	return 0;
}

void
MDIWindow::InsertEnginesMenuItems(HMENU hmenu)
{
    MENUITEMINFO	lpmii={0};
    wyInt32         engineid = ENGINE_ID_START;
    wyWChar         *enginebuff, *tokpointer;
    wyInt32         state, image;
	wyString        strengine;
	
    image = m_pcqueryobject->GetSelectionImage();

	if(image != NTABLE)
        state = MFS_GRAYED;
    else
        state = MFS_ENABLED;
		
    // remove all existing table engines
    while(DeleteMenu(hmenu, 0, MF_BYPOSITION));

	GetTableEngineString(m_tunnel, &m_mysql, strengine);
    
    enginebuff = AllocateBuffWChar((strengine.GetLength() + 1) * sizeof(wyWChar));
    wcscpy(enginebuff, strengine.GetAsWideChar());

    tokpointer = wcstok(enginebuff, L";");

    while(tokpointer != NULL && engineid < ENGINE_ID_END)
	{
        lpmii.cbSize        = sizeof(MENUITEMINFO);
	    lpmii.fMask         = MIIM_STRING|MIIM_ID|MIIM_STATE;
	    lpmii.wID           = engineid++;
	    lpmii.cch           = wcslen(tokpointer);
	    lpmii.dwTypeData	= tokpointer;
        lpmii.fState        = state;

        VERIFY(::InsertMenuItem(hmenu, -1, TRUE, &lpmii));
	        
        tokpointer = wcstok(NULL, L";");		
    }

    free(enginebuff);

    wyTheme::SetMenuItemOwnerDraw(hmenu);
}

void 
MDIWindow::StopQueryExecution()
{
    // Access the shared resource.
    EnterCriticalSection(&m_cs);
    m_stopquery = 1;
    LeaveCriticalSection(&m_cs);
    return ;
}

wyBool
MDIWindow::IsStopQueryVariableReset()
{
	wyBool ret;
	
	EnterCriticalSection(&m_cs);

	ret = (m_stopquery) ? wyFalse : wyTrue;

	LeaveCriticalSection(&m_cs);

	return ret;
}


wyBool
MDIWindow::ReConnect(Tunnel * tunnel, PMYSQL mysql, wyBool isssh, wyBool isimport, wyBool isprofile)
{
	wyWChar     curdb[SIZE_512]= {0};
	MYSQL		*newmysql;
	wyString	currentdb;
	
	newmysql = tunnel->mysql_init((MYSQL*)NULL);

	if(isprofile == wyTrue)
	{
		pGlobals->m_pcmainwin->GetCurrentSelDB(curdb, SIZE_512 - 1);

		if(0 == wcsicmp(curdb, NODBSELECTED))
			curdb[0]= 0;
		
		currentdb.SetAs(curdb);
	}

	if(isssh == wyTrue)
	{   
		if(!ReConnectSSH(&m_conninfo))
			return wyFalse;		

        ExecuteInitCommands(newmysql, tunnel, m_conninfo.m_initcommand);

		//For SSH its requured because the above function gives new LOCAL PORT to do port-forward
		SetMySQLOptions(&m_conninfo, tunnel, &newmysql);

		newmysql = tunnel->mysql_real_connect(newmysql,(*mysql)->host, 
			(*mysql)->user,(*mysql)->passwd, NULL, 
					m_conninfo.m_localport, NULL,(*mysql)->client_flag | CLIENT_MULTI_RESULTS | CLIENT_REMEMBER_OPTIONS, NULL);	
		
		if(newmysql)
		{
			tunnel->mysql_close(*mysql);
			*mysql = newmysql;

			if(isimport == wyFalse)
				m_stopmysql = m_mysql = newmysql;
			
		}
	}

	/*
	if(m_conninfo.m_issslchecked == wyTrue)
    {
        if(m_conninfo.m_issslauthchecked == wyTrue)
        {
            mysql_ssl_set(newmysql, m_conninfo.m_clikey.GetString(), m_conninfo.m_clicert.GetString(), 
                            m_conninfo.m_cacert.GetString(), NULL, 
                            m_conninfo.m_cipher.GetLength() ? m_conninfo.m_cipher.GetString() : NULL);
        }
        else
        {
            mysql_ssl_set(newmysql, NULL, NULL, 
                            m_conninfo.m_cacert.GetString(), NULL, 
                            m_conninfo.m_cipher.GetLength() ? m_conninfo.m_cipher.GetString() : NULL);
        }
    }
	*/
	
	if(isprofile == wyTrue && currentdb.GetLength() &&  UseDatabase(currentdb, *mysql, tunnel) == wyFalse)
		return wyFalse;

		
	/* if its tunnel then we need the server version */
	if(tunnel->IsTunnel())
    {
		if(!tunnel->GetMySQLVersion(*mysql))
        {
            tunnel->mysql_close(*mysql);
            return wyFalse;
        }
    }

	return wyTrue;
}

TabTableData*
MDIWindow::GetActiveTabTableData()
{
    return  m_pctabmodule->GetActiveTabTableData();
}

TabEditor*	
MDIWindow::GetActiveTabEditor()
{
   return  m_pctabmodule->GetActiveTabEditor();
}


TabHistory*	
MDIWindow::GetActiveHistoryTab()
{
   return  m_pctabmodule->GetActiveHistoryTab();

}

TabObject*	
MDIWindow::GetActiveInfoTab()
{
   return  m_pctabmodule->GetActiveInfoTab();

}

void
MDIWindow::HandleEditMenu()
{
	TabTypes		*ptabeditor = NULL;
	ptabeditor	=	m_pctabmodule->GetActiveTabType();

	if(ptabeditor->m_istextresult == wyTrue)
		m_pctabmodule->HandleCheckMenu(this, wyTrue , IDM_EDIT_RESULT_TEXT);
	else if(ptabeditor->m_istextresult == wyFalse)
		m_pctabmodule->HandleCheckMenu(this, wyFalse , IDM_EDIT_RESULT_TEXT);
   
	if(ptabeditor->m_isresultwnd == wyTrue)
		m_pctabmodule->HandleCheckMenu(this, wyFalse, IDC_EDIT_SHOWRESULT);
	else if(ptabeditor->m_isresultwnd == wyFalse)
		m_pctabmodule->HandleCheckMenu(this, wyTrue, IDC_EDIT_SHOWRESULT);
      
	if(ptabeditor->m_iseditwnd == wyTrue)
		m_pctabmodule->HandleCheckMenu(this, wyFalse, IDC_EDIT_SHOWEDIT);
	else if(ptabeditor->m_iseditwnd == wyFalse)
		m_pctabmodule->HandleCheckMenu(this, wyTrue, IDC_EDIT_SHOWEDIT);

}

void
MDIWindow::EnableExportDialog()
{
    wyString    query, tablename, dbname;
    wyInt32     item;
	HTREEITEM   hitem;

	VERIFY(hitem = TreeView_GetSelection(m_pcqueryobject->m_hwnd));
	item = GetItemImage(m_pcqueryobject->m_hwnd, hitem);
	
	if(m_pcqueryobject->HandlerToGetTableDatabaseName(item, hitem) == wyFalse)
        return ;
    
    tablename.SetAs(m_pcqueryobject->m_seltable.GetString());
    dbname.SetAs(m_pcqueryobject->m_seldatabase.GetString());
   
    if(wcscmp(m_pcqueryobject->m_seltable.GetAsWideChar(), TXT_VIEWS) == 0)
    {
        ExportViews();
        return;
    }

    StartExportDialog(dbname, tablename);
}

void
MDIWindow::StartExportDialog(wyString &dbname, wyString &tablename)
{
    CExportResultSet    exportresult;
    MySQLDataEx         *ptr = new MySQLDataEx(this);
    

    //TabQueryTypes       *ptr = new TabQueryTypes(this);
    ptr->Initialize();
    ptr->m_table.SetAs(tablename.GetString());
    ptr->m_db.SetAs(dbname.GetString());
    
    exportresult.Export(m_hwnd, ptr);

    delete ptr;
}

void
MDIWindow::ExportViews()
{
    wyString    viewname;
	HTREEITEM	hitem;

    VERIFY(hitem = TreeView_GetSelection(m_pcqueryobject->m_hwnd));

    m_pcqueryobject->GetTableDatabaseName(hitem);

	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	VERIFY(hitem = TreeView_GetParent(m_hwnd, hitem));
	m_pcqueryobject->GetDatabaseName(hitem);
   
    viewname.SetAs(m_pcqueryobject->m_seltable.GetString());
      
    StartExportDialog(m_database, viewname);
}

void 
MDIWindow::HandleMysqlError()
{
    wyString    fullerror;
    wyString    error;
    wyInt32     errorno;

	errorno = m_tunnel->mysql_errno(m_mysql);
    error.SetAs(m_tunnel->mysql_error(m_mysql));
    fullerror.Sprintf(_("%s, Error No : %d"), error.GetString(), errorno);

    yog_message(m_hwnd, fullerror.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
}

wyBool
MDIWindow::ReConnectSSH(ConnectionInfo *coninfo)
{
#ifndef COMMUNITY		
	wyInt32         sshret;
    
	PROCESS_INFORMATION     pi;

	if(coninfo->m_hprocess != INVALID_HANDLE_VALUE)
	{
		//If SSH connection then close handles at end
		if(coninfo->m_isssh == wyTrue)
			OnExitSSHConnection(&coninfo->m_sshpipeends);

		if(coninfo->m_sshsocket)
			closesocket(coninfo->m_sshsocket);
           
		coninfo->m_sshsocket = NULL;

		TerminateProcess(coninfo->m_hprocess, 1);
	}
	
	sshret = CConnectionEnt::CreateSSHSession(coninfo, &pi);

	if(sshret)
	{	
		ShowSSHError(m_hwnd );
      	coninfo->m_hprocess = INVALID_HANDLE_VALUE;
		return wyFalse;
	}
    else
    {
        if(pGlobals->m_hmapfile)
          VERIFY(CloseHandle(pGlobals->m_hmapfile));

        pGlobals->m_hmapfile = NULL;
    }

	coninfo->m_hprocess      = pi.hProcess;	

#endif
	
    return wyTrue;
		
}

void 
MDIWindow::LoadQueryTabPlusMenu(LPARAM lparam)
{
	HMENU	hmenu, htrackmenu;
    LPNMCTC lpnmctc = (LPNMCTC)lparam;
    wyInt32 extramenucount = 3;

    hmenu =	LoadMenu(pGlobals->m_hinstance, MAKEINTRESOURCE(IDR_TABPLUSMENU));
    LocalizeMenu(hmenu);
	htrackmenu = GetSubMenu(hmenu, 0);

    switch(pGlobals->m_pcmainwin->m_connection->m_enttype)
    {
        case ENT_PRO:
            DeleteMenu(htrackmenu, ID_QUERYBUILDER, MF_BYCOMMAND);
            DeleteMenu(htrackmenu, ID_SCHEMADESIGNER, MF_BYCOMMAND);
        case ENT_NORMAL:
            DeleteMenu(htrackmenu, ID_DATASEARCH, MF_BYCOMMAND);
    }

	if(m_executing == wyTrue /*|| m_pingexecuting == wyTrue*/)
    {
        EnableMenuItem(htrackmenu, ID_QUERYBUILDER, MF_DISABLED);
        EnableMenuItem(htrackmenu, ID_SCHEMADESIGNER, MF_DISABLED);
        EnableMenuItem(htrackmenu, ID_DATASEARCH, MF_DISABLED);
        EnableMenuItem(htrackmenu, ID_NEW_EDITOR, MF_DISABLED);
        EnableMenuItem(htrackmenu, ID_HISTORY, MF_DISABLED);
        EnableMenuItem(htrackmenu, ID_INFOTAB, MF_DISABLED);
        EnableMenuItem(htrackmenu, ID_OBJECT_VIEWDATA, MF_DISABLED);
    }

    if(pGlobals->m_istabledataunderquery == wyTrue)
    {
        extramenucount--;
        DeleteMenu(htrackmenu, ID_OBJECT_VIEWDATA, MF_BYCOMMAND);
    }

    if(pGlobals->m_isinfotabunderquery == wyTrue)
    {
        extramenucount--;
        DeleteMenu(htrackmenu, ID_INFOTAB, MF_BYCOMMAND);
    }

    if(pGlobals->m_ishistoryunderquery == wyTrue)
    {
        extramenucount--;
        DeleteMenu(htrackmenu, ID_HISTORY, MF_BYCOMMAND);
    }

    if(extramenucount == 0)
    {
        DeleteMenu(htrackmenu, GetMenuItemCount(htrackmenu) - 1, MF_BYPOSITION);
    }
    
	// Set menu draw property for drawing icon
	wyTheme::SetMenuItemOwnerDraw(htrackmenu);

    TrackPopupMenu(htrackmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, lpnmctc->curpos.x, lpnmctc->curpos.y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL);
	
	FreeMenuOwnerDrawItem(htrackmenu);

   	VERIFY(DestroyMenu(hmenu));	
}

void
MDIWindow::GetCtrlRects()
{
    RECT    rect;
    wyInt32 i, count;
    HWND    hwnd;

    //ctrlid    size-x     size-y
    wyInt32 ctrlid[] = {
       	IDC_TEMPLIST, 1, 1,
        IDC_SAMPLE, 1, 1,
        IDOK, 0, 0,
        IDCANCEL, 0, 0,
        IDC_TEMP_STATIC, 1, 0,
		IDC_TEMP_STATIC1, 0, 0,
    };

    count = sizeof(ctrlid)/sizeof(ctrlid[0]);

    //store the control handles and related information in the linked list
    for(i = 0; i < count; i+=3)
    {
		hwnd = GetDlgItem(m_hwndtemp, ctrlid[i]);
        GetWindowRect(hwnd, &rect);
		MapWindowPoints(NULL, m_hwndtemp, (LPPOINT)&rect, 2);
        m_controllist.Insert(new DlgControl(hwnd, 
                                            ctrlid[i], 
                                            &rect, 
                                            ctrlid[i + 1] ? wyTrue : wyFalse, 
                                            ctrlid[i + 2] ? wyTrue : wyFalse));
    }
}

void
MDIWindow::PositionCtrls()
{
    DlgControl* pdlgctrl;
    wyInt32     leftpadding, toppadding, rightpadding, bottompadding, width, height;
    wyInt32     x, y;
    RECT        rect;
    HDWP        hdefwp;
		
	GetClientRect(m_hwndtemp, &rect);

    //BeginDeferWindowPos is used to make the control positioning atomic
    hdefwp = BeginDeferWindowPos(m_controllist.GetCount() + 1);

    //iterate throught the control lists
    for(pdlgctrl = (DlgControl*)m_controllist.GetFirst(); pdlgctrl;
        pdlgctrl = (DlgControl*)pdlgctrl->m_next)
    {
        leftpadding = pdlgctrl->m_rect.left - m_dlgrect.left;
        toppadding = pdlgctrl->m_rect.top - m_dlgrect.top;
        rightpadding = m_dlgrect.right - pdlgctrl->m_rect.right;
        bottompadding = m_dlgrect.bottom - pdlgctrl->m_rect.bottom;
        width = pdlgctrl->m_rect.right - pdlgctrl->m_rect.left;
        height = pdlgctrl->m_rect.bottom - pdlgctrl->m_rect.top;
        
        if(pdlgctrl->m_issizex == wyFalse)
        {
			switch(pdlgctrl->m_id)
			{
				case IDC_TEMP_STATIC1:
					x = (rect.right - rect.left)/3 - 7;
					break;
				default:
					x=rect.right - rightpadding - width;
			}
        }
        else
        {
			x = leftpadding;
			switch(pdlgctrl->m_id)
			{
				case IDC_TEMPLIST:
					width = (rect.right - rect.left)/3 - 25;
					break;
				case IDC_SAMPLE:
					x = (rect.right - rect.left)/3 - 7;
					width = rect.right - rightpadding - x;
					break;
				case IDC_TEMP_STATIC:
					width = rect.right - rightpadding - leftpadding;
					break;
			}
        }
	    switch(pdlgctrl->m_id)
        {
			case IDOK:
			case IDCANCEL:
			case IDC_TEMP_STATIC:
                y = rect.bottom - bottompadding - height;
                break;
            default:
                y = toppadding;
        }

        if(pdlgctrl->m_issizey == wyTrue)
        {
            height = rect.bottom - bottompadding - y;
        }

        //change the control position
        hdefwp = DeferWindowPos(hdefwp, pdlgctrl->m_hwnd, NULL, x, y, width, height, SWP_NOZORDER);
    }

    //finish the operation and apply changes
    EndDeferWindowPos(hdefwp);
}

void 
MDIWindow::SQLTempResize(HWND hwnd){

	PositionCtrls();

	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);
}

void
MDIWindow::OnWMSizeInfo(LPARAM lparam)
{
    MINMAXINFO* pminmax = (MINMAXINFO*)lparam;

    pminmax->ptMinTrackSize.x = m_wndrect.right - m_wndrect.left;
    pminmax->ptMinTrackSize.y = m_wndrect.bottom - m_wndrect.top;
}

void
MDIWindow::PositionTabs(wyBool isupdtabledata, wyBool isupdhistory, wyBool isupdinfo)
{
    wyInt32             i, count, j, k;
    CTCITEM             item = {0};
    TabEditor*          ptabeditor;
    wyBool              istabeditorcreated;
    wyInt32             bottomtabsettings[] = {
        pGlobals->m_istabledataunderquery, IDI_TABLE,
        pGlobals->m_isinfotabunderquery, IDI_TABLEINDEX,
        pGlobals->m_ishistoryunderquery, IDI_HISTORY
    };

    istabeditorcreated = m_pctabmodule->GetActiveTabEditor() ? wyTrue : wyFalse;
    count = CustomTab_GetItemCount(m_pctabmodule->m_hwnd);
    item.m_mask = CTBIF_IMAGE | CTBIF_LPARAM;

    for(i = 0; i < count; ++i)
    {
        CustomTab_GetItem(m_pctabmodule->m_hwnd, i, &item);

        if(item.m_iimage == IDI_TABLE && pGlobals->m_istabledataunderquery == wyTrue && 
            ((TabTableData*)item.m_lparam)->m_istabsticky == wyFalse)
        {
            if(istabeditorcreated == wyFalse)
            {
                m_pctabmodule->CreateQueryEditorTab(this, i, wyTrue);
                istabeditorcreated = wyTrue;
                CustomTab_DeleteItem(m_pctabmodule->m_hwnd, i + 1);
            }
            else
            {
                CustomTab_DeleteItem(m_pctabmodule->m_hwnd, i);
                --i;
                --count;
            }
        }
        else if(item.m_iimage == IDI_TABLEINDEX && pGlobals->m_isinfotabunderquery == wyTrue)
        {
            if(istabeditorcreated == wyFalse)
            {
                m_pctabmodule->CreateQueryEditorTab(this, i, wyTrue);
                istabeditorcreated = wyTrue;
                CustomTab_DeleteItem(m_pctabmodule->m_hwnd, i + 1);
            }
            else
            {
                CustomTab_DeleteItem(m_pctabmodule->m_hwnd, i);
                --i;
                --count;
            }
        }        
        else if(item.m_iimage == IDI_HISTORY && pGlobals->m_ishistoryunderquery == wyTrue)
        {
            if(istabeditorcreated == wyFalse)
            {
                m_pctabmodule->CreateQueryEditorTab(this, i, wyTrue);
                istabeditorcreated = wyTrue;
                CustomTab_DeleteItem(m_pctabmodule->m_hwnd, i + 1);
            }
            else
            {
                CustomTab_DeleteItem(m_pctabmodule->m_hwnd, i);
                --i;
                --count;
            }
        }
        else if((ptabeditor = m_pctabmodule->GetTabEditorAt(i)) && ptabeditor->m_pctabmgmt)
        {
            for(j = 0; j < 3; ++j)
            {
                k = ptabeditor->m_pctabmgmt->SelectFixedTab(bottomtabsettings[j * 2 + 1], wyTrue);

                if(bottomtabsettings[j * 2] && k == -1)
                {
                    ptabeditor->m_pctabmgmt->AddFixedTab(bottomtabsettings[j * 2 + 1]);
                }
                else if(!bottomtabsettings[j * 2] && k != -1)
                {
                    ptabeditor->m_pctabmgmt->DeleteTab(k);
                    ptabeditor->m_pctabmgmt->ChangeTitles();
                }
            }
        }
    }

    if(pGlobals->m_istabledataunderquery == wyFalse && isupdtabledata == wyTrue)
    {
        m_pctabmodule->CreateTabDataTab(this);
    }

    if(pGlobals->m_isinfotabunderquery == wyFalse && isupdinfo == wyTrue)
    {
        m_pctabmodule->CreateInfoTab(this);
    }

    if(pGlobals->m_ishistoryunderquery == wyFalse && isupdhistory == wyTrue)
    {
        m_pctabmodule->CreateHistoryTab(this, wyTrue, wyFalse);
    }
}

void
MDIWindow::HandleKeepAliveTimer()
{
    wyString    errmsg;

	if(m_executing == wyTrue || m_isthreadbusy == wyTrue || m_tunnel->IsExecuting() == wyTrue)
		return;
	
	m_statusbartext.SetAs(_(L"Pinging server..."));

	if(GetActiveWin() == this)
	{
		pGlobals->m_pcmainwin->AddTextInStatusBar(_(L"Pinging server..."));
	}

	ResetEvent(m_hevtexecution);
	m_pingexecuting = wyTrue;
	SetEvent(m_hevtkeepalive);

}

unsigned __stdcall 
MDIWindow::KeepAliveThreadProc(LPVOID lpparam)
{
    MDIWindow*  pmdi = (MDIWindow*)lpparam;
    wyInt32     ret;
    wyUInt32    time1 = 0, time2 = 0;
    wyString    errmsg, tmp,timestr;
	pmdi->m_ret = 0;
	wyWChar     systime[SIZE_256];
	mysql_thread_init();
	pmdi->m_hevthist = CreateEvent(NULL, TRUE, FALSE, NULL);


	while(1)
	{
		WaitForSingleObject(pmdi->m_hevtkeepalive, INFINITE);

		time1 = GetHighPrecisionTickCount();
        ret = pmdi->m_tunnel->mysql_ping(pmdi->m_mysql);
		time2 = GetHighPrecisionTickCount();

        memset(systime, 0, sizeof(systime));
		GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, systime, ((SIZE_256-1) * 2));
		timestr.SetAs(systime);
		
		if(ret)
        {
			errmsg.Sprintf(_("PING Error %d: %s"), 
            pmdi->m_tunnel->mysql_errno(pmdi->m_mysql), 
            pmdi->m_tunnel->mysql_error(pmdi->m_mysql));
			tmp.Sprintf(_("[%s] %s. %s: %I32d ms"),timestr.GetString(),errmsg.GetString(),_("Time taken"),(time2-time1)/1000);
		}
		else
		{
			
			tmp.Sprintf(_("[%s] %s. %s: %I32d ms"),timestr.GetString(),_("Server pinged"),_("Time taken"),(time2-time1)/1000);
		}

		pmdi->m_statusbartext.SetAs(tmp.GetAsWideChar());
		
		pmdi->m_pingexecuting = wyFalse;
		ResetEvent(pmdi->m_hevtkeepalive);
		SetTimer(pmdi->m_hwnd, KEEP_ALIVE, (pmdi->m_keepaliveinterval*1000), NULL);
		SetEvent(pmdi->m_hevtexecution);

		if(GetActiveWin() == pmdi)
		{
			pGlobals->m_pcmainwin->AddTextInStatusBar(pmdi->m_statusbartext.GetAsWideChar());
		
		}


	}

	mysql_thread_end();

    CloseHandle(pmdi->m_hevtkeepalive);
    pmdi->m_hevtkeepalive = NULL;
	pmdi->m_hthrdkeepalive = NULL;
    return 0;
}

void
MDIWindow::SetExecuting(wyBool isexecuting)
{
	if(isexecuting == wyTrue)
	{
		if(m_keepaliveinterval)
			WaitForSingleObject(m_hevtexecution, INFINITE);
		m_executing = wyTrue;
	}
	else
	{
		m_executing = wyFalse;
		if(m_keepaliveinterval)
			SetTimer(m_hwnd, KEEP_ALIVE, (m_keepaliveinterval*1000), NULL);
		
	}
}

void
MDIWindow::SetThreadBusy(wyBool isexecuting)
{
	if(isexecuting == wyTrue)
	{
		if(m_keepaliveinterval)
			WaitForSingleObject(m_hevtexecution, INFINITE);
		m_isthreadbusy = wyTrue;
	}
	else
	{	
		m_isthreadbusy = wyFalse;
		if(m_keepaliveinterval)
			SetTimer(m_hwnd, KEEP_ALIVE, (m_keepaliveinterval*1000), NULL);
		
	}
}