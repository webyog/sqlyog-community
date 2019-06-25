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

/*****************************************************************************
Rewritten by : Vishal P.R

This is the Version 2 of Custom Tab control. The drwing logic has been 
rewritten completly to enable improved scrolling and tab behaviors.
The external interfaces are kept as it is to make it compatible with the 
exisitng SQLyog code base, however some minor changes has been done to improve 
the perfomance and a few redundant members are removed.
*****************************************************************************/

#include <windowsX.h>
#include <stdio.h>
#include <windows.h>

#include "Verify.h"
#include "CustTab.h"
#include "MDIWindow.h"
#include "CommonHelper.h"
#include "DoubleBuffer.h"

#define IDC_CTAB		                    WM_USER + 116
#define UM_PAINTTABHEADER                   WM_USER + 200
#define FIXED_WIDTH		                    150
#define SCROLLBUTTONPADDING                 1
#define SCROLLBUTTONWIDTH                   (16 + SCROLLBUTTONPADDING)
#define RIGHTPADDING(ISPLUSSIGN)            (ISPLUSSIGN ? SCROLLBUTTONWIDTH + 50 : SCROLLBUTTONWIDTH)
#define SCROLLTHRESHOLD                     10
#define IDC_LEFTSCROLL                      1
#define IDC_RIGHTSCROLL                     2 
#define IDC_DROPDOWN	                    14
#define IDC_PLUSSIGN                        3
#define DRAGSCROLLTIMER                     100
#define SCROLLTIMER                         200
#define PAINTTIMER                          300
#define ICONANIMATETIMER_BASE               1000
#define GET_INDEX_FROM_ICONTIMER(TIMERID)   (TIMERID - ICONANIMATETIMER_BASE)    
#define BOTTOMLINEHEIGHT                    3
#define	EXTRAHEIGHT		                    8
#define	RECTHEIGHT		                    8
#define	IMAGEWIDTH		                    20
#define	TABTOPSPACE				            1
#define TABIMGWIDTH				            21
#define CLOSERECTWIDTH                      13
#define CLOSERECTPADDING                    5
#define CLOSEBUTTONWIDTH                    10
#define CLOSEBUTTONLEFTPADDING              10
#define CLOSEBUTTONRIGHTPADDING             4
#define SCALEANTIALIASING(X)                ((X) * 4)

#define TABCOLOR_TAB_BG_1                      GetSysColor(COLOR_BTNFACE)
#define TABCOLOR_TAB_BG_2                      RGB(0xF1, 0xF1, 0xF1)
#define TABCOLOR_SELTAB_FG_1                   RGB(0xF3, 0xF1, 0xE6);
#define TABCOLOR_SELTAB_FG_2                   RGB(0xFF, 0xFF, 0xFF)
#define TABCOLOR_SELTAB_TEXT                   RGB(0x00, 0x00, 0x00)
#define TABCOLOR_TAB_TEXT                      RGB(0x00, 0x00, 0x00)
#define TABCOLOR_ACTIVESEP_PEN                 RGB(0xA1, 0xA2, 0xA8)
#define TABCOLOR_INACTIVESEP_PEN               RGB(88,88,88)
#define TABCOLOR_CLOSEBUTTON                   RGB(128, 0, 0)
#define TABCOLOR_TAB_FG                        GetSysColor(COLOR_BTNFACE)
#define TABCOLOR_DRAGARROW                     RGB(42, 127, 255)
#define TABCOLOR_PLUSBUTTON                    GetSysColor(COLOR_3DDKSHADOW)
#define TABCOLOR_LINK                          RGB(00,00,255)

extern	PGLOBALS pGlobals;

wyWChar	szCustomTabName[] = L"Custom Tab Control";

wyBool CCustTab::m_isdrawwithdoublebuffering = wyFalse;

struct tab_item_information_ex
{
    tab_item_information    ctcitem;
    wyInt32                 m_tabwidth;
    RECT                    m_tabarea;
    wyInt32                 m_iconindex;
    wyInt32                 m_animationdelay;
};

CCustTab::CCustTab(HWND hwnd)
{
    HDC hdc;
    
    m_istabchanging = wyFalse;
    m_hdcantialias = NULL;
	m_tabs = 0;
	m_hwnd = hwnd;
	m_hfont = NULL;
	m_hselectedfont = NULL;
	m_tabdet = NULL;
	m_hpenactivesep = NULL;
	m_hpenclosebutton = NULL;
	m_hpengrey = NULL;
    m_hpenbottomline = NULL;
	m_selectedtab = 0;
	m_starttab = 0;
	m_minimumtabwidth = 20;
	m_tabheight	= 0;
	m_isdragging = wyFalse;
	m_isfixedlength = wyFalse;
	m_hwndtooltip = NULL;
	m_mouseprevpt.x = 0;
	m_mouseprevpt.y = 0;
    lpTBWndProc = NULL;
	m_isdragenabled = wyFalse;
    m_hwnddragwin = NULL; 
	m_isdragmagepresent = wyFalse;
	m_himl = NULL;
    m_isscroll = wyFalse;
    m_scrollpos = 0;
    m_maxscrollpos = 0;
    m_leftmostvisibletab = 0;
    m_rightmostvisibletab = 0;
    m_lastmouseovertab = -1;
    m_isplussign = wyFalse;
    m_ismouseoverclose = wyFalse;
    m_closebuttondowntab = -1;
    m_dragarrowtab = -2;
    m_isdrawonlytabheader = wyFalse;
    m_mbuttondowntab = -1;
    m_rbuttondowntab = -1;
    m_extflagclose = wyFalse;
    m_mintabcount = 0;
    m_isblock = wyFalse;
    m_hiconlist = NULL;
    m_tabcontroldown = -1;
    m_overtabcontrol = -1;
    m_lparamdata = NULL;
	m_prevtab = 0;
    
	hdc = GetDC(hwnd);
    GetTextExtentPoint32(hdc, L">", 1, &m_size);
    ReleaseDC(hwnd, hdc);
    m_hwndprevfocus = NULL;
    SetColorInfo(NULL);
}

CCustTab::~CCustTab()
{
    wyInt32 tabcount;
		
	if(m_tabdet)
	{
		for(tabcount = 0; tabcount < m_tabs; tabcount++)
                {
			FreeItem(&m_tabdet[tabcount], tabcount);
                }
			
		free(m_tabdet);
	}
	
	m_tabdet = NULL;

	if(m_himl)
    {
        VERIFY(ImageList_Destroy(m_himl)); 
        m_himl = NULL;
    }
		
	if(m_hfont)
	{
		DeleteFont(m_hfont);
	}

    if(m_hselectedfont)
    {
        DeleteFont(m_hselectedfont);
    }

	if(m_hpenactivesep)
	{
		DeletePen(m_hpenactivesep);
	}

	if(m_hpenclosebutton)
	{
		DeletePen(m_hpenclosebutton);
	}

	if(m_hpengrey)
	{
		DeletePen(m_hpengrey);
	}

    if(m_hpenbottomline)
    {
        DeletePen(m_hpenbottomline);
    }

    if(m_hpenhighlight)
    {
        DeletePen(m_hpenhighlight);
    }
}

/*	Window procedure for the Tab window*/
LRESULT CALLBACK
CCustTab::CustomTabWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  	CCustTab *pct = GetCustTabCtrlData(hwnd);
    LPNMHDR lpnmhdr;
    POINT pt;
    LRESULT lres = 0;
    wyBool  ishandled = wyFalse;
    HDC hdc;
    RECT rcwnd, rcclient, rect;
    HBRUSH hbr;

    if(message != WM_NCCREATE && pct && pct->lpTBWndProc != NULL)
    {
        lres = pct->lpTBWndProc(hwnd, message, wParam, lParam, &ishandled);

        if(ishandled)
        {
            return lres;
        }
    }

	switch(message)
	{
	case WM_NCCREATE:
		pct = new CCustTab(hwnd);
		SetCustTabCtrlData(hwnd,(CCustTab *)pct);
		pct->CreateFonts();
		return TRUE;

    case UM_PAINTTABHEADER:
        if(pct->m_tabdet != NULL)
        {
            pct->PaintTab(wyTrue);
        }
        return 1;

	case WM_PAINT:
		if(pct->m_tabdet != NULL && pct->m_istabchanging == wyFalse)
		{
			 pct->OnPaint();
			 return 1;
		}
		break;

	case WM_CREATE:
		return pct->OnCreate(lParam);
		
	case TBM_SETLONGDATA:
		return pct->SetLongData(lParam);

	case WM_ERASEBKGND:
		return 1;

	case WM_DESTROY:
		pct->SendTabMessage(CTCN_WMDESTROY);
		pct->DestroyResources();
		break;

	case WM_GETDLGCODE:
		return pct->OnGetDLGCode(lParam);

	case WM_MOUSEMOVE:
		if(pct->m_isblock == wyTrue)
		{
			return 1;
		}
		pct->GetMouseMovement();
        return pct->OnMouseMove(hwnd, wParam, lParam);

	case WM_MOUSELEAVE:
        if(pct->m_isblock == wyTrue)
        {
            return 1;
        }

		pct->OnMouseLeave();
		return 0;

	case WM_LBUTTONUP:
        KillTimer(hwnd, SCROLLTIMER);
        if(pct->m_isblock == wyTrue)
        {
            return 1;
        }

        pct->OnLButtonUp(wParam, lParam);
		break;

	case WM_LBUTTONDOWN:
        if(pct->m_isblock == wyTrue)
        {
            return 1;
        }

        pct->OnLButtonDown(wParam, lParam);
		break;

    case WM_CAPTURECHANGED:
        if(pct->m_isdragging == wyTrue)
        {
            pct->OnDragEnd();
        }

        break;

	case WM_MBUTTONDOWN:
        if(pct->m_isblock == wyTrue)
        {
            return 1;
        }

        pct->OnMButtonDown(hwnd, lParam);
        break;

    case WM_MBUTTONUP:
        if(pct->m_isblock == wyTrue)
        {
            return 1;
        }

        pct->OnMButtonUp(hwnd, lParam);
        break;

	case WM_RBUTTONDOWN:
        if(pct->m_isblock == wyTrue)
        {
            return 1;
        }

        pct->OnRButtonDown(hwnd, lParam);
		break;

    case WM_RBUTTONUP:
        if(pct->m_isblock == wyTrue)
        {
            return 1;
        }

        pct->OnRButtonUp(hwnd, lParam);
		break;

	case WM_LBUTTONDBLCLK:
        if(pct->m_isblock == wyTrue)
        {
            return 1;
        }

		pct->OnLButtonDblClick(hwnd, wParam, lParam);
		break;

	case WM_NCDESTROY:
		delete pct;
		break;
	

	case WM_NOTIFY:
        {
            lpnmhdr =(LPNMHDR)lParam;

            if(lpnmhdr->code == TTN_GETDISPINFO && lpnmhdr->hwndFrom == pct->m_hwndtooltip)
			{
				pct->ToolTipInfo(hwnd, (LPNMTTDISPINFO)lParam);
				return 1;
			}

			break;
        }

    case WM_SIZE:
        pct->OnWMSize();     
        pct->EnsureVisible(pct->m_selectedtab);
        break;

    case WM_HSCROLL:
        if(pct->m_isblock == wyTrue)
        {
            return 1;
        }

        pct->OnHScroll(wParam, lParam);
        return 1;

    case WM_KILLFOCUS:
        if(pct->m_istabchanging == wyFalse)
        {
            pct->EnsureVisible(pct->m_selectedtab);
        }
        return 1;

    case WM_TIMER:
        if(wParam == DRAGSCROLLTIMER)
        {
            pct->OnDragScrollTimer();
            return 0;
        }
        else if(wParam == PAINTTIMER)
        {
            KillTimer(hwnd, PAINTTIMER);
            pct->SendTabMessage(CTCN_PAINTTIMEREND);
        }
        else if(wParam == SCROLLTIMER)
        {
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            
            if(pct->OverTabControls(&pt) == pct->m_tabcontroldown)
            {
                SendMessage(hwnd, WM_HSCROLL, pct->m_tabcontroldown, 0);
            }
        }
        break;

    case UM_SETFOCUS:
        if(pct->m_hwndprevfocus)
        {
            SetFocus(pct->m_hwndprevfocus);
            pct->m_hwndprevfocus = NULL;
        }
        return 1;

    case WM_NCPAINT: 
        if(pct)
        {
            hdc = GetWindowDC(hwnd);
            GetWindowRect(hwnd, &rcwnd);
            GetClientRect(hwnd, &rcclient);
            MapWindowPoints(hwnd, NULL, (LPPOINT)&rcclient, 2);
            hbr = CreateSolidBrush(pct->m_colorinfo.m_tabbg1);

            rect.left = rect.top = 0;
            rect.bottom = rcclient.top - rcwnd.top;
            rect.right = rcwnd.right - rcwnd.left;

            if(rect.right - rect.left && rect.bottom - rect.top)
            {
                FillRect(hdc, &rect, hbr);
            }

            rect.top = rcclient.bottom - rcwnd.top;
            rect.bottom = rect.top + (rcwnd.bottom - rcclient.bottom);
        
            if(rect.right - rect.left && rect.bottom - rect.top)
            {
                FillRect(hdc, &rect, hbr);
            }

            rect.top = 0;
            rect.right = rcclient.left - rcwnd.left;
        
            if(rect.right - rect.left && rect.bottom - rect.top)
            {
                FillRect(hdc, &rect, hbr);
            }

            rect.left = rcclient.right - rcwnd.left;
            rect.right = rect.left + (rcwnd.right - rcclient.right);
        
            if(rect.right - rect.left && rect.bottom - rect.top)
            {
                FillRect(hdc, &rect, hbr);
            }

            DeleteBrush(hbr);
            ReleaseDC(hwnd, hdc);
            return 0;
        }

        break;

	default:
		break;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

void 
CCustTab::SetPaintTimer()
{
    SendTabMessage(CTCN_PAINTTIMERSTART);
    SetTimer(m_hwnd, PAINTTIMER, 10, NULL);
}

void 
CCustTab::OnDragScrollTimer()
{
    wyInt32 i;
    POINT pnt;

    GetCursorPos(&pnt);
    ScreenToClient(m_hwnd, &pnt);

    i = OverTabControls(&pnt);

    if(i == -1)
    {
        return;
    }

    if(i == IDC_LEFTSCROLL)
    {
        if(m_scrollpos == 0)
        {
            if(m_dragarrowtab == -1 || m_leftmostvisibletab == m_selectedtab)
            {
                return;
            }
        }

        m_scrollpos = max(m_scrollpos - SCROLLTHRESHOLD, 0);
        SetVisibleTabBoundary();
    }
    else if(i == IDC_RIGHTSCROLL)
    {
        if(m_scrollpos == m_maxscrollpos)
        {
            if(m_dragarrowtab == m_rightmostvisibletab || m_rightmostvisibletab == m_selectedtab)
            {
                return;
            }
        }

        m_scrollpos = min(m_scrollpos + SCROLLTHRESHOLD, m_maxscrollpos);
        SetVisibleTabBoundary();
    }

    if(m_isdragging == wyTrue)
    {
        m_lastmouseovertab = -1;
        
        if(OnDragMove(0, &pnt) == wyTrue)
        {
            i = GetDragArrowTab(&pnt);

            if(i + 1 == m_selectedtab || i == m_selectedtab)
            {
                i = -2;
            }
        }
        
        m_dragarrowtab = i;
    }
    
    PaintTab(wyTrue);
}

void 
CCustTab::OnHScroll(WPARAM wparam, LPARAM lparam)
{
    if(wparam == IDC_LEFTSCROLL)
    {
        if(m_scrollpos == 0)
        {
            return;
        }

        m_scrollpos = max(0, m_scrollpos - SCROLLTHRESHOLD);
    }
    else if(wparam == IDC_RIGHTSCROLL)
    {
        if(m_scrollpos == m_maxscrollpos)
        {
            return;
        }

        m_scrollpos = min(m_maxscrollpos, m_scrollpos + SCROLLTHRESHOLD);
    }

    SetVisibleTabBoundary();

    if(lparam == 0)
    {
        PaintTab(wyTrue);
    }
}

//Display tooltip
void
CCustTab::ToolTipInfo(HWND hwnd, LPNMTTDISPINFO lpnmtdi)
{
    wyInt32 i;
    POINT pt;
    wyBool isoverclose;

    if(m_isblock == wyTrue)
    {
        return;
    }

    GetCursorPos(&pt);
    ScreenToClient(m_hwnd, &pt);

    if(OverTabControls(&pt) != -1)
    {
        return;
    }

    i = OverTabs(&pt, &isoverclose);

    if(isoverclose == wyTrue)
    {
        m_tooltipstr.SetAs(_("Close"));
		lpnmtdi->lpszText = m_tooltipstr.GetAsWideChar();
    }
    else if(i != -1)
	{
        m_tooltipstr.SetAs(m_tabdet[i].ctcitem.m_tooltiptext);
		lpnmtdi->lpszText = m_tooltipstr.GetAsWideChar();
	}
	
}
void 
CCustTab::DestroyResources()
{
	if(m_hfont)
		DeleteObject(m_hfont);

	m_hfont = NULL;

	if(m_hselectedfont)
		DeleteObject(m_hselectedfont);

	m_hselectedfont = NULL;
    KillTimer(m_hwnd, PAINTTIMER);
    KillTimer(m_hwnd, DRAGSCROLLTIMER);
}

/*Function is used to initialize the fonts*/
wyBool 
CCustTab::CreateFonts()
{
	HDC			dc;
	wyInt32     fontheight;
	wyString	fontname;

	fontname.SetAs("Verdana");

	dc = GetDC(GetParent(m_hwnd));

    fontheight = -MulDiv(9, GetDeviceCaps(dc, LOGPIXELSY), 80);
	
    ReleaseDC(GetParent(m_hwnd), dc);
	
	if(m_hfont)
		DeleteObject(m_hfont);

	m_hfont = CreateFont(fontheight, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, fontname.GetAsWideChar());

	m_hselectedfont = CreateFont(fontheight, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, 0, 0, 0, 0, 0, fontname.GetAsWideChar());
	
	return wyTrue;
}

LRESULT
CCustTab::OnGetDLGCode(LPARAM lParam)
{
	if(lParam)
		return DLGC_WANTMESSAGE;
	else
		return 0;
}

LRESULT
CCustTab::OnPaint()
{	
    HDC				hdc;
	PAINTSTRUCT		ps;
    HBITMAP         hbmp;

    hdc = BeginPaint(m_hwnd, &ps);
    m_hdcantialias = CreateCompatibleDC(hdc);
    hbmp = CreateCompatibleBitmap(hdc, SCALEANTIALIASING(FIXED_WIDTH + 20), SCALEANTIALIASING(GetTabHeight()));
    hbmp = (HBITMAP)SelectObject(m_hdcantialias, hbmp);

    if(m_isdrawwithdoublebuffering == wyTrue && m_isdrawonlytabheader == wyFalse)
    {
        DrawWithDoubleBuffering(hdc);
    }
    else
    {
        DrawWithoutDoubleBuffering(hdc);
    }

    hbmp = (HBITMAP)SelectObject(m_hdcantialias, hbmp);
    DeleteDC(m_hdcantialias);
    DeleteBitmap(hbmp);
	EndPaint(m_hwnd, &ps);
    return 0;
}

void 
CCustTab::DrawWithoutDoubleBuffering(HDC hdc)
{
    HDC				hdcmem;
	RECT			rectwin;
	HFONT			hfontold;	
	HPEN			hpen,hpenold;
	HBRUSH			hbrush;
	HBITMAP			hbmmem, hbmold;
    wyInt32         tabheight = GetTabHeight();

	hpen = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_CAPTIONTEXT));
	GetClientRect(m_hwnd, &rectwin);

    hdcmem	= CreateCompatibleDC(hdc);
	//GetTextExtentPoint32(hdcmem, L">", 1, &m_size);
    VERIFY(hbmmem	= CreateCompatibleBitmap(hdc, rectwin.right - rectwin.left, tabheight + (2 * TABTOPSPACE)));
	hbmold	=(HBITMAP)SelectObject(hdcmem, hbmmem);	
    VERIFY(hbrush = CreateSolidBrush(m_colorinfo.m_tabbg1));
	hpenold =(HPEN)SelectObject(hdcmem, hpen);	
    
    if(m_isdrawonlytabheader == wyFalse)
    {
        rectwin.top = tabheight + (2 * TABTOPSPACE);
        FillRect(hdc, &rectwin, hbrush);
        rectwin.top = 0;
    }

	FillRect(hdcmem, &rectwin, hbrush);
	hfontold = (HFONT)SelectObject(hdcmem,(HGDIOBJ)m_hfont);
	DrawTabs(hdcmem);
    SelectObject(hdcmem, (HGDIOBJ)hpenold);

    if(m_hpenbottomline)
    {
        hpenold =(HPEN)SelectObject(hdcmem, m_hpenbottomline);
        MoveToEx(hdcmem, -1, tabheight - (2 * TABTOPSPACE), NULL);
        LineTo(hdcmem, rectwin.right, tabheight - (2 * TABTOPSPACE));
        SelectObject(hdcmem, hpenold);
    }

	BitBlt(hdc, rectwin.left, rectwin.top, rectwin.right, tabheight + (2 * TABTOPSPACE), hdcmem, 0, 0, SRCCOPY);
	
    SelectObject(hdcmem, (HGDIOBJ)hbmold);
	SelectObject(hdcmem,(HGDIOBJ)hfontold);
	DeleteObject((HGDIOBJ)hbrush);
	DeleteObject((HGDIOBJ)hbmmem);
	DeleteObject((HGDIOBJ)hpen);
	DeleteDC(hdcmem);
}

