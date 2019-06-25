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
#include "wyTheme.h"
#include "FrameWindowHelper.h"

//theme directory
#define THEMEDIR            "Themes"

//theme section in ini
#define ACTIVETHEME_SECTION "Themedetails"

//resource themes
#define AZURETHEME          IDR_THEME_TWILIGHT
#define METROTHEME          IDR_THEME_FLAT

//some predefines
#define MENUBARINDEX        0
#define MENUINDEX           1

//various xml element attributes/sub elements
#define BACKGROUND          "background"
#define COLOR1              "color1"
#define COLOR2              "color2"
#define IMAGE               "image"
#define TABBOTTOMLINE       "tabbottomline"
#define SEPERATOR           "seperator"
#define ACTIVE              "active"
#define INACTIVE            "inactive"
#define FORGROUND           "foreground"
#define SELTAB              "selectedtab"
#define TABTEXT             "tabtext"
#define SELTABTEXT          "seltabtext"
#define PLUSBUTTON          "plusbutton"
#define DRAGARROW           "dragarrow"
#define LINK                "hyperlink"
#define LINE                "line"

//xml elements
#define ELE_ROOT            "sqlyogtheme"
#define ELE_MDICLIENT       "mdiclient"
#define ELE_FRAMEWINDOW     "framewindow"
#define ELE_MENU            "menu"
#define ELE_MENUBAR         "menubar"
#define ELE_TOOLBAR         "toolbar"
#define ELE_HSPLITTER       "hsplitter"
#define ELE_VSPLITTER       "vsplitter"
#define ELE_CONNTAB         "conntab"
#define ELE_QUERYTAB        "querytab"
#define ELE_RESTAB          "restab"

//initialize the static members of the class
wyTheme*  wyTheme::m_theme = NULL;
WNDPROC   wyTheme::m_origeditclassproc = NULL;
HBRUSH    wyTheme::m_hbrushbuttonface = NULL;

extern PGLOBALS pGlobals;

//--------------------------------- wyTheme APIs

//allocate and initialize the theme object
wyBool 
wyTheme::Init()
{ 
    THEMEINFO   ti;
    wyString    tempstr;
    
    //check whether we have a theme set in ini, if not fill the flat theme
    if(wyTheme::GetSetThemeInfo(PROBE_THEME) == wyFalse)
    {
        //set the theme resource identifier as the file name
        ti.m_filename.SetAs(STRINGIZE(METROTHEME));

        //mark it as resource theme
        ti.m_type = RESOURCE_THEME;

        //set the theme
        wyTheme::GetSetThemeInfo(SET_THEME, &ti);
    }

    //create brush for button face 
    wyTheme::m_hbrushbuttonface = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

    //allocate the theme
    if(wyTheme::Allocate() == wyFalse)
    {
        return wyFalse;
    }

    //initialize the theme
    if(wyTheme::m_theme->InitTheme() == wyFalse)
    {
        //on failiure release the theme and return
        wyTheme::Release();
        return wyFalse;
    }

    //succesfull
    return wyTrue;
}

//free the allocated resources
void 
wyTheme::FreeResources()
{
    //delete button face brush
    if(wyTheme::m_hbrushbuttonface)
    {
        DeleteBrush(wyTheme::m_hbrushbuttonface);
        wyTheme::m_hbrushbuttonface = NULL;
    }

    //release the theme object
    wyTheme::Release();
}

///function to get the resource themes and file themes as an array of themes
wyInt32 
wyTheme::GetThemes(LPTHEMEINFO* pthemeinfo)
{
    wyString        themedir;
    WIN32_FIND_DATA fd;
    HANDLE          hfile;
    wyString        temp;
    BOOL            ret;
    wyInt32         i = -1, size, j;
    THEMEINFO       ti;
    tinyxml2::XMLDocument*  ptx;
	wyInt32         themes[] = {0, AZURETHEME,METROTHEME};

    //first initialize the array pointer
    *pthemeinfo = NULL;

    //get theme directory
    wyTheme::SetThemeDir(&themedir);

    //load default and resource themes
    for(j = 0, size = sizeof(themes)/sizeof(themes[0]); j < size; ++j)
    {
        //initialize the element
        ti.m_type = RESOURCE_THEME;
        ti.m_filename.Sprintf("%d", themes[j]);
        ti.m_name.SetAs(_("Default"));

        //xml node
        ptx = NULL;

        //load the theme. if it is the default theme, no need to load. in case we load the team, make sure to delete the document pointer returned
        if(themes[j] == 0 || (ptx = wyTheme::LoadTheme(&themedir, &ti)))
        {
            i++;

            //allocate/reallocate the array to fit the number of themes
            if(*pthemeinfo)
            {
                *pthemeinfo = (LPTHEMEINFO)realloc(*pthemeinfo, (i + 1) * sizeof(THEMEINFO));
            }
            else
            {
                *pthemeinfo = (LPTHEMEINFO)malloc(sizeof(THEMEINFO));
            }

            //allocat the theme object. carefull.. we are using placement new here
            new(*pthemeinfo + i) THEMEINFO;

            //set the properties
            (*pthemeinfo + i)->m_filename.SetAs(ti.m_filename);
            (*pthemeinfo + i)->m_type = themes[j] == 0 ? NO_THEME : RESOURCE_THEME;
            (*pthemeinfo + i)->m_name.SetAs(ti.m_name.GetString());

            //delete the document pointer
            delete ptx;
        }
    }

    //if we have valid theme directory, loa
    if(themedir.GetLength())
    {
        //now load file themes
        temp.Sprintf("%s*.xml", themedir.GetString());

        //find the first file in the directory
        ret = ((hfile = FindFirstFile(temp.GetAsWideChar(), &fd)) == INVALID_HANDLE_VALUE) ? FALSE : TRUE;
    
        //while there are files in the directory
        while(ret)
        {
            //set the properties
            ti.m_type = FILE_THEME;
            ti.m_filename.SetAs(fd.cFileName);
            ti.m_name.Clear();
        
            //load the theme from the file
            if((ptx = wyTheme::LoadTheme(&themedir, &ti)))
            {
                ++i;

                //reallocate the array
                *pthemeinfo = (LPTHEMEINFO)realloc(*pthemeinfo, (i + 1) * sizeof(THEMEINFO));

                //set the properties
                new(*pthemeinfo + i) THEMEINFO;
                (*pthemeinfo + i)->m_filename.SetAs(fd.cFileName);
                (*pthemeinfo + i)->m_type = FILE_THEME;
                (*pthemeinfo + i)->m_name.SetAs(ti.m_name.GetString());

                //delete the document pointer
                delete ptx;
            }
        
            //find the next file
            ret = FindNextFile(hfile, &fd);
        }

        if(hfile)
        {
            FindClose(hfile);
        }
    }

    //return the count
    return i + 1;
}

//free the theme array
void
wyTheme::FreeThemes(LPTHEMEINFO pthemeinfo, wyInt32 count)
{
    wyInt32 i;
    
    //if it is valid pointer
    if(pthemeinfo)
    {
        //loop throught the array
        for(i = 0; i < count; ++i)
        {
            //since we allocated the array elements using placement new, we have to call the destructor explicitly to free the members
            (pthemeinfo + i)->m_filename.~wyString();
            (pthemeinfo + i)->m_name.~wyString();
        }

        //free the array
        free(pthemeinfo);
    }
}

//function to get the active theme info, if any
const LPTHEMEINFO 
wyTheme::GetActiveThemeInfo()
{
    if(wyTheme::m_theme)
    {
        return &wyTheme::m_theme->m_themeinfo;
    }

    return NULL;
}

//function to get the brush curresponding to the index specified
wyBool
wyTheme::GetBrush(wyInt32 index, HBRUSH* phbrush)
{
    //if we have valid theme loaded
    if(wyTheme::m_theme)
    {
        //return the theme brush
        return wyTheme::m_theme->GetThemeBrush(index, phbrush);
    }
    //else, if it is toolbar brush or button brush
    else if(index == BRUSH_BTNFACE || index == BRUSH_TOOLBAR)
    {
        if(wyTheme::m_hbrushbuttonface)
        {
            //se the static button brush
            *phbrush = wyTheme::m_hbrushbuttonface;
            return wyTrue;
        }
    }
    
    return wyFalse;
}

