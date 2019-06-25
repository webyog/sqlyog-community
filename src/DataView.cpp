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

#include "CustGrid.h"
#include "FrameWindow.h"
#include "DataView.h"
#include "OtherDialogs.h"
#include "FKDropDown.h"
#include "CalendarCtrl.h"
#ifndef COMMUNITY
#include "FormView.h"
#endif
#include "BlobMgmt.h"
#include "EditorFont.h"
#include "ClientMySQLWrapper.h"
#include "jsoncpp.h"

#define	DEFAULT_FIELD_SIZE  2
#define	CHARSET_NUMBER      63
#define MAXSIZE             1600000

extern PGLOBALS pGlobals;

//constructor for object represeting data
MySQLDataEx::MySQLDataEx(MDIWindow* pmdi)
{
    m_pmdi = pmdi;
    m_limit = 0;
    m_startrow = 0;
    m_islimit = wyTrue;
    m_viewtype = GRID;
    m_lastview = -1;
    m_selrow = 0;
	m_selcol = 0;
    m_initcol = 0;
    m_initrow = 0;
    m_psortfilter = NULL;
    m_rowarray = NULL;
    memset(&m_textviewscrollpos, 0, sizeof(m_textviewscrollpos));
    m_textviewselstartpos = 0;
    m_textviewselendpos = 0;
    m_isrowarrayinitialized = wyFalse;

    //initialize modifyiable members
	Initialize();
}

//destructor
MySQLDataEx::~MySQLDataEx()
{
    Free();
}

//function to initialize the modifyable members of data class
void
MySQLDataEx::Initialize()
{
    //we have to allocate a new array if a it is not allocated already
    if(!m_rowarray)
    {
        m_rowarray = new MySQLRowExArray();
    }

    //initialize other members
	m_datares = NULL;
	m_keyres = NULL;
	m_fieldres = NULL;
	m_colvirtual = NULL;
	m_oldrow = NULL;	
	m_modifiedrow = -1;
	m_checkcount = 0;
    m_warningres = NULL;
    m_autoinccol = -1;
    m_pfkinfo = NULL;
    m_fkcount = -1;
    m_createtablestmt.Clear();
    
    //initialize sort and filter
    if(m_psortfilter)
    {
        m_psortfilter->Initialize();
    }
}

//function to handle limit persistence, the base class version does nothing
void 
MySQLDataEx::HandleLimitPersistence(wyBool isset)
{
}

//function to handle view persistence
ViewType 
MySQLDataEx::HandleViewPersistence(wyBool isset)
{
    return GRID;
}

//free any resources allocated
void
MySQLDataEx::FreeAllocatedResources()
{
    /*caution***************************
    you should delete SortAndFilter object before deleting MySQLRowExArray used to display the rows, to avoid memory leak
    ***********************************/

    //dekete sort and filter. 
    delete m_psortfilter;    
    m_psortfilter = NULL;

    //delete row array
    delete m_rowarray;
    m_rowarray = NULL;

    //delete any old row generated while updating a row
    if(m_oldrow)
    {
        delete m_oldrow;
        m_oldrow = NULL;
    }

	if(m_colvirtual)
    {
        delete m_colvirtual;
        m_colvirtual = NULL;
    }

    //free warnings
    if(m_warningres)
    {
        m_pmdi->m_tunnel->mysql_free_result(m_warningres);
        m_warningres = NULL;
    }

    //free FK list
    if(m_pfkinfo)
    {
		FKDropDown::DeleteFKList(&m_pfkinfo, &m_fkcount);
    }
}

//free all the resources associated with the data, typically this is called from the d
void
MySQLDataEx::Free()
{
    //free any allocated resources
    FreeAllocatedResources();

    //free thre result set
    if(m_datares)
    {
	    m_pmdi->m_tunnel->mysql_free_result(m_datares);
    }

    //free field result
    if(m_fieldres)
    {
	    m_pmdi->m_tunnel->mysql_free_result(m_fieldres);
    }


    //free key result
    if(m_keyres)
    {
	    m_pmdi->m_tunnel->mysql_free_result(m_keyres);
    }
}

//function to get the number of saved row count
wyInt32 
MySQLDataEx::GetSavedRowCount()
{
    wyInt32     rowcount = 0;
    MySQLRowEx* myrowex;

    //if row array is present
    if(m_rowarray)
    {
        //total number of rows in the array
        rowcount = m_rowarray->GetLength();

        //deduct 1, if the last row is a new row
        rowcount -= ((myrowex = m_rowarray->GetRowExAt(rowcount - 1)) && myrowex->IsNewRow()) ? 1 : 0;

        //deduct 1, if the second last row is a new row
        rowcount -= ((myrowex = m_rowarray->GetRowExAt(rowcount - 2)) && myrowex->IsNewRow()) ? 1 : 0;

        //deduct 1, if the modified row is a new row
        rowcount -= ((myrowex = m_rowarray->GetRowExAt(m_modifiedrow)) && myrowex->IsNewRow()) ? 1 : 0;
    }

    return rowcount;
}

//DataView constructor
DataView::DataView(MDIWindow *wnd, HWND hwndparent, IQueryBuilder* querybuilder)
{
    //initialize the members
    InitializeCriticalSection(&m_cs);
    m_viewtype = GRID;
    m_isbuffereddrawing = wyFalse;

#ifndef COMMUNITY
    m_formview = NULL;
#endif
    m_hwndpadding = NULL;
    m_hwndrefreshtool = NULL;
    m_hwndtoolbar = NULL;
    m_hwndfirstrow = NULL;
    m_hwndfirstrowlabel = NULL;
    m_hwndframe = NULL;
    m_hwndgrid = NULL;
    m_hwndislimit = NULL;
    m_hwndlimit = NULL;
    m_hwndlimitlabel = NULL;
    m_hwndmodified = NULL;
    m_hwndnext = NULL;
    m_hwndprev = NULL;
    m_hwndshowtableinfo = NULL;
    m_hwndtext = NULL;
    m_hwndwarning = NULL;
    m_hwndcal = NULL;
    m_hwndparent = hwndparent;
    m_hrefreshtoolimglist = NULL;
    m_iconnext = NULL;
    m_iconprevious = NULL;
    m_himglist = NULL;
    m_hrefreshtoolimglist = NULL;
    m_warningwndproc = NULL;
    m_refreshtoolproc = NULL;
    m_gridwndproc = DataView::GridWndProc;
	m_wnd = wnd;
	m_markicon = (HICON)LoadImage(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_MANREL_16), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    m_modifiedfont = NULL;
    m_arg = m_wnd->GetKeywords(&m_argcount);
    m_stop = 0;
    m_data = NULL;
    m_extraimages[IMG_INDEX_STOP] = IDI_CLOSETAB;
    m_extraimages[IMG_INDEX_RESET_FILTER] = IDI_RESETFILTER;
    m_htrackmenu = NULL;
    m_findreplace = NULL;
	m_querybuilder = querybuilder;

    ResetView();
}

//destructor
DataView::~DataView()
{
    wyInt32 i;

    //destroy the frame, and all its child windows will be destroyed automatically
    if(m_hwndframe)
    {
        DestroyWindow(m_hwndframe);
    }

    //free the special functions and keywords list
    if(m_arg)
    {
        for(i = 1; i < m_argcount; i++)
        {
            if(m_arg[i])
            {
                free(m_arg[i]);
            }

            m_arg[i] = NULL;
        }

        free(m_arg);
        m_arg = NULL;
    }

    //delete the font for warning
    if(m_modifiedfont)
    {
        DeleteObject(m_modifiedfont);
    }

    //free icon used for FK columns
    if(m_markicon)
    {
        DestroyIcon(m_markicon);
    }

    //delete formview
#ifndef COMMUNITY
    delete m_formview;
#endif

    //free image list 
    if(m_himglist)
    {
        ImageList_Destroy(m_himglist);
    }

    //free refreshbar imagelist
    if(m_hrefreshtoolimglist)
    {
        ImageList_Destroy(m_hrefreshtoolimglist);
    }

    //free button icons
    if(m_iconnext)
    {
        DestroyIcon(m_iconnext);
    }

    if(m_iconprevious)
    {
        DestroyIcon(m_iconprevious);
    }

    //free critical section
    DeleteCriticalSection(&m_cs);
}

//show help file. this is a dummy function and does nothing. derived class should implement this
void 
DataView::ShowHelpFile()
{
}

//function to create the windows 
void
DataView::Create()
{
    //create the outer frame, parent to all other windows
	CreateFrame();
	
    //create grid view 
    CreateGrid();

    //create text view
	CreateText();

    //create form view
	CreateForm();

    //create toolbar, this is a virtual function so that derived class can create additional toolbars
	CreateToolBar();

    //create the refresh bar
    CreateRefreshBar();

    //create the padding window to give some space on top
    CreatePaddingWindow();

    //the warning windows which shows data modified or any warning occured
    CreateWarnings();
    
    //disable all buttons
    EnableToolButton(wyFalse, -1);
}

//function to get the toolbar handle contaning the button 
HWND 
DataView::GetToolBarFromID(wyInt32 id)
{
    //check whether we have a valid index with the toolbars the class knows about
    if(SendMessage(m_hwndtoolbar, TB_COMMANDTOINDEX, id, 0) != -1)
    {
        return m_hwndtoolbar;
    }

    if(SendMessage(m_hwndrefreshtool, TB_COMMANDTOINDEX, id, 0) != -1)
    {
        return m_hwndrefreshtool;
    }

    return NULL;
}

//enable the limit check box
wyBool 
DataView::EnableLimitCheckbox()
{
    wyBool enable = m_data ? wyTrue : wyFalse;

    EnableWindow(m_hwndislimit, enable == wyTrue ? TRUE : FALSE);
    return enable;
}

//WM_COMMAND handler
void
DataView::OnWMCommand(WPARAM wparam, LPARAM lparam)
{	
    wyBool          ret = wyFalse;
    QueryPreview*   preview;
    wyString        query;
    EXEC_INFO       exeinfo = {TA_NONE, LA_NONE, 0, 0, TE_SUCCESS, this};
    wyInt32         rowcount, i;

    //check for identifier
    switch(LOWORD(wparam))
	{
        //add a new row
		case ID_RESULT_INSERT:
            CustomGrid_ApplyChanges(m_hwndgrid);

            //we will add a new row if and only if there is no new row at the end
            if(!(rowcount = m_data->m_rowarray->GetLength()) || m_data->m_rowarray->GetRowExAt(rowcount - 1)->IsNewRow() == wyFalse)
            {
                //add a new row and paint it
                AddNewRow(); 
                RefreshActiveDispWindow();
            }

            //finally set the selection to first column in the last row, which is ofcourse is the new row
            CustomGrid_SetCurSelection(m_hwndgrid, rowcount - 1, 0);
            CustomGrid_EnsureVisible(m_hwndgrid, rowcount - 1, 0, wyTrue);
		    break;

        //delete row(s)
	    case ID_RESULT_DELETE:
            //if the thread is stopped, then we have to initiate a deletion
            if(ThreadStopStatus())
            {
                CustomGrid_ApplyChanges(m_hwndgrid);
                exeinfo.m_ta = TA_DELETE;
                Execute(&exeinfo, wyTrue, wyTrue);
            }
            //otherwise stop the thread
            else
            {
                Stop(ID_RESULT_DELETE);
            }
		    break;

        //set the cell value to NULL
	    case IDC_SETNULL:
            SetValue("(NULL)");
            break;
	
        //set the cell value to default
	    case IDC_SETDEF:
		    SetValue("(DEFAULT)");
		    break;

        //set cell value to empty string
	    case IDC_SETEMPTY:
            SetValue("");
		    break;

        //save any unsaved changes
	    case ID_RESULT_SAVE:
            //if the thread is stoped, then we have to initiate an insert/update operation
		    if(ThreadStopStatus())
            {
                //call apply function and it will either insert/update the changes
                ApplyChanges(LA_NONE, 0, 0);
            }
            //if the thread is running, stop it
            else
            {
                Stop(ID_RESULT_SAVE);
            }
		    break;

        //cancel the changes
	    case ID_RESULT_CANCEL:
       	    CancelChanges();
		    break;

        //limit check click
	    case IDC_ROWSINRANGE:
            CustomGrid_ApplyChanges(m_hwndgrid);

            //we are reversing the state here, once the refresh is done, it will set the state again
            Button_SetCheck(m_hwndislimit, Button_GetCheck(m_hwndislimit) == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);

            SetFocus(GetActiveDispWindow());

            //refresh
            Execute(TA_REFRESH, wyTrue, wyTrue, LA_LIMITCLICK);
		    break;

        //refresh the view
	    case IDC_REFRESH:
            //if the thread is stoped, then we have to initiate refreshing
            if(ThreadStopStatus())
		    {
                CustomGrid_ApplyChanges(m_hwndgrid);
                SetFocus(GetActiveDispWindow());
                Execute(TA_REFRESH, wyTrue, wyTrue);
		    }
            //else stop the thread
		    else
		    {
			    Stop(IDC_REFRESH);
		    }

		    break;

        //next button pressed
        case IDC_NEXT:
            HandleNext();
            break;

        //previous button pressed
        case IDC_PREVIOUS:
            HandlePrevious();
            break;

        //handle click on "Data modified but not saved" warning
        case IDC_ROW_MSG:
            if(HIWORD(wparam) == STN_CLICKED)
            {
                //make sure we have a valid modified row, just a precaution
                if(m_data->m_modifiedrow >= 0)
	            {
                    //check whether it is new row, if not generate update query else generate insert query
                    if(m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow)->IsNewRow() == wyFalse)
                    {
		   		        ret = GenerateUpdateQuery(query, wyFalse);
                    }
		            else 
                    {
                        ret = GenerateInsertQuery(query);
                    }
	            }

                //preview instance
                preview = new QueryPreview();

                //if query is generated, then show preview
                if(ret == wyTrue)
                {
                    preview->Create(m_hwndframe, (wyChar*)query.GetString());
                }
                //there may be cases where a valid query is not generated. for example if somebody modified a cell value and changed it back
                //in those cases the return value from the query generation function will be wyFalse but we can check the query length to see there was an incomplete query
                //show a comment in such situations
                else if(query.GetLength())
                {
                    preview->Create(m_hwndframe, "/* No query to execute */");
                }

                //delete preview instance
                delete preview;
            }

            break;

        //handle click on "Show warnings"
	    case IDC_WARN_MSG:
		    if(HIWORD(wparam) == STN_CLICKED)
			{
                m_wnd->m_tunnel->mysql_data_seek(m_data->m_warningres, 0);
                ShowMySQLWarnings(m_hwndframe, m_wnd->m_tunnel, &m_wnd->m_mysql, m_data->m_lastdmlquery.GetString(), m_data->m_warningres);
			}

		    break;

        //handle export data
	    case IDM_IMEX_EXPORTDATA:
            if(m_data && m_data->m_rowarray && m_data->m_rowarray->GetLength())
            {
                ExportData();
            }

		    break;
        
        //handle copy cell
	    case IDM_IMEX_EXPORTCELL:
            if(m_data->m_datares)
            {
                CustomGrid_ApplyChanges(m_hwndgrid);
                CopyCellDataToClipBoard();
            }

		    break;
	
        //copy whole rows to clipboard
	    case IDM_DATATOCLIPBOARD:
            if(m_data->m_datares)
            {
                CustomGrid_ApplyChanges(m_hwndgrid);
		        AddDataToClipBoard();
            }

		    break;

        //copy selected rows to clipboard
	    case IDM_SELDATATOCLIPBOARD:
            if(m_data->m_datares)
            {
                CustomGrid_ApplyChanges(m_hwndgrid);
		        AddSelDataToClipBoard();
            }
            
            break;

        //switch to grid view
	    case ID_VIEW_GRIDVIEW:	
            SwitchView(GRID);
		    break;

        //handle switch to form view
	    case ID_VIEW_FORMVIEW:
            //if it is not community then only we are switching to form view otherwise we will open up the adv dialog
#ifndef COMMUNITY
            SwitchView(FORM);
#else
            pGlobals->m_pcmainwin->m_connection->GetSQLyogEntDialog();
#endif
		    break;

        //switch to text view
        case ID_VIEW_TEXTVIEW:
            if(m_data)
            {
                m_data->m_lastview = -1;
            }

            SwitchView(TEXT);
            break;

        //handle filter menu commands
	    case ID_FILTER_FIELD_EQUALS:
        case ID_FILTER_FIELD_NOTEQUALS:
        case ID_FILTER_FIELD_GREATERTHEN:
        case ID_FILTER_FIELD_LESSTHEN:
        case ID_FILTER_FIELDLIKEBEGIN:
        case ID_FILTER_FIELDLIKEEND:
        case ID_FILTER_FIELDLIKEBOTH:
        case ID_FILTER_CUSTOMFILTER:
            if(m_data->m_psortfilter)
            {
                OnRightClickFilter(LOWORD(wparam));
            }

            break;
		
        //hnalde duplicate row
	    case IDM_DUPLICATE_ROW:
            CustomGrid_ApplyChanges(m_hwndgrid);

            //duplicate the row
            DuplicateCurrentRow();
		    break;

        //click on filter button in toolbar or the reset filter menu command
        case ID_RESETFILTER:
            //we have to make sure that we have valid sort and filter
            if(m_data->m_psortfilter)
            {
                //get the current button imgage, second parameter will tell us whether it is the original image
             /*   GetButtonImage(ID_RESETFILTER, &ret);*/

                //if it is the original image and it is from a control, then we will open custom filter dialog
				if(m_data->m_psortfilter->m_isfilter == wyFalse && lparam)
                {
                    OnRightClickFilter(ID_FILTER_CUSTOMFILTER);
                }
                else
                {
                    //reset filter values
                    for(i = 0; i < m_data->m_psortfilter->m_filtercolumns; ++i)
                    {
                        m_data->m_psortfilter->m_filter[i].m_colindex = -1;
                        m_data->m_psortfilter->m_filter[i].m_filtertype = FT_NONE;
                        m_data->m_psortfilter->m_filter[i].m_value.Clear();
                    }

                    //execute reset filter
                    if(m_data->m_psortfilter->BeginFilter(0, NULL, 0, 0, m_hwndframe, m_querybuilder) == wyTrue)
                    {
                        Execute(TA_REFRESH, wyTrue, wyTrue, LA_FILTER);
                    }
                }
            }

            break;
		case ID_UNSORT:
			//we have to make sure that we have valid sort and filter
            if(m_data->m_psortfilter)
            {
                //get the current button imgage, second parameter will tell us whether it is the original image
             /*   GetButtonImage(ID_RESETFILTER, &ret);*/

                //if it is the original image and it is from a control, then we will open custom filter dialog
				//if(m_data->m_psortfilter->m_isfilter == wyFalse && lparam)
                //{
                   // OnRightClickFilter(ID_FILTER_CUSTOMFILTER);
                //}
                //else
                //{
                    //reset filter values
                    for(i = 0; i < m_data->m_psortfilter->m_sortcolumns; ++i)
                    {
                        m_data->m_psortfilter->m_sort[i].m_colindex = -1;
                        m_data->m_psortfilter->m_sort[i].m_sorttype = ST_NONE;
						m_data->m_psortfilter->m_sort[i].m_currcolindex = -1;
                        m_data->m_psortfilter->m_sort[i].m_currsorttype = ST_NONE;
                    }

                    //execute reset filter
					//m_data->m_psortfilter->BeginColoumnSort(wparam, wyTrue);
                    //if(m_data->m_psortfilter->BeginFilter(0, NULL, 0, 0, m_hwndframe, m_querybuilder) == wyTrue)
                    //{
                    Execute(TA_REFRESH, wyTrue, wyTrue, LA_SORT);
                    //}
                //}
            }
			break;
    }
}

//function reset the toolbar button
void 
DataView::ResetButtons(HWND hwndtool)
{
    wyInt32         i, count;
    TBBUTTONINFO    tbbi = {0};

    //precaution
    if(hwndtool)
    {
        //get the button count
        count = SendMessage(hwndtool, TB_BUTTONCOUNT, 0, 0);

        tbbi.cbSize = sizeof(tbbi);

        //loop thorught the buttons
        for(i = 0; i < count; ++i)
        {
            tbbi.dwMask = TBIF_LPARAM | TBIF_BYINDEX | TBIF_COMMAND | TBIF_IMAGE;
        
            //get the button info and make sure that the current image is not the original image
            if(SendMessage(hwndtool, TB_GETBUTTONINFO, i, (LPARAM)&tbbi) != -1 && tbbi.lParam != tbbi.iImage)
            {
                //handle filter button seperately
                if(tbbi.idCommand == ID_RESETFILTER)
                {
                    ChangeFilterIcon();
                }
                //otherwise reset the button image to its original image
                else
                {
                    SendMessage(hwndtool, TB_CHANGEBITMAP, (WPARAM)tbbi.idCommand, tbbi.lParam);
                }
            }
        }

        //update toolabr
        InvalidateRect(hwndtool, NULL, TRUE);
        UpdateWindow(hwndtool);
    }
}

//function to update the three view buttons
void
DataView::UpdateViewButtons()
{
    HWND hwnd;

    //get the toolbar handle for the button
    if((hwnd = GetToolBarFromID(ID_VIEW_GRIDVIEW)))
    {
        //now based on the view type, update the buttons
        switch(m_viewtype)
        {
            case TEXT:
                SendMessage(hwnd, TB_SETSTATE, (WPARAM)ID_VIEW_FORMVIEW, TBSTATE_ENABLED);
                SendMessage(hwnd, TB_SETSTATE, (WPARAM)ID_VIEW_GRIDVIEW, TBSTATE_ENABLED);
                SendMessage(hwnd, TB_SETSTATE, (WPARAM)ID_VIEW_TEXTVIEW, TBSTATE_ENABLED | TBSTATE_PRESSED);
                break;

            case GRID:
                SendMessage(hwnd, TB_SETSTATE, (WPARAM)ID_VIEW_FORMVIEW, TBSTATE_ENABLED);
                SendMessage(hwnd, TB_SETSTATE, (WPARAM)ID_VIEW_TEXTVIEW, TBSTATE_ENABLED);
                SendMessage(hwnd, TB_SETSTATE, (WPARAM)ID_VIEW_GRIDVIEW, TBSTATE_ENABLED | TBSTATE_PRESSED);
                break;

			case FORM:
				SendMessage(hwnd, TB_SETSTATE, (WPARAM)ID_VIEW_GRIDVIEW, TBSTATE_ENABLED);
				SendMessage(hwnd, TB_SETSTATE, (WPARAM)ID_VIEW_TEXTVIEW, TBSTATE_ENABLED);
				SendMessage(hwnd, TB_SETSTATE, (WPARAM)ID_VIEW_FORMVIEW, TBSTATE_ENABLED | TBSTATE_PRESSED);
				break;
        }		
    }
}

//function to set the display mode
void
DataView::SetDispMode(ViewType mode, wyBool isupdateview)
{
    wyBool isviewupdated = wyFalse;

    //get the view type into a member variable
    m_viewtype = (mode == FORM ? (ShowFormView(SW_SHOW, wyTrue) == wyTrue ? FORM : m_viewtype) : mode);

    //close the find window
    if(pGlobals->m_pcmainwin->m_finddlg)
	{
	    DestroyWindow(pGlobals->m_pcmainwin->m_finddlg);
		pGlobals->m_pcmainwin->m_finddlg = NULL;
    }    

    //if we have data loaded
    if(m_data)
    {
        //check the current mode is not the same as the mode to switch to 
        if(m_data->m_viewtype != mode)
        {
            CustomGrid_ApplyChanges(m_hwndgrid, wyTrue);
            m_data->m_viewtype = mode;
            isviewupdated = wyTrue;
        }

        //show respective window
        if(isupdateview == wyTrue && ShowDataWindow(SW_SHOW) == wyFalse)
        {
            return;
        }

        if(isviewupdated == wyTrue)
        {
            //handle view persistance
            m_data->HandleViewPersistence(wyTrue);
        }
    }
    //even if there is no data we will update the window
    else if(isupdateview == wyTrue)
    {
        ShowDataWindow(SW_SHOW);
    }

    //if no query execution then we will enable the toolbar, otherwise just update the view buttons
    if(ThreadStopStatus())
    {
        EnableToolbar(wyTrue);
    }
    else
    {
        UpdateViewButtons();
    }
}

//function to get the database name from the result set
wyBool 
DataView::GetDBName(wyString& db, wyInt32 col)
{
    //make sure everything is proper
    if(m_data && m_data->m_datares && col >= 0 && col < m_data->m_datares->field_count)
    {
        //if it is http connection, then db member will be invalide, so we will supply the current db name
        if(m_wnd->m_tunnel->IsTunnel() == wyTrue)
        {
            db.SetAs(m_data->m_db.GetString());
        }
        //otherwise get the db name from the filed sturcture
        else if(m_data->m_datares->fields[col].db)
        {
            db.SetAs(m_data->m_datares->fields[col].db, m_wnd->m_ismysql41);
        }

        //default action if db is empty
        if(!db.GetLength())
        {
            db.SetAs(m_data->m_db.GetString());
        }

        return wyTrue;
    }

    return wyFalse;
}

//function to get the table name from the result set
wyBool 
DataView::GetTableName(wyString& table, wyInt32 col)
{
    //make sure things are in proper shape
    if(m_data && m_data->m_datares && col >= 0 && col < m_data->m_datares->field_count)
    {
        //if orig_table is valid then use it otherwise use table name
        if(m_data->m_datares->fields[col].org_table && m_data->m_datares->fields[col].org_table[0])
        {
            table.SetAs(m_data->m_datares->fields[col].org_table, m_wnd->m_ismysql41);
        }
        else
        {
            table.SetAs(m_data->m_datares->fields[col].table, m_wnd->m_ismysql41);
        }

        return table.GetLength() ? wyTrue : wyFalse;
    }

    return wyFalse;
}

//function to set the database and table name
void 
DataView::SetDbTable(const wyChar* db, const wyChar* table)
{
    m_data->m_db.SetAs(db);
    m_data->m_table.SetAs(table);
}

//function to get the column name from the result set
wyBool 
DataView::GetColumnName(wyString& column, wyInt32 col)
{
    //make sure things are in proper shape
    if(m_data && m_data->m_datares && col >= 0 && col < m_data->m_datares->field_count)
    {
        //if orig_name is valid then use it otherwise use name
        if(m_data->m_datares->fields[col].org_name && m_data->m_datares->fields[col].org_name[0])
        {
            column.SetAs(m_data->m_datares->fields[col].org_name, m_wnd->m_ismysql41);
        }
        else
        {
            column.SetAs(m_data->m_datares->fields[col].name, m_wnd->m_ismysql41);
        }

        return column.GetLength() ? wyTrue : wyFalse;
    }

    return wyFalse;
}

//function to check whether a column is readonly
wyBool 
DataView::IsColumnReadOnly(wyInt32 col)
{
    wyString db, table;

    //get the db and table name 
    if(GetDBName(db, col) == wyTrue && GetTableName(table, col) == wyTrue)
    {
        //if the db and table for the column are same as the active db and table, then it is not readonly
        if(!m_data->m_db.Compare(db) && !m_data->m_table.Compare(table))
        {
            return wyFalse;
        }
    }

    //if the control is here, it means the column is readonly
    return wyTrue;
}

//function to check whether a column is virtual or not
wyInt32 
DataView::IsColumnVirtual(wyInt32 col)
{
    wyString query,db, table,column;
	MYSQL_RES  *fieldres = NULL;

	if(! IsMySQL576Maria52(m_wnd->m_tunnel, &m_wnd->m_mysql))
		return 0;

	if(m_data->m_colvirtual)
	{
		if(m_data->m_colvirtual[col]!=-1)
		{
			return m_data->m_colvirtual[col];
		}

		else
		{
			//get the db and table name 
			if(GetDBName(db, col) == wyTrue && GetTableName(table, col) == wyTrue && GetColumnName(column, col) == wyTrue)
			 {
				query.Sprintf("show full fields from `%s`.`%s` WHERE FIELD='%s' AND (Extra LIKE '%%VIRTUAL%%' OR Extra LIKE '%%STORED%%' OR Extra LIKE '%%PERSISTENT%%')", db.GetString(), table.GetString(),column.GetString());
				fieldres = SjaExecuteAndGetResult(m_wnd->m_tunnel,&m_wnd->m_mysql,query);

				if(fieldres)
				{
					if(fieldres->row_count == 1)
						{
							m_data->m_colvirtual[col] = 1;
						}
					else
						{
							m_data->m_colvirtual[col] = 0;
						}

				m_wnd->m_tunnel->mysql_free_result(fieldres);
				}
		
			}
			
			return m_data->m_colvirtual[col];
		}

	}
    
	else
		return 0;
}

//function to check whether a column is of type binary
wyBool 
DataView::IsBinary(wyInt32 col)
{
    wyBool isbin;

    isbin = ((m_data->m_datares->fields[col].flags & BINARY_FLAG) && 
        (m_data->m_datares->fields[col].type == MYSQL_TYPE_TINY_BLOB || m_data->m_datares->fields[col].type == MYSQL_TYPE_JSON ||
        m_data->m_datares->fields[col].type == MYSQL_TYPE_MEDIUM_BLOB ||
        m_data->m_datares->fields[col].type == MYSQL_TYPE_BLOB ||
        m_data->m_datares->fields[col].type == MYSQL_TYPE_LONG_BLOB)) ? wyTrue : wyFalse;

    return isbin;
}

//wrapper function to exprot data
void
DataView::ExportData()
{
    CExportResultSet    exportresult;

    exportresult.Export(m_hwndframe, m_data);
}

//function to add data to query
wyBool
DataView::AddDataToQuery(MYSQL_ROW data, wyString &query, const wyChar* delimiter, const wyChar* nullcheck, wyInt32 col, wyBool isfirst, wyBool ischeckspdata)
{
    wyBool      isblob, isgenerated = wyFalse;
    wyInt32     i, max;
    wyString    colname;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    //if valid col is specified then we will add data for that column only
    if(col >= 0)
    {
        i = col;
        max = col + 1;
    }
    //otherwise add data for all columns
    else
    {
        i = 0;
        max = m_data->m_datares->field_count;
    }
    
    for(; i < max; i++)
    {
        //we ignore readonly columns
        if(IsColumnReadOnly(i) == wyFalse && IsColumnVirtual(i) != 1 )
        {
            isblob = IsBinary(i);
            GetColumnName(colname, i);

            //for not null data
            if(data[i] && strcmp(data[i], "NULL") != 0 && strcmp(data[i], "(NULL)") != 0)
		    {	
                query.AddSprintf("%s%s%s%s%s = ", 
                    isfirst == wyFalse ? " " : "",
                    isfirst == wyFalse ? delimiter : "",
                    m_backtick, colname.GetString(), m_backtick);

                //consider interpreting keywords/functions
                if(ischeckspdata == wyFalse || AppendSpecialDataToQuery(data[i], (wyChar*)colname.GetString(), query) == wyFalse)
                {
                    //if it is not special data or we dont want to consider special data, then add the raw data to query
                    AppendDataToQuery(data, i, m_data->m_datares->field_count, query, isblob);
                }
            }
		    else
            {
                //add query data for null, for an insert/update query nullcheck is '= NULL', for others it is 'IS NULL'
			    query.AddSprintf("%s%s%s%s%s %s", 
                    isfirst == wyFalse ? " " : "",
                    isfirst == wyFalse ? delimiter : "",
                    m_backtick, colname.GetString(), m_backtick,
                    nullcheck);
            }

            //set some flags so that we can add proper delimiters
            isfirst = wyFalse;
            isgenerated = wyTrue;
        }
    }

    return isgenerated;
}

wyBool
DataView::AddDataToQueryJSON(MYSQL_ROW data, wyString &query, const wyChar* delimiter, const wyChar* nullcheck, wyInt32 col, wyBool isfirst, wyBool ischeckspdata)
{
    wyBool      isjson,isbin ,isgenerated = wyFalse;
    wyInt32     i, max;
    wyString    colname;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    //if valid col is specified then we will add data for that column only
    if(col >= 0)
    {
        i = col;
        max = col + 1;
    }
    //otherwise add data for all columns
    else
    {
        i = 0;
        max = m_data->m_datares->field_count;
    }
    
    for(; i < max; i++)
    {
        //we ignore readonly columns
        if(IsColumnReadOnly(i) == wyFalse && IsColumnVirtual(i) != 1 )
        {
            isbin = IsBinary(i);
			isjson = IsJSON(i);

            GetColumnName(colname, i);

			if(isjson==wyFalse)
			{
				if(data[i] && strcmp(data[i], "NULL") != 0 && strcmp(data[i], "(NULL)") != 0)
		    {	
                query.AddSprintf("%s%s%s%s%s = ", 
                    isfirst == wyFalse ? " " : "",
                    isfirst == wyFalse ? delimiter : "",
                    m_backtick, colname.GetString(), m_backtick);

                //consider interpreting keywords/functions
                if(ischeckspdata == wyFalse || AppendSpecialDataToQuery(data[i], (wyChar*)colname.GetString(), query) == wyFalse)
                {
                    //if it is not special data or we dont want to consider special data, then add the raw data to query
                    AppendDataToQuery(data, i, m_data->m_datares->field_count, query, isbin);
                }
            }
		    else
            {
                //add query data for null, for an insert/update query nullcheck is '= NULL', for others it is 'IS NULL'
			    query.AddSprintf("%s%s%s%s%s %s", 
                    isfirst == wyFalse ? " " : "",
                    isfirst == wyFalse ? delimiter : "",
                    m_backtick, colname.GetString(), m_backtick,
                    nullcheck);
            }

		    }
            //for not null data
			else
			{
			if(data[i] && strcmp(data[i], "NULL") != 0 && strcmp(data[i], "(NULL)") != 0)
		    {	
                query.AddSprintf("%s%s%s", 
                    isfirst == wyFalse ? " " : "",
                    isfirst == wyFalse ? delimiter : "",
                    "JSON_CONTAINS(");

                //consider interpreting keywords/functions
                if(ischeckspdata == wyFalse || AppendSpecialDataToQuery(data[i], (wyChar*)colname.GetString(), query) == wyFalse)
                {
                    //if it is not special data or we dont want to consider special data, then add the raw data to query
                    AppendDataToQuery(data, i, m_data->m_datares->field_count, query, isbin);
                }
				query.AddSprintf(",%s%s%s)", m_backtick, colname.GetString(), m_backtick);

            }
		    else
            {
                //add query data for null, for an insert/update query nullcheck is '= NULL', for others it is 'IS NULL'
			    query.AddSprintf("%s%s%s%s%s %s", 
                    isfirst == wyFalse ? " " : "",
                    isfirst == wyFalse ? delimiter : "",
                    m_backtick, colname.GetString(), m_backtick,
                    nullcheck);
            }
			}

            //set some flags so that we can add proper delimiters
            isfirst = wyFalse;
            isgenerated = wyTrue;
        }
    
	}

    return isgenerated;
}


//function to generate query to find duplicates of an item in the table
wyBool
DataView::GenerateDuplicatesQuery(wyInt32 row, wyString &query)
{
	query.Sprintf("select count(*) from `%s`.`%s` where ", m_data->m_db.GetString(), m_data->m_table.GetString());

    //here we supply nullcheck as 'IS NULL' and we dont want to check for special data
    return AddDataToQuery(m_data->m_oldrow ? m_data->m_oldrow->m_row : m_data->m_rowarray->GetRowExAt(row)->m_row, 
        query, "and ", "is null", -1);
}

