/* Copyright (C) 2013 Webyog Inc

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

#include <string.h>
#include <stdio.h>
#include <windowsx.h>

#include "ConnectionTab.h"
#include "FrameWindowHelper.h"
#include "Global.h"
#include "MDIWindow.h"  
#include "TabMgmt.h"
#include "CustTab.h"

#include <scilexer.h>

#define	SIZE_30	        30
#define	SIZE_15	        15
#define	SIZE_80			80


extern	PGLOBALS		pGlobals;

ConnectionTab::ConnectionTab()
{
	pGlobals->m_connectiontabnamelist = new List;
	pGlobals->m_mdilistfordropdown = new List;
}

ConnectionTab::~ConnectionTab()
{
}

//To create connection tab 
HWND
ConnectionTab::CreateConnectionTabControl(HWND hwnd)
{
	HWND            hwndtab;
    TABCOLORINFO    ci = {0};
	
	hwndtab = CreateCustomTab(hwnd, 0, 0, 0, 0, TabWndProc, (LPARAM)IDC_CONNECTIONTAB);
	CustomTab_IsFixedLength(hwndtab, wyTrue);
	CustomTab_EnableDrag(hwndtab, pGlobals->m_pcmainwin->m_hwndmain, wyTrue);
    CustomTab_EnableAddButton(hwndtab, wyTrue);
	CustomTab_SetClosable(hwndtab, wyTrue);

    if(wyTheme::GetTabColors(COLORS_CONNTAB, &ci))
    {
        CustomTab_SetColorInfo(hwndtab, &ci);
    }
    else
    {
        ci.m_mask = CTCF_BOTTOMLINE | CTCF_SELTABFG2;
        ci.m_seltabfg2 = GetSysColor(COLOR_BTNFACE);
        CustomTab_SetColorInfo(hwndtab, &ci);
    }

	MoveWindow(hwndtab, 0, 0, 0, 0, TRUE);
	UpdateWindow(hwndtab);

	return hwndtab;
}

//Tab window procedure
LRESULT CALLBACK  
ConnectionTab::TabWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, wyBool *pishandled)
{
    *pishandled = wyFalse;
	return 0;
}

//Handles on Tab Closed, deletes the tab
void 
ConnectionTab::DeleteConnectionTabItem(HWND hwnd)
{
	wyInt32  itemindex;

	if(CustomTab_GetItemCount(hwnd) < 1)
	{
		return;	
	}

	itemindex = CustomTab_GetCurSel(hwnd);    
	CustomTab_DeleteItem(hwnd, itemindex);

	//MoveWindow(hwnd, 0, 0, 0, 0, TRUE);
	//UpdateWindow(hwnd);

	//pGlobals->m_pcmainwin->ResizeToolBar();

}


//To insert a connection tab item 
wyBool
ConnectionTab::InsertConnectionTab(wyString * title, LPARAM lparam, wyBool iscon_res,ConnectionInfo* conninfo)
{
	wyInt32			count, ret;
	HWND			 hwndtab;
	unsigned long	tempcolor = -1;
	MDIWindow		*wnd = GetActiveWin();
	TabModule * hwndTabModule = wnd->m_pctabmodule;
	wyString		tabname,t1;

	if(!wnd)
	{
		return wyFalse;
	}

	hwndtab = pGlobals->m_pcmainwin->m_hwndconntab;
	if (hwndtab)
	{
		tabname.Sprintf(" %s", wnd->m_title.GetString());

		ListofOpenTabs *tabdetails = new ListofOpenTabs;

		//set the node of global m_connectiontabnamelist
		tabdetails->name.SetAs(wnd->m_title.GetString());
		tabdetails->m_hwndTabModuleinlist = hwndTabModule;
		pGlobals->m_connectiontabnamelist->Insert(tabdetails);
	}


	CTCITEM				item = {0};
	item.m_psztext    = (wyChar*)tabname.GetString();
	item.m_cchtextmax = tabname.GetLength();
	item.m_mask       = CTBIF_TEXT | CTBIF_LPARAM | CTBIF_CMENU | CTBIF_COLOR | CTBIF_TOOLTIP;
	item.m_lparam	  = lparam;

	item.m_tooltiptext = (wyChar*)title->GetString();
	

	//if connection color is there
	if(!iscon_res)
	{
		if(wnd->m_conninfo.m_rgbconn  >= 0)
		{
			//if color is -1, then set color as system color because
			//TreeView_SetBkColor will take -1 as system color
			if(wnd->m_conninfo.m_rgbconn == tempcolor)
			{
				item.m_color = GetSysColor(COLOR_WINDOW);
				item.m_fgcolor = item.m_color^0xFFFFFF;
			}
			else
			{
				item.m_color = wnd->m_conninfo.m_rgbconn;
				item.m_fgcolor = wnd->m_conninfo.m_rgbfgconn;
			}
		}
		else
		{
			item.m_color = COLOR_WHITE;
			item.m_fgcolor = RGB(0,0,0);
		}
	}
	else
	{
		item.m_color = pGlobals->m_pcmainwin->m_connection->m_rgbobbkcolor;
		item.m_fgcolor = pGlobals->m_pcmainwin->m_connection->m_rgbobfgcolor;
	}
	count = CustomTab_GetItemCount(hwndtab);
	
	VERIFY((ret = CustomTab_InsertItem(hwndtab, count, &item))!= -1);

	 if(ret == -1)
	 {
        return wyFalse;
	 }

	
	CustomTab_SetCurSel(hwndtab, count);

    count = CustomTab_GetItemCount(hwndtab);

	CustomTab_EnsureVisible(hwndtab, count - 1);

	ShowWindow(hwndtab, TRUE);	

	pGlobals->m_pcmainwin->ResizeToolBar();
    pGlobals->m_pcmainwin->ResizeMDIWindow();

	InvalidateRect(hwndtab, NULL, TRUE);
	UpdateWindow(hwndtab);

	if(count == 1)
	{
		CreateCustomTabTooltip(hwndtab);
	}

	return wyTrue;
}

//To get active tab
HWND
ConnectionTab::GetActiveWindowHandle(HWND hwndtab)
{
	CTCITEM				item  = {0};
	wyInt32				itemindex;
	MDIWindow			*wnd;

    item.m_mask       = CTBIF_LPARAM | CTBIF_COLOR;

	
	itemindex	 =	CustomTab_GetCurSel(hwndtab);
	
	if(itemindex > -1)
	{
		CustomTab_GetItem(hwndtab, itemindex, &item);

		wnd = (MDIWindow *)item.m_lparam;

		if((!wnd) || (!wnd->m_hwnd))
		{
			return NULL;
		}

		return wnd->m_hwnd;	
	}
	else
		return NULL;	
}

//gets the tab index for the active window
wyInt32 
ConnectionTab::GetActiveTabIndex(HWND hwndtab, HWND hwndwin)
{
	wyInt32			index;
	wyInt32			tabcount = 0;
	MDIWindow		*wnd;
	CTCITEM			item;

	item.m_mask     = CTBIF_LPARAM | CTBIF_COLOR;

	tabcount		= CustomTab_GetItemCount(hwndtab);

	for(index = 0; index <= tabcount; index++)
	{
		CustomTab_GetItem(hwndtab, index, &item);

		//get tab handle
		wnd = (MDIWindow *)item.m_lparam;

		if((!wnd) || (!wnd->m_hwnd))
		{
			return -1;
		}

		//check if tab handle is same as window handle
		if(wnd->m_hwnd == hwndwin)
		{
			return index;
		}
	}

	return -1;
}

//change the tab color, when changed from object browser
wyBool
ConnectionTab::ChangeTabColor(COLORREF  tabcolor, COLORREF  textcolor)
{
	HWND		hwndtab;
	wyInt32		itemindex;
	CTCITEM			item;
	RECT rect	= { 0 };


	item.m_mask     = CTBIF_COLOR;

	hwndtab = pGlobals->m_pcmainwin->m_hwndconntab;

	itemindex = CustomTab_GetCurSel(hwndtab);

	item.m_color  = tabcolor;
	item.m_fgcolor  = textcolor;

	CustomTab_SetItemColor(hwndtab, itemindex, &item);

	GetClientRect(hwndtab, &rect);
	
	InvalidateRect(hwndtab, &rect, TRUE);

	UpdateWindow(hwndtab);

	return wyTrue;
}

 // select 1 to 8 and then last connections, on Ctrl+1,2....9
wyBool
ConnectionTab::SelectActiveConnection(wyInt32 index)
{
	wyInt32 count, tabid = 0;
	HWND    hwndtab;

	hwndtab = pGlobals->m_pcmainwin->m_hwndconntab;

	count = CustomTab_GetItemCount(hwndtab);

	//Set the flag of custom WM_MDINEXT message to true
	pGlobals->m_iscustomwmnext  = wyTrue;

	//if Ctrl+9, then select last connection
	if(index == ACCEL_NINTHCONN)
	{
		CustomTab_SetCurSel(hwndtab, count - 1, 1);
	}
	else
	{
		//select 1 to 8 connection on Ctrl+1,2....8
		if((index < ACCEL_NINTHCONN) && (index >= ACCEL_FIRSTCONN))
		{
			tabid = index - ACCEL_FIRSTCONN;
		}

		if(count >= tabid)
		{
			CustomTab_SetCurSel(hwndtab, tabid, 1);
		}
	}

	//Set the flag of custom WM_MDINEXT message to false
	pGlobals->m_iscustomwmnext  = wyFalse;

	return wyTrue;
}