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

#include "ConnectionCommunity.h"
#include "CommonHelper.h"
#include "ExportMultiFormat.h"
#include "FrameWindowHelper.h"
#include "Global.h"
#include "Verify.h"
#include "GUIHelper.h"
#include "MySQLVersionHelper.h"
#include "CCustomComboBox.h"

#define GPL_MSG _(L"This software is released under the GNU General Public Licence(GPL), \
which is probably the best known Open Source licence. The formal terms of GPL licence \
can be found at http://www.fsf.org/licence/.")

#define STATUS_LINK _("Upgrade to SQLyog Professional/Enterprise/Ultimate")

ConnectionCommunity::ConnectionCommunity()
{
    m_isent			= wyFalse;
	m_isapclose		= wyFalse;
	m_powertoolsID  = 0;
	pGlobals->m_appname.SetAs("SQLyog Community");
    m_statuslink.SetAs(STATUS_LINK);

	m_rgbconnection		= RGB(255,255,255);
	m_rgbconnectionfg	= RGB(0,0,0);
	m_rgbobbkcolor		= RGB(255,255,255);
	m_rgbobfgcolor		= RGB(0,0,0);
	m_setcolor			= wyFalse;
	m_changeobcolor     = wyFalse;
}

void 
ConnectionCommunity::HandlerToOnInitDialog(HWND hwnd, LPARAM lparam)
{
    OnInitDialog(hwnd, lparam);
    return;
}

void
ConnectionCommunity::OnTestConnection(HWND hwnd)
{
	UpdateWindow(hwnd);

	if(GetActiveWin())
		UpdateWindow(GetActiveWin()->m_pctabmodule->GetHwnd());		

	TestConnection(hwnd, wyTrue);
}

wyBool 
ConnectionCommunity::AboutRegInfo(HWND hwnd)
{
    SetWindowText(GetDlgItem(hwnd, IDC_REGINFO), L"");
    SetWindowText(GetDlgItem(hwnd, IDC_WARNING), GPL_MSG);
    return wyTrue;
}

void 
ConnectionCommunity::OnAboutWmCommand(HWND hwnd, WPARAM wparam)
{
    switch(LOWORD(wparam))
    {
    case IDCANCEL:
    case IDOK:
	    VERIFY(yog_enddialog(hwnd, 1));
	    break;
    }

   	//If click on url
	if((HIWORD(wparam) == STN_CLICKED)&&(LOWORD(wparam) == IDC_LINK))
		ShellExecute(NULL, L"open", TEXT(HOMEURL_ABOUTUS), NULL, NULL, SW_SHOWNORMAL);
    
    return;
}

void 
ConnectionCommunity::OnStatusBarWmCommand(HWND hwnd, WPARAM wparam)
{
    if((HIWORD(wparam) == STN_CLICKED)&&(LOWORD(wparam) == IDC_LINK))
		ShellExecute(NULL, L"open", TEXT(BUYURL_STATUSBAR), NULL, NULL, SW_SHOWNORMAL);
    
    return;
}

wyBool 
ConnectionCommunity::CreateTabs(HWND hwnd, wyUInt32 id, wyBool ispowertools)
{
	HWND        hwndtab;
	wyWChar*     titles[]  = {L"MySQL", L"HTTP", L"SSH", L"SSL", _(L"Advanced")};
	wyInt32     numtabs, count;
	TCITEM		tci = {0};

	//if connection dialog is of powertools, then no advanced tab
	if(ispowertools == wyFalse)
		numtabs = sizeof(titles)/ sizeof(titles[0]);
	else
		numtabs = (sizeof(titles)/ sizeof(titles[0])) - 1;

	/* get the tab window */
	VERIFY(hwndtab = GetDlgItem(hwnd, id));

	for(count = 0; count < numtabs; count++)
	{
		tci.mask	= TCIF_TEXT ;//| TCIF_IMAGE;
		tci.pszText = titles[count];

		VERIFY(TabCtrl_InsertItem(hwndtab, 2000, &tci)!= -1);
	}

	return wyTrue;
}

wyBool 
ConnectionCommunity::HandlerToGetInitialDetails(HWND hdlg)
{
    return GetInitialDetails(hdlg);
	
	//To retain focus on saved connection name
	//SetFocus(GetDlgItem(hdlg, IDC_DESC));
}

// Function deletes the conn detail section from the ini file.
// Basically it sets the name to zero.
wyInt32 
ConnectionCommunity::DeleteConnDetail(HWND hdlg)
{
	wyInt32     count, cursel, totcount, confirm;
	HWND		hwndcombo;
    wyWChar      directory[MAX_PATH + 1]={0}, *lpfileport = 0, connname[SIZE_512] = {0};
    wyString    conn, autodirectory, dirstr;
	wyUInt32    ret;
	
	confirm = yog_message(hdlg, _(L"Do you really want to delete the connection detail?"), pGlobals->m_appname.GetAsWideChar(), 
                            MB_YESNO | MB_HELP | MB_ICONQUESTION | MB_DEFBUTTON2);

	if(confirm != IDYES)
		return FALSE;

	// Get the complete path.
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	if(ret == 0)
		return 0;

	VERIFY(hwndcombo = GetDlgItem(hdlg, IDC_DESC));

	totcount  = SendMessage(hwndcombo, CB_GETCOUNT, 0, 0);

	if(totcount == 0)
		return 0;

	VERIFY ((cursel = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0))!= CB_ERR);
	count = SendMessage(hwndcombo, CB_GETITEMDATA, cursel, 0);
	conn.Sprintf("Connection %u", count);
	
	dirstr.SetAs(directory);
	//ret = WritePrivateProfileStringA(conn.GetString(), NULL, NULL, dirstr.GetString());
	ret = wyIni::IniDeleteSection(conn.GetString(), dirstr.GetString());
	
	SendMessage(hwndcombo, WM_GETTEXT, SIZE_512 - 1,(LPARAM)connname);

	SendMessage(hwndcombo, CB_DELETESTRING, cursel, 0);

	if(totcount == 1)
    {
		EnableWindow(GetDlgItem(hdlg, IDC_SAVE), FALSE);
		ChangeConnStates(hdlg, wyFalse);
		TabCtrl_SetCurSel(GetDlgItem(hdlg, IDC_CONNTAB), 0);
		OnConnDialogTabSelChange(hdlg, GetDlgItem(hdlg, IDC_CONNTAB));
		m_conndetaildirty = wyFalse;

		SetFocus(hdlg);

		return 1;
	} 
    else if(cursel ==(totcount - 1)) 
		SendMessage(hwndcombo, CB_SETCURSEL, cursel-1, 0);
	else if(cursel == 0)
		SendMessage(hwndcombo, CB_SETCURSEL, 0, 0);
	else
		SendMessage(hwndcombo, CB_SETCURSEL, cursel-1, 0);

	GetInitialDetails(hdlg);

	//populate color array for connection combo
	PopulateColorArray(hwndcombo, &dirstr);

    TabCtrl_SetCurSel(GetDlgItem(hdlg, IDC_CONNTAB), 0);
	m_conndetaildirty = wyFalse;
    //for showing the server tab and hiding all other tabs
    ShowServerOptions(hdlg);

	return 1;
}

void 
ConnectionCommunity::WriteDefValues(HWND hdlg)
{
	WriteMysqlDefValues(hdlg);
	return;
}