//function to generate insert query
wyBool
DataView::GenerateInsertQuery(wyString &query)
{
    wyInt32         i, k;
    wyString        colname;
    wyBool          isblob;
    wyChar*         fieldarray;    
    MYSQL_ROW       myrow;
    MYSQL_ROWEX*    myrowex;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    fieldarray = new wyChar[m_data->m_datares->field_count];  
    myrowex = m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow);
    memset(fieldarray, myrowex && (myrowex->m_state & ROW_DUPLICATE) ? 1 : 0, m_data->m_datares->field_count * sizeof(wyChar));    
    
    query.Sprintf("insert into %s%s%s.%s%s%s (", 
		m_backtick, m_data->m_db.GetString(), m_backtick, 
		m_backtick, m_data->m_table.GetString(), m_backtick);

    //loop through columns
	for(i = 0, k = 0; i < m_data->m_datares->field_count; i++)
	{
        myrow = m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow)->m_row;

        //ignore readonly columns and columns that are same        
        if(IsColumnReadOnly(i) == wyFalse && IsColumnVirtual(i) != 1 && (!m_data->m_oldrow || m_data->m_oldrow->m_row[i] != myrow[i] || fieldarray[i]))        
        {
            if(!fieldarray[i] && m_data->m_oldrow && m_data->m_oldrow->m_row[i] && myrow[i] && !strcmp(m_data->m_oldrow->m_row[i], myrow[i]))            
            {                
                continue;            
            }            
            
            fieldarray[i] = 1;            
            
            //if the column type is spatial, then we are showing a message and aborting.
            //you should use SendThreadMessage as it is not guarenteed that the function is called from gui thread
            if(m_data->m_datares->fields[i].type == MYSQL_TYPE_GEOMETRY)
            {
                SendThreadMessage(m_hwndframe, UM_QUERYGENERATIONFAILED, TA_INSERT, 
                    (LPARAM)_("INSERT and UPDATE of Spatial data types are not supported."));
                query.Clear();
                delete[] fieldarray;
                return wyFalse;
            }

            //add column name
            GetColumnName(colname, i);
            query.AddSprintf("%s%s%s%s", k++ ? ", " : "", m_backtick, colname.GetString(), m_backtick);
        }
	}

    //now we add the values
    query.Add(")\r\n\tvalues\r\n\t(");

    for(i = 0, k = 0; i < m_data->m_datares->field_count; i++)
	{
        //ignore readonly columns
        if(IsColumnReadOnly(i) == wyFalse && IsColumnVirtual(i) != 1 && fieldarray[i])
        {
            if(k++)
            {
                query.Add(", ");
            }

            isblob = IsBinary(i);
            GetColumnName(colname, i);
        
            //consider adding special data
            if(AppendSpecialDataToQuery(m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow)->m_row[i], 
                (wyChar*)colname.GetString(), query) == wyFalse)
            {
                //if it was not special data, then add the data as it is
                AppendDataToQuery(m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow)->m_row, 
                    i, m_data->m_datares->field_count, query, isblob);
            }
        }
    }

	query.Add(");\r\n");
    delete[] fieldarray;
    return wyTrue;
}

//function to generate delete query
wyBool 
DataView::GenerateDeleteQuery(wyString& query, wyBool issetlimit, wyUInt32 row)
{
    wyString    colname, tempstr;
    wyInt32     i, pkcount = 0;
    wyBool      isfirst = wyTrue, isanyprimary;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    query.Sprintf("delete from %s%s%s.%s%s%s where ", 
		m_backtick, m_data->m_db.GetString(), m_backtick,
		m_backtick, m_data->m_table.GetString(), m_backtick);
    isanyprimary = IsAnyPrimary(m_wnd->m_tunnel, m_data->m_keyres, &pkcount);

    //if the table has any primary key defined, then we will use only that in our where clause
    if(isanyprimary == wyTrue)
    {
        //loop through fileds
	    for(i = 0; i < m_data->m_datares->field_count ; i++)
        {
            GetColumnName(colname, i);
		
            //if it is primary key column
		    if(IsColumnPrimary(m_wnd->m_tunnel, m_data->m_fieldres, (wyChar*)colname.GetString()) == wyTrue)
		    {
                //if the row is modified but not saved, then we will consider the old data otherwise current data
                if(m_data->m_modifiedrow >= 0 && row == m_data->m_modifiedrow)
                {
                    isfirst = AddDataToQuery(m_data->m_oldrow->m_row, tempstr, "and ", "is null", i, isfirst) ? --pkcount, wyFalse : wyTrue;
                }
                else
                {
                    isfirst = AddDataToQuery(m_data->m_rowarray->GetRowExAt(row)->m_row, tempstr, "and ", "is null", i, isfirst) ? --pkcount, wyFalse : wyTrue;
                }
		    }
	    }
    }

    //in case no primary keys
    if(isanyprimary == wyFalse || pkcount)
	{
        tempstr.Clear();

        //if the row is modified but not saved, then we will consider the old data otherwise current data
        if(m_data->m_modifiedrow >= 0 && row == m_data->m_modifiedrow)
        {
            isfirst = AddDataToQueryJSON(m_data->m_oldrow->m_row, tempstr, "and ", "is null", -1) ? wyFalse : wyTrue;
        }
        else
        {
            isfirst = AddDataToQueryJSON(m_data->m_rowarray->GetRowExAt(row)->m_row, tempstr, "and ", "is null", -1) ? wyFalse : wyTrue;
        }

        //whether to delete only the current row or all its duplicates
        if(issetlimit == wyTrue)
        {
            tempstr.Add(" limit 1");
        }
	}

    query.Add(tempstr.GetString());
    return isfirst ? wyFalse : wyTrue;
}

//function to generate update query
wyBool
DataView::GenerateUpdateQuery(wyString &query, wyBool issetlimit)
{
    wyString    colname, tempstr;
    wyInt32     i, pkcount = 0;
    wyBool      isfirst, isanyprimary, isprimary;
    wyUInt32     len1 = 0;
    wyUInt32     len2 = 0;

    //if no modified row or no old row, means we came here by mistake, so lets return
    if(m_data->m_modifiedrow < 0 || !m_data->m_oldrow)
    {
        return wyFalse;
    }
	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    isanyprimary = IsAnyPrimary(m_wnd->m_tunnel, m_data->m_keyres, &pkcount);
    query.Sprintf("update %s%s%s.%s%s%s set ", 
		m_backtick, m_data->m_db.GetString(), m_backtick,
		m_backtick, m_data->m_table.GetString(), m_backtick);

    //loop throgh the columns adding only the columns that are updated
	for(isfirst = wyTrue, i = 0; i < m_data->m_datares->field_count ; i++)
    {
        //precaution
        if(!(!(m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow)->m_row[i]) && !(m_data->m_oldrow->m_row[i])))
		{
            if(m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow)->m_row[i])
            {
                GetUtf8ColLength(m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow)->m_row, m_data->m_datares->field_count, i, &len1);
            }

            if(m_data->m_oldrow->m_row[i])
            {
                GetUtf8ColLength(m_data->m_oldrow->m_row, m_data->m_datares->field_count, i, &len2);
            }

            if((!m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow)->m_row[i] || !(m_data->m_oldrow->m_row[i])) || len1 != len2 || 
                memcmp(m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow)->m_row[i], m_data->m_oldrow->m_row[i], len1 > len2 ? len2 : len1) != 0)
            {
                //add the data to query, and we use '= null' as nullcheck since we are setting a column value
                isfirst = AddDataToQuery(m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow)->m_row, 
                    query, ", ", "= null", i, isfirst, wyTrue) ? wyFalse : wyTrue;

                //if data was added to the query then we have to make sure that its not spacial data
                if(isfirst == wyFalse)
                {
                    if(m_data->m_datares->fields[i].type == MYSQL_TYPE_GEOMETRY)
                    {
                        //if the column type is spatial, then we are showing a message and aborting.
                        //you should use SendThreadMessage as it is not necessery that the function is called from gui thread
                        SendThreadMessage(m_hwndframe, UM_QUERYGENERATIONFAILED, TA_UPDATE, 
                            (LPARAM)_("INSERT and UPDATE of Spatial data types are not supported."));
                        query.Clear();
                        return wyFalse;
                    }
                }
            }
		}       
    }

    //if no column added to the query, then we are aborting
    if(isfirst == wyTrue)
    {
        return wyFalse;
    }

	query.Add("\r\n\twhere\r\n\t");

    //if the table has any primary key, then we need to add only those fields in the where clause
	if(isanyprimary)
    {
	    for(isfirst = wyTrue, i = 0; i < m_data->m_datares->field_count; i++)
        {
            GetColumnName(colname, i);
            isprimary = IsColumnPrimary(m_wnd->m_tunnel, m_data->m_fieldres, (wyChar*)colname.GetString());
		
		    if(isprimary)
		    {
                isfirst = AddDataToQuery(m_data->m_oldrow->m_row, tempstr, "and ", "is null", i, isfirst) ? --pkcount, wyFalse : wyTrue;
		    }
	    }
    }
	
    if(isanyprimary == wyFalse || pkcount)
	{
        tempstr.Clear();

        //else add all the columns to the query
	//	if(!FIELD_TYPE_JSON)
   //    if(m_data->m_datares->fields->type != MYSQL_TYPE_JSON)
	//	AddDataToQuery(m_data->m_oldrow->m_row, tempstr, "and ", "is null", -1);
	//   else
		AddDataToQueryJSON(m_data->m_oldrow->m_row, tempstr, "and ", "is null", -1);
        //whether to update only the current row or all its duplicates
        if(issetlimit == wyTrue)
        {
            tempstr.Add(" limit 1");
        }
	}

    query.Add(tempstr.GetString());
	query.Add(";\r\n");
    return wyTrue;
}

//function to enable tool button
wyBool 
DataView::EnableToolButton(wyBool isenable, wyInt32 id)
{
    wyInt32     i, count;
    TBBUTTON    tbb;
    wyBool      ret = wyFalse;

    //if id is -1 then we are enabling/disableing all the buttons
    if(id == -1)
    {
        count = SendMessage(m_hwndtoolbar, TB_BUTTONCOUNT, 0, 0);

        for(i = 0; i < count; ++i)
        {
            SendMessage(m_hwndtoolbar, TB_GETBUTTON, i, (LPARAM)&tbb);
            SendMessage(m_hwndtoolbar, TB_SETSTATE, (WPARAM)tbb.idCommand, isenable == wyTrue ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE);        
        }
        
        //enable refresh controls too
        EnableRefreshControls(isenable);
    }
    else
    {
        //check whether the button belongs to a toolbar and if yes, enable disable it
        if(SendMessage(m_hwndtoolbar, TB_COMMANDTOINDEX, id, 0) != -1)
        {
            SendMessage(m_hwndtoolbar, TB_SETSTATE, (WPARAM)id, isenable == wyTrue ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE);
            ret = wyTrue;
        }
        else if(SendMessage(m_hwndrefreshtool, TB_COMMANDTOINDEX, id, 0) != -1)
        {
            SendMessage(m_hwndrefreshtool, TB_SETSTATE, (WPARAM)id, isenable == wyTrue ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE);
            ret = wyTrue;
        }
    }

    return ret;
}

//function to create the parent frame
void
DataView::CreateFrame()
{
    m_hwndframe = CreateWindowEx(WS_EX_CONTROLPARENT, 
								 DATA_WINDOW, 
                                 L"", WS_CHILD, 0, 0, 0, 0, 	
								 m_hwndparent, 
								 (HMENU)20, 
								 pGlobals->m_entinst, NULL);

	SetWindowLongPtr(m_hwndframe, GWLP_USERDATA, (LONG_PTR)this);
    ShowWindow(m_hwndframe, SW_HIDE);
}

//create the grid
void
DataView::CreateGrid()
{
	m_hwndgrid = CreateCustomGridEx(m_hwndframe, 
									0, 0, 0, 0, 
									m_gridwndproc, 
									GV_EX_ROWCHECKBOX | GV_EX_OWNERDATA | GV_EX_COL_TOOLTIP, (LPARAM)this);
    
	CustomGrid_SetOwnerData(m_hwndgrid, wyTrue);
	SetGridFont();
    ShowWindow(m_hwndgrid, SW_HIDE);
}

//function to set the grid font
void 
DataView::SetGridFont()
{
	wyWChar*    lpfileport = 0;
	wyWChar	    directory[MAX_PATH + 1] = {0};
	wyString	dirstr, fontstr;
	LOGFONT	    datafont = {0};
	wyInt32	    px, height;
	HDC		    hdc;

	hdc = GetDC(m_hwndframe);
	height = GetDeviceCaps( hdc, LOGPIXELSY );

	//get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	
	dirstr.SetAs(directory);

	//fill with default data.
	FillEditFont(&datafont, m_hwndgrid);
	
    //read from ini
	wyIni::IniGetString(DATAFONT, "FontName", "Courier New", &fontstr, dirstr.GetString()); 	
	wcsncpy(datafont.lfFaceName, fontstr.GetAsWideChar(), 31);
	datafont.lfFaceName[31] = '\0';
    px = wyIni::IniGetInt(DATAFONT, "FontSize", 9, dirstr.GetString()); 	
	
    datafont.lfHeight = -MulDiv(px, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    datafont.lfWeight = wyIni::IniGetInt(DATAFONT, "FontStyle", 0, dirstr.GetString()); 	
	datafont.lfItalic = wyIni::IniGetInt(DATAFONT, "FontItalic", 0, dirstr.GetString()); 	
	datafont.lfCharSet = wyIni::IniGetInt(DATAFONT, "FontCharSet", DEFAULT_CHARSET, dirstr.GetString()); 
	
    //set the font
	CustomGrid_SetFont(m_hwndgrid, &datafont);

	ReleaseDC(m_hwndframe, hdc);
}

//subclassed window proc for the bottom ribbon showing additional information
LRESULT CALLBACK 
DataView::TableInfoWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC         wndproc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    RECT            rect;
    HDC             hdc;
    PAINTSTRUCT     ps;
    DataView*       dvptr;
    HBRUSH          hbr;
    COLORREF        fg, bg;
        
    //we are handling the paint so that custom drawing is possible
    if(message == WM_PAINT)
    {
        dvptr = (DataView*)GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA);
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rect);

        //get the ribbon color and fill the background
        dvptr->GetRibbonColorInfo(&bg, &fg);
        hbr = CreateSolidBrush(bg);
        FillRect(hdc, &rect, hbr);
        DeleteBrush(hbr);

        //give margin
        rect.left += 1;
        rect.right -= 1;

        //set the forground color
        SetTextColor(hdc, fg);
        SetBkMode(hdc, TRANSPARENT);

        //draw the info
        dvptr->DrawTableInfoRibbon(hdc, rect);

        EndPaint(hwnd, &ps);
        return 1;
    }
    //blocked to avoid flickering
    else if(message == WM_ERASEBKGND)
    {
        return 1;
    }

    //call the original window proc for other messages
    return CallWindowProc(wndproc, hwnd, message, wParam, lParam);
}


///function to change text view color
void
DataView::SetColor()
{
    wyWChar		*lpfileport = 0;
	wyWChar		directory[MAX_PATH+1] = {0};
    COLORREF	color,backcolor;
	
	//Get the complete path.
	if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyTrue)
    {
        wyString	dirstr(directory);

        backcolor   =   wyIni::IniGetInt(GENERALPREFA, "MTISelectionColor",   DEF_TEXTSELECTION, dirstr.GetString());
        SendMessage(m_hwndtext,SCI_SETSELBACK,1,backcolor);
        
        backcolor=wyIni::IniGetInt(GENERALPREFA, "MTIBgColor", DEF_BKGNDEDITORCOLOR, dirstr.GetString()); 
        SendMessage( m_hwndtext, SCI_STYLESETBACK, STYLE_DEFAULT, (LPARAM)backcolor);
        
        SendMessage( m_hwndtext, SCI_SETCARETFORE,backcolor ^ 0xFFFFFF,0); //Change Caret color in editor window

        color = wyIni::IniGetInt(GENERALPREFA, "MTIFgColor", DEF_NORMALCOLOR, dirstr.GetString()); 
        
        SendMessage(m_hwndtext, SCI_STYLESETFORE, SCE_MYSQL_DEFAULT, color);
        SendMessage(m_hwndtext, SCI_STYLESETBACK, SCE_MYSQL_DEFAULT, backcolor);
        SendMessage( m_hwndtext, SCI_SETCARETFORE,backcolor ^ 0xFFFFFF,0);
        SendMessage(m_hwndtext, SCI_STYLESETBOLD, SCE_MYSQL_DEFAULT, FALSE);
    }
}


//function to create the text view control
void
DataView::CreateText()
{
	wyInt32 styles = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL;

    //create the scintilla window so that we have the luxury of rectangular selection
	m_hwndtext = CreateWindowEx(0, L"Scintilla",  NULL, styles, 0, 0, 0, 0, 
                                m_hwndframe, (HMENU)IDC_TABLEDATAEDIT, 
								GetModuleHandle(0), this);

    //set some properties
    SendMessage(m_hwndtext, SCI_SETREADONLY, TRUE, 0);
    SendMessage(m_hwndtext, SCI_SETMARGINWIDTHN, 1, 0);
    SendMessage(m_hwndtext, SCI_SETSCROLLWIDTHTRACKING, TRUE, 0);
    SendMessage(m_hwndtext, SCI_SETCODEPAGE, SC_CP_UTF8, 0);

    //subclass it
	m_wptextview = (WNDPROC)SetWindowLongPtr(m_hwndtext, GWLP_WNDPROC, (LONG_PTR)DataView::EditWndProc);	
	SetWindowLongPtr(m_hwndtext, GWLP_USERDATA,(LONG_PTR)this);

    //set the font
    SetTextViewFont();
    SetColor();
	ShowWindow(m_hwndtext, SW_HIDE);
}

//create form view
void
DataView::CreateForm()
{
    //we create formview only for enterprise and ultimate editions
#ifndef COMMUNITY
    if(pGlobals->m_entlicense.CompareI("Professional"))
    {
        m_formview = new FormView(this);
        m_formview->InitFormView();
    }
#endif
}

///function to create warning windows
void 
DataView::CreateWarnings()
{
    LOGFONT lf = {0};
    HFONT   hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    WNDPROC wndproc;
    
    //create 'Data modified but not saved' window
    m_hwndmodified = CreateWindowEx(0, WC_STATIC, SAVE_WARNING, 
                                    WS_CHILD | SS_NOTIFY, 0, 0, 0, 0, m_hwndframe, (HMENU)IDC_ROW_MSG, 
									(HINSTANCE)pGlobals->m_hinstance, this);

    //subclass it so that we can alter the behavior
    m_warningwndproc = (WNDPROC)SetWindowLongPtr(m_hwndmodified, GWLP_WNDPROC,(LONG_PTR)ShowWarningWndProc);	
	SetWindowLongPtr(m_hwndmodified, GWLP_USERDATA, (LONG_PTR)this);

    //create warning window
	m_hwndwarning = CreateWindowEx(0, WC_STATIC, SHOW_WARNING, 
                                   WS_CHILD | SS_NOTIFY, 0, 0, 0, 0,
                                   m_hwndframe,
								   (HMENU)IDC_WARN_MSG,
								   (HINSTANCE)pGlobals->m_hinstance, this);

    //subclass it so that we can alter the behavior
    SetWindowLongPtr(m_hwndwarning, GWLP_WNDPROC,(LONG_PTR)ShowWarningWndProc);	
	SetWindowLongPtr(m_hwndwarning, GWLP_USERDATA,(LONG_PTR)this);

    //create the bottom ribbon
	m_hwndshowtableinfo = CreateWindow(L"STATIC", L"", 
								       WS_CHILD | WS_VISIBLE | SS_WORDELLIPSIS, 
								       0, 0, 0, 0,
                                       m_hwndframe, (HMENU)IDC_SHOWDBTABLE,
								       (HINSTANCE)pGlobals->m_hinstance, this);

    wndproc = (WNDPROC)SetWindowLongPtr(m_hwndshowtableinfo, GWLP_WNDPROC, (LONG_PTR)TableInfoWndProc);
    SetWindowLongPtr(m_hwndshowtableinfo, GWLP_USERDATA, (LONG_PTR)wndproc);
    SendMessage(m_hwndshowtableinfo, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE);

    //now we set fonts for warning windows, so that we can underline the text
    GetObject(hfont, sizeof(lf), &lf);
    lf.lfUnderline = TRUE;
    m_modifiedfont = CreateFontIndirect(&lf);
    SendMessage(m_hwndwarning, WM_SETFONT, (WPARAM)m_modifiedfont, TRUE);
    SendMessage(m_hwndmodified, WM_SETFONT, (WPARAM)m_modifiedfont, TRUE);

    //hide everything
	ShowWindow(m_hwndwarning, SW_HIDE);
    ShowWindow(m_hwndmodified, SW_HIDE);
    ShowWindow(m_hwndshowtableinfo, SW_SHOW);
}

//function to add limit values to the query
void
DataView::GetLimits(wyString &query)
 {
     if(m_data->m_islimit == wyTrue)
     {
        query.AddSprintf(" limit %d, %d",m_data->m_startrow, m_data->m_limit);
     }
 }

//function to add where clauss to the query
void 
DataView::GetFilterInfo(wyString& query)
{
    wyString filterinfo;

    if(m_data->m_psortfilter)
    {
        m_data->m_psortfilter->GetFilterString(filterinfo);
    }

    query.Add(filterinfo.GetString());
}

//function to add order by clauss to the query
void 
DataView::GetSortInfo(wyString& query)
{
    wyString sortinfo;

    if(m_data->m_psortfilter)
    {
        m_data->m_psortfilter->GetSortString(sortinfo);
    }

    query.Add(sortinfo.GetString());
}

//function to perform post-execute operations. used only for refresh operations
void 
DataView::OnQueryFinish(LPEXEC_INFO pexeinfo)
{
    wyInt32 result = pexeinfo->m_ret;

    //now if this function was called in response to refresh operation caused by client side sort/filter, then we simply return.
    //remember that reset filter may be enabled even there is no result set, lets say user stoped refresh operation and there is an existing filter. in that case we should complete this function.
    if((pexeinfo->m_la == LA_SORT || pexeinfo->m_la == LA_FILTER) && 
        m_data->m_psortfilter && m_data->m_psortfilter->m_isclient == wyTrue && CustomGrid_GetColumnCount(m_hwndgrid))
    {
        //reset the last clicked row
        m_lastclick = -1;
        return;
    }

    //if there is an error or the user stopped the query
    if(result == TE_STOPPED || result == TE_ERROR)
    {
        //reset the view and free the data
        ResetView(wyFalse);
        Execute(TA_CLEARDATA, wyFalse, wyTrue);
    }
    //esle we need to populate the grid, no matter what view is active we will populate the grid because all other views are just a layer over grid view
    else if(result == TE_SUCCESS)
    {
        //we may need to sort/filter if client side sorting/filtering is on
        if(m_data->m_psortfilter && m_data->m_psortfilter->m_isclient == wyTrue)
        {
            //check we have an existing sort/filter enabled, if so sort/filter it
            if((m_data->m_psortfilter->m_sort[0].m_colindex != -1 && m_data->m_psortfilter->m_sort[0].m_currsorttype != ST_NONE) ||
                (m_data->m_psortfilter->m_filter[0].m_currcolindex != -1 && m_data->m_psortfilter->m_filter[0].m_currfiltertype != FT_NONE))
            {
                m_data->m_psortfilter->ExecuteSortAndFilter();
            }
        }

        //persist limit
        m_data->HandleLimitPersistence(wyTrue);

        //populate grid view
        FillDataGrid();

#ifndef COMMUNITY
        //we have to refresh the form view also, if it is the selected view
        if(IsFormView())
        {
            m_formview->LoadForm(wyTrue);
            SetRefreshStatus(wyFalse, FORMVIEW_REFRESHED);
        }
#endif

        //persist the selected row/column, form view state and text view state
        SetViewState(pexeinfo);
    }

    //update the display
    ShowDataWindow(SW_SHOW);
    InvalidateRect(m_hwndshowtableinfo, NULL, TRUE);
    UpdateWindow(m_hwndshowtableinfo);
}

//subclassed window proc for text view
LRESULT	CALLBACK 
DataView::EditWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	DataView* dvptr = (DataView*)GetWindowLongPtr(hwnd, GWLP_USERDATA);	

    //handle find window
    if(message == (wyUInt32)pGlobals->m_pcmainwin->m_findmsg) 
	{		
		if(dvptr->m_findreplace->FindReplace(hwnd, lparam) == wyFalse)
		{
            //when we are closing the find dialog, deleting the memory allocated for Find
			delete(dvptr->m_findreplace);
			dvptr->m_findreplace = NULL;
		}

		return 0;		
	}
    
	switch(message)
	{
        //handle context menu
        case WM_CONTEXTMENU:
            if(dvptr->OnTextContextMenu(lparam) == 1)
            {
                return 1;
            }

	        break;

        //measure item for owner drawing context menu
        case WM_MEASUREITEM:
            return wyTheme::MeasureMenuItem((LPMEASUREITEMSTRUCT)lparam, dvptr->m_htrackmenu);

        //draw the menu item
    	case WM_DRAWITEM:		
            return wyTheme::DrawMenuItem((LPDRAWITEMSTRUCT)lparam);	

        //set focus
        case UM_FOCUS:
	        SetFocus(hwnd);
	        break;

        //update the line and column number in the status bar. 
        //here it post a message to the parent of frame window. the parent need to take apropriate action to update the status bar
        case WM_SETFOCUS:
        case WM_KEYUP:
        case WM_LBUTTONUP:
            PostMessage(GetParent(dvptr->m_hwndframe), UM_SETSTATUSLINECOL, (WPARAM)hwnd, 1);
            break;

        //clear the line and column number in the status bar. 
        //here it post a message to the parent of frame window. the parent need to take apropriate action to update the status bar
        case WM_KILLFOCUS:
            PostMessage(GetParent(dvptr->m_hwndframe), UM_SETSTATUSLINECOL, (WPARAM)NULL, 0);
	        break;

        //handle menu command
        case WM_COMMAND:
		    dvptr->OnEditWmCommand(wparam);
			return 1;

        case WM_SYSKEYDOWN:
            if(!dvptr->OnSysKeyDown(wparam, lparam))
            {
                return 0;
            }

            break;

        default:
	        break;
	}

    //call original window proc
	return CallWindowProc(dvptr->m_wptextview, hwnd, message, wparam, lparam);
}

//subclassed window proc for refresh tool bar
LRESULT	CALLBACK 
DataView::RefreshToolWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    DataView*   dvptr	= (DataView*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    HBRUSH      hbrush;
    DUALCOLOR   dc = {0};
    wyInt32     ret;

    //handle static window painting
    if(message == WM_CTLCOLORSTATIC && wyTheme::GetBrush(BRUSH_TOOLBAR, &hbrush))
    {
        SetBkMode((HDC)wparam, TRANSPARENT);

        if(wyTheme::GetDualColor(DUAL_COLOR_TOOLBARTEXT, &dc))
        {
            if(IsWindowEnabled(hwnd))
            {
                if(dc.m_flag & 1)
                {
                    SetTextColor((HDC)wparam, dc.m_color1);
                }
            }
            else
            {
                if(dc.m_flag & 2)
                {
                    SetTextColor((HDC)wparam, dc.m_color2);
                }
            }
        }

        return (LRESULT)hbrush;
    }
    //handle button caption painting for limit check box
    else if(message == WM_NOTIFY && wyTheme::GetDualColor(DUAL_COLOR_TOOLBARTEXT, &dc) && 
        (ret = wyTheme::PaintButtonCaption(lparam, &dc)) != -1)
    {
        return ret;
    }

    //call original window procedure
    return CallWindowProc(dvptr->m_refreshtoolproc, hwnd, message, wparam, lparam);
}

//function to change filter icon based on whether the filter is on or off
void 
DataView::ChangeFilterIcon()
{
    wyInt32 image, id, flag;
    wyBool  isfilteron;

    //is filter applied
	if(m_data && m_data->m_psortfilter && m_data->m_psortfilter->m_isfilter)
		isfilteron =  m_data->m_psortfilter->m_isfilter;
	else
		isfilteron = wyFalse;

    if(m_hwndrefreshtool)
    {
		if(isfilteron == wyTrue)
		{
			id = IMG_INDEX_RESET_FILTER;
			flag = GETIMG_IMG;
		}
		else
		{
			id = ID_RESETFILTER;
			flag = GETIMG_COMMAND;
		}

        //update filter icon
		image = GetImageIndex(id, flag, m_hwndrefreshtool, NULL);
        if(image != -1)
        {
            SendMessage(m_hwndrefreshtool, TB_CHANGEBITMAP, (WPARAM)ID_RESETFILTER, (LPARAM)(image));
        }

    }
    
}

//function to enable/disable controls
void 
DataView::EnableControls(wyBool isenable)
{
    //enable/disable toolbar
    EnableToolbar(isenable);

    //enable/disable other view windows
    EnableWindow(m_hwndgrid, isenable);
    EnableWindow(m_hwndtext, isenable);
#ifndef COMMUNITY
    if(m_formview)
    {
        EnableWindow(m_formview->m_hwndhtml, isenable);
    }
#endif
}

//function is called when execution starts
void 
DataView::OnExecutionStart(ThreadAction ta, LPTHREAD_PARAM ptp)
{
    LPEXEC_INFO lpexec;

    CustomGrid_ApplyChanges(m_hwndgrid);

    //push the current execution info to stack and get the previous top element
    lpexec = GetExecInfoFromStack(&ptp->m_execinfo, wyTrue);

    //if there is no previous exec info
    if(!lpexec)
    {
        //if it is not refresh operation or if it is not because of client side sort/filter
        if(ta != TA_REFRESH || !m_data->m_psortfilter || m_data->m_psortfilter->m_isclient == wyFalse ||
            ptp->m_execinfo.m_la != LA_SORT || ptp->m_execinfo.m_la != LA_FILTER)
        {
            //inform the parent about execution start
            PostMessage(m_hwndparent, UM_STARTEXECUTION, 0, 0);

            //set variables
			m_wnd->SetExecuting(wyTrue);
	        ThreadStopStatus(0);
        }
    }
    
    switch(ta)
    {
        //handle crer data
        case TA_CLEARDATA:
            //save the current state and reset the view
            SaveViewState();
            ResetView(wyFalse);
            break;

        //handle insert/update
        case TA_INSERT:
        case TA_UPDATE:
            //reset all the buttons and disable the view windows
            ResetToolBarButtons();
            EnableControls(wyFalse);

            //enable the toolbar button and change the icon to stop
            EnableToolButton(wyTrue, ID_RESULT_SAVE);
            ChangeToolButtonIcon(wyTrue, ID_RESULT_SAVE);
            break;

        //handle delete
        case TA_DELETE:
            //reset all the buttons and disable the view windows
            ResetToolBarButtons();
            EnableControls(wyFalse);

            //enable the toolbar button and change the icon to stop
            EnableToolButton(wyTrue, ID_RESULT_DELETE);
            ChangeToolButtonIcon(wyTrue, ID_RESULT_DELETE);
            break;

        //handle refresh
        case TA_REFRESH:
            //reset all the buttons and disable the view windows
            ResetToolBarButtons();
            EnableControls(wyFalse);

            //enable the toolbar button and change the icon to stop
            EnableToolButton(wyTrue, IDC_REFRESH);
            ChangeToolButtonIcon(wyTrue, IDC_REFRESH);
            break;
    }
}

//function is called when execution finished
void 
DataView::OnExecutionEnd(ThreadAction ta, LPTHREAD_PARAM ptp)
{
    DWORD           lastid	= (DWORD)m_wnd->m_tunnel->mysql_insert_id(m_wnd->m_mysql);
    wyChar          last[30] = {0};
    LPEXEC_INFO     lpexeinfo;
    wyInt32         id = -1, row;
    RECT            cellrect = {0};

    //get last execution info
    lpexeinfo = GetExecInfoFromStack(&ptp->m_execinfo, wyFalse);

    switch(ta)
    {
        //handle insert finsish
        case TA_INSERT:    
            //reset the toolbar buttons
            ResetToolBarButtons();

            //if insert was successful
            if(ptp->m_execinfo.m_ret == TE_SUCCESS)
            {
                //now if we have an autoinc column
                if(m_data->m_autoinccol != -1)
		        {
                    //update the value in the autoinc column
			        sprintf(last, "%ld", lastid);
                    row = CustomGrid_GetCurSelRow(m_hwndgrid);
                    CustomGrid_SetText(m_hwndgrid, row, m_data->m_autoinccol, last);
                }    

                //show warnings if any
                HandleRowModified(-2);

                //if the last action was rowchange, then we need to set the row
                if(ptp->m_execinfo.m_la == LA_ROWCHANGE)
                {
                    CustomGrid_SetCurSelection(m_hwndgrid, ptp->m_execinfo.m_wparam, ptp->m_execinfo.m_lparam, wyTrue);
                    CustomGrid_GetSubItemRect(m_hwndgrid, ptp->m_execinfo.m_wparam, ptp->m_execinfo.m_lparam, &cellrect);

                    if(cellrect.left == cellrect.right && cellrect.bottom == cellrect.top && cellrect.left == 0)
                    {
                        CustomGrid_EnsureVisible(m_hwndgrid, ptp->m_execinfo.m_wparam, ptp->m_execinfo.m_lparam, wyTrue);
                    }
                    else
                    {
                        CustomGrid_EnsureVisible(m_hwndgrid, ptp->m_execinfo.m_wparam, ptp->m_execinfo.m_lparam, wyFalse);
                    }
                }
                //else we are getting the current selected row and setting the current selected row
                //this is required for updating modules that depends on grid view
                else
                {
                    row = CustomGrid_GetCurSelRow(m_hwndgrid);
                    CustomGrid_SetCurSelRow(m_hwndgrid, row);
                }

                RefreshActiveDispWindow();
	    	}	

            break;

        //handle update operation
        case TA_UPDATE:
            //reset the toolbar buttons
            ResetToolBarButtons();

            //if operationn was successful
            if(ptp->m_execinfo.m_ret == TE_SUCCESS)
            {
                //show any warnings
                HandleRowModified(-2);
            
                //if the update was caused by row change notification, we need to set the row and column
                if(ptp->m_execinfo.m_la == LA_ROWCHANGE)
                {
                    CustomGrid_SetCurSelection(m_hwndgrid, ptp->m_execinfo.m_wparam, ptp->m_execinfo.m_lparam, wyTrue);
                    CustomGrid_GetSubItemRect(m_hwndgrid, ptp->m_execinfo.m_wparam, ptp->m_execinfo.m_lparam, &cellrect);

                    if(cellrect.left == cellrect.right && cellrect.bottom == cellrect.top && cellrect.left == 0)
                    {
                        CustomGrid_EnsureVisible(m_hwndgrid, ptp->m_execinfo.m_wparam, ptp->m_execinfo.m_lparam, wyTrue);
                    }
                    else
                    {
                        CustomGrid_EnsureVisible(m_hwndgrid, ptp->m_execinfo.m_wparam, ptp->m_execinfo.m_lparam, wyFalse);
                    }

                    RefreshActiveDispWindow();
                }
            }

            break;

        //handle delete
        case TA_DELETE:
            //reset the toolbar buttons
            ResetToolBarButtons();

            row = CustomGrid_GetCurSelRow(m_hwndgrid);

            //there may be chances that the current selected row in the grid no more valid after delete operation
            //so if the current selected row is more that the row count, we need to adjust it
            if(row >= m_data->m_rowarray->GetLength())
            {
                CustomGrid_SetCurSelRow(m_hwndgrid, m_data->m_rowarray->GetLength() - 1, wyFalse);
                RefreshActiveDispWindow();
            }

            break;

        //handle refresh operation
        case TA_REFRESH:
            //reset toolbar buttons
            ResetToolBarButtons();
            
            //the operation might be a resut of server side sort/filter
            if(m_data->m_psortfilter && m_data->m_psortfilter->m_isclient == wyFalse)
            {
                //if it is sort, we need to finish the sort
                if(ptp->m_execinfo.m_la == LA_SORT)
                {
                    m_data->m_psortfilter->EndColumnSort(ptp->m_execinfo.m_ret == TE_SUCCESS ? wyTrue : wyFalse);
                }
                //finish filter
                else if(ptp->m_execinfo.m_la == LA_FILTER)
                {
                    m_data->m_psortfilter->EndFilter(ptp->m_execinfo.m_ret == TE_SUCCESS ? wyTrue : wyFalse);
                    
                    //update filter icon
                    ChangeFilterIcon();
                }
            }

            //call the generic query finish
            OnQueryFinish(&ptp->m_execinfo);
            break;
    }

    //if last action was given as a callback, the calling function expects the function to be invoked
    if(ptp->m_execinfo.m_la == LA_CALLBACK)
    {
        ((LACALLBACK)(ptp->m_execinfo.m_lparam))(&ptp->m_execinfo);
    }

    //if the execution stack is empty
    if(!lpexeinfo)
    {
        //reset the variables and enable the controls
		m_wnd->SetExecuting(wyFalse);
        ThreadStopStatus(1);
        EnableControls(wyTrue);

        //inform the parent about the execution finish
        PostMessage(m_hwndparent, UM_ENDEXECUTION, m_data ? m_data->GetSavedRowCount() : 0, 
            (LPARAM)(ptp->m_hwndfocus ? ptp->m_hwndfocus : GetActiveDispWindow()));
    }
    //else we need to switch back to the previous operation
    //for example, a refresh operation will check whether the any unsaved changes are there in the grid, if so it should finish that.
    //so, first it will change the refresh icon to stop, and then it changed the save icon to stop, now once save completed we have to change the refresh icon to stop
    else
    {
        switch(lpexeinfo->m_ta)
        {
            case TA_INSERT:
            case TA_UPDATE:
                id = ID_RESULT_SAVE;
                break;

            case TA_DELETE:
                id = ID_RESULT_DELETE;
                break;

            case TA_REFRESH:
                id = IDC_REFRESH;
                break;
        }

        //enable the button and change the icon to stop
        if(id != -1)
        {
            EnableToolButton(wyTrue, id);
            ChangeToolButtonIcon(wyTrue, id);
        }
    }

    //now there is a possibility that the refresh operation caused by client side sort/filter
    if(ptp->m_execinfo.m_ta == TA_REFRESH && m_data->m_psortfilter && 
        m_data->m_psortfilter->m_isclient == wyTrue)
    {
        //if execution was successful
        if(ptp->m_execinfo.m_ret == TE_SUCCESS)
        {
            //perform sort
            if(ptp->m_execinfo.m_la == LA_SORT)
            {
                PostMessage(m_hwndframe, UM_DOCLIENTSIDESORT, wyTrue, wyTrue);
            }
            //perform filter
            else if(ptp->m_execinfo.m_la == LA_FILTER)
            {
                PostMessage(m_hwndframe, UM_DOCLIENTSIDEFILTER, wyTrue, wyTrue);
            }
        }
        else
        {
            //rest sort to its old state
            if(ptp->m_execinfo.m_la == LA_SORT)
            {
                m_data->m_psortfilter->EndColumnSort(wyFalse);
            }
            //reset filter to old state
            else if(ptp->m_execinfo.m_la == LA_FILTER)
            {
                m_data->m_psortfilter->EndFilter(wyFalse);
                ChangeFilterIcon();
            }
        }
    }
}

