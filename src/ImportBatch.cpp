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

#include "FrameWindowHelper.h"
#include "ImportBatch.h"
#include "ImportFromSQL.h"
#include "CommonHelper.h"
#include "GUIHelper.h"

extern	PGLOBALS		 pGlobals;

#define	IMPORTJOB		"ImportBatch"

ImportBatch::ImportBatch()
{
	m_stopimport = wyFalse;
	m_importing  = wyFalse;
	m_p = new Persist;
	m_p->Create("EXECUTEBATCH");
	memset(m_curdb, 0, sizeof(m_curdb));
}

ImportBatch::~ImportBatch()
{
}

wyBool
ImportBatch::Create(HWND hwnd, Tunnel * tunnel, PMYSQL mysql)
{
	wyInt64 ret;
	m_umysql = mysql;
	m_tunnel = tunnel;
	m_hwndparent = hwnd;

	//Post 8.01
    //RepaintTabModule();
	
	ret = (wyInt64)DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_IMPORTBATCH),
							hwnd, ImportBatch::DlgProc, (LPARAM)this);

    if(ret == wyTrue )
	{
		/*if(m_tunnel->IsTunnel())
		{
			GetDlgItem(
		}*/
		return wyTrue;
	}
    else
        return wyFalse;
}

INT_PTR CALLBACK	
ImportBatch::DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ImportBatch * pcib = (ImportBatch*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
        LocalizeWindow(hwnd);
		SendMessage(hwnd, WM_INITEXPORTDATA, 0, 0);
		break;

	case WM_INITEXPORTDATA:
        pcib->InitilizeExportData(hwnd);
		break;

	case UM_AFTEROK:
		{
			if(pcib->m_importing) 
    			pcib->m_stopimport = wyTrue;
	        else
            {
    			pcib->ExecuteBatch();
				pGlobals->m_pcmainwin->ChangeDBInCombo(pcib->m_tunnel, pcib->m_umysql);
            }
		}
		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/117-excute-sql-script");
		return wyTrue;

	case WM_COMMAND:
        pcib->OnWMCommand(hwnd, wParam);
		break;

	case WM_DESTROY:
		delete pcib->m_p;
		break;

	case WM_MOUSEMOVE:
		pcib->OnWMMouseMove(hwnd, lParam);
		break;

	default:
		break;
	}	
	return wyFalse;
}

void 
ImportBatch::InitilizeExportData(HWND hwnd)
{
    m_p->Add(hwnd, IDC_EXPORTFILENAME, "FILENAME", "", TEXTBOX);
	VERIFY(m_hwnd = hwnd);
	VERIFY(m_hwndedit = GetDlgItem(hwnd, IDC_EXPORTFILENAME));
	if(!m_tunnel->IsTunnel())
	{
		SendMessage(GetDlgItem(hwnd, IDC_CHECKBOX),BM_SETCHECK,BST_CHECKED,0);
	}
	else
	{
		ShowWindow(GetDlgItem(hwnd, IDC_CHECKBOX), SW_HIDE);
	}
	SetCurDbFont();
	GetCurDatabase();
	
}

