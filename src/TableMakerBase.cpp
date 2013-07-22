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


#include "TableMakerBase.h"
#include "CustGrid.h"
#include "MySQLVersionHelper.h"
#include "GUIHelper.h"
#include "CommonHelper.h" 

#define	ZERO 0

#define	RELDLGWD	735
#define	RELDLGHT    400

extern PGLOBALS		pGlobals;

TableMakerBase::TableMakerBase()
{
	m_autoinccheck		= wyFalse;
	m_autoincpresent	= wyFalse;
	m_isprimarydefined	= wyFalse;
	m_ret				= wyFalse;
	m_isenumorset		= wyFalse; 
	m_charsetres		= NULL;

	m_p = new Persist;
	m_p->Create("TableMaker");
}


TableMakerBase::~TableMakerBase() 
{
	if(m_charsetres)
	{
		m_tunnel->mysql_free_result(m_charsetres);
		m_charsetres = NULL;
	}
}

NewTableElem::NewTableElem(const wyChar* tablename)
{
	m_tablename.SetAs(tablename);
}

NewTableElem::~NewTableElem()
{
}

wyInt32
TableMakerBase::CreateDialogWindow()
{
	return ::DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_TABLEMAKER), m_querywnd->GetHwnd(), (DLGPROC)TableMakerBase::DlgWndProc,
						   (LONG)this);
}

void
TableMakerBase::SetInitialValues(MDIWindow * querywindow, 
                             Tunnel * tunnel, 
                             PMYSQL mysql, 
                             const wyChar * dbname, 
                             const wyChar * tablename)
{
	m_mysql             =	mysql;
	m_tunnel            =   tunnel;
	m_querywnd          =   querywindow;
	
    m_dbname.SetAs(dbname);
    m_tablename.SetAs(tablename);
}

BOOL CALLBACK
TableMakerBase::DlgWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TableMakerBase*	tablemgmt = (TableMakerBase*)GetWindowLong(hwnd, GWL_USERDATA);
	
	switch (message )
	{
	case WM_INITDIALOG:
		{
			SetWindowLong (hwnd, GWL_USERDATA, (LONG)lParam );
            LocalizeWindow(hwnd);
			PostMessage (hwnd, UM_INITDLGVALUES, 0, 0 );
			
		}
		return 0;

	case WM_COMMAND:
        tablemgmt->HandleWMCommand(wParam, lParam);
        break;

	case WM_PAINT:
		//draws the resize gripper at bottom-right corner of dialog
		DrawSizeGripOnPaint(hwnd);
	break;

	case WM_HELP:
		{
			tablemgmt->HandleHelp();
		}
		return wyTrue;

    case WM_NCDESTROY:
        {
            // call implemented class ncdestroy so that they can destroy their specific memory
            tablemgmt->ProcessWMNCDestroy();
        }
        break;

	case WM_DESTROY:
		SaveInitPos(hwnd, TABLEMAKER_SECTION);
		delete tablemgmt->m_p;
		return 0;

	case UM_INITDLGVALUES:
		{
			tablemgmt->SetDlgHwnd(hwnd);
			if(tablemgmt->InitDialog() != wyTrue)
            {
                EndDialog(hwnd, 0);
                return 1;
            }
		}
		break;

	case WM_SIZE:
		if(wParam == SIZE_RESTORED)
			tablemgmt->Resize(lParam);
		return 0;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO pMMI = (LPMINMAXINFO)lParam;
			pMMI->ptMinTrackSize.x = RELDLGWD;
			pMMI->ptMinTrackSize.y = RELDLGHT;
		}
		break;

	default:
		break;
	}
	
	// Dialog box always return 0.
	return 0;
}

LRESULT CALLBACK
TableMakerBase::GridWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TableMakerBase		*tablemgmt = (TableMakerBase*)CustomGrid_GetLongData(hwnd);
	wyString			tblname("__create_table");
		
	switch(message)
	{
    case GVN_PASTECLIPBOARDBEGIN:
        return TRUE;

	case GVN_BEGINLABELEDIT:
		return tablemgmt->OnGVNBeginLabelEdit(wParam,lParam);

	case GVN_ENDLABELEDIT:
		return tablemgmt->OnGVNEndLabelEdit(wParam,lParam);			

	case GVN_ENDADDNEWROW:
		return tablemgmt->OnGVNEndAddNewRow(wParam,lParam);

	case GVN_SPLITTERMOVE:
		OnGridSplitterMove(hwnd, &tblname, wParam);	
		break;

	case GVN_FINISHENDLABELEDIT:
		if(lParam == 1)
		{
			tablemgmt->HandleEnumColumn(hwnd, LOWORD(wParam), HIWORD(wParam));
		}
		break;
	}

	return 1;
}

	
wyBool
TableMakerBase::InitDialog()
{
	HICON hicon;

	SetTitle();
	
	if(CreateGrid() == wyFalse)
		return wyFalse;

	//Sets the dialog cordinates
	SetInitPos(m_hwnd, TABLEMAKER_SECTION);

	//Set icon for dialog	
	hicon = CreateIcon(IDI_CREATETABLE);
	SendMessage(m_hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hicon);
	DestroyIcon(hicon);
	return InitDialogStep2();
}

