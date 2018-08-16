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

#include <stdlib.h>

#include "TabTypes.h"
#include "MDIWindow.h"
#include "include.h"

#define NO_PENDING_QUERIES _("No pending queries to be executed")
#define TABLE_NAME_MISSING _("Table name not specified")

#define UM_CLOSEDLG         12569

class TabTypes;

class TableTabInterface : public TabTypes
{
public:

    ///Handle to the Static Windows
    HWND                                    m_hstattblname;
    HWND                                    m_hstatdbname;
    HWND                                    m_hstattabletype;
    HWND                                    m_hstatcharset;
    HWND                                    m_hstatcollation;


    ///Handle to the Edit-boxes and Combo-boxes
    HWND                                    m_hedittblname;
    HWND                                    m_hcmbdbname;
    HWND                                    m_hcmbtabletype;
    HWND                                    m_hcmbcharset;
    HWND                                    m_hcmbcollation;

    ///Handle to the buttons
    HWND                                    m_hbtnsave;
    HWND                                    m_hbtncancelchanges;
	HWND									m_hbtncancel;
	
	HWND									m_hwndgrip;

    MYSQL_RES                               *m_myrestablestatus;
    
    //..Window Procedures
    WNDPROC                                 m_wporigtblname;
    WNDPROC                                 m_wporigdbname;
    WNDPROC                                 m_wporigengine;
    WNDPROC                                 m_wporigcharset;
    WNDPROC                                 m_wporigcollate;
    WNDPROC                                 m_wporigbtnsave;
    WNDPROC                                 m_wporigbtncancelchanges;
	WNDPROC                                 m_wporigbtncancel;

    TableTabInterfaceTabMgmt*               m_ptabintmgmt;

	List									m_controllist;
    
    //..A bool variable which indicates whether this is an "Alter Table" Operation or not..
    wyBool                                  m_isaltertable;

    wyBool                                  m_open_in_dialog;

    //..Indicates whether FK is supported or not..?
    wyBool                                  m_isfksupported;
	
	//..Indicates if the mySQL cluster version supports foreign keys for NDBcluster or not.. 
	wyBool									m_isfkforndbcluster;

    ///The index of sub-tab to be opened on opening the Alter Table tab
    wyInt32                                 m_setfocustotab;

    ///currently selected database name
    wyString                                m_dbname;
    
    ///The table name which is being altered
    wyString                                m_origtblname;

    wyBool                                  m_ismysql41;

    ///flag to indicate that something is changed in the tab
    wyBool                                  m_dirtytab;

    ///flag indicates not to process EN_CHANGE message
    wyBool                                  m_disableenchange;

    MDIWindow*                              m_mdiwnd;

    ///Handle to the Last Focused window
    HWND                                    m_lastfocus;

    //..Used for storing original values for Alter table
    wyString                                m_origtableengine;
    wyString                                m_origcharset;
    wyString                                m_origcollate;
    
    //..Stores previous value of the combo-box.
    wyString                                m_prevvalue;

    wyBool                                  m_issdempty;                    
    wyString                                m_sddatabase;                   /// schema designer database
    wyString**                              m_listoftablescreated;
    wyUInt32                                m_ntablescreated;
    wyBool                                  m_istablealtered;


	RECT									m_dlgrect;

	HBRUSH									m_objbkcolor;
    
    wyBool                                  m_isbuffereddraw;

    TableTabInterface(HWND hwnd, wyBool open_in_dialog, wyBool isaltertable, wyInt32 setfocustotab);

    ~TableTabInterface();

    wyBool Create();

    static LRESULT	CALLBACK	TableTabInterfaceWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    static LRESULT	CALLBACK	SysKeyDownWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    //static LRESULT	CALLBACK	TabIntBottomFrameWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    //static LRESULT	            GroupBoxWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    VOID						Resize(wyBool issplittermoved = wyFalse);

    //Functions creates the Fields, Advanced Properties, Indexes, FOreign Keys, Preview tabs to the Window
    /*
    @param  wnd             :   MDIWindow pointer
    */
    wyBool		                CreateTabMgmt();


    /// Shows/Hides all the content of the tab
    /**
    @param tabindex				: IN currently selected tab index.
    @param status				: IN wyTrue/wyfalse
    @returns void
    */
	VOID						ShowTabContent(wyInt32 tabindex, wyBool status);