// Function saves the current profile in the .ini file.
wyBool 
ConnectionCommunity::SaveConnection(HWND hdlg)
{	
	HWND		hwndcombo = NULL; //con. name combo
	wyInt32     count = 0; //connection index
	wyUInt32    ret = 0;
	wyString    conn, dirstr, connnamestr;
	wyWChar     directory[MAX_PATH + 1]={0}, *lpfileport=0; //for keeping the .ini path
	RECT rect	= { 0 };

		
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	if(ret == 0)
		return wyFalse;

	dirstr.SetAs(directory);
		
	//Con. name combo
	VERIFY(hwndcombo = GetDlgItem(hdlg, IDC_DESC));

	VERIFY ((count = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0))!= CB_ERR);
	count = SendMessage(hwndcombo, CB_GETITEMDATA, count, 0);
	conn.Sprintf("Connection %u", count);

	wyIni::IniGetString(conn.GetString(), "Name", "", &connnamestr, dirstr.GetString());

	if(connnamestr.GetLength() == 0)
	{
		GetConnectionName(hdlg, &connnamestr, conn.GetString(), dirstr.GetString());
	}

	//Save "Mysql' tab controls details
	SaveServerDetails(hdlg, conn.GetString(), dirstr.GetString());

	//save advanced tab details
	m_rgbobbkcolor = m_rgbconnection;
	m_rgbobfgcolor = m_rgbconnectionfg;
	SaveAdvancedTabDetails(hdlg, conn.GetString(), dirstr.GetString());

	//populate color array for connection combo
	PopulateColorArray(hwndcombo, &dirstr);
	EnableWindow(GetDlgItem(hdlg, IDC_SAVE), FALSE);
    
    InvalidateRect(hdlg, NULL, true);
	UpdateWindow(hdlg);

	//Change color of combobox
	GetClientRect(hwndcombo, &rect);
	
    InvalidateRect(hwndcombo, &rect, TRUE);

	UpdateWindow((HWND)SendMessage(hwndcombo, CCBM_GETCOMBOHWND, NULL, NULL));
    
	m_isnewconnection = wyFalse;
	m_newconnid = -1;

	m_currentconn.SetAs(conn.GetAsWideChar());
	m_consectionname.SetAs(conn.GetAsWideChar());
	
	EnableWindow(GetDlgItem(hdlg, IDC_CLONECONN), TRUE);
	EnableWindow(GetDlgItem(hdlg, IDC_DELETE), TRUE);	
	EnableWindow(GetDlgItem(hdlg, IDC_EDITCONN), TRUE);	

	return wyTrue;
}

/* Function just tests the connection and checks whether it is correct or not */
wyBool 
ConnectionCommunity::TestConnection(HWND hwnd, wyBool showmsg /*=wyTrue*/)
{
	ConnectionInfo	dbname; 
	MYSQL			*mysql;
	Tunnel			*tunnel;
    wyString	    query("select version()"), version;
	wyInt32         ret;
	MYSQL_RES	    *res;
	MYSQL_ROW	    row;
	wyString		myrowstr;

	InitConnectionDetails(&dbname);

    tunnel = CreateTunnel(wyFalse);
	dbname.m_tunnel = tunnel;

	// Call ConnectToMySQL function to initialize a new mySQL structure and connect to
	// mySQL server with the connection details. The function will return a valid mysql
	// pointer if connection successful and if not then it will return NULL.
	// If null then show the standard mySQL errors given by the mySQL API.
	//if database name is not correct, then throw error
	mysql = pGlobals->m_pcmainwin->m_connection->ConnectToMySQL(hwnd, &dbname);
	if(mysql == NULL)
    {
        if(tunnel)
            delete tunnel;
		return wyFalse;
	} 
	else if((dbname.m_db.GetLength() != 0) && (IsDatabaseValid(dbname.m_db, mysql, tunnel) == wyFalse))
	{
		query.Clear();
		goto cleanup;
	}

	/* now we send a test query "select version()", if everything is ok then we show a message
	   with the version number */
	ret = HandleMySQLRealQuery(tunnel, mysql, query.GetString(), query.GetLength(), false);

	if(ret)
		goto cleanup;

	res = tunnel->mysql_store_result(mysql);
	if(!res && mysql->affected_rows == -1)
		goto cleanup;

	row = tunnel->mysql_fetch_row(res);
	
	myrowstr.SetAs(row[0], IsMySQL41(dbname.m_tunnel, &mysql));
	version.Sprintf(_("Connection successful!\nMySQL version: %s"), myrowstr.GetString());

	if(showmsg)
		yog_message(hwnd, version.GetAsWideChar(), _(L"Connection Info"), MB_OK | MB_ICONINFORMATION);

	tunnel->mysql_free_result(res);
	tunnel->mysql_close(mysql);

	delete	tunnel;

	return wyTrue;

cleanup:
	if(query.GetLength())
		ShowMySQLError(hwnd, tunnel, &mysql, query.GetString());
	else
		ShowMySQLError(hwnd, tunnel, &mysql, NULL, wyTrue);
    delete tunnel;
	return wyFalse;
}


MYSQL *
ConnectionCommunity::ConnectToMySQL(ConnectionInfo *conn, Tunnel *tunnel, MYSQL *mysql, wyString &errormsg)
{
    wyUInt32		client=0;
    	
	SetMySQLOptions(conn, tunnel, &mysql);

	/// Now connect to the mysql server if there is any error then it is shown in the calling function..
		mysql	= tunnel->mysql_real_connect(mysql, conn->m_host.GetString(), conn->m_user.GetString(), 
				conn->m_pwd.GetString(), NULL, conn->m_port, NULL, client | CLIENT_MULTI_RESULTS | CLIENT_REMEMBER_OPTIONS, 
                                                conn->m_host.GetString());
	return mysql;
}

// Connect to a mysql server with the given details and return a valid MYSQl* connection handle
// and if there is an error it return NULL
MYSQL *
ConnectionCommunity::ConnectToMySQL(HWND hdlg, ConnectionInfo *coninfo)
{
	Tunnel			*tunnel = coninfo->m_tunnel;
	wyUInt32		portno, client=0;
	wyInt32         ret;
    wyWChar         host[SIZE_512]={0}, user[SIZE_512]={0}, timeout[32] = {0};
	wyWChar			pwd[SIZE_512]={0}, port[10]={0};
	MYSQL           *mysql, *temp;
    wyString        strinitcommand;

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	InitConnectionDetails(coninfo);

	// FIRST initialize the object.
	VERIFY(mysql = tunnel->mysql_init((MYSQL*)0));
	temp = mysql;

	// Get all the information.
	ret = SendMessage(GetDlgItem(hdlg, IDC_DLGCONNECT_HOST), WM_GETTEXT,(WPARAM)SIZE_512-1, (LPARAM)host);
	ret = SendMessage(GetDlgItem(hdlg, IDC_DLGCONNECT_USER), WM_GETTEXT,(WPARAM)SIZE_512-1, (LPARAM)user);
	ret = SendMessage(GetDlgItem(hdlg, IDC_DLGCONNECT_PORT), WM_GETTEXT,(WPARAM)9, (LPARAM)port);
	ret = SendMessage(GetDlgItem(hdlg, IDC_DLGCONNECT_PASSWORD), WM_GETTEXT,(WPARAM)SIZE_512-1, (LPARAM)pwd);

	portno = _wtoi(port);

	coninfo->m_host.SetAs(host);
	//to trim empty spaces from right, to avoid mysql errors
	coninfo->m_host.RTrim();

	coninfo->m_user.SetAs(user);
	//to trim empty spaces from right, to avoid mysql errors
	coninfo->m_user.RTrim();
	
	coninfo->m_pwd.SetAs(pwd); 
	coninfo->m_port = portno;
	coninfo->m_ishttp = wyFalse;
	coninfo->m_issslchecked = wyFalse;

	coninfo->m_hprocess      = INVALID_HANDLE_VALUE;
	coninfo->m_isssh         = wyFalse;
	coninfo->m_origmysqlport = 0;

	//Compress protocol settings
	ret =(SendMessage(GetDlgItem(hdlg, IDC_COMPRESS), BM_GETCHECK, 0, 0));
	(ret == BST_CHECKED) ? coninfo->m_iscompress = wyTrue : coninfo->m_iscompress = wyFalse;

	//send password in cleartext settings
	/*ret =(SendMessage(GetDlgItem(hdlg, IDC_ISCLEARTEXT), BM_GETCHECK, 0, 0));
	(ret == BST_CHECKED) ? coninfo->m_ispwdcleartext = wyTrue : coninfo->m_ispwdcleartext = wyFalse;*/	

	//Session wait_timeout settings
	ret =(SendMessage(GetDlgItem(hdlg, IDC_TIMEOUTDEF), BM_GETCHECK, 0, 0));
	if(ret == BST_CHECKED) 
	{
		coninfo->m_isdeftimeout = wyTrue;
		coninfo->m_strwaittimeout.SetAs("28800");
	}
	else
	{
		coninfo->m_isdeftimeout = wyFalse;	
		ret = SendMessage(GetDlgItem(hdlg, IDC_TIMEOUTEDIT), WM_GETTEXT,(WPARAM)32-1, (LPARAM)timeout);
		if(wcslen(timeout))
			coninfo->m_strwaittimeout.SetAs(timeout);
        
		else
			coninfo->m_strwaittimeout.SetAs("28800");
	}

	//Get SQL_Mode settings
	GetSqlModeSettings(hdlg, coninfo);

    //..Getting & Setting init_command if the connection is not http
    if(!tunnel->IsTunnel())
    {
		GetInitCommands(hdlg, coninfo);
		strinitcommand.SetAs("set names 'utf8';");
		strinitcommand.Add(coninfo->m_initcommand.GetString());
		ExecuteInitCommands(mysql, tunnel, coninfo->m_initcommand);
    }

    GetKeepAliveInterval(hdlg, coninfo);

	SetMySQLOptions(coninfo, tunnel, &mysql);

	mysql	= tunnel->mysql_real_connect(mysql, coninfo->m_host.GetString(), coninfo->m_user.GetString(), coninfo->m_pwd.GetString(), NULL, coninfo->m_port, NULL, client | CLIENT_MULTI_RESULTS | CLIENT_REMEMBER_OPTIONS, coninfo->m_url.GetString());

	if(!mysql)
    {
		ShowMySQLError(hdlg, tunnel, &temp, NULL, wyTrue);
		tunnel->mysql_close(temp);
		SetCursor(LoadCursor(NULL, IDC_WAIT));
		return NULL;
	}
		
	GetOtherValues(hdlg, coninfo);
    SetCursor(LoadCursor(NULL, IDC_WAIT));

	return mysql;	
}

