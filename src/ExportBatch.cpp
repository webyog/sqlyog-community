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

#include <assert.h>
#include "FrameWindowHelper.h"
#include "MySQLVersionHelper.h"
#include "CommonHelper.h"
#include "ExportBatch.h"
#include "ExportAsSQL.h"
#include "SQLMaker.h"
#include "GUIHelper.h"
//#include "SyncStructure.h"

extern	PGLOBALS		 pGlobals;

#define      TABLE         0
#define      VIEW          1
#define      PROCEDURE     2
#define      FUNCTION      3
#define      TRIGGER       4
#define      EVENT         5

#define      NODBSEL	  _(L"No database selected. Please select a database from the dropdown.")

#define	WRITEEXPORTMESSAGE(a,b,c,d)		{ sprintf(a,b,c);SetWindowText(d,a);UpdateWindow(d); }

#define	EXPORTJOB   "ExportBatch"

ExportBatch::ExportBatch()
{
	m_exporting      = wyFalse;
	m_stopexport     = wyFalse;
	m_alltables      = wyTrue;
	m_allviews       = wyTrue;
	m_allprocedures  = wyTrue;
	m_allfunctions   = wyTrue;	
	m_dump_table     = wyFalse;
	m_p              = new Persist;
	m_dumptablecnt   = 0;
    m_dump_all_tables = wyFalse;
}

ExportBatch::~ExportBatch()
{	
}

wyBool
ExportBatch::Create(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, TUNNELAUTH * auth, const wyChar *db, const wyChar *table)
{
	wyInt64     ret;
	m_mysql     = mysql;
	m_tunnel    = tunnel;
	m_auth      = auth;
	m_hwndparent = hwnd;
	
	m_db.SetAs(db);
    
    //if table is not null then only set m_table
    if(table)
    {
	    m_table.SetAs(table);
        
        if(m_table.GetLength() != 0)
		    m_dump_table = wyTrue;
    }
    //else set the flag to check all the tables
    else
        m_dump_all_tables = wyTrue;

	m_p->Create("ExportData");

	//Post 8.01
	//RepaintTabModule();

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_EXPORTDATA),
							hwnd, ExportBatch::ExpDataDlgProc,(LPARAM)this);
	if(ret)
        return wyTrue;
   
    return wyFalse;
}


// Callback dilog proc to handle export data dialog box.

INT_PTR CALLBACK	
ExportBatch::ExpDataDlgProc	(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPNMHDR        lpnm = (LPNMHDR)lParam;
	LPNMTVKEYDOWN   ptvkd = (LPNMTVKEYDOWN)lParam ;
	ExportBatch    *pexp = (ExportBatch*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	DlgControl* pdlgctrl;
	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
        LocalizeWindow(hwnd);
		pexp = (ExportBatch*)lParam;
		pexp->CreateImageList(hwnd);
        pexp->HandleDlgPersistance(hwnd);
		pexp->OnWmInitDialog(hwnd, lParam);
		break;

	case WM_DESTROY:
		//delete the dialog control list
		while((pdlgctrl = (DlgControl*)pexp->m_controllist.GetFirst()))
		{
			pexp->m_controllist.Remove(pdlgctrl);
			delete pdlgctrl;
		}
		SaveInitPos(hwnd, SQLDUMP_SECTION);
		delete pexp->m_p;
		break;

	case WM_SIZE:
		pexp->ExpBatResize(hwnd);
		break;

	case WM_PAINT:
		//draws the resize gripper at bottom-right corner of dialog
		pexp->OnPaint(hwnd);
        return 1;
		break;

	case WM_ERASEBKGND:
		//blocked for double buffering
		return 1;
		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/116-backup-database-as-sql-dump");
		return TRUE;

	case WM_MOUSEMOVE:
        pexp->OnMouseMove(hwnd, lParam);
		break;

	case WM_COMMAND:
		pexp->OnWmCommand(hwnd, wParam);
		break;
	case WM_NOTIFY:
		{
			TVHITTESTINFO ht = {0};
			if(lpnm->idFrom == IDC_EXPORTTREE && lpnm->code == NM_CLICK)
			{
				HandleTreeViewItem(lParam, wParam);
				HWND	hwndexpmode;
				wyInt32	chkstate;
	
				VERIFY(hwndexpmode = GetDlgItem(hwnd, IDC_CHK_DATAONLY));
				chkstate = SendMessage(hwndexpmode, BM_GETCHECK, 0, 0);
				if(chkstate == BST_CHECKED)
				{
					DWORD dwpos = GetMessagePos();
					ht.pt.x = GET_X_LPARAM(dwpos);
					ht.pt.y = GET_Y_LPARAM(dwpos);
					MapWindowPoints(HWND_DESKTOP, lpnm->hwndFrom, &ht.pt, 1);

					TreeView_HitTest(lpnm->hwndFrom, &ht);
         
					if(TVHT_ONITEMSTATEICON & ht.flags)
					{
						
						PostMessage(hwnd, UM_CHECKSTATECHANGE, 0, (LPARAM)ht.hItem);
					}
				}
			}	

			if((ptvkd->hdr.idFrom == IDC_EXPORTTREE) && ptvkd->wVKey == VK_SPACE)
				{
					HandleTreeViewItem(lParam, wParam, wyTrue);
					HWND	hwndexpmode;
					wyInt32	chkstate;
	
					VERIFY(hwndexpmode = GetDlgItem(hwnd, IDC_CHK_DATAONLY));
					chkstate = SendMessage(hwndexpmode, BM_GETCHECK, 0, 0);
					if(chkstate == BST_CHECKED)
					{
						HWND		hwndtree;
						VERIFY(hwndtree = GetDlgItem(hwnd, IDC_EXPORTTREE));
						HTREEITEM hitem = TreeView_GetSelection(hwndtree);
						PostMessage(hwnd, UM_CHECKSTATECHANGE, 0, (LPARAM)hitem);		
					}
				}
			if(lpnm->code == TVN_ITEMEXPANDING)
			{
				HWND hwndexpmode;
				VERIFY(hwndexpmode = GetDlgItem(hwnd, IDC_CHK_DATAONLY));
				wyInt32 chk_data = SendMessage(hwndexpmode, BM_GETCHECK, 0, 0);
				return pexp->OnItemExpandingHelper(lpnm->hwndFrom, (LPNMTREEVIEW)lParam, chk_data); 
			}

		}
		break;
	case UM_CHECKSTATECHANGE:
		{
						HWND		hwndtree;
						wyWChar		temptext[SIZE_512] = {0};
						TVITEM		tvi = {0};

						VERIFY(hwndtree = GetDlgItem(hwnd, IDC_EXPORTTREE));
						HTREEITEM   htreeitem = (HTREEITEM)lParam;
						HTREEITEM	hrootitem = TreeView_GetParent(hwndtree,htreeitem);
						HTREEITEM	hchilditem;
						if(hrootitem != NULL)
						{
							htreeitem = hrootitem;
						}
						tvi.mask		= TVIF_HANDLE |TVIF_TEXT ;
						tvi.hItem		= htreeitem;
						tvi.pszText		= temptext;
						tvi.cchTextMax	= 512 - 1;
						
						VERIFY(TreeView_GetItem(hwndtree, &tvi));

						if(wcsicmp(tvi.pszText,TXT_TABLES) != 0)
						{
							for(hchilditem = TreeView_GetChild(hwndtree, htreeitem); hchilditem; hchilditem = TreeView_GetNextSibling(hwndtree, hchilditem))
							{
								TreeView_SetCheckState(hwndtree, hchilditem, 0);
							}
							TreeView_SetCheckState(hwndtree, htreeitem, 0);
							yog_message(hwnd, _(L"Only table(s) have data. You need to select only table(s) for dumping - Data Only"), 
							pGlobals->m_appname.GetAsWideChar(), MB_ICONINFORMATION | MB_OK);
							return TRUE;							
						}
		}
		break;

	case WM_GETMINMAXINFO:
		pexp->OnWMSizeInfo(lParam);
		break;

	default:
		break;
		
	}
	return 0;
}

void 
ExportBatch::OnWmInitDialog(HWND hwnd, LPARAM lparam)
{
    m_hwnd = hwnd;
	m_expflg = wyFalse;
	m_expchange = wyFalse;
	InitializeExpData();
	//keep cursor in waiting state untill objects are adding to the treeview.
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	IntializeTree(hwnd, m_db.GetString());
	SetCursor(LoadCursor(NULL, IDC_ARROW));
		

	GetClientRect(m_hwnd, &m_dlgrect);
	GetWindowRect(m_hwnd, &m_wndrect);

	//set the initial position of the dialog
	SetWindowPositionFromINI(m_hwnd, SQLDUMP_SECTION, 
		m_wndrect.right - m_wndrect.left, 
		m_wndrect.bottom - m_wndrect.top);
	
	GetCtrlRects();
	PositionCtrls();
#ifdef COMMUNITY
                EnableWindow(GetDlgItem(m_hwnd, IDC_FILE_TABLE), FALSE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_TIMESTAMP_PREFIX), FALSE);

#endif



}

