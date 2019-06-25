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
#include "TableTabInterfaceTabMgmt.h"
#include "TableTabInterface.h"
#include "TabFields.h"
#include "TabCheck.h"

extern PGLOBALS		pGlobals;

#define UM_SETINITFOCUS 545

#define			HNDLEIDXMGWD		    400
#define			HNDLEIDXMGHT		    275
#define			GRIDCHECKBOXWD		    75

#define         UM_GRIDROWFOCUSCHANGE   4628

#define CONSTRAINTNAME       0
#define CONSTRAINTEXPRESSION    1

CheckColumn::CheckColumn(FieldStructWrapper *value)
{
	m_pcwrapobj = value;
	m_lenth = -1;
}

TabCheck::TabCheck(HWND hwnd, TableTabInterfaceTabMgmt* ptabmgmt)
{
	m_hwnd = hwnd;
	m_hgridtblcheckconst = NULL;
	m_hdlggrid = NULL;
	m_mdiwnd = GetActiveWin();
	m_ptabmgmt = ptabmgmt;
	m_ismysql41 = IsMySQL41(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));
	m_ismariadb52 = IsMySQL564MariaDB53(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));
	m_ismysql553 = IsMySQL553MariaDB55(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql));

	m_lastclickindgrid = -1;
	m_lastclickdlggrid = -1;

}

TabCheck::~TabCheck()
{
	//ClearAllMemory(wyTrue);
}

TabCheck::TabCheck()
{
	//ClearAllMemory(wyTrue);
}

CheckConstraintStructWrapper::CheckConstraintStructWrapper(CheckConstarintInfo *value, wyBool isnew)
{
	if (isnew)
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

CheckConstraintStructWrapper::~CheckConstraintStructWrapper()
{
	List    *lstcols = NULL;
	CheckColumn   *indcol1 = NULL, *indcol2 = NULL;

	if (m_oldval)
	{
		lstcols = m_oldval->m_listcolumns;

		if (lstcols)
			indcol1 = (CheckColumn*)lstcols->GetFirst();

		while (indcol1)
		{
			indcol2 = (CheckColumn*)lstcols->Remove(indcol1);
			delete indcol1;
			indcol1 = indcol2;
		}
		if (lstcols)
			delete lstcols;

		m_oldval->m_listcolumns = lstcols = NULL;
	}

	if (m_newval)
	{
		lstcols = m_newval->m_listcolumns;

		if (lstcols)
			indcol1 = (CheckColumn*)lstcols->GetFirst();

		while (indcol1)
		{
			indcol2 = (CheckColumn*)lstcols->Remove(indcol1);
			delete indcol1;
			indcol1 = indcol2;
		}
		if (lstcols)
			delete lstcols;

		m_newval->m_listcolumns = lstcols = NULL;
	}

	if (m_oldval == m_newval)
	{
		delete m_newval;
		m_oldval = m_newval = NULL;
	}
	else
	{
		if (m_oldval)
			delete m_oldval;

		if (m_newval)
			delete m_newval;
	}
	if (m_errmsg)
		delete m_errmsg;
	m_errmsg = NULL;
}

LRESULT CALLBACK
TabCheck::GridWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{


	TabCheck*     ptabind = (TabCheck*)CustomGrid_GetLongData(hwnd);
	wyString		tblname("");

	switch (message)
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
		return ptabind->OnGVNEndLabelEdit(wParam, lParam);
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
		//ptabind->ClearAllMemory(wyTrue);
		//ptabind->m_lastclickindgrid = -1;
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
		//if (ptabind->m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
			//ShowHelp("http://sqlyogkb.webyog.com/article/92-alter-index");
		//else
			ShowHelp("https://sqlyogkb.webyog.com/article/192-check-constraint");
	}
	}
	return 1;
}


void
TabCheck::InitGrid()
{

	wyInt32			counter;		// normal counter
	wyInt32			num_cols;		// number of columns
	GVCOLUMN		gvcol;			// structure used to create columns for grid
	wyChar		    *heading[] = { _("Constraint Name"), _("Expression") };
	wyInt32			mask[] = { GVIF_TEXT, GVIF_TEXT};
	VOID		    *listtype[] = { NULL, NULL };
	wyInt32			elemsize[] = { 0, 10};
	wyInt32			elemcount[] = { 0, 8};
	wyInt32			cx[] = { 200, 500 };
	wyInt32			format[] = { GVIF_LEFT, GVIF_LEFT };
	wyInt32			width = 0;

	wyString		colname;
	HFONT hfont;

	m_ptabmgmt->m_tabinterfaceptr->SetFont(m_hgridtblcheckconst);
	hfont = CustomGrid_GetColumnFont(m_hgridtblcheckconst);

	num_cols = sizeof(heading) / sizeof(heading[0]);
	//Bug: SQLyog closes abruptly on creating/altering the table with MySQL v5.0 server versions
	/*if (!m_ismysql553)
		num_cols--;*/

	for (counter = 0; counter < num_cols; counter++)
	{
		//for getting the retained column width
		colname.SetAs(heading[counter]);
		width = GetTextSize(colname.GetAsWideChar(), m_hgridtblcheckconst, hfont).right + 15; 

		memset(&gvcol, 0, sizeof(gvcol));

		gvcol.mask = mask[counter];		// Set the mask for the sturcture  i.e. what kind of column in the grid.
		gvcol.fmt = format[counter];		// Alignment
		gvcol.pszList = listtype[counter];	// set the kind list in between
		gvcol.cx = (width < cx[counter]) ? cx[counter] : width;
		gvcol.text = heading[counter];
		gvcol.nElemSize = elemsize[counter];
		gvcol.nListCount = elemcount[counter];
		gvcol.cchTextMax = strlen(heading[counter]);

		CustomGrid_InsertColumn(m_hgridtblcheckconst, &gvcol);
	}
	return;
}

void
TabCheck::ClearAllMemory(wyBool iscallfromdestructor)
{
	CheckConstraintStructWrapper *cwrapobj = NULL, *tmpcwrapobj = NULL;
	cwrapobj = (CheckConstraintStructWrapper *)m_listwrapperstruct.GetFirst();

	while (cwrapobj)
	{
		tmpcwrapobj = cwrapobj;
		cwrapobj = (CheckConstraintStructWrapper *)m_listwrapperstruct.Remove(cwrapobj);
		delete tmpcwrapobj;
	}

	return;
}