wyBool
TableMakerBase::InitDialogStep2 ()
{
	wyBool			ismysql41 = IsMySQL41(m_tunnel, m_mysql);

	InitializeGrid();

	// Now we need to call the virtual initialize, if tablemake or tableditor want to
	// initialize something specific to them
	if(Initialize() == wyFalse)
        return wyFalse;

	if(ismysql41 == wyTrue)
	{
		m_p->Add(m_hwnd, IDC_HIDECOLUMNS, "HideCharsetAndCollation", "1", CHECKBOX);
		HideCharsetAndCollation();
	}
	else
		ShowWindow(GetDlgItem(m_hwnd, IDC_HIDECOLUMNS), SW_HIDE);

	// set the correct selection and focus to the grid
	CustomGrid_SetCurSelection(m_hwndgrid, 0, 0 );
	SetFocus(m_hwndgrid );

	return wyTrue;
}

void 
TableMakerBase::SetTitle()
{
	wyString	title;

	GetTitle(title);

	SetWindowText(m_hwnd, title.GetAsWideChar() );
}

void 
TableMakerBase::MoveDialog()
{
	RECT		rcobj;			// RECT coordinates of Object Browser
	RECT		rcedit;			// RECT coordinates of the edit control
	
	VERIFY  (GetWindowRect (m_querywnd->GetQueryObject()->m_hwnd, &rcobj ) );
	VERIFY(GetWindowRect(m_querywnd->GetTabModule()->GetHwnd(), &rcedit));      
	
	// Basically we move the grid control to the left hand corner of object browser
	// to the right extent of the edit control.  

	// Now move the the window.
	VERIFY  (MoveWindow (m_hwnd, rcobj.left+5, rcobj.top+5, rcedit.right-rcobj.left-15, rcobj.bottom-rcobj.top-15, wyTrue ) );
}

void 
TableMakerBase::MoveButtons(LPARAM lParam)
{
	HWND		hwndCancel, hwndOK, hwndDelete, hwndInsert, hwndOtherProp, hwndPreview, hwndhidecolumns;		// handle to various windows
	RECT		rcDlg, rcCancel, rcOK, rcDelete, rcInsert, rcOtherProp, rcttemp, rcPreview, rchidecolumns;			// RECT coordinates of all the windows
	wyInt32		wd = 0, ht = 0, left = 0, top = 0;
	
	//Sets the RECT struct
	rcDlg.right = LOWORD(lParam);
	rcDlg.bottom = HIWORD(lParam);
	rcDlg.left = 0;
	rcDlg.top = 0;
	
	VERIFY(hwndOK = GetDlgItem(m_hwnd, IDOK));
	VERIFY(hwndCancel = GetDlgItem(m_hwnd, IDCANCEL));
	VERIFY(hwndDelete = GetDlgItem(m_hwnd, IDDELETE));
	VERIFY(hwndInsert = GetDlgItem(m_hwnd, IDINSERT));
	VERIFY(hwndOtherProp = GetDlgItem(m_hwnd, IDC_OTHERPROP));
	VERIFY(hwndPreview = GetDlgItem(m_hwnd, IDC_SHOWPREVIEW));
	VERIFY(hwndhidecolumns = GetDlgItem(m_hwnd, IDC_HIDECOLUMNS));
	
	// Now get the original width and height ok the buttons so that we can set it after moving the buttons.
	VERIFY(GetClientRect(hwndCancel, &rcCancel ) );
	VERIFY(GetClientRect(hwndOK, &rcOK ) );
	VERIFY(GetClientRect(hwndDelete, &rcDelete));
	VERIFY(GetClientRect(hwndInsert, &rcInsert));
	VERIFY(GetClientRect(hwndOtherProp, &rcOtherProp));
	VERIFY(GetClientRect(hwndPreview, &rcPreview));
	VERIFY(GetClientRect(hwndhidecolumns, &rchidecolumns));

	
	//Create/Alter
	VERIFY(MoveWindow(hwndOK, rcDlg.left+10, rcDlg.bottom-35, rcOK.right, rcOK.bottom, FALSE ));
	GetWindowRect(hwndOK, &rcttemp);
	MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcttemp, 2);
	
	//Advanced property
	top = rcttemp.top;
	left = rcttemp.right + WIDTHBETBUTTON;
	ht = rcttemp.bottom - rcttemp.top;
	wd = rcOtherProp.right;

	VERIFY (MoveWindow (hwndOtherProp, left, top, wd, ht, FALSE));
	GetWindowRect(hwndOtherProp, &rcttemp);
	MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcttemp, 2);

	//Advanced property
	left = rcttemp.right + WIDTHBETBUTTON;
	wd = rcPreview.right;

	VERIFY (MoveWindow (hwndPreview, left, top, wd, ht, FALSE));
	GetWindowRect(hwndPreview, &rcttemp);
	MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcttemp, 2);

	//insert
	left = rcttemp.right + WIDTHBETBUTTON;
	wd = rcInsert.right;
	VERIFY(MoveWindow(hwndInsert, left, top, wd, ht, FALSE));
	GetWindowRect(hwndInsert, &rcttemp);
	MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcttemp, 2);

	//delete
	left = rcttemp.right + WIDTHBETBUTTON;
	wd = rcDelete.right;
	VERIFY(MoveWindow(hwndDelete, left, top, wd, ht, FALSE));
	GetWindowRect(hwndDelete, &rcttemp);
	MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcttemp, 2);

	//Cancel
	left = rcttemp.right + WIDTHBETBUTTON;
	wd = rcCancel.right;
	VERIFY (MoveWindow(hwndCancel, left, top, wd, ht, FALSE));
	GetWindowRect(hwndCancel, &rcttemp);
	MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcttemp, 2);

	//Cancel
	left = rcDlg.right - WIDTHBETBUTTON - rchidecolumns.right ;
	wd = rchidecolumns.right;
	VERIFY (MoveWindow(hwndhidecolumns, left, top + 4 , wd, rchidecolumns.bottom - rchidecolumns.top , FALSE));

	ShowWindow (hwndCancel, SW_SHOW );
}

