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


/*! \file TabMessage.cpp
    \brief Implements "TabMessage" class
    
*/
#include "MDIWindow.h"
#include "Global.h"
#include "TabMessage.h"
#include "FrameWindowHelper.h"
#include "ExportMultiFormat.h"
#include "CommonHelper.h"
#include "GUIHelper.h"
#include "EditorFont.h"
#include "Scintilla.h"
#include "Include.h"



extern PGLOBALS pGlobals;

TabMessage::TabMessage(MDIWindow* wnd, HWND hwndparent):TabQueryTypes(wnd, hwndparent)
{
	m_hfont = NULL;
	m_hwnd	= NULL;
    m_sumoftotaltime = 0;
    m_sumofexectime = 0;
    m_selectedoption = 0;
    m_hwndcombo = NULL;
    m_origlistproc = NULL;
    m_isautomatedmove = wyFalse;
}

TabMessage::~TabMessage()
{
    if(m_hwnd)
    {
		DestroyWindow(m_hwnd);
    }

    if(m_hwndcombo)
    {
        DestroyWindow(m_hwndcombo);
    }

	if(m_hfont)
    {
		DeleteObject(m_hfont);
    }
}

wyBool  
TabMessage::Create()
{
    wyWChar* items[] = {
        _(L"All"),
        _(L"Queries with errors"),
        _(L"Queries with warnings"),
        _(L"Queries with errors/warnings"),
        _(L"Queries with result set"), 
        _(L"Queries without result set")
    };

    wyInt32 count, i, width = 0;
    HDC     hdc;
    HFONT   hfont = GetStockFont(DEFAULT_GUI_FONT);
    RECT    rect = {0};
    COMBOBOXINFO cbinfo = {0};

    CreateQueryMessageEdit(m_hwndparent, m_pmdi);
    m_hwndcombo = CreateWindowEx(0, L"combobox", L"", WS_CHILD | CBS_DROPDOWN | WS_VISIBLE, 
        0, 0, 0, 0, m_hwndparent, (HMENU)IDC_TOOLCOMBO, (HINSTANCE)GetModuleHandle(0), NULL);
    SendMessage(m_hwndcombo, WM_SETFONT, (WPARAM)hfont, 0);
    cbinfo.cbSize = sizeof(COMBOBOXINFO);
    GetComboBoxInfo(m_hwndcombo, &cbinfo);
    SetWindowLongPtr(cbinfo.hwndList, GWLP_USERDATA, (LONG_PTR)this);
    //m_origlistproc = (WNDPROC)SetWindowLongPtr(cbinfo.hwndList, GWLP_WNDPROC, (LONG)ComboListProc);
   
    count = sizeof(items)/sizeof(items[0]);
    hdc = GetDC(m_hwndcombo);
    hfont = SelectFont(hdc, hfont);

    for(i = 0; i < count; ++i)
    {
        SendMessage(m_hwndcombo, CB_ADDSTRING, 0, (LPARAM)items[i]);
        DrawText(hdc, items[i], -1, &rect, DT_CALCRECT | DT_NOPREFIX);

        if(rect.right > width)
        {
            width = rect.right;
        }
    }

    SelectFont(hdc, hfont);
    ReleaseDC(m_hwndcombo, hdc);
    SendMessage(m_hwndcombo, CB_SETCURSEL, 0, 0);
    GetWindowRect(m_hwndcombo, &rect);
    SetWindowPos(m_hwndcombo, NULL, 0, 0, width + (cbinfo.rcButton.right - cbinfo.rcButton.left) + 10, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOMOVE);
    //SendMessage(m_hwndcombo, CB_SETDROPPEDWIDTH, width + 10, 0);
	return wyTrue;
}

// Function to create the querymessage edit box.
// This edit box is required for showing error or any messages returned from execution of queries.
HWND
TabMessage::CreateQueryMessageEdit(HWND hwndparent, MDIWindow *wnd)
{
    wyUInt32	exstyles = 0;
    wyUInt32	styles   = WS_CHILD | WS_VSCROLL | WS_VISIBLE;
	
	m_hwnd	= ::CreateWindowEx(exstyles, L"Scintilla", L"Source", styles, 5, 160, 150, 150,
        hwndparent,(HMENU)IDC_QUERYMESSAGEEDIT, pGlobals->m_pcmainwin->GetHinstance(), this);
	
	SendMessage(m_hwnd, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
	SendMessage(m_hwnd, SCI_SETWRAPMODE, SC_WRAP_WORD, 0);
    SendMessage(m_hwnd, SCI_SETSCROLLWIDTHTRACKING, (WPARAM)1, 0);
    SendMessage(m_hwnd, SCI_SETSCROLLWIDTH, 10, 0);
    
    m_wporigproc =(WNDPROC)SetWindowLongPtr(m_hwnd, GWLP_WNDPROC,(LONG_PTR)TabMessage::WndProc);	
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);


	
    SetFont();
    SetColor();

	//for disabling the default context menu
	SendMessage(m_hwnd, SCI_USEPOPUP, 0, 0);
    SendMessage(m_hwnd,SCI_SETMARGINWIDTHN,1,0);
    SendMessage(m_hwnd, SCI_SETREADONLY, true, 0);

	return m_hwnd;
}
 