void
ConnectionCommunity::OnConnect(HWND hwnd, ConnectionInfo * dbname)
{
	Tunnel		*tunnel;
    MYSQL		*mysql;
	HWND		lastfocus = GetFocus();
	wyString	conn, temp, dirnamestr;
	wyWChar		directory[MAX_PATH + 1] = {0}, *lpfileport = 0, pass[MAX_PATH + 1] = {0};
	HWND		hwndcombo;
	wyInt32		count, storepwd, ret;

//	DEBUG_ENTER("clicked ok");

    tunnel = CreateTunnel(wyFalse);

//	DEBUG_LOG("created tunnel");

	// Call ConnectToMySQL function to initialize a new mySQL structure and connect to
	// mySQL server with the connection details. The function will return a valid mysql
	// pointer if connection successful and if not then it will return NULL.
	// If null then show the standard mySQL errors given by the mySQL API.
	/* set correct values for tunnel to use HTTP authentication */
	dbname->m_tunnel = tunnel;

    mysql = ConnectToMySQL(hwnd, dbname);
	if(mysql == NULL)
    {
        delete tunnel;
		SetFocus(lastfocus);
	} 
    else if(dbname->m_db.GetLength() ==0 ||(IsDatabaseValid(dbname->m_db, mysql, tunnel) == wyTrue))
    {
		// Successful so write the current details in connection .ini file 
		// so that when the user uses the software again it will show the same details.
		dbname->m_mysql = mysql;
		dbname->m_tunnel = tunnel;

		WriteConnDetails(hwnd);

		ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
		if(ret == 0)
        {
		    yog_enddialog(hwnd, (wyInt32)1);
			return;
		}
		
		/* if the user has changed any information then we ask him whether he wants to save */
		if(m_conndetaildirty == wyTrue)
		{
			if(ConfirmAndSaveConnection(hwnd, wyTrue) == wyFalse)
				return;
		}
		else 
        {
			/*	even if he has not selected to save, we need to check for password thing
				as the user might diable on the store_password first and then make
				modification to the password, in that case the state will not be dirty
				and the value will not be stored */
			VERIFY(hwndcombo = GetDlgItem(hwnd, IDC_DESC));

			count  = SendMessage(hwndcombo, CB_GETCOUNT, 0, 0);

			if(!count)
            {
				yog_enddialog(hwnd,(wyInt32)1);
				return;
			}

			VERIFY ((count = SendMessage(hwndcombo, CB_GETCURSEL, 0, 0))!= CB_ERR);
			count = SendMessage(hwndcombo, CB_GETITEMDATA, count, 0);
			conn.Sprintf("Connection %u", count);

			storepwd = Button_GetCheck(GetDlgItem(hwnd, IDC_DLGCONNECT_STOREPASSWORD));

			/* now we only save the password if the user has asked for otherwise we remove it only */
			/* feature implemented in v5.0 */
			dirnamestr.SetAs(directory);
            //WritePrivateProfileStringA(conn.GetString(), "Password01", NULL, dirnamestr.GetString());
			wyIni::IniDeleteKey(conn.GetString(), "Password01", dirnamestr.GetString());
			if(!storepwd)
            {
				//WritePrivateProfileStringA(conn.GetString(), "Password", NULL, dirnamestr.GetString());
				wyIni::IniDeleteKey(conn.GetString(), "Password", dirnamestr.GetString());
            }
            else
            {
                GetWindowText(GetDlgItem(hwnd, IDC_DLGCONNECT_PASSWORD), pass, MAX_PATH);
                temp.SetAs(pass);
                EncodePassword(temp);
                wyIni::IniWriteString(conn.GetString(), "Password", temp.GetString(), dirnamestr.GetString());
            }
			
			/* write the store password value too */
			temp.Sprintf("%d", storepwd);
			ret =	wyIni::IniWriteString (conn.GetString(), "StorePassword", temp.GetString(), dirnamestr.GetString());
		}

		m_rgbobbkcolor = m_rgbconnection;
		m_rgbobfgcolor = m_rgbconnectionfg;

		yog_enddialog(hwnd,(wyInt32)1);
	}
	else
	{
		ShowMySQLError(hwnd, tunnel, &mysql, NULL, wyTrue);

		//fix a bug, database neme was erasing on error
		//SendMessage(GetDlgItem(hwnd, IDC_DLGCONNECT_DATABASE), WM_SETTEXT, 0,(LPARAM)L"");

		SetFocus(GetDlgItem(hwnd,IDC_DLGCONNECT_DATABASE));
		return;
	}
}

