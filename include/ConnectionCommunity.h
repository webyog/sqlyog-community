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


#ifndef _CCONNECTIONFREE_H
#define _CCONNECTIONFREE_H

#include "ConnectionBase.h"
#include "Copydatabase.h"

class ConnectionCommunity : public ConnectionBase
{

public:

    ConnectionCommunity();

	/// Handler to OnInitDialog()
	/**
	@param hwnd			: Windows Handler
	@param lparam		: Long message parameter
	@returns void
	*/
    void    HandlerToOnInitDialog(HWND hwnd, LPARAM lparam);

	/// This function is called when the text connection button is pressed
	/**
	@param hwnd			: Window HANDLE
	@returns void
	*/
    void    OnTestConnection(HWND hwnd);

    /// Helps to show the user name (Registered) in the about window
    /**
    @param hwnd: IN window handle
    @return wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
    wyBool  AboutRegInfo(HWND hwnd);

	/// Command handler for about window
	/**
	@param hwnd			: IN Window HANDLE
	@param wparam		: IN The high-order word specifies the notification code
	@returns void
	*/
    void    OnAboutWmCommand(HWND hwnd, WPARAM wparam);

	/// Command handler for status bar window
	/**
	@param hwnd			: IN Window HANDLE
	@param wparam		: IN The high-order word specifies the notification code
	@returns void
	*/
    void    OnStatusBarWmCommand(HWND hwnd, WPARAM wparam);

     /// Creates tab for connection window
    /**
    @param hwnd			: IN Dialog handle
    @param id			: IN tab id to create
	@param ispowertools	: Is powertools connection window
    @return wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
    wyBool  CreateTabs(HWND hwnd, wyUInt32 id,  wyBool ispowertools);

	/// Handler to GetInitialDetails();
	/**
	@param hdlg			: IN Dialog handle
	@returns wyTrue on success else wyFalse
	*/
    wyBool  HandlerToGetInitialDetails(HWND hdlg);
    
	/// Function deletes the currently selected connection details from the ini file.
    /**
    @param hdlg: IN dialog handle
    @returns wyInt32, i if SUCCESS, otherwise 0
    */
    wyInt32 DeleteConnDetail(HWND hdlg);

	/// Writes the default values on to the window
	/**
	@param conn			: IN Connection details
	@param directory	: IN Directory name
	@param connname		: IN Connection name
	@returns void
	*/
    void WriteDefValues(HWND hdlg);

    /// Saves a connection details
    /**
    @param hdlg: IN dialog window handle
    @return wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
    wyBool  SaveConnection(HWND hdlg);

    /// Function just tests the connection and checks whether it is correct or not
    /**
    @param hdlg     : IN Connection dialog window handle
    @param showmsg  : IN whether to show the version msg or not (Default is TRUE)
    @returns wyBool, wyTrue if SUCCESS, otherwise wyFalse
    */
    wyBool  TestConnection(HWND hwnd, wyBool showmsg =wyTrue);

    /// Gathers all the info for the connection
    /**
    @param hdlg     : IN dialog window handle
    @param coninfo  : IN ConnectionInfo structure
    @returns MYSQL pointer, If SUCCESS, otherwise NULL
    */
	MYSQL* ConnectToMySQL(HWND hdlg, ConnectionInfo * coninfo);

    /// Connects to the mysql server
    /**
    */
    MYSQL* ConnectToMySQL(ConnectionInfo *conn, Tunnel *tunnel, MYSQL *mysql, wyString &errormsg);

	//MYSQL* ConnectToMySQL(ConnectionInfo * coninfo);

	MYSQL* ConnectToMySQL(ConnectionInfo * coninfo, wyBool iscon_res = wyFalse);

	/// Function is called when the connect button is pressed 
	/**
	@param hwnd			: IN Window HANDLE
	@dbname				: IN Connection information
	@returns void
	*/
    void    OnConnect(HWND hwnd, ConnectionInfo * dbname);

	void    OnConnect(ConnectionInfo * dbname, wyBool iscon_res = wyFalse);

	//void    OnConnect(ConnectionInfo * dbname);

