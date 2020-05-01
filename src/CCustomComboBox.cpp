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
#include <Shlwapi.h>
#include "CCustomComboBox.h"
// Default Constructor
CCustomComboBox::CCustomComboBox()
{
    memset(&m_cbif, 0, sizeof(COMBOBOXINFO));
    m_cbif.cbSize       = sizeof(COMBOBOXINFO);
    m_fillText          = wyFalse;
    m_isBkSpcPrsd       = wyFalse;
    m_showDropDown      = wyTrue;
    m_origEditCtrlProc  = NULL;
    m_origComboCtrlProc = NULL;
    m_hwndParent        = NULL;
    m_hwnd              = NULL;
    m_hwndCombo         = NULL;
	m_isconnlist		=wyFalse;
    memset(&m_editRect, 0, sizeof(RECT));
    memset(&m_marginRect, 0, sizeof(RECT));
}

// Destructor
CCustomComboBox::~CCustomComboBox()
{
	if(m_isconnlist==wyTrue)
			{
				free(m_connectionlist);
				m_isconnlist=wyFalse;
			}
}

// Subcalssed procedure to handle Handle ComboBox's Edit control
LRESULT CALLBACK
CCustomComboBox::EditCtrlProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    CCustomComboBox *con = (CCustomComboBox *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    DWORD     ch;
    switch(message)
    {
    case WM_SETTEXT:                        // To fill text in ComboBox
        if(con->m_fillText == wyTrue)
        {
            con->m_fillText = wyFalse;
            return CallWindowProc(con->m_origEditCtrlProc, hwnd, message, wParam, lParam);
        }
        else
            return TRUE;
        break;
    case WM_KEYDOWN:                        
        ch = (DWORD)wParam;
        if(ch == VK_DELETE)                     // To check if Delete button is pressed
        {
            con->m_isBkSpcPrsd = wyTrue;
        }
        else
        {
            con->m_isBkSpcPrsd = wyFalse;
        }
        break;
    case WM_CHAR:
        ch = (DWORD)wParam;
        if(ch == VK_BACK)                       // To check if BackSpace is pressed
        {
            con->m_isBkSpcPrsd = wyTrue;
        }
        else
        {
            con->m_isBkSpcPrsd = wyFalse;
        }
        break;
    } 
    return CallWindowProc(con->m_origEditCtrlProc, hwnd, message, wParam, lParam);
}

// SubClassed procedure to handle ComboBox's messages
LRESULT CALLBACK
CCustomComboBox::ComboCtrlProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    CCustomComboBox *con = (CCustomComboBox *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    wyInt32     ret = -1;
    HDC hdc;
    
    switch(message)
    {
	case WM_ERASEBKGND:
        return 1;           // On WM_ERASEBKGND do nothing.

    case WM_PAINT:

        // First let the default procedure paint ComboBox
        CallWindowProc(con->m_origComboCtrlProc, con->m_hwndCombo, message, wParam, lParam);
        hdc = GetDC(hwnd);

        // Send Message to Parent to handle Painting of ComboBox
        ret = SendMessage(con->m_hwndParent, CCBM_ERASEBKGND, (WPARAM)hdc, (LPARAM)con->m_id);
        
        InvalidateRect(con->m_cbif.hwndItem, NULL, TRUE);
        UpdateWindow(con->m_cbif.hwndItem);
        ReleaseDC(hwnd, hdc);
		return 1;

	case CB_SETDROPPEDWIDTH: // force this to be sent to combo
		ret = SendMessage(con->m_hwndParent, CB_SETDROPPEDWIDTH, wParam, lParam);
    }

    return CallWindowProc(con->m_origComboCtrlProc, con->m_hwndCombo, message, wParam, lParam);
}


