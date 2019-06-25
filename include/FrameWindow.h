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


#ifndef _CMainWin_H
#define _CMainWin_H

#include "FrameWindowHelper.h"
#include "MDIWindow.h"
#include "Global.h"
#include "EditorQuery.h"
#include "FavoriteMenu.h"
#include "FavoriteBase.h"
#include "TabMgmt.h"
#include "FavoriteAdd.h"
#include "FavoriteOrganize.h"
#include "ConnectionBase.h"
#include "ConnectionCommunity.h"
#include "ConnectionTab.h"
#include "ExportImportConnectionDetails.h"


using namespace std;

#ifdef COMMUNITY
class CommunityRibbon;
#endif

#define			CONRESTORE_TIMER	1001
#define			CONSAVE_TIMER		1002

class	MDIWindow;
class	EditorBase;
class	Favorites;
class	UpgradeCheck;
class	ConnectionTab;
class	tabeditorelem;
class	TabHistory;
class	TabTypes;
class	TabEditor;
class	List;

/*! \struct SSHPORTINFO
	\brief ssh port information
    \param wyInt32     m_port;          // Port number
	\param wyInt32     m_ret;           // Return value
*/
struct SSHPORTINFO
{
	wyInt32     m_port;
	wyInt32     m_ret;
};
void 
tablog(const char * buff);

class MDIlist : public wyElem
{
public:
	MDIWindow	*mdi;
	wyInt32		m_id;
	wyInt32		m_position;
	wyBool		m_ispresent;
	wyBool		m_isfocussed;
	COLORREF	m_rgbconn;	
	COLORREF	m_rgbfgconn;
	wyString	m_obdetails;
	wyString	m_historydata;
	List		*m_listtabeditor;
};

class MDIlisttemp : public wyElem
{
public:
	MDIWindow	*mdi;
	wyInt32		m_id;
	wyInt32		m_position;
	wyBool		m_ispresent;
	wyBool		m_isfocussed;
	COLORREF	m_rgbconn;	
	COLORREF	m_rgbfgconn;
	wyString	m_obdetails;
	wyString	m_historydata;
    TabHistory	*m_tabhistory;
	TabTypes	*m_activetab;
	TabEditor	*m_tabqueryactive;
	List		*m_listtabeditor_temp;		
};

enum TabType
{
	querytab,
	querybuilder,
	schemadesigner,
	datasearch,
	none
};
class ListOfOpenQueryTabs : public wyElem
{
public:
	wyString tabname;
	wyInt32 seqofquerytab;
	wyInt32 seqofquerybuilder;
	wyInt32 seqofschemadesigner;
	wyInt32 seqofdatasearch;
	TabType tabtype;
	ListOfOpenQueryTabs()
	{
		tabname.SetAs("");
		seqofquerytab = 1;
		seqofquerybuilder = 1;
		seqofschemadesigner = 1;
		seqofdatasearch = 1;
		tabtype = none;
	}
};
class MDIListForDropDrown : public wyElem
{
public:
	wyString name;
	MDIWindow	*mdi;
	List *opentab;
	//HWND m_hwndconntabinlist;
	//TabModule * m_hwndTabModuleinlist;
	MDIListForDropDrown()
	{
		name.SetAs("");
		opentab = new List(); 
	}
};


class tabdetailelem : public wyElem
{
public:
	//~tabdetailelem();
	wyInt32		m_id;
	wyInt32		m_tabid;
	wyInt32		m_position;
	wyInt32		m_iimage;
	wyString	m_psztext;
	COLORREF	m_color;//Color of tab
	COLORREF	m_fgcolor;//Text color of tab
	wyString	m_tooltiptext;//tooltip text (also filename if it is a file)
	wyBool		m_isfile;
	wyBool		m_isedited;
	wyBool		m_isfocussed;
	wyString	m_content;
	wyInt32		m_leftortoppercent;
};

class QueryRestore : public wyElem
{
public:
	wyInt32		m_id;
	wyInt32		m_tabid;
	wyInt32		m_position;
	wyBool		m_ispresent;
	wyString	m_psztext;
	COLORREF	m_color;//Color of tab
	COLORREF	m_fgcolor;//Text color of tab
	wyString	m_tooltiptext;//tooltip text (also filename if it is a file)
	wyBool		m_isfile;
	wyBool		m_isfocussed;
	wyString	m_content;
};

class ListofOpenTabs : public wyElem
{
	public:
		wyString name;
		//HWND m_hwndconntabinlist;
		TabModule * m_hwndTabModuleinlist;
		ListofOpenTabs()
		{
			name.SetAs("");
			
		}
};

class FrameWindow
{

public:
	///constructor with the current instance as the argument.
    /**
    @param hinstance:  current instance
    */
	FrameWindow(HINSTANCE hinstance);

    /// Destructor
    /**
    Free up all the allocated resources
    */
	virtual	~FrameWindow();

    static	LRESULT			CALLBACK	ToolbarWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
	static	LRESULT			CALLBACK	TrialbuyWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
	static unsigned __stdcall		RestoreStatusThreadProc(LPVOID lpparam);

	static INT_PTR CALLBACK		RestoreStatusDlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    ///Window MainWindow window procedure 
    /**
    @param hwnd                 : IN Window procedure
    @param message              : IN Window message
    @param wparam               : IN Unsigned int message parameter
    @param lparam               : IN Long message parameter
    @returns long pointer
    */
	static	LRESULT			CALLBACK	WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    
	///Subclassed window procedure for frame window procedure
    /**
    @param hwnd                 : IN Window procedure
    @param message              : IN Window message
    @param wparam               : IN Unsigned int message parameter
    @param lparam               : IN Long message parameter
    @returns long pointer
    */	
	static LRESULT	CALLBACK    FrameWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    
    /// Nag screen window procedure
    /**
    @param hwnd                 : IN Window procedure
    @param message              : IN Window message
    @param wparam               : IN Unsigned int message parameter
    @param lparam               : IN Long message parameter
    @returns wyTrue on success
    */
	static	wyInt32			CALLBACK	NagScreenProc		(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam); 		
    
    /// Callback procedure to close every Query Window.
    /**
    @param hwnd                 : IN Window procedure
    @param lparam               : IN Long message parameter
    @returns wyTrue on success else wyFalse
    */
	static	wyInt32			CALLBACK	CloseEnumProc		(HWND hwnd, LPARAM lParam);

	 /// Callback procedure to MDI child window.
    /**
    @param hwnd                 : IN Window procedure
    @param lparam               : IN Long message parameter
    @returns wyTrue on success else wyFalse
    */
	static BOOL CALLBACK			EnumMDIChildren(HWND hwnd, LPARAM lParam);

