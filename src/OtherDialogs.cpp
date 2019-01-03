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


#include "ExportMultiFormat.h"
#include "FrameWindowHelper.h"
#include "MySQLVersionHelper.h"
#include "Global.h"
#include "MDIWindow.h"
#include "ExportMultiFormat.h"
#include "OtherDialogs.h"
#include "ClientMySQLWrapper.h"
#include "CommonHelper.h"
#include "FileHelper.h"
#include "DataType.h"
#include "GUIHelper.h"
#include "EditorFont.h"

#include <string>

#include "scintilla.h"
#include "scilexer.h"

#ifndef COMMUNITY
#include "SCIFormatter.h"
#endif

extern	PGLOBALS	pGlobals;

#define STYLE "<style type=\"text/css\">\r\n<!--\r\n"\
".toptext {font-family: verdana; color: #000000; font-size: 20px; font-weight: 600; width:550;  background-color:#999999; }\r\n"\
".normal {  font-family: Verdana, Arial, Helvetica, sans-serif; font-size: 11px; font-weight: normal; color: #000000}\r\n"\
".fieldheader {font-family: verdana; color: #000000; font-size: 12px; font-weight: 600; width:550;  background-color:#c0c0c0; }\r\n"\
".fieldcolumn {font-family: verdana; color: #000000; font-size: 10px; font-weight: 600; width:550;  background-color:#ffffff; }\r\n"\
".header {background-color: #ECE9D8;}\r\n"\
".headtext {font-family: verdana; color: #000000; font-size: 12; font-weight: 600; width:550;  background-color:#999999; }\r\n"\
"BR.page {page-break-after: always}\r\n"\
"//-->\r\n</style>\r\n"

#define REORDERCONFIRM _(L"You are about to reorder column(s) in the table-structure. While there is no handy one-query-method to do that in MySQL, this will be done in 11 steps: \n\n"\
L"1) Creating a temporary table with the reordered columns\n"\
L"2) Put a read lock in original table\n"\
L"3) Populating the temporary table with data from original table\n"\
L"4) Unlock the original table\n"\
L"5) Checking the number of rows in both the table\n"\
L"6) Drop the original table\n"\
L"7) Recreate the original table with the reordered columns\n"\
L"8) Copy all rows from temporary table into the original table\n"\
L"9) Compare the number of rows in both the tables\n"\
L"10) Add Foreign Relations (if any)\n"\
L"11) Drop the temporary table\n"\
L"\nWhile all possible precautions are taken to ensure proper duplication of data, this operation can result in loss of data.\n\n"\
L"Do you still want to continue?")

#define	PFERROR "Could not create Personal File. Possible reasons for error are -\n"\
"1.)Personal Folder does not exist in the current path of SQLyog.exe\n"\
"2.)Invalid characters were typed as Key Name\n"\
"3.)You have entered a key name which already exists"

#define	WRITESCHEMAMESSAGE(a,b,c,d)	{ sprintf(a,b,c);SetWindowText(d,a);UpdateWindow(d); }

#define CONSTRAINT	"CONSTRAINT "
#define	FOREIGNKEY	"FOREIGN KEY"

#define	RELDLGWD	450
#define	RELDLGHT    200


void AddStyledText(HWND hwnd, const wyChar *s, size_t len, wyInt32 attr)
{
	wyChar *buf;

	buf = new wyChar[len * 2];

	for(size_t i = 0; i < len; i++)
	{
		buf[i * 2] = s[i];
		buf[i * 2 + 1] = (wyChar)(attr);
	}

	SendMessage(hwnd, SCI_ADDSTYLEDTEXT, (len * 2), (LPARAM)buf);

	delete[] buf;
}


/*-----------------------------------------------------------------------------------------
	Implementation of advance properties dialog box.
------------------------------------------------------------------------------------------*/

// Constructor.
CAdvProp::CAdvProp()
{

}

// Destructor.
CAdvProp::~CAdvProp()
{

}

// this function creates the actual dialog box and also initializes some values of the class

wyBool
CAdvProp::Create(HWND hwndparent, Tunnel *tunnel, PMYSQL mysql, wyChar *database, wyChar *tbl)
{
	wyInt64	ret;

	m_hwndparent = m_hwndparent;
	m_mysql = mysql;
	m_tunnel = tunnel;
	m_db = database;
	m_tbl = tbl;
	
	//post 8.01
    //RepaintTabModule();

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_SHOWVALUE), hwndparent,
							CAdvProp::AdvPropDialogProc, (LPARAM)this);

	return wyTrue;
}

wyInt32 CALLBACK
CAdvProp::GVWndProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam)
{
	wyString    tblname("__adv_prop");

	switch(message)
	{
		case GVN_BEGINLABELEDIT:
        case GVN_PASTECLIPBOARDBEGIN:
			return 0;

		case GVN_SPLITTERMOVE:
			OnGridSplitterMove(hwnd, &tblname, wparam);
			return 1;
	}
	return 1;
}

// Callback dialog procedure for the window.

INT_PTR CALLBACK
CAdvProp::AdvPropDialogProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam)
{
	HICON hicon;
	CAdvProp *pcadvprop = (CAdvProp*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		return wyTrue;
		
	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/96-advanced-tab");
		return wyTrue;

	case WM_INITDLGVALUES:
		// we set some initial value.
		pcadvprop->m_hwnddlg = hwnd;
		pcadvprop->m_hwndlist = GetDlgItem(hwnd, IDC_ADVPROPLIST);
		SendMessage(pcadvprop->m_hwndlist, LVM_SETEXTENDEDLISTVIEWSTYLE, 
            (WPARAM)LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT,(WPARAM)LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
		ShowWindow(GetDlgItem(hwnd, IDC_REFRESH), SW_HIDE);
		pcadvprop->CreateListViewColumns();
		pcadvprop->FillData();
		ShowValueResize(hwnd, pcadvprop->m_hwndgrid);
		
		//Set icon for dialog	
		hicon = CreateIcon(IDI_TABLEPROP);
		SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hicon);
		
		break;

	case WM_SIZE:
		ShowValueResize(hwnd, pcadvprop->m_hwndgrid);
		break;

	case WM_PAINT:
		//draws the resize gripper at bottom-right corner of dialog
		DrawSizeGripOnPaint(hwnd);
		break;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO pMMI = (LPMINMAXINFO)lparam;
			pMMI->ptMinTrackSize.x = RELDLGWD;
			pMMI->ptMinTrackSize.y = RELDLGHT;
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wparam))
		{
		case IDOK:
		case IDCANCEL:
			VERIFY(yog_enddialog(hwnd, 1));
			break;
		}
	}
	return wyFalse;
}

HWND 
CAdvProp::InitGrid(HWND hwnd)
{
	HWND	m_hwndgrid; 
	RECT	rcok = {0};

	// calculate the window size using the ok button
	GetWindowRect(GetDlgItem(hwnd, IDOK), &rcok	);
	MapWindowPoints(NULL, hwnd,(LPPOINT)&rcok, 2);

	m_hwndgrid = CreateCustomGrid(hwnd, 10, 10, rcok.right - 10, rcok.top - 20, (GVWNDPROC)GVWndProc, (LPARAM)this);

	return m_hwndgrid;
}

// we create the column header for the listview control.
wyBool
CAdvProp::CreateListViewColumns()
{	
	wyString	title;
	wyInt32		width;
	RECT		rc;

	m_hwndgrid = InitGrid(m_hwnddlg);
	
	VERIFY(GetClientRect(m_hwndgrid, &rc));

	// we calculate the width of the first column. its approx 35% of the width.
	width  =(wyInt32)(.45*(rc.right-rc.left));

	GVCOLUMN		gvcol;
	VOID			*szlisttype[]	= { NULL };
	wyInt32			nelemsize[]		= { 0 };
	wyInt32			nelemcount[]	= { 0 };
	wyInt32			nmask[]			= { GVIF_TEXT };  
	wyInt32			cx[]			= { width };
	wyInt32			fmt[]			= { GVIF_LEFT };
	wyInt32			index			= 0;
	wyChar			*szheading[]	= {"Variable", "Values" };

	wyInt32			colwidth = 0;
	wyString		colname, dbname(RETAINWIDTH_DBNAME), tblname("__adv_prop");
	wyBool			isretaincolumnwidth = IsRetainColumnWidth();
	
	/* Give the details of columns to insert. */
	for(index=0; index<2 ; index++)
	{
		//Retaining user modified column width in preferences is checked or not
        if(isretaincolumnwidth == wyTrue)
		{
			//for getting the retained column width
			colname.SetAs(szheading[index]);
			colwidth = GetColumnWidthFromFile(&dbname, &tblname, &colname);
		}

		memset(&gvcol, 0, sizeof(gvcol));

		gvcol.mask			= nmask[0];
		gvcol.fmt			= fmt[0];
		gvcol.pszList		= szlisttype[0];
		gvcol.cx			= (colwidth > 0)?colwidth:cx[0];
		gvcol.text		    = szheading[index];
		gvcol.nElemSize		= nelemsize[0];
		gvcol.nListCount	= nelemcount[0];
		gvcol.cchTextMax	= strlen(szheading[index]);
        gvcol.uIsReadOnly   = wyTrue;

		CustomGrid_InsertColumn(m_hwndgrid, &gvcol);
	}

	title.AddSprintf(_(" Table Properties - '%s' in '%s'"), m_tbl, m_db);
	SetWindowText(m_hwnddlg, title.GetAsWideChar());
	return wyTrue;
}

// Fill the listview with values.

wyBool
CAdvProp::FillData()
{
	wyInt32			   numfields = 0, count, colcount;
	wyString		   query;
	MYSQL_RES		  *myres;	
	MYSQL_FIELD		  *myfield;	
	MYSQL_ROW		   myrow;

	query.AddSprintf("show table status from `%s` like '%s'", m_db, m_tbl);

    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!myres)
	{
		ShowMySQLError(m_hwnddlg, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}
	numfields = myres->field_count;
	myrow = m_tunnel->mysql_fetch_row(myres);

	//// we move the pointer to the row which contains the table value.
	//while(myrow = m_tunnel->mysql_fetch_row(myres))
	//	if(strcmp(myrow[0], m_tbl) == 0)
	//		break;

	if(myrow == NULL)
		return wyFalse;
	// now we move each field and insert text in the listview.
	for(count = 0; count < numfields; count++)
	{
		VERIFY((colcount = CustomGrid_InsertRow(m_hwndgrid)) != -1);
		myfield = m_tunnel->mysql_fetch_field(myres);
		CustomGrid_SetText(m_hwndgrid, count, 0, (LPSTR)myfield->name);
		CustomGrid_SetText(m_hwndgrid, count, 1, (LPSTR)(myrow[count])?(myrow[count]): STRING_NULL);
	}
	m_tunnel->mysql_free_result(myres);

	return wyTrue;
}

/*-----------------------------------------------------------------------------------------
	Implementation of show value dialog box.
	This dialog box is used to show properties like variables, status and processlist
------------------------------------------------------------------------------------------*/

// Constructor.
CShowValue::CShowValue()
{
}

// Destructor.
CShowValue::~CShowValue()
{
}

// this function creates the actual dialog box and also initializes some values of the class

wyBool
CShowValue::Create(HWND hwndparent, Tunnel * tunnel, PMYSQL mysql, wyInt32 valtype)
{
	wyInt64	ret;

	m_hwndparent    	= hwndparent;
	m_mysql		        = mysql;
	m_valtype		    = valtype;
	m_tunnel			= tunnel;
	m_prow				= -1;
	m_pcol				= -1;
	
    //post 8.01
	//RepaintTabModule();

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_SHOWVALUE), m_hwndparent,
							CShowValue::ShowValueDialogProc, (LPARAM)this);

	return wyTrue;
}

// Callback dialog procedure for the window.
INT_PTR CALLBACK
CShowValue::ShowValueDialogProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam)
{
	HICON hicon;
	CShowValue *pcshowvalue = (CShowValue *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		return 1;

	case WM_INITDLGVALUES:
		
		// we set some initial value.
		pcshowvalue->m_hwnddlg = hwnd;
		/* to fix the issue with XP in 2000k theme, we are changing all the listview to grid*/
		pcshowvalue->m_hwndgrid = pcshowvalue->InitGrid(hwnd);
		
		//Set icon for dialog	
		hicon = CreateIcon(IDI_MYSQL);
		SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hicon);

		//if it is warning dialog ,then no need to show refresh button 
		if(pcshowvalue->m_valtype == WARNINGS )
		{
			ShowWindow(GetDlgItem(hwnd, IDC_REFRESH), SW_HIDE);
	                SetInitPos(hwnd, SHOWVALUE_WARNING_SECTION);
		}
		else if(pcshowvalue->m_valtype == PROCESSLIST)
		{
			ShowWindow(GetDlgItem(hwnd, IDC_KILL), SW_SHOW);
#ifndef COMMUNITY
	if(GetActiveWin()->m_conninfo.m_isreadonly == wyTrue)
		EnableWindow(GetDlgItem(hwnd, IDC_KILL), FALSE); 
#endif
			SetInitPos(hwnd, SHOWVALUE_PROCESSLIST_SECTION);
		}
		else if(pcshowvalue->m_valtype == STATUS)
		{
			SetInitPos(hwnd, SHOWVALUE_STATUS_SECTION);
		}
		else if(pcshowvalue->m_valtype == VARIABLES)
		{
			SetInitPos(hwnd, SHOWVALUE_VARIABLE_SECTION);
		}

		pcshowvalue->FillData();
		
                //SetDialogPos(hwnd, SHOWVALUE_SECTION);

		ShowValueResize(hwnd, pcshowvalue->m_hwndgrid);

		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/132-environment-variables");
		return 1;		
	
	case WM_CONTEXTMENU:
		if(pcshowvalue->m_valtype == PROCESSLIST)
			CallBackContextMenu(hwnd, pcshowvalue, lparam );
		break;

	case WM_COMMAND:
		ShowValueDialogProcCommand(pcshowvalue, hwnd, wparam);
		break;
	
	case WM_PAINT:
		//draws the resize gripper at bottom-right corner of dialog
		DrawSizeGripOnPaint(hwnd);
	break;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO pMMI = (LPMINMAXINFO)lparam;
			pMMI->ptMinTrackSize.x = RELDLGWD;
			pMMI->ptMinTrackSize.y = RELDLGHT;
		}
		break;

	case WM_SIZE:
		ShowValueResize(hwnd, pcshowvalue->m_hwndgrid);
		break;

	case WM_DESTROY:
		if(pcshowvalue->m_valtype == STATUS)
			SaveInitPos(hwnd, SHOWVALUE_STATUS_SECTION);
		else if(pcshowvalue->m_valtype == WARNINGS )
			SaveInitPos(hwnd, SHOWVALUE_WARNING_SECTION);
		else if(pcshowvalue->m_valtype == PROCESSLIST)
			SaveInitPos(hwnd, SHOWVALUE_PROCESSLIST_SECTION);
		else if(pcshowvalue->m_valtype == VARIABLES)
			SaveInitPos(hwnd, SHOWVALUE_VARIABLE_SECTION);	
		break;
	}
	return 0;
}

HWND 
CShowValue::InitGrid(HWND hwnd)
{
	HWND	m_hwndgrid; 
	RECT	rcok = {0};

	// calculate the window size using the ok button
	GetWindowRect(GetDlgItem(hwnd, IDOK), &rcok	);
	MapWindowPoints(NULL, hwnd,(LPPOINT)&rcok, 2);

	m_hwndgrid = CreateCustomGrid(hwnd, 10, 10, rcok.right - 10, rcok.top - 20,
				                (GVWNDPROC)OtherDialogGridWndProc,(LPARAM)this);

	return m_hwndgrid;
}

wyInt32 CALLBACK
CShowValue::OtherDialogGridWndProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam)
{
	CShowValue *showvalue = (CShowValue *)CustomGrid_GetLongData(hwnd);;
	
	switch(message)
	{
		case GVN_BEGINLABELEDIT:
        case GVN_PASTECLIPBOARDBEGIN:
        case GVN_ENDLABELEDIT:
			return 0;

		case GVN_SPLITTERMOVE:
			showvalue->OnSplitterMove(wparam);	
			break;
	}
	return 1;
}

// Function creates the columns in the grid.
wyBool 
CShowValue::CreateColumnsWithProperLength(MYSQL_RES * result, PGVCOLUMN	gvcol, wyChar **heading)
{
	wyUInt32		k = 0;
	MYSQL_ROW		myrow;
	MYSQL_FIELD		*fields;	
	wyUInt32		count;
	wyString		myrowstr;

	wyInt32			width = 0;
	wyString		colname, dbname(RETAINWIDTH_DBNAME), tblname;
	wyBool			isretaincolumnwidth = IsRetainColumnWidth();	

    // create grid columns 
	VOID			*szlisttype[]	= {NULL};
	wyInt32			nelemsize[]		= {0};
	wyInt32			nelemcount[]	= {0};
	wyInt32			nmask[]			= {GVIF_TEXT};  
	wyInt32			cx[]			= {370};
	wyInt32			fmt[]			= {GVIF_LEFT};
	wyInt32			rowcount		= 0;
	wyInt32			index			= 0;
	/* Give the details of columns to insert. */
    
	rowcount = (wyUInt32)m_tunnel->mysql_num_rows(result);

	fields  = m_tunnel->mysql_fetch_fields(result);

	count = m_tunnel->mysql_field_count(*m_mysql);

	if(m_valtype == PROCESSLIST)
		tblname.SetAs("__show_processlist");
	else if(m_valtype == STATUS)
		tblname.SetAs("__show_status");
	else if(m_valtype == VARIABLES)
		tblname.SetAs("__show_variables");
	else if(m_valtype == WARNINGS)
		tblname.SetAs("__show_warnings");	
	
	while(k < count)
	{
		if(isretaincolumnwidth == wyTrue)
		{
			//for getting the resized column width
			colname.SetAs(heading[k]);
			width = GetColumnWidthFromFile(&dbname, &tblname, &colname);
		}

        gvcol->mask = nmask[0];
	    gvcol->fmt  = fmt[0];
	    gvcol->pszList = szlisttype[0];
	    gvcol->cx   = cx[0];
	    gvcol->text = heading[k];
	    gvcol->nElemSize = nelemsize[0];
	    gvcol->nListCount = nelemcount[0];
    	gvcol->cchTextMax = strlen(heading[k]);
        gvcol->uIsReadOnly = wyTrue;

		m_tunnel->mysql_data_seek(result, k);

		VERIFY(myrow = m_tunnel->mysql_fetch_row(result));
		
		//isretainedcolumnwidth is false or if it is not present the SQlite database we will calculate the width 
		gvcol->cx   = (width > 0)? width : GetColWidth(m_hwndgrid, &fields[k], k);

    	VERIFY((index = CustomGrid_InsertColumn(m_hwndgrid, gvcol)) != -1);

		k++;
	}

	m_tunnel->mysql_data_seek(result, 0);
	//myrow = m_tunnel->mysql_fetch_row(result);

	return wyTrue;
}

wyBool
CShowValue::CreateColumns(MYSQL_RES *myres)
{
	wyInt32				colcount = 0;
	wyString			temptext, myfieldstr, myrowstr;
	LONG				numfields;
	MYSQL_FIELD			*myfield;
	MYSQL_ROW			myrow;
	wyChar				**heading;
	wyBool				ismysql41 = ((GetActiveWin())->m_ismysql41);
	
	temptext.SetAs("E");
	numfields = myres->field_count;
	heading =(wyChar **)calloc(numfields, sizeof(wyChar *));
	
	while(myfield = m_tunnel->mysql_fetch_field(myres))
	{	
		myfieldstr.SetAs(myfield->name, ismysql41);

		*(heading + colcount) = (wyChar *)calloc(myfieldstr.GetLength() + 1, sizeof(wyChar));
		strcpy(*(heading + colcount), myfieldstr.GetString());
		colcount++;
	}

    GVCOLUMN		gvcol;
    memset(&gvcol, 0, sizeof(gvcol));
    wyInt32         rowcount = 0;

	
    if(CreateColumnsWithProperLength(myres, &gvcol, heading) == wyFalse)    
        return wyFalse;    

	rowcount = 0;

	 //in older version here first process id (show full processlist result set)
	//was missing at the time of adding result to grid		
	while(myrow = mysql_fetch_row(myres))
	{		
		VERIFY((colcount = CustomGrid_InsertRow(m_hwndgrid))!= -1);
		for(colcount = 0; colcount<numfields; colcount++)
		{	
			if(myrow[colcount])
			{
				myrowstr.SetAs(myrow[colcount], ismysql41);
				CustomGrid_SetText(m_hwndgrid, rowcount, colcount,
								  (LPSTR)(myrowstr.GetString() != NULL)?(myrowstr.GetString()):(STRING_NULL));
			}
		}
		
		rowcount++;
	}

	/// if still the row count is zero then disable the delete key & Edit key
	if(m_valtype ==  PROCESSLIST)
	{	if(CustomGrid_GetRowCount(m_hwndgrid) == 0)
			EnableWindow(GetDlgItem(m_hwnddlg, IDC_KILL), FALSE); 
		else
			EnableWindow(GetDlgItem(m_hwnddlg, IDC_KILL), TRUE); 
#ifndef COMMUNITY
	if(GetActiveWin()->m_conninfo.m_isreadonly == wyTrue)
		EnableWindow(GetDlgItem(m_hwnddlg, IDC_KILL), FALSE); 
#endif
	}

	// free the memory allocated
	for(colcount--; colcount >= 0; colcount--)
	{
		if(*(heading + colcount))
			free(*(heading + colcount));

		*(heading + colcount) = NULL;
	}

	if(heading)
		free(heading);

	return wyTrue;
}

// Fill the listview with values.

wyBool
CShowValue::FillData()
{
	wyString	 query;
	MYSQL_RES	*myres;

	switch(m_valtype)
	{
	case STATUS:
		SetWindowText(m_hwnddlg, _(L" Server Status"));
		query.Add("show status");
		break;

	case VARIABLES:
		SetWindowText(m_hwnddlg, _(L" Server Variables"));
		query.Add("show variables");
		break;

	case PROCESSLIST:
		SetWindowText(m_hwnddlg, _(L" Server Processlist"));
		if(IsNewMySQL(m_tunnel, m_mysql))
			query.Add("show full processlist");
		else
			query.Add("show processlist");
		break;

	case WARNINGS:
		SetWindowText(m_hwnddlg, _(L" Warnings"));
		query.Add("show warnings");
		break;
	}

    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!myres)
	{
		ShowMySQLError(m_hwnddlg, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}
	CreateColumns(myres);
	m_tunnel->mysql_free_result(myres);

	return wyTrue;
}

// Function to kill the selected process.
// Before that it checks whether it is the same window process and if wyTrue then it dosnt kill it.

wyBool
CShowValue::KillProcess()
{
	MYSQL_RES	*res;
	wyInt32		intprocess;
	wyWChar		process[512];
	wyString	query;
	wyString	processstr;

	if(m_prow == -1)
		return wyFalse;

	CustomGrid_GetItemText(m_hwndgrid, m_prow, 0, process);
	intprocess = _wtoi(process);

	processstr.SetAs(process);

	if(intprocess ==(wyInt32)(*m_mysql)->thread_id)
	{
		yog_message(m_hwnddlg, _(L"Cannot kill the process. \nIt is the current connections thread id"), 
                                pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);
		return wyFalse;
	}
	query.AddSprintf("kill %s", processstr.GetString());

    res = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!res && m_tunnel->mysql_affected_rows(*m_mysql)== -1)
	{
		ShowMySQLError(m_hwndparent, m_tunnel, m_mysql, query.GetString());

		return wyFalse;
	}
	m_tunnel->mysql_free_result(res);

	CustomGrid_DeleteRow(m_hwndgrid, m_prow, wyTrue);

		if(CustomGrid_GetRowCount(m_hwndgrid) == 0)
			EnableWindow(GetDlgItem(m_hwnddlg, IDC_KILL), FALSE); 
		else
			EnableWindow(GetDlgItem(m_hwnddlg, IDC_KILL), TRUE); 
#ifndef COMMUNITY
	if(GetActiveWin()->m_conninfo.m_isreadonly == wyTrue)
		EnableWindow(GetDlgItem(m_hwnddlg, IDC_KILL), FALSE); 
#endif

	m_prow = -1;

	return wyTrue;
}

