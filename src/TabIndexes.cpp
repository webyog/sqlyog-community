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


#include "TabIndexes.h"
#include "MDIWindow.h"
#include "Global.h"
#include "GUIHelper.h"
#include "CommonHelper.h"
#include "TableTabInterface.h"
#include "TableTabInterfaceTabMgmt.h"
#include "TabFields.h"

extern PGLOBALS		pGlobals;

#define INDEXNAME       0
#define INDEXCOLUMNS    1
#define INDEXTYPE       2
#define INDEXCOMMENT	3

#define UM_SETINITFOCUS 545

#define			HNDLEIDXMGWD		    400
#define			HNDLEIDXMGHT		    275
#define			GRIDCHECKBOXWD		    75

#define         UM_GRIDROWFOCUSCHANGE   4628

IndexColumn::IndexColumn(FieldStructWrapper *value)
{   
    m_pcwrapobj = value;
    m_lenth = -1;
}

IndexesStructWrapper::IndexesStructWrapper(IndexInfo *value, wyBool isnew)
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

IndexesStructWrapper::~IndexesStructWrapper()
{
    List    *lstcols = NULL;
    IndexColumn   *indcol1 = NULL, *indcol2 = NULL;

    
    //..Removing m_oldval elements of Index-Columns list
    if(m_oldval)
    {
        lstcols = m_oldval->m_listcolumns;

        if(lstcols)
            indcol1 = (IndexColumn*) lstcols->GetFirst();

        while(indcol1)
        {
            indcol2 = (IndexColumn*) lstcols->Remove(indcol1);
            delete indcol1;
            indcol1 = indcol2;
        }
        if(lstcols)
            delete lstcols;
    
        m_oldval->m_listcolumns = lstcols = NULL;
    }

    //..Removing m_oldval elements of Index-Columns list
    if(m_newval)
    {
        lstcols = m_newval->m_listcolumns;

        if(lstcols)
            indcol1 = (IndexColumn*) lstcols->GetFirst();

        while(indcol1)
        {
            indcol2 = (IndexColumn*) lstcols->Remove(indcol1);
            delete indcol1;
            indcol1 = indcol2;
        }
        if(lstcols)
            delete lstcols;
    
        m_newval->m_listcolumns = lstcols = NULL;
    }

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

TabIndexes::TabIndexes(HWND hwnd, TableTabInterfaceTabMgmt* ptabmgmt)
{
    m_hwnd                  =   hwnd;
    m_hgridindexes          =   NULL;
    m_hdlggrid              =   NULL;
    m_mdiwnd                =   GetActiveWin();
    m_ptabmgmt              =   ptabmgmt;
    m_ismysql41             =   IsMySQL41(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));
	m_ismariadb52           =   IsMySQL564MariaDB53(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));
	m_ismysql553			=	IsMySQL553MariaDB55(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));
    
    m_automatedindexrow		=	-1;
    m_lastclickindgrid      =   -1;
    m_lastclickdlggrid      =   -1;
}

TabIndexes::~TabIndexes()
{
    ClearAllMemory(wyTrue);
}

wyBool
TabIndexes::Create()
{
    m_hgridindexes  =   CreateCustomGridEx(m_hwnd, 0, 0, 0, 0, (GVWNDPROC)GridWndProc, GV_EX_ROWCHECKBOX, (LPARAM)this);

    //..Initializing grid
    InitGrid();

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    if(m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
    {
        //..Fetching index values
        if(!FetchIndexValuesIntoWrapper())
            return wyFalse;

        //..Setting index-values into grid
        FillInitValues();
    }
    //..Inserting extra row
    InsertRow();
    
    ShowWindow(m_hgridindexes, SW_HIDE);
    return wyTrue;
}

wyBool
TabIndexes::FetchIndexValuesIntoWrapper()
{
    wyString query;
    MYSQL_RES	*myres;
	MYSQL_ROW	myrow;
    wyString    tblname(""), dbname("");
    wyString    colstr, indexname(""), indcolsstr(""), indexlength(""),indexcomment("");
    wyBool	    isunique = wyFalse, isfulltext = wyFalse;
    wyInt32     ind_keyname = -1, ind_colname = -1, ind_subpart = -1, ind_nonunique = -1, ind_indextype = -1,ind_indexcomment= -1;
    IndexesStructWrapper   *cwrapobj = NULL;
    IndexInfo                *iindex = NULL;
    IndexColumn           *indcols = NULL;
    List                    *listindcols = NULL;
    FieldStructWrapper      *fieldswrap = NULL;
	wyChar* temp;	

    //..Escaping table name and database name
    tblname.SetAs(m_ptabmgmt->m_tabinterfaceptr->m_origtblname);
    tblname.FindAndReplace("`", "``");

    dbname.SetAs(m_ptabmgmt->m_tabinterfaceptr->m_dbname);
    dbname.FindAndReplace("`", "``");

    query.Sprintf("show keys from `%s`.`%s` ", dbname.GetString(), tblname.GetString());
    myres = ExecuteAndGetResult(m_mdiwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query);
    if(!myres)
	{
		ShowMySQLError(m_hwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		yog_enddialog(m_hwnd, 0);
		return wyFalse;
	}

    ind_keyname     =   GetFieldIndex(myres,"key_name", m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));
    ind_colname     =   GetFieldIndex(myres,"column_name", m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));
    ind_subpart     =   GetFieldIndex(myres,"sub_part", m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));
    ind_nonunique   =   GetFieldIndex(myres,"non_unique", m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));
    ind_indextype   =   GetFieldIndex(myres,"index_type", m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));
	ind_indexcomment=   GetFieldIndex(myres,"Index_comment", m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));

    while(myrow = m_mdiwnd->m_tunnel->mysql_fetch_row(myres))
    {
        //..Will be true when mysql_fetch will fetch another column for the same index as was previously fetched
        if((strcmp(indexname.GetString(), myrow[ind_keyname]))== 0)
		{
			colstr.SetAs(myrow[ind_colname], m_ismysql41);

            wyString tmpstr;
            tmpstr.SetAs(colstr);

            tmpstr.FindAndReplace("`", "``");
            indcolsstr.AddSprintf("%s%s%s", m_backtick, tmpstr.GetString(), m_backtick);
            
            fieldswrap = m_ptabmgmt->m_tabfields->GetWrapperObjectPointer(colstr);
            if(!fieldswrap)     //..If fieldwrapper not found, then return false
                return wyFalse;

            //..Inserting index-wrapper object pointer to the fields-wraper's m_listindexes member
            fieldswrap->m_listindexesbackupcopy.Insert(new IndexedBy(cwrapobj));
            indcols = new IndexColumn(fieldswrap);
            
            if(myrow[ind_subpart])
            {
                indexlength.SetAs(myrow[ind_subpart]);
                indcols->m_lenth = indexlength.GetAsInt32();
                indcolsstr.AddSprintf("(%s)", indexlength.GetString());
            }
            indcolsstr.Add(", ");
            if(!listindcols)
                listindcols = new List();

            listindcols->Insert(indcols);
	//	if(m_ismysql553 && myrow[ind_indexcomment])
	//			indexcomment.SetAs(myrow[ind_indexcomment]);
		}
        else
        {
            //..will be true only when mysql_fetch fetches the very first index..
            if(indexname.GetLength() == 0)
			{
                cwrapobj = new IndexesStructWrapper(NULL, wyFalse);
                m_listwrapperstruct.Insert(cwrapobj);

                indexname.SetAs(myrow[ind_keyname], m_ismysql41);
                colstr.SetAs(myrow[ind_colname], m_ismysql41);
				if(m_ismysql553 && myrow[ind_indexcomment])
					indexcomment.SetAs(myrow[ind_indexcomment], m_ismysql41);
                fieldswrap = m_ptabmgmt->m_tabfields->GetWrapperObjectPointer(colstr);
                //..If fieldwrapper not found, then return false
                if(!fieldswrap)
                    return wyFalse;

                //..Inserting index-wrapper object pointer to the fields-wraper m_listindexes
                fieldswrap->m_listindexesbackupcopy.Insert(new IndexedBy(cwrapobj));

                indcols = new IndexColumn(fieldswrap);
                
                if(myrow[ind_subpart])
                {
                    indexlength.SetAs(myrow[ind_subpart]);
                    indcols->m_lenth = indexlength.GetAsInt32();
                    
                }

                if(!listindcols)
                    listindcols = new List();
                listindcols->Insert(indcols);
                
                indcolsstr.AddSprintf("%s%s%s", m_backtick, colstr.GetString(), m_backtick);
                if(indexlength.GetLength())
                    indcolsstr.AddSprintf("(%s)", indexlength.GetString());

                indcolsstr.Add(", ");

				//..check whether its unique.
				if(stricmp(myrow[ind_nonunique], "0")== 0)
					isunique = wyTrue;

				if(IsMySQL402(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql)))
                {
					if(myrow[ind_indextype] && strstr(myrow[ind_indextype], "FULLTEXT")){
						isfulltext = wyTrue;
					}
				} 
            }
            else    //..will be true when mysql_fetch fetches the different index (except the first-one)
            {
                indcolsstr.Strip(2);    //..will strip last 2 chars. (Last 2 chars will be ','(comma) and ' '(1 space))
                
                iindex = new IndexInfo();
                cwrapobj->m_oldval = cwrapobj->m_newval = iindex;
                iindex->m_name.SetAs(indexname);
                iindex->m_colsstr.SetAs(indcolsstr);
                iindex->m_listcolumns = listindcols;
				if(m_ismysql553)
					iindex->m_indexcomment.SetAs(indexcomment.GetString());

				if(isunique)
                {
                    if(iindex->m_name.CompareI("PRIMARY") == 0)
                        iindex->m_indextype.SetAs("PRIMARY");
                    else
                        iindex->m_indextype.SetAs("UNIQUE");
                }
				else if(isfulltext)
                    iindex->m_indextype.SetAs("FULLTEXT");
				else
					iindex->m_indextype.SetAs("KEY");
                
                listindcols = NULL;

                indcolsstr.Clear();
				indexname.Clear();
				indexcomment.Clear();
					
				isunique = wyFalse;
				isfulltext = wyFalse;

                cwrapobj = new IndexesStructWrapper(NULL, wyFalse);
                m_listwrapperstruct.Insert(cwrapobj);
				// now copy this key into the buffer.
                indexname.SetAs(myrow[ind_keyname], m_ismysql41);
                colstr.SetAs(myrow[ind_colname], m_ismysql41);

				if(m_ismysql553)
				{
					temp = (wyChar *) myrow[ind_indexcomment];
					indexcomment.SetAs(temp, m_ismysql41);
				}
                fieldswrap = m_ptabmgmt->m_tabfields->GetWrapperObjectPointer(colstr);
                //..If fieldwrapper not found, then return false
                if(!fieldswrap)
                    return wyFalse;

                //..Inserting index-wrapper object pointer to the fields-wraper's m_listindexes member
                fieldswrap->m_listindexesbackupcopy.Insert(new IndexedBy(cwrapobj));
                indcols = new IndexColumn(fieldswrap);
                
                if(myrow[ind_subpart])
                {
                    indexlength.SetAs(myrow[ind_subpart]);
                    indcols->m_lenth = indexlength.GetAsInt32();
                }

                //..Create list
                listindcols = new List();
                listindcols->Insert(indcols);

                wyString tmpstr;
                tmpstr.SetAs(colstr);
                tmpstr.FindAndReplace("`", "``");

                indcolsstr.AddSprintf("%s%s%s", m_backtick, tmpstr.GetString(), m_backtick);
				
                if(indexlength.GetLength())
                    indcolsstr.AddSprintf("(%s)", indexlength.GetString());

                indcolsstr.Add(", ");

				if(stricmp(myrow[ind_nonunique], "0")== 0)
					isunique = wyTrue;

				if(IsMySQL402(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql))&& myrow[ind_indextype] && strstr(myrow[ind_indextype], "FULLTEXT"))
					isfulltext = wyTrue;
            }
        }
        indexlength.Clear();
    }

    // now add the last key.
	if(indexname.GetLength() > 0)
	{
        iindex = new IndexInfo();
        cwrapobj->m_oldval = cwrapobj->m_newval = iindex;
		indcolsstr.Strip(2);
		
        //..Setting index values
		iindex->m_name.SetAs(indexname);
        iindex->m_colsstr.SetAs(indcolsstr);
        iindex->m_listcolumns = listindcols;
		if(m_ismysql553)
		iindex->m_indexcomment.SetAs(indexcomment.GetString());
        //..Sets the index type
        if(isunique)
        {
            if(indexname.CompareI("primary") == 0)
                iindex->m_indextype.SetAs("PRIMARY");
            else
                iindex->m_indextype.SetAs("UNIQUE");
        }
		else if(isfulltext)
            iindex->m_indextype.SetAs("FULLTEXT");
		else
			iindex->m_indextype.SetAs("KEY");
    }
    m_mdiwnd->m_tunnel->mysql_free_result(myres);

    return wyTrue;
}

void
TabIndexes::InitGrid()
{
    wyInt32			counter;		// normal counter
    wyInt32			num_cols;		// number of columns
    GVCOLUMN		gvcol;			// structure used to create columns for grid
    wyChar		    *heading[]      = { _("Index Name"), _("Columns"), _("Index Type"), _("Comment")};
    wyInt32			mask[]          = { GVIF_TEXT, GVIF_TEXTBUTTON , GVIF_LIST, GVIF_TEXT};
    wyWChar			type[][20]      = { L"UNIQUE", L"PRIMARY", L"FULLTEXT", L"KEY"};
    VOID		    *listtype[]     = { NULL, NULL, (VOID*)type, NULL};
    wyInt32			elemsize[]      = {0, 10, sizeof(type[0]), 0 };
    wyInt32			elemcount[]     = {0, 8, sizeof(type)/sizeof(type[0]), 0 };
    wyInt32			cx[]            = { 150, 150, 150 , 150};
    wyInt32			format[]        = { GVIF_LEFT, GVIF_LEFT, GVIF_LEFT, GVIF_LEFT };
    wyInt32			width           = 0;

    wyString		colname, dbname(RETAINWIDTH_DBNAME), tblname("__create_table");
	HFONT hfont;

    m_ptabmgmt->m_tabinterfaceptr->SetFont(m_hgridindexes);
	hfont = CustomGrid_GetColumnFont(m_hgridindexes);
	
	num_cols = sizeof (heading)/sizeof(heading[0]);
	if(! m_ismysql553)
		num_cols--;

    for (counter=0; counter < num_cols ; counter++ )
    {
			//for getting the retained column width
			colname.SetAs(heading[counter]);
		width =  GetTextSize(colname.GetAsWideChar(), m_hgridindexes, hfont).right + 15; //GetColumnWidthFromFile(&dbname, &tblname, &colname);
		
		memset(&gvcol, 0,sizeof(gvcol));
		
		gvcol.mask		    = mask[counter];		// Set the mask for the sturcture  i.e. what kind of column in the grid.
		gvcol.fmt		    = format[counter];		// Alignment
		gvcol.pszList	    = listtype[counter];	// set the kind list in between
		gvcol.cx		    = (width < cx[counter] )? cx[counter]:width;
		gvcol.text	        = heading[counter];
		gvcol.nElemSize     = elemsize[counter];
		gvcol.nListCount    = elemcount[counter];
		gvcol.cchTextMax    = strlen(heading[counter]);
        
		CustomGrid_InsertColumn(m_hgridindexes, &gvcol);
    }
    

    return;
}

