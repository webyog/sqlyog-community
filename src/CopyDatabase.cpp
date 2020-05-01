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
#include <Shlwapi.h>
#include "ExportMultiFormat.h"
#include "FrameWindowHelper.h"
#include "MySQLVersionHelper.h"
#include "ClientMySQLWrapper.h"
#include "Global.h"
#include "CopyDatabase.h"
#include "CommonHelper.h"
#include "SQLMaker.h"
#include "GUIHelper.h"
#include "DBListBuilder.h"

#define COPIED_STOREDPGRMS_MANUALADJUST _(L"One or more stored programs or views were copied.\
			\r\nThey may contain SQL-referencing the <source> database name and will need to be adjusted manually.")

extern	PGLOBALS		pGlobals;

CopyDatabase::CopyDatabase()
{
     // Initialize the critical section one time only.
    InitializeCriticalSection(&m_cs);
	m_dontnotify		= wyFalse;
	m_newtargetmysql    = NULL;
	m_targettunnel      = NULL;
	m_newtargettunnel   = NULL;

    m_newsrctunnel      = NULL;
    m_newsrcmysql       = NULL;
	m_srcprocess        = INVALID_HANDLE_VALUE;
    m_tgtprocess        = INVALID_HANDLE_VALUE;
	m_tgtinfo           = NULL;
    m_srcinfo           = NULL;

	m_p                 = new Persist;
    m_stop              = wyFalse;

    m_hcopythread      = NULL;
    m_copyevent        = NULL;

    m_copying          = wyFalse;
    m_is5xobjects      = wyFalse;
	m_istrg5xobjects   = wyFalse;
	m_issrc5xobjects   = wyFalse;
	m_issrceventsupport = wyFalse;
	m_istrgeventsupports  = wyFalse;
	m_bulkinsert       = wyFalse;
	m_maxallowedsize   = 0;
	m_isobjecttoexport = wyFalse;
	m_isstoredpgrms	= wyFalse;	
	m_ispromtstorepgrmmessage = wyFalse;

	m_srcsshpipehandles.m_hreadpipe = INVALID_HANDLE_VALUE;
	m_srcsshpipehandles.m_hreadpipe2 = INVALID_HANDLE_VALUE;
	m_srcsshpipehandles.m_hwritepipe = INVALID_HANDLE_VALUE;
	m_srcsshpipehandles.m_hwritepipe2 = INVALID_HANDLE_VALUE;

	m_tgtsshpipehandles.m_hreadpipe= INVALID_HANDLE_VALUE;
	m_tgtsshpipehandles.m_hreadpipe2 = INVALID_HANDLE_VALUE;
	m_tgtsshpipehandles.m_hwritepipe = INVALID_HANDLE_VALUE;
	m_tgtsshpipehandles.m_hwritepipe2 = INVALID_HANDLE_VALUE;

    m_selalltables = wyFalse;
	m_isremdefiner = wyFalse;
	m_iscreatedb = wyFalse;
	m_iscopytrigger=wyFalse;
	m_dblist  = NULL;
	m_countdb =	0;
	m_defaultdbindex = 0;
}

CopyDatabase::~CopyDatabase()
{
	//delete the dialog control list
	DlgControl* pdlgctrl;
	while((pdlgctrl = (DlgControl*)m_controllist.GetFirst()))
	{
		m_controllist.Remove(pdlgctrl);
		delete pdlgctrl;
	}
	
    // Release resources used by the critical section object.
    DeleteCriticalSection(&m_cs);
	if(m_dblist)
	{
		free(m_dblist);
		m_countdb=0;
		m_defaultdbindex = 0;
	}
}

// this function creates the actual dialog box and also initializes some values of the class
wyBool
CopyDatabase::Create(HWND hwndparent, Tunnel * tunnel, PMYSQL umysql, wyChar *db, wyChar *table, wyBool checktables)
{
	wyInt64            ret;
	m_srcmysql			= umysql;
	m_srctunnel			= tunnel;
	m_dontnotify		= wyFalse;
	m_iscopytrigger      =wyFalse;
    //if table is valid, then set m_table
    if(table)
	    m_table.SetAs(table);
    //else if the the falg to set only the tables is there, then set the member variable
    else if(checktables == wyTrue)
        m_selalltables = wyTrue;

	m_srcdb.SetAs(db);
	
	//Post 8.01
	//RepaintTabModule();

	ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_COPYDATABASE), hwndparent,
							CopyDatabase::WndProc,(LPARAM)this);

	return wyTrue;
}

// Callback dialog procedure for the window.

INT_PTR CALLBACK
CopyDatabase::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPNMHDR        lpnm = (LPNMHDR)lParam;
	LPNMTVKEYDOWN   ptvkd = (LPNMTVKEYDOWN) lParam ;
	CopyDatabase * pcd =(CopyDatabase*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	LPNMTREEVIEW treeview = (LPNMTREEVIEW)lParam;

    switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		return TRUE;

	case WM_INITDLGVALUES:
		// we set some initial value.
		pcd->m_p->Create("COPYDATABASE");
		VERIFY(pcd->m_hwnddlg = hwnd);
		VERIFY(pcd->m_hwndcombo = GetDlgItem(hwnd, IDC_SOURCEDB));
		VERIFY(pcd->m_hwndcombodb = GetDlgItem(hwnd, IDC_SOURCEDB2));
		pcd->CreateImageList();
		pcd->AddInitData();	
		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/105-copy-table-to-different-host");
		return TRUE;

	case UM_COPYDATABASE:
       	pcd->OnUmCopydatabase(hwnd);
       	break;

    case WM_MOUSEMOVE:
        pcd->OnMouseMove(hwnd, lParam);
		break;

	case WM_MEASUREITEM:
		OnComboMeasureItem((LPMEASUREITEMSTRUCT)lParam);
		break;

	case WM_DRAWITEM:
		//draw connection color with connection name
		OnDrawConnNameCombo(GetDlgItem(hwnd, LOWORD(wParam)), (LPDRAWITEMSTRUCT)lParam, wyTrue);
		break;

	case WM_NOTIFY:
		{
		if(lpnm->idFrom == IDC_TREE && lpnm->code == NM_CLICK)
			{
			HandleTreeViewItem(lParam, wParam);
			if(pcd->m_iscopytrigger==wyTrue)
			{
				SetCursor(LoadCursor(NULL, IDC_WAIT));
				HWND htree;
				TVITEM		        tvi;
				wyWChar             temptext[SIZE_1024] = {0};
				VERIFY(htree=GetDlgItem(hwnd, IDC_TREE));
				ZeroMemory(&tvi, sizeof(TVITEM));
				tvi.mask       = TVIF_IMAGE | TVIF_TEXT;
				tvi.pszText    = temptext;
				tvi.cchTextMax = SIZE_1024;
				tvi.hItem      = TreeView_GetSelection(htree);
				wyString table_name;
				TreeView_GetItem(htree, &tvi);	
				if(tvi.iImage==NTABLE)
				{
					if(TreeView_GetCheckState(htree, tvi.hItem)==0)
					{
						table_name.SetAs(tvi.pszText);
						pcd->FindTrigerAndCheck(htree,&table_name,wyFalse);
					}

					else
					{
						table_name.SetAs(tvi.pszText);
						pcd->FindTrigerAndCheck(htree,&table_name,wyTrue);
					}
				}	
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			}
		}
		if((ptvkd->hdr.idFrom == IDC_TREE) && ptvkd->wVKey == VK_SPACE)
		{HandleTreeViewItem(lParam, wParam, wyTrue);
		if(pcd->m_iscopytrigger==wyTrue){
				SetCursor(LoadCursor(NULL, IDC_WAIT));
				HWND htree;
				TVITEM		        tvi;
				wyWChar             temptext[SIZE_1024] = {0};
				VERIFY(htree=GetDlgItem(hwnd, IDC_TREE));
				ZeroMemory(&tvi, sizeof(TVITEM));
				tvi.mask       = TVIF_IMAGE | TVIF_TEXT;
				tvi.pszText    = temptext;
				tvi.cchTextMax = SIZE_1024;
				tvi.hItem      = TreeView_GetSelection(htree);
				wyString table_name;
				TreeView_GetItem(htree, &tvi);	
				if(tvi.iImage==NTABLE)
				{
					if(TreeView_GetCheckState(htree, tvi.hItem)==0)
					{
						table_name.SetAs(tvi.pszText);
						pcd->FindTrigerAndCheck(htree,&table_name,wyFalse);
					}

					else
					{
						table_name.SetAs(tvi.pszText);
						pcd->FindTrigerAndCheck(htree,&table_name,wyTrue);
					}
				}	
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			}
		
		
		}
		
		if(lpnm->code == TVN_ITEMEXPANDING )
		{
			if(treeview->itemNew.iImage == NTABLES)
			{
				if(pcd->m_dontnotify)
					return wyFalse;
			}
			return pcd->OnItemExpandingHelper(lpnm->hwndFrom, (LPNMTREEVIEW)lParam);			
		}
		}
		break;
	case WM_COMMAND:
		pcd->OnWmCommand(hwnd, wParam);
		break;

	case WM_SIZE:
		pcd->CopyDbResize(hwnd);
		break;

	case WM_GETMINMAXINFO:
		pcd->OnWMSizeInfo(lParam);
		break;

	case WM_PAINT:
		pcd->OnPaint(hwnd);
		break;

	case WM_ERASEBKGND:
		//blocked for double buffering
		return 1;

	case WM_DESTROY:
		SaveInitPos(hwnd, COPYDATABASE_SECTION);
		if(pcd->m_himl)
		{
			ImageList_Destroy(pcd->m_himl);
		}

		pcd->FreeComboParam();
		
		if(pcd->m_p)
		{
			delete pcd->m_p;
		}
		break;
	}

	return 0;
}

void 
CopyDatabase::OnMouseMove(HWND hwnd, LPARAM lparam)
{
    RECT    rect;
    POINT   pt;

    if(m_copying == wyTrue)
	{
		pt.x = GET_X_LPARAM(lparam);
		pt.y = GET_Y_LPARAM(lparam);
		
		GetWindowRect(GetDlgItem(hwnd, IDOK), &rect);
		MapWindowPoints(hwnd, NULL, &pt, 1); 			

		//Cursor shows wait  on Stop button while copying
		if( PtInRect(&rect, pt) && IsCopyStopped() == wyFalse)
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		else
			SetCursor(LoadCursor(NULL, IDC_WAIT));
	}

    return;
}

void 
CopyDatabase::OnUmCopydatabase(HWND hwnd)
{
    wyBool       ret;
    wyUInt32     thdid,index;
    COPYDBPARAM  copydbparam;
    COPYDB       evt;
	HWND         hwndtree;
	MDIWindow	*wnd = NULL;
	wyString	msg;
	HWND		hwndmsg = NULL;
	wyWChar     buffer[SIZE_128];
	VERIFY(wnd = GetActiveWin());
	//LPDIFFCOMBOITEM lpdiff;
	m_ispromtstorepgrmmessage = wyFalse;
	m_isstoredpgrms = wyFalse;

    SetFocus(GetDlgItem(m_hwnddlg, IDC_CHECK1));

    EnableDlgWindows(wyFalse);  
	EnableWindow(GetDlgItem(hwnd, IDDONE ), false);
	EnableWindow(GetDlgItem(m_hwnddlg, IDC_SUMMARY), wyFalse);
    
	m_summary.Clear();
    
	VERIFY(hwndtree = GetDlgItem(m_hwnddlg, IDC_TREE));	
	// first see if any objects are selected.
	if(IsObjectsSelected(hwndtree) == wyFalse)
	{ 
		yog_message(hwndtree, _(L"Please select atleast one object"), pGlobals->m_appname.GetAsWideChar(),MB_OK|MB_ICONINFORMATION | MB_HELP);
		EnableDlgWindows(wyTrue);
        m_copying   = wyFalse;
		return;
	}
	else
	{ 
		GetDlgItemText(m_hwnddlg, IDC_SOURCEDB2, buffer, SIZE_128 - 1);
		m_targetdb.SetAs(buffer);
		m_targetdb.RTrim();
		index = SendMessage(m_hwndcombodb, CB_FINDSTRINGEXACT, -1,(LPARAM)m_targetdb.GetAsWideChar());
		if(index != CB_ERR)
		{
			m_iscreatedb = wyFalse;
		}
		else
		{
			m_iscreatedb = wyTrue;
		}
		if(m_targetdb.GetLength() == 0)
		{
			yog_message(hwndtree, _(L"Please select a database or enter the name for new database"), pGlobals->m_appname.GetAsWideChar(),MB_OK|MB_ICONINFORMATION | MB_HELP);
			EnableDlgWindows(wyTrue);
			m_copying   = wyFalse;
			return;
		}
	}

	ret = GetTargetDatabase();
	if(ret == wyFalse)
    {
        EnableDlgWindows(wyTrue);
        m_copying   = wyFalse;
		return;
    }
		
	hwndmsg = GetDlgItem(m_hwnddlg, IDC_MESSAGE);


	evt.m_copydbevent = CreateEvent(NULL, TRUE, FALSE, NULL);
    evt.m_copydb    = this;
    evt.m_lpParam   = &copydbparam;

	if(hwndmsg)
	{
		msg.Sprintf(_("Connecting to source server  "));
		SendMessage(hwndmsg, WM_SETTEXT, 0, (LPARAM)msg.GetAsWideChar());
	}
	    
    ret = pGlobals->m_pcmainwin->m_connection->CreateSourceInstance(this);
    if(ret == wyFalse)
    {
        EnableDlgWindows(wyTrue);
        m_copying   = wyFalse;
		if(evt.m_copydbevent)
			VERIFY(CloseHandle(evt.m_copydbevent));
		return;
    }

	if(hwndmsg)
	{
		msg.Sprintf(_("Connecting to target server  "));
		SendMessage(hwndmsg, WM_SETTEXT, 0, (LPARAM)msg.GetAsWideChar());
	}

	ret = pGlobals->m_pcmainwin->m_connection->CreateTargetInstance(this);
	if(ret == wyFalse)
    {
        EnableDlgWindows(wyTrue);
        m_copying   = wyFalse;
        RemoveSourceInstances();
		if(evt.m_copydbevent)
			VERIFY(CloseHandle(evt.m_copydbevent));
		return;
    }

    ret = InitExportData(hwndtree);
    if(ret == wyFalse)
    {
		Delete(&m_seltables);
        EnableDlgWindows(wyTrue);
        m_copying   = wyFalse;
        RemoveInstances();
		if(evt.m_copydbevent)
			VERIFY(CloseHandle(evt.m_copydbevent));
		return;
    }
    
    SetGroupProcess(wnd, wyTrue);
    copydbparam.m_hwndmsg = GetDlgItem(m_hwnddlg, IDC_MESSAGE);
    copydbparam.m_hwnddlg = m_hwnddlg;
	copydbparam.m_summary = &m_summary;
    m_copydbparam = &copydbparam;

//	evt.m_copydbevent = CreateEvent(NULL, TRUE, FALSE, NULL);
 //   evt.m_copydb    = this;
 //   evt.m_lpParam   = &copydbparam;

    
    m_stopcopying =  wyFalse;
	m_isobjecttoexport = wyFalse;

    VERIFY(m_hcopythread = (HANDLE)_beginthreadex(NULL, 0, 
                           CopyDatabase::ExecuteCopyThread, &evt, 0, &thdid));

    if(!m_hcopythread)
    	goto cleanup;

    HandleMsgs(evt.m_copydbevent, wyFalse,m_hwnddlg);

cleanup:
    if(evt.m_copydbevent)
        VERIFY(CloseHandle(evt.m_copydbevent));

    if(m_hcopythread)
	    VERIFY(CloseHandle(m_hcopythread));

    //rebuilding Target DB to which Db is copyied 
	//SpecificDBRebuild((wyChar*)evt.m_copydb->m_targetdb.GetString());

    //wnd = GetActiveWin();             
    SetGroupProcess(wnd, wyFalse);

	if(pGlobals->m_isautocomplete == wyTrue)
		RebuildACTags_SpecificDB(*m_tgtinfo, (wyChar*)m_targetdb.GetString());

    FreeExportData();

    EnableDlgWindows(wyTrue);
    m_copying = wyFalse;

	EnableDisable(IDC_STRUC);

	//set the focus for 'Escape' key to cancel the dialog
	SetFocus(m_hwnddlg);
	SetCursor(LoadCursor(NULL, IDC_ARROW));

    return;
}

unsigned __stdcall 
CopyDatabase::ExecuteCopyThread(LPVOID lpparam)
{
    wyBool  ret;
    COPYDB  *copydb = (COPYDB*)lpparam;
	wyBool  flag = wyFalse, isdone = wyFalse;
	wyString	msg;
	
	ret = copydb->m_copydb->ChangeTargetDB();

	if(ret == wyFalse)
    {
        copydb->m_copydb->RemoveInstances();
		goto close;
    }
	
	// if enable Transaction support (preference option)is selected and if it is not HTTP tunnel then execute 
	// start transaction by setting autocommit = 0
	if(IsStartTransactionEnable() == wyTrue  && copydb->m_copydb->m_newtargettunnel->IsTunnel() == wyFalse)
	{
		StartTransaction(copydb->m_copydb->m_newtargettunnel, copydb->m_copydb->m_newtargetmysql);
		flag = wyTrue;
	}

    ret = copydb->m_copydb->ExportData(CopyDatabase::UpdateGui, (void*)copydb->m_lpParam);
	
	//For working 'cancel'
	//SetFocus(copydb->m_copydb->m_hwnddlg);

	//if enable Transaction support (preference option)is selected then execute commit query
	if(flag == wyTrue)
		EndTransaction(copydb->m_copydb->m_newtargettunnel, copydb->m_copydb->m_newtargetmysql);	
	
    copydb->m_copydb->RemoveInstances();

    //If user pressed on 'Stop'
	if(ret == wyTrue && copydb->m_copydb && copydb->m_copydb->IsCopyStopped() == wyTrue)
    {
        //Shows number of rows copied
		msg.Sprintf(_("Aborted by user "));
		SendMessage(copydb->m_lpParam->m_hwndmsg, WM_SETTEXT, 0, (LPARAM)msg.GetAsWideChar());
    }
	else if(ret == wyTrue && copydb->m_copydb && copydb->m_copydb->m_isobjecttoexport == wyTrue)
    {
		//Shows number of rows copied
		msg.Sprintf(_("Copied successfully "));
		SendMessage(copydb->m_lpParam->m_hwndmsg, WM_SETTEXT, 0, (LPARAM)msg.GetAsWideChar());
			
		isdone = wyTrue;		
	}

	//if parent folder selected and press copy but if folder doesn;t have any child
	else if(ret == wyTrue && copydb->m_copydb && copydb->m_copydb->m_isobjecttoexport == wyFalse)
    {
        SendMessage(copydb->m_lpParam->m_hwndmsg, WM_SETTEXT, 0, 
                                        (LPARAM)_(L"There was nothing to be copied"));

		isdone = wyTrue;
		
	}

    else
    {
		
		//Shows number of rows copied
		msg.Sprintf(_("Error occured while copying "));

		if( GetForegroundWindow() != copydb->m_lpParam->m_hwnddlg )
			FlashWindow(pGlobals->m_pcmainwin->m_hwndmain, TRUE);

		SendMessage(copydb->m_lpParam->m_hwndmsg, WM_SETTEXT, 0, (LPARAM)msg.GetAsWideChar());

		//SetFocus(copydb->m_copydb->m_hwnddlg);		
    }

	if(copydb->m_copydb->m_summary.GetLength())
		EnableWindow(GetDlgItem(copydb->m_lpParam->m_hwnddlg, IDC_SUMMARY), wyTrue);
	
	
	if( GetForegroundWindow() != copydb->m_lpParam->m_hwnddlg )
		FlashWindow(pGlobals->m_pcmainwin->m_hwndmain, TRUE);


	//If it is succesful then Close button is changed to Done
	ShowWindow(GetDlgItem(copydb->m_lpParam->m_hwnddlg, IDCANCEL), !isdone);
	ShowWindow(GetDlgItem(copydb->m_lpParam->m_hwnddlg, IDDONE ), isdone);
	EnableWindow(GetDlgItem(copydb->m_lpParam->m_hwnddlg, IDDONE ), isdone);
	
	//Flag sets for prompting the Message box.
	if(isdone == wyTrue && copydb->m_copydb)
	{		
		if(copydb->m_copydb->m_isstoredpgrms == wyTrue && copydb->m_copydb->m_srcdb.Compare(copydb->m_copydb->m_targetdb) != 0)
			copydb->m_copydb->m_ispromtstorepgrmmessage = wyTrue;			
	}
	
close:
    SetEvent(copydb->m_copydbevent);

    return 1;
}

