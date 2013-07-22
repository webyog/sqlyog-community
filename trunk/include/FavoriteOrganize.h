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


#ifndef _ORGFAVORITE_H
#define	_ORGFAVORITE_H

#include "FavoriteBase.h"
#include "List.h"
#include "DoubleBuffer.h"


class  COrganizeFavorite : public  FavoriteBase
{
public :
	
    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
	COrganizeFavorite();

    /// Destructor
    /**
    Free up all the allocated resources
    */
	~COrganizeFavorite();

    /// Shows the organize window
    /**
    @param htree:            IN      Window handle
    @returns wyBool wyTrue if SUCCESS, otherwise wyFalse.
    */
	wyBool	Display(HWND hwnd);	
    
private:

    /// Show the dialog
    /**
    Displays the dialog to get the input

    @param hwndparent:     IN       Parent of the dialog box
    @param title:          IN       Message to show on the dialog.
    @param caption:        IN       Caption to be shown on the static control
    @param error:          IN       Error message to be shown to user if the user presses OK with an empty string
    @param strvalue:       IN/OUT   The default value, which may be the empty string. On return, it will contain the new data entered by user.
    @returns wyTrue if user pressed OK, wyFalse if user pressed Cancel
    */
    static	INT_PTR	CALLBACK OrganizeFavoriteDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

     /// Show the move to dialog
    /**
    Displays the dialog to get the input

    @param hwndparent:     IN       Parent of the dialog box
    @param title:          IN       Message to show on the dialog.
    @param caption:        IN       Caption to be shown on the static control
    @param error:          IN       Error message to be shown to user if the user presses OK with an empty string
    @param strvalue:       IN/OUT   The default value, which may be the empty string. On return, it will contain the new data entered by user.
    @returns wyTrue if user pressed OK, wyFalse if user pressed Cancel
    */
	static	INT_PTR	CALLBACK MovetoFolderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

     /// Show the treeview with all nodes
    /**
    handles all the messages from the treeview control

    @param hwndparent:     IN       Parent of the dialog box
    @param title:          IN       Message to show on the dialog.
    @param caption:        IN       Caption to be shown on the static control
    @param error:          IN       Error message to be shown to user if the user presses OK with an empty string
    @param strvalue:       IN/OUT   The default value, which may be the empty string. On return, it will contain the new data entered by user.
    @returns wyTrue if user pressed OK, wyFalse if user pressed Cancel
    */
	static	LRESULT CALLBACK TreeWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    /// Initializes the main dlg procedure
    /**
    @param hwndparent:     IN       Window handle
    @returns void
    */
    void    OnWMInitdlgValues(HWND hwnd);

    /// wm_COMMAND on main organize window is handled here
    /**
    @param hwnd:            IN      Window handle
    @param wParam:          IN      WPARAM of the window procedure    
    @returns void
    */
    void    OnWMCommand(HWND hwnd, WPARAM wParam);

    /// wm_NOTIFY on main organize window is handled here
    /**
    @param hwnd:            IN      Window handle
    @param lParam:          IN      LPARAM of the window procedure    
    @returns void
    */
    void    OnWMNotify(HWND hwnd, LPARAM lParam);

    /// Enabling or disabling the organize window buttons
    /**
    @param hwnd:            IN      Window handle
    @param isenable:        IN      wyTrue/wyFalse  
    @returns void
    */
    void    EnableButtons(HWND hwnd, wyBool isenable);

    /// handles when the node selected is file object.
    /**
    @param hwnd:            IN      Window handle
    @returns void
    */
    void    HandleFileSelection(HWND hwnd);
	
    
    /// Deletes an item from the favorites
    /**
    @param htree:            IN      Tree window handle
    @param hitem:            IN      HTREEITEM to delete
    @returns void
    */
	wyInt32	DeleteFavoriteItem(HWND htree, HTREEITEM hitem);

