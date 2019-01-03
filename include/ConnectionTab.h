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

#ifndef __CONNECTIONTAB_H__
#define __CONNECTIONTAB_H__

#include "MDIWindow.h"
#include "TabTypes.h"
#include "TabModule.h"
#include "Global.h"
#include "TabMgmt.h"
#include "CustTab.h"

extern	PGLOBALS pGlobals;

class TabTypes;
class TabMgmt;
class MDIWindow;

class ConnectionTab
{

public:

	///Constructor
	ConnectionTab();

	///Destructor
	~ConnectionTab();
	
	/// To create connection tab 
    /**
    @param hwnd			: IN frame window handle
	@returns tab handle
    */
	HWND		CreateConnectionTabControl(HWND hwnd);

	/// Tab window procedure
	/**
	@param hwnd     : IN Window HANDLE
	@param message  : IN Window message
	@param wparam   : IN Unsigned int message parameter
	@param lparam   : IN Long message parameter
	@returns long pointer
	*/
	static		LRESULT CALLBACK TabWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, wyBool *pishandled);
	

	/// Handles on Tab Closed, deletes the tab
	/*
	@param hwndtab  : IN connetiontab HANDLE
	@returns VOID
	*/
	VOID		DeleteConnectionTabItem(HWND hwndtab);

	/// To insert a connection tab item 
    /**
    @param hwndtab	: IN tab handle
	@param index	: IN Window message
	@param image	: IN image for the tab
	@param title	: IN tab title
	@param lparam   : IN Long message parameter
	@returns tab handle
    */
	wyBool      InsertConnectionTab(wyString *title, LPARAM lparam, wyBool iscon_res = wyFalse,ConnectionInfo* conninfo = NULL);

	/// To get active window handle
	/*
	@param hwndtab  : IN connetiontab HANDLE
	@returns tab handle
	*/
	HWND		GetActiveWindowHandle(HWND hwndtab);

	//void insert(wyString tabname, opentabs *head);

    /// To get tab index for active window
	/*
	@param hwndtab  : IN connetiontab HANDLE
	@param hwnd     : IN Window HANDLE
	@returns tab index 
	*/
	wyInt32		GetActiveTabIndex(HWND hwndtab, HWND hwndwin);

	/// To change the color of tab
	/*
	@param tabcolor  : IN tabcolor
	@param textcolor  : IN textcolor
	@returns wyTrue if success else wyFalse
	*/
	wyBool		ChangeTabColor(COLORREF  tabcolor, COLORREF  textcolor);

	/// To select active window
	/*
	@param index  : IN tab index
	@returns wyTrue if success else wyFalse
	*/
	wyBool		SelectActiveConnection(wyInt32 index);


	//tab handle
	HWND		m_hwndtab;

	//HWND		m_hwndtooltip;

};
#endif