void 
ImportBatch::OnWMCommand(HWND hwnd, WPARAM wParam)
{
    switch LOWORD(wParam)
	{	
	case IDC_EXPFILESELECT:
		SetExpFileName();
		break;

	case IDOK:
		{
#ifndef COMMUNITY
		MDIWindow* wnd = GetActiveWin();
		wyInt32 presult = 6;
		wyInt32			isintransaction = 1;

			if(wnd->m_ptransaction && wnd->m_ptransaction->m_starttransactionenabled == wyFalse)
			{
				if(pGlobals->m_pcmainwin->m_topromptonimplicit)
						presult = MessageBox(GetActiveWindow(), _(L"You have an active transaction. This operation will cause Transaction to commit and end. Do you want to Continue?"), 
						_(L"Warning"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
			if(presult == 6)
			{
				wyString query;
				query.Sprintf("COMMIT");
				wnd->m_ptransaction->m_implicitcommit = wyTrue;
				ExecuteAndGetResult(wnd, wnd->m_tunnel, &wnd->m_mysql, query, wyFalse, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction);
				
				if(isintransaction == 1)
					break;
			} 
			}
			if(presult == 6)
#endif
			ImportDump();
		}

		break;	

	case IDDONE:
	case IDCANCEL:
		yog_enddialog(hwnd, 0);
		break;
	}
}

/*Importing process
Also if select a db in OB and use 'Restore from SQL dump' warning pops up
*/
void 
ImportBatch::ImportDump()
{
	MDIWindow *wnd;
	HTREEITEM	hitem = NULL;
	wyString	msg, dbname;
	wyInt32		ret = 0;

	if((SendMessage(m_hwndedit, WM_GETTEXTLENGTH, 0, 0)) > 0)
    {
		VERIFY(wnd = GetActiveWin());

		//Check whether 'Restore from SQL dump' used after selecting a db in OB
		if(m_importing == wyFalse && wnd && wnd->m_pcqueryobject && wnd->m_pcqueryobject->GetSelectionImage() == NDATABASE)
		{
			hitem = wnd->m_pcqueryobject->GetDatabaseNode();
			wnd->m_pcqueryobject->GetDatabaseName(hitem);

			dbname.SetAs(wnd->m_pcqueryobject->m_seldatabase);
		    
			//warning shows when restore from sql dump does database(to avoid confusion of USE db in dump)
			if(dbname.GetLength())
			{
				msg.Sprintf(_("The current database context is `%s`. SQL statements in the file will be executed here, unless one or more USE statement(s) specifying another database context is in the file. Do you want to continue?"),
								dbname.GetString()); 

				ret = MessageBox(wnd->m_hwnd, msg.GetAsWideChar(),pGlobals->m_appname.GetAsWideChar(), MB_ICONQUESTION | MB_YESNO);
				if(ret != IDYES)
					return;
			}
		}
		
		if(m_importing) 
        {
			/* behave as if the stop has been pushed */
//			DEBUG_LOG("stopping query");
			m_stopimport = wyTrue;
		} 
        else 
        {
//			DEBUG_LOG("executing import");
			VERIFY(wyFalse == m_importing);
			SetCursor(LoadCursor(NULL, IDC_WAIT));
			ExecuteBatch();
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		}
	}
	else
		yog_message(m_hwnd, _(L"Please enter a filename"), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);

	return;
}

void 
ImportBatch::OnWMMouseMove(HWND hwnd, LPARAM lParam)
{
    if(m_importing)
	{
		RECT		rect;
		POINT		pt;

		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);
		
		GetWindowRect(GetDlgItem(hwnd, IDOK), &rect);
		MapWindowPoints(hwnd, NULL, &pt, 1); 			

		if(PtInRect(&rect, pt))
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		else
			SetCursor(LoadCursor(NULL, IDC_WAIT));
	}
}


/// Function starts the execution of batch.

/* from 4.1 we perform the import in a different thread so that a user can cancel
	an import process and also the GUI dosnt get disrupted 

	The basic idea is that we create an event and send the import process in
	another thread.

	after the import is over the event will be signalled and we will come out of the
	message loop which is written down.

	the idea is not to disrupt GUI and allow the user to cancel a import process. */

wyBool 
ImportBatch::ExecuteBatch()
{
	wyString        msg;
	wyUInt32		timetaken;
	wyUInt32		thdid;
	HANDLE			impthd = NULL;
	ImportFromSQL	import;

	MDIWindow		*wnd = NULL;
    
	IMPORTBATCH		evt = {0};
	IMPORTPARAM		param = {0};

//	DEBUG_ENTER ( "Executebatch" );

	VERIFY(wnd = GetActiveWin());

	/* set the yogimport and importparam values */
	SetImportValues(&import);
	SetParamValues(&param);

	/* set initial progress bar values */
	if(!(param.m_totalfilesize = SetInitProgressValues()))
    {
        ShowEmptyFileError(&param);
        return wyFalse;
	}

	param.m_count = 0;
	
	/* disable the button and other windows */
	EnableDlgWindows(wyFalse);
	EnableWindow(GetDlgItem(m_hwnd, IDDONE ), false);

	SetFocus(GetDlgItem(m_hwnd, IDC_PROGRESS));
	/* change text of the ok button */
	ChangeOKButtonText(_(L"S&top"));

	VERIFY(evt.m_impevent	= CreateEvent(NULL, TRUE, FALSE, NULL));
	evt.m_import		= &import;
	evt.m_lpParam		= &param;
	evt.m_stopquery	= (wyBool*)&m_stopimport;
	evt.wnd = wnd;

//	DEBUG_LOG("%u - created", &evt.m_impevent);

	timetaken = GetTickCount();
	m_importing = wyTrue;
//	DEBUG_LOG("Creating thread");
	impthd = (HANDLE)_beginthreadex(NULL, 0, ImportBatch::ImportThread, &evt, 0, &thdid);

	if(!impthd)
    	goto cleanup;

	/* handle all other GUI messages and all in the main thread */
//	DEBUG_LOG("Handling messages");
	HandleMsgs(evt.m_impevent, wyFalse);
	timetaken = GetTickCount() - timetaken;
	VERIFY(CloseHandle(evt.m_impevent));
	VERIFY(CloseHandle(impthd));
    ImportConclude(&evt, &param);
    return wyTrue;

cleanup:
	if(evt.m_impevent)
		VERIFY(CloseHandle(evt.m_impevent));

	if(impthd)
		VERIFY(CloseHandle(impthd));

	m_importing = m_stopimport = wyFalse;
	EnableDlgWindows(wyTrue);
	ChangeOKButtonText(_(L"&Execute"));
	return wyFalse;
}

void 
ImportBatch::ShowEmptyFileError(IMPORTPARAM * param)
{
    SetWindowText(param->m_hwndMsg, _(L"Can't import dump file of size 0."));
	SetWindowText(param->m_hwndSize, L"");
}

void 
ImportBatch::ImportConclude(IMPORTBATCH * evt, IMPORTPARAM * param)
{
    wyString msg;
	wyBool  isdone = wyFalse ;
	
    /* we show complete details of the query execution */
	msg.Sprintf(_("Query(s) Executed: %u\n"), evt->m_num_queries);

	/* show whether file was imported correctly or it was stopped in between */
	if(evt->m_retcode == IMPORTSTOPPED)
	{	
		msg.Add(_("Aborted by user"));
	}
	else 
    {
		switch ( evt->m_retcode )
		{	
			case MYSQLERROR:
				HandleError ();
				msg.Add(_("Import unsuccessful"));		
				break;

			case SYSTEMERROR:
				msg.Add(_("Import unsuccessful. Error opening batch file"));	
				break;

			case FILEERROR:
			    msg.Add(_("Import unsuccessful. Error opening batch file"));	
				break;

			case SUCCESS:
				msg.Add(_("Import successful"));
				isdone = wyTrue;
				break;
				
			case SUCCESSWITHERROR:
				msg.Add(_("Import successful with error(s)"));
				isdone = wyTrue;
				break;
		}
	}

	/* show the message */
	SetWindowText(GetDlgItem(m_hwnd, IDC_MESSAGE), msg.GetAsWideChar());
	
	//If it is succesful then Close button is changed to Done
	ShowWindow(GetDlgItem(m_hwnd, IDCANCEL), !isdone);
	ShowWindow(GetDlgItem(m_hwnd, IDDONE ), isdone);
	EnableWindow(GetDlgItem(m_hwnd, IDDONE ), isdone);

	/* update the size and progressbar with the correct values */
	if(!m_stopimport && (SUCCESS == evt->m_retcode || SUCCESSWITHERROR == evt->m_retcode))
    	UpdateFinishValues(param);
	
    m_importing = m_stopimport = wyFalse;

	/* enable all the necessary windows that had been disabled */
	EnableDlgWindows(wyTrue);
	ChangeOKButtonText(_(L"&Execute"));

	if(GetForegroundWindow() != m_hwnd)
		FlashWindow(pGlobals->m_pcmainwin->m_hwndmain, TRUE);

	if(SUCCESSWITHERROR == evt->m_retcode)//creating the dialog box to tell the user about the error that were there
		HandleError();
	
//	DEBUG_LOG("m_importing = %d", (m_importing)?(1):(0));
}

void
ImportBatch::SetExpFileName ()
{
	HWND			hwndfile;
    wyWChar          strfilename[MAX_PATH+1] = {0};;
	OPENFILENAME	openfilename;

	VERIFY((hwndfile = GetDlgItem(m_hwnd, IDC_EXPORTFILENAME)) != NULL);

	memset(&openfilename, 0, sizeof(openfilename));

	openfilename.lStructSize       = OPENFILENAME_SIZE_VERSION_400;
	openfilename.hwndOwner         = m_hwnd;
	openfilename.hInstance         = pGlobals->m_pcmainwin->GetHinstance();
	openfilename.lpstrFilter       = L"SQL File (*.sql)\0*.sql\0Text (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
	openfilename.lpstrCustomFilter = (LPWSTR) NULL;
	openfilename.nFilterIndex      = 1L;
	openfilename.lpstrFile         = strfilename;
	openfilename.nMaxFile          = MAX_PATH;
	openfilename.lpstrTitle        = _(L"Open");
	openfilename.Flags             = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	
	if(GetOpenFileName(&openfilename))
	{
		SendMessage(hwndfile, WM_SETTEXT, 0, (LPARAM)strfilename);
		return;
	}

	return;
}


unsigned __stdcall
ImportBatch::ImportThread(LPVOID lpParam)
{
	IMPORTBATCH *imp = (IMPORTBATCH*)lpParam;



	imp->m_retcode = imp->m_import->Import(&imp->m_num_bytes, &imp->m_num_queries, 
										 ImportBatch::UpdateGui, 
										 (void*)imp->m_lpParam, 
										 imp->m_stopquery, imp->wnd);

	SetEvent(imp->m_impevent);
//	DEBUG_LOG("%u - signalled", &imp->m_impevent);

	return 0;
}

VOID
ImportBatch::UpdateGui(void * lpParam, wyInt64 num_bytes, wyInt64 num_queries)
{
	IMPORTPARAM *param = (IMPORTPARAM*)lpParam;
	wyString    msg;
    wyString    size;
    wyInt64     c;

    c = (num_bytes * 100) / param->m_totalfilesize;
    
	msg.Sprintf(_("Query(s) executed: %lld"), num_queries);
    size.Sprintf("%lld%%", c);
		
	//Advances the positon of the progressbar when it reaches the muliple of the progressbar increment
	if(max(num_bytes/1024,1) > (param->m_count *((param->m_totalfilesize /1024)/100)) && param->m_count <= 90)
	{	
		param->m_count++;
		SendMessage(param->m_hwndProgress, PBM_STEPIT, 0, 0);
	}
	
	SetWindowText(param->m_hwndMsg, msg.GetAsWideChar());
	SetWindowText(param->m_hwndSize, size.GetAsWideChar());
}

void
ImportBatch::UpdateFinishValues(IMPORTPARAM * param)
{
	wyString    msg;
	wyInt64		fsize;

	fsize = max(param->m_totalfilesize/1024, 1);
	
	msg.AddSprintf("Data size: %lld KB", fsize);

	SetWindowText(param->m_hwndSize, msg.GetAsWideChar());

	SendMessage(param->m_hwndProgress, PBM_SETRANGE32, 0, 1);
	SendMessage(param->m_hwndProgress, PBM_DELTAPOS, (WPARAM)1, 0); 
}

wyInt64
ImportBatch::SetInitProgressValues()
{
	wyWChar		importfile[MAX_PATH+1]={0};
	HWND		progress;
	DWORD		lowfilesize;
	DWORD		highfilesize;
	wyInt64		filesize = 0;
	HANDLE		handle;
	wyUInt32	incr;
	
	/* get the filename from the edit box */
	if(!SendMessage(m_hwndedit, WM_GETTEXT, MAX_PATH, (LPARAM)importfile))
		return false;

	handle  =  CreateFile(importfile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if(handle == INVALID_HANDLE_VALUE)
    {
		DisplayErrorText(GetLastError(), _("Could not open file."));
		return false;
	}
	
	lowfilesize = GetFileSize(handle, &highfilesize);
	
	//if file size is > 4GB. then first 32 bits(LSB) is in lowfilesize and next 32 bits(MSB) in high filesize
	//converting these two 32 bits to 64 bits 
	filesize = ((wyInt64)highfilesize << 32) + lowfilesize;
		
	VERIFY(CloseHandle(handle));
	VERIFY(progress = GetDlgItem(m_hwnd, IDC_PROGRESS));

	SendMessage(progress,  PBM_SETRANGE32, 0, 0);
	UpdateWindow(progress);
	//setting the range of progressbar
	SendMessage(progress,  PBM_SETRANGE32, 0, ((filesize )/1024));

	//finding the increment value for the progress bar
	incr = ((filesize/1024)/100); 
	SendMessage(progress,  PBM_SETSTEP, (WPARAM)incr, 0); 
	
	return filesize;
}

void
ImportBatch::ShowProgressBarOrStatic(wyBool show)
{
	ShowWindow(GetDlgItem(m_hwnd, IDC_PROGRESS), show);
	ShowWindow(GetDlgItem(m_hwnd, IDC_MESSAGE), !show);
}

void
ImportBatch::SetImportValues(ImportFromSQL * import)
{
	wyString	curdb;
    wyWChar      szcurdb[128] = {0};
	wyString	importfile;
    wyString    errfile;
    wyWChar      file[1024] = {0};
	wyWChar		*lpFilePort = 0;
    wyWChar      filepath[MAX_PATH] = {0};

	if(SendMessage(GetDlgItem(m_hwnd,IDC_CHECKBOX), BM_GETSTATE, 0, 0) == BST_CHECKED)
		import->SetAbortFlag(wyTrue);
	else
		import->SetAbortFlag(wyFalse);
	
	import->SetTunnel((wyBool)m_tunnel->IsTunnel());

	if((*m_umysql)->db && strlen((*m_umysql)->db))
		curdb.SetAs((*m_umysql)->db);
	else
    {
		pGlobals->m_pcmainwin->GetCurrentSelDB( szcurdb, 127 );
        curdb.SetAs(szcurdb);
    }
	if( m_tunnel->IsTunnel())
    {
		import->SetTunnelHostSite(m_tunnel->GetHttpAddr());
		import->SetHostInfo(m_tunnel->GetHost(), m_tunnel->GetUser(), m_tunnel->GetPwd(), curdb.GetString(), m_tunnel->GetPort()); 
		import->SetContentType(m_tunnel->GetContentType()); //for setting the Content-type for import structure
		import->SetImportEncodingScheme(m_tunnel->GetEncodingScheme()); //Set the encoding scheme Base64 or not, for new connection for Import
	}
    else
    {
		import->SetHostInfo((*m_umysql)->host, (*m_umysql)->user, (*m_umysql)->passwd, curdb.GetString(), (*m_umysql)->port); 	
	}
	/* get the filename from the edit box */
	if(!SendMessage(m_hwndedit, WM_GETTEXT, SIZE_1024-1, (LPARAM)file))
		return;

    importfile.SetAs(file);
	import->SetImportFile(importfile);
	SearchFilePath(L"sqlyog", L".err", MAX_PATH, (wyWChar*)filepath, &lpFilePort);
	errfile.SetAs(filepath);
	import->SetErrorFile(errfile);
}

void
ImportBatch::SetParamValues(IMPORTPARAM * param)
{
	param->m_hwndSize		= GetDlgItem(m_hwnd, IDC_SIZESTATIC);
	param->m_hwndMsg		= GetDlgItem(m_hwnd, IDC_MESSAGE);
	param->m_hwndProgress = GetDlgItem(m_hwnd, IDC_PROGRESS);
}

void
ImportBatch::EnableDlgWindows(wyBool enable)
{
	EnableWindow(m_hwndedit, enable);
	EnableWindow(GetDlgItem(m_hwnd, IDC_EXPFILESELECT), enable);
	EnableWindow(GetDlgItem(m_hwnd, IDCANCEL), enable);

	return;
}

VOID
ImportBatch::ChangeOKButtonText(wyWChar * text)
{
	SetWindowText(GetDlgItem(m_hwnd, IDOK), text);
	return;
}

wyBool
ImportBatch::HandleError()
{
	wyWChar			errfile[MAX_PATH+1]={0};
	wyString        msg;
	CImportError	err;	
	wyWChar			*lpFilePort;
    wyString        file;
	
	if(SearchFilePath(L"sqlyog", L".err", MAX_PATH, errfile, &lpFilePort) == wyTrue) 
    {
        file.SetAs(errfile);
        msg.Sprintf(_("There was error(s) while executing the queries .\r\nThe query and the error message has been logged at:\r\n%s.\r\nPlease click on \"Open Error File...\" to open the error file."), file.GetString());
    }
    else
        msg.Sprintf(_("Unable to get SQLyog.err file information!"));

	if(GetForegroundWindow() != m_hwnd)
		FlashWindow(pGlobals->m_pcmainwin->m_hwndmain, TRUE);

	err.Create(m_hwnd, msg.GetString(), errfile);

	return wyFalse;
}

wyBool 

ImportBatch::IsQueryEmpty(const wyChar * query)
{
	wyInt32 i = 0;

	while(query[i] != 0)
	{
		if(!(isspace(query[i])))
			return wyFalse;
		i++;
	}

	return wyTrue;
} 

wyBool 
ImportBatch::SetCurDbFont()
{
	HDC		    dc;
	wyUInt32    fontheight;
	HWND	    hwndstatic;

	VERIFY(hwndstatic = GetDlgItem(m_hwnd, IDC_CURDATABASE));
	dc = GetDC(hwndstatic);
    fontheight = -MulDiv(8, GetDeviceCaps(dc, LOGPIXELSY), 72);
    VERIFY(ReleaseDC(hwndstatic, dc));
	VERIFY(m_hfont = CreateFont(fontheight, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, L"Arial"));
	SendMessage(hwndstatic, WM_SETFONT, (WPARAM)m_hfont, TRUE);
	DeleteFont(m_hfont);

	return wyTrue;
}

// show the user the current database.

wyBool 
ImportBatch::GetCurDatabase()
{
	wyInt32			ret;
    wyString        query;
	MYSQL_RES*		myres;
	MYSQL_ROW		myrow;
	wyString		myrowstr;
	MDIWindow		*wnd = GetActiveWin();
	wyBool			ismysql41 = wnd->m_ismysql41;

	query.Sprintf("select database()");

	wnd->SetThreadBusy(wyTrue);
	ret = my_query(wnd, m_tunnel, m_umysql, query.GetString(), query.GetLength());
	wnd->SetThreadBusy(wyFalse);
	if(ret)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_umysql, query.GetString());
		return wyFalse;
	}

	myres	=	m_tunnel->mysql_store_result(*m_umysql, false, false, GetActiveWin());
	if(!myres)
	{
		ShowMySQLError(m_hwnd, m_tunnel, m_umysql, query.GetString());
		return wyFalse;
	}

	myrow	=	m_tunnel->mysql_fetch_row(myres);
	if(myrow[0])
    {
		myrowstr.SetAs(myrow[0], ismysql41);
		strncpy(m_curdb, myrowstr.GetString(), myrowstr.GetLength());
    }
    else 
		m_curdb[0] = NULL;
	if( strlen(m_curdb) == 0)
		strcpy(m_curdb, "<None>");

        myrowstr.SetAs(m_curdb);
	SendMessage(GetDlgItem(m_hwnd, IDC_CURDATABASE), WM_SETTEXT, 0, (LPARAM)myrowstr.GetAsWideChar()); 		
	
	m_tunnel->mysql_free_result(myres);	

	return wyTrue;
}