    /// Removes a particular item from the tree
    /**
    @param htree:            IN      Tree Window handle
    @returns wyBool wyTrue if SUCCESS, otherwise wyFalse.
    */
	wyBool	DeleteFavoriteFromTree(HWND htree);

    /// Shows the moveto dialog window
    /**
    @param htree:            IN      Window handle
    @returns wyBool wyTrue if SUCCESS, otherwise wyFalse.
    */
    wyBool	MoveToFolder(HWND htree);

    /// Creates a new folder in the tree
    /**
    @param htree:            IN      Tree Window handle
    @returns wyBool wyTrue if SUCCESS, otherwise wyFalse.
    */
	wyBool	CreateNewFolder(HWND  htree);

    /// Handles Editing the node is completed
    /**
    @param hwnd:            IN      Window handle
    @param tvdisp:          IN      LPNMTVDISPINFO of the node edited

    @returns wyBool wyTrue if SUCCESS, otherwise wyFalse.
    */
	wyBool	EndLabelEdit(HWND hwnd, LPNMTVDISPINFO tvdisp);

    /// Initiates the label editing of node
    /**
    @param hwnd:            IN      Window handle
    @param tvdisp:          IN      LPNMTVDISPINFO of the node edited

    @returns wyBool wyTrue if SUCCESS, otherwise wyFalse.
    */
	wyBool	RenameFavorite(HWND htree);

    /// Confirms the end label edit
    /**
    @param hwnd:            IN      Window handle
    @returns wyBool wyTrue if SUCCESS, otherwise wyFalse.
    */
	wyBool	ProcessOK(HWND hwnd);

    /// Initializes the tree view control while showing the organize dialog
    /**
    @param hwnd:            IN      Window handle
    @returns wyBool wyTrue if SUCCESS, otherwise wyFalse.
    */
	wyBool	ProcessInitDlg(HWND hwnd);

    /// processes the delete item
    /**
    @param htree:            IN      Tree Window handle
    @param hitem:            IN      The item to be removed
    @returns wyBool wyTrue if SUCCESS, otherwise wyFalse.
    */
	wyBool	ProcessDelete(HWND htree, HTREEITEM  hitem);

    /// Gets the selected image ID
    /**
    @param hwnd:            IN      Window handle
    @returns wyBool wyTrue if SUCCESS, otherwise wyFalse.
    */
	wyInt32	GetSelectionImage(HWND hwnd);

    /// Gets the image ID of a particular item
    /**
    @param hwnd:            IN      Window handle
    @param hItem:           IN      HTREEITEM item
    @returns wyInt32, of the item selected.
    */
	wyInt32	GetItemImage(HWND hwnd, HTREEITEM hItem);

    /// Reads the file into the editor
    /**
    @param hwnd:            IN      Window handle
    @param completepath:    IN      Complete path of the file
    @returns wyInt32, of the item selected.
    */
	wyBool	ReadFromFile(HWND hwnd, wyChar *completepath);

    /// Creates a unique folder name
    /**
    @param htree:            IN     Tree Window handle
    @param node:             IN     Folder node               
    @param foldername:       IN/OUT Folder name
    @returns wyInt32, of the item selected.
    */
    void    CreateUniqueFolderName(HWND  htree, HTREEITEM node, wyString &foldername);

				/* Function to handle resize
	@param hwnd         : Window HANDLE
    */
			void OrgFavResize(HWND hwnd);

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


    /// Before editing the item in treeview store the text
	wyWChar		m_olddata[MAX_PATH + 1];	

    /// Destination path for moving the folder
	wyString	m_destpath;			

    /// subclassed procedure for the tree control to handle VK_F2 and VK_RETURN
	WNDPROC		m_wndproc;			

	//Dialog Handle
	HWND m_hwnd;

	/// List to store the dialogbox component values
	List m_controllist;

	/// To store the original size of the dialog box
	RECT m_dlgrect;

	/// To store the co-ordinates of the dialog box wrt the screen
	RECT m_wndrect;
};

#endif
