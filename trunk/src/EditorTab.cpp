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

/*
#include <string.h>
#include <stdio.h>

#include "EditorTab.h"
#include "FrameWindowHelper.h"
#include "LogMgmt.h"
#include "EditorFont.h"
//stk
#include "TabMgmt.h"
//stk

#include <scilexer.h>

#define IDC_CTAB		WM_USER+116
extern	PGLOBALS		pGlobals;

EditorTab::EditorTab(HWND hwnd)
{
	m_hwndparent = hwnd;
}


EditorTab::~EditorTab()
{
}

wyBool 
EditorTab::Create(MDIWindow * wnd)
{
	CreateEditorTab(m_hwndparent);
	CreateQueryEditorTab(wnd);
	return wyTrue;
}

HWND
EditorTab::CreateEditorTab(HWND hwnd)
{
	HWND hwndtab;

	VERIFY(hwndtab	= CreateCustomTab(hwnd, 0, 0, 0, 0,(CTBWNDPROC)TabWndProc, IDC_CTAB));
	MoveWindow(hwndtab, 0, 0, 0, 0, TRUE);
	m_hwnd = hwndtab;

	return hwndtab;
}

//stk
//wyBool
//EditorTab::CreateTabMgmt(

//stk

LRESULT CALLBACK 
EditorTab::TabWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return 1;
}

wyBool 
EditorTab::CreateQueryEditorTab(MDIWindow * wnd)
{
	EditorQuery		*pcqueryedit;
	wyInt32			ret, count;
	CTCITEM			item = {0};

	item.m_psztext    = "Query";
	item.m_cchtextmax = _tcslen("Query");
	item.m_mask       = CTBIF_IMAGE | CTBIF_TEXT | CTBIF_LPARAM | CTBIF_CMENU;
	item.m_iimage     = IDI_QUERY;

	pcqueryedit = CreateQueryEdit(wnd, m_hwnd);
	
	item.m_lparam =(LPARAM)pcqueryedit;

	ShowWindow(pcqueryedit->m_hwnd, FALSE);	

	count = CustomTab_GetItemCount(m_hwnd);

	VERIFY((ret = CustomTab_InsertItem(m_hwnd, count, &item))!= -1);

	count = CustomTab_GetItemCount(m_hwnd);

	if(count > 1)
		CustomTab_EnsureVisible(m_hwnd, count - 1);

	ShowWindow(m_hwnd, TRUE);			
	this->Resize();
	
	return wyTrue;
}

EditorQuery * 
EditorTab::CreateQueryEdit(MDIWindow * wnd, HWND hwndtab)
{
	EditorQuery * pcqueryedit = new EditorQuery(hwndtab);

	pcqueryedit->Create(wnd);
	pcqueryedit->SetParentPtr(this);

	return pcqueryedit;
}

wyBool 
EditorTab::CreateAdvEditorTab(MDIWindow *wnd, wyChar* title, wyInt32 image, HTREEITEM hitem)
{
	EditorProcs		*pcadvedit;
	wyInt32			ret, count;
	CTCITEM			item = {0};

	item.m_psztext    = title;
	item.m_cchtextmax = _tcslen(title);
	item.m_mask       = CTBIF_IMAGE | CTBIF_TEXT | CTBIF_LPARAM | CTBIF_CMENU;
	item.m_iimage     = image;

	pcadvedit = CreateAdvEdit(wnd, m_hwnd, hitem);
	pcadvedit->SetAdvancedEditor(wyTrue);

	item.m_lparam =(LPARAM)pcadvedit;
	ShowWindow(pcadvedit->m_hwnd, FALSE);
	
	count = CustomTab_GetItemCount(m_hwnd);
	VERIFY((ret = CustomTab_InsertItem(m_hwnd, count, &item))!= -1);
	count = CustomTab_GetItemCount(m_hwnd);

	if(count > 1)	
	    CustomTab_EnsureVisible(m_hwnd, count - 1);
	
	ShowWindow(m_hwnd, TRUE);		
	this->Resize();
	
	return wyTrue;
}



EditorProcs * 
EditorTab::CreateAdvEdit(MDIWindow *wnd, HWND hwndtab, HTREEITEM hitem)
{
	EditorProcs * pcadvedit = new EditorProcs(hwndtab);

	pcadvedit->Create(wnd, hitem);
	pcadvedit->SetParentPtr(this);

	return pcadvedit;
}


void
EditorTab::Resize()
{
	RECT	    rcmain, rcvsplitter, rchsplitter;
	wyInt32     hpos, vpos, width, height;
    wyInt32     tabcount, selindex;
    CTCITEM	    item;

	MDIWindow       *pcquerywnd	        =(MDIWindow*)GetWindowLong(m_hwndparent, GWL_USERDATA);
	FrameWindowSplitter  *pcqueryvsplitter   = pcquerywnd->GetVSplitter();
	FrameWindowSplitter  *pcqueryhsplitter   = pcquerywnd->GetHSplitter();
	
	VERIFY(GetClientRect(m_hwndparent, &rcmain));

	VERIFY(GetWindowRect(pcqueryvsplitter->GetHwnd(), &rcvsplitter));
	VERIFY(GetWindowRect(pcqueryhsplitter->GetHwnd(), &rchsplitter));

	VERIFY(MapWindowPoints(NULL, m_hwndparent,(LPPOINT)&rcvsplitter, 2));
	VERIFY(MapWindowPoints(NULL, m_hwndparent,(LPPOINT)&rchsplitter, 2));

	hpos			=	(rcvsplitter.right);
	vpos			=	5;
	width			=	(rcmain.right - hpos) - 2;

	if(pcquerywnd->m_iseditwindow == wyTrue)
		height		=	(rchsplitter.top - rcvsplitter.top) - 3;
	else
		height		=	0;

	SendMessage(m_hwnd, WM_SETREDRAW,  FALSE, NULL);

	VERIFY(MoveWindow(m_hwnd, hpos, vpos, width, height, TRUE));

    SendMessage(m_hwnd, WM_SETREDRAW, TRUE, NULL);
	
    item.m_mask = CTBIF_LPARAM;

	selindex = CustomTab_GetCurSel(m_hwnd);

	for(tabcount = 0; tabcount < CustomTab_GetItemCount(m_hwnd); tabcount++)
	{
		CustomTab_GetItem(m_hwnd, tabcount, &item);
		EditorBase *eb =(EditorBase *)item.m_lparam;
		eb->Resize();		

        pGlobals->m_pcmainwin->m_connection->HandleEditorControls(eb->m_hwnd,
            eb->m_hwndhelp, eb->m_hwndfilename, eb->m_save, tabcount, selindex);
	}
			
	InvalidateRect(m_hwnd, NULL, TRUE);

	return;
}

HWND
EditorTab::GetHwnd()
{
	return m_hwnd;
}

EditorBase * 
EditorTab::GetEditorBase()
{
	CTCITEM         item;
	EditorBase	    *pceditorbase;
	wyInt32         itemindex;

    item.m_mask = CTBIF_LPARAM;

	itemindex = CustomTab_GetCurSel(m_hwnd);
	CustomTab_GetItem(m_hwnd, itemindex, &item);

	pceditorbase = (EditorBase *)item.m_lparam;

	return pceditorbase;
}


void 
EditorTab::ShowTabContent(wyInt32 tabindex, wyBool status)
{
	CTCITEM     item;
    EditorBase	*pceditorbase;
    BOOL        val = (status == wyTrue)?TRUE:FALSE;

    item.m_mask = CTBIF_LPARAM;
	
	CustomTab_GetItem( m_hwnd, tabindex, &item);
	pceditorbase = (EditorBase *)item.m_lparam;
ShowWindow ( pceditorbase->m_hwnd, val );


	
	ShowWindow ( pceditorbase->m_hwndfilename, val );

	if ( val )
		SetFocus ( pceditorbase->m_hwnd );

	return;
}

wyBool 
EditorTab::SetParentPtr(MDIWindow * qw)
{
	m_parentptr = qw;
	return wyTrue;
}


MDIWindow * 
EditorTab::GetParentPtr()
{
	return m_parentptr;
}


wyBool 
EditorTab::CloseTab(wyInt32 index)
{
	CTCITEM item;
    EditorBase *peditorbase; 

        item.m_mask = CTBIF_LPARAM;

	CustomTab_GetItem(m_hwnd , index, &item);
	peditorbase = (EditorBase *)item.m_lparam;

    
    if(peditorbase)
    {
        if(peditorbase->m_hwnd)
			VERIFY(DestroyWindow ( peditorbase->m_hwnd ));

		peditorbase->m_hwnd = NULL;

		if(peditorbase->m_hwndfilename)
			VERIFY(DestroyWindow ( peditorbase->m_hwndfilename ));

		peditorbase->m_hwndfilename = NULL;

        delete peditorbase;

		peditorbase = NULL;
    }
    
	return wyTrue;
}


void  
EditorTab::SetTabFont()
{
	CTCITEM			item;
	EditorBase	    *pceditorbase;
	wyInt32	        itemindex, totalitems;

	totalitems = CustomTab_GetItemCount(m_hwnd);

    item.m_mask = CTBIF_LPARAM;

	for(itemindex = 0; itemindex < totalitems; itemindex++)
	{
		CustomTab_GetItem(m_hwnd, itemindex, &item);
		pceditorbase =(EditorBase *)item.m_lparam;
		EditorFont::SetFont(pceditorbase->m_hwnd, "EditFont", wyTrue);
	}
}

void  
EditorTab::SetTabFontColor()
{
	CTCITEM         item;
	EditorBase	    *pceditorbase;
	wyInt32	        itemindex, totalitems;

	totalitems = CustomTab_GetItemCount(m_hwnd);

    item.m_mask = CTBIF_LPARAM;

	for(itemindex = 0; itemindex < totalitems; itemindex++)
	{
		CustomTab_GetItem(m_hwnd , itemindex, &item);
		pceditorbase =(EditorBase *)item.m_lparam;
		EditorFont::SetColor(pceditorbase->m_hwnd, wyTrue);
		EditorFont::SetWordWrap(pceditorbase->m_hwnd);
	}
}

