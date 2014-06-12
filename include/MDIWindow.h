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


#ifndef _QUERYWND_H_
#define _QUERYWND_H_

#include "FrameWindowHelper.h"
#include "ObjectBrowser.h"
#include "TabModule.h"
#include "TabEditor.h"
#include "TabMgmt.h"
#include "StatusbarMgmt.h"
#include "FrameWindowSplitter.h"
#include "DialogPersistence.h"
#include "Tunnel.h"
#include "sqlite3.h"
#include "ExportMultiFormat.h"
#include "AutoCompleteInterface.h"	
#include "ConnectionTab.h"
#include "Announcements.h"

#ifndef COMMUNITY
#include "ProfilerAdvisors.h"
#endif

class TabMgmt;
class AutoCompleteInterface;
class TabModule;
class TabHistory;
class TabObject;
class TabTableData;

class tabeditorelem : public wyElem
{
public:
	TabEditor*	m_pctabeditor;
	//TabQueryBuilder* m_
	wyInt32		m_tabptr;
	wyInt32		m_id;
	wyInt32		m_tabid;
	wyInt32		m_iimage;
	wyInt32		m_position;
	wyBool		m_ispresent;
	wyString	m_psztext;
	COLORREF	m_color;//Color of tab
	COLORREF	m_fgcolor;//Text color of tab
	wyString	m_tooltiptext;//tooltip text (also filename if it is a file)
	wyBool		m_isfile;
	wyBool		m_isfocussed;
	wyBool		m_isedited;
	wyInt32		m_leftortoppercent;
};

class TableDiag
{
public:

	TableDiag(HWND hwndParent, Tunnel * tunnel, PMYSQL mysql);

	virtual		~TableDiag();

    /// Handles callback procedure for TableDlg
    /**
    @param phwnd        : Window HANDLE
    @param pmessage     : Window messages
    @param wparam      : Unsigned message parameter
    @param lparam      : Long message parameter
	@returns zero
    */
	static	INT_PTR	CALLBACK		TableDlgProc(HWND phwnd, UINT pmessage, WPARAM	wparam, LPARAM lparam);

    /// Window handler
	HWND				m_hwnd;

    /// Handler for the parent window
    HWND				m_hwndparent;

    /// Handler for the combo box
    HWND				m_hwndcombo;

    /// Handler to the list tables
    HWND				m_hwndlisttables;

    /// Handler to list result
    HWND				m_hwndlistresult;

    /// Check / uncheck handler
    HWND				m_hwndcheck;

    /// Pointer to mysql pointer
	PMYSQL				m_mysql;

    /// Tunnel pointer
	Tunnel	            *m_tunnel;

    /// Persistence variable
	Persist				*m_p;
	
	wyBool				m_ischk;
	
	/// Flag to check whether ListView's ListBox State is changed because of automation
	wyBool				m_isautomated;

	/// List to store the dialogbox component values
	
	List				m_controllist;

	/// To store the original size of the dialog box

	RECT				m_dlgrect;

	/// To store the co-ordinates of the dialog box wrt the screen
	
	RECT				m_wndrect;


    /// It initializes the database combobox
    /**
	@returns wyTrue 
    */
	wyBool				InitializeCOMBOBOX();

    /// This function changes the tables in the listcontrol whenever a new database is selected.
    /**
	@returns wyTrue on success else wyFalse
    */
	wyBool				OnComboChange( HWND );

    /// This function toggles the check uncheck all database option.
    /**
	@returns wyTrue 
    */
	
	wyBool				OnCheckClick();
    
    // Sets the list box into extended style so that we can use checkboxes in the listbox.
    // Also it initializes the report list with a columns.
    /**
	@returns wyTrue on success else wyFalse
    */
	wyBool				SetListBoxExtendedStyle();

    // The function gets the DBname from the COMBOBOX and copies it in the buffer.
    /**
    @param dbname       : Database name
	@returns wyTrue 
    */
	wyBool				GetDBName(wyString &dbname);

	// Functions to implement diagnostics features.
    /**
	@returns wyTrue on success else wyFalse
    */
	wyBool				DiagOptimize();

    // The function prepares the check table query for the table diagnostics.
    /**
	@returns wyTrue on success else wyFalse
    */
	wyBool				DiagCheck();
	
    /// The function prepares the analyze table query for the table diagnostics.
    /**
	@returns wyTrue on success else wyFalse
    */
	wyBool				DiagAnalyze();

    /// Handles the repair dialog
    /**
	@returns wyTrue 
	*/
	wyBool				DiagRepair();
	