void 
ConnectionCommunity::OnWmCommandConnDialog(HWND hwnd, WPARAM wparam, LPARAM lparam, ConnectionInfo * dbname)
{
    wyInt32 id;

    if(HIWORD(wparam) == EN_UPDATE)
    {
		id = LOWORD(wparam);
		
		if(id != IDC_DLGCONNECT_PASSWORD ||	(id == IDC_DLGCONNECT_PASSWORD &&
		   Button_GetCheck(GetDlgItem(hwnd, IDC_DLGCONNECT_STOREPASSWORD))== BST_CHECKED))
        {
			EnableWindow(GetDlgItem(hwnd, IDC_SAVE), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_CLONECONN), FALSE);
			m_conndetaildirty = wyTrue;
			return;
		}
	}
	//handle connection name combobox
    /*else if(LOWORD(wparam) == IDC_DESC)
    {
		//if different connection is selected, then save the previous one
		 if((HIWORD(wparam)== CBN_DROPDOWN) && (m_conndetaildirty  == wyTrue))
		 {
			 PostMessage(hwnd, SHOW_CONFIRM, 0, 0);
			 m_conndetaildirty = wyFalse;
		 }
		else if((HIWORD(wparam) == CBN_CLOSEUP) || (HIWORD(wparam) == CBN_SELENDOK))
		{
			if((m_conndetaildirty == wyTrue) && (m_isnewconnection == wyTrue))
			{
				SendMessage(GetDlgItem(hwnd, IDC_DESC), CB_DELETESTRING, m_newconnid, 0);
				
				m_conndetaildirty = wyFalse;
				m_isnewconnection = wyFalse;
				m_newconnid = -1;
			}

			//if connection is selected using keyboard or mouse, display the contents
			GetInitialDetails(hwnd);
			
			/* we have to change the tab selection to server also */
			/*TabCtrl_SetCurSel(GetDlgItem(hwnd, IDC_CONNTAB), 0);
			
			ShowServerOptions(hwnd);
			m_conndetaildirty = wyFalse;
			return;
		}
		else if((HIWORD(wparam)== CBN_SELCHANGE))
		{
			SetFocus(GetDlgItem(hwnd, IDC_DESC));
		}
	}*/
	//handle connection color combobox
	else if(LOWORD(wparam)== IDC_COLORCOMBO||LOWORD(wparam)== IDC_COLORCOMBO3)
	{
		if((HIWORD(wparam) == CBN_DROPDOWN) || (HIWORD(wparam)== CBN_SELCHANGE))
		{
			//Command handler for color combo box
			
			OnWmCommandColorCombo(hwnd, wparam, lparam);
			dbname->m_rgbconn = m_rgbconnection;
			dbname->m_rgbfgconn = m_rgbconnectionfg;
			m_rgbobbkcolor = m_rgbconnection;
			m_rgbobfgcolor = m_rgbconnectionfg;

			//if color is changed in combo, then set m_conndetaildirty to true
			if((HIWORD(wparam)== CBN_SELCHANGE) && (m_conndetaildirty == wyFalse))
			{
				if(m_changeobcolor == wyFalse)
					m_conndetaildirty = wyFalse;
				else
					m_conndetaildirty  = wyTrue;
			}
		}
		
		if(m_conndetaildirty == wyTrue)
		{
			EnableWindow(GetDlgItem(hwnd, IDC_SAVE), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_CLONECONN), FALSE);
			dbname->m_rgbconn = m_rgbconnection;
			dbname->m_rgbfgconn = m_rgbconnectionfg;
			m_rgbobbkcolor = m_rgbconnection;
			m_rgbobfgcolor = m_rgbconnectionfg;
		}
		
		return;
	}

	//Handles the common option in 'MySQL' tab
	HandleCommonConnectOptions(hwnd, dbname, LOWORD(wparam));
	
	switch(LOWORD(wparam))
	{
	case IDOK:
        OnConnect(hwnd, dbname);
		break;

	case IDCANCEL:
		yog_enddialog(hwnd, 0);
		break;

	case IDC_DELETE:
		DeleteConnDetail(hwnd);
		break;

	case IDC_NEW:
		if(m_conndetaildirty == wyTrue)
		{
			PostMessage(hwnd, SHOW_CONFIRM, 0, 0);
		}
		m_conndetaildirty = wyFalse;
		NewConnection(hwnd);
		break;

	case IDC_SAVE:
		SaveConnection(hwnd);
		m_conndetaildirty = wyFalse;
		EnableWindow(GetDlgItem(hwnd, IDC_SAVE), FALSE);
		//EnableWindow(GetDlgItem(hwnd, IDC_CLONECONN), TRUE);
		break;

	case IDC_TESTCONN:
		OnTestConnection(hwnd);
		break;
	
	case IDC_CLONECONN:
		CloneConnection(hwnd);
		break;

	case IDC_TUNNELHELP:
		ShowHelp("HTTP%20Tunneling%20SQLyog%20MySQL%20Client.html");
		break;

    case IDC_INITCOMMANDHELP:
        ShowHelp("Advanced%20Connection%20SQLyog%20MySQL%20Manager.htm");
        break;

	case IDC_SSHHELP:
		ShowHelp("SSH%20Tunneling%20SQLyog%20MySQL%20Client.html");
		break;

    case IDC_SSLHELP:
        ShowHelp("SSL%20encryption%20SQLyog%20MySQL%20Client.html");
		break;

	case IDC_TIMEOUTHELP:
	case IDC_COMPRESSHELP:
		 ShowHelp("Direct%20Connection%20SQLyog%20MySQL%20Front%20End.htm");
		 break;
	}
}

wyBool 
ConnectionCommunity::OnNewSameConnection(HWND hwndactive, MDIWindow *pcquerywnd, ConnectionInfo &conninfo)
{
    MYSQL       *mysqlnew, *mysqlold, *mysqltemp;
    Tunnel	    *oldtunnel, *newtunnel;	
    wyString    msg;

	if(hwndactive)
	{
		oldtunnel   = pcquerywnd->m_tunnel;
        newtunnel   = CreateTunnel(wyFalse);
        mysqlnew	= newtunnel->mysql_init((MYSQL*)0);
        mysqlold	= pcquerywnd->m_mysql;

		if(!mysqlnew)
		{
			yog_message(pGlobals->m_pcmainwin->m_hwndmain, _(L"Could not initialize a new window."), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK | MB_HELP);
			delete newtunnel;
			return wyFalse;
		}
		
		SetCursor(LoadCursor(NULL, IDC_WAIT	));
		ShowCursor(1);

        InitConInfo(pcquerywnd->m_conninfo, conninfo);

		//Get SQL_MODE info //following function remarked for issue #1654
		//GetAdvancedTabDetailsFromFile(&conninfo, (wyChar *)pcquerywnd->m_currentconn.GetString(), NULL);
	
		//post 8.01
		/*InvalidateRect(pcquerywnd->GetActiveTabEditor()->m_peditorbase->m_hwnd, NULL, FALSE);
		UpdateWindow(pcquerywnd->GetActiveTabEditor()->m_peditorbase->m_hwnd);*/
		
        if(conninfo.m_initcommand.GetLength())
            ExecuteInitCommands(mysqlnew, newtunnel, conninfo.m_initcommand);

		SetMySQLOptions(&pcquerywnd->m_conninfo, newtunnel, &mysqlnew);
		
        mysqltemp = newtunnel->mysql_real_connect(mysqlnew, mysqlold->host, mysqlold->user, 
                    mysqlold->passwd, NULL, mysqlold->port, NULL, 
                    mysqlold->client_flag | CLIENT_MULTI_RESULTS, NULL);
		
		if(!mysqltemp)
        {
			ShowMySQLError(pGlobals->m_pcmainwin->m_hwndmain, oldtunnel, &mysqlnew, NULL, wyTrue);
			newtunnel->mysql_close(mysqlnew);
			delete	newtunnel;
			return wyFalse;
		}

		/* change the conninfo things */
		conninfo.m_tunnel		= newtunnel;
		conninfo.m_mysql		= mysqlnew;
		conninfo.m_hprocess	    = INVALID_HANDLE_VALUE;
	}
    return wyTrue;
}
// Callback function for the connect dialog box.
INT_PTR CALLBACK
ConnectionCommunity::ConnectDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ConnectionInfo *dbname = (ConnectionInfo*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_INITDIALOG:
        SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
        LocalizeWindow(hwnd);
		pGlobals->m_pcmainwin->m_connection->OnInitDialog(hwnd, lParam);		
        break;

	case WM_INITCONNDIALOG:
		pGlobals->m_pcmainwin->m_connection->OnInitConnDialog(hwnd);
		break;

	case SHOW_CONFIRM:
		pGlobals->m_pcmainwin->m_connection->ConfirmAndSaveConnection(hwnd);
		break;

	case WM_COMMAND:
        pGlobals->m_pcmainwin->m_connection->OnWmCommandConnDialog(hwnd, wParam, lParam, dbname);
		break;

	case WM_MEASUREITEM:
		OnComboMeasureItem((LPMEASUREITEMSTRUCT)lParam);
		break;

	case WM_DRAWITEM:
		//draw connection color with connection name
		if(LOWORD(wParam) == IDC_COMBOCUSTOM)
		{
            HWND hwndCombo = (HWND)SendMessage(GetDlgItem(hwnd, IDC_DESC), CCBM_GETCOMBOHWND, NULL, NULL);
			OnDrawConnNameCombo(hwndCombo, (LPDRAWITEMSTRUCT)lParam);

			//OnDrawConnNameCombo(GetDlgItem(hwnd, IDC_DESC), (LPDRAWITEMSTRUCT)lParam);
		}
		else if(LOWORD(wParam) == IDC_COLORCOMBO) 
		{
			OnDrawColorCombo(hwnd, (LPDRAWITEMSTRUCT)lParam, pGlobals->m_pcmainwin->m_connection->m_rgbconnection);
		}
		else if(LOWORD(wParam) == IDC_COLORCOMBO3)
		{
			OnDrawColorCombo(hwnd, (LPDRAWITEMSTRUCT)lParam, pGlobals->m_pcmainwin->m_connection->m_rgbconnectionfg);
		}
		break;

	case WM_NOTIFY:
        pGlobals->m_pcmainwin->m_connection->OnWmConnectionNotify(hwnd, lParam);
        break;

	case WM_DESTROY:
		if(pGlobals->m_pcmainwin->m_geninfo)
		{
			DestroyWindow(pGlobals->m_pcmainwin->m_geninfo);
			pGlobals->m_pcmainwin->m_geninfo = NULL;
		}
        EnableDisableExportMenuItem();
		break;

	case WM_HELP:
		pGlobals->m_pcmainwin->m_connection->OnWmConnectionHelp(hwnd);
		return 1;
        break;
    case CCBM_NOTIFY:
        pGlobals->m_pcmainwin->m_connection->HandleCustomComboNotifiction(hwnd, wParam, lParam);
        break;
    case CCBM_ERASEBKGND:
        wyInt32 ret = pGlobals->m_pcmainwin->m_connection->OnComboEraseBkGnd(hwnd, wParam, lParam);
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, ret);
        return TRUE;
        break;
	}

	return 0;
}