void
TableMakerBase::MoveGrid(LPARAM lParam)
{
	wyInt32		ret = 0, ht = 0, wd = 0;

	wd = LOWORD(lParam);
	ht = HIWORD(lParam);
		
	VERIFY(ret = MoveWindow(m_hwndgrid, 5, 5, wd-20, ht-50, FALSE));

	return;
}

void
TableMakerBase::Resize(LPARAM lParam)
{
	MoveButtons(lParam);
	MoveGrid(lParam);

	RECT rctgrd, rctdlg, rcttmp;

	GetWindowRect(m_hwndgrid, &rctgrd);
	MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctgrd, 2);

	GetClientRect(m_hwnd, &rctdlg);

	rcttmp.left = rctgrd.right;
	rcttmp.top = rctdlg.top;
	rcttmp.bottom = rctdlg.bottom;
	rcttmp.right = rctdlg.right;

	rctdlg.top = rctgrd.bottom;
	rctdlg.left = 0;
	
	InvalidateRect(m_hwnd, NULL, FALSE);
	UpdateWindow(m_hwnd);
	
	//Invalidating the area that starts right scroll of grid
	InvalidateRect(m_hwnd, &rcttmp, TRUE);
	
	//Invalidate area start bottom of grid
	InvalidateRect(m_hwnd, &rctdlg, TRUE);
	UpdateWindow(m_hwnd);	
	
	return;
}

wyBool  
TableMakerBase::CreateGrid()
{	
	m_hwndgrid = CreateCustomGrid(m_hwnd, 0, 0, 0, 0, (GVWNDPROC)TableMakerBase::GridWndProc, (LPARAM)this);
	if (m_hwndgrid == NULL)
	{
		DisplayErrorText (GetLastError(), "Grid control not created");
		return wyFalse;
	}

	return wyTrue;
}

void 
TableMakerBase::InitializeGrid()
{
	wyInt32			counter;		// normal counter
	wyInt32			num_cols;		// number of columns
	GVCOLUMN		gvcol;			// structure used to create columns for grid

	wyWChar			type[][20] =	{	L"tinyint", L"smallint", L"mediumint", L"int", L"bigint", L"real", L"bit", L"bool", L"boolean",
										L"float", L"double", L"decimal", L"date", L"datetime", L"timestamp", L"numeric",  L"time", L"year", L"char", L"varchar", L"tinyblob", L"tinytext", L"text",
										L"blob", L"mediumblob", L"mediumtext", L"longblob", L"longtext", L"enum", L"set", L"binary", L"varbinary" 
                                    };    
    // all the data types that are supported by MySQL. entend it as new data types come into
   	                                        
	wyChar		    *heading[] = { 
		"Field Name", "Datatype", "Len", "Default",  "PK?", "Binary?", "Not Null?", "Unsigned?", "Auto Incr?", "Zerofill?", "Charset", "Collation", "Comment"
	};			// grid headers
	VOID		    *listtype[] = { 
									NULL, (VOID*)type, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL 
	};
	wyInt32			elemsize[] = {  0, sizeof(type[0]), 0, 0, 0,0,0,0,0,0, 0, 0,0,0
	};
	wyInt32			elemcount[] = { 
									0, sizeof(type)/sizeof(type[0]), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0, 0, 0
	};
	wyInt32			mask[] = { 
									GVIF_TEXT, GVIF_LIST, GVIF_TEXT, GVIF_TEXT,  GVIF_BOOL, GVIF_BOOL, GVIF_BOOL, GVIF_BOOL, GVIF_BOOL, GVIF_BOOL, GVIF_LIST, GVIF_LIST, GVIF_TEXT
	};  
	wyInt32			cx[] = { 150, 125, 80, 125, 70, 60, 70, 75, 75, 70,  70, 150, 125	};
	wyInt32			format[] = { GVIF_LEFT, GVIF_LEFT, GVIF_LEFT, GVIF_LEFT, GVIF_CENTER, GVIF_CENTER, GVIF_CENTER, GVIF_CENTER, GVIF_CENTER, GVIF_CENTER, GVIF_CENTER, GVIF_CENTER, GVIF_LEFT, GVIF_LEFT, GVIF_CENTER };

	wyInt32			width = 0;
	wyString		colname, dbname(RETAINWIDTH_DBNAME), tblname("__create_table");
	wyBool			isretaincolumnwidth = IsRetainColumnWidth();
	
	num_cols = sizeof (heading)/sizeof(heading[0]);
	for (counter=0; counter < num_cols ; counter++ )
	{
		if(isretaincolumnwidth == wyTrue)
		{
			//for getting the retained column width
			colname.SetAs(heading[counter]);
			width = GetColumnWidthFromFile(&dbname, &tblname, &colname);
		}
		
		memset(&gvcol, 0,sizeof(gvcol));
		
		gvcol.mask		= mask[counter];		// Set the mask for the sturcture  i.e. what kind of column in the grid.
		gvcol.fmt		= format[counter];		// Alignment
		gvcol.pszList	= listtype[counter];	// set the kind list in between
		gvcol.cx		 = (width <= 0)? cx[counter]:width;
		gvcol.text	= heading[counter];
		gvcol.nElemSize = elemsize[counter];
		gvcol.nListCount = elemcount[counter];
		gvcol.cchTextMax = strlen(heading[counter]);

		// if the column being entered is the collation column and the mysql version is < 4.1, then
		// we skip this column as column level collation is not present in v < 4.1
		if ((counter == COLLATION+1 || counter == CHARSET+1) && !IsMySQL41(m_tunnel, m_mysql))
			continue;

		// if the column being entered is last and the mysql version is < 4.1, then
		// we exit as the last column is for column level comments which is only supported
		// in v4.1
		if((counter==(num_cols-1)) && (IsMySQL41(m_tunnel, m_mysql)) == 0)
			break;

		wyBool result = IsMySQL41(m_tunnel, m_mysql);
		if((result == wyTrue) && (stricmp(heading[counter] , "Binary?") == 0))
			continue;
		CustomGrid_InsertColumn(m_hwndgrid, &gvcol);
	}

	// get how many rows will come in the display grid and by default add those many rows
	// this function is implemented by derived class because somebody might want to add, some class might not

	SetFont();

	CreateInitRows();
	
	return;
}