wyBool 
ExportBatch::InitializeExpData()
{
	wyInt32			ret, i = 0;
	LVITEM			lvi;
	HTREEITEM		hitem;
	MDIWindow		*pcquerywnd = NULL;
	TVITEM			tvi = {0};
	wyWChar         seldb[SIZE_512+1] = {0};
	HWND			hwnddb;
	wyInt32			index = 0;
	// we now add all the database which are there in the Object Browser.
	VERIFY(pcquerywnd = GetActiveWin());

	// we first get the handle of all the root.
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	VERIFY(hitem = TreeView_GetRoot(pcquerywnd->m_pcqueryobject->m_hwnd));
	VERIFY( hwnddb = GetDlgItem(m_hwnd, IDC_EXPORT_DBCOMBO));

	for(hitem = TreeView_GetChild(pcquerywnd->m_pcqueryobject->m_hwnd, hitem); hitem; hitem = TreeView_GetNextSibling(pcquerywnd->m_pcqueryobject->m_hwnd, hitem))
	{
		tvi.mask	    = TVIF_TEXT;
		tvi.pszText     = seldb;
		tvi.cchTextMax  = SIZE_512;
		tvi.hItem       = hitem;

		VERIFY(TreeView_GetItem(pcquerywnd->m_pcqueryobject->m_hwnd, &tvi));

		lvi.mask		=	LVIF_TEXT;
		lvi.iItem		=	i++;
		lvi.iSubItem	=	0;
		lvi.pszText		=	(LPWSTR)seldb;
		lvi.cchTextMax	=	wcslen(seldb);

		VERIFY(ret =SendMessage(hwnddb, CB_ADDSTRING, 0,(LPARAM)seldb));
	}
	index = SendMessage(hwnddb, CB_FINDSTRING, -1,(LPARAM)m_db.GetAsWideChar());
	SendMessage(hwnddb, CB_SETCURSEL, index, 0);
	SetFocus(hwnddb);

	EnableDisableOptions(IDC_CHK_DATAONLY);
	EnableDisableOptions(IDC_CHK_STRUCTURE);
	EnableDisableOptions(IDC_CHK_DATA);	
	
	if(IsMySQL402(m_tunnel, m_mysql) == wyFalse)
	{
		EnableWindow(GetDlgItem(m_hwnd, IDC_SINGLE_TRANSACTION), FALSE);
		SendMessage(GetDlgItem(m_hwnd, IDC_SINGLE_TRANSACTION), BM_SETCHECK, 0,0);
	}
	else
		EnableWindow(GetDlgItem(m_hwnd, IDC_SINGLE_TRANSACTION), TRUE);
	
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return wyTrue;
}

void
ExportBatch::EnableDisableOptions(wyInt32 id)
{
	HWND	hwndexpmode;
	wyInt32	state;
	
	switch(id)
	{
	case IDC_CHK_DATAONLY:
		{
			VERIFY(hwndexpmode = GetDlgItem(m_hwnd, IDC_CHK_DATAONLY));
			state = SendMessage(hwndexpmode, BM_GETCHECK, 0, 0);
			if(state == BST_CHECKED)
			{
				wyWChar		temptext[SIZE_512] = {0};
				TVITEM		tvi = {0};
				HWND hwndtree;
				HTREEITEM		htreeparent;
				VERIFY(hwndtree = GetDlgItem(m_hwnd, IDC_EXPORTTREE));
				htreeparent = TreeView_GetRoot(hwndtree);
				for(; htreeparent; htreeparent = TreeView_GetNextSibling(hwndtree, htreeparent))
				{
					tvi.mask		= TVIF_TEXT;
					tvi.hItem		= htreeparent;
					tvi.pszText		= temptext;
					tvi.cchTextMax	= 512 - 1;
					VERIFY(TreeView_GetItem(hwndtree, &tvi));
					if(wcsicmp(temptext, TXT_TABLES)!= 0)
					{
						TreeView_SetCheckState(hwndtree, htreeparent, 0);
						for(HTREEITEM hchilditem = TreeView_GetChild(hwndtree, htreeparent); hchilditem; hchilditem = TreeView_GetNextSibling(hwndtree, hchilditem))
						{
							TreeView_SetCheckState(hwndtree, hchilditem, 0);
						}
					}
				}
				
				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_CREATEDB), FALSE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_DROPOBJECT), FALSE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_DEFINER), FALSE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_HEXBLOB), TRUE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_BULKINSERT), TRUE);
				
				//If Bulk Insert is Checked then enable "One Row Per line option"
				if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd, IDC_BULKINSERT), BM_GETCHECK, 0, 0))
					EnableWindow(GetDlgItem(m_hwnd, IDC_ONEROW), TRUE);
				else
					EnableWindow(GetDlgItem(m_hwnd, IDC_ONEROW), FALSE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_LOCKAROUNDINSERT), TRUE);
			}
			else
			{
				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_CREATEDB), TRUE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_DROPOBJECT), TRUE);
			}
		}
		break;

	case IDC_CHK_STRUCTURE:
		{
			VERIFY(hwndexpmode = GetDlgItem(m_hwnd, IDC_CHK_STRUCTURE));
			state = SendMessage(hwndexpmode, BM_GETCHECK, 0, 0);
			if(state == BST_CHECKED)
			{
				EnableWindow(GetDlgItem(m_hwnd, IDC_ONEROW), FALSE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_LOCKAROUNDINSERT), FALSE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_BULKINSERT), FALSE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_CREATEDB), TRUE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_DROPOBJECT), TRUE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_DEFINER), TRUE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_HEXBLOB), FALSE);
				
			}
			else
			{
				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_LOCKAROUNDINSERT), TRUE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_BULKINSERT), TRUE);
				//If Bulk Insert is Checked then enable "One Row Per line option"
				if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd,IDC_BULKINSERT), BM_GETCHECK, 0, 0))
					EnableWindow(GetDlgItem(m_hwnd, IDC_ONEROW), TRUE);
				else
					EnableWindow(GetDlgItem(m_hwnd, IDC_ONEROW), FALSE);
			}
		}
		break;

	case IDC_CHK_DATA:
		{
			VERIFY(hwndexpmode = GetDlgItem(m_hwnd, IDC_CHK_DATA));
			state = SendMessage(hwndexpmode, BM_GETCHECK, 0, 0);
			if(state == BST_CHECKED)
			{
				EnableWindow(GetDlgItem(m_hwnd, IDC_BULKINSERT), TRUE);

				//If Bulk Insert is Checked then enable "One Row Per line option"
				if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd,IDC_BULKINSERT), BM_GETCHECK, 0, 0))
					EnableWindow(GetDlgItem(m_hwnd, IDC_ONEROW), TRUE);
				else
					EnableWindow(GetDlgItem(m_hwnd, IDC_ONEROW), FALSE);

				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_CREATEDB), TRUE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_DROPOBJECT), TRUE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_LOCKAROUNDINSERT), TRUE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_DEFINER), TRUE);
				EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_HEXBLOB), TRUE);
			}
		}
		break;
	}
}