void
TabIndexes::InitDlgGrid()
{
    wyInt32     i,num_cols;
	GVCOLUMN	gvcol;

	wyChar      *heading[] = {_("Column"), _("Data Type"), _("Length"), _("Comment")};
	wyInt32     elemsize[] = {0 ,0 ,0, 0};
	wyInt32     elemcount[] = {0, 0, 0, 0 };
	wyInt32     mask[] = {GVIF_TEXT, GVIF_TEXT, GVIF_TEXT, GVIF_TEXT };  
	wyInt32     cx[] = {195, 98, 85, 200};
	wyInt32     fmt[] = {GVIF_LEFT, GVIF_LEFT, GVIF_LEFT, GVIF_LEFT };

	wyInt32		width = 0;
	wyString	colname, dbname(RETAINWIDTH_DBNAME), tblname("__create_index");
	wyBool		isretaincolumnwidth = IsRetainColumnWidth();
    HFONT hfont;
    
    m_ptabmgmt->m_tabinterfaceptr->SetFont(m_hdlggrid);
    hfont = CustomGrid_GetRowFont(m_hdlggrid);
	num_cols = sizeof(heading)/ sizeof(heading[0]);
	if(! m_ismysql553)
		num_cols-- ;

	for(i=0; i < num_cols ; i++)
	{
		//if retain user modified width(prefernce option) is checked or not
		//if(isretaincolumnwidth == wyTrue)
		//{
		//	//for getting the resized column width
		//	colname.SetAs(heading[i]);
		//	width =  GetTextSize(colname.GetAsWideChar(), m_hdlggrid, hfont).right + 15;
		//}

		memset(&gvcol, 0, sizeof(gvcol));
		
		gvcol.mask = mask[i];
		gvcol.fmt  = fmt[i];
		gvcol.cx   = /*(width > 0)?width:*/cx[i];
		gvcol.text = heading[i];
		gvcol.nElemSize = elemsize[i];
		gvcol.nListCount = elemcount[i];
		gvcol.cchTextMax = strlen(heading[i]);

		CustomGrid_InsertColumn(m_hdlggrid, &gvcol);
	}
}

LRESULT CALLBACK
TabIndexes::GridWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    
    TabIndexes*     ptabind = (TabIndexes*)CustomGrid_GetLongData(hwnd);
    wyString		tblname("");

    switch(message)
	{
	case GVN_BEGINLABELEDIT:
        {
        return ptabind->OnGVNBeginLabelEdit(hwnd, wParam, lParam);
        }
        break;

    case GVN_BUTTONCLICK:
        {
        ptabind->OnGVNButtonClick();
        }
        break;

	case GVN_ENDLABELEDIT:
        {
        return ptabind->OnGVNEndLabelEdit(wParam,lParam);
        }
        break;

    case GVN_BEGINADDNEWROW:
        return 0;

    case GVN_CHECKBOXCLICK:
        {
            ptabind->HandleCheckboxClick(hwnd, lParam, wParam, ptabind->m_lastclickindgrid);
        }
        break;

    case GVN_DRAWROWCHECK:
        {
            ((GVROWCHECKINFO*)lParam)->ischecked = CustomGrid_GetRowCheckState(hwnd, wParam) ? wyTrue : wyFalse;
        }
        break;

    case GVN_DESTROY:
        {
            ptabind->ClearAllMemory(wyTrue);
            ptabind->m_lastclickindgrid = -1;
        }
        break;

    case GVN_NEXTTABORDERITEM:
        SetFocus(GetNextDlgTabItem(ptabind->m_ptabmgmt->m_tabinterfaceptr->m_hwnd, hwnd, FALSE));
        break;

    case GVN_PREVTABORDERITEM:
        SetFocus(GetNextDlgTabItem(ptabind->m_ptabmgmt->m_tabinterfaceptr->m_hwnd, hwnd, TRUE));
        break;

    case GVN_SYSKEYDOWN:
        return ptabind->OnGVNSysKeyDown(hwnd, wParam, lParam);

    case GVN_HELP:
        {
            if(ptabind->m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
                ShowHelp("http://sqlyogkb.webyog.com/article/92-alter-index");
            else
                ShowHelp("http://sqlyogkb.webyog.com/article/91-create-index");
        }
    }
    return 1;
}

LRESULT
TabIndexes::OnGVNSysKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if(m_ptabmgmt->OnSysKeyDown(hwnd, wParam, lParam) == wyTrue)
    {
        return 1;
    }

    if(!m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog)
        return 1;

    return m_ptabmgmt->m_tabinterfaceptr->OnWMSysKeyDown(1, wParam, lParam);
}
// Function to check whether the length fields should be edited or not coz if 
// it is char, varchar, blob or text then only it should be edited.
LRESULT
TabIndexes::DoLengthValidation(wyInt32 row)
{
	wyWChar  datatype[SIZE_128] = {0};

	CustomGrid_GetItemText(m_hdlggrid, row, 1, datatype);

	if((wcsicmp(datatype, L"blob")== 0)||
		(wcsicmp(datatype, L"json")== 0)||
		(wcsicmp(datatype, L"char")== 0)||
		(wcsicmp(datatype, L"varchar")== 0)||
		(wcsicmp(datatype, L"tinyblob")== 0)||
		(wcsicmp(datatype, L"mediumblob")== 0)||
		(wcsicmp(datatype, L"longblob")== 0)||
		(wcsicmp(datatype, L"text")== 0)||
		(wcsicmp(datatype, L"tinytext")== 0)||
		(wcsicmp(datatype, L"mediumtext")== 0)||
		(wcsicmp(datatype, L"longtext")== 0))
		 return 1;
	
    return 0;
}


LRESULT CALLBACK
TabIndexes::DlgGVWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    TabIndexes*     ptabind = (TabIndexes*)CustomGrid_GetLongData(hwnd);

    switch(message)
	{
    case GVN_ENDCHANGEROW:
        {
			return ptabind->EnableDisableUpDownButtons(hwnd, wParam);
        }
        break;

    case GVN_BEGINLABELEDIT:
        {
            //..Disabling the row editing for the 1st and 2nd column
			if(lParam == 0 || lParam == 1)
				return FALSE;
			else if(lParam == 2)
            {
                //..Check whether length is allowed for the datatype
				return ptabind->DoLengthValidation(wParam);
            }
			else
				return 1;
        }
        break;

	case GVN_ENDLABELEDIT:
        return ptabind->OnDlgGVNEndLabelEdit(wParam,lParam);

    case GVN_BEGINADDNEWROW:
        return 0;

    case GVN_CHECKBOXCLICK:
        ptabind->HandleCheckboxClick(hwnd, lParam, wParam, ptabind->m_lastclickdlggrid);
        break;

    case GVN_DRAWROWCHECK:
        {
            ((GVROWCHECKINFO*)lParam)->ischecked = CustomGrid_GetRowCheckState(hwnd, wParam) ? wyTrue : wyFalse;
        }
        break;

    case GVN_DESTROY:
        //..resetting the last-click-index variable
        ptabind->m_lastclickdlggrid = -1;
        break;

    case GVN_NEXTTABORDERITEM:
        SetFocus(GetNextDlgTabItem(GetParent(hwnd), hwnd, FALSE));
        break;

    case GVN_PREVTABORDERITEM:
        SetFocus(GetNextDlgTabItem(GetParent(hwnd), hwnd, TRUE));
        break;
    }
    return 1;
}

