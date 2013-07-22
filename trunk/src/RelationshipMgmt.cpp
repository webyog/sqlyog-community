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


#include "RelationShipMgmt.h"
#include "IndexManager.h"
#include "CommonHelper.h"
#include "FrameWindowHelper.h"
#include "MySQLVersionHelper.h"
#include "OtherDialogs.h"
#include "SQLMaker.h"
#include "GUIHelper.h"

extern PGLOBALS pGlobals;

#define RELATIONERROR L"Could not create relation. The possible causes of error are -\n\n"\
L"1) The reference table does not have InnoDB/SolidDB/PBXT table handler\n"\
L"2) No index was found on selected columns. You need to explicitly create index on the selected columns\n"\
L"3) The selected columns will create incorrect foreign definition\n"\
L"4) You did not select correct columns\n"\
L"5) Selected columns were not found in the Reference Table\n"\
L"6) Constraint name already exists\n"\
L"\nPlease correct the errors."

#define EDITRELERROR L"There in no direct way to ALTER a Foreign Key in MySQL.\n"\
L"SQLyog will drop the existing one and create a new one using the existing name for the key.\n"\
L"If necessary preconditions for a Foreign Key (matching data types, appropriate indexes etc.) are not met, the create process will fail."\
L"The result will be that existing FK is dropped and no new one is created.\n"\
L"You can avoid this by specify a new name for the key.\n"\
L"Do you want to continue?"


#define	CONSTRAINT			"CONSTRAINT"

#define INITROWCOUNT            100

#define	DLGDEFWIDTH				350

#define RELDLGHTWD				400
#define HANDLERELHT				410
#define HANDLERELWD				438

/**!
	Module for manage references dialog box.
	With this dialog box you can create and delete references.
*/

///Constructor
RelationshipMgmt::RelationshipMgmt()
{
	m_hwndgrid		= NULL;
	m_mystatusres	= NULL;
	m_hwnd			= NULL;
	m_isfksupport   = wyTrue;
}

///Destructor
RelationshipMgmt::~RelationshipMgmt()
{
	if(m_mystatusres)
		m_tunnel->mysql_free_result(m_mystatusres);
}

///The dialog gets initialized from here.
wyInt32
RelationshipMgmt::Create(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, wyChar *db, wyChar *table, const wyChar *constraint)
{
	wyInt32		ret;
	
	m_hwndparent= hwnd;
	m_mysql     = mysql;
	m_tunnel    = tunnel;

	m_db.SetAs(db);
	m_table.SetAs(table);

	//sets the variable name(m_deffkey) if constraint is passed
	if(constraint)
		m_deffkey.SetAs(constraint);
	
	//Post 8.01
	//RepaintTabModule();

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_MANFOREIGN), m_hwndparent, (DLGPROC)RelationshipMgmt::DlgProc, (LPARAM)this);

	if(m_isrelcreated == wyTrue || m_isreldeleted == wyTrue || m_isreledited == wyTrue)
		return 1;

	else
		return 0;	
}

wyInt32 CALLBACK
RelationshipMgmt::DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RelationshipMgmt	*pcrelman = (RelationshipMgmt*)GetWindowLong(hwnd, GWL_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLong(hwnd, GWL_USERDATA, (wyInt32)lParam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, UM_CREATEGRID, 0, 0);
		break;

	case UM_CREATEGRID:
		if(pcrelman->OnCreateGrid(hwnd) == wyFalse)
			break;

		pcrelman->MoveDialog(hwnd);
		break;

	case UM_MOVEWINDOWS:
		PostMessage(hwnd, WM_INITGRIDVALUES, 0, 0);
		break;

	case WM_PAINT:
		//draws the resize gripper at bottom-right corner of dialog
		DrawSizeGripOnPaint(hwnd);
	break;

	case WM_HELP:
		ShowHelp("FK%20In%20SQLyog.htm");
		return wyTrue;

	case WM_INITGRIDVALUES:
		pcrelman->InitGridData();
		CustomGrid_SetCurSelection(pcrelman->m_hwndgrid, 0, 0);
		SetFocus(pcrelman->m_hwndgrid);
		return wyTrue;

	case WM_COMMAND:
		pcrelman->OnWmCommand(hwnd, wParam);
		break;

	case WM_SIZE:
		if(wParam == SIZE_RESTORED)
			pcrelman->Resize(lParam);
		return 0;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO pMMI = (LPMINMAXINFO)lParam;
			pMMI->ptMinTrackSize.x = RELDLGHTWD;
			pMMI->ptMinTrackSize.y = RELDLGHTWD;
		}
		break;
	
	case WM_DESTROY:
		if(pcrelman->m_isfksupport == wyTrue)
			SaveInitPos(hwnd, MANRELATION_SECTION);
		//	StoreDialogPersist(hwnd, MANRELATION_SECTION);
		break;
	}
	
	return 0;
}

void 
RelationshipMgmt::OnWmCommand(HWND hwnd, WPARAM wparam)
{
    switch(LOWORD(wparam))
	{
	case IDCANCEL:
		VERIFY(yog_enddialog(hwnd, 0));
		break;

	case IDC_NEW:
		OnIDCNew(hwnd);
		break;

	case IDC_EDIT:
		OnIDCEdit(hwnd);
		break;

	case IDC_DELETE:
		OnDelete(hwnd);
		break;
	}
    return;
}

void
RelationshipMgmt::MoveDialog(HWND hwnd)
{
	HICON hicon;

	//Set icon for dialog	
	hicon = CreateIcon(IDI_MANREL_16);
	SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hicon);

	//SetDialogPos(hwnd, MANRELATION_SECTION);
	SetInitPos(hwnd, MANRELATION_SECTION);
	return;
}

void		
RelationshipMgmt::Resize(LPARAM lParam)
{
	HWND hwndnew, hwndedit, hwnddelete, hwndclose, hwndtopline1, hwndtopline2, hwndbtmline, hwndtitle;
	RECT rctdlg, rcttmp, rctbut, rcttitle;

	rctdlg.left = 0;
	rctdlg.top = 0;
	rctdlg.right = LOWORD(lParam);
	rctdlg.bottom = HIWORD(lParam);

	VERIFY(hwndnew = GetDlgItem(m_hwnd, IDC_NEW));
	VERIFY(hwndedit = GetDlgItem(m_hwnd, IDC_EDIT));
	VERIFY(hwnddelete = GetDlgItem(m_hwnd, IDC_DELETE));
	VERIFY(hwndclose = GetDlgItem(m_hwnd, IDCANCEL));
	VERIFY(hwndtopline1 = GetDlgItem(m_hwnd, IDC_TOPLINE));
	VERIFY(hwndtopline2 = GetDlgItem(m_hwnd, IDC_BOTTOMLINE));
	VERIFY(hwndbtmline = GetDlgItem(m_hwnd, IDC_BOTTOMLINE2));
	VERIFY(hwndtitle = GetDlgItem(m_hwnd, IDC_EXISTITLE));

	//Top line
	GetWindowRect(hwndtopline1, &rcttmp);
	MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcttmp, 2);
	VERIFY(MoveWindow(hwndtopline1, rctdlg.left+10, rcttmp.top, 
		(rctdlg.right - 10) - (rctdlg.left + 10), rcttmp.bottom - rcttmp.top, FALSE));

	//2nd line
	GetWindowRect(hwndtopline2, &rcttmp);
	MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcttmp, 2);
	VERIFY(MoveWindow(hwndtopline2, rctdlg.left+10, rcttmp.top, 
		(rctdlg.right - 10) - (rctdlg.left + 10), rcttmp.bottom - rcttmp.top, FALSE));

	//New
	VERIFY(GetClientRect(hwndnew, &rctbut));
	VERIFY(MoveWindow(hwndnew, rctdlg.left+10, rctdlg.bottom-35, rctbut.right, rctbut.bottom, FALSE ));
	VERIFY(GetWindowRect(hwndnew, &rcttmp));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcttmp, 2));

	//Edit
	VERIFY(MoveWindow(hwndedit, rcttmp.right + WIDTHBETBUTTON, rcttmp.top, 
		rcttmp.right - rcttmp.left, rcttmp.bottom - rcttmp.top, FALSE ));
	
	VERIFY(GetWindowRect(hwndedit, &rcttmp));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcttmp, 2));

	//Delete
	VERIFY(MoveWindow(hwnddelete, rcttmp.right + WIDTHBETBUTTON, rcttmp.top, 
		rcttmp.right - rcttmp.left, rcttmp.bottom - rcttmp.top, FALSE ));
	
	VERIFY(GetWindowRect(hwnddelete, &rcttmp));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcttmp, 2));

	//Close
	VERIFY(MoveWindow(hwndclose, (rctdlg.right - WIDTHBETBUTTON) - (rcttmp.right - rcttmp.left), rcttmp.top, 
		rcttmp.right - rcttmp.left, rcttmp.bottom - rcttmp.top, FALSE ));

	VERIFY(GetWindowRect(hwnddelete, &rctbut));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctbut, 2));
	
	//Bottom line
	GetWindowRect(hwndbtmline, &rcttmp);
	MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcttmp, 2);
	VERIFY(MoveWindow(hwndbtmline, rctdlg.left+10, rctbut.top - 10, 
		(rctdlg.right - 10) - (rctdlg.left + 10), rcttmp.bottom - rcttmp.top, FALSE));
	GetWindowRect(hwndbtmline, &rcttmp);
	MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcttmp, 2);

	//Title rect
	GetWindowRect(hwndtitle, &rcttitle);
	MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcttitle, 2);
	
	//Grid
	VERIFY(MoveWindow(m_hwndgrid, rctdlg.left+10, rcttitle.bottom + 10, 
		(rctdlg.right - 10) - (rctdlg.left + 10), (rcttmp.top - 20) - (rcttitle.bottom), FALSE));
		
	//Handle flickering
	HandleIndexDlgsFlicker(m_hwnd, m_hwndgrid);
		
	return;	
}

wyBool
RelationshipMgmt::OnCreateGrid(HWND hwnd)
{
	wyBool      ret;
	m_hwnd      = hwnd;

	//Check whether the table engine supports relationship 
	ret = IsTableInnoDB(hwnd);
	if(ret == wyFalse)
	{
		m_isfksupport = wyFalse;
		yog_enddialog(hwnd, 0);
		return wyFalse;
	}

	///we create the grid but first set the text of the database and m_table.
	VERIFY(SendMessage(GetDlgItem(hwnd, IDC_DATABASE), WM_SETTEXT, 0, (LPARAM)m_db.GetAsWideChar()));
	VERIFY(SendMessage(GetDlgItem(hwnd, IDC_TABLES), WM_SETTEXT, 0, (LPARAM)m_table.GetAsWideChar()));
	
	VERIFY(m_hwndgrid = CreateCustomGrid(m_hwnd, 0, 0, 0, 0, (GVWNDPROC)GVWndProc, (LPARAM)this));
	VERIFY(SetWindowPos(m_hwndgrid, GetDlgItem(hwnd, IDCANCEL), 0, 0, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOMOVE));
	PostMessage(hwnd, UM_MOVEWINDOWS, 0, 0);

	return wyTrue;
}

void
RelationshipMgmt::OnIDCNew(HWND hwnd)
{
    CCreateRel pcei;
	wyInt32	   ret;
	wyChar	  *newindex = NULL;
	
	UpdateWindow(m_hwndgrid);
	
	ret = pcei.Create(hwnd, m_tunnel, m_mysql, (wyChar*)m_db.GetString(), (wyChar*)m_table.GetString(), newindex);
	
	if(ret)
	{
		CustomGrid_DeleteAllRow(m_hwndgrid);
		
		/**
		now depending upon the MySQL version we have to refill it
		accordingly 
		*/
		if(IsMySQL4013(m_tunnel, m_mysql))
		{
			FillGridWithDataNew();
		} 
		else 
		{
			if(m_mystatusres)
				m_tunnel->mysql_free_result(m_mystatusres);

			IsTableInnoDB(hwnd);
			FillGridWithData();
		}

		ReseekIndex((wyWChar*)pcei.m_neweditindexname.GetAsWideChar());

		m_isrelcreated = wyTrue;
	}

	//post 8.01
	//InvalidateRect(m_hwndgrid, NULL, FALSE);
	//CustomGrid_SetCurSelection(m_hwndgrid, 0, 0);
	SetFocus(m_hwndgrid);
	UpdateWindow(m_hwndgrid);
}