	/// Command handler for connection dialog
	/**
	@param hwnd			: IN Window handle
	@param wparam		: IN The high-order word specifies the notification code of the control
	@param lparam		: Long message parameter
	@param dbname		: Connection information
	*/
    void    OnWmCommandConnDialog(HWND hwnd, WPARAM wparam, LPARAM lparam, ConnectionInfo * dbname);

	/// Function to create duplicate connection
	/**
	@param hwndactive	: IN Active window handle
	@param pcquerywnd	: IN MDI window pointer
	@param conninfo		: IN Connection information
	@returns wyTrue on success else wyFalse
	*/
    wyBool OnNewSameConnection(HWND hwndactive, MDIWindow *pcquerywnd, ConnectionInfo &conninfo);

	/// Command handler for connection dialog
	/**
	@param hwnd			: IN Window handle
	@param message		: IN Window message
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	*/
    static INT_PTR CALLBACK ConnectDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	/// Activates the connection dialog
	/**
	@param conninfo		: IN Connection information
	@returns non zero on success else zero
	*/
    wyInt32 ActivateDialog(ConnectionInfo *conninfo);

	/// Handles menu items
	/**
	@param menuindex	: IN Menu index
	@param hmenu		: IN Menu handle
	@returns void
	*/
    void    HandleMenu(wyInt32 menuindex, HMENU hmenu);

	/// Handles the tab notification
	/**
	@param lparam		: IN Pointer to an NMHDR structure 
	@returns 1 
	*/
    LRESULT  OnTabWmNotify(LPARAM lParam){return 1;};

	/// Handles the help window
	/**
	@param hwndhelp		: IN Handle to help window
	@param hpos			: IN Horizontal position
	@param vpos			: IN Vertical position
	@param width		: IN Width
	@param height		: IN Height
	@returns void
	*/
    void HandleHelp(HWND hwndhelp,  wyInt32 *hpos, wyInt32 *vpos, wyInt32 *width, wyInt32 *height, wyInt32 rectbottom);

	///Handle on click 'buy' button
	/**
	@return void
	*/
	void	OnClickBuy();

	/// Handlers the editor control
	/**
	@param hwnd			: IN Window handle
	@param hwndhelp		: IN Handler to help window
	@param hwndfilename : IN Filename window HANDLE
	@param save			: IN Saved or not
	@param tabcount		: IN Tab count 
	@param selindex		: IN Select index
	@returns void	
	*/
    void HandleEditorControls(HWND hwnd, HWND hwndhelp, HWND hwndfilename, HWND hwndsplitter, HWND hwndtabmgmt, wyBool save, wyInt32 tabcount, wyInt32 selindex);

	/// Creates icon list
	/**
	@param hwndmain			: IN Window handle
	@param numicons			: IN Number of icons
	@returns icons details
	*/
    yogIcons * CreateIconList(HWND hwndmain, wyUInt32 *numicons);
	    
	/// Creates a export window
    /**
    @param wnd              : IN Query window pointer
    @param wparam           : IN Unsigned int message parameter
    @returns void
    */
    void                    OnExport(MDIWindow *wnd, WPARAM wparam);

    /// Used to create the source instance 
	/**
    @param copydb : IN CopyDatabase instance pointer
	@returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
	*/
    wyBool                  CreateSourceInstance(CopyDatabase *copydb);

	/// Used to create the target instance if the target database is on different connection
	/**
    @param copydb : IN CopyDatabase instance pointer
	@returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
	*/
	wyBool  CreateTargetInstance(CopyDatabase *copydb);

	/// Copies http authentication information
	/**
	@param src			: IN Source tunnel pointer
	@param tgt			: IN Target tunnel pointer
	@returns wyFalse	
	*/
	wyBool  CopyHttpAuthInfo(Tunnel *src, Tunnel *tgt){ return wyFalse;}    

	/// Command handler
	/**
	@param hwndactive	: IN Active window handle
	@param wnd			: IN MDI window handle
	@param wparam		: IN The high-order word specifies the notification code of the control
	@returns wyFalse
	*/
	wyBool  OnWmCommand(HWND hwndactive, MDIWindow *wnd, WPARAM wparam);