// Function to subclass the messageedit box. It is not used now but it is reserved for later
// use.
LRESULT	CALLBACK 
TabMessage::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	TabMessage* pcquerymessageedit = (TabMessage*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
    case WM_COMMAND:
        pcquerymessageedit->OnWMCommand(wparam);
        break;

    case UM_FOCUS:
	    SetFocus(pcquerymessageedit->m_hwnd);
		break;

     case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/78-message-tab");
		return 1;

     case SCI_COPY:
     case WM_KEYDOWN:
        if(wparam == VK_INSERT || wparam == 'c' || wparam == 'C')
        {
            pcquerymessageedit->CopyTextToClipBoard();
            return 1;
        }
	    break;

	case WM_SETFOCUS:
	case WM_KEYUP:
	case WM_LBUTTONUP:
        PostMessage(pcquerymessageedit->m_pmdi->m_pctabmodule->m_hwnd, UM_SETSTATUSLINECOL, (WPARAM)hwnd, 1);
		break;
        
    case WM_KILLFOCUS:
        PostMessage(pcquerymessageedit->m_pmdi->m_pctabmodule->m_hwnd, UM_SETSTATUSLINECOL, (WPARAM)NULL, 0);
        break;

    case WM_DESTROY:
	    if(pcquerymessageedit->m_hfont)
        {
		    DeleteObject(pcquerymessageedit->m_hfont);
        }

	    pcquerymessageedit->m_hfont = NULL;
	    break;

    /*case WM_CONTEXTMENU:
        if(pcquerymessageedit->OnContextMenu(lparam) == wyTrue)
        {
            return 0;
        }

        break;*/
    }

    return CallWindowProc(pcquerymessageedit->m_wporigproc, hwnd, message, wparam, lparam);
}

void
TabMessage::CopyTextToClipBoard()
{
    wyInt32 len, selstart, selend, linestart, lineend, i, linestartpos, selstartpos;
    wyChar* text;
    wyString str;

    selstart = SendMessage(m_hwnd, SCI_GETSELECTIONSTART, 0, 0);
    selend = SendMessage(m_hwnd, SCI_GETSELECTIONEND, 0, 0);

    if(selstart == selend)
    {
        return;
    }

    linestart = SendMessage(m_hwnd, SCI_LINEFROMPOSITION, selstart, 0);
    lineend = SendMessage(m_hwnd, SCI_LINEFROMPOSITION, selend, 0);

    for(i = linestart; i <= lineend; ++i)
    {
        if(!SendMessage(m_hwnd, SCI_GETLINEVISIBLE, i, 0))
        {
            continue;
        }

        len = SendMessage(m_hwnd, SCI_LINELENGTH, i, 0);
        text = new wyChar[len + 1];
        SendMessage(m_hwnd, SCI_GETLINE, i, (LPARAM)text);
        selstartpos = linestartpos = SendMessage(m_hwnd, SCI_POSITIONFROMLINE, i, 0);
        
        if(i == linestart)
        {
            selstartpos = selstart;
            len -= (selstartpos - linestartpos);
			if(i == lineend)
			{
				len = selend - selstartpos;
			}
        }
        else if(i == lineend)
        {
            len = selend - linestartpos;
        }

        str.AddSprintf("%.*s", len, text + (selstartpos - linestartpos));
        delete text;
    }

    str.FindAndReplace("\r\n<r>", "\r\n");
    str.FindAndReplace("\r\n<e>", "\r\n");
    str.FindAndReplace("\r\n<n>", "\r\n");
    str.FindAndReplace("\r\n<w>", "\r\n");
    SendMessage(m_hwnd, SCI_COPYTEXT, str.GetLength(), (LPARAM)str.GetString());
}

