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

#include "TabEditorSplitter.h"
#include "GUIHelper.h"

#define TAB_HEIGHT      24

TabEditorSplitter::TabEditorSplitter(HWND hwnd, wyUInt32 ptoppercent)
{
	m_hwndparent	    = hwnd;
	m_hwnd			    = NULL;
	m_leftortoppercent = ptoppercent;
	m_pctabeditor	   = NULL;
    m_ischanged         = wyFalse;
	m_isdlgsplitter		= wyFalse;
    m_height            = 6; //GetSystemMetrics(SM_CXSIZEFRAME) + 1;
    m_hwndprevfocus = NULL;
    m_y = 0;
    m_hdc = NULL;
    m_hpen = NULL;
}

TabEditorSplitter::~TabEditorSplitter()
{
    EndDrag(wyFalse);

    if(m_hwnd)
	{	
		DestroyWindow(m_hwnd);
		m_hwnd = NULL;
	}
}

wyBool
TabEditorSplitter::Create(wyBool isdlgsplitter)
{
	m_hwnd = CreateHSplitter();
    
	if(isdlgsplitter == wyTrue)
		m_isdlgsplitter = wyTrue;

	return wyTrue;
}

HWND
TabEditorSplitter::CreateHSplitter()
{
	wyUInt32        style = WS_CHILD | WS_VISIBLE ;
	HWND			hwndsplitter;

	// create the splitter window
	hwndsplitter = CreateWindowEx(0, HSPLITTER_WINDOW_CLASS_NAME_STR, L"", 
									style, 0, 0, 0, 0, m_hwndparent, (HMENU)IDC_HSPLITTER,
									pGlobals->m_pcmainwin->GetHinstance(), this);

	_ASSERT(hwndsplitter != NULL);

	ShowWindow(hwndsplitter, SW_SHOWDEFAULT);    
	VERIFY(UpdateWindow(hwndsplitter));

	return hwndsplitter;
}


// callback function for the splitter window.
LRESULT	CALLBACK 
TabEditorSplitter::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	TabEditorSplitter	*ptabsplitter =(TabEditorSplitter*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    HBRUSH hbr;
    RECT rc;

	switch(message)
	{
    case WM_NCCREATE:
		// Get the initial creation pointer to the window object
		ptabsplitter = (TabEditorSplitter *)((CREATESTRUCT *)lparam)->lpCreateParams;        
		ptabsplitter->m_hwnd = hwnd;
		SetWindowLongPtr(hwnd, GWLP_USERDATA,(LONG_PTR)ptabsplitter);
        break;

	case WM_PAINT:
		return ptabsplitter->OnPaint();
		
	case WM_MOUSEMOVE:
		ptabsplitter->MouseMove();
		return 0;
	
    case WM_MOVE:
        //return ptabsplitter->OnPaint();
        InvalidateRect(ptabsplitter->m_hwnd, NULL, FALSE);  // hav to test it
        UpdateWindow(hwnd);
        return 0;

	case WM_LBUTTONDOWN:
        ptabsplitter->m_hwndprevfocus = GetFocus();
        ptabsplitter->m_prevstyle = GetWindowLongPtr(ptabsplitter->m_hwndparent, GWL_STYLE);
        SetWindowLongPtr(ptabsplitter->m_hwndparent, GWL_STYLE, ptabsplitter->m_prevstyle & ~WS_CLIPCHILDREN);
        SetFocus(hwnd);
		SetCapture(hwnd);
		ptabsplitter->m_isdragged	= wyTrue;
        ptabsplitter->MouseMove(wyTrue);
		break;

	case WM_LBUTTONUP:
		ReleaseCapture();
		break;

	case WM_CAPTURECHANGED:
		if(ptabsplitter->m_isdragged == wyTrue)
		{
            ptabsplitter->EndDrag(wyTrue);
            SetWindowLongPtr(ptabsplitter->m_hwndparent, GWL_STYLE, ptabsplitter->m_prevstyle);

			if(ptabsplitter->m_isdlgsplitter == wyTrue)
				PostMessage(GetParent(hwnd), UM_SPLITTERRESIZED, 0, 0);
			else
			{
                ptabsplitter->Resizeall();
				InvalidateRect(hwnd, NULL, TRUE);
				UpdateWindow(hwnd);
			}

			ptabsplitter->m_isdragged = wyFalse;

            if(ptabsplitter->m_hwndprevfocus)
            {
                SetFocus(ptabsplitter->m_hwndprevfocus);
                ptabsplitter->m_hwndprevfocus = NULL;
            }
		}
		break;

    case WM_ERASEBKGND:
        if(ptabsplitter->m_isdlgsplitter == wyFalse &&
            wyTheme::GetBrush(BRUSH_HSPLITTER, &hbr))
        {
            GetClientRect(hwnd, &rc);
            FillRect((HDC)wparam, &rc, hbr);
            return 1;
        }
	}
			
	return(DefWindowProc(hwnd, message, wparam, lparam));
}

wyBool
TabEditorSplitter::Resize(RECT rcmain)
{
	wyInt32     padding;
		
	SendMessage(m_hwnd, WM_SETREDRAW,  FALSE, NULL);

    padding = m_isdlgsplitter == wyTrue ? 0 : CustomTab_GetTabHeight(m_hwndparent);
	m_rect.bottom	= m_height;
	m_rect.left		= rcmain.left;

	//If we are hiding edit window then m_leftortoppercent = 0,
    m_rect.top      = (long)(padding + ((rcmain.bottom - TAB_HEIGHT + rcmain.top)*((long)m_leftortoppercent /(float)100)));
	m_rect.right	= rcmain.right + 2;

    if(m_rect.top > rcmain.bottom - m_height)
    {
        m_rect.top = rcmain.bottom - m_height;
    }
	
    MoveWindow(m_hwnd,  m_rect.left, m_rect.top, m_rect.right, m_rect.bottom, TRUE);

	SendMessage(m_hwnd, WM_SETREDRAW,  TRUE, NULL);
	InvalidateRect(m_hwnd, NULL, TRUE);
		
	return wyTrue;
}

