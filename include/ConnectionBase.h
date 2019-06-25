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


#include "CopyDatabase.h"

#ifndef _CCONNECTION_H
#define _CCONNECTION_H

#include "Global.h"

extern	PGLOBALS pGlobals;

#define FIND_STR_LEN	256

#define		REGAPPID		"InU"
/* window identifiers for all windows in both the connection dialog tabs */
/* since we create everything by hand ( and not using the resource eidtor) 
            we the IDs here in addition to the definitions in resource.h */

#define		STARTID				2000
#define		GENERALWND			STARTID+1
#define		TUNNELWND			STARTID+2
#define		IDC_GENHOST			STARTID+3
#define		IDC_GENUSER			STARTID+4
#define		IDC_GENPWD			STARTID+5
#define		IDC_GENDB			STARTID+6
#define		IDC_GENPORT			STARTID+7

#define SERVER_TAB  0
#define HTTP_TAB    1
#define SSH_TAB     2
#define SSL_TAB     3 
#define ADVANCE_TAB		4

#define		DEFAULT_CONN		_(L"(Select an existing connection...)")

#define     CURRENT_QUERY                                  1
#define     SELECTED_QUERY                                 2
#define     ALL_QUERY                                      3
#define     PASTE_QUERY									   4 


class ConnectionBase
{

protected:

	/// Connection details modified?
    wyBool			m_conndetaildirty;

	/// Is Ent?
    wyBool			m_isent;

	///Is .ini Converted?
	wyBool			m_isconv;

	/// Font HANDLE 1 for about dialog
    HFONT			m_haboutfont;

    /// Font HANDLE 2 for about dialog
    HFONT			m_haboutfont2;

	//Licance info caption Font
	HFONT			m_hreginfofont;

    /// Font HANDLE for staus bar
    HFONT			m_hstatusfont;

	/// Handles font for title
    HFONT           m_hfonttitle;

	/// Font handler for contents
    HFONT           m_hfontcontent;

    /// Status bar parts
	wyInt32         m_sbparts[7];
    
    /// Status bar message
    wyString        m_statuslink;

    /// Status window HANDLE
    HWND           m_hwndlink;
    
	/// Status bar static original window procedure
    WNDPROC		    m_wpstatusbatstaticcorigproc;

    /// Status bar original window procedure
	WNDPROC         m_wpstatusbarorigproc;

	wyBool			m_isencrypted_clone;

    
public:

    ConnectionBase();
    virtual ~ConnectionBase();
    
	/// Checks for Ent edition
	/**
	@returns wyTrue if ent else wyFalse
	*/
	wyBool IsEnt();

