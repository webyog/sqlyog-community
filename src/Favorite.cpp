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


#include<shlobj.h>

#include "MDIWindow.h"
#include "EditorBase.h"
#include "FavoriteBase.h"
#include "GUIHelper.h"
#include "commonhelper.h"

extern	PGLOBALS	pGlobals;

FavoriteBase::FavoriteBase()
{	
    m_image.Add(NULL, (wyUInt32)IDI_WARNING);
    m_image.Add(pGlobals->m_hinstance, IDI_OPEN);
    m_image.Add(NULL, (wyUInt32)IDI_INFORMATION);

	//starting ID of favorites
	m_menuid = FAVORITEMENUID_START;
}

FavoriteBase::~FavoriteBase()
{
}

// function to add favorite files  & folders to the Favorite Menu//
wyBool 
FavoriteBase::CreateFavoriteMenu(HMENU hmenu)
{
	wyWChar	    foldpath[MAX_PATH + 1] = {0};	
    wyBool      ret = wyFalse;
    wyString    path, folderpathstr;

	//starting ID of favorites
	m_menuid = FAVORITEMENUID_START;
	
	// to get application data path
	if(pGlobals->m_configdirpath.GetLength() || SUCCEEDED(::SHGetFolderPath (NULL, CSIDL_APPDATA, NULL, 0, foldpath)))
	{
		if(pGlobals->m_configdirpath.GetLength())
		{
			folderpathstr.SetAs(pGlobals->m_configdirpath);
			path.SetAs(folderpathstr.GetString());
			path.Add("\\Favorites\\");
		}

		else
		{
			folderpathstr.SetAs(foldpath);
			path.SetAs(folderpathstr.GetString());
			path.Add("\\SQLyog\\Favorites\\");
		}

		ret = AddMenu(hmenu, path);
	}
    else
        return OnError(_("Error in AddMenu"));
	
	return ret;
}

// Function to add  files & folders to  Favorite menu
wyBool 
FavoriteBase::AddMenu(HMENU hmenu, wyString &parentpath)
{
	wyInt32				res;
	wyWChar				filename[MAX_PATH + 1] = {0};
	HANDLE				hfile;
	wyString			parent;
	WIN32_FIND_DATA		wfd={0};

    parent.SetAs(parentpath);
	parent.Add("*");
    hfile = ::FindFirstFile(parent.GetAsWideChar(), &wfd); 		

	if(hfile == INVALID_HANDLE_VALUE) 
		return wyFalse;
	
	do
	{	
		if ( wcscmp(L".", wfd.cFileName) == 0 || wcscmp(L"..", wfd.cFileName) == 0)
            res = ::FindNextFile(hfile, &wfd);
		else
		{	
			//wcscpy(filename, wfd.cFileName);
			wcsncpy(filename, wfd.cFileName, MAX_PATH);
			filename[MAX_PATH] = '\0';
			
			// check for favorite folder 
			if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
				AddDirectory(hmenu, parentpath, filename);
			else
				AddFile(hmenu, parentpath, filename);
						
            res = ::FindNextFile(hfile, &wfd);
		}
	
	}while(res);
	
    ::FindClose( hfile );			
	return wyTrue;
}

//Function to handle folder type Favorite
wyBool  
FavoriteBase::AddDirectory(HMENU hmenu, wyString & parentpath, wyWChar * filename)
{
	HMENU			hsubmenu;
	wyString		path;
    wyInt32         menucount = 0;
	wyString		filenamestr(filename);

    hsubmenu = ::CreatePopupMenu();
	path.SetAs(parentpath);
	path.AddSprintf("%s\\", filenamestr.GetString());
				
    VERIFY(::InsertMenu(hmenu, -1, MF_POPUP | MF_BYPOSITION | MF_STRING | MF_ENABLED, (LONG)hsubmenu, filename));
	
	AddMenu(hsubmenu, path);

	// if the  folder is empty then add <empty> as its submenu
    menucount = ::GetMenuItemCount(hsubmenu);

    if(menucount == 0)
        VERIFY(::InsertMenu(hsubmenu , -1 , MF_BYPOSITION | MF_STRING | MF_GRAYED ,IDM_EMPTY , L"<empty>"));
		
	return wyTrue;
}

