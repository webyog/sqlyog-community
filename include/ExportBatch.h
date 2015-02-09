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


#include "Tunnel.h"
#include "DialogPersistence.h"
#include "ExportAsSQL.h"
#include "DoubleBuffer.h"

#ifndef __EXPORTBATCH__
#define __EXPORTBATCH__


/*! \struct __export_longparam
    \brief 
	\param HWND		m_hwndmsg;				// Handle to static control that shows the message
	\param HWND		m_hwndprogress;			// Handle to the progress bar control that shows the export status 

*/
struct __export_longparam
{
	HWND		m_hwndmsg;
	HWND		m_hwndprogress;
};
typedef __export_longparam EXPORTPARAM;


/*! \struct __stexportbatch
    \brief structure for thread sync in ImportFromSQL. this structure pointer is sent as
			a long parameter in the thread for ImportFromSQL 
	\param HANDLE			 m_expevent;		// Export thread handle
	\param MySQLDump		*m_export;			// Mysqldump class pointer
	\param wyInt32			*m_stopquery;		// Parameter used to stop the execution of the query
	\param EXPORTERROR		 m_retcode;			// Error code returned
	\param EXPORTPARAM		*m_lpparam;			// Pointer to struct __export_longparam

*/
struct __stexportbatch
{
	HANDLE       m_expevent;
	MySQLDump   *m_export;
	wyInt32      *m_stopquery;
	EXPORTERROR  m_retcode;
	EXPORTPARAM *m_lpparam;

};
typedef __stexportbatch EXPORTBATCH;



class ExportBatch
{
public:
	
	/// Constructor
	ExportBatch();

	/// Destructor
	~ExportBatch();


	/// Thread and related function for exporting in batch file 
	/**
	@param lpparam		: 
	@param table		: Table name
	@param state		: 
	@returns void
	*/
	static  void                GuiUpdate(void * lpparam, const wyChar * table, wyUInt32 rows, EXPORTTABLESTATE state);		

	///
	/**
	@param lpparam		:
	@return 0
	*/
	static	unsigned __stdcall  ExportThread(LPVOID lpparam);
	
	/// Callback window procedure
    /**
    @param hwnd			: IN Window HANDLE
    @param message		: IN Window message
    @param wparam		: IN Unsigned message parameter
    @param lparam		: IN Long message parameter
	@returns long pointer
    */
	static	INT_PTR CALLBACK    ExpDataDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	
	///// Callback window procedure
 //   /**
 //   @param hwnd			: IN Window HANDLE
 //   @param message		: IN Window message
 //   @param wparam		: IN Unsigned message parameter
 //   @param lparam		: IN Long message parameter
	//@returns long pointer
 //   */
	//static	LRESULT	CALLBACK    SrcTableListViewWndProc(HWND pHWND, UINT pMessage, WPARAM pWPARAM, LPARAM pLPARAM);

	///// Callback window procedure
 //   /**
 //   @param hwnd			: IN Window HANDLE
 //   @param message		: IN Window message
 //   @param wparam		: IN Unsigned message parameter
 //   @param lparam		: IN Long message parameter
	//@returns long pointer
 //   */
	//static	LRESULT	CALLBACK    TgtTableListViewWndProc(HWND pHWND, UINT pMessage, WPARAM pWPARAM, LPARAM pLPARAM);

	/// Initialize the dialog window with the required data.
	/**
    @param hwnd			: IN Window HANDLE
    @param lparam		: IN Long message parameter
	@returns void
    */
    void    OnWmInitDialog(HWND hwnd, LPARAM lparam);

	/// Initializes dialog for Export batch
	/**
    @param hwnd			: IN Window HANDLE
    @param lparam		: IN Long message parameter
	@returns void
    */
    void    OnWmInitExportData(HWND hwnd);


	/// Handles window notify message 
	/**
    @param hwnd			: IN Window HANDLE
    @param lparam		: IN Long message parameter
	@returns void
    */
    void    OnWmNotify(HWND hwnd, LPARAM lparam);

	/// Handles the mouse movement over the window
	/**
    @param hwnd			: IN Window HANDLE
    @param lparam		: IN Long message parameter
	@returns void
    */
    void    OnMouseMove(HWND hwnd, LPARAM lparam);

