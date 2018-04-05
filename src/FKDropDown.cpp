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

#include "FKDropDown.h"
#include "FrameWindowHelper.h"
#include "ButtonDropDown.h"
#include "Include.h"
#include "BlobMgmt.h"
#ifndef COMMUNITY
#include "FormView.h"
#endif

#define UM_ACTIVATE         WM_USER + 20
#define FKDROPDOWN_SECTION  "FKDropDown"

//thread structure
struct FKThreadStruct
{
    wyString*   m_pquery;
    FKDropDown* m_pfk;
    HANDLE      m_hevent;
};

FKDropDown::FKDropDown(DataView* pdv, BEHAVIOR_EVENT_PARAMS* params)
{
    htmlayout::dom::element src;
    wyString                temp;

    InitializeCriticalSection(&m_cs);
    m_pdv = pdv;
    m_pdbtemp = NULL;
    m_ptabletemp = NULL;
    m_threadstopstatus = 0;
    m_pfkinfo = m_pdv->m_data->m_pfkinfo;
    m_fkcount = m_pdv->m_data->m_fkcount;
    m_hwnd = NULL;
    m_hwndparentgrid = m_pdv->m_hwndgrid;
    m_hwndgrid = NULL;
	m_parentmyres = m_pdv->m_data->m_datares;
    m_pfuncptr = CustomGrid_GetGridProc(m_hwndparentgrid);
	m_pmdi = m_pdv->m_wnd;
    
    //if the call contains valid params, then it is from form view otherwise from grid, based on which we are identifying the current column
    if(params)
    {
        src = params->heTarget;
        src = src.first_sibling();
        temp.SetAs(src.get_attribute("id"));
        m_column = temp.GetAsInt32();
    }
    else
    {
        m_column = CustomGrid_GetCurSelCol(m_hwndparentgrid);
    }

    //get the slected row and column name
    m_row = CustomGrid_GetCurSelRow(m_hwndparentgrid);
    m_fkcolumnname.SetAs(m_parentmyres->fields[m_column].org_name ? 
        m_parentmyres->fields[m_column].org_name : m_parentmyres->fields[m_column].name, m_pmdi->m_ismysql41);

    m_index = NULL;
    m_indexcount = 0;
    m_myres = NULL;
    m_myrows = NULL;
    m_hwndcolumns = NULL;
    m_sortcol = -1;
    m_sortorder = GV_UNKNOWN;
    m_isautomated = wyTrue;
    m_item = -1;
    m_selrow = -1;
    m_selcol = -1;
    m_params = params;
    m_lastcheckeditem = -1;
    m_isinitcompleted = wyFalse;
    m_filteronvaluesentered = 1;
    m_pdb = &m_pdv->m_data->m_db;
    m_ptable = NULL;
}


//destructor
FKDropDown::~FKDropDown()
{
    DlgControl* pdlgctrl;

    if(m_index)
    {
        //free the index
        free(m_index);
    }

    //free the row array
    delete[] m_myrows;

    if(m_myres)
    {
        //free the MySQL result
        m_pmdi->m_tunnel->mysql_free_result(m_myres);
    }

    delete m_pdbtemp;
    delete m_ptabletemp;
    
    while((pdlgctrl = (DlgControl*)m_controllist.GetFirst()))
    {
        m_controllist.Remove(pdlgctrl);
        delete pdlgctrl;
    }

    DeleteCriticalSection(&m_cs);
}

//function creates the dialog box
wyInt32
FKDropDown::Create()
{
    const wyChar*   str;
    MSG             msg;

    //create and show the dialog box
    if(DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_FKDIALOG), m_hwndparentgrid, FKDropDown::DlgProc, (LPARAM)this) == IDCANCEL)
    {
        return IDCANCEL;
    }

    //remove any WM_LBUTTONUP messages from the queue. This is required to prevent the WM_LBUTTONUP firing on the underlying window, there by losing the focus
    while(PeekMessage(&msg, NULL, WM_LBUTTONUP, WM_LBUTTONUP, PM_REMOVE));
    CustomGrid_ApplyChanges(m_hwndparentgrid);    
    str = (*(m_myrows + m_selrow))[m_selcol] ? (*(m_myrows + m_selrow))[m_selcol] : "(NULL)";

    //call the grid procedure to mark it for editing
    m_pfuncptr(m_hwndparentgrid, GVN_BEGINLABELEDIT, m_row, m_column);

    //set the custom grid text
    CustomGrid_SetText(m_hwndparentgrid, 
        m_row, m_column, 
        str);

    //call the grid procedure to end edit
    m_pfuncptr(m_hwndparentgrid, GVN_ENDLABELEDIT, MAKELONG(m_row, m_column), (LPARAM)str);
    CustomGrid_SetCurSelCol(m_hwndparentgrid, m_column, wyFalse);
    return IDOK;
}

//function initializes the dialog
wyBool
FKDropDown::InitDialog(HWND hwnd)
{
    RECT                    rect = {0};
    wyInt32                 i;
    HWND                    hwndsearch;
    WNDPROC                 origproc, wndproc;
    wyString                temp;
    htmlayout::dom::element src;
    wyWChar*                buffer;
    json::string            items;
    json::value	            value;
    wyBool                  isfullname = wyFalse;
    HICON                   hicon;

    hicon = LoadIcon(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_TABLE));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon);
    DestroyIcon(hicon);

    //save the window handle
    m_hwnd = hwnd;
    
    //change the search edit control window procedure so that we can draw a text in the absense of focus and text
    hwndsearch = GetDlgItem(m_hwnd, IDC_SEARCHTEXT);
    origproc = (WNDPROC)SetWindowLongPtr(hwndsearch, GWLP_WNDPROC, (LONG_PTR)FKDropDown::SearchCtrlProc);
    SetWindowLongPtr(hwndsearch, GWLP_USERDATA, (LONG_PTR)origproc);
    
    //initialize the navigation controls
    InitializeNaviagationControls();

    //create the index, the index actually contains the entires in fk list in which the parent tables are referd by the selected column
    isfullname = CreateIndex();    

    //loop thyrought the index and form the window title
    for(i= 0; i < m_indexcount; ++i)
    {
        if(i)
        {
            temp.Add(", ");
        }

        if(isfullname == wyTrue)
        {
            temp.AddSprintf("`%s`.", (m_pfkinfo + m_index[i])->m_parenttabledb.GetString());
        }

        temp.AddSprintf("`%s`", (m_pfkinfo + m_index[i])->m_parenttable.GetString());
    }

    SetWindowText(m_hwnd, temp.GetAsWideChar());
    temp.Clear();

    //if there is no valid m_params, means we are processing a call from grid view
    if(!m_params)
    {
        //find the selected cell's coordinate, we need this to position the window relatively
        CustomGrid_GetSubItemRect(m_hwndparentgrid, m_row, m_column, &rect);
        MapWindowRect(m_hwndparentgrid, NULL, &rect);
        
        //Get the active control text, which we will use as the search string, if any
        if((i = CustomGrid_GetActiveControlTextLength(m_hwndparentgrid)))
        {
            buffer = new wyWChar[i + 2];
            CustomGrid_GetActiveControlText(m_hwndparentgrid, buffer, i + 1);
            temp.SetAs(buffer);
            delete[] buffer;
        }
    }
#ifndef COMMUNITY
    else
    {
        //find the selected field's coordinate, we need this to position the window relatively
        src = m_params->heTarget;
        src = src.prev_sibling();
        rect = src.get_location(1);
        rect.left -= 3;
        rect.right += 3;
        rect.bottom += 3;
        rect.top -= 3;
        MapWindowRect(src.get_element_hwnd(true), NULL, &rect);   

        //get the text
        m_pdv->m_formview->GetColumnElementText(&src, temp);
    }
#endif

    //set the search text formed
    SetWindowText(hwndsearch, temp.GetAsWideChar());

    GetWindowRect(hwnd, &m_wndrect);

    //create the grid to display the result
    CreateGrid();

    //now set the position of the grid after refresh button to get proper tab order
    SetWindowPos(m_hwndgrid, GetDlgItem(m_hwnd, IDC_REFRESHFK), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    //clear the search text if it represents NULL or autoinc
    if(!temp.Compare("(NULL)") || !temp.Compare("(Auto)") || !temp.CompareI("CURRENT_TIMESTAMP"))
    {
        SetWindowText(hwndsearch, L"");
    }
    
    //function executes the query looking for the exact match
    if(ExecuteQuery(wyFalse) == wyFalse)
    {
        //if the query fails, means a MySQL error occured. thus we are returning without showing the dialog box
        return wyFalse;
    }

    //modify the window procedure for the static control which draws the resize grip
    wndproc = (WNDPROC)GetWindowLongPtr(GetDlgItem(m_hwnd, IDC_SIZEGRIP), GWLP_WNDPROC);
    SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_SIZEGRIP), GWLP_WNDPROC, (LONG_PTR)GripProc);
    SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_SIZEGRIP), GWLP_USERDATA, (LONG_PTR)wndproc);

    //modify the window proc of refresh button, so that we can detect the mouse movement over it. It is required to change the cursor to arrow on mouse hover while executing queries
    wndproc = (WNDPROC)GetWindowLongPtr(GetDlgItem(m_hwnd, IDC_REFRESHFK), GWLP_WNDPROC);
    SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_REFRESHFK), GWLP_WNDPROC, (LONG_PTR)ExecButtonProc);
    SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_REFRESHFK), GWLP_USERDATA, (LONG_PTR)wndproc);

    GetWindowRect(GetDlgItem(m_hwnd, IDC_SIZEGRIP), &m_dlgrect);
    MapWindowRect(NULL, m_hwnd, &m_dlgrect);
    SetWindowPos(GetDlgItem(m_hwnd, IDCANCEL), GetDlgItem(m_hwnd, IDC_SIZEGRIP), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    //get the client rect of the window
    GetClientRect(m_hwnd, &m_dlgrect);

    //get the control rects and save it in a linked list
    GetCtrlRects();

    //set the window postion by reading ini. But here we will persist only window size and will discard the window position
    SetWindowPositionFromINI(m_hwnd, FKDROPDOWN_SECTION, m_wndrect.right - m_wndrect.left, m_wndrect.bottom - m_wndrect.top);

    //function positions the window properly based on the cell/element currently selected
    PositionWindow(&rect);

    //set this flag to true to indicate the success of initialization, which we check in WM_DESTROY 
    m_isinitcompleted = wyTrue;

    //post this message, so that it can do a uzzy search on dialog activation, this will be done only when the initial query returns zro result
    PostMessage(hwnd, UM_ACTIVATE, 0, 0);
    return wyTrue;
}

