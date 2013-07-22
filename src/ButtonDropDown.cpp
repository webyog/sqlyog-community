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

#include "ButtonDropDown.h"

//constructor
ButtonDropDown::ButtonDropDown(HWND hwnd)
{
    m_hwnd = hwnd;
    m_hwndlistview = NULL;
    m_hwndbutton = NULL;    
    m_style = 0;
    m_checkstate = BST_UNCHECKED;
    m_isautomated = wyFalse;
    m_iconwidth = 0;
    m_origparentdata = 0;
    m_origparentproc = NULL;
}

//destructor
ButtonDropDown::~ButtonDropDown()
{
}

//the list view window procedure
LRESULT CALLBACK ButtonDropDown::ListViewWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    ButtonDropDown*     pbd = (ButtonDropDown*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    POINT               pt;
    RECT                rect = {0};
    WNDPROC             wndproc;
    NMBUTTONDROPDOWN    nmbd = {0};
    wyInt32             i, count;

    switch(message)
    {
        //handle the the button down event 
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
            GetWindowRect(hwnd, &rect);

            //get the postion
            pt.x = LOWORD(lparam);
            pt.y = HIWORD(lparam);
            MapWindowPoints(hwnd, NULL, &pt, 1);

            //automattically close the drop down and reset the button check state if the event happened outside the window bounds
            if(!PtInRect(&rect, pt))
            {
                ReleaseCapture();
                ShowWindow(hwnd, SW_HIDE);
                pbd->m_checkstate = BST_UNCHECKED;
                SendMessage(pbd->m_hwndbutton, BM_SETCHECK, (WPARAM)pbd->m_checkstate, 0);
                //SetWindowLongPtr(GetParent(pbd->m_hwnd), GWLP_WNDPROC, (LONG)pbd->m_origparentproc);
                //SetWindowLongPtr(GetParent(pbd->m_hwnd), GWLP_USERDATA, pbd->m_origparentdata);
                return 1;
            }

            //if it is happened inside the window bounds, we are performing a hit test
            if((wndproc = (WNDPROC)GetWindowLongPtr(GetParent(pbd->m_hwnd), GWLP_WNDPROC)))
            {
                //form the notification structure
                MapWindowPoints(NULL, hwnd, &pt, 1);
                nmbd.info.pt = pt;
                ListView_HitTest(hwnd, &nmbd.info);
                nmbd.hdr.code = BDN_BEGINHIT;
                nmbd.hdr.hwndFrom = hwnd;
                nmbd.message = message;

                //Send notification to parent window(ButtonDropDown control) which will then forward to its parent window
                CallWindowProc(wndproc, GetParent(pbd->m_hwnd), WM_NOTIFY, (WPARAM)nmbd.hdr.idFrom, (LPARAM)&nmbd);
                
                //if this value is set, then return from processing it further
                if(nmbd.retvalue)
                {
                    return 1;
                }
            }
            break;

        //same as button down event handling, but is used to send the notification only
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            GetCursorPos(&pt);
            
            if((wndproc = (WNDPROC)GetWindowLongPtr(GetParent(pbd->m_hwnd), GWLP_WNDPROC)))
            {
                //form the structure and send a BDN_ENDHIT notification
                MapWindowPoints(NULL, hwnd, &pt, 1);
                nmbd.info.pt = pt;
                ListView_HitTest(hwnd, &nmbd.info);
                nmbd.hdr.code = BDN_ENDHIT;
                nmbd.hdr.hwndFrom = hwnd;
                nmbd.message = message;
                CallWindowProc(wndproc, GetParent(pbd->m_hwnd), WM_NOTIFY, (WPARAM)nmbd.hdr.idFrom, (LPARAM)&nmbd);

                if(nmbd.retvalue)
                {
                    return 1;
                }
            }
            break;

        case WM_ACTIVATE:
            //if the window is deactivated, then close the drop down and reset the button check state
            if(wparam == WA_INACTIVE)
            {
                ReleaseCapture();
                ShowWindow(hwnd, SW_HIDE);
                pbd->m_checkstate = BST_UNCHECKED;
                SendMessage(pbd->m_hwndbutton, BM_SETCHECK, (WPARAM)pbd->m_checkstate, 0);
                //SetWindowLongPtr(GetParent(pbd->m_hwnd), GWLP_WNDPROC, (LONG)pbd->m_origparentproc);
                //SetWindowLongPtr(GetParent(pbd->m_hwnd), GWLP_USERDATA, pbd->m_origparentdata);
                return 1;
            }
            break;

        //Handle the key down event just like we handled the mouse down events
        case WM_KEYDOWN:
            //close the dropdown and reset the button check state
            if(wparam == VK_ESCAPE)
            {
                ReleaseCapture();
                ShowWindow(hwnd, SW_HIDE);
                pbd->m_checkstate = BST_UNCHECKED;
                SendMessage(pbd->m_hwndbutton, BM_SETCHECK, (WPARAM)pbd->m_checkstate, 0);
                //SetWindowLongPtr(GetParent(pbd->m_hwnd), GWLP_WNDPROC, (LONG)pbd->m_origparentproc);
                //SetWindowLongPtr(GetParent(pbd->m_hwnd), GWLP_USERDATA, pbd->m_origparentdata);
                return 1;
            }
            //if it is a space key, then we are forming a notfication structure simillar to the one formed mouse down event
            else if(wparam == VK_SPACE)
            {
                if((wndproc = (WNDPROC)GetWindowLongPtr(GetParent(pbd->m_hwnd), GWLP_WNDPROC)))
                {
                    nmbd.info.flags = LVHT_ONITEMSTATEICON;

                    //loop through and find the selected item
                    for(i = 0, count = ListView_GetItemCount(hwnd); i < count; ++i)
                    {
                        if(ListView_GetItemState(hwnd, i, LVIS_SELECTED) == LVIS_SELECTED)
                        {
                            nmbd.info.iItem = i;
                            nmbd.hdr.code = BDN_BEGINHIT;
                            nmbd.hdr.hwndFrom = hwnd;
                            nmbd.message = message;

                            //send the notification to the control
                            CallWindowProc(wndproc, GetParent(pbd->m_hwnd), WM_NOTIFY, (WPARAM)nmbd.hdr.idFrom, (LPARAM)&nmbd);
                
                            if(nmbd.retvalue)
                            {
                                return 1;
                            }

                            break;
                        }
                    }
                }
            }
            break;
            
        case WM_KEYUP:
            //same as space down handling, but here we are changing the notification to BDN_ENDHIT
            if(wparam == VK_SPACE)
            {
                if((wndproc = (WNDPROC)GetWindowLongPtr(GetParent(pbd->m_hwnd), GWLP_WNDPROC)))
                {
                    nmbd.info.flags = LVHT_ONITEMSTATEICON;

                    for(i = 0, count = ListView_GetItemCount(hwnd); i < count; ++i)
                    {
                        if(ListView_GetItemState(hwnd, i, LVIS_SELECTED) == LVIS_SELECTED)
                        {
                            nmbd.info.iItem = i;
                            nmbd.hdr.code = BDN_ENDHIT;
                            nmbd.hdr.hwndFrom = hwnd;
                            nmbd.message = message;
                            CallWindowProc(wndproc, GetParent(pbd->m_hwnd), WM_NOTIFY, (WPARAM)nmbd.hdr.idFrom, (LPARAM)&nmbd);

                            if(nmbd.retvalue)
                            {
                                return 1;
                            }

                            break;
                        }
                    }
                }
            }
            break;
    }

    //call the original window procedure
    return CallWindowProc(pbd->m_listvieworigproc, hwnd, message, wparam, lparam);
}