wyBool
TableMakerBase::ValidateOnBeginLabelEdit(wyUInt32 row, wyUInt32 col)
{
	// For a column to be editable, it has to satisfy the following two conditions:
	// 1.) First column can be editable only if the previous row has data. 0th row is an exception
	// 2.) 1+n column can only be edited if there is data in the first column. So you will have to enter name first to change other values
	switch(col)
	{
	case ZERO:
		if (row == 0 )
			break;		// First row edit anyway.
		else
		{
			if(CustomGrid_GetItemTextLength(GetGrid(), row-1, col) == 0 )
				wyFalse;
		}
		break;

	default:
		if(CustomGrid_GetItemTextLength(GetGrid(), row, 0) == 0)
			return wyFalse;
	}

	if(col == DATATYPE)
	{
		CustomGrid_SetBoolValue(GetGrid(), row, CHARSET, "");
		CustomGrid_SetBoolValue(GetGrid(), row, COLLATION, "");
	}

	return wyTrue;
}

wyBool
TableMakerBase::ValidateOnEndLabelEdit(WPARAM wParam, LPARAM lParam)
{
	wyUInt32		index = 0, addcol = 0;
	wyUInt32		row = LOWORD(wParam);
	wyUInt32		col = HIWORD(wParam);
	wyBool			checked;
    wyInt32         issuccess = 0;
    HWND            hwndgrid = GetGrid();
    wyBool          ismysql41 = IsMySQL41(GetTunnel(), GetMySQL());
	
	if(ismysql41 == wyTrue)
		index = 1;							// this operation needs to be commented
    else
        addcol = 1;

	/* Basically we do certain validation in this function:

	   The following validations are done:
	   1. If a user has CHECKED primary then we select AUTOCINCREMENT column and NOT NULL column as a column cannot be primary and NOT AUTOINCR and NOTNULL and we make NOT NULL readonly.
	   2. If a user has UNCHECKED primary, then we unselect AUTOCINCREMENT and make NOT NULL column non-read only.
	   3. If a user has selected DATATYPE then we do some validation depending upon selected datatype.
	   4. If a user has selected AUTOINCREMENT column then we select PRIMARY KEY too as there can be no AUTOINCREMENT without PRIMARY KEY.
	   5. If a user has selected AUTOINCREMENT column then DEFAULT VALUE column is disabled
	   6. If a user has selected DEFAULT VALUE column then AUTOINCREMENT column is disabled
	*/
    if(col == PRIMARY)
	{
		if(stricmp((wyChar*)lParam, GV_TRUE)== 0) 
		{
            // user has checked PRIMARY KEY.
			CustomGrid_SetBoolValue(hwndgrid, row, NOTNULL + addcol, GV_TRUE);
			CustomGrid_SetColumnReadOnly(hwndgrid, row, NOTNULL + addcol, wyTrue);
			m_isprimarydefined	= wyTrue;
		} 
		else if (stricmp((CHAR*)lParam, GV_FALSE) == 0) 
		{
			// user has unchecked PRIMARY KEY option
			//CustomGrid_SetBoolValue(GetGrid(), row, AUTOINCR+index, GV_FALSE);
			CustomGrid_SetColumnReadOnly(hwndgrid, row, NOTNULL + addcol, wyFalse);
			m_isprimarydefined	= wyFalse;
		}
	}

	else if(col == DATATYPE)
		SetValidation(row, (wyChar*)lParam);

	else if(col == AUTOINCR+ addcol)
	{	
		if(m_autoincpresent == wyFalse)
		{
			CustomGrid_SetBoolValue(hwndgrid, row, AUTOINCR + addcol, GV_TRUE);

			//If a user has selected AUTOINCREMENT column then DEFAULT VALUE column is disabled
			CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyTrue);
			m_autoincpresent = wyTrue;

		}
		else
		{
			checked = CustomGrid_GetBoolValue(hwndgrid,row, col);
			if(checked == wyFalse)
			{
				m_autoincpresent = wyFalse;
				CustomGrid_SetColumnReadOnly(hwndgrid, row, DEFVALUE, wyFalse);
			}
			/*else
				CustomGrid_SetBoolValue(hwndgrid, row, AUTOINCR + addcol, GV_FALSE);*/
		}
		
	}
    if(ismysql41 == wyTrue && col == CHARSET)
    {
        issuccess = CustomGrid_DeleteListContent(hwndgrid, COLLATION);
        CustomGrid_SetText(hwndgrid, row, COLLATION, "");
    }

	// if default value is present then disable autoincrement column 
	if(col == DEFVALUE)
	{
		if(strlen((wyChar*)lParam) !=  0)
			CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR + addcol, wyTrue);
		else
			CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR + addcol, wyFalse);
	}

	return wyTrue;
}

