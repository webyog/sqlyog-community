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


#include "TabForeignKeys.h"
#include "MDIWindow.h"
#include "Global.h"
#include "Include.h"
#include "GUIHelper.h"
#include "CommonHelper.h"
#include "TableTabInterface.h"
#include "TableTabInterfaceTabMgmt.h"
#include "TabFields.h"

#define UM_SETINITFOCUS 545

extern PGLOBALS		pGlobals;

#define RELATIONERROR L"Could not create relation. The possible causes of error are -\n\n" \
L"1) The reference table does not have InnoDB/SolidDB/PBXT table handler\n" \
L"2) No index was found on selected columns. You need to explicitly create index on the selected columns\n" \
L"3) The selected columns will create incorrect foreign definition\n" \
L"4) You did not select correct columns\n" \
L"5) Selected columns were not found in the Reference Table\n" \
L"6) Constraint name already exists\n" \
L"\nPlease correct the errors."

#define EDITRELERROR _(L"There in no direct way to ALTER a Foreign Key in MySQL.\r\n" \
L"SQLyog will drop the existing one and create a new one using the existing name for the key.\r\n" \
L"If necessary preconditions for a Foreign Key (matching data types, appropriate indexes etc.) are not met, the create process will fail. " \
L"The result will be that existing FK is dropped and no new one is created.\r\n" \
L"You can avoid this by specifying a new name for the key.\r\n\r\n" \
L"Do you want to continue?")

//..Structure Destructor
StructFK::~StructFK()
{
    FKSourceColumn  *srccol = NULL, *tmpsrccol = NULL;
    FKTargetColumn  *tgtcol = NULL, *tmptgtcol = NULL;

    //..Releasing list memory
    if(m_listsrccols)
    {
        srccol = (FKSourceColumn *)m_listsrccols->GetFirst();
        while(srccol)
        {
            tmpsrccol = srccol;
            srccol = (FKSourceColumn *)m_listsrccols->Remove(srccol);
            delete tmpsrccol;
        }
        delete m_listsrccols;
    }
    m_listsrccols = NULL;

    //..Releasing list memory
    if(m_listtgtcols)
    {
        tgtcol = (FKTargetColumn*)m_listtgtcols->GetFirst();

        while(tgtcol)
        {
            tmptgtcol = (FKTargetColumn*)tgtcol;
            tgtcol = (FKTargetColumn*)m_listtgtcols->Remove(tgtcol);
            delete tmptgtcol;
        }
        delete m_listtgtcols;
    }
    m_listtgtcols = NULL;
}

//..Wrapper class constructor
FKStructWrapper::FKStructWrapper(StructFK  *value, wyBool  isnew)
{
    if(isnew)
    {
        m_oldval = NULL;
        m_newval = value;
    }
    else
    {
        m_oldval = m_newval = value;
    }
    m_errmsg = NULL;
}

//..Wrapper class destructor
FKStructWrapper::~FKStructWrapper()
{
    if(m_oldval == m_newval)
    {
        delete m_newval;
        m_oldval = m_newval = NULL;
    }
    else
    {
        if(m_oldval)
            delete m_oldval;
        if(m_newval)
            delete m_newval;
    }
    if(m_errmsg)
        delete m_errmsg;
    m_errmsg = NULL;
}

//..SrcCols Constructor
FKSourceColumn::FKSourceColumn(FieldStructWrapper *value)
{
    //..Storing Field's wrapper
    m_pcwrapobj = value;
}

TabForeignKeys::TabForeignKeys(HWND hwnd, TableTabInterfaceTabMgmt *ptabmgmt)
{
    m_hwnd              =   hwnd;
    m_ptabmgmt          =   ptabmgmt;
    m_hgridfk           =   NULL;
    m_hdlggrid          =   NULL;
    m_mdiwnd            =   GetActiveWin();
    m_ismysql41         =   m_mdiwnd->m_ismysql41;
	m_ismariadb52           =   IsMySQL564MariaDB53(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));
    m_prevtgtdb.SetAs("");

    m_lastclickfkgrid   =   -1;
    m_lastclickdlggrid  =   -1;

    m_failedwrap        =   NULL;
    m_failedwraprow     =   -1;
    m_failedwrapsrccols =   NULL;
    m_nfailedwrapsrccols=   -1;
}

TabForeignKeys::~TabForeignKeys()
{
    if(m_hgridfk)
    {
        DestroyWindow(m_hgridfk);
        m_hgridfk = NULL;
    }
}

void
TabForeignKeys::InitStructFK(StructFK *value)
{
    if(!value)
        return;

    value->m_name.Clear();
    value->m_listsrccols = NULL;
    value->m_srccols.Clear();
    value->m_tgtdb.Clear();
    value->m_tgttbl.Clear();
    value->m_listtgtcols = NULL;
    value->m_tgtcols.Clear();
    value->m_onupdate.Clear();
    value->m_ondelete.Clear();
}

wyBool
TabForeignKeys::CreateGrid()
{
    m_hgridfk  =   CreateCustomGridEx(m_hwnd, 0, 0, 0, 0, (GVWNDPROC)GridWndProc, GV_EX_ROWCHECKBOX, (LPARAM)this);

    if(!m_hgridfk)
        return wyFalse;

    ShowWindow(m_hgridfk, SW_HIDE);
    //UpdateWindow(m_hgridfk);

    return wyTrue;
}

void
TabForeignKeys::OnTabSelChanging()
{
    CustomGrid_ApplyChanges(m_hgridfk, wyTrue);
    ValidateFKs();
}

void
TabForeignKeys::OnTabSelChange()
{
    HWND    hwndarr[5] = {m_hgridfk, m_ptabmgmt->m_hwndtool, NULL};

    /// Hiding all child windows of CommonWindow
    //EnumChildWindows(m_hwnd, TableTabInterfaceTabMgmt::ShowWindowsProc, (LPARAM) harrnownd);

    if(m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
    {
        if(!m_ptabmgmt->m_tabinterfaceptr->m_isfksupported)
        {
			MessageBox(m_hwnd, _(L"The selected table does not support foreign keys.\nTable engine must be InnoDB, PBXT, SolidDB or ndbcluster (if ndbcluster engine version is greater than or equal to 7.3)."), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
			return;
        }
    }

    //..Showing FKGrid and toolbar
    EnumChildWindows(m_hwnd, TableTabInterfaceTabMgmt::ShowWindowsProc, (LPARAM)hwndarr);
    SetFocus(m_hgridfk);

    //..Hiding (some)Toolbar buttons
    SendMessage(m_ptabmgmt->m_hwndtool, TB_HIDEBUTTON, (WPARAM)IDM_SEPARATOR, (LPARAM)MAKELONG(TRUE,0));
    SendMessage(m_ptabmgmt->m_hwndtool, TB_HIDEBUTTON, (WPARAM)IDI_MOVEUP, (LPARAM)MAKELONG(TRUE,0));
    SendMessage(m_ptabmgmt->m_hwndtool, TB_HIDEBUTTON, (WPARAM)IDI_MOVEDOWN, (LPARAM)MAKELONG(TRUE,0));
}

wyBool
TabForeignKeys::InitGrid()
{
    wyInt32			counter;		// normal counter
    wyInt32			num_cols;		// number of columns
    GVCOLUMN		gvcol;			// structure used to create columns for grid
    
    wyChar		    *heading[] = {_("Constraint Name"), _("Referencing Columns"), _("Referenced Database"), 
		                          _("Referenced Table"), _("Referenced Columns"), _("On Update"), _("On Delete")};          // grid headers

    wyInt32			mask[] = {GVIF_TEXT, GVIF_TEXTBUTTON, GVIF_LIST, GVIF_LIST, GVIF_TEXTBUTTON, GVIF_LIST, GVIF_LIST};
    wyWChar         onupdateordelete[][10]  =   {L"No Action", L"Cascade", L"Set null", L"Restrict"};
    VOID		    *listtype[] = {NULL, NULL, NULL, NULL, NULL, (VOID*)onupdateordelete, (VOID*)onupdateordelete};

    wyInt32			elemsize[] = {0, 10, 0, 0, 0, 0, 0};
    wyInt32			elemcount[] = {0, 8, 0, 0, 0, 0, 0};
    wyInt32			cx[] = { 150, 150, 150, 150, 150, 150, 150}; // Default minimum column width

    wyInt32			format[] = { GVIF_LEFT, GVIF_LEFT, GVIF_LEFT, GVIF_LEFT, GVIF_LEFT, GVIF_CENTER, GVIF_CENTER};
    wyInt32			width = 0;
	HFONT			hfont;

    wyString		colname, dbname(RETAINWIDTH_DBNAME), tblname("__create_table");
    //wyBool			isretaincolumnwidth = IsRetainColumnWidth();
	
	m_ptabmgmt->m_tabinterfaceptr->SetFont(m_hgridfk);
	hfont = CustomGrid_GetColumnFont(m_hgridfk);
	num_cols = sizeof (heading)/sizeof(heading[0]);

	cx[5] = GetTextSize(L"No Action ", m_hgridfk, hfont).right + 15; //For On Update
	cx[6] = GetTextSize(L"No Action ", m_hgridfk, hfont).right + 15; //For On Delete
	
	for (counter=0; counter < num_cols ; counter++ )
    {
		colname.SetAs(heading[counter]);
		width = GetTextSize(colname.GetAsWideChar(), m_hgridfk, hfont).right + 15;
		
		memset(&gvcol, 0,sizeof(gvcol));
		
		gvcol.mask		    = mask[counter];		// Set the mask for the sturcture  i.e. what kind of column in the grid.
		gvcol.fmt		    = format[counter];		// Alignment
		gvcol.pszList	    = listtype[counter];	// set the kind list in between
		gvcol.cx		    = (width < cx[counter])? cx[counter]:width;
		gvcol.text	        = heading[counter];
		gvcol.nElemSize     = elemsize[counter];
		gvcol.nListCount    = elemcount[counter];
		gvcol.cchTextMax    = strlen(heading[counter]);

        if(counter == CHILDCOLUMNS || counter == PARENTCOLUMNS)
        {
            gvcol.pszButtonText = "...";
            gvcol.uIsButtonVis = wyTrue;
        }

		CustomGrid_InsertColumn(m_hgridfk, &gvcol);
    }

    for(int i=0; i<4; i++)
    {
        CustomGrid_InsertTextInList(m_hgridfk, ONUPDATE, onupdateordelete[i]);
        CustomGrid_InsertTextInList(m_hgridfk, ONDELETE, onupdateordelete[i]);
    }

    //..Inserting database names into Database Column CustomGrid-Listbox
    if(!InsertDBNamesIntoGridList())
        return wyFalse;

	return wyTrue;
}

wyBool
TabForeignKeys::InsertDBNamesIntoGridList()
{
    //The array which holds the db names
    wyString**      pdbarr;
    wyUInt32        ind, dbcount = 0;

    dbcount = GetDatabasesFromOB(&pdbarr);

    for(ind=0; ind<dbcount; ind++)
    {
        if(pdbarr[ind]->GetLength())
        {
            CustomGrid_InsertTextInList(m_hgridfk, PARENTDATABASE, pdbarr[ind]->GetAsWideChar());
            delete pdbarr[ind];
        }
    }
    free(pdbarr);

    return wyTrue;
}

void
TabForeignKeys::OnGVNButtonClick()
{
    CustomGrid_ApplyChanges(m_hgridfk, wyTrue);
    wyInt64 col = CustomGrid_GetCurSelCol(m_hgridfk);

    switch(col)
    {
    case CHILDCOLUMNS:
        DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_INDEXCOLUMNS), m_hgridfk, SrcColsDlgWndProc,(LPARAM)this);
        break;

    case PARENTCOLUMNS:
        ShowTargetColumns();
        break;
    }
}

void
TabForeignKeys::ShowTargetColumns()
{
    wyString tblname;
    wyUInt32 row;
    wyUInt32 col;

    row = CustomGrid_GetCurSelRow(m_hgridfk);
    col = CustomGrid_GetCurSelCol(m_hgridfk);

    GetGridCellData(m_hgridfk, row, PARENTTABLE, tblname);

    if(tblname.GetLength() == 0)
    {
        return;
    }
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_INDEXCOLUMNS), m_hgridfk, TgtColsDlgWndProc,(LPARAM)this);
}

void
TabForeignKeys::OnGVNEndLabelEdit(WPARAM wParam, LPARAM lParam)
{
    wyChar              *data = (wyChar*)lParam;
    wyUInt32            row, col;
    wyString            currentdata("");
    FKStructWrapper     *cwrapobj;
    FKTargetColumn      *tgtcol = NULL;
    StructFK            *fk = NULL;

    row = LOWORD(wParam);
    col = HIWORD(wParam);

    /// To avoid the custom-grid issue(Right-click)
    if(!(col >= 0 && col <= 6))
        return;

    if(!(row >= 0 && row <= CustomGrid_GetRowCount(m_hgridfk)))
        return;

	//from  .ini file. Refresh always
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    if(data)
        currentdata.SetAs(data);

    currentdata.RTrim();

    if(m_celldataprevval.Compare(currentdata) != 0)
    {
        if(!m_ptabmgmt->m_tabinterfaceptr->m_dirtytab)
            m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);
    }

    cwrapobj = (FKStructWrapper*) CustomGrid_GetRowLongData(m_hgridfk, row);

    if(cwrapobj)        //..if wrapper is associated
    {
        if(m_celldataprevval.Compare(currentdata) != 0)
        {
            if(currentdata.GetLength() == 0)
            {
                //..If user has deleted all values except ON UPDATE/DELETE for any fk
                if(col != ONDELETE && col != ONUPDATE && (!StructFKContainsOtherValues(cwrapobj->m_newval, col)))
                {
                    //..For existing fk, we shall set m_newval to NULL (DROP FK)
                    if(cwrapobj->m_oldval)
                    {
                        if(cwrapobj->m_newval && cwrapobj->m_newval->m_listsrccols)
                        {
                            ClearListSrcCols(cwrapobj, cwrapobj->m_newval->m_listsrccols);
                            delete cwrapobj->m_newval->m_listsrccols;
                            cwrapobj->m_newval->m_listsrccols = NULL;
                        }
                        
                        if(cwrapobj->m_oldval != cwrapobj->m_newval)    //..true, if existing fk is already modified
                            delete cwrapobj->m_newval;
                        cwrapobj->m_newval = NULL;
                    }
                    //..For newly added fk, we shall simply remove it from m_listfkwrapper
                    else if(cwrapobj->m_newval)                                                //..newly added fk
                    {
                        m_listfkwrappers.Remove(cwrapobj);
                        if(cwrapobj->m_newval->m_listsrccols)
                            ClearListSrcCols(cwrapobj, cwrapobj->m_newval->m_listsrccols);
                        delete cwrapobj->m_newval->m_listsrccols;
                        cwrapobj->m_newval->m_listsrccols = NULL;
                        delete cwrapobj;
                        cwrapobj = NULL;
                    }
                    CustomGrid_SetItemLongValue(m_hgridfk, row, CHILDCOLUMNS, (LPARAM)NULL);
                    CustomGrid_SetItemLongValue(m_hgridfk, row, PARENTCOLUMNS, (LPARAM)NULL);
                }
                else
                {
                    if(cwrapobj->m_newval)  //..Condition required to check that user has not deleted all data from the row.
                    {
                        if(cwrapobj->m_oldval == cwrapobj->m_newval)
                        {
                            //..Create a new structure if both newval and oldval are equal
                            cwrapobj->m_newval = GetDupStructFK(cwrapobj->m_oldval);
                            cwrapobj->m_newval->m_listsrccols = GetDupListSrcCols(cwrapobj->m_oldval->m_listsrccols);
                            cwrapobj->m_newval->m_listtgtcols = GetDupListTgtCols(cwrapobj->m_oldval->m_listtgtcols);
                        }

                        //..This is possible for this column only when user has pressed 'delete' on the column CHILDCOLUMNS
                        if(col == CHILDCOLUMNS)
                        {
                            //..Removing all child-columns and deleting the list and modifying the structure value
                            ClearListSrcCols(cwrapobj, cwrapobj->m_newval->m_listsrccols);
                            if(cwrapobj->m_newval->m_listsrccols)
                                delete cwrapobj->m_newval->m_listsrccols;
                            cwrapobj->m_newval->m_listsrccols = NULL;
                            cwrapobj->m_newval->m_srccols.Clear();
                        }

                        //..When user changes the targer database or targer table
                        if(col == PARENTDATABASE || col == PARENTTABLE)
                        {
                            ClearListTgtCols(cwrapobj->m_newval->m_listtgtcols);
                            cwrapobj->m_newval->m_listtgtcols = NULL;
                        }

                        //..True when user presses the 'delete' button on the PARENTCOLUMNS column
                        else if(col == PARENTCOLUMNS)
                        {
                            if(!cwrapobj->m_newval->m_listtgtcols)
                                return;

                            tgtcol = (FKTargetColumn *) cwrapobj->m_newval->m_listtgtcols->GetFirst();

                            while(tgtcol)
                            {
                                tgtcol->m_selected = wyFalse;
                                tgtcol = (FKTargetColumn *) tgtcol->m_next;;
                            }
                        }

                        CustomGrid_SetItemLongValue(m_hgridfk, row, CHILDCOLUMNS, (LPARAM) cwrapobj->m_newval->m_listsrccols);
                        CustomGrid_SetItemLongValue(m_hgridfk, row, PARENTCOLUMNS, (LPARAM) cwrapobj->m_newval->m_listtgtcols);
                    }
                }
            }
            //..When user has entered some value
            else
            {
                if(m_celldataprevval.Compare(currentdata) != 0)
                {
                    //..If value for existing fk is not modified, then create a duolicate of the m_oldval
                    if(cwrapobj->m_oldval == cwrapobj->m_newval)
                    {
                        cwrapobj->m_newval = GetDupStructFK(cwrapobj->m_oldval);
                        cwrapobj->m_newval->m_listsrccols = GetDupListSrcCols(cwrapobj->m_oldval->m_listsrccols);
                        cwrapobj->m_newval->m_listtgtcols = GetDupListTgtCols(cwrapobj->m_oldval->m_listtgtcols);
                    }
                    //..Create a new struct for m_newval when user has enter value for column other than ONUPDATE and ONDELETE and user has deleted all fk values manually from the row
                    else if(col != ONDELETE && col != ONUPDATE && (cwrapobj->m_oldval && !cwrapobj->m_newval))
                    {
                        cwrapobj->m_newval = new StructFK();
                        GetGridCellData(m_hgridfk, row, PARENTDATABASE, cwrapobj->m_newval->m_tgtdb);
                        cwrapobj->m_newval->m_listsrccols = NULL;
                        cwrapobj->m_newval->m_listtgtcols = NULL;
                        GetGridCellData(m_hgridfk, row, ONDELETE, cwrapobj->m_newval->m_ondelete);
                        GetGridCellData(m_hgridfk, row, ONUPDATE, cwrapobj->m_newval->m_onupdate);
                    }
                    
                    if(col != ONDELETE && col != ONUPDATE)
                    {
                        CustomGrid_SetItemLongValue(m_hgridfk, row, CHILDCOLUMNS, (LPARAM) cwrapobj->m_newval->m_listsrccols);
                        CustomGrid_SetItemLongValue(m_hgridfk, row, PARENTCOLUMNS, (LPARAM) cwrapobj->m_newval->m_listtgtcols);
                    }
                }
            }
        }
    }
    //..True when wrapper is not associated with the row
    else
    {
        if(currentdata.GetLength() && col != ONDELETE && col != ONUPDATE && col != PARENTDATABASE)
        {
            fk = new StructFK();
            InitStructFK(fk);
            GetGridCellData(m_hgridfk, row, PARENTDATABASE, fk->m_tgtdb);

            cwrapobj = new FKStructWrapper(fk, wyTrue);

            GetGridCellData(m_hgridfk, row, ONDELETE, fk->m_ondelete);
            GetGridCellData(m_hgridfk, row, ONUPDATE, fk->m_onupdate);

            m_listfkwrappers.Insert(cwrapobj);

            CustomGrid_SetItemLongValue(m_hgridfk, row, CHILDCOLUMNS, (LPARAM) cwrapobj->m_newval->m_listsrccols);
            CustomGrid_SetItemLongValue(m_hgridfk, row, PARENTCOLUMNS, (LPARAM) cwrapobj->m_newval->m_listtgtcols);
        }
    }

    CustomGrid_SetRowLongData(m_hgridfk, row, (LPARAM) cwrapobj);
    
    //..Setting value to the structure
    if(cwrapobj && cwrapobj->m_newval && (cwrapobj->m_oldval != cwrapobj->m_newval))
        SetValueToStructure(row, col, data);
    
    switch(col)
    {
    case PARENTDATABASE:
        OnTgtDBSelection(lParam);
        break;

    case PARENTTABLE:
        OnTgtTblEndLabelEdit(row, currentdata);
        break;

    case CHILDCOLUMNS:
        //OnSrcColEndLabelEdit(wParam, lParam);
        break;

    case PARENTCOLUMNS:
        OnTgtColEndLabelEdit(wParam, lParam);
        break;
    }

    if(cwrapobj && cwrapobj->m_newval && (!currentdata.GetLength()) && (!StructFKContainsOtherValues(cwrapobj->m_newval, col)))
    {
        if(cwrapobj->m_oldval == cwrapobj->m_newval)
            cwrapobj->m_newval = NULL;
        else
        {
            delete cwrapobj->m_newval;
	        cwrapobj->m_newval = NULL;

            if(!cwrapobj->m_oldval)
            {
                m_listfkwrappers.Remove(cwrapobj);
	            delete cwrapobj;
	            cwrapobj = NULL;
            }
        }
    }

    ScanEntireRow(row, col, currentdata);

    if(cwrapobj)
    {
        if(cwrapobj->m_oldval == cwrapobj->m_newval)
        {
            CustomGrid_SetItemLongValue(m_hgridfk, row, CHILDCOLUMNS, (LPARAM) cwrapobj->m_oldval->m_listsrccols);
            CustomGrid_SetItemLongValue(m_hgridfk, row, PARENTCOLUMNS, (LPARAM) cwrapobj->m_oldval->m_listtgtcols);
        }
        else if(cwrapobj->m_newval)
        {
            CustomGrid_SetItemLongValue(m_hgridfk, row, CHILDCOLUMNS, (LPARAM) cwrapobj->m_newval->m_listsrccols);
            CustomGrid_SetItemLongValue(m_hgridfk, row, PARENTCOLUMNS, (LPARAM) cwrapobj->m_newval->m_listtgtcols);
        }
        else
        {
            CustomGrid_SetItemLongValue(m_hgridfk, row, CHILDCOLUMNS, (LPARAM) NULL);
            CustomGrid_SetItemLongValue(m_hgridfk, row, PARENTCOLUMNS, (LPARAM) NULL);
        }
    }
    CustomGrid_SetRowLongData(m_hgridfk, row, (LPARAM) cwrapobj);
}