void 
CopyDatabase::RemoveInstances()
{
	RemoveSourceInstances();
    RemoveTargetInstances();
}

void 
CopyDatabase::RemoveSourceInstances()
{
	if(m_newsrcmysql)
		m_newsrctunnel->mysql_close(m_newsrcmysql);

	if(m_srcprocess != INVALID_HANDLE_VALUE)
	{
		//If SSH connection then close handles at end
		if(m_srcinfo->m_isssh == wyTrue)
			OnExitSSHConnection(&m_srcsshpipehandles);

		if(m_srcinfo->m_sshsocket)
			closesocket(m_srcinfo->m_sshsocket);
           
		m_srcinfo->m_sshsocket = NULL;
			
		VERIFY(TerminateProcess(m_srcprocess, 1));
	}

	if(m_newsrctunnel)
		delete m_newsrctunnel;
}

void 
CopyDatabase::RemoveTargetInstances()
{
    if(m_newtargetmysql)
		m_newtargettunnel->mysql_close(m_newtargetmysql);

	if(m_tgtprocess != INVALID_HANDLE_VALUE)
	{
		//If SSH connection then close handles at end
		if(m_tgtinfo->m_isssh == wyTrue)
			OnExitSSHConnection(&m_tgtsshpipehandles);

		if(m_tgtinfo->m_sshsocket)
			closesocket(m_tgtinfo->m_sshsocket);
           
		m_tgtinfo->m_sshsocket = NULL;
			
		VERIFY(TerminateProcess(m_tgtprocess, 1));
	}

	if(m_newtargettunnel)
		delete m_newtargettunnel;
}

void 
CopyDatabase::OnWmCommand(HWND hwnd, WPARAM wparam)
{
	CShowInfo	csi;

    switch(LOWORD(wparam))
	{
	case IDOK:
        if(m_copying == wyTrue)
        {
            StopCopy();
            SetWindowText(GetDlgItem(m_hwnddlg, IDC_MESSAGE), _(L"Aborting copy..."));
            EnableWindow(GetDlgItem(m_hwnddlg, IDOK), FALSE);
			SetFocus(m_hwnddlg);
        }
        else
        {
            m_copying = wyTrue;
		    UpdateWindow(hwnd);
		    PostMessage(hwnd, UM_COPYDATABASE, NULL, NULL);
        }
		break;

	case IDC_SELALL2:
		SendMessage(GetDlgItem(hwnd,IDC_TABLES),LB_SETSEL,TRUE,-1);
		break;

	case IDC_UNSELALL2:
		SendMessage(GetDlgItem(hwnd,IDC_TABLES),LB_SETSEL,FALSE,-1);
		break;

	case IDDONE:
	case IDCANCEL:
		yog_enddialog(hwnd, 0);
		break;
	case IDC_CPY_DEFINER:
		if(SendMessage(GetDlgItem(hwnd, IDC_CPY_DEFINER), BM_GETCHECK, 0, 0) == BST_CHECKED)
			m_isremdefiner = wyTrue;
		else
			m_isremdefiner = wyFalse;
		//m_isremdefiner = m_isremdefiner?wyFalse:wyTrue;
		break;
    case IDC_CPY_ASCTRIGGERS:
		if(SendMessage(GetDlgItem(hwnd, IDC_CPY_ASCTRIGGERS), BM_GETCHECK, 0, 0) == BST_CHECKED)
		{
		m_iscopytrigger=wyTrue;
		SetCursor(LoadCursor(NULL, IDC_WAIT));
		ExpandAndFillTriggers(hwnd);
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		}
		else
          m_iscopytrigger=wyFalse;
		break;
	case IDC_SOURCEDB:
		if(HIWORD(wparam) == CBN_SELCHANGE)
			OnCBSelChange(hwnd);
		break;
	case IDC_SOURCEDB2:
		{
			if(HIWORD(wparam) == CBN_EDITCHANGE)
			OnCBEditChangeDb(hwnd);
		}
		break;

	// To disable bulkinsert when only structure is checked. 
	case IDC_STRUC:
	case IDC_STRUCDATA:
		EnableDisable(LOWORD(wparam));
		break;

	case IDC_SUMMARY:
		csi.ShowInfo(hwnd, NULL, NULL, _(" Copy Database Summary"), (wyChar *)m_summary.GetString());
		break;

	}
    return;
}

void 
	CopyDatabase::OnCBEditChangeDb(HWND hwnd){

	wyWChar     str[70] = {0},*temp=NULL;
    wyInt32     len = -1;
	int i, status=0,index;	//status flag is set when atleast one match found
	wyWChar     lbStr[70] = {0};
	GetWindowText(m_hwndcombodb, str, 65);
	len=wcslen(str);
		if(len)
			{
					 /*if(len==1)
						 SendMessage(hdb, CB_SHOWDROPDOWN, FALSE, NULL);*/

				for(i=0;i<m_countdb;i++)
					{	 
						if(m_dblist[i].m_dbname.GetLength()==0)
							continue;
						temp = StrStrI(m_dblist[i].m_dbname.GetAsWideChar(),str);
						if(temp)
							status=1;
							

						if(temp && m_dblist[i].m_dropdown==wyFalse)
						{
							index=SendMessage(m_hwndcombodb, CB_ADDSTRING, 0,(LPARAM)m_dblist[i].m_dbname.GetAsWideChar());
							m_dblist[i].m_dropdown=wyTrue;
							
						}

						 if(!temp &&  m_dblist[i].m_dropdown==wyTrue)
						 {
							 int index_delete=SendMessage(m_hwndcombodb,CB_FINDSTRINGEXACT,-1,(LPARAM)m_dblist[i].m_dbname.GetAsWideChar());
							 SendMessage(m_hwndcombodb, CB_DELETESTRING, index_delete,NULL);
							 m_dblist[i].m_dropdown=wyFalse;
						}
				   }
					 
				}
				 if(!len || status==0)
				 {
					 if(status==0)
							SendMessage(m_hwndcombodb, CB_SHOWDROPDOWN, FALSE, NULL);
					 for(i=0;i<m_countdb;i++)
					 {
						 if( m_dblist[i].m_dbname.GetLength()==0)
							continue;
						 
						SetCursor(LoadCursor(NULL, IDC_ARROW));
						if( m_dblist[i].m_dropdown==wyFalse)
						{
						 index=SendMessage(m_hwndcombodb, CB_ADDSTRING, 0,(LPARAM)m_dblist[i].m_dbname.GetAsWideChar());
						 m_dblist[i].m_dropdown=wyTrue;
						}
					 }
					 if(!len)
						SendMessage(m_hwndcombodb, CB_SHOWDROPDOWN, TRUE, NULL);
				        
				}
				 else
				{
					
				 if(SendMessage(m_hwndcombodb, CB_GETDROPPEDSTATE, NULL, NULL) == FALSE)
						{
							
							SendMessage(m_hwndcombodb, CB_SHOWDROPDOWN, TRUE, NULL);
							SetCursor(LoadCursor(NULL, IDC_ARROW));
							int index = SendMessage(m_hwndcombodb, CB_FINDSTRING, -1,(LPARAM)str);
			   if(index!=CB_ERR){
			   SendMessage(m_hwndcombodb, CB_GETLBTEXT, index, (LPARAM)lbStr);
			   SetWindowText(m_hwndcombodb, lbStr);
			   SendMessage(m_hwndcombodb, CB_SETEDITSEL, NULL, MAKELPARAM(wcslen(str),-1));
			   }

						}
			   
				 // select only auto-filled text so that user can continue typing
			  
              //SendMessage(hdb, CB_SETEDITSEL, NULL, MAKELPARAM(-1,0));
					
			  }

		wyInt32 width = SetComboWidth(m_hwndcombodb);
		SendMessage(m_hwndcombodb, CB_SETDROPPEDWIDTH, width + 50, 0);

}

void 
CopyDatabase::OnCBSelChange(HWND hwnd)
{
	HWND  treehandle;

	if(m_table.GetLength())
		m_table.Clear();

	treehandle = GetDlgItem(hwnd, IDC_TREE);
	if(treehandle == NULL)
		return;
    CheckTargetDB(hwnd);
	VERIFY(SendMessage(treehandle, WM_SETREDRAW, FALSE, 0));
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	
	HandleTreeviewOnComboChange(wyFalse, wyTrue);

	//SelectTreeviewParentItems(treehandle);

	SetCursor(LoadCursor(NULL, IDC_ARROW));
	VERIFY(SendMessage(GetDlgItem(hwnd, IDC_TREE), WM_SETREDRAW, TRUE, 0));
}
void CopyDatabase::ExpandAndFillTriggers(HWND hwnd)
{
	HWND htree;
	HTREEITEM hrootitem;

	VERIFY(htree = GetDlgItem(hwnd, IDC_TREE));
	hrootitem = GetObjectsHtreeItem(htree, TXT_TRIGGERS);
	
	if(m_selalltables==wyTrue)
	{
		
		TreeView_SetCheckState(htree, hrootitem, 1);
		CheckAllChilds(htree,hrootitem,wyTrue);
		TreeView_Expand(htree, hrootitem, TVE_EXPAND);

	}
	else
	{
	TreeView_Expand(htree, hrootitem, TVE_EXPAND);
	Filltriggers(htree);
	
	}
}

void CopyDatabase::CheckAllChilds(HWND htreeitem,HTREEITEM hrootitem,wyBool check)
{
HTREEITEM hti;

for(hti	= TreeView_GetChild(htreeitem,hrootitem); hti; hti = TreeView_GetNextSibling(htreeitem, hti))
	{
		if(check==wyTrue)
		{TreeView_SetCheckState(htreeitem, hti, 1);}
		else
		{ TreeView_SetCheckState(htreeitem, hti, 0);}
     

}



}

void CopyDatabase::Filltriggers(HWND htree)
{

HTREEITEM htable,hti;
TVITEM		tvi = {0};
	wyWChar     temptext[SIZE_512] = {0};
	wyInt32     checkstate = 0;
	wyString    table_name;
htable = TreeView_GetRoot(htree);

for(hti	= TreeView_GetChild(htree,htable); hti; hti = TreeView_GetNextSibling(htree, hti))
	{
		// first see whether the item is checked or not.
		checkstate = TreeView_GetCheckState(htree, hti);
		if(!checkstate)
       		continue;
	
		tvi.mask		= TVIF_PARAM |TVIF_HANDLE | TVIF_TEXT;
		tvi.hItem		= hti;
		tvi.pszText		= temptext;
		tvi.cchTextMax	= 512 - 1;

		TreeView_GetItem(htree, &tvi);
		//now find all triggers and check them in GUI
		table_name.SetAs(tvi.pszText);
		FindTrigerAndCheck(htree,&table_name,wyFalse);
		
	}



}

void CopyDatabase::FindTrigerAndCheck(HWND htree, wyString *tablename, wyBool remove)
{

	wyString    myrowstr, query;
	MYSQL_RES	*myres = NULL;
	MYSQL_ROW	myrow;
	wyBool		ismysql41 = ((GetActiveWin())->m_ismysql41);//, ret
	MDIWindow   *wnd;
	wyBool		iscollate = wyFalse;
	VERIFY(wnd = GetActiveWin());
	if(!wnd)
		return;

	//we use collate utf8_bin only if lower_case_table_names is 0 and lower_case_file_system is OFF
	if(GetmySQLCaseVariable(wnd) == 0)
		if(!IsLowercaseFS(wnd))
			iscollate = wyTrue;
	
	if(iscollate)
		/*query.Sprintf("select `EVENT_NAME` from `INFORMATION_SCHEMA`.`EVENTS` where BINARY `EVENT_SCHEMA` = '%s'", 
					m_srcdb.GetString());*/
	query.Sprintf("SELECT TRIGGER_NAME FROM information_schema.`TRIGGERS` WHERE BINARY EVENT_OBJECT_SCHEMA='%s' AND BINARY EVENT_OBJECT_TABLE='%s'", 
					m_srcdb.GetString(), tablename->GetString());
	else
		query.Sprintf("SELECT TRIGGER_NAME FROM information_schema.`TRIGGERS` WHERE EVENT_OBJECT_SCHEMA='%s' AND EVENT_OBJECT_TABLE='%s'", 
					m_srcdb.GetString(),tablename->GetString());
	myres = ExecuteAndGetResult(wnd, m_srctunnel, m_srcmysql, query);
	if(!myres)
	{
		ShowMySQLError(m_hwnddlg, m_srctunnel, m_srcmysql, query.GetString());
		return;
	}

	while(myrow = m_srctunnel->mysql_fetch_row(myres))
	{

		myrowstr.SetAs(myrow[0], ismysql41);
		 checkcurrenttrigger(htree,&myrowstr, remove);
		

	}
			

	m_srctunnel->mysql_free_result(myres);

}
void CopyDatabase:: checkcurrenttrigger(HWND htreeitem, wyString *triggername, wyBool remove)
{
   HTREEITEM htrigger,hti;
   
    TVITEM		tvi = {0};
	wyWChar     temptext[SIZE_512] = {0};
	wyString    table_name;

htrigger = GetObjectsHtreeItem(htreeitem, TXT_TRIGGERS);
for(hti	= TreeView_GetChild(htreeitem,htrigger); hti; hti = TreeView_GetNextSibling(htreeitem, hti))
	{
		//add or remove triggers one by one.
	
		tvi.mask		= TVIF_PARAM |TVIF_HANDLE | TVIF_TEXT;
		tvi.hItem		= hti;
		tvi.pszText		= temptext;
		tvi.cchTextMax	= 512 - 1;

		TreeView_GetItem(htreeitem, &tvi);
		//now find all triggers and check them in GUI
		if(wcscmp(temptext, triggername->GetAsWideChar()) == 0)
		{
		if(remove==wyFalse)
		{TreeView_SetCheckState(htreeitem, hti, true);}
		else
		{ TreeView_SetCheckState(htreeitem, hti, false );}
		
		}
		
	}



}

// Function fills up the combo with diff database from diff connection.
wyBool
CopyDatabase::AddInitData()
{
	HWND				hwndtree = NULL;
	DBListBuilder	    cdb;
    wyInt32             ret,ncursel;
	MDIWindow           *wnd = GetActiveWin();
	wyString            activevalue,activevalue2, srcdetail;
	wyBool              flag = wyFalse;
	LPDIFFCOMBOITEM     lpdiff;
	srcdetail.Sprintf("%s - %s", wnd->m_title.GetString(), m_srcdb.GetString());

	cdb.GetSVs(m_hwndcombo);	
	SendMessage(GetDlgItem(m_hwnddlg, IDC_TARGETDB), WM_SETTEXT, 0,(LPARAM)srcdetail.GetAsWideChar());

	m_p->Add(m_hwnddlg, IDC_DROP, "DropTable", "0", CHECKBOX);
	m_p->Add(m_hwnddlg, IDC_STRUCDATA, "StrucData", "1", CHECKBOX);
	m_p->Add(m_hwnddlg, IDC_STRUC, "StrucOnly", "0", CHECKBOX);
	m_p->Add(m_hwnddlg, IDC_CPY_BULKINSERT, "CpyBulkInsert", "1", CHECKBOX);
	m_p->Add(m_hwnddlg, IDC_CPY_DEFINER, "RemoveDefiner", "0", CHECKBOX);
	m_p->Add(m_hwnddlg, IDC_CPY_ASCTRIGGERS, "Coppyasctriggers", "0", CHECKBOX);
    if(wnd)
        m_srcinfo = &wnd->m_conninfo;
	
	m_issrc5xobjects = IsMySQL5010(m_srctunnel, m_srcmysql);
	m_issrceventsupport = IsMySQL516(m_srctunnel, m_srcmysql);	

	/* now we remove the the current db that is being exported from combobox so that we dont accidently
		drop the data */
	/* starting from 4.1 BETA 4 we use connection name for unique values */

	//if active connection is same as source connection then remove source db from databases combo
	activevalue.Sprintf("%s", wnd->m_title.GetString());
	activevalue2.Sprintf("%s", m_srcdb.GetString());
	//populate dbs for first connection in the list
	//ncursel = SendMessage(m_hwndcombo, CB_GETCURSEL, 0, 0);
	ncursel = 0;
	VERIFY(lpdiff =(LPDIFFCOMBOITEM)SendMessage(m_hwndcombo, CB_GETITEMDATA, ncursel, 0));

	DBListBuilder::GetDBFromActiveWinscopydb(lpdiff->wnd->m_hwnd, (LPARAM)m_hwndcombodb);
	if(lpdiff->wnd == wnd )
	{
		ret = SendMessage(m_hwndcombodb, CB_FINDSTRINGEXACT, -1,(LPARAM)activevalue2.GetAsWideChar());
		if(ret != CB_ERR)
			SendMessage(m_hwndcombodb, CB_DELETESTRING, ret, 0);
	}
	FillDbList();
	//delete lpdiff;
	///If combo list is empty then disable the combo box
	//11.52 we do not disable combo box on single connection since user can create a new db
	ret = SendMessage(m_hwndcombo, CB_GETCOUNT, (WPARAM)0,(LPARAM)0);
	if(!ret)
	{
		EnableWindow(m_hwndcombo, FALSE);
		EnableWindow(m_hwndcombodb, FALSE);
	}

	else
	{
		EnableWindow(m_hwndcombo, TRUE);
		EnableWindow(m_hwndcombodb, TRUE);
	}
	

	m_p->Add(m_hwnddlg, IDC_SOURCEDB, "Target", "", COMBOBOX_P);

	// if user has only one connection and having only one db
	if(CheckTargetDB(m_hwnddlg) == wyFalse)
		flag = wyTrue;
        
	// now add all the tables to the tree.
	VERIFY(hwndtree = GetDlgItem(m_hwnddlg, IDC_TREE));	
	if(!hwndtree)
		return wyFalse;

	// To disable bulkinsert when only structure is checked.     
	EnableDisable(IDC_STRUC);

	EnableWindow(GetDlgItem(m_hwnddlg, IDC_SUMMARY), wyFalse);
	
    if(SendMessage(GetDlgItem(m_hwnddlg, IDC_CPY_DEFINER), BM_GETCHECK, 0, 0) == BST_CHECKED)
		m_isremdefiner = wyTrue;
	else
		m_isremdefiner = wyFalse;
	SendMessage(GetDlgItem(m_hwnddlg, IDC_CPY_ASCTRIGGERS), BM_SETCHECK, BST_UNCHECKED, 0);
	m_iscopytrigger=wyFalse;
	//Initialise the treeview
	InitializeTree(m_hwnddlg, m_srcdb.GetString(), flag);

	GetClientRect(m_hwnddlg, &m_dlgrect);
	GetWindowRect(m_hwnddlg, &m_wndrect);
		
	//set the initial position of the dialog
	SetWindowPositionFromINI(m_hwnddlg, COPYDATABASE_SECTION, 
	m_wndrect.right - m_wndrect.left, 
	m_wndrect.bottom - m_wndrect.top);
	
	GetCtrlRects();
	PositionCtrls();

	// set the width of the combos
	
	wyInt32 width = SetComboWidth(GetDlgItem(m_hwnddlg, IDC_SOURCEDB));
	SendMessage(GetDlgItem(m_hwnddlg, IDC_SOURCEDB), CB_SETDROPPEDWIDTH, width + 50, 0);

	width = SetComboWidth(GetDlgItem(m_hwnddlg, IDC_SOURCEDB2));
	SendMessage(GetDlgItem(m_hwnddlg, IDC_SOURCEDB2), CB_SETDROPPEDWIDTH, width + 50, 0);


	return wyTrue;
}

