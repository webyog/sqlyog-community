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
#include "QueryAnalyzerBase.h"
#include "htmlrender.h"

QueryAnalyzerBase::QueryAnalyzerBase(MDIWindow* wnd, HWND hwndparent):TabQueryTypes(wnd, hwndparent)
{
	m_hwnd				= NULL;
	m_hwndscieditor		= NULL;
	m_hwndquerystatic	= NULL;	
	m_hfont				= NULL;
	m_isdefaultpqatab   = wyFalse;
    m_isbuffereddrawing = wyFalse;
    m_origmainwndproc   = NULL;
}

QueryAnalyzerBase::~QueryAnalyzerBase()
{
	if(m_hwndscieditor)
		VERIFY(DestroyWindow(m_hwndscieditor));

	if(m_hwndquerystatic)
		VERIFY(DestroyWindow(m_hwndquerystatic));

	if(m_hwnd)
		VERIFY(DestroyWindow(m_hwnd));

	if(m_hfont)
		DeleteFont(m_hfont);
	
	m_hwndscieditor		= NULL;
	m_hwndquerystatic	= NULL;
	m_hwnd				= NULL;
	m_hfont				= NULL;	
}

LRESULT	CALLBACK 
QueryAnalyzerBase::MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    QueryAnalyzerBase*  pqa = (QueryAnalyzerBase*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    HDC                 hdc;
    PAINTSTRUCT         ps;
    TABCOLORINFO        ci = {0};
    
    if(message == WM_PAINT)
    {
        hdc = BeginPaint(hwnd, &ps);
        CustomTab_GetColorInfo(GetParent(hwnd), &ci);

        if(pqa->m_isbuffereddrawing == wyTrue)
        {
            DoubleBuffer db(hwnd, hdc);
            db.EraseBackground(ci.m_tabbg1);
            db.PaintWindow();
        }
        else
        {
            DoubleBuffer::EraseBackground(hwnd, hdc, NULL, ci.m_tabbg1);
        }

        EndPaint(hwnd, &ps);
        return 1;
    }
    else if(message == WM_ERASEBKGND)
    {
        return 1;
    }

    return CallWindowProc(pqa->m_origmainwndproc, hwnd, message, wParam, lParam);
}

LRESULT	CALLBACK 
QueryAnalyzerBase::QueryWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC         wndproc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    RECT            rect;
    HDC             hdc;
    PAINTSTRUCT     ps;
    TABCOLORINFO    ci = {0};
    HBRUSH          hbr;
    wyWChar*        text;
    wyInt32         len;
    HFONT           hfont;
    
    if(message == WM_PAINT)
    {
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rect);
        CustomTab_GetColorInfo(GetParent(GetParent(hwnd)), &ci);
        SetTextColor(hdc, ci.m_tabtext);
        SetBkMode(hdc, TRANSPARENT);
        hbr = CreateSolidBrush(ci.m_tabbg1);
        FillRect(hdc, &rect, hbr);
        DeleteBrush(hbr);
        hfont = GetStaticTextFont(hwnd);
        hfont = (HFONT)SelectObject(hdc, hfont);
        len = GetWindowTextLength(hwnd);
        text = new wyWChar[len + 2];
        GetWindowText(hwnd, text, len + 1);
        rect.left += 1;
        rect.right -= 1;
        DrawText(hdc, text, -1, &rect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
        delete[] text;
        hfont = (HFONT)SelectObject(hdc, hfont);
        DeleteFont(hfont);
        EndPaint(hwnd, &ps);
        return 1;
    }
    else if(message == WM_ERASEBKGND)
    {
        return 1;
    }

    return CallWindowProc(wndproc, hwnd, message, wParam, lParam);
}

wyBool
QueryAnalyzerBase::CreateAnalyzerTab(wyInt32 index)
{
	wyString    result("Analyzer");
	wyInt32		icon;
	
	icon = IDI_QAALERT;
	
	//The index of this tab is '1'. means It comes after the 1st Result tab(whose index is '0')	
	InsertTab(m_hwndparent, index, icon, result, (LPARAM)this);
	return wyTrue;
}