void
TabForeignKeys::OnTgtColEndLabelEdit(WPARAM wParam, LPARAM lParam)
{
    wyUInt32            row, col;
    FKStructWrapper     *cwrapobj;
    wyChar              *data = (wyChar*)lParam;
    wyString            curdata("");

    row = LOWORD(wParam);
    col = HIWORD(wParam);

    if(data)
        curdata.SetAs(data);

    cwrapobj = (FKStructWrapper*) CustomGrid_GetRowLongData(m_hgridfk, row);

    if(!curdata.GetLength())
    {
        if(cwrapobj && (cwrapobj->m_newval) && (cwrapobj->m_oldval != cwrapobj->m_newval))
        {
            FKTargetColumn  *tgtcol = NULL;
            cwrapobj->m_newval->m_tgtcols.Clear();

            if(cwrapobj->m_newval->m_listtgtcols)
                tgtcol = (FKTargetColumn *)cwrapobj->m_newval->m_listtgtcols->GetFirst();

            while(tgtcol)
            {
                tgtcol->m_selected = wyFalse;               //..don't remove from list.. Only unset m_selected variable
                tgtcol = (FKTargetColumn *)tgtcol->m_next;
            }

            //ClearListTgtCols(cwrapobj->m_newval->m_listtgtcols);

            //delete cwrapobj->m_newval->m_listtgtcols;
            //cwrapobj->m_newval->m_listtgtcols = NULL;
        }
        CustomGrid_SetText(m_hgridfk, row, col, "");
    }

}

void
TabForeignKeys::OnSrcColEndLabelEdit(WPARAM wParam, LPARAM lParam)
{
    LPARAM            row, col;
    FKStructWrapper     *cwrapobj;
    wyChar              *data = (wyChar*)lParam;
    wyString            curdata("");

    row = LOWORD(wParam);
    col = HIWORD(wParam);

    if(data)
        curdata.SetAs(data);

    cwrapobj = (FKStructWrapper*) CustomGrid_GetRowLongData(m_hgridfk, row);

    if(!curdata.GetLength())
    {
        if(cwrapobj && (cwrapobj->m_newval) && (cwrapobj->m_oldval != cwrapobj->m_newval))
        {
            cwrapobj->m_newval->m_srccols.Clear();
            
            ClearListSrcCols(cwrapobj, cwrapobj->m_newval->m_listsrccols);
            delete cwrapobj->m_newval->m_listsrccols;
            cwrapobj->m_newval->m_listsrccols = NULL;
        }
        CustomGrid_SetItemLongValue(m_hgridfk, row, col, (LPARAM)NULL);
    }
}

wyBool
TabForeignKeys::StructFKContainsOtherValues(StructFK *value, wyInt32 col)
{
    if(!value)
        return wyFalse;

    switch(col)
    {
    case CONSTRAINT:
        if(value->m_srccols.GetLength() || /* value->m_tgtdb.GetLength() || */value->m_tgttbl.GetLength() || value->m_tgtcols.GetLength())
            return wyTrue;
        break;

    case CHILDCOLUMNS:
        if(value->m_name.GetLength() || /*value->m_tgtdb.GetLength() || */value->m_tgttbl.GetLength() || value->m_tgtcols.GetLength())
            return wyTrue;
        break;

    case PARENTDATABASE:
        if(value->m_name.GetLength() || value->m_srccols.GetLength() || value->m_tgttbl.GetLength() || value->m_tgtcols.GetLength())
            return wyTrue;
        break;

    case PARENTTABLE:
        if(value->m_name.GetLength() || /*value->m_tgtdb.GetLength() || */value->m_srccols.GetLength() || value->m_tgtcols.GetLength())
            return wyTrue;
        break;

    case PARENTCOLUMNS:
        if(value->m_name.GetLength() || /*value->m_tgtdb.GetLength() || */value->m_tgttbl.GetLength() || value->m_srccols.GetLength())
            return wyTrue;
        break;
        
    case ONDELETE:
        if(value->m_name.GetLength() || /*value->m_tgtdb.GetLength() || */value->m_tgttbl.GetLength() || value->m_tgtcols.GetLength() || value->m_srccols.GetLength())
            return wyTrue;
        break;

    case ONUPDATE:
        if(value->m_name.GetLength() || /*value->m_tgtdb.GetLength() || */value->m_tgttbl.GetLength() || value->m_tgtcols.GetLength() || value->m_srccols.GetLength())
            return wyTrue;
        break;   
    }

    return wyFalse;
}

void
TabForeignKeys::OnTgtDBSelection(LPARAM lparam)
{
    wyChar*     dbname = (wyChar*) lparam;
    wyString    newtgtdb;
    wyUInt32    row = CustomGrid_GetCurSelRow(m_hgridfk);
    FKStructWrapper *cwrapobj = NULL;

    cwrapobj = (FKStructWrapper*) CustomGrid_GetRowLongData(m_hgridfk, row);

    newtgtdb.SetAs(dbname);
    if(newtgtdb.CompareI(m_prevtgtdb) != 0)
    {
        //..Clear PARENTTABLE and PARENTCOLUMNS column values from the grid-row
        CustomGrid_SetText(m_hgridfk, row, PARENTTABLE, "");
        CustomGrid_SetText(m_hgridfk, row, PARENTCOLUMNS, "");

        if((cwrapobj) && (cwrapobj->m_newval) && (cwrapobj->m_oldval != cwrapobj->m_newval))
        {
            cwrapobj->m_newval->m_tgttbl.Clear();
            cwrapobj->m_newval->m_tgtcols.Clear();
            ClearListTgtCols(cwrapobj->m_newval->m_listtgtcols);
            delete cwrapobj->m_newval->m_listtgtcols;
            cwrapobj->m_newval->m_listtgtcols = NULL;
            
            CustomGrid_SetItemLongValue(m_hgridfk, row, PARENTCOLUMNS, (LPARAM) NULL);
            CustomGrid_SetRowLongData(m_hgridfk, row, (LPARAM) cwrapobj);
        }
    }
}

void
TabForeignKeys::OnSrcColsDlgIDOK(HWND hwnd)
{
    wyInt32                 count, row, col;
    List                    *listfksrccols = NULL;
    FKStructWrapper         *cwrapobj = NULL;
    StructFK                *fk = NULL;
    ReferencedBy            *refby = NULL;
    FKSourceColumn          *srccol = NULL, *tmpsrccol = NULL;
    FieldStructWrapper      *fieldwrap = NULL;
    //wyBool                  markasdirty = wyFalse;
    wyString                refcols(""), tmpcolname;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    CustomGrid_ApplyChanges(m_hdlggrid);

    row = CustomGrid_GetCurSelRow(m_hgridfk);
    col = CustomGrid_GetCurSelCol(m_hgridfk);

    cwrapobj = (FKStructWrapper*) CustomGrid_GetRowLongData(m_hgridfk, row);
    listfksrccols = (List*) CustomGrid_GetItemLongValue(m_hgridfk, row, col);

    if(!cwrapobj)      //..Create/Alter table, adding a new foreign key - (selecting source(child)-columns only, when other values in the row are NULL)
    {
        fk = new StructFK;
        InitStructFK(fk);
        GetGridCellData(m_hgridfk, row, PARENTDATABASE, fk->m_tgtdb);

        listfksrccols = new List();
        fk->m_listsrccols = listfksrccols;

        cwrapobj = new FKStructWrapper(fk, wyTrue);
        m_listfkwrappers.Insert(cwrapobj);
    }
    else if(cwrapobj->m_newval == cwrapobj->m_oldval)       //..unchanged existing foreign key [Alter table only]
    {
        srccol = (FKSourceColumn*)listfksrccols->GetFirst();

        while(srccol)
        {
            tmpsrccol = srccol;
            refby = (ReferencedBy*) srccol->m_pcwrapobj->m_listfkworkingcopy.GetFirst();

            while(refby)
            {
                if(refby->m_pfkwrap = cwrapobj)
                {
                    srccol->m_pcwrapobj->m_listfkworkingcopy.Remove(refby);
                    delete refby;
                    break;
                }
                refby = (ReferencedBy*)refby->m_next;
            }
            srccol = (FKSourceColumn*)srccol->m_next;
        }
        
        cwrapobj->m_newval = GetDupStructFK(cwrapobj->m_oldval);
        cwrapobj->m_newval->m_listsrccols = listfksrccols = new List();//GetDupListSrcCols(cwrapobj->m_oldval->m_listsrccols);
        cwrapobj->m_newval->m_listtgtcols = GetDupListTgtCols(cwrapobj->m_oldval->m_listtgtcols);
    }
    else if(!listfksrccols)  //.. Alter/Create table : added a new fk but not selected source columns yet
    {
        listfksrccols = new List();
        
        if(!cwrapobj->m_newval && cwrapobj->m_oldval)
        {
            cwrapobj->m_newval = new StructFK();
            InitStructFK(cwrapobj->m_newval);
            GetGridCellData(m_hgridfk, row, PARENTDATABASE, cwrapobj->m_newval->m_tgtdb);
        }
        cwrapobj->m_newval->m_listsrccols = listfksrccols;
    }
    else                        //.. (Alter/Crate table : user has already selected index-columns and again he is modifying)
    {
        ClearListSrcCols(cwrapobj, listfksrccols);
    }
    
    count = CustomGrid_GetRowCount(m_hdlggrid);
    for(int i=0; i<count; i++)
    {
        if(!CustomGrid_GetRowCheckState(m_hdlggrid, i))
            continue;

        fieldwrap = (FieldStructWrapper*)CustomGrid_GetRowLongData(m_hdlggrid, i);

        srccol = new FKSourceColumn(fieldwrap);

        tmpcolname.SetAs(fieldwrap->m_newval->m_name);
        //..Escaping Backtick
        tmpcolname.FindAndReplace("`", "``");
        refcols.AddSprintf("%s%s%s, ", m_backtick, tmpcolname.GetString(), m_backtick);
        listfksrccols->Insert(srccol);

        refby = new ReferencedBy(cwrapobj);
        fieldwrap->m_listfkworkingcopy.Insert(refby);
    }

    refcols.RTrim();

    if(refcols.GetLength() == 0)
    {
        //..if wrapper is attached and newval is not deleted.
        if(cwrapobj && cwrapobj->m_newval)
        {
            //..If no other value is there in the row
            if(!StructFKContainsOtherValues(cwrapobj->m_newval, col))
            {
                if(cwrapobj->m_oldval)
                {
                    if(cwrapobj->m_newval && cwrapobj->m_newval != cwrapobj->m_oldval)
                    {
                        ClearListTgtCols(cwrapobj->m_newval->m_listtgtcols);
                        delete cwrapobj->m_newval;
                    }
                    cwrapobj->m_newval = NULL;
                }
                else if(cwrapobj->m_newval)
                {
                    m_listfkwrappers.Remove(cwrapobj);
                    ClearListSrcCols(cwrapobj, cwrapobj->m_newval->m_listsrccols);
                    ClearListTgtCols(cwrapobj->m_newval->m_listtgtcols);

                    delete cwrapobj;
                    cwrapobj = NULL;
                }
                
                CustomGrid_SetRowLongData(m_hgridfk, row, (LPARAM) cwrapobj);
                CustomGrid_SetItemLongValue(m_hgridfk, row, CHILDCOLUMNS, (LPARAM)NULL);
                CustomGrid_SetItemLongValue(m_hgridfk, row, PARENTCOLUMNS, (LPARAM)NULL);
                CustomGrid_SetText(m_hgridfk, row, col, "");
                return;
            }
        }
    }


    while(refcols.GetLength() && refcols.GetCharAt(refcols.GetLength() - 1) == ',')
        refcols.Strip(1);

    //cindexeswrap->m_newval->m_name.SetAs(refcols);
    CustomGrid_SetText(m_hgridfk, row, col, refcols.GetString());
    cwrapobj->m_newval->m_srccols.SetAs(refcols);

    ScanEntireRow(row, col, refcols);

    if((!cwrapobj->m_oldval)    &&     //..This section checks for any value in the index. if no value is there and it's a new fk, then the associated wrapper object will be dropped
        cwrapobj->m_newval      && 
        cwrapobj->m_newval->m_name.GetLength() == 0     &&
        cwrapobj->m_newval->m_srccols.GetLength() == 0  &&
        cwrapobj->m_newval->m_tgtdb.GetLength() == 0    &&
        cwrapobj->m_newval->m_tgttbl.GetLength() == 0   &&
        cwrapobj->m_newval->m_tgtcols.GetLength() == 0
        )
    {
        m_listfkwrappers.Remove(cwrapobj);
        delete cwrapobj;
        cwrapobj = NULL;
    }

    //cwrapobj = ManageWrapperForNewAndOldVal(cwrapobj);
    listfksrccols = (cwrapobj && cwrapobj->m_newval) ? cwrapobj->m_newval->m_listsrccols : NULL;

    CustomGrid_SetItemLongValue(m_hgridfk, row, col, (LPARAM) listfksrccols);
    CustomGrid_SetItemLongValue(m_hgridfk, row, PARENTCOLUMNS, (LPARAM) ((cwrapobj && cwrapobj->m_newval) ? cwrapobj->m_newval->m_listtgtcols : NULL));
    CustomGrid_SetRowLongData(m_hgridfk, row, (LPARAM)cwrapobj);

    if(refcols.GetLength() && (row == CustomGrid_GetRowCount(m_hgridfk) - 1))
    {
        InsertRow();
    }

    if(!m_ptabmgmt->m_tabinterfaceptr->m_dirtytab)
        m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);

    ValidateFKs();
}

wyInt32
TabForeignKeys::InsertRow()
{
    wyInt32 row;
    wyString dbname;

    m_ptabmgmt->m_tabinterfaceptr->GetComboboxValue(m_ptabmgmt->m_tabinterfaceptr->m_hcmbdbname, dbname);

    row = CustomGrid_InsertRow(m_hgridfk, wyTrue);
    CustomGrid_SetButtonVis(m_hgridfk, row, CHILDCOLUMNS, wyTrue);
    CustomGrid_SetButtonText(m_hgridfk, row, CHILDCOLUMNS, L"...");
    CustomGrid_SetButtonVis(m_hgridfk, row, PARENTCOLUMNS, wyTrue);
    CustomGrid_SetButtonText(m_hgridfk, row, PARENTCOLUMNS, L"...");

    if(CustomGrid_FindTextInList(m_hgridfk, PARENTDATABASE, dbname.GetAsWideChar()) != LB_ERR)
        CustomGrid_SetText(m_hgridfk, row, PARENTDATABASE, (wyChar*)dbname.GetString());

    //..do not set current selection here(in this function)
    //CustomGrid_SetCurSelection(m_hgridfk, row, CHILDCOLUMNS);
    return row;
}

FKStructWrapper*
TabForeignKeys::ManageWrapperForNewAndOldVal(FKStructWrapper* cwrapobj)
{
    //..This section checks for the equality of new and old values of index. If both are same, then we shall delete the newval and will assign oldval to newval
    //..Applicable to Alter table only
    if(cwrapobj->m_oldval &&                                                                         //..If all newval and oldval values are equal, then drop the newval;
        cwrapobj->m_oldval->m_name.CompareI(cwrapobj->m_newval->m_name) == 0 &&                  
        cwrapobj->m_oldval->m_srccols.CompareI(cwrapobj->m_newval->m_srccols) == 0 &&
        cwrapobj->m_oldval->m_tgtdb.CompareI(cwrapobj->m_newval->m_tgtdb) == 0 &&
        cwrapobj->m_oldval->m_tgttbl.CompareI(cwrapobj->m_newval->m_tgttbl) == 0 &&
        cwrapobj->m_oldval->m_tgtcols.CompareI(cwrapobj->m_newval->m_tgtcols) == 0 &&
        cwrapobj->m_oldval->m_onupdate.CompareI(cwrapobj->m_newval->m_onupdate) == 0 &&
        cwrapobj->m_oldval->m_ondelete.CompareI(cwrapobj->m_newval->m_ondelete) == 0
        )
    {
        FKSourceColumn *srccolsold = NULL, *srccolsnew = NULL;

        srccolsold = (FKSourceColumn*) cwrapobj->m_oldval->m_listsrccols->GetFirst();
        srccolsnew = (FKSourceColumn*) cwrapobj->m_newval->m_listsrccols->GetFirst();

        while(srccolsold && srccolsnew)
        {
            if(srccolsold->m_pcwrapobj != srccolsnew->m_pcwrapobj)
                break;

            srccolsold = (FKSourceColumn*) srccolsold->m_next;
            srccolsnew = (FKSourceColumn*) srccolsnew->m_next;
        }

        if(!srccolsold && !srccolsnew)
        {
            ClearListSrcCols(NULL, cwrapobj->m_newval->m_listsrccols);
            delete cwrapobj->m_newval->m_listsrccols;
            cwrapobj->m_newval->m_listsrccols = NULL;

            if(cwrapobj->m_newval->m_listtgtcols)
            {
                ClearListTgtCols(cwrapobj->m_newval->m_listtgtcols);
                delete cwrapobj->m_newval->m_listtgtcols;
                cwrapobj->m_newval->m_listtgtcols = NULL;
            }
            delete cwrapobj->m_newval;
            cwrapobj->m_newval = cwrapobj->m_oldval;
        }
    }
    else if((!cwrapobj->m_oldval)    &&     //..This section checks for any value in the index. if no value is there and it's a new fk, then the associated wrapper object will be dropped
        cwrapobj->m_newval      && 
        cwrapobj->m_newval->m_name.GetLength() == 0     &&
        cwrapobj->m_newval->m_srccols.GetLength() == 0  &&
        cwrapobj->m_newval->m_tgtdb.GetLength() == 0    &&
        cwrapobj->m_newval->m_tgttbl.GetLength() == 0   &&
        cwrapobj->m_newval->m_tgtcols.GetLength() == 0
        )
    {
        m_listfkwrappers.Remove(cwrapobj);
        delete cwrapobj;
        cwrapobj = NULL;
    }

    return cwrapobj;
}

void
TabForeignKeys::OnButtonUpDown(wyBool   up)
{
    wyInt32                 selrow = CustomGrid_GetCurSelRow(m_hdlggrid);
    wyInt32                 selcol = CustomGrid_GetCurSelCol(m_hdlggrid);
    wyUInt32                beforerow = -1;
    void                    *pvoid1 = NULL, *pvoid2 = NULL;
    wyBool                  checkval1, checkval2;
    wyString                colname1, datatype1, colname2, datatype2;

    if(selrow == -1)
        return;

    if(CustomGrid_GetRowCheckState(m_hdlggrid, selrow))
        checkval1 = wyTrue;
    else
        checkval1 = wyFalse;

    pvoid1 = (void*) CustomGrid_GetRowLongData(m_hdlggrid, selrow);
    GetGridCellData(m_hdlggrid, selrow, 0, colname1);
    GetGridCellData(m_hdlggrid, selrow, 1, datatype1);
    
    if(up)
    {
        beforerow = selrow - 1;
    }
    else
    {
        beforerow = selrow + 1;
    }

    if(beforerow == -1)
        return;

    if(CustomGrid_GetRowCheckState(m_hdlggrid, beforerow))
        checkval2 = wyTrue;
    else
        checkval2 = wyFalse;

    pvoid2 = (void*) CustomGrid_GetRowLongData(m_hdlggrid, beforerow);
    GetGridCellData(m_hdlggrid, beforerow, 0, colname2);
    GetGridCellData(m_hdlggrid, beforerow, 1, datatype2);

    CustomGrid_SetRowCheckState(m_hdlggrid, beforerow, checkval1);
    CustomGrid_SetText(m_hdlggrid, beforerow, 0, (wyChar*)colname1.GetString());
    CustomGrid_SetText(m_hdlggrid, beforerow, 1, (wyChar*)datatype1.GetString());

    CustomGrid_SetRowCheckState(m_hdlggrid, selrow, checkval2);
    CustomGrid_SetText(m_hdlggrid, selrow, 0, (wyChar*)colname2.GetString());
    CustomGrid_SetText(m_hdlggrid, selrow, 1, (wyChar*)datatype2.GetString());

    CustomGrid_SetRowLongData(m_hdlggrid, beforerow, (LPARAM) pvoid1);
    CustomGrid_SetRowLongData(m_hdlggrid, selrow, (LPARAM) pvoid2);

    CustomGrid_EnsureVisible(m_hdlggrid, beforerow, selcol);
    CustomGrid_SetCurSelection(m_hdlggrid, beforerow, selcol, wyTrue);
    
    EnableDisableUpDownButtons(m_hdlggrid, beforerow);

    if(beforerow == (CustomGrid_GetRowCount(m_hdlggrid) - 1))
    {
        SetFocus(GetDlgItem(GetParent(m_hdlggrid), IDC_MOVEUP));
    }
    else if(beforerow  == 0)
    {
        SetFocus(GetDlgItem(GetParent(m_hdlggrid), IDC_MOVEDOWN));
    }
}

void 
TabForeignKeys::OnSrcColsDlgWMCommand(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    switch(LOWORD(wparam))
    {
    case IDOK:
        {
            if(m_hdlggrid)
                OnSrcColsDlgIDOK(hwnd);
            SendMessage(hwnd, WM_CLOSE, wparam, lparam);
        }
        break;

    case IDCANCEL:
        {
        SendMessage(hwnd, WM_CLOSE, wparam, lparam);
        }
        break;

    case IDC_MOVEUP:
        if(m_hdlggrid)
            OnButtonUpDown(wyTrue);
        break;

    case IDC_MOVEDOWN:
        if(m_hdlggrid)
            OnButtonUpDown(wyFalse);
        break;
    }
}

