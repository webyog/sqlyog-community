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

#include "WyMySQLError.h"
#include "MDIWindow.h"
#include "winuser.h"
#include "Include.h"
#include "GUIHelper.h"

#define SIZE_2K  (2*1024)

MySQLError::MySQLError(wyString* pkeywords, wyString* pfunctions)
{
	m_tunnel	 = NULL;
	m_mysql		 = NULL;
	m_query      = NULL;
	m_mysqlerrno = 0;
	m_hwnddlg	 = NULL;
	m_iswarning  = wyFalse;
    m_pkeywords = pkeywords;
    m_pfunctions = pfunctions;
}

// Handles error messaage 
void
MySQLError::Show(HWND hdlg, Tunnel * tunnel, PMYSQL mysql, const wyChar * query , wyBool val, MYSQL_RES *reswarning)
{
	wyString	 err;	
   
	if(query)
		m_query = query;

	if(!reswarning)
	{
		//Mysql Error
		if(HandleMySQLError(tunnel, mysql, &err) == 0)		
		{
			return;
		}
	}

	else
	{
		//Mysql Warnings
		HandleMySQLWarnings(tunnel, mysql, reswarning);		
	}

	if(val == wyTrue && !reswarning)
	{
		//Message box shows MySQL error
		MessageBox(hdlg, err.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR);
	}

	else
	{
		//Common for warning & error that shows in a dialog
		CreateErrorDialog(hdlg); // creates the common error dialog box
	}

	return;

}

void 
MySQLError::Show(HWND hdlg, const wyChar* query, wyInt32 mysqlerrorno, const wyChar* mysqlerror)
{
    m_query = query;
    m_mysqlerrno = mysqlerrorno;
    m_mysqlerrmsg.SetAs(mysqlerror);
    CreateErrorDialog(hdlg);
}

wyInt32 
MySQLError::HandleMySQLError(Tunnel * tunnel, PMYSQL mysql, wyString *err)
{
	MDIWindow *wnd = NULL;
	//wyString  err;
	wyInt32   ret = 0;

	m_iswarning = wyFalse;
	m_mysqlerrno = tunnel->mysql_errno(*mysql);

	/* it may happen that due to http error the errornumber is 0 so we return */
	if(0 == m_mysqlerrno)
		return 0;

	if(tunnel->IsTunnel()&& m_mysqlerrno > 12000)
		err->Sprintf(_("HTTP Error No. %d\n%s"), m_mysqlerrno, tunnel->mysql_error(*mysql));

	else
	{
		m_mysqlerrmsg.SetAs(tunnel->mysql_error(*mysql));
		
		if(err)
			err->Sprintf(_("Error No. %d\n%s"), m_mysqlerrno, m_mysqlerrmsg.GetString());

		if(m_mysqlerrno == MYSQL_SERVER_GONE)
		{
			wnd = GetActiveWin();

			if(wnd)
			{
				/* starting from 4.02 we issue query like show variables like and set the client value to the database */
				/* only required in direct connection as in HTTP we execute it always in the server side */
		
				SetCharacterSet(tunnel, *mysql, (wyChar*)wnd->m_conninfo.m_codepage.m_cpname.GetString());

				/* if its tunnel then we need the server version */
				if(tunnel->IsTunnel())
					ret = tunnel->GetMySQLVersion(*mysql);

	    		if(ret && IsMySQL5010(tunnel, mysql))
					wnd->SetDefaultSqlMode(tunnel, mysql ,wyTrue);
			}
		}
	}
    return 1;
}
/*
Display warning.
warning code - warning
*/
void 
MySQLError::HandleMySQLWarnings(Tunnel * tunnel, PMYSQL mysql, MYSQL_RES *reswarning)
{
	MYSQL_ROW	 rowwarning;
	wyInt32		 warnindex;
	wyInt32		 warncodefld;	

	m_iswarning = wyTrue;
		
	//Gets the field index for 'Code' and 'Message'
	warnindex = GetFieldIndex(tunnel, reswarning, "Message");
	warncodefld = GetFieldIndex(tunnel, reswarning, "Code");

	if(warnindex == -1 || warncodefld == -1)
		return;

	while(rowwarning = tunnel->mysql_fetch_row(reswarning))
	{			
		if(rowwarning[warnindex] && rowwarning[warncodefld])
		{
			m_mysqlerrmsg.AddSprintf("%s - %s\r\n", 
									rowwarning[warncodefld], 
									rowwarning[warnindex]);			
		}
	}
}

/// Create Commom error dialog box
void
MySQLError::CreateErrorDialog(HWND hwndparent)
{
	m_hwnd = GetFocus();
	DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_MYSQL_ERROR), hwndparent,
		WndProc, (LPARAM)this);
}

