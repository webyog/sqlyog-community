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


#ifndef _QueryMessageEdit_H_
#define _QueryMessageEdit_H_

#include "FrameWindowHelper.h"
#include "TabQueryTypes.h"


class TabMessage :  public TabQueryTypes
{
public:

    /// constructor with parent window as argument.
    /**
    @param hwndparent	: IN parent window handle
    */
	TabMessage(MDIWindow* wnd, HWND hwndparent);

    /// Destructor
    /**
    Free up all the allocated resources
    */
	~TabMessage();

    /// Wrapper to Creates the message window
    /**
    @returns wyBool wyTrue if success, otherwise wyFalse.
    */
    wyBool      Create();

    /// Adds text to the edit control
    /**
    @param text			: IN text to add
    @returns void
    */
	void        AddText(wyString& str);

    /// Returns the message edit window handle
    /**
    @returns the handle to the current message edit
    */
	HWND        GetHwnd();

    /// resizes the window
    /**
    @returns void
    */
	void        Resize();

	/// Sets the message edit font
    /**
    @returns void
    */
	void        SetFont();

    void        SetColor();

    void        OnTabSelChange(wyBool isselected);

    void        UpdateStatusBar(StatusBarMgmt* pmgmt);

    wyBool        OnContextMenu(LPARAM lparam);

    void        OnWMCommand(WPARAM wparam);
	
	 /// HANDLE to the message edit window 
    HWND        m_hwnd;

    wyInt64        m_sumoftotaltime;
    wyInt64        m_sumofexectime;
private:

    /// Creates the message edit window with all properties
    /**
    @param hwndparent	: IN parentwindow handle
    @returns handle to the new message window
    */
    HWND        CreateQueryMessageEdit(HWND hwndparent, MDIWindow *wnd);

    /// Window procedure for the message window
	/**
	@param hwnd			: Window HANDLE
	@param message		: Window messages
	@param wparam		: Unsigned message parameter
	@param lparam		: Long message parameter
	*/
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    static LRESULT	CALLBACK ComboListProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    void                CopyTextToClipBoard();
    
	/// Font HANDLE
	HFONT       m_hfont;

    /// original wnd procedure
	WNDPROC     m_wporigproc;

    COLORREF m_rgbbgcolor;

    COLORREF m_rgbfgcolor;

    COLORREF m_rgbselectioncolor;

    wyInt32     m_selectedoption;
    HWND        m_hwndcombo;
    WNDPROC     m_origlistproc;
    wyBool      m_isautomatedmove;
};

#endif