void
TabMessage::OnWMCommand(WPARAM wparam)
{
    wyInt32 i, linecount, temp, condition1 = 0, condition2 = 0, condition3 = 0, condition4 = 0, lastvisibleline = -1;

    m_selectedoption = SendMessage(m_hwndcombo, CB_GETCURSEL, 0, 0);
    linecount = SendMessage(m_hwnd, SCI_GETLINECOUNT, 0, 0) - 1;

    switch(m_selectedoption)
    {
        case 0:
            SendMessage(m_hwnd, SCI_SHOWLINES, 0, linecount - 1);
            return;

        case 1:
            condition1 = condition3 = SCE_SQLYOGMSG_ERRORTAG;
            condition2 = condition4 = SCE_SQLYOGMSG_ERROR;
            break;

        case 2:
            condition1 = condition3 = SCE_SQLYOGMSG_WARNINGTAG;
            condition2 = condition4 = SCE_SQLYOGMSG_WARNING;
            break;

        case 3:
            condition1 = SCE_SQLYOGMSG_WARNINGTAG;
            condition2 = SCE_SQLYOGMSG_WARNING;
            condition3 = SCE_SQLYOGMSG_ERRORTAG;
            condition4 = SCE_SQLYOGMSG_ERROR;
            break;

        case 4:
            condition1 = condition3 = SCE_SQLYOGMSG_RESULTTAG;
            condition2 = condition4 = SCE_SQLYOGMSG_RESULT;
            break;

        case 5:
            condition1 = SCE_SQLYOGMSG_NORESULTTAG;
            condition2 = SCE_SQLYOGMSG_NORESULT;
            condition3 = SCE_SQLYOGMSG_WARNINGTAG;
            condition4 = SCE_SQLYOGMSG_WARNING;
    }

    for(i = linecount - 1; i > 1; --i)
    {
        temp = SendMessage(m_hwnd, SCI_POSITIONFROMLINE, i, 0);
        temp = SendMessage(m_hwnd, SCI_GETSTYLEAT, temp, 0);
                
        if(temp != condition1 && temp != condition2 && temp != condition3 && temp != condition4)
        {
            SendMessage(m_hwnd, SCI_HIDELINES, i, i);
        }
        else
        {
            SendMessage(m_hwnd, SCI_SHOWLINES, i, i);

            if(lastvisibleline == -1)
            {
                lastvisibleline = i;
            }
        }
    }

    if(lastvisibleline != linecount - 1)
    {
        SendMessage(m_hwnd, SCI_HIDELINES, lastvisibleline - 1, lastvisibleline);
    }
}

wyBool
TabMessage::OnContextMenu(LPARAM lparam)
{
    HMENU   hmenu, htrackmenu;
	POINT   pnt;
	wyInt32 pos;
    RECT    rect;

	if(lparam == -1)
	{		
		pos = SendMessage(m_hwnd, SCI_GETCURRENTPOS, 0, 0);
		pnt.x = SendMessage(m_hwnd, SCI_POINTXFROMPOSITION, 0, pos) ; 
		pnt.y = SendMessage(m_hwnd, SCI_POINTYFROMPOSITION, 0, pos); 
		ClientToScreen(m_hwnd, &pnt);
	}
	else
	{
		pnt.x = (LONG)LOWORD(lparam);
		pnt.y = (LONG)HIWORD(lparam);
	}

    GetClientRect(m_hwnd, &rect);
    MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rect, 2);

    if(!PtInRect(&rect, pnt))
    {
        return wyFalse;
    }

    hmenu = LoadMenu(pGlobals->m_hinstance, MAKEINTRESOURCE(IDR_MESSAGETAB_MENU));
    LocalizeMenu(hmenu);
    htrackmenu = GetSubMenu(hmenu, 0);
	TrackPopupMenu(htrackmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt.x, pnt.y, 0, m_hwnd, NULL);
    DestroyMenu(hmenu);
    return wyTrue;
}

void
TabMessage::SetColor()
{
    wyWChar     directory[MAX_PATH+1] = {0}, *lpfileport = 0;
	wyString	dirstr;

    if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyTrue)
    {
        dirstr.SetAs(directory);
        m_rgbselectioncolor   =   wyIni::IniGetInt(GENERALPREFA, "MTISelectionColor",   DEF_TEXTSELECTION, dirstr.GetString());
        m_rgbbgcolor=wyIni::IniGetInt(GENERALPREFA, "MTIBgColor", DEF_BKGNDEDITORCOLOR, dirstr.GetString()); 
        m_rgbfgcolor=wyIni::IniGetInt(GENERALPREFA, "MTIFgColor", DEF_NORMALCOLOR, dirstr.GetString()); 


        SendMessage(m_hwnd,SCI_SETSELBACK,1,m_rgbselectioncolor);
        SendMessage( m_hwnd, SCI_SETCARETFORE,m_rgbbgcolor ^ 0xFFFFFF,0); //Change Caret color in editor window
        SendMessage(m_hwnd, SCI_STYLESETBACK, STYLE_DEFAULT, (LPARAM)m_rgbbgcolor);
        SendMessage(m_hwnd, SCI_STYLESETFORE, STYLE_DEFAULT, m_rgbfgcolor);
        SendMessage(m_hwnd, SCI_STYLESETBOLD, STYLE_DEFAULT, FALSE);
        SendMessage(m_hwnd, SCI_STYLECLEARALL, 0, 0);
        SendMessage(m_hwnd, SCI_STYLESETVISIBLE, SCE_SQLYOGMSG_ERRORTAG, 0);
        SendMessage(m_hwnd, SCI_STYLESETVISIBLE, SCE_SQLYOGMSG_RESULTTAG, 0);
        SendMessage(m_hwnd, SCI_STYLESETVISIBLE, SCE_SQLYOGMSG_NORESULTTAG, 0);
        SendMessage(m_hwnd, SCI_STYLESETVISIBLE, SCE_SQLYOGMSG_WARNINGTAG, 0);
    }
}