void	
RelationshipMgmt::OnIDCEdit(HWND hwnd)
{
	wyInt32			textlen, ret;
	wyWChar			*reltext = NULL;
	wyChar			*newindex = NULL;
	wyBool			retbool;
	wyString		constraintname, fkey;
	long			selrow;
	RelFieldsParam	*relfldparam = NULL;
	FKEYINFOPARAM	*fkeyparam;	
	CCreateRel		pcei;	
	List			*fklist = NULL, *pklist = NULL;
	
	VERIFY(fklist = new List());
	VERIFY(pklist = new List());

	selrow = CustomGrid_GetCurSelRow(m_hwndgrid);

	textlen = CustomGrid_GetItemTextLength(m_hwndgrid, selrow, 0);
	reltext = AllocateBuffWChar(textlen + 1);   

	if(!reltext)
		return;

	fkeyparam = new FKEYINFOPARAM;	
	relfldparam = new RelFieldsParam;

	if(!fkeyparam || !relfldparam)
		return;

	fkeyparam->m_fkeyfldlist = fklist;
	fkeyparam->m_pkeyfldlist = pklist;

	CustomGrid_GetItemText(m_hwndgrid, selrow, 0, reltext); 
	fkey.SetAs(reltext);

	//Gets parsed F-key infos into a struct
	retbool = GetForeignKeyDetails(&fkey, fkeyparam);
	if(retbool == wyFalse)
		return;

	UpdateWindow(m_hwndgrid);
    
	relfldparam->fkeyinfos = &fkey;
	
	relfldparam->constraintname = &fkeyparam->m_constraint;
	relfldparam->parenttable = &fkeyparam->m_parenttable;

	relfldparam->srcfldinfolist = fkeyparam->m_fkeyfldlist;
	relfldparam->tgtfldinfolist = fkeyparam->m_pkeyfldlist;  

	//ON DELETE option if any
	relfldparam->ondelete = fkeyparam->m_ondelete;

	//ON UPDATE option if any
	relfldparam->onupdate = fkeyparam->m_onupdate;

	//Flag tells Dialog box needed for Edit Relationship
	relfldparam->iseditfkey = wyTrue;
	
	//Sets the "Edit Relationship" dialog box with values parsed frm the Foregnkey details
	ret = pcei.Create(hwnd, m_tunnel, m_mysql, (wyChar*)m_db.GetString(), (wyChar*)m_table.GetString(), newindex, relfldparam);
	
	//Replace the Grid row by new Foreign key if the Edit relationship is successful
	if(ret)
	{
		CustomGrid_SetText(m_hwndgrid, selrow, 0, "");
		CustomGrid_SetText(m_hwndgrid, selrow, 0, relfldparam->fkeyinfos->GetString());

		//Flag sets wyTrue if the F-key edition is successful
		m_isreledited = wyTrue;
	}
	// If edit is failed delete the corresponding row
	else if(!relfldparam->fkeyinfos->GetLength())
	{
		/* delete it from the grid */
		CustomGrid_DeleteRow(m_hwndgrid, selrow);

		/* if nothing is there then we need to disable the keys */
		if(!CustomGrid_GetRowCount(m_hwndgrid))
		{
			EnableWindow(GetDlgItem(m_hwnd, IDC_DELETE), wyFalse);
			EnableWindow(GetDlgItem(m_hwnd, IDC_EDIT), wyFalse);
		}

		m_isreledited = wyTrue;
	}

	if(fkeyparam->m_pkeyfldlist)
		delete fkeyparam->m_pkeyfldlist;

	if(fkeyparam->m_fkeyfldlist)
		delete fkeyparam->m_fkeyfldlist;

	if(fkeyparam)
		delete fkeyparam;

	if(relfldparam)
		delete relfldparam;	
}

void
RelationshipMgmt::OnDelete(HWND hwnd)
{
	wyBool		    ret, isdeleted;
	wyInt32			len, retval;
	wyString		message;

	len = CustomGrid_GetItemTextLength(m_hwndgrid, 0, 0);
	if(!len)
	{
		yog_message(m_hwndgrid, L"No relationship to drop", pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
		return;
	}
	
	message.Sprintf(_("Are you sure you want to delete this foreign key from the database?"));
	retval = MessageBox(m_hwnd, message.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONQUESTION | MB_YESNO);
	switch(retval)
	{
	case IDYES:
		break;

	case IDNO:
		return;
	}
	
	SetCursor(LoadCursor(NULL, IDC_WAIT));

	/* now depending upon the mysql version number we call the 
		required function */
	if(IsMySQL4013(m_tunnel, m_mysql))
	{
		/* use the new method to drop the index i.e. drop with
			the Constraint name...its more safe and reliable */
		isdeleted = DeleteRelationNew();

		m_isreldeleted = isdeleted;
	} 
	else
	{
		//ret = DeleteIndex ();
		if(ret == wyTrue)
		{
			AddRemaining ();
			if(!(CustomGrid_GetRowCount(m_hwndgrid)))
				CustomGrid_InsertRow(m_hwndgrid);
		}
	}
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	SetFocus(m_hwndgrid);
}

wyBool
RelationshipMgmt::IsTableInnoDB(HWND m_hwnd)
{
	wyString        query;

	query.Sprintf("show table status from `%s` like '%s'",
						m_db.GetString(), m_table.GetString());

    SetCursor(LoadCursor(NULL, IDC_WAIT ));
    m_mystatusres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);

    if(!m_mystatusres && m_tunnel->mysql_affected_rows(*m_mysql)== -1)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
		SetCursor(LoadCursor(NULL, IDC_ARROW ));
		return wyFalse;
	}

	VERIFY(m_mystatusrow = m_tunnel->mysql_fetch_row(m_mystatusres));
	
	if(!m_mystatusrow || !m_mystatusrow[1] || (stricmp(m_mystatusrow[1], "InnoDB") != 0 && stricmp(m_mystatusrow[1], "pbxt")!= 0 &&
        stricmp(m_mystatusrow[1], "solidDB") != 0 ))
	{
		yog_message(m_hwnd, _(L"The selected table does not support foreign keys.\nTable engine must be InnoDB, PBXT or SolidDB."), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
		m_tunnel->mysql_free_result(m_mystatusres);
		m_mystatusres = NULL;
		SetCursor(LoadCursor(NULL, IDC_ARROW ));
		yog_enddialog(m_hwnd, 0);
		return wyFalse;
	}

    m_index = m_tunnel->mysql_num_fields(m_mystatusres) - 1;
	SetCursor(LoadCursor(NULL, IDC_ARROW ));

	return wyTrue;
}

// Callback procedure for the grid control.
LRESULT CALLBACK
RelationshipMgmt::GVWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RelationshipMgmt *relmg = (RelationshipMgmt*)CustomGrid_GetLongData(hwnd);
	wyString		tblname("__manage_relation");

	switch(message)
	{
	case GVN_LBUTTONDBLCLK:
		relmg->OnIDCEdit(relmg->m_hwnd);
		return TRUE;

	case GVN_SPLITTERMOVE:
		OnGridSplitterMove(hwnd, &tblname, wParam);
		return TRUE;

		case GVN_BEGINLABELEDIT:
		case GVN_BEGINADDNEWROW:
        case GVN_PASTECLIPBOARDBEGIN:
			return 0;

		default:
			return 1;
	}
}

wyBool
RelationshipMgmt::MoveWindows()
{
	HWND	hwndtitle, hwndbotline;
	RECT	recttitle, rectbotline;
	POINT	pnttitlebottom, pntlinetop, pnttitletop;

	VERIFY(hwndtitle = GetDlgItem(m_hwnd, IDC_EXISTITLE));
	VERIFY(hwndbotline = GetDlgItem(m_hwnd, IDC_BOTTOMLINE2));

	VERIFY(GetWindowRect(hwndtitle, &recttitle));
	VERIFY(GetWindowRect(hwndbotline, &rectbotline));

	// convert the coordinates to the dialog mode
	pnttitlebottom.x = recttitle.right;
	pnttitlebottom.y = recttitle.bottom;

	VERIFY(ScreenToClient(m_hwnd, &pnttitlebottom));

	pntlinetop.x = rectbotline.left;
	pntlinetop.y = rectbotline.top;

	VERIFY(ScreenToClient(m_hwnd, &pntlinetop));

	pnttitletop.x = recttitle.left;
	pnttitletop.y = recttitle.top;

	VERIFY(ScreenToClient(m_hwnd, &pnttitletop));

	VERIFY(MoveWindow(m_hwndgrid, pntlinetop.x, pnttitlebottom.y+3, 
						 (pnttitlebottom.x-pnttitletop.x) + 12, pntlinetop.y - pnttitlebottom.y - 10, wyTrue));
	 
	return wyTrue;
}

wyBool
RelationshipMgmt::InitGridData()
{
	wyUInt32    fcount, width;
	RECT        rect = {0};
	GVCOLUMN	gvcol;
	wyChar      *heading[] = { "References" };
	wyInt32     elemsize[] = { 0, 0, 0, 0 , 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 };
	wyInt32     elemcount[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	wyInt32     mask[] = { GVIF_TEXT, GVIF_TEXT };  
	wyInt32     cx[2];
	wyInt32     fmt[] = { GVIF_LEFT, GVIF_LEFT, GVIF_CENTER, GVIF_CENTER };

	wyInt32		colwidth = 0;
	wyString	colname, dbname(RETAINWIDTH_DBNAME), tblname("__manage_relation");
	wyBool		isretaincolumnwidth = IsRetainColumnWidth();
	
	VERIFY(GetClientRect(m_hwndgrid, &rect));
	width =  rect.right-rect.left;
	width -= 30;
	cx[0] = width-15;

	for(fcount = 0; fcount < sizeof(heading) / sizeof(heading[0]); fcount++)
	{
		if(isretaincolumnwidth == wyTrue)
		{
			//for getting the retained column width
			colname.SetAs(heading[fcount]);
			colwidth = GetColumnWidthFromFile(&dbname, &tblname, &colname);
		}

		memset(&gvcol, 0, sizeof(gvcol));		
		
		gvcol.mask = mask[fcount];
		gvcol.fmt  = fmt[fcount];
		gvcol.cx   = (colwidth > 0)? colwidth : cx[fcount];
		gvcol.text = heading[fcount];
		gvcol.nElemSize = elemsize[fcount];
		gvcol.nListCount = elemcount[fcount];
		gvcol.cchTextMax = strlen(heading[fcount]);

		VERIFY(CustomGrid_InsertColumn(m_hwndgrid, &gvcol)>= 0);
	}

	//The grid fills with the data if default value is present	
	if(m_deffkey.GetLength())	
		FillGridWithDefaultData();
		
	/* now depending upon the version use different method */
	else if(IsMySQL4013(m_tunnel, m_mysql))
		FillGridWithDataNew();
	else
		FillGridWithData ();

	return wyTrue;
}


wyBool
RelationshipMgmt::FillGridWithData()
{
	wyInt32         retrow = -1;
	wyUInt32        len = 0;
	wyChar		    *text, seps[] = ";", *start, *token;
	
	len  = strlen(m_mystatusrow[m_index]);
	
	VERIFY(text =(wyChar*)calloc(sizeof(wyChar), len+1));
    
	strcpy(text, m_mystatusrow[m_index]);

	VERIFY(start = strstr(text, "InnoDB free:"));
	
	token = strtok(start, seps);
	token = strtok(NULL, seps);

	while(token)
	{
		if(strstr(token, " REFER "))
		{
			retrow = CustomGrid_InsertRow(m_hwndgrid);
			CustomGrid_SetText(m_hwndgrid, retrow, 0, token); 
			token = strtok(NULL, seps);
			continue;
		}

		token = strtok(NULL, seps);
	}

	if(retrow == -1)
		CustomGrid_InsertRow(m_hwndgrid);

	free(text);
	return wyTrue;
}

/* if mysql version > 4.0.13 then we keep track of the fk id and use it to
   drop the key rather then use the 11 steps 
   
   Since id info is not kept in show table status info, we have to get it from 
   show create table statement.
   
*/
wyBool
RelationshipMgmt::FillGridWithDataNew ()
{
    wyChar			delm;
	wyInt32         retrow = 0, len;
	wyBool			ret = wyTrue;
	wyString        key, showcreatetable, query;

    //Gets the 'CREATE TABLE' statement
	ret = GetCreateTableString(m_tunnel, m_mysql, m_db.GetString(), 
		m_table.GetString(), showcreatetable, query);

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

        retrow = CustomGrid_InsertRow(m_hwndgrid);
        CustomGrid_SetText(m_hwndgrid, retrow, 0, key.GetString()); 
        key.Clear();

        retrow ++;
        GetForeignKeyInfo(&showcreatetable, key, retrow + 1);
    }
	
	/// if still the row count is zero then disable the delete key & Edit key
	if(CustomGrid_GetRowCount(m_hwndgrid) == 0)
	{
		EnableWindow(GetDlgItem(m_hwnd, IDC_DELETE), FALSE); 
		EnableWindow(GetDlgItem(m_hwnd, IDC_EDIT), FALSE); 
	}

	else
	{
		EnableWindow(GetDlgItem(m_hwnd, IDC_DELETE), TRUE); 
		EnableWindow(GetDlgItem(m_hwnd, IDC_EDIT), TRUE); 
	}

	//Sets the grid column width
	/*if(CustomGrid_GetColumnCount(m_hwndgrid))
	{		
		GetClientRect(m_hwndgrid, &rctdlg);
		CustomGrid_SetColumnWidth(m_hwndgrid, 0, rctdlg.right - 30);
	}*/
	
	return wyTrue;
}

