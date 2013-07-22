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


#include<windows.h>
#include<shlobj.h>
#include "FavoriteMenu.h"
#include "MDIWindow.h"
#include "GUIHelper.h"

extern	PGLOBALS	pGlobals;

CFavoriteMenu::CFavoriteMenu()
{
}

CFavoriteMenu::~CFavoriteMenu()
{
}

HMENU
CFavoriteMenu::GetFavoriteMenu(HWND hwnd)
{
	MDIWindow		*wnd;
	wyInt32			lstyle;  // window style //
	HMENU			hmenu = NULL;
	
    wnd = ::GetActiveWin();

	hmenu  = ::GetSubMenu(GetMenu(hwnd), 2);
    
	if(wnd)
	{
        lstyle = ::GetWindowLongPtr(wnd->GetHwnd(), GWL_STYLE);
 
        /* Here we assumes that FAVORITES manu is on the 4th place(Since it is MAXIMIZED)*/
		if((lstyle & WS_MAXIMIZE) && wyTheme::IsSysmenuEnabled(wnd->m_hwnd))
            hmenu = ::GetSubMenu(GetMenu(hwnd), 3);
	}

	return hmenu;
}

wyBool 
CFavoriteMenu::Display(HWND hwnd)
{	
	MENUINFO		info = {0};
	HMENU			hmenu;

	hmenu = GetFavoriteMenu(hwnd);
	
	info.cbSize		= sizeof(info);
	info.fMask		= MIM_STYLE |MIM_APPLYTOSUBMENUS;
	info.dwStyle	= MNS_NOTIFYBYPOS;
			
    ::SetMenuInfo(GetMenu(hwnd) , &info);
	AddFavoriteToMenu(hmenu);
	
	return wyTrue;
}

wyBool  
CFavoriteMenu::AddFavoriteToMenu(HMENU hmenu)
{
    wyBool ret;

    DeleteFavoriteFromMenu(hmenu);
    ret = CreateFavoriteMenu(hmenu);

    return ret;
}

void 
CFavoriteMenu::Remove(HWND hwnd)
{
	HMENU			hmenu;

	hmenu = GetFavoriteMenu(hwnd);
	DeleteFavoriteFromMenu(hmenu);

	return;
}

wyInt32 
CFavoriteMenu::DeleteFavoriteFromMenu(HMENU hmenu)
{
	wyInt32 count, i;		// number of submenu //
	
    count = ::GetMenuItemCount(hmenu);
	
	//delete submenu from 3rd position ( starting from  0 )
	//since first 3 submenus are constant  ( Add Favorite , Organize Favorite , separator )
	if(count > 4)
	{
		for(i = 4; i < count; i++)
		{
			// we need to free all the resource allocated to all the menu items
            
			DestroyMenuResources(hmenu, 4);

            ::DeleteMenu(hmenu, 4, MF_BYPOSITION);
		}
	
        ::DrawMenuBar(pGlobals->m_pcmainwin->GetHwnd());
	}
	return count - 4; // number of favorites item deleted
}

void 
CFavoriteMenu::DestroyMenuResources(HMENU hmenu, wyInt32 pos)
{
	wyInt32			menuid, menucount, count;
	wyChar			*path = NULL;
	MENUITEMINFO	mif;
	HMENU			hsubmenu;

	menuid = GetMenuItemID(hmenu, pos);

	if(menuid == NULL) // non existing id
        return;
	else if(menuid == -1)  // if the specified item opens a submenu, the return value is -1
	{
		hsubmenu = GetSubMenu(hmenu, pos);
		menucount = GetMenuItemCount(hsubmenu);

		for(count = 0; count < menucount; count++)
			DestroyMenuResources(hsubmenu, count);

		DestroyMenu(GetSubMenu(hmenu, pos)); 

		return;
	}
	
	mif.cbSize		= sizeof(MENUITEMINFO);
	mif.fMask		= MIIM_ID | MIIM_DATA;

	// retreive menuitem information 
	VERIFY(GetMenuItemInfo(hmenu, menuid, FALSE, &mif));

	path = (wyChar*)mif.dwItemData;

	if(path) // free if valid
	{
		free(path);
		path = NULL;
	}
	
	return;
}

// Function to Create Favorites  folder // 
wyBool
CFavoriteMenu::CreateFavoriteFolder()
{
	wyWChar	path[MAX_PATH + 1] = {0};
    wyString fullpath;
		
	// get application data path //
	if(pGlobals->m_configdirpath.GetLength() || SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path)))
	{
		if(pGlobals->m_configdirpath.GetLength())
			fullpath.SetAs(pGlobals->m_configdirpath);

		else
		{
			fullpath.SetAs(path);
			fullpath.Add("\\SQLyog");
		}

		// Create SQLyog folder 
        if( ::CreateDirectory(fullpath.GetAsWideChar(), NULL) == 0)
        {
            if(GetLastError() != ERROR_ALREADY_EXISTS)
                return OnError(_("Error while creating favorites folder"));
        }
        fullpath.Add("\\Favorites");
		// Create Favorites folder
        if( ::CreateDirectory(fullpath.GetAsWideChar(), NULL) == 0)
        {
            if(GetLastError() != ERROR_ALREADY_EXISTS)
                return OnError(_("Error while creating favorites folder"));
        }
        return wyTrue;
	} 
    else
    {
		yog_message(NULL, _(L"Error while creating favorites folder"), pGlobals->m_appname.GetAsWideChar(), MB_OK);
        return wyFalse;
	}
}