    /// Initializing procedure for TableDig
    /**
    @param hwnd         : Window HANDLE
    @param lparam       : Long message parameter
	@returns zero
    */
	
	wyInt32 	TableDlgProcInit(HWND phwnd, LPARAM plparam);


    /// Command procedure for TableDig
    /**
    @param hwnd         : Window HANDLE
    @param pctablediag  : Table dialog pointer
    @param wparam       : Unsigned message parameter
	@returns wyFalse
    */
		
	wyInt32 	TableDlgProcCommand(HWND hwnd, TableDiag *pctablediag, WPARAM wparam, LPARAM plparam);

    /// Diagnosis and repair
    /**
    @param query        : Query
    @param myresult     : Mysql result
	@returns wyTrue on success else wyFalse
    */
            
	MYSQL_RES   *DiagRepairOther(wyString &query);

    /// Diagnosis and repair with binary log
	/**
	@param query	: Query
	@param count	: Item count
	@returns wyTrue on success else wyFalse
	*/
            wyBool      DiagRepairBinLog(wyString &query, wyInt32 count);

    /// The function prepares the repair table query for the table diagnostics.
    /**
    @param query    : Query
    @param csi      : Showing information
	@returns wyTrue on success else wyFalse
    */
            wyBool      DiagCheckAddOptions(wyString &query, CShowInfo *csi);

	/// Function to check uncheck all the tables in the listbox.
    /**
    @param ncheckstate      : State
	@returns wyTrue on success else wyFalse
    */
	        wyBool				SetCheckUncheck (wyInt32 ncheckstate);	

	/* Function to Disable / Enable TableDiag. Controls .
	 @param hwnd     : Window HANDLE
	 @BOOL wyBOOL- TRUE for Enabling, FALSE for Disabling.
	*/
			void				EnableDisableTableDiag( HWND , BOOL);
	
	/* Function to handle the event on ListBox
	@param hwnd         : Window HANDLE
    @param pctablediag  : Table dialog pointer
	*/
			void TableDlgListViewEvent(HWND, TableDiag*);

	/* Function to handle resize
	@param hwnd         : Window HANDLE
    @param pctablediag  : Table dialog pointer
	*/
			void TableDlgResize(HWND, TableDiag*);

	/* Function to reposition the controls
	@param pctablediag  : Table dialog pointer
	*/		
			void PositionCtrls(TableDiag *pctablediag);

	/* Function to add the dialog controls to a list
	@param pctablediag  : Table dialog pointer
	*/		
			
			void GetCtrlRects(TableDiag *pctablediag);

	/* Sets the minimum size of the window
	@param pctablediag  : Table dialog pointer
	*/	
			void OnWMSizeInfo(LPARAM lparam, TableDiag *pctablediag);

	///Function thats draws the window contents using double buffering
    /**
    @param hwnd             : IN window handle
    @returns void
    */
			void OnPaint(HWND hwnd);

};

class MDIWindow
{
public:

    /// Parametric constructor
    /**
    @param hwnd     : Window HANDLE
    @param conninfo : Connection information
    @param dbname   : Database name
    @param title    : Title
    */
    MDIWindow(HWND hwnd, ConnectionInfo * conninfo, wyString &dbname, wyString &title);
	
    virtual		~MDIWindow();

    /// Window procedure for the window.
    /**
    @param phwnd        : Window HANDLE
    @param pmessage     : Window messages
    @param pwparam      : Unsigned message parameter
    @param plparam      : Long message parameter
	@returns zero
    */
	static	LRESULT	CALLBACK	WndProc(HWND pHWND, UINT pMessage, WPARAM pWPARAM, LPARAM pLPARAM);

    /// Dialog Procedure for the Flush Dialog Box
    /**
    @param phwnd        : Window HANDLE
    @param pmessage     : Window messages
    @param pwparam      : Unsigned message parameter
    @param plparam      : Long message parameter
	@returns zero
    */
	static	INT_PTR	CALLBACK	FlushWndProc(HWND pHWND, UINT pMessage, WPARAM pWPARAM, LPARAM pLPARAM);

    /// Window procedure for the template dialog box.
    /**
    @param phwnd        : Window HANDLE
    @param pmessage     : Window messages
    @param pwparam      : Unsigned message parameter
    @param plparam      : Long message parameter
	@returns zero
    */
	static	INT_PTR	CALLBACK	TemplateWndProc(HWND phwnd, UINT pmessage, WPARAM pwparam, LPARAM plparam);

	

	/// Methods to enable disable everything on start/end of query execution 
    /**
    @param enable       : Enable or disable
	@returns void
    */
	void				EnableWindowOnQueryExecution (wyBool enable);