wyBool			
QueryAnalyzerBase::CreateMainWindow()
{	
	DWORD style = WS_CHILD;

    VERIFY(m_hwnd =  CreateWindow(INSERT_UPDATE_WINDOW_CLASS_NAME_STR,
								L"", 
								style, 
								0, 0, 0, 0,
							    m_hwndparent, (HMENU)ID_ANALYZERMAIN,
								pGlobals->m_entinst, NULL));

    m_origmainwndproc = (WNDPROC)SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)QueryAnalyzerBase::MainWndProc);
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	return wyTrue;
}

//Scintilla ediotr displays query executed 
wyBool
QueryAnalyzerBase::CreateHtmlEditor(wyString *htmlbuffer)
{	
	wyUInt32	exstyles = NULL;
	wyUInt32	styles   = WS_CHILD | WS_VISIBLE | WS_VSCROLL; 	
	HINSTANCE	hinst = NULL;	

#ifdef COMMUNITY
	hinst = pGlobals->m_hinstance;

#else
	hinst = pGlobals->m_entinst;
#endif
	

	//INSERT_UPDATE_WINDOW_CLASS_NAME_STR
	VERIFY(m_hwndscieditor = ::CreateWindowEx(exstyles, PQA_RESULT_WINDOW, 
										L"Source", styles, 
										0, 0, 0, 0,
										m_hwnd, (HMENU)NULL, 
										hinst, 
										this));

	if(!m_hwndscieditor)
		return wyFalse;

	
	htmlayout::attach_event_handler(m_hwndscieditor, &DOMEventsHandler);
		 
	//Processing the html code 
	HTMLayoutLoadHtml(m_hwndscieditor, (PBYTE)htmlbuffer->GetString(), htmlbuffer->GetLength());

	 //html page Selection mode
	HTMLayoutSetMode(m_hwndscieditor, HLM_SHOW_SELECTION);
    return wyTrue;
}

LRESULT	CALLBACK
QueryAnalyzerBase::PQAHtmlWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult;
	BOOL    bHandled;
    // HTMLayout +
    // HTMLayout could be created as separate window 
    // using CreateWindow API.
    // But in this case we are attaching HTMLayout functionality
    // to the existing window delegating windows message handling to 
    // HTMLayoutProcND function.
    lResult = HTMLayoutProcND(hwnd,message,wParam,lParam, &bHandled);
    
    if(bHandled)
    {
        return lResult;
    }

    switch(message)
    {		
    case WM_HELP:
	    ShowHelp("http://sqlyogkb.webyog.com/article/84-query-profiler");
	    return 1;//On clicking help, profiler page should open			
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

//Resizeing all the controls in analyzer tab
void
QueryAnalyzerBase::Resize()
{	
	RECT	rctparent;
	wyInt32	height, width, top, left, ribbonheight;

    GetClientRect(m_hwndparent, &rctparent);
    top = CustomTab_GetTabHeight(m_hwndparent);
    width = rctparent.right;
    left = 0;
    height = rctparent.bottom - top;
    MoveWindow(m_hwnd, left, top, width, height, TRUE);

	GetClientRect(m_hwnd, &rctparent);
	top = 0;		
	left = 0;
    width = rctparent.right;
    height = rctparent.bottom;	

	//query static window(ribbon)
	if(m_isdefaultpqatab == wyFalse)
	{
        ribbonheight = DataView::CalculateRibbonHeight(m_hwndquerystatic);
		MoveWindow(m_hwndquerystatic, left, height - ribbonheight, width, ribbonheight, TRUE);
        height -= ribbonheight;
	}
	else
	{
        MoveWindow(m_hwndquerystatic, left, height, 0, 0, TRUE);
	}
	
	MoveWindow(m_hwndscieditor, left, top, width, height, TRUE); 
}

void
QueryAnalyzerBase::OnTabSelChange(wyBool isselected)
{
    wyInt32 status = (isselected == wyTrue) ? SW_SHOW : SW_HIDE;

	ShowWindow(m_hwnd, status);

    if(isselected == wyTrue)
    {
        SetFocus(m_hwndscieditor);
        UpdateStatusBar(m_pmdi->m_pcquerystatus);
    }
}

void
QueryAnalyzerBase::SetBufferedDrawing(wyBool isset)
{
    m_isbuffereddrawing = isset;
}
