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

/*********************************************

Author: Vishal P.R, Janani Sriguha

*********************************************/

#include "TabTableData.h"

class MDIWindow;

TabTableData::TabTableData(MDIWindow *wnd, HWND hwndparent, wyBool issticky):TabTypes(hwndparent)
{
    m_hwndparent	= hwndparent;
	m_pmdi			= wnd;
	m_peditorbase	= NULL;
	m_tunnel		= m_pmdi->m_tunnel;
    m_isrefreshed   = wyFalse;
    m_istabsticky   = issticky;
    m_tabledata		= wnd->m_pcqueryobject->IsSelectionOnTable() == wyTrue ? new MySQLTableDataEx(wnd) : NULL;
	m_tableview		= wnd->m_pctabmodule->m_tableview;
}


TabTableData::~TabTableData(void)
{
    delete m_tabledata;
}

wyInt32
TabTableData::CreateTab(wyBool issetfocus)
{    
    wyInt32				count = 0;
    CTCITEM				item = {0};
	wyString			buffer, buffer2;
    wyInt32             len = 0;
	
    buffer.Sprintf("`%s`.`%s`", m_pmdi->m_pcqueryobject->m_seldatabase.GetString(), m_pmdi->m_pcqueryobject->m_seltable.GetString());

    if(m_istabsticky == wyTrue)
    {
	    len = m_pmdi->m_pcqueryobject->m_seltable.GetLength();

        if(len > 24)
        {
            buffer2.SetAs(m_pmdi->m_pcqueryobject->m_seltable.Substr(0, 12));
            buffer2.Add("...");
            buffer2.Add(m_pmdi->m_pcqueryobject->m_seltable.Substr(len - 13, 12));
            
        }
        else
        {
            buffer2.SetAs(m_pmdi->m_pcqueryobject->m_seltable);
        }
        item.m_psztext    = (wyChar*)buffer2.GetString();
        item.m_cchtextmax = buffer2.GetLength();
    }
    else
    {
        if(m_pmdi->m_pcqueryobject->IsSelectionOnTable() == wyFalse)
        {
            buffer.Sprintf(_("Table Data"));
        }
            
        item.m_psztext    = _("Table Data");
	    item.m_cchtextmax = strlen(_("Table Data"));
    }

	item.m_mask       = CTBIF_IMAGE | CTBIF_TEXT | CTBIF_LPARAM | CTBIF_CMENU  | CTBIF_TOOLTIP;
	item.m_iimage     = IDI_TABLE;
	item.m_tooltiptext = (wyChar*)buffer.GetString();
	item.m_lparam     = (LPARAM)this;
	
	count = CustomTab_GetItemCount(m_hwndparent);
    CustomTab_InsertItem(m_hwndparent, count, &item);
    CustomTab_SetItemLongValue(m_hwndparent, count, (LPARAM)this);

    if(issetfocus == wyTrue)
    {
        CustomTab_SetCurSel(m_hwndparent, count);
	    CustomTab_EnsureVisible(m_hwndparent, count, wyTrue);	
    }

    return count;
}


wyBool      
TabTableData::CloseTab(wyInt32 index)
{
    m_tableview->ShowAll(SW_HIDE);
    m_tableview->SetData(NULL);
	return wyTrue;
}