//function to push/pop execution elem to stck. we need a execution stack since an operation can nest another operation
LPEXEC_INFO 
DataView::GetExecInfoFromStack(LPEXEC_INFO pexecinfo, wyBool ispush)
{
    wyElem* pelem;

    //get the topmost element
    pelem = m_execelemstack.GetLast();

    if(ispush == wyTrue)
    {
        //push the element
        m_execelemstack.Insert(new ExecutionElem(*pexecinfo));
    }
    else
    {
        //the loop removes all the elements with a different thread action compared to the given elemetn from the end
        for(; pelem; pelem = m_execelemstack.GetLast())
        {
            m_execelemstack.Remove(pelem);

            //if it is the same, break from the loop
            if(pexecinfo && ((ExecutionElem*)pelem)->m_execinfo.m_ta == pexecinfo->m_ta)
            {
                delete pelem;
                pelem = m_execelemstack.GetLast();
                break;
            }   

            delete pelem;
        }
    }

    return pelem ? &(((ExecutionElem*)pelem)->m_execinfo) : NULL;
}

//function to set buffered drawing on/off
void 
DataView::SetBufferedDrawing(wyBool isbuffered)
{
    m_isbuffereddrawing = isbuffered;
}

//function gets the banner text/watermark that will be shown in the absense of data.
void 
DataView::GetBanner(wyString& bannertext)
{
    bannertext.Clear();
}