	static  LRESULT		CALLBACK StaticDlgProcLinkCursor (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	WNDPROC			m_wpstaticorigproc;

    void    OnInitConnDialog(HWND hwnd);

    /// Helps to show the user name (Registered) in the about window
    /**
    @param hwnd: IN window handle
    @return wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
    virtual wyBool  AboutRegInfo(HWND hwnd) = 0;

	/// Function initializes the dialog box with the default value.
	// also sets the maximum text which can be entered into the value fields.
	/**
	@param hwnd			: Window HANDLE
	@param lparam		: Long message parameter
	@returns void
	*/
    void OnInitDialog(HWND hwnd, LPARAM lparam);

	/// Function initializes the connection list dialog box with the default value.
	// also sets the maximum text which can be entered into the value fields.
	/**
	@param hwnd			: Window HANDLE
	@param lparam		: Long message parameter
	@returns void
	*/

	/// Handles when test connection is selected
	/**
	@param hwnd			: IN Window HANDLE
	@returns void
	*/
    virtual void OnTestConnection(HWND hwnd) = 0;

	
	/// Handles the commands for about window
	/**
	@param hwnd		: IN Windows HANDLE 
	@param wparam	: IN Unsigned message parameter
	@returns void
	*/
    virtual void OnAboutWmCommand(HWND hwnd, WPARAM wparam) = 0;

	/// Handles commands for status bar
	/**
	@param hwnd		: IN Windows HANDLE 
	@param wparam	: IN Unsigned message parameter
	@returns void
	*/
    virtual void OnStatusBarWmCommand(HWND hwnd, WPARAM wparam) = 0;

    /// Creates tab for connection window
    /**
    @param hwnd			: IN Dialog handle
    @param id			: IN tab id to create
	@param ispowertools	: Is powertools connection window
    @return wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
    virtual wyBool  CreateTabs(HWND hwnd, wyUInt32 id, wyBool ispowertools) = 0;
    
	/// Gets the initial details
    /**
    @param hdlg		: IN dialog window handle
    @return wyBool, wyTrue is success, otherwise wyFalse
    */
    wyBool  GetInitialDetails(HWND hdlg);

    /// Function deletes the curretnly selected connection details from the ini file.
    /**
    @param hdlg		: IN dialog window handle
    @returns wyInt32, i if success, otherwise 0
    */
    virtual wyInt32 DeleteConnDetail(HWND hdlg) = 0;

	/// Writes the default values on to the window
	/**
	@param conn			: IN Connection details
	@param directory	: IN Directory name
	@param connname		: IN Connection name
	@returns void
	*/
    virtual void WriteDefValues(HWND hdlg) = 0;

    /// Saves a connection details
    /**
    @param hdlg			: IN dialog window handle
    @return wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
    virtual wyBool  SaveConnection(HWND hdlg) = 0;

	/// Function is called when the connect button is pressed 
	/**
	@param hwnd			: IN Window HANDLE
	@dbname				: IN/OUT Connection information
	@returns void
	*/
    virtual void    OnConnect(HWND hwnd, ConnectionInfo * dbname) = 0;


	virtual	void    OnConnect(ConnectionInfo * dbname) = 0;

		    /// Gets the details to connect to mysql
    /**
    @param hdlg: IN dialog window handle
    @param coninfo: IN ConnectionInfo structure
    @returns MYSQL pointer, If SUCCESS, otherwise NULL
    */
	virtual MYSQL* ConnectToMySQL(ConnectionInfo * coninfo) = 0;

    
	/// Command handler for connection dialog
	/**
	@param hwnd			: IN Window HANDLE
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@param dbname		: IN Connection information
	*/
	virtual void    OnWmCommandConnDialog(HWND hwnd, WPARAM wparam, LPARAM lparam, ConnectionInfo * dbname) = 0;

    /// About window procedure
	/**
	@param hwnd			: IN Window HANDLE
	@param message		: IN Window Messages
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	*/
    static INT_PTR CALLBACK AboutDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        
	/// Subclassed window procedure for the link static control so that we can change the cursor to something better.
	/**
	@param hwnd			: IN Window HANDLE
	@param message		: IN Window Messages
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	*/	
	//Created in the GUIHelper so that all dialogs can use it..
    //static LRESULT CALLBACK AboutStaticDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        
	/// Connection name dialog window procedure
	/**
	@param hwnd			: IN Window HANDLE
	@param message		: IN Window Messages
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	*/
    static INT_PTR CALLBACK ConnDetailDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /// Initializes the dialog box with the details in the about dialog box.
	/**
	@param hwnd			: IN Window HANDLE
	@returns zero
	*/
	wyInt32 OnAboutInitDialog(HWND hwnd);
    
	/// Handles the color static
	/**
	@param hwnd			: IN Window HANDLE
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns zero
	*/
	wyInt32 OnWmCtlColorStatic(HWND hwnd, WPARAM wparam, LPARAM lparam);



	/// Handles the color static for status bar
	/**
	@param hwnd			: IN Window HANDLE
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns non - zero on success else zero
	*/
    virtual wyInt32 OnStatusBarWmCtlColorStatic(HWND hwnd, WPARAM wparam, LPARAM lparam) = 0;
    
    /// This invoke the confirmation msg to save the connection details
    /**
    @param hdlg			: IN dialog window handle
    @returns void
    */  
    wyBool    ConfirmAndSaveConnection(HWND hwnd, wyBool onconnect = wyFalse);

    /// Creates the connection dialog controls
    /**
    @param hwnd			: IN dialog handle
	@param ispowertools	: Is powertools connection window
    @return wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
    static wyBool  CreateConnectDialogWindows(HWND hwnd,  wyBool ispowertools = wyFalse);

    /// Initializes the connection window
    /**
    @param hdlg			: IN dialog handle
    @param combo		: IN combo handle
    @param toadd		: IN whether to add DEFAULT_CONN string or not
    @param selectfirst: IN whether to select first or not
    @returns void
    */
    static void    InitConnDialog(HWND hdlg, HWND combo = 0, wyBool toadd = wyTrue, wyBool selectfirst = wyTrue);

    /// Initializes the code page combo in the connection dialog
    /** 
    @param hdlg			: IN connection dialog handle
    @param ctrl_id		: IN combo id
    @returns void
    */
    static void    InitCodePageCombo(HWND hdlg, wyInt32 ctrl_id);

    /// Function changes the state of various buttons in the conection dialog box
    /**
    @param hdlg			: IN dialog window handle
    @param state		: IN 
    */
    static wyInt32 ChangeConnStates(HWND hdlg, wyBool state);

    /// Creates a new connection dialog window
    /**
    @param hdlg			: IN dialog window handle
    @returns wyInt32, 1 if SUCCESS, 0 otherwise 
    */
    wyInt32 NewConnection(HWND hdlg);

	  /// Clones a connection
    /**
    @param hdlg			: IN dialog window handle
    @returns wyInt32, 1 if SUCCESS, 0 otherwise 
	*/
	
	wyInt32 CloneConnection (HWND hdlg);

    /// This invoke the connection change dialog
    /**
    @param hdlg			: IN dialog window handle
    @return wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
    wyBool  ChangeConnName(HWND hdlg);

	/// Checks the connection name
	/**
	@param connectrionname		: Connection name
	@param newcon				: Old connection or new 
	*/
    wyInt32 CheckConnectionName(wyWChar *connectionname, wyString * currname, wyBool newcon);

	/// Handles the connection notification
	/**
	@param hwnd			: IN Window HANDLE
	@param lparam		: IN Long message parameter
	@returns void
	*/
    void    OnWmConnectionNotify(HWND hwnd, LPARAM lParam);

	static wyBool  OnConnDialogTabSelChange(HWND hwnd, HWND hwndtab);

	 /// Shows or hides the HTTP tab options
    /**
    @param hwnd         : IN Window HANDLE
    @param enable       : Flag for showing or hiding
    */
    static  void    ShowHttpTabOptions(HWND hwnd, wyBool enable = wyFalse);

    /// Shows or hides the Server tab options
    /**
    @param hwnd         : IN Window HANDLE
    @param enable       : Flag for showing or hiding
    */
    static  void    ShowSeverTabOptions(HWND hwnd, wyBool enable = wyFalse);

    /// Shows or hides the SSH tab options
    /**
    @param hwnd         : IN Window HANDLE
    @param enable       : Flag for showing or hiding
    */
    static  VOID    ShowSshTabOptions(HWND hwnd, wyBool enable = wyFalse);

    /// Shows or hides the SSL tab options
    /**
    @param hwnd         : IN Window HANDLE
    @param enable       : Flag for showing or hiding
    */
    static  VOID    ShowSslTabOptions(HWND hwnd, wyBool enable = wyFalse);

	// Shows or hides a perticular arry of ids
    /**
    @param hwnd         : IN Window HANDLE
    @param array        : IN Array of ids 
    @param arraycount   : IN Number of ids
    @param enable       : IN Enable or disable
    */
    static  VOID    ShowOrHide(HWND hwnd, wyInt32 array[], wyInt32 arraycount, wyBool enable);


    /// Enable or Disable the given list of IDs
    /**
    @param hwnd         : IN Window HANDLE
    @param array        : IN array if ids
    @param arraycount   : IN Number of ids
    @param enable       : IN Flag for showing or hiding
    */
    static  VOID    EnableOrDisable(HWND hwnd, wyInt32 array[], wyInt32 arraycount, wyBool enable);
   
    /// Connectes to the mysql server
    /**
    @param hdlg			: IN dialog window handle
    @param coninfo		: IN ConnectionInfo structure
    @returns MYSQL pointer, If SUCCESS, otherwise NULL
    */
	virtual MYSQL* ConnectToMySQL(HWND hdlg, ConnectionInfo * coninfo) = 0;

	/// Function to create duplicate connection
	/**
	@param hwndactive	: Active window HANDLE
	@param pcquerywnd	: Query window pointer
	@param conninfo		: Connection information
	@returns wyTrue on success else wyFalse
	*/
    virtual wyBool OnNewSameConnection(HWND hwndactive, MDIWindow *pcquerywnd, ConnectionInfo &conninfo) = 0;


    /// Establishes the connection to mysql
    /**
    @param coninfo      : IN Connection details
    @param errormsg     : IN Get the error messages
    */
    virtual MYSQL* ConnectToMySQL(ConnectionInfo *coninfo, Tunnel *tunnel, MYSQL *mysql, wyString &errormsg) = 0;

	/// Activates the connection dialog
	/**
	@param conninfo		: Connection information
	@returns non zero on success else zero
	*/
    virtual wyInt32 ActivateDialog(ConnectionInfo *conninfo) = 0;

	/// Handles menu items
	/**
	@param menuindex	: IN Menu index
	@param hmenu		: IN Menu HANDLE
	@returns void
	*/
    virtual VOID    HandleMenu(wyInt32 menuindex, HMENU hmenu) = 0;

	/// Handles the tab notification
	/**
	@param lparam		: Long message parameter
	@returns 1 
	*/
    virtual LRESULT  OnTabWmNotify(LPARAM lParam) = 0;

	/// Handles the help window
	/**
	@param hwndhelp		: IN Handler to help window
	@param hpos			: IN Horizontal position
	@param vpos			: IN Vertical position
	@param width		: IN Width
	@param height		: IN Height
	@returns void
	*/
    virtual VOID HandleHelp(HWND hwndhelp,  wyInt32 *hpos, wyInt32 *vpos, wyInt32 *width, wyInt32 *height, wyInt32 rectbottom) = 0;

	/// Handlers the editor control
	/**
	@param hwnd			: IN Window HANDLE
	@param hwndhelp		: IN Handler to help window
	@param hwndfilename : IN Filename window HANDLE
	@param save			: IN Saved or not
	@param tabcount		: IN Tab count 
	@param selindex		: IN Select index
	@returns void	
	*/
    virtual VOID HandleEditorControls(HWND hwnd, HWND hwndhelp, HWND hwndfilename, HWND hwndsplitter, HWND hwndtabmgmt,  wyBool save, wyInt32 tabcount, wyInt32 selindex) = 0;
    
	/// Creates icon list
	/**
	@param hwndmain			: IN Window HANDLE
	@param numicons			: IN/OUT  Number of icons
	@returns icons details
	*/
    virtual yogIcons * CreateIconList(HWND hwndmain, wyUInt32 *numicons) = 0;
	
    /// Creates a export window
    /**
    @param wnd              : IN Query window pointer
    @param wparam           : IN Unsigned int message pointer
    @returns void
    */
    virtual VOID OnExport(MDIWindow *wnd, WPARAM wparam) = 0;

    /// Used to create the source instance 
	/**
    @param copydb : IN CopyDatabase instance pointer
	@returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
	*/
    virtual wyBool  CreateSourceInstance(CopyDatabase *copydb) = 0;

	/// Used to create the target instance if the target database is on different connection
    /**
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
    virtual wyBool	CreateTargetInstance(CopyDatabase *copydb) = 0;

	/// Copies http authentication information
	/**
	@param src			: IN Source tunnel pointer
	@param tgt			: OUT Target tunnel pointer
	@returns wyFalse	
	*/
    virtual wyBool	CopyHttpAuthInfo(Tunnel *src, Tunnel *tgt) = 0;
	
	/// Command handler
	/**
	@param hwndactive	: IN Active window HANDLE
	@param wnd			: IN Query window HANDLE
	@param wparam		: IN Unsigned window parameter
	@returns wyFalse
	*/
    virtual wyBool	OnWmCommand(HWND hwndactive, MDIWindow *wnd, WPARAM wparam) = 0;

	/// Handler to AddToMessageQueue()
	/**
	@param wnd			: IN Query window pointer
	@param query		: IN/OUT  Query
	@returns void
	*/
    virtual VOID    HandlerAddToMessageQueue(MDIWindow *wnd, wyChar *query) = 0;
			VOID    GetOtherValues(HWND hdlg, ConnectionInfo *coninfo);

	/// Register information
	/**
	@param hwnd			: IN Window HANDLE
	@returns wyTrue
	*/
    virtual wyBool	RegisterInformation(HWND hwnd) = 0;

	/// Checks for registration 
	/**
	@param hwnd			: IN Window HANDLE
	@param main			: IN/OUT Void 
	*/
    virtual wyBool  CheckRegistration(HWND hwnd, void *main) = 0;

	/// Handles things to do when the window is closed
	/**
	@returns void
	*/
    virtual VOID    OnClose() = 0;

	/// Function to update the sqlite node
	/**
	@param wnd			: IN Query window pointer
	@returns void
	*/
    virtual VOID    UpdateSqliteNodes(MDIWindow *wnd) = 0;

	/// Handler to GetInitialDetails();
	/**
	@param hdlg			: IN Dialog HANDLE
	@returns wyTrue on success else wyFalse
	*/
    virtual wyBool  HandlerToGetInitialDetails(HWND hdlg) = 0;

	/// Function initializes array of buttons and adds them to the second toolbar(Power tools) 
	/**
	@param hwndsecondtool	: IN Second toolbar handle
	@param hsecondiml		: IN Image list handle
	@returns wyTrue
	*/
	wyBool CreateOtherToolButtons(HWND hwndsecondtool, HIMAGELIST hsecondiml);

	/// Enables the window menu items
	/**
	@param hmenu		: IN Menu HANDLE
	@returns wyTrue
	*/
    wyBool  EnableWindowItems(HMENU hmenu);

	/// Enables the help menu items
	/**
	@param hmenu		: IN Menu HANDLE
	@returns wyTrue
	*/
    wyBool  EnableHelpItems(HMENU hmenu);

	// Checks the index is of window menu
	/**
	@param index		: IN Index
	@returns wyTrue if window menu else wyFalse
	*/
    wyBool     IsWindowMenu(wyInt32 index) ;

	/// Function to manage status bar
	/**
	@param hwndstatus	: IN Handler to status bar
	@returns void
	*/
    void    ManageStatusBar(HWND hwndstatus);

	// Function to set various parts in the status bar of the main window.
	/**
	@param hwndstatus	: IN Status window HANDLE
	@returns wyTrue
	*/
    wyBool  SetStatusParts(HWND hwndstatus);

	/// Handles the resizing of status window
	/**
	@param hwndmain		: IN Main window HANDLE
	@param hwndstatus	: IN Status window HANDLE
	@returns void
	*/
    void    ResizeStatusWindow(HWND hwndmain, HWND hwndstatus);

    /// Sets the Imagelist
	/**
	@param main		: IN FrameWindow pointer
	@returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
	*/
    virtual wyBool SetToolComboImageList(FrameWindow *main) = 0;

    /// Callback function for status bar control
	/**
	@param hwnd			: IN Window handle
	@param message		: IN Window message
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	*/
    static wyInt32 CALLBACK StatusBarDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	/// Callback function for static control on status bar
	/**
	@param hwnd			: IN Window handle
	@param message		: IN Window message
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	*/
    static LRESULT CALLBACK StatusBarStaticDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /// Enables/Disables the the listing tags menu
    /**
    @param hmenu: IN handle to the menu
    @returns void
    */
    virtual VOID    HandleTagsMenu(HMENU hmenu) = 0;

    /// helps to close the helper threads
    virtual VOID    CloseThreads(MDIWindow *wnd) = 0;

    // rebuild after group process
	virtual VOID  RebuildTags(MDIWindow *wnd, wyChar *database) = 0;

	///handles connection window help
	/**
	@param hwnd			: IN Window handle
	@returns void
    */
	virtual VOID OnWmConnectionHelp(HWND hwnd)=0;

	/// To paint the Tabs
	/**
	@param wparam        : IN Unsigned int message parameter
	@return void
	*/
	virtual VOID		RepaintTabs(WPARAM wparam) = 0;
	
	/// Destroys all the fonts created during the about dialog paint 
	/**
	@returns void
	*/
	VOID			DestroyFonts();
	
	wyString         m_linkadd;

	static VOID	ExamineData(wyString &codepage, wyString &buffer);

	/// Handles the tool tip information
	/**
	@param lpnmtdi		: IN Tool tip display information
	@param string		: String to display
	@returns void	
	*/
	VOID    OnToolTipInfo(LPNMTTDISPINFO lpnmtdi, wyWChar *string);

	/// Enables/Diables all tool bar buttons and combo boxes
	/**
	@param hwndtool			: IN Toolbar window handle
	@param hwndsecondtool	: IN Second toolbar handle
	@param hwndcombo		: IN Combo box handle
	@param enable			: IN Enabled or disabled
	@param isexec			: IN Query is executing or not.
	@returns wyTrue
	*/
    wyBool EnableToolButtonsAndCombo(HWND hwndtool, HWND hwndsecondtool, HWND hwndcombo, wyBool enable, wyBool isexec = wyFalse);

    /// Shows the server options and hide the rest
    /**
    @param hdlg			: IN dialog window handle
    @returns void
    */    
    VOID    ShowServerOptions(HWND hdlg);

	//Set the Autocomplte .ini value to specify the Building tag is completed or not
	/**
	@param connsection : IN connection section name
	@param dirstr	   : IN .ini path
	@return VOID
	*/
	VOID	SetAutocompleteTagBuildFlag(wyString *connsection, wyString *dirstr);

	///Pops the 'Ultimate' Dilaog box here
	/**
	@return VOID
	*/
	VOID	GetSQLyogUltimateDialog();

	///Pops the 'Get Ent' Dilaog box here
	/**
	@return VOID
	*/
	VOID	GetSQLyogEntDialog();

	///window procedure for the dialog which appears when we click an enterprise feature
	/**
	@param hwnd			: IN Window HANDLE
	@param message		: IN Window Messages
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	*/
	static	INT_PTR CALLBACK GetUltimateDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam); 

