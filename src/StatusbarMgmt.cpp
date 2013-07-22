/* Copyright (C) 2013 Webyog Inc

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

#include "StatusbarMgmt.h"

extern PGLOBALS pGlobals;

// Program to implement status window 
StatusBarMgmt::StatusBarMgmt(HWND hwnd)
{
    wyInt32 i, temp;

	m_hwnd = hwnd;
    m_countparts = SendMessage(hwnd, SB_GETPARTS, 0, (LPARAM)&temp);
    m_text = new wyString[m_countparts];
    
    for(i = 0; i < m_countparts; ++i)
    {
        SendMessage(m_hwnd, SB_SETTEXT, i | SBT_OWNERDRAW | SBT_NOBORDERS, 0);
    }
}

StatusBarMgmt::~StatusBarMgmt()
{
    delete[] m_text;
}

HWND
StatusBarMgmt::GetHwnd()
{
	return m_hwnd;
}

const wyWChar*
StatusBarMgmt::GetPartText(wyInt32 part)
{
    if(part >= m_countparts || part < 0)
    {
        return NULL;
    }

    return m_text[part].GetAsWideChar();
}

// This function formats and add time taken to execute the query in the second part of the status
// bar. The time taken in ms is passed as parameter. This function is executed whenever a query is executed
// it shows the time taken in executing that perticular query.
// The time taken is with respect to the client software. This includes the time taken in transferring
// the data from server including network data movement.
void
StatusBarMgmt::AddTickCount(Tunnel * tunnel, wyInt64 execsec, wyInt64 totsec, wyBool clear)
{
	wyString timestr, msg;
	
	if(clear == wyTrue || tunnel->IsTunnel())// If connection is HTTP then we are not printing execution time
	{
		msg.SetAs("");
	}
	else
	{	GetTime(execsec, timestr);
		msg.Sprintf(_(" Exec: %s"), timestr.GetString());
	}
	
    m_text[1].SetAs(msg);
    UpdatePart(1);

    if(clear == wyTrue)
    {
        msg.SetAs("");
    }
    else
    {
	    GetTime(totsec, timestr);
	    msg.Sprintf(_(" Total: %s"), timestr.GetString());
    }
	
    m_text[2].SetAs(msg);
    UpdatePart(2);
	return;
}

// Formats and sets the number of rows returned in the resultset after execution of a query.
// This message is shown in the third part of the status bar.

void
StatusBarMgmt::AddNumRows(wyUInt32   rows, wyBool clear)
{
	wyString msg;

    if(clear == wyTrue)
    {
        msg.SetAs("");
    }
    else
    {
	    msg.Sprintf(_(" %lu row(s)"), rows);
    }

    m_text[3].SetAs(msg);
    UpdatePart(3);
	return;
}

// Formats and add line and column number in which the cursor is located in the edit box.
void
StatusBarMgmt::AddLineColNum(HWND hwnd, wyBool iscintilla)
{
	wyString    msg; 
	wyInt32     col, pos;
	wyUInt32    charpos, line = 1, start, end, ncol = 1, index;
	POINT	    pt;
	
	if(hwnd && iscintilla)
    {
        pos = SendMessage(hwnd, SCI_GETCURRENTPOS, 0, 0);
		line = SendMessage(hwnd, SCI_LINEFROMPOSITION, pos, 0 ) + 1;
        ncol = SendMessage(hwnd, SCI_GETCOLUMN, pos, 0) + 1;
	}
    else if(hwnd)
    {
		GetCaretPos(&pt);
		charpos = SendMessage(hwnd, EM_CHARFROMPOS, 0,(LPARAM)&pt);
		line = SendMessage(hwnd, EM_LINEFROMCHAR, charpos, 0);
		SendMessage(hwnd, EM_GETSEL,(WPARAM)&start,(LPARAM)&end);
		index = SendMessage(hwnd, EM_LINEINDEX, line, 0);
		ncol = end - index + 1;
		col = (wyInt32)ncol;
        line++;

		if(col <= 0)
        {
			return;
        }

	}

    if(!hwnd)
    {
        msg.SetAs("");
    }
    else
    {
	    msg.Sprintf(" Ln %d, Col %d", line, ncol);
    }

    m_text[4].SetAs(msg);
    UpdatePart(4);
	return;
}

// Function formats and adds shows whether query executed had error or not.
// This message is shown in the first part of the status bar.
void
StatusBarMgmt::AddQueryResult(wyBool issuccess)
{
    wyString msg;
	MDIWindow	*wnd = GetActiveWin();

	if(issuccess)
    {
		msg.Sprintf(_(" Query batch completed successfully"));
		if(wnd)
		{
			wnd->m_statusbartext.SetAs(_(" Query batch completed successfully"));
		}
    }
	else
    {
		msg.Sprintf(_(" Query batch completed with error(s)"));
		if(wnd)
		{
			wnd->m_statusbartext.SetAs(_(" Query batch completed with error(s)"));
		}
    }

    m_text[0].SetAs(msg);
    UpdatePart(0);
    return;
}

// Function shows some addtional information if required inthe first part of the status bar.
// Message to be shown is passed as parameter.
void
StatusBarMgmt::ShowInformation(wyWChar *msg, wyInt32 part)
{
    if(part >= 0 && part < m_countparts && msg)
    {
        m_text[part].SetAs(msg);
        UpdatePart(part);
    }
}

void
StatusBarMgmt::UpdatePart(wyInt32 part)
{
    RECT rect = {0};

    SendMessage(m_hwnd, SB_GETRECT, part, (LPARAM)&rect);
    InvalidateRect(m_hwnd, &rect, FALSE);
    UpdateWindow(m_hwnd);
}