wyBool
TabCheck::Create()
{
	
	m_hgridtblcheckconst = CreateCustomGridEx(m_hwnd, 0, 0, 0, 0, (GVWNDPROC)GridWndProc, GV_EX_ROWCHECKBOX, (LPARAM)this);

	//..Initializing grid
	InitGrid();


	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

	if (m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
	{
		//..Fetching check constraints into wrapper
		if (!FetchCheckConstraintsIntoWrapper())
			return wyFalse;

		//..Setting checkconstraint into grid
		FillInitValues();
	}
	//..Inserting extra row
	InsertRow();

	ShowWindow(m_hgridtblcheckconst, SW_HIDE);
	return wyTrue;
}

wyBool
TabCheck::FetchCheckConstraintsIntoWrapper()
{
	wyString query ;
	MYSQL_RES *myres;
	MYSQL_ROW myfieldrow;
	wyString        tblname(""), dbname(""), createtable(""), checkexpression, checkname, alltblcheck(""), str(""), * allcheck = NULL;
	wyChar          *tempstr = NULL, *currentrowstr = NULL, *wholecreatestring = NULL, *wholecreate=NULL;
	CheckConstraintStructWrapper   *cwrapobj = NULL;
	CheckConstarintInfo                *icheck = NULL;
	FieldStructWrapper      *fieldswrap = NULL;
	wyChar * findc = "CONSTRAINT", *findch = "CHECK";
	wyBool found = wyFalse;

	tblname.SetAs(m_ptabmgmt->m_tabinterfaceptr->m_origtblname);
	tblname.FindAndReplace("`", "``");

	dbname.SetAs(m_ptabmgmt->m_tabinterfaceptr->m_dbname);
	dbname.FindAndReplace("`", "``");

	query.Clear();
	query.Sprintf("show create table `%s`.`%s`", dbname.GetString(), tblname.GetString());
	myres = ExecuteAndGetResult(m_mdiwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query);

	if (!myres)
	{
		ShowMySQLError(m_hwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query.GetString());
		return wyFalse;
	}
	
		myfieldrow = m_mdiwnd->m_tunnel->mysql_fetch_row(myres);
		createtable = myfieldrow[1];
		wholecreate =(wyChar*) createtable.GetString();

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
				if (!GettablelevelCheckConstraintValue(currentrowstr, &checkexpression))
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
				//If quotes are present in show create table 
				//if (AppendBackQuotes()) {
					CheckForQuotesAndReplace(&checkname);
				//}

				cwrapobj = new CheckConstraintStructWrapper(NULL, wyFalse);
				m_listwrapperstruct.Insert(cwrapobj);

				icheck = new CheckConstarintInfo();
				cwrapobj->m_oldval = cwrapobj->m_newval = icheck;
				icheck->m_name.SetAs(checkname);
				icheck->m_checkexpression.SetAs(checkexpression);

				cwrapobj = (CheckConstraintStructWrapper*)cwrapobj->m_next;

				//moving to next set of check constraint
				if (currentrowstr)
					currentrowstr = strtok(NULL, "\n");

			}
		}

		m_mdiwnd->m_tunnel->mysql_free_result(myres);
	
	

	return wyTrue;
}
wyBool TabCheck::GetAllCheckConstraint(wyChar * createstring, wyString *allcheckconstraint)
{
	wyChar * findc = "CONSTRAINT", *findch = "CHECK";
	wyBool found = wyFalse;
	wyString tempstr,str = createstring;
	const char *ptr = strstr(createstring, findc);

	if (ptr)
	{
		const char *ptr2 = strstr(createstring, findch);
		int index = ptr2 - createstring;
		wyString *p = &tempstr;
		tempstr.SetAs(str.Substr(index, str.GetLength()));
		
		//allcheckconstraint->AddSprintf("%S", tempstr);
		
		found = wyTrue;
	}

	return found;

}

void TabCheck::FillInitValues()
{

	CheckConstraintStructWrapper   *cwrapobj = NULL;
	wyUInt32                row = -1;

	cwrapobj = (CheckConstraintStructWrapper *)m_listwrapperstruct.GetFirst();

	while (cwrapobj)
	{
		row = InsertRow();

		//..Setting text & Long Values
		CustomGrid_SetText(m_hgridtblcheckconst, row, CONSTRAINTNAME, cwrapobj->m_oldval->m_name.GetString());
		CustomGrid_SetText(m_hgridtblcheckconst, row, CONSTRAINTEXPRESSION, cwrapobj->m_oldval->m_checkexpression.GetString());

		CustomGrid_SetRowLongData(m_hgridtblcheckconst, row, (LPARAM)cwrapobj);

		cwrapobj = (CheckConstraintStructWrapper *)cwrapobj->m_next;
	}

	CustomGrid_SetCurSelection(m_hgridtblcheckconst, 0, 0, wyTrue);
}
wyInt32
TabCheck::InsertRow()
{
	wyInt32 row;

	//..Adding a new row in the grid
	row = CustomGrid_InsertRow(m_hgridtblcheckconst);
	CustomGrid_SetButtonVis(m_hgridtblcheckconst, row, 1, wyTrue);
	CustomGrid_SetButtonText(m_hgridtblcheckconst, row, CONSTRAINTNAME, L"...");

	return row;
}