    /// Enables or disables the buttons on query execution period
    /**
	@returns void
    */
	void				EnableToolOnNoQuery();

	///Set the text color and backgroung
	/**
	@param wparam      : Unsigned message parameter
    @param lparam      : Long message parameter
	*/
	wyInt32			   OnDlgProcColorStatic(WPARAM wparam, LPARAM lparam);

	/// The function to stopping query 
    /**
	@returns wyTrue on success else wyFalse
    */
	wyBool				StopQuery();
	
	
    /// Create the main window.
    /**
	@returns wyTrue
    */
	wyBool				Create(wyBool iscon_res = wyFalse, ConnectionInfo* conninfo = NULL);

    /// Create the query window.
    /**
    @param hwnd     : Window HANDLE
    @param mysql    : Pointer to mysql pointer
	@returns window handler
    */
	HWND				CreateQueryWindow(HWND hwnd, PMYSQL mysql);

    /// Function to set the default database when a new query window is selected.
    /**
	@returns void
    */
	void				SelectDefaultDatabase();

    /// Create the object browser
    /**
    @param hwnd     :Window HANDLE
	@returns void
    */
	void				CreateObjectBrowser (HWND hwnd);

    /// Create the  Tabcontroller.
    /**
    @param hwnd     :Window HANDLE
	@returns void
    */
	void				CreateTabController(HWND hwnd);

    /// Create the query status 
    /**
    @param hwnd     :Window HANDLE
	@returns void
    */
	void				CreateQueryStatus	(HWND hwnd);

    /// Create the vertical splitter
    /**
    @param hwnd     :Window HANDLE
	@returns void
    */
	void				CreateVSplitter		(HWND hwnd);

    /// Creates a new table
    /**
	@returns void
    */
	void				CreateTableMaker	();

    /// After creating various windows we set the Window Position, so that no window overlap other window.
    /**
	@returns void
    */
	void				SetWindowPositions();

    /// Makes the title dirty after every change 
    /**
	@returns wyTrue
    */
	wyBool				SetDirtyTitle();

    /// Function sets various information of the querywindow tab data when the window is selected.
    /**
	@returns wyTrue on success else wyFalse
    */
	wyBool				SetVariousStatusInfo();

    /// Function to SHOW/HIDE the object browser window.
    /**
	@returns wyTrue
    */
	wyBool				ShowOrHideObjBrowser();

    /// Function to SHOW/HIDE the edit window.
    /**
	@returns wyTrue
    */
	wyBool				ShowOrHideEditWindow();

    /// Function to SHOW/HIDE the result window when the user select HIDE Result Window in the
    /// edit menu.
    /**
	@returns wyTrue
    */
	wyBool				ShowOrHideResultWindow ();

    /// Handles toolbar
    /**
	@returns void
    */
	VOID				HandleToolBar();
	
    /// Gets the host information
    /**
	@returns mysql host
    */
	const wyChar		*GetMySQLHost();

    /// Gets the user information
    /**
	@returns mysql user
    */
	const wyChar		*GetMySQLUser();

    /// Gets the vertical spliter
    /**
	@returns query splitter object
    */
	FrameWindowSplitter	    *GetVSplitter();

        /// Gets the query status
    /**
	@returns query status object
    */
	StatusBarMgmt	    *GetStatus();

    /// Takes care of resizing the window.
	/**
	@returns wyTrue
	*/
	void				Resize();

    /// Ask or confirmation to save file and return the value.
    /**
	@returns non zero on success else zero
    */
	wyInt32				ConfirmSaveFile ();

	/// Handle the save option in the file menu
    /**
	@returns wyTrue on success else wyFalse
    */
	wyBool				HandleFileSave();

    /// Implements the save option in the file menu
    /**
	@param istabclose			: IN flag tells tab is closing or not
	@returns wyTrue on success else wyFalse
    */
	wyBool				SaveFile (wyBool istabclose = wyFalse);

	/// Handle the save as option in the file menu for all tabs
    /**
	@returns wyTrue
    */
	wyBool				HandleSaveAsFile();

    /// Implements the save as option in the file menu(query editor)
    /**
	@returns wyTrue
    */
	wyBool				SaveAsFile ();

	// This function opens the sql file for Query editor and XML file for Layout(QB or SD) 
    /**
	@param filetype  : IN file type to be selected in Open dialog
	@param issametab : IN opens in same tab or new tab
	@returns wyTrue on success else wyFalse
    */
	wyBool				HandleFileOpen(wyInt32 filetype, wyBool issametab = wyFalse);