    ///Called by MDIWindow
    wyBool                      CloseTab(wyInt32 index);

    ///Called by TabModule
    VOID		                HandleTabControls(wyInt32 tabcount, wyInt32 selindex);

    VOID                        HandleFlicker();

    void                        SetBufferedDrawing(wyBool isset);

    //void        OnGetChildWindows(wyInt32 tabcount, LPARAM lparam);

    ///Enables/Disables toolbar icons and sets the focus
    VOID		OnTabSelChange();

    ///
    VOID		OnTabSelChanging();

    ///Prompts the user about any change in the Tab(depends on Preference Settings)
    wyInt32		OnTabClosing(wyBool ismanual);

    VOID		OnTabClose();

    wyInt32		OnWmCloseTab();

    /// Creates windows like "Table name", "Database", etc.
    /*
    @param hwnd : parent window handle
    */
    void                        CreateOtherWindows();

    /// Sets the title of the MDIWindow
    void                        TabInterfaceTitle(wyString *tunneltitle);
    
    /// Handles the WM_NOTIFY case
    /**
    @param hwnd         : Window HANDLE
    @param lparam       : Long message parameter
    @param wparam       : Unsigned message parameter
	@returns zero on success else one
    */
    wyInt32             OnWmNotify(HWND hwnd, WPARAM wparam, LPARAM lparam);

    /// Handles the CTCN_SELCHANGING case
    /**
    @param hwnd         : Window HANDLE
    @param tabid        : Table id
    */
    wyInt32                 OnCtcnSelChanging(HWND hwnd, wyInt32 tabid);

    /// Handles WM_COMMNAD for the tab control
    /**
    @param hwnd			            : IN handle to the control
    @param wParam		            : IN window procedure WPARAM
    @param lParam		            : IN window procedure LPARAM
    @returns LRESULT 1
    */
    wyBool                  OnWmCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
    
    /// Perform the "Save" operations
    /*
    @param  queryexecuted           :   OUT sets to True if any query is executed
    @returns wyFalse, if any executed, any query fails, else wyTrue
    */
    wyBool                  SaveTable(wyBool &queryexecuted);
    
    /// Sets the TableTabInterface tab title after "Save"
    wyBool                  SetTabItem(HWND hwnd, wyInt32 tabindex, wyString& text, wyUInt32 mask, wyInt32 image, wyInt32 lparam, wyBool addasterisk = wyFalse);

    /// Handles the "Save" and after "Save" operations
    /*
    @param  onclosetab              :   IN  wyTrue, if functions is called on closing the tab/MDIWindow/FrameWindow, else wyFalse
    @param  showsuccessmsg          :   IN  wyFalse, if functions is called on closing the tab/MDIWindow/FrameWindow, else wyTrue
    @returns    wyFalse on query-execution-fail, else wyTrue
    */
    wyBool                  OnClickSave(wyBool onclosetab = wyFalse, wyBool showsuccessmsg = wyTrue);

    /// Performs the "Revert" operations
    wyBool                  OnClickCancelChanges();

    /// Generates the query to be executed
    /*
    @param  query           :   OUT the query generated will be stored
    @param  showmsg         :   IN  wyTrue, if user clicks on "Save", wyFalse if the user goes to TabPreview
    @returns                    wyFalse, if any client-side validations fails, else wyTrue
    */
    wyBool                  GenerateQuery(wyString &query, wyBool showmsg = wyFalse);
    
    /// Executes the Query
    /*
    @param  query           :   IN  the query to be executed
    @returns                :   wyTrue, if query is executed successfully, else wyFalse
    */
    wyBool                  ExecuteQuery(wyString &query);

    /// Performs the "Revert" operation for the Table-name, Database, Engine, Charset, Collation windows
    /*
    @returns                void
    */
    void                    CancelChanges();

    LRESULT                 OnWMSysKeyDown(wyUInt32 currenttab, WPARAM wParam, LPARAM lParam);

    /// Gets the databased from the OB and sets them to the "Database"combo-box
    void                    GetAllDatabases();

	void					SelectDatabase();

    /// Fill table type options
    /**
    @param  includedefault  :   IN  wyTrue for Create table, wyFalse for Alter table
	@returns void
    */
    void                    InitTableTypeValues(wyBool includedefault=wyTrue);

