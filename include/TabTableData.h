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

#include "TabTypes.h"
#include "TableView.h"

class TableView;
class MySQLTableDataEx;
class MDIWindow;

#ifndef _TABTABLEDATA_H_
#define _TABTABLEDATA_H_

class TabTableData : public TabTypes
{
public:
	TabTableData(void);
	
	TabTableData(MDIWindow *wnd, HWND hwnd, wyBool issticky);
	
	~TabTableData(void);

	void Create();

	wyInt32 CreateTab(wyBool issetfocus = wyTrue);

	void	Resize(wyBool issplittermoved = wyFalse);

	wyBool      CloseTab(wyInt32 index);

	VOID	ShowTabContent(wyInt32 tabindex, wyBool status);

	VOID		OnTabSelChange();

	VOID		OnTabSelChanging();

	wyInt32		OnTabClosing(wyBool ismanual);

	VOID		OnTabClose();

	wyInt32		OnWmCloseTab();

	VOID		HandleTabControls(wyInt32 tabcount, wyInt32 selindex);

	VOID		HandleFlicker();
	
	void        OnGetChildWindows(wyInt32 tabcount, LPARAM lparam);

	void GetCurrentSelection();

	wyBool ReExecute(wyInt32 action);

	wyBool FreeMySQLResources(wyBool isfreemyres);

	wyBool FreeItemData();

	static wyBool   FreeItemData(TABLEDATARES *tbldatares, wyBool isnewrow = wyTrue);

	wyBool		ExecuteTableDataQuery();

    void        SetBufferedDrawing(wyBool isset);

	HWND		m_hwndparent;

	MDIWindow		*m_pmdi;

	wyBool m_isautorefreshon;

	wyBool m_isonoption;

	Tunnel  *m_tunnel;

	wyBool m_isshowdata;

	MySQLTableDataEx	*m_tabledata;

	TableView			*m_tableview;

    wyBool              m_istabsticky;

    wyBool              m_isrefreshed;
};

#endif