void 
ExportBatch::OnWmCommand(HWND hwnd, WPARAM wparam)
{
	HWND		hwndcombo = NULL, hwndfocus = NULL;
	wyInt32		width = 0;
	wyString	dbname;
	ExportBatch *pexp = (ExportBatch*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	
    switch LOWORD(wparam)
	{
	
	case IDC_EXPFILESELECT:
		pexp->SetExpFileName( hwnd);
		break;
	case IDC_FILE_TABLE:
		pexp->IsDirectory( hwnd);
		break;

	case IDC_EXPORT_DBCOMBO:
	{
		if((HIWORD(wparam))== CBN_SELENDOK || HIWORD(wparam) == CBN_SELENDCANCEL)
		{
            hwndfocus = GetFocus();
            hwndcombo = GetDlgItem(m_hwnd, IDC_EXPORT_DBCOMBO);
            if(hwndfocus == hwndcombo)
            {
			    SetCursor(LoadCursor(NULL, IDC_WAIT));
			    pexp->SelectDatabaseName(hwnd);
			    SetCursor(LoadCursor(NULL, IDC_ARROW));
            }
		}

		if((HIWORD(wparam)) == CBN_DROPDOWN)
		{
			VERIFY(hwndcombo = GetDlgItem(m_hwnd, IDC_EXPORT_DBCOMBO));
			if(!hwndcombo)
				return;

			width = SetComboWidth(hwndcombo);
			SendMessage(hwndcombo, CB_SETDROPPEDWIDTH, width + 50, 0);
		}
	}
		break;	
	case IDOK:
		{
			ExportData();
			if( GetForegroundWindow() != hwnd)
				FlashWindow(pGlobals->m_pcmainwin->m_hwndmain, TRUE);
			EnableDisableOptions(IDC_CHK_DATAONLY);
			EnableDisableOptions(IDC_CHK_STRUCTURE);
			EnableDisableOptions(IDC_CHK_DATA);	
		}
		break;

	case IDDONE:
	case IDCANCEL:
		yog_enddialog(hwnd, 0);
		break;

	case IDC_CHK_STRUCTURE:
	case IDC_CHK_DATAONLY:
	case IDC_CHK_DATA:
		EnableDisableOptions(LOWORD(wparam));
		break;

	case IDC_BULKINSERT:
		EnableDisableOneRowPerLine();
		break;

	}	
    return;
}
void 
ExportBatch::EnableDisableOneRowPerLine()
{

	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd, IDC_BULKINSERT), BM_GETCHECK, 0, 0))
		EnableWindow(GetDlgItem(m_hwnd, IDC_ONEROW), TRUE);
	else
		EnableWindow(GetDlgItem(m_hwnd, IDC_ONEROW), FALSE);

}
void 
ExportBatch::OnMouseMove(HWND hwnd, LPARAM lparam)
{
    RECT    rect;
    POINT   pt;

    if(m_exporting)
	{
		pt.x = GET_X_LPARAM(lparam);
		pt.y = GET_Y_LPARAM(lparam);
		
		GetWindowRect(GetDlgItem(hwnd, IDOK), &rect);
		MapWindowPoints(hwnd, NULL, &pt, 1); 			

		if(PtInRect(&rect, pt) && IsExporting())
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		else
			SetCursor(LoadCursor(NULL, IDC_WAIT));
	}
    return;
}
void
ExportBatch::SetExpFileName(HWND hwnd)
{
	HWND			hwndfile;
	wyWChar			file[MAX_PATH + 1] = {0};
	OPENFILENAME	openfilename;

	VERIFY((hwndfile = GetDlgItem(hwnd, IDC_EXPORTFILENAME))!= NULL);

	memset(&openfilename, 0, sizeof(openfilename));

	openfilename.lStructSize        = OPENFILENAME_SIZE_VERSION_400;
	openfilename.hwndOwner          = hwnd;
	openfilename.hInstance          = pGlobals->m_pcmainwin->GetHinstance();
	openfilename.lpstrFilter        = L"SQL Files(*.sql)\0*.sql\0All Files(*.*)\0*.*\0";
	openfilename.lpstrCustomFilter =(LPWSTR)NULL;
	openfilename.nFilterIndex      = 1L;
	openfilename.lpstrFile         = file;
	openfilename.nMaxFile          = sizeof(file);
	openfilename.lpstrTitle        = _(L"Save As");
	openfilename.lpstrDefExt       = L"sql";
	openfilename.Flags             = OFN_HIDEREADONLY;
	openfilename.lpfnHook          =(LPOFNHOOKPROC)(FARPROC)NULL;
	openfilename.lpTemplateName    =(LPWSTR)NULL;
	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd,IDC_FILE_TABLE), BM_GETCHECK, 0, 0))
	{
		if(GetDirectoryFromUser(hwnd, file, L"Select a folder where you want to backup the database"))
    {
		SendMessage(hwndfile, WM_SETTEXT, 0,(LPARAM)file);
    	return ;
    }
	}
	else{

		if(GetSaveFileName(&openfilename))
		{
			SendMessage(hwndfile, WM_SETTEXT, 0,(LPARAM)file);
			return;
		}
	}

	return;
}
void
ExportBatch::SelectDatabaseName(HWND hwnd)
{
	
	MDIWindow	*pcquerywnda;
    wyInt32     length = 0, ncursel;
    wyWChar     *charsetname = NULL;
    wyString    dbname;
	HWND        hwndcombo, hwndtree;

	if(m_table.GetLength())
		m_table.Clear();

	pcquerywnda = GetActiveWin();     
	hwndcombo = GetDlgItem(hwnd, IDC_EXPORT_DBCOMBO);
	hwndtree = GetDlgItem(hwnd, IDC_EXPORTTREE);

	ncursel = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0);;
	length  = SendMessage(hwndcombo, CB_GETLBTEXTLEN, ncursel, NULL);
	PostMessage(hwndcombo, CB_SETCURSEL, ncursel, 0);
    charsetname    = AllocateBuffWChar(length + 1);
	SendMessage(hwndcombo, CB_GETLBTEXT,(WPARAM)ncursel,(LPARAM)charsetname);
	dbname.SetAs(charsetname);    
	m_db.SetAs(dbname.GetString());
    free(charsetname);
	VERIFY(TreeView_DeleteAllItems(hwndtree));
	IntializeTree(hwnd, dbname.GetString(), wyTrue);

}
 
//Function adds table(s), view(s), procedure(s), function(s) and trigger(s)of the selected database to the tree control.
wyBool
ExportBatch::IntializeTree(HWND hwnd, const wyChar * dbname, wyBool cbselchange)
{
	HWND			hwndtree, hobtree;
	MDIWindow		*wnd;
	TREEVIEWPARAMS	treeviewparam = {0};
	HTREEITEM		htreeparent, hti;
	wyInt32			state = 0, checkstate = 0;
    wyInt32         image;
    TVITEM          tviOB;
	wyWChar			str[70];

    VERIFY(wnd = GetActiveWin());
	if(!wnd)
		return wyFalse;
    
	m_dumptablecnt = 0;

	VERIFY(hwndtree = GetDlgItem(hwnd, IDC_EXPORTTREE));

	treeviewparam.database = NULL;
	treeviewparam.hwnd = hwndtree;
	treeviewparam.isopentables = wyFalse;
	treeviewparam.isrefresh = wyTrue;
	treeviewparam.mysql = m_mysql;
	treeviewparam.tunnel = m_tunnel;
	treeviewparam.tvi = NULL;
	treeviewparam.checkboxstate = wyFalse;

	if(m_table.GetLength() && cbselchange == wyFalse)
	{
		treeviewparam.issingletable = wyFalse;
		//OnItemExpanding(hwndtree, NULL, m_tunnel, *m_mysql, NULL, wyTrue);
		OnItemExpanding(treeviewparam);
        htreeparent = TreeView_GetRoot(hwndtree);
        TreeView_Expand(hwndtree, htreeparent, TVE_EXPAND);
		//ProcessSingleTable(m_hwnd, hwndtree, m_table.GetAsWideChar());		
	}
	
	else if(m_db.GetLength())
	{
		treeviewparam.issingletable = wyFalse;
		OnItemExpanding(treeviewparam);

		//Select all parent folderes by default
		htreeparent = TreeView_GetRoot(hwndtree);
		
        hobtree = wnd->m_pcqueryobject->m_hwnd;

        if(hobtree)
        {
            hti = TreeView_GetSelection(hobtree);
            image = wnd->m_pcqueryobject->GetSelectionImage();
            
			if(image == NSERVER)
			{
				hti = TreeView_GetChild(hobtree, hti);
				while(hti)
				{
					tviOB.hItem = hti;
					tviOB.pszText = str;
					tviOB.cchTextMax = 65;
					tviOB.mask = TVIF_TEXT;
					TreeView_GetItem(hobtree, &tviOB);
					if(!wcscmp(tviOB.pszText, m_db.GetAsWideChar()))
					{
						break;
					}
					hti = TreeView_GetNextSibling(hobtree, hti);
				}
			}
			else
			{
				while(image != NDATABASE)
				{
					hti = TreeView_GetParent(hobtree, hti);
					image = GetItemImage(hobtree, hti);
				}
			
            }
			hti = TreeView_GetChild(hobtree, hti);

			for(htreeparent; htreeparent; htreeparent = TreeView_GetNextSibling(hwndtree, htreeparent))
			{
				state = TreeView_GetItemState(hwndtree, htreeparent, TVIS_EXPANDEDONCE);
				checkstate = TreeView_GetCheckState(hwndtree, htreeparent);
				wyInt32 dataonly;

				dataonly = SendMessage(GetDlgItem(m_hwnd, IDC_CHK_DATAONLY), BM_GETCHECK, 0, 0)	;
				if(!checkstate && !(state & TVIS_EXPANDEDONCE))
					TreeView_SetCheckState(hwndtree, htreeparent, 1);
				if(dataonly == BST_CHECKED) 
				{
					break;
				}
				if(hti)
				{
					tviOB.hItem = hti;
					tviOB.mask = TVIF_PARAM;
					TreeView_GetItem(hobtree, &tviOB);
					if((tviOB.lParam && wcslen(((OBDltElementParam *)tviOB.lParam)->m_filterText)) || m_dump_all_tables)
					{
						TreeView_Expand(hwndtree, htreeparent, TVE_EXPAND);
					}
					//if only tables are selected, then we can safely break from loop since TABLES is the first root of the tree view
					hti = TreeView_GetNextSibling(hobtree, hti);
				}

				if(m_dump_all_tables == wyTrue)
				{   
					break;
				}
			}
			
        }
        
        TreeView_SelectItem(hwndtree, TreeView_GetRoot(hwndtree));
	}

	return wyTrue;
}

wyBool
ExportBatch::CreateImageList(HWND hwnd)
{
	wyInt32			ret, imagecount;
	HICON			hbmp;
	HWND			hwndtree;
	HIMAGELIST		m_himl;		
	wyInt32			imgres[] = {IDI_SERVER, IDI_DATABASE, IDI_OPEN, IDI_TABLE, IDI_COLUMN, 
								IDI_INDEX, IDI_OPEN,IDI_PROCEDURE, IDI_OPEN, IDI_FUNCTION,
								IDI_OPEN, IDI_VIEW, IDI_OPEN, IDI_TRIGGER, IDI_OPEN, IDI_EVENT, IDI_OPEN};

	VERIFY(m_himl = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK , 6, 0));
	hwndtree = GetDlgItem(hwnd, IDC_EXPORTTREE);

	for(imagecount = 0; imagecount < (sizeof(imgres)/sizeof(imgres[0])); imagecount++)
	{
		VERIFY(hbmp = (HICON)LoadImage(pGlobals->m_hinstance, 
                      MAKEINTRESOURCE(imgres[imagecount]), IMAGE_ICON, 
                      16, 16, LR_DEFAULTCOLOR));

		VERIFY((ret = ImageList_AddIcon(m_himl, hbmp))!= -1);
		VERIFY(DestroyIcon(hbmp));
	}

	TreeView_SetImageList(hwndtree, m_himl, TVSIL_NORMAL);
	
	return wyTrue;
}

//HTREEITEM 
//ExportBatch::addEntry(HWND htree, HTREEITEM parent, LPCTSTR text, wyInt32 icontype)
//{
//	TV_INSERTSTRUCT str;
//
//	str.hParent = parent;
//	str.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE	;
//	str.hInsertAfter = TVI_LAST;
//	str.item.iImage = icontype;
//	str.item.iSelectedImage = icontype;
//
//	str.item.pszText = (LPTSTR) text;
//
//	return ((HTREEITEM) SendMessage(htree, TVM_INSERTITEM, NULL, (long)&str));
//
//}

