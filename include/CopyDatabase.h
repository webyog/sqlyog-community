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

#include "FrameWindowHelper.h"
#include "DialogPersistence.h"
#include "DoubleBuffer.h"

#ifndef _CCOPYDB_
#define _CCOPYDB_
class Tunnel;
class CopyDatabase;

/*! \struct __copydb_longparam
    \brief Used to store copydb information
    \param HWND hwndmsg MessageBox handle
    \param HWND hwnddlg Dialog handle
*/
struct __copydb_longparam
{
	HWND		m_hwndmsg;
	HWND		m_hwnddlg;
	wyString	*m_summary;
};

/*! Creates a type copydb information */ 
typedef struct	__copydb_longparam COPYDBPARAM;

/*! \struct __stcopydb
    \brief Structure for thread sync in copydb. 
    This structure pointer is sent as a long parameter in the thread for importbatch
    \param HANDLE impevent Handle to the import event
    \param ImportFromSQL *import ImportFromSQL object pointer
	\param wyUInt32 num_bytes Number of total bytes
	\param wyUInt32 num_queries Number of queries
    \param wyBool *stopquery Stop flag
    \param IMPORTERROR retcode IMPORTERROR enum value
    \param IMPORTPARAM *lpParam __import_longparam pointer
*/
struct __stcopydb
{
	HANDLE          m_copydbevent;
    CopyDatabase    *m_copydb;
	wyInt32			*m_copystop;
	COPYDBPARAM     *m_lpParam;
};

/*! Creates a type __stcopydb */ 
typedef struct	__stcopydb COPYDB;

typedef enum __copy_state
{
	TABLECOPIED,
	CREATETABLE,
	ROWSCOPIED,
	VIEWCOPIED,
	PROCEDURECOPIED,
	FUNCTIONCOPIED,	
	TRIGGERCOPIED,
	EVENTCOPIED,
	COPYSTART,
	FETCHCOPYDATA,
	OBJECTSCOPIED
}COPYSTATE;

typedef void (*gui_copydb_update_routin)(void * lpParam, wyChar *tablename, 
										 wyUInt32 rowsinserted, wyBool isdata, COPYSTATE state);
typedef gui_copydb_update_routin LPGUI_COPYDB_UPDATE_ROUTINE;

class CopyDatabase
{
	
public:

    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
	CopyDatabase();

     /// Destructor
    /**
    Free up all the allocated resources
    */
	~CopyDatabase();

    /// function creates the actual dialog box and also initializes some values of the class
    /**
    @param hwndparent   : IN parent window handle
    @param tunnel       : IN Tunnel pointer
    @param mysql        : IN mysql pointer
    @param db           : IN database name to copy
    @param table        : IN table name to copy
    @param checktables  : IN flag to check the only the tables. this parameter is valid only if the table parameter is set to NULL
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool		Create(HWND hwndparent, Tunnel * tunnel, PMYSQL mysql, wyChar *db, wyChar *table = NULL, wyBool checktables = wyFalse);

    /// Checks whether the copy is in progress or not
    /**
    @returns wyBool, wyTrue if copy is stopped, else wyFalse
    */
    wyBool      IsCopyStopped();

    /// handles the mouse move while the copy is on progress.
    /**
    @param hwnd: IN window handle
    @param lparam: IN window proc lparam.
    @returns void
    */
    void        OnMouseMove(HWND hwnd, LPARAM lparam);

    /// Stops copy process;
    /**
    @returns void
    */
    void        StopCopy();

	wyBool		CreateTargetDB();

	static	wyInt32 CALLBACK	TreeWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    /// handles image list
	HIMAGELIST		m_himl;    
	
	
	///Window procedure pointer
	WNDPROC			m_wpsrcorigproc;

        /// Handle to the main dialog window
	HWND			m_hwnddlg;

    /// handle to the combo box
    HWND            m_hwndcombo;
	/// handle to the db combo box
	HWND            m_hwndcombodb;
    
    /// source tunnel pointer
	Tunnel			*m_srctunnel;
    
