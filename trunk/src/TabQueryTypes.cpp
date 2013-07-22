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


#include "TabQueryTypes.h"
#include "StatusbarMgmt.h"
#include "Tunnel.h"

TabQueryTypes::TabQueryTypes(MDIWindow* wnd, HWND hwndparent)
{
    m_pmdi = wnd;
    m_hwndparent = hwndparent;
}

TabQueryTypes::~TabQueryTypes()
{
}

void 
TabQueryTypes::OnTabSelChanging()
{

}

void TabQueryTypes::SetBufferedDrawing(wyBool isset)
{

}


void
TabQueryTypes::UpdateStatusBar(StatusBarMgmt* pmgmt)
{
    pmgmt->AddNumRows(0, wyTrue);
    pmgmt->AddTickCount(NULL, 0, 0, wyTrue);
}