wyInt32 
ConnectionCommunity::ActivateDialog(ConnectionInfo *conninfo)
{
    wyInt32 ret = 0;
	

	// if click on buy or on image of start up dialog will go to website 
	if(pGlobals->m_pcmainwin->m_connection->m_linkadd.GetLength() == 0)
	{
		ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_CONNECT), pGlobals->m_pcmainwin->m_hwndmain, ConnectDialogProc, (LPARAM)conninfo);
	}
	else
	{
		ShowWindow(pGlobals->m_pcmainwin->m_hwndmain, SW_NORMAL);
		
		//Execute command 'ShellExecute'
		OnClickBuy();		
	}
	return ret;
}

void 
ConnectionCommunity::HandleMenu(wyInt32 menuindex, HMENU hmenu)
{
	switch(menuindex)
	{
	case MNUFILE_INDEX:
		ChangeFileMenuItem(hmenu);
		break;

	case MNUEDIT_INDEX:					
		ChangeEditMenuItem(hmenu);
		break;

	case MNUFAV_INDEX :
		EnableFavoriteMenu(hmenu);
		break;
	
	case MNUDB_INDEX:
		EnableDBItems(hmenu);
		break;

	case MNUTBL_INDEX:
		EnableTableItems(hmenu);
		break;

	case MNUOBJ_INDEX:
		EnableColumnItems(hmenu);
		break;

	case MNUTOOL_INDEX:
		EnableToolItems(hmenu);
		break;

    case MNUWINDOW_INDEX:
        EnableWindowItems(hmenu);
        break;

    case MNUHELP_INDEX:
        EnableHelpItems(hmenu);
        break;
	}
    return;
}


void 
ConnectionCommunity::HandleHelp(HWND hwndhelp,  wyInt32 *hpos, wyInt32 *vpos, wyInt32 *width, wyInt32 *height, wyInt32 rectbottom)
{
    ::SendMessage(hwndhelp, WM_SETREDRAW,  TRUE, NULL);
		
	VERIFY(MoveWindow(hwndhelp, 0, 0, 0, 0, TRUE));
		
	::SendMessage(hwndhelp, WM_SETREDRAW,  TRUE, NULL);

    return;
}

void
ConnectionCommunity::OnClickBuy()
{
	ShellExecute(NULL, L"open",pGlobals->m_pcmainwin->m_connection->m_linkadd.GetAsWideChar(),
				        NULL, NULL, SW_MAXIMIZE);
	pGlobals->m_pcmainwin->m_connection->m_linkadd.SetAs("");		
}