wyBool
CShowValue::OnRefreshClick()
{
	wyInt32 initcol, initrow;

	//getting initial row & column
	initcol = CustomGrid_GetInitCol(m_hwndgrid);
	initrow = CustomGrid_GetInitRow(m_hwndgrid);

	//delete all rows & columns
	VERIFY(CustomGrid_DeleteAllRow(m_hwndgrid));
	VERIFY(CustomGrid_DeleteAllColumn(m_hwndgrid));

	FillData();

	//setting initial row & column
	CustomGrid_SetInitCol(m_hwndgrid, initcol, wyTrue);
	CustomGrid_SetInitRow(m_hwndgrid, initrow, wyTrue);

	return wyTrue;
}

//If User resizes the column in grid
wyBool
CShowValue::OnSplitterMove(WPARAM wparam)
{
	wyString tblname;

	//checking the type of the dialog
	if(m_valtype == PROCESSLIST)
		tblname.SetAs("__show_processlist");
	else if(m_valtype == STATUS)
		tblname.SetAs("__show_status");
	else if(m_valtype == VARIABLES)
		tblname.SetAs("__show_variables");
	else if(m_valtype == WARNINGS)
		tblname.SetAs("__show_warnings");

	//saving the width of the  resized column
	OnGridSplitterMove(m_hwndgrid, &tblname, wparam);

	return wyTrue;
}
/*------------------------------------------------------------------------------------------
	Implementation of CSchema class.
	Thorough this we create a simple schema for the selected tables in the database
------------------------------------------------------------------------------------------*/

// constructor
CSchema::CSchema()
{
	m_mysql			= NULL;
	m_tunnel		= NULL;
	m_hwndparent	= NULL;
	m_hfile			= NULL;
	m_flg			= wyFalse;
	m_change		= wyFalse;
	
}

// destructore
CSchema::~CSchema()
{
	
}

wyBool
CSchema::Build(CQueryObject *pcqueryobject, Tunnel * tunnel, PMYSQL mysql, HTREEITEM hitem)
{
	wyInt64		ret;
	TVITEM		tvi;
    wyWChar      dbname[SIZE_512] = {0};
	
	m_mysql = mysql;
	m_tunnel = tunnel;
	m_hwndparent = pcqueryobject->m_hwnd;

	tvi.mask		= TVIF_TEXT;
	tvi.hItem		= hitem;
	tvi.cchTextMax	= SIZE_512-1;
	tvi.pszText		= dbname;

	VERIFY(TreeView_GetItem(m_hwndparent, &tvi));

    if(dbname)
        m_db.SetAs(dbname);

    //post 8.01
	//RepaintTabModule();

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_SCHEMA), m_hwndparent,
						CSchema::SchemaDialogProc,(LPARAM)this);

	return wyTrue;
}

// the dialog proc for the dialog box.
INT_PTR CALLBACK
CSchema::SchemaDialogProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam)
{
	CSchema  *pcschema = (CSchema *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		return wyTrue;

	case WM_INITDLGVALUES:
		// we set some initial value.
		VERIFY(pcschema->m_hwnddlg = hwnd);
		VERIFY(pcschema->m_hwndtables = GetDlgItem(hwnd, IDC_TABLENAME));
		pcschema->FillData();
		SendMessage(pcschema->m_hwndtables, LB_SETSEL, TRUE, -1);
		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/119-database-schema");
		return wyTrue;

	case UM_CREATESCHEMA:
		{
			wyBool ret = pcschema->Create(hwnd);
			if(ret)
            {
				yog_message(hwnd, _(L"Schema generation successful"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
				ShellExecute(NULL, TEXT("open"), (wyWChar*)pcschema->m_file.GetAsWideChar(), NULL, NULL, SW_SHOWNORMAL);
				yog_enddialog(hwnd, 1);
			} else {
				SetWindowText(GetDlgItem(hwnd, IDC_MESSAGE), _(L"Error while generating schema.")); 
			}
		}
		break;

	case WM_COMMAND:
		SchemaDialogProcCommand(hwnd, pcschema, wparam);
		break;
	}
	
	return wyFalse;
}

// function to initialize the listview with the table names.
wyBool
CSchema::FillData()
{
	wyInt32			count, width;
	wyString		title;
	wyString		query;
	MYSQL_RES		*myres;
	MYSQL_ROW		myrow;
	wyString		myrowstr;	

    title.Sprintf(_(" Create Schema For - '%s'"), m_db.GetString());
	SetWindowText(m_hwnddlg, title.GetAsWideChar());

	count = PrepareShowTable(m_tunnel, m_mysql, m_db, query);

	SetCursor(LoadCursor(NULL, IDC_WAIT));

    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!myres)
		goto cleanup;
	
	while(myrow = m_tunnel->mysql_fetch_row(myres))
	{
		myrowstr.SetAs(myrow[0], (GetActiveWin())->m_ismysql41);
		VERIFY(SendMessage(m_hwndtables, LB_ADDSTRING, 0,(LPARAM)myrowstr.GetAsWideChar())!=LB_ERR);
	}

	 //sets the maximum width for the list box	
    width = SetListBoxWidth(m_hwndtables);

	// Sets the H-scroll for listbox
	SendMessage(m_hwndtables, (UINT)LB_SETHORIZONTALEXTENT, (WPARAM)width + 5, (LPARAM)0);

	m_tunnel->mysql_free_result(myres);

	SetCursor(LoadCursor(NULL, IDC_ARROW));

	return wyTrue;

cleanup:
	ShowMySQLError(m_hwndparent, m_tunnel, m_mysql, query.GetString());
	EnableWindow(GetDlgItem(m_hwnddlg, ID_SELECTALL), wyFalse);
	EnableWindow(GetDlgItem(m_hwnddlg, ID_UNSELECT), wyFalse);
	EnableWindow(GetDlgItem(m_hwnddlg, IDOK), wyFalse);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return wyFalse;

}

void 
CSchema::WriteSchemaMessage(wyChar *format, wyChar *val, HWND hwnd)
{
    wyString msg;

    msg.Sprintf(format, val);
    SetWindowText(hwnd, msg.GetAsWideChar());
    UpdateWindow(hwnd);
    return;
}

// This function is called to create the schema. it interanly calls functions
// to create the schema based on the database name. 

wyBool
CSchema::Create(HWND hwnd)
{
	wyBool	ret;
	MDIWindow	*wnd = GetActiveWin();
	
	m_hfile = CreateFile((LPCWSTR)m_file.GetAsWideChar(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
							 FILE_ATTRIBUTE_NORMAL, NULL);
	
	if(m_hfile == INVALID_HANDLE_VALUE)
	{
		DisplayErrorText(GetLastError(), _("Could not create file."));
		return wyFalse;
	}
	pGlobals->m_pcmainwin->AddTextInStatusBar(_(L"Creating Schema..."));
	if(wnd)
	{
		wnd->m_statusbartext.SetAs(_(L"Creating Schema..."));
	}


	SetCursor(LoadCursor(NULL, IDC_WAIT));

	ret = WriteHeaders();
	if(!ret)
		goto cleanup;

	ret = WriteTableHeaders();
	if(!ret)
		goto cleanup;

	ret = WriteTables(GetDlgItem(hwnd, IDC_MESSAGE));
	if(!ret)
		goto cleanup;

	ret = WriteFooters();
	if(!ret)
		goto cleanup;

	VERIFY(CloseHandle(m_hfile));
	pGlobals->m_pcmainwin->AddTextInStatusBar(_(L"Schema created successfully..."));
	if(wnd)
	{
		wnd->m_statusbartext.SetAs(_(L"Schema created successfully..."));
	}
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	return wyTrue;

cleanup:
	DisplayErrorText(GetLastError(), _("Could not write into file."));
	VERIFY(CloseHandle(m_hfile));
	pGlobals->m_pcmainwin->AddTextInStatusBar(_(L"Error In Creating Schema..."));
	if(wnd)
	{
		wnd->m_statusbartext.SetAs(_(L"Error In Creating Schema..."));
	}
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	return wyFalse;
}

// Function to write the headers in the HTML file of the schema . this includes all the top thins
// and the style sheet.

wyBool
CSchema::WriteHeaders()
{
	wyString temptext;
	wyBool result;
	
	result = WriteToFile("<html>\r\n<head>\r\n");
	if(result == wyFalse)
		return result;
	
	temptext.SetAs("<title>Schema for database '");
	temptext.Add(m_db.GetString());
	temptext.Add("'</title>\r\n");
	result = WriteToFile(temptext.GetString());
	if(result == wyFalse)
		return result;
		
	result = WriteToFile("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf8\">\r\n");
	if(result == wyFalse)
		return result;

	temptext.SetAs(STYLE);
	result = WriteToFile(temptext.GetString());
	if(result == wyFalse)
		return result;	
	
	result = WriteToFile("</head>\r\n<body bgcolor='#ffffff' topmargin=\"0\">\r\n");
	if(result == wyFalse)
		return result;
	
	temptext.SetAs("<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"5\">\r\n<tr>\r\n<td class=\"toptext\"><p align=\"center\">");
	temptext.Add(m_db.GetString());
	temptext.Add("</td>\r\n</tr>\r\n</table>\r\n");
	result = WriteToFile(temptext.GetString());
	if(result == wyFalse)
		return result;

	return wyTrue;
}

wyBool
CSchema::WriteTableHeaders()
{
	wyInt32			ret, count = 0, i, m_table = 0;
	wyString		table;
	wyString		temp;
	DWORD			byteswritten;
	wyWChar			*tablewchar = {0};	
	
	// first we make it unordered list.
	WriteFile(m_hfile, "<a name=\"header\">&nbsp</a><ul>\r\n", strlen("<a name=\"header\">&nbsp</a><ul>\r\n"), 
                                    &byteswritten, NULL);

	count = SendMessage(m_hwndtables, LB_GETCOUNT, 0, 0);
	for(i=0; i<count; i++)
	{

		if(!SendMessage(m_hwndtables, LB_GETSEL, i, 0))
			continue;
		tablewchar = AllocateBuffWChar(512);
		VERIFY(ret = SendMessage(m_hwndtables, LB_GETTEXT, i,(LPARAM)tablewchar));

		if(tablewchar)
			table.SetAs(tablewchar);

		m_table = temp.Sprintf("\t<li><a href=\"#%s\"><p class=\"normal\">%s</a></li>\r\n", 
                                     table.GetString(), table.GetString());
		
		ret = WriteFile(m_hfile, temp.GetString(), m_table, &byteswritten, NULL);
		
		free(tablewchar);
		if(!ret)
			return wyFalse;
	}

	ret =  WriteFile(m_hfile, "</ul>\r\n", strlen("</ul>\r\n"), &byteswritten, NULL);
	if(!ret)
		return wyFalse;

	return wyTrue;
}

wyBool
CSchema::WriteTables(HWND hwndmessage)
{
	wyInt32		ret, count = 0, i;
	wyString	temp, table;
	wyWChar		*tablewchar;
	
	count = SendMessage(m_hwndtables, LB_GETCOUNT, 0, 0);
	for(i=0; i<count; i++)
	{
		if(!SendMessage(m_hwndtables, LB_GETSEL, i, 0))
	    	continue;

		tablewchar = AllocateBuffWChar(512);

        VERIFY(ret = SendMessage(m_hwndtables, LB_GETTEXT, i,(LPARAM)tablewchar));

		if(tablewchar)
			table.SetAs(tablewchar);
		
		free(tablewchar);

		// insert a page break
        WriteTablesToFile("<br class=page>\r\n");
        temp.Sprintf("<p><a name='%s'>&nbsp</a>\r\n<table width=\"100%%\" border=\"0\" cellspacing=\"0\" cellpadding=\"3\">\r\n<tr>\r\n\t" , table.GetString());
        WriteTablesToFile(temp.GetString());
        temp.Sprintf("<td class=\"headtext\" width=\"30%%\" align=\"left\" valign=\"top\">%s</td>\r\n\t<td>&nbsp</td>\r\n", table.GetString());
        WriteTablesToFile(temp.GetString());
        temp.Sprintf("<tr>\r\n</table>");
        WriteTablesToFile(temp.GetString());
        WriteTablesToFile("\r\n\r\n<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"3\">\r\n<tr>\r\n\t");
        WriteTablesToFile("<td class=\"fieldheader\" width=\"100%\" align=\"left\" valign=\"top\">Fields</td>\r\n");
        WriteTablesToFile("</tr>\r\n</table>\r\n");
		
		ret = WriteFields(hwndmessage, (wyChar *) table.GetString());
		if(!ret)
			return wyFalse;

        WriteTablesToFile("\r\n\r\n<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"3\">\r\n<tr>\r\n\t");
		WriteTablesToFile("<td class=\"fieldheader\" width=\"100%\" align=\"left\" valign=\"top\">Indexes</td>\r\n");
        temp.Sprintf("</tr>\r\n</table>\r\n");
        WriteTablesToFile(temp.GetString());
      
		ret = WriteIndexes(hwndmessage,(wyChar *) table.GetString());
		if(!ret)
			return wyFalse;

		/* starting with 4.1 we now support FK in schema if m_mysql ver > 4.0.18 */
		if(IsMySQL4018(m_tunnel, m_mysql))
        {
			if(!WriteFK(hwndmessage,(wyChar *) table.GetString()))
				return wyFalse;
		}
        WriteTablesToFile("\r\n\r\n<a href=\"#header\"><p class=\"normal\">Back</a>");
	}
	return wyTrue;
}

wyBool
CSchema::WriteTablesToFile(const wyChar *text)
{
    wyString	temp;
    wyInt32		ret;
    DWORD		byteswritten;
	
    temp.SetAs(text);
	
	ret = WriteFile(m_hfile, temp.GetString(), temp.GetLength(), &byteswritten, NULL);
	if(!ret)
		return wyFalse;
    return wyTrue;
}

wyBool
CSchema::WriteFields(HWND hwndmessage, wyChar *table)
{
	wyInt32			ret, m_table = 0, count = 0;
	wyString		temp, query, temp2 ;
	wyString		message;
	DWORD			byteswritten;
	MYSQL_RES		*myres;
	MYSQL_FIELD		*myfield;
	MYSQL_ROW		myrow;


    temp.Sprintf("<table width=\"100%%\" cellspacing=\"0\" cellapdding=\"2\" border=\"1\">\r\n");
    WriteTablesToFile(temp.GetString());
	
	query.Sprintf("show full fields from `%s`.`%s`", m_db.GetString(), table);
    WriteTablesToFile("<tr>\r\n");
	WriteSchemaMessage(_("Fetching field information for %s"), table, hwndmessage);

    myres = ExecuteAndGetResult( GetActiveWin(), m_tunnel, m_mysql, query);
	if(!myres)
	{
		ShowMySQLError(m_hwnddlg, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}

	while(myfield = m_tunnel->mysql_fetch_field(myres))
    {
        temp.Sprintf("\t<td align=\"center\" valign=\"top\" class=\"fieldcolumn\">%s</td>\r\n", myfield->name);
	    WriteTablesToFile(temp.GetString());
    }
		
	WriteTablesToFile("</tr>\r\n");
	WriteSchemaMessage(_("Writing field information for %s"), table, hwndmessage);

	while(myrow = m_tunnel->mysql_fetch_row(myres))
	{
        WriteTablesToFile("<tr>\r\n");
	
		for(count = 0; count < (wyInt32)myres->field_count; count++)
		{			
			if(myrow[count] == NULL)
			    temp2.SetAs(STRING_NULL);
			
			else if(myrow[count][0] == 0)
				temp2.SetAs("&nbsp;");
			
			else
			    temp2.SetAs( myrow[count], (GetActiveWin())->m_ismysql41);
			
			m_table = temp.Sprintf("\t<td align=\"left\" valign=\"top\"><p class=\"normal\">%s</td>\r\n", temp2.GetString());
						
			ret = 	WriteFile(m_hfile, temp.GetString(), m_table, &byteswritten, NULL);
			if(!ret)
				return wyFalse;
		}
        WriteTablesToFile("</tr>\r\n");
		
	}
    WriteTablesToFile("</table>\r\n");

	m_tunnel->mysql_free_result(myres);
	return wyTrue;
}

wyBool
CSchema::WriteIndexes(HWND hwndmessage, wyChar *table)
{
	wyInt32			ret, itable = 0, i = 0,j = 0;
	wyString		temp, query, temp2;
	wyString		message;
	DWORD			byteswritten;
	MYSQL_RES		*myres;
	MYSQL_FIELD		*myfield;
	MYSQL_ROW		myrow;
	wyBool          ismysql41 = (GetActiveWin())->m_ismysql41;

	// get information about the fields.
    query.Sprintf("show keys from `%s`.`%s`", m_db.GetString(), table);
	WriteSchemaMessage(_("Fetching index information for %s"), table, hwndmessage);
    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!myres)
	{
		ShowMySQLError(m_hwnddlg, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}
	if(myres->row_count == 0)
	{
		m_tunnel->mysql_free_result(myres);
		return wyTrue;
	}
    WriteTablesToFile("<table width=\"100%\" cellspacing=\"0\" cellapdding=\"2\" border=\"1\">\r\n");
	
    temp.Sprintf("<tr>\r\n");
    WriteTablesToFile(temp.GetString());
	
	while(myfield = m_tunnel->mysql_fetch_field(myres))
	{
		temp.SetAs("\t<td align=\"left\" valign=\"top\" class=\"fieldcolumn\">");
		ret = 	WriteFile(m_hfile, temp.GetString(), temp.GetLength(), &byteswritten, NULL);
		for(j=0; myfield->name[j] != NULL; j++)
		{
			if(myfield->name[j] != '_')
				WriteFile(m_hfile, &myfield->name[j], 1, &byteswritten, NULL);
			else
				WriteFile(m_hfile, "<br>", strlen("<br>"), &byteswritten, NULL);
		}
		ret = WriteFile(m_hfile, "</td>\r\n", strlen("</td>\r\n"), &byteswritten, NULL);
		if(!ret)
			return wyFalse;
	}

    WriteTablesToFile("</tr>\r\n");

	WriteSchemaMessage(_("Writing index information for %s"), table, hwndmessage);

	while(myrow = m_tunnel->mysql_fetch_row(myres))
	{
		temp.Sprintf("<tr>\r\n");
		ret = 	WriteTablesToFile(temp.GetString());
		if(!ret)
			return wyFalse;

		for(i=0; i<(wyInt32)myres->field_count; i++)
		{
			if(myrow[i] == NULL)
			    temp2.SetAs(STRING_NULL);
			else if(myrow[i][0] == 0)
				temp2.SetAs("&nbsp;");
			else
			    temp2.SetAs(myrow[i],ismysql41) ;
			
			itable = temp.Sprintf("\t<td align=\"left\" valign=\"top\"><p class=\"normal\">%s</td>\r\n", temp2.GetString());
						
			ret = 	WriteFile(m_hfile, temp.GetString(), itable, &byteswritten, NULL);
			if(!ret)
				return wyFalse;
		}
       	itable = temp.Sprintf("</tr>\r\n");
		ret = WriteFile(m_hfile, temp.GetString(), itable, &byteswritten, NULL);
		if(!ret)
			return wyFalse;
	}
    itable = temp.Sprintf("</table>\r\n");
	ret = 	WriteFile(m_hfile, temp.GetString(), itable, &byteswritten, NULL);
	if(!ret)
		return wyFalse;

	m_tunnel->mysql_free_result(myres);

	return wyTrue;

}

wyBool
CSchema::WriteIndexesGetFeildInfo(wyString &query, wyChar *table)
{
    query.Sprintf("show keys from `%s`.`%s`", m_db.GetString(), table);
	return wyTrue;
}

/* function writes fk information for the table */

wyBool
CSchema::WriteFK(HWND hwndmessag, wyChar *table)
{
	wyInt32			itable = 0, i = 0, retval = 0;
	wyString		temp, query, message;
	DWORD			byteswritten;
	MYSQL_RES		*myres = NULL;
	MYSQL_ROW		myrow;
	wyString		create;
	wyChar			fktitles[][30] = {"FK Id", "Reference Table", "Source Column", "Target Column", "Extra Info"};

	/* get the create table info to get the fk info */
    if(WriteFKTableInfo(query, &myrow, &myres, table, create, hwndmessag, &retval) == wyFalse)
        return wyFalse;

    if(retval == 1)
	{	
        return wyTrue;
	}

	/* now write the titles for the table */
	for(i=0; i <(sizeof(fktitles)/sizeof(fktitles[0])); i++)
	{
		temp.Sprintf("\t<td align=\"left\" valign=\"top\" class=\"fieldcolumn\">");
		VERIFY(WriteFile(m_hfile, temp.GetString(), temp.GetLength(), &byteswritten, NULL));
		VERIFY(WriteFile(m_hfile, fktitles[i], strlen(fktitles[i]), &byteswritten, NULL));
		VERIFY(WriteFile(m_hfile, "</td>\r\n", strlen("</td>\r\n"), &byteswritten, NULL));
	}

    WriteTablesToFile("</tr>\r\n");
	
	WriteSchemaMessage(_("Writing FK information for %s"), table, hwndmessag);

	/* now get stuff from the create table stmt. and add it in the the html */

	/* basically we get each constraint information line...strip out information from it
	   and it as rows...little cumbersome but clean and simple solution */
	/* we read the buffer line by line using strtok()to delimit at \n\r and continue */
	WriteFKAddinHtml(&myrow);
	
	itable = temp.Sprintf("</table>\r\n");
	WriteTablesToFile(temp.GetString());

	m_tunnel->mysql_free_result(myres);

    return wyTrue;

}

wyBool
CSchema::WriteFKTableInfo(wyString &query, MYSQL_ROW *myrow, MYSQL_RES **myres,
                          wyChar *table, wyString &create,HWND hwndmessag, wyInt32 *retval)
{
    wyInt32     len;
    wyString    message, temp;

    len = query.Sprintf("show create table `%s`.`%s`", m_db.GetString(), table);
	
    WriteSchemaMessage(_("Fetching FK information for %s"), table, hwndmessag);

    *myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel,m_mysql,query);
    if(!(*myres))
    {
        ShowMySQLError(m_hwnddlg, m_tunnel, m_mysql, query.GetString());
        return wyFalse;
    }

	*myrow = m_tunnel->mysql_fetch_row(*myres);
	create.Add(*((*myrow)+ 1));

    /* if fk information exists then only we move forward otherwise we return back */
	if(FkInfoPresent(create) == wyFalse)
    {
        *retval = 1;
		m_tunnel->mysql_free_result(*myres);
		return wyTrue;
    }

	/* write the header details specifying fk information */
     WriteTablesToFile("\r\n\r\n<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"3\">\r\n<tr>\r\n\t" );
	 WriteTablesToFile("<td class=\"fieldheader\" width=\"100%\" align=\"left\" valign=\"top\">Foreign Key Relationships</td>\r\n");

	temp.Sprintf("</tr>\r\n</table>\r\n");
	WriteTablesToFile(temp.GetString());
	WriteTablesToFile("<table width=\"100%\" cellspacing=\"0\" cellapdding=\"2\" border=\"1\">\r\n");
	temp.Sprintf("<tr>\r\n");
	WriteTablesToFile(temp.GetString());

    return wyTrue;
}

wyBool
CSchema::WriteFKAddinHtml(MYSQL_ROW *myrow)
{
	wyChar		seps[] = "\n";
	wyChar      *token;
	wyInt32		pos, pos2, pos3;
    wyString	constraint;
	wyString	myrowstr;

	token = strtok(*((*myrow)+ 1), seps);

	while(token)
	{
		constraint.SetAs(token);

		if((pos = constraint.Find(CONSTRAINT, 0)) != -1)
		{
            WriteTablesToFile("<tr>\r\n");
			
			/* found line with constraint info so add it */
			//CONSTRAINT `child_ibfk_2` FOREIGN KEY(`id`)REFERENCES `master`(`id`)ON DELETE CASCADE

			/* get and write the fk id information */
			VERIFY((pos2 = constraint.Find("`", pos)) != -1);
			VERIFY((pos3 = constraint.Find("`", pos2+1))!= -1);
			WriteFkInformation(constraint.Substr(pos2 + 1, (pos3-pos2)-1));

			/* get and write the refernce table info */
			VERIFY((pos = (wyInt32)constraint.Find("REFERENCES", 0)) != -1);
			VERIFY((pos2 = (wyInt32)constraint.Find("`", pos)) != -1);
			VERIFY((pos3 = (wyInt32)constraint.Find("`", pos2+1)) != -1);
			WriteFkInformation(constraint.Substr(pos2 + 1, (pos3-pos2)-1));

			/* get the source column info */
			VERIFY((pos = (wyInt32)constraint.Find(FOREIGNKEY, 0)) != -1);
			VERIFY((pos2 = (wyInt32)constraint.Find("(", pos))!= -1);
			VERIFY((pos3 = (wyInt32)constraint.Find(")", pos2+1))!= -1);
			WriteFkInformation(constraint.Substr(pos2 +1, (pos3-pos2)-1));

			/* get the target column info */
			VERIFY((pos =(wyInt32)constraint.Find("REFERENCES", 0)) != -1);
			VERIFY((pos2 =(wyInt32)constraint.Find("(", pos)) != -1);
			VERIFY((pos3 =(wyInt32)constraint.Find(")", pos2+1)) != -1);
			WriteFkInformation(constraint.Substr(pos2 + 1, (pos3-pos2)-1));
			
			/* we use pos3 from above to add extra info */
			/* this method is very unoptmized */
			if(constraint.Substr(pos3+1, constraint.GetLength()- pos3-1))
				WriteFkInformation(constraint.Substr(pos3 + 1, constraint.GetLength()- pos3-1));

            WriteTablesToFile("<tr>\r\n");
		}
		token = strtok(NULL, seps); 
	}
    return wyFalse;
}

/* Function writes each values of FK information in columns */
wyBool
CSchema::WriteFkInformation(wyChar *info)
{
	wyString temp;
	DWORD	 byteswritten = 0;
	
	temp.Sprintf("\t<td align=\"left\" valign=\"top\"><p class=\"normal\">%s</td>\r\n", info);
	
	return (wyBool) WriteFile(m_hfile, temp.GetString(), temp.GetLength(), &byteswritten, NULL);

}

/* function parses a CREATE TABLE statement and checks if any FK info is present or not */

wyBool
CSchema::FkInfoPresent(wyString &createstmt)
{

	if(createstmt.Find(CONSTRAINT, 0) == -1 || 
		 createstmt.Find(FOREIGNKEY, 0) == -1)
		return wyFalse;

	return wyTrue;
}

// Function to write the footers in the HTML file of the schema . this includes all the top thins
// and the style sheet.

wyBool
CSchema::WriteFooters()
{
	wyInt32		count, ret;
	wyChar		temptext[SIZE_512];
	DWORD		byteswritten;

	count = sprintf(temptext, "\r\n<h1 width=\"100%%\"></body>\r\n</html>\r\n");
	ret = WriteFile(m_hfile, temptext, count, &byteswritten, NULL);
	if(!ret)
		return wyFalse;

	return wyTrue;
}

/* ----------------------------------------------------------------------------------------
	Implementation of CopyTable dialog box. This dialog box is used when the user wants
	to make a fast copy of the table.
   --------------------------------------------------------------------------------------*/

// Constructor.
CCopyTable::CCopyTable()
{
	m_p = new Persist;
	m_p->Create("COPYTABLE");
}

// Destructor.
CCopyTable::~CCopyTable()
{
	//delete the dialog control list
		{
			DlgControl* pdlgctrl;
			while((pdlgctrl = (DlgControl*)m_controllist.GetFirst()))
			{
				m_controllist.Remove(pdlgctrl);
				delete pdlgctrl;
			}
		}
}

// this function creates the actual dialog box and also initializes some values of the class

wyBool
CCopyTable::Create(HWND hwndparent, Tunnel *tunnel, PMYSQL umysql, wyChar *db, 
				   wyChar *table, HTREEITEM hitemtable, HTREEITEM hitemdb)
{
	wyInt64         ret;
	TVITEM			tvi;
	TVINSERTSTRUCT	tvins;
	HTREEITEM		hitemnew;

	m_hwndparent = hwndparent;
	m_mysql = umysql;
	m_tunnel = tunnel;

	m_db.SetAs(db);
	m_table.SetAs(table);
	
	if(SetNewDB(hwndparent) == wyFalse)
		return wyFalse;

	//Post 8.01
	//RepaintTabModule();

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_COPYTABLE), m_hwndparent,
		CCopyTable::CopyTableDialogProc, (LPARAM)this);

	if(ret)
	    CreatetviSet(hwndparent, tvi, tvins, hitemdb, hitemtable, hitemnew);
	else
	{
		SetOldDB();
		return wyTrue;
	}

	VERIFY(TreeView_SelectItem(hwndparent, hitemnew));
	SetOldDB();

	return wyTrue;
}