//frame window proc
LRESULT	CALLBACK 
DataView::FrameWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	DataView*           dvptr = (DataView*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	wyString            msg, *pquery;
	wyInt32*            presult;
	wyInt32             dup, temp, id, ret;
    LPTHREAD_MSG_PARAM  lptmp = (LPTHREAD_MSG_PARAM)lparam;
    HDC		            hdc;
    PAINTSTRUCT         ps;
    RECT                rect, rectclient;
    POINT               pt;
    HBRUSH              hbr;
    COLORREF            fg, bg;
    wyString            bannertext;
    MYSQL_ROWEX*        prowex;
    
    switch(message)
	{
        //blocked to avoid flickering
        case WM_ERASEBKGND:
            return 1;

        //handle paing
        case WM_PAINT:
            //if buffered drawing is on
            if(dvptr->m_isbuffereddrawing == wyTrue)
            {
                hdc = BeginPaint(hwnd, &ps);
                DoubleBuffer db(hwnd, hdc);
                db.EraseBackground(RGB(255, 255, 255));
                GetWindowRect(dvptr->m_hwndgrid, &rect);
                MapWindowRect(NULL, hwnd, &rect);
                SetBkMode(db.m_buffer.m_hmemdc, TRANSPARENT);
                SetTextColor(db.m_buffer.m_hmemdc, GetSysColor(COLOR_GRAYTEXT));

                //get the banner text and draw it
                dvptr->GetBanner(bannertext);
                DrawText(db.m_buffer.m_hmemdc, bannertext.GetAsWideChar(), -1, &rect, DT_VCENTER | DT_CENTER | DT_SINGLELINE);

                GetClientRect(hwnd, &rectclient);
                rectclient.top = rect.bottom;

                //paint the bottom ribbon
                dvptr->GetRibbonColorInfo(&bg, &fg);
                hbr = CreateSolidBrush(bg);
                FillRect(db.m_buffer.m_hmemdc, &rectclient, hbr);
                DeleteBrush(hbr);

                //paint the windows
                db.PaintWindow();
                EndPaint(hwnd, &ps);

            }
            else
            {
                hdc = BeginPaint(hwnd, &ps);

                //erase the background
                DoubleBuffer::EraseBackground(hwnd, hdc, NULL, RGB(255, 255, 255));

                GetWindowRect(dvptr->m_hwndgrid, &rect);
                MapWindowRect(NULL, hwnd, &rect);
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));

                //get the banner text and draw it
                dvptr->GetBanner(bannertext);
                DrawText(hdc, bannertext.GetAsWideChar(), -1, &rect, DT_VCENTER | DT_CENTER | DT_SINGLELINE);

                GetClientRect(hwnd, &rectclient);
                rectclient.top = rect.bottom;

                //paint the bottom ribbon
                dvptr->GetRibbonColorInfo(&bg, &fg);
                hbr = CreateSolidBrush(bg);
                FillRect(hdc, &rectclient, hbr);
                DeleteBrush(hbr);

                EndPaint(hwnd, &ps);
            }

            return 1;

        //handle context menu
        case WM_CONTEXTMENU:
            pt.x = GET_X_LPARAM(lparam);
            pt.y = GET_Y_LPARAM(lparam);
            GetWindowRect(dvptr->m_hwndgrid, &rect);

            //since this is the parent window and if the child window is disabled, this window will receive the message meant for the child
            //so we are checking wether the message happened on the grid area, only then we will show context menu
            if(PtInRect(&rect, pt))
            {           
                //if the view is text, show text view context menu
                if(dvptr->m_viewtype == TEXT)
                {
                    dvptr->OnTextContextMenu(lparam);
                }
                //else show the normal context menu
                else if(!dvptr->GetActiveDispWindow())
                {
                    dvptr->ShowContextMenu(-1, -1, &pt);
                }
            }

            return 1;

        //measure the size of the menu item
        case WM_MEASUREITEM:
            return wyTheme::MeasureMenuItem((LPMEASUREITEMSTRUCT)lparam, dvptr->m_htrackmenu);

        //draw menu item
	    case WM_DRAWITEM:		
            return wyTheme::DrawMenuItem((LPDRAWITEMSTRUCT)lparam);	

        //handle various commands
		case WM_COMMAND:
            dvptr->OnWMCommand(wparam, lparam);
			break;

        //handle help
        case WM_HELP:
	        dvptr->ShowHelpFile();
            return 1;

        //message sent by DoubleBuffer class while drawing windows. if a window's background is transparent then it should set the lparam to 1, so that it will be ignored while painting
        case UM_ISIGNOREHWNDFROMPAINT:
            id = GetDlgCtrlID((HWND)wparam);

            if(id == IDC_ROW_MSG || id == IDC_WARN_MSG)
            {
                *((wyInt32*)lparam) = 1;
            }

            return 1;

        //handle statci window painting
        case WM_CTLCOLORSTATIC:
			hdc = (HDC)wparam;
			id = GetDlgCtrlID((HWND)lparam);

            //warning links at the bottom
			if(id == IDC_ROW_MSG || id == IDC_WARN_MSG)  
			{				
                //select the font and set the bg mode
				SelectObject(hdc, dvptr->m_modifiedfont);
				SetBkMode(hdc, TRANSPARENT);

                //get the hyperlink color and set the color
                dvptr->GetHyperLinkColorInfo(&fg);
				SetTextColor(hdc, fg);

				return (BOOL)GetStockObject(HOLLOW_BRUSH);	
			}
            else if((HWND)lparam == dvptr->m_hwndpadding)
            {
                wyTheme::GetBrush(BRUSH_TOOLBAR, &hbr);
                return (BOOL)hbr;	
            }

			break;

        //handle thread finish. the message is from the worker thread once it is about to finish
		case UM_THREADFINISHED:
			dvptr->OnUMThreadFinish(wparam, lparam);
            return 1;

        //show a message box asking for users confirmation regarding multiple rows updated/deleted
		case UM_UPDATEWARNING:
        case UM_DELETEWARNING:
			presult = (wyInt32*)lptmp->m_lparam;
			dup = (wyInt32)wparam;
            msg.Sprintf(_("There are %d duplicates of the row you are trying to %s. Do you want to %s all the duplicates?\n\nNote: You can turn off this warning by unchecking Tools -> Preferences -> Others -> Prompt if multiple rows are getting updated"), 
				  dup, (message == UM_UPDATEWARNING) ? _("update") : _("delete"), (message == UM_UPDATEWARNING) ? _("update") : _("delete"));
            *presult = MessageBox(hwnd, msg.GetAsWideChar(), _(L"Warning"), MB_YESNOCANCEL | MB_ICONQUESTION);
			SetThreadMessageEvent(lparam);
			break;

        //show mysql error
        case UM_MYSQLERROR:
			pquery = (wyString*)lptmp->m_lparam;
            ShowMySQLError(hwnd, dvptr->m_wnd->m_tunnel, &dvptr->m_wnd->m_mysql, pquery->GetString());
			SetThreadMessageEvent(lparam);
			break;

        //ask the user to confirm whether to save or not any unsaved changes in the grid
        case UM_CONFIRMREFRESH:
            presult = (wyInt32*)lptmp->m_lparam;
            *presult = MessageBox(hwnd, _(L"Do you want to save the changes?"), _(L"Warning"), MB_YESNOCANCEL | MB_ICONQUESTION);
            SetThreadMessageEvent(lparam);
            return 1;

        //confirm delete rows
        case UM_CONFIRMDELETE:
            presult = (wyInt32*)lptmp->m_lparam;

            //if wparam is non zero, we are going to delete multiple rows
            if(wparam)
            {
                *presult = MessageBox(hwnd, _(L"Do you really want to delete the selected row(s)?"), 
                    _(L"Warning"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
            }
            else
            {
                *presult = MessageBox(hwnd, _(L"Do you really want to delete the currently selected row?"), 
                    _(L"Warning"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
            }
             
            SetThreadMessageEvent(lparam);
            return 1;

        //delete a row, this is posted from the worker thread. the deletion SHOULD be done in the gui thread to avoid any crashes happening because of the invalid pointer refrennce in grid painting
        case UM_DELETEROW:
            dvptr->FreeRow(wparam, (wyBool)lptmp->m_lparam);
            SetThreadMessageEvent(lparam);
            return 1;

        //get the current selected row
        case UM_GETCURSELROW:
            presult = (wyInt32*)lptmp->m_lparam;
            *presult = CustomGrid_GetCurSelRow(dvptr->m_hwndgrid);
            SetThreadMessageEvent(lparam);
            return 1;

        //get the error flag state
        case UM_ERRFLAGSTATE:
            presult = (wyInt32*)lptmp->m_lparam;
            temp = dvptr->ErrorFlagState(wparam);

            if(presult)
            {
                *presult = temp;
            }

            SetThreadMessageEvent(lparam);
            return 1;

        //posted when the thread started to perfrom an operation
        case UM_STARTEXECUTION:
            dvptr->OnExecutionStart((ThreadAction)wparam, (THREAD_PARAM*)lptmp->m_lparam);
            SetThreadMessageEvent(lparam);
            return 1;

        //posted when the thread finished an operation
        case UM_ENDEXECUTION:
            dvptr->OnExecutionEnd((ThreadAction)wparam, (THREAD_PARAM*)lptmp->m_lparam);
            SetThreadMessageEvent(lparam);
            return 1;

        //perform client side sort/filter
        case UM_DOCLIENTSIDESORT:
        case UM_DOCLIENTSIDEFILTER:
            SetCursor(LoadCursor(NULL, IDC_WAIT));

            //perform sort/filter
            dvptr->m_data->m_psortfilter->ExecuteSortAndFilter();

            SetCursor(LoadCursor(NULL, IDC_ARROW));

            //update the rowcount
            SendMessage(dvptr->m_hwndparent, UM_SETROWCOUNT, dvptr->m_data->GetSavedRowCount(), 0);

            //if wparam is set, then only we need to finish sort/filter
            if(wparam)
            {
                if(message == UM_DOCLIENTSIDESORT)
                {
                    //end sort, so that we have the new sort in place
                    dvptr->m_data->m_psortfilter->EndColumnSort((wyBool)lparam);
                }
                else
                {
                    //end filter so that we have the new filter in place
                    dvptr->m_data->m_psortfilter->EndFilter((wyBool)lparam);

                    //change the filter icon
                    dvptr->ChangeFilterIcon();

                    //set row count to be displayed
                    CustomGrid_SetMaxRowCount(dvptr->m_hwndgrid, dvptr->m_data->m_rowarray->GetLength());

                    //try to keep the current selected row as it is, but make sure we are inside row count
                    temp = max(min(CustomGrid_GetCurSelRow(dvptr->m_hwndgrid), dvptr->m_data->m_rowarray->GetLength() - 1), 0);

                    //now if the row index we calculated is for a new row, we are setting the index to previous row available.
                    //this is required because if we filter on a value in a row users like to see the selection remaining on a row the filter condition matched
                    if((prowex = dvptr->m_data->m_rowarray->GetRowExAt(temp)) && prowex->IsNewRow())
                    {
                        temp = max(temp - 1, 0);
                    }
                    
                    //adjust the scroll bar limits so that everything look smooth
                    id = CustomGrid_GetInitCol(dvptr->m_hwndgrid);
                    CustomGrid_SetInitRow(dvptr->m_hwndgrid, 0, wyFalse);
                    CustomGrid_SetCurSelRow(dvptr->m_hwndgrid, temp);
                    CustomGrid_EnsureVisible(dvptr->m_hwndgrid, temp, id);
                }
            }

            //refresh the view
            dvptr->SetRefreshStatus();
            dvptr->ShowDataWindow(SW_SHOW);
            dvptr->RefreshActiveDispWindow();
            return 1;

        //handle notifications
        case WM_NOTIFY:
            //draw the toolbar with the theme color
            if((ret = wyTheme::DrawToolBar(lparam)) != -1)
            {
                return ret;
            }

            //handle toolbar drop down notification and show the menu
            if(((LPNMHDR)lparam)->code == TBN_DROPDOWN)
		    {
			    dvptr->HandleToolBarDropDown(((LPNMTOOLBAR)lparam));
			    return TBDDRET_DEFAULT;
		    }
            //handle tooltip notifcation and show proper tooltip
            else if(((LPNMHDR)lparam)->code == TTN_GETDISPINFO)
            {
                dvptr->OnToolTipInfo((LPNMTTDISPINFO)lparam);
                return 1;
            }

            break;

        //show message box whenever query generation failed, for inset/update/count(*) etc
        case UM_QUERYGENERATIONFAILED:
            //form the message
            msg.Sprintf(_("Failed to generate %s query"), wparam == TA_INSERT ? "insert" : "update");
            
            //add additional information
            if(lptmp->m_lparam)
            {
                msg.AddSprintf("\r\n\r\n%s", lptmp->m_lparam);
            }

            //show message
            MessageBox(dvptr->m_hwndframe, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
            SetThreadMessageEvent(lparam);
            return 1;

        //we update the refresh control values(first row, limit etc) once it is ready to refresh. 
        case UM_SETREFRESHVALUES:
            dvptr->SetRefreshValues((LPEXEC_INFO)lptmp->m_lparam);
            SetThreadMessageEvent(lparam);
            return 1;

        //cancel changes
        case UM_CANCELCHANGES:
            dvptr->CancelChanges();
            SetThreadMessageEvent(lparam);
            return 1;
    }

	return DefWindowProc(hwnd, message, wparam, lparam);
}

//function get or set error flag states
wyInt32
DataView::ErrorFlagState(wyInt32 state)
{
    //if state is < 0 then we are retreiving the current flag state
    if(state <= -1)
    {
        if(IsWindowVisible(m_hwndmodified))
        {
            return EFS_MODIFIED;
        }
        else if(IsWindowVisible(m_hwndwarning))
        {
            return EFS_WARNING;
        }

        return EFS_NONE;
    }

    //hide both warnings
    ShowWindow(m_hwndmodified, SW_HIDE);
    ShowWindow(m_hwndwarning, SW_HIDE);

    //now enable the warning, and based on that the toolbar buttons
    if(state == EFS_MODIFIED)
    {
        ShowWindow(m_hwndmodified, SW_SHOW);
        EnableToolButton(wyFalse, IDM_DUPLICATE_ROW);
        EnableToolButton(wyTrue, ID_RESULT_CANCEL);
        EnableToolButton(wyTrue, ID_RESULT_SAVE);

    }
    else if(state == EFS_WARNING)
    {
        ShowWindow(m_hwndwarning, SW_SHOW);
    }
    else
    {
        state = EFS_NONE;
        EnableToolButton(IsCurrRowDuplicatable(), IDM_DUPLICATE_ROW);
        EnableToolButton(wyFalse, ID_RESULT_CANCEL);
        EnableToolButton(wyFalse, ID_RESULT_SAVE);
    }
    
    return state;
}

//function checks whether the current row can be duplicated
wyBool 
DataView::IsCurrRowDuplicatable()
{
    wyInt32 currow, rowcount;

    //if there is no data or the row count is zero, or we have an invalid current row or the current row is modified or the current row is a new row
    if(!m_data || !(rowcount = m_data->m_rowarray->GetLength()) || 
        (currow = CustomGrid_GetCurSelRow(m_hwndgrid)) >= rowcount ||
        currow < 0 || m_data->m_modifiedrow == currow ||
        m_data->m_rowarray->GetRowExAt(currow)->IsNewRow())
    {
        return wyFalse;
    }
    
    //if it can be ediited
#ifndef COMMUNITY
	if (m_data->m_db.GetLength() && m_data->m_table.GetLength() && GetActiveWin()->m_conninfo.m_isreadonly == wyFalse)
	{
		return wyTrue;
	}
#else
	if (m_data->m_db.GetLength() && m_data->m_table.GetLength())// && GetActiveWin()->m_conninfo.m_isreadonly == wyFalse)
	{
		return wyTrue;
	}
#endif // !COMMUNITY



    return wyFalse;
}

//very import function used to send cross thread messages. worker thread should communicate with the UI thread using this function
void 
DataView::SendThreadMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    THREAD_MSG_PARAM tmp = {0};

    //set the lparam sent
    tmp.m_lparam = lparam;

    //check whether the calling thread is gui thread, if so use send message
    if(GetWindowThreadProcessId(hwnd, NULL) == GetCurrentThreadId())
    {
        SendMessage(hwnd, message, wparam, (LPARAM)&tmp);
    }
    else
    {
        //create an event to synchronize the worker thread and ui thread
        tmp.m_hevent = CreateEvent(NULL, TRUE, FALSE, NULL);

        //now post the message to ui thread and wait for the event to be set
        PostMessage(hwnd, message, wparam, (LPARAM)&tmp);
        WaitForSingleObject(tmp.m_hevent, INFINITE);

        //close the event handle
        CloseHandle(tmp.m_hevent);
    }
}

//function to signal the event for messages sent using SendThreadMessage
void
DataView::SetThreadMessageEvent(LPARAM lparam)
{
    LPTHREAD_MSG_PARAM lptmp = (LPTHREAD_MSG_PARAM)lparam;

    //if there is a valid event handle, then signal the event. if the message was sent using SendMessage, then event handle will be NULL
    if(lptmp && lptmp->m_hevent)
    {
        SetEvent(lptmp->m_hevent);
    }
}

//function to close the thread handle
void
DataView::OnUMThreadFinish(WPARAM wparam, LPARAM lparam)
{
    HANDLE hthread = (HANDLE)lparam;

    //precaution
    if(hthread)
    {
        //since the message is posted from the thread, we should wait for the thread to finish
        WaitForSingleObject(hthread, INFINITE);

        //close the handle
        CloseHandle(hthread);
    }
}

//atomic function to check thread stop status
wyInt32 
DataView::ThreadStopStatus(wyInt32 stop)
{
    wyInt32 temp;

    EnterCriticalSection(&m_cs);

    //if stop is < 0 then we are retreiving the thread stop status
    if(stop < 0)
    {
        temp = m_stop;
    }
    //esle set the status
    else
    {
        m_stop = temp = stop;
    }

    LeaveCriticalSection(&m_cs);

    return temp;
}

//function to stop execution
wyBool
DataView::Stop(wyInt32 buttonid)
{
	wyInt32 presult = 6;
#ifndef COMMUNITY
	if(pGlobals->m_entlicense.CompareI("Professional") != 0 && m_wnd->m_ptransaction && !m_wnd->m_ptransaction->m_starttransactionenabled)
	{
		presult = MessageBox(m_wnd->m_pcqueryobject->m_hwnd, _(L"The session has an active transaction. Stopping the execution will cause transaction to rollback and end. Do you want to continue?"), 
                    _(L"Warning"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
		if(presult == 6)
			m_wnd->m_ptransaction->CallOnCommit();
	}
	if(presult != 6)
		return wyFalse; //dosent matter for professional, as initial value of presult is 6	
#endif
    //set the thread stop status and disable the button
    ThreadStopStatus(1);
    EnableToolButton(wyFalse, buttonid);

    //stop execution
    if(m_wnd->m_tunnel->IsTunnel())
    {
		m_wnd->StopQueryExecution();
    } 
    else
    { 
		m_wnd->StopQuery();
    }

   	return wyTrue;
}

//functon to show the data view
void
DataView::ShowAll(wyInt32 show)
{
    //save view state before hiding
    if(show == SW_HIDE)
    {
        SaveViewState();
    }

    //show/hide window
    ShowWindow(m_hwndframe, show);
}

//function to get the display info for grid
wyInt32
DataView::OnGvnGetDispInfo(WPARAM wparam, LPARAM lparam)
{	
	wyUInt32        len = 0;
	wyString	    cellvaluestr, dispstr;
	const wyChar*   value = NULL;
	wyWChar*        cellvalwchar = NULL ;
	wyString        codepage, binstr;
	wyUInt32	    length = 0;
    wyBool          isblob = wyFalse, isbinary = wyFalse, isbit = wyFalse;
	GVDISPINFO*     disp = (GVDISPINFO*)wparam;

    //precaution
	if(!m_data) 
    {
		return 0;
    }

	//check for bit type
	if(lparam)
	{
		isbit = IsBin(disp->nCol);
	}

    //get cell value
    value = GetCellValue(disp->nRow, disp->nCol, &len, wyFalse, isbit == wyTrue ? &binstr : NULL);

	//check for blob type
	isblob = IsBlob(disp->nCol);
		
	//handle bit type
	if(isbit == wyTrue)
	{		
		length = binstr.GetLength();		
		strcpy(disp->text, binstr.GetString());
		return 0;
	}	
    //handle NULL
	else if(!value ) 
	{
		strcpy(disp->text, STRING_NULL);
		
        //copy the size too
		if(disp->pszButtonText) 
        {
    		sprintf(disp->pszButtonText, "0K");
        }
	}	
	else
	{    
        //if we dont have NULL in our value
		if(!memchr(value, NULL, len))
        {
            //set the string with proper encoding
		    cellvaluestr.SetAs(value, m_wnd->m_ismysql41);

            //set the value and length. this is required since the value and len may change based on encoding selected
            value = cellvaluestr.GetString();
            len = cellvaluestr.GetLength();
        }
        //if it is auto inc column
		else if(!len && !strcmp(value, "(Auto)"))
		{
			cellvaluestr.SetAs("(Auto)");
		}
        //now, it is a binary/text type. use SetAsDirect to avoid encoding issues 
		else 
		{
			cellvaluestr.SetAsDirect(value, isblob == wyTrue && len > 64 ? 64 : len);			
		}			
				
        //now get as wide char string
		cellvalwchar = cellvaluestr.GetAsWideChar(&length);

        //if we dont have NULL in our string
		if(!wmemchr(cellvalwchar, NULL, length))
		{
			if(!memchr(value, NULL, len))
			{								
				if(isblob == wyTrue)
				{
					dispstr.SetAsDirect(value, len > 64 ? 64 : len);
					cellvaluestr.SetAs(dispstr);
															
					//function copies the Codepage of the data to be displayed(only 64 chars)
					ExamineData(codepage, cellvaluestr);
														
					if(codepage.CompareI("US-Ascii") != 0 && codepage.CompareI("Unicode (utf-8)") != 0 && m_wnd->m_ismysql41 == wyFalse)
                    {
                        cellvaluestr.SetAs(value, wyFalse);
                    }
				}

				strncpy(disp->text, cellvaluestr.GetString(), disp->cchTextMax - 1);
			}
            //its binary
			else
			{				
				strncpy(disp->text, BINARY_DATA, disp->cchTextMax - 1);
				isbinary = wyTrue;
			}
		}
		else
		{
			if(isblob)
			{
			    strncpy(disp->text, BINARY_DATA, disp->cchTextMax - 1);
			    isbinary = wyTrue;
			}
			else
			{
				strncpy(disp->text, cellvaluestr.GetString(), disp->cchTextMax - 1);
				isbinary = wyFalse;
			}
		}
		
		/// also copy the size
		if(disp->pszButtonText) 
		{
			//if len < 1000  ---> bytes, len/1000 <1000 ---> Kb, len /(1000*1000) <1000 --> MB, len/(1000*1000) >1000 --> GB.
			//for getting the rounded value, first we are dividing the number by 1000, then take the integer part of that value.
			sprintf(disp->pszButtonText, "%d%s", 
                (len<1000)?(len):
                ((len/1000)< 1000)?(wyUInt32)(((wyDouble)len/1000)):
                (((len/(1000*1000))< 1000)?(wyUInt32)((wyDouble)len/(1000*1000)):(wyUInt32 )(((wyDouble)len/(1000*1000*1000)))),
                (len<1000)?("B"):(((len/1000)< 1000)?("K"):(((len/(1000*1000))< 1000)?("M"):("G"))));				
		}		
	}
    
	return (isbinary == wyTrue ? 1 : 0);
}

//get the disp info length, so that the grid can allocate reqired buffer to get disp info
wyInt32  
DataView::OnGvnGetDispLenInfo(WPARAM wparam)
{
	wyUInt32		len = 0;
	wyString		cellvalstr, bitstr;
	const wyChar*   value = {0};
	GVDISPINFO*     disp = (GVDISPINFO *)wparam;

    //get the value
	value = GetCellValue(disp->nRow, disp->nCol, &len, wyFalse, &bitstr);

	if(value)
	{
        //if we dont have a null in our string
		if(!memchr(value, NULL, len))
		{
            cellvalstr.SetAs(value, m_wnd->m_ismysql41);
			value = cellvalstr.GetString();
            len = cellvalstr.GetLength();
        }
        else
        {
            cellvalstr.SetAs(value);
        }
		
		if(!memchr(value, NULL, len))
        {
			disp->cchTextMax = cellvalstr.GetLength() + 1;
        }
		else
        {
			disp->cchTextMax = strlen(BINARY_DATA) + 1;
        }

	    if(len)
        {
		    return 1;
        }
	}
	else if(bitstr.GetLength())
	{
		disp->cchTextMax = bitstr.GetLength() + 1 + 3;
	}

	return 0;
}

void
DataView::OnGvnGetDispInfoData(WPARAM wparam)
{
	wyUInt32    len = 0;
	wyChar*     value = {0};
	wyString	cellvalstr, bitstr;
	GVDISPINFO* disp = (GVDISPINFO *)wparam;
	
	value = GetCellValue(disp->nRow, disp->nCol, &len, wyFalse, &bitstr);

	if(!value && bitstr.GetLength())
	{
		if(!wmemchr(bitstr.GetAsWideChar(), NULL, len))
        {
			memcpy(disp->text, bitstr.GetString(), disp->cchTextMax - 1);
        }
	}

	if(value)
	{
        cellvalstr.SetAs(value, m_wnd->m_ismysql41);
		cellvalstr.GetAsWideChar(&len);

		if(!wmemchr(cellvalstr.GetAsWideChar(), NULL, len))
        {
			memcpy(disp->text, cellvalstr.GetString(), disp->cchTextMax - 1);
        }
	}

	return;
}

//function checks whether the clumn is bit type
wyBool
DataView::IsBin(wyInt32 col)
{
	MYSQL_FIELD* myfield;

    //get the field struct
	myfield = m_wnd->m_tunnel->mysql_fetch_fields(m_data->m_datares);

	if(myfield && myfield[col].type == MYSQL_TYPE_BIT)
	{
		return wyTrue;
	}

	return wyFalse;
}

//check if the column is blob
wyBool 
DataView::IsBlob(wyInt32 col)
{
	MYSQL_FIELD* myfield;

    //get the field struct
	myfield = m_wnd->m_tunnel->mysql_fetch_fields(m_data->m_datares);

	if(myfield[col].charsetnr == CHARSET_NUMBER && 
		(myfield[col].type == MYSQL_TYPE_LONG_BLOB || myfield[col].type == MYSQL_TYPE_MEDIUM_BLOB || myfield[col].type == MYSQL_TYPE_JSON ||
        myfield[col].type == MYSQL_TYPE_TINY_BLOB || myfield[col].type == MYSQL_TYPE_BLOB))
	{
			 return wyTrue;
	}
	
    return wyFalse;
}

wyBool 
DataView::IsJSON(wyInt32 col)
{
	MYSQL_FIELD* myfield;

    //get the field struct
	myfield = m_wnd->m_tunnel->mysql_fetch_fields(m_data->m_datares);

	if(myfield[col].type == MYSQL_TYPE_JSON)
	{
			 return wyTrue;
	}
	
    return wyFalse;
}

//function to get cell value
wyChar*  
DataView::GetCellValue(wyInt32 row, wyInt32 col, wyUInt32* len, wyBool fromorig, wyString* bitstr)
{
	MYSQL_ROWEX*    rowsex;
	wyString		myrowstr;
	wyInt32			colautoincr = -1, bitcolwidth = -1;	
	wyBool          isbit = wyFalse;
	wyUInt32        length = 0;
    MYSQL_ROW       myrow;

    //precaution
    if(row < 0)
    {
        return NULL;
    }

    //clear bitstr if any
    if(bitstr)
    {
        bitstr->Clear();
    }

    colautoincr = m_data->m_autoinccol;
	isbit = IsBin(col);

	//if we want the data from orig row
	if(fromorig)
    {
        myrow = m_data->m_rowarray->GetRowExAt(row)->m_row;
		
        if(!myrow)
		{
			if(isbit == wyTrue && bitstr)
			{
				bitstr->SetAs("(NULL)");
			}

			return NULL;
		}
		if(len)
		{
			//If its BIT type handle it here
			if(isbit == wyTrue && bitstr)
			{
                bitcolwidth = GetBitFieldColumnWidth(m_data->m_createtablestmt, col);
				GetUtf8ColLength(myrow, m_data->m_datares->field_count, col, (wyUInt32*)len);
				ConvertStringToBinary(myrow[col], *len, bitcolwidth, bitstr);
				*len = bitstr->GetLength();				
				return NULL;
			}
			else
			{
				GetUtf8ColLength(myrow, m_data->m_datares->field_count, col, (wyUInt32*)len);
			}
		}

		return myrow[col];
	}

    rowsex = m_data->m_rowarray->GetRowExAt(row);

    //iff its BIT type handle it here (convert to bsa2 format and displying in grid)
	if(rowsex && isbit == wyTrue && bitstr)
	{
       	bitcolwidth = GetBitFieldColumnWidth(m_data->m_createtablestmt, col);
		
        if(len) 
        {
            GetUtf8ColLength(rowsex->m_row, m_data->m_datares->field_count, col, (wyUInt32 *)len);
        }
		
        ConvertStringToBinary(rowsex->m_row[col], *len, bitcolwidth, bitstr);
		length = bitstr->GetLength();			
		
		*len = length;
		return NULL;		
	}			
    
	if(rowsex && col >= 0 && col < m_data->m_datares->field_count)
    {
        if(!rowsex->m_row)
        {
			return NULL;
        }

		if(len) 
        {
            GetUtf8ColLength(rowsex->m_row, m_data->m_datares->field_count, col, (wyUInt32 *)len);
        }
	
		if(colautoincr == col && !rowsex->m_row[col])
        {
			return "(Auto)";
        }
		else
        {
			return  rowsex->m_row[col];
        }
	} 

    return NULL;
}

//function to reset the view
void 
DataView::ResetView(wyBool isresetexecstack)
{
    wyElem*     pelem;
    wyString    temp;
    
    //we may want to clear the execution stack, say we are supplying another data to the view
    if(isresetexecstack == wyTrue)
    {
        //clear it
        for(pelem = m_execelemstack.GetFirst(); pelem; pelem = m_execelemstack.GetFirst())
        {
            m_execelemstack.Remove(pelem);
            delete pelem;
        }

        //reset the refresh controls
        if(m_hwndfirstrow)
        {
            SetWindowText(m_hwndfirstrow, L"0");
            temp.Sprintf("%d", DEF_LIMIT);
            SetWindowText(m_hwndlimit, temp.GetAsWideChar());
        }
    }

    //reset all the windows
    if(m_hwndgrid)
    {
        CustomGrid_DeleteAllColumn(m_hwndgrid, wyFalse);
        CustomGrid_SetMaxRowCount(m_hwndgrid, 0);
        ErrorFlagState(EFS_NONE);
        EnableControls(wyFalse);
        ShowDataWindow(SW_HIDE);
        SendMessage(m_hwndtext, SCI_SETSCROLLWIDTHTRACKING, TRUE, 0);
        SendMessage(m_hwndtext, SCI_SETSCROLLWIDTH, 5, 0);
        InvalidateRect(m_hwndshowtableinfo, NULL, TRUE);
        UpdateWindow(m_hwndshowtableinfo);
    }

    //reset refresh status
    SetRefreshStatus();

    m_lastclick = -1;
}

//public function to supply the data
void 
DataView::SetData(MySQLDataEx *data)
{
    //clear the status bar
    SendMessage(GetParent(m_hwndframe), UM_SETROWCOUNT, 0, 0);

    //save the sate if not the same data and the current data is not NULL
    if(m_data && m_data != data)
    {
        SaveViewState();
    }

    //reset the view and set the data
    ResetView();
	m_data = data;

    //if valid data
    if(m_data)
    {
        //reset everything
        UpdateViewButtons();
        ThreadStopStatus(0);
        ErrorFlagState(EFS_NONE);

        //if we have a valid data and row array is not initialized, then we need to allocate it
        if(m_data->m_datares && m_data->m_isrowarrayinitialized == wyFalse)
        {
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            AllocateRowsExArray();
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
    
        //fill the grid view
        FillDataGrid();

        //if there is a modified row, then set the flag and disable duplicate row button
        if(m_data->m_modifiedrow >= 0)
        {
            ErrorFlagState(EFS_MODIFIED);
            EnableToolButton(wyFalse, IDM_DUPLICATE_ROW);
        }
        //else check for warnings and set the warning flag
        else if(m_data->m_warningres)
        {
            ErrorFlagState(EFS_WARNING);
        }

        //update the view
        ThreadStopStatus(1);
        SetLimitValues();
        EnableControls(wyTrue);
        SetRefreshStatus();
        SetDispMode(m_data->m_viewtype);
        SetViewState();
    }

    //change the filter icon
    ChangeFilterIcon();
}

//public function to get the data
MySQLDataEx* 
DataView::GetData()
{
    return m_data;
}

//function to set refresh status for form view and text view
void 
DataView::SetRefreshStatus(wyBool isreset, wyInt32 flag)
{
    wyInt32 temp = 0;

    //if we want to reset 
    if(isreset == wyTrue)
    {
        if(!(flag & TEXTVIEW_REFRESHED))
        {
            temp |= (m_refreshstatus & TEXTVIEW_REFRESHED) ? TEXTVIEW_REFRESHED : 0;
        }

        if(!(flag & FORMVIEW_REFRESHED))
        {
            temp |= (m_refreshstatus & FORMVIEW_REFRESHED) ? FORMVIEW_REFRESHED : 0;
        }
    }
    //else set the status for the view
    else
    {
        temp = m_refreshstatus;
        temp |= flag;
    }

    //update the member
    m_refreshstatus = temp;
}

//function to fill the grid view
void 
DataView::FillDataGrid()
{
    //create all the columns
	CreateColumns();

    //set the number of rows to be displayed
    CustomGrid_SetMaxRowCount(m_hwndgrid, m_data->m_rowarray->GetLength());
}

//function to show blob
wyBool
DataView::HandleBlobValue(WPARAM wparam, LPARAM lparam)
{
	wyChar*             data = 0;
	INSERTUPDATEBLOB	pib = {0};	
	BlobMgmt			biu;
    wyUInt32            len = 0;
	wyInt32				offset = 0, row = 0, col = 0, isdataupdated = 0;
    wyBool              ret = wyFalse, isedit;
	wyString            datastr;
	wyWChar				directory[MAX_PATH + 1] = { 0 };
	wyWChar				*lpfileport = 0;
	wyInt32				jsonOpt;
	wyString			dirstr;

    //get the selected row and column
    row = CustomGrid_GetCurSelRow(m_hwndgrid);
    col = CustomGrid_GetCurSelCol(m_hwndgrid);

	offset = row;

    //get cell value
    pib.m_data = GetCellValue(row, col, &len, wyFalse);

    //check whether the column is readonly or not
#ifndef COMMUNITY
    isedit = (IsColumnReadOnly(col) || IsColumnVirtual(col)==1 || GetActiveWin()->m_conninfo.m_isreadonly)? wyFalse : wyTrue;
#else
	isedit = (IsColumnReadOnly(col) || IsColumnVirtual(col)==1)? wyFalse : wyTrue;
#endif
	//if chardown is true then lparam = 1, else 0.
	if(!pib.m_data)
    {
		pib.m_isnull = (lparam == 0) ? wyTrue : wyFalse;
    }

	//to check whether it is a blob or text
	pib.m_isblob = IsBlob(col);
	pib.m_isJson = IsJSON(col);

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	jsonOpt = wyIni::IniGetInt(GENERALPREFA, "JsonFormat", 0, dirstr.GetString());

	//if data is there then we make a copy of it.
	if(pib.m_data)
    {
		Json::Reader reader;
		Json::Value jvalue;

		if (pib.m_isJson && (0 != jsonOpt) && reader.parse(pib.m_data, pib.m_data + len, jvalue))
		{
			JSONCPP_STRING		jsonValue; // This will get automatically freed

			jsonValue = jvalue.toStyledString();

			data = (wyChar*)calloc(sizeof(wyChar), jsonValue.length() + 1);
			memcpy(data, jsonValue.c_str(), jsonValue.length());
			pib.m_data = data;
			pib.m_datasize = jsonValue.length();
		}
		else
		{
			data = (wyChar*)calloc(sizeof(wyChar), len + 1);
			memcpy(data, pib.m_data, len);
			pib.m_data = data;
			pib.m_datasize = len;
		}
	}

	 //show blob
    ret = biu.Create(m_hwndgrid, &pib, isedit);

    //is anything changed
	if(pib.m_ischanged == wyTrue && isedit == wyTrue)
    {
        //update the cell value
        if(pib.m_isnull)
        {
			isdataupdated = UpdateCellValue(row, col, NULL, 0, wyTrue, wyFalse);
        }
		else
        {
			isdataupdated = UpdateCellValue(row, col, pib.m_data, pib.m_datasize, wyTrue, wyFalse);
        }

        //if cell data updated and we are in the last row, then add a new row
        if(isdataupdated && row == m_data->m_rowarray->GetLength() - 1)
        {
            AddNewRow();
        }
	}

    //free data
	if(data)
    {
		free(data);
    }

    return ret;
}

//function to check the current view is form view
wyBool 
DataView::IsFormView()
{
#ifndef COMMUNITY
    if(m_data && m_data->m_viewtype == FORM && m_formview)
    {
        return wyTrue;
    }
#endif

    return wyFalse;
}

//when we are moving the selection from one row
BOOL 
DataView::OnGvnBeginChangeRow(wyInt32 row, wyInt32 col, wyBool onkillfocus)
{
    EXEC_INFO exeinfo;

	CustomGrid_ApplyChanges(m_hwndgrid, wyTrue);

    //precaution
    if(!m_data || row >= m_data->m_rowarray->GetLength())
    {
        return FALSE;
    }

    //if we have a modified row and the row changing to is not the modified row
	if(m_data->m_modifiedrow >= 0 && m_data->m_modifiedrow != row)
	{
        //fill the structure so that once the operation is completed succesfully, the grid will be updated
        exeinfo.m_la = LA_ROWCHANGE;
        exeinfo.m_wparam = row;
        exeinfo.m_lparam = col;
        exeinfo.m_ret = TE_SUCCESS;
        exeinfo.m_pdataview = this;
         
        //set the operation to be performed
        if(m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow)->IsNewRow())
        {
            exeinfo.m_ta = TA_INSERT;
        }
		else
        {
            exeinfo.m_ta = TA_UPDATE;
        }

        //execute
        Execute(&exeinfo, wyTrue, wyTrue);

        //we are always preventing the row change if there is a modification. the insert/update is perfomed in the worker thread, so we cannot allow row change until the operation is completed
        //once the operation is completed succesfully, the cleanup module will set the selection to the new row
        return FALSE;
    }

    //return true so that the row can be changed
	return TRUE;
}

//when we starting updating a cell
LRESULT
DataView::OnGvnBeginLabelEdit(wyInt32 row, wyInt32 col)
{
    //if it is the last row, then add a new row
    if(row == m_data->m_rowarray->GetLength() - 1)
    {
		AddNewRow();
	}
	
    //make sure the cell is visible
	CustomGrid_EnsureVisible(m_hwndgrid, row, col);

    //display the calandar control for appropriate column types
	if((m_data->m_datares->fields[col].type == FIELD_TYPE_DATETIME || 
        m_data->m_datares->fields[col].type == FIELD_TYPE_TIMESTAMP || 
        m_data->m_datares->fields[col].type == FIELD_TYPE_DATE))
	{ 
		if(m_hwndcal)
        {
			DestroyWindow(m_hwndcal);
        }

		if(m_data->m_datares->fields[col].type == FIELD_TYPE_DATE)
        {
			m_hwndcal = DisplayCalendarGrid(m_hwndgrid, this, wyTrue, wyFalse);
        }
		else
        {
			m_hwndcal = DisplayCalendarGrid(m_hwndgrid, this, wyFalse, wyFalse);
        }
	}

    //return true so that the cell can be edited
    return TRUE;
}

//function is called when the cell editing is completed
wyBool 
DataView::OnGvnEndLabelEdit(WPARAM wparam, wyChar* buff)
{
    //destroy the calandar control
	if(m_hwndcal)
    {
		SendMessage(m_hwndcal, UM_ENDDIALOG, 0, 1);
    }

    return wyTrue;
}

//update the cell value by allocating required space 
LRESULT 
DataView::OnGvnSetOwnerColData(WPARAM wparam, LPARAM lparam)
{
	GVDISPINFO* disp = (GVDISPINFO*)wparam;
    
	return UpdateCellValue(disp->nRow, disp->nCol, (VOID*)disp->text, disp->cchTextMax, wyTrue, lparam ? wyTrue : wyFalse);
}

//function to update the cell value
wyInt32 
DataView::UpdateCellValue(wyInt32 row, wyInt32 col, void* data, wyUInt32 collen, wyBool isupdateformview, wyBool iscancel)
{
	wyUInt32		len = 0, fieldlen;
    wyInt32         i = 0, fcount;
    MYSQL_ROWEX*    oldrow;
	wyChar			*rowbuffer = NULL, *def = NULL;
    wyString        tempstr;
    MYSQL_ROW       myrow;

    //precaution
    if(row < 0 || col < 0)
    {
        return FALSE;
    }

    if(data && !strcmpi((wyChar*)data, "(NULL)"))
    {
        data = NULL;
        collen = 0;
    }
	
    //get the field count
	fcount = m_data->m_datares->field_count;
    
    //get the mysql row for the row index
    myrow = m_data->m_rowarray->GetRowExAt(row)->m_row;

    //if the function was called from grid notification and the user has cancelled the editing
    if(iscancel == wyTrue)
	{
        //cancel the changes if the user started editing and cancel it and there was no other modifications in the row
        //to see this in action start editing the last row and then press escape
        if(ErrorFlagState() != EFS_MODIFIED && row == m_data->m_rowarray->GetLength() - 2)
        {
            //we call cancel changes to remove the current row. but cancel changes requires a valid row index as modified row. 
            //we are here because we dont have a valid modified row, so lets make our current row as the modified row and cancel changes will take care the rest
            m_data->m_modifiedrow = row;
            CancelChanges(wyFalse);
        }

        //no editing has been done
		return FALSE;
	}	

	//if the old value and current value not NULL
    if(myrow[col] && data)
	{
        //if the value is not changed, then we have to fill the default value if any. this happens when the user starts editing and press esc
        if(strcmp(myrow[col], (wyChar*)data) == 0)
		{
            //get the default value for the column
            GetColumnName(tempstr, col);
            def = GetDefaultValue(m_wnd->m_tunnel, m_data->m_fieldres, NULL, tempstr.GetString());
			
            //if it is valid
            if(def)
			{
				tempstr.SetAs(def);

                //if it is not default or it is not a new row. if the user starts editing and cancel it on a saved row, then we dont have anything to update
                if(tempstr.Compare((wyChar*)data) || 
                    m_data->m_rowarray->GetRowExAt(row)->IsNewRow() == wyFalse)
                {
                    return FALSE;
                }
            }
            else
            {   
                //we are here because something went wrong in the grid code. lets get out from here
                if(!(m_data->m_datares->fields[col].flags & BLOB_FLAG))
                    return FALSE;
            }
		}
	}
    //handle NULL
    else if((!data || strcmp((wyChar*)data, "(NULL)") == 0) && 
        (!myrow[col] || !strcmp(myrow[col], "(NULL)")) && 
        m_data->m_rowarray->GetRowExAt(row)->IsNewRow() == wyFalse)
    {
        return FALSE;
    }

    //we are here means we got something changed in the cell. need to allocate memory for it
    //calculate the total size for the row
	for(i = 0, len; i < fcount; i++) 
    {
        //if this is the column update, then consider the new column length
		if(i == col)
        {
			len += collen;
		}
        //else get the old length
        else
        {
			fieldlen = 0;
			GetUtf8ColLength(myrow, fcount, i, &fieldlen);
			len += fieldlen;
		}
	}

    //if no old row, means this is the a new change happened in grid
    //we need this to do perform update etc in the grid, also when the user cancel changes, we simply replace with this row
	if(!m_data->m_oldrow)
	{
        //store the current row
        m_data->m_oldrow = m_data->m_rowarray->GetRowExAt(row);
    }

    //get the row
    oldrow = m_data->m_rowarray->GetRowExAt(row);

    //update the row array item with the new row
    m_data->m_rowarray->Update(row, new MYSQL_ROWEX());

    //whatever it is we need to refresh the text view next time it is made visible
    SetRefreshStatus(wyTrue, TEXTVIEW_REFRESHED);

    //allocate memory for the row
    m_data->m_rowarray->GetRowExAt(row)->m_row = (MYSQL_ROW)calloc((sizeof(wyChar*) * (fcount + 1)) + len + fcount + 1, 1);

    //set the state and check based on the old row state
    m_data->m_rowarray->GetRowExAt(row)->m_state = (oldrow->m_state & ~ROW_MYSQL);
    m_data->m_rowarray->GetRowExAt(row)->m_ischecked = m_data->m_oldrow->m_ischecked;
	
    //now starts the real allocation, a mysql row is a 1*N charecter array.
    rowbuffer = (wyChar*)(m_data->m_rowarray->GetRowExAt(row)->m_row + (fcount + 1));

    //loop throguh the fields
	for(i = 0, len = 0; i < fcount; i++)
    {
        //if old data was not and it is not the column where the modification happened
        if(!oldrow->m_row[i] && i != col)
        {
            m_data->m_rowarray->GetRowExAt(row)->m_row[i] = NULL;
			continue;
		}

        //if this the column modified, means we have to copy the new value to the column
		if(i == col) 
        {
			len = collen;
			
            //if no data
            if(!data)
            {
                m_data->m_rowarray->GetRowExAt(row)->m_row[i] = NULL;
				continue;
			}

            //if it is not binary data
			if(!(m_data->m_datares->fields[i].flags & BINARY_FLAG))
            {
                //you can be sure that the data coming is utf8 encoded
                tempstr.SetAs((wyChar*)data);

                //now set column to point to the buffer allocated
                m_data->m_rowarray->GetRowExAt(row)->m_row[i] = rowbuffer;

                //if it is < MySQL 4.1, means we need to interpret the data as ansi. not unicode
                if(m_wnd->m_ismysql41 == wyFalse)
                {
                    //copy the data
                    strcpy(m_data->m_rowarray->GetRowExAt(row)->m_row[i], tempstr.GetAsAnsi());

                    //calculate the length
                    len = strlen(tempstr.GetAsAnsi());
                }
                else
                {
                    //else do a mem copy and point the column to the buffer, any way it is utf8 encoded unicode data
                    m_data->m_rowarray->GetRowExAt(row)->m_row[i] = (wyChar*)memcpy(rowbuffer, data, collen);
                }
            }
            //if it is binary data, then we do a memcopy, no encoding need to be considered
            else
            {
                m_data->m_rowarray->GetRowExAt(row)->m_row[i] = (wyChar*)memcpy(rowbuffer, data, collen);
            }
		} 
        //for other columns
        else
        {
            //calculate the length
            GetUtf8ColLength(oldrow->m_row, fcount, i, (wyUInt32*)&len);

            //do a mem copy and point the column to the buffer
            m_data->m_rowarray->GetRowExAt(row)->m_row[i] = (wyChar*)memcpy(rowbuffer, oldrow->m_row[i], len);
		}

        //adjust the buffer pointer
		rowbuffer += (len + 1);
	}

    //once all the columns are copied, set the row
    m_data->m_rowarray->GetRowExAt(row)->m_row[i] = rowbuffer;

    //if temperory old row is not the old row, this is required for the below mentioned condition
    //we update the first column in the row, then m_data->m_oldrow will be set. this is the original row, and should not be deleted
    //now we update the second column, this time we are again allocating a row that can be safely deleted, because this was the row allocated when we updated the first column
    if(oldrow != m_data->m_oldrow)
	{
        delete oldrow;
	}

	//set the error flag state to show data modified but not saved
    HandleRowModified(row);

#ifndef COMMUNITY
    //if the active view is form view and we need to update the form view
    //isupdateformview is very important. because when we edit in form view we call UpdateCellValue to do a real time replication. so in that case the form view need not be updated
    if(isupdateformview == wyTrue && IsFormView() == wyTrue)
    {
        //update the column in form view
        m_formview->LoadCurrentRow(col);
    }
#endif

	return TRUE;
}

//function handles the keywords, functions while adding data to quwery
wyBool
DataView::AppendSpecialDataToQuery(wyChar* value, wyChar* fldname, wyString& query)
{
	wyString	myrowstr;
    wyBool		issmartkw;
	wyUInt32	datalen = 0;

    //we are dealing with NULL :D
	if(!value)
	{
		query.Add("NULL");	
		return wyTrue;
	}

    //whether to handle keywords
    issmartkw = IsSmartKeyword();

    //set the value and ge the length
    myrowstr.SetAs(value);
    datalen = myrowstr.GetLength();

    //check for variable
    if(issmartkw == wyTrue && CheckForVariable(myrowstr.GetString()))
    {
	    query.Add(myrowstr.GetString());
	    return wyTrue;
    }
    //handle defaults
    else if(issmartkw == wyTrue && (!myrowstr.Compare("(DEFAULT)") || !myrowstr.CompareI("DEFAULT")))
    {
	    if(HandlerToAddDefault(m_data->m_fieldres, fldname, query) == wyTrue)
        {
		    return wyTrue;
        }
    } 
    //handle NULL
    else if(!myrowstr.CompareI("NULL") || !myrowstr.Compare("(NULL)"))
    {
	    query.Add("NULL");
	    return wyTrue;
    }
    //handle functions
    else if(issmartkw == wyTrue && CheckForFunction(myrowstr.GetString()))
    {
	    query.Add(myrowstr.GetString());
	    return wyTrue;
    }
    //handling the b'111' & x'4A' as smart functions
    else if((myrowstr.GetCharAt(0) == 'b' || myrowstr.GetCharAt(0) == 'x') && 
	    myrowstr.GetCharAt(1) == '\'' && myrowstr.GetCharAt(datalen - 1) == '\'')
    {
	    query.Add(myrowstr.GetString());
	    return wyTrue;
    }
    else if(issmartkw == wyTrue)
    {
        if((datalen = strlen(value) - 2) > 0 && value[0] == '`' && value[datalen + 1] == '`')
        {
            myrowstr.Sprintf("%.*s", datalen, value + 1);
            
            if(!myrowstr.CompareI("DEFAULT") || !myrowstr.CompareI("NULL") ||
                CheckForVariable(myrowstr.GetString()) || CheckForFunction(myrowstr.GetString()))
            {
                query.AddSprintf("'%s'", myrowstr.GetString());
                return wyTrue;
            }
        }
    }

	return wyFalse;
}

//function to add data to query
wyBool  
DataView::AppendDataToQuery(MYSQL_ROW myrow, wyInt32 col, wyInt32 colcount, wyString &query, wyBool isblob)
{
	wyUInt32    collen;
	wyChar*     escape;
	wyString	escapestr, strautoincr;
	wyInt32		colautoincr = -1;

    colautoincr = m_data->m_autoinccol;

    //get the length
	GetUtf8ColLength(myrow, colcount, col, (wyUInt32*)&collen);
	
    //if it is auto incrment column
	if(colautoincr == col && myrow && myrow[col])
	{
        strautoincr.SetAs(myrow[col], m_wnd->m_ismysql41);
		strautoincr.LTrim();
		strautoincr.RTrim();

        //if the data was (Auto), add NULL. server will automatically add proper value for the column
		if(!strautoincr.Compare("(Auto)"))
		{		
			query.AddSprintf("%s", "NULL");
			return wyTrue;
		}
	}

    //escape the data
	escape = (wyChar*)malloc((collen * 2) + 1); 
    m_wnd->m_tunnel->mysql_real_escape_string(m_wnd->m_mysql, escape, myrow[col], collen);

    //if it is not blob, means we need to take care of encoding
    if(isblob == wyFalse)
    {
	    escapestr.SetAs(escape, m_wnd->m_ismysql41);
        query.AddSprintf("'%s'", escapestr.GetString());
    }
    //else add the string directly
    else
    {
        query.AddSprintf("'%s'", escape);
    }

    free(escape);
    return wyTrue;
}

//function add default
wyBool  
DataView::HandlerToAddDefault(MYSQL_RES *fieldres, const wyChar* fieldname, wyString &query)
{
	wyString    defstr;
	wyChar*     def = {0};
    wyChar*     escape;
    wyInt32     len;
    
    //we need a proper field res
    if(fieldres) 
	{
        //get the default
		def = GetDefaultValue(m_wnd->m_tunnel, m_data->m_fieldres, NULL, fieldname);
	
		if(def)
		{
            //check it it is NULL
			if(strcmpi(def, "NULL") == 0)
            {
				query.Add("NULL");
				return wyTrue;
			}
            //check for variable
			else if(CheckForVariable(def))
            { 
				query.Add(def);
				return wyTrue;
			}

            //else escape and add the string. no need to worry about blobs because blobs cannot have defaults
            len = strlen(def);
            escape = (wyChar*)malloc((len * 2) + 1); 
            m_wnd->m_tunnel->mysql_real_escape_string(m_wnd->m_mysql, escape, def, len);
	        defstr.SetAs(escape, m_wnd->m_ismysql41);
            free(escape);
			query.AddSprintf("'%s'", defstr.GetString());
			return wyTrue;
		}	
        else
        {
            query.Add("NULL");
            return wyTrue;
        }
	}

    return wyFalse;
}

//hanlde escape key press and destroy the calandar handle
void 
DataView::HandleEscape()
{
	if(m_hwndcal)
    {
		SendMessage(m_hwndcal, WM_COMMAND, IDCANCEL, 0);
    }

	m_hwndcal = NULL;
}

//handler function for check box click in grid
void 
DataView::OnGvnCheckBoxClick(WPARAM wparam)
{
	wyInt32    row = wparam, i, j;
    wyBool     checkstate, lastcheckstate;
    
    //if the row passed in is > row count or the the row is a new row, simply return
    if(row > m_data->m_rowarray->GetLength() || m_data->m_rowarray->GetRowExAt(row)->IsNewRow())
    {
        return;
    }
    
    //handle shift click
    //if shift key is down and there was a valid previous click and it was not on the current row
    if((GetKeyState(VK_SHIFT) & SHIFTED) && 
       m_lastclick != -1 &&
       m_lastclick != row)
    {
        //get the check state of the previously clicked row
        lastcheckstate = m_data->m_rowarray->GetRowExAt(m_lastclick)->m_ischecked;

        //get the check state of current row
        checkstate = m_data->m_rowarray->GetRowExAt(row)->m_ischecked;

        //adjust the counter and limit properly so that we can loop forward through the row array
        if(m_lastclick > row)
        {
            i = row;
            j = m_lastclick;
        }
        else
        {
            i = m_lastclick;
            j = row;
        }

        //find the new check state
        if(lastcheckstate == checkstate)
        {
            checkstate = checkstate == wyTrue ? wyFalse : wyTrue;
        }
        else
        {
            checkstate = lastcheckstate;
        }

        //loop throguht the rows
        while(i <= j)
        {
            //if the row is checked currently and the new check state is false, decrement the checkded row count
            if(m_data->m_rowarray->GetRowExAt(i)->m_ischecked == wyTrue && checkstate == wyFalse)
            {
                m_data->m_checkcount--;
            }
            //if the row is unchecked currently and the new check state is true, increment the checkded row count
            else if(m_data->m_rowarray->GetRowExAt(i)->m_ischecked == wyFalse && checkstate == wyTrue)
            {
                m_data->m_checkcount++;
            }

            //set the check state
            m_data->m_rowarray->GetRowExAt(i)->m_ischecked = checkstate;
            i++;
        }
    }
    //handle plain click. need to update the check state for this row only
    else
    {
        //find the new check state
        checkstate = m_data->m_rowarray->GetRowExAt(row)->m_ischecked == wyTrue ? wyFalse : wyTrue;

        //update the check state and checked row count
        m_data->m_rowarray->GetRowExAt(row)->m_ischecked = checkstate;
        m_data->m_checkcount += (checkstate == wyTrue ? 1 : -1);
    }

    //save the click
    m_lastclick = row;
}

//function to draw check box
wyBool 
DataView::OnGvnDrawRowCheck(wyInt32 row, void* rowdat)
{
	GVROWCHECKINFO* rowcheckinfo = (GVROWCHECKINFO*)rowdat;

    //if it is NULL row by any chance
    if(!m_data->m_rowarray->GetRowExAt(row))
    {
        return wyFalse;
    }

    //if it is not a new row then only we draw the check box
    if(m_data->m_rowarray->GetRowExAt(row)->IsNewRow() == wyFalse)
    {
        //give the check state
        rowcheckinfo->ischecked = m_data->m_rowarray->GetRowExAt(row)->m_ischecked;
    }
    //otherwise draw a star
	else
    {
		rowcheckinfo->checkorstar = wyTrue;
    }
	
	return wyTrue;
}

//function to enable the buttons
void 
DataView::EnableToolButtons()
{
    wyInt32 i, size;
    wyBool  isenable;
    wyInt32 command[] = {
        IDM_IMEX_EXPORTDATA, IDM_DATATOCLIPBOARD, ID_RESULT_INSERT, 
        ID_RESULT_SAVE, ID_RESULT_DELETE, 
        ID_RESULT_CANCEL, 
    };
    
    //enable/disable refresh controls
    EnableRefreshControls(m_wnd->m_executing == wyFalse ? wyTrue : wyFalse);

    //enable only if we have a result set, no execution happenning and not text view
    isenable = m_data && m_data->m_datares && m_viewtype != TEXT && m_wnd->m_executing == wyFalse ? wyTrue : wyFalse;
    size = sizeof(command) / sizeof(command[0]);
    
    //enable the buttons
    for(i = 0; i < size; i++)
    {
        EnableToolButton(isenable, command[i]);
    }

    //handle the export button seperately
    if(m_data && m_data->m_datares && m_wnd->m_executing == wyFalse)
    {
        EnableToolButton(wyTrue, IDM_IMEX_EXPORTDATA);
    }

    //disable save and cancel if there is no modified row
    if(isenable && m_data && m_data->m_modifiedrow < 0)
    {
        EnableToolButton(wyFalse, ID_RESULT_SAVE);
        EnableToolButton(wyFalse, ID_RESULT_CANCEL);
    }

    //enable/disable the duplicate button
    EnableToolButton((IsCurrRowDuplicatable() && isenable) ? wyTrue : wyFalse, IDM_DUPLICATE_ROW);
#ifndef COMMUNITY
	if(m_wnd->m_conninfo.m_isreadonly == wyTrue)
	{
		EnableToolButton(wyFalse, IDM_DUPLICATE_ROW);
		EnableToolButton(wyFalse, ID_RESULT_SAVE);
        EnableToolButton(wyFalse, ID_RESULT_CANCEL);
		EnableToolButton(wyFalse, ID_RESULT_DELETE);
        EnableToolButton(wyFalse, ID_RESULT_INSERT);
	}
#endif
}

//function to enable toolbar
void 
DataView::EnableToolbar(wyBool isenable)
{
    if(isenable == wyFalse)
    {
        EnableToolButton(isenable, -1);
    }
    else
    {
        EnableToolButtons();
    }

    UpdateViewButtons();
}

//helper function used for comparison in binary search
wyInt32  
strcmpare(char **arg1, char **arg2)
{
   return _strcmpi(*arg1, *arg2);
}

//function to check whether the string given is a function name
wyBool  
DataView::CheckForFunction(const wyChar* funcname)
{
	wyInt32	    flen;
	wyChar**    fns;
    wyChar*     tfuncname;

    //get the length and validate
    flen = strlen(funcname);
    
	if((flen == 0) || !m_arg || m_argcount == 0)
    {
		return wyFalse;
    }

    //copy to a buffer
    tfuncname = (wyChar*)calloc(sizeof(wyChar), flen + 1);
    strcpy(tfuncname, funcname);
					  
    //skip to opening bracket
	for(flen = 0; tfuncname[flen] != '(' && tfuncname[flen] != '\0' ; flen++);
	
    //if there is no opening bracket
	if(!flen || tfuncname[flen] != '(')
    {
        free(tfuncname);
		return wyFalse;
    }
	
    //get only the function name
    tfuncname[flen] ='\0';

    //search for the function
    fns = (wyChar**)bsearch((wyChar *)&tfuncname, (wyChar *)m_arg, m_argcount, sizeof(wyChar*), (wyInt32 (*)(const void*, const void*))strcmpare);

	free(tfuncname);

    //if a match is found return true
	if(fns)
    {
        return wyTrue;
    }

    //else false
	return wyFalse;
}

//function to enable/disable refresh controls
void 
DataView::EnableRefreshControls(wyBool isenable)
{
    wyBool isorigimage = wyTrue;

    //precaution
    if(!m_hwndrefreshtool)
    {
        return;
    }
    
    //if it is disable, no need to check for anything. just disable it
    if(isenable == wyFalse)
    {
        SendMessage(m_hwndrefreshtool, TB_SETSTATE, (WPARAM)IDC_REFRESH, TBSTATE_INDETERMINATE);
        SendMessage(m_hwndrefreshtool, TB_SETSTATE, (WPARAM)ID_RESETFILTER, TBSTATE_INDETERMINATE);
        EnableLimitWindows(wyFalse);        
    }
    else
    {
        //if we have data, enable the refresh button
        SendMessage(m_hwndrefreshtool, TB_SETSTATE, (WPARAM)IDC_REFRESH, m_data ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE);

        //get the current image for reset filter
        GetButtonImage(ID_RESETFILTER, &isorigimage);

        //if we have data and a result set then enable the button else disable
        //if we have data and the current image is not the original image, enable the button else disable. this is required because the reset filter can be enabled in the absense of a result set
        SendMessage(m_hwndrefreshtool, TB_SETSTATE, (WPARAM)ID_RESETFILTER, 
            m_data && (m_data->m_datares || isorigimage == wyFalse) ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE);

        //enable other windows
        EnableLimitWindows();
    }
}

//function to enable/disable the child windows on refresh toolbar
void
DataView::EnableLimitWindows(wyBool enable)
{
    wyInt32 count, i;

    HWND hwndctrls[] = {
        m_hwndfirstrowlabel, m_hwndprev, m_hwndfirstrow, 
        m_hwndnext, m_hwndlimitlabel, m_hwndlimit        
    };

    count = sizeof(hwndctrls)/sizeof(hwndctrls[0]);

    //if it is disable its simple, just disable everything
    if(enable == wyFalse)
    {
        EnableWindow(m_hwndislimit, FALSE);

        for(i = 0; i < count; ++i)
        {
            EnableWindow(hwndctrls[i], FALSE);
        }
    }
    else
    {
        //enable/disable the check box
        EnableLimitCheckbox();

        //determine the enable state
        enable = m_data ? m_data->m_islimit : wyFalse;

        //enable/disable the window
        for(i = 0; i < count; ++i)
        {
            EnableWindow(hwndctrls[i], enable == wyTrue ? TRUE : FALSE);
        }

        //now we have to disable the next and prev buttons
        if(enable == wyTrue)
        {
            //disable next if there if no data or the number of rows is less than the limit specified
            EnableWindow(m_hwndnext, (!m_data->m_datares || m_data->m_datares->row_count < m_data->m_limit) ? FALSE : TRUE);
            
            //if the start row is zer, disable prev
            if(m_data->m_startrow <= 0)
            {
                EnableWindow(m_hwndprev, FALSE);
            }
        }
    }
}

//function to duplicate the current row
void
DataView::DuplicateCurrentRow()
{
    wyInt32         row, col;
    MYSQL_ROWEX*    temp;
    wyUInt32        size, len = 0, i;
    wyChar*         rowbuffer;
    LONG            colcount;

    //check the row can be duplicated
    if(IsCurrRowDuplicatable() == wyFalse)
    {
        return;
    }

    //get current row and column and column count
    row = CustomGrid_GetCurSelRow(m_hwndgrid);
    col = max(CustomGrid_GetCurSelCol(m_hwndgrid), 0);
    colcount = m_data->m_datares->field_count;

    //allocate a new row
    temp = new MYSQL_ROWEX();
    temp->m_state = ROW_NEW | ROW_DUPLICATE;
    temp->m_ischecked = wyFalse;
    
    //calculate the size required
    for(i=0, size = 0; i < colcount; i++) 
	{
        GetUtf8ColLength(m_data->m_rowarray->GetRowExAt(row)->m_row, colcount, i, (wyUInt32*)&len, m_wnd->m_ismysql41);
		size += len;
	}

    size = (sizeof(wyChar*) * (colcount + 1)) + size + colcount + 1;

    //allocate the memory with the calculated size
    temp->m_row = (MYSQL_ROW)calloc(size, 1);
    rowbuffer = (wyChar*)(temp->m_row + (colcount + 1));
    
	//now allocate memory to it
	for(i = 0; i < colcount; i++)
    {
        if(!m_data->m_rowarray->GetRowExAt(row)->m_row[i])
        {
			temp->m_row[i] = NULL;
			continue;
		}

        GetUtf8ColLength(m_data->m_rowarray->GetRowExAt(row)->m_row, colcount, i, (wyUInt32*)&len);
        temp->m_row[i] = (wyChar*)memcpy(rowbuffer, m_data->m_rowarray->GetRowExAt(row)->m_row[i], len);
		rowbuffer +=(len + 1);
	}

    //pont the row to the allocated buffer
    temp->m_row[i] = rowbuffer;

    //insert the row
    m_data->m_rowarray->Insert(temp, row + 1);

    //whatever it is we need to refresh the text view next time it is made visible
    SetRefreshStatus(wyTrue, TEXTVIEW_REFRESHED);
        
    //handle auto increment column
    if((i = GetAutoIncrIndex()) != -1)
    {
        CustomGrid_SetText(m_hwndgrid, row + 1, i, "(Auto)");
    }    
    
    //increase the row count that can be displayed in the grid
    CustomGrid_SetMaxRowCount(m_hwndgrid, m_data->m_rowarray->GetLength());

    //set the selection
    CustomGrid_SetCurSelRow(m_hwndgrid, row + 1, wyTrue);
	CustomGrid_EnsureVisible(m_hwndgrid, row + 1, col, wyFalse);

    //mark the row as modified
    HandleRowModified(row + 1);

    //done this to consider all the columns in the row for insert query
    delete m_data->m_oldrow;
    m_data->m_oldrow = NULL;

    InvalidateRect(m_hwndgrid, NULL, FALSE);
}

//function to cancel changes in the grid
wyBool 
DataView::CancelChanges(wyBool iscancelediting)
{
    wyInt32         row;
    MYSQL_ROWEX*    currrow;

    //finish editing
    if(iscancelediting == wyTrue)
    {
        CustomGrid_CancelChanges(m_hwndgrid, wyTrue);
    }

    row = CustomGrid_GetCurSelRow(m_hwndgrid);

    //if there are no modified row, we are here by mistake :-)
    if(m_data->m_modifiedrow < 0)
    {
        return wyFalse;
    }

    //if it is anew row, delete it straight away, dont think twice. also adjust the number of rows to be displayed in the grid
    if(m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow)->IsNewRow())
    {
        m_data->m_rowarray->Delete(m_data->m_modifiedrow);
        CustomGrid_SetMaxRowCount(m_hwndgrid, m_data->m_rowarray->GetLength());

        //whatever it is we need to refresh the text view next time it is made visible
        SetRefreshStatus(wyTrue, TEXTVIEW_REFRESHED);
    }
    //else replace the row with the row pointer we kept before updating
    else if(m_data->m_oldrow)
    {
        //get the current row pointer so that we can free it
        currrow = m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow);

        //set the check state same as the current row check state
        m_data->m_oldrow->m_ischecked = currrow->m_ischecked;

        //update the row index with the old row
        m_data->m_rowarray->Update(m_data->m_modifiedrow, m_data->m_oldrow);

        //whatever it is we need to refresh the text view next time it is made visible
        SetRefreshStatus(wyTrue, TEXTVIEW_REFRESHED);

        //set to NULL
		m_data->m_oldrow = NULL;

        //delete the current row
        delete currrow;
    }

    //function to update the status
    HandleRowModified(-1, wyFalse);

    //set the selected row
    row = row > m_data->m_rowarray->GetLength() - 1 ? max(row - 1, 0) : row;
    CustomGrid_SetCurSelRow(m_hwndgrid, row, wyTrue);
    RefreshActiveDispWindow();

	return wyTrue;
}

//function to refresh the active display window
void
DataView::RefreshActiveDispWindow()
{
    HWND hwnd = NULL;

    //if we have data, based on the view, get the handle
    if(m_data)
    {
        switch(m_data->m_viewtype)
        {
            case GRID:
                hwnd = m_hwndgrid;
                break;

            case FORM:
#ifndef COMMUNITY
                hwnd = m_formview->m_hwndhtml;
#endif
                break;

            case TEXT:
                hwnd = m_hwndtext;
                break;
        }
    }

    //update the window
    if(hwnd)
    {
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }
}

//function to handle click on next. it start execution 
void
DataView::HandleNext()
{
    CustomGrid_ApplyChanges(m_hwndgrid);
    SetFocus(GetActiveDispWindow());
    Execute(TA_REFRESH, wyTrue, wyTrue, LA_NEXT);
}

//function to handle click on prev. it start execution 
void 
DataView::HandlePrevious()
{
    CustomGrid_ApplyChanges(m_hwndgrid);
    SetFocus(GetActiveDispWindow());
    Execute(TA_REFRESH, wyTrue, wyTrue, LA_PREV);
}

//function saved the current view state
void 
DataView::SaveViewState()
{
    CustomGrid_ApplyChanges(m_hwndgrid);
    
    //if and only if we have a valid data
    if(m_data)
    {
        //get the intial row and column shown in the grid
        m_data->m_initcol = CustomGrid_GetInitCol(m_hwndgrid);
        m_data->m_initrow = CustomGrid_GetInitRow(m_hwndgrid);

        //get the selected row and column
        m_data->m_selcol = max(CustomGrid_GetCurSelCol(m_hwndgrid), 0);
        m_data->m_selrow = max(CustomGrid_GetCurSelRow(m_hwndgrid), 0);

#ifndef COMMUNITY
        if(m_formview)
        {
            //save the form view state
            m_formview->SaveState();
        }
#endif
        //save text view scroll position, selection and selection mode
        m_data->m_textviewscrollpos.x = SendMessage(m_hwndtext, SCI_GETXOFFSET, 0, 0);
        m_data->m_textviewscrollpos.y = SendMessage(m_hwndtext, SCI_GETFIRSTVISIBLELINE, 0, 0);
        m_data->m_textviewselstartpos = SendMessage(m_hwndtext, SCI_GETSELECTIONSTART, 0, 0);
        m_data->m_textviewselendpos = SendMessage(m_hwndtext, SCI_GETSELECTIONEND, 0, 0);
        m_data->m_isselectionrect = SendMessage(m_hwndtext, SCI_SELECTIONISRECTANGLE, 0, 0) ? wyTrue : wyFalse;
    }
}

//function to show data window, it checks the display mode and shows the current window
wyBool
DataView::ShowDataWindow(wyInt32 show)
{   
    //if to show
    if(show != SW_HIDE)
    {
        //if we have a result set
        if(m_data && m_data->m_datares)
        {
            switch(m_data->m_viewtype)
            {
                //show grid
                case GRID:
                    ShowFormView(SW_HIDE);
                    ShowWindow(m_hwndtext, SW_HIDE);
                    ShowWindow(m_hwndgrid, SW_SHOW);
                    break;
            
                //show form
                case FORM:
                    //first we probe to see whether the form view is available and can be shown
                    if(ShowFormView(SW_SHOW, wyTrue) == wyFalse)
                    {
                        return wyFalse;
                    }

#ifndef COMMUNITY
                    //Load form view. refresh if needed
                    m_formview->LoadForm((m_refreshstatus & FORMVIEW_REFRESHED) ? wyFalse : wyTrue);
#endif
                    //whatever the case form view can be considered refreshed
                    SetRefreshStatus(wyFalse, FORMVIEW_REFRESHED);

                    //show form view
                    ShowWindow(m_hwndgrid, SW_HIDE);
                    ShowWindow(m_hwndtext, SW_HIDE);
                    ShowFormView(SW_SHOW);
                    break;

                //show text view
                case TEXT:
                    //check text view need to be refreshed
                    if(!(m_refreshstatus & TEXTVIEW_REFRESHED))
                    {
                        //format and add the result
                        SetCursor(LoadCursor(NULL, IDC_WAIT));
                        FormatAndAddResultSet(m_wnd->m_tunnel, m_data->m_datares, m_hwndtext, m_data, wyFalse, wyTrue);
                        SetCursor(LoadCursor(NULL, IDC_ARROW));
                        SendMessage(m_hwndtext, SCI_GOTOLINE, 0, 0);

                        //text view is refreshed
                        SetRefreshStatus(wyFalse, TEXTVIEW_REFRESHED);
                    }

                    //show text view
                    ShowWindow(m_hwndgrid, SW_HIDE);
                    ShowFormView(SW_HIDE);
                    ShowWindow(m_hwndtext, SW_SHOW);
                    break;
            }
        }
        //esle hide all data windows
        else
        {
            ShowFormView(SW_HIDE);
            ShowWindow(m_hwndtext, SW_HIDE);
            ShowWindow(m_hwndgrid, SW_HIDE);
        }
    }
    //hide all the views
    else
    {
        ShowFormView(SW_HIDE);
        ShowWindow(m_hwndtext, SW_HIDE);
        ShowWindow(m_hwndgrid, SW_HIDE);
    }

    return wyTrue;
}

//function to restore the view state
void 
DataView::SetViewState(LPEXEC_INFO pexeinfo)
{
    wyInt32         selmode, colcount, rowcount, temp;
    MYSQL_ROWEX*    prowex;

    //only if we have data
    if(m_data)
    {
        //get the row and column count
        rowcount = CustomGrid_GetRowCount(m_hwndgrid);
        colcount = CustomGrid_GetColumnCount(m_hwndgrid);

        //set the initial column to be displayed
        CustomGrid_SetInitCol(m_hwndgrid, max(min(m_data->m_initcol, colcount - 1), 0));

        //if this was called after a client side filtering, we need to adjust the initial row  so that scroll bar is at proper poistion
        if(pexeinfo && pexeinfo->m_la == LA_FILTER && pexeinfo->m_ret == TE_SUCCESS)
        {
            temp = max(min(m_data->m_selrow, rowcount - 1), 0);

            if((prowex = m_data->m_rowarray->GetRowExAt(temp)) && prowex->IsNewRow())
            {
                temp = max(temp - 1, 0);
            }

            CustomGrid_SetInitRow(m_hwndgrid, 0, wyFalse);
            CustomGrid_SetCurSelRow(m_hwndgrid, temp);
            CustomGrid_EnsureVisible(m_hwndgrid, temp, CustomGrid_GetInitCol(m_hwndgrid));
        }
        //else just the the initial row and set the selection
        else
        {
            CustomGrid_SetInitRow(m_hwndgrid, max(min(m_data->m_initrow, rowcount - 1), 0));
            CustomGrid_SetCurSelRow(m_hwndgrid, max(min(m_data->m_selrow, rowcount - 1), 0), wyTrue);
        }

        //set selected column
        CustomGrid_SetCurSelCol(m_hwndgrid, max(min(m_data->m_selcol, colcount - 1), 0), wyFalse);

#ifndef COMMUNITY
        if(m_formview)
        {
            //restore form view state
            m_formview->SetState();
        }
#endif

        //set vertical scrolling in text view
        SendMessage(m_hwndtext, SCI_SETFIRSTVISIBLELINE, m_data->m_textviewscrollpos.y, 0);

        selmode = SendMessage(m_hwndtext, SCI_GETSELECTIONMODE, 0, 0);

        if(m_data->m_isselectionrect == wyTrue)
        {
            SendMessage(m_hwndtext, SCI_SETSELECTIONMODE, SC_SEL_RECTANGLE, 0);
        }

        //set selection in text view
        SendMessage(m_hwndtext, SCI_SETSELECTIONSTART, m_data->m_textviewselstartpos, 0);
        SendMessage(m_hwndtext, SCI_SETSELECTIONEND, m_data->m_textviewselendpos, 0);
        SendMessage(m_hwndtext, SCI_SETSELECTIONMODE, selmode, 0);

        //set horizontal scroll in text view
        SendMessage(m_hwndtext, SCI_SETXOFFSET, m_data->m_textviewscrollpos.x, 0);        
    }
}

//function to handle mysql errors
wyInt32 
DataView::HandleErrors(wyString &query)
{
    //since the function can be called from worker thread, post it to the main window and return error
    SendThreadMessage(m_hwndframe, UM_MYSQLERROR, 0, (LPARAM)&query);
    return TE_ERROR;
}

//function to check for duplicates from count(*) result
wyBool
DataView::CheckForDuplicates(MYSQL_RES* data, wyInt32 &response, wyInt32 message)
{
    wyString    rowcount;
    MYSQL_ROW   temp;
    wyBool      ret = wyTrue;
    wyInt32     c;

    //fetch first row
    m_wnd->m_tunnel->mysql_data_seek(data, 0);
	temp = m_wnd->m_tunnel->mysql_fetch_row(data);

    //if it is more than one
    if(temp && strcmp(temp[0], "1") != 0)
	{
		rowcount.Add(temp[0]);
		c = rowcount.GetAsInt32();

        //post a message to the main window and wait for the response
        if(c > 1)
		{
            SendThreadMessage(m_hwndframe, message, (WPARAM)c - 1, (LPARAM)&response);
			
            if(response == IDCANCEL)
            {
				ret =  wyFalse;
            }
		}
	}

    return ret;
}

//subclassed window proc for warning controls
LRESULT CALLBACK
DataView::ShowWarningWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DataView* pdv = (DataView*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    //handle all the mouse messages
    if(message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
    {
        //return if it is executing
        if(pdv->m_wnd->m_executing == wyTrue || pdv->m_wnd->m_pingexecuting == wyTrue)
        {
            return 1;
        }

        //show a hand cursor
        SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(32649)));
	}

	return CallWindowProc(pdv->m_warningwndproc, hwnd, message, wParam, lParam);
}

//function set the limit values
void 
DataView::SetLimitValues()
{
    wyString    temp;

    if(m_data)
    {
        //if limit is applied
        if(m_data->m_islimit == wyTrue)
        {
            //set the check state
            SendMessage(m_hwndislimit, BM_SETCHECK, BST_CHECKED, 0);

            //update the edit controls with values
            temp.Sprintf("%d", m_data->m_startrow);
            SetWindowText(m_hwndfirstrow, temp.GetAsWideChar());
            temp.Sprintf("%d", m_data->m_limit);
            SetWindowText(m_hwndlimit, temp.GetAsWideChar());
        }
        else
        {
            //uncheck the button
            SendMessage(m_hwndislimit, BM_SETCHECK, BST_UNCHECKED, 0);
        }
    }
    //no data. set to default
    else
    {
        SetWindowText(m_hwndfirstrow, L"0");
        temp.Sprintf("%d", DEF_LIMIT);
        SetWindowText(m_hwndlimit, temp.GetAsWideChar());
    }
 }

//function to get the limit values
void
DataView::GetLimitValues()
{
    wyInt32 size;
    wyWChar *buff;
    wyString temp;

    //find the start row
    size = GetWindowTextLength(m_hwndfirstrow);

    if(size > 0)     
    {
        buff = new wyWChar[size + 1];
        GetWindowText(m_hwndfirstrow, buff, size + 1);
        temp.SetAs(buff);
        delete[] buff;

        if(temp.GetAsInt32() < 0)
        {
            m_data->m_startrow = 0;
        }
        else
        {
            m_data->m_startrow = temp.GetAsInt32();
        }
    }
    else
    {
        m_data->m_startrow = 0;
    }
         
    //find the limit
    size = GetWindowTextLength(m_hwndlimit);

    if(size > 0)     
    {
        buff = new wyWChar[size + 1];
        GetWindowText(m_hwndlimit, buff, size + 1);
        temp.SetAs(buff);
        delete[] buff;
                
        if(temp.GetAsInt32() < 0)
        {
            m_data->m_limit = DEF_LIMIT;
        }
        else
        {
            m_data->m_limit = temp.GetAsInt32();
        }
    }
    else
    {
        m_data->m_limit = DEF_LIMIT;
    }

    //update the window. this is required becuase user can paste some invalide charecters in the control.
    SetLimitValues();
}

//function to handle tool bar drop down notification
wyInt32	
DataView::HandleToolBarDropDown(LPNMTOOLBAR hdtoolbar)
{	
	if(hdtoolbar->iItem == IDM_DATATOCLIPBOARD)
	{
        OnTBDropDownCopy(hdtoolbar, m_hwndgrid, m_data->m_checkcount);
		return TBDDRET_DEFAULT;
	}

	return 1;
}

//function load the popup menu and enable/disable the items
void
DataView::OnTBDropDownCopy(NMTOOLBAR *tbh, HWND hwndgrid, wyInt32 selectedrows)
{
	HMENU   hmenu, htrackmenu, hsubmenu;
	RECT    rc;
    wyInt32 row, col;
		
    //load menu and localize it
	hmenu =	LoadMenu(pGlobals->m_hinstance, MAKEINTRESOURCE(IDR_QUERYLISTMENU));
    LocalizeMenu(hmenu);

    htrackmenu = GetSubMenu(hmenu, 0);
    hsubmenu = GetSubMenu(htrackmenu, 14);
    
    //now find the location where the button is. we will place the menu just under that to get dropdown effect
    SendMessage(tbh->hdr.hwndFrom, TB_GETRECT,(WPARAM)tbh->iItem,(LPARAM)&rc);
    MapWindowPoints(tbh->hdr.hwndFrom, HWND_DESKTOP,(LPPOINT)&rc, 2);

    //enable/disable the menu items
    row = CustomGrid_GetCurSelRow(m_hwndgrid);
    col = CustomGrid_GetCurSelCol(m_hwndgrid);
    SetCopyMenu(hsubmenu, row, col);

    //save the menu handle, need it to owner draw the items
    m_htrackmenu = hsubmenu;

    //set owner draw property and display the menu
    wyTheme::SetMenuItemOwnerDraw(m_htrackmenu);
	TrackPopupMenu(hsubmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, rc.left, rc.bottom, 0, tbh->hdr.hwndFrom, NULL);
    DestroyMenu(hmenu);
}

//function checks for any unsaved changes in the grid and returns the thread action curresponding to it
ThreadAction
DataView::CheckForModifications()
{
    wyInt32 temp = 0;
        
    SendThreadMessage(m_hwndframe, UM_ERRFLAGSTATE, -1, (LPARAM)&temp);

    //if the data modified flag is visible
    if(temp == EFS_MODIFIED)
    {
        //confirm whether to save/discard or cancel the operation
        SendThreadMessage(m_hwndframe, UM_CONFIRMREFRESH, 0, (LPARAM)&temp);

        switch(temp)
        {
            //user want to save the changes
            case IDYES:
                //if it is new row, we need to insert, else update
                if(m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow)->IsNewRow())
                {
                    return TA_INSERT;
                }
                else
                {
                    return TA_UPDATE;
                }

                break;

            //discard the changes
            case IDNO:
                SendThreadMessage(m_hwndframe, UM_CANCELCHANGES, 0, 0);
                return TA_REFRESH;
                
            //cancel the operation
            case IDCANCEL:
                return TA_NONE;
        }
    }

    //no modification, refresh please
    return TA_REFRESH;
}

