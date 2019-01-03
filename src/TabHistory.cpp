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


#include "scintilla.h"
#include "MDIWindow.h"
#include "Global.h"
#include "TabHistory.h"
#include "FrameWindowHelper.h"
#include "ExportMultiFormat.h"
#include "Editorfont.h"
#include "GUIHelper.h"
#include "CommonHelper.h"

#define UM_ADDTEXT      WM_USER + 201

extern	PGLOBALS		pGlobals;

TabHistory::TabHistory(HWND hwnd, MDIWindow* wnd):TabTypes(hwnd)
{
    m_hevent = NULL;
	m_wnd = wnd;
	m_hwnd = NULL;
	m_isMDIclose = wyFalse;
	m_findreplace = NULL;
}

TabHistory::~TabHistory()
{
    if(m_hwnd)
    {
        DestroyWindow(m_hwnd);
    }

	if(m_hevent)
    {
        CloseHandle(m_hevent);
    }
}

wyBool  
TabHistory::Create()
{
	CreateEditWindow();
	return wyTrue;
}

void
TabHistory::OnTabSelChange()
{
	if(pGlobals->m_pcmainwin->m_finddlg)
	{
		DestroyWindow(pGlobals->m_pcmainwin->m_finddlg);
		pGlobals->m_pcmainwin->m_finddlg = NULL;
	}

	ShowWindow(m_hwnd, SW_SHOW);
	SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)ID_OBJECT_INSERTUPDATE, TBSTATE_ENABLED);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ID_OBJECT_VIEWDATA, TBSTATE_ENABLED);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ID_TABLE_OPENINNEWTAB, TBSTATE_ENABLED);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ACCEL_EXECUTEALL,(LPARAM)TBSTATE_INDETERMINATE);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)IDM_EXECUTE,(LPARAM)TBSTATE_INDETERMINATE);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ACCEL_QUERYUPDATE ,(LPARAM)TBSTATE_INDETERMINATE); 
	SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)ID_FORMATCURRENTQUERY,(LPARAM)TBSTATE_INDETERMINATE);

    if(m_wnd)
        m_wnd->SetQueryWindowTitle();
}

// Function to create the querymessage edit box.
// This edit box is required for showing error or any messages returned from execution of queries.

HWND
TabHistory::CreateEditWindow()
{
	wyUInt32	exstyles = 0;
	wyUInt32	styles   = WS_CHILD | WS_VSCROLL | WS_HSCROLL;
	HWND	    hwndedit;

	hwndedit	= ::CreateWindowEx(exstyles, L"Scintilla", L"Source", styles, 5, 160, 150, 150,
        m_hwndparent,(HMENU)IDC_HISTORY, pGlobals->m_pcmainwin->GetHinstance(), NULL);
	m_hwnd = hwndedit;

	SendMessage(m_hwnd, SCI_SETCODEPAGE, SC_CP_UTF8, 0);

	//Set scintilla properties
	SetScintillaModes(m_hwnd, m_wnd->m_keywordstring, m_wnd->m_functionstring, wyFalse, wyTrue, "LineMySQL");
	
	SendMessage(m_hwnd, SCI_SETWRAPMODE, SC_WRAP_NONE, SC_WRAP_NONE);
    SendMessage(m_hwnd, SCI_SETSCROLLWIDTHTRACKING, (WPARAM)1, 0);
    SendMessage(m_hwnd, SCI_SETSCROLLWIDTH, 10, 0);
    //SendMessage(hwndedit, SCI_SETFONTQUALITY, (WPARAM)SC_EFF_QUALITY_LCD_OPTIMIZED, 0);

	SetFont();
	
	//for disabling the default context menu
	SendMessage(m_hwnd, SCI_USEPOPUP, 0, 0);

    SetColor();
	m_wporigproc =(WNDPROC)SetWindowLongPtr(m_hwnd, GWLP_WNDPROC,(LONG_PTR)TabHistory::WndProc);	
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA,(LONG_PTR)this);	
	return m_hwnd;
}

void
TabHistory::SetFont()
{
	EditorFont::SetFont(m_hwnd, "HistoryFont", wyTrue);
}

void
TabHistory::SetColor()
{
	EditorFont::SetColor(m_hwnd, wyTrue);
}