List*
TabForeignKeys::GetListOfFields(wyString &dbname, wyString &tblname)
{
    PKEYSINFO       keyinfo = NULL;
    PKEYCOLSINFO    keycolinfo;
    List            *list = new List();
    FKTargetColumn  *tgtcol = NULL, *tmptgtcol = NULL;
    wyString        query, myrowstr;
    wyString        localdbname, localtblname;
    wyBool          error = wyFalse;
    wyBool          flag;
    
    localdbname.SetAs(dbname);
    localtblname.SetAs(tblname);

    localdbname.FindAndReplace("`", "``");
    localtblname.FindAndReplace("`", "``");

    keyinfo = FetchKeysInfo(localdbname, localtblname, error);

    for(int i=0; i<m_totkeys; i++)
    {
        keycolinfo = keyinfo[i].m_colinfo;
        
        while(keycolinfo)
        {
            tmptgtcol = (FKTargetColumn*) list->GetFirst();

            flag = wyTrue;
            while(tmptgtcol)
            {
                if(tmptgtcol->m_name.Compare(keycolinfo->m_colname) == 0)
                {
                    flag = wyFalse;
                    break;
                }
                tmptgtcol = (FKTargetColumn*) tmptgtcol->m_next;
            }
            if(flag)
            {
                tgtcol = new FKTargetColumn();
                tgtcol->m_name.SetAs(keycolinfo->m_colname);
                tgtcol->m_datatype.SetAs(keycolinfo->m_datatype);
                tgtcol->m_selected = wyFalse;
                list->Insert(tgtcol);
            }
            keycolinfo = keycolinfo->m_next;
        }
    }

    //..Releasing keyinfo memory
    for(int i=0; i<m_totkeys; i++)
    {
        PKEYCOLSINFO keycolsinfo, tmpkeycolsinfo; 
        keycolsinfo = keyinfo[i].m_colinfo;
        while(keycolsinfo)
        {
            tmpkeycolsinfo = keycolsinfo;
            keycolsinfo = keycolsinfo->m_next;
            delete tmpkeycolsinfo;
        }
    }
    delete keyinfo;

    return list;
}

wyBool
TabForeignKeys::FetchTgtTblCols(wyInt32 row, wyString& tblnamestr)
{
    wyString        dbnamestr, localtblnamestr, prevtblnamestr, tgtcolsstr("");
    wyString        colnamestr;
    List            *listkeycolumns = NULL;
    FKStructWrapper *cwrapobj = NULL;
    FKTargetColumn  *tgtcol = NULL;
    wyBool          retval = wyTrue, error = wyFalse;
    PKEYSINFO       keyinfo = NULL;

    /// Getting database name and table name from the row
    GetGridCellData(m_hgridfk, row, PARENTDATABASE, dbnamestr);
    localtblnamestr.SetAs(tblnamestr);

    dbnamestr.FindAndReplace("`", "``");
    localtblnamestr.FindAndReplace("`", "``");

    /// Fetching Key-columns of the target table
    keyinfo = FetchKeysInfo(dbnamestr, localtblnamestr, error);

    if(error)
        return wyFalse;

    if(!keyinfo)
        return wyTrue;

    /// Comparing target-table keys with source-columns. If datatypes of source-columns match with any of the key, then select that key as default (Auto-selection of the key-columns)
    listkeycolumns = CompareWithSourceColumns(keyinfo);
    if(!listkeycolumns)
        return wyFalse;

    cwrapobj = (FKStructWrapper*) CustomGrid_GetRowLongData(m_hgridfk, row);

    if(cwrapobj && cwrapobj->m_newval && cwrapobj->m_newval != cwrapobj->m_oldval)
        cwrapobj->m_newval->m_listtgtcols = listkeycolumns;

    CustomGrid_SetItemLongValue(m_hgridfk, row, PARENTCOLUMNS, (LPARAM) listkeycolumns);

    /// Forming the string of the selected target-columns
    tgtcol = (FKTargetColumn*)listkeycolumns->GetFirst();
    while(tgtcol)
    {
        if(tgtcol->m_selected)
        {
            colnamestr.SetAs(tgtcol->m_name);
            colnamestr.FindAndReplace("`", "``");
            tgtcolsstr.AddSprintf("%s%s%s, ", m_backtick, colnamestr.GetString(), m_backtick);
        }
        tgtcol = (FKTargetColumn*)tgtcol->m_next;
    }

    tgtcolsstr.RTrim();
    while(tgtcolsstr.GetLength() && tgtcolsstr.GetCharAt(tgtcolsstr.GetLength()-1) == ',')
        tgtcolsstr.Strip(1);

    CustomGrid_SetText(m_hgridfk, row, PARENTCOLUMNS, (wyChar*)tgtcolsstr.GetString());

    /// Setting the structure value
    if(cwrapobj && cwrapobj->m_newval && cwrapobj->m_newval != cwrapobj->m_oldval)
    {
        cwrapobj->m_newval->m_tgtcols.SetAs(tgtcolsstr);
    }

    return retval;
}

PKEYSINFO
TabForeignKeys::FetchKeysInfo(wyString& dbnamestr, wyString& tblnamestr, wyBool &error)
{
    wyUInt32        totkeys = 0, seq;
    wyString        query, prevkeyname, keyname;
    PKEYSINFO       keyinfo = NULL;
    PKEYCOLSINFO    keycolsinfo = NULL, tempkeycolsinfo = NULL;
    MYSQL_RES	    *mykeyres;
    MYSQL_ROW	    mykeyrow;
    wyInt32         indkeyname = -1, indcolname = -1;
    
    m_totkeys = 0;      //Setting totkeys to 0;
    
    query.Sprintf("show keys from `%s`.`%s`", dbnamestr.GetString(), tblnamestr.GetString());

    ///Fetching key-columns for the table
    mykeyres = ExecuteAndGetResult(GetActiveWin(), m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query);
	if(!mykeyres)
	{
        ShowMySQLError(m_hwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query.GetString());
        error = wyTrue;
		return NULL;
	}
    
    seq = 0;
    
    indkeyname = GetFieldIndex(mykeyres, "Key_name", m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));
    indcolname = GetFieldIndex(mykeyres, "Column_name", m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));

    if(indkeyname == -1 || indcolname == -1)
    {
        error = wyTrue;
        return NULL;
    }
    ///Looping through result row-by-row (all key-columns)
    while(mykeyrow = m_mdiwnd->m_tunnel->mysql_fetch_row(mykeyres))
    {
        
        if(mykeyrow[indkeyname])
            keyname.SetAs(mykeyrow[indkeyname]);

        ///True when the row fetched is either the first index or the next index
        if(keyname.GetLength() && keyname.CompareI(prevkeyname)!= 0)
        {
            prevkeyname.SetAs(keyname);
            if(keycolsinfo)
            {
                keyinfo[totkeys-1].m_colinfo = keycolsinfo;
                tempkeycolsinfo = NULL;
                keycolsinfo = NULL;
            }
            seq = 1;
            if(!keyinfo)
                keyinfo = (PKEYSINFO)malloc(sizeof(KEYSINFO));
            else
                keyinfo = (PKEYSINFO)realloc(keyinfo, sizeof(KEYSINFO) * (totkeys + 1));

            new(keyinfo + totkeys) KEYSINFO;

            keyinfo[totkeys].m_colinfo = NULL;
            keyinfo[totkeys].m_keyname.SetAs(keyname);

            totkeys++;
        }

        if(!keycolsinfo)
        {
            keycolsinfo = new KEYCOLSINFO;
            tempkeycolsinfo = keycolsinfo;
        }
        else
        {
            tempkeycolsinfo = keycolsinfo;

            while(tempkeycolsinfo->m_next)
                tempkeycolsinfo = tempkeycolsinfo->m_next;

            tempkeycolsinfo->m_next = new KEYCOLSINFO;
            tempkeycolsinfo = tempkeycolsinfo->m_next;
        }

        tempkeycolsinfo->m_next = NULL;
        tempkeycolsinfo->m_sequenceinindex = seq;
        if(mykeyrow[indcolname])
            tempkeycolsinfo->m_colname.SetAs(mykeyrow[indcolname]);
        tempkeycolsinfo->m_datatype.SetAs("");
        tempkeycolsinfo->m_size = -1;
        
        seq++;
    }

    ///Setting KeyInfo when there is any index-column and the mykeyres reaches the EOF
    if(totkeys && keycolsinfo)
        keyinfo[totkeys-1].m_colinfo = keycolsinfo;

    m_totkeys = totkeys;

    mysql_free_result(mykeyres);

    ///Getting datatypes of the key-columns
    if(m_totkeys > 0)
        GetKeyColumnsDatatypes(dbnamestr, tblnamestr, keyinfo);

    error = wyFalse;

    return keyinfo;
}

wyBool
TabForeignKeys::GetKeyColumnsDatatypes(wyString& dbnamestr, wyString& tblnamestr, PKEYSINFO keyinfo)
{
    wyString        query;
    wyString        size;
    wyString        columnname("");
    PKEYCOLSINFO    keycolsinfo = NULL;
    MYSQL_RES	    *myfieldres;
    MYSQL_ROW	    myfieldrow;
    wyChar*         tok = NULL;

    query.Sprintf("show full fields from `%s`.`%s`", dbnamestr.GetString(), tblnamestr.GetString());

    myfieldres = ExecuteAndGetResult(GetActiveWin(), m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query);
	if(!myfieldres)
	{
        ShowMySQLError(m_hwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query.GetString());
		return wyFalse;
	}

    while(myfieldrow = m_mdiwnd->m_tunnel->mysql_fetch_row(myfieldres))
    {
        if(myfieldrow[0])
            columnname.SetAs(myfieldrow[0], m_ismysql41);

        for(int i=0; i< m_totkeys; i++)
        {
            keycolsinfo = keyinfo[i].m_colinfo;

            while(keycolsinfo)
            {
                if(keycolsinfo->m_colname.CompareI(columnname) == 0)
                {
                    if(myfieldrow[1])
                    {
                        keycolsinfo->m_datatype.SetAs(myfieldrow[1]);
                        
                        tok = (wyChar*)keycolsinfo->m_datatype.GetString();
                        tok = (wyChar*)strtok(tok, "(");
                        tok = (wyChar*)strtok(NULL, ")");
                        
                        if(tok)
                        {
                            size.SetAs(tok);
                            keycolsinfo->m_size = size.GetAsInt32();
                        }
                    }
                }
                keycolsinfo = keycolsinfo->m_next;
            }
        }
    }
    mysql_free_result(myfieldres);
    return wyTrue;
}

wyBool
TabForeignKeys::AreInternalDatatypesSame(wyString &datatype1, wyString &datatype2)
{
    if((datatype1.CompareI("char") == 0) || (datatype1.CompareI("varchar") == 0))
    {
        if((datatype2.CompareI("char") == 0) || (datatype2.CompareI("varchar") == 0))
            return wyTrue;
    }
    else if((datatype1.CompareI("binary") == 0) || (datatype1.CompareI("varbinary") == 0))
    {
        if((datatype2.CompareI("binary") == 0) || (datatype2.CompareI("varbinary") == 0))
            return wyTrue;
    }
    else if((datatype1.CompareI("bool") == 0) || (datatype1.CompareI("boolean") == 0) || (datatype1.CompareI("tinyint") == 0))
    {
        if((datatype2.CompareI("bool") == 0) || (datatype2.CompareI("boolean") == 0) || (datatype2.CompareI("tinyint") == 0))
            return wyTrue;
    }
    else if((datatype1.CompareI("enum") == 0) || (datatype1.CompareI("set") == 0))
    {
        if((datatype2.CompareI("enum") == 0) || (datatype2.CompareI("set") == 0))
            return wyTrue;
    }
    else if((datatype1.CompareI("int") == 0) || (datatype1.CompareI("integer") == 0))
    {
        if((datatype2.CompareI("int") == 0) || (datatype2.CompareI("integer") == 0))
            return wyTrue;
    }

    return wyFalse;
}

List*
TabForeignKeys::CompareWithSourceColumns(PKEYSINFO pkeyinfo)
{
    List                *listtgtcols = NULL, *listsrccols = NULL;
    FKSourceColumn      *srccols = NULL, *tmpsrccols = NULL;
    PKEYCOLSINFO        keycolsinfo = NULL;
    wyUInt32            row = CustomGrid_GetCurSelRow(m_hgridfk);
    wyBool              ispossiblekey = wyTrue;
    wyInt32             keyindex = -1;
    wyString            datatype;

    if(!pkeyinfo)
        return NULL;

    listsrccols = (List*)CustomGrid_GetItemLongValue(m_hgridfk, row, CHILDCOLUMNS);
    
    listtgtcols = new List();
    if(listsrccols)
    {
        srccols = (FKSourceColumn*)listsrccols->GetFirst();

        if(srccols)
        {
            for(int i=0; i<m_totkeys; i++)
            {
                ispossiblekey = wyTrue;
                keycolsinfo = pkeyinfo[i].m_colinfo;
                tmpsrccols = srccols;
            
                //..If source columns are there, then only we shall search for possible key columns
                while(tmpsrccols && keycolsinfo)
                {
                    datatype.SetAs(keycolsinfo->m_datatype);
                    if(datatype.CompareI(tmpsrccols->m_pcwrapobj->m_newval->m_datatype) != 0)
                    {
                        if(!AreInternalDatatypesSame(datatype, tmpsrccols->m_pcwrapobj->m_newval->m_datatype))
                        {
                            ispossiblekey = wyFalse;
                            break;
                        }
                    }

                    tmpsrccols = (FKSourceColumn *)tmpsrccols->m_next;
                    keycolsinfo = keycolsinfo->m_next;
                }

                if(tmpsrccols && !keycolsinfo)
                {
                    ispossiblekey = wyFalse;
                }

                AppendParentColumnsStruct(listsrccols, listtgtcols, pkeyinfo[i].m_colinfo,  (ispossiblekey) && (keyindex == -1) ? wyTrue : wyFalse);
            
                if(ispossiblekey)
                {
                    if(keyindex == -1)
                        keyindex = i;
                }
            }
        }
    }
    else
    {
        for(int i=0; i<m_totkeys; i++)
        {
            AppendParentColumnsStruct(listsrccols, listtgtcols, pkeyinfo[i].m_colinfo, wyFalse);
        }
    }
    return listtgtcols;
}

void
TabForeignKeys::AppendParentColumnsStruct(List* listsrccols, List* listkeycolumns, PKEYCOLSINFO keycolsinfo, wyBool select)
{
    FKSourceColumn  *tmpsrccol = NULL;
    FKTargetColumn  *tgtcols = NULL, *firsttgtcol = NULL, *tmptgtcol = NULL;
    List            tmplistkeycols;
    PKEYCOLSINFO    tempkeycolsinfo = NULL;
    wyBool          alreadyexists = wyFalse;

    tempkeycolsinfo = keycolsinfo;

    if(listsrccols)
        tmpsrccol = (FKSourceColumn*) listsrccols->GetFirst();

    while(tempkeycolsinfo)
    {
        alreadyexists = wyFalse;

        tgtcols = (FKTargetColumn*) listkeycolumns->GetFirst();
        
        if(!tgtcols)        //..No column in the list
        {
            tgtcols = new FKTargetColumn();
        }
        else
        {
            while(tgtcols)
            {
                if(tgtcols->m_name.Compare(tempkeycolsinfo->m_colname) == 0)
                {
                    alreadyexists = wyTrue;
                    break;
                }
                tgtcols = (FKTargetColumn*)tgtcols->m_next;
            }
            if(alreadyexists)
            {
                if(select)
                {
                    tgtcols->m_selected = wyTrue;
                    listkeycolumns->Remove(tgtcols);
                    tmplistkeycols.Insert(tgtcols);
                }
            }
            else
            {
                tgtcols = new FKTargetColumn();
            }
        }

        if(alreadyexists)
        {
            
        }
        else
        {
            tgtcols->m_name.SetAs(tempkeycolsinfo->m_colname);
            tgtcols->m_datatype.SetAs(tempkeycolsinfo->m_datatype);
            
            if(select)
            {
                if(tmpsrccol)
                {
                    tgtcols->m_selected = select;
                    tmplistkeycols.Insert(tgtcols);
                    tmpsrccol = (FKSourceColumn*)tmpsrccol->m_next;
                }
                else
                {
                    tgtcols->m_selected = wyFalse;
                    listkeycolumns->Insert(tgtcols);
                }
            }
            else
            {
                tgtcols->m_selected = wyFalse;
                listkeycolumns->Insert(tgtcols);
            }
        }
        
        tempkeycolsinfo = tempkeycolsinfo->m_next;
    }
    //..inserting key-columns in "listkeycolumns" list in a correct order (if they are possible key(select == wyTrue) and the number of key-columns are more than 1).
    if(select && tmplistkeycols.GetFirst())
    {
        tgtcols = (FKTargetColumn*)tmplistkeycols.GetLast();
        while(tgtcols)
        {
            tmptgtcol = tgtcols;
            tgtcols = (FKTargetColumn*)tgtcols->m_prev;
            tmplistkeycols.Remove(tmptgtcol);

            firsttgtcol = (FKTargetColumn*) listkeycolumns->GetFirst();
            if(firsttgtcol)
                listkeycolumns->InsertBefore(firsttgtcol, tmptgtcol);
            else
                listkeycolumns->Insert(tmptgtcol);
        }
    }
}

wyBool
TabForeignKeys::FetchInitValuesHelper(wyUInt32 row, wyString &fkey)
{
	wyBool			retbool;
	wyString		constraintname, dbname;
    wyString        keyval("");
	//RelFieldsParam	*relfldparam = NULL;
	FKEYINFOPARAM	*fkeyparam;	
	List			*fklist = NULL, *pklist = NULL;
    RelTableFldInfo	*srcfldinfo = NULL, *tempinfo = NULL, *tgtfldinfo = NULL;
    FKStructWrapper *cwrapobj = NULL;
    StructFK        *value = NULL;
	
	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    dbname.SetAs(m_ptabmgmt->m_tabinterfaceptr->m_dbname);

    VERIFY(fklist = new List());
	VERIFY(pklist = new List());

    fkeyparam   = new FKEYINFOPARAM;
	//relfldparam = new RelFieldsParam;

	if(!fkeyparam /*|| !relfldparam*/)
		return wyTrue;

    fkeyparam->m_fkeyfldlist = fklist;
	fkeyparam->m_pkeyfldlist = pklist;

    //Gets parsed F-key infos into a struct
    retbool = GetForeignKeyDetails(&fkey, fkeyparam, dbname.GetString());
	if(retbool == wyFalse)
		return wyTrue;

	//casevalue = GetmySQLCaseVariable(m_mdiwnd);
    
    //..Getting Source table info
    if(!fkeyparam->m_fkeyfldlist || !fkeyparam->m_fkeyfldlist->GetFirst())
        return wyTrue;
    
    value = new StructFK();
    InitStructFK(value);
    GetGridCellData(m_hgridfk, row, PARENTDATABASE, value->m_tgtdb);

    cwrapobj = new FKStructWrapper(value, wyFalse);
    m_listfkwrappers.Insert(cwrapobj);

    srcfldinfo = (RelTableFldInfo*)fkeyparam->m_fkeyfldlist->GetFirst();

    FKSourceColumn  *srccol = NULL;
    List            *listsrccols = new List();
    
    FieldStructWrapper *fieldwrap = NULL;

    while(srcfldinfo)
    {
        if(srcfldinfo->m_tablefld.GetLength())
        {
            fieldwrap = m_ptabmgmt->m_tabfields->GetWrapperObjectPointer(srcfldinfo->m_tablefld);
            if(!fieldwrap)
                return wyFalse;
            /*
            fieldwrap->m_listfkbackupcopy.Insert(new ReferencedBy(cwrapobj));
            fieldwrap->m_listfkworkingcopy.Insert(new ReferencedBy(cwrapobj));
            */
            srccol = new FKSourceColumn(fieldwrap);
            listsrccols->Insert(srccol);

            wyString tmpstr;
            tmpstr.SetAs(srcfldinfo->m_tablefld);
            tmpstr.FindAndReplace("`", "``");

            keyval.AddSprintf("%s%s%s, ", m_backtick, tmpstr.GetString(), m_backtick);
        }
        tempinfo = srcfldinfo;
        srcfldinfo = (RelTableFldInfo*)fkeyparam->m_fkeyfldlist->Remove(srcfldinfo);
        delete tempinfo;
        tempinfo = NULL;
    }
    cwrapobj->m_oldval->m_listsrccols = listsrccols;
    
    keyval.RTrim();
    while(keyval.GetCharAt(keyval.GetLength()-1) == ',')
        keyval.Strip(1);
    cwrapobj->m_oldval->m_srccols.SetAs(keyval);

    wyString ttemp;
    ttemp.SetAs(fkeyparam->m_constraint);
    ttemp.Strip(1);
    wyChar *somedata;
    somedata = ttemp.Substr(1,ttemp.GetLength()-1);
    
    cwrapobj->m_oldval->m_name.SetAs(somedata);

    //...Getting Target Table Fields and Filling in the grid.
    if(!fkeyparam->m_pkeyfldlist|| !fkeyparam->m_pkeyfldlist->GetFirst())
        return wyFalse;
    
    keyval.Clear();
    FKTargetColumn  *tgtcol = NULL, *tmptgtcol = NULL;
    List            *listtgtcols = new List();
    tgtfldinfo = (RelTableFldInfo*)fkeyparam->m_pkeyfldlist->GetFirst();
    
    List    *listparentcolumns = GetListOfFields(fkeyparam->m_parenttabledb, fkeyparam->m_parenttable);
    
    wyString    colnamestr;
    if(listparentcolumns)
    {
        wyString    datatype("");
        while(tgtfldinfo)
	    {
            if(tgtfldinfo->m_tablefld.GetLength())
		    {
                datatype.Clear();
                tgtcol = (FKTargetColumn*)listparentcolumns->GetFirst();
                while(tgtcol)
                {
                    if(tgtcol->m_name.Compare(tgtfldinfo->m_tablefld) == 0)
                    {
                        listparentcolumns->Remove(tgtcol);
                        datatype.SetAs(tgtcol->m_datatype);
                        delete tgtcol;
                        tgtcol = NULL;
                        break;
                    }
                    tgtcol = (FKTargetColumn*)tgtcol->m_next;
                }

                tgtcol = new FKTargetColumn();
                tgtcol->m_name.SetAs(tgtfldinfo->m_tablefld.GetString(), m_ismysql41);
                tgtcol->m_selected = wyTrue;
                tgtcol->m_datatype.SetAs(datatype);

                colnamestr.SetAs(tgtfldinfo->m_tablefld);
                colnamestr.FindAndReplace("`", "``");
                keyval.AddSprintf("%s%s%s, ", m_backtick, colnamestr.GetString(), m_backtick);

                listtgtcols->Insert(tgtcol);
            }
            tempinfo = tgtfldinfo;
            tgtfldinfo = (RelTableFldInfo*)fkeyparam->m_pkeyfldlist->Remove(tgtfldinfo);
            delete tempinfo;
            tempinfo = NULL;
        }

        //..Adding extra columns from the parent table.
        tgtcol = (FKTargetColumn*)listparentcolumns->GetFirst();
        while(tgtcol)
        {
            tmptgtcol = tgtcol;
            tgtcol = (FKTargetColumn*)listparentcolumns->Remove(tgtcol);
            listtgtcols->Insert(tmptgtcol);
        }
    }
    else
    {
        wyString tmpkeycols("");
        while(tgtfldinfo)
        {
            colnamestr.SetAs(tgtfldinfo->m_tablefld);
            colnamestr.FindAndReplace("`", "``");
            tmpkeycols.AddSprintf("%s%s%s, ", m_backtick, colnamestr.GetString(), m_backtick);
            tgtfldinfo = (RelTableFldInfo*)tgtfldinfo->m_next;
        }
        keyval.SetAs(tmpkeycols);
    }

    if(listparentcolumns)
        delete listparentcolumns;
    listparentcolumns = NULL;

    keyval.RTrim();
    while(keyval.GetCharAt(keyval.GetLength()-1) == ',')
        keyval.Strip(1);

    cwrapobj->m_oldval->m_tgtcols.SetAs(keyval);
    cwrapobj->m_oldval->m_tgttbl.SetAs(fkeyparam->m_parenttable);
    cwrapobj->m_oldval->m_tgtdb.SetAs(fkeyparam->m_parenttabledb);
    cwrapobj->m_oldval->m_listtgtcols = listtgtcols;

    //CustomGrid_SetItemLongValue(m_hgridfk, row, PARENTCOLUMNS, (LPARAM) cwrapobj->m_oldval->m_listtgtcols);             //  Setting ParentColumns Structure to Cell
    

    wyString onupdate, ondelete;
    //..Fetching and setting ForeignKey OnUpdate value
    switch(fkeyparam->m_onupdate)
    {
    case CASCADE:
        onupdate.SetAs("Cascade");
        break;
    case SETNULL:
        onupdate.SetAs("Set null");
        break;
    case NOACTION:
        onupdate.SetAs("No Action");
        break;
    case RESTRICT:
        onupdate.SetAs("Restrict");
        break;
    default:
        onupdate.SetAs("");
        break;
    }

    //..Fetching and setting ForeignKey OnDelete value
    switch(fkeyparam->m_ondelete)
    {
    case CASCADE:
        ondelete.SetAs("Cascade");
        break;

    case SETNULL:
        ondelete.SetAs("Set null");
        break;

    case NOACTION:
        ondelete.SetAs("No Action");
        break;

    case RESTRICT:
        ondelete.SetAs("Restrict");
        break;

    default:
        ondelete.SetAs("");
        break;
    }
    cwrapobj->m_oldval->m_onupdate.SetAs(onupdate);
    cwrapobj->m_oldval->m_ondelete.SetAs(ondelete);

    //..Releasing List* memory
    if(fkeyparam->m_fkeyfldlist)
        delete fkeyparam->m_fkeyfldlist;
    if(fkeyparam->m_pkeyfldlist)
        delete fkeyparam->m_pkeyfldlist;
    if(fkeyparam)
        delete fkeyparam;

    return wyTrue;
}