wyBool
CCopyTable::CreatetviSet(HWND hwndparent, TVITEM &tvi, TVINSERTSTRUCT &tvins, 
                         HTREEITEM &hitemdb,HTREEITEM &hitemtable, HTREEITEM &hitemnew)
{
    tvi.mask			= TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE;
	tvi.pszText			= (wyWChar*)m_newtable.GetAsWideChar();
	tvi.cchTextMax		= SIZE_128-1;
	tvi.iImage			= NTABLE;
	tvi.iSelectedImage	= NTABLE;

	tvins.hParent		= hitemdb;
	tvins.hInsertAfter	= hitemtable;
	tvins.item			= tvi;

	// Add it as the root element.
	VERIFY(hitemnew = TreeView_InsertItem(hwndparent, &tvins));

	// Now add a dummy item so that we get a + arrow even if nothing is added.
	tvi.mask			= TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE;
	tvi.pszText			=(wyWChar*)"Dummy Column";
	tvi.cchTextMax		= wcslen(tvi.pszText);
	tvi.iImage			= NDATABASE;
	tvi.iSelectedImage	= NDATABASE;

	tvins.hParent		= hitemnew;
	tvins.hInsertAfter	= TVI_LAST;
	tvins.item			= tvi;

	VERIFY(TreeView_InsertItem(hwndparent, &tvins)); 	

    return wyTrue;
}

// Callback dialog procedure for the window.

INT_PTR CALLBACK
CCopyTable::CopyTableDialogProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam )
{
	CCopyTable *cpt = (CCopyTable *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		return wyTrue;

	case WM_INITDLGVALUES:
		// we set some initial value.
		cpt->InitWindows(hwnd);
		cpt->FillData();
		break;

	case WM_DESTROY:
		SaveInitPos(hwnd, COPYTABLE_SECTION);
		delete cpt->m_p;
		break;

	case WM_SIZE:
		cpt->CopyTableResize(hwnd);
		break;

	case WM_PAINT:
		cpt->OnPaint(hwnd);
        return wyTrue;;
		break;

	case WM_ERASEBKGND:
		//blocked for double buffering
		return wyTrue;

	case WM_GETMINMAXINFO:
		cpt->OnWMSizeInfo(lparam);
		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/105-copy-table-to-different-host");
		return wyTrue;

	case WM_COMMAND:
		CopyTableDialogProcCommand(hwnd, cpt, wparam);
		break;
	}

	return wyFalse;
}

// Initialize the various startthing for the dialog box windows.
// List adding column for the listview. Selecting some checkboxes and stuff.
wyBool
CCopyTable::InitWindows(HWND hwnd)
{		
	RECT		rectwin;
	wyString	title;
	HWND		hwndstatic, hwndedit;
	LVCOLUMN	lvc;

    memset(&lvc, 0, sizeof(LVCOLUMN));
	GetClientRect(hwnd, &rectwin);
	
	// get the window handle so that we dont have to get it again and again.
	m_hwnd = hwnd;
	m_hwndlist = GetDlgItem(hwnd, IDC_COLLIST);
	//hwndIndex = GetDlgItem(hwnd, IDC_WITHINDEX);
	m_hwndallfield = GetDlgItem(hwnd, IDC_ALLFIELDS);
	m_hwndstruc = GetDlgItem(hwnd, IDC_STRUC);
	m_hwndstrucdata = GetDlgItem(hwnd, IDC_STRUCDATA);
	hwndstatic = GetDlgItem(hwnd, IDC_STATICTITLE);
	hwndedit = GetDlgItem(hwnd, IDC_TABLENAME);

	_ASSERT(m_hwnd && m_hwndlist && m_hwndallfield && m_hwndstruc && m_hwndstrucdata);

	// Set some initial property of the windows in the dialog.
	SendMessage(m_hwndlist, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
	// add to persist list
	m_p->Add(hwnd, IDC_STRUC, "Structure", "0", CHECKBOX);
	m_p->Add(hwnd, IDC_STRUCDATA, "StrucData", "1", CHECKBOX);

	SendMessage(GetDlgItem(hwnd, IDC_ALLINDEX), BM_SETCHECK, BST_CHECKED, 0);
	SendMessage(GetDlgItem(hwnd, IDC_ALLFIELDS), BM_SETCHECK, BST_CHECKED, 0);

	EnableWindow(m_hwndlist, FALSE);
	
	SendMessage(hwndedit, EM_LIMITTEXT, 64, 0);

	// add a column in the listview.
	InitWindowsAddColumn(&lvc, &rectwin, title, hwndstatic);

	// set the default name of the new table.
	InitWindowsSetDefaultName(hwnd, hwndedit, title);
	
	SetFocus(hwndedit);

	GetClientRect(m_hwnd, &m_dlgrect);
	GetWindowRect(m_hwnd, &m_wndrect);

	//set the initial position of the dialog
	SetWindowPositionFromINI(m_hwnd, COPYTABLE_SECTION, 
		m_wndrect.right - m_wndrect.left, 
		m_wndrect.bottom - m_wndrect.top);
	
	GetCtrlRects();
	PositionCtrls();

	return wyTrue;
}

wyBool
CCopyTable::InitWindowsSetDefaultName(HWND hwnd, HWND hwndedit, wyString &title)
{
    title.Sprintf("%s_copy", m_table.GetString());
	SetWindowText(hwndedit, title.GetAsWideChar());

	SendMessage(hwndedit, EM_SETSEL, 0, -1);
	
	if(IsMySQL5010(m_tunnel, m_mysql))
	{
		EnableWindow(GetDlgItem(hwnd, IDC_TRIGGERS), wyTrue); 
		SendMessage(GetDlgItem(hwnd, IDC_TRIGGERS), BM_SETCHECK, BST_UNCHECKED, 0);
	}
	else
	{
		SendMessage(GetDlgItem(hwnd, IDC_TRIGGERS), BM_SETCHECK, BST_UNCHECKED, 0);
		EnableWindow(GetDlgItem(hwnd, IDC_TRIGGERS), 0); 
	}
    return wyFalse;
}

wyBool
CCopyTable::InitWindowsAddColumn(LVCOLUMN *lvc, RECT *rectwin, wyString &title, HWND hwndstatic)
{
    wyInt32		ret;

    lvc->mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	
	//to disable H Scroll bar when not required
	lvc->cx = rectwin->right - 45;
	//lvc->cx = SIZE_512;
	lvc->fmt = LVCFMT_LEFT;

	VERIFY(ret  = ListView_InsertColumn(m_hwndlist, 1, lvc)!= -1);

	title.Sprintf("Copy '%s' to new table", m_table.GetString());
	SetWindowText(hwndstatic, title.GetAsWideChar());

    return wyFalse;
}

// Toggles the state of the listwindow whenever the allfield checkbox is selected or
// unselected.
wyBool
CCopyTable::ToggleListState()
{
	wyInt32 	state;

	state = SendMessage(m_hwndallfield, BM_GETCHECK, 0, 0);

	if(state & BST_CHECKED)
	{
		SetCheckBoxes(wyTrue);        
		EnableWindow(m_hwndlist, wyFalse);
	}
	else
	{
		EnableWindow(m_hwndlist, wyTrue);
	}

	return wyTrue;
}

//sets checkboxes in listview if allfield checkbox is selected 
void
CCopyTable::SetCheckBoxes(wyBool state)
{
	wyUInt32 count,itemcount;

	VERIFY(itemcount = ListView_GetItemCount(m_hwndlist));
	
	for(count = 0; count < itemcount; count++)
		ListView_SetCheckState(m_hwndlist, count,state);

}
	
// Fill the listview with all the column names from the table.
wyBool
CCopyTable::FillData()
{
	wyInt32		ret, len = 0, colindex = -1;
	wyString	query, rowstr;
	LVITEM		lvi;
	MYSQL_RES*	myres;
	MYSQL_ROW	myrow;
	wyWChar		*rowwidechar = 0;
    wyBool      ismysql41 = IsMySQL41(m_tunnel, m_mysql);
	
	SetCursor(LoadCursor(NULL, IDC_WAIT));	
	query.Sprintf("show full fields from `%s`.`%s`", m_db.GetString(), m_table.GetString());
    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!myres)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;
	}

	while(myrow = m_tunnel->mysql_fetch_row(myres))
	{
		rowstr.SetAs(myrow[0], ismysql41);
		rowwidechar = rowstr.GetAsWideChar();

        lvi.mask = LVIF_TEXT;
		lvi.iItem = 2000;			// A large number so that it gets added at last.
		lvi.iSubItem = 0;
		lvi.pszText = rowwidechar;

		VERIFY((ret = ListView_InsertItem(m_hwndlist, &lvi))!= -1);

		ListView_SetCheckState(m_hwndlist, ret, TRUE);
	}

	m_tunnel->mysql_free_result(myres);
	len = query.Sprintf("show table status from `%s` like '%s'", m_db.GetString(), m_table.GetString());
    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	m_type.Clear();
	if(!myres)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;
	}

	myrow = m_tunnel->mysql_fetch_row(myres);

	if(myrow[1] &&(stricmp(myrow[1], "InnoDB")== 0))
		m_comment.SetAs(myrow[GetFieldIndex(myres, "comment", m_tunnel, m_mysql)], ismysql41);

	if(myrow[1])
		m_type.SetAs(myrow[1], ismysql41);
	else
		m_type.SetAs("MyISAM");

	//Create table options: min_rows, max_rows, checksum, delay_key_write, row_format
	if(m_createoptions.GetLength())
		m_createoptions.Clear();

	colindex = GetFieldIndex(myres, "Create_options", m_tunnel, m_mysql);
	if(colindex > 0)
	{		
		if(myrow[colindex])
			m_createoptions.SetAs(myrow[colindex], ismysql41);
	}

	if(m_type.CompareI("innodb") == 0)
		GetCommentString(m_comment);

	if(ismysql41 == wyTrue)
		m_collate.SetAs(myrow[GetFieldIndex(myres, "collation", m_tunnel, m_mysql)], ismysql41);
	
	m_tunnel->mysql_free_result(myres);
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	return wyTrue;
}

// If ANSI_QUOTES global value is set....double quotes will be resulting in an error 

// Function to prepare the query to copy the table and execute it.
wyBool
CCopyTable::DoCopy()
{
	wyInt32		fieldcount, query_for_virtuality_length, isintransaction = 1;;
	wyBool		relation, isssel, isindex, success, ret;
	wyString    query, keys, select, query_for_virtuality, constraintstmt;
	MYSQL_RES	*res;
    wyWChar     newtable[SIZE_512]={0};
    wyChar      *tbuff;
	if(SendMessage(GetDlgItem(m_hwnd, IDC_TABLENAME), WM_GETTEXTLENGTH, 0, 0)== 0)
    {
		yog_message(m_hwnd, _(L"Please give a table name"), 
            pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
		return wyFalse;
	}

	isssel = IsAnyColSelected();

	if(isssel == wyFalse)
	{
		yog_message(m_hwnd, _(L"Please select atleast one field"), 
            pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
		return wyFalse;
	}

	// get the flag whether we need to copy the indexes or not.
	isindex = (wyBool) SendMessage(GetDlgItem(m_hwnd, IDC_ALLINDEX), BM_GETCHECK, 0, 0);

	fieldcount = ListView_GetItemCount(m_hwndlist);
	SendMessage(GetDlgItem(m_hwnd, IDC_TABLENAME), WM_GETTEXT, SIZE_512-1, (LPARAM)newtable);
    m_newtable.SetAs(newtable);
	SetCursor(LoadCursor(NULL, IDC_WAIT));

	GetFields(query, newtable);
			
	if(isindex == wyTrue)
	{
		ret = GetKeys(keys);
		if(ret == wyFalse)	
		{
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			return wyFalse;
		}
	}

	ret = GetSelectStmt(select, query_for_virtuality,newtable);
	if(!ret)	
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;
	}
	query_for_virtuality_length = query_for_virtuality.GetLength();
	if(query_for_virtuality_length)
	{
		query_for_virtuality.Strip(2);
		query.AddSprintf("%s)", query_for_virtuality.GetString());
	}

	if(keys.GetLength() == 0)
	{  
		if(query_for_virtuality_length == 0)
        query.Strip(2);
	}
	else
	{	
		query.Strip(1);
		if(query_for_virtuality_length)
			query.Add(",");
		query.AddSprintf("%s", keys.GetString());
	}

	//ret = GetCheckConstraints(select, constraintstmt, newtable);

	//if (!ret)
	//{
	//	SetCursor(LoadCursor(NULL, IDC_ARROW));
	//	return wyFalse;
	//}
	//if (constraintstmt.GetLength())
	//{
	//	constraintstmt.Strip(1);
	//	query.Strip(1);
	//	query.AddSprintf(",%s)", constraintstmt.GetString());
	//}
	
	//Collation, Engine type, Create table options(min_rows, max_rows, checksum, delay_key_write, row_format)
	if(IsMySQL41(m_tunnel, m_mysql))
		query.AddSprintf("engine=%s %s collate = %s comment = '%s' ", m_type.GetString(), m_createoptions.GetString(), m_collate.GetString(), m_comment.GetString());
	else
		query.AddSprintf("type=%s ", m_type.GetString());

	query.AddSprintf("%s", select.GetString());

	res = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction, GetActiveWindow());

	if(isintransaction == 1)
		return wyFalse;

	
//	res = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);

	if(!res && m_tunnel->mysql_affected_rows(*m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return wyFalse;
	}
	query.Clear();

    success = AddTimestampDefault(m_newtable.GetString());
    if(!success)
        return wyFalse;

	// add the extra info i.e.  autoincrement value..
	if(isindex)
    {
		success = AddExtraInfo(m_newtable.GetString());
		if(!success)
            return wyFalse;
	}

	if(m_comment.GetLength())
    {
		if(!IsAlterOK(m_tunnel, m_mysql))
        {
			yog_message(m_hwnd, _(L"Could not alter table to add Foreign Key Relationship(s)for the table.\nHowever the structure and data were successfully copied."), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
			return wyFalse;
		}

        /// Real escaping the text
        tbuff = (wyChar*)calloc(strlen(m_comment.GetString()) * 2 + 1, sizeof(wyChar));
			
        m_tunnel->mysql_real_escape_string(*m_mysql, tbuff, m_comment.GetString(), m_comment.GetLength());
		m_comment.AddSprintf(" comment '%s' ", tbuff);

        free(tbuff);

		relation = AddRelation();
		if(!relation){
			yog_message(m_hwnd, _(L"There were errors while copying Foreign Key Relationship(s)for the table.\nStructure and data were successfully copied."), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
			return wyFalse;
		}
	}
					
	/* Copy trigger if it supports */

	if(SendMessage(GetDlgItem(m_hwnd, IDC_TRIGGERS), BM_GETCHECK, 0, 0))
	{		
		if(CopyTriggers(m_db.GetString(), m_table.GetString(), m_newtable.GetString()) == wyFalse)
			return wyFalse;
	}
	
	yog_message(m_hwnd, _(L"Copy of table successful"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);

	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return wyTrue;
}

// Function adds the timestamp field default value correctly
wyBool
CCopyTable::AddTimestampDefault(const wyChar *newtable)
{
	wyString	query, primary;
	MYSQL_RES	*myres, *res;
	MYSQL_ROW	myrow;
    wyInt32     typeval, defaultval, index, ret, fieldno = 0, isintransaction = 1;
    wyChar      *escapecomment;
	wyString	strcreate = "";

	GetCreateTableString(m_tunnel, m_mysql, m_db.GetString(), m_table.GetString(), strcreate, query);

	query.Sprintf("show full fields from `%s`.`%s`", m_db.GetString(), m_table.GetString());
    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);

	if(!myres)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}

	defaultval  = GetFieldIndex(myres, "default", m_tunnel, m_mysql);
    typeval     = GetFieldIndex(myres, "type", m_tunnel, m_mysql);
    index       = GetFieldIndex(myres, "comment", m_tunnel, m_mysql);
	
	
	// we check whether we have a primary value.
	while(myrow = m_tunnel->mysql_fetch_row(myres))
	{
		//we need to check whether the field is selected or not
		if(ListView_GetCheckState(m_hwndlist, fieldno) && myrow[defaultval] && myrow[typeval] && stricmp(myrow[defaultval], "CURRENT_TIMESTAMP") == 0 && stricmp(myrow[typeval], "timestamp") == 0)
		{
			primary.Sprintf("alter table `%s`.`%s` change `%s` `%s` %s not null default CURRENT_TIMESTAMP ",
								 m_db.GetString(), newtable, myrow[0], myrow[0], myrow[1]); 
			
			//on update is required or not
			if(CheckForOnUpdate(strcreate, fieldno))
				primary.AddSprintf("on update CURRENT_TIMESTAMP ");

			// so we reached the auto_increment column.
            if(index != -1)
            {
                escapecomment = AllocateBuff(strlen(myrow[index]) * 2 + 1);

                ret = m_tunnel->mysql_real_escape_string(*m_mysql, escapecomment, myrow[index], strlen(myrow[index]));

			    primary.AddSprintf("comment '%s' ", escapecomment); 

                free(escapecomment);
            }
            
            res = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, primary, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

			if(isintransaction == 1)
				return wyFalse;

			if(!res && m_tunnel->mysql_affected_rows(*m_mysql)== -1)
			{
				ShowMySQLError(m_hwnd, m_tunnel, m_mysql, primary.GetString());
				m_tunnel->mysql_free_result(myres);
				return wyFalse;
			}
			else 
            {
				m_tunnel->mysql_free_result(res);
				break;
			}
		}
		fieldno++;
	}
	m_tunnel->mysql_free_result(myres);
	return wyTrue;
}


wyBool 
CCopyTable::CopyTriggers(const wyChar *database, const wyChar *table, const wyChar *newtable)
{
	MYSQL_RES	*res = NULL, *res1 = NULL;
	MYSQL_ROW	row = NULL;
	wyString	query, triggername;
	wyString	name_buff;
	wyString	msg;
	wyInt32		isintransaction = 1;

	query.Sprintf("show triggers from `%s` like %s", database,
        quote_for_like(table,(wyChar *)name_buff.GetString()));
	
    res = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
    if(!res)
	    return wyFalse;

	while(row = m_tunnel->mysql_fetch_row(res))
	{
		triggername.Sprintf("%s_%s", row[0], m_newtable.GetString());
		query.Sprintf("create trigger `%s` %s %s on `%s` for each row %s",
							 triggername.GetString(), /* Trigger */
							 row[4],		 /* Timing */
							 row[1],		 /* Event */
							 newtable,
							 row[3]			 /* Statement */);
		
        res1 = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

		if(isintransaction == 1)
			return wyFalse;

		if(!res1 && m_tunnel->mysql_affected_rows(*m_mysql)== -1)
		{
			ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
			return wyFalse;
		}

        if(res1)
            m_tunnel->mysql_free_result(res);
	}		

	m_tunnel->mysql_free_result(res);

	return wyTrue;
}	

// Function checks if any column are selected or not.
wyBool
CCopyTable::IsAnyColSelected()
{
	wyUInt32 count,itemcount;

	VERIFY(itemcount = ListView_GetItemCount(m_hwndlist));
	
	for(count = 0; count < itemcount; count++)
	{
		if(ListView_GetCheckState(m_hwndlist, count)== wyTrue)
			return wyTrue;
	}

	return wyFalse;
}

// Function copies the initial field stmt. into the buffer,
wyUInt32
CCopyTable::GetFields(wyString &query, const wyWChar *newtable)
{
	wyString	newtablestr;
	newtablestr.SetAs(newtable);
	query.Sprintf("create table `%s`.`%s` ( ", m_db.GetString(), newtablestr.GetString());
	return 1;
}

// checks if the column is selected in the listview.
wyBool
CCopyTable::IsColSelected(wyWChar *colname)
{
	wyWChar	col[SIZE_128] = {0};
	wyInt32	count, itemcount;

	itemcount = ListView_GetItemCount(m_hwndlist);

	for(count = 0; count < itemcount; count++)
	{
		ListView_GetItemText(m_hwndlist, count, 0, col, SIZE_128-1);

		if((wcscmp(colname, col) == 0))
        {
			if(ListView_GetCheckState(m_hwndlist, count))
				return wyTrue;
			
            return wyFalse;
		} 
	}
	return wyFalse;
}

// Function prepares the key thing and puts it in the buffer which is passed as parameter.
wyBool
CCopyTable::GetKeys(wyString &keys)
{	
	wyBool		flgprimary = wyFalse, flgindex = wyFalse;
	wyString	query, primary;
	wyString    index, tempindex;
	MYSQL_RES	*myres = NULL;
	MYSQL_ROW	myrow = NULL;

	// If include index is not selected then we dont have to do anything.
    if(GetKeysIfIncluded(query, &myres) == wyFalse)
        return wyFalse;

	// now we check for primary keys if the keys is not primary then we start adding as a different index. .
    flgprimary = GetKeysPrimeKeyCheck(&myrow, myres, primary);
	
	// now since all the primary keys have been used now we move on for other indexes.
    flgindex = GetKeysOtherIndex(&myrow, myres, index);
	
	m_tunnel->mysql_free_result(myres);

	// now we add everything to make a proper query
	keys.Sprintf("%s%c%s)", primary.GetString(), 
        (flgprimary && flgindex)?(','):(' '), index.GetString());

	// now we have to see if both primary and index are not there then we have to 
	// remove the whole text otherwise it will be an error.
	if(flgprimary == wyFalse && flgindex == wyFalse)
		keys.Clear();

	return wyTrue;
}

//this function fetches the primary key and also the index length if it is there 
// appends it to wyString primary which is passed by reference
wyBool
CCopyTable::GetKeysPrimeKeyCheck(MYSQL_ROW *myrow, MYSQL_RES *myres, wyString &primary)
{
    wyBool      flgprimary = wyFalse;
    primary.Add("primary key(");
	wyBool		ismysql553 ;
	wyString comment;
	ismysql553=   IsMySQL553(m_tunnel,m_mysql);
	wyInt32		flag = 0; 	
	comment.SetAs("");

	while(*myrow = m_tunnel->mysql_fetch_row(myres))
	{
		if(stricmp((*myrow)[GetFieldIndex(myres,"key_name", m_tunnel, m_mysql)], "primary") == 0)
        {
			primary.AddSprintf("`%s`", (*myrow)[GetFieldIndex(myres,"column_name", m_tunnel, m_mysql)]);
            
			if(flag == 0)
			{
				flag++;			
				if(ismysql553)
				{
				if((*myrow)[GetFieldIndex(myres,"Index_comment", m_tunnel, m_mysql)])
					comment.SetAs((*myrow)[GetFieldIndex(myres,"Index_comment", m_tunnel, m_mysql)]);
				}
			}

            if((*myrow)[GetFieldIndex(myres,"sub_part", m_tunnel, m_mysql)])
                primary.AddSprintf("(%s),", (*myrow)[GetFieldIndex(myres,"sub_part", m_tunnel, m_mysql)]);
            else
                primary.Add(",");    

			flgprimary = wyTrue;
		} 
        else 
			break;
	}

    if(flgprimary)
    {
        primary.Strip(1);
		primary.Add(")");
    }
	else
		primary.Clear();

	if(comment.GetLength() != 0)
		primary.AddSprintf(" Comment \"%s\" ", comment.GetString());

	return flgprimary;
}

wyBool
CCopyTable::GetKeysIfIncluded(wyString &query, MYSQL_RES **myres)
{
	query.Sprintf("show keys from `%s`.`%s`", m_db.GetString(), m_table.GetString());
    
    *myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!*myres)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}

    return wyTrue;
}