// Function creates of menuid and its corresponding m_icons
yogIcons * 
ConnectionCommunity::CreateIconList(HWND hwndmain, wyUInt32 *numicons)
{
	wyInt32     iconcount;
	wyInt32     size;
    HMENU       menu;
    yogIcons    *icons; 

	wyUInt32    ids[] = { 
		IDM_FILE_NEWCONNECTION,			IDM_FILE_CLOSECONNECTION, 
        IDM_FILE_CLOASEALL,				IDM_FILE_SAVESQL, 
		IDM_FILE_CLOSECONNECTION,		IDM_FILE_CLOASEALL, 
		IDM_EDIT_CUT,					IDM_EDIT_COPY, 
        IDM_EDIT_PASTE,					IDM_REFRESHOBJECT,	
		IDM_DB_REFRESHOBJECT,			IDM_HELP_SHORT,
		IDM_TOOL_ADDUSER,				IDM_TOOL_RENAMEUSER,
		IDM_TOOL_EDITUSER,              ID_TOOLS_USERMANAGER,				
		ACCEL_EXECUTE_MENU,				ACCEL_EXECUTEALL, 
		ACCEL_EXECUTESEL,				ACCEL_QUERYUPDATE, 
		ID_OBJECT_DROPTABLE,			IDM_WINDOW_CASCADE,
		ID_OBJECT_ADVANCED,				ID_OBJECT_REORDER,
        ID_OBJECT_VIEWDATA,				ID_OBJECT_DROPFIELD, 
		ID_OBJECT_MANINDEX,				ID_OBJECT_MAINMANINDEX, 
        IDM_TABLE_RELATION,				ID_OBJECT_TABLEEDITOR, 
		ID_EXPORT_AS,					
		ID_TABLE_MAKER,					ID_OBJECT_DROPDATABASE, 
        IDM_ALTERDATABASE,				IDM_WINDOW_TILE,
		ID_IMPORTEXPORT_DBEXPORTTABLES, ID_IMPORTEXPORT_TABLESEXPORTTABLES, 
		ID_IMPORTEXPORT_EXPORTTABLES,	ID_OBJECT_CLEARTABLE, 
		ID_OPEN_COPYDATABASE,			ID_OPEN_COPYTABLE, 
		IDM_CREATEDATABASE,				IDC_PREF, 
		IDM_IMEX_EXPORTDATA,			ID_IMEX_TEXTFILE, 
		IDM_TOOLS_TABLEDIAG,			IDM_HELP_HELP, 
		ID_IMPORTEXPORT_DBEXPORTTABLES2,ID_IMEX_TEXTFILE2, 
		ID_OBJECT_CREATEVIEW,			ID_OBJECT_ALTERVIEW, 
		ID_OBJECT_DROPVIEW,				ID_NEW_EDITOR, 
		ID_EXPORT_EXPORTTABLEDATA, 		ID_OBJECT_EXPORTVIEW, 
		IDM_TOOL_JOBMAN,				IDC_TOOLS_NOTIFY, 
		ID_IMPORT_EXTERNAL_DATA,		ID_POWERTOOLS_SCHEDULEEXPORT2, 
		IDM_DATASYNC,					IDC_DIFFTOOL, 
		ID_QUERYBUILDER,				ID_SCHEMADESIGNER, 
		ID_REBUILDTAGS,					ACCEL_FORMATALLQUERIES,
		ACCEL_FORMATSELECTEDQUERY,		ACCEL_FORMATCURRENTQUERY,			
										IDM_FILE_EXIT,
		IDM_FILE_NEWSAMECONN,			ID_FILE_CLOSETAB,
		IDM_FILE_OPENSQL,				ID_OBJECT_CREATEFUNCTION,
		ID_OBJECT_CREATETRIGGER,		ID_OBJECT_ALTERTRIGGER,
		ID_OBJECT_DROPTRIGGER,			ID_OBJECT_ALTERFUNCTION,
		ID_OBJECT_DROPFUNCTION,			ID_OBJECT_CREATEEVENT,
		ID_OBJECT_ALTEREVENT,			ID_OBJECT_DROPEVENT,
		ID_OBJECT_CREATESTOREDPROCEDURE,ID_OBJECT_ALTERSTOREDPROCEDURE,
		ID_OBJECT_DROPSTOREDPROCEDURE,	ID_ADDTOFAVORITES,
		ID_ORGANIZEFAVORITES,			ID_REFRESHFAVORITES,
		IDM_TOOLS_TABLEDIAG,			IDC_ROW_MSG,
		ID_TEST_TEST3,					ID_DELETE_LINE,
		ID_OBJECT_COPYTABLE,			ID_OBJECT_RENAMETABLE,
		ID_OBJECT_RENAMEEVENT,			ID_OBJECTS_RENAMETRIGGER,
		ID_OBJECTS_RENAMEVIEW,			ID_COLUMNS_MANAGECOLUMNS,
		ID_IMPORT_FROMCSV,				ID_OBJECT_CREATESCHEMA,
		IDM_EDIT_UNDO,					IDM_EDIT_REDO,
		IDM_EDIT_FIND,					IDM_EDIT_REPLACE,
		ID_EDIT_INSERTTEMPLATES,		ID_OBJECT_TRUNCATEDATABASE,
		ID_TOOLS_FLUSH,					IDM_TOOL_JOBMAN,
		ID_OBJECT_EMPTYDATABASE,        IDM_FILE_OPENSQLNEW,
		ID_DB_TABLE_MAKER,				ID_DB_CREATEVIEW,
		ID_DB_CREATESTOREDPROCEDURE,	ID_DB_CREATEFUNCTION,
		ID_DB_CREATETRIGGER,			ID_DB_CREATEEVENT,
		IDM_FILE_SAVEAS,				IDM_HELP_ABOUT,
        IDM_OBCOLOR,					ID_DATASEARCH,
		ID_DB_DATASEARCH,				ID_HISTORY,									
        ID_INFOTAB,                     ID_EDIT_LISTALLTAGS,
        ID_EDIT_LISTMATCHINGTAGS,       ID_EDIT_LISTFUNCTIONANDROUTINEPARAMETERS,
		ID_OBJECT_COPY,								ID_OBJECT_FIND,
		ID_OPEN_COPY,								ID_QUERYINFO_FIND,
        ID_OPEN_SAVEHISTORY,            ID_VDDTOOL
	};

	wyUInt32    iconid[] = { 
		IDI_CONNECT_16,		IDI_DISCON, 
		IDI_DISCONALL,		IDI_SAVE, 
        IDI_DISCON,			IDI_DISCONALL, 
		IDI_CUT,			IDI_COPY, 
		IDI_PASTE,			IDI_REFRESH_16,
		IDI_REFRESH_16,		IDI_KEYSHORT,
		IDI_ADDUSER,		IDI_RENAMEUSER,
		IDI_EDITUSER,       IDI_USER_16,			
		IDI_EXECUTE_16,		IDI_EXECUTEALL_16, 
		IDI_EXECUTE_16,		IDI_EXECUTEFORUPD_16, 
		IDI_DROPTABLE,		IDI_CASCADE,
		IDI_TABLEPROP,		IDI_REORDERCOL,
		IDI_VIEWDATA,       IDI_DROPCOLUMN, 
		IDI_MANINDEX_16,	IDI_MANINDEX_16, 
		IDI_MANREL_16,		IDI_ALTERTABLE,
		IDI_CSV,			
		IDI_CREATETABLE,	IDI_DROPDB, 
		IDI_ALTERDB,		IDI_TILE, 
		IDI_EXPORTDATA_16,	IDI_EXPORTDATA_16, 
		IDI_EXPORTDATA_16,	IDI_EMPTYTABLE, 
        IDI_COPYDATABASE_16,IDI_COPYDATABASE, 
        IDI_NEWDATABASE,	IDI_PREF,
		IDI_XMLHTML_16,		IDI_EXECBATCH_16, 
		IDI_TABLEDIAG,		IDI_HELP, 
		IDI_EXPORTDATA_16,	IDI_EXECBATCH_16,
		IDI_CREATEVIEW,		IDI_ALTERVIEW, 
		IDI_DROPVIEW,		IDI_QUERY_16, 
		IDI_XMLHTML_16,		IDI_EXPORTVIEW, 
		IDI_ENTMENU,		IDI_ENTMENU, 
		IDI_ENTMENU,		IDI_ENTMENU, 
		IDI_ENTMENU,		IDI_ENTMENU, 
		IDI_ENTMENU,		IDI_ENTMENU, 
		IDI_ENTMENU,		IDI_ENTMENU, 
		IDI_ENTMENU,		IDI_ENTMENU,
							IDI_EXITICO,
		IDI_NEWQUERY,		IDI_CLOSETAB,
		IDI_FOLDER,			IDI_CREATEFUNCTION,
		IDI_CREATETRIGGER,	IDI_ALTERTRIGGER,
		IDI_DROPTRIGGER,	IDI_ALTERFUNCTION,
		IDI_DROPFUNCTION,	IDI_CREATEEVENT,
		IDI_ALTEREVENT,		IDI_DROPEVENT,	
		IDI_CREATEPROCEDURE,IDI_ALTERPROCEDURE,
		IDI_DROPPROCEDURE,	IDI_ADDTOFAV,
		IDI_FAVORITES,		IDI_FAVREFRESH,
		IDI_TABLEDIAG,		IDI_INDEXCREATE,
		IDI_INDEXEDIT,		IDI_INDEXDROP,
		IDI_COPYTABLE,		IDI_RENAME,
		IDI_RENAMEEVENT,	IDI_RENAMETRIGGER,
		IDI_RENAMEVIEW,		IDI_COLUMN,
		IDI_CSV,			IDI_SCHEMA,
		IDI_UNDOSM,			IDI_REDOSM,
		IDI_FIND,			IDI_REPLACE,
		IDI_TEMPLATE,		IDI_TRUNCDB,
		IDI_FLUSH,			IDI_ENTMENU,
		IDI_EMPTYDB,		IDI_OPENINNEWTAB,
		IDI_CREATETABLE,	IDI_CREATEVIEW,
		IDI_CREATEPROCEDURE, IDI_CREATEFUNCTION,
		IDI_CREATETRIGGER,	IDI_CREATEEVENT,
		IDI_SAVEAS,			IDI_MAIN,
        IDI_CONNCOLOR,		IDI_ENTMENU,
		IDI_ENTMENU,		IDI_HISTORY,			
		IDI_TABLEINDEX,     IDI_ENTMENU,
        IDI_ENTMENU,        IDI_ENTMENU,
		IDI_COPY,				IDI_FIND,
		IDI_COPY,				IDI_FIND,
        IDI_SAVE,           IDI_ENTMENU

	}; //From 7.0 version we are adding all the menus in Enterprise to Community. for this we are using IDI_ENTMENU.

	*numicons = size = sizeof(ids)/ sizeof(ids[0]);
	VERIFY((sizeof(ids)/ sizeof(ids[0]))==(sizeof(iconid)/ sizeof(iconid[0]))); 
	*numicons = size = sizeof(ids)/ sizeof(ids[0]);
	icons = new yogIcons[*numicons];

	VERIFY(menu = GetMenu(hwndmain));

	for(iconcount = 0; iconcount < size; iconcount++)
    {
		icons[iconcount].m_iconid = iconid[iconcount];
		icons[iconcount].m_menuid = ids[iconcount];
	}

	// sort it.
	qsort(icons, size, sizeof(yogIcons), compareid);

	return icons;
}

void 
ConnectionCommunity::HandleEditorControls(HWND hwnd, HWND hwndhelp, HWND hwndfilename, HWND hwndsplitter, 
                                          HWND hwndtabmgmt, wyBool save, wyInt32 tabcount, wyInt32 selindex)
{
    if(tabcount == selindex)
	{
	    ShowWindow(hwnd , TRUE);
		ShowWindow(hwndsplitter , TRUE);
		ShowWindow(hwndtabmgmt , TRUE);
	}
	else
	{
		ShowWindow(hwnd , FALSE);	
		ShowWindow(hwndhelp , FALSE);	
		ShowWindow(hwndsplitter , FALSE);

		ShowWindow(hwndtabmgmt , FALSE);
	}
    return;
}

void 
ConnectionCommunity::OnExport(MDIWindow *wnd, WPARAM wparam)
{
    wnd->m_pcqueryobject->ExportData(wnd->m_tunnel, &wnd->m_mysql, NULL, LOWORD(wparam));
    return;
}