	/// Command handler for the export batch dialog
	/**
    @param hwnd			: IN Window HANDLE
    @param lparam		: IN Long message parameter
	@returns void
    */
    void    OnWmCommand(HWND hwnd, WPARAM wparam);

	///// Inserts item to the list control
	///**
 //   @param hwndlist		: IN Window HANDLE
 //   @param width		: IN Long message parameter
	//@param caption		: 
	//@returns void
 //   */
 //   void    InsertListColumn(HWND hwndlist, wyInt32 width, wyWChar *caption);

    void    HandleDlgPersistance(HWND hwnd);

	/// The function to create the export dialog box.
	/**
    @param tunnel		: IN tunnel pointer
    @param mysql		: IN Pointer to mysql pointer
	@param auth			: IN Tunnel authentication
    @param db			: IN Database name
	@param table		: IN Table name; set this parameter to empty string for selecting all object, or set it to NULL to select only tables
    @returns wyTrue on success else wyFalse
    */
    wyBool  Create(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, TUNNELAUTH * auth, const wyChar *db, const wyChar *table);

	///// Function to set the initial dialog box properties.
	///**
	//@returns void
	//*/
	//void    SetDlgProps();

	/// Gets the export filename from the user and stores it in the filename textbox.
	/**
	@returns void
	*/
	void    SetExpFileName(HWND hwnd);
	//this function verifies that given path in select file is directory
	void    IsDirectory(HWND hwnd);

	///This function is used to select the database name from the combo box
	/**
	@param hwnd		:IN Handle to the combo box
	@returns void
	*/
    void	SelectDatabaseName(HWND	hwnd);

	/// Fills the Tree with table(s), view(s), trigger(s), function(s) and stored procedure(s)
    /**
    @param hwnd		:IN Handle to the dialog
	@param dbname	:IN Holdes the Database name 
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool    IntializeTree( HWND hwnd, const wyChar *dbname, wyBool cbselchange = wyFalse);

	//This function is used to create image list for the tree
	/**
	@param  hwnd	:IN Handle to the tree
	@returns wybool i.e wyTrue if sucess
	*/
	wyBool	CreateImageList(HWND hwnd);
	/// This function initializes the dialog box with the database available.
	/**
	@returns wyTrue 
	*/
	wyBool  InitializeExpData();

	//Enable disable options
	/**
	@param id : IN control id
	@return void
	*/
	void	EnableDisableOptions(wyInt32 id);
	
	/// Function checks whether exporting is going or not and performs job as required.
	/// from 4.1, exporting is done in another thread so we have more issues to take care of
	/**
	@returns wyTrue 
	*/
	wyBool  ExportData();
		
	// This is main function which exports data.
	// This function itself calls three functions to write initial comments, add table structure and table data.
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool		ExportData2();

	/// Function checks whether the user has entered correct values or not 
	/**
	@returns wyTrue on success else wyFalse
	*/
	wyBool		ValidateInput();

	/**
	@param istable     : IN if any table is selected its wyTrue
	@param isnontable  : IN if any object other table is selected its wyTrue
	@param checkcount  : IN count of parenet folders that are selected
	@return wyTrue if success else return wyFalse
	*/
	wyBool		CheckForDumpOptions(wyBool istable, wyBool isnontable, wyInt32 checkcount);

	/// Sets the initial values for the export dialog
	/**
	@param dump			: IN/OUT MySQLDump pointer
	@returns wyTrue
	*/
	wyBool		SetInitExportValues(MySQLDump * dump);

	/// Set the table names that need to be exported 
	/**
	@param dump			: IN/OUT MySQLDump pointer
	@returns wybool
	*/
	 void        SetOtherValues(MySQLDump *dump);

	 /// get htreeitem of (tables or views or procedures, functions, triggers, events ) node
	/**
	@param hwnd		    :IN Handle to the tree
	@param object       :database object name
	@param checkstate   :tree element check state
	@returns HTREEITEM
	*/
	HTREEITEM   GetObjectsHtreeItem( HWND hwnd, wyWChar *object, wyInt32 &checkstate);
	