void 
CCustTab::DrawWithDoubleBuffering(HDC hdc)
{
	RECT			rectwin;
	HFONT			hfontold;	
	HPEN			hpen,hpenold;
    DoubleBuffer    db(m_hwnd, hdc);
    NMCTC           nmctc = {0};
    wyInt32         tabheight = GetTabHeight();

    db.EraseBackground(m_colorinfo.m_tabbg1);
    hpen = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_CAPTIONTEXT));
	GetClientRect(m_hwnd, &rectwin);
	GetTextExtentPoint32(db.m_buffer.m_hmemdc, L">", 1, &m_size);
	hpenold =(HPEN)SelectObject(db.m_buffer.m_hmemdc, hpen);	
    hfontold = (HFONT)SelectObject(db.m_buffer.m_hmemdc,(HGDIOBJ)m_hfont);
    DrawTabs(db.m_buffer.m_hmemdc);

	SelectObject(db.m_buffer.m_hmemdc, (HGDIOBJ)hpenold);
	SelectObject(db.m_buffer.m_hmemdc,(HGDIOBJ)hfontold);

    if(m_hpenbottomline)
    {
        hpenold =(HPEN)SelectObject(db.m_buffer.m_hmemdc, m_hpenbottomline);
        MoveToEx(db.m_buffer.m_hmemdc, -1, tabheight - (2 * TABTOPSPACE), NULL);
        LineTo(db.m_buffer.m_hmemdc, rectwin.right, tabheight - (2 * TABTOPSPACE));
        SelectObject(db.m_buffer.m_hmemdc, hpenold);
    }

    if(m_isdrawonlytabheader == wyTrue)
    {
        rectwin.bottom = tabheight + (2 * TABTOPSPACE);
        db.CopyBufferToScreen(-1, &rectwin);
    }
    else
    {    
        SendTabMessage(CTCN_GETCHILDWINDOWS, &nmctc);

        if(nmctc.retvalue)
        {
            db.PaintWindow(nmctc.phwnd, nmctc.count);
        }
        else
        {
            db.PaintWindow();
        }
    }

	DeleteObject((HGDIOBJ)hpen);
}

void
CCustTab::DrawDragTabImage(PRECT rect)
{
    HDC hdc, hmemdc;
    HBITMAP hbitmap, holdbitmap;
    wyInt32    id;
    BITMAPINFO   bi;
    VOID *pvbits;

    if(m_isdragging == wyTrue)
    {
        hdc = GetDC(m_hwnd);

        hmemdc = CreateCompatibleDC(hdc);

        ZeroMemory(&bi.bmiHeader, sizeof(BITMAPINFOHEADER));
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);   
        bi.bmiHeader.biWidth = rect->right + 1 - rect->left;   
        bi.bmiHeader.biHeight = rect->bottom - rect->top; 
        bi.bmiHeader.biPlanes = 1;   
        bi.bmiHeader.biBitCount = 24;   
        bi.bmiHeader.biCompression = BI_RGB;   
        bi.bmiHeader.biSizeImage = 0; 
        bi.bmiHeader.biXPelsPerMeter = 0;   
        bi.bmiHeader.biYPelsPerMeter = 0;   
        bi.bmiHeader.biClrUsed = 0;   
        bi.bmiHeader.biClrImportant = 0;

        hbitmap = CreateDIBSection(hmemdc, &bi, DIB_RGB_COLORS, &pvbits, NULL, 0);
        holdbitmap = (HBITMAP)SelectObject(hmemdc, hbitmap);
        m_draghotspot.x = rect->left;
        m_draghotspot.y = rect->top;     
               
        if(m_isfixedlength == wyFalse)
        {
            BitBlt(hmemdc, 0, 0, rect->right + 1 - rect->left, rect->bottom - rect->top, hdc, rect->left, rect->top, SRCCOPY);
        }
        else
        {
            BitBlt(hmemdc, 0, 0, rect->right + 1 - rect->left, rect->bottom - rect->top, hdc, rect->left, rect->top, SRCCOPY);
        }

        SelectObject(hmemdc, holdbitmap);

        VERIFY(m_himl = ImageList_Create(rect->right + 1 - rect->left, rect->bottom - rect->top, ILC_COLOR24 | ILC_MASK, 1, 0));

        id = ImageList_Add(m_himl, hbitmap, NULL);
        DeleteObject(hbitmap);
        DeleteDC(hmemdc);
        VERIFY(ReleaseDC(m_hwnd, hdc));
    }
}

void
CCustTab::DrawCloseControls(HDC hdcmem, PRECT rect, wyInt32 tabcount, PPOINT ptcursor)
{
	RECT	rectclose = {0};
    wyInt32 width, height;
    HPEN    holdpen, temppen = NULL;
	
	// Create the rectangular region for the Close button 
    rectclose.left		= (rect->left + m_tabdet[tabcount].m_tabwidth) - (CLOSERECTWIDTH + CLOSERECTPADDING);
	rectclose.right		= (rect->left + m_tabdet[tabcount].m_tabwidth) - CLOSERECTPADDING;
    width = CLOSERECTWIDTH;
    height = rect->bottom - rect->top;
    rectclose.top = ((height / 2) - (width / 2)) + (m_isfixedlength == wyTrue ? TABTOPSPACE : 0);
    rectclose.bottom = rectclose.top + width;

    holdpen = (HPEN)SelectObject(hdcmem, m_hpenclosebutton);
	MoveToEx(hdcmem, rectclose.left + 1, rectclose.top + 1, NULL);

	//if mouse is over close button, then highlight the close button border with red color
    if(m_lastmouseovertab != -1 && PtInRect(&rectclose, *ptcursor) &&  m_tabs > m_mintabcount && (m_closebuttondowntab == -1 || m_closebuttondowntab == tabcount) && 
        m_tabcontroldown == -1)
	{
        if(m_colorinfo.m_closebutton == TABCOLOR_CLOSEBUTTON)
        {
            temppen = CreatePen(PS_SOLID, 0,m_colorinfo.m_closebutton);
            holdpen = (HPEN)SelectObject(hdcmem,temppen);
        }
        else
        {
            if(tabcount == m_selectedtab)
            {
                if((m_tabdet[tabcount].ctcitem.m_mask & CTBIF_COLOR)  && (m_tabdet[tabcount].ctcitem.m_color  >= 0) && m_isfixedlength)
                    temppen = CreatePen(PS_SOLID, 0, m_tabdet[tabcount].ctcitem.m_fgcolor);
                else
                    temppen = CreatePen(PS_SOLID, 0, m_colorinfo.m_seltabtext);
            
            holdpen = (HPEN)SelectObject(hdcmem,temppen);
            //SetGradientArrayOnMouseOver(&m_tabdet[tabcount].ctcitem, &rectclose, vertex, wyTrue);
            }
            else
                holdpen = (HPEN)SelectObject(hdcmem,m_hpenhighlight);
        }
       // DrawCloseButtonBorder(hdcmem, &rectclose);
	}	

	DrawCloseButton(hdcmem, &rectclose, ptcursor);
    if(temppen)
        DeleteObject(temppen);
    SelectObject(hdcmem, holdpen);
}

void
CCustTab::SetGradientToTabRibbon(HDC hdcmem, PRECT rectwin, LPTRIVERTEX vertex)
{
	//Sets the Gradient to Tab ribbon
	vertex[0].x     = rectwin->left;
	vertex[0].y     = rectwin->top;

	vertex[1].x     = rectwin->right;
	vertex[1].y     = rectwin->bottom + 2; 

	vertex[0].Red   = m_red;
	vertex[0].Green = m_green;
	vertex[0].Blue  = m_blue;
	vertex[0].Alpha = 0x000000;
		
    vertex[1].Red   = (COLOR16)(GetRValue(m_colorinfo.m_tabbg2) << 8);
	vertex[1].Green = (COLOR16)(GetGValue(m_colorinfo.m_tabbg2) << 8);
	vertex[1].Blue  = (COLOR16)(GetBValue(m_colorinfo.m_tabbg2) << 8);
	vertex[1].Alpha = 0x0000;
			
	SetGradient(hdcmem, vertex);
}

void  
CCustTab::SetGradientArrayOnFocus(LPCTCITEM pitem, PRECT prect, LPTRIVERTEX vertex)
{
	vertex[1].x     = prect->right + (m_isfixedlength == wyTrue ? 0 : 1);
	vertex[1].y     = prect->bottom + TABTOPSPACE + 1; 			
   
	if((pitem->m_mask & CTBIF_COLOR)  && (pitem->m_color  >= 0) && m_isfixedlength)
	{
		vertex[1].Red   = (COLOR16)(GetRValue(pitem->m_color) << 8);
		vertex[1].Green = (COLOR16)(GetGValue(pitem->m_color) << 8);
		vertex[1].Blue  = (COLOR16)(GetBValue(pitem->m_color) << 8);
		vertex[1].Alpha = 0x0000;
		
        /*t1 = vertex[0].Red > vertex[0].Green ? vertex[0].Green : vertex[0].Red;
        t1 = t1 > vertex[0].Blue? vertex[0].Blue : t1;*/

		vertex[0].Red   = vertex[1].Red + ( 13 * (0xff00 - vertex[1].Red)/16); //m_red;
		vertex[0].Green = vertex[1].Green + ( 13 * (0xff00 - vertex[1].Green)/16);  //m_green;
		vertex[0].Blue  = vertex[1].Blue + ( 13 * (0xff00 - vertex[1].Blue)/16); //m_blue;
		vertex[0].Alpha = 0x0000;

        //m_colorinfo.m_tabbg1 = RGB(vertex[1].Red, vertex[1].Green, vertex[1].Blue);
	}
	else
	{
		vertex[0].Red   = (COLOR16)(GetRValue(m_colorinfo.m_seltabfg1) << 8);//0xf300;
		vertex[0].Green = (COLOR16)(GetGValue(m_colorinfo.m_seltabfg1) << 8);//0xf100;
		vertex[0].Blue  = (COLOR16)(GetBValue(m_colorinfo.m_seltabfg1) << 8);//0xe600;
		vertex[0].Alpha = 0x0000;
	

    vertex[1].Red   = (COLOR16)(GetRValue(m_colorinfo.m_seltabfg2) << 8);
    vertex[1].Green = (COLOR16)(GetGValue(m_colorinfo.m_seltabfg2) << 8);
    vertex[1].Blue  = (COLOR16)(GetBValue(m_colorinfo.m_seltabfg2) << 8);
    vertex[1].Alpha = 0xfe00;
    
    }

	return;
}

void  
CCustTab::SetGradientArrayOnMouseOver(LPCTCITEM pitem, PRECT prect, LPTRIVERTEX vertex, wyBool isselected)
{
    if(pitem)
    {
	    vertex[1].x     = prect->right + (m_isfixedlength == wyTrue ? -1 : 1);
	vertex[1].y     = prect->bottom + TABTOPSPACE + 1; 
    }
		
	if(pitem && (pitem->m_mask & CTBIF_COLOR)  && (pitem->m_color  >= 0) && m_isfixedlength)
	{
		if(isselected)
        {
            vertex[0].Red   = (COLOR16)(GetRValue(pitem->m_color) << 8);
		    vertex[0].Green = (COLOR16)(GetGValue(pitem->m_color) << 8);
		    vertex[0].Blue  = (COLOR16)(GetBValue(pitem->m_color) << 8);
		    vertex[0].Alpha = 0x0000;
            vertex[1].Red   = 0xef00;
	        vertex[1].Green = 0xef00;
	        vertex[1].Blue  = 0xef00;
	        vertex[1].Alpha = 0xfe00;
        }
        else
        {
            vertex[1].Red   = (COLOR16)(GetRValue(pitem->m_color) << 8);
		    vertex[1].Green = (COLOR16)(GetGValue(pitem->m_color) << 8);
		    vertex[1].Blue  = (COLOR16)(GetBValue(pitem->m_color) << 8);
		    vertex[1].Alpha = 0x0000;

            vertex[0].Red   = vertex[1].Red   = vertex[1].Red + ( 7 * (0xff00 - vertex[1].Red)/16); //m_red;
		    vertex[0].Green = vertex[1].Green = vertex[1].Green + ( 7 * (0xff00 - vertex[1].Green)/16);  //m_green;
		    vertex[0].Blue  = vertex[1].Blue  = vertex[1].Blue + ( 7 * (0xff00 - vertex[1].Blue)/16); //m_blue;
		    vertex[0].Alpha = 0xfe00;
        }
	}
	else
	{
        if(isselected)
        {
            vertex[1].Red   = (COLOR16)(GetRValue(m_colorinfo.m_seltabfg2) << 8);
		    vertex[1].Green = (COLOR16)(GetGValue(m_colorinfo.m_seltabfg2) << 8);
		    vertex[1].Blue  = (COLOR16)(GetBValue(m_colorinfo.m_seltabfg2) << 8);
		    vertex[1].Alpha = 0xfe00;

            vertex[0].Red   = vertex[1].Red   = vertex[1].Red + ( 4 * (0xff00 - vertex[1].Red)/16); //m_red;
		    vertex[0].Green = vertex[1].Green = vertex[1].Green + ( 4 * (0xff00 - vertex[1].Green)/16);  //m_green;
		    vertex[0].Blue  = vertex[1].Blue  = vertex[1].Blue + ( 4 * (0xff00 - vertex[1].Blue)/16); //m_blue;
		    vertex[0].Alpha = 0x0000;
        }
        else
        {
            vertex[0].Red   = (COLOR16)(GetRValue(m_colorinfo.m_hottabfg1) << 8);
		    vertex[0].Green = (COLOR16)(GetGValue(m_colorinfo.m_hottabfg1) << 8);
		    vertex[0].Blue  = (COLOR16)(GetBValue(m_colorinfo.m_hottabfg1) << 8);
    		vertex[0].Alpha = 0x0000;

            vertex[1].Red   = (COLOR16)(GetRValue(m_colorinfo.m_hottabfg1) << 8);
		    vertex[1].Green = (COLOR16)(GetGValue(m_colorinfo.m_hottabfg1) << 8);
		    vertex[1].Blue  = (COLOR16)(GetBValue(m_colorinfo.m_hottabfg1) << 8);
    		vertex[1].Alpha = 0x0000;
        }
	}
	return;
}