//Function adds table(s), view(s), procedure(s), function(s) and trigger(s)of the selected database to the tree control.
wyBool
CopyDatabase::InitializeTree(HWND hwnd, const wyChar * dbname, wyBool issingledb)
{
	HWND			hwndtree, hobtree;
	MDIWindow		*wnd;
    HTREEITEM       root, temp, hti;
    wyInt32         image;
    TVITEM          tviOB;
	    
	VERIFY(wnd = GetActiveWin());
	if(!wnd)
		return wyFalse;

	VERIFY(hwndtree = GetDlgItem(hwnd, IDC_TREE));
	
	InsertTableParent(hwndtree, TVI_ROOT);
	HandleTreeviewOnComboChange(issingledb);

	//for copying single table
	if(m_table.GetLength())// && cbselchange == wyFalse)
    {
        root = TreeView_GetRoot(hwndtree);
        SelectTreeviewParentItems(hwndtree, root);
        TreeView_Expand(hwndtree, root, TVE_EXPAND);
    }
    //select the table objects only
    else if(m_selalltables == wyTrue)
    {
        root = TreeView_GetRoot(hwndtree);
        SelectTreeviewParentItems(hwndtree, root);
        TreeView_Expand(hwndtree, root, TVE_EXPAND);
    }
	//Selects parent for all objects
	else
    {
        root = TreeView_GetRoot(hwndtree);
		temp = root;
        SelectTreeviewParentItems(hwndtree);

        hobtree = wnd->m_pcqueryobject->m_hwnd;

        if(hobtree)
        {
            hti = TreeView_GetSelection(hobtree);
            image = wnd->m_pcqueryobject->GetSelectionImage();
            while(image != NDATABASE)
            {
                hti = TreeView_GetParent(hobtree, hti);
                image = GetItemImage(hobtree, hti);
            }
            hti = TreeView_GetChild(hobtree, hti);

            while(root && hti)
            {
                tviOB.hItem = hti;
                tviOB.mask = TVIF_PARAM;
                TreeView_GetItem(hobtree, &tviOB);
                if(tviOB.lParam && wcslen(((OBDltElementParam *)tviOB.lParam)->m_filterText))
                {
                    TreeView_Expand(hwndtree, root, TVE_EXPAND);
                }
                root = TreeView_GetNextSibling(hwndtree, root);
                hti = TreeView_GetNextSibling(hobtree, hti);
            }
        }
        TreeView_SelectItem(hwndtree, temp);
    }	
	return wyTrue;
}

///When expands treeview node
wyBool
CopyDatabase::OnItemExpandingHelper(HWND hwnd , LPNMTREEVIEW pnmtv)
{
	TVITEM  tvi, tviOB;
	TREEVIEWPARAMS	treeviewparam = {0};
    
    MDIWindow   *wnd = GetActiveWin();
    HWND        hobtree = wnd->m_pcqueryobject->m_hwnd;
    wyInt32     image = 0, image1 = 0;
    HTREEITEM   hti = NULL,selected_hti = NULL;
    wyWChar     filterText[70], str[70];
    wyBool      atleastOneItemChecked = wyFalse;
	tvi = pnmtv->itemNew;

	treeviewparam.database = m_srcdb.GetAsWideChar();
	treeviewparam.hwnd = hwnd;
	treeviewparam.isopentables = wyFalse;
	treeviewparam.isrefresh = wyTrue;
	treeviewparam.issingletable = wyFalse;
	treeviewparam.mysql = m_srcmysql;
	treeviewparam.tunnel = m_srctunnel;
	treeviewparam.tvi = &tvi;	
	treeviewparam.checkboxstate = wyTrue;

	OnItemExpanding(treeviewparam);
    
    image = wnd->m_pcqueryobject->GetSelectionImage();
    image1 = GetItemImage(hwnd, tvi.hItem);

    if(!(tvi.state & TVIS_EXPANDEDONCE) && ((m_selalltables == wyTrue && image1 == NTABLES) || (m_selalltables == wyFalse)))
    {
        if(!m_table.GetLength())
        {
            hti = TreeView_GetSelection(hobtree);
            
            switch(image)
            {
            case NTABLES:
            case NEVENTS:
            case NSP:
            case NFUNC:
            case NTRIGGER:
            case NVIEWS:
                hti = TreeView_GetParent(hobtree, hti);
                break;
            case NTABLE:
            case NSPITEM:
            case NEVENTITEM:
            case NFUNCITEM:
            case NTRIGGERITEM:
            case NVIEWSITEM:
                hti = TreeView_GetParent(hobtree, hti);
                hti = TreeView_GetParent(hobtree, hti);
                break;
            case NFOLDER:
                hti = TreeView_GetParent(hobtree, hti);
                hti = TreeView_GetParent(hobtree, hti);
                hti = TreeView_GetParent(hobtree, hti);
                break;
            case NPRIMARYKEY:
            case NCOLUMN:
	        case NPRIMARYINDEX:
	        case NINDEX:
	            hti = TreeView_GetParent(hobtree, hti);
                hti = TreeView_GetParent(hobtree, hti);
                hti = TreeView_GetParent(hobtree, hti);
                hti = TreeView_GetParent(hobtree, hti);
                break;    
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
                    TreeView_SetCheckState(hwnd, hti, 1);
					selected_hti=hti;
                    atleastOneItemChecked = wyTrue;


                }
                else
                {
                    TreeView_SetCheckState(hwnd, hti, 0);
                }
            }
            if(atleastOneItemChecked)
            {
				if(image1 == NTABLES)
				{
					m_dontnotify = wyTrue;

					//Fixed:http://code.google.com/p/sqlyog/issues/detail?id=1919
					//TreeView_SelectItem(hwnd,selected_hti);
					TreeView_SelectSetFirstVisible(hwnd,selected_hti);
				}
                TreeView_SetCheckState(hwnd, tvi.hItem, 1);
            }
            else
            {
                TreeView_SetCheckState(hwnd, tvi.hItem, 0);
            }
        }
    }
	return wyFalse;
}

wyBool
CopyDatabase::HandleTreeviewOnComboChange(wyBool issinglesrcdb, wyBool cbselchange)
{
	HWND		hwndtree = NULL;
	HTREEITEM	htreeitem = NULL;
	wyBool      iscopytable = wyFalse;
	wyString    query;
	
	VERIFY(hwndtree = GetDlgItem(m_hwnddlg, IDC_TREE));	
	if(!hwndtree)
		return wyFalse;
	
    // if we have only one connection and connection having single db
	if(issinglesrcdb == wyTrue)
	{
		m_istrg5xobjects = wyTrue;
		m_istrgeventsupports = wyTrue;			
	}

	//For versions less tahn 5.1, delete all parent folders from view,(view, sp, fun, trig, etc)
	if(m_issrc5xobjects == wyFalse || m_istrg5xobjects == wyFalse)
	{
        if(cbselchange == wyTrue && (htreeitem = GetObjectsHtreeItem(hwndtree, TXT_VIEWS, wyTrue)))
			Remove5XRoutines(htreeitem, hwndtree);
		return wyTrue;
	}
	if(cbselchange == wyTrue && (htreeitem = GetObjectsHtreeItem(hwndtree, TXT_VIEWS, wyTrue)))
		goto end;

	// insert all views into Viewsfolder treeview
	if(InsertDatabaseObjects(hwndtree, TXT_VIEWS, query, NVIEWS, iscopytable) == wyFalse)
		return wyFalse;	
	
	// insert all procedures into procedures folder treeview
    if(InsertDatabaseObjects(hwndtree, TXT_PROCEDURES, query, NSP, iscopytable) == wyFalse)
		return wyFalse;
	
	// insert all funs into Funsfolder treeview
    if(InsertDatabaseObjects(hwndtree, TXT_FUNCTIONS, query, NFUNC, iscopytable) == wyFalse)
		return wyFalse;
	
    if(InsertDatabaseObjects(hwndtree, TXT_TRIGGERS, query, NTRIGGER, iscopytable) == wyFalse)
		return wyFalse;
end:
	// if mysql version is above 5.16 then only it is supporting events
	if(m_issrceventsupport == wyFalse || m_istrgeventsupports == wyFalse)
	{
        if(cbselchange == wyTrue && (htreeitem = GetObjectsHtreeItem(hwndtree, TXT_EVENTS, wyTrue)))
			TreeView_DeleteItem(hwndtree, htreeitem);
		return wyTrue;
	}

    if(cbselchange == wyTrue && (htreeitem = GetObjectsHtreeItem(hwndtree, TXT_EVENTS, wyTrue)))
		return wyTrue;
		
    if(InsertDatabaseObjects(hwndtree, TXT_EVENTS, query, NEVENTS, iscopytable) == wyFalse)
		return wyFalse;

	return wyTrue;
}

//Select all parent folders selected by default
wyBool
CopyDatabase::SelectTreeviewParentItems(HWND hwndtree, HTREEITEM item)
{
	HTREEITEM	htreeparent;
	wyInt32		state = 0, checkstate = 0;

	//Select all parent folderes by default
	htreeparent = TreeView_GetRoot(hwndtree);
	for(htreeparent; htreeparent; htreeparent = TreeView_GetNextSibling(hwndtree, htreeparent))
	{
		state = TreeView_GetItemState(hwndtree, htreeparent, TVIS_EXPANDEDONCE);
		checkstate = TreeView_GetCheckState(hwndtree, htreeparent);

		if(!checkstate && !(state & TVIS_EXPANDEDONCE))
			TreeView_SetCheckState(hwndtree, htreeparent, 1);
        
        //if the current root is same as the passed item, then break (for checking only tables)
        if(htreeparent == item)
            break;
	}

	return wyTrue;
}

//Inserts the database objects(only parent folder) depends on version 
wyBool
CopyDatabase::InsertDatabaseObjects(HWND hwndtree,  wyWChar *object, wyString &query,wyInt32 imagetype, wyBool iscopytable)
{	
	HTREEITEM	htreeitem = TVI_ROOT;
	
	if(wcsicmp(object, TXT_VIEWS) == 0)
		InsertViews(hwndtree, htreeitem);

    if(wcsicmp(object, TXT_PROCEDURES) == 0)
		InsertStoredProcs(hwndtree, htreeitem);

    if(wcsicmp(object, TXT_FUNCTIONS) == 0)
		InsertFunctions(hwndtree, htreeitem);

    if(wcsicmp(object, TXT_TRIGGERS) == 0)
		InsertTriggers(hwndtree, htreeitem);

    if(wcsicmp(object, TXT_EVENTS)== 0)
		InsertEvents(hwndtree, htreeitem);		

	return wyTrue;
}

void
CopyDatabase::Remove5XRoutines(HTREEITEM hitem, HWND hwndtree)
{
	HTREEITEM  hitemtemp = NULL;
	wyInt32    count = 0;
	
	for(; count < 5 ; count++)
	{
		hitemtemp = TreeView_GetNextItem(hwndtree, hitem, TVGN_NEXT);
		TreeView_DeleteItem(hwndtree, hitem);
		if(hitemtemp == NULL)
			break;
		hitem = hitemtemp;
	}
}

wyBool
CopyDatabase::FreeComboParam()
{
	LPDIFFCOMBOITEM     lpdiff;
	wyInt32             count, i;

	VERIFY((count = SendMessage(GetDlgItem(m_hwnddlg, IDC_SOURCEDB), CB_GETCOUNT, 0, 0))!= CB_ERR);
	
	for(i = 0; i < count; i++)
    {
		VERIFY(lpdiff =(LPDIFFCOMBOITEM)SendMessage(m_hwndcombo, CB_GETITEMDATA, i, 0));
		delete lpdiff;
	}
	return wyTrue;
}

wyBool
CopyDatabase::FreedbComboParam()
{
	LPDIFFCOMBOITEM     lpdiff;
	wyInt32             count, i;

	VERIFY((count = SendMessage(GetDlgItem(m_hwnddlg, IDC_SOURCEDB2), CB_GETCOUNT, 0, 0))!= CB_ERR);
	
	for(i = 0; i < count; i++)
    {
		VERIFY(lpdiff =(LPDIFFCOMBOITEM)SendMessage(m_hwndcombodb, CB_GETITEMDATA, i, 0));
		delete lpdiff;
	}
	return wyTrue;
}

/* This is where the real game begins we start copying data */
// Function creates a new instance to the target server
// so that if the source and target are same we dont go out of sync
// because we use mysql_use_result

wyBool
CopyDatabase::GetTargetDatabase()
{
	LPDIFFCOMBOITEM	lpdiff;//,lpdiff2
	wyInt32         index;

	VERIFY((index = SendMessage(m_hwndcombo, CB_GETCURSEL, 0, 0))!= CB_ERR);

	if(index==CB_ERR)
		return wyFalse;
	VERIFY(lpdiff =(LPDIFFCOMBOITEM)SendMessage(m_hwndcombo, CB_GETITEMDATA, index, 0));

	//VERIFY((index = SendMessage(m_hwndcombodb, CB_GETCURSEL, 0, 0))!= CB_ERR);

	//VERIFY(lpdiff2 =(LPDIFFCOMBOITEM)SendMessage(m_hwndcombodb, CB_GETITEMDATA, index, 0));


   // m_targetdb.SetAs(lpdiff2->szDB);
	VERIFY(m_targetmysql = lpdiff->mysql);
	VERIFY(m_targettunnel = lpdiff->tunnel);
	VERIFY(m_tgtinfo = lpdiff->info);

	return wyTrue;
}

HTREEITEM
CopyDatabase::GetObjectsHtreeItem(HWND hwnd, wyWChar *object, wyBool isfindroot)
{
	TVITEM		tvi = {0};
	wyWChar     temptext[SIZE_512] = {0};
	wyInt32     checkstate = 0;
	HTREEITEM   hrootitem;

	for(hrootitem	= TreeView_GetRoot(hwnd); hrootitem; hrootitem = TreeView_GetNextSibling(hwnd, hrootitem))
	{
		// first see whether the item is checked or not.
		checkstate = TreeView_GetCheckState(hwnd, hrootitem);
		if(!checkstate && (isfindroot == wyFalse) && (m_iscopytrigger==wyFalse))
       		continue;
	
		tvi.mask		= TVIF_PARAM |TVIF_HANDLE | TVIF_TEXT;
		tvi.hItem		= hrootitem;
		tvi.pszText		= temptext;
		tvi.cchTextMax	= 512 - 1;

		TreeView_GetItem(hwnd, &tvi);

		if(wcscmp(temptext, object) == 0)
			break;
	}

	return hrootitem;

}
wyBool
CopyDatabase::InitExportData(HWND hwndtree)
{
	wyString	tablestr, procstr;
	wyString    funcstr, viewstr, triggerstr;
	wyString    whichtree, text, strcreate;			
	
	if(SendMessage(GetDlgItem(m_hwnddlg, IDC_DROP), BM_GETCHECK, TRUE, 0))
		m_dropobjects = wyTrue;
	else 
		m_dropobjects = wyFalse;	
			
	// let us see if user wants data also.
	m_exportdata =(wyBool)SendMessage(GetDlgItem(m_hwnddlg, IDC_STRUCDATA), BM_GETCHECK, 0, 0);
    if(SendMessage(GetDlgItem(m_hwnddlg, IDC_CPY_DEFINER), BM_GETCHECK, 0, 0) == BST_CHECKED)
		m_isremdefiner = wyTrue;
	else
		m_isremdefiner = wyFalse;
	InitTargetServer();	
	SetNamesToUTF8();	

    return wyTrue;
}

void 
CopyDatabase::Delete(List *list)
{
	RelTableFldInfo  *temp, *elem;
	
	elem = (RelTableFldInfo*)list->GetFirst();
	while(elem)
	{
		temp = (RelTableFldInfo*)elem->m_next;
		elem->m_tablefld.Clear();
		list->Remove(elem);
		delete elem;
		elem = (RelTableFldInfo*)temp;
	}
}

//Initialse the table(s) to export
void 
CopyDatabase::HandleTables(HWND hwndtree, HTREEITEM hrootitem)
{
	wyInt32		checkstate, state;
	
	state = TreeView_GetItemState(hwndtree, hrootitem, TVIS_EXPANDEDONCE);
	checkstate = TreeView_GetCheckState(hwndtree, hrootitem);

	if(checkstate && !(state & TVIS_EXPANDEDONCE))
		HandleTablesOnParentNotExpanded(hwndtree);

	else if(checkstate && (state & TVIS_EXPANDEDONCE))
		HandleTablesOnParentExpanded(hwndtree, hrootitem);
}