//WNDPROC to handle Custom Combo Box's messages
LRESULT CALLBACK
CCustomComboBox::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    CCustomComboBox     *pcb = (CCustomComboBox *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    wyInt32             ret = -1;
    
    
    if(message >= 0x0140 && message <= 0x0163)
    {

        // if messages recieved are CB_SETCURSEL, CB_SELECTSTRING or CB_DELETESTRING, then set the flag to edit Edit Control
        if((message == CB_SETCURSEL || message == CB_SELECTSTRING || message == CB_DELETESTRING) && pcb != NULL)
        {   
            pcb->m_fillText = wyTrue;
        }

        ret = SendMessage(pcb->m_hwndCombo, message, wparam, lparam);
        return ret;
    }

    switch(message)
    {
    case WM_GETTEXTLENGTH:
    case WM_GETFONT:
    case WM_GETTEXT:
        return SendMessage(pcb->m_hwndCombo, message, wparam, lparam);
        break;
    case WM_NCCREATE:
        {
            pcb = new CCustomComboBox();
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pcb);
        }
        break;
    case CCBM_SETMARGIN:
        pcb->SetMargin(lparam);
        return 0;
        break;
    case CCBM_NOSHOWDROPDOWN:
        pcb->m_showDropDown = wyFalse;
        break;
    case CCBM_GETCOMBOHWND:
        return  (LRESULT)pcb->m_hwndCombo;
        break;
    case WM_CREATE:
        pcb->CreateCtrls(hwnd, lparam);
        break;
    case WM_COMMAND:
        pcb->HandleWmCommand(hwnd, wparam, lparam);
        break;
    case WM_SETTEXT:
        pcb->m_fillText = wyTrue;
        ret = SetWindowText(pcb->m_hwndCombo, (wyWChar *)lparam);
        InvalidateRect(pcb->m_hwndCombo, NULL, TRUE);
        UpdateWindow(pcb->m_hwndCombo);
        return ret;
        break;
    case WM_NOTIFY:
        SendMessage(pcb->m_hwndParent, message, wparam, lparam);
        break;
    case WM_SETFOCUS:
        if(pcb)
        {
            SetFocus(pcb->m_hwndCombo);
            InvalidateRect(pcb->m_hwndCombo, NULL, TRUE);
        }
        return 1;
        break;
    case WM_DRAWITEM:
        return SendMessage(pcb->m_hwndParent, message, wparam, lparam);
        break;
    case WM_MEASUREITEM:
        return SendMessage(pcb->m_hwndParent, message, wparam, lparam);
        break;
    case WM_HELP:
        PostMessage(pcb->m_hwndParent, message, wparam, lparam);
        break;
    case WM_DESTROY:
        if(pcb->m_hwndCombo)
        {
            DestroyWindow(pcb->m_hwndCombo);
            delete(pcb);
        }
        break;
    case WM_ENABLE:
        {   
            switch(wparam)
            {
            case TRUE:
                if(IsWindowEnabled(pcb->m_hwndCombo) == FALSE)
                {
                    pcb->SetMargin(NULL);
                }
            break;
            case FALSE:
                if(IsWindowEnabled(pcb->m_hwndCombo) == TRUE)
                {
                    pcb->SetMargin(NULL);
                }
            break;
            }
            EnableWindow(pcb->m_hwndCombo, wparam);
            InvalidateRect(pcb->m_hwndCombo, NULL, TRUE);
        }
    }
    return DefWindowProc(hwnd, message, wparam, lparam);
}