    /// target tunnel pointer
    Tunnel          *m_targettunnel;
    
    /// new target tunnel pointer
    Tunnel          *m_newtargettunnel;
    
    /// new src tunnel pointer
    Tunnel          *m_newsrctunnel;
    
    /// Target connection info structure
	ConnectionInfo	*m_tgtinfo;
    
    /// Source connection info structure
	ConnectionInfo	*m_srcinfo;
	PIPEHANDLES		m_srcsshpipehandles;
    
    /// SSH process handle
	HANDLE			m_tgtprocess;
	PIPEHANDLES		m_tgtsshpipehandles;
    
    /// SSH process handle
	HANDLE			m_srcprocess;
    
    /// source mysql pointer
	PMYSQL			m_srcmysql;
    
    /// Target mysql  pointer
    PMYSQL          m_targetmysql;
    
    /// New target mysql pointer
	MYSQL			*m_newtargetmysql;
    
    /// Target mysql  pointer
    MYSQL          *m_newsrcmysql;
    
    /// source dbname
	wyString        m_srcdb;
    
    /// Target dbname
    wyString        m_targetdb;
    
    /// Selected table name
	wyString        m_table;
    
    /// whether to drop 5x objects if it is existing
	wyBool          m_dropobjects;
    
    /// Stop flag
    wyBool          m_stop;
    
    /// persistence object
	Persist			*m_p;
    
    /// Handle to the copy thread
    HANDLE          m_hcopythread;
    
    /// Hadle to copy event
    HANDLE          m_copyevent;
    
    /// whether to export data also
    wyBool          m_exportdata;    
   
	/// list for views
	List           m_selviews;

	/// list for tables
	List          m_seltables;

	
    /// Export View?
    wyBool          m_isview;
    /// Export Procedure?
    wyBool          m_isprocedure;
    /// Export Function?
    wyBool          m_isfunction;
    /// Export Trigger?
    wyBool          m_istrigger;
    //Copydb parameters
    void            *m_copydbparam;

    /// GUI call back routine.
	LPGUI_COPYDB_UPDATE_ROUTINE m_gui_routine;
    void            *m_gui_lparam;
    /// Copy status flag
    wyBool          m_copying;
    /// Stop copying Flag
    wyBool           m_stopcopying;
    /// Enable 5x objects flag
    wyBool          m_is5xobjects;
	wyBool          m_issrc5xobjects;
    wyBool          m_istrg5xobjects;
	wyBool          m_issrceventsupport;
	wyBool          m_istrgeventsupports;

	//flag sets wyTrue if atleast one object is there to be exported
	wyBool			m_isobjecttoexport;
    
    /// CRITICAL_SECTION
    CRITICAL_SECTION    m_cs;

	wyString		m_summary;

	//Flag tells is there any stored program or view to be copied
	wyBool			m_isstoredpgrms;

	//Flag tells whether to promt message for user if stored pgrms or view copied if the copy process was successful
	wyBool			m_ispromtstorepgrmmessage;

    //flag tells whether to select only tables
    wyBool          m_selalltables;

	wyBool          m_isremdefiner;
	/// List to store the dialogbox component values
	
	List				m_controllist;

	/// To store the original size of the dialog box

	RECT				m_dlgrect;

	/// To store the co-ordinates of the dialog box wrt the screen
	
	RECT				m_wndrect;

	//Flag tells not to handle TVN_ITEMEXPANDING notification from TreeView_SelectSetFirstVisible macro
	
	wyBool				m_dontnotify;

	wyBool				m_iscreatedb;

private:

	//max allowed packet size
	wyUInt32        m_maxallowedsize;
	wyBool			m_bulkinsert;

	//max bulk insert stmt length(max is 16MB)
	wyUInt32			m_bulklimit;