	/// Handler to AddToMessageQueue()
	/**
	@param wnd			: IN MDI window pointer
	@param query		: IN Query
	@returns void
	*/
	void    HandlerAddToMessageQueue(MDIWindow *wnd, wyChar *query){return;};
	
	/// Handles the connection notification
	/**
	@param hwnd			: IN Window handle
	@param lparam		: IN Pointer to an NMHDR structure that contains the notification code and additional information. 
	@returns void
	*/
	void    OnWmConnectionNotify(HWND hwnd, LPARAM lparam){return;};

	/// Register information
	/**
	@param hwnd			: IN Window handle
	@returns wyTrue
	*/
	wyBool  RegisterInformation(HWND hwnd){return wyTrue;};

	/// Checks for registration 
	/**
	@param hwnd			: IN Window handle
	@param main			: IN Void FrameWindow pointer
	*/
	wyBool  CheckRegistration(HWND hwnd, void *main);

   	/// Handles things to do when the window is closed
	/**
	@returns void
	*/
	void    OnClose();

    ///Shows the Dialof box while opens and close the application
    /**
    @param hwndparent : IN main window handle
    @return 1;
    */
    wyInt32 ShowDialog(HWND hwndparent);

    ///window procedure for the dialor appears while opens & close connection   
	/**
	@param hwnd			: IN Window HANDLE
	@param message		: IN Window Messages
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	*/
    static INT_PTR	CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam); 

    static  LRESULT		CALLBACK StaticDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    		
	///Handles the WM_COMMAND message of dialog window
    /**
    @param hwnd   : IN handle to dialog
    @param wparam : IN WPARAM message parameter
    @returns void
    */
    void            OnDlgWmCommand(HWND hwnd, WPARAM wparam);

    /// Function to update the sqlite node
	/**
	@param wnd			: IN pointer to MDI window
	@returns void
	*/
	void    UpdateSqliteNodes(MDIWindow *wnd){return;}

    /// populates the tool combo imagelist
    /**
    @param main : IN Framewindow pointer
    */
    wyBool SetToolComboImageList(FrameWindow *main);

    /// Handles the color static for status bar
	/**
	@param hwnd			: IN Window HANDLE
	@param wparam		: IN Unsigned message parameter
	@param lparam		: IN Long message parameter
	@returns non - zero on success else zero
	*/
    wyInt32 OnStatusBarWmCtlColorStatic(HWND hwnd, WPARAM wparam, LPARAM lparam);

    /// Enables/Disables the the listing tags menu
    /**
    @param hmenu: IN handle to the menu
    @returns void
    */
    void    HandleTagsMenu(HMENU hmenu);
    /// helps to close the helper threads
    /**
    @param  wnd: IN connection window pointer
    @returns void
    */
    void    CloseThreads(MDIWindow *wnd);

    ///handles connection window help
	/**
	@param hwnd			: IN Window handle
	@returns void
    */
	void OnWmConnectionHelp(HWND hwnd);

    /// Rebuilding tags
	/**
	@param wnd			: IN con. window handle
	@param database		: IN database name
	*/
	virtual void  RebuildTags(MDIWindow *wnd, wyChar *database){}

	/// To paint the Tabs
	/**
	@param wparam        : IN Unsigned int message parameter
	@return void
	*/
	void		  RepaintTabs(WPARAM wparam);

	//for Importing External Data
	/**
	@return VOID
	*/
	void    OnSchdOdbcImport();

	//for Importing External Data
	/**
	@return VOID
	*/
	void    OnScheduleExport();

	//For formatting queries
	/**
	@param wnd : IN MDIWindow handle
	@param hwndeditor : IN editor handle
	@param : IIn query to be formatted
	@param typeofformate : IN Format type
	@return VOID
	*/
	void        FormateAllQueries(MDIWindow *wnd, HWND hwndeditor, wyChar *query, wyInt32 typeofformate);
	
	///DIalog control procedure
    WNDPROC			m_wporigstaticproc;

	//Flag sets wytrue when applicaion is closing
	wyBool			 m_isapclose;		
};

#endif