//Gets the tables from treeview
wyBool		
CopyDatabase::HandleTablesOnParentExpanded(HWND hwndtree, HTREEITEM htreeitem)
{
	wyInt32			checkstate;
	HTREEITEM		htable;
	wyWChar			temptext[SIZE_512] = {0};
	TVITEM			tvi = {0};
	wyString		tablestr;
	RelTableFldInfo *tables = NULL;

	htable = TreeView_GetChild(hwndtree, htreeitem);

	if(htable)
	{
		if(m_isobjecttoexport == wyFalse)
			m_isobjecttoexport = wyTrue;
	}

	/* patch after 5.1, if the table is not selected we dont generate 
            any statement for it and we continue to next */
	for(; htable; htable = TreeView_GetNextSibling(hwndtree, htable))
	{
		checkstate = TreeView_GetCheckState(hwndtree, htable);
		if(!checkstate)
			continue;						
		/* get the correct handle and pointer */
		tvi.mask	= TVIF_PARAM | TVIF_IMAGE | TVIF_TEXT;
		tvi.hItem	= htable;
		tvi.pszText = temptext;
		tvi.cchTextMax = 512-1;

		VERIFY(TreeView_GetItem(hwndtree, &tvi));		
		tablestr.SetAs(temptext);
		tables = new RelTableFldInfo(tablestr.GetString());
		m_seltables.Insert(tables);
	}

	return wyTrue;
}

//Gets tables by executing query
wyBool		
CopyDatabase::HandleTablesOnParentNotExpanded(HWND hwndtree)
{
	wyString		query;
	wyString		myrowstr;
	MYSQL_RES		*myres = NULL;
	MYSQL_ROW		myrow;
	MDIWindow		*wnd = NULL;
	wyBool			ismysql41;
	RelTableFldInfo *tables = NULL;

	VERIFY(wnd = GetActiveWin());
	ismysql41 = wnd->m_ismysql41;

	PrepareShowTable(m_srctunnel, m_srcmysql,  m_srcdb, query);

	myres = ExecuteAndGetResult(wnd, m_srctunnel, m_srcmysql, query, wyTrue, wyFalse, wyFalse);
	if(!myres)
	{
		ShowMySQLError(m_hwnddlg, m_srctunnel, m_srcmysql, query.GetString());
		return wyFalse;
	}

	while(myrow = m_srctunnel->mysql_fetch_row(myres))
	{
		if(IsCopyStopped() == wyTrue)
		{
			m_srctunnel->mysql_free_result(myres);	
			return wyFalse;
		}

        myrowstr.SetAs(myrow[0], ismysql41);

		tables = new RelTableFldInfo(myrowstr.GetString());
		m_seltables.Insert(tables);

		if(m_isobjecttoexport == wyFalse)
			m_isobjecttoexport = wyTrue;
	}

	m_srctunnel->mysql_free_result(myres);	

	return wyTrue;
}

//Export views
wyBool 
CopyDatabase::ExportViews(HWND hwndtree, HTREEITEM hrootitem)
{
	wyInt32 checkstate = 0, state = 0;
	wyBool	ret = wyTrue;
		
	state = TreeView_GetItemState(hwndtree, hrootitem, TVIS_EXPANDEDONCE);
	checkstate = TreeView_GetCheckState(hwndtree, hrootitem);

	if(checkstate && !(state & TVIS_EXPANDEDONCE))
		ret = ExportViewsOnParentNotExpanded(hwndtree);

	else if(checkstate && (state & TVIS_EXPANDEDONCE))
		ret = ExportViewsOnParentExpanded(hwndtree, hrootitem);

	return ret;	
}

//Export views when parent folder is expanded
wyBool
CopyDatabase::ExportViewsOnParentExpanded(HWND hwndtree, HTREEITEM hrootitem)
{
	wyBool           ret = wyFalse;
	HTREEITEM        hviews;
	wyWChar          temptext[SIZE_512] = {0};
	TVITEM		     tvi = {0};
	wyString         view;	
	RelTableFldInfo  *views;
	wyInt32			 checkstate;

	hviews = TreeView_GetChild(hwndtree, hrootitem);

	if(hviews)
	{
		if(m_isobjecttoexport == wyFalse)
			m_isobjecttoexport = wyTrue;
	}

	//for displaying copying views...
	m_gui_routine((void*)m_gui_lparam, "views", 0, wyFalse, COPYSTART);	

	for(; hviews; hviews = TreeView_GetNextSibling(hwndtree, hviews))
	{
		checkstate = TreeView_GetCheckState(hwndtree, hviews);		
		if(checkstate == 0)
			continue;
		tvi.mask = TVIF_HANDLE | TVIF_TEXT;
		tvi.hItem = hviews;
		tvi.pszText = temptext;
		tvi.cchTextMax = SIZE_512 - 1;

		VERIFY(TreeView_GetItem(hwndtree, &tvi));
		view.SetAs(temptext);
		if(view.GetLength() != 0)
		{
			if(IsCopyStopped() == wyTrue)
				return wyFalse;
			if(CreateTemporaryTables(m_srcdb.GetString(), view.GetString()) == wyFalse)
				return wyFalse;				
		}

		RelTableFldInfo *viewnames = new RelTableFldInfo(view.GetString());
		m_selviews.Insert(viewnames);
	}
	views = (RelTableFldInfo*)m_selviews.GetFirst();
	while(views)
	{
		ret = ExportView(m_srcdb.GetString(), views->m_tablefld.GetString());
		if(ret == wyFalse)
			return wyFalse;

		views = (RelTableFldInfo*)views->m_next;

		if(m_isstoredpgrms == wyFalse)
			m_isstoredpgrms = wyTrue;
	}	

	m_gui_routine((void*)m_gui_lparam, "view(s)", m_selviews.GetCount(), wyFalse, OBJECTSCOPIED);	

	return ret;	
}

//Export views when parent folder is not expanded
wyBool
CopyDatabase::ExportViewsOnParentNotExpanded(HWND hwndtree)
{
	wyString		myrowstr, query;
	MYSQL_RES		*myres = NULL;
	MYSQL_ROW		myrow;
	wyBool			ismysql41, ret;
	MDIWindow		*wnd = NULL;
	
	VERIFY(wnd = GetActiveWin());
	ismysql41 = wnd->m_ismysql41;

	//fetching the list from server
	m_gui_routine((void*)m_gui_lparam, "views", 0, wyFalse, FETCHCOPYDATA);	
	
	query.Sprintf("show full tables from `%s` where Table_type = 'view'", m_srcdb.GetString());

	myres = ExecuteAndGetResult(wnd, m_srctunnel, m_srcmysql,query, wyTrue, wyFalse, wyFalse);
	if(!myres)
	{
		ShowMySQLError(m_hwnddlg, m_srctunnel, m_srcmysql, query.GetString());
		return wyFalse;
	}

	//for displaying copying views... if view is there
	if(myres->row_count > 0)
		m_gui_routine((void*)m_gui_lparam, "views", 0, wyFalse, COPYSTART);	

	while(myrow = m_srctunnel->mysql_fetch_row(myres))
	{
		myrowstr.SetAs(myrow[0], ismysql41);	

		if(IsCopyStopped() == wyTrue)
		{
			m_srctunnel->mysql_free_result(myres);
			return wyFalse;
		}

		if(CreateTemporaryTables(m_srcdb.GetString(), myrowstr.GetString()) == wyFalse)
		{
			m_srctunnel->mysql_free_result(myres);
			return wyFalse;	
		}

		if(m_isobjecttoexport == wyFalse)
			m_isobjecttoexport = wyTrue;

		if(m_isstoredpgrms == wyFalse)
			m_isstoredpgrms = wyTrue;
	}
	
	m_srctunnel->mysql_data_seek(myres,0);

	//Export all views
	while(myrow = m_srctunnel->mysql_fetch_row(myres))
	{
		myrowstr.SetAs(myrow[0], ismysql41);	

		if(IsCopyStopped() == wyTrue)
			return wyFalse;

		ret = ExportView(m_srcdb.GetString(), myrowstr.GetString());
		if(ret == wyFalse)
		{
			m_srctunnel->mysql_free_result(myres);	
			return wyFalse;	
		}
	}

	if(myres->row_count > 0)
		m_gui_routine((void*)m_gui_lparam, "view(s)", myres->row_count, wyFalse, OBJECTSCOPIED);

	m_srctunnel->mysql_free_result(myres);

	return wyTrue;
}

//Export events
wyBool
CopyDatabase::ExportEvents(HWND hwndtree, HTREEITEM hrootitem)
{
	wyBool      ret = wyTrue;
	wyInt32 checkstate = 0, state = 0;
	
		
	state = TreeView_GetItemState(hwndtree, hrootitem, TVIS_EXPANDEDONCE);
	checkstate = TreeView_GetCheckState(hwndtree, hrootitem);
	
	if(checkstate && !(state & TVIS_EXPANDEDONCE))
		ret = ExportEventsOnParentNotExpanded(hwndtree);	
		
	if(checkstate && (state & TVIS_EXPANDEDONCE))
		ret = ExportEventsOnParentExpanded(hwndtree, hrootitem);
			
	
	return ret;
}

//Export event when parent folder is expanded
wyBool
CopyDatabase::ExportEventsOnParentExpanded(HWND hwndtree, HTREEITEM hrootitem)
{
	HTREEITEM hevents;
	wyInt32     checkstate = 0;
	wyWChar     temptext[SIZE_512] = {0};
	TVITEM		tvi = {0};
	wyString    event;
	wyBool		ret = wyTrue;
	wyInt32		count = 0;

	hevents = TreeView_GetChild(hwndtree, hrootitem);

	if(hevents)
	{
		if(m_isobjecttoexport == wyFalse)
			m_isobjecttoexport = wyTrue;
	}

	//for displaying copying events...
	m_gui_routine((void*)m_gui_lparam, "events", 0, wyFalse, COPYSTART);	

	for(; hevents; hevents = TreeView_GetNextSibling(hwndtree, hevents))
	{
		checkstate = TreeView_GetCheckState(hwndtree, hevents);		
		if(checkstate == 0)
			continue;
		tvi.mask = TVIF_HANDLE | TVIF_TEXT;
		tvi.hItem = hevents;
		tvi.pszText = temptext;
		tvi.cchTextMax = SIZE_512 - 1;

		VERIFY(TreeView_GetItem(hwndtree, &tvi));
		event.SetAs(temptext);
		if(IsCopyStopped() == wyTrue)
			return wyFalse;

		if(event.GetLength() != 0)
			ret = ExportEvent(m_srcdb.GetString(), event.GetString());

		if(ret == wyFalse)
			return wyFalse;
		count++;
	}

	m_gui_routine((void*)m_gui_lparam, "event(s)", count, wyFalse, OBJECTSCOPIED);	

	return wyTrue;
}

//Export event when parent folder is not expanded
wyBool
CopyDatabase::ExportEventsOnParentNotExpanded(HWND hwndtree)
{	
	wyString    myrowstr, query;
	MYSQL_RES	*myres = NULL;
	MYSQL_ROW	myrow;
	wyBool		ismysql41 = ((GetActiveWin())->m_ismysql41), ret;
	MDIWindow   *wnd;
	wyBool		iscollate = wyFalse;
	VERIFY(wnd = GetActiveWin());
	if(!wnd)
		return wyFalse;

	//we use collate utf8_bin only if lower_case_table_names is 0 and lower_case_file_system is OFF
	if(GetmySQLCaseVariable(wnd) == 0)
		if(!IsLowercaseFS(wnd))
			iscollate = wyTrue;
	//fetching list from server
	m_gui_routine((void*)m_gui_lparam, "events", 0, wyFalse, FETCHCOPYDATA);
	if(iscollate)
		query.Sprintf("select `EVENT_NAME` from `INFORMATION_SCHEMA`.`EVENTS` where BINARY `EVENT_SCHEMA` = '%s'", 
					m_srcdb.GetString());
	else
		query.Sprintf("select `EVENT_NAME` from `INFORMATION_SCHEMA`.`EVENTS` where `EVENT_SCHEMA` = '%s' ", 
					m_srcdb.GetString());
	myres = ExecuteAndGetResult(wnd, m_srctunnel, m_srcmysql, query);
	if(!myres)
	{
		ShowMySQLError(m_hwnddlg, m_srctunnel, m_srcmysql, query.GetString());
		return wyFalse;
	}

	//if event is there
	if(myres->row_count != 0)
		m_gui_routine((void*)m_gui_lparam, "events", 0, wyFalse, COPYSTART);	

	while(myrow = m_srctunnel->mysql_fetch_row(myres))
	{
		if(IsCopyStopped() == wyTrue)
		{
			m_srctunnel->mysql_free_result(myres);	
			return wyFalse;
		}

		myrowstr.SetAs(myrow[0], ismysql41);
		
		ret = ExportEvent(m_srcdb.GetString(), myrowstr.GetString());
		if(ret == wyFalse)
		{
			m_srctunnel->mysql_free_result(myres);	
            return wyFalse;
		}

		if(m_isobjecttoexport == wyFalse)
			m_isobjecttoexport = wyTrue;
	}
		
	if(myres->row_count > 0)
		m_gui_routine((void*)m_gui_lparam, "event(s)", myres->row_count, wyFalse, OBJECTSCOPIED);	

	m_srctunnel->mysql_free_result(myres);
	
	return wyTrue;
}

//Export triggers
wyBool 
CopyDatabase::ExportTriggers(HWND hwndtree, HTREEITEM hrootitem)
{
	wyBool      ret = wyTrue;
	wyInt32 checkstate = 0, state = 0;	
		
	state = TreeView_GetItemState(hwndtree, hrootitem, TVIS_EXPANDEDONCE);
	checkstate = TreeView_GetCheckState(hwndtree, hrootitem);
	
	if(checkstate && (state & TVIS_EXPANDEDONCE))
		ret = ExportTriggersOnParentExpanded(hwndtree, hrootitem);

	if(checkstate && !(state & TVIS_EXPANDEDONCE))
		ret = ExportTriggersOnParentNotExpanded(hwndtree);	
	
	return ret;
}

//Export trigger(s) when parent folder is expanded
wyBool
CopyDatabase::ExportTriggersOnParentExpanded(HWND hwndtree, HTREEITEM hrootitem)
{

	HTREEITEM		hitemtrigger;
	wyInt32			checkstate;
  	TVITEM			tvi = {0};
	wyString		triggername;
	wyWChar			temptext[SIZE_512] = {0};
	wyBool          ret = wyTrue;
	wyInt32         count = 0;

	hitemtrigger = TreeView_GetChild(hwndtree, hrootitem);

	if(hitemtrigger)
	{
		if(m_isobjecttoexport == wyFalse)
			m_isobjecttoexport = wyTrue;
	}

	m_gui_routine((void*)m_gui_lparam, "triggers", 0, wyFalse, COPYSTART);	
	
	for(; hitemtrigger;	hitemtrigger = TreeView_GetNextSibling(hwndtree, hitemtrigger))			
	{
		checkstate = TreeView_GetCheckState(hwndtree, hitemtrigger);
		if(!checkstate)
			continue;			
		tvi.mask	= TVIF_PARAM | TVIF_TEXT;
		tvi.hItem	= hitemtrigger;
		tvi.pszText = temptext;
		tvi.cchTextMax = 512-1;

		VERIFY(TreeView_GetItem(hwndtree, &tvi));
		triggername.SetAs(temptext);
		if(IsCopyStopped() == wyTrue)
			return wyFalse;

		if(triggername.GetLength() != 0)
            ret = ExportTrigger(m_srcdb.GetString(), triggername.GetString());
		if(ret == wyFalse)
			return wyFalse;
		count++;
	}

	m_gui_routine((void*)m_gui_lparam, "trigger(s)", count, wyFalse, OBJECTSCOPIED);	
	return ret;	
}
	
//Export trigger(s) when parent folder is not expanded
wyBool
CopyDatabase::ExportTriggersOnParentNotExpanded(HWND hwndtree)
{
	wyString		myrowstr, trigger, query;
	MYSQL_RES		*myres = NULL;
	MYSQL_ROW		myrow;
	wyBool			ismysql41, ret;
	MDIWindow		*wnd = NULL;
	
	VERIFY(wnd = GetActiveWin());
	ismysql41 = wnd->m_ismysql41;

	m_gui_routine((void*)m_gui_lparam, "triggers", 0, wyFalse, FETCHCOPYDATA);	
	
	//query.Sprintf("select `TRIGGER_NAME` from `INFORMATION_SCHEMA`.`TRIGGERS` where `TRIGGER_SCHEMA` = '%s'", 
		//			m_srcdb.GetString());

	query.Sprintf("SHOW TRIGGERS FROM `%s`", m_srcdb.GetString());
	

	myres = ExecuteAndGetResult(wnd, m_srctunnel, m_srcmysql,query);
	if(!myres)
	{
		ShowMySQLError(m_hwnddlg, m_srctunnel, m_srcmysql, query.GetString());
		return wyFalse;
	}	

	if(myres->row_count != 0)
		m_gui_routine((void*)m_gui_lparam, "triggers", 0, wyFalse, COPYSTART);	

	//Export all views
	while(myrow = m_srctunnel->mysql_fetch_row(myres))
	{
		myrowstr.SetAs(myrow[0], ismysql41);	

		if(IsCopyStopped() == wyTrue)
		{
			m_srctunnel->mysql_free_result(myres);
			return wyFalse;
		}

		ret = ExportTrigger(m_srcdb.GetString(), myrowstr.GetString());
		if(ret == wyFalse)
		{
			m_srctunnel->mysql_free_result(myres);	
			return wyFalse;	
		}

		if(m_isobjecttoexport == wyFalse)
			m_isobjecttoexport = wyTrue;
	}

	if(myres->row_count > 0)
		m_gui_routine((void*)m_gui_lparam, "trigger(s)", myres->row_count, wyFalse, OBJECTSCOPIED);

	m_srctunnel->mysql_free_result(myres);	

	return wyTrue;
}

//Export procedure/function
wyBool
CopyDatabase::HandleProcedureOrFunction(HWND hwndtree, HTREEITEM hrootitem, wyBool isproc)
{
	wyInt32		checkstate = 0, state = 0;
	
	wyBool      ret = wyTrue;

	state = TreeView_GetItemState(hwndtree, hrootitem, TVIS_EXPANDEDONCE);
	checkstate = TreeView_GetCheckState(hwndtree, hrootitem);

	if(checkstate && !(state & TVIS_EXPANDEDONCE))
	{
		ret = HandleProcedureFunOnParentNotExpanded(hwndtree, isproc);
	}
    
	else if(checkstate && (state & TVIS_EXPANDEDONCE))
	{
		ret = HandleProcedureFunOnParentExpanded(hwndtree, hrootitem, isproc);		
	}
	return ret;
}

