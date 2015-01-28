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

#include  <wchar.h>
#include "MDIWindow.h"
#include "Global.h"
#include "TabResult.h"
#include "FrameWindowHelper.h"
#include "ExportMultiFormat.h"
#include "CommonHelper.h"

#include "GUIHelper.h"

extern	PGLOBALS		pGlobals;

TabResult::TabResult(MDIWindow* wnd, HWND hwndparent, ResultView* presultview) : TabQueryTypes(wnd, hwndparent)
{
    m_presultview = presultview;
	pctabmgmt=wnd->m_pctabmodule->GetActiveTabEditor()->m_pctabmgmt;
    m_data = NULL;
}

TabResult::~TabResult()
{
    m_presultview->SetData(NULL);
    delete m_data;
    //m_presultview->Execute(TA_CLEARDATA, wyFalse, wyTrue);
    m_data = NULL;
}

MySQLResultDataEx*
TabResult::GetResultData()
{
    return m_data;
}

void
TabResult::OnTabSelChange(wyBool isselected)
{
    if(isselected == wyTrue)
    {
        m_presultview->SetData(m_data);
    }
    
    m_presultview->ShowAll(isselected == wyTrue ? SW_SHOW : SW_HIDE);
    
    if(isselected == wyTrue)
    {
        SetFocus(m_presultview->GetActiveDispWindow());
        UpdateStatusBar(m_pmdi->m_pcquerystatus);
    }
}

void
TabResult::OnTabSelChanging()
{

}

void
TabResult::Resize()
{
    m_presultview->Resize();
}

void
TabResult::UpdateStatusBar(StatusBarMgmt* pmgmt)
{
	
	pmgmt->AddTickCount(m_data->m_pmdi->m_tunnel, pctabmgmt->m_pcquerymessageedit->m_sumofexectime,pctabmgmt->m_pcquerymessageedit->m_sumoftotaltime);
    pmgmt->AddNumRows(m_data ? m_data->GetSavedRowCount() : 0);
}


void
TabResult::SetBufferedDrawing(wyBool isset)
{
    m_presultview->SetBufferedDrawing(isset);
}