VOID
TabTableData::ShowTabContent(wyInt32 tabindex, wyBool status)
{
    MySQLTableDataEx*   temp;
    CTCITEM				tmp = {0};
	wyString			buffer;
    wyBool              isselontable = m_pmdi->m_pcqueryobject->IsSelectionOnTable();

    if(status == wyTrue)
    {
        if((m_tabledata && (!m_tabledata->m_db.Compare(m_pmdi->m_pcqueryobject->m_seldatabase)
            && !m_tabledata->m_table.Compare(m_pmdi->m_pcqueryobject->m_seltable)) 
            || isselontable == wyFalse) || m_istabsticky == wyTrue)
        {
            m_tableview->SetData(m_tabledata);

            if(m_pmdi->m_ismdiclosealltabs == wyFalse && m_tabledata &&
                (m_isrefreshed == wyFalse || (m_istabsticky == wyFalse && IsRefreshTableData())))
            {
                ExecuteTableDataQuery();
            }
        }
        else
        {
            temp = m_tabledata;
            m_tabledata = new MySQLTableDataEx(m_pmdi);
            m_tableview->SetData(m_tabledata);

            if(m_pmdi->m_ismdiclosealltabs == wyFalse)
            {
                ExecuteTableDataQuery();
                tmp.m_mask = CTBIF_IMAGE | CTBIF_TEXT | CTBIF_LPARAM | CTBIF_TOOLTIP;
                buffer.Sprintf("`%s`.`%s`", m_pmdi->m_pcqueryobject->m_seldatabase.GetString(), m_pmdi->m_pcqueryobject->m_seltable.GetString());
                tmp.m_tooltiptext = (wyChar*)buffer.GetString();
                tmp.m_psztext = _("Table Data");
                tmp.m_cchtextmax = strlen(_("Table Data"));
                tmp.m_iimage = IDI_TABLE;
                tmp.m_lparam = (LPARAM)this;
                CustomTab_SetItem(m_hwndparent, tabindex, &tmp);
            }

            delete temp;
        }

    }

    m_tableview->ShowAll(status == wyTrue ? SW_SHOW : SW_HIDE);
}

void
TabTableData::Resize(wyBool issplittermoved)
{
	
}

VOID		
TabTableData::OnTabSelChange()
{
    if(pGlobals->m_pcmainwin->m_finddlg)
	{
		DestroyWindow(pGlobals->m_pcmainwin->m_finddlg);
		pGlobals->m_pcmainwin->m_finddlg = NULL;
	}

    SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)ID_OBJECT_INSERTUPDATE, TBSTATE_ENABLED);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ACCEL_EXECUTEALL,(LPARAM)TBSTATE_INDETERMINATE);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)IDM_EXECUTE,(LPARAM)TBSTATE_INDETERMINATE);
    SendMessage(pGlobals->m_pcmainwin->m_hwndtool, TB_SETSTATE,(WPARAM)ACCEL_QUERYUPDATE ,(LPARAM)TBSTATE_INDETERMINATE); 
	SendMessage(pGlobals->m_pcmainwin->m_hwndsecondtool, TB_SETSTATE,(WPARAM)ID_FORMATCURRENTQUERY,(LPARAM)TBSTATE_INDETERMINATE);
    m_pmdi->m_pcquerystatus->AddNumRows(m_tabledata && m_tabledata->m_datares ? m_tabledata->m_datares->row_count : 0);
}

VOID		
TabTableData::OnTabSelChanging()
{

}

wyInt32		
TabTableData::OnTabClosing(wyBool ismanual)
{
    if(ismanual == wyTrue && m_istabsticky == wyFalse)
    {
        GetTabOpenPersistence(IDI_TABLE, wyTrue);
    }

	return 1;
}

VOID		
TabTableData::OnTabClose()
{
}

wyInt32		
TabTableData::OnWmCloseTab()
{
	return 0;
}

void
TabTableData::GetCurrentSelection()
{

}

wyBool 
TabTableData::FreeMySQLResources(wyBool isfreemyres)
{
	return wyTrue;
}

wyBool
TabTableData::ReExecute(wyInt32 action)
{
		return wyTrue;
}

VOID		
TabTableData::HandleTabControls(wyInt32 tabcount, wyInt32 selindex)
{

}

VOID		
TabTableData::HandleFlicker()
{
}
	
void        
TabTableData::OnGetChildWindows(wyInt32 tabcount, LPARAM lparam)
{

}

wyBool 
TabTableData::FreeItemData()
{
    wyBool			ret = wyFalse;


    return ret;
}

wyBool 
TabTableData::FreeItemData(TABLEDATARES *tbldatares, wyBool isnewrow)
{
		
	return wyTrue;
}

wyBool
TabTableData::ExecuteTableDataQuery()
{	
    m_tableview->Execute(TA_REFRESH, wyTrue, wyTrue);
	return wyTrue;
}

void
TabTableData::SetBufferedDrawing(wyBool isset)
{
    if(m_tableview)
    {
        m_tableview->SetBufferedDrawing(isset);
    }
}