wyBool
CCopyTable::GetKeysOtherIndex(MYSQL_ROW *myrow, MYSQL_RES *myres, wyString &index)
{
    wyInt32	 ftindex;
    wyString tempindex, myrowstr, myrowstr1;
    wyBool   flgindex = wyFalse, ismysql41 = IsMySQL41(m_tunnel, m_mysql), ismysql553=   IsMySQL553(m_tunnel,m_mysql);
	wyString comment;
	
    while(*myrow)
	{
        flgindex = wyTrue;
		comment.SetAs("");

		tempindex.SetAs((*myrow)[GetFieldIndex(myres, "key_name", m_tunnel, m_mysql)], (GetActiveWin())->m_ismysql41);
		
		if(IsMySQL402(m_tunnel, m_mysql))
			ftindex = GetFieldIndex(myres, "index_type", m_tunnel, m_mysql);
		else
			ftindex = 0;
		
		myrowstr.SetAs((*myrow)[ftindex], ismysql41);

		if(IsMySQL402(m_tunnel, m_mysql) && myrowstr.GetString() && !stricmp(myrowstr.GetString(), "fulltext"))	
			index.AddSprintf("FULLTEXT KEY `%s`(", tempindex.GetString());
		else
		{	
			myrowstr1.SetAs((*myrow)[UNIQUECOL], ismysql41);
            index.AddSprintf("%s `%s`(",((stricmp(myrowstr1.GetString(), "0")==0)?("UNIQUE"): (!myrowstr.CompareI("SPATIAL") ? "SPATIAL KEY" : "KEY")), tempindex.GetString());		
		}

		if(ismysql553)
			{
				if((*myrow)[GetFieldIndex(myres,"Index_comment", m_tunnel, m_mysql)])
					comment.SetAs((*myrow)[GetFieldIndex(myres,"Index_comment", m_tunnel, m_mysql)]);
			}

		while(1)
		{
			myrowstr.SetAs((*myrow)[GetFieldIndex(myres, "column_name", m_tunnel, m_mysql)], ismysql41);
			
			index.AddSprintf(" `%s` ", myrowstr.GetString());
            
            if((*myrow)[GetFieldIndex(myres, "sub_part", m_tunnel, m_mysql)])
                index.AddSprintf("(%s),", (*myrow)[GetFieldIndex(myres, "sub_part", m_tunnel, m_mysql)]);
            else
                index.AddSprintf(",");

			(*myrow) = m_tunnel->mysql_fetch_row(myres);

			if(!(*myrow))
				break;
			
			myrowstr1.SetAs((*myrow)[GetFieldIndex(myres, "key_name", m_tunnel, m_mysql)], ismysql41);

			if((*myrow) && (tempindex.Compare(myrowstr1.GetString()) != 0))
			{
				index.Strip(1);
				index.Add(")");
				if(comment.GetLength() != 0)
					index.AddSprintf(" Comment \"%s\" ", comment.GetString());
				index.Add(", ");
				break;
			}
		}
    }

    // now see if the indexg has been set otherwise we handle it differently
	if(flgindex)
    {
        index.Strip(1);
		index.Add(")");
		if(comment.GetLength() != 0)
					index.AddSprintf(" Comment \"%s\" ", comment.GetString());
    }
	else
		index.Clear();

    return flgindex;
}

// Function adds the autoincrement value correctly.
wyBool
CCopyTable::AddExtraInfo(const wyChar *newtable)
{
	wyString	query, primary;
	MYSQL_RES	*myres, *res;
	MYSQL_ROW	myrow;
    wyInt32     extraval, index, ret, isintransaction = 1;
    wyChar      *escapecomment;
	wyString	myrowstr, myrowstr1;
	wyBool		ismysql41 = IsMySQL41(m_tunnel, m_mysql);

	query.Sprintf("show full fields from `%s`.`%s`", m_db.GetString(), m_table.GetString());
    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!myres)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}
	extraval = GetFieldIndex(myres, "extra", m_tunnel, m_mysql);
    index = GetFieldIndex(myres, "comment", m_tunnel, m_mysql);
	
	// we check whether we have a primary value.
	while(myrow = m_tunnel->mysql_fetch_row(myres))
	{
		if(strstr(myrow[extraval], "auto_increment"))
		{
			myrowstr.SetAs(myrow[0], ismysql41);
			myrowstr1.SetAs(myrow[1], ismysql41);
			// so we reached the auto_increment column.
            if(index != -1)
            {
                escapecomment = AllocateBuff(strlen(myrow[index]) * 2 + 1);

                ret = m_tunnel->mysql_real_escape_string(*m_mysql, escapecomment, myrow[index], strlen(myrow[index]));

			    primary.Sprintf("alter table `%s`.`%s` change `%s` `%s` %s not null auto_increment comment '%s'",
								 m_db.GetString(), newtable, myrow[0], myrow[0], myrow[1], escapecomment); 

                free(escapecomment);
            }
            else
            {
			primary.Sprintf("alter table `%s`.`%s` change `%s` `%s` %s not null auto_increment",
				m_db.GetString(), newtable, myrowstr.GetString(), myrowstr.GetString(), myrowstr1.GetString()); 
             }

            res = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, primary, wyFalse, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

			if(isintransaction == 1)
				return wyFalse;

			if(!res && m_tunnel->mysql_affected_rows(*m_mysql)== -1)
			{
				ShowMySQLError(m_hwnd, m_tunnel, m_mysql, primary.GetString());
				m_tunnel->mysql_free_result(myres);
				return wyFalse;
			}
			else 
            {
				m_tunnel->mysql_free_result(res);
				break;
			}
		}
	}
	m_tunnel->mysql_free_result(myres);
	return wyTrue;
}


// Function prepares the select stmt and stores it in the buffer passed as parameter.
wyBool
CCopyTable::GetSelectStmt(wyString &select, wyString &query_for_virtuality,wyWChar new_table[])
{
	wyInt32	    state, count, itemcount, extraval, pos1 = 0, pos2 = 0, comment_index;
    wyWChar     temptext[SIZE_512] = {0};
	wyString	tempstr, query, create_query;
	MYSQL_RES *myres, *myres1;
	MYSQL_ROW myrow;
	query.Sprintf("show full fields from `%s`.`%s`", m_db.GetString(), m_table.GetString());
 	myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
 
 	if(!myres)
 	{
 		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
 		return wyFalse;
 	}

	query.Sprintf("Show create table `%s`.`%s`",m_db.GetString(), m_table.GetString());
 	myres1 = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);

	if(!myres1)
 	{
 		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
 		return wyFalse;
 	}
	
	extraval = GetFieldIndex(myres, "extra", m_tunnel, m_mysql);
 	comment_index = GetFieldIndex(myres, "Comment", m_tunnel, m_mysql);
 	myrow=m_tunnel ->mysql_fetch_row(myres1);
 	create_query.Sprintf(myrow[1]);

	state = SendMessage(m_hwndallfield, BM_GETCHECK, 0, 0);
	itemcount = ListView_GetItemCount(m_hwndlist);
    select.SetAs("select ");

	for(count = 0; count < itemcount; count++)
	{   
		myrow = m_tunnel->mysql_fetch_row(myres);
		state = ListView_GetCheckState(m_hwndlist, count);
	/*
		Refer this if some issue comes regarding backticks http://dev.mysql.com/doc/refman/5.7/en/server-system-variables.html#sysvar_sql_quote_show_create
	execute the required query , save the previous state and execute it again 
	issue may come only when sql_quote_show_create is set to 0
	
	*/
		if(state)
		{
			ListView_GetItemText(m_hwndlist, count, 0, temptext, SIZE_512-1);
			tempstr.SetAs(temptext);
			if(strstr(myrow[extraval], "VIRTUAL") || strstr(myrow[extraval], "VIRTUAL GENERATED") || strstr(myrow[extraval], "STORED GENERATED") || strstr(myrow[extraval], "STORED") || strstr(myrow[extraval], "PERSISTENT"))
			{
				wyString temp("");
				temp.AddSprintf("`%s`",tempstr.GetString());
				pos1 = create_query.Find(temp.GetString(),pos2+1);
				wyInt32 temp_length = temp.GetLength();
				pos2 = pos1 + temp_length; 
				if(strstr(myrow[extraval], "VIRTUAL")|| strstr(myrow[extraval], "VIRTUAL GENERATED"))
				pos2 =  create_query.Find("VIRTUAL",pos2);
				else if	(strstr(myrow[extraval], "STORED GENERATED") || strstr(myrow[extraval], "STORED"))
				pos2 =  create_query.Find("STORED",pos2);
				else
				pos2 =  create_query.Find("PERSISTENT",pos2);

				//pos2 = create_query.Find("Comment",pos1+1);
				wyInt32 pos3 = create_query.FindChar(',',pos2);
				wyInt32 pos4 = create_query.FindChar(')',pos2);
				//pos2 = ( (pos3 < pos2) && (pos3 >= 0) ) ? pos3 : pos2;
				if( comment_index >=0 )
				{
					temp.SetAs(myrow[comment_index]);
					int comment_length = temp.GetLength();
					if(comment_length)
					{
						pos2 =  (create_query.Find(temp.GetString(),pos2)) + comment_length;
					}
				else
					pos2 = ( (pos3 < pos4) && (pos3 > 0) ) ? (pos3-1) : (pos4-1);
				}
				else
					pos2 = ( (pos3 < pos4) && (pos3 > 0) ) ? (pos3-1) : (pos4-1);
				
				
				query_for_virtuality.AddSprintf("%s, ",create_query.Substr(pos1, pos2-pos1+1));
				continue;
			}
			select.AddSprintf("`%s`, ", tempstr.GetString());
		}
	}

    select.Strip(2);
	select.AddSprintf(" from `%s`.`%s`", m_db.GetString(), m_table.GetString());

	// now see if only structure is selected or not.
	state = SendMessage(m_hwndstrucdata, BM_GETCHECK, 0, 0);

	mysql_free_result(myres);
	mysql_free_result(myres1);

	// this causes only structure to be copied.
	if(!(state & BST_CHECKED))
		select.Add(" where 1 = 0");

	return wyTrue;
}

wyBool
CCopyTable::GetCheckConstraints(wyString &select, wyString &constraintstmt, wyWChar new_table[])
{
	wyString query;
	MYSQL_RES *myres;
	MYSQL_ROW myfieldrow;
	wyString        tblname(""), dbname(""), createtable(""), checkexpression, checkname, alltblcheck(""), str(""), *allcheck = NULL;
	wyChar          *tempstr = NULL, *currentrowstr = NULL, *wholecreatestring = NULL, *wholecreate = NULL;
	//CheckConstraintStructWrapper   *cwrapobj = NULL;
	//CheckConstarintInfo                *icheck = NULL;
	//FieldStructWrapper      *fieldswrap = NULL;
	wyChar * findc = "CONSTRAINT", *findch = "CHECK";
	wyBool found = wyFalse;

	query.Clear();
	query.Sprintf("show create table `%s`.`%s`", m_db.GetString(), m_table.GetString());
	myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);

	if (!myres)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}
	myfieldrow = m_tunnel->mysql_fetch_row(myres);
	createtable = myfieldrow[1];
	wholecreate = (wyChar*)createtable.GetString();

	const char *ptr = strstr(wholecreate, findc);
	str = wholecreate;
	if (ptr)
	{
		const char *ptr2 = strstr(wholecreate, findch);
		int index = ptr - wholecreate;
		alltblcheck = str.Substr(index, str.GetLength());
		found = wyTrue;
	}

	if (found) {
		wholecreatestring = (wyChar*)strdup(alltblcheck.GetString());
		if (wholecreatestring)
			currentrowstr = strtok(wholecreatestring, "\n");

		//loop to get all check constraint

		while (currentrowstr != NULL) {
			if (checkexpression.GetLength() != 0)
				checkexpression.Clear();
			if (!GetCheckConstraintValue(currentrowstr, &checkexpression))
			{
				currentrowstr = strtok(NULL, "\n");
				continue;
			}

			if (checkname.GetLength() != 0)
				checkname.Clear();
			if (!GetCheckConstraintName(currentrowstr, &checkname))
			{
				currentrowstr = strtok(NULL, "\n");
				continue;
			}

			if (checkname.GetLength() && checkexpression.GetLength())
			{
				constraintstmt.AddSprintf("\r\n  constraint %s CHECK %s",checkname.GetString(),checkexpression.GetString());
			}
			constraintstmt.AddSprintf(",");

			//moving to next set of check constraint
			if (currentrowstr)
				currentrowstr = strtok(NULL, "\n");

		}
	}

	m_tunnel->mysql_free_result(myres);

}

// Function to make the new DB the default for the connection
// becase in the create table we cant use the form db.table name
// so we have to make the default db like the new one.
// so that we do things properly.

wyBool
CCopyTable::SetNewDB(HWND hwndparent)
{
	wyString	query;
	MYSQL_RES	*res;
	
	query.Sprintf("use `%s`", m_db.GetString());
    res = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!res && m_tunnel->mysql_affected_rows(*m_mysql)== -1)
	{
		ShowMySQLError(hwndparent, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}

	m_tunnel->mysql_free_result(res);
	m_tunnel->SetDB(m_db.GetString());
	return wyTrue;	
}

// Function to set back to the old database which was there before the execution was done.
// It gives the user an impression no change of database in the current connection happened.
wyBool
CCopyTable::SetOldDB()
{
	wyString	query;
	MYSQL_RES	*res;
	MDIWindow	*pcquerywnd;
	
	VERIFY(pcquerywnd = GetActiveWin());
	
	query.Sprintf("use `%s`", pcquerywnd->m_database.GetString());
    
    res = ExecuteAndGetResult(pcquerywnd, m_tunnel, m_mysql, query);
	if(!res && m_tunnel->mysql_affected_rows(*m_mysql)== -1)
		return wyFalse;

	m_tunnel->mysql_free_result(res);
	m_tunnel->SetDB(pcquerywnd->m_database.GetString());

	return wyTrue;	
}

// Function to copy the foreign key relationships.
wyBool
CCopyTable::AddRelation()
{
	wyChar  	*temp, *comment, *newtext;
	wyString	query;
	wyChar		seps[] = ";";
    MYSQL_RES	*res;
	wyString	newtextstr;
	wyInt32		isintransaction = 1;
	
    if(m_type.CompareI("InnoDB") != 0 && m_type.CompareI("pbxt") != 0 && m_type.CompareI("soliddb") != 0 )
		return wyTrue;

    comment = AllocateBuff(m_comment.GetLength() + 1);
    strcpy(comment, m_comment.GetString());

	temp = strtok(comment, seps);
	while(temp)
	{
		if(!(strstr(temp, "REFER")))
        {
			temp = strtok(NULL, seps);
			continue;
		}

		newtext = FormatReference(temp);
		if(!newtext)
        {
             if(comment)
                free(comment);

			return wyTrue;
        }
		
		newtextstr.SetAs(newtext);

		query.Sprintf("alter table `%s`.`%s` add foreign key %s", m_db.GetString(), m_newtable.GetString(), newtextstr.GetString());

        res = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);

		if(isintransaction == 1)
			return wyFalse;
		if(!res && m_tunnel->mysql_affected_rows(*m_mysql))
		{
            if(comment)
                free(comment);

			ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
			return wyFalse;
		}
		m_tunnel->mysql_free_result(res);
		temp = strtok(NULL, seps);
	}

     if(comment)
        free(comment);

	return wyTrue;
}

void
CCopyTable::GetCtrlRects()
{
    RECT    rect;
    wyInt32 i, count;
    HWND    hwnd;

    //ctrlid    size-x     size-y
    wyInt32 ctrlid[] = {
       	IDC_TABLENAME, 1, 0,
        IDC_COLLIST, 1, 1,
        IDOK, 0, 0,
        IDCANCEL, 0, 0,
        IDC_STAT_HORZ, 1, 0,
		IDC_COPYTABLE_GRIP, 0, 0,
    };

    count = sizeof(ctrlid)/sizeof(ctrlid[0]);

    //store the control handles and related information in the linked list
    for(i = 0; i < count; i+=3)
    {
		hwnd = GetDlgItem(m_hwnd, ctrlid[i]);
        GetWindowRect(hwnd, &rect);
		MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rect, 2);
        m_controllist.Insert(new DlgControl(hwnd, 
                                            ctrlid[i], 
                                            &rect, 
                                            ctrlid[i + 1] ? wyTrue : wyFalse, 
                                            ctrlid[i + 2] ? wyTrue : wyFalse));
    }
}

void
CCopyTable::PositionCtrls()
{
    DlgControl* pdlgctrl;
    wyInt32     leftpadding, toppadding, rightpadding, bottompadding, width, height;
    wyInt32     x, y;
    RECT        rect;
    HDWP        hdefwp;
		
	GetClientRect(m_hwnd, &rect);

    //BeginDeferWindowPos is used to make the control positioning atomic
    hdefwp = BeginDeferWindowPos(m_controllist.GetCount() + 1);

    //iterate throught the control lists
    for(pdlgctrl = (DlgControl*)m_controllist.GetFirst(); pdlgctrl;
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
			if(pdlgctrl->m_id == IDC_COPYTABLE_GRIP)
				x = rect.right - width;
			else
				x=rect.right - rightpadding - width;
        }
        else
        {
			x= leftpadding;
			width = rect.right - leftpadding - rightpadding;
        }
	    switch(pdlgctrl->m_id)
        {
			case IDC_TABLENAME:
			case IDC_COLLIST:
				y = toppadding;
				break;

			case IDC_COPYTABLE_GRIP:
				y = rect.bottom - height;
				break;

            default:
				y = rect.bottom - bottompadding - height;
                break;        
        }

        if(pdlgctrl->m_issizey == wyTrue)
        {
            height = rect.bottom - bottompadding - y;
        }

        //change the control position
        hdefwp = DeferWindowPos(hdefwp, pdlgctrl->m_hwnd, NULL, x, y, width, height, SWP_NOZORDER);
    }

    //finish the operation and apply changes
    EndDeferWindowPos(hdefwp);
}

void 
CCopyTable::CopyTableResize(HWND hwnd){

	PositionCtrls();
}

void
CCopyTable::OnWMSizeInfo(LPARAM lparam)
{
    MINMAXINFO* pminmax = (MINMAXINFO*)lparam;
    pminmax->ptMinTrackSize.x = m_wndrect.right - m_wndrect.left;
    pminmax->ptMinTrackSize.y = m_wndrect.bottom - m_wndrect.top;
}

void
CCopyTable::OnPaint(HWND hwnd)
{
    HDC         hdc;
	PAINTSTRUCT ps;
	HWND hwndstat = GetDlgItem(hwnd, IDC_COPYTABLE_GRIP);
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



/*+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=

  Implementation of the table diagnostics dialog box.

+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=*/

TableDiag::TableDiag(HWND hwndparent, Tunnel * tunnel, PMYSQL mysql)
{
	wyInt64				ret;

	m_mysql         =   mysql;
	m_tunnel        =   tunnel;
	m_hwndcombo		=   NULL;
	m_hwndlisttables=   NULL;
	m_hwndparent    =   hwndparent;
	m_isautomated	=	wyTrue;
	
	m_p = new Persist;
	m_p->Create("TABLEDIAG");

	//Post 8.01
	//RepaintTabModule();

	ret	=	DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE   (IDD_TABLEDIAG),m_hwndparent, TableDiag::TableDlgProc,(LPARAM)this);

}

TableDiag::~TableDiag()
{
		//delete the dialog control list
		{
			DlgControl* pdlgctrl;
			while((pdlgctrl = (DlgControl*)m_controllist.GetFirst()))
			{
				m_controllist.Remove(pdlgctrl);
				delete pdlgctrl;
			}
		}


}

INT_PTR CALLBACK 
TableDiag::TableDlgProc(HWND phwnd, wyUInt32 pmessage, WPARAM pwparam, LPARAM plparam)
{	
	TableDiag *pctablediag	= (TableDiag*)GetWindowLongPtr(phwnd, GWLP_USERDATA);
	
	switch(pmessage)
	{
	case WM_INITDIALOG:
        SetWindowLongPtr(phwnd, GWLP_USERDATA,plparam);
        LocalizeWindow(phwnd);
		pctablediag->TableDlgProcInit(phwnd, plparam);
		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/109-table-diagnostics");
		return wyTrue;		

	case WM_COMMAND:
		pctablediag->TableDlgProcCommand(phwnd, pctablediag, pwparam, plparam);
		break;

	//Notify Msg While clicking Table Diag...(For Disabling/Enabling Controls)
	case WM_NOTIFY:
		{
			LPNMHDR lpnm = (LPNMHDR)plparam;

			switch (lpnm->code)
			{	
			case LVN_ITEMCHANGED:
				pctablediag->TableDlgListViewEvent(phwnd, pctablediag);
				HandleListViewCheckboxItems(pctablediag->m_hwndlisttables, plparam, pctablediag->m_isautomated);
				pctablediag->EnableDisableTableDiag(pctablediag->m_hwnd,IsAnyListItemChecked(pctablediag->m_hwndlisttables));
				break;
			}
		}
		break;

	case WM_SIZE:
		pctablediag->TableDlgResize(phwnd,pctablediag);
		break;

	case WM_GETMINMAXINFO:
		pctablediag->OnWMSizeInfo(plparam,pctablediag);
		break;

	case WM_PAINT:
		pctablediag->OnPaint(phwnd);
        return 1;
		break;

	case WM_ERASEBKGND:
		//blocked for double buffering
		return 1;

	case WM_DESTROY:
		SaveInitPos(phwnd, TABLEDIAGNOSTICS_SECTION);
		delete pctablediag->m_p;
		break;

	default:
		break;
	}

	return 0;
}

