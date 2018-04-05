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

#include <wchar.h>
#include "MDIWindow.h"
#include "Global.h"
#include "FrameWindowHelper.h"
#include "CommonHelper.h"
#include "GUIHelper.h"
#include "TabQueryHistory.h"
#include "EditorFont.h"

extern	PGLOBALS		pGlobals;

TabQueryHistory::TabQueryHistory(MDIWindow* wnd, HWND hwndparent) : TabQueryTypes(wnd, hwndparent)
{
    m_origproc = NULL;
    m_hwndedit = NULL;
    m_referencedptr = NULL;
    m_findreplace = NULL;
}

TabQueryHistory::~TabQueryHistory()
{
    DestroyWindow(m_hwndedit);

    if(m_referencedptr)
    {
        *m_referencedptr = NULL;
    }
}

void
TabQueryHistory::OnTabSelChange(wyBool isselected)
{
    ShowWindow(m_hwndedit, isselected == wyTrue ? SW_SHOW : SW_HIDE);
    
    if(isselected == wyTrue)
    {
        SetFocus(m_hwndedit);
        UpdateStatusBar(m_pmdi->m_pcquerystatus);
    }
}

void
TabQueryHistory::OnTabSelChanging()
{
}

void
TabQueryHistory::Resize()
{
    RECT rect;

    GetClientRect(m_hwndparent, &rect);
    rect.top = CustomTab_GetTabHeight(m_hwndparent);
    SetWindowPos(m_hwndedit, NULL, 0, rect.top, rect.right, rect.bottom - rect.top, SWP_NOZORDER);
}

void
TabQueryHistory::UpdateStatusBar(StatusBarMgmt* pmgmt)
{
    pmgmt->AddTickCount(NULL, 0, 0, wyTrue);
    pmgmt->AddLineColNum(m_hwndedit, wyTrue);
    pmgmt->AddNumRows(0, wyTrue);
}

void
TabQueryHistory::SetBufferedDrawing(wyBool isset)
{
}

void
TabQueryHistory::SetFont()
{
    EditorFont::SetFont(m_hwndedit, "HistoryFont", wyTrue);
}

void
TabQueryHistory::SetColor()
{
    EditorFont::SetColor(m_hwndedit, wyTrue);
}

void
TabQueryHistory::Create()
{
    void* pdochistory;

    m_hwndedit = CreateWindowEx(0, 
                                L"Scintilla", 
                                L"Source", 
                                WS_CHILD | WS_VSCROLL | WS_HSCROLL, 
                                0, 0, 0, 0,
                                m_hwndparent,
                                (HMENU)IDC_HISTORY, 
                                pGlobals->m_pcmainwin->GetHinstance(), 
                                NULL);

    SendMessage(m_hwndedit, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
    SetScintillaModes(m_hwndedit, m_pmdi->m_keywordstring, m_pmdi->m_functionstring, wyFalse, wyTrue, "LineMySQL");
	SendMessage(m_hwndedit, SCI_SETWRAPMODE, SC_WRAP_NONE, SC_WRAP_NONE);
    SendMessage(m_hwndedit, SCI_SETSCROLLWIDTHTRACKING, (WPARAM)1, 0);
    SendMessage(m_hwndedit, SCI_SETSCROLLWIDTH, 10, 0);
    SendMessage(m_hwndedit, SCI_SETMODEVENTMASK, SC_MOD_INSERTTEXT, 0);
    SendMessage(m_hwndedit, SCI_USEPOPUP, 0, 0);
    
    SetFont();
    SetColor();
    m_origproc =(WNDPROC)SetWindowLongPtr(m_hwndedit, GWLP_WNDPROC,(LONG_PTR)TabQueryHistory::EditProc);	
    SetWindowLongPtr(m_hwndedit, GWLP_USERDATA,(LONG_PTR)this);
    Resize();

    pdochistory = (void*)SendMessage(m_pmdi->m_pctabmodule->m_pctabhistory->m_hwnd, SCI_GETDOCPOINTER, 0, 0);
    SendMessage(m_hwndedit, SCI_SETDOCPOINTER, 0, (LPARAM)pdochistory);
    SendMessage(m_hwndedit, SCI_GOTOLINE, SendMessage(m_hwndedit, SCI_GETLINECOUNT, 0, 0), 0);
}

LRESULT CALLBACK 
TabQueryHistory::EditProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    TabQueryHistory* ptqhistory = (TabQueryHistory*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if(message == (wyUInt32)pGlobals->m_pcmainwin->m_findmsg) 
	{		
		if(ptqhistory->m_findreplace && ptqhistory->m_findreplace->FindReplace(hwnd, lparam) == wyFalse)
		{
			delete(ptqhistory->m_findreplace);
			ptqhistory->m_findreplace = NULL;
		}

		return 0;		
	}    

    switch(message)
    {
        case WM_CONTEXTMENU:
            if(TabHistory::OnContextMenu(lparam, hwnd, &ptqhistory->m_hmenu) == 1)
            {
                return 1;
            }

	        break;

        case WM_COMMAND:
            TabHistory::OnWmCommand(hwnd, wparam);
	        break;

        case WM_HELP:
	        ShowHelp("http://sqlyogkb.webyog.com/article/79-history-tab");
	        return 1;

        case WM_SETFOCUS:
        case WM_KEYUP:
        case WM_LBUTTONUP:
            PostMessage(ptqhistory->m_hwndparent, UM_SETSTATUSLINECOL, (WPARAM)hwnd, 1);
	        break;

        case WM_KILLFOCUS:
            PostMessage(ptqhistory->m_hwndparent, UM_SETSTATUSLINECOL, (WPARAM)NULL, 0);
            break;

        case WM_KEYDOWN:
	        if(HandleScintillaStyledCopy(hwnd, wparam) == wyTrue)
            {
		        return 1;
            }

	        break;

        case WM_MEASUREITEM:
            return wyTheme::MeasureMenuItem((LPMEASUREITEMSTRUCT)lparam, ptqhistory->m_hmenu);

        case WM_DRAWITEM:		
            return wyTheme::DrawMenuItem((LPDRAWITEMSTRUCT)lparam);	
    }

    return CallWindowProc(ptqhistory->m_origproc, hwnd, message, wparam, lparam);
}