      /// Common window procedure for the copydatabase window
    /**
    @param      hwnd        : IN Handle to the dialog box. 
    @param      message     : IN Specifies the message. 
    @param      wParam      : IN Specifies additional message-specific information. 
    @param      lParam      : IN Specifies additional message-specific information. 

    @returns    1 or 0
    */
    static INT_PTR CALLBACK    WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    /// Thread procedure for copy database to different host
    static	unsigned __stdcall  ExecuteCopyThread(LPVOID lpparam);

    /// Procedure that is used to update the GUI messages while copying.
    /** This is used to show the messages like how many rows copyed
    @param lpparam		: IN IMPORTPARAM pointer.
    @param rowsinserted	: IN Number of rows inserted
    @param isdata	: IN is data copy or structure copy 
    @returns void
    */
	static void UpdateGui(void * lpparam, wyChar *tablename, wyUInt32 rowsinserted, wyBool isdata, COPYSTATE state);

    /// Initializes the copy database window
    /**
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool		AddInitData();

	/// Fills the Tree with table(s), view(s), trigger(s), function(s) and stored procedure(s)
    /**
    @param hwnd		:IN Handle to the dialog
	@param dbname	:IN Holdes the Database name 
	@param issingledb : IN sets wyTRue if single connection single db
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool    InitializeTree( HWND hwnd, const wyChar *dbname, wyBool issingledb = wyFalse);

	/// insert tables, views, procedures, functions, triggers, events into copy db tree 
	/**
	@param HWND       : IN hwndtree is tree handle
	@param wyWChar    : IN database object name
	@param wyString   : query for getting database objects
	@param wyInt32    : imagetype
	@param wybool     : copy table or databse
	@returns wybool.
	*/
	wyBool      InsertDatabaseObjects( HWND hwndtree, wyWChar *object , wyString &query, wyInt32 imagetype, wyBool iscopytable = wyFalse);

	/// insert tables when copying table only
	/**
	@param HWND       : IN hwndtree is tree handle
	@param wyString   : query for getting tables
	@returns wybool.
	*/
	wyBool      InsertTables(HWND hwndtree, wyString &query);

    /// Frees the lparam stored in combo box
    /**
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool		FreeComboParam();
	wyBool		FreedbComboParam();

    /// HAndles the UM_COPYDATABASE message
    /**
    @param hwnd : IN handle to copy database dialog
    @returns void
    */
    void        OnUmCopydatabase(HWND hwnd);

    /// helps to remove the connection instances created tfor copying
    /**
    returns void
    */
    void        RemoveInstances();
    void        RemoveSourceInstances();
    void        RemoveTargetInstances();
    
    /// HAndles the wm_command message
    /**
    @param hwnd     : IN Handle to window
    @param wparam   : IN The high-order word specifies the notification code if the message is from a control
    @returns void
    */
    void        OnWmCommand(HWND hwnd, WPARAM wparam);

    /// Initiates the copy process
    /**
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool      ExportData(LPGUI_COPYDB_UPDATE_ROUTINE gui_routine, void * lpParam);

    /// Initilizes the copy process variables
    /**
	@param hwnd		:IN Handle to the tree.
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
    wyBool      InitExportData(HWND hwndtree);

	/// get htreeitem of (tables or views or procedures, functions, triggers, events ) node
	/**
	@param hwnd		:IN Handle to the tree
	@param object   :database object name
	@returns HTREEITEM
	*/
	HTREEITEM   GetObjectsHtreeItem( HWND hwnd, wyWChar *object, wyBool isfindroot = wyFalse);
	
	/// This function is uesd to intilize tree with table(s)
	/**
	@param hwndtree		:IN Handle to the tree
	@param hrootitem	:IN Handle  to the tree root item type
	@returns void
	*/
	void		HandleTables(HWND hwndtree, HTREEITEM hrootitem);
    	
	/// This function is uesd to intilize tree with table(s) if parent is expanded
	/**
	@param hwndtree		:IN Handle to the tree
	@param hrootitem	:IN Handle  to the tree root item type
	@returns wyTrue on success else return wyFalse
	*/
	wyBool		HandleTablesOnParentExpanded(HWND hwndtree, HTREEITEM htreeitem);