// Function to enable and disable Up/Down button depending upon where the selected row is 
// focused.
LRESULT
TabIndexes::EnableDisableUpDownButtons(HWND hgrid, wyInt32 row)
{
	wyInt32    totrow;
	HWND	    hwndup, hwnddown;

    VERIFY(hwndup	 = GetDlgItem(GetParent(hgrid), IDC_MOVEUP));
	VERIFY(hwnddown  = GetDlgItem(GetParent(hgrid), IDC_MOVEDOWN));

	totrow = CustomGrid_GetRowCount(m_hdlggrid);

    //..Disabling the Down button when the focus is on the last row of the grid
	if(row == (totrow - 1))
	{
		EnableWindow(hwnddown, FALSE);

		if(totrow == 1)
			EnableWindow(hwndup, FALSE);
		else
			EnableWindow(hwndup, TRUE);
	}
    //..Disabling the Up button when the focus is on the first row of the grid
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

wyInt32
TabIndexes::InsertRow()
{
    wyInt32 row;

    //..Adding a new row in the grid
    row = CustomGrid_InsertRow(m_hgridindexes);      
    CustomGrid_SetButtonVis(m_hgridindexes, row, 1, wyTrue);
    CustomGrid_SetButtonText(m_hgridindexes, row, INDEXCOLUMNS, L"...");

    return row;
}

wyBool
TabIndexes::OnGVNBeginLabelEdit(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    wyUInt32		    row = wParam;
	wyUInt32		    col = lParam;
    wyUInt32            count = -1;
    wyString            celldata, indnamestr, indcolsstr, indexcomment;

    GetGridCellData(m_hgridindexes, row, INDEXNAME, indnamestr);
    GetGridCellData(m_hgridindexes, row, INDEXCOLUMNS, indcolsstr);

    GetGridCellData(m_hgridindexes, row, INDEXTYPE, celldata);
	
    GetGridCellData(m_hgridindexes, row, INDEXCOMMENT, indexcomment);

    //..Stoping user from setting the index-name of the PRIMARY index
    if(col == INDEXNAME && celldata.CompareI("primary") == 0)
        return wyFalse;

    GetGridCellData(m_hgridindexes, row, col, m_celldataprevval);

    count = CustomGrid_GetRowCount(m_hgridindexes);

    if(row == (count - 1))
    {
        //..Adding extra row when the row being edited is the last row of the grid
        InsertRow();
    }

    return wyTrue;
}

wyBool
TabIndexes::ScanEntireRow(wyUInt32  currentrow, wyInt32 currentcol, wyString& currentdata)
{
    IndexesStructWrapper   *cwrapobj = NULL;
    IndexColumn             *indcol1 = NULL, *indcol2 = NULL;
    List    *list1 = NULL, *list2 = NULL;
    IndexInfo                *indexes = NULL;
    wyUInt32                row, col, ncols;
    wyString                newtext(""), origtext("");
    wyBool                  changed = wyFalse;

    row = currentrow;
    col = currentcol;

    cwrapobj = (IndexesStructWrapper*) CustomGrid_GetRowLongData(m_hgridindexes, row);

    if(!cwrapobj)
        return wyFalse;
    
    //..Return if anyone of oldval and newval is not there
    if(!cwrapobj->m_oldval || !cwrapobj->m_newval)
    {
        return wyFalse;
    }

    //..Return if both oldval and newval are equal
    if(cwrapobj->m_oldval == cwrapobj->m_newval)
        return wyFalse;

    indexes = cwrapobj->m_oldval;

    ncols = CustomGrid_GetColumnCount(m_hgridindexes);

    for(int i=0; i< ncols; i++)
    {
        origtext.Clear();

        if(i == col)
            newtext.SetAs(currentdata);
        else
            GetGridCellData(m_hgridindexes, row, i, newtext);

        switch(i)
        {
        case INDEXNAME:
            origtext.SetAs(indexes->m_name);
            break;

        case INDEXCOLUMNS:
            {
                list1 = cwrapobj->m_oldval->m_listcolumns;
                list2 = cwrapobj->m_newval->m_listcolumns;

                if(list1)
                    indcol1 = (IndexColumn *)list1->GetFirst();

                if(list2)
                    indcol2 = (IndexColumn *)list2->GetFirst();

                while(indcol1 && indcol2)
                {
                    //..comparing the field-wrapper pointers. break the loop if they are not equal
                    if(indcol1->m_pcwrapobj != indcol2->m_pcwrapobj)
                        break;

                    indcol1 = (IndexColumn *) indcol1->m_next;
                    indcol2 = (IndexColumn *) indcol2->m_next;
                }

                //..Will be true if list1 and list2 don't contain equal index-columns
                if(indcol1 || indcol2)
                {
                    //..Setting flag
                    changed = wyTrue;
                    break;
                }
                else
                    continue;
            }
            break;

        case INDEXTYPE:
            origtext.SetAs(indexes->m_indextype);
            break;

		case INDEXCOMMENT:
			origtext.SetAs(indexes->m_indexcomment);
            break;

        default:
            origtext.SetAs("");
        }
        newtext.RTrim();

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

wyBool
TabIndexes::OnDlgGVNEndLabelEdit(WPARAM wParam, LPARAM lParam)
{
    wyUInt32		col = HIWORD(wParam);
    wyChar*         data = (wyChar*)lParam;
    wyString        len;

    if(col != 2)
        return wyTrue;

    //..if the length is not valid, set data(lParam) to empty string("")
    len.Sprintf("%d", atoi(data));
    if(strlen(data))
    {
        if(len.Compare("0") == 0)
            strcpy(((wyChar*)lParam), "");
        else
            strcpy(((wyChar*)lParam), len.GetString());
    }
    return wyTrue;
}

wyBool
TabIndexes::OnGVNEndLabelEdit(WPARAM wParam, LPARAM lParam)
{
    wyChar                  *data = (wyChar*)lParam;
    wyUInt32                row, col;
    wyString                currentdata;
    wyString                indexnamestr(""), indexcolsstr("");
    wyUInt32						no_of_columns;
    row = LOWORD(wParam);
    col = HIWORD(wParam);

	if(m_ismysql553)
		no_of_columns	=	3;
	else
		no_of_columns	=	2;
    //..Work-around to the cutsomgrid issue
    if(!(col >= 0 && col <= no_of_columns))
        return wyTrue;

    if(!(row >= 0 && row <= CustomGrid_GetRowCount(m_hgridindexes)))
        return wyTrue;
    
    if(data)
        currentdata.SetAs(data);

    currentdata.RTrim();

    //.. return if modified celldata is same as previous celldata
    if(m_celldataprevval.Compare(currentdata) == 0)
    {
        return wyTrue;
    }

    switch(col)
    {
    case INDEXNAME:
        if(!OnEndEditIndexName(wParam, lParam))
            return wyFalse;
        break;

    case INDEXCOLUMNS:
        OnEndEditIndexColumns(wParam, lParam);
        break;

    case INDEXTYPE:
        if(!OnEndEditIndexType(wParam, lParam))
            return wyFalse;
        break;

	case INDEXCOMMENT:
		if(!OnEndEditIndexComment(wParam, lParam))
            return wyFalse;
        break;
    }

    if(!m_ptabmgmt->m_tabinterfaceptr->m_dirtytab)
        m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);

    return wyTrue;
}

wyBool
TabIndexes::OnEndEditIndexType(WPARAM wParam, LPARAM lParam)
{
    wyChar                  *data = (wyChar*)lParam;
    wyUInt32                row, col;
    wyString                currentdata;
    IndexesStructWrapper    *cwrapobj = NULL;
    IndexesStructWrapper    *deletedpkwrap = NULL;
    IndexInfo               *iindexes = NULL;
    wyString                indexnamestr(""), indexcolsstr("");
    
    row = LOWORD(wParam);
    col = HIWORD(wParam);

    if(data)
        currentdata.SetAs(data);

    currentdata.RTrim();

    cwrapobj = (IndexesStructWrapper*) CustomGrid_GetRowLongData(m_hgridindexes, row);

    if(currentdata.GetLength())
    {
        if(currentdata.CompareI("PRIMARY") == 0)  //.. if user sets the PRIMARY, then we need to check whether, he has already defined or we have automatically set PRIMARY as index type
        {
            if(m_automatedindexrow != -1)
            {
                if(row != m_automatedindexrow)
                {
                    MessageBox(m_hgridindexes, _(L"Multiple primary keys cannot be defined."), pGlobals->m_appname.GetAsWideChar(), MB_DEFBUTTON2 | MB_ICONINFORMATION);
                    return wyFalse;
                }
            }
            else
            {
                m_automatedindexrow = row;
                
                //..Getting the dropped old(existing) PRIMARY index, if any
                deletedpkwrap = GetDroppedPKWrapper();

                //..If no wrapper is associated with the row
                if(!cwrapobj)
                {
                    //..If Existing PK is dropped, then bring it to the current row
                    if(deletedpkwrap)
                    {
                        cwrapobj = deletedpkwrap;
                        cwrapobj->m_newval = new IndexInfo();
                    }
                    //..Creating a new wrapper and insert it in the main list
                    else
                    {
                        iindexes = new IndexInfo();
                        cwrapobj = new IndexesStructWrapper(iindexes, wyTrue);
                        m_listwrapperstruct.Insert(cwrapobj);
                    }
                    //..Setting structure variable values
                    cwrapobj->m_newval->m_name.SetAs("PRIMARY");
                    cwrapobj->m_newval->m_colsstr.SetAs("");
                    cwrapobj->m_newval->m_indextype.SetAs("PRIMARY");
                    cwrapobj->m_newval->m_listcolumns = new List();
                    
                    //..Attaching cwrapobj to the gridrow
                    CustomGrid_SetRowLongData(m_hgridindexes, row, (LPARAM) cwrapobj);
                }
                //..Checking whether the attached index-wrapper is an existing one or a new one
                else if(cwrapobj->m_oldval)
                {
                    //..Checking whether the user has manually deleted all values of the index-row from the grid-row
                    if(cwrapobj->m_newval)
                    {
                        if(cwrapobj->m_oldval == cwrapobj->m_newval)
                        {
                            cwrapobj->m_newval = GetDuplicateIndexesStruct(cwrapobj->m_oldval);
                            cwrapobj->m_newval->m_colsstr.SetAs(cwrapobj->m_oldval->m_colsstr);
                        }
                    }
                    //..User has manually deleted all grid-row values (and thus has dropped index)
                    else
                    {
                        cwrapobj->m_newval = new IndexInfo();
                        cwrapobj->m_newval->m_listcolumns = new List();
                    }
                }
                //..true when the user is setting PRIMARY to the newly added index
                else
                {
                    if(deletedpkwrap)
                    {
                        deletedpkwrap->m_newval = GetDuplicateIndexesStruct(cwrapobj->m_newval);
                        m_listwrapperstruct.Remove(cwrapobj);
                        delete cwrapobj;
                        cwrapobj = deletedpkwrap;
                        
                        CustomGrid_SetRowLongData(m_hgridindexes, row, (LPARAM) cwrapobj);
                        CustomGrid_SetItemLongValue(m_hgridindexes, row, INDEXCOLUMNS, (LPARAM) ((cwrapobj && cwrapobj->m_newval) ? cwrapobj->m_newval->m_listcolumns : NULL));
                        
                    }
                }
                cwrapobj->m_newval->m_name.SetAs("PRIMARY");
                
                m_ptabmgmt->m_tabfields->OnPrimaryIndexChange();
                CustomGrid_SetText(m_hgridindexes, row, INDEXNAME, "PRIMARY");
            }
        }
        else if(row == m_automatedindexrow) //..if user has changed the automated PK index, then, we need to call the OnPrimaryIndexChange(); function (to reset the PK column in TabFields)
        {
            if(cwrapobj->m_oldval == cwrapobj->m_newval)
                cwrapobj->m_newval = GetDuplicateIndexesStruct(cwrapobj->m_oldval);

            m_automatedindexrow = -1;
            m_ptabmgmt->m_tabfields->OnPrimaryIndexChange();
            CustomGrid_SetText(m_hgridindexes, row, INDEXNAME, "");
            if(!cwrapobj->m_newval->m_listcolumns || !cwrapobj->m_newval->m_listcolumns->GetFirst())
            {
                delete cwrapobj->m_newval;
                cwrapobj->m_newval = NULL;

                if(!cwrapobj->m_oldval)
                {
                    m_listwrapperstruct.Remove(cwrapobj);
                    delete cwrapobj;
                    cwrapobj = NULL;
                }
            }
        }
        else if(cwrapobj && cwrapobj->m_oldval == cwrapobj->m_newval)
        {
            cwrapobj->m_newval = GetDuplicateIndexesStruct(cwrapobj->m_oldval);
            cwrapobj->m_newval->m_colsstr.SetAs(cwrapobj->m_oldval->m_colsstr);
        }
    }
    else
    {
        if(!cwrapobj)
            return wyTrue;

        if(row == m_automatedindexrow)
        {
            m_automatedindexrow = -1;
            m_ptabmgmt->m_tabfields->OnPrimaryIndexChange();
            
            if(cwrapobj->m_oldval == cwrapobj->m_newval)
            {
                cwrapobj->m_newval = GetDuplicateIndexesStruct(cwrapobj->m_oldval);
                cwrapobj->m_newval->m_colsstr.SetAs(cwrapobj->m_oldval->m_colsstr);
            }
            cwrapobj->m_newval->m_name.Clear();
            CustomGrid_SetText(m_hgridindexes, row, INDEXNAME, "");

            if(cwrapobj->m_newval->m_colsstr.GetLength() == 0)
            {
                if(cwrapobj->m_oldval)
                {
                    delete cwrapobj->m_newval;
                    cwrapobj->m_newval = NULL;
                }
                else
                {
                    m_listwrapperstruct.Remove(cwrapobj);
                    delete cwrapobj->m_newval;
                    cwrapobj = NULL;
                }
            }
        }
        else
        {
            if(cwrapobj->m_newval == cwrapobj->m_oldval)
            {
                cwrapobj->m_newval = GetDuplicateIndexesStruct(cwrapobj->m_oldval);
            }
        }
    }

    CustomGrid_SetRowLongData(m_hgridindexes, row, (LPARAM) cwrapobj);

    if(cwrapobj && cwrapobj->m_newval)
        SetValueToStructure(row, col, data);

    ScanEntireRow(row, col, currentdata);

    CustomGrid_SetRowLongData(m_hgridindexes, row, (LPARAM) cwrapobj);
    CustomGrid_SetItemLongValue(m_hgridindexes, row, INDEXCOLUMNS, (LPARAM) ((cwrapobj && cwrapobj->m_newval) ? cwrapobj->m_newval->m_listcolumns : NULL));
    return wyTrue;
}

wyBool
TabIndexes::OnEndEditIndexColumns(WPARAM wParam, LPARAM lParam)
{
    wyChar                  *data = (wyChar*)lParam;
    wyUInt32                row, col;
    wyString                currentdata("");
    IndexesStructWrapper   *cwrapobj = NULL;
    wyString                indexnamestr("");
    List                    *listindcols = NULL;
    IndexColumn             *indcol = NULL, *tmpindcol = NULL;

    row = LOWORD(wParam);
    col = HIWORD(wParam);

    GetGridCellData(m_hgridindexes, row, INDEXNAME, indexnamestr);

    if(data)
        currentdata.SetAs(data);
    currentdata.RTrim();

    if(currentdata.GetLength())     //..Index-Columns column can never have any data on endedit.
        return wyTrue;

    cwrapobj = (IndexesStructWrapper*) CustomGrid_GetRowLongData(m_hgridindexes, row);

    if(!cwrapobj)
        return wyTrue;

    //..If index-name is not present, then we shall remove it if it's a newly added, (for existing index, we shall set m_newval to NULL)
    if(!indexnamestr.GetLength())
    {
        //..This will remove the index-columns entries from fieldswrapper
        listindcols = (List*)CustomGrid_GetItemLongValue(m_hgridindexes, row, col);
        if(listindcols)
            indcol = (IndexColumn*)listindcols->GetFirst();

        while(indcol)
        {
            tmpindcol = indcol;
            RemoveIndexWrappersFromFieldsWrappers(indcol, cwrapobj);
            indcol = (IndexColumn*)listindcols->Remove(indcol);
            delete tmpindcol;
        }

        //..For existing index (Alter table), we shall delete the m_newval;
        if(cwrapobj->m_oldval)
        {
            if(cwrapobj->m_oldval != cwrapobj->m_newval)    //..true, if existing index is already modified
                delete cwrapobj->m_newval;
            cwrapobj->m_newval = NULL;
        }
        //..For newly added index, we shall remove the wrapper from the m_listwrapper and delete it.
        else if(cwrapobj->m_newval)
        {
            m_listwrapperstruct.Remove(cwrapobj);
            delete cwrapobj;
            cwrapobj = NULL;
        }
        CustomGrid_SetRowLongData(m_hgridindexes, row, (LPARAM) cwrapobj);
        CustomGrid_SetItemLongValue(m_hgridindexes, row, INDEXCOLUMNS, (LPARAM)NULL);
    }
    else
    {
        //..Creating m_newval same as m_oldval, if the index is not modified yet
        if(cwrapobj->m_oldval == cwrapobj->m_newval)
        {
            cwrapobj->m_newval = GetDuplicateIndexesStruct(cwrapobj->m_oldval);
        }

        //..Initializing the variables
        listindcols = cwrapobj->m_newval->m_listcolumns;
        indcol = NULL;
        tmpindcol = NULL;
        
        if(listindcols)
            indcol = (IndexColumn *)listindcols->GetFirst();

        //..Remove index-column entries from the fieldwrapper and deleting them
        while(indcol)
        {
            RemoveIndexWrappersFromFieldsWrappers(indcol, cwrapobj);
            tmpindcol = indcol;
            indcol = (IndexColumn*) listindcols->Remove(indcol);
            delete tmpindcol;
        }
        if(row == m_automatedindexrow)
        {
            m_automatedindexrow = -1;
            m_ptabmgmt->m_tabfields->OnPrimaryIndexChange();
            m_automatedindexrow = row;
        }
        cwrapobj->m_newval->m_colsstr.SetAs(currentdata);
        cwrapobj->m_newval->m_listcolumns = listindcols;
        CustomGrid_SetItemLongValue(m_hgridindexes, row, INDEXCOLUMNS, (LPARAM)listindcols);
    }

    return wyTrue;
}

wyBool
TabIndexes::OnEndEditIndexName(WPARAM wParam, LPARAM lParam)
{
    wyChar                  *data = (wyChar*)lParam;
    wyUInt32                row, col;
    wyString                currentdata("");
    IndexesStructWrapper   *cwrapobj = NULL;
    IndexInfo                *iindexes = NULL;
    wyString                indexcolsstr("");

    row = LOWORD(wParam);
    col = HIWORD(wParam);
    GetGridCellData(m_hgridindexes, row, INDEXCOLUMNS, indexcolsstr);

    if(data)
        currentdata.SetAs(data);

    currentdata.RTrim();

    cwrapobj = (IndexesStructWrapper*) CustomGrid_GetRowLongData(m_hgridindexes, row);

	//..If no wrapper is attached
    if(!cwrapobj)
    {
        iindexes = new IndexInfo();
        iindexes->m_colsstr.Clear();
        iindexes->m_indextype.Clear();
        iindexes->m_listcolumns = NULL;
        iindexes->m_name.Clear();
		if(m_ismysql553)
			iindexes->m_indexcomment.Clear();
        cwrapobj = new IndexesStructWrapper(iindexes, wyTrue);
        m_listwrapperstruct.Insert(cwrapobj);
    }
    //..If any wrapper is attached
    else
    {
        //..If the index-name is erased
        if(!currentdata.GetLength())
        {
            //..If user has deleted index-name and index-columns values from existing index, this condition will be false
            //      we need to proceed only when user has not cleared Existing index
            if(cwrapobj->m_newval)
            {
                //..If Index-Columns are not selected, then set m_newval to NULL.
                //..    if the index is new, then remove it from the m_listwrapper.
                if(!indexcolsstr.GetLength())
                {
                    //..existing index (Alter table)
                    if(cwrapobj->m_oldval)
                    {
                        if(cwrapobj->m_oldval != cwrapobj->m_newval)    //..true, if existing index is already modified
                            delete cwrapobj->m_newval;
                        cwrapobj->m_newval = NULL;
                    }
                    //..newly added index
                    else if(cwrapobj->m_newval)
                    {
                        m_listwrapperstruct.Remove(cwrapobj);
                        delete cwrapobj;
                        cwrapobj = NULL;
                    }
                }
                //.. If index-columns are selected,
                else
                {
                    if(cwrapobj->m_newval == cwrapobj->m_oldval)
                    {
                        cwrapobj->m_newval = GetDuplicateIndexesStruct(cwrapobj->m_oldval);
                        CustomGrid_SetItemLongValue(m_hgridindexes, row, INDEXCOLUMNS, (LPARAM)cwrapobj->m_newval->m_listcolumns);
                    }
                }
            }
        }
        //.. if something is there as index-name
        else
        {
            //..If user has not deleted all index-values from grid row
            if(cwrapobj->m_newval)
            {
                //..If existing index is not yet modified
                if(cwrapobj->m_newval == cwrapobj->m_oldval)
                {
                    cwrapobj->m_newval = GetDuplicateIndexesStruct(cwrapobj->m_oldval);
                    CustomGrid_SetItemLongValue(m_hgridindexes, row, INDEXCOLUMNS, (LPARAM)cwrapobj->m_newval->m_listcolumns);
                }
            }
            //.. If for existing index, user has deleted all values from the grid row
            else
            {
                cwrapobj->m_newval = new IndexInfo;
                cwrapobj->m_newval->m_name.Clear();
                GetGridCellData(m_hgridindexes, row, INDEXTYPE, cwrapobj->m_newval->m_indextype);
                cwrapobj->m_newval->m_colsstr.Clear();
                cwrapobj->m_newval->m_listcolumns = new List;
				cwrapobj->m_newval->m_indexcomment.Clear();
            }
        }
    }
    CustomGrid_SetRowLongData(m_hgridindexes, row, (LPARAM) cwrapobj);
    CustomGrid_SetItemLongValue(m_hgridindexes, row, col, (LPARAM) (cwrapobj && cwrapobj->m_newval ? cwrapobj->m_newval->m_listcolumns : NULL));
    
    //CustomGrid_SetItemLongValue(m_hgridindexes, row, INDEXCOLUMNS, (LPARAM)((cwrapobj && cwrapobj->m_newval) ? cwrapobj->m_newval->m_listcolumns : NULL));
    if(cwrapobj && cwrapobj->m_newval)
        cwrapobj->m_newval->m_name.SetAs(currentdata);

    ScanEntireRow(row, col, currentdata);

    return wyTrue;
}


wyBool
	TabIndexes::OnEndEditIndexComment(WPARAM wParam, LPARAM lParam)
{
    wyChar                  *data = (wyChar*)lParam;
    wyUInt32                row, col;
    wyString                currentdata("");
    IndexesStructWrapper   *cwrapobj = NULL;
    IndexInfo                *iindexes = NULL;
    wyString                indexcommstr("");
	wyString                indexcolsstr("");

    row = LOWORD(wParam);
    col = HIWORD(wParam);
    GetGridCellData(m_hgridindexes, row, INDEXCOLUMNS, indexcolsstr);
	GetGridCellData(m_hgridindexes, row, INDEXCOMMENT, indexcommstr);

	cwrapobj = (IndexesStructWrapper*) CustomGrid_GetRowLongData(m_hgridindexes, row);

    if(data)
        currentdata.SetAs(data);

    currentdata.RTrim();
	cwrapobj = (IndexesStructWrapper*) CustomGrid_GetRowLongData(m_hgridindexes, row);

    //..If no wrapper is attached
    if(!cwrapobj)
    {
        iindexes = new IndexInfo();
        iindexes->m_colsstr.Clear();
        iindexes->m_indextype.Clear();
        iindexes->m_listcolumns = NULL;
        iindexes->m_name.Clear();
		if(m_ismysql553)
			iindexes->m_indexcomment.Clear();
        cwrapobj = new IndexesStructWrapper(iindexes, wyTrue);
        m_listwrapperstruct.Insert(cwrapobj);
    }
    else
    {
       //..If the index-name is erased
        if(!currentdata.GetLength())
        {
            //..If user has deleted index-name and index-columns values from existing index, this condition will be false
            //      we need to proceed only when user has not cleared Existing index
            if(cwrapobj->m_newval)
            {
                //..If Index-Columns are not selected, then set m_newval to NULL.
                //..    if the index is new, then remove it from the m_listwrapper.
                if(!indexcolsstr.GetLength())
                {
                    //..existing index (Alter table)
                    if(cwrapobj->m_oldval)
                    {
                        if(cwrapobj->m_oldval != cwrapobj->m_newval)    //..true, if existing index is already modified
                            delete cwrapobj->m_newval;
                        cwrapobj->m_newval = NULL;
                    }
                    //..newly added index
                    else if(cwrapobj->m_newval)
                    {
                        m_listwrapperstruct.Remove(cwrapobj);
                        delete cwrapobj;
                        cwrapobj = NULL;
                    }
                }
                //.. If index-columns are selected,
                else
                {
                    if(cwrapobj->m_newval == cwrapobj->m_oldval)
                    {
                        cwrapobj->m_newval = GetDuplicateIndexesStruct(cwrapobj->m_oldval);
                        CustomGrid_SetItemLongValue(m_hgridindexes, row, INDEXCOLUMNS, (LPARAM)cwrapobj->m_newval->m_listcolumns);
                    }
                }
            }
        }
        //.. if something is there as index-name
        else
        {
            //..If user has not deleted all index-values from grid row
            if(cwrapobj->m_newval)
            {
                //..If existing index is not yet modified
                if(cwrapobj->m_newval == cwrapobj->m_oldval)
                {
                    cwrapobj->m_newval = GetDuplicateIndexesStruct(cwrapobj->m_oldval);
                    CustomGrid_SetItemLongValue(m_hgridindexes, row, INDEXCOLUMNS, (LPARAM)cwrapobj->m_newval->m_listcolumns);
                }
            }
        }
    }
    CustomGrid_SetRowLongData(m_hgridindexes, row, (LPARAM) cwrapobj);
    CustomGrid_SetItemLongValue(m_hgridindexes, row, col, (LPARAM) (cwrapobj && cwrapobj->m_newval ? cwrapobj->m_newval->m_listcolumns : NULL));
    
    if(cwrapobj && cwrapobj->m_newval)
		cwrapobj->m_newval->m_indexcomment.SetAs(currentdata);

    ScanEntireRow(row, col, currentdata);

    return wyTrue;
}

void
TabIndexes::RemoveIndexWrappersFromFieldsWrappers(IndexColumn *indcol, IndexesStructWrapper* indexwrap)
{
    IndexedBy          *indby = NULL;
    if(!indcol)
        return;

    indby = (IndexedBy *)indcol->m_pcwrapobj->m_listindexesworkingcopy.GetFirst();

    //..Removing index-entry from fieldswrapper
    while(indby)
    {
        if(indby->m_pindexwrap == indexwrap)
        {
            indcol->m_pcwrapobj->m_listindexesworkingcopy.Remove(indby);
            delete indby;
            break;
        }
        indby = (IndexedBy *)indby->m_next;
    }
}

IndexInfo*
TabIndexes::GetDuplicateIndexesStruct(IndexInfo* duplicateof)
{
    IndexInfo* indexes = NULL;
    IndexColumn *icols = NULL, *newicol = NULL;

    if(duplicateof)
    {
        indexes = new IndexInfo();
        indexes->m_listcolumns = new List();

        //..Setting index_name and index_type
        indexes->m_name.SetAs(duplicateof->m_name);
        indexes->m_indextype.SetAs(duplicateof->m_indextype);
		if(m_ismysql553)
			indexes->m_indexcomment.SetAs(duplicateof->m_indexcomment);
        //..Setting list_columns backup_copy and working_copy
        icols = (IndexColumn*) duplicateof->m_listcolumns->GetFirst();
        while(icols)
        {
            newicol = new IndexColumn(icols->m_pcwrapobj);
            newicol->m_lenth = icols->m_lenth;
            indexes->m_listcolumns->Insert(newicol);
            icols = (IndexColumn*)icols->m_next;
        }
    }
    return indexes;
}

void
TabIndexes::SetValueToStructure(wyUInt32 row, wyUInt32 col, wyChar* data)
{
    IndexesStructWrapper *cwrap = NULL;

    cwrap = (IndexesStructWrapper*) CustomGrid_GetRowLongData(m_hgridindexes, row);

    switch(col)
    {
    case INDEXNAME:
        cwrap->m_newval->m_name.SetAs(data);
        break;

    case INDEXCOLUMNS:
        break;

    case INDEXTYPE:
        cwrap->m_newval->m_indextype.SetAs(data);
        break;

	case INDEXCOMMENT:
		cwrap->m_newval->m_indexcomment.SetAs(data);
        break;
    }
}

wyUInt32
TabIndexes::GetGridCellData(HWND hwndgrid, wyUInt32 row, wyUInt32 col, wyString &celldata)
{
    wyString    tempstr("");
    wyWChar     *data;
    wyUInt32    celldatalen = 0;

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
TabIndexes::FillColumnsGrid(HWND hwnd)
{
    FieldStructWrapper      *cfieldswrapobj = NULL;
    IndexColumn             *indcols = NULL, *tmpindcols = NULL;
    List                    *list = NULL;
    wyString                tempstr("");
    wyUInt32                row = CustomGrid_GetCurSelRow(m_hgridindexes);
    wyUInt32                col = CustomGrid_GetCurSelCol(m_hgridindexes);
    wyUInt32                newrow = NULL;

    list = (List*)CustomGrid_GetItemLongValue(m_hgridindexes, row, col);

    //..Adding columns on the grid that the 'list' contains
    if(list)
    {
        indcols = (IndexColumn*) list->GetFirst();
        tmpindcols = indcols;

        while(tmpindcols)
        {
            if(tmpindcols->m_pcwrapobj->m_newval)
            {
				
                newrow = CustomGrid_InsertRow(m_hdlggrid);
                CustomGrid_SetRowCheckState(m_hdlggrid, newrow, wyTrue);
                CustomGrid_SetText(m_hdlggrid, newrow, 0, (wyChar*)tmpindcols->m_pcwrapobj->m_newval->m_name.GetString());
                CustomGrid_SetText(m_hdlggrid, newrow, 1, (wyChar*)tmpindcols->m_pcwrapobj->m_newval->m_datatype.GetString());

                if(tmpindcols->m_lenth != -1)
                    tempstr.Sprintf("%d", tmpindcols->m_lenth);
                else
                    tempstr.Clear();
                CustomGrid_SetText(m_hdlggrid, newrow, 2, (wyChar*)tempstr.GetString());

                CustomGrid_SetRowLongData(m_hdlggrid, newrow, (LPARAM) tmpindcols->m_pcwrapobj);
            }
            tmpindcols = (IndexColumn*)tmpindcols->m_next;
        }
    }

    cfieldswrapobj = (FieldStructWrapper*)m_ptabmgmt->m_tabfields->m_listwrapperstruct.GetFirst();
    tmpindcols = indcols;

    //..Adding other columns(from Columns-tab) on the grid that the 'list' doesn't contain
    while(cfieldswrapobj)
    {
        if(cfieldswrapobj->m_newval)
        {
            tmpindcols = indcols;
			
            while(tmpindcols)
            {
                if(tmpindcols->m_pcwrapobj == cfieldswrapobj)
                    break;
                tmpindcols = (IndexColumn*)tmpindcols->m_next;
            }
            if(!tmpindcols)
            {
				//add only thoes columns for index which are not virtual
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
TabIndexes::OnDlgWMSize(HWND hwnd)
{
    RECT        rect, rc2;
    wyUInt32    width, height;
    wyUInt32    row = CustomGrid_GetCurSelRow(this->m_hgridindexes);
    wyUInt32    col = CustomGrid_GetCurSelCol(this->m_hgridindexes);
    
    CustomGrid_GetSubItemRect(this->m_hgridindexes,row, col, &rect);
    MapWindowPoints(this->m_hgridindexes, NULL, (LPPOINT)&rect, 2);

    GetWindowRect(hwnd, &rc2);

    width   = rc2.right - rc2.left;
    height  = rc2.bottom - rc2.top;

    MoveWindow(hwnd, rect.left, rect.bottom + 4, width, height, TRUE);

    return;
}

void
TabIndexes::OnListViewItemChanged(HWND hwnd, LPARAM lparam)
{
    wyUInt32        count;
    LPNMLISTVIEW    lpnmlist = (LPNMLISTVIEW)lparam;
    wyUInt32        item = lpnmlist->iItem;
    HWND            hlist = GetDlgItem(hwnd, IDC_LISTCOLUMNS);
    
    count = ListView_GetItemCount(hlist);

    //we need the changes caused only because of checking/unchecing the item
    if((lpnmlist->uNewState & LVIS_STATEIMAGEMASK) == 0)
    {
        EnableWindow(GetDlgItem(hwnd,IDC_BTNUP), (item == 0) ? FALSE : TRUE);
        EnableWindow(GetDlgItem(hwnd,IDC_BTNDOWN), (item == count-1) ? FALSE : TRUE);
    }
}

void
TabIndexes::HandleCheckboxClick(HWND hwnd, LPARAM lparam, WPARAM wparam, wyInt32 &lastclickindex)
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
            if(lastclickindex == -1 || (CustomGrid_GetRowCount(hwnd) == 0))
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

void
TabIndexes::OnButtonUpDown(HWND hwnd, wyBool up)
{
    wyInt32                 selrow = CustomGrid_GetCurSelRow(m_hdlggrid);
    wyInt32                 selcol = CustomGrid_GetCurSelCol(m_hdlggrid);
    wyUInt32                otherrow = -1;
    wyUInt32                count = CustomGrid_GetRowCount(m_hdlggrid);
    void                    *pvoid1 = NULL, *pvoid2 = NULL;
    wyBool                  checkval1, checkval2;
    wyString                colname1, datatype1, length1;
    wyString                colname2, datatype2, length2;

    if(selrow == -1)
        return;
    
    if(up)
    {
        otherrow = selrow - 1;
    }
    else
    {
        otherrow = selrow + 1;
    }

    if(otherrow == -1)
        return;

    //..Fetching Selected row values
    if(CustomGrid_GetRowCheckState(m_hdlggrid, selrow))
        checkval1 = wyTrue;
    else
        checkval1 = wyFalse;

    GetGridCellData(m_hdlggrid, selrow, 0, colname1);
    GetGridCellData(m_hdlggrid, selrow, 1, datatype1);
    GetGridCellData(m_hdlggrid, selrow, 2, length1);
    pvoid1 = (void*) CustomGrid_GetRowLongData(m_hdlggrid, selrow);

    
    //..Fetching other row values
    if(CustomGrid_GetRowCheckState(m_hdlggrid, otherrow))
        checkval2 = wyTrue;
    else
        checkval2 = wyFalse;

    GetGridCellData(m_hdlggrid, otherrow, 0, colname2);
    GetGridCellData(m_hdlggrid, otherrow, 1, datatype2);
    GetGridCellData(m_hdlggrid, otherrow, 2, length2);
    pvoid2 = (void*) CustomGrid_GetRowLongData(m_hdlggrid, otherrow);
    
    //..Setting 1st row values to the 2nd row
    CustomGrid_SetRowCheckState(m_hdlggrid, otherrow, checkval1);
    CustomGrid_SetText(m_hdlggrid, otherrow, 0, (wyChar*)colname1.GetString());
    CustomGrid_SetText(m_hdlggrid, otherrow, 1, (wyChar*)datatype1.GetString());
    CustomGrid_SetText(m_hdlggrid, otherrow, 2, (wyChar*)length1.GetString());
    CustomGrid_SetRowLongData(m_hdlggrid, otherrow, (LPARAM) pvoid1);
    CustomGrid_EnsureVisible(m_hdlggrid, otherrow, selcol);
    CustomGrid_SetCurSelection(m_hdlggrid, otherrow, selcol, wyTrue);
    
    //..Setting 2nd row values to the 1st row
    CustomGrid_SetRowCheckState(m_hdlggrid, selrow, checkval2);
    CustomGrid_SetText(m_hdlggrid, selrow, 0, (wyChar*)colname2.GetString());
    CustomGrid_SetText(m_hdlggrid, selrow, 1, (wyChar*)datatype2.GetString());
    CustomGrid_SetText(m_hdlggrid, selrow, 2, (wyChar*)length2.GetString());
    CustomGrid_SetRowLongData(m_hdlggrid, selrow, (LPARAM) pvoid2);
    
    if(otherrow == 0)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_MOVEUP), FALSE);
        SetFocus(GetDlgItem(hwnd, IDC_MOVEDOWN));
    }
    else
        EnableWindow(GetDlgItem(hwnd, IDC_MOVEUP), TRUE);

    if(otherrow == count-1)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_MOVEDOWN), FALSE);
        SetFocus(GetDlgItem(hwnd, IDC_MOVEUP));
    }
    else
        EnableWindow(GetDlgItem(hwnd, IDC_MOVEDOWN), TRUE);
}

wyBool
TabIndexes::OnWMNotify(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    LPNMHDR pnmh = (LPNMHDR)lparam;

    switch(pnmh->code)
    {
        case LVN_ITEMCHANGED:
            OnListViewItemChanged(hwnd, lparam);
            break;

        default:
            return wyFalse;
    }

    return wyTrue;
}

void
TabIndexes::OnWMInitDialog(HWND hwnd)
{
    RECT rect;
	wyInt32 row =0, col = 0;
	RECT temp = m_dlgrect;
    HWND hwndgripper;
    HICON   hicon;

    hicon = LoadIcon(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_COLUMN));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon);
    DestroyIcon(hicon);

	m_hdlggrid = CreateCustomGridEx(hwnd, 0, 0, 0, 0, (GVWNDPROC)DlgGVWndProc, GV_EX_ROWCHECKBOX, (LPARAM)this);

    if(!m_hdlggrid)
        return;

    InitDlgGrid();
    PostMessage(hwnd, WM_SIZE, 0, 0);

    //..Filling grid data
    FillColumnsGrid(hwnd);
    
    if(CustomGrid_GetRowCount(m_hdlggrid) != 0)
        PostMessage(hwnd, UM_SETINITFOCUS, (WPARAM)m_hdlggrid, NULL);
    
	GetWindowRect(hwnd, &m_wndrect);
	GetClientRect(hwnd, &m_dlgrect);
	
	//set the position and size of the static control that is used to draw the size gripper
    hwndgripper = GetDlgItem(hwnd, IDC_INDEXGRIP);
    temp.left = temp.right - GetSystemMetrics(SM_CXHSCROLL);
	temp.top = temp.bottom - GetSystemMetrics(SM_CYVSCROLL);
    SetWindowPos(hwndgripper, NULL, temp.left, temp.top, temp.right - temp.left, temp.bottom - temp.top, SWP_NOZORDER);
    
    SetWindowText(hwndgripper, L"");

	row = CustomGrid_GetCurSelRow(m_hgridindexes);
	col = CustomGrid_GetCurSelCol(m_hgridindexes);
	CustomGrid_GetSubItemRect(m_hgridindexes, row, col, &rect);
    MapWindowRect(m_hgridindexes, NULL, &rect);

	GetCtrlRects(hwnd);

	PositionWindow(&rect, hwnd);
	
	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd);
}

