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
/*********************************************

Author: Vishal P.R

*********************************************/

#include <iostream>
#include <Windows.h>

#if _WIN32_WINNT >= 0x600
#include <VersionHelpers.h>
#endif

#include "CustomSaveDlg.h"
#include "Include.h"
#include "L10n.h"


wyBool CustomSaveDlg::m_vistastyle = wyFalse;

//parameterized cons
CustomSaveDlg::CustomSaveDlg(HWND hwndparent, wyWChar* message, wyWChar* caption, wyBool yestoall)
{
    m_hwndparent = hwndparent;
    m_yestoall   = yestoall;
    m_message.SetAs(message);
    m_caption.SetAs(caption);
    m_hfont = NULL;
}

//destructor
CustomSaveDlg::~CustomSaveDlg()
{
    DeleteObject(m_hfont);
}

//shwo the dialg box
wyInt32 
CustomSaveDlg::Create()
{
    HGLOBAL     hglobal;
	wyInt64     ret;
	wyInt32 fontsize;
    wyString    fontname;
    HDC         hdc;
	NONCLIENTMETRICS    ncm = {0};

    //alocate the memory for template
    hglobal = GlobalAlloc(GMEM_ZEROINIT, 1024);

    if (!hglobal)
        return -1;

	ncm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
	m_hfont = CreateFontIndirect(&ncm.lfMessageFont);
	hdc = GetDC(m_hwndparent);
	fontname.SetAs(ncm.lfMessageFont.lfFaceName);
    fontsize = -MulDiv(ncm.lfMessageFont.lfHeight, 72, GetDeviceCaps(hdc, LOGPIXELSY));
    ReleaseDC(m_hwndparent, hdc);

    //detect the os version and set the style
    SetOSStyle();

    //create the dialog template in the allocated memory
    CreateDlgTemplate(hglobal, fontname.GetString(), fontsize);

    //show the dialog
    ret = DialogBoxIndirectParam(GetModuleHandle(NULL), (LPDLGTEMPLATE)hglobal, 
                                 m_hwndparent, CustomSaveDlg::DlgProc, 
                                 (LPARAM)this);

    //free the space
    GlobalFree(hglobal);
    
    return ret;
}

//standard dialgo box procedure for custom save dialgog
INT_PTR CALLBACK
CustomSaveDlg::DlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    CustomSaveDlg*  pcustsave;
	HDC             hdc;

    switch (message)
	{
	    case WM_INITDIALOG:
            LocalizeWindow(hwnd);
            pcustsave = (CustomSaveDlg*)lparam;
			pcustsave->Initialize(hwnd);
            break;
		
		case WM_COMMAND:
            switch(LOWORD(wparam))
            {
                case IDYES:
                case IDNO:
                case IDCANCEL:
                case IDYESTOALL:
                case IDNOTOALL:
                    EndDialog(hwnd, LOWORD(wparam));
			        break;

                default:
                    return FALSE;
            }
		    break;

        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            break;

		case WM_CTLCOLORSTATIC:

            //if it is vista style, then only color the frame with the background brush
            if(m_vistastyle == wyTrue)
            {
                hdc = (HDC)wparam;
			    SetBkMode(hdc, TRANSPARENT);
			    return (BOOL)GetSysColorBrush(COLOR_WINDOW);
            }

            return FALSE;

        default:
            return FALSE;
	}

	return TRUE;
}

//initialize the dialog
void
CustomSaveDlg::Initialize(HWND hwnd)
{
    HICON   hicon;

    m_hwnd = hwnd;

    //calculate the maximum allowable dialog width
    InitDlgWidth();
    
    //show hide any buttons, and resize it accordingly
    AdjustInitialPosition();
        
    //resize the dialog to best fit its content
    ResizeDialog();

    //set message text and caption
    SetDlgItemText(hwnd, IDC_MESSAGE, m_message.GetAsWideChar());
    SetWindowText(hwnd, m_caption.GetAsWideChar());

    //position the dialog center to the monitor
    PositionCenterMonitior();

    //align the buttons
    AlignButtons();
    
    //set the icon
    hicon = LoadIcon(NULL, IDI_QUESTION);
    SendDlgItemMessage(hwnd, IDC_ICONBOX, STM_SETICON, (WPARAM)hicon, 0);
    DestroyIcon(hicon);

    //play the sound
    PlaySound(L"SystemQuestion", NULL, SND_ALIAS | SND_ASYNC | SND_NODEFAULT | SND_NOSTOP);
}