	/// This function is uesd to intilize tree with table(s) if parent is not expanded
	/**
	@param hwndtree		:IN Handle to the tree
	@returns wyTrue on success else return wyFalse
	*/
	wyBool		HandleTablesOnParentNotExpanded(HWND hwndtree);

	/// This function is uesd to intilize tree with views(s)
	/**
	@param hwndtree		:IN Handle to the tree
	@param hrootitem	:IN Handle  to the tree root item type
	@returns wybool
	*/
	wyBool		HandleViews(HWND hwndtree, HTREEITEM hrootitem);

	/// This function is uesd to intilize tree with Procedure(s)or Function
	/**
	@param hwndtree		:IN Handle to the tree
	@param hrootitem	:IN Handle  to the tree root item type
	@param ISproc		:IN specifies function or procedure, wyTrue means procedure
	@returns wybool
	*/
	wyBool		HandleProcedureOrFunction(HWND hwndtree, HTREEITEM hrootitem, wyBool Isproc);

	///Handle if coby without expanding function/procedure
	/**
	@return wyTrue on succcess else return wyFalse
	*/
	wyBool		HandleProcedureFunOnParentNotExpanded(HWND hwndtree, wyBool isproc);

	///Handle if coby function/procedure with expanded
	/**
	@return wyTrue on succcess else return wyFalse
	*/
	wyBool		HandleProcedureFunOnParentExpanded(HWND hwndtree, HTREEITEM htreeitem, wyBool isproc);
    
	/// Copies all the views
    /**
	@param hwndtree		:IN Handle to the tree
	@param hrootitem	:IN Handle  to the tree root item type
    @returns wybool
    */
	wyBool      ExportViews(HWND hwndtree, HTREEITEM hrootitem);

	/// Copies all the views if the parent is expanded
    /**
	@param hwndtree		:IN Handle to the tree
	@param hrootitem	:IN Handle  to the tree root item type
    @returns wybool
    */
	wyBool      ExportViewsOnParentExpanded(HWND hwndtree, HTREEITEM hrootitem);
	
	/// Copies all the views if the parent is not expanded
    /**
	@param hwndtree		:IN Handle to the tree
	@returns wybool
    */
	wyBool      ExportViewsOnParentNotExpanded(HWND hwndtree);

	/// Copies all the events
    /**
	@param hwndtree		:IN Handle to the tree
	@param hrootitem	:IN Handle  to the tree root item type
    @returns wybool
    */
	wyBool       ExportEvents(HWND hwndtree, HTREEITEM hrootitem);

	/// Copies all the events if parent is expanded
    /**
	@param hwndtree		:IN Handle to the tree
	@param hrootitem	:IN Handle  to the tree root item type
    @returns wybool
    */
	wyBool      ExportEventsOnParentExpanded(HWND hwndtree, HTREEITEM hrootitem);
	
	/// Copies all the events if parent is not expanded
    /**
	@param hwndtree		:IN Handle to the tree
	@returns wybool
    */
	wyBool      ExportEventsOnParentNotExpanded(HWND hwndtree);

	/// Copies all the triggers
	/**
	@param hwndtree		:IN Handle to the tree
	@param hrootitem	:IN Handle  to the tree root item type
    @returns wybool
    */
	wyBool		ExportTriggers(HWND hwndtree, HTREEITEM hrootitem);

	/// Copies all the triggers when parent is expanded
	/**
	@param hwndtree		:IN Handle to the tree
	@param hrootitem	:IN Handle  to the tree root item type
    @returns wybool
    */
	wyBool      ExportTriggersOnParentExpanded(HWND hwndtree, HTREEITEM hrootitem);
	
	/// Copies all the triggers if parent is not expanded
	/**
	@param hwndtree		:IN Handle to the tree
	@returns wybool
    */
	wyBool      ExportTriggersOnParentNotExpanded(HWND hwndtree);

    /// frees the copy process variables
    /**
    @returns void
    */
    void        FreeExportData();

