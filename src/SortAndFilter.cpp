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

Author: Vishal P.R, Janani SriGuha

*********************************************/

#include "SortAndFilter.h"
#include "FrameWindow.h"
#include "BlobMgmt.h"
#include "EditorFont.h"
#ifndef COMMUNITY
#include "SCIFormatter.h"
#endif

//constructor
SortAndFilter::SortAndFilter(wyBool isclient, MySQLDataEx* data)
{
    //initialize the members
    m_wnd = data->m_pmdi;
    m_isfilter = wyFalse;
    m_isclient = isclient;
    m_data = data;
	m_querybuilder = NULL;
    Initialize();

    //we are hardcoding the number of sort/filter columns available
    //for client side it is one and server side it is five
    if(isclient == wyFalse)
    {
        m_sortcolumns = 5;
        m_filtercolumns = 5;
    }
    else
    {
        m_sortcolumns = 1;
        m_filtercolumns = 1;
    }

    //allocate sort and filter data
    m_sort = new SORT_DATA[m_sortcolumns];
    m_filter = new FILTER_DATA[m_filtercolumns];

    //reset sort and filter
    ResetSort();
    ResetFilter();
}

//destructor
SortAndFilter::~SortAndFilter()
{
    //free the original array
    FreeOrigRowArray(wyTrue);

    //delete sort and filter
    delete[] m_sort;
    delete[] m_filter;
}

const SortAndFilter& 
SortAndFilter::operator=(const SortAndFilter& operand)
{
    m_porigrowarray = operand.m_porigrowarray;
    return *this;
}

void
SortAndFilter::Initialize()
{
    m_porigrowarray = NULL;
    m_ignoredcheckcount = 0;
}

//function to open custom filter dialog box
wyInt32
SortAndFilter::OpenFilterDialog(HWND hwndparent, wyInt32 colno, wyChar *data, wyUInt32 datalen, RECT* prect)
{
    //save the arguments in members. we need them in the dialog box operation
    m_hwndparent = hwndparent;
    m_celldata = data;
    m_celldatalen = datalen;
    m_colno = colno;
    m_prelrect = prect;

    //show the dialog
    return DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_CUSTOMFILTER),
            m_hwndparent, SortAndFilter::FilterWndProc, (LPARAM)this);

}

//dialog box callback proc
INT_PTR CALLBACK
SortAndFilter::FilterWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{	
	wyString        filtercon;
	wyString        filtertxt;
    SortAndFilter*  sortandfilter;
	RECT			rect, rectLastItem;
	LONG			height;

	sortandfilter = (SortAndFilter*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
		case WM_CTLCOLORSTATIC:
		{
			int ctrlId = GetDlgCtrlID((HWND)lparam);

			if (IDC_SHOWSQL == ctrlId || IDC_HIDESQL == ctrlId)
				return SetAsLink((HDC)wparam);
			return 0;
		}

        //initialize the dialog
	    case WM_INITDIALOG:
            //associate the lparam as the user data
            sortandfilter = (SortAndFilter*)lparam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);

            //save the window handle and localize the window
            sortandfilter->m_hwnd = hwnd;
            LocalizeWindow(hwnd);
		
            //initialize. if it returns false for some reasone, we end dialog with cancel
            if(sortandfilter->InitDialog() == wyFalse)
		    {
			    EndDialog(hwnd, IDCANCEL);
		    }
			PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);

            break;

		case WM_INITDLGVALUES:
			sortandfilter->OnWMInitdlgValues(hwnd);
			break;

        //command handling
	    case WM_COMMAND:
			int wNotifyCode;

			wNotifyCode = HIWORD(wparam);
			if (NULL != sortandfilter->m_querybuilder && (EN_KILLFOCUS == wNotifyCode || CBN_KILLFOCUS == wNotifyCode))
			{
				wyString query;
				HWND hwndPreview = GetDlgItem(hwnd, IDC_PREVIEW);

				// Refresh the sql query as user moves across filter values
				sortandfilter->ProcessFilter();
				sortandfilter->SetFilterString(wyTrue);
				sortandfilter->m_querybuilder->GetQuery(query);

				SendMessage(hwndPreview, SCI_SETREADONLY, (WPARAM)FALSE, (LPARAM)0);
				SendMessage(hwndPreview, SCI_SETTEXT, (WPARAM)query.GetLength(), (LPARAM)query.GetString());
				//Format query
#ifndef COMMUNITY
				Format(hwndPreview, IsStacked(), GetLineBreak() ? wyFalse : wyTrue, FORMAT_ALL_QUERY, GetIndentation());
#endif
				SendMessage(hwndPreview, SCI_SETSELECTIONSTART, (WPARAM)0, 0);
				SendMessage(hwndPreview, SCI_SETSELECTIONEND, (WPARAM)0, 0);
				SendMessage(hwndPreview, SCI_SETFIRSTVISIBLELINE, 0, 0);
				SendMessage(hwndPreview, SCI_SETREADONLY, (WPARAM)TRUE, (LPARAM)0);
			}
			else if (STN_CLICKED == wNotifyCode || BN_CLICKED == wNotifyCode)
			{
				switch (LOWORD(wparam))
				{
					//close the dialog
				case IDCANCEL:
					EndDialog(hwnd, IDCANCEL);
					return TRUE;

					//handle ok press
				case IDOK:
					//process the filter
					if (sortandfilter->ProcessFilter() == wyTrue)
					{
						EndDialog(hwnd, IDOK);
					}
					else
					{
						EndDialog(hwnd, IDCANCEL);
					}

					return TRUE;
				case IDC_SHOWSQL:
					ShowWindow(GetDlgItem(hwnd, IDC_SHOWSQL), SW_HIDE);
					ShowWindow(GetDlgItem(hwnd, IDC_HIDESQL), SW_SHOW);
					GetWindowRect(GetDlgItem(hwnd, IDC_PREVIEW), &rect);
					height = rect.bottom - rect.top;
					GetWindowRect(GetDlgItem(hwnd, IDOK), &rectLastItem);
					height += rect.top - rectLastItem.bottom;
					GetWindowRect(hwnd, &rect);
					SetWindowPos(hwnd, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top + height, SWP_NOZORDER | SWP_NOMOVE);
					// Force a refresh of Preview
					SetFocus(GetDlgItem(hwnd, 100));
					SetFocus(GetDlgItem(hwnd, IDC_PREVIEW));
					return TRUE;

				case IDC_HIDESQL:
					ShowWindow(GetDlgItem(hwnd, IDC_HIDESQL), SW_HIDE);
					ShowWindow(GetDlgItem(hwnd, IDC_SHOWSQL), SW_SHOW);
					GetWindowRect(GetDlgItem(hwnd, IDC_PREVIEW), &rect);
					height = rect.bottom - rect.top;
					GetWindowRect(GetDlgItem(hwnd, IDOK), &rectLastItem);
					height += rect.top - rectLastItem.bottom;
					// cover it up
					GetWindowRect(hwnd, &rect);
					SetWindowPos(hwnd, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top - height, SWP_NOZORDER | SWP_NOMOVE);
					return TRUE;
				}
			}

		    break;

		case WM_SETCURSOR:
			if ((HWND)wparam == GetDlgItem(hwnd, IDC_SHOWSQL) ||
				(HWND)wparam == GetDlgItem(hwnd, IDC_HIDESQL)   )
			{
				SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(32649)));
				SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
				return TRUE;
			}
			break;

        //show filter help
	    case WM_HELP:
		    ShowHelp("http://sqlyogkb.webyog.com/article/83-gui-filtering-and-sorting-of-data");
		    return TRUE;
	}

	return FALSE;
}