//position the dialog to the center of the monitor
void 
CustomSaveDlg::PositionCenterMonitior()
{
    MONITORINFO mi;
    RECT        rcmonitor;
    wyInt32     width, height;

    width = m_rcwnd.right - m_rcwnd.left;
    height = m_rcwnd.bottom - m_rcwnd.top;

    //get the monitor info and find the monitor rect
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(m_hmonitor, &mi);
    rcmonitor = mi.rcMonitor;

    //position the dialog to the center of dialog
    m_rcwnd.left    = rcmonitor.left + (rcmonitor.right  - rcmonitor.left - width) / 2;
    m_rcwnd.top     = rcmonitor.top  + (rcmonitor.bottom - rcmonitor.top - height) / 2;
    /*m_rcwnd.right   = m_rcwnd.left + width;
    m_rcwnd.bottom  = m_rcwnd.top  + height;*/
    SetWindowPos(m_hwnd, NULL, m_rcwnd.left, m_rcwnd.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

//center the buttons horizontally
void
CustomSaveDlg::AlignButtons()
{
    RECT    rcfirst, rclast, rcclient;
    wyInt32 widthbuttons, widthwnd, space, lastpos, i, diff;
    wyInt32 buttons[] = {IDYES, IDNO, IDCANCEL, IDYESTOALL, IDNOTOALL};

    lastpos = (m_yestoall == wyTrue) ? 4 : 2;

    //calculate the space between buttons
    GetWindowRect(GetDlgItem(m_hwnd, IDYES), &rcfirst);
    GetWindowRect(GetDlgItem(m_hwnd, IDNO), &rclast);
    diff = rclast.left - rcfirst.right;

    //get the last button rect
    GetWindowRect(GetDlgItem(m_hwnd, buttons[lastpos]), &rclast);

    MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcfirst, 2);
    MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rclast, 2);

    //get the client rect of dialog and calculate the space between buttons
    GetClientRect(m_hwnd, &rcclient);
    widthbuttons = rclast.right - rcfirst.left;
    widthwnd = rcclient.right - rcclient.left;

    if(m_vistastyle == wyFalse)
        space = (widthwnd - widthbuttons) / 2;
    else
        space = (widthwnd - m_buttonrightpadding - widthbuttons);

    rclast.right = space - diff;

    //align all the buttons
    for(i = 0; i <= lastpos; ++i)
    {
        GetWindowRect(GetDlgItem(m_hwnd, buttons[i]), &rcfirst);
        MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcfirst, 2);
        
        SetWindowPos(GetDlgItem(m_hwnd, buttons[i]), NULL, rclast.right + diff, rcfirst.top, 
                     rcfirst.right - rcfirst.left, rcfirst.bottom - rcfirst.top, 
                     SWP_NOZORDER);

        GetWindowRect(GetDlgItem(m_hwnd, buttons[i]), &rclast);
        MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rclast, 2);
    }
}