//this function postionns the window relative to the prect passed in
void
FKDropDown::PositionWindow(RECT* prect)
{
    HMONITOR    hmonitor;
    MONITORINFO mi = {0};
    RECT        temprect = {0};
    wyInt32     animate = 0, width, height, hmargin = 0, vmargin = 0;

    //get the modified window rect
    GetWindowRect(m_hwnd, &temprect);

    //get the monitor info for the monitor associated with the rect
    hmonitor = MonitorFromRect(prect, MONITOR_DEFAULTTONEAREST);
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hmonitor, &mi);

    //calculate and modify the width and height based on the availabe space to best fit the window
    width = temprect.right - temprect.left;
    height = temprect.bottom - temprect.top;
    hmargin = mi.rcWork.right - prect->left;
    hmargin = prect->left - mi.rcWork.left > hmargin ? prect->left - mi.rcWork.left : hmargin;
    vmargin = mi.rcWork.bottom - prect->bottom;
    vmargin = prect->top - mi.rcWork.top > vmargin ? prect->top - mi.rcWork.top : vmargin;

    if(width > hmargin)
    {
        width = hmargin;
    }

    if(height > vmargin)
    {
        height = vmargin;
    }

    //resize the window
    SetWindowPos(m_hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
    
    //calculate the left cordinate of the window
    if(prect->left + width > mi.rcWork.right)
    {  
        prect->left = prect->right - width;
        animate |= AW_HOR_NEGATIVE;
    }
    else
    {
        animate |= AW_HOR_POSITIVE;
    }

    //calculate the top cordinate of the window
    if(prect->bottom + height > mi.rcWork.bottom)
    {
        prect->bottom = prect->top - height;
        animate |= AW_VER_NEGATIVE;
    }
    else
    {
        animate |= AW_VER_POSITIVE;
    }

    //position the window properly
    SetWindowPos(m_hwnd, NULL, prect->left, prect->bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    
    //now position the controls
    PositionCtrls();

    //uncomment the below code to animate the window
    /*AnimateWindow(m_hwnd, 200, AW_ACTIVATE | animate);
    InvalidateRect(m_hwnd, NULL, TRUE);
    UpdateWindow(m_hwnd);*/
}

//callback window procedure for grip static window
LRESULT CALLBACK 
FKDropDown::GripProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    WNDPROC proc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if(message == WM_PAINT)
    {
        //draw the size grip
        DrawSizeGripOnPaint(hwnd);
        return 1;
    }

    return CallWindowProc(proc, hwnd, message, wparam, lparam);
}

//callback window procedure for grip refresh button
LRESULT CALLBACK 
FKDropDown::ExecButtonProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    WNDPROC     proc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    FKDropDown* pfk;
    POINT       pt = {0};
    RECT        rect = {0};

    if(message == WM_MOUSEMOVE)
    {
        //we are handling the mouse move
        GetCursorPos(&pt);
        GetWindowRect(hwnd, &rect);

        if(PtInRect(&rect, pt))
        {
            //if it is over the refresh button, change the cursor to arrow
            SetCursor(LoadCursor(NULL, IDC_ARROW)); 
        }
        else
        {
            //else, we are checking for the execution status
            pfk = (FKDropDown*)GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA);

            if(!pfk->ThreadStopStatus())
            {
                //if execution is in progress, change it to wait cursor
                SetCursor(LoadCursor(NULL, IDC_WAIT));
            }
        }
    }

    return CallWindowProc(proc, hwnd, message, wparam, lparam);
}

//function initializes the navigation control
void 
FKDropDown::InitializeNaviagationControls()
{
    SendMessage(GetDlgItem(m_hwnd, IDC_LIMITROWS), BM_SETCHECK, (WPARAM)TRUE, 0);
    SetWindowText(GetDlgItem(m_hwnd, IDC_ROWSTART), L"0");
    SetWindowText(GetDlgItem(m_hwnd, IDC_ROWCOUNT), L"25");
    SetLimits(TRUE);
}

//the main dialog procedure
INT_PTR CALLBACK 
FKDropDown::DlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    FKDropDown* pfk = (FKDropDown*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	
	switch(message)
	{
        //since it is a tool bar window, prevent the attemp to minimize, amximize
        case WM_NCRBUTTONDOWN:
        case WM_NCRBUTTONUP:
        case WM_NCLBUTTONDBLCLK:
            return TRUE;

        //we get this message once the dialog is displayed
        case UM_ACTIVATE:
            if(pfk && pfk->m_isautomated == wyTrue)
            {
                pfk->m_isautomated = wyFalse;

                //if the initial query returns 0 rows, then we are doing a fuzzy search
                if(pfk->m_pmdi->m_tunnel->mysql_num_rows(pfk->m_myres) == 0)
                {
                    pfk->ExecuteQuery(wyFalse);
                }
                //otherwise, set the focus on the grid
                else
                {
                    SetFocus(pfk->m_hwndgrid);
                }
            }
            break;

        case WM_INITDIALOG:
    		pfk = (FKDropDown*)lparam;   
		    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pfk);
            LocalizeWindow(hwnd);

            //initialize the dialog and if it fails, destroy the dialog
            if(pfk->InitDialog(hwnd) == wyFalse)
            {
                EndDialog(hwnd, IDCANCEL);
            }
            break;

        case WM_CLOSE:

            //if the execution is stoped, then destroy the dialog
            if(pfk && pfk->ThreadStopStatus())
            {
                EndDialog(hwnd, IDCANCEL);
            }
            break;

        case WM_COMMAND:

            if(pfk)
            {
                //handle the various commands
                return pfk->OnWMCommand(wparam, lparam);
            }
            break;

        case WM_DESTROY:
            if(pfk)
            {
                //on destruction, if the initialization was successful we need to persist the settings
                if(pfk->m_isinitcompleted == wyTrue)
                {
                    //persist the "filter onvalues (already) enterd" setting
                    pfk->FilterOnValuesEntered(pfk->m_filteronvaluesentered);

                    //save the dialog position
                    SaveInitPos(hwnd, FKDROPDOWN_SECTION);
                }

                if(pfk->m_hwndgrid)
                {                       
                    //destroy the grid
                    DestroyWindow(pfk->m_hwndgrid);
                }
            }
            break;

        case WM_NOTIFY:
            //handle the notifications coming from filter column dropdown
            if(pfk && ((LPNMHDR)lparam)->hwndFrom == pfk->m_hwndcolumns)
            {
                pfk->OnColumnListItemChange(lparam);
            }
            break;

        case WM_SIZE:
            //handle the window resizing and position the controls
		    if(pfk && (wparam == SIZE_RESTORED || wparam == SIZE_MAXIMIZED))
            {
                pfk->PositionCtrls();
                break;
            }
            return FALSE;

        case WM_GETMINMAXINFO:
            //restrict the minimum possible size
            if(pfk)
            {
                pfk->OnWMSizeInfo(lparam);
            }
            break;

        case WM_HELP:
            ShowHelp("http://sqlyogkb.webyog.com/article/103-looking-up-constraints-for-a-value");
		    break;

        default:
            return FALSE;
    }

    return TRUE;
}

//when a list item notification is received
void
FKDropDown::OnColumnListItemChange(LPARAM lparam)
{
    LPNMBUTTONDROPDOWN  pnmbd = (LPNMBUTTONDROPDOWN)lparam;
    wyInt32             i, j, checkstate, lastcheckstate;

    if(((LPNMHDR)lparam)->code != BDN_ENDHIT || (pnmbd->info.flags & LVHT_ONITEMSTATEICON) == 0)
    {
        //if it is not BDN_ENDHIT and the hit is not on top of state icon, we are discarding it
        return;
    }

    //the code handles the shift click
    if((GetKeyState(VK_SHIFT) & SHIFTED) && 
       m_lastcheckeditem != -1 &&
       m_lastcheckeditem != pnmbd->info.iItem)
    {
        lastcheckstate = ListView_GetCheckState(m_hwndcolumns, m_lastcheckeditem);
        checkstate = !ListView_GetCheckState(m_hwndcolumns, pnmbd->info.iItem);

        if(m_lastcheckeditem > pnmbd->info.iItem)
        {
            i = pnmbd->info.iItem;
            j = m_lastcheckeditem;
        }
        else
        {
            i = m_lastcheckeditem;
            j = pnmbd->info.iItem;
        }

        if(lastcheckstate == checkstate)
        {
            checkstate = !checkstate;
        }
        else
        {
            checkstate = lastcheckstate;
        }

        while(i <= j)
        {
            ListView_SetCheckState(m_hwndcolumns, i, checkstate);
            i++;
        }
    }

    //store the last check item and the state of the last item in the list
    m_lastcheckeditem = pnmbd->info.iItem;
    m_filteronvaluesentered = ListView_GetCheckState(m_hwndcolumns, ListView_GetItemCount(m_hwndcolumns) - 1);
}