// Sets the list box into extended style so that we can use checkboxes in the listbox.
// Also it initiliazes the report list with a columns.

wyBool
TableDiag::SetListBoxExtendedStyle()
{
	
	wyBool		ret;
	DWORD		ret2;
	LVCOLUMN	lvc;

	ret2 = SendMessage(m_hwndlisttables, LVM_SETEXTENDEDLISTVIEWSTYLE, 
                    (WPARAM)LVS_EX_CHECKBOXES,(LPARAM)LVS_EX_CHECKBOXES);

	lvc.mask	= LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	lvc.cx		= 200;
	lvc.fmt		= LVCFMT_LEFT;
	lvc.pszText = 0;
	// check it later wether its correct r no

	VERIFY(ret =(wyBool)(ListView_InsertColumn(m_hwndlisttables, 0, &lvc)!= -1));

	return ret;
}

// It initializes the database COMBOBOX with the databases which are there in the treeview control.
// If for some reason while the dialog box is opened and some new addition has been done in the
// object browser then the user has to close the dialog box and refresh the object browser and gain
// start the dialog box.

wyBool
TableDiag::InitializeCOMBOBOX()
{
	MDIWindow*		pcquerywnd	= (MDIWindow *)GetWindowLongPtr(m_hwndparent, GWLP_USERDATA);
    CQueryObject*   obj         = pcquerywnd->m_pcqueryobject;
	wyString    	dbname;
    wyWChar         db[SIZE_512]={0};
	HWND			hwndtree, hwndcheckopt;
	TVITEM			tvi;
	HTREEITEM		hitem;
	COMBOBOXEXITEM	cbi;
	wyInt32			count, size, ret;
	wyWChar			option[][20] = {L"None", L"Quick", L"Fast", L"Medium", L"Extended", L"Changed" };

	_ASSERT(pcquerywnd != NULL);

	hwndtree	=	pcquerywnd->m_pcqueryobject->m_hwnd;
	hitem		=	TreeView_GetRoot(hwndtree);

	// Initialize the COMBOBOX with the database name which we get from the treeview.
	for(hitem = TreeView_GetChild(hwndtree, hitem); hitem != NULL; hitem = TreeView_GetNextSibling(hwndtree, hitem))
	{
		tvi.mask		=	TVIF_TEXT;
		tvi.pszText		=	(LPWSTR)db;
		tvi.hItem		=	hitem;
		tvi.cchTextMax	=	SIZE_512 - 1;

		VERIFY(TreeView_GetItem(hwndtree, &tvi));

        dbname.SetAs(db);

		VERIFY(ret = SendMessage(m_hwndcombo, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)dbname.GetAsWideChar())!= CB_ERR);
	}

	// Now get the selected textbox from the main COMBOBOX.
	cbi.mask = CBEIF_TEXT ;
	cbi.pszText = (LPWSTR)db;
	cbi.cchTextMax = sizeof(db) - 1;
	cbi.iItem = pGlobals->m_pcmainwin->GetCurrentSel();

	// This we implement to get the current database from the COMBOBOX so that we can select
	// the current database. If no database is found then we select the first one.
	VERIFY(ret = SendMessage(pGlobals->m_pcmainwin->m_hwndtoolcombo, CBEM_GETITEM, 0,(LPARAM)&cbi)!= CB_ERR);
	
    //If database selected on object browser that would be the current db, else first db in object browser
    if(obj->m_seldatabase.GetLength())
        ret = SendMessage(m_hwndcombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)obj->m_seldatabase.GetAsWideChar());
    
    else
        ret = 0;         

	if(ret == CB_ERR)
		SendMessage(m_hwndcombo, CB_SETCURSEL,(WPARAM)0, 0);
	else
		SendMessage(m_hwndcombo, CB_SETCURSEL,(WPARAM)ret, 0);
	
	// now add all the check option in the COMBOBOX.
	VERIFY(hwndcheckopt = GetDlgItem(m_hwnd, IDC_CHECKOPTION));
	size = sizeof(option)/ sizeof(option[0]);

	for(count = 0; count < size; count++)
		VERIFY(ret = SendMessage(hwndcheckopt, CB_INSERTSTRING,(WPARAM)-1,(LPARAM)option[count]) != CB_ERR);

	SendMessage(hwndcheckopt, CB_SETCURSEL,(WPARAM)0, 0);

	return wyTrue;
}

// This function changes the tables in the listcontrol whenever a new database is selected.
wyBool
TableDiag::OnComboChange( HWND phwnd)
{
	wyInt32			ncursel, count, tabcount = 0, ret = 0;
	wyString		dbname, query, myrowstr;
	LVITEM			lvi;
    MYSQL_RES*		myres;
	MYSQL_ROW		myrow;
	wyBool			oldstate, ismysql41 = IsMySQL41(m_tunnel, m_mysql);
	wyWChar			*db;
	wyInt32			length = 0;
	HCURSOR			hcursor = GetCursor();
	ncursel		=	SendMessage(m_hwndcombo, CB_GETCURSEL, 0, 0);
	
	length = SendMessage(m_hwndcombo, CB_GETLBTEXTLEN, ncursel, NULL);

	db = AllocateBuffWChar(length + 1);
	if(SendMessage(m_hwndcombo, CB_GETLBTEXT,(WPARAM)ncursel,(LPARAM)db) != CB_ERR)
	{	
		dbname.SetAs(db);
        PostMessage(m_hwndcombo, CB_SETCURSEL, ncursel, 0);
	}

	free(db);

	VERIFY(ListView_DeleteAllItems(m_hwndlisttables));
	EnableWindow(GetDlgItem(phwnd, IDC_DIAG_CHKUNCHK), wyTrue);
	count = PrepareShowTable(m_tunnel, m_mysql, dbname, query);

	SetCursor(LoadCursor(NULL, IDC_WAIT));

    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(myres == NULL)
	{
		ShowMySQLError(GetParent(m_hwndcombo), m_tunnel, m_mysql, query.GetString());
		ListView_DeleteAllItems(m_hwndlisttables);
		return wyFalse;
	}

    oldstate = m_ischk;
    m_ischk = wyFalse;

	// Execute the query and fetch the records and add them to the list box.
	m_isautomated = wyTrue;
	while(myrow = m_tunnel->mysql_fetch_row(myres))
	{
		myrowstr.SetAs(myrow[0], ismysql41);

		lvi.mask		=	LVIF_TEXT;
		lvi.iItem		=	0;
		lvi.iSubItem	=	0;
		lvi.pszText		=	(LPWSTR)myrowstr.GetAsWideChar();
		lvi.cchTextMax	=	lstrlenW(lvi.pszText);

		VERIFY((ret  = SendMessage(m_hwndlisttables, LVM_INSERTITEM, 0,(LPARAM)&lvi))!= -1);

		ListView_SetCheckState(m_hwndlisttables, ret, wyTrue);
		++tabcount; // count for number of tables in a Data base.
	}	
	m_isautomated = wyFalse;


    m_ischk = oldstate;
	//if table count is zero , Disable all Tablediag controls.
	if(tabcount == 0)
	{
		EnableDisableTableDiag(phwnd, FALSE);
		EnableWindow(GetDlgItem(phwnd, IDC_DIAG_CHKUNCHK), wyFalse); 
		return wyTrue;
	}
    else
    {
        EnableDisableTableDiag(phwnd, wyTrue);
		EnableWindow(GetDlgItem(phwnd, IDC_DIAG_CHKUNCHK), wyTrue); 
    }

	m_tunnel->mysql_free_result(myres);

	SendMessage(m_hwndcheck, BM_SETCHECK, (WPARAM)1, (LPARAM)0);
	SetCursor(hcursor);
	ShowCursor(1);
	
	return wyTrue;
}

// This function toggles the check uncheck all database option.
wyBool
TableDiag::OnCheckClick()
{
	wyInt32 ret;

	ret	= SendMessage(m_hwndcheck, BM_GETCHECK, 0, 0);

	if(ret == BST_UNCHECKED)
	{	
		SetCheckUncheck(BST_UNCHECKED);
		return wyFalse;
	}
		
	else
		SetCheckUncheck(BST_CHECKED);

	return wyTrue;
}

// The function goes through all the items in the listbox and checks/unchecks them as passed in the parameter.
wyBool
TableDiag::SetCheckUncheck(wyInt32 ncheckstate)
{
	wyInt32	count, itemcount;
	m_ischk =  wyFalse;
	itemcount =	ListView_GetItemCount(m_hwndlisttables);

	if(itemcount == 0)
	{
		EnableDisableTableDiag(m_hwnd, FALSE);
		return wyFalse;
	}

	else
	{
	for(count = 0; count < itemcount; count++)
		ListView_SetCheckState(m_hwndlisttables, count,(ncheckstate==BST_CHECKED)?(wyTrue):(wyFalse));
		//Check atleast one item in ListView is checked or not , if yes set the flag true.
		if(ncheckstate)		
			m_ischk = wyTrue;
	}
					  
	if(m_ischk)  
		//Enable all Tablediag controls
		EnableDisableTableDiag(m_hwnd, TRUE); 
	else
		//Disable all Tablediag controls
		EnableDisableTableDiag(m_hwnd, FALSE);
	
	m_ischk = wyTrue;

	return  wyTrue;
}



// The function gets the DBname from the COMBOBOX and copies it in the buffer.

wyBool 
TableDiag::GetDBName(wyString &dbname)
{
	wyInt32 cursel;
    wyWChar  db[512] = {0};
	
	cursel = SendMessage(m_hwndcombo, CB_GETCURSEL, 0, 0);
	SendMessage(m_hwndcombo, CB_GETLBTEXT, (WPARAM)cursel, (LPARAM)db);
    dbname.SetAs(db);

	return wyTrue;
}

// The function prepares the optimize table query for the table diagnostaics.
// Basically it keeps on reallocating buffer for the cheked tablename in the listview.
// And at the end executes it.
wyBool
TableDiag::DiagOptimize()
{
	wyInt32		count, ret, itemcount, checkeditemcount = 0, binlog, isintransaction = 1;
	wyString	query, dbname, tablestr;
    wyWChar     table[SIZE_512]= {0};
	LVITEM		lvi;
	CShowInfo	csi;
	MYSQL_RES	*myres;

	itemcount =	ListView_GetItemCount(m_hwndlisttables);
	
	/* whether to issue write to bin log option */
	binlog = SendMessage(GetDlgItem(m_hwnd, IDC_DIAG_OPTIMIZELOCAL), BM_GETCHECK, 0, 0);

	query.Sprintf("optimize %s table ", (BST_CHECKED == binlog)?("NO_WRITE_TO_BINLOG"):(""));
	GetDBName(dbname);
	
	for(count = 0; count < itemcount; count++)
	{
		ret = ListView_GetCheckState(m_hwndlisttables, count);

		if(!ret)
			continue;

		++checkeditemcount;

		lvi.mask		=	LVIF_TEXT;
		lvi.iItem		=	count;
		lvi.iSubItem	=	0;
		lvi.pszText		=	(LPWSTR)table;
		lvi.cchTextMax	=	SIZE_512 -1;

		ListView_GetItem(m_hwndlisttables, &lvi);
		
		tablestr.SetAs(table);

		query.AddSprintf("`%s`.`%s`, ", dbname.GetString(), tablestr.GetString());	
	}

	if(checkeditemcount == 0)
	    return wyFalse;

	query.Strip(2);
	SetCursor(LoadCursor(NULL, IDC_WAIT));
    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction, GetActiveWindow());

	if(isintransaction == 1)
		return wyFalse;
	if(!myres)
	{
		ShowMySQLError(m_hwndparent, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}
	
	csi.ShowInfo(m_hwnd, m_tunnel, myres, _(" Table Diagnostics Information"));
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	m_tunnel->mysql_free_result(myres);

	return wyTrue;
}

// The function prepares the analyze table query for the table diagnostaics.
// Basically it keeps on reallocating buffer for the cheked tablename in the listview.
// And at the end executes it.

wyBool
TableDiag::DiagAnalyze()
{
	wyInt32		count, ret, itemcount, checkeditemcount = 0, binlog, isintransaction = 1;
	wyString	query, dbname, tablestr;
    wyWChar     table[SIZE_512] = {0};
	LVITEM		lvi;
	CShowInfo	csi;
	MYSQL_RES   *myres;
    
	itemcount =	ListView_GetItemCount(m_hwndlisttables);
	/* whether to issue write to bin log option */
	binlog = SendMessage(GetDlgItem(m_hwnd, IDC_DIAG_ANALYZELOCAL), BM_GETCHECK, 0, 0);
	
	query.Sprintf("analyze %s table ", (BST_CHECKED==binlog)?("LOCAL"):("NO_WRITE_TO_BINLOG"));
	
	GetDBName(dbname);
	
	for(count = 0; count < itemcount; count++)
	{
		ret = ListView_GetCheckState(m_hwndlisttables, count);
		if(!ret)
			continue;

		++checkeditemcount;
	
		lvi.mask		=	LVIF_TEXT;
		lvi.iItem		=	count;
		lvi.iSubItem	=	0;
		lvi.pszText		=	(LPWSTR)table;
		lvi.cchTextMax	=	SIZE_512 - 1;

		ListView_GetItem(m_hwndlisttables, &lvi);
		tablestr.SetAs(table);
		query.AddSprintf("`%s`.`%s`, ", dbname.GetString(), tablestr.GetString());
	}
	if(checkeditemcount == 0)
	    return wyFalse;
		
    query.Strip(2);
	SetCursor(LoadCursor(NULL, IDC_WAIT));

    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction, GetActiveWindow());

	if(isintransaction == 1)
		return wyFalse;
	if(!myres)
	{
		ShowMySQLError(m_hwndparent, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}

	csi.ShowInfo(m_hwnd, m_tunnel, myres, _(" Table Diagnostics Information"));
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	m_tunnel->mysql_free_result(myres);
	return wyTrue;
}

// The function prepares the check table query for the table diagnostaics.
// Basically it keeps on reallocating buffer for the cheked tablename in the listview.
// And at the end executes it.

wyBool
TableDiag::DiagCheck()
{
	wyInt32		count, ret, itemcount,  checkeditemcount = 0;
	wyString    query, dbname, tablestr;
    wyWChar     table[SIZE_512] = {0};
	LVITEM		lvi;
	CShowInfo	csi;
    	
	itemcount = ListView_GetItemCount(m_hwndlisttables);
	query.SetAs("check table ");

	GetDBName(dbname);
	
	for(count = 0; count < itemcount; count++)
	{
		ret = ListView_GetCheckState(m_hwndlisttables, count);
		if(!ret)
			continue;
		++checkeditemcount;
				
		lvi.mask		=	LVIF_TEXT;
		lvi.iItem		=	count;
		lvi.iSubItem	=	0;
		lvi.pszText		=	(LPWSTR)table;
		lvi.cchTextMax	=	sizeof(table)-1;
		
        ListView_GetItem(m_hwndlisttables, &lvi);

		tablestr.SetAs(table);

		query.AddSprintf("`%s`.`%s`, ", dbname.GetString(), tablestr.GetString());
	}

	if(checkeditemcount == 0)
	    return wyFalse;

	query.Strip(2);
	
    DiagCheckAddOptions(query, &csi);

    return wyTrue;
}

// The function prepares the repair table query for the table diagnostaics.
// Basically it keeps on reallocating buffer for the cheked tablename in the listview.
// And at the end executes it.

wyBool
TableDiag::DiagCheckAddOptions(wyString &query, CShowInfo *csi)
{
    // now add the option things.
	// but first we get the text.
    wyInt32		cursel, isintransaction = 1;
    wyWChar      option[SIZE_512];
    MYSQL_RES   *myres;
	wyString	optionstr;

    cursel = SendMessage(GetDlgItem(m_hwnd, IDC_CHECKOPTION), CB_GETCURSEL, 0, 0);
	if(cursel != CB_ERR)
    {
		SendMessage(GetDlgItem(m_hwnd, IDC_CHECKOPTION), CB_GETLBTEXT, cursel, (LPARAM)option);
		
		optionstr.SetAs(option);
        
		if((optionstr.CompareI("none")) != 0)
			query.AddSprintf(" %s", optionstr.GetString());
	}

	SetCursor(LoadCursor(NULL, IDC_WAIT));

    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction, GetActiveWindow());

	if(isintransaction == 1)
		return wyFalse;

	if(!myres)
	{
		ShowMySQLError(m_hwndparent, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}

	SetCursor(LoadCursor(NULL, IDC_ARROW));
	csi->ShowInfo(m_hwnd, m_tunnel, myres, _(" Table Diagnostics Information"));
	m_tunnel->mysql_free_result(myres);

    return wyTrue;
}

wyBool
TableDiag::DiagRepair()
{
	wyInt32		itemcount;
	wyString    query;
	CShowInfo	csi;
	MYSQL_RES   *myres = NULL;
	wyBool		result;

	itemcount = ListView_GetItemCount(m_hwndlisttables);

	/* whether to issue write to bin log option */
    result = DiagRepairBinLog(query, itemcount);
   
	if(result == wyFalse)
		return wyTrue;
	
	query.Strip(2);
	// get the other type of reapir.
	myres = DiagRepairOther(query);

    if(!myres)
		return wyFalse;

	csi.ShowInfo(m_hwnd, m_tunnel, myres, _(" Table Diagnostics Information"));
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	m_tunnel->mysql_free_result(myres);

    return wyTrue;
}

wyBool 
TableDiag::DiagRepairBinLog(wyString &query, wyInt32 itemcount)
{
    wyInt32     count,  checkeditemcount = 0, binlog, ret;
    LVITEM		lvi;
    wyString    dbname, tablestr;
    wyWChar     table[SIZE_512] = {0};

    binlog = SendMessage(GetDlgItem(m_hwnd, IDC_DIAG_USELOCALREPAIR), BM_GETCHECK, 0, 0);
	query.Sprintf("repair %s table ", (BST_CHECKED==binlog)?("NO_WRITE_TO_BINLOG"):(""));
	GetDBName(dbname);
	
	for(count = 0; count < itemcount; count++)
	{
		ret = ListView_GetCheckState(m_hwndlisttables, count);

		if(!ret)
			continue;

		++checkeditemcount;

		lvi.mask		=	LVIF_TEXT;
		lvi.iItem		=	count;
		lvi.iSubItem	=	0;
		lvi.pszText		=	(LPWSTR)table;
		lvi.cchTextMax	=	sizeof(table) - 1;

		ListView_GetItem(m_hwndlisttables, &lvi);

		tablestr.SetAs(table);
	
        query.AddSprintf("`%s`.`%s`, ", dbname.GetString(), tablestr.GetString());
	}
    if(checkeditemcount == 0)
	    return wyFalse;

    return wyTrue;
}


MYSQL_RES * 
TableDiag::DiagRepairOther(wyString &query)
{
    MYSQL_RES *myres;
	wyInt32 isintransaction = 1;

    if(SendMessage(GetDlgItem(m_hwnd, IDC_DIAG_QUICKREPAIR), BM_GETCHECK, 0, 0))
		query.Add(" QUICK ");

	if(SendMessage(GetDlgItem(m_hwnd, IDC_DIAG_EXTENDEDREPAIR), BM_GETCHECK, 0, 0))
		query.Add(" EXTENDED ");

	if(SendMessage(GetDlgItem(m_hwnd, IDC_DIAG_USEFRMREPAIR), BM_GETCHECK, 0, 0))
	     query.Add(" USE_FRM ");

	SetCursor(LoadCursor(NULL, IDC_WAIT));

    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction, GetActiveWindow());

	if(isintransaction == 1)
		return NULL;

	if(!myres)
	{
		ShowMySQLError(m_hwndparent, m_tunnel, m_mysql, query.GetString());
		return myres;
	}
    return myres;
}

/*
	Implementation of CShowInfo.
	It formats and shows information for various queries or it will display the string.
															*/

CShowInfo::CShowInfo()
{
	m_res = NULL;

}

CShowInfo::~CShowInfo()
{
}

wyBool 
CShowInfo::ShowInfo(HWND hwndparent, Tunnel * tunnel, MYSQL_RES * myres, wyChar *title, wyChar *summary)
{
	wyInt64     ret;

	m_hwndparent    = hwndparent;
	
	m_tunnel        = tunnel;

	//table diagnostic query result
	if(myres)
	{
		m_res           = myres;
		VERIFY(m_field	= tunnel->mysql_fetch_field(myres));
	}

	if(title)
		m_title.SetAs(title);

	//for Copy database summary
	if(summary)
		m_summary.SetAs(summary);

	//Post 8.01
    //RepaintTabModule();

	if( GetForegroundWindow() != hwndparent)
		FlashWindow(pGlobals->m_pcmainwin->m_hwndmain, TRUE);

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_SHOWINFO),
		hwndparent, CShowInfo::DlgProc, (LPARAM)this);

	if(ret)
        return wyTrue;

    return wyFalse;
}

INT_PTR CALLBACK
CShowInfo::DlgProc(HWND hwnd, wyUInt32 message, WPARAM wParam, LPARAM lparam)
{
	CShowInfo *csi = (CShowInfo *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		break;

	case WM_INITDLGVALUES:
        csi->OnInitDlgvalues(hwnd);
    	break;
	
	case WM_HELP:
		if(csi->m_res)
			ShowHelp("http://sqlyogkb.webyog.com/article/132-environment-variables");
		return wyTrue;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			yog_enddialog(hwnd, 0);
			break;
		}
		break;

	case WM_CTLCOLORSTATIC:
		return csi->OnCtlColorStatic(wParam, lparam);

	case WM_SIZE:
		csi->ShowInfoResize(hwnd);
		break;

	case WM_PAINT:
		//draws the resize gripper at bottom-right corner of dialog
		DrawSizeGripOnPaint(hwnd);
	break;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO pMMI = (LPMINMAXINFO)lparam;
			pMMI->ptMinTrackSize.x = RELDLGWD;
			pMMI->ptMinTrackSize.y = RELDLGHT;
		}
		break;
		
	case WM_NCDESTROY:
		DeleteFont(csi->m_hfont);
		break;

	case WM_DESTROY:
		//StoreDialogPersist(hwnd, SHOWSUMMARY_SECTION);
		break;
	}
	return 0;
}

void 
CShowInfo::OnInitDlgvalues(HWND hwnd)
{
    wyString result;
	m_hwnd = hwnd;
	wyChar	*res = NULL;
	wyBool	ismysql41 = GetActiveWin()->m_ismysql41;
	HICON	hicon;

	SetWindowText(m_hwnd, m_title.GetAsWideChar());

	VERIFY(m_hwndedit = GetDlgItem(m_hwnd, IDC_INFO));

	//SetDialogPos(hwnd, SHOWSUMMARY_SECTION);
	ShowInfoResize(hwnd);
	SetFont();

	//for copy database summary. for this we will set m_res to NULL
	if(!m_res)
	{
		hicon = CreateIcon(IDI_COPYDATABASE_16);
		SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hicon);
		SendMessage(m_hwndedit, WM_SETTEXT, 0, (LPARAM)m_summary.GetAsWideChar());
		return;
	}

	hicon = CreateIcon(IDI_TABLEDIAG);
	SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hicon);

	//for table diagnostics query result
	res = FormatResultSet(m_tunnel, m_res, m_field);
	if(res)
	{
		if(ismysql41 == wyFalse)
			VERIFY(result.SetAs(res, wyFalse));
		else
			VERIFY(result.SetAs(res));
		HeapFree(GetProcessHeap(), NULL, res);
	}
	SendMessage(m_hwndedit, WM_SETTEXT, 0, (LPARAM)result.GetAsWideChar());
}