wyBool
TabCheck::GetNewAndModifiedChecks(wyString &query, wyBool  execute)
{
	wyInt32             count = 0;
	wyBool              validflg = wyTrue;
	wyString            tempstr(""), celldata, namestr, temp, exprstr, columnsstr, indexcomment = "";
	wyString            dropck(""), addck("");
	CheckConstraintStructWrapper *pwrapobj = NULL;
	wyChar      *tbuff = NULL;

	count = CustomGrid_GetRowCount(m_hgridtblcheckconst);
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

	for (int row = 0; row < count; row++)
	{
		pwrapobj = (CheckConstraintStructWrapper *)CustomGrid_GetRowLongData(m_hgridtblcheckconst, row);

		if (!pwrapobj)       //..empty rows
			continue;

		if (pwrapobj->m_newval == pwrapobj->m_oldval)        //..if, the check constraint is not changed or it's not a new check constraint
			continue;

		if (!pwrapobj->m_newval)                             //..when user had deleted only constraint name and not entire constraint, that time constraint row will be there but m_newval will be NULL;
			continue;

		if (pwrapobj->m_oldval)
		{
			namestr.SetAs(pwrapobj->m_oldval->m_name);
		
			//from  .ini file. Refresh always
			m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

			//If quotes are present in show create table 
			//if (AppendBackQuotes()) {
				CheckForQuotesAndReplace(&namestr);
			//}

			dropck.AddSprintf("\r\n  drop constraint %s%s%s,", m_backtick,namestr.GetString(), m_backtick);
		}

		GetGridCellData(m_hgridtblcheckconst, row, CONSTRAINTNAME, namestr);
		GetGridCellData(m_hgridtblcheckconst, row, CONSTRAINTEXPRESSION, exprstr);

		exprstr.Sprintf("%s", exprstr.GetString());
		tbuff = GetEscapedValue(m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), namestr.GetString());
		namestr.Sprintf("%s", tbuff);

		namestr.FindAndReplace("`", "``");

		if (namestr.GetLength() > 64)
		{
			MessageBox(m_hwnd, _(L"Please enter a constraint name less than 64 characters"),
				_(L"Warning"), MB_ICONWARNING | MB_OK | MB_DEFBUTTON2);
			return wyFalse;
		}
		if (exprstr.GetLength() == 0)
		{
			tempstr.SetAs("");
			tempstr.SetAs(NO_EXPRESSION_SPECIFIED_FOR_CHECK);

			exprstr.AddSprintf("\t\t/* %s */", _(tempstr.GetString()));
		}

		//from  .ini file. Refresh always
		m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

		//If quotes are present in show create table 
		//if (AppendBackQuotes()) {
			CheckForQuotesAndReplace(&namestr);
		//}

		if (!(pwrapobj->m_oldval)&& (pwrapobj->m_newval)) //If a new constraint is being added
		{
			if (namestr.GetLength() <= 0 && exprstr.GetLength())
			{
				addck.AddSprintf("\r\n add constraint CHECK (%s)", exprstr.GetString());
			}
			else if (namestr.GetLength() && exprstr.GetLength())
			{
				addck.AddSprintf("\r\n  add constraint %s%s%s CHECK (%s)", m_backtick, namestr.GetString(), m_backtick, exprstr.GetString());
			}

			if (row<count-1)
			addck.AddSprintf(",");
		}
		else if (namestr.CompareI(pwrapobj->m_oldval->m_name) != 0)
		{
			if (namestr.GetLength() <= 0 && exprstr.GetLength())
			{
				addck.AddSprintf("\r\n add constraint CHECK (%s)", exprstr.GetString());
			}
			else if (namestr.GetLength() && exprstr.GetLength())
			{
				addck.AddSprintf("\r\n  add constraint %s%s%s CHECK %s", m_backtick, namestr.GetString(), m_backtick, exprstr.GetString());
			}
			if (row<count-1)
			addck.AddSprintf(",");
		}
		else if (exprstr.CompareI(pwrapobj->m_oldval->m_checkexpression) != 0)
		{
			if (namestr.GetLength() <= 0 && exprstr.GetLength())
			{
				addck.AddSprintf("\r\n add constraint CHECK (%s)", exprstr.GetString());
			}
			else if (namestr.GetLength() && exprstr.GetLength())
			{
				addck.AddSprintf("\r\n  add constraint %s%s%s CHECK %s", m_backtick, namestr.GetString(), m_backtick, exprstr.GetString());
			}
			if (row<count - 1)
				addck.AddSprintf(",");
		}
		if (pwrapobj->m_errmsg)
		{
			tempstr.AddSprintf("\t\t/* %s */", pwrapobj->m_errmsg->GetString());
		}
	}
	if (dropck.GetLength())
		query.AddSprintf("%s", dropck.GetString());

	if (addck.GetLength())
		query.AddSprintf("%s", addck.GetString());

	query.RTrim();

	return validflg;
}

void
TabCheck::CheckForQuotesAndReplace(wyString *name)
{
	wyBool flag = wyFalse;
	name->LTrim();
	name->RTrim();
	const char first = name->GetCharAt(0);
	const char last = name->GetCharAt(name->GetLength()-1);

	if (first == '`' && last == '`')
	{
		name->Strip(1);
		name->Erase(0, 1);
	}
	return ;
}

void
TabCheck::OnTabSelChanging()
{
	ApplyCancelGridChanges();
	ValidateChecks();
}

void
TabCheck::OnTabSelChange()
{
	HWND   hwndarr[10] = { m_hgridtblcheckconst,m_ptabmgmt->m_hwndtool, NULL};

	EnumChildWindows(m_hwnd, TableTabInterfaceTabMgmt::ShowWindowsProc, (LPARAM)hwndarr);

	SetFocus(m_hgridtblcheckconst);

	//int b = CustomGrid_GetRowCount(m_hgridtblcheckconst);

	SendMessage(m_ptabmgmt->m_hwndtool, TB_HIDEBUTTON, (WPARAM)IDM_SEPARATOR, (LPARAM)MAKELONG(TRUE, 0));
	SendMessage(m_ptabmgmt->m_hwndtool, TB_HIDEBUTTON, (WPARAM)IDI_MOVEUP, (LPARAM)MAKELONG(TRUE, 0));
	SendMessage(m_ptabmgmt->m_hwndtool, TB_HIDEBUTTON, (WPARAM)IDI_MOVEDOWN, (LPARAM)MAKELONG(TRUE, 0));
}

void
TabCheck::ApplyCancelGridChanges()
{
	wyInt32 row, col;

	row = CustomGrid_GetCurSelRow(m_hgridtblcheckconst);
	col = CustomGrid_GetCurSelCol(m_hgridtblcheckconst);

	//if (col == INDEXTYPE && m_automatedindexrow != row)
		//CustomGrid_CancelChanges(m_hgridtblcheckconst, wyTrue);
	//else
	CustomGrid_ApplyChanges(m_hgridtblcheckconst, wyTrue);
}