void
ExportBatch::HandleDlgPersistance(HWND hwnd)
{
	m_p->Add(hwnd, IDC_CHK_DATA, "Data", "1", CHECKBOX);
	m_p->Add(hwnd, IDC_CHK_STRUCTURE, "Structure", "0", CHECKBOX);
	m_p->Add(hwnd, IDC_CHK_DATAONLY, "DataOnly", "0", CHECKBOX);
	m_p->Add(hwnd, IDC_EXPORTFILENAME, "ExportFile", "", TEXTBOX);
	m_p->Add(hwnd, IDC_CHK_USEDBNAME, "UseDatabaseName", "1", CHECKBOX);
	m_p->Add(hwnd, IDC_CHK_CREATEDB, "CreateDatabase", "1", CHECKBOX);
	m_p->Add(hwnd, IDC_CHK_FLUSHLOGS, "FlushLogs", "0", CHECKBOX);
	m_p->Add(hwnd, IDC_SETUNIQUE, "ForeignChecks", "1", CHECKBOX);
	m_p->Add(hwnd, IDC_CHK_DROPOBJECT , "DropObject", "1", CHECKBOX);
	m_p->Add(hwnd, IDC_CHK_DEFINER , "RemoveDefiner", "0", CHECKBOX);
	m_p->Add(hwnd, IDC_CHK_LOCKFORREAD, "LocksForRead", "0", CHECKBOX);
	m_p->Add(hwnd, IDC_CHK_LOCKAROUNDINSERT, "LocksAroundInsert", "0", CHECKBOX);
	m_p->Add(hwnd, IDC_BULKINSERT, "CreateBulk", "1", CHECKBOX);
	m_p->Add(hwnd, IDC_ONEROW, "OneRowPerline", "1", CHECKBOX);
	m_p->Add(hwnd, IDC_SINGLE_TRANSACTION, "SingleTransaction", "0", CHECKBOX);
	m_p->Add(hwnd, IDC_CHK_VERSION, "Version", "1", CHECKBOX);
	m_p->Add(hwnd, IDC_CHK_HEXBLOB, "Hexblob", "0", CHECKBOX);
	return;
}

wyBool
ExportBatch::ExportData()
{
    if(m_exporting)
    {
		/* behave as if the stop has been pushed */
		StopExporting();
        EnableWindow(GetDlgItem(m_hwnd, IDOK), FALSE);
		SetFocus(m_hwnd);
    }
    else
    {
		VERIFY(wyFalse == m_exporting);

		if(!m_db.GetLength())
		{
			MessageBox(m_hwnd, NODBSEL, pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION); 
			return wyFalse;
		}

		SetCursor(LoadCursor(NULL, IDC_WAIT));
        		
		//Checks any object is selected to export.
		if(ValidateInput() == wyFalse)
			return wyFalse;

		ExportData2();
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	}
	return wyTrue;
}

wyBool 
ExportBatch::ExportData2()
{
	wyInt32         count;
	wyString        msg, errmsg;
	wyUInt32        timetaken;
	wyUInt32        thdid;
	HANDLE			expthd = NULL;
	MySQLDump		dump;
	EXPORTBATCH		evt = {0};
	EXPORTPARAM		param = {0};
	wyBool          isdone = wyFalse;

	/* initialize things */
	SetInitExportValues(&dump);
	SetParamValues(&param);

	count = SetInitProgressValues();
	
	/* disable the button and other windows */
	EnableDlgWindows(false);
	EnableWindow(GetDlgItem(m_hwnd, IDDONE ), false);
	EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_DROPEVENTS), false);
	SetFocus(GetDlgItem(m_hwnd, IDC_PROGRESS));

	ChangeOKButtonText(_(L"S&top"));
	VERIFY(evt.m_expevent = CreateEvent(NULL, TRUE, FALSE, NULL));
	evt.m_export    = &dump;
	evt.m_lpparam   = &param;
	evt.m_stopquery = &m_stopexport;	
	timetaken       = GetTickCount();
	m_exporting     = wyTrue; 
    m_stopexport    = wyFalse;

	expthd =(HANDLE)_beginthreadex(NULL, 0, ExportBatch::ExportThread, &evt, 0, &thdid);
	assert(expthd);
	HandleMsgs(evt.m_expevent, wyFalse);
	VERIFY(CloseHandle(evt.m_expevent));
	VERIFY(CloseHandle(expthd));

	/* event has been signalled i.e. either exporting is over or the user has stopped the process */
	/* show whether file was imported correctly or it was stopped in between */
	if(m_stopexport == wyTrue)
	{	
		msg.Add(_("Aborted by user"));	
	}
	 else if(evt.m_retcode == SUCCESS)
	 {	
		msg.Add(_("Exported successfully"));	
		isdone = wyTrue;
	 }		
	 else 
     {
		if(evt.m_retcode == FILEERROR)
			errmsg.Sprintf(_("Could not create dump file.\nMake sure the dump file - \n1. Is not read-only\n2. Is not opened by another process\n3. Is file path existing"));

	    ///If error num == 0 ; for case like 'No User Permission' 
		else if(!dump.GetErrorNum())
			errmsg.Sprintf("%s", dump.GetError());

		else
			errmsg.Sprintf(_("Error No.: %d\n%s"), dump.GetErrorNum(), dump.GetError());
		
		yog_message(m_hwnd, errmsg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);

		msg.Sprintf(_("Export unsuccessful"));
		
	 }
	
	/* show the message */
	SetWindowText(GetDlgItem(m_hwnd, IDC_MESSAGE2), msg.GetAsWideChar());
		
	if(!m_stopexport && SUCCESS == evt.m_retcode)
		UpdateFinishValues(&param);

    m_exporting = m_stopexport = 0;
	/* enable all the necessary windows that had been disabled */
	EnableDlgWindows(wyTrue);

	if(IsMySQL516(m_tunnel, m_mysql)==wyTrue)
		EnableWindow(GetDlgItem(m_hwnd, IDC_CHK_DROPEVENTS), true); 
	/* now if its mysql 5.0 then we have to do extra */
	ChangeOKButtonText(_(L"&Export"));
	
	//If it is succesful then Close button is changed to Done
	ShowWindow(GetDlgItem(m_hwnd, IDCANCEL), !isdone);
	ShowWindow(GetDlgItem(m_hwnd, IDDONE ), isdone);
	EnableWindow(GetDlgItem(m_hwnd, IDDONE ), isdone);
	EnableWindow(GetDlgItem(m_hwnd, IDOK), TRUE);

	return wyTrue;
}