//function to check whether the system menu is enabled 
wyBool
wyTheme::IsSysmenuEnabled(HWND hwnd)
{
    //if the window handle is specified
    if(hwnd)
    {
        //if we have valid system menu, return wyTrue
        if(GetSystemMenu(hwnd, FALSE))
        {
            return wyTrue;
        }
        
        //no system menu for the window
        return wyFalse;
    }

    //in case no valid window handle given, we return theme's system menu property
    if(wyTheme::m_theme)
    {
        return wyTheme::m_theme->m_issysmenu;
    }
    
    //default will be wyTrue
    return wyTrue;
}

//function to get the pen for the index specified
wyBool
wyTheme::GetPen(wyInt32 index, HPEN* phpen)
{
    //if we have valid theme
    if(wyTheme::m_theme)
    {
        //set the theme pen
        return wyTheme::m_theme->GetThemePen(index, phpen);
    }
    
    return wyFalse;
}

//get tab colors for the index specified
wyBool
wyTheme::GetTabColors(wyInt32 index, LPTABCOLORINFO pcolorinfo)
{
    //if we have valid theme
    if(wyTheme::m_theme)
    {
        //set the theme tab colors
        return wyTheme::m_theme->GetThemeTabColors(index, pcolorinfo);
    }

    return wyFalse;
}

//function to get the hyperlink color for the index specified
wyBool
wyTheme::GetLinkColor(wyInt32 index, COLORREF* plinkcolor)
{
    //if valid theme
    if(wyTheme::m_theme)
    {
        //get the theme link colors
        return wyTheme::m_theme->GetThemeLinkColor(index, plinkcolor);
    }

    return wyFalse;
}

//function to get dual color for the index specified
wyBool
wyTheme::GetDualColor(wyInt32 index, DUALCOLOR* pdualcolor)
{
    //if we have valid theme
    if(wyTheme::m_theme)
    {
        //get the theme dual color
        return wyTheme::m_theme->GetThemeDualColor(index, pdualcolor);
    }
    
    return wyFalse;
}

//function to set gradient in the rectangle given
void
wyTheme::SetGradient(HDC hdc, RECT* prect, DUALCOLOR* pdualcol, COLORREF* pframecolor)
{
    TRIVERTEX       tv[2] = {0};
    GRADIENT_RECT   grect = {0, 1};
    HBRUSH          hbr;

    //set the first color
    tv[0].Red = GetRValue(pdualcol->m_color1) << 8;
    tv[0].Green = GetGValue(pdualcol->m_color1) << 8;
    tv[0].Blue = GetBValue(pdualcol->m_color1) << 8;

    //set the second color
    tv[1].Red = GetRValue(pdualcol->m_color2) << 8;
    tv[1].Green = GetGValue(pdualcol->m_color2) << 8;
    tv[1].Blue = GetBValue(pdualcol->m_color2) << 8;

    //set the cordinates
    tv[0].x = prect->left;
    tv[0].y = prect->top;
    tv[1].x = prect->right;
    tv[1].y = prect->bottom;

    //fill the gradient
    GradientFill(hdc, tv, 2, &grect, 1, GRADIENT_FILL_RECT_V);

    //if we want to frame the rectangle
    if(pframecolor)
    {
        //create a solid brush
        hbr = CreateSolidBrush(*pframecolor);

        //frame the rect
        FrameRect(hdc, prect, hbr);

        //free the brush
        DeleteObject(hbr);
    }   
}

//function to draw the button caption with the color given, applicable to check boxes only
wyInt32
wyTheme::PaintButtonCaption(LPARAM lparam, LPDUALCOLOR pdc)
{
    wyWChar         str[256];
    LPNMHDR         lpnmhdr = (LPNMHDR)lparam;
    LPNMCUSTOMDRAW  lpnmcd;
    LONG            style;
    RECT            rc;
    wyBool          flag = wyFalse;
    COLORREF        clr = 0;

    //if the notification is not about custom drawing
    if(lpnmhdr->code != NM_CUSTOMDRAW)
    {        
        return -1;
    }

    //get the class name of the window
    GetClassName(lpnmhdr->hwndFrom, str, 255);
    
    //if it is not button
    if(wcsicmp(str, L"button"))
    {
        return -1;
    }

    lpnmcd = (LPNMCUSTOMDRAW)lparam;

    //if the paint is about to happen
    if(lpnmcd->dwDrawStage == CDDS_PREPAINT)
    {
        //get the window style
        style = GetWindowLongPtr(lpnmcd->hdr.hwndFrom, GWL_STYLE);

        //if it is check box control
        if((style & BS_AUTOCHECKBOX) || (style & BS_CHECKBOX) || (style & BS_AUTORADIOBUTTON))
        {
            //if the window is enabled
            if(IsWindowEnabled(lpnmcd->hdr.hwndFrom))
            {
                //se the enabled color
                if(pdc->m_flag & 1)
                {
                    clr = pdc->m_color1;
                    flag = wyTrue;
                }
            }
            else
            {
                //set the disabled color
                if(pdc->m_flag & 2)
                {
                    clr = pdc->m_color2;
                    flag = wyTrue;
                }
            }

            if(flag == wyTrue)
            {
                //set the text color
                clr = SetTextColor(lpnmcd->hdc, clr);

                //get window text
                GetWindowText(lpnmcd->hdr.hwndFrom, str, 255);

                //get the rect
                rc = lpnmcd->rc;

                //get check box size
                rc.left += GetSystemMetrics(SM_CXMENUCHECK) + GetSystemMetrics(SM_CXEDGE);

                //draw the text after the check box rect
                DrawText(lpnmcd->hdc, str, -1, &rc, DT_SINGLELINE | DT_VCENTER);

                //restore the text color
                clr = SetTextColor(lpnmcd->hdc, clr);
                
                return CDRF_SKIPDEFAULT;
            }
        }
    }

    return -1;
}

//function to draw toolbar
wyInt32
wyTheme::DrawToolBar(LPARAM lparam, wyBool ismaintoolbar)
{
    wyWChar             classname[256];
    LPNMHDR             lpnmhdr = (LPNMHDR)lparam;
    LPNMTBCUSTOMDRAW    lpnmtbcd;
    DUALCOLOR           dc = {0}, dc2 = {0};
    HBRUSH              hbr;

    //if the notification is not about custom dra
    if(lpnmhdr->code != NM_CUSTOMDRAW)
    {
        return -1;
    }

    GetClassName(lpnmhdr->hwndFrom, classname, 255);
    
    //check if the window is a toolbar
    if(wcsicmp(classname, TOOLBARCLASSNAME))
    {
        return -1;
    }

    lpnmtbcd = (LPNMTBCUSTOMDRAW)lparam;

    switch(lpnmtbcd->nmcd.dwDrawStage)
    {
        //handle painting the window
        case CDDS_PREPAINT:
            //get th brush
            if(wyTheme::GetBrush(ismaintoolbar == wyTrue ? BRUSH_MAINTOOLBAR : BRUSH_TOOLBAR, &hbr))
            {
                //fill the rectangle with the brush
                FillRect(lpnmtbcd->nmcd.hdc, &lpnmtbcd->nmcd.rc, hbr);
            }
            
            //return 
            return CDRF_NOTIFYITEMDRAW;

        //handle button painting, we fill the background of the button with a custom color
        case CDDS_ITEMPREPAINT:
            //get the dual color 
            if(wyTheme::GetDualColor(ismaintoolbar == wyTrue ? DUAL_COLOR_MAINTOOLBAR : DUAL_COLOR_TOOLBAR, &dc))
            {
                if(!(lpnmtbcd->nmcd.uItemState & CDIS_DISABLED) && 
                    !(lpnmtbcd->nmcd.uItemState & CDIS_GRAYED) &&
                    !(lpnmtbcd->nmcd.uItemState & CDIS_INDETERMINATE) &&
                    !(lpnmtbcd->nmcd.uItemState & CDIS_SELECTED) &&
                    (lpnmtbcd->nmcd.uItemState & CDIS_HOT))
                {
                    //set the gradient
                    if(wyTheme::GetDualColor(ismaintoolbar == wyTrue ? DUAL_COLOR_MAINTOOLSELECTED : DUAL_COLOR_TOOLBARSELECTED, &dc2))
                    {
                        wyTheme::SetGradient(lpnmtbcd->nmcd.hdc, &lpnmtbcd->nmcd.rc, &dc, &dc2.m_color1);
                    }
                    else
                    {
                        wyTheme::SetGradient(lpnmtbcd->nmcd.hdc, &lpnmtbcd->nmcd.rc, &dc, &dc.m_color2);
                    }
                    
                    return TBCDRF_NOBACKGROUND;
                }
                else if((lpnmtbcd->nmcd.uItemState & CDIS_CHECKED) || (lpnmtbcd->nmcd.uItemState & CDIS_SELECTED))
                {
                    if(wyTheme::GetDualColor(ismaintoolbar == wyTrue ? DUAL_COLOR_MAINTOOLSELECTED : DUAL_COLOR_TOOLBARSELECTED, &dc2))
                    {
                        dc.m_color1= dc.m_color2;
                    }

                    //set the gradient
                    wyTheme::SetGradient(lpnmtbcd->nmcd.hdc, &lpnmtbcd->nmcd.rc, &dc, &dc2.m_color1);

                    return TBCDRF_NOBACKGROUND;
                }
            }

            break;
    }

    return -1;
}