void
TabForeignKeys::ProcessFailedWrapper(wyUInt32 row)
{
    FKStructWrapper *cwrapobj = NULL;
    FKSourceColumn  *srccol = NULL;
    List*       listsrccols = new List();

    cwrapobj = m_failedwrap;

    if(m_failedwrapsrccols)
    {
        for(int j=0; j<m_nfailedwrapsrccols; j++)
        {
            FieldStructWrapper *fieldwrap = NULL;
            wyString fieldname;

            if(m_failedwrapsrccols[j])
                fieldname.SetAs((wyChar*)m_failedwrapsrccols[j], strlen((wyChar*)m_failedwrapsrccols[j]));

            fieldwrap = m_ptabmgmt->m_tabfields->GetWrapperObjectPointer(fieldname);
                    
            if(fieldwrap)
            {
                fieldwrap->m_listfkworkingcopy.Insert(new ReferencedBy(cwrapobj));
                fieldwrap->m_listfkbackupcopy.Insert(new ReferencedBy(cwrapobj));

                srccol = new FKSourceColumn(fieldwrap);
                listsrccols->Insert(srccol);
            }
        }
        cwrapobj->m_newval->m_listsrccols = listsrccols;
    }

    CustomGrid_SetText(m_hgridfk, row, CONSTRAINT, cwrapobj->m_newval->m_name.GetString());
    CustomGrid_SetText(m_hgridfk, row, CHILDCOLUMNS, cwrapobj->m_newval->m_srccols.GetString());
    CustomGrid_SetText(m_hgridfk, row, PARENTDATABASE, cwrapobj->m_newval->m_tgtdb.GetString());
    CustomGrid_SetText(m_hgridfk, row, PARENTTABLE, cwrapobj->m_newval->m_tgttbl.GetString());
    CustomGrid_SetText(m_hgridfk, row, PARENTCOLUMNS, cwrapobj->m_newval->m_tgtcols.GetString());
    CustomGrid_SetText(m_hgridfk, row, ONUPDATE, cwrapobj->m_newval->m_onupdate.GetString());
    CustomGrid_SetText(m_hgridfk, row, ONDELETE, cwrapobj->m_newval->m_ondelete.GetString());

    CustomGrid_SetItemLongValue(m_hgridfk, row, CHILDCOLUMNS, (LPARAM) cwrapobj->m_newval->m_listsrccols);
    CustomGrid_SetItemLongValue(m_hgridfk, row, PARENTCOLUMNS, (LPARAM) cwrapobj->m_newval->m_listtgtcols);

    CustomGrid_SetRowLongData(m_hgridfk, row, (LPARAM) cwrapobj);

    //..Reseting failed fk-wrapper records
    m_failedwrap = NULL;
    m_failedwraprow = -1;

    for(int i=0; i<m_nfailedwrapsrccols; i++)
    {
        free(m_failedwrapsrccols[i]);
        m_failedwrapsrccols[i] = NULL;
    }

    m_nfailedwrapsrccols = -1;
    if(m_failedwrapsrccols)
        free(m_failedwrapsrccols);

    m_failedwrapsrccols = NULL;
}

void
TabForeignKeys::FillInitValues(List* listunsavedwrappers)
{
    wyUInt32    rowind = -1;
    FKStructWrapper *cwrapobj = NULL, *tmpcwrapobj = NULL;
    FKStructWrapper *cwrapunsaved = NULL;
    wyString    keyval("");

    cwrapobj = (FKStructWrapper *) m_listfkwrappers.GetFirst();

    if(listunsavedwrappers)
        cwrapunsaved = (FKStructWrapper*)listunsavedwrappers->GetFirst();

    while(cwrapobj)
    {
        rowind = InsertRow();
        
        if(listunsavedwrappers)
        {
            //..Getting the first from the list of unsaved wrappers
            cwrapunsaved = (FKStructWrapper*)listunsavedwrappers->GetFirst();

            //..Going through all unsaved wrappers
            while(cwrapunsaved)
            {
                //..compare the names of re-fetched fks and unsaved fks.
                if(cwrapunsaved->m_oldval && cwrapunsaved->m_newval && cwrapobj->m_oldval->m_name.CompareI(cwrapunsaved->m_oldval->m_name) == 0)
                {
                    //..Removing wrapper from main list, deleting it and inserting unsaved wrapper into main-list, if any match found
                    tmpcwrapobj = cwrapobj;
                    cwrapobj = (FKStructWrapper*)m_listfkwrappers.Remove(cwrapobj);
                    delete tmpcwrapobj;

                    ///Removing unsaved wrapper from unsavedwrapper list
                    listunsavedwrappers->Remove(cwrapunsaved);

                    //..Inserting unsaved wrapper into main-list
                    if(cwrapobj)
                        m_listfkwrappers.InsertBefore(cwrapobj, cwrapunsaved);
                    else
                        m_listfkwrappers.Insert(cwrapunsaved);
                    cwrapobj = cwrapunsaved;

                    if(cwrapobj->m_newval)
                    {
                        if(cwrapobj->m_newval->m_listsrccols)
                        {
                            delete cwrapobj->m_newval->m_listsrccols;
                            cwrapobj->m_newval->m_listsrccols = NULL;
                        }
                        cwrapobj->m_newval->m_listsrccols = ParseAndGetSrcCols(cwrapobj->m_newval->m_srccols);
                    }

                    if(cwrapobj->m_oldval)
                    {
                        if(cwrapobj->m_newval->m_listsrccols)
                        {
                            delete cwrapobj->m_oldval->m_listsrccols;
                            cwrapobj->m_oldval->m_listsrccols = NULL;
                        }
                        cwrapobj->m_oldval->m_listsrccols = ParseAndGetSrcCols(cwrapobj->m_oldval->m_srccols);
                    }

                    //..parsing source-columns and fetching field-wrappers for each column.
                    /*
                    {
                        wyString 	columns("");
	                    wyString 	colname("");
	                    wyInt32		nticks = 0;

                        columns.SetAs(cwrapobj->m_newval->m_srccols);

                        for(int i=1; i<columns.GetLength(); i++)
	                    {
		                    if(columns.GetCharAt(i) == '`')
			                    nticks++;
			
		                    colname.AddSprintf("%c", columns.GetCharAt(i));
		
		                    if(nticks%2 == 1)
		                    {
			                    if(columns.GetCharAt(++i) == ',')
			                    {
				                    colname.Strip(1);

                                    FieldStructWrapper *fieldwrap = NULL;
                                    fieldwrap = (FieldStructWrapper *)m_ptabmgmt->m_tabfields->GetWrapperObjectPointer(colname);

                                    if(fieldwrap)
                                    {
                                        srccol = new FKSourceColumn(fieldwrap);
                                        if(!cwrapobj->m_newval->m_listsrccols)
                                            cwrapobj->m_newval->m_listsrccols = new List();
                                        cwrapobj->m_newval->m_listsrccols->Insert(srccol);
                                    }

                                    while(columns.GetCharAt(i) != '`')
                                        i++;
                                    i++;
                                    colname.Clear();
			                    }
		                    }
	                    }
                    }
                    */

                    break;
                }
                cwrapunsaved = (FKStructWrapper*)cwrapunsaved->m_next;
            }
        }
        
        if(cwrapobj->m_newval)
        {
            InsertWrapperIntoRow(rowind, cwrapobj);
            CustomGrid_SetRowLongData(m_hgridfk, rowind, (LPARAM) cwrapobj);
        }
        cwrapobj = (FKStructWrapper *)cwrapobj->m_next;
    }

    if(listunsavedwrappers)
    {
        cwrapobj = (FKStructWrapper*)listunsavedwrappers->GetFirst();

        while(cwrapobj)
        {
            tmpcwrapobj = cwrapobj;
            cwrapobj = (FKStructWrapper*)listunsavedwrappers->Remove(cwrapobj);

            if(tmpcwrapobj->m_newval)
                tmpcwrapobj->m_newval->m_listsrccols = ParseAndGetSrcCols(tmpcwrapobj->m_newval->m_srccols);

            rowind = InsertRow();
            InsertWrapperIntoRow(rowind, tmpcwrapobj);
            CustomGrid_SetRowLongData(m_hgridfk, rowind, (LPARAM) tmpcwrapobj);
            m_listfkwrappers.Insert(tmpcwrapobj);
        }
    }

    CustomGrid_SetCurSelection(m_hgridfk, 0, 0, wyTrue);
    return;
}

List*
TabForeignKeys::ParseAndGetSrcCols(wyString& columnsstr)
{
    List    *listsrccols = NULL;
    FKSourceColumn  *srccol = NULL;

    if(columnsstr.GetLength())
        listsrccols = new List();

    wyString 	columns;
	wyString 	colname("");
	wyInt32		nticks = 0;

    columns.SetAs(columnsstr);

    for(int i=1; i<columns.GetLength(); i++)
	{
		if(columns.GetCharAt(i) == '`')
			nticks++;
			
		colname.AddSprintf("%c", columns.GetCharAt(i));
		
        //..checking for the enclosing backtick
		if(nticks%2 == 1)
		{
            //..If only 1 column in the srccols string
            if(i == (columns.GetLength() - 1))
            {
                colname.Strip(1);
                FieldStructWrapper *fieldwrap = NULL;
                fieldwrap = (FieldStructWrapper *)m_ptabmgmt->m_tabfields->GetWrapperObjectPointer(colname);

                if(fieldwrap)
                {
                    srccol = new FKSourceColumn(fieldwrap);
                    listsrccols->Insert(srccol);
                }
            }
            //..Condition will be true for the multiple columns as srccols
			else if(columns.GetCharAt(++i) == ',')
			{
                //..Gets the Field wrapper from the column name and inserts into list
				colname.Strip(1);
                FieldStructWrapper *fieldwrap = NULL;
                fieldwrap = (FieldStructWrapper *)m_ptabmgmt->m_tabfields->GetWrapperObjectPointer(colname);

                if(fieldwrap)
                {
                    srccol = new FKSourceColumn(fieldwrap);
                    listsrccols->Insert(srccol);
                }

                //..Moves i to the next column name(after backtick)
                while(columns.GetCharAt(i) != '`')
                    i++;

                //..Reseting the variable
                colname.Clear();
                nticks = 0;
			}
		}
	}

    return listsrccols;
}

void
TabForeignKeys::InsertWrapperIntoRow(wyUInt32 row, FKStructWrapper *cwrapobj)
{
    FKSourceColumn  *srccol = NULL;

    if(row < 0)
        return;

    if(!cwrapobj)
        return;

    if(cwrapobj->m_newval->m_listsrccols)
    {
        srccol = (FKSourceColumn*) cwrapobj->m_newval->m_listsrccols->GetFirst();
        while(srccol)
        {
            srccol->m_pcwrapobj->m_listfkworkingcopy.Insert(new ReferencedBy(cwrapobj));
            srccol->m_pcwrapobj->m_listfkbackupcopy.Insert(new ReferencedBy(cwrapobj));
            srccol = (FKSourceColumn*) srccol->m_next;
        }
    }

    CustomGrid_SetText(m_hgridfk, row, CONSTRAINT, cwrapobj->m_newval->m_name.GetString());
    CustomGrid_SetText(m_hgridfk, row, CHILDCOLUMNS, cwrapobj->m_newval->m_srccols.GetString());
    CustomGrid_SetText(m_hgridfk, row, PARENTDATABASE, cwrapobj->m_newval->m_tgtdb.GetString());
    CustomGrid_SetText(m_hgridfk, row, PARENTTABLE, cwrapobj->m_newval->m_tgttbl.GetString());
    CustomGrid_SetText(m_hgridfk, row, PARENTCOLUMNS, cwrapobj->m_newval->m_tgtcols.GetString());
    CustomGrid_SetText(m_hgridfk, row, ONUPDATE, cwrapobj->m_newval->m_onupdate.GetString());
    CustomGrid_SetText(m_hgridfk, row, ONDELETE, cwrapobj->m_newval->m_ondelete.GetString());

    CustomGrid_SetItemLongValue(m_hgridfk, row, CHILDCOLUMNS, (LPARAM) cwrapobj->m_newval->m_listsrccols);
    CustomGrid_SetItemLongValue(m_hgridfk, row, PARENTCOLUMNS, (LPARAM) cwrapobj->m_newval->m_listtgtcols);
}

wyBool
TabForeignKeys::FetchInitValues()
{
    wyChar			delm;
	wyInt32         retrow = 0, len;
	wyBool			ret = wyTrue;
	wyString        key(""), showcreatetable(""), query("");
    wyString        tblname(""), dbname("");

    tblname.SetAs(m_ptabmgmt->m_tabinterfaceptr->m_origtblname);
    tblname.FindAndReplace("`", "``");

    dbname.SetAs(m_ptabmgmt->m_tabinterfaceptr->m_dbname);
    dbname.FindAndReplace("`", "``");

    ret = GetCreateTableString(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), dbname.GetString(), tblname.GetString(), showcreatetable, query);

    if(ret == wyFalse || !showcreatetable.GetLength())
		return wyFalse;
	
	GetForeignKeyInfo(&showcreatetable, key, retrow + 1);

    while(key.GetLength() > 0)
    {
		//If multiple F-key are present , it is seperated by ','. So remove this ',' before passing to grid
		len = key.GetLength();
		delm = key.GetCharAt(len - 1);
		if(delm == ',')
			key.Erase(len - 1, 1);		  

        if(!FetchInitValuesHelper(retrow, key))
            return wyFalse;

        retrow ++;
        GetForeignKeyInfo(&showcreatetable, key, retrow + 1);
    }
    return wyTrue;
}

wyBool
TabForeignKeys::OnWMNotify(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    LPNMHDR pnmh = (LPNMHDR)lparam;

    switch(pnmh->code)
    {
        case LVN_ITEMCHANGED:
            
            break;

        default:
            return wyFalse;
    }

    return wyTrue;
}

LRESULT CALLBACK
TabForeignKeys::DlgGridWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    TabForeignKeys*     ptabfk = (TabForeignKeys*)CustomGrid_GetLongData(hwnd);

    switch(message)
	{
    case GVN_ENDCHANGEROW:
        {
			return ptabfk->EnableDisableUpDownButtons(hwnd, wParam);
        }
        break;

    case GVN_BEGINLABELEDIT:
        return FALSE;

    case GVN_BEGINADDNEWROW:
        return 0;

    case GVN_CHECKBOXCLICK:
        {
            ptabfk->HandleCheckboxClick(hwnd, lParam, wParam, ptabfk->m_lastclickdlggrid);
        }
        break;

    case GVN_DRAWROWCHECK:
        {
            ((GVROWCHECKINFO*)lParam)->ischecked = CustomGrid_GetRowCheckState(hwnd, wParam) ? wyTrue : wyFalse;
        }
        break;

    case GVN_NEXTTABORDERITEM:
        SetFocus(GetNextDlgTabItem(GetParent(hwnd), hwnd, FALSE));
        break;

    case GVN_PREVTABORDERITEM:
        SetFocus(GetNextDlgTabItem(GetParent(hwnd), hwnd, TRUE));
        break;

    case GVN_DESTROY:
        ptabfk->m_lastclickdlggrid = -1;
        break;
    }
    return 1;
}

// Function to enable and disable Up/Down button depending upon where the selected row is 
// focused.
LRESULT
TabForeignKeys::EnableDisableUpDownButtons(HWND hgrid, wyInt32 row)
{
	wyInt32    totrow;
	HWND	    hwndup, hwnddown;

    VERIFY(hwndup	 = GetDlgItem(GetParent(hgrid), IDC_MOVEUP));
	VERIFY(hwnddown  = GetDlgItem(GetParent(hgrid), IDC_MOVEDOWN));

	totrow = CustomGrid_GetRowCount(m_hdlggrid);

	if(row == (totrow - 1))
	{
		EnableWindow(hwnddown, FALSE);

		if(totrow == 1)
			EnableWindow(hwndup, FALSE);
		else
			EnableWindow(hwndup, TRUE);
	}
	else if(row == 0)
	{
		EnableWindow(hwndup, FALSE);

		if(totrow == 1)
			EnableWindow(hwnddown, FALSE);
		else
			EnableWindow(hwnddown, TRUE);
	}
	else
	{
		EnableWindow(hwndup, TRUE);
		EnableWindow(hwnddown, TRUE);
	}

	return wyTrue;
}

/*
This function initializes the SourceColumnsDialogbox values
*/
wyBool
TabForeignKeys::OnWMInitSrcColsDlg(HWND hwnd)
{
	RECT rect;
	wyInt32 row = 0, col = 0;
    HICON   hicon;

    hicon = LoadIcon(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_COLUMN));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon);
    DestroyIcon(hicon);

	//..Create a Grid
    m_hdlggrid = CreateCustomGridEx(hwnd, 0, 0, 0, 0, (GVWNDPROC)DlgGridWndProc, GV_EX_ROWCHECKBOX, (LPARAM)this);

    if(!m_hdlggrid)
        return wyFalse;

    //..Disabling Buttons Up/Down
    EnableWindow(GetDlgItem(hwnd, IDC_BTNUP), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_BTNDOWN), FALSE);

    //..Creating Grid Columns
    InitDlgGrid();

    PostMessage(hwnd, WM_SIZE, 0, 0);
    FillSrcColsDlgGrid(hwnd);   //..Fill Source-Columns in the grid
	
    if(CustomGrid_GetRowCount(m_hdlggrid) != 0)
    PostMessage(hwnd, UM_SETINITFOCUS, (WPARAM)m_hdlggrid, NULL);
    
	GetWindowRect(hwnd, &m_wndrect);
	GetClientRect(hwnd, &m_dlgrect);
	
	//set the position and size of the static control that is used to draw the size gripper
	RECT temp = m_dlgrect;
	HWND hwndgripper = GetDlgItem(hwnd, IDC_INDEXGRIP);
    temp.left = temp.right - GetSystemMetrics(SM_CXHSCROLL);
	temp.top = temp.bottom - GetSystemMetrics(SM_CYVSCROLL);
    SetWindowPos(hwndgripper, NULL, temp.left, temp.top, temp.right - temp.left, temp.bottom - temp.top, SWP_NOZORDER);

    SetWindowText(hwndgripper, L"");

	row = CustomGrid_GetCurSelRow(m_hgridfk);
	col = CustomGrid_GetCurSelCol(m_hgridfk);
	CustomGrid_GetSubItemRect(m_hgridfk, row, col, &rect);
    MapWindowRect(m_hgridfk, NULL, &rect);

	GetCtrlRects(hwnd);

	PositionWindow(&rect, hwnd);
	
	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd);

    return wyTrue;
}