// function to set margin of the edit control of ComboBox
void
CCustomComboBox::SetMargin(LPARAM lparam)
{
    RECT    *rct = (RECT *)lparam;
    RECT    temp;
    memset(&temp, 0, sizeof(RECT));
    wyInt32 count = SendMessage(m_hwndCombo, CB_GETCOUNT, NULL, NULL);
    
    if(rct != NULL)
    {
        m_marginRect.left = rct->left;
        m_marginRect.right = rct->right;
        m_marginRect.top = rct->top;
        m_marginRect.bottom = rct->bottom;
    }

    // if there is anything in Combo Box, then set margins else No Margins
    if(count)
    {
        temp.left      = m_editRect.left + m_marginRect.left;
        temp.top       = m_editRect.top +  m_marginRect.top;
        temp.bottom    = m_editRect.bottom + m_marginRect.bottom;
        temp.right     = m_editRect.right + m_marginRect.right;
    }
    else
    {
        temp.left = m_editRect.left;
        temp.right = m_editRect.right;
        temp.top = m_editRect.top;
        temp.bottom = m_editRect.bottom;
    }

    if((temp.left != m_cbif.rcItem.left || temp.right != m_cbif.rcItem.right || 
        temp.top != m_cbif.rcItem.top || temp.bottom != m_cbif.rcItem.bottom))
    {
        m_cbif.rcItem.left      = temp.left;
        m_cbif.rcItem.top       = temp.top;
        m_cbif.rcItem.bottom    = temp.bottom;
        m_cbif.rcItem.right     = temp.right;
        
        SetWindowPos(m_cbif.hwndItem, NULL, 
            m_cbif.rcItem.left, m_cbif.rcItem.top, 
            m_cbif.rcItem.right -m_cbif.rcItem.left , m_cbif.rcItem.bottom - m_cbif.rcItem.top, 
            SWP_NOZORDER);

        InvalidateRect(m_hwndCombo, NULL, TRUE);
    }
}

// functio to handle CBN_EDITCHANGE message
void
CCustomComboBox::HandleEditChange()
{
    int         len = -1;
    wyWChar     str[70] = {0};
    wyWChar     lbStr[70] = {0};

	if(m_isconnlist==wyFalse){
		CreateConnectionList();
		m_isconnlist=wyTrue;
	}
	
    len = GetWindowText(m_hwndCombo, str, 65);
	OnHandleEditChange();
     
}