void
TableMakerBase::HandleColDefaultValidation(HWND &hwndgrid, wyUInt32 &row, wyChar *data)
{
    wyInt32     len = CustomGrid_GetItemTextLength(hwndgrid, row, DATATYPE);
    wyWChar     *datatype = NULL;
    wyString    tempstr, warning;
    wyBool      typeflag = wyFalse;

    if(data)
        tempstr.SetAs(data);
    
    if(len != 0)
    {
        datatype = AllocateBuffWChar(len + 1);
        CustomGrid_GetItemText(hwndgrid, row, DATATYPE, datatype);
    }
    else
        return;

    if(tempstr.CompareI("''") == 0)
    {
        if(wcsicmp(datatype, L"varchar") == 0 || wcsicmp(datatype, L"char") == 0  || 
        wcsicmp(datatype, L"varbinary") == 0 || wcsicmp(datatype, L"enum") == 0 || wcsicmp(datatype, L"set") == 0)
            typeflag = wyTrue;            
    }
    else
        return;

    if(typeflag == wyFalse)         
    {
        tempstr.SetAs(datatype);
        warning.SetAs(_("Default empty string('') is not supported for the datatype "));
        warning.AddSprintf("\"%s\"", tempstr.GetString());
        MessageBox(m_hwnd, warning.GetAsWideChar(), L"Warning", MB_ICONWARNING);
    }
}
wyBool
TableMakerBase::SetValidation(wyUInt32 row, wyChar* datatype)
{
	wyUInt32	index = 0;
    HWND        hwndgrid = GetGrid();
	wyBool		isvarbinarydatatype = wyFalse, isbinarydatatype = wyFalse;

	if(IsMySQL41(GetTunnel(), GetMySQL()) == wyFalse)
		index = 1;
    else
        index = 0;

	if ((stricmp (datatype , "tinyint" ) == 0 ) )
	   return HandleTinyIntValidation(hwndgrid, row, index);
	
	if ((stricmp (datatype , "bit" ) == 0 ) )
        return HandleBitValidation(hwndgrid, row, index);

	if ((stricmp (datatype , "bool" ) == 0 ) ||
		 (stricmp (datatype , "boolean" ) == 0 ) )

         return HandleBoolAndBooleanValidation(hwndgrid, row, index);
		
	if ((stricmp (datatype , "smallint" ) == 0 ) )
        return HandleSmallIntValidation(hwndgrid, row, index);
	
	if ((stricmp (datatype , "mediumint") == 0 ) )
        return HandleMediumIntValidation(hwndgrid, row, index);
	
	if ((stricmp (datatype , "int") == 0 ) )
        return HandleIntValidation(hwndgrid, row, index);
	
	if ((stricmp (datatype , "integer") == 0 ) )
        return HandleIntegerValidation(hwndgrid, row, index);
	
	if ((stricmp (datatype , "bigint") == 0 ) )
        return HandleBigIntValidation(hwndgrid, row, index);

	if ((stricmp (datatype , "float") == 0 ) )
        return HandleFloatValidation(hwndgrid, row, index);
	
	if ((stricmp (datatype , "double") == 0 ) )
        return HandleDoubleValidation(hwndgrid, row, index);
	
	if (((stricmp ((datatype ), "real" ) == 0 ) ) ) 
        return HandleRealValidation(hwndgrid, row, index);
	
	if (((stricmp ((datatype ), "decimal" ) == 0 ) ) ) 
        return HandleDecimalValidation(hwndgrid, row, index);
	
	if (((stricmp ((datatype ), "numeric" ) == 0 ) ) ) 
        return HandleNumericValidation(hwndgrid, row, index);
	
	if (((stricmp ((datatype ), "date" ) == 0 ) ) ) 
        return HandleDateValidation(hwndgrid, row, index);
	
	if (((stricmp ((datatype ), "datetime" ) == 0 ) ) )
        return HandleDataTypeValidation(hwndgrid, row, index);
	
	if (((stricmp ((datatype ), "timestamp" ) == 0 ) ) ) 
        return HandleTimeStampValidation(hwndgrid, row, index);
	
	if (((stricmp ((datatype ), "time" ) == 0 ) ) ) 
        return HandleTimeValidation(hwndgrid, row, index);
	
	if (stricmp (datatype , "year" ) == 0 )
        return HandleYearValidation(hwndgrid, row, index);

    if (stricmp (datatype , "binary" ) == 0) 
        isbinarydatatype = wyTrue;
	
	if (stricmp (datatype , "char" ) == 0 || isbinarydatatype == wyTrue)
        return HandleCharAndBinaryValidation(hwndgrid, row, index, isbinarydatatype);
	
	if(stricmp (datatype , "varbinary" ) == 0)
		isvarbinarydatatype = wyTrue;

	if((stricmp (datatype , "varchar" ) == 0 ) || isvarbinarydatatype == wyTrue)
        return HandleVarCharAndVarbinaryValidation(hwndgrid, row, index, isvarbinarydatatype);
	
	if ((stricmp (datatype , "tinyblob" )  == 0 )  ||
		 (stricmp (datatype , "tinytext" )  == 0 )  ||
		 (stricmp (datatype , "mediumblob" )  == 0 ) ||
		 (stricmp (datatype , "mediumtext" )  == 0  )||
		 (stricmp (datatype , "longblob" )  == 0 ) ||
		 (stricmp (datatype , "longtext" )  == 0 ) )
         return HandleAllBlobTextValidation(hwndgrid, row, index, datatype);
	

	if ((stricmp (datatype , "blob" ) == 0 )  ||
		 (stricmp (datatype , "text" ) == 0) )
         return HandleBlobTextValidation(hwndgrid, row, index, datatype);
	
	if ((stricmp (datatype , "set" )  == 0 )  ||
		 (stricmp (datatype , "enum" )  == 0 ) )
         return HandleSetEnumValidation(hwndgrid, row, index);
	
	return wyTrue;
}