	//Handle  open files
	/**
	@param filename       : IN filename to be loaded
	@param issametab	  : IN opens in same tab or new tab
	@param isrecentfiles  : IN open file from 'Recent Files' or not
	@returns wyTrue on success else wyFalse
	*/
	wyBool				OpenFileinTab(wyString *filename, wyBool issametab = wyFalse, wyBool isrecentfiles = wyFalse);

    // This function opens the SQL file asked from user and copies its content in the edit box.
    /**
	@param filename       : IN filename to be loaded
	@param issametab	  : IN opens in same tab or new tab
	@param isrecentfiles  : IN open file from 'Recent Files' or not
	@returns wyTrue on success else wyFalse
    */
	wyBool				OpenSQLFile (wyString *filename, wyBool issametab = wyFalse, wyBool isrecentfiles = wyFalse);
	wyBool				OpenSQLFile2 (wyString *filename, EditorBase *peditorbase, wyBool isrecentfiles = wyFalse);
	// This function opens the Schema file asked by user and copies its content in the schema designer.
    /**
	@param filename         : IN filename to be loaded
	@param issametab		: IN opens in same tab or new tab
	@param isrecentfiles	: IN open file from 'Recent Files' or not
	@returns wyTrue on success else wyFalse
    */
	wyBool				OpenSchemaFile (wyString *filename, wyBool issametab = wyFalse, wyBool isrecentfiles = wyFalse);

    /// Write the current SQL into a file. 
    /**
	@param hwnd                 : Window HANDLE
	@filename                   : Name of the file
	@param istabclose			: IN flag tells tab is closing or not
	@returns wyTrue on success else wyFalse
    */
	wyBool				WriteSQLToFile (HWND hwnd, wyString &filename, wyBool istabclose = wyFalse);

    /// Function writes the data from the SQL file whose path is passed as parameter into the function.
    /**
    @param hfile     : IN File handle
	@returns wyTrue on success else wyFalse
    */
	wyBool				WriteSQLToEditor (HANDLE hfile, wyBool issametab = wyFalse);
	wyBool				WriteSQLToEditor2 (HANDLE hfile, EditorBase* peditorbase);
    /// Collects all the supported Engines and add to the menu
    /**
    @param hmenu: IN parent menu handle
    @returns void
    */
    void                InsertEnginesMenuItems(HMENU hmenu);


	/// Function to show the FLUSH dialog box. 
    /**
	@returns non zero on success else zero
    */
	wyInt32				ShowFlushDlg();

    /// Executes the flush operation
    /**
    @param query        : query
    @param hwndedit     : Edit window HANDLE
    @param writetobin   : Write to binary log
    @param hwnddlg      : Dialog window HANDLE
	@returns wyTrue on success else wyFalse
    */
	wyBool				ExecuteFlush(wyString query, HWND hwndedit, INT writetobin, HWND hwnddlg);

    /// Enables or disables the option in flush window.
    /**
    @param hwndlist     : List window HANDLE
    @param state        : State(Enable/Disable)
	@returns wyTrue
    */
	wyBool				EnableDisableFlushOptions(HWND hwnd, wyInt32 state);
	
	/// Enables or Disables Flush Button in Flush Dialog
	/**
	@param hwnd			: List window HANDLE
	@returns wyTrue
	*/
	wyBool				EnableDisableFlushButton(HWND hwnd);

    /// Takes cares of the persistence in flush window.
    /**
    @param hwnd      : Window HANDLE
	@returns wyTrue
    */
	wyBool				AddFlushPersistence(HWND hwnd);
	
	/// Function to show the template dialog box and set the text in the edit box.
    /**
	@returns wyTrue on success else wyFalse
    */
	wyBool				ShowTemplateDlg();

    /// Initializes the template dialog box with the template items.
    /**
    @param hwndlist     : List window HANDLE
    @param hwndedit     : Edit window HANDLE
	@returns wyTrue
    */
	wyBool				InitListBox(HWND hwndlist, HWND hwndedit);

	// Initializes the template dialog box with the template items.
    /**
    @param hwnd		    : Dialog window HANDLE
    @returns wyTrue
    */
	wyBool				InitTempDlg(HWND hwnd);

    /// Function shows the template of the item selected from the listbox on every change.
    /**
    @param hwndlist     : List window HANDLE
    @param hwndedit     : Edit window HANDLE
	@returns wyTrue
    */
	wyBool				ShowTmpPreview(HWND hwndlist, HWND hwndedit);