//adjust the initial position
void
CustomSaveDlg::AdjustInitialPosition()
{
    wyInt32 buttons[] = {IDYES, IDNO, IDCANCEL, IDYESTOALL, IDNOTOALL};
    wyInt32 lastpos = 4, width, space, textwidth, messagepadding;
    RECT    rctemp, rcframe, rcclient;

    //Set the control fonts and calculate the new window rect
    SetControlFonts();
    GetWindowRect(m_hwnd, &m_rcwnd);

    //get the window rect of frame and button
	GetWindowRect(GetDlgItem(m_hwnd, IDC_FRAME), &rcframe);
    GetWindowRect(GetDlgItem(m_hwnd, IDNOTOALL), &rctemp);

    //find the various paddings
    space = m_rcwnd.right - rctemp.right;
	m_buttontoppadding = rctemp.top - rcframe.bottom; 
    
    MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctemp, 2);
    GetClientRect(m_hwnd, &rcclient);
    m_buttonrightpadding = rcclient.right - rctemp.right;

    //show/hide Yes To All and No To All based on the flag
    if(m_yestoall == wyFalse)
    {
        ShowWindow(GetDlgItem(m_hwnd, IDYESTOALL), SW_HIDE);
        ShowWindow(GetDlgItem(m_hwnd, IDNOTOALL), SW_HIDE);
        lastpos = 2;
    }
    else
    {
        //if the flag is set, means we have nothing to modify, so simply return
        return;
    }

    //calculate the width
    GetWindowRect(GetDlgItem(m_hwnd, buttons[lastpos]), &rctemp);
    width = (rctemp.right + space) - m_rcwnd.left;

    //get the message rect and find the right padding
	GetWindowRect(GetDlgItem(m_hwnd, IDC_MESSAGE), &rctemp);
	messagepadding = m_rcwnd.right - rctemp.right;

    //resize the window and reclculate the window rect
	SetWindowPos(m_hwnd, NULL, 0, 0, width, m_rcwnd.bottom - m_rcwnd.top, SWP_NOMOVE | SWP_NOZORDER);
    GetWindowRect(m_hwnd, &m_rcwnd);

    //find the width allowed for message
    textwidth = width - (rctemp.left - m_rcwnd.left) - messagepadding;

    MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctemp, 2);

    //resize the message
    SetWindowPos(GetDlgItem(m_hwnd, IDC_MESSAGE), NULL, 0, 0, textwidth, 
                 rctemp.bottom - rctemp.top, SWP_NOMOVE | SWP_NOZORDER);
}

//resize the dialog to best fit its contents
void
CustomSaveDlg::ResizeDialog()
{
    wyInt32         width, textwidth, lastpos, i, diff, height;
    wyInt32         buttons[] = {IDYES, IDNO, IDCANCEL, IDYESTOALL, IDNOTOALL};
	wyInt32	        leftpadding, rightpadding;
    RECT            rctext, rcbutton, rctemp, rcframe;
    HDC             hdc;
    TITLEBARINFO    tbinfo;
	HFONT			hfont;

    //get the rect of message and button controls
	GetWindowRect(GetDlgItem(m_hwnd, IDC_MESSAGE), &rctext);
	GetWindowRect(GetDlgItem(m_hwnd, IDYES), &rcbutton);

    //calculate the paddings
    height = m_rcwnd.bottom - rcbutton.bottom;
	leftpadding = rctext.left - m_rcwnd.left;
	rightpadding = m_rcwnd.right - rctext.right;

    //calculate the padding between message and buttons
    diff = rcbutton.top - rctext.bottom;

    MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctext, 2);
	rctemp = rctext;

    //get the device handle and select the current font into it
	hdc = GetDC(m_hwnd);
	hfont = (HFONT)SelectObject(hdc, m_hfont);

    //calculate the rect for message
	DrawText(hdc, m_message.GetAsWideChar(), -1, &rctext, DT_CALCRECT);

    //calculate message width and dialog width
	textwidth = rctext.right - rctext.left;
	width = textwidth + leftpadding + rightpadding;

    //resize the dialog to limit the width to maximum allowable width
	if(width > m_maxdlgwidth)
	{
        //recalculate the message width and modify the rect for the message
		textwidth = m_maxdlgwidth - (leftpadding + rightpadding);
		rctext.right = rctext.left + textwidth;
		rctemp = rctext;

        //recalculate the rect for message
		DrawText(hdc, m_message.GetAsWideChar(), -1, &rctext, DT_CALCRECT | DT_WORDBREAK);

        //wrap the message if the calculated rect is greater than max allowable rect for message
		if(rctext.right - rctext.left > textwidth)
			rctext = WrapText(hdc, rctemp);

        //recalculate the dialog width
		width = rctext.right - rctext.left + leftpadding + rightpadding;
	}

	SelectObject(hdc, hfont);
	ReleaseDC(m_hwnd, hdc);

    //resize the message rect
    SetWindowPos(GetDlgItem(m_hwnd, IDC_MESSAGE), NULL, 0, 0, textwidth, 
                 rctext.bottom - rctext.top, SWP_NOMOVE | SWP_NOZORDER);

    //adjust the width if the recalculated width is less than dialog width
	if(width < (m_rcwnd.right - m_rcwnd.left))
		width = m_rcwnd.right - m_rcwnd.left;

    //get the frame rect
	GetWindowRect(GetDlgItem(m_hwnd, IDC_FRAME), &rcframe);
	MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcframe, 2);
	
    //resize the frame
	if(rcframe.bottom - rctext.bottom < height)
		SetWindowPos(GetDlgItem(m_hwnd, IDC_FRAME), NULL, 0, 0, width, 
                     rctext.bottom + rctext.top, SWP_NOMOVE | SWP_NOZORDER);
    else
        SetWindowPos(GetDlgItem(m_hwnd, IDC_FRAME), NULL, 0, 0, width, 
                     rcframe.bottom - rcframe.top, SWP_NOMOVE | SWP_NOZORDER);

    //update the frame rect
    GetWindowRect(GetDlgItem(m_hwnd, IDC_FRAME), &rcframe);
	MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcframe, 2);
    
    lastpos = (m_yestoall == wyTrue) ? 4 : 2;

    //update the button positions
    for(i = 0; i <= lastpos; ++i)
    {
        GetWindowRect(GetDlgItem(m_hwnd, buttons[i]), &rcbutton);
        MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcbutton, 2);
        
		SetWindowPos(GetDlgItem(m_hwnd, buttons[i]), NULL, rcbutton.left, 
                     rcframe.bottom + m_buttontoppadding, 0, 0, 
                     SWP_NOSIZE | SWP_NOZORDER);
    }

    //calculate the dialog height
	tbinfo.cbSize = sizeof(TITLEBARINFO);
	GetTitleBarInfo(m_hwnd, &tbinfo);
	GetWindowRect(GetDlgItem(m_hwnd, IDYES), &rcbutton);
	MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcbutton, 2);
	height += rcbutton.bottom + (tbinfo.rcTitleBar.bottom - tbinfo.rcTitleBar.top);

    //resize the dialog
	SetWindowPos(m_hwnd, NULL, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE);  
	
    //save the dialog rect for further calculations
    GetWindowRect(m_hwnd, &m_rcwnd);
}