// function to process file type Favorite//
wyBool
FavoriteBase::AddFile(HMENU hmenu, wyString &parentpath,  wyWChar *filename)
{
	wyInt32				i , j=0;
	wyUInt32			lengthwchar = 1;
	wyWChar				ext[_MAX_EXT] = {0} , *data = {0};
	wyChar				*path = {0};
	MENUITEMINFO		lpmii={0};
	
	parentpath.GetAsWideChar(&lengthwchar);
	
	path = AllocateBuff(parentpath.GetLength() + 2);
	data = AllocateBuffWChar(wcslen(filename) + 2);

	wcscpy(data, (wyWChar*)filename);
	strcpy(path, (wyChar*)parentpath.GetString());

	for(i = wcslen(data) - 1; i && data[i]!='.'; i--, j++)
		ext[j] = data[i];
	
	if(wcsnicmp(ext, L"lqs", 3) != 0)
		return wyFalse;

	ext[j] = 0;
	data[i] = 0;
	
	lpmii.cbSize		= sizeof(MENUITEMINFO);
	lpmii.fMask			= MIIM_STRING|MIIM_ID|MIIM_DATA;
	lpmii.wID			= m_menuid++;
	lpmii.dwItemData	= (ULONG_PTR)path;
	lpmii.cch			= wcslen(data);
	lpmii.dwTypeData	= data;
	
    VERIFY(::InsertMenuItem(hmenu, -1, TRUE, &lpmii));
	free(data);

    return wyTrue;
}

// Function to add files & Folders to TreeView Control //
wyBool 
FavoriteBase::InitTreeView(HWND  handle, wyBool isfile)
{	
	HTREEITEM   root = 0;			// Root node handle //

    if(TreeView_DeleteAllItems(handle) == wyFalse)
        return wyFalse;     
	
    root = AddItem(handle, TVI_ROOT, "Favorites", 1);
	AddToTreeView(handle, root, isfile);

    if(TreeView_Select(handle, TreeView_GetRoot(handle), TVGN_CARET) == wyFalse )
        return wyFalse;

	::SendMessage(handle, TVM_EXPAND, (WPARAM)TVE_EXPAND, (LPARAM )root);
	
	return wyTrue;
}

wyBool 
FavoriteBase::AddToTreeView(HWND handle ,HTREEITEM  root, wyBool isfile)
{
	wyInt32				res;
	wyWChar				filename[MAX_PATH + 1]= {0};
	wyString			parentpath , path;
	HANDLE				hfile;
	HTREEITEM			child;
	WIN32_FIND_DATA		wfdd;

	if(SelItemPath(handle, root, parentpath) == wyFalse)
         return wyFalse;

	path.AddSprintf("%s\\*", parentpath.GetString());
    hfile = FindFirstFile(path.GetAsWideChar(), &wfdd); 		

	if(hfile == INVALID_HANDLE_VALUE) 
		return wyFalse;
	
    do
	{	 
		if (wcscmp(L".", wfdd.cFileName) == 0 || wcscmp(L"..", wfdd.cFileName) == 0)
            res = ::FindNextFile(hfile , &wfdd);
	    else
		{	
			//wcscpy(filename , wfdd.cFileName);
			wcsncpy(filename, wfdd.cFileName, MAX_PATH);
			filename[MAX_PATH] = '\0';
			
			if(wfdd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
			{								
				child = AddItem(handle, root, filename, NDIR);
				AddToTreeView(handle, child, isfile);
               	//::SendMessage(handle, TVM_EXPAND, (WPARAM)TVE_EXPAND, (LPARAM)child);
			}
			else if(isfile)
				ProcessFile(handle, root, filename);
									
            res = ::FindNextFile(hfile , &wfdd);
		}
	}while(res);

	FindClose(hfile);			
	return wyTrue;
}

// Function to add  file favorite to treeview control//
wyBool
FavoriteBase::ProcessFile(HWND handle , HTREEITEM root , wyWChar * favoritename)
{
	INT			j = 0 , i ;
	wyWChar		ext[_MAX_EXT]={0} ;
    		
	// copy the file extension
	for(i = wcslen(favoritename) - 1; i && favoritename[i] != '.'; i--, j++)
		ext[j] = favoritename[i];
				
	// check for SQL file type , if not no need for adding the file//
	if(wcsnicmp(ext, L"lqs", 3) != 0)
		return wyFalse;
	
	ext[j] = 0;
    favoritename[i] = 0;
    AddItem(handle, root, favoritename, 2);
	return wyTrue;
}


// Function to add the given item to the treeview control //
HTREEITEM	
FavoriteBase::AddItem(HWND htree, HTREEITEM parent, wyString name, wyInt32 pos) 
{	
	TV_INSERTSTRUCT			str ;
	str.hParent				= parent ;
	str.item.mask			= TVIF_TEXT |  TVIF_IMAGE | TVIF_SELECTEDIMAGE ; 
	str.hInsertAfter		= TVI_SORT;
	str.item.pszText		= (LPWSTR)name.GetAsWideChar();
	str.item.iImage			= pos ;
	str.item.iSelectedImage = pos;

    HTREEITEM htreeitem;
    VERIFY(htreeitem = (HTREEITEM)SendMessage(htree, TVM_INSERTITEM, NULL, (wyInt64)&str));
	return(htreeitem);
 }

// Function to get the path the selected item in a  Treeview //
wyBool 
FavoriteBase::SelItemPath(HWND htree, HTREEITEM  item, wyString &fullpath, wyBool bFileFlag)
{	
	wyWChar		data[MAX_PATH + 1] = {0};
	wyWChar		foldpath[MAX_PATH + 1] = {0};
	TVITEM		tvitem;
	wyString	path, temp, temp2, datastr, foldpathstr;
	
	// application data path //
	if(!pGlobals->m_configdirpath.GetLength())
	{
		if(!SUCCEEDED(::SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, foldpath)))
			return OnError(_("Error in favorites"));

		wcscat(foldpath ,L"\\SQLyog");
	}

	else
	{
		//wcscpy(foldpath, pGlobals->m_configdirpath.GetAsWideChar());
		wcsncpy(foldpath, pGlobals->m_configdirpath.GetAsWideChar(), MAX_PATH);
		foldpath[MAX_PATH] = '\0';
	}
	
	while(item)
	{
		tvitem.mask		  = TVIF_TEXT | TVIF_IMAGE;
		tvitem.hItem	  = item;
		tvitem.cchTextMax = MAX_PATH;
		tvitem.pszText	  = data ;

		TreeView_GetItem(htree, &tvitem);
		datastr.SetAs(data);
		if(tvitem.iImage != NFILE || !bFileFlag)
        {
			temp.Sprintf("\\%s", datastr.GetString());
            if(path.GetLength() > 0)
            {
                temp2.SetAs(path);
				path.Sprintf("\\%s%s", datastr.GetString(), temp2.GetString());
            }
            else
				path.Sprintf("\\%s", datastr.GetString());
        }
		
		item = TreeView_GetParent(htree, item);
	}
	
	foldpathstr.SetAs(foldpath);
	fullpath.SetAs(foldpathstr.GetString());
	fullpath.Add(path.GetString());

    return wyTrue;
}