// Set font for the messageedit box.
void
TabMessage::SetFont()
{
	wyInt32     ret;
	wyWChar     directory[MAX_PATH+1] = {0}, *lpfileport = 0;
	wyInt32		px, fontitalics;
	wyString	dirstr, fontnamestr;

    if(m_hfont)
    {
		DeleteObject(m_hfont);
        m_hfont = NULL;
    }

	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	
    if(ret == 0)
    {
		return;
    }
	
	dirstr.SetAs(directory);

	wyIni::IniGetString("DataFont", "FontName", "Courier New", &fontnamestr, dirstr.GetString());
	px = wyIni::IniGetInt("DataFont", "FontSize", 10, dirstr.GetString());
	fontitalics = wyIni::IniGetInt("DataFont", "FontItalic", 0, dirstr.GetString());
	SendMessage(m_hwnd, SCI_SETLEXERLANGUAGE, 0, (LPARAM)"SQLyogMsg");

    SendMessage(m_hwnd, SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)fontnamestr.GetString());
	SendMessage(m_hwnd, SCI_STYLESETSIZE, STYLE_DEFAULT, px);
	SendMessage(m_hwnd, SCI_STYLESETITALIC, STYLE_DEFAULT, fontitalics);
}

// Function to resize the messageedit box. It resizes itself with respect to its parent window 
// that is the query tab.
void
TabMessage::Resize()
{
	wyInt32     vpos, width, height, tabheight;
	RECT	    rcparent, rccombo;

	GetClientRect(m_hwndparent, &rcparent);
    GetWindowRect(m_hwndcombo, &rccombo);
    tabheight = CustomTab_GetTabHeight(m_hwndparent);
    vpos = tabheight;
	width = rcparent.right - rcparent.left;
	height = ((rcparent.bottom - rcparent.top) - vpos) - ((rccombo.bottom - rccombo.top) + 4);
	
    SetWindowPos(m_hwndcombo, NULL, 0, vpos + height + 2, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	MoveWindow(m_hwnd, 0, vpos, width, height, TRUE);
}

void
TabMessage::OnTabSelChange(wyBool isselected)
{
    ShowWindow(m_hwnd, isselected == wyTrue ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hwndcombo, isselected == wyTrue ? SW_SHOW : SW_HIDE);

    if(isselected == wyTrue)
    {
        SetFocus(m_hwnd);
        UpdateStatusBar(m_pmdi->m_pcquerystatus);
    }
}

//Function adds text in the edit box.
void
TabMessage::AddText(wyString& str)
{
    SendMessage(m_hwnd, SCI_SETREADONLY, false, 0);
    SendMessage(m_hwnd, SCI_SETTEXT, 0, (LPARAM)str.GetString());
    SendMessage(m_hwnd, SCI_SETREADONLY, true, 0);
    SendMessage(m_hwndcombo, CB_SETCURSEL, 0, 0);
}

HWND
TabMessage::GetHwnd()
{
	return m_hwnd;
}

void
TabMessage::UpdateStatusBar(StatusBarMgmt* pmgmt)
{
    pmgmt->AddTickCount(m_pmdi->m_tunnel, m_sumofexectime, m_sumoftotaltime);
    pmgmt->AddNumRows(0, wyTrue);
}


//callback procedure for search control
LRESULT	CALLBACK 
TabMessage::ComboListProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    TabMessage* ptm = (TabMessage*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    RECT        rect = {0}, rectparent = {0};
    HMONITOR    hmonitor;
    MONITORINFO mi = {0};
    
    if(message == WM_MOVE && ptm->m_isautomatedmove == wyFalse)
    {
        GetWindowRect(hwnd, &rect);
        hmonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
        mi.cbSize = sizeof(mi);
        GetMonitorInfo(hmonitor, &mi); 

        if(rect.bottom > mi.rcWork.bottom)
        {
            GetWindowRect(ptm->m_hwndcombo, &rectparent);
            ptm->m_isautomatedmove = wyTrue;
            SetWindowPos(hwnd, NULL, rect.left, rectparent.top - (rect.bottom - rect.top), 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOSENDCHANGING);
            ptm->m_isautomatedmove = wyFalse;
        }
    }

    //call the original proc
    return CallWindowProc(ptm->m_origlistproc, hwnd, message, wparam, lparam);
}