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

#ifndef __TABTYPES__ 
#define __TABTYPES__
#include "datatype.h"

#define TABCOLUMNS  0
#define TABINDEXES  1
#define TABFK       2
#define TABADVANCED 3
#define TABPREVIEW  4


class TabModule;
class TabEditorSplitter;
class EditorBase;
class TabMgmt;
class TableTabInterfaceTabMgmt;

//Abstract class for all Tab types
class TabTypes
{

public:
	
	/// Constructor with parent window handle.
    TabTypes(HWND hwnd);
	
	///Destructor
	/**
    Free up all the allocated resources
    */
	virtual ~TabTypes();
	
	///Function to resize the Tab box.
	/**
    @returns void
    */
	virtual VOID	Resize(wyBool issplittermoved = wyFalse)	=  0 ;

	
	/// Gets the window HANDLE
    /**
	@returns window handle
    */
	HWND			GetHwnd();


	/// Sets the parent window pointer
	/**
	@param parentptr	: IN EditorTab object pointer
    */
    wyBool			SetParentPtr(TabModule * parentptr);

	
	/// Get the parent connection window pointer
    /**
    @returns parent connection window pointer.
    */
	TabModule		*GetParentPtr();

	/// Closes the Tab
    /**
    @param index: IN tabindex to close
	@param status : IN wyTrue/wyfalse
    @return wyBool wytrue if it is SUCCESS, otherwise failure.
    */
	virtual wyBool      CloseTab(wyInt32 index) = 0;

	/// Shows/Hides all the content of the editor
    /**
    @param tabindex: IN currently selected tab index.
    @param status: IN wyTrue/wyfalse
    @returns void
    */
	virtual VOID	ShowTabContent(wyInt32 tabindex, wyBool status) = 0 ;

	/// Handles on tab selection change
	/*
	@returns VOID
	*/
	virtual	VOID		OnTabSelChange() = 0;

	/// Handles on tab selection changing.
	/*
	@returns VOID
	*/
	virtual	VOID		OnTabSelChanging() = 0;

	/// Handles on Tab Closing.
	/*
	@returns one on success else zero
	*/
	virtual	wyInt32		OnTabClosing(wyBool ismanual) = 0;

	/// Handles on  Tab Closed.
	/*
	@returns VOID
	*/
	virtual	VOID		OnTabClose() = 0;	

	/// HAndles when MDIWindow is closing.
	/*
	@returns one on success else zero
	*/
	virtual	wyInt32		OnWmCloseTab() = 0;

	///Handles all tab controls
	/*
	@param tabcount: IN total number of tabs in tabcontroller
	@param selindex: IN currently selected tab index.
    @returns void
	*/
	virtual VOID		HandleTabControls(wyInt32 tabcount, wyInt32 selindex) = 0;

	virtual VOID		HandleFlicker() = 0;

    virtual void        SetBufferedDrawing(wyBool isset);

    virtual void        OnGetChildWindows(wyInt32 tabcount, LPARAM lparam);

	/// Parent TabModule pointer
	TabModule			*m_parentptr;

	/// EditorQuery object pointer
	EditorBase			*m_peditorbase;

	/// TabMgmt Object Pointer
	TabMgmt				*m_pctabmgmt;
   	
    //TableTabInterfaceTabMgmt Object pointer
    //TableTabInterfaceTabMgmt	*m_ptabinterfacetabmgmt;
   	
	/// Horizontal splitter object pointer
	TabEditorSplitter	*m_pcetsplitter;

	/// Current Tab handle
	HWND				m_hwnd;

    /// Parent window handle
    HWND				m_hwndparent;

	/// Flag to Check result in Edit or Grid form.
	wyBool				m_istextresult;

	/// Flag to check the edit Query editor window is hidden or not.
	wyBool				m_iseditwnd;

	/// Flag to check the Result window (TabMgmt) is hidden or not.
	wyBool				m_isresultwnd;
};

#endif