// Function to create a favorite file or folder in the given path //
wyBool 
FavoriteBase::InsertFavoriteItem(wyString &path, wyBool isfolder , wyString &favquery)
{	
	DWORD				byteswritten;
	HANDLE				hfile;
	//const unsigned char utf8bom[10] = {unsigned char(0xEF), unsigned char(0xBB), unsigned char(0xBF)};
			
	// if the favorite is file type then add the extension//
	if(!isfolder)
	{
		path.Add(".sql");

        hfile = ::CreateFile(path.GetAsWideChar(), GENERIC_WRITE, 0, NULL, CREATE_NEW,
								   NULL, NULL);

		if(hfile  == INVALID_HANDLE_VALUE)
		{
            if(::GetLastError() == ERROR_FILE_EXISTS)
			{
                if(::MessageBox(NULL, _(L"The name you have entered for the shortcut already exists in Favorites menu.\nWould you like to overwrite it?"), _(L"Add Favorite"), MB_YESNO | MB_TASKMODAL | MB_ICONINFORMATION | MB_DEFBUTTON2) == IDYES)
				{
                    hfile = ::CreateFile(path.GetAsWideChar(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, NULL, NULL);
					
					if(hfile  == INVALID_HANDLE_VALUE)
                         OnError(_("Cannot write into Favorite File"));
				}
				else
					return wyFalse;			
			}
			else
			    return OnError(_("Cannot write into Favorite File"));
		}
		
		//if (!::WriteFile(hfile,  utf8bom, 3, &byteswritten , NULL))
		//	return OnError("Cannot write into Favorite File");

        if (!::WriteFile(hfile,  favquery.GetString(), favquery.GetLength(), &byteswritten , NULL))
			return OnError(_("Cannot write into Favorite File"));

        ::CloseHandle(hfile);
	}
    else if(!::CreateDirectory(path.GetAsWideChar(), NULL))
	  return OnError(_("Cannot create Favorite Folder"));
	
    return wyTrue;
}

wyBool 
FavoriteBase::OnError(wyString errmsg)
{
    DisplayErrorText(::GetLastError(), errmsg.GetString());
    return wyFalse;
}

