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


#ifndef		_FAVMENU_H
#define		_FAVMENU_H

#include "FavoriteBase.h"


class  CFavoriteMenu : public  FavoriteBase
{
public :
    
    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
	CFavoriteMenu();

    /// Destructor
    /**
    Free up all the allocated resources
    */
	~CFavoriteMenu();

    /// Display all the items in favorite menu
    /** This procedure is used to display all 
    the items in the favorite menu
    @param hwnd			: IN Window handle.
    @returns wyBool wytrue if it is SUCCESS, otherwise failure
    */
    wyBool	    Display(HWND);

    /// Creates the favorites folder in user profile folder
    /** 
    @returns wyBool wytrue if it is SUCCESS, otherwise failure
    */
	wyBool	    CreateFavoriteFolder();

	/// Removes the Favourite menu items from menu
	/**
	@param hwnd : IN handle to the window
	@returns void;
	*/
	void		Remove(HWND hwnd);
    
    /// Destroys all the resources allocated to menu
	/**
	@param	hmenu	: IN menu handle
	@param pos		: IN sub menu item position to destroy
	@returns void
	*/
	void		DestroyMenuResources(HMENU hmenu, wyInt32 pos);
    
private:

	/// Gets the Favorite menu handle
	/**
	@param hwnd : IN handle to the window
	@returns HMENU, handle to the favorite menu if success, otherwise NULL 
	*/
	HMENU		GetFavoriteMenu(HWND hwnd);

	/// Adds the favorites to the menu
    /** 
    @param hmenu		: IN handle to the favorites menu
    @returns wyBool wytrue if it is SUCCESS, otherwise failure
    */
	wyBool 		AddFavoriteToMenu(HMENU hmenu);
    
    /// Removes all favorites items
    /** 
    @param hmenu		: IN handle to the favorites menu
	@returns number of favorites item removed
    */
	wyInt32		DeleteFavoriteFromMenu(HMENU hmenu);
};
#endif