/**
Fills the grid with the Default F-key details passed through 'Create' function
*/
void
RelationshipMgmt::FillGridWithDefaultData()
{
	wyInt32	retrow = 0;

	retrow = CustomGrid_InsertRow(m_hwndgrid);
    
	CustomGrid_SetText(m_hwndgrid, retrow, 0, m_deffkey.GetString()); 
    
	m_deffkey.Clear();
}


wyBool
RelationshipMgmt::DeleteRelationNew ()
{
	if(CustomGrid_GetRowCount(m_hwndgrid) <= 0)
		return wyFalse;

	wyInt32     sel, len;
	wyWChar     *fkname;
    wyString    query;
	wyWChar		*tok;
    MYSQL_RES   *res = NULL;
	wyString	fknamestr;

	sel = CustomGrid_GetCurSelection(m_hwndgrid);

    len = CustomGrid_GetItemTextLength(m_hwndgrid, sel, 0);

    fkname = AllocateBuffWChar(len + 1);

	/* get the complete index name */
	CustomGrid_GetItemText(m_hwndgrid, sel, 0, fkname);

	/* get the id set null at that pos and use it for the alter table query*/
	tok = wcsstr(fkname, L"FOREIGN KEY");
	*tok = 0; 
	fknamestr.SetAs(fkname);
	query.Sprintf("alter table `%s`.`%s` drop foreign key %s",
		m_db.GetString(), m_table.GetString(), fknamestr.GetString());

    free(fkname);

    res = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!res && m_tunnel->mysql_affected_rows(*m_mysql) == -1)
	{
		ShowMySQLError(m_hwndgrid, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}

	if(res)
		m_tunnel->mysql_free_result(res);

	/* delete it from the grid */
	CustomGrid_DeleteRow(m_hwndgrid, sel);

	/* if nothing is there then we need to disable the keys */
	if(!CustomGrid_GetRowCount(m_hwndgrid))
		EnableWindow(GetDlgItem(m_hwnd, IDC_DELETE), wyFalse);

	return wyTrue;
}

/*
wyBool
RelationshipMgmt::DeleteIndex()
{
	wyInt32			ret;
	CReorder	cro;
	// if there are no rows then dont allow anything to be deleted.
	if(CustomGrid_GetRowCount(m_hwndgrid)<= 0)
		return wyFalse;

	ret = cro.Reorder(m_hwnd, m_tunnel, m_mysql, (wyChar*)m_db.GetString(), (wyChar*)m_table.GetString());

	return (ret)?(wyTrue):(wyFalse);
}
*/

wyBool
RelationshipMgmt::ReseekIndex(wyWChar * index)
{
	wyInt32		count,counter;
	wyWChar	    *tempindexname = NULL;
	wyUInt32	len;

	count  = CustomGrid_GetRowCount(m_hwndgrid);

	if(count <= 0)
		return wyFalse;

	for(counter = 0; counter < count; counter++)
	{
		len = CustomGrid_GetItemTextLength(m_hwndgrid, counter, 0);		
		
		VERIFY(tempindexname = AllocateBuffWChar(len + 1));

		CustomGrid_GetItemText(m_hwndgrid, counter, 0, tempindexname);

		if(wcsstr(tempindexname, index))
		{
			CustomGrid_SetCurSelection(m_hwndgrid, counter, 0);
			free(tempindexname);
			return wyTrue;
		}
	}

	if(tempindexname)
		free(tempindexname);

	CustomGrid_SetCurSelection(m_hwndgrid, 0, 0);
	return wyFalse;
}

wyBool
RelationshipMgmt::AddRemaining()
{
	wyUInt32	len;
	wyInt32	    cursel, rowcount, rcount;
	wyChar	    *newname;
	wyWChar		*name;
    wyString    query;
	wyString	newnamestr, namestr;
    MYSQL_RES   *res;

	cursel = CustomGrid_GetCurSelection(m_hwndgrid);
	cursel = LOWORD(cursel);
	rowcount = CustomGrid_GetRowCount(m_hwndgrid);

	for(rcount = 0; rcount < rowcount; rcount++)
	{
	    if(rcount == cursel)
			continue;

		len = CustomGrid_GetItemTextLength(m_hwndgrid, rcount, 0);

		name = AllocateBuffWChar(len + 1);
		CustomGrid_GetItemText(m_hwndgrid,rcount, 0, name);
		namestr.SetAs(name);
		VERIFY(newname = FormatReference((wyChar *)namestr.GetString()));

		len  = strlen(newname);
		
		query.Sprintf("alter table `%s`.`%s` add foreign key %s", 
			m_db.GetString(), m_table.GetString(), newname);

        free(name);

        res = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
		if (!res && m_tunnel->mysql_affected_rows(*m_mysql)== -1)
		{
			ShowMySQLError(m_hwndgrid, m_tunnel, m_mysql, query.GetString());
			yog_message(m_hwndgrid, RELATIONERROR, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
			return wyFalse;
		}

		m_tunnel->mysql_free_result(res);
	}

	CustomGrid_DeleteRow(m_hwndgrid , cursel);
	if(cursel == 0 && rowcount)
		CustomGrid_SetCurSelection(m_hwndgrid, 0, 0);

	return wyTrue;
}

/*
	Implementation of New Foreign Dialog Box.
	This dialog allows you to create new foreign key relationship.
	*/
CCreateRel::CCreateRel()
{
	m_mysql = NULL;
	m_srcfieldslist	 = NULL;
	m_tgetfieldslist = NULL;
	m_isindexcreated = wyFalse;	
	m_isfkeychanged	 = wyFalse;
	m_isfkeylost     = wyFalse;
	m_relfldparam	 = NULL;
	m_iseditrelationship = wyFalse;
}

CCreateRel::~CCreateRel()
{
}

//class used to store unique index field name & insert into linked list
CUniqueIndexElem::CUniqueIndexElem(wyString &indexname)
{
	m_indexname.SetAs(indexname);
}

CUniqueIndexElem::~CUniqueIndexElem()
{
}


wyInt32
CCreateRel::Create(HWND m_hwnd, Tunnel * tunnel, PMYSQL mysql, wyChar *db, wyChar *table, wyChar *neweditindexname, RelFieldsParam *relfldparam)
{
	wyInt32		  ret;
	m_mysql		= mysql;
	m_tunnel	= tunnel;
	
	m_db.SetAs(db);
	m_table.SetAs(table);
	m_neweditindexname.SetAs(m_neweditindexname.GetString());

	/*Handles sets the source column(s) with values passed into the linked list*/	 
	if(relfldparam)
	{
		m_relfldparam = relfldparam;

		if(relfldparam->parenttable && relfldparam->parenttable->GetLength())
			m_referencedtable.SetAs(*relfldparam->parenttable);

		else 
			return 0;

		//List contains the values used to set the source columns 
		if(relfldparam->srcfldinfolist && relfldparam->srcfldinfolist->GetFirst())
			m_srcfieldslist = relfldparam->srcfldinfolist;

		//List contains the values used to set the target columns 
		if(relfldparam->iseditfkey == wyTrue && relfldparam->tgtfldinfolist && relfldparam->tgtfldinfolist->GetFirst())
		{
			m_iseditrelationship = wyTrue;
			m_tgetfieldslist = relfldparam->tgtfldinfolist;
		}				
	}

	//Post 8.01
	//RepaintTabModule();

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_NEWFOREIGN), m_hwnd, (DLGPROC)CCreateRel::DlgProc, (LPARAM)this);	

	///setd the linked list parameters with first row values of "Create relationship' dlg box if the F-key is created
	if(ret && relfldparam && m_iseditrelationship == wyFalse)
	{
		if(m_fknamestr.GetLength() && m_firstsrcfld.GetLength() && m_firsttgtfld.GetLength())
		{
			relfldparam->constraintname	= &m_fknamestr;
			relfldparam->firstsrcfld	= &m_firstsrcfld;	
			relfldparam->firsttgtfld	= &m_firsttgtfld;  
		}
		else
			return 0;
	}

	//If Edit Relationship is failed with same F-key name
	else
	{
		// Clear the F-key info buffer need to check for updating the "Relationship Mgmt" grid.
		if(m_isfkeylost == wyTrue)
			m_relfldparam->fkeyinfos->Clear();		
	}

	return ret;
}

wyInt32 CALLBACK
CCreateRel::DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CCreateRel * prel = (CCreateRel*)GetWindowLong(hwnd, GWL_USERDATA);
	wyBool	    ret;

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLong(hwnd, GWL_USERDATA, (wyInt32)lParam);
        LocalizeWindow(hwnd);
		SendMessage(GetDlgItem(hwnd, IDC_ONDELNOACTION), BM_SETCHECK, BST_CHECKED, 0);
		SendMessage(GetDlgItem(hwnd, IDC_ONUPDATENOACTION), BM_SETCHECK, BST_CHECKED, 0);
		PostMessage(hwnd, UM_CREATEGRID, 0, 0);
		break;
		
	case UM_CREATEGRID:
		// Set title for "Edit Relationship"
		if(prel->m_iseditrelationship == wyTrue)
			SendMessage(hwnd, (UINT)WM_SETTEXT, 0, (LPARAM)_(L"Edit Relationship"));

		prel->OnCreateGrid(hwnd);
	
		//sets the main dialog
		prel->MoveDialog();
        
		break;
		
	case UM_MOVEWINDOWS:
		PostMessage(hwnd, WM_INITGRIDVALUES, 0, 0);
		break;

	case WM_PAINT:
		//draws the resize gripper at bottom-right corner of dialog
		DrawSizeGripOnPaint(hwnd);
	break;

	case WM_HELP:
		ShowHelp("Create%20FK.htm");
		return wyTrue;

	case WM_INITGRIDVALUES:
		ret = prel->InitGridData();
		if(ret == wyFalse)
		{
			return 0;
		}
		ret = prel->FillCombo ();
		if(ret == wyFalse)
		{
			yog_enddialog(hwnd, 0);
			return 0;
		}

		//Fill the all the fields in the dialog.
		ret = prel->FillDefaultVaues();
		break;

	case WM_COMMAND:
		prel->OnWmCommand(hwnd, wParam, lParam);
		break;

	case WM_SIZE:
		prel->Resize(lParam);
		break;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO pMMI = (LPMINMAXINFO)lParam;
			pMMI->ptMinTrackSize.x = HANDLERELWD;
			pMMI->ptMinTrackSize.y = HANDLERELHT;
		}
		break;

	case WM_DESTROY:
		//StoreDialogPersist(hwnd, HANDLERELATION_SECTION);
		SaveInitPos(hwnd, HANDLERELATION_SECTION);
		return 0;
		
	}	

	return 0;
}