LRESULT	CALLBACK 
TabHistory::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TabHistory* pcqueryhistory = (TabHistory*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    wyString*   pstring;
	
    if(!pcqueryhistory)
    {
		return 1;
    }

	if(message == (wyUInt32)pGlobals->m_pcmainwin->m_findmsg) 
	{		
		if( pcqueryhistory->m_findreplace->FindReplace(hwnd, lParam) == wyFalse)
		{
            //when we are closing the find dialog, deleting the memory allocated for Find
			delete(pcqueryhistory->m_findreplace);
			pcqueryhistory->m_findreplace = NULL;
		}

		return 0;		
	}    

	switch(message)
	{
        case UM_ADDTEXT:
            pstring = (wyString*)lParam;
	        SendMessage(hwnd, SCI_SETREADONLY, false, 0);

            if(SendMessage(hwnd, SCI_GETLINECOUNT, 0, 0) > 50000)
            {
                SendMessage(hwnd, SCI_SETCURRENTPOS, 0, 0);
                SendMessage(hwnd, SCI_LINEDELETE, 0, 0);
            }

            SendMessage(hwnd, SCI_APPENDTEXT, pstring->GetLength(), (LPARAM)pstring->GetString());
            SendMessage(hwnd, SCI_SETSEL, -1, -1);
	        SendMessage(hwnd, SCI_SETREADONLY, true, 0);

            if(wParam)
            {
                SetEvent(pcqueryhistory->m_hevent);
            }

            return 1;

        case WM_CONTEXTMENU:
            if(TabHistory::OnContextMenu(lParam, hwnd, &pcqueryhistory->m_menu) == 1)
            {
                return 1;
            }

	        break;

        case WM_COMMAND:
            OnWmCommand(hwnd, wParam);
	        break;

        case WM_HELP:
	        ShowHelp("http://sqlyogkb.webyog.com/article/79-history-tab");
	        return 1;

        case WM_SETFOCUS:
        case WM_KEYUP:
        case WM_LBUTTONUP:
            PostMessage(pcqueryhistory->m_wnd->m_pctabmodule->m_hwnd, UM_SETSTATUSLINECOL, (WPARAM)hwnd, 1);
	        break;

        case WM_KILLFOCUS:
            PostMessage(pcqueryhistory->m_wnd->m_pctabmodule->m_hwnd, UM_SETSTATUSLINECOL, (WPARAM)NULL, 0);
            break;

        case WM_KEYDOWN:
	        if(HandleScintillaStyledCopy(hwnd, wParam) == wyTrue)
            {
		        return 1;
            }

	        break;

        case WM_MEASUREITEM:
            return wyTheme::MeasureMenuItem((LPMEASUREITEMSTRUCT)lParam, pcqueryhistory->m_menu);

        case WM_DRAWITEM:		
            return wyTheme::DrawMenuItem((LPDRAWITEMSTRUCT)lParam);	
	}

	return CallWindowProc(pcqueryhistory->m_wporigproc, hwnd, message, wParam, lParam);
}


void 
TabHistory::OnWmCommand(HWND hwnd, WPARAM wParam)
{
	wyInt32 pos, spos, epos;

	switch(LOWORD(wParam))
	{
	    case ID_QUERYINFO_DELETEALL:
		    SendMessage(hwnd, SCI_SETREADONLY, false, 0);
		    SendMessage(hwnd, SCI_SETTEXT, 0, (LPARAM)"");
		    SendMessage(hwnd, SCI_SETREADONLY, true, 0);
		    break;

	    case ID_QUERYINFO_COPY:
		    spos = SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0);
		    epos = SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0);
		    pos = SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
		    SendMessage(hwnd, SCI_SELECTALL, 0, 0);
		    CopyStyledTextToClipBoard(hwnd);
		    SendMessage(hwnd, SCI_SETSELECTION,(WPARAM)epos, (LPARAM)spos);
		    break;
	
	    case ID_OPEN_COPY:
		    CopyStyledTextToClipBoard(hwnd);
		    break;

	    case ID_OPEN_SAVEHISTORY:
		    SaveHistory(hwnd);
		    break;

	    case ID_QUERYINFO_SELECTALL:
		    SendMessage(hwnd, SCI_SELECTALL, 0, 0);
		    break;

	    case ID_QUERYINFO_FIND:
		    FindTextOrReplace(GetActiveWin(), wyFalse);
		    break;

	    case ID_QUERYINFO_FINDNEXT:
		    OnAccelFindNext(GetActiveWin());
		    break;
	}

    return;
}

wyBool
TabHistory::SaveHistory(HWND hwnd)
{
	wyString    file;
	wyWChar     filename[MAX_PATH+1] = {0};

	if(InitFile(hwnd, filename, SQLINDEX, MAX_PATH))
	{  
		file.SetAs(filename);
		GetActiveWin()->WriteSQLToFile(hwnd, file);
	}

	return wyTrue;
}