    /// Procedure used to create the different types of objects like view, trigger, sp, function...
    /**
    @param hwnd                 : IN Window procedure
    @param message              : IN Window message
    @param wparam               : IN Unsigned int message parameter
    @param lparam               : IN Long message parameter
    @returns wyTrue on success
    */
	static	INT_PTR         CALLBACK	CreateObjectDlgProc	(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    /// Function registers the main window
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue on success, otherwise wyFalse
    */
	wyBool					RegisterWindow(HINSTANCE hinstance);

    /// Functions registers the Connection window that is the QueryWindow.
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue on success, otherwise wyFalse
    */
	wyBool					RegisterQueryWindow(HINSTANCE hinstance);

    /// Function registers the TabInterface window for Table Operations
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue on success, otherwise wyFalse
    */
    wyBool                  RegisterTabInterfaceWindow(HINSTANCE hinstance);

    /// Function registers the TabInterface bottom frame window for Table Operations
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue on success, otherwise wyFalse
    */
    //wyBool                  RegisterTabIntBottomFrameWindow(HINSTANCE hinstance);

    /// Function registers the PlainWindow for Sub-tabs of Table Operations
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue on success, otherwise wyFalse
    */
    wyBool                  RegisterPlainWindow(HINSTANCE hinstance);

    /// Function registers the TabFields window for Table Operations
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue on success, otherwise wyFalse
    */
    //wyBool                  RegisterTabFieldsWindow(HINSTANCE hinstance);

    /// Function registers the TabAdvancedProperties window for Table Operations
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue on success, otherwise wyFalse
    */
    //wyBool                  RegisterTabAdvPropWindow(HINSTANCE hinstance);

    /// Function registers the TabForeignKeys window for Table Operations
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue on success, otherwise wyFalse
    */
    //wyBool                  RegisterTabFKWindow(HINSTANCE hinstance);

    /// Function registers the TabIndexes window for Table Operations
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue on success, otherwise wyFalse
    */
    //wyBool                  RegisterTabIndexesWindow(HINSTANCE m_hinstance);

    /// Functions registers the vertical splitter window used in the query window as divider
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue on success, otherwise wyFalse
    */
	wyBool					RegisterVSplitter(HINSTANCE hinstance);

    /// Functions registers the horizontal splitter window.
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue on success, otherwise wyFalse
    */
	wyBool					RegisterHSplitter(HINSTANCE hinstance);

    /// Function registers the InsertUpdate window that contains all the child windows
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue is success, otherwise wyFalse
    */
	wyBool					RegisterInsertUpdateWindow(HINSTANCE hinstance);

	 /// Function registers the SD - Canvas window with white back ground
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue is success, otherwise wyFalse
    */
	wyBool					RegisterSDCanvasWindow(HINSTANCE hinstance);

	/// Function registers the QB - Canvas window with white back ground
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue is success, otherwise wyFalse
    */
	wyBool					RegisterQBCanvasWindow(HINSTANCE hinstance);

	/// Function registers the PQA - Tab(shown result in html format)
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue is success, otherwise wyFalse
    */
	wyBool					RegisterPQAResultwindow(HINSTANCE hinstanceent);
	wyBool					RegisterAnnouncementswindow(HINSTANCE hinstanceent);
	wyBool					RegisterAnnouncementsMainwindow(HINSTANCE hinstanceent);
	/// Function registers the HTML window for object tab(shown result in html format)
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue is success, otherwise wyFalse
    */
	wyBool					RegisterHTMLInfoTabwindow(HINSTANCE hinstanceent);

	/// Function registers the HTML window for search tab
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue is success, otherwise wyFalse
    */
	wyBool					RegisterHTMLDbSearchWindow(HINSTANCE hinstanceent);

	/// Function registers the Form view html window
	/**
    @param hinstance            : IN Handle to the instance
    @return wyTrue is success, otherwise wyFalse
    */
	wyBool					RegisterHTMLFormViewWindow(HINSTANCE hinstanceent);

	/// Function registers the HTTP-Error window
    /**
    @param hinstance            : IN Handle to the instance
    @return wyTrue is success, otherwise wyFalse
    */
	wyBool					RegisterHTTPErrorWindow(HINSTANCE hinstanceent);

    wyBool                  RegisterDataWindow(HINSTANCE hInstance);

    /// sets the initial window coordinates
    /**
    @param hwnd                 : IN Window handle
    @return wyTrue on success, otherwise wyFalse
    */
	wyBool					MoveToInitPos(HWND hwnd);
	wyBool					MoveToInitPosByrc(HWND hwnd, RECT *rc);

    /// writes the initial window coordinates top file(.ini)
    /**
    @param hwnd                 : IN Window handle
    @return wyTrue is success, otherwise wyFalse
    */
	wyBool					WriteInitPos(HWND hwnd);

    /// Helps to resize the window
    /**
	@param wparam           : IN Window procedure wparam
    @return wyTrue always
    */
	wyBool					Resize(WPARAM wparam = 0);
	
    /// Creates  the main window
    /**
    @return wyTrue on success, otherwise wyFalse
    */
	wyBool					Create();


    ///Shows the current MYSQL error
    /**
    @param hdlg                 : IN Dialog window handle
    @param tunnel               : IN Tunnel pointer
    @param mysql                : IN Mysql pointer
    @returns void
    */
	wyInt32					ShowMySQLErrorCancel(HWND hdlg, Tunnel * tunnel, PMYSQL mysql);
    
    /// The function reinitializes the main menu and adds the updated latest file in the main menu.
    /** 
    @returns wyTrue if SUCCESS, otherwise wyFalse
    */
	wyBool					InitLatestFiles();

    ///Gets the SQLyog.ini file path
	/**
	@path : OUT gets the ini file path
	@return void
	*/
	void					GetSQLyogIniPath(wyString *path);

	/// This function initializes the splitter position taken from the ini file.
    /**
	@param dirstr : IN .ini path
    @returns wyTrue if SUCCESS, otherwise wyFalse
    */
	wyBool					InitSplitterPos(const wyChar *dirstr);

	///Initialize the PQA(get the ini value, for 'show profile' option)
	/**
	@param dirstr : IN .ini path
	@return void
	*/
	VOID					InitPQAOptions(const wyChar *dirstr);

	///Initialize the Autocomplete Case option
	/**
	@param dirstr : IN .ini path
	@return void
	*/
	//VOID					InitAutoCompleteCaseSelection(const wyChar *dirstr);
	
    /// The function reinitializes the main menu and adds the updated latest file in the main menu.
    /** 
	@param iscreate : IN flag tells , fun calls during creating frame window or not
    @returns wyTrue if SUCCESS, otherwise wyFalse
    */
	wyBool					ReInitLatestFiles(wyBool iscreate = wyFalse);

	/// Handle inserts Recent Sql files and Schema designs to menu
	/**
	@param hsubmenu		: IN sub menu for where the recent files to be inserted.
	@param tabimageid   	: IN id to differentiate between query tab, schema designer tab and query builder tab
	 @returns wyTrue if success, otherwise wyFalse
	*/
    wyBool					InsertRecentMenuItems(HMENU hsubmenu, wyInt32 tabimageid = IDI_QUERY_16);

    /// Function writes the file which is passed as a parameter.
    /**
    @param filename             : IN Filename to add
    @returns wyTrue if success, otherwise wyFalse
    */
	wyBool					WriteLatestFile(wyWChar *filename);

    /// This function sets the splitter position to the values sent in the RECT pointer.
    /**
    @param isvertical           : IN Specifies whether vertical or horizontal
    @param rect                 : IN Left or top rectangular area
    @param leftortoppercent     : IN Percentage
    @returns wyTrue if success, otherwise wyFalse
    */
	wyBool					SetSplitterPos(RECT *rect, wyUInt32 leftortoppercent);

    /// Function writes the splitter position to the file so that when we start the application again we can start in the same position.
    /** 
    @param section              : IN Section name
    @returns wyTrue if success, otherwise wyFalse
    */
	wyBool					WriteSplitterPos(wyWChar *section);

    /// Function writes the file which is selected from latest files in the Main Menu.
    /** 
    @param id                   : IN Id of the menu selected
    @param pcquerywnd           : IN/OUT Current connection window handle
    @returns wyTrue if success, otherwise wyFalse
    */
	wyBool					InsertFromLatestFile(wyInt32 id, MDIWindow* pcquerywnd);

	/// Function writes the SQl file which is selected from latest files in the Main Menu.
    /** 
    @param id                   : IN Id of the menu selected
    @param pcquerywnd           : IN/OUT Current connection window handle
	@param filename				: IN file name to be loaded on to editor.
    @returns wyTrue if success, otherwise wyFalse
    */
	wyBool					InsertFromLatestSQLFile(wyInt32 id, MDIWindow * pcquerywnd, wyWChar *filename);

	/// Function loads the Schema Designer file which is selected from latest files in the Main Menu.
    /** 
    @param id                   : IN Id of the menu selected
    @param pcquerywnd           : IN/OUT Current connection window handle
	@param filename				: IN file name to be loaded on to editor.
    @returns wyTrue if success, otherwise wyFalse
    */
	wyBool					InsertFromLatestSchemaDesignFile(wyInt32 id, MDIWindow * pcquerywnd, wyWChar *filename);

    /// helper function to stop a query
    /** 
    @param hwndactive           : IN The handle to the active MDI child window
    @param pcquerywnd           : IN/OUT Current connection window handle
    @returns wyTrue if success, otherwise wyFalse
    */
	wyBool					StopQuery(HWND hwndactive, MDIWindow * pcquerywnd);

    ///Gets the Currently selected db, it will select db even though the selected node is a child of db
    /** 
    @param pcquerywnd           : IN Current query window handle
    @param db                   : OUT Database name
    @returns void
    */
	void                    GetSelectedDB(MDIWindow	*pcquerywnd, wyString &db);

    ///Gets the Currently selected table, it will select table even though the selected node is a child of table
    /** 
    @param pcquerywnd           : IN Current connection window handle
    @param tablename            : OUT Table name
    @returns void
    */
	void					GetSelectedTable(MDIWindow	*pcquerywnd, wyString &table);

    /// Gets the currently selected OBJECT
    /** 
    @param pcquerywnd           : IN Current connection window handle
    @param objectname           : OUT Object name
    @returns void
    */
	void					GetObjectName(MDIWindow	*pcquerywnd, wyString &objectname);

    /// Sets the MAin window handle
    /** 
    @param hwnd                 : IN Handle to set
    @returns void
    */
	void					SetHwnd(HWND hwnd);

    /// Sets the current Handel to HINSTANCE
    /** 
    @param hinstance            : IN New hinstance
    @returns void
    */
	void					SetHinstance(HINSTANCE hinstance);

    ///Function to add text in the combobox of the toolbar. 
    /** 
    @param text                 : IN Text to add to combo
    @returns wyTrue always
    */
	wyBool					AddTextInCombo(const wyWChar *text);

    /// Displays text which is passed as parameter in the first part of the status bar in the main window.
    /** 
    @param text                 : Text to show
    @returns wyTrue if success, otherwise wyFalse
    */
	wyInt32					AddTextInStatusBar(const wyWChar *text);

    ///Function sets the connection number in the status bar of the main window
    /** 
    @returns wyTrue always
    */
	wyBool					SetConnectionNumber();

    /// Helps to change the context db in the toolbar combo box
    /** 
    @param tunnel           : IN Tunnel pointer
    @param mysql            : IN mysql pointer
    @param db               : IN/OUT Database to select
    @returns wyTrue if success, otherwise wyFalse
    */
	wyBool					ChangeDBInCombo(Tunnel * tunnel, PMYSQL mysql, wyWChar *db = NULL);

    /// initializes the create object dialog box
    /** 
    @param hwnd             : IN Create object dialog handle
    @param object           : IN Object name
    @returns void
    */
	void                    InitCreateObjectDlg(HWND hwnd, wyChar *object);

    ///Handle WM_COMMAND message
    /** 
    @param wparam           : IN Window procedure wparam
    @returns 1 if success, otherwise 0
    */
	wyInt32                 OnWmCommand(WPARAM wparam);

    /// Handle the table engine change command
    /**
    @param hmenu:   IN active menu handle
    @param id:      IN menu item id
    @return wyBool, wyTrue if success, otherwise wyFalse
    */
    wyBool                  HandleChangeTabletype(HMENU hmenu, wyInt32 id);

    /// Handles WM_MENUCOMMAND message
    /** 
    @param hwnd             : IN Window procedure hwnd
    @param wparam           : IN Window procedure wparam
    @param lparam           : IN Window procedure lparam
    @returns void
    */
    void                    OnWmMenuCommand(HWND hwnd, WPARAM wparam, LPARAM lparam);

    /// Handles WM_MENUSELECT message, changes the status bar information on mouse over change
    /** 
    @param menuid           : IN Menu id
    */
	wyInt32                 OnWmMenuSelect(HMENU hmenuhandle, wyInt32 menuid);

    ///Gets the mainwindow handle
    /** 
    @returns mainwindow handle
    */
	HWND					GetHwnd();

    /// Gets the toolbar handle
    /** 
    @returns toolbar handle
    */
	HWND					GetToolHwnd();

    /// Gets the status bar handle
    /** 
    @returns status bar handle
    */
	HWND					GetStatusHwnd();

    /// Gets toolbar combo handle
    /** 
    @returns toolbar combo
    */
	HWND					GetToolCombo();

    /// Gets the mdi handle
    /** 
    @returns MDI handle
    */
	HWND					GetMDIWindow();

    /// Gets the current instance
    /** 
    @returns current instance
    */
	HINSTANCE				GetHinstance();
  
    /// Insert from the list of latest files
    /**
    @param id               : IN id of the menu selected
    @param pcquerywnd       : IN handle to the active window
    @param filename         : IN filename of the file to be loaded
    @returns wyTrue on success and wyFalse on faillure
    */
    wyBool                  InsertFromLatestQBFile(wyInt32 id, MDIWindow * pcquerywnd, wyWChar *filename);

	///Shorcut for switching between Form & Grid
	/**
	@param ptabmgmt : IN TabMgmt pointer
	@return void
	*/
	void					AccelaratorSwitchGridForm(MDIWindow *wnd, TabMgmt *ptabmgmt);

    void                    LoadConnTabPlusMenu(LPARAM lparam);

	//Display connection and query tab drop down menu

	void                    LoadConnTabDropDownMenu(LPARAM lparam);

    static void CALLBACK    TooltipTimerProc(HWND hwnd, UINT message, UINT_PTR id, DWORD time);
    
    static void             ShowQueryExecToolTip(wyBool show = wyTrue);

    StatusBarMgmt*          m_statusbarmgmt;

    /// Number of icons
	wyUInt32				m_numicons;

    /// 
	wyInt32					m_findmsg;

    /// Main window HANDLE
	HWND					m_hwndmain;

    /// Tool window HANDLE
    HWND                    m_hwndtool;

    /// Second tool HANDLE
    HWND                    m_hwndsecondtool;

    /// Status window HANDLE
    HWND                    m_hwndstatus;
    
    /// Handler to mdi client window
    HWND                    m_hwndmdiclient;

    /// Handler to combo box
    HWND                    m_hwndtoolcombo;

	/// Handler to static text for trial days left
    HWND                    m_hwndtrialtext;

	/// Handler to static text for trial buy
    HWND                    m_hwndtrialbuy;

    /// Find dialog window HANDLE
    HWND                    m_finddlg;

    /// Tabbed Interface Handle
    HWND                    m_htabinterface;

    /// Handler to lasft focused window
    HWND                    m_lastfocus;

    ///
	HWND					m_geninfo;

	/// Query class icon
	HICON					m_hicon;

    /// Vertical spliter rectangle
	RECT					m_rcvsplitter;

    /// Horizontal spliter rectangle
    RECT                    m_rchsplitter;

    /// Left percentage
	wyUInt32				m_leftpercent;

    /// Top percentage
    wyUInt32				m_toppercent;

    /// Font HANDLE
	HFONT					m_hfont;

	/// Font Handle (used for..)
	HFONT					m_hfontlink;

    /// Connection HANDLE
    ConnectionBase          *m_connection;

    /// Window original procedure for enterprise edition
	WNDPROC					m_wpstaticorigprocent;
        
    /// Instance HANDLE
	HINSTANCE				m_hinstance;

    /// Handler to image list
	HIMAGELIST				m_himl;

    /// Image list HANDLE for combo box
    HIMAGELIST				m_hcomboiml;
    
    /// Second image list window HANDLE
    HIMAGELIST				m_hsecondiml;

    /// Imagelist handle
    HIMAGELIST				m_hmainiml;
	
    /// Tunnel authentication details
    TUNNELAUTH				m_tunnelauth;
	
    /// Favorite menu class object pointer
    CFavoriteMenu			*m_pcfavoritemenu;
	
    /// Add favorite class object pointer
    FavoriteAdd				*m_pcaddfavorite;
	
    /// Organize class object pointer
    COrganizeFavorite		*m_pcorganizefavorite;

    // array of yogIcons.
	// keeps information about icon id and its corresponding icon id.
	yogIcons				*m_icons;

	//Upgrade check class pointer
	UpgradeCheck			*m_upgrdchk;

	//Tool bar icon size
	wyInt32					m_toolbariconsize;

	//error dialog in Tbale data / Result Tab is present or not
	wyBool					m_iserrdlg;

    //Flag tells whether the Redundant Index Help is requested
    wyBool                  m_isredindexhelp;

	wyInt32					m_focussedcon;

	wyBool					m_isresizing;


#ifdef COMMUNITY	
	//Tab Header object
	CommunityRibbon				*m_commribbon;
#endif

    /// Gets the current selected item in the combobox in the toolbar.
    /**
    @returns wyInt32, currently selected item index
    */
	wyInt32					GetCurrentSel();

    ///get current database from the combobox
    /**
    @param db					: OUT currently selected database
    @param size					: IN buffersize
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool					GetCurrentSelDB(wyWChar *db, wyInt32 size);

    ///The function is called whenever the combo box is drop downed.
    /** Before showing any info it gets all the database in the mysql connection of the current active window
      * and adds them to combo box. If any error occurs while getting info about the databases then
      * it shows the error and add only No Database Selected and return
      @returns wyBool, wyTrue if success, otherwise wyFalse
      */
	wyBool					OnToolComboDropDown();

    /// This function is called whenever there is a change in selection in the combobox in the toolbar.
    /** When a new database is selected then we change the current database in the MYSQL connection.
      @returns wyBool, wyTrue if success, otherwise wyFalse
      */
	wyBool					OnToolComboSelChange();

    /// This function is called whenever there is any error selecting a database in the combobox.
    /** It adds the old database which is stored in the QueryWnd class and adds it to the combobox
      * so that the user doesn't get confused that the database has changed.
        @returns wyBool, wyTrue if success, otherwise wyFalse
      */
	wyBool					SetOldDB();

     /// This function is called whenever a QueryWindow is activated, deactivated, closed or a new one opened.
     /** It checks the current status of various window and enables and disables various menu item.
      @returns wyBool, wyTrue if success, otherwise wyFalse
     */
	wyBool					OnActiveConn();

    /// Helps to handle the resultpane menu check mark
	wyBool					CheckOrUncheckResultWindowVis(MDIWindow *pcquerywnd);

    /// Helps to handle the text result menu check mark
	wyBool					CheckOrUncheckTextResult(MDIWindow *pcquerywnd);

    /// Helps to handle the Object browser menu check mark
	wyBool					CheckOrUncheckObjBrowserVis(MDIWindow *pcquerywnd);

    /// Helps to handle the SQL editor menu check mark
	wyBool					CheckOrUncheckEditWindowVis(MDIWindow *pcquerywnd);

    /// Function Checks the result is displaying in text or not
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
    wyBool					IsTextResult();
	
    /// Helps to read from favorites
    /**
    @param hmenu				: IN favorites menu handle
    @param id					: IN selected menu id
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool					ReadFromFavorite(HMENU hmenu, wyInt32 id);

    /// Helps to read a file to the SQL editor
    /**
    @param completepath			: IN file path
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool					ReadFromFile(wyChar	*completepath);

    /// This function is called whenever the system sends notification requiring tool tip text for the toolbar.
    /**
    @param tbh					 : IN tooltip notify info
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool					OnToolTipInfo(LPNMTTDISPINFO lpnmtdi);

	///Function creates the main window i.e the main application window.
    /**
    @param hinstance: IN handle to the instance
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool					CreateMainWindow(HINSTANCE hinstance);

	//Set the Window title with license info
	/**
	@return VOID
	*/
	VOID					SetMainWindowTitle();

    /// Creates toolbar window
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool					CreateToolBarWindow();

    /// Creates status bar window
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool					CreateStatusBarWindow();

    /// Creates Main MDI window
    /**
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool					CreateMDIWindow();

    /// Creates the toolbar combo
    /**
    @param hwndtool				: IN Toolbar handle
    @returns HWND, handle to the toolbar combo
    */
	HWND					CreateToolCombo(HWND hwndtool);
	HWND					CreateTrialText(HWND hwndtool);
	HWND					CreateTrialBuy(HWND hwndtool);

    /// Initializes the favorites system
    /**
    @param hwnd					: IN Main window handle
    @returns void
    */
	void					InitFavorites(HWND hwnd);

    /// sets the toolbar window handle
    /**
    @param hwnd					: IN toolbar handle
    @returns void
    */
	void					SetToolHwnd(HWND hwnd);

    /// sets the MDI window handle
    /**
    @param hwnd					: IN MDI handle
    @returns void
    */
	void					SetMDIHwnd(HWND hwnd);

    /// sets the statusbar window handle
    /**
    @param hwnd					: IN statusbar handle
    @returns void
    */
	void					SetStatusHwnd(HWND hwnd);

    /// sets the tool combo handle
    /**
    @param hwnd					: IN tool combo handle
    @returns void
    */
	void					SetToolCombo(HWND hwnd);

    /// Helps to resize the toolbar
    /**
    @returns void
    */
	void					ResizeToolBar();

    /// Helps to resize the MDI window
    /**
    @returns void
    */
	void					ResizeMDIWindow();

	/// Function to create buttons in the toolbar.(First toolbar)
    /**
    @returns void
    */
	wyBool					CreateToolButtons(HWND hwndtool);

    /// Function to create buttons in the toolbar.(Sec toolbar)
    /**
    @returns void
    */
	wyBool					SetStatusParts(HWND hwndstatus);

	/// Function to set the font of the combo box in the toolbar to ARIAL
	/**
	@returns wyTrue
	*/
	wyBool					SetToolComboFont();

	/// Function to initialize and set the image list in the combobox of the toolbar.
	/**
	@returns wyFalse
	*/
	wyBool					SetToolComboImageList();
	

	/// Handles the creation database
	/**
	@param hwnd			: IN Window HANDLE
	@param pcquerywnd	: IN Query window pointer
	@param name			: OUT Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool					HandleCreateDatabase	(HWND hwnd, MDIWindow *pcquerywnd, wyWChar *name);


	///Fun set the charset variable
	/**
	@param charset		: IN charset to set
	@return VOID
	*/
	VOID					GetDBCharset(wyString *charset);

	/// Handles the database alter
	/**
	@param hwnd			: IN Window HANDLE
	@param pcquerywnd	: IN Query window pointer
	@param name			: OUT Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool					HandleAlterDatabase	(HWND hwnd, MDIWindow *pcquerywnd, wyWChar *name);

	/// Handles the creation view
	/**
	@param hwnd			: IN Window HANDLE
	@param pcquerywnd	: IN Query window pointer
	@param name			: OUT Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool					HandleCreateView		(HWND hwnd, MDIWindow *pcquerywnd, wyWChar *name);

	/// Handles the creation procedure
	/**
	@param hwnd			: IN Window HANDLE
	@param pcquerywnd	: IN Query window pointer
	@param name			: OUT Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool					HandleCreateProcedure	(HWND hwnd, MDIWindow *pcquerywnd, wyWChar *name);

	/// Handles the creation function
	/**
	@param hwnd			: IN Window HANDLE
	@param pcquerywnd	: IN Query window pointer
	@param name			: OUT Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool					HandleCreateFunction	(HWND hwnd, MDIWindow *pcquerywnd, wyWChar *name);

	/// Handles the creation trigger
	/**
	@param hwnd			: IN Window HANDLE
	@param pcquerywnd	: IN Query window pointer
	@param name			: OUT Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool					HandleCreateTrigger		(HWND hwnd, MDIWindow *pcquerywnd, wyWChar *name);
	/// Handles the creation event
	/**
	@param hwnd			: IN Window HANDLE
	@param pcquerywnd	: IN Query window pointer
	@param name			: OUT Database name
	@returns wyTrue on success else wyFalse
	*/
	wyBool                  HandleCreateEvent       (HWND hwnd, MDIWindow *pcquerywnd, wyWChar *name);

	/// Handles the creation view
	/**
	@param hwnd			: IN Window HANDLE
	@param pcquerywnd	: IN Query window pointer
	@param linenumber	: OUT Line number
	@returns wyTrue 
	*/
	wyBool					HandleGoTo(HWND hwnd, MDIWindow	*pcquerywnd, wyWChar *linenumber);

	/// Prepares the creation of view
	/**
	@param pcquerywnd	: IN Query window pointer
	@param viewname		: IN View name
	@param stringview	: OUT Query for view creation
	@param isqb         : IN flag for create view from querybuilder
	@returns void
	*/
	void PrepareCreateView		(MDIWindow	*pcquerywnd, const wyChar *viewname, wyString &strview, wyString *qbquery);
	/// Prepares the creation of event
	/**
	@param pcquerywnd	: IN Query window pointer
	@param eventname	: IN event name
	@param stringevent	: OUT Query for event creation
	@returns void
	*/
	void PrepareCreateEvent     (MDIWindow *pcquerywnd, const wyChar *eventname, wyString &strevent);
	/// Prepares the  query for Alter event
	/**
	@param pcquerywnd	: IN Query window pointer
	@param altereventstmt	: OUT Query for alter event
	@returns wyboll.
	*/
	wyBool PrepareAlterEvent      (MDIWindow *pcquerywnd, wyString &altereventstmt); 

	/// Prepares the creation of view
	/**
	@param pcquerywnd	: IN Query window pointer
	@param procname		: IN Procedure name
	@param stringproc	: OUT Query for procedure creation
	@returns void
	*/
	void PrepareCreateProcedure	(MDIWindow	*pcquerywnd, const wyChar *procname, wyString &strproc);

	/// Prepares the creation of function
	/**
	@param pcquerywnd	: IN Query window pointer
	@param funcname		: IN Function name
	@param stringfunc	: OUT Query for function creation
	@returns void
	*/
	void PrepareCreateFunction	(MDIWindow	*pcquerywnd, const wyChar *funcname, wyString &strfunc);

	/// Prepares the creation of trigger
	/**
	@param pcquerywnd	: IN Query window pointer
	@param triggername	: IN Trigger name
	@param stringtrigger: OUT Query for view creation
	@returns void
	*/
	void PrepareCreateTrigger	(MDIWindow	*pcquerywnd, const wyChar *triggname, wyString &strtrigger);

	/// Function takes care of menu enabling recursively
	/**
	@param hsubmenu		: IN Handler to sub menu
	@param state		: IN State (Enabled/Disable)
	@returns void		
	*/
	static void				RecursiveMenuEnable(HMENU hsubmenu, wyBool iswindowmenu = wyFalse, wyInt32 state = MF_DISABLED);

	/// 
	/**
	@returns void
	*/
	void					SetYogMenuOwnerDraw();
	
	///
	/**
	@returns void
	*/
	void					DestroyResources();
	/// Function used to Enable or Disable the buttons on the toolbar 
	/**
	@returns void
	*/
	void					HandleFirstToolBar();

	/// Refreshes the object browser
	/**
	@param pcquerywnd		: IN Query window HANDLE
	@returns void
	*/
	void					Refresh(MDIWindow *pcquerywnd);

	/// Checks the connection name
	/**
	@param connectionname	: IN Connection name
	@param newcon			: IN New connection or not
	*/
	static wyInt32          CheckConnectionName(wyChar *connectionname, wyBool newcon);

	/// Converts the old Ini file from ANSI to UTF-8
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool					ConvertIniFileToUtf8();

	/// Actual Coversion of ini file takes place
	/**
	@returns wyTrue if Successful else wyFalse
	*/
	wyBool					Convert(wyString &dirstr);

    void                    MigrateFiles();
    wyBool                  CopyAndRename(wyString&, wyString&, wyString&);
    wyInt32                 StripExe(wyWChar *directory, wyString &directorystr);
    void                    HandleAutocompleteTagFiles();
    wyBool                  MigrateSpecificFileToAppFolder(wyString &pwdpath, wyString &apppath, wyWChar *filename, wyWChar *extension);
    wyBool                  CreateIni(wyString &apppath);
    void                    HandleFiles(wyWChar *filename, wyWChar *extension);

	/// Write utf-8 converted Password into temporary file
	/**
	@param	conncount	: IN Connection Count
	@param	whichpwd	: IN Is it SShpwd or password
	@param	path		: IN File Path	
	@returns void
	*/
	void					ConvertAndWritePwd(wyString	&conncount, wyChar *whichpwd, wyString	&path);

	/// Function takes care of the things to be done when connect button is pressed in connection window
	/**
	@param hwnd			: IN Window HANDLE
	@param dbname		: IN Connection information
	@returns void
	*/
    void                    OnConnect(HWND hwnd, ConnectionInfo * dbname);

	/// Handles the window font and color
	/**
	@param hwnd			: IN Window HANDLE
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message pointer
	@returns zero
	*/
    wyInt32                 OnWmCtlColorStatic(HWND hwnd, WPARAM wparam, LPARAM lparam);

	/// Loads the main icons
	/**
	@returns void
	*/
    void                    LoadMainIcon();

	/// Handles the window creation
	/**
	@returns void
	*/
    void                    OnCreate();

	/// Function handles the things to do when a window is activated 
	/**
	@param wparam		: IN Unsigned message parameter
	@returns void
	*/
    void                    OnActivate(WPARAM wparam);

	/// Handles the menu option when no connection window is open
	/**
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message pointer
	@returns void
	*/
    void                    HandleMenuOnNoConnection(WPARAM wparam, LPARAM lparam);


   	/// Handles the initialization of popup menus
	/**
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns wyTrue on success else wyFalse
	*/
    wyBool                  OnWmInitPopup(WPARAM wparam, LPARAM lparam);

	///Handle wehn destroy menu.this used to handle the Table->engine menu should execute query only once when main menu is oponed
	/**
	@param hmenu : IN handle to the menu
	@return void
	*/
	void					HandleOnMenuDestroy(HMENU hmenu);

	/// Handles notification
	/**
    @param hwnd			: IN Frame window HANDLE
	@param lparam		: IN Long message pointer
	@returns one on success
	*/
    wyInt32                 ONWmMainWinNotify(HWND hwnd, LPARAM lparam, WPARAM wparam);
    /// Handles toolbar combobox
    /**
    @param wparam           : IN Unsigned window parameter
    @returns void
    */
    void                    HandleToolCombo(WPARAM wparam);

    /// Handle the execution of current query
    /**
    @param hwndactive       : IN Active window HANDLE
    @param pcquerywnd       : IN Query window HANDLE
    @param pceditorbase     : IN Editor base pointer
    @param wparam           : IN Unsigned int message pointer
    @returns void
    */
    void                    HandleExecuteCurrentQuery(HWND hwndactive, MDIWindow *pcquerywnd, 
                                                        EditorBase *pceditorbase, WPARAM wparam);

    /// Handle the execution of all query
    /**
    @param hwndactive       : IN Active window HANDLE
    @param pcquerywnd       : IN Query window HANDLE
    @param pceditorbase     : IN Editor base pointer
    @returns void
    */
    void                    HandleExecuteAllQuery(HWND hwndactive, MDIWindow *pcquerywnd, EditorBase *pceditorbase);


    void                    HandleExecuteExplain(HWND hwndactive, MDIWindow *pcquerywnd, EditorBase *pceditorbase, wyBool isExtended = wyFalse); 

    /// Handle the execution of selected query
    /**
    @param hwndactive       : IN Active window HANDLE
    @param pcquerywnd       : IN Query window HANDLE
	@param pceditorbase     : IN Editor base pointer
    @returns void
    */
    void                    HandleExecuteSelQuery(HWND hwndactive, MDIWindow *pcquerywnd, EditorBase *pceditorbase);

    /// Things to do after the query execution is over
    /**
    @param pcquerywnd       : IN Query window HANDLE
    @returns void   
    */
    void                    OnQueryExecFinish(MDIWindow *pcquerywnd);

    /// Gets the corresponding Object Browser node for the Advanced Tab Editor
    /*
    @param  wnd             : IN Query window HANDLE
    */
    HTREEITEM GetTreeItem(MDIWindow *wnd, wyString& hitemname);

    /// Creates new query editor
    /**
    @param pcquerywnd       : IN/OUT  Query window HANDLE
    @returns void   
    */
    void                    CreateNewQueryEditor(MDIWindow *pcquerywnd);

	///delete active query tab item
	 /**
    @param pcquerywnd       : IN/OUT  Query window HANDLE
    @returns void   
    */
	void                    DeleteQueryTabItem(MDIWindow *pcquerywnd);

	///navigate query tabs
	/**
    @param pcquerywnd       : IN/OUT  Query window HANDLE
	@param moveup           : IN movetoup indicates type of navigation( up or down).
    @returns void   
    */
	void                   HandleTabNavigation(MDIWindow *pcquerywnd, wyBool movetoup);

	/// Creates new Query Builder
    /**
    @param pcquerywnd       : IN/OUT  Query window HANDLE
    @returns void   
    */
	void					CreateNewQueryBuilder(MDIWindow *pcquerywnd);


	void					CreateNewTableDataTab(MDIWindow *pcquerywnd, wyBool isnewtab = wyFalse);

    /// Refresh handler
    /**
    @param pcquerywnd       : IN/OUT Query window HANDLE
    @returns void   
    */
    void                    HandleOnRefresh(MDIWindow *pcquerywnd);


	/// Collapse Handler
	/**
	@param pcquerywnd       : IN/OUT Query window HANDLE
	@returns void
	*/
	void                    HandleOnCollapse(MDIWindow *pcquerywnd);
	
	
	///Calling Refresh or Excute query functions based on SwitchShortcut Preference
	/**
	 @param hwndactive      : IN Active window HANDLE
	 @param pcquerywnd      : IN Query window HANDLE
	 @param wparam          : IN Unsigned int message pointer
	 @isrefresh				: IN for diffirentiating ACCEL_REFRESH and ACCEL_EXECUTE cases
	 @returns void 
	*/
	void                    HandleOnRefreshExecuteQuery(HWND hwndactive, MDIWindow *pcquerywnd, 
														WPARAM wparam, wyBool isrefresh = wyFalse);
    /// Takes care of window resizing
    /**
    @param wparam           : IN Unsigned int message parameter
    @returns wyFalse always
    */
    wyInt32                 OnWmSize(WPARAM wparam);
	
    /// Handles the goto line number option
    /**
    @param hwndactive       : IN Handler to active window
    @param wnd              : IN/OUT Query window pointer
    @returns void
    */
    void                    GoToLine(HWND hwndactive, MDIWindow *wnd);

    /// Handles when export data is selected
    /**
    @param wnd              : IN/OUT Query window HANDLE
    @returns wyTrue on success else wyFalse
    */
    wyBool                  OnExportData(MDIWindow *wnd);

    /// Handles when create database is selected
    /**
    @param hwndactive       : IN Window HANDLE
    @param wnd              : IN/OUT Query window pointer
    @returns void
    */
    void                    OnCreateDatabase(HWND hwndactive, MDIWindow *wnd);

    /// Handles when create function is selected
    /**
    @param hwndactive       : IN Window handler
    @param wnd              : IN/OUT Query window pointer
    @returns void
    */
    void                    OnCreateFunction(HWND hwndactive, MDIWindow *wnd);

    /// Handles when create procedure is selected
    /**
    @param hwndactive       : IN Window HANDLE
    @param wnd              : IN/OUT Query window pointer
    @returns void
    */
    void                    OnCreateProcedure(HWND hwndactive, MDIWindow *wnd);

    /// Handles when create view is selected
    /**
    @param hwndactive       : IN Window HANDLE
    @param wnd              : IN/OUT Query window pointer
    @param isqb         	: IN flag for create view from querybuilder
	@returns void
    */
	void                    OnCreateView(HWND hwndactive, MDIWindow *wnd, wyString *qbquery = NULL);
	/// Handles when create event is selected
    /**
    @param hwndactive       : IN Window HANDLE
    @param wnd              : IN/OUT Query window pointer
    @returns void
    */
	void                    OnCreateEvent(HWND hwndactive, MDIWindow *wnd);

    /// Handles when create trigger is selected
    /**
    @param hwndactive       : IN Window handler
    @param wnd              : IN/OUT Query window pointer
    @returns void
    */
    void                    OnCreateTrigger(HWND hwndactive, MDIWindow *wnd);

    /// Handles when alter database is selected
	/**
	@param hwndactive       : IN Window handler
	@param wnd              : IN/OUT Query window pointer
    @returns void
	*/
	void                    OnAlterDatabase(HWND hwndactive,MDIWindow *wnd);

	/// Handles when alter view is selected
    /**
    @param wnd              : IN/OUT Query window pointer
    @returns void
    */
    void                    OnAlterView(MDIWindow *wnd);
	void                    OnAlterEvent(MDIWindow *wnd);

    /// Handles when alter function is selected
    /**
    @param wnd              : IN/OUT Query window pointer
    @returns void
    */
    void                    OnAlterFunction(MDIWindow *wnd);

    /// Handles when alter procedure is selected
    /**
    @param wnd              : IN/OUT Query window pointer
    @returns void
    */
    void                    OnAlterProcedure(MDIWindow *wnd);

    /// Handles when alter table is selected
    /**
    @param wnd              : IN/OUT Query window pointer
    @returns void
    */
    void                    OnAlterTrigger(MDIWindow *wnd);

    /// Handles when the about option is clicked
    /**
    @returns nResult parameter on success else returns one
    */
    wyInt32                 OnAbout();

    /// Sends all the existing MDI child window to ask whether to close or not.
    /**
    @param hwnd             : IN Window handler
    @returns wyTrue on success else wyFalse
    */
    wyBool                  OnWmClose(HWND hwnd);

    /// Creates a show values window
    /**
    @param wnd              : IN Query window manager
    @param type             :
    @returns void
    */
    void                    OnShowValues(MDIWindow *wnd, wyInt32 type);

    /// Creates a User Manager window
    /**
    @returns void
    */
    void                    OnManageUser();

    /// Creates a table dialog window
    /**
    @param wnd              : IN Query window manager
    @returns void
    */
    void                    OnTableDiag(MDIWindow *wnd);

    /// Creates a batch import window
    /**
    @param wnd              : IN Query window manager
    @returns void
    */
    void                    OnImportFromSQL(MDIWindow *wnd);


    /// Creates a preferences manager window
    /**
    @returns void
    */
    void                    ManagePreferences();

    /// Things to do before closing the application
    /**
    @returns zero on success else one
    */
    wyInt32                 OnExit();

    /// Gets the command name
    /**
    @param menustring       : IN/OUT Menu string
    @param cmdstring        : IN/OUT Command string
    */
    wyInt32                 GetCommand(wyWChar *menustring, wyWChar *cmdstring);


    /// Gets the keyboard shortcuts
    /**
    @param menustring       : IN/OUT Menu string
    @param shortcut         : IN/OUT Shortcut
    @returns the shortcut length
    */
    wyInt32                 GetKBShortcut(wyWChar *menustring, wyWChar *shortcut);

    /// Handles the table drop down menu
    /**
    @param tbh              : IN Contains information used to process toolbar notification messages
    @param buttonid         : IN Tool bar icon id
    @returns void
    */
    void                    HandleTBDropDown(NMTOOLBAR *tbh, wyUInt32 buttonid);

    /// Handles the checking or unchecking of items
    /**
    @param pcquerywnd       : IN Query window pointer
    @param ischecked        : IN Checked or not
    @param menuid           : IN Menu id
    @returns wyTrue on success
    */
    wyBool                  HandleCheckMenu(MDIWindow *pcquerywnd, wyBool ischecked, wyUInt32 menuid);

    /// Command handler for CreateObject
    /**
    @param hwnd             : IN Window HANDLE
    @param wparam           : IN Long message pointer
    @returns void
    */
    void                    OnCreateObjectWmCommand(HWND hwnd, WPARAM wparam, wyChar *object);

    /// function to check for keywords file and get the keys
    /**
    @returns			wyTrue on success, otherwise wyFalse.
    */	
    static wyBool		CheckForAutoKeywords();

	/// Create the connection dialog
	/**
	@returns wyTrue
    */
	wyBool              CreateConnDialog(wyBool readsession = wyFalse);
	

	/// Creates the new connection using the same details
	/**
	@param hwndactive		: IN Active window HANDLE
	@param pcquerywindow	: IN Pointer to query window class
	@returns wyTrue
	*/
    wyBool              OnNewSameConnection(HWND hwndactive, MDIWindow *pcquerywnd);

	/// Enables the tool bar combo buttons
	/**
	@param hwndtool			: IN Tool window HANDLE
	@param hwndsecondtool	: IN Second tool window HANDLE
	@param hwndcombo		: IN Combo box window HANDLE
	@param enable			: IN Enable / disable the toolbar combo box
	@param isexec			: IN query is executing or not
	*/
    wyBool              EnableToolButtonsAndCombo(HWND hwndtool, HWND hwndsecondtool, 
                                                        HWND hwndcombo, wyBool enable, wyBool isexec = wyFalse);
    
    /// Initializes the combobox displayed in the create DB dialog with list of MySQL supported character sets
	/**
	@param hwnd		        : IN Parent window HANDLE
    @returns void
	*/
    void               InitCollationCombo(HWND hwnd); 

    /// Reinitializes the collation combobox displayed in the create DB dialog with list of Filtered MySQL supported collations
	/**
	@param hwnd		        : IN Parent window HANDLE
    @returns void
	*/
    void               ReInitRelatedCollations(HWND hwnd);
    
    /// Gets the Selcted Character Set from the combo box and if a invalid charset is typed then to fetch it we are using GetWindowtext() iscopy parameter is used. 
	/**
	@param hwnd		        : IN Parent window HANDLE
    @param iscopy           : IN Mode of Fetching the text    
    @returns void
	*/
    void               FetchSelectedCollation(HWND hwnd);

	///  find any query is selected in Query editor if selected return true
	/**
	@param pceditorbase     : IN Editor base pointer
	@returns void
	*/
	wyBool             IsQuerySelected(EditorBase *pceditorbase);
	
	/// Initialize the combobox in the CreateDB when alterdatabase is selected
    /**
	@param hwnd		        : IN Parent window HANDLE
    @returns void
	*/
	void				InitCharsetAndCollationInfo(HWND hwnd);

	/// handling help
    /** 
    @param hwnd             : IN Create object dialog handle
    @param object           : IN Object name
    @returns void
    */
	void				OnCreateObjectWmHelp(HWND hwnd, wyChar *object);

	///Handle for upgrade check
	/**
	@param isexplict : IN its wyTrue if its explicit check(help->check for upgrade), e;se wyFalse(while start up)
	@return voi
	*/
	void				CheckForUpgrade(wyBool isexplict = wyFalse);	

	///Handles the community ribbon
	/**
	@return void
	*/
	void				HandleCommunityRibbon();

	///Handle destroy Main tollbar resources(It requires while changing the icon size)
	/**
	@return void
	*/
	void				DestroyToolBarResources();


	//Resize the Tool mbar as per the Icon size selection
	/**
	
	*/
	void				ReArranageToolBar();

	/// Function to handle TOOLBAR dropdown message.
    /**
    @param tbh					: IN toolbar notify info
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool				OnTBDropDown(NMTOOLBAR *tbh);

     
     /// Function to handle Export Connections Details
    /**

    */
    void                OnExportConnectionDetails();


    /// Function to handle Import Connection Details
    /**

    */
    void                OnImportConnectionDetails();

	/// Function to update dropdown structure
	/**

	*/
	void                UpdateDropDownStruct(wyString tabname);


	wyBool				SaveConnectionDetails(wySQLite	*ssnsqliteobj = NULL);
	wyBool				SaveConnectionDetails2(wySQLite	*ssnsqliteobj = NULL);
	wyBool				SaveSessionFile(HWND hwnd, wyBool issaveas);
	wyBool				OpenSessionFile();
	void				MigratePasswordofSessionFile(wyString filename);
	wyString			m_sessionfile;
	wyString			m_sessionname;
	wyString			m_previoussessionfile;
	wyBool				m_previoussessionedited;
	wyBool				WriteTabDetailsToTable2(tabeditorelem *temptabeditorele, CTCITEM quetabitem, wyInt32 tabid, wyInt32 position, wyInt32 id, TabTypes *tabactive, MDIWindow *wnd,wySQLite	*ssnsqliteobj = NULL);
	wyBool				WriteTabDetailsToTable(tabeditorelem *temptabeditorele, tabeditorelem *temptabeditorele_temp, wyInt32 tabid, wyInt32 position, wyInt32 id,TabTypes *tabqueryactive,wySQLite	*ssnsqliteobj = NULL);
	wyBool				WriteTabDetailsToTempList(tabeditorelem *temptabeditorele, CTCITEM quetabitem, wyInt32 tabid, wyInt32 position, wyInt32 id,TabTypes *tabqueryactive, MDIWindow *wnd);
	wyBool				SetStatusParts2(HWND hwndstatus);
	wyInt32				OnStatusBarWmCtlColorStatic(HWND hwnd, WPARAM wparam, LPARAM lparam);
	void				CreateIniFileBackup();
	HFONT			    m_trialtextfont;
	HFONT			    m_trialbuyfont;
	//HBRUSH				m_trialbuybrush;
    //flag tells whether to close all the mdi windows
    wyBool              m_iscloseallmdi;

    //save to all selection
    wyInt32             m_framewndsaveselection;

    //handle to connection tab
	HWND	m_hwndconntab;
   
    //flag to close connection tab or not
	wyBool	m_closetab;

    // Connection tab pointer to frame window
	ConnectionTab			*m_conntab;  

    //Text used in find/replace dialog
    wyString            m_findtext;

    //For persistance in find/replace dialog
    FINDREPLACE         m_frstruct;

    //original window procedure for find/replace dialog
    WNDPROC             m_findproc;

    wyInt32             m_languagecount;

    wyInt32             m_popupmenucount;

    HWND                m_hwndtooltip;

    wyInt32             m_editorcolumnline;

	wyInt32				m_showwindowstyle;

	HWND				m_hwndrestorestatus;

	HANDLE				m_savetimerevent;
	HANDLE				m_sqlyogcloseevent;
	HANDLE				m_sessionsaveevent;
	HANDLE				m_sessionchangeevent;
	HANDLE				m_savethread_handle;
	wyBool				m_sqlyogclosed;
	wyBool				m_mouseoverbuy;
	wyString			m_trialtext;
	wyBool				m_issessionedited;
#ifndef COMMUNITY
	wyInt32				m_closealltrans;
	wyInt32				m_intransaction;
	wyBool				m_topromptonimplicit;
	wyBool				m_topromptonclose;
#endif
};

/// Function to rstore connections
/**
*/
	typedef struct ConnectFromList_arg
	{
		wyString		connstr;
		wyString		pathstr;
		wyString		historydata;
		wyString		obdb;
		ConnectionInfo	conninfo;
		wyBool			isfocussed;
		wyIni *inimgr;
		List			*tabdetails;
		MDIlist			*tempmdilist;
		wyInt32			id;
		wyString		sessionfile;
		wyInt32			*issessionedited;
	}MY_ARG;
wyBool	ConnectFromList(wyString* failedconnections, wyString* sessionfile = NULL);
unsigned __stdcall  ConnectFromList_mt(void* arg_list);
unsigned __stdcall  Htmlannouncementsproc(void* arg);
unsigned __stdcall  sessionsavesproc(void* arg);
#endif
