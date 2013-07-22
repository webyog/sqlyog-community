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

#ifndef _TABEDITORSPLITTER_H_
#define _TABEDITORSPLITTER_H_

#include "MDIWindow.h"
#include "Global.h"

class TabEditorSplitter
{
public:
	/// Constructor
	/*
	@param hwndparent  : IN parent window handle
	@param ptoppercent : IN splitter position
	*/
	TabEditorSplitter(HWND hwndparent, wyUInt32 ptoppercent);

	/// Destructor
	~TabEditorSplitter();

	/// Initiates the creation of splitter window
    /**
    @returns wyBool, wyTrue is SUCCESS, otherwise wyFalse
    */
	wyBool				Create(wyBool isdlg = wyFalse);

	/// Resizes the splitter window
    /**
	@returns wyBool, wyTrue always
    */
	wyBool				Resize(RECT rcthwnd);

	/// Helps to Resize connection window
    /**
    @returns wyBool, wyTrue always
    */
	wyBool				Resizeall();

	/// Callback function for the splitter window.
	/**
	@param hwnd			: Window Handler
	@param message		: Window message
	@param wparam		: Unsigned message parameter
	@param lparam		: Long message parameter
	*/
	static	LRESULT		CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

	/// Creates the splitter window
    /**
    @param hwndparent: IN parent window handle
    @returns HWND, handle to the splitter window if SUCCESS, otherwise NULL.
    */
    HWND				CreateHSplitter();

	/// Function to drag the window splitter window
    /**
    Window is dragged by trapping the mousemove message
    @returns void
    */
	VOID				MouseMove(wyBool isinit = wyFalse);

    /// Handles the paint message of the window. we trap this message so that we can give the window a beveled outlook.
    /**
    @returns LRESULT, always zero
    */
	LRESULT				OnPaint();

	/// Sets the TabTypes pointer
	/*
	@param ptabtypes : pointer to TabTypes  
	*/
	void				SetTabEditorPtr(TabEditor *ptabeditor);

	/// sets the default minimum percentage
    /*
    @ returns top percent value
    */
	wyInt32             GetLeftTopPercent();

    ///Sets the top percentage value
    /*
    @param perc : ner toppercent value
    @returns void
    */
    void                SetLeftTopPercent(wyInt32 perc);

    void                DrawTrackerLine();

    void                EndDrag(wyBool isdrawline);


    /// Splitter window handle
	HWND				m_hwnd;

    /// Parent window handle
	HWND				m_hwndparent;

	/// Horizontal spliter rectangle
	RECT				m_rect;

	/// Splitting position for the window
	wyUInt32			m_leftortoppercent;
	
	/// Used to draw the splitter (remaining portions)
    wyUInt32			m_lastleftpercent;
    
	/// Used to draw the splitter (remaining portions)
    wyUInt32			m_lasttoppercent;
	
	/// whether it is dragging or not
    wyBool				m_isdragged;

	/// TabEditor pointer
	TabEditor			*m_pctabeditor;
    wyBool              m_ischanged;

	//Flag tells splitter is tab's fliter or other's(dialog;s)
	wyBool				m_isdlgsplitter;

    wyInt32             m_height;

    HWND                m_hwndprevfocus;

    wyInt32             m_y;
    wyInt32             m_prevstyle;
    HDC                 m_hdc;
    HPEN                m_hpen;
};
#endif
	