//function to initialize the dialog
wyBool
SortAndFilter::InitDialog()
{
	wyInt32			    i, j, k = 0, paddingtobutton, paddingtowindow, selcol = 0, dropwidth = 0, filterindex = -1, existingfilter = -1;
	wyInt32				topPreview;
    enum_field_types    coltype = MYSQL_TYPE_VARCHAR;
    RECT                rect, rectwin, recttemp, rectcol, rectcond, rectvalue, recttext = {0};
    wyString            temp;
    HWND                hwndtemp;
    HDC                 hdc;
    HFONT               hfont;
    const wyChar*       filtercond;
    HICON               hicon;

    hicon = LoadIcon(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_FILTER));
    SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon);
    DestroyIcon(hicon);

    //save the window rect, we need this to calculate the new window height
    GetWindowRect(m_hwnd, &rectwin); 

    //the dialog box initially has three controls, a column selection, a filter condition and a value field
    //we ignore the ids assigned to them and assign new ids
    //we can create as many rows of these controls. the starting id is 100
    hwndtemp = GetDlgItem(m_hwnd, IDC_FILTER_COLUMN);
    SetWindowLongPtr(hwndtemp, GWL_ID, 100);
    GetWindowRect(hwndtemp, &rectcol);
    MapWindowRect(NULL, m_hwnd, &rectcol);
    hwndtemp = GetDlgItem(m_hwnd, IDC_FILTER_COND);
    SetWindowLongPtr(hwndtemp, GWL_ID, 101);
    GetWindowRect(hwndtemp, &rectcond);
    MapWindowRect(NULL, m_hwnd, &rectcond);
    hwndtemp = GetDlgItem(m_hwnd, IDC_FILTER_VALUE);
    SetWindowLongPtr(hwndtemp, GWL_ID, 102);
    GetWindowRect(hwndtemp, &rectvalue);
    MapWindowRect(NULL, m_hwnd, &rectvalue);

    //calculate the padding between OK button and the last control
    GetWindowRect(GetDlgItem(m_hwnd, IDOK), &rect);
    GetWindowRect(GetDlgItem(m_hwnd, 102), &recttemp);
    paddingtobutton = rect.top - recttemp.bottom;
    paddingtowindow = rectwin.bottom - rect.bottom;
    
    //create the additional controls for the multi column filter 
    for(i = 1, j = 102; i < m_filtercolumns; ++i)
    {
        //create the column combo
        hwndtemp = CreateWindowEx(0, WC_COMBOBOX, L"", CBS_DROPDOWNLIST | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | WS_CHILD | CBS_SORT,
            rectcol.left, rectcol.top + (i * paddingtobutton) + (i * (rectcol.bottom - rectcol.top)) , 
            rectcol.right - rectcol.left, rectcol.bottom - rectcol.top, m_hwnd, (HMENU)++j, GetModuleHandle(NULL), NULL);

        //get the font for the precreated column combo and apply it to the new control created
        hfont = (HFONT)SendMessage(GetDlgItem(m_hwnd, 100), WM_GETFONT, NULL, NULL);
        SendMessage(hwndtemp, WM_SETFONT, (WPARAM)hfont, 0);

        //create filter condition combo
        hwndtemp = CreateWindowEx(0, WC_COMBOBOX, L"", CBS_DROPDOWNLIST | WS_VISIBLE | CBS_SORT | WS_VSCROLL | WS_TABSTOP | WS_CHILD,
            rectcond.left, rectcol.top + (i * paddingtobutton) + (i * (rectcol.bottom - rectcol.top)) , 
            rectcond.right - rectcond.left, rectcol.bottom - rectcol.top, m_hwnd, (HMENU)++j, GetModuleHandle(NULL), NULL);

        //get the font for the precreated filter condition combo and apply it to the new control created
        hfont = (HFONT)SendMessage(GetDlgItem(m_hwnd, 101), WM_GETFONT, NULL, NULL);
        SendMessage(hwndtemp, WM_SETFONT, (WPARAM)hfont, 0);

        //create filter value combo
        hwndtemp = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_VISIBLE | WS_TABSTOP | WS_CHILD,
            rectvalue.left, rectcol.top + (i * paddingtobutton) + (i * (rectcol.bottom - rectcol.top)) , 
            rectvalue.right - rectvalue.left, rectcol.bottom - rectcol.top, m_hwnd, (HMENU)++j, GetModuleHandle(NULL), NULL);

        //get the font for the precreated filter value edit control and apply it to the new control created
        hfont = (HFONT)SendMessage(GetDlgItem(m_hwnd, 102), WM_GETFONT, NULL, NULL);
        SendMessage(hwndtemp, WM_SETFONT, (WPARAM)hfont, 0);
    }
    
    hwndtemp = GetDlgItem(m_hwnd, j);
    GetWindowRect(hwndtemp, &rect);
    MapWindowRect(NULL, m_hwnd, &rect);

	// position Show/Hide static links
	hwndtemp = GetDlgItem(m_hwnd, IDC_SHOWSQL);
	GetWindowRect(hwndtemp, &recttemp);
	MapWindowRect(NULL, m_hwnd, &recttemp);
	SetWindowPos(hwndtemp, GetDlgItem(m_hwnd, j), recttemp.left, rect.bottom + paddingtobutton, 0, 0, SWP_NOSIZE);

	hwndtemp = GetDlgItem(m_hwnd, IDC_HIDESQL);
	GetWindowRect(hwndtemp, &recttemp);
	MapWindowRect(NULL, m_hwnd, &recttemp);
	SetWindowPos(hwndtemp, GetDlgItem(m_hwnd, IDC_SHOWSQL), recttemp.left, rect.bottom + paddingtobutton, 0, 0, SWP_NOSIZE);

	//position OK button
	hwndtemp = GetDlgItem(m_hwnd, IDOK);
    GetWindowRect(hwndtemp, &recttemp);
    MapWindowRect(NULL, m_hwnd, &recttemp);
    SetWindowPos(hwndtemp, GetDlgItem(m_hwnd, IDC_HIDESQL), recttemp.left, rect.bottom + paddingtobutton, 0, 0, SWP_NOSIZE);

    //position cancel button
    hwndtemp = GetDlgItem(m_hwnd, IDCANCEL);
    GetWindowRect(hwndtemp, &recttemp);
    MapWindowRect(NULL, m_hwnd, &recttemp);
    SetWindowPos(hwndtemp, GetDlgItem(m_hwnd, IDOK), recttemp.left, rect.bottom + paddingtobutton, 0, 0, SWP_NOSIZE);
	// calculate the top for Preview which is same as dialog's bottom covering it 
	topPreview = rect.bottom + paddingtobutton + (recttemp.bottom - recttemp.top) + paddingtowindow;
	
	// Resize dialog window to include additional filters we added
	// This will cover the Preview window completely
	GetWindowRect(hwndtemp, &recttemp);
	SetWindowPos(m_hwnd, NULL, 0, 0, rectwin.right - rectwin.left, recttemp.bottom + paddingtowindow - rectwin.top, SWP_NOZORDER | SWP_NOMOVE);

	// position SQL Preview window
	hwndtemp = GetDlgItem(m_hwnd, IDC_PREVIEW);
	GetWindowRect(hwndtemp, &recttemp);
	MapWindowRect(NULL, m_hwnd, &recttemp);
	SetWindowPos(hwndtemp, GetDlgItem(m_hwnd, IDCANCEL), recttemp.left, topPreview, 0, 0, SWP_NOSIZE);

	// Both Show & Hide SQL Preview are not visible to start with. If server side queries is on, turn on Show
	if (NULL != m_querybuilder)
		ShowWindow(GetDlgItem(m_hwnd, IDC_SHOWSQL), SW_SHOW);

    hwndtemp = GetDlgItem(m_hwnd, 100);
    hdc = GetDC(hwndtemp);
    hfont = GetWindowFont(hwndtemp);
    hfont = (HFONT)SelectObject(hdc, hfont);

    //fill the first combo box with the column names.
    for(i = 0, j = 0; i < m_data->m_datares->field_count ; i++)
	{
		coltype = m_data->m_datares->fields[i].type;
        
        //if column type is not bit and spatial
        if(coltype != MYSQL_TYPE_BIT && coltype != MYSQL_TYPE_GEOMETRY)
		{
            //calculate the drop width for the combo box so that it will show the column name completely
            temp.SetAs(m_data->m_datares->fields[i].name, m_wnd->m_ismysql41);
            memset(&recttext, 0, sizeof(recttext));
            DrawText(hdc, temp.GetAsWideChar(), -1, &recttext, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
            
            if(recttext.right > dropwidth)
            {
                dropwidth = recttext.right;
            }

            //set the selection
			for(k = 0; k < m_filtercolumns; k++)
            {
                hwndtemp = GetDlgItem(m_hwnd, 100 + (k * 3));
                SendMessage(hwndtemp, CB_INSERTSTRING, (WPARAM)j, (LPARAM)temp.GetAsWideChar());
				SendMessage(hwndtemp, CB_SETITEMDATA, (WPARAM)j, (LPARAM)i);

                if(i == m_filter[k].m_currcolindex)
                {
                    SendMessage(hwndtemp, CB_SETCURSEL, j, 0);
                }
            }
            
            if(m_colno == i)
			{
				selcol = j;
			}

            j++;
		}
	}

    SelectObject(hdc, hfont);
    ReleaseDC(GetDlgItem(m_hwnd, 100), hdc);

    //if no column created, return 
	if(j == 0)
	{
		return wyFalse;
	}

    dropwidth += 20;

    //set the drop width, add the empty column for column combo
    for(i = 0; i < m_filtercolumns; ++i)
    {
        hwndtemp = GetDlgItem(m_hwnd, 100 + (i * 3));
        SendMessage(hwndtemp, CB_SETDROPPEDWIDTH , dropwidth, 0);
        SendMessage(hwndtemp, CB_INSERTSTRING, 0, (LPARAM)L"");
        SendMessage(hwndtemp, CB_SETITEMDATA, 0, (LPARAM)-1);

        if(existingfilter == -1 && m_filter[i].m_currcolindex == m_colno)
        {
            existingfilter = i;
        }

        if(existingfilter == -1 && filterindex == -1 && m_filter[i].m_currcolindex == -1)
        {
            filterindex = i;
        }
    }

    for(j = 0; j < m_filtercolumns; j++)
    {
        hwndtemp = GetDlgItem(m_hwnd, 101 + (j * 3));

        for(i = FT_EQUAL; i <= FT_LIKE; i++)
        {
            if((filtercond = GetFilterTypeStr((FilterType)i)))
            {
                temp.SetAs(filtercond);
                SendMessage(hwndtemp, CB_INSERTSTRING, i, (LPARAM)temp.GetAsWideChar());
                SendMessage(hwndtemp, CB_SETITEMDATA, i, (LPARAM)i);
                filtercond = GetFilterTypeStr(m_filter[j].m_currfiltertype);

                if(filtercond && !temp.Compare(filtercond))
                {
                    SendMessage(hwndtemp, CB_SETCURSEL, i, 0);
                }
            }
        }

        SendMessage(hwndtemp, CB_INSERTSTRING, 0, (LPARAM)L"");
        SendMessage(hwndtemp, CB_SETITEMDATA, 0, (LPARAM)-1);

        if(m_filter[j].m_currfiltertype == FT_NONE)
        {
            SendMessage(hwndtemp, CB_SETCURSEL, 0, 0);
        }
    }

    //set the filter values
	for(i = 0; i < m_filtercolumns; i++)
    {
        hwndtemp = GetDlgItem(m_hwnd, 102 + (i * 3));
        SendMessage(hwndtemp, EM_LIMITTEXT, (WPARAM)256, 0);
        
        if(m_filter[i].m_colindex >= 0 && m_filter[i].m_currfiltertype != FT_NONE)
        {
            //we add % in the begin if FT_LIKEBOTH or FT_LIKEBEGIN
            //we add % in the end if FT_LIKEBOTH or FT_LIKEEND
            temp.Sprintf("%s%s%s", m_filter[i].m_currfiltertype == FT_LIKEBOTH || m_filter[i].m_currfiltertype == FT_LIKEBEGIN ? "%" : "",
                m_filter[i].m_currvalue.GetString(), 
                m_filter[i].m_currfiltertype == FT_LIKEBOTH || m_filter[i].m_currfiltertype == FT_LIKEEND ? "%" : "");

            //set the text
            SetWindowText(hwndtemp, temp.GetAsWideChar());
        }
    }

    if(filterindex != -1)
    {
        SendMessage(GetDlgItem(m_hwnd, 100 + (filterindex * 3)), CB_SETCURSEL, selcol + 1, 0); 
        SendMessage(GetDlgItem(m_hwnd, 101 + (filterindex * 3)), CB_SETCURSEL, 1, 0); 
        selcol = SendMessage(GetDlgItem(m_hwnd, 100 + (filterindex * 3)), CB_GETITEMDATA, selcol + 1, 0);

        if(selcol == m_colno)
        {
            if(!m_celldata)
            {
                SetWindowText(GetDlgItem(m_hwnd, 102 + (filterindex * 3)), TEXT(STRING_NULL)); 
            }
            else if(coltype != MYSQL_TYPE_BLOB || BlobMgmt::IsDataBinary(m_celldata, m_celldatalen) == wyFalse)
            {
                temp.SetAs(m_celldata ? m_celldata : "", m_wnd->m_ismysql41);
                SetWindowText(GetDlgItem(m_hwnd, 102 + (filterindex * 3)), temp.GetAsWideChar()); 
            }
        }
    }
    else
    {
        filterindex = existingfilter == -1 ? 0 : existingfilter;
    }

    if((hwndtemp = GetDlgItem(m_hwnd, 102 + (filterindex * 3))))
    {
        SendMessage(hwndtemp, EM_SETSEL, 0, (LPARAM)-1);
        SetFocus(hwndtemp);
    }

    //position window relative to the filter button in the toolbar
    if(m_prelrect)
    {
        PositionWindow();
    }
        
	return wyTrue;
}