///Function to create list of connection
void
CCustomComboBox::CreateConnectionList(){


	wyInt32	ret,i=0;
	wyWChar     directory[MAX_PATH + 1], *lpfileport=0;
    wyChar      *tempnum = NULL, *allsectionnames, *tempconsecname;
	wyString    conn, dirstr, connnamestr, connselnamestr, tempnumstr;
	wyString	selconnnamestr, codepage, allsecnames, tempdir;
    wyChar	    seps[] = ";";

	// Get the complete path.
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	if(ret == 0)
		return;
	
	dirstr.SetAs(directory);
	tempdir.SetAs(directory);
    
    ret = wyIni::IniGetString(SECTION_NAME, "Host", "root", &selconnnamestr, dirstr.GetString());

    //wyIni::IniWriteString(SECTION_NAME, "Host", selconnnamestr.GetString(), dirstr.GetString());

	//wyIni::IniWriteString(SECTION_NAME, "Encoding", "utf8", dirstr.GetString());

	m_conncount=wyIni::IniGetSection(&allsecnames, &tempdir);

	m_connectionlist=(CONNLIST *)calloc(m_conncount, sizeof(CONNLIST));
    if(m_connectionlist==NULL)
	{
	exit(1);
	}
	

    allsectionnames = (wyChar*)allsecnames.GetString();

    tempconsecname = strtok(allsectionnames, seps);
	while(tempconsecname)
	{
        conn.SetAs(tempconsecname);
		
        tempnum  = strstr(tempconsecname, " ");
		
		if(tempnum != NULL)
		{
			tempnum = tempnum + 1;
			tempnumstr.SetAs(tempnum);
		}

        wyIni::IniGetString(conn.GetString(), "Name", "", &connnamestr, dirstr.GetString());


		m_connectionlist[i].m_dropdown=wyTrue;
		m_connectionlist[i].m_connectionname.SetAs(connnamestr.GetAsWideChar());
		m_connectionlist[i].m_itemvalue.SetAs(tempnumstr.GetAsWideChar());
		i++;
        tempconsecname = strtok(NULL, seps);
    }


	return;

}
//Function to Handle Edit Change
wyBool
CCustomComboBox::OnHandleEditChange(){
	wyWChar     str[70] = {0},*temp=NULL;
    wyInt32     id = -1;
    wyInt32     len = -1;
	int i, status=0, index, ret;	//status flag is set when atleast one match found
	wyWChar     directory[MAX_PATH + 1], *lpfileport=0;
    wyString	dirstr;
	
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	if(ret == 0)
		return wyFalse;
	dirstr.SetAs(directory);
	GetWindowText(m_hwndCombo, str, 65);
	len=wcslen(str);
	if(len)
	{
		if(len==1)
			SendMessage(m_hwndCombo, CB_SHOWDROPDOWN, FALSE, NULL);

		for(i=0;i<m_conncount;i++)
		{	 
			if(m_connectionlist[i].m_connectionname.GetLength()==0)
				continue;
			temp = StrStrI(m_connectionlist[i].m_connectionname.GetAsWideChar(),str);
			if(temp)
				status=1;
						
			if(temp && m_connectionlist[i].m_dropdown == wyFalse)
			{
				index=SendMessage(m_hwndCombo, CB_ADDSTRING, 0,(LPARAM)m_connectionlist[i].m_connectionname.GetAsWideChar());
				//MessageBox(hcb,connlist[i].itemvalue.GetAsWideChar(),connlist[i].conn_name.GetAsWideChar(),MB_OK);
				VERIFY(SendMessage(m_hwndCombo, CB_SETITEMDATA, index,m_connectionlist[i].m_itemvalue.GetAsInt32()));
				m_connectionlist[i].m_dropdown=wyTrue;
			}

			if(!temp && m_connectionlist[i].m_dropdown==wyTrue)
			 {
				 int index_delete=SendMessage(m_hwndCombo,CB_FINDSTRINGEXACT,-1,(LPARAM)m_connectionlist[i].m_connectionname.GetAsWideChar());		
				 SendMessage(m_hwndCombo, CB_DELETESTRING, index_delete,NULL);
				 m_connectionlist[i].m_dropdown=wyFalse;
			}
		 }
					 
	}
	
	if(!len || status==0)
	{
		SendMessage(m_hwndCombo, CB_SETCURSEL, -1, 0);
		for(i=0;i<m_conncount;i++)
		{
			if(m_connectionlist[i].m_connectionname.GetLength())
			{	
				if(m_connectionlist[i].m_dropdown == wyFalse)
				{
					index=SendMessage(m_hwndCombo, CB_ADDSTRING, 0,(LPARAM)m_connectionlist[i].m_connectionname.GetAsWideChar());
					VERIFY(SendMessage(m_hwndCombo, CB_SETITEMDATA, index, (LPARAM)m_connectionlist[i].m_itemvalue.GetAsInt32()));
					m_connectionlist[i].m_dropdown=wyTrue;
				}
						
			}
		}
		pGlobals->m_pcmainwin->m_connection->PopulateColorArray(m_hwndCombo, &dirstr);
	}
	else
	{
		pGlobals->m_pcmainwin->m_connection->PopulateColorArray(m_hwndCombo, &dirstr);
		if(SendMessage(m_hwndCombo, CB_GETDROPPEDSTATE, NULL, NULL) == FALSE)
		{
			if(m_showDropDown == wyTrue)
			{
				SendMessage(m_hwndCombo, CB_SHOWDROPDOWN, TRUE, NULL);
				SetCursor(LoadCursor(NULL, IDC_ARROW));
			}
			else
				m_showDropDown = wyTrue;
		}
		SendMessage(m_hwndCombo, CB_SETEDITSEL, NULL, MAKELPARAM(-1,0));
					
	}
	
	return wyTrue;
}