wyBool 
CShowInfo::SetFont()
{
	HDC      dc;
	wyUInt32 fontheight;

	dc = GetDC(m_hwnd);

    fontheight = -MulDiv(8, GetDeviceCaps(dc, LOGPIXELSY), 72);
	VERIFY(ReleaseDC(m_hwnd, dc));
	m_hfont = CreateFont(fontheight, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Courier New");

	_ASSERT(m_hfont != NULL);

	SendMessage(m_hwndedit, WM_SETFONT,(WPARAM)m_hfont, TRUE);

	return wyTrue;
}

//this function is used to set the Background color of edit box
//In this dialog edit box is read only,If we set readonly then it's color is grey.
//this function is used for changing grey color to White color
wyInt32
CShowInfo::OnCtlColorStatic(WPARAM wparam, LPARAM lparam)
{
	HWND hwndedit;
	HDC			hdc = NULL;
	wyInt32		identifier = 0;
	
	identifier = GetDlgCtrlID((HWND)lparam);
	hdc = (HDC)wparam;
	VERIFY(hwndedit = GetDlgItem(m_hwnd, IDC_INFO));
		
	if(identifier == IDC_INFO)
	{
      SetBkColor(hdc, COLOR_WHITE);
	  return (wyInt32)GetStockObject(DC_BRUSH);
	}	
	return 1;
}

//resizes the dialog
void
CShowInfo::ShowInfoResize(HWND hwnd)
{
	HWND    hwndedit, hwndbutton;
	RECT	rctedit, rctbutton, rctdlg, rcttmp;
	wyInt32	buttonht = 0, buttonwd = 0, height = 0;
	wyInt32 top = 0, left = 0;

	hwndbutton = GetDlgItem(hwnd, IDOK);
	hwndedit = GetDlgItem(hwnd, IDC_INFO);
	
	GetClientRect(hwnd, &rctdlg);
	GetClientRect(hwndbutton, &rctbutton);
	GetClientRect(hwndedit, &rctedit);

	buttonht = rctbutton.bottom;
	buttonwd = rctbutton.right;

	top = rctdlg.bottom - buttonht - 10;
	left = rctdlg.right - buttonwd - 10; 

	MoveWindow(hwndbutton, left, top, buttonwd, buttonht, TRUE);//ok

	height = rctdlg.bottom - 50;
	MoveWindow(hwndedit, 10, 10, rctdlg.right - 20, height, TRUE); 

	GetWindowRect(hwndedit, &rctedit);
	MapWindowPoints(NULL, hwnd, (LPPOINT)&rctedit, 2);

	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd);

	//left of edit box
	rcttmp.top = 0;
	rcttmp.left = rctdlg.left;
	rcttmp.right = rctedit.left;
	rcttmp.bottom = rctdlg.bottom;
	InvalidateRect(hwnd, &rcttmp, TRUE);
	UpdateWindow(hwnd);	

	//top of edit box
	rcttmp.top = 0;
	rcttmp.left = rctdlg.left;
	rcttmp.right = rctdlg.right ;
	rcttmp.bottom = rctedit.top;
	InvalidateRect(hwnd, &rcttmp, TRUE);
	UpdateWindow(hwnd);	

	//bottom of edit box
	rcttmp.top = rctedit.bottom;
	rcttmp.left = rctdlg.left;
	rcttmp.right = rctdlg.right;
	rcttmp.bottom = rctdlg.bottom;
	InvalidateRect(hwnd, &rcttmp, TRUE);
	UpdateWindow(hwnd);	

	//right of 
	rcttmp.top = 0;
	rcttmp.left = rctedit.right;
	rcttmp.right = rctdlg.right;
	rcttmp.bottom = rctdlg.bottom;
	InvalidateRect(hwnd, &rcttmp, TRUE);
	UpdateWindow(hwnd);	
}

/*

  Implementation of CReorderColumn class.
  Used to reorder columns

  Starting from SQLyog v4.1, we use m_mysql - ALTER TABLE CHANGE syntax. If m_mysql version is less then
  4.0.18, we use 10 steps to reorder as we did before.

*/



// Implementation of IDD_SHOWWARDING dialog. This is a simple genricdialog, which SQLyog can 
// call for operations where SHOW warnings is required. It will open up a list box to show the warnings. */

/* simple dialog to show a message */

// Constructor.
CShowWarning::CShowWarning()
{


}

// Destructor.
CShowWarning::~CShowWarning()
{

}

// this function creates the actual dialog box and also initializes some values of the class

wyBool
CShowWarning::Create(HWND hwndparent, Tunnel *tunnel, PMYSQL mysql, const wyWChar *title, const wyWChar *text)
{
	m_tunnel = tunnel;
	m_mysql  = mysql;
	m_title	 = title;
	m_text	 = text;

	DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_SHOWWARNING), hwndparent,
							CShowWarning::WndProc,(LPARAM)this);
	return wyTrue;
}

INT_PTR CALLBACK
CShowWarning::WndProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam)
{
	CShowWarning *warn  = (CShowWarning *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA,lparam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		return wyTrue;

	case WM_INITDLGVALUES:
		WndProcInitValues(hwnd, warn, wparam);		
		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/100-import-csv-data-using-load-local");
		return wyTrue;

	case WM_COMMAND:
		WndProcCommands(hwnd, warn, wparam);
		break;
	}
	return wyFalse;
}

void
CShowWarning::ShowWarnings(HWND hwnd)
{
	CShowValue val;
	val.Create(hwnd, m_tunnel, m_mysql, WARNINGS);
}

wyBool 
CShowValue::CallBackContextMenu(HWND hwnd, CShowValue *pcshowvalue, LPARAM lparam)
{
	BOOL	menuselect;
	HMENU	hmenu, htrackmenu;
	POINT	pnt;

	pnt.x = (LONG)LOWORD(lparam);
	pnt.y = (LONG)HIWORD(lparam);
	
	pcshowvalue->m_prow = CustomGrid_GetCurSelRow(pcshowvalue->m_hwndgrid);
	
	VERIFY(hmenu = LoadMenu(pGlobals->m_hinstance, MAKEINTRESOURCE(IDR_PROCESS)));
    LocalizeMenu(hmenu);
	VERIFY(htrackmenu =	GetSubMenu(hmenu, 0));
	
	if(pcshowvalue->m_prow == -1)
		VERIFY(EnableMenuItem(htrackmenu, ID_OBJECT_KILLPROCESS, MF_GRAYED)!= -1);
	
	VERIFY(menuselect = TrackPopupMenu(htrackmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pnt.x, pnt.y, 0, hwnd, NULL));
	VERIFY(DestroyMenu(hmenu));

	return wyFalse;
	
}

wyBool 
CSchema::SchemaDialogProcIdok(HWND hwnd , CSchema *pcschema)
{
	wyBool	ret;
	wyInt32 count;
	wyWChar	*filename;

	count = SendMessage(pcschema->m_hwndtables, LB_GETSELCOUNT, 0, 0);
	if(count == 0)
	{
		yog_message(hwnd, L"Select at least one table", pGlobals->m_appname.GetAsWideChar(), MB_ICONINFORMATION | MB_OK);
		return wyFalse;
	}

	filename = AllocateBuffWChar(512);

	ret = InitFile(hwnd, (wyWChar *)filename, HTMINDEX, 512);

	if(filename)
		pcschema->m_file.SetAs(filename);
	
	//post 8.01
	/* update the window because of scintilla repainting issue */
	//InvalidateRect(GetParent(hwnd), NULL, FALSE);
	//InvalidateRect(hwnd, NULL, FALSE);

	UpdateWindow(GetParent(hwnd));
	UpdateWindow(hwnd);

	if(ret)
		PostMessage(hwnd, UM_CREATESCHEMA, NULL, NULL);
	
	free(filename);

	return wyFalse;
}

wyBool
CSchema::WriteToFile(const wyChar *string)
{
	wyInt32		ret;
	wyString	temptext;
	DWORD		byteswritten;
	
	temptext.SetAs(string);
	ret = WriteFile(m_hfile, temptext.GetString(), temptext.GetLength(), &byteswritten, NULL);
	if(!ret)
		return wyFalse;

	return wyTrue;
}


wyInt32 
TableDiag::TableDlgProcInit(HWND phwnd, LPARAM plparam)
{
	TableDiag	*pctablediag		=	(TableDiag *)plparam;
			
	pctablediag->m_hwnd			    =	phwnd;
	pctablediag->m_hwndcheck		=	GetDlgItem(phwnd, IDC_DIAG_CHKUNCHK);
	pctablediag->m_hwndcombo		=	GetDlgItem(phwnd, IDC_DIAG_DATABASE);
	pctablediag->m_hwndlisttables   =	GetDlgItem(phwnd, IDC_DIAG_TABLES);

	_ASSERT((pctablediag->m_hwndcombo != NULL)&&(pctablediag->m_hwndlisttables != NULL)&&(pctablediag->m_hwndcheck != NULL));
	
	pctablediag->InitializeCOMBOBOX();
	pctablediag->SetListBoxExtendedStyle();
	pctablediag->OnComboChange(phwnd);
	pctablediag->m_p->Add(phwnd, IDC_CHECKOPTION, "Check", "0", COMBOBOX_P);
	pctablediag->m_p->Add(phwnd, IDC_DIAG_QUICKREPAIR, "RepairQuick", "0", CHECKBOX);
	pctablediag->m_p->Add(phwnd, IDC_DIAG_EXTENDEDREPAIR, "RepairExtended", "0", CHECKBOX);
	pctablediag->m_p->Add(phwnd, IDC_DIAG_USEFRMREPAIR, "RepairUseFrm", "0", CHECKBOX);
	pctablediag->m_p->Add(phwnd, IDC_DIAG_USELOCALREPAIR, "RepairLocal", "0", CHECKBOX);
	pctablediag->m_p->Add(phwnd, IDC_DIAG_OPTIMIZELOCAL, "OptimizeLocal", "0", CHECKBOX);
	pctablediag->m_p->Add(phwnd, IDC_DIAG_ANALYZELOCAL, "AnalyzeLocal", "0", CHECKBOX);
	
	GetClientRect(phwnd, &pctablediag->m_dlgrect);
	GetWindowRect(phwnd, &pctablediag->m_wndrect);

	//set the initial position of the dialog
	SetWindowPositionFromINI(pctablediag->m_hwnd, TABLEDIAGNOSTICS_SECTION, 
		pctablediag->m_wndrect.right - pctablediag->m_wndrect.left, 
		pctablediag->m_wndrect.bottom - pctablediag->m_wndrect.top);

	GetCtrlRects(pctablediag);
	PositionCtrls(pctablediag);
#ifndef COMMUNITY
	if(GetActiveWin()->m_conninfo.m_isreadonly == wyTrue)
	{
	//	EnableWindow(GetDlgItem(phwnd, IDC_DIAG_OPTIMIZE), wyFalse);
	//	EnableWindow(GetDlgItem(phwnd, IDC_DIAG_OPTIMIZELOCAL), wyFalse);
		EnableWindow(GetDlgItem(phwnd, IDC_DIAG_QUICKREPAIR), wyFalse);
		EnableWindow(GetDlgItem(phwnd, IDC_DIAG_EXTENDEDREPAIR), wyFalse);
		EnableWindow(GetDlgItem(phwnd, IDC_DIAG_USEFRMREPAIR), wyFalse);
		EnableWindow(GetDlgItem(phwnd, IDC_DIAG_USELOCALREPAIR), wyFalse);
		EnableWindow(GetDlgItem(phwnd, IDC_DIAG_REPAIR), wyFalse);
	}
#endif

	return wyTrue;	
}

wyBool 
CShowValue::ShowValueDialogProcCommand(CShowValue *pcshowvalue, HWND hwnd, WPARAM wparam)
{
	switch(LOWORD(wparam))
	{
	case IDOK:

	case IDCANCEL:
		VERIFY(yog_enddialog(hwnd, 1));
		break;

	case IDC_KILL:
		pcshowvalue->m_prow = CustomGrid_GetCurSelRow(pcshowvalue->m_hwndgrid);
		if(pcshowvalue->m_prow != -1)
			pcshowvalue->KillProcess();
		else
			yog_message(hwnd, _(L"Please select a process from the list"), 
				_(L"Cannot kill processlist..."), MB_OK | MB_ICONERROR);
		break;

	case ID_OBJECT_KILLPROCESS:
		pcshowvalue->KillProcess();
		break;

	case IDC_REFRESH:
		pcshowvalue->OnRefreshClick();
		break;
	}
	return wyFalse;

}

wyBool 
CSchema::SchemaDialogProcCommand(HWND hwnd, CSchema *pcschema, WPARAM wparam)
{
	switch(LOWORD(wparam))
	{
	case IDOK:
		SchemaDialogProcIdok(hwnd, pcschema);
		break;

	case ID_SELECTALL:
		SendMessage(pcschema->m_hwndtables, LB_SETSEL, TRUE, -1);
		break;

	case ID_UNSELECT:
		SendMessage(pcschema->m_hwndtables, LB_SETSEL, FALSE, -1);
		break;
	
	case IDCANCEL:
		yog_enddialog(hwnd, 0);
		break;
	}
	return wyFalse;

}

wyBool 
CCopyTable::CopyTableDialogProcCommand(HWND hwnd, CCopyTable *cpt, WPARAM wparam)
{
	switch(LOWORD(wparam))
	{
	wyBool ret;

	case IDC_ALLFIELDS:
		cpt->ToggleListState();
		break;

	case IDOK:
		ret = cpt->DoCopy();
		if(ret)
			VERIFY(yog_enddialog(hwnd, 1));
		break;
	
	case IDCANCEL:
		cpt->m_p->Cancel();
		VERIFY(yog_enddialog(hwnd, 0));
		break;
	}
	return wyFalse;
}

wyInt32 
TableDiag::TableDlgProcCommand(HWND phwnd, TableDiag *pctablediag, WPARAM pwparam, LPARAM plparam)
{
    HWND hwnd = NULL;
	switch(LOWORD(pwparam))
	{
	case IDOK:
		VERIFY(yog_enddialog(phwnd, 0));
		break;

	case IDCANCEL:
		pctablediag->m_p->Cancel();
		VERIFY(yog_enddialog(phwnd, 0));
		break;

	case IDC_DIAG_CHKUNCHK:
		pctablediag->OnCheckClick();
		break;

	case IDC_DIAG_DATABASE:
        if((HIWORD(pwparam))== CBN_SELENDOK || (HIWORD(pwparam))== CBN_SELENDCANCEL)
        {
            hwnd = GetFocus();
			if(m_hwndcombo == hwnd)
            {
                pctablediag->OnComboChange(phwnd);
            }
        }
		break;

	case IDC_DIAG_OPTIMIZE:
		pctablediag->DiagOptimize();
		break;

	case IDC_DIAG_ANALYZE:
		pctablediag->DiagAnalyze();
		break;

	case IDC_DIAG_CHECK:
		pctablediag->DiagCheck();
		break;

	case IDC_DIAG_REPAIR:
		pctablediag->DiagRepair();
		break;
	}
	return wyFalse;
}

wyBool
CShowWarning::WndProcCommands(HWND hwnd, CShowWarning *warn, WPARAM wparam)
{
	
	switch LOWORD(wparam)
	{
	case IDC_SHOWWARNINGS:
		warn->ShowWarnings(hwnd);
		break;

	case IDCANCEL:
	case IDOK:
		yog_enddialog(hwnd, 1);
		break;
	}
	return wyFalse;
}

wyBool 
CShowWarning::WndProcInitValues(HWND hwnd, CShowWarning *warn, WPARAM wparam)
{
	SetWindowText(hwnd, warn->m_title);
	SetWindowText(GetDlgItem(hwnd, IDC_MESSAGE), warn->m_text);
	if(IsMySQL41(warn->m_tunnel, warn->m_mysql) == wyFalse)
		EnableWindow(GetDlgItem(hwnd, IDC_SHOWWARNINGS), false);

	// load the information image
	HICON icon = LoadIcon(NULL, IDI_INFORMATION);
	VERIFY(icon);
	SendMessage(GetDlgItem(hwnd, IDC_STATICIMG), STM_SETIMAGE, IMAGE_ICON,(LPARAM)(wyUInt32)icon);

	return wyFalse;
}

//Function for checking the state of ListView
void 
TableDiag::TableDlgListViewEvent(HWND phwnd, TableDiag *pctablediag)
{
	wyInt32	count, itemcount;
	wyBool flag = wyFalse;

	if(!pctablediag || m_ischk == wyFalse)
		return;
		
	itemcount =	ListView_GetItemCount(m_hwndlisttables);

	for(count = 0; count < itemcount; count++) 
	{
		if(ListView_GetCheckState(m_hwndlisttables, count) == wyFalse) 
		{	
			//set the select/unselect to be unchecked if atleast one table is not selected.
			SendMessage(m_hwndcheck, BM_SETCHECK, (WPARAM)0, (LPARAM)0);
		}
		else
			flag= wyTrue;//if any table is selected
	}
	//if atlest one table is selected, then we need to enable all the controls, otherwise we disable.			
	EnableDisableTableDiag(phwnd, flag);
		
	return;
}

//Function for Enable/Disable TableDiag. Controls
void 
TableDiag::EnableDisableTableDiag(HWND thwnd, BOOL flag)
{
	EnableWindow(GetDlgItem(thwnd, IDC_DIAG_OPTIMIZE), flag);
	EnableWindow(GetDlgItem(thwnd, IDC_DIAG_USEFRMREPAIR), flag);
	EnableWindow(GetDlgItem(thwnd, IDC_DIAG_CHECK), flag);
	EnableWindow(GetDlgItem(thwnd, IDC_DIAG_ANALYZE), flag);
	EnableWindow(GetDlgItem(thwnd, IDC_DIAG_REPAIR), flag);
	EnableWindow(GetDlgItem(thwnd, IDC_DIAG_OPTIMIZELOCAL), flag);
	EnableWindow(GetDlgItem(thwnd, IDC_DIAG_CHECK), flag);
	EnableWindow(GetDlgItem(thwnd, IDC_DIAG_ANALYZELOCAL), flag);
	EnableWindow(GetDlgItem(thwnd, IDC_DIAG_QUICKREPAIR), flag);
	EnableWindow(GetDlgItem(thwnd, IDC_DIAG_EXTENDEDREPAIR), flag);
	EnableWindow(GetDlgItem(thwnd, IDC_DIAG_USELOCALREPAIR), flag);
	EnableWindow(GetDlgItem(thwnd, IDC_CHECKOPTION), flag); 
#ifndef COMMUNITY
	if(GetActiveWin()->m_conninfo.m_isreadonly == wyTrue)
	{
	//	EnableWindow(GetDlgItem(thwnd, IDC_DIAG_OPTIMIZE), wyFalse);
	//	EnableWindow(GetDlgItem(thwnd, IDC_DIAG_OPTIMIZELOCAL), wyFalse);
		EnableWindow(GetDlgItem(thwnd, IDC_DIAG_QUICKREPAIR), wyFalse);
		EnableWindow(GetDlgItem(thwnd, IDC_DIAG_EXTENDEDREPAIR), wyFalse);
		EnableWindow(GetDlgItem(thwnd, IDC_DIAG_USEFRMREPAIR), wyFalse);
		EnableWindow(GetDlgItem(thwnd, IDC_DIAG_USELOCALREPAIR), wyFalse);
		EnableWindow(GetDlgItem(thwnd, IDC_DIAG_REPAIR), wyFalse);
	}
#endif

	return;
}

void
TableDiag::OnWMSizeInfo(LPARAM lparam, TableDiag *pt)
{
    MINMAXINFO* pminmax = (MINMAXINFO*)lparam;

    pminmax->ptMinTrackSize.x = pt->m_wndrect.right - pt->m_wndrect.left;
    pminmax->ptMinTrackSize.y = pt->m_wndrect.bottom - pt->m_wndrect.top;
}




void
TableDiag::GetCtrlRects(TableDiag *pt)
{
    RECT    rect;
    wyInt32 i, count;
    HWND    hwnd;

    //ctrlid    size-x     size-y
    wyInt32 ctrlid[] = {
       	IDC_DIAG_DATABASE, 1, 0,
        IDC_DIAG_CHKUNCHK, 0, 0,
        IDC_DIAG_TABLES, 1, 1,
        IDC_DIAG_OPTIMIZE, 0, 0,
        IDC_DIAG_OPTIMIZELOCAL, 0, 0,
        IDC_DIAG_CHECK, 0, 0,
        IDC_CHECKOPTION, 0, 0,
        IDC_DIAG_ANALYZE, 0, 0,
        IDC_DIAG_ANALYZELOCAL, 0, 0,
        IDC_DIAG_REPAIR, 0, 0,
        IDC_DIAG_QUICKREPAIR, 0, 0,
        IDC_DIAG_EXTENDEDREPAIR, 0, 0,
        IDC_DIAG_USEFRMREPAIR, 0, 0,
        IDC_DIAG_USELOCALREPAIR, 0, 0,
        IDOK, 0, 0,
        IDC_HORZ, 1, 0,
        IDC_DIAG_GROUP2, 0, 0,
        IDC_DIAG_GROUP1, 0, 0,
        IDC_DIAG_GROUP3, 0, 0,
        IDC_DIAG_GROUP4, 0, 0,
        IDC_DIAG_ICON, 0, 0,
		IDC_TABLEDIAG_GRIP, 0, 0,
    };

    count = sizeof(ctrlid)/sizeof(ctrlid[0]);

    //store the control handles and related information in the linked list
    for(i = 0; i < count; i+=3)
    {
        hwnd = GetDlgItem(pt->m_hwnd, ctrlid[i]);
        GetWindowRect(hwnd, &rect);
        MapWindowPoints(NULL, pt->m_hwnd, (LPPOINT)&rect, 2);
        pt->m_controllist.Insert(new DlgControl(hwnd, 
                                            ctrlid[i], 
                                            &rect, 
                                            ctrlid[i + 1] ? wyTrue : wyFalse, 
                                            ctrlid[i + 2] ? wyTrue : wyFalse));
    }
}

void
TableDiag::PositionCtrls(TableDiag *pt)
{
    DlgControl* pdlgctrl;
    wyInt32     leftpadding, toppadding, rightpadding, bottompadding, width, height;
    wyInt32     x, y;
    RECT        rect;
    HDWP        hdefwp;
		
	GetClientRect(pt->m_hwnd, &rect);

    //BeginDeferWindowPos is used to make the control positioning atomic
    hdefwp = BeginDeferWindowPos(pt->m_controllist.GetCount() + 1);

    //iterate throught the control lists
    for(pdlgctrl = (DlgControl*)pt->m_controllist.GetFirst(); pdlgctrl;
        pdlgctrl = (DlgControl*)pdlgctrl->m_next)
    {
        leftpadding = pdlgctrl->m_rect.left - pt->m_dlgrect.left;
        toppadding = pdlgctrl->m_rect.top - pt->m_dlgrect.top;
        rightpadding = pt->m_dlgrect.right - pdlgctrl->m_rect.right;
        bottompadding = pt->m_dlgrect.bottom - pdlgctrl->m_rect.bottom;
        width = pdlgctrl->m_rect.right - pdlgctrl->m_rect.left;
        height = pdlgctrl->m_rect.bottom - pdlgctrl->m_rect.top;
        
        if(pdlgctrl->m_issizex == wyFalse)
        {
			if(pdlgctrl->m_id == IDC_TABLEDIAG_GRIP)
				x = rect.right - width;
			else
				x = rect.right - rightpadding - width;
        }
        else
        {
			x = leftpadding;
            width = rect.right - rightpadding - leftpadding;
        }
	    switch(pdlgctrl->m_id)
        {
			case IDOK:
			case IDC_HORZ:
                y = rect.bottom - bottompadding - height;
                break;

			case IDC_TABLEDIAG_GRIP:
				y = rect.bottom - height;
				break;
            default:
                y = toppadding;
        }

        if(pdlgctrl->m_issizey == wyTrue)
        {
            height = rect.bottom - bottompadding - y;
        }

        //change the control position
        hdefwp = DeferWindowPos(hdefwp, pdlgctrl->m_hwnd, NULL, x, y, width, height, SWP_NOZORDER);
    }

    //finish the operation and apply changes
    EndDeferWindowPos(hdefwp);
}

void
TableDiag::TableDlgResize(HWND hwnd, TableDiag* pt){

	PositionCtrls(pt);
}

//function handles the WM_PAINT and paints the window using double buffering
void
TableDiag::OnPaint(HWND hwnd)
{
    HDC         hdc;
	PAINTSTRUCT ps;
	HWND hwndstat = GetDlgItem(hwnd, IDC_TABLEDIAG_GRIP);
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


/* ----------------------------------------------------------------------------------------
	Implementation of Empty Database dialog box. This dialog box is used when the user wants
	to empty the database.
   --------------------------------------------------------------------------------------*/

//Constructor
EmptyDB::EmptyDB()
{

}

// Destructor.
EmptyDB::~EmptyDB()
{

}

wyInt32
EmptyDB::Create(HWND hwndparent, Tunnel *tunnel, PMYSQL umysql, wyChar *db)
{
	wyInt32		ret = 0;

    m_mysql		= umysql;
	m_tunnel	= tunnel;
	m_db.SetAs(db);

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_EMPTYDB), hwndparent,
							EmptyDB::EmptyDBDialogProc,(LPARAM)this);
	return ret;
}

