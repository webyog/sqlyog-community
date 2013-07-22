/* Copyright (C) 2013 Webyog Inc.

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


#ifndef	_FAVORITE_H
#define	_FAVORITE_H

#include <windows.h>
#include <commctrl.h>

#include "wyString.h"
#include "ImageList.h"

#define	IDM_EMPTY		901



class FavoriteBase
{
public :

    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
	FavoriteBase();

    /// Destructor
    /**
    Free up all the allocated resources
    */
	virtual ~FavoriteBase();

    /// Pure virtual function to display the content
    /**
    @param wnd			: IN window handle
    @returns wyBool wytrue if it is success, otherwise failure
    */
	virtual	wyBool	Display(HWND wnd) = 0;

    /// Creates the favorites menu
    /** This procedure is used to display all 
    the items in the favorite like folders and files
    @param hmanu		: IN Menu handle handle.
    @returns wyBool wytrue if it is success, otherwise failure
    */
	wyBool CreateFavoriteMenu(HMENU  hmenu);

protected:

    /// menuid, starts from FAVORITEMENUID_START
	wyUInt32	m_menuid;
    ImageList   m_image;
    
    /// Recursively adds the items to the menu
    /** 
    @param hmenu		: IN menu handle.
    @param parentpath	: IN Parent item path.
    @returns wyBool wytrue if it is success, otherwise failure
    */
	wyBool AddMenu(HMENU hmenu, wyString &parentpath);

    /// Adds a folder in to the favorite
    /** 
    @param hmenu		: IN menu handle.
    @param parentpath	: IN Parent item path.
    @param filename		: IN item name
    @returns wyBool wytrue if it is success, otherwise failure
    */
    wyBool AddDirectory(HMENU  hmenu, wyString &parentpath , wyWChar *filename );

    /// Adds a file in to the favorite
    /** 
    @param hmenu		: IN menu handle.
    @param parentpath	: IN Parent item path.
    @param filename		: IN item name
    @returns wyBool wytrue if it is success, otherwise failure
    */
    wyBool AddFile(HMENU hmenu, wyString &parentpath ,  wyWChar *filename );

    /// Initializes the favorites tree.
    /** 
    @param htree		: IN tree handle.
    @param isfile		: IN Is it a file or folder.
    @returns wyBool wytrue if it is success, otherwise failure
    */
    wyBool InitTreeView(HWND htree, wyBool isfile);

    /// Adds items to tree
    /** 
    @param htree		: IN tree handle.
    @param root			: IN root HTREEITEM
    @param isfile		: IN Is it a file or folder.
    @returns wyBool wytrue if it is success, otherwise failure
    */
	wyBool AddToTreeView(HWND htree, HTREEITEM root, wyBool isfile);

    /// Processes the file if the "Add a link to tree view ..." is selected
    /**
    @param htree		: IN tree handle.
    @param root			: IN root HTREEITEM
    @param favname: IN Favorite name
    */
	wyBool ProcessFile(HWND htree, HTREEITEM  root , wyWChar * favname );	
    
    /// Adds node in tree
    /**
    @param htree		: IN tree handle.
    @param item			: IN HTREEITEM
    @param data			: IN content of favorite item
    @param image		: IN file image or dir image
    @returns HTREEITEM, the item added
    */
    HTREEITEM AddItem( HWND htree, HTREEITEM item, wyString data, wyInt32 image ) ;

    /// Creates the path to the given HTREEITEM
    /**
    @param htree		: IN tree handle.
    @param item			: IN HTREEITEM
    @param path			: OUT path to this item
    @param bfileflag	: IN file/dir
    @returns wyBool wytrue if it is success, otherwise failure
    */
    wyBool SelItemPath(HWND htree, HTREEITEM  item, wyString &fullpath, wyBool bfileflag = wyFalse );

    /// Creates the path to the given HTREEITEM
    /**
    @param path			: IN item path.
    @param isfolder		: IN file/folder
    @param favquery		: IN favorite data
    @returns wyBool wytrue if it is success, otherwise failure
    */
	wyBool InsertFavoriteItem(wyString &path , wyBool isfolder , wyString &favquery );
	
    /// Display the error message
    /**
    @param errmsg		: IN message to display
    @returns wyBool wytrue if it is success, otherwise failure
    */
    wyBool OnError(wyString errmsg);
};

#endif

