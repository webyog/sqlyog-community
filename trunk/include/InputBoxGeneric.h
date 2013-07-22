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


#ifndef	_TEXTENTRYDIALOGS_H
#define _TEXTENTRYDIALOGS_H

#include <windows.h>
#include <commctrl.h>
#include "ExportMultiFormat.h"
#include "wyString.h"

class InputBoxGeneric
{
public:

    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
    InputBoxGeneric();

    /// Destructor
    /**
    Free up all the allocated resources if there are
       */
	~InputBoxGeneric();

    /// Show the dialog
    /**
    Displays the dialog to get the input
    @param hwndparent		: IN Parent of the dialog box
    @param title			: IN Message to show on the dialog.
    @param caption			: IN Caption to be shown on the static control
    @param error			: IN Error message to be shown to user if the user presses OK with an empty string
    @param strvalue			: IN/OUT The default value, which may be the empty string. On return, it will contain the new data entered by user.
    @returns wyTrue if user pressed OK, wyFalse if user pressed Cancel
    */
    wyBool 
    Create(HWND hwndparent, const wyChar * title, const wyChar * caption, const wyChar * errormsg, 
           wyString & strvalue);

private:
    
	/// Local copy of the title
    wyString    m_title;   

	/// Local copy of the caption
    wyString    m_caption;  

	/// Local copy of the message
    wyString    m_errmsg; 

	/// Local copy of the value entered
    wyString    m_strvalue;   

	/// Local copy of the handle of the dialog box
    HWND        m_hwnd;             

    /// Callback dialog procedure
    /**
    Callback function for the dialog procedure
    @param  hwnd		: IN Handle to the dialog window
    @param  message		: IN Windows message
    @param  wparam		: IN wParam
    @param  lparam		: IN lParam
    @returns Value that needs to be sent to the system
    */
    static INT_PTR CALLBACK DlgWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM  lparam);

    /// Process WM_COMMAND for the dialog
    /**
    Process WM_COMMAND for the dialog
    @param  wparam		: IN wParam coming from the system
    @param  lparam		: IN lParam coming from the system
    @return wyTrue if user pressed OK and it was success, otherwise wyFalse
    */
    wyBool  HandleWMCommand(WPARAM wparam, LPARAM lparam); 


    /// Initialize dialog with its values
    /**
    Sets the title and default values of controls.
    */
    void    InitializeDlgValues();
};

#endif