//inti the maximum allowable dialog width
void
CustomSaveDlg::InitDlgWidth()
{
    MONITORINFO mi;
    wyInt32     dlgwidth;

    //get the dialog rect amd calculate the width
    GetWindowRect(m_hwnd, &m_rcwnd);
    dlgwidth = m_rcwnd.right - m_rcwnd.left;

    //find the monitor which is nearest to the parent window
    m_hmonitor = MonitorFromWindow(m_hwndparent, MONITOR_DEFAULTTONEAREST); 

    //get the monitor infos
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(m_hmonitor, &mi);

    //calculate the maximum allowable dialog width
    if(m_vistastyle == wyFalse)
        m_maxdlgwidth = (mi.rcMonitor.right - mi.rcMonitor.left) * 60 / 100;
    else
        m_maxdlgwidth = (mi.rcMonitor.right - mi.rcMonitor.left) * 40 / 100;

    //dont allow the dialog to shrink less than the req wisth
    if(m_maxdlgwidth < dlgwidth)
        m_maxdlgwidth = dlgwidth;
}

//set the control fonts
void
CustomSaveDlg::SetControlFonts()
{
    wyInt32 controls[] = {IDC_FRAME, IDC_MESSAGE, IDYES, IDNO, IDCANCEL, IDYESTOALL, IDNOTOALL};
    wyInt32 i;
    
    //get the GUI font and set it as the font for dialog
    //m_hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    //SendMessage(m_hwnd, WM_SETFONT, (WPARAM)m_hfont, TRUE);

    //set the font for each control
    for(i = 0; i < sizeof(controls); ++i)
        SendMessage(GetDlgItem(m_hwnd, controls[i]), WM_SETFONT, (WPARAM)m_hfont, TRUE);
}

