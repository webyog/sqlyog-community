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

#include "L10n.h"
#include "Include.h"
#include "GUIHelper.h"

#define STYLE_MASK      31

extern PGLOBALS pGlobals;

enum ControlType {TYPE_UNKNOWN, TYPE_BUTTON, TYPE_STATIC, TYPE_GROUP};

class WinControl : public wyElem
{
    public:
        WinControl(HWND hwnd, RECT* prect, wyWChar* pclassname);
        WinControl();
        RECT    m_rectold;
        RECT    m_rectnew;
        HWND    m_hwnd;
        wyBool  m_isadjust;
        wyInt32 m_width;
        wyInt32 m_height;
        wyBool  m_iscontainer;
        ControlType m_ctrltype;
};

///A callback function used to acheive localization in window
BOOL CALLBACK LocalizeChildProc(HWND hwnd, LPARAM lparam);
BOOL CALLBACK ExtractControls(HWND hwnd, LPARAM lparam);
BOOL CALLBACK ResizeRadioAndCheckBoxes(HWND, LPARAM lparam);
void FreeControlList(List* pcontrollist);
wyInt32 VerticalCompareFunct(const void* elem1, const void* elem2);
wyInt32 HorizontalCompareFunct(const void* elem1, const void* elem2);

WinControl::WinControl(HWND hwnd, RECT* prect, wyWChar* pclassname)
{
    wyInt32 style;

    m_hwnd = hwnd;
    m_rectold.left = prect->left;
    m_rectold.top = prect->top;
    m_rectold.right = prect->right - prect->left;
    m_rectold.bottom = prect->bottom - prect->top;
    m_rectnew = m_rectold;
    m_width = 0;
    m_height = 0;
    m_iscontainer = m_isadjust = wyFalse;
    m_ctrltype = TYPE_UNKNOWN;

    if(!wcsicmp(pclassname, L"static"))
    {
        m_ctrltype = TYPE_STATIC;
        m_isadjust = wyTrue;
    }
    else if(!wcsicmp(pclassname, L"button"))
    {
        m_ctrltype = TYPE_BUTTON;
        m_isadjust = wyTrue;
    }
    else if(!wcsicmp(pclassname, L"SysTabControl32"))
    {
        m_ctrltype = TYPE_GROUP;
        m_isadjust = wyTrue;
    }
    
    if(m_ctrltype == TYPE_STATIC)
    {
        style = GetWindowLongPtr(hwnd, GWL_STYLE);
        
        if((STYLE_MASK & style) == SS_BLACKRECT || (STYLE_MASK & style) == SS_GRAYRECT || (STYLE_MASK & style) == SS_BLACKFRAME ||
            (STYLE_MASK & style) == SS_WHITERECT || (STYLE_MASK & style) == SS_GRAYFRAME || (STYLE_MASK & style) == SS_WHITEFRAME)
        {
            m_ctrltype = TYPE_GROUP;
        }
    }
    else if(m_ctrltype == TYPE_BUTTON)
    {
        style = SendMessage(hwnd, WM_GETDLGCODE, NULL, NULL);
        if(style == DLGC_STATIC)
        {
            m_ctrltype = TYPE_GROUP;
        }
    }
}

WinControl::WinControl()
{
}

wyWChar* LocalizeStringResource(wyInt32 id, HINSTANCE hinstance, wyBool defaulttoid)
{
    wyWChar     str[512] = {0};
    wyWChar*    ptr;

    if(LoadString(hinstance, id, str, 511))
    {
        if((ptr = _(str)) == str)
        {
            return (defaulttoid == wyTrue) ? MAKEINTRESOURCE(id) : NULL;
        }

        return ptr;
    }

    return (defaulttoid == wyTrue) ? MAKEINTRESOURCE(id) : NULL;
}