void
SortAndFilter::OnWMInitdlgValues(HWND hwnd)
{
	MDIWindow *wnd = GetActiveWin();
	HWND hwndpreview = GetDlgItem(hwnd, IDC_PREVIEW);

	// attempt to set scintilla code page to support utf8 data

	//set the lexer language 
	SendMessage(hwndpreview, SCI_SETLEXERLANGUAGE, 0, (LPARAM)"MySQL");

	SendMessage(hwndpreview, SCI_SETCODEPAGE, SC_CP_UTF8, 0);

	EditorFont::FormatEditor(hwndpreview, wyTrue, wnd->m_keywordstring, wnd->m_functionstring);

	/* make the scintilla preview control word wrap */
	SendMessage(hwndpreview, SCI_SETWRAPMODE, SC_WRAP_WORD, SC_WRAP_WORD);

	//Line added because on changing color there was margin coming for editor
	SendMessage(hwndpreview, SCI_SETMARGINWIDTHN, 1, 0);
	SendMessage(hwndpreview, SCI_SETREADONLY, (WPARAM)TRUE, (LPARAM)0);
}


//function to position the window relative to button
void
SortAndFilter::PositionWindow()
{
    HMONITOR    hmonitor;
    MONITORINFO mi = {0};
    RECT        temprect = {0}, rc = *m_prelrect;
	RECT		temprect2 = { 0 }, temprect3 = { 0 };
    wyInt32     width, height, animate = 0;
    RECT*       prect = &rc;

    //get the modified window rect
    GetWindowRect(m_hwnd, &temprect);

	if (NULL != m_querybuilder)
	{
		// Since the Preview starts by hidden, we need to include it also in our calculations
		GetWindowRect(GetDlgItem(m_hwnd, IDC_PREVIEW), &temprect2);
		temprect.bottom += temprect2.bottom - temprect2.top; // add the size of preview window which is now hidden
		GetWindowRect(GetDlgItem(m_hwnd, IDOK), &temprect3);
		temprect.bottom += temprect2.top - temprect3.bottom; // including the additional gap when it shows
	}

    //get the monitor info for the monitor associated with the rect
    hmonitor = MonitorFromRect(prect, MONITOR_DEFAULTTONEAREST);
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hmonitor, &mi);

    //calculate and modify the width and height based on the availabe space to best fit the window
    width = temprect.right - temprect.left;
    height = temprect.bottom - temprect.top;
    
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

    //uncomment the below code to animate the window
    /*AnimateWindow(m_hwnd, 100, AW_ACTIVATE | animate);
    InvalidateRect(m_hwnd, NULL, TRUE);
    UpdateWindow(m_hwnd);*/
}

//reset sort
void 
SortAndFilter::ResetSort()
{
    wyInt32 i;

    for(i = 0; i < m_sortcolumns; ++i)
    {
        m_sort[i].m_colindex = m_sort[i].m_currcolindex = -1;
        m_sort[i].m_sorttype = m_sort[i].m_currsorttype = ST_NONE;
    }
}