/*Checks any objec is expanded or not. If expanded checks for child. If not expanded execute query to get items.
If "Data Only" option is selected we allowe to dump only tables. So you must select only table(s) this time
the variable 'istable' sets wyTrue if any table selected to dump, but we dont 'break' the loop after checking tables, but
we will contine till the value of 'isnontable' to be set
*/
wyBool
ExportBatch::ValidateInput()
{
	HWND		treehandle;
	wyWChar		temptext[SIZE_512] = {0};;
	wyInt32     checkstate = 0, count = 0, checkcount = 0, dataonly = 0;
	HTREEITEM	hrootitem, hchild;
	TVITEM		tvi = {0};
	wyBool		isexpanded = wyFalse, istoexport = wyFalse, istable = wyFalse, isnontable = wyFalse, ret = wyTrue;
	MYSQL_RES   *myres = NULL;
	wyString	query;
	MDIWindow	*wnd = NULL;
	wyBool		iscollate = wyFalse;

	VERIFY(wnd = GetActiveWin());

	//we use collate utf8_bin only if lower_case_table_names is 0 and lower_case_file_system is OFF
	if(GetmySQLCaseVariable(wnd) == 0)
		if(!IsLowercaseFS(wnd))
			iscollate = wyTrue;
	dataonly = SendMessage(GetDlgItem(m_hwnd, IDC_CHK_DATAONLY), BM_GETCHECK, 0, 0)	;
	VERIFY(treehandle	= GetDlgItem(m_hwnd, IDC_EXPORTTREE));
	
	hrootitem	= TreeView_GetRoot(treehandle);
	for(; hrootitem; hrootitem = TreeView_GetNextSibling(treehandle, hrootitem)) 
	{			
		tvi.mask		= TVIF_HANDLE | TVIF_TEXT;
		tvi.hItem		= hrootitem;
		tvi.pszText		= temptext;
		tvi.cchTextMax	= 512 - 1;

		VERIFY(TreeView_GetItem(treehandle, &tvi));

		// first see whether the item is checked or not.
		checkstate = TreeView_GetCheckState(treehandle, hrootitem);		
		if(checkstate)
		{

			checkcount++; //count incriment for each checked parent folder
            
			if((tvi.state & TVIS_EXPANDEDONCE))
			{
				isexpanded = wyTrue;
				hchild = TreeView_GetChild(treehandle, hrootitem);
				if(hchild)
				{
					istoexport = wyTrue;

                    if(wcsicmp(temptext, TXT_TABLES)== 0)
						istable = wyTrue;
					else
					{
						isnontable = wyTrue;
						break;
					}
				}
			}	

			//If 'Data Only' selected and any node except 'Tables' is checked,
            if(dataonly == BST_CHECKED && wcsicmp(temptext, TXT_TABLES)!= 0)
			{
				isnontable = wyTrue;
				break;
			}

			isexpanded = wyFalse;			
		}

		//If 'Data Only' selected but 'Tables' not checked
        else if(dataonly == BST_CHECKED && !checkstate && wcsicmp(temptext, TXT_TABLES)== 0)
		{
			istable = wyFalse;
			break;
		}
		else
			continue;
				
		//Gets the the table(s) if its not expanded...execute query for this
        if(istable == wyFalse && wcsicmp(temptext, TXT_TABLES)== 0)
		{
			if(isexpanded == wyFalse)
			{
				count = PrepareShowTable(m_tunnel, m_mysql, m_db, query);
				count = GetObjectsMyRes(m_hwnd, wnd, m_tunnel, m_mysql, myres , &query);
				if(count == -1)
					return wyFalse;

				else if(count)
				{
					istable = wyTrue;					
					//break;
				}

				if(myres)
					m_tunnel->mysql_free_result(myres);
			}
		}

		//Gets the the view(s) if its not expanded...execute query for this
        else if(isnontable == wyFalse && wcsicmp(temptext, TXT_VIEWS)== 0)
		{
			if(isexpanded == wyFalse)
			{
				count = GetSelectViewStmt(m_db.GetString(), query);
				count = GetObjectsMyRes(m_hwnd, wnd, m_tunnel, m_mysql, myres , &query);
				if(count == -1)
					return wyFalse;

				else if(count)
				{
					isnontable = wyTrue;

					break;
				}

				if(myres)
					m_tunnel->mysql_free_result(myres);
			}
		}

		//Gets the the Stored Procs(s) if its not expanded...execute query for this
        else if(isnontable == wyFalse && wcsicmp(temptext, TXT_PROCEDURES) == 0)
		{
			if(isexpanded == wyFalse)
			{
				count = GetSelectProcedureStmt(m_db.GetString(), query, iscollate);
				count = GetObjectsMyRes(m_hwnd, wnd, m_tunnel, m_mysql, myres , &query);
				if(count == -1)
					return wyFalse;

				else if(count)
				{
					isnontable = wyTrue;
					break;
				}

				if(myres)
					m_tunnel->mysql_free_result(myres);
			}
		}

		//Gets the the Function(s) if its not expanded...execute query for this
		else if(isnontable == wyFalse && wcsicmp(temptext, TXT_FUNCTIONS)== 0)
		{
			if(isexpanded == wyFalse)
			{
				count = GetSelectFunctionStmt(m_db.GetString(), query, iscollate);
				count = GetObjectsMyRes(m_hwnd, wnd, m_tunnel, m_mysql, myres , &query);
				if(count == -1)
					return wyFalse;

				else if(count)
				{
					isnontable = wyTrue;
					break;
				}

				if(myres)
					m_tunnel->mysql_free_result(myres);
			}
		}

		//Gets the the Trigger(s) if its not expanded...execute query for this
        else if(isnontable == wyFalse && wcsicmp(temptext, TXT_TRIGGERS)== 0)
		{
			if(isexpanded == wyFalse)
			{
				count = GetSelectTriggerStmt(m_db.GetString(), query);
				count = GetObjectsMyRes(m_hwnd, wnd, m_tunnel, m_mysql, myres , &query);
				if(count == -1)
					return wyFalse;

				else if(count)
				{
					isnontable = wyTrue;
					break;
				}

				if(myres)
					m_tunnel->mysql_free_result(myres);
			}
		}
			
		//Gets the the Event(s) if its not expanded...execute query for this
        else if(isnontable == wyFalse && wcsicmp(temptext, TXT_EVENTS)== 0)
		{
			if(isexpanded == wyFalse)
			{
				count = GetSelectEventStmt(m_db.GetString(), query, iscollate);
				count = GetObjectsMyRes(m_hwnd, wnd, m_tunnel, m_mysql, myres , &query);
				if(count == -1)
					return wyFalse;

				else if(count)
				{
					isnontable = wyTrue;
					break;
				}

				if(myres)
					m_tunnel->mysql_free_result(myres);
			}
		}			
	}

	ret = CheckForDumpOptions(istable, isnontable, checkcount);
	return ret;	
}

//Checks whether any object selected , Data Only option selected, 'Create database' option selected
wyBool
ExportBatch::CheckForDumpOptions(wyBool istable, wyBool isnontable, wyInt32 checkcount)
{
	HWND	hwndfilename;
	wyWChar	file[MAX_PATH + 1] = {0};
	wyInt32 iscreatedb, dataonly;

	VERIFY(hwndfilename	= GetDlgItem(m_hwnd, IDC_EXPORTFILENAME));

	//for DATA ONLY option allows to dump only tables
	dataonly = SendMessage(GetDlgItem(m_hwnd, IDC_CHK_DATAONLY), BM_GETCHECK, 0, 0)	;
	if(dataonly == BST_CHECKED) 
	{
		if(istable == wyTrue && isnontable == wyTrue) // //if tables as well as other objects are selected
		{
			yog_message(m_hwnd, _(L"Only table(s) have data. You need to select only table(s) for dumping - Data Only"), 
				pGlobals->m_appname.GetAsWideChar(), MB_ICONINFORMATION | MB_OK);
			return wyFalse;
		}

		else if(istable == wyFalse) //if objects other than table selected
		{
			yog_message(m_hwnd, _(L"Only table(s) have data. You need to select atleast one table for dumping - Data Only"), 
				pGlobals->m_appname.GetAsWideChar(), MB_ICONINFORMATION | MB_OK);
			return wyFalse;
		}
	}
	
	//If CREATE DATABASE option selected, then CREATE DB statement will dump if no object selected also
	iscreatedb = SendMessage(GetDlgItem(m_hwnd, IDC_CHK_CREATEDB), BM_GETCHECK, 0, 0);
	if(iscreatedb != BST_CHECKED)
	{
		//If no parent folder is selected
		if(!checkcount)
		{
			yog_message(m_hwnd, _(L"Select at least one object"), pGlobals->m_appname.GetAsWideChar(), 
						MB_ICONINFORMATION | MB_OK);
			return wyFalse;
		}	

		//If only parent folder(s) are selected but not children
		if(istable == wyFalse && isnontable == wyFalse)
		{
			yog_message(m_hwnd, _(L"There is nothing to export"), pGlobals->m_appname.GetAsWideChar(), 
						MB_ICONINFORMATION | MB_OK);

			return wyFalse;
		}
	}
	
	if(!SendMessage(hwndfilename, WM_GETTEXTLENGTH, 0, 0))
	{
		yog_message(m_hwnd, _(L"Please specify a filename"), pGlobals->m_appname.GetAsWideChar(), MB_ICONINFORMATION | MB_OK);
		SetFocus(hwndfilename);
		return wyFalse;
	}	
		
	/* get the export file name */
	GetWindowText(hwndfilename, file, MAX_PATH);

	// first we make sure that the file not being overwritten.
	if(!OverWriteFile(hwndfilename, file))
		return wyFalse;

	return wyTrue;
}

wyInt32 
ExportBatch::SetInitProgressValues()
{
HWND		hwndprogress;

	VERIFY(hwndprogress	= GetDlgItem(m_hwnd, IDC_PROGRESS));
	SendMessage(hwndprogress,  PBM_SETRANGE32, 0, 100);
	

    InitStepValues(hwndprogress, m_dumptablecnt, 1);
	
	return 0;
}


wyBool
ExportBatch::GetSelectedObjects(HWND hwndtree, HTREEITEM hrootitem, MySQLDump * dump,
								wyInt32 objecttype)
{
	HTREEITEM      hitem, firstchild;
	wyString       selnames;
	wyInt32        checkstate = 0;
	wyWChar		   temptext[SIZE_512] = {0};
	TVITEM		   tvi = {0};
	SelectedObjects  *str;
	

	hitem = firstchild = TreeView_GetChild(hwndtree, hrootitem);
	if(!hitem)
		return wyFalse;
	for(hitem; hitem; hitem = TreeView_GetNextSibling(hwndtree, hitem))
	{
        checkstate = TreeView_GetCheckState(hwndtree, hitem);
		if(!checkstate)
			goto loop;
	}
	return wyTrue;

loop:
	for(hitem = firstchild; hitem ;hitem = TreeView_GetNextSibling(hwndtree, hitem))
	{
        checkstate = TreeView_GetCheckState(hwndtree, hitem);
		if(!checkstate)
			continue;
		/* get the correct handle and pointer */
		tvi.mask	= TVIF_HANDLE| TVIF_TEXT;
		tvi.hItem	= hitem;
		tvi.pszText = temptext;
		tvi.cchTextMax = 512-1;

		VERIFY(TreeView_GetItem(hwndtree, &tvi));

		selnames.SetAs(temptext);		
		switch(objecttype)
		{
		case TABLE: 
			{
			 str = new SelectedObjects(selnames.GetString());
			 dump->SelectedTables(str);
			}
			 break;
		case VIEW:
			{
			 str = new SelectedObjects(selnames.GetString());
			 dump->SelectViews(str);
			}
			 break;		
		case PROCEDURE:
			{
			 str = new SelectedObjects(selnames.GetString());
			 dump->SelectedProcedures(str);
			}
			break;
		case FUNCTION :
			{
			str   = new SelectedObjects(selnames.GetString());
			dump->SelectedFunctions(str);
			}
			break;
		case TRIGGER:
			{
			str = new SelectedObjects(selnames.GetString());
			dump->SelectedTriggers(str);
			}
			break;
		case EVENT:
			{
			str = new SelectedObjects(selnames.GetString());
			dump->SelectedEvents(str);
			}
			break;		
	    }		
	}
	return wyFalse;
}