wyBool
TableMakerBase::HandleTinyIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED + index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL + index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);

	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{	
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE );
	}

    return wyTrue;
}

wyBool
TableMakerBase::HandleBitValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue);
    CustomGrid_SetBoolValue(hwndgrid, row, AUTOINCR + index, GV_FALSE);  
	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

    return wyTrue;
}

wyBool
TableMakerBase::HandleBoolAndBooleanValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);
	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE );
	}
	CustomGrid_SetText(hwndgrid, row, LENGTH, "");

	return wyTrue;

}

wyBool
TableMakerBase::HandleSmallIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
    CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);
	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}
    
    return wyTrue;
}

wyBool
TableMakerBase::HandleMediumIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);
	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

	return wyTrue;
}

wyBool
TableMakerBase::HandleIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);
	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

	return wyTrue;
}

wyBool
TableMakerBase::HandleIntegerValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);
	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

	return wyTrue;
}

wyBool
TableMakerBase::HandleBigIntValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);

	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

	return wyTrue;
}

wyBool
TableMakerBase::HandleFloatValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);

	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

	return wyTrue;
}

wyBool
TableMakerBase::HandleDoubleValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);

	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

	return wyTrue;
}

wyBool
TableMakerBase::HandleRealValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyFalse);

	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED, GV_FALSE);
	return wyTrue;
}

wyBool
TableMakerBase::HandleDecimalValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);

	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}
		
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE);
	return wyTrue;
}

wyBool
TableMakerBase::HandleNumericValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);

	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE);
	return wyTrue;
}
wyBool
TableMakerBase::HandleDateValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);

	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}

	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyTrue);
	CustomGrid_SetText			(hwndgrid, row, LENGTH, "");
	return wyTrue;
}	

wyBool
TableMakerBase::HandleDataTypeValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);

	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyTrue);
	CustomGrid_SetText			(hwndgrid, row, LENGTH, "");
	return wyTrue;
}	

wyBool
TableMakerBase::HandleTimeStampValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);

	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	return wyTrue;
}

wyBool
TableMakerBase::HandleTimeValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);

	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyTrue);
	CustomGrid_SetText			(hwndgrid, row, LENGTH, "");
	return wyTrue;
}	

wyBool
TableMakerBase::HandleYearValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse);

	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue);
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE);
	}
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse);
	return wyTrue;
}

wyBool
TableMakerBase::HandleCharAndBinaryValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index, wyBool isbinary)
{
    if(isbinary == wyTrue)
    {
        CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
	    CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
    }
    else
    {
        CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyFalse);
	    CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyFalse);
    }
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse );

	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyFalse );

	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse );
	return wyTrue;
}

wyBool
TableMakerBase::HandleVarCharAndVarbinaryValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index, wyBool isvarbinary)
{
	if(isvarbinary == wyTrue)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
		CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
	}
	else
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyFalse);
		CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyFalse);
	}
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse );
    
	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyFalse );

	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse );
	return wyTrue;
}

wyBool
TableMakerBase::HandleAllBlobTextValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index, wyChar *datatype)
{
	// makes the charset and collation column readonly for the following datatype
	if ((stricmp (datatype , "tinytext" )  == 0 )  || (stricmp (datatype , "mediumtext" )  == 0 ) || (stricmp (datatype , "longtext" )  == 0 ))
{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyFalse);
		CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyFalse);
	}
	else
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
		CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
	}
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, PRIMARY, GV_FALSE);
	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue );
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE );
	}
	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyTrue );
	CustomGrid_SetText			(hwndgrid, row, LENGTH, "" );
	return wyTrue;
}

wyBool
TableMakerBase::HandleBlobTextValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index, wyChar *datatype)
{
	// makes the charset and collation column readonly for the "text" datatype
	if((stricmp (datatype, "text") == 0))
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyFalse);
		CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyFalse);
	}
	else
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyTrue);
		CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyTrue);
		CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyTrue );
		CustomGrid_SetText			(hwndgrid, row, LENGTH, "" );
	}
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyTrue);
	CustomGrid_SetBoolValue     (hwndgrid, row, PRIMARY, GV_FALSE);

	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue );
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE );
	}

	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse );
	return wyTrue;
}