//Export procedure/function when parent folder is expanded
wyBool		
CopyDatabase::HandleProcedureFunOnParentExpanded(HWND hwndtree, HTREEITEM htreeitem, wyBool isproc)
{
	HTREEITEM   htable = NULL, hcolitem = NULL, hindexitem = NULL;;
	wyInt32		checkstate = 0, count = 0;
	TVITEM		tvi = {0};
	wyString	procstr;
	wyWChar     temptext[SIZE_512] = {0};
	wyBool		ret;

	htable = TreeView_GetChild(hwndtree, htreeitem);
	if(htable)
	{
		if(m_isobjecttoexport == wyFalse)
			m_isobjecttoexport = wyTrue;
	}

	if(isproc == wyTrue)
		m_gui_routine((void*)m_gui_lparam, "procedures", 0, wyFalse, COPYSTART);	
	else
		m_gui_routine((void*)m_gui_lparam, "functions", 0, wyFalse, COPYSTART);	

	/* patch after 5.1, if the table is not selected we dont generate 
	any statement for it and we continue to next */
	while(htable)
	{		
		checkstate = TreeView_GetCheckState(hwndtree, htable);
		if(!checkstate)
		{
			htable = TreeView_GetNextSibling(hwndtree, htable);
			continue;
		}						
		/* get the correct handle and pointer */
		tvi.mask	= TVIF_PARAM | TVIF_IMAGE | TVIF_TEXT;
		tvi.hItem	= htable;
		tvi.pszText = temptext;
		tvi.cchTextMax = 512-1;

		VERIFY(TreeView_GetItem(hwndtree, &tvi));
		tvi.mask	= TVIF_PARAM | TVIF_IMAGE;
		VERIFY(TreeView_GetItem(hwndtree, &tvi));
		procstr.SetAs(temptext);
		VERIFY(hcolitem   = TreeView_GetChild(hwndtree, htable));
		VERIFY(hindexitem = TreeView_GetNextSibling(hwndtree, hcolitem));
		htable = TreeView_GetNextSibling(hwndtree, htable);

		ret = ExportSP(m_srcdb.GetString(), procstr.GetString(), isproc);
		if(ret == wyFalse)
			return wyFalse;
		count++;

		if(m_isstoredpgrms == wyFalse)
			m_isstoredpgrms = wyTrue;
	}
	if(isproc == wyTrue)
		m_gui_routine((void*)m_gui_lparam, "procedure(s)", count, wyFalse, OBJECTSCOPIED);
	else
		m_gui_routine((void*)m_gui_lparam, "function(s)", count, wyFalse, OBJECTSCOPIED);	

	return wyTrue;
}

//Export procedure/function when parent folder is not expanded
wyBool
CopyDatabase::HandleProcedureFunOnParentNotExpanded(HWND hwndtree, wyBool isproc)
{
	wyString	query, myrowstr;
	MYSQL_RES	*myres = NULL;
	MYSQL_ROW	myrow;
	wyBool		ismysql41, ret;
	MDIWindow   *wnd = NULL;
	wyBool		iscollate = wyFalse;
	VERIFY(wnd = GetActiveWin());
	if(!wnd)
        return wyFalse;

	//we use collate utf8_bin only if lower_case_table_names is 0 and lower_case_file_system is OFF
	if(GetmySQLCaseVariable(wnd) == 0)
		if(!IsLowercaseFS(wnd))
			iscollate = wyTrue;
	ismysql41 = wnd->m_ismysql41;

	if(isproc == wyTrue)
	{
		m_gui_routine((void*)m_gui_lparam, "procedures", 0, wyFalse, FETCHCOPYDATA);
		if(iscollate)
			query.Sprintf("select `SPECIFIC_NAME` from `INFORMATION_SCHEMA`.`ROUTINES` where BINARY `ROUTINE_SCHEMA` = '%s' and ROUTINE_TYPE = 'PROCEDURE'",m_srcdb.GetString());
		else
			query.Sprintf("select `SPECIFIC_NAME` from `INFORMATION_SCHEMA`.`ROUTINES` where `ROUTINE_SCHEMA` = '%s' and ROUTINE_TYPE = 'PROCEDURE'",m_srcdb.GetString());
	}
	
	else
	{
		m_gui_routine((void*)m_gui_lparam, "functions", 0, wyFalse, FETCHCOPYDATA);	
		if(iscollate)
			query.Sprintf("select `SPECIFIC_NAME` from `INFORMATION_SCHEMA`.`ROUTINES` where BINARY  `ROUTINE_SCHEMA` = '%s' and ROUTINE_TYPE = 'FUNCTION'",m_srcdb.GetString());	
		else
			query.Sprintf("select `SPECIFIC_NAME` from `INFORMATION_SCHEMA`.`ROUTINES` where `ROUTINE_SCHEMA` = '%s' and ROUTINE_TYPE = 'FUNCTION'",m_srcdb.GetString());	
	}
	
	myres = ExecuteAndGetResult(wnd, m_srctunnel, m_srcmysql,query);
	if(!myres)
	{
		ShowMySQLError(m_hwnddlg, m_srctunnel, m_srcmysql, query.GetString());
		return wyFalse;
	}

	if(myres->row_count != 0)
	{
		if(isproc == wyTrue)
			m_gui_routine((void*)m_gui_lparam, "procedures", 0, wyFalse, COPYSTART);	
		else
			m_gui_routine((void*)m_gui_lparam, "functions", 0, wyFalse, COPYSTART);	
	}

	while(myrow = m_srctunnel->mysql_fetch_row(myres))
	{
		
		if(IsCopyStopped() == wyTrue)
		{
			m_srctunnel->mysql_free_result(myres);	
			return wyFalse;
		}

		myrowstr.SetAs(myrow[0], ismysql41);

		ret = ExportSP(m_srcdb.GetString(), myrowstr.GetString(), isproc);
		
		if(ret == wyFalse)
		{
			m_srctunnel->mysql_free_result(myres);	
			return wyFalse;		
		}

		if(m_isobjecttoexport == wyFalse)
			m_isobjecttoexport = wyTrue;

		if(m_isstoredpgrms == wyFalse)
			m_isstoredpgrms = wyTrue;
	}

	if(myres->row_count > 0)
	{
		if(isproc == wyTrue)
			m_gui_routine((void*)m_gui_lparam, "procedure(s)", myres->row_count, wyFalse, OBJECTSCOPIED);	
		else
			m_gui_routine((void*)m_gui_lparam, "function(s)", myres->row_count, wyFalse, OBJECTSCOPIED);	
	}
	m_srctunnel->mysql_free_result(myres);	

	return wyTrue;
}

void
CopyDatabase::FreeExportData()
{
   	Delete(&m_seltables);

    m_exportdata    = wyFalse;
    m_isview        = wyFalse;
    m_isprocedure   = wyFalse;
    m_isfunction    = wyFalse;
    m_istrigger     = wyFalse;
    m_dropobjects   = wyFalse;

    return;
}

wyBool
CopyDatabase::ExportData(LPGUI_COPYDB_UPDATE_ROUTINE gui_routine, void * lpParam)
{
    wyString             query;
	wyBool		         ret = wyTrue;
	RelTableFldInfo      *table;
	HWND				  hwndtree = NULL;
	HTREEITEM			  hrootitem = NULL;
	wyInt32				  bulksize = 0,j=0;
	wyString			  msg;
	wyUInt32			  maxallowedsize = 0;
	
    m_gui_routine = gui_routine;
    m_gui_lparam = lpParam;
	MYSQL_RES   *virt_res = NULL;
	MYSQL_ROW	virt_row;
	wyBool *isvirtual = NULL;
	wyInt32    no_rows;	
    wyString   virt_query;

	if(m_exportdata == wyTrue)//Checking struct & data option selected
	{
		// check whether bulkinsert or single insert statements are  used for copying table 	
		if(SendMessage(GetDlgItem(m_hwnddlg, IDC_CPY_BULKINSERT), BM_GETCHECK, 0, 0))
		{
			m_bulkinsert = wyTrue;

			GetBulkSize (&bulksize);
			maxallowedsize = Get_Max_Allowed_Packet_Size(m_newtargettunnel, &m_newtargetmysql);
			
			//if bulksize is not defined or if user defined size is greater that server default then we are setting server default value
			if(!bulksize || bulksize > maxallowedsize)
				m_maxallowedsize = maxallowedsize;
			else
				m_maxallowedsize = bulksize;

			m_bulklimit = min(m_maxallowedsize *1024, BULK_SIZE);
		}
		else 
		{
			m_bulkinsert = wyFalse;
		}

		// if  using bulkinsert stmts then find maximum packet size  
		if(m_bulkinsert == wyTrue)
		{
			
			if(m_maxallowedsize <= 0)
			{
				msg.SetAs("A BULK size specified must be increased.");
				MessageBox(NULL, msg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), 
							 MB_TASKMODAL | MB_OK | MB_ICONINFORMATION);

				return wyFalse;
			}
		}
	}
	
	VERIFY(hwndtree = GetDlgItem(m_hwnddlg, IDC_TREE));
    hrootitem = GetObjectsHtreeItem(hwndtree, TXT_TABLES);
	if(hrootitem)
		HandleTables(hwndtree, hrootitem);

	// now get each table and copy it.
	for(table = (RelTableFldInfo*)m_seltables.GetFirst(); table; table = (RelTableFldInfo*)table->m_next) 
	{
        if(IsCopyStopped() == wyTrue)
           return ret;

		if(m_dropobjects == wyTrue)
        {
			ret = DropTable((wyChar*)table->m_tablefld.GetString());
			if(ret == wyFalse)
				return ret;
		}
		
		// create table.
		// if the target database is of versions lower than v4.1 then we will
		// create tables with our own script, bcoz, it may not work with the character set

		if(IsMySQL41(m_newsrctunnel, &m_newsrcmysql)&& !IsMySQL41(m_newtargettunnel, &m_newtargetmysql))
			ret = CreateTableInTarget2((wyChar*)table->m_tablefld.GetString());
		else
			ret = CreateTableInTarget((wyChar*)table->m_tablefld.GetString());

		if(ret == wyFalse || (IsCopyStopped() == wyTrue))
				return ret;		
		
		
		if(m_exportdata == wyTrue)
        {
			m_copiedcount = 0;

		//---------------------------Virtuality------------------------------//
			
			virt_query.Sprintf("SHOW FIELDS FROM `%s`.`%s`",m_srcdb.GetString(), table->m_tablefld.GetString());
			virt_res = SjaExecuteAndGetResult(m_newsrctunnel, &m_newsrcmysql, virt_query);
			if(virt_res)
			no_rows =  m_newsrctunnel->mysql_num_rows(virt_res);

			if(!virt_res  || no_rows == -1) 
			{
			ShowMySQLError(m_hwnddlg, m_newsrctunnel, &m_newsrcmysql);
			return wyFalse;
			}
			
			isvirtual = (wyBool *)calloc(no_rows,sizeof(wyBool));
		
			virt_row = m_newsrctunnel->mysql_fetch_row(virt_res);
			j=0;
			while(virt_row)
			{
				if(strstr(virt_row[5], "VIRTUAL") || strstr(virt_row[5], "PERSISTENT") || strstr(virt_row[5], "STORED"))
				{
				isvirtual[j++] = wyTrue;
				}
				else
				{
				isvirtual[j++]= wyFalse;
				}
				virt_row = sja_mysql_fetch_row(m_newsrctunnel, virt_res);
			}
		
			m_newsrctunnel->mysql_free_result(virt_res);
	//-------------------------------------Virtuality----------------------------//


			ret = ExportActualData((wyChar*)table->m_tablefld.GetString(),isvirtual);

			
			if(isvirtual != NULL)
			{
			free(isvirtual);
			isvirtual = NULL;
			}

			//for showing the total no of rows copied in a table
			m_gui_routine((void*)m_gui_lparam,(wyChar*)table->m_tablefld.GetString(), m_copiedcount, wyTrue, TABLECOPIED );	
			if(ret == wyFalse)
				return ret;
		}
		else
			//for showing table copied if structure only is selected
			m_gui_routine((void*)m_gui_lparam,(wyChar*)table->m_tablefld.GetString(), 0, wyFalse, TABLECOPIED);	
	}

	if(m_seltables.GetCount() > 0)
		m_gui_routine((void*)m_gui_lparam, "table(s)", m_seltables.GetCount(), wyFalse, OBJECTSCOPIED);	
    
    if(IsCopyStopped() == wyTrue)
        return wyTrue;


	// if mysql version is below 5.1 then it is not supporting views, triggers, procedures, funs, events
	if(IsMySQL5010(m_newtargettunnel, &m_newtargetmysql) == wyTrue)
		ret = Export5XObjects();

	
	RevertTargetServer();
	
	SetNamesToOriginal();

	SetCursor(LoadCursor(NULL, IDC_ARROW));

	return ret;
}
wyBool
CopyDatabase::Export5XObjects()
{
    HWND        hwndtree;
	HTREEITEM   hrootitem;
	wyBool      ret = wyTrue;

	m_isprocedure = wyFalse;
	m_isstoredpgrms = wyFalse;
		
	if(IsCopyStopped() == wyTrue)
		return wyTrue;
	VERIFY(hwndtree = GetDlgItem(m_hwnddlg,IDC_TREE));

    hrootitem = GetObjectsHtreeItem(hwndtree, TXT_TRIGGERS);
	if(hrootitem)
		 ret = ExportTriggers(hwndtree, hrootitem);

	if(IsCopyStopped() == wyTrue || (ret == wyFalse))
		return ret;
    
    hrootitem = GetObjectsHtreeItem(hwndtree, TXT_EVENTS);
	if(hrootitem && (IsMySQL516(m_newtargettunnel, &m_newtargetmysql) == wyTrue))
		ret = ExportEvents(hwndtree, hrootitem);		
		
	if(IsCopyStopped() == wyTrue || (ret == wyFalse))
		return ret;
	
    hrootitem = GetObjectsHtreeItem(hwndtree, TXT_FUNCTIONS);
	if(hrootitem)
		ret = HandleProcedureOrFunction(hwndtree, hrootitem, wyFalse);
	
	if(IsCopyStopped() == wyTrue || (ret == wyFalse))
		return ret;
	
    hrootitem = GetObjectsHtreeItem(hwndtree, TXT_PROCEDURES);	
	if(hrootitem)
		ret = HandleProcedureOrFunction(hwndtree, hrootitem, wyTrue);

	if(IsCopyStopped() == wyTrue || (ret == wyFalse))
		return ret;		

	hrootitem = GetObjectsHtreeItem(hwndtree, TXT_VIEWS);
	if(hrootitem)
	{
		 ret = ExportViews(hwndtree, hrootitem);
		// release memory allcote  for list m_selviews used for saving selected views
		Delete(&m_selviews);		
	}
	return ret;
}

void 
CopyDatabase::InitTargetServer()
{
    wyString    query;
    MYSQL_RES   *res;
	
    /* from 4.0 RC 1 we issue a statement SET FOREIGN_KEY CHECK = 0 in the
	   beginning and = 1 at end */
	//Disable FK check(Set FK-CHECK = 0) for direct concection(for http with each query it has to be done)
	if(m_newtargettunnel->IsTunnel() == false)
	{
		query.SetAs("SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0"); 
		res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
		if(res)
			m_newtargettunnel->mysql_free_result(res);
	
		/* this is done on 5.1 beta 2 to avoid autoincrement field with value 0 in */	
		query.SetAs("SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO'"); 
		res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
		if(res)
			m_newtargettunnel->mysql_free_result(res);

		query.SetAs("/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;");
		res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
		if(res)
			m_newtargettunnel->mysql_free_result(res);

		query.SetAs("/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;");
		res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
		if(res)
			m_newtargettunnel->mysql_free_result(res);
	}
}

void 
CopyDatabase::RevertTargetServer()
{
    wyString    query;
    MYSQL_RES   *res;
	
	//Set the FK check to revert back after the copy, in direct con.
	if(m_newtargettunnel->IsTunnel() == false)
	{
		query.SetAs(" SET SQL_MODE=@OLD_SQL_MODE"); 
		res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
		if(res)
		  m_newtargettunnel->mysql_free_result(res);

		query.SetAs("SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS"); 
		res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
		if(res)
			m_newtargettunnel->mysql_free_result(res);

		query.SetAs("/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;");
		res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
		if(res)
			m_newtargettunnel->mysql_free_result(res);

		query.SetAs("/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;");
		res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
		if(res)
			m_newtargettunnel->mysql_free_result(res);
	}
}

void
CopyDatabase::SetNamesToUTF8()
{
    wyString    query;
    MYSQL_RES   *res;

    if(IsMySQL41(m_newsrctunnel, &m_newsrcmysql)&& IsMySQL41(m_newtargettunnel, &m_newtargetmysql))
	{
		if(m_newsrctunnel->IsTunnel())
			m_newsrctunnel->SetCharset("utf8");
		else
		{
			query.Sprintf("SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT"); 
			res = SjaExecuteAndGetResult(m_newsrctunnel, &m_newsrcmysql, query);
			if(res)
				m_newsrctunnel->mysql_free_result(res);

			query.Sprintf("SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS"); 
			res = SjaExecuteAndGetResult(m_newsrctunnel, &m_newsrcmysql, query);
			if(res)
				m_newsrctunnel->mysql_free_result(res);

			query.Sprintf("SET NAMES utf8"); 
			res = SjaExecuteAndGetResult(m_newsrctunnel, &m_newsrcmysql, query);
			if(res)
				m_newsrctunnel->mysql_free_result(res);
		}

		if(m_newtargettunnel->IsTunnel())
			m_newtargettunnel->SetCharset("utf8");
		else
		{

			query.Sprintf("SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT"); 
			res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
			if(res)
				m_newtargettunnel->mysql_free_result(res);

			query.Sprintf("SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS"); 
			res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
			if(res)
				m_newtargettunnel->mysql_free_result(res);

			query.Sprintf("SET NAMES utf8"); 
			res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
			if(res)
				m_newtargettunnel->mysql_free_result(res);
		}
    }

    return;
}

void 
CopyDatabase::SetNamesToOriginal()
{
    wyString    query;
    MYSQL_RES   *res;

    if(IsMySQL41(m_newsrctunnel, &m_newsrcmysql)&& IsMySQL41(m_newtargettunnel, &m_newtargetmysql))
	{
        query.Sprintf("SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT"); 
        res = SjaExecuteAndGetResult(m_newsrctunnel, &m_newsrcmysql, query);
	    if(res)
            m_newsrctunnel->mysql_free_result(res);

	    query.Sprintf("SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS"); 
	    res = SjaExecuteAndGetResult(m_newsrctunnel, &m_newsrcmysql, query);
	    if(res)
            m_newsrctunnel->mysql_free_result(res);

	    query.Sprintf("SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT"); 
        res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
	    if(res)
            m_newtargettunnel->mysql_free_result(res);

	    query.Sprintf("SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS"); 
        res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
	    if(res)
            m_newtargettunnel->mysql_free_result(res);
    }

    return;
}
wyBool
CopyDatabase::ChangeSourceDB()
{
    wyString    query;
    MYSQL_RES   *res;

	query.Sprintf("use `%s`", m_srcdb.GetString());

    res = SjaExecuteAndGetResult(m_srctunnel, m_srcmysql, query);

	if(!res && m_srctunnel->mysql_affected_rows(*m_srcmysql)== -1)
	{
		ShowMySQLError(m_hwnddlg, m_srctunnel, m_srcmysql, query.GetString());
		return wyFalse;
	}

	m_srctunnel->mysql_free_result(res);
	m_srctunnel->SetDB(m_srcdb.GetString());

	return wyTrue;
}