void
TabIndexes::FillInitValues()
{
    IndexesStructWrapper   *cwrapobj = NULL;
    IndexColumn           *indcols = NULL;
    wyUInt32                row = -1;

    cwrapobj = (IndexesStructWrapper *)m_listwrapperstruct.GetFirst();

    while(cwrapobj)
    {
        row = InsertRow();

        indcols = (IndexColumn*)cwrapobj->m_oldval->m_listcolumns->GetFirst();

        while(indcols)
        {
            indcols->m_pcwrapobj->m_listindexesworkingcopy.Insert(new IndexedBy(cwrapobj));
            indcols = (IndexColumn*)indcols->m_next;
        }
        
        //..Setting text & Long Values
        CustomGrid_SetText(m_hgridindexes, row, INDEXNAME, cwrapobj->m_oldval->m_name.GetString());
        CustomGrid_SetText(m_hgridindexes, row, INDEXCOLUMNS, cwrapobj->m_oldval->m_colsstr.GetString());

        CustomGrid_SetItemLongValue(m_hgridindexes, row, INDEXCOLUMNS, (LPARAM)cwrapobj->m_oldval->m_listcolumns);

        CustomGrid_SetText(m_hgridindexes, row, INDEXTYPE, cwrapobj->m_oldval->m_indextype.GetString());

		if(m_ismysql553)
		CustomGrid_SetText(m_hgridindexes, row, INDEXCOMMENT,cwrapobj->m_oldval->m_indexcomment.GetString());

        CustomGrid_SetRowLongData(m_hgridindexes, row, (LPARAM) cwrapobj);

		if(cwrapobj->m_oldval->m_indextype.CompareI("PRIMARY KEY") == 0 || cwrapobj->m_oldval->m_name.CompareI("PRIMARY") == 0)
        {
			m_automatedindexrow = row;
            m_ptabmgmt->m_tabfields->OnPrimaryIndexChange();
        }

        cwrapobj = (IndexesStructWrapper *) cwrapobj->m_next;
    }
    CustomGrid_SetCurSelection(m_hgridindexes, 0, 0, wyTrue);
}