//function to superclass the edit control
void 
wyTheme::SubclassControls()
{
    HWND hwnd;

    hwnd = CreateWindow(L"EDIT", L"", WS_POPUP, 0, 0, 0, 0, NULL, 0, GetModuleHandle(NULL), 0);
    m_origeditclassproc = (WNDPROC)SetClassLongPtr(hwnd, GCLP_WNDPROC, (LONG_PTR)SuperClassedEditProc);
    DestroyWindow(hwnd);
}

//function to draw status bar part
void 
wyTheme::DrawStatusBarPart(LPDRAWITEMSTRUCT lpds, const wyWChar* str)
{
    COLORREF    color = 0;
    wyInt32     mode;
    HBRUSH      hbr;
    DUALCOLOR   dc;

    //if we have valid string to draw
    if(str)
    {
        //get the framewindow brush
        if(wyTheme::GetBrush(BRUSH_FRAMEWINDOW, &hbr))
        {
            //fill rect
            FillRect(lpds->hDC, &lpds->rcItem, hbr);
        }
        //if no frame window brush, fill with button brush
        else
        {
            //fill rect
            FillRect(lpds->hDC, &lpds->rcItem, wyTheme::m_hbrushbuttonface);
        }

        //set the bg mode to transparent
        mode = SetBkMode(lpds->hDC, TRANSPARENT);

        //get the framewindow text colors
        wyTheme::GetDualColor(DUAL_COLOR_FRAMEWINDOWTEXT, &dc);

        //set text color
        if(dc.m_flag & 1)
        {
            color = SetTextColor(lpds->hDC, dc.m_color1);
        }

        //draw text
        DrawText(lpds->hDC, str, -1, &lpds->rcItem, DT_LEFT | DT_EXPANDTABS | DT_HIDEPREFIX | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

        //restore text color
        if(dc.m_flag & 1)
        {
            SetTextColor(lpds->hDC, color);
        }

        //restore bg mode
        SetBkMode(lpds->hDC, mode);
    }
}

//function to enable owner drawing for menu items
void
wyTheme::SetMenuItemOwnerDraw(HMENU hmenu, wyInt32 startitemindex, wyInt32 ignoreitemcount, wyInt32 themecolorindex, wyBool applytosubmenu)
{
    wyInt32			menucount, itemcount;
	MENUITEMINFO	mif;
    HMENU           hsubmenu;
    MENUINFO        mi = {0}; 

    //if theme color index is specified
    if(themecolorindex)
    {
        //get the bursh for the theme color specified, if no valid brush set system color brush for the menu
        if(wyTheme::GetBrush(themecolorindex, &mi.hbrBack) == wyFalse)
        {
            mi.hbrBack = GetSysColorBrush(COLOR_MENU);
        }

        //set the background
        mi.cbSize = sizeof(mi); 
        mi.fMask = MIM_BACKGROUND | ((applytosubmenu == wyTrue) ? MIM_APPLYTOSUBMENUS : 0); 
        SetMenuInfo(hmenu, &mi);
    }

    //menu item count
	menucount = GetMenuItemCount(hmenu) - ignoreitemcount;

    //loop through the menu items
	for(itemcount = startitemindex; itemcount < menucount; itemcount++)
    {
		//get the menu item info
        memset(&mif, 0, sizeof(mif));
		mif.cbSize = sizeof(mif);
        mif.fMask = MIIM_TYPE;
		mif.fType = MFT_SEPARATOR | MFT_OWNERDRAW;
		GetMenuItemInfo(hmenu, itemcount, TRUE, &mif);

        //skip if the menu item is a seperator or owner drawing is already set
        if((mif.fType & MFT_SEPARATOR) || (mif.fType & MFT_OWNERDRAW))
        {
			continue;
        }

        //set owner draw
		memset(&mif, 0, sizeof(mif));
		mif.cbSize = sizeof(mif);
        mif.fMask = MIIM_FTYPE;
        mif.fType = MFT_OWNERDRAW;
		SetMenuItemInfo(hmenu, itemcount, TRUE, &mif);

        //recurse into submenu
        if(applytosubmenu && (hsubmenu = GetSubMenu(hmenu, itemcount)))
        {
            wyTheme::SetMenuItemOwnerDraw(hsubmenu, 0, 0, themecolorindex, applytosubmenu);
        }
	}
}

//function to calculate the height and width for the menu item
wyBool
wyTheme::MeasureMenuItem(LPMEASUREITEMSTRUCT lpms, HMENU hmenu, wyBool ispopup)
{
    HDC	                hdc;
    RECT                rctext = {0}, rctab = {0}, rckey = {0};
    wyWChar             menustring[MAX_PATH + 1] = {0};
    MENUITEMINFO        mif = {0};
    HFONT               hfont;
    NONCLIENTMETRICS    ncm = {0};
    wyInt32             len = -1;
    const wyWChar*      ptr;

    //if it is not menu
    if(lpms->CtlType != ODT_MENU)
    {
        return wyFalse;
    }

    //get the menu string
    mif.cbSize = sizeof(mif);
    mif.fMask = MIIM_STRING | MIIM_FTYPE;
    mif.fType = MFT_STRING;
    mif.dwTypeData = menustring;
    mif.cch = MAX_PATH;
    GetMenuItemInfo(hmenu, lpms->itemID, FALSE, &mif);

    if(!menustring[0])
    {
        return wyFalse;
    }

    //get the nonclient metrics. required for calculating the size
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
    
    //create the font for non client area. then get the window dc and select the font in the dc
    hfont = CreateFontIndirect(&ncm.lfMenuFont);
    hdc = GetWindowDC(pGlobals->m_pcmainwin->m_hwndmain);
    hfont = (HFONT)SelectObject(hdc, hfont);

    //locate the tab charector that seperates the menu item label and key
    if((ptr = wcsstr(menustring, L"\t")))
    {
        //adjust the length
        len = ptr - menustring;

        //calculate the size of tab charector and shortcut
        DrawText(hdc, L"\t", -1, &rctab, DT_SINGLELINE | DT_EXPANDTABS | DT_CALCRECT);
        DrawText(hdc, ptr + 1, -1, &rckey, DT_SINGLELINE | DT_EXPANDTABS | DT_CALCRECT);
    }

    //calculate the size of menu item label
    DrawText(hdc, menustring, len, &rctext, DT_SINGLELINE | DT_EXPANDTABS | DT_CALCRECT);    
	
	//both icon and text must fit
	lpms->itemHeight = max(MENUBMPY + 4, (rctext.bottom - rctext.top) + 2);

    //if it is not a popup menu
    if(ispopup == wyFalse)
    {
        //set the width
        lpms->itemWidth = 2 + (rctext.right - rctext.left);
    }
    else
    {
        //set the width
        lpms->itemWidth = 2 + MENUBMPX + 2 + (rctext.right - rctext.left) + 6;
        lpms->itemWidth += (rctab.right - rctab.left) + (rckey.right - rckey.left);
    }
    
    //restore/free gdi objects
    hfont = SelectFont(hdc, hfont);
    ReleaseDC(pGlobals->m_pcmainwin->m_hwndmain, hdc);
	DeleteFont(hfont);

    return wyTrue;
}

//function to draw menu item
wyBool
wyTheme::DrawMenuItem(LPDRAWITEMSTRUCT lpds, HMENU hmenubar)
{
    wyInt32         dcstate;
	RECT            rccheck, rctext;
	HBRUSH          brhighlight, hbrtheme = NULL;
    wyInt32         image = -1, noffy;
	yogIcons        icontosearch = {0};
	yogIcons*       searched;
    HICON		    hicon;
    wyUInt32        fformat;
    MENUINFO        mi = {0};
    HMENU           hmenu;
    MENUITEMINFO    mif = {0};
    DUALCOLOR       dc = {0}, selmenutext = {0}, menutext = {0}, dc2 = {0};
    wyWChar         menustring[MAX_PATH + 1];
    const wyWChar*  ptr;
    wyInt32         len = -1;
    COLORREF        colortext = 0;

    //menu handle
    hmenu = (HMENU)lpds->hwndItem;

    //save the dc state
	dcstate = SaveDC(lpds->hDC);

    //set bg mode to transparent
	SetBkMode(lpds->hDC, TRANSPARENT);

    //get highlight color
	brhighlight = GetSysColorBrush(COLOR_HIGHLIGHT);
    
    //get menu theme bg color
    if(wyTheme::GetDualColor(hmenu != hmenubar ? DUAL_COLOR_MENU : DUAL_COLOR_MENUBAR, &dc))
    {
        if(dc.m_flag & 1)
        {
            hbrtheme = CreateSolidBrush(dc.m_color1);
            brhighlight = hbrtheme;
        }
    }

    //get the menu bg color
    mi.cbSize = sizeof(mi);
    mi.fMask = MIM_BACKGROUND;
    GetMenuInfo(hmenu, &mi);

    //set the background brush
    mi.hbrBack = mi.hbrBack ? mi.hbrBack : (hmenu == hmenubar ? GetSysColorBrush(COLOR_BTNFACE) : GetSysColorBrush(COLOR_MENU));
    
    //if the item is selected/highlighted
    if((lpds->itemState & ODS_SELECTED) || (lpds->itemState & ODS_HOTLIGHT))
	{
        //set the gradient if we have dual color
        if(hbrtheme && (dc.m_flag & 2))
        {
            if(wyTheme::GetDualColor(hmenu != hmenubar ? DUAL_COLOR_MENUSELECTED : DUAL_COLOR_MENUBARSELECTED, &dc2))
            {
                wyTheme::SetGradient(lpds->hDC, &lpds->rcItem, &dc, &dc2.m_color1);
            }
            else
            {
                wyTheme::SetGradient(lpds->hDC, &lpds->rcItem, &dc, &dc.m_color2);
            }
        }
        //else fill the highlight color
        else
        {
            FillRect(lpds->hDC, &lpds->rcItem, brhighlight);
		    FrameRect(lpds->hDC, &lpds->rcItem, brhighlight);
        }
	}
	else
    {
        //fill the background color
		FillRect(lpds->hDC, &lpds->rcItem, mi.hbrBack);
    }

    //get menu item info
	mif.cbSize = sizeof(mif);
    mif.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_ID;
    mif.fType = MFT_STRING;
    mif.dwTypeData = menustring;
    mif.cch = MAX_PATH;
    GetMenuItemInfo(hmenu, lpds->itemID, FALSE, &mif);

	icontosearch.m_iconid = 0; 
    icontosearch.m_menuid = mif.wID;
    
    //look for image associated with command ID
    if(!(searched = (yogIcons*)bsearch(&icontosearch, pGlobals->m_pcmainwin->m_icons, pGlobals->m_pcmainwin->m_numicons, sizeof(yogIcons ), compareid)))
    {
		image = -1;
    }
	else
    {
		image = searched->m_menuid;
    }

	//center icon vertically
    noffy = ((lpds->rcItem.bottom - lpds->rcItem.top) - MENUBMPY) / 2;

    //if it is popup menu
    if(hmenu != hmenubar)
    {
        //if the menu item is checked
        if(lpds->itemState & ODS_CHECKED)
	    {
            rccheck.left = 2; 
            rccheck.top = lpds->rcItem.top + noffy;
		    rccheck.right = 2 + MENUBMPX; 
            rccheck.bottom = lpds->rcItem.top + MENUBMPY + noffy;

            //load the check mark icon and draw it
		    hicon =(HICON)LoadImage(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_CHECK), IMAGE_ICON, MENUBMPX, MENUBMPY, LR_DEFAULTCOLOR);
		    DrawIconEx(lpds->hDC, lpds->rcItem.left + 2, lpds->rcItem.top + noffy, hicon, MENUBMPX, MENUBMPY, 0, NULL, DI_NORMAL);

		    DestroyIcon(hicon);
	    }
        //if no check mark and there is an icon associated
        else if(image >= 0)
	    {
		    if(searched->m_iconid != 0)
            {
                //load the icon and draw it
			    hicon = (HICON)LoadImage(pGlobals->m_hinstance, MAKEINTRESOURCE(searched->m_iconid), IMAGE_ICON, MENUBMPX, MENUBMPY, LR_DEFAULTCOLOR);
			    DrawIconEx(lpds->hDC, lpds->rcItem.left + 2, lpds->rcItem.top + noffy, hicon, MENUBMPX, MENUBMPY, 0, NULL, DI_NORMAL);

			    DestroyIcon(hicon);
		    }
	    }
    }

    //set the rectangle
    rctext = lpds->rcItem;

    //for popup menu
    if(hmenu != hmenubar)
    {
	    rctext.left	+= (MENUBMPX + 6);
	    rctext.right -= (MENUBMPY);
        fformat = DT_SINGLELINE | DT_VCENTER;
    }
    //for menu bar draw it in single line with the text centered in the rectangle
    else
    {
        fformat = DT_SINGLELINE | DT_VCENTER | DT_CENTER;
    }

    //get the menu text and selected menu text
    wyTheme::GetDualColor(hmenu != hmenubar ? DUAL_COLOR_MENUTEXT : DUAL_COLOR_MENUBARTEXT, &menutext);
    wyTheme::GetDualColor(hmenu != hmenubar ? DUAL_COLOR_SELMENUTEXT : DUAL_COLOR_SELMENUBARTEXT, &selmenutext);

    //if the menu item state is disabled
    if((lpds->itemState & ODS_DISABLED) || (lpds->itemState & ODS_GRAYED))
    {
        //set the text color
        colortext = ((lpds->itemState & ODS_SELECTED) || (lpds->itemState & ODS_HOTLIGHT)) 
            ? (selmenutext.m_flag & 2) ? selmenutext.m_color2 : GetSysColor(COLOR_3DSHADOW)
            : (menutext.m_flag & 2) ? menutext.m_color2 : GetSysColor(COLOR_GRAYTEXT);
    }
    else
    {
        //set the text color
        colortext = ((lpds->itemState & ODS_SELECTED) || (lpds->itemState & ODS_HOTLIGHT)) 
            ? (selmenutext.m_flag & 1) ? selmenutext.m_color1 : GetSysColor(COLOR_HIGHLIGHTTEXT)
            : (menutext.m_flag & 1) ? menutext.m_color1 : GetSysColor(COLOR_MENUTEXT);
    }

    //set dc text color
	SetTextColor(lpds->hDC, colortext);
    
    //now check whether we have a tab charecter
    if((ptr = wcsstr(menustring, L"\t")))
    {
        //adjuset the length
        len = ptr - menustring;

        //draw the keyboard shortcut as right aligned
        DrawText(lpds->hDC, ptr + 1, -1, &rctext, DT_SINGLELINE | DT_EXPANDTABS | DT_RIGHT | DT_VCENTER);
    }

    //draw the text
    DrawText(lpds->hDC, menustring, len, &rctext, fformat | ((lpds->itemState & ODS_NOACCEL) ? DT_HIDEPREFIX : 0));
	
    //restore dc
	RestoreDC(lpds->hDC, dcstate);

    //delete the brush
    if(hbrtheme)
    {
        DeleteBrush(hbrtheme);
    }

	return wyTrue;
}

