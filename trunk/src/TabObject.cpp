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

#include "Global.h"
#include "htmlayout.h"
#include "MDIWindow.h"
#include "TabObject.h"
#include "FrameWindowHelper.h"
#include "MySQLVersionHelper.h"
#include "ExportMultiFormat.h"
#include "EditorFont.h"
#include "scintilla.h"
#include "SQLMaker.h"
#include "GUIHelper.h"
#include "Htmlrender.h"

#ifndef COMMUNITY
#include "HelperEnt.h"
#endif

extern	PGLOBALS		pGlobals;

TabObject::TabObject(HWND hwndparent, MDIWindow* wnd):TabTypes(hwndparent)
{
    m_pobjinfo = new ObjectInfo(wnd, hwndparent);
}

TabObject::~TabObject()
{   	
    delete m_pobjinfo;	
}

wyInt32
TabObject::CreateInfoTab(MDIWindow * wnd, HWND hwnd)
{
    wyInt32				count = 0;
	CTCITEM				item = {0};
	wyString			buffer;
	
	// get the number of tabs
    count = CustomTab_GetItemCount(hwnd);

	item.m_psztext    = _("Info");
	item.m_cchtextmax = strlen(_("Info"));
	item.m_mask       = CTBIF_IMAGE | CTBIF_TEXT | CTBIF_LPARAM | CTBIF_CMENU  | CTBIF_TOOLTIP;
	item.m_iimage     = IDI_TABLEINDEX;
	item.m_tooltiptext = _("Info");
	item.m_lparam     = (LPARAM)this;

	//m_hwndparent = hwnd;
	
	SendMessage(m_hwndparent, WM_SETREDRAW, FALSE, 0);
	
    m_pobjinfo->Create();

	CustomTab_SetItemLongValue(hwnd, count, (LPARAM)this);
    CustomTab_InsertItem(hwnd, count, &item);
	CustomTab_EnsureVisible(hwnd, count, wyTrue);
	return count;
}

void
TabObject::OnSelectInfoTab(MDIWindow* wnd)
{
    if(wnd->m_pctabmodule->GetActiveTabImage() != IDI_TABLEINDEX)
	{
		return;
	}

    m_pobjinfo->Refresh();
}

wyBool
TabObject::ShowInfoTab(HWND hwnd)
{
	wyInt32 i, count;
	CTCITEM item = {0};
	
    count = CustomTab_GetItemCount(m_hwndparent);
	
	for(i = 0; i < count; i++)
	{
		item.m_mask = CTBIF_IMAGE;
		CustomTab_GetItem(m_hwndparent, i, &item);
		if(item.m_iimage == IDI_TABLEINDEX)
		{
			CustomTab_SetCurSel(m_hwndparent, i);
            CustomTab_EnsureVisible(m_hwndparent, i);
			return wyTrue;
		}
	}
	return wyFalse;
}

// Function to resize the edit box. It resizes itself with respect to its parent window 
void
TabObject::Resize(wyBool issplittermoved)
{
    m_pobjinfo->Resize();
}

HWND
TabObject::GetHwnd()
{
	return m_hwnd;
}

wyBool
TabObject::CloseTab(wyInt32 index)
{
	return wyTrue;
}

VOID
TabObject::ShowTabContent(wyInt32 tabindex, wyBool status)
{
    m_pobjinfo->Show(status);	
    SetFocus(m_pobjinfo->GetActiveDisplayWindow());
}

VOID		
TabObject::OnTabSelChange()
{
    if(pGlobals->m_pcmainwin->m_finddlg)
	{
		DestroyWindow(pGlobals->m_pcmainwin->m_finddlg);
		pGlobals->m_pcmainwin->m_finddlg = NULL;
	}

    if(m_pobjinfo->m_wnd->m_ismdiclosealltabs == wyTrue)
    {
        return;
    }

    OnSelectInfoTab(m_pobjinfo->m_wnd);
	SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)ID_OBJECT_INSERTUPDATE, TBSTATE_ENABLED);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ID_OBJECT_VIEWDATA, TBSTATE_ENABLED);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ID_TABLE_OPENINNEWTAB, TBSTATE_ENABLED);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ACCEL_EXECUTEALL,(LPARAM)TBSTATE_INDETERMINATE);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)IDM_EXECUTE,(LPARAM)TBSTATE_INDETERMINATE);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ACCEL_QUERYUPDATE ,(LPARAM)TBSTATE_INDETERMINATE); 
	SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)ID_FORMATCURRENTQUERY,(LPARAM)TBSTATE_INDETERMINATE);
	
    if(m_pobjinfo->m_wnd)
    {
        m_pobjinfo->m_wnd->SetQueryWindowTitle();
    }
}

VOID		
TabObject::OnTabSelChanging()
{
}

wyInt32		
TabObject::OnTabClosing(wyBool ismanual)
{
    if(ismanual == wyTrue)
    {
        GetTabOpenPersistence(IDI_TABLEINDEX, wyTrue);
    }

	return 1;
}

VOID		
TabObject::OnTabClose()
{
}

wyInt32		
TabObject::OnWmCloseTab()
{
	return 0;
}

VOID		
TabObject::HandleTabControls(wyInt32 tabcount, wyInt32 selindex)
{
}

VOID		
TabObject::HandleFlicker()
{
}
	
void        
TabObject::OnGetChildWindows(wyInt32 tabcount, LPARAM lparam)
{
}
