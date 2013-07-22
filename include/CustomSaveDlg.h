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

#ifndef _CUSTOM_SAVE_DLG_H_
#define _CUSTOM_SAVE_DLG_H_

#include "wyString.h"
#include "mmsystem.h"

class CustomSaveDlg
{
    public:

        ///Parameterized constructor
        /**
        @param hwndparent               : IN handle to the parent window
        @param message                  : IN message to be displayed
        @param caption                  : IN dialog caption
        @param yestoall                 : IN show/hid Yes To All and No To All buttons
        */
        CustomSaveDlg(HWND hwndparent, wyWChar* message, wyWChar* caption, wyBool yestoall = wyFalse);

        ///Destructor
        ~CustomSaveDlg();

        ///The function creates the dialog box
        /**
        @returns the ID of the button user has pressed
        */
        wyInt32                         Create();

        ///Standard dialog box proceduder for Custom Dialog
        /**
        @param hwnd                     : IN handle to the window
        @param message                  : IN message
        @param wparam                   : IN message parameter
        @param lparam                   : IN message parameter
        @returns TRUE if message is handled, else FALSE
        */
        static INT_PTR CALLBACK         DlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        ///Initialize the dialog box
        /**
        @param hwnd                     : IN handle to the window
        @returns void
        */
        void                            Initialize(HWND hwnd);

        ///Position the dialog center to the monitor
        /**
        @returns void
        */
        void                            PositionCenterMonitior();

        ///Center align the buttons
        /**
        @returns void
        */
        void                            AlignButtons();

        ///Resizes the dialog initially showing only required buttons
        /**
        @returns void
        */
        void                            AdjustInitialPosition();

        ///Resize the dialog to best fit its contents
        /**
        @returns void
        */
        void                            ResizeDialog();

        ///Initialize the maximum allowable dialog width
        /**
        @returns void
        */
        void                            InitDlgWidth();

        ///Set the control fonts
        /**
        @returns void
        */
        void                            SetControlFonts();

        ///Wrap the text as required
        /**
        @param hdc                      : IN handle to the device context
        @param rctext                   : IN message rect
        @returns calculated rect of the message
        */
		RECT							WrapText(HDC hdc, RECT rctext);

        ///Helper function for WrapText
        /**
        @param tempbuff                 : IN/OUT temperory buffer
        @param finalstring              : IN/OUT the modified string
        @param hdc                      : IN     handle to the device context
        @param textwidth                : IN     maximum allowable message width
        */
        void                            WrapTextHelper(wyString& tempbuff, wyString& finalstring, HDC hdc, wyInt32 textwidth);

        /////Aligns the pointer to the boundary given
        /**
        @param ptr                      : IN pointer to be aligned
        @param align                    : IN the space to be padded up, default is WORD
        @returns the next DWORD
        */
        LPWORD                          AlignMemory(LPWORD ptr, ULONG align = 4);

        ///Function creates the dialog template
        /**
        @param hglobal                  : IN handle to the allocated space
        @param fontname                 : IN name of the font
        @param fontsize                 : IN size of the font
        @returns void
        */
        void                            CreateDlgTemplate(HGLOBAL hglobal, const wyChar* fontname, wyInt32 fontsize);

        ///Function adds the dialog controls to the template
        /**
        @param lpword                   : IN/OUT template pointer 
        @returns template pointer
        */
        LPWORD                          AddControls(LPWORD lpword);

        ///Function sets the display style to be used
        /**
        @returns void
        */
        void                            SetOSStyle();

        ///Window handle to the parent
        HWND        m_hwndparent;

        //Dialgo handle
        HWND        m_hwnd;

        ///Message string
        wyString    m_message;

        ///Caption string
        wyString    m_caption;

        ///Flag determines whether to show/hide buttons
        wyBool      m_yestoall;

        ///Dialog rect
        RECT        m_rcwnd;

        ///Maximum allowable dialog width
        wyInt32     m_maxdlgwidth;

        ///Handle to the nearest monitor
        HMONITOR    m_hmonitor;

        ///Space between the frame and buttons
		wyInt32		m_buttontoppadding;

        wyInt32     m_buttonrightpadding;

        ///Handle to the font used
        HFONT       m_hfont;

        //whether to use vista style or not
        static wyBool      m_vistastyle;
};

///API to invoke Custom Save Message
/**
@param hwnd         : IN handle to the parent window
@param message      : IN message to be printed
@param caption      : IN caption for the dialog
@param yestoall     : IN whether to show/hide all buttons
@
*/
wyInt32
CustomSaveMessageBox(HWND hwnd, wyWChar* message, wyWChar* caption, wyBool yestoall = wyFalse);

#endif