//---------------------------------

//constructor
wyTheme::wyTheme()
{
    //initialize everything
    m_issysmenu = wyTrue;
    m_hframebrush = NULL;
    m_hhsplitterbrush = NULL;
    m_hhsplitterpen = NULL;
    m_hmdibrush = NULL;
    m_hmenubrush[0] = m_hmenubrush[1] = NULL;
    m_hversplitterbrush = NULL;
    m_htoolbarbrush = NULL;
    m_hmaintoolbarbrush = NULL;
    m_hmdichildbrush = NULL;
    SetThemeDir(&m_themedir);    
}

//destructor
wyTheme::~wyTheme()
{
    if(m_hframebrush != NULL)
    {
        DeleteBrush(m_hframebrush);
    }

    if(m_htoolbarbrush != NULL)
    {
        DeleteBrush(m_htoolbarbrush);
    }

    if(m_hmaintoolbarbrush != NULL)
    {
        DeleteBrush(m_hmaintoolbarbrush);
    }

    if(m_hhsplitterbrush != NULL)
    {
        DeleteBrush(m_hhsplitterbrush);
    }

    if(m_hhsplitterpen != NULL)
    {
        DeleteBrush(m_hhsplitterpen);
    }

    if(m_hmdibrush != NULL)
    {
        DeleteBrush(m_hmdibrush);
    }

    if(m_hmdichildbrush != NULL)
    {
        DeleteBrush(m_hmdichildbrush);
    }

    if(m_hmenubrush[0] != NULL)
    {
        DeleteBrush(m_hmenubrush[0]);
    }

    if(m_hmenubrush[1] != NULL)
    {
        DeleteBrush(m_hmenubrush[1]);
    }

    if(m_hversplitterbrush != NULL)
    {
        DeleteBrush(m_hversplitterbrush);
    }

    if(m_hhsplitterpen != NULL)
    {
        DeletePen(m_hhsplitterpen);
    }

    if(m_hvsplitterpen != NULL)
    {
        DeletePen(m_hvsplitterpen);
    }
}