void
CCreateRel::OnCreateGrid(HWND hwnd)
{
    wyString deffkname;

	m_hwnd = hwnd;
		
	VERIFY(m_hwndgrid = CreateCustomGrid(hwnd, 200, 10, 50, 50, (GVWNDPROC)GVWndProc, (LPARAM)this));
	EnableWindow(m_hwndgrid, wyFalse);
	PostMessage(hwnd, UM_MOVEWINDOWS, 0, 0);
	VERIFY(SetWindowPos(m_hwndgrid, GetDlgItem(hwnd, IDC_INDEXNAME), 0, 0, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOMOVE));
	SetWindowText(GetDlgItem(hwnd, IDC_TABLENAME), m_table.GetAsWideChar());
	deffkname.Sprintf("FK_%s", m_table.GetString());
	SetWindowText(GetDlgItem(hwnd, IDC_FKNAME), deffkname.GetAsWideChar());
	
}

void
CCreateRel::OnWmCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	if(HIWORD(wParam)== CBN_SELENDOK)
	{
		ChangeDestColumns((HWND)lParam);
		EnableWindow(m_hwndgrid, wyTrue);
		return;

	} 
	else if(HIWORD(wParam)== BN_CLICKED)
	{
		switch(LOWORD(wParam))
		{
		case IDC_ONDELETE:
			OnDelete(hwnd, lParam);
			break; 

		case IDC_ONUPDATE:
			OnUpdate(hwnd, lParam);
			break;

		case IDC_ONUPDATECASCADE:
			OnUpdateCascade(hwnd, lParam);
			break;

		case IDC_ONUPDATENULL:
			OnUpdateNull(hwnd, lParam);
			break;

		case IDC_ONUPDATENOACTION:
			OnUpdateAction(hwnd, lParam);
			break;

		case IDC_ONUPDATERESTRICT:
			OnUpdateRestrict(hwnd, lParam);
			break;

		case IDC_ONDELCASCADE:
		    OnDelCascade(hwnd, lParam);
			break;

		case IDC_ONDELNULL:
            OnDelNull(hwnd, lParam);
			break;

		case IDC_ONDELNOACTION:
			OnDelNoAction(hwnd, lParam);
            break;

		case IDC_ONDELRESTRICT:
			OnDelRestrict(hwnd, lParam);
			break;
		}
	}

	switch(LOWORD(wParam))
	{
	case IDCANCEL:
		yog_enddialog(hwnd, 0);
		break;

	case IDC_KEYINDEX:
		ShowKeyIndex ();
		break;

	case IDC_REFINDEX:
		ShowRefIndex ();
		break;

	case IDOK:
	    CreateIndex(hwnd);
        break;
	}
}

void 
CCreateRel::CreateIndex(HWND hwnd)
{
	wyInt32 ret;

	SetCursor(LoadCursor(NULL, IDC_WAIT));
	
    /*Check whether the F-key info changed in the "Edit Relationship" dialog box, if yes delete the F-key , and create 
	new one by using the new values*/
	if(m_iseditrelationship == wyTrue)
	{
		m_isfkeychanged = IsForeignKeyEdited();
		
		//If the Foreign key is not at all changed just skip this function
		if(m_isfkeychanged == wyFalse)
		{
			VERIFY(yog_enddialog(m_hwnd, 0));
			return;
		}
	}
    
	//This fun. is getting called for both Create New F-key & Edit F-key . 
	//For Edit Relationship: we are creating a new one and droping the old one in a single query. 
	ret = CreateNewIndex();

	if(ret)
	{
		GetIndexName();
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		yog_enddialog(hwnd, (wyInt32)1);
		
		//Flag sets wyTrue if rel.ship created successfully
		m_isindexcreated = wyTrue;
        return;
	}

	SetCursor(LoadCursor(NULL, IDC_ARROW));
	
	//post 8.01
	//InvalidateRect(m_hwndgrid, NULL, FALSE);  			
	//UpdateWindow(m_hwndgrid);
    return;
}

void 
CCreateRel::OnDelete(HWND hwnd, LPARAM lparam)
{
	wyInt32		nid[] = { IDC_ONDELCASCADE, IDC_ONDELNULL, IDC_ONDELNOACTION, IDC_ONDELRESTRICT	};
	wyInt32		size= sizeof(nid)/ sizeof(nid[0]);
	wyInt32		counter;
	if(SendMessage((HWND)lparam, BM_GETCHECK, 0, 0) == BST_CHECKED)
    {
		for(counter = 0; counter < size; counter++)
			EnableWindow(GetDlgItem(hwnd, nid[counter]), wyTrue );
	} 
    else 
    {
		for(counter=0; counter<size; counter++)
			EnableWindow(GetDlgItem(hwnd, nid[counter]), wyFalse);
	}
}

void 
CCreateRel::OnUpdate(HWND hwnd, LPARAM lparam)
{
	wyInt32		nid[] = { IDC_ONUPDATECASCADE, IDC_ONUPDATENULL, IDC_ONUPDATENOACTION, IDC_ONUPDATERESTRICT	};
	wyInt32		size= sizeof(nid)/ sizeof(nid[0]);
	wyInt32		counter;
	if(SendMessage((HWND)lparam, BM_GETCHECK, 0, 0)== BST_CHECKED)
    {
		for(counter = 0; counter < size; counter++)
			EnableWindow(GetDlgItem(hwnd, nid[counter]), wyTrue );

	} 
    else 
    {
		for(counter = 0; counter < size; counter++)
			EnableWindow(GetDlgItem(hwnd, nid[counter]), wyFalse );
	}
}

void 
CCreateRel::OnUpdateCascade(HWND hwnd, LPARAM lparam)
{
	wyInt32		nid[] = { IDC_ONUPDATENULL, IDC_ONUPDATENOACTION, IDC_ONUPDATERESTRICT	};
	wyInt32		size= sizeof(nid)/ sizeof(nid[0]);

	UnCheckAll(hwnd, lparam, nid, size);
}

void 
CCreateRel::OnUpdateNull(HWND hwnd, LPARAM lparam)
{
    wyInt32		nid[] = { IDC_ONUPDATECASCADE, IDC_ONUPDATENOACTION, IDC_ONUPDATERESTRICT	};
    wyInt32		size= sizeof(nid)/ sizeof(nid[0]);
    
    UnCheckAll(hwnd, lparam, nid, size);
}

void 
CCreateRel::OnUpdateAction(HWND hwnd, LPARAM lparam)
{
	wyInt32		nid[] = { IDC_ONUPDATECASCADE, IDC_ONUPDATENULL, IDC_ONUPDATERESTRICT	};
	wyInt32		size= sizeof(nid)/ sizeof(nid[0]);

    UnCheckAll(hwnd, lparam, nid, size);
}

void 
CCreateRel::OnUpdateRestrict(HWND hwnd, LPARAM lparam)
{
	wyInt32		nid[] = { IDC_ONUPDATECASCADE, IDC_ONUPDATENULL, IDC_ONUPDATENOACTION };
	wyInt32		size= sizeof(nid)/ sizeof(nid[0]);

    UnCheckAll(hwnd, lparam, nid, size);
}

void 
CCreateRel::OnDelCascade(HWND hwnd, LPARAM lparam)
{
	wyInt32		nid[] = {IDC_ONDELNULL, IDC_ONDELNOACTION, IDC_ONDELRESTRICT};
	wyInt32		size = sizeof(nid)/ sizeof(nid[0]);

    UnCheckAll(hwnd, lparam, nid, size);
}

void 
CCreateRel::OnDelNull(HWND hwnd, LPARAM lparam)
{
	wyInt32		nid[] = { IDC_ONDELCASCADE, IDC_ONDELNOACTION, IDC_ONDELRESTRICT	};
	wyInt32		size= sizeof(nid)/ sizeof(nid[0]);

    UnCheckAll(hwnd, lparam, nid, size);
}

void 
CCreateRel::OnDelNoAction(HWND hwnd, LPARAM lparam)
{
	wyInt32		nid[] = { IDC_ONDELCASCADE, IDC_ONDELNULL, IDC_ONDELRESTRICT };
	wyInt32		size= sizeof(nid)/ sizeof(nid[0]);
    
    UnCheckAll(hwnd, lparam, nid, size);
}

void 
CCreateRel::OnDelRestrict(HWND hwnd, LPARAM lparam)
{
	wyInt32		nid[] = { IDC_ONDELCASCADE, IDC_ONDELNULL, IDC_ONDELNOACTION	};
	wyInt32		size= sizeof(nid)/ sizeof(nid[0]);

    UnCheckAll(hwnd, lparam, nid, size);
}

void 
CCreateRel::UnCheckAll(HWND hwnd, LPARAM lparam, wyInt32 *nid, wyInt32 size)
{
    wyInt32		count;

	if(SendMessage((HWND)lparam, BM_GETCHECK, 0, 0)== BST_CHECKED)
    {
		for(count = 0; count < size; count++)
			SendMessage(GetDlgItem(hwnd, (*(nid + count))), BM_SETCHECK, BST_UNCHECKED, 0);

	} 

}

LRESULT CALLBACK
CCreateRel::GVWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CCreateRel	*crel = (CCreateRel*)CustomGrid_GetLongData(hwnd);
	wyString	tblname("__create_relation");

	switch(message)
	{
		case GVN_BEGINCHANGEROW:
			return wyTrue;

		case GVN_SPLITTERMOVE:
			OnGridSplitterMove(hwnd, &tblname, wParam);	
			break;

		case GVN_ENDLABELEDIT:
			//For Edit Relationship , set a flag when grid is edited
			if(crel->m_iseditrelationship == wyTrue)
				crel->m_isfkeychanged = wyTrue;			
			return wyTrue;
			
		default:
			break;
	}

	return wyTrue;
}

void		
CCreateRel::MoveDialog()
{
	HICON hicon;

	//Set icon for dialog	
	hicon = CreateIcon(IDI_MANREL_16);
	SendMessage(m_hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hicon);

	//SetDialogPos(m_hwnd, HANDLERELATION_SECTION);
	SetInitPos(m_hwnd, HANDLERELATION_SECTION);
}

