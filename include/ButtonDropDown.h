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

Author: Vishal P.R

*********************************************/

//This is the header file for ButtonDropDown custom control which is used in FK DropDown feature. 
//This is not a complete control and is created only to acheive a specific purpose (a drop down to select the filter columns in FK dropdown). However it can be used anywhere where such need arise
//There are two messages BDM_GETLISTHANDLE and BDM_GETBUTTONHANDLE which gives the control handles, using which you can do whatever you want

#ifndef _BUTTON_DRIOPDOWN_H_
#define _BUTTON_DRIOPDOWN_H_

#include "GUIHelper.h"

//button dropdown window class
#define BUTTONDROPDOWN          L"ButtonDropDown"

//message to get the list view handle of button dropdonw
#define BDM_GETLISTHANDLE       WM_USER + 10

//message to get the button handle of button dropdown
#define BDM_GETBUTTONHANDLE     WM_USER + 11

//message to set the drop style
#define BDM_SETDROPSTYLE        WM_USER + 12

//notification before hit on an item
#define BDN_BEGINHIT            WM_USER + 13

//notification after hit on an item
#define BDN_ENDHIT              WM_USER + 14

//button drop style
#define BDS_UP                 1
#define BDS_DOWN               2
#define BDS_AUTO               0

//structur for button dropdown notifications
typedef struct tagNMBD{
    NMHDR hdr;
    LVHITTESTINFO info;
    UINT message;

    //set this value to non zero to prevent the notification from processing
    wyInt32 retvalue;
}NMBUTTONDROPDOWN, *LPNMBUTTONDROPDOWN;

//the class that encapsulates the handles and functionalities
class ButtonDropDown
{
    public:
        ///Constructor
        /**
        @param hwnd                 : IN handle of the window
        */
        ButtonDropDown(HWND hwnd);

        ///Destructor
        ~ButtonDropDown();

        ///Function creates the controls for the button dropdown control
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                      CreateCtrls();

        ///The function which shows list view window
        /**
        @param checkstate : IN check state of the button
        */
        void                        OnButtonCheck(wyBool checkstate);

        ///Callback procedure for list view window
        /**
        @param hwnd                 : IN window handle
        @param message              : IN window message
        @param wparam               : IN message parameter
        @param lparam               : IN message parameter
        @returns non zero on success else zero
        */
        static LRESULT CALLBACK     ListViewWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        ///Callback procedure for list button dropdown control
        /**
        @param hwnd                 : IN window handle
        @param message              : IN window message
        @param wparam               : IN message parameter
        @param lparam               : IN message parameter
        @returns non zero on success else zero
        */
        static LRESULT CALLBACK     WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        ///Callback procedure for parent window
        /**
        @param hwnd                 : IN window handle
        @param message              : IN window message
        @param wparam               : IN message parameter
        @param lparam               : IN message parameter
        @returns non zero on success else zero
        */
        static LRESULT CALLBACK     ParentWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        ///Function sets the list view width based on the text extent of the item in the list
        /**
        @param prect                : IN pointer to a rect structure representing the minimum width
        @returns width of the list
        */
        wyInt32                     SetListViewWidth(RECT* prect);
        
    private:
        ///Button handle
        HWND    m_hwndbutton;

        ///List view handle
        HWND    m_hwndlistview;

        ///Main control handle
        HWND    m_hwnd;

        ///style
        wyInt32 m_style;

        ///check state of the button
        wyInt32 m_checkstate;

        ///Used as a security flag to prevent some automated actions
        wyBool  m_isautomated;

        ///width of the icon
        wyInt32 m_iconwidth;

        ///List control window procedure
        WNDPROC m_listvieworigproc;

        //original parent proc
        WNDPROC m_origparentproc;

        //original parent data
        LONG_PTR  m_origparentdata;
};

///Function registers the button drop down control
wyBool	RegisterButtonDropDown(HINSTANCE hinstance);

#endif