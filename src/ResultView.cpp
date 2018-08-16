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

#include "ResultView.h"
#include "Include.h"
#include "FrameWindowHelper.h"
#include "FKDropDown.h"
#include "WyCrc.h"
#include "SQLFormatter.h"

#ifndef COMMUNITY
#include "FormView.h"
#endif

//constructor
MySQLResultDataEx::MySQLResultDataEx(MDIWindow* pmdi, const wyChar* query, wyUInt32 len) : MySQLDataEx(pmdi)
{
    //initialize the variables
    m_isselectquery = wyFalse;
    m_isquerylimited = wyFalse;
    m_ispartofmultiresult = wyFalse;
    m_isExplain = wyFalse;
    SetQuery(query, len);
    Initialize();

    //set view persistence
    m_viewtype = HandleViewPersistence(wyFalse);

    //create client side sorting
    m_psortfilter = new SortAndFilter(wyTrue, this);
}

//function initialize the modifyable members
void
MySQLResultDataEx::Initialize()
{
    //call the base class version to reduce the code
    MySQLDataEx::Initialize();

    m_totaltime = 0;
    m_exectime = 0;
    m_selectedtable = 0;
    m_ptablelist = NULL;
}

//function sets the query
void 
MySQLResultDataEx::SetQuery(const wyChar* query, wyUInt32 len)
{
    SQLFormatter    formatter;
    wyString        tempString;
    //first clear them
    m_query.Clear();
    m_formattedquery.Clear();
    
    if(query)
    {
        //set the query and formatted query
        m_query.SetAs(query, len);
        tempString.SetAs(query, len);
        formatter.GetQueryWtOutComments(&tempString, &m_formattedquery);

        //format the query to be displayed in the ribbon
        FormatQueryByRemovingSpaceAndNewline(&m_formattedquery);

        //calculate the crc for view persistence
        CalculateQueryCRC();
    }
}

//function to get the query
wyString&
MySQLResultDataEx::GetQuery()
{
    return m_query;
}

//function calculates the crc for query for view persistence
void 
MySQLResultDataEx::CalculateQueryCRC()
{
	wyCrc		crc;
	wyString	tempstr;
		
    m_crcquery = 0;

    if(!m_formattedquery.GetLength())
    {
        return;
    }

    //trim the white spaces and calculate crc
	tempstr.SetAs(m_formattedquery);
	tempstr.RTrim();
	tempstr.LTrim();
	tempstr.ToLower();
    m_crcquery = crc.CrcFast((const unsigned char*)tempstr.GetString(), tempstr.GetLength());
}

//function handles the limit persistence
void 
MySQLResultDataEx::HandleLimitPersistence(wyBool isset)
{
    wyInt32 limit;

    //whether to retain the page value for each query?
    if(pGlobals->m_retainpagevalue == wyTrue)
    {
        //persist the limit
        limit = (m_islimit == wyTrue) ? m_limit : -1;
        HandleQueryPersistance((wyChar*)m_query.GetString(), &limit);
    }
}