///Handle window procedure for error dialog
INT_PTR CALLBACK
MySQLError::WndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	MySQLError * errorobj = (MySQLError *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	wyInt32		wmid;
	
	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hDlg, GWLP_USERDATA,(LONG_PTR)lParam);
        LocalizeWindow(hDlg);
		PostMessage(hDlg, WM_INITDLGVALUES, 0, 0);
		return wyTrue;

	case WM_INITDLGVALUES:
		errorobj->CreateScintillaWindow(hDlg);
		break;

	case WM_COMMAND:
		{
			wmid = LOWORD(wParam);

			if((wmid == ID_OK) || wmid == IDCANCEL)
			{
				EndDialog(hDlg, wmid);
				SetFocus(errorobj->m_hwnd);
			}
				
				
			else if(wmid == IDC_SAVE_BUTTON)
				errorobj->WriteToTextfile(hDlg); // Writes the query, error number and error message into a text file
				break;
		}

	case WM_DESTROY:
		return 0;

	}
    return 0;
}

/// Create scintilla window on error dialog box for query, error number and error message
void
MySQLError::CreateScintillaWindow(HWND hDlg)
{
	wyString	errornumber, query;
	HWND		hquery, herrorno, herrorstring;
	MDIWindow	*wnd;
	wyInt32		command;
	HICON		hicon;
	
	wnd = GetActiveWin();

	m_hwnddlg = hDlg;
 	
	VERIFY(hquery = GetDlgItem(hDlg, IDC_QUERY));
	VERIFY(herrorno = GetDlgItem(hDlg, IDC_ERROR_NUMBER));
	VERIFY(herrorstring = GetDlgItem(hDlg, IDC_ERROR_MESSAGE));

    SetWindowLongPtr(hquery, GWLP_USERDATA,(LONG_PTR)this);
	m_wporigquerytextproc =(WNDPROC)SetWindowLongPtr(hquery, GWLP_WNDPROC,(LONG_PTR)QueryTextProc);

	SetWindowLongPtr(herrorstring, GWLP_USERDATA,(LONG_PTR)this);
	m_wporigerrtextproc =(WNDPROC)SetWindowLongPtr(herrorstring, GWLP_WNDPROC,(LONG_PTR)ErrTextProc);
	
	if(wnd)
    {
		//Set scintilla properties
		SetScintillaModes(hquery, wnd->m_keywordstring, wnd->m_functionstring, wyTrue);
    }
    else if(m_pkeywords && m_pfunctions)
    {
        SetScintillaModes(hquery, *m_pkeywords, *m_pfunctions, wyTrue);
    }

	//Set error message modes
	SetErrMsgEditorModes(herrorstring);

    //Line added because on changing color there was margin coming for editor
    SendMessage(hquery, SCI_SETMARGINWIDTHN,1,0);
	
    //Line added because on changing color there was margin coming for editor
    SendMessage(herrorstring, SCI_SETMARGINWIDTHN,1,0);

	if(m_query)
	{
		query.SetAs(m_query);

		//if query size is more than 2KB, strip the query 
		if(query.GetLength() > SIZE_2K)
		{
			query.Strip(query.GetLength() - SIZE_2K);
			query.Add("...");
		}

		SendMessage(hquery, SCI_SETTEXT, query.GetLength(), (LPARAM)query.GetString());
	}

	errornumber.Sprintf("%d", m_mysqlerrno);
	SendMessage(herrorno, WM_SETTEXT, 0, (LPARAM)errornumber.GetAsWideChar());
	SendMessage(herrorstring, SCI_SETTEXT, m_mysqlerrmsg.GetLength(), (LPARAM)m_mysqlerrmsg.GetString());
	
	SendMessage(hquery, SCI_SETREADONLY, true, 0);
	SendMessage(herrorstring, SCI_SETREADONLY, true, 0);
	
	//Handle the dialog control as per Error or Warning
	command = (m_iswarning == wyTrue) ? SW_HIDE : SW_SHOW;
    
	ShowWindow(GetDlgItem(hDlg, IDC_STATIC_ERRNO), command);
	ShowWindow(GetDlgItem(hDlg, IDC_ERROR_NUMBER), command);

	if(m_iswarning == wyTrue)
	{
		//For warnings
		hicon = LoadIcon(NULL, IDI_WARNING);
		SendDlgItemMessage(hDlg, IDC_MYSQL_ERROR, STM_SETICON, (WPARAM)hicon, 0);
		DestroyIcon(hicon);

		SetWindowText(GetDlgItem(hDlg, IDC_STATIC_ERRMSG), _(L"Warnings"));	
		SetWindowText(GetDlgItem(hDlg, IDC_STATIC_MYSQLERROR), _(L"MySQL Warning"));	
		SetWindowText(hDlg, _(L"MySQL Warning"));
	}

	else
	{
		//For errors
		hicon = LoadIcon(NULL, IDI_ERROR);
		SendDlgItemMessage(hDlg, IDC_MYSQL_ERROR, STM_SETICON, (WPARAM)hicon, 0);
		DestroyIcon(hicon);

		SetWindowText(GetDlgItem(hDlg, IDC_STATIC_ERRMSG), _(L"Error message"));
		SetWindowText(GetDlgItem(hDlg, IDC_STATIC_MYSQLERROR), _(L"MySQL Error"));
		SetWindowText(hDlg, _(L"MySQL Error"));
	}
}