void
CCreateRel::Resize(LPARAM lParam)
{
	HWND	hwndtopgrp, hwndcombo, hwndtxt, hwndtopline, hwndbtmline, hwndbtmline2;
	HWND	hwndok, hwndcancel, hwnddelgrp, hwndupdgrp, hwndeditbutton;
	HWND	hwndoptcsd, hwndoptnul, hwndoptno, hwndoptrst, hwndoptupd, hwndoptdel;
	RECT	rct, rcttmp, rctopt, rctupdtopt, rctcp, rctgp;
	wyInt32 height, width, twd, tht, ttop, tlt;

	width = LOWORD(lParam);
	height = HIWORD(lParam);

	VERIFY(hwndtopgrp = GetDlgItem(m_hwnd, IDC_GRPBASIC));
	VERIFY(hwndcombo = GetDlgItem(m_hwnd, IDC_TABLELIST));
	VERIFY(hwndtxt = GetDlgItem(m_hwnd, IDC_FKNAME));
	VERIFY(hwndtopline = GetDlgItem(m_hwnd, IDC_TOPLINE));
	VERIFY(hwndbtmline = GetDlgItem(m_hwnd, IDC_BOTLINE));
	VERIFY(hwndbtmline2 = GetDlgItem(m_hwnd, IDC_BOTTOMLINE));
	VERIFY(hwndok = GetDlgItem(m_hwnd, IDOK));
	VERIFY(hwndcancel = GetDlgItem(m_hwnd, IDCANCEL));
	VERIFY(hwnddelgrp = GetDlgItem(m_hwnd, IDC_GRP41));
	VERIFY(hwndupdgrp = GetDlgItem(m_hwnd, IDC_GRP51));
	VERIFY(hwndeditbutton = GetDlgItem(m_hwnd, IDC_REFINDEX));
	
	VERIFY(GetWindowRect(hwndtopgrp, &rcttmp));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcttmp, 2));
	
	//Combo grp box
	MoveWindow(hwndtopgrp, rcttmp.left, rcttmp.top, width - rcttmp.left - 10, rcttmp.bottom - rcttmp.top, FALSE);
	VERIFY(GetWindowRect(hwndtopgrp, &rcttmp));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rcttmp, 2));

	//Grp button and combobox
	VERIFY(GetWindowRect(hwndeditbutton, &rct));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rct, 2));

	//index button
	tht = rct.bottom - rct.top;
	twd = rct.right - rct.left;
	tlt = rcttmp.right - (rct.right - rct.left) - 4;// + 2;
	ttop = rct.top;
	
	VERIFY(MoveWindow(hwndeditbutton, tlt, ttop, twd, tht, FALSE));
	VERIFY(GetWindowRect(hwndeditbutton, &rct));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rct, 2));

	//Combo
	VERIFY(GetWindowRect(hwndcombo, &rctopt));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctopt, 2));
	
	tht = rctopt.bottom - rctopt.top;
	twd = (rct.left - WIDTHBETBUTTON) -  rcttmp.left - 4;
	tlt = rcttmp.left + 2;
	ttop = rctopt.top;
	VERIFY(MoveWindow(hwndcombo, tlt, ttop, twd, tht, FALSE));
	
	//Text box
	VERIFY(GetWindowRect(hwndtxt, &rct));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rct, 2));

	tht = rct.bottom - rct.top;
	twd = rcttmp.right - rcttmp.left;
	tlt = rcttmp.left;
	ttop = rct.top;
	MoveWindow(hwndtxt, tlt, ttop, twd, tht, FALSE);
	
	//Buttons
    VERIFY(GetClientRect(hwndcancel, &rctopt)); 
	MoveWindow(hwndcancel, width - (rctopt.right) - WIDTHBETBUTTON, height - 30, rctopt.right, rctopt.bottom, FALSE);
	VERIFY(GetWindowRect(hwndcancel, &rctopt));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctopt, 2));

	//Ok button
	VERIFY(GetClientRect(hwndcancel, &rct)); 
	MoveWindow(hwndok, (rctopt.left - WIDTHBETBUTTON) - rct.right, rctopt.top, rct.right, rct.bottom, FALSE);

	//Mottom lne 2
	VERIFY(GetClientRect(hwndbtmline2, &rct));
	MoveWindow(hwndbtmline2, 10, rctopt.top - 5, rcttmp.right - 10, 1, FALSE);
	VERIFY(GetWindowRect(hwndbtmline2, &rct));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rct, 2));
		
	//Update section
	VERIFY(hwndoptcsd = GetDlgItem(m_hwnd, IDC_ONUPDATECASECADE));
	VERIFY(hwndoptnul = GetDlgItem(m_hwnd, IDC_ONUPDATENULL));
	VERIFY(hwndoptno = GetDlgItem(m_hwnd, IDC_ONUPDATENOACTION));
	VERIFY(hwndoptrst = GetDlgItem(m_hwnd, IDC_ONUPDATERESTRICT));
	VERIFY(hwndoptupd = GetDlgItem(m_hwnd, IDC_ONUPDATE));

	VERIFY(GetClientRect(hwndupdgrp, &rctgp));

	MoveWindow(hwndupdgrp, rct.left, rct.top - (rctgp.bottom - rctgp.top) - 5, 
		rct.right - rct.left, rctgp.bottom - rctgp.top, FALSE);
	VERIFY(GetWindowRect(hwndupdgrp, &rctgp));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctgp, 2));
        
	VERIFY(GetClientRect(hwndoptcsd, &rctupdtopt)); //cascade
	MoveWindow(hwndoptcsd, rctgp.left + WIDTHBETBUTTON * 3, rctgp.top + (WIDTHBETBUTTON * 2) - 1, 
		rctupdtopt.right, rctupdtopt.bottom, FALSE);
	VERIFY(GetWindowRect(hwndoptcsd, &rctcp));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctcp, 2));

	VERIFY(GetClientRect(hwndoptnul, &rctupdtopt)); //Set Null
	MoveWindow(hwndoptnul, rctcp.right + WIDTHBETBUTTON, rctcp.top,	rctupdtopt.right, rctupdtopt.bottom, FALSE);
	VERIFY(GetWindowRect(hwndoptnul, &rctcp));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctcp, 2));

	VERIFY(GetClientRect(hwndoptno, &rctupdtopt)); //No action
	MoveWindow(hwndoptno, rctcp.right + WIDTHBETBUTTON, rctcp.top,	rctupdtopt.right, rctupdtopt.bottom, FALSE);
	VERIFY(GetWindowRect(hwndoptno, &rctcp));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctcp, 2));
		
	VERIFY(GetClientRect(hwndoptrst, &rctupdtopt)); //Restrict
	MoveWindow(hwndoptrst, rctcp.right + WIDTHBETBUTTON, rctcp.top,	rctupdtopt.right, rctupdtopt.bottom, FALSE);

	VERIFY(GetClientRect(hwndoptupd, &rctupdtopt)); //Update checkbox
	MoveWindow(hwndoptupd, rctgp.left + WIDTHBETBUTTON, rctgp.top - 2,
		rctupdtopt.right, rctupdtopt.bottom, FALSE);

	VERIFY(GetWindowRect(hwndoptupd, &rct));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rct, 2));

	//DELETE Option
	VERIFY(hwndoptcsd = GetDlgItem(m_hwnd, IDC_ONDELCASCADE));
	VERIFY(hwndoptnul = GetDlgItem(m_hwnd, IDC_ONDELNULL));
	VERIFY(hwndoptno = GetDlgItem(m_hwnd, IDC_ONDELNOACTION));
	VERIFY(hwndoptrst = GetDlgItem(m_hwnd, IDC_ONDELRESTRICT));
	VERIFY(hwndoptdel = GetDlgItem(m_hwnd, IDC_ONDELETE));

	tht = rctgp.bottom - rctgp.top;
	twd = rctgp.right - rctgp.left;
	tlt = rctgp.left;
		
	VERIFY(GetClientRect(hwnddelgrp, &rctgp));

	//Delete option group
	MoveWindow(hwnddelgrp, tlt, rct.top - (rctgp.bottom - rctgp.top) - 5, twd, tht, FALSE);
	VERIFY(GetWindowRect(hwnddelgrp, &rctgp));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctgp, 2));
        
	VERIFY(GetClientRect(hwndoptcsd, &rctupdtopt)); //cascade
	MoveWindow(hwndoptcsd, rctgp.left + WIDTHBETBUTTON * 3, rctgp.top + (WIDTHBETBUTTON * 2) - 1, 
		rctupdtopt.right, rctupdtopt.bottom, FALSE);
	VERIFY(GetWindowRect(hwndoptcsd, &rctcp));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctcp, 2));

	VERIFY(GetClientRect(hwndoptnul, &rctupdtopt)); //Set Null
	MoveWindow(hwndoptnul, rctcp.right + WIDTHBETBUTTON, rctcp.top,	rctupdtopt.right, rctupdtopt.bottom, FALSE);
	VERIFY(GetWindowRect(hwndoptnul, &rctcp));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctcp, 2));

	VERIFY(GetClientRect(hwndoptno, &rctupdtopt)); //No action
	MoveWindow(hwndoptno, rctcp.right + WIDTHBETBUTTON, rctcp.top,	rctupdtopt.right, rctupdtopt.bottom, FALSE);
	VERIFY(GetWindowRect(hwndoptno, &rctcp));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctcp, 2));
		
	VERIFY(GetClientRect(hwndoptrst, &rctupdtopt)); //Restrict
	MoveWindow(hwndoptrst, rctcp.right + WIDTHBETBUTTON, rctcp.top,	rctupdtopt.right, rctupdtopt.bottom, FALSE);

	VERIFY(GetClientRect(hwndoptdel, &rctupdtopt)); //Delete checkbox
	MoveWindow(hwndoptdel, rctgp.left + WIDTHBETBUTTON, rctgp.top - 2,// + rctupdtopt.bottom / 2, 
		rctupdtopt.right, rctupdtopt.bottom, FALSE);
    	
	//Topline & middle line
	VERIFY(GetWindowRect(hwndbtmline2, &rctopt));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctopt, 2));

	VERIFY(GetWindowRect(hwndtopline, &rctcp));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctcp, 2));

	//top line
	MoveWindow(hwndtopline, rctopt.left, rctcp.top, rctopt.right - rctopt.left, rctopt.bottom - rctopt.top, FALSE);

	//Middle line
	VERIFY(GetWindowRect(hwndbtmline, &rctcp));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctcp, 2));
	MoveWindow(hwndbtmline, rctopt.left, rctgp.top - WIDTHBETBUTTON, rctopt.right - rctopt.left, rctopt.bottom - rctopt.top, FALSE);

	//Grid
	VERIFY(GetWindowRect(hwndtopline, &rct));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rct, 2));
	VERIFY(GetWindowRect(hwndbtmline, &rctcp));
	VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rctcp, 2));
	MoveWindow(m_hwndgrid, rct.left, rct.bottom, rct.right - rct.left, rctcp.top - rct.bottom, FALSE);

	HandleIndexDlgsFlicker(m_hwnd, m_hwndgrid);
		
	return;
}

wyBool
CCreateRel::InitGridData()
{
	wyUInt32    colcount, width;
	RECT        rect = {0};
	GVCOLUMN    gvcol;

	VERIFY(GetClientRect(m_hwndgrid, &rect));

	width       =  (rect.right-rect.left) - 70;

	wyChar		*heading[] = { "Source Column", "Target Column" };
	wyInt32     elemsize[] = { 0, 0, 0, 0 , 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 };
	wyInt32     elemcount[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	wyInt32     mask[] = { GVIF_LIST, GVIF_LIST };  
	wyInt32     cx[2];
	wyInt32	    fmt[] = { GVIF_LEFT, GVIF_LEFT };

	wyInt32		colwidth = 0;
	wyString	colname, dbname(RETAINWIDTH_DBNAME), tblname("__create_relation");
	wyBool		isretaincolumnwidth = IsRetainColumnWidth();

	cx[0] = width / 2; cx[1] = width / 2;

	for(colcount = 0; colcount < sizeof(heading)/ sizeof(heading[0]); colcount++)
	{
		if(isretaincolumnwidth == wyTrue)
		{
			//for getting the retained column width
			colname.SetAs(heading[colcount]);
			colwidth = GetColumnWidthFromFile(&dbname, &tblname, &colname);
		}

		memset(&gvcol, 0, sizeof(gvcol));		
		gvcol.mask = mask[colcount];
		gvcol.fmt  = fmt[colcount];
		gvcol.cx   = (colwidth > 0)? colwidth : cx[colcount];
		gvcol.text = heading[colcount];
		gvcol.nElemSize = elemsize[colcount];
		gvcol.nListCount = elemcount[colcount];
		gvcol.cchTextMax = strlen(heading[colcount]);

		VERIFY(CustomGrid_InsertColumn(m_hwndgrid, &gvcol)>= 0);
	}

	for(colcount = 0; colcount < INITROWCOUNT; colcount++)
		CustomGrid_InsertRow(m_hwndgrid);

	FillGridData();

	return wyTrue;
}

wyBool
CCreateRel::FillCombo()
{
	wyString    query, myrowstr;
	wyInt32     count;
	HWND        hwndcombo;

	MYSQL_RES	*myres;
	MYSQL_ROW	myrow;
        wyBool          ismysql41 = ((GetActiveWin())->m_ismysql41);

	count = PrepareShowTable(m_tunnel, m_mysql, m_db, query);

	VERIFY(hwndcombo = GetDlgItem(m_hwnd, IDC_TABLELIST));

	VERIFY(myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query));

	if(!myres)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}
	
	count = -1;
	while(myrow = m_tunnel->mysql_fetch_row(myres))
	{
		myrowstr.SetAs(myrow[0], ismysql41);
		count = SendMessage(hwndcombo, CB_ADDSTRING, 0, (LPARAM)myrowstr.GetAsWideChar());
	}

	if(count == -1)
    {
		yog_message(m_hwnd, _(L"No table found to create relations"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION | MB_HELP);
		m_tunnel->mysql_free_result(myres);
		return wyFalse;
	}

	m_tunnel->mysql_free_result(myres);
	return wyTrue;
}