	///window procedure for the dialog which appears when we click an enterprise feature
	/**
	@param hwnd			: IN Window HANDLE
	@param message		: IN Window Messages
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	*/
	static	INT_PTR CALLBACK GetEntDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam); 

	VOID			SetGetEntCaptionText(HWND hwnddlg);

	//for Importing External Data
	/**
	@return VOID
	*/
	virtual  void    OnSchdOdbcImport() = 0;

	//for Exporting database
	/**
	@return VOID
	*/
	virtual  void    OnScheduleExport() = 0;

	//For formatting queries
	/**
	@param wnd : IN MDIWindow handle
	@param hwndeditor : IN editor handle
	@param : IIn query to be formatted
	@param typeofformate : IN Format type
	@return VOID
	*/
	virtual void        FormateAllQueries(MDIWindow *wnd, HWND hwndeditor, wyChar *query, wyInt32 typeofformate) = 0;

	/// Shows or hides the AdvancedTab options
    /**
    @param hwnd         : IN Window HANDLE
    @param enable       : Flag for showing or hiding
    */
    static  VOID    ShowAdvanceTabOptions(HWND hwnd, wyBool enable = wyFalse);


	/// Gets the AdvancedTab Connection details from the .ini file
    /**
    @param conn     : IN Structure containing connection details
    @param connspe  : IN Connection specification
    @param path     : IN Ini file path
    */
	void    GetAdvancedTabDetailsFromFile(ConnectionInfo *conn, wyChar *connspe, const wyChar *path);


	/// Fills the AdvancedTab with all the values given in 'conninfo'
    /**
    @param hwnd         : IN Window HANDLE
    @param conninfo     : IN Connection details 
    */
	VOID    FillAdvancedTab(HWND hwnd, ConnectionInfo *conninfo);

	/// Saves the AdvancedTab details to ini file
    /**
    @param hwnd				: IN Window HANDLE
    @param conn				: IN Connection name
    @param directory		: IN Path for the ini file
    */
	VOID    SaveAdvancedTabDetails(HWND hdlg, const wyChar *conn, const wyChar *directory);

	//Add the UUID to regestry
	/**
	@return VOID
	*/
	VOID	HandleApplicationUUID();

	/// Saves the server details to ini file
    /**
    @param hwnd         : IN Window HANDLE
    @param conn         : IN Connection name
    @param directory    : IN Path for the ini file
    */
    VOID    SaveServerDetails(HWND hwnd, const wyChar *conn, const wyChar *directory);

	//Handle the common options in "MySQL' tab of Con. dialog
	/**
	@param hwnd		: IN Handle to con. dialog
	@param dbname	: IN ConnectionInfo struct handle
	@param id		: IN Message id to handle
	@return VOID
	*/
	VOID	HandleCommonConnectOptions(HWND hwnd, ConnectionInfo *dbname, wyInt32 id);

	//Handle the Table->Engine sub menu
	/**
	@param hmenuhandle : IN handle to submenu
	@param index	   : IN submenu iposition
	@return VOID
	*/
	VOID	HandleTableEngineMenu(HMENU hmenuhandle, wyInt32 index);

	/// Populate color array for connection combo
    /**
    @param hwndcombo		: IN combobox HANDLE
    @param dirstr			: IN Path for the ini file
	@return VOID
	*/
	void	PopulateColorArray(HWND hwndcombo, wyString *dirstr, wyInt32 ConnNo = -1, COLORREF bkcolor = RGB(255,255,255));


	void	WriteMysqlDefValues(HWND hdlg);

	void	GetConnectionName(HWND hdlg, wyString *connnamestr, const wyChar *conn, const wyChar *directory);

	void	DeleteNewConnectionDetails(HWND hdlg);
	
	void	GetSqlModeSettings(HWND hdlg, ConnectionInfo *coninfo);

    void    GetInitCommands(HWND hdlg, ConnectionInfo *coninfo);

    void    GetKeepAliveInterval(HWND hdlg, ConnectionInfo *coninfo);

    void    OnValidConNameChngState(HWND hdlg, BOOL state);

    wyInt32  OnComboEraseBkGnd(HWND hwnd, WPARAM wparam, LPARAM lparam);

    void    HandleCustomComboNotifiction(HWND hwnd, WPARAM wparam, LPARAM lparam);

	///Flag sets wyTrue when the 'Tagfile' for this connection built successfully while building
	wyBool			m_isbuiltactagfile;

	///Connection name 'section name' as in the .ini file
	wyString		m_consectionname;

	wyBool			m_isnewconnection;

	wyInt32			m_newconnid;

	//Version type
	EntType			m_enttype;

	//Check 'Rebuild tags' for Community
	wyUInt32		 m_powertoolsID;

	//selected color for object browser in connection dialog
	COLORREF	    m_rgbconnection;

	//selected color for object browser in connection dialog
	COLORREF	    m_rgbconnectionfg;

	//object browser background  color
	COLORREF        m_rgbobbkcolor;

	//object browser foreground  color
	COLORREF        m_rgbobfgcolor;

	//current connection name
	wyString        m_currentconn;

	//set the custom color for combobox
	wyBool			m_setcolor;

	//whether to change object browser color or not
	wyBool			m_changeobcolor;

	//array for connection color
	wyInt32         *m_arrayofcolor;

	wyBool			CopyAndRename(wyString& directorystr, wyString& fullpathstr, wyString& newpath);

};

#endif