//reset filter
void 
SortAndFilter::ResetFilter(wyInt32 startindex)
{
    wyInt32 i;

    m_isfilter = wyFalse;

    for(i = startindex; i < m_filtercolumns; ++i)
    {
        m_filter[i].m_colindex = m_filter[i].m_currcolindex = -1;
        m_filter[i].m_filtertype = m_filter[i].m_currfiltertype = FT_NONE;
        m_filter[i].m_value.Clear();
        m_filter[i].m_currvalue.Clear();
    }
}

//function to process filter when pressed OK from custom filter dialog
wyBool
SortAndFilter::ProcessFilter()
{
    wyInt32         i, j = 0;
    wyInt32         index, condition, textlen;
    wyWChar         text[258];
    wyString        temp;
    HWND            hwndcol, hwndcond, hwndvalue;
    FilterType      temptype;

    //loop through the filter rows
    for(i = 0, j = 0; i < m_filtercolumns; i++)
    {
        //get the handles for the current row
        hwndcol = GetDlgItem(m_hwnd, 100 + i * 3);
        hwndcond = GetDlgItem(m_hwnd, 101 + i * 3);
        hwndvalue = GetDlgItem(m_hwnd, 102 + i * 3);

        //get column index and filter condition
        index = SendMessage(hwndcol, CB_GETCURSEL, 0, 0);
        index = SendMessage(hwndcol, CB_GETITEMDATA, index, 0);
        condition = SendMessage(hwndcond, CB_GETCURSEL, 0, 0);
        condition = SendMessage(hwndcond, CB_GETITEMDATA, condition, 0);

        //get filter text
        textlen = min(256, GetWindowTextLength(hwndvalue));
        GetWindowText(hwndvalue, text, textlen + 1);
        temp.SetAs(text);

        //set it to filter data
        m_filter[j].m_colindex = index;
        m_filter[j].m_filtertype = (FilterType)condition;
        m_filter[j].m_value.SetAs(temp);
        
        //if filter data ins FT_LIKE, then we need to check whether we have a % in the begin and/or end and modify the filter condition accordingly
        if(m_filter[j].m_filtertype == FT_LIKE)
        {
            temptype = FT_NONE;

            //we have filter text with % in the begining, so change the condition to FT_LIKEBEGIN and erase the %
            if(!m_filter[j].m_value.FindChar('%'))
            {
                temptype = FT_LIKEBEGIN;
                m_filter[j].m_value.Erase(0, 1);
            }

            //now check whether we have a % at the end and modify the condition + erase the %
            if(m_filter[j].m_value.FindChar('%', m_filter[j].m_value.GetLength() - 1) != -1)
            {
                temptype = temptype == FT_NONE ? FT_LIKEEND : FT_LIKEBOTH;
                m_filter[j].m_value.Erase(m_filter[j].m_value.GetLength() - 1, 1);
            }
            
            if(temptype == FT_NONE)
            {
                temptype = FT_LIKE;
            }

            //set the modified filter condition
            m_filter[j].m_filtertype = temptype;
        }

        //if it is valid
        if(m_filter[j].m_colindex != -1 && m_filter[j].m_filtertype != FT_NONE)
        {
            j++;
        }
    }
    
    //if there was an existing filter or atlease on new filter row is valid, then we return wyTrue
    //we are considering existing filter so that user can reset the filter
    return (j == 0 && m_isfilter == wyFalse) ? wyFalse : wyTrue;
}

//function to set the filter string
void
SortAndFilter::SetFilterString(wyBool isnewfilter)
{
    wyInt32         i;
    wyString        colname;
    const wyChar*   typetext;
    wyChar*         escapedstr = NULL;
    wyString*       ptemp;
	wyChar*			backtick;

	//from  .ini file
	backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    //depending on we are asking for new filter or the existing filter
    ptemp = isnewfilter == wyTrue ? &m_filterstring : &m_currfilterstring;
    ptemp->Clear();

    //loop throguth filters
    for(i = 0; i < m_filtercolumns; i++)
    {
		//if it is isnt valid, then all filters are over.
		if (m_filter[i].m_colindex == -1 || m_filter[i].m_filtertype == FT_NONE)
			break;

		//we always use AND to combine two conditions
		if (ptemp->GetLength())
		{
			ptemp->Add(" AND ");
		}

		//set the column name
		colname.SetAs(m_data->m_datares->fields[m_filter[i].m_colindex].name, m_wnd->m_ismysql41);

		//get the string representation of filter
		typetext = GetFilterTypeStr(m_filter[i].m_filtertype);

		//handle NULL
		if ((m_filter[i].m_filtertype == FT_EQUAL || m_filter[i].m_filtertype == FT_NOTEQUAL) &&
			(!m_filter[i].m_value.CompareI("NULL") || !m_filter[i].m_value.CompareI("(NULL)")))
		{
			ptemp->AddSprintf("%s%s%s IS %s NULL", backtick, colname.GetString(), backtick,
				m_filter[i].m_filtertype == FT_EQUAL ? "" : "NOT");
		}
		else
		{
			//escape the string and add it to the filter text
			escapedstr = (wyChar*)malloc(m_filter[i].m_value.GetLength() * 2 + 1);
			m_wnd->m_tunnel->mysql_real_escape_string(m_wnd->m_mysql,
				escapedstr, m_filter[i].m_value.GetString(), m_filter[i].m_value.GetLength());
			ptemp->AddSprintf("%s%s%s %s '%s%s%s'", backtick, colname.GetString(), backtick, typetext ? typetext : "",
				m_filter[i].m_filtertype == FT_LIKEBEGIN || m_filter[i].m_filtertype == FT_LIKEBOTH ? "%" : "",
				escapedstr,
				m_filter[i].m_filtertype == FT_LIKEEND || m_filter[i].m_filtertype == FT_LIKEBOTH ? "%" : "");
			free(escapedstr);
		}
    }
}

//compare function for qsort
wyInt32
SortAndFilter::Compare(void* p, const void* a, const void* b)
{
	MYSQL_ROWEX**   arg1 = (MYSQL_ROWEX**)a;
	MYSQL_ROWEX**   arg2 = (MYSQL_ROWEX**)b;
    SortAndFilter*  psnf = (SortAndFilter*)p;
	wyString        wystringForArg1, wystringForArg2;
    wyInt32         sortcolumn = psnf->m_sort[0].m_colindex;
    wyInt32         sorttype = psnf->m_sort[0].m_sorttype;
    MYSQL_ROW       arg1row, arg2row;

    arg1row = (*arg1)->m_row;
    arg2row = (*arg2)->m_row;

    if(arg1row[sortcolumn] != NULL)
	{
        wystringForArg1.SetAs(arg1row[sortcolumn]);
	}

	if(arg2row[sortcolumn] != NULL)
	{
		wystringForArg2.SetAs(arg2row[sortcolumn]);
	}

    //handle NULL
	if((arg1row[sortcolumn] == 0 && arg2row[sortcolumn] == 0 ||
		wystringForArg1.CompareI("(NULL)") == 0 && wystringForArg2.CompareI("(NULL)") == 0))
	{
		return 0;
	}
	else if((arg1row[sortcolumn] == 0 || wystringForArg1.CompareI("(NULL)") == 0))
	{
        return (sorttype == GV_ASCENDING) ? -1 : 1;
	}
	else if((arg2row[sortcolumn] == 0 || wystringForArg2.CompareI("(NULL)") == 0))
	{
		return (sorttype == GV_ASCENDING) ? 1 : -1;
	}
	
	//MYSQL string data type handling
	if(psnf->m_coltype == MYSQL_TYPE_VARCHAR || 
		psnf->m_coltype == MYSQL_TYPE_VAR_STRING || 
		psnf->m_coltype == MYSQL_TYPE_STRING || 
		psnf->m_coltype == MYSQL_TYPE_DATE || 
		psnf->m_coltype == MYSQL_TYPE_NEWDATE ||
		psnf->m_coltype == MYSQL_TYPE_TIMESTAMP ||
		psnf->m_coltype == MYSQL_TYPE_DATETIME || 
		psnf->m_coltype == MYSQL_TYPE_BLOB ||
		psnf->m_coltype	== MYSQL_TYPE_TIME)
	{ 
		return psnf->HandleStringTypeSort(&wystringForArg1, &wystringForArg2, sorttype);
	}
    //MYSQL Decimal data type handling
	else if(psnf->m_coltype == MYSQL_TYPE_DECIMAL || 
		psnf->m_coltype == MYSQL_TYPE_DOUBLE || 
		psnf->m_coltype == MYSQL_TYPE_FLOAT ||
		psnf->m_coltype == MYSQL_TYPE_NEWDECIMAL)
	{
		return psnf->HandleFloatingTypeSort(&wystringForArg1, &wystringForArg2, sorttype);
	}
    //MYSQL INTEGRAL data type handling
	else if(psnf->m_coltype == MYSQL_TYPE_TINY || 
		psnf->m_coltype == MYSQL_TYPE_SHORT || 
		psnf->m_coltype == MYSQL_TYPE_LONG || 
		psnf->m_coltype == MYSQL_TYPE_LONGLONG || 
		psnf->m_coltype == MYSQL_TYPE_INT24 ||
		psnf->m_coltype == MYSQL_TYPE_BIT)
	{
		return psnf-> HandleIntegralTypeSort(&wystringForArg1, &wystringForArg2, sorttype);
	}
    //Handle YEAR differently
	else if(psnf->m_coltype == MYSQL_TYPE_YEAR)
	{
		if(wystringForArg1.GetLength() == 2)
		{
			return psnf->HandleSortOnYearCol(&wystringForArg1, &wystringForArg2, sorttype);
		}
		else
		{
			return psnf->HandleStringTypeSort(&wystringForArg1, &wystringForArg2, sorttype);
		}
	}
	
    return 0;
}