//function to set the theme directory
void 
wyTheme::SetThemeDir(wyString* pstr)
{
    wyWChar path[MAX_PATH + 1];

	if(!pGlobals->m_configdirpath.GetLength())
	{	
		pstr->Clear();

		if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path))) 
		{
			pstr->SetAs(path);
			pstr->AddSprintf("\\SQLyog\\%s\\", THEMEDIR);
		}
	}
	else
	{
		pstr->SetAs(pGlobals->m_configdirpath.GetString());
		pstr->AddSprintf("\\%s\\", THEMEDIR);
	}
}

//allocat the theme object
wyBool
wyTheme::Allocate()
{
    //delete and recreate the object
    delete wyTheme::m_theme;
    wyTheme::m_theme = new(std::nothrow) wyTheme();

    if(wyTheme::m_theme == NULL)
    {
        return wyFalse;
    }

    return wyTrue;
}

//function to delete the theme object
void
wyTheme::Release()
{
    delete wyTheme::m_theme;
    wyTheme::m_theme = NULL;
}

//function to initialize the theme
wyBool
wyTheme::InitTheme()
{
    wyString        themefile;
    tinyxml2::XMLDocument*  ptixd;
    tinyxml2::XMLElement*   ele;
    COLORREF        color;
    const wyChar*   attr;

    //load theme
    if(wyTheme::GetSetThemeInfo(GET_THEME, &m_themeinfo, &m_themedir) == wyFalse || 
        !(ptixd = wyTheme::LoadTheme(&m_themedir, &m_themeinfo)))
    {
        return wyFalse;
    }

    ele = ptixd->RootElement();

    //set system menu property
    if((attr = ele->Attribute("sysmenu")))
    {
        if(!strcmpi(attr, "true"))
        {
            m_issysmenu = wyTrue;
        }
        else if(!strcmpi(attr, "no"))
        {
            m_issysmenu = wyFalse;
        }
    }

    //create brushes
    m_hmdibrush = CreateBgBrush(ele, ELE_MDICLIENT);
    m_hframebrush = CreateBgBrush(ele, ELE_FRAMEWINDOW);
    m_htoolbarbrush = CreateBgBrush(ele, ELE_TOOLBAR);
    m_hmaintoolbarbrush = CreateBgBrush(ele, "maintoolbar");
    m_hmenubrush[MENUBARINDEX] = CreateBgBrush(ele, ELE_MENUBAR);
    m_hmenubrush[MENUINDEX] = CreateBgBrush(ele, ELE_MENU);
    m_hhsplitterbrush = CreateBgBrush(ele, ELE_HSPLITTER);
    m_hversplitterbrush = CreateBgBrush(ele, ELE_VSPLITTER);
    m_hmdichildbrush = CreateBgBrush(ele, "mdichild");
    
    //create tab colors
    CreateTabColors(ele, ELE_CONNTAB, &m_conncolorinfo);
    CreateTabColors(ele, ELE_QUERYTAB, &m_qtcolorinfo);
    CreateTabColors(ele, ELE_RESTAB, &m_tmcolorinfo);
    CreateTabColors(ele, "tabletab", &m_tabletabcolorinfo);
    
    //create dual colors
    SetDualColors(ele, ELE_MENUBAR, FORGROUND, COLOR1, COLOR2, &m_colormenuhighlight[MENUBARINDEX]);
    SetDualColors(ele, ELE_MENUBAR, "text", ACTIVE, INACTIVE, &m_colormenutext[MENUBARINDEX]);
    SetDualColors(ele, ELE_MENUBAR, "seltext", ACTIVE, INACTIVE, &m_colorselmenutext[MENUBARINDEX]);
    SetDualColors(ele, ELE_MENU, FORGROUND, COLOR1, COLOR2, &m_colormenuhighlight[MENUINDEX]);
    SetDualColors(ele, ELE_MENU, "text", ACTIVE, INACTIVE, &m_colormenutext[MENUINDEX]);
    SetDualColors(ele, ELE_MENU, "seltext", ACTIVE, INACTIVE, &m_colorselmenutext[MENUINDEX]);
    SetDualColors(ele, ELE_TOOLBAR, FORGROUND, COLOR1, COLOR2, &m_colortoolbarhighlight);
    SetDualColors(ele, "maintoolbar", FORGROUND, COLOR1, COLOR2, &m_colormaintoolbarhighlight);
    SetDualColors(ele, ELE_TOOLBAR, "selected", COLOR1, COLOR2, &m_colortoolbarselected);
    SetDualColors(ele, "maintoolbar", "selected", COLOR1, COLOR2, &m_colormaintoolbarselected);
    SetDualColors(ele, ELE_MENU, "selected", COLOR1, COLOR2, &m_colormenuselected);
    SetDualColors(ele, ELE_MENUBAR, "selected", COLOR1, COLOR2, &m_colormenubarselected);
    SetDualColors(ele, ELE_FRAMEWINDOW, "text", ACTIVE, INACTIVE, &m_colorframewindowtext);
    SetDualColors(ele, ELE_TOOLBAR, "text", ACTIVE, INACTIVE, &m_colortoolbartext);
    SetDualColors(ele, "maintoolbar", "text", ACTIVE, INACTIVE, &m_colormaintoolbartext);
    SetDualColors(ele, ELE_CONNTAB, "text", ACTIVE, INACTIVE, &m_colorconntabtext);
    SetDualColors(ele, ELE_QUERYTAB, "text", ACTIVE, INACTIVE, &m_colorquerytabtext);
    SetDualColors(ele, ELE_RESTAB, "text", ACTIVE, INACTIVE, &m_colorrestabtext);
    SetDualColors(ele, "tabletab", "text", ACTIVE, INACTIVE, &m_colortabletabtext);
    SetDualColors(ele, "mdichild", "hyperlink", COLOR1, COLOR2, &m_mdichildlink);
    SetDualColors(ele, ELE_FRAMEWINDOW, "hyperlink", COLOR1, COLOR2, &m_framewindowlink);
    SetDualColors(ele, "mdichild", "text", ACTIVE, INACTIVE, &m_colormdichildtext);
    m_hvsplitterpen = m_hhsplitterpen = NULL;

    //create vertical splitter pen
    if(GetColorInfo(ele->FirstChildElement(ELE_VSPLITTER), LINE, COLOR1, &color))
    {
        m_hvsplitterpen = CreatePen(PS_SOLID, 2, color);
    }

    //create horizontal splitter pen
    if(GetColorInfo(ele->FirstChildElement(ELE_HSPLITTER), LINE, COLOR1, &color))
    {
        m_hhsplitterpen = CreatePen(PS_SOLID, 2, color);
    }

    //delete the document pointer
    delete ptixd;

    return wyTrue;
}

