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

/*********************************************

Author: Shubhansh

*********************************************/

#ifndef _CCUSTOM_COMBO_BOX_H_
#define _CCUSTOM_COMBO_BOX_H_

#include "GUIHelper.h"
#include "FrameWindowHelper.h"

/***
Custom Combo Box comprises of ComboBox as its child. 
Custom Combo Control can be Owner drawn as normal ComboBox.
Custom Combo Control has feature of showing selection if any prefix match is found in drop down list. 
It fills Edit Control of Combo Box with remaining characters of first prefix match found.
When no match is found, closes the List Box if visible.
*/

// Custom ComboBox Messages
/*** Custom Combo Message Sent to parent corresponding to WM_PAINT for Combo Box for custom drawing the tool
wparam: handle to device context as sent by WM_PAINT of Combo Box
lparam: Identifier of the parent
*/
#define CCBM_ERASEBKGND         WM_USER + 10

/*** Custom Combo Message Sent to parent corresponding to Combo Box's notification messages like CBN_EDITCHANGE, CBN_SELCHANGE, etc
wparam: LOWORD -> Identifier of parent, HIWORD -> specifies the Notification Code
lparam: Handle to the Combo Box (Note: Not Custom Combo Box)
*/
#define CCBM_NOTIFY             WM_USER + 11

/***Message Sent to CustomCombo Box to Set Margin of the Edit Control in Combo Box to draw images or other things.
wparam: Not Used
lparam: pointer to RECT type with
        structure variable              value-meaning
        right                           right-margin
        left                            left-margin
        top                             top-margin
        bottom                          bottom-margin
*/
#define CCBM_SETMARGIN          WM_USER + 12

/***Message Sent to CustomCombo Box to get handle to the ComboBox
wparam: Not Used
lparam: Not Used
*/
#define CCBM_GETCOMBOHWND       WM_USER + 13

/***Message Sent to CustomCombo Box to not show drop down list. Message is effective for single time use only.
That means, if you do not want to show drop down list again, then you will again have to send the message.
wparam: Not Used
lparam: Not Used
*/
#define CCBM_NOSHOWDROPDOWN     WM_USER + 14

//Custom ComboBox class registered for Powertools 
#define CUSTOMCOMBOBOX      L"CustomComboBox"

//Custom ComboBox class registered for Community and Connection Manager 
#define CUSTOMCOMBOBOX1     L"CustomComboBox1"

// Custom Combo Box Class
class CCustomComboBox
{
private:
    // Holds information about the ComboBox.
    COMBOBOXINFO        m_cbif;

    // Holds Parent Identifier 
    wyInt32             m_id;

    // wyTrue if Text is to be placed in Editor else wyFalse
    wyBool              m_fillText;

    // wyTrue if Back space is pressed else wyFalse
    wyBool              m_isBkSpcPrsd;

    // wyTrue if DropDown is to be shown else wyFalse
    wyBool              m_showDropDown;

    // Holds Handle to parent control 
    HWND                m_hwndParent;

    // Holds handle to Combo Control
    HWND                m_hwndCombo;

    //  Holds handle to Custom Combo Control
    HWND                m_hwnd;

    // Holds Default Edit Control's WNDPROC 
    WNDPROC             m_origEditCtrlProc;

    // Holds Default ComboControl's WNDPROC
    WNDPROC             m_origComboCtrlProc;

    // Holds Edit Control's Dimensions
    RECT                m_editRect;

    // Holds The margin values set by CCBM_SETMARGIN
    RECT                m_marginRect;

public:

    // Constructor
    CCustomComboBox();
    
    // Dextructor
    ~CCustomComboBox();
    
    ///Function creates ComboBox and set the corresponding parameters
    /**
    @param hwnd             : IN window handle
    @param lParam           : IN A pointer to a CREATESTRUCT structure that contains information about the window being created.
                                As passed by WM_CREATE
    @returns wyTrue if successful else wyFalse
    */
    wyBool CreateCtrls(HWND hwnd, LPARAM lParam);


    ///Function handle WM_COMMAND message for Custom ComboControl
    /**
    @param hwnd             : IN Custom Combo Control handle
    @param wParam           : IN message parameter
    @param lParam           : IN message parameter
    */
    void    HandleWmCommand(HWND hwnd, WPARAM wparam, LPARAM lparam);

    /// Function handles the CBN_EDITCHANGE message of the Combo Box
    void    HandleEditChange();

    /// Function Sets the margin for edit control of the Combo Box, handles the CCBM_SETMARGIN message
    /***
    @param lparam           : IN pointer to RECT type
    */
    void    SetMargin(LPARAM lparam);

    /// Procedure to handle Custom Combo Control's Messages 
    /***
    @param hwnd             : IN handle to Custom ComboBox
    @param message          : IN message
    @param wparam           : IN message parameter
    @param lparam           : IN message parameter
    */
    static LRESULT CALLBACK     WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    /// Subclassed procedure to handle Custom Combo Control's Edit Box Messages 
    /***
    @param hwnd             : IN handle to Edit Control
    @param message          : IN message
    @param wparam           : IN message parameter
    @param lparam           : IN message parameter
    */
    static LRESULT CALLBACK     EditCtrlProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    
    /// Subclassed procedure to handle Custom Combo Control's ComboBox Control's Messages 
    /***
    @param hwnd             : IN handle to ComboBox
    @param message          : IN message
    @param wparam           : IN message parameter
    @param lparam           : IN message parameter
    */
    static LRESULT CALLBACK     ComboCtrlProc(HWND hwnd, UINT message, WPARAM wpwaram, LPARAM lparam);  
};


/// Function registers the Custom Combo Control
/***
@param hInstance        : IN Handle to instance
@return wyTrue if succesfull else wyFalse
*/
wyBool RegisterCustomComboBox(HINSTANCE hInstance);
#endif