void 
LocalizeMenu(HMENU hmenu)
{
    wyInt32 size, i, count;
    HMENU hsubmenu;
    MENUITEMINFO minfo = {0};
    wyWChar* buffer;

    count = GetMenuItemCount(hmenu);

    for(i = 0; i < count; ++i)
    {
        hsubmenu = GetSubMenu(hmenu, i);

        if(hsubmenu)
        {
            LocalizeMenu(hsubmenu);
        }

        memset(&minfo, 0, sizeof(minfo));
        minfo.cbSize = sizeof(minfo);

        minfo.fMask = MIIM_TYPE;
        minfo.fType = MFT_STRING;
        GetMenuItemInfo(hmenu, i, TRUE, &minfo);

        size = minfo.cch + 1;
        buffer = new wyWChar[size];            

        minfo.fMask = MIIM_STRING;
        minfo.fType = MFT_STRING;
        minfo.cch = size;
        minfo.dwTypeData = buffer;
        GetMenuItemInfo(hmenu, i, TRUE, &minfo);

        if(wcslen(buffer) == 0)
        {
            delete[] buffer;
            continue; 
        }
              
	    minfo.fMask = MIIM_TYPE;
        minfo.fType = MFT_STRING;
        minfo.dwTypeData = _(buffer);
        SetMenuItemInfo(hmenu, i, TRUE, &minfo);

        delete[] buffer;
    }
}

BOOL CALLBACK 
LocalizeChildProc(HWND hwnd, LPARAM lparam)
{
    wyWChar     *buffer;
    wyInt32     size;

    size = GetWindowTextLength(hwnd) + 1;
    buffer = new wyWChar[size];
    GetWindowText(hwnd, buffer, size);
    SetWindowText(hwnd, _(buffer));
    delete[] buffer;

    return TRUE;
}

void 
LocalizeWindow(HWND hwnd, void (_cdecl *rearrange)(HWND hwnd, wyBool beforetranslating, LPARAM& lparam))
{  
    LPARAM lparam;

    if(rearrange)
    {
        rearrange(hwnd, wyTrue, lparam);    
    }

    LocalizeChildProc(hwnd, NULL);
    EnumChildWindows(hwnd, LocalizeChildProc, NULL);

    if(rearrange)
    {
        rearrange(hwnd, wyFalse, lparam);    
    }
}