void
TabCheck::Resize()
{
	RECT			rcmain, rctoolbar;
	wyInt32			hpos, vpos, height;

	VERIFY(GetWindowRect(m_hwnd, &rcmain));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcmain, 2));

	VERIFY(GetWindowRect(m_ptabmgmt->m_hwndtool, &rctoolbar));
	VERIFY(MapWindowPoints(NULL, m_ptabmgmt->m_hwndtool, (LPPOINT)&rctoolbar, 2));

	//..Moving Grid
	hpos = m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog ? 2 : 1;
	vpos = rctoolbar.bottom; // 25;
	height = rcmain.bottom - rcmain.top - rctoolbar.bottom - rctoolbar.top - 2;    //..25 for "Hide Language Options" checkbox So that grid in each tab is consistent with TabField's grid

	if (m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog)
		MoveWindow(m_hgridtblcheckconst, hpos, vpos, rcmain.right - 3, height, TRUE);
	else
		MoveWindow(m_hgridtblcheckconst, hpos, vpos, rcmain.right - 2, height, TRUE);
}

void
TabCheck::ReInitializeGrid()
{
	/// Function re-initializes all index-values in the grid after 'Save'
	wyString    tblname, dbname;
	wyInt32     row = -1;
	//m_automatedindexrow = -1;

	//from  .ini file. Refresh always
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

	ClearAllMemory();
	CustomGrid_DeleteAllRow(m_hgridtblcheckconst, wyTrue);

	FetchCheckConstraintsIntoWrapper();
	FillInitValues();

	row = InsertRow();
	if (row != -1)
	CustomGrid_SetCurSelection(m_hgridtblcheckconst, row, CONSTRAINTNAME);

}


void
TabCheck::HandleChecksOnFieldRename(CheckConstraintStructWrapper* checkwrap )
{
	/// Changes the index-columns string, if the column from grid in the Columns-tab in renamed
	wyString    expr(""), name("");
	wyString    tmpstr;
	wyUInt32    nrows = 0, row = -1;
	IndexColumn *iindcols = NULL;

	if (!checkwrap)
		return;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

	nrows = CustomGrid_GetRowCount(m_hgridtblcheckconst);

	for (row = 0; row<nrows; row++)
	{
		if (checkwrap == ((CheckConstraintStructWrapper*)CustomGrid_GetRowLongData(m_hgridtblcheckconst, row)))
			break;
	}

	if (row == nrows)
		return;

	if (checkwrap->m_newval == checkwrap->m_oldval)
		checkwrap->m_newval = GetDuplicateCheckStruct(checkwrap->m_oldval);

	checkwrap->m_newval->m_name.SetAs(name);
	ScanEntireRow(row, CONSTRAINTNAME, name);

	checkwrap->m_newval->m_checkexpression.SetAs(expr);
	ScanEntireRow(row, CONSTRAINTEXPRESSION, expr);

	//CustomGrid_SetText(m_hgridtblcheckconst, row, CONSTRAINTNAME, checkwrap->m_newval->m_name);
}



wyBool
TabCheck::ValidateChecks(wyBool showmsg)
{
	wyUInt32    nrows = 0;
	wyString    columnsstr;

	nrows = CustomGrid_GetRowCount(m_hgridtblcheckconst);
	//IndexesStructWrapper *indwrap = NULL;

	/*for (int row = 0; row<nrows; row++)
	{
		indwrap = (IndexesStructWrapper *)CustomGrid_GetRowLongData(m_hgridindexes, row);

		if (!indwrap)
			continue;

		delete indwrap->m_errmsg;
		indwrap->m_errmsg = NULL;

		if (row == m_automatedindexrow)
			continue;

		if (!indwrap->m_newval)
			continue;

		GetGridCellData(m_hgridindexes, row, INDEXCOLUMNS, columnsstr);

		if (!columnsstr.GetLength())
		{
			if (!indwrap->m_errmsg)
				indwrap->m_errmsg = new wyString;
			indwrap->m_errmsg->SetAs(NO_COLUMNS_DEFINED_FOR_INDEX);

			if (showmsg)
			{
				if (m_ptabmgmt->GetActiveTabImage() != IDI_TABIMG_INDEXES)
					CustomTab_SetCurSel(m_ptabmgmt->m_hwnd, 1);

				CustomGrid_SetCurSelection(m_hgridindexes, row, INDEXCOLUMNS, wyTrue);
				CustomGrid_EnsureVisible(m_hgridindexes, row, INDEXCOLUMNS, wyTrue);

				MessageBox(m_hwnd, NO_COLUMNS_DEFINED_FOR_INDEX, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
				return wyFalse;
			}
		}
	}*/
	return wyTrue;
}

wyBool
TabCheck::ProcessInsert()
{
	wyInt32 newrowid = -1;
	ApplyCancelGridChanges();

	newrowid = InsertRow();

	if (newrowid != -1)
		CustomGrid_SetCurSelection(m_hgridtblcheckconst, newrowid, CONSTRAINTNAME);

	return wyTrue;
}