//function handles the WM_COMMAND
BOOL
FKDropDown::OnWMCommand(WPARAM wparam, LPARAM lparam)
{
    BOOL checkstate;

    if(LOWORD(wparam) == IDC_REFRESHFK)
    {
        //if the thread is stoped, i.e, no query execution
        if(ThreadStopStatus())
        {
            //execute the query
            ExecuteQuery(wyFalse);
        }
        else
        {       
            //set the thread stop status, this will the flag the thread and next time when the tread check this flag, it will stop
            ThreadStopStatus(1);    
                
            //if it is http tunnel, then we are setting a variable to stop the query
            if(m_pmdi->m_tunnel->IsTunnel())
            {
                EnterCriticalSection(&m_cs);
		    	m_pmdi->StopQueryExecution();
                LeaveCriticalSection(&m_cs);
            }
            //else, we are killing the current process and replacing it with a new one
            else
            {
                m_pmdi->StopQuery();
            }
                
            //disable the window and once the thread stoped completely, it will enable it back
            EnableWindow(GetDlgItem(m_hwnd, IDC_REFRESHFK), FALSE);
        }

        return TRUE;
    }

    //prevent commands from other controls when execution is in progress
    if(!ThreadStopStatus())
    {
        if(LOWORD(wparam) == IDC_LIMITROWS)
        {
            checkstate = SendMessage(GetDlgItem(m_hwnd, IDC_LIMITROWS), BM_GETCHECK, 0, 0);
            SendMessage(GetDlgItem(m_hwnd, IDC_LIMITROWS), BM_SETCHECK, (WPARAM)!checkstate, 0);
        }

        return TRUE;
    }

    switch(LOWORD(wparam))
    {
        case IDCANCEL:
            EndDialog(m_hwnd, IDCANCEL);
            break;
            
        case IDOK:
            //insert the currently selected row
            OnEnterKeyInGrid();
            break;

        case IDC_LIMITROWS:
            //set the limits and execute the query
            checkstate = SendMessage(GetDlgItem(m_hwnd, IDC_LIMITROWS), BM_GETCHECK, 0, 0);
            SetLimits(checkstate);
            ExecuteQuery(wyTrue);
            break;

        case IDC_PREV:
        case IDC_NEXT:
            //on previous/next buttons
            OnNextOrPrev((HWND)lparam);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

//function enables/disable the limit related controls
void FKDropDown::SetLimits(BOOL set)
{
    EnableWindow(GetDlgItem(m_hwnd, IDC_ROWSTART), set);
    EnableWindow(GetDlgItem(m_hwnd, IDC_ROWCOUNT), set);
    EnableWindow(GetDlgItem(m_hwnd, IDC_ROWS), set);
    EnableWindow(GetDlgItem(m_hwnd, IDC_FIRSTROW), set);
    EnableWindow(GetDlgItem(m_hwnd, IDC_ROWS), set);
    EnableWindow(GetDlgItem(m_hwnd, IDC_PREV), set);
    EnableWindow(GetDlgItem(m_hwnd, IDC_NEXT), set);
}

//callback procedure for search control
LRESULT	CALLBACK 
FKDropDown::SearchCtrlProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    WNDPROC     origproc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    HDC         hdc;
    PAINTSTRUCT ps;
    HFONT       hfont;
    LOGFONT     lf = {0};
    RECT        rect = {0};
    HBRUSH      hbrush;

    switch(message)
    {
        case WM_PAINT:
            //we handle paint message to draw a search prompt in the control when there is no search text and focus
            if(!GetWindowTextLength(hwnd) && GetFocus() != hwnd)
            {
                hdc = BeginPaint(hwnd, &ps);
                hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
                GetObject(hfont, sizeof(LOGFONT), &lf);
                DeleteObject(hfont);
                lf.lfItalic = 1;
                wcscpy(lf.lfFaceName, L"Courier New");
                hfont = CreateFontIndirect(&lf);
                hfont = SelectFont(hdc, hfont);
                GetClientRect(hwnd, &rect);
                rect.left += 5;
                DrawText(hdc, _(L"Enter filter text here..."), -1, &rect, DT_VCENTER);
                hfont = SelectFont(hdc, hfont);
                DeleteFont(hfont);
                EndPaint(hwnd, &ps);
                return 1;
            }
            break;

        case WM_ERASEBKGND:
            //handle erase message to fill the control with window color when there is no search text and focus
            if(!GetWindowTextLength(hwnd) && GetFocus() != hwnd)
            {
                hbrush = GetSysColorBrush(COLOR_WINDOW);
                hdc = (HDC)wparam;
                GetClientRect(hwnd, &rect);
                FillRect(hdc, &rect, hbrush);
                return 1;
            }

            break;
    }

    //call the original proc
    return CallWindowProc(origproc, hwnd, message, wparam, lparam);
}

//function creates the index, this is the most important part of this feature
wyBool 
FKDropDown::CreateIndex()
{
    wyInt32         i, j, k, count, t;
    RelTableFldInfo *prelinfo = NULL, *ptemprelinfo;
    wyString        temp;
    wyChar*         outstring;
    wyBool          isdiffdb = wyFalse;
    MYSQL_ROWEX*    myrowex;
    MYSQL_ROW       myrow;

    count = CustomGrid_GetColumnCount(m_hwndparentgrid);

    //the following loop builds the index
    //as per our refactoring rules, these many inner loops are not recommended, but I couldn't resist writing it :-( (may be I was a bit lazy on that day; and pls don't talk about complexity here ;-))
    for(i = 0; i < m_fkcount; ++i)
    {
        //traverse trhough the field list of the ith item
        for(prelinfo = (RelTableFldInfo*)(m_pfkinfo + i)->m_fkeyfldlist->GetFirst(); prelinfo; 
            prelinfo = (RelTableFldInfo*)prelinfo->m_next)
        {
            //compare the field name with fk comun name
            if(!prelinfo->m_tablefld.CompareI(m_fkcolumnname))
            {
                //if it ismatching, inner traverse the field list of the ith item
                for(prelinfo = (RelTableFldInfo*)(m_pfkinfo + i)->m_fkeyfldlist->GetFirst(); prelinfo; 
                    prelinfo = (RelTableFldInfo*)prelinfo->m_next)
                {
                    //inner traverse the fk list
                    for(j = 0; j < m_fkcount; ++j)
                    {
                        //inner travere through the field list of the jth item
                        for(ptemprelinfo = (RelTableFldInfo*)(m_pfkinfo + j)->m_fkeyfldlist->GetFirst(); ptemprelinfo; 
                            ptemprelinfo = (RelTableFldInfo*)ptemprelinfo->m_next)
                        {
                            //if the field name and fk column name are matching, add it to the index
                            if(!prelinfo->m_tablefld.CompareI(ptemprelinfo->m_tablefld))
                            {
                                AddIndex(j);

                                if(isdiffdb == wyFalse && m_indexcount)
                                {
                                    if(m_indexcount == 1)
                                    {
                                        if(m_pdb->Compare((m_pfkinfo + m_index[0])->m_parenttabledb))
                                        {
                                            isdiffdb = wyTrue;
                                        }
                                    }
                                    else
                                    {
                                        if((m_pfkinfo + m_index[m_indexcount - 1])->m_parenttabledb.Compare((m_pfkinfo + m_index[m_indexcount - 2])->m_parenttabledb))
                                        {
                                            isdiffdb = wyTrue;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                break;
            }
        }
    }

    //the following code block creates composite filter if any
    for(i = 0; i < m_indexcount; ++i)
    {
        //search the fk column name in the field list of ith item in the index
        for(k = 0, prelinfo = (RelTableFldInfo*)(m_pfkinfo + m_index[i])->m_fkeyfldlist->GetFirst();
            prelinfo; 
            ++k, prelinfo = (RelTableFldInfo*)prelinfo->m_next)
        {
            if(!prelinfo->m_tablefld.CompareI(m_fkcolumnname))
            {
                break;
            }
        }

        if(k >= (m_pfkinfo + m_index[i])->m_fkeyfldlist->GetCount())
        {
            //if no match found, continue with the next item in the list
            continue;
        }

         //now, we will compare the fields listed with the column names in the parent grid
        for(t = 0, ptemprelinfo = (RelTableFldInfo*)(m_pfkinfo + m_index[i])->m_pkeyfldlist->GetFirst(); 
            ptemprelinfo; 
            ++t, ptemprelinfo = (RelTableFldInfo*)ptemprelinfo->m_next)
        {
            if(t == k)
            {
                continue;
            }

            prelinfo = (RelTableFldInfo*)(*(m_pfkinfo + m_index[i])->m_fkeyfldlist)[t];
            
            for(j = 0; j < count; ++j)
            {
                if(m_ptable)
                {
                    if(m_ptable->Compare(m_parentmyres->fields[j].org_table ? m_parentmyres->fields[j].org_table : m_parentmyres->fields[j].table) ||
                        m_pdb->Compare(m_parentmyres->fields[j].db))
                    {
                        continue;
                    }
                }

                //get the column title in the parent grid
                temp.SetAs(m_parentmyres->fields[j].org_name ? m_parentmyres->fields[j].org_name : m_parentmyres->fields[j].name, m_pmdi->m_ismysql41);
                
                if(!prelinfo->m_tablefld.CompareI(temp))
                {
                    //when a match is found break from the loop
                    break;
                }
            }

            //when a match is found, add it to composite filter condition
            if(j < count)
            {
                myrowex = m_pdv->m_data->m_rowarray->GetRowExAt(m_row);

                if(!myrowex || !(myrowex->m_row[j]))
                {
                    continue;
                }

                myrow = myrowex->m_row;
                
                if(strlen(myrow[j]) && strcmp("(NULL)", myrow[j]) && 
                    strcmp("(Auto)", myrow[j]) && strcmpi("CURRENT_TIMESTAMP", myrow[j]))
                {
                    if(m_compositefilter.GetLength())
                    {
                        m_compositefilter.Add(" and ");
                    }

                    //get the data of the jth column in the parent result set
                    temp.SetAs(myrow[j], m_pmdi->m_ismysql41);
                    
                    //mysql escape
                    outstring = new wyChar[temp.GetLength() * 2 + 2];
                    m_pmdi->m_tunnel->mysql_real_escape_string(m_pmdi->m_mysql, outstring, temp.GetString(), temp.GetLength());
                    temp.Sprintf(" = '%s'", outstring);
                    delete[] outstring;

                    //add it to the composite filter string 
                    m_compositefilter.AddSprintf("`%s`.`%s`.`%s` %s", 
                        (m_pfkinfo + m_index[i])->m_parenttabledb.GetString(),
                        (m_pfkinfo + m_index[i])->m_parenttable.GetString(), 
                        ptemprelinfo->m_tablefld.GetString(),
                        temp.GetString());
                }
            }
        }
    }

    return isdiffdb;
}

//helper function to add index
void
FKDropDown::AddIndex(wyInt32 i)
{
    wyInt32 j;

    //first make sure that the table is not there in the index list
    for(j = 0; j < m_indexcount; ++j)
    {
        if(!(m_pfkinfo + i)->m_parenttable.CompareI((m_pfkinfo + m_index[j])->m_parenttable) &&
            !(m_pfkinfo + i)->m_parenttabledb.CompareI((m_pfkinfo + m_index[j])->m_parenttabledb))
        {
            break;
        }
    }

    if(j == m_indexcount)
    {
        //increase index count
        m_indexcount++;

        //allocate/reallocate memory
        if(m_index)
        {
            m_index = (wyInt32*)realloc(m_index, m_indexcount * sizeof(wyInt32));
        }
        else
        {
            m_index = (wyInt32*)malloc(m_indexcount * sizeof(wyInt32));
        }

        //set the value
        m_index[m_indexcount - 1] = i;
    }
}

//function creates the grid to display the result
wyBool 
FKDropDown::CreateGrid()
{
    RECT    rectsearch = {0}, rectcancel = {0}, rectrowcount = {0};
    HFONT   hfont;
    LOGFONT lf = {0};

    //get the window rectangle of surrounding controls. this is done to calculate the grid size
    GetWindowRect(GetDlgItem(m_hwnd, IDC_FILTERPROMPT), &rectsearch);
    MapWindowRect(NULL, m_hwnd, &rectsearch);
    GetWindowRect(GetDlgItem(m_hwnd, IDCANCEL), &rectcancel);
    MapWindowRect(NULL, m_hwnd, &rectcancel);
    GetWindowRect(GetDlgItem(m_hwnd, IDC_NAVGROUP), &rectrowcount);
    MapWindowRect(NULL, m_hwnd, &rectrowcount);

    //create the grid
    m_hwndgrid = CreateCustomGridEx(m_hwnd, 
        rectsearch.left, 
        rectrowcount.bottom + 4, 
        rectcancel.right - rectsearch.left, 
        (rectcancel.top - rectrowcount.bottom) - 8, 
        GridWndProc, 
        GV_EX_OWNERDATA, 
        (LPARAM)this);
     
     if(!m_hwndgrid)
         return wyFalse;

     //set owner data to true, so that we can supply the data when required
     CustomGrid_SetOwnerData(m_hwndgrid, wyTrue);

     //get the column font of the parent grid and use it in the grid
     hfont = CustomGrid_GetColumnFont(m_hwndparentgrid);
     GetObject(hfont, sizeof(lf), &lf);
     CustomGrid_SetFont(m_hwndgrid, &lf);
     return wyTrue;
}

//the callback procedure for notifications received from grid window
LRESULT CALLBACK
FKDropDown::GridWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    FKDropDown* pfk = (FKDropDown*)CustomGrid_GetLongData(hwnd);
    POINT       pt = {0};
    wyInt32     row, col;
	
	switch (message)
	{
        case GVN_GETDISPINFO: 
            //supply the data
            pfk->OnGVDispInfo(wparam);
            break;

        case GVN_BEGINLABELEDIT:
            //no editing
            return 0;

        //handle sorting
        case GVN_COLUMNCLICK:
            //find the sort column
            if(pfk->m_sortcol != wparam)
            {
                pfk->m_sortcol = wparam;
                pfk->m_sortorder = GV_UNKNOWN;
            }

            //find the sort order
            switch(pfk->m_sortorder)
            {
                case GV_UNKNOWN:
                    pfk->m_sortorder = GV_ASCENDING;
                    break;
                    
                case GV_ASCENDING:
                    pfk->m_sortorder = GV_DESCENDING;
                    break;

                default:    
                    pfk->m_sortorder = GV_UNKNOWN;
            }

            //execute the query
            pfk->ExecuteQuery(wyTrue);
            break;

        //icon for sort order
        case GVN_COLUMNDRAW:
            if(pfk->m_sortcol == wparam)
            {
                return pfk->m_sortorder;
            }

            return GV_UNKNOWN;

        case GVN_KEYDOWN:
            //if key down on grid is Enter key, we need to prcess it as Insert
            if(wparam == VK_RETURN)
            {
                return pfk->OnEnterKeyInGrid();
            }

            return TRUE;

        case GVN_LBUTTONDBLCLK:
            pt.x = wparam;
            pt.y = lparam;
            CustomGrid_GetItemFromPoint(hwnd, &pt, &row, &col);

            //if double click is on row, process it as insert
            if(row != -1 && col != -1)
            {
                pfk->OnEnterKeyInGrid();
            }

            break;

        case GVN_BUTTONCLICK:
            //for blob fields
            pfk->OnGVButtonClick();
            break;

        case GVN_SPLITTERMOVE:
            //persist the column width if the option is turned on in preferences
            if(pfk->m_myres && IsRetainColumnWidth() == wyTrue)
            {
                pfk->OnSplitterMove(wparam);
            }

            break;

        case GVN_NEXTTABORDERITEM:
            SetFocus(GetNextDlgTabItem(pfk->m_hwnd, hwnd, FALSE));
            break;

        case GVN_PREVTABORDERITEM:
            SetFocus(GetNextDlgTabItem(pfk->m_hwnd, hwnd, TRUE));
            break;
	}

	return TRUE;
}

//function to perform insert value
wyInt32 
FKDropDown::OnEnterKeyInGrid()
{
    wyInt32     row, col;
    wyWChar     buffer[256];
    LVITEM      lvi = {0};
    wyString    temp;

    //just a precaution
    if(!CustomGrid_GetRowCount(m_hwndgrid))
    {
        return 1;
    }

    //get the selected row
    row = CustomGrid_GetCurSelRow(m_hwndgrid);

    //get the item curresponding to the fk column we are operating with
    lvi.mask = LVIF_PARAM | LVIF_TEXT;
    lvi.cchTextMax = 255;
    lvi.pszText = buffer;
    lvi.iItem = m_item;
    lvi.iSubItem = 0;
    ListView_GetItem(m_hwndcolumns, &lvi);
    temp.SetAs(lvi.pszText);

    //locate the column in the list -- only a precaution
    for(col = 0; col < m_myres->field_count; ++col)
    {
        if(!temp.CompareI(m_myres->fields[col].name) && 
            !(m_pfkinfo + m_index[lvi.lParam])->m_parenttable.CompareI(m_myres->fields[col].table) &&
            !(m_pfkinfo + m_index[lvi.lParam])->m_parenttabledb.CompareI(GetDbName(col)))
        {
            break;
        }
    }

    if(row == -1 || col == -1 || col >= m_myres->field_count)
    {
        return 1;
    }

    //set the selected row and column, so that we can insert proper value in the column and end the dialog
    m_selrow = row;
    m_selcol = col;
    EndDialog(m_hwnd, IDOK);
    return 0;
}

const wyChar*
FKDropDown::GetDbName(wyInt32 col)
{
    if(m_pmdi->m_tunnel->IsTunnel())
    {
        return (m_pfkinfo + m_index[0])->m_parenttabledb.GetString();
    }

    return m_myres->fields[col].db;
}

//save the column width
void
FKDropDown::OnSplitterMove(wyInt32 column)
{
    wyString table, columnname, dbname;

    table.SetAs(m_myres->fields[column].table, m_pmdi->m_ismysql41);
    columnname.SetAs(m_myres->fields[column].name, m_pmdi->m_ismysql41);
    dbname.SetAs(GetDbName(column), m_pmdi->m_ismysql41);
    SaveColumnWidth(m_hwndgrid, &dbname, &table, &columnname, column);
}

//this function forms the query to be executed
void 
FKDropDown::FormQuery(wyString& query, wyBool isignorefilter)
{
    wyString            condition, limit;
    const wyChar*       table = NULL;
    wyInt32             i, t = 0, k = -1;
    RelTableFldInfo*    prelinfo;

    //get the table name
    table = GetTableName().GetString();

    //whether it is called during dialog initialization
    if(m_isinitcompleted == wyFalse)
    {
        //get filter text
        GetFilterText(condition);

        //find the index of the curresponding parent table name
        for(i = 0; i < m_indexcount; ++i)
        {
            for(t = 0, prelinfo = (RelTableFldInfo*)(m_pfkinfo + m_index[i])->m_fkeyfldlist->GetFirst(); prelinfo; 
                ++t, prelinfo = (RelTableFldInfo*)prelinfo->m_next)
            {
                if(!m_fkcolumnname.CompareI(prelinfo->m_tablefld))
                {
                    k = i;
                    break;
                }
            }

            if(k != -1)
            {
                break;
            }
        }

        //make the query
        query.Sprintf("select * from %s", table);

        if(condition.GetLength())
        {
            m_filter.Sprintf(" where `%s`.`%s`.`%s`", 
                (m_pfkinfo + m_index[k])->m_parenttabledb.GetString(),
                (m_pfkinfo + m_index[k])->m_parenttable.GetString(),
                ((RelTableFldInfo*)(*(m_pfkinfo + m_index[k])->m_pkeyfldlist)[t])->m_tablefld.GetString());

            //add proper condition
            m_filter.AddSprintf(" = '%s'", condition.GetString());
        }

        //add the composite filter if any
        if((m_filteronvaluesentered = FilterOnValuesEntered(-1)) && m_compositefilter.GetLength())
        {
            m_filter.AddSprintf(condition.GetLength() ? " and %s" : " where %s", m_compositefilter.GetString());
        }

        //add filter to the query
        query.Add(m_filter.GetString());

        //get the limit clause
        GetLimits(limit);

        //final query
        query.AddSprintf("%s", limit.GetString());
        return;
    }

    query.SetAs("select *");

    //check whether to ignore filter. when sorting, next/prev happens, we dont want to change the filter
    if(isignorefilter == wyFalse)
    {
        //get the filter
        GetColumnsToFilter(m_filter);
    }

    //add the filter
    query.AddSprintf(" from %s %s", table, m_filter.GetString());

    //get the limits
    GetLimits(limit);

    //if sort order is present, add sort clause
    if(m_sortorder != GV_UNKNOWN)
    {
        query.AddSprintf(" ORDER BY %d %s ", m_sortcol + 1, m_sortorder == GV_ASCENDING ? "ASC" : "DESC");
    }
    
    //final query
    query.AddSprintf("%s", limit.GetString());
}

//function gets the filter text
wyString& 
FKDropDown::GetFilterText(wyString& text)
{
    wyWChar buffer[256];
    wyChar* out;
    
    GetWindowText(GetDlgItem(m_hwnd, IDC_SEARCHTEXT), buffer, 255);
    text.SetAs(buffer);
    text.LTrim();
    text.RTrim();
    out = new wyChar[text.GetLength() * 2 + 2];
    m_pmdi->m_tunnel->mysql_escape_string(out, text.GetString(), text.GetLength());
    text.SetAs(out);
    delete[] out;
    return text;
}

//Function gets the table name. it may or may not be join of tables depending on the number of parent tables the column refers to
wyString&
FKDropDown::GetTableName()
{
    wyInt32         i, j, count1, count2, x, y;
    wyString        temp;
    const wyChar*   fieldname;

    //if it is already created, return
    if(m_table.GetLength())
    {
        return m_table;
    }

    //join the parent tables tables 
    for(i = 0; i < m_indexcount; ++i)
    {
        if(i)
        {
            m_table.Add(" inner join ");
        }

        m_table.AddSprintf("`%s`.`%s`", (m_pfkinfo + m_index[i])->m_parenttabledb.GetString(), (m_pfkinfo + m_index[i])->m_parenttable.GetString());
    }

    //if more than one parent table is there, then we need to build proper join condition
    if(m_indexcount > 1)
    {
        m_table.AddSprintf(" on ");

        //it is a multi multi level loop finding out the common fields in the parent parent tables and forming the query
        for(i = 0; i < m_indexcount; ++i)
        {
            for(x = 0, count1 = (m_pfkinfo + m_index[i])->m_fkeyfldlist->GetCount(); x < count1; ++x)
            {
                for(j = i + 1; j < m_indexcount; ++j)
                {
                    for(y = 0, count2 = (m_pfkinfo + m_index[j])->m_fkeyfldlist->GetCount(); y < count2; ++y)
                    {
                        fieldname = ((RelTableFldInfo*)(*((m_pfkinfo + m_index[j])->m_fkeyfldlist))[y])->m_tablefld.GetString();
                        
                        if(!((RelTableFldInfo*)(*((m_pfkinfo + m_index[i])->m_fkeyfldlist))[x])->m_tablefld.CompareI(fieldname))
                        {
                            if(temp.GetLength())
                            {
                                temp.Add(" and ");
                            }

                            temp.AddSprintf("`%s`.`%s`.`%s` = `%s`.`%s`.`%s`",
                                (m_pfkinfo + m_index[i])->m_parenttabledb.GetString(),
                                (m_pfkinfo + m_index[i])->m_parenttable.GetString(), 
                                ((RelTableFldInfo*)(*((m_pfkinfo + m_index[i])->m_pkeyfldlist))[x])->m_tablefld.GetString(),
                                (m_pfkinfo + m_index[j])->m_parenttabledb.GetString(), 
                                (m_pfkinfo + m_index[j])->m_parenttable.GetString(), 
                                ((RelTableFldInfo*)(*((m_pfkinfo + m_index[j])->m_pkeyfldlist))[y])->m_tablefld.GetString()
                                );
                        }
                    }
                }
            }
        }
    }

    //add it to the table
    m_table.Add(temp.GetString());
    return m_table;
}

//function gets the columns to filter
void FKDropDown::GetColumnsToFilter(wyString& filter)
{
    wyInt32     i, count;
    LVITEM      lvi;
    wyInt32     j = 0;
    wyString    temp, condition;
    wyWChar     buffer[256];

    //get the filter text
    GetFilterText(condition);

    //get the list view count omiting the last one
    count = ListView_GetItemCount(m_hwndcolumns) - 1;

    if(!condition.GetLength())
    {
        //if there is no filter text clear the filter text
        filter.Clear();
        
        //and if filter on values enterd is enabled, add the composite filter
        if(ListView_GetCheckState(m_hwndcolumns, count) && m_compositefilter.GetLength())
        {
            filter.AddSprintf("where %s ", m_compositefilter.GetString());
        }
        
        return;
    }

    filter.SetAs("where (");

    //loop through the items
    for(i = 0; i < count; ++i)
    {
        //if the item is checked then we need to include it in our filter
        if(ListView_GetCheckState(m_hwndcolumns, i))
        {
            memset(&lvi, 0, sizeof(lvi));
            lvi.mask = LVIF_PARAM | LVIF_TEXT;
            lvi.cchTextMax = 255;
            lvi.pszText = buffer;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            ListView_GetItem(m_hwndcolumns, &lvi);

            if(j)
            {
                //we are performing a fuzzy search using or operator
                filter.Add(" or ");
            }

            temp.SetAs(lvi.pszText);
            
            //if more than one table is used, then we will use fully qualified name
            if(m_indexcount > 1)
            {
                filter.AddSprintf("`%s`.`%s`.", 
                    (m_pfkinfo + m_index[lvi.lParam])->m_parenttabledb.GetString(), 
                    (m_pfkinfo + m_index[lvi.lParam])->m_parenttable.GetString());
            }

            //add column name and filter text
            if(condition.CompareI("(null)"))
            {
                filter.AddSprintf("`%s` like '%%%s%%'", 
                    temp.GetString(),
                    condition.GetString());
            }
            else
            {
                filter.AddSprintf("`%s` IS NULL", temp.GetString());
            }

            j++;
        }
    }

    if(!j)
    {
        filter.Clear();
    }
    else
    {
        filter.Add(")");
    }
    
    //check for last item in the list view and add it
    if(ListView_GetCheckState(m_hwndcolumns, i) && m_compositefilter.GetLength())
    {
        if(filter.GetLength())
        {
            filter.Add(" and ");
        }
        else
        {
            filter.Add(" where ");
        }

        filter.AddSprintf(" %s ", m_compositefilter.GetString());
    }
}

//Function which provides the data to be displayed in the grid
void 
FKDropDown::OnGVDispInfo(WPARAM wparam)
{
    wyUInt32    size;
    GVDISPINFO* disp = (GVDISPINFO*)wparam;

    m_text.Clear();
    m_buttontext.Clear();
    size = 0;

    //precaution
    if(!m_myres)
    {
        return;
    }

    //if it is not null
    if((*(m_myrows + disp->nRow))[disp->nCol])
    {
        //get the column size 
        GetUtf8ColLength((*(m_myrows + disp->nRow)), m_myres->field_count, disp->nCol, &size, m_pmdi->m_ismysql41);

        //if the column is bit type, then we need to convert it into binary representation
        if(m_myres->fields[disp->nCol].type == MYSQL_TYPE_BIT)
        {
            ConvertStringToBinary((*(m_myrows + disp->nRow))[disp->nCol], size, m_myres->fields[disp->nCol].length, &m_text);
        }
        else
        {
            //otherwise check for NULL data
            if(memchr((*(m_myrows + disp->nRow))[disp->nCol], NULL, size))
            {
                m_text.SetAs(BINARY_DATA);
            }
        }
    }
    else
    {
        m_text.SetAs("(NULL)");
    }

    //set the text
    disp->text = m_text.GetLength() ? (wyChar*)m_text.GetString() : (*(m_myrows + disp->nRow))[disp->nCol];

    //if it is a blob column, then we need to calculate and display the size also
    if(CustomGrid_GetColumnLongValue(m_hwndgrid, disp->nCol))
    {
        if((wyInt32)(size / 1024) > 0) 
        {
            m_buttontext.Sprintf("%dKB", (wyInt32)(size / 1024));
        }
        else
        {
            m_buttontext.Sprintf("%dB", size);
        }

        disp->pszButtonText = (wyChar*)m_buttontext.GetString();
    }
}

//function executes the query
wyBool 
FKDropDown::ExecuteQuery(wyBool isignorefilter)
{
    wyUInt32        tbhid;
    wyString        query;
    HANDLE          hthread;
    FKThreadStruct  fkts = {0};
    POINT           pt;
    RECT            rect;
    wyInt64         startrow = 0, rowcount = 0;

    //get the start row and row count
    startrow = GetStartRow();
    rowcount = GetRowCount();

    //set it in the fields
    query.Sprintf("%d", startrow);
    SetWindowText(GetDlgItem(m_hwnd, IDC_ROWSTART), query.GetAsWideChar());
    query.Sprintf("%d", rowcount);
    SetWindowText(GetDlgItem(m_hwnd, IDC_ROWCOUNT), query.GetAsWideChar());
    query.Clear();

    GetCursorPos(&pt);
    GetWindowRect(GetDlgItem(m_hwnd, IDC_REFRESHFK), &rect);

    if(!PtInRect(&rect, pt))
    {
        SetCursor(LoadCursor(NULL, IDC_WAIT));
    }

    SetCapture(GetDlgItem(m_hwnd, IDC_REFRESHFK));

    //save the prev seleced row and column, to persist after the query execution
    m_prevselcol = CustomGrid_GetCurSelCol(m_hwndgrid);
    m_prevselrow = CustomGrid_GetCurSelRow(m_hwndgrid);

    //form the query
    FormQuery(query, isignorefilter);

    //destroy the result set if any and delete all the columns in the custom grid
    if(m_myres)
    {
        delete[] m_myrows;
        m_myrows = NULL;
        m_pmdi->m_tunnel->mysql_free_result(m_myres);
        m_myres = NULL;
        CustomGrid_DeleteAllColumn(m_hwndgrid, wyTrue);
        CustomGrid_SetMaxRowCount(m_hwndgrid, 0);
    }
    
    //form the structure for execution thread
    fkts.m_pquery = &query;
    fkts.m_pfk = this;
    fkts.m_hevent = CreateEvent(NULL, TRUE, FALSE, NULL);

    //reset the thread stop status
    ThreadStopStatus(0);

    //begin execution thread
    hthread = (HANDLE)_beginthreadex(NULL, 0, FKDropDown::QueryThread, &fkts, 0, &tbhid);

    //change refresh to stop
    SetWindowText(GetDlgItem(m_hwnd, IDC_REFRESHFK), _(L"&Stop"));

    //handle the messages till the event occures
    HandleMsgs(fkts.m_hevent, wyFalse);
    
    //cleanup
    if(hthread)
    {
        CloseHandle(hthread);
    }

    if(fkts.m_hevent)
    {
        CloseHandle(fkts.m_hevent);
    }

    //perform post execution events
    PostExecution();
    return m_myres ? wyTrue : wyFalse;
}

//thread procedure to execute the query
unsigned __stdcall	
FKDropDown::QueryThread(LPVOID lpparam)
{
    FKThreadStruct* pfkts = (FKThreadStruct*)lpparam;
    wyInt32         count, i = 0;
    MYSQL_ROW       myrow;
    wyBool          istunnel = pfkts->m_pfk->m_pmdi->m_tunnel->IsTunnel() ? wyTrue : wyFalse;

    //initialize mysql thread specific variables. as per mysql ref manual, this should be called for each created thread
    if(istunnel == wyFalse)
    {
        mysql_thread_init();
    }

    //reset the stop status in the mdi level and execute the query. after execution reset the status again
    pfkts->m_pfk->m_pmdi->m_stopquery = 0;
    pfkts->m_pfk->m_myres = ExecuteAndGetResult(pfkts->m_pfk->m_pmdi, pfkts->m_pfk->m_pmdi->m_tunnel, &pfkts->m_pfk->m_pmdi->m_mysql, *pfkts->m_pquery, wyTrue, wyFalse, wyTrue, 
															false, false, wyFalse, &pfkts->m_pfk->m_pmdi->m_stopquery);
    pfkts->m_pfk->m_pmdi->m_stopquery = 0;

    //check whether the thread is stoped
    if(pfkts->m_pfk->ThreadStopStatus())
    {
        //if yes, free any result obtained
        if(pfkts->m_pfk->m_myres)
        {
            pfkts->m_pfk->m_pmdi->m_tunnel->mysql_free_result(pfkts->m_pfk->m_myres);
            pfkts->m_pfk->m_myres = NULL;
        }

        //signal the thread finish
        SetEvent(pfkts->m_hevent);

        //free the mysql thread specific variables
        if(istunnel == wyFalse)
        {
            mysql_thread_end();
        }

        return 0;
    }

    //if result is null, show error
    if(pfkts->m_pfk->m_myres == NULL)
    {
        ShowMySQLError(pfkts->m_pfk->m_hwnd, pfkts->m_pfk->m_pmdi->m_tunnel, &pfkts->m_pfk->m_pmdi->m_mysql, pfkts->m_pquery->GetString());

        //signal the thread finish
        SetEvent(pfkts->m_hevent);

        //free the mysql thread specific variables
        if(istunnel == wyFalse)
        {
            mysql_thread_end();
        }

        return 0;
    }

    //if there are rows, then allocate row array
    if((count = pfkts->m_pfk->m_pmdi->m_tunnel->mysql_num_rows(pfkts->m_pfk->m_myres)))
    {
        pfkts->m_pfk->m_myrows = new MYSQL_ROW[count];
    }

    //seek through result set and store the pointer in the array
    while((myrow =  pfkts->m_pfk->m_pmdi->m_tunnel->mysql_fetch_row(pfkts->m_pfk->m_myres)))
    {
        //check thread stop status
        if(pfkts->m_pfk->ThreadStopStatus())
        {
            //if yes, free any result obtained
            if(pfkts->m_pfk->m_myres)
            {
                pfkts->m_pfk->m_pmdi->m_tunnel->mysql_free_result(pfkts->m_pfk->m_myres);
                pfkts->m_pfk->m_myres = NULL;
            }

            //delete the rows created
            delete[] pfkts->m_pfk->m_myrows;
            pfkts->m_pfk->m_myrows = NULL;

            //signal the thread finish
            SetEvent(pfkts->m_hevent);

            //free the mysql thread specific variables
            if(istunnel == wyFalse)
            {
                mysql_thread_end();
            }

            return 0;
        }

        //assign the row
        pfkts->m_pfk->m_myrows[i++] = myrow;
    }

    //signal the thread finish
    SetEvent(pfkts->m_hevent);

    //free the mysql thread specific variables
    if(istunnel == wyFalse)
    {
        mysql_thread_end();
    }

    return 1;
}

//function is called after query execution
wyInt32 
FKDropDown::PostExecution()
{
    wyInt32 selcol = -1, checkstate;
    wyInt32 count = 0;

    if(m_myres)
    {
        //if it is done while initializing, then initialize the filter columns
        if(m_isinitcompleted == wyFalse)
        {
            InitializeColumnsDropDown();
        }

        //get the number of rows
        count = m_pmdi->m_tunnel->mysql_num_rows(m_myres);

        //create the columns
        selcol = CreateColumns();

        //set the maximum row count
        CustomGrid_SetMaxRowCount(m_hwndgrid, count);
    }

    //if there are rows
    if(count)
    {
        if(m_prevselrow == -1)
        {
            m_prevselrow = 0;
        }
        else if(m_prevselrow >= count)
        {
            m_prevselrow = count - 1;
        }

        if(m_prevselcol == -1)
        {
            m_prevselcol = selcol;
        }
        else
        {
            m_prevselcol = m_prevselcol >= m_myres->field_count ? selcol : m_prevselcol;
        }

        //set the row and column selection persisting the previous selection
        CustomGrid_SetCurSelection(m_hwndgrid, m_prevselrow, m_prevselcol, wyTrue);
        CustomGrid_EnsureVisible(m_hwndgrid, m_prevselrow, m_prevselcol);
        EnableWindow(GetDlgItem(m_hwnd, IDOK), TRUE);
        SetFocus(m_hwndgrid);
    }
    else
    {
        //else disable the insert button
        EnableWindow(GetDlgItem(m_hwnd, IDOK), FALSE);
    }

    //enable the navigation controls based on the number of rows and check states
    checkstate = SendMessage(GetDlgItem(m_hwnd, IDC_LIMITROWS), BM_GETCHECK, 0, 0);
    EnableWindow(GetDlgItem(m_hwnd, IDC_NEXT), (checkstate && GetRowCount() <= count) ? TRUE : FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_PREV), (checkstate && GetStartRow() > 0) ? TRUE : FALSE);
    SetWindowText(GetDlgItem(m_hwnd, IDC_REFRESHFK), _(L"Refre&sh"));
    EnableWindow(GetDlgItem(m_hwnd, IDC_REFRESHFK), TRUE);

    //set the trhead status to stop
    ThreadStopStatus(1);
    ReleaseCapture();
    InvalidateRect(m_hwndgrid, NULL, TRUE);
    UpdateWindow(m_hwndgrid);
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    return count;
}

//function access a shared resource to get/set the value
wyInt32
FKDropDown::ThreadStopStatus(wyInt32 value)
{
    wyInt32 temp;

    EnterCriticalSection(&m_cs);

    if(value < 0)
    {
        temp = m_threadstopstatus;
    }
    else
    {
        m_threadstopstatus = temp = value;
    }

    LeaveCriticalSection(&m_cs);

    return temp;
}

//function create the columns
wyInt32 
FKDropDown::CreateColumns()
{
    wyInt32     i, colwidth, selectedcolumn = -1;
    GVCOLUMN    gvcol;
    wyString    table, column, db;
    wyBool      isretaincolumnwidth = IsRetainColumnWidth();
    LVITEM      lvi = {0};
    wyWChar     buffer[256];

    //get the item of our interest
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText = buffer;
    lvi.cchTextMax = 255;
    lvi.iItem = m_item;
    ListView_GetItem(m_hwndcolumns, &lvi);

    //create columns by looing through thefield list
    for(i = 0; i < m_myres->field_count; ++i)
    {
        colwidth = 0;
        table.SetAs(m_myres->fields[i].table, m_pmdi->m_ismysql41);
        column.SetAs(m_myres->fields[i].name, m_pmdi->m_ismysql41);
        db.SetAs(GetDbName(i), m_pmdi->m_ismysql41);
        memset(&gvcol, 0, sizeof(GVCOLUMN));
        gvcol.uIsReadOnly = wyTrue;
        gvcol.cchTextMax = strlen(m_myres->fields[i].name);
        gvcol.text = m_myres->fields[i].name;

        //specify the button style
        if((m_myres->fields[i].type >= FIELD_TYPE_TINY_BLOB) && (m_myres->fields[i].type <= FIELD_TYPE_BLOB))
        {
            gvcol.mask |= GVIF_TEXTBUTTON;
		} 
        else
        {
            gvcol.mask = GVIF_TEXT;
        }

        //set the column alignment
        gvcol.fmt = GetColAlignment(&m_myres->fields[i]);

        //get the column width
        if(isretaincolumnwidth == wyTrue)
			colwidth = GetColumnWidthFromFile(&db, &table ,&column);
				
        //set the column width and insert the column
        gvcol.cx = (colwidth > 0)? colwidth : GetColWidth(m_hwndgrid, &m_myres->fields[i], i); 
        CustomGrid_InsertColumn(m_hwndgrid, &gvcol);

        //if column is button type, set a value associated with it so that we can identify the blob
        if(gvcol.mask & GVIF_TEXTBUTTON)
        {
            CustomGrid_SetColumnLongValue(m_hwndgrid, i, (LPARAM)1);
        }
        else
        {
            CustomGrid_SetColumnLongValue(m_hwndgrid, i, (LPARAM)0);
        }

        //set the selected column
        if(selectedcolumn == -1 && !wcsicmp(lvi.pszText, column.GetAsWideChar()) && 
            !(m_pfkinfo + m_index[lvi.lParam])->m_parenttable.CompareI(m_myres->fields[i].table) &&
            !(m_pfkinfo + m_index[lvi.lParam])->m_parenttabledb.CompareI(GetDbName(i))) 
        {
            selectedcolumn = i;
        }
    }

    return selectedcolumn;
}

//function initializes the column drop down
void 
FKDropDown::InitializeColumnsDropDown()
{
    LVITEM              lvi;
    wyInt32             i, j, k = -1, t = 0, count;
    wyString            temp;
    wyWChar*            text;
    RelTableFldInfo*    prelinfo = NULL;        

    //get the list view handle of the drop down and set the check box style
    m_hwndcolumns = (HWND)SendMessage(GetDlgItem(m_hwnd, IDC_COLUMN_FILTER), BDM_GETLISTHANDLE, 0, 0);
    ListView_SetExtendedListViewStyle(m_hwndcolumns, LVS_EX_CHECKBOXES);
    
    //set the drop down style
    SendMessage(GetDlgItem(m_hwnd, IDC_COLUMN_FILTER), BDM_SETDROPSTYLE, (WPARAM)BDS_UP, 0);

    //find the column currusponding to the fk column
    for(i = 0; i < m_indexcount; ++i)
    {
        for(t = 0, prelinfo = (RelTableFldInfo*)(m_pfkinfo + m_index[i])->m_fkeyfldlist->GetFirst(); prelinfo; 
            ++t, prelinfo = (RelTableFldInfo*)prelinfo->m_next)
        {
            if(!m_fkcolumnname.CompareI(prelinfo->m_tablefld))
            {
                k = i;
                break;
            }
        }

        if(k != -1)
        {
            break;
        }
    }

    lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iSubItem = 0;

    count = m_pmdi->m_tunnel->mysql_num_rows(m_myres);

    //loop through the field list, creating items in the list view
    for(i = 0; i < m_myres->field_count; ++i)
    {
        lvi.iItem = i;
        temp.SetAs(m_myres->fields[i].name);
        text = temp.GetAsWideChar();
        lvi.pszText = text;
        lvi.cchTextMax = wcslen(text);

        //set the index as param
        for(j = 0; j < m_indexcount; ++j)
        {
            if(!(m_pfkinfo + m_index[j])->m_parenttable.CompareI(m_myres->fields[i].table) &&
                !(m_pfkinfo + m_index[j])->m_parenttabledb.CompareI(GetDbName(i)))
            {
                lvi.lParam = j;
                break;
            }
        }

        //find the pk column curresponding to the fk column and save its index
        if(m_item == -1 && !(m_pfkinfo + m_index[k])->m_parenttable.CompareI(m_myres->fields[i].table) && 
            !((RelTableFldInfo*)(*(m_pfkinfo + m_index[k])->m_pkeyfldlist)[t])->m_tablefld.CompareI(temp) &&
            !(m_pfkinfo + m_index[k])->m_parenttabledb.CompareI(GetDbName(i)))
        {
            m_item = i;
        }

        //insert the item
        ListView_InsertItem(m_hwndcolumns, &lvi);

        if(!count)
        {
            ListView_SetCheckState(m_hwndcolumns, i, TRUE);
        }
    }

    if(count)
    {
        ListView_SetCheckState(m_hwndcolumns, m_item, TRUE);
    }

    lvi.iItem = i;
    lvi.pszText = _(L"(Filter on values already entered)");
    lvi.cchTextMax = wcslen(_(L"(Filter on values already entered)"));
    lvi.lParam = -1;
    ListView_InsertItem(m_hwndcolumns, &lvi);
    ListView_SetCheckState(m_hwndcolumns, i, m_filteronvaluesentered);
}

//function to handle click on next/prev
void 
FKDropDown::OnNextOrPrev(HWND hwnd)
{
    wyInt64     startrow = 0, rowcount = 0;
    wyString    temp;
    
    //get the start row and row count
    startrow = GetStartRow();
    rowcount = GetRowCount();

    //increase/decrese start row based on the handle passed in
    if(hwnd == GetDlgItem(m_hwnd, IDC_NEXT))
    {
        startrow += rowcount;
    }
    else
    {
        startrow -= rowcount;
    }

    if(startrow < 0)
    {
        startrow = 0;
    }

    //set the value in the text fields
    temp.Sprintf("%d", startrow);
    SetWindowText(GetDlgItem(m_hwnd, IDC_ROWSTART), temp.GetAsWideChar());
    temp.Sprintf("%d", rowcount);
    SetWindowText(GetDlgItem(m_hwnd, IDC_ROWCOUNT), temp.GetAsWideChar());

    //execute the query
    ExecuteQuery(wyTrue);
}

//function gets the start row
wyInt64
FKDropDown::GetStartRow()
{
    wyWChar     buffer[256];
    wyString    temp("0");
    
    GetWindowText(GetDlgItem(m_hwnd, IDC_ROWSTART), buffer, 255);

    if(wcslen(buffer))
    {
        temp.SetAs(buffer);
    }

    return temp.GetAsInt64();
}

//function gets the row count
wyInt64
FKDropDown::GetRowCount()
{
    wyWChar     buffer[256];
    wyString    temp("25");
    
    GetWindowText(GetDlgItem(m_hwnd, IDC_ROWCOUNT), buffer, 255);

    if(wcslen(buffer))
    {
        temp.SetAs(buffer);
    }

    return temp.GetAsInt64();
}

//get the limit clause
void
FKDropDown::GetLimits(wyString& limit)
{
    wyInt32     size;
    wyWChar*    buffer;
    wyString    temp;

    limit.Clear();

    //if the limit rows is checked
    if(SendMessage(GetDlgItem(m_hwnd, IDC_LIMITROWS), BM_GETCHECK, (WPARAM)TRUE, 0) == BST_CHECKED)
    {
        size = GetWindowTextLength(GetDlgItem(m_hwnd, IDC_ROWSTART)) + 1;
        buffer = new wyWChar[size + 1];
        GetWindowText(GetDlgItem(m_hwnd, IDC_ROWSTART), buffer, size);
        temp.SetAs(buffer);
        delete[] buffer;
        limit.AddSprintf(" limit %s, ", temp.GetString());
        size = GetWindowTextLength(GetDlgItem(m_hwnd, IDC_ROWCOUNT)) + 1;
        buffer = new wyWChar[size + 1];
        GetWindowText(GetDlgItem(m_hwnd, IDC_ROWCOUNT), buffer, size);
        temp.SetAs(buffer);
        delete[] buffer;
        limit.AddSprintf("%s", temp.GetString());
    }
}

//function gets the control rectangles that is used while resizing the dialog box
void
FKDropDown::GetCtrlRects()
{
    RECT    rect;
    wyInt32 i, count;
    HWND    hwnd;

    //ctrlid    size-x     size-y
    wyInt32 ctrlid[] = {
        IDC_LIMITROWS, 0, 0,
        IDC_FIRSTROW, 0, 0,
        IDC_PREV, 0, 0,
        IDC_ROWSTART, 0, 0,
        IDC_NEXT, 0, 0,
        IDC_ROWS, 0, 0,
        IDC_ROWCOUNT, 0, 0,
        IDC_NAVGROUP, 0, 0,
        IDC_REFRESHFK, 0, 0,
        IDC_FILTERPROMPT, 0, 0,
        IDC_SEARCHTEXT, 1, 0,
        IDC_COLUMN_FILTER, 0, 0,
        IDOK, 0, 0,
        IDC_SIZEGRIP, 0, 0,
        IDCANCEL, 0, 0
    };

    count = sizeof(ctrlid)/sizeof(ctrlid[0]);

    //store the control handles and related infos in the linked list
    for(i = 0; i < count; i += 3)
    {
        hwnd = GetDlgItem(m_hwnd, ctrlid[i]);
        GetWindowRect(hwnd, &rect);
        MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rect, 2);
        m_controllist.Insert(new DlgControl(hwnd, 
                                            ctrlid[i], 
                                            &rect, 
                                            ctrlid[i + 1] ? wyTrue : wyFalse, 
                                            ctrlid[i + 2] ? wyTrue : wyFalse));
    }

    GetWindowRect(m_hwndgrid, &rect);
    MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rect, 2);
    m_controllist.Insert(new DlgControl(m_hwndgrid, 0, &rect, wyTrue, wyTrue));
}

//function positions the controls in the dialog box
void
FKDropDown::PositionCtrls()
{
    DlgControl* pdlgctrl;
    wyInt32     leftpadding, toppadding, rightpadding, bottompadding, width, height;
    wyInt32     x, y;
    RECT        rect;
    HDWP        hdefwp;
    wyBool      ismovedown = wyFalse;

    GetClientRect(m_hwnd, &rect);

    //BeginDeferWindowPos is used to make the control positioning atomic
    hdefwp = BeginDeferWindowPos(m_controllist.GetCount() + 1);

    //iterate throught the control lists
    for(pdlgctrl = (DlgControl*)m_controllist.GetFirst(); pdlgctrl;
        pdlgctrl = (DlgControl*)pdlgctrl->m_next)
    {
        if(pdlgctrl->m_id == IDC_FILTERPROMPT)
        {
            ismovedown = wyTrue;
        }

        leftpadding = pdlgctrl->m_rect.left - m_dlgrect.left;
        toppadding = pdlgctrl->m_rect.top - m_dlgrect.top;
        rightpadding = m_dlgrect.right - pdlgctrl->m_rect.right;
        bottompadding = m_dlgrect.bottom - pdlgctrl->m_rect.bottom;
        width = pdlgctrl->m_rect.right - pdlgctrl->m_rect.left;
        height = pdlgctrl->m_rect.bottom - pdlgctrl->m_rect.top;
        
        if(pdlgctrl->m_issizex == wyFalse)
        {
            if(ismovedown == wyTrue && pdlgctrl->m_id != IDC_FILTERPROMPT)
            {
                x = rect.right - rightpadding - width;
            }
            else
            {
                x = leftpadding;
            }
        }
        else
        {
            x = leftpadding;
            width = rect.right - rightpadding - leftpadding;
        }

        if(ismovedown == wyTrue && pdlgctrl->m_id)
        {
            y = rect.bottom - bottompadding - height;
        }
        else
        {
            y = toppadding;
        }

        if(pdlgctrl->m_issizey == wyTrue)
        {
            height = rect.bottom - bottompadding - toppadding;
        }

        //change the control position
        hdefwp = DeferWindowPos(hdefwp, pdlgctrl->m_hwnd, NULL, x, y, width, height, SWP_NOZORDER);
    }

    //finish the operation and apply changes
    EndDeferWindowPos(hdefwp);
    InvalidateRect(m_hwndgrid, NULL, TRUE);
    UpdateWindow(m_hwndgrid);
}

//restrict the minimum size
void
FKDropDown::OnWMSizeInfo(LPARAM lparam)
{
    MINMAXINFO* pminmax = (MINMAXINFO*)lparam;

    pminmax->ptMinTrackSize.x = m_wndrect.right - m_wndrect.left;
    pminmax->ptMinTrackSize.y = m_wndrect.bottom - m_wndrect.top;
}

//function open the blob 
void
FKDropDown::OnGVButtonClick()
{
    wyInt32             row, col;
    INSERTUPDATEBLOB    iub = {0};
    BlobMgmt            bmgmt;

    row = CustomGrid_GetCurSelRow(m_hwndgrid);
    col = CustomGrid_GetCurSelCol(m_hwndgrid);

    //fill the data based on the column value
    if((*(m_myrows + row))[col])
    {
        GetUtf8ColLength((*(m_myrows + row)), m_myres->field_count, col, (wyUInt32*)&iub.m_datasize, m_pmdi->m_ismysql41);
        iub.m_data = (*(m_myrows + row))[col];
    }
    else
    {
        iub.m_data = NULL;
        iub.m_datasize = 0;
        iub.m_isnull = wyTrue;
    }

    //set the blob flag
    if(m_myres->fields[col].charsetnr == 63 && (m_myres->fields[col].type == MYSQL_TYPE_LONG_BLOB ||
		     m_myres->fields[col].type == MYSQL_TYPE_MEDIUM_BLOB ||
		     m_myres->fields[col].type == MYSQL_TYPE_TINY_BLOB || m_myres->fields[col].type == MYSQL_TYPE_BLOB))
	{
        iub.m_isblob = wyTrue;
	}
	else
    {
        iub.m_isblob = wyFalse;
    }

    //show the blob dialog
    bmgmt.Create(m_hwnd, &iub, wyFalse);
}

//function reads/write from/to ini to/from the last item in the list view for persisting the user selection
wyInt32
FKDropDown::FilterOnValuesEntered(wyInt32 value)
{
    wyInt32     ret = 1;
    //uncomment the bellow code to persist the value;
    /*wyWChar     directory[MAX_PATH];
    wyWChar*    lpfileport = NULL;
    wyString    dirstr;

    if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyFalse)
    {
        return value < 0 ? 1 : 0;
    }

    dirstr.SetAs(directory);

    if(value < 0)
    {
        ret = (wyIni::IniGetInt(FKDROPDOWN_SECTION, "FilterOnValuesEntered", 0, dirstr.GetString())) ? 1 : 0;
    }
    else
    {
        ret = wyIni::IniWriteString(FKDROPDOWN_SECTION, "FilterOnValuesEntered", value ? "1" : "0", dirstr.GetString()) == wyTrue ? 1 : 0;
    }*/
    
    return ret;
}

//function creates the FK array for the table, total elements in the array is one more than the actual
//so while looping through upper bound should be count - 1
wyInt32
FKDropDown::CreateFKList(FKEYINFOPARAM** ppfkinfo, const wyChar* dbname, const wyChar* tablename, Tunnel* ptunnel, wyString* pcreatetable)
{
    wyInt32     i = -1, j, col;
    wyString    key;
    RelTableFldInfo *prelinfo;

    *ppfkinfo = NULL;
   
    do
    {
        ++i;

        //alocate/reallocate the array
        if(*ppfkinfo)
        {
            *ppfkinfo = (FKEYINFOPARAM*)realloc(*ppfkinfo, (i + 1) * sizeof(FKEYINFOPARAM));
        }
        else
        {
            *ppfkinfo = (FKEYINFOPARAM*)malloc(sizeof(FKEYINFOPARAM));
        }

        //call the constructor for allocated space, required to initialize the members
        new(*ppfkinfo + i) FKEYINFOPARAM;

        //create the lists for fk and pk fields
        (*ppfkinfo + i)->m_fkeyfldlist = new List;
        (*ppfkinfo + i)->m_pkeyfldlist = new List;
    }while(GetForeignKeyInfo(pcreatetable, key, i + 1, *ppfkinfo + i, dbname) && key.GetLength());

    ++i;

    //now for http connection, we are checking if parent tables are from same db
    if(ptunnel->IsTunnel())
    {
        for(j = 0; j < i - 1; ++j)
        {
            for(col = 0, prelinfo = (RelTableFldInfo*)(*ppfkinfo + j)->m_fkeyfldlist->GetFirst();
                prelinfo;
                ++col, prelinfo = (RelTableFldInfo*)prelinfo->m_next)
            {
                if(FKDropDown::IsParentFromDiffDB(*ppfkinfo, i - 1, j, col) == wyTrue)
                {
                    FKDropDown::DeleteFKList(ppfkinfo, &i);
                    break;
                }
            }
        }
    }

    return i;
}

wyBool 
FKDropDown::IsParentFromDiffDB(FKEYINFOPARAM* pfkinfo, wyInt32 count, wyInt32 index, wyInt32 col)
{
    wyInt32 i;
    RelTableFldInfo *prelinfo, *pcolinfo;

    pcolinfo = (RelTableFldInfo*)(*((pfkinfo + index)->m_fkeyfldlist))[col];

    for(i = 0; i < count; ++i)
    {
        if(i == index)
        {
            continue;
        }

        for(prelinfo = (RelTableFldInfo*)(pfkinfo + i)->m_fkeyfldlist->GetFirst();
            prelinfo;
            prelinfo = (RelTableFldInfo*)prelinfo->m_next)
        {
            if(!prelinfo->m_tablefld.Compare(pcolinfo->m_tablefld) && (pfkinfo + i)->m_parenttabledb.Compare((pfkinfo + col)->m_parenttabledb))
            {
                return wyTrue;
            }
        }
    }

    return wyFalse;
}

//function deletes the fk list
void 
FKDropDown::DeleteFKList(FKEYINFOPARAM** pfkinfo, wyInt32* count)
{
    wyInt32             i;
    RelTableFldInfo*    prelinfo = NULL;

    //loop through the array
    for(i = 0; pfkinfo && i < *count; ++i)
    {
        //free the fk list for the item
        for(prelinfo = (RelTableFldInfo*)(*pfkinfo + i)->m_fkeyfldlist->GetFirst(); prelinfo; 
            prelinfo = (RelTableFldInfo*)(*pfkinfo + i)->m_fkeyfldlist->GetFirst())
        {
            (*pfkinfo + i)->m_fkeyfldlist->Remove(prelinfo);
            delete prelinfo;
        }

        //free the pk list for the item
        for(prelinfo = (RelTableFldInfo*)(*pfkinfo + i)->m_pkeyfldlist->GetFirst(); prelinfo; 
            prelinfo = (RelTableFldInfo*)(*pfkinfo + i)->m_pkeyfldlist->GetFirst())
        {
            (*pfkinfo + i)->m_pkeyfldlist->Remove(prelinfo);
            delete prelinfo;
        }

        //release fk and pk field lists
        delete (*pfkinfo + i)->m_fkeyfldlist;
        delete (*pfkinfo + i)->m_pkeyfldlist;

        //explicit call to the destructor 
        (*pfkinfo + i)->m_constraint.~wyString();
        (*pfkinfo + i)->m_parenttable.~wyString();
        (*pfkinfo + i)->m_parenttabledb.~wyString();
    }

    //finally free the array
    if(*pfkinfo)
    {
        free(*pfkinfo);
        *pfkinfo = NULL;
    }

    *count = 0;
}

wyInt32 FKLookupAndInsert(DataView* pdv, BEHAVIOR_EVENT_PARAMS* params)
{
    wyInt32     image;
    MDIWindow*  pmdi = GetActiveWin();
    
    if(pmdi->m_executing == wyTrue)
    {
        if((image = pmdi->m_pctabmodule->GetActiveTabImage()) == IDI_DATASEARCH)
        {
            MessageBox(pmdi->m_pctabmodule->m_hwnd, _(L"Please stop the search to retreive parent table rows"), pGlobals->m_appname.GetAsWideChar(), MB_ICONINFORMATION | MB_OK);
		    return IDCANCEL;
        }
    }
	if(pmdi->m_pingexecuting == wyTrue)
	{
		return IDCANCEL;
	}

    FKDropDown ob(pdv, params);
    return ob.Create();
}