wyBool
ConnectionCommunity::CreateSourceInstance(CopyDatabase *copydb)
{
	MYSQL *tempmysql, *newsrcmysql;

	copydb->m_newsrctunnel   = NULL;
	copydb->m_srcprocess     = INVALID_HANDLE_VALUE;

	// create a tunnel and mysql object
    copydb->m_newsrctunnel = CreateTunnel(wyFalse);

	VERIFY(tempmysql = copydb->m_newsrctunnel->mysql_init((MYSQL*)0));
	
	//if(IsMySQL41(copydb->m_newtargettunnel, &tempmysql))
		//VERIFY((copydb->m_newtargettunnel->mysql_options(tempmysql, MYSQL_SET_CHARSET_NAME, "utf8")));

	VERIFY(!(copydb->m_newsrctunnel->mysql_options(tempmysql, MYSQL_INIT_COMMAND, "/*40030 SET net_write_timeout=3600 */")));

	SetMySQLOptions(copydb->m_srcinfo, copydb->m_newsrctunnel, &tempmysql, wyTrue);

	newsrcmysql = copydb->m_newsrctunnel->mysql_real_connect(tempmysql, 
                             (*copydb->m_srcmysql)->host, (*copydb->m_srcmysql)->user, 
                             (*copydb->m_srcmysql)->passwd, NULL, 
                             (*copydb->m_srcmysql)->port, NULL, 
                             (*copydb->m_srcmysql)->client_flag | CLIENT_MULTI_RESULTS, NULL);

	if(!newsrcmysql)
    {
		ShowMySQLError(copydb->m_hwnddlg, copydb->m_newsrctunnel, &tempmysql, NULL, wyTrue);
		return wyFalse;
	}
	if(UseDatabase(copydb->m_srcdb, newsrcmysql, copydb->m_newsrctunnel) == wyFalse)
	{
		ShowMySQLError(copydb->m_hwnddlg, copydb->m_newsrctunnel, &tempmysql, NULL, wyTrue);
		return wyFalse;
	}
	copydb->m_newsrctunnel->SetServerInfo(newsrcmysql, 
                    copydb->m_srctunnel->mysql_get_server_info(*copydb->m_srcmysql));
	copydb->m_newsrcmysql = newsrcmysql;
	
	return wyTrue;
}

wyBool
ConnectionCommunity::CreateTargetInstance(CopyDatabase *copydb)
{
	MYSQL *tempmysql, *newtargetmysql;
   
	copydb->m_newtargettunnel   = NULL;
	copydb->m_tgtprocess        = INVALID_HANDLE_VALUE;

	// create a tunnel and mysql object
    copydb->m_newtargettunnel = CreateTunnel((wyBool)copydb->m_targettunnel->IsTunnel());
		
	VERIFY(tempmysql = copydb->m_newtargettunnel->mysql_init((MYSQL*)0));
		
	SetMySQLOptions(copydb->m_tgtinfo, copydb->m_newtargettunnel, &tempmysql);

	newtargetmysql = copydb->m_newtargettunnel->mysql_real_connect(tempmysql, 
                             (*copydb->m_targetmysql)->host, (*copydb->m_targetmysql)->user, 
                             (*copydb->m_targetmysql)->passwd, NULL, 
                             (*copydb->m_targetmysql)->port, NULL, 
                             (*copydb->m_targetmysql)->client_flag | CLIENT_MULTI_RESULTS, NULL);

	if(!newtargetmysql)
    {
		ShowMySQLError(copydb->m_hwnddlg, copydb->m_newtargettunnel, &tempmysql, NULL, wyTrue);
		return wyFalse;
	}
	
	if(UseDatabase(copydb->m_targetdb, newtargetmysql, copydb->m_newtargettunnel) == wyFalse)
	{
		ShowMySQLError(copydb->m_hwnddlg, copydb->m_newtargettunnel, &tempmysql, NULL, wyTrue);
		return wyFalse;
	}
	copydb->m_newtargettunnel->SetServerInfo(newtargetmysql, 
                                copydb->m_targettunnel->mysql_get_server_info(*copydb->m_targetmysql));
	copydb->m_newtargetmysql = newtargetmysql;
	
	return wyTrue;
}

wyBool 
ConnectionCommunity::OnWmCommand(HWND hwndactive, MDIWindow *wnd, WPARAM wparam)
{
    switch(LOWORD(wparam))
    {
    case ACCEL_DATASYNC:
	case IDM_DATASYNC:
    case IDC_DIFFTOOL:
	case ACCEL_DIFFTOOL:
	case ACCEL_NOTIFY:
	case IDC_TOOLS_NOTIFY:
    case ACCEL_ODBC:
	case ID_IMPORT_EXTERNAL_DATA:
    case ACCEL_SCHDEXPORT:
	case ID_POWERTOOLS_SCHEDULEEXPORT2:
    case ID_POWERTOOLS_SCHEDULEEXPORT:
    case IDM_TOOL_JOBMAN:
 	case ID_REBUILDTAGS:
    case ID_EDIT_LISTALLTAGS:
	case ID_EDIT_LISTMATCHINGTAGS:
    case ID_EDIT_LISTFUNCTIONANDROUTINEPARAMETERS:
    case ACCEL_QUERYBUILDER:
    case ID_QUERYBUILDER:           
	case ACCEL_SCHEMADESIGNER:
	case ID_SCHEMADESIGNER:
	case ACCEL_FORMATALLQUERIES:
	case ID_FORMATCURRENTQUERY:
	case ACCEL_FORMATCURRENTQUERY:
	case ACCEL_FORMATSELECTEDQUERY:	
		{
			m_powertoolsID = LOWORD(wparam);
			GetSQLyogEntDialog();
			m_powertoolsID = 0;
		}
		break;

    case ACCEL_DATASEARCH:
	case ID_DATASEARCH:
	case ID_DB_DATASEARCH:
    case ACCEL_VDDTOOL:
    case ID_VDDTOOL:
		{
			m_powertoolsID = LOWORD(wparam);
			GetSQLyogUltimateDialog();
			m_powertoolsID = 0;
		}
	    break;
	}
    return wyFalse;
}


wyBool  
ConnectionCommunity::CheckRegistration(HWND hwnd, void *main)
{     
	HKEY		key;
	DWORD       dwdisposition;

	 //ShowDialog(hwnd);
	
	// create the registry.
	VERIFY((RegCreateKeyEx(HKEY_CURRENT_USER, TEXT(REGKEY), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &key, &dwdisposition))== ERROR_SUCCESS);
	
	return wyTrue;
}

void 
ConnectionCommunity::OnClose()
{	
	m_isapclose = wyTrue;
	ShowDialog(pGlobals->m_pcmainwin->m_hwndmain);
	return;
}

 wyInt32 
 ConnectionCommunity::ShowDialog(HWND hwndparent)
 {
    wyInt32 ret;

    ret = DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_STARTDLG), 
						   hwndparent , DlgProc, (LPARAM)this);

	return	ret;
 }

INT_PTR CALLBACK
ConnectionCommunity::DlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	ConnectionCommunity*	pcomm	=	(ConnectionCommunity*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    HWND hwndactive;

	switch(message)
	{
	case WM_INITDIALOG:
		{
			SetWindowLongPtr(hwnd, GWLP_USERDATA,(LONG_PTR)lparam);
            LocalizeWindow(hwnd);
			PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);			
		}
		break;

    case WM_INITDLGVALUES:
		hwndactive = GetDlgItem(hwnd, IDC_COMLINK);
        pcomm->m_wporigstaticproc = (WNDPROC)SetWindowLongPtr(hwndactive, GWLP_WNDPROC,(LONG_PTR)StaticDlgProc);	
		SetWindowLongPtr(hwndactive, GWLP_USERDATA,(LONG_PTR)pcomm);
		break;

	case WM_COMMAND:
        pcomm->OnDlgWmCommand(hwnd, wparam);
        break;	
	}
	return 0;
}

void            
ConnectionCommunity::OnDlgWmCommand(HWND hwnd, WPARAM wparam)
{
    wyInt32  ret = LOWORD(wparam);	
    
    switch(ret)
    {
    case IDCONTINUE:
        EndDialog(hwnd, 1);
        break;
	case IDC_BUY_LINK:	
		EndDialog(hwnd, 1);
		//m_linkadd.SetAs(BUYURL_BUYNAGBEGIN);
		
		if(m_isapclose == wyTrue)
			ShellExecute(NULL, L"open", TEXT(BUYURL_BUYNAGEND), NULL, NULL, SW_SHOWNORMAL);

				
		break;

    }

    if((HIWORD(wparam)== STN_CLICKED))
    {
	    if(LOWORD(wparam)== IDC_COMLINK )
		{
			if(m_isapclose == wyTrue)
			{
				//SHELLEXECUTE on press 'buy'on nag screen on closing application
				pGlobals->m_pcmainwin->m_connection->m_linkadd.SetAs(BUYURL_NAGEND);
				OnClickBuy();				
				EndDialog(hwnd, 1);				
			}

			/*else
			{
				m_linkadd.SetAs(BUYURL_NAGBEGIN);
				EndDialog(hwnd, 1);				
			}*/
		}
	}
}