    /// Changes the source connection context to the database to copy
    /**
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool      ChangeSourceDB();

    /// Changes the target connection context to the database where to be copied
    /**
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool      ChangeTargetDB();

    /// collects the target database instance
    /**
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool      GetTargetDatabase();

    /// drops a particular table
    /**
    @param table: IN table to be dropped
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool      DropTable(wyChar *table);

    /// copies the content of a table
    /**
    @param table: IN table to be copied
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool      ExportActualData(wyChar *table);
	/// adding field values of a row to the query 
	/**
	@param myres            : Mysql result pointer
	@param row		        : IN Mysql row pointer
	@param insertstmt       : OUT Insert Query
	@param nfield           : IN no of field of a row
	@param ismysql41        : IN mysql version is greter than 4.1 or not( wybool)
	@returns void.

	*/
	void		GetInsertQuery(MYSQL_RES *myres, MYSQL_ROW myrow, wyString &insertstmt, wyInt32 nfilelds, 
								wyBool ismysql41);
	/// Execute Bulk query or Single query
	/**
	@param myres            : Mysql result pointer
	@param row		        : IN Mysql row pointer
	@param table            : IN table name
	@param exportedrowcount : OUT no of rows copyied
	@returns wytrue or wyfalse
	*/
	wyBool		ExecuteInsertQuery(MYSQL_RES *myres, wyString  &insertstmt, wyChar* table, wyInt32  &exportedrowcount);

	///Wrappert to execute insert queries, its added when omplemented Set FK-CHECK = 0, with http.
	///For http, with each insert, also issue SET FK-CHECK =0.
	/**
	@param wnd		  : IN MDIWindow handler	
	@param myres      : Mysql result pointer
	@param insertstmt : IN insert statemebt to be executed
	@param batch      : IN true for batch
	@param force	  : IN true to force the queries to execute
	@return wyTrue on success else return wyFalse
	*/
	wyBool		HandleExecuteInsertQuery(MDIWindow *wnd, MYSQL_RES *myres, wyString &insertstmt, bool batch, bool force);

	/// check query length (no of bytes) if it exceed maximum packet lenth then execute query 
	/// after that use another insert stmt
	/**
	@param myres             : Mysql result pointer
	@param fieldcoun         : IN no of field of a row
	@param querylength       : OUT query length
	@returns wytrue or wyfalse
	*/
	wyBool      CheckQueryLimit(MYSQL_RES *myres, wyInt32 fieldcount, wyInt32 &querylength);

    /// Creates the table in the target(MySQL server version >= 4.1)
    /**
    @param table: IN table to be created
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool      CreateTableInTarget(wyChar *table);

    /// Creates the table in the target(MySQL server version < 4.1)
    /**
    @param table: IN table to be created
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool      CreateTableInTarget2(wyChar *table);

	///Wrapper executes CREATE TABLE stmt, its added when SET-FK CHECK =0, with http
	/**
	@param query : IN Query to be executed
	@param batch : IN set true for batch process
	@param force : IN force to execute if its true
	@return wyTrue on success else return wyFalse
	*/
	wyBool		HandleExecuteQuery(wyString &query, bool batch, bool force);

    /// Copies the 5x objects
    /**
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool       Export5XObjects();

    /// Copies all the Stored procedures
    /**
    @param isprocedure: IN is it procedure or function
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool      ExportSPs(wyBool isprocedure);

    /// Copies a particular  Stored procedures
    /**
    @param db: IN dbname
    @param obj: IN object name
    @param isprocedure: IN is it procedure or function
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool      ExportSP(const wyChar *db, const wyChar *obj, wyBool isprocedure);   

    /// Copies a particular view
    /**
    @param db: IN dbanme
    @param view: IN view name to copy
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool      ExportView(const wyChar *db, const wyChar *view);

    /// Copies all triggers
    /**
    @param db: IN dbname
    @param table: IN table name
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool      ExportTrigger(const wyChar *db, const wyChar *triggername);

	/// Copies all events
    /**
    @param db: IN dbname
    @param table: IN event name
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool      ExportEvent(const wyChar *db, const wyChar *event);

    /// Checks for the target db whether it is on 5x server or not
    /** if it is in 5x server we will enable all the options otherwise we will disable some
    @return void
    */
	wyBool        CheckTargetDB(HWND hwnd);

