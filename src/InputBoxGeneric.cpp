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


#include "InputBoxGeneric.h"
#include "Global.h"

extern      PGLOBALS        pGlobals;

InputBoxGeneric::InputBoxGeneric()
{
}

InputBoxGeneric::~InputBoxGeneric()
{
}

wyBool 
InputBoxGeneric::Create(HWND hwndparent, const wyChar * title, const wyChar * caption, const wyChar * errmsg, wyString& strvalue )
{
    wyInt32     ret;

    m_title.SetAs(title);
    m_caption.SetAs(caption);
    m_errmsg.SetAs(errmsg);
    m_strvalue.SetAs(strvalue.GetString());
    
    ret = DialogBoxParam ( pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_TEXTENTRYDIALOG), 
					       hwndparent, DlgWndProc, (LPARAM)this);
    if(ret == 0)
        return wyFalse;
    else {
        strvalue.SetAs(m_strvalue.GetString());
        return wyTrue;
    }
}

INT_PTR  CALLBACK 
InputBoxGeneric::DlgWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    InputBoxGeneric *txtentdlg = (InputBoxGeneric*)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
	
	switch(message)
	{
	case WM_INITDIALOG:
        ::SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
        LocalizeWindow(hwnd);
        ::PostMessage(hwnd, UM_INITDLGVALUES, 0, 0);
        break;

    case UM_INITDLGVALUES:
        txtentdlg->m_hwnd = hwnd;
        txtentdlg->InitializeDlgValues();
        break;
    
    case WM_COMMAND:
        txtentdlg->HandleWMCommand(wParam,lParam);
        break;
        
    }

    return 0;
}

void
InputBoxGeneric::InitializeDlgValues()
{
    VERIFY(::SetWindowText(m_hwnd, m_title.GetAsWideChar()));
    VERIFY(::SetWindowText(::GetDlgItem(m_hwnd, IDC_CAPTION), m_caption.GetAsWideChar()));
    VERIFY(::SetWindowText(::GetDlgItem(m_hwnd, IDC_DBNAME), m_strvalue.GetAsWideChar()));
}

wyBool
InputBoxGeneric::HandleWMCommand(WPARAM wParam, LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
		case IDOK :
            {
                wyUInt32 length = ::SendMessage(GetDlgItem(m_hwnd, IDC_DBNAME) ,WM_GETTEXTLENGTH ,0, 0);
                if(length == 0)
                    yog_message(m_hwnd, m_errmsg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK|MB_ICONERROR);
                else
                {
                    // We cannot use wyString here as the API asks for TCHAR*
                    wyWChar * value = (wyWChar*)calloc(sizeof(wyWChar), (length+2) * (sizeof(wyWChar)));
                    ::SendMessage(GetDlgItem(m_hwnd, IDC_DBNAME), WM_GETTEXT, length+1, (LPARAM)value);
	                m_strvalue.SetAs(value);
                    free(value);
					EndDialog(m_hwnd, 1);
                    return wyTrue;
                }
            }
			break;

		case IDCANCEL :
			EndDialog(m_hwnd, 0);
            return wyFalse;
	}

    return wyFalse;
}