wyBool
TabCheck::GenerateQuery(wyString& query)
{
	wyString    str("");
	wyBool      retval = wyTrue;

	//from  .ini file
	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

	if (m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
		retval = GenerateAlterQuery(str);
	else
		retval = GenerateCreateQuery(str);

	if (str.GetLength())
	{
		if (m_ptabmgmt->m_tabinterfaceptr->m_isaltertable)
			query.AddSprintf(" %s\r\n", str.GetString());
		else
			query.AddSprintf("%s\r\n", str.GetString());
		return wyTrue;
	}

	return retval;
}

wyBool
TabCheck::GenerateAlterQuery(wyString &query)
{
	wyBool      validflg = wyTrue, flag = wyTrue;
	wyString    localquerystr("");

	//..Combining queries for dropped, modified and new indexes
	validflg = GetDroppedChecks(localquerystr);
	validflg = (flag = GetNewAndModifiedChecks(localquerystr)) ? validflg : flag;

	localquerystr.RTrim();

	if (localquerystr.GetLength())
		query.AddSprintf("%s", localquerystr.GetString());

	return validflg;
}


wyBool
TabCheck::GetDroppedChecks(wyString& query)
{
	wyString            keyname, temp;
	wyString            dbname, tblname, localquerystr("");
	CheckConstraintStructWrapper *pwrapobj = NULL;

	m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

	pwrapobj = (CheckConstraintStructWrapper *)m_listwrapperstruct.GetFirst();

	while (pwrapobj)
	{
		if (!pwrapobj->m_newval)
		{
			temp.SetAs(pwrapobj->m_oldval->m_name);
			//if (pwrapobj->m_oldval->m_name.CompareI("PRIMARY") == 0)
				//localquerystr.Add("\r\n  drop primary key,");
			//else
				localquerystr.AddSprintf("\r\n  drop constraint %s%s%s,", m_backtick, temp.GetString(),m_backtick);
		}
		pwrapobj = (CheckConstraintStructWrapper *)pwrapobj->m_next;
	}

	localquerystr.RTrim();
	query.SetAs(localquerystr);

	return wyTrue;
}


wyBool
TabCheck::GenerateCreateQuery(wyString &query)
{
	wyBool      flag = wyTrue;
	wyUInt32    rowcount = 0;
	wyString    constraintnamestr, expressionstr, chkconstraintstr;
	wyString    tempstr;
	rowcount = CustomGrid_GetRowCount(m_hgridtblcheckconst);
	CheckConstraintStructWrapper *chkwrap = NULL;

	for (int row = 0; row<rowcount; row++)
	{
		chkwrap = (CheckConstraintStructWrapper *)CustomGrid_GetRowLongData(m_hgridtblcheckconst, row);

		if (!chkwrap)
			continue;

		GetGridCellData(m_hgridtblcheckconst, row, CONSTRAINTNAME, constraintnamestr);
		GetGridCellData(m_hgridtblcheckconst, row, CONSTRAINTEXPRESSION, expressionstr);

		//if (constraintnamestr.GetLength() > 64)
		//{
		//	MessageBox(m_hwnd, _(L"Please enter a constraint name less than 64 characters"),
		//		_(L"Warning"), MB_ICONWARNING | MB_OK | MB_DEFBUTTON2);
		//	return wyFalse;
		//}

		//from  .ini file
		m_backtick = AppendBackQuotes() == wyTrue ? "`" : "";

		//if ((!constraintnamestr.GetLength()) && (!constraintnamestr.GetLength()))
		//	continue;

		//if ((expressionstr.GetLength() == 0))
		//	continue;
		if (constraintnamestr.GetLength() <= 0 && expressionstr.GetLength()>0)
		{
			chkconstraintstr.AddSprintf("\r\n CHECK (%s)", expressionstr.GetString());
		}
		else
		{
			chkconstraintstr.AddSprintf("\r\n CONSTRAINT %s%s%s CHECK (%s)", m_backtick,constraintnamestr.GetString(), m_backtick, expressionstr.GetString());
		}
		
		
		chkconstraintstr.Add(",");

		//..Appending error message
		if (expressionstr.GetLength() == 0)
		{
			tempstr.SetAs(NO_EXPRESSION_SPECIFIED_FOR_CHECK);
			chkconstraintstr.AddSprintf("\t\t/* %s */", _(tempstr.GetString()));
		}
	}
	if (chkconstraintstr.GetLength())
	{
		chkconstraintstr.Strip(1);
		query.AddSprintf("%s", chkconstraintstr.GetString());
	}
	return flag;
}

wyUInt32
TabCheck::GetGridCellData(HWND hwndgrid, wyUInt32 row, wyUInt32 col, wyString &celldata)
{
	wyString    tempstr("");
	wyWChar     *data;
	wyUInt32    celldatalen = 0;

	celldatalen = 0;
	celldatalen = CustomGrid_GetItemTextLength(hwndgrid, row, col);
	if (celldatalen)
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

wyBool
TabCheck::OnGVNBeginLabelEdit(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	wyUInt32		    row = wParam;
	wyUInt32		    col = lParam;
	wyUInt32            count = -1;
	wyString            celldata, constnamestr, indcolsstr, indexcomment;

	GetGridCellData(m_hgridtblcheckconst, row, CONSTRAINTNAME, constnamestr);
	GetGridCellData(m_hgridtblcheckconst, row, CONSTRAINTEXPRESSION, indcolsstr);


	GetGridCellData(m_hgridtblcheckconst, row, col, m_celldataprevval);

	count = CustomGrid_GetRowCount(m_hgridtblcheckconst);

	if (row == (count - 1))
	{
		//..Adding extra row when the row being edited is the last row of the grid
		InsertRow();
	}

	return wyTrue;
}

void
TabCheck::OnGVNButtonClick()
{

	//row = CustomGrid_GetCurSelRow(m_hgridtblcheckconst);
	//col = CustomGrid_GetCurSelCol(m_hgridtblcheckconst);

	//CustomGrid_ApplyChanges(m_hgridtblcheckconst, wyTrue);

	//if (col == INDEXCOLUMNS)
	//{
	//	CustomGrid_ApplyChanges(m_hgridindexes, wyTrue);
	//	ShowColumnsDialog();
	//}
}


wyBool
TabCheck::OnGVNEndLabelEdit(WPARAM wParam, LPARAM lParam)
{
	wyChar                  *data = (wyChar*)lParam;
	wyUInt32                row, col;
	wyString                currentdata;
	wyString                indexnamestr(""), indexcolsstr("");
	row = LOWORD(wParam);
	col = HIWORD(wParam);


	//..Work-around to the cutsomgrid issue
	if (!(col >= 0 ))
		return wyTrue;

	if (!(row >= 0 && row <= CustomGrid_GetRowCount(m_hgridtblcheckconst)))
		return wyTrue;

	if (data)
		currentdata.SetAs(data);

	currentdata.RTrim();

	//.. return if modified celldata is same as previous celldata
	if (m_celldataprevval.Compare(currentdata) == 0)
	{
		return wyTrue;
	}

	switch (col)
	{
	case CONSTRAINTNAME:
		if (!OnEndEditName(wParam, lParam))
			return wyFalse;
		break;

	case CONSTRAINTEXPRESSION:
		if (!OnEndEditExpression(wParam, lParam))
			return wyFalse;
		break;
	}
		if (!m_ptabmgmt->m_tabinterfaceptr->m_dirtytab)
			m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);

		return wyTrue;
	
}



LRESULT
TabCheck::OnGVNSysKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	//if (m_ptabmgmt->OnSysKeyDown(hwnd, wParam, lParam) == wyTrue)
	//{
	//	return 1;
	//}

	//if (!m_ptabmgmt->m_tabinterfaceptr->m_open_in_dialog)
	//	return 1;

	return m_ptabmgmt->m_tabinterfaceptr->OnWMSysKeyDown(1, wParam, lParam);
}

