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

#ifndef __TABEDITOR_H__
#define __TABEDITOR_H__

#include "MDIWindow.h"
#include "TabTypes.h"
#include "TabModule.h"
#include "Global.h"
#include "TabMgmt.h"

class TabTypes;
class TabMgmt;
class EditorBase;

class TabEditor : public TabTypes
{

public:
	/// Constructor & Destructor
	TabEditor(HWND hwnd);
	~TabEditor();
	
	/// Wrapper to create Editor tab and initializes with a default normal query editor
    /**
    @param wnd			: IN parent connection window pointer.
	@param hitem		: IN The HTREEITEM pointer.
	@param iseditquery  : IN flag to check whether for EditorQuery or EditorProc.
    @return wyBool wytrue if it is SUCCESS, otherwise failure.
    */
	wyBool		Create(MDIWindow * wnd, HTREEITEM hitem, wyBool iseditquery, wyString *strhitemname = NULL);

	/// Function to create the TabEditor custom tab
	/*
	@returns void
	**
	*/
	VOID		CreateTab();

	/// Tab window procedure
	/**
	@param hwnd     : IN Window HANDLE
	@param message  : IN Window message
	@param wparam   : IN Unsigned int message parameter
	@param wparam   : IN Long message parameter
	@returns long pointer
	*/
	static		LRESULT CALLBACK	TabWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	
	/// Create the horizontal splitter
    /**
    @param hwnd     : IN parent connection window pointer.
	@returns void
    */
	VOID		CreateTabEditorSplitter(MDIWindow *wnd);

	///Function to create the QueryEditor
	/**
    @param wnd			: IN parent connection window pointer.
    @return wyBool wytrue if it is SUCCESS, otherwise failure.
    */
	wyBool 		CreateEditorQuery(MDIWindow * wnd);

	/// Creates the Advanced editor
    /**
    @param wnd: IN parent connection window pointer.
    @param hwnd: IN handle to the parent editor tab.
    @param hitem: IN The HTREEITEM pointer, 
    @returns EditorProcs pointer, NULL if it fails
    */
	wyBool		CreateAdvEdit(MDIWindow *wnd, HTREEITEM hitem, wyString *strhitemname);

	/// Function to create the Result Tab
	/*
	@param wnd : IN connection window pointer.
    @returns VOID
	*
	*/
	VOID		CreateTabMgmt(MDIWindow * wnd);

	/// Handles resize of editor tab
    /**
	@param issplittermoved : IN flag sets while mving editor splitter (to reduce flickering)
    @returns void
    */
	VOID		Resize(wyBool issplittermoved = wyFalse);    
	
	/// Shows/Hides all the content of the editor
    /**
    @param tabindex: IN currently selected tab index.
    @param status: IN wyTrue/wyfalse
    @returns void
    */
	void		ShowTabContent(wyInt32 tabindex, wyBool status);

	/// Handles on tab selection change, if it is TabEditor 
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

	/// Function to handle 'Edit' menu items - CTRL+L, CTRL+2, CTRL+3
	/*
	@returns VOID
    */
	VOID		HandleEditMenu();


	/// Closes the Tab
    /**
    @param index: IN tabindex to close
	@param status : IN wyTrue/wyfalse
    @return wyBool wytrue if it is SUCCESS, otherwise failure.
    */
	wyBool      CloseTab(wyInt32 index);
	
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

     // virtual fun defined in Tabtypes for enbling/ disabling Menu items for particular tab

     /// Hndles the Menu Bar by select/deselect menu items for QueryBuilder
    /*
    @returns void
    */
     VOID                        HandleMenu() { };

     void       HandleNoFormatCopy();

};
#endif