    /// Function get the id from the selected text and close the dialog with returning the itemdata of
    /// the selected item. This itemdata is nothing but the string table id.
    /**
    @param hwndlist     : List window HANDLE
    @param hwnddlg      : Dialog window HANDLE
	@returns wyTrue on success else wyFalse
    */
	wyBool				CloseTmpDlg(HWND hwndlist, HWND hwnddlg);

    /// Frees the list contents
    /**
    @param hwndlist     : Window HANDLE
	@returns wyTrue
    */
	wyBool				FreeList(HWND hwndlist);
	    
    /// Function to set the check of result in grid or text when this window is activated.
    /**
	@returns wyTrue
    */
	wyBool				SetResultTextOrGrid();

    /// Function to set whether the user wants object browser or not.
    /**
	@returns wyTrue
    */
	wyBool				SetObjBrowVis();

    /// Gets the query status.
    /**
	@returns query status object
    */
	StatusBarMgmt	    *GetQueryStatus();

    /// Function to show the template menu when the user use the keyboard to popup the template menu.
    /**
	@returns wyTrue
    */
	wyBool				ShowTemplateMenu();
    
    /// Sets the Tab window title
    /**
	@returns void
    */
	void				SetQueryWindowTitle();

	/// Set titles for Query editor
	/**	
	@param mdititle : IN MDI window title
	*/
	void				TabEditorTitles(wyString *mdititle);
	
	/// Set titles for SchemaDesigner
	/**
	@param mdititle : IN MDI window title
	*/
	void				TabSchemaDesignerTitles(wyString *mdititle);

    /// Function to set sql_mode, it is used bs a hack to STRICT_ALL_TABLES 
    /**
    @param profile  : IN profile or not
	@returns void
    */
	void				SetDefaultSqlMode(Tunnel * tunnel, PMYSQL pmysql, wyBool reconnect, wyBool profile = wyTrue);

    /// It is used to get the character length
    /**
	@returns wyTrue on success else wyFalse
    */
	wyBool				GetMySQLCharsetLength();

    /// Gets the window HANDLE
    /**
	@returns window handler
    */
	HWND				GetHwnd();

    /// Returns the TabModule information.
    /**
	@returns TabModule object pointer
    */
	TabModule *		GetTabModule();   

    /// Returns the query object
    /**
	@returns query window object
    */
	CQueryObject*		GetQueryObject();

    /// Handles the WM_MDIACTIVATE case
    /**
    @param hwnd         : Window HANDLE
    @param lparam       : Long message parameter
    @param wparam       : Unsigned message parameter
	@returns void
    */
    void                OnMDIActivate(HWND hwnd, WPARAM wparam, LPARAM lparam);

    /// Handles the WM_INITDLGVALUES case
    /**
	@returns wyTrue on success else wyFasle
    */
    wyBool              OnInitDialog();

    /// Handles the WM_CREATE case
    /**
    @param hwnd         : Window HANDLE
	@returns void
    */
    void                OnWmCreate(HWND hwnd);

    /// Handles the WM_CLOSE case
    /**
    @param hwnd         : Window HANDLE
	@returns one on success else zero
    */
    wyInt32             OnWmClose(HWND hwnd);

    /// Handles the WM_NOTIFY case
    /**
    @param hwnd         : Window HANDLE
    @param lparam       : Long message parameter
    @param wparam       : Unsigned message parameter
	@returns zero on success else one
    */
    wyInt32             OnWmNotify(HWND hwnd, WPARAM wparam, LPARAM lparam);

    /// Handles the TVN_ENDLABELEDIT case
    /**
    @param lparam       : Long message parameter
	@returns one on success else zero
    */
    wyInt32             OnTvnEndLabelEdit(LPARAM lparam);

    /// Handles the CTCN_SELCHANGING case
    /**
    @param hwnd         : Window HANDLE
    @param tabid        : Table id
    */
    void                OnCtcnSelChanging(HWND hwnd, wyInt32 tabid);

    /// Handles the CTCN_SELCHANGE case
    /**
    @param hwnd         : Window HANDLE
    @param tabid        : Table id
    */
    void                OnCtcnSelChange(HWND hwnd, wyInt32 tabid, LPARAM lparam = NULL);

    /// Handles the CTCN_TABCLOSING case
    /**
    @param hwnd         : Window HANDLE
    @param tabid        : Table id
    */
    wyInt32             OnCtcnTabClosing(HWND hwnd, LPNMCTC lpnmctc);

    /// Handles the CTCN_TABCLOSED case
    /**
    @param hwnd         : Window HANDLE
    @param tabid        : Table id
	@returns void
    */
    void                OnCtcnTabClose(HWND hwnd, LPNMCTC lpnmctc);

