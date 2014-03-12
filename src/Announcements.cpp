 /* Copyright (C) 2013 Webyog Inc.

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

#include "Announcements.h"
#include "Global.h"
#include "FrameWindowHelper.h"
#include "Include.h"
#include "Http.h"
#include "htmlayout.h"
#include "htmlrender.h"
#include "GUIHelper.h"


extern	PGLOBALS		pGlobals;

static struct AnnDOMEvtHandlerT : htmlayout::event_handler
{    	
    AnnDOMEvtHandlerT() : event_handler(HANDLE_ALL)
    {
    }

    virtual BOOL handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) 
    {
        return Announcements::HandleEvents(he, params);
    }
}AnnEvtHandler;

Announcements::Announcements()
{
	m_hwnd =		NULL;
	m_hwndHTML =	NULL;
	m_htmlx =		0;
	m_htmlh =		0;

	//m_hwndmain			= pGlobals->m_hwndclient;
}

Announcements::~Announcements()
{
		CloseWindow(m_hwndHTML);
		CloseWindow(m_hwnd);
		DestroyWindow(m_hwndHTML);
		DestroyWindow(m_hwnd);
}

wyBool 
Announcements::HandleAnnouncementsCheck(HWND hwnd)
{
	HWND		hwndHTML;
	wyString	htmlbuffer;
	WNDPROC		wndproc, origmainwndproc;
	CHttp		http;
	htmlbuffer.SetAs(pGlobals->m_announcementshtml.GetString());
	VERIFY(m_hwnd =  CreateWindow(ANNOUNCEMENTS_MAIN_WINDOW,
								L"", 
								WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 
								2, 450, 300, 180,
							    hwnd, (HMENU)-1,
								pGlobals->m_pcmainwin->GetHinstance(), NULL));

	VERIFY(hwndHTML = CreateWindowEx(0, ANNOUNCEMENTS_WINDOW, 
								L"AnnouncementsWindow",  
								WS_CHILD | WS_VISIBLE, 
                                0, 0, 270, 270, 
								m_hwnd, (HMENU)-1, 
								pGlobals->m_pcmainwin->GetHinstance(), NULL));

    origmainwndproc = (WNDPROC)SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)Announcements::AnnounceWndMainProc);
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)origmainwndproc);
	
	m_hwndHTML = hwndHTML;

	htmlayout::attach_event_handler(hwndHTML, &AnnEvtHandler);
	HTMLayoutSetCallback(hwndHTML, HTMLayoutNotifyHandler, 0);  
	//htmlayout::attach_event_handler(hwndHTML, &DOMEventsHandler);

    wndproc = (WNDPROC)SetWindowLongPtr(hwndHTML, GWLP_WNDPROC, (LONG_PTR)Announcements::AnnounceWndProc);
    SetWindowLongPtr(hwndHTML, GWLP_USERDATA, (LONG_PTR)wndproc);

	if(HTMLayoutLoadHtml(hwndHTML, (PBYTE)htmlbuffer.GetString(), htmlbuffer.GetLength()))
	{
		//if loadhtml fails return false and the html is malformed 
		HTMLayoutSetMode(hwndHTML, HLM_SHOW_SELECTION);
		return wyTrue;
	}
	else
	{
		//cleanup
		CloseWindow(m_hwndHTML);
		CloseWindow(m_hwnd);
		DestroyWindow(m_hwndHTML);
		DestroyWindow(m_hwnd);
		return wyFalse;
	}

}

void 
Announcements::Resize(HWND hwnd, wyBool isstart)
{
	RECT                rcmain, rcvsplitter;
	wyInt32             ret;
	wyInt32             hpos, vpos_ob, vpos, width, height, ob_height, ypos;
	MDIWindow			*wnd = (MDIWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	FrameWindowSplitter	*pcqueryvsplitter = wnd->GetVSplitter();

	VERIFY(GetClientRect(hwnd, &rcmain));
	VERIFY(GetWindowRect(pcqueryvsplitter->GetHwnd(), &rcvsplitter));
	VERIFY(MapWindowPoints(NULL, hwnd,(LPPOINT)&rcvsplitter, 2));

	ob_height		=	wnd->m_pcqueryobject->m_height;
	hpos			=	2;
	ypos			=	2;
	vpos_ob			=	(ob_height + 10) > 22 ? ob_height + 10: 22;
	width			=	rcvsplitter.left-2;
	height			=	rcmain.bottom - (2 * vpos_ob) - 10;
	m_htmlh			=   (wyInt32)(0.2 * height);
	vpos			=	(vpos_ob * 2)+ 8 + height - m_htmlh; 

	if( wnd->m_isanncreate == wyFalse )	
	{
		VERIFY(ret = MoveWindow(m_hwndHTML, 0, ypos, width, m_htmlh, TRUE));
		m_htmlw = width;
	}
	else
	{
		VERIFY(ret = MoveWindow(m_hwndHTML, m_htmlx, ypos, width, m_htmlh, TRUE));
		m_htmlw = width;
	}

	if(!(wnd->m_isanncreate || isstart))
		wnd->m_isanncreate = wyTrue;

	VERIFY(ret = MoveWindow(m_hwnd, hpos, vpos-ypos, width, m_htmlh+2, TRUE));

}

LRESULT	CALLBACK 
Announcements::AnnounceWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult;
	BOOL    bHandled;

	Announcements*  ann = (Announcements*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    lResult = HTMLayoutProcND(hwnd,message,wParam,lParam, &bHandled);
    
    if(bHandled)
    {
        return lResult;
    }

    switch(message)
    {	
    case WM_NCCREATE:
        ann = (Announcements*)((CREATESTRUCT *) lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) ann);
        break;
	//case WM_COMMAND:
	//	if(((HWND)lParam) && (HIWORD(wParam) == BN_CLICKED))
	//		i=1;
	//case WM_LBUTTONUP:
	//	i=1;

	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}


LRESULT	CALLBACK 
Announcements::AnnounceWndMainProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//LRESULT lResult;
	//BOOL    bHandled;
	RECT            rect;
    HDC             hdc;
    PAINTSTRUCT     ps;
    //TABCOLORINFO    ci = {0};
    HBRUSH          hbr;
    //wyWChar*        text;
    //wyInt32         len;
    //HFONT           hfont;
	//int i;
	//Announcements*  ann = (Announcements*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if(message == WM_PAINT)
    {	
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rect);
        SetBkMode(hdc, TRANSPARENT);
		//hbr = CreateSolidBrush(RGB(240, 0, 0));
		hbr = CreateSolidBrush(RGB(240, 240, 240));
        FillRect(hdc, &rect, hbr);
        DeleteBrush(hbr);
        EndPaint(hwnd, &ps);
        return 1;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}



//Function to handle a hyperlink click event. The close button is also a hyperlink.
//We notify the server with uuid of installation whenever such an event occurs.

BOOL
Announcements::HandleEvents(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params)
{
    HWND                    hwndform;
    htmlayout::dom::element src;
	CHttp		http;
	wyString	url_server;
	int status;
	const wchar_t *url = NULL;
	src = params.heTarget; 	
	MDIWindow *wnd = GetActiveWin();
    hwndform = src.get_element_hwnd(true);

	url_server.SetAs("http://www.webyog.com/notifications/");
	switch(params.cmd)
    {
        case HYPERLINK_CLICK:
			url = src.get_attribute("href");	
			if(wcsicmp(url, L"closeann") == 0)
			{
				url_server.AddSprintf("canceled?uuid=%s",pGlobals->m_appuuid.GetString());
				CloseWindow(wnd->m_announcements->m_hwndHTML);
				CloseWindow(wnd->m_announcements->m_hwnd);
				DestroyWindow(wnd->m_announcements->m_hwndHTML);
				DestroyWindow(wnd->m_announcements->m_hwnd);
				pGlobals->m_isannouncementopen = wyFalse;
				wnd->m_pcqueryobject->Resize();
			}
			else
			{
				url_server.AddSprintf("clicked?uuid=%s",pGlobals->m_appuuid.GetString());
				::ShellExecuteW(NULL,L"open", url, NULL,NULL,SW_SHOWNORMAL);
			}
			http.SetUrl(url_server.GetAsWideChar());
			http.SetContentType(L"text/xml");
			http.SendData("abc", 3, false, &status, false );
            return TRUE;
	}
		    
	return FALSE; 
}