//window procedure for the ButtonDropDown control
LRESULT CALLBACK ButtonDropDown::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    ButtonDropDown* pbd = (ButtonDropDown*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    wyBool          checkstate;

    switch(message)
    {
        //create a new ButtonDropDown object and associate it with the window
        case WM_NCCREATE:
            pbd = new ButtonDropDown(hwnd);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pbd);
            break;

        //Create the child controls, i.e a list view and button control
        case WM_CREATE:
            pbd->CreateCtrls();
            break;

        //set the text of the button so that it will look like the text of the window itself
        case WM_SETTEXT:
            SetWindowText(pbd->m_hwndbutton, (LPCWSTR)lparam);
            return 1;

        case WM_COMMAND:
            //if the command is from the button, then set the check state of the button and display the list view
            if((HWND)lparam == pbd->m_hwndbutton)
            {
                SendMessage(pbd->m_hwndbutton, BM_SETCHECK, (WPARAM)(pbd->m_checkstate = !pbd->m_checkstate), 0);
                checkstate = SendMessage(pbd->m_hwndbutton, BM_GETCHECK, 0, 0) ? wyTrue : wyFalse;
                pbd->OnButtonCheck(checkstate);
                return 1;
            }

        case WM_DESTROY:
            //on window destruction, destroy the list view and button
            if(pbd->m_hwndlistview)
            {
                DestroyWindow(pbd->m_hwndlistview);
            }

            if(pbd->m_hwndbutton)
            {
                DestroyWindow(pbd->m_hwndbutton);
            }
            break;

        case WM_NCDESTROY:
            //delete the object
            delete pbd;
            break;

        //get the button handle
        case BDM_GETBUTTONHANDLE:
            return (LRESULT)pbd->m_hwndbutton;

        //get the list view handle
        case BDM_GETLISTHANDLE:
            return (LRESULT)pbd->m_hwndlistview;

        //set the drop style fo the control
        case BDM_SETDROPSTYLE:
            pbd->m_style = wparam;
            return 1;

        case WM_SETFOCUS:
            if(pbd)
            {
                SetFocus(pbd->m_hwndbutton);
            }
            return 1;

        case WM_NOTIFY:
            //forward the notification to parent window
            SendMessage(GetParent(hwnd), message, wparam, lparam);
    }

    return DefWindowProc(hwnd, message, wparam, lparam);
}