	/// Handles the CTCN_LBUTTOWNDOWN case
    /**
    @param hwnd         : Window HANDLE
    @param tabid        : Table id
	@returns void
    */
	void				OnCtnLButtonUp(HWND hwnd, wyInt32 tabid, LPARAM Position);

    /// Handles the WM_MOUSEMOVE case
	/**
	@returns void
	*/
    void                OnMouseMove();

    /// Handles the UM_REFRESHOBJECT case
	/**
	@returns void
	*/
    void                OnRefreshObject();

    /// Handles the UM_FOCUS case
	/**
	@returns void
	*/
    void                OnUMFocus();

    /// Action to perform when ok button in the flush window is pressed
    /**
    @param hwnd         : Window HANDLE
	@returns one on success else zero
    */
    wyInt32             OnFlushOK(HWND hwnd);

    /// Flushes all the items 
    /**
    @param hwnd         : Window Handler
    @param hwndtext     : Text windows HANDLE
    @param writetobin   : Write to binary log
	@returns one on success else zero
    */
    wyInt32             FlushAll(HWND hwnd, HWND hwndtext, wyInt32 writetobin);

    /// Flushes the specific items 
    /**
    @param hwnd         : Window Handler
    @param hwndtext     : Text windows HANDLE
    @param writetobin   : Write to binary log
	@returns one on success else zero
    */
    wyInt32             FlushSpecific(HWND hwnd, HWND hwndtext, wyInt32 writetobin);

    /// Command handler for template
    /**
    @param hwnd     : Window HANDLE
    @param wparam   : Unsigned message parameter
    @param lparam   : Long message parameter
	@returns void
    */
    void                OnTemplateWmCommand(HWND hwnd, WPARAM wparam, LPARAM lparam);
		
    /// Updates the sqlite node
    /**
	@param tunnel	: Tunnel pointer
	@param mysql	: Mysql pointer
	@param keyword	: wyTrue if keywords else wyFalse
	@param buffer	: Stores the keywords
	@returns wyTrue on success else wyFalse
    */
    static wyBool                      GetScintillaKeyWordsAndFunctions(Tunnel *tunnel, MYSQL *mysql, 
                                wyBool bkeyword, wyString &buffer);

	/// Callback function for keywords and functions
	/**
	@param t		: Tag array
	@param argc		: Argument count
	@param argv		: Argument vector
	@param colname	: Column name
	@returns wyFalse
	*/
    static wyInt32              callback_tags(void *t, wyInt32 argc, wyChar **argv, wyChar **colname);

	/// Gets the keywords
	/**
	@param functincount		: Function count
	@returns keywords
	*/
    wyChar **                   GetKeywords(wyInt32 *functioncount);

	/// Callback function 2 for keywords and functions
	/**
	@param t		: Tag array
	@param argc		: Argument count
	@param argv		: Argument vector
	@param colname	: Column name
	@returns wyFalse
	*/
    static wyInt32              callback_tags2(void *t, wyInt32 argc, wyChar **argv, wyChar **colname);

	wyBool                      ReConnect(Tunnel * tunnel, PMYSQL mysql, wyBool isssh, wyBool isimport, wyBool isprofile);
	
	///Reconnect the SSH 
	/**
    @param conninfo	            : IN Connection Info
    returns wyTrue on successful export else wyFalse
	*/
	wyBool						ReConnectSSH(ConnectionInfo *conninfo);

	//wyBool						HandleProfilerShowStatusCost();
	
    /// Starts the export process from the context menu of object browser
    /**
    */
    void                        EnableExportDialog();

    /// Initiates the values and calls the export dialog
    /**
    @param dbname               : IN Current database name
    @param tablename            : IN Current table name
    returns wyTrue on successful export else wyFalse
    */
    void                        StartExportDialog(wyString &dbname, wyString &tablename);

    /// Starts the export view directly
    /**
    */
    void                        ExportViews();

    /// Handles the mysql error messages
    /**
    */
    void                        HandleMysqlError();

    TabTableData*               GetActiveTabTableData();
	///Function to Get the Active TabEditor
	/*
	@returns pointer to TabEditor object
	**
    */
	TabEditor	* GetActiveTabEditor();

	///Function to Get the Active HistoryTab
	/*
	@returns pointer to TabHistory object
	**
    */
	TabHistory	* GetActiveHistoryTab();

	///Function to Get the Active InfoTab
	/*
	@returns pointer to TabObject object
	**
    */
	TabObject	* GetActiveInfoTab();

    /// Function to handle 'Edit' menu items - CTRL+L, CTRL+2, CTRL+3
	/*
	@returns VOID
	***
    */
	VOID		HandleEditMenu();

