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

#include <ocidl.h>
#include <unknwn.h>
#include <olectl.h>
#include <scintilla.h>
#include <shlobj.h>
#ifndef COMMUNITY
#include "tinyxml.h"
#endif
#include "Symbols.h"
#include "ExportMultiFormat.h"
#include "FrameWindow.h"
#include "FrameWindowHelper.h"
#include "MySQLVersionHelper.h"
#include "Global.h"
#include "MDIWindow.h"
#include "ExportMultiFormat.h"
#include "PreferenceBase.h"
#include "PreferenceCommunity.h"
#include "HelpHeader.h"
#include "OtherDialogs.h"
#include "ImportFromSQL.h"
#include "ImportBatch.h"
#include "EditorFont.h"
#include "CommonHelper.h"
#include "SQLTokenizer.h"
#include "GUIHelper.h"
#include "TabEditorSplitter.h"
#include  "wyIni.h"
#include "UpgradeCheck.h"
#include "QueryAnalyzerBase.h"
#include "ConnectionTab.h"
#include "UserManager.h"
#include "ButtonDropDown.h"
#include "TableTabInterface.h"
#include "TableTabInterfaceTabMgmt.h"
#include "TabFields.h"
#include "TabAdvancedProperties.h"
#include "TabForeignKeys.h"
#include "TabIndexes.h"
#include "DataView.h"
#include "CCustomComboBox.h"
#include "Http.h"
#include "htmlayout.h"

#ifndef COMMUNITY
#include "RegistrationApi.h"
#include "TabSchemaDesigner.h"
#include "TabQueryBuilder.h"
#include "HttpError.h"
#include "FormView.h"
#include "DatabaseSearch.h"
#include "VisualDataDiff.h"
#include "QueryAnalyzerEnt.h"
#include "Transactions.h"
#endif

#ifdef COMMUNITY
#include "CommunityRibbon.h"
#endif

extern	PGLOBALS		pGlobals;

#define			UM_UPDATE_PROGRESS		WM_USER + 100
#define			UM_CLOSE_RESTORE_STATUS	WM_USER + 101
#define			UM_UPDATE_CONNECTION	WM_USER + 102

#define			MDISTARTID			60000
#define			FIND_STR_LEN		256
#define			DELIMITEROPEN		"DELIMITER $$\r\n\r\n"
#define			DELIMITERCLOSE		"$$\r\n\r\nDELIMITER ;"
#define			MAX_RECENT_FILES	9
#define			RECENT_FILE_COUNT	10
#define			RECENT_SQLFILE_SUBMENU			22
#define         RECENT_QUERYBUILDER_SUBMENU     23
#define			RECENT_SCHEMADESIGNS_SUBMENU	24
#define         WND_MNU_ITEMS       3
#define         MNU_QUERYBUILDER    8
#define         MNU_SCHEMADESIGNER				9
#define         MNU_REBUILDTAGS     12
#define			ZERO				0
#define			TRIAL_DAYS_LEFT		14
#define			IN_TRANSACTION		-10


#define			FIRSTTOOLICONCOUNT	6

#define			TABBED_INTERFACE_HEIGHT	25
#define         TABBED_INTERFACE_TOP_PADDING    10

#define			SCHEMA_DESCRIPTION "13.00"
#define			SCHEMA_MAJOR_VERSION "13"
#define			SCHEMA_MINOR_VERSION "00"

#define			CONSAVE_INTERVAL	10000
/* some of the features are not available when you connect using tunneling feature */
/* we show the user a decent message and exit */

#define	NO_BACKUP_TUNNEL	_("This feature is not available if you are connecting using HTTP Tunneling")
#define	NO_RESTORE_TUNNEL	_("This feature is not available if you are connecting using HTTP Tunneling")
#define	NO_IMPORTCSV_TUNNEL	_("This feature is not available if you are connecting using HTTP Tunneling")
#define CONNECT_MYSQL_MSG   _(L" Please Connect To A MySQL Server")

//sqlite tables for session save
#define TABLE_SCHEMADETAILS "CREATE TABLE schema_version (description TEXT, major_version TEXT, minor_version TEXT)"
#define TABLE_CONNDETAILS "CREATE TABLE conndetails (Id INTEGER PRIMARY KEY  NOT NULL, position  INTEGER, ObjectbrowserBkcolor TEXT, ObjectbrowserFgcolor TEXT, isfocussed INTEGER, \
							Name TEXT , Host TEXT , User TEXT ,Password TEXT ,Port TEXT, StorePassword TEXT,keep_alive TEXT,Database TEXT ,compressedprotocol TEXT,defaulttimeout TEXT,\
							waittimeoutvalue TEXT,Tunnel TEXT,Http TEXT ,HTTPTime TEXT,HTTPuds TEXT,HTTPudsPath TEXT , Is401 TEXT,IsProxy TEXT,Proxy TEXT , ProxyUser TEXT, ProxyPwd TEXT , ProxyPort TEXT , User401 TEXT , Pwd401 TEXT  , ContentType TEXT  , HttpEncode TEXT,SSH TEXT,SshUser TEXT,SshPwd TEXT,SshHost TEXT,SshPort TEXT,SshForHost TEXT,SshPasswordRadio TEXT,SSHPrivateKeyPath TEXT,SshSavePassword TEXT,SslChecked TEXT,SshAuth TEXT,Client_Key TEXT,Client_Cert TEXT,CA TEXT,Cipher TEXT,sqlmode_global TEXT,sqlmode_value TEXT,init_command TEXT,readonly TEXT)"

#define TABLE_TABDETAILS "CREATE TABLE tabdetails (Id INTEGER ,\
							Tabid INTEGER, Tabtype INTEGER DEFAULT 0, isedited INTEGER DEFAULT 0,position INTEGER, leftortoppercent  INTEGER, title TEXT,tooltip TEXT,isfile INTEGER, isfocussed INTEGER, content TEXT, \
							FOREIGN KEY(Id) REFERENCES conndetails(Id) ON DELETE CASCADE ) "
#define TABLE_HISTORYDETAILS "CREATE TABLE historydetails (Id INTEGER PRIMARY KEY  NOT NULL, historydata TEXT,FOREIGN KEY(Id) REFERENCES conndetails(Id) ON DELETE CASCADE ) "
#define TABLE_OBDETAILS "CREATE TABLE obdetails (Id INTEGER PRIMARY KEY  NOT NULL, obdb TEXT,FOREIGN KEY(Id) REFERENCES conndetails(Id) ON DELETE CASCADE ) "
#define TABLE_SESSIONDETAILS "CREATE TABLE sessiondetails (Id INTEGER, filename TEXT, isedited INTEGER) "


void 
log(const char * buff)
{
#ifdef _DEBUG
	FILE	*fp = fopen ( "E:\\SQLini_log.log", "a" );
	fprintf(fp, "%s\n" , buff);
	fclose ( fp );
#endif
}

FrameWindow::FrameWindow(HINSTANCE hinstance)
{	
    wyWChar     directory[MAX_PATH];
    wyWChar*    lpfileport = NULL;
    wyString    dirstr, section;
    wyInt32     count;

    m_popupmenucount = 0;
    m_statusbarmgmt = NULL;
    m_editorcolumnline = -1;

    if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyTrue)
    {
        dirstr.SetAs(directory);
		pGlobals->m_prefpersist=wyIni::IniGetInt(GENERALPREFA, "PrefPersist",   GENERALPREF_PAGE, dirstr.GetString());
        wyIni::IniGetString(GENERALPREFA, "EdgeColumn", "0", &section, dirstr.GetString());
        m_editorcolumnline = section.GetAsInt32();
#ifndef COMMUNITY
		m_topromptonimplicit = wyIni::IniGetInt(GENERALPREFA, "PromptinTransaction", 1, dirstr.GetString())? wyTrue: wyFalse;
		m_topromptonclose = wyIni::IniGetInt(GENERALPREFA, "PromptinTransactionClose", 1, dirstr.GetString())? wyTrue: wyFalse;
#endif
        wyIni::IniGetString("UserInterface", "Language", "en", &section, dirstr.GetString());
        count = GetModuleFileName(NULL, directory, MAX_PATH - 1);
	    directory[count - pGlobals->m_modulenamelength] = '\0';
        dirstr.SetAs(directory);
        dirstr.Add(LANGUAGE_DBFILE);

        //Initialize L10nText library
#ifdef _DEBUG
		InitL10n(section.GetString(), dirstr.GetString(), wyFalse, wyTrue, wyFalse); 
#else
		InitL10n(section.GetString(), dirstr.GetString(), wyFalse, wyFalse, wyFalse); 
#endif
    }
    
    m_hwndtooltip       = NULL;
	m_hinstance			= hinstance;
	m_findmsg			= 0;
	m_finddlg			= NULL;
    m_htabinterface     = NULL;
	m_icons				= NULL;
	m_hicon				= NULL;
	m_numicons			= 0;
	m_upgrdchk			= NULL;
	m_hsecondiml		= NULL;
	m_hwndtool			= NULL;
	m_hwndsecondtool	= NULL;
	m_hwndtoolcombo		= NULL;
	memset(&m_tunnelauth, 0, sizeof(TUNNELAUTH));
	memset(&m_rchsplitter, 0, sizeof(RECT));
	memset(&m_rcvsplitter, 0, sizeof(RECT));

	m_iserrdlg = wyFalse;
    m_isredindexhelp = wyFalse;

	//UpgradeCheck object
	m_upgrdchk = new UpgradeCheck();

	m_toolbariconsize = GetToolBarIconSize();
    m_iscloseallmdi = wyFalse;
    m_framewndsaveselection = -1;

	//Connection tab object
	m_conntab = new ConnectionTab();

	m_hwndconntab = NULL;//handle for connection tab
	m_closetab = wyFalse;//flag to check if connection is closed

    m_findtext.Clear();
    ZeroMemory(&m_frstruct, sizeof(FINDREPLACE));
    m_frstruct.lStructSize = sizeof(FINDREPLACE);
    m_findproc = NULL;
    m_languagecount = GetL10nLanguageCount();
	m_showwindowstyle = SW_NORMAL;
	m_hwndrestorestatus = NULL;
	m_savetimerevent = NULL;
	m_sqlyogclosed = wyFalse;
	m_sqlyogcloseevent = NULL;
	m_sessionsaveevent = NULL;
	m_sessionchangeevent = NULL;
	//m_trialbuybrush = NULL;
	m_trialtextfont = NULL;
	m_trialbuyfont = NULL;
	m_mouseoverbuy = wyFalse;
#ifndef COMMUNITY
	m_closealltrans = 0; 
	m_intransaction = 0;
#endif
#ifdef COMMUNITY
	m_commribbon    =  NULL;
#endif
}


FrameWindow::~FrameWindow()
{	
    ShowQueryExecToolTip(wyFalse);

	if(m_upgrdchk)
		delete m_upgrdchk;

	if(m_hmainiml)
		VERIFY(ImageList_Destroy(m_hmainiml)); 
        
	VERIFY(ImageList_Destroy(m_hsecondiml));
	VERIFY(ImageList_Destroy(m_hcomboiml));
	delete m_pcfavoritemenu;
	delete m_pcaddfavorite;
	delete m_pcorganizefavorite;

#ifdef COMMUNITY
	if(m_commribbon)
		delete m_commribbon;
#endif
	if(m_connection->m_enttype == ENT_TRIAL)
	{
		DeleteObject(m_trialbuyfont);
		DeleteObject(m_trialtextfont);
		//DeleteObject(m_trialbuybrush);
	}
    delete m_connection;

	if(m_conntab)
	{
		delete m_conntab;
	}
    delete m_statusbarmgmt;
    CloseL10n();

	

}

wyBool
FrameWindow::Create()
{		
	wyString path; 

	//CreateInitFile();

    m_connection = CreateConnection();

    if(!m_connection)
        return wyFalse;

    m_hinstance = pGlobals->m_entinst?pGlobals->m_entinst:pGlobals->m_hinstance;
    
    if(RegisterWindow(m_hinstance))
    {
		if(!(CreateMainWindow(m_hinstance)))
			return wyFalse;
	}

    LoadMainIcon();

    RegisterButtonDropDown(GetModuleHandle(NULL));
	RegisterQueryWindow(m_hinstance);
	RegisterVSplitter(m_hinstance);
	RegisterHSplitter(m_hinstance);
	RegisterInsertUpdateWindow(m_hinstance);  
	RegisterSDCanvasWindow(m_hinstance);
	RegisterQBCanvasWindow(m_hinstance);
    RegisterTabInterfaceWindow(m_hinstance);
    RegisterDataWindow(m_hinstance);
    RegisterPlainWindow(m_hinstance);
	RegisterPQAResultwindow(m_hinstance);
	RegisterAnnouncementswindow(m_hinstance);
	RegisterAnnouncementsMainwindow(m_hinstance);
	RegisterHTMLInfoTabwindow(m_hinstance);
	RegisterHTMLFormViewWindow(m_hinstance);
	RegisterHTMLDbSearchWindow(m_hinstance);
    RegisterCustomComboBox(GetModuleHandle(NULL));

    #ifndef COMMUNITY
        RegisterCustomComboBox(pGlobals->m_entinst);
        VisualDataDiff::RegisterWindows(m_hinstance);
    #endif
    
	
    InitFavorites(m_hwndmain);
	
	SetConnectionNumber();

	//Sets the community ribbon
	HandleCommunityRibbon();
	
	//Upgrade check
	CheckForUpgrade();
	
    Resize();
		
	m_icons = m_connection->CreateIconList(m_hwndmain, &m_numicons);
	
	///Initialise the recent file menu items.
	InitLatestFiles();

	// Enable/ Disable menu and buttons
	OnActiveConn();

	//Gets the .ini path
	GetSQLyogIniPath(&path);
	if(!path.GetLength())
		return wyFalse;

	InitSplitterPos(path.GetString());

#ifndef COMMUNITY
	InitPQAOptions(path.GetString());
//	InitAutoCompleteCaseSelection(path.GetString());

#endif
		
	return wyTrue;
}
// Function registers the main window.
wyBool  
FrameWindow::RegisterWindow(HINSTANCE hInstance)
{
	WNDCLASS	wndclass;
    HBRUSH      hbr;

	memset(&wndclass, 0, sizeof(WNDCLASS));
	
	// Register the main window.
	wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
	wndclass.lpfnWndProc   = FrameWindow::WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance ;
	wndclass.hIcon         = LoadIcon(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_MAIN));
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = wyTheme::GetBrush(BRUSH_FRAMEWINDOW, &hbr) ? hbr : (HBRUSH)(COLOR_BTNFACE+1);
	wndclass.lpszMenuName  =(LPCWSTR)IDR_MAINMENU ;
	wndclass.lpszClassName = MAIN_WINDOW_CLASS_NAME_STR;	
	
	VERIFY(RegisterClass(&wndclass));

	return wyTrue;
}

// Functions registers the Connection window that is the QueryWindow.
wyBool 
FrameWindow::RegisterQueryWindow(HINSTANCE hInstance)
{
	ATOM		ret;
	WNDCLASS	wndclass;
    HBRUSH      hbr;
	
	// Register the main window.
	wndclass.style         = 0;//CS_HREDRAW | CS_VREDRAW ;
	wndclass.lpfnWndProc   = MDIWindow::WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
	wndclass.hInstance     = hInstance ;
	VERIFY(wndclass.hIcon  = ImageList_ExtractIcon(0, m_hmainiml, 0));  
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = wyTheme::GetBrush(BRUSH_MDICHILD, &hbr) ? hbr : (HBRUSH)(COLOR_BTNFACE+1);
	wndclass.lpszMenuName  = (LPCWSTR)NULL ;
	wndclass.lpszClassName = QUERY_WINDOW_CLASS_NAME_STR;	
	
	VERIFY(ret = RegisterClass(&wndclass));

	m_hicon = wndclass.hIcon;
	return wyTrue;
}

// Functions registers the vertical splitter window used in the query window as divider.

wyBool
FrameWindow::RegisterVSplitter(HINSTANCE hInstance)
{
	ATOM		ret;
	WNDCLASS	wndclass;
    HBRUSH hbr;
	
	// Register the main window.
	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = FrameWindowSplitter::WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
	wndclass.hInstance     = hInstance ;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_SIZEWE);
    wndclass.hbrBackground = wyTheme::GetBrush(BRUSH_VSPLITTER, &hbr) ? hbr : (HBRUSH)(COLOR_BTNFACE+1);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = VSPLITTER_WINDOW_CLASS_NAME_STR ;	
	
	VERIFY(ret = RegisterClass(&wndclass));

	return wyTrue;
}

// Functions registers the horizontal splitter window.
wyBool
FrameWindow::RegisterHSplitter(HINSTANCE hInstance)
{
	ATOM		ret;
	WNDCLASS	wndclass;
	
	// Register the main window.
	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = TabEditorSplitter::WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
	wndclass.hInstance     = hInstance ;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_SIZENS);
	wndclass.hbrBackground =(HBRUSH)(COLOR_BTNFACE+1);
	wndclass.lpszMenuName  = NULL ;
	wndclass.lpszClassName = HSPLITTER_WINDOW_CLASS_NAME_STR;	
	
	VERIFY(ret = RegisterClass(&wndclass));

	return wyTrue;
}

// Function registers the InsertUpdate window that contains all the child windows
wyBool
FrameWindow::RegisterInsertUpdateWindow(HINSTANCE hInstance)
{
	ATOM		ret;
	WNDCLASS	wndclass;
	
	// Register the main window.
	wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wndclass.lpfnWndProc   = DefWindowProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
	wndclass.hInstance     = hInstance ;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = INSERT_UPDATE_WINDOW_CLASS_NAME_STR;	
	
	VERIFY(ret = RegisterClass(&wndclass));

	return wyTrue;
}

// Function registers the InsertUpdate window that contains all the child windows
wyBool
FrameWindow::RegisterDataWindow(HINSTANCE hInstance)
{
	ATOM		ret;
	WNDCLASS	wndclass;
	
	// Register the main window.
	wndclass.style         = CS_DBLCLKS;
	wndclass.lpfnWndProc   = DataView::FrameWndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
	wndclass.hInstance     = hInstance ;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = DATA_WINDOW;	
	
	VERIFY(ret = RegisterClass(&wndclass));

	return wyTrue;
}

// Function registers the SD - Canvas window with white back ground
wyBool
FrameWindow::RegisterSDCanvasWindow(HINSTANCE hInstance)
{
#ifndef COMMUNITY

	ATOM		ret;
	WNDCLASS	wndclass;
	
    // Register the main window.
	wndclass.style         = CS_DBLCLKS;//CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndclass.lpfnWndProc   = TabSchemaDesigner::CanvasWndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
	wndclass.hInstance     = hInstance ;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground =(HBRUSH)(COLOR_WINDOW + 1);
	wndclass.lpszMenuName  = NULL ;
	wndclass.lpszClassName = SD_CANVAS_WINDOW;	
	
	VERIFY(ret = RegisterClass(&wndclass));
#endif

	return wyTrue;
}

// Function registers the QB - Canvas window with white back ground
wyBool
FrameWindow::RegisterQBCanvasWindow(HINSTANCE hInstance)
{
#ifndef COMMUNITY

	ATOM		ret;
	WNDCLASS	wndqbclass;
	
	// Register the main window.
	wndqbclass.style         = CS_DBLCLKS;//CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndqbclass.lpfnWndProc   = TabQueryBuilder::CanvasWndProc;
	wndqbclass.cbClsExtra    = 0;
	wndqbclass.cbWndExtra    = sizeof(HANDLE);
	wndqbclass.hInstance     = hInstance ;
	wndqbclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndqbclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndqbclass.hbrBackground =(HBRUSH)(COLOR_WINDOW + 1);
	wndqbclass.lpszMenuName  = NULL;
	wndqbclass.lpszClassName = QB_CANVAS_WINDOW;	
	
	VERIFY(ret = RegisterClass(&wndqbclass));
#endif

	return wyTrue;
}


// Functions registers the Tab Interface window for Table Operations.
wyBool 
FrameWindow::RegisterTabInterfaceWindow(HINSTANCE hInstance)
{
    ATOM		ret;
	WNDCLASS	wndclass;
	
	// Register the main window.
	wndclass.style         = 0;//CS_HREDRAW | CS_VREDRAW ;
	wndclass.lpfnWndProc   = TableTabInterface::TableTabInterfaceWndProc;//MDIWindow::WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
	wndclass.hInstance     = hInstance ;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
	wndclass.lpszMenuName  = (LPCWSTR)NULL ;
	wndclass.lpszClassName = TABLE_TABINTERFACE_WINDOW;	
	
	VERIFY(ret = RegisterClass(&wndclass));

	//m_hicon = wndclass.hIcon;
	return wyTrue;

}

/*
wyBool
FrameWindow::RegisterTabIntBottomFrameWindow(HINSTANCE hinstance)
{
    ATOM		ret;
	WNDCLASS	wndclass;
	
	// Register the main window.
	wndclass.style         = 0;//CS_HREDRAW | CS_VREDRAW ;
	wndclass.lpfnWndProc   = TableTabInterface::TabIntBottomFrameWndProc;//TableTabInterfaceWndProc;//MDIWindow::WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
	wndclass.hInstance     = hinstance ;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    //wndclass.hbrBackground = (HBRUSH)(COLOR_3DLIGHT);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(COLOR_3DSHADOW);
	wndclass.lpszMenuName  = (LPCWSTR)NULL ;
	wndclass.lpszClassName = TTI_BOTTOMFRAME_WNDCLS;	
	
	VERIFY(ret = RegisterClass(&wndclass));

	//m_hicon = wndclass.hIcon;

    return wyTrue;
}
*/
wyBool
FrameWindow::RegisterPlainWindow(HINSTANCE hInstance)
{
    ATOM		ret;
	WNDCLASS	wndclass;

    // Register the Plain window.
    wndclass.style         = 0;//CS_HREDRAW | CS_VREDRAW ;
    wndclass.lpfnWndProc   = TableTabInterfaceTabMgmt::SubTabsCommonWndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
	wndclass.hInstance     = hInstance ;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wndclass.lpszMenuName  = (LPCWSTR)NULL ;
	wndclass.lpszClassName = TTI_SUBTABS_COMMONWNDCLS;
	
	VERIFY(ret = RegisterClass(&wndclass));

	//m_hicon = wndclass.hIcon;
	return wyTrue;
}

wyBool
FrameWindow::RegisterPQAResultwindow(HINSTANCE hinstanceent)
{
	ATOM		ret;
	WNDCLASS	wndclass;
	
	// Register the main window.
	wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
#ifndef COMMUNITY
	switch(pGlobals->m_pcmainwin->m_connection->m_enttype)
	{
	case ENT_ULTIMATE:
	case ENT_TRIAL:
		wndclass.lpfnWndProc   = QueryAnalyzer::PQAHtmlWndProc;
		break;
	default:
	wndclass.lpfnWndProc   = QueryAnalyzerBase::PQAHtmlWndProc;
	}
#else
	wndclass.lpfnWndProc   = QueryAnalyzerBase::PQAHtmlWndProc;
#endif
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
	wndclass.hInstance     = hinstanceent ;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground =(HBRUSH)(COLOR_BTNFACE+1);
	wndclass.lpszMenuName  = NULL ;
	wndclass.lpszClassName = PQA_RESULT_WINDOW;	
	
	VERIFY(ret = RegisterClass(&wndclass));

	return wyTrue;
}

wyBool
FrameWindow::RegisterAnnouncementswindow(HINSTANCE hinstanceent)
{
	ATOM		ret;
	WNDCLASS	wndclass;
	
	// Register the main window.
	wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndclass.lpfnWndProc   = Announcements::AnnounceWndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
	wndclass.hInstance     = hinstanceent ;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground =(HBRUSH)(COLOR_BTNFACE+1);
	wndclass.lpszMenuName  = NULL ;
	wndclass.lpszClassName = ANNOUNCEMENTS_WINDOW;	
	
	VERIFY(ret = RegisterClass(&wndclass));

	return wyTrue;
}

wyBool
FrameWindow::RegisterAnnouncementsMainwindow(HINSTANCE hinstanceent)
{
	ATOM		ret;
	WNDCLASS	wndclass;
	
	// Register the main window.
	wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndclass.lpfnWndProc   = Announcements::AnnounceWndMainProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
	wndclass.hInstance     = hinstanceent ;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground =(HBRUSH)(COLOR_BTNFACE+1);
	wndclass.lpszMenuName  = NULL ;
	wndclass.lpszClassName = ANNOUNCEMENTS_MAIN_WINDOW;	
	
	VERIFY(ret = RegisterClass(&wndclass));

	return wyTrue;
}

wyBool
FrameWindow::RegisterHTMLInfoTabwindow(HINSTANCE hinstanceent)
{
	ATOM		ret;
	WNDCLASS	wndclass;
	
	// Register the main window.
	wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndclass.lpfnWndProc   = ObjectInfo::HtmlWndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
	wndclass.hInstance     = hinstanceent ;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground =(HBRUSH)(COLOR_BTNFACE+1);
	wndclass.lpszMenuName  = NULL ;
	wndclass.lpszClassName = INFO_HTML_WINDOW;	
	
	VERIFY(ret = RegisterClass(&wndclass));

	return wyTrue;
}

wyBool
FrameWindow::RegisterHTMLDbSearchWindow(HINSTANCE hinstanceent)
{
#ifndef COMMUNITY	
	ATOM		ret;
	WNDCLASS	wndclass;
	
	// Register the main window.
	wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndclass.lpfnWndProc   = TabDbSearch::HtmlWndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
	wndclass.hInstance     = hinstanceent ;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground =(HBRUSH)(COLOR_BTNFACE+1);
	wndclass.lpszMenuName  = NULL ;
	wndclass.lpszClassName = DBSEARCH_HTML_WINDOW;	
	
	VERIFY(ret = RegisterClass(&wndclass));
#endif
	return wyTrue;
}

wyBool
FrameWindow::RegisterHTMLFormViewWindow(HINSTANCE hinstanceent)
{
#ifndef COMMUNITY	
	ATOM		ret;
	WNDCLASS	wndclass;
	
	// Register the main window.
	wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndclass.lpfnWndProc   = FormView::HtmlWndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
	wndclass.hInstance     = hinstanceent ;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground =(HBRUSH)(COLOR_BTNFACE+1);
	wndclass.lpszMenuName  = NULL ;
	wndclass.lpszClassName = FORMVIEW_HTML_WINDOW;	
	
	VERIFY(ret = RegisterClass(&wndclass));
#endif
	return wyTrue;
}

wyBool
FrameWindow::RegisterHTTPErrorWindow(HINSTANCE hinstanceent)
{
#ifndef COMMUNITY

	ATOM		ret;
	WNDCLASS	wndclass;
	
	// Register the main window.
	wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndclass.lpfnWndProc   = CHttpError::HTTPErrorHtmlWndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
	wndclass.hInstance     = hinstanceent ;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground =(HBRUSH)(COLOR_BTNFACE+1);
	wndclass.lpszMenuName  = NULL ;
	wndclass.lpszClassName = HTTP_ERROR_WINDOW;	
	
	VERIFY(ret = RegisterClass(&wndclass));

#endif

	return wyTrue;
}


void
FrameWindow::SetYogMenuOwnerDraw()
{
	HMENU       hmenu;
    wyInt32     i = 0, ignorecount = 0;
    
	
    if(IsWindowMaximised() && wyTheme::IsSysmenuEnabled(GetActiveWin()->m_hwnd))
    {
        i = 1;
        ignorecount = 3;
    }

    hmenu = GetMenu(m_hwndmain);
    
    if(wyTheme::GetBrush(BRUSH_MENUBAR, NULL))
    {
        wyTheme::SetMenuItemOwnerDraw(hmenu, i, ignorecount, BRUSH_MENUBAR, wyFalse);
    }
    else
    {
        wyTheme::SetMenuItemOwnerDraw(hmenu, i, ignorecount, BRUSH_BTNFACE, wyFalse);
    }
}

// Function creates the main window i.e the main application window.
wyBool
FrameWindow::CreateMainWindow(HINSTANCE hinstance)
{
	wyUInt32    exstyle	= NULL;
	wyUInt32    style   =  WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
	HWND	    hwnd;
	wyBool		val;
	wyWChar		*appname = NULL;
	wyString tempstr;
	tempstr.SetAs(pGlobals->m_appname.GetAsWideChar());
#ifndef ENTERPRISE

#ifdef _WIN64
	tempstr.AddSprintf(" 64");
#else
	tempstr.AddSprintf(" 32");
#endif

#endif
	appname = tempstr.GetAsWideChar();
	// Create the main window.
	VERIFY(hwnd	= CreateWindowEx(exstyle, MAIN_WINDOW_CLASS_NAME_STR, appname,
								  style, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
								  NULL, NULL, hinstance, this));

	// keep track of the last window.
	m_lastfocus = hwnd;

	// Show and update the window.
	
    LocalizeMenu(GetMenu(hwnd));
    SetYogMenuOwnerDraw();
	//ShowWindow(hwnd, SW_NORMAL);
	//VERIFY(ret = UpdateWindow(hwnd));

    MoveToInitPos(hwnd);	

     val = m_connection->CheckRegistration(m_hwndmain, this);
	
	 if(!pGlobals->m_entlicense.CompareI("Professional"))
	 {
		 RemoveMenu(GetMenu(hwnd), MNUTRANSACTION_INDEX - 1, MF_BYPOSITION);
	 }

	 ///Handle UUID for upgrade check
	 m_connection->HandleApplicationUUID();
	 
	 if(val == wyTrue)
	 {
		 CreateToolBarWindow();

#ifdef ENTERPRISE
	 	 SetMainWindowTitle();		 	 
#endif
	 }
	 return val;
}

VOID
FrameWindow::SetMainWindowTitle()
{
	wyString tempstr;
	
	if(pGlobals->m_entlicense.GetLength() == 0)
		return;
	tempstr.SetAs(pGlobals->m_appname.GetAsWideChar());
#ifdef _WIN64
	tempstr.AddSprintf(" 64");
#else
	tempstr.AddSprintf(" 32");
#endif	
	SetWindowText(m_hwndmain, tempstr.GetAsWideChar());
}

wyBool
FrameWindow::MoveToInitPos(HWND hwnd)
{
	RECT	    rc;
	wyInt32     lstyle = 0;
    wyWChar     *lpfileport=0;
	wyWChar     directory[MAX_PATH + 1] = {0};
	wyString	dirstr;
	wyInt32	    screenwidth, width, screenheight, height;
	wyInt32		virtx, virty;
	wyInt32		diffht, diffwd;
	wyInt32		xprim, yprim;
	static const char* (CDECL *pwine_get_version)(void);
	HMODULE hntdll;

    hntdll = GetModuleHandle(L"ntdll.dll");
	if (hntdll)
	{
		if(GetProcAddress(hntdll, "wine_get_version"))
		{
			m_showwindowstyle = SW_NORMAL;
			return wyFalse;
		}
	}

    if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyFalse)
    {
		m_showwindowstyle = SW_NORMAL;
        return wyFalse;
    }
	
	dirstr.SetAs(directory);


    rc.left = wyIni::IniGetInt(SECTION_NAME, "Left", 0, dirstr.GetString());
	rc.top	= wyIni::IniGetInt(SECTION_NAME, "Top", 0, dirstr.GetString());
	rc.right = wyIni::IniGetInt(SECTION_NAME, "Right", 600, dirstr.GetString());
	rc.bottom = wyIni::IniGetInt(SECTION_NAME, "Bottom", 600, dirstr.GetString());
	
	/*If change from extended monitor to single , 
	its required if closed the SQLyog last time from 2nd monitor.
	So we moved to Most Left
	*/
	screenwidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	screenheight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	
	virtx = GetSystemMetrics(SM_XVIRTUALSCREEN);
	virty = GetSystemMetrics(SM_YVIRTUALSCREEN);
		
	xprim = GetSystemMetrics(SM_CXSCREEN);
	yprim = GetSystemMetrics(SM_CYSCREEN);
	
	width = rc.right - rc.left;
	height = rc.bottom - rc.top;

	diffht = screenheight + virty;
    diffwd = screenwidth + virtx; 
				
	//check whether the 2nd monitor is on corner or not before closing application
	if(screenheight >= yprim && virtx >=0 && rc.left <= (virtx))
	{
		if(rc.top <= 0)
		{
			rc.left = 0;
			rc.right = rc.left + width;

			rc.top = 0;
			rc.bottom = rc.top + height;
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
	

	if(rc.left >= screenwidth) //if right side 
	{
		rc.left =  screenwidth - width;
		rc.right = rc.left + width;
	}

	// If SQLyog was in 2nd monitor and moved 2nd monitor from right to left & open sqlyog 
	else if(rc.left >= diffwd) 
	{
		rc.left =  diffwd - width;
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
	
	// If SQLyog was in 2nd monitor and moved 2nd monitor from bottom to top & open sqlyog 
	else if(rc.top >= diffht)
	{
		rc.top = diffht - height;
		rc.bottom = rc.top + height;
	}

	if(rc.bottom <= virty) // if top
	{
		rc.top = 0;
		rc.bottom = height;
	}
	
	MoveWindow(hwnd, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, FALSE);  

	lstyle = wyIni::IniGetInt(SECTION_NAME, "Maximize", 0, dirstr.GetString());

    if(lstyle == 1)
		m_showwindowstyle = SW_MAXIMIZE;
	else
		 m_showwindowstyle = SW_NORMAL;

	return wyTrue;
}

wyBool
FrameWindow::MoveToInitPosByrc(HWND hwnd, RECT	*rc)
{
	//RECT	    rc;
//	wyInt32     lstyle = 0;
//    wyWChar     *lpfileport=0;
//	wyWChar     directory[MAX_PATH + 1] = {0};
	wyString	dirstr;
	wyInt32	    screenwidth, width, screenheight, height;
	wyInt32		virtx, virty;
	wyInt32		diffht, diffwd;
	wyInt32		xprim, yprim;

   /* if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyFalse)
    {
		m_showwindowstyle = SW_NORMAL;
        return wyFalse;
    }*/
	
	//dirstr.SetAs(directory);


    //rc.left = wyIni::IniGetInt(SECTION_NAME, "Left", 0, dirstr.GetString());
	//rc.top	= wyIni::IniGetInt(SECTION_NAME, "Top", 0, dirstr.GetString());
	//rc.right = wyIni::IniGetInt(SECTION_NAME, "Right", 600, dirstr.GetString());
	//rc.bottom = wyIni::IniGetInt(SECTION_NAME, "Bottom", 600, dirstr.GetString());
	
	/*If change from extended monitor to single , 
	its required if closed the SQLyog last time from 2nd monitor.
	So we moved to Most Left
	*/
	screenwidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	screenheight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	
	virtx = GetSystemMetrics(SM_XVIRTUALSCREEN);
	virty = GetSystemMetrics(SM_YVIRTUALSCREEN);
		
	xprim = GetSystemMetrics(SM_CXSCREEN);
	yprim = GetSystemMetrics(SM_CYSCREEN);
	
	width = rc->right - rc->left;
	height = rc->bottom - rc->top;

	diffht = screenheight + virty;
    diffwd = screenwidth + virtx; 
				
	//check whether the 2nd monitor is on corner or not before closing application
	if(screenheight >= yprim && virtx >=0 && rc->left <= (virtx))
	{
		if(rc->top <= 0)
		{
			rc->left = 0;
			rc->right = rc->left + width;

			rc->top = 0;
			rc->bottom = rc->top + height;
		} 

		else if(rc->top >= yprim) 
		{
			rc->left = 0;
			rc->right = rc->left + width;
			
			rc->top = yprim - height;
			rc->bottom = rc->top + height;
		}
	}

	else if(screenheight >= yprim && virtx <= 0 && rc->left >= (xprim))
	{
		if(rc->top <= 0)
		{
			rc->left = xprim - width;
			rc->right = rc->left + width;

			rc->top = 0;
			rc->bottom = rc->top + height;
		}

		else if(rc->top >= yprim)
		{
			rc->left = xprim - width;
			rc->right = rc->left + width;

			rc->top = yprim - height;
			rc->bottom = rc->top + height;
		}
	}
	

	if(rc->left >= screenwidth) //if right side 
	{
		rc->left =  screenwidth - width;
		rc->right = rc->left + width;
	}

	// If SQLyog was in 2nd monitor and moved 2nd monitor from right to left & open sqlyog 
	else if(rc->left >= diffwd) 
	{
		rc->left =  diffwd - width;
		rc->right = rc->left + width;
	}

	if(rc->right <= virtx) // lf left side
	{
		rc->left = 0;
		rc->right = width;
	}
	
	if(rc->top >= screenheight) //if down 
	{
		rc->top = screenheight - height;
		rc->bottom = rc->top + height;		
	}
	
	// If SQLyog was in 2nd monitor and moved 2nd monitor from bottom to top & open sqlyog 
	else if(rc->top >= diffht)
	{
		rc->top = diffht - height;
		rc->bottom = rc->top + height;
	}

	if(rc->bottom <= virty) // if top
	{
		rc->top = 0;
		rc->bottom = height;
	}
	
	MoveWindow(hwnd, rc->left, rc->top, rc->right-rc->left, rc->bottom-rc->top, FALSE);  


	//lstyle = wyIni::IniGetInt(SECTION_NAME, "Maximize", 0, dirstr.GetString());

    /*if(lstyle == 1)
		m_showwindowstyle = SW_MAXIMIZE;
	else
		 m_showwindowstyle = SW_NORMAL;*/

	return wyTrue;
}

wyBool
FrameWindow::WriteInitPos(HWND hwnd)
{
	RECT        rc;
    wyWChar      *lpfileport=0;
    wyString    value, dirstr;
    wyInt32     lstyle = 0, tmpleft = 0, tmptop = 0;;
	wyWChar      directory[MAX_PATH + 1] = {0};

    if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyFalse)
        return wyFalse;

	VERIFY(GetWindowRect(hwnd, &rc));
	
	// bug if the window is in minimized position then wrong values are entered and you cannot 
	// change the sizing...so we need to see that only correct values are entered
    lstyle = GetWindowLongPtr(hwnd, GWL_STYLE);

	if(lstyle & WS_MINIMIZE)
    {
        rc.left     = 0;
        rc.top      = 0;
        rc.bottom   = 600;
        rc.right    = 600;
    }
	
	dirstr.SetAs(directory);

	/*if(rc.left < 0)
	{
		tmpleft = -rc.left;
		//rc.left = 0;
	}

	if(rc.top < 0)
	{
		tmptop = -rc.top;
		//rc.top = 0;
	}*/

	// write the values.
	value.Sprintf("%d", rc.left);
	wyIni::IniWriteString(SECTION_NAME, "Left", value.GetString(), dirstr.GetString());

	value.Sprintf("%d", rc.top);
	wyIni::IniWriteString(SECTION_NAME, "Top", value.GetString(), dirstr.GetString());
	
	value.Sprintf("%d", rc.right + tmpleft);
	wyIni::IniWriteString(SECTION_NAME, "Right", value.GetString(), dirstr.GetString());
	
	value.Sprintf("%d", rc.bottom + tmptop);
	wyIni::IniWriteString(SECTION_NAME, "Bottom", value.GetString(), dirstr.GetString());

    
    if(lstyle & WS_MAXIMIZE)
		value.Sprintf("1");
	else
		value.Sprintf("0");

	wyIni::IniWriteString(SECTION_NAME, "Maximize", value.GetString(), dirstr.GetString());

	return wyTrue;

}

// The window procedure for MainWindow. 
// It creates all its child window in WM_CREATE message.
LRESULT	CALLBACK
FrameWindow::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	FrameWindow*	    pcmainwin = (FrameWindow *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	wyInt32             ret;
	MDIWindow*          wnd;
	wyInt32		        tabimageid;
	LPNMHDR			    lpnmhdr =(LPNMHDR)lparam;
    LPDRAWITEMSTRUCT    lpds;

	switch(message)
	{
	
	case WM_NCCREATE:
		pcmainwin	= (FrameWindow *)(((CREATESTRUCT*)lparam)->lpCreateParams);
		pcmainwin->SetHwnd(hwnd);
		SetWindowLongPtr(hwnd, GWLP_USERDATA,(LONG_PTR)pcmainwin);
		break;

	case WM_CREATE:
		pcmainwin->OnCreate();
		return 0;

	case UM_COMBOCHANGE:
		pcmainwin->OnToolComboSelChange();
		break;
	
    case UM_SETFOCUSAFTERFINDMSG:
        SetActiveWindow((HWND)wparam);
        break;
	
	case UM_SETFOCUS:
		SetFocus((HWND)wparam);
        
    case WM_SETACTIVEWINDOW:
		SetFocus((HWND)wparam);
        SetActiveWindow((HWND)wparam);
        break;

    case UM_UPDATEMAINTOOLBAR:
		{								
			pcmainwin->EnableToolButtonsAndCombo(pcmainwin->m_hwndtool, pcmainwin->m_hwndsecondtool, pcmainwin->m_hwndtoolcombo, wparam ? wyTrue : wyFalse, wyTrue);					
		}
		break;

    case MAX_WINDOW:
        SetWindowLongPtr(hwnd, GWL_STYLE, GetWindowLongPtr(hwnd, GWL_STYLE) | WS_MAXIMIZE);
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        break;

	case WM_COMMAND:
		pcmainwin->OnWmCommand(wparam);
		break;

	case WM_MENUCOMMAND:
        pcmainwin->OnWmMenuCommand(hwnd, wparam, lparam);
		break;

	case WM_MEASUREITEM:
        return wyTheme::MeasureMenuItem((LPMEASUREITEMSTRUCT)lparam, GetMenu(hwnd), pcmainwin->m_popupmenucount ? wyTrue : wyFalse);

	case WM_DRAWITEM:
        if(((LPDRAWITEMSTRUCT)lparam)->CtlType == ODT_MENU && wparam == 0)
        {
            return wyTheme::DrawMenuItem((LPDRAWITEMSTRUCT)lparam, GetMenu(hwnd));
        }
        else
        {
            lpds = (LPDRAWITEMSTRUCT)lparam;
            wyTheme::DrawStatusBarPart((LPDRAWITEMSTRUCT)lparam, pcmainwin->m_statusbarmgmt ? pcmainwin->m_statusbarmgmt->GetPartText(lpds->itemID) : L"");
            return 1;
        }
        break;

	case WM_ACTIVATE:
        pcmainwin->OnActivate(wparam);
		break;

	case WM_MENUSELECT:
		return pcmainwin->OnWmMenuSelect((HMENU)lparam, LOWORD(wparam));		

	case WM_HELP:
		{
			VERIFY(wnd = GetActiveWin());

			if(!wnd)
				ShowHelp("http://sqlyogkb.webyog.com/category/22-getting-started");
			else
			{
				tabimageid = wnd->m_pctabmodule->GetActiveTabImage();

				//If user presses Help in dialog of SchemaDesigner
				if(tabimageid == IDI_SCHEMADESIGNER_16)
					ShowHelp("http://sqlyogkb.webyog.com/article/166-the-sqlyog-schema-designer");
			
				//If user presses Help in dialog of QueryBuilder
				else if(tabimageid == IDI_QUERYBUILDER_16)
					ShowHelp("http://sqlyogkb.webyog.com/article/58-the-sqlyog-query-builder");
				else
				{
					//Spatial data type err dialog is present,if user preeses the Help in that dialog
					if(pcmainwin->m_iserrdlg == wyTrue)
						ShowHelp("http://sqlyogkb.webyog.com/article/102-data-manipulation");
                    else if(pcmainwin->m_isredindexhelp == wyTrue)
                        ShowHelp("http://sqlyogkb.webyog.com/article/82-redundant-index-analyzer");
					else
						ShowHelp("http://sqlyogkb.webyog.com/category/22-getting-started");
				}	
			}
			return wyTrue;
		}

	// Sends all the existing MDI child window to ask whther to close or not.
	// This is required if a user has enetered some text and selected cancel when asked to save.
	case WM_CLOSE:

        //set the flag to identify frame window closing
        pcmainwin->m_iscloseallmdi = wyTrue;
        
        if(pcmainwin->OnWmClose(hwnd) == wyFalse)
        {
            //reset the frame window variable used in custom save dialog
            pcmainwin->m_iscloseallmdi = wyFalse;
            pcmainwin->m_framewndsaveselection = -1;
            return 0;
        }
		break;

	case WM_TIMER : 
		if(wparam == CONRESTORE_TIMER)
		{
            PostMessage(pcmainwin->m_hwndrestorestatus, UM_CLOSE_RESTORE_STATUS, 0, 0);
			KillTimer(hwnd, CONRESTORE_TIMER);
			if(pGlobals && pGlobals->m_pcquerywnd !=NULL && pGlobals->m_pcquerywnd->m_pcqueryobject!=NULL)
			{
				if(pGlobals->m_database.GetLength())
				{
					pGlobals->m_pcquerywnd->m_pcqueryobject->m_seldatabase.SetAs(pGlobals->m_database);
					pGlobals->m_pcquerywnd->m_pcqueryobject->SyncObjectDB(pGlobals->m_pcquerywnd);
				}
				else
				{
					pGlobals->m_pcquerywnd->m_database.SetAs("");
					pGlobals->m_pcmainwin->AddTextInCombo(NODBSELECTED);
				}
			}
			ShowWindow(hwnd, pcmainwin->m_showwindowstyle);
			pcmainwin->m_showwindowstyle = SW_HIDE;
			SetForegroundWindow(hwnd);

		}
		else
		if(wparam == CONSAVE_TIMER)
		{
			//set event
			SetEvent(pcmainwin->m_savetimerevent);
			//KillTimer(hwnd, CONRESTORE_TIMER);
		}
		break;
	case WM_CLOSEALLWINDOW:
		EnumChildWindows((HWND)pGlobals->m_hwndclient, FrameWindow::CloseEnumProc,(LPARAM)lparam);
		break;
	
	case WM_DESTROY:
		pcmainwin->DestroyResources();
		break;

	case WM_NCDESTROY:
		delete[] pcmainwin->m_icons;
		return 0;

	case WM_INITMENUPOPUP:
        if(HIWORD(lparam) == TRUE)
        {
            break;
        }

        wnd = GetActiveWin();

        if(LOWORD(lparam) != 0 || IsWindowMaximised() == wyFalse || wyTheme::IsSysmenuEnabled(wnd ? wnd->m_hwnd : NULL) == wyFalse)
        {
            wyTheme::SetMenuItemOwnerDraw((HMENU)wparam);
        }

        pcmainwin->m_popupmenucount++;

		if(pcmainwin->OnWmInitPopup(wparam, lparam) == wyFalse)
        {
			return 0;
        }

		break;

	case WM_UNINITMENUPOPUP:
        pcmainwin->m_popupmenucount--;
		pcmainwin->HandleOnMenuDestroy((HMENU)wparam);
		break;

	case WM_NOTIFY:
        if((ret = wyTheme::DrawToolBar(lparam, wyTrue)) != -1)
        {
            return ret;
        }

		if(pcmainwin->ONWmMainWinNotify(hwnd, lparam, wparam) == 0)
		{
            return 0;
		}
        else if(lpnmhdr->code == CTCN_SELCHANGING)
        {
            FrameWindow::ShowQueryExecToolTip(wyFalse);
            return 1;
        }
        else if(lpnmhdr->code == CTCN_TABCLOSING && lpnmhdr->idFrom == IDC_CONNECTIONTAB)
		{
			if(pcmainwin->m_closetab == wyTrue)
            {
                pcmainwin->m_closetab = wyFalse; 
                return 1;
            }
            
            return 0;
		}

		break;

	case WM_SIZE:
        return pcmainwin->OnWmSize(wparam);	
        
	case UM_DESTROY_CONNTAB:
		if(pcmainwin->m_hwndconntab)
		{
			DestroyWindow(pcmainwin->m_hwndconntab);
			pcmainwin->m_hwndconntab = NULL;
		}
		return 1;

    case WM_NCPAINT:
    case WM_NCACTIVATE:
        {
            wyInt32 ret;
            HDC hdc = GetWindowDC(hwnd);
            RECT rcwnd, rcclient, rect;
            GetWindowRect(hwnd, &rcwnd);
            GetClientRect(hwnd, &rcclient);
            MapWindowRect(hwnd, NULL, &rcclient);
            
            ret = DefFrameProc(hwnd, pGlobals->m_hwndclient, message, wparam, lparam);
			HBRUSH hbr = GetStockBrush(LTGRAY_BRUSH)/*GetSysColorBrush(COLOR_BTNFACE)*/;
            //wyTheme::GetBrush(BRUSH_FRAMEWINDOW, &hbr);
            rect.left = rect.top = (rcclient.left - rcwnd.left);
            rect.top = (rcclient.top - rcwnd.top) - 1;
            rect.bottom = rect.top +1;
            rect.right = rcclient.right - rcwnd.left;

            if(rect.right - rect.left && rect.bottom - rect.top)
            {
                FillRect(hdc, &rect, hbr);
            }
            ReleaseDC(hwnd, hdc);
            return ret;
        }
        break;
	}

  	return DefFrameProc(hwnd, pGlobals->m_hwndclient, message, wparam, lparam);
}

void 
FrameWindow::DestroyResources()
{
	HMENU			menu, submenu, subsubmenu;
	wyInt32			count, menucount, itemcount = 0, submenucount, subsubmenucount;
    wyInt32         subsubcount = 0;
	MENUITEMINFO	mif;
	wyWChar          *newbuf = NULL;

	menu = GetMenu(m_hwndmain);
	menucount = GetMenuItemCount(menu);

	for(count = 0; count < 0/*menucount*/; count++)
    {
		VERIFY(submenu = GetSubMenu(menu, count));
		/*	starting from v5.1 we support favorites menu and since we keep some other data
			as long data we dont owner draw it, thus we check for the index number of the menu here
			and dont call the sub procedure to owner draw it */
		
		submenucount = GetMenuItemCount(submenu);

		for(itemcount = 0; itemcount < submenucount; itemcount++)
		{
            subsubmenu = GetSubMenu(submenu, itemcount);

            if(subsubmenu)
            {
                subsubmenucount = GetMenuItemCount(subsubmenu);

		        for(subsubcount = 0; subsubcount < subsubmenucount; subsubcount++)
		        {
			        memset(&mif, 0, sizeof(mif));

                    // first we know whther its a separator.
			        mif.cbSize = sizeof(mif);
			        mif.fMask  = MIIM_TYPE | MIIM_DATA | MIIM_ID;
			        mif.fType  = MFT_SEPARATOR;
        			
			        VERIFY(GetMenuItemInfo(subsubmenu, subsubcount, TRUE, &mif));

			        newbuf = (wyWChar*)mif.dwItemData;

			        if(mif.fType & MFT_SEPARATOR )
				        continue;
        			
			        if(newbuf)
				        free(newbuf);

                    newbuf = NULL;
		        }
            }

			memset(&mif, 0, sizeof(mif));

			// first we know whther its a separator.
			mif.cbSize = sizeof(mif);
			mif.fMask  = MIIM_TYPE | MIIM_DATA | MIIM_ID;
			mif.fType  = MFT_SEPARATOR;
			
			VERIFY(GetMenuItemInfo(submenu, itemcount, TRUE, &mif));

			newbuf = (wyWChar*)mif.dwItemData;

			if(mif.fType & MFT_SEPARATOR )
				continue;
			
			if(newbuf)
				free(newbuf);

            newbuf = NULL;
		}
	}

	if(m_hicon)
		DestroyIcon(m_hicon);

	PostQuitMessage(0);  

	return;
}


void 
FrameWindow::RecursiveMenuEnable(HMENU hsubmenu, wyBool iswindowmenu, wyInt32 state)
{
  wyInt32 isubmenuitemindex;
  wyInt32 isubmenuitemcount;
  wyInt32 isubmenuitemid;

  // Reset the menu counter, for running through all available items
  isubmenuitemindex=0;

  // Retrieve the count of the subitems
  isubmenuitemcount = GetMenuItemCount(hsubmenu);

  if(iswindowmenu)
    isubmenuitemcount = 0;//WND_MNU_ITEMS; // number of items in menu window except the connection menuitems.

  for(isubmenuitemindex = 0; isubmenuitemindex < isubmenuitemcount; isubmenuitemindex++)
  {
    isubmenuitemid = GetMenuItemID(hsubmenu,isubmenuitemindex);

    if(isubmenuitemid != -1)
      EnableMenuItem(hsubmenu,isubmenuitemid, state | MF_BYCOMMAND);
    else
      RecursiveMenuEnable(GetSubMenu(hsubmenu, isubmenuitemindex), wyFalse, state);
   }

  return;
}

// Functions to set various handle value of the members and to get the handles when needed.
void
FrameWindow::SetHwnd(HWND hwndmain)
{
	m_hwndmain = hwndmain;

	return;
}

void
FrameWindow::SetMDIHwnd(HWND hwndmdiclient)
{
	m_hwndmdiclient = hwndmdiclient;

	return;
}

void 
FrameWindow::SetStatusHwnd(HWND hwndstatus)
{
	m_hwndstatus = hwndstatus;

	return;
}

void
FrameWindow::SetToolHwnd(HWND hwndtool)
{
	m_hwndtool = hwndtool;

	return;
}

void 
FrameWindow::SetToolCombo(HWND hwndtoolcombo)
{
	m_hwndtoolcombo = hwndtoolcombo;

	return;
}

HWND
FrameWindow::GetHwnd()
{
	return m_hwndmain;
}

HINSTANCE
FrameWindow::GetHinstance()
{
	return m_hinstance;
}

HWND
FrameWindow::GetStatusHwnd()
{
	return m_hwndstatus;
}

HWND
FrameWindow::GetToolHwnd()
{
	return m_hwndtool;
}

HWND
FrameWindow::GetToolCombo()
{
	return m_hwndtoolcombo;
}

HWND
FrameWindow::GetMDIWindow()
{
	return m_hwndmdiclient;
}

// Function creates the main window.
wyBool
FrameWindow::CreateToolBarWindow()
{
	HWND	 hwndtool;
	wyUInt32 exstyle = NULL;
	wyUInt32 style   = TBSTYLE_CUSTOMERASE | WS_CHILD | CCS_NORESIZE | CCS_NOPARENTALIGN | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT | CCS_NODIVIDER;
	wyUInt32 daysleft = TRIAL_DAYS_LEFT;
	static const char * (CDECL *pwine_get_version)(void);
#ifndef COMMUNITY
	WNDPROC wndproc;
	HMODULE hntdll;
#endif

	//Make sure destroy all toolbar resources during the chage of icon size
	DestroyToolBarResources();
		
	// Create the toolbar.
	VERIFY(hwndtool	= CreateWindowEx(exstyle, TOOLBARCLASSNAME, NULL, style, 0,0,0,0, GetHwnd(),
								  (HMENU)IDC_TOOLBAR,(HINSTANCE)GetHinstance(), NULL));
	VERIFY(m_hwndsecondtool = CreateWindowEx(exstyle, TOOLBARCLASSNAME, NULL, style, 0,0,0,0, GetHwnd(),
								  (HMENU)IDC_TOOLBAR2,(HINSTANCE)GetHinstance(), NULL));

    //wndproc = (WNDPROC)SetWindowLongPtr(hwndtool, GWLP_WNDPROC, (LONG)FrameWindow::ToolbarWndProc);
    //SetWindowLongPtr(hwndtool, GWLP_USERDATA, (LONG)wndproc);

    

	// Set the class toolbar handle to newly created window.
	SetToolHwnd(hwndtool);

	// Create tool buttons.
	VERIFY(CreateToolButtons(hwndtool));
	VERIFY(m_connection->CreateOtherToolButtons(m_hwndsecondtool, m_hsecondiml));

	// Create the combo box which will hold the databases of the connections.
	VERIFY(m_hwndtoolcombo = CreateToolCombo(hwndtool));
	//create static text for trial no of days left
	//create button for buy
	SetToolCombo(m_hwndtoolcombo);

	SetToolComboFont();
	m_connection->SetToolComboImageList(this);
	AddTextInCombo(NODBSELECTED);
#ifndef COMMUNITY
	if(pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_TRIAL)
	{
		hntdll = GetModuleHandle(L"ntdll.dll");
		if(hntdll)
		{
			//pwine_get_version = (void *)GetProcAddress(hntdll, "wine_get_version");
			if(!GetProcAddress(hntdll, "wine_get_version"))
			{
				GetTimeDifference(&daysleft);
				m_trialtext.Sprintf("%d ", daysleft);
				if(daysleft > 1)
				{
					m_trialtext.AddSprintf(_("days trial left"));
				}
				else
				{
					m_trialtext.AddSprintf(_("day trial left"));
				}
				wndproc = (WNDPROC)SetWindowLongPtr(m_hwndsecondtool, GWLP_WNDPROC, (LONG_PTR)FrameWindow::ToolbarWndProc);
				SetWindowLongPtr(m_hwndsecondtool, GWLP_USERDATA, (LONG_PTR)wndproc);
				VERIFY(m_hwndtrialtext = CreateTrialText(hwndtool));
				SetWindowText(m_hwndtrialtext, m_trialtext.GetAsWideChar());
				EnableWindow(m_hwndtrialtext, TRUE);
				ShowWindow(m_hwndtrialtext, TRUE);
				VERIFY(m_hwndtrialbuy = CreateTrialBuy(hwndtool));
				wndproc = (WNDPROC)SetWindowLongPtr(m_hwndtrialbuy, GWLP_WNDPROC, (LONG_PTR)FrameWindow::TrialbuyWndProc);
				SetWindowLongPtr(m_hwndtrialbuy, GWLP_USERDATA, (LONG_PTR)wndproc);
				EnableWindow(m_hwndtrialbuy, TRUE);
			}
		}
	}
#endif
	return wyTrue;
}

LRESULT	CALLBACK
FrameWindow::TrialbuyWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	WNDPROC wndproc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	TRACKMOUSEEVENT te;
//	RECT rc;
	switch (message)
	{
	case WM_MOUSEMOVE:	
		if(!pGlobals->m_pcmainwin->m_mouseoverbuy)
		{
			pGlobals->m_pcmainwin->m_mouseoverbuy = wyTrue;
			InvalidateRect(pGlobals->m_pcmainwin->m_hwndtrialbuy , NULL, TRUE);
		}
		te.cbSize    = sizeof(TRACKMOUSEEVENT);
		te.dwFlags   = TME_LEAVE;
		te.hwndTrack = pGlobals->m_pcmainwin->m_hwndtrialbuy;

		TrackMouseEvent(&te);
		return 0;
	case WM_MOUSELEAVE:
		if(pGlobals->m_pcmainwin->m_mouseoverbuy)
		{
			pGlobals->m_pcmainwin->m_mouseoverbuy = wyFalse;
			InvalidateRect(pGlobals->m_pcmainwin->m_hwndtrialbuy , NULL, FALSE);
		}
		return 0;
	case WM_LBUTTONUP:
		ShellExecute(NULL, L"open", TEXT(BUYURL), NULL, NULL, SW_SHOWNORMAL);
		return 0;



	}
	return CallWindowProc(wndproc, hwnd, message, wparam, lparam);
}

LRESULT	CALLBACK
FrameWindow::ToolbarWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    WNDPROC wndproc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
//    HDC hdc;
//    RECT  rect;// rcwnd,rcclient,
//    HBRUSH hbr;
//    wyBool ret = wyFalse;
	LPDRAWITEMSTRUCT lpdis;
//	RECT  textrect;
	GRADIENT_RECT gRect;
	TRIVERTEX vertex[2];
	wyInt32  fontheight = 0;
	HGDIOBJ prevobj;
	COLORREF prevcolor;
	wyInt32	prevbk;
	
	if(message == WM_CTLCOLORSTATIC)
	{
		if(pGlobals->m_pcmainwin->m_trialtextfont == NULL)
		{
			fontheight = -MulDiv(10, GetDeviceCaps((HDC)wparam, LOGPIXELSY), 72);
			pGlobals->m_pcmainwin->m_trialtextfont = CreateFont(fontheight, 0, 0, 0, FW_NORMAL, 0, 0, 
												        0, 0, 0, 0, 0, 0, L"Arial");
		}
		if((HWND)lparam == pGlobals->m_pcmainwin->m_hwndtrialtext)
		{
			SetBkMode((HDC)wparam, TRANSPARENT);
			SelectObject((HDC)wparam, pGlobals->m_pcmainwin->m_trialtextfont);
			SetTextColor((HDC)wparam, RGB(0, 0, 0));
			return (BOOL)GetStockObject(HOLLOW_BRUSH);
		}
	}
	else
	if(message == WM_CTLCOLORBTN)
	{
		SetBkMode((HDC)wparam, TRANSPARENT);
		return (BOOL)GetStockObject(HOLLOW_BRUSH);
	}
	else
	if(message == WM_DRAWITEM )
	{
		lpdis = (LPDRAWITEMSTRUCT)lparam;
		if(pGlobals->m_pcmainwin->m_trialbuyfont == NULL)
		{
			fontheight = -MulDiv(10, GetDeviceCaps(lpdis->hDC, LOGPIXELSY), 72);
			pGlobals->m_pcmainwin->m_trialbuyfont = CreateFont(fontheight, 0, 0, 0, 1000, 0, 0, 
												        0, 0, 0, 0, 0, 0, L"Arial");
		}
		
		if(lpdis->hwndItem == pGlobals->m_pcmainwin->m_hwndtrialbuy)
		{
			//if(lpdis->itemState != ODS_HOTLIGHT)
			if( pGlobals->m_pcmainwin->m_mouseoverbuy == wyFalse)
			{
				vertex[0].Red   = 0x6f00;
				vertex[0].Green = 0xb500;
				vertex[0].Blue  = 0x3600;
				vertex[0].Alpha = 0x0000;

				vertex[1].Red   = 0x6000;
				vertex[1].Green = 0xa300;
				vertex[1].Blue  = 0x2a00;
				vertex[1].Alpha = 0x0000;
			}
			else
			{
				vertex[0].Red   = 0x5900;
				vertex[0].Green = 0x9d00;
				vertex[0].Blue  = 0x2200;
				vertex[0].Alpha = 0x0000;

				vertex[1].Red   = 0x4300;
				vertex[1].Green = 0x8100;
				vertex[1].Blue  = 0x2a00;
				vertex[1].Alpha = 0x0000;
			}			
			vertex[0].x     = lpdis->rcItem.left;
			vertex[0].y     = lpdis->rcItem.top;

			vertex[1].x     = lpdis->rcItem.right;
			vertex[1].y     = lpdis->rcItem.bottom; 

			gRect.UpperLeft  = 0;
			gRect.LowerRight = 1;
			BeginPath(lpdis->hDC);
			RoundRect(lpdis->hDC, lpdis->rcItem.left-1, lpdis->rcItem.top-1,  lpdis->rcItem.right+1, lpdis->rcItem.bottom+1, 10, 10);
			EndPath(lpdis->hDC);
			SelectClipPath(lpdis->hDC, RGN_COPY);
			//FrameRect(lpdis->hDC, &lpdis->rcItem, (HBRUSH) GetStockObject(BLACK_BRUSH));
			GradientFill(lpdis->hDC, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
			prevbk = SetBkMode(lpdis->hDC, TRANSPARENT);
			prevobj = SelectObject(lpdis->hDC, pGlobals->m_pcmainwin->m_trialbuyfont);
			SetTextColor(lpdis->hDC, RGB(255, 255, 255));
#if IS_FROM_WINDOWS_STORE == 1
			prevcolor = DrawText(lpdis->hDC, _(L"Visit Us"), -1, &lpdis->rcItem, DT_CENTER | DT_SINGLELINE | DT_VCENTER );
#else
			prevcolor = DrawText(lpdis->hDC, _(L"BUY"), -1, &lpdis->rcItem, DT_CENTER | DT_SINGLELINE | DT_VCENTER );
#endif
			SelectObject(lpdis->hDC, prevobj);
			SetBkMode(lpdis->hDC, prevbk);
			SetTextColor(lpdis->hDC, prevcolor);
		}
	}
	//
	//HFONT		hfont = NULL;
    //if(message == WM_NCPAINT)
    //{
    //    if((ret = wyTheme::GetBrush(BRUSH_MAINTOOLBAR, &hbr)) == wyFalse)
    //    {
    //        hbr = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    //    }

    //    hdc = GetWindowDC(hwnd);
    //    GetWindowRect(hwnd, &rcwnd);
    //    GetClientRect(hwnd, &rcclient);
    //    MapWindowRect(hwnd, NULL, &rcclient);
    //    
    //    rect.left = rect.top = 0;
    //    rect.bottom = rcclient.top - rcwnd.top;
    //    rect.right = rcwnd.right - rcwnd.left;

    //    if(rect.right - rect.left && rect.bottom - rect.top)
    //    {
    //        FillRect(hdc, &rect, hbr);
    //    }

    //    rect.top = rcclient.bottom - rcwnd.top;
    //    rect.bottom = rect.top + (rcwnd.bottom - rcclient.bottom);
    //    
    //    if(rect.right - rect.left && rect.bottom - rect.top)
    //    {
    //        FillRect(hdc, &rect, hbr);
    //    }

    //    rect.top = 0;
    //    rect.right = rcclient.left - rcwnd.left;
    //    
    //    if(rect.right - rect.left && rect.bottom - rect.top)
    //    {
    //        FillRect(hdc, &rect, hbr);
    //    }

    //    rect.left = rcclient.right - rcwnd.left;
    //    rect.right = rect.left + (rcwnd.right - rcclient.right);
    //    
    //    if(rect.right - rect.left && rect.bottom - rect.top)
    //    {
    //        FillRect(hdc, &rect, hbr);
    //    }

    //    if(ret == wyFalse)
    //    {
    //        DeleteBrush(hbr);
    //    }

    //    ReleaseDC(hwnd, hdc);
    //    return 0;
    //}

    return CallWindowProc(wndproc, hwnd, message, wparam, lparam);
}

// Function initializes array of buttons and adds them to the toolbar whose handle is sent as
// parameter.
wyBool
FrameWindow::CreateToolButtons(HWND hwndtool)
{
	wyInt32     size, iconid;
	HICON		hicon;
	
	/*
		We are adding m_icons from the 2 element in the toolbar as we dont show
		stop icon everytime. When the user has pressed the exeuction icon we replace
		it with stop icon so we always put that icon in the first list of the image list */

	wyInt32 command[] =
		{	
			IDM_FILE_NEWCONNECTION, ID_NEW_EDITOR , IDM_EXECUTE, ACCEL_EXECUTEALL, 
			ACCEL_QUERYUPDATE, IDM_REFRESHOBJECT
		};
	wyUInt32 states[][2] = 
	{ 
		{TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON} ,
        {TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
        {TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
   	};
	wyInt32 imgres[] =  
		{	IDI_CONNECT, IDI_QUERY, IDI_EXECUTE, IDI_EXECUTEALL, IDI_EXECUTEFORUPD, 
			IDI_REFRESH
		};
		
	VERIFY(m_hsecondiml	= ImageList_Create(m_toolbariconsize, m_toolbariconsize, ILC_COLOR32 | ILC_MASK, 1, 0));
	SendMessage(hwndtool, TB_SETIMAGELIST, 0,(LPARAM)m_hsecondiml);
	SendMessage(hwndtool, TB_SETEXTENDEDSTYLE, 0,(LPARAM)TBSTYLE_EX_DRAWDDARROWS);

	size = sizeof(command)/ sizeof(command[0]);
	
	hicon = (HICON)LoadImage(m_hinstance, MAKEINTRESOURCE(IDI_STOP), IMAGE_ICON, m_toolbariconsize, m_toolbariconsize, LR_DEFAULTCOLOR);
	VERIFY((iconid = ImageList_AddIcon(m_hsecondiml, hicon))!= -1);
	VERIFY(DestroyIcon(hicon));

	// set some required values
	SendMessage(hwndtool, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    AddToolBarButtons(m_hinstance, hwndtool, m_hsecondiml, command, size, states, imgres);

    SendMessage(hwndtool, TB_AUTOSIZE, 0, 0);
	ShowWindow(hwndtool, TRUE);

	return wyTrue;
}


// Create the combo box which will be there in the toolbar window.
HWND
FrameWindow::CreateToolCombo(HWND hwndtool)
{
	HWND        hwndcombo;
	wyUInt32    style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_VSCROLL | CBS_DROPDOWN | CBS_DISABLENOSCROLL ;
		
	VERIFY(hwndcombo = CreateWindowEx(0, WC_COMBOBOXEX, TEXT(""), style, 516, 0, 150, 425,
												m_hwndtool,(HMENU)IDC_TOOLCOMBO,(HINSTANCE)GetHinstance(), NULL));
	return hwndcombo;
}

HWND
FrameWindow::CreateTrialText(HWND hwndtool)
{
	HWND        hwndtrialtext;
	wyUInt32    style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_CENTER;
		
	VERIFY(hwndtrialtext = CreateWindowEx(0, WC_STATIC, TEXT(""), style, 700, 8, 120, 32,
												m_hwndsecondtool,(HMENU)IDC_TRIALTEXT,(HINSTANCE)GetHinstance(), NULL));
	return hwndtrialtext;
}


HWND
FrameWindow::CreateTrialBuy(HWND hwndtool)
{
	HWND        hwndtrialbuy;
	RECT rcmain;
	GetClientRect((HWND)GetHwnd(), &rcmain);
	wyUInt32    style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_TEXT | BS_CENTER | BS_OWNERDRAW  ;

#if IS_FROM_WINDOWS_STORE == 1
	VERIFY(hwndtrialbuy = CreateWindowEx(0, WC_BUTTON, TEXT("Visit Us"), style, rcmain.right-84, 2, 64, 26,
												m_hwndsecondtool,(HMENU)IDC_TRIALBUY,(HINSTANCE)GetHinstance(), NULL));
#else
	VERIFY(hwndtrialbuy = CreateWindowEx(0, WC_BUTTON, TEXT("BUY"), style, rcmain.right-84, 2, 64, 26,
												m_hwndsecondtool,(HMENU)IDC_TRIALBUY,(HINSTANCE)GetHinstance(), NULL));
#endif
	return hwndtrialbuy;
}



// Function to set the font of the combo box in the toolbar to ARIAL
wyBool
FrameWindow::SetToolComboFont()
{
	SendMessage(m_hwndtoolcombo, WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
	return wyTrue;
}

// Function to create MDI client window.
wyBool
FrameWindow::CreateMDIWindow()
{
	wyUInt32                style = WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | WS_CLIPCHILDREN;
	CLIENTCREATESTRUCT      cs;

	// Initialize the cs structure.
	cs.hWindowMenu  = GetSubMenu((HMENU)GetMenu(GetHwnd()), MDISUBMENU );
	cs.idFirstChild = MDISTARTID;

	VERIFY(pGlobals->m_hwndclient	= CreateWindow(	TEXT("MDICLIENT"), NULL,
			style,  0, 0, 0, 0, m_hwndmain,(HMENU)1,(HINSTANCE)GetHinstance(),
			(PSTR)&cs));

	SetMDIHwnd(pGlobals->m_hwndclient);

	//subclass the window procedure to handle Ctrl+Tab shortcut
	pGlobals->m_wmnextproc = (WNDPROC)SetWindowLongPtr(pGlobals->m_hwndclient, GWLP_WNDPROC, (LONG_PTR)FrameWindow::FrameWndProc);

	return wyTrue;
}

//handle Ctrl+tab shortcut for WM_MDINEXT message 
LRESULT	CALLBACK 
FrameWindow::FrameWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	wyInt32  tabid, count;
	wyInt32 tabcount = 0;
    HBRUSH                  hbr;
    RECT    rect;

	/*if message is WM_MDINEXT, then on Ctrl+Tab select the tab at next right position.
	if it is the last tab then first tab will be selected*/

	if(message == WM_MDINEXT)
	{
		//if it is default message
		if(pGlobals->m_iscustomwmnext  == wyFalse)
		{
			count = CustomTab_GetItemCount(pGlobals->m_pcmainwin->m_hwndconntab);
			tabid = CustomTab_GetCurSel(pGlobals->m_pcmainwin->m_hwndconntab);
			
			if(count > 1)
			{
				//left to right on Ctrl+tab
				if(lparam == 0)
				{
					tabcount = (tabid + 1) % count;
				}
				//right to left on Ctrl+shift+tab
				else if(lparam == 1)
				{
					tabcount = (tabid - 1 >= 0)? (tabid - 1) : (count - 1);
				}
				CustomTab_SetCurSel(pGlobals->m_pcmainwin->m_hwndconntab, tabcount, 1);
				CustomTab_EnsureVisible(pGlobals->m_pcmainwin->m_hwndconntab, tabcount);	
			}
			return 1;
		}
	}
    else if(message == WM_ERASEBKGND && wyTheme::GetBrush(BRUSH_MDICLIENT, &hbr))
    {
        GetClientRect(hwnd, &rect);
        FillRect((HDC)wparam, &rect, hbr);
        return 1;
    }
	else if(message == UM_MDIGETACTIVE )
	{
		//This message is posted by sessionsave thread
		LPTHREAD_MSG_PARAM lptmp = (LPTHREAD_MSG_PARAM)lparam;
		//if there is a valid event handle, then signal the event. if the message was sent using SendMessage, then event handle will be NULL
		if(lptmp && lptmp->m_hevent)
		{
			lptmp->m_lparam = (LPARAM)GetActiveWin();
			SetEvent(lptmp->m_hevent);
		}
		return wyTrue;
	}

	//call default frame window procedure
	return CallWindowProc(pGlobals->m_wmnextproc, hwnd, message, wparam, lparam);	
}

// Function to create the status bar of the main window.
wyBool
FrameWindow::CreateStatusBarWindow()
{
	RECT     rc;
	HWND     hwndstatus;
	wyUInt32 exstyles = NULL;
	wyUInt32 styles = WS_CHILD | WS_VISIBLE;

	VERIFY(GetClientRect(GetHwnd(), &rc));

	VERIFY(hwndstatus = CreateWindowEx(exstyles, STATUSCLASSNAME, L"", styles, 0,0,0,0, GetHwnd(),
				(HMENU)IDC_FRAMESTATUS,(HINSTANCE)GetHinstance(), NULL));

    m_connection->ManageStatusBar(hwndstatus);

	VERIFY(UpdateWindow(hwndstatus));

    SetStatusHwnd(hwndstatus);
    
	m_connection->SetStatusParts(hwndstatus);
    m_statusbarmgmt = new StatusBarMgmt(hwndstatus);
	return wyTrue;
}


// Functions to resize various windows.
wyBool
FrameWindow::Resize(WPARAM wparam)
{
	ResizeToolBar();
	m_connection->ResizeStatusWindow(GetHwnd(), GetStatusHwnd());
	
	ResizeMDIWindow();

	//To paint properely while maximizing frame window
	if(GetHwnd() && wparam == SIZE_MAXIMIZED)
		InvalidateRect(NULL, NULL, TRUE);			

	return wyTrue;
}

void
FrameWindow::ResizeToolBar()
{
	wyInt32	vpos, hpos, height, width, comboht = TOOLBAR_HEIGHT, tabheight = 0,sectoolx;
	wyInt32 firsttoolextrawidth = 0, combovpos = 0, firsttoolbarheight = 0;
	RECT	rc, rctool, rcttemp;
	VERIFY(GetClientRect((HWND)GetHwnd(), &rc));
	if(m_toolbariconsize == ICON_SIZE_28)
	{
		/*firsttoolextrawidth = m_toolbariconsize + m_toolbariconsize / 2-14;
		combovpos = 12;	*/	
		firsttoolextrawidth = (m_toolbariconsize * 2) - 12;
		combovpos = 7;
	}

	else if(m_toolbariconsize == ICON_SIZE_32)
	{
		/*firsttoolextrawidth = (m_toolbariconsize * 2) - 2;
		combovpos = 4;*/
		firsttoolextrawidth = m_toolbariconsize + m_toolbariconsize / 2-3;
		combovpos = 10;
	}

	else
	{
		/*firsttoolextrawidth = (m_toolbariconsize * 3) - 2;
		combovpos = 0;	*/	
		firsttoolextrawidth = (m_toolbariconsize * 2) - 2;
		combovpos = 4;
	}

	firsttoolbarheight = m_toolbariconsize +  8;

	// Set the values for toolbar.
	vpos	= 1; 
	hpos	= 0; 
	height = firsttoolbarheight;
	
	width = TOOLBAR_WIDTH;
	
	width = (FIRSTTOOLICONCOUNT * m_toolbariconsize) + TOOLBAR_COMBO_WIDTH + firsttoolextrawidth;

	// Move both the toolbars.
	VERIFY(MoveWindow(m_hwndtool, hpos, vpos, width, height, TRUE));

	//Tab interface
    if(m_hwndconntab)
    {
        tabheight = CustomTab_GetTabHeight(m_hwndconntab);
        VERIFY(MoveWindow(m_hwndconntab, hpos, m_toolbariconsize + TABBED_INTERFACE_TOP_PADDING, rc.right, tabheight, TRUE));
    }

	VERIFY(GetClientRect(m_hwndtool, &rcttemp)); 
	
	hpos = rcttemp.right + 10;	
	comboht = m_toolbariconsize;    

	// Set the values for the combobox.
	hpos	= width - TOOLBAR_COMBO_WIDTH;	
	width = TOOLBAR_COMBO_WIDTH;

	VERIFY(MoveWindow((HWND)GetToolCombo(), hpos, combovpos, width, comboht , TRUE));

	VERIFY(GetWindowRect(GetToolHwnd(), &rctool)); 
	VERIFY(MapWindowPoints(NULL, GetHwnd(),(LPPOINT)&rctool, 2));

	VERIFY(MoveWindow((HWND)m_hwndsecondtool, hpos + TOOLBAR_COMBO_WIDTH, vpos, 
                                (rc.right-rc.left) - (hpos + 150), height, TRUE));
	if(pGlobals->m_pcmainwin->m_connection->m_enttype == ENT_TRIAL)
	{
		sectoolx = (rc.right-rc.left) - (hpos + 150);
		if(sectoolx - 74 > (TOOLBAR_WIDTH + 80 + 120) )
		VERIFY(MoveWindow((HWND)m_hwndtrialbuy, sectoolx - 74, 2, 
									64, 26, TRUE));
		//700, 5, 120, 32
		if(sectoolx - 194 > (TOOLBAR_WIDTH + 80) )
		VERIFY(MoveWindow((HWND)m_hwndtrialtext, sectoolx - 194, 5, 
									120, 32, TRUE));
		ShowWindow(m_hwndtrialtext, TRUE);
		ShowWindow(m_hwndtrialbuy, TRUE);
	}
	ShowWindow(GetToolCombo(), TRUE);
	ShowWindow(m_hwndsecondtool, TRUE);
	

	return;
}

void
FrameWindow::ResizeMDIWindow()
{
	wyInt32 ret, hpos, vpos, width, height;
	RECT	rcframe, rctool, rcstatus;

	VERIFY(GetClientRect(GetHwnd(), &rcframe));
	VERIFY(GetWindowRect(GetToolHwnd(), &rctool));
	VERIFY(GetWindowRect(GetStatusHwnd(), &rcstatus));

	hpos	= 0;
    vpos	= m_toolbariconsize + TABBED_INTERFACE_TOP_PADDING + (m_hwndconntab ? CustomTab_GetTabHeight(m_hwndconntab) : TABBED_INTERFACE_HEIGHT);
	height	= (rcframe.bottom - (rcstatus.bottom - rcstatus.top))- vpos;
	width	= rcframe.right - rcframe.left;

	VERIFY(ret = MoveWindow(GetMDIWindow(), 0, vpos, width, height, FALSE));

	return;
}

wyInt32
FrameWindow::ShowMySQLErrorCancel(HWND hdlg, Tunnel * tunnel, PMYSQL mysql)
{
	wyUInt32    mysqlerrno;
	wyString    err;

	mysqlerrno = tunnel->mysql_errno(*mysql);

	/* it may happen that due to http error the errornumber is 0 so we return */
	if(0 == mysqlerrno)
		return IDYES;

	if(tunnel->IsTunnel()&& mysqlerrno > 12000)
		err.Sprintf(_("HTTP Error No. %d\n%s"), mysqlerrno, tunnel->mysql_error(*mysql));
	else
		err.Sprintf(_("Error No. %d\n%s"), mysqlerrno, tunnel->mysql_error(*mysql));

	return MessageBox(hdlg, err.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OKCANCEL | MB_DEFBUTTON2);
}


// Callback procedure to close every Query Window.
wyInt32 CALLBACK
FrameWindow::CloseEnumProc(HWND hwnd, LPARAM lParam)
{
	wyInt32	*ret = (wyInt32*)lParam;

	if(GetWindow(hwnd, GW_OWNER))        
		return wyTrue ;
 
	SendMessage(GetParent(hwnd), WM_MDIRESTORE, (WPARAM)pGlobals->m_hwndclient, 0);
 
	if(!(SendMessage(hwnd, WM_QUERYENDSESSION, 0, 0)))
	{
		*(ret)= 0;
		return wyFalse;
	}
 
	SendMessage(GetParent(hwnd), WM_MDIDESTROY, (WPARAM)hwnd, 0);

	return wyTrue ;
}

// Function to add text in the combobox of the toolbar. This function is called whenever
// the current database is changed so that the current database is shown in the combobox. When
// called for the first time or if NULL is sent as parameter then it writes no database selected.
wyBool
FrameWindow::AddTextInCombo(const wyWChar * text)
{
	wyUInt32        ret;
	const wyWChar*   def = NODBSELECTED;
	COMBOBOXEXITEM	cbi;
	wyString		seldb, title;

	MDIWindow *wnd = GetActiveWin();

	SendMessage(m_hwndtoolcombo, CB_RESETCONTENT, 0, 0);

	cbi.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
	cbi.pszText     = (wyWChar*)(wcslen(text)!= 0 ? text : def);
	cbi.cchTextMax  = 50;
	cbi.iImage	    = 0;
	cbi.iSelectedImage = 0;
	cbi.iItem       = -1;

	SendMessage(m_hwndtoolcombo, CBEM_INSERTITEM, 0L,(LPARAM)&cbi);
	VERIFY(ret = SendMessage(m_hwndtoolcombo, CB_SETCURSEL,(WPARAM)0, 0)!= CB_ERR);

	//Handle the title with DB name
	if(!wnd || !text || !wcslen(text))
		return wyFalse;
	
	seldb.SetAs(text);
	if(pGlobals->m_pcmainwin->m_sessionname.GetLength())
		title.Sprintf("%s-%s/%s %s %s",pGlobals->m_pcmainwin->m_sessionname.GetString(), wnd->m_title.GetString(), seldb.GetString(), wnd->m_tunneltitle.GetString(), wnd->m_conninfo.m_isreadonly == wyTrue?"- Read-Only":"" );
	else
		title.Sprintf("%s/%s %s %s", wnd->m_title.GetString(), seldb.GetString(), wnd->m_tunneltitle.GetString(), wnd->m_conninfo.m_isreadonly == wyTrue?"- Read-Only":"" );

	SetWindowText(wnd->m_hwnd, title.GetAsWideChar());
	
	return wyTrue;
}

// Displays text which is passed as paramter in the first part of the status bar in the main
// window.
wyInt32
FrameWindow::AddTextInStatusBar(const wyWChar *text)
{
    wyString temp(text);

    temp.LTrim();
    temp.Insert(0, " ");

    m_statusbarmgmt->ShowInformation(temp.GetAsWideChar(), 0);
    return 0;
}

// Function sets the connection number in the other part of the status bar of the main window
// It takes the current number of connection which is stored globally.
wyBool
FrameWindow::SetConnectionNumber()
{
	wyString    con;

	con.Sprintf(_(" Connections: %d"), pGlobals->m_conncount);
    m_statusbarmgmt->ShowInformation(con.GetAsWideChar(), 5);

	return wyTrue;
}

// function handle all the onwmcommand values in the main window.
// somewhere classname is checked so that different command is not executed for different MDI
// window otherwise it will give an GPF error.
wyInt32
FrameWindow::OnWmCommand(WPARAM wParam)
{
	HWND			hwndactive = (HWND)SendMessage(pGlobals->m_hwndclient, WM_MDIGETACTIVE, 0, NULL);
	wyBool			ret = wyFalse;
	MDIWindow *		pcquerywnd = NULL;
	EditorBase		*peditorbase = NULL;
	TabMgmt			*ptabmgmt	  = NULL;
    TabEditor       *ptabeditor = NULL;
    TabTypes        *ptabtype   = NULL;
	TabHistory		*ptabhistory = NULL;
	TabObject		*ptabobject = NULL;
    TabTableData    *ptabledata = NULL;
	HWND            hwnd;
	ConnectionTab	*conntab  = NULL;
	wyInt32			tabicon = 0;
    MySQLDataEx*    mydata = NULL;
	RenameTabDlg	conncolor;
	wyInt32			ptabimage;
	CTCITEM quetabitem = {0};
	TabEditor*		tabquery;
	HMENU			hmenu;
#ifndef COMMUNITY
	TabDbSearch		*ptabdbsearch = NULL;
	Transactions	*ptransaction = NULL;
#endif

	if(hwndactive)
	{
		pcquerywnd	 = (MDIWindow*)GetWindowLongPtr(hwndactive, GWLP_USERDATA);

		tabicon = pcquerywnd->m_pctabmodule->GetActiveTabImage();

		if(tabicon != IDI_DATASEARCH)
		{
            ptabeditor   = pcquerywnd->GetActiveTabEditor();
			ptabtype     = pcquerywnd->GetTabModule()->GetActiveTabType();
		    if(ptabeditor)
		    {
		        peditorbase = pcquerywnd->GetActiveTabEditor()->m_peditorbase;
		        ptabmgmt	 = pcquerywnd->GetActiveTabEditor()->m_pctabmgmt;
	        }
			ptabhistory = pcquerywnd->GetActiveHistoryTab();
			ptabobject = pcquerywnd->GetActiveInfoTab();

            if(tabicon == IDI_TABLE)
            {
                ptabledata = (TabTableData*)ptabtype;
            }
		}
#ifndef COMMUNITY
		else
		{
			ptabdbsearch = (TabDbSearch*) pcquerywnd->m_pctabmodule->GetActiveTabType();
		}
		ptransaction = (Transactions*) GetActiveWin()->m_ptransaction;
		
	//	if(pcquerywnd && pGlobals->m_entlicense.CompareI("Professional") == 0)
	//		pcquerywnd->m_conninfo.m_isreadonly = wyFalse;
#endif

		//connection tab object
		conntab = pGlobals->m_pcmainwin->m_conntab;
	}

	/* now we check that if a query is being executed for the selected window then we dont do anything */
	if(pcquerywnd && (pcquerywnd->m_executing == wyTrue || pcquerywnd->m_pingexecuting == wyTrue))
    {
		wyInt32 id =  LOWORD(wParam);

		if(id != IDM_FILE_EXIT &&
			 id != IDM_FILE_NEWCONNECTION && 
			 id != IDM_FILE_NEWSAMECONN &&
			 id != ACCEL_NEWCONN &&     // if we r executing a query ctrl+m and ctrl+n is not working . issue - http://code.google.com/p/sqlyog/issues/detail?id=270
			 id != ACCEL_NEWSAMECONN &&
			 id != IDM_EXECUTE &&
			 id != ACCEL_EXECUTEALL &&
             id != IDM_WINDOW_CASCADE &&
             id != IDM_WINDOWS_ICONARRANGE &&
             id != IDM_WINDOW_TILE &&
			 id != ACCEL_FIRSTCONN &&
			 id != ACCEL_SECONDCONN &&
			 id != ACCEL_THIRDCONN &&
			 id != ACCEL_FOURTHCONN &&
			 id != ACCEL_FIFTHCONN &&
			 id != ACCEL_SIXTHCONN &&
			 id != ACCEL_SEVENTHCONN &&
			 id != ACCEL_EIGTHCONN &&
			 id != ACCEL_NINTHCONN)
			 return 1;
	}

    if(m_connection->OnWmCommand(hwndactive, pcquerywnd, wParam) == wyTrue)
        return 1;

	switch(LOWORD(wParam))
	{
    case ID_EXPORT_EXPORTTABLEDATA: 
        pcquerywnd->EnableExportDialog();
        break;
    case ACCEL_OBFILTER:
        SetFocus(pcquerywnd->m_pcqueryobject->m_hwndFilter);
        SendMessage(pcquerywnd->m_pcqueryobject->m_hwndFilter, EM_SETSEL, 0, -1);
        break;
    case ACCEL_ALTERTABLE:
        pcquerywnd->m_pcqueryobject->ProcessF6();
        break;
	case ACCEL_CREATETABLE:
		pcquerywnd->m_pcqueryobject->ProcessF4();
		break;
	case ACCEL_NEWCONN:
	case IDM_FILE_NEWCONNECTION:
		CreateConnDialog();
		break;

	case ACCEL_NEWSAMECONN:
	case IDM_FILE_NEWSAMECONN:
        if(pcquerywnd && OnNewSameConnection(hwndactive, pcquerywnd) == wyFalse)
            return 0;
		break;

	case ACCEL_DISCONN:
	case ACCEL_CLOSECONNECTION:
	case IDM_FILE_CLOSECONNECTION:
		if(hwndactive)
			SendMessage((HWND)SendMessage(pGlobals->m_hwndclient, WM_MDIGETACTIVE, 0, NULL), WM_CLOSE, 0, 0);
		break;

	case IDM_TMAKER_CLOSE:
		SendMessage((HWND)SendMessage(pGlobals->m_hwndclient, WM_MDIGETACTIVE, 0, NULL), WM_CLOSE, 0, 0);
		break;
		
	case IDM_FILE_CLOASEALL:
		{
			wyInt32 ret = 0;
			SendMessage(m_hwndmain, WM_CLOSEALLWINDOW, 0,(LPARAM)&ret);
		}
		break;

	case IDM_FILE_OPENSQL:
        if(hwndactive )
			pcquerywnd->HandleFileOpen(SQLYOGFILEINDEX);			        
		break;

	//To open file in new tab
	case ACCEL_OPENNEW:
	case IDM_FILE_OPENSQLNEW:
		if(hwndactive)
		{
//For Commubnity & PRO, there's no SD & QB
#ifndef COMMUNITY
			if(pGlobals->m_entlicense.CompareI("Professional"))
			{
				pcquerywnd->HandleFileOpen(SQLYOGFILEINDEX);
			}
			else
			{
				pcquerywnd->HandleFileOpen(SQLINDEX);
			}
#else
			pcquerywnd->HandleFileOpen(SQLINDEX);
#endif
		}
		break;
	
	case ACCEL_SAVE:
	case IDM_FILE_SAVESQL:
        if(hwndactive)
			pcquerywnd->HandleFileSave();      
		break;

	case IDM_FILE_SAVEAS:
		if(hwndactive )
			pcquerywnd->HandleSaveAsFile();
		break;

	case ID_LF_1+1:   // Sqlyog Recent files
	case ID_LF_1+2:
	case ID_LF_1+3:
	case ID_LF_1+4:
	case ID_LF_1+5:
	case ID_LF_1+6:
	case ID_LF_1+7:
	case ID_LF_1+8:
	case ID_LF_1+9:
	case ID_LF_1+10:

			if(hwndactive )
			pGlobals->m_pcmainwin->InsertFromLatestFile(LOWORD(wParam), pcquerywnd);
		break;

	case IDM_FILE_EXIT:
        
        PostMessage(m_hwndmain, WM_CLOSE, 0, 0);
        return 1;
        break;

	case IDM_OBCOLOR:
		SendMessage(pcquerywnd->m_pcqueryobject->m_hwnd, WM_COMMAND, wParam, NULL);
		break;
		
	case IDC_TOOLCOMBO:
        HandleToolCombo(wParam);
		break;
#ifndef COMMUNITY

	case ID_TRANSACTION_SETAUTOCOMMIT:
			VERIFY(hmenu = GetMenu(m_hwndmain));
			ptransaction->EnabledisableAutoCommit(hmenu, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql);
		break;

	case ID_TRX_REPEATABLEREAD:
	case ID_TRX_READCOMMITED:             
	case ID_TRX_READUNCOMMITED:           
	case ID_TRX_SERIALIZABLE:
			VERIFY(hmenu = GetMenu(m_hwndmain));
			ptransaction->HandletrxIsolationmode(LOWORD(wParam), hmenu, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql); 

		break;

	case ID_STARTTRANSACTION_WITHNOMODIFIER:
	case ID_WITHCONSISTENTSNAPSHOT_READONLY:
	case ID_WITHCONSISTENTSNAPSHOT_READWRITE:
	case ID_STARTTRANSACTION_READONLY:
	case ID_STARTTRANSACTION_READWRITE:
		if(pcquerywnd->m_tunnel->IsTunnel() == false)
		ptransaction->HandleStartTransaction(LOWORD(wParam), pcquerywnd->m_tunnel, &pcquerywnd->m_mysql); 
		else
			ptransaction->OnTunnelMessage(GetActiveWindow());
		break;

	case ID_COMMIT_WITHNOMODIFIER:
	case ID_COMMIT_ANDCHAIN:
	case ID_COMMIT_ANDNOCHAIN:
	case ID_COMMIT_RELEASE:
	case ID_COMMIT_NORELEASE:
		VERIFY(hmenu = GetMenu(m_hwndmain));
		ptransaction->HandleCommit(LOWORD(wParam), hmenu, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql); 
		break;

	case ID_ROLLBACK_TRANSACTION:
	case ID_ROLLBACK_ANDCHAIN:
	case ID_ROLLBACK_ANDNOCHAIN:
	case ID_ROLLBACK_RELEASE:
	case ID_ROLLBACK_NORELEASE:
		VERIFY(hmenu = GetMenu(m_hwndmain));
		ptransaction->HandleRollback(LOWORD(wParam), hmenu, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql); 
		break;

	case ID_SAVEPOINT_CREATESAVEPOINT:

		if(hwndactive)
		{
			ptransaction->HandleCreateSavepoint(hwndactive, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql);
		}
			break;
		
	case ID_SAVEPOINT_RELEASESAVEPOINT:
		ptransaction->HandleReleaseSavepoint(hwndactive, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql);
		break;
	case ID_ROLLBACK_TOSAVEPOINT:
		ptransaction->HandleRollbackTo(hwndactive, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql);
		break;

#endif
	case ACCEL_QUERYUPDATE:
	case ACCEL_QUERYUPDATE_KEY:
#ifndef COMMUNITY
	if(pcquerywnd->m_conninfo.m_isreadonly == wyTrue)
	{
		break;
	}
#endif
	case IDM_EXECUTE:
	case ACCEL_EXECUTE_MENU:
        if(!ptabeditor)
            break;
        HandleExecuteCurrentQuery(hwndactive, pcquerywnd, peditorbase, wParam);
		break;
	case ACCEL_EXECUTE:
		if(pcquerywnd)
			HandleOnRefreshExecuteQuery(hwndactive, pcquerywnd, wParam);
		break;

	case ACCEL_EXECUTESEL_CTRL_F9:
		if(!(pGlobals->m_isrefreshkeychange == wyFalse && ptabeditor))    
				break;
		HandleExecuteAllQuery(hwndactive, pcquerywnd, peditorbase);
		break;

	case ACCEL_EXECUTESEL_CTRL_F5:
		if(pGlobals->m_isrefreshkeychange == wyFalse)
            break;        
	case ACCEL_EXECUTEALL:
        if(!ptabeditor)
            break;
        HandleExecuteAllQuery(hwndactive, pcquerywnd, peditorbase);
		break;
    case ID_EXPLAIN_EXPLAIN:
        if(!ptabeditor)
            break;
        HandleExecuteExplain(hwndactive, pcquerywnd, peditorbase);
        break;
    case ID_EXPLAIN_EXTENDED:
        if(!ptabeditor)
            break;
        HandleExecuteExplain(hwndactive, pcquerywnd, peditorbase, wyTrue);
        break;

	//case ACCEL_EXECUTESEL_CTRL_F9:
	//	if(!(pGlobals->m_isrefreshkeychange == wyFalse&& ptabeditor))    
	//			break;
	//	if(IsQuerySelected(peditorbase)) // if Query is selected in query tab 
	//		HandleExecuteSelQuery(hwndactive, pcquerywnd, peditorbase);
	//	break;

	//case ACCEL_EXECUTESEL_CTRL_F5:
	//	if((pGlobals->m_isrefreshkeychange == wyFalse) ||(ptabeditor) &&(!IsQuerySelected(peditorbase)))
	//		break;
	//case ACCEL_EXECUTESEL:
 //       if(!ptabeditor)
 //           break;
 //       HandleExecuteSelQuery(hwndactive, pcquerywnd, peditorbase);
	//	break;

	case ACCEL_NEWEDITOR:
	case ID_NEW_EDITOR:
        if(hwndactive)
            CreateNewQueryEditor(pcquerywnd);
		break;
	
	case ID_EDIT_SWITCHTABSTORIGHT:
	case ACCEL_NAVIGATETABDOWN:
		if(hwndactive )
			HandleTabNavigation(pcquerywnd, wyTrue);
		break;

	case ID_EDIT_SWITCHTABSTOLEFT:
	case ACCEL_NAVIGATETABUP:
		if(hwndactive )
			HandleTabNavigation(pcquerywnd, wyFalse);
		break;
	case ACCEL_RENAMEQUERYTAB:
		ptabimage = pcquerywnd->m_pctabmodule->GetActiveTabImage();
		if( ptabimage == IDI_QUERY_16 )
		{
				quetabitem.m_mask = quetabitem.m_mask |CTBIF_LPARAM|CTBIF_IMAGE; 
				CustomTab_GetItem(pcquerywnd->m_pctabmodule->m_hwnd, CustomTab_GetCurSel(pcquerywnd->m_pctabmodule->m_hwnd), &quetabitem);
				tabquery = (TabEditor*)quetabitem.m_lparam;
				if( tabquery->m_peditorbase->m_filename.GetLength() == 0)
					conncolor.ShowRenameTabDlg(pcquerywnd->m_hwnd);
		}
		
		break;
	case ID_FILE_RENAMETAB:
		//pcquerywnd->m_pctabmodule->SetTabRename(L"abc1abc2abc3abc4abc5abc6", wyFalse);
		conncolor.ShowRenameTabDlg(pcquerywnd->m_hwnd);
		break;
	case ID_SAVESESSIONAS:
		if(SaveSessionFile(pGlobals->m_hwndclient, wyTrue))
		{
			ResetEvent(m_sessionchangeevent);
		}
		break;
	case ACCEL_SAVESESSION:
	case ID_SAVESESSION:
		if(WaitForSingleObject(m_sessionchangeevent, 0) == WAIT_OBJECT_0 )
		{
			if(SaveSessionFile(pGlobals->m_hwndclient, wyFalse))
			{
				ResetEvent(m_sessionchangeevent);
			}
			break;
		}
		break;
	case ACCEL_OPENSESSION:
	case ID_OPENSESSION:	
		{
#ifndef COMMUNITY
			if(pGlobals->m_entlicense.CompareI("Professional") != 0)
			{
				MDIWindow *wnd = GetActiveWin();
				wyInt32 presult = 6;
				if(wnd && wnd->m_ptransaction && wnd->m_ptransaction->m_starttransactionenabled == wyFalse && pGlobals->m_pcmainwin->m_topromptonclose == wyTrue)
				{
					presult = MessageBox(wnd->m_hwnd , _(L"One or more session(s) have (a) transaction(s) running. Do you still want to continue? The transaction will be rolled back by the server."), 
					_(L"Warning"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2); 
					if(presult != 6)
					{
						break;
					}
					pGlobals->m_pcmainwin->m_closealltrans = IN_TRANSACTION;
					pGlobals->m_pcmainwin->m_intransaction = 0;
				}
			}
#endif
			//Dialog to save current session
		wyInt32 msgreturn;
		if(hwndactive)
			if(WaitForSingleObject(m_sessionchangeevent, 0) == WAIT_OBJECT_0)
			{
				msgreturn = MessageBox(pGlobals->m_hwndclient, _(TEXT("Do you want to save the current session?")), pGlobals->m_appname.GetAsWideChar(), MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON1);
				if(msgreturn == IDYES)
				{
					if(SaveSessionFile(pGlobals->m_hwndclient, wyFalse))
						ResetEvent(m_sessionchangeevent);
				}
				else
				if(msgreturn == IDCANCEL)
					break;
			}
		OpenSessionFile();
#ifndef COMMUNITY
pGlobals->m_pcmainwin->m_closealltrans = 1;
#endif
		}
		break;
	case ACCEL_ENDSESSION:
	case ID_CLOSESESSION:
		{
		#ifndef COMMUNITY
			if(pGlobals->m_entlicense.CompareI("Professional") != 0)
			{
				MDIWindow *wnd = GetActiveWin();
				wyInt32 presult = 6;
				if(wnd && wnd->m_ptransaction && pGlobals->m_pcmainwin->m_intransaction != 0 && wnd->m_ptransaction->m_starttransactionenabled == wyFalse && pGlobals->m_pcmainwin->m_topromptonclose == wyTrue)
				{
					presult = MessageBox(wnd->m_hwnd , _(L"One or more session(s) have (a) transaction(s) running. Do you still want to continue? The transaction will be rolled back by the server."), 
					_(L"Warning"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2); 
					if(presult != 6)
					{
						break;
					}
					pGlobals->m_pcmainwin->m_closealltrans = IN_TRANSACTION;
					pGlobals->m_pcmainwin->m_intransaction = 0;
				}
			}
#endif
		m_sessionfile.Clear();
		m_sessionname.Clear();
		VERIFY(hmenu = GetMenu(m_hwndmain));
		EnableMenuItem(hmenu, ID_CLOSESESSION, MF_GRAYED | MF_BYCOMMAND);
		if(hwndactive)
			pcquerywnd->SetQueryWindowTitle();

#ifndef COMMUNITY
pGlobals->m_pcmainwin->m_closealltrans = 1;
#endif
		}
		break;
	case ID_FILE_CLOSETAB:
	case ACCEL_CLOSETAB:
		if(hwndactive )
			DeleteQueryTabItem(pcquerywnd);
			break;
	case IDM_DB_REFRESHOBJECT:
	case IDM_REFRESHOBJECT:
	    if(hwndactive)
            HandleOnRefresh(pcquerywnd);
		break;

	case IDM_COLLAPSEOBJECT:
		if (hwndactive)
			HandleOnCollapse(pcquerywnd);
		break;

	case ACCEL_REFRESH:
        if(hwndactive)
			HandleOnRefreshExecuteQuery(hwndactive, pcquerywnd, wParam, wyTrue);
		break;

	case IDM_EDIT_UNDO:
		if(hwndactive)
			SendMessage(peditorbase->m_hwnd, SCI_UNDO, 0, 0); 		
		break;

	case IDM_EDIT_REDO:
		if(hwndactive)
			SendMessage(peditorbase->m_hwnd, SCI_REDO, 0, 0);  
		break;


	case IDM_EDIT_CUT:
		if(hwndactive)
		{
			hwnd = GetFocus();
			CopyStyledTextToClipBoard(hwnd);
		    SendMessage(hwnd, SCI_REPLACESEL, 0, (LPARAM)""); 
		}
		break;		

	case IDM_EDIT_COPY:
		if(hwndactive)
			CopyStyledTextToClipBoard(GetFocus());
		break;		

    case IDM_COPYNORMALIZED:
        if(hwndactive && peditorbase)
            CopyWithoutWhiteSpaces(peditorbase->m_hwnd);
        break;

	case ACCEL_PASTE:
	case IDM_EDIT_PASTE:
        if(hwndactive)
			peditorbase->PasteData();
		break;	
		
	/*case IDM_EDIT_CLEAR:
		if(hwndactive)
			SendMessage(peditorbase->m_hwnd, SCI_SETTEXT, 0, (LPARAM)"");
		break;*/

	case IDM_EDIT_SELECTALL:
		if(hwndactive)
			SendMessage(peditorbase->m_hwnd, EM_SETSEL, (WPARAM)0, (LPARAM)-1); 
		break;

    case IDM_EDIT_FINDNEXT:
    case ACCEL_FINDNEXT:
        if(!ptabeditor && !ptabhistory && !ptabobject && tabicon != IDI_TABLE)
        {
#ifndef COMMUNITY
            if(!ptabdbsearch)
                break;
#else
            break;
#endif
        }
		if(hwndactive)
            OnAccelFindNext(pcquerywnd);
        break;

	case IDM_EDIT_FIND:
	case ACCEL_FIND:
		if(!ptabeditor && !ptabhistory && !ptabobject && tabicon != IDI_TABLE)
        {
#ifndef COMMUNITY
            if(!ptabdbsearch)
                break;
#else
            break;
#endif
        }

		if(hwndactive)
			FindTextOrReplace(pcquerywnd, wyFalse);
		break;		

	case IDM_EDIT_REPLACE:
	case ACCEL_REPLACE:
        if(!ptabeditor)
            break;
		if(hwndactive )
			FindTextOrReplace(pcquerywnd, wyTrue);
		break;		

	case IDM_EDIT_GOTO:
	case ACCEL_GOTO:
        if(!ptabeditor)
            break;
		if(hwndactive)
            GoToLine(hwndactive, pcquerywnd);
		break;		

	case ACCEL_TMENU:
	case IDM_FILE_CONNECT:
	case ID_EDIT_INSERTTEMPLATES:
		if(hwndactive)
			pcquerywnd->ShowTemplateDlg();
		break;

	case ID_EDIT_INSERTFILE:
		pcquerywnd->HandleFileOpen(SQLINDEX, wyTrue);
		break;

	case ACCEL_GRIDFORM:
        if(hwndactive)
		{
#ifndef COMMUNITY
        
            if(tabicon == IDI_DATASEARCH)
            {
                ptabdbsearch->m_pdataview->AccelaratorSwitchForm();
            }
            else
		
#endif
		    if(ptabledata)
            {
                ptabledata->m_tableview->AccelaratorSwitchForm();
            }
            else if(ptabmgmt && ptabmgmt->m_presultview)
            {
                ptabmgmt->m_presultview->AccelaratorSwitchForm();
            }
		}			
		break;

    case ACCEL_RESULT:
	case IDM_EDIT_RESULT_TEXT:
        if(!ptabeditor && !ptabledata && tabicon != IDI_DATASEARCH)
            break;
        if(hwndactive)
		{
#ifndef COMMUNITY
		    if(tabicon == IDI_DATASEARCH)
            {
                ptabdbsearch->m_pdataview->ShowTextView();
            }
            else
#endif
        
            if(ptabledata)
            {
                ptabledata->m_tableview->ShowTextView();
            }
            else if(ptabmgmt && ptabmgmt->GetDataView())
            {
                ptabmgmt->GetDataView()->ShowTextView();
            }
        }
		break;		

    case ACCEL_CHANGEVIEW:
        if(hwndactive)
		{
      
#ifndef COMMUNITY
		    if(tabicon == IDI_DATASEARCH)
            {
                ptabdbsearch->m_pdataview->SwitchView();
            }
            else
#endif
            if(ptabledata)
            {
                ptabledata->m_tableview->SwitchView();
            }
            else if(ptabmgmt && ptabmgmt->GetDataView())
            {
                ptabmgmt->GetDataView()->SwitchView();
            }
		}
        break;

	case ACCEL_OBJECT:
	case IDC_EDIT_SHOWOBJECT:
		if(hwndactive)
		{
			pGlobals->m_pcmainwin->CheckOrUncheckObjBrowserVis(pcquerywnd);
			pcquerywnd->ShowOrHideObjBrowser();
		}
		break;		

	case ACCEL_RESULTWINDOW:
	case IDC_EDIT_SHOWRESULT:
        if(!ptabeditor)
            break;
		if(hwndactive)
		{
			ret = pGlobals->m_pcmainwin->CheckOrUncheckResultWindowVis(pcquerywnd);
			if(ret)
				pcquerywnd->ShowOrHideResultWindow();
		}
		break;		

	case ACCEL_EDITWINDOW:
	case IDC_EDIT_SHOWEDIT:
        if(!ptabeditor)
            break;
		if(hwndactive)
		{
			ret = pGlobals->m_pcmainwin->CheckOrUncheckEditWindowVis(pcquerywnd);
			if(ret == wyTrue)
				pcquerywnd->ShowOrHideEditWindow();
		}
		break;
		
	case IDM_EDIT_ADVANCED_UPPER:
	case ACCEL_UPPERCASE:
        if(!ptabeditor)
            break;
		if(hwndactive )
			peditorbase->MakeSelUppercase();
		break;

	case IDM_EDIT_ADVANCED_LOWER:
	case ACCEL_LOWERCASE:
        if(!ptabeditor)
            break;
		if(hwndactive )
			peditorbase->MakeSelLowercase();
		break;

	case IDM_EDIT_ADVANCED_COMMENT:
	case ACCEL_COMMENT:
        if(!ptabeditor)
            break;
		if(hwndactive)
			peditorbase->CommentSel();
		break;

	case IDM_EDIT_ADVANCED_REMOVE:
	case ACCEL_REMOVE_COMMENT:
        if(!ptabeditor)
            break;
		if(hwndactive)
			peditorbase->CommentSel(wyTrue);
		break;

	case ACCEL_EXPORT:
	case IDM_IMEX_EXPORTDATA:
        if(!ptabeditor &&  
#ifndef COMMUNITY
            !ptabdbsearch &&
#endif
            !ptabledata)
        {
            break;
        }

		if(hwndactive)
        {
            if(ptabledata && (mydata = ptabledata->m_tableview->GetData()) && mydata->m_datares)
            {
                ptabledata->m_tableview->ExportData();
            }
            else if(ptabeditor && (mydata = ptabeditor->m_pctabmgmt->GetResultData()) && mydata->m_datares)
            {
                ptabeditor->m_pctabmgmt->GetDataView()->ExportData();
            }
#ifndef COMMUNITY
            else if(ptabdbsearch && (mydata = ptabdbsearch->m_pdataview->GetData()) && mydata->m_datares)
            {
                ptabdbsearch->m_pdataview->ExportData();
            }
#endif
        }

		break;
        
	case IDC_PREF:
        ManagePreferences();
		break;

	case ACCEL_IMPORTBATCH:
	case ID_IMEX_TEXTFILE:
	case ID_IMEX_TEXTFILE2:
        if(hwndactive)
            OnImportFromSQL(pcquerywnd);
		break;

	case ACCEL_EXPORTBATCH:
	case ID_IMPORTEXPORT_DBEXPORTTABLES:
	case ID_IMPORTEXPORT_DBEXPORTTABLES2:
	case ID_IMPORTEXPORT_TABLESEXPORTTABLES:
	case ID_IMPORTEXPORT_EXPORTTABLES:
		if(hwndactive) 
            pGlobals->m_pcmainwin->m_connection->OnExport(pcquerywnd, wParam);
		break;

	case ACCEL_EXPORTCSV:
	case ID_EXPORT_AS:
		if(hwndactive)
			pcquerywnd->EnableExportDialog();
		break;

	case ACCEL_IMPORTCSV:
	case ID_IMPORT_FROMCSV:
		if(pcquerywnd->m_conninfo.m_isreadonly == wyTrue)
			break;
		if(hwndactive)
			pcquerywnd->m_pcqueryobject->ImportFromCSV(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql);
		break;
	case ACCEL_IMPORTXML:	
	case ID_IMPORT_FROMXML:
		if(pcquerywnd->m_conninfo.m_isreadonly == wyTrue)
			break;
		if(hwndactive)
			pcquerywnd->m_pcqueryobject->ImportFromXML(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql);
		break;
			break;

	case ACCEL_CREATEDB:
	case IDM_CREATEDATABASE:
		if(hwndactive)
			OnCreateDatabase(hwndactive, pcquerywnd);	
		break;
	
	
	case ACCEL_FLUSH:
	case ID_TOOLS_FLUSH:
		if(hwndactive)
			pcquerywnd->ShowFlushDlg();
		break;

	case ACCEL_TABLEDIAG:
	case IDM_TOOLS_TABLEDIAG:
		if(hwndactive)
            OnTableDiag(pcquerywnd);
		break;

	case ACCEL_ADDUSER:
	case IDM_TOOL_ADDUSER:
    case ID_TOOLS_USERMANAGER:
		if(hwndactive)
            OnManageUser();
		break;

	case ACCEL_MANUSER:
	case ID_TOOLS_MANPERM:
		/*if(hwndactive)
            OnUserPermission(pcquerywnd);*/
		break;

	case ID_SHOW_VARIABLES:
		if(hwndactive)
            OnShowValues(pcquerywnd, VARIABLES);
		break;

	case ID_SHOW_PROCESSLIST:
		if(hwndactive)
			OnShowValues(pcquerywnd, PROCESSLIST);
		break;

	case ID_SHOW_STATUS:
		if(hwndactive)
		    OnShowValues(pcquerywnd, STATUS);
		break;

	case ID_OBJECT_DROPDATABASE:
		if(hwndactive )
			pcquerywnd->m_pcqueryobject->DropDatabase(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql);
		break;

	case ID_OBJECT_EMPTYDATABASE:
		if(hwndactive)
			pcquerywnd->m_pcqueryobject->EmptyDatabase(pcquerywnd->m_hwnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql);
		break;

	case ID_OBJECT_TRUNCATEDATABASE:
		if(hwndactive)
			pcquerywnd->m_pcqueryobject->TruncateDatabase(pcquerywnd->m_hwnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql);
		break;

	case ID_OBJECT_DROPTABLE:
		if(hwndactive)
			pcquerywnd->m_pcqueryobject->DropTable(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql);
		break;

	case ACCEL_REORDER:
	case ID_OBJECT_REORDER:
#ifndef COMMUNITY
	if(pcquerywnd->m_conninfo.m_isreadonly == wyTrue)
	{
		break;
	}
#endif
		if(hwndactive )
            pcquerywnd->m_pctabmodule->CreateTableTabInterface(pcquerywnd, wyTrue, TABCOLUMNS);
			//pcquerywnd->m_pcqueryobject->ReorderColumns();
		break;

	case ID_OBJECT_DROPFIELD:
		if(hwndactive )
			pcquerywnd->m_pcqueryobject->DropField(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql);
		break;
	
	case ID_DB_CREATESTOREDPROCEDURE:
	case ID_OBJECT_CREATESTOREDPROCEDURE:
		if(hwndactive )
            OnCreateProcedure(hwndactive, pcquerywnd);
		break;

	case ID_DB_CREATEFUNCTION:
	case ID_OBJECT_CREATEFUNCTION:
		if(hwndactive )
            OnCreateFunction(hwndactive, pcquerywnd);
		break;
	
	case ID_DB_CREATEVIEW:
	case ID_OBJECT_CREATEVIEW:
		if(hwndactive )
            OnCreateView(hwndactive, pcquerywnd);
		break;

	case ID_DB_CREATEEVENT:
	case ID_OBJECT_CREATEEVENT:
		if(hwndactive )
            OnCreateEvent(hwndactive, pcquerywnd);
		break;

    case ID_OBJECT_EXPORTVIEW:
        pcquerywnd->ExportViews();
        break;

	case ID_DB_CREATETRIGGER:			
	case ID_OBJECT_CREATETRIGGER:
		if(hwndactive )
            OnCreateTrigger(hwndactive, pcquerywnd);
		break;

	case ID_OBJECT_ALTERVIEW:
		if(hwndactive )
            OnAlterView(pcquerywnd);
		break;

	case ID_OBJECT_ALTERSTOREDPROCEDURE:
		if(hwndactive )
            OnAlterProcedure(pcquerywnd);
		break;

	case ID_OBJECT_ALTERFUNCTION:
		if(hwndactive )
            OnAlterFunction(pcquerywnd);
		break;

	case ID_OBJECT_ALTEREVENT:
		if(hwndactive )
			OnAlterEvent(pcquerywnd);
		break;

	case ID_OBJECT_ALTERTRIGGER:
		if(hwndactive )
            OnAlterTrigger(pcquerywnd);
		break;

	case ID_OBJECT_DROPVIEW:
		if(hwndactive )
			pcquerywnd->m_pcqueryobject->DropDatabaseObject(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, "view");
		break;
	case ID_OBJECT_DROPEVENT:
		if(hwndactive )
			pcquerywnd->m_pcqueryobject->DropDatabaseObject(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, "event");
		break;
	case ID_OBJECT_DROPSTOREDPROCEDURE:
		if(hwndactive )
			pcquerywnd->m_pcqueryobject->DropDatabaseObject(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, "procedure");
		break;

	case ID_OBJECT_DROPTRIGGER:
		if(hwndactive )
			pcquerywnd->m_pcqueryobject->DropTrigger(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql);
		break;

	case ID_OBJECT_DROPFUNCTION:
		if(hwndactive )
			pcquerywnd->m_pcqueryobject->DropDatabaseObject(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, "function");
		break;

	case ID_COLUMNS_DROPINDEX:
		if(hwndactive)
			pcquerywnd->m_pcqueryobject->DropIndex(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql);
		break;

    case ID_COLUMNS_MANAGECOLUMNS:
        if(hwndactive)
			pcquerywnd->m_pctabmodule->CreateTableTabInterface(pcquerywnd, wyTrue, 0);
        break;

	case ACEL_EMPTYTABLE:
	case ID_OBJECT_CLEARTABLE:
		if(hwndactive )
			pcquerywnd->m_pcqueryobject->EmptyTable(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql);
			break;

	case ACCEL_COPYTABLE:
	case ID_OBJECT_COPYTABLE:
		if(hwndactive)
			pcquerywnd->m_pcqueryobject->CopyTable(pcquerywnd);
		break;

	case ID_OBJECT_VIEWDATA:
	case ACCEL_INSERTUPDATE:
	case ID_OBJECT_INSERTUPDATE:
		if(hwndactive)
        {
            SetFocus(NULL);
			CreateNewTableDataTab(pcquerywnd);
        }

		break;

    case ID_TABLE_OPENINNEWTAB:
    case ACCEL_OPENTABLESTICKY:
        if(hwndactive)
        {
            if(pcquerywnd->m_pcqueryobject->GetSelectionImage() == NTABLE)
            {
                SetFocus(NULL);
			    CreateNewTableDataTab(pcquerywnd, wyTrue);
            }
        }

        break;

	case ID_OBJECT_INSERTSTMT:
	case ACCEL_INSERT:
		if(hwndactive && peditorbase && pcquerywnd->m_conninfo.m_isreadonly == wyFalse)
		{
			pcquerywnd->m_pcqueryobject->CreateInsertStmt();
			EditorFont::SetLineNumberWidth(peditorbase->m_hwnd);    
		}
		break;

	case ID_OBJECT_DELETESTMT:
	case ACCEL_DELETE:
		if(hwndactive && peditorbase && pcquerywnd->m_conninfo.m_isreadonly == wyFalse)
		{
			pcquerywnd->m_pcqueryobject->CreateDeleteStmt();
			EditorFont::SetLineNumberWidth(peditorbase->m_hwnd);    
		}
		break;

	case ID_OBJECT_UPDATESTMT:
	case ACCEL_UPDATE:
		if(hwndactive && peditorbase && pcquerywnd->m_conninfo.m_isreadonly == wyFalse)
		{
			pcquerywnd->m_pcqueryobject->CreateUpdateStmt();
			EditorFont::SetLineNumberWidth(peditorbase->m_hwnd);    
		}
		break;

	case ID_OBJECT_SELECTSTMT:
	case ACCEL_SELECT:
		if(hwndactive && peditorbase)
		{
			pcquerywnd->m_pcqueryobject->CreateSelectStmt();
			EditorFont::SetLineNumberWidth(peditorbase->m_hwnd);   
		}
		break;

	case ID_DB_TABLE_MAKER:
	case ID_TABLE_MAKER:
		if(hwndactive )
		{
            pcquerywnd->m_pctabmodule->CreateTableTabInterface(pcquerywnd);  //m_pcqueryobject->
            
            /*
			pcquerywnd->m_pcqueryobject->GetItemInfo();
			pcquerywnd->m_pcqueryobject->CreateTableMaker();
            */
		}
		break;

	case ID_OPEN_COPYTABLE:
		if(hwndactive )
		    pcquerywnd->m_pcqueryobject->CopyTableToDiffertHostDB();
		break;

	case ID_OPEN_COPYDATABASE:
		if(hwndactive )
		    pcquerywnd->m_pcqueryobject->CopyDB();
		break;
	case ID_OBJECT_RENAMEEVENT:
	case ID_OBJECTS_RENAMEVIEW:
	case ID_OBJECTS_RENAMETRIGGER:
	case ID_OBJECT_RENAMETABLE:
		if(hwndactive)
			pcquerywnd->m_pcqueryobject->RenameObject();
		break;

    case ID_INDEXES_CREATEINDEX:
        if(hwndactive)
            pcquerywnd->m_pctabmodule->CreateTableTabInterface(pcquerywnd, wyTrue, TABINDEXES);  
		break;

    case ID_INDEXES_EDITINDEX:
        if(hwndactive)
			pcquerywnd->m_pctabmodule->CreateTableTabInterface(pcquerywnd, wyTrue, TABINDEXES); 
		break;

    case ID_INDEXES_DROPINDEX:
        if(hwndactive)
			pcquerywnd->m_pcqueryobject->DropIndex(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql);
		break;

	case ACCEL_MANINDEX:
	case ID_OBJECT_MAINMANINDEX:
	case ID_OBJECT_MANINDEX:
		if(hwndactive)
            pcquerywnd->m_pctabmodule->CreateTableTabInterface(pcquerywnd, wyTrue, TABINDEXES);
		break;
		
	case IDM_TABLE_RELATION:
	case ACCEL_MANREL:
		if(hwndactive)
            pcquerywnd->m_pctabmodule->CreateTableTabInterface(pcquerywnd, wyTrue, TABFK);
		break;
		
	case IDM_ALTERDATABASE:
		if(hwndactive)	
			OnAlterDatabase(hwndactive, pcquerywnd);
		break;
	
	case ID_OBJECT_TABLEEDITOR:
		if(hwndactive)
            pcquerywnd->m_pctabmodule->CreateTableTabInterface(pcquerywnd, wyTrue);
		break;
	
	case ID_OBJECT_ADVANCED:
		if(hwndactive)
			pcquerywnd->m_pcqueryobject->AdvProperties(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql);
		break;

	case ACCEL_SCHEMA:
	case ID_OBJECT_CREATESCHEMA:
		if(hwndactive)
			pcquerywnd->m_pcqueryobject->CreateSchema();
		break;
	//case ID_DATABASE_REBUILDTAGS:		vgladcode 
	//	if(hwndactive)
	//	  m_connection->OnWmCommand(hwndactive, pcquerywnd, wParam);
	//	return wyTrue;			vgladcode


	case IDM_WINDOW_TILE:
		SendMessage(pGlobals->m_hwndclient, WM_MDITILE, 0, 0);
		return 0 ;

	case IDM_WINDOW_CASCADE:
        SendMessage(pGlobals->m_hwndclient, WM_MDICASCADE, 0, 0);
		return 0 ;

	case IDM_WINDOWS_ICONARRANGE:
		SendMessage(pGlobals->m_hwndclient, WM_MDIICONARRANGE, 0, 0);
		return 0;

	case IDM_HELP_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/");
		break;

	case ACCEL_KEYSHORT:
	case IDM_HELP_SHORT:
		return(DialogBox(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_KEYSHORT), pGlobals->m_pcmainwin->m_hwndmain, KeyShortCutsDlgProc));

	case IDM_HELP_ABOUT:
        return OnAbout();

	case IDM_HELP_CHKUPGRD:
		CheckForUpgrade(wyTrue);
		break;

    case ID_TOOLS_CHANGELANGUAGE:
    case ACCEL_CHANGELANGUAGE:
        SetLanguage(pGlobals->m_pcmainwin->m_hwndmain, wyFalse);
        break;

	case ID_HISTORY:
    case ACCEL_HISTORYTAB:
        if(hwndactive)
        {
		    pcquerywnd->m_pctabmodule->CreateHistoryTab(pcquerywnd, wyTrue);
        }
		break;

    case ACCEL_INFOTAB:
	case ID_INFOTAB:
		if(hwndactive)
        {
			pcquerywnd->m_pctabmodule->CreateInfoTab(pcquerywnd, wyTrue);
        }
		break;

	case ACCEL_FIRSTTAB:
        if(!ptabeditor && !ptabtype)
            break;
		if(hwndactive)
		{
			if(ptabmgmt)
            {
			    ptabmgmt->HideResultWindow();
			    ptabmgmt->SelectTab(0);
            }
            else
            {
                if(pcquerywnd->GetTabModule()->GetActiveTabImage() == IDI_CREATETABLE || pcquerywnd->GetTabModule()->GetActiveTabImage() == IDI_ALTERTABLE)
                {
                    TableTabInterfaceTabMgmt *tabintmgmt;
                    tabintmgmt = ((TableTabInterface*)pcquerywnd->GetTabModule()->GetActiveTabType())->m_ptabintmgmt;
                    tabintmgmt->SelectTab(0);
                }
            }
		}
		break;

	case ACCEL_SECONDTAB:
        if(!ptabeditor && !ptabtype)
            break;
		if(hwndactive )
		{
            if(ptabmgmt)
            {
			    ptabmgmt->HideResultWindow();
			    ptabmgmt->SelectTab(1);
		    }
            else
            {
                if(pcquerywnd->GetTabModule()->GetActiveTabImage() == IDI_CREATETABLE  || pcquerywnd->GetTabModule()->GetActiveTabImage() == IDI_ALTERTABLE)
                {
                    TableTabInterfaceTabMgmt *tabintmgmt;
                    tabintmgmt = ((TableTabInterface*)pcquerywnd->GetTabModule()->GetActiveTabType())->m_ptabintmgmt;
                    tabintmgmt->SelectTab(1);
                }
            }
		}
		break;

	case ACCEL_THIRDTAB:
        if(!ptabeditor && !ptabtype)
            break;
		if(hwndactive )
		{
			if(ptabmgmt)
            {
			    ptabmgmt->HideResultWindow();
			    ptabmgmt->SelectTab(2);
		    }
            else
            {
                if(pcquerywnd->GetTabModule()->GetActiveTabImage() == IDI_CREATETABLE || pcquerywnd->GetTabModule()->GetActiveTabImage() == IDI_ALTERTABLE)
                {
                    TableTabInterfaceTabMgmt *tabintmgmt;
                    tabintmgmt = ((TableTabInterface*)pcquerywnd->GetTabModule()->GetActiveTabType())->m_ptabintmgmt;// m_ptabinterfacetabmgmt;
                    tabintmgmt->SelectTab(2);
                }
            }
		}
		break;

	case ACCEL_FOURTHTAB:
        if(!ptabeditor && !ptabtype)
            break;
		if(hwndactive )
		{
			if(ptabmgmt)
            {
			    ptabmgmt->HideResultWindow();
			    ptabmgmt->SelectTab(3);
		    }
            else
            {
                if(pcquerywnd->GetTabModule()->GetActiveTabImage() == IDI_CREATETABLE || pcquerywnd->GetTabModule()->GetActiveTabImage() == IDI_ALTERTABLE)
                {
                    TableTabInterfaceTabMgmt *tabintmgmt;
                    tabintmgmt = ((TableTabInterface*)pcquerywnd->GetTabModule()->GetActiveTabType())->m_ptabintmgmt;// m_ptabinterfacetabmgmt;
                    tabintmgmt->SelectTab(3);
                }
            }
		}
		break;

	case ACCEL_FIFTHTAB:
        if(!ptabeditor && !ptabtype)
            break;
		if(hwndactive)
		{
			if(ptabmgmt)
            {
			    ptabmgmt->HideResultWindow();
			    ptabmgmt->SelectTab(4);
		    }
            else
            {
                if(pcquerywnd->GetTabModule()->GetActiveTabImage() == IDI_CREATETABLE || pcquerywnd->GetTabModule()->GetActiveTabImage() == IDI_ALTERTABLE)
                {
                    TableTabInterfaceTabMgmt *tabintmgmt;
                    tabintmgmt = ((TableTabInterface*)pcquerywnd->GetTabModule()->GetActiveTabType())->m_ptabintmgmt;// m_ptabinterfacetabmgmt;
                    tabintmgmt->SelectTab(4);
                }
            }
		}
		break;

	case ACCEL_SIXTHTAB:
        if(!ptabeditor)
            break;
		if(hwndactive)
		{
			ptabmgmt->HideResultWindow();
			ptabmgmt->SelectTab(5);
		}
		break;

	case ACCEL_SEVENTHTAB:
        if(!ptabeditor)
            break;
		if(hwndactive )
		{
			ptabmgmt->HideResultWindow();
			ptabmgmt->SelectTab(6);

		}
		break;

	case ACCEL_EIGHTHTAB:
        if(!ptabeditor)
            break;
		if(hwndactive )
		{
			ptabmgmt->HideResultWindow();
			ptabmgmt->SelectTab(7);
			
		}
		break;

	case ACCEL_NINTHTAB:
        if(!ptabeditor && !ptabtype)
            break;
		if(hwndactive )
		{
            if(ptabmgmt)
            {
			    ptabmgmt->HideResultWindow();
			    ptabmgmt->SelectTab(CustomTab_GetItemCount(ptabmgmt->m_hwnd) - 1);
            }
            else
            {
                if(pcquerywnd->GetTabModule()->GetActiveTabImage() == IDI_CREATETABLE || pcquerywnd->GetTabModule()->GetActiveTabImage() == IDI_ALTERTABLE)
                {
                    TableTabInterfaceTabMgmt *tabintmgmt;
                    tabintmgmt = ((TableTabInterface*)pcquerywnd->GetTabModule()->GetActiveTabType())->m_ptabintmgmt;// m_ptabinterfacetabmgmt;
                    tabintmgmt->SelectTab(4);
                }
            }
		}
		break;

	case ACCEL_FOCUSEDITOR:
        if(hwndactive)
		{	
			if(ptabeditor)
			{
				/*if(pcquerywnd->GetActiveTabEditor())
					CustomGrid_ApplyChanges(pcquerywnd->GetActiveTabEditor()->m_pctabmgmt->m_insert->m_hwndgrid, wyTrue);*/		// for closing the Grid Listbox
				SetFocus(peditorbase->m_hwnd);
			}
#ifndef COMMUNITY
			else if(ptabdbsearch)
			{
				ptabdbsearch->SetFocusSearchBox();
			}
#endif
		}    
		break;

	case ACCEL_FOCUSRESULT:
        if(!ptabeditor)
            break;

		//if(hwndactive)
		//	ptabmgmt->SetFocusToResultPane();
		if(hwndactive)
		{
			//ptabeditor->m_peditorbase->ShowResultWindow();

			//if(ptabeditor->m_pctabmgmt && ptabeditor->m_pctabmgmt->m_presultview)
			//	if(ptabeditor->m_pctabmgmt->m_presultview->GetActiveDispWindow())
			//SetFocus(ptabeditor->m_pctabmgmt->m_presultview->GetActiveDispWindow());
			//SetFocus(ptabmgmt->m_hwnd);
			if(ptabmgmt && ptabmgmt->m_hwnd)
				ptabmgmt->SelectTab(CustomTab_GetCurSel(ptabmgmt->m_hwnd));
		}

		break;

	case ACCEL_FOCUSOBJECT:
		if(hwndactive)
			SetFocus(pcquerywnd->m_pcqueryobject->m_hwnd);
		break;
        case ID_EXPORT_EXPORTCONNECTIONDETAILS:
            OnExportConnectionDetails();
            break;
        case ID_EXPORT_IMPORTCONNECTIONDETAILS:
            OnImportConnectionDetails();
            break;
	case IDM_DATATOCLIPBOARD:
		break;

	case IDM_SELDATATOCLIPBOARD:
		break;

	case ACCEL_ADDFAVORITE:
	case ID_ADDTOFAVORITES:
        if(!ptabeditor)
            break;
		if(hwndactive)
			m_pcaddfavorite->Display(hwndactive);
		break;
		
	case ID_REFRESHFAVORITES:
		if(hwndactive)
			m_pcfavoritemenu->Display(pGlobals->m_pcmainwin->m_hwndmain);
		break;

	case ID_ORGANIZEFAVORITES:
		if(hwndactive)
			m_pcorganizefavorite->Display(hwndactive);
		break;

    // select 1 to 8 and then last connections, on Ctrl+1,2....9
	case ACCEL_FIRSTCONN:
	case ACCEL_SECONDCONN:
	case ACCEL_THIRDCONN:
	case ACCEL_FOURTHCONN:
	case ACCEL_FIFTHCONN:
	case ACCEL_SIXTHCONN:
	case ACCEL_SEVENTHCONN:
	case ACCEL_EIGTHCONN:
	case ACCEL_NINTHCONN:
		if(hwndactive)
		{
			conntab->SelectActiveConnection(LOWORD(wParam));
		}
		break;
		
    
    /*case ACCEL_NOFORMATCOPY:
        ptabeditor->HandleNoFormatCopy();
        break;*/

	}	
	return 1;
}


void
FrameWindow::OnExportConnectionDetails()
{
    wyInt32 ret;
    ExportImportConnection *e=new ExportImportConnection(false);
    ret = e->ActivateConnectionManagerDialog(pGlobals->m_pcmainwin->m_hwndmain);
    delete(e);
    
}

void FrameWindow::OnImportConnectionDetails()
{
    wyInt32 ret;
    ExportImportConnection *e=new ExportImportConnection(true);
    ret=e->ActivateConnectionManagerDialog(pGlobals->m_pcmainwin->m_hwndmain);
    delete(e);
}

// Function to change the status bar information on mousehover change
wyInt32
FrameWindow::OnWmMenuSelect(HMENU hmenuhandle, wyInt32 menuid)
{
	wyInt32			ret = 0;
	wyWChar			buff[SIZE_512]={0};
	ConnectionBase *conbase = pGlobals->m_pcmainwin->m_connection;
	wyWChar			name[SIZE_64];
	MDIWindow		*wnd = GetActiveWin();

	//Build/Rebuild tag menu text is changing dynamically
	if(menuid == ID_REBUILDTAGS && conbase)
	{		
		if(conbase->m_isbuiltactagfile == wyFalse)
		{
			AddTextInStatusBar(_(L"Build tag file"));
			if(wnd)
			{
				wnd->m_statusbartext.SetAs(_(L"Build tag file"));
			}
		}
		else
		{
			AddTextInStatusBar(_(L"Rebuild tag file"));
			if(wnd)
			{
				wnd->m_statusbartext.SetAs(_(L"Rebuild tag file"));
			}
		}
	}

	//For all other menus except Powertools->Build/Rebuild tag
	else
	{
		if(menuid >= FAVORITEMENUID_START && menuid <= FAVORITEMENUID_END)
			ret = LoadString(pGlobals->m_hinstance, FAVORITE_MENU_ID, buff, SIZE_512-1);
		else
			ret = LoadString(pGlobals->m_hinstance, menuid, buff, SIZE_512-1);

		if(ret == 0)
		{
			wcscpy(buff, _(L" Ready"));
		}

		AddTextInStatusBar(buff);
		if(wnd)
		{
			wnd->m_statusbartext.SetAs(buff);
		}
	}

	//Handles the Table's Engine menu
	if(hmenuhandle)
	{
		GetMenuString(hmenuhandle, menuid, name, SIZE_64, MF_BYPOSITION);
				
		//Check for Engine sub-menu here
		if(!wcsicmp(name, _(L"Change Table T&ype To")))
		{
			conbase->HandleTableEngineMenu(hmenuhandle, menuid);
		}
		if(!wcsicmp(name, _(L"&Columns"))) 
		{
			if(GetMenuItemID(GetSubMenu(hmenuhandle,menuid), 1) == ID_DATASEARCH)
			{
				FreeMenuOwnerDrawItem(GetSubMenu(hmenuhandle,menuid), 1);
				RemoveMenu(GetSubMenu(hmenuhandle,menuid),ID_DATASEARCH, MF_BYCOMMAND);
				DrawMenuBar(m_hwndmain);
			}
		}
		if(!wcsicmp(name, _(L"T&able")) || !wcsicmp(name, _(L"&Database"))) 
		{
			if(GetMenuItemID(GetSubMenu(hmenuhandle,menuid), 2) == ID_DATASEARCH)
			{
				FreeMenuOwnerDrawItem(GetSubMenu(hmenuhandle,menuid), 2);
				RemoveMenu(GetSubMenu(hmenuhandle,menuid),ID_DATASEARCH, MF_BYCOMMAND);
				DrawMenuBar(m_hwndmain);
			}
			if(GetMenuItemID(GetSubMenu(hmenuhandle,menuid), 3) == ID_DB_DATASEARCH)
			{
				FreeMenuOwnerDrawItem(GetSubMenu(hmenuhandle,menuid), 3);
				RemoveMenu(GetSubMenu(hmenuhandle,menuid),ID_DB_DATASEARCH, MF_BYCOMMAND);
				DrawMenuBar(m_hwndmain);
			}
			
		}
	}
		
	return 0;
}

// Gets the current selected item in the combobox in the toolbar.
wyInt32
FrameWindow::GetCurrentSel()
{
	wyInt32		index;

	VERIFY((index	= SendMessage(m_hwndtoolcombo, CB_GETCURSEL, 0, 0))!= CB_ERR);

	return index;
}

// The function is called whenever ths combo box is drop downed.
// Before showing any info it gets all the database in the mysql connection of the current active window
// and adds them to combo box. If any error occurs while getting info about the databases then
// it shows the error and add only No Database Selected and return
wyBool
FrameWindow::OnToolComboDropDown()
{
	FillComboWithDBNames(m_hwndtoolcombo);	
	
	return wyTrue;
}

// This function is called whenever there is a change in selection in the combobox in the toolbar.
// When a new database is selected then we change the current database in the MYSQL connection.

wyBool
FrameWindow::OnToolComboSelChange()
{
	wyInt32         index;
	wyWChar          db[SIZE_512] = {0};
    wyString        query;
	MDIWindow *		pcquerywnd;
	COMBOBOXEXITEM	cbi;
    MYSQL_RES       *res;

	VERIFY(pcquerywnd = GetActiveWin());
	// Now change the cursor to wait mode.
	VERIFY(SetCursor(LoadCursor(NULL, IDC_WAIT)));
	// Get the original database so if the user dosnt select a database or an error occurs
	// then change it to the current database. i mean show it in combodropdown.
	VERIFY((index = SendMessage(m_hwndtoolcombo, CB_GETCURSEL, 0, 0)) != CB_ERR);	

	cbi.mask        = CBEIF_TEXT ;
	cbi.pszText     = db;
	cbi.cchTextMax  = sizeof(db);
	cbi.iItem       = index;
	
	VERIFY(SendMessage(m_hwndtoolcombo, CBEM_GETITEM, 0,(LPARAM)&cbi));
	
	wyString dbname(db);

	query.Sprintf("use `%s`", dbname.GetString());

    res = ExecuteAndGetResult(pcquerywnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query);
	if(!res && pcquerywnd->m_tunnel->mysql_affected_rows(pcquerywnd->m_mysql)== -1)
	{
		ShowMySQLError(m_hwndmain, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query.GetString());
		return wyFalse;
	}

	pcquerywnd->m_tunnel->mysql_free_result(res);
	pcquerywnd->m_tunnel->SetDB(dbname.GetString());

	pcquerywnd->m_database.Sprintf("%s", dbname.GetString());
	pGlobals->m_lastdatabase.SetAs(db);

	SendMessage(m_hwndtoolcombo, CB_SETCURSEL,(WPARAM)index, 0);
	SetFocus(m_hwndtoolcombo);
	AddTextInCombo(db);

	//selecting new database in to the object browser
	pcquerywnd->m_pcqueryobject->OnComboChanged(db);

	return wyTrue;
}

// get current database from the combobox
wyBool
FrameWindow::GetCurrentSelDB(wyWChar *db, wyInt32 size)
{
	wyInt32         index;
	COMBOBOXEXITEM  cbi;

	VERIFY((index = SendMessage(m_hwndtoolcombo, CB_GETCURSEL, 0, 0)) != CB_ERR);	

	cbi.mask        = CBEIF_TEXT ;
	cbi.pszText     = db;
	cbi.cchTextMax  = size;
	cbi.iItem       = index;
	
	VERIFY(SendMessage(m_hwndtoolcombo, CBEM_GETITEM, 0,(LPARAM)&cbi));

	if(wcsicmp(db, NODBSELECTED) == 0)
		db[0] = 0;

	return wyTrue;
}

// This function is called whenever there is any error selecting a database in the combobox.
// It adds the old database which is stored in the QueryWnd class and adds it to the combobox
// so that the user dosnt get confused that the database has changed.
wyBool
FrameWindow::SetOldDB()
{
	MDIWindow *	pcquerywnd;		

	VERIFY(pcquerywnd = GetActiveWin());

    if(pcquerywnd)
		AddTextInCombo(pcquerywnd->m_database.GetAsWideChar());
		
	return wyTrue;
}

// This function is called whenever the system sends notification requiring toottip text for the
// toolbar.
wyBool
FrameWindow::OnToolTipInfo(LPNMTTDISPINFO lpnmtdi)
{
	wyWChar      string[SIZE_512]={0};
	MDIWindow	*wnd;

	wnd = GetActiveWin();

    m_connection->OnToolTipInfo(lpnmtdi, string);

    if(!wcslen(string))
    {
	    switch(lpnmtdi->hdr.idFrom)
	    {

	    case IDM_FILE_NEWCONNECTION:
		    wcscpy(string, L"Create A New Connection (Ctrl+M)");
		    break;

	    case ID_NEW_EDITOR:
		    wcscpy(string, L"New Query Editor (Ctrl+T)");
		    break;

	    case ID_OBJECT_VIEWDATA:
		    wcscpy(string, L"View Table Data");
		    break;

	    case IDM_FILE_OPENSQL:
		    VERIFY(LoadString(pGlobals->m_hinstance, IDM_FILE_OPENSQL, string, SIZE_512-1));
		    break;

	    //To open file in new tab
	    case IDM_FILE_OPENSQLNEW:
		    VERIFY(LoadString(pGlobals->m_hinstance, IDM_FILE_OPENSQLNEW, string, SIZE_512-1));
		    break;

	    case IDM_FILE_SAVESQL:
		    VERIFY(LoadString(pGlobals->m_hinstance, IDM_FILE_SAVESQL, string, SIZE_512-1));
		    break;

	    case IDM_EDIT_CUT:
		    VERIFY(LoadString(pGlobals->m_hinstance, IDM_EDIT_CUT, string, SIZE_512-1));
		    break;

	    case IDM_EDIT_COPY:
		    VERIFY(LoadString(pGlobals->m_hinstance, IDM_EDIT_COPY, string, SIZE_512-1));
		    break;

        case IDM_COPYNORMALIZED:
		    VERIFY(LoadString(pGlobals->m_hinstance, IDM_COPYNORMALIZED, string, SIZE_512-1));
		    break;

	    case IDM_EDIT_PASTE:
		    VERIFY(LoadString(pGlobals->m_hinstance, IDM_EDIT_PASTE, string, SIZE_512-1));
		    break;

	    case IDM_EXECUTE:
		    if(wnd && wnd->m_executing)
			    wcscpy(string, L"Stop Execution");
			//else if(wnd /*&& wnd->m_pingexecuting*/)
			//{
			//	break;
			//}
		    else
		    {
			    if(pGlobals->m_isrefreshkeychange == wyFalse)
				    wcscpy(string, L"Execute Query (F9)");
			    else
				    wcscpy(string, L"Execute Query (F5)");
		    }
		    break;

	    case ACCEL_EXECUTEALL:
	    case ID_STOPQUERY:
		    if(wnd && wnd->m_executing)
			    wcscpy(string, L"Stop Execution");
		    else
		    {
			    if(pGlobals->m_isrefreshkeychange == wyFalse)
				    wcscpy(string, L"Execute All Queries (Ctrl+F9)");
			    else
				    wcscpy(string, L"Execute All Queries (Ctrl+F5)");
		    }
		    break;		

	    case ACCEL_QUERYUPDATE:
	    case ACCEL_EXECUTESEL:
		    wcscpy(string, L"Execute And Edit Resultset (F8)");
		    break;

	    case IDM_REFRESHOBJECT:
		    if(pGlobals->m_isrefreshkeychange == wyFalse)
			    wcscpy(string, L"Refresh Object Browser (F5)");
		    else
			    wcscpy(string, L"Refresh Object Browser (F9)");
		    break;

	    case IDM_TMAKER_SAVE:
		    VERIFY(LoadString(pGlobals->m_hinstance, IDM_TMAKER_SAVE, string, SIZE_512-1));
		    break;

	    case ID_IMPORTEXPORT_DBEXPORTTABLES:
		    wcscpy(string, L"Backup Database As SQL Dump (Ctrl+Alt+E)");
		    break;

	    case IDM_TOOL_ADDUSER:
		    wcscpy(string, L"User Manager (Ctrl+U)");
		    break;
		
	    case ID_IMEX_TEXTFILE:
		    wcscpy(string, L"Execute SQL Script (Ctrl+Shift+Q)");
		    break;

	    case ID_OPEN_COPYDATABASE:
		    wcscpy(string, L"Copy Database To Different Host/Database");
		    break;
		
	    case ID_EXPORT_AS:
		    wcscpy(string, L"Export Table Data (Ctrl+Alt+C)");
		    break;

		
	    case ID_OBJECT_MAINMANINDEX:
		    wcscpy(string, L"Manage Indexes (F7)");
		    break;
		
	    case ACCEL_MANREL:
		    wcscpy(string, L"Relationships/Foreign Keys (F10)");		
		    break;	

        case ID_QB_QUERYTOCLIPBOARD:
            wcscpy(string, L"Copy Query To Clipboard");
            break;

        case ID_QB_QUERYTONEWEDITOR:
            wcscpy(string, L"Copy Query To New Query Tab");
            break;

	    case ID_QB_QUERYTOSAMEEDITOR:
		    wcscpy(string, L"Copy Query To Same Query Tab");
            break;

	    //To add Direct Create View Option in Query Builder.
	    case ID_OBJECT_CREATEVIEW:
		    wcscpy(string, L"Create View Using Current Query");
		    break;
    
	    case ID_SD_ALTERTABLE:
		    wcscpy(string, L"Alter Table");
		    break;

	    case ID_SD_MANAGE_INDEX:
		    wcscpy(string, L"Manage Indexes");
		    break;

	    case ID_SD_MANAGE_RELATIONSHIP:
		    wcscpy(string, L"Relationships/Foreign Keys");
		    break;

	    case ID_SD_TABLEVIEW:
		    wcscpy(string, L"Table View");
		    break;

	    case ID_SCHEMA_NEWTABLE:
		    wcscpy(string, L"Create Table");
		    break;

	    case ID_SCHEMA_REFRESHCANVAS:
		    if(pGlobals->m_isrefreshkeychange == wyFalse)
			    wcscpy(string, L"Refresh Canvas (F5)");
		    else
			    wcscpy(string, L"Refresh Canvas (F9)");
		    break;
	
	    case ID_SCHEMA_ADDTABLES:
		    wcscpy(string, L"Add Table(s) To Canvas");
		    break;

	    case ID_SCHEMA_LOADCANVAS:
		    wcscpy(string, L"Load Schema Design (Ctrl+O)");
		    break;

	    case ID_SCHEMA_SAVEASBMP:
		    wcscpy(string, L"Export Canvas As Image");
		    break;

	    case ID_SD_PRINTCANVAS:
		    wcscpy(string, L"Print");
		    break;

	    case ID_SDZOOMIN:
		    wcscpy(string, L"Zoom In");
		    break;

	    case ID_SDZOOMOUT:
		    wcscpy(string, L"Zoom Out");
		    break;
	    }

        wcscpy(lpnmtdi->lpszText, _(string));
    }
    else
    {
        wcscpy(lpnmtdi->lpszText, string);
    }

	return wyTrue;
}

// Function is called whenever a new database is selected in query by using USE statement.
// This function calls the select database()query in mySQL and gets the currrent database 
// and adds it to the combobox.
wyBool
FrameWindow::ChangeDBInCombo(Tunnel * tunnel, PMYSQL mysql, wyWChar * db)
{
	wyString        query;
	MYSQL_RES*      myres;
	MYSQL_ROW       myrow;
	MDIWindow*      pcquerywnd;
	wyString		dbname(db);
	wyString		myrowstr;

	VERIFY(pcquerywnd = GetActiveWin());
	
	query.Sprintf("select database()");

	myres = ExecuteAndGetResult(pcquerywnd, tunnel, mysql, query);
	if(!myres)
	{
		ShowMySQLError(m_hwndmain, tunnel, mysql, query.GetString());
		return wyFalse;
	}

	myrow	=	tunnel->mysql_fetch_row(myres);
	
	if(myrow[0])
    {
		myrowstr.SetAs(myrow[0], IsMySQL41(tunnel, mysql));
		AddTextInCombo(myrowstr.GetAsWideChar());
		pcquerywnd->m_pcqueryobject->OnComboChanged(myrowstr.GetAsWideChar());
		pcquerywnd->m_database.Sprintf("%s", myrowstr.GetString());
		// Also copy it in the the temporary buffer so that we can select it when a new mySQLyog window is selected.
		pGlobals->m_lastdatabase.SetAs(myrow[0], IsMySQL41(tunnel, mysql));
	} 
    else 
		AddTextInCombo(NODBSELECTED);

	if(db)
    {
		AddTextInCombo(db);
		pcquerywnd->m_pcqueryobject->OnComboChanged(db);
		pcquerywnd->m_database.Sprintf("%s", dbname.GetString());
		pGlobals->m_lastdatabase.SetAs(db);
		tunnel->SetDB(dbname.GetString());
	}

	tunnel->mysql_free_result(myres);
	return wyTrue;
}

// The function initializes the main menu with all the latest files which are there in the init file.
wyBool
FrameWindow::InitLatestFiles()
{
	ReInitLatestFiles(wyTrue);

	return wyTrue;
}

void
FrameWindow::GetSQLyogIniPath(wyString *path)
{
	wyWChar     directory[MAX_PATH + 1], *lpfileport = 0;
	wyUInt32    ret;
	
	// Get the complete path.
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	if(ret == 0)
		return;

	path->SetAs(directory);
}

// This function initializes the splitter position taken from the ini file.
wyBool
FrameWindow::InitSplitterPos(const wyChar *dirstr)
{
	//wyWChar     directory[MAX_PATH + 1], *lpfileport = 0;
	//wyUInt32    ret;
	//wyString	dirstr;

	//// Get the complete path.
	//ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	//if(ret == 0)
	//	return wyFalse;
	//dirstr.SetAs(directory);

    // first get the horz splitter position
	
	m_rchsplitter.left = wyIni::IniGetInt(HSPLITTER_SECTION_NAME, "Left", 0, dirstr);
	m_rchsplitter.right = wyIni::IniGetInt(HSPLITTER_SECTION_NAME, "Right", 0, dirstr);
	m_rchsplitter.top = wyIni::IniGetInt(HSPLITTER_SECTION_NAME, "Top", 0, dirstr);
	m_rchsplitter.bottom = wyIni::IniGetInt(HSPLITTER_SECTION_NAME, "Bottom", 0, dirstr);
	m_toppercent = wyIni::IniGetInt(HSPLITTER_SECTION_NAME, "ULeftOrTopPercent", 0, dirstr);
	
	// then get the vert splitter position
	m_rcvsplitter.left = wyIni::IniGetInt(VSPLITTER_SECTION_NAME, "Left", 0, dirstr);
	m_rcvsplitter.right = wyIni::IniGetInt(VSPLITTER_SECTION_NAME,  "Right", 0, dirstr);
	m_rcvsplitter.top = wyIni::IniGetInt(VSPLITTER_SECTION_NAME, "Top", 0, dirstr);
	m_rcvsplitter.bottom = wyIni::IniGetInt(VSPLITTER_SECTION_NAME, "Bottom", 0, dirstr);
	m_leftpercent = wyIni::IniGetInt(VSPLITTER_SECTION_NAME, "ULeftOrTopPercent", 0, dirstr);
	
	return wyTrue;
}

void
FrameWindow::InitPQAOptions(const wyChar *dirstr)
{
	wyUInt32 truncdata;

	if(m_connection->m_enttype == ENT_PRO || m_connection->m_enttype == ENT_NORMAL)
		return;
	
	//Eabled/Disabled PQA
	truncdata	= wyIni::IniGetInt(GENERALPREFA, "QueryAnalyzer", 1, dirstr);
	pGlobals->m_ispqaenabled = truncdata ? wyTrue : wyFalse;

	//Eabled/Disabled PQA-show profiler
	truncdata	= wyIni::IniGetInt(GENERALPREFA, "PQAProfile", 1, dirstr);
	pGlobals->m_ispqashowprofile = (truncdata && pGlobals->m_ispqaenabled == wyTrue) ? wyTrue : wyFalse;
}

//VOID
//FrameWindow::InitAutoCompleteCaseSelection(const wyChar *dirstr)
//{
//	wyInt32		truncdata = 0;
//		
//	pGlobals->m_isautocompleteupper = wyTrue;
//
//	truncdata = wyIni::IniGetInt(GENERALPREFA, "AutoComplete", 1, dirstr);
//	
//	if(!truncdata)
//	{
//		pGlobals->m_isautocompleteupper = wyFalse;
//		return;
//	}
//	
//	//Eabled/Disabled PQA-show profiler
//	truncdata	= wyIni::IniGetInt(GENERALPREFA, "AutoCompleteUpperCase", 1, dirstr);
//	pGlobals->m_isautocompleteupper = (truncdata) ? wyTrue : wyFalse;
//
//}

// The function reinitializes the main menu and adds the updated latest file in the main menu.
// This asme fun is getting called when frame window is creating too , this could identify by the flag "iscreate = wyTrue"
wyBool
FrameWindow::ReInitLatestFiles(wyBool iscreate)
{
	wyInt32     lstyle, menupos = RECENT_SQLFILE_SUBMENU;
    wyString    compkeyvalue;
	HMENU		hmenu, hsubmenu, hmenurecent;
    HWND		hwndactive =(HWND)SendMessage(pGlobals->m_hwndclient, WM_MDIGETACTIVE, 0, NULL);
	wyString	keyvaluestr, dirstr;
	wyBool		retq = wyTrue, retsd = wyTrue, retqb = wyTrue;

	if(!hwndactive && iscreate == wyFalse)
		return wyFalse;

	hmenu = GetMenu(m_hwndmain);

	// we first check whether the first menu is in the first position or not.
	lstyle = GetWindowLongPtr(hwndactive, GWL_STYLE);

	if((lstyle & WS_MAXIMIZE) && wyTheme::IsSysmenuEnabled(hwndactive) && iscreate == wyFalse)
		hsubmenu = GetSubMenu(hmenu, 1);
	else
		hsubmenu = GetSubMenu(hmenu, 0);

	VERIFY(hmenurecent = GetSubMenu(hsubmenu, menupos));
	if(!hmenurecent)
		return wyFalse;

	retq = InsertRecentMenuItems(hmenurecent);

	/// Recent Schema design only for Enterprise
#ifndef COMMUNITY
	VERIFY(hmenurecent = GetSubMenu(hsubmenu, RECENT_SCHEMADESIGNS_SUBMENU));
	if(!hmenurecent)
		return wyFalse;

    retsd = InsertRecentMenuItems(hmenurecent, IDI_SCHEMADESIGNER_16);

    VERIFY(hmenurecent = GetSubMenu(hsubmenu, RECENT_QUERYBUILDER_SUBMENU));
	if(!hmenurecent)
		return wyFalse;

    retqb = InsertRecentMenuItems(hmenurecent, IDI_QUERYBUILDER_16);
#endif 

    if(retsd == wyFalse || retq == wyFalse || retqb == wyFalse)
		return wyFalse;

	return wyTrue;
}

wyBool					
FrameWindow::InsertRecentMenuItems(HMENU hsubmenu, wyInt32 tabimageid)
{
	wyInt32     count, idcount, menuid = 0;
    wyUInt32    ret;
	//wyChar		keyvalue[SIZE_512] = {0};
	wyWChar     directory[MAX_PATH +1] = {0}, *lpfileport = 0;
    wyString	compkeyvalue;
	wyString	keyvaluestr, dirstr, keyname, keynamecpy;
	
   	keyname.SetAs("File");
	menuid = ID_LF_1;
    
	// now we delete all the menu items and then add all the values again.
	//duplicating issue of more than 10 files
	for(count = 0; count <= MAX_RECENT_FILES; count++)
		DeleteMenu(hsubmenu, 0, MF_BYPOSITION);

	// first install a default id key.
	VERIFY(InsertMenu(hsubmenu, 0, MF_BYPOSITION | MF_STRING, menuid, _(L"No Recent Files")));

	// now we get all the keys from ini file.
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	if(ret ==0)
		return wyFalse;

	dirstr.SetAs(directory);
	
	keynamecpy.SetAs(keyname);

	for(count = 1, idcount = 1; count <= RECENT_FILE_COUNT; count++)
	{
		keyname.AddSprintf("%d", count);		

		 ret = wyIni::IniGetString(SECTION_NAME, keyname.GetString(), "0", &keyvaluestr, dirstr.GetString());
		 if(ret == 0 || keyvaluestr.GetCharAt(0) == '0')
			 continue;

		compkeyvalue.Sprintf("&%d %s", idcount, keyvaluestr.GetString());

		VERIFY(InsertMenu(hsubmenu, menuid, MF_BYCOMMAND | MF_STRING, menuid + count, compkeyvalue.GetAsWideChar()));

		 idcount++;

		keyname.SetAs(keynamecpy);
	}

	if(idcount > 1)
		DeleteMenu(hsubmenu, menuid, MF_BYCOMMAND);

	return wyTrue;
}

// Function writes the file which is passed as a parameter to the function and adds them to the
// sqlyog.ini file. but first it checks whether the name exists or not. then only it adds.
wyBool
FrameWindow::WriteLatestFile(wyWChar *filename)
{
	wyInt32		count, idcount = 0;
	wyWChar     keyvalue[MAX_RECENT_FILES * 2][SIZE_512] = {0}, directory[MAX_PATH+1] = {0}, *lpfilereport = NULL;
	
    wyString    keyname, keyvalstr, dirstr;
	wyUInt32    ret;

	MDIWindow	*wnd;
	wyInt32		tabimageid;

	VERIFY(wnd = GetActiveWin());

	tabimageid = wnd->m_pctabmodule->GetActiveTabImage();

	// clear all the buffer.
	memset(keyvalue, 0,(SIZE_512 * MAX_RECENT_FILES));

	 if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfilereport) == wyFalse)
        return wyFalse;

    if(directory)
        dirstr.SetAs(directory);

	
	dirstr.SetAs(directory);

	// Now get all the values.
	for(count = 1; count <= RECENT_FILE_COUNT; count++)
	{
		keyname.Sprintf("File%d", count);
				
		ret = wyIni::IniGetString(SECTION_NAME, keyname.GetString(), "0", &keyvalstr, dirstr.GetString());		
		
		//wcscpy(keyvalue[count-1], keyvalstr.GetAsWideChar());
		wcsncpy(keyvalue[count-1], keyvalstr.GetAsWideChar(), SIZE_512 - 1);
		keyvalue[count-1][SIZE_512 - 1] = '\0';

		if(ret == 0)
			return wyFalse;

		// we compare it here only if there exist. if it does then we simply return.
		// as the file already exists in the recent files so we dont have to do anything
		if((wcsicmp(keyvalue[count-1], filename)) == 0)
			return wyFalse;
	}

	// If it has reached here then it does not exist and we have all the values.
	// so we move one down all the keys.
	for(count = MAX_RECENT_FILES - 2; count >= 0; count--)
		wcscpy(keyvalue[count+1], keyvalue[count]);

	// at last add the new file at the top.
	//wcscpy(keyvalue[0], filename);
	wcsncpy(keyvalue[0], filename, SIZE_512 - 1);
	keyvalue[0][SIZE_512 - 1] = '\0';

	// Now write in the file also.
	for(count = 1, idcount = 1; count <= MAX_RECENT_FILES; count++)
	{
		keyname.Sprintf("File%d", idcount);
		
		if(!keyvalue[count-1][0])
			continue;
		keyvalstr.SetAs(keyvalue[count - 1]);
		ret = wyIni::IniWriteString(SECTION_NAME, keyname.GetString(), keyvalstr.GetString(), dirstr.GetString());

		idcount++;
	}
	return wyTrue;
}

// Function writes the splitter poition to the file so that when we start the application again we 
// can start in the same position.

wyBool
FrameWindow::WriteSplitterPos(wyWChar *section)
{
	//wyUInt32    count;
    wyWChar     directory[MAX_PATH+1], *lpfileport = 0;
    wyString    keyvalue, sectionstr;
	wyInt32	    ret;
	wyString	dirstr;
	
	// Get the complete path.

    if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyFalse)
        return wyFalse;
	
	dirstr.SetAs(directory);
	
	// now add all the values in the file.
	// we check what splitter we have to write.
	if((wcscmp(section, TEXT(HSPLITTER_SECTION_NAME)))== 0)
	{
		sectionstr.SetAs(section);
		keyvalue.Sprintf("%d", m_rchsplitter.left);
		ret = wyIni::IniWriteString(sectionstr.GetString(), "Left", keyvalue.GetString(), dirstr.GetString());
		if(ret)
			return wyFalse;

		keyvalue.Sprintf("%d", m_rchsplitter.right);
		ret = wyIni::IniWriteString(sectionstr.GetString(), "Right", keyvalue.GetString(), dirstr.GetString());
		if(ret)
			return wyFalse;

		keyvalue.Sprintf("%d", m_rchsplitter.top);
		ret = wyIni::IniWriteString(sectionstr.GetString(), "Top", keyvalue.GetString(), dirstr.GetString());
		if(ret)
			return wyFalse;

		keyvalue.Sprintf("%d", m_rchsplitter.bottom);
		ret = wyIni::IniWriteString(sectionstr.GetString(), "Bottom", keyvalue.GetString(), dirstr.GetString());
		if(ret)
			return wyFalse;

		keyvalue.Sprintf("%d", m_toppercent);
		ret = wyIni::IniWriteString(sectionstr.GetString(), "ULeftOrTopPercent", keyvalue.GetString(), dirstr.GetString());
		if(ret)
			return wyFalse;
	}
	else
	{
		sectionstr.SetAs(section);
		keyvalue.Sprintf("%d", m_rchsplitter.left);
		ret = wyIni::IniWriteString(sectionstr.GetString(), "Left", keyvalue.GetString(), dirstr.GetString());
		if(ret)
			return wyFalse;

		keyvalue.Sprintf("%d", m_rchsplitter.right);
		ret = wyIni::IniWriteString(sectionstr.GetString(), "Right", keyvalue.GetString(), dirstr.GetString());
		if(ret)
			return wyFalse;

		keyvalue.Sprintf("%d", m_rchsplitter.top);
		ret = wyIni::IniWriteString(sectionstr.GetString(), "Top", keyvalue.GetString(), dirstr.GetString());
		if(ret)
			return wyFalse;

		keyvalue.Sprintf("%d", m_rchsplitter.bottom);
		ret = wyIni::IniWriteString(sectionstr.GetString(), "Bottom", keyvalue.GetString(), dirstr.GetString());
		if(ret)
			return wyFalse;

		keyvalue.Sprintf("%d", m_leftpercent);
		ret = wyIni::IniWriteString(sectionstr.GetString(), "ULeftOrTopPercent", keyvalue.GetString(), dirstr.GetString());
		if(ret)
			return wyFalse;

	}
	return wyTrue;
}

// This function sets the splitter position to the values sent in the RECT pointer.
// This is required so that when the user closes the application we can keep the position of
// the splitters and when the user starts the aplpication again we can give him the same position.
wyBool
FrameWindow::SetSplitterPos(RECT *rect, wyUInt32 leftortoppercent)
	{
		memcpy(&m_rcvsplitter, rect, sizeof(RECT));
		m_leftpercent = leftortoppercent;

	return wyTrue;
}

// Function writes the file which is selected from latest files in the Main Menu.
// Opens it and puts the text into the edit box.
wyBool
FrameWindow::InsertFromLatestFile(wyInt32 id, MDIWindow * pcquerywnd)
{
	wyInt32		ret, lstyle;
	wyWChar     filename[MAX_PATH+1]={0};
    HMENU	    hmenu, hsubmenu;	
    wyBool      retval = wyFalse;
	wyString    recfilename,finalerrormessage;

	VERIFY(hmenu  = GetMenu(m_hwndmain));
	// First get the maximized state of the window,
	lstyle = GetWindowLongPtr(pcquerywnd->m_hwnd, GWL_STYLE);
	
	if((lstyle & WS_MAXIMIZE) && wyTheme::IsSysmenuEnabled(pcquerywnd->m_hwnd))
		VERIFY(hsubmenu = GetSubMenu(hmenu, 1));
	else
		VERIFY(hsubmenu = GetSubMenu(hmenu, 0));
	
	// Now get the menu string.
	VERIFY(ret = GetMenuString(hsubmenu, id, filename, MAX_PATH, MF_BYCOMMAND));

	if(id > ID_LF_1 && id <= ID_LF_1 + 10)
	{
		VERIFY(SetCursor(LoadCursor(NULL, IDC_WAIT)));
	
		recfilename.SetAs(filename+3); //to avoid recent index num, just incriment to +3
		
		retval = pcquerywnd->OpenFileinTab(&recfilename,&finalerrormessage, wyFalse, wyTrue);
		VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));
	}	

	if(finalerrormessage.GetLength())
	yog_message(NULL, finalerrormessage.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK | MB_TASKMODAL);

	return retval;
}

// Loads the recent sql file to editor.
// If the current tab is other thae Query editor, creates a new editor & loads the file 
wyBool
FrameWindow::InsertFromLatestSQLFile(wyInt32 id, MDIWindow * pcquerywnd, wyWChar *filename)
{
	wyInt32		tabimage, tosave;
	wyBool		retval;
	EditorBase	*peditorbase = NULL;
    MDIWindow*  wnd;
	HANDLE      hfile;
	
    wnd = GetActiveWin();
    
    // Gets the file handle to process
    hfile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);                     
	
    if(hfile == INVALID_HANDLE_VALUE)
	{
        DisplayErrorText(GetLastError(), _("Could not open file."), wnd ? wnd->m_hwnd : NULL);
		return wyFalse;
	}
	    
	tabimage = pcquerywnd->m_pctabmodule->GetActiveTabImage();
	
    if(tabimage == IDI_QUERYBUILDER_16 || tabimage == IDI_SCHEMADESIGNER_16)
		pcquerywnd->m_pctabmodule->CreateQueryEditorTab(pcquerywnd);

    peditorbase = pcquerywnd->GetActiveTabEditor()->m_peditorbase;

	// First check whether the existing file needs to be checked.
	if((peditorbase->m_edit == wyTrue && IsConfirmOnTabClose() == wyTrue) 
		|| ((peditorbase->m_save == wyTrue) && (peditorbase->m_edit == wyTrue)))
	{
		tosave = pcquerywnd->ConfirmSaveFile();
		switch(tosave)
		{
		case IDYES:
			if(!pcquerywnd->SaveFile())
				return wyFalse;
			break;

		case IDNO:
			break;

		default:
			return wyFalse;
		}
	}

	peditorbase->m_filename.SetAs(filename);
	retval = pcquerywnd->WriteSQLToEditor(hfile);
	//if(retval == wyTrue)
		//peditorbase->m_filename.SetAs(filename);

	peditorbase->m_edit = wyFalse;
	peditorbase->m_save = wyTrue;
	pcquerywnd->SetQueryWindowTitle();

	return wyTrue;
}

// Loads the recent Schema Design. 
// If the current tab is other thae Schema Designer, creates a new Schema Designer & loads the file 
wyBool	
FrameWindow::InsertFromLatestSchemaDesignFile(wyInt32 id, MDIWindow * pcquerywnd, wyWChar *filename)
{
#ifndef COMMUNITY

	wyInt32				tabimage, idmsg;
	wyString			fname, jobbuff;
	wyBool				issdactve = wyFalse, ret;
	TabSchemaDesigner	*ptabsd = NULL, tabsd(NULL);
	TiXmlDocument		*doc;
		
	// Gets the white space
	TiXmlBase::SetCondenseWhiteSpace(false);

	doc = new TiXmlDocument();
	fname.SetAs(filename);	

    //Handles the .schemadesign file
	ret = ptabsd->HandleSchemaDesignfile(fname.GetAsWideChar(), &jobbuff);
	
    if(ret == wyFalse)
    {
        delete doc;
		return ret;
    }
    		
	tabimage = pcquerywnd->m_pctabmodule->GetActiveTabImage();
	
    if(tabimage != IDI_SCHEMADESIGNER_16)
		pcquerywnd->m_pctabmodule->CreateSchemaDesigner(pcquerywnd);

	else
		issdactve = wyTrue;
		
	ptabsd = (TabSchemaDesigner*)pcquerywnd->m_pctabmodule->GetActiveTabType();

    //handle if this is an active SD
	if(issdactve == wyTrue)
	{
		idmsg = ptabsd->OnTabClosing(wyFalse);
		
        if(!idmsg)
			return wyFalse;

        //clear SD
        ptabsd->ClearSD();
	}

    pcquerywnd->SetQueryWindowTitle();
	doc->Parse(jobbuff.GetString());
	ptabsd->CanvasLoadXML(doc, &fname);
    delete doc;
#endif

	return wyTrue;    
}

//the function handles the inserting from latest QB files
wyBool
FrameWindow::InsertFromLatestQBFile(wyInt32 id, MDIWindow * pcquerywnd, wyWChar *filename)
{
#ifndef COMMUNITY
    wyInt32				tabimage, idmsg;
	wyString			fname, jobbuff;
	wyBool				isqbactve = wyFalse, ret;
    TabQueryBuilder     *ptabqb = NULL, tabqb(NULL);
	TiXmlDocument		*doc;

    // Gets the white space
	TiXmlBase::SetCondenseWhiteSpace(false);

	doc = new TiXmlDocument();
	fname.SetAs(filename);	

    //Handles the qb file
    ret = tabqb.HandleQueryBuilderfile(fname.GetAsWideChar(), &jobbuff);

    if(ret == wyFalse)
    {
        delete doc;
		return ret;
    }

	tabimage = pcquerywnd->m_pctabmodule->GetActiveTabImage();

    if(tabimage != IDI_QUERYBUILDER_16)
		pcquerywnd->m_pctabmodule->CreateQueryBuilderTab(pcquerywnd);
	else
		isqbactve = wyTrue;
		
	ptabqb = (TabQueryBuilder*)pcquerywnd->m_pctabmodule->GetActiveTabType();

	if(isqbactve == wyTrue)
	{
        idmsg = ptabqb->OnTabClosing(wyFalse);

        if(!idmsg)
			return wyFalse;

        ptabqb->ClearQueryBuilder();
	}

	pcquerywnd->SetQueryWindowTitle();
	doc->Parse(jobbuff.GetString());
    
    //set the m_isautomated flag, so that the file loading routine can use the existing QB functions to generate the query
	ptabqb->m_isautomated = wyTrue;
    ptabqb->LoadQueryXML(doc, &fname);
    ptabqb->m_isautomated = wyFalse;
    delete doc;
#endif
    return wyTrue;
}

//Shorcut for switching between Form & Grid
void					
FrameWindow::AccelaratorSwitchGridForm(MDIWindow *wnd, TabMgmt *ptabmgmt)
{
}

// This function is called whenever a QueryWindow is activated, deactivaed, closed or a new one opened.
// It checks the current status of various window and enables and disables verious menu item.
wyBool
FrameWindow::OnActiveConn()
{
    wyInt32     itemcount, size;
	HMENU       hmenu;

	wyInt32		menugrayitems[] = { IDM_FILE_NEWSAMECONN, IDM_FILE_CLOSECONNECTION, ID_NEW_EDITOR, ID_FILE_RENAMETAB,
								ID_FILE_CLOSETAB, IDM_FILE_CLOSECONNECTION, IDM_FILE_CLOASEALL, 
								IDM_FILE_OPENSQL, IDM_FILE_OPENSQLNEW, 
								IDM_FILE_SAVESQL, IDM_FILE_SAVEAS,
								IDM_EDIT_UNDO, IDM_EDIT_REDO, IDM_EDIT_CUT, IDM_EDIT_COPY, IDM_COPYNORMALIZED,
                                IDM_EDIT_PASTE, IDM_EDIT_SELECTALL, IDM_EDIT_FIND, IDM_EDIT_FINDNEXT, 
								IDM_EDIT_REPLACE, IDM_EDIT_GOTO, IDM_REFRESHOBJECT, IDM_COLLAPSEOBJECT, IDM_OBCOLOR , ID_EDIT_INSERTTEMPLATES,
								IDM_EDIT_RESULT_TEXT, IDC_EDIT_SHOWOBJECT, IDC_EDIT_SHOWRESULT, 
								IDC_EDIT_SHOWEDIT, IDM_EDIT_ADVANCED_UPPER, IDM_EDIT_ADVANCED_LOWER,
								IDM_EDIT_ADVANCED_COMMENT, IDM_EDIT_ADVANCED_REMOVE, IDM_DB_REFRESHOBJECT, ID_DATASEARCH,
								IDM_EDIT_MANPF, IDM_EDIT_ADDSQL, IDM_TOOLS_TABLEDIAG, ID_HISTORY, ID_INFOTAB,
								ID_SHOW_STATUS, ID_SHOW_VARIABLES, ID_SHOW_PROCESSLIST, ID_TOOLS_USERMANAGER, ID_IMPORTEXPORT_DBEXPORTTABLES, 
								IDM_WINDOW_CASCADE, IDM_WINDOW_TILE, IDM_WINDOWS_ICONARRANGE, ID_REFRESHFAVORITES, ID_OBJECT_SELECTSTMT,
								ID_REBUILDTAGS, ID_ORGANIZEFAVORITES, IDM_IMEX_EXPORTDATA, ID_IMPORTEXPORT_EXPORTTABLES,ID_IMEX_TEXTFILE,
								ID_OBJECT_MANINDEX, ID_IMPORTEXPORT_TABLESEXPORTTABLES, ID_OBJECT_MAINMANINDEX, IDM_TABLE_RELATION,
								ID_IMPORTEXPORT_DBEXPORTTABLES2, ID_IMEX_TEXTFILE2, 
                                ID_EXPORT_EXPORTTABLEDATA, ID_OBJECT_EXPORTVIEW,
								ID_EXPORT_AS, ID_EXPORT_ASXML, ID_EXPORT_ASHTML, ID_OBJECT_VIEWDATA, ID_TABLE_OPENINNEWTAB, ID_OBJECT_ADVANCED,
								ID_OBJECT_CREATEVIEW, ID_DB_CREATEVIEW, ID_OBJECT_CREATESTOREDPROCEDURE, ID_DB_CREATESTOREDPROCEDURE,
								ID_OBJECT_CREATEFUNCTION,ID_DB_CREATEFUNCTION, ID_OBJECT_CREATEEVENT, ID_DB_CREATEEVENT,
								ID_OBJECT_CREATETRIGGER,ID_DB_CREATETRIGGER, ID_OBJECT_ALTERVIEW, ID_OBJECT_ALTERSTOREDPROCEDURE,
								ID_OBJECT_ALTERFUNCTION, ID_OBJECT_ALTERTRIGGER, ID_OBJECT_DROPVIEW,
								ID_OBJECT_DROPSTOREDPROCEDURE,ID_OBJECT_DROPFUNCTION, ID_OBJECT_DROPTRIGGER,
								ID_OBJECTS_RENAMEVIEW, ID_OBJECTS_RENAMETRIGGER,
								ID_TOOLS_FLUSH, IDC_DIFFTOOL, 
								IDM_CREATEDATABASE,ID_OBJECT_TRUNCATEDATABASE, ID_DB_TABLE_MAKER, ID_TABLE_MAKER, IDM_ALTERDATABASE,ID_OPEN_COPYDATABASE, 
								ID_OBJECT_DROPDATABASE, ID_OBJECT_EMPTYDATABASE, ID_OBJECT_CREATESCHEMA,/*ID_DATABASE_REBUILDTAGS, vgladcode*/ 
								ID_OBJECT_TABLEEDITOR,  
								ID_OBJECT_COPYTABLE, ID_IMPORT_FROMCSV,ID_IMPORT_FROMXML, ID_OBJECT_RENAMETABLE, ID_OBJECT_CLEARTABLE, 
								ID_OBJECT_DROPTABLE, ID_OBJECT_REORDER, ID_OBJECT_CHANGETABLETYPE_ISAM, 
								ID_OBJECT_CHANGETABLETYPE_MYISAM, ID_OBJECT_CHANGETABLETYPE_HEAP, 
								ID_OBJECT_CHANGETABLETYPE_MERGE, ID_OBJECT_CHANGETABLETYPE_INNODB, ID_OBJECT_CHANGETABLETYPE_BDB, 
								ID_OBJECT_CHANGETABLETYPE_GEMINI,
								ID_OBJECT_INSERTSTMT, ID_OBJECT_UPDATESTMT,ID_OBJECT_DELETESTMT, 
								ID_OBJECT_DROPFIELD, ID_COLUMNS_DROPINDEX, ID_OPEN_COPYTABLE, 
								ID_TRANSACTION_SETAUTOCOMMIT, ID_TRX_REPEATABLEREAD, ID_TRX_READCOMMITED, ID_TRX_READUNCOMMITED,
								ID_TRX_SERIALIZABLE, ID_WITHCONSISTENTSNAPSHOT_READONLY,
								ID_WITHCONSISTENTSNAPSHOT_READWRITE, ID_STARTTRANSACTION_READONLY, ID_STARTTRANSACTION_READWRITE,
								ID_COMMIT_ANDCHAIN, ID_COMMIT_ANDNOCHAIN, ID_COMMIT_RELEASE,
								ID_COMMIT_NORELEASE, ID_ROLLBACK_TOSAVEPOINT, ID_ROLLBACK_ANDCHAIN,
								ID_ROLLBACK_ANDNOCHAIN, ID_ROLLBACK_RELEASE, ID_ROLLBACK_NORELEASE, ID_SAVEPOINT_CREATESAVEPOINT,
								ID_SAVEPOINT_RELEASESAVEPOINT};

	if(pGlobals->m_conncount == 0)
	{
		VERIFY(hmenu = GetMenu(m_hwndmain));

		size = sizeof(menugrayitems)/ sizeof(menugrayitems[0]);

		for(itemcount = 0; itemcount < size; itemcount++)
			EnableMenuItem(hmenu, menugrayitems[itemcount], MF_GRAYED | MF_BYCOMMAND);
						
		// grey the latest files.
		for(itemcount = 0; itemcount <= MAX_RECENT_FILES; itemcount++)
			EnableMenuItem(hmenu, ID_LF_1 + itemcount, MF_GRAYED);
				
		EnableMenuItem(hmenu, ID_QUERYBUILDER, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_SCHEMADESIGNER, MF_GRAYED | MF_BYCOMMAND);

		EnableMenuItem(hmenu, ID_STARTTRANSACTION_WITHNOMODIFIER, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_COMMIT_WITHNOMODIFIER, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_ROLLBACK_TRANSACTION, MF_GRAYED | MF_BYCOMMAND);

		//disable format query options
		EnableMenuItem(hmenu, ACCEL_FORMATALLQUERIES, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ACCEL_FORMATCURRENTQUERY, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ACCEL_FORMATSELECTEDQUERY, MF_GRAYED | MF_BYCOMMAND);

		//EnableToolButtonsAndCombo(m_hwndtool, m_hwndsecondtool, m_hwndtoolcombo, wyFalse);
        PostMessage(m_hwndmain, UM_UPDATEMAINTOOLBAR, 0, 0);
						
		// change the status bar text also.
		AddTextInStatusBar(CONNECT_MYSQL_MSG);
        m_statusbarmgmt->ShowInformation(L"", 3);
        m_statusbarmgmt->ShowInformation(L"", 2);
        m_statusbarmgmt->ShowInformation(L"", 4);
        m_statusbarmgmt->ShowInformation(L"", 5);

		//do not display execution time when connection is closed
        m_statusbarmgmt->ShowInformation(L"", 1);
		//grey session save and save as
		EnableMenuItem(hmenu, ID_SAVESESSION, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem(hmenu, ID_SAVESESSIONAS, MF_GRAYED | MF_BYCOMMAND);
		if(!m_sessionfile.GetLength())
			EnableMenuItem(hmenu, ID_CLOSESESSION, MF_GRAYED | MF_BYCOMMAND);
		else
			EnableMenuItem(hmenu, ID_CLOSESESSION, MF_ENABLED);
	}
	else
	{
		hmenu = GetMenu(m_hwndmain);

		for(itemcount = 0; itemcount < (sizeof(menugrayitems)/sizeof(menugrayitems[0])); itemcount++)
			EnableMenuItem(hmenu, menugrayitems[itemcount], MF_ENABLED);

		// enable the latest files.
		for(itemcount=0; itemcount <= MAX_RECENT_FILES; itemcount++)
			EnableMenuItem(hmenu, ID_LF_1 + itemcount, MF_ENABLED);

		for(itemcount=0; itemcount <= MAX_RECENT_FILES; itemcount++)
			EnableMenuItem(hmenu, ID_SDLF_1 + itemcount, MF_ENABLED);

		EnableMenuItem(hmenu, ID_SAVESESSION, MF_ENABLED);
		EnableMenuItem(hmenu, ID_SAVESESSIONAS, MF_ENABLED);
		if(!m_sessionfile.GetLength())
			EnableMenuItem(hmenu, ID_CLOSESESSION, MF_GRAYED | MF_BYCOMMAND);
		else
			EnableMenuItem(hmenu, ID_CLOSESESSION, MF_ENABLED);
		//EnableToolButtonsAndCombo(m_hwndtool, m_hwndsecondtool, m_hwndtoolcombo, wyTrue);
        PostMessage(m_hwndmain, UM_UPDATEMAINTOOLBAR, 0, 0);
	}

    if(!m_languagecount)
    {
        EnableMenuItem(hmenu, ID_TOOLS_CHANGELANGUAGE, MF_GRAYED | MF_BYCOMMAND);
    }
    else
    {
        EnableMenuItem(hmenu, ID_TOOLS_CHANGELANGUAGE, MF_ENABLED);
    }
	
	return wyTrue;
}

/* helper function to stop a query */
wyBool
FrameWindow::StopQuery(HWND hwndactive, MDIWindow * wnd)
{
#ifndef COMMUNITY
	wyInt32		presult = 6;
	
	if(pGlobals->m_entlicense.CompareI("Professional") != 0 && wnd->m_ptransaction && !wnd->m_ptransaction->m_starttransactionenabled)
	{
		presult = MessageBox(wnd->m_pcqueryobject->m_hwnd, _(L"The session has an active transaction. Stopping the execution will cause transaction to rollback and end. Do you want to continue?"), 
                    _(L"Warning"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
		if(presult == 6)
			wnd->m_ptransaction->CallOnCommit();
	}
		if(presult != 6) //dosent matter for professional, as initial value of presult is 6
		{
			return wyFalse;
		}
#endif
    VERIFY(SetCursor(LoadCursor(NULL, IDC_WAIT)));

	if(hwndactive && wnd->m_executing && wnd->m_tunnel->IsTunnel())
    {
        EnterCriticalSection(&wnd->m_cs);
		/* just need to set the m_stopquery flag to true */
		wnd->StopQueryExecution();
        LeaveCriticalSection(&wnd->m_cs);
    }
    else if(hwndactive && wnd->m_executing)
    { 
		// try to stop the query and if successful then enable the correct the toolbar buttons 
		if(wnd->StopQuery())
        {
			wnd->EnableToolOnNoQuery();
            wnd->OnInitDialog();
			//PostMessage(wnd->m_hwnd, WM_INITDLGVALUES, 0, 0);
			Sleep(100);
		}
        else
        {
            VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));
            return wyFalse;
        }
	}
    VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));
	return wyTrue;
}

wyBool
FrameWindow::HandleCheckMenu(MDIWindow * pcquerywnd, wyBool ischecked, wyUInt32 menuid)
{
    long        lstyle;		
	HMENU       hmenu, hsubmenu;

	VERIFY(hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain));
	
	lstyle = GetWindowLongPtr(pcquerywnd->m_hwnd, GWL_STYLE);

	if((lstyle & WS_MAXIMIZE) && wyTheme::IsSysmenuEnabled(pcquerywnd->m_hwnd))
		VERIFY(hsubmenu = GetSubMenu(hmenu, 2));
	else
		VERIFY(hsubmenu = GetSubMenu(hmenu, 1));
	
	lstyle = (ischecked == wyFalse)?(MF_UNCHECKED):(MF_CHECKED);

	CheckMenuItem(hsubmenu, menuid, MF_BYCOMMAND | lstyle);

	return wyTrue;

}

// Function to toggle result in text option. The function is called whenever the user clicks on it.
wyBool
FrameWindow::CheckOrUncheckTextResult(MDIWindow * pcquerywnd)
{    
	/*if(!pcquerywnd->GetActiveTabEditor())		
        return wyFalse;

	if(pcquerywnd->GetActiveTabEditor()->m_pctabmgmt->GetSelectedItem() == IDI_TABLE)
	{
		if(pcquerywnd->GetActiveTabEditor()->m_resultviewtype == GRID_VIEW)
		{
			if(!pGlobals->m_entlicense.CompareI("Professional"))
			{
				pcquerywnd->GetActiveTabEditor()->m_resultviewtype = TEXT_VIEW;		
			}

			else
			{
				pcquerywnd->GetActiveTabEditor()->m_resultviewtype = FORM_VIEW;		
			}
		
		}
		else if(pcquerywnd->GetActiveTabEditor()->m_resultviewtype == FORM_VIEW)
		{
			pcquerywnd->GetActiveTabEditor()->m_resultviewtype = TEXT_VIEW;
		}
		else if(pcquerywnd->GetActiveTabEditor()->m_resultviewtype == TEXT_VIEW)
		{
			pcquerywnd->GetActiveTabEditor()->m_resultviewtype = GRID_VIEW;
		}
	}	

	if(pcquerywnd->GetActiveTabEditor()->m_pctabmgmt->GetSelectedItem() == IDI_QUERYRECORD)
	{
		if(pcquerywnd->GetActiveTabEditor()->m_resultviewtype == GRID_VIEW)
		{
			pcquerywnd->GetActiveTabEditor()->m_resultviewtype = TEXT_VIEW;
		}
		else if(pcquerywnd->GetActiveTabEditor()->m_resultviewtype == FORM_VIEW)
		{
			pcquerywnd->GetActiveTabEditor()->m_resultviewtype = TEXT_VIEW;
		}
		else
		{
			pcquerywnd->GetActiveTabEditor()->m_resultviewtype = GRID_VIEW;
		}
	}

		
	pcquerywnd->GetActiveTabEditor()->m_istextresult = (pcquerywnd->GetActiveTabEditor()->m_istextresult == wyTrue)?wyFalse:wyTrue;
	
	HandleCheckMenu(pcquerywnd, pcquerywnd->GetActiveTabEditor()->m_istextresult, IDM_EDIT_RESULT_TEXT);*/

	if(!pcquerywnd->GetActiveTabEditor() && pcquerywnd->m_pctabmodule->GetActiveTabImage() != IDI_DATASEARCH)		
    {
		return wyFalse;
	}
	if(pcquerywnd->m_pctabmodule->GetActiveTabImage() == IDI_DATASEARCH)
	{
		HandleCheckMenu(pcquerywnd,  pcquerywnd->m_pctabmodule->GetActiveTabType()->m_istextresult == wyTrue?wyFalse:wyTrue, IDM_EDIT_RESULT_TEXT);
	}
	else
	{
		pcquerywnd->GetActiveTabEditor()->m_istextresult = (pcquerywnd->GetActiveTabEditor()->m_istextresult == wyTrue)?wyFalse:wyTrue;
		HandleCheckMenu(pcquerywnd, pcquerywnd->GetActiveTabEditor()->m_istextresult, IDM_EDIT_RESULT_TEXT);
	}
	

	return wyTrue;
}

wyBool
FrameWindow::CheckOrUncheckObjBrowserVis(MDIWindow * pcquerywnd)
{
	HandleCheckMenu(pcquerywnd, pcquerywnd->m_isobjbrowvis, IDC_EDIT_SHOWOBJECT);
    pcquerywnd->m_isobjbrowvis = (pcquerywnd->m_isobjbrowvis == wyTrue)?wyFalse:wyTrue;

	return wyTrue;
}

wyBool
FrameWindow::CheckOrUncheckResultWindowVis(MDIWindow * pcquerywnd)
{
	TabTypes	*ptabeditor;

	if(pcquerywnd->GetActiveTabEditor() == NULL)
        return wyFalse;

	ptabeditor =  pcquerywnd->GetActiveTabEditor();

	if(ptabeditor->m_iseditwnd == wyFalse)
		return wyFalse;

	HandleCheckMenu(pcquerywnd, ptabeditor->m_isresultwnd, IDC_EDIT_SHOWRESULT);  
    ptabeditor->m_isresultwnd = (ptabeditor->m_isresultwnd == wyTrue)?wyFalse:wyTrue;

	return wyTrue;
}

wyBool 
FrameWindow::CheckOrUncheckEditWindowVis(MDIWindow *pcquerywnd)
{
	TabTypes	*ptabeditor;

	if(pcquerywnd->GetActiveTabEditor() == NULL)   
        return wyFalse;

	ptabeditor =  pcquerywnd->GetActiveTabEditor();

    if(ptabeditor->m_isresultwnd == wyFalse)
		return wyFalse;

    HandleCheckMenu(pcquerywnd, ptabeditor->m_iseditwnd, IDC_EDIT_SHOWEDIT);
	ptabeditor->m_iseditwnd = (ptabeditor->m_iseditwnd == wyTrue)?wyFalse:wyTrue;

	return wyTrue;
}

// Function returns TRUE if result in text is checked. else it return wyFalse.
// Required to know whether to show result in text or grid mode.

wyBool
FrameWindow::IsTextResult()
{
	wyUInt32    menustate;
	HMENU       hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain);

	menustate = GetMenuState(hmenu, IDM_EDIT_RESULT_TEXT, MF_BYCOMMAND);

	if(!(menustate & MF_CHECKED))
		return wyFalse;
	
    return wyTrue;
}


// Callback function for the create database dialog box.
INT_PTR CALLBACK
FrameWindow::CreateObjectDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	wyChar *object = (wyChar *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
	    SetWindowLongPtr(hwnd, GWLP_USERDATA,lParam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
        return 1;
        
	case WM_CTLCOLORSTATIC:
		{
		 if(GetDlgCtrlID((HWND)lParam) == IDC_TEXTFILTERDB)
		 {
			 SetTextColor((HDC)wParam, RGB(255, 0, 0));
			 SetBkMode((HDC)wParam, TRANSPARENT);

			 return(wyInt32)GetStockObject(HOLLOW_BRUSH);
		 }
		}

		break;

	case WM_INITDLGVALUES:
		pGlobals->m_pcmainwin->InitCreateObjectDlg(hwnd, object);
		break;

	case WM_HELP:
		pGlobals->m_pcmainwin->OnCreateObjectWmHelp(hwnd, object);
		return 1;

	case WM_COMMAND:
        pGlobals->m_pcmainwin->OnCreateObjectWmCommand(hwnd, wParam, object);
		break;
	}

	return 0;
}


/* Function used to Enable or Disable the buttons on the toolbar */
void 
FrameWindow::HandleFirstToolBar()
{
	HWND		hwnd;
    wyInt32     nid[] = { IDM_EXECUTE, ACCEL_QUERYUPDATE, ACCEL_EXECUTESEL};
	MDIWindow   *pcquerywnd;
    EditorBase  *pceditorbase;;
    wyInt32     size = sizeof(nid)/ sizeof(nid[0]), count;
	
	pcquerywnd = GetActiveWin(); 	

	if(!pcquerywnd)
		return;
	hwnd = GetFocus();
	
	//If active tab is QB disable tool bar
    if(pcquerywnd->GetActiveTabEditor() == NULL)
       return;
       
    pceditorbase = pcquerywnd->GetActiveTabEditor()->m_peditorbase;     

    SendMessage(m_hwndtool, TB_SETSTATE,(WPARAM)ACCEL_EXECUTEALL, TBSTATE_ENABLED);

    if(pceditorbase && pceditorbase->GetAdvancedEditor() == wyFalse)
    {
        for(count = 0; count < size; count++)
    	    SendMessage(m_hwndtool, TB_SETSTATE,(WPARAM)nid[count], TBSTATE_ENABLED);
    }
	#ifndef COMMUNITY
	if(pcquerywnd->m_conninfo.m_isreadonly == wyTrue)
	{
		SendMessage(m_hwndtool, TB_SETSTATE,ACCEL_QUERYUPDATE, TBSTATE_INDETERMINATE);
	}
#endif
}

void 
FrameWindow::InitCreateObjectDlg(HWND hwnd, wyChar *object)
{  
	MDIWindow	*wnd;

   	if(stricmp(object, "CreateView")== 0 )
	{
		SetWindowText(hwnd, L"Create View");
		SendMessage(GetDlgItem(hwnd, IDC_CAPTION), WM_SETTEXT, 0, (LPARAM)_(L"Enter new view name"));
	}
	else if(stricmp(object, "CreateProcedure")== 0 )
	{
		SetWindowText(hwnd, L"Create Procedure");
		SendMessage(GetDlgItem(hwnd, IDC_CAPTION), WM_SETTEXT, 0, (LPARAM)_(L"Enter new procedure name"));
	}
	else if(stricmp(object, "CreateFunction")== 0 )
	{
		SetWindowText(hwnd, L"Create Function");
		SendMessage(GetDlgItem(hwnd, IDC_CAPTION), WM_SETTEXT, 0, (LPARAM)_(L"Enter new function name"));
	}
	else if(stricmp(object, "CreateEvent")== 0 )
	{
		SetWindowText(hwnd, L"Create Event");
		SendMessage(GetDlgItem(hwnd, IDC_CAPTION), WM_SETTEXT, 0, (LPARAM)_(L"Enter new event name"));
	}
	else if(stricmp(object, "CreateTrigger")== 0 )
	{
		SetWindowText(hwnd, L"Create Trigger");
		SendMessage(GetDlgItem(hwnd, IDC_CAPTION), WM_SETTEXT, 0, (LPARAM)_(L"Enter new trigger name"));
	}
	else if(stricmp(object, "GoToLine")== 0 )
	{
		SetWindowText(hwnd, L"Go To");
		SendMessage(GetDlgItem(hwnd, IDC_CAPTION), WM_SETTEXT, 0, (LPARAM)_(L"Enter line number"));
		SendMessage(GetDlgItem(hwnd, IDOK), WM_SETTEXT, 0, (LPARAM)_(L"&Go To"));
		SetWindowLongPtr(GetDlgItem(hwnd, IDC_DBNAME), GWL_STYLE, GetWindowLongPtr(GetDlgItem(hwnd, IDC_DBNAME), GWL_STYLE) | ES_NUMBER);
	}
    else if(stricmp(object, "createdb") == 0)
    {
       SendMessage(GetDlgItem(hwnd, IDC_DBEDIT), EM_LIMITTEXT, 63, 0);
		if(GetActiveWin()->m_ismysql41 == wyFalse)
        {
            EnableWindow((GetDlgItem(hwnd, IDC_CHARSETCOMBO)), FALSE);
            EnableWindow((GetDlgItem(hwnd, IDC_COLLATECOMBO)), FALSE);
        }
        else
        {
            InitCharacterSetCombo(hwnd);
            InitCollationCombo(hwnd);
        }

		VERIFY(wnd = GetActiveWin());
		if(wnd && wnd->m_conninfo.m_db.GetLength())
		{
			ShowWindow(GetDlgItem(hwnd, IDC_TEXTFILTERDB), SW_SHOW);
		}
    }
	else if(stricmp(object, "AlterDB") == 0)
	{  	
		EnableWindow((GetDlgItem(hwnd, IDC_DBEDIT)), FALSE);
		SetWindowText(hwnd, _(L"Alter Database"));
		SendMessage(GetDlgItem(hwnd, IDOK), WM_SETTEXT, 0, (LPARAM)_(L"&Alter"));
		InitCharsetAndCollationInfo(hwnd);
	}

    //disable the OK button initially
    //EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
}
void 
FrameWindow::InitCharsetAndCollationInfo(HWND hwnd)
{  
	wyString        query, dbcollation, dbname, dbcharset;
	MYSQL_RES		*res;
	MYSQL_ROW		row;

	MDIWindow *wnd = GetActiveWin();
    HWND hwndCharsetCombo = GetDlgItem(hwnd, IDC_CHARSETCOMBO);
    HWND hwndCollateCombo = GetDlgItem(hwnd, IDC_COLLATECOMBO);
	
	SendMessage(GetDlgItem(hwnd, IDC_DBEDIT), WM_SETTEXT, 0, (LPARAM)wnd->m_pcqueryobject->m_seldatabase.GetAsWideChar());
    
	InitCharacterSetCombo(hwnd);
	query.Sprintf("show variables like 'character_set_database'");
	res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);
	if(!res)
	{
        ShowMySQLError(hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return;
	}
	
    while(row = wnd->m_tunnel->mysql_fetch_row(res))
	{ 
		dbcharset.SetAs(row[1]);
	   	SendMessage(hwndCharsetCombo, CB_SELECTSTRING, -1, (LPARAM)dbcharset.GetAsWideChar());
	}

    wnd->m_pcqueryobject->m_dbcharset.SetAs(dbcharset.GetAsWideChar());
	ReInitRelatedCollations(hwnd);
	query.Sprintf("show variables like 'collation_database'");
	res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);
	if(!res)
	{
        ShowMySQLError(hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return;
	}
	while(row = wnd->m_tunnel->mysql_fetch_row(res))
	{  
		dbcollation.SetAs(row[1]);
	   	SendMessage(hwndCollateCombo, CB_SELECTSTRING, -1, (LPARAM)dbcollation.GetAsWideChar());
	}
	SetFocus(hwndCharsetCombo);

    if(res)
    {
        wnd->m_tunnel->mysql_free_result(res);
    }
}
    
void
FrameWindow::InitCollationCombo(HWND hwnd)
{
    MDIWindow   *wnd = GetActiveWin();
    wyString    query, collationstr;
    MYSQL_ROW   myrow;
    MYSQL_RES   *myres;
    wyInt32     index;
    
    HWND        hwndcombo = GetDlgItem(hwnd, IDC_COLLATECOMBO);

    query.SetAs("show collation");
    myres = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);
    if(!myres)
	{
        ShowMySQLError(hwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return;
	}
    while(myrow = wnd->m_tunnel->mysql_fetch_row(myres))
    {
        collationstr.SetAs(myrow[0]);
        SendMessage(hwndcombo , CB_ADDSTRING, 0,(LPARAM)collationstr.GetAsWideChar());
    }
    if((index = SendMessage(hwndcombo , CB_ADDSTRING, 0,(LPARAM)TEXT(STR_DEFAULT))) != CB_ERR)
        SendMessage(hwndcombo, CB_SELECTSTRING, index, (LPARAM)TEXT(STR_DEFAULT));
	if(myres)
	{
		wnd->m_tunnel->mysql_free_result(myres);
	}
}

wyBool 
FrameWindow::HandleCreateDatabase(HWND hwnd, MDIWindow	*pcquerywnd, wyWChar *dbname)
{
	wyInt32     len;
	HWND		hwndedit;
	MYSQL_RES	*res;
    wyString    query;
	wyString	dbnamestr, tempcharset;
	MDIWindow	*wnd = NULL;
	wyInt32		isintransaction = 1;

	VERIFY(wnd = GetActiveWin());
	if(!wnd)
		return wyFalse;

    if(!dbname)
		return wyFalse;

	FetchSelectedCharset(hwnd, &tempcharset, wyTrue);

	if(tempcharset.GetLength())
		GetDBCharset(&tempcharset);
   
	// get the db name.
	VERIFY(hwndedit = GetDlgItem(hwnd, IDC_DBEDIT));

	VERIFY(len = SendMessage(hwndedit, WM_GETTEXT, SIZE_64, (LPARAM)dbname));
	
    if(IsFieldBlank(hwndedit) != 1)
    {
		yog_message(hwndedit, _(L"Please provide a valid database name"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR | MB_HELP);
		SetFocus(hwndedit);
		return wyFalse;
	}

	dbnamestr.SetAs(dbname); 

	//to trim empty spaces from right, to avoid mysql errors
	dbnamestr.RTrim();

	query.Sprintf("create database `%s`", dbnamestr.GetString());
    
    if(pcquerywnd->m_pcqueryobject->m_dbcharset.GetLength() != 0 && 
       pcquerywnd->m_pcqueryobject->m_dbcharset.CompareI(STR_DEFAULT) != 0)
    {
        query.AddSprintf("character set %s ", pcquerywnd->m_pcqueryobject->m_dbcharset.GetString());
    }
    if(pcquerywnd->m_pcqueryobject->m_dbcollation.GetLength() != 0 && 
       pcquerywnd->m_pcqueryobject->m_dbcollation.CompareI(STR_DEFAULT) != 0)
    {
        query.AddSprintf("collate %s", pcquerywnd->m_pcqueryobject->m_dbcollation.GetString());
    }

    res = ExecuteAndGetResult(pcquerywnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse,
								0, wyFalse, &isintransaction, hwnd);
	
	if(isintransaction == 1)
		return wyFalse;
	
	if(!res && pcquerywnd->m_tunnel->mysql_affected_rows(pcquerywnd->m_mysql)== -1)
	{
		ShowMySQLError(hwnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query.GetString());
		return wyFalse;
	}
	
	pcquerywnd->m_tunnel->mysql_free_result(res);

	///New db select and insert into db combo only if database(s) not specified in Con.window
    if(!wnd->m_filterdb.GetLength())
	{
        HTREEITEM hti = TreeView_GetRoot(pcquerywnd->m_pcqueryobject->m_hwnd);
        TVITEM    tvi;

        tvi.mask = TVIF_PARAM;
        tvi.hItem = hti;
        TreeView_GetItem(pcquerywnd->m_pcqueryobject->m_hwnd, &tvi);
        if(tvi.lParam && /*!wcscmp(((OBDltElementParam *)tvi.lParam)->m_filterText,L"") &&*/ wcsstr(dbname, ((OBDltElementParam *)tvi.lParam)->m_filterText))
        {
	        query.Sprintf("use `%s`", dbnamestr.GetString());
            res = ExecuteAndGetResult(pcquerywnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query);
	        if(!res && pcquerywnd->m_tunnel->mysql_affected_rows(pcquerywnd->m_mysql)== -1)
	        {
		        ShowMySQLError(hwnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query.GetString());
		        return wyFalse;
	        }

	        pcquerywnd->m_tunnel->mysql_free_result(res);
	        /* Show the selected dbname in combo */
	        pGlobals->m_pcmainwin->AddTextInCombo(dbname);
        }
	}

	return wyTrue;
}

void
FrameWindow::GetDBCharset(wyString *charset)
{
	MDIWindow *wnd = NULL;

	if(!charset)
		return;

	VERIFY(wnd = GetActiveWin());

	wnd->m_pcqueryobject->m_dbcharset.SetAs(*charset);
}

wyBool
FrameWindow::HandleAlterDatabase(HWND hwnd, MDIWindow *pcquerywnd, wyWChar *dbname)
{	
	HWND		hwndedit;
	MYSQL_RES	*res;
    wyString    query;
	wyString	dbnamestr, tempcharset;
	wyInt32 isintransaction = 1;

	hwndedit = GetDlgItem(hwnd, IDC_DBEDIT);
	CQueryObject *pcqueryobject=pcquerywnd->m_pcqueryobject;
	SendMessage(hwndedit, WM_GETTEXT, SIZE_64, (LPARAM)dbname);

	FetchSelectedCharset(hwnd, &tempcharset, wyTrue);
	if(tempcharset.GetLength())
		GetDBCharset(&tempcharset);

	FetchSelectedCollation(hwnd);

	dbnamestr.SetAs(dbname); 

	query.Sprintf("alter database `%s` ", dbnamestr.GetString());
    
    if(pcqueryobject->m_dbcharset.GetLength() != 0 && 
       pcqueryobject->m_dbcharset.CompareI(STR_DEFAULT) != 0)
    {
        query.AddSprintf("character set %s ", pcqueryobject->m_dbcharset.GetString());
    }
    if(pcqueryobject->m_dbcollation.GetLength() != 0 && 
       pcqueryobject->m_dbcollation.CompareI(STR_DEFAULT) != 0)
    {
        query.AddSprintf("collate %s", pcqueryobject->m_dbcollation.GetString());
    } 

    res = ExecuteAndGetResult(pcquerywnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse,
								0, wyFalse, &isintransaction, GetActiveWindow());
	if(isintransaction == 1)
		return wyFalse;

	if(!res && pcquerywnd->m_tunnel->mysql_affected_rows(pcquerywnd->m_mysql) == -1)
	{
		ShowMySQLError(hwnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query.GetString());
		return wyFalse;
	}
	
	AddTextInCombo(dbname);
	yog_message(hwnd, _(L"Database altered successfully"), pGlobals->m_appname.GetAsWideChar(), MB_OK|MB_ICONINFORMATION);
    return wyTrue;

}

wyBool 
FrameWindow::HandleCreateView(HWND hwnd, MDIWindow	*pcquerywnd, wyWChar *viewname)
{
	wyInt32		len;
	HWND		hwndedit;
	wyString    query, msg, db;
	MYSQL_RES	*res;
	wyString	viewnamestr;

	if(!viewname)
		return wyFalse;
	
	GetSelectedDB(pcquerywnd, db);
	// get the db name.
	VERIFY(hwndedit = GetDlgItem(hwnd, IDC_DBNAME));

	VERIFY(len = SendMessage(hwndedit, WM_GETTEXT, SIZE_64,(LPARAM)viewname));
	
	if(IsFieldBlank(hwndedit) != 1)
	{
		yog_message(hwndedit, _(L"Please provide a valid view name"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR | MB_HELP);
		SetFocus(hwndedit);
		return wyFalse;
	}

	viewnamestr.SetAs(viewname);

	//to trim empty spaces from right, to avoid mysql errors
	viewnamestr.RTrim();

	query.Sprintf("show tables from `%s` like '%s'", db.GetString(), viewnamestr.GetString());
    res = ExecuteAndGetResult(pcquerywnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query);
	if(!res && pcquerywnd->m_tunnel->mysql_affected_rows(pcquerywnd->m_mysql)== -1)
	{
		ShowMySQLError(hwnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query.GetString());
		return wyFalse;
	}
	if(pcquerywnd->m_tunnel->mysql_num_rows(res) > 0)
	{
		msg.Sprintf(_("Can't create view '%s'; a table or view with that name already exists"), viewnamestr.GetString());
		yog_message(hwndedit, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
		SetFocus(hwndedit);
		return wyFalse;
	}

	pcquerywnd->m_tunnel->mysql_free_result(res);
	return wyTrue;
}

wyBool
FrameWindow::HandleCreateEvent(HWND hwnd, MDIWindow *pcquerywnd, wyWChar *eventname)
{
	wyInt32     len;
	HWND        hwndedit;
    wyString    query, msg, db;
    MYSQL_RES	*res;
	wyString	eventnamestr;
	wyBool		iscollate = wyFalse;

	if(GetmySQLCaseVariable(pcquerywnd) == 0)
		if(!IsLowercaseFS(pcquerywnd))
			iscollate = wyTrue;
	GetSelectedDB(pcquerywnd, db);
	// get the db name.
	hwndedit = GetDlgItem(hwnd, IDC_DBNAME);

	len = SendMessage(hwndedit, WM_GETTEXT, SIZE_64, (LPARAM)eventname);
	
	if(eventname)
		eventnamestr.SetAs(eventname);

	//to trim empty spaces from right, to avoid mysql errors
	eventnamestr.RTrim();

	if(IsFieldBlank(hwndedit) != 1)
	{
		yog_message(hwndedit, _(L"Please provide a valid event name"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR |MB_HELP);
		SetFocus(hwndedit);
		return wyFalse;
	}

	if(iscollate)
		query.Sprintf("show events where BINARY db = '%s' and name = '%s'", db.GetString(), eventnamestr.GetString());
	else
		query.Sprintf("show events where db = '%s' and name = '%s'", db.GetString(), eventnamestr.GetString());
	
    res = ExecuteAndGetResult(pcquerywnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query);

	if(!res && pcquerywnd->m_tunnel->mysql_affected_rows(pcquerywnd->m_mysql)== -1)
	{
		ShowMySQLError(hwnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query.GetString());
		return wyFalse;
	}
	if(pcquerywnd->m_tunnel->mysql_num_rows(res)> 0)
	{
		msg.Sprintf(_("Can't create event '%s'; Event exists"), eventnamestr.GetString());
		yog_message(hwndedit, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
		SetFocus(hwndedit);
		return wyFalse;
	}

	pcquerywnd->m_tunnel->mysql_free_result(res);
	return wyTrue;
}

wyBool 
FrameWindow::HandleCreateProcedure(HWND hwnd, MDIWindow *pcquerywnd, wyWChar *procedurename)
{
	wyInt32     len;
	HWND        hwndedit;
    wyString    query, msg, db;
    MYSQL_RES	*res;
	wyString	procedurenamestr;
    wyBool		iscollate = wyFalse;  

    if(!procedurename)
		return wyFalse;	    		
	if(GetmySQLCaseVariable(pcquerywnd) == 0)
		if(!IsLowercaseFS(pcquerywnd))
			iscollate = wyTrue;
        GetSelectedDB(pcquerywnd, db);
	// get the db name.
	VERIFY(hwndedit = GetDlgItem(hwnd, IDC_DBNAME));

	VERIFY(len = SendMessage(hwndedit, WM_GETTEXT, SIZE_64, (LPARAM)procedurename));

	if(IsFieldBlank(hwndedit) != 1)
	{
		yog_message(hwndedit, _(L"Please provide a valid procedure name"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR |MB_HELP);
		SetFocus(hwndedit);
		return wyFalse;
	}

	procedurenamestr.SetAs(procedurename);

	//to trim empty spaces from right, to avoid mysql errors
	procedurenamestr.RTrim();
	if(iscollate)
		query.Sprintf("show procedure status where BINARY db = '%s' and name = '%s'", db.GetString(), procedurenamestr.GetString());
	else
		query.Sprintf("show procedure status where db = '%s' and name = '%s'", db.GetString(), procedurenamestr.GetString());
    res = ExecuteAndGetResult(pcquerywnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query);

	if(!res && pcquerywnd->m_tunnel->mysql_affected_rows(pcquerywnd->m_mysql)== -1)
	{
		ShowMySQLError(hwnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query.GetString());
		return wyFalse;
	}
	if(pcquerywnd->m_tunnel->mysql_num_rows(res)> 0)
	{
		msg.Sprintf(_("Can't create procedure '%s'; Procedure exists"), procedurenamestr.GetString());
		yog_message(hwndedit, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
		SetFocus(hwndedit);
		return wyFalse;
	}

	pcquerywnd->m_tunnel->mysql_free_result(res);
	return wyTrue;
}

wyBool 
FrameWindow::HandleCreateFunction(HWND hwnd, MDIWindow	*pcquerywnd, wyWChar *functionname)
{
	wyInt32     len;
	HWND        hwndedit;
    wyString    query, msg, db;
	MYSQL_RES	*res;
	wyString	functionnamestr;
	wyBool		iscollate = wyFalse;
	if(!functionname)
		return wyFalse;
	if(GetmySQLCaseVariable(pcquerywnd) == 0)
		if(!IsLowercaseFS(pcquerywnd))
			iscollate = wyTrue;
	GetSelectedDB(pcquerywnd, db);
	// get the db name.
	VERIFY(hwndedit = GetDlgItem(hwnd, IDC_DBNAME));

	VERIFY(len = SendMessage(hwndedit, WM_GETTEXT, SIZE_64,(LPARAM)functionname));

	if(IsFieldBlank(hwndedit) != 1)
	{
		yog_message(hwndedit, _(L"Please provide a valid function name"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR |MB_HELP);
		SetFocus(hwndedit);
		return wyFalse;
	}

	functionnamestr.SetAs(functionname);

	//to trim empty spaces from right, to avoid mysql errors
	functionnamestr.RTrim();
	if(iscollate)
		query.Sprintf("show function status where BINARY db = '%s' and name = '%s'", db.GetString(), functionnamestr.GetString());
	else
	query.Sprintf("show function status where db = '%s' and name = '%s'", db.GetString(), functionnamestr.GetString());

    res = ExecuteAndGetResult(pcquerywnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query);
	if(!res && pcquerywnd->m_tunnel->mysql_affected_rows(pcquerywnd->m_mysql)== -1)
	{
		ShowMySQLError(hwnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query.GetString());
		return wyFalse;
	}
	if(pcquerywnd->m_tunnel->mysql_num_rows(res)> 0)
	{
		msg.Sprintf(_("Can't create function '%s'; Function exists"), functionnamestr.GetString());
		yog_message(hwndedit, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
		SetFocus(hwndedit);
		return wyFalse;
	}

	pcquerywnd->m_tunnel->mysql_free_result(res);
	return wyTrue;
}


wyBool 
FrameWindow::HandleCreateTrigger(HWND hwnd, MDIWindow	*pcquerywnd, wyWChar *triggername)
{
	wyInt32	    len;
	HWND		hwndedit;    
    wyString    query, msg, db;
	MYSQL_RES	*res;
	wyString	triggernamestr;

	if(!triggername)
		return wyFalse;

	GetSelectedDB(pcquerywnd, db);
	// get the db name.
	VERIFY(hwndedit = GetDlgItem(hwnd, IDC_DBNAME));
	VERIFY(len = SendMessage(hwndedit, WM_GETTEXT, SIZE_64, (LPARAM)triggername));

	if(IsFieldBlank(hwndedit) != 1)
	{
		yog_message(hwndedit, _(L"Please provide a valid trigger name"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR |MB_HELP);
		SetFocus(hwndedit);
		return wyFalse;
	}

	triggernamestr.SetAs(triggername);

	//to trim empty spaces from right, to avoid mysql errors
	triggernamestr.RTrim();
	
	//query.Sprintf("select `TRIGGER_NAME` from `INFORMATION_SCHEMA`.`TRIGGERS`  where `TRIGGER_SCHEMA` = '%s' and  TRIGGER_NAME ='%s'", 
		//db.GetString(), triggernamestr.GetString());

	query.Sprintf("show triggers from `%s` where `Trigger` = '%s'", db.GetString(), triggernamestr.GetString());

	res = ExecuteAndGetResult(pcquerywnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query);
	if(!res && pcquerywnd->m_tunnel->mysql_affected_rows(pcquerywnd->m_mysql)== -1)
	{
		ShowMySQLError(hwnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query.GetString());
		return wyFalse;
	}

	if(pcquerywnd->m_tunnel->mysql_num_rows(res) > 0)
	{
		msg.Sprintf(_("Can't create trigger '%s'; already exists"), triggernamestr.GetString());
		yog_message(hwndedit, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
		SetFocus(hwndedit);
		return wyFalse;
	}

	pcquerywnd->m_tunnel->mysql_free_result(res);
	return wyTrue;
}

void  
FrameWindow::PrepareCreateProcedure(MDIWindow *pcquerywnd, const wyChar *procedurename, wyString &strproc)
{
	wyString    db;

	GetSelectedDB(pcquerywnd, db);

	if(db.GetLength() == 0)
        db.SetAs("db");

    strproc.Sprintf("CREATE\r\n    /*[DEFINER = { user | CURRENT_USER }]*/\r\n    PROCEDURE `%s`.`%s`()\r\n\
    /*LANGUAGE SQL\r\n\
    | [NOT] DETERMINISTIC\r\n\
    | { CONTAINS SQL | NO SQL | READS SQL DATA | MODIFIES SQL DATA }\r\n\
    | SQL SECURITY { DEFINER | INVOKER }\r\n\
    | COMMENT 'string'*/\r\n\tBEGIN\r\n\r\n\tEND",
    db.GetString(), procedurename, db.GetString(), procedurename);

	return;
}

void  
FrameWindow::PrepareCreateFunction(MDIWindow	*pcquerywnd, const wyChar *functionname, wyString &strfunc)
{
	wyString    db;

	GetSelectedDB(pcquerywnd, db);

	if(db.GetLength() == 0)
        db.SetAs("db");

    strfunc.Sprintf("CREATE\r\n    /*[DEFINER = { user | CURRENT_USER }]*/\r\n    FUNCTION `%s`.`%s`()\r\n    RETURNS type\r\n\
    /*LANGUAGE SQL\r\n\
    | [NOT] DETERMINISTIC\r\n\
    | { CONTAINS SQL | NO SQL | READS SQL DATA | MODIFIES SQL DATA }\r\n\
    | SQL SECURITY { DEFINER | INVOKER }\r\n\
    | COMMENT 'string'*/\r\n\
    BEGIN\r\n\r\n\
    END",
    db.GetString(), functionname, db.GetString(), functionname);

	return ;
}


void 
FrameWindow::PrepareCreateTrigger(MDIWindow *pcquerywnd, const wyChar *triggername, wyString &strtrigg)
{
	wyString	db, tbl;
	wyInt32     image;

	GetSelectedDB(pcquerywnd, db);

	image = pcquerywnd->m_pcqueryobject->GetSelectionImage();

	///Insert table name into the trigger template if current selection on table in object browser
	if(image == NTABLE)
		GetSelectedTable(pcquerywnd, tbl);

	else	
		tbl.SetAs("<Table Name>");
	
	if(db.GetLength() == 0)
        db.SetAs("db");

    strtrigg.Sprintf("CREATE\r\n    /*[DEFINER = { user | CURRENT_USER }]*/\r\n    TRIGGER `%s`.`%s` BEFORE/AFTER INSERT/UPDATE/DELETE\r\n    ON `%s`.`%s`\r\n    FOR EACH ROW BEGIN\r\n\r\n    END",
                        db.GetString(), triggername, db.GetString(), tbl.GetString());

	return;
}

void  
FrameWindow::PrepareCreateView(MDIWindow *pcquerywnd, const wyChar *viewname, wyString &strview, wyString *qbquery)
{
	wyString		db;
	    
	GetSelectedDB(pcquerywnd, db);

	if(db.GetLength() == 0)
        db.SetAs("db");
	

    strview.Sprintf("\r\nCREATE\r\n\
    /*[ALGORITHM = {UNDEFINED | MERGE | TEMPTABLE}]\r\n\
    [DEFINER = { user | CURRENT_USER }]\r\n\
    [SQL SECURITY { DEFINER | INVOKER }]*/\r\n\
    VIEW `%s`.`%s` \r\n    AS\r\n",db.GetString(), viewname, db.GetString(), viewname);
	
	//If view is created from object browser
	if(!qbquery) 
		strview.AddSprintf("(SELECT * FROM ...);\r\n");

#ifndef COMMUNITY
	if(qbquery)
		strview.Add(qbquery->GetString());		
#endif

	return;
}
void  
FrameWindow::PrepareCreateEvent(MDIWindow *pcquerywnd, const wyChar *eventname, wyString &strevent)
{
	wyString    db;

	GetSelectedDB(pcquerywnd, db);

	if(db.GetLength() == 0)
        db.SetAs("db");

    strevent.Sprintf("-- SET GLOBAL event_scheduler = ON$$     -- required for event to execute but not create\
    \r\n\r\nCREATE\t/*[DEFINER = { user | CURRENT_USER }]*/\
	EVENT `%s`.`%s`\r\n\r\nON SCHEDULE\r\n\t /* uncomment the example below you want to use */\r\n\r\n\
	-- scheduleexample 1: run once\r\n\r\n\
	   --  AT 'YYYY-MM-DD HH:MM.SS'/CURRENT_TIMESTAMP { + INTERVAL 1 [HOUR|MONTH|WEEK|DAY|MINUTE|...] }\r\n\r\n\
	-- scheduleexample 2: run at intervals forever after creation\r\n\r\n\
	   -- EVERY 1 [HOUR|MONTH|WEEK|DAY|MINUTE|...]\r\n\r\n\
	-- scheduleexample 3: specified start time, end time and interval for execution\r\n\
	   /*EVERY 1  [HOUR|MONTH|WEEK|DAY|MINUTE|...]\r\n\r\n\
	   STARTS CURRENT_TIMESTAMP/'YYYY-MM-DD HH:MM.SS' { + INTERVAL 1[HOUR|MONTH|WEEK|DAY|MINUTE|...] }\r\n\r\n\
	   ENDS CURRENT_TIMESTAMP/'YYYY-MM-DD HH:MM.SS' { + INTERVAL 1 [HOUR|MONTH|WEEK|DAY|MINUTE|...] } */\r\n\r\n\
/*[ON COMPLETION [NOT] PRESERVE]\r\n[ENABLE | DISABLE]\r\n[COMMENT 'comment']*/\r\n\r\nDO\r\n\tBEGIN\r\n\t    (sql_statements)\r\n\tEND",		          
	db.GetString(), eventname, db.GetString(), eventname);
	
	return;
}
//using this fun getting query for alter event displaing on the adv EditorTab of event
wyBool 
FrameWindow::PrepareAlterEvent(MDIWindow *pcquerywnd, wyString &altereventstmt)
{
	wyInt32      index = 0;
	wyString     db, alterevent;
    wyString     query;
	MYSQL_RES	 *res;
	MYSQL_ROW	 row;	
	
	db.SetAs(pcquerywnd->m_pcqueryobject->m_seldatabase.GetString());
	alterevent.SetAs(pcquerywnd->m_pcqueryobject->m_seltable.GetString());// currently selected event will get from m_seltable string

	if((db.GetLength() == 0) || (alterevent.GetLength() == 0)) 
		return wyFalse;

	query.Sprintf("show create event `%s`.`%s`", db.GetString(), alterevent.GetString());
	
    res = ExecuteAndGetResult(pcquerywnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query);

	if(!res && pcquerywnd->m_tunnel->mysql_affected_rows(pcquerywnd->m_mysql)== -1)
	{
		ShowMySQLError(pcquerywnd->m_hwnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query.GetString());
		return wyFalse;
	}

	index = GetFieldIndex(res, "Create Event", pcquerywnd->m_tunnel, &pcquerywnd->m_mysql);
	if(index == -1)
		return wyFalse;

	if(row = pcquerywnd->m_tunnel->mysql_fetch_row(res))
		altereventstmt.SetAs(row[index]);
	else
		return wyFalse;    	

	pcquerywnd->m_tunnel->mysql_free_result(res);
	altereventstmt.Replace(0, 6, "ALTER");   
   
	return wyTrue;
}

wyBool 
FrameWindow::HandleGoTo(HWND hwnd, MDIWindow	*pcquerywnd, wyWChar *linenumber)
{
    wyInt32     lineno, linecount;
    wyString    line;
	EditorBase	*peditorbase = pcquerywnd->GetActiveTabEditor()->m_peditorbase;

	SendMessage(GetDlgItem(hwnd, IDC_DBNAME), WM_GETTEXT,(WPARAM)SIZE_128-1,(LPARAM)linenumber);

		SetFocus(peditorbase->m_hwnd);      
	lineno = _wtoi(linenumber);
    
	linecount = SendMessage(peditorbase->m_hwnd, SCI_GETLINECOUNT, 0, 0); 

    if(lineno == 0 || linecount < (lineno))
    {
        yog_message(hwnd, _(L"Line number out of range!"), pGlobals->m_appname.GetAsWideChar(), MB_ICONINFORMATION);
        line.Sprintf("%d", linecount);
        SendMessage(GetDlgItem(hwnd, IDC_DBNAME), WM_SETTEXT, 0, (LPARAM)line.GetAsWideChar());
        SendMessage(GetDlgItem(hwnd, IDC_DBNAME), EM_SETSEL, 0, -1);
		return wyFalse;
    }
    else
    {
        SendMessage(peditorbase->m_hwnd, SCI_GOTOLINE,
					 (WPARAM)(lineno - 1), 0);

	    return wyTrue;
    }
}


void  
FrameWindow::GetSelectedDB(MDIWindow *pcquerywnd, wyString &db)
{
	HTREEITEM	hitem;
	wyInt32     image = pcquerywnd->m_pcqueryobject->GetSelectionImage();
	
	VERIFY(hitem = TreeView_GetSelection(pcquerywnd->m_pcqueryobject->m_hwnd));

	switch(image)
	{
		case NPRIMARYKEY:
		case NCOLUMN:
		case NPRIMARYINDEX:
		case NINDEX:
			VERIFY(hitem = TreeView_GetParent(pcquerywnd->m_pcqueryobject->m_hwnd, hitem));
			
		case NFOLDER:
			VERIFY(hitem = TreeView_GetParent(pcquerywnd->m_pcqueryobject->m_hwnd, hitem));

		case NFUNCITEM:
		case NSPITEM:
		case NEVENTITEM:
		case NVIEWSITEM:
		case NTRIGGERITEM:
		case NTABLE:
			VERIFY(hitem = TreeView_GetParent(pcquerywnd->m_pcqueryobject->m_hwnd, hitem));	

		case NEVENTS:
		case NTRIGGER:
		case NFUNC:
		case NVIEWS:
		case NSP:
		case NTABLES:
			VERIFY(hitem = TreeView_GetParent(pcquerywnd->m_pcqueryobject->m_hwnd, hitem));			
	}
	
	 pcquerywnd->m_pcqueryobject->GetDatabaseName(hitem);
	 db.SetAs(pcquerywnd->m_pcqueryobject->m_seldatabase.GetString());
	 return;
}

void  
FrameWindow::GetSelectedTable(MDIWindow *pcquerywnd, wyString &table)
{
	HTREEITEM	hitem;
	wyInt32     image= pcquerywnd->m_pcqueryobject->GetSelectionImage();
	
	VERIFY(hitem = TreeView_GetSelection(pcquerywnd->m_pcqueryobject->m_hwnd));

	switch(image)
	{
		case NPRIMARYKEY:
		case NCOLUMN:
		case NPRIMARYINDEX:
		case NINDEX:
			VERIFY(hitem = TreeView_GetParent(pcquerywnd->m_pcqueryobject->m_hwnd, hitem));

		case NFOLDER:
		case NTRIGGER:
		case NFUNCITEM:
		case NSPITEM:
		//case NVIEWSITEM:
		case NTRIGGERITEM:
			VERIFY(hitem = TreeView_GetParent(pcquerywnd->m_pcqueryobject->m_hwnd, hitem));
	}
	
	pcquerywnd->m_pcqueryobject->GetTableDatabaseName(hitem);
	table.SetAs(pcquerywnd->m_pcqueryobject->m_seltable.GetString());
	return;
}

void  
FrameWindow::GetObjectName(MDIWindow *pcquerywnd, wyString &objectname)
{
	HTREEITEM	hitem;
	
	VERIFY(hitem = TreeView_GetSelection(pcquerywnd->m_pcqueryobject->m_hwnd));
	pcquerywnd->m_pcqueryobject->GetTableDatabaseName(hitem);
	objectname.SetAs(pcquerywnd->m_pcqueryobject->m_seltable.GetString());
	return;
}
void 
FrameWindow::Refresh(MDIWindow	*pcquerywnd)
{
	EditorBase		*peditorbase = pcquerywnd->GetActiveTabEditor()->m_peditorbase;
	//pcquerywnd->m_pcqueryobject->RefreshObjectBrowserOnCreateAlterTable()
	if(pcquerywnd->m_pcqueryobject->GetSelectionImage()== NSERVER)
		pcquerywnd->m_pcqueryobject->RefreshObjectBrowser(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql,pcquerywnd);
	else
	{
	/*	Refresh Parent node only, leaving others unchanged, So we can
		reduce the flickering little more */
		SendMessage(pcquerywnd->m_pcqueryobject->m_hwnd, WM_SETREDRAW, FALSE, 0);
		
		HTREEITEM hitem = peditorbase->m_hitem;

		//..Code that makes Selection in the Object Browser when the procedure is executed and the database node is never expanded..
        if(TreeView_GetItemState(pcquerywnd->m_pcqueryobject->m_hwnd, pcquerywnd->m_pcqueryobject->GetDatabaseNode() , TVIS_EXPANDEDONCE) & TVIS_EXPANDEDONCE)
			pcquerywnd->m_pcqueryobject->RefreshParentNode(hitem);
		
		SendMessage(pcquerywnd->m_pcqueryobject->m_hwnd, WM_SETREDRAW, TRUE, 0);
	}	

	return;
}

void 
FrameWindow::InitFavorites(HWND hwnd)
{	
	m_pcfavoritemenu    = new  CFavoriteMenu();
	m_pcaddfavorite     = new FavoriteAdd();
	m_pcorganizefavorite= new COrganizeFavorite();

	m_pcfavoritemenu->CreateFavoriteFolder();
	m_pcfavoritemenu->Display(hwnd);

	return;
}

wyBool
FrameWindow::ReadFromFavorite(HMENU hmenu, wyInt32  id)
{
	wyWChar         menuname[MAX_PATH + 1]={0};
    wyString        completepath;
	wyString		menunamestr;
	wyChar          *text = 0, *trimmed = 0, *temp = NULL;
	DWORD			dwfilesize,dwbyteswritten;
	HANDLE			hfile;
	MENUITEMINFO	lpmii = {0};
	MDIWindow       *wnd = GetActiveWin();
	EditorBase		*peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
	wyString		menuitemstr,textbufferstr;
    wyBool          isutf8 = wyTrue, isansi, ischecked = wyFalse;

	memset((void *)&lpmii, 0, sizeof(MENUITEMINFO));

	lpmii.cbSize		= sizeof(MENUITEMINFO);
	lpmii.fMask			= MIIM_DATA | MIIM_STRING | MIIM_ID;
	lpmii.cch			= MAX_PATH;
	lpmii.dwTypeData	= menuname;

	VERIFY(GetMenuItemInfo(GetMenu(pGlobals->m_pcmainwin->m_hwndmain), id, FALSE, &lpmii));
	
	menunamestr.SetAs(menuname);

	completepath.Sprintf("%s\\%s.sql", (wyChar *)lpmii.dwItemData, menunamestr.GetString());
	
	hfile = CreateFile(completepath.GetAsWideChar(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
								NULL, NULL);
	
	if(hfile == INVALID_HANDLE_VALUE)
    {
		DisplayErrorText(GetLastError(), _("Could not open the referenced query file.\r\nMake sure that the query file exists."));
		return wyFalse;
	}

	dwfilesize = GetFileSize(hfile, NULL);

	if(dwfilesize == 0)
    {
		yog_message(pGlobals->m_pcmainwin->m_hwndmain, _(L"The selected Favorite item is empty. Nothing to add."), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);
		CloseHandle(hfile);
		return wyFalse;
	}

	text = AllocateBuff((dwfilesize) + 1);

	if(!ReadFile(hfile, text, dwfilesize, &dwbyteswritten, NULL))
	{
    	DisplayErrorText(GetLastError(), _("Error reading favorite file."));
		free(text);
		CloseHandle(hfile);
		return wyFalse;
	}

	text[dwbyteswritten] = 0;
	
	textbufferstr.SetAs(text);
    
    isutf8 = CheckForUtf8(textbufferstr);
    
    if(isutf8 == wyFalse)
    {
        ischecked = wyTrue;
        textbufferstr.SetAs(text, wyFalse);
    }
    else
    {
        isansi = CheckForUtf8Bom(completepath);
        if(isansi == wyFalse)
        {
            temp = text + 3;
            textbufferstr.SetAs(temp);
        }
    }

	if(text)
		free(text);
	
	text = AllocateBuff(textbufferstr.GetLength() + 1);
	strcpy(text, textbufferstr.GetString());

	trimmed = TrimLeft(text);

    if(trimmed == NULL)
        return wyFalse;
    	
	if(strnicmp(trimmed, "file", 4) == 0)
    {
		trimmed += 5; // to remove the "File:" string in the startup
		ReadFromFile(trimmed);
	}
	else 
    {
		SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, 
					(WPARAM)strlen(text), (LPARAM)text);
		
		//SendMessage(peditorbase->m_hwnd, SCI_SETSEL, -1, -1);
	}
	
	EditorFont::SetLineNumberWidth(peditorbase->m_hwnd);     
	VERIFY(CloseHandle(hfile));
	
	free(text);
	return wyFalse;
}

wyBool	
FrameWindow::ReadFromFile(wyChar *completepath)
{	
	wyChar      *sqltext = 0;
    wyString    msg, completepathstr;
	DWORD	    dwfilesize, dwbyteswritten;
	HANDLE		hfile;
	MDIWindow   *wnd = GetActiveWin();
	wyString	sqltextstr;
	EditorBase	*peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;

         if(completepath)
		completepathstr.SetAs(completepath);

	hfile = CreateFile(completepathstr.GetAsWideChar(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
								NULL, NULL);
	
	if(hfile == INVALID_HANDLE_VALUE)
    {
		if(GetLastError()== ERROR_FILE_NOT_FOUND)
		{	
			wyString comppathstr(completepath);
			msg.Sprintf(_("Could not find '%s'\nPlease verify the referenced file exists."), comppathstr.GetString());
			DisplayErrorText(GetLastError(), msg.GetString());
		}
		else
			DisplayErrorText(GetLastError(), _("Could not Open Favorite File."));

		return wyFalse;
	}

	dwfilesize = GetFileSize(hfile, NULL);
	
	if(dwfilesize == 0)
    {
		yog_message(pGlobals->m_pcmainwin->m_hwndmain, _(L"File is Empty"), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);
		CloseHandle(hfile);
		return wyFalse;
	}

	sqltext = AllocateBuff(dwfilesize+1);

	if(!ReadFile(hfile, sqltext, dwfilesize, &dwbyteswritten, NULL))
	{	
		DisplayErrorText(GetLastError(), _("Error reading favorite file."));
		VERIFY(CloseHandle(hfile));
		free(sqltext);
		return wyFalse;
	}

	if(sqltext)
		sqltextstr.SetAs(sqltext);

    
	//Testing .. image handling
/*
	wyInt32	headersize;

	wyInt32	filetype = DetectFileFormat(sqltext, dwfilesize, &headersize);

	if(filetype != NCP_UTF8)
	{
		sqltextstr.SetAs(sqltext);

		if(CheckForUtf8(sqltextstr) == wyFalse)
			sqltextstr.SetAs(sqltext, wyFalse);
	}*/

	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, (WPARAM)sqltextstr.GetLength(), (LPARAM)sqltextstr.GetString());
	SendMessage(peditorbase->m_hwnd, SCI_SETSEL, -1, -1);
	
	VERIFY(CloseHandle(hfile));
	free(sqltext);

	return wyFalse;
}


wyBool
FrameWindow::ConvertIniFileToUtf8()
{
	wyWChar     directory[MAX_PATH+1]={0}, *lpfileport=0;
	wyInt32		ret;
	wyString	dirstr;
	wyString	encoding;
	wyBool		isconv;

	// Get the complete path.
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	if(ret == wyFalse)
		return wyFalse;

	dirstr.SetAs(directory);
	
	ret = wyIni::IniGetString(SECTION_NAME, "Encoding", "", &encoding, dirstr.GetString());
	
	if(encoding.CompareI("utf8") == 0)
	{
		return wyTrue;
	}
	else
	{
		isconv = Convert(dirstr);
		if(isconv == wyTrue)
		{
			wyIni::IniWriteString(SECTION_NAME, "Encoding", "utf8", dirstr.GetString());
		}

		return isconv;
	}
}

wyBool
FrameWindow::Convert(wyString &dirstr)
{
	DWORD		filesize, byteswritten = 0, bytesread = 0;
    wyWChar		file[MAX_PATH + 1] = {0}, path[MAX_PATH + 1] = {0};
	wyChar		*filebuf = NULL, *tempfilebuf = NULL;
	HANDLE		htempfile;
	wyString	connnumber, passwordstr, temppwd, filebuffconv;	
	wyString	temppathstr, tempfilename;
	wyChar		*pwdarr[] = {"Password", "Password01", "SshPwd", "SshPwd01", 
							 "ProxyPwd", "ProxyPwd01", "401Pwd", "401Pwd01", NULL};
	wyInt32		pwdcounter = 0, counter;
	
	
	HANDLE	hfile = CreateFile(dirstr.GetAsWideChar(), GENERIC_READ, 0, NULL, 
							   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if(hfile == INVALID_HANDLE_VALUE)
	{
		if(GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			return wyTrue;
		}
		else
		{
			DisplayErrorText(GetLastError(), _("Could not open the referenced file."));
			return wyFalse;
		}
	}

	filesize = GetFileSize(hfile, NULL);
	filebuf = AllocateBuff(filesize + 1);
	
	if(ReadFile(hfile, filebuf, filesize, &bytesread, NULL) == 0)
	{
		DisplayErrorText(GetLastError(), _("Could not open the referenced file."));
		return wyFalse;
	}

	if(pGlobals->m_configdirpath.GetLength() ||  SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, (LPWSTR)path)))
    {
		if(pGlobals->m_configdirpath.GetLength())
		{
			//wcscpy(path, pGlobals->m_configdirpath.GetAsWideChar());
			wcsncpy(path, pGlobals->m_configdirpath.GetAsWideChar(), MAX_PATH);
			path[MAX_PATH] = '\0';
		}
		else
		{
			wcscat(path, L"\\");
			wcscat(path, L"SQLyog");
		}

	    if(GetTempFileName(path, L"ini", 0, file))
	    {
		    temppathstr.SetAs(file);

		    if((htempfile = CreateFile(temppathstr.GetAsWideChar(),GENERIC_WRITE, 0, NULL, 
                                    CREATE_ALWAYS, NULL, NULL)) != INVALID_HANDLE_VALUE) 
		    {
			    if(filebuf)
				    filebuffconv.SetAs(filebuf, wyFalse);
                
			    if(WriteFile(htempfile, filebuffconv.GetString(), bytesread, &byteswritten, NULL) == 0)
			    {
				    DisplayErrorText(GetLastError(), _("Could not Write into referenced file."));
				    return wyFalse;
			    }
			    else
			    {
				    CloseHandle(htempfile);		
                    for(counter = 0; counter < MAX_CONN; counter++)
				    {
					    connnumber.Sprintf("Connection %u", counter + 1);

					    for(pwdcounter = 0; pwdarr[pwdcounter] != NULL; pwdcounter++)
						    ConvertAndWritePwd(connnumber, pwdarr[pwdcounter], temppathstr);

				    }
			    }
		    }
		    else
		    {
			    DisplayErrorText(GetLastError(), _("Could not open the referenced file."));
			    return wyFalse;
		    }
        }
        else
	    {
		    DisplayErrorText(GetLastError(), _("Could not open the referenced file."));
		    return wyFalse;
	    }
    }

	if((htempfile = CreateFile(temppathstr.GetAsWideChar(),GENERIC_READ, 0, NULL, 
                                OPEN_EXISTING, NULL, NULL)) != INVALID_HANDLE_VALUE) 
	{
		filesize = GetFileSize(htempfile, NULL);
		tempfilebuf = AllocateBuff(filesize + 1);
		if(ReadFile(htempfile, tempfilebuf, filesize, &bytesread, NULL) == 0)
		{
			DisplayErrorText(GetLastError(), _("Could not open the referenced file."));
			return wyFalse;
		}
	}
	else
	{
		DisplayErrorText(GetLastError(), _("Could not open the referenced file."));
		return wyFalse;
	}
	CloseHandle(hfile);

	hfile = CreateFile(dirstr.GetAsWideChar(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
	if(hfile == INVALID_HANDLE_VALUE)
	{
		DisplayErrorText(GetLastError(), _("Could not open the referenced file."));
		return wyFalse;
	}

	if(WriteFile(hfile, tempfilebuf, bytesread, &byteswritten, NULL) == 0)
	{
		DisplayErrorText(GetLastError(), _("Could not open the referenced file."));
		return wyFalse;
	}
	
	CloseHandle(hfile);	
	free(tempfilebuf);
	CloseHandle(htempfile);
	free(filebuf);
	DeleteFile(temppathstr.GetAsWideChar());

	return wyTrue;
}

void
FrameWindow::ConvertAndWritePwd(wyString &conncount, wyChar	*whichpwd, wyString	&path)
{
	wyString	passwordstr, temppwd;

	wyIni::IniGetString(conncount.GetString(), whichpwd, "", &passwordstr, path.GetString());						
	if(passwordstr.GetLength())
	{
		DecodePassword(passwordstr);
		temppwd.SetAs(passwordstr.GetString(), wyFalse);
		EncodePassword(temppwd);
		wyIni::IniWriteString(conncount.GetString(), whichpwd, temppwd.GetString(), path.GetString());
	}

}
void 
FrameWindow::LoadMainIcon()
{
    HICON	mainicon;

	VERIFY(m_hmainiml = ImageList_Create(ICON_SIZE, ICON_SIZE, ILC_COLOR32  | ILC_MASK, 1, 0));
	VERIFY(mainicon = (HICON)LoadImage(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_SMALL), IMAGE_ICON, DEF_IMG_WIDTH, DEF_IMG_HEIGHT, LR_DEFAULTCOLOR));
	VERIFY((ImageList_AddIcon(m_hmainiml, mainicon))!= -1);
	VERIFY(DestroyIcon(mainicon));
}

void 
FrameWindow::OnCreate()
{    
 //   CreateToolBarWindow();
	CreateStatusBarWindow();
	CreateMDIWindow();
	m_findmsg = RegisterWindowMessage(FINDMSGSTRING);
    CheckForAutoKeywords();
	ConvertIniFileToUtf8();
    MigrateFiles();
	//MigratePersonalFolderToFavorites();
    return;
}

void
FrameWindow::HandleFiles(wyWChar *filename, wyWChar *extension)
{
    wyWChar         fullpath[MAX_PATH] = {0}, pathbuffer[MAX_PATH] = {0};
    wyString        apppath, directorystr, newpath, pwdpath, bak(".bak");
	wyWChar		    directory[MAX_PATH+1] = {0};
    wyWChar         **lpfileport = 0;
    wyString        filenamestr, extensionstr, renamepath, bakupini;
    wyInt32         pos;
	
    if(filename && extension)
    {
        filenamestr.SetAs(filename);
        extensionstr.SetAs(extension);
    }

	if(pGlobals->m_configdirpath.GetLength())
		apppath.SetAs(pGlobals->m_configdirpath);

	//Handle ini file migration
    else if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, fullpath))) 
	{
        apppath.SetAs(fullpath);
		apppath.Add("\\SQLyog");
	}
    else
        return;

    if(GetModuleFileName(NULL, directory, MAX_PATH) != 0)
        pwdpath.SetAs(directory);
    else
        return;
    
    pos = StripExe(directory, pwdpath);
    if(pos == -1)
        return;
    
    //First search in the app folder
    if(SearchPath(apppath.GetAsWideChar(), filename, extension, MAX_PATH, pathbuffer, lpfileport) == 0)
    {
        //if not found search in the installation path
        if(SearchPath(NULL, filename, extension, MAX_PATH, pathbuffer, lpfileport) == 0)
        {
            return;
        }
        else
        {
            // copy the file from installation path app folder
            if(MigrateSpecificFileToAppFolder(pwdpath, apppath, filename, extension) == wyFalse)
            {
                filenamestr.SetAs(filename);
                extensionstr.SetAs(extension);
                bakupini.SetAs(pwdpath);

                bakupini.AddSprintf("%s%s%s", filenamestr.GetString(), bak.GetString(), extensionstr.GetString());
                if(_wrename(pwdpath.GetAsWideChar(), bakupini.GetAsWideChar()) != 0)
                {                    
                    return;
                }
            }
        }
    }
    else
    {
        // search for the same file in installation path if found rename
        if(SearchPath(NULL, filename, extension, MAX_PATH, pathbuffer, lpfileport) != 0)
        {   
            bakupini.SetAs(pwdpath);
            pwdpath.AddSprintf("%s%s", filenamestr.GetString(), extensionstr.GetString());
            bakupini.AddSprintf("%s%s%s", filenamestr.GetString(), bak.GetString(), extensionstr.GetString());
            if(_wrename(pwdpath.GetAsWideChar(), bakupini.GetAsWideChar()) != 0)
            {
                return;
            }
        }
   }
}

wyBool 
FrameWindow::MigrateSpecificFileToAppFolder(wyString &pwdpath, wyString &apppath, wyWChar *filename, wyWChar *extension)
{
    wyBool      iscopiedandrenamed;
    wyString    renamepath, newinipath, oldinipath, bakup(".bak");
    wyString    filenamestr, extensionstr;
    wyWChar     directory[MAX_PATH];
    
    if(filename && extension)
    {
        filenamestr.SetAs(filename);
        extensionstr.SetAs(extension);
    }

    newinipath.SetAs(apppath);
    newinipath.AddSprintf("\\%s%s", filenamestr.GetString(), extensionstr.GetString());

    if(wcscpy(directory, pwdpath.GetAsWideChar()) == NULL)
        return wyFalse;

    oldinipath.SetAs(pwdpath);
    oldinipath.AddSprintf("%s%s", filenamestr.GetString(), extensionstr.GetString());
    
    renamepath.SetAs(pwdpath);
    renamepath.AddSprintf("%s%s%s", filenamestr.GetString(), bakup.GetString(), extensionstr.GetString());

    //Copy the ini file from source path to 
    iscopiedandrenamed = CopyAndRename(oldinipath, newinipath, renamepath);
	if(iscopiedandrenamed == wyFalse)
        return wyFalse;
    
    apppath.SetAs(newinipath);
    pwdpath.SetAs(oldinipath);

    return wyTrue;
}

void
FrameWindow::MigrateFiles()
{
    //The below file types will be migrated
    HandleFiles(L"sqlyog", L".ini");
    HandleFiles(L"sja", L".log");
    HandleFiles(L"sjasession", L".xml");
    HandleFiles(L"sqlyog", L".err");
    HandleAutocompleteTagFiles();
}

// Strip the Executable name off.
wyInt32
FrameWindow::StripExe(wyWChar *directory, wyString &directorystr)
{
    wyWChar *rev = NULL;
    wyString tempstr;
    wyInt32  pos;

    rev = wcsrev(directory);
    if(rev)
        tempstr.SetAs(rev);
    else
        return -1;
    pos = tempstr.Find("\\", 0);
	directorystr.Strip(pos);
    wcsrev(directory);
    return pos;
}
void
FrameWindow::HandleAutocompleteTagFiles()
{
    wyWChar		    directory[MAX_PATH], fullpath[MAX_PATH];
    wyString        directorystr, fullpathstr, tagfilename, tempstr;
    wyString        newpath;
    WIN32_FIND_DATA	wfd;
    HANDLE          hfile;
    wyInt32         count;
    
    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, fullpath)))
        fullpathstr.SetAs(fullpath);

    VERIFY(count = GetModuleFileName(NULL, directory, MAX_PATH - 1));
	
	// change it to .ini
	directory[count - pGlobals->m_modulenamelength] = '\0';

	wcscat(directory, L"Tags");
    directorystr.SetAs(directory);
    wcscat(directory, L"\\*");

    if(_wchdir(directorystr.GetAsWideChar()) != -1)
    {
        fullpathstr.Add("\\SQLyog\\Tags");
       
        if(!CreateDirectory(fullpathstr.GetAsWideChar(), NULL))
	    {
    	    /* If the folder is there , then we will continue the process, otherwise we will return wyFalse */
	    	if(GetLastError()!= ERROR_ALREADY_EXISTS)
		    	return;
	    }
        
        hfile = FindFirstFile(directory, &wfd);
        if(hfile == INVALID_HANDLE_VALUE)
            return;
        
        //Search for the Source tags folder and copy the new 
        do
        {
            if(wfd.cFileName)
            {
                if(wcscmp(wfd.cFileName, L".") != 0)
                {
                    if(wcscmp(wfd.cFileName, L"..") != 0)
                    {
                        tagfilename.SetAs(wfd.cFileName);
                        fullpathstr.SetAs(fullpath);
                        fullpathstr.Add("\\SQLyog\\Tags\\");
                        fullpathstr.Add(tagfilename.GetString());
                        newpath.SetAs(directorystr.GetString());
                        tempstr.SetAs(directorystr.GetString());
                        tempstr.Add("\\");
                        tempstr.Add(tagfilename.GetString());
                    
                        CopyAndRename(tempstr, fullpathstr, newpath);
                    }
                }
            }

            FindNextFile(hfile, &wfd);  

		}while(GetLastError()!= ERROR_NO_MORE_FILES);

        fullpathstr.Strip(tagfilename.GetLength());

        if(_wchdir(fullpathstr.GetAsWideChar()) == -1)
            return;
            
        FindClose(hfile); 
   
    }
    
    //renaming Tags folder to tags_bakup
    /*tempstr.SetAs(directorystr.GetString());
    tempstr.Strip(strlen("Tags"));
    tempstr.Add("Tags_Backup");
    
    //rename the source
    MoveFile(directorystr.GetAsWideChar(), tempstr.GetAsWideChar());*/
}

wyBool
FrameWindow::CopyAndRename(wyString& directorystr, wyString& fullpathstr, wyString& newpath)
{
	fullpathstr.Strip(strlen("sqlyog.ini") + 1);
	if(!CreateDirectory(fullpathstr.GetAsWideChar(), NULL))
	{
		/* If the folder is there , then we will continue the process, otherwise we will return wyFalse */
		if(GetLastError()!= ERROR_ALREADY_EXISTS)
			return wyFalse;
	}
	fullpathstr.AddSprintf("\\sqlyog.ini");
    if(CopyFile(directorystr.GetAsWideChar(), fullpathstr.GetAsWideChar(), TRUE))
    {
        if(_wrename(directorystr.GetAsWideChar(), newpath.GetAsWideChar()) == 0)
        {
			return wyTrue;
        }
    }
    else
    {
		return wyFalse;
	}
	return wyTrue;
}
void 
FrameWindow::OnWmMenuCommand(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	wyUInt32    id = 0;

	if(pGlobals->m_windowsbuild)
    {			
		id = GetMenuItemID((HMENU)lparam, wparam);

		if((id < FAVORITEMENUID_START || id > FAVORITEMENUID_END) &&
            (id < ENGINE_ID_START || id > ENGINE_ID_END))

		{
			SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(id, 0), NULL);
			return;
		}
	}
	else if(!pGlobals->m_windowsbuild && (LOWORD(wparam) < FAVORITEMENUID_START || LOWORD(wparam)> FAVORITEMENUID_END ))// Windows Me/98/95
	{
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(LOWORD(wparam), 0), NULL);
		return;
	}

	if(!pGlobals->m_windowsbuild)
    {
        if(LOWORD(wparam) >= ENGINE_ID_START && LOWORD(wparam) < ENGINE_ID_END)
        {
            if(HandleChangeTabletype((HMENU)lparam, LOWORD(wparam)) == wyTrue)
                return;
        }
        else
		    ReadFromFavorite((HMENU)lparam, LOWORD(wparam));
    }
	else
    {
        if(id >= ENGINE_ID_START && id < ENGINE_ID_END)
        {
            if(HandleChangeTabletype((HMENU)lparam, id) == wyTrue)
                return;
        }
        else
	    	ReadFromFavorite((HMENU)lparam, id);
    }

    return;
}

void
FrameWindow::OnActivate(WPARAM wparam)
{
	MDIWindow			*pcquerywnd = NULL;
#ifndef COMMUNITY
	wyInt32				imgid = 0;
	HWND				tabhwnd = NULL;
	TabSchemaDesigner	*tabsd = NULL;
	TabQueryBuilder		*tabqb = NULL;
#endif

	VERIFY(pcquerywnd = GetActiveWin());

	if(!pcquerywnd)
		return;

	if(wparam == WA_ACTIVE || wparam == WA_CLICKACTIVE)
	{
		PostMessage(pcquerywnd->m_hwnd, UM_FOCUS, 0, 0);

#ifndef COMMUNITY
		//To solve Painting ob SD/QB canvas and its tables when switching between different SQLyogs
		imgid = pcquerywnd->m_pctabmodule->GetActiveTabImage();
		
		if(imgid == IDI_SCHEMADESIGNER_16)
		{
			tabsd = (TabSchemaDesigner*)pcquerywnd->m_pctabmodule->GetActiveTabType();
			if(!tabsd)
				return;

			tabhwnd = tabsd->m_hwndcanvas;
		}

		else if(imgid == IDI_QUERYBUILDER_16)
		{
			tabqb = (TabQueryBuilder*)pcquerywnd->m_pctabmodule->GetActiveTabType();
			if(!tabqb)
				return;

			tabhwnd = tabqb->m_hwndtabview;
		}

		if(tabhwnd)
		{
			InvalidateRect(tabhwnd , NULL, TRUE);
			UpdateWindow(tabhwnd);
		}
#endif
	}

	else
	{
		if(GetFocus())
			pcquerywnd->m_lastfocus = GetFocus();
	}

	return;
}


wyBool 
FrameWindow::OnWmClose(HWND hwnd)
{
	wyInt32 ret = 1;
#ifndef COMMUNITY
    HWND    hwndvdd;
	wyString  vddmsg;
	if(pGlobals->m_pcmainwin->m_closealltrans != IN_TRANSACTION)
		pGlobals->m_pcmainwin->m_closealltrans = 1;
#endif

	wyBool retval = wyTrue;
	wyString sqlitequery, sqliteerr;

	//block background save thread
	
		
		//m_sqlyogclosed = wyTrue;
	SetEvent(m_savetimerevent);
	ResetEvent(m_sqlyogcloseevent);
	if(pGlobals->m_sessionrestore)
	{
		if(pGlobals->m_issessionsaveactive == wyTrue)
		{
			WaitForSingleObject (m_sessionsaveevent, INFINITE);
			retval = SaveConnectionDetails();
			//if(retval == wyFalse)
				//return wyFalse;
		}
	}

	SendMessage(hwnd, WM_CLOSEALLWINDOW, 0,(LPARAM)&ret);

    if(m_iscloseallmdi == wyTrue)
    {
        SendMessage(m_hwndconntab, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(m_hwndconntab, NULL, TRUE);
        UpdateWindow(m_hwndconntab);
    }

	if(!ret)
    {
	
		SetEvent(m_sqlyogcloseevent);
		return wyFalse;
    }
	else
	{
#ifndef COMMUNITY
        if((hwndvdd = VisualDataDiff::FindInstance()))
        {
			vddmsg.Sprintf(_("There are one or more open instances of %s. Please close them to close SQLyog."), _("Visual Data Compare"));
			MessageBox(m_hwndmain,vddmsg.GetAsWideChar(), 
                pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);

            ShowWindow(hwndvdd, SW_SHOWNORMAL);
			
			SetEvent(m_sqlyogcloseevent);
            return wyFalse;
        }
#endif
        m_connection->OnClose();
    /* for mysql pro we dont have to do anything */
		WriteSplitterPos(TEXT(HSPLITTER_SECTION_NAME));
		WriteSplitterPos(TEXT(VSPLITTER_SECTION_NAME));
		VERIFY(WriteInitPos(m_hwndmain));
		VERIFY(DestroyWindow(m_hwndmain));
	}
	//kill background save thread
	m_sqlyogclosed = wyTrue;
	KillTimer(pGlobals->m_pcmainwin->m_hwndmain, CONSAVE_TIMER);
	SetEvent(m_sqlyogcloseevent);
	WaitForSingleObject (m_savethread_handle, 1000);
	if(!retval)
	{
		sqlitequery.Sprintf("DELETE FROM conndetails");
		pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
	}
	if(pGlobals->m_sqliteobj)
	{
		pGlobals->m_sqliteobj->Close();
		delete (pGlobals->m_sqliteobj);
	
	}
	if(pGlobals->m_mdiwlist){
		delete pGlobals->m_mdiwlist;
		pGlobals->m_mdiwlist = NULL;
	}
    return wyTrue;
}

wyBool 
FrameWindow::OnWmInitPopup(WPARAM wparam, LPARAM lparam)
{
	wyInt32     lstyle, menuindex;
	HWND	    hwndactive = (HWND)SendMessage(pGlobals->m_hwndclient, WM_MDIGETACTIVE, 0, NULL);
	HMENU       menu = GetMenu(m_hwndmain);
    wyBool      iswindowmenu = wyFalse;
	MDIWindow	*wnd;
    TabEditor*  pte = NULL;
		
    wnd = (MDIWindow*)GetWindowLongPtr(hwndactive, GWLP_USERDATA);
						
	if(!hwndactive)
	{
        HandleMenuOnNoConnection(wparam, lparam);
		return wyTrue;		
	}
    else
	{
		// now we check whether the window is maximized or not.
		lstyle = GetWindowLongPtr(hwndactive, GWL_STYLE);
		menuindex = (LOWORD(lparam));

		wnd = (MDIWindow*)GetWindowLongPtr(hwndactive, GWLP_USERDATA);

		if(GetSubMenu(menu, menuindex) != (HMENU)wparam)
		{
			return wyTrue;
		}
		if(!(lstyle & WS_MAXIMIZE))
			menuindex++;
		else if(menuindex == 0 && (wnd->m_executing == wyTrue || wnd->m_pingexecuting == wyTrue))
            return wyTrue;

        iswindowmenu = m_connection->IsWindowMenu(menuindex);
		
		if(wnd->m_executing == wyTrue || wnd->m_pingexecuting == wyTrue)
        {
			RecursiveMenuEnable((HMENU)wparam, iswindowmenu);
			
			// if its the first menu then we allow two option - new window and exit 
			if(menuindex < MNUEDIT_INDEX)
            {
				EnableMenuItem((HMENU)wparam,IDM_FILE_NEWCONNECTION, MF_ENABLED | MF_BYCOMMAND);
				EnableMenuItem((HMENU)wparam,IDM_FILE_EXIT, MF_ENABLED | MF_BYCOMMAND);
				EnableMenuItem((HMENU)wparam,IDM_FILE_NEWSAMECONN, MF_ENABLED | MF_BYCOMMAND);
			} 

			return wyFalse;
		} 

		RecursiveMenuEnable((HMENU)wparam, iswindowmenu, MF_ENABLED);
		if(wyTheme::m_theme->IsSysmenuEnabled(GetActiveWin()->m_hwnd))
			m_connection->HandleMenu(menuindex, (HMENU)wparam);//if system menu is not enabled we need to move one extra in menu
		else
			m_connection->HandleMenu(menuindex+1, (HMENU)wparam);

	}

    /// Dissable list all tags and list matching tags menu items if qb/sd is on
    if(wnd)
	{
        if(!(pte = (TabEditor*)wnd->m_pctabmodule->GetActiveTabEditor()) || 
            wnd->m_pctabmodule->GetActiveTabImage() == IDI_DATASEARCH ||
            GetFocus() != pte->m_peditorbase->m_hwnd)
        {
            EnableMenuItem((HMENU)wparam, ID_EDIT_LISTALLTAGS,  MF_GRAYED | MF_BYCOMMAND);
            EnableMenuItem((HMENU)wparam, ID_EDIT_LISTMATCHINGTAGS, MF_GRAYED | MF_BYCOMMAND);   
            EnableMenuItem((HMENU)wparam, ID_EDIT_LISTFUNCTIONANDROUTINEPARAMETERS, MF_GRAYED | MF_BYCOMMAND);   
        }
#ifdef COMMUNITY
        else
        {
            EnableMenuItem((HMENU)wparam, ID_EDIT_LISTALLTAGS,  MF_ENABLED);
            EnableMenuItem((HMENU)wparam, ID_EDIT_LISTMATCHINGTAGS, MF_ENABLED);   
            EnableMenuItem((HMENU)wparam, ID_EDIT_LISTFUNCTIONANDROUTINEPARAMETERS, MF_ENABLED);   
        }
#endif

    }

    return wyTrue;
}

/*
-This function gets called when destroy menu
-This used to handle the Table->Engine type menu, should execute query only once when the main menu(table menu) is popped up.
when closing the Table menu clearing the 'string' holds the engine
*/
void
FrameWindow::HandleOnMenuDestroy(HMENU hmenu)
{
	wyWChar			name[SIZE_128];
	MENUITEMINFO	menuinfo;

	if(!hmenu)
	{
		pGlobals->m_menutableengine.Clear();
		return;
	}
	
	menuinfo.cbSize = sizeof(MENUITEMINFO);
	menuinfo.fMask = MIIM_STRING;
	menuinfo.cch = SIZE_128;
	menuinfo.dwTypeData = name;
	GetMenuItemInfo(hmenu, 0, TRUE, &menuinfo);

	if(!wcsicmp(name, _(L"&Paste SQl Statement")))
	{
		pGlobals->m_menutableengine.Clear();
	}
}

void 
FrameWindow::HandleMenuOnNoConnection(WPARAM wparam, LPARAM lparam)
{
//    DEBUG_ENTER("HandleMenuOnNoConnection");
		
    if((LOWORD(lparam))== MNUOBJ_INDEX - 1)
	    /* if it is objects menu while there is no connection*/
	    EnableColumnItems((HMENU)wparam);
    else if((LOWORD(lparam)) == MNUDB_INDEX - 1 ||(LOWORD(lparam))== MNUTBL_INDEX - 1)
    {
	    EnableMenuItem((HMENU)wparam, ID_TABLE_MAKER, MF_GRAYED | MF_BYCOMMAND);
		EnableMenuItem((HMENU)wparam, ID_OBJECT_CREATETRIGGER , MF_GRAYED | MF_BYCOMMAND);

		HandleDBMenuOptionsPRO((HMENU)wparam);      
    }

	else if((LOWORD(lparam)) == MNUFILE_INDEX - 1) 
		HandleFileMenuOptionsProEnt((HMENU)wparam);

    else if((LOWORD(lparam)) == MNUEDIT_INDEX - 1)
	    ChangeEditMenuItem((HMENU)wparam);
	
    else if((LOWORD(lparam)) == MNUFAV_INDEX - 1)
	    EnableFavoriteMenu((HMENU)wparam);
		
    else if((LOWORD(lparam))== MNUENGINE_INDEX)
    {
        wyInt32 count, menucount = GetMenuItemCount((HMENU)wparam);

        for(count = 0; count < menucount; count++)
        {
               EnableMenuItem((HMENU)wparam, count, MF_BYPOSITION | MF_GRAYED);
        }
        
    }

    else if((LOWORD(lparam))== MNUPTOOL_INDEX - 1)
    {
		if(m_connection->m_enttype == ENT_PRO)
			HandlePowertoolsOptionsPRO((HMENU)wparam);

      //  EnableMenuItem((HMENU)wparam, MNU_QUERYBUILDER, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem((HMENU)wparam, MNU_SCHEMADESIGNER, MF_BYPOSITION | MF_GRAYED);
        EnableMenuItem((HMENU)wparam, MNU_REBUILDTAGS, MF_BYPOSITION | MF_GRAYED);

        if(m_connection->m_enttype != ENT_ULTIMATE && m_connection->m_enttype != ENT_TRIAL)
        {
            RemoveMenu((HMENU)wparam, ID_VDDTOOL, MF_BYCOMMAND);
        }
	}
	else if((LOWORD(lparam)) == MNUTRANSACTION_INDEX - 1)
    {
			wyUInt32 mitems[] ={ID_TRANSACTION_SETAUTOCOMMIT, ID_TRX_REPEATABLEREAD, ID_TRX_READCOMMITED, ID_TRX_READUNCOMMITED,
								ID_TRX_SERIALIZABLE, ID_STARTTRANSACTION_WITHNOMODIFIER, ID_WITHCONSISTENTSNAPSHOT_READONLY,
								ID_WITHCONSISTENTSNAPSHOT_READWRITE, ID_STARTTRANSACTION_READONLY, ID_STARTTRANSACTION_READWRITE,
								ID_COMMIT_WITHNOMODIFIER, ID_COMMIT_ANDCHAIN, ID_COMMIT_ANDNOCHAIN, ID_COMMIT_RELEASE,
								ID_COMMIT_NORELEASE, ID_ROLLBACK_TOSAVEPOINT, ID_ROLLBACK_TRANSACTION, ID_ROLLBACK_ANDCHAIN,
								ID_ROLLBACK_ANDNOCHAIN, ID_ROLLBACK_RELEASE, ID_ROLLBACK_NORELEASE, ID_SAVEPOINT_CREATESAVEPOINT,
								ID_SAVEPOINT_RELEASESAVEPOINT};
			for(int i = 0; mitems[i] != NULL; i++) 
				EnableMenuItem((HMENU)wparam, mitems[i], MF_GRAYED);
	}
    return;
}

wyInt32 
FrameWindow::ONWmMainWinNotify(HWND hwnd, LPARAM lparam, WPARAM wparam)
{
	LPNMHDR			lpnmhdr =(LPNMHDR)lparam;
	HWND            hwndwindow, hwndtemp;
	MDIWindow       *wnd,*tempwnd = NULL;
	MDIWindow		*mdiwndann = NULL;
    HMENU           hmenu, htrackmenu;
    POINT           pnt;
	
    if(((LPNMTTDISPINFO)lparam)->hdr.code == TTN_GETDISPINFO)
	{
		OnToolTipInfo((LPNMTTDISPINFO)lparam);
	}

	else if(((LPNMTOOLBAR)lparam)->hdr.code == TBN_DROPDOWN)
	{
		OnTBDropDown((NMTOOLBAR*)lparam);
		return TBDDRET_DEFAULT;
	}

	switch(lpnmhdr->code)
	{
		case CTCN_TABCLOSING:
			if(lpnmhdr->idFrom == IDC_CONNECTIONTAB)
			{
				//close the respective connection, on closing a connection tab
				wnd = GetActiveWin();

				if(!wnd)
					return 0;

				//get active tab handle
				hwndwindow = m_conntab->GetActiveWindowHandle(GetDlgItem(hwnd, lpnmhdr->idFrom));

				if(wnd->m_hwnd == hwndwindow)
				{
					//check if tab is alredy closed by MDI window close
					if(wnd->m_iswinclosed == wyFalse)
					{
                        wnd->m_iswinclosed = wyTrue;
						SendMessage(wnd->m_hwnd, WM_CLOSE, 0, 0);
					}
					else
					{
						m_closetab = wyTrue;
						return 1;
					}
				}
				else
				{
					return 0;
				}
			}
			break;

		case CTCN_TABCLOSED:
			return 1;

		case CTCN_SELCHANGING:
            return 1;
			break;

		case CTCN_SELCHANGE:
			//Set the flag of custom WM_MDINEXT message to true
			pGlobals->m_iscustomwmnext  = wyTrue;

			//on changing a tab, check the handle and focus on the selected window
			hwndwindow = m_conntab->GetActiveWindowHandle(GetDlgItem(hwnd, lpnmhdr->idFrom));
                        
            if(hwndwindow)
			{
				hwndtemp = hwndwindow;
				
				/*to avoid flickering/restoring while switching between connections, 
				WM_MDINEXT message will be send, where wparam is 1. But in case of creating  
				or deleting a connection WM_MDIACTIVATE will be send where wparam is 0*/
				if(wparam)
				{
					tempwnd = GetActiveWin();
					EnumChildWindows(GetActiveWin()->m_hwndparent, FrameWindow::EnumMDIChildren, (LPARAM)&hwndtemp);
					hwndtemp && SendMessage(GetActiveWin()->m_hwndparent, WM_MDINEXT, (WPARAM)hwndtemp, (LPARAM)0);
				}
				else
				{
					tempwnd = GetActiveWin();
					mdiwndann = (MDIWindow*)tempwnd;

				    SendMessage(GetActiveWin()->m_hwndparent, WM_MDIACTIVATE, (WPARAM)hwndwindow, 0);
					if(mdiwndann && pGlobals->m_isannouncementopen) 
						mdiwndann->m_isanncreate = wyTrue;
				}

				//Set the flag of custom WM_MDINEXT message to false
				pGlobals->m_iscustomwmnext  = wyFalse;
				return 0;
			}

			//Set the flag of custom WM_MDINEXT message to false
			pGlobals->m_iscustomwmnext  = wyFalse;

			break;

		/*case CTCN_WMDESTROY:
			break;	*/

        case CTCN_PLUSBUTTONCLICK:
            if(lpnmhdr->idFrom == IDC_CONNECTIONTAB)
            {
                LoadConnTabPlusMenu(lparam);
            }
            break;

		case CTCN_LBUTTONDBLCLK:
			if(lpnmhdr->idFrom == IDC_CONNECTIONTAB)
			{
				//if double clicked on empty space , then open the connection window
				CreateConnDialog();
			}
			break;

        case CTCN_ONCONTEXTMENU:
            if(lpnmhdr->idFrom == IDC_CONNECTIONTAB)
            {
                hmenu = LoadMenu(pGlobals->m_hinstance, MAKEINTRESOURCE(IDR_FIXTABMENU));
                LocalizeMenu(hmenu);
	            htrackmenu = GetSubMenu(hmenu, 0);
                wyTheme::SetMenuItemOwnerDraw(htrackmenu);
                GetCursorPos(&pnt);
			    TrackPopupMenu(htrackmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt.x, pnt.y, 0, hwnd, NULL);
                FreeMenuOwnerDrawItem(htrackmenu);
                DestroyMenu(hmenu);
            }
            break;

        case CTCN_PAINTTIMERSTART:
            CustomTab_SetBufferedDrawing(wyTrue);
            break;

        case CTCN_PAINTTIMEREND:
            CustomTab_SetBufferedDrawing(wyFalse);
            break;
	}

    return 1;
}

void
FrameWindow::HandleToolCombo(WPARAM wparam)
{
    switch(HIWORD(wparam))
	{
	case CBN_DROPDOWN:
		OnToolComboDropDown();
		break;

	case CBN_SELCHANGE:
		PostMessage(pGlobals->m_pcmainwin->m_hwndmain, UM_COMBOCHANGE, 0, 0);
		break;

	case CBN_SELENDCANCEL:
		SetOldDB();
		break;
	}

    return;
}

void 
FrameWindow::HandleExecuteCurrentQuery(HWND hwndactive, MDIWindow *pcquerywnd, EditorBase *pceditorbase, WPARAM wparam)
{
	wyBool		isedit = wyFalse;
	TabEditor	*pctabeditor = NULL;

	//if it is execute & edit result set
	if((LOWORD(wparam) == ACCEL_QUERYUPDATE) ||(LOWORD(wparam) == ACCEL_QUERYUPDATE_KEY))
		isedit = wyTrue;

	//Check whether the tab is QueryBuilder or Schema designer or Advance editor and execute & edit result set 
	if((pcquerywnd->m_pctabmodule->GetActiveTabEditor() == NULL) ||(pceditorbase->GetAdvancedEditor() == wyTrue && isedit == wyTrue))
		return;

	pctabeditor = (TabEditor*)pcquerywnd->m_pctabmodule->GetActiveTabEditor();

	if(!pctabeditor->m_pctabmgmt || !pctabeditor->m_pctabmgmt->m_hwnd)
		return;

	if(hwndactive)
    {
		//SendMessage(pctabeditor->m_pctabmgmt->m_hwnd, WM_SETREDRAW, FALSE, 0);
		
		/* if a query is being executed then we need to interpret it as stop query */
		if(pcquerywnd->m_executing == wyTrue && pcquerywnd->IsStopQueryVariableReset() == wyTrue)
        {
			if(StopQuery(hwndactive, pcquerywnd) == wyFalse)
			{
				SendMessage(pctabeditor->m_pctabmgmt->m_hwnd, WM_SETREDRAW, TRUE, 0);
                return;
			}
        }
		else if(pcquerywnd->IsStopQueryVariableReset() == wyTrue) 
		{
			pceditorbase->ExecuteCurrentQuery(&pcquerywnd->m_stopquery, isedit);
		}

		/*OnQueryExecFinish(pcquerywnd);
		SendMessage(pctabeditor->m_pctabmgmt->m_hwnd, WM_SETREDRAW, TRUE, 0);*/
	}

    return;
}

void 
FrameWindow::HandleExecuteExplain(HWND hwndactive, MDIWindow *pcquerywnd, EditorBase *pceditorbase, wyBool isExtended)
{
    TabEditor   *pctabeditor = NULL;

    pctabeditor = (TabEditor *)pcquerywnd->m_pctabmodule->GetActiveTabEditor();

    if(hwndactive)
    {
        if(pcquerywnd->m_executing == wyTrue && pcquerywnd->IsStopQueryVariableReset() == wyTrue)
        {
			if(StopQuery(hwndactive, pcquerywnd) == wyFalse)
			{
				SendMessage(pctabeditor->m_pctabmgmt->m_hwnd, WM_SETREDRAW, TRUE, 0);
                return;
			}
        }
		else if(pcquerywnd->IsStopQueryVariableReset() == wyTrue) 
		{
			pceditorbase->ExecuteExplainQuery(&pcquerywnd->m_stopquery, isExtended);
		}

		OnQueryExecFinish(pcquerywnd);
		SendMessage(pctabeditor->m_pctabmgmt->m_hwnd, WM_SETREDRAW, TRUE, 0);
	}
    return;
}

void 
FrameWindow::HandleExecuteAllQuery(HWND hwndactive, MDIWindow *pcquerywnd, EditorBase *pceditorbase)
{
	TabEditor	*pctabeditor = NULL;

	if(!pcquerywnd->m_pctabmodule->GetActiveTabEditor())
	{
		return;
	}
	
	pctabeditor = (TabEditor*)pcquerywnd->m_pctabmodule->GetActiveTabEditor();
    
	if(!pctabeditor->m_pctabmgmt || !pctabeditor->m_pctabmgmt->m_hwnd)
	{
		return;
	}

	if(hwndactive)
    {
		//SendMessage(pctabeditor->m_pctabmgmt->m_hwnd, WM_SETREDRAW, FALSE, 0);

		/* if a query is being executed then we need to interpret it as stop query */
		if(pcquerywnd->m_executing == wyTrue && pcquerywnd->IsStopQueryVariableReset() == wyTrue)
        {
			if(StopQuery(hwndactive, pcquerywnd) == wyFalse)
			{
				SendMessage(pctabeditor->m_pctabmgmt->m_hwnd, WM_SETREDRAW, TRUE, 0);
                return;
			}
        }
		else if(pcquerywnd->IsStopQueryVariableReset() == wyTrue) 
		{
			pceditorbase->ExecuteAllQuery(&pcquerywnd->m_stopquery);
		}

		//SendMessage(pctabeditor->m_pctabmgmt->m_hwnd, WM_SETREDRAW, TRUE, 0);
	}
    return;
}
void 
FrameWindow::HandleExecuteSelQuery(HWND hwndactive, MDIWindow *pcquerywnd, EditorBase *pceditorbase)
{
	TabEditor	*pctabeditor = NULL;

	if(!pcquerywnd->m_pctabmodule->GetActiveTabEditor())
	{
		return;
	}
	
	pctabeditor = (TabEditor*)pcquerywnd->m_pctabmodule->GetActiveTabEditor();
    
	if(!pctabeditor->m_pctabmgmt || !pctabeditor->m_pctabmgmt->m_hwnd)
	{
		return;
	}

	if(hwndactive)
	{
		//SendMessage(pctabeditor->m_pctabmgmt->m_hwnd, WM_SETREDRAW, FALSE, 0);

		if(pcquerywnd->m_executing == wyTrue && pcquerywnd->IsStopQueryVariableReset() == wyTrue)
        {
			if(StopQuery(hwndactive, pcquerywnd) == wyFalse)
			{
				SendMessage(pctabeditor->m_pctabmgmt->m_hwnd, WM_SETREDRAW, TRUE, 0);
                return;
			}
        }

        else if(pcquerywnd->IsStopQueryVariableReset() == wyTrue) 
		{
		    pceditorbase->ExecuteSelQuery(&pcquerywnd->m_stopquery);
		}

        OnQueryExecFinish(pcquerywnd);

		SendMessage(pctabeditor->m_pctabmgmt->m_hwnd, WM_SETREDRAW, TRUE, 0);
	}

	return;
}

HTREEITEM
FrameWindow ::GetTreeItem(MDIWindow *wnd, wyString& hitemname)
{
    EditorBase		*peditorbase	= NULL; //wnd->GetActiveTabEditor()->m_peditorbase;
    HTREEITEM       dbhitem = NULL, folderhitem = NULL, objhitem = NULL;
    wyWChar         database[SIZE_512] = {0}, object[SIZE_512] = {0};
    wyString        objname, objname2;
    wyInt32         image, tabimageid, dbfound = 0;

    if(!wnd && wnd->GetActiveTabEditor() == NULL)
    {
        return NULL;
    }

    peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
    dbhitem = TreeView_GetChild(wnd->m_pcqueryobject->m_hwnd, TreeView_GetRoot(wnd->m_pcqueryobject->m_hwnd));

    while(dbhitem)
    {
        GetNodeText(wnd->m_pcqueryobject->m_hwnd, dbhitem, database, SIZE_512 - 1);
        objname.SetAs(database);
        if(!peditorbase->m_dbname.CompareI(objname))
        {
            dbfound = 1;
            break;
        }
        dbhitem = TreeView_GetNextSibling(wnd->m_pcqueryobject->m_hwnd, dbhitem);
    }
    
    if(!dbfound)
        return TreeView_GetRoot(wnd->m_pcqueryobject->m_hwnd);
    
    TreeView_Expand(wnd->m_pcqueryobject->m_hwnd, dbhitem, TVM_EXPAND);
    tabimageid = wnd->m_pctabmodule->GetActiveTabImage();
    
    switch(tabimageid)
    {
    case IDI_CREATEPROCEDURE:
        peditorbase->m_nodeimage = NSP;
        break;
    case IDI_CREATEVIEW:
        peditorbase->m_nodeimage = NVIEWS;
        break;
    case IDI_CREATETRIGGER:
        peditorbase->m_nodeimage = NTRIGGER;
        break;
    case IDI_CREATEFUNCTION:
        peditorbase->m_nodeimage = NFUNC;
        break;
    case IDI_CREATEEVENT:
        peditorbase->m_nodeimage = NEVENTS;
        break;
    }

    folderhitem = TreeView_GetChild(wnd->m_pcqueryobject->m_hwnd, dbhitem);

    while(folderhitem)
    {
        image = GetItemImage(wnd->m_pcqueryobject->m_hwnd, folderhitem);
        if(image == peditorbase->m_nodeimage)
            break;
        folderhitem = TreeView_GetNextSibling(wnd->m_pcqueryobject->m_hwnd, folderhitem);
    }

    objname.SetAs(hitemname);
    if(objname.GetLength() == 0)
        return folderhitem;

    TreeView_Expand(wnd->m_pcqueryobject->m_hwnd, folderhitem, TVM_EXPAND);
    
    objhitem = TreeView_GetChild(wnd->m_pcqueryobject->m_hwnd, folderhitem);
    while(objhitem)
    {
        GetNodeText(wnd->m_pcqueryobject->m_hwnd, objhitem, object, SIZE_512 - 1);
        objname2.SetAs(object);
        if(!objname.CompareI(objname2))
            return objhitem;
        objhitem = TreeView_GetNextSibling(wnd->m_pcqueryobject->m_hwnd, objhitem);
    }
       
    return folderhitem;
}

void 
FrameWindow::OnQueryExecFinish(MDIWindow *pcquerywnd)
{
	EditorBase		*peditorbase	= pcquerywnd->GetActiveTabEditor()->m_peditorbase;
	TabMgmt			*ptabmgmt		= pcquerywnd->GetActiveTabEditor()->m_pctabmgmt;	
	HWND			hwndfocus;

  	if(peditorbase->GetAdvancedEditor())
    {
        //peditorbase->m_hitem = GetTreeItem(pcquerywnd);
        pGlobals->m_pcmainwin->Refresh(pcquerywnd);
        //peditorbase->m_hitemname
        peditorbase->m_hitem = GetTreeItem(pcquerywnd, peditorbase->m_hitemname);
		TreeView_SelectItem(pcquerywnd->m_pcqueryobject->m_hwnd, peditorbase->m_hitem);
		SetFocus(pcquerywnd->m_pcqueryobject->m_hwnd);
		
        //pGlobals->m_pcmainwin->Refresh(pcquerywnd);

		
		//if IsEditorFocus() is true or query execution is not successful then we are setting focus to editor
		if(IsEditorFocus() == wyTrue || pcquerywnd->m_querysuccessful != 0 )
		{
			if(pcquerywnd == GetActiveWin())
				PostMessage(pGlobals->m_pcmainwin->m_hwndmain, UM_SETFOCUS, (WPARAM)peditorbase->m_hwnd, 0);
			else
				pcquerywnd->m_lastfocus = peditorbase->m_hwnd;
		}		
	}
    else
    { 
	   if(IsEditorFocus() == wyFalse)
		{	
            hwndfocus = peditorbase->m_hwnd;

            if(ptabmgmt->m_presultview)
            {
                hwndfocus = ptabmgmt->m_presultview->GetActiveDispWindow();
            }
			
			if(pcquerywnd == GetActiveWin())
				PostMessage(pGlobals->m_pcmainwin->m_hwndmain, UM_SETFOCUS, (WPARAM)hwndfocus, 0); 
			else
				pcquerywnd->m_lastfocus = hwndfocus;
		}
	}
    return;
}
// move focus to next or previous tab based on key action.
void
FrameWindow::HandleTabNavigation(MDIWindow *pcquerywnd, wyBool movetoup)
{
	wyInt32  itemindex, itemcount, index;

	itemcount= CustomTab_GetItemCount(pcquerywnd->m_pctabmodule->m_hwnd);

	//if  tabs count equal to one.
	if(itemcount == 1)
		return;

	itemindex = CustomTab_GetCurSel(pcquerywnd->m_pctabmodule->m_hwnd); 
    
	//move to next tab
	if(movetoup == wyTrue)
	{
		if(itemcount  == itemindex + 1)
			index = 0;
		else
			index = itemindex + 1;
	}
	//move to previous tab
	else
	{
		if(itemindex != 0)
			index = itemindex - 1;
		else
			index = itemcount - 1;
			
	}

	//POst 8.04 Beta 2 to avoid flicker on Tab Navigation
	CustomTab_SetCurSel(pcquerywnd->m_pctabmodule->m_hwnd, index);

	CustomTab_EnsureVisible(pcquerywnd->m_pctabmodule->m_hwnd, index); 	
}

void 
FrameWindow::DeleteQueryTabItem(MDIWindow *pcquerywnd)
{
	wyInt32  itemindex;

	if(CustomTab_GetItemCount(pcquerywnd->m_pctabmodule->m_hwnd) <= 1)
		return;	

	itemindex = CustomTab_GetCurSel(pcquerywnd->m_pctabmodule->m_hwnd);    
	CustomTab_DeleteItem(pcquerywnd->m_pctabmodule->m_hwnd, itemindex);	
}

void 
FrameWindow::CreateNewTableDataTab(MDIWindow *pcquerywnd, wyBool isnewtab)
{
	TabModule	*ptabmodule  = pcquerywnd->m_pctabmodule;

    //CustomTab_SetBufferedDrawing(wyTrue);
	
    ptabmodule->CreateTabDataTab(pcquerywnd, isnewtab, wyTrue); 

	//pcquerywnd->SetQueryWindowTitle();
	//CustomTab_SetBufferedDrawing(wyFalse);		
	return;
}


void 
FrameWindow::CreateNewQueryEditor(MDIWindow *pcquerywnd)
{
	EditorBase	*peditorbase; 
	TabModule	*ptabmodule  = pcquerywnd->m_pctabmodule;

    //CustomTab_SetBufferedDrawing(wyTrue);
	ptabmodule->CreateQueryEditorTab(pcquerywnd); 
	peditorbase = pcquerywnd->GetActiveTabEditor()->m_peditorbase;
	
	SetFocus(peditorbase->m_hwnd);
	pcquerywnd->SetQueryWindowTitle();
	//CustomTab_SetBufferedDrawing(wyFalse);		
	return;
}


void 
FrameWindow::CreateNewQueryBuilder(MDIWindow *pcquerywnd)
{
	TabModule	*ptabmodule  = pcquerywnd->m_pctabmodule;

    //CustomTab_SetBufferedDrawing(wyTrue);
	ptabmodule->CreateQueryBuilderTab(pcquerywnd);
    //CustomTab_SetBufferedDrawing(wyFalse);

	return;
}
//Function for Calling Refresh or Excute query functions based on Switching F5 and F9 functionalities
void 
FrameWindow::HandleOnRefreshExecuteQuery(HWND hwndactive, MDIWindow *pcquerywnd, WPARAM wparam, wyBool isrefresh)
{
	wyBool				issd		= wyFalse;
	TabEditor			*ptabeditor = NULL;
	EditorBase			*peditorbase = NULL;
	wyInt32				 tabimageid = 0;
	
	tabimageid = pcquerywnd->m_pctabmodule->GetActiveTabImage();
	
	if(tabimageid != IDI_DATASEARCH)
	{
	ptabeditor = pcquerywnd->GetActiveTabEditor();
	}
    
	if(!hwndactive)
		return;

#ifndef COMMUNITY
	TabSchemaDesigner	*ptabsd = NULL;		
	HWND				  hwndfocus	= NULL;

	tabimageid = pcquerywnd->m_pctabmodule->GetActiveTabImage();
	if(tabimageid == IDI_SCHEMADESIGNER_16)
	{
		ptabsd = (TabSchemaDesigner*)pcquerywnd->m_pctabmodule->GetActiveTabType();
		if(ptabsd && pcquerywnd->m_pcqueryobject && pcquerywnd->m_pcqueryobject->m_hwnd)
		{
			VERIFY(hwndfocus = GetFocus());
			if(hwndfocus != pcquerywnd->m_pcqueryobject->m_hwnd)
				issd = wyTrue;
		}
	}
#endif
	
	if(pGlobals->m_isrefreshkeychange == wyFalse)//if refresh key option switches from f9 to f5
	{
		if(isrefresh == wyTrue)//flag using for differentiating ACCEL_REFRESH,ACCEL_EXECUTE cases
		{	
			if(!ptabeditor)
				return;
			peditorbase = ptabeditor->m_peditorbase;
			HandleExecuteCurrentQuery(hwndactive, pcquerywnd, peditorbase, wparam);	
		}		
		else
		{			
			/// For refreshing the SD- canvas if focus is on that window
			if(issd == wyTrue)	
			{
#ifndef COMMUNITY
				ptabsd->CanvasRefresh();							
#endif
			}
			
			else
				HandleOnRefresh(pcquerywnd);
		}
	}
	else
	{
		if(isrefresh == wyTrue)
		{
			if(issd == wyTrue)	
			{
#ifndef COMMUNITY
				ptabsd->CanvasRefresh();
#endif
			}

			else
				HandleOnRefresh(pcquerywnd);		
		}

		else 
		{
			if(!ptabeditor)
				return;
			peditorbase = ptabeditor->m_peditorbase;
			HandleExecuteCurrentQuery(hwndactive, pcquerywnd, peditorbase, wparam);		
		}
	}
}

void 
FrameWindow::HandleOnRefresh(MDIWindow *pcquerywnd)
{
	if(pcquerywnd->m_pcqueryobject->GetSelectionImage() == NSERVER)
	{
		pcquerywnd->m_pcqueryobject->RefreshObjectBrowser(pcquerywnd->m_tunnel, &pcquerywnd->m_mysql,pcquerywnd);
	}
	else
	{
		/*	Refresh Parent node only, leaving others unchanged, So we can
			eliminate the flickering little more */
		pcquerywnd->m_pcqueryobject->RefreshParentNodeHelper();
	}

	SetFocus(pcquerywnd->m_pcqueryobject->m_hwnd);

    return;
}

void
FrameWindow::HandleOnCollapse(MDIWindow *pcquerywnd)
{
	pcquerywnd->m_pcqueryobject->CollapseObjectBrowser(pcquerywnd->m_pcqueryobject->m_hwnd);
	SetFocus(pcquerywnd->m_pcqueryobject->m_hwnd);
	return;
}

wyInt32 
FrameWindow::OnWmSize(WPARAM wparam)
{
	/* no need to handle SIZE_MINIMIZED */
	if(wparam == SIZE_MINIMIZED)
		return 0;

	Resize(wparam);

	//Repaint tha tabs
	if(m_connection)
		m_connection->RepaintTabs(wparam);
	/*if(pGlobals->m_pcquerywnd)
		pGlobals->m_pcquerywnd->m_isanncreate = wyTrue;*/

	return 0;
}
 
void 
FrameWindow::GoToLine(HWND hwndactive, MDIWindow *wnd)
{
    HWND hedit;

    if(!wnd->GetActiveTabEditor())
        return;

    hedit = wnd->GetActiveTabEditor()->m_peditorbase->GetHWND();	

    int ret;
    if(GetFocus() == hedit)
    {
        wnd->m_lastfocus = hedit;
        ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_DBMAKER), 
	                            hwndactive, FrameWindow::CreateObjectDlgProc,(LPARAM)"GoToLine");
    }
    return;
}

wyBool 
FrameWindow::OnExportData(MDIWindow *wnd)
{
	CExportResultSet    cer;
    MySQLDataEx         *ptrp;

	ptrp = wnd->GetActiveTabEditor()->m_pctabmgmt->GetResultData();    
    
	// if the user has pressed the keypboard shortcut bogusly then we
	// exit.
	if(!ptrp)
		return wyFalse;

	cer.Export(wnd->m_hwnd, ptrp);

    return wyTrue;
}

void 
FrameWindow::OnCreateDatabase(HWND hwndactive, MDIWindow *wnd)
{
	wyWChar     *dbname;
	wyInt64		ret;
#ifndef COMMUNITY
	if(GetActiveWin()  && GetActiveWin()->m_conninfo.m_isreadonly == wyTrue)
	{
		return;
	}
#endif

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_CREATEDB), 
			hwndactive, FrameWindow::CreateObjectDlgProc,(LPARAM)"CreateDB" );
    
	if(ret)
    {        
		dbname = (wyWChar*)ret;

		if(wcslen(dbname) == 0)
            return;
		
        SendMessage(wnd->m_pcqueryobject->m_hwnd, WM_SETREDRAW, FALSE, 0);
        wnd->m_pcqueryobject->GetTreeState();
		wnd->m_pcqueryobject->RefreshObjectBrowser(wnd->m_tunnel, &wnd->m_mysql,wnd);				
        wnd->m_pcqueryobject->RestoreTreeState();
        wnd->m_pcqueryobject->m_seltype = NDATABASE;
		wnd->m_pcqueryobject->m_seldatabase.SetAs(dbname);
        wnd->m_pcqueryobject->m_seltable.Strip(wnd->m_pcqueryobject->m_seldatabase.GetLength());
		wnd->m_pcqueryobject->ExpandDatabase();
		SendMessage(wnd->m_pcqueryobject->m_hwnd, WM_SETREDRAW, TRUE, 0);
		free(dbname);
	}
    return;
}

void 
FrameWindow::OnCreateFunction(HWND hwndactive, MDIWindow *wnd)
{
	wyWChar     *functionname;
	wyInt64     ret;
	HTREEITEM	hfunctionitem;
    wyString    buff;
	EditorBase	*peditorbase;
	wyString	functionnamestr;

	hfunctionitem = wnd->m_pcqueryobject->GetDatabaseNode();

	if(hfunctionitem)
		hfunctionitem = wnd->m_pcqueryobject->GetNode(hfunctionitem, NFUNC);

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_DBMAKER), 
			hwndactive, FrameWindow::CreateObjectDlgProc,(LPARAM)"CreateFunction");

	if(ret)
		functionname = (wyWChar*)ret;
    else
		return;
	
	if(wcslen(functionname) == 0)
        return;

	VERIFY(SetCursor(LoadCursor(NULL, IDC_WAIT)));

    functionnamestr.SetAs(functionname);

	//to trim empty spaces from right, to avoid mysql errors
	functionnamestr.RTrim();

	wnd->m_pctabmodule->CreateAdvEditorTab(wnd, (wyChar *)functionnamestr.GetString(), IDI_CREATEFUNCTION, hfunctionitem, &functionnamestr);
	
	peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
    peditorbase->m_isdiscardchange = wyTrue;
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITEROPEN);

	pGlobals->m_pcmainwin->PrepareCreateFunction(wnd, functionnamestr.GetString(), buff);
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)buff.GetString());

	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITERCLOSE);
    SendMessage(peditorbase->m_hwnd, SCI_SETSAVEPOINT, 0, 0);
    SendMessage(peditorbase->m_hwnd, SCI_EMPTYUNDOBUFFER, 0, 0);
    peditorbase->m_isdiscardchange = wyFalse;
	EditorFont::SetLineNumberWidth(peditorbase->m_hwnd);
	PostMessage(pGlobals->m_pcmainwin->m_hwndmain, UM_SETFOCUS,(WPARAM)peditorbase->m_hwnd,0);
	
	//wnd->SetQueryWindowTitle();
	
	pGlobals->m_pcmainwin->HandleGoTo(pGlobals->m_pcmainwin->m_hwndmain, wnd, L"8"); // set focus on 8th line

	free(functionname);

	VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));

    return;
}

void 
FrameWindow::OnCreateProcedure(HWND hwndactive, MDIWindow *wnd)
{
	wyWChar     *procedurename;
	wyInt64     ret;
	HTREEITEM	hprocedureitem;
    wyString    buff;
	EditorBase	*peditorbase;
	wyString	procedurenamestr;

	hprocedureitem = wnd->m_pcqueryobject->GetDatabaseNode();

	if(TreeView_GetItemState(wnd->m_pcqueryobject->m_hwnd, hprocedureitem, TVIS_EXPANDEDONCE) & TVIS_EXPANDEDONCE)
	{
		hprocedureitem = wnd->m_pcqueryobject->GetNode(hprocedureitem, NSP);
	}

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_DBMAKER), 
			hwndactive, FrameWindow::CreateObjectDlgProc,(LPARAM)"CreateProcedure");

	if(ret)
		procedurename = (wyWChar*)ret;
	else
		return;

	if(wcslen(procedurename)== 0)
        return;

	VERIFY(SetCursor(LoadCursor(NULL, IDC_WAIT)));
	
	procedurenamestr.SetAs(procedurename);

	//to trim empty spaces from right, to avoid mysql errors
	procedurenamestr.RTrim();

	wnd->m_pctabmodule->CreateAdvEditorTab(wnd, (wyChar *)procedurenamestr.GetString(), IDI_CREATEPROCEDURE, hprocedureitem, &procedurenamestr);
	
	peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
	peditorbase->m_isdiscardchange = wyTrue;
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITEROPEN);
	
	pGlobals->m_pcmainwin->PrepareCreateProcedure(wnd, procedurenamestr.GetString(), buff);
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)buff.GetString());

	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITERCLOSE);
    SendMessage(peditorbase->m_hwnd, SCI_SETSAVEPOINT, 0, 0);
    SendMessage(peditorbase->m_hwnd, SCI_EMPTYUNDOBUFFER, 0, 0);
    peditorbase->m_isdiscardchange = wyFalse;
	EditorFont::SetLineNumberWidth(peditorbase->m_hwnd);
	PostMessage(pGlobals->m_pcmainwin->m_hwndmain, UM_SETFOCUS,(WPARAM)peditorbase->m_hwnd,0);

	//wnd->SetQueryWindowTitle();
	pGlobals->m_pcmainwin->HandleGoTo(pGlobals->m_pcmainwin->m_hwndmain, wnd, L"7"); // set focus on 7th line
	free(procedurename);

	VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));

    return;
}
void 
FrameWindow::OnCreateEvent(HWND hwndactive, MDIWindow *wnd)
{
	wyWChar     *eventname;
	wyInt64     ret;
	HTREEITEM   heventitem;
    wyString    buff;
	EditorBase	*peditorbase;
	wyString	eventnamestr;

	heventitem = wnd->m_pcqueryobject->GetDatabaseNode();

	if(heventitem)
		heventitem = wnd->m_pcqueryobject->GetNode(heventitem, NEVENTS);
    

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_DBMAKER), 
			hwndactive, FrameWindow::CreateObjectDlgProc,(LPARAM)"CreateEvent");

	if(ret)
		eventname = (wyWChar*)ret;
	else
		return;

	if(wcslen(eventname) == 0)
        return;

	VERIFY(SetCursor(LoadCursor(NULL, IDC_WAIT)));

	eventnamestr.SetAs(eventname);

	//to trim empty spaces from right, to avoid mysql errors
	eventnamestr.RTrim();

	wnd->m_pctabmodule->CreateAdvEditorTab(wnd, (wyChar *)eventnamestr.GetString(), IDI_CREATEEVENT, heventitem, &eventnamestr);
	
    peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
    peditorbase->m_isdiscardchange = wyTrue;
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITEROPEN);
	pGlobals->m_pcmainwin->PrepareCreateEvent(wnd, eventnamestr.GetString(), buff);
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)buff.GetString());
    SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITERCLOSE);
    SendMessage(peditorbase->m_hwnd, SCI_SETSAVEPOINT, 0, 0);
    SendMessage(peditorbase->m_hwnd, SCI_EMPTYUNDOBUFFER, 0, 0);
    peditorbase->m_isdiscardchange = wyFalse;
	EditorFont::SetLineNumberWidth(peditorbase->m_hwnd);
	PostMessage(pGlobals->m_pcmainwin->m_hwndmain, UM_SETFOCUS,(WPARAM)peditorbase->m_hwnd, 0);

	//wnd->SetQueryWindowTitle();

	pGlobals->m_pcmainwin->HandleGoTo(pGlobals->m_pcmainwin->m_hwndmain, wnd, L"7"); // set focus on 7th line
	
	free(eventname);

	VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));

    return;

}

void 
FrameWindow::OnCreateView(HWND hwndactive, MDIWindow *wnd, wyString *qbquery)
{
	wyWChar     *viewname;
	wyInt64      ret;
	HTREEITEM   hviewitem;
    wyString    buff;
	EditorBase	*peditorbase;
	wyString	viewnamestr;

	hviewitem = wnd->m_pcqueryobject->GetDatabaseNode();

	if(TreeView_GetItemState(wnd->m_pcqueryobject->m_hwnd, hviewitem, TVIS_EXPANDEDONCE) & TVIS_EXPANDEDONCE)
	{
		hviewitem = wnd->m_pcqueryobject->GetNode(hviewitem, NVIEWS);
	}

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_DBMAKER), 
			hwndactive, FrameWindow::CreateObjectDlgProc,(LPARAM)"CreateView");

	if(ret)
		viewname = (wyWChar*)ret;
	else
		return;

	if(wcslen(viewname) == 0)
        return;

	VERIFY(SetCursor(LoadCursor(NULL, IDC_WAIT)));

	viewnamestr.SetAs(viewname);

	//to trim empty spaces from right, to avoid mysql errors
	viewnamestr.RTrim();

    wnd->m_pctabmodule->CreateAdvEditorTab(wnd, (wyChar *)viewnamestr.GetString(), IDI_CREATEVIEW, hviewitem, &viewnamestr);
	
    peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
    peditorbase->m_isdiscardchange = wyTrue;
	pGlobals->m_pcmainwin->PrepareCreateView(wnd, viewnamestr.GetString(), buff, qbquery);
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)buff.GetString());
    SendMessage(peditorbase->m_hwnd, SCI_SETSAVEPOINT, 0, 0);
    SendMessage(peditorbase->m_hwnd, SCI_EMPTYUNDOBUFFER, 0, 0);
    peditorbase->m_isdiscardchange = wyFalse;

	EditorFont::SetLineNumberWidth(peditorbase->m_hwnd);
	PostMessage(pGlobals->m_pcmainwin->m_hwndmain, UM_SETFOCUS,(WPARAM)peditorbase->m_hwnd,0);

	//wnd->SetQueryWindowTitle();

	// set focus on line after PrepareCreateView query.
	pGlobals->m_pcmainwin->HandleGoTo(pGlobals->m_pcmainwin->m_hwndmain, wnd, L"8");
	
	free(viewname);

	VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));

    return;
}

void 
FrameWindow::OnCreateTrigger(HWND hwndactive, MDIWindow *wnd)
{
	wyWChar     *triggername;
	wyInt64     ret;
	HTREEITEM	htriggeritem;
    wyString    buff;
	EditorBase	*peditorbase ;
	wyString	triggernamestr;

	//htriggeritem = wnd->m_pcqueryobject->GetTableNode();

    htriggeritem = wnd->m_pcqueryobject->GetDatabaseNode();

    if(TreeView_GetItemState(wnd->m_pcqueryobject->m_hwnd, htriggeritem, TVIS_EXPANDEDONCE) & TVIS_EXPANDEDONCE)
	{
		htriggeritem = wnd->m_pcqueryobject->GetNode(htriggeritem, NTRIGGER);
	}
		
    /*
	if(htriggeritem)
		htriggeritem = wnd->m_pcqueryobject->GetNode(htriggeritem, NTRIGGER);
    */

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_DBMAKER), 
			hwndactive, FrameWindow::CreateObjectDlgProc,(LPARAM)"CreateTrigger");

	if(ret)
		triggername = (wyWChar*)ret;
	else
		return;

	if(wcslen(triggername) == 0)
		return;
	
	VERIFY(SetCursor(LoadCursor(NULL, IDC_WAIT)));

	triggernamestr.SetAs(triggername);

	//to trim empty spaces from right, to avoid mysql errors
	triggernamestr.RTrim();

	wnd->m_pctabmodule->CreateAdvEditorTab(wnd, (wyChar *)triggernamestr.GetString(), IDI_CREATETRIGGER, htriggeritem, &triggernamestr);
	peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
    peditorbase->m_isdiscardchange = wyTrue;         
    SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITEROPEN);
	pGlobals->m_pcmainwin->PrepareCreateTrigger(wnd, triggernamestr.GetString(), buff);

	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)buff.GetString());

	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITERCLOSE);
    SendMessage(peditorbase->m_hwnd, SCI_SETSAVEPOINT, 0, 0);
    SendMessage(peditorbase->m_hwnd, SCI_EMPTYUNDOBUFFER, 0, 0);
    peditorbase->m_isdiscardchange = wyFalse;
	EditorFont::SetLineNumberWidth(peditorbase->m_hwnd);
	PostMessage(pGlobals->m_pcmainwin->m_hwndmain, UM_SETFOCUS,(WPARAM)peditorbase->m_hwnd, 0);

	//wnd->SetQueryWindowTitle();

	pGlobals->m_pcmainwin->HandleGoTo(pGlobals->m_pcmainwin->m_hwndmain, wnd, L"7"); // set focus on 7th line 

	free(triggername);

	VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));

    return;
}

void
FrameWindow::OnAlterDatabase(HWND hwndactive, MDIWindow *wnd)
{
	wyWChar     *dbname;
	wyInt64		ret;
	MYSQL_RES	*res;
    wyString    query;
    
	if(IsMySQL41(wnd->m_tunnel, &wnd->m_mysql) == wyFalse)
		return;

	query.Sprintf("use `%s`", wnd->m_pcqueryobject->m_seldatabase.GetString());
    res = ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query);
	if(!res && wnd->m_tunnel->mysql_affected_rows(wnd->m_mysql)== -1)
	{
		ShowMySQLError(hwndactive, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return ;
	}

	wnd->m_tunnel->mysql_free_result(res);

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_CREATEDB), 
			hwndactive, FrameWindow::CreateObjectDlgProc, (LPARAM)"AlterDB" );

	if(ret)
	{
		dbname = (wyWChar*)ret;

		if(wcslen(dbname) == 0)
            return;

		wnd->m_pcqueryobject->m_seltype = NDATABASE;
		wnd->m_pcqueryobject->m_seldatabase.SetAs(dbname);
		wnd->m_pcqueryobject->m_seltable.Strip(wnd->m_pcqueryobject->m_seldatabase.GetLength());
		free(dbname);
	}
	return;
}
void 
FrameWindow::OnAlterEvent(MDIWindow *wnd)
{
	wyString      altereventstmt, usedb;
	EditorBase	  *peditorbase;
	HTREEITEM     hitem;
	wyBool        ret = wyFalse;
	
	VERIFY(hitem = TreeView_GetSelection(wnd->m_pcqueryobject->m_hwnd));
	// selected event and database name will get from the following fun
	wnd->m_pcqueryobject->GetTableDatabaseName(hitem);	

	VERIFY(SetCursor(LoadCursor(NULL, IDC_WAIT)));
	// query for alter event
	ret = PrepareAlterEvent(wnd, altereventstmt);	

	if(ret == wyFalse)
		return;

	wnd->m_pctabmodule->CreateAdvEditorTab(wnd, (wyChar*)wnd->m_pcqueryobject->m_seltable.GetString()
											, IDI_ALTEREVENT, TreeView_GetParent(wnd->m_pcqueryobject->m_hwnd, hitem));

	peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
    peditorbase->m_isdiscardchange = wyTrue;
	//usedb.Sprintf("USE `%s`$$\r\n\r\n", wnd->m_pcqueryobject->m_seldatabase.GetString());
	
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITEROPEN);	
	//SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)usedb.GetString());
		
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)altereventstmt.GetString());
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITERCLOSE);
    SendMessage(peditorbase->m_hwnd, SCI_SETSAVEPOINT, 0, 0);
    SendMessage(peditorbase->m_hwnd, SCI_EMPTYUNDOBUFFER, 0, 0);
    peditorbase->m_isdiscardchange = wyFalse;
	
	EditorFont::SetLineNumberWidth(peditorbase->m_hwnd);
	
	SetFocus(peditorbase->m_hwnd);

	//wnd->SetQueryWindowTitle();

	VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));   


}
void 
FrameWindow::OnAlterView(MDIWindow *wnd)
{
    wyString viewname, alterview;
	EditorBase	*peditorbase;

	wnd->m_pcqueryobject->GetAlterView(wnd->m_hwnd, wnd->m_tunnel, &wnd->m_mysql, alterview);
	if(alterview.GetLength()== 0)
		return;
	
	pGlobals->m_pcmainwin->GetObjectName(wnd, viewname);
	
	VERIFY(SetCursor(LoadCursor(NULL, IDC_WAIT)));

	//Format the SELECT clause in 'Alter View' statement
	FormatCreateViewStatement(&alterview, viewname.GetString(), wyFalse);

	wnd->m_pctabmodule->CreateAdvEditorTab(wnd,(wyChar*)viewname.GetString(), IDI_ALTERVIEW,
	TreeView_GetParent(wnd->m_pcqueryobject->m_hwnd, 
	TreeView_GetSelection(wnd->m_pcqueryobject->m_hwnd)));
	
	peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
    peditorbase->m_isdiscardchange = wyTrue;
    SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITEROPEN);
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)alterview.GetString());
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITERCLOSE);
    SendMessage(peditorbase->m_hwnd, SCI_SETSAVEPOINT, 0, 0);
    SendMessage(peditorbase->m_hwnd, SCI_EMPTYUNDOBUFFER, 0, 0);
    peditorbase->m_isdiscardchange = wyFalse;
	
	EditorFont::SetLineNumberWidth(peditorbase->m_hwnd);
	
	SetFocus(peditorbase->m_hwnd);
	//wnd->SetQueryWindowTitle();

	VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));

    return;
}

void 
FrameWindow::OnAlterProcedure(MDIWindow *wnd)
{
    wyString createsp, dropsp, spname;
	EditorBase	*peditorbase;

    wnd->m_pcqueryobject->GetCreateProcedure(wnd->m_hwnd, wnd->m_tunnel, &wnd->m_mysql, createsp);

    if(createsp.GetLength()== 0)
	    return;

    wnd->m_pcqueryobject->GetDropProcedure(wnd->m_hwnd, wnd->m_tunnel, &wnd->m_mysql, dropsp);
    pGlobals->m_pcmainwin->GetObjectName(wnd, spname);

	VERIFY(SetCursor(LoadCursor(NULL, IDC_WAIT)));

	wnd->m_pctabmodule->CreateAdvEditorTab(wnd,(wyChar*)spname.GetString(), IDI_ALTERPROCEDURE, 
			TreeView_GetParent(wnd->m_pcqueryobject->m_hwnd, 
			TreeView_GetSelection(wnd->m_pcqueryobject->m_hwnd)));
    peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
    peditorbase->m_isdiscardchange = wyTrue;
    SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITEROPEN);
    SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)dropsp.GetString());
    SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)createsp.GetString());
    SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITERCLOSE);
    SendMessage(peditorbase->m_hwnd, SCI_SETSAVEPOINT, 0, 0);
    SendMessage(peditorbase->m_hwnd, SCI_EMPTYUNDOBUFFER, 0, 0);
    peditorbase->m_isdiscardchange = wyFalse;

    EditorFont::SetLineNumberWidth(peditorbase->m_hwnd);

    SetFocus(peditorbase->m_hwnd);
    //wnd->SetQueryWindowTitle();

	VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));

    return;
}

void
FrameWindow::OnAlterFunction(MDIWindow *wnd)
{
    wyString createfunction, dropfunction, functionname;
	EditorBase	*peditorbase;

	wnd->m_pcqueryobject->GetCreateFunction(wnd->m_hwnd, wnd->m_tunnel, &wnd->m_mysql, createfunction);

	if(createfunction.GetLength()== 0)
		return;

	wnd->m_pcqueryobject->GetDropFunction(wnd->m_hwnd, wnd->m_tunnel, &wnd->m_mysql, dropfunction);
	pGlobals->m_pcmainwin->GetObjectName(wnd, functionname);

	VERIFY(SetCursor(LoadCursor(NULL, IDC_WAIT)));
	
	wnd->m_pctabmodule->CreateAdvEditorTab(wnd, (wyChar*)functionname.GetString(), IDI_ALTERFUNCTION,
		TreeView_GetParent(wnd->m_pcqueryobject->m_hwnd, 
		TreeView_GetSelection(wnd->m_pcqueryobject->m_hwnd)));
	peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
    
    peditorbase->m_isdiscardchange = wyTrue;
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITEROPEN);
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)dropfunction.GetString());
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)createfunction.GetString());
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITERCLOSE);
    SendMessage(peditorbase->m_hwnd, SCI_SETSAVEPOINT, 0, 0);
    SendMessage(peditorbase->m_hwnd, SCI_EMPTYUNDOBUFFER, 0, 0);
    peditorbase->m_isdiscardchange = wyFalse;
	
	EditorFont::SetLineNumberWidth(peditorbase->m_hwnd);
	SetFocus(peditorbase->m_hwnd);
	//wnd->SetQueryWindowTitle();

	VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));

    return;
}

void 
FrameWindow::OnAlterTrigger(MDIWindow *wnd)
{
    wyString createtrigger, droptrigger, triggername;
	EditorBase	*peditorbase;

	wnd->m_pcqueryobject->GetCreateTrigger(wnd->m_hwnd, wnd->m_tunnel, &wnd->m_mysql, createtrigger);

	if(createtrigger.GetLength()== 0)
		return;

	wnd->m_pcqueryobject->GetDropTrigger(wnd->m_hwnd, wnd->m_tunnel, &wnd->m_mysql, droptrigger);
	pGlobals->m_pcmainwin->GetObjectName(wnd, triggername);

	VERIFY(SetCursor(LoadCursor(NULL, IDC_WAIT)));

	wnd->m_pctabmodule->CreateAdvEditorTab(wnd,(wyChar*)triggername.GetString(), IDI_ALTERTRIGGER, 
		TreeView_GetParent(wnd->m_pcqueryobject->m_hwnd, 
		TreeView_GetSelection(wnd->m_pcqueryobject->m_hwnd)));

	peditorbase = wnd->GetActiveTabEditor()->m_peditorbase;
	peditorbase->m_isdiscardchange = wyTrue;
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITEROPEN);
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)droptrigger.GetString());
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)createtrigger.GetString());
	SendMessage(peditorbase->m_hwnd, SCI_REPLACESEL, TRUE,(LPARAM)DELIMITERCLOSE);
    SendMessage(peditorbase->m_hwnd, SCI_SETSAVEPOINT, 0, 0);
    SendMessage(peditorbase->m_hwnd, SCI_EMPTYUNDOBUFFER, 0, 0);
    peditorbase->m_isdiscardchange = wyFalse;
	
	EditorFont::SetLineNumberWidth(peditorbase->m_hwnd);
	SetFocus(peditorbase->m_hwnd);
	wnd->SetQueryWindowTitle();

	VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));

    return;
}

wyInt32 
FrameWindow::OnAbout()
{
    return(DialogBox(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_ABOUTDIALOG), m_hwndmain, ConnectionBase::AboutDialogProc));
}

void 
FrameWindow::OnShowValues(MDIWindow *wnd, wyInt32 type)
{
	CShowValue csv;
	csv.Create(wnd->m_hwnd, wnd->m_tunnel, &wnd->m_mysql, type);
}

void 
FrameWindow::OnManageUser()
{
    CreateUserManager();
}

void 
FrameWindow::OnTableDiag(MDIWindow *wnd)
{
	TableDiag* pctablediag	= new TableDiag(wnd->m_hwnd, wnd->m_tunnel, &wnd->m_mysql);
    if(pctablediag)
	    delete pctablediag;

    return;
}

void 
FrameWindow::OnImportFromSQL(MDIWindow *wnd)
{
#ifndef COMMUNITY
	if(wnd->m_conninfo.m_isreadonly == wyTrue)
	{
		return;
	}
#endif
    ImportBatch *cib = new ImportBatch();

	if(cib)
	{
		cib->Create(wnd->m_hwnd, wnd->m_tunnel, &wnd->m_mysql);
		delete cib;
	}
    return;	
}

void 
FrameWindow::ManagePreferences()
{
	PreferenceBase* pref = CreatePreferences();
    
    if(pref)
    {
	    pref->Create(pGlobals->m_prefpersist);
        delete pref;
    }

    return;
}

wyInt32 
FrameWindow::OnExit()
{
	wyInt32 ret = 1;

	SendMessage(m_hwndmain, WM_CLOSEALLWINDOW, 0,(LPARAM)&ret);
	if(ret == 0)
		return 0;
	else
		DestroyWindow(m_hwndmain);

    return 1;
}

wyInt32 
FrameWindow::GetCommand(wyWChar *menustring, wyWChar *cmdstring)
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

wyInt32 
FrameWindow::GetKBShortcut(wyWChar *menustring, wyWChar *shortcut)
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

void 
FrameWindow::HandleTBDropDown(NMTOOLBAR *tbh, wyUInt32 buttonid)
{
	RECT		rc;
	HMENU		hmenu, hmenutrack, hmenutracksub;
	TPMPARAMS	tpm;
	wyInt32     isstyle, menuindex = MNUTOOL_INDEX;
	MDIWindow	*wnd = NULL;

	VERIFY(wnd = GetActiveWin());
	if(!wnd)
		return;

	///If focussed con.window not maximized mainmenu get reduced by 1
	isstyle = GetWindowLongPtr(wnd->m_hwnd, GWL_STYLE);
	
	if(isstyle & WS_MAXIMIZE)
		menuindex = MNUTOOL_INDEX;
	else
		menuindex = MNUTOOL_INDEX - 1;

	VERIFY(hmenu = GetMenu(pGlobals->m_pcmainwin->m_hwndmain));
	VERIFY(hmenutrack =	GetSubMenu(hmenu, menuindex));

	switch(buttonid)
	{
	case IDM_TOOL_ADDUSER:
		{
		VERIFY(hmenutracksub = GetSubMenu(hmenutrack, MNUTOOL_INDEX));
			
			//if mysql version is less than 5.0.2, then disable rename user
			if(IsMySQL502(wnd->m_tunnel, &wnd->m_mysql) == wyFalse)
				EnableMenuItem(hmenu, IDM_TOOL_RENAMEUSER,  MF_GRAYED | MF_BYCOMMAND);
			else
				EnableMenuItem(hmenu, IDM_TOOL_RENAMEUSER,  MF_ENABLED | MF_BYCOMMAND);
		}

		break;

	case ID_SHOW_VARIABLES:
		VERIFY(hmenutracksub = GetSubMenu(hmenutrack, MNUTOOL_INDEX + 1));
		break;

	default:
		return;
	}

    SendMessage(tbh->hdr.hwndFrom, TB_GETRECT,(WPARAM)tbh->iItem,(LPARAM)&rc);

    MapWindowPoints(tbh->hdr.hwndFrom, HWND_DESKTOP,(LPPOINT)&rc, 2);                         

    tpm.cbSize		=	sizeof(TPMPARAMS);
    tpm.rcExclude	=	rc;
	
    VERIFY(TrackPopupMenu(hmenutracksub, TPM_LEFTALIGN | TPM_RIGHTBUTTON, rc.left, rc.bottom, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL));

//  SetFocus(m_hwndsecondtool);
    

    return;
}

void 
FrameWindow::OnCreateObjectWmCommand(HWND hwnd, WPARAM wParam, wyChar *object)
{
    wyString	tempcharset;
	
    switch(LOWORD(wParam))
    {
    case IDCANCEL:
	    yog_enddialog(hwnd, 0);
		
		//post 8.01
		//RepaintTabModule();

	    break;

    // get the database and get the name else
    case IDOK:
	    {
		    wyWChar      *objectname;
		    MDIWindow	*pcquerywnd = GetActiveWin();
		    wyBool		ret = wyFalse;
			wyString    objectstr;
    		
		    // just check for safety.
		    if(!pcquerywnd)
			    break;

            objectname = AllocateBuffWChar(SIZE_128 + 1);

		    if(stricmp(object, "CreateDB") == 0)
			    ret = pGlobals->m_pcmainwin->HandleCreateDatabase(hwnd, pcquerywnd, objectname);
			else if(stricmp(object, "AlterDB") == 0)
			    ret = pGlobals->m_pcmainwin->HandleAlterDatabase(hwnd, pcquerywnd, objectname);
			else if(stricmp(object, "CreateProcedure") == 0)
			    ret = pGlobals->m_pcmainwin->HandleCreateProcedure(hwnd, pcquerywnd, objectname);
		    else if(stricmp(object, "CreateFunction") == 0)
			    ret = pGlobals->m_pcmainwin->HandleCreateFunction(hwnd, pcquerywnd, objectname);
		    else if(stricmp(object, "CreateTrigger") == 0)
			    ret = pGlobals->m_pcmainwin->HandleCreateTrigger(hwnd, pcquerywnd, objectname);
			else if(stricmp(object, "CreateEvent") == 0)
				ret = pGlobals->m_pcmainwin->HandleCreateEvent(hwnd, pcquerywnd, objectname);
		    else if(stricmp(object, "CreateView") == 0)
			    ret = pGlobals->m_pcmainwin->HandleCreateView(hwnd, pcquerywnd, objectname);
		    else if(stricmp(object, "GoToLine") == 0)
			    ret = pGlobals->m_pcmainwin->HandleGoTo(hwnd, pcquerywnd, objectname);

		    if(ret == wyTrue)
			{
				objectstr.SetAs(objectname);
				objectstr.RTrim();

				swprintf(objectname, L"%s", objectstr.GetAsWideChar());

			    VERIFY(yog_enddialog(hwnd, (wyInt64)objectname));
			}
			else
				free(objectname);
	    }
	    break;

    case IDC_CHARSETCOMBO:
        {
           /* if((HIWORD(wParam))== CBN_SELENDOK)
            {
                FetchSelectedCharset(hwnd, &tempcharset, wyFalse);
				if(tempcharset.GetLength())
					GetDBCharset(&tempcharset);

                ReInitRelatedCollations(hwnd);
            }*/
			
			if((HIWORD(wParam) == CBN_CLOSEUP) || ((HIWORD(wParam))== CBN_SELENDOK))
			{
				//CBN_CLOSEUP is a matter when select combo listbox item by using tab key
				FetchSelectedCharset(hwnd, &tempcharset, wyFalse);
				
				if(tempcharset.GetLength())
				{
					GetDBCharset(&tempcharset);
				}
				
				ReInitRelatedCollations(hwnd);								
			}			
        }
		break;
    
    case IDC_COLLATECOMBO:
        {
            if((HIWORD(wParam))== CBN_SELENDOK)
            {
                FetchSelectedCollation(hwnd);
            }
        }

    /*
    case IDC_DBNAME:
        {
            //handle the notification
            if(HIWORD(wParam) == EN_CHANGE)
            {
                //if a modification is notified, then check whether the field is blank or not and make changes
                if(IsFieldBlank(GetDlgItem(hwnd, IDC_DBNAME)) < 1)
                    EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
                else
                    EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
            }
        }
        break;
    */
    }
    return;
}
// Fetch The Selected Collation
void
FrameWindow::FetchSelectedCollation(HWND hwnd)
{
    MDIWindow	*pcquerywnda = GetActiveWin();
    wyInt32     length = 0, ncursel;
    wyWChar     *collation = NULL;
    wyString    chsetnamestr;
    HWND        hwndcombo;

    hwndcombo = GetDlgItem(hwnd, IDC_COLLATECOMBO);

    ncursel = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0);;
    length  = SendMessage(hwndcombo, CB_GETLBTEXTLEN, ncursel, NULL);
	collation  = AllocateBuffWChar(length + 1);
    SendMessage(hwndcombo, CB_GETLBTEXT,(WPARAM)ncursel,(LPARAM)collation);
    pcquerywnda->m_pcqueryobject->m_dbcollation.SetAs(collation);
    free(collation);
}

//Filtering of Collation Based on selected charset
void
FrameWindow::ReInitRelatedCollations(HWND hwnd)
{
    MDIWindow	*pcquerywnd = GetActiveWin();
    MYSQL_RES   *myres;
    MYSQL_ROW   myrow;
    wyWChar     *relcollation = NULL;
    wyString    query, collationstr;
    wyInt32      ret, index = 0;
    
    HWND    hwndcombo = GetDlgItem(hwnd, IDC_COLLATECOMBO);
    
    query.SetAs("show collation");

    myres = ExecuteAndGetResult(pcquerywnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query);
    if(!myres)
	{
        ShowMySQLError(hwnd, pcquerywnd->m_tunnel, &pcquerywnd->m_mysql, query.GetString());
		return;
	}
    
    VERIFY(SendMessage(hwndcombo, CB_RESETCONTENT, 0, 0));
    
    while(myrow = pcquerywnd->m_tunnel->mysql_fetch_row(myres))
    {
        collationstr.SetAs(myrow[0]);
        ret = SendMessage(hwndcombo, CB_FINDSTRINGEXACT, -1,(LPARAM)collationstr.GetAsWideChar());
	    if(ret != CB_ERR)
        {
            if(wcsstr(collationstr.GetAsWideChar(), pcquerywnd->m_pcqueryobject->m_dbcharset.GetAsWideChar()) == NULL)
                SendMessage(hwndcombo, CB_DELETESTRING, ret, 0);
        }
        else if((relcollation = wcsstr(collationstr.GetAsWideChar(), pcquerywnd->m_pcqueryobject->m_dbcharset.GetAsWideChar())) != NULL)
        {
            if(collationstr.GetCharAt(pcquerywnd->m_pcqueryobject->m_dbcharset.GetLength()) == '_')
                SendMessage(hwndcombo, CB_ADDSTRING, 0, (LPARAM)collationstr.GetAsWideChar());
        }
        else if(pcquerywnd->m_pcqueryobject->m_dbcharset.CompareI(STR_DEFAULT) == 0)
            SendMessage(hwndcombo, CB_ADDSTRING, 0, (LPARAM)collationstr.GetAsWideChar());
    }
    
    //Select 'Default'	
	 if((index = SendMessage(hwndcombo , CB_ADDSTRING, 0,(LPARAM)TEXT(STR_DEFAULT))) != CB_ERR)
        SendMessage(hwndcombo, CB_SELECTSTRING, 0, (LPARAM)TEXT(STR_DEFAULT));

    pcquerywnd->m_tunnel->mysql_free_result(myres);
}

wyBool  
FrameWindow::CheckForAutoKeywords()
{
	wyInt32     count = 0, ret, tblcount = 0;
	wyWChar		directory[MAX_PATH + 16] = {0};
	wyString	err, query;	
	wyString	directorynamestr;
	sqlite3		*hdb; // SQLite db handle 
    HANDLE      hfind;
	_WIN32_FIND_DATAW fdata;
	
	VERIFY(count = GetModuleFileName(NULL, directory, MAX_PATH - 1));
	
	// change it to .ini
	directory[count - pGlobals->m_modulenamelength] = '\0';
	wcscat(directory, L"Keywords.db");

	directorynamestr.SetAs(directory);

	hfind = FindFirstFile(directory, &fdata);
	
	if(hfind == INVALID_HANDLE_VALUE)
		goto onErr;
		
	FindClose(hfind);
	
	ret = sqlite3_open_v2(directorynamestr.GetString(), &hdb, SQLITE_OPEN_READONLY, 0);
	
	if(ret != SQLITE_OK)
		goto onErr;
		
	query.Sprintf("select * from sqlite_master where type = 'table' and name = 'objects'");
	YogSqliteExec( hdb, query.GetString(), callback_count, &tblcount);
	sqlite3_close(hdb);
	
	if(!tblcount)
		goto onErr;
	
	return wyTrue;

onErr:
	err.Sprintf(_("%s not found or corrupted.\nPlease reinstall %s."), directorynamestr.GetString(), pGlobals->m_appname.GetString());
	yog_message(NULL, err.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);
	return wyFalse;
}

// This function implements the main dialog window which asks for the hostname, username
// password and port number so that we connect to a mySQL server. The dialog box returns
// 0 if any error ouucred or if the connection was successful it returns a pointer to a
// valid mysql structure.
wyBool 
FrameWindow::CreateConnDialog(wyBool readsession)
{
	MDIWindow		*pcquerywnd = NULL;//, *wnd;
	wyInt32         ret;
	ConnectionInfo	conninfo;
    
	wyString		failedconnections;
	HANDLE thread_handle, savethread_handle;
    InitializeConnectionInfo(conninfo);
	SYSTEMTIME	systime ={0};
	DWORD		currntday = 0, lastchkdday = 0;
	wyString	dirstr;
	wyWChar		directory[MAX_PATH +1]={0}, *lpfileport;
	wyString	keyvaluestr, currentdate;
	UpgradeCheck upg;
	wyBool		isannthread = wyFalse;
	VERIFY(IsWindow(pGlobals->m_hwndclient));
	
	if(!pGlobals->m_mdiwlist)
		pGlobals->m_mdiwlist = new List();
	pGlobals->m_pcmainwin->AddTextInStatusBar(CONNECT_MYSQL_MSG);

	//pOST 8.01
	//wnd = GetActiveWin();
	/*if(wnd)
    {
		//InvalidateRect(wnd->m_pctabmodule->GetHwnd(), NULL, FALSE);                 
		//InvalidateRect(wnd->m_pcqueryobject->m_hwnd, NULL, FALSE);
		UpdateWindow(wnd->m_pctabmodule->GetHwnd());
		UpdateWindow(wnd->m_pcqueryobject->m_hwnd);
	}*/

	//call thread only during startup
	//read last chect date time from ini
	if(readsession)
	{
		SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
		if(directory)
		{
			dirstr.SetAs(directory);
		}
		if(dirstr.GetLength())
		{
			ret = wyIni::IniGetString(GENERALPREFA, "AnnouncementsCheckedDate", "0", &keyvaluestr, dirstr.GetString());
			lastchkdday = strtoul(keyvaluestr.GetString(), NULL, 10); 
		}
		GetSystemTime(&systime);		
		currntday = upg.HandleSetDayFormat(systime.wDay, systime.wMonth, systime.wYear);
		currentdate.Sprintf("%lu", currntday);

#if IS_FROM_WINDOWS_STORE == 1
			lastchkdday = 0;
#endif

		if(currntday > lastchkdday && currntday - lastchkdday >= 1)
		//if(TRUE)
		{
			isannthread = wyTrue;
			thread_handle = (HANDLE)_beginthreadex(NULL, 0, Htmlannouncementsproc, NULL, 0, NULL);
			pGlobals->m_isannouncementopen = wyFalse;
		}
	}
	//check if pref option says do not open restore connection dlg
	//if(readsession && IsConnectionRestore() && GetSessionFile(path))
	if(readsession && IsConnectionRestore() && ConnectFromList(&failedconnections))
	{
		
		SetTimer(pGlobals->m_pcmainwin->m_hwndmain, CONRESTORE_TIMER, 500, NULL);

		if(failedconnections.GetLength())
		{
			failedconnections.Insert(0, _("Failed to restore following connections\r\n\r\n"));
			failedconnections.Insert(0, _("\r\n\r\n"));
			failedconnections.Insert(0, (wyChar*)pGlobals->m_sessionbackup.GetString());
			failedconnections.Insert(0, _("The backup of Session Savepoint is created at\r\n\r\n"));
			MessageBox(pGlobals->m_pcmainwin->m_hwndmain, failedconnections.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONINFORMATION | MB_OK);
		}

		ret = 0;
	}	
	else
	{
		if(m_showwindowstyle != SW_HIDE) 
		{
			ShowWindow(m_hwndmain, m_showwindowstyle);
			m_showwindowstyle = SW_HIDE;
		}

		ret = pGlobals->m_pcmainwin->m_connection->ActivateDialog(&conninfo);
	}
    
	// Now check what was returned from the dialog box.
	switch(ret)
	{
	case ZERO:
		//CustomTab_SetCurSel(pGlobals->m_pcmainwin->m_hwndconntab, pGlobals->m_pcmainwin->m_focussedcon);
		break;

	default:

		if(m_hwndconntab == NULL)
		{
			//create connection tab
			m_hwndconntab = pGlobals->m_pcmainwin->m_conntab->CreateConnectionTabControl(GetHwnd());
		}
			
//		DEBUG_LOG("starting mdiwindow");
		pcquerywnd	= new MDIWindow(pGlobals->m_pcmainwin->GetMDIWindow(), &conninfo, conninfo.m_db, conninfo.m_title);
//		DEBUG_LOG("starting mdiwindow creation");
		pcquerywnd->Create();
//		DEBUG_LOG("created");
		// if successful then create a new MDI child window with the just connected mysql
		// info.
		if(pcquerywnd)
        {
			pGlobals->m_pcquerywnd = pcquerywnd;
			pGlobals->m_conncount++;
            pGlobals->m_pcmainwin->SetConnectionNumber();
			pGlobals->m_pcmainwin->OnActiveConn();
			pcquerywnd->m_pcqueryobject->OnSelChanged(TreeView_GetSelection(pcquerywnd->m_pcqueryobject->m_hwnd));
			SetForegroundWindow(pcquerywnd->m_hwnd);
	
            PostMessage(pGlobals->m_pcmainwin->m_hwndmain, WM_SETACTIVEWINDOW, (WPARAM)pcquerywnd->m_hwnd, 0 );
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		}

		break;
	}
	if(readsession && isannthread)
	{
		if(WaitForSingleObject (thread_handle, 5000) == WAIT_TIMEOUT )
		{
			//pGlobals->m_announcementshtml.SetAs("");
			pGlobals->m_isannouncementopen = wyFalse;
		}
		else
		{
			pGlobals->m_isannouncementopen = wyTrue;
			if(pGlobals->m_pcquerywnd !=NULL)
			{
				if(pGlobals->m_announcementshtml.GetLength()!= 0)
					pGlobals->m_pcquerywnd->Resize();
				else
					pGlobals->m_isannouncementopen = wyFalse;
					
				//write date time to ini
				wyIni::IniWriteString(GENERALPREFA, "AnnouncementsCheckedDate", currentdate.GetString(), dirstr.GetString());
			}
		}
	}

	if(readsession)
	{
		// create a thread here to save session periodically
		m_savetimerevent = CreateEvent(NULL, TRUE, TRUE, NULL);
		m_sqlyogcloseevent = CreateEvent(NULL, TRUE, TRUE, NULL);
		m_sessionsaveevent = CreateEvent(NULL, TRUE, TRUE, NULL);
		//pGlobals->m_pcmainwin->m_issessionedited
		m_sessionchangeevent = CreateEvent(NULL, TRUE, m_issessionedited, NULL);
		//SetEvent(m_sessionchangeevent);
		SetTimer(pGlobals->m_pcmainwin->m_hwndmain, CONSAVE_TIMER, CONSAVE_INTERVAL, NULL);
		savethread_handle = (HANDLE)_beginthreadex(NULL, 0, sessionsavesproc, NULL, 0, NULL);
		m_savethread_handle = savethread_handle;
	}

	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return wyTrue;
}


wyBool 
FrameWindow::OnNewSameConnection(HWND hwndactive, MDIWindow *pcquerywnd)
{
    wyBool          ret;
    MDIWindow	    *pcquerywndnew;
    ConnectionInfo  conninfo;
		
    //Changes Made to fix issue#1654
    pGlobals->m_pcmainwin->m_connection->m_currentconn.SetAs(pcquerywnd->m_currentconn.GetString());
    pGlobals->m_pcmainwin->m_connection->m_rgbobbkcolor=pcquerywnd->m_conninfo.m_rgbconn;
    pGlobals->m_pcmainwin->m_connection->m_rgbobfgcolor=pcquerywnd->m_conninfo.m_rgbfgconn;
    
    
    ret = m_connection->OnNewSameConnection(hwndactive, pcquerywnd, conninfo);
		
	//use same color if user presses ctrl N
	m_connection->m_rgbobbkcolor  =  conninfo.m_rgbconn;
	m_connection->m_rgbobfgcolor  =  conninfo.m_rgbfgconn;

    

	if(pcquerywnd && hwndactive && ret == wyTrue)
    {
        pcquerywndnew	= new MDIWindow(pGlobals->m_pcmainwin->GetMDIWindow(), 
            &conninfo, pcquerywnd->m_filterdb, pcquerywnd->m_title);
		pcquerywndnew->Create();

		pGlobals->m_conncount++;
		pGlobals->m_pcmainwin->SetConnectionNumber();
		pGlobals->m_pcmainwin->OnActiveConn();
	}

	VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));
	ShowCursor(1);

    return wyTrue;
}

wyBool
FrameWindow::EnableToolButtonsAndCombo(HWND hwndtool, HWND hwndsecondtool, HWND hwndcombo, wyBool enable, wyBool isexec)
{
    wyInt32     id, tabicon = 0, state, count, i, image;
    MDIWindow   *wnd = GetActiveWin();
    HTREEITEM hitem;

    wyInt32 tb1id[] = {IDM_FILE_CONNECT, ID_NEW_EDITOR, IDM_EXECUTE, ACCEL_EXECUTEALL, ACCEL_QUERYUPDATE, IDM_REFRESHOBJECT};

    wyInt32 tb2id[] = {	
        IDM_TOOL_ADDUSER, 
        ID_IMPORTEXPORT_DBEXPORTTABLES,
        ID_IMEX_TEXTFILE, 
        ID_OPEN_COPYDATABASE,
        ID_EXPORT_AS,
        ID_OBJECT_MAINMANINDEX,
        ACCEL_MANREL,
        ID_FORMATCURRENTQUERY,
        IDC_DIFFTOOL,
        ID_QUERYBUILDER,
        ID_SCHEMADESIGNER,
		ID_ROLLBACK_TRANSACTION,
		ID_STARTTRANSACTION_WITHNOMODIFIER,
		ID_COMMIT_WITHNOMODIFIER
    };

    wyInt32	tb3id[] = {	IDM_DATASYNC, ID_IMPORT_EXTERNAL_DATA, IDC_TOOLS_NOTIFY, ID_POWERTOOLS_SCHEDULEEXPORT};

    if(wnd && wnd->m_executing == wyFalse)
    {
        count = sizeof(tb2id)/sizeof(tb2id[0]);
        image = wnd->m_pcqueryobject->GetSelectionImage();
        tabicon = wnd->m_pctabmodule->GetActiveTabImage();

        for(i = 0; i < count; ++i)
        {
            state = TBSTATE_ENABLED;
#ifndef COMMUNITY
			if(wnd->m_conninfo.m_isreadonly == wyTrue && 
				(tb2id[i] == IDC_DIFFTOOL || tb2id[i] == ID_IMEX_TEXTFILE ||  
				 tb2id[i] == ID_OBJECT_MAINMANINDEX || tb2id[i] == ID_OPEN_COPYDATABASE || tb2id[i] == ACCEL_MANREL))
				state = TBSTATE_INDETERMINATE;
#endif
            if(tb2id[i] == ID_EXPORT_AS || tb2id[i] == ID_OBJECT_MAINMANINDEX || tb2id[i] == ACCEL_MANREL)
            {      
                id = image;

                if(id == NFOLDER)
                {
                    if((hitem = TreeView_GetSelection(wnd->m_pcqueryobject->m_hwnd)) && 
                        (hitem = TreeView_GetParent(wnd->m_pcqueryobject->m_hwnd, hitem)))
                    {
                        id = GetItemImage(wnd->m_pcqueryobject->m_hwnd, hitem);
                    }
                }

                switch(id)
                {
                    case NTABLE:
                    case NCOLUMN:
                    case NPRIMARYKEY:
                    case NINDEX:
                    case NPRIMARYINDEX:
                        break;

                    default:
                        state = TBSTATE_INDETERMINATE;
                }
            }
            else if(tb2id[i] == ID_OPEN_COPYDATABASE)
            {
                if(image == NSERVER)
                {
                    state = TBSTATE_INDETERMINATE;
                }
            }
            else if(tb2id[i] == ID_FORMATCURRENTQUERY)
            {
                if(tabicon == IDI_SCHEMADESIGNER_16 || tabicon == IDI_QUERYBUILDER_16 || tabicon == IDI_DATASEARCH || tabicon == IDI_CREATETABLE || tabicon == IDI_ALTERTABLE
                    || tabicon == IDI_HISTORY || tabicon == IDI_TABLEINDEX || tabicon == IDI_TABLE) 
                {
                    state = TBSTATE_INDETERMINATE;
                }
            }
#ifndef COMMUNITY
			else if(tb2id[i] == ID_STARTTRANSACTION_WITHNOMODIFIER || tb2id[i] == ID_ROLLBACK_TRANSACTION || tb2id[i] == ID_COMMIT_WITHNOMODIFIER )
			{
				HWND hwnd = wnd->GetHwnd();
				if(wnd->m_ptransaction)
				{
					if(tb2id[i] == ID_STARTTRANSACTION_WITHNOMODIFIER)
					{
						if(wnd->m_ptransaction->m_starttransactionenabled)
							state = TBSTATE_ENABLED;
						else
							state = TBSTATE_INDETERMINATE;
					}
					else
						if(!wnd->m_ptransaction->m_starttransactionenabled)
							state = TBSTATE_ENABLED;
						else
							state = TBSTATE_INDETERMINATE;
				}
			}
#endif
            SendMessage(hwndsecondtool, TB_SETSTATE, (WPARAM)tb2id[i], state);
        }

        EnableWindow(hwndcombo, TRUE);
        count = sizeof(tb1id)/sizeof(tb1id[0]);

        for(i = 0; i < count; ++i)
        {
            state = TBSTATE_ENABLED;

            if(tb1id[i] == IDM_EXECUTE || tb1id[i] == ACCEL_QUERYUPDATE)
            {
				if(tabicon != IDI_QUERY_16)
                {
                    state = TBSTATE_INDETERMINATE;
                }
            }
            else if(tb1id[i] == ACCEL_EXECUTEALL)
            {
                if(tabicon == IDI_SCHEMADESIGNER_16 || tabicon == IDI_QUERYBUILDER_16 || tabicon == IDI_DATASEARCH || tabicon == IDI_CREATETABLE || tabicon == IDI_ALTERTABLE
                    || tabicon == IDI_HISTORY || tabicon == IDI_TABLEINDEX || tabicon == IDI_TABLE)
                {
                    state = TBSTATE_INDETERMINATE;
                }
            }
#ifndef COMMUNITY
			if(wnd->m_conninfo.m_isreadonly == wyTrue && tb1id[i] == ACCEL_QUERYUPDATE)
			{
				state = TBSTATE_INDETERMINATE;
			}
#endif
            SendMessage(hwndtool, TB_SETSTATE, (WPARAM)tb1id[i], state);
        }

        count = sizeof(tb3id)/sizeof(tb3id[0]);

        for(i = 0; i < count; ++i)
        {
#ifndef COMMUNITY
			if(wnd->m_conninfo.m_isreadonly == wyTrue && (tb3id[i] != ID_POWERTOOLS_SCHEDULEEXPORT && tb3id[i] != IDC_TOOLS_NOTIFY ))
				SendMessage(hwndsecondtool, TB_SETSTATE, (WPARAM)tb3id[i], TBSTATE_INDETERMINATE);
			else
#endif
				SendMessage(hwndsecondtool, TB_SETSTATE, (WPARAM)tb3id[i], TBSTATE_ENABLED);
        }
	}
    else
    {
        count = sizeof(tb2id)/sizeof(tb2id[0]);

        for(i = 0; i < count; ++i)
        {
            SendMessage(hwndsecondtool, TB_SETSTATE, (WPARAM)tb2id[i], TBSTATE_INDETERMINATE);
        }

        EnableWindow(hwndcombo, FALSE);

        count = sizeof(tb1id)/sizeof(tb1id[0]);

        for(i = 0; i < count; ++i)
        {
            SendMessage(hwndtool, TB_SETSTATE, (WPARAM)tb1id[i], TBSTATE_INDETERMINATE);
        }

        count = sizeof(tb3id)/sizeof(tb3id[0]);

        for(i = 0; i < count; ++i)
        {
            SendMessage(hwndsecondtool, TB_SETSTATE, (WPARAM)tb3id[i], wnd == NULL ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE);
        }
    }

	return wyTrue;
}

wyBool
FrameWindow::HandleChangeTabletype(HMENU hmenu, wyInt32 id) 
{
    MENUITEMINFO	lpmii;
    wyWChar          menuname[64] = {0};

    MDIWindow       *wnd = GetActiveWin();

    if(!wnd)
        return wyFalse;

    memset((void*)&lpmii, 0, sizeof(MENUITEMINFO));

	lpmii.cbSize		= sizeof(MENUITEMINFO);
    lpmii.fMask			= MIIM_STRING | MIIM_ID | MIIM_STATE;
	lpmii.cch			= MAX_PATH;
	lpmii.dwTypeData	= menuname;

	VERIFY(GetMenuItemInfo(hmenu, id, FALSE, &lpmii));

    if(!(lpmii.fState & MFS_CHECKED))
    {
        wnd->m_pcqueryobject->ChangeTableType(wnd->m_tunnel, &wnd->m_mysql, menuname);
    }

    return wyTrue;
}
// 
// find any query is selected in Query editor if selected return true
wyBool
FrameWindow::IsQuerySelected(EditorBase *peditorbase)
{
	wyInt32			ret;
	CHARRANGE		chr;

	chr.cpMin = SendMessage(peditorbase->m_hwnd, SCI_GETSELECTIONSTART, 0, 0);
	chr.cpMax = SendMessage(peditorbase->m_hwnd, SCI_GETSELECTIONEND, 0, 0);
	ret = chr.cpMin != chr.cpMax;
    return(ret)?(wyTrue):(wyFalse);
}


void	
FrameWindow::OnCreateObjectWmHelp(HWND hwnd, wyChar *object)
{
	
	if(stricmp(object, "createdb") == 0)
	{
		ShowHelp("http://sqlyogkb.webyog.com/article/113-create-database");
	}
	else if(stricmp(object, "CreateView")== 0 )
	{
		ShowHelp("http://sqlyogkb.webyog.com/article/186-views");
	}
	else if(stricmp(object, "CreateProcedure")== 0 )
	{
		ShowHelp("http://sqlyogkb.webyog.com/article/188-stored-procedures");
	}
	else if(stricmp(object, "CreateFunction")== 0 )
	{
		ShowHelp("http://sqlyogkb.webyog.com/article/189-functions");
	}
	else if(stricmp(object, "CreateEvent")== 0 )
	{
		ShowHelp("http://sqlyogkb.webyog.com/article/191-events");
	}
	else if(stricmp(object, "CreateTrigger")== 0 )
	{
		ShowHelp("http://sqlyogkb.webyog.com/article/190-triggers");
	}	
}

////check for upgrade	

void
FrameWindow::CheckForUpgrade(wyBool isexplict)
{	
#if IS_FROM_WINDOWS_STORE == 1
	return;
#endif
	UpgradeCheck *upgradeexplicit = NULL;
		
	if(isexplict == wyTrue)
	{
		upgradeexplicit = new UpgradeCheck();
		upgradeexplicit->m_hwndmain = m_hwndmain;
		upgradeexplicit->HandleUpgradeCheck(isexplict);						

		delete upgradeexplicit;
	}

	else
	{
		m_upgrdchk->m_hwndmain = m_hwndmain;
		m_upgrdchk->HandleUpgradeCheck(isexplict);						
	}
}

///Handles the community ribbon
void				
FrameWindow::HandleCommunityRibbon()
{
#ifdef COMMUNITY
	m_commribbon = new CommunityRibbon();

	m_commribbon->HandleCommunityHeader();	
#endif
}

void
FrameWindow::DestroyToolBarResources()
{
	if(m_hsecondiml)
	{
		VERIFY(ImageList_Destroy(m_hsecondiml));
		m_hsecondiml = NULL;
	}

	if(m_hwndtool)
	{
		VERIFY(DestroyWindow(m_hwndtool));
		m_hwndtool =  NULL;
	}

	if(m_hwndsecondtool)
	{
		VERIFY(DestroyWindow(m_hwndsecondtool));
		m_hwndsecondtool =  NULL;
	}

	if(m_hwndtoolcombo)
	{
		VERIFY(DestroyWindow(m_hwndtoolcombo));
		m_hwndtoolcombo =  NULL;
	}
}

//Tool bar is re arranging according to the icon size selected in Preferences
void
FrameWindow::ReArranageToolBar()
{
	MDIWindow	*wnd = NULL;
	RECT		rectmain;

	VERIFY(wnd = GetActiveWin());
	
	CreateToolBarWindow();
	Resize();

	if(wnd)
	{
		EnableToolButtonsAndCombo(m_hwndtool, m_hwndsecondtool, m_hwndtoolcombo, wyTrue);
		wnd->m_pcqueryobject->OnSelChanged(TreeView_GetSelection(wnd->m_pcqueryobject->m_hwnd));
	}
	else
		EnableToolButtonsAndCombo(m_hwndtool, m_hwndsecondtool, m_hwndtoolcombo, wyFalse);

	//Paint after changing the tool bar icon size	
	GetClientRect(m_hwndmain, &rectmain);
	rectmain.bottom = m_toolbariconsize + 30;

	//post 8.01 painting
	InvalidateRect(m_hwndmain, NULL, TRUE);
	UpdateWindow(GetParent(m_hwndmain));		
}

wyBool
FrameWindow::OnTBDropDown(NMTOOLBAR* tbh)
{
	MDIWindow*	pcquerywnd = GetActiveWin();

	_ASSERT(pcquerywnd);

    if(!pcquerywnd)
        return wyFalse;

	SciRedraw(pcquerywnd, wyFalse);

	//Pop up corresponding menus
	HandleTBDropDown(tbh, tbh->iItem);		

	SciRedraw(pcquerywnd, wyTrue);
	return wyTrue;
}

// Callback procedure to MDI child window.
BOOL CALLBACK 
FrameWindow::EnumMDIChildren(HWND hwnd, LPARAM lParam)
{
    HWND* hwndtoactivate;
    static HWND hwndtemp = NULL;
    
    hwndtoactivate = (HWND*)lParam;

    if(GetParent(hwnd) == GetParent(*hwndtoactivate))
    {
        if(hwnd == *hwndtoactivate)
        {
            *hwndtoactivate = hwndtemp;
            return FALSE;
        }
        hwndtemp = hwnd;
    }
    return TRUE;
}

void 
FrameWindow::LoadConnTabPlusMenu(LPARAM lparam)
{
    LPNMCTC lpnmctc;
	HMENU	hmenu, htrackmenu;

    lpnmctc = (LPNMCTC)lparam;
    hmenu =	LoadMenu(pGlobals->m_hinstance, MAKEINTRESOURCE(IDR_CONNTABPLUSMENU));
    LocalizeMenu(hmenu);
	htrackmenu = GetSubMenu(hmenu, 0);
	// Set menu draw property for drawing icon
	wyTheme::SetMenuItemOwnerDraw(htrackmenu);
    TrackPopupMenu(htrackmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, lpnmctc->curpos.x, lpnmctc->curpos.y, 0, pGlobals->m_pcmainwin->m_hwndmain, NULL);
    FreeMenuOwnerDrawItem(htrackmenu);
   	VERIFY(DestroyMenu(hmenu));	
}

void CALLBACK 
FrameWindow::TooltipTimerProc(HWND hwnd, UINT message, UINT_PTR id, DWORD time)
{
    wyInt32 timerid;

    KillTimer(hwnd, id);

    if(pGlobals->m_pcmainwin->m_hwndtooltip)
    {
        timerid = GetWindowLongPtr(pGlobals->m_pcmainwin->m_hwndtooltip, GWLP_USERDATA);

        if(timerid == id)
        {
            FrameWindow::ShowQueryExecToolTip(wyFalse);
        }
    }
}

void
FrameWindow::ShowQueryExecToolTip(wyBool show)
{
    POINT           pt;
    TOOLINFO        ti = {0};
    static wyInt32  timerid = 100;

    if(pGlobals->m_pcmainwin->m_hwndtooltip)
    {
        KillTimer(pGlobals->m_pcmainwin->m_hwndmain, timerid);
        DestroyWindow(pGlobals->m_pcmainwin->m_hwndtooltip);
        pGlobals->m_pcmainwin->m_hwndtooltip = NULL;
    }

    if(show == wyFalse)
    {
        return;
    }
        
    GetCursorPos(&pt);
    pGlobals->m_pcmainwin->m_hwndtooltip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, 
            TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, 
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
                                 pGlobals->m_pcmainwin->m_hwndmain, NULL, GetModuleHandle(NULL),NULL);    

    if(!pGlobals->m_pcmainwin->m_hwndtooltip)
    {
        return;
    }

    timerid = (timerid == 100) ? 101 : 100;
    SetWindowLongPtr(pGlobals->m_pcmainwin->m_hwndtooltip, GWLP_USERDATA, timerid);
    ti.cbSize   = sizeof(TOOLINFO);
    ti.hwnd     = pGlobals->m_pcmainwin->m_hwndmain;
    ti.uId      = (UINT_PTR)pGlobals->m_pcmainwin->m_hwndmain;
    ti.lpszText = L"You cannot switch to another tab. Please create another connection (Ctrl+N)";
    ti.uFlags   = TTF_IDISHWND | TTF_TRACK;
    ti.hinst    = GetModuleHandle(NULL);
    ti.lParam   = (LPARAM)&pGlobals->m_pcmainwin->m_hwndtooltip;
    GetClientRect (pGlobals->m_pcmainwin->m_hwndmain, &ti.rect);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtooltip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtooltip, TTM_SETTITLE, TTI_INFO, (LPARAM)L"Query execution in progress!");
    SendMessage(pGlobals->m_pcmainwin->m_hwndtooltip, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(pt.x, pt.y));
    SendMessage(pGlobals->m_pcmainwin->m_hwndtooltip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtooltip, TTM_UPDATE, 0, 0);
    SetTimer(pGlobals->m_pcmainwin->m_hwndmain, timerid, 4000, FrameWindow::TooltipTimerProc);
}


wyBool
FrameWindow::WriteTabDetailsToTempList(tabeditorelem *temptabeditorele_temp, CTCITEM quetabitem, wyInt32 tabid, wyInt32 position, wyInt32 id, TabTypes *tabactive, MDIWindow *wnd)
{
	TabEditor		*tabquery=NULL;
	wyString		testquery, sqlitequery;

#ifndef COMMUNITY
	//if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
	TabQueryBuilder		*tabquerybuilder;	
	TabSchemaDesigner	*tabschemadesigner;
#endif
	
	if(quetabitem.m_iimage == IDI_QUERYBUILDER_16)
	{
#ifndef COMMUNITY
if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
		tabquerybuilder = (TabQueryBuilder*)quetabitem.m_lparam;
#endif
	}
	else
	if(quetabitem.m_iimage == IDI_SCHEMADESIGNER_16)
	{
#ifndef COMMUNITY
if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
		tabschemadesigner = (TabSchemaDesigner*)quetabitem.m_lparam;
#endif
	}
	else
	{
	tabquery = (TabEditor*)quetabitem.m_lparam;
		temptabeditorele_temp->m_pctabeditor = tabquery;
		
	}

	temptabeditorele_temp->m_tabptr = quetabitem.m_lparam;	
	temptabeditorele_temp->m_iimage = quetabitem.m_iimage;	
	//tabquery = (TabEditor*)quetabitem.m_lparam;
	temptabeditorele_temp->m_id = id;
	temptabeditorele_temp->m_ispresent = wyTrue;
	
	temptabeditorele_temp->m_tabid = tabid;
	temptabeditorele_temp->m_position = position;
	temptabeditorele_temp->m_color = quetabitem.m_color;
	temptabeditorele_temp->m_fgcolor = quetabitem.m_fgcolor;
	temptabeditorele_temp->m_isfocussed = wyFalse;
	

	if((TabTypes*)quetabitem.m_lparam == tabactive)
			temptabeditorele_temp->m_isfocussed = wyTrue;
	if(quetabitem.m_iimage != IDI_QUERYBUILDER_16 && quetabitem.m_iimage != IDI_SCHEMADESIGNER_16 )
	{
	temptabeditorele_temp->m_isedited = temptabeditorele_temp->m_pctabeditor->m_peditorbase->m_edit;
	temptabeditorele_temp->m_isfile = temptabeditorele_temp->m_pctabeditor->m_peditorbase->m_filename.GetLength() > 0 ? wyTrue : wyFalse;
		
		
		if(!temptabeditorele_temp->m_isfile || temptabeditorele_temp->m_isedited )
				temptabeditorele_temp->m_pctabeditor->m_peditorbase->GetCompleteTextByPost(testquery, wnd);
		else
			testquery.SetAs("");
	temptabeditorele_temp->m_leftortoppercent = tabquery->m_pcetsplitter->GetLeftTopPercent();
	}
	else
	if(quetabitem.m_iimage == IDI_SCHEMADESIGNER_16)
	{
#ifndef COMMUNITY
if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
{
		temptabeditorele_temp->m_isedited = tabschemadesigner->m_isedited;
		temptabeditorele_temp->m_isfile = tabschemadesigner->m_filename.GetLength() > 0 ? wyTrue : wyFalse;
		temptabeditorele_temp->m_leftortoppercent = 50;
		if(!temptabeditorele_temp->m_isfile || temptabeditorele_temp->m_isedited )
			tabschemadesigner->WriteToXMLbuffer(testquery);
		else
			testquery.SetAs("");
}
#endif
	}
	else
	{
#ifndef COMMUNITY
if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
{
		temptabeditorele_temp->m_isedited = tabquerybuilder->m_isedited;
		temptabeditorele_temp->m_isfile = tabquerybuilder->m_filename.GetLength() > 0 ? wyTrue : wyFalse;
		temptabeditorele_temp->m_leftortoppercent = 50;
	if(!temptabeditorele_temp->m_isfile || temptabeditorele_temp->m_isedited )
			tabquerybuilder->WriteToXMLbuffer(testquery);
	else
		testquery.SetAs("");
}
#endif
	}
	CustomTab_GetTitle(wnd->m_pctabmodule->m_hwnd, position, &temptabeditorele_temp->m_psztext);
	CustomTab_GetTooltip(wnd->m_pctabmodule->m_hwnd, position, &temptabeditorele_temp->m_tooltiptext);
	temptabeditorele_temp->m_content.SetAs(testquery.GetString());
						
	return wyTrue;
}


wyBool
FrameWindow::WriteTabDetailsToTable2(tabeditorelem *temptabeditorele, CTCITEM quetabitem, wyInt32 tabid, wyInt32 position, wyInt32 id, TabTypes *tabactive, MDIWindow *wnd, wySQLite	*ssnsqliteobj)
{
	sqlite3_stmt*   stmt;
	TabEditor		*tabquery=NULL;
	wyString		testquery, sqlitequery;
	wySQLite	*sqliteobj;
	sqliteobj = ssnsqliteobj ? ssnsqliteobj : pGlobals->m_sqliteobj;
#ifndef COMMUNITY
//if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
	TabQueryBuilder		*tabquerybuilder;	
	TabSchemaDesigner	*tabschemadesigner;
#endif
	
	if(quetabitem.m_iimage == IDI_QUERYBUILDER_16)
	{
#ifndef COMMUNITY
if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
		tabquerybuilder = (TabQueryBuilder*)quetabitem.m_lparam;
#endif
	}
	else
	if(quetabitem.m_iimage == IDI_SCHEMADESIGNER_16)
	{
#ifndef COMMUNITY
if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
		tabschemadesigner = (TabSchemaDesigner*)quetabitem.m_lparam;
#endif
	}
	else
	{
	tabquery = (TabEditor*)quetabitem.m_lparam;
		temptabeditorele->m_pctabeditor = tabquery;
		
	}

	temptabeditorele->m_tabptr = quetabitem.m_lparam;	
	//tabquery = (TabEditor*)quetabitem.m_lparam;
	temptabeditorele->m_iimage = quetabitem.m_iimage;
	temptabeditorele->m_id = id;
	temptabeditorele->m_ispresent = wyTrue;
	
	temptabeditorele->m_tabid = tabid;
	temptabeditorele->m_position = position;
	temptabeditorele->m_color = quetabitem.m_color;
	temptabeditorele->m_fgcolor = quetabitem.m_fgcolor;
	temptabeditorele->m_isfocussed = wyFalse;
	

	if((TabTypes*)quetabitem.m_lparam == tabactive)
			temptabeditorele->m_isfocussed = wyTrue;
	if(quetabitem.m_iimage != IDI_QUERYBUILDER_16 && quetabitem.m_iimage != IDI_SCHEMADESIGNER_16 )
	{
	temptabeditorele->m_isedited = temptabeditorele->m_pctabeditor->m_peditorbase->m_edit;
	temptabeditorele->m_isfile = temptabeditorele->m_pctabeditor->m_peditorbase->m_filename.GetLength() > 0 ? wyTrue : wyFalse;
		
		
		if(!temptabeditorele->m_isfile || temptabeditorele->m_isedited )
				temptabeditorele->m_pctabeditor->m_peditorbase->GetCompleteTextByPost(testquery, wnd);
		else
			testquery.SetAs("");
	temptabeditorele->m_leftortoppercent = tabquery->m_pcetsplitter->GetLeftTopPercent();
	}
	else
	if(quetabitem.m_iimage == IDI_SCHEMADESIGNER_16)
	{
#ifndef COMMUNITY
if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
{
		temptabeditorele->m_isedited = tabschemadesigner->m_isedited;
		temptabeditorele->m_isfile = tabschemadesigner->m_filename.GetLength() > 0 ? wyTrue : wyFalse;
		temptabeditorele->m_leftortoppercent = 50;
		if(!temptabeditorele->m_isfile || temptabeditorele->m_isedited )
			tabschemadesigner->WriteToXMLbuffer(testquery);
		else
			testquery.SetAs("");
}
#endif
	}
	else
	{
#ifndef COMMUNITY
if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
{
		temptabeditorele->m_isedited = tabquerybuilder->m_isedited;
		temptabeditorele->m_isfile = tabquerybuilder->m_filename.GetLength() > 0 ? wyTrue : wyFalse;
		temptabeditorele->m_leftortoppercent = 50;
	if(!temptabeditorele->m_isfile || temptabeditorele->m_isedited )
			tabquerybuilder->WriteToXMLbuffer(testquery);
	else
		testquery.SetAs("");
}
#endif
	}
	CustomTab_GetTitle(wnd->m_pctabmodule->m_hwnd, position, &temptabeditorele->m_psztext);
	CustomTab_GetTooltip(wnd->m_pctabmodule->m_hwnd, position, &temptabeditorele->m_tooltiptext);
	temptabeditorele->m_content.SetAs(testquery.GetString());

	if(!ssnsqliteobj)
		wnd->m_listtabeditor->Insert(temptabeditorele);
	
	sqlitequery.Sprintf("INSERT INTO tabdetails (Id, Tabid,Tabtype, position,leftortoppercent,isedited, title, tooltip, isfile,isfocussed,content) VALUES \
												(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

	sqliteobj->Prepare(&stmt, sqlitequery.GetString());

	//sqlitequery.Sprintf("INSERT INTO tabdetails (Id, Tabid, position,leftortoppercent,isedited, title, tooltip, isfile,isfocussed,content) 
	//VALUES (%d , %d, %d, %d, %d, \"%s\", \"%s\", %d, %d, \"%s\")",
	//tempmdilist->m_id,temptabeditorele->m_tabid,temptabeditorele->m_position,temptabeditorele->m_leftortoppercent,temptabeditorele->m_isedited,temptabeditorele->m_psztext.GetString(),temptabeditorele->m_tooltiptext.GetString(),temptabeditorele->m_isfile,temptabeditorele->m_isfocussed,testquery.GetString());
	sqliteobj->SetInt(&stmt, 1, id);
	sqliteobj->SetInt(&stmt, 2, temptabeditorele->m_tabid);
	sqliteobj->SetInt(&stmt, 3, quetabitem.m_iimage);
	sqliteobj->SetInt(&stmt, 4, temptabeditorele->m_position);
	sqliteobj->SetInt(&stmt, 5, temptabeditorele->m_leftortoppercent);
	sqliteobj->SetInt(&stmt, 6, temptabeditorele->m_isedited);
	sqliteobj->SetText(&stmt, 7, temptabeditorele->m_psztext.GetString());
	sqliteobj->SetText(&stmt, 8,	temptabeditorele->m_tooltiptext.GetString());
	sqliteobj->SetInt(&stmt, 9,	temptabeditorele->m_isfile);
	sqliteobj->SetInt(&stmt, 10,	temptabeditorele->m_isfocussed);
	sqliteobj->SetText(&stmt, 11,	testquery.GetString());
	
	sqliteobj->Step(&stmt, wyFalse);
	sqliteobj->Finalize(&stmt);						
	return wyTrue;
}

wyBool
FrameWindow::WriteTabDetailsToTable(tabeditorelem *temptabeditorele, tabeditorelem *temptabeditorele_temp, wyInt32 tabid, wyInt32 position, wyInt32 id, TabTypes *tabactive, wySQLite	*ssnsqliteobj)
{
	sqlite3_stmt*   stmt;
	TabEditor		*tabquery=NULL;
	wyString		sqlitequery;
	wySQLite	*sqliteobj;
	sqliteobj = ssnsqliteobj ? ssnsqliteobj : pGlobals->m_sqliteobj;
#ifndef COMMUNITY
//if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
	TabQueryBuilder		*tabquerybuilder;	
	TabSchemaDesigner	*tabschemadesigner;
#endif
	
	if(temptabeditorele_temp->m_iimage == IDI_QUERYBUILDER_16)
	{
#ifndef COMMUNITY
if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
	tabquerybuilder = (TabQueryBuilder*)temptabeditorele_temp->m_tabptr;
#endif
	}
	else
	if(temptabeditorele_temp->m_iimage == IDI_SCHEMADESIGNER_16)
	{
#ifndef COMMUNITY
if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
		tabschemadesigner = (TabSchemaDesigner*)temptabeditorele_temp->m_tabptr;
#endif
	}
	else
	{
	tabquery = (TabEditor*)temptabeditorele_temp->m_tabptr;
		temptabeditorele->m_pctabeditor = tabquery;
		
	}

	temptabeditorele->m_tabptr = temptabeditorele_temp->m_tabptr;	
	//tabquery = (TabEditor*)quetabitem.m_lparam;
	temptabeditorele->m_iimage = temptabeditorele_temp->m_iimage;
	temptabeditorele->m_id = id;
	temptabeditorele->m_ispresent = wyTrue;
	
	temptabeditorele->m_tabid = tabid;
	temptabeditorele->m_position = position;
	temptabeditorele->m_color = temptabeditorele_temp->m_color;
	temptabeditorele->m_fgcolor = temptabeditorele_temp->m_fgcolor;
	temptabeditorele->m_isfocussed = wyFalse;
	

	if((TabTypes*)temptabeditorele_temp->m_tabptr == tabactive)
			temptabeditorele->m_isfocussed = wyTrue;

	if(temptabeditorele_temp->m_iimage != IDI_QUERYBUILDER_16 && temptabeditorele_temp->m_iimage != IDI_SCHEMADESIGNER_16 )
	{
		temptabeditorele->m_isedited = temptabeditorele_temp->m_isedited;
		temptabeditorele->m_isfile = temptabeditorele_temp->m_isfile;
		temptabeditorele->m_leftortoppercent = temptabeditorele_temp->m_leftortoppercent ;

		/*
		if(!temptabeditorele->m_isfile || temptabeditorele->m_isedited )
				temptabeditorele->m_pctabeditor->m_peditorbase->GetCompleteTextByPost(testquery, wnd);
		else
			testquery.SetAs("");*/
	
	}

	else
	if(temptabeditorele_temp->m_iimage == IDI_SCHEMADESIGNER_16)
	{
#ifndef COMMUNITY
if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
{
		temptabeditorele->m_isedited = temptabeditorele_temp->m_isedited;
		temptabeditorele->m_isfile = temptabeditorele_temp->m_isfile;
		temptabeditorele->m_leftortoppercent = 50;

		/*if(!temptabeditorele->m_isfile || temptabeditorele->m_isedited )
			tabschemadesigner->WriteToXMLbuffer(testquery);
		else
			testquery.SetAs("");*/
}
#endif
	}
	else
	{
#ifndef COMMUNITY
if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
{
		temptabeditorele->m_isedited = temptabeditorele_temp->m_isedited;
		temptabeditorele->m_isfile =  temptabeditorele_temp->m_isfile;
		temptabeditorele->m_leftortoppercent = 50;
	/*if(!temptabeditorele->m_isfile || temptabeditorele->m_isedited )
			tabquerybuilder->WriteToXMLbuffer(testquery);
	else
		testquery.SetAs("");*/
}
#endif
	}
	temptabeditorele->m_psztext = temptabeditorele_temp->m_psztext;
	temptabeditorele->m_tooltiptext = temptabeditorele_temp->m_tooltiptext;
	temptabeditorele->m_content.SetAs(temptabeditorele_temp->m_content.GetString());

	//if(!ssnsqliteobj)
		
	sqlitequery.Sprintf("INSERT INTO tabdetails (Id, Tabid,Tabtype, position,leftortoppercent,isedited, title, tooltip, isfile,isfocussed,content) VALUES \
												(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

	sqliteobj->Prepare(&stmt, sqlitequery.GetString());

	//sqlitequery.Sprintf("INSERT INTO tabdetails (Id, Tabid, position,leftortoppercent,isedited, title, tooltip, isfile,isfocussed,content) 
	//VALUES (%d , %d, %d, %d, %d, \"%s\", \"%s\", %d, %d, \"%s\")",
	//tempmdilist->m_id,temptabeditorele->m_tabid,temptabeditorele->m_position,temptabeditorele->m_leftortoppercent,temptabeditorele->m_isedited,temptabeditorele->m_psztext.GetString(),temptabeditorele->m_tooltiptext.GetString(),temptabeditorele->m_isfile,temptabeditorele->m_isfocussed,testquery.GetString());
	sqliteobj->SetInt(&stmt, 1, id);
	sqliteobj->SetInt(&stmt, 2, temptabeditorele->m_tabid);
	sqliteobj->SetInt(&stmt, 3, temptabeditorele->m_iimage);
	sqliteobj->SetInt(&stmt, 4, temptabeditorele->m_position);
	sqliteobj->SetInt(&stmt, 5, temptabeditorele->m_leftortoppercent);
	sqliteobj->SetInt(&stmt, 6, temptabeditorele->m_isedited);
	sqliteobj->SetText(&stmt, 7, temptabeditorele->m_psztext.GetString());
	sqliteobj->SetText(&stmt, 8,	temptabeditorele->m_tooltiptext.GetString());
	sqliteobj->SetInt(&stmt, 9,	temptabeditorele->m_isfile);
	sqliteobj->SetInt(&stmt, 10,	temptabeditorele->m_isfocussed);
	sqliteobj->SetText(&stmt, 11,	temptabeditorele->m_content.GetString());
	
	sqliteobj->Step(&stmt, wyFalse);
	sqliteobj->Finalize(&stmt);						
	return wyTrue;
}

wyBool
FrameWindow::SaveConnectionDetails(wySQLite	*ssnsqliteobj)
{
	wyInt32 tabcount, i;
	CTCITEM contabitem = {0}, quetabitem = {0};
	contabitem.m_mask = CTBIF_LPARAM | CTBIF_COLOR;
	MDIWindow	*wnd, *wndactive;
 	wyBool isend = wyFalse, isendq = wyFalse,issessionedited = wyFalse;
	
	wyString	dirstr;
	
	tabeditorelem		*temptabeditorele, *deltabeditorele, *temptabeditorele_temp;
	MDIlist				*tempmdilist, *deltempmdilist;
	MDIlisttemp         *tempmdilist_temp;
	wyInt32				totalconntabs, j, k, l;
	wyInt32				subtabcount, totalquetabs;
	TabEditor			*tabquery=NULL;
#ifndef COMMUNITY
	TabQueryBuilder		*tabquerybuilder;	
	TabSchemaDesigner	*tabschemadesigner;
#endif
	wyString			sqlitequery,sqliteerr,tempstr;
	wyInt32				newid = 0,newqid = 0;
	sqlite3_stmt*		stmt;
	wySQLite			*sqliteobj;
	List				*mdiconlist;					

	sqliteobj = ssnsqliteobj ? ssnsqliteobj : pGlobals->m_sqliteobj;
	
	//---------------------------------------------Session Restore Acquire Lock-Creation of temporary linked list------------------------------------------------------------//

	wndactive = GetActiveWinByPost();
	if(!wndactive)
		return wyFalse;
	if(m_hwndconntab)
		{
			tabcount = CustomTab_GetItemCount(m_hwndconntab);
			for(i = 0; i < tabcount; i++)
			{
				if(i == 0)
					mdiconlist = new List();

				contabitem.m_mask = contabitem.m_mask |CTBIF_LPARAM|CTBIF_IMAGE| CTBIF_COLOR; 
				CustomTab_GetItem(m_hwndconntab, i, &contabitem);
				wnd = (MDIWindow*)contabitem.m_lparam;
				newid = newid + 1;
					tempmdilist_temp = new MDIlisttemp;
					tempmdilist_temp->m_id = newid;
					tempmdilist_temp->mdi = wnd;
					tempmdilist_temp->m_ispresent = wyTrue;
					tempmdilist_temp->m_position = i;
					tempmdilist_temp->m_rgbconn = wnd->m_conninfo.m_rgbconn;
					tempmdilist_temp->m_rgbfgconn = wnd->m_conninfo.m_rgbfgconn;
					tempmdilist_temp->m_obdetails.SetAs(wnd->m_pcqueryobject->m_seldatabase.GetString());

					if(wnd == wndactive)
						tempmdilist_temp->m_isfocussed = wyTrue;
					else
						tempmdilist_temp->m_isfocussed = wyFalse;

				tempmdilist_temp->m_tabhistory = wnd->GetActiveHistoryTab();
				tempmdilist_temp->m_tabhistory->GetCompleteTextByPost(tempmdilist_temp->m_historydata,wnd);
				subtabcount = CustomTab_GetItemCount(wnd->m_pctabmodule->m_hwnd);
				tempmdilist_temp->m_tabqueryactive = wnd->GetActiveTabEditor();
				tempmdilist_temp->m_activetab = wnd->m_pctabmodule->GetActiveTabType();

				mdiconlist->Insert(tempmdilist_temp);

				tempmdilist_temp->m_listtabeditor_temp = new List();

					for(k = 0; k < subtabcount; k++)
					{
						quetabitem.m_mask = quetabitem.m_mask |CTBIF_COLOR|CTBIF_LPARAM|CTBIF_IMAGE; 
						CustomTab_GetItem(wnd->m_pctabmodule->m_hwnd, k, &quetabitem);
						if(quetabitem.m_iimage == IDI_SCHEMADESIGNER_16 || quetabitem.m_iimage == IDI_QUERYBUILDER_16 || quetabitem.m_iimage == IDI_QUERY_16 || quetabitem.m_iimage == IDI_CREATEVIEW || quetabitem.m_iimage == IDI_CREATEEVENT|| quetabitem.m_iimage == IDI_CREATEPROCEDURE|| quetabitem.m_iimage == IDI_CREATEFUNCTION|| quetabitem.m_iimage == IDI_CREATETRIGGER || quetabitem.m_iimage == IDI_ALTERVIEW || quetabitem.m_iimage == IDI_ALTEREVENT || quetabitem.m_iimage == IDI_ALTERPROCEDURE || quetabitem.m_iimage == IDI_ALTERFUNCTION || quetabitem.m_iimage == IDI_ALTERTRIGGER )
						{
							
							temptabeditorele_temp = new tabeditorelem;
							WriteTabDetailsToTempList(temptabeditorele_temp, quetabitem, k, k, tempmdilist_temp->m_id, tempmdilist_temp->m_activetab,wnd);
							tempmdilist_temp->m_listtabeditor_temp->Insert(temptabeditorele_temp);
						}
					}
			
				}

			}



	//----------------------------------------------------Session Restore Release Lock--------------------------------------------------------//

	//------------------------------------------------------------Writing SQlite-------------------------------------------------------------//
	newid=0;
	newqid=0;
	sqlitequery.Sprintf("BEGIN IMMEDIATE");
	sqliteobj->Execute(&sqlitequery, &sqliteerr);
	if(m_hwndconntab)
		{
			if(m_sessionfile.GetLength())
			{
				if(WaitForSingleObject(m_sessionchangeevent, 0) == WAIT_OBJECT_0)
					issessionedited = wyTrue;
				else
					issessionedited = wyFalse;
				
				if(!m_previoussessionfile.GetLength())
				{
					m_previoussessionfile.SetAs(m_sessionfile);
					sqlitequery.Sprintf("INSERT INTO sessiondetails (Id, filename, isedited) VALUES (?, ?, ?)");
					sqliteobj->Prepare(&stmt, sqlitequery.GetString());
					sqliteobj->SetInt(&stmt, 1,	1);
					sqliteobj->SetText(&stmt, 2,	m_sessionfile.GetString());
					sqliteobj->SetInt(&stmt, 3,	issessionedited);
					sqliteobj->Step(&stmt, wyFalse);
					sqliteobj->Finalize(&stmt);
				}
				else
				if(m_previoussessionfile.Compare(m_sessionfile) != 0)
				{
					sqlitequery.Sprintf("UPDATE sessiondetails set filename = ? where Id = 1");
					sqliteobj->Prepare(&stmt, sqlitequery.GetString());
					sqliteobj->SetText(&stmt, 1,	m_sessionfile.GetString());
					sqliteobj->Step(&stmt, wyFalse);
					sqliteobj->Finalize(&stmt);
					m_previoussessionfile.SetAs(m_sessionfile);
					if((m_previoussessionedited && !issessionedited) || (!m_previoussessionedited && issessionedited))
					{
						sqlitequery.Sprintf("UPDATE sessiondetails set isedited = %d where Id = 1", issessionedited?1:0);
						sqliteobj->Execute(&sqlitequery, &sqliteerr);
					}
				}
			}
			else
			{
				if(m_previoussessionfile.GetLength())
				{
				sqlitequery.Sprintf("DELETE FROM sessiondetails");
				sqliteobj->Execute(&sqlitequery, &sqliteerr);
				m_previoussessionfile.SetAs("");
				}
			}

		totalconntabs = pGlobals->m_mdiwlist->GetCount();
			for(i = 0; i < totalconntabs; i++)
			{
				if(i == 0)
					tempmdilist = (MDIlist*)pGlobals->m_mdiwlist->GetFirst();

				tempmdilist->m_ispresent = wyFalse;
				//write a function to get new id
				if(tempmdilist->m_id > newid)
					newid = tempmdilist->m_id;

				tempmdilist = (MDIlist*)tempmdilist->m_next;
			}

		tabcount = mdiconlist->GetCount();
	for(i = 0; i < tabcount; i++)
		{
		if(i == 0)
			tempmdilist_temp = (MDIlisttemp*)mdiconlist->GetFirst();
		
		wnd	= tempmdilist_temp->mdi;
		subtabcount = tempmdilist_temp->m_listtabeditor_temp->GetCount();
		isend = wyFalse;

			for(j = 0; j < totalconntabs && !isend; j++)
				{
					if(j == 0)
						tempmdilist = (MDIlist*)pGlobals->m_mdiwlist->GetFirst();

					if(tempmdilist->mdi == wnd )
					{
						//mark as present
						tempmdilist->m_ispresent = wyTrue;
						if(tempmdilist->m_position != i)
						{
							tempmdilist->m_position = i;//position change update to sqlite based on tempmdilist->m_id & update list
							sqlitequery.Sprintf("UPDATE conndetails set position = %d where Id = %d", i, tempmdilist->m_id);
							sqliteobj->Execute(&sqlitequery, &sqliteerr);
						}
						//else
						isend = wyTrue;//end of loop matched mdi
						//check for color change 
						if(tempmdilist_temp->m_rgbconn != tempmdilist->m_rgbconn )
						{
							//update sqlite and list
							tempmdilist->m_rgbconn = tempmdilist_temp->m_rgbconn;
							sqlitequery.Sprintf("UPDATE conndetails set ObjectbrowserBkcolor = %d where Id = %d", tempmdilist->m_rgbconn, tempmdilist->m_id);
							sqliteobj->Execute(&sqlitequery, &sqliteerr);
						}
						if(tempmdilist_temp->m_rgbfgconn != tempmdilist->m_rgbfgconn )
						{
							//update sqlite and list
							tempmdilist->m_rgbfgconn = tempmdilist_temp->m_rgbfgconn;
							sqlitequery.Sprintf("UPDATE conndetails set ObjectbrowserFgcolor = %d where Id = %d", tempmdilist->m_rgbfgconn, tempmdilist->m_id);
							sqliteobj->Execute(&sqlitequery, &sqliteerr);
						}
						if(wnd == wndactive && !tempmdilist->m_isfocussed)
						{
							tempmdilist->m_isfocussed = wyTrue;
							sqlitequery.Sprintf("UPDATE conndetails set isfocussed = 1 where Id = %d", tempmdilist->m_id);
							sqliteobj->Execute(&sqlitequery, &sqliteerr);
						}
						else
						if(wnd != wndactive && tempmdilist->m_isfocussed)
						{
							tempmdilist->m_isfocussed = wyFalse;
							sqlitequery.Sprintf("UPDATE conndetails set isfocussed = 0 where Id = %d", tempmdilist->m_id);
							sqliteobj->Execute(&sqlitequery, &sqliteerr);
						}

						//Sleep(30000);
						if(tempmdilist_temp->m_historydata.Compare(tempmdilist->m_historydata)){

						tempmdilist->m_historydata.SetAs(tempmdilist_temp->m_historydata.GetString());
						sqlitequery.Sprintf("UPDATE historydetails set historydata = ? where Id = %d",tempmdilist->m_id);
						sqliteobj->Prepare(&stmt, sqlitequery.GetString());
						sqliteobj->SetText(&stmt, 1,tempmdilist->m_historydata.GetString());
						sqliteobj->Step(&stmt, wyFalse);
						sqliteobj->Finalize(&stmt);	
						}
						if(tempmdilist_temp->m_obdetails.Compare(tempmdilist->m_obdetails)){

							tempmdilist->m_obdetails.SetAs(tempmdilist_temp->m_obdetails.GetString());
						sqlitequery.Sprintf("UPDATE obdetails set obdb = ? where Id = %d", tempmdilist->m_id);
						sqliteobj->Prepare(&stmt, sqlitequery.GetString());
						sqliteobj->SetText(&stmt, 1,	tempmdilist->m_obdetails.GetString());
						sqliteobj->Step(&stmt, wyFalse);
						sqliteobj->Finalize(&stmt);	
					}
//--------------------------------------- Connection pointer found, Inside each connection list traversing query tab list----------------------------------------------------------------//
					//Sleep(30000);
					totalquetabs = tempmdilist->m_listtabeditor->GetCount();
					newqid = 0;
					for(k = 0; k < totalquetabs; k++)
					{
						if(k == 0)
							temptabeditorele = (tabeditorelem*)tempmdilist->m_listtabeditor->GetFirst();

						temptabeditorele->m_ispresent = wyFalse;

						//write a function to get new id
						if(temptabeditorele->m_tabid > newqid)
							newqid = temptabeditorele->m_tabid;
						temptabeditorele = (tabeditorelem*)temptabeditorele->m_next;
					}

					for(k = 0; k < subtabcount; k++)
					{
						if(k == 0)
							temptabeditorele_temp = (tabeditorelem*)tempmdilist_temp->m_listtabeditor_temp->GetFirst();

						if(temptabeditorele_temp->m_iimage == IDI_SCHEMADESIGNER_16 || temptabeditorele_temp->m_iimage == IDI_QUERYBUILDER_16 || temptabeditorele_temp->m_iimage == IDI_QUERY_16 || temptabeditorele_temp->m_iimage == IDI_CREATEVIEW || temptabeditorele_temp->m_iimage == IDI_CREATEEVENT|| temptabeditorele_temp->m_iimage == IDI_CREATEPROCEDURE|| temptabeditorele_temp->m_iimage == IDI_CREATEFUNCTION|| temptabeditorele_temp->m_iimage == IDI_CREATETRIGGER || temptabeditorele_temp->m_iimage == IDI_ALTERVIEW || temptabeditorele_temp->m_iimage == IDI_ALTEREVENT || temptabeditorele_temp->m_iimage == IDI_ALTERPROCEDURE || temptabeditorele_temp->m_iimage == IDI_ALTERFUNCTION || temptabeditorele_temp->m_iimage == IDI_ALTERTRIGGER )
						{
							if(temptabeditorele_temp->m_iimage == IDI_QUERYBUILDER_16)
							{
#ifndef COMMUNITY
if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
								tabquerybuilder = (TabQueryBuilder*)temptabeditorele_temp->m_tabptr;
#endif
							}
							else
							if(temptabeditorele_temp->m_iimage == IDI_SCHEMADESIGNER_16)
						{
#ifndef COMMUNITY
if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
								tabschemadesigner = (TabSchemaDesigner*)temptabeditorele_temp->m_tabptr;
#endif
							}
							else
							tabquery = (TabEditor*)temptabeditorele_temp->m_tabptr;
								
							isendq = wyFalse;
							for(l = 0; l < totalquetabs && !isendq; l++)
							{
								if(l == 0)
									temptabeditorele = (tabeditorelem*)tempmdilist->m_listtabeditor->GetFirst();
								//if((quetabitem.m_iimage == IDI_QUERY_16 && temptabeditorele->m_pctabeditor == tabquery) || (quetabitem.m_iimage == IDI_QUERYBUILDER_16 && temptabeditorele->m_pctabeditor == tabquerybuilder))
								if(temptabeditorele->m_tabptr == temptabeditorele_temp->m_tabptr)
								{
									//mark as present
									temptabeditorele->m_ispresent = wyTrue;
									isendq = wyTrue;//end of loop matched querytab
									if(temptabeditorele->m_position!=k)
									{
										temptabeditorele->m_position = k;//position change update to sqlite based on tempmdilist->m_id & update list
										sqlitequery.Sprintf("UPDATE tabdetails SET position = %d WHERE Id = %d AND Tabid = %d", k,tempmdilist->m_id, temptabeditorele->m_tabid);
										sqliteobj->Execute(&sqlitequery, &sqliteerr);
									}
									if(temptabeditorele->m_color != temptabeditorele_temp->m_color)
									{
										temptabeditorele->m_color = temptabeditorele_temp->m_color;
										sqlitequery.Sprintf("UPDATE tabdetails set color = %d where Id = %d AND Tabid = %d",temptabeditorele->m_color,tempmdilist->m_id, temptabeditorele->m_tabid);
										sqliteobj->Execute(&sqlitequery, &sqliteerr);
									}
									if(temptabeditorele->m_fgcolor != temptabeditorele_temp->m_fgcolor)
									{
										temptabeditorele->m_fgcolor = temptabeditorele_temp->m_fgcolor;
										sqlitequery.Sprintf("UPDATE tabdetails set fgcolor = %d where Id = %d AND Tabid = %d",temptabeditorele->m_fgcolor,tempmdilist->m_id, temptabeditorele->m_tabid);
										sqliteobj->Execute(&sqlitequery, &sqliteerr);
									}
									if(temptabeditorele->m_psztext.Compare(temptabeditorele_temp->m_psztext.GetString()) != 0 )
									{
										temptabeditorele->m_psztext.SetAs(temptabeditorele_temp->m_psztext.GetString());
										sqlitequery.Sprintf("UPDATE tabdetails set title = ? where Id = %d AND Tabid = %d",tempmdilist->m_id, temptabeditorele->m_tabid);

										sqliteobj->Prepare(&stmt, sqlitequery.GetString());
										sqliteobj->SetText(&stmt, 1,  temptabeditorele->m_psztext.GetString());
										sqliteobj->Step(&stmt, wyFalse);
										sqliteobj->Finalize(&stmt);	
									}

									if(temptabeditorele->m_tooltiptext.Compare(temptabeditorele_temp->m_tooltiptext.GetString()) != 0 )
									{
										temptabeditorele->m_tooltiptext.SetAs(temptabeditorele_temp->m_tooltiptext.GetString());
										sqlitequery.Sprintf("UPDATE tabdetails set tooltip = ? where Id = %d AND Tabid = %d",tempmdilist->m_id, temptabeditorele->m_tabid);
										sqliteobj->Prepare(&stmt, sqlitequery.GetString());
										sqliteobj->SetText(&stmt, 1,  temptabeditorele->m_tooltiptext.GetString());
										sqliteobj->Step(&stmt, wyFalse);
										sqliteobj->Finalize(&stmt);	
									}
									if((TabTypes*)temptabeditorele_temp->m_tabptr == tempmdilist_temp->m_activetab && !temptabeditorele->m_isfocussed)
									{
										temptabeditorele->m_isfocussed = wyTrue;
										sqlitequery.Sprintf("UPDATE tabdetails set isfocussed = 1 where Id = %d AND Tabid = %d", tempmdilist->m_id, temptabeditorele->m_tabid );
										sqliteobj->Execute(&sqlitequery, &sqliteerr);
									}
									else
									if((TabTypes*)temptabeditorele_temp->m_tabptr != tempmdilist_temp->m_activetab && temptabeditorele->m_isfocussed)
									{
										temptabeditorele->m_isfocussed = wyFalse;
										sqlitequery.Sprintf("UPDATE tabdetails set isfocussed = 0 where Id = %d AND Tabid = %d", tempmdilist->m_id, temptabeditorele->m_tabid );
										sqliteobj->Execute(&sqlitequery, &sqliteerr);
									}
									if(temptabeditorele_temp->m_iimage != IDI_QUERYBUILDER_16 && temptabeditorele_temp->m_iimage != IDI_SCHEMADESIGNER_16)
									{
										//pctabeditor = (TabEditor*)quetabitem.m_lparam;
										if(temptabeditorele->m_isedited != temptabeditorele_temp->m_isedited)
										{
											temptabeditorele->m_isedited = temptabeditorele_temp->m_isedited;
											sqlitequery.Sprintf("UPDATE tabdetails set isedited = %d where Id = %d AND Tabid = %d",temptabeditorele->m_isedited,tempmdilist->m_id, temptabeditorele->m_tabid);
											sqliteobj->Execute(&sqlitequery, &sqliteerr);
										}
										if(!temptabeditorele->m_isfile && temptabeditorele_temp->m_isfile)
										{
											temptabeditorele->m_isfile = wyTrue;
											sqlitequery.Sprintf("UPDATE tabdetails set isfile = 1 where Id = %d AND Tabid = %d",tempmdilist->m_id,temptabeditorele->m_tabid);
											sqliteobj->Execute(&sqlitequery, &sqliteerr);
										}
										
									if(!temptabeditorele->m_isfile || temptabeditorele->m_isedited )
									{
										if(temptabeditorele_temp->m_content.Compare(temptabeditorele->m_content))
										{
										temptabeditorele->m_content.SetAs(temptabeditorele_temp->m_content.GetString());
										sqlitequery.Sprintf("UPDATE tabdetails set content = ? where Id = %d AND Tabid = %d",tempmdilist->m_id, temptabeditorele->m_tabid);
										sqliteobj->Prepare(&stmt, sqlitequery.GetString());
										sqliteobj->SetText(&stmt, 1,  temptabeditorele->m_content.GetString());
										sqliteobj->Step(&stmt, wyFalse);
										sqliteobj->Finalize(&stmt);
										}
									}
									//temptabeditorele->m_leftortoppercent = tabquery->m_pcetsplitter->GetLeftTopPercent();
									if(temptabeditorele->m_leftortoppercent != temptabeditorele_temp->m_leftortoppercent)
									{
										temptabeditorele->m_leftortoppercent = temptabeditorele_temp->m_leftortoppercent;
										sqlitequery.Sprintf("UPDATE tabdetails set leftortoppercent = %d where Id = %d AND Tabid = %d", temptabeditorele->m_leftortoppercent, tempmdilist->m_id, temptabeditorele->m_tabid );
										sqliteobj->Execute(&sqlitequery, &sqliteerr);
									}
								}
									else
									if(temptabeditorele_temp->m_iimage == IDI_QUERYBUILDER_16)
									{
#ifndef COMMUNITY
if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
{
										if(temptabeditorele->m_isedited != temptabeditorele_temp->m_isedited)
										{
											temptabeditorele->m_isedited = temptabeditorele_temp->m_isedited;
											sqlitequery.Sprintf("UPDATE tabdetails set isedited = %d where Id = %d AND Tabid = %d",temptabeditorele->m_isedited,tempmdilist->m_id, temptabeditorele->m_tabid);
											sqliteobj->Execute(&sqlitequery, &sqliteerr);
										}
										if(!temptabeditorele->m_isfile && temptabeditorele_temp->m_isfile)
										{
											temptabeditorele->m_isfile = wyTrue;
											sqlitequery.Sprintf("UPDATE tabdetails set isfile = 1 where Id = %d AND Tabid = %d",tempmdilist->m_id,temptabeditorele->m_tabid);
											sqliteobj->Execute(&sqlitequery, &sqliteerr);
										}
										
										if(!temptabeditorele->m_isfile || temptabeditorele->m_isedited )
										{
										
											if(temptabeditorele_temp->m_content.Compare(temptabeditorele->m_content))
										{
										
											temptabeditorele->m_content.SetAs(temptabeditorele_temp->m_content.GetString());
											sqlitequery.Sprintf("UPDATE tabdetails set content = ? where Id = %d AND Tabid = %d",tempmdilist->m_id, temptabeditorele->m_tabid);
											sqliteobj->Prepare(&stmt, sqlitequery.GetString());
											sqliteobj->SetText(&stmt, 1,  temptabeditorele->m_content.GetString());
											sqliteobj->Step(&stmt, wyFalse);
											sqliteobj->Finalize(&stmt);	
										}
										}
}
#endif
									}
									else
									{
#ifndef COMMUNITY
if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
{
										if(temptabeditorele->m_isedited != temptabeditorele_temp->m_isedited)
										{
											temptabeditorele->m_isedited = temptabeditorele_temp->m_isedited;
											sqlitequery.Sprintf("UPDATE tabdetails set isedited = %d where Id = %d AND Tabid = %d",temptabeditorele->m_isedited,tempmdilist->m_id, temptabeditorele->m_tabid);
											sqliteobj->Execute(&sqlitequery, &sqliteerr);
										}
										if(!temptabeditorele->m_isfile && temptabeditorele_temp->m_isfile)
										{
											temptabeditorele->m_isfile = wyTrue;
											sqlitequery.Sprintf("UPDATE tabdetails set isfile = 1 where Id = %d AND Tabid = %d",tempmdilist->m_id,temptabeditorele->m_tabid);
											sqliteobj->Execute(&sqlitequery, &sqliteerr);
										}
										if(!temptabeditorele->m_isfile || temptabeditorele->m_isedited )
										{
											
											if(temptabeditorele_temp->m_content.Compare(temptabeditorele->m_content))
										{
											temptabeditorele->m_content.SetAs(temptabeditorele_temp->m_content.GetString());
											sqlitequery.Sprintf("UPDATE tabdetails set content = ? where Id = %d AND Tabid = %d",tempmdilist->m_id, temptabeditorele->m_tabid);
											sqliteobj->Prepare(&stmt, sqlitequery.GetString());
											sqliteobj->SetText(&stmt, 1,  temptabeditorele->m_content.GetString());
											sqliteobj->Step(&stmt, wyFalse);
											sqliteobj->Finalize(&stmt);	
										}
										}
}
#endif
									}
								}
								temptabeditorele = (tabeditorelem*)temptabeditorele->m_next;
								//flag as new tab
							}
							//--------------------------Outside temptabeditorele linked list-----------------------//

							
							if(l == totalquetabs && !isendq)		//------------------------new query tab found-----------------//
							{
								//save in list and sqlite
								newqid = newqid + 1;
								temptabeditorele = new tabeditorelem;
								WriteTabDetailsToTable(temptabeditorele, temptabeditorele_temp, newqid, k, tempmdilist->m_id, tempmdilist_temp->m_activetab);
								tempmdilist->m_listtabeditor->Insert(temptabeditorele);

							}
						}

						temptabeditorele_temp = (tabeditorelem*)temptabeditorele_temp->m_next;
					}
					//--------------------------Outside temptabeditorele_temp liinked list-----------------------//



					for(k = 0; k < totalquetabs; k++)
					{
						if(k == 0)
							temptabeditorele = (tabeditorelem*)tempmdilist->m_listtabeditor->GetFirst();

						if(temptabeditorele->m_ispresent == wyFalse)
						{
							//drop the row , dont process its query tabs
							sqlitequery.Sprintf("DELETE FROM tabdetails WHERE Id = %d AND Tabid = %d",temptabeditorele->m_id, temptabeditorele->m_tabid);
							sqliteobj->Execute(&sqlitequery, &sqliteerr);
							deltabeditorele = temptabeditorele;
							temptabeditorele = (tabeditorelem*)temptabeditorele->m_next;
							tempmdilist->m_listtabeditor->Remove(deltabeditorele);
							delete deltabeditorele;
						}
						else
							temptabeditorele = (tabeditorelem*)temptabeditorele->m_next;
					}
				


	//-------------------------------------------Updating Query tab list completed-------------------------------------------------//

				}
					if(!isend)
						tempmdilist = (MDIlist*)tempmdilist->m_next;
					
			}
  //-----------------------------------------------------Outside Tempmdilist------------------------------------------------------//

 //-------------------------------------------------New Connection Found--------------------------------------------------------//
			if(j == totalconntabs && !isend)
				{
					//write to sqlite connection attr which will not change throughout the entire session
					//process all subtabs
					//save all conn details, list and sqlite
					newid = newid + 1;
					tempmdilist = new MDIlist;
					tempmdilist->m_id = newid;
					tempmdilist->mdi = wnd;
					tempmdilist->m_ispresent = wyTrue;
					tempmdilist->m_position = i;
					tempmdilist->m_rgbconn = tempmdilist_temp->m_rgbconn;
					tempmdilist->m_rgbfgconn = tempmdilist_temp->m_rgbfgconn;
					tempmdilist->m_obdetails.SetAs(tempmdilist_temp->m_obdetails);
					tempmdilist->m_historydata.SetAs(tempmdilist_temp->m_historydata.GetString());

					if(wnd == wndactive)
						tempmdilist->m_isfocussed = wyTrue;
					else
						tempmdilist->m_isfocussed = wyFalse;
					sqlitequery.SetAs("");
					WriteFullSectionToTable(&sqlitequery, tempmdilist->m_id, tempmdilist->m_position, &wnd->m_conninfo, wnd->m_title.GetString(), tempmdilist->m_isfocussed);
					//write history

					sqlitequery.Sprintf("INSERT INTO historydetails (Id, historydata) VALUES (? , ?)");
					sqliteobj->Prepare(&stmt, sqlitequery.GetString());
					sqliteobj->SetInt(&stmt, 1,	tempmdilist->m_id);
					sqliteobj->SetText(&stmt, 2,	tempmdilist->m_historydata.GetString());
					sqliteobj->Step(&stmt, wyFalse);
					sqliteobj->Finalize(&stmt);	

					sqlitequery.Sprintf("INSERT INTO obdetails (Id, obdb) VALUES (? , ?)");
					sqliteobj->Prepare(&stmt, sqlitequery.GetString());
					sqliteobj->SetInt(&stmt, 1,	tempmdilist->m_id);
					sqliteobj->SetText(&stmt, 2,tempmdilist->m_obdetails.GetString());
					sqliteobj->Step(&stmt, wyFalse);
					sqliteobj->Finalize(&stmt);	

					tempmdilist->m_listtabeditor = new List();
					for(k = 0; k < subtabcount; k++)
					{
						if(k==0)
							temptabeditorele_temp =(tabeditorelem*)tempmdilist_temp->m_listtabeditor_temp->GetFirst();
						if(temptabeditorele_temp->m_iimage == IDI_SCHEMADESIGNER_16 || temptabeditorele_temp->m_iimage == IDI_QUERYBUILDER_16 || temptabeditorele_temp->m_iimage == IDI_QUERY_16 || temptabeditorele_temp->m_iimage == IDI_CREATEVIEW || quetabitem.m_iimage == IDI_CREATEEVENT|| temptabeditorele_temp->m_iimage == IDI_CREATEPROCEDURE|| temptabeditorele_temp->m_iimage == IDI_CREATEFUNCTION|| temptabeditorele_temp->m_iimage == IDI_CREATETRIGGER || temptabeditorele_temp->m_iimage == IDI_ALTERVIEW || temptabeditorele_temp->m_iimage == IDI_ALTEREVENT || temptabeditorele_temp->m_iimage == IDI_ALTERPROCEDURE ||  temptabeditorele_temp->m_iimage == IDI_ALTERFUNCTION ||  temptabeditorele_temp->m_iimage == IDI_ALTERTRIGGER )
						{
							
							temptabeditorele = new tabeditorelem;
							WriteTabDetailsToTable(temptabeditorele, temptabeditorele_temp, k, k, tempmdilist->m_id, tempmdilist_temp->m_activetab);
							tempmdilist->m_listtabeditor->Insert(temptabeditorele);							
						}
						temptabeditorele_temp = (tabeditorelem*)temptabeditorele_temp->m_next;
					}
					pGlobals->m_mdiwlist->Insert(tempmdilist);
				}

//---------------------------------------------------------New Connecton Added in the linked List------------------------------------//

			tempmdilist_temp = (MDIlisttemp*)tempmdilist_temp->m_next; //----Move to next connection window----//
			}

			for(i = 0; i < totalconntabs; i++)
			{
				if(i==0)
						tempmdilist = (MDIlist*)pGlobals->m_mdiwlist->GetFirst();
				if(tempmdilist->m_ispresent == wyFalse)
				{
					//drop the row , dont process its query tabs
					sqlitequery.Sprintf("DELETE FROM conndetails WHERE Id = %d",tempmdilist->m_id);
					sqliteobj->Execute(&sqlitequery, &sqliteerr);
					deltempmdilist = tempmdilist;
					tempmdilist = (MDIlist*)tempmdilist->m_next;
					pGlobals->m_mdiwlist->Remove(deltempmdilist);
					delete deltempmdilist;
				}
				else
					tempmdilist = (MDIlist*)tempmdilist->m_next;
			}
		}

		else
		{
			sqlitequery.Sprintf("DELETE FROM conndetails");
			sqliteobj->Execute(&sqlitequery, &sqliteerr);
			//tempmdilist = (MDIlist*)pGlobals->m_mdiwlist->GetFirst();
			/*if(pGlobals->m_mdiwlist){
				delete pGlobals->m_mdiwlist;
				pGlobals->m_mdiwlist = NULL;
			}*/
			tempmdilist = (MDIlist*)pGlobals->m_mdiwlist->GetFirst();
			totalconntabs = pGlobals->m_mdiwlist->GetCount();
			for(i = 0; i < totalconntabs; i++)
			{
				deltempmdilist = tempmdilist;
				tempmdilist = (MDIlist*)tempmdilist->m_next;
				pGlobals->m_mdiwlist->Remove(deltempmdilist);
				delete deltempmdilist;
			}


		}
		sqlitequery.Sprintf("COMMIT");
		sqliteobj->Execute(&sqlitequery, &sqliteerr);
	//------------------------------------------------------------Writing SQlite completed--------------------------------------------------//
	

	//----------------------Deleting temporary connection and query tab list---------------------------------------//
	if(m_hwndconntab)
	{
		if(mdiconlist){
		tabcount = mdiconlist->GetCount();
		for(i = 0; i < tabcount; i++)
		{
		if(i == 0)
			tempmdilist_temp = (MDIlisttemp*)mdiconlist->GetFirst();

		if(tempmdilist_temp->m_listtabeditor_temp)
			{
				delete tempmdilist_temp->m_listtabeditor_temp;
				tempmdilist_temp->m_listtabeditor_temp = NULL;
			}

		tempmdilist_temp = (MDIlisttemp*)tempmdilist_temp->m_next;

		}

		delete mdiconlist;
		mdiconlist = NULL;
		}
		
	}
		
	return wyTrue;
	}


wyBool
FrameWindow::SaveConnectionDetails2(wySQLite	*ssnsqliteobj)
{
	wyInt32 tabcount, i;
	CTCITEM contabitem = {0}, quetabitem = {0};
	contabitem.m_mask = CTBIF_LPARAM | CTBIF_COLOR;
	MDIWindow	*wnd, *wndactive;
 	wyBool isend = wyFalse;//, isendq = wyFalse
	
	wyString	dirstr;
	
	tabeditorelem		*temptabeditorele;//, *deltabeditorele
	MDIlist				*tempmdilist;//, *deltempmdilist
	wyString			 historydata;
	wyInt32				 k;//, j, l,totalconntabs,
	wyInt32				subtabcount;//, totalquetabs
	TabEditor			*tabqueryactive;//*tabquery,
	TabHistory			*tabhistory;	
#ifndef COMMUNITY
//	TabQueryBuilder		*tabquerybuilder;	
#endif
	wyString			sqlitequery,sqliteerr,tempstr;
	wyInt32				newid = 0;//,newqid = 0
	sqlite3_stmt*		stmt;
	TabTypes			*activetab;
	wySQLite			*sqliteobj;

	sqliteobj = ssnsqliteobj;
	
	/*if(m_sqlyogclosed)
		return wyTrue;*/
	wndactive = GetActiveWinByPost();
	if(!wndactive)
		return wyFalse;
	/*if(IsConnectionRestore())
	{*/
		//calculate next id for a new tab
		sqlitequery.Sprintf("BEGIN IMMEDIATE");
		sqliteobj->Execute(&sqlitequery, &sqliteerr);
		if(m_hwndconntab)
		{
			tabcount = CustomTab_GetItemCount(m_hwndconntab);

			for(i = 0; i < tabcount; i++)
			{
				contabitem.m_mask = contabitem.m_mask |CTBIF_LPARAM|CTBIF_IMAGE| CTBIF_COLOR; 
				CustomTab_GetItem(m_hwndconntab, i, &contabitem);
				wnd = (MDIWindow*)contabitem.m_lparam;

				tabhistory = wnd->GetActiveHistoryTab();
				tabhistory->GetCompleteTextByPost(historydata, wnd);

				isend = wyFalse;

				subtabcount = CustomTab_GetItemCount(wnd->m_pctabmodule->m_hwnd);
				tabqueryactive = wnd->GetActiveTabEditor();
				activetab = wnd->m_pctabmodule->GetActiveTabType();
				/*if(!tabqueryactive)
					tabquerybuilderactive = wnd->g*/
				//if(j == totalconntabs && !isend)
				//{
					//write to sqlite connection attr which will not change throughout the entire session
					//process all subtabs
					//save all conn details, list and sqlite
					newid = newid + 1;
					tempmdilist = new MDIlist;
					tempmdilist->m_id = newid;
					tempmdilist->mdi = wnd;
					tempmdilist->m_ispresent = wyTrue;
					tempmdilist->m_position = i;
					tempmdilist->m_rgbconn = wnd->m_conninfo.m_rgbconn;
					tempmdilist->m_rgbfgconn = wnd->m_conninfo.m_rgbfgconn;
					if(wnd == wndactive)
						tempmdilist->m_isfocussed = wyTrue;
					else
						tempmdilist->m_isfocussed = wyFalse;
					sqlitequery.SetAs("");
					WriteFullSectionToTable(&sqlitequery, tempmdilist->m_id, tempmdilist->m_position, &wnd->m_conninfo, wnd->m_title.GetString(), tempmdilist->m_isfocussed, sqliteobj);
					//write history
					//pGlobals->m_mdiwlist->Insert(tempmdilist);

					sqlitequery.Sprintf("INSERT INTO historydetails (Id, historydata) VALUES (? , ?)");
					sqliteobj->Prepare(&stmt, sqlitequery.GetString());
					sqliteobj->SetInt(&stmt, 1,	tempmdilist->m_id);
					sqliteobj->SetText(&stmt, 2,	historydata.GetString());
					sqliteobj->Step(&stmt, wyFalse);
					sqliteobj->Finalize(&stmt);	

					tempmdilist->m_historydata.SetAs(historydata.GetString());

					sqlitequery.Sprintf("INSERT INTO obdetails (Id, obdb) VALUES (? , ?)");
					sqliteobj->Prepare(&stmt, sqlitequery.GetString());
					sqliteobj->SetInt(&stmt, 1,	tempmdilist->m_id);
					sqliteobj->SetText(&stmt, 2,	wnd->m_pcqueryobject->m_seldatabase.GetString());
					sqliteobj->Step(&stmt, wyFalse);
					sqliteobj->Finalize(&stmt);	

					tempmdilist->m_obdetails.SetAs(wnd->m_pcqueryobject->m_seldatabase.GetString());

					for(k = 0; k < subtabcount; k++)
					{
						quetabitem.m_mask = quetabitem.m_mask |CTBIF_COLOR|CTBIF_LPARAM|CTBIF_IMAGE; 
						CustomTab_GetItem(wnd->m_pctabmodule->m_hwnd, k, &quetabitem);
						if(quetabitem.m_iimage == IDI_SCHEMADESIGNER_16 || quetabitem.m_iimage == IDI_QUERYBUILDER_16 || quetabitem.m_iimage == IDI_QUERY_16 || quetabitem.m_iimage == IDI_CREATEVIEW || quetabitem.m_iimage == IDI_CREATEEVENT|| quetabitem.m_iimage == IDI_CREATEPROCEDURE|| quetabitem.m_iimage == IDI_CREATEFUNCTION|| quetabitem.m_iimage == IDI_CREATETRIGGER || quetabitem.m_iimage == IDI_ALTERVIEW || quetabitem.m_iimage == IDI_ALTEREVENT || quetabitem.m_iimage == IDI_ALTERPROCEDURE || quetabitem.m_iimage == IDI_ALTERFUNCTION || quetabitem.m_iimage == IDI_ALTERTRIGGER )
						{
							
							temptabeditorele = new tabeditorelem;
							WriteTabDetailsToTable2(temptabeditorele, quetabitem, k, k, tempmdilist->m_id, activetab,  wnd, sqliteobj);

						}
					}
				}
		}

		sqlitequery.Sprintf("COMMIT");
		sqliteobj->Execute(&sqlitequery, &sqliteerr);
	/*}
	else
		return wyFalse;*/
	
	
	return wyTrue;
}

wyBool
FrameWindow::OpenSessionFile()
{
	
	wyInt32			ret = 0, retcls = 0;
//    wyBool          retval = wyFalse;
	wyWChar			openfilename[MAX_PATH + 1] = {0};
	wyString		failedconnections, filename;
	wyWChar			fname[MAX_PATH] = {0}, ext[MAX_PATH] = {0};
	wyString		file, extn;
	MDIWindow		*wnd;
	HANDLE			hfile;
	RECT        rc;
	wyInt32     lstyle = 0;
	//pause session save
	SetEvent(m_savetimerevent);
	ResetEvent(m_sqlyogcloseevent);
	//Get File 'open' dialog
	ret = InitOpenFile(pGlobals->m_hwndclient, openfilename, SESSIONINDEX, MAX_PATH);

	if(!ret)
		return wyFalse;
	
	filename.SetAs(openfilename);
	hfile = CreateFile(filename.GetAsWideChar(), GENERIC_READ, 
						FILE_SHARE_READ, NULL, 
						OPEN_EXISTING, 
						FILE_ATTRIBUTE_NORMAL, NULL);                     

	if(hfile == INVALID_HANDLE_VALUE)
	{
        DisplayErrorText(GetLastError(), _("Could not open file."), pGlobals->m_hwndclient);
		return wyFalse;
	}
	VERIFY (CloseHandle(hfile));
	//wait for session save to pause
	if(pGlobals->m_sessionrestore)
	{
		if(pGlobals->m_issessionsaveactive == wyTrue)
		{
			WaitForSingleObject (m_sessionsaveevent, INFINITE);
			//restore connections
		}
	}

	GetWindowRect(m_hwndmain, &rc);
	
	// bug if the window is in minimized position then wrong values are entered and you cannot 
	// change the sizing...so we need to see that only correct values are entered
    lstyle = GetWindowLongPtr(m_hwndmain, GWL_STYLE);

	if(lstyle & WS_MINIMIZE)
    {
        rc.left     = 0;
        rc.top      = 0;
        rc.bottom   = 600;
        rc.right    = 600;
    }
	if(lstyle & WS_MAXIMIZE)
	{
		m_showwindowstyle = SW_MAXIMIZE;
	}
	else
	{
		m_showwindowstyle = SW_NORMAL;
	}

	//purge autosessionsave db and the session lists
	/*delete pGlobals->m_mdiwlist;*/
	if(pGlobals->m_mdiwlist){
		delete pGlobals->m_mdiwlist;
		pGlobals->m_mdiwlist = NULL;
	}
	pGlobals->m_mdiwlist = new List();
	//close all connections
	m_iscloseallmdi = wyTrue;
	SendMessage(m_hwndmain, WM_CLOSEALLWINDOW, 0,(LPARAM)&retcls);
	//restore connections
	//pGlobals->m_pcmainwin->m_showwindowstyle = SW_NORMAL;
	ShowWindow(m_hwndmain, SW_HIDE);
	SetTimer(m_hwndmain, CONRESTORE_TIMER, 500, NULL);
	ConnectFromList(&failedconnections, &filename);
	MoveToInitPosByrc(m_hwndmain, &rc);
	//restart session save
	SetEvent(m_sqlyogcloseevent);
	m_iscloseallmdi = wyFalse;
	m_sessionfile.SetAs(filename);
	_wsplitpath(filename.GetAsWideChar(), NULL, NULL, fname, ext);
	m_sessionname.SetAs(fname);
	wnd = GetActiveWin();
	if(wnd)
		wnd->SetQueryWindowTitle();
	if(failedconnections.GetLength())
	{
		failedconnections.Insert(0, _("Failed to restore following connections\r\n\r\n"));
		MessageBox(m_hwndmain, failedconnections.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONINFORMATION | MB_OK);
	}
	return wyTrue;
}

wyBool
FrameWindow::SaveSessionFile(HWND hwnd, wyBool issaveas)
{
	wyString    file;
	wyWChar     filename[MAX_PATH+1] = {0};
	wyString	buffer;
//	DWORD		dwbyteswritten;
//	HANDLE		hfile;
	wyBool		ret;
	wySQLite	*sqliteobj;
	//#define  SIZE_8K		(8*1024)
	buffer.SetAs("");
	wyBool		isopened;
	HANDLE				hfind;
    WIN32_FIND_DATAW	fdata;
	wyString			sqlitequery,sqliteerr;
	wyWChar			fname[MAX_PATH] = {0}, ext[MAX_PATH] = {0};
	MDIWindow *wnd;
	
	if(!m_sessionfile.GetLength() || issaveas == wyTrue)
	{
		if(InitFile(hwnd, filename, SESSIONINDEX, MAX_PATH))
		{  
			file.SetAs(filename);
			if(file.GetLength() == 0)
				return wyFalse;
			m_sessionfile.SetAs(file);
			_wsplitpath(filename, NULL, NULL, fname, ext);
			m_sessionname.SetAs(fname);
			//extn.SetAs(ext);
		}
		else
			return wyFalse;
	}
	else
		file.SetAs(m_sessionfile);

	hfind = FindFirstFile(file.GetAsWideChar(), &fdata);
	sqliteobj = new wySQLite;
	isopened = sqliteobj->Open(file.GetString());
	if(hfind != INVALID_HANDLE_VALUE && isopened)
	{
		sqlitequery.Sprintf("DROP TABLE IF EXISTS schema_version");
		sqliteobj->Execute(&sqlitequery, &sqliteerr);
		sqlitequery.Sprintf("DROP TABLE IF EXISTS conndetails");
		sqliteobj->Execute(&sqlitequery, &sqliteerr);
		sqlitequery.Sprintf("DROP TABLE IF EXISTS historydetails");
		sqliteobj->Execute(&sqlitequery, &sqliteerr);
		sqlitequery.Sprintf("DROP TABLE IF EXISTS obdetails");
		sqliteobj->Execute(&sqlitequery, &sqliteerr);
		sqlitequery.Sprintf("DROP TABLE IF EXISTS tabdetails");
		sqliteobj->Execute(&sqlitequery, &sqliteerr);
	}
	if(isopened)
	{
		//create tables
		sqlitequery.Sprintf("PRAGMA foreign_keys = ON");
		sqliteobj->Execute(&sqlitequery, &sqliteerr);
		sqlitequery.Sprintf(TABLE_SCHEMADETAILS);
		sqliteobj->Execute(&sqlitequery, &sqliteerr);
		//insert schema_version values
		sqlitequery.Sprintf(TABLE_CONNDETAILS);
		sqliteobj->Execute(&sqlitequery, &sqliteerr);

		sqlitequery.Sprintf(TABLE_HISTORYDETAILS);
		sqliteobj->Execute(&sqlitequery, &sqliteerr);

		sqlitequery.Sprintf(TABLE_OBDETAILS);
		sqliteobj->Execute(&sqlitequery, &sqliteerr);
		//Id, Tabid, position, title, tooltip, isfile,isfocussed,content && tabtype? also issaved?
		sqlitequery.Sprintf(TABLE_TABDETAILS);
		sqliteobj->Execute(&sqlitequery, &sqliteerr);
		sqlitequery.Sprintf("INSERT INTO schema_version(description, major_version, minor_version) VALUES (\"%s\",\"%s\",\"%s\")",SCHEMA_DESCRIPTION, SCHEMA_MAJOR_VERSION, SCHEMA_MINOR_VERSION);
		sqliteobj->Execute(&sqlitequery, &sqliteerr);
		//Saveconnectiondetails
		ret = SaveConnectionDetails2(sqliteobj);
		sqliteobj->Close();
			
		if(ret == wyFalse)
		{
			DisplayErrorText(GetLastError(), _("Error writing in file."));
			//if(hfile)
				//VERIFY (CloseHandle(hfile));
			return wyFalse;
		}
		//VERIFY (CloseHandle(hfile));
	}
	else
	{
		DisplayErrorText(GetLastError(), _("Could not create file !"));
		return wyFalse;
	}

	wnd = GetActiveWin();
	wnd->SetQueryWindowTitle();
	
	return wyTrue;
}

unsigned __stdcall  
FrameWindow::RestoreStatusThreadProc(LPVOID lpparam)
{
	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_RESTORE_STATUS), NULL, FrameWindow::RestoreStatusDlgProc, (LPARAM)lpparam);    
    _endthreadex(0);
    return 0;
}

INT_PTR CALLBACK 
FrameWindow::RestoreStatusDlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    HICON		hicon;
    HMENU		hmenu;  
	wyString*	ptemp;

	switch(message) 
	{
		case WM_INITDIALOG: 
            pGlobals->m_pcmainwin->m_hwndrestorestatus = hwnd;
            hicon = LoadIcon(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_MAIN));
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon);
            DestroyIcon(hicon);
            hmenu = GetSystemMenu(hwnd, FALSE);
            EnableMenuItem(hmenu, SC_CLOSE, MF_BYCOMMAND | MF_DISABLED);
			SendMessage(GetDlgItem(hwnd, IDC_RESTORE_PROGRESS), PBM_SETRANGE32, 0, lparam);
			SendMessage(GetDlgItem(hwnd, IDC_RESTORE_PROGRESS), PBM_SETSTEP, 1, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RESTORE_PROGRESS), PBM_SETPOS, 0, 0);
            SetWindowText(hwnd, pGlobals->m_appname.GetAsWideChar());
			SetWindowText(GetDlgItem(hwnd, IDC_CONNECTION_NAME), _(L"Restoring session"));
			break;

		case UM_UPDATE_PROGRESS:
			SendMessage(GetDlgItem(hwnd, IDC_RESTORE_PROGRESS), PBM_STEPIT, 0, 0);
			return TRUE;

		case UM_UPDATE_CONNECTION:
			ptemp = new wyString();
			ptemp->Sprintf(_("Restoring %s"), (wyChar*)lparam);
			free((wyChar*)lparam);
			SetWindowText(GetDlgItem(hwnd, IDC_CONNECTION_NAME), ptemp->GetAsWideChar());
			delete ptemp;
			return TRUE;

		case UM_CLOSE_RESTORE_STATUS:
			EndDialog(hwnd, 0);
			return TRUE;
	}

	return FALSE;
}

wyBool
ConnectFromList(wyString* failedconnections, wyString* sessionfile)
{
	ConnectionInfo	conninfo;
	MDIWindow		*pcquerywnd = NULL;
	wyInt32         focussedconn = -1, totalcon = 0, i = 0,j = 0,totaltabs = 0;
	wyUInt32		threadid;
    
	wyString		pathstr, connstr, sectionstring,temptest;
	
	HANDLE *thread_handle_l;
	MY_ARG *my_arg;
	wyIni inimgr;
	
	wyBool	isfileopen = wyTrue/*, isinitfired=wyFalse*/;
	
    wyWChar				directory[MAX_PATH+64] = {0},directorybackup[MAX_PATH+64] = {0};
    wyInt32				focuspos,firstindex = 0;
    HANDLE				hfind;
    WIN32_FIND_DATAW	fdata;
    wyString			directoryname,directorynamebackup,jobbuff,seldatabase;
	wySQLite			*sqliteobj;
	wyString			sqlitequery,sqliteerr;
	sqlite3_stmt    *res;
	tabdetailelem  *temptabdetail;
	tabeditorelem  *temptabeditorele;
	TabHistory *tabhistory;

#ifndef COMMUNITY
	TiXmlDocument		*doc;
	TabQueryBuilder *tabquerybuilder;
    TabSchemaDesigner	*tabschemadesigner;
#endif
	
	//	HTREEITEM hitem;
	const wyChar    *colval = NULL;

	//if(sessionfile == NULL)
	//{
	if(pGlobals->m_configdirpath.GetLength())
	{
		wcscat(directory, pGlobals->m_configdirpath.GetAsWideChar());
		wcscat(directory, L"\\connrestore.db");
		wcscat(directorybackup, pGlobals->m_configdirpath.GetAsWideChar());
		wcscat(directorybackup, L"\\connrestore_backup.ysav");
	}
	else
	{
		if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, directory)))
		{
			wcscat(directory, L"\\SQLyog\\");
			wcscat(directorybackup, directory);
			wcscat(directory, L"connrestore.db");	
			wcscat(directorybackup, L"connrestore_backup.ysav");
		}
		else 
			return wyFalse;
	}
	if(sessionfile != NULL)
	{
		directoryname.SetAs(sessionfile->GetString());
	}
	else
	{
		hfind = FindFirstFile(directory, &fdata);	
		if(hfind == INVALID_HANDLE_VALUE)
			{
			//no db found
			//done
			//pGlobals->m_pcmainwin->CreateConnDialog();
			return wyFalse;
			}
		directoryname.SetAs(directory);
		directorynamebackup.SetAs(directorybackup);
	}
	//}
	
	sqliteobj = new wySQLite();
	//sqliteobj->Open(directoryname.GetString());
	sqliteobj->Open(directoryname.GetString(), wyTrue);

	sqlitequery.SetAs("SELECT count(Id) from conndetails");
	sqliteobj->Prepare(&res, sqlitequery.GetString());
	if(sqliteobj->Step(&res, wyFalse) && sqliteobj->GetLastCode() == SQLITE_ROW)
	{
		totalcon = sqliteobj->GetInt(&res, 0);
	}
	sqliteobj->Finalize(&res);
	thread_handle_l=new HANDLE[totalcon];
	my_arg = new MY_ARG[totalcon];

	sqlitequery.SetAs("SELECT Id FROM conndetails ORDER BY position");
	sqliteobj->Prepare(&res, sqlitequery.GetString());

	i = 0;
	while(sqliteobj->Step(&res, wyFalse) && sqliteobj->GetLastCode() == SQLITE_ROW)
	{
		my_arg[i].id = sqliteobj->GetInt(&res, 0);
		i = i + 1;
	}
	sqliteobj->Finalize(&res);
	
	if(i == 0)
		return wyFalse;
	//if(GetSessionFile(path) == wyTrue)
	//{
		pathstr.SetAs(directoryname);

		i = 0;
		pGlobals->m_conrestore = wyTrue;
		//sectionstring.StripToken(seps, &connstr);
		while(i < totalcon)
		{
			if(i == 0)
			{
				_beginthreadex(NULL, 0, FrameWindow::RestoreStatusThreadProc, (void*)totalcon, 0, &threadid);
			}

			//my_arg[i].connstr.SetAs(connstr);
			my_arg[i].pathstr.SetAs(pathstr);
			my_arg[i].tempmdilist = new MDIlist;
			my_arg[i].tabdetails = new List;
			//my_arg[i].inimgr = &inimgr;
			thread_handle_l[i] = (HANDLE)_beginthreadex(NULL, 0, ConnectFromList_mt,(void*) &my_arg[i], 0, NULL);
			++i;
			//sectionstring.StripToken(seps, &connstr); 

		}
				
		for(j=0;j<totalcon;j++)
			{
			WaitForSingleObject(thread_handle_l[j], INFINITE);
			CloseHandle(thread_handle_l[j]);
			
			PostMessage(pGlobals->m_pcmainwin->m_hwndrestorestatus, UM_UPDATE_PROGRESS, 0, 0);
			if(!my_arg[j].conninfo.m_mysql )
			{
				failedconnections->AddSprintf("%s\r\n", my_arg[j].conninfo.m_title.GetString());
				pGlobals->m_mdiwlist->Insert(my_arg[j].tempmdilist);
				continue;
			}
			/*else
			{
				
				if(isinitfired==wyFalse)
				{
				mysql_init((MYSQL*)0);
				isinitfired=wyTrue;
				}
				
			}*/
			if(pGlobals->m_pcmainwin->m_hwndconntab == NULL)
			{
				//create connection tab
				pGlobals->m_pcmainwin->m_hwndconntab = pGlobals->m_pcmainwin->m_conntab->CreateConnectionTabControl(pGlobals->m_pcmainwin->GetHwnd());
			}

			pcquerywnd	= new MDIWindow(pGlobals->m_pcmainwin->GetMDIWindow(), &my_arg[j].conninfo, my_arg[j].conninfo.m_db, my_arg[j].conninfo.m_title);
			if(pcquerywnd)
			{
				pcquerywnd->m_listtabdetails = my_arg[j].tabdetails;
				pcquerywnd->m_postactivatemsg = wyFalse;
				pGlobals->m_pcmainwin->m_connection->m_rgbobbkcolor = my_arg[j].conninfo.m_rgbconn;
				pGlobals->m_pcmainwin->m_connection->m_rgbobfgcolor = my_arg[j].conninfo.m_rgbfgconn;
				pcquerywnd->Create(wyTrue,&my_arg[j].conninfo);
				
				pGlobals->m_conncount++;
				
				pGlobals->m_pcmainwin->SetConnectionNumber();
				pcquerywnd->m_pcqueryobject->OnSelChanged(TreeView_GetSelection(pcquerywnd->m_pcqueryobject->m_hwnd));
				pGlobals->m_pcmainwin->OnActiveConn();
				//create query tabs
				if(my_arg[j].isfocussed)
				{	
					focussedconn = j;
					pGlobals->m_pcquerywnd = pcquerywnd;
				}

				my_arg[j].tempmdilist->mdi=pcquerywnd;

				totaltabs = pcquerywnd->m_listtabdetails->GetCount();
				if(totaltabs > 0)
				{
					temptabdetail = (tabdetailelem*)pcquerywnd->m_listtabdetails->GetFirst();
					temptabeditorele = (tabeditorelem*)pcquerywnd->m_listtabeditor->GetFirst();
					focuspos = 0;
					for(i = 0; i < totaltabs ; i++)
					{
						if(temptabeditorele->m_isfile)
						{
							//load file
							CustomTab_SetCurSel(pcquerywnd->m_pctabmodule->m_hwnd,  i);
							if(temptabeditorele->m_iimage != IDI_QUERYBUILDER_16 && temptabeditorele->m_iimage != IDI_SCHEMADESIGNER_16)
							{
							temptabeditorele->m_pctabeditor->m_peditorbase->m_save = temptabeditorele->m_isedited ? wyFalse : wyTrue;
							isfileopen = pcquerywnd->OpenSQLFile2(&temptabeditorele->m_tooltiptext,temptabeditorele->m_pctabeditor->m_peditorbase, wyFalse);
							
							}
							else
							if(temptabeditorele->m_iimage == IDI_QUERYBUILDER_16)
							{
#ifndef COMMUNITY
							if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
							{
								doc = new TiXmlDocument();
								isfileopen = tabquerybuilder->HandleQueryBuilderfileOnrestore(temptabeditorele->m_tooltiptext.GetAsWideChar(), &jobbuff);
								if(isfileopen && !temptabeditorele->m_isedited)
								{
									doc->Parse(jobbuff.GetString());
									CustomTab_SetCurSel(pcquerywnd->m_pctabmodule->m_hwnd,  i);
									tabquerybuilder = (TabQueryBuilder*)temptabeditorele->m_tabptr;
									//m_isautomated is set to wyTrue to automate the existing QB generation routines
									tabquerybuilder->m_isautomated = wyTrue;
									tabquerybuilder->LoadQueryXML(doc, &temptabeditorele->m_tooltiptext);
									tabquerybuilder->m_isautomated = wyFalse;
									//CustomTab_SetCurSel(pcquerywnd->m_pctabmodule->m_hwnd,  i, 1);
								}
							}
#endif
							}
							else
							{
#ifndef COMMUNITY
							if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
							{
								doc = new TiXmlDocument();
								isfileopen = tabschemadesigner->HandleSchemaDesignfileOnrestore(temptabeditorele->m_tooltiptext.GetAsWideChar(), &jobbuff);
								
								if(isfileopen && !temptabeditorele->m_isedited)
								{
									doc->Parse(jobbuff.GetString());
									CustomTab_SetCurSel(pcquerywnd->m_pctabmodule->m_hwnd,  i);
									tabschemadesigner = (TabSchemaDesigner*)temptabeditorele->m_tabptr;
									//m_isautomated is set to wyTrue to automate the existing QB generation routines
									tabschemadesigner->m_isedited = wyFalse;
									tabschemadesigner->m_isautomated = wyTrue;
									tabschemadesigner->CanvasLoadXML(doc, &temptabeditorele->m_tooltiptext);
									tabschemadesigner->m_isautomated = wyFalse;
								}
							}
#endif
							}
							if(isfileopen)
								pcquerywnd->m_pctabmodule->SetTabName(temptabeditorele->m_tooltiptext.GetAsWideChar(), wyFalse, temptabeditorele->m_isedited);
							else
							{
								temptabdetail = (tabdetailelem*)temptabdetail->m_next;
								temptabeditorele = (tabeditorelem*)temptabeditorele->m_next;
								//pcquerywnd->m_listtabeditor->Remove(i);
								pcquerywnd->m_listtabdetails->Remove(firstindex);
								CustomTab_DeleteItem(pcquerywnd->m_pctabmodule->m_hwnd,i);
								i = i - 1;
								totaltabs = totaltabs - 1;
								continue;
							}
						}
						if((temptabeditorele->m_iimage != IDI_QUERYBUILDER_16 && temptabeditorele->m_iimage != IDI_SCHEMADESIGNER_16) && (!temptabeditorele->m_isfile || temptabeditorele->m_isedited))
						{
							
							SendMessage(temptabeditorele->m_pctabeditor->m_peditorbase->m_hwnd, SCI_SETTEXT, temptabdetail->m_content.GetLength(),(LPARAM)temptabdetail->m_content.GetString());
							EditorFont::SetLineNumberWidth(temptabeditorele->m_pctabeditor->m_peditorbase->m_hwnd );
						}
						else
						if(temptabeditorele->m_iimage == IDI_QUERYBUILDER_16 && (!temptabeditorele->m_isfile || temptabeditorele->m_isedited))
							{
#ifndef COMMUNITY
							if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
							{
								doc = new TiXmlDocument();
								doc->Parse(temptabdetail->m_content.GetString());
								CustomTab_SetCurSel(pcquerywnd->m_pctabmodule->m_hwnd,  i);
								tabquerybuilder = (TabQueryBuilder*)temptabeditorele->m_tabptr;
								//m_isautomated is set to wyTrue to automate the existing QB generation routines
								tabquerybuilder->m_isautomated = wyTrue;
								tabquerybuilder->LoadQueryXML(doc, NULL, wyTrue);
								tabquerybuilder->m_isautomated = wyFalse;
								tabquerybuilder->m_isedited = temptabeditorele->m_isedited;
								if(temptabeditorele->m_isfile)
								{
									tabquerybuilder->m_filename.SetAs(temptabeditorele->m_tooltiptext);
									pcquerywnd->m_pctabmodule->SetTabName(temptabeditorele->m_tooltiptext.GetAsWideChar(),wyFalse,temptabeditorele->m_isedited);
								}
								//CustomTab_SetCurSel(pcquerywnd->m_pctabmodule->m_hwnd,  i, 1);
								pcquerywnd->SetDirtyTitle();
								delete doc;							
							}
#endif
							}
						else
						if(temptabeditorele->m_iimage == IDI_SCHEMADESIGNER_16 && (!temptabeditorele->m_isfile || temptabeditorele->m_isedited))
							{
#ifndef COMMUNITY
							if(pGlobals->m_pcmainwin->m_connection->m_enttype != ENT_PRO)
							{
								doc = new TiXmlDocument();
								doc->Parse(temptabdetail->m_content.GetString());
								CustomTab_SetCurSel(pcquerywnd->m_pctabmodule->m_hwnd,  i);
								tabschemadesigner = (TabSchemaDesigner*)temptabeditorele->m_tabptr;
								//m_isautomated is set to wyTrue to automate the existing QB generation routines
								tabschemadesigner->m_isautomated = wyTrue;
								tabschemadesigner->m_isedited = wyTrue;
								tabschemadesigner->CanvasLoadXML(doc, NULL, NULL, wyTrue);
								tabschemadesigner->m_isautomated = wyFalse;
								tabschemadesigner->m_isedited = temptabeditorele->m_isedited;
								if(temptabeditorele->m_isfile)
								{
									tabschemadesigner->m_filename.SetAs(temptabeditorele->m_tooltiptext);
									pcquerywnd->m_pctabmodule->SetTabName(temptabeditorele->m_tooltiptext.GetAsWideChar(), wyFalse, temptabeditorele->m_isedited);
								}

								pcquerywnd->SetDirtyTitle();
							}
#endif
							}
						if(temptabeditorele->m_iimage != IDI_QUERYBUILDER_16 && temptabeditorele->m_iimage != IDI_SCHEMADESIGNER_16)
							temptabeditorele->m_pctabeditor->m_peditorbase->m_edit = temptabeditorele->m_isedited;
						//set focus
						if(temptabeditorele->m_isfocussed)
						{
							focuspos = i;
							//SetFocus(temptabeditorele->m_pctabeditor->m_peditorbase->m_hwnd);
						}
						temptabdetail = (tabdetailelem*)temptabdetail->m_next;
						temptabeditorele = (tabeditorelem*)temptabeditorele->m_next;
						pcquerywnd->m_listtabdetails->Remove(firstindex);
							
						
					}

					if(totaltabs == 0)
					{
						pcquerywnd->m_pctabmodule->CreateQueryEditorTab(pcquerywnd);
						focuspos = 0;
					}
					//load history.
					tabhistory = pcquerywnd->GetActiveHistoryTab();
					tabhistory->SetText(&my_arg[j].historydata);
					//SendMessage(tabhistory->GetHwnd(), SCI_SETTEXT, (WPARAM)my_arg[j].historydata.GetLength(), (LPARAM)my_arg[j].historydata.GetString());
					//SetFocus(hwndquery);
					if(my_arg[j].obdb.GetLength())
					{
						pcquerywnd->m_pcqueryobject->m_seldatabase.SetAs(my_arg[j].obdb);
						pcquerywnd->m_pcqueryobject->SyncObjectDB(pcquerywnd);
					}
					else
					{
						pcquerywnd->m_pcqueryobject->m_seldatabase.SetAs("");
						pGlobals->m_pcmainwin->AddTextInCombo(NODBSELECTED);
					}
					if(my_arg[j].isfocussed)
					{
						//seldatabase.SetAs(pcquerywnd->m_pcqueryobject->m_seldatabase);
						pGlobals->m_database.SetAs(pcquerywnd->m_pcqueryobject->m_seldatabase);
					}
					//pcquerywnd->m_database.SetAs(L"mysqlzzz");
					CustomTab_SetCurSel(pcquerywnd->m_pctabmodule->m_hwnd, focuspos, 1);
				}

				my_arg[j].tempmdilist->m_listtabeditor = new List;
				*(my_arg[j].tempmdilist->m_listtabeditor) = *(pcquerywnd->m_listtabeditor);
				pGlobals->m_mdiwlist->Insert(my_arg[j].tempmdilist);
				delete my_arg[j].tabdetails;
			}
		}
		//WaitForMultipleObjects(totalcon, thread_handle_l, TRUE, INFINITE);

		//getsessionfilename
		sqlitequery.Sprintf("SELECT * from sessiondetails where Id = 1");
		sqliteobj->Prepare(&res, sqlitequery.GetString());
		if(sqliteobj->Step(&res, wyFalse) && sqliteobj->GetLastCode() == SQLITE_ROW)
		{
			colval = sqliteobj->GetText(&res , "filename");
			if(colval)
				pGlobals->m_pcmainwin->m_sessionfile.SetAs(colval);

			colval = sqliteobj->GetText(&res , "isedited");
			if(colval)
				pGlobals->m_pcmainwin->m_issessionedited = sqliteobj->GetInt(&res , "isedited")?wyTrue:wyFalse;
		}
		sqliteobj->Finalize(&res);
		sqliteobj->Close();
		//create a backup incase of failed connection
		//close both the files first
		if(failedconnections->GetLength() && sessionfile == NULL)
		{
			pGlobals->m_sessionbackup.SetAs(directorynamebackup);
			CopyFile(directoryname.GetAsWideChar(), directorynamebackup.GetAsWideChar(), FALSE);
		}
		//if opening a session replace automatic sessionrestore db with the session file
		if(sessionfile != NULL)
		{
			CopyFile(sessionfile->GetAsWideChar(), directory, FALSE);
		}
		delete[] thread_handle_l;
		delete[] my_arg;
		//focussedconn = 0;
		if(focussedconn >= 0)
		{
			CustomTab_SetCurSel(pGlobals->m_pcmainwin->m_hwndconntab, focussedconn, 1);
			//if(seldatabase.GetLength())
			//	pGlobals->m_pcmainwin->AddTextInCombo(seldatabase.GetAsWideChar());
			//else
			//	pGlobals->m_pcmainwin->AddTextInCombo(NODBSELECTED);
		}
		pGlobals->m_conrestore = wyFalse;
		if(sqliteobj)
		{
		delete(sqliteobj);
		sqliteobj=NULL;
		
		}
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		PostMessage(pGlobals->m_pcmainwin->m_hwndrestorestatus, UM_CLOSE_RESTORE_STATUS, 0, 0);
		return wyTrue;
		
		
}

unsigned __stdcall  
ConnectFromList_mt(void* arg_list)
{
	ConnectionInfo	conninfo;
	wyString		pathstr,  sectionstring;
	
	//connstr.SetAs(((MY_ARG*)arg_list)->connstr);
	pathstr.SetAs(((MY_ARG*)arg_list)->pathstr);

	InitializeConnectionInfo(((MY_ARG*)arg_list)->conninfo);

	//((MY_ARG*)arg_list)->isfocussed = GetSessionDetails(connstr.GetAsWideChar(), pathstr.GetAsWideChar(), &((MY_ARG*)arg_list)->conninfo, ((MY_ARG*)arg_list)->inimgr);
	((MY_ARG*)arg_list)->isfocussed = GetSessionDetailsFromTable(pathstr.GetAsWideChar(), &((MY_ARG*)arg_list)->conninfo,((MY_ARG*)arg_list)->id,((MY_ARG*)arg_list)->tempmdilist);
	pGlobals->m_pcmainwin->m_connection->OnConnect(&((MY_ARG*)arg_list)->conninfo);
	if(((MY_ARG*)arg_list)->conninfo.m_mysql)
	{
		//get tabdetails
		GetTabDetailsFromTable(pathstr.GetAsWideChar(),((MY_ARG*)arg_list)->id,((MY_ARG*)arg_list)->tabdetails);
		GetHistoryDetailsFromTable(pathstr.GetAsWideChar(),((MY_ARG*)arg_list)->id,&((MY_ARG*)arg_list)->historydata);
		GetOBDetailsFromTable(pathstr.GetAsWideChar(),((MY_ARG*)arg_list)->id,&((MY_ARG*)arg_list)->obdb);
	}
	//fire mysql_thread_end in the end of thread
	mysql_thread_end();
    return 0;
}



unsigned __stdcall  
Htmlannouncementsproc(void *arg)
{
	CHttp		http;
	char		*received=NULL;
	int status;
	const wyChar    *langcode;
	wyString productid, appmajorversion, appminorversion, extrainfo, url, htmlbuffer, str;
	wyWChar		headers[1024*3] = {0};
	wyUInt32 daysleft = 0;

	//Gets the product id(Ent, Commmunity or Trial)
#ifdef COMMUNITY
	productid.SetAs("Community");
#elif ENTERPRISE
	//SQLyog versions
	if(!pGlobals)
		return 1;

	if(!pGlobals->m_entlicense.CompareI("Professional"))
		productid.SetAs("Pro");
		
	else if(!pGlobals->m_entlicense.CompareI("Enterprise"))
		productid.SetAs("Ent");

	else if(!pGlobals->m_entlicense.CompareI("Ultimate"))			
		productid.SetAs("Ulti");	
	
		
#else
	productid.SetAs("Trial");
	GetTimeDifference(&daysleft);
#endif

// showing the announcements only if installation passed over 7 days
#if IS_FROM_WINDOWS_STORE == 1
	if(daysleft >= 7)
		goto cleanup;
#endif

	productid.EscapeURL();
	//Major and minor version num
	appmajorversion.SetAs(MAJOR_VERSION);
	appminorversion.SetAs(MINOR_VERSION UPDATE_VERSION);
	extrainfo.SetAs(EXTRAINFO); // beta string	
			
#ifdef _DEBUG
	url.SetAs("http://dev.webyog.com/notifications/sqlyog");
#else
	url.SetAs("http://www.webyog.com/notifications/sqlyog");
#endif
#ifdef _WIN64

		url.AddSprintf("?user_type=%s&major_version=%s&minor_version=%s&extra=%s&os=win64&language=%s&uuid=%s", productid.GetString(), 
		appmajorversion.GetString(), appminorversion.GetString(), extrainfo.GetString(), 
        (langcode = GetL10nLangcode()) ? langcode : "en", 
        pGlobals->m_appuuid.GetString());
#else
		url.AddSprintf("?user_type=%s&major_version=%s&minor_version=%s&extra=%s&os=win32&language=%s&uuid=%s", productid.GetString(), 
		appmajorversion.GetString(), appminorversion.GetString(), extrainfo.GetString(), 
        (langcode = GetL10nLangcode()) ? langcode : "en", 
        pGlobals->m_appuuid.GetString());
#endif

    if(pGlobals->m_regname.GetLength()>0)
		url.AddSprintf("&regname=%s",pGlobals->m_regname.GetString());
	if(daysleft > 0 && daysleft <= 7)
		url.AddSprintf("&trialends=1");
    //no of days left 

#if IS_FROM_WINDOWS_STORE == 1
	url.AddSprintf("&FromWindowsStore=1");
#endif

	http.SetUrl(url.GetAsWideChar());
	http.SetContentType(L"text/xml");
	
	if(!http.SendData("abc", 3, false, &status, false ))
	{
		goto cleanup;
	}
	if(!http.GetAllHeaders(headers, sizeof(headers)))
	{
		goto cleanup;
	}
	if(status != HTTP_STATUS_OK )
	{
		goto cleanup;
	}
	
	received = http.GetResponse();
	
	if(!received)
	{
		goto cleanup;
	}

	str.SetAs(received);
	///The HTTP message received from the server will contain "closeann" string if its a valid HTTP response from webyog server.
	if(str.FindI("closeann", 0) != -1)
	{
		htmlbuffer.SetAs(received);

		pGlobals->m_announcementshtml.SetAs(received);
	}

cleanup:
	
    return 0;
}


unsigned __stdcall  
sessionsavesproc(void *arg)
{

	tabeditorelem*		temptabeditorele,*deltabeditorele;
	MDIlist*			tempmdilist, *deltempmdilist;
	MDIWindow			*wnd;
	wyString			testquery;
	wyInt32				totalconntabs, i, j, k, tabcount;
	wyInt32				totalquerytabs;
    wyWChar				directory[MAX_PATH+64] = {0};
   
    HANDLE				hfind;
    WIN32_FIND_DATAW	fdata;
    wyString			directoryname;
	CTCITEM contabitem = {0};
	wyString			sqlitequery,sqliteerr;
	//HANDLE					namedmutex;
	//SECURITY_ATTRIBUTES		lpMutexAttributes;
	wyBool				ismutexowned = wyFalse, ispurge = wyFalse, isvacuum = wyTrue, ret;
	wyFile					plinklock;
	sqlite3_stmt		*res;
	wySQLite            *pwySQLite;
	/*lpMutexAttributes.bInheritHandle = TRUE;
	lpMutexAttributes.lpSecurityDescriptor = NULL;
	lpMutexAttributes.nLength = sizeof(lpMutexAttributes);*/
	if(pGlobals->m_configdirpath.GetLength())
	{
		wcscat(directory, pGlobals->m_configdirpath.GetAsWideChar());
		wcscat(directory, L"\\connrestore.db");
	}
	else
	{
		if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, directory)))
		{
			wcscat(directory, L"\\SQLyog\\");	
			wcscat(directory, L"connrestore.db");
		}
		else 
			return 0;
	}
	
	hfind = FindFirstFile(directory, &fdata);			
	directoryname.SetAs(directory);
	pwySQLite=new wySQLite();
	pGlobals->m_sqliteobj = pwySQLite;
	pGlobals->m_sqliteobj->Open(directoryname.GetString());

	sqlitequery.Sprintf("PRAGMA foreign_keys = ON");
	pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);

	if(hfind == INVALID_HANDLE_VALUE)	/* File is existing */
	{
		//create tables
		sqlitequery.Sprintf(TABLE_SCHEMADETAILS);
		pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
		//insert schema_version values
		sqlitequery.Sprintf(TABLE_CONNDETAILS);
		pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);

		sqlitequery.Sprintf(TABLE_HISTORYDETAILS);
		pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);

		sqlitequery.Sprintf(TABLE_OBDETAILS);
		pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
		//Id, Tabid, position, title, tooltip, isfile,isfocussed,content && tabtype? also issaved?
		sqlitequery.Sprintf(TABLE_TABDETAILS);
		pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);

		sqlitequery.Sprintf(TABLE_SESSIONDETAILS);
		pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);

		sqlitequery.Sprintf("INSERT INTO schema_version(description, major_version, minor_version) VALUES (\"%s\",\"%s\",\"%s\")",SCHEMA_DESCRIPTION, SCHEMA_MAJOR_VERSION, SCHEMA_MINOR_VERSION);
		pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
			//return wyFalse;
	}
	else
	{
		const wyChar *desc = NULL, *majorver , *minorver ;

		sqlitequery.Sprintf("SELECT * from schema_version");
		pGlobals->m_sqliteobj->Prepare(&res, sqlitequery.GetString()); 
		if(pGlobals->m_sqliteobj->Step(&res, wyFalse) && pGlobals->m_sqliteobj->GetLastCode() == SQLITE_ROW)
		{
			desc = pGlobals->m_sqliteobj->GetText(&res, 0);
			majorver = pGlobals->m_sqliteobj->GetText(&res, 1);
			minorver = pGlobals->m_sqliteobj->GetText(&res, 2);
		}

		if(desc && desc != NULL && (strcmp(SCHEMA_MAJOR_VERSION, majorver) != 0 || strcmp(SCHEMA_MINOR_VERSION, minorver) != 0 ))
		{
			//common option for version changre
			sqlitequery.SetAs("");
			sqlitequery.Sprintf("UPDATE schema_version set description = \"%s\", major_version=\"%s\", minor_version = \"%s\"  where description = \"%s\"", SCHEMA_DESCRIPTION, SCHEMA_MAJOR_VERSION, SCHEMA_MINOR_VERSION, desc);
			pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
			int maj = atoi(majorver);
			int min = atoi(minorver);
			if(maj <= 12  && min < 34)//adding cloumn for readonly in version 12.34
			{
				sqlitequery.SetAs("");
				sqlitequery.Sprintf("ALTER TABLE conndetails ADD COLUMN 'readonly' INTEGER default 0");
				pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
			}
			if (maj < 13)
			{
				sqlitequery.SetAs("");
				sqlitequery.Sprintf("ALTER TABLE conndetails ADD COLUMN 'rebuild_tags' INTEGER default 1");
				pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
			}
		}
		sqlitequery.Sprintf("SELECT type FROM sqlite_master WHERE name='sessiondetails'");
		pGlobals->m_sqliteobj->Prepare(&res, sqlitequery.GetString());
		if(!pGlobals->m_sqliteobj->Step(&res, wyFalse) || pGlobals->m_sqliteobj->GetLastCode() != SQLITE_ROW)
		{
			pGlobals->m_sqliteobj->Finalize(&res);
			sqlitequery.Sprintf(TABLE_SESSIONDETAILS);
			pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
		}
		else
			pGlobals->m_sqliteobj->Finalize(&res);	

		sqlitequery.Sprintf("SELECT type FROM sqlite_master WHERE name='conndetails'");
		pGlobals->m_sqliteobj->Prepare(&res, sqlitequery.GetString());
		if(!pGlobals->m_sqliteobj->Step(&res, wyFalse) || pGlobals->m_sqliteobj->GetLastCode() != SQLITE_ROW)
		{
			pGlobals->m_sqliteobj->Finalize(&res);
			sqlitequery.Sprintf(TABLE_CONNDETAILS);
			pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
		}
		else
			pGlobals->m_sqliteobj->Finalize(&res);	

		sqlitequery.Sprintf("SELECT type FROM sqlite_master WHERE name='tabdetails'");
		pGlobals->m_sqliteobj->Prepare(&res, sqlitequery.GetString());
		if(!pGlobals->m_sqliteobj->Step(&res, wyFalse) || pGlobals->m_sqliteobj->GetLastCode() != SQLITE_ROW)
		{
			pGlobals->m_sqliteobj->Finalize(&res);
			sqlitequery.Sprintf(TABLE_TABDETAILS);
			pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
		}
		else
			pGlobals->m_sqliteobj->Finalize(&res);

		sqlitequery.Sprintf("SELECT type FROM sqlite_master WHERE name='historydetails'");
		pGlobals->m_sqliteobj->Prepare(&res, sqlitequery.GetString());
		if(!pGlobals->m_sqliteobj->Step(&res, wyFalse) || pGlobals->m_sqliteobj->GetLastCode() != SQLITE_ROW)
		{
			pGlobals->m_sqliteobj->Finalize(&res);
			sqlitequery.Sprintf(TABLE_HISTORYDETAILS);
			pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
			
			sqlitequery.SetAs("SELECT Id from conndetails");
			pGlobals->m_sqliteobj->Prepare(&res, sqlitequery.GetString());
			while(pGlobals->m_sqliteobj->Step(&res, wyFalse) && pGlobals->m_sqliteobj->GetLastCode() == SQLITE_ROW)
			{
				i = pGlobals->m_sqliteobj->GetInt(&res, 0);
				sqlitequery.Sprintf("INSERT INTO historydetails (Id, historydata) VALUES (%d ,\"\")",i);
				pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
			}
			pGlobals->m_sqliteobj->Finalize(&res);
		}
		else
			pGlobals->m_sqliteobj->Finalize(&res);
		
		sqlitequery.Sprintf("SELECT type FROM sqlite_master WHERE name='obdetails'");
		pGlobals->m_sqliteobj->Prepare(&res, sqlitequery.GetString());
		if(!pGlobals->m_sqliteobj->Step(&res, wyFalse) || pGlobals->m_sqliteobj->GetLastCode() != SQLITE_ROW)
		{
			pGlobals->m_sqliteobj->Finalize(&res);
			sqlitequery.Sprintf(TABLE_OBDETAILS);
			pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
			
			sqlitequery.SetAs("SELECT Id from conndetails");
			pGlobals->m_sqliteobj->Prepare(&res, sqlitequery.GetString());
			while(pGlobals->m_sqliteobj->Step(&res, wyFalse) && pGlobals->m_sqliteobj->GetLastCode() == SQLITE_ROW)
			{
				i = pGlobals->m_sqliteobj->GetInt(&res, 0);
				sqlitequery.Sprintf("INSERT INTO obdetails (Id, obdb) VALUES (%d ,\"\")",i);
				pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
			}
			pGlobals->m_sqliteobj->Finalize(&res);
		}
		else
			pGlobals->m_sqliteobj->Finalize(&res);

		sqlitequery.Sprintf("SELECT type FROM sqlite_master WHERE name='schema_version'");
		pGlobals->m_sqliteobj->Prepare(&res, sqlitequery.GetString());
		if(!pGlobals->m_sqliteobj->Step(&res, wyFalse) || pGlobals->m_sqliteobj->GetLastCode() != SQLITE_ROW)
		{
			pGlobals->m_sqliteobj->Finalize(&res);
			sqlitequery.Sprintf(TABLE_SCHEMADETAILS);
			pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
			sqlitequery.Sprintf("INSERT INTO schema_version(description, major_version, minor_version) VALUES (\"%s\",\"%s\",\"%s\")",SCHEMA_DESCRIPTION, SCHEMA_MAJOR_VERSION, SCHEMA_MINOR_VERSION);
			pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
		}
		else
			pGlobals->m_sqliteobj->Finalize(&res);
	}
	
	FindClose(hfind);

	if(!IsConnectionRestore())
	{
		pGlobals->m_sessionrestore = wyFalse;
		ispurge = wyTrue;
	}
	else
	{
		pGlobals->m_sessionrestore = wyTrue;
	}

	//mdiwindow and tabeditor pointers, can be deleted during save
	//open db or create db if it does not exist

	while(1)
	{
		//check for close flag
		ResetEvent(pGlobals->m_pcmainwin->m_sessionsaveevent);

		if(pGlobals->m_pcmainwin->m_sqlyogclosed)
		{
			if(pGlobals->m_issessionsaveactive)
			{
				//ReleaseMutex(pGlobals->m_sessionsavemutex);
				//CloseHandle(pGlobals->m_sessionsavemutex);
				plinklock.Close();
			}
			return 0;
		}
		//check if timer event is signalled
		WaitForSingleObject(pGlobals->m_pcmainwin->m_savetimerevent, CONSAVE_INTERVAL);

		//set timer event to non signalled
		ResetEvent(pGlobals->m_pcmainwin->m_savetimerevent);

		if(!pGlobals->m_sessionrestore)
		{
			pGlobals->m_issessionsaveactive = wyFalse;
			
		}
		else
		{
			if(!ismutexowned)
			{
				//pGlobals->m_sessionsavemutex = CreateMutex(&lpMutexAttributes, TRUE, TEXT("Global\\SQLyogSessionSave"));
				//pGlobals->m_sessionsavemutex = CreateMutex(&lpMutexAttributes, TRUE, TEXT("SQLyogSessionSave"));
				if(LockPlinkLockFile(&plinklock)==wyTrue)
					ismutexowned = wyTrue;
			}
			//if(ismutexowned || (pGlobals->m_sessionsavemutex != NULL && GetLastError() != ERROR_ALREADY_EXISTS)) 
			if(ismutexowned)
			{
				pGlobals->m_issessionsaveactive = wyTrue;
				ismutexowned = wyTrue;
				if(ispurge)
				{
					//purges the tables and lists
					sqlitequery.Sprintf("DELETE FROM conndetails");
					pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
					tempmdilist = (MDIlist*)pGlobals->m_mdiwlist->GetFirst();
					totalconntabs = pGlobals->m_mdiwlist->GetCount();
					for(i = 0; i < totalconntabs; i++)
					{
						deltempmdilist = tempmdilist;
						tempmdilist = (MDIlist*)tempmdilist->m_next;
						pGlobals->m_mdiwlist->Remove(deltempmdilist);
						delete deltempmdilist;
					}
					//if(pGlobals->m_mdiwlist)
					//	{
					//	delete pGlobals->m_mdiwlist;
					//	pGlobals->m_mdiwlist = NULL;
					//	}
					//
					if(pGlobals->m_pcmainwin->m_hwndconntab)
					{
						
						tabcount = CustomTab_GetItemCount(pGlobals->m_pcmainwin->m_hwndconntab);
						for(j = 0; j < tabcount; j++)
						{
							contabitem.m_mask = contabitem.m_mask |CTBIF_LPARAM|CTBIF_IMAGE; 
							CustomTab_GetItem(pGlobals->m_pcmainwin->m_hwndconntab, j, &contabitem);
							wnd = (MDIWindow*)contabitem.m_lparam;
							totalquerytabs = wnd->m_listtabeditor->GetCount();
							temptabeditorele = (tabeditorelem*)wnd->m_listtabeditor->GetFirst();
							for(k = 0; k < totalquerytabs; k++)
							{
								deltabeditorele = temptabeditorele;
								temptabeditorele = (tabeditorelem*)temptabeditorele->m_next;
								wnd->m_listtabeditor->Remove(deltabeditorele);	
								delete deltabeditorele;
							}
						}

					}
					ispurge = wyFalse;
				}
				if(isvacuum)
				{
					sqlitequery.Sprintf("VACUUM");
					pGlobals->m_sqliteobj->Execute(&sqlitequery, &sqliteerr);
					isvacuum = wyFalse;
				}
				if(WaitForSingleObject(pGlobals->m_pcmainwin->m_sqlyogcloseevent, 0) == WAIT_OBJECT_0 )
				{
					if(pGlobals->m_pcmainwin)
					{
						ret = pGlobals->m_pcmainwin->SaveConnectionDetails();
						if(!ret && !pGlobals->m_pcmainwin->m_hwndconntab)
							ispurge = wyTrue;
					}
				}
			

			}
			else
			{
				ispurge = wyTrue;
			}
		}
		SetEvent(pGlobals->m_pcmainwin->m_sessionsaveevent);
		WaitForSingleObject(pGlobals->m_pcmainwin->m_sqlyogcloseevent, INFINITE);

	}

}