//function creates the controls
wyBool 
ButtonDropDown::CreateCtrls()
{
    RECT        rect = {0};
    wyWChar*    buffer = NULL;
    wyInt32     size;
    HFONT       hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    //create the list view control
    m_hwndlistview = CreateWindowEx(WS_EX_WINDOWEDGE,
        WC_LISTVIEW, 
        NULL, 
        LVS_LIST | LVS_SINGLESEL | LVS_ALIGNLEFT | LVS_NOCOLUMNHEADER | WS_TABSTOP | WS_POPUP | WS_BORDER,
        0,0,500,100, 
        m_hwnd, 
        (HMENU)0,
        GetModuleHandle(0), 
        NULL);
    SendMessage(m_hwndlistview, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);
    SetWindowLongPtr(m_hwndlistview, GWLP_USERDATA, (LONG_PTR)this);
    m_listvieworigproc = (WNDPROC)SetWindowLongPtr(m_hwndlistview, GWLP_WNDPROC, (LONG_PTR)ListViewWndProc);
    size = GetWindowTextLength(m_hwnd);

    if(size)
    {
        buffer = new wyWChar[size + 2];
        GetWindowText(m_hwnd, buffer, size + 1);
    }

    //create the button with the window caption
    GetWindowRect(m_hwnd, &rect);
    m_hwndbutton = CreateWindowEx(NULL, L"button", buffer, WS_CHILD | BS_CHECKBOX | BS_PUSHLIKE, 
        0, 0, 
        rect.right - rect.left, 
        rect.bottom - rect.top,
        m_hwnd, (HMENU)0, GetModuleHandle(0), 0);
    SendMessage(m_hwndbutton, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);
    SetWindowLongPtr(m_hwndbutton, GWLP_USERDATA, (LONG_PTR)this);

    //make the button visible
    ShowWindow(m_hwndbutton, SW_SHOW);
    delete[] buffer;
    return wyTrue;
}