    /// Stop query execution
    void        StopQueryExecution();

	///CHecking if currenet execution stopped or not
	/**
	@return wyTrue if stopped/not executing anything, or return wyFalse
	*/
	wyBool		IsStopQueryVariableReset();

    /// initiate the process of opening queryxml
    /**
    @param filename             : IN filename to be loaded
    @returns wyTrue on scucess else wyFalse on faillure
    */
	wyBool                      OpenQBFile(wyString *filename, wyBool issametab = wyFalse, wyBool isrecentfiles = wyFalse);

    ///Function to change the MDI and QB tab titles when the query builder is activated
    /**
    @param mdititle             : IN MDI title 
    @returns void
    */
    void                        TabQueryBuilderTitles(wyString *mdititle);

    void                        OnCtcnGetChildWindows(HWND hwnd, LPARAM lparam);

    void                        LoadQueryTabPlusMenu(LPARAM lparam);

    void                        PositionTabs(wyBool isupdtabledata, wyBool isupdhistory, wyBool isupdinfo);

    //Function to handle Timer for keep-alive ping

    void                        HandleKeepAliveTimer();

    static	unsigned __stdcall  KeepAliveThreadProc(LPVOID lpparam);

	void						SetExecuting(wyBool isexecuting);

	void						SetThreadBusy(wyBool isexecuting);

    void                        HandleQueryExecFinished(HWND hwndactive, MDIWindow *pcquerywnd, WPARAM wparam);

	/* Function to handle resize
	@param hwnd         : Window HANDLE
    */
			void SQLTempResize(HWND hwnd);

	/* Function to reposition the controls
	*/		
			void PositionCtrls();

	/* Function to add the dialog controls to a list
	*/		
			
			void GetCtrlRects();

	/* Sets the minimum size of the window
	@param lparam       : Long message parameter
	*/	
			void OnWMSizeInfo(LPARAM lparam);

	/// Querytab object pointer
	CQueryObject	*m_pcqueryobject;

	Announcements	*m_announcements;

	///	TabModule object pointer
	TabModule	*m_pctabmodule;

	///Active 'TabEditor' pointer
	TabEditor * m_pcactivetab;

    /// Status bar object pointer
	StatusBarMgmt	*m_pcquerystatus;

    /// Vertical splitter object pointer
	FrameWindowSplitter	*m_pcqueryvsplitter;

       /// Auto completeinterface object
	AutoCompleteInterface	*m_acinterface;

    /// Flush persistence pointer
	Persist			*m_p_flush;

    /// Handles to auto complete thread
	HANDLE			 m_hevt;

    /// Handle to the auto complete update thread
    HANDLE			 m_hchildevt;

    /// Handles thread
    HANDLE			 m_hthrd;

    //Handle to QueryEvent;
    HANDLE			 m_queryevt;

    /// Handles thread to mysql resource
    HANDLE			 m_hmysqlfreeres;

    /// Handles tool tip thread 
    HANDLE			 m_htooltipthrd;

    /// Sqlite node
	PSQLITENODE		m_psqlite;

    /// Window HANDLE
    HWND				m_hwnd;

    /// Parent window HANDLE
	HWND				m_hwndparent;

    /// Last focus window HANDLE
	HWND				m_lastfocus;

    /// Mysql pointer
    MYSQL               *m_mysql;
    MYSQL               *m_stopmysql;

    /// Tunnel pointer
	Tunnel	            *m_tunnel;

    /// Database name
	wyString			 m_database;

    /// Filter database name
    wyString			 m_filterdb;

    /// Title
    wyString			 m_title;

	 /// Full Title with SSH and HTTP information
    wyString			 m_tunneltitle;

    /// File name
	wyString			 m_filename;

    /// Edit table name
	wyString			 m_edittable;

    /// Open flag
	wyBool				 m_bopenflag;

    /// If object browser visible
    wyBool				 m_isobjbrowvis;

       /// Connection information
	ConnectionInfo		 m_conninfo;

	/// Stop query
	wyInt32             m_stopquery;

	/// Executing flag
	wyBool				m_executing;
	wyBool				m_isthreadbusy;
	wyBool				m_pingexecuting;

	/// Execute options
	EXECUTEOPTION		m_execoption;

	/// Keywords
    wyString            m_keywordstring;

	/// Function name 
    wyString            m_functionstring;
	
	/// Window procedure
	WNDPROC				wpOrigSciProc;

    /// Duplicating thread handle;
    HANDLE              m_hconthread;

    /// Critical-section object. 
    CRITICAL_SECTION    m_cs;