//function to get/set/probe themes
wyBool 
wyTheme::GetSetThemeInfo(wyInt32 mode, LPTHEMEINFO pthemeinfo, wyString* pthemedir)
{
    wyWChar         directory[MAX_PATH];
    wyWChar*        lpfileport = NULL;
    wyString        dirstr, themefile;
    wyInt32         themetype;
    tinyxml2::XMLDocument*  ptx = NULL;

    //search for ini
    if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyFalse)
    {
        return wyFalse;
    }

    //set the ini path
    dirstr.SetAs(directory);

    //if we are getting or probing the theme
    if(mode != SET_THEME)
    {
        //if it is get theme, and a valid theme info pointer is supplied
        if(mode == GET_THEME && pthemeinfo)
        {
            //initialize the theme info
            pthemeinfo->m_filename.SetAs("0");
            pthemeinfo->m_type = NO_THEME;
            pthemeinfo->m_name.Clear();
        }

        //read theme info
        wyIni::IniGetString(ACTIVETHEME_SECTION, "ThemeFile", "", &themefile, dirstr.GetString());
        themetype = wyIni::IniGetInt(ACTIVETHEME_SECTION, "ThemeType", -1, dirstr.GetString());

        //if we are justing probing the theme
        if(mode == PROBE_THEME)
        {
            //if no theme file name or the theme type is invalid, return wyFalse indicating that no theme present
            if(!themefile.GetLength() || 
                (themetype != NO_THEME && themetype != RESOURCE_THEME && themetype != FILE_THEME))
            {
                return wyFalse;
            }

            //there is a valid theme
            return wyTrue;
        }

        //theme type is invalid or the them file is invalid
        if((themetype != RESOURCE_THEME && themetype != FILE_THEME) || 
            !themefile.GetLength())
        {            
            return wyFalse;
        }
         
        //if theme type is resource theme and the identifier is zero, return
        if(themetype == RESOURCE_THEME && atoi(themefile.GetString()) == 0)
        {
            return wyFalse;
        }

        //if valid theme info pointer
        if(pthemeinfo)
        {
            //set the properties
            pthemeinfo->m_filename.SetAs(themefile);
            pthemeinfo->m_type = themetype;
            
            //try loading the theme file, on error reset the values
            if(!(ptx = wyTheme::LoadTheme(pthemedir, pthemeinfo)))
            {
                pthemeinfo->m_filename.SetAs("0");
                pthemeinfo->m_type = NO_THEME;
            }

            //delete the doc pointer
            delete ptx;
        }
        
        return wyTrue;
    }

    //we are here means we need to write the files
    wyIni::IniWriteString(ACTIVETHEME_SECTION, "ThemeFile", pthemeinfo ? pthemeinfo->m_filename.GetString() : "0", dirstr.GetString());
    wyIni::IniWriteInt(ACTIVETHEME_SECTION, "ThemeType", pthemeinfo ? pthemeinfo->m_type : 0, dirstr.GetString());

    return wyTrue;
}

//function to load the theme file
tinyxml2::XMLDocument* 
wyTheme::LoadTheme(wyString* pthemedir, LPTHEMEINFO pthemeinfo)
{
    FILE*           file;
    wyString        temp;
    wyInt32         size, id;
    wyChar*         data = NULL;
    tinyxml2::XMLDocument*  ptixd;
    tinyxml2::XMLElement*   ele;
    const wyChar    *tagname, *attr;
    HRSRC           hrsc;
    HGLOBAL		    htheme;
    void*           lpvdata;

    //if it is resource theme
    if(pthemeinfo->m_type == RESOURCE_THEME)
    {
        //get the id
        id = atoi(pthemeinfo->m_filename.GetString());
        
        //find the resource and load it
        if(!(hrsc = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(id), L"THEME")) ||
            !(htheme = LoadResource(GetModuleHandle(NULL), hrsc)))
        {
            return NULL;
        }

        //lock the resource
        lpvdata = LockResource(htheme);

        //get the size of resource and allocate it
        size = SizeofResource(GetModuleHandle(0), hrsc);
        data = new wyChar[size + 1];

        //copy the data
        strncpy(data, (wyChar*)lpvdata, size);

        //unlock the resource and free it
        UnlockResource(htheme);
        FreeResource(htheme);

        data[size] = 0;
    }
    //else it is the external file
    else
    { 
        //path to file
        temp.Sprintf("%s%s", pthemedir->GetString(), pthemeinfo->m_filename.GetString());

        //open the file
        if(!(file = _wfopen(temp.GetAsWideChar(), L"rb")))
        {
            return NULL;
        }

        //find the file length and allocate the buffer
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        data = new wyChar[size + 1];

        //read the data and close the file
        fseek(file, 0, SEEK_SET);
        fread(data, sizeof(wyChar), size, file);
        fclose(file);

        data[size] = 0;
    }

    //if unsuccesfull
    if(!data)
    {
        return NULL;
    }

    //create a new xml document and set the data
    ptixd = new tinyxml2::XMLDocument();
    ptixd->Parse(data);
    delete[] data;

    //if parsing is succesful and the theme name is difined
    if(!ptixd->Error() && (ele = ptixd->RootElement()) && (tagname = ele->Value()))
    {
        //if valid sqlyog theme file
        if(!strcmp(tagname, ELE_ROOT) && (attr = ele->Attribute("name")))
        {
            temp.SetAs(attr);
            temp.LTrim();
            temp.RTrim();

            //if theme name is given, then set it and return the document pointer
            if(temp.GetLength())
            {
                pthemeinfo->m_name.SetAs(temp.GetString());
                return ptixd;
            }
        }
    }
    
    //cleanup
    delete ptixd;

    return NULL;
}

//function creates the background brush
HBRUSH 
wyTheme::CreateBgBrush(tinyxml2::XMLElement* prootele, const wyChar* element)
{
    tinyxml2::XMLElement* pele;
    COLORREF      color;
    HBRUSH        hbrush = NULL;    

    //get the background property for the element
    if((pele = prootele->FirstChildElement(element)) && (pele = pele->FirstChildElement(BACKGROUND)))
    {
        //try creating bitmap brush
        hbrush = CreateBitMapBrush(pele->Attribute(IMAGE));

        //if no bitmap brush created, try creating color brush
        if(hbrush == NULL && GetColorInfo(pele, NULL, COLOR1, &color) == wyTrue)
        {
            hbrush = CreateSolidBrush(color);
        }
    }

    return hbrush;
}

//function to set dual colors
void
wyTheme::SetDualColors(tinyxml2::XMLElement* prootele, const wyChar* element, const wyChar* subele, const wyChar* attrib1, const wyChar* attrib2, DUALCOLOR* pdualcolor)
{
    tinyxml2::XMLElement* pele;
    
    memset(pdualcolor, 0, sizeof(DUALCOLOR));

    //get the element
    if(!(pele = prootele->FirstChildElement(element)))
    {
        return;
    }

    //get the colors and adjust the flag
    pdualcolor->m_flag |= GetColorInfo(pele, subele, attrib1, &pdualcolor->m_color1) ? 1 : 0;
    pdualcolor->m_flag |= GetColorInfo(pele, subele, attrib2, &pdualcolor->m_color2) ? 2 : 0;
}

