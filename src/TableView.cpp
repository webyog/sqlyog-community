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

Author: Vishal P.R, Janani SriGuha

*********************************************/

#include "TableView.h"
#include "Symbols.h"
#include "FrameWindowHelper.h"
#include "FKDropDown.h"

class FKDropDown;

//constructor
MySQLTableDataEx::MySQLTableDataEx(MDIWindow* pmdi) : MySQLDataEx(pmdi)
{
    //if OB selection is on table/view, set db and table name
    if(pmdi->m_pcqueryobject->IsSelectionOnTable() == wyTrue)
    {
        m_db.SetAs(pmdi->m_pcqueryobject->m_seldatabase);
        m_table.SetAs(m_pmdi->m_pcqueryobject->m_seltable);
    }

    //get the view
    m_viewtype = HandleViewPersistence(wyFalse);

    //get the limits
    HandleLimitPersistence(wyFalse);

    //create server side sort/filtering
    m_psortfilter = new SortAndFilter(wyFalse, this);
}

//function to handle limit persistence
void 
MySQLTableDataEx::HandleLimitPersistence(wyBool isset)
{
    wyInt32 limit = -1;
    wyInt64 startrow = -1;

    //get limit persistence?
    if(isset == wyFalse)
    {
        //get the limits, if it is false, set the values to zero
        if((m_islimit = HandleDatatabLimitPersistance(m_db.GetString(), m_table.GetString(), (wyInt64*)&m_startrow, (wyInt32*)&m_limit)) == wyFalse)
        {
            m_startrow = 0;
            m_limit = 0;
        }

        //set the values
        m_startrow = max(m_startrow, 0);
        m_limit = max(m_limit, 0);
    }
    //set limit persistence
    else
    {
        //set the values
        if(m_islimit == wyTrue)
        {
            startrow = m_startrow;
            limit = m_limit;
        }

        //store it
        HandleDatatabLimitPersistance(m_db.GetString(), m_table.GetString(), &startrow, &limit, wyTrue);
    }
}

//function to handle view persistence for a table/view
ViewType 
MySQLTableDataEx::HandleViewPersistence(wyBool isset)
{
    ViewType    view;

    //precaution
    if(!m_db.GetLength() && !m_table.GetLength())
    {
        return m_viewtype;
    }

    //get/set view persistence
    view = m_viewtype;
    view = (ViewType)HandleTableViewPersistance(m_db.GetString(), 
                                                m_table.GetString(), 
                                                m_viewtype, isset);

    //if it is form view, make sure we have valid licenece
    if(isset == wyFalse && view == FORM)
    {
#ifndef COMMUNITY
        if(!pGlobals->m_entlicense.CompareI("Professional"))
        {
            view = GRID;
        }
#else
        view = GRID;
#endif
    }

    return view;
}

//view constructor
TableView::TableView(MDIWindow *wnd, HWND hwndparent):DataView(wnd, hwndparent, (IQueryBuilder *)this)
{
	m_mydata = NULL;
}

//destructor
TableView::~TableView()
{
}

//set the data
void
TableView::SetData(MySQLDataEx *data)
{
    //set the data for this class
    m_mydata = (MySQLTableDataEx*)data;

    //call the base class version
	DataView::SetData(data);  
}

//function to get the banner text 
void 
TableView::GetBanner(wyString& bannertext)
{
    bannertext.Clear();

    //set the text
    if(!m_mydata || !m_mydata->m_table.GetLength() || !m_mydata->m_db.GetLength())
    {
        bannertext.SetAs(_("Select a Table/View from the Object Browser"));
    }
}

//create toolbar
void
TableView::CreateToolBar()
{
    wyUInt32 style = WS_CHILD | CCS_NOPARENTALIGN | TBSTYLE_TOOLTIPS | WS_VISIBLE | CCS_NODIVIDER | TBSTYLE_FLAT;

	m_hwndtoolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, style, 
                                   0, 0, 0, 0, m_hwndframe,
                                   (HMENU)IDC_TOOLBAR, (HINSTANCE)GetModuleHandle(0), NULL);

    //add tool buttons
	AddToolButtons();

    //size the toolbar
    SendMessage(m_hwndtoolbar, TB_AUTOSIZE, 0, 0);

    //remove auto sizing
    style = GetWindowLongPtr(m_hwndtoolbar, GWL_STYLE);
    SetWindowLongPtr(m_hwndtoolbar, GWL_STYLE, style | CCS_NORESIZE);
}