//wrap the text to best fit the message rect
RECT
CustomSaveDlg::WrapText(HDC hdc, RECT rctext)
{
	wyString    tempbuff, finalstring;
	wyChar      temp[] = {0, 0};
	wyInt32	    textwidth;
	const       wyChar*	charptr;

    //calculate the text width
	textwidth = rctext.right - rctext.left;

	for(charptr = m_message.GetString(); *charptr; ++charptr)
	{
        temp[0] = *charptr;
		tempbuff.Add(temp);

        //wrap the text
		if(*charptr == '\n' || *charptr == ' ')
            WrapTextHelper(tempbuff, finalstring, hdc, textwidth);
	}

    //wrap any remaining text
    WrapTextHelper(tempbuff, finalstring, hdc, textwidth);

    //asign the modified message
	m_message.SetAs(finalstring);

    //recalculate the message rect
	DrawText(hdc, m_message.GetAsWideChar(), -1, &rctext, DT_CALCRECT | DT_WORDBREAK);
	return rctext;
}

//helper function to wrap the text
void
CustomSaveDlg::WrapTextHelper(wyString& tempbuff, wyString& finalstring, HDC hdc, wyInt32 textwidth)
{
    wyInt32 totcount;
    SIZE    size;

    while(tempbuff.GetLength())
	{
        //get the maximum charecters that can be fit in a given space
		GetTextExtentExPoint(hdc, tempbuff.GetAsWideChar(), 
                             tempbuff.GetLength() + 1, textwidth, &totcount, 
                             NULL, &size);
		
        //if the number of charecters is less than the buffer length, then break it
        if(totcount < tempbuff.GetLength())
		{
			finalstring.AddSprintf("%s\r\n", tempbuff.Substr(0, totcount));
			tempbuff.Erase(0, totcount);
		}
		else
		{
			finalstring.Add(tempbuff.GetString());
            tempbuff.Clear();
		}
	}
}

//Aligns the pointer to the boundary given
LPWORD
CustomSaveDlg::AlignMemory(LPWORD  ptr, ULONG align)
{
    unsigned long long n, align_cpy;
	// Crash with windows 10 system for 64 bit -- Aadarsh
	align_cpy = (unsigned long long)align;
    
	n = (unsigned long long)ptr;
    n += align_cpy - 1;
    n &= ~(align_cpy - 1);
    return (LPWORD)n;
}

void
CustomSaveDlg::CreateDlgTemplate(HGLOBAL hglobal, const wyChar* fontname, wyInt32 fontsize)
{
    LPDLGTEMPLATE   lpdlg;
    LPWORD          lpword;
    LPWSTR          lpstr;
    wyInt64*        dialogdef;

    //dialog template for XP
    wyInt64 dialogdefxp[] = {
        DS_SETFONT | DS_MODALFRAME | DS_FIXEDSYS | WS_POPUP | WS_CAPTION | WS_SYSMENU,
        0, 0, 284, 61
    };

    //dialog template for vista
    wyInt64 dialogdefvista[] = {
        DS_SETFONT | DS_MODALFRAME | DS_FIXEDSYS | WS_POPUP | WS_CAPTION | WS_SYSMENU,
        0, 0, 307, 74
    };

    //get the dialog template based on the style detected
    dialogdef = (m_vistastyle == wyFalse) ? dialogdefxp : dialogdefvista;

    //get the allocated memory and cast it to dialog template
    lpdlg = (LPDLGTEMPLATE)GlobalLock(hglobal);

    //dialog header
    lpdlg->style    = dialogdef[0];
    lpdlg->x        = dialogdef[1];  
    lpdlg->y        = dialogdef[2];
    lpdlg->cx       = dialogdef[3];; 
    lpdlg->cy       = dialogdef[4];
    lpdlg->cdit     = 8;

    lpword = (LPWORD)(lpdlg + 1);
    *lpword++ = 0;
    *lpword++ = 0;

    //set the caption
    lpstr = (LPWSTR)lpword;
    lpword += MultiByteToWideChar(CP_ACP, 0, "Caption", -1, lpstr, 50);

    //set the font size
    *lpword++ = fontsize;

    //set the font name
    lpstr = (LPWSTR)lpword;
    lpword += MultiByteToWideChar(CP_ACP, 0, fontname, -1, lpstr, 50);

    //add the child control templates
    AddControls(lpword);

    GlobalUnlock(hglobal);
}