void  
CCustTab::SetGradientArrayOnUnFocus(LPCTCITEM pitem, PRECT prect, LPTRIVERTEX vertex)
{
	wyInt32		tabred, tabblue, tabgreen;

    if(pitem)
    {
        vertex[1].x     = prect->right + (m_isfixedlength == wyTrue ? -1 : 1);
	    vertex[1].y     = prect->bottom + TABTOPSPACE + 1; 
    }

	if(pitem && (pitem->m_mask & CTBIF_COLOR)  && (pitem->m_color >= 0) && m_isfixedlength)
	{
		tabred		= (COLOR16)(GetRValue(pitem->m_color) << 8);
		tabgreen	= (COLOR16)(GetGValue(pitem->m_color) << 8);
		tabblue		= (COLOR16)(GetBValue(pitem->m_color) << 8);

		vertex[0].Red   = tabred;
		vertex[0].Green = tabgreen;
		vertex[0].Blue  = tabblue;
		vertex[0].Alpha = 0;

		vertex[1].Red   = tabred;
		vertex[1].Green = tabgreen;
		vertex[1].Blue  = tabblue;
		vertex[1].Alpha = 0;
	}
	else
	{
        tabred		= (COLOR16)(GetRValue(m_colorinfo.m_tabbg1) << 8);
		tabgreen	= (COLOR16)(GetGValue(m_colorinfo.m_tabbg1) << 8);
		tabblue		= (COLOR16)(GetBValue(m_colorinfo.m_tabbg1) << 8);

		vertex[0].Red   = tabred;//m_red;//0xe500;
        vertex[0].Green = tabgreen;// m_green;//0xe400;
        vertex[0].Blue  = tabblue; //m_blue;//0xd800;
		vertex[0].Alpha = 0x0000;

		vertex[1].Red   = tabred;//0xdd00;
		vertex[1].Green = tabgreen;//0xdc00;
		vertex[1].Blue  = tabgreen;//0xd800;
		vertex[1].Alpha = 0x0000;

    }

	return;
}


/*Function is used to Draw the border when the mouse is moved over it*/
void 
CCustTab::DrawCloseButtonBorder(HDC hdcmem, PRECT rectclose)
{
	MoveToEx(hdcmem, rectclose->left , rectclose->bottom, NULL);
	LineTo	(hdcmem, rectclose->right , rectclose->bottom);
	LineTo (hdcmem, rectclose->right , rectclose->top );
	LineTo	(hdcmem, rectclose->left , rectclose->top);
	LineTo (hdcmem, rectclose->left , rectclose->bottom );
}

/*	Function is used to Set the close button flag*/
void 
CCustTab::SetClosable(wyBool val, wyInt32 mintabcount, wyBool ispaint)
{
	m_extflagclose = val;

    if(mintabcount >= 0)
    {
        m_mintabcount = mintabcount;
    }

	if(ispaint == wyTrue)
	{
    PaintTab(wyTrue);
}
}

void 
CCustTab::SetDragProperties(HWND hwnddragwin, wyBool val)
{
	if(val == wyFalse)
	{
		m_isdragenabled = wyFalse;
		m_hwnddragwin   = NULL;
	}
	else
	{
		m_isdragenabled = wyTrue;
		m_hwnddragwin   = hwnddragwin;
	}
}

void
CCustTab::SetBlockEventsFlag(wyBool val)
{
	m_isblock = val;
}

void 
CCustTab::SetFixedLengthProp(wyBool val)
{
	m_isfixedlength = val;
}


/*Function is used to Draw close button*/
void 
CCustTab::DrawCloseButton(HDC hdcmem, PRECT rectclose, PPOINT pt)
{
    wyBool ispressed = wyFalse;

    if(PtInRect(rectclose, *pt) && m_closebuttondowntab != -1 && m_closebuttondowntab == m_lastmouseovertab)
    {
        ispressed = wyTrue;
    }

    MoveToEx(hdcmem, rectclose->left + (ispressed ? 4 : 3), rectclose->bottom - (ispressed ? 4 : 3), NULL);
	LineTo	(hdcmem, rectclose->right - (ispressed ? 3 : 2), rectclose->top + (ispressed ? 3 : 2));
	MoveToEx(hdcmem, rectclose->left + (ispressed ? 5 : 4), rectclose->bottom - (ispressed ? 4 : 3), NULL);
	LineTo	(hdcmem, rectclose->right - (ispressed ? 3 : 2), rectclose->top + (ispressed ? 4 : 3));
	MoveToEx(hdcmem, rectclose->left + (ispressed ? 4 : 3), rectclose->bottom - (ispressed ? 5 :4), NULL);
	LineTo	(hdcmem, rectclose->right - (ispressed ? 4 : 3), rectclose->top + (ispressed ? 3 : 2));
	MoveToEx(hdcmem, rectclose->left + (ispressed ? 4 : 3), rectclose->top + (ispressed ? 4 : 3) , NULL);
	LineTo	(hdcmem, rectclose->right - (ispressed ? 3 : 2), rectclose->bottom - (ispressed ? 3 : 2));
	MoveToEx(hdcmem, rectclose->left + (ispressed ? 5 : 4), rectclose->top + (ispressed ? 4 : 3) , NULL);
	LineTo	(hdcmem, rectclose->right - (ispressed ? 3 : 2), rectclose->bottom - (ispressed ? 4 : 3));
	MoveToEx(hdcmem, rectclose->left + (ispressed ? 4 : 3), rectclose->top + (ispressed ? 5 : 4) , NULL);
	LineTo	(hdcmem, rectclose->right - (ispressed ? 4 : 3), rectclose->bottom - (ispressed ? 3 : 2));
}

LRESULT
CCustTab::SetLongData(LPARAM lParam)
{
	this->m_lparamdata = lParam;

	return TRUE;
}

/*Function is used to Add a tab*/
wyInt32	
CCustTab::InsertItem(wyInt32 position, LPCTCITEM pitem)
{
	wyInt32 i;
    HDC hdc;

	if(position < 0)
		return -1;

	if(m_tabdet)
		m_tabdet = (LPCTCITEMEX)realloc(m_tabdet, sizeof(CTCITEMEX)* ++m_tabs);
	else
		m_tabdet = (LPCTCITEMEX)calloc(sizeof(CTCITEMEX), ++m_tabs);

		if(position > m_tabs)
			position = m_tabs - 1;

	
	if(position < (m_tabs - 1))
	{
		/* We have to copy the push the Current content one place */
		for(i = m_tabs - 1; i > position; i--)
		{
            if(m_tabdet[i-1].ctcitem.m_mask & CTBIF_TEXT)
			{
				if((i !=(m_tabs - 1))&&(m_tabdet[i].ctcitem.m_mask & CTBIF_TEXT))
					m_tabdet[i].ctcitem.m_psztext =(wyChar *)realloc(m_tabdet[i].ctcitem.m_psztext, (m_tabdet[i-1].ctcitem.m_cchtextmax + 1) * sizeof(wyChar));
				else if(i ==(m_tabs - 1))
					m_tabdet[i].ctcitem.m_psztext =(wyChar *)calloc(m_tabdet[i-1].ctcitem.m_cchtextmax + 1, sizeof(wyChar));

				strncpy(m_tabdet[i].ctcitem.m_psztext, m_tabdet[i-1].ctcitem.m_psztext, m_tabdet[i-1].ctcitem.m_cchtextmax);
                m_tabdet[i].ctcitem.m_psztext[m_tabdet[i-1].ctcitem.m_cchtextmax] = 0;
			}
			else
			{
				m_tabdet[ i - 1 ].ctcitem.m_psztext = 0;
			}

			//if tooltip is present
			if(m_tabdet[i-1].ctcitem.m_mask & CTBIF_TOOLTIP)
			{
				if((i !=(m_tabs - 1))&&(m_tabdet[i].ctcitem.m_mask & CTBIF_TOOLTIP))
					m_tabdet[i].ctcitem.m_tooltiptext =(wyChar *)realloc(m_tabdet[i].ctcitem.m_tooltiptext, (strlen(m_tabdet[i-1].ctcitem.m_tooltiptext) + 1) * sizeof(wyChar));
				else if(i ==(m_tabs - 1))
					m_tabdet[i].ctcitem.m_tooltiptext =(wyChar *)calloc(strlen(m_tabdet[i-1].ctcitem.m_tooltiptext) + 1, sizeof(wyChar));

				strcpy(m_tabdet[i].ctcitem.m_tooltiptext, m_tabdet[i-1].ctcitem.m_tooltiptext);
			}
			else
			{
				m_tabdet[ i - 1 ].ctcitem.m_tooltiptext = 0;
			}
		
			m_tabdet[ i ].ctcitem.m_iimage		= m_tabdet[i-1].ctcitem.m_iimage;
			m_tabdet[ i ].ctcitem.m_mask		= m_tabdet[i-1].ctcitem.m_mask;
			m_tabdet[ i ].ctcitem.m_cchtextmax	= m_tabdet[i-1].ctcitem.m_cchtextmax;
			m_tabdet[ i ].ctcitem.m_color		= m_tabdet[i-1].ctcitem.m_color;
			m_tabdet[ i ].ctcitem.m_fgcolor		= m_tabdet[i-1].ctcitem.m_fgcolor;
            m_tabdet[ i ].m_tabwidth            = m_tabdet[i - 1].m_tabwidth;
            m_tabdet[ i ].m_animationdelay      = m_tabdet[i - 1].m_animationdelay;
            m_tabdet[ i ].m_iconindex           = m_tabdet[i - 1].m_iconindex;
			
			

			if(m_tabdet[i-1].ctcitem.m_mask & CTBIF_LPARAM)
				m_tabdet[ i ].ctcitem.m_lparam = m_tabdet[i-1].ctcitem.m_lparam;
			else
				m_tabdet[ i ].ctcitem.m_lparam = NULL;
		}
	}

	if(pitem->m_mask & CTBIF_TEXT)
	{
		if(position == m_tabs - 1)
			m_tabdet[position].ctcitem.m_psztext =(wyChar *)calloc(pitem->m_cchtextmax + 1, sizeof(wyChar));
		else
			m_tabdet[position].ctcitem.m_psztext =(wyChar *)realloc(m_tabdet[position].ctcitem.m_psztext, (pitem->m_cchtextmax + 1) * sizeof(wyChar));

		strncpy(m_tabdet[position].ctcitem.m_psztext, pitem->m_psztext, pitem->m_cchtextmax);
        m_tabdet[position].ctcitem.m_psztext[pitem->m_cchtextmax] = 0;
	}
	else
	{
		m_tabdet[position].ctcitem.m_psztext = 0;
	}

	//if tooltip is present
	if(pitem->m_mask & CTBIF_TOOLTIP)
	{
		if(position == m_tabs - 1)
			m_tabdet[position].ctcitem.m_tooltiptext =(wyChar *)calloc(strlen(pitem->m_tooltiptext) + 1, sizeof(wyChar));
		else
			m_tabdet[position].ctcitem.m_tooltiptext =(wyChar *)realloc(m_tabdet[position].ctcitem.m_tooltiptext, (strlen(pitem->m_tooltiptext) + 1) * sizeof(wyChar));

		strcpy(m_tabdet[position].ctcitem.m_tooltiptext, pitem->m_tooltiptext);
	}
	else
	{
		m_tabdet[position].ctcitem.m_tooltiptext = 0;
	}
	
	m_tabdet[position].ctcitem.m_iimage			= pitem->m_iimage;
	m_tabdet[position].ctcitem.m_mask			= pitem->m_mask;
	m_tabdet[position].ctcitem.m_cchtextmax		= pitem->m_cchtextmax;
	m_tabdet[position].ctcitem.m_color			= pitem->m_color;
	m_tabdet[position].ctcitem.m_fgcolor		= pitem->m_fgcolor;
	
	
    m_tabdet[position].m_iconindex              = -1;
    m_tabdet[position].m_animationdelay         = -1;

	if(pitem->m_mask & CTBIF_LPARAM)
		m_tabdet[position].ctcitem.m_lparam = pitem->m_lparam;
	else
		m_tabdet[position].ctcitem.m_lparam = NULL;

    hdc = GetDC(m_hwnd);
    m_tabdet[position].m_tabwidth       = CalculateTabLength(hdc, position);
    ReleaseDC(m_hwnd, hdc);

	return position;
}

/*Function is used to select a particular tab*/
wyBool  
CCustTab::SetCurSel(wyInt32 index, WPARAM wparam)
{
    NMCTC nmctc = {0};

	if(index >= m_tabs)
		return wyFalse;

	nmctc.count = index;
    m_istabchanging = wyTrue;
	
    if(SendTabMessage(CTCN_SELCHANGING, &nmctc, wparam) == 0)
    {
        m_istabchanging = wyFalse;
        return wyFalse;
    }

    nmctc.count   = m_selectedtab;
	m_selectedtab = index;
    
    if((m_tabdet[m_selectedtab].ctcitem.m_mask & CTBIF_COLOR)  
        && (m_tabdet[m_selectedtab].ctcitem.m_color  >= 0) && m_isfixedlength)
        CreateBottomLinePen();
    
    SendTabMessage(CTCN_SELCHANGE, &nmctc, wparam);
    m_istabchanging = wyFalse;

	return wyTrue;
}

LRESULT	
CCustTab::OnLButtonDown(WPARAM wPram, LPARAM lParam)
{
    POINT pnt;
    wyInt32 i, ret;
    wyBool  isonclosebutton;
    RECT recttab;

    pnt.x = GET_X_LPARAM(lParam);
	pnt.y = GET_Y_LPARAM(lParam);
    m_closebuttondowntab = -1;
    SetCapture(m_hwnd);

    if((ret = OverTabControls(&pnt)) == -1)
    {
        i = OverTabs(&pnt, &isonclosebutton);

        if(i != -1 && isonclosebutton == wyTrue)
        {
            m_closebuttondowntab = i;
        }

		m_prevtab = m_selectedtab;

        if(i != -1)
        {
            if(m_selectedtab != i)
            {
                if(SetCurSel(i, 1) == wyFalse)
                {
                    return 1;
                }

                PaintTab();
            }

            EnsureVisible(m_selectedtab);
            GetCursorPos(&pnt);
            ScreenToClient(m_hwnd, &pnt);

            if(OverTabs(&pnt, NULL) == i)
            {
                if(m_isdragenabled == wyTrue && m_closebuttondowntab == -1 && m_leftmostvisibletab != -1 && m_rightmostvisibletab != -1)
                {
                    GetTabRect(m_selectedtab, &recttab);
                    OnDragStart(&pnt, &recttab);
                }
            }
        }
    }
    else
    {
        m_tabcontroldown = ret;
        PaintTab(wyTrue);

        if(m_tabcontroldown == IDC_LEFTSCROLL || m_tabcontroldown == IDC_RIGHTSCROLL)
        {
            SetFocus(m_hwnd);
            SendMessage(m_hwnd, WM_HSCROLL, (WPARAM)m_tabcontroldown, (LPARAM)0);
            SetTimer(m_hwnd, SCROLLTIMER, 10, NULL);
        }
    }

    return 1;
}

wyBool CCustTab::GetTabRect(wyInt32 tabindex, RECT* prect)
{
    wyInt32 i;

    if(tabindex < 0 || tabindex >= m_tabs || prect == NULL)
    {
        return wyFalse;
    }

    prect->left = 0;

    if(m_isscroll == wyTrue)
    {
        prect->left = SCROLLBUTTONWIDTH;
    }

    prect->left -= m_scrollpos;

    for(i = 0 ; i < tabindex; ++i)
    {
        prect->left += m_tabdet[i].m_tabwidth;
    }

    prect->right = prect->left + m_tabdet[i].m_tabwidth;
    prect->top = 0;
    prect->bottom = m_size.cy + EXTRAHEIGHT - 1;
    return wyTrue;
}


LRESULT	
CCustTab::OnLButtonUp(WPARAM wPram, LPARAM lParam)
{
    POINT pnt;
    NMCTC nmctc = {0};
    wyInt32 i;
    wyBool  isonclosebutton, ispaintonlyheader = wyTrue;

    pnt.x = GET_X_LPARAM(lParam);
	pnt.y = GET_Y_LPARAM(lParam);

    i = OverTabs(&pnt, &isonclosebutton);

    if(m_isdragging == wyTrue)
    {
        OnDragEnd();
    }
    else if(i != -1 && isonclosebutton == wyTrue && m_closebuttondowntab == i)
        {
            if(DeleteItem(i, wyTrue) == wyTrue)
            {
                OnWMSize();
                ispaintonlyheader = wyFalse;

				///for rightly selecting the previous tab when tab being deleted is not same as previously selected tab, or the selected tab is first one
				if(m_prevtab != i)
				{
					if(m_prevtab > i)
						m_prevtab -= 1;	
				}
				else if(m_prevtab != 0 && m_prevtab == m_tabs )
				{
					m_prevtab -= 1;	
				}
				if(SetCurSel(m_prevtab, 1) == wyFalse)
				{
					return 1;						
				}     
			}
	}
    else if((i = OverTabControls(&pnt)) != -1)
    {
        if(i == m_overtabcontrol && i == m_tabcontroldown)
        {
            if(i == IDC_PLUSSIGN)
            {
                ClientToScreen(m_hwnd, &pnt);
                nmctc.curpos = pnt;
                SendTabMessage(CTCN_PLUSBUTTONCLICK, &nmctc);
            }
			else if (i == IDC_DROPDOWN)
			{
				ClientToScreen(m_hwnd, &pnt);
				nmctc.curpos = pnt;
				SendTabMessage(CTCN_DROPDOWNBUTTONCLICK, &nmctc);
			}
            else
            {
                KillTimer(m_hwnd, SCROLLTIMER);
            }
        }
    }
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    m_closebuttondowntab = -1;
    m_tabcontroldown = -1;
    ReleaseCapture();
    PaintTab(ispaintonlyheader);
    PostMessage(m_hwnd, UM_SETFOCUS, 0, 0);
    return 1;
}


void
CCustTab::OnDragStart(PPOINT pnt, PRECT rect)
{
	m_isdragging = wyTrue;		
	DrawDragTabImage(rect);
}

wyInt32
CCustTab::FreeDragResources()
{
    wyInt32 dragarrowtab = -2;

	if(m_isdragenabled == wyTrue)
	{
        KillTimer(m_hwnd, DRAGSCROLLTIMER);
		m_isdragging = wyFalse;
        dragarrowtab = m_dragarrowtab;
        m_dragarrowtab = -2;
		ImageList_DragLeave(m_hwnd);
		ImageList_EndDrag();

		ReleaseCapture();

		m_isdragmagepresent = wyFalse;

		VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));

		if(m_himl)
		{
			VERIFY(ImageList_Destroy(m_himl)); 
			m_himl = NULL;
		}
	}

    return dragarrowtab;
}