//an important function that initiates various operations
wyUInt32
DataView::Execute(LPEXEC_INFO lpexecinfo, wyBool issendmsg, wyBool spawnthread)
{
    LPTHREAD_PARAM  lptp = NULL;
    wyUInt32        tid = 0;
    wyInt32         ret = 1;

    //allocate a new thread param, the thread function is responsible to delete this
    lptp = new THREAD_PARAM;

    //set the members
    memset(lptp, 0, sizeof(THREAD_PARAM));
    lptp->m_execinfo = *lpexecinfo;
    lptp->m_dataview = this;
    lptp->m_issendmsg = issendmsg;

    //send execution start message
    if(issendmsg == wyTrue)
    {
        lptp->m_hwndfocus = GetFocus();
        SendThreadMessage(lptp->m_dataview->m_hwndframe, UM_STARTEXECUTION, lptp->m_execinfo.m_ta, (LPARAM)lptp);
    }

    //whether to create a new thread
    if(spawnthread == wyTrue)
    {
        //create an event handle that the thread should wait on
        lptp->m_hthreadstartevent = CreateEvent(NULL, TRUE, FALSE, NULL);

        //execute the function in a new thread
        lptp->m_hthread = (HANDLE)_beginthreadex(NULL, 0, DataView::ExecuteThreadProc, lptp, 0, &tid);

        //once we have the valid thread handle, set the event so that the thread can continue
        SetEvent(lptp->m_hthreadstartevent);
    }
    else
    {
        //execute in the same thread
        ret = DataView::ExecuteThreadProc(lptp);
    }

    //return the value. if the function spawn a new thread this value is irrelevent
    return ret;
}

//helper function to call the actual execute function
wyUInt32
DataView::Execute(ThreadAction action, wyBool issendmsg, wyBool spawnthread, LastAction la)
{
    EXEC_INFO execinfo;

    //fill the members
    execinfo.m_la = la;
    execinfo.m_lparam = NULL;
    execinfo.m_ta = action;
    execinfo.m_wparam = NULL;
    execinfo.m_ret = TE_SUCCESS;
    execinfo.m_pdataview = this;

    //call execute
    return Execute(&execinfo, issendmsg, spawnthread);
}

//thread function
unsigned __stdcall 
DataView::ExecuteThreadProc(LPVOID lpparam)
{
    LPTHREAD_PARAM lptp = (LPTHREAD_PARAM)lpparam;
    ThreadAction   ta = TA_NONE;
    wyBool         isthread = wyFalse;
    wyInt32        ret = TE_SUCCESS;
    MySQLDataEx*   pmdata = NULL;
    wyBool         istunnel = lptp->m_dataview->m_wnd->m_tunnel->IsTunnel() ? wyTrue : wyFalse;

    //this is valid means, it is a new thread
    if(lptp->m_hthreadstartevent)
    {
        //wait for the event to be signaled. this is required so that we can be sure that the thread handle is valid.
        //accroding to MSDN, the thread handle can be invalid if the thread function exits quickly. 
        WaitForSingleObject(lptp->m_hthreadstartevent, INFINITE);

        //close the event
        CloseHandle(lptp->m_hthreadstartevent);

        //if it is not tunnel, we need to call thread_init to initialize thread specific variables. to avoid memory leak
        if(istunnel == wyFalse)
        {
            mysql_thread_init();
        }

        lptp->m_hthreadstartevent = NULL;

        //indicate that this is in a new thread
        isthread = wyTrue;
    }

    //perform the operation based on thread action
    switch(lptp->m_execinfo.m_ta)
    {  
        //refresh the view
        case TA_REFRESH:
            //check for unsaved changes
            ta = lptp->m_dataview->CheckForModifications();

            //if there was a modification and the thread action required 
            switch(ta)
            {
                //insert/update
                case TA_INSERT:
                case TA_UPDATE:
                    //execute the action 
                    if((ret = lptp->m_dataview->Execute(ta, wyTrue, wyFalse)) == TE_SUCCESS)
                    {
                        //if the actopm was successful we are good to refresh
                        ta = TA_REFRESH;
                    }
                    else
                    {
                        //else cancel it
                        ret = TE_CANCEL;
                    }

                    break;

                //if the user decided not to refresh
                case TA_NONE:
                    ret = TE_CANCEL;
                    break;
            }

            //now are we still with refresh?
            if(ta == TA_REFRESH)
            {
                //if the last action is due to client side sort and filter
                if((lptp->m_dataview->m_data->m_psortfilter && lptp->m_dataview->m_data->m_psortfilter->m_isclient == wyTrue) &&
                    (lptp->m_execinfo.m_la == LA_SORT || lptp->m_execinfo.m_la == LA_FILTER))
                {
                    //if there was not data, we can still have reset filter enabled, in that case we should continue with refreshing, because we need the data from the server
                    //else, we do it in client side, no need to get it from server, so no refresh please
                    ta = lptp->m_dataview->m_data->m_datares ? TA_NONE : TA_REFRESH;
                }
            }
           
            //are we still with refresh?
            if(ta == TA_REFRESH)
            {
                //send the message to update the refresh values.
                //since the check for modification is done in the refresh operation itself, we dont update the refresh controls till we reach here
                SendThreadMessage(lptp->m_dataview->m_hwndframe, UM_SETREFRESHVALUES, 0, (LPARAM)&lptp->m_execinfo);

                //clear data
                lptp->m_dataview->Execute(TA_CLEARDATA, wyTrue, wyFalse);

                //now refresh
                ret = lptp->m_dataview->RefreshDataView();
            }
            break;

        //free the data
        case TA_CLEARDATA:
            //get the data and delete it
            pmdata = lptp->m_dataview->ResetData(lptp->m_dataview->m_data);
            delete pmdata;
            break;

        //delete rows
        case TA_DELETE:
            ret = lptp->m_dataview->ProcessDelete();
            break;
            
        //insert row
        case TA_INSERT:
            ret = lptp->m_dataview->InsertRow();
            break;

        //update row
        case TA_UPDATE:
            ret = lptp->m_dataview->UpdateRow();
            break;
    
    }

    //get the return status
    lptp->m_execinfo.m_ret = (ThreadExecStatus)ret;
    
    //send the end execution message
    if(lptp->m_issendmsg == wyTrue)
    {
        SendThreadMessage(lptp->m_dataview->m_hwndframe, UM_ENDEXECUTION, lptp->m_execinfo.m_ta, (LPARAM)lptp);
    }

    //now if it is a seperate thread
    if(isthread == wyTrue)
    {
        //precaution
        if(lptp->m_hthread)
        {
            //post the message so that we can close the thread handle
            PostMessage(lptp->m_dataview->m_hwndframe, UM_THREADFINISHED, lptp->m_execinfo.m_ta, (LPARAM)lptp->m_hthread);
        }

        //if it is not http connection, free the thread specfic variables, to avoid memory leak
        if(istunnel == wyFalse)
        {
            mysql_thread_end();
        }
    }

    //delete thread param
    delete lptp;

    return (wyUInt32)ret;
}

//function to resize the view and warning controls
void
DataView::ResizeControls(RECT parentrect)
{
    HDC     hdc;
    HFONT   hfontprev;
    RECT    textrect;
    wyInt32 warningwidth, widthwarn, widthmod, warningheight, ribbonheight, vpos;

    hdc = GetDC(m_hwndwarning);
    hfontprev = (HFONT)SelectObject(hdc, m_modifiedfont);
    memset(&textrect, 0, sizeof(textrect));
    DrawText(hdc, SAVE_WARNING, -1, &textrect, DT_CALCRECT);
    widthmod = textrect.right + 4;
    warningwidth = widthmod;
    memset(&textrect, 0, sizeof(textrect));
    DrawText(hdc, SHOW_WARNING, -1, &textrect, DT_CALCRECT);
    widthwarn = textrect.right + 4;
    warningwidth = (warningwidth > widthwarn ? warningwidth : widthwarn) + 2; 
    warningheight = textrect.bottom;
    SelectObject(hdc, hfontprev);
    ReleaseDC(m_hwndwarning, hdc);
    ribbonheight = DataView::CalculateRibbonHeight(m_hwndshowtableinfo);

    vpos = (parentrect.bottom - ribbonheight) < parentrect.top ? parentrect.top : (parentrect.bottom - ribbonheight);
    SetWindowPos(m_hwndmodified, NULL, (parentrect.right - warningwidth) + (warningwidth - widthmod), vpos + ((ribbonheight / 2) - (warningheight / 2)),
        widthmod, warningheight, SWP_NOZORDER);
    SetWindowPos(m_hwndwarning, NULL, (parentrect.right - warningwidth) + (warningwidth - widthwarn), vpos + ((ribbonheight / 2) - (warningheight / 2)),
        widthwarn, warningheight, SWP_NOZORDER);

    SetWindowPos(m_hwndshowtableinfo, NULL, 0, vpos, parentrect.right - warningwidth, ribbonheight, SWP_NOZORDER);
    parentrect.bottom = (parentrect.bottom - ribbonheight < parentrect.top ) ? parentrect.top : parentrect.bottom - ribbonheight;
    MoveWindow(m_hwndgrid, 0, parentrect.top, parentrect.right - parentrect.left, parentrect.bottom - parentrect.top, TRUE);
    MoveWindow(m_hwndtext, 0, parentrect.top, parentrect.right - parentrect.left, parentrect.bottom - parentrect.top, TRUE);

#ifndef COMMUNITY
    //resize the form view too
    if(m_formview)
    {
        m_formview->Resize();
    }
#endif
}

//static function to calculate the ribbon height used to display the info
wyInt32 
DataView::CalculateRibbonHeight(HWND hwnd)
{
    HDC     hdc;
    HFONT   hfont;
    RECT    fontrect = {0}, deffontrect = {0};
    wyInt32 ribbonheight = 0;

    hdc = GetDC(hwnd);
    hfont = GetStaticTextFont(hwnd);
    hfont = (HFONT)SelectObject(hdc, hfont);
    DrawText(hdc, L"W", -1, &fontrect, DT_CALCRECT);
    hfont = (HFONT)SelectObject(hdc, hfont);
    DeleteFont(hfont);
    hfont = GetStockFont(DEFAULT_GUI_FONT);
    hfont = (HFONT)SelectObject(hdc, hfont);
    DrawText(hdc, L"W", -1, &deffontrect, DT_CALCRECT);
    hfont = (HFONT)SelectObject(hdc, hfont);
    DeleteFont(hfont);
    ReleaseDC(hwnd, hdc);
    ribbonheight = (fontrect.bottom > deffontrect.bottom ? fontrect.bottom : deffontrect.bottom) + 6;

    return ribbonheight;
}

//function handles the row modification status
void
DataView::HandleRowModified(wyInt32 modifiedrow, wyBool issetstate)
{
    //hide any warnings
    SendThreadMessage(m_hwndframe, UM_ERRFLAGSTATE, EFS_NONE, 0);

    //if we have a valid modified row, means we are going to show "data modified" flag
    if(modifiedrow >= 0)
    {
        m_data->m_modifiedrow = modifiedrow;
        SendThreadMessage(m_hwndframe, UM_ERRFLAGSTATE, EFS_MODIFIED, 0);
    }
    else 
    {
        //if we have an old row, delete it
        if(m_data->m_oldrow)
        {
            delete m_data->m_oldrow;
            m_data->m_oldrow = NULL;
        }

        //set the row state
        if(issetstate == wyTrue && m_data->m_modifiedrow >= 0)
        {
            m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow)->m_state = ROW_DEFAULT;
        }

        //reset the modified row
        m_data->m_modifiedrow = -1;
        
        //if we are asked to show warning caused by last insert/update operation
        if(modifiedrow == -2)
        {
            //show warnings if any
            if(GetLastWarning())
            {
                SendThreadMessage(m_hwndframe, UM_ERRFLAGSTATE, EFS_WARNING, 0);
            }
        }
    }
}

//function to get warnings if any
wyBool 
DataView::GetLastWarning()
{
    wyString    query("show warnings");
    MYSQL_RES*  myres = NULL;

    //free if there is existing warnings
    if(m_data->m_warningres)
    {
        m_data->m_pmdi->m_tunnel->mysql_free_result(m_data->m_warningres);
        m_data->m_warningres = NULL;
    }

    //now check the last query caused any warnings, 
    //for http tunnel we dont get this info from the api, so have to execute the query every time
    if(!m_wnd->m_tunnel->mysql_warning_count(m_wnd->m_mysql) ||
        m_wnd->m_tunnel->IsTunnel() ||
        !(myres = ExecuteAndGetResult(m_wnd, m_wnd->m_tunnel, &m_wnd->m_mysql, query)) ||
        !myres->row_count)
    {
        //no warnings, free any resources and abort
        if(myres)
        {
            m_wnd->m_tunnel->mysql_free_result(myres);
        }

        return wyFalse;
    }

    //save the warning res
    m_data->m_warningres = myres;
    return wyTrue;
}

//function to execute a query. all the queries executed in DataView should go through this
//this is required to save the last executed DML query
MYSQL_RES*
DataView::ExecuteQuery(wyString& query, wyBool isdml)
{
    MYSQL_RES* res;

    //execute
    m_wnd->m_stopquery = 0;
    res = ExecuteAndGetResult(m_wnd, m_wnd->m_tunnel, &m_wnd->m_mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, &m_wnd->m_stopquery);
    m_wnd->m_stopquery = 0;

    //if it is a dml query and we have some rows affected
    if(isdml == wyTrue && (res || m_wnd->m_tunnel->mysql_affected_rows((m_wnd->m_mysql)) != -1))
    {
        //save the query
        m_data->m_lastdmlquery.SetAs(query);
    }
    
    return res;
}

//function to save any modifications made in the grid
wyInt32
DataView::ApplyChanges(LastAction la, WPARAM wparam, LPARAM lparam)
{
    EXEC_INFO   exeinfo = {TA_NONE, la, wparam, lparam, TE_SUCCESS, this};
    wyInt32     ret = 0;

    CustomGrid_ApplyChanges(m_hwndgrid);

    //check wheather we have something modified
    if(ErrorFlagState() == EFS_MODIFIED)
	{
        //if it is not a new row, we will update it, else insert
        if(m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow)->IsNewRow() == wyFalse)
        {
            exeinfo.m_ta = TA_UPDATE;
        }
        else
        {
            exeinfo.m_ta = TA_INSERT;
        }
        
        //execute
        ret = Execute(&exeinfo, wyTrue, wyTrue);
	}
    
    return ret;
}

//function to copy the cell data clipboard
wyBool
DataView::CopyCellDataToClipBoard()
{
	wyInt32		row = 0,col = 0;
	wyUInt32    len = 0;
	wyString	cellvalstr;
	LPWSTR		lpstrcopy;
	wyChar		*cellval = {0};
    HGLOBAL	    hglbcopy;

    //precaution
	if(!CustomGrid_GetRowCount(m_hwndgrid))
    {
		return wyFalse;
    }

    //get current selected row and column
	row = CustomGrid_GetCurSelRow(m_hwndgrid);
    col = CustomGrid_GetCurSelCol(m_hwndgrid);

    //make sure we have valid values here
	if(row == -1 || col == -1)
    {
		return wyTrue;
    }
	
    //get the cell value
	cellval = GetCellValue(row, col, &len, wyFalse);

	if(!cellval)
    {
        //if no cell value, set to string NULL
		cellvalstr.SetAs(STRING_NULL);
    }
	else
    {
        //copy it with proper encoding
        cellvalstr.SetAs(cellval, m_wnd->m_ismysql41);
    }
	
    //allocate the buf
	len = cellvalstr.GetLength();
	hglbcopy = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (unsigned long)((len + 1)* sizeof(wyWChar)));
		
	if(!hglbcopy)
	{
		DisplayErrorText(GetLastError(), _("Could not copy data to clipboard"));
		return wyFalse;
	}

    //copy it as wide charecter
	lpstrcopy = (wyWChar*)GlobalLock(hglbcopy);	
    swprintf(lpstrcopy, L"%s", cellvalstr.GetAsWideChar());
	GlobalUnlock(hglbcopy);

    //open the clipboard and copy it
	if(!OpenClipboard(m_hwndgrid))
    {
		return wyFalse;
    }
	
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, hglbcopy);
	CloseClipboard();

	return wyTrue;
}

//helper function to calculate the height and width of a button
wyBool 
DataView::GetButtonSize(HWND hwnd, RECT* prect, wyInt32 ctrltype)
{
    wyWChar*    str = NULL;
    wyUInt32    len;
    HDC         hdc;
    HFONT       hfont;

    memset(prect, 0, sizeof(RECT));

    if(!(len = GetWindowTextLength(hwnd)))
    {
        prect->right = prect->bottom = 18;
        return wyFalse;
    }

    str = new wyWChar[len + 1];
    GetWindowText(hwnd, str, len + 1);
    hdc = GetDC(hwnd);
    hfont = GetStockFont(DEFAULT_GUI_FONT);
    hfont = (HFONT)SelectObject(hdc, hfont);
    DrawText(hdc, str, -1, prect, DT_SINGLELINE | DT_CALCRECT);
    hfont = (HFONT)SelectObject(hdc, hfont);
    ReleaseDC(hwnd, hdc);

    if(ctrltype != 0)
    {
        prect->bottom = max(prect->bottom, 16) + 2;
    }

    if(ctrltype == -1)
    {
        prect->right += 16;
    }

    if(ctrltype != 0)
    {
        prect->right = max(prect->right, 18) + (ctrltype == -1 ? 2 : 16);
    }

    delete[] str;
    return wyTrue;
}


//helper function to calculate the height and width of a button
void
DataView::CreatePaddingWindow()
{
    m_hwndpadding = CreateWindowEx(0, WC_STATIC, L"", 
                                   WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0, 0, 0, 0, m_hwndframe, (HMENU)0, 
								   GetModuleHandle(0), NULL);
}