//string comparison for MYSQL string data type
wyInt32
SortAndFilter::HandleStringTypeSort(wyString* wystringForArg1, wyString* wystringForArg2, wyInt32 sorttype)
{
    wyInt32 ret;

    ret =  (sorttype == GV_ASCENDING) ? 
		strcmpi(wystringForArg1->GetString(), wystringForArg2->GetString()) 
        : -(strcmpi(wystringForArg1->GetString(), wystringForArg2->GetString()));

    return ret;
}


//called inside the compare function to sort the Floating point type data
wyInt32
SortAndFilter::HandleFloatingTypeSort(wyString* wystringForArg1, wyString* wystringForArg2, wyInt32 sorttype)
{
	if(wystringForArg1->GetAsDouble(NULL) > wystringForArg2->GetAsDouble(NULL))
	{
		return (sorttype == GV_ASCENDING) ? 1 : -1;
	}
	else if(wystringForArg1->GetAsDouble(NULL) < wystringForArg2->GetAsDouble(NULL)) 
	{
		return (sorttype == GV_ASCENDING) ? -1 : 1;
	}
	
    return 0;
}

//called inside the compare func to sort the MYSQL Integral types
wyInt32 
SortAndFilter::HandleIntegralTypeSort(wyString* wystringForArg1, wyString* wystringForArg2, wyInt32 sorttype)
{
	//sort other Non-NULL values.
	if(wystringForArg1->GetAsInt32() > wystringForArg2->GetAsInt32())
	{
        return (sorttype == GV_ASCENDING) ? 1 : -1;
	}
	else if(wystringForArg1->GetAsInt32() < wystringForArg2->GetAsInt32()) 
	{
		return (sorttype == GV_ASCENDING) ? -1 : 1;
	}
	
    return 0;
}

//handle YEAR(2) type. function common for both Sort and filter.
wyInt32
SortAndFilter::HandleSortOnYearCol(wyString* wystringForArg1, wyString* wystringForArg2, wyInt32 sorttype)
{
	wyString temp1, temp2;

	//make a copy of wystringForArg1
	temp1.SetAs(wystringForArg1->GetString());

	//00-69 = 2000-2069 70-99 = 1970-1999
	//prefix the appropriate century for param 1
	if(wystringForArg1->GetAsInt32() >= 0 && wystringForArg1->GetAsInt32() <= 69)
	{
		wystringForArg1->SetAs("20");
    }
	else
	{
		wystringForArg1->SetAs("19");
	}

	wystringForArg1->Add(temp1.GetString());

	//prefix the appropriate century for param 2
	temp2.SetAs(wystringForArg2->GetString());

	if(wystringForArg2->GetAsInt32() >= 0 && wystringForArg2->GetAsInt32() <= 69)
	{	
		wystringForArg2->SetAs("20");
	}
	else
	{
		wystringForArg2->SetAs("19");
	}

	wystringForArg2->Add(temp2.GetString());

	//if this function call is from sort
	if(wystringForArg1->GetAsInt32() > wystringForArg2->GetAsInt32())
	{
		return (sorttype == GV_ASCENDING) ? 1 : -1;
	}
	else if(wystringForArg1->GetAsInt32() < wystringForArg2->GetAsInt32())
	{
		return (sorttype == GV_ASCENDING) ? -1 : 1;
	}
    
    return 0;
}

//function free original row array backed up when client side sorting/filtering is done
void
SortAndFilter::FreeOrigRowArray(wyBool iscallfromdestructor)
{
    //we no longer require any notification from the array, so lets remove it
    if(m_data && m_data->m_rowarray)
    {
        m_data->m_rowarray->SetCallBackProc(NULL);
        m_data->m_rowarray->SetLongData(NULL);
    }

    if(m_porigrowarray)
    {
        //if the call was from destructor, we have to clear the display row array, and the elements deletion will be done from the original array backed up by the sort and filter module
        //this is done to prevent any memory leak
        if(iscallfromdestructor == wyTrue)
        {
            m_data->m_rowarray->Resize(0, wyFalse);
        }

        m_porigrowarray->Resize(0, iscallfromdestructor);
        delete m_porigrowarray;
        m_porigrowarray = NULL;
    }
}

//function to sync changes from the row array used in DataView to the original row array backed up before sorting/filtering
void
SortAndFilter::SyncToOrigRowArray(ArrayAction aa, wyInt32 index, void* extra)
{
    wyInt32         i, count;
    MYSQL_ROWEX*    prow;

    //if the original array exist
    if(m_porigrowarray)
    {
        //get the row, 
        //for an update operation the extra parameter is the one to be considered as row
        //for any other operation, we get the row at the index position
        prow = (aa == AA_UPDATE) ? (MYSQL_ROWEX*)extra : m_data->m_rowarray->GetRowExAt(index);

        //if it is insert operation
        if(aa == AA_INSERT)
        {
            //find the index at which the element to be inserted
            index = (index == m_data->m_rowarray->GetLength() - 1) ? m_porigrowarray->GetLength() : index;
            m_porigrowarray->Insert(prow, index);
        }
        else
        {
            //loop through the original array and find the position
            for(i = 0, count = m_porigrowarray->GetLength(); i < count; ++i)
            {
                if(m_porigrowarray->GetRowExAt(i) == prow)
                {
                    break;
                }
            }

            //update
            if(aa == AA_UPDATE)
            {
                m_porigrowarray->Update(i, m_data->m_rowarray->GetRowExAt(index));
            }
            else
            {
                //for delete operation
                m_porigrowarray->Remove(i);
            }
        }
    }
}

//function to end the column sort
void 
SortAndFilter::EndColumnSort(wyBool issuccess)
{
    wyInt32 i, j, count;

    //loop through the columns
    for(i = 0; i < m_sortcolumns; ++i)
    {
        //if it is success, then copy the new sort to current sort
        if(issuccess == wyTrue)
        {
            m_sort[i].m_currcolindex = m_sort[i].m_colindex;
            m_sort[i].m_currsorttype = m_sort[i].m_sorttype;
        }
        //else copy the current sort to new sort
        else
        {
            m_sort[i].m_colindex = m_sort[i].m_currcolindex;
            m_sort[i].m_sorttype = m_sort[i].m_currsorttype;
        }
    }

    //shift the invalid columns to the end
    for(i = 0, count = m_sortcolumns; i < count; ++i)
    {
        if(m_sort[i].m_currcolindex == -1 || m_sort[i].m_currsorttype == ST_NONE)
        {
            for(j = i; j < count - 1; ++j)
            {
                m_sort[j] = m_sort[j + 1];
            }

            m_sort[count - 1].m_colindex = m_sort[count - 1].m_currcolindex = -1;
            m_sort[count - 1].m_sorttype = m_sort[count - 1].m_currsorttype = ST_NONE;
            --i;
            --count;
        }
    }
}