void
CCustTab::OnDragEnd()
{
	wyInt32     i, dropindex = 0, iconindex, delay;
	CTCITEMEX	    tabitem = {0};

    if(m_dragarrowtab > -2 && m_dragarrowtab < m_tabs)
    {
        tabitem = m_tabdet[m_selectedtab];

        if(m_dragarrowtab < m_selectedtab)
        {
            dropindex = min(m_dragarrowtab + 1, m_tabs - 1);

            for(i = m_selectedtab; i > dropindex; --i)
            {
                m_tabdet[i] = m_tabdet[i - 1];
            }
        }
        else if(m_dragarrowtab > m_selectedtab)
        {
            dropindex = min(m_dragarrowtab, m_tabs - 1);

            for(i = m_selectedtab; i < dropindex; ++i)
            {
                m_tabdet[i] = m_tabdet[i + 1];
            }
        }

		//m_selectedtab and dropindex , find the node store in a tem and insert before and after wyInt32         m_selectedtab
		FindAndUpdateTheDropDown(m_selectedtab, dropindex);

        m_tabdet[dropindex] = tabitem;
        m_selectedtab = dropindex;
    }

    if(FreeDragResources() != -2)
    {
        PaintTab(wyTrue);
        
        for(i = 0; i < m_tabs; ++i)
        {
            iconindex = m_tabdet[i].m_iconindex;
            delay = m_tabdet[i].m_animationdelay;

            if(iconindex != -1)
            {
                StartIconAnimation(i, delay);
            }
            else
            {
                StopIconAnimation(i, wyFalse);
            }
        }
    }

    EnsureVisible(m_selectedtab);

}

void
CCustTab::FindAndUpdateTheDropDown(wyInt32 selectedtab,wyInt32 dropindex)
{
	wyInt32 nooftab, conncount=0;
	wyBool isconndrag = wyFalse;

	wyString titleclicked("");
	GetItemTitle(selectedtab, &titleclicked);
	//find the name and type of the tab and change the location of that perticular tab only
	ListofOpenTabs * listofopentabs = (ListofOpenTabs *)pGlobals->m_connectiontabnamelist->GetFirst();

	//getting the count of open connections
	 conncount = pGlobals->m_connectiontabnamelist->GetCount();
	for (nooftab = 0; nooftab < conncount; nooftab++)
	{
		titleclicked.LTrim();
		titleclicked.RTrim();
		//int y = title1->CompareI(listofopentabs->name.GetString());//getting name of the connection
		if (titleclicked.CompareI(listofopentabs->name.GetString())==0)
		{
			isconndrag = wyTrue;
			break;
		}
		listofopentabs = (ListofOpenTabs *)listofopentabs->m_next; //moving pointer to next conenction window
	}
	if (isconndrag)
	{
		UpdateDropDownNodePosforconnection(m_selectedtab, dropindex);
	}
	else
	{
		UpdateDropDownStructPosition(m_selectedtab, dropindex);
	}

}

void	
CCustTab::UpdateDropDownStructPosition(wyInt32 selectedtab, wyInt32 dropindex) 
{
	//to initialise the structure for drop down
	MDIListForDropDrown *p = (MDIListForDropDrown *)pGlobals->m_mdilistfordropdown->GetFirst();
	wyBool found = wyFalse;
	ListOfOpenQueryTabs *listofopentabs,*currnodefroseltab, *currnodefroindextab,*prevsel;
	wyBool dragnodefound = wyFalse, selnodefound = wyFalse;
	wyInt32 tabcount = 0;

	MDIWindow *wnd = GetActiveWin();
	if(!wnd)
	{
		return;
	}
	if (!p)
	{
		return;
	}

	//To search for the particular tab
	while (p)
	{
		if (wnd == p->mdi)
		{
			found = wyTrue;
			break;
		}
		p = (MDIListForDropDrown *)p->m_next;
	}

	if (found) {
		listofopentabs = (ListOfOpenQueryTabs *)p->opentab->GetFirst();
		if(!listofopentabs)
		{
			return;
		}
		tabcount = p->opentab->GetCount();

		for (int i = 0; i < tabcount; i++)
		{
			if (i == dropindex && dragnodefound==wyFalse) //not to change the index if dragindex is found once
			{
				prevsel = (ListOfOpenQueryTabs *)listofopentabs->m_prev;
				currnodefroindextab = listofopentabs;
				dragnodefound = wyTrue;
			}
			if (i == selectedtab && selnodefound==wyFalse) //not to change the index if selindex is found once
			{
				currnodefroseltab = listofopentabs;
				selnodefound = wyTrue;
			}
			if (selnodefound && dragnodefound) //if both node are found
			{
				break;
			}

			listofopentabs = (ListOfOpenQueryTabs *)listofopentabs->m_next;

		}
		if (selnodefound && dragnodefound) //if both node are found
		{
			if (dropindex<selectedtab)
			{
				if (dropindex == 0)
				{
					p->opentab->Remove(currnodefroseltab);
					p->opentab->InsertBefore(currnodefroindextab, currnodefroseltab);
					
				}
				else
				{
					p->opentab->Remove(currnodefroseltab);
					p->opentab->InsertAfter(prevsel, currnodefroseltab);
				}

			}
			else
			{
				p->opentab->Remove(currnodefroseltab);
				p->opentab->InsertAfter(currnodefroindextab, currnodefroseltab);
			}

		}

	}

}

void
CCustTab::UpdateDropDownNodePosforconnection(wyInt32 selectedtab, wyInt32 dropindex)
{
	ListofOpenTabs   *listofopentabs, *currnodefroseltab, *currnodefroindextab,*prevsel;
	wyBool dragnodefound = wyFalse, selnodefound = wyFalse;
	wyInt32 conncount=0;
	
	listofopentabs = (ListofOpenTabs *)pGlobals->m_connectiontabnamelist->GetFirst();
	List *p = pGlobals->m_connectiontabnamelist;

	if (!listofopentabs)
	{
		return;
	}
		
	conncount = pGlobals->m_connectiontabnamelist->GetCount();

		for (int i = 0; i < conncount; i++)
		{
			if (i == dropindex && dragnodefound == wyFalse) //not to change the index if dragindex is found once
			{
				prevsel = (ListofOpenTabs *)listofopentabs->m_prev;
				currnodefroindextab = listofopentabs;
				dragnodefound = wyTrue;
			}
			if (i == selectedtab && selnodefound == wyFalse) //not to change the index if selindex is found once
			{
				currnodefroseltab = listofopentabs;
				selnodefound = wyTrue;
			}
			if (selnodefound && dragnodefound) //if both node are found
			{
				break;
			}

			listofopentabs = (ListofOpenTabs *)listofopentabs->m_next;

		}
		if (selnodefound && dragnodefound) //if both node are found
		{
			if(dropindex<selectedtab)
			{
				if (dropindex == 0)
				{
					p->Remove(currnodefroseltab);
					p->InsertBefore(currnodefroindextab, currnodefroseltab);
				}
				else
				{
					p->Remove(currnodefroseltab);
					p->InsertAfter(prevsel, currnodefroseltab);
				}

			}
			else
			{
				p->Remove(currnodefroseltab);
				p->InsertAfter(currnodefroindextab, currnodefroseltab);
			}
			
		}

}

wyInt32  
CCustTab::OverTabs(PPOINT pnt, wyBool* pisoverclosebutton)
{
    wyInt32 lefttab, righttab, i, left = 0;
    RECT    recttab, rectclose = {0}, cliprect;

    lefttab = max(m_leftmostvisibletab - 1, 0);
    righttab = min(m_rightmostvisibletab + 1, m_tabs - 1);
    GetClientRect(m_hwnd, &cliprect);

    if(pisoverclosebutton)
    {
        *pisoverclosebutton = wyFalse;
    }

    if(m_isscroll == wyTrue)
    {
        left = SCROLLBUTTONWIDTH;
        cliprect.left = SCROLLBUTTONWIDTH;
        cliprect.right -= RIGHTPADDING(m_isplussign);
    }

    left -= m_scrollpos;

    for(i = 0; i < lefttab; ++i)
    {
        left += m_tabdet[i].m_tabwidth;
    }

    for(i = lefttab; i <= righttab; ++i)
    {
        recttab.left = m_isfixedlength == wyTrue ? left + 1 : left;
        left += m_tabdet[i].m_tabwidth;
        recttab.right = left - (m_isfixedlength == wyTrue ? 1 : 0);
        recttab.bottom = m_size.cy + EXTRAHEIGHT - 1;
        recttab.top = m_isfixedlength == wyTrue ? (i == m_selectedtab ? 1 : 3) : 0;

        if(PtInRect(&recttab, *pnt) && PtInRect(&cliprect, *pnt))
        {
            if(m_extflagclose == wyTrue && pisoverclosebutton)
            {
                rectclose.left = left - (CLOSERECTWIDTH + CLOSERECTPADDING);
                rectclose.right = rectclose.left + CLOSERECTWIDTH;
                rectclose.top = ((recttab.bottom / 2) - (CLOSERECTWIDTH / 2)) + (m_isfixedlength == wyTrue ? TABTOPSPACE : 0);
                rectclose.bottom = rectclose.top + CLOSERECTWIDTH;

                if(PtInRect(&rectclose, *pnt) && m_tabs > m_mintabcount)
                {
                    *pisoverclosebutton = wyTrue;
                }
            }

            return i;
        }
    }

    return -1;
}

wyInt32 
CCustTab::OverTabControls(POINT* pt)
{
    RECT rect = {0, 0, SCROLLBUTTONWIDTH, m_size.cy + EXTRAHEIGHT - 1}, temprect;
    wyInt32 i, left = 0, width = 20;

    if(m_isscroll)
    {
        rect.left = 0;
        rect.right = SCROLLBUTTONWIDTH - SCROLLBUTTONPADDING;

        if(PtInRect(&rect, *pt))
        {
            return IDC_LEFTSCROLL;
        }

        GetClientRect(m_hwnd, &rect);
        rect.left = rect.right - RIGHTPADDING(m_isplussign) + 1 + SCROLLBUTTONPADDING;
        rect.bottom = m_size.cy + EXTRAHEIGHT - 1;

        if(PtInRect(&rect, *pt))
        {
            if(m_isplussign)
            {
                rect.left += SCROLLBUTTONWIDTH;

                if(PtInRect(&rect, *pt))
                {
					temprect.left = rect.left + ((rect.right - rect.left) / 2);// -(width / 2) + 1;
                    temprect.right = temprect.left + width;
					temprect.top = rect.top + ((rect.bottom - rect.top) / 2) -(width / 2);
                    temprect.top += (m_isfixedlength == wyTrue) ? 2 : 1;
                    temprect.bottom = temprect.top + width;

					if (PtInRect(&temprect, *pt))
					{
						return IDC_DROPDOWN;
					}

					temprect.left = rect.left;// +((rect.right - rect.left) / 2);// -(width / 2) + 1;
					temprect.right = temprect.left + width;
					temprect.top = rect.top + ((rect.bottom - rect.top) / 2) -(width / 2);
					temprect.top += (m_isfixedlength == wyTrue) ? 2 : 1;
					temprect.bottom = temprect.top + width;

                  /*  temprect.left -= 5;
                    temprect.top -= 4;
                    temprect.bottom += 4;
                    temprect.right -= 1;
*/
                    if(!PtInRect(&temprect, *pt))
                    {
                        return -1;
                    }

                    return IDC_PLUSSIGN;
                }
            }

            return IDC_RIGHTSCROLL;
        }
    }
    else
    {
        if(m_isplussign)
        {
            left -= m_scrollpos;

            for(i = 0; i < m_tabs; ++i)
            {
                left += m_tabdet[i].m_tabwidth;
            }
            
            rect.left = m_isfixedlength == wyTrue ? left : left + 1;
            rect.right = rect.left + (RIGHTPADDING(m_isplussign) - SCROLLBUTTONWIDTH);

            if(PtInRect(&rect, *pt))
            {
				temprect.left = rect.left + ((rect.right - rect.left) / 2) -(width / 2) - 2;
                temprect.right = temprect.left + width;
                temprect.top = rect.top + ((rect.bottom - rect.top) / 2) - (width / 2);
                temprect.top += (m_isfixedlength == wyTrue) ? 2 : 1;
                temprect.bottom = temprect.top + width;

				temprect.left -= 10;
				temprect.top += 4;
				temprect.bottom += 4;
				temprect.right -= 10;

                if(!PtInRect(&temprect, *pt))
                {
                    return -1;
                }

                return IDC_PLUSSIGN;
            }
        }
    }

    return -1;
}

LRESULT
CCustTab::OnMouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    POINT pnt;
    wyInt32 i, j;
    wyBool isonclosebutton;
    wyBool ispaint = wyFalse;

    GetCursorPos(&pnt);
    ScreenToClient(m_hwnd, &pnt);

    if(pnt.x != m_mouseprevpt.x || pnt.y != m_mouseprevpt.y)
    {
        m_mouseprevpt = pnt;
        SendMessage(m_hwndtooltip, TTM_POP, 0, 0);

        if(m_isdragmagepresent == wyTrue)
        {
            m_hwndprevfocus = GetFocus();
            SetFocus(m_hwnd);
        }
    }

    if(m_isdragging == wyTrue)
    {
        m_lastmouseovertab = -1;
        
        if(OnDragMove(wParam, &pnt) == wyTrue)
        {
            j = GetDragArrowTab(&pnt);

            if(j + 1 == m_selectedtab || j == m_selectedtab)
            {
                j = -2;
            }

            if(j != m_dragarrowtab)
            {
                m_dragarrowtab = j;
                ImageList_DragShowNolock(FALSE);
                PaintTab(wyTrue);
                ImageList_DragShowNolock(TRUE);
            }
        }
        else if(m_dragarrowtab != -2)
        {
            m_dragarrowtab = -2;
            ImageList_DragShowNolock(FALSE);
            PaintTab(wyTrue);
            ImageList_DragShowNolock(TRUE);
        }

        i = OverTabControls(&pnt);

        if(m_isscroll && (i == IDC_LEFTSCROLL || i == IDC_RIGHTSCROLL))
        {
            ImageList_DragShowNolock(FALSE);
            SetTimer(m_hwnd, DRAGSCROLLTIMER, 20, NULL);
        }
        else
        {
            ImageList_DragShowNolock(TRUE);
            KillTimer(m_hwnd, DRAGSCROLLTIMER);
        }

        return 1;
    }

    if((i = OverTabControls(&pnt)) != m_overtabcontrol)
        {
            m_lastmouseovertab = -1;
            m_ismouseoverclose = wyFalse;
        m_overtabcontrol = i;
        ispaint = wyTrue;
    }
    else if((i = OverTabs(&pnt, &isonclosebutton)) == m_lastmouseovertab)
    {
        if(m_ismouseoverclose != isonclosebutton)
        {
            m_ismouseoverclose = isonclosebutton;
            ispaint = wyTrue;
        }
    }
    else if(m_closebuttondowntab == -1 || i == m_selectedtab)
    {
        m_ismouseoverclose = isonclosebutton;
        m_lastmouseovertab = i;
        ispaint = wyTrue;
    }

    if(ispaint == wyTrue)
        {
            PaintTab(wyTrue);
        }

    return TRUE;
}

wyInt32 
CCustTab::GetDragArrowTab(PPOINT pnt)
{
    wyInt32 left = 0, temp = 0, i;

    if(m_isscroll == wyTrue)
    {
        temp = left = SCROLLBUTTONWIDTH;
    }

    if(pnt->x <= left)
    {
        return m_leftmostvisibletab - 1;
    }

    left -= m_scrollpos;

    for(i = 0; i < m_tabs; ++i)
    {
        if(pnt->x >= left && pnt->x <= left + m_tabdet[i].m_tabwidth)
        {
            if(left < temp)
            {
                return i;
            }

            temp = pnt->x - left;
            
            if(temp > m_tabdet[i].m_tabwidth / 2)
            {
                return i;
            }

            return i - 1;
        }

        left += m_tabdet[i].m_tabwidth;
    }

    return m_rightmostvisibletab;
}


wyBool
CCustTab::OnDragMove(WPARAM wparam, PPOINT pnt)
{
    RECT  rect, rectdragwin, rectwin;
    wyInt32 toppadding, leftpadding;

    GetWindowRect(m_hwnddragwin, &rectdragwin);
    GetWindowRect(m_hwnd, &rectwin);
    toppadding = rectwin.top - rectdragwin.top;
    leftpadding = rectwin.left - rectdragwin.left;
    
    GetClientRect(m_hwnd, &rect);
    rect.bottom = m_size.cy + RECTHEIGHT;

    if(m_isdragmagepresent == wyFalse)
    {
        ImageList_BeginDrag(m_himl , 0, pnt->x - m_draghotspot.x, pnt->y - m_draghotspot.y);
        ImageList_DragEnter(m_hwnddragwin, leftpadding, toppadding);
        m_isdragmagepresent = wyTrue;
    }

    ImageList_DragMove(pnt->x + leftpadding, pnt->y + toppadding);

    if(PtInRect(&rect, *pnt))
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return wyTrue;
    }
    else
    {
        SetCursor(LoadCursor(NULL, IDC_NO));
    }

    return wyFalse;
}


/*Function is used to handle the events when the left button us pressed*/
LRESULT	
CCustTab::OnLButtonDblClick(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	POINT		pnt;
	wyString	ctcitext;
    			
	pnt.x = GET_X_LPARAM(lParam);
	pnt.y = GET_Y_LPARAM(lParam);

    if(OverTabs(&pnt, NULL) == -1 && OverTabControls(&pnt) == -1 && pnt.y <= m_size.cy + EXTRAHEIGHT)
    {
        SendTabMessage(CTCN_LBUTTONDBLCLK);
    }
	else
	{
		//OverTabs(&pnt, &isoverclose);
		if(m_id == IDC_CTAB && OverTabControls(&pnt) == -1 && pnt.y <= m_size.cy + EXTRAHEIGHT)
		{
			SendTabMessage(CTCN_TABRENAME);
		}
	}
	

   	return TRUE;
}

 //Here Throwing the WM_MOUSELEAVE Message 
void
CCustTab::GetMouseMovement()
{
    TRACKMOUSEEVENT	 te;
  
	te.cbSize    = sizeof(TRACKMOUSEEVENT);
	te.dwFlags   = TME_LEAVE;
	te.hwndTrack = m_hwnd;

    TrackMouseEvent(&te);
}

void
CCustTab::OnMouseLeave()
{
    if(m_lastmouseovertab != -1 || m_overtabcontrol != -1)
    {
        m_lastmouseovertab = -1;
        m_overtabcontrol = -1;
        PaintTab(wyTrue);
    }

    m_ismouseoverclose = wyFalse;
    KillTimer(m_hwnd, DRAGSCROLLTIMER);
}