//function handles the view persistence for the query
ViewType 
MySQLResultDataEx::HandleViewPersistence(wyBool isset)
{
    wyString    tempstr;
    ViewType    view;
		
    if(!m_formattedquery.GetLength())
    {
        return m_viewtype;
    }

    //we use crc as the the name of the table and db as empty
    //this way we reuse the same sqlite table created for table view persistence
    tempstr.Sprintf("%u", m_crcquery);
    view = m_viewtype;
    view = (ViewType)HandleTableViewPersistance("", tempstr.GetString(), m_viewtype, isset);

    //form view check
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

//destructor
MySQLResultDataEx::~MySQLResultDataEx()
{
    delete m_ptablelist;
}

//list element constructor
ResultTabTableElem::ResultTabTableElem(wyString& db, wyString& table)
{
    //initialize everything
    m_isorigdb = db.GetLength() ? wyTrue : wyFalse;
    m_pdb = m_ptable = NULL;
    m_pdb = new wyString(db.GetString(), db.GetLength() + 1);
    m_ptable = new wyString(table.GetString(), table.GetLength() + 1);
}

//destructor
ResultTabTableElem::~ResultTabTableElem()
{
    delete m_pdb;
    delete m_ptable;
}

//view constructor
ResultView::ResultView(MDIWindow *wnd, HWND hwndparent) : DataView(wnd, hwndparent, NULL)
{
    m_mydata = NULL;
}

//destuctor
ResultView::~ResultView()
{
}

//function to clear the table combo and repopulate
void
ResultView::RefreshTablesCombo()
{
    if(m_mydata && m_mydata->m_datares)
    {
        //build the table list if it is not there
        if(!m_mydata->m_ptablelist)
        {
            m_mydata->m_ptablelist = new List;
            BuildTableList();
        }
    }

    //refresh
    SetTablesCombo();
}

//external function to set the data
void
ResultView::SetData(MySQLDataEx* data)
{
    //save a copy for this class
	m_mydata = (MySQLResultDataEx*)data;

    //set the base class data, dont forget this
	DataView::SetData(data);

    //refresh the table comobo
    RefreshTablesCombo();
}

//function resets the data
MySQLDataEx* 
ResultView::ResetData(MySQLDataEx* pdata)
{
    MySQLResultDataEx*  ptemp = NULL;
    MySQLResultDataEx*  tempdata = (MySQLResultDataEx*)pdata;
    SortAndFilter*      psfnew;

    if(pdata)
    {
        //allocate a new data
        ptemp = new MySQLResultDataEx(pdata->m_pmdi, NULL);

        //clear the row array for the object
        delete ptemp->m_rowarray;
        ptemp->m_rowarray = NULL;

        //store the pointer to the allocated filter
        psfnew = ptemp->m_psortfilter;

        //copy the data
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

//function to change the tool button icon on exec start and stop
wyBool 
ResultView::ChangeToolButtonIcon(wyBool isexecstart, wyInt32 id)
{
    wyInt32 image;

    //call the base class version to make sure that it was handled in the base class
    if(DataView::ChangeToolButtonIcon(isexecstart, id) == wyTrue)
    {
        return wyTrue;
    }

    //try with the second toolbar
    if(SendMessage(m_hwndtoolbar2, TB_COMMANDTOINDEX, id, 0) == -1)
    {
        return wyFalse;
    }

    //on execution start get the stop icon
    if(isexecstart == wyTrue)
    {
        image = GetImageIndex(IMG_INDEX_STOP, GETIMG_IMG, m_hwndtoolbar, m_hwndtoolbar2, NULL);
    }
    //esle get the original icon
    else
    {
        image = GetImageIndex(id, GETIMG_COMMAND, m_hwndtoolbar2, NULL);
    }

    //update the icon
    if(image != -1)
    {
        SendMessage(m_hwndtoolbar2, TB_CHANGEBITMAP, (WPARAM)id, (LPARAM)image);
        InvalidateRect(m_hwndtoolbar2, NULL, TRUE);
        UpdateWindow(m_hwndtoolbar2);
        return wyTrue;
    }

    return wyFalse;    
}

//function reset the toolbar buttons to its original state
void 
ResultView::ResetToolBarButtons()
{
    //reset refresh toolbar
    if(m_hwndrefreshtool)
    {
        ResetButtons(m_hwndrefreshtool);
    }

    //first toolbar
    if(m_hwndtoolbar)
    {
        ResetButtons(m_hwndtoolbar);
    }

    //second toolbar
    if(m_hwndtoolbar2)
    {
        ResetButtons(m_hwndtoolbar2);
    }
}

//function creates the toolbars
void
ResultView::CreateToolBar()
{
    wyUInt32 style = WS_CHILD | CCS_NOPARENTALIGN | TBSTYLE_TOOLTIPS | WS_VISIBLE | CCS_NODIVIDER | TBSTYLE_FLAT;

    //create the first toolbar for export options and table combo
    m_hwndtoolbar = CreateWindowEx(WS_EX_CONTROLPARENT, TOOLBARCLASSNAME, NULL, style, 
                                   0, 0, 0, 0, m_hwndframe,
                                   (HMENU)IDC_TOOLBAR, (HINSTANCE)GetModuleHandle(0), NULL);

    //create second toolbar for DML and view buttons
	m_hwndtoolbar2 = CreateWindowEx(WS_EX_CONTROLPARENT, TOOLBARCLASSNAME, NULL, style, 
                                    0, 0, 0, 0, m_hwndframe,
                                    (HMENU)IDC_TOOLBAR2, (HINSTANCE)GetModuleHandle(0), NULL);

    //for refresh controls
    m_hwndtablecombo = CreateWindowEx(0, L"combobox", L"", WS_CHILD | CBS_DROPDOWNLIST | WS_VISIBLE, 
                                      0, 0, 0, 0, m_hwndtoolbar,
                                      (HMENU)IDC_TOOLCOMBO, (HINSTANCE)GetModuleHandle(0), NULL);
    SendMessage(m_hwndtablecombo, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
	
    //add buttons to the toolbar
	AddToolButtons();

    //size the toolbar
    SendMessage(m_hwndtoolbar, TB_AUTOSIZE, 0, 0);
    SendMessage(m_hwndtoolbar2, TB_AUTOSIZE, 0, 0);

    //remove auto stylings
    style = GetWindowLongPtr(m_hwndtoolbar, GWL_STYLE);
    SetWindowLongPtr(m_hwndtoolbar, GWL_STYLE, style | CCS_NORESIZE);
    style = GetWindowLongPtr(m_hwndtoolbar2, GWL_STYLE);
    SetWindowLongPtr(m_hwndtoolbar2, GWL_STYLE, style | CCS_NORESIZE);

    //subclass the first toolbar
    m_toolbarorigproc = (WNDPROC)SetWindowLongPtr(m_hwndtoolbar, GWLP_WNDPROC, (LONG_PTR)ToolBarWindowProc);
    SetWindowLongPtr(m_hwndtoolbar, GWLP_USERDATA, (LONG_PTR)this);
}

//toolbar subclass funtion to handle commands and drop down menu drawing
LRESULT CALLBACK 
ResultView::ToolBarWindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    ResultView*  prv = (ResultView*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch(message)
    {
        //handle table combo drop down
        case WM_COMMAND:
            if(HIWORD(wparam) == CBN_SELENDOK)
            {
                prv->OnTableComboChange();
                return 0;
            }
            break;

        case WM_MEASUREITEM:
            return wyTheme::MeasureMenuItem((LPMEASUREITEMSTRUCT)lparam, prv->m_htrackmenu);

	    case WM_DRAWITEM:		
            return wyTheme::DrawMenuItem((LPDRAWITEMSTRUCT)lparam);	
    }

    return CallWindowProc(prv->m_toolbarorigproc, hwnd, message, wparam, lparam);
}

//when the table combo selection changes
void            
ResultView::OnTableComboChange()
{
    wyInt32             i, ret;
    ResultTabTableElem* pelem;

    //get the selected item
    i = SendMessage(m_hwndtablecombo, CB_GETCURSEL, 0, 0);

    //if the new selection is same as old selection, we have nothing to do, so just return
    if(i == m_mydata->m_selectedtable)
    {
        return;
    }

    //apply the changes in grid
    CustomGrid_ApplyChanges(m_hwndgrid);

    //get the element associated with the item. for the first item, i.e (Read Only) the element will be NULL
    if((pelem = (ResultTabTableElem*)SendMessage(m_hwndtablecombo, CB_GETITEMDATA, i, 0)))
    {
        //precaution
        if(!pelem->m_ptable->GetLength())
        {
            return;
        }

        //if there was no DB name
        if(!pelem->m_pdb->GetLength())
        {
            //popup a message box
            ret = MessageBox(m_hwndgrid, _(L"SQLyog could not identify the database for the selected table. \r\n\r\nDo you want to use the active database context?"),
                pGlobals->m_appname.GetAsWideChar(), MB_YESNOCANCEL | MB_ICONQUESTION);

            //if the user want to continue with the current data base selected
            if(ret == IDYES)
            {
                //check we have a database selection. if not popup an error
                if(!m_wnd->m_database.GetLength())
                {
                    MessageBox(m_hwndgrid, _(L"No database selected. Please select a database and try again"), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);

                    //revert the selection
                    SendMessage(m_hwndtablecombo, CB_SETCURSEL, m_mydata->m_selectedtable, 0);
                    return;
                }

                //set the db name
                pelem->m_pdb->SetAs(m_wnd->m_database);
            }
            //else revert the selection
            else
            {
                SendMessage(m_hwndtablecombo, CB_SETCURSEL, m_mydata->m_selectedtable, 0);
                return;
            }
        }
    }

    //if we have an unsaved change in the grid
    if(m_mydata->m_modifiedrow >= 0)
    {
        //confirm the refresh
        SendThreadMessage(m_hwndframe, UM_CONFIRMREFRESH, 0, (LPARAM)&ret);

        //revert the selection if the user cancel refresh
        if(ret == IDCANCEL)
        {
            SendMessage(m_hwndtablecombo, CB_SETCURSEL, m_mydata->m_selectedtable, 0);
            return;
        }
        //insert/update the changes. once the thread for insert/update finishes, it will call the function given
        else if(ret == IDYES)
        {
            ApplyChanges(LA_CALLBACK, 0, (LPARAM)TableComboChangeCallback);
            return;
        }
    }

    //end the table combo change
    EndTableComboChange(TE_SUCCESS);
}

//callback function for thread action
wyInt32 CALLBACK 
ResultView::TableComboChangeCallback(LPEXEC_INFO lpexeinfo)
{
    ResultView* prv = (ResultView*)lpexeinfo->m_pdataview;
    
    //finish the change
    prv->EndTableComboChange(lpexeinfo->m_ret);    

    return 0;
}

//function to finish the table combo change
void 
ResultView::EndTableComboChange(ThreadExecStatus te)
{
    ResultTabTableElem* pelem;
    wyInt32             row, prevseltable, rowcount, temp,no_row;
    HWND                hwndfocus = GetActiveDispWindow();

    //if the operation was succesfull
    if(te == TE_SUCCESS)
    {
        //get the current selected row, we need this to set the selection later
        row = CustomGrid_GetCurSelRow(m_hwndgrid);

        //save the selected table index and get the new index
        prevseltable = m_mydata->m_selectedtable;
        m_mydata->m_selectedtable = SendMessage(m_hwndtablecombo, CB_GETCURSEL, 0, 0);

        //cancel all the changes and get the row count
        CancelChanges();
        rowcount = m_mydata->m_rowarray->GetLength();

        //get the element associated with the selection, if it is the first item, i.e (Read Only), this will be NULL
        if((pelem = (ResultTabTableElem*)SendMessage(m_hwndtablecombo, CB_GETITEMDATA, m_mydata->m_selectedtable, 0)))
        {
            //set the db and table
            SetDbTable(pelem->m_pdb->GetString(), pelem->m_ptable->GetString());

            //mark the thread as running, so that we can get the table details
            ThreadStopStatus(0);
            
            //get the table details, if it was not succesful
            if(GetTableDetails() != TE_SUCCESS)
            {
                //mark the thread as stopped
                ThreadStopStatus(1);
                
                //if it was not the original db, clear the db
                if(pelem->m_isorigdb == wyFalse)
                {
                    pelem->m_pdb->Clear();
                }

                //revert the selection
                m_mydata->m_selectedtable = prevseltable;
                SendMessage(m_hwndtablecombo, CB_SETCURSEL, prevseltable, 0);
                                
                //set the db and table
                if((pelem = (ResultTabTableElem*)SendMessage(m_hwndtablecombo, CB_GETITEMDATA, m_mydata->m_selectedtable, 0)))
                {
                    SetDbTable(pelem->m_pdb->GetString(), pelem->m_ptable->GetString());
                }
                else
                {
                    SetDbTable("", "");
                }

                //set the focus
                if(hwndfocus)
                {
                    SetFocus(hwndfocus);
                }

                return;
            }

			no_row = m_wnd->m_tunnel->mysql_num_fields(m_mydata->m_datares);

			if(!m_data->m_colvirtual)
			{
				m_data->m_colvirtual = (wyInt32*)calloc(no_row, sizeof(wyInt32));
				memset(m_data->m_colvirtual,-1,no_row * sizeof(wyInt32));
			}

			

            //we are here means, the operation was succesful

            //if it was not the original db
            if(pelem->m_isorigdb == wyFalse)
            {
                //update the item so that it shows the db name
                UpdateTableComboItem(m_mydata->m_selectedtable);

                //mark the db as original
                pelem->m_isorigdb = wyTrue;

                //set the selection
                SendMessage(m_hwndtablecombo, CB_SETCURSEL, m_mydata->m_selectedtable, 0);
            }

            //mark the thread as stopped
            ThreadStopStatus(1);

            //if there are rows and the last row was a new row
            if(rowcount && m_mydata->m_rowarray->GetRowExAt(rowcount - 1)->IsNewRow())
            {
                //delete the last row
                m_mydata->m_rowarray->Delete(rowcount - 1);
                CustomGrid_SetMaxRowCount(m_hwndgrid, rowcount - 1);
            }

            //add a new row
            AddNewRow();
        }
        //if the selection was (Read only)
        else
        {
            //check the last row is a new row
            if(rowcount && m_mydata->m_rowarray->GetRowExAt(rowcount - 1)->IsNewRow())
            {
                //delete the last row
                m_mydata->m_rowarray->Delete(rowcount - 1);
                CustomGrid_SetMaxRowCount(m_hwndgrid, rowcount - 1);
            }

            //set db and table
            SetDbTable("", "");
        }

        //update the columns to show FKs, enum/list columns
		if(m_wnd->m_tunnel->IsTunnel())
			CreateColumns(wyFalse);
		else
        CreateColumns(wyTrue);

        //set the selection
        row = row >= m_data->m_rowarray->GetLength() ? max(row - 1, 0) : max(row, 0);
        CustomGrid_SetCurSelRow(m_hwndgrid, row, wyFalse);
        
        //update the form 
        if(IsFormView() == wyTrue)
        {
#ifndef COMMUNITY
            m_formview->LoadForm(wyTrue);
#endif
        }
        else
        {
            RefreshActiveDispWindow();
        }

        //enable/disable DML buttons
        EnableDMLButtons(m_mydata->m_selectedtable && m_mydata->m_viewtype != TEXT ? wyTrue : wyFalse);
    }
    //end table change was called due to a failure
    else
    {
        //get the current element associated with the selection
        temp = SendMessage(m_hwndtablecombo, CB_GETCURSEL, 0, 0);
        pelem = (ResultTabTableElem*)SendMessage(m_hwndtablecombo, CB_GETITEMDATA, temp, 0);
        
        if(pelem && pelem->m_isorigdb == wyFalse)
        {
            pelem->m_pdb->Clear();
        }

        //revert the selection
        SendMessage(m_hwndtablecombo, CB_SETCURSEL, m_mydata->m_selectedtable, 0);
    }

    //set the focus
    if(hwndfocus)
    {
        SetFocus(hwndfocus);
    }
}

//function add tool buttons
void 
ResultView::AddToolButtons()
{
	INT			i = 0, j, size, k;
	HICON		hicon;
	TBBUTTON	tbb[30];

    wyInt32 command[] = {
        IDM_IMEX_EXPORTDATA, 
        IDM_DATATOCLIPBOARD,
        IDM_SEPARATOR
    };

    wyInt32 command2[] = {
        ID_RESULT_INSERT, IDM_DUPLICATE_ROW, ID_RESULT_SAVE,
        ID_RESULT_DELETE, ID_RESULT_CANCEL,
        IDM_SEPARATOR, ID_VIEW_GRIDVIEW, ID_VIEW_FORMVIEW,
        ID_VIEW_TEXTVIEW
    };

	wyUInt32 states[][2] = {
        {TBSTATE_ENABLED, TBSTYLE_BUTTON},  
        {TBSTATE_ENABLED, BTNS_WHOLEDROPDOWN}, 
        {TBSTATE_ENABLED, TBSTYLE_SEP}							  
    };

    wyUInt32 states2[][2] = {
        {TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
        {TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON},  
        {TBSTATE_ENABLED, TBSTYLE_SEP}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
        {TBSTATE_ENABLED, TBSTYLE_BUTTON}, {TBSTATE_ENABLED, TBSTYLE_BUTTON}, 							  
    };

	wyInt32 imgres[] = {
        IDI_TABLEEXPORT, 
        IDI_EXPORTRESULTCSV,
        IDI_USERS
    };

    wyInt32 imgres2[] = {  
        IDI_RESULTINSERT, IDI_DUPLICATEROW, IDI_SAVE,
        IDI_RESULTDELETE, IDI_RESULTCANCEL, 
        IDI_USERS, IDI_DATAGRID,
        IDI_FORMICON, IDI_TEXTVIEW
    };

	m_himglist = ImageList_Create(ICON_SIZE, ICON_SIZE, ILC_COLOR32 | ILC_MASK, 1, 0);
    SendMessage(m_hwndtoolbar, TB_SETIMAGELIST, 0, (LPARAM)m_himglist);
	SendMessage(m_hwndtoolbar, TB_SETEXTENDEDSTYLE, 0 , (LPARAM)TBSTYLE_EX_DRAWDDARROWS);
    SendMessage(m_hwndtoolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(m_hwndtoolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    SendMessage(m_hwndtoolbar2, TB_SETIMAGELIST, 0, (LPARAM)m_himglist);
	SendMessage(m_hwndtoolbar2, TB_SETEXTENDEDSTYLE, 0 , (LPARAM)TBSTYLE_EX_DRAWDDARROWS);
    SendMessage(m_hwndtoolbar2, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(m_hwndtoolbar2, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    
	size = sizeof(command)/sizeof(command[0]);

	for(j = 0, size = sizeof(command)/sizeof(command[0]); j < size; j++)	
	{
		hicon = (HICON)LoadImage(pGlobals->m_hinstance, MAKEINTRESOURCE(imgres[j]), IMAGE_ICON, ICON_SIZE, ICON_SIZE, LR_DEFAULTCOLOR);
		i = ImageList_AddIcon(m_himglist, hicon);
		DestroyIcon(hicon);
		
		memset(&tbb[j], 0, sizeof(TBBUTTON));
        tbb[j].iBitmap = MAKELONG(i, 0);
		tbb[j].idCommand = command[j];
		tbb[j].fsState = (UCHAR)states[j][0];
		tbb[j].fsStyle = (UCHAR)states[j][1];
	}  

    SendMessage(m_hwndtoolbar, TB_ADDBUTTONS, (WPARAM)size,(LPARAM)&tbb);

    for(j = 0, k = 0, size = sizeof(command2)/sizeof(command2[0]); j < size; j++)	
	{
#ifndef COMMUNITY
        if(command2[j] == ID_VIEW_FORMVIEW && !m_formview)
        {
            continue;
        }
#endif
        hicon = (HICON)LoadImage(pGlobals->m_hinstance, MAKEINTRESOURCE(imgres2[j]), IMAGE_ICON, ICON_SIZE, ICON_SIZE, LR_DEFAULTCOLOR);
		i = ImageList_AddIcon(m_himglist, hicon);
		DestroyIcon(hicon);
		
		memset(&tbb[k], 0, sizeof(TBBUTTON));
        tbb[k].iBitmap = MAKELONG(i, 0);
		tbb[k].idCommand = command2[j];
		tbb[k].fsState = (UCHAR)states2[j][0];
		tbb[k].fsStyle = (UCHAR)states2[j][1];
        ++k;
	}  

    SendMessage(m_hwndtoolbar2, TB_ADDBUTTONS, (WPARAM)k, (LPARAM)&tbb);

    //add extra images
    AddExtraImages(m_himglist);

    //set the image index as the lparam for toolbars
    SetImageIndexes(m_hwndtoolbar);
    SetImageIndexes(m_hwndtoolbar2);
}

//when a refresh operation finished
void 
ResultView::OnQueryFinish(LPEXEC_INFO pexeinfo)
{
    wyInt32             seltable;
    wyString            db, table;
    ResultTabTableElem* pelem;
    LRESULT             ret;

    //get the db and table
    db.SetAs(m_data->m_db);
    table.SetAs(m_data->m_table);

    //call the base class version
    DataView::OnQueryFinish(pexeinfo);
    
    //if the operation was succesful
    if(pexeinfo->m_ret == TE_SUCCESS)
    {
        //if this was not called because of a client side sort/filtering
        if(pexeinfo->m_la != LA_SORT && pexeinfo->m_la != LA_FILTER)
        {
            //reset the db and table
            SetDbTable("", "");

            //get the current selection from the table combo
            seltable = SendMessage(m_hwndtablecombo, CB_GETCURSEL, 0, 0);

            //refresh the table combo
            RefreshTablesCombo();
        
            //retain the table combo selection and set table and db
            if((ret = SendMessage(m_hwndtablecombo, CB_GETITEMDATA, seltable, 0)) != CB_ERR && 
                (pelem = (ResultTabTableElem*)ret) && !pelem->m_pdb->GetLength() && !pelem->m_ptable->CompareI(table))
            {
                pelem->m_pdb->SetAs(db);
            }

            //select the item
            SelectTableComboItem(seltable);
        }
    }
}

//function to refresh the view
wyInt32 
ResultView::RefreshDataView()
{
    wyString        query;
    wyInt32         ret;
    MySQLDataEx*    pdata;
    
    //set the query
    query.SetAs(m_mydata->GetQuery());

    //add limits if any
    GetLimits(query);

    //execute the query
    m_mydata->m_datares = ExecuteQuery(query);

    //if thread was stopped
	if(ThreadStopStatus())
	{	
        return TE_STOPPED;
	}

    //show any errors
	if(!m_mydata->m_datares)
	{
        return HandleErrors(query);
	}

    //allocate the row array
    if((ret = AllocateRowsExArray()) != TE_SUCCESS)
    {
        pdata = ResetData(m_data);
        delete pdata;
        return ret;
    }

    return TE_SUCCESS;
}

//function to resize and position the controls
void
ResultView::Resize()
{
    RECT	    rcparent, rctemp, tbrect, cbrect;
	wyInt32		hpos, vpos, width, height, toolwidth, toolheight;
    wyInt32     padding = 6, itemcount;
    TBBUTTON    tbinfo = {0};

    GetClientRect(m_hwndparent, &rcparent);
	
    hpos	= rcparent.left;
    vpos	= rcparent.top = CustomTab_GetTabHeight(m_hwndparent);
	width	= rcparent.right - rcparent.left;
	height	= rcparent.bottom - rcparent.top;
	
    
	SetWindowPos(m_hwndframe, HWND_BOTTOM, hpos, vpos, width, height,  SWP_NOZORDER);
    GetClientRect(m_hwndframe, &rcparent);

    GetWindowRect(m_hwndtablecombo, &cbrect);
    ResizeRefreshTool((cbrect.bottom - cbrect.top) + 4, &tbrect);
    toolheight = max((cbrect.bottom - cbrect.top) + 4, tbrect.bottom - tbrect.top);
        
    itemcount = SendMessage(m_hwndtoolbar, TB_BUTTONCOUNT, 0, 0) - 1;
    memset(&rctemp, 0, sizeof(RECT));

    if(itemcount >= 0)
    {
        SendMessage(m_hwndtoolbar, 
            TB_GETBUTTON, 
            itemcount,
            (LPARAM)&tbinfo);

        SendMessage(m_hwndtoolbar, TB_GETRECT, tbinfo.idCommand, (LPARAM)&rctemp);
    }

    toolwidth = rctemp.right + padding + 100 + padding;
    SetWindowPos(m_hwndtablecombo, NULL, rctemp.right + padding, (toolheight / 2) - ((cbrect.bottom - cbrect.top) / 2) - 2 , 100, cbrect.bottom - cbrect.top, SWP_NOZORDER);
    SetWindowPos(m_hwndtoolbar, m_hwndframe, 0, DATAVIEW_TOPPADDING, toolwidth, toolheight, SWP_NOZORDER);
    
    itemcount = SendMessage(m_hwndtoolbar2, TB_BUTTONCOUNT, 0, 0) - 1;
    memset(&rctemp, 0, sizeof(RECT));

    if(itemcount >= 0)
    {
        SendMessage(m_hwndtoolbar2, 
            TB_GETBUTTON, 
            itemcount,
            (LPARAM)&tbinfo);

        SendMessage(m_hwndtoolbar2, TB_GETRECT, tbinfo.idCommand, (LPARAM)&rctemp);
        rctemp.right += padding;
    }

    if(toolwidth + rctemp.right + tbrect.right < rcparent.right)
    {
        rctemp.right = rcparent.right - (toolwidth + tbrect.right);
    }
        
    SetWindowPos(m_hwndpadding, NULL, 0, 0, rcparent.right - 0, DATAVIEW_TOPPADDING, SWP_NOZORDER);
    SetWindowPos(m_hwndtoolbar2, NULL, toolwidth, DATAVIEW_TOPPADDING, rctemp.right, toolheight, SWP_NOZORDER);
    hpos = toolwidth + rctemp.right;
    SetWindowPos(m_hwndrefreshtool, NULL, hpos, DATAVIEW_TOPPADDING, rcparent.right - hpos, toolheight, SWP_NOZORDER);
    
    rcparent.top += toolheight + DATAVIEW_TOPPADDING;
    ResizeControls(rcparent);

    InvalidateRect(m_hwndframe, NULL, TRUE);
    UpdateWindow(m_hwndframe);
}

//function to draw the bottom ribbon showing the query executed
void 
ResultView::DrawTableInfoRibbon(HDC hdc, RECT drawrect)
{
    HFONT       hfont = GetStaticTextFont(m_hwndshowtableinfo);
    wyString    text;

    hfont = (HFONT)SelectObject(hdc, hfont);

    if(m_mydata)
    {
        //set the formatted query
        text.SetAs(m_mydata->m_formattedquery);
        text.RTrim();
        
        //add limits if any
        if(m_data->m_islimit == wyTrue)
        {
            text.AddSprintf(" LIMIT %d, %d", m_data->m_startrow, m_data->m_limit); 
        }

        //draw the text
        DrawText(hdc, text.GetAsWideChar(), -1, &drawrect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    hfont = (HFONT)SelectObject(hdc, hfont);
    DeleteFont(hfont);
}


//function to enable tool button
wyBool 
ResultView::EnableToolButton(wyBool isenable, wyInt32 id)
{
    wyInt32 i, count;
    TBBUTTON tbb;
    wyBool  ret = wyFalse;

    if(id == -1)
    {
        EnableWindow(m_hwndtablecombo, isenable);
    }

    if(DataView::EnableToolButton(isenable, id) == wyTrue)
    {
        return wyTrue;
    }

    if(id == -1)
    {
        count = SendMessage(m_hwndtoolbar2, TB_BUTTONCOUNT, 0, 0);

        for(i = 0; i < count; ++i)
        {
            SendMessage(m_hwndtoolbar2, TB_GETBUTTON, i, (LPARAM)&tbb);
            SendMessage(m_hwndtoolbar2, TB_SETSTATE, (WPARAM)tbb.idCommand, isenable == wyTrue ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE);
        }
    }
    else
    {
        if(SendMessage(m_hwndtoolbar2, TB_COMMANDTOINDEX, id, 0) != -1)
        {
            SendMessage(m_hwndtoolbar2, TB_SETSTATE, (WPARAM)id, isenable == wyTrue ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE);
            ret = wyTrue;
        }
    }

    return ret;
}

//get the index of a column from the field result
wyInt32         
ResultView::GetFieldResultIndexOfColumn(wyInt32 col)
{
    MYSQL_ROW   myrow;
    wyInt32     i, index = -1;
    wyString    column, tempstr;

    //if no field res, return -1
    if(!m_data->m_fieldres)
    {
        return -1;
    }

    //get the original column name
    GetColumnName(column, col);

    m_wnd->m_tunnel->mysql_data_seek(m_data->m_fieldres, 0);

    //loop through the field res
    for(i = 0; (myrow = m_wnd->m_tunnel->mysql_fetch_row(m_data->m_fieldres)); ++i)
    {
        tempstr.SetAs(myrow[0], m_wnd->m_ismysql41);

        //compare the column name
        if(!tempstr.CompareI(column))
        {
            index = i;
            break;
        }
    }

    m_wnd->m_tunnel->mysql_data_seek(m_data->m_fieldres, 0);
    return index;
}

//function to build the list of tables in the result
wyBool 
ResultView::BuildTableList()
{
    wyString            db, table;
    wyInt32             i, count, temp;
    ResultTabTableElem* pte;
    wyBool              ismultidb = wyFalse;

    count = m_mydata->m_datares->field_count;

    //loop throught the fields
    for(i = 0; i < count; ++i)
    {
        db.Clear();

        //set the db name
        if(m_data->m_datares->fields[i].db)
        {
            db.SetAs(m_data->m_datares->fields[i].db, m_wnd->m_ismysql41);
        }

        //get the table name
        if(GetTableName(table, i))
        {
            //loop through the list to check whether the table was already added
            for(pte = (ResultTabTableElem*)m_mydata->m_ptablelist->GetFirst();
                pte;
                pte = (ResultTabTableElem*)pte->m_next)
            {
                //check for the table
                if(!(temp = pte->m_pdb->Compare(db)) && !pte->m_ptable->Compare(table))
                {
                    break;
                }

                if(ismultidb == wyFalse && temp)
                {
                    ismultidb = wyTrue;
                }
            }

            //if the table element was not found, we will add a new one
            if(!pte)
            {
                m_mydata->m_ptablelist->Insert(new ResultTabTableElem(db, table));
            }
        }
    }

    return ismultidb;
}

//function to update an item in the table combo box
void
ResultView::UpdateTableComboItem(wyInt32 index)
{
    wyInt32             i, dropdownwidth = 0, count;
    HFONT               hfont;
    HDC                 hdc;
    ResultTabTableElem* pelem;
    wyString            tempstr;
    RECT                rect;

    count = SendMessage(m_hwndtablecombo, CB_GETCOUNT, 0, 0);
    hfont = (HFONT)SendMessage(m_hwndtablecombo, WM_GETFONT, 0, 0);
    //get dc and select the font
    hdc = GetDC(m_hwndtablecombo);
    hfont = SelectFont(hdc, hfont);
    
    //loop through the items
    for(i = 0; i < count; ++i)
    {
        //get the elements associated with the item
        pelem = (ResultTabTableElem*)SendMessage(m_hwndtablecombo, CB_GETITEMDATA, i, 0);
        //if it is valid
        if(pelem)
        {
            //for the string to be displayed
            if(pelem->m_pdb->GetLength())
            {
                tempstr.Sprintf("`%s`.`%s`", pelem->m_pdb->GetString(), pelem->m_ptable->GetString());
            }
            else
            {
                tempstr.Sprintf("`%s`", pelem->m_ptable->GetString());
            }
        }
        else
        {
            tempstr.SetAs("(Read Only)");
        }

        //we need to calculate the width of the table combo dropdown
        memset(&rect, 0, sizeof(rect));
        DrawText(hdc, tempstr.GetAsWideChar(), -1, &rect, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);

        //find the maximum width
        if(rect.right > dropdownwidth)
        {
            dropdownwidth = rect.right;
        }

        //if this is the index we want to update
        if(index == i)
        {
            //first delete the item
            SendMessage(m_hwndtablecombo, CB_DELETESTRING, index, 0);

            //now insert the item 
            SendMessage(m_hwndtablecombo, CB_INSERTSTRING, index, (LPARAM)tempstr.GetAsWideChar());

            //set the item data
            SendMessage(m_hwndtablecombo, CB_SETITEMDATA, index, (LPARAM)pelem);
        }
    }

    hfont = SelectFont(hdc, hfont);
    ReleaseDC(m_hwndtablecombo, hdc);

    //set the drop width
    SendMessage(m_hwndtablecombo, CB_SETDROPPEDWIDTH, dropdownwidth + 20, 0);
}

//function to add the combo box items
void 
ResultView::SetTablesCombo()
{
    wyInt32             i, dropdownwidth = 0;
    ResultTabTableElem* pte;
    wyString            text;
    HDC                 hdc;
    HFONT               hfont;
    RECT                rect;

    //get the font used to draw the items, need this to calculate the drop down width
    hfont = (HFONT)SendMessage(m_hwndtablecombo, WM_GETFONT, 0, 0);

    //delete all items from combo box
    SendMessage(m_hwndtablecombo, CB_RESETCONTENT, 0, 0);

    //get dc and select the font
    hdc = GetDC(m_hwndtablecombo);
    hfont = SelectFont(hdc, hfont);

    if(m_mydata && m_mydata->m_ptablelist)
    {
        //loop through the table list
        for(pte = (ResultTabTableElem*)m_mydata->m_ptablelist->GetFirst();
            pte;
            pte = (ResultTabTableElem*)pte->m_next)
        {
            //form the string
            if(pte->m_pdb->GetLength())
            {
                text.Sprintf("`%s`.`%s`", pte->m_pdb->GetString(), pte->m_ptable->GetString());
            }
            else
            {
                text.Sprintf("`%s`", pte->m_ptable->GetString());
            }

            //calculate the width
            memset(&rect, 0, sizeof(rect));
            DrawText(hdc, text.GetAsWideChar(), -1, &rect, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);

            //find the maximum width
            if(rect.right > dropdownwidth)
            {
                dropdownwidth = rect.right;
            }

            //add string
            i = SendMessage(m_hwndtablecombo, CB_ADDSTRING, 0, (LPARAM)text.GetAsWideChar());

            //set item data
            SendMessage(m_hwndtablecombo, CB_SETITEMDATA, i, (LPARAM)pte);
        }
    }

    //calculate the width for the first item
    memset(&rect, 0, sizeof(rect));
    DrawText(hdc, _(L"(Read Only)"), -1, &rect, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);

    if(rect.right > dropdownwidth)
    {
        dropdownwidth = rect.right;
    }
#ifndef COMMUNITY
	if(GetActiveWin()->m_conninfo.m_isreadonly == wyTrue)
	{
		 SendMessage(m_hwndtablecombo, CB_RESETCONTENT, 0, 0);
	}
#endif
    //insert (read-only) in the begining
    SendMessage(m_hwndtablecombo, CB_INSERTSTRING, 0, (LPARAM)_(L"(Read Only)"));
    SendMessage(m_hwndtablecombo, CB_SETITEMDATA, 0, NULL);

    hfont = SelectFont(hdc, hfont);
    ReleaseDC(m_hwndtablecombo, hdc);

    //set the current selection
    SendMessage(m_hwndtablecombo, CB_SETCURSEL, m_mydata ? m_mydata->m_selectedtable : 0, 0);
    SendMessage(m_hwndtablecombo, CB_SETDROPPEDWIDTH, dropdownwidth + 20, 0);
}

//function to enab le the limit check box
wyBool 
ResultView::EnableLimitCheckbox()
{
    wyBool enable;

    //if we have data and the query has a limit clause added explicitly, enable it
    enable = (m_mydata && m_mydata->m_isquerylimited == wyTrue && m_mydata->m_ispartofmultiresult == wyFalse) ? wyTrue : wyFalse;
    EnableWindow(m_hwndislimit, enable == wyTrue ? TRUE : FALSE);

    //disable the refresh button if the result is part of multiple result set
    if(m_mydata && m_mydata->m_ispartofmultiresult == wyTrue)
    {
        SendMessage(m_hwndrefreshtool, TB_SETSTATE, (WPARAM)IDC_REFRESH, TBSTATE_INDETERMINATE);
    }

    //return the state
    return enable;
}

//function to get the toobar contianing a button
HWND 
ResultView::GetToolBarFromID(wyInt32 id)
{
    //check for the first toolbar
    if(SendMessage(m_hwndtoolbar, TB_COMMANDTOINDEX, id, 0) != -1)
    {
        return m_hwndtoolbar;
    }
    //check in the refresh toolbar
    else if(SendMessage(m_hwndrefreshtool, TB_COMMANDTOINDEX, id, 0) != -1)
    {
        return m_hwndrefreshtool;
    }
    //check in the second toolbar
    else if(SendMessage(m_hwndtoolbar2, TB_COMMANDTOINDEX, id, 0) != -1)
    {
        return m_hwndtoolbar2;
    }

    return NULL;
}

//function to enable tool buttons
void 
ResultView::EnableToolButtons()
{
    wyBool  isenable = wyFalse;
    
    //call the base class version
    DataView::EnableToolButtons();

    //enable the table combo if there is valid data and the current view is not text view
    if(m_mydata && m_mydata->m_datares && m_viewtype != TEXT)
    {
        EnableWindow(m_hwndtablecombo, wyTrue);
        isenable = m_mydata->m_selectedtable ? wyTrue : wyFalse;
    }
    else
    {
        EnableWindow(m_hwndtablecombo, wyFalse);
    }

    //enable the dml buttons
    EnableDMLButtons(isenable);
}

//function to enable DML buttons
void 
ResultView::EnableDMLButtons(wyBool isenable)
{
#ifndef COMMUNITY
	if(m_wnd->m_conninfo.m_isreadonly == wyTrue)
		return;
#endif
    EnableToolButton(isenable, ID_RESULT_INSERT);
    EnableToolButton(isenable, ID_RESULT_DELETE);
    EnableToolButton(ErrorFlagState() != EFS_MODIFIED ? wyFalse : isenable, ID_RESULT_SAVE);
    EnableToolButton(ErrorFlagState() != EFS_MODIFIED ? wyFalse : isenable, ID_RESULT_CANCEL);
    EnableToolButton(IsCurrRowDuplicatable() && isenable ? wyTrue : wyFalse, IDM_DUPLICATE_ROW);
}

//helper function to select an item in table combo
void 
ResultView::SelectTableComboItem(wyInt32 i)
{
    //validate the index and set the selection
    if(i >= 0 && i < SendMessage(m_hwndtablecombo, CB_GETCOUNT, 0, 0))
    {
        SendMessage(m_hwndtablecombo, CB_SETCURSEL, i, 0);
    }

    //update the selection
    OnTableComboChange();
}

//show help
void 
ResultView::ShowHelpFile()
{
    ShowHelp("http://sqlyogkb.webyog.com/article/65-result-tab");
}

//function to get auto incrment column index
wyInt32 
ResultView::GetAutoIncrIndex()
{
	wyInt32		colindex = -1, tempcolindex = 0, i;
	MYSQL_ROW	myrow;
	wyInt32		extraval;
    wyString	colname, tempcolname;

    //if no field res
    if(!m_mydata->m_fieldres || !m_mydata->m_datares || 
        !m_mydata->m_db.GetLength() || !m_mydata->m_table.GetLength())
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
            colindex = tempcolindex;
            break;
        }
			
        tempcolindex++;
	}
    
    //if we find a valid auto_inc column
    if(colindex != -1)
    {
        //set the column name
        tempcolname.SetAs(myrow[0], m_wnd->m_ismysql41);

        //now loop through the fields in the result and check whether the column is used in the result
        for(i = 0; i < m_mydata->m_datares->field_count; ++i)
        {
            //make sure the column is not read-only
            if(IsColumnReadOnly(i) == wyTrue)
            {
                continue;
            }

            //get the column name for the current column in the result set
            colname.Clear();
            GetColumnName(colname, i);

            //compare the column name
            if(!tempcolname.Compare(colname))
            {
                return i;
            }
        }
    }

    //we are here means, there are no auto-inc column used in the result. return -1 to indicate that we dont have an auto-inc column 
	return -1;
}

//wrapper function to exprot data
void
ResultView::ExportData()
{
    CExportResultSet    exportresult;
    wyBool              isenablesqlexport;

    isenablesqlexport = (m_mydata->m_ptablelist && m_mydata->m_ptablelist->GetCount() == 1) ? wyTrue : wyFalse;

    exportresult.Export(m_hwndframe, m_data, wyFalse, isenablesqlexport);
}