//function creates the refresh bar and refresh controls
void
DataView::CreateRefreshBar()
{
    DWORD   style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
    WNDPROC wndproc;

    //create the toolbar and subclass it
    m_hwndrefreshtool = CreateWindowEx(WS_EX_CONTROLPARENT, TOOLBARCLASSNAME, NULL, 
                                       WS_CHILD | CCS_NOPARENTALIGN | TBSTYLE_TOOLTIPS | WS_VISIBLE | CCS_NODIVIDER | TBSTYLE_FLAT, 
                                       0, 0, 0, 0, m_hwndframe,
                                       (HMENU)IDC_TOOLBAR, (HINSTANCE)GetModuleHandle(0), NULL);
	SetWindowLongPtr(m_hwndrefreshtool, GWLP_USERDATA, (LONG_PTR)this);
    m_refreshtoolproc = (WNDPROC)SetWindowLongPtr(m_hwndrefreshtool, GWLP_WNDPROC,(LONG_PTR)DataView::RefreshToolWndProc);
    
    //create the limit checkbox and subclass it
    m_hwndislimit = CreateWindowEx(0, WC_BUTTON, _(TEXT("Limit rows")), 
                                   style | WS_TABSTOP | WS_GROUP | BS_AUTOCHECKBOX,	
                                   0, 0, 0, 0, m_hwndrefreshtool, 
                                   (HMENU)IDC_ROWSINRANGE, 
								   GetModuleHandle(0), NULL);
	SendMessage(m_hwndislimit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE );
    wndproc = (WNDPROC)SetWindowLongPtr(m_hwndislimit, GWLP_WNDPROC, (LONG_PTR)DataView::RefreshCtrlWndProc);
    SetWindowLongPtr(m_hwndislimit, GWLP_USERDATA, (LONG_PTR)wndproc);

    //create the label for first row
    m_hwndfirstrowlabel	= CreateWindowEx(0, WC_STATIC, _(TEXT("First row")), 
									     style, 0, 0, 0, 0,	m_hwndrefreshtool, (HMENU)IDC_ABORT, 
										 GetModuleHandle(0), NULL);
    SendMessage(m_hwndfirstrowlabel, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE );

    //create previous button 
	m_hwndprev = CreateWindowEx(0, TEXT("BUTTON"), TEXT(""), 
                                WS_CHILD | WS_VISIBLE | WS_GROUP | BS_ICON,
                                0, 0, 0, 0, m_hwndrefreshtool, (HMENU)IDC_PREVIOUS, 
                                GetModuleHandle(0), 
                                NULL);
	SendMessage(m_hwndprev, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE);
	SetWindowLongPtr(m_hwndprev, GWLP_USERDATA, (LONG_PTR)this);

    //create first row edit control and subclass it
	m_hwndfirstrow = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, TEXT(""), 
                                    style | ES_SUNKEN | ES_NUMBER |ES_AUTOHSCROLL | WS_TABSTOP | WS_GROUP, 
                                    0, 0, 0, 0,
                                    m_hwndrefreshtool, (HMENU)IDC_LIMIT1, 
                                    GetModuleHandle(0),
                                    NULL);
	SendMessage(m_hwndfirstrow, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE );
    wndproc = (WNDPROC)SetWindowLongPtr(m_hwndfirstrow, GWLP_WNDPROC, (LONG_PTR)DataView::RefreshCtrlWndProc);
    SetWindowLongPtr(m_hwndfirstrow, GWLP_USERDATA, (LONG_PTR)wndproc);

    //create next button
	m_hwndnext = CreateWindowEx(0, TEXT("BUTTON"), TEXT(""),	
                                WS_CHILD | WS_VISIBLE | WS_GROUP | BS_ICON,
                                0, 0, 0, 0, m_hwndrefreshtool, (HMENU)IDC_NEXT, 
                                GetModuleHandle(0), NULL);
	SetWindowLongPtr(m_hwndnext, GWLP_USERDATA, (LONG_PTR)this);

    //create label for number of rows
	m_hwndlimitlabel = CreateWindowEx(0, WC_STATIC, _(TEXT("# of rows")), 
                                      style, 0, 0, 0, 0, m_hwndrefreshtool, (HMENU)IDC_ABORT, 
                                      GetModuleHandle(0), NULL);
    SendMessage(m_hwndlimitlabel, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE);

    //create edit control for number of rows and subclass it
	m_hwndlimit = CreateWindowEx(WS_EX_CLIENTEDGE , WC_EDIT, TEXT(""), 
                                 style | WS_TABSTOP | WS_GROUP | ES_NUMBER, 0,0, 0,0,
                                 m_hwndrefreshtool, (HMENU)IDC_LIMIT2, 
                                 GetModuleHandle(0), NULL);
	SendMessage(m_hwndlimit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE);
	wndproc = (WNDPROC)SetWindowLongPtr(m_hwndlimit, GWLP_WNDPROC, (LONG_PTR)DataView::RefreshCtrlWndProc);
    SetWindowLongPtr(m_hwndlimit, GWLP_USERDATA, (LONG_PTR)wndproc);

    //icons for previous and next buttons
    m_iconprevious = (HICON)LoadImage((HINSTANCE)pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_PREVIOUS), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	SendMessage(m_hwndprev, BM_SETIMAGE, IMAGE_ICON, (LPARAM)m_iconprevious); 
	m_iconnext = (HICON)LoadImage((HINSTANCE)pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_NEXT), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    SendMessage(m_hwndnext, BM_SETIMAGE, IMAGE_ICON, (LPARAM)m_iconnext);

    //add refresh bar buttons
    AddRefreshBarButtons();

    //auto size it and then remove the property
    SendMessage(m_hwndrefreshtool, TB_AUTOSIZE, 0, 0);
    style = GetWindowLongPtr(m_hwndrefreshtool, GWL_STYLE);
    SetWindowLongPtr(m_hwndrefreshtool, GWL_STYLE, style | CCS_NORESIZE);
}

//add buttons for refresh bar
void
DataView::AddRefreshBarButtons()
{
    TBBUTTON tbb[10];
    wyInt32 size, j, i;
    HICON hicon;
    wyInt32 command[] = {IDM_SEPARATOR, ID_RESETFILTER, IDC_REFRESH};
    wyUInt32 states[][2] = {
        {TBSTATE_ENABLED, TBSTYLE_SEP},
        {TBSTATE_ENABLED, TBSTYLE_BUTTON}, 
        {TBSTATE_ENABLED, TBSTYLE_BUTTON}
    };

    wyInt32 imgres[] = {IDI_USERS, IDI_FILTER, IDI_TABLEREFRESH};

    m_hrefreshtoolimglist = ImageList_Create(ICON_SIZE, ICON_SIZE, ILC_COLOR32  | ILC_MASK, 1, 0);
    SendMessage(m_hwndrefreshtool, TB_SETIMAGELIST, 0, (LPARAM)m_hrefreshtoolimglist);
	SendMessage(m_hwndrefreshtool, TB_SETEXTENDEDSTYLE, 0 , (LPARAM)TBSTYLE_EX_DRAWDDARROWS);
    SendMessage(m_hwndrefreshtool, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    size = sizeof(command)/sizeof(command[0]);

	for(j=0; j < size; j++)	
	{
		hicon = (HICON)LoadImage(pGlobals->m_hinstance, MAKEINTRESOURCE ( imgres[j] ), IMAGE_ICON, ICON_SIZE, ICON_SIZE, LR_DEFAULTCOLOR);
		VERIFY((i = ImageList_AddIcon(m_hrefreshtoolimglist, hicon))!= -1);
		VERIFY(DestroyIcon(hicon));
		memset(&tbb[j], 0, sizeof(TBBUTTON));
		tbb[j].iBitmap = MAKELONG(i, 0);
		tbb[j].idCommand = command[j];
		tbb[j].fsState = (UCHAR)states[j][0];
		tbb[j].fsStyle = (UCHAR)states[j][1];
	}  

    //add extra images
    AddExtraImages(m_hrefreshtoolimglist);
    SendMessage(m_hwndrefreshtool, TB_ADDBUTTONS, (WPARAM)size,(LPARAM) &tbb);

    //set image indexes
    SetImageIndexes(m_hwndrefreshtool);
}

//function to change the tool button to/from stop icon when execution starts/stops
wyBool 
DataView::ChangeToolButtonIcon(wyBool isexecstart, wyInt32 id)
{
    wyInt32 image;
    HWND    hwnd = NULL;

    //find the toolbar associated with the button id
    if(SendMessage(m_hwndtoolbar, TB_COMMANDTOINDEX, id, 0) != -1)
    {
        hwnd = m_hwndtoolbar;
    }
    else if(SendMessage(m_hwndrefreshtool, TB_COMMANDTOINDEX, id, 0) != -1)
    {
        hwnd = m_hwndrefreshtool;
    }    

    //if no toolbar found
    if(hwnd == NULL)
    {
        return wyFalse;
    }

    //on execution start, change the icon to stop
    if(isexecstart == wyTrue)
    {
        image = GetImageIndex(IMG_INDEX_STOP, GETIMG_IMG, hwnd, NULL);
    }
    //else change the icon to original
    else
    {
        image = GetImageIndex(id, GETIMG_COMMAND, hwnd, NULL);
    }

    //apply the changes
    if(image != -1)
    {
        SendMessage(hwnd, TB_CHANGEBITMAP, (WPARAM)id, (LPARAM)image);
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        return wyTrue;
    }

    return wyFalse;    
}


//function to get the image index
wyInt32
DataView::GetImageIndex(wyInt32 id, wyInt32 flag, HWND hwndtool1, ...)
{
    va_list         vl;
    wyInt32         count = 0, command;
    HWND            hwndtemp;
    TBBUTTONINFO    tbbi = {0};

    command = id;

    //if we are looking for an extra image associated with the toolbar, id is the offset
    if(flag == GETIMG_IMG)
    {
		va_start(vl, hwndtool1);      //va_start(vl, flag);
        count = SendMessage(hwndtool1, TB_BUTTONCOUNT, 0, 0);

        //loop throught the toolbar handles that shares the same image list
        while((hwndtemp = va_arg(vl, HWND)))
        {
            count += SendMessage(hwndtemp, TB_BUTTONCOUNT, 0, 0);
        }

        va_end(vl);


        if(id > -1 && id < IMG_INDEX_LAST)
        {
            //return the image index
            return count + id;
        }
    }
    //otherwise its the image associated with the button
    else if(flag == GETIMG_COMMAND || flag == GETIMG_INDEX)
    {
        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_LPARAM | (flag == GETIMG_INDEX ? TBIF_BYINDEX : 0);
        
        //get the info
        if(SendMessage(hwndtool1, TB_GETBUTTONINFO, id, (LPARAM)&tbbi) != -1)
        {
            //the lparam has the orginal image
            return tbbi.lParam;
        }
    }

    return -1;
}

//function to set the image indexes to the buttons lparam
void 
DataView::SetImageIndexes(HWND hwndtool)
{
    TBBUTTONINFO tbbi;
    wyInt32 count, i, image;

    count = SendMessage(hwndtool, TB_BUTTONCOUNT, 0, 0);
    
    //loop through the buttons
    for(i = 0; i < count; ++i)
    {
        //get the image of the button
        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_IMAGE | TBIF_BYINDEX;
        SendMessage(hwndtool, TB_GETBUTTONINFO, i, (LPARAM)&tbbi);
        image = tbbi.iImage;

        //set the image as lparam
        tbbi.dwMask = TBIF_LPARAM | TBIF_BYINDEX;
        tbbi.lParam = image;
        SendMessage(hwndtool, TB_SETBUTTONINFO, i, (LPARAM)&tbbi);
    }
}

//function to add extra images to the image list
void 
DataView::AddExtraImages(HIMAGELIST himglist)
{
    wyInt32 i, count;
    HICON   hicon;

    count = sizeof(m_extraimages) / sizeof(m_extraimages[0]);

    for(i = 0; i < count; ++i)
    {
        hicon = (HICON)LoadImage(pGlobals->m_hinstance, MAKEINTRESOURCE(m_extraimages[i]), IMAGE_ICON, ICON_SIZE, ICON_SIZE, LR_DEFAULTCOLOR);
		ImageList_AddIcon(himglist, hicon);
		DestroyIcon(hicon);
    }
}

//function to resize refresh toolbar
void
DataView::ResizeRefreshTool(wyInt32 minheight, RECT* prect)
{
    RECT	    rctemp, rccheck, rcnext, textrect = {0}, tbrect;
	wyInt32		hpos, width, height, toolwidth, toolheight;
    wyInt32     padding = 6, horzpadd = 4, itemcount;
    TBBUTTON    tbinfo = {0};

    memset(&rctemp, 0, sizeof(rctemp));
    itemcount = SendMessage(m_hwndrefreshtool, TB_BUTTONCOUNT, 0, 0) - 1;

    if(itemcount >= 0)
    {
        SendMessage(m_hwndrefreshtool, 
            TB_GETBUTTON, 
            itemcount,
            (LPARAM)&tbinfo);

        SendMessage(m_hwndrefreshtool, TB_GETRECT, tbinfo.idCommand, (LPARAM)&rctemp);
    }

    memset(prect, 0, sizeof(RECT));
    GetWindowRect(m_hwndrefreshtool, &tbrect);
    GetButtonSize(m_hwndislimit, &rccheck, -1);
    GetButtonSize(m_hwndnext, &rcnext, 1);
    height = max(max(minheight, tbrect.bottom - tbrect.top), max(rccheck.bottom, rcnext.bottom));

    hpos = horzpadd + rctemp.right;
    width = rccheck.right;
    SetWindowPos(m_hwndislimit, NULL, hpos, (height / 2) - (rccheck.bottom / 2) - 1 , width, rccheck.bottom,  SWP_NOZORDER);

    hpos += width + padding;
    GetButtonSize(m_hwndfirstrowlabel, &textrect, 0);
    width = textrect.right;
    SetWindowPos(m_hwndfirstrowlabel, NULL, hpos, (height / 2) - (textrect.bottom / 2) - 2 , width, textrect.bottom,  SWP_NOZORDER);

    hpos += width + 5;
    width = rcnext.right;
    SetWindowPos(m_hwndprev, NULL, hpos, (height / 2) - (rcnext.bottom / 2) - 1 , width, rcnext.bottom,  SWP_NOZORDER);

    hpos += width + 3;
    width = 50;
    SetWindowPos(m_hwndfirstrow, NULL, hpos, (height / 2) - (rcnext.bottom / 2) - 1 , width, rcnext.bottom,  SWP_NOZORDER);

    hpos += width + 3;
    width = rcnext.right;
    SetWindowPos(m_hwndnext, NULL, hpos, (height / 2) - (rcnext.bottom / 2) - 1, width, rcnext.bottom,  SWP_NOZORDER);
    
    hpos += width + padding;
    GetButtonSize(m_hwndlimitlabel, &textrect, 0);
    width = textrect.right;
    SetWindowPos(m_hwndlimitlabel, NULL, hpos, (height / 2) - (textrect.bottom / 2) - 2, width, textrect.bottom,  SWP_NOZORDER);

    hpos += width + 5;
    width = 50;
    SetWindowPos(m_hwndlimit, NULL, hpos, (height / 2) - (rcnext.bottom / 2) - 1, width, rcnext.bottom,  SWP_NOZORDER);

    width += hpos + horzpadd;    
    toolheight = height;
    toolwidth = width;
    SetWindowPos(m_hwndrefreshtool, NULL, 0, 0, toolwidth, toolheight, SWP_NOZORDER | SWP_NOMOVE);
    prect->right = toolwidth;
    prect->bottom = toolheight;
}

//function to add data to clipboard
wyBool
DataView::AddDataToClipBoard()
{
	HGLOBAL hglbcopy;
	
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	
	//calls getdata function which copies the required data allocates global memory and returns handle
	hglbcopy = GetViewData(wyFalse);

	if(hglbcopy == 0)
    {
		return wyFalse;
    }

	SetCursor(LoadCursor(NULL, IDC_ARROW));
	
    //open the clipboard and copy the data
	if(!(OpenClipboard(m_hwndgrid)))
    {
		return wyFalse;
    }
	
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, hglbcopy);
	CloseClipboard();
	
	return wyTrue;
}


HGLOBAL
DataView::GetViewData(wyBool selected)
{
	wyInt32         ret, tocopy;
	wyChar			fterm, lterm; 
	wyChar		    encl;
	wyBool 			isescaped, isfterm, islterm, isencl;
	INT64			numfields, i;
	wyUInt32    	len[4096] = {0}, rowcount, j, lenwchar = 1; // changed the max allowed number of column per table from 3400 to 4096
	wyUInt32		*length, nsize, esclen;
	LPWSTR			lpstrcopy = NULL;
	HGLOBAL			hglbcopy;
	wyChar          enclosed[12]={0}, escape[12]={0}, fescape[12]={0}, lescape[12]={0}, nullchar[12] = {0};
	wyString		fescapestr, lescapestr, escapestr, enclosedstr, nullcharstr;
	wyBool			isrowchecked = wyFalse, copyselectedrow = wyFalse;
	wyInt32			selectedrow = 0;

    m_totalsize = 0;
	
	ExportCsv		cesv;
	EscapeChar		esch;

	MYSQL_ROW		myrow;
	MYSQL_FIELD*	fields;

	//get the total number of fields in the resultset.
    fields		=	m_data->m_datares->fields;
    numfields	=	m_data->m_datares->field_count;
	nsize		=	0;

	//set copy from clipboard flag true
	cesv.m_esch.m_isclipboard = wyTrue;

	cesv.m_esch.ReadFromFile(CSVSECTION);

	// get the escape character and format it..
	tocopy = esch.GetEscapeChar(m_hwndgrid, &cesv.m_esch, wyTrue, wyTrue);

	if(!tocopy)
    {
		return 0;
    }

	// for processing escape chars 
	strncpy(enclosed,	cesv.m_esch.m_enclosed,	sizeof(enclosed)-1);
	strncpy(escape,		cesv.m_esch.m_escape,	sizeof(escape)-1);
	strncpy(fescape,	cesv.m_esch.m_fescape,	sizeof(fescape)-1);
	strncpy(lescape,	cesv.m_esch.m_lescape,	sizeof(lescape)-1);
	strncpy(nullchar,	cesv.m_esch.m_nullchar,	sizeof(nullchar)-1);

	// process the escaping characters.
	cesv.ProcessEscChar(enclosed);
	cesv.ProcessEscChar(escape);
	cesv.ProcessEscChar(fescape);
	cesv.ProcessEscChar(lescape);
	cesv.ProcessEscChar(nullchar);

	cesv.GetDefEscChar(&fterm, &isfterm, &lterm, &islterm, &encl, &isencl);

	isescaped = (wyBool)cesv.m_esch.m_isescaped;

	esclen = strlen(fescape)+ strlen(lescape)+ 5; 

	m_totalsize = GetTotalMemSize(&cesv, selected, esclen);
    
    if(m_totalsize <= 0)
    {
        return 0;
    }
		
    ret = OpenClipboard(m_hwndgrid);		
	ret = EmptyClipboard();
	ret = CloseClipboard();

	hglbcopy = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, m_totalsize);
	
	// check whether so much pof memory has been allocated or not.
	// if the user presses Copy things for say a BLOB column with lots of data.
	// hglbCopy may be NULL;
	if(!hglbcopy)
	{
		DisplayErrorText(GetLastError(), _("Could not copy data to clipboard"), m_hwndgrid);
		return NULL;
	}

	// Lock the handle to the memory and the pointer to the first byte so that we 
	// can copy the data into the memory.
	lpstrcopy = (wyWChar*)GlobalLock(hglbcopy);	
	nsize = 0;
	//Add filednames if required
	if(cesv.m_esch.m_isincludefieldnames)
	{
		for(i = 0; i < numfields; i++)
		{
			if(cesv.m_esch.m_isfixed)
			{
				ret = WriteFixed(&hglbcopy, &lpstrcopy, &nsize, fields[i].name, &fields[i], strlen(fields[i].name), isescaped, escape[0], fterm, isfterm, lterm, islterm, encl, isencl);
				if(!ret)
                {
                    DisplayErrorText(GetLastError(), _("Could not copy data to clipboard"), m_hwndgrid);
                    GlobalFree(hglbcopy);
					return 0;
                }
			}
			else
			{
				ret = WriteVarible(&hglbcopy, &lpstrcopy, &nsize, fields[i].name, &fields[i], strlen(fields[i].name), isescaped, escape[0], fterm, isfterm, lterm, islterm, encl, isencl, cesv.m_esch.m_isoptionally, cesv.m_esch.m_nullchar, cesv.m_esch.m_isnullreplaceby); 
				if(!ret)
                {
                    DisplayErrorText(GetLastError(), _("Could not copy data to clipboard"), m_hwndgrid);
                    GlobalFree(hglbcopy);
					return 0;
                }
				
				// we only put the field separator if its not the last field as the last field will
				   //require line seprator 
				if(i != (numfields-1))
                {
					fescapestr.SetAs(fescape);
					fescapestr.GetAsWideChar(&lenwchar);

                    if(VerifyMemory(&hglbcopy, &lpstrcopy, nsize, lenwchar) == wyFalse)
                    {
                        DisplayErrorText(GetLastError(), _("Could not copy data to clipboard"), m_hwndgrid);
                        GlobalFree(hglbcopy);
					    return 0;
                    }

					wmemcpy(lpstrcopy + nsize, fescapestr.GetAsWideChar(), lenwchar);
					nsize += lenwchar;
				}
			}
		}
		lescapestr.SetAs(lescape);
		lescapestr.GetAsWideChar(&lenwchar);

        if(VerifyMemory(&hglbcopy, &lpstrcopy, nsize, lenwchar) == wyFalse)
        {
            DisplayErrorText(GetLastError(), _("Could not copy data to clipboard"), m_hwndgrid);
            GlobalFree(hglbcopy);
			return 0;
        }

		wmemcpy(lpstrcopy+nsize, lescapestr.GetAsWideChar(), lenwchar);
		nsize += lenwchar;
	}
	// Seek the data and move it to the row as given by 

    rowcount = m_data->m_rowarray->GetLength(); 
    selectedrow = CustomGrid_GetCurSelRow(m_hwndgrid);
	
	for(j=0; j<rowcount; j++)
	{
		unsigned int tj = j;

        if(selected == wyTrue && m_data->m_checkcount == 0)
		{
		    tj = j = selectedrow;
			copyselectedrow = wyTrue;
		}

		//no need to copy unsaved rows.if it is an unsaved row IsNewRow(GetCurrentRow(GetBase(), j),j) will return wyTrue
        else if((selected == wyTrue && m_data->m_rowarray->GetRowExAt(j)->m_ischecked == wyFalse && copyselectedrow == wyFalse) 
            || (m_data->m_rowarray->GetRowExAt(j)->IsNewRow() || j == m_data->m_modifiedrow))
		{
            continue;
		}

		// if we found any row in the result set that's been checked.
		isrowchecked = wyTrue;
        myrow =  m_data->m_rowarray->GetRowExAt(tj)->m_row;
			
		GetColLengthArray(myrow, numfields, len);
		length = len;

		for(i=0; i<numfields; i++)
		{
			if(cesv.m_esch.m_isfixed)
			{
				ret = WriteFixed(&hglbcopy, &lpstrcopy, &nsize, myrow[i], &fields[i], length[i], isescaped, escape[0], fterm, isfterm, lterm, islterm, encl, isencl);
				if(!ret)
                {
                    DisplayErrorText(GetLastError(), _("Could not copy data to clipboard"), m_hwndgrid);
                    GlobalFree(hglbcopy);
					return 0;
                }
			}
			else
			{
				ret = WriteVarible(&hglbcopy, &lpstrcopy, &nsize, myrow[i], &fields[i], length[i], isescaped, escape[0], fterm, isfterm, lterm, islterm, encl, isencl, cesv.m_esch.m_isoptionally, cesv.m_esch.m_nullchar, cesv.m_esch.m_isnullreplaceby);
				if(!ret)
                {
                    DisplayErrorText(GetLastError(), _("Could not copy data to clipboard"), m_hwndgrid);
                    GlobalFree(hglbcopy);
					return 0;
                }
				
				// we only put the field separator if its not the last field as the last field will
				  // require line seprator 
				if(i !=(numfields-1))
				{
					fescapestr.SetAs(fescape);
					fescapestr.GetAsWideChar(&lenwchar);

                    if(VerifyMemory(&hglbcopy, &lpstrcopy, nsize, lenwchar) == wyFalse)
                    {
                        DisplayErrorText(GetLastError(), _("Could not copy data to clipboard"), m_hwndgrid);
                        GlobalFree(hglbcopy);
					    return 0;
                    }

					wmemcpy(lpstrcopy+nsize, fescapestr.GetAsWideChar(), lenwchar);
					nsize += lenwchar;
				}
			}
		}

        lescapestr.SetAs(lescape);
		lescapestr.GetAsWideChar(&lenwchar);

        if(VerifyMemory(&hglbcopy, &lpstrcopy, nsize, lenwchar) == wyFalse)
        {
            DisplayErrorText(GetLastError(), _("Could not copy data to clipboard"), m_hwndgrid);
            GlobalFree(hglbcopy);
			return 0;
        }

		wmemcpy(lpstrcopy+nsize, lescapestr.GetAsWideChar(), lenwchar);
		nsize += lenwchar;

		if(copyselectedrow == wyTrue)
		{
			break;
		}
	}

	if(nsize && lescape[0])
		lpstrcopy[nsize - (strlen(lescape))] = 0;

	// Reallocate the memory so that extra memory is freed.
	if(nsize)
		hglbcopy = GlobalReAlloc(hglbcopy, (unsigned long)((nsize + 1) * sizeof(wyWChar)), GMEM_MOVEABLE | GMEM_ZEROINIT);

	VERIFY((GlobalUnlock(hglbcopy)) == NO_ERROR);

	return hglbcopy;

}

DWORD64
DataView::GetTotalMemSize(ExportCsv *cesv, wyBool selected, wyInt32 esclen)
{
    wyInt32         numfields, i,  top = 0;
    DWORD64          nsize = 0, totalsize;
    wyInt32         rowcount, selrowcount = 0, totsize;
    MYSQL_FIELD    *fields;

    fields		=	m_data->m_datares->fields;
    numfields	=	m_data->m_datares->field_count;

    //now get the whole size so that we can allocate 
	for(i = 0; i < numfields; i++)
	{	
		if(fields[i].max_length == 0 && fields[i].name_length == 0)
			fields[i].length; 

		else if(fields[i].name_length)
		{                
			if(fields[i].max_length > fields[i].name_length)
            {
				nsize += fields[i].max_length * 2 + 1;
            }
			else
            {
				nsize += fields[i].name_length * 2 + 1;
            }
		}			
		else
        {
            if(fields[i].max_length == 0)
            {
			    nsize += strlen(fields[i].name) * 2 + 1;
            }
            else
            {
                nsize += fields[i].max_length * 2 + 1;
            }
        }
	}

	rowcount = CustomGrid_GetRowCount(m_hwndgrid);

	//if its selected i.e. user wants to copy only selected rows then 
	//we have to allocate memory only for the selected rows 
    selrowcount = m_data->m_checkcount;

	if(selected == wyTrue)
    {
		totsize = (selrowcount * esclen);
    }
	else
    {
		totsize = (rowcount * esclen);
    }

	if(cesv->m_esch.m_isincludefieldnames)
	{
		top = 1;
		totsize += esclen;
	}

    //allocate total memory in one shot. The extra fields is for , & new line characters and 
	//some extra space.
	totalsize = (((rowcount + 1 + top) * nsize) + totsize);
    totalsize = (sizeof(wyWChar*) * (numfields + 1)) + totalsize;
    return totalsize;
}

//function checks for a potential memory overrun, if so it will allocate the memory required
wyBool
DataView::VerifyMemory(HGLOBAL* hglobal, LPWSTR* buffer, wyUInt32 nsize, wyUInt32 sizetobeadded)
{
    HGLOBAL htemp;
    
    //check for a potential buffer overrun
    if((nsize + sizetobeadded) * sizeof(wyWChar) > m_totalsize - 2)
    {
        //reallocate the memory
        m_totalsize += nsize + sizetobeadded + 2;
        GlobalUnlock(*hglobal);
        htemp = GlobalReAlloc(*hglobal, m_totalsize, GMEM_MOVEABLE | GMEM_ZEROINIT);

        //check for allocation failure
        if(htemp == NULL)
        {
            return wyFalse;
        }

        *hglobal = htemp;
        *buffer = (wyWChar*)GlobalLock(*hglobal);
    }

    return wyTrue;
}

wyBool 
DataView::WriteFixed(HGLOBAL* hglobal, LPWSTR* buffer, wyUInt32 * nsize, wyChar *text, MYSQL_FIELD * field, wyUInt32 length, wyBool isescaped, wyChar escape, wyChar fterm, wyBool isfterm, wyChar lterm, wyBool islterm, wyChar encl, wyBool isencl)
{
	wyChar	    *pad = NULL; 
	wyChar		*temp = NULL;
	wyUInt32    newsize = 0, txtlen = 0;
    wyInt32		fieldsize = 0, lengthpad; 
	wyString	tempstr, textstr, padstr;
	wyWChar		*padwchar = NULL;
	wyUInt32	padlen = 1;

	if(field->type >= FIELD_TYPE_TINY_BLOB && field->type <= FIELD_TYPE_BLOB  ||  field->type == MYSQL_TYPE_JSON)
    {
		return wyTrue;
    }

	if(!text)
	{	
		if(field->max_length == 0 && field->name_length == 0)
			txtlen = field->length;

		else if(field->name_length)
		{
			if(field->name_length > field->max_length)
            {
				txtlen = field->name_length;
            }
			else
            {
				txtlen = field->max_length;
            }
		}

		else
		{
			if(field->max_length)
            {
				txtlen = field->max_length;
            }
			else
            {
				txtlen = strlen(field->name);
            }
		}

		txtlen += DEFAULT_FIELD_SIZE;
				
		pad = AllocateBuff(txtlen + 1);
		memset(pad, C_SPACE, txtlen);
		padstr.SetAs(pad);
		padwchar = padstr.GetAsWideChar(&padlen);

        if(VerifyMemory(hglobal, buffer, *nsize, padlen) == wyFalse)
        {
            return wyFalse;
        }

		wmemcpy(*buffer+(*nsize), padwchar, padlen);
		*nsize += txtlen;
		free(pad);
	}
	else
	{
		textstr.SetAs(text);
		temp = MySqlEscape((LPCSTR)textstr.GetString(), length, &newsize, escape, isfterm, fterm, isencl, encl, islterm, lterm, wyTrue, wyTrue);

		if(field->max_length == 0 && field->name_length == 0)
        {
			txtlen = field->length;
        }

		else if(field->name_length)
		{
			if(field->max_length > field->name_length)
            {
				fieldsize = (field->max_length);
            }
            else
            {
				fieldsize = field->name_length;
            }
		}

		else
		{
			if(field->max_length)
            {
				fieldsize = field->max_length;
            }
            else
            {
				fieldsize = strlen(field->name);
            }
		}

		fieldsize += DEFAULT_FIELD_SIZE;
        
		tempstr.SetAs(temp);
        lengthpad = fieldsize + (newsize - length) + 1;
		pad = AllocateBuff(lengthpad + 1);
		memset((wyChar *)pad, C_SPACE, lengthpad - 1);
		strncpy(pad, tempstr.GetString(), tempstr.GetLength());
		padstr.SetAs(pad);
		padwchar = padstr.GetAsWideChar(&padlen);

        if(VerifyMemory(hglobal, buffer, *nsize, padlen) == wyFalse)
        {
            return wyFalse;
        }

		wmemcpy(*buffer + (*nsize), padwchar, padlen); 

		*nsize += padlen;

		free(pad);
		free(temp);
	}

	return wyTrue;
}


wyBool 		
DataView::WriteVarible(HGLOBAL* hglobal, LPWSTR* buffer, wyUInt32 *nsize, wyChar *text, MYSQL_FIELD *field, wyUInt32 length, 
                           wyBool isescaped, wyChar escape, wyChar fterm, wyBool isfterm, wyChar lterm, 
                           wyBool islterm, wyWChar encl, wyBool isencl, wyBool isoptionally, wyChar* nullchar, wyBool isnullreplaceby)
{

	wyWChar	    *temp = {0};
	wyString	tempstr, textstr;
	wyChar		*tempval = {0};
	wyUInt32    newsize =0, len = 1;
    wyString    nullstr;
	
  	if(!text)
	{
		if(escape == 0)
		{
            if(VerifyMemory(hglobal, buffer, *nsize, wcslen(L"NULL")) == wyFalse)
            {
                return wyFalse;
            }

			wmemcpy(*buffer+(*nsize), L"NULL", wcslen(L"NULL"));
			(*nsize)+= wcslen(L"NULL");
		}
		else
		{
			nullstr.SetAs(nullchar);
            if(VerifyMemory(hglobal, buffer, *nsize, wcslen(nullstr.GetAsWideChar())) == wyFalse)
            {
                return wyFalse;
            }

			wmemcpy(*buffer+(*nsize), nullstr.GetAsWideChar(), wcslen(nullstr.GetAsWideChar()));
			(*nsize)+= wcslen(nullstr.GetAsWideChar());
		}
	}
	else
	{
		if(isencl)
		{
			if(!isoptionally ||(field->type == FIELD_TYPE_VAR_STRING || field->type == FIELD_TYPE_STRING ||  field->type == FIELD_TYPE_ENUM || field->type == FIELD_TYPE_SET))
			{
                if(VerifyMemory(hglobal, buffer, *nsize, 1) == wyFalse)
                {
                    return wyFalse;
                }

				wmemcpy(*buffer+(*nsize), &encl, 1);
				(*nsize)++;
			}
		}

		tempval = MySqlEscape(text, length, &newsize, escape, isfterm, fterm, isencl, encl, islterm, lterm, wyTrue, wyTrue);
		
		tempstr.SetAs(tempval);
				
		tempstr.GetAsWideChar(&len);
		temp = AllocateBuffWChar(len + 1);
		wcscpy(temp, tempstr.GetAsWideChar());

        if(VerifyMemory(hglobal, buffer, *nsize, len) == wyFalse)
        {
            return wyFalse;
        }
		
        wmemcpy(*buffer + (*nsize), tempstr.GetAsWideChar(), len);

		(*nsize)+= len;
		free(temp);
		free(tempval);

		if(isencl)
		{
			if(!isoptionally ||(field->type == FIELD_TYPE_VAR_STRING || field->type == FIELD_TYPE_STRING ||  field->type == FIELD_TYPE_ENUM || field->type == FIELD_TYPE_SET))
			{
                if(VerifyMemory(hglobal, buffer, *nsize, 1) == wyFalse)
                {
                    return wyFalse;
                }

				wmemcpy(*buffer+(*nsize), &encl, 1);
				(*nsize)++;
			}
		}
	}
	
	return wyTrue;
}

//function adds the selected rows to clipboard
wyBool
DataView::AddSelDataToClipBoard()
{
	HCURSOR		hcursor;
	HGLOBAL		hglbcopy;

	hcursor = GetCursor();
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	ShowCursor(1);

	hglbcopy = GetViewData(wyTrue);

	if(!hglbcopy)
    {
		return wyFalse;
    }

	SetCursor(hcursor);
	ShowCursor(1);
	
	if(!(OpenClipboard(m_hwndgrid)))
    {
		return wyFalse;
    }
	
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, hglbcopy);
	CloseClipboard();
	return wyFalse;
}

//function to set the value of the current selected cell
void
DataView::SetValue(wyChar* value, wyBool isupdateformview)
{
    wyString    data;
    wyInt32     col, row;

    data.SetAs(value);

    //get the selected row and column
    row = CustomGrid_GetCurSelRow(m_hwndgrid);
    col = CustomGrid_GetCurSelCol(m_hwndgrid);
    
    //if we are updating the last row, add a new row
    if(row == m_data->m_rowarray->GetLength() - 1)
    {
        AddNewRow();
    }
    
    //updat the cell
    m_gridwndproc(m_hwndgrid,GVN_BEGINLABELEDIT, row, col);
    UpdateCellValue(row, col, value, data.GetLength(), isupdateformview, wyFalse);
    m_gridwndproc(m_hwndgrid,GVN_ENDLABELEDIT, MAKEWPARAM(row, col),(LPARAM) value);
    InvalidateRect(m_hwndgrid, NULL, FALSE);
}

//function to set the text view font
void
DataView::SetTextViewFont()
{
	wyInt32     pixel, fontitalic;
	wyWChar     directory[MAX_PATH + 1] = {0};
	wyWChar		*lpfileport = 0;
	wyString	fontnamestr, dirstr;	

    if(SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport) == wyFalse)
    {
        return;
    }

    //read the view font
	dirstr.SetAs(directory);
	wyIni::IniGetString("DataFont", "FontName", "Courier New", &fontnamestr, dirstr.GetString());
	pixel = wyIni::IniGetInt("DataFont", "FontSize", 10, dirstr.GetString());
	fontitalic = wyIni::IniGetInt("DataFont", "FontItalic", 0, dirstr.GetString());

    //set the font
    SendMessage(m_hwndtext, SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)fontnamestr.GetString());
    SendMessage(m_hwndtext, SCI_STYLESETSIZE, STYLE_DEFAULT, pixel);
    SendMessage(m_hwndtext, SCI_STYLESETITALIC, STYLE_DEFAULT, fontitalic);
	UpdateWindow(m_hwndtext);
	return;
}