wyInt32 
CCustTab::SendTabMessage(wyInt32 message, LPNMCTC lpnmctc, WPARAM wparam)
{
    LPNMCTC ptempctc = NULL;
    NMCTC   nmctc = {0};
    wyInt32 ret;

    ptempctc = lpnmctc ? lpnmctc : &nmctc;

	ptempctc->hdr.hwndFrom  = m_hwnd;
	ptempctc->hdr.idFrom    = m_id;
	ptempctc->hdr.code      = message;

    if(lpnmctc == NULL)
    {
        GetCursorPos(&ptempctc->curpos);
    }
				
	ret = SendMessage(GetParent(m_hwnd), WM_NOTIFY, wparam, (LPARAM)ptempctc);
    return ret;
}

void
CCustTab::SetGradient(HDC hdcmem, TRIVERTEX *vertex)
{
	// Create a GRADIENT_RECT structure that
	// references the TRIVERTEX vertices.
	GRADIENT_RECT gRect;
	
	gRect.UpperLeft  = 0;
	gRect.LowerRight = 1;

	GradientFill(hdcmem, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
}

/*Function is used to remove a particular tab*/
wyBool  
CCustTab::DeleteItem(wyInt32 tabindex, wyBool isexplicit, wyBool ispositionrequired)
{
	LPCTCITEMEX newa = NULL;
    wyInt32     ret, selindex;
    NMCTC       nmctc = {0};
	ListofOpenTabs * listofopentabs;
	wyInt64 tabcount=0,i;
	wyBool deletedfromstruct = wyFalse, found = wyFalse, delfound = wyFalse,nodefound =wyFalse;
	MDIListForDropDrown *pfound = NULL, *p;
	ListofOpenTabs * temp;
	MDIWindow *wnd;
	wnd = GetActiveWin();

    if(tabindex >= m_tabs || tabindex < 0)
    {
        return wyFalse;
    }

    nmctc.lparam = isexplicit;
    nmctc.count = tabindex;				
	ret = SendTabMessage(CTCN_TABCLOSING, &nmctc);
    selindex = m_selectedtab;
	
    if(ret == 0)
    {
		return wyFalse;
    }

	if(m_tabs > 1)
	{
		/* first allocate memory for m_tab - 1 */
		newa = (LPCTCITEMEX)calloc(sizeof(CTCITEMEX), m_tabs - 1);

		if(tabindex == 0)
			memcpy(newa, m_tabdet + 1, sizeof(CTCITEMEX) * (m_tabs - 1));
		else  
		{
			memcpy(newa, m_tabdet, sizeof(CTCITEMEX) * tabindex);
			memcpy(newa + tabindex, m_tabdet + tabindex + 1,  sizeof(CTCITEMEX) * (m_tabs -(tabindex+1)));
		}
	}
	
	if(m_tabs > 0)
	{
		FreeItem(&m_tabdet[tabindex], tabindex);
		free(m_tabdet);
	}
	
	m_tabdet = newa;
	m_tabs--; // reduce the tab count

	if(m_selectedtab >= m_tabs)
		m_selectedtab--;

	if(m_selectedtab < m_starttab)
	{
		if(m_selectedtab > -1)
			m_starttab = m_selectedtab;
	}

	//if left scroll is enabled, then enable auto scroll
	if(m_isfixedlength == wyTrue)
	{
		if(m_starttab > 0)
		{
			m_starttab = m_starttab - 1;
		}
	}
	
    nmctc.retvalue = 0;
    nmctc.count = tabindex >= m_tabs ? -1 : tabindex;
	SendTabMessage(CTCN_TABCLOSED, &nmctc);

    if(selindex == tabindex && nmctc.retvalue && m_selectedtab > -1 && m_selectedtab < m_tabs)
    {
        nmctc.count = -1;
        m_istabchanging = wyTrue;
        SendTabMessage(CTCN_SELCHANGE);
        m_istabchanging = wyFalse;
    }

	//Added code to Remove the tab from drop down structure 
	if (!ispositionrequired) { //
		//getting the head of the list containing all connection details
		p = (MDIListForDropDrown *)pGlobals->m_mdilistfordropdown->GetFirst();
		if (!p)
		{
			return wyTrue;
		}
		if (!wnd)
		{
			return wyTrue;
		}
		// To search for the particular tab
		while (p)
		{
			if (wnd == p->mdi)
			{
				found = wyTrue;
				pfound = p;
				nodefound = wyTrue;
				break;
			}
			p = (MDIListForDropDrown *)p->m_next;
		}

		// getting the tab which is being closed
		if (found) {
			listofopentabs = (ListofOpenTabs *)p->opentab->GetFirst();
			tabcount = p->opentab->GetCount();

			for (i = 0; i < tabcount; i++)
			{
				if (i == tabindex && !deletedfromstruct) 
				{
					temp = listofopentabs;
					deletedfromstruct = wyTrue;
					delfound = wyTrue;
					break;
				}
				listofopentabs = (ListofOpenTabs *)listofopentabs->m_next;
			}
			if (delfound)
			{
				p->opentab->Remove(temp);
				delfound = wyFalse;
			}
		}
	}

	return wyTrue;
}

/*Function is used to Free an item*/
wyBool  
CCustTab::FreeItem(LPCTCITEMEX item, wyInt32 tabindex)
{
	if(item->ctcitem.m_psztext)
	{
		free(item->ctcitem.m_psztext);
		item->ctcitem.m_psztext = NULL;
        item->ctcitem.m_cchtextmax = 0;
	}

	if(item->ctcitem.m_tooltiptext)
	{
		free(item->ctcitem.m_tooltiptext);
		item->ctcitem.m_tooltiptext = NULL;
	}

    if(tabindex >= 0)
    {
        KillTimer(m_hwnd, ICONANIMATETIMER_BASE + tabindex);
	}
		
	return wyTrue;
}

/*Function is used to remove all items*/
wyBool  
CCustTab::RemoveAllItem()
{
	wyInt32 tabcount;

	for(tabcount = 0; tabcount < m_tabs; tabcount++)
		DeleteItem(tabcount);

	m_tabs = 0;
	free(m_tabdet);
	m_tabdet = NULL;
	PaintTab();
	return wyTrue;
}

LPARAM
CCustTab::GetLongData()
{
	return m_lparamdata;	
}

LPARAM
CCustTab::SetItemLongValue(wyInt32 tabcount, LPARAM lParam)
{
	LPCTCITEM	ptvcol;	
	LPARAM		prev;

	ptvcol	=	GetSubItemStruct(tabcount);
	
	if(!ptvcol)
		return -1;

	prev = ptvcol->m_lparam;
	ptvcol->m_lparam = lParam;

	return prev;
}

LPCTCITEM  
CCustTab::GetSubItemStruct(wyInt32 tabcount)
{
	if(tabcount < m_tabs)
	{
        if(m_tabdet[tabcount].ctcitem.m_mask & CTBIF_LPARAM)
            return(&m_tabdet[tabcount].ctcitem);
	}

	return NULL;
}


/*Function is used to get the number of tabs*/
wyInt32 
CCustTab::GetItemCount()
{
	return m_tabs;
}

/*Function is used to get the index of the currently focused tab*/
wyInt32  
CCustTab::GetCurFocus()
{
	if(m_selectedtab >= 0 && m_selectedtab < m_tabs)
		return m_selectedtab; // returns the index of the selected tab
	else
		return -1; // No tab is selected
}

/*Function is used to get visible tab count*/
wyInt32 
CCustTab::GetVisibleTabCount()
{
    return(m_rightmostvisibletab - m_leftmostvisibletab + 1);
}



const wyChar*  
CCustTab::GetItemTitle(wyInt32 tabindex, wyString* title)
{
	title->SetAs(m_tabdet[tabindex].ctcitem.m_psztext);
	return title->GetString();
}

const wyChar*  
CCustTab::GetItemTooltip(wyInt32 tabindex, wyString* Tooltip)
{
	Tooltip->SetAs(m_tabdet[tabindex].ctcitem.m_tooltiptext);
	return Tooltip->GetString();
}

wyBool  
CCustTab::GetItem(wyInt32 index, LPCTCITEM pitem)
{
	if((index <= m_tabs) && (index > -1))
	{
        if(pitem->m_mask & CTBIF_TEXT)
        {
		    strncpy(pitem->m_psztext, m_tabdet[index].ctcitem.m_psztext, m_tabdet[index].ctcitem.m_cchtextmax);
            pitem->m_psztext[m_tabdet[index].ctcitem.m_cchtextmax] = 0;
        }

        if(pitem->m_mask & CTBIF_IMAGE)
		    pitem->m_iimage = m_tabdet[index].ctcitem.m_iimage;

        if(pitem->m_mask & CTBIF_LPARAM)
		    pitem->m_lparam = m_tabdet[index].ctcitem.m_lparam;

		if(pitem->m_mask & CTBIF_COLOR)
		{
			pitem->m_color = m_tabdet[index].ctcitem.m_color;
			pitem->m_fgcolor = m_tabdet[index].ctcitem.m_fgcolor;
		}

		if(pitem->m_mask & CTBIF_TOOLTIP)
		    strcpy(pitem->m_tooltiptext, m_tabdet[index].ctcitem.m_tooltiptext);
		
		
		return wyTrue;
	}
	else
	{
		pitem = NULL;
		return wyFalse;
	}
}

/*Function is used to Change a particular item*/
wyBool 
CCustTab::SetItem(wyInt32 index, LPCTCITEM pitem)
{
    HDC hdc;

	if((index < m_tabs) && (index > -1))
	{
        if(pitem->m_mask & CTBIF_TEXT)
        {
            /* If already memory is allocated check whether it is enough or not*/
            if(m_tabdet[index].ctcitem.m_mask & CTBIF_TEXT)
            {
		        if(m_tabdet[index].ctcitem.m_cchtextmax <= pitem->m_cchtextmax)
			        m_tabdet[index].ctcitem.m_psztext =(wyChar *)realloc(m_tabdet[index].ctcitem.m_psztext, (pitem->m_cchtextmax + 1) * sizeof(wyChar));
            }
            else
			     m_tabdet[index].ctcitem.m_psztext =(wyChar *)calloc((pitem->m_cchtextmax + 1), sizeof(wyChar));

            strncpy(m_tabdet[index].ctcitem.m_psztext, pitem->m_psztext, pitem->m_cchtextmax);
            m_tabdet[index].ctcitem.m_psztext[pitem->m_cchtextmax] = 0;
            m_tabdet[index].ctcitem.m_cchtextmax = pitem->m_cchtextmax;
        }
        else
        {
            m_tabdet[index].ctcitem.m_psztext = NULL;
            m_tabdet[index].ctcitem.m_cchtextmax = 0;
        }

		if(pitem->m_mask & CTBIF_TOOLTIP)
        {
            /* If already memory is allocated check whether it is enough or not*/
            if(m_tabdet[index].ctcitem.m_mask & CTBIF_TOOLTIP)
            {
		        if(strlen(m_tabdet[index].ctcitem.m_tooltiptext) <= strlen(pitem->m_tooltiptext))
			        m_tabdet[index].ctcitem.m_tooltiptext =(wyChar *)realloc(m_tabdet[index].ctcitem.m_tooltiptext, (strlen(pitem->m_tooltiptext) + 1) * sizeof(wyChar));
            }
            else
			     m_tabdet[index].ctcitem.m_tooltiptext =(wyChar *)calloc(strlen(pitem->m_tooltiptext) + 1, sizeof(wyChar));

            strcpy(m_tabdet[index].ctcitem.m_tooltiptext, pitem->m_tooltiptext);
        }
        else
        {
            m_tabdet[index].ctcitem.m_tooltiptext = NULL;
        }

        if(pitem->m_mask & CTBIF_IMAGE)
    		m_tabdet[index].ctcitem.m_iimage = pitem->m_iimage;

		if(pitem->m_mask & CTBIF_LPARAM)
		    m_tabdet[index].ctcitem.m_lparam = pitem->m_lparam;

        m_tabdet[index].ctcitem.m_mask |= pitem->m_mask;
		
		
        hdc = GetDC(m_hwnd);
        m_tabdet[index].m_tabwidth = CalculateTabLength(hdc, index);
        ReleaseDC(m_hwnd, hdc);
		
		return wyTrue;
	}
	else
		return wyFalse;
}

wyBool 
CCustTab::SetItemColor(wyInt32 index, LPCTCITEM pitem)
{
	if((index <= m_tabs) && (index > -1))
	{
		if(pitem->m_mask & CTBIF_COLOR)
		{
			m_tabdet[index].ctcitem.m_color = pitem->m_color;
			m_tabdet[index].ctcitem.m_fgcolor = pitem->m_fgcolor;
		}
	    CreateBottomLinePen();
		return wyTrue;
	}
	else
	{
		return wyFalse;
	}
}


/*Function is used to Set the minimum tab width*/
wyInt32	
CCustTab::SetMinTabWidth(wyInt32 cx)
{
	wyInt32		ret;

	ret = m_minimumtabwidth;
	m_minimumtabwidth = cx;
    PaintTab(wyTrue);
	return ret;
}

wyBool	
CCustTab::IsMenuEnabled(wyInt32 index)
{
    if((index <= m_tabs) && (index > -1))
	{
        if(m_tabdet[index].ctcitem.m_mask & CTBIF_CMENU)
        {
            return wyTrue;
        }
    }

    return wyFalse;
}

wyBool	
CCustTab::EnsureVisible(wyInt32 index, wyBool isanimate)
{
	wyInt32     i, temp, left;
    RECT        rect;
    
    if(m_leftmostvisibletab == -1 || m_rightmostvisibletab == -1)
    {
        return wyTrue;
    }

    if(index < 0 || index >= m_tabs)
    {
        return wyTrue;
    }

    if(index >= m_leftmostvisibletab && index <= m_rightmostvisibletab)
    {
        PaintTab(wyTrue);
        return wyTrue;
    }

    GetClientRect(m_hwnd, &rect);
    left = SCROLLBUTTONWIDTH;
    left -= m_scrollpos;

    for(i = 0; i <= index; ++i)
    {
        left += m_tabdet[i].m_tabwidth;
    }

    if(index > m_rightmostvisibletab)
    {
        temp = left - m_tabdet[index].m_tabwidth;

        for(; index - 1 >= 0 && m_scrollpos <= m_maxscrollpos; m_scrollpos += SCROLLTHRESHOLD)
        {
            temp -= SCROLLTHRESHOLD;

            if(temp < rect.right - RIGHTPADDING(m_isplussign))
            {
                m_rightmostvisibletab = index - 1;
                break;
            }
        }

        m_scrollpos = min(m_scrollpos, m_maxscrollpos);

        while(index != m_rightmostvisibletab && m_rightmostvisibletab != -1)
        {
            if(index - m_rightmostvisibletab == 1)
            {
                if(isanimate == wyTrue)
                {
                    Sleep(5);
                }
            }

            OnHScroll((WPARAM)IDC_RIGHTSCROLL, isanimate == wyTrue ? 0 : 1);
        }
    }    
    else if(index < m_leftmostvisibletab)
    {
        temp = left;

        for(; index + 1 < m_tabs && m_scrollpos >= 0; m_scrollpos -= SCROLLTHRESHOLD)
        {
            temp += SCROLLTHRESHOLD;
            
            if(temp > 0)
            {
                m_leftmostvisibletab = index + 1;
                break;
            }
        }

        m_scrollpos = max(m_scrollpos, 0);

        while(index != m_leftmostvisibletab && m_leftmostvisibletab != -1)
        {
            if(m_leftmostvisibletab - index == 1)
            {
                if(isanimate == wyTrue)
                {
                    Sleep(5);
                }
            }

            OnHScroll((WPARAM)IDC_LEFTSCROLL, isanimate == wyTrue ? 0 : 1);
        }
    }

    PaintTab(wyTrue);
    return wyTrue;
}

LRESULT
CCustTab::OnCreate(LPARAM lParam)
{
    lpTBWndProc = (CTBWNDPROC)((LPCREATESTRUCT)lParam)->lpCreateParams;
    return 0;
}

LRESULT 
CCustTab::OnRButtonUp(HWND hwnd, LPARAM lParam)
{
	wyInt32     i;
	POINT       pnt;
    wyBool      ismenu, isoverclosebutton;


    pnt.x = (LONG)GET_X_LPARAM(lParam);
	pnt.y = (LONG)GET_Y_LPARAM(lParam);
	
    if((ismenu = IsMenuEnabled(m_selectedtab)) == wyFalse)
    {
        m_rbuttondowntab = -1;
        return 1;
    }

    i = OverTabs(&pnt, &isoverclosebutton);

    if(i != -1 && i == m_rbuttondowntab && isoverclosebutton == wyFalse)
    {
        SendTabMessage(CTCN_ONCONTEXTMENU);
    }

	m_rbuttondowntab = -1;	
	return 1;
}

LRESULT 
CCustTab::OnRButtonDown(HWND hwnd, LPARAM lParam)
{
	wyInt32     i;
	POINT       pnt;
    wyBool      isoverclosebutton;

    if(m_isdragging == wyTrue)
    {
        FreeDragResources();
    }

    pnt.x = (LONG)GET_X_LPARAM(lParam);
	pnt.y = (LONG)GET_Y_LPARAM(lParam);
	
    i = OverTabs(&pnt, &isoverclosebutton);
    m_rbuttondowntab = -1;	

    if(i != -1)
    {
        if(m_selectedtab != i)
        {
            if(SetCurSel(i, 1) == wyFalse)
            {
                return 1;
            }

            PaintTab();
        }

        if(isoverclosebutton == wyFalse)
        {
            m_rbuttondowntab = i;
        }

        EnsureVisible(m_selectedtab);
    }

	return 1;
}


LRESULT 
CCustTab::OnMButtonDown(HWND hwnd, LPARAM lParam)
{
    wyInt32     i;
	POINT       pnt;

    if(m_isdragging == wyTrue)
    {
        FreeDragResources();
    }

    pnt.x = (LONG)GET_X_LPARAM(lParam);
	pnt.y = (LONG)GET_Y_LPARAM(lParam);
	
    i = OverTabs(&pnt, NULL);
    m_mbuttondowntab = -1;	

    if(i != -1)
    {
        if(m_selectedtab != i)
        {
            if(SetCurSel(i, 1) == wyFalse)
            {
                return 1;
            }

            PaintTab();
        }

        EnsureVisible(m_selectedtab);
        m_mbuttondowntab = i;
    }

	return 1;
}

LRESULT 
CCustTab::OnMButtonUp(HWND hwnd, LPARAM lParam)
{
    wyInt32     i;
	POINT       pnt;

    pnt.x = (LONG)GET_X_LPARAM(lParam);
	pnt.y = (LONG)GET_Y_LPARAM(lParam);
    
    i = OverTabs(&pnt, NULL);

    if(m_isdragging == wyTrue)
    {
        OnDragEnd();
    }
    else if(i != -1 && i == m_mbuttondowntab)
    {
        if(m_extflagclose == wyTrue && m_tabs > m_mintabcount)
        {
            if(DeleteItem(i, wyTrue) == wyTrue)
            {
                OnWMSize();
                PaintTab();
            }
        }
    }

    m_mbuttondowntab = -1;
    return 1;
}


void        
CCustTab::SetId(wyInt32 id)
{
    m_id = id;
}

void 
CCustTab::DrawTab(HDC hdc, wyInt32 tabindex, RECT* pdrawrect, RECT* porigrect, POINT* pt, RECT* pselectedrect)
{
    TRIVERTEX vertex[2] = {0};
    RECT temprect = {0};
    wyBool ismouseovertab = wyFalse;

    if(m_lastmouseovertab != -1 && PtInRect(porigrect, *pt) && m_isdragging == wyFalse && m_tabcontroldown == -1)
    {
        ismouseovertab = wyTrue;
    }

    if(m_isfixedlength == wyTrue)
    {
        DrawFixedLengthTab(hdc, pdrawrect, tabindex, ismouseovertab);
        vertex[0].x     = pdrawrect->left + ((tabindex == m_selectedtab) ? 2 : 3);
        vertex[0].y     = pdrawrect->top + TABTOPSPACE + 5;
    }
    else
    {
        DrawVaryingLengthTab(hdc, pdrawrect, tabindex, ismouseovertab);
        vertex[0].x     = pdrawrect->left + 1;
        vertex[0].y     = pdrawrect->top + TABTOPSPACE;
    }

    temprect.left = pdrawrect->left;
    temprect.right = pdrawrect->left + m_tabdet[tabindex].m_tabwidth - 1;
    temprect.top = pdrawrect->top;
    temprect.bottom = pdrawrect->bottom - 2;

	//Sets the Gradient to the selected tab
	if(tabindex == m_selectedtab)
	{	
        SetGradientArrayOnFocus(&m_tabdet[tabindex].ctcitem, &temprect, vertex);
		SetGradient(hdc, vertex);

        if(pselectedrect)
        {
            *pselectedrect = *pdrawrect;
            pselectedrect->right = pdrawrect->left + m_tabdet[tabindex].m_tabwidth;
        }
	}
    else
    {
        if(ismouseovertab == wyTrue)
        {
            SetGradientArrayOnMouseOver(&m_tabdet[tabindex].ctcitem, &temprect, vertex);
        }
        else
        {
            SetGradientArrayOnUnFocus(&m_tabdet[tabindex].ctcitem, &temprect, vertex);
        }

       // SetGradient(hdc, vertex);
    }
}

void 
CCustTab::DrawTabs(HDC hdc)
{
    RECT rect, drawrect, rectwin, temprect, selectedrect = {0};
    wyInt32 i, left = 0, right, rightmostvisibleright = 0;
    DWORD barcolor;
    TRIVERTEX	vertex[2];
    HPEN hpen;
    HFONT hfont = NULL;
    HICON hicon;
    wyString temp;
    POINT pt;
    wyBool isdragmarkdrawn = wyFalse, flag = wyFalse;
    COLORREF prevcolor, color, fgcolor;
    HRGN hrgn = NULL;
    

    GetClientRect(m_hwnd, &rect);
    rect.bottom = m_size.cy + EXTRAHEIGHT - 1;
    rectwin = drawrect = rect;
    right = rect.right;

    if(m_isscroll)
    {
        left = drawrect.left = SCROLLBUTTONWIDTH;
        right -= RIGHTPADDING(m_isplussign);
        hrgn = CreateRectRgn(rect.left + SCROLLBUTTONWIDTH, rect.top, rect.right - RIGHTPADDING(m_isplussign) + 1, rect.bottom);
    }

    drawrect.left -= m_scrollpos;
    barcolor = m_colorinfo.m_tabbg1;

    m_blue = (barcolor & 0xff0000) / 0x100;
    m_green = (barcolor & 0x00ff00);
    m_red = (barcolor & 0x0000ff) * 0x100;
		
	// Fill the top bar for the tab controll 
	//rectwin.left += 1;
	//rectwin.right -= 2;

	MoveToEx(hdc,  rectwin.left + 1, rectwin.bottom - 1, NULL);
	LineTo(hdc, rectwin.right + 2 , rectwin.bottom - 1);
	rectwin.bottom = m_size.cy + EXTRAHEIGHT;
	m_tabheight = rectwin.bottom + 2;
		
	SetBkMode(hdc, TRANSPARENT);
	SetGradientToTabRibbon(hdc, &rectwin, vertex); 

    GetCursorPos(&pt);
    MapWindowPoints(NULL, m_hwnd, &pt, 1);

    if(hrgn)
    {
        SelectClipRgn(hdc, hrgn);
    }

    for(i = 0; i < m_tabs; drawrect.left += m_tabdet[i++].m_tabwidth)
    {
        if(drawrect.left + m_tabdet[i].m_tabwidth < 0)
        {
            memset(&m_tabdet[i].m_tabarea, -1, sizeof(m_tabdet[i].m_tabarea));
            continue;
        }

        if(drawrect.left > rect.right)
        {
            break;
        }

        if(flag == wyFalse && drawrect.left >= left)
        {
            flag = wyTrue;
            rightmostvisibleright = drawrect.left + m_tabdet[i].m_tabwidth;
        }

        m_tabdet[i].m_tabarea = drawrect;
        m_tabdet[i].m_tabarea.right = m_tabdet[i].m_tabarea.left + m_tabdet[i].m_tabwidth;
        DrawTab(hdc, i, &drawrect, &m_tabdet[i].m_tabarea, &pt, &selectedrect);
        memset(&temprect, 0, sizeof(temprect));
        temprect.left = drawrect.left + CLOSERECTPADDING;

        if(m_tabdet && m_tabdet[i].ctcitem.m_iimage != -1 &&(m_tabdet[i].ctcitem.m_mask & CTBIF_IMAGE))
		{	
            hicon = NULL;

            if(m_hiconlist && m_tabdet[i].m_iconindex >= 0 && m_tabdet[i].m_iconindex < ImageList_GetImageCount(m_hiconlist))
            {
                hicon = ImageList_ExtractIcon(0, m_hiconlist, m_tabdet[i].m_iconindex);
            }
            
            if(!hicon)
            {
                hicon =(HICON)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(m_tabdet[i].ctcitem.m_iimage), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
            }

            DrawIconEx(hdc, temprect.left, ((m_size.cy + EXTRAHEIGHT) / 2) - 8, hicon, 16, 16, 0, NULL, DI_NORMAL);
	    	DestroyIcon(hicon);
            temprect.left += IMAGEWIDTH;
        }

        temprect.right = (drawrect.left + m_tabdet[i].m_tabwidth) - CLOSERECTPADDING - (m_extflagclose == wyTrue ? CLOSERECTPADDING + CLOSERECTWIDTH : 0);
        temprect.bottom = drawrect.bottom;
        temprect.top = drawrect.top + (m_isfixedlength == wyTrue ? 2 : (-2));
        temp.SetAs(m_tabdet[i].ctcitem.m_psztext);

        if(i == m_selectedtab)
        {
            hfont = (HFONT)SelectObject(hdc, m_hselectedfont);
        }

        color =  m_isfixedlength == wyTrue ? m_tabdet[i].ctcitem.m_color : COLOR_WHITE;
        fgcolor = m_isfixedlength == wyTrue ? m_tabdet[i].ctcitem.m_fgcolor : ((i == m_selectedtab) ? m_colorinfo.m_seltabtext : m_colorinfo.m_tabtext);
        prevcolor = SetTextColor(hdc, fgcolor);
        DrawText(hdc, temp.GetAsWideChar(), -1, &temprect, DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE);
        SetTextColor(hdc, prevcolor);

        if(m_extflagclose && (i == m_selectedtab || i == m_lastmouseovertab) && m_tabs > m_mintabcount && 
            (m_tabcontroldown == -1 || i == m_selectedtab))
        {
            DrawCloseControls(hdc, &drawrect, i, &pt);
        }

        if(i == m_selectedtab)
        {
            SelectObject(hdc, hfont);
        }

        if(drawrect.left + m_tabdet[i].m_tabwidth <= right)
        {
            rightmostvisibleright = drawrect.left + m_tabdet[i].m_tabwidth;
        }

        if(isdragmarkdrawn == wyFalse && m_isdragging == wyTrue && m_isdragmagepresent == wyTrue && m_dragarrowtab == i - 1)
        {
            DrawDragArrow(hdc, drawrect.left, drawrect.top);
            isdragmarkdrawn = wyTrue;
        }
    }

    for(; i < m_tabs; ++i)
    {
        memset(&m_tabdet[i].m_tabarea, rect.right + 1, sizeof(m_tabdet[i].m_tabarea));
    }

    if(isdragmarkdrawn == wyFalse && m_isdragging == wyTrue && m_isdragmagepresent == wyTrue && m_dragarrowtab != -2)
    {
        DrawDragArrow(hdc, rightmostvisibleright, drawrect.top);
    }

    if(hrgn)
    {
        DeleteRgn(hrgn);
        SelectClipRgn(hdc, NULL);
    }

    hpen = (HPEN)SelectObject(hdc, m_hpenactivesep);
    left = 0 - m_scrollpos;
    MoveToEx(hdc, left, drawrect.bottom - 1, NULL);
    LineTo(hdc, selectedrect.left + 1, drawrect.bottom - 1);
    MoveToEx(hdc, selectedrect.right, drawrect.bottom - 1, NULL);
    LineTo(hdc, rect.right + 1, drawrect.bottom - 1);
    SelectObject(hdc, hpen);

    if(m_isscroll == wyTrue)
    {
        DrawScrollButton(hdc, IDC_LEFTSCROLL, 0, &pt);
        DrawScrollButton(hdc, IDC_RIGHTSCROLL, rect.right - RIGHTPADDING(m_isplussign) + 1 + SCROLLBUTTONPADDING, &pt);
		DrawScrollButton(hdc, IDC_DROPDOWN, rect.right - RIGHTPADDING(m_isplussign) + 1 + SCROLLBUTTONPADDING + 40, &pt);
        i = rect.right;
        hpen = (HPEN)SelectObject(hdc, m_hpenactivesep);
        //rect.bottom = m_size.cy + RECTHEIGHT + TABTOPSPACE;
        rect.top = 0;
        rect.left = rect.right - RIGHTPADDING(m_isplussign) + 1;
		MoveToEx(hdc, rect.left, drawrect.bottom - 1, NULL);
        LineTo(hdc, rect.right + 1, drawrect.bottom - 1);

        rect.right = SCROLLBUTTONWIDTH;
        rect.left = 0;
        MoveToEx(hdc, rect.left, drawrect.bottom - 1, NULL);
        LineTo(hdc, rect.right + 1, drawrect.bottom - 1);
        SelectObject(hdc, hpen);

        rect.right = i;
        rect.left = rect.right - (RIGHTPADDING(m_isplussign) - SCROLLBUTTONWIDTH);
    }
    else
    {
        rect.left = m_isfixedlength == wyTrue ? drawrect.left : drawrect.left + 1;
        rect.right = rect.left + (RIGHTPADDING(m_isplussign) - SCROLLBUTTONWIDTH);
    }

    if(m_isplussign == wyTrue)
    {
        DrawPlusButton(hdc, &rect, &pt);
    }

}

void 
CCustTab::DrawDragArrow(HDC hdc, wyInt32 left, wyInt32 top)
{
    HRGN    hrgnarrow;
    HBRUSH  hbr;
    POINT   arrowpoints[]= {{left - 5, top}, {left + 5, top}, {left, top + 12}};

    hbr = CreateSolidBrush(m_colorinfo.m_dragarrow);
    hrgnarrow = CreatePolygonRgn(arrowpoints, 3, WINDING);
    FillRgn(hdc, hrgnarrow, hbr);
    DeleteObject((HGDIOBJ)hrgnarrow);
    DeleteBrush(hbr);
}

void 
CCustTab::DrawScrollButton(HDC hdc, wyInt32 id, wyInt32 x,PPOINT pnt)
{
    RECT        rect, drawrect;
    TRIVERTEX   vertex[2];
    wyInt32     height, stretchmode, scale = 0;
    HRGN        hrgnarrow;
    HBRUSH      hbr;
    POINT       arrowpoints[3];

    rect.left = x;
    rect.right = x + (SCROLLBUTTONWIDTH - SCROLLBUTTONPADDING);
    rect.top = 1;//TABTOPSPACE;
    rect.bottom = m_size.cy + EXTRAHEIGHT - 1;

    drawrect.left = SCALEANTIALIASING(4);
    drawrect.right = SCALEANTIALIASING(rect.right - rect.left) - SCALEANTIALIASING(4);
    drawrect.top = SCALEANTIALIASING(7);
    drawrect.bottom = SCALEANTIALIASING(rect.bottom - rect.top) - SCALEANTIALIASING(7);

    vertex[0].x = SCALEANTIALIASING(1);
    vertex[0].y = SCALEANTIALIASING(1);
    vertex[1].x = SCALEANTIALIASING(rect.right - rect.left) - SCALEANTIALIASING(1);
    vertex[1].y = SCALEANTIALIASING(rect.bottom - rect.top) - SCALEANTIALIASING(1);

    stretchmode = SetStretchBltMode(m_hdcantialias, HALFTONE);
    StretchBlt(m_hdcantialias, 0, 0, 
        SCALEANTIALIASING((rect.right - rect.left)), 
        SCALEANTIALIASING((rect.bottom - rect.top)),
        hdc, 
        rect.left, rect.top, 
        rect.right - rect.left, 
        rect.bottom - rect.top, 
        SRCCOPY
        );
    SetStretchBltMode(m_hdcantialias, stretchmode);
    
    if(PtInRect(&rect, *pnt) && m_closebuttondowntab == -1 && (m_tabcontroldown == -1 || m_tabcontroldown == id))
    {
        SetGradientArrayOnMouseOver(NULL, NULL, vertex);
        SetGradient(m_hdcantialias, vertex);
        hbr = CreateSolidBrush(m_colorinfo.m_tabcontrols);
    }
    else
    {
        hbr = CreateSolidBrush(m_colorinfo.m_tabcontrols);
    }

    if(PtInRect(&rect, *pnt) && (m_tabcontroldown == id || m_isdragging == wyTrue))
    {
        scale = SCALEANTIALIASING(1);
    }
        
    height = ((drawrect.bottom - drawrect.top) / 2) + drawrect.top;
    arrowpoints[0].x = (id == IDC_RIGHTSCROLL) ? drawrect.left : drawrect.right;
    arrowpoints[0].y = height - SCALEANTIALIASING(6) + scale;
    arrowpoints[1].x = (id != IDC_RIGHTSCROLL) ? drawrect.left + scale : drawrect.right - scale;
    arrowpoints[1].y = height;
    arrowpoints[2].x = (id == IDC_RIGHTSCROLL) ? drawrect.left : drawrect.right;
    arrowpoints[2].y = height + SCALEANTIALIASING(6) - scale;

	if (id == IDC_DROPDOWN)
	{
		height = ((drawrect.bottom - drawrect.top) / 2) + drawrect.top;
		arrowpoints[0].x = drawrect.left - SCALEANTIALIASING(2);
		arrowpoints[0].y = height - SCALEANTIALIASING(6) + scale;
		arrowpoints[1].x = drawrect.right + SCALEANTIALIASING(2);
		arrowpoints[1].y = height - SCALEANTIALIASING(6) + scale;
		arrowpoints[2].x = (drawrect.left + drawrect.right) / 2;
		arrowpoints[2].y = height + SCALEANTIALIASING(2) - scale;
	}
    
    hrgnarrow = CreatePolygonRgn(arrowpoints, 3, WINDING);
    FillRgn(m_hdcantialias, hrgnarrow, hbr);
    DeleteObject((HGDIOBJ)hrgnarrow);
    DeleteBrush(hbr);

    stretchmode = SetStretchBltMode(hdc, HALFTONE);
    StretchBlt(hdc, rect.left, rect.top, 
        rect.right - rect.left, 
        rect.bottom - rect.top, 
        m_hdcantialias,
        0, 0, 
        SCALEANTIALIASING((rect.right - rect.left)), 
        SCALEANTIALIASING((rect.bottom - rect.top)), 
        SRCCOPY
        );
    SetStretchBltMode(hdc, stretchmode);
}

void 
CCustTab::DrawPlusButton(HDC hdc, PRECT prect, PPOINT pnt)
{
    wyInt32 width;
    HRGN hrgnplus;
    RECT rect = {0}, borderrect = {0};
    HBRUSH hbrush;
    HPEN hpen;
    TRIVERTEX vertex[2];

    width = 8;
    rect.left = prect->left + ((prect->right - prect->left) / 2) - (width / 2) - 10;
    rect.right = rect.left + width;
    
    rect.top = prect->top + ((prect->bottom - prect->top) / 2) - (width / 2);
    rect.top += (m_isfixedlength == wyTrue) ? 2 : 1;
    rect.bottom = rect.top + width;

    borderrect.left = rect.left - 4;
    borderrect.top = rect.top - 4;
    borderrect.bottom = rect.bottom + 4;
    borderrect.right = rect.right + 4;
    hpen = (HPEN)SelectObject(hdc, PtInRect(&borderrect, *pnt) ? m_hpenhighlight : m_hpenactivesep);
   // Rectangle(hdc, borderrect.left, borderrect.top, borderrect.right, borderrect.bottom);
    SelectObject(hdc, hpen);

    vertex[0].x = borderrect.left + 1;
    vertex[0].y = borderrect.top + 1;
    vertex[1].x = borderrect.right - 1;
    vertex[1].y = borderrect.bottom - 1;

    rect.left -= 1;
    rect.top -= 1;
    rect.bottom += 1;
    rect.right += 1;

    if(m_tabcontroldown == IDC_PLUSSIGN && m_overtabcontrol == IDC_PLUSSIGN && PtInRect(&borderrect, *pnt))
    {
        rect.top += 1;
        rect.bottom -= 1;
        rect.left += 1;
        rect.right -= 1;
    }

    POINT arrowpoints[]= {
        {rect.left, rect.top + ((rect.bottom - rect.top) / 2) - 1}, 
        {rect.left + ((rect.right - rect.left) / 2) - 1, rect.top + ((rect.bottom - rect.top) / 2) - 1},
        {rect.left + ((rect.right - rect.left) / 2) - 1, rect.top}, 
        {rect.left + ((rect.right - rect.left) / 2) + 1, rect.top}, 
        {rect.left + ((rect.right - rect.left) / 2) + 1, rect.top + ((rect.bottom - rect.top) / 2) - 1},
        {rect.right, rect.top + ((rect.bottom - rect.top) / 2) - 1},
        {rect.right, rect.top + ((rect.bottom - rect.top) / 2) + 1},
        {rect.left + ((rect.right - rect.left) / 2) + 1, rect.top + ((rect.bottom - rect.top) / 2) + 1},
        {rect.left + ((rect.right - rect.left) / 2) + 1, rect.bottom},
        {rect.left + ((rect.right - rect.left) / 2) - 1, rect.bottom},
        {rect.left + ((rect.right - rect.left) / 2) - 1, rect.top + ((rect.bottom - rect.top) / 2) + 1},
        {rect.left, rect.top + ((rect.bottom - rect.top) / 2) + 1}
    };

    hrgnplus = CreatePolygonRgn(arrowpoints, 12, WINDING);

    if(m_isdragging == wyFalse && m_overtabcontrol == IDC_PLUSSIGN && PtInRect(&borderrect, *pnt) && m_closebuttondowntab == -1
        && (m_tabcontroldown == -1 || m_tabcontroldown == IDC_PLUSSIGN))
    {
        SetGradientArrayOnMouseOver(NULL, NULL, vertex);
        hbrush = CreateSolidBrush(m_colorinfo.m_tabtext);
    }
    else
    {
        SetGradientArrayOnUnFocus(NULL, NULL, vertex);
        //SetPlusGradientArrayOnUnFocus(vertex, wyTrue);
        hbrush = CreateSolidBrush(m_colorinfo.m_tabcontrols);
    }

    //SetGradient(hdc, vertex);
    FillRgn(hdc, hrgnplus, hbrush);
    DeleteObject(hbrush);
    DeleteObject((HGDIOBJ)hrgnplus);
}

void 
CCustTab::PaintTab(wyBool ispaintonlyheader)
{
    RECT rect;
    wyInt32 style = 0;

    GetClientRect(m_hwnd, &rect);

    if(ispaintonlyheader == wyTrue)
    {
        //I realized the mistake, without WS_CLIPCHILDREN the tab will ask its children to redraw again :)
        style = GetWindowLongPtr(m_hwnd, GWL_STYLE);
        SetWindowLongPtr(m_hwnd, GWL_STYLE, style | WS_CLIPCHILDREN);
        rect.bottom = GetTabHeight() + TABTOPSPACE;
    }
    else
    {
        SetPaintTimer();
    }

    m_isdrawonlytabheader = ispaintonlyheader;
    InvalidateRect(m_hwnd, &rect, TRUE);
    UpdateWindow(m_hwnd);
    m_isdrawonlytabheader = wyFalse;

    if(ispaintonlyheader == wyTrue)
    {
        SetWindowLongPtr(m_hwnd, GWL_STYLE, style);
    }
}

void
CCustTab::OnWMSize()
{
    wyInt32 i, tabsize = 0;
    RECT rect;

    for(i = 0; i < m_tabs; ++i)
    {
        tabsize += m_tabdet[i].m_tabwidth;
    }

    GetClientRect(m_hwnd, &rect);

    if(tabsize + (m_isplussign == wyTrue ? SCROLLBUTTONWIDTH : 0) > rect.right)
    {
        m_isscroll = wyTrue;
        rect.right -= RIGHTPADDING(m_isplussign) + SCROLLBUTTONWIDTH;
        m_maxscrollpos = tabsize - (rect.right);
    }
    else
    {
        m_isscroll = wyFalse;
        m_maxscrollpos = 0;
        m_scrollpos = 0;
    }

    if(m_scrollpos > m_maxscrollpos)
    {
        m_scrollpos = m_maxscrollpos;
    }

    SetVisibleTabBoundary();
}

wyInt32
CCustTab::DrawFixedLengthTab(HDC hdcmem, RECT * prect, wyInt32 tabindex, wyBool ismouseovertab)
{
	HPEN    hboldpen, temppen;
	HBRUSH  holdbrush;
    HBRUSH  hbrush;
    wyInt32 red, blue, green;

    if(m_selectedtab == tabindex)
	{
        red = (COLOR16)(GetRValue(m_tabdet[tabindex].ctcitem.m_color));
        green = (COLOR16)(GetGValue(m_tabdet[tabindex].ctcitem.m_color));
	    blue  = (COLOR16)(GetBValue(m_tabdet[tabindex].ctcitem.m_color));
        red   = red + ( 17 * (0xfe - red)/20); //m_red;
	    green = green + ( 17 * (0xfe - green)/20);  //m_green;
	    blue  = blue + ( 17 * (0xfe - blue)/20); //m_blue;
        hbrush = CreateSolidBrush(RGB(red,green, blue));
        temppen = m_hpengrey;//CreatePen(PS_SOLID, 0,/*m_tabdet[tabindex].ctcitem.m_color*/) ;//(HPEN)SelectObject(hdcmem, m_hpenbottomline);
        hboldpen = (HPEN)SelectObject(hdcmem, temppen);
	}
	else
	{
        if(ismouseovertab)
        {
            red   = (COLOR16)(GetRValue(m_tabdet[tabindex].ctcitem.m_color));
		    green = (COLOR16)(GetGValue(m_tabdet[tabindex].ctcitem.m_color));
		    blue  = (COLOR16)(GetBValue(m_tabdet[tabindex].ctcitem.m_color));
		    
            red   = red + ( 7 * (0xff - red)/16); //m_red;
		    green = green + ( 7 * (0xff - green)/16);  //m_green;
		    blue  = blue + ( 7 * (0xff - blue)/16); //m_blue;
            hbrush = CreateSolidBrush(RGB(red,green, blue));
        }
        else
            hbrush = CreateSolidBrush(m_tabdet[tabindex].ctcitem.m_color);/*m_tabdet[tabindex].ctcitem.m_color*///);
        
        hboldpen = (HPEN)SelectObject(hdcmem, (ismouseovertab == wyTrue) ? m_hpenhighlight : m_hpenactivesep);
	}

	holdbrush = (HBRUSH)SelectObject(hdcmem, hbrush);
					
	// Draws a filled rounded rect using the selected brush and pen
	if(m_selectedtab == tabindex)
	{
		RoundRect(hdcmem, prect->left + 1, prect->top + 1,  prect->left + 1 + FIXED_WIDTH/*CalculateTabLength(hdcmem,m_selectedtab)*/ + 1, prect->bottom + IMAGEWIDTH, 10, 10);
	
		MoveToEx(hdcmem, prect->left + 1 + FIXED_WIDTH + 1, prect->top+5, NULL);
		LineTo(hdcmem, prect->left + 1 + FIXED_WIDTH + 1, prect->bottom + IMAGEWIDTH);
	}
	else
	{
		RoundRect(hdcmem, prect->left + 1, prect->top + 3, prect->left + 1 + FIXED_WIDTH/*CalculateTabLength(hdcmem,tabindex)*/ + 1, prect->bottom + IMAGEWIDTH, 10, 10);
		/*MoveToEx(hdcmem, prect->left + 1 + FIXED_WIDTH + 1, prect->top + 3, NULL);
		LineTo(hdcmem, prect->left + 1 + FIXED_WIDTH + 1,  prect->bottom + IMAGEWIDTH);*/
	}
	
    /*if(m_selectedtab == tabindex)
        DeleteObject(temppen);*/
    //else//restore the old pen and brush
	SelectObject (hdcmem, hboldpen);
	SelectObject (hdcmem, holdbrush);
    DeleteObject(hbrush);
    
    return m_tabdet[tabindex].m_tabwidth;
}


wyInt32 
CCustTab::DrawVaryingLengthTab(HDC hdcmem, RECT* prect, wyInt32 tabindex, wyBool ismouseovertab)
{
    HPEN    holdpen;
	HBRUSH  holdbrush;
    HBRUSH  hbrush;

    hbrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

    if(m_selectedtab == tabindex)
	{
        holdpen = (HPEN)SelectObject(hdcmem, m_hpengrey);
	}
	else
	{
		holdpen = (HPEN)SelectObject(hdcmem, (ismouseovertab == wyTrue) ? m_hpenhighlight : m_hpenactivesep);
	}

	holdbrush = (HBRUSH)SelectObject(hdcmem, hbrush);

    if(tabindex == 0 || (ismouseovertab == wyTrue && tabindex - 1 != m_selectedtab) || m_selectedtab == tabindex)
    {
        MoveToEx(hdcmem, prect->left, prect->bottom - 1, NULL);
        LineTo(hdcmem, prect->left, prect->top - 1);
    }

    MoveToEx(hdcmem, prect->left + 1, prect->top, NULL);
    LineTo(hdcmem, prect->left + m_tabdet[tabindex].m_tabwidth + 1, prect->top);
    MoveToEx(hdcmem, prect->left + m_tabdet[tabindex].m_tabwidth, prect->top + 1, NULL);
    LineTo(hdcmem, prect->left + m_tabdet[tabindex].m_tabwidth, prect->bottom - 1);

    SelectObject (hdcmem, holdpen);
	SelectObject (hdcmem, holdbrush);
    DeleteObject(hbrush);    
    return m_tabdet[tabindex].m_tabwidth;
}

wyInt32 CCustTab::CalculateTabLength(HDC hdc, wyInt32 index)
{
    wyInt32 size = 0;
    RECT rect = {0};
    wyString temp;
    HFONT hfont;

    if(m_isfixedlength == wyTrue)
    {
        return FIXED_WIDTH + 2;
    }

    temp.SetAs(m_tabdet[index].ctcitem.m_psztext);
    hfont = (HFONT)SelectObject(hdc, m_hselectedfont);
    DrawText(hdc, temp.GetAsWideChar(), -1, &rect, DT_CALCRECT);
    SelectObject(hdc, hfont);
    size = CLOSERECTPADDING + ((m_tabdet[index].ctcitem.m_mask & CTBIF_IMAGE) ? IMAGEWIDTH : 0) + rect.right + CLOSERECTPADDING + (m_extflagclose ? CLOSERECTWIDTH + CLOSERECTPADDING : 0);

    return size;
}


void 
CCustTab::EnableAddButton(wyBool enable)
{
    m_isplussign = enable;
    OnWMSize();
    PaintTab(wyTrue);
}

void 
CCustTab::SetVisibleTabBoundary()
{
    wyInt32 left = SCROLLBUTTONWIDTH, i;
    RECT rect;

    if(m_isscroll == wyFalse)
    {
        m_leftmostvisibletab = 0;
        m_rightmostvisibletab = m_tabs - 1;
        return;
    }
    
    GetClientRect(m_hwnd, &rect);
    rect.left = SCROLLBUTTONWIDTH;
    rect.right -= RIGHTPADDING(m_isplussign);
    m_rightmostvisibletab = m_leftmostvisibletab = -1;

    for(i = 0, left -= m_scrollpos; i < m_tabs; left += m_tabdet[i++].m_tabwidth)
    {
        if(left < rect.left && left + m_tabdet[i].m_tabwidth > rect.right)
        {
            m_leftmostvisibletab /*= m_rightmostvisibletab*/ = i;
            break;
        }

        if(left < rect.left)
        {
            continue;
        }

        if(left > rect.right)
        {
            break;
        }

        if(m_leftmostvisibletab == -1 && left >= rect.left)
        {
            m_leftmostvisibletab = i;
        }

        if(m_leftmostvisibletab != -1 && left + m_tabdet[i].m_tabwidth <= rect.right)
        {
            m_rightmostvisibletab = i;
        }
    }

    if(m_rightmostvisibletab == -1)
    {
        m_leftmostvisibletab = -1;
    }
}

void 
CCustTab::SetIconList(HIMAGELIST hiconlist)
{
    wyInt32 i;

    for(i = 0; i < m_tabs; ++i)
    {
        KillTimer(m_hwnd, ICONANIMATETIMER_BASE + i);
        m_tabdet[i].m_iconindex = -1;
        m_tabdet[i].m_animationdelay = -1;
    }

    m_hiconlist = hiconlist;
}

wyBool 
CCustTab::StartIconAnimation(wyInt32 tabindex, wyInt32 delay)
{
    if(tabindex < 0 || tabindex >= m_tabs || m_hiconlist == NULL)
    {
        return wyFalse;
    }

    StopIconAnimation(tabindex, wyFalse);
    m_tabdet[tabindex].m_animationdelay = delay;
    SetTimer(m_hwnd, ICONANIMATETIMER_BASE + tabindex, delay, IconAnimateTimerProc);
    return wyTrue;
}

wyBool 
CCustTab::StopIconAnimation(wyInt32 tabindex, wyBool isrepaint)
{
    if(tabindex < 0 || tabindex >= m_tabs || m_hiconlist == NULL)
    {
        return wyFalse;
    }

    KillTimer(m_hwnd, ICONANIMATETIMER_BASE + tabindex);
    m_tabdet[tabindex].m_iconindex = -1;
    m_tabdet[tabindex].m_animationdelay = -1;

    if(isrepaint == wyTrue)
    {
        PostMessage(m_hwnd, UM_PAINTTABHEADER, 0, 0);
    }

    return wyTrue;
}

VOID CALLBACK 
CCustTab::IconAnimateTimerProc(HWND hwnd, UINT message, UINT_PTR idevent, DWORD dwtime)
{
    CCustTab	*pct = GetCustTabCtrlData(hwnd);
    pct->DrawNextAnimationIcon(GET_INDEX_FROM_ICONTIMER(idevent));
}

void 
CCustTab::DrawNextAnimationIcon(wyInt32 tabindex)
{
    RECT    rect, temprect = {0};
    HICON   hicon;
    HDC     hdc, hmemdc;
    HBITMAP hbmp;
    POINT   pt;
    HRGN    hrgn = NULL;

    GetClientRect(m_hwnd, &rect);
    
    if(m_tabdet[tabindex].m_tabarea.left > rect.right || m_tabdet[tabindex].m_tabarea.right < 0 || m_hiconlist == NULL)
    {
        return;
    }

    //if scroll buttons are there, we need to clip the device region exculding the button width
    if(m_isscroll == wyTrue)
    {
        hrgn = CreateRectRgn(rect.left + SCROLLBUTTONWIDTH, rect.top, rect.right - RIGHTPADDING(m_isplussign), rect.bottom);
    }

    if(m_tabdet[tabindex].ctcitem.m_iimage != -1 && (m_tabdet[tabindex].ctcitem.m_mask & CTBIF_IMAGE))
    {
        GetCursorPos(&pt);
        ScreenToClient(m_hwnd, &pt);
        temprect.right = m_tabdet[tabindex].m_tabarea.right - m_tabdet[tabindex].m_tabarea.left;
        temprect.bottom = m_tabdet[tabindex].m_tabarea.bottom - m_tabdet[tabindex].m_tabarea.top;
        rect.left = m_tabdet[tabindex].m_tabarea.left + CLOSERECTPADDING;
        rect.top = m_tabdet[tabindex].m_tabarea.top + ((m_size.cy + EXTRAHEIGHT) / 2) - 8;
        hdc = GetDC(m_hwnd);

        if(hrgn)
        {
            SelectClipRgn(hdc, hrgn);
        }

        hmemdc = CreateCompatibleDC(hdc);
        hbmp = CreateCompatibleBitmap(hdc, temprect.right, temprect.bottom);
        hbmp = (HBITMAP)SelectObject(hmemdc, hbmp);
        DrawTab(hmemdc, tabindex, &temprect, &m_tabdet[tabindex].m_tabarea, &pt, NULL);
        m_tabdet[tabindex].m_iconindex = (m_tabdet[tabindex].m_iconindex + 1) % ImageList_GetImageCount(m_hiconlist);
        hicon = ImageList_ExtractIcon(0, m_hiconlist, m_tabdet[tabindex].m_iconindex);
        SetBkMode(hmemdc, TRANSPARENT);
        DrawIconEx(hmemdc, CLOSERECTPADDING, ((m_size.cy + EXTRAHEIGHT) / 2) - 8, hicon, 16, 16, 0, NULL, DI_NORMAL);
        DestroyIcon(hicon);
        BitBlt(hdc, rect.left, rect.top, 16, 16, hmemdc, CLOSERECTPADDING, ((m_size.cy + EXTRAHEIGHT) / 2) - 8, SRCCOPY);
        hbmp = (HBITMAP)SelectObject(hmemdc, hbmp);
        DeleteBitmap(hbmp);
        DeleteDC(hmemdc);
        ReleaseDC(m_hwnd, hdc);
    }

    if(hrgn)
    {
        DeleteRgn(hrgn);
    }
}

void 
CCustTab::SetDefaultColorInfo()
{
    m_colorinfo.m_tabbg1 = TABCOLOR_TAB_BG_1;
    m_colorinfo.m_tabbg2 = TABCOLOR_TAB_BG_2;
    m_colorinfo.m_seltabfg1 = TABCOLOR_SELTAB_FG_1;
    m_colorinfo.m_seltabfg2 = TABCOLOR_SELTAB_FG_2;
    m_colorinfo.m_seltabtext = TABCOLOR_SELTAB_TEXT;
    m_colorinfo.m_tabtext = TABCOLOR_TAB_TEXT;
    m_colorinfo.m_activesep = TABCOLOR_ACTIVESEP_PEN;
    m_colorinfo.m_inactivesep = TABCOLOR_INACTIVESEP_PEN;
    m_colorinfo.m_closebutton = TABCOLOR_CLOSEBUTTON;
    m_colorinfo.m_tabfg = TABCOLOR_TAB_FG;
    m_colorinfo.m_bottomline = TABCOLOR_SELTAB_FG_2;
    m_colorinfo.m_dragarrow = TABCOLOR_DRAGARROW;
    m_colorinfo.m_tabcontrols = TABCOLOR_PLUSBUTTON;
    m_colorinfo.m_highlightsep = m_colorinfo.m_activesep;
    m_colorinfo.m_hottabfg1 = TABCOLOR_SELTAB_FG_2;
    m_colorinfo.m_hottabfg2 = TABCOLOR_TAB_FG;
    m_colorinfo.m_linkcolor = TABCOLOR_LINK;
    m_colorinfo.m_border = wyFalse;
    m_colorinfo.m_mask = CTCF_ALLEXECPT_CTCF_BOTTOMLINE;
    m_colorinfo.m_mask = m_colorinfo.m_mask & (~TABCOLOR_LINK);
}

void CCustTab::CreateResources()
{
    if(m_hpenactivesep)
    {
        DeletePen(m_hpenactivesep);
        m_hpenactivesep = NULL;
    }

    if(m_hpenclosebutton)
    {
        DeletePen(m_hpenclosebutton);
        m_hpenclosebutton = NULL;
    }

    if(m_hpengrey)
    {
        DeletePen(m_hpengrey);
        m_hpengrey = NULL;
    }

    if(m_hpenbottomline)
    {
        DeletePen(m_hpenbottomline);
        m_hpenbottomline = NULL;
    }

    if(m_hpenhighlight)
    {
        DeletePen(m_hpenhighlight);
        m_hpenhighlight = NULL;
    }

    m_hpenactivesep = CreatePen(PS_SOLID, 1, m_colorinfo.m_activesep);
    m_hpenclosebutton = CreatePen(PS_SOLID, 0, m_colorinfo.m_closebutton);
    m_hpengrey = CreatePen(PS_SOLID, 2, m_colorinfo.m_inactivesep);
    m_hpenhighlight = CreatePen(PS_SOLID, 0, m_colorinfo.m_highlightsep);

    if(m_colorinfo.m_mask & CTCF_BOTTOMLINE)
    {
        CreateBottomLinePen();
    }
}

void CCustTab::CreateBottomLinePen()
{
    wyInt32 red, blue, green;
    if(m_hpenbottomline)
    {
        DeletePen(m_hpenbottomline);
        m_hpenbottomline = NULL;
    }

    
    if(m_tabdet && (m_selectedtab >= 0) && (m_tabdet[m_selectedtab].ctcitem.m_mask & CTBIF_COLOR)  
        && (m_tabdet[m_selectedtab].ctcitem.m_color  >= 0) && m_isfixedlength)
    {
    
        red = (COLOR16)(GetRValue(m_tabdet[m_selectedtab].ctcitem.m_color));
		green = (COLOR16)(GetGValue(m_tabdet[m_selectedtab].ctcitem.m_color));
		blue  = (COLOR16)(GetBValue(m_tabdet[m_selectedtab].ctcitem.m_color));
  //      red   = red + ( 13 * (0xfe - red)/16); //m_red;
		//green = green + ( 13 * (0xfe - green)/16);  //m_green;
		//blue  = blue + ( 13 * (0xfe - blue)/16); //m_blue;
        m_hpenbottomline = CreatePen(PS_SOLID, BOTTOMLINEHEIGHT, RGB(red, green, blue));
    }
    else
        m_hpenbottomline = CreatePen(PS_SOLID, BOTTOMLINEHEIGHT, m_colorinfo.m_bottomline);
}

void
CCustTab::GetColorInfo(LPTABCOLORINFO pcolorinfo)
{
    *pcolorinfo = m_colorinfo;
}

void
CCustTab::SetColorInfo(LPTABCOLORINFO pcolorinfo)
{
    long style;

    if(!pcolorinfo)
    {
        SetDefaultColorInfo();
    }
    else
    {
        if(pcolorinfo->m_mask & CTCF_TABBG1)
        {
            m_colorinfo.m_tabbg1 = pcolorinfo->m_tabbg1;
        }

        if(pcolorinfo->m_mask & CTCF_TABBG2)
        {
            m_colorinfo.m_tabbg2 = pcolorinfo->m_tabbg2;
        }

        if(pcolorinfo->m_mask & CTCF_SELTABFG1)
        {
            m_colorinfo.m_seltabfg1 = pcolorinfo->m_seltabfg1;
        }

        if(pcolorinfo->m_mask & CTCF_SELTABFG2)
        {
            m_colorinfo.m_seltabfg2 = pcolorinfo->m_seltabfg2;
        }

        if(pcolorinfo->m_mask & CTCF_SELTABTEXT)
        {
            m_colorinfo.m_seltabtext = pcolorinfo->m_seltabtext;
        }

        if(pcolorinfo->m_mask & CTCF_TABTEXT)
        {
            m_colorinfo.m_tabtext = pcolorinfo->m_tabtext;
        }

        if(pcolorinfo->m_mask & CTCF_ACTIVESEP)
        {
            m_colorinfo.m_activesep = pcolorinfo->m_activesep;
            m_colorinfo.m_highlightsep = pcolorinfo->m_activesep;
        }

        if(pcolorinfo->m_mask & CTCF_INACTIVESEP)
        {
            m_colorinfo.m_inactivesep = pcolorinfo->m_inactivesep;
        }

        if(pcolorinfo->m_mask & CTCF_CLOSEBUTTON)
        {
            m_colorinfo.m_closebutton = pcolorinfo->m_closebutton;
        }

        if(pcolorinfo->m_mask & CTCF_TABFG)
        {
            m_colorinfo.m_tabfg = pcolorinfo->m_tabfg;
        }

        if(pcolorinfo->m_mask & CTCF_DRAGARROW)
        {
            m_colorinfo.m_dragarrow = pcolorinfo->m_dragarrow;
        }

        if(pcolorinfo->m_mask & CTCF_TABCONTROLS)
        {
            m_colorinfo.m_tabcontrols = pcolorinfo->m_tabcontrols;
        }

        if(pcolorinfo->m_mask & CTCF_HIGHLIGHTSEP)
        {
            m_colorinfo.m_highlightsep = pcolorinfo->m_highlightsep;
        }

        if(pcolorinfo->m_mask & CTCF_BORDER)
        {
            m_colorinfo.m_border = pcolorinfo->m_border;
        }

        if(pcolorinfo->m_mask & CTCF_BOTTOMLINE)
        {

            m_colorinfo.m_bottomline = m_colorinfo.m_seltabfg2;
        }

        if(pcolorinfo->m_mask & CTCF_HOTTABFG1)
        {
            m_colorinfo.m_hottabfg1 = pcolorinfo->m_hottabfg1;
        }

        if(pcolorinfo->m_mask & CTCF_HOTTABFG2)
        {
            m_colorinfo.m_hottabfg2 = pcolorinfo->m_hottabfg2;
        }

        if(pcolorinfo->m_mask & CTCF_LINK)
        {
            m_colorinfo.m_linkcolor = pcolorinfo->m_linkcolor;
        }

        m_colorinfo.m_mask |= pcolorinfo->m_mask;
        style = GetWindowLongPtr(m_hwnd, GWL_STYLE);

        if(m_colorinfo.m_border == wyTrue)
        {
            SetWindowLongPtr(m_hwnd, GWL_STYLE, style | WS_BORDER);
        }
        else
        {
            SetWindowLongPtr(m_hwnd, GWL_STYLE, style & (~WS_BORDER));
        }
    }

    CreateResources();
}

wyInt32
CCustTab::GetTabHeight()
{
    return m_size.cy + EXTRAHEIGHT - TABTOPSPACE + (m_hpenbottomline ? BOTTOMLINEHEIGHT : 0);
}

//------------- external functions -----------------------

/*Function is used to initialize Custom Tab*/
ATOM InitCustomTab()
{
	WNDCLASSEX wc;

	wc.cbSize         = sizeof(wc);
	wc.lpszClassName  = szCustomTabName;
	wc.hInstance      = GetModuleHandle(0);
	wc.lpfnWndProc    =(WNDPROC)CCustTab::CustomTabWndProc;
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon          = 0;
	wc.lpszMenuName   = 0;
	wc.hbrBackground  = (HBRUSH)GetSysColorBrush(COLOR_BTNFACE);
	wc.style          = CS_DBLCLKS;
	wc.cbClsExtra     = 0;
	wc.cbWndExtra     = 0;
	wc.hIconSm        = 0;

	return RegisterClassEx(&wc);
}

HWND CreateCustomTab(HWND hwndParent, wyInt32 x, wyInt32 y, wyInt32 width, wyInt32 height, CTBWNDPROC lpTBWndProc, LPARAM lParam, LPARAM style)
{
	HWND	hwndtab;
	
    hwndtab = CreateWindowEx(WS_EX_CONTROLPARENT, szCustomTabName, szCustomTabName, WS_VISIBLE | WS_CHILD | WS_TABSTOP | style, 
								x, y, width, height, hwndParent,(HMENU)lParam, GetModuleHandle(0),(LPVOID)lpTBWndProc);

	if(!hwndtab)
		return NULL;

	CCustTab	*pct = GetCustTabCtrlData(hwndtab);
    
    if(pct)
    {
	    pct->SetId(lParam);
    }

	return hwndtab;
}

//create tooltip for tab
VOID CreateCustomTabTooltip(HWND hwndtab)
{
	CCustTab	*pct	= GetCustTabCtrlData(hwndtab);
	
	pct->m_hwndtooltip = CreateWindowEx( WS_EX_TOPMOST,
										TOOLTIPS_CLASS,
										NULL,
										WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,		
										CW_USEDEFAULT,
										CW_USEDEFAULT,
										CW_USEDEFAULT,
										CW_USEDEFAULT,
										hwndtab,
										NULL,
										GetModuleHandle(0),
										NULL);

	//return if CreateWindowEx fail
	if(pct->m_hwndtooltip == NULL)
		return ;

	SetWindowPos(pct->m_hwndtooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	

	//fill the TOOLINFO struct
	pct->m_toolinfo.cbSize = sizeof(TOOLINFO);
	pct->m_toolinfo.uFlags = TTF_SUBCLASS | TTF_ABSOLUTE;
	pct->m_toolinfo.hinst = GetModuleHandle(0);
	pct->m_toolinfo.hwnd = hwndtab;
	pct->m_toolinfo.lpszText = LPSTR_TEXTCALLBACK;
	pct->m_toolinfo.uId = 0;
	 
	//Get the client rect oftab control
	GetClientRect(hwndtab, &pct->m_toolinfo.rect);
	
	//add the tool tip
	SendMessage(pct->m_hwndtooltip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &pct->m_toolinfo);
    SendMessage(pct->m_hwndtooltip, TTM_SETDELAYTIME, TTDT_AUTOMATIC, 800);

	return;
}

HWND CreateCustomTabEx(HWND hwndParent, wyInt32 x, wyInt32 y, wyInt32 width, wyInt32 height,  CTBWNDPROC lpTBWndProc, DWORD styles, LPARAM lParam)
{

	HWND	hwndtab;
	
	hwndtab = CreateWindowEx(WS_EX_STATICEDGE,  szCustomTabName, L"Custom Tab Control", styles, 
								x, y, width, height, hwndParent,(HMENU)lParam, GetModuleHandle(0),(LPVOID)lpTBWndProc);

	if(!hwndtab)
		return NULL;

    CCustTab	*pct = GetCustTabCtrlData(hwndtab);
    if(pct)
	    pct->SetId(lParam);

	return hwndtab;
}

wyInt32 CustomTab_InsertItem(HWND hwnd, wyInt32 position, LPCTCITEM item)
{
	wyInt32 ret;

	CCustTab	*pct = GetCustTabCtrlData(hwnd);
	ret = pct->InsertItem(position, item);
    pct->OnWMSize();
    pct->PaintTab();

	return ret;
}

wyBool CustomTab_SetCurSel(HWND hwnd , wyInt32 index, WPARAM wparam)
{
	wyBool ret;

	CCustTab	*pct = GetCustTabCtrlData(hwnd);
	ret = pct->SetCurSel(index, wparam);
    pct->PaintTab();
	return ret;
}


LPARAM CustomTab_GetLongData(HWND hwnd)
{
	CCustTab	*pct = GetCustTabCtrlData(hwnd);

	return pct->GetLongData();
}

wyBool CustomTab_DeleteItem(HWND hwnd, wyInt32 tabcount,wyBool ispositionrequired)
{
	wyBool ret;

	CCustTab	*pct = GetCustTabCtrlData(hwnd);
	ret = pct->DeleteItem(tabcount, wyFalse ,ispositionrequired);
    pct->OnWMSize();
    pct->PaintTab();

	return ret;
}

wyBool CustomTab_RemoveAllItem(HWND hwnd)
{
	wyBool ret;

	CCustTab	*pct = GetCustTabCtrlData(hwnd);
	ret = pct->RemoveAllItem();
    pct->OnWMSize();
	return ret;
}

LPARAM CustomTab_SetItemLongValue(HWND hwnd, wyInt32 tabcount, LPARAM lParam)
{
	CCustTab	*pct = GetCustTabCtrlData(hwnd);

	return pct->SetItemLongValue(tabcount, lParam);
}

wyInt32 CustomTab_GetItemCount(HWND hwnd)
{
	CCustTab	*pct = GetCustTabCtrlData(hwnd);

	return pct->GetItemCount();
}

wyInt32 CustomTab_GetCurFocus(HWND hwnd)
{
	CCustTab	*pct = GetCustTabCtrlData(hwnd);

	return pct->GetCurFocus();
}

wyInt32 CustomTab_GetCurSel(HWND hwnd)
{
	CCustTab	*pct = GetCustTabCtrlData(hwnd);

	return pct->GetCurFocus();
}

wyInt32 CustomTab_GetVisibleTabCount(HWND hwnd)
{
	CCustTab	*pct = GetCustTabCtrlData(hwnd);

	return pct->GetVisibleTabCount();
}

wyBool CustomTab_GetItem(HWND hwnd, wyInt32 index, LPCTCITEM pitem)
{
	CCustTab	*pct = GetCustTabCtrlData(hwnd);

	return pct->GetItem(index, pitem);
}

CCustTab* GetCustTabCtrlData(HWND hwnd)
{
	return(CCustTab*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

wyInt32 SetCustTabCtrlData(HWND hwnd, CCustTab* ccp)
{
	return SetWindowLongPtr(hwnd, GWLP_USERDATA,(LONG_PTR)ccp);
}

wyInt32	CustomTab_SetMinTabWidth(HWND hwnd, wyInt32 cx)
{
	CCustTab	*pct = GetCustTabCtrlData(hwnd);

	return pct->SetMinTabWidth(cx);
}

wyBool CustomTab_SetItem(HWND hwnd, wyInt32 index, LPCTCITEM pitem)
{
    wyBool ret;

	CCustTab	*pct = GetCustTabCtrlData(hwnd);
    ret = pct->SetItem(index, pitem);
    pct->OnWMSize();
    pct->PaintTab(wyTrue);

	return ret;
}

//set color for tab
wyBool CustomTab_SetItemColor(HWND hwnd, wyInt32 index, LPCTCITEM pitem)
{
	CCustTab	*pct = GetCustTabCtrlData(hwnd);

	return pct->SetItemColor(index, pitem);
}


void CustomTab_SetClosable(HWND hwnd, wyBool val, wyInt32 mintabcount, wyBool ispaint)
{
	CCustTab	*pct = GetCustTabCtrlData(hwnd);

	return pct->SetClosable(val, mintabcount, ispaint);
}

wyBool CustomTab_EnsureVisible(HWND hwnd, wyInt32 index, wyBool isanimate)
{
	CCustTab	*pct = GetCustTabCtrlData(hwnd);

	return pct->EnsureVisible(index, isanimate);
}

wyBool CustomTab_IsMenuEnabled(HWND hwnd, wyInt32 index)
{
    CCustTab	*pct = GetCustTabCtrlData(hwnd);

	return pct->IsMenuEnabled(index);
}

//check if the tab is of fixed length
void 
CustomTab_IsFixedLength(HWND hwnd, wyBool isfixedlength)
{
	 CCustTab	*pct = GetCustTabCtrlData(hwnd);

	return pct->SetFixedLengthProp(isfixedlength);
}

//check if the tab is of fixed length
void 
CustomTab_EnableDrag(HWND hwnd, HWND hwnddragwin,  wyBool isdragenable)
{
	 CCustTab	*pct = GetCustTabCtrlData(hwnd);

	 return pct->SetDragProperties(hwnddragwin, isdragenable);
}

void
CustomTab_SetBufferedDrawing(wyBool enable)
{
    CCustTab::m_isdrawwithdoublebuffering = enable;
}

void
CustomTab_EnableAddButton(HWND hwnd, wyBool enable)
{
    CCustTab	*pct = GetCustTabCtrlData(hwnd);
    pct->EnableAddButton(enable);
}

void	
CustomTab_BlockEvents(HWND hwnd, wyBool isblock)
{
	CCustTab	*pct = GetCustTabCtrlData(hwnd);

	pct->SetBlockEventsFlag(isblock);
}

void
CustomTab_SetIconList(HWND hwnd, HIMAGELIST hiconlist)
{
    CCustTab	*pct = GetCustTabCtrlData(hwnd);
    pct->SetIconList(hiconlist);
}

wyBool
CustomTab_StartIconAnimation(HWND hwnd, wyInt32 tabindex, wyInt32 delay)
{
    CCustTab	*pct = GetCustTabCtrlData(hwnd);
    return pct->StartIconAnimation(tabindex, delay);
}

wyBool
CustomTab_StopIconAnimation(HWND hwnd, wyInt32 tabindex)
{
    CCustTab	*pct = GetCustTabCtrlData(hwnd);
    return pct->StopIconAnimation(tabindex, wyTrue);
}

wyInt32
CustomTab_GetTabHeight(HWND hwnd)
{
    CCustTab	*pct = GetCustTabCtrlData(hwnd);
    return pct ? pct->GetTabHeight() : 0;
}

void
CustomTab_SetColorInfo(HWND hwnd, LPTABCOLORINFO pcolorinfo)
{
    CCustTab	*pct = GetCustTabCtrlData(hwnd);
    pct->SetColorInfo(pcolorinfo);
}

void
CustomTab_GetColorInfo(HWND hwnd, LPTABCOLORINFO pcolorinfo)
{
    CCustTab* pct = GetCustTabCtrlData(hwnd);
    pct->GetColorInfo(pcolorinfo);
}

void 
CustomTab_SetPaintTimer(HWND hwnd)
{
    CCustTab* pct = GetCustTabCtrlData(hwnd);
    pct->SetPaintTimer();
}

const wyChar*				
CustomTab_GetTitle(HWND hwnd, wyInt32 index, wyString* title)
{
	CCustTab* pct = GetCustTabCtrlData(hwnd);
	return pct->GetItemTitle(index, title);
}

const wyChar*				
CustomTab_GetTooltip(HWND hwnd, wyInt32 index, wyString* Tooltip)
{
	CCustTab* pct = GetCustTabCtrlData(hwnd);
	return pct->GetItemTooltip(index, Tooltip);
}