/// Gets Charset and Collation for MySQL versions > 4.1
wyBool					
CopyDatabase::GetCharsetAndCollation(wyString *charset, wyString *collation)
{
	MDIWindow	    *wnd;
	wyString	    query, dbname;
	wyInt32		    fldindex;
	wyBool		    ismysql41; 
	//TableinfoElem*  ptabinfo;
	MYSQL_RES*	    res;
	MYSQL_ROW	    row;

	VERIFY(wnd = GetActiveWin());

    if(!wnd)
        return wyFalse;

	ismysql41 = IsMySQL41(m_srctunnel, m_srcmysql);
    
	//ptabinfo = (TableinfoElem*)m_list.GetFirst();

	//if(!ptabinfo)
		//return wyFalse;

	dbname.SetAs(m_srcdb);

	if(IsMySQL41(m_srctunnel, m_srcmysql) == wyTrue)
	{
		query.Sprintf("use `%s`", dbname.GetString());
		res = ExecuteAndGetResult(wnd, m_srctunnel, m_srcmysql, query);

        if(!res && m_srctunnel->mysql_affected_rows(*m_srcmysql) == -1)
		{
			return wyFalse;
		}

		// Charset
		query.Sprintf("show variables like 'character_set_database'");
		res = ExecuteAndGetResult(wnd, m_srctunnel, m_srcmysql, query);

        if(!res && m_srctunnel->mysql_affected_rows(*m_srcmysql)== -1)
		{
			return wyFalse;
		}

		fldindex = GetFieldIndex(m_srctunnel, res, "Value"); 
		row = m_srctunnel->mysql_fetch_row(res);

        if(!row)
			return wyFalse;
		
		charset->SetAs(row[fldindex], ismysql41);
		m_srctunnel->mysql_free_result(res);
        
		// Collation
		query.Sprintf("show variables like 'collation_database'");
		res = ExecuteAndGetResult(wnd, m_srctunnel, m_srcmysql, query);

        if(!res && m_srctunnel->mysql_affected_rows(*m_srcmysql)== -1)
		{
			return wyFalse;
		}

		fldindex = GetFieldIndex(m_srctunnel, res, "Value");
		row = m_srctunnel->mysql_fetch_row(res);

        if(!row)
			return wyFalse;

		collation->SetAs(row[fldindex], ismysql41);
		m_srctunnel->mysql_free_result(res);
	}
	else
		return wyFalse;

	if(!collation->GetLength() || !charset->GetLength())
		return wyFalse;

	return wyTrue;
}

wyBool
CopyDatabase::CreateTargetDB()
{
	wyString    query,charset,collation;
    MYSQL_RES   *res;

	if(m_iscreatedb)
	{
		query.Sprintf("CREATE DATABASE IF NOT EXISTS `%s` ", m_targetdb.GetString());
		GetCharsetAndCollation(&charset, &collation);
		if(charset.GetLength() != 0)
		{
			query.AddSprintf("character set %s ", charset.GetString());
		}
		if(collation.GetLength() != 0)
		{
			query.AddSprintf("collate %s", collation.GetString());
		}
		res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);

		if(!res && m_newtargettunnel->mysql_affected_rows(m_newtargetmysql)== -1)
		{
			ShowMySQLError(m_hwnddlg, m_newtargettunnel, &m_newtargetmysql, query.GetString());
			return wyFalse;
		}
	}
	return wyTrue;
}
wyBool
CopyDatabase::ChangeTargetDB()
{
	wyString    query,charset,collation;
    MYSQL_RES   *res;

	query.Sprintf("use `%s`", m_targetdb.GetString());

    res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);

	if(!res && m_newtargettunnel->mysql_affected_rows(m_newtargetmysql)== -1)
	{
		ShowMySQLError(m_hwnddlg, m_newtargettunnel, &m_newtargetmysql, query.GetString());
		return wyFalse;
	}

	m_newtargettunnel->mysql_free_result(res);
	m_newtargettunnel->SetDB(m_targetdb.GetString());
	
	return wyTrue;
}

//With Tunnel, SET FK-CHECK = 0, send with each query (as batch)
wyBool
CopyDatabase::DropTable(wyChar *table)
{
    wyString    query;
    bool		force = false, batch = false;

	if(m_newtargettunnel->IsTunnel() == true)
	{
		batch = true; 

		query.SetAs("set FOREIGN_KEY_CHECKS = 0");
		if(HandleExecuteQuery(query, batch, force) == wyFalse)
			return wyFalse;

		force = true; //for drop query force is true
	}
	
	query.Sprintf("drop table if exists `%s`.`%s`", m_targetdb.GetString(), table);

	return HandleExecuteQuery(query, batch, force);	
}

// Create the database in the target mysql.
wyBool
CopyDatabase::CreateTableInTarget(wyChar  *table)
{
    wyString    query, msg, fkquery;
    MYSQL_RES	*myres;
	MYSQL_ROW	myrow;
	wyBool		ismysql41 =IsMySQL41(m_newsrctunnel, &m_newsrcmysql);
	bool		force = false, batch = false;
	
    m_gui_routine((void*)m_gui_lparam, table, 0, wyFalse, CREATETABLE);

	query.Sprintf("show create table `%s`.`%s`", m_srcdb.GetString(), table);
    myres = SjaExecuteAndGetResult(m_newsrctunnel, &m_newsrcmysql, query);

	if(!myres)
	{
		m_copying = wyFalse;
		ShowMySQLError(m_hwnddlg, m_newsrctunnel, &m_newsrcmysql, query.GetString());
		return wyFalse;
	}

	myrow = m_newsrctunnel->mysql_fetch_row(myres);
    query.SetAs(myrow[1], ismysql41);

	if(myres)
		m_newsrctunnel->mysql_free_result(myres);
	
	//Create in target
	if(m_newtargettunnel->IsTunnel() == true)
	{
		batch = true;
		//Disable FK check
		fkquery.SetAs("set FOREIGN_KEY_CHECKS = 0");
		if(HandleExecuteQuery(fkquery, batch, force) == wyFalse)
			return wyFalse;

		force = true;
	}

	return HandleExecuteQuery(query, batch, force);	
}

wyBool
CopyDatabase::CreateTableInTarget2(wyChar *table)
{
    wyString    msg, query, fkquery;
	wyInt32     ret;
	bool		batch = false, force = false;

    m_gui_routine((void*)m_gui_lparam, table, 0, wyFalse, CREATETABLE);
		
	//Gets the Create table stmt
	ret = CopyTableFromNewToOld(m_newsrctunnel, &m_newsrcmysql, m_newtargettunnel, &m_newtargetmysql, m_srcdb.GetString(), table, query);

	if(ret == 1)// From sorce(if query exection at source side fails )
	{
		ShowMySQLError(m_hwnddlg, m_newsrctunnel, &m_newsrcmysql, query.GetString());
		return wyFalse;
	}
	
	//Create query execution at target
	if(m_newtargettunnel->IsTunnel() == true)
	{
		batch = true;

		fkquery.SetAs("set FOREIGN_KEY_CHECKS = 0");//Disable FK Check
		if(HandleExecuteQuery(fkquery, batch, force) == wyFalse)
			return wyFalse;

		force = true;
	}

	//Executes the Create table statement
	return HandleExecuteQuery(query, batch, force);
}

/*Its added when implemented disable FK-CHECK during http.
during Set FK-CHECK =0 - force = false, batch = true
during query(drop, create, insert)- force = true, batch = true
*/
wyBool
CopyDatabase::HandleExecuteQuery(wyString &query, bool batch, bool force)
{
	MYSQL_RES	*myres;
	
	myres = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query, wyFalse, batch, force);

	if(!myres && (m_newtargetmysql)->affected_rows == -1)
	{
		m_copying = wyFalse;
		ShowMySQLError(m_hwnddlg, m_newtargettunnel, &m_newtargetmysql, query.GetString());
		return wyFalse;
	}

	if(myres)
		m_newtargettunnel->mysql_free_result(myres);

	return wyTrue;
}

// check query length (no of bytes) if it exceed maximum packet lenth then execute query 
// after that use another insert stmt
wyBool 
CopyDatabase::CheckQueryLimit(MYSQL_RES *myres, wyInt32 fieldcount, wyInt32 &querylength)
{
	
	wyULong		*fieldlengths ;
	wyInt32      count;

	fieldlengths = m_newsrctunnel->mysql_fetch_lengths(myres);
	if(fieldlengths == 0)
		return wyFalse;

	for(count = 0; count < fieldcount; count++)
		querylength = querylength + fieldlengths[count];
	// find out whether query length is more than packet size or lower than packet size
	if((querylength + fieldcount * 3 + 3) >= m_bulklimit)
		return wyTrue;

	return wyFalse;
}
// execute insert query 
wyBool
CopyDatabase::ExecuteInsertQuery(MYSQL_RES *myres, wyString &insertstmt, wyChar *table,
								  wyInt32 &exportedrowcount)
{
	MYSQL_RES	*tempres = NULL;
	wyInt32     ret = 0;
	MDIWindow   *wnd = GetActiveWin();
	wyBool		isfkcheck = wyFalse;
		
	if(!wnd)
		return wyFalse;

	/*If http, Add SET FK-CHK = 0, with each batch, to disable the FK check.
	So with each batch SET FK-CHECK=0, will be added in the begining
	*/
	if(m_newtargettunnel->IsTunnel() == true)
		isfkcheck = wyTrue;

	wnd->SetThreadBusy(wyTrue);
	ret = my_query(wnd, m_newtargettunnel, &m_newtargetmysql, insertstmt.GetString(), 
		insertstmt.GetLength(), wyTrue, wyFalse, NULL, 0, wyFalse, wyTrue, false, wyFalse, isfkcheck);
	wnd->SetThreadBusy(wyFalse);

    if(ret) 
    {
		ShowMySQLError(m_hwnddlg, m_newtargettunnel, &m_newtargetmysql, insertstmt.GetString());
		m_newsrctunnel->mysql_free_result(myres);
		return wyFalse;
	}

	tempres = m_newtargettunnel->mysql_store_result(m_newtargetmysql, true, false, GetActiveWin());

	if(!tempres && (m_newtargetmysql)->affected_rows == -1)
	{
		ShowMySQLError(m_hwnddlg, m_newtargettunnel, &m_newtargetmysql, insertstmt.GetString());
		return wyFalse;
	}

    if(tempres)
    {
		m_newtargettunnel->mysql_free_result(tempres);
        tempres = NULL;
    }
	// for every 100 rows we will refreash the message (if it is not bulk insert query )
	if(m_bulkinsert == wyFalse)
	{
		if(exportedrowcount % 100 == 0)
			m_gui_routine((void*)m_gui_lparam, table, exportedrowcount, wyTrue, ROWSCOPIED);
		return wyTrue;
	}
	// if bulk query refresh after bulk query executed
	m_gui_routine((void*)m_gui_lparam, table, exportedrowcount, wyTrue, ROWSCOPIED);

	return wyTrue;
}

// usung these function getting single insert or bulkinsert(adding row values to the query)
void
CopyDatabase::GetInsertQuery(MYSQL_RES *myres, MYSQL_ROW myrow, wyString &insertstmt, wyInt32 nfilelds, 
							 wyBool ismysql41,wyBool *isvirtual)
{
	wyULong        *lengths;
	wyChar          *temp;
	MYSQL_FIELD		*myfield;
	wyInt32         j, newlength;
	
	// get the length of the row so that we can place some binary data as 
	lengths		= m_newsrctunnel->mysql_fetch_lengths(myres);	

    for(j = 0; j < nfilelds; j++)
	{
		if(isvirtual[j]==wyTrue)
			continue;
	
		VERIFY(myfield = m_newsrctunnel->mysql_fetch_field_direct(myres, j));
		// Write NULL if NULL value.
		if(myrow[j] == NULL)
			insertstmt.Add("NULL, ");
        else if(!m_newsrctunnel->IsTunnel()&& IsNumber(myfield->type))
		{
			wyString        myrowstr;

			myrowstr.SetAs(myrow[j], ismysql41);
			insertstmt.AddSprintf("%s, ", myrowstr.GetString());
		}
        else	
        {
			temp =(CHAR*)calloc(sizeof(CHAR),((lengths[j]*2) + 1));
			newlength = m_newtargettunnel->mysql_real_escape_string(m_newtargetmysql, temp, myrow[j], lengths[j]);
						
			if(myfield->type != MYSQL_TYPE_LONG_BLOB ||
				myfield->type != MYSQL_TYPE_MEDIUM_BLOB ||
				myfield->type != MYSQL_TYPE_TINY_BLOB || myfield->type != MYSQL_TYPE_BLOB)
            {
				if(ismysql41 ==	wyFalse)
				{
					wyString        myrowstr;					
					myrowstr.SetAs(temp, ismysql41);
					free(temp);
					insertstmt.AddSprintf("'%s', ", myrowstr.GetString());
				} 
				else
				{
					insertstmt.AddSprintf("'%s', ", temp);
					free(temp);
				}
				
			}
			else
			{
				insertstmt.AddSprintf("'%s', ", temp);
				free(temp);
			}
		}
	}
}
// Function creates insert statement and executes in the targetmysql.

wyBool
CopyDatabase::ExportActualData(wyChar * table,wyBool *isvirtual)
{
	wyInt32         ret;
	wyString        query, msg, insertstmt, myrowstr;
	wyInt32         nfilelds, numrows, errornum = 0;
    wyInt32         rowcount = 0, startrowcount = 0, endrowcount = 0, exportedrowcount = 0;
    MYSQL_RES		*myres, *tempres;
	MYSQL_ROW		myrow;
	MYSQL_FIELD		*fieldnames;
	wyBool			ismysql41 = ((GetActiveWin())->m_ismysql41);
	wyInt32         bulkinsertcnt = 0;
	wyString        bulkquery, fieldval, errmsg;
	wyBool          flag = wyTrue;
	wyInt32         querylength = 0;
	wyInt32			rowscopied = 0,i=0;
	MDIWindow		*wnd = GetActiveWin();
	wyInt32			chunklimit;
	wyBool			ischunkinsert;
	//wyString st;
	//MYSQL_RES *rs;
	//MYSQL_ROW row;
	//st.SetAs("show variables like 'net_write_timeout'");
	////st.SetAs("show variables like 'wait_timeout'");
	//rs = SjaExecuteAndGetResult(m_newsrctunnel, &m_newsrcmysql, st, wyTrue);
	//row = m_newsrctunnel->mysql_fetch_row(rs);


    m_gui_routine((void*)m_gui_lparam, table, 0, wyTrue, ROWSCOPIED);	

	rowcount = GetRowCount(m_newsrctunnel, m_newsrcmysql, m_srcdb.GetString(), table, NULL);
   
	ischunkinsert = IsChunkInsert();
	GetChunkLimit(&chunklimit);
	if(m_newsrctunnel->IsTunnel())
	{
		//In HTTP, if don't break into chunks is checked or chunk size is zero or chunk size is > 1000 , then we will use the chunk size as 1000
		//otherwise , means if chunksize is <1000, that size will use.
		if(chunklimit > MAX_ROW_LIMIT || ischunkinsert == wyFalse || chunklimit == 0)
			chunklimit = MAX_ROW_LIMIT;
	}
	else {
		if(ischunkinsert ==  wyTrue && chunklimit == 0)
			chunklimit = MAX_ROW_LIMIT;
	}

	/* if it is in tunnel mode we need to retrieve row by packet by packet, 
	other wise the HTTP buffer cannot hold all the values if the table 
	contains a large number of rows. So we make use of MAX_ROW_COUNT 
	defined in Global.h. we will retrieve only MAX_ROW_COUNT rows at a time,
	and we will do it iteratively */
	do
	{
		startrowcount	= endrowcount;
		endrowcount		+= chunklimit;
		i=0;

		 if(IsCopyStopped() == wyTrue)
			return wyTrue;
		//if source is tunnel OR when chunkinsert is enabled in preferences with a value > 0 we break into chunks
		if(m_newsrctunnel->IsTunnel() || (ischunkinsert && chunklimit > 0))
			query.Sprintf("select * from `%s`.`%s` limit %d, %d", 
					m_srcdb.GetString(), table, startrowcount, chunklimit);
		else
		    query.Sprintf("select * from `%s`.`%s`", m_srcdb.GetString(), table);

        myres = ExecuteAndUseResult(m_newsrctunnel, &m_newsrcmysql, query);
     
		if(!myres)
		{
			ShowMySQLError(m_hwnddlg, m_newsrctunnel, &m_newsrcmysql, query.GetString());
			return wyFalse;
		}

		nfilelds = m_newsrctunnel->mysql_num_fields(myres);
		numrows = m_newsrctunnel->mysql_num_rows(myres);
		// first time find bulk insert query if it is tunnel remaining times using same bulk query formate
		if(flag == wyTrue)
		{			
			bulkquery.Sprintf("insert into `%s`.`%s` (", m_targetdb.GetString(), table);
			// insert field names in bulkinsert query
			while(fieldnames = m_newsrctunnel->mysql_fetch_field(myres))
			{
				if(isvirtual[i]== wyTrue)
					{
						i++;
						continue;
					}
				fieldval.SetAs(fieldnames->name, ismysql41);
				bulkquery.AddSprintf("`%s`,", fieldval.GetString());
				i++;
			}

			bulkquery.Strip(1);
			bulkquery.Add(") values (");
			bulkinsertcnt = querylength = bulkquery.GetLength();			
		}
		
		// empty the new targettunnel query buffer
		m_newtargettunnel->EmptyQueryBuffer();
		// if bulk insert is checked use bulk query
		if(m_bulkinsert == wyTrue)
		{
			insertstmt.SetAs(bulkquery.GetString());
			while(myrow = m_newsrctunnel->mysql_fetch_row(myres))
			{
				querylength = insertstmt.GetLength();
				exportedrowcount ++;	
				// if query limit exceed maximum packet size insert one more insert stmt
				if(querylength != bulkinsertcnt && CheckQueryLimit(myres, nfilelds, querylength) == wyTrue )
				{
					if(IsCopyStopped() == wyTrue)
						return wyTrue;
					insertstmt.Strip(2);
					if(ExecuteInsertQuery(myres, insertstmt, table, exportedrowcount) == wyFalse)
					{
						insertstmt.Clear();
						return wyFalse;
					}

					//total number of rows copied in a table
					rowscopied = exportedrowcount - rowscopied;
					m_copiedcount += rowscopied;
					rowscopied = exportedrowcount;

					querylength = bulkinsertcnt;
					insertstmt.SetAs("");
					insertstmt.SetAs(bulkquery.GetString());
				}
				GetInsertQuery(myres, myrow, insertstmt, nfilelds, ismysql41,isvirtual);				
				insertstmt.Strip(2);
				insertstmt.Add("),(");				
			}
			if(IsCopyStopped() == wyTrue)
				return wyTrue;
			
			insertstmt.Strip(2); // if number of exportted row is zero then no need to execute any query		
			if((exportedrowcount != 0) && ExecuteInsertQuery(myres, insertstmt,table, exportedrowcount) == wyFalse)
			{
				insertstmt.Clear();
				return wyFalse;
			}

			//count of number of rows copied
			m_copiedcount += (exportedrowcount - rowscopied);
			rowscopied = exportedrowcount;
		
		}
		else
		{
			i = 0;
			while(myrow = m_newsrctunnel->mysql_fetch_row(myres))
			{				
				exportedrowcount ++;
				// get the length of the row so that we can place some binary data as 	
				insertstmt.SetAs(bulkquery.GetString());
				GetInsertQuery(myres, myrow, insertstmt, nfilelds, ismysql41,isvirtual);
				insertstmt.Strip(2);
				insertstmt.Add(")");
				if(IsCopyStopped() == wyTrue)
				{
					m_newsrctunnel->mysql_free_result(myres);
					myres = NULL;
					
					return wyTrue;
				}
				if(ExecuteInsertQuery(myres, insertstmt,table, exportedrowcount) == wyFalse)
				{
					m_newsrctunnel->mysql_free_result(myres);
					myres = NULL;

					return wyFalse;	
				}
				
				//Increase the count of total number of rows copied
				m_copiedcount++;
			}	

			//if error occured while fetching row from 'server' display the status in copy db dialog
			if(m_copiedcount < rowcount)
			{	
				//_________________________log info tkt 9305
				wyString st;
				MYSQL_RES *rs = NULL;
				MYSQL_ROW row = NULL;
				st.SetAs("show variables like 'net_write_timeout'");
				rs = SjaExecuteAndGetResult(m_newsrctunnel, &m_newsrcmysql, st, wyTrue);
                
				if(rs)
				{
					row = m_newsrctunnel->mysql_fetch_row(rs);

					if(row && row[1])
					{
						st.SetAs(row[1]);
						errmsg.Sprintf("net_write_timeout = %s", st.GetString());
						YOGLOG(0, (char*)errmsg.GetString());

					}
					else
					{
						YOGLOG(0, "net_write_timeout fecth_row returns NULL");
					}

					m_newsrctunnel->mysql_free_result(rs);
				}

				else
				{
					YOGLOG(0, "net_write_timeout result returns NULL");
				}
				
				//_________________________

				
				errornum = m_newsrctunnel->mysql_errno(m_newsrcmysql);
				
				errmsg.Sprintf("MySQL error = %d, error = %s", errornum, m_newsrctunnel->mysql_error(m_newsrcmysql));
				YOGLOG(0, (char*)errmsg.GetString());

				if(errornum)
				{
                    ShowMySQLError(m_hwnddlg, m_newsrctunnel, &m_newsrcmysql, NULL, wyTrue);				
					{
						if(myres)
							m_newsrctunnel->mysql_free_result(myres);

                        myres = NULL;
						return wyFalse;				
					}
				}
			}
		}
		// now if its tunneling then we batch process it.
		// so we need to process extra..
		if(m_newtargettunnel->IsTunnel())
		{
			wnd->SetThreadBusy(wyTrue);
			ret = my_query(GetActiveWin(), m_newtargettunnel, &m_newtargetmysql, "", 0, wyTrue, wyTrue);
			wnd->SetThreadBusy(wyFalse);
			m_newtargettunnel->SetBatchEnd(true);

			tempres = m_newtargettunnel->mysql_store_result(m_newtargetmysql, true, false, GetActiveWin());
			if(!myres && (m_newtargetmysql)->affected_rows == -1)
			{
				ShowMySQLError(m_hwnddlg, m_newtargettunnel, &m_newtargetmysql, NULL, wyTrue);
				return wyFalse;
			}

			m_newtargettunnel->mysql_free_result(tempres);
		}

		m_newsrctunnel->mysql_free_result(myres);
            
        if(IsCopyStopped() == wyTrue)
            break;
	
	}while((m_newsrctunnel->IsTunnel() && rowcount > endrowcount) || (!m_newsrctunnel->IsTunnel() && ischunkinsert && rowcount > endrowcount));

	insertstmt.Clear();
	m_gui_routine((void*)m_gui_lparam, table, exportedrowcount, wyTrue, ROWSCOPIED);

	return wyTrue;
}