INT_PTR CALLBACK    
L10n::DlgProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam)
{
    L10n* pl10n = (L10n*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch(message)
    {
        case WM_INITDIALOG: 
            SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
            pl10n = (L10n*)lparam;
            
            if(pl10n->InitDialog(hwnd) == wyFalse)
            {
                EndDialog(hwnd, -1);
            }

            LocalizeWindow(hwnd);
            break;

        case WM_CLOSE:
            if(pl10n->m_dbname)
            {
                return TRUE;
            }
            return FALSE;

        case WM_COMMAND:
            if(LOWORD(wparam) == IDOK)
            {
                EndDialog(hwnd, pl10n->OnOK(hwnd));
                break;
            }
            else if(LOWORD(wparam) == IDCANCEL)
            {
                EndDialog(hwnd, -1);
                break;
            }

        default:
            return FALSE;
    }

    return TRUE;
}

L10n::L10n(HWND hwnd, const wyChar* dbname)
{
    m_dbname = dbname;
    m_plang = NULL;
    m_hwndparent = hwnd;
    m_langcount = GetL10nLanguageCount(m_dbname);

}

L10n::~L10n()
{
    FreeL10nLanguages(m_plang);
}

wyBool
L10n::InitDialog(HWND hwnd)
{
    wyInt32     i, j, sel = -1, locsel = -1, versionold = 0, versionnew = 0;
    wyWChar     directory[MAX_PATH];
    wyWChar*    lpfileport = NULL;
    wyString    temp, section, langcode, country;
    HWND        hwndcombo = GetDlgItem(hwnd, IDC_LANGCOMBO);
    wyWChar     langlocale[128];
    HICON       hicon;
    LCID        lcid;
	LANGID		id;

	id = GetUserDefaultUILanguage();
	lcid = MAKELCID(id, SORT_DEFAULT);
    GetLocaleInfo(lcid, LOCALE_SISO639LANGNAME, langlocale, 127);

    m_plang = GetL10nLanguages(m_dbname);
    versionnew = GetL10nDBVersion(m_dbname);
    
    if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyTrue)
    {
        temp.SetAs(directory);
        wyIni::IniGetString("UserInterface", "Language", "", &langcode, temp.GetString());
        versionold = wyIni::IniGetInt("UserInterface", "Version", 0, temp.GetString());
    }

    hicon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon);
    DestroyIcon(hicon);
    temp.SetAs(langlocale);
	langlocale[0] = 0;
	GetLocaleInfo(lcid, LOCALE_SISO3166CTRYNAME, langlocale, 127);

	if(langlocale[0])
	{
		country.SetAs(langlocale);
		temp.AddSprintf("-%s", country.GetString());
	}

    if(m_dbname)
    {
        EnableWindow(GetDlgItem(hwnd, IDCANCEL), FALSE);
    }
        
    for(i = 0; i < m_langcount; ++i)
    {
        j = (WPARAM)SendMessage(hwndcombo, CB_ADDSTRING, 0, (LPARAM)(m_plang + i)->m_lang.GetAsWideChar());
        SendMessage(hwndcombo, CB_SETITEMDATA, j, (LPARAM)i);
    }   

    SendMessage(hwndcombo, CB_SETCURSEL, (WPARAM)0, 0);

    for(i = 0; i < m_langcount; ++i)
    {
        j = SendMessage(hwndcombo, CB_GETITEMDATA, (WPARAM)i, 0);
            
        if(!(m_plang + j)->m_langcode.CompareI(langcode))
        {
            sel = i;
        }

        if(locsel == -1 && !(m_plang + j)->m_langcode.CompareI(temp))
        {
            locsel = i;
        }
    }

    if(m_dbname)
    {
        if(sel != -1 && versionnew <= versionold)
        {
            return wyFalse;
        }
        else if(locsel == -1)
        {
            locsel = MatchLocale(&temp, hwndcombo);
        }

        if(locsel == sel && locsel != -1)
        {
            return wyFalse;
        }

        if(locsel != -1)
        {
            InitL10n(temp.GetString(), m_dbname, wyTrue); 
        }
    }

    sel = (sel == -1) ? ((locsel == -1) ? 0 : locsel) : sel;

    SendMessage(hwndcombo, CB_SETCURSEL, (WPARAM)sel, 0);
    return wyTrue;
}

wyInt32 
L10n::MatchLocale(wyString* plocale, HWND hwndcombo)
{
    wyInt32 i, j;
    wyString temp;

    if((i = plocale->Find("-", 0)) == -1)
    {
        i = plocale->GetLength();
    }

    temp.Sprintf("%.*s", i, plocale->GetString());

    for(i = 0; i < m_langcount; ++i)
    {
        j = SendMessage(hwndcombo, CB_GETITEMDATA, (WPARAM)i, 0);

        if(!(m_plang + j)->m_langcode.FindI((wyChar*)temp.GetString(), 0))
        {
            plocale->SetAs(temp);
            return i;
        }
    }

    return -1;
}

wyInt32 
L10n::OnOK(HWND hwnd)
{
    wyInt32 i = -1;

    if(IsWindowEnabled(GetDlgItem(hwnd, IDC_LANGCOMBO)))
    {
        i = SendMessage(GetDlgItem(hwnd, IDC_LANGCOMBO), CB_GETCURSEL, 0, 0);
        i = SendMessage(GetDlgItem(hwnd, IDC_LANGCOMBO), CB_GETITEMDATA, (WPARAM)i, 0);
    }

    return i;
}