//Handle fills the Dialog box with default values
wyBool
CCreateRel::FillDefaultVaues()
{
	wyInt32				srcrow = 0, ret;
	wyBool				retval;
	HWND				hwndcombo;
	RelTableFldInfo		*srcfldinfo = NULL, *tempinfo = NULL;
	wyInt32				itemcount = 0, index, len, casevalue, cmp = 0;
	wyWChar				*tabletemp = NULL;
	wyString			tablename;

	MDIWindow			*wnd = NULL;

	VERIFY(wnd = GetActiveWin());
		
	//read - 'lower_case_table_names'
	/*
	For case sensitive File system(Linux), if variable return - 0 use 'Compare'
	For case insensitive File system(Windows), if variable return != 0 use 'CompareI'
	*/
	casevalue = GetmySQLCaseVariable(wnd);

	VERIFY(hwndcombo = GetDlgItem(m_hwnd, IDC_TABLELIST));
	
	if(!m_srcfieldslist || !m_srcfieldslist->GetFirst())
		return wyTrue;
		
	srcfldinfo = (RelTableFldInfo*)m_srcfieldslist->GetFirst();
	while(srcfldinfo)
	{
		if(srcfldinfo->m_tablefld.GetLength())
		{
			CustomGrid_SetText(m_hwndgrid, srcrow, 0, srcfldinfo->m_tablefld.GetString());
			srcrow = srcrow + 1;
		}
		tempinfo = (RelTableFldInfo	*)srcfldinfo->m_next;

		m_srcfieldslist->Remove(srcfldinfo);
		delete srcfldinfo;

		srcfldinfo = tempinfo;
	}	 

	if(hwndcombo && m_referencedtable.GetLength())
	{
		/// Set the referenced table name to the combo text box		
		VERIFY((itemcount = SendMessage(hwndcombo, CB_GETCOUNT, (WPARAM)0, (LPARAM)0)) != CB_ERR);
        
		for(index = 0; index < itemcount; index++)
		{
			VERIFY((len = SendMessage(hwndcombo, CB_GETLBTEXTLEN, (WPARAM)index, (LPARAM)m_referencedtable.GetAsWideChar())) != CB_ERR);

			tabletemp = AllocateBuffWChar(len + 1);
			VERIFY((len = SendMessage(hwndcombo, CB_GETLBTEXT, (WPARAM)index, (LPARAM)tabletemp)) != CB_ERR);
			tablename.SetAs(tabletemp);
			if(tabletemp)
			{
				free(tabletemp);
				tabletemp = NULL;
			}

			if(casevalue == 0) // For case insensitive file systems
				cmp = m_referencedtable.Compare(tablename);

			else // for the value 1 & 2 , for this case mysql look up table names in lower case
				cmp = m_referencedtable.CompareI(tablename);

			//Fills the 'eference table' combo box
			if(!cmp)
			{
				VERIFY((ret = SendMessage(hwndcombo, CB_SETCURSEL,(WPARAM)index, 0)) != CB_ERR);
				break;
			}
		}

		//The combobox be enabled in case of the "Edit Relationship"
		if(m_iseditrelationship == wyTrue)
			EnableWindow(hwndcombo, TRUE);

		//Fills the target column list box
		retval = FillGridDestColumns(m_referencedtable.GetString());
        
		if(retval == wyFalse)
			return wyFalse;

		//Fills the Destination column by either P-key or by unique key
		FillDestColumnField(m_referencedtable.GetString());

		EnableWindow(m_hwndgrid, TRUE);   
	}

	//Sets F-key option as per the selected F-key to be edited
	SetDialogFkeyOption();
	 
	return wyTrue;
}

//Enable the Corresponding F-key option in the dialog box
void
CCreateRel::SetDialogFkeyOption()
{
	fkeyOption ondelete, onupdate;

	if(m_iseditrelationship == wyFalse || !m_relfldparam)
		return;
	
	//Set the corresponding F-key options(ON DELETE / ON UPDATE) if it is defined
	ondelete = m_relfldparam->ondelete;
	onupdate = m_relfldparam->onupdate;

	if(ondelete != NOOPTION)
	{
		SendMessage(GetDlgItem(m_hwnd, IDC_ONDELETE), BM_SETCHECK, BST_CHECKED, 0);

		EnableWindow(GetDlgItem(m_hwnd, IDC_ONDELCASCADE), TRUE);
		EnableWindow(GetDlgItem(m_hwnd, IDC_ONDELNULL), TRUE);
		EnableWindow(GetDlgItem(m_hwnd, IDC_ONDELNOACTION), TRUE);
		EnableWindow(GetDlgItem(m_hwnd, IDC_ONDELRESTRICT), TRUE);
	}

	if(onupdate != NOOPTION)
	{
		SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATE), BM_SETCHECK, BST_CHECKED, 0);

		EnableWindow(GetDlgItem(m_hwnd, IDC_ONUPDATECASCADE), TRUE);
		EnableWindow(GetDlgItem(m_hwnd, IDC_ONUPDATENULL), TRUE);
		EnableWindow(GetDlgItem(m_hwnd, IDC_ONUPDATENOACTION), TRUE);
		EnableWindow(GetDlgItem(m_hwnd, IDC_ONUPDATERESTRICT), TRUE);
	}
		
	//For the option ON DELETE
	switch(ondelete)
		{
		case NOOPTION:
			break;

		case CASCADE:
			SendMessage(GetDlgItem(m_hwnd, IDC_ONDELETE), BM_SETCHECK, BST_CHECKED, 0);

			SendMessage(GetDlgItem(m_hwnd, IDC_ONDELCASCADE), BM_SETCHECK, BST_CHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONDELNULL), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONDELNOACTION), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONDELRESTRICT), BM_SETCHECK, BST_UNCHECKED, 0);			
			break;

		case SETNULL:
			SendMessage(GetDlgItem(m_hwnd, IDC_ONDELNULL), BM_SETCHECK, BST_CHECKED, 0);

			SendMessage(GetDlgItem(m_hwnd, IDC_ONDELCASCADE), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONDELNOACTION), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONDELRESTRICT), BM_SETCHECK, BST_UNCHECKED, 0);			
			break;

		case NOACTION:
			SendMessage(GetDlgItem(m_hwnd, IDC_ONDELNOACTION), BM_SETCHECK, BST_CHECKED, 0);

			SendMessage(GetDlgItem(m_hwnd, IDC_ONDELCASCADE), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONDELNULL), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONDELRESTRICT), BM_SETCHECK, BST_UNCHECKED, 0);
			break;

		case RESTRICT:
			SendMessage(GetDlgItem(m_hwnd, IDC_ONDELRESTRICT), BM_SETCHECK, BST_CHECKED, 0);

			SendMessage(GetDlgItem(m_hwnd, IDC_ONDELCASCADE), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONDELNULL), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONDELNOACTION), BM_SETCHECK, BST_UNCHECKED, 0);
			break;
		}		

		//For the option ON UPDATE
		switch(onupdate)
		{
		case NOOPTION:
			break;

		case CASCADE:
			
			SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATECASCADE), BM_SETCHECK, BST_CHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATENULL), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATENOACTION), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATERESTRICT), BM_SETCHECK, BST_UNCHECKED, 0);			
			break;

		case SETNULL:
			SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATENULL), BM_SETCHECK, BST_CHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATECASCADE), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATENOACTION), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATERESTRICT), BM_SETCHECK, BST_UNCHECKED, 0);			
			break;

		case NOACTION:
			SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATENOACTION), BM_SETCHECK, BST_CHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATECASCADE), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATENULL), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATERESTRICT), BM_SETCHECK, BST_UNCHECKED, 0);
			break;

		case RESTRICT:
			SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATERESTRICT), BM_SETCHECK, BST_CHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATECASCADE), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATENULL), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATENOACTION), BM_SETCHECK, BST_UNCHECKED, 0);
			break;
		}		
}

wyBool
CCreateRel::FillGridData()
{
	wyString    query, myrowstr;

	MYSQL_RES	*myres;
	MYSQL_ROW	myrow;

	query.Sprintf("show full fields from `%s`.`%s`", 
							m_db.GetString(), m_table.GetString());

    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
    if(!myres)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}

	/// Insert an emty row 
	CustomGrid_InsertTextInList(m_hwndgrid, 0, L"");

	while(myrow = m_tunnel->mysql_fetch_row(myres))
	{        
		myrowstr.SetAs(myrow[0], (GetActiveWin())->m_ismysql41);
		CustomGrid_InsertTextInList(m_hwndgrid, 0, myrowstr.GetAsWideChar());
	}

	m_tunnel->mysql_free_result(myres);

	return wyTrue;
}

wyBool
CCreateRel::ChangeDestColumns(HWND hwndCombo)
{
	wyString	tablestr;
	wyInt32		cursel;
	wyWChar	    table[SIZE_512] = {0};
	wyBool		ret;

	VERIFY((cursel = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0))!= CB_ERR);

	VERIFY((SendMessage(hwndCombo, CB_GETLBTEXT, cursel, (LPARAM)table)) != CB_ERR);

	tablestr.SetAs(table);

         // Fills the target column list box
	ret = FillGridDestColumns(tablestr.GetString());
    
	if(ret == wyFalse)
		return wyFalse;

	//Fill the target column with primary key(s) or unique index(s)
	FillDestColumnField(tablestr.GetString());
	
	return wyTrue;
} 