void
CopyDatabase::UpdateGui(void * lpParam, wyChar *tablename, wyUInt32 rowsinserted, wyBool isdata, COPYSTATE state)
{
	COPYDBPARAM *param = (COPYDBPARAM *)lpParam;
	wyString    msg;
    MSG         message;
    
	switch(state)
	{
	case CREATETABLE:
		msg.Sprintf(_("Create table structure for %s"), tablename);
		SetWindowText(param->m_hwndmsg, msg.GetAsWideChar());
		break;
		
	case ROWSCOPIED:
		if(rowsinserted != 0)
			msg.Sprintf(_("Copying data for %s (%ld rows completed)"), tablename, rowsinserted);
		else
			msg.Sprintf(_("Copying data for %s"), tablename);

		SetWindowText(param->m_hwndmsg, msg.GetAsWideChar());
		break;

	case COPYSTART:
		msg.Sprintf(_("Copying %s ..."), tablename);
		SetWindowText(param->m_hwndmsg, msg.GetAsWideChar());
		break;

	case FETCHCOPYDATA:
		msg.Sprintf(_("Fetching list of %s from server..."), tablename);
		SetWindowText(param->m_hwndmsg, msg.GetAsWideChar());
		break;
		
	case TABLECOPIED:
		if(param->m_summary)
		{
			param->m_summary->AddSprintf("Table '%s'", tablename);
			if(isdata == wyTrue)
				param->m_summary->AddSprintf(_(" (%lu rows))"), rowsinserted);
			param->m_summary->AddSprintf(_(" copied\r\n"));
		}
		break;

	case VIEWCOPIED:
		param->m_summary->AddSprintf(_("View '%s' copied\r\n"), tablename);
		break;
	
	case FUNCTIONCOPIED:
		param->m_summary->AddSprintf(_("Function '%s' copied\r\n"), tablename);
		break;
	
	case PROCEDURECOPIED:
		param->m_summary->AddSprintf(_("Procedure '%s' copied\r\n"), tablename);
		break;
	
	case TRIGGERCOPIED:
		param->m_summary->AddSprintf(_("Trigger '%s' copied\r\n"), tablename);
		break;
	
	case EVENTCOPIED:
		param->m_summary->AddSprintf(_("Event '%s' copied\r\n"), tablename);
		break;

	case OBJECTSCOPIED:
		param->m_summary->AddSprintf(_("%d %s copied\r\n\r\n"), rowsinserted, tablename);
		break;
	}
	
	//post 8.01
    //InvalidateRect(param->m_hwnddlg, NULL, FALSE );
    //UpdateWindow(param->m_hwnddlg);

    while(PeekMessage(&message, pGlobals->m_pcmainwin->m_hwndmain, WM_PAINT, WM_PAINT, PM_NOREMOVE))
    {
        if(pGlobals->m_pcmainwin->m_hwndtooltip && (/*message.message == WM_LBUTTONDBLCLK ||*/ message.message == WM_LBUTTONDOWN))
        {
            FrameWindow::ShowQueryExecToolTip(wyFalse);
        }

        TranslateMessage(&message);
        DispatchMessage(&message);
    }	
}
// copy views
wyBool 
CopyDatabase::ExportView(const wyChar *db, const wyChar *view)
{
	 wyString       query, tquery;
	 MYSQL_RES		*res, *tgtres;
	 MYSQL_ROW		row;
     wyString pattern("DEFINER=`.*`@`.*`\\sSQL\\sSECURITY");
	query.Sprintf("drop table if exists `%s`.`%s`", m_targetdb.GetString(), view);

	res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
    
    if(res)
        m_newtargettunnel->mysql_free_result(res);

	if(m_dropobjects)
	{
		query.Sprintf("drop view if exists `%s`.`%s`", m_targetdb.GetString(), view);

        res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
		if(!res && (m_newtargetmysql)->affected_rows == -1)
        {
			ShowMySQLError(m_hwnddlg, m_newtargettunnel, &m_newtargetmysql, query.GetString());
			return wyFalse;
		}

		m_newsrctunnel->mysql_free_result(res);
	}

    query.Sprintf("show create table `%s`.`%s`", db, view);

    res = SjaExecuteAndGetResult(m_newsrctunnel, &m_newsrcmysql, query);
    if(!res)
    {
		ShowMySQLError(m_hwnddlg, m_newsrctunnel, &m_newsrcmysql, query.GetString());
		return wyFalse;
	}

    row = m_newsrctunnel->mysql_fetch_row(res);
	
	tquery.SetAs(row[1]);
	if(m_isremdefiner)
		RemoveDefiner(tquery, pattern.GetString(), 12);
    //StripDatabase(tquery, m_srcdb.GetString());

    tgtres = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, tquery);
		
	if((!tgtres) && (m_newtargetmysql->affected_rows == -1))
	{
		ShowMySQLError(m_hwnddlg, m_newtargettunnel, &m_newtargetmysql, tquery.GetString());
        m_newsrctunnel->mysql_free_result(res);
		return wyFalse;
	}

	m_newsrctunnel->mysql_free_result(res);
    m_newtargettunnel->mysql_free_result(tgtres);

	m_gui_routine((void*)m_gui_lparam, (wyChar*)view, 0, wyFalse, VIEWCOPIED);	
			
	return wyTrue;
}


wyBool 
CopyDatabase::ExportSP(const wyChar *db, const wyChar *sp, wyBool isproc)
{
	 wyString       query, tquery,strmsg;
	 MYSQL_RES		*res, *tgtres;
	 MYSQL_ROW		row;
	 wyString pattern("DEFINER=`.*`@`.*`\\s");
	 wyInt32 extra;
	 if(m_dropobjects)
	 {
		if(isproc)
		{
			pattern.AddSprintf("PROCEDURE");
			extra = 9;
			query.Sprintf("drop procedure if exists `%s`.`%s`", m_targetdb.GetString(), sp);
		}
		else
		{
			pattern.AddSprintf("FUNCTION");
			extra = 8;
			query.Sprintf("drop function if exists `%s`.`%s`", m_targetdb.GetString(), sp);
		}

        res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
	    if(res)
            m_newtargettunnel->mysql_free_result(res);
	 }

	 else
	 {
		if(isproc)
		{
			pattern.AddSprintf("PROCEDURE");
			extra = 9;
		}
		else
		{
			pattern.AddSprintf("FUNCTION");
			extra = 8;
		 }
	 
	 }

	 if(isproc)
	 {
		query.Sprintf("show create procedure `%s`.`%s`", db, sp);
	 }
	 else
	 {
		query.Sprintf("show create function `%s`.`%s`", db, sp);
	 }
	 
	res = SjaExecuteAndGetResult(m_newsrctunnel, &m_newsrcmysql, query);
	if(!res)
	{
		ShowMySQLError(m_hwnddlg, m_newsrctunnel, &m_newsrcmysql, query.GetString());
		return wyFalse;
	}

	row = m_newsrctunnel->mysql_fetch_row(res);

    if(row == NULL )
        return wyFalse;
	if(row[2] == NULL)
	{
		strmsg.SetAs(PROCEDURE_FUNC_ERRMSG);
		yog_message(m_hwnddlg, strmsg.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
		return wyFalse;
	}

	tquery.SetAs(row[2]);
	if(m_isremdefiner)
		RemoveDefiner(tquery, pattern.GetString(), extra);	
	//StripDatabase(tquery, m_srcdb.GetString());
	
    tgtres = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, tquery);
	if(!tgtres && m_newtargetmysql->affected_rows == -1)
    {
		ShowMySQLError(m_hwnddlg, m_newtargettunnel, &m_newtargetmysql, tquery.GetString());
		m_newsrctunnel->mysql_free_result(res);
		return wyFalse;
	}

	m_newsrctunnel->mysql_free_result(res);
	m_newtargettunnel->mysql_free_result(tgtres);

	if(isproc == wyTrue)
		m_gui_routine((void*)m_gui_lparam,(wyChar*)sp, 0, wyFalse, PROCEDURECOPIED);
	else
		m_gui_routine((void*)m_gui_lparam,(wyChar*)sp, 0, wyFalse, FUNCTIONCOPIED);

			
	return wyTrue;
}
// copying  event
wyBool
CopyDatabase::ExportEvent(const wyChar *db, const wyChar *event)
{
	MYSQL_RES	 *res;
	wyString     query, eventstr;	
	MYSQL_ROW    row;
	wyInt32      index = 0;
	wyString	pattern("DEFINER=`.*`@`.*`\\sEVENT");
	if(m_dropobjects == wyTrue)
	{  
		//drop event if event is exists in the target 
		query.Sprintf("drop event if exists `%s`.`%s`", m_targetdb.GetString(), event);

		res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);

		if(!res && (m_newtargetmysql)->affected_rows == -1)
        {
			ShowMySQLError(m_hwnddlg, m_newtargettunnel, &m_newtargetmysql, query.GetString());
			return wyFalse;
		}

		m_newsrctunnel->mysql_free_result(res);
	}

	query.Sprintf("show create event `%s`.`%s`;", db, event);	

	res = SjaExecuteAndGetResult(m_newsrctunnel, &m_newsrcmysql, query);

	if(!res && m_newsrcmysql->affected_rows == -1)
    {
		ShowMySQLError(m_hwnddlg, m_newsrctunnel, &m_newsrcmysql, query.GetString());
		return wyFalse;
	}
	
	row = m_newsrctunnel->mysql_fetch_row(res);
	// getting  exact row field of a create event statement
	index = GetFieldIndex(m_newsrctunnel, res, "Create Event");

	
	if(row && index == -1)
		return wyFalse;
	
	eventstr.SetAs(row[index]);
	if(m_isremdefiner)
		RemoveDefiner(eventstr, pattern.GetString(), 5);
	res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, eventstr);

	if(!res && (m_newtargetmysql)->affected_rows == -1)
    {
		ShowMySQLError(m_hwnddlg, m_newtargettunnel, &m_newtargetmysql, eventstr.GetString());
		return wyFalse;
	}

	m_newsrctunnel->mysql_free_result(res);

	m_gui_routine((void*)m_gui_lparam, (wyChar*)event, 0, wyFalse, EVENTCOPIED);	
	return wyTrue;

}

wyBool 
CopyDatabase::ExportTrigger(const wyChar *db, const wyChar *triggername)
{
	MYSQL_RES	*res = NULL, *tgtres = NULL,*myres1=NULL;
	MYSQL_ROW	 myrow,myrow1=NULL;
	wyString    query,query1, row0str, row1str, trigger, row3str, row4str,bodyoftrigger;
	wyBool      isshowcreateworked=wyTrue;
	 //There are mutiple issues with show triggers and show create trigger in MySQL servers--
	//with show create trigger there are following issues-
	//1-SHOW CREATE TRIGGER was added in MySQL 5.1.21 so it will not work for older versions
	//2-This bug reprot-http://bugs.mysql.com/bug.php?id=58996
	//With show triggers there are following issues--
	//1-Curruption of quotes refer-http://forums.webyog.com/index.php?showtopic=7625&hl= and http://bugs.mysql.com/bug.php?id=75685.
	//So now here is the logic for getting things correct up to maximum extent--
	//fire both queries and
	//First try to use show create trigger if query works then get the body of trigger from the result
	//if show create trigger fails use show triggers--because user can have trigger without quotes
	//Below is the implementation of this logic.
	 if(m_dropobjects)
	 {
		 //drop trigger if trigger is exists in the target 
		 query.Sprintf("drop trigger if exists `%s`.`%s`", m_targetdb.GetString(), triggername);
		  
		 res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
	    
		 if(res)
			m_newtargettunnel->mysql_free_result(res);
	 }

	query.Sprintf("show triggers from `%s` where `trigger` = '%s'", db, triggername);
    res = SjaExecuteAndGetResult(m_newsrctunnel, &m_newsrcmysql, query);
	trigger.SetAs(triggername);
	//bug http://bugs.mysql.com/bug.php?id=75685. We have to fire show create trigger to get the body of trigger
	query1.Sprintf("show create trigger `%s`. `%s`", db, triggername);
	myres1 = SjaExecuteAndGetResult(m_newsrctunnel,  &m_newsrcmysql, query1);
	if(!res)
	{
		ShowMySQLError(m_hwnddlg, m_newsrctunnel, &m_newsrcmysql, query.GetString());
		return wyFalse;
	}
	if(!myres1)
	{
		isshowcreateworked=wyFalse;
	}
	if(m_newsrctunnel->mysql_num_rows(res) == 0)
	{
		m_newsrctunnel->mysql_free_result(res);
		if(myres1)
			 m_newsrctunnel->mysql_free_result(myres1);
		return wyFalse;
	}
	
	myrow = m_newsrctunnel->mysql_fetch_row(res);
	if(isshowcreateworked)
	myrow1=m_newsrctunnel->mysql_fetch_row(myres1);
		//body of trigger
	if(isshowcreateworked && myrow1[2])
	{
	bodyoftrigger.SetAs(myrow1[2]);
	if(GetBodyOfTrigger(&bodyoftrigger)==-1)
		{
         
		  m_newsrctunnel->mysql_free_result(res);
		  if(myres1)
			   m_newsrctunnel->mysql_free_result(myres1);
           return wyFalse;

	     }
	}
	if(myrow && myrow[0] && myrow[1] && myrow[3]&& myrow[2] && myrow[4])
    {
		if(isshowcreateworked && bodyoftrigger.GetLength()!=0)
       		query.Sprintf("CREATE TRIGGER `%s` %s %s ON `%s`.`%s` FOR EACH ROW %s", 
				trigger.GetString(), myrow[4], myrow[1], m_targetdb.GetString(),  myrow[2], bodyoftrigger.GetString());
		else
           query.Sprintf("CREATE TRIGGER `%s` %s %s ON `%s`.`%s` FOR EACH ROW %s", 
				trigger.GetString(), myrow[4], myrow[1], m_targetdb.GetString(),  myrow[2], myrow[3]);
			
    }
    else
    {
		  m_newsrctunnel->mysql_free_result(res);
		  if(myres1)
		  m_newsrctunnel->mysql_free_result(myres1);
          return wyFalse;
    }

	tgtres = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
		
	if(!tgtres && (m_newtargetmysql)->affected_rows == -1)
	{
		ShowMySQLError(m_hwnddlg, m_newtargettunnel, &m_newtargetmysql, query.GetString());
		m_newsrctunnel->mysql_free_result(res);
		return wyFalse;
	}
		m_newtargettunnel->mysql_free_result(tgtres);

	// for showing Trigger copied message
	m_gui_routine((void*)m_gui_lparam, (wyChar*)triggername, 0, wyFalse, TRIGGERCOPIED);	
	
	m_newsrctunnel->mysql_free_result(res);
	m_newsrctunnel->mysql_free_result(myres1);

	return wyTrue;
}