void L10n::Show()
{
    wyInt32     i, version;
    wyWChar     directory[MAX_PATH];
    wyWChar*    lpfileport = NULL;
    wyString    dirstr;
    const wyChar*     str;

    if(m_langcount == 0 ||
        (i = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LANGUAGE), m_hwndparent, L10n::DlgProc, (LPARAM)this)) == -1)
    {
        return;
    }

    if((m_plang + i) && SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyTrue)
    {
        dirstr.SetAs(directory);
        wyIni::IniWriteString("UserInterface", "Language", (m_plang + i)->m_langcode.GetString(), dirstr.GetString());
        
        version = GetL10nDBVersion(m_dbname);
        wyIni::IniWriteInt("UserInterface", "Version", version, dirstr.GetString());

        if(!m_dbname)
        {        
            str = GetL10nLangcode();

            if(str && (m_plang + i)->m_langcode.Compare(str))
            {
                MessageBox(m_hwndparent, _(L"Please restart SQLyog for the language change to take effect"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
            }
        }
        else
        {
            CloseL10n();
        }
    }
}

//sample control rearranging begins here ---------------------

BOOL CALLBACK 
ExtractControls(HWND hwnd, LPARAM lparam)
{
    wyWChar     *buffer, *text;
    wyInt32     size;
    wyWChar     classname[257];
    List*       pcontrollist = (List*)lparam;
    WinControl* pwincontrol;
    RECT        rect, rctext;
    HWND        hwndparent;
    HDC         hdc;
    HFONT       hfont;

    size = GetWindowTextLength(hwnd) + 1;
    buffer = new wyWChar[size];
    GetWindowText(hwnd, buffer, size);
    text = _(buffer);

    if(!pcontrollist->GetCount())
    {
        GetClientRect(hwnd, &rect);
    }
    else
    {
        hwndparent = ((WinControl*)pcontrollist->GetFirst())->m_hwnd;
        GetWindowRect(hwnd, &rect);
        MapWindowPoints(NULL, hwndparent, (LPPOINT)&rect, 2);
    }

    GetClassName(hwnd, classname, 256);
    pwincontrol = new WinControl(hwnd, &rect, classname);

    if((pwincontrol->m_ctrltype == TYPE_BUTTON || pwincontrol->m_ctrltype == TYPE_STATIC) && wcscmp(text, buffer))
    {
        ::memset(&rctext, 0, sizeof(rctext));
        hdc = GetDC(((WinControl*)pcontrollist->GetFirst())->m_hwnd);
        hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        hfont = (HFONT)SelectObject(hdc, hfont);
        DrawText(hdc, buffer, -1, &rctext, DT_CALCRECT);
        hfont = (HFONT)SelectObject(hdc, hfont);
        ReleaseDC(((WinControl*)pcontrollist->GetFirst())->m_hwnd, hdc);
        DeleteObject(hfont);
        pwincontrol->m_width = pwincontrol->m_rectold.right - (rctext.right - rctext.left);
        pwincontrol->m_height = pwincontrol->m_rectold.bottom - (rctext.bottom - rctext.top);
    }

    pcontrollist->Insert(pwincontrol);
    delete[] buffer;
    return TRUE;
}

void RearrangeControls(HWND hwnd, wyBool beforetranslating, LPARAM& lparam)
{
    WinControl* pcontrol;
    wyInt32     size, bottom = 0, right = 0, dlgwidth, dlgheight, width, largest = 0;
    wyWChar*    temp;
    HDC         hdc;
    wyString    text;
    RECT        rctext, rctemp;
    WinControl  pdlg, tempcontrol, basecontrol;
    wyInt32     marginright, marginbottom, controlcount, i;
    WinControl* pcontrolarray;
    HFONT       hfont;
    List        *pcontrollist;

    if(beforetranslating == wyTrue)
    {
        pcontrollist = new List;
        lparam = (LPARAM)pcontrollist;
        ExtractControls(hwnd, (LPARAM)pcontrollist);
        EnumChildWindows(hwnd, ExtractControls, (LPARAM)pcontrollist);
        return;
    }
    
    pcontrollist = (List*)lparam;
    pdlg = *((WinControl*)pcontrollist->GetFirst());
    controlcount = pcontrollist->GetCount() - 1;
    pcontrolarray = new WinControl[controlcount];
    marginright = pdlg.m_rectold.right;
    marginbottom = pdlg.m_rectold.bottom;

    GetWindowRect(pdlg.m_hwnd, &rctemp);
    dlgwidth = (rctemp.right - rctemp.left) - pdlg.m_rectold.right;
    dlgheight = (rctemp.bottom - rctemp.top) - pdlg.m_rectold.bottom;

    hdc = GetDC(pdlg.m_hwnd);
    hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    hfont = (HFONT)SelectObject(hdc, hfont);
    
    for(i = 0, pcontrol = (WinControl*)pdlg.m_next; pcontrol; ++i, pcontrol = (WinControl*)pcontrol->m_next)
    {
        if(pdlg.m_rectold.right - (pcontrol->m_rectold.left + pcontrol->m_rectold.right) < marginright)
        {
            marginright = pdlg.m_rectold.right - (pcontrol->m_rectold.left + pcontrol->m_rectold.right);
        }

        if(pdlg.m_rectold.bottom - (pcontrol->m_rectold.top + pcontrol->m_rectold.bottom) < marginbottom)
        {
            marginbottom = pdlg.m_rectold.bottom - (pcontrol->m_rectold.top + pcontrol->m_rectold.bottom);
        }

        if(pcontrol->m_ctrltype == TYPE_GROUP || pcontrol->m_isadjust == wyFalse)
        {
            pcontrolarray[i] = *pcontrol;
            continue;
        }

        size = GetWindowTextLength(pcontrol->m_hwnd) + 1;
        temp = new wyWChar[size];
        GetWindowText(pcontrol->m_hwnd, temp, size);
        text.SetAs(temp);
        delete[] temp;
        
        memset(&rctext, 0, sizeof(rctext));
        DrawText(hdc, text.GetAsWideChar(), -1, &rctext, DT_CALCRECT);

        if(pcontrol->m_rectnew.right < rctext.right - rctext.left + pcontrol->m_width)
        {
            pcontrol->m_rectnew.right = rctext.right - rctext.left + pcontrol->m_width;
        }
        else if(pcontrol->m_rectnew.bottom < rctext.bottom - rctext.top + pcontrol->m_height)
        {
            pcontrol->m_rectnew.bottom = rctext.bottom - rctext.top + pcontrol->m_height;
        }

        SetWindowPos(pcontrol->m_hwnd, NULL, 0, 0, pcontrol->m_rectnew.right, pcontrol->m_rectnew.bottom, SWP_NOZORDER | SWP_NOMOVE);
        pcontrolarray[i] = *pcontrol;
    }

    qsort(pcontrolarray, controlcount, sizeof(WinControl), VerticalCompareFunct);

    memset(&tempcontrol, 0, sizeof(WinControl));
    
    for(i = 1; i < controlcount; ++i)
    {
        if(pcontrolarray[i].m_rectold.top > pcontrolarray[i - 1].m_rectold.top)
        {
            tempcontrol = pcontrolarray[i - 1];
        }

        pcontrolarray[i].m_rectnew.top = tempcontrol.m_rectnew.top + tempcontrol.m_rectnew.bottom + (pcontrolarray[i].m_rectold.top - (tempcontrol.m_rectold.top + tempcontrol.m_rectold.bottom));

        if(pcontrolarray[i].m_rectnew.top + pcontrolarray[i].m_rectnew.bottom > bottom)
        {
            bottom = pcontrolarray[i].m_rectnew.top + pcontrolarray[i].m_rectnew.bottom;
        }
    }

    qsort(pcontrolarray, controlcount, sizeof(WinControl), HorizontalCompareFunct);

    memset(&tempcontrol, 0, sizeof(WinControl));
    
    for(largest = 0, i = 1; i < controlcount; ++i)
    {
        if(pcontrolarray[i].m_rectold.left > pcontrolarray[i - 1].m_rectold.left)
        {
            width = largest;
            tempcontrol = pcontrolarray[largest];
            if(pcontrolarray[i].m_isadjust == wyTrue)
            {
                largest = i;
            }
        }
        
        if(pcontrolarray[i].m_isadjust == wyTrue && pcontrolarray[i].m_rectnew.right > pcontrolarray[largest].m_rectnew.right)
        {
            largest = i;
        }

        pcontrolarray[i].m_rectnew.left = tempcontrol.m_rectnew.left + tempcontrol.m_rectnew.right + (pcontrolarray[i].m_rectold.left - (tempcontrol.m_rectold.left + tempcontrol.m_rectold.right));

        if(pcontrolarray[i].m_rectnew.left + pcontrolarray[i].m_rectnew.right > right)
        {
            right = pcontrolarray[i].m_rectnew.left + pcontrolarray[i].m_rectnew.right;
        }
    }

    right += dlgwidth + marginright;
    bottom += dlgheight + marginbottom;
    SetWindowPos(pdlg.m_hwnd, NULL, 0, 0, right, bottom, SWP_NOZORDER | SWP_NOMOVE);

    for(i = 0; i < controlcount; ++i)
    {
        if(pcontrolarray[i].m_ctrltype == TYPE_GROUP)
        {
            marginright = pdlg.m_rectold.right - (pcontrolarray[i].m_rectold.left + pcontrolarray[i].m_rectold.right);
            marginbottom = pdlg.m_rectold.bottom - (pcontrolarray[i].m_rectold.top + pcontrolarray[i].m_rectold.bottom);
            pcontrolarray[i].m_rectnew.right =  (right - marginright - dlgwidth) - pcontrolarray[i].m_rectnew.left;
            pcontrolarray[i].m_rectnew.bottom = (bottom - marginbottom - dlgheight) - pcontrolarray[i].m_rectnew.top;
            SetWindowPos(pcontrolarray[i].m_hwnd, NULL, pcontrolarray[i].m_rectnew.left, pcontrolarray[i].m_rectnew.top, pcontrolarray[i].m_rectnew.right, pcontrolarray[i].m_rectnew.bottom, SWP_NOZORDER);
        }
        else
        {
            SetWindowPos(pcontrolarray[i].m_hwnd, NULL, pcontrolarray[i].m_rectnew.left, pcontrolarray[i].m_rectnew.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        }
    }

    delete[] pcontrolarray;    
    hfont = (HFONT)SelectObject(hdc, hfont);
    ReleaseDC(pdlg.m_hwnd, hdc);
    DeleteObject(hfont);
    FreeControlList(pcontrollist);
    PositionWindowCenterToMonitor(GetParent(hwnd), hwnd);
    delete pcontrollist;
}

wyInt32
VerticalCompareFunct(const void* elem1, const void* elem2)
{
    WinControl* pelem1 = (WinControl*)elem1;
    WinControl* pelem2 = (WinControl*)elem2;

    if(pelem1->m_rectold.top > (pelem2->m_rectold.top))
    {
        return 1;
    }
    else if(pelem1->m_rectold.top < (pelem2->m_rectold.top))
    {
        return -1;
    }

    return 0;
}

wyInt32
HorizontalCompareFunct(const void* elem1, const void* elem2)
{
    WinControl* pelem1 = (WinControl*)elem1;
    WinControl* pelem2 = (WinControl*)elem2;

    if(pelem1->m_rectold.left > pelem2->m_rectold.left)
    {
        return 1;
    }
    else if(pelem1->m_rectold.left < (pelem2->m_rectold.left))
    {
        return -1;
    }

    return 0;
}

void FreeControlList(List* pcontrollist)
{
    WinControl* pcontrol;

    while((pcontrol = (WinControl*)pcontrollist->GetFirst()))
    {
        pcontrollist->Remove(pcontrol);
        delete pcontrol;
    }
}

//sample control rearranging ends here ---------------------


BOOL CALLBACK ResizeRadioAndCheckBoxes(HWND hwnd, LPARAM lparam)
{
    wyInt32 style;

    style = GetWindowLongPtr(hwnd, GWL_STYLE);

    if((STYLE_MASK & style) == BS_AUTOCHECKBOX || (STYLE_MASK & style) == BS_CHECKBOX || 
        (STYLE_MASK & style) == BS_AUTORADIOBUTTON || (STYLE_MASK & style) == BS_RADIOBUTTON)
    {

    }

    return TRUE;
}

void SetLanguage(HWND hwndparent, wyBool isautomated)
{
    wyWChar		directory[MAX_PATH + 16] = {0};
    wyInt32     count;
    wyString    temp;

	count = GetModuleFileName(NULL, directory, MAX_PATH - 1);
	directory[count - pGlobals->m_modulenamelength] = '\0';
    temp.SetAs(directory);
    temp.Add(LANGUAGE_DBFILE);

    L10n l10n(hwndparent, isautomated == wyTrue ? temp.GetString() : NULL);
    l10n.Show(); 
}


