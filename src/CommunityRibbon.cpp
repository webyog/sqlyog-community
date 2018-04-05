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

#include "CommunityRibbon.h"
#include "Global.h"
#include "FrameWindowHelper.h"

#define ADTEXTAPPEND	_("Reason #%d to upgrade")
#define ADTEXTDELAY		60000 * 2 // (in milli seconds)1minute

extern	PGLOBALS		pGlobals;

CommunityRibbon::CommunityRibbon()
{	
	m_thread	= NULL;
	m_hwnd		= NULL;
	m_hfont		= NULL;
	m_wndproc	= NULL;
}

CommunityRibbon::~CommunityRibbon()
{
	wyInt32 ret;
	DWORD	thrid;

	if(m_thread)
	{
		ret = WaitForSingleObject(m_thread, 0);

		if(ret == WAIT_TIMEOUT)
		{
			GetExitCodeThread(m_thread, &thrid);
			TerminateThread(m_thread, thrid);
		}

		CloseHandle(m_thread);
	}

	if(m_hfont)
		DeleteFont(m_hfont);
}

wyBool
CommunityRibbon::CreateRibbon(HWND hwndparent)
{
	HDC	        hdc; 
	wyInt32     fontheight;
	wyWChar		adtext[SIZE_512]={0};
	wyInt32		randomindex;
	wyString	comadtext, tempstr;

	// seed for random number generator, need to be called only once.
	srand(GetTickCount());

	// take caption randomly for showing first also
	randomindex = CommunityRibbon::GetRandomIndex();

	VERIFY(LoadString(pGlobals->m_hinstance, randomindex, adtext, SIZE_512-1));
	tempstr.SetAs(_(adtext));
    comadtext.Sprintf(" %s : ", tempstr.GetString());
    comadtext.AddSprintf(ADTEXTAPPEND, randomindex % 100);

	
	VERIFY(m_hwnd = CreateWindow(L"STATIC", comadtext.GetAsWideChar(), 
									WS_CHILD | WS_VISIBLE | SS_WORDELLIPSIS | SS_NOTIFY, //SS_NOTIFY Sends the parent window STN_CLICKED
									0, 0, 0, 0,
							        hwndparent, (HMENU)IDC_COMMTITLE,
									pGlobals->m_pcmainwin->GetHinstance(), this));
		        
	if(!m_hwnd)
		return wyFalse;

	m_wndproc = (WNDPROC)SetWindowLongPtr(m_hwnd, GWLP_WNDPROC,(LONG_PTR)CommunityRibbon::WndProc);	
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

	//Sets the font
	VERIFY(hdc = GetDC(m_hwnd));
	fontheight = -MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72);

	VERIFY(m_hfont = CreateFont(fontheight, 0, 0, 0,
		        FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, L"Verdana"));
	
	ReleaseDC(m_hwnd, hdc);

	::SendMessage(m_hwnd, WM_SETFONT, (WPARAM)m_hfont, TRUE);

	return wyTrue;
}

LRESULT CALLBACK
CommunityRibbon::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    CommunityRibbon *cribbon = (CommunityRibbon*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
		case WM_MOUSEMOVE:
			SetCursor(LoadCursor(NULL, IDC_HAND)); 
			pGlobals->m_pcmainwin->AddTextInStatusBar(L"http://www.webyog.com");
			return 0;
	}

	return CallWindowProc(cribbon->m_wndproc, hwnd, message, wparam, lparam);	
}

void
CommunityRibbon::HandleCommunityHeader()
{
	COMTYHDR    evt;
	wyUInt32	thdid;

	evt.m_comribbon = this;
	
	VERIFY(m_thread = (HANDLE)_beginthreadex(NULL, 0, CommunityRibbon::HandleChangeTabText, &evt, 0, &thdid));	
}

unsigned __stdcall		
CommunityRibbon::HandleChangeTabText(LPVOID param)
{
	COMTYHDR *comhdr = (COMTYHDR*)param;

	comhdr->m_comribbon->EnumConWindows(comhdr);

	return 1;
}

void
CommunityRibbon::EnumConWindows(COMTYHDR *comhdr)
{
	DWORD tswait = ADTEXTDELAY;

	while(1)
	{
		if(GetActiveWin())
			EnumChildWindows(pGlobals->m_hwndclient, CommunityRibbon::EnumChildProc, NULL);	

		Sleep(tswait);
	}
}

BOOL CALLBACK
CommunityRibbon::EnumChildProc(HWND hwnd, LPARAM lParam)
{
	MDIWindow*		wnd;
	wyWChar  		classname[SIZE_128]={0};
	wyWChar			adtext[SIZE_512]={0};
	wyInt32			randomindex;
	wyString		comadtext, tempstr;

	VERIFY(GetClassName(hwnd, classname, SIZE_128-1));

	if((wcscmp(classname, QUERY_WINDOW_CLASS_NAME_STR)== 0))
	{
		VERIFY(wnd =(MDIWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if(!wnd)
			return FALSE;

		randomindex = CommunityRibbon::GetRandomIndex();

		VERIFY(LoadString(pGlobals->m_hinstance, randomindex, adtext, SIZE_512-1));
        tempstr.SetAs(_(adtext));
        comadtext.Sprintf(" %s : ", tempstr.GetString());
        comadtext.AddSprintf(ADTEXTAPPEND, randomindex % 100);

		SetWindowText(wnd->m_pctabmodule->m_hwndcommtytitle, comadtext.GetAsWideChar());
	}

	return TRUE;
}

wyInt32
CommunityRibbon::GetRandomIndex()
{
	wyInt32 min = IDS_COMMUNITY_AD_1;
	wyInt32 max = IDS_COMMUNITY_AD_75;
	wyInt32 range = (max - min) + 1;
	wyInt32 somerandomnum;

	//now generate some random numbers
	somerandomnum = min + (wyInt32)(range * rand() / (RAND_MAX + 1.0));

	return somerandomnum;
}
 