INT_PTR CALLBACK
EmptyDB::EmptyDBDialogProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam)
{
	EmptyDB *emptydb = (EmptyDB*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
	    SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		return wyTrue;
		
	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/118-empty-database");
		return wyTrue;

	case WM_INITDLGVALUES:
        emptydb->m_hwnddlg = hwnd;
        emptydb->EmptyDbDlgInit();
       	break;

	case WM_COMMAND:
        emptydb->OnWmCommand(wparam);
		break;

	case WM_NOTIFY:
		{
			LPNMHDR lpnm = (LPNMHDR)lparam;

			switch (lpnm->code)
			{	
			case LVN_ITEMCHANGED:
				emptydb->EmptyDbListViewEvent();
				break;
			}
		}
		break;
	}
	return wyFalse;
}

void
EmptyDB::EmptyDbDlgInit()
{
    wyString dbtext;
    //TOOLINFO ti;

    m_hwndlist      = GetDlgItem(m_hwnddlg, IDC_OBJECTSLIST);
    m_hwndselectall = GetDlgItem(m_hwnddlg, IDC_EMPTYDBSELECTALL);

    dbtext.Sprintf("'%s'", m_db.GetString());
    SendMessage(GetDlgItem(m_hwnddlg, IDC_STATICEMPTYDB), WM_SETTEXT,0, (LPARAM)dbtext.GetAsWideChar());

	//By default SelectAll is checked
    SendMessage(m_hwndselectall, BM_SETCHECK , BST_CHECKED, 0);

	//Setting the listcontrol properties
    InitList();

	//Filling the List Control
    FillList();

    //Create the tool tip
    /*m_hwndtooltip = CreateWindowEx( WS_EX_TOPMOST,
                                    TOOLTIPS_CLASS,
                                    NULL,
                                    WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,		
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    m_hwnddlg,
                                    NULL,
                                    pGlobals->m_hinstance,
                                    NULL);

    //return if CreateWindowEx fail
    if(m_hwndtooltip == NULL)
        return;

    SetWindowPos(m_hwndtooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    //fill the TOOLINFO struct
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_SUBCLASS;
    ti.hinst = pGlobals->m_hinstance;
    ti.hwnd = m_hwnddlg;
    ti.lpszText = dbtext.GetAsWideChar();
    ti.uId = 0;

    //Get the client rect of IDC_STATICEMPTYDB control and convert it to dialog cordinates
    GetClientRect(GetDlgItem(m_hwnddlg, IDC_STATICEMPTYDB), &ti.rect);
    MapWindowPoints(GetDlgItem(m_hwnddlg, IDC_STATICEMPTYDB), m_hwnddlg, (LPPOINT)&ti.rect, 2);

    //add the tool tip
    SendMessage(m_hwndtooltip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
    */
	return;
}

//Setting the list control Style
void
EmptyDB::InitList()
{
    LVCOLUMN	lvc;
    wyInt32		ret;
  
    SendMessage(m_hwndlist, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
      
	lvc.mask        = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	lvc.fmt		    = LVCFMT_LEFT;
	lvc.pszText     = (LPWSTR)"XXX";
    lvc.cchTextMax  = lstrlenW(lvc.pszText);
	lvc.cx		    = 200;

   VERIFY(ret  = ListView_InsertColumn(m_hwndlist, 1, &lvc)!= -1);
}

//Filling the List Control
void
EmptyDB::FillList()
{
    wyInt32     i = 0,ret;
    wyWChar*    objects[] = {TXT_TABLES, TXT_VIEWS, TXT_PROCEDURES, TXT_FUNCTIONS, TXT_TRIGGERS, TXT_EVENTS};
    LVITEM	    lvi;

    for(i = 0; i <= 5; i++)
    {
		//if < mysql 5.0 there is no views ,Procedures functions and triggers */
		//i=0 means Tables. In the case of table no need for checking the mysql version
		if((i != 0 ) && (IsMySQL5010(m_tunnel, m_mysql) == wyFalse))
			break;
	
		//if < mysql 5.1.6 there is no events
        if(i == 5 && (IsMySQL516(m_tunnel, m_mysql) == wyFalse))
            break;

        lvi.mask		=	LVIF_TEXT | LVIF_IMAGE ;
        lvi.iItem       =   i;
        lvi.iSubItem    =   0;
        lvi.pszText		=	objects[i];
        lvi.cchTextMax	=	wcslen(lvi.pszText);

        VERIFY((ret = ListView_InsertItem(m_hwndlist, &lvi))!= -1);
        ListView_SetCheckState(m_hwndlist, ret, TRUE);
    }

    return;
}

wyInt32
EmptyDB::OnWmCommand(WPARAM wParam)
{
	wyInt32	ret;
    switch(LOWORD(wParam))
	{
    case IDC_EMPTYDBSELECTALL:
        if(SendMessage(m_hwndselectall, BM_GETCHECK , 0, 0) == BST_CHECKED)
			SetCheckBoxes(wyTrue);
		else
			//uncheck all list items on deselectall
			SetCheckBoxes(wyFalse);
        break;

    case IDOK:

		ret = EmptyDatabase();
		
        if(ret)
        {
            //if tooltip is there, destroy it
            /*if(m_hwndtooltip)
                DestroyWindow(m_hwndtooltip);*/
			VERIFY(yog_enddialog(m_hwnddlg, 1));
        }

        break;

	case IDCANCEL:

        //if tooltip is there, destroy it
        /*if(m_hwndtooltip)
            DestroyWindow(m_hwndtooltip);*/

		VERIFY(yog_enddialog(m_hwnddlg, 0));
		return 0;
	}
    return 1;
}

wyInt32
EmptyDB::EmptyDatabase()
{
    wyString    query, msg, createdb;
    MYSQL_RES   *res;
    wyInt32     nodeleteobj = 0, count, itemcount;
    wyInt32		state;
	MDIWindow	*wnd = GetActiveWin();
	
	if(IsAnyItemSelected() == wyFalse)
	{
		yog_message(m_hwnddlg, _(L"Please select atleast one object"), 
            pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
		return wyFalse;
	}	
#ifndef COMMUNITY
	wyInt32 presult = 6;
	if(wnd->m_ptransaction && wnd->m_ptransaction->m_starttransactionenabled == wyFalse)
	{
		if(pGlobals->m_pcmainwin->m_topromptonimplicit)
			presult = MessageBox(GetActiveWindow(), _(L"You have an active transaction. This operation will cause Transaction to commit and end. Do you want to Continue?"), 
						_(L"Warning"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
		if(presult == 6)
		{
			wyString	query;
			wyInt32		isintransaction =	1;
			wnd->m_ptransaction->m_implicitcommit = wyTrue;
			query.Sprintf("COMMIT");
			ExecuteAndGetResult(wnd, m_tunnel, m_mysql, query, wyFalse, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);
				
			if(isintransaction == 1)
				return wyFalse;
		} 
		else
			return wyFalse;
	}
#endif
	SetCursor(LoadCursor(NULL, IDC_WAIT));

    SetGroupProcess(wnd, wyTrue);

	/* starting from 4.0 RC1 we isse set_foreign_key checks = 0 command 
	With http this is not useful, FK check disabling needs with each batch(queries send at a time)
	*/
	if(m_tunnel->IsTunnel() == false)
	{
		query.Sprintf("set FOREIGN_KEY_CHECKS=0"); 
		res = ExecuteAndGetResult(wnd, m_tunnel, m_mysql, query);
		if(res)
			m_tunnel->mysql_free_result(res);
	}

	VERIFY(itemcount = ListView_GetItemCount(m_hwndlist));	
	for(count = 0; count < itemcount; count++)
	{
		state = ListView_GetCheckState(m_hwndlist, count);

		if(state)
		{
			if(count == 0)
			{/* Drop all tables */
				nodeleteobj += wnd->m_pcqueryobject->DropTables(wnd->m_hwnd, m_tunnel, m_mysql, m_db.GetString());
			}
			else if(count == 1)
			/* Drop all views */
				nodeleteobj += wnd->m_pcqueryobject->DropViews(wnd->m_hwnd, m_tunnel, m_mysql, m_db.GetString());
			else if(count == 2)	
			/*Drop all Procedures*/
				nodeleteobj += wnd->m_pcqueryobject->DropProcedures(wnd->m_hwnd, m_tunnel, m_mysql, m_db.GetString());
			else if( count == 3)
			/*Drop all Functions  */
				nodeleteobj +=	wnd->m_pcqueryobject->DropFunctions(wnd->m_hwnd, m_tunnel, m_mysql, m_db.GetString());
			else if(count == 4 && m_isalltables == wyFalse)
			/* Drop all triggers*/
				nodeleteobj +=	wnd->m_pcqueryobject->DropTriggers(wnd->m_hwnd, m_tunnel, m_mysql, m_db.GetString());
			else if(count == 5)
			/*Drop all Events*/
				nodeleteobj +=	wnd->m_pcqueryobject->DropEvents(wnd->m_hwnd, m_tunnel, m_mysql, m_db.GetString());
		}
	}

	//With http this is not useful, FK check disabling needs with each batch(queries send at a time)
	if(m_tunnel->IsTunnel() == false)
	{
		query.Sprintf("set FOREIGN_KEY_CHECKS=1"); 
		res = ExecuteAndGetResult(wnd, m_tunnel, m_mysql, query);

		if(res)
			m_tunnel->mysql_free_result(res);
	}
	
	// if emptying allready emptied database then no need to rebuild again
	if(nodeleteobj != 0)
	{
		SpecificDBRebuild((wyChar*)m_db.GetString());

		createdb.Sprintf("Create database `%s`", m_db.GetString());

		//For creating an Empty row with this emtied db.
		pGlobals->m_pcmainwin->m_connection->HandlerAddToMessageQueue(wnd, (wyChar*)createdb.GetString());
	}
	
	return wyTrue;
}

void
EmptyDB::SetCheckBoxes(wyBool state)
{
	wyUInt32 count,itemcount;

	VERIFY(itemcount = ListView_GetItemCount(m_hwndlist));
	
	for(count = 0; count < itemcount; count++)
		ListView_SetCheckState(m_hwndlist, count,state);
}

wyBool
EmptyDB::IsAnyItemSelected()
{
	wyInt32	count, itemcount;

	itemcount = ListView_GetItemCount(m_hwndlist);

	for(count = 0; count < itemcount; count++)
	{
		if(ListView_GetCheckState(m_hwndlist, count))
			return wyTrue;
	}
	return wyFalse;
}

void 
EmptyDB::EmptyDbListViewEvent()
{
	wyInt32	itemcount, count;
	wyBool	flag = wyFalse;

	itemcount =	ListView_GetItemCount(m_hwndlist);

	for(count = 0; count < itemcount; count++) 
	{
		if(ListView_GetCheckState(m_hwndlist, count) == wyFalse) 
		{	
			//set the select/unselect to be unchecked if atleast one is not selected.
			SendMessage(m_hwndselectall, BM_SETCHECK, (WPARAM)0, (LPARAM)0);
			flag = wyTrue;
		}
	}

	//if everything is checked we will check the Select All option 
	if(flag == wyFalse)
		SendMessage(m_hwndselectall, BM_SETCHECK , BST_CHECKED, 0);

	//Tables

	if(ListView_GetCheckState(m_hwndlist, 0) == wyTrue) 
	{	//Tables is selected, then we we select trigger also.
		// Drop table query will drop both tables and triggers
		m_isalltables = wyTrue;

		//checking whether triggers is checked or not
		if(ListView_GetCheckState(m_hwndlist, 4) == wyFalse) 
		{	//if triggers is not chacked, then we are checking triggers.
			ListView_SetCheckState(m_hwndlist, 4, wyTrue);
			m_istriggerchecked = wyFalse;
		}
		else
			m_istriggerchecked = wyTrue;
	}
	else
	{
		m_isalltables = wyFalse;
		//if we check tables,it will check Triggers also.
		//if we are unchecking tables then we need to uncheck the triggers if it is checked by checking in Tables folder
		if(m_istriggerchecked == wyFalse)
			ListView_SetCheckState(m_hwndlist, 4, wyFalse);
	}

	if(ListView_GetCheckState(m_hwndlist, 4) == wyFalse && m_isalltables == wyTrue) 
	{
		ListView_SetCheckState(m_hwndlist, 4, wyTrue);
	}
	else if(ListView_GetCheckState(m_hwndlist, 4) == wyFalse)
		m_istriggerchecked = wyTrue;
	
	return;
}

/* ----------------------------------------------------------------------------------------
	Implementation of Query Preview dialog box. This dialog box is used when the user wants
	to empty the database.
   --------------------------------------------------------------------------------------*/

//Constructor
QueryPreview::QueryPreview(wyString* pkeywords, wyString* pfunctions)
{
    m_pkeywords = pkeywords;
    m_pfunctions = pfunctions;
}

// Destructor.
QueryPreview::~QueryPreview()
{

}

wyInt32
QueryPreview::Create(HWND hwndparent, wyChar *query)
{
	wyInt32		ret = 0;
   
	m_query.SetAs(query);

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_QUERYPREVIEW), hwndparent,
							QueryPreview::QueryPreviewDialogProc,(LPARAM)this);
	return ret;
}

INT_PTR CALLBACK
QueryPreview::QueryPreviewDialogProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam)
{
	QueryPreview *querypreview = (QueryPreview*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
	    SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		return wyTrue;
		
	case WM_INITDLGVALUES:
        querypreview->m_hwnddlg = hwnd;
        querypreview->InitDlg();
       	break;

	case WM_COMMAND:
		if(LOWORD(wparam) == IDCANCEL)
			yog_enddialog(hwnd, 0);
		break;

	case WM_SIZE:
		querypreview->Resize(hwnd);
		break;

	case WM_PAINT:
		//draws the resize gripper at bottom-right corner of dialog
		DrawSizeGripOnPaint(hwnd);
	break;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO pMMI = (LPMINMAXINFO)lparam;
			pMMI->ptMinTrackSize.x = RELDLGWD;
			pMMI->ptMinTrackSize.y = RELDLGHT;
		}
		break;
		
	case WM_DESTROY:
		//StoreDialogPersist(hwnd, SHOWPREVIEW_SECTION);
		break;

	}
	return wyFalse;
}

void
QueryPreview::InitDlg()
{
	MDIWindow	*wnd;
	HWND hwndedit;
	HICON hicon;
	
	VERIFY(wnd = GetActiveWin());

	//Set icon for dialog	
	hicon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_TABPREVIEW));
	SendMessage(m_hwnddlg, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hicon);
    DestroyIcon(hicon);
	
	hwndedit = GetDlgItem(m_hwnddlg, IDC_QUERYPREVIEW);

	SetWindowLongPtr(hwndedit, GWLP_USERDATA,(LONG_PTR)this);
	m_wporigtextproc =(WNDPROC)SetWindowLongPtr(hwndedit, GWLP_WNDPROC,(LONG_PTR)TextProc);

	//Set scintilla properties
	if(wnd)
    {
		SetScintillaModes(hwndedit, wnd->m_keywordstring, wnd->m_functionstring, wyTrue);
    }
    else if(m_pkeywords && m_pfunctions)
    {
        SetScintillaModes(hwndedit, *m_pkeywords, *m_pfunctions, wyTrue);
    }
	
	//Format query
#ifdef COMMUNITY
	pGlobals->m_pcmainwin->m_connection->FormateAllQueries(wnd,
		hwndedit, (wyChar *) m_query.GetString(), ALL_QUERY);
#else
	SendMessage(hwndedit, SCI_SETTEXT, m_query.GetLength(), (LPARAM)m_query.GetString());

	Format(hwndedit, IsStacked(), GetLineBreak() ? wyFalse : wyTrue, FORMAT_ALL_QUERY, GetIndentation());
	SendMessage(hwndedit, SCI_SETSELECTIONSTART, (WPARAM)0, 0);
	SendMessage(hwndedit, SCI_SETSELECTIONEND, (WPARAM)0, 0);
#endif

	SendMessage(hwndedit, SCI_SETREADONLY, true, 0);

    //Line added because on changing color there was margin coming for editor
    SendMessage(hwndedit, SCI_SETMARGINWIDTHN,1,0);
	

	//Commented to solve multi-monitor issues
	//SetDialogPos(m_hwnddlg, SHOWPREVIEW_SECTION);
	Resize(m_hwnddlg);

}

//resizes the dialog
void
QueryPreview::Resize(HWND hwnd)
{
	HWND    hwndedit, hwndbutton;
	RECT	rctedit, rctbutton, rctdlg,rcttmp;
	wyInt32	buttonht = 0, buttonwd = 0, height = 0;
	wyInt32 top = 0, left = 0;

	hwndbutton = GetDlgItem(hwnd, IDCANCEL);
	hwndedit = GetDlgItem(hwnd, IDC_QUERYPREVIEW);
	
	GetClientRect(hwnd, &rctdlg);
	GetClientRect(hwndbutton, &rctbutton);
	
	buttonht = rctbutton.bottom;
	buttonwd = rctbutton.right;

	top = rctdlg.bottom - buttonht - WIDTHBETBUTTON;
	left = rctdlg.right - buttonwd - WIDTHBETBUTTON; 

	MoveWindow(hwndbutton, left, top, buttonwd, buttonht, TRUE);//Cancel

	height = rctdlg.bottom - buttonht - 25;
	MoveWindow(hwndedit, 8, 8, rctdlg.right - WIDTHBETBUTTON - 8, height, FALSE); 

	GetWindowRect(hwndedit, &rctedit);
	MapWindowPoints(NULL, hwnd, (LPPOINT)&rctedit, 2);

	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd);

	//left of the editor
	rcttmp.top = rctdlg.top;
	rcttmp.left = rctdlg.left;
	rcttmp.right = rctedit.left;
	rcttmp.bottom = rctdlg.bottom;
	InvalidateRect(hwnd, &rcttmp, TRUE);
	UpdateWindow(hwnd);	

	//top of editor
	rcttmp.top = rctdlg.top;
	rcttmp.left = rctdlg.left;
	rcttmp.right = rctdlg.right;
	rcttmp.bottom = rctedit.top;
	InvalidateRect(hwnd, &rcttmp, TRUE);
	UpdateWindow(hwnd);	

	//right of editor
	rcttmp.top = rctedit.top;
	rcttmp.left = rctedit.right;
	rcttmp.right = rctdlg.right;
	rcttmp.bottom = rctedit.bottom;
	InvalidateRect(hwnd, &rcttmp, TRUE);
	UpdateWindow(hwnd);	

	//bottom of editor
	rcttmp.top = rctedit.bottom;
	rcttmp.left = rctdlg.left;
	rcttmp.right = rctdlg.right;
	rcttmp.bottom = rctdlg.bottom;
	InvalidateRect(hwnd, &rcttmp, TRUE);
	UpdateWindow(hwnd);	
}

LRESULT CALLBACK
QueryPreview::TextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	QueryPreview	*pib = (QueryPreview*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	wyInt32			key;

    switch(message)
	{
	//if focus is on editor and user is pressing the esc key
	case WM_KEYDOWN:
		 key = LOWORD(wParam);
		 if(key == VK_ESCAPE)
         {
			VERIFY(yog_enddialog( pib->m_hwnddlg, 0));
			  return 0;
         }
	}
	return CallWindowProc(pib->m_wporigtextproc, hwnd, message, wParam, lParam);
}

///Enum/Set value List
//Constructor
ValueList::ValueList(PMYSQL mysql)
{
	this->m_mysql = mysql;
}

// Destructor.
ValueList::~ValueList()
{

}

wyInt32
ValueList::Create(HWND hwnd, wyString *values, wyBool isenum)
{
	wyInt32		ret = 0;
	
	if(values)
		m_values.SetAs(values->GetString());

	m_isenum = isenum;
   
	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_ENUMSETVALUES), hwnd,
							ValueList::ValueListDialogProc,(LPARAM)this);
	if(ret == 1)
		values->SetAs(m_values.GetString());

	return ret;
}

INT_PTR CALLBACK
ValueList::ValueListDialogProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam)
{
	ValueList *valuelist = (ValueList*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
	    SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		return wyTrue;
		
	case WM_INITDLGVALUES:
		valuelist->InitDlg(hwnd);
       	break;

	case WM_COMMAND:
		valuelist->OnWmCommand(hwnd, wparam, lparam);
		break;
	}
	return wyFalse;
}

void
ValueList::InitDlg(HWND hwnd)
{
	m_hwnddlg	= hwnd;
	m_hwndlist	= GetDlgItem(hwnd, IDC_VALUELIST);

	InitListBox();

	if(m_isenum == wyTrue)
		SetWindowText(m_hwnddlg, _(L"Enum Value List"));
	else
		SetWindowText(m_hwnddlg, _(L"Set Value List"));

	EnableWindow(GetDlgItem(m_hwnddlg, IDC_ADD), wyTrue);
	EnableWindow(GetDlgItem(m_hwnddlg, IDC_REPLACE), wyFalse);
	EnableWindow(GetDlgItem(m_hwnddlg, IDC_REMOVE), wyFalse);
	EnableWindow(GetDlgItem(m_hwnddlg, IDC_UP), wyFalse);
	EnableWindow(GetDlgItem(m_hwnddlg, IDC_DOWN), wyFalse);
	EnableWindow(GetDlgItem(m_hwnddlg, IDOK), FALSE);

	SetFocus(GetDlgItem(m_hwnddlg, IDC_VALUE));
}

//Initializes the listbox
wyBool
ValueList::InitListBox()
{
	wyString	textstr;
	wyBool		close = wyFalse;
	wyInt32		j, i = 1;//i=1 because searching start from the second character(ie, we are not checking the first character('))
	wyChar		*valuelist = NULL;

	if(m_values.GetLength() == 0)
		return wyFalse;

	valuelist = AllocateBuff(strlen(m_values.GetString()) + 1);
	strcpy(valuelist, m_values.GetString()); 

	if(valuelist[0] != C_SINGLE_QUOTE)
		return wyFalse;   
	
	SendMessage(GetDlgItem(m_hwnddlg, IDC_VALUELIST), LB_RESETCONTENT, 0, 0);

	while(!close)
    {
		wyChar *text = (wyChar*)calloc(sizeof(wyChar), strlen(m_values.GetString()) + 1);
		
		for(j = 0; valuelist[i]; j++, i = i + 1)
        {
			if(valuelist[i] == C_SINGLE_QUOTE)
            {
				//if it is null 
				if(!valuelist[i+1])
				{
					close = wyTrue;
					break;
				}//if it is comma, then we are stripping three characters
				else if(valuelist[i+1] == C_COMMA) 
				{  
					i = i + 3;
					break;
				}
				//else if(valuelist[i+1] == C_SINGLE_QUOTE || valuelist[i+1] == C_DOUBLE_QUOTE || valuelist[i+1] == '/')
				else if(valuelist[i+1] == C_SINGLE_QUOTE)
				{
					i++;
				}
			}
			//we are calling mysql_real_escape_string function before showing all enum list values into the "length" field of the "Alter Table dialog" and it may
			//	be possible that user again opens the ENUM Value List dialog-box.. At that time '/' should be handled..
			else if(valuelist[i] == '\\')
			{
				if(valuelist[i+1] == C_SINGLE_QUOTE || valuelist[i+1] == C_DOUBLE_QUOTE || valuelist[i+1] == '\\')
					i++;
			}
			
			text[j] = valuelist[i];
		}	

		//add value to listbox
		textstr.SetAs(text);
		SendMessage(GetDlgItem(m_hwnddlg, IDC_VALUELIST), LB_ADDSTRING, 0, (LPARAM)textstr.GetAsWideChar());
        free(text);
	}

	if(valuelist)
		free(valuelist);

	return wyTrue;
}