///Fill the target column list box
wyBool
CCreateRel::FillGridDestColumns(const wyChar *reftabletable)
{
	wyString	query, tablestr, myrowstr;
	MYSQL_RES	*myres;
	MYSQL_ROW	myrow;
	wyBool		ismysql41 = (GetActiveWin())->m_ismysql41;

	query.Sprintf("show full fields from `%s`.`%s`", m_db.GetString(), reftabletable);

	CustomGrid_DeleteListContent(m_hwndgrid, 1);

    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!myres)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
		return wyFalse;
	}
	
	/// Insert an empty row
	CustomGrid_InsertTextInList(m_hwndgrid, 1, L"");

	while(myrow = m_tunnel->mysql_fetch_row	(myres))
	{
		myrowstr.SetAs(myrow[0], ismysql41);
		CustomGrid_InsertTextInList(m_hwndgrid, 1, myrowstr.GetAsWideChar());
	}

	m_tunnel->mysql_free_result(myres);
	return wyTrue;
}
/// Filling the target column by primary key or by ubnique index
void	
CCreateRel::FillDestColumnField(const wyChar *reftabletable)
{
	wyBool				ismysql41, retbool;
	wyInt32				i, rowcount;
	List				uniquelist;

	ismysql41 = (GetActiveWin())->m_ismysql41;

	rowcount = CustomGrid_GetRowCount(m_hwndgrid);	

	//Clear all Target columns 
	for(i = 0; i< rowcount; i++)
		CustomGrid_SetText(m_hwndgrid, i, 1, "");

	/*For Edit Relationship: the dest column contains only the fields that contins in F-ky relationship when the dialog box pops up, 
	Once the Referenced table (combo)is changed the dest. columns fill with P-key/Unique key of selected table*/
	if(m_iseditrelationship == wyTrue && !(m_referencedtable.Compare(reftabletable)))
        retbool = DestColumnsFillDefaultValues();		

	else
		// Fills the grid destination columns with Primary keys / Unique keys of the referenced table
		retbool = DestColumnsFillKeys(reftabletable);

	return;
}
/// Fills the grid destination columns with Primary keys / Unique keys of the referenced table
wyBool		 
CCreateRel::DestColumnsFillKeys(const wyChar *reftabletable)
{
	wyString			query, myrowstr, colname, keytype, keyname, uniquekeymame;
	MYSQL_RES			*myres;
	MYSQL_ROW			myrow;
	wyBool				ismysql41, isprimary = wyFalse;
	wyInt32				tgtrow = 0, rowcount, fldindex = -1;
	List				uniquelist;
	CUniqueIndexElem	*uniqueinfo = NULL, *tempindex = NULL;

	ismysql41 = (GetActiveWin())->m_ismysql41;

	rowcount = CustomGrid_GetRowCount(m_hwndgrid);	
	
	query.Sprintf("show keys from `%s`.`%s`", m_db.GetString(), reftabletable);
	
    myres = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	
	while(myrow = m_tunnel->mysql_fetch_row(myres))
	{
		fldindex = GetFieldIndex(m_tunnel, myres, "Non_unique"); 
		if(fldindex >= 0)
			myrowstr.SetAs(myrow[fldindex], ismysql41);

		fldindex = GetFieldIndex(m_tunnel, myres, "Key_name"); 
		if(fldindex >= 0)
			keyname.SetAs(myrow[fldindex], ismysql41);
		
		fldindex = GetFieldIndex(m_tunnel, myres, "Column_name"); 
		if(fldindex >= 0)
			colname.SetAs(myrow[fldindex], ismysql41);

		if(isprimary == wyFalse && !keyname.CompareI("PRIMARY"))
			isprimary = wyTrue;

		if(isprimary == wyTrue)
		{
			if(!keyname.CompareI("PRIMARY"))
			{
				CustomGrid_SetText(m_hwndgrid, tgtrow, 1, colname.GetString());
				tgtrow = tgtrow + 1;
			}	
		}

		if(isprimary == wyFalse)
		{
			if(!uniquekeymame.GetLength())
				uniquekeymame.SetAs(keyname);

			if(!myrowstr.CompareI("0") && !keyname.CompareI(uniquekeymame))
			{
				uniqueinfo = new CUniqueIndexElem(colname);

				if(!uniqueinfo)
					return wyFalse;

				uniquelist.Insert(uniqueinfo);				
			}
		}
	}
			
	if(myres)
		m_tunnel->mysql_free_result(myres);
			
	if(tgtrow > 0)
	{
		/*if list is created any way make sure it's destroying.
		Becase Its not documented in Mysql that the reultant query will always shows P-keys in first row*/
		uniqueinfo = (CUniqueIndexElem*)uniquelist.GetFirst();
		while(uniqueinfo)
		{			
			tempindex = (CUniqueIndexElem*)uniqueinfo->m_next;

			uniquelist.Remove(uniqueinfo);
			delete uniqueinfo;

			uniqueinfo = tempindex;
		}
		return wyTrue;
	}
	
	//If there is no primary key in table consider the unique index(s)
	if(isprimary == wyFalse && uniquelist.GetCount())
	{		
		uniqueinfo = (CUniqueIndexElem*)uniquelist.GetFirst();
		while(uniqueinfo)
		{
			CustomGrid_SetText(m_hwndgrid, tgtrow, 1, uniqueinfo->m_indexname.GetString());

			tempindex = (CUniqueIndexElem*)uniqueinfo->m_next;

			uniquelist.Remove(uniqueinfo);
			delete uniqueinfo;

            tgtrow = tgtrow + 1;
			uniqueinfo = tempindex;
		}
	}	
	return wyTrue;
}

//Fills the Target columns with values for 'Edit Relationship"
wyBool		
CCreateRel::DestColumnsFillDefaultValues()
{
	wyInt32			tgtrow = 0;
	wyString		constraintname;
	RelTableFldInfo *tgtfldinfo = NULL, *tempinfo = NULL;

	if(m_iseditrelationship == wyFalse || !m_tgetfieldslist || !m_tgetfieldslist->GetFirst())
		return wyFalse;

	tgtfldinfo = (RelTableFldInfo*)m_tgetfieldslist->GetFirst();

	while(tgtfldinfo)
	{
		tempinfo = (RelTableFldInfo*)tgtfldinfo->m_next;

		CustomGrid_SetText(m_hwndgrid, tgtrow, 1, tgtfldinfo->m_tablefld.GetString());
		tgtrow = tgtrow + 1;

		m_tgetfieldslist->Remove(tgtfldinfo);
		delete tgtfldinfo;

		tgtfldinfo = tempinfo;
	}	

	//Sets the Constraint Name
	if(m_relfldparam->constraintname && m_relfldparam->constraintname->GetLength())
	{
		constraintname.SetAs(*m_relfldparam->constraintname);
		//removes the back ticks
		constraintname.Erase(constraintname.GetLength() - 1, 1);
		constraintname.Erase(0, 1);	

		SendMessage(GetDlgItem(m_hwnd, IDC_FKNAME), WM_SETTEXT, SIZE_512 - 1, (LPARAM)constraintname.GetAsWideChar());
	}

	return wyTrue;
}

//For Edit Relationship: it checks whether the F-key is edited or not
wyBool	
CCreateRel::IsForeignKeyEdited()
{
	wyInt32		cursel, cmp;
	wyWChar	    table[SIZE_512] = {0}, fkname[SIZE_512] = {0};
	wyBool		iskeychanged = wyFalse;
	HWND		hwndCombo;
	wyString	tablestr, fkeystr, referencedtable, constraint, message;
	fkeyOption	ondel, onupdate;
	wyBool		isoption;

	//If the grid is edited
	if(m_isfkeychanged == wyTrue)
		iskeychanged = wyTrue;
	
	constraint.SetAs(*m_relfldparam->constraintname);
	
	//removes the backtick
	constraint.Erase(constraint.GetLength() - 1, 1);
	constraint.Erase(0, 1);

	referencedtable.SetAs(*m_relfldparam->parenttable);

	VERIFY(hwndCombo = GetDlgItem(m_hwnd, IDC_TABLELIST));
	
	//Gets the referenced table from combo
	VERIFY((cursel = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0))!= CB_ERR);
	VERIFY((SendMessage(hwndCombo, CB_GETLBTEXT, cursel, (LPARAM)table)) != CB_ERR);
	tablestr.SetAs(table);
		
	cmp = tablestr.Compare(referencedtable);
	
	//if the referenced table is changed
	if(cmp)
		iskeychanged = wyTrue;
	
	//Gets the constraint name from text box
	SendMessage(GetDlgItem(m_hwnd, IDC_FKNAME), WM_GETTEXT, SIZE_512 - 1, (LPARAM)fkname);
	fkeystr.SetAs(fkname);
	
	// ON DELETE Option
	//Gets the 'On Delete' check box state
	if(SendMessage(GetDlgItem(m_hwnd, IDC_ONDELETE), BM_GETCHECK, 0, 0)== BST_CHECKED)
		isoption = wyTrue;

	else
		isoption = wyFalse;

	//Check whether the ON DELETE / ON UPDATE option is edited?
	ondel = m_relfldparam->ondelete;
	switch(ondel)
	{
	case NOOPTION:
		if(isoption == wyTrue)
			iskeychanged = wyTrue;
		break;

	case CASCADE:
		if((SendMessage(GetDlgItem(m_hwnd, IDC_ONDELCASCADE), BM_GETCHECK, 0, 0) == BST_UNCHECKED) || isoption == wyFalse)
			iskeychanged = wyTrue;
		break;

	case SETNULL:
		if((SendMessage(GetDlgItem(m_hwnd, IDC_ONDELNULL), BM_GETCHECK, 0, 0) == BST_UNCHECKED) || isoption == wyFalse)
			iskeychanged = wyTrue;
		break;

	case RESTRICT:
		if((SendMessage(GetDlgItem(m_hwnd, IDC_ONDELRESTRICT), BM_GETCHECK, 0, 0) == BST_UNCHECKED) || isoption == wyFalse)
			iskeychanged = wyTrue;
		break;

	case NOACTION:
		if((SendMessage(GetDlgItem(m_hwnd, IDC_ONDELNOACTION), BM_GETCHECK, 0, 0) == BST_UNCHECKED) || isoption == wyFalse)
			iskeychanged = wyTrue;
		break;
	}

	// ON UPDATE Option	
	if(SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATE), BM_GETCHECK, 0, 0)== BST_CHECKED)
		isoption = wyTrue;

	else
		isoption = wyFalse;

	//Check whether the ON UPDATE option is edited?
	onupdate = m_relfldparam->onupdate;
	switch(onupdate)
	{
	case NOOPTION:
		if(isoption == wyTrue)
			iskeychanged = wyTrue;
		break;

	case CASCADE:
		if((SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATECASCADE), BM_GETCHECK, 0, 0) == BST_UNCHECKED ) || isoption == wyFalse)
			iskeychanged = wyTrue;
		break;

	case SETNULL:
		if((SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATENULL), BM_GETCHECK, 0, 0) == BST_UNCHECKED) || isoption == wyFalse)
			iskeychanged = wyTrue;
		break;

	case RESTRICT:
		if((SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATERESTRICT), BM_GETCHECK, 0, 0) == BST_UNCHECKED) || isoption == wyFalse)
			iskeychanged = wyTrue;
		break;

	case NOACTION:
		if((SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATENOACTION), BM_GETCHECK, 0, 0) == BST_UNCHECKED) || isoption == wyFalse)
			iskeychanged = wyTrue;
		break;
	}	
	
	//If no change other than '1' concatenates into F-key name ,discard to create new key
	if(iskeychanged == wyFalse)
	{
		cmp = constraint.Compare(fkeystr);
		if(cmp)
			iskeychanged = wyTrue;
	}
    	
	if(iskeychanged == wyTrue)
		return wyTrue;
	    	
	return wyFalse;  
}

/*For Edit Relationship: it drops the original F-key first , and trying to create a new one with edited infos.
This will be happen only if the user doesnt change the F-key name . If
the F-keys unique and could be executed the Drop old one & Create new one in a single mySql query.
Here since the both names are same we can't do both operation in a single query.
So executing 2 different queries, Chances are there Drop may successful and Create may not. So we will loose the original F-key
*/
wyBool	
CCreateRel::DropAndCreateNewForeignKey(wyString *query, wyString *fknamestr)
{
	wyInt32		uid, pos, len;
	wyString	temp, tempbuff;
	wyString	mbmssg, dropquery, dropstmt;
	MYSQL_RES   *res;
	
	uid = MessageBox(m_hwnd,  EDITRELERROR, pGlobals->m_appname.GetAsWideChar(), MB_ICONWARNING | MB_YESNO);
	switch(uid)
	{
	case IDYES:
		//Dropping the original one
		dropstmt.AddSprintf("drop foreign key %s", m_relfldparam->constraintname->GetString());	
		dropquery.AddSprintf("alter table `%s`.`%s` %s", m_db.GetString(), m_table.GetString(), dropstmt.GetString());

		if(m_isfkeylost == wyFalse)
		{
			res = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, dropquery);
			if(!res && m_tunnel->mysql_affected_rows(*m_mysql)== -1)
			{
				ShowMySQLError(m_hwndgrid, m_tunnel, m_mysql, query->GetString());
				yog_message(m_hwndgrid, RELATIONERROR, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION |MB_HELP);
				return wyFalse;
			}
			m_tunnel->mysql_free_result(res);
		}
		
		// Flag sets while lost the original Fkey & not created new one 
		m_isfkeylost = wyFalse;

		//Creating the new one with edited infos.
		res = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, *query);
		if(!res && m_tunnel->mysql_affected_rows(*m_mysql)== -1)
		{
			ShowMySQLError(m_hwndgrid, m_tunnel, m_mysql, query->GetString());
			yog_message(m_hwndgrid, RELATIONERROR, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION | MB_HELP);

			// Flag sets while lost the original Fkey & not created new one 
			m_isfkeylost = wyTrue;
						
			return wyFalse;
		}
		m_tunnel->mysql_free_result(res);
		break;

	case IDNO:
		return wyFalse;

	default:
		return wyFalse;
	}	
	
	//Sets the Foreign key infos- needs to display into "Manage Relationship" dlg box. and sets into the structure
	if(m_iseditrelationship == wyTrue && m_relfldparam)
	{		
		tempbuff.SetAs(*query);
		temp.AddSprintf("alter table `%s`.`%s` add constraint ", m_db.GetString(), m_table.GetString());
		pos = tempbuff.Find(temp.GetString(), 0);
		if(pos >= 0)
		{
			len = temp.GetLength();
			tempbuff.Erase(pos, len - 1);
		}

		m_relfldparam->fkeyinfos->SetAs(tempbuff);
		return wyTrue;
	}

	return wyFalse;
}