wyBool
ExportBatch::SetInitExportValues(MySQLDump * dump)
{
	wyInt32			structonly, dataonly;
    wyString		tables, appname, tablename, viewname, trigger;
	wyString         views, procname, procs, funcname, functions, triggers;
    wyInt32			checkstate;
    ConnectionInfo  conninfo;
	wyString		indtablestr, tempstr;
	ConvertString	conv;
	HWND			hwndtree;
	HTREEITEM		hrootitem;
	TVITEM			tvi = {0};
	wyWChar			temptext[SIZE_512] = {0};
	wyBool          selall, isexpanded = wyFalse;

   MDIWindow		*wnd = GetActiveWin();

	dump->SetTunnelMode((wyBool)m_tunnel->IsTunnel());
    dump->SetCriticalSection(&wnd->m_cs);

	if(m_tunnel->IsTunnel())
    {
		dump->SetTunnelHostSite(m_tunnel->GetHttpAddr());
        conninfo.m_host.SetAs(m_tunnel->GetHost());
        conninfo.m_user.SetAs(m_tunnel->GetUser());
        conninfo.m_pwd.SetAs(m_tunnel->GetPwd());
        conninfo.m_port = m_tunnel->GetPort();
		
		dump->SetHostInfo(&conninfo, m_auth); 	
	}
    else
    {
        conninfo.m_host.SetAs((*m_mysql)->host);
        conninfo.m_user.SetAs((*m_mysql)->user);
        conninfo.m_pwd.SetAs((*m_mysql)->passwd);
        conninfo.m_port = (*m_mysql)->port;

		dump->SetHostInfo(&conninfo, NULL); 	
    }

    // Set SSL Details for export
    dump->SetSSLInfo(&wnd->m_conninfo);

	//compressed protocol, wait_timeout
	dump->SetOtherConnectionInfo(&wnd->m_conninfo);

	/* set different options for the class depending upon the selection */
	dataonly	= SendMessage(GetDlgItem(m_hwnd, IDC_CHK_DATAONLY), BM_GETCHECK, 0, 0);
	if(dataonly == BST_CHECKED)
		dump->SetDumpDataOnly(wyTrue);
	else 
    {
		structonly	= SendMessage(GetDlgItem(m_hwnd, IDC_CHK_STRUCTURE), BM_GETCHECK, 0, 0);
		if(structonly == BST_CHECKED)
			dump->SetDumpStructureOnly(wyTrue);
	}

    SetOtherValues(dump);

	VERIFY(hwndtree = GetDlgItem(m_hwnd, IDC_EXPORTTREE));

	hrootitem	= TreeView_GetRoot(hwndtree);
	
	for(; hrootitem; hrootitem = TreeView_GetNextSibling(hwndtree, hrootitem)) 
	{	
		isexpanded = wyFalse;

		selall = wyFalse;
		// first see whether the item is checked or not.
		checkstate = TreeView_GetCheckState(hwndtree, hrootitem);
		
		tvi.mask		= TVIF_HANDLE | TVIF_TEXT | TVIF_STATE;
		tvi.hItem		= hrootitem;
		tvi.pszText		= temptext;
		tvi.cchTextMax	= 512 - 1;

		VERIFY(TreeView_GetItem(hwndtree, &tvi));

		if((tvi.state & TVIS_EXPANDEDONCE))
			isexpanded = wyTrue;
		
        if(wcsicmp(temptext, TXT_TABLES)== 0)
		{
			if(checkstate && isexpanded == wyTrue)
				selall = GetSelectedObjects(hwndtree, hrootitem, dump, TABLE); 
			else if(checkstate)
				selall = wyTrue;

			dump->SetAllTables(selall);
			continue;		
		}
		
		if(wcsicmp(temptext, TXT_VIEWS)== 0)
		{
			if(checkstate && isexpanded == wyTrue)
				selall = GetSelectedObjects(hwndtree, hrootitem, dump, VIEW); 
			else if(checkstate)
				selall = wyTrue;

			dump->SetAllViews(selall);
			continue;				
		}
        if(wcsicmp(temptext, TXT_PROCEDURES)== 0)
		{
			if(checkstate && isexpanded == wyTrue)
				selall = GetSelectedObjects(hwndtree, hrootitem, dump, PROCEDURE); 
			else if(checkstate)
				selall = wyTrue;

			dump->SetAllProcedures(selall);
			continue;
		}
        if(wcsicmp(temptext, TXT_FUNCTIONS)== 0)
		{
			if(checkstate && isexpanded == wyTrue)
				selall = GetSelectedObjects(hwndtree, hrootitem, dump, FUNCTION); 
			else if(checkstate)
				selall = wyTrue;

			dump->SetAllFunctions(selall);
			continue;
		}			
        if(wcsicmp(temptext, TXT_TRIGGERS)== 0)
		{
			if(checkstate && isexpanded == wyTrue)
				selall = GetSelectedObjects(hwndtree, hrootitem, dump, TRIGGER); 
			else if(checkstate)
				selall = wyTrue;

			dump->SetAllTriggers(selall);
			continue;
		}
        if(wcsicmp(temptext, TXT_EVENTS)== 0)
		{
			if(checkstate && isexpanded == wyTrue)
				selall = GetSelectedObjects(hwndtree, hrootitem, dump, EVENT); 
			else if(checkstate)
				selall = wyTrue;

			dump->SetAllEvents(selall);
			continue;
		}
	}
	dump->SetAllObjects();

	/* set the app name for comment */
	appname.Sprintf("%s %s", pGlobals->m_appname.GetString(), APPVERSION);

	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd, IDC_CHK_VERSION), BM_GETCHECK, 0, 0))
		dump->SetAppName(appname.GetString());
	else
		dump->SetAppName(pGlobals->m_appname.GetString());
	/* set append mode to false */
	dump->SetAppendFileMode(wyFalse);
	return wyTrue;
}

void 
ExportBatch::SetOtherValues(MySQLDump *dump)
{
    wyInt32     bulksize, chunklimit;
    wyWChar     file[MAX_PATH+1] = {0};
	wyString	filename;
    wyString    charset, query;
	wyBool		ischunkinsert,isprifixtimestamp=wyFalse;

    dump->SetDatabase(m_db.GetString());
	dump->SetAllTables(m_alltables);

	ischunkinsert = IsChunkInsert();
	dump->SetChunkInsert(ischunkinsert);

	if(m_tunnel->IsTunnel())
	{
		GetChunkLimit(&chunklimit);
		//In HTTP, if don't break into chunks is checked or chunk size is zero or chunk size is > 1000 , then we will use the chunk size as 1000
		//otherwise , means if chunksize is <1000, that size will use.
		if(chunklimit > MAX_ROW_LIMIT || ischunkinsert == wyFalse || chunklimit == 0)
			chunklimit = MAX_ROW_LIMIT;
		dump->SetChunkLimit(chunklimit);
	}
	else
	{
		//In Direct, If don't break into chunks is checked or chunk size = 0, then we will not break data into chunks.
		//if user defined a value in chunk size , then we will use that value as chunk size
		if(ischunkinsert == wyTrue)
		{
			GetChunkLimit(&chunklimit);
			dump->SetChunkLimit(chunklimit);
		}
	}
   
	if(IsMySQL41(m_tunnel, m_mysql))
		dump->SetCharSet("utf8");
    else
    {
        if(GetServerDefaultCharset(m_tunnel, *m_mysql, charset, query) == wyFalse)
            ShowMySQLError(m_hwnd, m_tunnel, m_mysql, query.GetString());
        
        dump->SetCharSet(charset.GetString());
        /*wyString    query, charset;
        MYSQL_RES   *myres;
        MYSQL_ROW   myrow;
        MDIWindow *wnd = GetActiveWin();

        query.SetAs("show variables like '%character%';");    
        myres = ExecuteAndGetResult(wnd, m_tunnel, m_mysql, query, wyTrue);
        if(!myres)
        {
            
            query.SetAs("latin1");
	    }
        myrow = m_tunnel->mysql_fetch_row(myres);
        
        if(myrow[1])
            charset.SetAs(myrow[1]);*/
        
    }
	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd, IDC_CHK_USEDBNAME), BM_GETCHECK, 0, 0))
		dump->SetUseDb(wyTrue);
	
	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd, IDC_CHK_DROPOBJECT), BM_GETCHECK, 0, 0))
		dump->SetDropObjects(wyTrue); 
	
	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd, IDC_CHK_CREATEDB), BM_GETCHECK, 0, 0))
		dump->SetCreateDatabase(wyTrue);
	
	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd, IDC_CHK_LOCKFORREAD), BM_GETCHECK, 0, 0))
		dump->SetLockTable(wyTrue);
	
	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd, IDC_CHK_FLUSHLOGS), BM_GETCHECK, 0, 0))
		dump->SetFlushLogs(wyTrue);

	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd, IDC_CHK_LOCKAROUNDINSERT), BM_GETCHECK, 0, 0))
		dump->SetInsertWriteLocks(wyTrue);

	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd, IDC_SETUNIQUE), BM_GETCHECK, 0, 0))
		dump->SetFKChecks(wyTrue);

	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd, IDC_CHK_VERSION), BM_GETCHECK, 0, 0))
		dump->SetVersionInfo(wyTrue);
	else 
		dump->SetVersionInfo(wyFalse);

	/*if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd,IDC_ZIP), BM_GETCHECK, 0, 0))
		dump->SetToCompress(wyTrue);*/
	//timestamp as prefix 
	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd,IDC_TIMESTAMP_PREFIX), BM_GETCHECK, 0, 0))
		{
			dump->SetTimeStampMode(wyTrue);
			   isprifixtimestamp=wyTrue;
	    }
	else
		{
			dump->SetTimeStampMode(wyFalse);
			isprifixtimestamp=wyFalse;
	}
	

	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd, IDC_BULKINSERT), BM_GETCHECK, 0, 0))
	{
		
		dump->SetBulkInsert(wyTrue);
		// sets the size of query that can be used in bulkinsert.
		GetBulkSize (&bulksize);
		dump->SetBulkSize(bulksize);
	}

	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd, IDC_SINGLE_TRANSACTION), BM_GETCHECK, 0, 0))
		dump->SetSingleTransaction(wyTrue);

	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd, IDC_CHK_DEFINER), BM_GETCHECK, 0, 0))
		dump->SetDefiner(wyTrue);
	else
		dump->SetDefiner(wyFalse);

	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd, IDC_CHK_HEXBLOB), BM_GETCHECK, 0, 0))
		dump->SetHexBlob(wyTrue);
	else
		dump->SetHexBlob(wyFalse);
	
	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd, IDC_ONEROW), BM_GETCHECK, 0, 0))
		dump->SetOneRowPerLine(wyTrue);
	else
		dump->SetOneRowPerLine(wyFalse);

	GetWindowText(GetDlgItem(m_hwnd, IDC_EXPORTFILENAME), file, MAX_PATH);
	
	filename.SetAs(file);
	//file per table option
	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd,IDC_FILE_TABLE), BM_GETCHECK, 0, 0))
	{
		dump->SetExpFilePath((wyChar*)filename.GetString(),isprifixtimestamp,wyFalse);
	 
	}
	else
	{
	dump->SetExpFile(filename.GetString());
	}

}

