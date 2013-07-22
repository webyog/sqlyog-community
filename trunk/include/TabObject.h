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


#ifndef _QueryTableInfo_H_
#define _QueryTableInfo_H_

#include "FrameWindowHelper.h"
#include "Global.h"
#include "FindAndReplace.h"
#include "ObjectInfo.h"

#ifndef COMMUNITY
class SchemaOptimizer;
class RedundantIndexFinder;
#endif

class FindAndReplace;
class ObjectInfo;

class TabObject : public TabTypes
{

	//friend class SchemaOptimizer;

public:

	TabObject(HWND hwnd, MDIWindow* wnd);
	~TabObject();

    /// Callback window procedure
    /**
    @param hwnd			: IN Window HANDLE
    @param message		: IN Window message
    @param wparam		: IN Unsigned message parameter
    @param lparam		: IN Long message parameter
	@returns long pointer
    */
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
	
    /// Gets the current window HANDLE
    /**
    @returns the window HANDLE
    */
	HWND		GetHwnd();

	wyInt32			CreateInfoTab(MDIWindow *wnd, HWND hwnd);
		
	void			OnSelectInfoTab(MDIWindow *wnd);

	wyBool			CloseTab(wyInt32 index);

	VOID	ShowTabContent(wyInt32 tabindex, wyBool status);

	VOID		OnTabSelChange();

	VOID		OnTabSelChanging();

	wyInt32		OnTabClosing(wyBool ismanual);

	VOID		OnTabClose();

	wyInt32		OnWmCloseTab();

	VOID		HandleTabControls(wyInt32 tabcount, wyInt32 selindex);

	VOID		HandleFlicker();
	
	void        OnGetChildWindows(wyInt32 tabcount, LPARAM lparam);

    wyBool      ShowInfoTab(HWND hwnd);

    void        Resize(wyBool issplittermoved);

    ObjectInfo* m_pobjinfo;
};

#endif