wyBool
TableMakerBase::HandleSetEnumValidation(HWND &hwndgrid, wyUInt32 &row, wyUInt32 &index)
{
	CustomGrid_SetColumnReadOnly(hwndgrid, row, CHARSET, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, COLLATION, wyFalse);
	CustomGrid_SetColumnReadOnly(hwndgrid, row, PRIMARY, wyFalse );

	if(IsMySQL41(m_tunnel, m_mysql) == wyFalse)
	{
		CustomGrid_SetColumnReadOnly(hwndgrid, row, BINARY, wyTrue );
		CustomGrid_SetBoolValue     (hwndgrid, row, BINARY, GV_FALSE );
	}

	CustomGrid_SetColumnReadOnly(hwndgrid, row, UNSIGNED+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, UNSIGNED+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, AUTOINCR+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, AUTOINCR+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, ZEROFILL+ index, wyTrue );
	CustomGrid_SetBoolValue     (hwndgrid, row, ZEROFILL+ index, GV_FALSE );
	CustomGrid_SetColumnReadOnly(hwndgrid, row, LENGTH, wyFalse );
	m_isenumorset = wyTrue;

	return wyTrue;
}

wyUInt32
TableMakerBase::InsertRowInBetween()
{
    wyUInt32        ret=0;
    wyUInt32        curselrow=0;

    // When a user asks SQLyog to insert a row, it will always insert above the currently selected row.

    // First get the current selection
	ret = CustomGrid_GetCurSelection(GetGrid());
    //pCTableMaker->currow = LOWORD(ret);
    curselrow = LOWORD(ret);

    if(!curselrow)
        ret=CustomGrid_InsertRowInBetween(GetGrid(), 0);
    else
	    ret=CustomGrid_InsertRowInBetween(GetGrid(), curselrow);

    CustomGrid_SetCurSelection(GetGrid(), ret, 0);
    ::SetFocus(GetGrid());

    return ret;
}

void
TableMakerBase::HandleWMCommand(WPARAM wParam, LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
	case IDOK:
		ProcessOk();
		break;

	case IDDELETE:
        ProcessDelete();
        break;

	case IDINSERT:
        ProcessInsert();
        break;

	case IDC_OTHERPROP:
        ProcessAdvanceProperties();
		break;

	case IDCANCEL:
		ProcessCancel();
		break;

	case IDC_SHOWPREVIEW:
		ShowPreview();
		break;
	
	case IDC_HIDECOLUMNS:
		HideCharsetAndCollation();
		break;
	}
}

void 
TableMakerBase::SetFont()
{
	
	wyWChar	    *lpfileport = 0;
	wyWChar	    directory[MAX_PATH + 1] = {0};
	wyString	fontnamestr, dirstr;
	LOGFONT	    datafont = {0};
	wyInt32	    px, height;
	HDC		    hdc;
	HWND		hwnd;
	hwnd	=	GetGrid();

	VERIFY(hdc = GetDC(GetParent(hwnd)));

	height = GetDeviceCaps( hdc, LOGPIXELSY );

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	
	dirstr.SetAs(directory);
	// fill with default data.
	FillEditFont(&datafont, hwnd);
	
	wyIni::IniGetString(DATAFONT, "FontName", "Courier New", &fontnamestr, dirstr.GetString());
	
	wcscpy(datafont.lfFaceName, fontnamestr.GetAsWideChar());

	px = wyIni::IniGetInt(DATAFONT, "FontSize", 9, dirstr.GetString()); 	
	
    datafont.lfHeight = (wyInt32)((- px) * height / 72.0);

	datafont.lfWeight = wyIni::IniGetInt(DATAFONT, "FontStyle", 0, dirstr.GetString()); 	
	datafont.lfItalic = wyIni::IniGetInt(DATAFONT, "FontItalic", 0, dirstr.GetString()); 	
	datafont.lfCharSet = wyIni::IniGetInt(DATAFONT, "FontCharSet", DEFAULT_CHARSET, dirstr.GetString()); 
	
	CustomGrid_SetFont(hwnd, &datafont);
	
	//post 8.01
	//InvalidateRect(hwnd, NULL, FALSE);

}