wyBool
TabCheck::OnEndEditExpression(WPARAM wParam, LPARAM lParam)
{
	wyChar                  *data = (wyChar*)lParam;
	wyUInt32                row, col;
	wyString                currentdata("");
	CheckConstraintStructWrapper   *cwrapobj = NULL;
	CheckConstarintInfo                *icheck = NULL;
	wyString                contraintnamestr("");
	wyString                expressionstr("");

	row = LOWORD(wParam);
	col = HIWORD(wParam);

	GetGridCellData(m_hgridtblcheckconst, row, CONSTRAINTEXPRESSION, expressionstr);

	if (data)
	currentdata.SetAs(data);

	currentdata.RTrim();
	cwrapobj = (CheckConstraintStructWrapper*)CustomGrid_GetRowLongData(m_hgridtblcheckconst, row);

	//..If no wrapper is attached
	if (!cwrapobj)
	{
		icheck = new CheckConstarintInfo();
		icheck->m_name.Clear();
		icheck->m_listcolumns = NULL;
		icheck->m_checkexpression.Clear();
		cwrapobj = new CheckConstraintStructWrapper(icheck, wyTrue);
		m_listwrapperstruct.Insert(cwrapobj);
	}
	else
	{
		//..If the constraint-name is erased
		if (!currentdata.GetLength())
		{
			//..If user has deleted constraint-name and constraint-expression values from existing check constraint, this condition will be false
			//      we need to proceed only when user has not cleared Existing check constraint
			if (cwrapobj->m_newval)
			{
				//..If Check constraint name is not selected, then set m_newval to NULL.
				//..    if the constraint name is new, then remove it from the m_listwrapper.
				if (!contraintnamestr.GetLength())
				{
					//..existing constraint (Alter table)
					if (cwrapobj->m_oldval)
					{
						if (cwrapobj->m_oldval != cwrapobj->m_newval)    //..true, if existing constraint is already modified
							delete cwrapobj->m_newval;
						cwrapobj->m_newval = NULL;
					}
					//..newly added constraint
					else if (cwrapobj->m_newval)
					{
						m_listwrapperstruct.Remove(cwrapobj);
						delete cwrapobj;
						cwrapobj = NULL;
					}
				}
				//.. If constraint name is selected,
				else
				{
					if (cwrapobj->m_newval == cwrapobj->m_oldval)
					{
						cwrapobj->m_newval = GetDuplicateCheckStruct(cwrapobj->m_oldval);
					}
				}
			}
		}
		//.. if something is there as constraint name
		else
		{
			//..If user has not deleted all constraint-values from grid row
			if (cwrapobj->m_newval)
			{
				//..If existing constraint is not yet modified
				if (cwrapobj->m_newval == cwrapobj->m_oldval)
				{
					cwrapobj->m_newval = GetDuplicateCheckStruct(cwrapobj->m_oldval);
				}
			}
		}
	}
	CustomGrid_SetRowLongData(m_hgridtblcheckconst, row, (LPARAM)cwrapobj);
	
	if (cwrapobj && cwrapobj->m_newval)
		cwrapobj->m_newval->m_checkexpression.SetAs(currentdata);

	ScanEntireRow(row, col, currentdata);

	return wyTrue;
}

wyBool
TabCheck::OnEndEditName(WPARAM wParam, LPARAM lParam)
{
	wyChar                  *data = (wyChar*)lParam;
	wyUInt32                row, col;
	wyString                currentdata("");
	CheckConstraintStructWrapper   *cwrapobj = NULL;
	CheckConstarintInfo                *icheck = NULL;
	wyString                contraintnamestr("");
	wyString                expressionstr("");

	row = LOWORD(wParam);
	col = HIWORD(wParam);

	GetGridCellData(m_hgridtblcheckconst, row, CONSTRAINTNAME, contraintnamestr);

	if (data)
		currentdata.SetAs(data);

	currentdata.RTrim();
	cwrapobj = (CheckConstraintStructWrapper*)CustomGrid_GetRowLongData(m_hgridtblcheckconst, row);

	//..If no wrapper is attached
	if (!cwrapobj)
	{
		icheck = new CheckConstarintInfo();
		icheck->m_name.Clear();
		icheck->m_listcolumns = NULL;
		icheck->m_checkexpression.Clear();
		cwrapobj = new CheckConstraintStructWrapper(icheck, wyTrue);
		m_listwrapperstruct.Insert(cwrapobj);
	}
	else
	{
		//..If the constraint-name is erased
		if (!currentdata.GetLength())
		{
			//..If user has deleted constraint-name and constraint-expression values from existing check constraint, this condition will be false
			//      we need to proceed only when user has not cleared Existing check constraint
			if (cwrapobj->m_newval)
			{
				//..If Check constraint name is not selected, then set m_newval to NULL.
				//..    if the constraint name is new, then remove it from the m_listwrapper.
				if (!contraintnamestr.GetLength())
				{
					//..existing constraint (Alter table)
					if (cwrapobj->m_oldval)
					{
						if (cwrapobj->m_oldval != cwrapobj->m_newval)    //..true, if existing constraint is already modified
							delete cwrapobj->m_newval;
						cwrapobj->m_newval = NULL;
					}
					//..newly added constraint
					else if (cwrapobj->m_newval)
					{
						m_listwrapperstruct.Remove(cwrapobj);
						delete cwrapobj;
						cwrapobj = NULL;
					}
				}
				//.. If constraint name is selected,
				else
				{
					if (cwrapobj->m_newval == cwrapobj->m_oldval)
					{
						cwrapobj->m_newval = GetDuplicateCheckStruct(cwrapobj->m_oldval);
					}
				}
			}
		}
		//.. if something is there as constraint name
		else
		{
			//..If user has not deleted all constraint-values from grid row
			if (cwrapobj->m_newval)
			{
				//..If existing constraint is not yet modified
				if (cwrapobj->m_newval == cwrapobj->m_oldval)
				{
					cwrapobj->m_newval = GetDuplicateCheckStruct(cwrapobj->m_oldval);
				}
			}
		}
	}
	CustomGrid_SetRowLongData(m_hgridtblcheckconst, row, (LPARAM)cwrapobj);
	
	if (cwrapobj && cwrapobj->m_newval)
		cwrapobj->m_newval->m_name.SetAs(currentdata);

	ScanEntireRow(row, col, currentdata);

	return wyTrue;
}