//function to begin column sort
void 
SortAndFilter::BeginColoumnSort(wyInt32 col, wyBool isadd)
{
    wyInt32 i, sortcol;
    wyBool  iscolinsort = wyFalse;

    //loop through available sort
    for(i = 0, sortcol = 0; i < m_sortcolumns; ++i)
    {
        //if we have a free sort available
        if(m_sort[i].m_currcolindex == -1)
        {
            break;
        }

        //lets check whether the sort index match with the current column index
        if(m_sort[i].m_currcolindex == col)
        {
            m_sort[i].m_sorttype = m_sort[i].m_currsorttype;

            //if it is ascending, change to descending
            if(m_sort[i].m_currsorttype == ST_ASC)
            {
                m_sort[i].m_sorttype = ST_DESC;
            }
            //if it is descending, change to ascending
            else if(m_sort[i].m_currsorttype == ST_DESC)
            {
                m_sort[i].m_sorttype = ST_NONE;
            }

            //if no sort, remvoe the column from sort
            if(m_sort[i].m_sorttype == ST_NONE)
            {
                m_sort[i].m_colindex = -1;
            }

            //set the flag to indicate that the column is has an existing sort
            iscolinsort = wyTrue;

            break;
        }

        sortcol++;
    }

    //if the current column is not in sort
    if(iscolinsort == wyFalse)
    {
        //sort column
        sortcol = min(m_sortcolumns - 1, sortcol);

        //if we want to replace the existing sort with the new sort
        if(isadd == wyFalse)
        {
            //reset the sort
            for(i = 0; i < m_sortcolumns; ++i)
            {
                m_sort[i].m_colindex = -1;
                m_sort[i].m_sorttype = ST_NONE;
            }

            //sort col
            sortcol = 0;
        }
        
        //set sorting
        m_sort[sortcol].m_colindex = col;
        m_sort[sortcol].m_sorttype = ST_ASC;
    }
}

//function to get sort column
SortType 
SortAndFilter::GetColumnSort(wyInt32 col)
{
    wyInt32 i;
		
    for(i = 0; i < m_sortcolumns; ++i)
    {
        if(m_sort[i].m_colindex == col)
        {
            return m_sort[i].m_currsorttype;
        }
    }

    return ST_NONE;
}

//function to get the 'order by' clause for sorting
wyBool 
SortAndFilter::GetSortString(wyString& sortstring)
{
    wyInt32 i;

    sortstring.Clear();

    //for client side sorting we dont add any 'order by' clause
    if(m_isclient == wyTrue)
    {
        return wyFalse;
    }

    //loop through sort
    for(i = 0; i < m_sortcolumns; ++i)
    {
        //if it is valid sort, add the column to sort with sort order
        if(m_sort[i].m_colindex >= 0 && m_sort[i].m_sorttype != ST_NONE)
        {
            if(sortstring.GetLength())
            {
                sortstring.Add(", ");
            }
            else
            {
                sortstring.Add(" order by ");
            }

            sortstring.AddSprintf("%d ", m_sort[i].m_colindex + 1);
            sortstring.Add(m_sort[i].m_sorttype == ST_ASC ? "asc" : "desc");
        }
    }

    return wyTrue;
}

//function to get 'where' clause for filterging server side
wyBool 
SortAndFilter::GetFilterString(wyString& filterstring)
{
    wyString temp;

    filterstring.Clear();

    //for client side filtering we dont add 'where' clause
    if(m_isclient == wyTrue)
    {
        return wyFalse;
    }

    //set where clause
    if(m_filterstring.GetLength())
    {
        temp.Sprintf(" where %s", m_filterstring.GetString());
    }

    filterstring.SetAs(temp);
    return wyTrue;
}

//function to filter in client side
wyBool
SortAndFilter::FilterResult()
{
    wyInt32         rowcount, i, j, col, tempignoredcheckcount = 0;
    MYSQL_ROWEX*    myrowex;
    MYSQL_ROWEX**   temp;

    //well nothing to filter
    if(m_filter[0].m_colindex == -1 || m_filter[0].m_filtertype == FT_NONE)
    {
        return wyFalse;
    }

    //get column type and colum index
    m_coltype = m_data->m_datares->fields[m_filter[0].m_colindex].type;
    col = m_filter[0].m_colindex;

    //allocate memory for row array
    rowcount = m_porigrowarray->GetLength();
    temp = (MYSQL_ROWEX**)calloc(rowcount, sizeof(MYSQL_ROWEX*));

    //copy the currently ignored checked rows count and reset it
    tempignoredcheckcount = m_ignoredcheckcount;
    m_ignoredcheckcount = 0;
    m_isfilteronnull = ((!m_filter[0].m_value.CompareI(STRING_NULL) || !m_filter[0].m_value.CompareI("NULL")) && 
        (m_filter[0].m_filtertype == FT_EQUAL || m_filter[0].m_filtertype == FT_NOTEQUAL)) ? wyTrue : wyFalse;

    //loop through rows
    for(i = 0, j = 0; i < rowcount; i++)
    {
        //get the row
        myrowex = m_porigrowarray->GetRowExAt(i);

        //if it is new row or the row match the filter criteria, add it to the temperory row array
        if(myrowex->IsNewRow() == wyTrue || CheckTypeAndFilterOnCondition(m_coltype, myrowex->m_row[col], myrowex->IsMySQLRow()) == wyTrue)
        {
            temp[j] = myrowex;
            j++;
        }
        //else if it is not new row, we need to identify whether the row is checked. if it is checked it should added to ignroed check row count
        else if(myrowex->IsNewRow() == wyFalse)
        {
            m_ignoredcheckcount += myrowex->m_ischecked == wyTrue ? 1 : 0;
        }
    }

    m_isfilteronnull = wyFalse;

    //now realloc the array to save space
    temp = (MYSQL_ROWEX**)realloc(temp, j * sizeof(MYSQL_ROWEX*));

    //set the array to the original display row array
    m_data->m_rowarray->UseRowArray(temp, j);   

    //adjust the checked row count
    m_data->m_checkcount -= (m_ignoredcheckcount - tempignoredcheckcount);

    return wyTrue;
}

//a single function to do clinet side sort and filter. the module that uses SortAndFilter should only call this function
void 
SortAndFilter::ExecuteSortAndFilter()
{
    //flag tells whether we can delete the array, i.e it is no longer required for sort and/or filter
    wyInt32 candeletearray = 0;

    //allocate original array if it is not available
    if(!m_porigrowarray)
    {
        m_porigrowarray = new MySQLRowExArray();
        m_porigrowarray->SetRowArray(m_data->m_rowarray->GetRowArray(), m_data->m_rowarray->GetLength());

        //we need notifications from the array so that we can modify the original array
        m_data->m_rowarray->SetCallBackProc(SortAndFilter::RowsExArrayCallback);
        m_data->m_rowarray->SetLongData(this);
    }

    //if no sorting required
    if(m_sort[0].m_currsorttype != ST_NONE && m_sort[0].m_sorttype == ST_NONE)
    {
        //set the original row array as the display array
        m_data->m_rowarray->SetRowArray(m_porigrowarray->GetRowArray(), m_porigrowarray->GetLength(), wyFalse);

        //sort no longer require the originla array
        candeletearray |= 2;
    }

    //no filter, so filter no longer rquire the original array
    if(((m_filter[0].m_currcolindex != -1 && m_filter[0].m_currfiltertype != FT_NONE) && 
        (m_filter[0].m_colindex == -1 || m_filter[0].m_filtertype == FT_NONE)))
    {
        //check whether sort has already set the array to original, if not set it
        if(!(candeletearray & 2))
        {
            m_data->m_rowarray->SetRowArray(m_porigrowarray->GetRowArray(), m_porigrowarray->GetLength(), wyFalse);
            m_data->m_checkcount += m_ignoredcheckcount;
            m_ignoredcheckcount = 0;
        }

        //sort no longer require the original array
        candeletearray |= 1;
    }
    else
    {
        //filter it
        candeletearray |= (FilterResult() == wyFalse) ? 1 : 0;
    }

    //sort it
    candeletearray |= (SortResult() == wyFalse) ? 2 : 0;

    //neither sort nor filter require the original array
    if(candeletearray == 3)
    {
        FreeOrigRowArray();
    }
}