    /// Initializes the Charset Combobox with all the MySQL supported charsets
    /**
    @param  includedefault  :   IN  wyTrue for Create table, wyFalse for Alter table
    @returns void
    */
    void                    InitCharacterSetCombo(wyBool includedefault=wyTrue);

    /// Initializes the Collation Combobox with all the MySQL supported collations
    /**
    @param  includedefault  :   IN  wyTrue for Create table, wyFalse for Alter table
    @returns void
    */
    void                    InitCollationCombo(wyBool includedefault=wyTrue);

    /// Generates the "Rename" query
    /*
    @param  renamequery     :   OUT rename query is stored in this wyString object
    @param  showmsg         :   IN  wyTrue, if user clicks on "Save", wyFalse if the user goes to TabPreview
    */
    wyBool                  GetRenameQuery(wyString& renamequery, wyBool showmsg = wyFalse);

    /// Stores the new table name
    /*
    @param  tblname         :   OUT new table name is stored in this wyString object
    @param  showmsg         :   IN  wyTrue, if user clicks on "Save", wyFalse if the user goes to TabPreview
    */
    wyBool                  GetNewTableName(wyString& tblname, wyBool showmsg = wyFalse);

    /// Gets the combo-box value
    /*
    @param  hcombobox       :   IN  handle to the combo-box
    @param  value           :   OUT value of the combo-box
    */
    wyBool                  GetComboboxValue(HWND hcombobox, wyString& value);

    /// Fetches all Table values after "Save"
    wyBool                  ReInitializeAllValues();

    /// Sets the Focus on Initialize
    void                    SetInitFocus();

    /// Checks whether the Table supports Foreign-Keys
    /*
    @returns                    wyTrue, if table supports FK, else wyFalse
    */
    wyBool		            IsTableInnoDB(wyBool& error);

    /// Handles the Charset Combobox Change 
    /**
    @returns void
    */
    void                    HandleTabCharset();

    /// Called when user changes the Charset
    /*
    @param  charsetname     :   Charset name
    */
    void                    ReInitRelatedCollations(HWND hwnd, wyWChar *charsetname);

    /// ReInitializes the "Table name", "Database", "Engine", etc. after "Save"
    wyBool                  ReInitializeBasicOptions();

    /// Appends Engine, Charset, Collation to the Query
    /*
    @param  query           :   OUT query to be appended
    */
    void                    AppendBasics(wyString& query);

    /// marks the Tabbed Interface as dirty
    /*
    @param  flag            :   IN  wyTrue, if the tab is to be set to dirty, or wyFalse
    @param  onsave          :   IN  wyTrue, if it is to set after user Saves the current Create/Alter table tab, else wyFalse
    */
    void                    MarkAsDirty(wyBool  flag, wyBool onsave = wyFalse);

	void                    GetMaxWidthOfStaticText(wyInt32* col1, wyInt32* col2);

    //wyInt32                 GetTextLenOfStaticWindow(HWND hwnd);

    /// Refreshes the Object Browser after "Save"
    void                    RefreshObjectBrowser(wyString &database, wyString &tablename);

    void		            SetFont(HWND hgrid);
    void                    SetAllFonts();

    /// Sets focus to the new tab after user saves the "Create table" tab successfully and agrees to create another table
    void                    SetFocusToNewTab();

    /// Gets the list of unsaved wrappers from the Foreign-Keys tab 
    /// (Called when user alters FK(other than the last fk in the grid) without changing fk-name and it fails)
    /*
    @param  initrow         :   IN  Row index from where to fetch the unsaved rows
    @returns                List* of wrappers from all next rows
    */
    List                    *GetListOfUnsavedWrappers(wyInt32 initrow);

    /// Used by SD to point the exact row on the grid for the object name
    wyBool                  SetFocusToGridRow(wyString& objname);

    /// called by SD for confirmation to close the dialog
    /*
    @param  hparent         :   DialogBox handle
    @returns                wyTrue if user has not selected "Cancel" option, else, wyFalse
    */
    wyBool                  ConfirmClose(HWND hparent);


    wyBool                  GetMyResTableStatus();


	void                        OnDlgWMSizeInfo(LPARAM lparam, HWND hwnd);

	void						OnDlgWMPaint(HWND hwnd);

	void						CreateDlgCancel(HWND hwnd);
};