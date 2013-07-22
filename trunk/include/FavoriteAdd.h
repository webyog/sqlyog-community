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

#ifndef	_ADDFAV_H
#define _ADDFAV_H

#include "FavoriteBase.h"
#include "wyString.h"
#include "List.h"
#include "DoubleBuffer.h"

#define	NFILE	2
#define	NDIR	1
    

class FavoriteAdd : public FavoriteBase
{

public :
    
    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
	FavoriteAdd();

    /// Destructor
    /**
    Free up all the allocated resources
    */
	~FavoriteAdd();

    /// Display all the items in favorite.
    /** This procedure is used to display all 
    the items in the favorite like folders and files
    @param hwnd			: IN Window handle.
    @returns wyBool wytrue if it is success, otherwise failure
    */
    wyBool Display(HWND  hwnd);
    
private:

    /// Tree handle.
    HWND m_haddtree;

	//Dialog Handle
	HWND m_hwnd;

	/// List to store the dialogbox component values
	List m_controllist;

	/// To store the original size of the dialog box
	RECT m_dlgrect;

	/// To store the co-ordinates of the dialog box wrt the screen
	RECT m_wndrect;

	/// window procedure for addfavorite
    /** 
    */
	static INT_PTR CALLBACK AddFavoDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM  lParam);

    /// Handle the notification message
    /** 
    @param hwnd		: IN Window handle
    @param lparam	: IN Long message parameter
    */
    void OnWMNotify(HWND hwnd, LPARAM lparam);
    /// Handle the WM_COMMAND message
    /** 
    @param hwnd		: IN Window handle
    @param lparam	: IN Long message parameter
    */
    void OnWMCommand(HWND hwnd, WPARAM wParam);
  
    /// Initializes the add favorites diap\log
    /** 
    @param hwnd		: IN window handle
    @returns void
    */
    void InitializeAddFavorite(HWND hwnd);

    /// window procedure for add folder
    /** 
    */
	static INT_PTR CALLBACK AddFolderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	/// Gets the favorite name and content.
    /** It gets the favorite name as well as the content
    @returns wyBool wyTrue if success, otherwise wyFalse.
    */
	wyBool GetFavoriteName(HWND hwnd, wyString & favname, wyString & favquery);

    /// Gets the folder name
    /**
    @param hwnd			: IN Window handle
    @param foldername	: IN Pointer to a pointer to keep foldername
    @returns wyBool wyTrue if success, otherwise wyFalse.
    */
	wyBool GetFolderName(HWND  hwnd , wyString &foldname);

    /// Gets the  and content.
    /**  Gets the actual data top the buffer
    @param hwnd			: IN Window handle
    @param favname		: IN Pointer to pointer contains the favoritename
    @param favquery		: IN Pointer to pointer contains data
    @param iseditor		: IN Is it from file or from editor
    @returns wyBool wyTrue if success, otherwise wyFalse.
    */
	wyBool GetDataToBuffer(HWND hwnd, wyString & favname, wyString & favquery, wyBool iseditor);
	
	/// This will handle when OK button is pressed
    /**  it will start the initialization of Import process
    @param hwnd			: IN Window handle
    @returns wyBool wyTrue if success, otherwise wyFalse.
    */
	wyBool ProcessOK(HWND hwnd);

    /// Creates new folder in the selected path
    /** It will create a folder in the selected folder in favorite
    @param hwnd			: In Window handle
    @returns wyBool wyTrue if success, otherwise wyFalse.
    */
	wyBool CreateNewFolder(HWND hwnd);

    /// Handles the ENTER key press in Editfolder
    /** It will check for any folder with the same name exists or not
    @param hwnd			: IN Window handle
    @returns wyBool wyTrue if success, otherwise wyFalse.
    */
	wyBool NewFolderProcessOK(HWND hwnd);

    /// Inserts a favorites item in the specific folder
    /** It will create a favorite item in specific folder.
    @param favoritename	: IN Favorite item to be inserted.
    @param isfolder		: IN Tells whether it is a folder or not.
    @param favquery		: IN Content of the item.
    @returns wyBool wyTrue if success, otherwise wyFalse.
    */
	wyBool InsertFavorite(wyString &favoritename, wyBool isfolder, wyString &favquery);
	
    /// Function enable/disable the open from file extra options
    /** function enable/disable the open from file extra options
    @param HWND hwnd	: IN Window handle.
    @param enable		: IN Enabled ?.
    @returns wyBool wyTrue if success, otherwise wyFalse.
    */
	wyBool EnableExtraOptionWindows(HWND hwnd, wyBool enable);

    /// Gets the SQL filename
    /** 
    @param HWND hwnd	: IN Window handle.
    @returns wyBool wyTrue if success, otherwise wyFalse.
    */
	wyBool GetSQLFileName(HWND hwnd);

    /// Gets selected image
    /** 
    @param HWND hwnd	: IN Window handle.
    @returns wyInt32 NFILE if file icon, otherwise NDIR.
    */
	wyInt32 GetSelectionImage(HWND hwnd);

    /// Gets selected image text
    /** 
    @param HWND hwnd	: IN Window handle.
    @returns wychar, Image text.
    */
	wyWChar* GetSelectionText(HWND hwnd, wyWChar * text);

			/* Function to handle resize
	@param hwnd         : Window HANDLE
    */
			void AddFavResize(HWND hwnd);

	/* Function to reposition the controls
	*/		
			void PositionCtrls();

	/* Function to add the dialog controls to a list
	*/		
			
			void GetCtrlRects();

	/* Sets the minimum size of the window
	@param lparam       : Long message parameter
	*/	
			void OnWMSizeInfo(LPARAM lparam);

	///Function thats draws the window contents using double buffering
        /**
        @param hwnd             : IN window handle
        @returns void
        */
			void OnPaint(HWND hwnd);

};

#endif