void
TabForeignKeys::FillSrcColsDlgGrid(HWND hwnd)
{
    FieldStructWrapper  *cfieldswrapobj = NULL;
    FKSourceColumn      *srccol = NULL, *tmpsrccol = NULL;
    List                *list = NULL;

    wyUInt32 row = CustomGrid_GetCurSelRow(m_hgridfk);
    wyUInt32 col = CustomGrid_GetCurSelCol(m_hgridfk);
    wyUInt32 newrow = NULL;

    // => BEGIN : FILLING ALREADY SELECTED/ATTACHED COLUMNS 
    // ..Getting List pointer attached with the grid-cell
    list = (List*)CustomGrid_GetItemLongValue(m_hgridfk, row, col);
    if(list)
    {
        srccol = (FKSourceColumn*) list->GetFirst();
        tmpsrccol = srccol;

        while(tmpsrccol)
        {
            if(tmpsrccol->m_pcwrapobj->m_newval)
            {
                newrow = CustomGrid_InsertRow(m_hdlggrid);
                CustomGrid_SetRowCheckState(m_hdlggrid, newrow, wyTrue);
                //CustomGrid_SetBoolValue(m_hdlggrid, newrow, 0, GV_TRUE);
                CustomGrid_SetText(m_hdlggrid, newrow, 0, (wyChar*)tmpsrccol->m_pcwrapobj->m_newval->m_name.GetString());
                CustomGrid_SetText(m_hdlggrid, newrow, 1, (wyChar*)tmpsrccol->m_pcwrapobj->m_newval->m_datatype.GetString());

                CustomGrid_SetRowLongData(m_hdlggrid, newrow, (LPARAM) tmpsrccol->m_pcwrapobj);
            }
            tmpsrccol = (FKSourceColumn*) tmpsrccol->m_next;
        }
    }
    // => END : FILLING ALREADY SELECTED/ATTACHED COLUMNS 

    // => BEGIN : FILLING NON-SELECTED COLUMNS
    cfieldswrapobj = (FieldStructWrapper*)m_ptabmgmt->m_tabfields->m_listwrapperstruct.GetFirst();
    tmpsrccol = srccol;

    while(cfieldswrapobj)
    {
        if(cfieldswrapobj->m_newval)
        {
            tmpsrccol = srccol;

            while(tmpsrccol)
            {
                //..Checking whether it's not already there in the list
                if(tmpsrccol->m_pcwrapobj == cfieldswrapobj)
                    break;
                tmpsrccol = (FKSourceColumn*)tmpsrccol->m_next;
            }

            if(!tmpsrccol)
            {
                //..Inserting into grid if it's not already inserted
				if(!(m_ismariadb52 && cfieldswrapobj->m_newval->m_virtuality.CompareI("VIRTUAL")==0) )
				{
                newrow = CustomGrid_InsertRow(m_hdlggrid);
                CustomGrid_SetText(m_hdlggrid, newrow, 0, (wyChar*)cfieldswrapobj->m_newval->m_name.GetString());
                CustomGrid_SetText(m_hdlggrid, newrow, 1, (wyChar*)cfieldswrapobj->m_newval->m_datatype.GetString());
                CustomGrid_SetRowLongData(m_hdlggrid, newrow, (LPARAM) cfieldswrapobj);
				}
            }
        }
        cfieldswrapobj = (FieldStructWrapper*)cfieldswrapobj->m_next;
    }

    if(newrow >= 0)
    {
        CustomGrid_SetCurSelection(m_hdlggrid, 0, 0, wyTrue);
        if(newrow > 0)
            EnableWindow(GetDlgItem(hwnd, IDC_MOVEDOWN), TRUE);
    }
}

void
TabForeignKeys::InitDlgGrid()
{
    wyInt32     i;
	GVCOLUMN	gvcol;

	wyChar      *heading[] = {_("Column"), _("Data Type")};
	wyInt32     elemsize[] = {0, 0};
	wyInt32     elemcount[] = {0, 0};
	wyInt32     mask[] = { GVIF_TEXT, GVIF_TEXT};  
	wyInt32     cx[] = { 220, 80};
	wyInt32     fmt[] = { GVIF_LEFT, GVIF_LEFT};

	wyInt32		width = 0;
	wyString	colname, dbname(RETAINWIDTH_DBNAME), tblname("__create_index");
	wyBool		isretaincolumnwidth = IsRetainColumnWidth();
	HFONT hfont;
    
    m_ptabmgmt->m_tabinterfaceptr->SetFont(m_hdlggrid);
    hfont = CustomGrid_GetRowFont(m_hdlggrid);

	for(i=0; i < sizeof(heading)/ sizeof(heading[0]); i++)
	{
		//if retain user modified width(prefernce option) is checked or not
		//if(isretaincolumnwidth == wyTrue)
		//{
		//	//for getting the resized column width
		//	colname.SetAs(heading[i]);
  //          width =  GetTextSize(colname.GetAsWideChar(), m_hdlggrid, hfont).right + 15;
		//	//width = GetColumnWidthFromFile(&dbname, &tblname, &colname);
		//}

		memset(&gvcol, 0, sizeof(gvcol));
		
		gvcol.mask = mask[i];
        gvcol.uIsReadOnly = wyTrue;
		gvcol.fmt  = fmt[i];
		gvcol.cx   = /*(width > 0)?width:*/cx[i];
		gvcol.text = heading[i];
		gvcol.nElemSize = elemsize[i];
		gvcol.nListCount = elemcount[i];
		gvcol.cchTextMax = strlen(heading[i]);

		CustomGrid_InsertColumn(m_hdlggrid, &gvcol);
	}
}

void
TabForeignKeys::OnWMInitTgtColsDlg(HWND hwnd)
{
	RECT rect;
	wyInt32 row =0, col = 0;
    HICON   hicon;

    hicon = LoadIcon(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_COLUMN));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon);
    DestroyIcon(hicon);

    m_hdlggrid = CreateCustomGridEx(hwnd, 0, 0, 0, 0, (GVWNDPROC)DlgGridWndProc, GV_EX_ROWCHECKBOX, (LPARAM)this);
    
    if(!m_hdlggrid)
        return;

    //..Disabling Up/Down buttons
    EnableWindow(GetDlgItem(hwnd, IDC_BTNUP), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_BTNDOWN), FALSE);

    InitDlgGrid();
    PostMessage(hwnd, WM_SIZE, 0, 0);


    FillTgtColsDlgGrid(hwnd);   //..Filling Target Columns into Dialog-Grid

    if(CustomGrid_GetRowCount(m_hdlggrid) != 0)
    PostMessage(hwnd, UM_SETINITFOCUS, (WPARAM)m_hdlggrid, NULL);

	GetWindowRect(hwnd, &m_wndrect);
	GetClientRect(hwnd, &m_dlgrect);
	
	//set the position and size of the static control that is used to draw the size gripper
	RECT temp = m_dlgrect;
	HWND hwndgripper = GetDlgItem(hwnd, IDC_INDEXGRIP);
    temp.left = temp.right - GetSystemMetrics(SM_CXHSCROLL);
	temp.top = temp.bottom - GetSystemMetrics(SM_CYVSCROLL);
    SetWindowPos(hwndgripper, NULL, temp.left, temp.top, temp.right - temp.left, temp.bottom - temp.top, SWP_NOZORDER);
    
    SetWindowText(hwndgripper, L"");

	row = CustomGrid_GetCurSelRow(m_hgridfk);
	col = CustomGrid_GetCurSelCol(m_hgridfk);
	CustomGrid_GetSubItemRect(m_hgridfk, row, col, &rect);
    MapWindowRect(m_hgridfk, NULL, &rect);

	GetCtrlRects(hwnd);
	PositionWindow(&rect, hwnd);
	
	//PositionCtrls(hwnd);
	
	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd);

}

void
TabForeignKeys::FillTgtColsDlgGrid(HWND hwnd)
{
    List                *list = NULL;
    FKTargetColumn      *tgtcol = NULL, *tmptgtcol = NULL;
    wyUInt32 	        row = CustomGrid_GetCurSelRow(m_hgridfk);
    wyUInt32            col = CustomGrid_GetCurSelCol(m_hgridfk);
    wyUInt32            newrow = NULL;
    HCURSOR		        hCursor;
    wyString            tblname;

    list = (List*)CustomGrid_GetItemLongValue(m_hgridfk, row, col);

    if(list)
    {
        tgtcol = (FKTargetColumn*) list->GetFirst();
        //..If the list is empty, We shall Fetch again the Parent_table-index_columns
        if(!tgtcol)
        {
            GetGridCellData(m_hgridfk, row, PARENTTABLE, tblname);

            //..If no parent table is selected, then destroy the dialog.
            if(!tblname.GetLength())
            {
                EndDialog(hwnd, 1);
                return;
            }

            ///Changing the mouese-icon
            hCursor = ::GetCursor();
            ::SetCursor(LoadCursor(NULL,IDC_WAIT));
            ::ShowCursor(1);
            
            ///Fetching the Target-table columns
            if(!FetchTgtTblCols(row, tblname))
            {
                ::SetCursor( hCursor );
                ::ShowCursor(1);

                PostMessage(hwnd, WM_CLOSE, 0, 0);
                //EndDialog(hwnd, 1);
                return;
            }

            ::SetCursor( hCursor );
            ::ShowCursor(1);
        }
        else
        {
            tmptgtcol = tgtcol;

            ///Looping through the Target-columns in the list
            while(tmptgtcol)
            {
				
                newrow = CustomGrid_InsertRow(m_hdlggrid);

                if(tmptgtcol->m_selected)
                    CustomGrid_SetRowCheckState(m_hdlggrid, newrow, wyTrue);

                CustomGrid_SetText(m_hdlggrid, newrow, 0, (wyChar*)tmptgtcol->m_name.GetString());
                CustomGrid_SetText(m_hdlggrid, newrow, 1, (wyChar*)tmptgtcol->m_datatype.GetString());
                CustomGrid_SetRowLongData(m_hdlggrid, newrow, (LPARAM) tmptgtcol);

                tmptgtcol = (FKTargetColumn*) tmptgtcol->m_next;
            }
        }
    }

    if(newrow >= 0)
    {
        CustomGrid_SetCurSelection(m_hdlggrid, 0, 0, wyTrue);
        if(newrow > 0)
            EnableWindow(GetDlgItem(hwnd, IDC_MOVEDOWN), TRUE);
    }
}

void
TabForeignKeys::ResizeColumnsDialog(HWND hwnd, LPARAM lParam)
{
	PositionCtrls(hwnd);
}

