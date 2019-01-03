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

#include "MDIWindow.h"
#include "Global.h"
#include "FrameWindowHelper.h"
#include "FrameWindow.h"

#ifndef _QUERYSPLITTER_H_
#define _QUERYSPLITTER_H_


/*! \class FrameWindowSplitter 
    \brief implement the splitters for the connection window
*/
class FrameWindowSplitter
{
public:
    
    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
	FrameWindowSplitter(HWND hwnd, wyUInt32 pleftortoppercent);

    /// Default destructor
    /**
    Free up all the allocated resources
    */
	virtual     ~FrameWindowSplitter();

    /// Initiates the creation of splitter window
    /**
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
    wyBool      Create();
    /// Resizes the splitter window
    /**
    @returns wyBool, wyTrue always
    */
	wyBool      Resize(wyBool isannouncements = wyFalse, wyBool ismanualresize =wyFalse);

    /// Helps to Resize connection window
    /**
    @returns wyBool, wyTrue always
    */
	wyBool      Resizeall();
	
    /// Gets the splitter window handled
    /**
    @returns HWND, handle to the splitter window
    */
	HWND        GetHwnd();
    
	/// Callback function for the splitter window.
	/**
	@param hwnd			: Window Handler
	@param message		: Window message
	@param wparam		: Unsigned message parameter
	@param lparam		: Long message parameter
	*/
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    /// Splitting position for the window
	wyUInt32    m_leftortoppercent;

    /// Used to draw the splitter (remaining portions)
    wyUInt32    m_lastleftpercent;

    /// Used to draw the splitter (remaining portions)
    wyUInt32    m_lasttoppercent;

private:

    /// Creates the splitter window
    /**
    @param hwndparent: IN parent window handle
    @returns HWND, handle to the splitter window if SUCCESS, otherwise NULL.
    */
    HWND        CreateSplitter(HWND hwndparent);

   

    /// Function to drag the window splitter window
    /**
    Window is dragged by trapping the mousemove message
    @returns void
    */
	void        MouseMove(wyBool isinit = wyFalse);

    void        DrawTrackerLine();

    void        EndDrag(wyBool isdrawline);

    /// Handles the paint message of the window. we trap this message so that we can give the window a beveled outlook.
    /**
    @returns LRESULT, always zero
    */
	LRESULT     OnPaint();

    /// Splitter window handle
	HWND        m_hwnd;

    /// Parent window handle
	HWND        m_hwndparent;

    /// rect coordinates of the window
	RECT        m_rect;
  
    /// whether it is dragging or not
    wyBool      m_isdragged;
    
    /// original window procedure
	WNDPROC     m_wporigproc;
    wyInt32     m_width;

    HWND         m_hwndprevfocus;
    wyInt32      m_x;   
    wyInt32      m_prevstyle;
    HDC         m_hdc;
    HPEN        m_hpen;
};

#endif;