LRESULT
TabEditorSplitter::OnPaint()
{
	HDC			hdc;
	RECT		rect;
	PAINTSTRUCT ps;
    HPEN        hpen;

	// Begin Paint
	VERIFY(hdc = BeginPaint(m_hwnd, &ps));

	// Get the client rect of of the window.
	VERIFY(GetClientRect(m_hwnd, &rect));

    if(m_isdlgsplitter == wyFalse && wyTheme::GetPen(PEN_HSPLITTER, &hpen))
    {
        hpen = SelectPen(hdc, hpen);
        MoveToEx(hdc, -1, rect.bottom, NULL);
        LineTo(hdc, rect.right, rect.bottom);
        hpen = SelectPen(hdc, hpen);
    }
    else
    {
	// now draw a beveled window depending upon whether it is horz splitter
	// or vertical splitter.
	DrawEdge(hdc, &rect, BDR_RAISEDOUTER | BDR_RAISEDOUTER, BF_RECT);
    }
		
	// End Painting
	EndPaint(m_hwnd, &ps);

	return 0;
}

void
TabEditorSplitter::MouseMove(wyBool isinit)
{
	RECT		parentrect;
	POINT		curpos, pt;
    LOGBRUSH    lb = {0};
    WORD        bitmap[] = {0x00AA, 0x0055, 0x00AA, 0x0055, 0x00AA, 0x0055, 0x00AA, 0x0055};
    HBITMAP     hbitmap;

	VERIFY(GetClientRect(m_hwndparent, &parentrect));

    //set the tab height
    parentrect.top = m_isdlgsplitter == wyTrue ? 0 : CustomTab_GetTabHeight(m_hwndparent);
	
	// Get the screen coordinates and convert it into client points.
	VERIFY(GetCursorPos(&curpos));
	VERIFY(ScreenToClient(m_hwndparent, &curpos));

	if(m_isdragged == wyTrue)
	{
        pt = curpos;

        if(isinit == wyTrue)
        {
            m_y = pt.y;
            m_hdc = GetDC(m_hwndparent);
            hbitmap = CreateBitmap(8, 8, 1, 1, bitmap);
            lb.lbStyle = BS_PATTERN; 
            lb.lbColor = 0;     
            lb.lbHatch = (ULONG_PTR)hbitmap;
            m_hpen = ExtCreatePen(PS_GEOMETRIC | PS_ENDCAP_FLAT , 4, &lb, 0, NULL); 
            DeleteBitmap(hbitmap);
        }

        if(curpos.y <= parentrect.top)
		{
			m_rect.top = parentrect.top;
			m_leftortoppercent = 0;
		} 
		else
        {
            m_ischanged = wyTrue;

            if(curpos.y <= (parentrect.bottom - m_height))
            {
                m_rect.top = curpos.y;
                m_leftortoppercent = (((curpos.y - parentrect.top) * 100 / (parentrect.bottom - parentrect.top) * 100) / 100);
            } 
            else 
            {
                m_rect.top = parentrect.bottom - m_height;
                m_leftortoppercent = 100;
            }
        }

        DrawTrackerLine();

        if(isinit == wyTrue)
        {
            return;
        }

        m_y = pt.y;
        DrawTrackerLine();
	}

	return;
}

void 
TabEditorSplitter::DrawTrackerLine()
{
    wyInt32 rop;
    HPEN    hpen;

    rop = SetROP2(m_hdc, R2_NOTXORPEN);
    hpen = (HPEN)SelectObject(m_hdc, m_hpen);
    MoveToEx(m_hdc, m_rect.left, m_y, NULL);
    LineTo(m_hdc, m_rect.right, m_y);
    SelectObject(m_hdc, hpen);
    SetROP2(m_hdc, rop);
}

void
TabEditorSplitter::EndDrag(wyBool isdrawline)
{
    if(isdrawline == wyTrue)
    {
        DrawTrackerLine();
    }

    if(m_hpen)
    {
        DeletePen(m_hpen);
        m_hpen = NULL;
    }

    if(m_hdc)
    {
        ReleaseDC(m_hwndparent, m_hdc);
        m_hdc = NULL;
    }
}

wyBool
TabEditorSplitter::Resizeall()
{
	MDIWindow *wnd;
	//TabTypes   *tabtype;

	wnd = GetActiveWin();

	if(!wnd || !wnd->m_pctabmodule)
		return wyFalse;
	
    /*
	tabtype = wnd->m_pctabmodule->GetActiveTabType();

	if(!tabtype)
		return wyFalse;

    tabtype->Resize(wyTrue);
    */

    CustomTab_SetPaintTimer(wnd->m_pctabmodule->m_hwnd);
    wnd->Resize();
	return wyTrue;
}

void
TabEditorSplitter::SetTabEditorPtr(TabEditor *ptabeditor)
{
	m_pctabeditor = ptabeditor;
}

wyInt32 
TabEditorSplitter::GetLeftTopPercent()
{
    return m_leftortoppercent;
}

void 
TabEditorSplitter::SetLeftTopPercent(wyInt32 perc)
{
    m_leftortoppercent = perc;
}