void
TabIndexes::ResizeColumnsDialog(HWND hwnd, LPARAM lParam)
{
	PositionCtrls(hwnd);
}

void
TabIndexes::Refresh()
{
	IndexesStructWrapper   *cwrapobj = NULL;
	wyUInt32    nrows = 0;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

	cwrapobj = (IndexesStructWrapper *)m_listwrapperstruct.GetFirst();
	while (cwrapobj)
	{
		if (cwrapobj->m_oldval)
			Refresh(cwrapobj->m_oldval);
		if (cwrapobj->m_newval && cwrapobj->m_newval != cwrapobj->m_oldval)
			Refresh(cwrapobj->m_newval);

		cwrapobj = (IndexesStructWrapper *)cwrapobj->m_next;
	}

	nrows = CustomGrid_GetRowCount(m_hgridindexes);
	cwrapobj = NULL;

	for (int row = 0; row<nrows; row++)
	{
		cwrapobj = (IndexesStructWrapper *)CustomGrid_GetRowLongData(m_hgridindexes, row);

		if (!cwrapobj)
			continue;

		if (!cwrapobj->m_newval)
			continue;

		CustomGrid_SetText(m_hgridindexes, row, INDEXCOLUMNS, cwrapobj->m_newval->m_colsstr.GetString());
	}
}

void
TabIndexes::Refresh(IndexInfo *indexInfo)
{
	IndexColumn *indcol = NULL;
	wyString    indcolsstr("");
	wyString    tmpstr;

	if (NULL == indexInfo->m_listcolumns)
		return;

	indcol = (IndexColumn*)indexInfo->m_listcolumns->GetFirst();

	while (indcol)
	{
		tmpstr.SetAs(indcol->m_pcwrapobj->m_newval->m_name);
		tmpstr.FindAndReplace("`", "``");

		indcolsstr.AddSprintf("%s%s%s", m_backtick, tmpstr.GetString(), m_backtick);
		if (indcol->m_lenth != -1)
			indcolsstr.AddSprintf("(%d)", indcol->m_lenth);

		indcolsstr.Add(", ");

		indcol = (IndexColumn*)indcol->m_next;
	}

	indcolsstr.RTrim();
	if (indcolsstr.GetLength())
	{
		if (indcolsstr.GetCharAt(indcolsstr.GetLength() - 1) == ',')
			indcolsstr.Strip(1);
	}

	indexInfo->m_colsstr.SetAs(indcolsstr);
}