INT_PTR CALLBACK
TabForeignKeys::SrcColsDlgWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    TabForeignKeys  *ptabfk = (TabForeignKeys*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch(message)
    {
    case WM_INITDIALOG:
        {
            ptabfk = (TabForeignKeys*) lparam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ptabfk);
            LocalizeWindow(hwnd);
            return ptabfk->OnWMInitSrcColsDlg(hwnd);
        }
        break;

    case WM_COMMAND:
        {
            ptabfk->OnSrcColsDlgWMCommand(hwnd, wparam, lparam);
        }
        break;

    case UM_SETINITFOCUS:
        {
            SetFocus(HWND(wparam));
        }
        break;

    case WM_NOTIFY:
        {
            ptabfk->OnWMNotify(hwnd, wparam, lparam);
        }
        break;

    case WM_SIZE:
        {
            ptabfk->ResizeColumnsDialog(hwnd, lparam);
        }
        break;

	case WM_GETMINMAXINFO:
		{
			ptabfk->OnWMSizeInfo(lparam, hwnd);
        }
		break;


	case WM_PAINT:
		ptabfk->OnWMPaint(hwnd);
        return 1;
		break;

	case WM_ERASEBKGND:
		//blocked for double buffering
		return 1;

    case WM_HELP:
        {
            ShowHelp("http://sqlyogkb.webyog.com/article/90-fk-in-mysql-and-sqlyog");
        }
        return 1;

    case WM_DESTROY:
        {
			//delete the ctrl list
			DlgControl* pdlgctrl;
			while((pdlgctrl = (DlgControl*)ptabfk->m_controllist.GetFirst()))
			{
				ptabfk->m_controllist.Remove(pdlgctrl);
				delete pdlgctrl;
			}
            ptabfk->m_hdlggrid = NULL;
        }
        return 0;

    case WM_CLOSE:
            EndDialog(hwnd, 1);
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

INT_PTR CALLBACK
TabForeignKeys::TgtColsDlgWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    TabForeignKeys  *ptabfk = (TabForeignKeys*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch(message)
    {
    case WM_INITDIALOG:
        ptabfk = (TabForeignKeys*) lparam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ptabfk);
        LocalizeWindow(hwnd);
        ptabfk->OnWMInitTgtColsDlg(hwnd);
        break;

    case WM_COMMAND:
        {
            ptabfk->OnTargetColDlgWMCommand(hwnd, wparam, lparam);
        }
        break;

    case UM_SETINITFOCUS:
        {
            SetFocus(HWND(wparam));
        }
        break;

    case WM_NOTIFY:
        ptabfk->OnWMNotify(hwnd, wparam, lparam);
        break;
    case WM_SIZE:
        ptabfk->ResizeColumnsDialog(hwnd, lparam);
        break;

	case WM_GETMINMAXINFO:
		{
			ptabfk->OnWMSizeInfo(lparam, hwnd);
        }
		break;


	case WM_PAINT:
		ptabfk->OnWMPaint(hwnd);
        return 1;
		break;

	case WM_ERASEBKGND:
		//blocked for double buffering
		return 1;

    case WM_HELP:
        {
            ShowHelp("http://sqlyogkb.webyog.com/article/90-fk-in-mysql-and-sqlyog");
        }
        return 1;

    case WM_DESTROY:
        {
			//delete the ctrl list
			DlgControl* pdlgctrl;
			while((pdlgctrl = (DlgControl*)ptabfk->m_controllist.GetFirst()))
			{
				ptabfk->m_controllist.Remove(pdlgctrl);
				delete pdlgctrl;
			}
            ptabfk->m_hdlggrid = NULL;
        }
        return 0;

    case WM_CLOSE:
            EndDialog(hwnd, 1);
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

void
TabForeignKeys::OnTargetColDlgWMCommand(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    switch(LOWORD(wparam))
    {
    case IDOK:
        if(m_hdlggrid)
	        OnTgtColsDlgIDOK(hwnd);
        SendMessage(hwnd, WM_CLOSE, wparam, lparam);
        break;

    case IDCANCEL:
        SendMessage(hwnd, WM_CLOSE, wparam, lparam);
        break;

    case IDC_MOVEUP:
        if(m_hdlggrid)
			OnButtonUpDown(wyTrue);
        break;

    case IDC_MOVEDOWN:
        if(m_hdlggrid)
			OnButtonUpDown(wyFalse);
        break;
    }
}

void
TabForeignKeys::OnTgtColsDlgIDOK(HWND hwnd)
{
    wyUInt32    nrows;
    wyInt32     row, col;
    List        *list = NULL;
    FKTargetColumn  *tgtcol = NULL, *tmptgtcol = NULL;
    FKStructWrapper *cwrapobj = NULL;
    StructFK    *fk = NULL;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    //wyBool      markasdirty = wyFalse;
    nrows = CustomGrid_GetRowCount(m_hdlggrid);
    row = CustomGrid_GetCurSelRow(m_hgridfk);
    col = CustomGrid_GetCurSelCol(m_hgridfk);

    if(row >= 0)
        cwrapobj = (FKStructWrapper*) CustomGrid_GetRowLongData(m_hgridfk, row);

    if(cwrapobj)
    {
        if(cwrapobj->m_oldval)
        {
            if(cwrapobj->m_oldval == cwrapobj->m_newval)
            {
                cwrapobj->m_newval = GetDupStructFK(cwrapobj->m_oldval);
                cwrapobj->m_newval->m_listsrccols = GetDupListSrcCols(cwrapobj->m_oldval->m_listsrccols);
                cwrapobj->m_newval->m_listtgtcols = NULL;////GetDupListTgtCols(cwrapobj->m_oldval->m_listtgtcols);
            }
            //.. Create new FKTargetColumn for each row in TargetColumnsDialogGrid (for m_newval) and attach it with those grid rows..
            //List    *newlist = new List();
            for(int i=0; i< CustomGrid_GetRowCount(m_hdlggrid); i++)
            {
                tgtcol = (FKTargetColumn *) CustomGrid_GetRowLongData(m_hdlggrid, i);
                if(!tgtcol)
                    continue;

                FKTargetColumn *tgtcolnew = new FKTargetColumn();
                tgtcolnew->m_name.SetAs(tgtcol->m_name);
                tgtcolnew->m_datatype.SetAs(tgtcol->m_datatype);
                tgtcolnew->m_selected = tgtcol->m_selected;
                //newlist->Insert(tgtcolnew);
                CustomGrid_SetRowLongData(m_hdlggrid, i, (LPARAM) tgtcolnew);
            }
            //cwrapobj->m_newval->m_listtgtcols = newlist;

        }
        else if(cwrapobj->m_oldval && !cwrapobj->m_newval)
        {
            fk = new StructFK();
            InitStructFK(fk);
            GetGridCellData(m_hgridfk, row, PARENTDATABASE, fk->m_tgtdb);
            cwrapobj->m_newval = fk;
        }
    }
    else
    {
        fk = new StructFK();
        InitStructFK(fk);
        GetGridCellData(m_hgridfk, row, PARENTDATABASE, fk->m_tgtdb);

        cwrapobj = new FKStructWrapper(fk, wyTrue);
        m_listfkwrappers.Insert(cwrapobj);
    }

    list = cwrapobj->m_newval->m_listtgtcols;
    if(list)
    {
        tgtcol = (FKTargetColumn*) list->GetFirst();
        
        while(tgtcol)
        {
            tgtcol = (FKTargetColumn*)list->Remove(tgtcol); //..Only remove the tgtcols from list. Don't delete them, b'coz they are attached to grid rows.
        }
    }
    else
    {
        list = new List();
        cwrapobj->m_newval->m_listtgtcols = list;
    }

    List        checkedlist, uncheckedlist;
    wyString    keydef("");
    wyString    colnamestr;
    
    for(wyInt32 i=0; i<nrows; i++)
    {
        tgtcol = (FKTargetColumn*)CustomGrid_GetRowLongData(m_hdlggrid, i);

        //..Storing in different lists so that we can combine them such that selected comumns will lead the list
        if(CustomGrid_GetRowCheckState(m_hdlggrid, i))
        {
            checkedlist.Insert(tgtcol);
            colnamestr.SetAs(tgtcol->m_name);
            colnamestr.FindAndReplace("`", "``");
            keydef.AddSprintf("%s%s%s, ", m_backtick, colnamestr.GetString(), m_backtick);
        }
        else
        {
            uncheckedlist.Insert(tgtcol);
        }
    }
    
    keydef.RTrim();

    while(keydef.GetLength() && keydef.GetCharAt(keydef.GetLength() - 1) == ',')
        keydef.Strip(1);

    cwrapobj->m_newval->m_tgtcols.SetAs(keydef);

    //..combining checked and unchecked lists
    tgtcol = (FKTargetColumn*) checkedlist.GetFirst();
    while(tgtcol)
    {
        tmptgtcol = tgtcol;
        tgtcol = (FKTargetColumn*)checkedlist.Remove(tgtcol);
        tmptgtcol->m_selected = wyTrue;
        list->Insert(tmptgtcol);

    }
    tgtcol = (FKTargetColumn*) uncheckedlist.GetFirst();
    while(tgtcol)
    {
        tmptgtcol = tgtcol;
        tgtcol = (FKTargetColumn*)uncheckedlist.Remove(tgtcol);
        tmptgtcol->m_selected = wyFalse;
        list->Insert(tmptgtcol);
    }

    cwrapobj = ManageWrapperForNewAndOldVal(cwrapobj);

    list = (cwrapobj && cwrapobj->m_newval) ? cwrapobj->m_newval->m_listtgtcols : NULL;

    CustomGrid_SetItemLongValue(m_hgridfk, row, col, (LPARAM) list);
    CustomGrid_SetItemLongValue(m_hgridfk, row, CHILDCOLUMNS, (LPARAM) ((cwrapobj && cwrapobj->m_newval) ? cwrapobj->m_newval->m_listsrccols : NULL));
    CustomGrid_SetRowLongData(m_hgridfk, row, (LPARAM) cwrapobj);
    CustomGrid_SetText(m_hgridfk, row, col, (wyChar*) keydef.GetString());

    if(keydef.GetLength() && (row == CustomGrid_GetRowCount(m_hgridfk) - 1))
    {
        InsertRow();
    }
    if(!m_ptabmgmt->m_tabinterfaceptr->m_dirtytab)
        m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);
    ValidateFKs();
}

void
TabForeignKeys::HandleFKsOnFieldRename(FKStructWrapper* fkwrapobj, FieldStructWrapper *modifiedwrap)
{
    wyUInt32    nrows = 0, row = -1;
    wyString        srccolsstr("");

    if(!fkwrapobj)
        return;

    nrows = CustomGrid_GetRowCount(m_hgridfk);

    for(row=0; row<nrows; row++)
    {
        if(fkwrapobj == ((FKStructWrapper*)CustomGrid_GetRowLongData(m_hgridfk, row)))
            break;
    }

    if(row == nrows)
        return;

    FKSourceColumn  *srccol = NULL;

    if(fkwrapobj->m_newval->m_listsrccols)
        srccol = (FKSourceColumn *) fkwrapobj->m_newval->m_listsrccols->GetFirst();

    while(srccol)
    {
        wyString tmpstr;
        tmpstr.SetAs(srccol->m_pcwrapobj->m_newval->m_name);
        tmpstr.FindAndReplace("`", "``");
        srccolsstr.AddSprintf("`%s`, ", tmpstr.GetString());
        srccol = (FKSourceColumn *)srccol->m_next;
    }

    srccolsstr.RTrim();
    if(srccolsstr.GetLength())
    {
        while(srccolsstr.GetCharAt(srccolsstr.GetLength()-1) == ',')
            srccolsstr.Strip(1);
    }

    if(fkwrapobj->m_newval == fkwrapobj->m_oldval)
    {
        fkwrapobj->m_newval = GetDupStructFK(fkwrapobj->m_oldval);
        fkwrapobj->m_newval->m_listsrccols = GetDupListSrcCols(fkwrapobj->m_oldval->m_listsrccols);
        fkwrapobj->m_newval->m_listtgtcols = GetDupListTgtCols(fkwrapobj->m_oldval->m_listtgtcols);
    }

    fkwrapobj->m_newval->m_srccols.SetAs(srccolsstr);
    ScanEntireRow(row, CHILDCOLUMNS, srccolsstr);

    CustomGrid_SetText(m_hgridfk, row, CHILDCOLUMNS, (wyChar*) srccolsstr.GetString());
}

void
TabForeignKeys::HandleFKsOnFieldDelete(FKStructWrapper *fkwrapobj, FieldStructWrapper *fieldwrap)
{
    wyUInt32    nrows = 0, row = -1;

    if(!fkwrapobj)
        return;

    nrows = CustomGrid_GetRowCount(m_hgridfk);

    for(row=0; row<nrows; row++)
    {
        if(fkwrapobj == ((FKStructWrapper*)CustomGrid_GetRowLongData(m_hgridfk, row)))
            break;
    }

    if(row == nrows)
        return;

    if(fkwrapobj->m_oldval == fkwrapobj->m_newval)
    {
        fkwrapobj->m_newval = GetDupStructFK(fkwrapobj->m_oldval);
        fkwrapobj->m_newval->m_listsrccols = GetDupListSrcCols(fkwrapobj->m_oldval->m_listsrccols);
        fkwrapobj->m_newval->m_listtgtcols = GetDupListTgtCols(fkwrapobj->m_oldval->m_listtgtcols);
    }

    FKSourceColumn  *srccol = NULL, *tmpsrccol = NULL;
    FKTargetColumn  *tgtcol = NULL, *tmptgtcol = NULL;
    wyString        srccolsstr(""), tgtcolsstr("");
    
    if(fkwrapobj->m_newval->m_listsrccols)
        srccol = (FKSourceColumn *) fkwrapobj->m_newval->m_listsrccols->GetFirst();
    if(fkwrapobj->m_newval->m_listtgtcols)
        tgtcol = (FKTargetColumn *) fkwrapobj->m_newval->m_listtgtcols->GetFirst();

    while(srccol)
    {
        tmpsrccol = srccol;
        tmptgtcol = tgtcol;
        if(srccol->m_pcwrapobj == fieldwrap)
        {
            srccol = (FKSourceColumn *) fkwrapobj->m_newval->m_listsrccols->Remove(srccol);
            delete tmpsrccol;

            if(fkwrapobj->m_newval->m_listtgtcols)
            {
                tgtcol = (FKTargetColumn *) fkwrapobj->m_newval->m_listtgtcols->Remove(tgtcol);
                delete tmptgtcol;
            }
        }
        else
        {
            wyString tmpstr;

            tmpstr.SetAs(srccol->m_pcwrapobj->m_newval->m_name);
            tmpstr.FindAndReplace("`", "``");

            srccolsstr.AddSprintf("`%s`, ", tmpstr.GetString());
            srccol = (FKSourceColumn *)srccol->m_next;

            if(tgtcol)
            {
                tmpstr.SetAs(tgtcol->m_name);
                tmpstr.FindAndReplace("`", "``");

                tgtcolsstr.AddSprintf("`%s`, ", tmpstr.GetString());
                tgtcol = (FKTargetColumn *)tgtcol->m_next;
            }
        }
    }

    if(srccolsstr.GetLength())
    {
        srccolsstr.RTrim();
        while(srccolsstr.GetCharAt(srccolsstr.GetLength()-1) == ',')
            srccolsstr.Strip(1);

        tgtcolsstr.RTrim();
        while(tgtcolsstr.GetCharAt(tgtcolsstr.GetLength()-1) == ',')
            tgtcolsstr.Strip(1);

        fkwrapobj->m_newval->m_srccols.SetAs(srccolsstr);
        fkwrapobj->m_newval->m_tgtcols.SetAs(tgtcolsstr);

        CustomGrid_SetText(m_hgridfk, row, CHILDCOLUMNS, (wyChar*) srccolsstr.GetString());
        CustomGrid_SetText(m_hgridfk, row, PARENTCOLUMNS, (wyChar*) tgtcolsstr.GetString());
        
        ScanEntireRow(row, CHILDCOLUMNS, srccolsstr);
    }
    else
    {
        if(fkwrapobj->m_oldval) 
        {
            if(fkwrapobj->m_oldval != fkwrapobj->m_newval)
            {
                delete fkwrapobj->m_newval;
            }
            fkwrapobj->m_newval = NULL;
        }
        else
        {
            m_listfkwrappers.Remove(fkwrapobj);
            delete fkwrapobj;
        }
        CustomGrid_DeleteRow(m_hgridfk, row, wyTrue);

        if(CustomGrid_GetRowCount(m_hgridfk) == 0)
        {
            InsertRow();
        }
    }
}

StructFK*
TabForeignKeys::GetDupStructFK(StructFK *value)
{
    StructFK    *newval = NULL;

    if(value)
    {
        newval = new StructFK();
        newval->m_name.SetAs(value->m_name);
        newval->m_srccols.SetAs(value->m_srccols);
        newval->m_tgtdb.SetAs(value->m_tgtdb);
        newval->m_tgttbl.SetAs(value->m_tgttbl);
        newval->m_tgtcols.SetAs(value->m_tgtcols);
        newval->m_onupdate.SetAs(value->m_onupdate);
        newval->m_ondelete.SetAs(value->m_ondelete);
    }
    return newval;
}

List*
TabForeignKeys::GetDupListSrcCols(List *listsrccols)
{
    List                *list = NULL;
    FKSourceColumn      *srccol = NULL, *newsrccol = NULL;

    if(!listsrccols)
        return NULL;

    list = new List();

    srccol = (FKSourceColumn*) listsrccols->GetFirst();
        
    while(srccol)
    {
        newsrccol = new FKSourceColumn(srccol->m_pcwrapobj);
        list->Insert(newsrccol);

        srccol = (FKSourceColumn*)srccol->m_next;
    }
    return list;
}

List*
TabForeignKeys::GetDupListTgtCols(List *listtgtcols)
{
    List                *list = NULL;
    FKTargetColumn      *tgtcol = NULL, *newtgtcol = NULL;

    if(!listtgtcols)
        return NULL;

    list = new List();

    tgtcol = (FKTargetColumn*) listtgtcols->GetFirst();
        
    while(tgtcol)
    {
        newtgtcol = new FKTargetColumn();
        newtgtcol->m_name.SetAs(tgtcol->m_name);
        newtgtcol->m_datatype.SetAs(tgtcol->m_datatype);
        newtgtcol->m_selected = tgtcol->m_selected;

        list->Insert(newtgtcol);

        tgtcol = (FKTargetColumn*)tgtcol->m_next;
    }
    return list;
}

void
TabForeignKeys::OnTgtTblEndLabelEdit(wyInt32 row, wyString& tblname)
{
    wyString        prevtblname;
    HCURSOR		    hCursor;
    FKStructWrapper *cwrapobj = NULL;
    List            *listtmp = NULL;

    GetGridCellData(m_hgridfk, row, PARENTTABLE, prevtblname);

    if(m_celldataprevval.Compare(tblname) == 0)
        return;
    
    cwrapobj = (FKStructWrapper*) CustomGrid_GetRowLongData(m_hgridfk, row);

    //..If the table is changed, remove the targer-columns
    if(cwrapobj && cwrapobj->m_oldval != cwrapobj->m_newval)
    {
        listtmp = (List *)CustomGrid_GetItemLongValue(m_hgridfk, row, PARENTCOLUMNS);
        ClearListTgtCols(listtmp);
    }

    //..Clearing target-columns from the grid-row
    CustomGrid_SetText(m_hgridfk, row, PARENTCOLUMNS, "");
    CustomGrid_SetItemLongValue(m_hgridfk, row, PARENTCOLUMNS, (LPARAM) NULL);

    if(tblname.GetLength())
    {
        hCursor = ::GetCursor();
        ::SetCursor(LoadCursor(NULL,IDC_WAIT));
        ::ShowCursor(1);
        
        //..Fetching the target table columns
        FetchTgtTblCols(row, tblname);

        ::SetCursor( hCursor );
        ::ShowCursor(1);
    }
}

void
TabForeignKeys::ClearListTgtCols(List   *list)
{
    FKTargetColumn  *tgtcol = NULL, *tmptgtcol = NULL;

    if(!list)
        return;

    tgtcol = (FKTargetColumn *) list->GetFirst();

    while(tgtcol)
    {
        tmptgtcol = tgtcol;
        tgtcol = (FKTargetColumn *) list->Remove(tgtcol);
        delete tmptgtcol;
    }
}

void
TabForeignKeys::ClearListSrcCols(FKStructWrapper *cwrapobj, List *list)
{
    FKSourceColumn          *srccol = NULL, *prevsrccol = NULL;
    ReferencedBy            *refby = NULL;

    if(!list)
        return;

    srccol = (FKSourceColumn *) list->GetFirst();

    while(srccol)
    {
        prevsrccol = srccol;
        srccol = (FKSourceColumn *)list->Remove(srccol);
        
        //..Removing FK-Entries from FieldsWrapper
        refby = (ReferencedBy*)prevsrccol->m_pcwrapobj->m_listfkworkingcopy.GetFirst();
        while(refby)
        {
            if(refby->m_pfkwrap == cwrapobj)
            {
                prevsrccol->m_pcwrapobj->m_listfkworkingcopy.Remove(refby);
                delete refby;
                break;
            }
            refby = (ReferencedBy*)refby->m_next;
        }
        delete prevsrccol;
    }
}

wyBool
TabForeignKeys::GetAllTables()
{
    wyString    query, myrowstr, prevselection, dbname;
    MYSQL_RES	*myres;
	MYSQL_ROW	myrow;
    HCURSOR		hCursor;
    wyUInt32    row = CustomGrid_GetCurSelRow(m_hgridfk);
    wyUInt32    col = CustomGrid_GetCurSelCol(m_hgridfk);

    CustomGrid_DeleteListContent(m_hgridfk, col);

    GetGridCellData(m_hgridfk, row, PARENTDATABASE, dbname);

    if(!dbname.GetLength())
        return wyFalse;

    /// Replacing ` by ``
    dbname.FindAndReplace("`", "``");

    PrepareShowTable(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), dbname, query);

    /// Changing the cursor to Wait cursor
    hCursor = ::GetCursor();
    ::SetCursor(LoadCursor(NULL,IDC_WAIT));
    ::ShowCursor(1);

    VERIFY(myres = ExecuteAndGetResult(GetActiveWin(), m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query));

	if(!myres)
	{
		ShowMySQLError(m_hwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query.GetString());
		return wyFalse;
	}

	while(myrow = m_mdiwnd->m_tunnel->mysql_fetch_row(myres))
	{
		myrowstr.SetAs(myrow[0], m_ismysql41);
        CustomGrid_InsertTextInList(m_hgridfk, PARENTTABLE, myrowstr.GetAsWideChar());
	}
    mysql_free_result(myres);

    ::SetCursor( hCursor );
    ::ShowCursor(1);

    return wyTrue;
}

wyBool
TabForeignKeys::OnGVNBeginLabelEdit(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    wyUInt32	row = wParam;
	wyUInt32	col = lParam;
    wyUInt32    count = -1;
    wyString    celldata;
    
    GetGridCellData(m_hgridfk, row, col, m_celldataprevval);

    //..Return if the Target database is not selected
    if(col == PARENTTABLE)
    {
        GetGridCellData(m_hgridfk, row, PARENTDATABASE, celldata);

        if(!celldata.GetLength())
            return wyFalse;
        GetAllTables();
    }

    if(col == PARENTDATABASE)
        m_prevtgtdb.SetAs(m_celldataprevval);

    count = CustomGrid_GetRowCount(m_hgridfk);

    if(row == (count - 1))
    {
        InsertRow();
    }
    //m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);

    return wyTrue;
}

LRESULT CALLBACK
TabForeignKeys::GridWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    
    TabForeignKeys*     ptabfk = (TabForeignKeys*)CustomGrid_GetLongData(hwnd);
    wyString			tblname("");
    
    switch(message)
	{
    case GVN_CHECKBOXCLICK:
        {
            ptabfk->HandleCheckboxClick(hwnd, lParam, wParam, ptabfk->m_lastclickfkgrid);
        }
        break;

	case GVN_BEGINLABELEDIT:
        {
            if(!ptabfk->OnGVNBeginLabelEdit(hwnd, wParam, lParam))
                return 0;
        }
        break;

    case GVN_BUTTONCLICK:
        ptabfk->OnGVNButtonClick();
        break;

	case GVN_ENDLABELEDIT:
        {
            ptabfk->OnGVNEndLabelEdit(wParam,lParam);
        }
        break;

    case GVN_BEGINADDNEWROW:
        {
            return 0;
        }
        break;

    case GVN_DRAWROWCHECK:
        {
            ((GVROWCHECKINFO*)lParam)->ischecked = CustomGrid_GetRowCheckState(hwnd, wParam) ? wyTrue : wyFalse;
        }
        break;

    case GVN_DESTROY:
        {
            ptabfk->ClearAllMemory();
        }
        break;

    case GVN_SYSKEYDOWN:
        return ptabfk->OnGVNSysKeyDown(hwnd, wParam, lParam);

    case GVN_NEXTTABORDERITEM:
        SetFocus(GetNextDlgTabItem(ptabfk->m_ptabmgmt->m_tabinterfaceptr->m_hwnd, hwnd, FALSE));
        break;

    case GVN_PREVTABORDERITEM:
        SetFocus(GetNextDlgTabItem(ptabfk->m_ptabmgmt->m_tabinterfaceptr->m_hwnd, hwnd, TRUE));
        break;
	}
    
	return 1;
}

LRESULT
TabForeignKeys::OnGVNSysKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if(m_ptabmgmt->OnSysKeyDown(hwnd, wParam, lParam) == wyTrue)
    {
        return 1;
    }

    if(!m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog)
        return 1;

    return m_ptabmgmt->m_tabinterfaceptr->OnWMSysKeyDown(2, wParam, lParam);
}

wyBool
TabForeignKeys::ScanEntireRow(wyInt32  currentrow, wyInt32 currentcol, wyString& currentdata)
{
    FKStructWrapper         *cwrapobj = NULL;
    wyUInt32                row, col, ncols;
    wyString                newtext(""), origtext("");
    wyBool                  changed = wyFalse;
    List                    *list1 = NULL, *list2 = NULL;
    FKSourceColumn          *srccol1 = NULL, *srccol2 = NULL;

    row = currentrow;
    col = currentcol;

    cwrapobj = (FKStructWrapper*) CustomGrid_GetRowLongData(m_hgridfk, row);

    if(!cwrapobj)
        return wyFalse;

    if(!cwrapobj->m_oldval)
        return wyFalse;

    if(cwrapobj->m_oldval == cwrapobj->m_newval)
        return wyFalse;

    ncols = CustomGrid_GetColumnCount(m_hgridfk);

    for(int i=0; i< ncols; i++)
    {
        if(i == col)
            newtext.SetAs(currentdata);
        else
            GetGridCellData(m_hgridfk, row, i, newtext);

        switch(i)
        {
        case CONSTRAINT:
            origtext.SetAs(cwrapobj->m_oldval->m_name);
            break;

        case CHILDCOLUMNS:
            {
                list1 = cwrapobj->m_oldval->m_listsrccols;
                list2 = cwrapobj->m_newval->m_listsrccols;

                if(list1)
                    srccol1 = (FKSourceColumn *) list1->GetFirst();

                if(list2)
                    srccol2 = (FKSourceColumn *) list2->GetFirst();

                while(srccol1 && srccol2)
                {
                    if(srccol1->m_pcwrapobj != srccol2->m_pcwrapobj)
                        break;

                    srccol1 = (FKSourceColumn *) srccol1->m_next;
                    srccol2 = (FKSourceColumn *) srccol2->m_next;
                }

                if(srccol1 || srccol2)
                {
                    changed = wyTrue;
                    break;
                }
                else
                    continue;
            }
            break;

        case PARENTDATABASE:
            origtext.SetAs(cwrapobj->m_oldval->m_tgtdb);
            break;

        case PARENTTABLE:
            origtext.SetAs(cwrapobj->m_oldval->m_tgttbl);
            break;

        case PARENTCOLUMNS:
            origtext.SetAs(cwrapobj->m_oldval->m_tgtcols);
            break;

        case ONUPDATE:
            origtext.SetAs(cwrapobj->m_oldval->m_onupdate);
            break;

        case ONDELETE:
            origtext.SetAs(cwrapobj->m_oldval->m_ondelete);
            break;

        default:
            origtext.SetAs("");
        }

        if(changed || origtext.Compare(newtext) != 0)
        {
            changed = wyTrue;
            break;
        }
    }

    if(!changed)
    {
        delete cwrapobj->m_newval;
        cwrapobj->m_newval = cwrapobj->m_oldval;
    }
    return wyTrue;
}

void
TabForeignKeys::HandleCheckboxClick(HWND hwnd, LPARAM lparam, WPARAM wparam, wyInt32 &lastclickindex)
{
    wyInt32     ret, startpos, endpos, flag, currow = -1;

    if(CustomGrid_GetRowCount(hwnd) == 0)
        return;

    ret = GetKeyState(VK_SHIFT);// shift key is pressed or not

    if(wparam == VK_SPACE)// && GetKeyState(VK_CONTROL)
    {
        if(lastclickindex == -1 || !(ret & 0x8000))
        {
            lastclickindex = CustomGrid_GetCurSelRow(hwnd);
            if(lastclickindex == -1)
                return;

            flag = CustomGrid_GetRowCheckState(hwnd, lastclickindex);
            CustomGrid_SetRowCheckState(hwnd, lastclickindex, flag == GV_CHEKCED ? wyFalse : wyTrue);
            return;
        }
        currow = CustomGrid_GetCurSelRow(hwnd);
    }
    else
    {
        if(lastclickindex == -1 || !(ret & 0x8000))   //..If first time, any checkbox is clicked or shift key is not pressed, then no multiple selection required..
        {
            lastclickindex = wparam;
            return;
        }

        if(lastclickindex == wparam)
            return;

        currow = wparam;
    }

    if(currow < lastclickindex)
    {
        startpos = currow;
        endpos = lastclickindex;
    }
    else
    {
        startpos = lastclickindex;
		endpos = currow;
    }

    flag = CustomGrid_GetRowCheckState(hwnd, currow);

    while(startpos <= endpos)
    {
        if(wparam == VK_SPACE)
            CustomGrid_SetRowCheckState(hwnd, startpos, flag == GV_CHEKCED ? wyFalse : wyTrue);
        else
            CustomGrid_SetRowCheckState(hwnd, startpos, flag == GV_CHEKCED ? wyTrue : wyFalse);
        startpos++;
    }
    lastclickindex = currow;
}

wyBool
TabForeignKeys::ProcessInsert()
{
    wyUInt32        newrowindex;
    
    CustomGrid_ApplyChanges(m_hgridfk, wyTrue);

    newrowindex = InsertRow();
    CustomGrid_SetCurSelection(m_hgridfk, newrowindex, CHILDCOLUMNS);

    return wyTrue;
}

wyBool	
TabForeignKeys::ProcessDelete()
{
    // we delete the selected row but if there are no rows selected we just return.
	wyInt32		selrow, newrow;

	// First get the current selection
    selrow = CustomGrid_GetCurSelRow(m_hgridfk); //LOWORD(CustomGrid_GetCurSelection(m_hgridfk));
    
    if(selrow == -1)
        return wyTrue;

	CustomGrid_ApplyChanges(m_hgridfk);

    /// True, If any row is checked.
    if(!DropSelectedForeignKeys())
    {
        /// Drop the currently selected foreign-key on the grid
        DropForeignKey(selrow);
    }

    /// Adding extra row if there is no row on the grid
    if(CustomGrid_GetRowCount(m_hgridfk) == 0)
    {
        newrow = InsertRow();
        CustomGrid_SetCurSelection(m_hgridfk, newrow, CHILDCOLUMNS);
    }

    return wyTrue;
}

