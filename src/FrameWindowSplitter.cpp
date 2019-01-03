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

#include "FrameWindowSplitter.h"
#include "GUIHelper.h"

extern	PGLOBALS		pGlobals;

// Class to implement the splitters. Both the horizontal as well as vertical splitter is 
// implemented.
FrameWindowSplitter::FrameWindowSplitter(HWND hwnd, wyUInt32 pleftortoppercent)
{
    m_hwndprevfocus     = NULL;
	m_leftortoppercent  = pleftortoppercent;
	m_isdragged	        = wyFalse;
	m_lastleftpercent   = 0;
	m_lasttoppercent    = 35;
	m_hwndparent        = hwnd;
    m_width             = 4; //GetSystemMetrics(SM_CXSIZEFRAME) + 1;
    m_x                 = 0;
    m_prevstyle         = 0;
    m_hdc               = NULL;
    m_hpen              = NULL;
}

FrameWindowSplitter::~FrameWindowSplitter()
{
    EndDrag(wyFalse);
}

wyBool
FrameWindowSplitter::Create()
{
	CreateSplitter(m_hwndparent);
	return wyTrue;
}

// function to create splitter window.
HWND
FrameWindowSplitter::CreateSplitter(HWND hwnd)
{
    wyUInt32        style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
	HWND			hwndsplitter;

	// create the splitter window
	hwndsplitter = CreateWindowEx(0, 
		            VSPLITTER_WINDOW_CLASS_NAME_STR, 
		            L"", style, 0, 0, 0, 0, 	hwnd, 
		            (HMENU)IDC_VSPLITTER, 
		            pGlobals->m_pcmainwin->GetHinstance(), this);

	_ASSERT(hwndsplitter != NULL);

	ShowWindow(hwndsplitter, SW_SHOWDEFAULT);    
	VERIFY(UpdateWindow(hwndsplitter));

	return hwndsplitter;
}

// callback function for the splitter window.
LRESULT	CALLBACK 
FrameWindowSplitter::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
   // Get a pointer to the window class object.
    FrameWindowSplitter	*pFrameWindowSplitter = (FrameWindowSplitter *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
    case WM_NCCREATE:
        // Get the initial creation pointer to the window object
		pFrameWindowSplitter = (FrameWindowSplitter *)((CREATESTRUCT *)lparam)->lpCreateParams;        
		pFrameWindowSplitter->m_hwnd = hwnd;
		SetWindowLongPtr(hwnd, GWLP_USERDATA,(LONG_PTR)pFrameWindowSplitter);
        break;

	case WM_PAINT:
		return pFrameWindowSplitter->OnPaint();
		
	case WM_MOUSEMOVE:
		pFrameWindowSplitter->MouseMove();
		return 0;

	case WM_LBUTTONDOWN:
		{
			MDIWindow * pcquerywnd = (MDIWindow*)GetWindowLongPtr(pFrameWindowSplitter->m_hwndparent, GWLP_USERDATA);
			if(pcquerywnd->m_isobjbrowvis == wyFalse)
				break;
			else
			{
                pFrameWindowSplitter->m_prevstyle = GetWindowLongPtr(pFrameWindowSplitter->m_hwndparent, GWL_STYLE);
                SetWindowLongPtr(pFrameWindowSplitter->m_hwndparent, GWL_STYLE, pFrameWindowSplitter->m_prevstyle & ~WS_CLIPCHILDREN);
                pFrameWindowSplitter->m_hwndprevfocus = GetFocus();
                SetFocus(hwnd);
				SetCapture(hwnd);

				/*	starting from v4.2 BETA we stop the repaining of the two custom tab controls 
					so that flicker does not happen */
				pFrameWindowSplitter->m_isdragged	= wyTrue;
                pFrameWindowSplitter->MouseMove(wyTrue);
			}
		}
		break;
		
	case WM_LBUTTONUP:
		ReleaseCapture();
		break;

	case WM_CAPTURECHANGED:
		if(pFrameWindowSplitter->m_isdragged)
		{
            SetWindowLongPtr(pFrameWindowSplitter->m_hwndparent, GWL_STYLE, pFrameWindowSplitter->m_prevstyle);
            pFrameWindowSplitter->EndDrag(wyTrue);
			pFrameWindowSplitter->Resizeall();
			InvalidateRect(hwnd, NULL, TRUE);
			UpdateWindow(hwnd);
			pFrameWindowSplitter->m_isdragged = wyFalse;

            if(pFrameWindowSplitter->m_hwndprevfocus)
            {
                SetFocus(pFrameWindowSplitter->m_hwndprevfocus);
                pFrameWindowSplitter->m_hwndprevfocus = NULL;
            }
		}
		break;
	}

	return(DefWindowProc(hwnd, message, wparam, lparam));
}

