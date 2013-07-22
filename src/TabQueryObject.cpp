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

#include <wchar.h>
#include "MDIWindow.h"
#include "Global.h"
#include "FrameWindowHelper.h"
#include "CommonHelper.h"
#include "GUIHelper.h"
#include "TabQueryObject.h"

extern	PGLOBALS		pGlobals;

TabQueryObject::TabQueryObject(MDIWindow* wnd, HWND hwndparent) : TabQueryTypes(wnd, hwndparent)
{
    m_pobjectinfo = new ObjectInfo(wnd, hwndparent);
    m_preferencedptr = NULL;
}

TabQueryObject::~TabQueryObject()
{
    delete m_pobjectinfo;

    if(m_preferencedptr)
    {
        *m_preferencedptr = NULL;
    }
}

void
TabQueryObject::OnTabSelChange(wyBool isselected)
{
    m_pobjectinfo->Show(isselected);
    
    if(isselected == wyTrue)
    {
        SetFocus(m_pobjectinfo->GetActiveDisplayWindow());
        UpdateStatusBar(m_pmdi->m_pcquerystatus);
    }
}

void
TabQueryObject::OnTabSelChanging()
{
}

void
TabQueryObject::Resize()
{
    m_pobjectinfo->Resize();
}

void
TabQueryObject::UpdateStatusBar(StatusBarMgmt* pmgmt)
{
    pmgmt->AddTickCount(NULL, 0, 0, wyTrue);
    pmgmt->AddNumRows(0, wyTrue);
    Refresh();
}

void
TabQueryObject::SetBufferedDrawing(wyBool isset)
{
}

void
TabQueryObject::Create()
{
    m_pobjectinfo->Create();
    m_pobjectinfo->Resize();
}

void
TabQueryObject::Refresh(wyBool isforce)
{
    if(m_pmdi->m_ismdiclosealltabs == wyFalse)
    {
        m_pobjectinfo->Refresh(isforce);
    }
}