	/// buffer to store history data
	wyChar				*m_historydata;

    /// Last query execution time
    wyInt64             m_lastquerytime;

	///Flag used for Cheking MySQL version whether it is lesser or greater than Ver. 4.1
	 wyBool				m_ismysql41;

        ///flag for Dragging table
	wyBool				m_dragged;

	/// Storing the ImageList for Dragging
	LRESULT			    m_dragimaglist;  

    wyBool              m_isactivated;

	///Execution start time
	wyInt64             m_execstarttime;
	
	///Execution end time
	wyInt64             m_execendtime;

	///Transfer end time
	wyInt64             m_transfertime;
	
	//query execution is successful or not
	wyInt32				m_querysuccessful;

	//Flag tells whether closes all tabs in con. window(to avoid flickering issue)
	wyBool				m_ismdiclosealltabs;

	///mySQL result for PQA SHOW STATUS on before SELCT query execution
	MYSQL_RES			*m_myresstatusfirst;
	MYSQL_RES			*m_myresstatussecond;
	
	//SHOW status afterthe query
	MYSQL_RES			*m_myresstatusforquery;

	//SHOW PROFILER o/p to display 
	MYSQL_RES			*m_myresprofileresultquery;

	///Flag sets wyFalse if qeury searched not found with PROFILER buffer
	wyBool				m_isqueryprofileidfound;

	///Flag terlls whether the server got reconnected or not while euting query(it uses now in PQA)
	wyBool				m_isreconnected;

	//Flag sets wyTrue once PQA used for a query(PQA will be for first SELECT query)
	wyBool				m_ispqacheck;

	//Check whether the query execte from editor is SELECT query or not
	wyBool				m_isselectquery;

    //Check from version 5.6 explain and explain extended is available for INSERT/UPDATE/DELETE and REPLACE.
    wyBool              m_isprofilerrequire;

	//Flag tells whether the mySQL version suppors PROFILING or not
	wyBool				m_isprofilesupported;

	//During PQA, this sets wyTrue if reconnect happen in SHOW SESSION STAUS(before SELECT query)
	wyBool				m_issessionstatusreconnected;
	
	//Flag sets wyTrue if reconnect happend for PROFILING info
	wyBool				m_isshowprofilereconnected;
	
	//Sets wyTrue if Reconnect happens during SHOW STAUS cost measurment
	wyBool				m_isstatuscostmeasurereconnected;

	//Sets wyTrue once the cost measuerd for SHOW STATUS, it does for 1st SELCT query for a connection
	wyBool				m_isstauscostmeasured;

	//query "SHOW VARIABLE like '%profiling%' execute only once to check whether server support this
	wyBool				m_isprofilecheckover;
		
	//Server version , for PQA - status(SESSION or GLOBAL)
	long				m_severversion;

	//Flag telle whether to build rebuild tags for specific db ornot
	wyBool				m_grpprocess;
    
	//Flag for finding when window is Restored(Manual resizing no need to consider here)
	wyBool				m_ismanualresizing;

	//Table engines
	//wyString			m_tableengines;

	//connection number
	wyString			m_currentconn;

    //save to all selection
    wyInt32             m_mdisaveselection;

	//flag to check, whether the connection is closed
	wyBool	m_iswinclosed;

	//event to instantiate the init autocomplete thread
	HANDLE				m_acinitevent;

	//Thread handle
	HANDLE				m_threadidinit;

	//Event for tooltip handle
	HANDLE				m_actooltipevent;

	//Event for tooltip handle thread
	HANDLE				m_actooltipwaitevent;

	/// List to store the dialogbox component values
	List				m_controllist;

	/// To store the original size of the dialog box
	RECT				m_dlgrect;

	/// To store the co-ordinates of the dialog box wrt the screen
	RECT				m_wndrect;

	/// Handle for the Template dialog
	HWND				m_hwndtemp;


	HWND				m_historyedit;

    wyUInt32            m_keepaliveinterval;

    HANDLE              m_hthrdkeepalive;

    HANDLE              m_hevtkeepalive;

	HANDLE				m_hevtexecution;

	HANDLE				m_hevthist;

	wyInt32				m_ret;

	wyString			m_statusbartext;

	wyBool				m_postactivatemsg;

	wyInt32				m_focussedcon;

	wyBool				m_isanncreate;

	//wyInt32				m_tabposition;

	List				*m_listtabeditor;
	List				*m_listtabdetails;
#ifndef COMMUNITY
	///The PQA SHAOW STATUS advice array keep in Connection level
	paramadvice			*m_constatusparm;
#endif
};

#endif