// function to resize the splitter window.
wyBool
FrameWindowSplitter::Resize(wyBool isannouncements, wyBool iswindowresize)
{
	RECT	parentrect;
	LONG	ret;
	MDIWindow * pcquerywnd = (MDIWindow*)GetWindowLongPtr(m_hwndparent, GWLP_USERDATA);

	_ASSERT(pcquerywnd);

	VERIFY(GetClientRect(m_hwndparent, &parentrect));
	parentrect.top += 2;
	parentrect.bottom -= 2;
	parentrect.left += 2;

	parentrect.right -= 2;
	if (isannouncements)
	{
		//force splitter to 22% of the screen for announcements window
		//m_rect.left = 272;
		if (m_leftortoppercent < 22)
			m_leftortoppercent = 22;
	}

	//if (m_rect.left <= 2)
	//	m_rect.left = (long)((parentrect.right *((long)m_leftortoppercent / (float)100)));

	if (!pGlobals->m_pcmainwin->m_isresizing) {
		m_rect.left = (long)((parentrect.right *((long)m_leftortoppercent / (float)100))); 
	}

	m_rect.top = parentrect.top; //set the splitter size same as other controls
	m_rect.right = m_width;
	m_rect.bottom = parentrect.bottom - m_rect.top;

		if (m_rect.left < 2)
		{
			m_rect.left = 2;
		}
		//If Object Browser size is greater than main window size then retain Object Browser size
		if (m_rect.left + m_width > parentrect.right)
		{
			m_rect.left = parentrect.right - m_width;
		}


	if(pcquerywnd->m_isobjbrowvis == wyTrue)
		pGlobals->m_pcmainwin->SetSplitterPos(&m_rect, m_leftortoppercent);

		VERIFY(ret = MoveWindow(m_hwnd, m_rect.left, m_rect.top, m_rect.right, m_rect.bottom, TRUE));
		
	return wyTrue;
}

// Function to drag the window splitter window.
// Window is dragged by trapping the mousemove message 
void
FrameWindowSplitter::MouseMove(wyBool isinit)
{
	RECT		parentrect;
	POINT		curpos, pt;
    LOGBRUSH    lb = {0};
    WORD        bitmap[] = {0x00AA, 0x0055, 0x00AA, 0x0055, 0x00AA, 0x0055, 0x00AA, 0x0055};
    HBITMAP     hbitmap;

	VERIFY(GetClientRect(m_hwndparent, &parentrect));
	
	// Get the screen coordinates and convert it into client points.
	VERIFY(GetCursorPos (&curpos));
	VERIFY(ScreenToClient(m_hwndparent, &curpos));

    parentrect.left += 2;
    parentrect.right -= 2;
    parentrect.top += 2;
    parentrect.bottom -= 2;

	// logic to move the splitter.
	if(m_isdragged)
	{
        pt = curpos;

        if(isinit == wyTrue)
        {
            m_x = pt.x;
            m_hdc = GetDC(m_hwndparent);
            hbitmap = CreateBitmap(8, 8, 1, 1, bitmap);
            lb.lbStyle = BS_PATTERN; 
            lb.lbColor = 0;     
            lb.lbHatch = (ULONG_PTR)hbitmap;
            m_hpen = ExtCreatePen(PS_GEOMETRIC | PS_ENDCAP_FLAT , 4, &lb, 0, NULL); 
            DeleteBitmap(hbitmap);
        }

        if(curpos.x <= parentrect.left)
		{
			m_rect.left = parentrect.left;
			m_leftortoppercent = 0;
		} 
		else 
		{
            if(curpos.x <= (parentrect.right - m_width))
			{
				m_rect.left = curpos.x;
				m_leftortoppercent = ((curpos.x * 100 / parentrect.right * 100)/ 100);
			} 
			else 
			{
                m_rect.left = (parentrect.right - m_width);
				m_leftortoppercent = 100;
			}
		}
        
        DrawTrackerLine();

        if(isinit == wyTrue)
        {
            return;
        }

        m_x = pt.x;
        DrawTrackerLine();
	}
}

// Handles the paint message of the window. we trap this window so that we can give the window a 
// a beveled outlook.

LRESULT
FrameWindowSplitter::OnPaint()
{
	HDC			hdc;
	RECT		rect;
	PAINTSTRUCT ps;
    HPEN        hpen;

	// Begin Paint
	VERIFY(hdc = BeginPaint(m_hwnd, &ps));

	// Get the client rect of of the window.
	VERIFY(GetClientRect(m_hwnd, &rect));

    if(wyTheme::GetPen(PEN_VSPLITTER, &hpen))
    {
        hpen = SelectPen(hdc, hpen);
        MoveToEx(hdc, rect.right, rect.top, NULL);
        LineTo(hdc, rect.right, rect.bottom);
        hpen = SelectPen(hdc, hpen);
    }
    else
    {
	// now draw a beveled window depending upon whether it is horz splitter
	// or vertical splitter.
    DrawEdge(hdc, &rect, BDR_RAISEDOUTER | BDR_RAISEDOUTER, BF_RIGHT);
    }
		
	// End Painting
	EndPaint(m_hwnd, &ps);

	return 0;
}

void 
FrameWindowSplitter::DrawTrackerLine()
{
    wyInt32 rop;
    HPEN    hpen;
    
    rop = SetROP2(m_hdc, R2_NOTXORPEN);
    hpen = (HPEN)SelectObject(m_hdc, m_hpen);
    MoveToEx(m_hdc, m_x, m_rect.top + 1, NULL);
    LineTo(m_hdc, m_x, m_rect.bottom);
    SelectObject(m_hdc, hpen);
    SetROP2(m_hdc, rop);
}

void
FrameWindowSplitter::EndDrag(wyBool isdrawline)
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

HWND
FrameWindowSplitter::GetHwnd()
{
	return m_hwnd;
}

wyBool
FrameWindowSplitter::Resizeall()
{
	MDIWindow	*	pcquerywnd = (MDIWindow*)GetWindowLongPtr(m_hwndparent, GWLP_USERDATA);

    CustomTab_SetPaintTimer(pcquerywnd->m_pctabmodule->m_hwnd);
	pcquerywnd->Resize();

	return wyTrue;
}