    /// Creates the temporary tables in the database
    /** Create a dummy table for the view. i.e.. a table  which has the
    same columns as the view should have. This table is dropped
    just before the view is created. The table is used to handle the
    case where a view references another view, which hasn't yet been
    created(during the load of the dump). BUG#10927
    @param db: IN dbname
    @param view: IN view name
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool      CreateTemporaryTables(const wyChar *db, const wyChar *view);

    /// Disables all the options
    /**
    @param hwnd:IN copy database window handle
    */
	void        DisableAll(HWND hwnd);	

    /// initializes the target server to start copy
    /**
    @returns void
    */
    void        InitTargetServer();

    /// revert the target server while finishing copy
    /**
    @returns void
    */
    void        RevertTargetServer();

    /// initializes the charset on target server to start copy
    /**
    @returns void
    */
    void        SetNamesToUTF8();

    /// restores the charset on the target server to while finishing copy
    /**
    @returns void
    */
    void        SetNamesToOriginal();

	/// Implements on item expanding message.
    /**
	@param hwnd      : IN treeview handle
    @param pnmtv     : IN Tree items containing notification message
    */
	wyBool		OnItemExpandingHelper(HWND hwnd , LPNMTREEVIEW pnmtv);

	///Handles when changes the target db combo item
	/**
	@param issinglesrcdb : IN sets wyTrue if con. has only one db
	@param cbselchange   : IN sets wyTrue if combobox item is changed
	@return wyTrue on succcess else return wyFalse
	*/
	wyBool		HandleTreeviewOnComboChange(wyBool issinglesrcdb = wyFalse, wyBool cbselchange = wyFalse);

	///Selectes the treeview parent nodes by default
	/**
	@param hwndtree   : IN handle to the treeview
    @param item       : IN the item to be checked
	@return wyTrue on success else return wyFalse
	*/
	wyBool		SelectTreeviewParentItems(HWND hwndtree, HTREEITEM item = NULL);
    
    /// Enables or disables the Dialog Controls
    /**
    @param enable: IN enable/Disable?
    @returns void
    */
   void        EnableDlgWindows(wyBool enable);

   ///This function add the nodes to the tree like table(s),view(s),triggers(s),procedure(s)and function(s)
	/**
	@param  htree	:IN Handle to the tree
	@param  hparent	:IN Parent Handle  to the tree item type
	@param  text	:IN text to be displayed on the node
	@param  icontype:IN type of the icon to be displayed next to the node for example folder icon
	@returns Handle  to the tree item type
	*/
	HTREEITEM addEntry (HWND htree, HTREEITEM parent, wyWChar *text, wyInt32 icontype);

	//This function is used to create image list for the tree
	/**
	@returns wybool i.e wyTrue if sucess
	*/
	wyBool	CreateImageList();	

	// this function used for releasing memory for linked list used by tables, views
	/*
	return void.
	*/
	void    Delete(List *list);
	void    OnCBSelChange(HWND hwnd);
	void    Remove5XRoutines(HTREEITEM htreeitem, HWND hwndtree);
	wyBool						GetCharsetAndCollation(wyString *charset, wyString *collation);
	
	// To disable bulkinsert when only structure is checked. 
	/**
	@param id : IN control id
	@return void
	*/
	void	EnableDisable(wyInt32 id);
	
	//Total number of rows copied
	wyInt32			m_copiedcount;

	/* Function to handle resize
	@param hwnd         : Window HANDLE
    */
			void CopyDbResize(HWND hwnd);

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

	///Function thats draws the window contents using double buffering
        /**
        @param hwnd             : IN window handle
        @returns void
        */
			void OnPaint(HWND hwnd);


};
#endif