/// Writes Error Number, Error message and Query to a text file
wyBool
MySQLError::WriteToTextfile(HWND hwnd)
{
	HANDLE		hfile;  // Handle to the file
	wyString	filebuff; // writes the query, error message and error number into the file buffer 
	DWORD		dwbyteswritten;
	wyWChar		filename[MAX_PATH+1]={0};
	wyString	namefile;
	BOOL		retval;
	wyString    query, errornumber;
			
	if(m_query)
	{
		query.SetAs(m_query);
		filebuff.AddSprintf(_("Executed SQL Statement : %s \r\n"), query.GetString());
	}

	errornumber.Sprintf("%d",m_mysqlerrno);
	filebuff.AddSprintf(_("Error Number : %s \r\n"),errornumber.GetString());
	filebuff.AddSprintf(_("Error Message: %s \r\n"),m_mysqlerrmsg.GetString());   	
		
	if(InitFile(hwnd, filename, TEXTINDEX, MAX_PATH))
	{
		namefile.SetAs(filename);
       	hfile = CreateFile((LPCWSTR)namefile.GetAsWideChar(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
			FILE_ATTRIBUTE_NORMAL, NULL);

		if(hfile == INVALID_HANDLE_VALUE)
		{
			DisplayErrorText(GetLastError(), _("Could not create file."));
			return wyFalse;
		}
		
		// writes the buffer contents to the file 
		retval = WriteFile(hfile, filebuff.GetString(), filebuff.GetLength(), &dwbyteswritten, NULL);
		VERIFY(CloseHandle(hfile));		
		
		return wyTrue;
	}

	return wyFalse;
}

//Sets the error message editor modes
void
MySQLError::SetErrMsgEditorModes(HWND hwnderror)
{
    
	//set the margin 
	SendMessage(hwnderror, SCI_SETMARGINWIDTHN, 0, 0);
	
	//set the lexer language 
	//SendMessage(hwnderror, SCI_SETLEXERLANGUAGE, 0, (long)"MySQL");
    
	//now scintilla can accept utf8 characters
	SendMessage(hwnderror, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
		
	// change wordwrap
	//SendMessage(hwnderror, SCI_SETWRAPMODE, SC_WRAP_WORD, SC_WRAP_WORD);
    SendMessage(hwnderror, SCI_SETWRAPMODE, SC_WRAP_NONE, SC_WRAP_NONE);
    //Change Foreground and background color
    SetMTIColor(hwnderror);
}

// Handles error messaage 
void
MySQLError::ShowDirectErrMsg(HWND hdlg, const wyChar * query , const wyChar * errmsg )
{
	    
	if(query)
		m_query = query;

	m_mysqlerrno = 0;

	m_mysqlerrmsg.SetAs(errmsg);

	CreateErrorDialog(hdlg); // creates the common error dialog box

	return;

}

LRESULT CALLBACK
MySQLError::QueryTextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MySQLError	*err = (MySQLError*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	wyInt32		key;

    switch(message)
	{
	//if focus is on editor and user is pressing the esc key
	case WM_KEYDOWN:
		 key = LOWORD(wParam);
		 if(key == VK_ESCAPE)
         {
			EndDialog(err->m_hwnddlg, 0);
			SetFocus(err->m_hwnd);
			return 0;
		  }
	}
	return CallWindowProc(err->m_wporigquerytextproc, hwnd, message, wParam, lParam);
}

LRESULT CALLBACK
MySQLError::ErrTextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MySQLError	*err = (MySQLError*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	wyInt32		key;

    switch(message)
	{
	//if focus is on editor and user is pressing the esc key
	case WM_KEYDOWN:
		 key = LOWORD(wParam);
		 if(key == VK_ESCAPE)
         {
			EndDialog(err->m_hwnddlg, 0);
			SetFocus(err->m_hwnd);
			return 0;				
         }
	}
	return CallWindowProc(err->m_wporigerrtextproc, hwnd, message, wParam, lParam);
}