void
ValueList::OnWmCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	wyInt32 cursel;

	if(HIWORD(wParam) == LBN_SELCHANGE)
	{
		cursel =  SendMessage(m_hwndlist, LB_GETCURSEL, 0, 0);
		if(cursel >= 0)
			EnableWindow(GetDlgItem(m_hwnddlg, IDC_REMOVE), wyTrue);
		else
			EnableWindow(GetDlgItem(m_hwnddlg, IDC_REMOVE), wyFalse);

		EnableOrDisableUpAndDown();
	}
	else if(HIWORD(wParam) == EN_CHANGE)
	{
		EnableOrDisableValueButtons();
	}
	else if(HIWORD(wParam)== LBN_DBLCLK)
	{
		SetValue();
	}

	switch(LOWORD(wParam))
	{
	case IDCANCEL:
        yog_enddialog(hwnd, 0);
		break;

	case IDOK:
		ProcessOK();
		break;

	case IDC_ADD:
		AddValue();
		break;
	
	case IDC_REPLACE:
		ReplaceValue();
		break;

	case IDC_UP:
		MoveUpOrDown(wyTrue);
		break;

	case IDC_DOWN:
		MoveUpOrDown(wyFalse);
		break;

	case IDC_REMOVE:
		RemoveValue();
		break;		
	}
}

void
ValueList::ProcessOK()
{
	wyInt32		count, i, textlen;
	wyWChar		*value = NULL;
	wyString	valuestr, valuelist, valuetemp;
    wyChar      *newstr;
    MDIWindow * wnd = NULL;

    wnd	 = GetActiveWin();

	count = SendMessage(m_hwndlist, (UINT)LB_GETCOUNT, (WPARAM)0, (LPARAM)0);
    
	for(i = 0; i < count; i++)
    {
        textlen = SendMessage(m_hwndlist, (UINT)LB_GETTEXTLEN,(WPARAM)i, (LPARAM)0);
        value	= AllocateBuffWChar(textlen + 1);
		
		SendMessage(m_hwndlist, (UINT)LB_GETTEXT,(WPARAM)i, (LPARAM)value);
		valuetemp.SetAs(value);

        newstr  = AllocateBuff( (valuetemp.GetLength()*2) + 1);

        wnd->m_tunnel->mysql_real_escape_string(*m_mysql,newstr, valuetemp.GetString(), valuetemp.GetLength());
		
		valuetemp.SetAs(newstr);
		valuelist.AddSprintf("'%s',", valuetemp.GetString());

		free(value);
        free(newstr);
	}

	if(valuelist.GetLength() != 0)
	{
		//stripping the comma character
		valuelist.Strip(1);
	}

	//Setting the new enum values 
	m_values.SetAs(valuelist);

	yog_enddialog(m_hwnddlg, 1);
}

wyBool
ValueList::AddValue()
{
	wyInt32		ret;
	wyWChar		value[SIZE_512] = {0};
	wyString	valuestr, listvalue;
	wyInt32		count, i, textlen;
	wyWChar		*lstvalue = NULL;

	//getting the value from the listbox
	SendMessage(GetDlgItem(m_hwnddlg, IDC_VALUE), WM_GETTEXT, SIZE_512 -1,(LPARAM) value); 
	valuestr.SetAs(value);

	//trimming all the spaces before and after the value
	valuestr.LTrim();
	valuestr.RTrim();

	count = SendMessage(m_hwndlist, (UINT)LB_GETCOUNT, (WPARAM)0, (LPARAM)0);
    
	for(i = 0; i < count; i++)
    {
        textlen = SendMessage(m_hwndlist, (UINT)LB_GETTEXTLEN,(WPARAM)i, (LPARAM)0);
        lstvalue	= AllocateBuffWChar(textlen + 1);
		SendMessage(m_hwndlist, (UINT)LB_GETTEXT,(WPARAM)i, (LPARAM)lstvalue);
		listvalue.SetAs(lstvalue);

		if(listvalue.CompareI(valuestr.GetString()) == 0)
		{
			yog_message(m_hwnddlg, _(L"Duplicate value"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
			SetFocus(GetDlgItem(m_hwnddlg, IDC_VALUE));
			SendMessage(GetDlgItem(m_hwnddlg, IDC_VALUE), EM_SETSEL, 0, -1);
			EnableWindow(GetDlgItem(m_hwnddlg, IDC_ADD), wyFalse);
			EnableWindow(GetDlgItem(m_hwnddlg, IDC_REPLACE), wyFalse);
			free(lstvalue);
			return wyFalse;
		}

		free(lstvalue);
	}
	
	//Adding the value to the listbox
	ret = SendMessage(m_hwndlist, LB_ADDSTRING, 0, (LPARAM)valuestr.GetAsWideChar());
	SendMessage(m_hwndlist, LB_SETCURSEL, ret, 0);

	EnableWindow(GetDlgItem(m_hwnddlg, IDOK), TRUE);
	EnableWindow(GetDlgItem(m_hwnddlg, IDC_ADD), wyFalse);
	EnableWindow(GetDlgItem(m_hwnddlg, IDC_REPLACE), wyFalse);
	EnableWindow(GetDlgItem(m_hwnddlg, IDC_REMOVE), wyTrue);
	EnableOrDisableUpAndDown();
	
	//Setting selection on the value
	SendMessage(GetDlgItem(m_hwnddlg, IDC_VALUE), EM_SETSEL, 0, -1);
	SetFocus(GetDlgItem(m_hwnddlg, IDC_VALUE));

	return wyTrue;
}

wyBool
ValueList::ReplaceValue()
{
	wyBool		ret;
	wyWChar		value[SIZE_512] = {0};
	wyString	valuestr, listvalue;
	wyInt32		cursel;
	wyInt32		count, i, textlen;
	wyWChar		*lstvalue = NULL;

	cursel =  SendMessage(m_hwndlist, LB_GETCURSEL, 0, 0);

	//getting the value from the listbox
	SendMessage(GetDlgItem(m_hwnddlg, IDC_VALUE), WM_GETTEXT, SIZE_512 -1,(LPARAM) value); 
	valuestr.SetAs(value);

	//trimming all the spaces before and after the value
	valuestr.LTrim();
	valuestr.RTrim();

	count = SendMessage(m_hwndlist, (UINT)LB_GETCOUNT, (WPARAM)0, (LPARAM)0);
    
	for(i = 0; i < count; i++)
    {
        textlen = SendMessage(m_hwndlist, (UINT)LB_GETTEXTLEN,(WPARAM)i, (LPARAM)0);
        lstvalue	= AllocateBuffWChar(textlen + 1);
		SendMessage(m_hwndlist, (UINT)LB_GETTEXT,(WPARAM)i, (LPARAM)lstvalue);
		listvalue.SetAs(lstvalue);

		if(listvalue.CompareI(valuestr.GetString()) == 0)
		{
			yog_message(m_hwnddlg, _(L"Duplicate value"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
			SetFocus(GetDlgItem(m_hwnddlg, IDC_VALUE));
			SendMessage(GetDlgItem(m_hwnddlg, IDC_VALUE), EM_SETSEL, 0, -1);
			EnableWindow(GetDlgItem(m_hwnddlg, IDC_ADD), wyFalse);
			EnableWindow(GetDlgItem(m_hwnddlg, IDC_REPLACE), wyFalse);
			free(lstvalue);
			return wyFalse;
		}

		free(lstvalue);
	}
	
	//Deleting the value from the listbox
	SendMessage(m_hwndlist, LB_DELETESTRING, cursel, 0);

	//Inserting the value in the current position
	ret = (wyBool)SendMessage(m_hwndlist, LB_INSERTSTRING, cursel, (LPARAM)valuestr.GetAsWideChar());
	SendMessage(m_hwndlist, LB_SETCURSEL, cursel, 0);

	EnableWindow(GetDlgItem(m_hwnddlg, IDOK), TRUE);
	EnableWindow(GetDlgItem(m_hwnddlg, IDC_ADD), wyFalse);
	EnableWindow(GetDlgItem(m_hwnddlg, IDC_REPLACE), wyFalse);
	EnableWindow(GetDlgItem(m_hwnddlg, IDC_REMOVE), wyTrue);
	EnableOrDisableUpAndDown();

	//Setting selection on the value
	SendMessage(GetDlgItem(m_hwnddlg, IDC_VALUE), EM_SETSEL, 0, -1);
	SetFocus(GetDlgItem(m_hwnddlg, IDC_VALUE));

	return ret;
}

wyBool
ValueList::MoveUpOrDown(wyBool isup)
{
	wyInt32		cursel,ret;
	wyInt32		textlen;
	wyWChar		*value = NULL;
	wyString	valuestr;

	cursel =  SendMessage(m_hwndlist, LB_GETCURSEL, 0, 0);

	//getting the value from the listbox
	textlen = SendMessage(m_hwndlist, (UINT)LB_GETTEXTLEN,(WPARAM)cursel, (LPARAM)0);
    value = AllocateBuffWChar(textlen + 1);
    SendMessage(m_hwndlist, (UINT)LB_GETTEXT,(WPARAM)cursel, (LPARAM)value);
	valuestr.SetAs(value);
	
	//Deleting the value from the listbox
	SendMessage(m_hwndlist, LB_DELETESTRING, cursel, 0);

	//Inserting the value depending on the Up/Down button clicked
	if(isup == wyTrue)
		cursel = cursel - 1;
	else
		cursel = cursel + 1;

	ret = SendMessage(m_hwndlist, LB_INSERTSTRING, cursel, (LPARAM)valuestr.GetAsWideChar());
	SendMessage(m_hwndlist, LB_SETCURSEL, cursel, 0);
	
	EnableWindow(GetDlgItem(m_hwnddlg, IDOK), TRUE);
	EnableOrDisableUpAndDown();

	if(value)
		free(value);

	return wyTrue;
}

wyBool
ValueList::SetValue()
{
	wyInt32		count, textlen, cursel;
	wyWChar		*value = NULL;
	wyString	valuestr, valuelist;

	cursel =  SendMessage(m_hwndlist, LB_GETCURSEL, 0, 0);
	count = SendMessage(m_hwndlist, (UINT)LB_GETCOUNT, (WPARAM)0, (LPARAM)0);

	if(count = 0 || cursel < 0)
	{
		EnableWindow(GetDlgItem(m_hwnddlg, IDC_ADD), wyTrue);
		EnableWindow(GetDlgItem(m_hwnddlg, IDC_REMOVE), wyFalse);
		EnableWindow(GetDlgItem(m_hwnddlg, IDC_REPLACE), wyFalse);
		EnableOrDisableUpAndDown();
		return wyFalse;
	}

	//getting the value from the listbox
	textlen = SendMessage(m_hwndlist, (UINT)LB_GETTEXTLEN,(WPARAM)cursel, (LPARAM)0);
    value = AllocateBuffWChar(textlen + 1);
    SendMessage(m_hwndlist, (UINT)LB_GETTEXT,(WPARAM)cursel, (LPARAM)value);
	valuestr.SetAs(value);

	//Setting the value to the edit box
	SendMessage(GetDlgItem(m_hwnddlg, IDC_VALUE), WM_SETTEXT, 0, (LPARAM)valuestr.GetAsWideChar());
	
	//Setting selection on the value
	SendMessage(GetDlgItem(m_hwnddlg, IDC_VALUE), EM_SETSEL, 0, -1);
	SetFocus(GetDlgItem(m_hwnddlg, IDC_VALUE));
    
	EnableOrDisableUpAndDown();
	free(value);
	
	return wyTrue;
}

void
ValueList::EnableOrDisableUpAndDown()
{
	wyInt32	count, cursel;
	
	cursel =  SendMessage(m_hwndlist, LB_GETCURSEL, 0, 0);
	count = SendMessage(m_hwndlist, (UINT)LB_GETCOUNT, (WPARAM)0, (LPARAM)0);

	if(count == 1 || count == 0 || cursel < 0)
	{
		EnableWindow(GetDlgItem(m_hwnddlg, IDC_UP), wyFalse);
		EnableWindow(GetDlgItem(m_hwnddlg, IDC_DOWN), wyFalse);
	}
	else
	{
		if(cursel == 0)
			EnableWindow(GetDlgItem(m_hwnddlg, IDC_UP), wyFalse);
		else
			EnableWindow(GetDlgItem(m_hwnddlg, IDC_UP), wyTrue);
		
		if(cursel == count - 1)
			EnableWindow(GetDlgItem(m_hwnddlg, IDC_DOWN), wyFalse);
		else
			EnableWindow(GetDlgItem(m_hwnddlg, IDC_DOWN), wyTrue);
	}
}

wyBool
ValueList::RemoveValue()
{
	wyInt32		cursel, count;

	cursel =  SendMessage(m_hwndlist, LB_GETCURSEL, 0, 0);
	
	SendMessage(m_hwndlist, LB_DELETESTRING, cursel, 0);
	//Setting the value to the edit box
	SendMessage(GetDlgItem(m_hwnddlg, IDC_VALUE), WM_SETTEXT, 0, (LPARAM)L"");
	count = SendMessage(m_hwndlist, (UINT)LB_GETCOUNT, (WPARAM)0, (LPARAM)0);
	
	//if last element
	if(cursel == count)
		cursel = cursel - 1;    

	SendMessage(m_hwndlist, LB_SETCURSEL, (cursel == 0) ? 0 : cursel, 0);
	SetValue();
	EnableWindow(GetDlgItem(m_hwnddlg, IDOK), TRUE);

	return wyTrue;
}

void
ValueList::EnableOrDisableValueButtons()
{
	wyInt32		count, textlen, cursel;
	wyWChar		*oldvalue = NULL;
	wyString	oldvaluestr, newvalue;
	wyWChar		value[SIZE_512] = {0};

	cursel =  SendMessage(m_hwndlist, LB_GETCURSEL, 0, 0);
	count = SendMessage(m_hwndlist, (UINT)LB_GETCOUNT, (WPARAM)0, (LPARAM)0);

	SendMessage(GetDlgItem(m_hwnddlg, IDC_VALUE), WM_GETTEXT, SIZE_512 -1,(LPARAM)value); 
	newvalue.SetAs(value);
	
	//if there is no element or current selection is -1
	if(count == 0 || cursel < 0)
	{
		EnableWindow(GetDlgItem(m_hwnddlg, IDC_REMOVE), wyFalse);
		EnableWindow(GetDlgItem(m_hwnddlg, IDC_REPLACE), wyFalse);
		EnableWindow(GetDlgItem(m_hwnddlg, IDC_ADD), wyTrue);
		return;
	}

	textlen = SendMessage(m_hwndlist, (UINT)LB_GETTEXTLEN,(WPARAM)cursel, (LPARAM)0);
    oldvalue = AllocateBuffWChar(textlen + 1);
    
	SendMessage(m_hwndlist, (UINT)LB_GETTEXT,(WPARAM)cursel, (LPARAM)oldvalue);

	oldvaluestr.SetAs(oldvalue);

	//if the value entered in the edit box is different from the the listbox selected item
	if(newvalue.CompareI(oldvaluestr.GetString()) != 0)
	{
		EnableWindow(GetDlgItem(m_hwnddlg, IDC_ADD), wyTrue);
		EnableWindow(GetDlgItem(m_hwnddlg, IDC_REPLACE), wyTrue);
	}
	else
	{
		EnableWindow(GetDlgItem(m_hwnddlg, IDC_ADD), wyFalse);
		EnableWindow(GetDlgItem(m_hwnddlg, IDC_REPLACE), wyFalse);
	}
	EnableWindow(GetDlgItem(m_hwnddlg, IDC_REMOVE), wyTrue);

	if(oldvalue)
		free(oldvalue);
}

////Connection Color dialog box
////Default constructor
ConnColorDlg::ConnColorDlg()
{
	m_changecolor = wyFalse;
}

//Default destructor
ConnColorDlg::~ConnColorDlg()
{
}

//Displayes the dialog box
wyInt32 
ConnColorDlg::ShowConnColorDlg(HWND hwndparent)
{
	wyInt32 ret;
	pGlobals->m_pcmainwin->m_connection->m_rgbconnection = pGlobals->m_pcmainwin->m_connection->m_rgbobbkcolor;
	pGlobals->m_pcmainwin->m_connection->m_rgbconnectionfg = pGlobals->m_pcmainwin->m_connection->m_rgbobfgcolor;

	
	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_OBCOLOR), 
						   hwndparent, ConnColorDlg::ConnColorDlgProc, (LPARAM)this);

	return	ret;
}

// Callback function for the invalid registration dialog box
INT_PTR CALLBACK
ConnColorDlg::ConnColorDlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	ConnColorDlg*	pcconncolor	=	(ConnColorDlg*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
   
	switch(message)
	{
	case WM_INITDIALOG:
		{
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
            LocalizeWindow(hwnd);
			PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		}
		break;

	case WM_INITDLGVALUES:
		{
			EnableWindow(GetDlgItem(hwnd, IDOK), wyFalse);
			ColorComboInitValues(GetDlgItem(hwnd, IDC_COLORCOMBO1));
			ColorComboFgInitValues(GetDlgItem(hwnd, IDC_COLORCOMBO2));
		}
		break;

	case WM_COMMAND:
        pcconncolor->OnWmColorCommand(hwnd, wparam, lparam);
		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/33-advanced-connection-settings");
		return 1;

		
	case WM_MEASUREITEM:
		{
			LPMEASUREITEMSTRUCT lpmis;
			lpmis = (LPMEASUREITEMSTRUCT) lparam;
			lpmis->itemWidth = 10;
			lpmis->itemHeight = 16;
		}
		break;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lparam;
			switch(lpdis->CtlID)
			{
				case IDC_COLORCOMBO:
				case IDC_COLORCOMBO1:
					OnDrawColorCombo(hwnd, (LPDRAWITEMSTRUCT)lparam, pGlobals->m_pcmainwin->m_connection->m_rgbconnection);
					break;
				case IDC_COLORCOMBO2:
					OnDrawColorCombo(hwnd, (LPDRAWITEMSTRUCT)lparam, pGlobals->m_pcmainwin->m_connection->m_rgbconnectionfg);
					break;
			}
		}
		break;
	}

	return 0;
}


//Function handles the WM_COMMAND on the main dialog window
void 
ConnColorDlg::OnWmColorCommand(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	wyString	msg;
	ConnectionBase *conbase = NULL;

	conbase = pGlobals->m_pcmainwin->m_connection;

	if(LOWORD(wparam)== IDC_COLORCOMBO1)
	{
		//Command handler for color combo box
		OnWmCommandColorCombo(hwnd, wparam, lparam);

		//if color is changed in combo, then enable OK button
		if(HIWORD(wparam)== CBN_SELCHANGE)
			EnableWindow(GetDlgItem(hwnd, IDOK), wyTrue);
	}
	else 
	if(LOWORD(wparam)== IDC_COLORCOMBO2)
	{
		//Command handler for color combo box
		OnWmCommandColorCombo(hwnd, wparam, lparam);

		//if color is changed in combo, then enable OK button
		if(HIWORD(wparam)== CBN_SELCHANGE)
			EnableWindow(GetDlgItem(hwnd, IDOK), wyTrue);
	}
	else if((LOWORD(wparam)) == IDOK)
	{
		conbase->m_rgbobbkcolor = conbase->m_rgbconnection;
		conbase->m_rgbobfgcolor = conbase->m_rgbconnectionfg;
		m_changecolor = wyTrue;
		yog_enddialog(hwnd, 0);
	}
	else if((LOWORD(wparam)) == IDCANCEL)
	{
		m_changecolor = wyFalse;
		yog_enddialog(hwnd, 0);
	}
	
	
	return;
}


////Connection Color dialog box
////Default constructor
RenameTabDlg::RenameTabDlg()
{
	
}

//Default destructor
RenameTabDlg::~RenameTabDlg()
{
}

wyInt32 
RenameTabDlg::ShowRenameTabDlg(HWND hwndparent)
{
	wyInt32 ret;

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_RENAMETAB), 
						   hwndparent, RenameTabDlg::RenameTabDlgProc, (LPARAM)this);

	return	ret;
}

// Callback function for the invalid registration dialog box
INT_PTR CALLBACK
RenameTabDlg::RenameTabDlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	
	CTCITEM pitem;
	pitem.m_mask = CTBIF_TEXT;
	wyString title;
	wyWChar     temptext[SIZE_1024];
	MDIWindow* wnd = GetActiveWin();
	
	switch(message)
	{
	case WM_INITDIALOG:
		{
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
            LocalizeWindow(hwnd);
			PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		}
		break;

	case WM_INITDLGVALUES:
		{
			//if(CustomTab_GetItem(wnd->m_pctabmodule->m_hwnd, CustomTab_GetCurSel(wnd->m_pctabmodule->m_hwnd), &pitem))
			//{
				CustomTab_GetTitle(wnd->m_pctabmodule->m_hwnd, CustomTab_GetCurSel(wnd->m_pctabmodule->m_hwnd), &title);
				SendMessage(GetDlgItem(hwnd, IDC_RENAMETAB), EM_LIMITTEXT, WPARAM(24), 0);
				SetWindowText(GetDlgItem(hwnd, IDC_RENAMETAB), title.GetAsWideChar());
			//}
			//else
				//SetWindowText(GetDlgItem(hwnd, IDC_RENAMETAB), L"Query");
			//EnableWindow(GetDlgItem(hwnd, IDOK), wyFalse);
			//ColorComboInitValues(GetDlgItem(hwnd, IDC_COLORCOMBO1));
			//ColorComboFgInitValues(GetDlgItem(hwnd, IDC_COLORCOMBO2));
		}
		break;

	case WM_COMMAND:
		if((LOWORD(wparam)) == IDOK)
		{
			GetWindowText(GetDlgItem(hwnd, IDC_RENAMETAB), temptext, sizeof(temptext)- 1);
			title.SetAs(temptext);
			if(IsQueryEmpty(title.GetAsWideChar()))
			{
				yog_message(hwnd, _(L"Please enter a valid name"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_HELP | MB_ICONINFORMATION);
				return 0;
			}
			wnd->m_pctabmodule->SetTabRename(temptext,wyFalse,wnd,wyTrue);
			yog_enddialog(hwnd, 0);
		}
		else 
		if((LOWORD(wparam)) == IDCANCEL)
		{
			yog_enddialog(hwnd, 0);
		}
        //pcconncolor->OnWmColorCommand(hwnd, wparam, lparam);
		break;

	//case WM_HELP:
	//	ShowHelp("Advanced%20Connection%20SQLyog%20MySQL%20Manager.htm");
	//	return 1;

		
	//case WM_MEASUREITEM:
	//	{
	//		LPMEASUREITEMSTRUCT lpmis;
	//		lpmis = (LPMEASUREITEMSTRUCT) lparam;
	//		lpmis->itemWidth = 10;
	//		lpmis->itemHeight = 16;
	//	}
	//	break;

	//case WM_DRAWITEM:
	//	{
	//		LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lparam;
	//		switch(lpdis->CtlID)
	//		{
	//			case IDC_COLORCOMBO:
	//			case IDC_COLORCOMBO1:
	//				OnDrawColorCombo(hwnd, (LPDRAWITEMSTRUCT)lparam, pGlobals->m_pcmainwin->m_connection->m_rgbconnection);
	//				break;
	//			case IDC_COLORCOMBO2:
	//				OnDrawColorCombo(hwnd, (LPDRAWITEMSTRUCT)lparam, pGlobals->m_pcmainwin->m_connection->m_rgbconnectionfg);
	//				break;
	//		}
	//	}
	//	break;
	}

	return 0;
}

//Function handles the WM_COMMAND on the main dialog window
void 
RenameTabDlg::OnWmColorCommand(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	/*wyString	msg;*/
	//ConnectionBase *conbase = NULL;

	//conbase = pGlobals->m_pcmainwin->m_connection;

	//if(LOWORD(wparam)== IDC_COLORCOMBO1)
	//{
	//	//Command handler for color combo box
	//	OnWmCommandColorCombo(hwnd, wparam, lparam);

	//	//if color is changed in combo, then enable OK button
	//	if(HIWORD(wparam)== CBN_SELCHANGE)
	//		EnableWindow(GetDlgItem(hwnd, IDOK), wyTrue);
	//}
	//else 
	//if(LOWORD(wparam)== IDC_COLORCOMBO2)
	//{
	//	//Command handler for color combo box
	//	OnWmCommandColorCombo(hwnd, wparam, lparam);

	//	//if color is changed in combo, then enable OK button
	//	if(HIWORD(wparam)== CBN_SELCHANGE)
	//		EnableWindow(GetDlgItem(hwnd, IDOK), wyTrue);
	//}
	if((LOWORD(wparam)) == IDOK)
	{
		//SetTabRename
		yog_enddialog(hwnd, 0);
	}
	else if((LOWORD(wparam)) == IDCANCEL)
	{
		yog_enddialog(hwnd, 0);
	}
	
	
	return;
}