LPWORD
CustomSaveDlg::AddControls(LPWORD lpword)
{
    wyInt32             i;
    LPDLGITEMTEMPLATE   lpdlgitem;
    LPWSTR              lpstr;
    wyInt32*            controlsdef;
    
    //control captions
    wyChar *caption[8] = {"&Yes", "&No", "Cance&l", "Ye&s to All", "N&o to All", "", "", "Message"};

    //control template for xp
    wyInt32 controlsdefxp[8][6] = {
        {IDYES, WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP, 7, 38, 50, 14},
        {IDNO, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 62, 38, 50, 14},
        {IDCANCEL,WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 117, 38, 50, 14},
        {IDYESTOALL, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 172, 38, 50, 14},
        {IDNOTOALL, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 227, 38, 50, 14},
        {IDC_FRAME, WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, 283, 32},
        {IDC_ICONBOX, WS_CHILD | WS_VISIBLE | SS_ICON, 7, 7, 21, 20},
        {IDC_MESSAGE, WS_CHILD | WS_VISIBLE | SS_LEFT, 37, 7, 240, 8}
    };

    //control template for vista
    wyInt32 controlsdefvista[8][6] = {
        {IDYES, WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP, 28,52,50,14},
        {IDNO, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 84,52,50,14},
        {IDCANCEL,WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 139,52,50,14},
        {IDYESTOALL, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 195,52,50,14},
        {IDNOTOALL, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 251,52,50,14},
        {IDC_FRAME, WS_CHILD | WS_VISIBLE | SS_LEFT, 0,0,308,46},
        {IDC_ICONBOX, WS_CHILD | WS_VISIBLE | SS_ICON, 15,15,20,20},
        {IDC_MESSAGE, WS_CHILD | WS_VISIBLE | SS_LEFT, 39,15,244,8}
    };

    //select the control def based on the style detected
    controlsdef = (m_vistastyle == wyFalse) ? &controlsdefxp[0][0] : &controlsdefvista[0][0];

    for(i = 0; i < 8; i++)
    {
        //every LPDLGITEMTEMPLATE should be DWORD aligned
        lpword = AlignMemory(lpword);

        //get the next DWORD and cast it to dialog item template
        lpdlgitem = (LPDLGITEMTEMPLATE)lpword;
        
        //control header
        lpdlgitem->id       = *(controlsdef + i * 6);
        lpdlgitem->style    = *(controlsdef + i * 6 + 1);
        lpdlgitem->x        = *(controlsdef + i * 6 + 2); 
        lpdlgitem->y        = *(controlsdef + i * 6 + 3);
        lpdlgitem->cx       = *(controlsdef + i * 6 + 4);
        lpdlgitem->cy       = *(controlsdef + i * 6 + 5);
        
        lpword = (LPWORD)(lpdlgitem + 1);

        //set the class to one of the default
        *lpword++ = 0xFFFF;

        if(i < 5)
        {
            //control class for buttons
            *lpword++ = 0x0080;
        }
        else
        {
            //control class for static controls
            *lpword++ = 0x0082;
        }

        //set the caption
        lpstr = (LPWSTR)lpword;
        lpword += MultiByteToWideChar(CP_ACP, 0, caption[i], -1, lpstr, 50);
        *lpword++ = 0;       
    }

    return lpword;
}

//function sets the display style
void
CustomSaveDlg::SetOSStyle()
{
#if _WIN32_WINNT < 0x600

    OSVERSIONINFO osvi;

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    //get the OS version and set the display style accordingly
    if(GetVersionEx(&osvi))
        m_vistastyle = (osvi.dwMajorVersion >= 6) ? wyTrue : wyFalse;

#else
	m_vistastyle = IsWindowsVistaOrGreater() ? wyTrue : wyFalse;
#endif
}

//API to invoke Custom Save Message
wyInt32
CustomSaveMessageBox(HWND hwnd, wyWChar* message, wyWChar* caption, wyBool yestoall)
{
    CustomSaveDlg cust(hwnd, message, caption, yestoall);
    return cust.Create();
}