//function to add tool bar buttons
void 
TableView::AddToolButtons()
{
	INT			i = 0, j, size, k;
	HICON		hicon;
	TBBUTTON	tbb[30];

	wyInt32 command[] = {
        IDM_IMEX_EXPORTDATA, 
		IDM_DATATOCLIPBOARD,
		IDM_SEPARATOR,	ID_RESULT_INSERT, IDM_DUPLICATE_ROW, ID_RESULT_SAVE,
        ID_RESULT_DELETE, ID_RESULT_CANCEL,
        IDM_SEPARATOR, ID_VIEW_GRIDVIEW, ID_VIEW_FORMVIEW,
        ID_VIEW_TEXTVIEW
    };

	wyUInt32 states[][2] = {
        {TBSTATE_ENABLED, TBSTYLE_BUTTON | BTNS_AUTOSIZE},  
		{TBSTATE_ENABLED, BTNS_WHOLEDROPDOWN}, 
		{TBSTATE_ENABLED, TBSTYLE_SEP}, 
		{TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
		{TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON},  
		{TBSTATE_ENABLED, TBSTYLE_SEP}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
        {TBSTATE_ENABLED, TBSTYLE_BUTTON}
    };

	wyInt32 imgres[] = {
        IDI_TABLEEXPORT, 
		IDI_EXPORTRESULTCSV,
		IDI_USERS,  
        IDI_RESULTINSERT, IDI_DUPLICATEROW, IDI_SAVE,
		IDI_RESULTDELETE, IDI_RESULTCANCEL, 
        IDI_USERS, IDI_DATAGRID,
        IDI_FORMICON, IDI_TEXTVIEW
    };

	m_himglist = ImageList_Create(ICON_SIZE, ICON_SIZE, ILC_COLOR32  | ILC_MASK, 1, 0);
	//SendMessage(m_hwndtoolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(25, 25));
	SendMessage(m_hwndtoolbar, TB_SETIMAGELIST, 0, (LPARAM)m_himglist);
	SendMessage(m_hwndtoolbar, TB_SETEXTENDEDSTYLE, 0 , (LPARAM)TBSTYLE_EX_DRAWDDARROWS);
    SendMessage(m_hwndtoolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    size = sizeof(command)/sizeof(command[0]);

	for(j = 0, k = 0; j < size; j++)	
	{
#ifndef COMMUNITY
        if(command[j] == ID_VIEW_FORMVIEW && !m_formview)
        {
            continue;
        }
#endif
		hicon = (HICON)LoadImage(pGlobals->m_hinstance, MAKEINTRESOURCE(imgres[j]), IMAGE_ICON, ICON_SIZE, ICON_SIZE, LR_DEFAULTCOLOR);
		i = ImageList_AddIcon(m_himglist, hicon);
		DestroyIcon(hicon);
		memset(&tbb[k], 0, sizeof(TBBUTTON));

		tbb[k].iBitmap = MAKELONG(i, 0);
		tbb[k].idCommand = command[j];
		tbb[k].fsState = (UCHAR)states[j][0];
		tbb[k].fsStyle = (UCHAR)states[j][1];
        ++k;
	}  

    //add extra images
    AddExtraImages(m_himglist);
	SendMessage(m_hwndtoolbar, TB_ADDBUTTONS, (WPARAM)k, (LPARAM) &tbb);

    //set the image indexes as lparam for buttons
    SetImageIndexes(m_hwndtoolbar);
}

//function to resize the view
void 
TableView::Resize()
{
    RECT	    rcparent, rctemp, tbrect;
	wyInt32		hpos, vpos, width, height,toolwidth, toolheight;
    wyInt32     padding = 6, itemcount;
    TBBUTTON    tbinfo = {0};

    GetClientRect(m_hwndparent, &rcparent);
	
    hpos	= rcparent.left;
    vpos	= rcparent.top = CustomTab_GetTabHeight(m_hwndparent);
	width	= rcparent.right - rcparent.left;
	height	= rcparent.bottom - rcparent.top;
	
    SetWindowPos(m_hwndframe, HWND_BOTTOM, hpos, vpos, width, height,  SWP_NOZORDER);
    GetClientRect(m_hwndframe, &rcparent);

    GetWindowRect(m_hwndtoolbar, &tbrect);
    ResizeRefreshTool(tbrect.bottom - tbrect.top, &tbrect);
    toolheight = tbrect.bottom - tbrect.top;
        
    itemcount = SendMessage(m_hwndtoolbar, TB_BUTTONCOUNT, 0, 0) - 1;
    memset(&rctemp, 0, sizeof(RECT));

    if(itemcount >= 0)
    {
        SendMessage(m_hwndtoolbar, 
            TB_GETBUTTON, 
            itemcount,
            (LPARAM)&tbinfo);

        SendMessage(m_hwndtoolbar, TB_GETRECT, tbinfo.idCommand, (LPARAM)&rctemp);
        rctemp.right += padding;
    }

    toolwidth = rctemp.right;

    if(toolwidth + tbrect.right < rcparent.right)
    {
        toolwidth = rcparent.right - tbrect.right;
    }

    SetWindowPos(m_hwndpadding, NULL, 0, 0, toolwidth + tbrect.right, DATAVIEW_TOPPADDING, SWP_NOZORDER);
    SetWindowPos(m_hwndtoolbar, NULL, 0, DATAVIEW_TOPPADDING, toolwidth, toolheight, SWP_NOZORDER);
    SetWindowPos(m_hwndrefreshtool, NULL, toolwidth, DATAVIEW_TOPPADDING, tbrect.right, toolheight, SWP_NOZORDER);
        
    rcparent.top += toolheight + DATAVIEW_TOPPADDING;
    ResizeControls(rcparent);
    InvalidateRect(m_hwndframe, NULL, TRUE);
    UpdateWindow(m_hwndframe);
}

//refresh data view
wyInt32
TableView::RefreshDataView()
{
    return ExecuteTableData();
}

//reset the button with their original image
void 
TableView::ResetToolBarButtons()
{
    //refresh toolbar
    if(m_hwndrefreshtool)
    {
        ResetButtons(m_hwndrefreshtool);
    }

    //toolbar
    if(m_hwndtoolbar)
    {
        ResetButtons(m_hwndtoolbar);
    }
}

/// IQueryBuilder implementation
/**
@returns wyString. Caller need to destroy this
*/
void
TableView::GetQuery(wyString& query)
{
	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

	query.Sprintf("select * from %s%s%s.%s%s%s\r\n", 
		m_backtick, m_mydata->m_db.GetString(), m_backtick,
		m_backtick, m_mydata->m_table.GetString(), m_backtick);

	//get filter info
	GetFilterInfo(query);

	//get sort info
	GetSortInfo(query);

	//get limits
	GetLimits(query);

	query.Add(";\r\n");
}

//function to execute SELECT * query
wyInt32
TableView::ExecuteTableData()
{
	wyString        query;
    wyInt32         ret,extraindex,j=0,no_row;
    MySQLDataEx*    pdata;
	MYSQL_ROW        fieldrow;

	GetQuery(query);

    //execut query
    m_mydata->m_datares = ExecuteQuery(query);

    //is thread stopped
	if(ThreadStopStatus())
	{	
        return TE_STOPPED;
	}

    //any error? show error dialog
	if(!m_mydata->m_datares)
	{
        return HandleErrors(query);
	}

    //allocate row array, if the thread is stopped, delete them
    if((ret = AllocateRowsExArray()) != TE_SUCCESS || 
        (ret = GetTableDetails()) != TE_SUCCESS)
    {
        pdata = ResetData(m_data);
        delete pdata;
        return ret;
    }
	extraindex = GetFieldIndex(m_wnd->m_tunnel, m_data->m_fieldres, "Extra");
	no_row = m_wnd->m_tunnel->mysql_num_rows(m_data->m_fieldres);
	m_data->m_colvirtual = (wyInt32*)calloc(no_row, sizeof(wyInt32));

	while(fieldrow = m_wnd->m_tunnel->mysql_fetch_row(m_data->m_fieldres)){

	if(!strstr(fieldrow[extraindex], "VIRTUAL") && !strstr(fieldrow[extraindex], "PERSISTENT") && !strstr(fieldrow[extraindex], "STORED"))
		{
			m_data->m_colvirtual[j++] = 0;
		}

		else 
		{
			m_data->m_colvirtual[j++] = 1;
		}
	
	}

    //add new row in the end
    AddNewRow();

    return TE_SUCCESS;
}

//function to reset the data
MySQLDataEx* 
TableView::ResetData(MySQLDataEx* pdata)
{
    MySQLTableDataEx*   ptemp = NULL;
    MySQLTableDataEx*   tempdata = (MySQLTableDataEx*)pdata;
    SortAndFilter*      psfnew;

    if(pdata)
    {
        //create temperory object
        ptemp = new MySQLTableDataEx(pdata->m_pmdi);

        //clear the row array for the object
        delete ptemp->m_rowarray;
        ptemp->m_rowarray = NULL;

        //get the original sort and filter for the temperory data
        psfnew = ptemp->m_psortfilter;

        *ptemp = *tempdata;

        //copy the filter into the filter for the temperory data
        if(psfnew)
        {
            *psfnew = *tempdata->m_psortfilter;
        }

        //restore the original filter for the temperory data
        ptemp->m_psortfilter = psfnew;

        //set row array to NULL, so that the initialization module will allocate a new one
        pdata->m_rowarray = NULL;

        //initialize the data
        pdata->Initialize();
    }
    
    return ptemp;
}

//function to draw the bottom ribbon with table name and db name
void 
TableView::DrawTableInfoRibbon(HDC hdc, RECT drawrect)
{
    HFONT   hfontnormal = GetStockFont(DEFAULT_GUI_FONT);
    HFONT   hfontbold = NULL;
    LOGFONT lf = {0};
    RECT    recttext;

    if(m_mydata && m_mydata->m_db.GetLength() && m_mydata->m_table.GetLength())
    {
        GetObject(hfontnormal, sizeof(lf), &lf);
        lf.lfWeight = FW_SEMIBOLD;
        hfontbold = CreateFontIndirect(&lf);

        hfontbold = (HFONT)SelectObject(hdc, hfontbold);
        memset(&recttext, 0, sizeof(RECT));
        DrawText(hdc, _(L"Database:"), -1, &recttext, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
        DrawText(hdc, _(L"Database:"), -1, &drawrect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
        hfontbold = (HFONT)SelectObject(hdc, hfontbold);

        drawrect.left += recttext.right + 5;

        hfontnormal = (HFONT)SelectObject(hdc, hfontnormal);
        memset(&recttext, 0, sizeof(RECT));
        DrawText(hdc, m_mydata->m_db.GetAsWideChar(), -1, &recttext, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
        DrawText(hdc, m_mydata->m_db.GetAsWideChar(), -1, &drawrect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
        hfontnormal = (HFONT)SelectObject(hdc, hfontnormal);
        
        drawrect.left += recttext.right + 10;

        hfontbold = (HFONT)SelectObject(hdc, hfontbold);
        memset(&recttext, 0, sizeof(RECT));
        DrawText(hdc, _(L"Table:"), -1, &recttext, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
        DrawText(hdc, _(L"Table:"), -1, &drawrect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
        hfontbold = (HFONT)SelectObject(hdc, hfontbold);

        drawrect.left += recttext.right + 5;

        hfontnormal = (HFONT)SelectObject(hdc, hfontnormal);
        memset(&recttext, 0, sizeof(RECT));
        DrawText(hdc, m_mydata->m_table.GetAsWideChar(), -1, &recttext, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
        DrawText(hdc, m_mydata->m_table.GetAsWideChar(), -1, &drawrect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
        hfontnormal = (HFONT)SelectObject(hdc, hfontnormal);
    }

    if(hfontbold)
    {
        DeleteFont(hfontbold);
    }

    if(hfontnormal)
    {
        DeleteFont(hfontnormal);
    }
}

//get the index of the column from the field res
wyInt32         
TableView::GetFieldResultIndexOfColumn(wyInt32 col)
{
    //since we are doing a select * and the columns appear in the same order as field res, column index itself is the index of the column in field res
    return col;
}

//function to show help file
void
TableView::ShowHelpFile()
{
    ShowHelp("http://sqlyogkb.webyog.com/article/102-data-manipulation");
}

//get the database name
wyBool 
TableView::GetDBName(wyString& db, wyInt32 col)
{
    db.SetAs(m_data->m_db);
    return db.GetLength() ? wyTrue : wyFalse;
}

//get table name
wyBool 
TableView::GetTableName(wyString& table, wyInt32 col)
{
    table.SetAs(m_data->m_table);
    return table.GetLength() ? wyTrue : wyFalse;
}

//get column name
wyBool 
TableView::GetColumnName(wyString& column, wyInt32 col)
{
    if(m_data && m_data->m_datares && col >= 0 && col < m_data->m_datares->field_count)
    {
        if(m_data->m_datares->fields[col].name && m_data->m_datares->fields[col].name[0])
        {
            column.SetAs(m_data->m_datares->fields[col].name, m_wnd->m_ismysql41);
        }

        return column.GetLength() ? wyTrue : wyFalse;
    }

    return wyFalse;
}

//function to get auto incrment column index
wyInt32 
TableView::GetAutoIncrIndex()
{
	wyInt32		colindex = 0;
	MYSQL_ROW	myrow;
	wyInt32		extraval;
	wyString	myrowstr;

    //if no field res
    if(!m_mydata->m_fieldres)
    {
        return -1;
    }

    //get the column index of extra in the field res
	m_wnd->m_tunnel->mysql_data_seek(m_mydata->m_fieldres, 0);
	extraval = GetFieldIndex(m_mydata->m_fieldres, "extra", m_wnd->m_tunnel, &m_wnd->m_mysql);

    //now loop through and check for auto_increment in extra column
	while(myrow = m_wnd->m_tunnel->mysql_fetch_row(m_mydata->m_fieldres))
	{
        if(myrow[extraval] && strstr(myrow[extraval], "auto_increment")) 
        {
            return colindex;
        }
			
        colindex++;
	}

	return -1;
}