//Filtering based on selected charset
void
TableMakerBase::FilterCollationColumn(HWND gridhwnd, wyInt32 row, wyInt32 charsetlen)
{
    wyWChar         *selcharset = NULL, *relcollation = NULL;
    wyString        collation, query, selcharsetstr;
    MYSQL_RES       *myres = NULL;
    MYSQL_ROW       myrow;
    MDIWindow       *wnd = GetActiveWin();
    
    selcharset  = AllocateBuffWChar(charsetlen + 1);

	CustomGrid_GetItemText(gridhwnd, row, CHARSETCOL, selcharset);
    CustomGrid_DeleteListContent(gridhwnd, COLLATIONCOL);
    selcharsetstr.SetAs(selcharset);
    
    free(selcharset);

    query.SetAs("show collation");
    myres = ExecuteAndGetResult(wnd, m_tunnel, m_mysql, query);
    if(!myres)
	{
        ShowMySQLError(gridhwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return;
	}
    while(myrow = wnd->m_tunnel->mysql_fetch_row(myres))
    {
        collation.SetAs(myrow[0]);
        
        // Add the relevent Items
        if((relcollation = wcsstr(collation.GetAsWideChar(), selcharsetstr.GetAsWideChar())) != NULL)
        {
            if(collation.GetCharAt(charsetlen) == '_')
                CustomGrid_InsertTextInList(gridhwnd, COLLATIONCOL, collation.GetAsWideChar());
        }
    }
    //if(CustomGrid_FindTextInList(gridhwnd, COLLATIONCOL, L"[default]") != -1)
    CustomGrid_InsertTextInList(gridhwnd, COLLATIONCOL, TEXT(STR_DEFAULT));
        
    wnd->m_tunnel->mysql_free_result(myres);

    return;
}

void
TableMakerBase::InitCollationCol(HWND gridhwnd)
{
    wyString        collation, query;
    MYSQL_RES       *myres = NULL;
    MYSQL_ROW       myrow;
    MDIWindow       *wnd = GetActiveWin();

    query.SetAs("show collation");
    myres = ExecuteAndGetResult(wnd, m_tunnel, m_mysql, query);
     if(!myres)
	{
        ShowMySQLError(gridhwnd, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return;
	}
    while(myrow = wnd->m_tunnel->mysql_fetch_row(myres))
    {
        collation.SetAs(myrow[0]);
        CustomGrid_InsertTextInList(gridhwnd, COLLATIONCOL, collation.GetAsWideChar());
    }
    if(CustomGrid_FindTextInList(gridhwnd, COLLATIONCOL, TEXT(STR_DEFAULT)) == -1)
        CustomGrid_InsertTextInList(gridhwnd, COLLATIONCOL, TEXT(STR_DEFAULT));

    wnd->m_tunnel->mysql_free_result(myres);

    return;
}

//for hiding the charset and collation
void 
TableMakerBase::HideCharsetAndCollation()
{	
	wyInt32		curselcol;

	CustomGrid_ApplyChanges(GetGrid());
	//CustomGrid_ShowScrollBar(GetGrid(), SB_HORZ, wyFalse);


	if(SendMessage(GetDlgItem(m_hwnd, IDC_HIDECOLUMNS), BM_GETCHECK, 0,0) == BST_CHECKED)
	{		
		CustomGrid_ShowOrHideColumn(GetGrid(), CHARSETCOL, wyFalse);
		CustomGrid_ShowOrHideColumn(GetGrid(), COLLATIONCOL, wyFalse);

		//if the current selection is in charset/collation column 
		//then after hiding we will set the selection to the comment column 
		curselcol = CustomGrid_GetCurSelCol(GetGrid());
		if(curselcol == CHARSETCOL || curselcol == COLLATIONCOL)
			CustomGrid_SetCurSelCol(GetGrid(), COMMENT, wyFalse);
	}
	else
	{
		CustomGrid_ShowOrHideColumn(GetGrid(), CHARSETCOL, wyTrue);
		CustomGrid_ShowOrHideColumn(GetGrid(), COLLATIONCOL, wyTrue);
	}
}

void
TableMakerBase::InitCharsetCol(HWND gridhwnd)
{
    wyString        charset, query;
    MYSQL_ROW       myrow;
    MDIWindow       *wnd = GetActiveWin();

	//only once we are executing the query.
	//if it is not NULL means if we already executed the query then we are reusing the resultset
	if(m_charsetres == NULL)
	{
		query.SetAs("show charset");
		m_charsetres = ExecuteAndGetResult(wnd, m_tunnel, m_mysql, query);
		if(!m_charsetres)
		{
			ShowMySQLError(gridhwnd, m_tunnel, m_mysql, query.GetString());
			return;
		}
	}
	else
		wnd->m_tunnel->mysql_data_seek(m_charsetres, 0);

    while(myrow = wnd->m_tunnel->mysql_fetch_row(m_charsetres))
    {
        charset.SetAs(myrow[0]);
        CustomGrid_InsertTextInList(gridhwnd, CHARSETCOL, charset.GetAsWideChar());
    }

    if(CustomGrid_FindTextInList(gridhwnd, CHARSETCOL, TEXT(STR_DEFAULT)) == -1)
        CustomGrid_InsertTextInList(gridhwnd, CHARSETCOL, TEXT(STR_DEFAULT));  

    return;
}

/**
-Handle the enum dialog
-When user selects Enum/set datatype from Create/Alter table datatype combo
- When doubleclick/space-bar on Length coulmn
*/
wyBool			
TableMakerBase::HandleEnumColumn(HWND hwndgrid, wyInt32 row, wyInt32 col)
{	
	wyWChar		datatype[SIZE_256], *value = NULL;
	wyString	datatypestr, values;
	wyBool		isenum = wyFalse;
	ValueList   valuelist(m_mysql);
	wyInt32		lentxt = 0;
	
	CustomGrid_GetItemText(GetGrid(), row, DATATYPE, datatype);
	datatypestr.SetAs(datatype);
	if(stricmp(datatypestr.GetString(),"enum") == 0 || stricmp(datatypestr.GetString(),"set") == 0)
	{
		if(stricmp(datatypestr.GetString(),"enum") == 0)
			isenum = wyTrue;
		else 
			isenum = wyFalse;
		CustomGrid_SetColumnReadOnly(GetGrid(), row, LENGTH, wyTrue);
		lentxt = CustomGrid_GetItemTextLength(GetGrid(), row, LENGTH);
		value = AllocateBuffWChar(lentxt + 1);
		if(!value)
		{
			return wyFalse;
		}
		CustomGrid_GetItemText(GetGrid(), row, LENGTH, value);
		values.SetAs(value);
		free(value);
		valuelist.Create(GetGrid(), &values, isenum);
		CustomGrid_SetText(GetGrid(), row, LENGTH, values.GetString());
		
		if(values.CompareI(m_oldvalue.GetString()) != 0)
		{
			return wyTrue;
		}
	}
	return wyFalse;
}