wyBool
TabCheck::ScanEntireRow(wyUInt32  currentrow, wyInt32 currentcol, wyString& currentdata)
{
	CheckConstraintStructWrapper   *cwrapobj = NULL;
	CheckColumn             *chkcol1 = NULL, *chkcol2 = NULL;
	List    *list1 = NULL, *list2 = NULL;
	CheckConstarintInfo     *checks = NULL;
	wyUInt32                row, col, ncols;
	wyString                newtext(""), origtext("");
	wyBool                  changed = wyFalse;

	row = currentrow;
	col = currentcol;

	cwrapobj = (CheckConstraintStructWrapper*)CustomGrid_GetRowLongData(m_hgridtblcheckconst, row);

	if (!cwrapobj)
		return wyFalse;

	//..Return if anyone of oldval and newval is not there
	if (!cwrapobj->m_oldval || !cwrapobj->m_newval)
	{
		return wyFalse;
	}

	//..Return if both oldval and newval are equal
	if (cwrapobj->m_oldval == cwrapobj->m_newval)
		return wyFalse;

	checks = cwrapobj->m_oldval;

	ncols = CustomGrid_GetColumnCount(m_hgridtblcheckconst);

	for (int i = 0; i< ncols; i++)
	{
		origtext.Clear();

		if (i == col)
			newtext.SetAs(currentdata);
		else
			GetGridCellData(m_hgridtblcheckconst, row, i, newtext);

		switch (i)
		{
		case CONSTRAINTNAME:
			origtext.SetAs(checks->m_name);
			break;

		case CONSTRAINTEXPRESSION:
			origtext.SetAs(checks->m_checkexpression);
			break;

		default:
			origtext.SetAs("");
		}
		newtext.RTrim();

		if (changed || origtext.Compare(newtext) != 0)
		{
			changed = wyTrue;
			break;
		}
	}

	if (!changed)
	{
		delete cwrapobj->m_newval;
		cwrapobj->m_newval = cwrapobj->m_oldval;
	}
	return wyTrue;
}


CheckConstarintInfo*
TabCheck::GetDuplicateCheckStruct(CheckConstarintInfo* duplicateof)
{
	CheckConstarintInfo* checks = NULL;

	if (duplicateof)
	{
		checks = new CheckConstarintInfo();
		checks->m_listcolumns = new List();

		//..Setting constraint name and expression
		checks->m_name.SetAs(duplicateof->m_name);
		checks->m_checkexpression.SetAs(duplicateof->m_checkexpression);
	}
	return checks;
}

void
TabCheck::CancelChanges(wyBool isaltertable)
{
	wyUInt32    count = -1, row = -1;
	CheckConstraintStructWrapper *cwrapobj = NULL;

	//..The below function will apply or cancel the custgrid changes
	OnTabSelChanging();

	count = CustomGrid_GetRowCount(m_hgridtblcheckconst);

	for (row = 0; row < count; row++)
	{
		cwrapobj = (CheckConstraintStructWrapper *)CustomGrid_GetRowLongData(m_hgridtblcheckconst, row);
		if (cwrapobj && cwrapobj->m_newval && !cwrapobj->m_oldval)                           //..Create/Alter table (If new (valid)field is added)
		{
			m_listwrapperstruct.Remove(cwrapobj);

			delete cwrapobj;
		}
		else if (cwrapobj && cwrapobj->m_newval && cwrapobj->m_oldval)                           //..Alter table (If field is added)
		{
			if (cwrapobj->m_newval != cwrapobj->m_oldval)                                    //..If field modified, delete m_newval, and set it as m_oldval
				delete cwrapobj->m_newval;

			cwrapobj->m_newval = cwrapobj->m_oldval;
		}
	}

	cwrapobj = (CheckConstraintStructWrapper*)m_listwrapperstruct.GetFirst();

	while (cwrapobj)
	{
		if (cwrapobj->m_errmsg)
			delete cwrapobj->m_errmsg;
		cwrapobj->m_errmsg = NULL;

		cwrapobj->m_newval = cwrapobj->m_oldval;
		cwrapobj = (CheckConstraintStructWrapper*)cwrapobj->m_next;
	}

	//m_automatedindexrow = -1;
	//m_lastclickindgrid = -1;

	CustomGrid_DeleteAllRow(m_hgridtblcheckconst, wyTrue);

	//..Create table
	if (!isaltertable)
	{
		row = InsertRow();
		CustomGrid_SetCurSelection(m_hgridtblcheckconst, row, CONSTRAINTNAME);
		InvalidateRect(m_hgridtblcheckconst, NULL, TRUE);
		return;
	}

	//..Alter table
	FillInitValues();

	row = InsertRow();

	//if (m_ptabmgmt->GetActiveTabImage() == IDI_TABIMG_INDEXES)
	//{
	//	SetFocus(m_hgridindexes);
	//}
	CustomGrid_SetCurSelection(m_hgridtblcheckconst, row, CONSTRAINTNAME);
	return;
}

wyBool
TabCheck::ProcessDelete()
{
	wyInt32		selrow;

	selrow = CustomGrid_GetCurSelRow(m_hgridtblcheckconst);

	if (selrow == -1)
		return wyFalse;

	ApplyCancelGridChanges();

	if (!DropSelectedChecks())
	{
		//..Processing Drop of the index for the current row, if user has not checked any row
		DropCheck(selrow);
	}
	//ReInitializeGrid();
	if (CustomGrid_GetRowCount(m_hgridtblcheckconst) == 0)
	{
		selrow = InsertRow();
		CustomGrid_SetCurSelection(m_hgridtblcheckconst, selrow, CONSTRAINTNAME);
	}

	return wyTrue;
}