//function called when button check state is changed and shows the list view
void
ButtonDropDown::OnButtonCheck(wyBool checkstate)
{
    RECT        rect = {0}, rectlistview = {0};
    wyInt32     width, height;
    HMONITOR    hmonitor;
    MONITORINFO mi = {0};

    //m_origparentproc = (WNDPROC)SetWindowLongPtr(GetParent(m_hwnd), GWLP_WNDPROC, (LONG)ParentWndProc);
    //m_origparentdata = SetWindowLongPtr(GetParent(m_hwnd), GWLP_USERDATA, (LONG)this);
    GetWindowRect(m_hwnd, &rect);

    //get the list view width
    width = SetListViewWidth(&rect);

    GetWindowRect(m_hwndlistview, &rectlistview);
    height = rectlistview.bottom - rectlistview.top;

    //set the drop position based on the style
    if(m_style == BDS_DOWN)
    {
        SetWindowPos(m_hwndlistview, NULL, rect.left, rect.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
    else if(m_style == BDS_UP)
    {
        SetWindowPos(m_hwndlistview, NULL, rect.left, rect.top - height, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
    else
    {
        //if the drop style is auto, we are calculating the best possible drop position
        hmonitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
        mi.cbSize = sizeof(mi);
        GetMonitorInfo(hmonitor, &mi);

        if(rect.bottom + height > mi.rcWork.bottom)
        {
            SetWindowPos(m_hwndlistview, NULL, rect.left, rect.top - height, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        else
        {
            SetWindowPos(m_hwndlistview, NULL, rect.left, rect.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }

    ShowWindow(m_hwndlistview, SW_SHOW);
    SetCapture(m_hwndlistview);
    PostMessage(m_hwndlistview, WM_LBUTTONDOWN, (WPARAM)MK_LBUTTON, MAKELPARAM(width - 2, 2));
}

//function sets the list view width
wyInt32
ButtonDropDown::SetListViewWidth(RECT* prect)
{
    wyInt32     i, count, width = 0, height = 0, temp = 0;
    RECT        rect = {0};
    LVITEM      lvi = {0};
    HFONT       hfont;
    wyWChar     buffer[256];
    HDC         hdc = GetDC(m_hwndlistview);

    //get the gui font and select it in the window dc
    hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    hfont = (HFONT)SelectObject(hdc, hfont);

    //loop through the list view item and calculate the maximum text extent
    for(i = 0, count = ListView_GetItemCount(m_hwndlistview); i < count; ++i)
    {
        if(i == 0)
        {
            ListView_GetItemRect(m_hwndlistview, 0, &rect, LVIR_ICON);

            if(!m_iconwidth)
            {
                m_iconwidth = rect.right;
            }

            temp = rect.bottom - rect.top;
        }

        memset(&lvi, 0, sizeof(lvi));
        lvi.mask = LVIF_TEXT;
        lvi.cchTextMax = 255;
        lvi.pszText = buffer;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        ListView_GetItem(m_hwndlistview, &lvi);
        memset(&rect, 0, sizeof(rect));
        DrawText(hdc, lvi.pszText, -1, &rect, DT_CALCRECT);

        if(rect.right - rect.left > width)
        {
            width = rect.right - rect.left;
        }

        height += temp + 10;
    }

    //padd some extra length
    memset(&rect, 0, sizeof(rect));
    DrawText(hdc, L"WWW", -1, &rect, DT_CALCRECT);
    width += m_iconwidth + rect.right;
        
    if(width < prect->right - prect->left)
    {
        width = prect->right - prect->left;
    }

    //set the column width
    ListView_SetColumnWidth(m_hwndlistview, 0, width);

    if(height > 10 * temp)
    {
        height = 10 * temp;
    }

    //set the height and width
    SetWindowPos(m_hwndlistview, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
    hfont = (HFONT)SelectObject(hdc, hfont);
    ReleaseDC(m_hwndlistview, hdc);
    DeleteObject(hfont);
    return width;
}

//the entire purpose of this is to eleminate the popup effect because of whcih the parent window's nc area will be disabled
LRESULT CALLBACK    
ButtonDropDown::ParentWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT         ret;
    ButtonDropDown* pbd = (ButtonDropDown*)GetWindowLongPtr(hwnd, GWLP_USERDATA);   

    if(message == WM_NCACTIVATE)
    {
        //prevent disabling the header
        wparam = TRUE;
    }
    
    //set the original data associated and call the original window proc
    SetWindowLongPtr(hwnd, GWLP_USERDATA, pbd->m_origparentdata);
    ret = CallWindowProc(pbd->m_origparentproc, hwnd, message, wparam, lparam);

    //set it back
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pbd);
    return ret;
}

wyBool 
RegisterButtonDropDown(HINSTANCE hInstance)
{
    ATOM		ret;
    WNDCLASS	wndclass = {0};
	
	wndclass.style         = 0;
    wndclass.lpfnWndProc   = ButtonDropDown::WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
	wndclass.hInstance     = hInstance ;
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	wndclass.lpszMenuName  = (LPCWSTR)NULL ;
    wndclass.lpszClassName = BUTTONDROPDOWN;
	
	VERIFY(ret = RegisterClass(&wndclass));
	return wyTrue;
}