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


#ifndef _QUERYSTATUS_H_
#define _QUERYSTATUS_H_

#define UM_SETSTATUSLINECOL WM_USER + 300

#include "FrameWindowHelper.h"

class StatusBarMgmt
{

public:      

    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
	StatusBarMgmt(HWND hwnd);

    /// Destructor
    /**
    Free up all the allocated resources
    */
	~StatusBarMgmt();


    /// Returns the window handle
    /**
    @returns HWND
    */
	HWND		GetHwnd();

    /// Adds how many ms it took to execute a query
    /**
    @param	tunnel			: IN tunnel pointer		
	@param execsec			: IN execution time second to add 
	@param totsec			: IN total time second to add 
    @returns void
    */
    void		AddTickCount(Tunnel * tunnel, wyInt64 execsec, wyInt64 totsec, wyBool clear = wyFalse);

    /// Adds number of rows in the result
    /**
    @param rows			: IN rows to add
    @returns void
    */
	void		AddNumRows(wyUInt32 rows, wyBool clear = wyFalse);
	
	/// Adds line number and column number 
    /**
    @param hwnd			: IN the edit control where the cursor is
    @param iscintilla	: IN is it scintilla control or not
    @param void
    */
	void		AddLineColNum(HWND hwnd, wyBool iscintilla = wyFalse);

    /// formats and adds shows whether query executed had error or not
    /**
    @param issuccess	: IN query executed successfully or not
    @returns void
    */
	void		AddQueryResult(wyBool issuccess);

    /// shows some additional information if required in the specified part of the status bar
    /**
    @param text			: IN additional info to show
    @returns void
    */
	void		ShowInformation(wyWChar* text, wyInt32 part = 0);

    const wyWChar*    GetPartText(wyInt32 part);

private:

    /// window HANDLE
	HWND		m_hwnd;

    wyString*   m_text;

    wyInt32     m_countparts;

    void        UpdatePart(wyInt32 part);
};

#endif