LRESULT CALLBACK
ConnectionCommunity::StaticDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ConnectionCommunity *pcomm	= (ConnectionCommunity*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(message)
	{
	case WM_MOUSEMOVE:
		SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(32649)));
		return 0;
	}

    return CallWindowProc(pcomm->m_wporigstaticproc, hwnd, message, wParam, lParam);
}

// Function to initialize and set the image list in the combobox of the toolbar.
wyBool
ConnectionCommunity::SetToolComboImageList(FrameWindow *main)
{
	wyInt32 iconid, count;
    wyInt32 icons[] = { IDI_DATABASE, IDI_SERVERGROUP};				

	HICON   icon;

	main->m_hcomboiml = ImageList_Create(DEF_IMG_WIDTH, DEF_IMG_HEIGHT, ILC_COLOR32 | ILC_MASK, 1, 0);

	for(count = 0; count < sizeof(icons)/sizeof(icons[0]); count++)
	{
		VERIFY(icon =(HICON)LoadImage(main->m_hinstance, MAKEINTRESOURCE(icons[count]), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
		VERIFY((iconid = ImageList_AddIcon(main->m_hcomboiml, icon))!= -1);
		VERIFY(DestroyIcon(icon));
	}

	SendMessage(main->m_hwndtoolcombo, CBEM_SETIMAGELIST, 0,(LPARAM)main->m_hcomboiml);

	return wyFalse;
}

wyInt32
ConnectionCommunity::OnStatusBarWmCtlColorStatic(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    wyInt32   identifier;
	wyInt32  fontheight = 0;	
    HDC	hdc = (HDC)wparam;
    COLORREF clr = RGB(0, 0, 255);

    identifier = GetDlgCtrlID((HWND)lparam);
	if(identifier == IDC_LINK)
	{
		fontheight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		if(!m_hstatusfont)
			m_hstatusfont = CreateFont(fontheight, 0, 0, 0, FW_BOLD, 0, 
            TRUE, 0, 0, 0, 0, 0, 0, L"Verdana");
		SelectObject(hdc, m_hstatusfont);
		SetBkMode(hdc, TRANSPARENT);
        wyTheme::GetLinkColor(LINK_COLOR_FRAMEWINDOW, &clr);
		SetTextColor(hdc, clr);
		return (wyInt32)GetStockObject(HOLLOW_BRUSH);
	} 

    return 0;
}

void 
ConnectionCommunity::HandleTagsMenu(HMENU hmenu)
{
    EnableMenuItem(hmenu, ID_EDIT_LISTALLTAGS, MF_ENABLED);
    EnableMenuItem(hmenu, ID_EDIT_LISTMATCHINGTAGS, MF_ENABLED);
    EnableMenuItem(hmenu, ID_EDIT_LISTFUNCTIONANDROUTINEPARAMETERS, MF_ENABLED);
    return;
}

void 
ConnectionCommunity::CloseThreads(MDIWindow *wnd)
{
    return;
}

void 
ConnectionCommunity::OnWmConnectionHelp(HWND hwnd)
{
	int tab = TabCtrl_GetCurSel(GetDlgItem(hwnd, IDC_CONNTAB));

	switch(tab)
	{
	case 0:
		ShowHelp("Getting%20started%20SQLyog%20GUI%20for%20MySQL.htm");
		break;
	case 1:
		ShowHelp("HTTP%20Tunneling%20SQLyog%20MySQL%20Client.html");
		break;
	case 2:	
		ShowHelp("SSH%20Tunneling%20SQLyog%20MySQL%20Client.html");
		break;
	case 3:
		ShowHelp("SSL%20encryption%20SQLyog%20MySQL%20Client.html");
		break;
	case 4:
		ShowHelp("Advanced%20Connection%20SQLyog%20MySQL%20Manager.htm");
		break;
	}
    return;
}

void
ConnectionCommunity::RepaintTabs(WPARAM wparam)
{
	MDIWindow	*wnd = NULL;
	wyInt32		imageid = 0;		

	VERIFY(wnd =  GetActiveWin());
	if(!wnd)
		return;

	imageid = wnd->m_pctabmodule->GetActiveTabImage();

	RepaintTabOnSize();		
	
	return;
}

void 
ConnectionCommunity::OnSchdOdbcImport()
{
	GetSQLyogEntDialog();	
	return;
}

void 
ConnectionCommunity::OnScheduleExport()
{
	GetSQLyogEntDialog();
	return;
}

void
ConnectionCommunity::FormateAllQueries(MDIWindow *wnd, HWND hwndeditor, 
									   wyChar *query, wyInt32 typeofformate)
{
	if(hwndeditor && query)
	{
		SendMessage(hwndeditor, SCI_REPLACESEL, TRUE,(LPARAM)query);
	}

	return;
}

void    
ConnectionCommunity::OnConnect(ConnectionInfo *dbname)
{
	Tunnel		*tunnel;
    MYSQL		*mysql;
	HWND		lastfocus = GetFocus();
	
    tunnel = CreateTunnel(wyFalse);

	if( ! tunnel )
	{
		return;
	}

	dbname->m_tunnel = tunnel;

    mysql = ConnectToMySQL(dbname);

	if(mysql == NULL)
    {
        delete tunnel;
		tunnel = NULL;
		SetFocus(lastfocus);
	}
    else if(dbname->m_db.GetLength() ==0 || (IsDatabaseValid(dbname->m_db, mysql, tunnel) == wyTrue))
    {
		dbname->m_mysql = mysql;
		dbname->m_tunnel = tunnel;
		if(!pGlobals->m_conrestore)
		{
			m_rgbobbkcolor = dbname->m_rgbconn;
			m_rgbobfgcolor = dbname->m_rgbfgconn;
		}
		pGlobals->m_isconnected = wyTrue;
	}
	else
	{
		ShowMySQLError(NULL, tunnel, &mysql, NULL, wyTrue);
		return;
	}

}



MYSQL* 
ConnectionCommunity::ConnectToMySQL(ConnectionInfo * coninfo)
{
	Tunnel			*tunnel = coninfo->m_tunnel;
	wyUInt32		client=0;
	MYSQL           *mysql, *temp;
    
	SetCursor(LoadCursor(NULL, IDC_WAIT));

	// FIRST initialize the object.
	VERIFY(mysql = tunnel->mysql_init((MYSQL*)0));
	temp = mysql;

	coninfo->m_hprocess      = INVALID_HANDLE_VALUE;
	coninfo->m_isssh         = wyFalse;
	coninfo->m_origmysqlport = 0;


	if(!tunnel->IsTunnel())
    {
        if(coninfo->m_initcommand.GetLength())
            ExecuteInitCommands(mysql, tunnel, coninfo->m_initcommand);
    }

	if(coninfo->m_isdeftimeout == wyTrue) 
	{	
		coninfo->m_strwaittimeout.SetAs(WAIT_TIMEOUT_SERVER);
	}
	else
	{
		if(!coninfo->m_strwaittimeout.GetLength())
		{
			coninfo->m_strwaittimeout.SetAs(WAIT_TIMEOUT_SERVER);
		}
	}

    SetMySQLOptions(coninfo, tunnel, &mysql);

	mysql	= tunnel->mysql_real_connect(mysql, coninfo->m_host.GetString(), coninfo->m_user.GetString(), coninfo->m_pwd.GetString(), NULL, coninfo->m_port, NULL, client | CLIENT_MULTI_RESULTS | CLIENT_REMEMBER_OPTIONS, coninfo->m_url.GetString());

	if(!mysql)
    {
		ShowMySQLError(NULL, tunnel, &temp, NULL, wyTrue);
		tunnel->mysql_close(temp);
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return NULL;
	}
		
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	return mysql;
}