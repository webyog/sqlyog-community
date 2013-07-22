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

class TableTabInterfaceTabMgmt;

class TabPreview
{
public:
    //HWND                            m_hwndparent;

    HWND                            m_hwnd;

    HWND                            m_hwndpreview;

    TableTabInterfaceTabMgmt*       m_ptabmgmt;

    /// Original window proc
	WNDPROC                         m_wporigproc;
    
    wyInt32                         m_renamequeryrowno;

    wyBool                          m_istabempty;

    MDIWindow                       *m_mdiwnd;

    //..To avoid EN_CHANGE message processing in the TableTabInterfaceTabMgmt.
    wyBool                          m_settingpreviewcontent;

    HMENU		                    m_menu;

    /// Constructor
	/**
	@param hwndparent: IN HNADLE to the parent window
	*/

    
    
    TabPreview(HWND hwndparent, TableTabInterfaceTabMgmt *ptabmgmt);

    /// Creates the Preview Window
    /**
    @returns void
    */
    void                            Create();
    
    ///this sets up all XPM values.
	/**
	@param hwndedit: IN HNADLE to editor window
	*/
    void                            SetScintillaValues(HWND hwndedit);
    
    /// Resize function
    /**
    @returns void
    */
    void                            Resize();
    
    /// Window procedure for the query history class
	static LRESULT CALLBACK         WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    //..Handles Alt key pressed event to achieve Alt+1, Alt+2 in SD Create/Alter table dialog
    LRESULT                         OnWMSysKeyDown(WPARAM wParam, LPARAM lParam);
    

    /// Handles the key-down event of the window procedure
	/**
	@param hwnd			: IN Window HANDLE
	@param wparam		: IN Unsigned message parameter
	@param wnd          : IN con. window pointer
	@param ebase        : IN EditorBase pointer
	*/
	wyInt32				            OnWMChar(HWND hwnd, WPARAM wparam);

    /// Shows the context menu
    /**
    @param lParam: IN window procedure lParam(Window cordinamtes of mouse)
    @return void
    */
	wyInt32                         OnContextMenu(LPARAM lParam);

    ///Function enables and disables copy in history menu  
	/**
	@param hmenu         : IN Handle to the menu
	@returns wyTrue if success, otherwise wyFalse
	*/
	wyBool                          ChangeMenuItem(HMENU hmenu);

    /// Sets the fonts of the Preview Window
    /**
    @returns void
    */
    void                            SetFont();
    
    /// Generate the Query
    /** Uses various helper functions as listed below to generate the correct statements
    @param query		: OUT Column definitions
    @param execute      : IN    whether execute the query or not
    @returns    Creation of query was successful or not
    */	
	//wyBool			GenerateAndExecuteQuery(wyString &query, wyBool execute=wyFalse);

    /// Generate the actual query
    /** Uses various helper functions as listed below to generate the correct statements
    @param query		: IN Column definitions
    @param primary		: IN Primary key value definitions
    @param autoincr     : IN autoincrement definition
    @returns    Creation of query was successful or not
    */
    //wyBool		CreateCompleteQuery(wyString& query, wyString& primary, wyString& autoincr);

    /// create the actual query
    /** Uses various helper functions as listed below to generate the correct statements
    @param query		: OUT Column definitions
    @returns    Creation of query was successful or not
    */
	wyBool			CreateQuery(wyString &query);

    /// Generate (and execute) Alter Table Query
    /** Uses various helper functions as listed below to generate the correct statements
    @param query		: OUT Column definitions
    @param execute      : IN    whether execute the query or not
    @returns    Creation of query was successful or not
    */
    //wyBool          GenerateAndExecuteAlterTableQuery(wyString &query, wyBool execute=wyFalse);

    /// Generate (and execute) Create Table Query
    /** Uses various helper functions as listed below to generate the correct statements
    @param query		: OUT Column definitions
    @param execute      : IN    whether execute the query or not
    @returns    Creation of query was successful or not
    */
    //wyBool          GenerateAndExecuteCreateTableQuery(wyString &query, wyBool execute=wyFalse);

    /// Executes Partial Query
    /** Uses various helper functions as listed below to generate the correct statements
    @param query		: IN    query to be executed..
    @returns    wyTrue if executed successfully, else wyFalse
    */
    //wyBool          ExecutePartialAlterTableQuery(wyString &query);

    //..makes Windows Visible/Invisible
    /*
    @returns    void
    */
    wyInt32         OnTabSelChange();

    //..Sets the Preview Content
    /*
    @param  content     : IN    content
    @returns    void
    */
    void            SetPreviewContent(wyChar* content);

    void            UpdateTableName(wyString &newname);

    void            GenerateQuery(wyString &query);

    void            GenerateAndSetPreviewContent();

    //.. 
    /*
    @param isaltertable     : IN    
    @returns    void
    */
    void                    CancelChanges();
};