wyBool
TabForeignKeys::DropSelectedForeignKeys()
{
    wyInt32         count, row, ret = IDNO;
    FKStructWrapper *cwrapobj = NULL;
    FKSourceColumn  *srccol = NULL;
    ReferencedBy    *refby  = NULL;
    wyBool          checkrowfound = wyFalse;

    count = CustomGrid_GetRowCount(m_hgridfk);

    for(row=0; row<count; row++)
    {
        if(CustomGrid_GetRowCheckState(m_hgridfk, row))
        {
            cwrapobj = (FKStructWrapper*)CustomGrid_GetRowLongData(m_hgridfk, row);
            checkrowfound = wyTrue;

            /// Delete the row withput prompting, if the user has already deleted all fk values manually for the row or if the row is empty.
            if(!cwrapobj || !cwrapobj->m_newval)
            {
                CustomGrid_DeleteRow(m_hgridfk, row, wyTrue);
                row--;
                count--;
                continue;
            }
            else if(ret == IDNO)
            {
                /// Prompt the user for the first time only
                ret = yog_message(m_hgridfk, _(L"Do you want to drop the selected Foreign Key(s)?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
                if(ret != IDYES)
                    return wyTrue;
            }
            if(ret == IDYES)
            {
                if(cwrapobj->m_newval && cwrapobj->m_newval->m_listsrccols)
                {
                    /// Don't delete srccols here. (So don't use ClearListSrcCols() API)
                    srccol = (FKSourceColumn*) cwrapobj->m_newval->m_listsrccols->GetFirst();

                    while(srccol)
                    {
                        refby = (ReferencedBy*) srccol->m_pcwrapobj->m_listfkworkingcopy.GetFirst();
                        while(refby)
                        {
                            if(refby->m_pfkwrap == cwrapobj)
                            {
                                srccol->m_pcwrapobj->m_listfkworkingcopy.Remove(refby);
                                delete refby;
                                break;
                            }
                            refby = (ReferencedBy*) refby->m_next;
                        }
                        srccol = (FKSourceColumn*) srccol->m_next;
                    }
                }

                /// Change the list according to whether the fk is new or existing one..
                ChangeListOnDelete(cwrapobj);
                
                CustomGrid_DeleteRow(m_hgridfk, row, wyTrue);
                row--;
                count--;
            }
        }
    }
    if(ret == IDYES)
    {
        if(m_ptabmgmt->m_tabinterfaceptr->m_dirtytab != wyTrue)
        {
            m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);
        }
    }
    if(checkrowfound && (ret == IDYES) && m_lastclickfkgrid != -1)
        m_lastclickfkgrid = -1;

    return checkrowfound;
}

void
TabForeignKeys::ChangeListOnDelete(FKStructWrapper* cwrapobj)
{
    if(!cwrapobj)
        return;

    if(!cwrapobj->m_newval)
        return;

    /// For existing fk
    if(cwrapobj->m_oldval)
    {
        /// Deleting the m_newval if user has modified wrapper(fk) on the grid before dropping
        if(cwrapobj->m_oldval != cwrapobj->m_newval)
        {
            /// Removing all source-columns from the list
            ClearListSrcCols(NULL, cwrapobj->m_newval->m_listsrccols);
            delete cwrapobj->m_newval->m_listsrccols;
            cwrapobj->m_newval->m_listsrccols = NULL;

            /// Removing all tagert-columns from the list
            ClearListTgtCols(cwrapobj->m_newval->m_listtgtcols);
            delete cwrapobj->m_newval->m_listtgtcols;
            cwrapobj->m_newval->m_listtgtcols = NULL;

            delete cwrapobj->m_newval;
        }
        cwrapobj->m_newval = NULL;
    }
    /// For new fk
    else
    {
        /// Simply removing from the wrappers-list and deleting it..
        m_listfkwrappers.Remove(cwrapobj);
        delete cwrapobj;
    }
}

wyBool
TabForeignKeys::DropForeignKey(wyUInt32 row)
{
    wyInt32                 ret = IDNO;
    FKStructWrapper         *cwrapobj = NULL;
    FKSourceColumn          *srccol = NULL;
    ReferencedBy            *refby  = NULL;

    cwrapobj = (FKStructWrapper*) CustomGrid_GetRowLongData(m_hgridfk, row);

    if(cwrapobj && cwrapobj->m_newval)
    {
        /// Show confirmation dialog
        ret = MessageBox(m_hgridfk, _(L"Do you want to drop this foreign key?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
        
        if(ret != IDYES)
            return wyTrue;

        /// Remove ReferencedBy instances from fieldwrappers (Don't delete srccol)
        if(cwrapobj->m_newval->m_listsrccols)
        {
            srccol = (FKSourceColumn*) cwrapobj->m_newval->m_listsrccols->GetFirst();
            while(srccol)
            {
                refby = (ReferencedBy*) srccol->m_pcwrapobj->m_listfkworkingcopy.GetFirst();
                while(refby)
                {
                    if(refby->m_pfkwrap == cwrapobj)
                    {
                        srccol->m_pcwrapobj->m_listfkworkingcopy.Remove(refby);
                        delete refby;
                        break;
                    }
                    refby = (ReferencedBy*) refby->m_next;
                }
                srccol = (FKSourceColumn*) srccol->m_next;
            }
        }

        ChangeListOnDelete(cwrapobj);
    }
    CustomGrid_DeleteRow(m_hgridfk, row, wyTrue);

    /// Resetting variable
    m_lastclickfkgrid = -1;

    if(CustomGrid_GetRowCount(m_hgridfk))
    {
        if(row == 0)    // if the selected row was first then we just select the first row and column
        {
	        CustomGrid_SetCurSelection(m_hgridfk, 0, CHILDCOLUMNS);
        }
        else
        {
	        CustomGrid_SetCurSelection(m_hgridfk, --row, CHILDCOLUMNS);
        }
    }

    if(ret == IDYES && m_ptabmgmt->m_tabinterfaceptr->m_dirtytab != wyTrue)
    {
        m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);
    }

    return wyTrue;
}

void 
TabForeignKeys::Resize()
{
    RECT			rcmain, rctoolbar;
	wyInt32			hpos, vpos, height;

    VERIFY(GetWindowRect(m_hwnd, &rcmain));
    VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcmain, 2));
    
    VERIFY(GetWindowRect(m_ptabmgmt->m_hwndtool, &rctoolbar));
    VERIFY(MapWindowPoints(NULL, m_ptabmgmt->m_hwndtool, (LPPOINT)&rctoolbar, 2));

    //..Moving Grid
    hpos    = m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog? 2 : 1;
    vpos    = rctoolbar.bottom; // 25;
    height  = rcmain.bottom - rcmain.top - rctoolbar.bottom - rctoolbar.top - 2;    //..25 for "Hide Language Options" checkbox So that grid in each tab is consistent with TabField's grid
    if(m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog)
        MoveWindow(m_hgridfk, hpos, vpos, rcmain.right - 3, height, TRUE);
    else
        MoveWindow(m_hgridfk, hpos, vpos, rcmain.right - 2, height, TRUE);

	CustomGrid_EnsureVisible(m_hgridfk, 0, 0, wyTrue);

}