	/// save (selected)tables, views, procedures, funs, triggers into corresponding linked list
	/**
	@param HWND         :IN tree handle
	@param HTREEITEM    : IN htreeitem of tree element
	@param dump			: IN/OUT MySQLDump pointer
	@returns wybool
	*/
	wyBool     GetSelectedObjects(HWND hwndtree, HTREEITEM hrootitem, MySQLDump * dump, wyInt32 objecttype);
	
   
	/// Sets the different parameter values
	/**
	@param param		: OUT Export parameters
	@returns wyTrue
	*/
	wyBool		SetParamValues(EXPORTPARAM * param);

	/// Initializes the progress values .
	/**
	*/
	wyInt32     SetInitProgressValues();

	/// Updates the size and progressbar with the correct values 
	/**
	@param param		: OUT Export parameters
	@returns wyTrue
	*/
	wyBool		UpdateFinishValues(EXPORTPARAM * param);

	/// Enables the dialog box
	/**
	@param state		: IN Enabled/Diabled
	@returns wyTrue
	*/
	wyBool		EnableDlgWindows(bool state);

	/// Change text of the ok button from OK to something else
	/**
	@param text			: IN Text to set
	@returns void
	*/
	void		ChangeOKButtonText(LPCWSTR text);
	///// Enables or disables mysql version 5.x options
	///**
	//@param hwnd			: IN Window handler
	//@param val			: Enable or disable
	//@returns void
	//*/
	//void		EnableDisable5XOptions(HWND hwnd, wyBool val);

	///// Enables or disables the trigger export ( available in mysql version 5.x only )
	///**
	//@param hwnd			: IN Window handler
	//@param val			: Enable or disable
	//@returns void
	//*/
	//void		EnableDisableTriggers(HWND hwnd, wyBool val);

        /// Initialize the progressbar details
    /**
    @param count        : IN progress bar range
    @param step         : IN progress bar step value
    @returns void
    */
    static void InitStepValues(HWND hwnd, wyInt32 count, wyInt32 step);

	/// Implements on item expanding message.
    /**
	@param hwnd      : IN treeview handle
    @param pnmtv     : IN Tree items containing notification message
    */
	wyBool		OnItemExpandingHelper(HWND hwnd , LPNMTREEVIEW pnmtv);
    
    /// stops exporting
    /**
    @returns void
    */
    void        StopExporting();
	
    /// Exporting status
    /**
    @returns exporting status
    */
    wyBool      IsExporting();

		/* Function to handle resize
	@param hwnd         : Window HANDLE
    */
			void ExpBatResize(HWND hwnd);

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


			void OnPaint(HWND hwnd);


	/// Parent window HANDLE
	HWND	    m_hwndparent;

	/// Window HANDLE
    HWND        m_hwnd;
	
	/// Export flag
	wyBool		m_expflg;

	/// Export change
	wyBool		m_expchange;

	/// HANDLE to database name dialog
    HWND        m_hwnddbname;

	/// HANDLE to table name dialog
    HWND        m_hwndtname;

	/// HANDLE to select table name dialog
    HWND        m_hwndseltname;

	/// Editor window HANDLE
    HWND        m_hwndedit;

	/// Database name
	wyString    m_db; 

	/// Table name
    wyString    m_table;

	/// Pointer to mysql pointer
	PMYSQL	    m_mysql;

	/// Tunnel pointer
	Tunnel	    *m_tunnel;

	/// Tunnel authentication pointer
	TUNNELAUTH  *m_auth;
	
	/// Flag , raised when the program is exporting
	wyInt32     m_exporting;

	/// Flag , raised when the user class cancels the exporting
	wyInt32     m_stopexport;
	// dump table only 
	wyBool     m_dump_table;

	wyInt32    m_dumptablecnt;

	/// All table  ?
	wyBool      m_alltables;
	/// all views are slected or not 
	wyBool      m_allviews;
	
	/// all functions are selected or not 
	wyBool      m_allfunctions;
	
	/// all procedures are selected or not 
	wyBool      m_allprocedures;
	
	/// Persistence class object pointer
	Persist	    *m_p;

	/// Window procedure 
	WNDPROC     m_wpsourceorigproc;

	/// Window procedure 
    WNDPROC     m_wptargetorigproc;
	
    //flag that tells to dump only tables
    wyBool      m_dump_all_tables;

	/// List to store the dialogbox component values
	
	List				m_controllist;

	/// To store the original size of the dialog box

	RECT				m_dlgrect;

	/// To store the co-ordinates of the dialog box wrt the screen
	
	RECT				m_wndrect;
};

#endif