//function to add a new row
void
DataView::AddNewRow()
{
    wyInt32         i, len = 0;
    wyChar          *def = NULL, *rowbuffer;
    wyString        defstr, colname;
    MYSQL_ROWEX*    temprow;
	
    //loop through the columns
	for(i = 0; i < m_data->m_datares->field_count; i++)
    {
        //get the default value
        GetColumnName(colname, i);
        def = GetDefaultValue(m_wnd->m_tunnel, m_data->m_fieldres, NULL, colname.GetString());
			
        //if there is a default, add it to the length
        if(def)
		{
			defstr.SetAs(def);
			len += defstr.GetLength();
		}
	} 

    //create a new row
    temprow = new MYSQL_ROWEX();

    //allocate row pointer
    temprow->m_row = (MYSQL_ROW)calloc((sizeof(wyChar *) * (m_data->m_datares->field_count + 1)) + len + m_data->m_datares->field_count + 1, 1);

    //set the state and check state
    temprow->m_state = ROW_NEW;
    temprow->m_ischecked = wyFalse;

    //insert the new row in array
    m_data->m_rowarray->Insert(temprow);

    //whatever it is we need to refresh the text view next time it is made visible
    SetRefreshStatus(wyTrue, TEXTVIEW_REFRESHED);

    //get the buffer
    rowbuffer = (wyChar*)(temprow->m_row + (m_data->m_datares->field_count + 1));
    
    //loop through the columns
    for(i = 0; i < m_data->m_datares->field_count; i++)
    {
		len = 0;
        GetColumnName(colname, i);
        def = GetDefaultValue(m_wnd->m_tunnel, m_data->m_fieldres, NULL, colname.GetString());

        //if there is a default value, copy it to the column
		if(def)
		{
            defstr.SetAs(def, m_wnd->m_ismysql41);
			len += defstr.GetLength();
            m_data->m_rowarray->GetRowExAt(m_data->m_rowarray->GetLength() - 1)->m_row[i] = (wyChar*)memcpy(rowbuffer, defstr.GetString(), len);
		}
        //else copy NULL
        else
        {
            m_data->m_rowarray->GetRowExAt(m_data->m_rowarray->GetLength() - 1)->m_row[i] = NULL;
            len = -1;
        }

        //adjust the buffer pointer to the next column
        rowbuffer += (len + 1);
	} 

    //set the buffer
    m_data->m_rowarray->GetRowExAt(m_data->m_rowarray->GetLength() - 1)->m_row[i] = rowbuffer;

    //set the number of rows to be displayed in the grid
	CustomGrid_SetMaxRowCount(m_hwndgrid, m_data->m_rowarray->GetLength());
}

//function to execute an INSERT query
wyInt32 
DataView::InsertRow()
{
	wyString    query;
	MYSQL_RES*  res;

    //generate insert query, if there is an error return the erro status
    if(GenerateInsertQuery(query) == wyFalse)
    {
        return TE_ERROR;
    }
    
    //check whether the thread is stopped 
	if(ThreadStopStatus())
	{
        return TE_STOPPED;
    }

    //execute the query, be sure to pass isdml parameter as wyTrue
    res = ExecuteQuery(query, wyTrue);
    
    //check thread stop status
    if(ThreadStopStatus())
	{
        return TE_STOPPED;
    }

    //now, if there is an error show mysql error dialog
    if(!res && m_wnd->m_tunnel->mysql_affected_rows((m_wnd->m_mysql)) == -1)
	{
		return HandleErrors(query);
	}

    //free the result
    m_wnd->m_tunnel->mysql_free_result(res);

    return TE_SUCCESS;
}

//function to execute UPDATA query
wyInt32 
DataView::UpdateRow()
{
	wyString    query, colname;
	wyBool      isanyprimary = wyFalse;
	MYSQL_RES*  res;
	wyInt32     response = -1, i, pkcount = 0;

    //check whether we have a primary key
	isanyprimary = IsAnyPrimary(m_wnd->m_tunnel, m_data->m_keyres, &pkcount);

    for(i = 0; pkcount && i < m_data->m_datares->field_count; ++i)
    {
        if(IsColumnReadOnly(i) == wyFalse && IsColumnVirtual(i) != 1)
        {
            GetColumnName(colname, i);

            if(IsColumnPrimary(m_wnd->m_tunnel, m_data->m_fieldres, (wyChar*)colname.GetString()) == wyTrue)
            {
                pkcount--;
            }
        }
    }

    //if there is no primary key, we should check for the number of rows going to be updated, but only if the user want to be warned
	if((isanyprimary == wyFalse || pkcount) && UpdatePrompt() == wyTrue)
	{
        //generate duplicates query. we need this to find the number of rows going to be updated as a result of the operation
        GenerateDuplicatesQuery(0, query);

        //check thread stop status
		if(ThreadStopStatus())
		{
            return TE_STOPPED;
        }
        
        //execute the query
        res = ExecuteQuery(query);

        //check thread stop status
        if(ThreadStopStatus())
		{
             return TE_STOPPED;
        }

        //if there was an error while checking the duplicates show the error and return
        if(!res)
	    {
		    return HandleErrors(query);
	    }

        //now check for duplicates
        if(CheckForDuplicates(res, response, UM_UPDATEWARNING) == wyFalse)
        {
            m_wnd->m_tunnel->mysql_free_result(res);
            return TE_STOPPED;
        }

        m_wnd->m_tunnel->mysql_free_result(res);
	}

    //check thread stop status
	if(ThreadStopStatus())
	{
        return TE_STOPPED;
    }

    //now generate the update query, depending on the response we get from duplicates confirmation we either set the limit or not
    //now if the query generation failed, we have two case, either there was nothing to execute or there was an error
    if(GenerateUpdateQuery(query, response == IDNO ? wyTrue : wyFalse) == wyFalse)
    {
        //if there is a half-baked query, we can assume that there was no query to be executed
        if(query.GetLength())
        {
            HandleRowModified(-1);
            return TE_SUCCESS;
        }
        //else there was an error
        else
        {
            return TE_ERROR;
        }
    }

    //check thread stop status
	if(ThreadStopStatus())
	{
        return TE_STOPPED;
    }

    //execute the query
    res = ExecuteQuery(query, wyTrue);
	
    //check thread stop status
    if(ThreadStopStatus())
	{
        return TE_STOPPED;
    }
    
    //if there was an error
    if(!res && m_wnd->m_tunnel->mysql_affected_rows((m_wnd->m_mysql)) == -1)
	{
		return HandleErrors(query);
	}

    m_wnd->m_tunnel->mysql_free_result(res);
    return TE_SUCCESS;
}

//function free the row after delete
void 
DataView::FreeRow(wyInt32 row, wyBool issetstate)
{
    wyInt32 selrow, modifiedrow;

    //get the selected row and modified row
    selrow = CustomGrid_GetCurSelRow(m_hwndgrid);
    modifiedrow = m_data->m_modifiedrow;

    //if the row to be deleted is checked, then we need to decrement the check count
    if(m_data->m_rowarray->GetRowExAt(row)->m_ischecked == wyTrue)
    {
        m_data->m_checkcount = max(m_data->m_checkcount - 1, 0);
    }

    //if the row to be deleted is the modified row, then we need to reset the modified flag
	if(row == m_data->m_modifiedrow)
	{
        HandleRowModified(-1, issetstate);
	}
    //else if the row to be deleted before modified row, then we have to decrement the modified row
    else if(m_data->m_modifiedrow > row && m_data->m_modifiedrow > 0)
    {
        m_data->m_modifiedrow--;
    }

    //delete the row and set the number of rows to be displayed in the grid
    m_data->m_rowarray->Delete(row);
    CustomGrid_SetMaxRowCount(m_hwndgrid, m_data->m_rowarray->GetLength());

    //whatever it is we need to refresh the text view next time it is made visible
    SetRefreshStatus(wyTrue, TEXTVIEW_REFRESHED);

    //calculate the selected row index and set it
    selrow = ((selrow > m_data->m_rowarray->GetLength() - 1) || (selrow == modifiedrow && selrow != m_data->m_modifiedrow)) ? 
        (selrow > 0 ? selrow - 1 : max(selrow, 0)) : selrow;
    CustomGrid_SetCurSelRow(m_hwndgrid, selrow, wyTrue);

    //update the display
    RefreshActiveDispWindow();
}

//function start the delete process
wyInt32
DataView::ProcessDelete()
{
    wyInt32         i, retval = TE_CANCEL, response = -1, ret = 0, rowcount;
	wyBool          deleteall = wyFalse;
    MYSQL_ROWEX*    myrowex;

    //get the current selected row. since this function is in a seperate thread we have to use thread message
    SendThreadMessage(m_hwndframe, UM_GETCURSELROW, 0, (LPARAM)&i);
    
    //now check whether we have multiple rows to be deleted
    if(m_data->m_checkcount > 0)
	{
        //confirm deletion
        SendThreadMessage(m_hwndframe, UM_CONFIRMDELETE, m_data->m_checkcount, (LPARAM)&ret);

        //whether to delete all the checked rows
        if(ret == IDYES)
		{
			deleteall = wyTrue;
		}
		else
        {
            return TE_STOPPED;
        }
	}
    //if there were no rows checked, we will delete the current row
    else
    {
        //make sure we have valid row index
        if(i < 0 || i == (m_data->m_rowarray->GetLength() - 1))
		{
            return TE_ERROR;
        }

        //confirm row deletion
        SendThreadMessage(m_hwndframe, UM_CONFIRMDELETE, m_data->m_checkcount, (LPARAM)&ret);

        if(ret == IDNO)
        {
			return TE_STOPPED;
        }
    }
	
    //delete multiple rows
	if(deleteall)
	{
        //loop through the rows
        for(i = 0, rowcount = m_data->m_rowarray->GetLength(); i < rowcount && m_data->m_checkcount > 0; i++)
		{
            //get the row
            myrowex = m_data->m_rowarray->GetRowExAt(i);

            //is the row checked
            if(myrowex->m_ischecked == wyTrue)
			{
                //if it is not a new row, then we need to execute a delete query to delete from the server
                if(myrowex->IsNewRow() == wyFalse)
				{
                    //delete row from server
                    //the second parameter in this case stores the answer for deleting duplicate rows confirmation. we prompt only once
                    retval = DeleteRow(i, response);
                    
                    //if the operation was not succesful or the user cancelled it
                    if(retval != TE_SUCCESS)
					{
                        break;
                    }
				}

                //free the row index
                SendThreadMessage(m_hwndframe, UM_DELETEROW, i, wyFalse);

                --i;
			}
		}
	}	
    //delete only the current selected row
	else
	{
        //if it is not a new row, then we need to execute a delete query to delete from the server
        if(m_data->m_rowarray->GetRowExAt(i)->IsNewRow() == wyFalse)
		{
            //delete row from server
            retval =  DeleteRow(i, response);
            
            //if the operation was not succesful or the user cancelled it
            if(retval != TE_SUCCESS)
            {
                return retval;
            }
        }

        //free the row index
		SendThreadMessage(m_hwndframe, UM_DELETEROW, i, wyFalse);
	}
	
    //return success
    return TE_SUCCESS;
}

//function that executes the delete query in the server
wyInt32 
DataView::DeleteRow(wyInt32 row, wyInt32& response)
{
	wyString    query, colname;
	MYSQL_RES*  res;
    wyInt32     pkcount = 0, i;
    wyBool      isanyprimary;

    //check whether we have a primary key
	isanyprimary = IsAnyPrimary(m_wnd->m_tunnel, m_data->m_keyres, &pkcount);

    //loop through the columns and find out whether all the primary keys are present in the result set
    for(i = 0; pkcount && i < m_data->m_datares->field_count; ++i)
    {
        if(IsColumnReadOnly(i) == wyFalse)
        {
            GetColumnName(colname, i);

            if(IsColumnPrimary(m_wnd->m_tunnel, m_data->m_fieldres, (wyChar*)colname.GetString()) == wyTrue)
            {
                pkcount--;
            }
        }
    }

    //if we dont have delete confirmation for duplicate entries and no primary keys available and the user wishes to confirm before deleting multiple rows
    if(response == -1 && (isanyprimary == wyFalse || pkcount) && UpdatePrompt() == wyTrue)
	{
        //generate duplicate query
        GenerateDuplicatesQuery(row, query);

        //check thread stop status
        if(ThreadStopStatus())
		{
            return TE_STOPPED;
        }

        //execute the query and show any error
        if(!(res = ExecuteQuery(query)))
		{
            return HandleErrors(query);
		}
		
        //now check for duplicate entries
        if(CheckForDuplicates(res, response, UM_DELETEWARNING) == wyFalse)
        {
            m_wnd->m_tunnel->mysql_free_result(res);
            return TE_STOPPED;
        }

        m_wnd->m_tunnel->mysql_free_result(res);

        //check thread stop status
        if(ThreadStopStatus())
		{
            return TE_STOPPED;
        }
	}

    //generate delete query, if the response was NO for duplicate entry deletion, then we add a limit
    GenerateDeleteQuery(query, response == IDNO ? wyTrue : wyFalse, row);

    //check thread stop status
    if(ThreadStopStatus())
	{
        return TE_STOPPED;
    }

    //execute the delete query
	res = ExecuteQuery(query, wyFalse);
    
    //check thread stop status
    if(ThreadStopStatus())
	{
        return TE_STOPPED;
    }
    
    //handle any errors
    if(!res && m_wnd->m_tunnel->mysql_affected_rows((m_wnd->m_mysql)) == -1)
	{
		return HandleErrors(query);
	}

	m_wnd->m_tunnel->mysql_free_result(res);
    return TE_SUCCESS;
}

//function that creats the columns in the grid
void 
DataView::CreateColumns(wyBool isupdate)
{
	wyInt32		        index = -1, colcount, k;
	MYSQL_FIELD*        fields;	
	GVCOLUMN		    gvcol;
	wyString		    db, table, column, colname;
	wyBool			    flag, iscolreadonly;
	wyInt32			    colwidth = 0, j;
    wyBool			    isretaincolumnwidth = wyFalse;
    RelTableFldInfo*    prelfield;
	MYSQL_ROW			myrow  = NULL;
	wyString			myrowstr;
	wyChar*				temp = NULL;

    //first we reset the refresh status for all the views
    SetRefreshStatus();

    //delete any FK list
    FKDropDown::DeleteFKList(&m_data->m_pfkinfo, &m_data->m_fkcount);

    //whether we are going to recreate all the columns or we are just updating columns
    if(isupdate == wyFalse)
    {
        isretaincolumnwidth = IsRetainColumnWidth();
	    CustomGrid_DeleteAllColumn(m_hwndgrid, wyFalse);
    }

    //if there is no data, return
    if(!m_data->m_datares)
    {
        return;
    }

    //get auto inc column index
    m_data->m_autoinccol = GetAutoIncrIndex();

    //get column count and field res
    colcount = m_data->m_datares->field_count;
	fields = m_wnd->m_tunnel->mysql_fetch_fields(m_data->m_datares);
    
    //check if we have valid db and table, that implies we have the column in non read-only mode
    if(m_data->m_db.GetLength() && m_data->m_table.GetLength())
    {
        //create the FK list
        m_data->m_fkcount = FKDropDown::CreateFKList(&m_data->m_pfkinfo, 
                                                     m_data->m_db.GetString(), 
                                                     m_data->m_table.GetString(), 
                                                     m_wnd->m_tunnel, 
                                                     &m_data->m_createtablestmt);
    }
	
    //update/create the columns
    for(k = 0; k < colcount; ++k)
	{
        //check if the column is readonly
        iscolreadonly = IsColumnReadOnly(k);

        memset(&gvcol, 0, sizeof(GVCOLUMN));

        //get the original column name
        GetColumnName(column, k);

		
        //if we are recreating the columns
        if(isupdate == wyFalse)
        {
            //set the column name
            colname.SetAs(fields[k].name, m_wnd->m_ismysql41); 

            //get the original db name and table name
		    GetDBName(db, k);
            GetTableName(table, k);
		
            //if we are going to retain the column width, then read the width from file
		    if(isretaincolumnwidth == wyTrue)
            {
			    colwidth = GetColumnWidthFromFile(&db, &table, &column);
            }
		
            //get the column alignment, integer to the righ, charecter to the left etc
            gvcol.fmt = GetColAlignment(&fields[k]);

            //set the column width
		    gvcol.cx = (colwidth > 0) ? colwidth : GetColWidth(m_hwndgrid, &fields[k], k); 

            //column name and name length
            gvcol.text = (wyChar*)colname.GetString();
		    gvcol.cchTextMax = colname.GetLength();

            //set the icon for the column. dont panic, if the column has GVIF_COLUMNMARK mask, then only the icon will visible
            gvcol.mark = (LPARAM)m_markicon;
            gvcol.marktype = GV_MARKTYPE_ICON;
        }
		
        //if the column is not read-only then we try to find the column is a FK column
        if(iscolreadonly == wyFalse)
        {
            for(flag = wyFalse, j = 0; j < m_data->m_fkcount - 1; ++j)
            {
                for(prelfield = (RelTableFldInfo*)(m_data->m_pfkinfo + j)->m_fkeyfldlist->GetFirst(); 
                    prelfield; 
                    prelfield = (RelTableFldInfo*)prelfield->m_next)
                {
                    //if the column is an FK column, add the mask to show an icon and browse button while editing
                    if(!prelfield->m_tablefld.CompareI(column))
                    {
                        gvcol.mask |= GVIF_BROWSEBUTTON | GVIF_COLUMNMARK;
                        flag = wyTrue;
                        break;
                    }
                }

                //we are done, break the loop
                if(flag == wyTrue)
                {
                    break;
                }
            }
        }
		
        //if it is blob
		if((fields[k].type >= FIELD_TYPE_TINY_BLOB) && (fields[k].type <= FIELD_TYPE_BLOB) || fields[k].type == MYSQL_TYPE_JSON)
        {
            //add blob mask
            gvcol.mask |= GVIF_TEXTBUTTON;

            //if it is recreate, then we insert the column and set the long value so that we can easily identify that the column is blob type
            if(isupdate == wyFalse)
            {
    			index = CustomGrid_InsertColumn(m_hwndgrid, &gvcol);
			    CustomGrid_SetColumnLongValue(m_hwndgrid, index, (LPARAM)1);
            }
		} 
        //if the column is enum/set
        else if((fields[k].flags & ENUM_FLAG) || (fields[k].flags & SET_FLAG)) 
        {
            //add the mask
            gvcol.mask |= (fields[k].flags & ENUM_FLAG) ? GVIF_LIST : GVIF_DROPDOWNMULTIPLE;
            
            //inser the column if it is to recreate
            if(isupdate == wyFalse)
            {
                index = CustomGrid_InsertColumn(m_hwndgrid, &gvcol);
            }
            
            //delete the list conent 
            CustomGrid_DeleteListContent(m_hwndgrid, isupdate == wyFalse ? index : k);

            //if the column is not read-only, then add the enum/set column values
            if(iscolreadonly == wyFalse)
            {
			    AddEnumSetColumnValues(isupdate == wyFalse ? index : k);
            }
		}//handling enum for http connections
		else if(m_wnd->m_tunnel->IsTunnel() && (m_data->m_fieldres))
		{
			m_wnd->m_tunnel->mysql_data_seek(m_data->m_fieldres, k);

			myrow = m_wnd->m_tunnel->mysql_fetch_row(m_data->m_fieldres);
			myrowstr.SetAs(myrow[1], IsMySQL41(m_wnd->m_tunnel, &m_wnd->m_mysql));
			temp = (wyChar*)calloc(sizeof(wyChar), (myrowstr.GetLength()) + 1);
			myrowstr.SetAs(myrow[0], IsMySQL41(m_wnd->m_tunnel, &m_wnd->m_mysql));

			// first copy the datatype so that we can get what we want.
			for(int i = 0; myrow[1][i] && myrow[1][i] != C_OPEN_BRACKET; i++)
				temp[i] = myrow[1][i];

			if ( strstr(temp, "enum") || strstr(temp, "set"))
			{
				gvcol.mask |= (strstr(temp, "enum")) ? GVIF_LIST : GVIF_DROPDOWNMULTIPLE;
            
				//inser the column if it is to recreate
				if(isupdate == wyFalse)
				{
					index = CustomGrid_InsertColumn(m_hwndgrid, &gvcol);
				}
            
				//delete the list conent 
				CustomGrid_DeleteListContent(m_hwndgrid, isupdate == wyFalse ? index : k);

				//if the column is not read-only, then add the enum/set column values
				if(iscolreadonly == wyFalse)
				{
					AddEnumSetColumnValues(isupdate == wyFalse ? index : k);
				}
			}
			else
			{
				gvcol.mask |= GVIF_TEXT;

				//insert the column if it is to recreate
				if(isupdate == wyFalse)
				{
					CustomGrid_InsertColumn(m_hwndgrid, &gvcol);
				}
			}
			if(temp)
			{
				free(temp);
				temp = NULL;
			}

		}
        //no sepecial properties need to be set
		else 
        {
            //add the mask for plain text
			gvcol.mask |= GVIF_TEXT;

            //insert the column if it is to recreate
            if(isupdate == wyFalse)
            {
			    CustomGrid_InsertColumn(m_hwndgrid, &gvcol);
            }
		}

        //if we wanted to update the column mask only
        if(isupdate == wyTrue)
        {
            CustomGrid_SetColumnMask(m_hwndgrid, k, gvcol.mask);
        }
	}
}

//get enum/set values from the string
void 
DataView::GetEnumListValues(wyInt32 col, List* plist)
{
    wyInt32		    j = 0, i, isnull, rowindex;
	wyString	    myrowstr, temp;
    wyChar*         text;
    const wyChar*   ptr;
    wyBool          close = wyFalse;
    MYSQL_ROW       myrow;
    wyInt32         quotecount;

    //get the the row index currensponding to the column from field res
    if((rowindex = GetFieldResultIndexOfColumn(col)) == -1)
    {
        return;
    }

    //get the filed index of ISNULL
    isnull = GetFieldIndex(m_data->m_fieldres, "Null", m_wnd->m_tunnel, &m_wnd->m_mysql);

    //seek to the row in field res and fetch the row
    m_wnd->m_tunnel->mysql_data_seek(m_data->m_fieldres, rowindex);
    myrow = m_wnd->m_tunnel->mysql_fetch_row(m_data->m_fieldres);
    
    //if the column is NULLable, add NULL in the list
    if(!myrow[isnull] || stricmp(myrow[isnull], "yes") == 0)
    {
        temp.SetAs(TEXT(STRING_NULL));
        plist->Insert(new EnumListElem(temp));
    }

    //now start parsing
	if(myrow[1])
    {
        myrowstr.SetAs(myrow[1], m_wnd->m_ismysql41);
    }

    //find the opening bracket, enum/set is written in the format enum('a','b')/set('a','b')
    if((i = myrowstr.Find("(", 0)) == -1)
    {
        return;
    }

    //skip past the opening bracket
    i++;

    text = new wyChar[myrowstr.GetLength() + 1];
    ptr = myrowstr.GetString();

    //extract the values
    while(close == wyFalse && ptr[i])
    {
        text[0] = 0;
        
        for(quotecount = j = 0; ptr[i]; i++)
        {
            if(quotecount == 0)
            {
                if(ptr[i] != C_SINGLE_QUOTE)
                {
                    continue;
                }
            }

            if(ptr[i] == C_BACK_SLASH)
		    {
                if(ptr[i + 1] != C_SINGLE_QUOTE)
                {
                    i = i + 1;
                }
		    }
		    else if(ptr[i] == C_SINGLE_QUOTE)
            {
                quotecount++;
            }		
            else if(!(quotecount % 2) && (ptr[i] == C_CLOSE_BRACKET || ptr[i] == C_COMMA))
            {
                text[j] = 0;

                if(ptr[i] == C_CLOSE_BRACKET)
                {
                    close = wyTrue;
                }

                break;
		    }			
			
		    text[j++] = ptr[i];
	    }

        //get the text, escpae it and add to the list
        temp.SetAs(text);
        temp.LTrim();
        temp.RTrim();
        temp.Erase(0, 1);
        temp.Erase(temp.GetLength() - 1, 1);
        temp.FindAndReplace("\\'", "''");
        temp.FindAndReplace("''", "'");
        plist->Insert(new EnumListElem(temp));
    }

    delete text;
}

//function to add the set/enum values into the column
void 
DataView::AddEnumSetColumnValues(wyInt32 col)
{
    List*           plist;
    EnumListElem*   pelem;
    
    //create a list and get the enum/set values into the list
    plist = new List;
    GetEnumListValues(col, plist);

    //loop throguh the list element and add to the column
    for(pelem = (EnumListElem*)plist->GetFirst(); pelem; pelem = (EnumListElem*)pelem->m_next)
    {
        CustomGrid_InsertTextInList(m_hwndgrid, col, pelem->m_str.GetAsWideChar());
    }

    //free the list
    delete plist;
}

//function to allocate roesex array
ThreadExecStatus 
DataView::AllocateRowsExArray()
{
	wyInt32     numrows, i;
	MYSQL_ROW   row;
    
    m_data->m_isrowarrayinitialized = wyTrue;

    //get the number of rows and resize the array to fit them
	numrows = m_wnd->m_tunnel->mysql_num_rows(m_data->m_datares);
    m_data->m_rowarray->Resize(numrows, wyTrue);

    //seek to the first row in the result set
	m_wnd->m_tunnel->mysql_data_seek(m_data->m_datares, 0);

    //set the refresh status
    SetRefreshStatus();

    //loop throught the rows
	for(i = 0; i < numrows; i++)
	{
        //get the row and update the row index in the array
		row = m_wnd->m_tunnel->mysql_fetch_row(m_data->m_datares);
        m_data->m_rowarray->Update(i, new MYSQL_ROWEX(row, ROW_MYSQL, wyFalse));
                
        //check for thread stop
        if(ThreadStopStatus())
        {
            //if the thread is stopped, free the array
            m_data->m_rowarray->Resize(0, wyTrue);
            return TE_STOPPED;
        }
	}

    return TE_SUCCESS;
}

//helper function to get the table details, such as keys, fileds etc
ThreadExecStatus 
DataView::GetTableDetails()
{
    wyString    query;
    MYSQL_RES   *myres = NULL, *fieldres = NULL, *keyres = NULL;
    MYSQL_ROW   myrow=NULL;

    //we can be sure that the form view need to be refreshed
    SetRefreshStatus(wyTrue, FORMVIEW_REFRESHED);
    
    //get create table statement
    query.Sprintf("show create table `%s`.`%s`", m_data->m_db.GetString(), m_data->m_table.GetString());
    myres = ExecuteQuery(query);

    //if the thread is stopped, cleanup and abort
    if(ThreadStopStatus())
    {
        if(myres)
        {
            m_wnd->m_tunnel->mysql_free_result(myres);
        }

        return TE_STOPPED;
    }
    
    //show any errors
    if(!myres)
    {
        HandleErrors(query);
        return TE_ERROR;
    }
    
    //get field res
    query.Sprintf("show full fields from `%s`.`%s`", m_data->m_db.GetString(), m_data->m_table.GetString());
    fieldres = ExecuteQuery(query);

    //if the thread is stopped, cleanup and abort
    if(ThreadStopStatus())
    {
        m_wnd->m_tunnel->mysql_free_result(myres);

        if(fieldres)
        {
            m_wnd->m_tunnel->mysql_free_result(fieldres);
        }

        return TE_STOPPED;
    }

    //show any errors
    if(!fieldres)
    {
        HandleErrors(query);
        m_wnd->m_tunnel->mysql_free_result(myres);
        return TE_ERROR;
    }


    //get the key res
    query.Sprintf("show keys from `%s`.`%s`", m_data->m_db.GetString(), m_data->m_table.GetString());
    keyres = ExecuteQuery(query);

    //if the thread is stopped, cleanup and abort
    if(ThreadStopStatus())
    {
        m_wnd->m_tunnel->mysql_free_result(myres);
        m_wnd->m_tunnel->mysql_free_result(fieldres);

        if(keyres)
        {
            m_wnd->m_tunnel->mysql_free_result(keyres);
        }

        return TE_STOPPED;
    }

    //show any errors
    if(!keyres)
    {
        HandleErrors(query);
        m_wnd->m_tunnel->mysql_free_result(myres);
        m_wnd->m_tunnel->mysql_free_result(fieldres);
        return TE_ERROR;
    }

    //now set the create table statement
    myrow = m_wnd->m_tunnel->mysql_fetch_row(myres);
    m_data->m_createtablestmt.SetAs(myrow[1], m_wnd->m_ismysql41);
    m_wnd->m_tunnel->mysql_free_result(myres);

    //free any existing fied res
    if(m_data->m_fieldres)
    {
        m_wnd->m_tunnel->mysql_free_result(m_data->m_fieldres);
    }

    //free any existing key res
    if(m_data->m_keyres)
    {
        m_wnd->m_tunnel->mysql_free_result(m_data->m_keyres);
    }

    //set the new field res and key res
    m_data->m_fieldres = fieldres;
    m_data->m_keyres = keyres;   

    return TE_SUCCESS;
}

//function to draw select all check box in grid
void 
DataView::OnDrawSelectAllCheck(LPARAM lparam)
{
    wyInt32                 i = 0, rowcount;
    GVINITIALBUTTONINFO*    lpinfo = (GVINITIALBUTTONINFO*)lparam;
    MYSQL_ROWEX             *myrowexlast, *myrowexseclast, *myrowexmodified;

    //get the row count
    rowcount = m_data->m_rowarray->GetLength();

    //get the last row
    myrowexlast = m_data->m_rowarray->GetRowExAt(rowcount - 1);

    //get the second last row
    myrowexseclast = m_data->m_rowarray->GetRowExAt(rowcount - 2);

    //get the modified row
    myrowexmodified = m_data->m_rowarray->GetRowExAt(m_data->m_modifiedrow);

    //if we have a last row and the last row is a new row, increase one
    i += (myrowexlast && myrowexlast->IsNewRow()) ? 1 : 0;

    //if we have a second last row and that is a new row, increase one
    i += (myrowexseclast && myrowexseclast->IsNewRow()) ? 1 : 0;

    //if we have a modified row and the modified row index is not the second last row index and modified row is a new row, add one
    i += (myrowexmodified && m_data->m_modifiedrow != rowcount - 2 && myrowexmodified->IsNewRow()) ? 1 : 0;

    //if we have checked rows and all other rows in the grid are new rows, then select all state should be checked, 
    //else if we have no checked rows it is unchecked
    //else it is indeterminate state
    lpinfo->checkstate = (i + m_data->m_checkcount >= rowcount) && m_data->m_checkcount ? BST_CHECKED : 
        (m_data->m_checkcount ? BST_INDETERMINATE : BST_UNCHECKED);
}

//handler for select all check
void 
DataView::OnSelectAllCheck(LPARAM lparam)
{
    wyBool                  checkstate;
    wyInt32                 i, rowcount;
    GVINITIALBUTTONINFO*    lpinfo = (GVINITIALBUTTONINFO*)lparam;
    MYSQL_ROWEX*            myrowex;

    //first we determine the current select all check state
    OnDrawSelectAllCheck(lparam);

    //if it is unchecked, then make it checked else unchecked
    checkstate = lpinfo->checkstate == BST_UNCHECKED ? wyTrue : wyFalse;

    //set the check state
    lpinfo->checkstate = lpinfo->checkstate == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED;

    //reset the checked rows count
    m_data->m_checkcount = 0;

    //loop through row array
    for(i = 0, rowcount = m_data->m_rowarray->GetLength(); i < rowcount; ++i)
    {
        //get the row at index
        myrowex = m_data->m_rowarray->GetRowExAt(i);

        //if it is not new row
        if(myrowex->IsNewRow() == wyFalse)
        {
            //set the check state and increase the checkcount
            myrowex->m_ischecked = checkstate;
            m_data->m_checkcount += (checkstate == wyTrue) ? 1 : 0;
        }
    }
}

//grid window proc
LRESULT CALLBACK
DataView::GridWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	DataView* pviewdata = (DataView*)CustomGrid_GetLongData(hwnd);
	static wyString tooltip;

	switch(message)
	{
		case GVN_MOUSELEAVE:
			return TRUE;

		case GVN_TOOLTIP:
			pviewdata->OnColNameMouseHover(wparam, tooltip);
			*((wyWChar**)lparam) =  tooltip.GetAsWideChar();
			return TRUE;

        //handle draw select all check
        case GVN_DRAWSELECTALL:
            pviewdata->OnDrawSelectAllCheck(lparam);
            return TRUE;

        //handle click on select all
        case GVN_SELECTALLCLICK:
            pviewdata->OnSelectAllCheck(wparam);
            return TRUE;

        //draw row check box
		case GVN_DRAWROWCHECK:
		    return pviewdata->OnGvnDrawRowCheck(wparam, (GVROWCHECKINFO*)lparam);
	
        //handle click on row check box
	    case GVN_CHECKBOXCLICK:
		    pviewdata->OnGvnCheckBoxClick(wparam);
		    break;
    		
        //set the owner column data
	    case GVN_SETOWNERCOLDATA: 
            return pviewdata->OnGvnSetOwnerColData(wparam, lparam);

        //get the display length for a cell
        case GVN_GETDISPLENINFO:
             return pviewdata->OnGvnGetDispLenInfo(wparam);

        //get the data for the cell
        case GVN_GETDISPINFODATA:
            pviewdata->OnGvnGetDispInfoData(wparam);
            return TRUE;
		
        //get the disp info
	    case GVN_GETDISPINFO:
		    pviewdata->OnGvnGetDispInfo(wparam, lparam);
		    return TRUE;
				
        //handle on column click
        case GVN_COLUMNCLICK:
            return pviewdata->OnGvnColumnClick(wparam, lparam);
				
        //draw sort order on column
	    case GVN_COLUMNDRAW:
            return pviewdata->OnGvnColumnDraw(wparam);
		
        //handle on splitter move
	    case GVN_SPLITTERMOVE:
            //if we want to retain the column width, save it
            if(IsRetainColumnWidth() == wyTrue)
            {
                pviewdata->OnSplitterMove(wparam);
            }

            //we need to refresh the form view to set the field length same as column width in grid
            pviewdata->SetRefreshStatus(wyTrue, FORMVIEW_REFRESHED);
            break;

        //whether to draw white background
        case GVN_ISWHITEBKGND:
            //if the column is readonly then we dont draw white background
            if(pviewdata->IsColumnReadOnly(lparam))
            {
                return FALSE;
            }
			if(pviewdata->IsColumnVirtual(lparam)==1)
			{
				return FALSE;
			}
			return TRUE;
        //begin label edit
	    case GVN_BEGINLABELEDIT:
            //allow only if the column is not read-only
#ifndef COMMUNITY
	if(GetActiveWin()->m_conninfo.m_isreadonly == wyTrue)
	{
		return FALSE;
	}
#endif
            if(pviewdata->IsColumnReadOnly(lparam))
            {
                return FALSE;
            }
			if(pviewdata->IsColumnVirtual(lparam)==1)
			{
			return FALSE;
			}

		    pviewdata->OnGvnBeginLabelEdit(wparam, lparam);
		    return TRUE;
		
        //end lable edit
        case GVN_ENDLABELEDIT:
		    pviewdata->OnGvnEndLabelEdit(wparam, (wyChar*)lparam);
            return TRUE;

        //cancel label edit
	    case GVN_CANCELLABELEDIT:
		    pviewdata->HandleEscape();
		    break;

	    case GVN_PASTECLIPBOARDBEGIN:
            break;

        case GVN_PASTECLIPBOARD:
            break;

        case GVN_BEGINROWDRAW:
            return (pviewdata->m_data && pviewdata->m_data->m_rowarray->GetRowExAt(wparam)) ? TRUE : FALSE;

        //the selection is changin to another row
	    case GVN_ROWCHANGINGTO:
           return pviewdata->OnGvnBeginChangeRow(wparam, lparam, wyFalse);

        //select is changed to another row
        case GVN_ENDCHANGEROW:
            //if the current view is form view, load the form view with the new row data
            if(pviewdata->IsFormView())
            {
#ifndef COMMUNITY
                pviewdata->m_formview->LoadCurrentRow();
#endif
            }

            //handle end row, such as enable disable various toolbar icons
            if(pviewdata->ThreadStopStatus())
            {
                pviewdata->OnGvnEndChangeRow(wparam);
            }

            break;
       
        //open blob fileds
	    case GVN_BUTTONCLICK:

	//		MYSQL_FIELD*        fields;
	//		fields = pviewdata->m_wnd->m_tunnel->mysql_fetch_fields(pviewdata->m_data->m_datares);
				pviewdata->HandleBlobValue(wparam, lparam);
					    return TRUE;

        //open FK dropdown
        case GVN_BROWSEBUTTONCLICK:
		    if(FKLookupAndInsert(pviewdata, NULL) == IDOK)
            {
                PostMessage(hwnd, GVM_SETFOCUS, 0, 0);
            }
            else
            {
                PostMessage(hwnd, GVM_SETFOCUSONACTIVECTRL, 0, 0);
            }
            break;

        //show context menu
        case GVN_RBUTTONDOWN:
            pviewdata->OnContextMenu(lparam);
            break;

        //handle sys key down
        case GVN_SYSKEYDOWN:
            return pviewdata->OnSysKeyDown(wparam, lparam);
			break;
	}

	return TRUE;
}