// Function to create a new index.
wyBool
CCreateRel::CreateNewIndex()
{
	HWND	    hwndcombo;
	MYSQL_RES   *res;
	wyBool	    isvalid, retval;
    wyUInt32	rowcount, len, cmp, cursel, pos;
    wyWChar	    table[SIZE_512] = {0}, col[SIZE_512] = {0}, fkname[SIZE_512] = {0};
	wyString	colstr, fknamestr, fknamebuf, tablestr, tempbuff;
	wyString	dropstmt, message, query, temp;	

	VERIFY(hwndcombo = GetDlgItem(m_hwnd, IDC_TABLELIST));
	
	cursel = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0);
	
	if(cursel == CB_ERR)
    {
		yog_message(m_hwndgrid, _(L"Please select a reference table"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_HELP | MB_ICONINFORMATION);
		return wyFalse;
	}

	CustomGrid_ApplyChanges(m_hwndgrid);
	VERIFY(SendMessage(hwndcombo, CB_GETLBTEXT, cursel, (LPARAM)table));
	tablestr.SetAs(table);

	isvalid = IsAnyColSel();

	if(!isvalid)
    {
		yog_message(m_hwndgrid, _(L"Please select a column to reference"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION | MB_HELP);
		return wyFalse;
	}

	isvalid = IsSelColTotalEqual();

	if(!isvalid)
    {
		yog_message(m_hwndgrid, _(L"Please select equal number of source and reference columns"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION | MB_HELP);
		return wyFalse;
	}

	SendMessage(GetDlgItem(m_hwnd, IDC_FKNAME), WM_GETTEXT, SIZE_512 - 1, (LPARAM)fkname);

	if(wcslen(fkname) == 0)
	{
		yog_message(m_hwnd, _(L"Give a valid foreign key name"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION | MB_HELP);
		SetFocus(GetDlgItem(m_hwnd, IDC_FKNAME));
		return wyFalse;
	}

	fknamestr.SetAs(fkname);
	query.Sprintf("alter table `%s`.`%s` add constraint `%s` FOREIGN KEY (", m_db.GetString(), m_table.GetString(), fknamestr.GetString());

	VERIFY(CustomGrid_GetRowCount(m_hwndgrid)== 100);

	for(rowcount = 0; rowcount < INITROWCOUNT; rowcount++)
	{
		len = CustomGrid_GetItemTextLength(m_hwndgrid, rowcount, 0);

		if(len)
        {
			CustomGrid_GetItemText(m_hwndgrid, rowcount, 0, col);
			colstr.SetAs(col);
			query.AddSprintf("`%s`,", colstr.GetString());

			//variable keeps the first field in the source column
			if(!m_firstsrcfld.GetLength())
				m_firstsrcfld.SetAs(col);
		}
	}
    query.Strip(1);
	query.AddSprintf( ") REFERENCES `%s` (", tablestr.GetString());

	for(rowcount = 0; rowcount < INITROWCOUNT; rowcount++)
    {
		len = CustomGrid_GetItemTextLength(m_hwndgrid, rowcount, 1);

		if(len)
        {
			CustomGrid_GetItemText(m_hwndgrid, rowcount, 1, col);
			colstr.SetAs(col);
			query.AddSprintf("`%s`,", colstr.GetString());

			//variable keeps the first field in the target column
			if(!m_firsttgtfld.GetLength())
				m_firsttgtfld.SetAs(col);
		}
	}
    query.Strip(1);
	query.Add(")");

    HandleOnDelete(query);
	HandleOnUpdate(query);

	/*For "Edit Relationship" : if any of the Fkey info is edited , then checks 
	 whether the user changes the F-key name back to original one ., if yes we will drop the original F-key and try to
	 create new one, But if creation of new one is failed we will loose the original one
	 */
	if(m_iseditrelationship == wyTrue && m_isfkeychanged == wyTrue && m_relfldparam->constraintname->GetLength() && m_isfkeylost == wyFalse)
	{
		fknamebuf.AddSprintf("`%s`", fknamestr.GetString());
		
		//If incase user changed the F-key name back to the original one
		cmp = m_relfldparam->constraintname->Compare(fknamebuf);
		if(!cmp)
		{
			retval = DropAndCreateNewForeignKey(&query, &fknamestr);
			return retval;
		}		
	}

	//Keeps the new F-key for displayin into Manage Relationship dlg box.
	tempbuff.SetAs(query);   

	/*For "Edit Relationship" : Drop the old F-key and creating the edited one trying to execute in single query. 
	If it execute successfully need to update F-key displays into Relationship Management dialog box.
	If the Original F-key is dropped once then can avooid DROP F-key query
	*/
	if(m_iseditrelationship == wyTrue && m_relfldparam && m_relfldparam->iseditfkey == wyTrue && m_isfkeylost == wyFalse)
	{		
		dropstmt.AddSprintf("drop foreign key %s", m_relfldparam->constraintname->GetString());	
		
		// Create & Drop F-key in single query.
		query.AddSprintf(", %s", dropstmt.GetString());		
	}

	res = ExecuteAndGetResult(GetActiveWin(), m_tunnel, m_mysql, query);
	if(!res && m_tunnel->mysql_affected_rows(*m_mysql)== -1)
	{
		ShowMySQLError(m_hwndgrid, m_tunnel, m_mysql, query.GetString());
		yog_message(m_hwndgrid, RELATIONERROR, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION | MB_HELP);
		return wyFalse;
	}

	m_tunnel->mysql_free_result(res);

	if(fknamestr.GetLength())
		m_fknamestr.SetAs(fknamestr);

	//Sets the strct varible wwioth F-key details nedd to display in the "Manage Relationship" dialog box
	if(m_iseditrelationship == wyTrue && m_relfldparam)
	{		
		temp.AddSprintf("alter table `%s`.`%s` add constraint ", m_db.GetString(), m_table.GetString());
		pos = tempbuff.Find(temp.GetString(), 0);
		if(pos >= 0)
		{
			len = temp.GetLength();
			tempbuff.Erase(pos, len - 1);
		}
		m_relfldparam->fkeyinfos->SetAs(tempbuff);
		m_isfkeylost = wyFalse;		
	}
	return wyTrue;
}

wyBool 
CCreateRel::HandleOnDelete(wyString &query)
{
    if(SendMessage(GetDlgItem(m_hwnd, IDC_ONDELETE), BM_GETCHECK, 0, 0)== BST_CHECKED)
    {
		if(SendMessage(GetDlgItem(m_hwnd, IDC_ONDELCASCADE), BM_GETCHECK, 0, 0)== BST_CHECKED)
			query.Add(" ON DELETE CASCADE ");
		else if(SendMessage(GetDlgItem(m_hwnd, IDC_ONDELNULL), BM_GETCHECK, 0, 0)== BST_CHECKED)
			query.Add(" ON DELETE SET NULL ");
		else if(SendMessage(GetDlgItem(m_hwnd, IDC_ONDELNOACTION), BM_GETCHECK, 0, 0)== BST_CHECKED)
			query.Add(" ON DELETE NO ACTION ");
		else 
			query.Add(" ON DELETE RESTRICT ");

        return wyTrue;
	}

    return wyFalse;
}

wyBool 
CCreateRel::HandleOnUpdate(wyString &query)
{
	if(SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATE), BM_GETCHECK, 0, 0)== BST_CHECKED)
    {
		if(SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATECASCADE), BM_GETCHECK, 0, 0)== BST_CHECKED)
			query.Add(" ON UPDATE CASCADE ");
		else if(SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATENULL), BM_GETCHECK, 0, 0)== BST_CHECKED)
			query.Add(" ON UPDATE SET NULL ");
		else if(SendMessage(GetDlgItem(m_hwnd, IDC_ONUPDATENOACTION), BM_GETCHECK, 0, 0)== BST_CHECKED)
			query.Add(" ON UPDATE NO ACTION ");
		else 
			query.Add(" ON UPDATE RESTRICT ");

        return wyTrue;
	}    

    return wyFalse;
}

wyBool
CCreateRel::GetIndexName()
{
	wyInt32		indexcount, len;
	wyWChar	    table[SIZE_512] = {0};
	wyString	tablestr;


	for(indexcount = 0; indexcount < INITROWCOUNT; indexcount++)
    {
		len = CustomGrid_GetItemTextLength(m_hwndgrid, indexcount, 0);

		if(len)
        {
			CustomGrid_GetItemText(m_hwndgrid, indexcount, 0, table);
			
			tablestr.SetAs(table);

			m_neweditindexname.AddSprintf("`%s` ", tablestr.GetString());
		}
	}

	m_neweditindexname.Strip(1);
	return wyTrue;
}

wyBool
CCreateRel::IsSelColTotalEqual()
{
	wyInt32  rowcount, len1, len2;

	for(rowcount = 0; rowcount < INITROWCOUNT; rowcount++)
    {
		len1 = CustomGrid_GetItemTextLength(m_hwndgrid, rowcount, 0);		
		len2 = CustomGrid_GetItemTextLength(m_hwndgrid, rowcount, 1);		

		if((len1 && !len2)|| (!len1 && len2))
			return wyFalse;

	}
	
	return wyTrue;
}

wyBool
CCreateRel::IsAnyColSel()
{
	wyInt32		rowcount, len;

	for(rowcount = 0; rowcount < INITROWCOUNT; rowcount++)
    {
		len = CustomGrid_GetItemTextLength(m_hwndgrid, rowcount, 0);		

		if(len)
			return wyTrue;
	}
	
	return wyFalse;
}


wyBool
CCreateRel::ShowKeyIndex()
{
	IndexManager	ciu;

	ciu.Create(m_hwnd, m_tunnel, m_mysql, (wyChar*)m_db.GetString(), (wyChar*)m_table.GetString());

	return wyTrue;
}

wyBool
CCreateRel::ShowRefIndex()
{
	IndexManager	ciu;
	wyInt32			cursel;
	wyWChar			table[SIZE_512] = {0};
	wyString		tablestr;
	
	cursel = SendMessage(GetDlgItem(m_hwnd, IDC_TABLELIST), CB_GETCURSEL, 0, 0);

	if(cursel == CB_ERR)
    {
		yog_message(m_hwnd, _(L"Please select a reference table"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION | MB_HELP);
		return wyFalse;
	}

	VERIFY(SendMessage(GetDlgItem(m_hwnd, IDC_TABLELIST), CB_GETLBTEXT, cursel, (LPARAM)table));
	
	tablestr.SetAs(table);

	ciu.Create(m_hwnd, m_tunnel, m_mysql, m_db.GetString(), tablestr.GetString());

	return wyTrue;

}