wyBool
ExportBatch::SetParamValues(EXPORTPARAM * param)
{
	param->m_hwndmsg = GetDlgItem(m_hwnd, IDC_MESSAGE2);
	param->m_hwndprogress = GetDlgItem(m_hwnd, IDC_PROGRESS);

	return wyTrue;
}

wyBool
ExportBatch::EnableDlgWindows(bool state)
{
	wyInt32 id[] = {
					 IDC_EXPORT_DBCOMBO, IDC_EXPORTTREE, IDC_CHK_DATA, IDC_CHK_DATAONLY, IDC_CHK_STRUCTURE,
					 IDC_EXPORTFILENAME, IDC_EXPFILESELECT, 
					 IDC_CHK_USEDBNAME, IDC_CHK_DROPTABLE, IDC_CHK_CREATEDB,
					 IDC_CHK_LOCKFORREAD, IDC_CHK_FLUSHLOGS, IDC_CHK_LOCKAROUNDINSERT,
					 IDC_SETUNIQUE, IDC_BULKINSERT,IDC_ONEROW,IDC_SINGLE_TRANSACTION, IDCANCEL, IDC_CHK_DROPPROCEDURES, IDC_CHK_DROPVIEWS,
					 IDC_CHK_DROPFUNCTIONS, IDC_CHK_DROPTRIGGERS, IDC_CHK_DEFINER, IDC_CHK_VERSION, IDC_CHK_HEXBLOB,IDC_TIMESTAMP_PREFIX, IDC_FILE_TABLE
	};

	wyInt32 i, count;
	HWND	hobj;

	count = sizeof(id) / sizeof(id[0]);

	for(i = 0; i < count; i++)
    {
		VERIFY(hobj = GetDlgItem(m_hwnd, id[i]));
		EnableWindow(hobj, state);
	}
	
	if(IsMySQL402(m_tunnel, m_mysql) == wyFalse)
		EnableWindow(GetDlgItem(m_hwnd, IDC_SINGLE_TRANSACTION), FALSE);
	
	return wyTrue;

}

void
ExportBatch::ChangeOKButtonText(LPCWSTR text)
{
	SetWindowText(GetDlgItem(m_hwnd, IDOK), text);
	return;
}
void 
ExportBatch::StopExporting()
{
    MDIWindow *wnd = GetActiveWin();

    if(!wnd)
        return;

#ifndef _CONSOLE
    // Request ownership of the critical section.
    EnterCriticalSection(&wnd->m_cs);
#endif
    // Access the shared resource.

    m_stopexport = wyTrue;

#ifndef _CONSOLE
     // Release ownership of the critical section.
    LeaveCriticalSection(&wnd->m_cs);
#endif
}
unsigned __stdcall 
ExportBatch::ExportThread(LPVOID lpparam)
{
	EXPORTBATCH		*exp =(EXPORTBATCH*)lpparam;
	
	/* by default we always to complete insert in this dialog. recommended by Daevid Vincent */
	exp->m_export->SetCompleteInsert(wyTrue);
    exp->m_export->SetEntInstance(pGlobals->m_hinstance);
	
	exp->m_retcode =  exp->m_export->Dump(ExportBatch::GuiUpdate, (void*)exp->m_lpparam, exp->m_stopquery);
	SetEvent(exp->m_expevent);
	return 0;
}
void
ExportBatch::GuiUpdate(void * lpParam, const wyChar * tablename, wyUInt32 rows, EXPORTTABLESTATE state)
{
	EXPORTPARAM	    *param =(EXPORTPARAM*)lpParam;
    wyString        msg;
	wyInt32			gtpos; 

    switch(state)
    {
        case  TABLESTART:
		    msg.Sprintf(_("Exporting table: %s"), tablename);
            break;

        case TABLEROWS:
            {
                msg.Sprintf(_("Exporting table: %s"), tablename);
				//InitStepValues(param->m_hwndprogress, rows, 100);
            }
            break;
		case VIEWSTART:
			msg.Sprintf(_("Exporting view: %s"), tablename);
			break;
		case PROCEDURESTART:
			msg.Sprintf(_("Exporting procedure: %s"), tablename);
			break;
		case FUNCTIONSTART:
			msg.Sprintf(_("Exporting function: %s"), tablename);
			break;
		case EVENTSTART:
			 msg.Sprintf(_("Exporting event: %s"), tablename);
            break;
		case TRIGGERSTART:
			msg.Sprintf(_("Exporting trigger: %s"), tablename);
			break;
        case TABLEEXPORT:
            {
				msg.Sprintf(_("Exporting table: %s \nRows exported: %d"), tablename, rows);
               	break;
            }
		case ENDSTART:
			SendMessage(param->m_hwndprogress, PBM_STEPIT, 0, 0);	
			break;

		case SETOBJECTCOUNT:
			gtpos = SendMessage(param->m_hwndprogress,  PBM_GETPOS, 0, 0);
			if(rows > 0)
			{
				InitStepValues(param->m_hwndprogress, rows + 1, 1);
				SendMessage(param->m_hwndprogress,  PBM_SETPOS, (WPARAM)gtpos, 0);
			}
			else
			{
				InitStepValues(param->m_hwndprogress, 100, 1);
			}
			SendMessage(param->m_hwndprogress, PBM_STEPIT, 0, 0);
			break;

		case FETCHDATA:
			msg.Sprintf(_("Fetching list of %s from server..."), tablename);
			InitStepValues(param->m_hwndprogress, 100, 1);
			SendMessage(param->m_hwndprogress, PBM_STEPIT, 0, 0);	
			break;
    }
    SetWindowText(param->m_hwndmsg, msg.GetAsWideChar());
}

void 
ExportBatch::InitStepValues(HWND hwnd, wyInt32 count, wyInt32 step)
{
	SendMessage(hwnd,  PBM_SETRANGE32, 0, 0);
	UpdateWindow(hwnd);
	SendMessage(hwnd,  PBM_SETRANGE32, 0, count);
	SendMessage(hwnd,  PBM_SETSTEP,(WPARAM)step, 0); 
}

wyBool
ExportBatch::UpdateFinishValues(EXPORTPARAM * param)
{
	SendMessage(param->m_hwndprogress,  PBM_SETRANGE32, 0, 1);
	SendMessage(param->m_hwndprogress,  PBM_DELTAPOS,(WPARAM)1, 0); 

	return wyTrue;
}
wyBool 
ExportBatch::IsExporting()
{
    MDIWindow *wnd = GetActiveWin();
    wyBool     ret  = wyFalse;

    if(!wnd)
        return ret;

#ifndef _CONSOLE
    // Request ownership of the critical section.
    EnterCriticalSection(&wnd->m_cs);
#endif
    // Access the shared resource.

    ret = (m_stopexport == wyTrue)? wyFalse:wyTrue;

#ifndef _CONSOLE
     // Release ownership of the critical section.
    LeaveCriticalSection(&wnd->m_cs);
#endif

    return ret;
}