//function to sort result
wyBool
SortAndFilter::SortResult()
{
	wyInt32 rowcount;
    
    //no sort
    if(m_sort[0].m_sorttype == ST_NONE)
    {
        return wyFalse;
    }

    //if there are rows
    if((rowcount = m_data->m_rowarray->GetLength()))
    {
        //adjust the row array to ignore the new row at the end
        rowcount = m_data->m_rowarray->GetRowExAt(rowcount - 1)->IsNewRow() ? rowcount - 1 : rowcount;
    }

    //set column type
    m_coltype = m_data->m_datares->fields[m_sort[0].m_colindex].type;

    //sort the array
    qsort_s(m_data->m_rowarray->GetRowArray(), rowcount, sizeof(MYSQL_ROWEX*), SortAndFilter::Compare, this);

    return wyTrue;
}

//function to get the string representation of filter type
const wyChar* 
SortAndFilter::GetFilterTypeStr(FilterType ft)
{
    switch(ft)
    {
        case FT_EQUAL:
            return "=";

        case FT_NOTEQUAL:
            return "<>";

        case FT_GREATERTHAN:
            return ">";

        case FT_LESSTHAN:
            return "<";

        case FT_LIKE:
        case FT_LIKEBEGIN:
        case FT_LIKEEND:
        case FT_LIKEBOTH:
            return "LIKE";
    }

    return NULL;
}

//end filter
void
SortAndFilter::EndFilter(wyBool issuccess)
{
    wyInt32 i;

    //reset filter flag
    m_isfilter = wyFalse;

    //loop through filters
    for(i = 0; i < m_filtercolumns; ++i)
    {
        //if it is successful, copy the new filter to existing filter
        if(issuccess == wyTrue)
        {
            m_filter[i].m_currcolindex = m_filter[i].m_colindex;
            m_filter[i].m_currfiltertype = m_filter[i].m_filtertype;
            m_filter[i].m_currvalue.SetAs(m_filter[i].m_value);
        }
        //else copy existing filter to new filter
        else
        {
            m_filter[i].m_colindex = m_filter[i].m_currcolindex;
            m_filter[i].m_filtertype = m_filter[i].m_currfiltertype;
            m_filter[i].m_value.SetAs(m_filter[i].m_currvalue);
        }

        //if no filter, lets clean up
        if(m_filter[i].m_currfiltertype == FT_NONE)
        {
            m_filter[i].m_currcolindex = m_filter[i].m_colindex = -1;
            m_filter[i].m_currvalue.Clear();
            m_filter[i].m_value.Clear();
        }

        //if there is atleast one valid filter, set the flag
        if(m_filter[i].m_currcolindex != -1 && m_filter[i].m_currfiltertype != FT_NONE)
        {
            m_isfilter = wyTrue;
        }
    }

    //if filter is off, clear the filter strings
    if(m_isfilter == wyFalse)
    {
        m_currfilterstring.Clear();
        m_filterstring.Clear();
    }
    else
    {
        //if success copy new filter string to current filter string
        if(issuccess == wyTrue)
        {
            m_currfilterstring.SetAs(m_filterstring);
        }
        //else copy current filter string to new filter string
        else
        {
            m_filterstring.SetAs(m_currfilterstring);
        }
    }
}

//function to begin filter
wyBool 
SortAndFilter::BeginFilter(wyInt32 command, wyChar* data, wyUInt32 datalen, wyInt32 col, HWND hwndparent, IQueryBuilder* querybuilder, RECT* prect)
{
    wyInt32 i;

    //open custom filter
    if(command == ID_FILTER_CUSTOMFILTER || command == ID_RESETFILTER)
    {
		m_querybuilder = querybuilder;
		wyInt32 wId;

		wId = OpenFilterDialog(hwndparent, col, data, datalen, prect);
		m_querybuilder = NULL;
        if (wId == IDCANCEL)
        {
			// Since we are now munging with filter variables we have to cancel it
			if (m_isfilter == wyTrue) // already in filter.
			{
				m_filterstring.SetAs(m_currfilterstring);
			}
			else
			{
				ResetFilter();
				SetFilterString(wyTrue); // this will set to null
			}
			return wyFalse;
        }
    }
    else
    {
        //initialize the new filter
        for(i = 0; i < m_filtercolumns; ++i)
        {
            m_filter[i].m_filtertype = FT_NONE;
            m_filter[i].m_colindex = -1;
            m_filter[i].m_value.Clear();
        }

        //now based on the command, set the filter type
        switch(command)
        {
            case ID_FILTER_FIELD_EQUALS:
                m_filter[0].m_filtertype = FT_EQUAL;
                break;
            case ID_FILTER_FIELD_NOTEQUALS:
                m_filter[0].m_filtertype = FT_NOTEQUAL;
                break;
            case ID_FILTER_FIELD_GREATERTHEN:
                m_filter[0].m_filtertype = FT_GREATERTHAN;
                break;
            case ID_FILTER_FIELD_LESSTHEN:
                m_filter[0].m_filtertype = FT_LESSTHAN;
                break;
            case ID_FILTER_FIELDLIKEBEGIN:
                m_filter[0].m_filtertype = FT_LIKEBEGIN;
                break;
            case ID_FILTER_FIELDLIKEBOTH:
                m_filter[0].m_filtertype = FT_LIKEBOTH;
                break;
            case ID_FILTER_FIELDLIKEEND:
                m_filter[0].m_filtertype = FT_LIKEEND;
                break;
        }

        //set the column index
        m_filter[0].m_colindex = col;

        //set the filter text
        if(data)
        {
            m_filter[0].m_value.SetAs(data, m_wnd->m_ismysql41);

            //consider only the first 256 charecters
            if(m_filter[0].m_value.GetLength() > 256)
            {
                m_filter[0].m_value.Strip(m_filter[0].m_value.GetLength() - 256);
            }
        }
        else
        {
            //set it to null
            m_filter[0].m_value.SetAs(STRING_NULL);
        }
    }

    //set the new filter string
    SetFilterString(wyTrue);

    return wyTrue;
}

//do client side filtering
wyBool
SortAndFilter::CheckTypeAndFilterOnCondition(enum_field_types coltype, wyChar* key, wyBool ismysqlrow)
{
    //if is filter on NULL
    if(m_isfilteronnull == wyTrue)
    {
        if(m_filter[0].m_filtertype == FT_EQUAL)
        {
            return (ismysqlrow == wyTrue) ? (key ? wyFalse : wyTrue) :
                ((!key || !strcmpi(key, STRING_NULL) || !strcmpi(key, "NULL")) ? wyTrue : wyFalse);
        }
        else
        {
            return (ismysqlrow == wyTrue) ? (key ? wyTrue : wyFalse) :
                ((!key || !strcmpi(key, STRING_NULL) || !strcmpi(key, "NULL")) ? wyFalse : wyTrue);
        }        
    }

    //filter on MYSQL integral type
	if(coltype == MYSQL_TYPE_TINY ||  
		coltype == MYSQL_TYPE_SHORT || 
		coltype == MYSQL_TYPE_LONG || 
		coltype == MYSQL_TYPE_LONGLONG || 
		coltype == MYSQL_TYPE_INT24)
	{
		return FilterOnIntegralTypes(key);
	}
    //filter on MSYQL floating type.
	else if(coltype == MYSQL_TYPE_DECIMAL || 
		coltype == MYSQL_TYPE_DOUBLE ||					
		coltype == MYSQL_TYPE_FLOAT ||
		coltype == MYSQL_TYPE_NEWDECIMAL)
	{
		return FilterOnFloatingPointType(key);
	}
    //filter on MYSQL string type
	else if(coltype == MYSQL_TYPE_VARCHAR ||  
		coltype == MYSQL_TYPE_VAR_STRING || 
		coltype == MYSQL_TYPE_STRING || 
		coltype == MYSQL_TYPE_DATE || 
		coltype == MYSQL_TYPE_NEWDATE ||
		coltype == MYSQL_TYPE_TIMESTAMP ||
		coltype == MYSQL_TYPE_DATETIME || 
		coltype == MYSQL_TYPE_BLOB ||
		coltype	== MYSQL_TYPE_TIME)
	{
		return FilterOnStringType(key);
	}
	//filter on MySQL year type
	else if(coltype == MYSQL_TYPE_YEAR)
	{
		return FilterOnYear(key);
	}
	
    return wyFalse;
}