INT_PTR CALLBACK
TabIndexes::ColDlgWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    TabIndexes  *ptabind = (TabIndexes*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch(message)
    {
    case WM_INITDIALOG:
        ptabind = (TabIndexes*) lparam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ptabind);
        LocalizeWindow(hwnd);
        ptabind->OnWMInitDialog(hwnd);
        break;

    case WM_COMMAND:
        ptabind->OnColDlgWMCommand(hwnd, wparam, lparam);
        break;

    case UM_SETINITFOCUS:
        SetFocus(HWND(wparam));
        break;

    case WM_NOTIFY:
        ptabind->OnWMNotify(hwnd, wparam, lparam);
        break;

	case WM_SIZE:
        ptabind->ResizeColumnsDialog(hwnd, lparam);
        break;

    case WM_GETMINMAXINFO:
		ptabind->OnWMSizeInfo(lparam, hwnd);
		break;

	case WM_PAINT:
		ptabind->OnWMPaint(hwnd);
        return 1;
		break;

	case WM_ERASEBKGND:
		//blocked for double buffering
		return 1;

    case WM_HELP:
        {
            if(ptabind->m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
                ShowHelp("http://sqlyogkb.webyog.com/article/92-alter-index");
            else
                   ShowHelp("http://sqlyogkb.webyog.com/article/91-create-index");
        }
        return 1;

    case WM_DESTROY:
        {
			//delete the ctrl list
			DlgControl* pdlgctrl;
			while((pdlgctrl = (DlgControl*)ptabind->m_controllist.GetFirst()))
			{
				ptabind->m_controllist.Remove(pdlgctrl);
				delete pdlgctrl;
			}
            ptabind->m_hdlggrid = NULL;
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
TabIndexes::ShowColumnsDialog()
{
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_INDEXCOLUMNS), m_hgridindexes, ColDlgWndProc,(LPARAM)this);
}

void
TabIndexes::OnGVNButtonClick()
{
    wyInt32 row;
    wyInt32 col;

    row = CustomGrid_GetCurSelRow(m_hgridindexes);
    col = CustomGrid_GetCurSelCol(m_hgridindexes);

    if(col == INDEXCOLUMNS)
    {
        CustomGrid_ApplyChanges(m_hgridindexes, wyTrue);
        ShowColumnsDialog();
    }
}

wyBool
TabIndexes::ProcessInsert()
{
    wyInt32 newrowid = -1;
    ApplyCancelGridChanges();
    
    newrowid = InsertRow();

    if(newrowid != -1)
        CustomGrid_SetCurSelection(m_hgridindexes, newrowid, INDEXCOLUMNS);

    return wyTrue;
}

wyBool	
TabIndexes::ProcessDelete()
{
	wyInt32		selrow;

	selrow = CustomGrid_GetCurSelRow(m_hgridindexes);
    
	if(selrow == -1)
		return wyFalse;

    ApplyCancelGridChanges();
	
    if(!DropSelectedIndexes())
    {
        //..Processing Drop of the index for the current row, if user has not checked any row
        DropIndex(selrow);
    }
    
    if(CustomGrid_GetRowCount(m_hgridindexes) == 0)
    {
        selrow = InsertRow();
        CustomGrid_SetCurSelection(m_hgridindexes, selrow, INDEXCOLUMNS);
    }

    return wyTrue;
}

wyBool
TabIndexes::DropSelectedIndexes()
{
    wyInt32     count, row, ret = IDNO;
    wyBool      checkrowfound = wyFalse;
    IndexColumn *iindcols = NULL;
    IndexedBy   *indexedby = NULL;
    IndexesStructWrapper *pwrapobj = NULL;

    count = CustomGrid_GetRowCount(m_hgridindexes);
    
    for(row=0; row<count; row++)
    {
        if(CustomGrid_GetRowCheckState(m_hgridindexes, row))
        {
            pwrapobj = (IndexesStructWrapper *) CustomGrid_GetRowLongData(m_hgridindexes, row);
            checkrowfound = wyTrue;

            //..Deleting empty row without confirmation
            if(!pwrapobj || !pwrapobj->m_newval)
            {
                CustomGrid_DeleteRow(m_hgridindexes, row, wyTrue);
                row--;
                count--;
                if((m_automatedindexrow != -1) && row <= m_automatedindexrow)
                    m_automatedindexrow--;
                continue;
            }
            else if(ret == IDNO)
            {
                ret = MessageBox(m_hgridindexes, _(L"Do you want to drop the selected index(es)?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
                if(ret != IDYES)
                    return wyTrue;
            }
            if(ret == IDYES)
            {
                if(row == m_automatedindexrow)
                {
                    //..Resetting the row id for the pk and unchecking pk column in the Columns-tab
                    m_automatedindexrow = -1;
                    m_ptabmgmt->m_tabfields->OnPrimaryIndexChange();
                }
                if(pwrapobj->m_newval && pwrapobj->m_newval->m_listcolumns)
                {
                    iindcols = (IndexColumn*)pwrapobj->m_newval->m_listcolumns->GetFirst();

                    //..Deleting index entries from the fields wrapper and deleting index-columns
                    while(iindcols)
                    {
                        indexedby = (IndexedBy*) iindcols->m_pcwrapobj->m_listindexesworkingcopy.GetFirst();

                        while(indexedby)
                        {
                            if(indexedby->m_pindexwrap == pwrapobj)
                            {
                                iindcols->m_pcwrapobj->m_listindexesworkingcopy.Remove(indexedby);
                                delete indexedby;
                                break;
                            }
                            indexedby = (IndexedBy*) indexedby->m_next;
                        }
                        iindcols = (IndexColumn*) iindcols->m_next;
                    }
                }

                ChangeListOnDelete(pwrapobj);

                CustomGrid_DeleteRow(m_hgridindexes, row, wyTrue);

                //..reduce m_automatedindexrow if row is less than the m_automatedindexrow
                if(row <= m_automatedindexrow)
                    m_automatedindexrow--;
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

    //..resetting m_lastclickindgrid
    if(checkrowfound && m_lastclickindgrid != -1)
        m_lastclickindgrid = -1;

    return checkrowfound;
}

void
TabIndexes::ChangeListOnDelete(IndexesStructWrapper* cwrapobj)
{
    /// For existing index, set m_newval to NULL
    ///     for new added index, remove wrapper from the m_listwrapperstruct and delete it

    IndexColumn *indcol = NULL, *tmpindcols;
    if(!cwrapobj)
        return;

    if(!cwrapobj->m_newval)
        return;

    if(cwrapobj->m_oldval)
    {
        if(cwrapobj->m_oldval != cwrapobj->m_newval)
        {
            indcol = (IndexColumn*) cwrapobj->m_newval->m_listcolumns->GetFirst();

            while(indcol)
            {
                tmpindcols = indcol;
                indcol = (IndexColumn*) cwrapobj->m_newval->m_listcolumns->Remove(indcol);
                delete tmpindcols;
            }
            delete cwrapobj->m_newval->m_listcolumns;
            cwrapobj->m_newval->m_listcolumns = NULL;
            delete cwrapobj->m_newval;
        }
        cwrapobj->m_newval = NULL;
    }
    else
    {
        m_listwrapperstruct.Remove(cwrapobj);
        delete cwrapobj;
    }
}

wyBool
TabIndexes::DropIndex(wyUInt32 row)
{
    /// Dropping the index from the currently selected/focused row in the grid
	wyInt32                 ret = IDNO;
    IndexesStructWrapper   *pwrapobj = NULL;
    wyString                indexname(""), indexcolsstr("");
    
    pwrapobj = (IndexesStructWrapper*) CustomGrid_GetRowLongData(m_hgridindexes, row);

    if(pwrapobj && pwrapobj->m_newval)
    {
        GetGridCellData(m_hgridindexes, row, CNAME, indexname);
        GetGridCellData(m_hgridindexes, row, DATATYPE, indexcolsstr);
        
        if(indexname.GetLength() || indexcolsstr.GetLength() || pwrapobj->m_oldval)
        {
            //  Ask for confirmation, if the wrapper is associated with the row and the index-values are not deleted from the row
            ret = MessageBox(m_hgridindexes, _(L"Do you want to drop this index?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
            
            if(ret != IDYES)
                return wyFalse;
        }
        IndexColumn *iindcols = NULL;
        IndexedBy *indexedby = NULL;

        if(row == m_automatedindexrow)
        {
            m_automatedindexrow = -1;
            m_ptabmgmt->m_tabfields->OnPrimaryIndexChange();
        }
        if(pwrapobj->m_newval && pwrapobj->m_newval->m_listcolumns)
        {
            iindcols = (IndexColumn*)pwrapobj->m_newval->m_listcolumns->GetFirst();

            //..remove index-column entried from the fieldswrapper and then deleting them
            while(iindcols)
            {
                indexedby = (IndexedBy*) iindcols->m_pcwrapobj->m_listindexesworkingcopy.GetFirst();

                while(indexedby)
                {
                    if(indexedby->m_pindexwrap == pwrapobj)
                    {
                        iindcols->m_pcwrapobj->m_listindexesworkingcopy.Remove(indexedby);
                        delete indexedby;
                        break;
                    }
                    indexedby = (IndexedBy*) indexedby->m_next;
                }
                iindcols = (IndexColumn*) iindcols->m_next;
            }
        }
        ChangeListOnDelete(pwrapobj);
    }
    CustomGrid_DeleteRow(m_hgridindexes, row, wyTrue);
    m_lastclickindgrid = -1;

    if(CustomGrid_GetRowCount(m_hgridindexes))
    {
        if(row == 0)    // if the selected row was first then we just select the first row and column
        {
            CustomGrid_SetCurSelection(m_hgridindexes, 0, INDEXCOLUMNS);
        }
        else
        {
	        CustomGrid_SetCurSelection(m_hgridindexes, --row, INDEXCOLUMNS);
        }
    }

    if(ret == IDYES && m_ptabmgmt->m_tabinterfaceptr->m_dirtytab != wyTrue)
    {
        m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);
    }
    
	return wyTrue;
}

void 
TabIndexes::Resize()
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
        MoveWindow(m_hgridindexes, hpos, vpos, rcmain.right - 3, height, TRUE);
    else
        MoveWindow(m_hgridindexes, hpos, vpos, rcmain.right - 2, height, TRUE);
}

wyBool
TabIndexes::GenerateCreateQuery(wyString &query)
{
    wyBool      flag = wyTrue;
    wyUInt32    rowcount = 0;
    wyString    indexnamestr, columnsstr, indextypestr, indexstr, indexcomment;
    wyString    tempstr;
    rowcount = CustomGrid_GetRowCount(m_hgridindexes);
    IndexesStructWrapper *indwrap = NULL;

    for(int row=0; row<rowcount; row++)
    {
        indwrap = (IndexesStructWrapper *) CustomGrid_GetRowLongData(m_hgridindexes, row);

        if(!indwrap)
            continue;

        GetGridCellData(m_hgridindexes, row, INDEXNAME, indexnamestr);
        GetGridCellData(m_hgridindexes, row, INDEXCOLUMNS, columnsstr);
        GetGridCellData(m_hgridindexes, row, INDEXTYPE, indextypestr);
		
		if(m_ismysql553)
			GetGridCellData(m_hgridindexes, row, INDEXCOMMENT, indexcomment);

        if((!indexnamestr.GetLength()) && (!columnsstr.GetLength()))
            continue;
        
        if((columnsstr.GetLength() == 0) && row == m_automatedindexrow)
            continue;

        indexnamestr.FindAndReplace("`", "``");

        if(!indextypestr.GetLength())
            indextypestr.SetAs("index");
        else
        {
            if(indextypestr.CompareI("primary") != 0  && indextypestr.CompareI("key") != 0 )
                indextypestr.Add(" index");
        }

        if(indextypestr.CompareI("primary") == 0)
        {
            //..Go for the next index, if no column is selected for the PRIMARY index-type
            if(columnsstr.GetLength() == 0)
                continue;

            indexstr.AddSprintf("\r\n  primary key (%s)", columnsstr.GetString());
        }
        else
        {
            if(indexnamestr.GetLength())
                indexstr.AddSprintf("\r\n  %s %s%s%s (%s)", indextypestr.GetString(),
					m_backtick, indexnamestr.GetString(), m_backtick,
					columnsstr.GetString());
            else
                indexstr.AddSprintf("\r\n  %s (%s)", indextypestr.GetString(), columnsstr.GetString());
        }

		if(indexcomment.GetLength())
			indexstr.AddSprintf(" Comment \"%s\" ", indexcomment.GetString());

        indexstr.Add(",");

        //..Appending error message
        if(columnsstr.GetLength() == 0)
        {
            tempstr.SetAs(NO_COLUMNS_DEFINED_FOR_INDEX);
            indexstr.AddSprintf("\t\t/* %s */", _(tempstr.GetString()));
        }
    }
    if(indexstr.GetLength())
    {
        query.AddSprintf("%s",indexstr.GetString());
    }
    return flag;
}

wyBool
TabIndexes::GenerateAlterQuery(wyString &query)
{
    wyBool      validflg = wyTrue, flag = wyTrue;
    wyString    localquerystr("");
    
    //..Combining queries for dropped, modified and new indexes
    validflg = GetDroppedIndexes(localquerystr);
    validflg = (flag = GetNewAndModifiedIndexes(localquerystr)) ? validflg : flag;

    localquerystr.RTrim();

    if(localquerystr.GetLength())
        query.AddSprintf("%s", localquerystr.GetString());

    return validflg;
}

wyBool 
TabIndexes::GetDroppedIndexes(wyString& query)
{
    wyString            keyname, temp;
    wyString            dbname, tblname, localquerystr("");
    IndexesStructWrapper *pwrapobj = NULL;

    pwrapobj = (IndexesStructWrapper *)m_listwrapperstruct.GetFirst();

    while(pwrapobj)
    {
        if(!pwrapobj->m_newval)
        {
            temp.SetAs(pwrapobj->m_oldval->m_name);
            temp.FindAndReplace("`", "``");
            if(pwrapobj->m_oldval->m_name.CompareI("PRIMARY") == 0)
                localquerystr.Add("\r\n  drop primary key,");
            else
                localquerystr.AddSprintf("\r\n  drop index %s%s%s,", m_backtick, temp.GetString(), m_backtick);
        }
        pwrapobj = (IndexesStructWrapper *)pwrapobj->m_next;
    }

    localquerystr.RTrim();
    query.SetAs(localquerystr);

    return wyTrue;
}

wyBool
TabIndexes::GetNewAndModifiedIndexes(wyString &query, wyBool  execute)
{
    wyInt32             count=0;
    wyBool              validflg = wyTrue;
    wyString            tempstr(""), celldata, indexnamestr, temp, indextypestr, columnsstr, indexcomment= "";
    wyString            droppk(""), addpk("");
    IndexesStructWrapper *pwrapobj = NULL;

    count = CustomGrid_GetRowCount(m_hgridindexes);

    for(int row=0; row < count; row++)
    {
        pwrapobj = (IndexesStructWrapper *)CustomGrid_GetRowLongData(m_hgridindexes, row);
        
        if(!pwrapobj)       //..empty rows
            continue;
        
        if(pwrapobj->m_newval == pwrapobj->m_oldval)        //..if, the index is not changed or it's not a new index
            continue;

        if(!pwrapobj->m_newval)                             //..when user had deleted only index-name and column and not entire index, that time index row will be there but m_newval will be NULL;
            continue;

        if(pwrapobj->m_oldval && (pwrapobj->m_oldval->m_indextype.CompareI("PRIMARY") == 0))
        {
            droppk.Add("\r\n  drop primary key,");
        }
        else if(pwrapobj->m_oldval)
        {
            indexnamestr.SetAs(pwrapobj->m_oldval->m_name);
            indexnamestr.FindAndReplace("`", "``");
            tempstr.AddSprintf("\r\n  drop index %s%s%s,", m_backtick, indexnamestr.GetString(), m_backtick);
        }

        GetGridCellData(m_hgridindexes, row, INDEXNAME, indexnamestr);
        GetGridCellData(m_hgridindexes, row, INDEXCOLUMNS, columnsstr);
        GetGridCellData(m_hgridindexes, row, INDEXTYPE, indextypestr);
		if(m_ismysql553)
		GetGridCellData(m_hgridindexes, row, INDEXCOMMENT, indexcomment);
        if(!indextypestr.GetLength())
            indextypestr.SetAs("index");
        else
        {
            if(indextypestr.CompareI("primary") != 0 && indextypestr.CompareI("key") != 0 )
                indextypestr.Add(" index");
        }

        if(indextypestr.CompareI("primary") == 0)
        {
            if(columnsstr.GetLength())
                addpk.AddSprintf("\r\n  add primary key (%s)", columnsstr.GetString());
			if(m_ismysql553 && indexcomment.GetLength())
			{
            	addpk.AddSprintf("Comment \"%s\" ", indexcomment.GetString());
			}
			addpk.AddSprintf(",");
        }
        else
        {
            indexnamestr.FindAndReplace("`", "``");
			if(indexnamestr.GetLength())
                tempstr.AddSprintf("\r\n  add  %s %s%s%s (%s)", indextypestr.GetString(), 
					m_backtick, indexnamestr.GetString(), m_backtick,
					columnsstr.GetString());
            else
                tempstr.AddSprintf("\r\n  add %s (%s)", indextypestr.GetString(), columnsstr.GetString());
			if(m_ismysql553 && indexcomment.GetLength())
			{
				indexcomment.LTrim();
				tempstr.AddSprintf(" Comment \"%s\" ",indexcomment.GetString());
			}
				tempstr.AddSprintf(",");
        }
        if(pwrapobj->m_errmsg)
        {
            tempstr.AddSprintf("\t\t/* %s */", pwrapobj->m_errmsg->GetString());
        }
    }
    if(droppk.GetLength())
        query.AddSprintf("%s", droppk.GetString());

    if(addpk.GetLength())
        query.AddSprintf("%s", addpk.GetString());

    query.AddSprintf("%s", tempstr.GetString());
    query.RTrim();
    
    return validflg;
}

void
TabIndexes::OnIDOK(HWND hwnd)
{
    wyInt32                 count, row, col;
    List                    *listindexcolumns = NULL;
    IndexColumn           *indcols = NULL, *tmpindcols = NULL;
    IndexesStructWrapper   *cindexeswrap = NULL;
    IndexInfo                *indexstruct = {0};
    IndexedBy              *indexby = NULL;
    wyBool                  markasdirty = wyFalse;
    wyString                refcols(""), indexlenstr("");
    FieldStructWrapper      *fieldwrap = NULL;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    CustomGrid_ApplyChanges(m_hdlggrid);

    row = CustomGrid_GetCurSelRow(m_hgridindexes);
    col = CustomGrid_GetCurSelCol(m_hgridindexes);

    cindexeswrap = (IndexesStructWrapper*) CustomGrid_GetRowLongData(m_hgridindexes, row);

    listindexcolumns = (List*) CustomGrid_GetItemLongValue(m_hgridindexes, row, col);

    if(!listindexcolumns)
        listindexcolumns = new List();

    if(!cindexeswrap)      //..Create/Alter table, adding a new index - (selecting index-columns only, when index-name is NULL)
    {
        //..Creating a new iIndex struct object
        indexstruct = new IndexInfo;
        indexstruct->m_name.SetAs("");
        indexstruct->m_colsstr.SetAs("");
        indexstruct->m_indextype.SetAs("");
		indexstruct->m_indexcomment.SetAs("");
        indexstruct->m_listcolumns = listindexcolumns;


        //..Creating a new wrapper
        cindexeswrap = new IndexesStructWrapper(indexstruct, wyTrue);
        m_listwrapperstruct.Insert(cindexeswrap);
        markasdirty = wyTrue;
    }
    else if(cindexeswrap->m_oldval)
    {
        //..unchanged existing index [Alter table only]
        if(cindexeswrap->m_newval == cindexeswrap->m_oldval)
        {
            //..Create duplicate of oldval and assign it to newval
            cindexeswrap->m_newval = GetDuplicateIndexesStruct(cindexeswrap->m_oldval);
            listindexcolumns = cindexeswrap->m_newval->m_listcolumns;
        }
        else if(!cindexeswrap->m_newval)
        {
            cindexeswrap->m_newval = new IndexInfo();
            cindexeswrap->m_newval->m_listcolumns = listindexcolumns;
        }
    }
    
    //..deleting all index-columns and remove their entries from the Columns-tab
    indcols = (IndexColumn*) listindexcolumns->GetFirst();
    while(indcols)
    {
        RemoveIndexWrappersFromFieldsWrappers(indcols, cindexeswrap);
        tmpindcols = indcols;
        indcols = (IndexColumn*) listindexcolumns->Remove(indcols);
        delete tmpindcols;
    }
    markasdirty = wyTrue;
    
    count = CustomGrid_GetRowCount(m_hdlggrid);

    //..Creating list of checked columns and making their entried in the fields-wrappers
    for(int i=0; i<count; i++)
    {
        if(!CustomGrid_GetRowCheckState(m_hdlggrid, i))
            continue;
        
        GetGridCellData(m_hdlggrid, i, 2, indexlenstr);

        fieldwrap = (FieldStructWrapper*)CustomGrid_GetRowLongData(m_hdlggrid, i);

        indcols = new IndexColumn(fieldwrap);
        wyString tmpstr;
        tmpstr.SetAs(fieldwrap->m_newval->m_name);
        tmpstr.FindAndReplace("`", "``");
        refcols.AddSprintf("%s%s%s", m_backtick, tmpstr.GetString(), m_backtick);
        if(indexlenstr.GetLength())
        {
            indcols->m_lenth = indexlenstr.GetAsInt32();
            refcols.AddSprintf("(%s)", indexlenstr.GetString());
        }

        refcols.Add(", ");

        listindexcolumns->Insert(indcols);

        indexby = new IndexedBy(cindexeswrap);
        fieldwrap->m_listindexesworkingcopy.Insert(indexby);
    }

    refcols.RTrim();

    wyString indexnamestr("");
	wyString indexcomment("");
    GetGridCellData(m_hgridindexes, row, INDEXNAME, indexnamestr);

	if(m_ismysql553)
		GetGridCellData(m_hgridindexes, row, INDEXCOMMENT, indexcomment);

    //..if no index name is there on the row and no ref-columns are selected, then treat that index as deleted
    if(!refcols.GetLength() && !indexnamestr.GetLength())
    {
        if(cindexeswrap->m_oldval)
        {
            if(cindexeswrap->m_newval && cindexeswrap->m_newval != cindexeswrap->m_oldval)
                delete cindexeswrap->m_newval;
            cindexeswrap->m_newval = NULL;
        }
        else if(cindexeswrap->m_newval)
        {
            m_listwrapperstruct.Remove(cindexeswrap);
            delete cindexeswrap;
            cindexeswrap = NULL;
        }
        CustomGrid_SetRowLongData(m_hgridindexes, row, (LPARAM)cindexeswrap);
        CustomGrid_SetItemLongValue(m_hgridindexes, row, col, (LPARAM) NULL);
        CustomGrid_SetText(m_hgridindexes, row, col, "");
        return;
    }
    
    if(refcols.GetLength())
    {
        if(refcols.GetCharAt(refcols.GetLength() - 1) == ',')
            refcols.Strip(1);
    }
    
    CustomGrid_SetText(m_hgridindexes, row, col, refcols.GetString());
    cindexeswrap->m_newval->m_colsstr.SetAs(refcols);
    cindexeswrap->m_newval->m_listcolumns = listindexcolumns;
	cindexeswrap->m_newval->m_indexcomment.SetAs(indexcomment);
    //..This section checks for the equality of new and old values of index. If both are same, then we shall delete the newval and newval = oldval
    //..Applicable to Alter table only
    {
        if(cindexeswrap->m_oldval && cindexeswrap->m_newval &&                                             //..If all newval and oldval values are equal, then drop the newval;
                cindexeswrap->m_oldval->m_name.CompareI(cindexeswrap->m_newval->m_name) == 0 &&                  
                cindexeswrap->m_oldval->m_indextype.CompareI(cindexeswrap->m_newval->m_indextype) == 0 &&
				(m_ismysql553?(cindexeswrap->m_oldval->m_indexcomment.CompareI(cindexeswrap->m_newval->m_indexcomment) == 0) : 1)
                )
        {
            IndexColumn *indcol = NULL, *tmpindcol = NULL;
            IndexColumn *indcolsold = NULL, *indcolsnew = NULL;

            indcolsold = (IndexColumn*) cindexeswrap->m_oldval->m_listcolumns->GetFirst();
            indcolsnew = (IndexColumn*) cindexeswrap->m_newval->m_listcolumns->GetFirst();

            while(indcolsold && indcolsnew)
            {
                if(indcolsold->m_pcwrapobj != indcolsnew->m_pcwrapobj)
                    break;

                if(indcolsold->m_lenth != indcolsnew->m_lenth)
                    break;

                indcolsold = (IndexColumn*) indcolsold->m_next;
                indcolsnew = (IndexColumn*) indcolsnew->m_next;
            }

            if(!indcolsold && !indcolsnew)
            {
                indcol = (IndexColumn*) cindexeswrap->m_newval->m_listcolumns->GetFirst();

                while(indcol)
                {
                    tmpindcol = indcol;
                    indcol = (IndexColumn*)cindexeswrap->m_newval->m_listcolumns->Remove(indcol);// indcol->m_next;
                    delete tmpindcol;
                }
                listindexcolumns = NULL;
                delete cindexeswrap->m_newval;
                cindexeswrap->m_newval = cindexeswrap->m_oldval;
                listindexcolumns = cindexeswrap->m_oldval->m_listcolumns;
                markasdirty = wyTrue;
            }
        }
    }
    
    //..This section checks for any value in the index. if no value is there and it's a new index, then the associated wrapper object will be dropped
    {
        if((!cindexeswrap->m_oldval)    &&
            cindexeswrap->m_newval      && 
            cindexeswrap->m_newval->m_name.GetLength() == 0     &&
            cindexeswrap->m_newval->m_colsstr.GetLength() == 0  &&
            cindexeswrap->m_newval->m_indextype.GetLength() == 0
            )
        {
            m_listwrapperstruct.Remove(cindexeswrap);
            delete cindexeswrap;
            cindexeswrap = NULL;
            markasdirty = wyTrue;
        }
    }
    
    CustomGrid_SetItemLongValue(m_hgridindexes, row, col, (LPARAM) listindexcolumns);
    CustomGrid_SetRowLongData(m_hgridindexes, row, (LPARAM)cindexeswrap);

    //..changing the pk-column in Columns-tab, if the index-type in the current row is PRIMARY
    if(row == m_automatedindexrow)
        m_ptabmgmt->m_tabfields->OnPrimaryIndexChange();

    if(refcols.GetLength() && (row == CustomGrid_GetRowCount(m_hgridindexes) - 1))
    {
        InsertRow();
    }
    if(markasdirty == wyTrue && !m_ptabmgmt->m_tabinterfaceptr->m_dirtytab)
        m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);

    //..Validating indexes
    ValidateIndexes();
}

void
TabIndexes::OnColDlgWMCommand(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    switch(LOWORD(wparam))
    {
    case IDOK:
        if(m_hdlggrid)
	        OnIDOK(hwnd);
        SendMessage(hwnd, WM_CLOSE, wparam, lparam);
        break;

    case IDCANCEL:
        SendMessage(hwnd, WM_CLOSE, wparam, lparam);
        break;

    case IDC_MOVEUP:
        if(m_hdlggrid)
			OnButtonUpDown(hwnd, wyTrue);
        break;

    case IDC_MOVEDOWN:
        if(m_hdlggrid)
			OnButtonUpDown(hwnd, wyFalse);
        break;
    }
}

void
TabIndexes::OnTabSelChanging()
{
    ApplyCancelGridChanges();
    ValidateIndexes();
}

void
TabIndexes::ApplyCancelGridChanges()
{
    wyInt32 row, col;
    
    row = CustomGrid_GetCurSelRow(m_hgridindexes);
    col = CustomGrid_GetCurSelCol(m_hgridindexes);

    if(col == INDEXTYPE && m_automatedindexrow != row)
        CustomGrid_CancelChanges(m_hgridindexes, wyTrue);
    else
        CustomGrid_ApplyChanges(m_hgridindexes, wyTrue);
}

void
TabIndexes::OnTabSelChange()
{
    HWND   hwndarr[10] = {m_hgridindexes, m_ptabmgmt->m_hwndtool, NULL};

    EnumChildWindows(m_hwnd, TableTabInterfaceTabMgmt::ShowWindowsProc, (LPARAM)hwndarr);

    SetFocus(m_hgridindexes);
    
    SendMessage(m_ptabmgmt->m_hwndtool, TB_HIDEBUTTON, (WPARAM)IDM_SEPARATOR, (LPARAM)MAKELONG(TRUE,0));
    SendMessage(m_ptabmgmt->m_hwndtool, TB_HIDEBUTTON, (WPARAM)IDI_MOVEUP, (LPARAM)MAKELONG(TRUE,0));
    SendMessage(m_ptabmgmt->m_hwndtool, TB_HIDEBUTTON, (WPARAM)IDI_MOVEDOWN, (LPARAM)MAKELONG(TRUE,0));
}

void
TabIndexes::CancelChanges(wyBool isaltertable)
{
    wyUInt32    count = -1, row = -1;
    IndexesStructWrapper *cwrapobj = NULL;

    //..The below function will apply or cancel the custgrid changes
    OnTabSelChanging();
    
    count = CustomGrid_GetRowCount(m_hgridindexes);
    
    for(row = 0; row < count; row++)
    {
        cwrapobj = (IndexesStructWrapper *)CustomGrid_GetRowLongData(m_hgridindexes, row);
        if(cwrapobj && cwrapobj->m_newval && !cwrapobj->m_oldval)                           //..Create/Alter table (If new (valid)field is added)
        {
            m_listwrapperstruct.Remove(cwrapobj);

            IndexColumn   *indcol = NULL, *tmpindcol = NULL;
            
            if(cwrapobj && cwrapobj->m_newval && cwrapobj->m_newval->m_listcolumns)
                indcol = (IndexColumn*) cwrapobj->m_newval->m_listcolumns->GetFirst();

            while(indcol)
            {
                tmpindcol = indcol;
                indcol = (IndexColumn*)cwrapobj->m_newval->m_listcolumns->Remove(indcol);
                delete tmpindcol;
            }

            if(cwrapobj->m_newval->m_listcolumns)
                delete cwrapobj->m_newval->m_listcolumns;
            cwrapobj->m_newval->m_listcolumns = NULL;

            delete cwrapobj;
        }
        else if(cwrapobj && cwrapobj->m_newval && cwrapobj->m_oldval)                           //..Alter table (If field is added)
        {
            if(cwrapobj->m_newval != cwrapobj->m_oldval)                                    //..If field modified, delete m_newval, and set it as m_oldval
                delete cwrapobj->m_newval;

            cwrapobj->m_newval = cwrapobj->m_oldval;
        }
    }

    cwrapobj = (IndexesStructWrapper*)m_listwrapperstruct.GetFirst();

    while(cwrapobj)
    {
        if(cwrapobj->m_errmsg)
            delete cwrapobj->m_errmsg;
        cwrapobj->m_errmsg = NULL;

        cwrapobj->m_newval = cwrapobj->m_oldval;
        cwrapobj = (IndexesStructWrapper*)cwrapobj->m_next;
    }

    m_automatedindexrow =   -1;
    m_lastclickindgrid  =   -1;

    CustomGrid_DeleteAllRow(m_hgridindexes, wyTrue);

    //..Create table
    if(!isaltertable)
    {
        row = InsertRow();
        CustomGrid_SetCurSelection(m_hgridindexes, row, INDEXCOLUMNS);
        InvalidateRect(m_hgridindexes, NULL, TRUE);
        return;
    }

    //..Alter table
    FillInitValues();

    row = InsertRow();

    if(m_ptabmgmt->GetActiveTabImage() == IDI_TABIMG_INDEXES)
    {
        SetFocus(m_hgridindexes);
    }
    CustomGrid_SetCurSelection(m_hgridindexes, row, INDEXCOLUMNS);
    return;
}

void
TabIndexes::ClearAllMemory(wyBool iscallfromdestructor)
{
    IndexesStructWrapper *cwrapobj = NULL, *tmpcwrapobj = NULL;
    cwrapobj = (IndexesStructWrapper *) m_listwrapperstruct.GetFirst();

    while(cwrapobj)
    {
        tmpcwrapobj = cwrapobj;
        cwrapobj = (IndexesStructWrapper *) m_listwrapperstruct.Remove(cwrapobj);
        delete tmpcwrapobj;
    }

    return;
}

void
TabIndexes::ClearListIndexCols(List *listindcols, IndexesStructWrapper  *indwrap)
{
    IndexColumn   *indcols = NULL, *tmpindcols = NULL;
    
    if(!listindcols)
        return;

    indcols = (IndexColumn*) listindcols->GetFirst();

    while(indcols)
    {
        tmpindcols = indcols;

        if(indwrap)
            RemoveIndexWrappersFromFieldsWrappers(tmpindcols, indwrap);
        
        indcols = (IndexColumn*)listindcols->Remove(indcols);
        delete tmpindcols;
    }
}

IndexesStructWrapper*
TabIndexes::GetDroppedPKWrapper()
{
    IndexesStructWrapper*   pkindwrap = NULL;

    pkindwrap = (IndexesStructWrapper*) m_listwrapperstruct.GetFirst();
    while(pkindwrap)
    {
        if(pkindwrap->m_oldval && !pkindwrap->m_newval && ((pkindwrap->m_oldval->m_name.CompareI("PRIMARY") == 0) || (pkindwrap->m_oldval->m_name.CompareI("PRIMARY KEY") == 0)))
        {
            break;
        }
        pkindwrap  = (IndexesStructWrapper*) pkindwrap->m_next;
    }

    return pkindwrap;
}

void
TabIndexes::HandlePrimaryKeyIndex()
{
	wyInt32		count = -1, row;
	wyString	newpkindexcols(""), tmpstr("");
    wyString    indextype("");
	HWND		hgridfields;
    wyBool              isaltertable;
    IndexColumn   *indcols = NULL;
    IndexInfo        *index = NULL;
    IndexesStructWrapper   *pwrapobj = NULL;
    List            *listindcols = NULL;
    FieldStructWrapper *fieldwrap = NULL;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    isaltertable = m_ptabmgmt->m_tabinterfaceptr->m_isaltertable;
    hgridfields  = m_ptabmgmt->m_tabfields->m_hgridfields;
    count = CustomGrid_GetRowCount(hgridfields);

    //..If PRIMARY KEY Index is not present on the grid
    if(m_automatedindexrow == -1)
    {
        //..Add new row in the 1st position
        if(CustomGrid_GetRowCount(m_hgridindexes) >=1)
			m_automatedindexrow = CustomGrid_InsertRowInBetween(m_hgridindexes, 0);
		else
			m_automatedindexrow = CustomGrid_InsertRow(m_hgridindexes, wyTrue);
        CustomGrid_SetButtonVis(m_hgridindexes, m_automatedindexrow, 1, wyTrue);
        CustomGrid_SetButtonText(m_hgridindexes, m_automatedindexrow, INDEXCOLUMNS, L"...");

        index = new IndexInfo();
        listindcols = new List();

        //..Checks whether existing PK is already dropped (Alter table)
        if(m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
            pwrapobj = GetDroppedPKWrapper();

        if(pwrapobj)
        {
            pwrapobj->m_newval = index;
        }
        else
        {
            pwrapobj = new IndexesStructWrapper(index, wyTrue);
            m_listwrapperstruct.Insert(pwrapobj);
        }
        
        pwrapobj->m_newval->m_name.SetAs("PRIMARY");
        pwrapobj->m_newval->m_listcolumns = listindcols;
        pwrapobj->m_newval->m_indextype.SetAs("PRIMARY");

        CustomGrid_SetRowLongData(m_hgridindexes, m_automatedindexrow, (LPARAM) pwrapobj);
    }
    else
    {
        pwrapobj = (IndexesStructWrapper*) CustomGrid_GetRowLongData(m_hgridindexes, m_automatedindexrow);

        if(pwrapobj->m_newval == pwrapobj->m_oldval)
        {
            pwrapobj->m_newval = index = GetDuplicateIndexesStruct(pwrapobj->m_oldval);
            listindcols = pwrapobj->m_newval->m_listcolumns;
        }
        else
            listindcols = (List*)CustomGrid_GetItemLongValue(m_hgridindexes, m_automatedindexrow, INDEXCOLUMNS);

        //..Clearing all the list columns, if they exist
        if(listindcols)
            ClearListIndexCols(listindcols, pwrapobj);
        else
            listindcols = new List();

        pwrapobj->m_newval->m_listcolumns = listindcols;
    }

    //..Reading pk-column from the grid in the Columns-tab, creating list of index-columns
    for(row = 0; row < count; row++)
    {
        fieldwrap = (FieldStructWrapper*) CustomGrid_GetRowLongData(hgridfields, row);
        
        if(!fieldwrap)
            continue;

        GetGridCellData(hgridfields, row, PRIMARY, tmpstr);

        if(tmpstr.Compare(GV_TRUE) != 0)
            continue;

        newpkindexcols.AddSprintf("%s%s%s, ", m_backtick, fieldwrap->m_newval->m_name.GetString(), m_backtick);
        indcols = new IndexColumn(fieldwrap);
        listindcols->Insert(indcols);
            
        fieldwrap->m_listindexesworkingcopy.Insert(new IndexedBy(pwrapobj));
    }

    if(newpkindexcols.GetLength())
    {
        newpkindexcols.Strip(2);

        pwrapobj->m_newval->m_colsstr.SetAs(newpkindexcols);

        CustomGrid_SetText(m_hgridindexes, m_automatedindexrow, INDEXNAME, "PRIMARY");
        CustomGrid_SetText(m_hgridindexes, m_automatedindexrow, INDEXTYPE,  "PRIMARY");

        if(pwrapobj->m_newval && pwrapobj->m_oldval && pwrapobj->m_newval != pwrapobj->m_oldval)
        {
            ScanEntireRow(m_automatedindexrow, INDEXCOLUMNS, newpkindexcols);
            if(pwrapobj->m_newval == pwrapobj->m_oldval)
            {
                ClearListIndexCols(listindcols);
                listindcols = pwrapobj->m_newval->m_listcolumns;
            }
        }
        
        CustomGrid_SetText(m_hgridindexes, m_automatedindexrow, INDEXCOLUMNS, (wyChar*)newpkindexcols.GetString());
        CustomGrid_SetItemLongValue(m_hgridindexes, m_automatedindexrow, INDEXCOLUMNS, (LPARAM) listindcols);
    }
    else
    {
        //..If there is no pk-column, then delete that index from the grid.
        CustomGrid_DeleteRow(m_hgridindexes, m_automatedindexrow, wyTrue);
        if(!pwrapobj->m_oldval)
        {
            m_listwrapperstruct.Remove(pwrapobj);
            delete pwrapobj;
            pwrapobj = NULL;
        }
        else if(pwrapobj->m_oldval == pwrapobj->m_newval)
        {
            pwrapobj->m_newval = NULL;
        }
        else
        {
            delete pwrapobj->m_newval;
            pwrapobj->m_newval = NULL;
        }

        m_automatedindexrow = -1;
    }
	return;
}

void
TabIndexes::ReInitializeGrid()
{
    /// Function re-initializes all index-values in the grid after 'Save'
    wyString    tblname, dbname;
    wyInt32     row = -1;

    m_automatedindexrow = -1;

    ClearAllMemory();
    CustomGrid_DeleteAllRow(m_hgridindexes, wyTrue);
    
    FetchIndexValuesIntoWrapper();
    FillInitValues();

    row = InsertRow();
    if(row != -1)
        CustomGrid_SetCurSelection(m_hgridindexes, row, INDEXCOLUMNS);
}

wyBool
TabIndexes::ValidateIndexes(wyBool showmsg)
{
    wyUInt32    nrows = 0;
    wyString    columnsstr;
    
    nrows = CustomGrid_GetRowCount(m_hgridindexes);
    IndexesStructWrapper *indwrap = NULL;

    for(int row=0; row<nrows; row++)
    {
        indwrap = (IndexesStructWrapper *) CustomGrid_GetRowLongData(m_hgridindexes, row);

        if(!indwrap)
            continue;

        delete indwrap->m_errmsg;
        indwrap->m_errmsg = NULL;

        if(row == m_automatedindexrow)
            continue;

        if(!indwrap->m_newval)
            continue;

        GetGridCellData(m_hgridindexes, row, INDEXCOLUMNS, columnsstr);

        if(!columnsstr.GetLength())
        {
            if(!indwrap->m_errmsg)
                indwrap->m_errmsg = new wyString;
            indwrap->m_errmsg->SetAs(NO_COLUMNS_DEFINED_FOR_INDEX);

            if(showmsg)
            {
                if(m_ptabmgmt->GetActiveTabImage() != IDI_TABIMG_INDEXES)
                    CustomTab_SetCurSel(m_ptabmgmt->m_hwnd, 1);

                CustomGrid_SetCurSelection(m_hgridindexes, row, INDEXCOLUMNS, wyTrue);
                CustomGrid_EnsureVisible(m_hgridindexes, row, INDEXCOLUMNS, wyTrue);

                MessageBox(m_hwnd, NO_COLUMNS_DEFINED_FOR_INDEX, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
                return wyFalse;
            }
        }
    }
    return wyTrue;
}

wyBool
TabIndexes::GenerateQuery(wyString& query)
{
    wyString    str("");
    wyBool      retval = wyTrue;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    if(m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
        retval = GenerateAlterQuery(str);
    else
        retval = GenerateCreateQuery(str);

    if(str.GetLength())
    {
        if(m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
            query.AddSprintf(" %s\r\n", str.GetString());
        else
            query.AddSprintf("%s\r\n", str.GetString());
        return wyTrue;
    }

    return retval;
}

void
TabIndexes::HandleIndexesOnDatatypeChange(IndexesStructWrapper* indexwrap, FieldStructWrapper *modifiedwrap)
{
    wyString    indcolsstr(""), indexname("");
    wyString    tmpstr;
    wyUInt32    nrows = 0, row = -1;
    IndexColumn *iindcols = NULL;

    if(!indexwrap)
        return;

    //..If index is not modified, then no need to change it..
    /////on change field, mysql will automatically drop the index-length, if the datatype is not valid (after modification in fields)
    if(indexwrap->m_newval == indexwrap->m_oldval)
        return;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    nrows = CustomGrid_GetRowCount(m_hgridindexes);

    //..Will go through each row and get the row of the index-wrapper
    for(row=0; row<nrows; row++)
    {
        if(indexwrap == ((IndexesStructWrapper*)CustomGrid_GetRowLongData(m_hgridindexes, row)))
            break;
    }

    if(row == nrows)
        return;

    if(indexwrap->m_newval == indexwrap->m_oldval)
        indexwrap->m_newval = GetDuplicateIndexesStruct(indexwrap->m_oldval);

    iindcols = (IndexColumn *)indexwrap->m_newval->m_listcolumns->GetFirst();

    //..Going through all index-columns
    while(iindcols)
    {
        tmpstr.SetAs(iindcols->m_pcwrapobj->m_newval->m_name);
        tmpstr.FindAndReplace("`", "``");

        indcolsstr.AddSprintf("%s%s%s", m_backtick, tmpstr.GetString(), m_backtick);
        
        //..if wrapper is same as modified field wrapper, set length to -1; (Reset length)..
        if(iindcols->m_pcwrapobj == modifiedwrap)
            iindcols->m_lenth = -1;

        if(iindcols->m_lenth != -1)
            indcolsstr.AddSprintf("(%d)", iindcols->m_lenth);
        indcolsstr.Add(", ");

        iindcols = (IndexColumn *)iindcols->m_next;
    }

    indcolsstr.RTrim();
    if(indcolsstr.GetLength())      //..true, if Other index-column is available for that particular INDEX
    {
        while(indcolsstr.GetCharAt(indcolsstr.GetLength()-1) == ',')
            indcolsstr.Strip(1);
    }
    
    indexwrap->m_newval->m_colsstr.SetAs(indcolsstr);
    ScanEntireRow(row,INDEXCOLUMNS, indcolsstr);

    CustomGrid_SetText(m_hgridindexes, row, INDEXCOLUMNS, (wyChar*) indcolsstr.GetString());
}

void
TabIndexes::HandleIndexesOnFieldRename(IndexesStructWrapper* indexwrap, FieldStructWrapper *modifiedwrap)
{
    /// Changes the index-columns string, if the column from grid in the Columns-tab in renamed
    wyString    indcolsstr(""), indexname("");
    wyString    tmpstr;
    wyUInt32    nrows = 0, row = -1;
    IndexColumn *iindcols = NULL;

    if(!indexwrap)
        return;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    nrows = CustomGrid_GetRowCount(m_hgridindexes);

    for(row=0; row<nrows; row++)
    {
        if(indexwrap == ((IndexesStructWrapper*)CustomGrid_GetRowLongData(m_hgridindexes, row)))
            break;
    }

    if(row == nrows)
        return;

    iindcols = (IndexColumn *)indexwrap->m_newval->m_listcolumns->GetFirst();
    while(iindcols)
    {
        tmpstr.SetAs(iindcols->m_pcwrapobj->m_newval->m_name);
        tmpstr.FindAndReplace("`", "``");

        indcolsstr.AddSprintf("%s%s%s", m_backtick, tmpstr.GetString(), m_backtick);
            
        if(iindcols->m_lenth != -1)
            indcolsstr.AddSprintf("(%d)", iindcols->m_lenth);
        indcolsstr.Add(", ");

        iindcols = (IndexColumn *)iindcols->m_next;
    }

    indcolsstr.RTrim();
    if(indcolsstr.GetLength())      //..true, if Other index-column is available for that particular INDEX
    {
        while(indcolsstr.GetCharAt(indcolsstr.GetLength()-1) == ',')
            indcolsstr.Strip(1);
    }
    if(indexwrap->m_newval == indexwrap->m_oldval)
        indexwrap->m_newval = GetDuplicateIndexesStruct(indexwrap->m_oldval);
    
    indexwrap->m_newval->m_colsstr.SetAs(indcolsstr);
    ScanEntireRow(row,INDEXCOLUMNS, indcolsstr);

    CustomGrid_SetText(m_hgridindexes, row, INDEXCOLUMNS, (wyChar*) indcolsstr.GetString());
}

void
TabIndexes::HandleIndexesOnFieldDelete(IndexesStructWrapper* indexwrap, FieldStructWrapper *deletedfieldwrap)
{
    wyString    indcolsstr(""), indexname("");
    wyString    tmpstr;
    wyUInt32    nrows = 0, row = -1;
    IndexColumn *iindcols = NULL;

    if(!indexwrap)
        return;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

    nrows = CustomGrid_GetRowCount(m_hgridindexes);

    for(row=0; row<nrows; row++)
    {
        if(indexwrap == ((IndexesStructWrapper*)CustomGrid_GetRowLongData(m_hgridindexes, row)))
            break;
    }

    if(row == nrows)
        return;

    if(indexwrap->m_newval == indexwrap->m_oldval)
    {
        indexwrap->m_newval = GetDuplicateIndexesStruct(indexwrap->m_oldval);
    }

    iindcols = (IndexColumn *)indexwrap->m_newval->m_listcolumns->GetFirst();
    while(iindcols)
    {
        IndexColumn *tmpindcols = iindcols;

        if(iindcols->m_pcwrapobj == deletedfieldwrap)
        {
            iindcols = (IndexColumn *)indexwrap->m_newval->m_listcolumns->Remove(iindcols);
            delete tmpindcols;
        }
        else
        {
            tmpstr.SetAs(iindcols->m_pcwrapobj->m_newval->m_name);
            tmpstr.FindAndReplace("`", "``");

            indcolsstr.AddSprintf("%s%s%s", m_backtick, tmpstr.GetString(), m_backtick);
            
            if(iindcols->m_lenth != -1)
                indcolsstr.AddSprintf("(%d)", iindcols->m_lenth);
            indcolsstr.Add(", ");

            iindcols = (IndexColumn *)iindcols->m_next;
        }
    }

    if(indcolsstr.GetLength())      //..true, if Other index-column is available for that particular INDEX
    {
        //indcolsstr.Strip(2);
        indcolsstr.RTrim();
        while(indcolsstr.GetCharAt(indcolsstr.GetLength()-1) == ',')
            indcolsstr.Strip(1);

        indexwrap->m_newval->m_colsstr.SetAs(indcolsstr);
        CustomGrid_SetText(m_hgridindexes, row, INDEXCOLUMNS, (wyChar*) indcolsstr.GetString());
        ScanEntireRow(row,INDEXCOLUMNS, indcolsstr);
    }
    else        //..No Index Columns in the list
    {
        if(indexwrap->m_oldval)
        {
            if(indexwrap->m_oldval != indexwrap->m_newval)
            {
                delete indexwrap->m_newval;
            }
            indexwrap->m_newval = NULL;
        }
        else
        {
            m_listwrapperstruct.Remove(indexwrap);
            delete indexwrap;
        }
        if(row == m_automatedindexrow)
            m_automatedindexrow = -1;

        CustomGrid_DeleteRow(m_hgridindexes, row, wyTrue);
    }
}
//restrict the minimum size
void
TabIndexes::OnWMSizeInfo(LPARAM lparam, HWND hwnd)
{
    MINMAXINFO* pminmax = (MINMAXINFO*)lparam;

    pminmax->ptMinTrackSize.x = m_wndrect.right - m_wndrect.left+75;
    pminmax->ptMinTrackSize.y = m_wndrect.bottom - m_wndrect.top+100;
}

void
TabIndexes::PositionCtrls(HWND hwnd)
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
TabIndexes::GetCtrlRects(HWND hwnddlg)
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
TabIndexes::OnWMPaint(HWND hwnd)
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
TabIndexes::PositionWindow(RECT* prect, HWND hwnd)
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
    width = temprect.right - temprect.left + 75;
    height = temprect.bottom - temprect.top+150;
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