//To expand treeview while click on treeview
wyBool
ExportBatch::OnItemExpandingHelper(HWND hwnd , LPNMTREEVIEW pnmtv, wyInt32 dataonly_chck)
{
	TVITEM			tvi, tviOB;
    wyWChar         filterText[70], str[70];
	wyBool			issingletable = wyFalse;
	TREEVIEWPARAMS	treeviewparam = {0};
    wyInt32         image = 0, image1 = 0;
    MDIWindow       *wnd = GetActiveWin();
    HWND            hobtree = wnd->m_pcqueryobject->m_hwnd;
    HTREEITEM       hti = NULL;
    wyBool      atleastOneItemChecked = wyFalse;

	tvi = pnmtv->itemNew;

	/*if(m_table.GetLength())
		issingletable = wyTrue;*/

	treeviewparam.database = m_db.GetAsWideChar();
	treeviewparam.hwnd = hwnd;
	treeviewparam.isopentables = wyFalse;
	treeviewparam.isrefresh = wyTrue;
	treeviewparam.issingletable = issingletable;
	treeviewparam.mysql = m_mysql;
	treeviewparam.tunnel = m_tunnel;
	treeviewparam.tvi = &tvi;	
	treeviewparam.checkboxstate = wyTrue;

	OnItemExpanding(treeviewparam);

    image = wnd->m_pcqueryobject->GetSelectionImage();
    image1 = GetItemImage(hwnd, tvi.hItem);

    if(!(tvi.state & TVIS_EXPANDEDONCE) && ((m_dump_all_tables == wyTrue && image1 == NTABLES) || (m_dump_all_tables == wyFalse)))
    {
        if(!m_table.GetLength())
        {
            hti = TreeView_GetRoot(hobtree);
            hti = TreeView_GetChild(hobtree, hti);
            while(hti)
            {
                tviOB.hItem = hti;
                tviOB.pszText = str;
                tviOB.cchTextMax = 65;
                tviOB.mask = TVIF_TEXT;
                TreeView_GetItem(hobtree, &tviOB);
                if(!wcscmp(tviOB.pszText, m_db.GetAsWideChar()))
                {
                    break;
                }
                hti = TreeView_GetNextSibling(hobtree, hti);
            }
            
            

            hti = TreeView_GetChild(hobtree, hti);
            if(hti)
            {
                for(;hti;hti = TreeView_GetNextSibling(hobtree, hti))
                {
                    image = GetItemImage(hobtree, hti);
                    if(image == image1)
                        break;
                }

                if(hti)
                {
                    tviOB.hItem = hti;
                    tviOB.mask = TVIF_PARAM;
                    TreeView_GetItem(hobtree, &tviOB);

                    if(tviOB.lParam && wcslen(((OBDltElementParam *)tviOB.lParam)->m_filterText))
                    {
                        wcscpy(filterText, ((OBDltElementParam *)tviOB.lParam)->m_filterText);
                        _wcslwr_s(filterText, 65);
                
                        for(hti = TreeView_GetChild(hwnd, tvi.hItem); hti; hti = TreeView_GetNextSibling(hwnd, hti))
                        {
                            tviOB.hItem = hti;
                            tviOB.mask = TVIF_TEXT;
                            tviOB.pszText = str;
                            tviOB.cchTextMax = 65;
                            TreeView_GetItem(hwnd, &tviOB);
                            _wcslwr_s(tviOB.pszText, 65);
                            if(wcsstr(tviOB.pszText, filterText))
                            {
                                TreeView_SetCheckState(hwnd, hti, 1);
                                atleastOneItemChecked = wyTrue;
                            }
                            else
                            {
                                TreeView_SetCheckState(hwnd, hti, 0);
                            }
                        }
                        if(atleastOneItemChecked)
                        {
                            TreeView_SetCheckState(hwnd, tvi.hItem, 1);
                        }
                        else
                        {
                            TreeView_SetCheckState(hwnd, tvi.hItem, 0);
                        }
                    }
                }
            }
        }
        else
        {
            hti = TreeView_GetRoot(hwnd);
            hti = TreeView_GetChild(hwnd, hti);
            for(;hti; hti = TreeView_GetNextSibling(hwnd, hti))
            {
                tviOB.hItem = hti;
                tviOB.mask = TVIF_TEXT;
                tviOB.cchTextMax = 70;
                tviOB.pszText = str;
                TreeView_GetItem(hwnd, &tviOB);
                if(!wcsicmp(tviOB.pszText, m_table.GetAsWideChar()))
                {
					if(image1 == NTABLES)
					{
						TreeView_SetCheckState(hwnd, hti, 1);
						atleastOneItemChecked = wyTrue;
					}
                }
            }
            if(atleastOneItemChecked)
            {
				
				if(dataonly_chck == BST_CHECKED)
				{	
					if(image1 == NTABLES)
					{
						TreeView_SetCheckState(hwnd, tvi.hItem, 1);
					}
					else
					{
						 TreeView_SetCheckState(hwnd, tvi.hItem, 0);
					}
				}
				else
					TreeView_SetCheckState(hwnd, tvi.hItem, 1);
			}
        }
    }

	return wyFalse;
}

void
ExportBatch::GetCtrlRects()
{
    RECT    rect;
    wyInt32 i, count;
    HWND    hwnd;

    //ctrlid    size-x     size-y
    wyInt32 ctrlid[] = {
       	IDC_TEXT1, 0, 0,
        IDC_GROUP1, 0, 0,
        IDC_CHK_STRUCTURE, 0, 0,
        IDC_CHK_DATAONLY, 0, 0,
        IDC_CHK_DATA, 0, 0,
        IDC_TEXT2, 0, 0,
        IDC_TEXT3, 0, 0,
        IDC_EXPORT_DBCOMBO, 1, 0,
        IDC_EXPORTFILENAME, 1, 0,
        IDC_EXPFILESELECT, 0, 0,
        IDC_TEXT4, 0, 0,
        IDC_EXPORTTREE, 1, 1,
        IDC_GROUP2, 0, 0,
        IDC_CHK_LOCKFORREAD, 0, 0,
        IDC_CHK_FLUSHLOGS, 0, 0,
        IDC_SINGLE_TRANSACTION, 0, 0,
        IDC_GROUP3, 0, 0,
        IDC_CHK_USEDBNAME, 0, 0,
        IDC_CHK_CREATEDB, 0, 0,
		IDC_SETUNIQUE, 0, 0,
		IDC_CHK_LOCKAROUNDINSERT, 0, 0,
		IDC_BULKINSERT, 0, 0,
		IDC_ONEROW, 0, 0,
		IDC_CHK_DROPOBJECT, 0, 0,
		IDC_CHK_DEFINER, 0, 0,
		IDC_CHK_VERSION, 0, 0,
		IDC_CHK_HEXBLOB, 0, 0,
		IDC_GROUP4, 0, 0,
        IDC_TIMESTAMP_PREFIX, 0, 0,
        IDC_FILE_TABLE, 0, 0,
		IDC_PROGRESS, 1, 0,
		IDC_MESSAGE2, 1, 0,
		IDOK, 0, 0,
		IDCANCEL, 0, 0,
		IDDONE, 0, 0,
		IDC_EXPDAT_GRIP, 0, 0,
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
ExportBatch::PositionCtrls()
{
    DlgControl* pdlgctrl;
    wyInt32     leftpadding, toppadding, rightpadding, bottompadding, width, height;
    wyInt32     x, y;

    RECT        rect;

    HDWP        hdefwp;
		
	GetClientRect(m_hwnd, &rect);

    //BeginDeferWindowPos is used to make the control positioning atomic
    hdefwp = BeginDeferWindowPos(m_controllist.GetCount() + 1);

    //iterate through the control lists
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
			switch(pdlgctrl->m_id)
			{
				case IDC_TEXT2:
				case IDC_TEXT4:
				case IDC_TEXT1:
				case IDC_GROUP1:
				case IDC_CHK_STRUCTURE:
				case IDC_CHK_DATAONLY:
				case IDC_CHK_DATA:
					x = leftpadding ;
					break;

				case IDC_TEXT3:
					x = rect.right/3 + 18;
					break;

				case IDC_EXPDAT_GRIP:
					x = rect.right - width;
					break;

				default:	
					x=rect.right - rightpadding - width;
			}
        }
        else
        {
			x= leftpadding;
			width = width + 1;
			switch(pdlgctrl->m_id)
			{
				case IDC_EXPORT_DBCOMBO:
					x = leftpadding;
					width = rect.right/3 - 3;
					break;
				
				case IDC_EXPORTFILENAME:
					x = rect.right/3 + 18;
					width = rect.right - x - rightpadding;
					break;

				default:
					x = leftpadding;
					width = rect.right - leftpadding - rightpadding;
			}
        }
	    switch(pdlgctrl->m_id)
        {
			case IDOK:
			case IDDONE:
			case IDCANCEL:
			case IDC_MESSAGE2:
			case IDC_PROGRESS:
                y = rect.bottom - bottompadding - height;
                break;

			case IDC_EXPDAT_GRIP:
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
ExportBatch::ExpBatResize(HWND hwnd){

	PositionCtrls();
}

void
ExportBatch::OnWMSizeInfo(LPARAM lparam)
{
    MINMAXINFO* pminmax = (MINMAXINFO*)lparam;
    pminmax->ptMinTrackSize.x = m_wndrect.right - m_wndrect.left;
    pminmax->ptMinTrackSize.y = m_wndrect.bottom - m_wndrect.top;
}

//function handles the WM_PAINT and paints the window using double buffering
void
ExportBatch::OnPaint(HWND hwnd)
{
    HDC         hdc;
	PAINTSTRUCT ps;
	HWND hwndstat = GetDlgItem(hwnd, IDC_EXPDAT_GRIP);
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
void ExportBatch::IsDirectory(HWND hwnd)
{
	TCHAR buff[1024];
    

	if(BST_CHECKED == SendMessage(GetDlgItem(m_hwnd,IDC_FILE_TABLE), BM_GETCHECK, 0, 0))
	{
	GetWindowText(GetDlgItem(hwnd,IDC_EXPORTFILENAME), buff, 1024);
	if(GetFileAttributes(buff)!=FILE_ATTRIBUTE_DIRECTORY)
	{
	  yog_message(hwnd, _(L"Path specified is not a directory, Please select a directory"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_HELP | MB_ICONINFORMATION);
	  SetFocus(GetDlgItem(hwnd,IDC_EXPORTFILENAME));
	}
	
	}

}