//function to create tab color
void 
wyTheme::CreateTabColors(tinyxml2::XMLElement* prootele, const wyChar* element, LPTABCOLORINFO pcolorinfo)
{
    tinyxml2::XMLElement* pele;
    const wyChar* strtemp;

    memset(pcolorinfo, 0, sizeof(TABCOLORINFO));

    //get the element
    if(!(pele = prootele->FirstChildElement(element)))
    {
        return;
    }

    //set bottom line
    if((strtemp = pele->Attribute(TABBOTTOMLINE)) && !strcmp(strtemp, "true"))
    {
        pcolorinfo->m_mask |= CTCF_BOTTOMLINE;
    }

    //set border
    if((strtemp = pele->Attribute("border")) && !strcmp(strtemp, "true"))
    {
        pcolorinfo->m_mask |= CTCF_BORDER;
        pcolorinfo->m_border = wyTrue;
    }
    
    //set colors
    pcolorinfo->m_mask |= GetColorInfo(pele, BACKGROUND, COLOR1, &pcolorinfo->m_tabbg1) ? CTCF_TABBG1 : 0;
    pcolorinfo->m_mask |= GetColorInfo(pele, BACKGROUND, COLOR2, &pcolorinfo->m_tabbg2) ? CTCF_TABBG2 : 0;
    pcolorinfo->m_mask |= GetColorInfo(pele, SEPERATOR, ACTIVE, &pcolorinfo->m_activesep) ? CTCF_ACTIVESEP : 0;
    pcolorinfo->m_mask |= GetColorInfo(pele, SEPERATOR, INACTIVE, &pcolorinfo->m_inactivesep) ? CTCF_INACTIVESEP : 0;
    pcolorinfo->m_mask |= GetColorInfo(pele, SEPERATOR, "highlight", &pcolorinfo->m_highlightsep) ? CTCF_HIGHLIGHTSEP : 0;
    pcolorinfo->m_mask |= GetColorInfo(pele, FORGROUND, COLOR1, &pcolorinfo->m_tabfg) ? CTCF_TABFG : 0;
    pcolorinfo->m_mask |= GetColorInfo(pele, FORGROUND, COLOR2, &pcolorinfo->m_hottabfg1) ? CTCF_HOTTABFG1 : 0;
    pcolorinfo->m_mask |= GetColorInfo(pele, FORGROUND, "color3", &pcolorinfo->m_hottabfg2) ? CTCF_HOTTABFG2 : 0;
    pcolorinfo->m_mask |= GetColorInfo(pele, SELTAB, COLOR1, &pcolorinfo->m_seltabfg1) ? CTCF_SELTABFG1 : 0;
    pcolorinfo->m_mask |= GetColorInfo(pele, SELTAB, COLOR2, &pcolorinfo->m_seltabfg2) ? CTCF_SELTABFG2 : 0;
    pcolorinfo->m_mask |= GetColorInfo(pele, SELTABTEXT, COLOR1, &pcolorinfo->m_seltabtext) ? CTCF_SELTABTEXT : 0;
    pcolorinfo->m_mask |= GetColorInfo(pele, TABTEXT, COLOR1, &pcolorinfo->m_tabtext) ? CTCF_TABTEXT : 0;
    pcolorinfo->m_mask |= GetColorInfo(pele, PLUSBUTTON, COLOR1, &pcolorinfo->m_tabcontrols) ? CTCF_TABCONTROLS : 0;
    pcolorinfo->m_mask |= GetColorInfo(pele, "closebutton", COLOR1, &pcolorinfo->m_closebutton) ? CTCF_CLOSEBUTTON : 0;
    pcolorinfo->m_mask |= GetColorInfo(pele, DRAGARROW, COLOR1, &pcolorinfo->m_dragarrow) ? CTCF_DRAGARROW : 0;    
    pcolorinfo->m_mask |= GetColorInfo(pele, "hyperlink", COLOR1, &pcolorinfo->m_linkcolor) ? CTCF_LINK : 0;    
}

//function to read the color from the element
wyBool
wyTheme::GetColorInfo(tinyxml2::XMLElement* pele, const wyChar* element, const wyChar* attribute, COLORREF* pcolor)
{
    tinyxml2::XMLElement*   pchildele = pele;
    const wyChar*   strcolor;

    //if we have specified a child element, then get the pointer to the element, else continue with the main element
    if(pchildele && (!element || (pchildele = pchildele->FirstChildElement(element))))
    {
        //get the attribute representing color
        if((strcolor = pchildele->Attribute(attribute)))
        {
            //scan the color
            sscanf(strcolor, "%i", pcolor);

            //check for hex notation
            if(strstr(strcolor, "0x") || strstr(strcolor, "0X"))
            {
                //convert hex to COLORREF
                *pcolor = GetColorFromHex(*pcolor);
            }
            
            return wyTrue;
        }
    }

    //failed
    return wyFalse;
}