wyBool
TabCheck::DropSelectedChecks()
{
		wyInt32     count, row, ret = IDNO;
	wyBool      checkrowfound = wyFalse;
		CheckConstraintStructWrapper *pwrapobj = NULL;
		wyString constraint;
	
		count = CustomGrid_GetRowCount(m_hgridtblcheckconst);
	
		for (row = 0; row<count; row++)
		{
			if (CustomGrid_GetRowCheckState(m_hgridtblcheckconst, row))
			{
				pwrapobj = (CheckConstraintStructWrapper *)CustomGrid_GetRowLongData(m_hgridtblcheckconst, row);
				checkrowfound = wyTrue;
				if (pwrapobj!=NULL && pwrapobj->m_oldval)
				{
					constraint.SetAs(pwrapobj->m_oldval->m_name);
				}
				//else //if (pwrapobj != NULL && pwrapobj->m_newval && pwrapobj->m_oldval)
				//{
				//	constraint.SetAs(pwrapobj->m_newval->m_name);
				//}
				
	
				//..Deleting empty row without confirmation
				if (!pwrapobj || !pwrapobj->m_newval)
				{
					CustomGrid_DeleteRow(m_hgridtblcheckconst, row, wyTrue);
					row--;
					count--;
					/*if ((m_automatedindexrow != -1) && row <= m_automatedindexrow)
						m_automatedindexrow--;*/
					continue;
				}
				else if (ret == IDNO)
				{
					ret = MessageBox(m_hgridtblcheckconst, _(L"Do you want to drop the selected check constraint(s)?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
					if (ret != IDYES)
						return wyTrue;
				}

				ChangeListOnDelete(pwrapobj);
					
	
					CustomGrid_DeleteRow(m_hgridtblcheckconst, row, wyTrue);
	
					////..reduce m_automatedindexrow if row is less than the m_automatedindexrow
					//if (row <= m_automatedindexrow)
					//	m_automatedindexrow--;
					row--;
					count--;
				}
			}
		
	
		if (ret == IDYES)
		{
			if (m_ptabmgmt->m_tabinterfaceptr->m_dirtytab != wyTrue)
			{
				m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);
			}
		}
	
		//..resetting m_lastclickindgrid
		if (checkrowfound && m_lastclickindgrid != -1)
			m_lastclickindgrid = -1;
	
	return checkrowfound;

}


void
TabCheck::ChangeListOnDelete(CheckConstraintStructWrapper * cwrapobj)
{
	/// For existing index, set m_newval to NULL
	///     for new added index, remove wrapper from the m_listwrapperstruct and delete it

	if (!cwrapobj)
		return;

	if (!cwrapobj->m_newval)
		return;

	if (cwrapobj->m_oldval)
	{
		if (cwrapobj->m_oldval != cwrapobj->m_newval)
		{
			cwrapobj->m_newval->m_name.SetAs("");
			cwrapobj->m_newval->m_checkexpression.SetAs("");
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
TabCheck::DropCheck(wyUInt32 row)
{
	/// Dropping the index from the currently selected/focused row in the grid
	wyInt32                 ret = IDNO;
	CheckConstraintStructWrapper   *pwrapobj = NULL;
	wyString                name(""), exprsstr("");

	pwrapobj = (CheckConstraintStructWrapper*)CustomGrid_GetRowLongData(m_hgridtblcheckconst, row);

	if (pwrapobj && pwrapobj->m_newval)
	{
		GetGridCellData(m_hgridtblcheckconst, row, CONSTRAINTNAME, name);
		GetGridCellData(m_hgridtblcheckconst, row, CONSTRAINTEXPRESSION, exprsstr);

		if (name.GetLength() || exprsstr.GetLength() || pwrapobj->m_oldval)
		{
			//  Ask for confirmation, if the wrapper is associated with the row and the index-values are not deleted from the row
			ret = MessageBox(m_hgridtblcheckconst, _(L"Do you want to drop this check constraint?"), pGlobals->m_appname.GetAsWideChar(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
			if (ret != IDYES)
				return wyFalse;
		}
		
		ChangeListOnDelete(pwrapobj);
	}
	CustomGrid_DeleteRow(m_hgridtblcheckconst, row, wyTrue);
	m_lastclickindgrid = -1;

	if (CustomGrid_GetRowCount(m_hgridtblcheckconst))
	{
		if (row == 0)    // if the selected row was first then we just select the first row and column
		{
			CustomGrid_SetCurSelection(m_hgridtblcheckconst, 0, CONSTRAINTNAME);
		}
		else
		{
			CustomGrid_SetCurSelection(m_hgridtblcheckconst, --row, CONSTRAINTNAME);
		}
	}

	if (ret == IDYES && m_ptabmgmt->m_tabinterfaceptr->m_dirtytab != wyTrue)
	{
		m_ptabmgmt->m_tabinterfaceptr->MarkAsDirty(wyTrue);
	}

 return wyTrue;
}

void
TabCheck::HandleCheckboxClick(HWND hwnd, LPARAM lparam, WPARAM wparam, wyInt32 &lastclickindex)
{
	wyInt32     ret, startpos, endpos, flag, currow = -1;

	if (CustomGrid_GetRowCount(hwnd) == 0)
		return;

	ret = GetKeyState(VK_SHIFT);// shift key is pressed or not

	if (wparam == VK_SPACE)// && GetKeyState(VK_CONTROL)
	{
		if (lastclickindex == -1 || !(ret & 0x8000))
		{
			lastclickindex = CustomGrid_GetCurSelRow(hwnd);
			if (lastclickindex == -1 || (CustomGrid_GetRowCount(hwnd) == 0))
				return;

			flag = CustomGrid_GetRowCheckState(hwnd, lastclickindex);
			CustomGrid_SetRowCheckState(hwnd, lastclickindex, flag == GV_CHEKCED ? wyFalse : wyTrue);
			return;
		}
		currow = CustomGrid_GetCurSelRow(hwnd);
	}
	else
	{
		if (lastclickindex == -1 || !(ret & 0x8000))   //..If first time, any checkbox is clicked or shift key is not pressed, then no multiple selection required..
		{
			lastclickindex = wparam;
			return;
		}

		if (lastclickindex == wparam)
			return;
		currow = wparam;
	}

	if (currow < lastclickindex)
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

	while (startpos <= endpos)
	{
		if (wparam == VK_SPACE)
			CustomGrid_SetRowCheckState(hwnd, startpos, flag == GV_CHEKCED ? wyFalse : wyTrue);
		else
			CustomGrid_SetRowCheckState(hwnd, startpos, flag == GV_CHEKCED ? wyTrue : wyFalse);
		startpos++;
	}
	lastclickindex = currow;
}

wyBool
TabCheck::ExecuteQuery(wyString &query)
{
	MYSQL_RES		*res;
	wyInt32 isintransaction = 1;

	res = ExecuteAndGetResult(m_mdiwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction, GetActiveWindow());

	if (isintransaction == 1)
		return wyFalse;

	if (!res && m_mdiwnd->m_tunnel->mysql_affected_rows(m_mdiwnd->m_mysql) == -1)
	{
		ShowMySQLError(m_hwnd, m_mdiwnd->m_tunnel, &(m_mdiwnd->m_mysql), query.GetString());
		return wyFalse;
	}
	m_mdiwnd->m_tunnel->mysql_free_result(res);

	return wyTrue;
}
