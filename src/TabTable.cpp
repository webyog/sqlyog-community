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
#include "ExportMultiFormat.h"
#include "CommonHelper.h"
#include "GUIHelper.h"
#include "TabTable.h"

extern	PGLOBALS		pGlobals;

TabTable::TabTable(MDIWindow* wnd, HWND hwndparent, MySQLTableDataEx* data) : TabQueryTypes(wnd, hwndparent)
{
    m_ptableview = new TableView(wnd, hwndparent);
    m_data = data;
    m_preferencedptr = NULL;
}

TabTable::~TabTable()
{
    m_ptableview->SetData(NULL);
    delete m_data;
    delete m_ptableview;

    if(m_preferencedptr)
    {
        *m_preferencedptr = NULL;
    }
}

MySQLTableDataEx*
TabTable::GetTableData()
{
    return m_data;
}

void
TabTable::OnTabSelChange(wyBool isselected)
{
    if(isselected == wyTrue)
    {
        m_ptableview->SetData(m_data);
    }
    
    m_ptableview->ShowAll(isselected == wyTrue ? SW_SHOW : SW_HIDE);
    
    if(isselected == wyTrue)
    {
        SetFocus(m_ptableview->GetActiveDispWindow());
        m_pmdi->m_pcquerystatus->AddTickCount(NULL, 0, 0, wyTrue);
        m_pmdi->m_pcquerystatus->AddNumRows(m_data ? m_data->GetSavedRowCount() : 0);
        Refresh();
    }
}

void
TabTable::OnTabSelChanging()
{
}

void
TabTable::Resize()
{
    m_ptableview->Resize();
}

void
TabTable::UpdateStatusBar(StatusBarMgmt* pmgmt)
{
    pmgmt->AddTickCount(NULL, 0, 0, wyTrue);
    pmgmt->AddNumRows(m_data ? m_data->GetSavedRowCount() : 0);

    if(!m_data || (m_pmdi->m_pcqueryobject->IsSelectionOnTable() == wyTrue && 
        (m_data->m_db.Compare(m_pmdi->m_pcqueryobject->m_seldatabase) ||
         m_data->m_table.Compare(m_pmdi->m_pcqueryobject->m_seltable))))
    {
        Refresh();
    }
}

void
TabTable::SetBufferedDrawing(wyBool isset)
{
    m_ptableview->SetBufferedDrawing(isset);
}

void
TabTable::Create()
{
    m_ptableview->Create();
    m_ptableview->Resize();
}

void
TabTable::Refresh()
{
    MySQLTableDataEx*   ptemp;
    wyInt32             diffdb = 0, difftable = 0;

    if(!m_data || 
        (m_pmdi->m_pcqueryobject->IsSelectionOnTable() == wyTrue && 
         ((diffdb = m_data->m_db.Compare(m_pmdi->m_pcqueryobject->m_seldatabase)) || 
         (difftable = m_data->m_table.Compare(m_pmdi->m_pcqueryobject->m_seltable)))) ||
        IsRefreshTableData() == wyTrue)
    {
        if(m_data)
        {
            if((diffdb || difftable))
            {
                ptemp = m_data;
                m_data = new MySQLTableDataEx(m_pmdi);
                m_ptableview->SetData(m_data);
                delete ptemp;
            }
        }
        else if(m_pmdi->m_pcqueryobject->IsSelectionOnTable() == wyTrue)
        {
            m_data = new MySQLTableDataEx(m_pmdi);
            m_ptableview->SetData(m_data);
        }

        if(m_data && m_pmdi->m_ismdiclosealltabs == wyFalse)
        {
            m_ptableview->Execute(TA_REFRESH, wyTrue, wyTrue);
        }
    }
}