void 
DataView::OnColNameMouseHover(WPARAM wparam, wyString& column)
{	
	wyString columntype;
	GetColumnName(column, wparam);
	
	if(!m_data->m_fieldres)
		return;

	columntype.SetAs(GetDataType(m_wnd->m_tunnel, m_data->m_fieldres, (wyChar*)column.GetString()));
	if(columntype.GetLength() == 0)
		return;
	column.AddSprintf("-");
	column.AddSprintf(columntype.GetString());
}

//function to persist the width of a column, typically called when the splitter in grid moves
void
DataView::OnSplitterMove(wyInt32 col)
{
    wyString table, column, db;

    //get the db name, table name and column name
    GetDBName(db, col);
    GetTableName(table, col);
    GetColumnName(column, col);

    //save column width
    SaveColumnWidth(m_hwndgrid, &db, &table, &column, col);
}

void 
DataView::DestroyCalandarControl()
{
}

//Function to disable menu items
void 
DataView::DisableMenuItems(HMENU hmenu)
{
    HMENU   hsubmenu;
    wyInt32 i, count;

    //loop through the menu items
    for(i = 0, count = GetMenuItemCount(hmenu); i < count; ++i)
    {
        //if an item has a sub menu, recurse the funtion to disable them too
        if((hsubmenu = GetSubMenu(hmenu, i)))
        {
            DisableMenuItems(hsubmenu);
        }
        //else disable the menu item
        else
        {
            EnableMenuItem(hmenu, i, MF_BYPOSITION | MF_GRAYED);
        }
    }
}

//function to show context menu for grid view and form view
void 
DataView::ShowContextMenu(wyInt32 row, wyInt32 col, LPPOINT pt)
{
    HMENU       hmenu, htrackmenu;
    wyBool      iscolreadonly = wyTrue, iscolnullable = wyFalse, iscolhasdefault = wyFalse;
    wyString    column, columntype;
    wyInt32     copymenupos = 14,i,iscolvirtual=0;
    HWND        hwndtoolbar;
	wyBool		isunsort = wyFalse;

    //load menu, localize it and disable all the items
    hmenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_QUERYLISTMENU));
    LocalizeMenu(hmenu);
    htrackmenu = GetSubMenu(hmenu, 0);
    DisableMenuItems(htrackmenu);

    //get toolbar handle containing reset filter button
    //if the toolbar is not found, delete all the filter related menu items
    if(!GetToolBarFromID(ID_RESETFILTER))
    {
        DeleteMenu(htrackmenu, 10, MF_BYPOSITION);
        DeleteMenu(htrackmenu, 10, MF_BYPOSITION);
        copymenupos -= 2;
    }

    //if valid data and result
    if(m_data && m_data->m_datares)
    {
        //if it is not text view
        if(m_data->m_viewtype != TEXT)
        {
            //export button is found and the button is enabled in the toolbar, then enable the mneu item
            if((hwndtoolbar = GetToolBarFromID(IDM_IMEX_EXPORTDATA)) && 
                SendMessage(hwndtoolbar, TB_GETSTATE, IDM_IMEX_EXPORTDATA, 0) == TBSTATE_ENABLED)
            {
                EnableMenuItem(htrackmenu, IDM_IMEX_EXPORTDATA, MF_ENABLED);
            }

            //if db name and table name are valid, then enable the menu items based on their counterpart in toolbar
            if(m_data->m_db.GetLength() && m_data->m_table.GetLength())
            {
                EnableMenuItem(htrackmenu, ID_RESULT_INSERT, 
                    ((hwndtoolbar = GetToolBarFromID(ID_RESULT_INSERT)) && 
                    SendMessage(hwndtoolbar, TB_GETSTATE, ID_RESULT_INSERT, 0) == TBSTATE_ENABLED) ? MF_ENABLED : MF_GRAYED);
                EnableMenuItem(htrackmenu, ID_RESULT_SAVE, 
                    ((hwndtoolbar = GetToolBarFromID(ID_RESULT_SAVE)) && 
                    SendMessage(hwndtoolbar, TB_GETSTATE, ID_RESULT_SAVE, 0) == TBSTATE_ENABLED) ? MF_ENABLED : MF_GRAYED);
                EnableMenuItem(htrackmenu, ID_RESULT_DELETE, 
                    ((hwndtoolbar = GetToolBarFromID(ID_RESULT_DELETE)) && 
                    SendMessage(hwndtoolbar, TB_GETSTATE, ID_RESULT_DELETE, 0) == TBSTATE_ENABLED) ? MF_ENABLED : MF_GRAYED);
                EnableMenuItem(htrackmenu, ID_RESULT_CANCEL, ((hwndtoolbar = GetToolBarFromID(ID_RESULT_CANCEL)) && 
                    SendMessage(hwndtoolbar, TB_GETSTATE, ID_RESULT_CANCEL, 0) == TBSTATE_ENABLED) ? MF_ENABLED : MF_GRAYED);

                //if current row is duplicatable, then enable duplicate row item
                if(IsCurrRowDuplicatable())
                {
                    EnableMenuItem(htrackmenu, IDM_DUPLICATE_ROW, MF_ENABLED);
                }
            }

            if(row >= 0)
            {
                if(col >= 0)
                {
                    GetColumnName(column, col);

                    //if the column is not read-only
                    if((iscolreadonly = IsColumnReadOnly(col)) == wyFalse && (iscolvirtual = IsColumnVirtual(col)) != 1 )
                    {
                        //check whether the column is nullable
                        iscolnullable = IsNullable(m_wnd->m_tunnel, m_data->m_fieldres, (wyChar*)column.GetString());
						
						if(!iscolnullable)
						{
							//if column type is timestamp,leave the set null key enabled as timestamp takes null and set to current timestamp
							columntype = GetDataType(m_wnd->m_tunnel, m_data->m_fieldres, (wyChar*)column.GetString());
							if(columntype.CompareI("timestamp") == 0)
							{
								iscolnullable = wyTrue;
							}
						}

                        //does the column has any defaults
                        iscolhasdefault = (GetDefaultValue(m_wnd->m_tunnel, m_data->m_fieldres, NULL, (wyChar*)column.GetString())) ? wyTrue : wyFalse;
                    }
                }
            }

            //enable/disable the menu items
            EnableMenuItem(htrackmenu, IDC_SETEMPTY, iscolreadonly == wyTrue||iscolvirtual == wyTrue? MF_GRAYED : MF_ENABLED);
            EnableMenuItem(htrackmenu, IDC_SETNULL, iscolnullable == wyFalse ? MF_GRAYED : MF_ENABLED);
            EnableMenuItem(htrackmenu, IDC_SETDEF, iscolhasdefault == wyFalse ? MF_GRAYED : MF_ENABLED); 

            //now enable the copy menu based on the button state in toolbar
            if((hwndtoolbar = GetToolBarFromID(IDM_DATATOCLIPBOARD)) && 
                SendMessage(hwndtoolbar, TB_GETSTATE, IDM_DATATOCLIPBOARD, 0) == TBSTATE_ENABLED)
            {
                SetCopyMenu(GetSubMenu(htrackmenu, copymenupos), row, col);
            }
        }
    }

    //if the reset filter in toolbar is enabled, enable filter menu
    if((hwndtoolbar = GetToolBarFromID(ID_RESETFILTER)) && 
        SendMessage(hwndtoolbar, TB_GETSTATE, ID_RESETFILTER, 0) == TBSTATE_ENABLED)
    {
        SetFilterMenu(GetSubMenu(htrackmenu, 11), row, col);
    }
	if(m_data && m_data->m_psortfilter)
		for(i = 0; i < m_data->m_psortfilter->m_sortcolumns; ++i)
		{
			if(m_data->m_psortfilter->m_sort[i].m_currsorttype != ST_NONE)
			{
				isunsort = wyTrue;
				i = m_data->m_psortfilter->m_sortcolumns;
			}
		}
	EnableMenuItem(htrackmenu, ID_UNSORT, isunsort == wyFalse ? MF_GRAYED : MF_ENABLED);

#ifndef COMMUNITY
	if(m_wnd->m_conninfo.m_isreadonly == wyTrue)
	{
		EnableMenuItem(htrackmenu, ID_RESULT_INSERT,MF_GRAYED); 
		EnableMenuItem(htrackmenu, ID_RESULT_SAVE,MF_GRAYED); 
		EnableMenuItem(htrackmenu, ID_RESULT_DELETE,MF_GRAYED); 
		EnableMenuItem(htrackmenu, ID_RESULT_CANCEL,MF_GRAYED); 
		EnableMenuItem(htrackmenu, IDC_SETEMPTY,MF_GRAYED); 
		EnableMenuItem(htrackmenu, IDC_SETDEF,MF_GRAYED); 
		EnableMenuItem(htrackmenu, IDC_SETNULL,MF_GRAYED); 
		EnableMenuItem(htrackmenu, IDM_DUPLICATE_ROW,MF_GRAYED);
	}
#endif
    //set owner draw property and show menu
    m_htrackmenu = htrackmenu;
    wyTheme::SetMenuItemOwnerDraw(m_htrackmenu);
    TrackPopupMenu(m_htrackmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt->x, pt->y, 0, m_hwndframe, NULL);
    DestroyMenu(hmenu);
    m_htrackmenu = NULL;
}

//grid context menu handler
void 
DataView::OnContextMenu(LPARAM lparam)
{
    wyInt32     row, col;
    POINT       pt = {0, 0};
    RECT        rect;

    //if there is a valid point
    if(lparam)
    {
        //get the cell that contains the point
        pt.x = GET_X_LPARAM(lparam);
        pt.y = GET_Y_LPARAM(lparam);
        CustomGrid_GetItemFromPoint(m_hwndgrid, &pt, &row, &col);
    }
    //otherwise it is invoked throgugh keyboard thus we will display the menu at currently selected row and col
    else
    {
        //get the row and column
        row = CustomGrid_GetCurSelRow(m_hwndgrid);
        col = CustomGrid_GetCurSelCol(m_hwndgrid);
        
        if(row >= 0 && col >= 0)
        {
            //find the cell right bottom corner
            CustomGrid_GetSubItemRect(m_hwndgrid, row, col, &rect);
            pt.x = rect.right;
            pt.y = rect.bottom;
        }
    }

    //map it to screen cordinates and show context menu
    MapWindowPoints(m_hwndgrid, NULL, &pt, 1);
    ShowContextMenu(row, col, &pt);
}

//function enables and sets the menu item labels for filter sub menu
void
DataView::SetFilterMenu(HMENU hmenu, wyInt32 row, wyInt32 col)
{
    wyString    temp, filtertext, colname;
    wyChar*     text;
    wyUInt32    len;
    wyInt32     count, i, existingfilterlen = 0;
    wyBool      isnull = wyFalse, isenable, isbinary;

    wyInt32     menuids[] = {
        ID_FILTER_FIELD_EQUALS, ID_FILTER_FIELD_NOTEQUALS, 
		ID_FILTER_FIELD_GREATERTHEN, ID_FILTER_FIELD_LESSTHEN, 
		ID_FILTER_FIELDLIKEBEGIN, ID_FILTER_FIELDLIKEEND, ID_FILTER_FIELDLIKEBOTH
    };

    //if no data or no sort filter
    if(!m_data || !m_data->m_psortfilter)
    {
        return;
    }

    temp.SetAs(m_data->m_psortfilter->m_currfilterstring);

    //enable reset filter if there is an existing filter applied
    if((existingfilterlen = temp.GetLength()))
    {
        EnableMenuItem(hmenu, ID_RESETFILTER, MF_ENABLED);
    }

    //if we have data and data res
    if(m_data && m_data->m_datares)
    {
        //if the selected cell is valid
        if(row != -1 && col != -1)
        {
            //get cell value
            text = GetCellValue(row, col, &len, wyFalse);

            //check if the column is binary/blob/spatial
            isbinary = (m_data->m_datares->fields[col].type == MYSQL_TYPE_BIT || m_data->m_datares->fields[col].type == MYSQL_TYPE_JSON || m_data->m_datares->fields[col].type == MYSQL_TYPE_GEOMETRY
		        || (m_data->m_datares->fields[col].type == MYSQL_TYPE_BLOB  && BlobMgmt::IsDataBinary(text, len) == wyTrue)) ? wyTrue : wyFalse;

            //dummy value
            filtertext.SetAs("value");

            //set filter text
            if(text)
            {
                if(isbinary == wyFalse)
                {
                    filtertext.Sprintf("%.*s%s", len > 20 ? 20 : len, text, len > 20 ? "..." : "");
                }
            }
            else
            {
                filtertext.SetAs("NULL");
            }

            //now check whether the filter text is NULL
            if(!filtertext.CompareI("NULL") || !filtertext.CompareI("(NULL)"))
            {
                isnull = wyTrue;
            }

            //set column name
            colname.Sprintf("`%s`", m_data->m_datares->fields[col].name);

            //loop throught the columns
            for(i = 0, count = sizeof(menuids) / sizeof(menuids[0]); i < count ; ++i)
            {
                temp.Clear();
                isenable = wyFalse;

                switch(menuids[i])
                {
                    case ID_FILTER_FIELD_EQUALS:
                        if(isnull == wyTrue)
                        {
                            temp.Sprintf("%s IS NULL", colname.GetString());
                        }
                        else
                        {
                            temp.Sprintf("%s = '%s'", colname.GetString(), filtertext.GetString());
                        }

                        isenable= wyTrue;
                        break;

                    case ID_FILTER_FIELD_NOTEQUALS:
                        if(isnull == wyTrue)
                        {
                            temp.Sprintf("%s IS NOT NULL", colname.GetString());
                        }
                        else
                        {
                            temp.Sprintf("%s <> '%s'", colname.GetString(), filtertext.GetString());
                        }

                        isenable= wyTrue;
                        break;

                    case ID_FILTER_FIELD_GREATERTHEN:
                        temp.Sprintf("%s > '%s'", colname.GetString(), filtertext.GetString());
                        isenable = isnull == wyTrue ? wyFalse : wyTrue;
                        break;

                    case ID_FILTER_FIELD_LESSTHEN:
                        temp.Sprintf("%s < '%s'", colname.GetString(), filtertext.GetString());
                        isenable = isnull == wyTrue ? wyFalse : wyTrue;
                        break;

                    case ID_FILTER_FIELDLIKEBEGIN:
                        temp.Sprintf("%s LIKE '%%%s'", colname.GetString(), filtertext.GetString());
                        isenable = isnull == wyTrue ? wyFalse : wyTrue;
                        break;

                    case ID_FILTER_FIELDLIKEEND:
                        temp.Sprintf("%s LIKE '%s%%'", colname.GetString(), filtertext.GetString());
                        isenable = isnull == wyTrue ? wyFalse : wyTrue;
                        break;

                    case ID_FILTER_FIELDLIKEBOTH:
                        temp.Sprintf("%s LIKE '%%%s%%'", colname.GetString(), filtertext.GetString());
                        isenable = isnull == wyTrue ? wyFalse : wyTrue;
                        break;
                }

                //enable the menu item, if it is not binary
                if(isenable == wyTrue && isbinary == wyFalse)
                {
                    EnableMenuItem(hmenu, menuids[i], MF_BYCOMMAND | MF_ENABLED);
                }

                //set the menu item label
                if(temp.GetLength())
                {
                    SetMenuOnPreferenceChange(hmenu, temp.GetAsWideChar(), menuids[i]);
                }
            }
        }

        //enable custom filter always
        EnableMenuItem(hmenu, ID_FILTER_CUSTOMFILTER, MF_ENABLED);
    }
}

//function to enable/disable copy sub menu
void 
DataView::SetCopyMenu(HMENU hmenu, wyInt32 row, wyInt32 col)
{
    //if we have no data or no result, disable everything
    if(!m_data || !m_data->m_datares)
    {
        EnableMenuItem(hmenu, IDM_IMEX_EXPORTCELL, MF_GRAYED);
        EnableMenuItem(hmenu, IDM_DATATOCLIPBOARD, MF_GRAYED);
        EnableMenuItem(hmenu, IDM_SELDATATOCLIPBOARD, MF_GRAYED);
    }
    else
    {
        //enable if there is a valid selection
        EnableMenuItem(hmenu, IDM_IMEX_EXPORTCELL, (row < 0 || col < 0) ? MF_GRAYED : MF_ENABLED);

        //disable it if there is no selected row or the check count is zero
        EnableMenuItem(hmenu, IDM_SELDATATOCLIPBOARD, (row < 0 && !m_data->m_checkcount) ? MF_GRAYED : MF_ENABLED);

        //disable if there is no data
        EnableMenuItem(hmenu, IDM_DATATOCLIPBOARD, (!m_data->m_rowarray->GetLength()) ? MF_GRAYED : MF_ENABLED);
    }
}

//function gets the current button image
wyInt32 
DataView::GetButtonImage(wyInt32 id, wyBool* pisorigimage)
{
    TBBUTTONINFO    tbbi = {0};
    HWND            hwndtoolbar;

    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_LPARAM | TBIF_IMAGE;
    hwndtoolbar = GetToolBarFromID(id);

    //get the toolbar button info
    SendMessage(hwndtoolbar, TB_GETBUTTONINFO, id, (LPARAM)&tbbi);

    if(pisorigimage)
    {
        //set is original image
        *pisorigimage = tbbi.lParam != tbbi.iImage ? wyFalse : wyTrue;
    }

    return tbbi.iImage;
}

//function fills the string for tooltip
wyBool 
DataView::OnToolTipInfo(LPNMTTDISPINFO lpnmtt)
{
    wyWChar string[SIZE_512] = {0};
    wyBool  isorigimage = wyTrue, ishandled = wyTrue;

    m_tooltiptext.Clear();

    //get the button image, more importantly we want to see whether it is the original image
    GetButtonImage(lpnmtt->hdr.idFrom, &isorigimage);

    //switch the id and set the curresponding tooltip
    switch(lpnmtt->hdr.idFrom)
    {
        case ID_RESULT_INSERT:
            wcscpy(string, L"Insert new row (Alt+Ins)");
            break;

	    case ID_RESULT_DELETE:
            wcscpy(string, L"Delete selected row(s) (Alt+Del)");
            break;

	    case ID_RESULT_CANCEL:
	    case ID_RESULT_SAVE:
	    case IDM_IMEX_EXPORTDATA:
	    case IDM_IMEX_EXPORTCELL:
	    case IDM_SELDATATOCLIPBOARD:
            //if the current image is not the original image, then we are executing a query, so tooltip should be Stop
            if(isorigimage == wyFalse)
            {
                wcscpy(string, L"Stop");
            }
            else
            {
		        LoadString(pGlobals->m_hinstance, lpnmtt->hdr.idFrom, string, SIZE_512 - 1);
            }
            break;

	    case IDM_DATATOCLIPBOARD:
		    wcscpy(string, L"Copy Data");
		    break;

        case IDM_DUPLICATE_ROW:
            wcscpy(string, L"Duplicate Current Row");
            break;

        case ID_VIEW_FORMVIEW:
            wcscpy(string, L"Form View");
            break;

        case ID_VIEW_TEXTVIEW:
            wcscpy(string, L"Text View");
            break;

        case ID_VIEW_GRIDVIEW:
            wcscpy(string, L"Grid View");
            break;

        case IDC_REFRESH:
            //if the current image is not the original image, then we are executing a query, so tooltip should be Stop
            wcscpy(string, isorigimage == wyFalse ? L"Stop" : L"Refresh (Alt+F5)");
            break;

        case ID_RESETFILTER:
            //show filter string if current image is not original image
            if(isorigimage == wyFalse && m_data && m_data->m_psortfilter)
            {
                m_tooltiptext.SetAs(m_data->m_psortfilter->m_currfilterstring);
                lpnmtt->lpszText = m_tooltiptext.GetAsWideChar();
                return wyTrue;
            }
            //esle set it to Filter
            else
            {
                wcscpy(string, L"Filter");
            }

            break;
		case ID_UNSORT:
			break;

        default:
            ishandled = wyFalse;
	}

    //localize the string and set it as tooltip
    m_tooltiptext.SetAs(_(string));
    lpnmtt->lpszText = m_tooltiptext.GetAsWideChar();

    return ishandled;
}

//function shows the form view
wyBool 
DataView::ShowFormView(wyInt32 show, wyBool isprobe)
{
#ifndef COMMUNITY
    //check we have valid form view pointer
    if(m_formview)
    {
        //if it is only to probe, dont show the window, just return true
        if(isprobe == wyFalse)
        {
            ShowWindow(m_formview->m_hwndhtml, show);
        }

        return wyTrue;
    }
#else
    //with community we show the ad dialog
    if(show != SW_HIDE && isprobe == wyFalse)
    {
        pGlobals->m_pcmainwin->m_connection->GetSQLyogEntDialog();
    }
#endif

    return wyFalse;
}

//function to get the ribbon color
void 
DataView::GetRibbonColorInfo(COLORREF* bg, COLORREF* fg)
{
    NMDVCOLORINFO   nmci = {0};

    //initialize the notification structure
    nmci.hdr.code = NM_GETCOLORS;
    nmci.hdr.hwndFrom = m_hwndframe;
    nmci.drawcolor.m_color1 = GetSysColor(COLOR_BTNTEXT);
    nmci.drawcolor.m_color2 = GetSysColor(COLOR_BTNFACE);

    //ask parent window to supply the colors
    SendMessage(GetParent(m_hwndframe), WM_NOTIFY, 0, (LPARAM)&nmci);

    //set the colors
    *bg = nmci.drawcolor.m_color2;
    *fg = nmci.drawcolor.m_color1;
}

//function to get the hyperlink color
void 
DataView::GetHyperLinkColorInfo(COLORREF* fg)
{
    NMDVCOLORINFO   nmci = {0};

    //initialize the notification structure
    nmci.hdr.code = NM_GETHYPERLINKCOLOR;
    nmci.hdr.hwndFrom = m_hwndframe;
    nmci.drawcolor.m_color1 = RGB(255, 0, 0);
    nmci.drawcolor.m_color2 = 0;

    //ask parent window to supply the color
    SendMessage(GetParent(m_hwndframe), WM_NOTIFY, 0, (LPARAM)&nmci);

    //set the color
    *fg = nmci.drawcolor.m_color1;
}

//function gets the active display window handle
HWND 
DataView::GetActiveDispWindow()
{
    if(IsWindowVisible(m_hwndgrid))
    {
        return m_hwndgrid;
    }
    else if(IsWindowVisible(m_hwndtext))
    {
        return m_hwndtext;
    }
#ifndef COMMUNITY
    else if(m_formview && IsWindowVisible(m_formview->m_hwndhtml))
    {
        return m_formview->m_hwndhtml;
    }
#endif

    return NULL;
}

void
DataView::OnRightClickFilter(wyInt32 action)
{
    wyInt32     col, row;
    wyUInt32    len = 0;
    wyChar*     data = "";
    HWND        hwndtool;
    RECT        rcbutton = {0};
    RECT*       prect = NULL;

    col = CustomGrid_GetCurSelCol(m_hwndgrid);   
    row = CustomGrid_GetCurSelRow(m_hwndgrid);

    if(row >= 0 && col >= 0 && row < m_data->m_rowarray->GetLength())
    {
        data = GetCellValue(row, col, &len, wyTrue);
    }

    if(action == ID_FILTER_CUSTOMFILTER)
    {
        if((hwndtool = GetToolBarFromID(ID_RESETFILTER)))
        {
            SendMessage(hwndtool, TB_GETRECT, ID_RESETFILTER, (LPARAM)&rcbutton);

            if(rcbutton.right != rcbutton.left)
            {
                MapWindowPoints(hwndtool, NULL, (LPPOINT)&rcbutton, 2);
                prect = &rcbutton;
            }
        }
    }

    if(m_data->m_psortfilter->BeginFilter(action, data, len, col, m_hwndframe, m_querybuilder, prect) == wyTrue)
    {
        Execute(TA_REFRESH, wyTrue, wyTrue, LA_FILTER);
    }
}

//draw the sort order mark in column header
wyInt32
DataView::OnGvnColumnDraw(WPARAM wparam)
{
    if(m_data->m_psortfilter)
    {
        return m_data->m_psortfilter->GetColumnSort(wparam);
    }

    return GV_UNKNOWN;
}

//sort from grid
wyBool 
DataView::OnGvnColumnClick(WPARAM wparam, LPARAM lparam)
{
    wyBool isaddtoexistingsort;

    //if no sort filter
    if(!m_data->m_psortfilter)
    {
        return wyTrue;
    }

    //if the control key is down, we add it to existing sort, else create a new sort
    isaddtoexistingsort = (GetKeyState(VK_CONTROL) & SHIFTED) ? wyTrue : wyFalse;

    //begin column sort
    m_data->m_psortfilter->BeginColoumnSort(wparam, isaddtoexistingsort);

    //execute sort
    Execute(TA_REFRESH, wyTrue, wyTrue, LA_SORT);
    return wyTrue;
}

//function to show context menu for text view
wyInt32
DataView::OnTextContextMenu(LPARAM lParam)
{
	HMENU   hmenu, htrackmenu;
	POINT   pnt;
	wyInt32 pos;
    RECT    rect;

    //if the context menu is generated from the keyboard then lParam = -1
	if(lParam == -1)
	{		
        pos = SendMessage(m_hwndtext, SCI_GETCURRENTPOS, 0, 0);
		pnt.x = SendMessage(m_hwndtext, SCI_POINTXFROMPOSITION, 0, pos) ; 
		pnt.y = SendMessage(m_hwndtext, SCI_POINTYFROMPOSITION, 0, pos); 
		ClientToScreen(m_hwndtext, &pnt);		
	}
	else
	{
		pnt.x   =   (LONG)LOWORD(lParam);
		pnt.y   =   (LONG)HIWORD(lParam);
	}

    GetClientRect(m_hwndtext, &rect);
    MapWindowPoints(m_hwndtext, NULL, (LPPOINT)&rect, 2);

    //check the point is inside the client area
    if(!PtInRect(&rect, pnt))
    {
        return -1;
    }

    //load menu and localize it
	hmenu = LoadMenu(pGlobals->m_hinstance, MAKEINTRESOURCE(IDR_INFOTABMENU));
    LocalizeMenu(hmenu);
    htrackmenu = GetSubMenu(hmenu, 0);

    if(IsWindowVisible(m_hwndtext) && IsWindowEnabled(m_hwndtext))
    {
        if(SendMessage(m_hwndtext, SCI_GETSELECTIONSTART, 0, 0) == SendMessage(m_hwndtext, SCI_GETSELECTIONEND, 0, 0))
        {
            EnableMenuItem(htrackmenu, ID_OBJECT_COPY, MF_DISABLED);
        }
        else
        {
            EnableMenuItem(htrackmenu, ID_OBJECT_COPY, MF_ENABLED);
        }
    }
    else
    {
        DisableMenuItems(htrackmenu);
    }
	m_htrackmenu = htrackmenu;
    wyTheme::SetMenuItemOwnerDraw(m_htrackmenu);

    //show menu
	TrackPopupMenu(htrackmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt.x, pnt.y, 0, m_hwndtext , NULL);

    DestroyMenu(hmenu);
    m_htrackmenu = NULL;
	return 1;
}

wyInt32
DataView::OnEditWmCommand(WPARAM wparam)
{
	switch(LOWORD(wparam))
	{
		case ID_OBJECT_COPY:
            CopyStyledTextToClipBoard(m_hwndtext);
			break;

		case ID_OBJECT_SELECTALL:
			SendMessage(m_hwndtext, SCI_SELECTALL, 0, 0);
			break;

		case ID_OBJECT_FIND:
            FindTextOrReplace(m_wnd, wyFalse);
			break;

		case ID_OBJECT_FINDNEXT:
			OnAccelFindNext(m_wnd);
			break;
	}

	return 0;
}
void 
DataView::AccelaratorSwitchForm()
{
    HWND    hwnd;
    wyInt32 row, col;

    row = CustomGrid_GetCurSelRow(m_hwndgrid);
    col = CustomGrid_GetCurSelCol(m_hwndgrid);

#ifndef COMMUNITY
    if(m_data)
    {
    if(m_data->m_viewtype == GRID)
    {
        SetDispMode(FORM);

        if(m_formview)
        {
            m_formview->ScrollColumnIntoView(col);
        }
    }
    else if(m_data->m_viewtype == FORM)
    {
        SetDispMode(GRID);
        CustomGrid_EnsureVisible(m_hwndgrid, row, col);
    }
    }
#endif

    if((hwnd = GetActiveDispWindow()))
    {
        SetFocus(hwnd);
    }
}

void
DataView::ShowTextView()
{
    HWND hwnd;

    if(m_data)
    {
        if(m_data->m_viewtype != TEXT)
        {
            m_data->m_lastview = m_viewtype;
            SetDispMode(TEXT);
        }
        else if(m_data->m_lastview >= 0)
        {
            SetDispMode((ViewType)m_data->m_lastview);
            m_data->m_lastview = -1;
        }
        else
        {
            SetDispMode(GRID);
            m_data->m_lastview = -1;
        }
    }

    if((hwnd = GetActiveDispWindow()))
    {
        SetFocus(hwnd);
    }
}

void
DataView::SwitchView(ViewType toview)
{
    ViewType    view;
    HWND        hwnd;
    wyInt32     row, col;

    row = CustomGrid_GetCurSelRow(m_hwndgrid);
    col = CustomGrid_GetCurSelCol(m_hwndgrid);

    if(toview == DUMMY)
    {
        view = m_data ? m_data->m_viewtype : m_viewtype;

        switch(view)
        {
            case GRID:
                view = ShowFormView(SW_SHOW, wyTrue) == wyTrue ? FORM : TEXT;
                break;

            case FORM:
                view = TEXT;
                break;

            case TEXT:
                view = GRID;
        }
    }
    else
    {
        if(toview == FORM && ShowFormView(SW_SHOW, wyTrue) == wyFalse)
        {
            return;
        }
        
        view = toview;
    }

    SetDispMode(view, wyTrue);

#ifndef COMMUNITY
    if(view == FORM && m_formview)
    {
        m_formview->ScrollColumnIntoView(col);
    }
    else
#endif
    if(view == GRID)
    {
        CustomGrid_EnsureVisible(m_hwndgrid, row, col);
    }
    
    if((hwnd = GetActiveDispWindow()))
    {
        SetFocus(hwnd);
    }
}

//function to set fonts for grid and text views
void
DataView::SetAllFonts()
{
    SetGridFont();
    SetTextViewFont();
}

//funtion is called when the selection change from one row to another
void        
DataView::OnGvnEndChangeRow(WPARAM wparam)
{
    //enable save and cancel buttons if there is unsaved changes
    EnableToolButton(m_data && m_data->m_modifiedrow >= 0 && m_data->m_viewtype != TEXT 
        ? wyTrue : wyFalse, ID_RESULT_SAVE);
    EnableToolButton(m_data && m_data->m_modifiedrow >= 0 && m_data->m_viewtype != TEXT 
        ? wyTrue : wyFalse, ID_RESULT_CANCEL);
    
    //enable duplicate row button if the current row can be duplicated
    EnableToolButton((IsCurrRowDuplicatable() == wyTrue && m_data->m_viewtype != TEXT) 
        ? wyTrue : wyFalse, IDM_DUPLICATE_ROW);
}

//subclassed window proc to shift the focus in refresh controls
LRESULT CALLBACK 
DataView::RefreshCtrlWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    WNDPROC wndproc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if(message == WM_KEYDOWN)
    {
        if(wparam == VK_TAB)
        {
            SetFocus(GetNextDlgTabItem(GetParent(hwnd), hwnd, GetKeyState(VK_SHIFT) & SHIFTED ? TRUE : FALSE));
        }
        else if(wparam == VK_RETURN)
        {
            PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_REFRESH, 0), (LPARAM)hwnd);
        }
    }

    return CallWindowProc(wndproc, hwnd, message, wparam, lparam);
}

//function updates the refresh control values
void
DataView::SetRefreshValues(LPEXEC_INFO pexeinfo)
{
    wyInt32 buttonstate;

    //if it is a click on limit check box
    if(pexeinfo->m_la == LA_LIMITCLICK)
    {
        //set the button check state
        buttonstate = Button_GetCheck(m_hwndislimit) == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED;
        Button_SetCheck(m_hwndislimit, buttonstate);

        //update the values
        if(buttonstate == BST_CHECKED)
		{				
            m_data->m_islimit = wyTrue;
        }
		else
		{
            m_data->m_islimit = wyFalse;
        }
    }
    //if it is next button click
    else if(pexeinfo->m_la == LA_NEXT)
    {
        //get the current limit values
        GetLimitValues();

        //calculate the start row and set it
        m_data->m_startrow = m_data->m_startrow + m_data->m_limit;
        SetLimitValues();
    }
    //previous button click
    else if(pexeinfo->m_la == LA_PREV)
    {
        //get the current limit values
        GetLimitValues();

        //update the values
        if(m_data->m_startrow < m_data->m_limit)
        {
            m_data->m_startrow = 0;
        }
        else
        {
            m_data->m_startrow = m_data->m_startrow - m_data->m_limit;
        }
    
        //set the values
        SetLimitValues();
    }

    //if the last action is not sorting and filtering, we will update the limit values
    if(pexeinfo->m_la != LA_SORT && pexeinfo->m_la != LA_FILTER)
    {
        GetLimitValues();
    }
}

//handle WM_SYSKEYDOWN from grid
wyInt32 
DataView::OnSysKeyDown(WPARAM wparam, LPARAM lparam)
{
    wyInt32 cmd = 0;
    HWND    hwndtool;

    //if ALT key is down
    if(lparam >> 29)
    {
        //check whether it is insert/delete
        switch(wparam)
        {
            case VK_INSERT:
                cmd = ID_RESULT_INSERT;
                break;

            case VK_DELETE:
                cmd = ID_RESULT_DELETE;
                break;

            case VK_F5:
                if(ThreadStopStatus())
                {
                    cmd = IDC_REFRESH;
                }

                break;
        }
    }

    //if we have a valid cmd and a valid toolbar handle
    if(cmd && (hwndtool = GetToolBarFromID(cmd)))
    {
        //check whether the button is enabled and based on that send WM_COMMAND with the id we found
        if(SendMessage(hwndtool, TB_GETSTATE, cmd, 0) == TBSTATE_ENABLED)
        {
            SendMessage(m_hwndframe, WM_COMMAND, MAKEWPARAM(cmd, 0), (LPARAM)hwndtool);
            return 0;
        }
    }

    return 1;
}

//function set the parent window handle for the frame
HWND 
DataView::SetParentWindow(HWND hwndparent)
{
    if(hwndparent)
    {
        m_hwndparent = hwndparent;
        SetParent(m_hwndframe, hwndparent);
    }

    //return the new parent
    return m_hwndparent;
}