//handle filter on NULL
wyBool 
SortAndFilter::FilterOnNull()
{
    if(m_filter[0].m_filtertype == FT_EQUAL && !m_filter[0].m_value.CompareI("(null)"))
    {
        return wyTrue;
    }

    return wyFalse;
}

//function to filter on integral types
wyBool
SortAndFilter::FilterOnIntegralTypes(wyChar* data)
{
	wyInt32     intnode, intvalue;
	wyBool      isnullstring;
    wyString    node(STRING_NULL);

    if(data)
    {
        node.SetAs(data);
    }
	
	//check for NULL Node
    isnullstring = (!node.CompareI(STRING_NULL) || !node.CompareI("NULL")) ? wyTrue : wyFalse;

    //not null
	if(isnullstring == wyFalse)
	{
        //get node and filter text as integers
        intnode = node.GetAsInt32();
        intvalue = m_filter[0].m_value.GetAsInt32();

        switch(m_filter[0].m_filtertype)
        {
            case FT_EQUAL:
                return intnode == intvalue ? wyTrue : wyFalse;

            case FT_NOTEQUAL:
                return intnode != intvalue ? wyTrue : wyFalse;

            case FT_LESSTHAN:
                return intnode < intvalue ? wyTrue : wyFalse;

            case FT_GREATERTHAN:
                return intnode > intvalue ? wyTrue : wyFalse;

            default:
                if(m_filter[0].m_filtertype != FT_NONE)
                {
                    return FilterOnLike(data);
                }
        }
	}

    //else filter on NULL
    return FilterOnNull();
}

//handle filter on floating point
wyBool
SortAndFilter::FilterOnFloatingPointType(wyChar* data)
{
	wyBool      isnullstring;
    wyString    node(STRING_NULL);
    wyDouble    doublenode, doublevalue;

    if(data)
    {
        node.SetAs(data);
    }
	
	//check for NULL Node
    isnullstring = (!node.CompareI(STRING_NULL) || !node.CompareI("NULL")) ? wyTrue : wyFalse;

    //if not NULL
	if(isnullstring == wyFalse)
	{
        //get the node and filter text as double
        doublenode = node.GetAsDouble(NULL);
        doublevalue = m_filter[0].m_value.GetAsDouble(NULL);

        switch(m_filter[0].m_filtertype)
        {
            case FT_EQUAL:
                return doublenode == doublevalue ? wyTrue : wyFalse;

            case FT_NOTEQUAL:
                return doublenode != doublevalue ? wyTrue : wyFalse;

            case FT_LESSTHAN:
                return doublenode < doublevalue ? wyTrue : wyFalse;

            case FT_GREATERTHAN:
                return doublenode > doublevalue ? wyTrue : wyFalse;

            default:
                if(m_filter[0].m_filtertype != FT_NONE)
                {
                    return FilterOnLike(data);
                }
        }
	}
	
    return FilterOnNull();
}


// Filtering on the string type.
wyBool
SortAndFilter::FilterOnStringType(wyChar* data)
{
    wyInt32     res;
	wyBool      isnullstring;
    wyString    node(STRING_NULL);

    if(data)
    {
        node.SetAs(data);
    }
	
	//check for NULL Node
    isnullstring = (!node.CompareI(STRING_NULL) || !node.CompareI("NULL")) ? wyTrue : wyFalse;
    res = strnicmp(node.GetString(), m_filter[0].m_value.GetString(), 30);

    //if Not NULL values
	if(isnullstring == wyFalse)
	{
        switch(m_filter[0].m_filtertype)
        {
            case FT_EQUAL:
                return (res == 0) ? wyTrue : wyFalse;

            case FT_NOTEQUAL:
                return (res != 0) ? wyTrue : wyFalse;

            case FT_LESSTHAN:
                return (res < 0) ? wyTrue : wyFalse;

            case FT_GREATERTHAN:
                return (res > 0) ? wyTrue : wyFalse;

            default:
                if(m_filter[0].m_filtertype != FT_NONE)
                {
                    return FilterOnLike(data);
                }
        }
	}
    
    return FilterOnNull();
}

//handling filtering on LIKE
wyBool
SortAndFilter::FilterOnLike(wyChar* data)
{
	wyInt32		lengthkey(0);
	wyInt32		lengthnode(0);
	wyString	nodestring, temp;
	wyString	substr;

    //if data is not NULL
    if(data && strcmp(data, STRING_NULL) && strcmp(data, "NULL"))
	{
		nodestring.SetAs(data);

        //value%
        if(m_filter[0].m_filtertype == FT_LIKEEND)
		{
			lengthkey = m_filter[0].m_value.GetLength();

			if(nodestring.Substr(0, lengthkey))
			{
				substr.SetAs(nodestring.Substr(0, lengthkey));
                return (substr.CompareI(m_filter[0].m_value.GetString()) == 0) ? wyTrue : wyFalse;
			}
			else
			{
				return wyFalse;
			}
		}
        //%value
		else if(m_filter[0].m_filtertype == FT_LIKEBEGIN)
		{
			lengthkey = m_filter[0].m_value.GetLength();
			lengthnode = nodestring.GetLength();

			//if the length of key is greater than the node, then return false.
			if(lengthkey > lengthnode)
			{
				return wyFalse;
			}
			else
			{
				if(nodestring.Substr(lengthnode - lengthkey, lengthkey))
				{
					substr.SetAs(nodestring.Substr(lengthnode - lengthkey, lengthkey));
					return (substr.CompareI(m_filter[0].m_value.GetString()) == 0) ? wyTrue : wyFalse;	
				}
				else
				{
					return wyFalse;
				}
			}	
		}
        //%value%
		else if(m_filter[0].m_filtertype == FT_LIKEBOTH)
		{
			return (nodestring.FindI(const_cast<wyChar*>(m_filter[0].m_value.GetString())) != -1 ) ? wyTrue : wyFalse;
		}
        //simple LIKE 
		else 
		{
			return (nodestring.CompareI(m_filter[0].m_value.GetString()) == 0) ? wyTrue : wyFalse;
		}
	}
	
    return wyFalse;
}


wyBool
SortAndFilter::FilterOnYear(wyChar* data)
{
	wyString temp;

	//filter on LIKE, no need to make any changes that are done for the NON-LIKE conditions
    temp.SetAs(data);

	//00-69 = 2000-2069 70-99 = 1970-1999
	//prefix the appropriate century for param 1
    if(temp.GetLength() <= 2)
    {
        if(temp.GetAsInt32() >= 0 && temp.GetAsInt32() <= 69)
	    {
    		temp.SetAs("20");
        }
	    else
	    {
		    temp.SetAs("19");
    	}

	    temp.Add(data);
    }

    return FilterOnIntegralTypes(temp.GetAsAnsi());
}

//callback funtion for array actions
void CALLBACK
SortAndFilter::RowsExArrayCallback(ArrayAction aa, void* data, wyInt32 index, void* extra)
{
    SortAndFilter*  psf = (SortAndFilter*)data;
    MYSQL_ROWEX*    myrowex;

    //precaution
    if(psf && psf->m_data)
    {
        //check whether we are working on a sorted/filtered array. if so we have to update in the original array too
        //original array will be in filter will be there for only client side filtering, so there wont be any performance overhead
        if(aa != AA_RESIZE && psf->m_porigrowarray)
        {
            psf->SyncToOrigRowArray(aa, index, extra);
        }

        //free the element removed from the array
        if(aa == AA_REMOVE)
        {
            myrowex = (MYSQL_ROWEX*)extra;
            delete myrowex;
        }
    }
}