void
TabHistory::Resize(wyBool issplittermoved)
{
	wyInt32	hpos, vpos, width, height;
	RECT	rcParent;
    
    GetClientRect(m_hwndparent, &rcParent);
	
	hpos	= 0;
    vpos	= CustomTab_GetTabHeight(m_hwndparent);;
	width	= (rcParent.right - rcParent.left);
    height	= (rcParent.bottom - (rcParent.top + vpos));
	
	MoveWindow(m_hwnd, hpos, vpos, width, height, TRUE);
}

// Adding text to TabHistory window
void
TabHistory::AddText(wyString &historystring)
{
    wyString tempstring;
    
    tempstring.SetAs(historystring);
    tempstring.FindAndReplace("\r", "");
    tempstring.FindAndReplace("\n", "");
    tempstring.FindAndReplace("\t", "");
    tempstring.Add("\r\n");

    //compare gui thread id and active thread id
    if(GetWindowThreadProcessId(m_hwnd, NULL) == GetCurrentThreadId())
    {
        //use send message in case of gui thread id = active thread id
        SendMessage(m_hwnd, UM_ADDTEXT, 0, (LPARAM)&tempstring);
    }
    else
    {
        m_hevent = CreateEvent(NULL, TRUE, FALSE, NULL);
        PostMessage(m_hwnd, UM_ADDTEXT, 1, (LPARAM)&tempstring);
        WaitForSingleObject(m_hevent, INFINITE);
        CloseHandle(m_hevent);
        m_hevent = NULL;
    }

	return;
}


wyBool      
TabHistory::CloseTab(wyInt32 index)
{
	ShowWindow(m_hwnd, SW_HIDE);
	return wyFalse;
}

/// Shows/Hides all the content of the editor
/**
@param tabindex: IN currently selected tab index.
@param status: IN wyTrue/wyfalse
@returns void
*/
VOID
TabHistory::ShowTabContent(wyInt32 tabindex, wyBool status)
{
    ShowWindow(m_hwnd, status == wyTrue ? SW_SHOW : SW_HIDE);
	
    if(status) 
	{	 
		SetFocus(m_hwnd);
	}	 
}

void
TabHistory::Show(wyBool setfocus)
{
    wyInt32				count = 0, i;
	CTCITEM				item1 = {0}, item = {0};
	wyString			buffer;
	wyBool found = wyFalse;
    
    wyWChar		*lpfileport = 0;
	wyWChar		directory[MAX_PATH+1] = {0};

    if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyTrue)
        EditorFont::SetColor(m_hwnd,wyTrue);
    else
        EditorFont::SetColor(m_hwnd,wyFalse);
    
    count = CustomTab_GetItemCount(m_hwndparent);

	//if history tab is present set focus to it
	for(i = 0; i < count; i++)
	{
		item1.m_mask = CTBIF_IMAGE;
		CustomTab_GetItem(m_hwndparent, i, &item1);

		if(item1.m_iimage == IDI_HISTORY)
		{
            break;
		}
	}

    if(i >= count)
    {
	    //Create a new tab
        item.m_psztext    = _("History");
	    item.m_cchtextmax = strlen(_("History"));
	    item.m_mask       = CTBIF_IMAGE | CTBIF_TEXT | CTBIF_LPARAM | CTBIF_CMENU  | CTBIF_TOOLTIP;
	    item.m_iimage     = IDI_HISTORY;
	    item.m_tooltiptext = _("History");
        item.m_lparam     = (LPARAM)this;
        CustomTab_InsertItem(m_hwndparent, count, &item);
    }

    if(setfocus == wyTrue)
    {
        CustomTab_SetCurSel(m_hwndparent, i);
        CustomTab_EnsureVisible(m_hwndparent, i);
    }

}


/// Handles on tab selection changing.
/*
@returns VOID
*/
VOID		
TabHistory::OnTabSelChanging()
{
}

/// Handles on Tab Closing.
/*
@returns one on success else zero
*/
wyInt32		
TabHistory::OnTabClosing(wyBool ismanual)
{
	ShowWindow(m_hwnd, SW_HIDE);

    if(ismanual == wyTrue)
    {
        GetTabOpenPersistence(IDI_HISTORY, wyTrue);
    }

	return 1;
}

/// Handles on  Tab Closed.
/*
@returns VOID
*/
void
TabHistory::OnTabClose()
{

}

/// HAndles when MDIWindow is closing.
/*
@returns one on success else zero
*/
wyInt32	
TabHistory::OnWmCloseTab()
{
    return 1;
}

///Handles all tab controls
/*
@param tabcount: IN total number of tabs in tabcontroller
@param selindex: IN currently selected tab index.
@returns void
*/
VOID
TabHistory::HandleTabControls(wyInt32 tabcount, wyInt32 selindex)
{
	BOOL isstyle;
    isstyle = (tabcount == selindex) ? TRUE : FALSE;
    
    ShowWindow(m_hwnd, isstyle);
    ShowWindow(GetDlgItem(m_hwnd, IDC_HISTORY), isstyle);
}