wyBool
TabForeignKeys::Create()
{
    m_db.SetAs(GetActiveWin()->m_pcqueryobject->m_seldatabase);
    m_table.SetAs(((TableTabInterface*)m_ptabmgmt->m_tabinterfaceptr)->m_origtblname);
    
    /// Creating grid
    if(!CreateGrid())
        return wyFalse;
    
    /// Initializing grid
    if(!InitGrid())
        return wyFalse;

    if(m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
    {
        /// Fetching FK values
        if(!FetchInitValues())
            return wyFalse;

        /// Filling FK values into grid
        FillInitValues();
    }

    /// Inserting extra row
    InsertRow();

    return wyTrue;
}

wyBool
TabForeignKeys::ValidateFKs(wyBool showmsg)
{
    wyUInt32            nrows = 0;
    wyString            srccols, tgtdb, tgttbl, tgtcols;
    FKStructWrapper     *cwrapobj = NULL;
    wyBool              flag = wyTrue;

    nrows = CustomGrid_GetRowCount(m_hgridfk);

    for(int row=0; row<nrows; row++)
    {
        cwrapobj = (FKStructWrapper *) CustomGrid_GetRowLongData(m_hgridfk, row);

        if(!cwrapobj)
            continue;

        if(!cwrapobj->m_newval)
            continue;

        if(cwrapobj->m_oldval == cwrapobj->m_newval)
            continue;

        if(cwrapobj->m_errmsg)
            delete cwrapobj->m_errmsg;
        cwrapobj->m_errmsg = NULL;

        GetGridCellData(m_hgridfk, row, CHILDCOLUMNS, srccols);
        GetGridCellData(m_hgridfk, row, PARENTDATABASE, tgtdb);
        GetGridCellData(m_hgridfk, row, PARENTTABLE, tgttbl);
        GetGridCellData(m_hgridfk, row, PARENTCOLUMNS, tgtcols);

        //..Validating Source Columns
        if(!srccols.GetLength())
        {
            if(!cwrapobj->m_errmsg)
                cwrapobj->m_errmsg = new wyString();

            cwrapobj->m_errmsg->SetAs(NO_SRCCOLS);

            if(showmsg)
            {
                if(m_ptabmgmt->GetActiveTabImage() != IDI_TABIMG_FOREIGNKEYS)
                    CustomTab_SetCurSel(m_ptabmgmt->m_hwnd, 2);

                CustomGrid_SetCurSelection(m_hgridfk, row, CHILDCOLUMNS, wyTrue);
                CustomGrid_EnsureVisible(m_hgridfk, row, CHILDCOLUMNS, wyTrue);

                MessageBox(m_hwnd, NO_SRCCOLS, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
                return wyFalse;
            }
        }

        //..Validating Target database
        if(!tgtdb.GetLength())
        {
            if(!cwrapobj->m_errmsg)
            {
                cwrapobj->m_errmsg = new wyString();
                cwrapobj->m_errmsg->SetAs(NO_TGTDB);
            }

            if(showmsg)
            {
                if(m_ptabmgmt->GetActiveTabImage() != IDI_TABIMG_FOREIGNKEYS)
                    CustomTab_SetCurSel(m_ptabmgmt->m_hwnd, 2);

                CustomGrid_SetCurSelection(m_hgridfk, row, PARENTDATABASE, wyTrue);
                CustomGrid_EnsureVisible(m_hgridfk, row, PARENTDATABASE, wyTrue);

                MessageBox(m_hwnd, NO_TGTDB, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
                return wyFalse;
            }
        }

        //..Validating Target Table
        if(!tgttbl.GetLength())
        {
            if(!cwrapobj->m_errmsg)
            {
                cwrapobj->m_errmsg = new wyString();
                cwrapobj->m_errmsg->SetAs(NO_TGTTBL);
            }
            
            if(showmsg)
            {
                if(m_ptabmgmt->GetActiveTabImage() != IDI_TABIMG_FOREIGNKEYS)
                    CustomTab_SetCurSel(m_ptabmgmt->m_hwnd, 2);

                CustomGrid_SetCurSelection(m_hgridfk, row, PARENTTABLE, wyTrue);
                CustomGrid_EnsureVisible(m_hgridfk, row, PARENTTABLE, wyTrue);

                MessageBox(m_hwnd, NO_TGTTBL, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
                return wyFalse;
            }
        }

        //..Validating Target Columns
        if(!tgtcols.GetLength())
        {
            if(!cwrapobj->m_errmsg)
            {
                cwrapobj->m_errmsg = new wyString();
                cwrapobj->m_errmsg->SetAs(NO_TGTCOLS);
            }

            if(showmsg)
            {
                if(m_ptabmgmt->GetActiveTabImage() != IDI_TABIMG_FOREIGNKEYS)
                    CustomTab_SetCurSel(m_ptabmgmt->m_hwnd, 2);

                CustomGrid_SetCurSelection(m_hgridfk, row, PARENTCOLUMNS, wyTrue);
                CustomGrid_EnsureVisible(m_hgridfk, row, PARENTCOLUMNS, wyTrue);

                MessageBox(m_hwnd, NO_TGTCOLS, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
                return wyFalse;
            }
        }

        //..Validating No. of source and target columns
        {
            List            *listsrccol = NULL, *listtgtcol = NULL;
            FKSourceColumn  *srccol = NULL;
            FKTargetColumn  *tgtcol = NULL;

            listsrccol = cwrapobj->m_newval->m_listsrccols;
            listtgtcol = cwrapobj->m_newval->m_listtgtcols;

            if(listsrccol)
                srccol = (FKSourceColumn *) listsrccol->GetFirst();

            if(listtgtcol)
                tgtcol = (FKTargetColumn *) listtgtcol->GetFirst();

            /*
            if((srccol && !tgtcol) || (!srccol && tgtcol))  //This case will be cought in validating source/target columns (wyString)
            */
            wyInt32 cntsrccols = 0, cnttgtcols = 0;
            
            while(srccol)
            {
                cntsrccols++;
                srccol = (FKSourceColumn *) srccol->m_next;
            }

            while(tgtcol)
            {
                if(tgtcol->m_selected)
                    cnttgtcols++;

                tgtcol = (FKTargetColumn *) tgtcol->m_next;
            }
            if(cntsrccols != cnttgtcols)
            {
                if(!cwrapobj->m_errmsg)
                {
                    cwrapobj->m_errmsg = new wyString();
                    cwrapobj->m_errmsg->SetAs(UNEQUAL_SRC_TGT_COLS);
                }

                if(showmsg)
                {
                    if(m_ptabmgmt->GetActiveTabImage() != IDI_TABIMG_FOREIGNKEYS)
                        CustomTab_SetCurSel(m_ptabmgmt->m_hwnd, 2);

                    CustomGrid_SetCurSelection(m_hgridfk, row, CHILDCOLUMNS, wyTrue);
                    CustomGrid_EnsureVisible(m_hgridfk, row, CHILDCOLUMNS, wyTrue);

                    MessageBox(m_hwnd, UNEQUAL_SRC_TGT_COLS, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
                    return wyFalse;
                }
            }
        }

        //..Warning for Drop and Recreate FK
        if(showmsg && flag)
        {
            if(cwrapobj->m_oldval && cwrapobj->m_oldval->m_name.Compare(cwrapobj->m_newval->m_name) == 0)
            {
                flag = wyFalse;
                wyInt32 ret = MessageBox(m_hwnd, EDITRELERROR, pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONWARNING);

                if(ret != IDYES)
                {
                    return wyFalse;
                }
            }
        }
    }
    return wyTrue;
}

wyBool 
TabForeignKeys::GetDroppedFKs(wyString& query)
{
    FKStructWrapper *cwrapobj = (FKStructWrapper *)m_listfkwrappers.GetFirst();

    for(; cwrapobj; cwrapobj=(FKStructWrapper*)cwrapobj->m_next)
    {
        if(!cwrapobj->m_oldval)
            continue;

        if(cwrapobj->m_oldval != cwrapobj->m_newval)        //..m_newval may be NULL or some modified value
        {
            query.AddSprintf("\r\n  drop foreign key `%s`,", cwrapobj->m_oldval->m_name.GetString());
        }
    }
    return wyTrue;
}

wyBool
TabForeignKeys::GenerateAlterQuery(wyString &query)
{
    wyString    localquery;
    GetDroppedFKs(localquery);

    query.AddSprintf("%s", localquery.GetString());
    return wyTrue;
}

wyUInt32
TabForeignKeys::GetGridCellData(HWND hwndgrid, wyUInt32 row, wyUInt32 col, wyString &celldata)
{
    wyString    tempstr("");
    wyWChar     *data;
    wyUInt32    celldatalen = 0;

    celldata.Clear();

    if(row < 0)
        return 0;

    celldatalen = 0;
    celldatalen = CustomGrid_GetItemTextLength(hwndgrid, row, col);
    if(celldatalen)
    {
        data = (wyWChar*)malloc(sizeof(wyWChar) * (celldatalen + 1));
        data[0] = '\0';

        CustomGrid_GetItemText(hwndgrid, row, col, data);
        tempstr.SetAs(data);
        free(data);
    }
    else
        tempstr.SetAs("");

    tempstr.RTrim();
    celldata.SetAs(tempstr);
    return celldata.GetLength();
}

void
TabForeignKeys::CancelChanges()
{
    wyUInt32    count = -1, row = -1;
    FKStructWrapper *cwrapobj = NULL;

    cwrapobj = (FKStructWrapper*) m_listfkwrappers.GetFirst();
    CustomGrid_ApplyChanges(m_hgridfk, wyTrue);

    count = CustomGrid_GetRowCount(m_hgridfk);
    for(row = 0; row < count; row++)
    {
        cwrapobj = (FKStructWrapper *)CustomGrid_GetRowLongData(m_hgridfk, row);

        if(cwrapobj && cwrapobj->m_newval && !cwrapobj->m_oldval)                           //..Create/Alter table (If fk is newly added)
        {
            m_listfkwrappers.Remove(cwrapobj);

            FKSourceColumn          *srccol = NULL, *prevsrccol = NULL;
            
            if(cwrapobj->m_newval->m_listsrccols)
                srccol = (FKSourceColumn *) cwrapobj->m_newval->m_listsrccols->GetFirst();

            while(srccol)
            {
                prevsrccol = srccol;
                srccol = (FKSourceColumn *)cwrapobj->m_newval->m_listsrccols->Remove(srccol);
                delete prevsrccol;
            }

            //don't call this function. It will try to remove the fk-entries from field-wrappers (and will crash)
            //ClearListSrcCols(cwrapobj, cwrapobj->m_newval->m_listsrccols);
            ClearListTgtCols(cwrapobj->m_newval->m_listtgtcols);
            delete cwrapobj;
        }
        else if(cwrapobj && cwrapobj->m_newval && cwrapobj->m_oldval)                           //..Alter table (If existing fk is not dropped)
        {
            if(cwrapobj->m_newval != cwrapobj->m_oldval)                                    //..If field modified, delete m_newval, and set it as m_oldval
            {
                FKSourceColumn          *srccol = NULL, *prevsrccol = NULL;
            
                if(cwrapobj->m_newval->m_listsrccols)
                    srccol = (FKSourceColumn *) cwrapobj->m_newval->m_listsrccols->GetFirst();

                while(srccol)
                {
                    prevsrccol = srccol;
                    srccol = (FKSourceColumn *)cwrapobj->m_newval->m_listsrccols->Remove(srccol);
                    delete prevsrccol;
                }

                //don't call this function. It will try to remove the fk-entries from field-wrappers (and will crash)
                //ClearListSrcCols(cwrapobj, cwrapobj->m_newval->m_listsrccols);

                ClearListTgtCols(cwrapobj->m_newval->m_listtgtcols);
                delete cwrapobj->m_newval;
            }
            cwrapobj->m_newval = cwrapobj->m_oldval;
        }
        else if(cwrapobj && !cwrapobj->m_newval && cwrapobj->m_oldval)                           //..Alter table (If all columns from entire row of the existing fk are deleted)
        {
            cwrapobj->m_newval = cwrapobj->m_oldval;
        }
    }

    cwrapobj = (FKStructWrapper*)m_listfkwrappers.GetFirst();
    while(cwrapobj)
    {
        if(cwrapobj->m_errmsg)
            delete cwrapobj->m_errmsg;
        cwrapobj->m_errmsg = NULL;

        cwrapobj->m_newval = cwrapobj->m_oldval;
        cwrapobj = (FKStructWrapper*)cwrapobj->m_next;
    }

    //..Create table
    if(! m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
    {
        CustomGrid_DeleteAllRow(m_hgridfk, wyTrue);
        
        InsertRow();

        InvalidateRect(m_hgridfk, NULL, TRUE);
        return;
    }

    m_lastclickfkgrid = -1;

    //..Alter table
    CustomGrid_DeleteAllRow(m_hgridfk, wyTrue);
    FillInitValues();

    InsertRow();

    if(m_ptabmgmt->GetActiveTabImage() == IDI_TABIMG_INDEXES)
    {
        SetFocus(m_hgridfk);
        CustomGrid_SetCurSelection(m_hgridfk, 0, 0, wyTrue);
    }
    return;
}

wyBool
TabForeignKeys::AddFKOnDrag(FKStructWrapper *cwrap, List *temp_listsrccols)
{
    wyUInt32            row, count = CustomGrid_GetRowCount(m_hgridfk);
    List                *listsrccol = NULL;
    FieldStructWrapper  *fieldwrap = NULL;
    FKSrcColName_SD     *temp_srccol = NULL;
    FKSourceColumn      *srccol = NULL;
    wyString            strsrccols, strtgtcols, tempstr;

    if(!cwrap)
        return wyFalse;

    listsrccol = new List();

    temp_srccol =(FKSrcColName_SD*) temp_listsrccols->GetFirst();

    //..Extract source column names from the list and get field wrappers and form list of source-columns containing wrappers
    while(temp_srccol)
    {
        fieldwrap = m_ptabmgmt->m_tabfields->GetWrapperObjectPointer(temp_srccol->m_colname);
        if(!fieldwrap)
            return wyFalse;

        tempstr.SetAs(temp_srccol->m_colname);
        tempstr.FindAndReplace("`", "``");

        strsrccols.AddSprintf("`%s`, ", tempstr.GetString());

        srccol = new FKSourceColumn(fieldwrap);
        listsrccol->Insert(srccol);

        temp_srccol = (FKSrcColName_SD*)temp_srccol->m_next;
    }

    strsrccols.RTrim();
    if(strsrccols.GetLength())
    {
        while(strsrccols.GetCharAt(strsrccols.GetLength() - 1) == ',')
            strsrccols.Strip(1);
    }

    cwrap->m_newval->m_listsrccols = listsrccol;
    cwrap->m_newval->m_srccols.SetAs(strsrccols);
    
    row = CustomGrid_InsertRowInBetween(m_hgridfk, count-1);
    
    CustomGrid_EnsureVisible(m_hgridfk, row, 1);
    CustomGrid_SetRowLongData(m_hgridfk, row, (LPARAM) cwrap);

    CustomGrid_SetText(m_hgridfk, row, CHILDCOLUMNS, strsrccols.GetString());
    CustomGrid_SetItemLongValue(m_hgridfk, row, CHILDCOLUMNS, (LPARAM)cwrap->m_newval->m_listsrccols);
    CustomGrid_SetText(m_hgridfk, row, 2, cwrap->m_newval->m_tgtdb.GetString());
    CustomGrid_SetText(m_hgridfk, row, 3, cwrap->m_newval->m_tgttbl.GetString());
    
    CustomGrid_SetCurSelRow(m_hgridfk, row);
    OnTgtTblEndLabelEdit(row, cwrap->m_newval->m_tgttbl);
    CustomGrid_SetItemLongValue(m_hgridfk, row, 4, (LPARAM)cwrap->m_newval->m_listtgtcols);

    m_listfkwrappers.Insert(cwrap);

    CustomGrid_SetButtonVis(m_hgridfk, row, CHILDCOLUMNS, wyTrue);
    CustomGrid_SetButtonText(m_hgridfk, row, CHILDCOLUMNS, L"...");
    CustomGrid_SetButtonVis(m_hgridfk, row, PARENTCOLUMNS, wyTrue);
    CustomGrid_SetButtonText(m_hgridfk, row, PARENTCOLUMNS, L"...");


    m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);
    return wyTrue;
}

void
TabForeignKeys::ClearAllMemory()
{
    FKStructWrapper *cwrapobj = NULL, *tmpcwapobj = NULL;

    cwrapobj = (FKStructWrapper *) m_listfkwrappers.GetFirst();
    while(cwrapobj)
    {
        tmpcwapobj = cwrapobj;
        cwrapobj = (FKStructWrapper *) m_listfkwrappers.Remove(cwrapobj);
        delete tmpcwapobj;
    }
}

wyBool
TabForeignKeys::IsTableInnoDB(HWND m_hwnd)      //..Remove this function and use the one defined in TableTabInerface
{
	wyString        query;

	query.Sprintf("show table status from `%s` like '%s'",
						m_db.GetString(), m_table.GetString());

    SetCursor(LoadCursor(NULL, IDC_WAIT ));
    m_mystatusres = ExecuteAndGetResult(GetActiveWin(), m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query);

    if(!m_mystatusres && m_mdiwnd->m_tunnel->mysql_affected_rows(m_mdiwnd->m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW ));
		return wyFalse;
	}

	VERIFY(m_mystatusrow = m_mdiwnd->m_tunnel->mysql_fetch_row(m_mystatusres));
	
	if(!m_mystatusrow || !m_mystatusrow[1] || (stricmp(m_mystatusrow[1], "InnoDB") != 0 && stricmp(m_mystatusrow[1], "pbxt")!= 0 &&
        stricmp(m_mystatusrow[1], "solidDB") != 0 ))
	{
        MessageBox(m_hwnd, _(L"The selected table does not support foreign keys.\nTable engine must be InnoDB, PBXT or SolidDB."), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
		m_mdiwnd->m_tunnel->mysql_free_result(m_mystatusres);
		m_mystatusres = NULL;
		SetCursor(LoadCursor(NULL, IDC_ARROW ));
		yog_enddialog(m_hwnd, 0);
		return wyFalse;
	}

	SetCursor(LoadCursor(NULL, IDC_ARROW ));

	return wyTrue;
}

void
TabForeignKeys::ReInitializeGrid(List *unsavedfkwrappers)
{

    //..Deleting all rows
    CustomGrid_DeleteAllRow(m_hgridfk, wyTrue);

    //..Clearing all wrapers from m_listfkwrappers
    ClearAllMemory();

    FetchInitValues();
    FillInitValues(unsavedfkwrappers);

    InsertRow();
}

void
TabForeignKeys::Refresh()
{
	FKStructWrapper *cwrapobj = NULL;
	wyUInt32    nrows = 0;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

	cwrapobj = (FKStructWrapper *)m_listfkwrappers.GetFirst();
	while (cwrapobj)
	{
		if (cwrapobj->m_oldval)
			Refresh(cwrapobj->m_oldval);
		if (cwrapobj->m_newval && cwrapobj->m_newval != cwrapobj->m_oldval)
			Refresh(cwrapobj->m_newval);

		cwrapobj = (FKStructWrapper *)cwrapobj->m_next;
	}

	nrows = CustomGrid_GetRowCount(m_hgridfk);
	cwrapobj = NULL;

	for (int row = 0; row<nrows; row++)
	{
		cwrapobj = (FKStructWrapper *)CustomGrid_GetRowLongData(m_hgridfk, row);

		if (!cwrapobj)
			continue;

		if (!cwrapobj->m_newval)
			continue;

		CustomGrid_SetText(m_hgridfk, row, CHILDCOLUMNS, cwrapobj->m_newval->m_srccols.GetString());
		CustomGrid_SetText(m_hgridfk, row, PARENTCOLUMNS, cwrapobj->m_newval->m_tgtcols.GetString());
	}
}

void
TabForeignKeys::Refresh(StructFK* fkInfo)
{
	FKSourceColumn *srccols = NULL;
	FKTargetColumn *tgtcols = NULL;
	wyString        srccolsstr(""), tgtcolsstr("");

	if (fkInfo->m_listsrccols)
	{
		srccols = (FKSourceColumn*)fkInfo->m_listsrccols->GetFirst();

		while (srccols)
		{
			wyString tmpstr;
			tmpstr.SetAs(srccols->m_pcwrapobj->m_newval->m_name);
			tmpstr.FindAndReplace("`", "``");

			srccolsstr.AddSprintf("%s%s%s, ", m_backtick, tmpstr.GetString(), m_backtick);

			srccols = (FKSourceColumn*)srccols->m_next;
		}

		srccolsstr.RTrim();
		if (srccolsstr.GetLength())
		{
			while (srccolsstr.GetCharAt(srccolsstr.GetLength() - 1) == ',')
				srccolsstr.Strip(1);
		}

		fkInfo->m_srccols.SetAs(srccolsstr);
	}

	if (fkInfo->m_listtgtcols)
	{
		tgtcols = (FKTargetColumn *)fkInfo->m_listtgtcols->GetFirst();

		while (tgtcols)
		{
			if (tgtcols->m_selected)
			{
				wyString tmpstr;
				tmpstr.SetAs(tgtcols->m_name);
				tmpstr.FindAndReplace("`", "``");

				tgtcolsstr.AddSprintf("%s%s%s, ", m_backtick, tmpstr.GetString(), m_backtick);
			}

			tgtcols = (FKTargetColumn *)tgtcols->m_next;
		}

		tgtcolsstr.RTrim();
		if (tgtcolsstr.GetLength())
		{
			while (tgtcolsstr.GetCharAt(tgtcolsstr.GetLength() - 1) == ',')
				tgtcolsstr.Strip(1);
		}
		fkInfo->m_tgtcols.SetAs(tgtcolsstr);
	}
}

wyBool
TabForeignKeys::GenerateFKDropRecreateQuery(wyString **query, wyUInt32 gridrow)
{
    wyString    str("");
    wyString    fkname, tgtdb, tgttbl;
    FKStructWrapper *cwrapobj = NULL;

    cwrapobj = (FKStructWrapper *) CustomGrid_GetRowLongData(m_hgridfk, gridrow);

    if(!cwrapobj)
        return wyFalse;

    if(!cwrapobj->m_newval || !cwrapobj->m_oldval)     //..either dropped or newly added
        return wyFalse;

    if(cwrapobj->m_oldval == cwrapobj->m_newval)    //..Unchanged existing fks..
        return wyFalse;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    tgtdb.SetAs(cwrapobj->m_newval->m_tgtdb);
    tgtdb.FindAndReplace("`", "``");

    tgttbl.SetAs(cwrapobj->m_newval->m_tgttbl);
    tgttbl.FindAndReplace("`", "``");

    if(cwrapobj->m_oldval->m_name.CompareI(cwrapobj->m_newval->m_name) == 0)
    {
        fkname.SetAs(cwrapobj->m_oldval->m_name);
        fkname.FindAndReplace("`", "``");

        query[0]->Sprintf("drop foreign key %s%s%s", m_backtick, fkname.GetString(), m_backtick);

        if(!cwrapobj->m_newval->m_name.GetLength())
        {
            query[1]->Sprintf("add constraint foreign key (%s) references %s%s%s.%s%s%s(%s)", cwrapobj->m_newval->m_srccols.GetString(),
											m_backtick, tgtdb.GetString(), m_backtick,
											m_backtick, tgttbl.GetString(), m_backtick,
											cwrapobj->m_newval->m_tgtcols.GetString());
        }
        else
        {
            query[1]->Sprintf("add constraint %s%s%s foreign key (%s) references %s%s%s.%s%s%s(%s)", 
											m_backtick, fkname.GetString(), m_backtick,
                                            cwrapobj->m_newval->m_srccols.GetString(), 
											m_backtick, tgtdb.GetString(), m_backtick,
											m_backtick, tgttbl.GetString(), m_backtick, 
											cwrapobj->m_newval->m_tgtcols.GetString());
        }
        if(cwrapobj->m_newval->m_onupdate.GetLength())
            query[1]->AddSprintf(" on update %s", cwrapobj->m_newval->m_onupdate.GetString());

        if(cwrapobj->m_newval->m_ondelete.GetLength())
            query[1]->AddSprintf(" on delete %s", cwrapobj->m_newval->m_ondelete.GetString());

        if(cwrapobj->m_errmsg  && cwrapobj->m_errmsg->GetLength())
            query[1]->AddSprintf("\t\t/* %s */", cwrapobj->m_errmsg->GetString());

    }
    else
    {
        fkname.SetAs(cwrapobj->m_newval->m_name);
        fkname.FindAndReplace("`", "``");

        if(cwrapobj->m_newval->m_name.GetLength())
        {
            query[0]->Sprintf("\r\n  add constraint %s%s%s foreign key (%s) references %s%s%s.%s%s%s(%s)", 
									m_backtick, fkname.GetString(), m_backtick,
                                    cwrapobj->m_newval->m_srccols.GetString(), 
									m_backtick, tgtdb.GetString(), m_backtick,
									m_backtick, tgttbl.GetString(), m_backtick,
                                    cwrapobj->m_newval->m_tgtcols.GetString());
        }
        else
        {
            query[0]->Sprintf("\r\n  add constraint foreign key (%s) references %s%s%s.%s%s%s(%s)", cwrapobj->m_newval->m_srccols.GetString(),
									m_backtick, tgtdb.GetString(), m_backtick,
									m_backtick, tgttbl.GetString(), m_backtick,
									cwrapobj->m_newval->m_tgtcols.GetString());
        }
        
        if(cwrapobj->m_newval->m_onupdate.GetLength())
            query[0]->AddSprintf(" on update %s", cwrapobj->m_newval->m_onupdate.GetString());

        if(cwrapobj->m_newval->m_ondelete.GetLength())
            query[0]->AddSprintf(" on delete %s", cwrapobj->m_newval->m_ondelete.GetString());

        query[0]->Add(",");

        if(cwrapobj->m_errmsg && cwrapobj->m_errmsg->GetLength())
            query[0]->AddSprintf("\t\t/* %s */", cwrapobj->m_errmsg->GetString());

        fkname.SetAs(cwrapobj->m_oldval->m_name);
        fkname.FindAndReplace("`", "``");

        query[0]->AddSprintf("\r\n  drop foreign key %s%s%s", m_backtick, fkname.GetString(), m_backtick );
    }

    return wyTrue;
}

void
TabForeignKeys::SetValueToStructure(wyUInt32 row, wyUInt32 col, wyChar* data)
{
    FKStructWrapper *cwrapobj = NULL;

    cwrapobj = (FKStructWrapper *) CustomGrid_GetRowLongData(m_hgridfk, row);

    switch(col)
    {
    case CONSTRAINT:
        cwrapobj->m_newval->m_name.SetAs(data);
        break;

    case CHILDCOLUMNS:
        cwrapobj->m_newval->m_srccols.SetAs(data);
        break;

    case PARENTDATABASE:
        cwrapobj->m_newval->m_tgtdb.SetAs(data);
        if(strlen(data) == 0)
        {
            cwrapobj->m_newval->m_tgttbl.Clear();
            cwrapobj->m_newval->m_tgtcols.Clear();
        }
        break;

    case PARENTTABLE:
        cwrapobj->m_newval->m_tgttbl.SetAs(data);
        if(strlen(data) == 0)
            cwrapobj->m_newval->m_tgtcols.Clear();
        break;

    case PARENTCOLUMNS:
        cwrapobj->m_newval->m_tgtcols.SetAs(data);
        break;

    case ONUPDATE:
        cwrapobj->m_newval->m_onupdate.SetAs(data);
        break;

    case ONDELETE:
        cwrapobj->m_newval->m_ondelete.SetAs(data);
        break;
    }
}

wyBool
TabForeignKeys::GenerateNewAndDroppedFKQuery(wyString   &query)
{
    FKStructWrapper *cwrapobj = (FKStructWrapper *)m_listfkwrappers.GetFirst();
    wyString    dropquery(""), addquery("");
    wyString    tmpstr(""), tgtdb, tgttbl;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    for( ; cwrapobj; cwrapobj = (FKStructWrapper *)cwrapobj->m_next)
    {
        //..If existing fk is dropped from the grid
        if(cwrapobj->m_oldval && !cwrapobj->m_newval)
        {
            tmpstr.SetAs(cwrapobj->m_oldval->m_name);
            tmpstr.FindAndReplace("`", "``");
            dropquery.AddSprintf("\r\n  drop foreign key %s%s%s,", m_backtick, tmpstr.GetString(), m_backtick);
        }
        //..If added a new fk
        else if(!cwrapobj->m_oldval && cwrapobj->m_newval)
        {
            addquery.Add("\r\n  ");

            if(m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
                addquery.Add("add ");

            tmpstr.SetAs(cwrapobj->m_newval->m_name);
            tmpstr.FindAndReplace("`", "``");

            tgtdb.SetAs(cwrapobj->m_newval->m_tgtdb);
            tgtdb.FindAndReplace("`", "``");

            tgttbl.SetAs(cwrapobj->m_newval->m_tgttbl);
            tgttbl.FindAndReplace("`", "``");

            if(cwrapobj->m_newval->m_name.GetLength())
                addquery.AddSprintf("constraint %s%s%s foreign key (%s) references %s%s%s.%s%s%s(%s)", 
									m_backtick, tmpstr.GetString(), m_backtick,
                                    cwrapobj->m_newval->m_srccols.GetString(),
									m_backtick, tgtdb.GetString(), m_backtick,
									m_backtick, tgttbl.GetString(), m_backtick,
                                    cwrapobj->m_newval->m_tgtcols.GetString());

            else
                addquery.AddSprintf("foreign key (%s) references %s%s%s.%s%s%s(%s)", cwrapobj->m_newval->m_srccols.GetString(),
									m_backtick, tgtdb.GetString(), m_backtick,
									m_backtick, tgttbl.GetString(), m_backtick,
                                    cwrapobj->m_newval->m_tgtcols.GetString());

            if(cwrapobj->m_newval->m_onupdate.GetLength())
                addquery.AddSprintf(" on update %s", cwrapobj->m_newval->m_onupdate.GetString());

            if(cwrapobj->m_newval->m_ondelete.GetLength())
                addquery.AddSprintf(" on delete %s", cwrapobj->m_newval->m_ondelete.GetString());

            addquery.Add(",");

            if(cwrapobj->m_errmsg)
                addquery.AddSprintf("\t\t/* %s */", cwrapobj->m_errmsg->GetString());
        }
    }
    query.AddSprintf("%s%s", dropquery.GetString(), addquery.GetString());
    return wyTrue;
}

//restrict the minimum size
void
TabForeignKeys::OnWMSizeInfo(LPARAM lparam, HWND hwnd)
{
    MINMAXINFO* pminmax = (MINMAXINFO*)lparam;

    pminmax->ptMinTrackSize.x = m_wndrect.right - m_wndrect.left;
    pminmax->ptMinTrackSize.y = m_wndrect.bottom - m_wndrect.top+130;
}

void
TabForeignKeys::PositionCtrls(HWND hwnd)
{
    DlgControl* pdlgctrl;
    wyInt32     leftpadding, toppadding, rightpadding, bottompadding, width, height;
    wyInt32     x, y;
    RECT        rect;
    HDWP        hdefwp;

    GetClientRect(hwnd, &rect);

    //BeginDeferWindowPos is used to make the control positioning atomic
    hdefwp = BeginDeferWindowPos(m_controllist.GetCount() + 1);

    //iterate throught the control lists
	for(pdlgctrl = (DlgControl*)m_controllist.GetFirst(); pdlgctrl && pdlgctrl->m_next; 
        pdlgctrl = (DlgControl*)pdlgctrl->m_next)
    {
        leftpadding = pdlgctrl->m_rect.left - m_dlgrect.left;
        toppadding = pdlgctrl->m_rect.top - m_dlgrect.top;
        rightpadding = m_dlgrect.right - pdlgctrl->m_rect.right;
        bottompadding = m_dlgrect.bottom - pdlgctrl->m_rect.bottom;
        width = pdlgctrl->m_rect.right - pdlgctrl->m_rect.left;
        height = pdlgctrl->m_rect.bottom - pdlgctrl->m_rect.top;
        
        if(pdlgctrl->m_issizex == wyFalse)
        {
            switch(pdlgctrl->m_id)
			{
				case IDC_HEADING:
				case IDC_MOVEUP:
				case IDC_MOVEDOWN:
					x = leftpadding;
					break;

				case IDC_INDEXGRIP:
					x = rect.right - width;

				default:
					x = rect.right - rightpadding - width;
			}
        }
        else
        {
            x = leftpadding;
            width = rect.right - rightpadding - leftpadding;
        }

        switch(pdlgctrl->m_id)
        {
			case IDC_HEADING:
			case IDC_TOPLINE:
				y = toppadding;
				break;

			case IDC_INDEXGRIP:
				y = rect.bottom - height;
				break;
		
			default:
				y = rect.bottom - bottompadding - height;

        }
        //change the control position
		hdefwp = DeferWindowPos(hdefwp, pdlgctrl->m_hwnd, NULL, x, y, width, height, SWP_NOZORDER);
    }

	height = rect.bottom - rect.top - 70;
	width = rect.right - rect.left - 25;

    hdefwp = DeferWindowPos(hdefwp, m_hdlggrid, NULL, 12 , 30 , width, height, SWP_NOZORDER);

	//finish the operation and apply changes
	EndDeferWindowPos(hdefwp);
    InvalidateRect(m_hdlggrid, NULL, TRUE);
    UpdateWindow(m_hdlggrid);
}

void
TabForeignKeys::GetCtrlRects(HWND hwnddlg)
{
    RECT    rect;
    wyInt32 i, count;
    HWND    hwnd;

    //ctrlid    size-x     size-y
    wyInt32 ctrlid[] = {
        IDC_BOTLINE, 1, 0,
        IDC_TOPLINE, 1, 0,
        IDC_MOVEUP, 0, 0,
        IDC_MOVEDOWN, 0, 0,
		IDC_HEADING, 0, 0,
        IDOK, 0, 0,
        IDC_INDEXGRIP, 0, 0,
        IDCANCEL, 0, 0
    };

    count = sizeof(ctrlid)/sizeof(ctrlid[0]);

    //store the control handles and related infos in the linked list
    for(i = 0; i < count; i += 3)
    {
        hwnd = GetDlgItem(hwnddlg, ctrlid[i]);
        GetWindowRect(hwnd, &rect);
        MapWindowPoints(NULL, hwnddlg, (LPPOINT)&rect, 2);
        m_controllist.Insert(new DlgControl(hwnd, 
                                            ctrlid[i], 
                                            &rect, 
                                            ctrlid[i + 1] ? wyTrue : wyFalse, 
                                            ctrlid[i + 2] ? wyTrue : wyFalse));
    }

	GetWindowRect(m_hdlggrid, &rect);
    MapWindowPoints(NULL, hwnddlg, (LPPOINT)&rect, 2);
    m_controllist.Insert(new DlgControl(m_hdlggrid, 0, &rect, wyTrue, wyTrue));
}

void
TabForeignKeys::OnWMPaint(HWND hwnd)
{
	HDC         hdc;
	PAINTSTRUCT ps;
	HWND hwndstat = GetDlgItem(hwnd, IDC_INDEXGRIP);
	RECT temp;
	
	VERIFY(hdc = BeginPaint(hwnd, &ps));
	DoubleBuffer::EraseBackground(hwnd, hdc, NULL, GetSysColor(COLOR_3DFACE));
	EndPaint(hwnd, &ps);

	//To paint the resize grip
	VERIFY(hdc = BeginPaint(hwndstat, &ps));
    GetClientRect(hwndstat, &temp);
    DrawFrameControl(hdc, &temp, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
    EndPaint(hwnd, &ps);
}

void
TabForeignKeys::PositionWindow(RECT* prect, HWND hwnd)
{
    HMONITOR    hmonitor;
    MONITORINFO mi = {0};
    RECT        temprect = {0};
    wyInt32     animate = 0, width, height, hmargin = 0, vmargin = 0;

    //get the modified window rect
    GetWindowRect(hwnd, &temprect);

    //get the monitor info for the monitor associated with the rect
    hmonitor = MonitorFromRect(prect, MONITOR_DEFAULTTONEAREST);
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hmonitor, &mi);

    //calculate and modify the width and height based on the availabe space to best fit the window
    width = temprect.right - temprect.left;
    height = temprect.bottom - temprect.top+130;
    hmargin = mi.rcWork.right - prect->left;
    hmargin = prect->left - mi.rcWork.left > hmargin ? prect->left - mi.rcWork.left : hmargin;
    vmargin = mi.rcWork.bottom - prect->bottom;
    vmargin = prect->top - mi.rcWork.top > vmargin ? prect->top - mi.rcWork.top : vmargin;

    if(width > hmargin)
    {
        width = hmargin;
    }

    if(height > vmargin)
    {
        height = vmargin;
    }

    //resize the window
    SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
    
    //calculate the left cordinate of the window
    if(prect->left + width > mi.rcWork.right)
    {  
        prect->left = prect->right - width;
        animate |= AW_HOR_NEGATIVE;
    }
    else
    {
        animate |= AW_HOR_POSITIVE;
    }

    //calculate the top cordinate of the window
    if(prect->bottom + height > mi.rcWork.bottom)
    {
        prect->bottom = prect->top - height;
        animate |= AW_VER_NEGATIVE;
    }
    else
    {
        animate |= AW_VER_POSITIVE;
    }

    //position the window properly
    SetWindowPos(hwnd, NULL, prect->left, prect->bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    //now position the controls
    PositionCtrls(hwnd);

    //AnimateWindow(hwnd, 200, AW_ACTIVATE | animate);
    //InvalidateRect(m_hwnd, NULL, TRUE);
    //UpdateWindow(m_hwnd);
}

wyBool
TabForeignKeys::SelectForeignKey(wyString &fkname)
{
    /// Selecting grid row for the fkname
    wyUInt32    row, nrows;
    wyString    celldata, tempstr;

    tempstr.SetAs(fkname);

    tempstr.Erase(0, 1);
    tempstr.Erase(tempstr.GetLength()-1, 1);

    nrows = CustomGrid_GetRowCount(m_hgridfk);

    for(row = 0; row < nrows; row++)
    {
        GetGridCellData(m_hgridfk, row, 0, celldata);
        if(celldata.Compare(tempstr) == 0)
        {
            CustomGrid_SetCurSelection(m_hgridfk, row, 1);
            return wyTrue;
        }
    }

    return wyFalse;
}