/* ================================================================== */

// Implementation of IDD_SHOWIMPORTERROR dialog. This is a simple dialog which
// is shown by SQLyog when it encounters an error while import from a SQL Dump.
// It will show a generic error and let the user open sqlyog.err (where sqlyog actually
// writes info of the error) by clicking on a command button)

// Constructor.
CImportError::CImportError()
{
}

// Destructor.
CImportError::~CImportError()
{
}

// this function creates the actual dialog box and also initializes some values of the class

wyBool 
CImportError::Create(HWND hwndParent, const wyChar * message, const wyWChar *errfile)
{
	m_message	= message;
	m_errfile	= errfile;

	DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_SHOWIMPORTERR), hwndParent,
					 CImportError::WndProc, (LPARAM)this);

	return wyTrue;
}

INT_PTR CALLBACK
CImportError::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CImportError	*warn = (CImportError*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		
	wyString		warnstr;
	
	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		return wyTrue;

	case WM_INITDLGVALUES:
		{
			warnstr.SetAs(warn->m_message);
			SetWindowText(GetDlgItem(hwnd, IDC_MESSAGE), warnstr.GetAsWideChar());	
			HICON icon = LoadIcon(NULL, IDI_INFORMATION);
            VERIFY(icon);
			SendMessage(GetDlgItem(hwnd, IDC_STATICIMG), STM_SETIMAGE, IMAGE_ICON, (LPARAM)(UINT)icon);
		}
		break;

	case WM_HELP:
		ShowHelp("http://sqlyogkb.webyog.com/article/117-excute-sql-script");
		return wyTrue;

	case WM_COMMAND:
		warn->OnWMCommand(hwnd, wParam);
		break;
	}
	return wyFalse;
}


void 
CImportError::OnWMCommand(HWND hwnd, WPARAM wParam)
{
	switch LOWORD(wParam)
	{
	case IDC_OPENFILE:
		ShellExecute(NULL, L"open", L"notepad", m_errfile, NULL, SW_SHOWNORMAL); 
		break;

	case IDCANCEL:
	case IDOK:
		yog_enddialog(hwnd, 1);
		break;
	}
}