//unnecessary
VOID
TabHistory::HandleFlicker()
{

}

// Set the history content as common.
void
TabHistory::SetText(wyString *historystring)
{
	if(!historystring)
		return;

	SendMessage(m_hwnd, SCI_SETREADONLY, false, 0);
	SendMessage(m_hwnd, SCI_SETTEXT, historystring->GetLength(),(LPARAM)historystring->GetString());
	SendMessage(m_hwnd, SCI_SETSEL, -1, -1);
	SendMessage(m_hwnd, SCI_SETREADONLY, true, 0);   
	
	return;
}


HWND
TabHistory::GetHwnd()
{
	return m_hwnd;
}


// Function to perform operations when the user right clicks on the window.
wyInt32
TabHistory::OnContextMenu(LPARAM lParam, HWND hwnd, HMENU* phmenu)
{
	HMENU   hmenu, htrackmenu;
	POINT   pnt;
	wyInt32 pos;
    RECT    rect;

	if(lParam == -1)
	{		
		pos = SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
		pnt.x = SendMessage(hwnd, SCI_POINTXFROMPOSITION, 0, pos) ; 
		pnt.y = SendMessage(hwnd, SCI_POINTYFROMPOSITION, 0, pos); 
		ClientToScreen(hwnd, &pnt);
	}
	else
	{
		pnt.x = (LONG)LOWORD(lParam);
		pnt.y = (LONG)HIWORD(lParam);
	}

    GetClientRect(hwnd, &rect);
    MapWindowPoints(hwnd, NULL, (LPPOINT)&rect, 2);

    if(!PtInRect(&rect, pnt))
    {
        return -1;
    }

	hmenu = LoadMenu(pGlobals->m_hinstance, MAKEINTRESOURCE(IDR_QUERYINFOMENU));
    LocalizeMenu(hmenu);
    
    if(SendMessage(hwnd, SCI_GETSELECTIONSTART, 0, 0) == SendMessage(hwnd, SCI_GETSELECTIONEND, 0, 0))
    {
        EnableMenuItem(hmenu, ID_OPEN_COPY, MF_GRAYED);
    }

	htrackmenu = GetSubMenu(hmenu, 0);
	*phmenu = htrackmenu;
	wyTheme::SetMenuItemOwnerDraw(htrackmenu);
	TrackPopupMenu(htrackmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt.x, pnt.y, 0, hwnd, NULL);
    DestroyMenu(hmenu);
	return 1;
}

// Sets the last line visible in the query history window, so that the user gets to see
// the last line only.
wyBool
TabHistory::SetLastLineVisible()
{
	SendMessage(m_hwnd, SCI_SETSEL, -1, -1);
	return wyTrue;
}
void 
TabHistory::GetCompleteTextByPost(wyString &query, MDIWindow *wnd)
{
	wyUInt32		nstrlen;
	wyChar			*data;

	THREAD_MSG_PARAM tmp = {0};

    //set the lparam sent
    
	if(GetWindowThreadProcessId(m_hwnd , NULL) == GetCurrentThreadId())
    {
        nstrlen = SendMessage(m_hwnd, SCI_GETTEXTLENGTH, 0, 0);
		data = AllocateBuff(nstrlen + 1);
		SendMessage(m_hwnd, SCI_GETTEXT, (WPARAM)nstrlen+1, (LPARAM)data);
		query.SetAs(data);

		free(data);
		
    }
    else
    {
		if(WaitForSingleObject(pGlobals->m_pcmainwin->m_sqlyogcloseevent, 0) != WAIT_OBJECT_0 )
		{
			query.SetAs("");
			return;
		}
		tmp.m_lparam = (LPARAM)&query;
		tmp.m_hevent = CreateEvent(NULL, TRUE, FALSE, NULL);

		//now post the message to ui thread and wait for the event to be set
		PostMessage(wnd->GetHwnd(), UM_GETEDITORTEXT, (WPARAM)this->GetHwnd(), (LPARAM)&tmp);
		if(WaitForSingleObject(tmp.m_hevent, 10000) == WAIT_TIMEOUT)
		{
			query.SetAs("");
			//return;
		}
		//WaitForSingleObject(tmp.m_hevent, INFINITE);


		//close the event handle
		CloseHandle(tmp.m_hevent);
		tmp.m_hevent = NULL;
		//data = (wyChar*)tmp.m_lparam;
		
	}
	

	return;
}
