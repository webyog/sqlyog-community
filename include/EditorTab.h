/* Copyright (C) 2006 Webyog Inc.

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

#ifndef __EDITORTAB_H__
#define __EDITORTAB_H__

#include "FrameWindowHelper.h"
#include "Global.h"
#include "scintilla.h"
#include "MDIWindow.h"
#include "CustTab.h"
#include "EditorQuery.h"
#include "EditorProcs.h"
#include "EditorBase.h"

class EditorProcs;

class EditorTab
{
public:
    
    /// Constructor with parent window handle.
    /**
    @param hwnd			: IN parent window handle
    */
	EditorTab(HWND hwnd);

    /// Destructor
    /**
    Free up all the allocated resources
    */
	~EditorTab();
   
    /// Wrapper to create Editor tab and initializes with a default normal query editor
    /**
    @param wnd			: IN parent connection window pointer.
    @return wyBool wytrue if it is SUCCESS, otherwise failure.
    */
    wyBool      Create(MDIWindow*wnd);
		
    /// Creates the EditorTab
    /**
    @param hwnd			: IN Parent window handle
    @returns HWND of the new editor tab
    */
	HWND        CreateEditorTab(HWND hwnd);

     /// Tab window procedure
	static	LRESULT CALLBACK TabWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    
    /// Wrapper to creates a normal query editor.
    /**
    @param wnd			: IN parent connection window pointer.
    @return wyBool wytrue if it is SUCCESS, otherwise failure.
    */
	wyBool      CreateQueryEditorTab(MDIWindow *wnd);

    /// Creates the normal query editor
    /**
    @param wnd			: IN parent connection window pointer.
    @param hwnd			: IN handle to the parent editor tab.
    @returns EditorQuery pointer, NULL if it fails
    */
    EditorQuery* CreateQueryEdit(MDIWindow *wnd, HWND hwnd);

    /// Wrapper to creates an Advanced Editor
    /**
    Creates an AdvEdit control on tab, it has got some difference with normal editor,
    because it executes all the queries in a single shot.
    @param wnd			: IN parent connection window pointer.
    @param title		: IN title to the new tab.
    @param image		: IN Image Id of the image to be shown with the tab caption.
    @param hitem		: IN The HTREEITEM pointer, 
    We are allowing to create an Advanced editor on the context of a treeview node only. 
    It is useful so that we can refresh that node once the content in the editor is executed successfully.
    @return wyBool wytrue if it is SUCCESS, otherwise failure.
    */

	wyBool      CreateAdvEditorTab(MDIWindow *wnd, wyWChar *title, wyInt32 image, HTREEITEM hitem);

    /// Creates the Advanced editor
    /**
    @param wnd: IN parent connection window pointer.
    @param hwnd: IN handle to the parent editor tab.
    @param hitem: IN The HTREEITEM pointer, 
    @returns EditorProcs pointer, NULL if it fails
    */
	EditorProcs*   CreateAdvEdit(MDIWindow *wnd, HWND hwnd, HTREEITEM hitem);
 	
    /// Handles resize of editor tab
    /**
    @returns void
    */
	void        Resize();

    /// Gets the editortab handle
    /**
    @returns Editor tab handle
    */
	HWND        GetHwnd();
	
    /// Gets the currently selected editor pointer.
    /**
    @returns pointer to the currently selected editor tab.
    */
	EditorBase* GetEditorBase();

    /// Shows/Hides all the content of the editor
    /**
    @param tabindex: IN currently selected tab index.
    @param status: IN wyTrue/wyfalse
    @returns void
    */
	void        ShowTabContent(wyInt32 tabindex, wyBool status);

    /// Sets the parent connection window pointer
    /**
    @param wnd: IN the connection window pointer to set.
    @return wyBool wytrue if it is SUCCESS, otherwise failure.
    */
	wyBool      SetParentPtr(MDIWindow *wnd);

    /// Get the parent connection window pointer
    /**
    @returns parent connection window pointer.
    */
	MDIWindow   *GetParentPtr();

    /// Closes the Tab
    /**
    @param index: IN tabindex to close
    @return wyBool wytrue if it is SUCCESS, otherwise failure.
    */
	wyBool      CloseTab(wyInt32 index);

    /// Sets the tab font.
    /**
    It will take the font details from the .ini file and sets the font.
    @returns void
    */
	void        SetTabFont();

    /// Sets the tab font color.
    /**
    It will take the font color details from the .ini file and sets the font color.
    @returns void
    */
	void        SetTabFontColor();

    /// Parent connection window pointer
	MDIWindow *			m_parentptr;

    /// Current EditorTab handle
	HWND				m_hwnd;

    /// Parent window handle
    HWND                m_hwndparent;
};

#endif