wyBool
CopyDatabase::CheckTargetDB(HWND m_hwnddlg)
{
	LPDIFFCOMBOITEM	lpdiff;//lpdiff2;
	wyInt32         index,ret;//, i=0
	//wyWChar     buffer[SIZE_128];
	wyString	activevalue;
	MDIWindow           *wnd = GetActiveWin();
	/*	fixed a bug #281. It might be that a user has only one connection with only one
		db. this time, CB_ERR will be returned */
	//11.52: one connection with only one db will be allowed! 
	index = SendMessage(m_hwndcombo, CB_GETCURSEL, 0, 0);

	if(index == CB_ERR)
    {
		EnableWindow(GetDlgItem(m_hwnddlg, IDOK),FALSE);
		DisableAll(m_hwnddlg);
		return wyFalse;
	}
	VERIFY(lpdiff =(LPDIFFCOMBOITEM)SendMessage(m_hwndcombo, CB_GETITEMDATA, index, 0));
	SendMessage(m_hwndcombodb, CB_RESETCONTENT , 0, 0);
	//SendMessage(m_hwndcombodb, CB_SETCURSEL , -1, 0);
	//while(SendMessage(m_hwndcombodb, CB_DELETESTRING , i++, 0) != CB_ERR);
	//index = SendMessage(m_hwndcombodb, CB_GETCURSEL, 0, 0);
	//VERIFY(lpdiff2 =(LPDIFFCOMBOITEM)SendMessage(m_hwndcombodb, CB_GETITEMDATA, index, 0));
	//11.52: Get target db from database combo
	//targetdb could be NULL in 2 cases :
	//case 1 : the only db is the source db during initialising the combos. 
	//case 2 : user types the value in the combo.

	//FreedbComboParam();
	activevalue.Sprintf("%s", m_srcdb.GetString());
	DBListBuilder::GetDBFromActiveWinscopydb(lpdiff->wnd->m_hwnd, (LPARAM)m_hwndcombodb);
	if(lpdiff->wnd == wnd )
	{
		ret = SendMessage(m_hwndcombodb, CB_FINDSTRINGEXACT, -1,(LPARAM)activevalue.GetAsWideChar());
		if(ret != CB_ERR)
			SendMessage(m_hwndcombodb, CB_DELETESTRING, ret, 0);
	}
	FillDbList();
	//GetDlgItemText(m_hwnddlg, IDC_SOURCEDB2, buffer, SIZE_128 - 1);
    //m_targetdb.SetAs(buffer);
    //m_targetdb.RTrim();
	//index = SendMessage(m_hwndcombodb, CB_FINDSTRINGEXACT, -1,(LPARAM)m_targetdb.GetAsWideChar());
	//if(index != CB_ERR)
	//{
		//m_iscreatedb = wyFalse;
	//}
	//else
	//{
		//m_iscreatedb = wyTrue;
	//}

	SendMessage(m_hwndcombodb, CB_SETCURSEL , m_defaultdbindex , 0);
	m_defaultdbindex = 0;
	VERIFY(m_targetmysql = lpdiff->mysql);
	VERIFY(m_targettunnel = lpdiff->tunnel);
	VERIFY(m_tgtinfo = lpdiff->info);
	m_istrg5xobjects = IsMySQL5010(m_targettunnel, m_targetmysql);
	m_istrgeventsupports  = IsMySQL516(m_targettunnel, m_targetmysql);	

	if(m_issrc5xobjects == wyFalse|| m_istrg5xobjects == wyFalse)
    {
        m_is5xobjects = wyFalse;
		DisableAll(m_hwnddlg);
    }
	else
	{
        m_is5xobjects = wyTrue;
		EnableWindow(GetDlgItem(m_hwnddlg, IDC_CHK_DROPOBJECTS), TRUE);		
	}
	return wyTrue;
}

void
	CopyDatabase::FillDbList()
{
	wyString temp;
	wyString temp1;
	wyWChar     lbStr[70] = {0};
	temp.SetAs(m_srcdb.GetString());
		
	if(m_dblist)
	{
		free(m_dblist);
		m_countdb = 0;
	}
	
	m_countdb = SendMessage(m_hwndcombodb, CB_GETCOUNT, 0, 0);
	m_dblist = (DB_LIST*)calloc(m_countdb,sizeof(DB_LIST));

	for(int i=0;i< m_countdb ; i++)
	{
	SendMessage(m_hwndcombodb, CB_GETLBTEXT , i, (LPARAM)lbStr);
	m_dblist[i].m_dbname.SetAs(lbStr);
	temp1.SetAs(lbStr);
	m_dblist[i].m_dropdown = wyTrue;
	if(temp.CompareI(temp1.GetString())==0)
	{
		m_defaultdbindex = i;
	}
	}
}
wyBool 
CopyDatabase::CreateTemporaryTables(const wyChar *db, const wyChar *view)
{
	wyString        query;
	MYSQL_RES		*res, *tgtres;
	MYSQL_ROW		row;

	MDIWindow		*wnd = GetActiveWin();
	wyString pattern("DEFINER=`.*`@`.*`\\sSQL\\sSECURITY");
	
	 /* Create a dummy table for the view. ie. a table  which has the
           same columns as the view should have. This table is dropped
           just before the view is created. The table is used to handle the
           case where a view references another view, which hasn't yet been
           created(during the load of the dump). BUG#10927 */

        /* Create temp table by selecting from the view */
        query.Sprintf("CREATE TEMPORARY TABLE `%s`.`%s` SELECT * FROM `%s`.`%s` WHERE 0",
                    db, view, db, view);
		
        //res = SjaExecuteAndGetResult(m_newsrctunnel, m_newsrcmysql, query);		
		
		res = ExecuteAndGetResult(wnd, m_newsrctunnel, &m_newsrcmysql, query, wyFalse, wyTrue);

		if(!res && m_newsrctunnel->mysql_affected_rows(m_newsrcmysql)== -1)
		{
			ShowMySQLError(m_hwnddlg, m_newsrctunnel, &m_newsrcmysql, query.GetString());
			return wyFalse;
		}

        m_newsrctunnel->mysql_free_result(res);

        /* Get CREATE statement for the temp table */
		query.Sprintf("SHOW CREATE TABLE `%s`.`%s`", db, view);	
                /*bug*/		
		/* here through tunnel connection "SHOW CREATE TABLE" query is not giving create statement 
		 of temporaryt table because before it was not a batch process now we made it as a batch process.*/
        
		//res = SjaExecuteAndGetResult(m_newsrctunnel, m_newsrcmysql, query);
		res = ExecuteAndGetResult(wnd, m_newsrctunnel, &m_newsrcmysql, query, wyFalse, wyTrue, wyFalse, wyFalse, wyTrue);

        if(!res)
		{
			ShowMySQLError(m_hwnddlg, m_newsrctunnel, &m_newsrcmysql, query.GetString());
			return wyFalse;
		}
        
        row = m_newsrctunnel->mysql_fetch_row(res);

		query.Sprintf("DROP TABLE IF EXISTS `%s`;", view);

        tgtres = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);

        if(tgtres)
		    m_newsrctunnel->mysql_free_result(tgtres);

        if(m_dropobjects)
		{
			query.Sprintf("DROP VIEW IF EXISTS `%s`;", view);

            tgtres = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);

			if(tgtres)
				m_newsrctunnel->mysql_free_result(res);

			m_newsrctunnel->mysql_free_result(tgtres);
		}		

        /* Print CREATE statement but remove TEMPORARY */
        query.Sprintf("\nCREATE %s;\n", row[1]+17);
	

		m_newsrctunnel->mysql_free_result(res);
		if(m_isremdefiner)
			RemoveDefiner(query, pattern.GetString(), 12);
        res = SjaExecuteAndGetResult(m_newtargettunnel, &m_newtargetmysql, query);
        
        if(!res && (m_newtargetmysql)->affected_rows == -1)
            ShowMySQLError(m_hwnddlg, m_newtargettunnel, &m_newtargetmysql, query.GetString());
        else
        	m_newsrctunnel->mysql_free_result(res);

        /* Drop the temp table */
		query.Sprintf("DROP TEMPORARY TABLE `%s`.`%s`", db, view);

        res = SjaExecuteAndGetResult(m_newsrctunnel, &m_newsrcmysql, query);

        if(res)
            m_newsrctunnel->mysql_free_result(res);
        
		return wyTrue;
}

void 
CopyDatabase::DisableAll(HWND hwnddlg)
{
	SendMessage(GetDlgItem(hwnddlg, IDC_CHK_DROPOBJECTS), BM_SETCHECK, FALSE, 0);
	EnableWindow(GetDlgItem(hwnddlg, IDC_CHK_DROPOBJECTS), FALSE);
}

void
CopyDatabase::EnableDlgWindows(wyBool enable)
{
    wyInt32 state = (enable == wyTrue)?TRUE:FALSE;

    EnableWindow(GetDlgItem(m_hwnddlg, IDC_TREE), state);
    EnableWindow(GetDlgItem(m_hwnddlg, IDC_SELALL2), state);
    EnableWindow(GetDlgItem(m_hwnddlg, IDC_UNSELALL2), state);
    EnableWindow(GetDlgItem(m_hwnddlg, IDC_SOURCEDB), state);
    EnableWindow(GetDlgItem(m_hwnddlg, IDC_DROP), state);
    EnableWindow(GetDlgItem(m_hwnddlg, IDC_STRUCDATA), state);
    EnableWindow(GetDlgItem(m_hwnddlg, IDC_STRUC), state);
	EnableWindow(GetDlgItem(m_hwnddlg, IDC_CPY_BULKINSERT), state);
	EnableWindow(GetDlgItem(m_hwnddlg, IDC_CPY_DEFINER), state);
	EnableWindow(GetDlgItem(m_hwnddlg, IDC_CPY_ASCTRIGGERS), state);
    if(m_is5xobjects == wyTrue)
        EnableWindow(GetDlgItem(m_hwnddlg, IDC_CHK_DROPOBJECTS), state);
    
    EnableWindow(GetDlgItem(m_hwnddlg, IDCANCEL), state);

    if(state)
    {
        SetWindowText(GetDlgItem(m_hwnddlg, IDOK), _(L"&Copy"));
        EnableWindow(GetDlgItem(m_hwnddlg, IDOK), TRUE);
    }
    else
	{

        SetWindowText(GetDlgItem(m_hwnddlg, IDOK), _(L"&Stop"));
		SetWindowText(GetDlgItem(m_hwnddlg, IDC_INVISIBLE), _(L"invis"));
		SetFocus(GetDlgItem(m_hwnddlg, IDC_INVISIBLE));

	}

	if(m_ispromtstorepgrmmessage == wyTrue)
		MessageBox(m_hwnddlg, COPIED_STOREDPGRMS_MANUALADJUST, pGlobals->m_appname.GetAsWideChar(), 
					 MB_TASKMODAL | MB_OK | MB_ICONINFORMATION);


    return;
}

wyBool 
CopyDatabase::IsCopyStopped()
{
    wyBool ret;
    // Request ownership of the critical section.
    EnterCriticalSection(&m_cs);

    // Access the shared resource.

    ret = (m_stopcopying == wyTrue)?wyTrue:wyFalse;

     // Release ownership of the critical section.
    LeaveCriticalSection(&m_cs);

    return ret;
}

void 
CopyDatabase::StopCopy()
{
    // Request ownership of the critical section.
    EnterCriticalSection(&m_cs);

    // Access the shared resource.

    m_stopcopying = wyTrue;

     // Release ownership of the critical section.
    LeaveCriticalSection(&m_cs);

    return ;
}

HTREEITEM 
CopyDatabase::addEntry(HWND htree, HTREEITEM parent, wyWChar *text, wyInt32 icontype)
{
	wyString         object;
	TV_INSERTSTRUCT  str;  

	object.SetAs(text);

	str.hParent = parent;
	str.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE	;
	str.hInsertAfter = TVI_LAST;
	str.item.iImage = icontype;
	str.item.iSelectedImage = icontype;

	str.item.pszText = (wyWChar*)object.GetAsWideChar();

	return ((HTREEITEM) SendMessage(htree, TVM_INSERTITEM, NULL, (long)&str));

}
wyBool
CopyDatabase::CreateImageList()
{
	wyInt32     ret, imagecount;
	HICON       hbmp;
	
	HWND		hwndtree;
		
	wyInt32		imgres[] = {IDI_SERVER, IDI_DATABASE, IDI_OPEN, IDI_TABLE, IDI_COLUMN, 
							IDI_INDEX, IDI_OPEN,IDI_PROCEDURE, IDI_OPEN, IDI_FUNCTION,
							IDI_OPEN, IDI_VIEW, IDI_OPEN, IDI_TRIGGER, IDI_OPEN, IDI_EVENT, IDI_OPEN};


	VERIFY(m_himl = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK , 6, 0));
	hwndtree = GetDlgItem(m_hwnddlg, IDC_TREE);

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
// To disable bulkinsert when only structure is checked. 
void
CopyDatabase::EnableDisable(wyInt32 id)
{
	wyInt32	state;
	
	switch(id)
	{
	case IDC_STRUC:
		{
			state = SendMessage(GetDlgItem(m_hwnddlg, IDC_STRUC), BM_GETCHECK, 0, 0);			
			EnableWindow(GetDlgItem(m_hwnddlg, IDC_CPY_BULKINSERT), (state == BST_CHECKED) ? FALSE : TRUE);		   		
		}
		break;

	case IDC_STRUCDATA:
		{
			state = SendMessage(GetDlgItem(m_hwnddlg, IDC_STRUCDATA), BM_GETCHECK, 0, 0);

	        if(state == BST_CHECKED)
				EnableWindow(GetDlgItem(m_hwnddlg, IDC_CPY_BULKINSERT), TRUE);					
		}
		break;
	}
}

void
CopyDatabase::GetCtrlRects()
{
    RECT    rect;
    wyInt32 i, count;
    HWND    hwnd;
	
	//ctrlid    size-x     size-y
    wyInt32 ctrlid[] = {
       	IDC_GROUP1, 1, 1,
        //IDC_STATICTITLE2, 0, 0,
        //IDC_TARGETDB, 0, 0,
        //IDC_STATICTITLE3, 0, 0,
        IDC_TREE, 1, 1,
        IDC_GROUP2, 1, 0,
        IDC_STATIC_NOTE, 1, 0,
		IDC_STATIC_NOTE2, 1, 0,
		IDC_STATIC_NOTE3, 1, 0,
        IDC_SOURCEDB, 1, 0,
		IDC_SOURCEDB2, 1, 0,
        IDC_GROUP3, 1, 0,
        IDC_STRUCDATA, 0, 0,
        IDC_STRUC, 0, 0,
        IDC_GROUP4, 1, 0,
        IDC_DROP, 0, 0,
        IDC_CPY_BULKINSERT, 0, 0,
		IDC_CPY_DEFINER, 0, 0,
		IDC_CPY_ASCTRIGGERS, 0, 0,
        IDC_SUMMARY, 0, 0,
		IDC_INVISIBLE, 0, 0,
        IDOK, 0, 0,
        IDCANCEL, 0, 0,
        IDDONE, 0, 0,
        IDC_MESSAGE, 1, 0,
		IDC_COPYDATA_GRIP, 0, 0,
    };

	count = sizeof(ctrlid)/sizeof(ctrlid[0]);

    //store the control handles and related information in the linked list
    for(i = 0; i < count; i+=3)
    {
		hwnd = GetDlgItem(m_hwnddlg, ctrlid[i]);
        GetWindowRect(hwnd, &rect);
		MapWindowPoints(NULL, m_hwnddlg, (LPPOINT)&rect, 2);
        m_controllist.Insert(new DlgControl(hwnd, 
                                            ctrlid[i], 
                                            &rect, 
                                            ctrlid[i + 1] ? wyTrue : wyFalse, 
                                            ctrlid[i + 2] ? wyTrue : wyFalse));
    }
}

void
CopyDatabase::PositionCtrls()
{
    DlgControl* pdlgctrl;
    wyInt32     leftpadding, toppadding, rightpadding, bottompadding, width, height;
    wyInt32     x, y;
    RECT        rect;
    HDWP        hdefwp;
		
	GetClientRect(m_hwnddlg, &rect);

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
			switch(pdlgctrl->m_id)
			{
				case IDC_STATIC_NOTE:
				case IDC_STRUCDATA:
				case IDC_STRUC:
				case IDC_DROP:
				case IDC_CPY_BULKINSERT:
				case IDC_CPY_DEFINER:
			    case IDC_CPY_ASCTRIGGERS:
					x = (rect.right - rect.left)/2 + 8 ;
					break;
				case IDC_SUMMARY:
				case IDC_INVISIBLE:
				case IDOK:
				case IDCANCEL:
				case IDDONE:
					x=rect.right - rightpadding - width;
					break;

				case IDC_COPYDATA_GRIP:
					x = rect.right - width;
					break;

				default:
					x = leftpadding;
			}
        }
        else
        {
			switch(pdlgctrl->m_id)
			{
				case IDC_GROUP1:
					x = leftpadding;
					width = (rect.right - rect.left)/2 - leftpadding - 7;
					break;
				case IDC_TREE:
					x = leftpadding;
					width = (rect.right - rect.left)/2 - leftpadding - 18;
					break;

				case IDC_STATIC_NOTE:
				case IDC_STATIC_NOTE2:
				case IDC_STATIC_NOTE3:
				case IDC_SOURCEDB2:
				case IDC_SOURCEDB:
					x = (rect.right - rect.left)/2 + 8 ;
					width = (rect.right - rect.left)/2 - rightpadding - 10;
					break;

				case IDC_MESSAGE:
					x = leftpadding;
					width = rect.right - rect.left - leftpadding - rightpadding;
					break;

				default:
					x = (rect.right - rect.left)/2 - 1 ;
					width = (rect.right - rect.left)/2 - rightpadding + 1;
					break;
			}
        }
	    switch(pdlgctrl->m_id)
        {
			case IDOK:
			case IDC_SUMMARY:
			case IDC_INVISIBLE:
			case IDDONE:
			case IDCANCEL:
			case IDC_MESSAGE:
                y = rect.bottom - bottompadding - height;
                break;

			case IDC_COPYDATA_GRIP:
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
CopyDatabase::CopyDbResize(HWND hwnd){

	PositionCtrls();

	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);
}

void
CopyDatabase::OnWMSizeInfo(LPARAM lparam)
{
    MINMAXINFO* pminmax = (MINMAXINFO*)lparam;

    pminmax->ptMinTrackSize.x = m_wndrect.right - m_wndrect.left+20;
    pminmax->ptMinTrackSize.y = m_wndrect.bottom - m_wndrect.top+20;
}

void
CopyDatabase::OnPaint(HWND hwnd)
{
    HDC         hdc;
	PAINTSTRUCT ps;
	HWND hwndstat = GetDlgItem(hwnd, IDC_COPYDATA_GRIP);
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