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


/*! \file TabHistory.h
    \brief handles everything related to history window
        
*/

#ifndef _QueryHistoryEdit_H_
#define _QueryHistoryEdit_H_

#include "FrameWindowHelper.h"
#include "FindAndReplace.h"

class FindAndReplace;

/*! \class TabHistory
    \brief SQLyog history (Query) logging is done by this class
*/
class TabHistory :  public TabTypes
{
public:
     /// Constructor with parent window handle.
    /**
    @param hwndparent: IN parent window handle
    */
	TabHistory(HWND hwndparent, MDIWindow* wnd);

    /// Default destructor
    /**
    Free up all the allocated resources
    */
	~TabHistory();

    /// Creates the history window
    /**
    @param wnd: IN pointer to the parent connection window class
    @returns wyBool 
    */
    wyBool      Create();

    void        Show(wyBool setfocus);
    
    wyBool      CloseTab(wyInt32 index);

	/// Shows/Hides all the content of the editor
    /**
    @param tabindex: IN currently selected tab index.
    @param status: IN wyTrue/wyfalse
    @returns void
    */
	VOID	ShowTabContent(wyInt32 tabindex, wyBool status);

	/// Handles on tab selection change
	/*
	@returns VOID
	*/
	VOID		OnTabSelChange();

	/// Handles on tab selection changing.
	/*
	@returns VOID
	*/
	VOID		OnTabSelChanging();

	/// Handles on Tab Closing.
	/*
	@returns one on success else zero
	*/
	wyInt32		OnTabClosing(wyBool ismanual);

	/// Handles on  Tab Closed.
	/*
	@returns VOID
	*/
	VOID		OnTabClose();	

	/// HAndles when MDIWindow is closing.
	/*
	@returns one on success else zero
	*/
    wyInt32		OnWmCloseTab();

	///Handles all tab controls
	/*
	@param tabcount: IN total number of tabs in tabcontroller
	@param selindex: IN currently selected tab index.
    @returns void
	*/
    VOID		HandleTabControls(wyInt32 tabcount, wyInt32 selindex);

    VOID		HandleFlicker();

    ///Copies selected Data
	/**
	@returns void
	*/
	void		CopySelectedData(HWND hwnd);

    /// Gets the current history window handle
    /**
    @returns HWND the window handle
    */
	HWND		GetHwnd();
	
    /// Resize the window
    /**
    @return void
    */
	void		Resize(wyBool issplittermoved = wyFalse);

    /// Brings the window on top
    /**
    @return wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool      ShowOnTop();

    /// Adds the string(query) to history window
    /**
    @param historystring: IN query string to add.
    @return void
    */
	void        AddText(wyString &historystring);

	/// sets the string(query) to history window
    /**
    @param historystring: IN query string to add.
    @return void
    */
	void		SetText(wyString *historystring);

    /// Sets the font 
    /**
    It will not take from the .ini, if we are taking from .ini 
        he positioning also need to be calculated
    @returns void
    */
	void		SetFont();
    /// Sets the font color
    /**
    It takes the font color from the .ini
    @returns void
    */
	void		SetColor();


    static wyBool SaveHistory(HWND hwnd);

     /// Shows the context menu
    /**
    @param lParam: IN window procedure lParam(Window cordinamtes of mouse)
    @return void
    */
	static wyInt32        OnContextMenu(LPARAM lParam, HWND hwnd, HMENU* phmenu);

    /// Handles the WM_COMMAND 
    /**
    @param hwnd: IN window handle
    @param wParam: IN window procedure WPARAM
    @return void
    */
    static void        OnWmCommand(HWND hwnd, WPARAM wParam);
	
	FindAndReplace		*m_findreplace;
	void GetCompleteTextByPost(wyString &query, MDIWindow *wnd);

private:

    /// Window procedure for the query history class
	static      LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /// Creates the edit control(Scintilla)
    /**
    @param wnd: IN connection window pointer
    @param hwndparent: IN parent window handle
    */
	HWND        CreateEditWindow();
    
    /// Sets the last line in scintilla visible
    /**
    @return wyBool, wyTrue if success, otherwise wyFalse
    */
	wyBool      SetLastLineVisible();
	

    WNDPROC     m_wporigproc;

    HANDLE      m_hevent;

	MDIWindow	*m_wnd;

	wyBool		m_isMDIclose;	

	HMENU		m_menu;
};

#endif