//function to create a bitmap brush
HBRUSH 
wyTheme::CreateBitMapBrush(const wyChar* filename)
{
    HBITMAP     hbm = NULL;
    wyString    path;
    HBRUSH      hbrush;
    wyInt32     id;

    //if no filename provided
    if(!filename)
    {
        return NULL;
    }

    //if the theme type is file theme, the the image will also be a file in the disk
    if(m_themeinfo.m_type == FILE_THEME)
    {
        //set the filename path
        path.Sprintf("%s%s", m_themedir.GetString(), filename);

        //load the image from the file
        hbm = (HBITMAP)LoadImage(0, path.GetAsWideChar(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    }
    //it is a resource theme, hence the id
    else if((id = atoi(filename)))
    {
        //load the image from resource
        hbm = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(id), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
    }

    //failed to load
    if(!hbm)
    {
        return NULL;
    }

    //create the pattern brush from the bitmap
    hbrush = CreatePatternBrush(hbm);

    //delete the bitmap
    DeleteObject(hbm);

    //return the brush
    return hbrush;
}

//get the theme brush
wyBool 
wyTheme::GetThemeBrush(wyInt32 index, HBRUSH* phbrush)
{
    HBRUSH hbrush = NULL;
    
    //match the index
    switch(index)
    {
        case BRUSH_TOOLBAR:
            if((hbrush = m_htoolbarbrush))
            {
                break;
            }

            hbrush = m_hbrushbuttonface;
            break;

        case BRUSH_MAINTOOLBAR:
            if((hbrush = m_hmaintoolbarbrush))
            {
                break;
            }

            hbrush = m_hbrushbuttonface;
            break;

        case BRUSH_BTNFACE:
            hbrush = m_hbrushbuttonface;
            break;

        case BRUSH_FRAMEWINDOW:
            hbrush = m_hframebrush;
            break;

        case BRUSH_MDICLIENT:
            hbrush = m_hmdibrush;
            break;

        case BRUSH_MDICHILD:
            hbrush = m_hmdichildbrush;
            break;

        case BRUSH_HSPLITTER:
            hbrush = m_hhsplitterbrush;
            break;

        case BRUSH_VSPLITTER:
            hbrush = m_hversplitterbrush;
            break;

        case BRUSH_MENU:
            hbrush = m_hmenubrush[MENUINDEX];
            break;

        case BRUSH_MENUBAR:
            hbrush = m_hmenubrush[MENUBARINDEX];
            break;
    }

    //if no match
    if(!hbrush)
    {
        return wyFalse;
    }

    //set the brush
    if(phbrush)
    {
        *phbrush = hbrush;
    }

    return wyTrue;
}

//get the theme pen
wyBool 
wyTheme::GetThemePen(wyInt32 index, HPEN* phpen)
{
    HPEN hpen = NULL;
    
    //match the index
    switch(index)
    {
        case PEN_HSPLITTER:
            hpen = m_hhsplitterpen;
            break;

        case PEN_VSPLITTER:
            hpen = m_hvsplitterpen;
            break;
    }

    //if no match
    if(!hpen)
    {
        return wyFalse;
    }

    //set the pen
    if(phpen)
    {
        *phpen = hpen;
    }

    return wyTrue;
}

//get the tab colors
wyBool 
wyTheme::GetThemeTabColors(wyInt32 index, LPTABCOLORINFO pcolorinfo)
{
    TABCOLORINFO ci = {0};

    //match the index
    switch(index)
    {
        case COLORS_CONNTAB:
            ci = m_conncolorinfo;
            break;

        case COLORS_QUERYTAB:
            ci = m_qtcolorinfo;
            break;

        case COLORS_RESTAB:
            ci = m_tmcolorinfo;
            break;

        case COLORS_TABLETAB:
            ci = m_tabletabcolorinfo;
    }

    //if no match
    if(!ci.m_mask)
    {
        return wyFalse;
    }

    //set the tab color
    if(pcolorinfo)
    {
        *pcolorinfo = ci;
    }

    return wyTrue;
}

//function to get the hyperlink color
wyBool 
wyTheme::GetThemeLinkColor(wyInt32 index, COLORREF* plinkcolor)
{
    COLORREF* pcolor = NULL;

    //match the index
    switch(index)
    {
        case LINK_COLOR_FRAMEWINDOW:
            pcolor = (m_framewindowlink.m_flag & 1) ? &m_framewindowlink.m_color1 : NULL;
            break;

        case LINK_COLOR_MDICHILD:
            pcolor = (m_mdichildlink.m_flag & 1) ? &m_mdichildlink.m_color1 : NULL;
            break;
    }

    //if no match
    if(!pcolor)
    {
        return wyFalse;
    }

    //set the color
    if(plinkcolor)
    {
        *plinkcolor = *pcolor;
    }

    return wyTrue;
}

//function to get the dual color
wyBool 
wyTheme::GetThemeDualColor(wyInt32 index, DUALCOLOR* pdualcolor)
{
    DUALCOLOR dc = {0};

    //match the index
    switch(index)
    {
        case DUAL_COLOR_MDICHILDTEXT:
            dc = m_colormdichildtext;
            break;
        case DUAL_COLOR_MENU:
            dc = m_colormenuhighlight[MENUINDEX];
            break;

        case DUAL_COLOR_MENUBAR:
            dc = m_colormenuhighlight[MENUBARINDEX];
            break;

        case DUAL_COLOR_MENUSELECTED:
            dc = m_colormenuselected;
            break;

        case DUAL_COLOR_MENUBARSELECTED:
            dc = m_colormenubarselected;
            break;

        case DUAL_COLOR_TOOLBAR:
            dc = m_colortoolbarhighlight;
            break;

        case DUAL_COLOR_MAINTOOLBAR:
            dc = m_colormaintoolbarhighlight;
            break;

        case DUAL_COLOR_MENUTEXT:
            dc = m_colormenutext[MENUINDEX];
            break;

        case DUAL_COLOR_MENUBARTEXT:
            dc = m_colormenutext[MENUBARINDEX];
            break;

        case DUAL_COLOR_SELMENUTEXT:
            dc = m_colorselmenutext[MENUINDEX];
            break;

        case DUAL_COLOR_SELMENUBARTEXT:
            dc = m_colorselmenutext[MENUBARINDEX];
            break;

        case DUAL_COLOR_FRAMEWINDOWTEXT:
            dc = m_colorframewindowtext;
            break;

        case DUAL_COLOR_TOOLBARTEXT:
            dc = m_colortoolbartext;
            break;

        case DUAL_COLOR_MAINTOOLBARTEXT:
            dc = m_colormaintoolbartext;
            break;

        case DUAL_COLOR_CONNTABTEXT:
            dc = m_colorconntabtext;
            break;

        case DUAL_COLOR_QUERYTABTEXT:
            dc = m_colorquerytabtext;
            break;

        case DUAL_COLOR_RESTABTEXT:
            dc = m_colorrestabtext;
            break;

        case DUAL_COLOR_TABLETABTEXT:
            dc = m_colortabletabtext;
            break;

        case DUAL_COLOR_MAINTOOLSELECTED:
            dc = m_colormaintoolbarselected;
            break;

        case DUAL_COLOR_TOOLBARSELECTED:
            dc = m_colortoolbarselected;
            break;
    }

    //if no match
    if(!dc.m_flag)
    {   
        return wyFalse;
    }

    //set the info
    if(pdualcolor)
    {
        *pdualcolor = dc;
    }

    return wyTrue;
}

//super classed edit proc to handle CTRL+A
LRESULT	CALLBACK	
wyTheme::SuperClassedEditProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    //handle CTRL+A
    if(message == WM_KEYDOWN && (wparam == 'a' || wparam == 'A'))
    {
        if(GetKeyState(VK_CONTROL) & 0x8000)
        {
            SendMessage(hwnd, EM_SETSEL, 0, -1);
            return 1;
        }
    }

    return m_origeditclassproc ? m_origeditclassproc(hwnd, message, wparam, lparam) : 0;
}

//function to apply theme in SQLyog
void
wyTheme::ApplyTheme()
{
    MDIWindow*  pmdi = GetActiveWin();
    HBRUSH      hbr;
    wyString    themedir;

    //hide the app window
    ShowWindow(pGlobals->m_pcmainwin->m_hwndmain, SW_HIDE);

    //free resources and reinitailize
    wyTheme::FreeResources();
    wyTheme::Init();
    
    //set the class brush for MDI window
    if(pmdi)
    {
        SetClassLongPtr(pmdi->m_hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)(wyTheme::GetBrush(BRUSH_MDICHILD, &hbr) ? hbr : GetSysColorBrush(COLOR_BTNFACE)));
        SetClassLongPtr(pmdi->m_pcqueryvsplitter->GetHwnd(), GCLP_HBRBACKGROUND, (LONG_PTR)(wyTheme::GetBrush(BRUSH_VSPLITTER, &hbr) ? hbr : GetSysColorBrush(COLOR_BTNFACE)));
    }

    //set class brush for frame window
    SetClassLongPtr(pGlobals->m_pcmainwin->m_hwndmain, GCLP_HBRBACKGROUND, (LONG_PTR)(wyTheme::GetBrush(BRUSH_FRAMEWINDOW, &hbr) ? hbr : GetSysColorBrush(COLOR_BTNFACE)));

    //enumerate the window for updating colors and resizing
    EnumChildWindows(pGlobals->m_pcmainwin->m_hwndmain, wyTheme::EnumChildProcUpdateTabs, NULL);
    EnumChildWindows(pGlobals->m_pcmainwin->m_hwndmain, wyTheme::EnumChildProcResize, NULL);

    //set the owner draw from menu and redraw the menu bar
    pGlobals->m_pcmainwin->SetYogMenuOwnerDraw();
    DrawMenuBar(pGlobals->m_pcmainwin->m_hwndmain);

    //repaint everything
    InvalidateRect(pGlobals->m_pcmainwin->m_hwndmain, NULL, TRUE);
    UpdateWindow(pGlobals->m_pcmainwin->m_hwndmain);
    InvalidateRect(pGlobals->m_pcmainwin->m_hwndconntab, NULL, TRUE);
    UpdateWindow(pGlobals->m_pcmainwin->m_hwndconntab);

    //show the window
    ShowWindow(pGlobals->m_pcmainwin->m_hwndmain, SW_SHOW);
}

//resize proc
BOOL CALLBACK 
wyTheme::EnumChildProcResize(HWND hwnd, LPARAM lparam)
{
    wyWChar     classname[SIZE_256];
    MDIWindow*	pmdi;

    GetClassName(hwnd, classname, SIZE_256 - 1);

    if(!wcsicmp(classname, QUERY_WINDOW_CLASS_NAME_STR))
    {
        pmdi = (MDIWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        pmdi->Resize();
    }

    return TRUE;
}

//function to update the colors
BOOL CALLBACK 
wyTheme::EnumChildProcUpdateTabs(HWND hwnd, LPARAM lparam)
{
    wyWChar         classname[SIZE_256];
    HWND            hwndparent;
    TABCOLORINFO    ci = {0};

    GetClassName(hwnd, classname, SIZE_256 - 1);

    if(!wcsicmp(classname, L"Custom Tab Control"))
    {
        CustomTab_SetColorInfo(hwnd, NULL);

        if(GetParent(hwnd) == pGlobals->m_pcmainwin->m_hwndmain)
        {
            if(wyTheme::GetTabColors(COLORS_CONNTAB, &ci))
            {
                CustomTab_SetColorInfo(hwnd, &ci);
            }
            else
            {
                ci.m_mask = CTCF_BOTTOMLINE | CTCF_SELTABFG2;
                ci.m_seltabfg2 = GetSysColor(COLOR_BTNFACE);
                CustomTab_SetColorInfo(hwnd, &ci);
            }
        }
        else if(GetParent(GetParent(GetParent(hwnd))) == pGlobals->m_pcmainwin->m_hwndmain)
        {
            if(wyTheme::GetTabColors(COLORS_QUERYTAB, &ci))
            {
                CustomTab_SetColorInfo(hwnd, &ci);
            }
        }
        else
        {
            hwndparent = GetParent(hwnd);

            GetClassName(hwndparent, classname, SIZE_256 - 1);

            if(!wcsicmp(classname, TABLE_TABINTERFACE_WINDOW))
            {
                ci.m_border = wyTrue;
                ci.m_mask = CTCF_BORDER;
                CustomTab_SetColorInfo(hwnd, &ci);

                if(wyTheme::GetTabColors(COLORS_TABLETAB, &ci))
                {
                    CustomTab_SetColorInfo(hwnd, &ci);
                }
            }
            else
            {
                if(wyTheme::GetTabColors(COLORS_RESTAB, &ci))
                {
                    CustomTab_SetColorInfo(hwnd, &ci);
                }
            }
        }
    }

    return TRUE;
}

//helper function to convert hex value into COLORREF
COLORREF
wyTheme::GetColorFromHex(wyInt32 color)
{
    wyInt32 red, blue, green;

    red = (color & 0xFF0000) >> 16;
    green = (color & 0xFF00) >> 8;
    blue = color & 0xFF;

    return RGB(red, green, blue);
}