// function handles WM_COMMAND message to the Custom Combo Control
void
CCustomComboBox::HandleWmCommand(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    wyInt32     id = -1;
    wyWChar     str[65] = {0};
    if(LOWORD(wparam) == IDC_COMBOCUSTOM)
    {
        switch(HIWORD(wparam))
        {
		
        case CBN_EDITCHANGE:
            {
                HandleEditChange();
                InvalidateRect(m_hwndCombo, NULL, TRUE);
            }
            break;
        case CBN_SELCHANGE:
            {
                m_fillText = wyTrue;
                InvalidateRect(m_hwndCombo, NULL, TRUE);
				
            }
            break;
        }
        SendMessage(m_hwndParent, CCBM_NOTIFY, MAKEWPARAM(m_id, HIWORD(wparam)), lparam);
        
        GetWindowText(m_hwndCombo, str, 65);
        id = SendMessage(m_hwndCombo, CB_FINDSTRING, -1, (LPARAM)str);
        if(id != CB_ERR)
        {
            switch(HIWORD(wparam))
            {
                case CBN_SELENDOK:
                case CBN_SELENDCANCEL:
                    SendMessage(m_hwndCombo, CB_SETEDITSEL, NULL, MAKELPARAM(0, -1));
                break;
            }
        }
        
    }
}

// Function to create Combo Box as child of Custom Combo Box
wyBool
CCustomComboBox::CreateCtrls(HWND hwnd, LPARAM lParam)
{
    CREATESTRUCT    *ctst = (CREATESTRUCT *)lParam;
    wyInt32         ret = 0;
    DWORD           style = CBS_DROPDOWN | CBS_SORT | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP | WS_CHILD | WS_CLIPCHILDREN;
    HFONT           hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    m_id            = (wyInt32)ctst->hMenu;
    m_hwndParent    = ctst->hwndParent;
    m_hwnd          = hwnd;

    if((ctst->style &  CBS_OWNERDRAWFIXED))
        style |=  CBS_OWNERDRAWFIXED;
    m_hwndCombo     = CreateWindowExW(NULL, WC_COMBOBOX, NULL, 
                                style, 
                                0, 0, ctst->cx,ctst->cy, 
                                hwnd, (HMENU)IDC_COMBOCUSTOM, GetModuleHandle(NULL), 0);
    
    if(m_hwndCombo == NULL)
        return wyFalse;

    SendMessage(m_hwndCombo, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);

    ShowWindow(m_hwndCombo, SW_SHOW);

    ret = GetComboBoxInfo(m_hwndCombo, &m_cbif);

    m_editRect.bottom   = m_cbif.rcItem.bottom;
    m_editRect.right    = m_cbif.rcItem.right;
    m_editRect.left     = m_cbif.rcItem.left;
    m_editRect.top      = m_cbif.rcItem.top;
    
    if(ctst->style & WS_DISABLED)
    {
        SendMessage(hwnd, WM_ENABLE, FALSE, NULL);
    }
	
    SetWindowLongPtr(m_hwndCombo, GWLP_USERDATA, (LONG_PTR) this);
    m_origComboCtrlProc = (WNDPROC)SetWindowLongPtr(m_hwndCombo, GWLP_WNDPROC, (LONG_PTR) CCustomComboBox::ComboCtrlProc);
    
    SetWindowLongPtr(m_cbif.hwndItem, GWLP_USERDATA, (LONG_PTR)this);
    m_origEditCtrlProc = (WNDPROC)SetWindowLongPtr(m_cbif.hwndItem, GWLP_WNDPROC, (LONG_PTR) CCustomComboBox::EditCtrlProc);

    return wyTrue;
}

// register Custom Combo Control
wyBool 
RegisterCustomComboBox(HINSTANCE hInstance)
{
    ATOM		ret;
    WNDCLASS	wndclass = {0};
	
	wndclass.style         = 0;
    wndclass.lpfnWndProc   = CCustomComboBox::WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(HANDLE);
    wndclass.hInstance     = hInstance;
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	wndclass.lpszMenuName  = (LPCWSTR)NULL ;
    wndclass.lpszClassName = hInstance == GetModuleHandle(NULL)?CUSTOMCOMBOBOX1:CUSTOMCOMBOBOX;
	
	VERIFY(ret = RegisterClass(&wndclass));
	return wyTrue;
}

