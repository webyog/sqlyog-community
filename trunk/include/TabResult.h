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


#ifndef _QueryResultEdit_H_
#define _QueryResultEdit_H_

#include "FrameWindowHelper.h"
#include "TabQueryTypes.h"
#include "FindAndReplace.h"
#include "SortAndFilter.h"

class TabResult	: public TabQueryTypes
{
public:

    TabResult(MDIWindow* wnd, HWND hwndparent, ResultView* presultview);
	~TabResult();

    MySQLResultDataEx* GetResultData();
    void Resize();

    void OnTabSelChange(wyBool isselected);

    void UpdateStatusBar(StatusBarMgmt* pmgmt);

    void OnTabSelChanging();

    void        SetBufferedDrawing(wyBool isset);

    ResultView* m_presultview;
    MySQLResultDataEx* m_data;

    //FindAndReplace  *m_findreplace;
};

#endif
